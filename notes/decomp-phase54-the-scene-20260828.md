# Phase 54 — the SCENE (`VIScene`)

**2026-08-28.  125 of 140 written, 114 byte-exact; the tree is 2,653 of 3,020.**

`VIScene` (SUPPORT `0x004613A8 .. 0x0046CC80`) is the object the whole engine
hangs off: it EMBEDS `VIWorld`, `VIClipStack`, `VIParticleSystem`,
`VIAtmosphere`, `VIQueryTriList`, `VISceneSoundTrack`, `VISpellEffectSystem`
and `VIRadialFloraSystem` by value, and at **19,372 bytes** it is now the
biggest object in the corpus by a factor of four.  Everything else in the
engine reaches the world, the actors, the sprites and the streamer through it.

The phase began with the tree in a **broken** state — a previous sitting had
retyped `VIScene::m_world` from `VIWorld *` to an embedded `VIWorld` and
`CreateSprite`'s first parameter from `int` to `VISpriteType`, and thirteen
translation units then failed to compile, which reads as a 131-symbol
regression.  Both changes are right; the callers were not updated.  Casting
`m_type` at the thirteen sprite `Copy` sites and spelling `m_scene->m_world`
as a member restored the build with no code change of substance.

## The calling convention

Every `VIScene` method that takes an "actor" takes an `int` that IS a
`VIActor *`, dereferenced with no bounds check — the same convention
`VIZone`'s room lists use (phase 52).  `VIList<VIActor>::Add()` returns that
pointer as an `int`, and `CreateActor` hands it straight back to the caller.

## The structures this phase settled

**`VISceneIntersectVars` was wrong in five of its eight fields.**
`VIScene::QueryIntersection` BUILDS one, and every offset is visible: the
caller's box is copied into **+0x04**, the four pointers go in at
**+0x1C `m_scene` / +0x20 `m_filter` / +0x24 `m_collide` / +0x28 `m_tris`**,
`VIBBox::Store` fills eight corners at +0x2C, and `VISphere::Init(bbox)` the
sphere at +0x8C.  `VIZone::QueryIntersectionRoom` confirms all four pointers
independently.  The old layout had the scene at +0x04 and the box at +0x10.

**`VISceneInsertVars`'s +0x70 is a `VISphere`, not a `VIVect3` and a float.**
`UpdateActor` zeroes FOUR words there twice over, which is `VISphere`'s
double-zeroing constructor; a `VIVect3` member plus a bare `float` leaves the
radius alone.

**`VISceneStreamVars::m_p14` is a `VICollide *`** (`StreamClearCache` stores
`VIScene::m_collide` into it), and **+0x10 is never written by anything.**

**`VISceneStreamRadialVars::m_p2C` and `::m_p30` are FLOATS.**  Both were
`int`; `StreamFillCache` stores stream radii into them with `swc1`, and
`VIZone::StreamLeaf` squares each against a float distance.  Correcting the
types took `VIZone::StreamLeaf` from 29 wrong words to 23.

**`VIActor::m_p70` is a `VIColor32F`, not a `VIVect4`** — the baked static
lighting colour.  `CreateStaticProxyActor` fills it from a `VIColor32`
divided by 255 and `RasterActor` hands it to `VIRaster::SetStaticLighting`.
`CreateActor` leaves it `{0, 0, 0, 1}`.

**`VIScene`'s two streaming-radius arrays are PAIRS.**  `m_p2A48[4]` is
`{outer, inner, outer2, inner2}` = 950 / 900 / 550 / 500 and `m_p2A58[5]` is
the same four with a 1050 in front.  Both cache primers copy one into the
other, and the copy is **`[2]=[1], [1]=[0], [4]=[3], [3]=[2]`** — inner before
outer, twice — which a 24-way `_perm.py` sweep found and which is what makes
`StreamFillCache` byte-exact.  `m_p2A58[0]` is the radius the streaming loop
resets `v.m_radius` to on every pass.

**`VIZone::Load`'s arguments are `(id, zone, VILoader &, file)`** — read off
`StreamInStaticActor`, which passes `VISceneStreamVars::m_zone` second and
`m_file` fourth.  The declaration had them the other way round.

## Levers

**A guard that `return 0`s lets gcc if-convert the LAST test; a guard that
`goto done`s does not.**  `QueryEmittersActor` ends `if (Sprite(n)->QueryEmitters(...) < 0)
return -1;` over a shared `return 0`, and written with three `return 0` guards
gcc collapses the tail into `slti` + `li -1` + `movz`.  Rewriting the three
guards as `goto done` with ONE `return 0` at the bottom leaves the target's
`move v1,v0` + `bltz` and is byte-exact.  Same edit closed
`QueryEmittersAtLocationActor` and `StreamInStaticActor`.  This is phase 38's
"a completion check is NESTED" run from the other end: what blocks the
if-conversion is the number of reaching definitions at the exit, not the shape
of the test.

**Two `return -1`s cross-jump only when they are the SAME statement.**
`CreateActor` has one at the top (`m_actors.Add()` failed) and one at the
bottom (`ShareSprite` failed); written as two `return -1`s the second lands
inline as `b` + `li`, and written as `goto bad` with one `return -1` at the
bottom both branch to it.

**A subscript at each use gives TWO induction pointers; a named reference
gives one pointer and a counter.**  `StopSoundTrack` walks
`m_soundTrack.m_state[0..1]` and the target carries `this + 40k` for the
fields AND `&m_state[k]` for the `Clear()` call, with `slt` against
`this + 80`.  A `VISceneSoundTrackState &st = ...` at the top of the body
gives one pointer plus an `int k` and is 37 words wrong; spelling
`m_soundTrack.m_state[k]` at all four uses is byte-exact.

**`a[k]` and `a.m_data[k]` are not the same to the strength reducer.**
`ReleaseStatic`'s inner loop reads `z.m_rooms[i].m_a3C[k]`.  Written
`.m_a3C.m_data[k]` gcc creates a second reduced induction variable for the
outer room address and SPILLS it (43 of 189 wrong); written `[k]` through
`VIArray::operator[]` it recomputes `i * 120` with a `mult` at the top of the
outer body, exactly as the target does, and the function is one word away.

**A local's constructor survives only if the local is declared with the
others.**  `StreamFillCache(VIBBox)` has a scratch `VIBBox` at sp+192 whose
six zero stores are in the target even though the next statement overwrites
all six.  Declared just before its first assignment gcc dead-eliminates them;
declared at the top of the function beside the vars struct they stay.
Phase 48's "a stack object declared FIRST lands at sp+0" decides which local
gets sp+0; this is the other half of the same rule.

**Name the clamp, or gcc distributes the add into all three arms.**
`CalcOutdoorsIntensity` ends `m_p30 = m_p30 + VIMIN(lim, VIMAX(-lim, d));`.
Written as one expression gcc if-converts the clamp into three branch-likely
arms each carrying its own `add.s`; assigning the clamp to a `float c` first
leaves the target's three `mov.s f2` and one `add.s` at the join
(31 wrong → 9).

**Two identical tails beat one shared tail when the target duplicates the
argument setup.**  The same function's `m_p28 = GetTime()` is reached from
both arms.  Written once after an if/else, gcc shares
`{store; move a0; jal}`; written out in BOTH arms it cross-jumps to
`{jal}` with the store as the delay-slot filler and `move a0,s1` duplicated,
which is the target (9 wrong → 3).

**An early `return -1` costs the entry-block constant.**  `GetNextColorBuffer`
guards `if (m_p444 == -1) return -1;` and then has a second `return -1`.
Nesting the body under `if (m_p444 != -1)` with ONE `return -1` at the bottom
lets gcc materialise `li v0,-1` in the ENTRY block, use it as the comparand
AND as the return value, and fill the guard's delay slot with the epilogue's
`lq ra` — byte-exact where the flat form is two words wrong.

**`(int)` on a `long` difference is three instructions of evidence.**
`(int)(GetTime() - m_p28)` gives `dsll32` / `dsra32` / `cvt.s.w`;
`(float)(long)` does not, and the cast is worth nine words.

**A store PAIR is emitted in reverse source order.**  `RasterActor` writes
`m_p444` then `m_p448`, and the object code stores 1096 then 1092.

**Phase 9's pointer lever, twice more.**  `RasterActor`'s distance needs
`VIVect3 &at` and `VIVect3 &eye` (the +0 component folds back onto the outer
base and +4/+8 go through the pointer); `CalcOutdoorsIntensity` needs
`VIVect3 &eye = cam.m_position`.  And a `VIColor32F &q = actor->m_p70` is what
puts `m_p70`'s w-store on its own base in both actor constructors.

## Shipped bugs

**`VIScene::ReleaseStatic` leaks a room's material palette when there is no
raster.**  The palette release and the `m_flags &= ~4` that records it are
BOTH inside `if (m_raster)`, so a scene constructed without a raster keeps bit
2 set on every room for ever and never retries.  This is the one word that
separated the function from byte-exact, so it is measured, not inferred.

**`CreateActor` and `StreamInStaticActor` write the SOURCE sprite's index into
the COPY's `VISprite::m_actor`.**  When `GetInstType` says 1 the scene copies
the sprite and then stores the ORIGINAL index at +0x38 of the copy — the field
every other site treats as the owning actor.

## Open

`Collide(sphere, filter, query)` 2/48 and `CalcOutdoorsIntensity` 3/107 are
both one register pair apart; `RemoveActor` 6/71, `SetTimeOfDay` 6/23,
`Clear` 8/198, `InsertActor` 8/60, `QueryVisibleActor` 14/89,
`Collide(sphere, filter)` 19/44, `CollideActor` 34/71, `Init` 51/225 and
`RemoveActorsFromZone` 54/107.

**Not attempted (15):** `InitTimeOfDay` (1,165 instructions), the three
`SetupTimeOfDay*` (714 / 499 / 336), `StreamOutStaticActor` (666),
`RasterActorBBox` (657), `RenderShadow` (559), `SetSoundTrack` (384),
`UpdateSoundEmitters` (349), `DebugResources` (286), `Render` (222),
`RenderDeferredList` (159), `SetSky()` (87), `SetSky(cam, i)` (70) and
`SetupTimeOfDay` (23).  `SetSky` needs the file static the two of them share
and `VIAtmosphere`'s entry points; the time-of-day group is a phase of its own.
