# Phase 41 — the asynchronous resource loader, the actor, and the compressed container

**2026-08-27.  1857/2052 -> 1886/2085.**  Three whole translation units:
`VILoader` 21 of 23, `VIActor` 3 of 3 and `VICSFFile` 5 of 7 — **33 written, 29
byte-exact, 20 of them on the FIRST build**, on the settled toolchain
(`ee-gcc 2.95.3-107 -O2 -G0 -fno-exceptions`, assembled with the `2.95.3-114`
`ps2eeas`) with no re-tuning.

## What the code is

`VILoader` (SUPPORT `0x0044FA78 .. 0x004508D8`) is the engine's **asynchronous
resource loader**, and everything the streaming zone loader pulls in goes
through it.  A *transaction* is a byte range of one open resource file; the
loader keeps a `VIPool<VILoaderTrans>` of them, ONE `VIFile::ReadAsynch` in
flight at a time, and a single bounce buffer they all read into.  `Process`
advances at most one transaction per call: if the current one is reading and
the file reports the read complete it hands the buffer to `VIESFParse` and
walks on; then, if the (possibly new) current transaction has not started, it
issues its read.  `CompleteAll` is that loop spun to exhaustion with the
progress callback fired between iterations.

`VIActor` (`0x00415CD8 .. 0x00415E08`) is one placed thing in a scene, and it
is three functions: a constructor, an empty destructor, and
`CalcObjectToWorld`, which is the whole point —
`m_objectToWorld.Transform(m_hpr, m_scale, m_location)`.

`VICSFFile` (`0x0049F6D0 .. 0x004A0094`) is the **compressed container**: a
`.CSF` is a 40-byte header followed by N zlib streams of at most 256 KB each.
`Decompress` inflates every stream into its own `new char[]` and hands the whole
set to `VIFile`'s scatter/gather **memory** open, so `VIObjFile` then reads a
`.CSF` byte for byte as it reads a flat `.ESF` — which is why the container
layer needed no changes at all when compression was added.  `Compress` is the
authoring end: read the file whole, split it into 256 KB chunks, `compress2` at
level 9 into `n + n/2` of scratch, write the header, then a record and a stream
per chunk.

## Layouts

```
VILoader                                   0x6C4
  +0x000 int                    m_numFiles
  +0x004 VIResourceFile        *m_files          new VIResourceFile[m_numFiles]
  +0x008 VIObjFile              m_objFile        592
  +0x258 VIESFParse             m_esf            1064  <- see below
  +0x680 int                    m_cur            the transaction Process is on
  +0x684 VIPool<VILoaderTrans>  m_trans          Init(100)
  +0x69C VIMap<unsigned,int>    m_map            DictID -> transaction
  +0x6B0 void (*m_progress)(void *)
  +0x6B4 void  *m_progressArg
  +0x6B8 int    m_bufSize
  +0x6BC char  *m_buffer                         m_bufferBase aligned up to 64
  +0x6C0 char  *m_bufferBase                     new char[m_bufSize + 64]

VILoaderTrans     48 (44 of fields, padded by the unsigned long at +0x10)
  +0x00 int      m_state       0 queued, 1 reading, 2 read, 3 done
  +0x04 int      m_refCount
  +0x08 unsigned m_id          DictID, 0 for an anonymous load
  +0x0C int      m_file
  +0x10 unsigned long m_position    rounded DOWN by Align
  +0x18 unsigned m_size             the object's own size
  +0x1C unsigned m_offset           its offset inside the buffer
  +0x20 unsigned m_readSize         the aligned read
  +0x24 unsigned m_readOffset
  +0x28 int      m_zone             -1 for a plain resource

VIResourceFile    200        VIFile m_file +0x00 (192) ; VIResourceDir m_dir +0xC0
VIResourceDir       8        : public VIArray<VIResourceElem>, and NOTHING else
VIResourceElem     16        unsigned long m_offset ; unsigned m_size ; unsigned m_id

VIActor           160
  +0x00 VIMatrix44 m_objectToWorld      +0x60 int  -1
  +0x40 int        0                    +0x64 int  0
  +0x44 VIVect3    m_location           +0x68 int  -1
  +0x50 VIVect3    m_hpr                +0x70 VIVect4
  +0x5C float      m_scale (NOT zeroed) +0x80 int 0  +0x84 int 0
  +0x6C int        (NOT zeroed)         +0x88 VISphere
                                        +0x98 int -1  +0x9C int -1

VICSFHeader        40        m_id 'CESF' ; m_numSegms ; long m_totalCompressed ;
                             long m_totalSize ; long m_offset (= 40) ; m_maxCompressed
VICSFSegm           8        int m_compressed ; int m_size
VICSFFile           8        the progress callback and its argument
```

**`VIESFParse` is 1064 bytes, not 1072.**  Phase 36 measured it off
`VIBattleMusic::Preload`'s frame, where the parser is a stack local and the
`sq` register saves above it round the number up.  `VILoader` holds one as a
MEMBER, between the `VIObjFile` ending at `+0x258` and the `int` at `+0x680`,
and 0x680 − 0x258 is 1064 exactly.  A member settles a size a local cannot.

**`VIResourceDir` is a real class and it emits nothing.**
`ParseResourceFile`'s mangled name spells `R13VIResourceDir`, so it is not a
typedef; but it derives from `VIArray<VIResourceElem>` and adds no member, so
its synthesized constructor and destructor inline to direct calls of the base's
and **no `__13VIResourceDir` symbol exists in the module at all** —
`VILoader::Clear` and `::Allocate` call `__t7VIArray1Z14VIResourceElem` and its
destructor by name.  An empty derived class is invisible except in a mangled
parameter.

## Levers

**ONE shared local, not two.**  Phase 40's "a SECOND LOCAL for the same index"
run backwards, twice.  `VILoader::Process` reads `t.m_file` in both of its two
blocks; with a separate `int f` in each the first block is 5 words wrong, and
with a single `int f;` at function scope assigned in both it is byte-exact.
`VICSFFile::Parse(VIFile &)` has two `for` loops over the same segment count;
one shared `int i` closed it from 11 wrong words to 5.  The tell is a spare
`move` or the choice of a callee-saved register for the index.

**Two guards are one `||`.**  `VILoader::Load`'s range check written as
`if (i < 0) return -1; if (i >= m_numFiles) return -1;` is ONE INSTRUCTION
longer than the target: the first guard's `return -1` fails to cross-jump with
the `return -1` after `m_trans.Add`, so it gets its own `li v0,-1` in a delay
slot instead of sharing the `b epilogue; li v0,-1` block.  Written as
`if (i < 0 || i >= m_numFiles) return -1;` it is byte-exact — as is
`!(i >= 0 && i < m_numFiles)` and `i < 0 || !(i < m_numFiles)`, so it is the
STATEMENT COUNT that matters and not the operator.  Phase 29's guard-chain rule;
phase 38's "the two guards of a function need not be the same shape" is about
two guards that do different things, not one range test written twice.

**Nested guards share ONE return at the bottom.**  `VILoader::Release` written
flat (`if (m_trans.Value(i).m_refCount) return 0;` then the body) is 15 words
wrong in two places at once: its early `return 0` lands inline instead of
cross-jumping to the function's single `move v0,zero`, and its LAST guard
(`if (m_trans.Erase(i) < 0) return -1; return 0;`) if-converts to
`slti`+`movz` where the target branches.  Nesting the whole body inside
`if (m_trans.Value(i).m_refCount == 0) { ... }` fixes both — phase 23's rule,
and phase 13's "`if (x) { … } return 0;` defeats the if-conversion", in one
edit.

**The success path in the `if`, twice.**  `VILoader::Process`'s tail written as
`if (IsAsynchComplete() == 0) return 0;` costs three words; written as
`if (IsAsynchComplete()) { … } return 0;` gcc notices that `v0` already HOLDS
zero on the failing branch and jumps straight to the epilogue.  And
`VILoader::Find`'s exit written as `if (lo > hi) return 0;` puts the `return 0`
at the end of the function; written as `if (lo <= hi) { …; return 1; } return 0;`
gcc moves the returning `if`-body out of line and the `return 0` lands AHEAD of
it, exactly where the target has it (phase 7).

**A declaration order decides which store fills a constructor's delay slot.**
`VICSFFile::Parse(VIFile &)` has a `VIObjFile o;` (a 592-byte local with a
constructor) and a `VIFile::MemSegm *segms = 0;`.  Declared after the VIObjFile,
the pointer's zero store is emitted below the constructor call and `move a0,sp`
takes the delay slot; declared BEFORE it, the zero store takes the delay slot
and the function is byte-exact.  Five words, and the only thing that moved is
which line the source declares first.

**A remainder written back into ITSELF gives the `movz`.**  `VILoader::Align`
rounds its read length up to the file's asynchronous alignment.  Written
`if (m) t.m_readSize += a - m;` gcc branches; written
`if (m) m = a - m; t.m_readSize += m;` it emits `subu s0,a,m` + `movz s0,m,m`
and one `addu` — phase 38's "ONE variable, not a separate `t`" applied to an
align-up, and the dual of phase 40's if/ELSE form (which is for an align-up
whose *input* must not be reassociated).

**The mask's SIGNEDNESS is visible in `VILoader::Allocate`.**  `& -64` is one
`li`; `& ~63u` is `lui 0xffff` + `ori 0xffc0`, which is what the target has —
and building the constant in `v0` is also what pushes the freshly-`new`ed
pointer into `v1`, so one character of source fixed 13 wrong words including a
register assignment that looked like the standing phenomenon.

**`sw`/`lw` on a zlib out-parameter says the typedef is 32-bit.**  `uncompress`
and `compress2` take a length by pointer; this build's `uLongf` is a **four
byte** type, not the eight-byte `long` this ABI otherwise uses — the target
stores the initial length with `sw` and compares it back with `lw`.  Declaring
it `unsigned long` cost `Decompress` five words of `sd`/`ld`.

**`new T[n]` for an element WITH a destructor carries a 16-byte cookie.**
`VILoader::Allocate` is the corpus's second real typed array-new and the first
whose element has a destructor: `__builtin_vec_new(n * 200 + 16)`, the count
stored at offset 0, the elements from +16, and a forward constructor loop
counting `n - 1` down through `-1`.  `Clear`'s `delete [] m_files` reads that
count at `m_files[-16]` and runs the destructor loop BACKWARDS from the end.
(Phase 27's `VIQueryTri` is the control: no destructor, so no cookie at all.)

## Semantics worth having

* **`Align` is the sector rounding.**  `r = position % AsynchAlignment()`, then
  `position -= r`, `offset += r`, `readSize += r`, and `readSize` rounded up to
  the same alignment.  The object therefore lands at `m_buffer + m_offset`
  inside a sector-aligned read, and `Complete` opens it with
  `VIObjFile::OpenObject(m_buffer + m_offset, m_size)`.
* **One read in flight, one buffer.**  `Process` starts a read only when the
  owning file's `IsAsynchComplete()` is true, so two transactions on the same
  file serialise; and every transaction reads into the SAME buffer, so
  `CompleteAll` is the only safe way to queue more than one.
* **The map is the dedup.**  `Load(file, id, …)` looks the DictID up in
  `m_map` first and, on a hit, just bumps the existing transaction's refcount
  and returns its index.  `Release` decrements, and on zero erases the map
  entry, tells the scene to release the resource (only if the transaction ever
  completed) and erases the transaction.
* **`Find` is a binary search** over `VIResourceDir`, keyed on the DictID at
  `+0x0C` of a 16-byte record — which is what `IsResourceAvailable` is, and what
  the one-argument `Load` uses to turn a DictID into an offset and a size.
* **A `.CSF`'s magic is `'CESF'`** = `0x46534543`, the header is 40 bytes and
  `m_offset` is always 40.  Segments are 256 KB (`size >> 18`, plus one if
  `size & 0x3FFFF`).

## Shipped

* **`VILoader::Load`'s capacity check is inverted.**
  `if (size > m_bufSize + m_files[i].m_file.AsynchAlignment() * 2) return -1;` —
  it should be `size + 2*align > m_bufSize`.  As written it accepts an object up
  to two alignments LARGER than the buffer, and `Align` then grows the read
  further, so a large enough resource overruns `m_buffer`.
* **`VILoader::Complete` throws away `VIObjFile::FileType(0xAB4F)`'s answer** —
  it sets or checks the file type and the result is never tested.
* **`VILoader::SetProgressCallback` returns void** among a class of `int`s, and
  so does `VICSFFile`'s.
* **`VICSFFile::Compress` closes its input file twice** — once after reading it
  whole and again after the compression loop.
* **`VICSFFile::Decompress` leaks on every failure path**: the segment array,
  every block already inflated, and the compressed bounce buffer are freed only
  on the success path.
* **`VIActor` never initialises `m_scale`** (+0x5C) or the word at +0x6C, and
  `CalcObjectToWorld` reads `m_scale` on the first call.

## Harness

Two new `SPAN_CAP` entries.  Both TUs close with the same run of unnamed
`VIMap<int,int>` walkers, so the last named symbol in each acquires a span
reaching to the next instantiation: `Align__8VILoaderR13VILoaderTrans` 2704 ->
**192** and `Decompress__9VICSFFileR6VIFileRiPPQ26VIFile7MemSegm` 3136 ->
**620**.  Both land one word past a `jr ra` and leave another behind, which is
what `test_span_caps_only_ever_shrink` checks; the canary passes at 27/27.

**Grep for an existing STUB before adding a class to `vi.h`.**  `viarray.cpp`
had modelled `VIResourceElem` as `{ int m_a, m_b, m_c, m_d; }` — the 16 POD
bytes phase 24 needed for the instantiation — and the real declaration in
`vi.h` collided with it.  The whole TU then failed to build, which took 126
`VIArray` symbols out of the score and read as a regression list spanning nine
element types.  Phase 36's rule (`extern`s) with a class instead of a function.

**Do not run `_q.py` while `eqcheck.py` is running.**  They share `decomp/build`
and `_q.py` deletes the object it is about to rebuild, so a full run made in
parallel reported 1646/1841 with 200 phantom regressions across classes nothing
in this phase touches.  The clean re-run is the only one to believe.

## Open

`VILoader::Find` 17/49 and `VILoader::Align` 17/48 are **instruction for
instruction identical** to the target and differ only in which registers hold
which value (and, in `Find`, one `nop` the target needs because its copy of
`d.m_data` is scheduled a slot earlier).  Twelve statement orderings, five
element-access spellings, a pointer-to-dir, a guarded do/while and an explicit
element pointer were all swept; every one scores the same.  This is the standing
register-allocation phenomenon.

`VICSFFile::Decompress` 6/155: the target keeps `VIEndian`'s local copy in `s0`
across the six header swaps and spills it to the stack at the top of the
segment loop, where ours never gets a hard register at all.  `VICSFFile::Compress`
26/286: the target reads `destLen` from its stack slot TWICE — once with `lw`
for the record and the maximum, once with `lwu` for the 64-bit total — where
gcc gives us one `lw` and a `dsll32`/`dsrl32` pair; four statement orders and
three spellings of the maximum were swept and none splits the two loads.
