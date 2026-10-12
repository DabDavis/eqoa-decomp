# Phase 51 — `viworld.cpp`, the world tree

**One translation unit, `VIWorld` — SUPPORT `0x0048E380 .. 0x004924D0`.  37 of
its 44 functions written, 27 byte-exact.**  The top of the spatial hierarchy:
a rectangular terrain grid, an array of zones, and a BSP tree over the zones.
Every scene-level query — render, collide, pick, insert, stream, and the five
`Query*` — arrives here as ONE `Vars` reference and is walked down the same
descent, which hands each leaf to the `VIZone` that owns it.

`go-eqoa-vi`'s `vicoll/scene.go` walks this tree against the console and was
the semantic reference for the phase; this is the source that produced it, and
the two agree.  The sibling TU, `vizone.cpp`, is phase 52: its descent is
instruction for instruction the same function.

## What the class is

**976 bytes**, and the size is not a guess: `SetProfiles`' last store is `+964`,
inside the 22nd and last 40-byte terrain profile, and `96 + 22*40` is 976.

    +0x00  VIBBox      m_bbox        -1000 .. +1000 from the constructor
    +0x18  int         m_numRows     1
    +0x1C  int         m_numCols     1
    +0x20  float       m_cellSize    2000.0
    +0x24  VIVect3     m_origin
    +0x30  int         m_subdiv      4
    +0x34  VIWorldTree m_tree
    +0x48  VIArray<VIZone>        m_zones      stride 592
    +0x50  VIArray<VIWorldRegion> m_regions    ONE PROFILE BYTE per sub-cell
    +0x58  float, float                        250.0 and 800.0
    +0x60  VITerrainProfile m_profiles[22]     40 bytes each

**`VIWorldTree::m_state` IS the pass stamp.**  Phase 27 named it `m_state` off
the constructor's `= 1` and left it at that.  Every traversal wrapper here
copies `m_tree.m_state` into the Vars, runs the walk, and increments it — so
the world's own visit counter lives inside its tree object, at `world+0x44`,
and it is what the room-level code compares against.

**A `VIWorldRegion` is one byte, and the region array is the terrain-profile
grid.**  `Init` sizes it `m_numRows * m_numCols * m_subdiv * m_subdiv` and
`GetTerrainProfile` ends in an `lbu`: the "regions" are profile INDICES, one
per sub-cell of the world grid.

**A child index is TAGGED.**  Any bit of `0xF0000000` says "not an ordinary
interior index"; `0x40000000` on top of that says LEAF; **any other tag is
stripped and used as an interior index anyway**, so the tag space is not a
simple leaf/interior bit.

**There are three descent shapes, not one.**  A SEGMENT walk (collide sphere,
collide ray, pick, the env ray) takes the signed distance of BOTH sweep
endpoints; a SPHERE walk (radial stream, proximal query, emitter query) takes
one point and a radius; a VOLUME walk (render, bbox stream, volume query)
tests EIGHT corners and sets a front/back flag pair.  What separates the three
segment walks is only what `r` is — `m_radius + 0.03125` for the sphere
collide, the bare `0.03125` for the ray, `0.0` for the pick.

## The levers

**The constant-selecting wrapper is TWO CALLS with the test inside each arm.**
Twelve wrappers here choose between node `0` and the leaf tag `0x40000000` and
call the descent.  Written as `int n; if (size == 0) n = LEAF; else n = 0;
r = Collide(n, v);` gcc if-converts the select to `movz` and the branch is
gone.  Written as two calls with the `!= 0` (or `< 0`) test INSIDE each arm,
gcc cross-jumps them back into ONE call, and the branch survives — which is
what the target has.  **Eleven of the twelve went byte-exact on that one edit**,
and the twelfth (`QueryVisible`) needed only its arms the other way round.

**`m_tree.m_nodes[node]` is not `m_tree.m_nodes.m_data[node]`.**  Written out,
the member chain folds to a single `lw <d>, 56(this)`.  The target instead has
`addiu <t>, this, 52` followed by `lw <d>, 4(<t>)` — an ADDRESS PSEUDO for the
array object, kept in a callee-saved register across every recursive call.
That is phase 32's "an inline member's `this` is an address pseudo", and it is
reached by giving `VIArray<T>` an inline `operator[]`.  `Collide(int,
VISceneCollVars&)` went from 58 wrong words to 17 on that change alone, and the
same spelling applied to `m_tree.m_leaves` closed it completely.  The
subscript is NOT used everywhere: `m_zones.m_data[...]` inside the leaf
dispatch stays written out, because the target folds THAT one.

**Two distances computed before the first test are LOCALS, reassigned.**  The
descent's first block computes the plane distance of BOTH sweep endpoints,
interleaved, before the first branch — and then recomputes both after each
recursive call.  Spelling the expression out at every use gives short-circuit
evaluation, with the second dot product in the successor block (207 of 227
words wrong).  Declaring `float ds, de;`, filling them once, and ASSIGNING THEM
AGAIN after each recursive call gives the target's shape (100 wrong), and the
`else if` chain plus `hit = 0; if (call) hit = 1;` closed it.

**`int r = -1; if (0 <= X()) r = 0; return r;`** is the tail every `Clear`-like
function has: `slti v1,v0,0`, `li v0,-1`, `movz v0,zero,v1`.  Written
`return X() < 0 ? -1 : 0;` gcc materialises `-1` into a register, compares
`-1 < r`, and uses `movn` — the same value, six words apart.  `VIWorld::Clear`
and `VIWorld::Init` both closed on it.

**A clamp on a REFERENCE parameter is ONE read-modify-write.**  `CalcTerrainCell`
writes `row` and `col` through `int &`, then clamps them.  Two `if` statements
on the reference give a branch and a store EACH; the target has one `lw`, two
conditional moves and one `sw`.  The source clamps a LOCAL and assigns the
reference once — and the second clamp is an `if`, not a ternary, because the
destination must start holding the value and take the bound conditionally
(`movn`), not the other way round.

**The clamp ORDER is a register-allocation fact.**  `GetTerrainProfile` clamps
COL fully and then ROW.  Every other order — row first, negatives first,
bounds hoisted into locals, `VIMAX`/`VIMIN` — costs eleven words of register
naming and nothing else.

**`bb.min = at; bb.min -= r; bb.max = at; bb.max += r;`** — in that order.
`VIVect3` already has `operator-=(float)` and `operator+=(float)` from phase 20;
building the two corners componentwise costs 26 words, and doing both
assignments before both adjustments costs 26 more.

**A local whose constructor emits code belongs in the scope that uses it.**
`QueryVisibleWithPortals`' `VIBBox` is constructed AFTER the early returns, not
at the top: the target's stack frame is 160 bytes and the zeroing stores sit
past the portal test.  Declared at the top the frame grows to 176 and the
stores move to the entry block.

## Open

`SetProfiles` (600 words) and `CalcProfileBlend` (166) are the terrain
time-of-day table and are not attempted; `QueryVisible(int)`,
`QueryIntersection(int)`, `StreamLeaf(int, VISceneBBoxStreamVars&)` and
`Collide(int, VIEnvRay&, VIVect3&, VIVect3&)` are written up in the header but
not yet in the source.

Of the ten written-but-open:

* `InsertActor(int)` 35/135, `StreamLeaf(radial)` 26/159 and
  `QueryVisibleWithPortals` 19/110 are structure — each has one block whose
  call pattern has not been matched yet.
* `RenderByVolume(int)` 15/143, `Stream(int, radial)` 17/116 and
  `QueryProximal`/`QueryEmitters` 15/110 are all ONE phenomenon: the last
  recursive call of the two arms cross-jumps in ours and does not in the
  target, so the two arms' tails must differ in a way not yet found.
* `FindLeaf` 28/73 wants `&m_tree.m_leaves` and `&m_zones` materialised in BOTH
  predecessors of the join, which a reference variable at the top does not give.
* `CalcTerrainCellBVolume` 21/63 needs `&m_origin` as an address pseudo while
  the three components are negated into a stack VIVect3 — an inline that takes
  `const VIVect3 &` and negates, which `vi.h` does not have.
* The constructor is TWO words: `li v1,21` and `addiu v0,s0,96` come out in the
  other order in the member array's implicit construction loop.

## Harness

`VIWorld` moved to the BOTTOM of `vi.h` — it holds `VIArray`s and the template
is declared 2,000 lines below `VIScene`, which owns a `VIWorld` at `+0x04`.
`VIScene` carries it as `char m_world[976]` and
`VIRadialFloraSystem::CalcZoneIndex` casts; the cast emits the same
`scene + 4` the member did.  The eight element classes `viarray.cpp` kept as
local stubs (`VIWorldNode`, `VIWorldLeaf`, `VIZoneNode`, `VIZoneLeaf`,
`VIWorldRegion`, `VIRadialFloraModel`/`Dist`/`Set`, `VIZoneStaticTableElem`)
moved to `vi.h` with them, and `viarray.cpp` still scores 132/132.
