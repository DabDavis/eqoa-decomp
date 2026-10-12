# Phase 47 — the hierarchical sprite

**2026-08-27. `VIHSprite`, the class every animated character in the engine
is: a node tree, a palette of blend matrices, a trigger table and three pools.
Phases 19/20 had taken nineteen of the twenty-six readers it already had; this
phase is the other fifty-five symbols in the translation unit — the constructor,
the play list, the pose evaluator, the centre-transform cache and all eight
virtuals. Fifty-five written, forty-two more byte-exact; the TU is 81 symbols
and 67 of them are exact. Tree: 2,194/2,438 -> 2,236/2,493.**

As in Phases 11/13/18/22/46 the semantics were settled from the console
**first**: `go-eqoa-vi/vianim` is a bit-exact Go port of `ProcessPlayList`,
`ProcessPlayTiming`, `ProcessHierarchy`, `SetBindPose`, `InterpTransAboutCenter`
and `UpdateCenterTransform`, checked against a live SUPPORT through the tracer's
`call`. So almost every mismatch here was codegen, and — as in Phase 18 — the
decomp paid the port back: it corrected two of `vi.h`'s own field names and
found a member the port had modelled as padding.

## What the class actually is

The constructor (0x00445AF0) and `Copy` (0x00448408) between them name every
field, and `go-eqoa-vi/vianim` had measured most of the same offsets off a live
game:

```
+0x040 VIVector<VIHSpriteNode>     m_nodes
+0x04C VIVector<VIMatrix44>        m_blend        the skinning palette
+0x058 VIArray<VIHSpriteTrigger>   m_triggers
+0x060 VIPool<VIHSpriteAttachment> m_attach
+0x078 VIPool<VIHSpritePlay>       m_plays
+0x090 VIPool<VIHSpritePlayNode>   m_playNodes
+0x0A8 VIHSpriteLOD                m_lod          count + three distances
+0x0B8 long  m_playTime     ProcessPlayList's cache stamp
+0x0C0 long  m_centerTime   CalcTransAboutCenter's
+0x0C8 long  m_poseTime     Process's
+0x0D0 float m_defaultScale
+0x0D4 VIVect3        m_center[2]      [0] blend from, [1] live
+0x0EC VIHSpriteFrame m_transform[2]   [0] blend from, [1] live
+0x12C int   m_blendMS      0 == no blend in progress
+0x130 long  m_blendStart
+0x140 VIMatrix44 m_centerMatrix
+0x180 int   m_refMap       shared/released through VIScene::Share/ReleaseRefMap
+0x184 int   m_defaultAnim  GetDefaultAnim
```

**The two frame names in `VIHSpriteNode` were the wrong way round, and the
console port was the one that had them right.** `SetNodeBindPose` (0x00446058)
and `GetNodeBindPose` write and read +0x7C / +0x8C / +0x90, so the BIND pose is
the *second* frame in the node and +0x10 is the *animated* one — which is what
`vianim`'s `NodeRot 0x10` / `NodeBindRot 0x7C` says. And +0xA0 is not a second
world matrix: `SetNodeBindPose` builds `Invert(Transform(q, s, t))` there and
then multiplies a child's world bind by its parent's copy to recover the child's
local rest transform, so it is the INVERSE bind-world matrix — which is exactly
what `SetBlendMatrices` hands `VIRaster::SetObjectBlends` as the skinning
palette (`blend[i] = invBindWorld[i] * world[i]`).

**`m_center` and `m_transform` are ARRAYS of two, not four separate members.**
The constructor zeroes them in LOOPS — two turns of twelve bytes at +0xD4 and
two turns of thirty-two at +0xEC — and no pair of separate constructors gives a
loop.

## Six levers

**The operand order of a comparison decides which side is loaded first, and
that decides whether the guard's load survives a CALL.** Three functions —
`IsCenterTransformBlendComplete`, `SetCenterTransform`, `BlendToCenterTransform`
— open the same way: `m_blendMS == 0 || <the blend has expired>`. Written
`m_blendMS < sc.GetTime() - m_blendStart` the member is the compare's LEFT
operand, gcc evaluates it first, CSE ties it to the guard's own load and the
register allocator parks it in a callee-saved register across `GetTime` — one
extra saved register, one extra stack slot, thirteen wrong words. Written
`sc.GetTime() - m_blendStart > m_blendMS` the subtraction is evaluated first,
the member is re-loaded after the call, and the guard's load dies at the branch.
`IsCenterTransformBlendComplete` 13/21 wrong -> byte-exact, and the same one-line
edit closed `SetCenterTransform` (33/51 -> 0) with it.

**A member whose constructor emits ONE store, landing between the last
out-of-line member constructor and the first inline array loop, is a MEMBER and
not a body statement.** `sw zero,168(s0)` sits between the sixth pool's
constructor call and the `m_center` array's zeroing loop — a place no body
statement can reach, because member constructors run in declaration order before
the body. So +0xA8 is an object with a zeroing constructor. `Copy` says what
shape: it carries +0xA8..+0xB8 across as two unaligned eight-byte block moves,
which is the implicit `operator=` of a class with an array member (written field
by field it would be a word and a loop). Hence `VIHSpriteLOD { int m_num; float
m_dist[3]; VIHSpriteLOD() { m_num = 0; } }`, and both `CalcLODLevelIndex`
overloads read exactly that.

**Naming a pool pointer can COST registers.** Phase 27's `VISet<T>::Value`
lever says a written-out `m_nodes[i].m_data` cannot keep `this` on the
container's own register — but the *container* pointer is a separate decision,
and here it goes the other way. `VIPool<VIHSpritePlay> *q = &m_plays; for (i =
q->Begin(); …)` hoists the 104-byte element stride into a callee-saved register
(one extra `li`, one extra `sq`/`lq` pair, a 96-byte frame where the target has
80); the member spelling `m_plays.Value(i)` rematerialises it inside the loop
the way the target does. `Release` went 43 of 79 wrong to byte-exact on that
edit alone, and `Collide(VICollSphere &)` and `QueryIntersection` closed with it.
The rule is not "never name the pool": `ProcessPlayTiming` and `ProcessPlayList`
both NEED the pointer (the target's `addiu s0,s8,120` is right there) — it is
that a pointer whose only uses are the three walk calls costs more than it saves.

**A container pointer declared INSIDE the loop.** Phase 41's "ONE shared local"
run the other way: `SetBindPose` reads `m_nodes.m_size` off `this` and
`m_nodes.m_data` off a materialised `&m_nodes`, and the `addiu` for it must land
in the guard's delay slot — which it does only when the pointer is declared in
the loop BODY and gcc's own loop pass hoists it to the preheader. Declared above
the `for`, it is computed before the guard instead and the whole tail shifts by
one instruction. 8 wrong words to 0.

**The outer clamp macro's argument ORDER, a second sighting.** Phase 39 found
that `VIMAX(x, k)`'s first argument decides the compare; here it decides which
of two identical-looking clamps gcc emits. `t = VIMIN(VIMAX(t, 0.0f), 1.0f)`
gives `c.lt.s 0,t` and `c.lt.s t,1.0`; `t = VIMIN(1.0f, VIMAX(0.0f, t))` gives
`c.lt.s t,0` and `c.lt.s 1.0,t`, which is the target. Eleven words in a
209-instruction function, and it was the last eleven —
`InterpTransAboutCenter` is byte-exact.

**A two-case if/else-if on a small int is a SWITCH.** `if (mode == 0) … else if
(mode == 1) …` if-converts and reorders; `switch (mode) { case 0: … case 1: … }`
emits the comparison tree the target has — `beqz` to case 0, `beq 1` to case 1,
fall through to the join. `ProcessPlayTiming` went 137 of 332 wrong to 26 on
that one change (the same shape appears twice more, in `ProcessPlayList`'s blend
dispatch).

And a seventh, small: **a second store through the same element needs a SECOND
named pointer.** The trigger fire writes `m_c` and then `m_b` through
`m_triggers.m_data[idx]`, and the target re-derives the address for the second —
reusing the first pointer keeps it in a register (offset-first `addu`, one extra
`move`), while `VIHSpriteTrigger *tb = &ta->m_data[idx]; tb->m_b = 1;` reloads
and goes base-first. 8 words.

## Shipped facts

`CalcNodeTransform` (0x00446340) constructs and destroys a **second
`VIMatrix44` it never uses** — 64 bytes of stack and two constructor loops per
call, on the path every attachment and every helm goes through.

`VIHSprite::Parent` answers only for node 0 and `VIHSprite::Prev` walks the
node's OWN child list rather than its parent's — both already recorded in Phase
19/20 and both confirmed here by the surrounding code that calls them.

`InterpTransAboutCenter` re-tests `m_blendMS` **three times** after an early
return that has already handled the zero case, so three whole branches of it are
dead. The shape is not a guess: each test is a real `lw`/`bnez` pair in the
object code, and writing the function without them does not match.

## Harness

**A new `SPAN_CAP`.** `vihsprite.cpp` closes with the TU-local copies of
`VIMap<int,int>`'s walkers, its red-black insert and the two rotations — a run
of unnamed statics — so `CalcLODLevelIndex(VIPickSegment &, VIRaster &)`, the
last named symbol in the file, acquires a 2,784-byte span. 268 is the function.

## The two that are open

`ProcessPlayList` (532 words) is the largest function in the class and the one
that is not settled: the target SPILLS `this`, the LOD level, the scene and the
play index to the stack at entry, where the candidate keeps three of them in
callee-saved registers, and every later divergence follows from that one
allocation difference. `ProcessPlayTiming` is 14 of 332 — the last of it is the
register the 64-bit clock difference lands in, twice, and one `addu` operand
order in the trigger fire.

Long form of the class itself: `decomp/README.md`. The pose evaluator's
semantics, and what each field means at runtime, are in
`go-eqoa-vi/vianim` and `claude-notes/vihsprite-pose-evaluator-20260817.md`.
