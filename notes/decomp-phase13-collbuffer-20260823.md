# Decomp phase 13 — the geometry buffer, and two levers that were open questions

**2026-08-23. 451/516 -> 468/534 byte-exact.**

`VICollBuffer` is done: **18 of its 19 functions byte-exact**, on the same
`ee-gcc 2.95.3-107 -O2 -G0 -fno-exceptions` + SN `ps2eeas` 2.95.3-114 build with
no re-tuning, and the nineteenth (`CalcBVolumes`, 247 instructions) is two
allocator decisions away.  Two of the levers found here also reached BACKWARDS:
`VIPool<T>::Allocate` went from 9 wrong words to 4 **across all thirteen
instantiations**, and `eqcheck.py`'s wrong-count was corrected for every
function whose candidate drifts in length.

It is the third class whose SEMANTICS were settled before the decomp started —
`go-eqoa-vi/vicoll/collbuffer.go` is a console-checked port of the READER half —
so, as with `VICollRay` in phase 11, every mismatch here was codegen and never a
misreading.  What is new is the WRITER: the cursor pair per section, the packing
switch, and the single allocation the three sections are carved out of.

## The layout

`go-eqoa-vi` models the six fields a walker reads.  The whole struct is:

```
+0x00 m_id          +0x04 m_userData      VIBuffer (see below)
+0x08 m_type        +0x0C m_locked        +0x10 m_scale
+0x14 m_bbox                              VIBBox, 24 bytes
+0x2C m_numGroups   +0x30 m_maxGroups
+0x34 m_numPrims    +0x38 m_maxPrims
+0x3C m_numVerts    +0x40 m_maxVerts
+0x44 m_groups      +0x48 m_prims         +0x4C m_verts
      -- byte OFFSETS until Init adds m_data to all three
+0x50 m_vertexGroup +0x51 m_floraType     signed char
+0x54 m_dataSize    +0x58 m_data
```

`VICollGroup` is `{int m_numPrims; int m_numVerts; VIBBox;}` = 32, `VICollPrim`
is `{short m_matIdx; short m_numVerts; VIBBox;}` = 28, and `CalcDataSize` sizes
the vertex section at 12 / 6 / 8 bytes for the three packings.  The num/max pair
per section is the shape phase 12's `VIPool` also has.

## `VIBuffer` — the two words every buffer class starts with

`VICollBuffer`, `VIPrimBuffer` and `VIColorBuffer` all zero +0x00 and +0x04 in
their constructors and never touch either again — `Clear()` does not.  Three
facts pin it down:

* the store to **+0x04 comes FIRST**, in all three, which is a chained
  assignment (`m_id = m_userData = 0;`) — phase 7's rule;
* `VIColorBuffer`'s entire constructor is those two stores, the `Clear()` call
  and `m_data = 0`, so they are not incidental to any one class;
* in `VICollBuffer` they are emitted **among the `m_bbox` member-constructor
  stores**, i.e. before the body.  Written in the body they land after all
  twelve; written as an initialiser list they come out `+0` then `+4`.  Only a
  base-class constructor gives both the position and the order.

A first data member with the same constructor is codegen-identical, and the
object code cannot separate the two.  The name `VIBuffer` is ours.

## `VIBBox` has a zeroing default constructor, and it calls `VIVect3::Zero()`

Both buffer constructors zero `min` and `max` **twice**, and *both* passes
address through `&min` and `&max` with the +0 folded back onto the outer object.
That is two nested inlines: the members' own `VIVect3()` constructors, then a
`VIBBox()` body.  Writing that body as six component stores on `this` puts all
six on ONE base register (`sw zero,20(v0)` for `max.z`); calling
`min.Zero(); max.Zero();` gives each vector a base of its own, which is what the
target has.  `VIVect3::Zero()` already existed in the header from an earlier
phase and had never been called.

## Lever 1 — the block's LAST store is hoisted to the FRONT

`Clear()` is sixteen independent member stores in one basic block.  gcc's
post-reload scheduler moves the block's **final** store to the head of the run,
so *the first store emitted after the join names the source's last statement*.
Sweeping `m_dataSize = 0;` through all eight positions changed nothing — the
statement that moved was always the last one, whichever it was:

| source ends with | first store emitted |
|---|---|
| `m_dataSize = 0;` | `sw zero,84` |
| `m_verts = 0;`    | `sw zero,76` |
| `m_data = 0;`     | `sw zero,88`  ← the target |

The target's first store is `sw zero,88(s0)`, so `m_data = 0;` is the last line
of `Clear()`.  Nothing else about the function reveals that.

## Lever 2 — `if (x) { … } return 0;` defeats the if-conversion

`VICollBuffer::Allocate` written flat —

```c
if (m_dataSize == 0) return 0;
m_data = new char[m_dataSize];
if (m_data == 0) return -1;
return 0;
```

— compiles the closing test to `li v0,-1` + `movn v0,zero,v1`.  The target
branches.  **Five spellings of that test are byte-identical** (member-then-test,
local-then-store, success-in-the-`if`, assign-in-the-condition, `!p`): the test
is not where the difference lives.  Nesting the body under the first test, with
ONE `return 0` at the bottom, is:

```c
if (m_dataSize)
{
    m_data = new char[m_dataSize];
    if (m_data == 0)
        return -1;
}
return 0;
```

and that is byte-exact.  Phase 12 had recorded exactly this shape as an
unexplained miss in `VIPool<T>::Allocate` ("the identical idiom if-converts in
the TARGET too" in `Init`).  Applying the same nesting there — the whole body
under `if (m_capacity)`, one `return 0` at the bottom — takes it from **9 wrong
words to 4, in all thirteen instantiations**.  The remaining 4 are the copy
loop's induction-variable numbering (`addiu a1,a1,12` and `addiu v1,v1,12`
swapped, and one `move` position), which six loop spellings do not move: that is
the `loop` pass, the standing phenomenon phase 4 named.

## Lever 3 — a shared tail written into EACH case cross-jumps

`Vertex(VIVect3S16 &)` switches on the packing and then bumps two counters.
Written once after the switch, the `li 28` those counters need stays in the
merged tail and **every register in the function moves by one** (`this` in a2
instead of a3).  Written into each case and left to cross-jump, the constant is
hoisted into both predecessors — two `li a2,28`, one per case block — which is
what the target has, and the function is byte-exact.

The single-case `Vertex(VIVect3 &)` overload directly above it wants the
opposite: one shared tail.  The rule is about how many predecessors the tail
has, not about the source reading tidily.

## Two more small ones

* **`VIVect3S16` needs a user `operator=`.**  The implicit one is an unaligned
  `lwl`/`lwr` + `lh` block move; the target copies three halfwords.  Same lever
  as `VIVect3` in phase 4, and the same class of finding: the type, not the
  statement.
* **`Begin` and `End` list packings 0, 1 and 3 as three duplicated case bodies.**
  A fall-through group (`case 0: case 1: case 3:`) lets gcc merge 0 and 1 into a
  range test (`bltz` + `slti 2`); the target keeps three distinct labels that
  cross-jump into one block.

## `CalcBVolumes` — 27 words, two causes

The structure is confirmed: two `VIBBox` accumulators on the stack (group at
sp+0, prim at sp+32), two independent "first" flags — one spanning the group,
one reset per prim, which is why gcc keeps them in *two* registers and
re-establishes `a1` after every call — a three-way packing switch inside the
vertex loop, and `m_bbox = gb` on the first group with `m_bbox.Union(m_bbox, gb)`
after.  Facts that fell out:

* the `int` first parameter is **UNUSED** — `a1` is written before it is ever
  read;
* `p` is VICollide's vertex-group offset table (the one at `VICollide+0x90` that
  `go-eqoa-vi` found from the console), and `p[g]` is reached with the R5900
  three-operand `madd` off `mtlo`;
* the stack slots go in **declaration** order and the initialising stores in
  **statement** order, and those two orders differ here: `gi, pi, vi, first`
  declared, `first, pi, vi` assigned;
* the packed cases read their vertex as an **array subscript**
  (`((VIVect3S16 *)m_verts)[vi]`), which puts the index first in the `addu`,
  where the V case materialises the address into a variable and comes out
  base-first — phase 3's rule, both halves visible in one function;
* `v += p[g]` (the inlined `operator+=`) rather than three component adds: the
  three-statement form leaves f1/f2/f3 free and the scheduler hoists all three
  loads, where the target reuses f0 for each.  64 -> 27 aligned words.

What is left is the pair `s4`/`s5` swapping roles (the `fp` flag vs the ×12
induction variable) and, downstream of that, one cross-jump gcc took in the
target and not here.  Fifteen source shapes moved neither.  That is the
allocator, i.e. the phenomenon phase 5 and phase 7 already named.

## Harness — the wrong-count was lying

`eqcheck.py` counted differing instructions **positionally**.  One extra
instruction in the candidate shifts every later word, so a function that is
structurally right reads as almost entirely wrong — and, worse, *every* source
variant scores the same, so a sweep cannot discriminate.  Four `CalcBVolumes`
shapes all scored `180/258` while the diff-aligned count ran 64 -> 54 -> 27.

`score()` now counts by diff alignment whenever the lengths differ, and the row
is marked `~`.  Nothing about which functions MATCH changes; what changes is
that several open functions were being overstated by a factor of two:

```
  wrong 171/201   Skitter__9VICollRay…      ->  wrong~ 91/201
  wrong 153/160   Invert__10VIMatrix33…     ->  wrong~108/160
  wrong  66/66    Scale__6VIBBoxf           ->  wrong~ 51/66
  wrong  47/47    Scale__6VIRectRC8VIVect2I ->  wrong~ 28/47
```

`Scale__6VIRecti` and `Scale__6VIRectRC8VIVect2I` had read as "100% wrong",
which is exactly the signal that sends you looking for a layout error.

## Groundwork for the next phase — `VIPrimBuffer`

Measured while establishing that it is a phase of its own, not an afterthought
to this one:

* same skeleton — `VIBuffer` base, `VIBBox` at +0x14, three num/max cursor pairs
  at +0x40/+0x48/+0x50, three section pointers at +0x58/+0x5C/+0x60 rebased by
  `m_data` at +0x68 — but **four floats at +0x30..+0x3C**, three of them written
  by a `SetPacking(int,int,int)` that calls `powf` three times, and the fourth
  set by `Unlock` from a five-way switch (1/128, 1/127, 1.0);
* **`Init(VIRaster &, VIPrimBufferType, int, int, int)` DISCARDS its raster
  argument** — `a1` is overwritten before it is read, exactly like
  `CalcBVolumes`'s `int`;
* the type enum has **9 values and is dispatched through a JUMP TABLE**
  (`sltiu v0,v1,9`), in `Begin`, `Vertex`, `End` and `GetNumVertices`;
* twelve file-scope statics (`PrimUV0`, `PrimUV0S16`, `PrimUV1`, `PrimUV1S16`,
  `PrimNormal`, `PrimNormalS8`, `PrimColor`, `PrimBones`, `PrimWeights`,
  `PrimWeightsS8`, `PrimVQStart`, `PrimVertexGroup`) hold the current vertex
  attributes; the ten setters each write one, and `Clear()` resets eleven of
  them — reading `0.0f` out of `m_scale`, which it has just zeroed, rather than
  materialising it;
* they are constructed at load by gcc 2.x's
  `__static_initialization_and_destruction(1, 0xffff)`, which is **static and
  therefore absent from the symbol map** — so `Allocate__12VIPrimBuffer`'s
  derived span (360 bytes) swallows it whole and will need a `SPAN_CAP` entry of
  72 (its last word is one past a `jr ra`, and the swallowed function has a
  `jr ra` of its own).
