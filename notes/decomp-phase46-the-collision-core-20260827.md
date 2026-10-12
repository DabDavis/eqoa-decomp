# Phase 46 — the collision core

**2026-08-27. Twenty functions written, eight byte-exact, and one of Phase 22's
open four closed on the way past — nine in all. Tree: 2,185 → 2,194 of 2,438.**

Phase 22 settled `VICollide` the *class* — the buffer pool, the transform pair,
the three dispatchers, the four `Cull` overloads and `DebugResources`, 18 of 22.
This phase is the other twenty symbols in that translation unit, which are the
collision *itself*: the six buffer walkers, the three query walkers, the
per-primitive tests (triangle, edge, vertex, sphere, capsule, cylinder), the
point-in-triangle test and the capsule entry point.

As in Phases 11/13/18/22 the semantics were settled from the console **first**:
`go-eqoa-vi/vicoll` is a bit-exact Go port of every one of these, checked
against a live SUPPORT through the tracer's `call`. So every mismatch here is
codegen and nothing else, and every finding below is a codegen finding.

## What matched

| | |
|---|---|
| `PointInTriangle` | 130 words, byte-exact on the FIRST build |
| `Collide(VICollRay &, int, tri)` | 124, exact on the second |
| `Cull(VICollSphere &, VIBBox &)` | 86 |
| `Cull(VICollRay &, VIBBox &)` | 67 — **open since Phase 22**, closed here |
| `CollideObj(VICollSphere &, VICapsule &)` | 70, exact on the first build |
| `CollideBufferPackV` ×2 (sphere, ray) | 241 each |
| `QueryBufferV`, `QueryBufferPackV` | 167 and 226 |

## Six levers

**A chain of six rejections is one `||`, not six `if`s.** `Cull(VICollRay &,
VIBBox &)` had been open since Phase 22 at 30 of 68 words, with a comment saying
the shape was settled and the difference was "the standing phenomenon". It was
not: the six tests are one `if ((a && b) || (c && d) || …) return 1; return 0;`.
Six separate `if (…) return 1;` statements leave the FIRST `return 1` in line and
jump to it from the other five; the `||` chain puts the single `return 1` after
all twelve compares, which is what the target does. Byte-exact, and the swept
form (`Cull(VICollSphere &, VIBBox &)`) was byte-exact the moment the same
spelling was used.

**A vector copy followed by in-place component updates keeps the first two
stores and loses the third.** `hp = r.m_delta; hp.x = hp.x * t + r.m_start.x;
…` emits the copy's store for x and y and dead-eliminates z's, which is exactly
the 2/2/1 store pattern the target has at three separate sites. Written through
a pointer variable (`VIVect3 *d = &r.m_delta; hp.x = d->x * t + …`) the function
is two words SHORT and everything after the first store shifts.

**An early exit written as `goto` keeps the accumulator; written as `return
hit` it folds to a constant.** In `Collide(VICollSphere &, int, tri)` the
`t < m_hitTime` rejection has to skip the three edge tests, so it cannot be a
nested `if`. Written `return hit;` gcc gives that exit a block whose only
reaching definition of `hit` is the initial 0 and emits `move v0,zero`; written
`goto done;` it lands on the shared `return hit` at the bottom, which has two
reaching definitions, and emits `move v0,s2` — the target's. Worth 5 words, and
the same shape appears again in `Collide(VICollSphere &, VICapsule &)`.

**`n = numVerts - 2` in an initialiser is not the same as `n = numVerts;` then
`n = n - 2;`.** Split into two statements the `lh` goes straight into the loop
counter's register; folded into the initialiser it lands in a scratch and the
subtract reads it. Twelve of `QueryBufferV`'s sixteen remaining words, and it
took `CollideBufferPackV` from 65 to 14.

**A skip's statement order decides which of the two loads is issued first.**
The group-culled path of every walker advances two cursors. Written
`vert += …m_numVerts; prim += …m_numPrims;` the loads come out numPrims-first;
written the other way round they come out numVerts-first, which is the target's,
and the two ADDs stay vert-then-prim either way. The last four words of
`QueryBufferV`, and it closed `QueryBufferPackV` too.

**Reload assigns stack slots in DECLARATION order, not first-assignment
order.** `CollideBufferPackV` sat at 10 words wrong, all of them the two spill
slots `48(sp)` and `52(sp)` being swapped between `b.m_verts` and the hit
accumulator. Declaring `char *vp;` before `int hit = 0;` and assigning
`vp = b.m_verts;` after the two early returns — the same code, the declaration
moved — is byte-exact.

## Two facts about the source, not about codegen

**`VICapsule` and `VICylinder` have a zeroing default constructor**, and the
zeroing comes out **twice** for the two vectors and **once** for the two floats:
the members' own `VIVect3()` runs first, then the body's
`m_base.Zero(); m_dir.Zero(); m_length = 0.0f; m_radius = 0.0f;`. That is the
same two-pass signature `VIBBox` has (Phase 28) and it is how the three stack
objects in `Collide(VICollSphere &, VICapsule &)` were identified.

**Two of these functions carry a `VIPlane` local that is not obviously a
plane.** `CollideNoCaps` and `Collide(VICollSphere &, VIVect3 &, VIVect3 &)`
both have sixteen zeroed bytes at the top of the frame where three vectors would
be twelve. It is a `VIPlane`: the normalised cross product is `pl.n`, the
negated dot with the base is `pl.d`, and the slab distance the two rejections
test is `pl.Distance(start)` — which is why the target NEGATES and ADDS where a
subtraction would be shorter. Modelling it that way took the edge collide from
252 to 212 wrong words in one edit.

**`VIPlane::Distance` is inline** — it has no symbol of its own — and both
triangle collides read the POINT first: `mul.s f0, f9, f5` is `start.x * n.x`,
not `n.x * start.x`.

## What is still open, and why it is two things and not one

**Register allocation (the five V and PackVGF walkers, the capsule collide, the
edge collide).** These are not shape problems: `CollideBufferPackV` is
byte-exact from the *same body* the V walker uses, with only the vertex fetch
substituted. Under packing 0 the target keeps `b.m_verts` in a callee-saved
register and multiplies by 12 with `mult`+`addu`; ours spills it, and gcc then
folds the reload into `mtlo`+`madd`. One register — and every callee-saved name
after it shifts, which is why 121 of 208 words "differ".

**The entry block's schedule (the three point/sphere collides).** All three have
the same twenty-odd words in the same place: the target issues all six loads
before the first subtract and stores the difference vector before the direction
loads, where ours interleaves them; and the accumulator's `hit = 0` fills the
first branch's delay slot there and sits in the prologue here. Permuting the
three component statements, naming the vectors through pointers, compound
operators, and moving the declaration all move it by at most one word.
`Collide(VICollSphere &, VISphere &)` is the control that says it is not the
source: hoisting `rad = s.m_radius + v.r` out of the guard — the delay slot
carries it, so it is unconditional — took it 37 → 23 and changed nothing else.
This is the load-hoisting shape `decomp-scheduler-rig-20260822.md` attributes to
the R5900 `load` ready-delay, where 2 gives ours and 1 gives the target's order.

## Two shipped facts the port had already found, now confirmed against the code

`Query`'s FIRST `VIBBox` parameter is dead in all three walkers — a2 and a3 are
stored in the prologue and a1 never is. `QueryObj`, its only in-engine caller,
puts the world-space box there and its image under `m_invWorld` second.

Under `PackVGF` the index a query reports is the **vertex's flora byte**, signed,
read from +7 of the newest vertex — the prim's material never reaches the list at
all. The other two packings report the material.
