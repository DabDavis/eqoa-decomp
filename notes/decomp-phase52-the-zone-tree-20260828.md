# Phase 52 — `vizone.cpp`, the zone tree

**One translation unit, `VIZone` — SUPPORT `0x00493830 .. 0x00498CC0`, 67
functions.  50 written, 24 byte-exact (2,484/2,798 -> 2,508/2,851).**  The sibling of phase 51's
`viworld.cpp`, one level down: where a `VIWorld` is a BSP over ZONES, a
`VIZone` is a BSP over ROOMS, and the room is where the scene's actual work —
raster, collide, pick, stream, query — finally happens.

## The two descents are ONE function, and that is measurable

Phase 51 predicted it; this phase checked it.  Disassemble
`Collide__7VIWorldiR15VISceneCollVars` and `Collide__6VIZoneiR15VISceneCollVars`
and diff the two by mnemonic: **they agree instruction for instruction** except
for (a) which register holds `node` (`t0` in the world, `a3` in the zone,
because the world's leaf arm needs more of them), (b) `m_tree`'s offset (52 vs
68) and (c) the leaf arm itself.  The same is true of `QueryProximal`,
`QueryEmitters`, `Pick`, `Stream` and `QueryVolume`.

So the whole descent half of this file is phase 51's source with one line
changed — `m_zones[m_tree.m_leaves[node].m_a].Collide(v)` becomes
`CollideLeaf(node & 0x0FFFFFFF, v)` — and it lands byte-exact on the first
build.  `Collide(int, VISceneCollVars&)` (212 instructions),
`Collide(int, VISceneCollRayVars&)` (204) and `Pick(int, VIScenePickVars&)`
(196) all matched with no tuning at all.

## What is new: the leaf end

A world leaf names a zone and forwards.  A zone leaf names a ROOM, and every
`*Leaf` / `*Room` pair in this file is built out of four facts:

* **`VIZoneRoom::m_p20` is the DESCENT stamp.**  Every leaf compares it against
  `v.m_stamp` — the world tree's own `m_state`, threaded down through the Vars —
  and writes it back, so a room reached through two leaves is walked once.
* **`VIZoneRoom::m_p24` is the SCENE stamp, on a different clock.**  The room
  walkers compare it against `v.m_scene->m_stamp`, a counter at **`VIScene +
  0x544`** — the last word before the embedded `VIParticleSystem`.  The two
  stamps are independent: a room can be visited once per descent and its sprite
  collided once per frame.
* **`m_p1C` is the room's own static sprite**, `-1` when it has none, and it is
  collided/picked/rastered through **`VIZone::IdentityMatrix`** — a static
  `VIMatrix44` the constructor re-`Identity()`s on every zone it builds.  The
  zone's geometry is already in world space, so the model-view for it is the
  identity, handed to `VICollide::SetWorld` / `VIRaster::SetModelView`.
* **`m_p68` is the head of the room's ACTOR list** — a pointer kept in an `int`,
  with the actor id at `+0x04` and the successor at `+0x10`, and the list ends
  at **`-1`, not at null**.

## The levers

**A NAMED REFERENCE to the array is what materialises its address.**  Three
wrappers hand `m_pretrans`'s size and block to `SetPreTranslations`, and the
target reads them as `lw a1, 84(s0)` (the size, folded) and
`addiu v0, s0, 84` / `lw a2, 4(v0)` (the block, through an address pseudo).
`m_pretrans.m_data`, `&m_pretrans[0]` and an inline `T *Data()` **all three fold
to `lw a2, 88(s0)`** — gcc simplifies `&*(m_data + 0)` straight back to the
member and then folds the base.  What does not fold is a named
`VIArray<VIVect3> &pt = m_pretrans;`, and `pt.m_size` still comes out as
`84(s0)` because the `+0` folds back on its own.  `VIZone::Collide` (both
overloads) and `VIZone::Pick` went from 13/33, 13/33 and 12/37 wrong to
byte-exact on that one line.  A pointer (`VIArray<VIVect3> *pt = &m_pretrans;`)
and a `const` reference both work; the subscript does not.

**Phase 51's cross-jump lever, run BACKWARDS.**  The world's `Collide(Vars&)`
wrapper has ONE call, because gcc cross-jumps the two arms of
`if (size) return Collide(0,v) != 0; return Collide(LEAF,v) != 0;` back together.
The **zone's** wrapper has TWO — the world's tail is longer (it also bumps
`m_tree.m_state`), and with nothing after the call the merge is what gcc
prefers, not what the target has.  Writing it as

    int hit;

    if (m_tree.m_nodes.m_size)
        hit = Collide(0, v) != 0;
    else
        hit = Collide(0x40000000, v) != 0;

    return hit;

blocks it: the if/ELSE assigning a shared variable keeps the two `jal`s apart,
where two `return` statements merge them.  So the rule is not "two calls with
the test inside each arm" — it is that the **if/else-into-a-variable** form and
the **two-returns** form differ, and which one the target used has to be read
off the number of `jal`s.

**A `-1` sentinel in a POINTER is two instructions; in an `int` it is one.**
The four room walkers end in a walk of the actor list.  Written
`for (VISceneActorRef *a = (VISceneActorRef *)m_rooms[room].m_p68;
a != (VISceneActorRef *)-1; a = a->m_next)`, gcc builds `-1` with
`lui 0xffff` + `ori 0xffff` at both the entry test and the loop test — four
instructions where the target has two `li`s.  Keeping the cursor as an `int`
and casting inside the body took `CollideRoom`, its ray twin and `PickRoom`
from 30/65 wrong to **byte-exact**.

## Corrections to phase 51's Vars structs

Phase 51 named only the fields `viworld.cpp` touched, and guessed at three.  The
zone's leaf and room walkers hand every one of them to a `VIScene` method, so
they are now read rather than inferred:

| struct | phase 51 | measured |
|---|---|---|
| `VISceneInsertVars` | `+0x04 m_scene`, `+0x0C m_room` | **`+0x04 m_actor`, `+0x0C m_scene`** — the leaf calls `v.m_scene->InsertActor(v.m_actor, v.m_zone, room)` |
| `VIScenePickVars` | `+0x10 m_p10` | **`VIRaster *m_raster`** — `Pick` hands it `SetPreTranslations`, `PickRoom` `SetModelView` |
| `VISceneRendVars` | `+0x34 m_room` | **`float m_viewDist`** — `RenderRoom` squares it against the room bbox's `DistanceSquared` |
| `VISceneProxQueryVars` | `m_at` + `m_radius` | **`VISphere m_sphere` at `+0x08`, `VISceneQuery *m_query` at `+0x18`** |
| `VISceneVisQueryVars` | `m_at` + `m_radius` + `m_p18` | **`VISphere m_sphere` at `+0x08`, `m_query` at `+0x18`**, plus a skip flag at `+0x38` |
| `VISceneEmitterQueryVars` | `m_p08` + `m_at` + `m_radius` | **`VICamera *m_camera` at `+0x08`, `VISphere m_sphere` at `+0x0C`, `VIEmitterQuery *m_query` at `+0x1C`** |
| `VISceneBBoxQueryVars` | seven loose ints | **`VIBBox m_bbox` at `+0x08`, `VISceneQuery *m_query` at `+0x20`** |
| `VISceneStreamVars` | `m_p08`, `m_p0C` | **`VIDictionary *m_dict`, `VIRaster *m_raster`** — `StreamRoomIn` hands `+0x08` to `FindTyped` and `+0x0C` to `ShareMaterialPal` |

Every rename is offset-for-offset, and `viworld.cpp` was migrated with them and
re-scored to prove it: nothing that was byte-exact stopped being so.

## Two things the class does that the notes did not have

**`VIZone::Find` is a BINARY SEARCH.**  The resource directory
(`VIArray<VIResourceElem> m_resources`, 16 bytes an entry, the DictID at `+0x0C`)
is sorted, and `Find(id, off, size)` bisects it — `lo = 0`, `hi = size - 1`,
`mid = (lo + hi) >> 1` with an ARITHMETIC shift, `sltu` on the unsigned key, and
a re-test of `hi < lo` after the loop that decides between "found" and 0.  So a
zone resolves a DictID in log time without a dictionary, which is what makes
`dictid-cross-file-resolution.md`'s cross-file lookup cheap.

**The room streamer is a six-way decision on ONE distance.**  `StreamLeaf`
takes the squared distance from the room's bbox to the camera (or, in the bbox
overload, the overlap of two boxes) and drives two independent triples off it:
the room's own geometry (`in` / `halt` / `out`, gated on `flags & 2`) and the
room's STATIC actors (`in` / `halt` / `out`, gated on `(flags & 0x38) == 0x38`).
The two use different radii — `v.m_inner`/`v.m_outer` for the room, `v.m_p2C`
and `v.m_p30` for the statics — so statics come and go on their own hysteresis,
inside the band where the room itself is already resident.

## Open

The phase's misses are almost all **one phenomenon, and it is phase 51's**: a
function whose last statement is `if (f() < 0) return -1; return 0;` if-converts
in our build (`slti` / `li -1` / `movz`) where the target branches
(`move v1,v0` / `bltz` / `li -1` / `move v0,zero`).  Nine spellings were tried —
the ternary, the named local, the uninitialised local, the inverted
`if (0 <= f()) return 0;`, the accumulator, the if/else — and every one of them
if-converts.  It is the same wall phase 51 hit on `RenderByVolume(int)`,
`Stream(int, radial)` and `QueryProximal`/`QueryEmitters`, and it costs 4 words
on each of five `*Leaf` functions and 10 on each of seven wrappers.

`Find` (35/49) is semantically settled and structurally right — the guard, the
`sra`, the `sltu` and the post-loop `hi < lo` re-test all line up — but the
target re-reads `m_resources.m_data` out of the array object on every iteration
where ours hoists it into a register once, and four spellings did not move it.

`SetPretranslations` (33/47) writes each pre-translation to `m_p[i]` **and** to a
dead 12-byte stack slot, from the same three registers, with the `m_p` stores
FIRST.  Seven spellings were tried; a `VIVect3` local always emits its zeroing
constructor (which the target has not) and always writes the temp before the
destination.

Not attempted: `RenderRoom`, `RenderThroughPortals`, `QueryThroughPortals`,
`QueryVisible(int)`, `QueryIntersection(int)`, `QueryIntersectionRoom` (719
instructions), `StreamRoomIn`, `StreamRoomStaticsIn/Halt/Out`, the three
`VIEnvRay` collide functions, and `Load`.
