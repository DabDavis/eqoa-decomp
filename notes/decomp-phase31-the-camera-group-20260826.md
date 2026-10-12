# Phase 31 — the CAMERA group

**1427/1567 -> 1464/1610.**  Three classes and a filter, on the settled
toolchain (`ee-gcc2.95.3-107 -O2 -G0 -fno-exceptions`, SN `ps2eeas` out of the
2.95.3-114 package) with no re-tuning:

| class | written | byte-exact | first build |
|---|---|---|---|
| `VICamera`      (0x0041C7F8) |  8 of 10 |  8 | 4 |
| `VIFlyCamera`   (0x00441210) | 13 of 13 | 10 | 4 |
| `VIWalkCamera`  (0x00482020) | 20 of 21 | 19 | 8 |
| `VIWalkCameraFilter` (0x00484888) | 2 of 2 | 1 | 1 |

`VIWalkCamera` derives from `VIWalkControl`, whose 24 methods Phase 18 matched
and whose semantics `go-eqoa-vi/viwalk` had already checked against the
console — so the character half of this class was settled before a line was
written and every miss below is codegen.

## The layouts

### VICamera — 0x1F0 bytes

The two thin `operator()` overloads name the four scalars: each forwards
`(m_fov, m_aspect, m_near, <its own float>)` into `Init`, so `Init`'s own four
stores ARE those fields.

```
+0x000 float      m_near        +0x008 VIVect3 m_dir
+0x004 float      m_far         +0x014 float   m_fov      DEGREES
+0x018 float      m_aspect      +0x01C VIVect3 m_position
+0x030 VIMatrix44 m_view        right/up/fwd as COLUMNS, row 3 the dots
+0x070 VIMatrix44 m_proj        copied wholesale from Init's local
+0x0B0 VIMatrix44 m_world       right/up/fwd/pos as ROWS -- the inverse
+0x0F0 VIMatrix44 m_viewProj    m_view * m_proj
+0x130 VIFrustum  m_frustum     eight corners in WORLD space
```

`VICamera::Init` is the only caller of `VIRaster::Frustum` and it passes
`(-t*n*aspect, t*n*aspect, t*n, -t*n, n, f)` where `t = tanf(fov * pi/180 *
0.5)`, which is what makes the `aspect` argument the HORIZONTAL half-extent the
QOL widescreen patch scales.  **The y extents are handed over inverted** --
`bottom` positive and `top` negative.

### VIFlyCamera — 0x0FC + a VICameraEffect

Structurally `VIWalkControl`'s little brother and NOT derived from it: the same
six-vector rigid body (position, angles, velocity, angular velocity, force,
torque) integrated by the same `powf`-damped step, with no liquid, no gravity
and no locomotion state.  It carries the body TWICE, live at +0x28 and
committed at +0x88, 0x50 bytes apart.  `this` IS `&m_facing`, which is why
`Process`'s first `HPR` call passes a0 unchanged.

### VIWalkCamera — +0x1A0 onward

**The orbit body is a real nested TYPE.**  `InitCameraIntermediate` copies
+0x1A0..+0x1F0 forward onto +0x1F8..+0x248 in one run through a POINTER to each
half, so the +0 field folds back onto `this` (Phase 9) and every other one
comes off the pointer -- a flat member list produces displacements off `this`
for all of them and cannot be made to do otherwise.

```
struct State {                       // 0x50 bytes, alignment 8
    long         m_time;      +0x00
    unsigned int m_flags;     +0x08   1 snap-angle, 2/4 zoom, 8 snap-pitch
    VIVect3      m_orbit;     +0x0C   (angle, pitch, -)
    VIVect3      m_orbitVel;  +0x18
    VIVect3      m_orbitForce;+0x24
    float        m_dist;      +0x30
    float        m_distVel;   +0x34
    float        m_distForce; +0x38
    VIVect3      m_cameraPos; +0x3C
    float        m_snapAngle; +0x48
    float        m_snapPitch; +0x4C
};
State m_cur;   +0x1A0     float m_angleAccel; +0x1F0    float m_distAccel; +0x1F4
State m_last;  +0x1F8     int m_autoOrbit; +0x248 ... float m_minCollisionDist; +0x288
VICameraEffect m_effect;  +0x28C
```

`camera-system.md`'s constant table is confirmed -- 25.0 really is
`m_maxOrbitDistance` and 11.75 `m_defOrbitDistance` -- but its field NAMES are
off by the two-copy structure it did not see.

## What was learned

### An address NAMED before a call cannot be coalesced into the argument register

`VICamera::Update` ends with four calls whose arguments are `&m_view`,
`&m_proj` and `&m_world`.  Written as `m_viewProj.Mul(m_view, m_proj)` gcc
rematerialises `addiu a1,this,48` in the call's delay slot; the target has
`move a1,s1` with `addiu s1,this,48` scheduled 100 instructions earlier.  The
lever is a local reference declared at the TOP of the function --

```c
VIMatrix44 &view = m_view;
VIMatrix44 &wld  = m_world;
...
m_viewProj.Mul(view, m_proj);      // the call takes the reference
m_view.m[0].x = right.x;           // but the STORES stay on the member
```

-- the reference's live range then crosses the intervening call, so the address
lands in a callee-saved register, while every field store keeps its
displacement because it is written through the member name.  Writing the stores
through the reference too is the Phase 9 pointer lever and gives `0(s0)` /
`4(s0)`, which the target does NOT have.  161 wrong words to 0.

### An inline helper's temporary is a NEW temp each expansion

`VICamera::CalcFrustum` runs the same eight-statement block over eight NDC
corners.  As an inline helper the sret temporary of `v * invProj` is a fresh
temp per expansion: gcc gives them all the same STACK SLOT but a distinct
address pseudo, so `addiu a0,sp,16` is rematerialised eight times.  Written out
as eight braced blocks at function scope the eight `t`s share one pseudo and
the address is hoisted into a callee-saved register once -- the target's shape,
and byte-exact.  It is Phase 19's `gcse` merge, with the roles the other way
round.

### A store-to-load forward is not a re-read of the source

`VICamera::Update`'s three view translations read the COLUMN back out of
`m_view` rather than the vector it came from:

```c
m_view.m[3].x = -pos.x * m_view.m[0].x + -pos.y * m_view.m[1].x + ...
```

A store and a load of the same slot forward for free, where a second read of
`right.x` has to be re-loaded -- the stores above it may alias the caller's
vector.  Six instructions, and it is the whole gap.

### VIMIN/VIMAX are MACROS, so a call inside one is evaluated three times

`VIFlyCamera::Forward` clamps the magnitude of its input vector and then throws
the result away.  Written `VIMIN(1.0f, VIMAX(-1.0f, d.Magnitude()))` the call
appears THREE times in the expansion and gcc emits three `jal`s plus a fourth
copy of the following `Normalize` cross-jumped into the arms; bound to a local
first there is one of each and the function is byte-exact.  The clamped value
is never read -- a shipped no-op, in both `Forward` and `Hover`.

### A pointer variable must be declared where it is USED

`VIFlyCamera::UpdateCameraPhysics` needs Phase 9's pointer variable for the two
`v *= damp` runs (`swc1 f0,72(s0)` with `4(v1)` / `8(v1)`), but a pointer held
from the top of the function is live across the `fmodf` call in the middle and
lands in a callee-saved register, where the target rematerialises the address
after it.  Declared inside a brace pair at each site, both come out right.

### One damping local, not two

The same function's two `powf` results go into ONE variable in the source:
reused, its live range crosses the second call, so both definitions need
`mov.s f21,f0`.  Two separate locals leave the first result in `f0` and the
function is two instructions short.

### assign-then-modify-in-place, again

`VIWalkCamera::SetOrbitAngle` is byte-exact only as

```c
p = m_position;
p.x = p.x + dir.x * m_cur.m_dist;
p.y = p.y + m_targetHeight + dir.y * m_cur.m_dist + m_field104;
p.z = p.z + dir.z * m_cur.m_dist;
```

`VIVect3::operator=` takes a reference, which is where the `addiu v0,this,176`
comes from; computing each component from `m_position.x` directly gives
displacements and 27 wrong words.  Phase 9's style rule, and the pointer
VARIABLE form is worse than either (67 wrong).

### The flag words are touched 32 bits at a time, unsigned

`VIWalkCamera::SetCameraMode` does `lw`/`ori`/`sw` on +0xF8 -- a field Phase 18
proved is a `long` -- and its clear mask is `lui 0xffff` + `ori 0xfeff`, ONE
constant shared by both flag words.  A signed `~0x100` is a single `addiu -257`.
So the site casts: `*(unsigned int *)&m_flags |= 0x100;`.  `VIWalkCamera::State`
carries `m_flags` as `unsigned` for the same reason.

### The `lui` order and the store order disagree, and only one of them is source

`VIWalkCamera::Init` stamps eighteen constants.  Phase 16's "last store of each
distinct constant is hoisted to the front" predicts the store sequence exactly
from one source order; Phase 21's "the `lui`s come out in SOURCE order"
predicts the materialisation sequence exactly from a DIFFERENT one, and the two
cannot both be source order (the first wants 11.75 before 0.5, the second wants
0.5 before 11.75).  The `lui` order is kept -- 29 wrong words against 40 -- and
the function is open.  **The 1.68 in that block is `1.68f` (0x3FD70A3D), not
1.67968750f**: a bare `lui 0x3fd7` with no `ori` is the tell.

## Shipped oddities

* **`VIFlyCamera::Forward` and `::Hover` clamp a magnitude and drop it.**  The
  value is computed, clamped to +-1 and never read; the `Normalize` that
  follows ignores it.
* **`VIWalkCamera::UpdateCameraPhysics` damps the orbit pitch by ZERO.**  The
  damping is a `VIVect3` whose x alone is ever written, and the angular
  velocity is multiplied by it component-wise, so `m_orbitVel.y` and `.z` are
  zeroed on every step.
* **`VIWalkCamera::OrbitIn` declares OrbitRight's two stack vectors** -- both
  constructed, both zeroed, neither read.
* **`VIFlyCamera::Init` never initialises the committed velocity or force**
  (+0xA8, +0xC0) and then copies them forward into the live body.
* **`VIWalkCamera::SetOrbitAngle` calls `fmodf` twice on the same argument**,
  once per copy of the orbit angle.
* **The zoom snap is asymmetric**: the `& 2` arm steps the distance and clears
  its flag without clamping, the `& 4` arm clamps to `m_minCollisionDist`.
* **`VIWalkCameraFilter` rejects every VICSprite** (sprite type 5) as well as
  the character's own actor, so the third-person camera swings through players
  and NPCs and stops only on world geometry.

## Open

* `VIFlyCamera::PitchDown` 22/62 -- the target does NOT fold `axis.y * rate`
  where its own `TurnLeft` (same source shape, axis `(1,0,0)`) does.  Five
  spellings swept; the three-float ctor loses the double-zero instead.
* `VIFlyCamera::Init(VICamera &, VIScene &)` 26/45 -- same instructions, same
  length; the target keeps six loads live where ours serialises them through
  one register.  The standing phenomenon.
* `VIFlyCamera::UpdateCameraPhysics` 20/192 -- one instruction: the target
  materialises `&m_angles.y` into a register for the pitch clamp's store.
* `VIWalkCamera::UpdateCameraPhysics` 52/230 -- one instruction, a `mov.s`
  copying the orbit angle before the wrap subtraction overwrites its register.
* `VIWalkCamera::Init(pos, angles, cam, scene)` 29/82 -- the `lui`/store order
  conflict above.
* `VIWalkCameraFilter::FilterActor` 2/21 -- register allocation, `v0` vs `v1`.

`FindOrbitAngle` (172) and `FindBestOrbitAngle` (190) are byte-exact -- the
eight-angle camera sweep, at the caller's angle and +-45/90/135/180 off it,
the first taking the first clear angle and the second the one whose sweep got
furthest.  `FindBestOrbitAngle` matched on the FIRST build; `FindOrbitAngle`
needed one line -- **the step-up offset goes on FROM first and TO second**, the
other order being the same 172 instructions with two registers renamed.  Not
attempted: `VICamera::CalcPick` (188) and `::CalcPickFrustum` (343), both with
VU0, and `VIWalkCamera::MoveCamera` (355).

## Harness

Two new `SPAN_CAP` entries.  `VIFlyCamera`'s last named function ends at
0x004422F8 and the next symbol is `VIFrustum`'s, 2.5 KB later; `VIWalkCamera`'s
ends at 0x00483EB8 and the next is `__tf18VIWalkCameraFilter`, 2.5 KB later.
Both gaps are the TUs' unnamed statics, and without the caps
`FindBestOrbitAngle` reads as 804 instructions instead of 188.
