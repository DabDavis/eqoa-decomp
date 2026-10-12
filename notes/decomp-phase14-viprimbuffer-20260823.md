# Phase 14 — `VIPrimBuffer`, the GS display-list builder

**2026-08-23. 468/534 → 496/565.** `VIPrimBuffer` is 29 of 31, on the settled
toolchain (`ee-gcc2.95.3-107 -O2 -G0 -fno-exceptions`, assembled with SN's
`ps2eeas` out of the 2.95.3-114 package) with **no re-tuning**. Fourteen of the
29 matched on the first build.

It is VICollBuffer's render-side sibling: the same `VIBuffer` base, the same
`VIBBox`, the same three num/max cursor pairs and three section pointers rebased
by `m_data`. What it fills is different — a **GS display list**, where every
vertex is one or more QUADWORDS carrying a VIF tag. That is why the vertex
section is indexed by `<< 4` and why `CalcDataSize` counts quadwords.

## `VIVect4` is `aligned(16)`, and VIPrimBuffer is what proves it

The class's four packing scales are one `VIVect4` member and it sits at
**+0x30**, four bytes past the end of the `VIBBox` above it. Every offset in
the class comes out one word short without that hole. Adding
`__attribute__((aligned(16)))` to `VIVect4` fixed **nine** functions at once
(constructor, `Init`, `Lock`, `GetNumVertices`, `GetNumTriangles`,
`SetPacking`, `BeginMaterial`, `EndMaterial`, `Allocate`) and **regressed
nothing** in the 468 already matched.

It also explains something Phase 3 had covered with an attribute: `VIMatrix44`
comes out 16-aligned because its `VIVect4 m[4]` is, not because the matrix class
needs one of its own.

## The layout

```
+0x00  int             m_id            (VIBuffer)
+0x04  int             m_userData      (VIBuffer)
+0x08  int             m_type
+0x0C  int             m_locked
+0x10  int             m_userFlags     zeroed by Clear, read by nothing here
+0x14  VIBBox          m_bbox
+0x2C                  -- 4 bytes of alignment padding --
+0x30  VIVect4         m_scale         x position, y UV, z normal/weight, w colour
+0x40  int             m_numMaterials
+0x44  int             m_maxMaterials
+0x48  int             m_numPrims
+0x4C  int             m_maxPrims
+0x50  int             m_numVerts      in QUADWORDS
+0x54  int             m_maxVerts      in QUADWORDS
+0x58  VIPrimMaterial *m_materials     stride 0x10
+0x5C  VIPrimPrim     *m_prims         stride 0x1C
+0x60  char           *m_verts
+0x64  int             m_dataSize
+0x68  char           *m_data
```

`VIPrimMaterial` is `{ int m_matIdx; int m_numPrims; int m_firstVert; int
m_numVerts; }`. `VIPrimPrim` is `{ int m_numVerts; VIBBox m_bbox; }` — an `int`
count where `VICollPrim` packs two shorts, because the VIF-tag arithmetic reads
it with `lw`.

## The nine type values are read off the DISPATCHERS, not guessed

`VIRaster::PickPrimBuffer(VIPickSegment&, int, int)` is an if-chain testing
**0, 2, 4 and 5** in that order and calling `PickPrimBufferVUNC` /
`PackVUNC` / `PackVUNCG` / `PackVUNBW`.
`VIRaster::DrawPrimBuffer(int, int, float, unsigned)` tests **8** alone and
calls `DrawPrimBufferPackVU`. So:

```
VUNC = 0, PackVUNC = 2, PackVUNCG = 4, PackVUNBW = 5, PackVU = 8
```

and `sltiu v0,v1,9` in every table dispatch follows from the maximum being 8.
The four unused values (1, 3, 6, 7) are named in the header for the shape of the
enum only; nothing in the module reads them.

`CalcDataSize` confirms the ordering independently: its five case bodies compute
`prims*3 + verts*K` with K = **4, 2, 2, 2, 1** — four quadwords per vertex for
the unpacked VUNC (position, UV, normal, colour), two for each packed
VUNC/VUNCG/VUNBW, one for PackVU.

## The tags are VIFcodes, and that unlocked the last structural puzzle

`0x6C008000`, `0x6D028001`, `0x6E028003`, `0x69028000`, `0x6E038000`,
`0x69028001` are VIFcodes: CMD in bits 24-31 (`0x6C` = UNPACK V4-32, `0x6D` =
V4-16, `0x6E` = V4-8, `0x69` = V2-16), NUM in bits 16-23, IMMEDIATE in bits
0-15. `Begin()` reserves two quadwords for the pair `End()` comes back and fills
in; `PrimVQStart` remembers where.

The last word `End()` writes is `X | 0x14000000` — and `0x14` is **MSCAL**, the
VIFcode that starts a VU1 microprogram. So `X` is a microprogram entry offset,
and the symbol map has exactly the right family sitting in `VIRaster`'s statics:

```
004E06E8 _8VIRaster$RInitOffset      004E0710 _8VIRaster$RStrip4Offset
004E06F0 _8VIRaster$RMaterialOffset  004E0718 _8VIRaster$RStrip5Offset
004E06F8 _8VIRaster$RStripOffset     004E0720 _8VIRaster$RStrip6Offset
004E0700 _8VIRaster$RStrip2Offset    004E0728 _8VIRaster$RFanOffset
004E0708 _8VIRaster$RStrip3Offset
```

**This is why `End`'s four packed cases look byte-identical in the image and
still do not cross-jump.** The image is un-relocated, so the symbol each `lw`
names is zeroed out and the four nine-instruction bodies compare equal. At
compile time they are five DIFFERENT symbols, `find_cross_jump`'s backward walk
stops on the first insn, and the merge covers only what follows — which is
exactly the shared block the target has. Writing one shared `PrimColor` load
instead merged all four bodies away; giving each case its own symbol restored
the length to 166 exactly.

Which offset goes with which format cannot be read back out of the image, for
the same reason: the relocation masks it. The ascending assignment in the source
is the declaration order of the data symbols, and it is unverifiable from the
image alone.

## Lever: the chained assignment is what makes a float zero a REGISTER

`Clear()` resets eleven static float fields, and the target stores every one of
them with `swc1 $f0` where `$f0` came from **`lwc1 $f0,4(v0)` — a load of
`m_scale.y`, which the same function zeroed three statements earlier.**

Written as separate statements, each store is `(set (mem:SF) (const_double 0))`.
cse would rather use a cheaper equivalent than the constant (a `CONST_DOUBLE`
costs `COSTS_N_INSNS(2)` on this target, a MEM one), but mem-to-mem is not a
valid insn, so it falls back and every store comes out `sw $0`. Measured: eleven
`sw $0`, no load.

Written as a CHAIN — `PrimUV0.y = PrimUV0.x = 0.0f;` — there is one temporary
holding the value for the whole chain, cse replaces **that register's** constant
source with the equivalent MEM, and the eleven stores become `swc1` off it. The
byte and halfword statics are NOT chained: an integer zero is free in `$0`, so
they need no temporary and their store order is plain source order.

The chains run last-member-first (`.y = .x`, `.z = .y = .x`,
`.w = .z = .y = .x`); the other direction scored 70/103 against 0/103.

## Lever: a local declared INSIDE the case

`Vertex(VIVect3 &)` builds a `VIVect4` on the stack for the colour. Declared at
function scope, its zeroing constructor's four stores are hoisted **above the
switch test**; declared inside the case block they land after it, where the
target has them. That one move took the function from 43 wrong to byte-exact.

The same lever runs through `CalcBVolumes`: its `VIVect3 v` is constructed in
each of the five decode cases (all five bodies carry the three zeroing stores),
and `pb.Bound(fp, v)` is written in with it and left to cross-jump into one
call — 109 wrong to 67.

## Lever: which of two identical stores ends the block

`Unlock`'s PackVU arm sets two of the four scales. Written
`m_scale.w = …; m_scale.z = …;` gcc emits the pair in reverse, so the block's
last instruction is `swc1 $f0,60(a0)` — identical to the default arm's — and
cross-jumping deletes it, losing the store. Written `z` then `w`, the last
instruction is `swc1 $f0,56(a0)`, nothing matches, and the function is
byte-exact. **The store order decides whether a block is a cross-jump
candidate**, and the source order that produces it is the reverse of the one you
would guess from the emitted code.

## Phase 9's pointer lever, three more times

`Clear` names `m_scale` through a pointer (`addiu v0,s8,48`, then `4(v0)`,
`8(v0)`, `12(v0)`, with `+0` folded back into `48(s8)`); `CalcBVolumes` names
the packed position the same way (`VIVect3S16 *pv = (VIVect3S16 *)(vp + 4);`
gives `lh 4(s0)` for x and `2(v1)`/`4(v1)` for y and z); and
`Vertex(VIVect3S16&)`'s PackVUNCG and PackVUNBW arms do it for the destination.

## `Begin`'s `int` and `Init`'s `VIRaster &` are both DISCARDED

`Begin(int)` never reads `a1`, and `Init(VIRaster &, VIPrimBufferType, int, int,
int)` never reads its raster. Same shape as `VICollBuffer::CalcBVolumes`'s
unused `int`, and `VIPrimBuffer::CalcBVolumes` has that one too.

## Harness: `SPAN_CAP` for `Allocate`, and `_q.py`

gcc 2.x's `__static_initialization_and_destruction(1, 0xffff)` — which
constructs the ten class statics that have zeroing constructors — is STATIC and
therefore absent from the symbol map. It sits between `Allocate` and
`_GLOBAL_$I$_12VIPrimBuffer$PrimUV0`, so `Allocate`'s gap-derived span swallows
it whole: 360 bytes for an 18-instruction function. `SPAN_CAP` gets a fifth
entry, 72, and `test_span_caps_only_ever_shrink` holds it to the same three
conditions as the other four.

`decomp/_q.py` compiles ONE translation unit and scores named symbols out of it:
4.5 s a build against ~60 s for a full `eqcheck.py` pass. Several of the levers
above were found by sweeping thirty-two source variants, which is not affordable
at the whole-tree price.

## Lever: cross-jumping does not come back for a second pass

`CalcDataSize` has five case bodies of which **three are byte-identical**. With
the closing three statements written ONCE after the switch, our build collapses
all three into one — 87 instructions against the target's 153. With the same
three statements written **into each case** and left to cross-jump, the length is
153 exactly and the bodies stay apart.

The mechanism: `find_cross_jump` walks BACKWARD from a block's jump. Written
into each case, the first thing it finds is the shared closing statements, it
merges those, and it does not come back for the prefixes that are now identical.
Written after the switch there is no tail to merge, so the first thing it finds
is the whole body.

This is Phase 13's "a shared tail written into EACH case cross-jumps" lever,
turned around: there it explained why a constant stayed duplicated in the
predecessors; here it explains why whole bodies do.

`CalcBVolumes` carries the same shape twice — its PackVUNC and PackVUNBW decode
arms are byte-identical and separate — and comes out at the right length for the
same reason.

## Lever: a fresh local for the intermediate, in INTEGER code

The 16-byte alignment helper is four lines, and how the middle two are spelled
decides the whole function:

```c
int m = n & 15;
int t;                       /* <- this */
int r;
if (m) { t = n + 16; r = t - m; } else { r = n; }
return r;
```

Written `r = n + 16; r = r - m;` the allocator coalesces `r` with `n`, the else
arm's `move` and its branch vanish, and the diamond collapses to three in-place
instructions. With `t`, `n` keeps its own register, the `move`/`b` survive, and
`r` takes the MASK's register — which is the target, byte for byte, at all ten
inline sites. Seven spellings were swept; only this one matches, and the two
that fold `+ 16 - m` into `- (m - 16)` are the furthest away.

Phase 9's "a fresh local per squared distance" rule, in integer code.

## Lever: the ORDER of a case's locals decides where the split lands

`End`'s five arms each read three case-specific values. Declared

```c
int s = PrimVQStart;
unsigned int tag = <VIFcode>;
int prog = VIRaster::RStripNOffset;
```

the function is byte-exact. Move `prog` above `tag` and it is 59 wrong: the
backward walk stops on the symbol load, so **everything before it stays
duplicated in the five prefixes and everything after merges**. Read last, the
prefixes keep the quadword start, the VIFcode, the vertex-section load and the
shift — nine instructions, which is what the target has. Read first, the VIFcode
and the shift merge away too and the prefixes come out two instructions short.

Ten declaration orders were swept; the two that match both have `prog` last.

## `PrimVertexGroup` is `unsigned short`

`Vertex(VIVect3S16&)`'s PackVUNCG arm reads it with **`lhu`**. The setter's
mangled name (`VertexGroup__12VIPrimBuffers`) says its PARAMETER is a signed
`short`, but the static it writes into is unsigned — a signed static read into
any local type gives `lh`, and no local type recovers the `lhu`.

The same arm wants the value in a local read at the TOP of the case: left as a
direct read at its store it is loaded late and the arm is 11 instructions worse.

## Lever: one pointer base per byte group

`Vertex(VIVect3S16&)` writes four groups into a 32-byte record, and the target
materialises a base register for SOME of them — Phase 9's rule, applied per
group rather than per class:

| arm | position | colour | weights |
|---|---|---|---|
| PackVUNC | off `p` | `p + 28` | — |
| PackVUNCG | `p + 4` | `p + 28` | — |
| PackVUNBW | `p + 4` | — | `p + 24` |
| PackVU | off `p` | — | — |

Reading that table off the target's `addiu` instructions and writing exactly
those groups through a pointer took the function from 218 wrong to 31; every
other combination of the seven knobs scored worse, and the two that add a
pointer where the target has none also come out two instructions long.

## Open: two, both allocation

| function | wrong | what is left |
|---|---|---|
| `Vertex(VIVect3S16&)` | 20/328 | one register shift, confined to the PackVUNCG arm: the target keeps the vertex-group address live where ours reuses it for the position base |
| `CalcBVolumes` | 66/273 | scheduling inside the PackVUNCG decode arm, and two unfilled load-delay slots at the head of the material loop that our schedule fills |

Neither is structural: the block boundaries, the three switch tables, the
pointer bases and the call placement all align.
