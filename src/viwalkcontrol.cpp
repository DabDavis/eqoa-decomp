#include "vi.h"

VIVect3 VIWalkControl::Gravity(0.0f, -19.6f, 0.0f);
VIVect3 VIWalkControl::WGravity(0.0f, -1.0f, 0.0f);

// The flag tests that come out `ld`: one-bit fields OR'd together, which fold
// merges into a single masked load of the 8-aligned word.  A test of the WORD
// (`m_flags & mask`) is the `lw` form; see VIWalkPhysics in vi.h.
#define IN_MUD(p)    ((p).m_bits.inMud || (p).m_bits.atMudSurface)
#define IN_LIQUID(p) ((p).m_bits.inWater || (p).m_bits.atWaterSurface ||      \
                      (p).m_bits.inLava || (p).m_bits.atLavaSurface ||        \
                      (p).m_bits.inSlime || (p).m_bits.atSlimeSurface ||      \
                      (p).m_bits.inMud || (p).m_bits.atMudSurface)
#define ON_FLOOR_OR_LIQUID(p) ((p).m_bits.onFloor || (p).m_bits.b1 ||         \
                               IN_LIQUID(p))

// SUPPORT 0x00484930 .. 0x00487BB0.  VIWalkControl, tier 2.
//
// Every function here has a console-checked semantic twin in
// go-eqoa-vi/viwalk, so a mismatch below is CODEGEN and nothing else -- the
// Phase 11/13 position, and the reason this class is worth doing before the
// ones whose behaviour still has to be read out of the disassembly.

// The vector helpers.  Each one is an inline taking its vectors BY REFERENCE,
// and the guest's aliasing is decided by WHAT the caller hands them:
//
//   a member (`m_right`) or a stack local -- the argument is an expression, so
//     the inlined parameter is a fresh pseudo with NO pointer flag.  A store
//     or load through it at +4/+8 has no known base (alias.c find_base_term),
//     so gcc must assume it can touch any addressable stack local: cse
//     forgets the local's known components, flow keeps stores to it it would
//     otherwise call dead, and sched keeps loads behind stores to it.  At +0
//     the register's own base value is used, so +0 aliases nothing.
//   a named `VIVect3 *` -- the parameter IS that register, pointer-flagged,
//     with a known base, and none of the above happens.
//
// All four of UpdateDirections, ViewRelativeMove, UpdateLocomotion and
// UpdatePhysics closed on handing these the member instead of a named pointer.
static inline void Cross(VIVect3 &out, const VIVect3 &a, const VIVect3 &b)
{
    out.x = a.y * b.z - a.z * b.y;
    out.y = -a.x * b.z + a.z * b.x;
    out.z = a.x * b.y - a.y * b.x;
}

// By value: the result is built in a caller temporary with the three-float
// ctor (no zeroing) and then copied by VIVect3::operator=.
static inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline float Dot(const VIVect3 &a, const VIVect3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// The tail every input method shares: `accum += dir * k`, one component at a
// time.  Both vectors by reference: with `accum` a member, its store at +4
// is the unflagged-pseudo store above and the guest reloads dir.z after it.
static inline void AddVec(VIVect3 &accum, const VIVect3 &dir, float k)
{
    accum.x += dir.x * k;
    accum.y += dir.y * k;
    accum.z += dir.z * k;
}

VIWalkControl::VIWalkControl()
{
}

VIWalkControl::~VIWalkControl()
{
}

// 0x00484A90.
int VIWalkControl::Init(const VIVect3 &pos, const VIVect3 &angles, VIScene &scene)
{
    InitParms();
    // Written committed-first: a store pair comes out in reverse order.
    m_committed.m_flags = 1;
    m_phys.m_flags = 1;
    SetLocation(pos, angles, scene);
    return 0;
}

// 0x00484B00.  Fill the committed state, keep its 0x100 bit, and copy it
// onto the working one.
int VIWalkControl::SetLocation(const VIVect3 &pos, const VIVect3 &angles,
                               VIScene &scene)
{
    unsigned int keep = m_committed.m_flags & 0x100;
    m_committed.m_time = scene.GetTime();
    m_committed.m_position = pos;
    m_committed.m_velocity.Zero();
    m_committed.m_force.Zero();
    m_committed.m_angles = angles;
    m_committed.m_angles.y = 0.0f;
    m_committed.m_angles.z = 0.0f;
    m_committed.m_angularVel.Zero();
    m_committed.m_torque.Zero();
    m_committed.m_flags = 1;
    m_committed.m_locoState = 0;
    m_committed.m_heightOffset = 0.0f;
    m_committed.m_floor = pos.y;
    m_committed.m_targetHeading = m_committed.m_angles.x;
    m_committed.m_landscapeNormal.Set(0.0f, 1.0f, 0.0f);
    m_committed.m_flags |= keep;
    m_phys = m_committed;
    InitIntermediate();
    return 0;
}

// 0x00485DA8.  Take a whole state from outside, stamp it with the scene's
// clock and, when it says the body is on a floor, snap it down onto one.
int VIWalkControl::SetUpdatePhysics(VIWalkPhysics &p, VIScene &scene)
{
    VIVect3 from;
    VIVect3 to;

    m_committed = p;
    m_committed.m_time = scene.GetTime();
    m_phys = m_committed;
    InitIntermediate();
    if (m_committed.m_flags & 1)
    {
        from = m_committed.m_position;
        from.y += m_parm38;
        if (CalcFloor(scene, from, to) < 0)
            return -1;
        m_committed.m_position = to;
        m_phys.m_position = m_committed.m_position;
    }
    return 0;
}

// 0x00484DB8.  Step the body to the scene's clock in fixed ticks.  A gap
// longer than m_tickAltMS is skipped, not simulated: the clock jumps to within
// one long tick of now and only that last stretch is integrated.
int VIWalkControl::Process(int flags, VIScene &scene)
{
    unsigned int old = m_phys.m_flags;
    long dt = scene.GetTime() - m_committed.m_time;
    int tick;
    if (flags)
        tick = m_tickAltMS;
    else
        tick = m_tickMS;
    while (dt > 0)
    {
        long step;
        if (m_tickAltMS < dt)
        {
            step = dt - m_tickAltMS;
            m_phys.m_time = m_committed.m_time + step;
        }
        else
        {
            step = VIMIN(tick, dt);
            m_phys.m_time = m_committed.m_time + step;
            float t = (float)(int)step * 0.001f;
            UpdatePhysics(t);
            AutoPitch(t);
            Move(flags, scene);
            UpdateDirections();
            UpdateLocomotion(t);
        }
        dt -= step;
        m_committed = m_phys;
    }
    if (flags == 0)
        ClearForces();
    m_parm118 = 0;
    if (!(old & 1) && (m_phys.m_flags & 1) != 0 &&
        (m_phys.m_flags & 0x854) == 0 && (old & 0x854) == 0)
        m_parm118 = 16;
    return 0;
}

// 0x00485EB0.  Hand the body's state to the character sprite: the
// locomotion animation (7 while m_parm118 has 0x10), and a blend to a centre
// transform -- pitched with the body when submerged, sunk by the part of its
// height the sprite's scale leaves out when at a surface.
int VIWalkControl::UpdateSprite(VICSprite &sprite, VIScene &scene)
{
    // The scale read into a local FIRST: that load order is what puts the
    // product in m_parm38's register.
    float s = sprite.m_defaultScale;
    VIVect3 center(0.0f, m_parm38 * s, 0.0f);
    VIVect3 hpr;
    VIVect3 trans;

    if ((m_parm118 & 0x10) == 0)
        sprite.SetLocomotion((VICSpriteAnimID)m_committed.m_locoState, scene);
    else
        sprite.SetLocomotion((VICSpriteAnimID)7, scene);

    if ((m_phys.m_flags & 0x854) != 0 && (m_phys.m_flags & 0x10A8) == 0)
    {
        hpr.Set(0.0f, m_phys.m_angles.y, 0.0f);
        trans.Zero();
    }
    else
    {
        if (m_phys.m_flags & 0x10A8)
            trans.Set(0.0f, m_parm38 - m_parm38 * sprite.m_defaultScale, 0.0f);
        else
            trans.Zero();
        hpr.Zero();
    }
    sprite.BlendToCenterTransform(center, hpr, 1.0f, trans, 400, scene);
    return 0;
}

// 0x00484F58.  `sw $zero` six times, not `swc1`, so a component holding -0.0
// comes back +0.0.  Zero() per member rather than six stores on `this`: the
// target gives each vector its own base register (Phase 13).
int VIWalkControl::ClearForces()
{
    m_phys.m_force.Zero();
    m_phys.m_torque.Zero();
    return 0;
}

// 0x00485D10.  `(flags & 0x0C) == 0x04` -- ONE test, not two, and the guest
// if-converts it to xori + movn.
int VIWalkControl::IsUnderWater()
{
    if (m_phys.m_bits.inWater && !m_phys.m_bits.atWaterSurface)
        return 1;
    return 0;
}

// 0x00485D28.  Submerged in something and at the surface of nothing.
int VIWalkControl::IsUnderLiquid()
{
    if ((m_phys.m_flags & 0x854) != 0 && (m_phys.m_flags & 0x10A8) == 0)
        return 1;
    return 0;
}

// 0x00485D50.
int VIWalkControl::IsAtLiquidSurface()
{
    return (m_phys.m_flags & 0x10A8) != 0;
}

// 0x00485D60.
int VIWalkControl::IsInLava()
{
    if (m_phys.m_bits.inLava || m_phys.m_bits.atLavaSurface)
        return 1;
    return 0;
}

// 0x00485D78.
int VIWalkControl::IsInSlime()
{
    if (m_phys.m_bits.inSlime || m_phys.m_bits.atSlimeSurface)
        return 1;
    return 0;
}

// 0x00485D90.
int VIWalkControl::IsInMud()
{
    if (IN_MUD(m_phys))
        return 1;
    return 0;
}

// 0x00487910.  93 instructions, every one a constant store: no arguments, no
// reads.  Five values are stored twice or three times from the same register.
int VIWalkControl::InitParms()
{
    m_tickMS = 32;
    m_tickAltMS = 200;
    m_speedThreshold = 0.5f;
    m_walkSpeed = 3.0f;
    m_runSpeed = 8.833330f;
    m_parm14 = 4.416665f;
    m_turnRate = 1.5707964f;
    m_pitchRate = 0.7853982f;
    m_swimSpeed = 3.0f;
    m_parm24 = 0.3f;
    m_parm28 = 0.6f;
    m_parm2C = 1.0471976f;
    m_parm30 = 1.8f;
    m_parm38 = 1.2f;
    m_parm34 = 0.6f;
    m_parm3C = 0.001f;
    m_moveDamping = 0.02f;
    m_parm44 = 0.8f;
    m_swimDamping = 0.02f;
    m_turnDamping = 0.01f;
    m_pitchDamping = 0.01f;
    m_parm54 = 0.1f;
    m_moveDeadzone = 0.01f;
    m_runThreshold = 0.7f;
    m_parm60 = 1;
    m_parm64 = -1;
    m_minPitch = -1.5607964f;
    m_maxPitch = 1.5607964f;
    m_autoPitchRate = 0.5235988f;
    return 0;
}

// 0x00485A68.  Angular thrust about the heading axis, which is Torque.x.  NO
// flag gate: a character falling through the air can still turn.
//
// The (1,0,0) axis is built on the stack and multiplied through in full -- gcc
// folds `a * 1.0f` down to `a` and CANNOT fold `a * 0.0f`, which is why the x
// component has no multiply and the other two share one.
int VIWalkControl::TurnLeft(float t)
{
    VIVect3 dir;
    VIVect3 axis;
    float   a;

    t = VIMIN(1.0f, VIMAX(-1.0f, t));
    a = m_turnAccel;
    axis.x = 1.0f;
    axis.y = 0.0f;
    axis.z = 0.0f;
    dir.x += a * axis.x;
    dir.y += a * axis.y;
    dir.z += a * axis.z;
    AddVec(m_phys.m_torque, dir, t);
    return 0;
}

// 0x004858C0.  Vertical thrust, gated on being in a liquid at all.  The
// direction is the constant (0,1,0) built on the stack and multiplied through
// in full: gcc folds the y term's `* 1.0f` away and cannot fold the other two.
int VIWalkControl::SwimUp(float t)
{
    if (IN_LIQUID(m_phys))
    {
        float k;

        t = VIMIN(1.0f, VIMAX(-1.0f, t));
        k = m_swimAccel * t;
        VIVect3 up(0.0f, 1.0f, 0.0f);
        AddVec(m_phys.m_force, up, k);
    }
    return 0;
}

// 0x00485B50.  Two different functions behind one name, and the flag word it
// reads is +0x180 -- NOT the +0xF8 every other liquid test in this class uses.
//
// At a liquid surface it is a SWIM: the swim acceleration, the direction
// (0,-1,0), and the result added to Force.  Everywhere else it is a real
// pitch: the pitch acceleration, (0,+1,0), and Torque.
//
// THE TWO BRANCHES DO NOT CLAMP TO THE SAME RANGE.  The pitch branch clamps to
// [-1,1] against a materialised -1.0; the swim branch clamps to [0,1], and the
// zero it compares against is `dir.x` -- the stack slot the constructor has
// already zeroed, which gcc reuses rather than materialising a second constant.
int VIWalkControl::PitchDown(float t)
{
    VIVect3 dir;
    VIVect3 axis;

    if ((m_committed.m_flags & 0x10A8) == 0)
    {
        t = VIMIN(1.0f, VIMAX(-1.0f, t));
        axis.Set(0.0f, 1.0f, 0.0f);
        AddVec(dir, axis, m_pitchAccel);
        AddVec(m_phys.m_torque, dir, t);
    }
    else
    {
        t = VIMIN(1.0f, VIMAX(0.0f, t));
        axis.Set(0.0f, -1.0f, 0.0f);
        AddVec(dir, axis, m_swimAccel);
        AddVec(m_phys.m_force, dir, t);
    }
    return 0;
}

// 0x00487028.  The four-vector basis every input method steers along, rebuilt
// from the Angles triple.
//
//   Forward = HPR(heading,   0, roll)   -- the heading only, pitch-free BY
//                                          CONSTRUCTION, not by projection
//   Facing  = HPR(heading, pitch, roll)
//   Right   = normalize(Forward x up)   -- built from Forward, so strafing
//                                          stays horizontal at any pitch
//   Up      = Facing x Right            -- NOT normalized
//
// The cross products are written out term by term because the world up vector
// is (0,1,0) and the guest's operand order is load-bearing on the sign of
// zero: `(-x)*0 + z*0` is not `z*0 - x*0` when the signs differ.
//
// Cross is handed the MEMBER, not a named `VIVect3 *`: the inline's `out` is
// then a fresh pseudo with no pointer flag, and a store through it at a
// nonzero offset has no known base -- so storing `right.y` makes cse forget
// the stack-held `up`, and `right.z` reloads up.x and up.y (0 and 1) where a
// named pointer lets it fold them.  `right.x`, at offset 0, kills nothing.
int VIWalkControl::UpdateDirections()
{
    m_forward.HPR(m_phys.m_angles.x, 0.0f, m_phys.m_angles.z);
    m_facing.HPR(m_phys.m_angles.x, m_phys.m_angles.y, m_phys.m_angles.z);

    VIVect3 up(0.0f, 1.0f, 0.0f);
    Cross(m_right, m_forward, up);
    m_right.Normalize();

    Cross(m_up, m_facing, m_right);
    return 0;
}

// 0x00484C30.  Install a pitch range, then bring the current pitch inside it.
// Both arguments are clamped to +-1.5607964 (89.43 degrees, NOT the pi/2
// movement-system.md records) first, so the range can never be opened past it
// however the function is called.
int VIWalkControl::SetPitchRange(float lo, float hi)
{
    float *p;

    m_minPitch = VIMIN(1.5607964f, VIMAX(-1.5607964f, lo));
    m_maxPitch = VIMIN(1.5607964f, VIMAX(-1.5607964f, hi));
    // ONE reused `float *`, written through and READ THROUGH THE MEMBER: the
    // store is `swc1 f0,0(v0)` off a computed address, which a plain member
    // assignment cannot give, while reading `*p` instead of the member lets
    // gcc CSE the two loads and collapses the macro's double evaluation.
    p = &m_phys.m_angles.y;
    *p = VIMIN(m_maxPitch, VIMAX(m_minPitch, m_phys.m_angles.y));
    p = &m_committed.m_angles.y;
    *p = VIMIN(m_maxPitch, VIMAX(m_minPitch, m_committed.m_angles.y));
    return 0;
}

// 0x00485980.  Record the target, wrap it to within pi of the current heading,
// then nudge with a FULL-STRENGTH TurnLeft in whichever direction closes the
// gap -- a bang-bang controller, smoothed by UpdatePhysics's damping and
// stopped by its overshoot detector, not by anything here.
//
// The value STORED at +0x108 is the RAW argument, before the wrap, so the
// recorded target and the one actually turned toward differ by 2pi whenever
// the wrap fires.
int VIWalkControl::SetTargetHeading(float h)
{
    float d;

    m_phys.m_targetHeading = h;
    d = h - m_phys.m_angles.x;
    if (3.14159274f < d)
        h = h - 6.28318548f;
    else if (d < -3.14159274f)
        h = h + 6.28318548f;
    if (m_phys.m_angles.x < h)
        TurnLeft(1.0f);
    if (h < m_phys.m_angles.x)
        TurnLeft(-1.0f);
    return 0;
}

// 0x00487820.  Step the pitch toward whatever the current medium says it
// should be, at AutoPitchRate radians a second.
//
// THE STEP IS TAKEN FROM PitchAlt AND STORED INTO Angles.y: 0x004878DC loads
// +0x148 and 0x00487904 stores +0xC0.  The snap path writes the target to
// +0xC0 as well, so the two fields agree only just after a snap; every stepping
// frame writes a pitch derived from a field this function never updates.  It is
// reproduced exactly, and it looks like a guest bug rather than a decision.
int VIWalkControl::AutoPitch(float dt)
{
    float target;
    float delta;
    float step;

    if ((m_phys.m_flags & 0x10A8) != 0)
        target = -0.08726646f;
    else if ((m_phys.m_flags & 0x854) != 0)
        target = m_phys.m_angles.y;
    else
        target = VIMIN(m_maxPitch, VIMAX(m_minPitch, m_phys.m_angles.y));

    delta = target - m_phys.m_angles.y;
    step = m_autoPitchRate * dt;
    if (0.0f <= delta)
    {
        if (delta <= step)
            m_phys.m_angles.y = target;
        else
            m_phys.m_angles.y = m_committed.m_angles.y + step;
    }
    else
    {
        if (step <= delta)
            m_phys.m_angles.y = target;
        else
            m_phys.m_angles.y = m_committed.m_angles.y - step;
    }
    return 0;
}

// 0x00484F80.  x is forward and y is strafe.  The gate is "on a floor OR in
// any liquid", so a character in mid-air with neither cannot steer -- that is
// the whole of the engine's air control.
//
// The heading it walks along is Facing in liquid and Forward out of it:
// swimming goes where you are LOOKING, walking goes where you are FACING with
// the pitch discarded.  The strafe half always uses Right.
int VIWalkControl::Walk(float x, float y)
{
    VIVect3 v;
    VIVect3 unused;

    if (ON_FLOOR_OR_LIQUID(m_phys))
    {
        v.x = VIMIN(1.0f, VIMAX(-1.0f, x));
        v.y = VIMIN(1.0f, VIMAX(-1.0f, y));
        v.z = 0.0f;
        if (1.0f < v.Magnitude())
            v.Normalize();
        // Two calls and not one through a selected pointer: a pointer picked
        // with `dir = cond ? &a : &b` if-converts to `movn`, where the guest
        // BRANCHES and duplicates the whole call into both arms.
        if (!IN_LIQUID(m_phys))
            AddVec(m_phys.m_force, m_forward, m_walkAccel * v.x);
        else
            AddVec(m_phys.m_force, m_facing, m_walkAccel * v.x);
        AddVec(m_phys.m_force, m_right, m_walkAccel * v.y);
    }
    return 0;
}

// 0x004853F8.  The stick itself, deadzone and all.  Under MoveDeadzone (0.01)
// it does NOTHING -- not a zero force, no force at all, which matters because
// the accumulator is not cleared here.  Over it the vector is normalized IN
// PLACE, so the run/walk decision is made on the RAW magnitude while the thrust
// that reaches Run or Walk is a unit vector.
int VIWalkControl::Move(float x, float y)
{
    VIVect2 v(x, y);
    float   m;

    m = v.Magnitude();
    if (m_moveDeadzone < m)
    {
        v.Normalize();
        if (m_runThreshold < m)
            Run(v.x, v.y);
        else
            Walk(v.x, v.y);
    }
    return 0;
}

// 0x00485150.  Walk's shape with the acceleration chosen per component, and
// the choice is the interesting part:
//
//  * The threshold test is against the RAW x, before the clamp and before the
//    normalize.  Move only ever calls this with a normalized vector, so the
//    distinction is invisible from there -- but a direct caller passing 5.0
//    gets the run branch with a clamped 1.0 of thrust.
//  * Above the threshold the FORWARD component runs unless mud (in liquid) or
//    the slow-run bit (out of it) is set, in which case it walks.
//  * The STRAFE component runs regardless, with no test at all -- so a
//    character running through mud strafes at run speed and advances at walk
//    speed.
//  * Below the threshold both components take the walk acceleration and the
//    function is Walk with a different name.
int VIWalkControl::Run(float x, float y)
{
    VIVect3 v;
    VIVect3 unused;

    if (ON_FLOOR_OR_LIQUID(m_phys))
    {
        v.x = VIMIN(1.0f, VIMAX(-1.0f, x));
        v.y = VIMIN(1.0f, VIMAX(-1.0f, y));
        v.z = 0.0f;
        if (1.0f < v.Magnitude())
            v.Normalize();
        if (m_runThreshold < x)
        {
            if (!IN_LIQUID(m_phys))
            {
                if ((m_phys.m_flags & 0x2000) == 0)
                    AddVec(m_phys.m_force, m_forward, m_runAccel * v.x);
                else
                    AddVec(m_phys.m_force, m_forward, m_walkAccel * v.x);
            }
            else if (!IN_MUD(m_phys))
                AddVec(m_phys.m_force, m_facing, m_runAccel * v.x);
            else
                AddVec(m_phys.m_force, m_facing, m_walkAccel * v.x);
            AddVec(m_phys.m_force, m_right, m_runAccel * v.y);
        }
        else
        {
            if (!IN_LIQUID(m_phys))
                AddVec(m_phys.m_force, m_forward, m_walkAccel * v.x);
            else
                AddVec(m_phys.m_force, m_facing, m_walkAccel * v.x);
            AddVec(m_phys.m_force, m_right, m_walkAccel * v.y);
        }
    }
    return 0;
}

// 0x00487A88.  Turn the parameter block's five speeds and four dampings into
// the five ACCELERATIONS the input methods add to the force accumulators, then
// rebuild the direction basis.  Each is
//
//   accel = (speed - speed*d) / (d*dt)      where d = damping^dt
//
// so it is the acceleration that reaches `speed` asymptotically under that
// damping in one tick.
//
// IT CALLS powf FIVE TIMES AND USES FOUR RESULTS.  The second call has its
// result in f0 overwritten by the third before anything reads it, and gcc
// could not delete it: powf sets errno, so the call is not pure.  The dampings
// are also NOT read in field order -- +0x40, +0x44 (discarded), +0x4C, +0x48,
// +0x50 -- so turn and swim come the wrong way round against the layout.
int VIWalkControl::InitIntermediate()
{
    float dt;
    float moveD, turnD, swimD, pitchD;

    dt = (float)m_tickMS * 0.001f;
    moveD = powf(m_moveDamping, dt);
    powf(m_parm44, dt);
    turnD = powf(m_turnDamping, dt);
    swimD = powf(m_swimDamping, dt);
    pitchD = powf(m_pitchDamping, dt);

    m_walkAccel = (m_walkSpeed - m_walkSpeed * moveD) / (moveD * dt);
    m_runAccel = (m_runSpeed - m_runSpeed * moveD) / (moveD * dt);
    m_turnAccel = (m_turnRate - m_turnRate * turnD) / (turnD * dt);
    m_swimAccel = (m_swimSpeed - m_swimSpeed * swimD) / (swimD * dt);
    m_pitchAccel = (m_pitchRate - m_pitchRate * pitchD) / (pitchD * dt);

    UpdateDirections();
    m_parm118 = 0;
    return 0;
}

// 0x00485FE8.  The lean that puts a character's feet on a slope.  The argument
// is IN/OUT and asymmetric: it arrives carrying a heading in x and leaves with
// a pitch in y and a roll in z.  x is never written, and the function is const,
// so the object is not touched at all.
//
//   fwd = HPR(heading, 0, 0)   the direction the character faces, flat
//   r   = normalize(n x fwd)   across the slope, level with it
//   u   = r x n                up the slope, in the character's plane
//   pitch = -asin(u.y)         how steeply that plane climbs
//   roll  =  asin(r.y)         how far the across-slope axis tips
//
// THE ASIN IS AN ATAN2, not a library asin: the guest computes
// `atan2f(s, sqrt(1 - s*s))`, so it inherits atan2's quadrant handling and its
// exact rounding, and |s| > 1 gives a NaN square root rather than a clamp.
// `u` is not a fourth local -- it is written back over `fwd`, which is why the
// frame holds three vectors and not four.
int VIWalkControl::CalcPitchRollToLandscape(VIVect3 &v) const
{
    VIVect3 n;
    VIVect3 fwd;
    VIVect3 r;

    if (m_committed.m_flags & 0x400)
    {
        n = m_committed.m_landscapeNormal;
        fwd.HPR(v.x, 0.0f, 0.0f);
        r.x = n.y * fwd.z - n.z * fwd.y;
        r.y = -n.x * fwd.z + n.z * fwd.x;
        r.z = n.x * fwd.y - n.y * fwd.x;
        r.Normalize();
        fwd.x = r.y * n.z - r.z * n.y;
        fwd.y = -r.x * n.z + r.z * n.x;
        fwd.z = r.x * n.y - r.y * n.x;
        v.y = -atan2f(fwd.y, sqrtf(1.0f - fwd.y * fwd.y));
        v.z = atan2f(r.y, sqrtf(1.0f - r.y * r.y));
    }
    else
    {
        v.y = 0.0f;
        v.z = 0.0f;
    }
    return 0;
}

// 0x00487178.  The one function in this class that MEASURES rather than
// commands: it takes the two position samples the integrator left behind,
// projects the difference onto the character's own forward and right axes,
// divides by dt and classifies.  So it reports what the physics DID, not what
// the player asked for -- a character pushed backwards by a slope gets the
// backing-up state without ever having pressed back.
//
// The chain's ORDER is the whole function; the first match wins.  Three things
// worth pinning: the run threshold is NOT RunSpeed but one tenth of the way
// from the walk speed to the run speed (3.58 with the stock block); MoveMode
// gates only the FIRST of the two strafe rules, so they are two different rules
// and not a refinement of each other; and the turning states read AngularVel,
// not Torque, so a commanded turn that has not moved the character does not
// animate.
int VIWalkControl::UpdateLocomotion(float dt)
{
    VIVect3  d;
    float    vF, vR;

    // The difference and both dots go through inline helpers handed the
    // MEMBERS: their reference parameters are unflagged pseudos, so every
    // read at +4/+8 may alias the stack.  That keeps d's zeroing ctor (the
    // reads sit between it and the copy-in, so flow cannot call the zeroes
    // dead) and holds fwd.y/z and right.* behind d's stores in the schedule.
    // The by-value `-` builds its result in a temporary at sp+16, which is
    // the second vector in the frame -- not a second local.
    d = m_phys.m_position - m_committed.m_position;
    vF = Dot(d, m_forward) / dt;
    vR = Dot(d, m_right) / dt;

    // State 6 is the ELSE of an outer test, not the first link of the chain:
    // the guest's `beqz` to the shared store with `li v0,6` in its slot is an
    // out-of-line arm folded by reorg.  As the first `if` it falls through and
    // reorg fills the `bnez` from its target instead.  The swim test is one
    // `||` for the same reason, and `fabsf(vR) > fabsf(vF)` is written that
    // way round because the guest takes |vR| first.
    if (!IN_LIQUID(m_phys))
    {
        if (m_phys.m_flags & 1)
        {
            if (m_walkSpeed + (m_runSpeed - m_walkSpeed) * 0.1f < vF)
                m_phys.m_locoState = 3;
            else if (m_speedThreshold < vR && fabsf(vR) > fabsf(vF) &&
                     m_committed.m_locoState != 1 && m_committed.m_locoState != 2)
                m_phys.m_locoState = 11;
            else if (vR < -m_speedThreshold && fabsf(vR) > fabsf(vF) &&
                     m_committed.m_locoState != 1 && m_committed.m_locoState != 2)
                m_phys.m_locoState = 10;
            else if (m_speedThreshold < vR && fabsf(vF) * 1.5f < fabsf(vR))
                m_phys.m_locoState = 11;
            else if (vR < -m_speedThreshold && fabsf(vF) * 1.5f < fabsf(vR))
                m_phys.m_locoState = 10;
            else if (m_speedThreshold < vF)
                m_phys.m_locoState = 1;
            else if (vF < -m_speedThreshold)
                m_phys.m_locoState = 2;
            else if (m_turnRate * 0.5f < m_phys.m_angularVel.x)
                m_phys.m_locoState = 4;
            else if (m_phys.m_angularVel.x < -(m_turnRate * 0.5f))
                m_phys.m_locoState = 5;
            else
                m_phys.m_locoState = 0;
        }
        else
            m_phys.m_locoState = 6;
    }
    else
    {
        // The liquid branch: QUARTER turn rate, not half, and the PITCH rate
        // counts as well as the heading rate -- a swimmer looking up and down
        // is animating.  The last comparison is on |vR| and its two outcomes
        // differ: fast sideways drift is state 12, not 13.
        if (fabsf(m_phys.m_angularVel.y) > m_turnRate * 0.25f ||
            m_turnRate * 0.25f < fabsf(m_phys.m_angularVel.x) ||
            m_speedThreshold < vF || m_speedThreshold < fabsf(vR))
            m_phys.m_locoState = 12;
        else
            m_phys.m_locoState = 13;
    }
    return 0;
}

// 0x00486198.  The integrator: the two accumulators the input methods filled
// become a position and a set of angles, damped, with gravity added and the
// heading wrapped.  No scene and no collision -- CalcFit and CalcFloor are the
// caller's job.  The shape is a damped semi-implicit Euler step:
//
//   d    = powf(damping, dt)
//   pos += force * (dt*dt*d)  +  vel * (dt*d)
//   vel  = (vel + force*dt) * d
//
// THE LINEAR AND ANGULAR HALVES USE DIFFERENT DAMPINGS, and the linear one is
// CHOSEN AT RUNTIME: SwimDamping in any liquid, MoveDamping on a floor, and
// Parm44 (0.8, forty times looser) in mid-air.  So Parm44 is the AIR DRAG, and
// it is the one field InitIntermediate computes a powf of and throws away.
// The angular half always uses TurnDamping.
//
// The force it integrates is a STACK COPY: gravity is added to the copy, so
// +0xE0 is left exactly as the input methods wrote it.  The velocity read for
// the position update is the OLD one.  Roll is unconditionally zeroed on every
// path.
int VIWalkControl::UpdatePhysics(float dt)
{
    VIVect3  f;
    VIVect3  tq;
    float    damp, dt2;
    float    wrapped;

    // The +0x18C -> +0x104 decay.  The unconditional copy on the first line is
    // a BIT copy and it is what survives when neither branch runs -- which is
    // not only the exact-zero case: a DENORMAL +0x18C satisfies neither
    // comparison, because the EE flushes the operand before comparing, so the
    // denormal is written through untouched.
    m_phys.m_heightOffset = m_committed.m_heightOffset;
    if (0.0f < m_committed.m_heightOffset)
    {
        // The clamp runs on the MEMBER, not on a local: the guest stores the
        // stepped value into +0x104, reads it back for the macro's third
        // comparison and stores again.
        m_phys.m_heightOffset = m_committed.m_heightOffset - dt * 6.0f;
        m_phys.m_heightOffset = VIMIN(m_parm28, VIMAX(0.0f, m_phys.m_heightOffset));
    }
    else if (m_committed.m_heightOffset < 0.0f)
    {
        m_phys.m_heightOffset = m_committed.m_heightOffset + dt * 6.0f;
        m_phys.m_heightOffset = VIMIN(0.0f, VIMAX(-m_parm28, m_phys.m_heightOffset));
    }

    // THREE powf calls, one per arm, cross-jumped after reload into the one
    // `jal` with each arm's damping loaded in a delay slot.  A selected
    // argument and one call gives the same branches, but then sched1 sees the
    // `damp = $f0` copy in the same block as the force copy below and sinks it
    // under the loads, which also swaps f.x and f.y between $f4 and $f5.
    if (!IN_LIQUID(m_phys))
    {
        if (m_phys.m_flags & 1)
            damp = powf(m_moveDamping, dt);
        else
            damp = powf(m_parm44, dt);
    }
    else
        damp = powf(m_swimDamping, dt);

    f = m_phys.m_force;
    if (m_parm60)
    {
        if (!IN_LIQUID(m_phys))
        {
            f.x += Gravity.x;
            f.y += Gravity.y;
            f.z += Gravity.z;
        }
        else if ((m_phys.m_flags & 0x10A8) == 0)
        {
            f.x += WGravity.x;
            f.y += WGravity.y;
            f.z += WGravity.z;
        }
    }

    dt2 = dt * dt;
    tq = m_phys.m_torque;

    // Every accumulator is handed to AddVec as the MEMBER, so each store at
    // +4 goes through an unflagged pseudo and kills the stack copy `f`: f.z
    // is reloaded after pos.y is written, and f.x/f.y again for the velocity.
    // ONE `damp`, reassigned by the second powf -- a separate angDamp gets a
    // register of its own where the guest moves both results into $f6.
    AddVec(m_phys.m_position, f, dt2 * damp);
    AddVec(m_phys.m_position, m_phys.m_velocity, dt * damp);
    AddVec(m_phys.m_velocity, f, dt);
    m_phys.m_velocity *= damp;

    damp = powf(m_turnDamping, dt);
    AddVec(m_phys.m_angles, tq, dt2 * damp);
    AddVec(m_phys.m_angles, m_phys.m_angularVel, dt * damp);
    AddVec(m_phys.m_angularVel, tq, dt);
    m_phys.m_angularVel *= damp;

    // The OVERSHOOT DETECTOR, the other half of SetTargetHeading's bang-bang
    // turn.  The target is wrapped to within pi of the heading for the
    // comparison, but the value ASSIGNED is the unwrapped one, so a snap can
    // move the heading by a full turn.  Torque is read from the OBJECT and it
    // is the pre-step torque: nothing above writes +0xEC.
    if (m_phys.m_flags & 0x100)
    {
        // No `heading` local: the member is read in all four places, so PRE
        // turns the later loads into a copy of the first (`mov.s f5,f0`); a
        // local assigned up front IS the load and gets no copy.
        wrapped = m_phys.m_targetHeading;
        if (3.14159274f < m_phys.m_targetHeading - m_phys.m_angles.x)
            wrapped = m_phys.m_targetHeading - 6.28318548f;
        else if (m_phys.m_targetHeading - m_phys.m_angles.x < -3.14159274f)
            wrapped = m_phys.m_targetHeading + 6.28318548f;
        if ((0.0f < m_phys.m_torque.x && wrapped < m_phys.m_angles.x) ||
            (m_phys.m_torque.x < 0.0f && m_phys.m_angles.x < wrapped))
        {
            m_phys.m_angles.x = m_phys.m_targetHeading;
            m_phys.m_torque.x = 0.0f;
            m_phys.m_angularVel.x = 0.0f;
        }
    }

    m_phys.m_angles.x = fmodf(m_phys.m_angles.x, 6.28318548f);

    // The pitch limit, and it reads FLAGS2 -- the second flag word -- not the
    // Flags word every test above this line reads.  The three media disagree
    // about what the limit even is: at a surface the pitch is REPLACED by
    // PitchAlt outright; submerged it is clamped to a HARDCODED +-89.43,
    // ignoring MinPitch and MaxPitch entirely; dry it is clamped against them
    // but the value clamped TO is PitchAlt, and only when PitchAlt is itself
    // on the far side.
    if (m_committed.m_flags & 0x10A8)
        m_phys.m_angles.y = m_committed.m_angles.y;
    else if (m_committed.m_flags & 0x854)
    {
        // The store is through a computed `float *` (SetPitchRange's idiom)
        // and the clamp runs on a LOCAL, so the result shares the input's
        // register; the dry arm reads its own local, so the two loads are
        // separate pseudos and reorg fills the `beqzl` from the dry side.
        float *p = &m_phys.m_angles.y;
        float y = *p;
        y = VIMIN(1.5607964f, VIMAX(-1.5607964f, y));
        *p = y;
    }
    else
    {
        float y = m_phys.m_angles.y;
        if (y < m_minPitch && y < m_committed.m_angles.y)
            m_phys.m_angles.y = m_committed.m_angles.y;
        else if (m_maxPitch < y && m_committed.m_angles.y < y)
            m_phys.m_angles.y = m_committed.m_angles.y;
    }
    m_phys.m_angles.z = 0.0f;
    return 0;
}

// 0x00485488.  Move relative to where the CAMERA is pointing rather than to
// where the character is facing.
//
// With the no-turn bit set it degenerates to Move(x, 0) -- the view is ignored
// AND the strafe input is dropped, because the guest passes a zero as the
// second argument rather than the caller's y.  Otherwise it builds a basis
// from the reference direction flattened onto the XZ plane (u), crosses it
// with world up for the strafe axis (w), and drives both the force and the
// heading from u*x + w*y, so the character turns to face where it is moving.
//
// Under the deadzone it does not stop: it re-issues the STORED TargetHeading,
// which keeps the bang-bang turn running while the stick is centred.
int VIWalkControl::ViewRelativeMove(const VIVect3 &ref, float x, float y)
{
    VIVect2 v;
    VIVect3 u;
    VIVect3 up;
    VIVect3 w;
    VIVect3 dir;
    float   h, p, r;
    float   mag;

    if (m_phys.m_flags & 0x200)
    {
        return Move(x, 0.0f);
    }
    if (!ON_FLOOR_OR_LIQUID(m_phys))
        return 0;
    v.x = x;
    v.y = y;
    mag = v.Magnitude();
    v.Normalize();
    u.x = ref.x;
    u.y = 0.0f;
    u.z = ref.z;
    u.Normalize();
    up.Set(0.0f, 1.0f, 0.0f);
    Cross(w, u, up);
    w.Normalize();
    if (m_moveDeadzone < mag)
    {
        dir = u;
        dir *= v.x;
        AddVec(dir, w, v.y);
        // THREE calls, one per arm, nested so that each arm's label has a
        // single jump and a barrier in front: cse then follows every arm from
        // the block that stored dir, keeps dir.x/dir.y in registers in all
        // three, and jump2 cross-jumps the identical tails into the one the
        // guest has.  `accel = c ? run : walk` and one call puts the call
        // after a join, where cse knows nothing and reloads all of dir; the
        // flat `&&` gives the else label two jumps and loses that arm.
        // dir.z is reloaded either way -- the m_force.y store before it goes
        // through AddVec's unflagged `accum` (see UpdateDirections).
        if (m_runThreshold < mag)
        {
            if (!(IN_MUD(m_phys) || m_phys.m_bits.slowRun))
                AddVec(m_phys.m_force, dir, m_runAccel);
            else
                AddVec(m_phys.m_force, dir, m_walkAccel);
        }
        else
            AddVec(m_phys.m_force, dir, m_walkAccel);
        dir.HPR(&h, &p, &r);
    }
    else
        h = m_phys.m_targetHeading;
    SetTargetHeading(h);
    return 0;
}

// 0x004856B8.  The same function with the run acceleration taken out -- it
// uses WalkAccel whatever the magnitude -- and with Walk rather than Move
// behind the no-turn bit.  Two separate 130-instruction functions in the guest
// with no call between them.
int VIWalkControl::ViewRelativeWalk(const VIVect3 &ref, float x, float y)
{
    VIVect2 v;
    VIVect3 u;
    VIVect3 up;
    VIVect3 w;
    VIVect3 dir;
    float   h, p, r;
    float   mag;

    if (m_phys.m_flags & 0x200)
    {
        return Walk(x, 0.0f);
    }
    if (!ON_FLOOR_OR_LIQUID(m_phys))
        return 0;
    v.x = x;
    v.y = y;
    mag = v.Magnitude();
    v.Normalize();
    u.x = ref.x;
    u.y = 0.0f;
    u.z = ref.z;
    u.Normalize();
    up.Set(0.0f, 1.0f, 0.0f);
    Cross(w, u, up);
    w.Normalize();
    if (m_moveDeadzone < mag)
    {
        dir = u;
        dir *= v.x;
        AddVec(dir, w, v.y);
        AddVec(m_phys.m_force, dir, m_walkAccel);
        dir.HPR(&h, &p, &r);
    }
    else
        h = m_phys.m_targetHeading;
    SetTargetHeading(h);
    return 0;
}

// 0x00486778.  Move the body from the committed position to the integrated
// one through the scene: a liquid-surface probe, a sphere sweep that skitters
// off what it hits (once with `flags`, twice without), the floor and headroom
// fit, and then the environment flags re-read at the new position.
int VIWalkControl::Move(int flags, VIScene &scene)
{
    int ok = 1;
    int moving = 1;
    int iters = flags ? 1 : 2;
    VICollSphere sphere;
    VITransEnvRay ray;
    VISceneFilter filter;
    VIVect3 from;
    VIVect3 to;
    VIVect3 floor;
    VIVect3 head;
    VIVect3 skit;
    VIVect3 above;
    VIVect3 below;
    VIVect3 normal;
    VIVect3 delta;

    from = m_committed.m_position;
    from.y += m_parm38;
    to = m_phys.m_position;
    to.y += m_parm38;

    if (IN_LIQUID(m_phys))
    {
        above.x = to.x;
        above.y = to.y + m_parm54;
        above.z = to.z;
        below.x = to.x;
        below.y = to.y - m_parm54;
        below.z = to.z;
        if (ray.Init(above, below, 15) < 0)
            return -1;
        if (scene.Collide(ray))
        {
            if (0.5f < ray.GetHitTime())
                to = ray.GetHitPoint();
            // A LOCAL read first: with the member in the else arm the load
            // traps (may_trap_p) and jump.c hoists the 0.0f arm instead.
            float v = m_phys.m_velocity.y;
            float r;
            if (v > 0.0f)
                r = 0.0f;
            else
                r = v;
            m_phys.m_velocity.y = r;
            float f = m_phys.m_force.y;
            if (0.0f < f)
                f = 0.0f;
            m_phys.m_force.y = f;
        }
    }

    if (flags)
    {
        delta.x = to.x - from.x;
        delta.y = 0.0f;
        delta.z = to.z - from.z;
        if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z < 0.0001f)
            moving = 0;
    }

    if (moving)
    {
        if (sphere.Init(from, to, m_parm34) < 0)
            return -1;
        filter.Init(m_parm64);
        if (scene.Collide(sphere, filter))
        {
            m_phys.m_flags |= 2;
            if (m_phys.m_flags & 1)
                skit.Set(0.0f, 1.0f, 0.0f);
            else
                skit.Zero();
            for (int i = 0; i < iters; i++)
            {
                sphere.Skitter(skit);
                if (!scene.Collide(sphere, filter))
                {
                    ok = 0;
                    break;
                }
                if (0.001f < sphere.GetHitTime())
                    ok = 0;
                else
                    normal += sphere.GetHitNormal();
            }
            if (ok && (m_phys.m_flags & 0x855) == 0)
            {
                normal.Normalize();
                normal *= m_parm34;
                to += normal;
                m_phys.m_force.x = 0.0f;
                m_phys.m_force.z = 0.0f;
                m_phys.m_velocity.x = 0.0f;
                m_phys.m_velocity.z = 0.0f;
                sphere.Init(from, to, m_parm34);
                scene.Collide(sphere, filter);
            }
            to = sphere.GetHitPoint();
        }
        else
        {
            m_phys.m_flags &= ~2;
            VIVect3 *e = &sphere.m_end;
            to = *e;
        }
    }

    if (CalcFit(flags, scene, to, floor, head) < 0)
        return -1;

    if (head.y - floor.y < m_parm30)
        to = from;
    else if (!IN_LIQUID(m_phys))
    {
        if (to.y - floor.y < m_parm38)
        {
            to.y = floor.y + m_parm38;
            m_phys.m_floor = floor.y;
            m_phys.m_flags |= 1;
            m_phys.m_velocity.y = 0.0f;
            if (m_committed.m_flags & 1)
            {
                float rise = floor.y - m_committed.m_floor;
                if (m_parm24 < rise && 0.9f < m_phys.m_landscapeNormal.y)
                    m_phys.m_heightOffset -= rise;
            }
        }
        else if (m_phys.m_floor - floor.y < m_parm28)
        {
            to.y = floor.y + m_parm38;
            m_phys.m_floor = floor.y;
            m_phys.m_flags |= 1;
            m_phys.m_velocity.y = 0.0f;
            if (m_committed.m_flags & 1)
            {
                float rise = floor.y - m_committed.m_floor;
                if (rise < -m_parm24 && 0.9f < m_phys.m_landscapeNormal.y)
                    m_phys.m_heightOffset -= rise;
            }
        }
        else
            m_phys.m_flags &= ~1;
    }
    else if (to.y - floor.y < m_parm38)
    {
        to.y = floor.y + m_parm38;
        m_phys.m_floor = floor.y;
        m_phys.m_velocity.y = 0.0f;
        m_phys.m_flags |= 1;
    }
    else
        m_phys.m_flags &= ~1;

    m_phys.m_position = to;

    unsigned int env = scene.GetEnvFlags(m_phys.m_position);
    if (env & 1)
        m_phys.m_flags |= 0x4;
    else
        m_phys.m_flags &= ~0x4;
    if (env & 4)
        m_phys.m_flags |= 0x10;
    else
        m_phys.m_flags &= ~0x10;
    if (env & 2)
        m_phys.m_flags |= 0x40;
    else
        m_phys.m_flags &= ~0x40;
    if (env & 8)
        m_phys.m_flags |= 0x800;
    else
        m_phys.m_flags &= ~0x800;

    above.x = to.x;
    above.y = to.y + m_parm54;
    above.z = to.z;
    below.x = to.x;
    below.y = to.y - m_parm54;
    below.z = to.z;
    unsigned int up = scene.GetEnvFlags(above);
    unsigned int dn = scene.GetEnvFlags(below);
    if (!(up & 1) && (dn & 1))
        m_phys.m_flags |= 0x8;
    else
        m_phys.m_flags &= ~0x8;
    if (!(up & 4) && (dn & 4))
        m_phys.m_flags |= 0x20;
    else
        m_phys.m_flags &= ~0x20;
    if (!(up & 2) && (dn & 2))
        m_phys.m_flags |= 0x80;
    else
        m_phys.m_flags &= ~0x80;
    if (!(up & 8) && (dn & 8))
        m_phys.m_flags |= 0x1000;
    else
        m_phys.m_flags &= ~0x1000;

    m_phys.m_position.y -= m_parm38;
    if (scene.GetEnvFlags(m_phys.m_position) & 8)
        m_phys.m_flags |= 0x2000;
    else
        m_phys.m_flags &= ~0x2000;
    return 0;
}

// 0x00487438.  CalcFloor's body written out again (there is no call), then
// a second sweep back UP from `a` to twice m_parm30 above the floor -- the
// headroom check -- unless the caller passes non-zero `flags`, which takes the
// clearance as given.
int VIWalkControl::CalcFit(int flags, VIScene &scene, VIVect3 &a, VIVect3 &b,
                           VIVect3 &c)
{
    VICollSphere sphere;
    VIWalkFloorFilter filter;

    filter.Init(m_parm64);
    m_phys.m_flags &= ~0x400;
    b = a;
    b.y -= m_parm30;
    if (sphere.Init(a, b, m_parm3C) < 0)
        return -1;
    if (scene.Collide(sphere, filter))
    {
        b = sphere.GetHitPoint();
        m_phys.m_landscapeNormal = sphere.GetHitNormal();
        if (filter.m_hitActor == -1)
            m_phys.m_flags |= 0x400;
    }
    else
    {
        VIVect3 *e = &sphere.m_end;
        b = *e;
        m_phys.m_landscapeNormal.Zero();
    }
    if (flags == 0)
    {
        c = a;
        c.y = b.y;
        c.y += m_parm30 * 2.0f;
        if (sphere.Init(a, c, m_parm3C) < 0)
            return -1;
        if (scene.Collide(sphere, filter))
            c = sphere.GetHitPoint();
        else
        {
            VIVect3 *e = &sphere.m_end;
            c = *e;
        }
    }
    else
    {
        c = a;
        c.y = b.y;
        c.y += m_parm30 * 2.0f;
    }
    return 0;
}

// 0x004876A0.  Drop a sphere from `from` down m_parm30 and leave the floor in
// `to`.  The landscape bit is set only when what was hit is not an actor --
// the terrain itself, not something standing on it.
int VIWalkControl::CalcFloor(VIScene &scene, VIVect3 &from, VIVect3 &to)
{
    VICollSphere sphere;
    VIWalkFloorFilter filter;

    filter.Init(m_parm64);
    m_phys.m_flags &= ~0x400;
    to = from;
    to.y -= m_parm30;
    if (sphere.Init(from, to, m_parm3C) < 0)
        return -1;
    if (scene.Collide(sphere, filter))
    {
        to = sphere.GetHitPoint();
        m_phys.m_landscapeNormal = sphere.GetHitNormal();
        if (filter.m_hitActor == -1)
            m_phys.m_flags |= 0x400;
    }
    else
    {
        // Through a pointer: +0 folds onto sp, +4/+8 come off the register.
        VIVect3 *e = &sphere.m_end;
        to = *e;
        m_phys.m_landscapeNormal.Zero();
    }
    return 0;
}

// 0x00487C00 -- sprite type 5 is VICSprite, so the floor query passes through
// every player and NPC and lands on the terrain under them.
int VIWalkFloorFilter::FilterActor(VIScene &scene, int actor)
{
    return scene.GetActorSpriteType(actor) == 5;
}

// 0x00487BB0.  The render position: the body's own, raised by its height.
VIVect3 VIWalkPhysics::GetSmoothLocation() const
{
    VIVect3 v;
    v = m_position;
    v.y += m_heightOffset;
    return v;
}
