# Decomp phase 30 — the FONT LAYER, and the enum phase 16 read off the wrong thing

**2026-08-26.  1390/1527 → 1427/1567.**  `VIRFont` 24 of 27 and `VIFont` 13 of
13, on the settled toolchain (`ee-gcc 2.95.3-107 -O2 -G0 -fno-exceptions`,
assembled with SN's `ps2eeas` out of the `2.95.3-114` package) with **no
re-tuning** — 30 of the 40 byte-exact on the first build, including both
257-instruction `Cache` twins once one line was right.

The long form of the layout, the levers and the three open functions is
`decomp/README.md`, "Phase 30 result".  This note records what phase 30 proved
that reaches outside the decomp.

## The correction: `VIPixelStore` has been wrong since phase 16

Phase 16 recorded the enum as *"0 I4, 1 I8, 2 RGBA32, 3 RGB24, 5 RGBA16 — bound
`sltiu 6`, and 4 has no case at all, so its table entry is the default's
`return -1`"*, and said it was read **off `CalcMipLevelStride`'s jump table**.
It was not.  It was read off the order of that function's case BODIES, which is
source order and says nothing about the values.

The table itself is in the relocated dump at **0x004EA560**, six words:

```
case 0 -> 0x0040e600   (w + 1) / 2      I4
case 1 -> 0x0040e614   w                I8
case 2 -> 0x0040e61c   w * 4
case 3 -> 0x0040e61c   w * 4            <- not the default
case 4 -> 0x0040e624   w * 3            RGB24, and it is 4
case 5 -> 0x0040e630   w * 2            RGBA16
```

`VIRFont::CalcDataSize` and `VIRFont::InitChars` carry the same switch with a
**seventh** entry (`sltiu 7`, table at 0x004EA680 / 0x004EA660), value 6, also
four bytes.  Nothing in the corpus names 3 or 6.

**Why it survived byte-exact for fourteen phases:** the jump table is DATA and
the differ compares instruction words.  A wrong label set with the right number
of case blocks, the right block order and the right bound produces exactly the
same code.  `src/vi.h` now carries the corrected values with a note; nothing
regressed.

The reading method is phase 27's — *a switch's data table is readable out of the
RELOCATED dump* — and this is the second time it has corrected something read
from a listing instead.

## The lever: `out[n]` is not `*p++`

`VIRFont::Cache(VIRaster &, const char *, int, int *)` and its UCS-2 twin are
257 instructions each, and both sat at 32 wrong through four spellings of the
output cursor.  A hand-written walking pointer — `int *p = out; … *p = c; p++;`
— **coalesces with the parameter**, so the target's `move s8,s0` in the loop
preheader has nothing to come from.  Written as a subscript, `out[n] = c`, gcc's
`loop` pass strength-reduces it and builds the induction variable out of a COPY
of the parameter, which IS that `move`.  One line; 32 wrong → 0 in both twins.

General form: **where the loop already carries the count, do not hand-write the
walking pointer.**  This is the exact inverse of phase 10's `VIString` rule (*a
copy loop WALKS THE PARAMETER, base in a local*), and the discriminator between
them is whether a loop-carried count already exists.

## The lever: slot order and store order are separately controllable

Phase 13: *stack slots go in DECLARATION order while the initialising stores go
in STATEMENT order.*  `Cache` needs both halves at once — `n` at `sp+16` with
its zero store SECOND, `locked` at `sp+20` with its store FIRST.  Declared
together (`int n = 0; int locked = 0;`) the two move as one and the prologue is
two words wrong.  Split the second from its initialiser —

```c
int n;
int locked = 0;
…
n = 0;
```

— and the function is byte-exact.  The declaration fixes the slot; the statement
fixes the store.

## The lever: the I4 rounding is a BRANCH

`n = w >> 1; if (w & 1) n++;` if-converts to `andi`/`srl`/`addu`, which is one
word shorter and is not what the target does.  Both `CalcDataSize` and
`InitChars` test the low bit and BRANCH, so the source names the shift in both
arms: `if (w & 1) n = (w >> 1) + 1; else n = w >> 1;` — the same spelling
`VISurface::CalcMipLevelStride` has carried since phase 16, which is what made
it findable.

## The lever: a binary search computes its exit test twice

`FindChar` ends `slt v1,hi,lo; li v0,-1; movz v0,mid,v1`, and the loop's own
back-edge test is a SECOND `slt` of the same two registers sitting in that
branch's delay slot.  Folded into one condition — `while (lo <= hi &&
Char(mid)->m_code != c)` with `mid` computed ahead of the loop — gcc CSEs the
two and 11 of 36 words are wrong.  Written with `mid` inside the loop and an
explicit `break`, the return block owns its own copy, reorg duplicates it into
the delay slot, and it is byte-exact.  **A `break` and a folded loop condition
are the same program and not the same object code.**

## `VIFont`: a mem-initialiser, and a static per entry point

The constructor is four `swc1`s of 1.0f and nothing else, so the colour is
initialised in the **mem-initialiser list**; written in the body,
`VIColor32F`'s own zeroing constructor lands in front of it.

Every lookup does `VIHashResourceID(sm_FontName)` on **every call** — cached
nowhere — and stores the result into a static before using it.  Each entry point
has a static of its OWN: 0xF40, 0xF44, 0xF48 … four bytes apart in the relocated
dump, in declaration order, with `FontLoaded` the one that stores nothing.  All
of those are `lui`/`sw 0(reg)` pairs, i.e. relocation sites the differ masks, so
one file-scope static in the decomp reproduces all seven.

`VIFont::sm_FontName` is **`"ArialUI"`** (0x004ED0B8) — so every `DrawString`
in the engine goes through the 7,573-glyph `ARIALUI.CSF`, which is the 1.42 MB
that `claude-notes/ee-memory-budget-20260814.md` names as the one real reclaim
and `EQOABOOT` clamps.

`SjisToJis` is the corpus's first function that is 64-bit end to end (`dsrl`,
`daddiu`, `dsll`, `daddu`) because `unsigned long` is eight bytes on this ABI
(phase 23).  Its closing constant is **0x2021, not 0x2121**: gcc 2.95 does not
distribute a constant add across a left shift, so the `+ 1` stays inside the
shift and the `0x20 << 8` folds into the tail.  35 instructions, first build.

## What the class actually does, and four shipped oddities

`VIRFont` is the decoded font — every glyph's pixels in one `new unsigned
char[]` block whose four sections are byte OFFSETS until `Init` rebases all four
onto the block in one pass — AND an LRU cache of the glyphs currently resident
in a 256x256 `VISurface`.  `Cache` finds each character (binary search, falling
back to `'?'`), promotes its slot to the tail of the `m_head`/`m_tail` chain,
blits the glyph in on a miss, precomputes the four corner UVs, and hands back
the character INDEX — not the slot.

* **`VIRFont::Lock(VIRaster &)` never reads its raster.**  Phase 16's discarded
  reference again; the parameter exists only to match `Unlock`, which does use
  it.
* **`Cache` reports the overflow after doing the damage.**  `if (m_numSlots < n)
  return -1;` is tested once the whole string has been cached, so a string
  longer than the atlas has already evicted its own earlier glyphs — and the
  indices already written into the caller's array are the stale ones.
* **`m_maxSlots` and `m_numSlots` come from different formulas.**
  `CreateResources` sizes the slot TABLE as `(256/cell) * (256/height)` while
  `InitSlots` counts what actually fits as `(256/cell) * (256/cell)`, with the
  cell rounded up to even for I4.  The allocation is therefore never short, but
  it over-allocates whenever the widest glyph is wider than the line is tall.
* **`InitSlots` takes its row stride from the LOCKED level's pixel store**, not
  from `m_pixelStore`; the two agree today only because `CreateResources` copies
  one into the other.

## The three open functions, and one thing that is not the source

`BeginChar` 7/14 — identical instructions, different registers (the target parks
the character's address in `t0`, ours in `v1`).  Five spellings swept.

`InitSlots` 27/132 — one word.  The target writes `m_pos.y` and `m_pos.x`
through an explicit `addiu v0,v0,16` where ours folds the 16 into the two
displacements.  **Eight spellings swept and gcc folds every one** — pointer,
reference, `char *` cast, integer expression, two statements, a slot local, an
inline `Set` on the member, an inline `SetPos` on the slot.  A probe TU
reproduces the target's shape only when the pointer has **two reaching
definitions**, which no plausible source here has.  Filed with the standing
phenomenon.

`CreateResources` 25/127 — **one word, and the rest is the shift it causes**.
The target fills the palette guard's delay slot with a hoisted `li a0,1` — the
`VIPixelStoreI8` comparand, materialised before a branch that does not use it —
which leaves `m_paletteCount = 0` as a block of its own after the arms; ours has
nothing to hoist, annuls instead (`beqzl`, the store in the slot), and
materialises the 1 inside the second test, coming out at 126 words against the
target's 127.  **Inverting the guard gets the length right and scores 13 instead
of 25, and is wrong**: it puts the zero block FIRST where the target puts it
last.  The lower score is measuring alignment, not fidelity — the same trap
phase 13 found in the wrong-count and phase 27 in "a worse score under the wrong
compiler is not evidence".

---

**Refined by phase 40 (2026-08-27): stores 3 and 4 are BOTH right.**
`VIEESurfCache::CalcCacheInfo`'s own jump table (0x004EA590, six entries over
the same enum) maps store **3 to GS PSMCT24** and REJECTS store 4 outright,
while `CalcMipLevelStride`'s table gives 3 a four-byte stride and 4 a
three-byte one.  Read together: **3 is 24-bit colour in a 32-bit slot** — four
bytes per pixel in EE memory, uploaded as PSMCT24 — and **4 is the PACKED
24-bit form**, which has a stride but no upload path at all.  So this note's
"RGB24 is 4" is correct for the packed form and phase 16's "3 is RGB24" was
correct for the padded one; only the single-name enum was wrong.
