# Phase 29 — `VIObjFile`, the ESF/CSF container, and the destructor that
# decided composition over inheritance

**1330/1464 → 1390/1527 byte-exact.**  59 of `VIObjFile`'s 62 functions, plus
`VIFile::~VIFile`, on the settled toolchain (`ee-gcc2.95.3-107 -O2 -G0
-fno-exceptions`, SN `ps2eeas` from the 2.95.3-114 package) with **no
re-tuning**.  21 were byte-exact on the first build.

`VIObjFile` is the layer every `.ESF`/`.CSF` in the game is read through:
a 32-byte file header at offset 0, then a tree of objects, each introduced by a
12-byte tagged record and closed by patching that record's size and sub-object
count back in place.  Its semantics were never in doubt — `esfviewer`, the
Blender add-on and `go-eqoa-esf-tools` all parse the same format — so every
mismatch here was codegen.

---

## The finding: it HAS a `VIFile`, it does not derive from one

`VIWadFile : public VIFile` has been settled since Phase 23, so the obvious
reading of `VIObjFile` — same base, same +0xC0 first member — was inheritance.
The destructor says otherwise, and it says it in four instructions:

```
_$_9VIObjFile:
    ...
    jal   _$_6VIFile
    li    a1,2                 <- __in_chrg = 2, "you are not in charge"
    andi  v0,s1,0x1
    beqz  v0,...
    jal   __builtin_delete     <- and I do my own delete
```

Written as a base, gcc 2.95 **forwards** `__in_chrg` to the last base
destructor and emits no delete of its own:

```
    jal   _$_6VIFile
    move  a1,s1                <- forwarded; no andi, no __builtin_delete
```

Both shapes were measured on the compiler with a six-line test case, `struct D :
B` against `struct D { B m; }`, and only the member form reproduces the target.
The layout is identical either way — the sub-object sits at +0 — so **the only
evidence that separates composition from inheritance in this ABI is the
destructor's `__in_chrg` argument**, and a class with no destructor cannot be
told apart at all.

That also makes the source read `m_file.Seek(...)`, `src.m_file.Read(...)`
rather than `VIFile::Seek(...)`, which is invisible in the object code.

`VIFile::~VIFile()` itself was missing from the tree entirely (`{ Close(); }`,
17 instructions) and matched on the first build once declared — adding it
regressed nothing in `vifile.cpp` or `viwadfile.cpp`.

---

## The lever that did the most work: a guard CHAIN, not a guard LIST

Six of these functions open with three or four cheap tests that all
`return -1`.  Written the obvious way —

```cpp
if (!m_read)      return -1;
if (m_writeOpen)  return -1;
if (m_readOpen)   return -1;
if (m_hdr.m_dirOffset == -1) return -1;
int r = ReadBegin(m_hdr.m_dirOffset, type, version);
if (r < 0) return -1;
return 0;
```

— gcc emits a *trampoline* (`b epilogue; li v0,-1`) for the first guard, and
**if-converts the last test to `slt`+`movn`**, where the target branches.
Fourteen spellings of that last test were swept — `long r`, `goto err`, a
ternary, `>= 0` with and without braces, an explicit `else`, an unsigned
compare — and every one of them if-converted.

Written as **one `&&` with the success path inside**, all of it goes away:

```cpp
if (m_read && !m_writeOpen && !m_readOpen && m_hdr.m_dirOffset != -1)
{
    if (ReadBegin(m_hdr.m_dirOffset, type, version) >= 0)
        return 0;
}
return -1;
```

`ReadDirBegin` 8/26 → **byte-exact**; `ReadBegin(unsigned long, …)` 26/38 →
**byte-exact**.  The dual — one `||` with the guards negated and a single
`return -1` — is what both `Copy` overloads needed: 38/166 and 34/155 →
**byte-exact**, on nothing but that one line.  (`||` and `&&` are not
interchangeable here: `||` on `WriteBegin`'s three guards is 35 wrong words
where `&&` is 25, and `||` over all four of `ReadBegin`'s is 31 where three
plus a separate fourth is 23.)

This is the general form of Phase 23's "nested guards share ONE return at the
bottom" and Phase 12's "a branch to a shared epilogue carrying that epilogue's
own first instruction in its delay slot is ONE return path": **a chain of tests
that all return the same value is one statement in the source, not N.**

## The if-direction, twenty-four times

Phase 20's "the if-direction is per FUNCTION" gets a bulk demonstration here.
`Write(char)` and `Write(short)` are the same seven lines and the target lays
them out oppositely — `Write(char)` puts `-1` inline after the test,
`Write(short)` puts it at the bottom — and **one source shape gives both**:

```cpp
int VIObjFile::Write(char v)
{
    if (m_canWrite) { …; return 0; }
    return -1;
}
```

That flip took all eleven scalar `Write` overloads plus `Write(const unsigned
char *, int)` byte-exact in one edit.  The twelve `Read` overloads need the
*other* shape — the flag and the byte-count test joined by `&&`:

```cpp
if (m_canRead && m_dirs[m_level].m_remain >= 2) { …; return 0; }
return -1;
```

Written as two separate guards they are 5 and 8 words wrong; written with `||`
and an early return, 5 and 8 wrong; written as `&&`, exact.  `WriteString`,
`ReadEnd`, `WriteEnd` and `Close` each needed the success-in-the-`if` form
too — `Close` went 9 wrong to 0 on it, and the nested `do`/`while` and the
plain `while` are byte-identical inside it, so Phase 10's guarded-do/while does
not arise.

## Where a global is READ is a source fact

`VIEndian` is a file-scope `int` that every wire function tests, and the ones
that swap twice around a `VIFile::Write` read it into a local — the target
keeps it in a callee-saved register across the calls, which a re-read of the
global could not do.  **A global load cannot move across a call**, so its
position in the emitted code IS its position in the source, to the statement:

* `WriteEnd(unsigned int &)` — moving `int endian = VIEndian;` one line up, to
  before `size = m_dirs[m_level].m_obj.m_size + 12;`, went **32/109 →
  byte-exact**.  Reading it late costs a whole callee-saved register (frame 80
  vs the target's 96) and re-numbers every allocation in the function.
* `WriteBegin` — the same move, before `offset = m_dirs[m_level].m_offset;`,
  is worth one word.

## Two more, both old rules in new clothes

**The fresh local for the intermediate (Phase 14/21), now in `unsigned short`
arithmetic.**  `Seek(len + 1 - max, Seek_Cur)` reassociates to
`len - (max - 1)`, which makes `max - 1` live across the call and parks it in a
callee-saved register; the target recomputes.  `int skip = len + 1;
Seek(skip - max, …)` does not reassociate.  `ReadStringBody(char *, unsigned
short, unsigned short)` 28/55 → **byte-exact**.

**Phase 16's constant hoist, verified in the forward direction.**
`OpenObject` writes thirteen fields with five distinct constants.  Reading the
object code by the rule — *the last store of each distinct constant is hoisted
to the front, hoisted stores keep their source order, the rest keep theirs* —
produced a statement order that matched **byte for byte on the first build**,
including that `m_read = 1` is the last `1` and `m_dirty = 0` the last `0`.
The rule is now good enough to write source FROM, not just to explain a diff
with.

It does not apply to `sw zero` blocks with no other constant, where there is no
register to materialise: `ResetState`'s nine stores come out in source order
(and only `m_level = -1` LAST works — nine orderings swept), and `PostOpen`'s
three trailing zeroes needed `m_canWrite, m_dirty, m_canRead`.

---

## The layout

`sizeof(VIObjFile)` is **592**, read off `Append`'s stack frame, which holds a
whole one as a local (656 bytes = 592 + two out-parameters).

```
+0x000  VIFile m_file           192 bytes, a MEMBER (see above)
+0x0C0  int    m_mode           the FileMode Open() was given
+0x0C8  Header m_hdr            32 bytes, written verbatim at file offset 0
          +0x00 unsigned m_magic          'OBJF' = 0x4F424A46
          +0x04 int      m_numObjects
          +0x08 unsigned m_fileType
          +0x10 long     m_headerSize     always 32   (eight bytes; `sd`)
          +0x18 long     m_dirOffset      -1 when there is no directory
+0x0E8  int    m_level          -1 when no object is open; max 9
+0x0F0  Dir    m_dirs[10]       320 bytes, the open-object stack
          +0x00 Obj  m_obj      the 12 bytes that go on the wire:
                   unsigned short m_type, m_version;
                   unsigned int   m_size, m_numSubObjects;
          +0x10 long m_offset   where the record was written
          +0x18 int  m_remain   read side: bytes of the object left
+0x230  int m_open, m_write, m_read, m_writeOpen,
            m_readOpen, m_canWrite, m_canRead, m_dirty
```

`Dir` is 32 bytes because `m_offset` is a `long` (eight bytes, Phase 23) and
forces alignment 8 — the two four-byte holes at +12 and +28 are natural
padding, not members.  `Obj` is a type of its own because both `Copy` overloads
read one into a stack local.

The three `VIFile::Open` flag words are **33 / 50 / 42** for
`Mode_Read` / `Mode_Update` / `Mode_Create`.

## Shipped oddities

* **`Copy` leaks its bounce buffer.**  Both overloads `new char[4096]`, and the
  `return -1` taken when the source's 12-byte record cannot be read skips the
  `delete []` at the bottom.  Every other exit after the allocation frees it.
* **The four-argument `Copy` never writes its `size` out-parameter.**  The
  fifth argument arrives in `t0` and is not touched anywhere in the 156
  instructions; the three-argument overload does write it.  Matched as written.
* **`WriteString` swaps the length in place and then uses the swapped value.**
  `VIBufferSwap(2, &len)` is applied to the local, `Write(&len, 2)` ships it,
  and the *following* `Write(s, len)` reads the same byte-swapped `len` back as
  its count.  Harmless on this console, where `VIEndian == 1` and the swap
  never runs.
* **`ReadBegin(offset, type, version)` resets `m_level` to -1 before seeking**,
  so reading an object by file offset silently discards whatever object stack
  was open.  (The `-1` it stores is the one the guard above left in `v0` — the
  delay slot of `bnez v0, epilogue`.)
* `VIObjFile::Read(char &)` passes the caller's reference straight to
  `VIFile::Read` with no stack temp, but `Write(char)` copies its by-value
  parameter to the stack first — the two are not mirror images.

## Open

Three functions, all block placement or register allocation, all with the
instruction sequence otherwise identical to the target:

* **`WriteBegin(…, unsigned long &)` 25/145.**  `eqasm.py` shows one real
  difference in 145 instructions: the target **rematerialises the constant 1
  into a second register** (`li s1,1`) halfway through, between the two endian
  swap blocks, and spends it on `m_canWrite`/`m_dirty` while the original copy
  goes to `m_writeOpen`; ours keeps one pseudo for all three.  Everything after
  that one insertion is the same instructions with the callee-saved registers
  rotated by one.  The standing phenomenon.
* **`ReadBegin(…, unsigned long &, unsigned int &)` 23/131.**  Two
  instructions: the `VIFile::Eof()` guard folds `li v0,-1` into its branch
  delay slot in the target and inverts to a branch-likely with a trampoline in
  ours.  Eight spellings of that guard and four of the `m_canRead` assignment
  were swept; the `!= 0` form of the latter is worth one word and is what the
  source now has.
* **`PostOpen` 3/78.**  One branch: the target moves the `AbortOpen(); return
  -2;` block out of line to the bottom next to the `return -1` block, ours
  keeps it inline.  A 2×2 sweep of the two error directions settled everything
  else — success-in-the-`if` for the size check, error-in-the-`else` for the
  magic check, 31/23/25 for the other three combinations.
