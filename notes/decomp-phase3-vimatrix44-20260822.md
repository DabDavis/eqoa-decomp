# Phase 3: 25 of 31 `VIMatrix44` methods rebuild byte-exact (2026-08-22)

**Status: the target set did NOT plateau.** Phase 2 ended with two functions matching and a
stated amber threshold — *"amber if the set plateaus below ~80%"*. It is at **28 of 31
pure-scalar methods (90%), 1,434 of 1,707 instructions (84%)**, plus the destructor, which is
not in the counted set. Nineteen of them matched **on the first attempt**, including
`RotateHPR` (77 instructions), `Transform` (104) and the 182-instruction
`ConvertTo(VIQuat&, float&, VIVect3&)` — so the per-function cost is not linear in size, it is
linear in how many *style facts* are still unknown, and those are now written down.

**The class layout is settled and the whole decomp is written against it:**
`VIMatrix44::m` is **`VIVect4 m[4]`**, not `float m[4][4]`, in a class marked
`__attribute__((aligned(16)))` — see below. All 28 matches survived the migration unchanged.

The three misses are two problems, both of them a single register-allocation choice over
otherwise identical code.

Harness, sources and the reproduction: [`decomp/README.md`](../decomp/README.md). This note
is the findings.

## The assembler is SN Systems' `ps2eeas` — and its VERSION matters too

The single biggest discovery of the phase, and it was found by **one nop**, then refined by
**two more**.

`ConvertToEulerXYZ` came out 56 instructions against the target's 57, identical except for a
missing `nop` between `mtc1 zero,$f21` and the `c.lt.s` that reads `$f21`. That is an R5900
hazard: after `mtc1` writes an FPR, a COP1 instruction reading it needs one slot. **GCC does
not emit it — it emits `#nop`, a COMMENT — and leaves the insertion to the assembler.** Every
GNU `as` in `decomp/toolchains/` drops it; every SN `ps2eeas` inserts it.

Then `ConvertTo(VIQuat&)` came out **two instructions long**: `ps2eeas 1.9.25.758` (shipped in
`ee-gcc2.9-991111` and `2.95.3-136`) pads a `div` that follows a branch delay slot with two
nops — the R5900 DIV errata, and `-Wdivbug-on` reports it as *"DIV related opcode too near
branch instruction - Added 2 padding NOP/s"*. **The target has no padding there**, so the
build predates that fix. Four SN assemblers ship across the toolchain set; the one that agrees
with every function at once is

> **`toolchains/ee-gcc2.95.3-114/lib/gcc-lib/ee/2.95.3/ps2eeas.exe`**

(2.95.2-273a's 1.8.19.316 does not pad either, but it breaks `Transpose` and `Debug`, so it is
not the one.) The build is therefore

> **`ee-gcc 2.95.x -O2 -G0`, compiled with `-S`, assembled with that `ps2eeas`.**

The compiler and the assembler come from **different packages**. This refines Phase 2's trap
("use the period assembler, not modern GAS") twice over: the period toolchains disagree with
each other, and the right assembler ships beside the wrong compiler.

`-Wa,-O0`, `-Wa,-mips1/2/3`, `-m5900`, `-mmips-as`, `-mfix4300` and `-fno-delayed-branch` were
all swept first, and none of them produces either nop. It is not a flag. (`-fno-delayed-branch`
in particular breaks all 27 matches, so the delay slots are gcc's, not the assembler's.)

## Relocation-aware diffing, and a bug it exposed in itself

`ConvertTo(VIQuat&)` copies `int nxt[3] = {1,2,0}` onto the stack from `.rodata`, so it carries
a `%lo` addend the linked image resolves (`addiu a2,v0,2248`) and an unlinked object cannot
(`addiu a2,v0,0` + `R_MIPS_LO16`). The differ now masks the linker-written field **at exactly
the offsets the candidate object's own relocation table names**, counting them — which is
principled where the old `hi16lo16` pass is a heuristic that can erase a float constant, and
which the 8-instruction pairing window would have missed here anyway (the `lui` is 17
instructions earlier).

The first version of the masking was wrong in a way worth remembering: **`readelf -r` offsets
are relative to the section each relocation applies to**, so `.rel.data` entries landed inside
`.text` functions' byte ranges and masked **14 real instruction words** in functions that have
no text relocations at all. Only the printed per-function counts made it visible. It now takes
a section argument, and a canary locks all three properties (section scoping, field-exact
masking, and a difference outside a relocation site still failing).

## Style facts recovered — each one unlocked several functions

**1. Row pointers are declared as variables.** `Add(const VIMatrix44&, float)` computes six
base pointers up front; no spelling of `m[i][j]`, hoisted or not, reproduces that, because
gcc folds a constant index into the load's immediate. What does reproduce it is what the
author actually wrote:

```cpp
float *d0 = m[0], *d1 = m[1], *d2 = m[2], *d3 = m[3];
const float *s0 = a.m[0], *s1 = a.m[1], *s2 = a.m[2], *s3 = a.m[3];
d0[0] = s0[0] + f; ...
```

**2. Loops are hoisted through a local `float (*d)[4] = m;`.** Phase 2 found this on
`Transpose(const&)`; `Transpose()` needs it too, and without it exactly one `addu` comes out
with its operands reversed. Two for two: assume the hoist.

**3. Inner loops are unrolled in the SOURCE, not by the compiler.** `-funroll-loops` is off
(it changes nothing anywhere). `Store` keeps its outer loop with four explicit stores inside;
`StoreTranspose` is sixteen explicit statements with no loop at all. Read the loop structure
off the object and write it literally.

**4. Two functions are Graphics Gems, and their temp order is load-bearing.**
`Rotate(const VIQuat&)` sat at 6 wrong instructions — the same instruction multiset, one
scheduling swap — through ~20 orderings of its stores and temps, including 150 random
permutations of the nine products (2 of which matched by luck). It matches exactly with
Shoemake's classic order, `wx, wy, wz, xx, xy, xz, yy, yz, zz`, over `s = 2/norm²`. Likewise
`ConvertToEulerXYZ`/`ZXY` are `Eul_FromHMatrix`, `RotateHPR` is `Eul_ToHMatrix` (i=1, j=0,
k=2), and `ConvertTo(VIQuat&)` is the matrix→quaternion gem with its `int nxt[3] = {1,2,0}`
table — copied onto the stack from `.rodata` on every call, which is what the `ldl/ldr/lw`
prologue in that function is. **When a function looks like a textbook routine, fetch the
textbook: it fixes the temp names, their order and the association, which is otherwise a
search over 9! orderings.**

**5. Quaternion doubling temps are declared `w2` first** in the two `Transform(const
VIQuat&, ...)` overloads (`w2, x2, y2, z2`), and x-first in `UnitRotate`. Nothing but
register allocation distinguishes them, and it is worth two minutes of sweeping rather than
an hour of theory.

**6. `Debug()` is an empty loop.** `for (int i = 0; i < 4; i++) ;` and `return 0` — whatever
it printed was compiled out, and the five consecutive `nop`s left inside the loop are real.
Byte-exact.

**7. An address that reaches a MEM directly comes out INDEX-first; one materialised into a
variable comes out BASE-first.** This is the rule behind the whole `ConvertTo`→quat family,
and it is measurable in isolation: `return p[i];` emits `addu $5,$5,$4`, while
`const float *q = p + i; return *q;` emits `addu $4,$4,$5`. It is also visible in an
already-matched function — `Transpose(const&)` has one of each, and the Phase 2 "hoist"
discovery is the same effect.

`ConvertTo(VIQuat&)`'s target wants the MIRROR of what the natural spelling gives on both
counts: the row pointer index-first (which a natural `m[i]` will not produce) and the element
address base-first (which a natural `mi[j]` will not produce). Both can be *forced* —
`(const float *)(i * 16 + (unsigned)m)` for the row, because integer arithmetic keeps source
order where pointer arithmetic is canonicalised; and `const float &x = mi[j];` for the
element, because binding a reference materialises the address. All three overloads then match
(the middle one first try). **The spelling is almost certainly not the original**: it is a
lever on gcc's canonicalisation, not something anyone writes. The likeliest reading is that
the real build's compiler canonicalises addresses the other way — a 2.95 patchlevel we do not
have — and the honest state is "reproduced, spelling unexplained".

## The layout: `VIVect4 m[4]`, not `float m[4][4]`

`SwapRows` settled it, and settled it *naturally*. The function had resisted every spelling
for a day: the target round-trips its 4-float temp through the stack (store, then reload
after the row stores) where this build forwards the values in registers, and no amount of
`volatile`, address-taking, casting or flag-flipping reproduced that. The answer was the
class:

```cpp
class VIVect4
{
public:
    float x, y, z, w;

    VIVect4() { x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; }
    void Set(const VIVect4 &v) { x = v.x; y = v.y; z = v.z; w = v.w; }
};

class VIMatrix44
{
public:
    VIVect4 m[4];
    ...
} __attribute__((aligned(16)));

void VIMatrix44::SwapRows(int i, int j)
{
    VIVect4 t;

    t.Set(m[i]);
    m[i].Set(m[j]);
    m[j].Set(t);
}
```

Byte-exact, first try, with nothing forced. Three facts fall out, each independently checked:

- **The copy goes through a function taking a `const VIVect4 &`.** That is what stops gcc
  forwarding the loaded values — the reference defeats the alias analysis that a direct
  `t.x = m[i].x` does not. An implicit (bitwise) copy instead becomes `ldl/ldr`, and with
  `aligned(16)` it becomes `lq/sq`; neither is what `SwapRows` holds.
- **It is `Set`, not `operator=`.** Both produce identical code *in `SwapRows`* — but an
  `operator=` on `VIVect4` would make `VIMatrix44`'s implicit assignment element-wise, and
  the copy constructor's copy is a 64-byte **block move**. Only the non-operator spelling
  satisfies both functions.
- **`VIVect4` has a zeroing default constructor**, which is where `SwapRows`' three surviving
  `sw zero`s come from (the fourth is dead-store-eliminated), and where the copy
  constructor's whole zero loop comes from — it is gcc's array-construction loop over
  `m[4]`, counter down to −1 with `bne`, which no hand-written loop over `float m[4][4]`
  reproduces.

**The migration was free.** All 28 functions matched before the change were rewritten
mechanically (`m[i][j]` → `m[i].x/y/z/w`, row pointers as `&m[i].x`, the two loops that index
with variables through an explicit `(float (*)[4])m` cast) and every one of them still
matches. The layout is now a fact of the decomp rather than a hypothesis from one function.

## A shipped bug, found by matching

`ConvertTo(VIVect3&, VIVect3&, VIVect3&)` and `ConvertTo(VIVect3&, float&, VIVect3&)` compile
to **the same 17 instructions**, and neither one touches its second or third out-parameter:

```cpp
VIQuat q;
ConvertTo(q);
q.ConvertTo(a);      // b and c are never written
```

Both overloads save only `a1` across the first call; `a2`/`a3` are dead. A caller asking a
matrix for its euler angles *and* scale gets whatever was in those references. This is not a
transcription artifact — it is what the shipped module does, and it is the kind of thing a
matching decomp finds that a hand-port cannot.

## What is still open

| Function | Instr | State |
|---|---:|---|
| `ConvertTo(VIQuat&, VIVect3&, VIVect3&)` | 231 | **71 of 245 words wrong, and the length is exactly right** — the per-row scale loop and both branch bodies match instruction for instruction. One register-allocation choice in the prologue: the target materialises `&inv.x` into a temporary and copies it (`addiu v0,sp,16` + `move s7,v0`) where this build writes `addiu s7,sp,16`, and the extra live pseudo renames registers through the else branch. Fifteen spellings of the local and its pointers all coalesce the same way. |
| `VIMatrix44(const VIMatrix44&)` | 20 | **19 of 21 words wrong, everything accounted for.** The zero loop is exact (the `VIVect4 m[4]` array-ctor loop) and the copy is exact (a 64-byte block move, which needs `aligned(16)` or it becomes `ldl/ldr`). What differs is the same choice again: the target keeps `this` in v0 across the loop and walks v1; this build walks v0 and rematerialises `this` from a0 at the end. **A self-assignment guard (`if (this != &a)`) reproduces the target's allocation exactly** — at the cost of the two instructions the guard costs — so the mechanism is "something keeps `this` live across the loop". Fifteen spellings that should have done it (a local pointer, a local reference, an inline `Copy`/`CopyTo`/`Set`, `memcpy`, a `mode(TI)` quadword loop, an explicit walker loop, a POD row type, a destructor, `-fno-*`) all coalesce the same way. `-fno-schedule-insns` gets it to 10 of 21 but breaks 22 other functions, so it is not the build's flag. |
| `operator=(const VIMatrix44D&)` | 22 | Not attempted: needs the `VIMatrix44D` (double) class. It calls a 2-argument function four times with `(this+16k, src+32k)`, so the row type has an assignment from a double row. |

The two real misses are **the same phenomenon**: one pseudo that the target's compiler keeps
live and this one coalesces away. It is also the same family as the address-canonicalisation
mirror in the `ConvertTo`→quat sources. Three independent functions now point at one suspect:
a 2.95 patchlevel that is not in `toolchains/`. That is the Phase 4 question, and it is now a
*compiler* question rather than a source question — which is a much better place to be than
where Phase 3 started.

## Harness changes

- **`eqcheck.py`** — the regression driver: builds `decomp/src/*.cpp` and diffs every global
  function in the objects against the image. `MATCHED.txt` is the baseline, and a symbol that
  was byte-exact and is not now exits non-zero, so a source change made for function N cannot
  quietly break N−1. A symbol in the source but absent from the image reports as `NO-TGT`
  rather than being skipped.
- **`eqasm.py`** — target and candidate side by side with a `<>` marker per differing word.
  `eqd.py diff` shows only the first difference in context, which hides "same instructions,
  different order".
- **`eqsweep.py`** now compiles with `-S` and assembles with the settled `ps2eeas`; a sweep
  through GNU `as` (or the later `ps2eeas`) can score the *right* source as wrong.
- **Relocation masking** in `eqdiff.py` — see above; `eqd.py diff --no-reloc-mask` turns it
  off, and every masked word is counted and printed beside the MATCH.
- **`eqdecomp.disasm` passes objdump `-z`.** Without it objdump abbreviates a run of
  identical words to `...`, so `Debug` decoded as 6 instructions in 44 bytes and every
  index-wise compare downstream was misaligned. Locked by a canary with a positive control.

## Method notes worth keeping

- **A pure scheduling difference is a source-ORDER problem, and the store order is usually
  not the lever.** For `Rotate`, 120 random permutations of the 16 stores were all worse than
  row-major; the answer was the declaration order of the temps.
- **Random permutation search is a cheap oracle but a bad answer.** Two random orders matched
  `Rotate` before the Graphics Gems order was tried; either would have "worked" and neither
  would have been the source. Search until it matches, then ask *why that order*.
- **Score by index-wise word equality and you get noise** — one inserted nop turned a
  one-instruction miss into `36/57 wrong`. Read the side-by-side before believing a count.
  The useful reading is *does the length match*: `ConvertTo(VIQuat&, VIVect3&, VIVect3&)` is
  245 instructions against 245 with 71 wrong, which says "right code, wrong registers", where
  a length difference would say "wrong code".
- **A masked difference must be counted per function and printed.** The relocation masking
  was silently eating 14 real instruction words in unrelated functions, and only the printed
  count made it visible; a differ that reports MATCH without saying what it forgave is the
  "manufactured clean result" this harness exists to avoid.
- **Two assemblers that agree on 26 functions can disagree on the 27th.** Cross-assemble one
  `.s` through every assembler on disk before concluding anything about the source — the
  whole `div`-padding finding is two nops in one function out of twenty-seven.
- **When one function will not yield, suspect the TYPE, not the statement.** `SwapRows` had
  absorbed ~30 spellings of its body and stayed at 32 of 34; it fell in one attempt once the
  class was `VIVect4 m[4]` with a zeroing ctor and a reference-taking copy member. The
  evidence for that class had been sitting in a *different* function's prologue (the copy
  constructor's zero loop) for a day.
- **A construct that fixes the code at the cost of extra instructions is still a result.**
  The self-assignment guard cannot be the original source — it emits a branch the target does
  not have — but it reproduces the register allocation exactly, which converts "unknown
  divergence" into "one pseudo's live range", and that is what makes the remaining gap a
  compiler question rather than an infinite source search.
