# Phase 23 — the FILE LAYER: `VIFile` and `VIWadFile`

**750/848 byte-exact** (711/803 before). `VIFile` 29 of 36 and `VIWadFile` 9 of 9,
on the settled toolchain (`ee-gcc2.95.3-107 -O2 -G0 -fno-exceptions`, assembled
with SN `ps2eeas` from the `2.95.3-114` package) with **no re-tuning**. 38 of the
45 written are exact; 19 of the first 19 matched on the first build.

Sources: `decomp/src/vifile.cpp`, `decomp/src/viwadfile.cpp`.
Declarations: `decomp/src/vi.h`.

## What the class is

`VIFile` is the engine's one file abstraction and it is **four back ends behind
one interface, with no virtuals at all**. Every entry point re-spells the same
ladder by hand:

```c
if (m_fd >= 0)   return Buffered...(...);
if (m_memory)    return Memory...(...);
if (m_segms)     return MemorySegm...(...);
return -1;
```

* **CD/host file** — `m_fd >= 0`. A 32 KB read-ahead buffer, a dirty window
  inside it, and an asynchronous `sceCdRead` path with an IOP-streaming twin.
* **flat memory** — `m_memory`, a base pointer plus a capacity and a length.
* **segmented memory** — `m_segms`, a scatter/gather list with a **binary
  search** (`MemorySegmCalcSegm`) for the segment holding the current position.
* **nothing** — every call returns -1.

`VIWadFile` derives from it (single inheritance at offset 0, no vtable) and adds
a flat archive: a three-word header, `count` 16-byte directory records, and the
data at a fixed 256-byte offset.

## Layout (`sizeof(VIFile)` = 0xC0)

```
+0x00 int  m_fd            sceOpen handle; -1 when this is not a real file
+0x04 int  m_flags         bits 0/1/2 access (at least one required), 8 truncate,
                           0x10 seek-to-end, 0x40 = do NOT allocate the buffer
+0x08 int  m_status        0 ok, 1 at end of data -- what Eof() returns
+0x10 long m_position      the logical position
+0x18 long m_osPosition    where the OS handle actually is, so BufferFlush can
                           skip the lseek when the write is already contiguous
+0x20 long m_memSize       memory back end: capacity
+0x28 long m_memLength     memory back end: valid length (grows on write)
+0x30 char *m_memory       memory back end: base
+0x34 int  m_numSegms      +0x38 Segm *m_segms   +0x3C int m_segm
+0x40 long m_segmOffset
+0x48 sceCdlFILE m_cdFile  +0x4C is its SIZE, and that is what Seek_End measures
+0x6C sceCdRMode m_cdMode  trycount 20, spindlctrl 1, datapattern 0
+0x70 int  m_cdPending     +0x74..+0x80 the in-flight read's arguments, kept so
                           CDSync can reissue it after a disc error
+0x88 long m_bufBase       file offset the read-ahead buffer starts at
+0x90 long m_dirtyStart    +0x98 long m_dirtyEnd    the unwritten window
+0xA0 long m_bufPos        the cursor inside the buffer
+0xA8 int  m_bufEof        this buffer runs to the end of the file
+0xB0 long m_bufLength     valid bytes in the buffer
+0xB8 char *m_buffer       +0xBC char *m_bufferAligned
```

`VIWadFile` adds `int m_numEntries` at +0xC0 and `WadEntry *m_entries` at +0xC4;
`WadEntry` is `{unsigned hash; int offset; int size; int pad;}`, 16 bytes.

## The levers

### 1. `long` is EIGHT bytes here; `long long` is SIXTEEN

Phase 16 recorded this as a fact about one `sd`-zeroed slot. In this class it
decides **every** 64-bit member. Writing them `long long` does not merely
mis-size the class — `m_position = m_position + offset` fails to compile at all:

```
vifile.cpp:178: unsupported wide integer operation
```

because `long long` is TImode on this ABI. Every `ld`/`sd` member is a `long`.
That single word is what took `MemoryOpen`, `MemoryRead`, `Seek`, `Tell` and the
constructor from "does not build" to byte-exact in one edit.

### 2. NESTED guards with ONE `return 0` at the bottom

`BufferFlush` opens with two rejections. Written either as two `if (X) return 0;`
statements **or** as `if (X || Y) return 0;`, gcc emits the second test as a
`bnel` to the *continue* label plus a `b` back to the exit — two instructions
too many, and the function is 11 words wrong. Nested, with the function falling
out through a single `return 0`:

```c
if (m_bufLength)
{
    if (m_dirtyStart != m_dirtyEnd)
    {
        ...                    // every failure is `return -1`
    }
}
return 0;
```

both guards become a plain `beqz`/`beq` straight to the shared exit and the
function is byte-exact. This is the complement of Phase 13's "`if (x) { … }
return 0;` defeats the if-conversion": the same shape, one level deeper, decides
BRANCH DIRECTION as well as branch kind.

### 3. `VIMIN`/`VIMAX` with the expression WRITTEN OUT beats a named local

Three functions turn on this. A clamp spelled

```c
long avail = m_bufLength - m_bufPos;
got = (int) (n < avail ? n : avail);
```

comes out as `movz` with the FIRST value as the register default. The macro,
which re-evaluates its second operand and lets gcc CSE it,

```c
got = (int) VIMIN(n, m_bufLength - m_bufPos);
```

comes out as `movn` with the SECOND value as the default — which is what the
target has. `BufferedReadCopy` went 7 wrong to 0 on that one line, and the
inner clamp of `MemorySegmRead`/`MemorySegmWrite` went 14 wrong to 6 the same
way. Both spellings compute the same number; only the macro reproduces the
register assignment.

Corollary, read off `MemoryWrite` and `BufferedWrite`: which of the two
arguments is first also decides the `slt` operand order, so `VIMAX(end, x)` and
`VIMAX(x, end)` are DIFFERENT code and both appear in this file.

### 4. A walking pointer's local is declared at FUNCTION scope

Phase 10's copy-loop lever, in its sharpest form yet. `MemorySegmRead` and
`MemorySegmWrite` were 6 wrong — entirely in the PROLOGUE, in the order the
callee-saved registers are pushed — until

```c
char *d = (char *) dst;     // function scope, above every guard
```

moved out of the `if (got > 0)` block it is used in. Both went to 0. The
declaration's position changes nothing about the loop; it changes when the
register's live range starts, and that reorders the save block.

### 5. `if (cond) return -1; return 0;` — not the ternary

Phase 11's rule, three more times (`SharedInit`, `ReadAsynch`, `WriteAsynch`).
`return r < 0 ? -1 : 0;` gives `li v1,-1; slt v0,v1,r; movn` — four words and
the wrong compare. The branching form gives the target's `slti v1,r,0; li v0,-1;
movz v0,zero,v1`.

### 6. The DISC path is the `if`; the IOP path is the tail

All three CD functions (`CDSearchFile`, `CDSync`, `CDRead`) exist twice over:
once against `sceCdSearchFile`/`sceCdSync`/`sceCdRead` and once against the
engine's own `VICdStream*` when `VIFile::sIopStream` is set. Written
`if (sIopStream) { iop } disc...` gcc puts the IOP half inline; the target
branches `bnez` to it. The source is

```c
if (sIopStream == 0)
{
    ... the disc half, ending in its own return ...
}
... the IOP half ...
```

which cost 28 words in `CDSync` and 30 in `CDRead` before it was applied.

### 7. `m_cdPending = 1` is the LAST store in the source

`CDRead` opens by recording its five arguments. Phase 16's rule — the last store
of each distinct CONSTANT is hoisted to the front — reads the emitted order
`112, 116, 120, 124, 128` backwards: `1` is the only constant, so the store that
was hoisted is `m_cdPending`, and the source stamps the four argument words
first and the pending flag last.

### 8. The retry loop's CONDITION carries the calls; its BODY is the wait

`CDSearchFile` retries the whole search after a disc error. Written as

```c
while ((r = sceCdSearchFile(f, name)) == 0)
{
    if (sceCdGetError() == 0) return r;
    ... printf, wait for the tray ...
}
return r;
```

the loop rotates the wrong way and the function is 25 words wrong. The target's
layout — the wait block FIRST, entered by a `b` over it to the call — is what
gcc emits when both calls are in the condition and only the wait is the body:

```c
while ((r = sceCdSearchFile(f, name)) == 0 && sceCdGetError())
{
    ... printf, wait for the tray ...
}
return r;
```

25 wrong to 9, and the short-circuit `&&` produces both of the target's exits
(`bnez s0` for a hit, and the fall-out for "no error, genuinely absent") for
free.

## A new `SPAN_CAP` entry

`CDRead__6VIFileUiUiPvP10sceCdRMode`'s derived span is 336 bytes; the function
ends at 292. The ten instructions after it are a **static interrupt handler**
(`iSignalSema`, then `sync` + `ei`) that the symbol map does not name, so the
span swallows it whole and the last quarter of every diff was another function.
Capped at 292 — it lands one word past a `jr ra` and leaves another `jr ra`
behind, which is what `test_span_caps_only_ever_shrink` enforces.

## Shipped behaviour worth knowing

* **`VIFile::Close` tests the FILE DESCRIPTOR for bit 6, not `m_flags`.**
  `lw v1,0(s0); andi v1,v1,0x40` — `m_fd` is at +0x00 and `m_flags` at +0x04.
  Since the enclosing test already proved `m_fd >= 0` and descriptors are small,
  the asynchronous-completion drain it guards **never runs**. Matched as written.
* **`VIWadFile::Load` returns 0, not -1, when the caller's buffer is too small.**
  -1 is "no such entry" or an I/O failure; the return value is the number of
  bytes loaded, and 0 is a coherent answer for "it does not fit".
* **`VIWadFile::Free` returns void.** Nothing sets `v0`.
* **`VIWadFile` lookup is LINEAR.** `LookupEntry` walks all `m_numEntries` and
  compares a 32-bit hash — `h = h * 131 + *p` over an in-place lower-cased copy
  of the name in a 256-byte stack buffer. There is no ordering and no bucket
  table, so a hash collision silently returns the wrong file.
* **The archive version must be < 2** and the data starts at a fixed offset of
  256 bytes past the file's beginning.
* **`MemorySeek` rejects a seek to exactly EOF**: it requires
  `0 <= m_position < m_memLength`, so seeking to the length fails.
* **`BufferedWrite`'s post-flush salvage is degenerate.** The number of bytes it
  keeps is `VIMIN(0, m_bufLength)` — zero for any non-negative length — and the
  loop it guards copies `m_bufferAligned[i]` to `m_bufferAligned[0]` (the
  destination is reloaded from the member every iteration and never advances).
  In practice the loop never executes.
* **The read-ahead buffer is `new char[0x8010]` aligned up to 16**, and flag bit
  `0x40` turns it OFF rather than on.
* **Reads are issued in at most 16 sectors at a time** and the clamp is written
  `sectors < 17 ? sectors : 16` — the constants 17 and 16 are both in the source.
* **`Seek_End` measures against `m_cdFile.size`**, the `sceCdSearchFile` result,
  not against anything the buffer knows.
* `SeekOrigin` is `{Cur = 0, Set = 1, End = 2}`: every seek tests 1 first, then
  zero, then 2, and anything else is -1.

## Open (7 of 45)

| function | wrong/total | what |
|---|---:|---|
| `BufferedRead` | 47/233 | the largest in the class. The three read strategies, the 16-sector loop and every store order are right; what is left is the tail-block ORDER (which of `return -1` / `return got` / the eof block lands where), which moves every branch offset in the function, plus two register swaps in the `min`/rounding sequences |
| `BufferedWrite` | 23/126 | two `memcpy` call sites the target CROSS-JOINS into one (its fast path is `b` to a shared `jal memcpy` whose delay slot is a `nop` — the signature of a merge), plus the s2/s3 pair that follows from having one call site instead of two |
| `Open(const char *, int)` | 35/93 | the guard is three SEPARATE `andi 1/2/4` tests in the target where every spelling tried folds to `andi 7`, and the join block then loads `m_flags` and ANDs it with 8 for a value **nothing reads** |
| `CDRead` | 6/73 | which of `lq ra` / `li v0,1` fills the branch delay slot, twice |
| `CDSync` | 8/102 | the same delay-slot choice on the early `return r`, twice over |
| `CDSearchFile` | 9/63 | the target keeps the result in `s0` across the IOP branch's call where ours takes `v0`; five real words |

Nothing structural is left in the first five: every member offset, every
constant and every call is confirmed by at least one byte-exact neighbour.
