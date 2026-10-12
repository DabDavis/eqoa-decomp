#include "vi.h"

VIVect3 VIWalkControl::Gravity(0.0f, -19.6f, 0.0f);
VIVect3 VIWalkControl::WGravity(0.0f, -1.0f, 0.0f);

// SUPPORT 0x00484930 .. 0x00487BB0.  VIWalkControl, tier 2.
//
// Every function here has a console-checked semantic twin in
// go-eqoa-vi/viwalk, so a mismatch below is CODEGEN and nothing else -- the
// Phase 11/13 position, and the reason this class is worth doing before the
// ones whose behaviour still has to be read out of the disassembly.

// The tail every input method shares: `accum += dir * k`, one component at a
// time.  It is a real helper and not three statements written out: taking both
// vectors by REFERENCE is what makes the store to accum alias `dir` on the
// stack, so the guest writes each dir component back and reloads it -- which a
// version reading dir straight out of registers cannot produce.
static inline void Cross(VIVect3 &out, const VIVect3 &a, const VIVect3 &b)
{
    out.x = a.y * b.z - a.z * b.y;
    out.y = -a.x * b.z + a.z * b.x;
    out.z = a.x * b.y - a.y * b.x;
}

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

// 0x00484F58.  `sw $zero` six times, not `swc1`, so a component holding -0.0
// comes back +0.0.  Zero() per member rather than six stores on `this`: the
// target gives each vector its own base register (Phase 13).
int VIWalkControl::ClearForces()
{
    m_force.Zero();
    m_torque.Zero();
    return 0;
}

// 0x00485D10.  `(flags & 0x0C) == 0x04` -- ONE test, not two, and the guest
// if-converts it to xori + movn.
int VIWalkControl::IsUnderWater()
{
    if ((m_flags & 0x0C) == 0x04)
        return 1;
    return 0;
}

// 0x00485D28.  Submerged in something and at the surface of nothing.
int VIWalkControl::IsUnderLiquid()
{
    if (((int)m_flags & 0x854) != 0 && ((int)m_flags & 0x10A8) == 0)
        return 1;
    return 0;
}

// 0x00485D50.
int VIWalkControl::IsAtLiquidSurface()
{
    return ((int)m_flags & 0x10A8) != 0;
}

// 0x00485D60.
int VIWalkControl::IsInLava()
{
    if (m_flags & 0x30)
        return 1;
    return 0;
}

// 0x00485D78.
int VIWalkControl::IsInSlime()
{
    if (m_flags & 0xC0)
        return 1;
    return 0;
}

// 0x00485D90.
int VIWalkControl::IsInMud()
{
    if (m_flags & 0x1800)
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
    AddVec(m_torque, dir, t);
    return 0;
}

// 0x004858C0.  Vertical thrust, gated on being in a liquid at all.  The
// direction is the constant (0,1,0) built on the stack and multiplied through
// in full: gcc folds the y term's `* 1.0f` away and cannot fold the other two.
int VIWalkControl::SwimUp(float t)
{
    if (m_flags & 0x18FC)
    {
        float k;

        t = VIMIN(1.0f, VIMAX(-1.0f, t));
        k = m_swimAccel * t;
        VIVect3 up(0.0f, 1.0f, 0.0f);
        AddVec(m_force, up, k);
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

    if ((m_flags2 & 0x10A8) == 0)
    {
        t = VIMIN(1.0f, VIMAX(-1.0f, t));
        axis.x = 0.0f;
        axis.y = 1.0f;
        axis.z = 0.0f;
        AddVec(dir, axis, m_pitchAccel);
        AddVec(m_torque, dir, t);
    }
    else
    {
        t = VIMIN(1.0f, VIMAX(0.0f, t));
        axis.x = 0.0f;
        axis.y = -1.0f;
        axis.z = 0.0f;
        AddVec(dir, axis, m_swimAccel);
        AddVec(m_force, dir, t);
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
int VIWalkControl::UpdateDirections()
{
    VIVect3 *fwd = &m_forward;
    VIVect3 *fac = &m_facing;
    VIVect3 *rgt = &m_right;

    fwd->HPR(m_angles.x, 0.0f, m_angles.z);
    fac->HPR(m_angles.x, m_angles.y, m_angles.z);

    VIVect3 up(0.0f, 1.0f, 0.0f);
    Cross(*rgt, *fwd, up);
    rgt->Normalize();

    Cross(m_up, *fac, *rgt);
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
    p = &m_angles.y;
    *p = VIMIN(m_maxPitch, VIMAX(m_minPitch, m_angles.y));
    p = &m_anglesAlt.y;
    *p = VIMIN(m_maxPitch, VIMAX(m_minPitch, m_anglesAlt.y));
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

    m_targetHeading = h;
    d = h - m_angles.x;
    if (3.14159274f < d)
        h = h - 6.28318548f;
    else if (d < -3.14159274f)
        h = h + 6.28318548f;
    if (m_angles.x < h)
        TurnLeft(1.0f);
    if (h < m_angles.x)
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

    if (((int)m_flags & 0x10A8) != 0)
        target = -0.08726646f;
    else if (((int)m_flags & 0x854) != 0)
        target = m_angles.y;
    else
        target = VIMIN(m_maxPitch, VIMAX(m_minPitch, m_angles.y));

    delta = target - m_angles.y;
    step = m_autoPitchRate * dt;
    if (0.0f <= delta)
    {
        if (delta <= step)
            m_angles.y = target;
        else
            m_angles.y = m_anglesAlt.y + step;
    }
    else
    {
        if (step <= delta)
            m_angles.y = target;
        else
            m_angles.y = m_anglesAlt.y - step;
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

    if (m_flags & 0x18FF)
    {
        v.x = VIMIN(1.0f, VIMAX(-1.0f, x));
        v.y = VIMIN(1.0f, VIMAX(-1.0f, y));
        v.z = 0.0f;
        if (1.0f < v.Magnitude())
            v.Normalize();
        // Two calls and not one through a selected pointer: a pointer picked
        // with `dir = cond ? &a : &b` if-converts to `movn`, where the guest
        // BRANCHES and duplicates the whole call into both arms.
        if ((m_flags & 0x18FC) == 0)
            AddVec(m_force, m_forward, m_walkAccel * v.x);
        else
            AddVec(m_force, m_facing, m_walkAccel * v.x);
        AddVec(m_force, m_right, m_walkAccel * v.y);
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

    if (m_flags & 0x18FF)
    {
        v.x = VIMIN(1.0f, VIMAX(-1.0f, x));
        v.y = VIMIN(1.0f, VIMAX(-1.0f, y));
        v.z = 0.0f;
        if (1.0f < v.Magnitude())
            v.Normalize();
        if (m_runThreshold < x)
        {
            if ((m_flags & 0x18FC) == 0)
            {
                if (((int)m_flags & 0x2000) == 0)
                    AddVec(m_force, m_forward, m_runAccel * v.x);
                else
                    AddVec(m_force, m_forward, m_walkAccel * v.x);
            }
            else if ((m_flags & 0x1800) == 0)
                AddVec(m_force, m_facing, m_runAccel * v.x);
            else
                AddVec(m_force, m_facing, m_walkAccel * v.x);
            AddVec(m_force, m_right, m_runAccel * v.y);
        }
        else
        {
            if ((m_flags & 0x18FC) == 0)
                AddVec(m_force, m_forward, m_walkAccel * v.x);
            else
                AddVec(m_force, m_facing, m_walkAccel * v.x);
            AddVec(m_force, m_right, m_walkAccel * v.y);
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

    if (m_flags2 & 0x400)
    {
        n = m_landscapeNormal;
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
    VIVect3  d2;
    VIVect3 *prev = &m_prevPosition;
    VIVect3 *pos = &m_position;
    VIVect3 *fwd = &m_forward;
    VIVect3 *rgt = &m_right;
    float    vF, vR;

    d.x = pos->x - prev->x;
    d.y = pos->y - prev->y;
    d.z = pos->z - prev->z;
    d2 = d;
    vF = (d.x * fwd->x + d.y * fwd->y + d.z * fwd->z) / dt;
    vR = (d.x * rgt->x + d.y * rgt->y + d.z * rgt->z) / dt;

    if ((m_flags & 0x18FC) == 0)
    {
        if (((int)m_flags & 1) == 0)
            m_locoState = 6;
        else if (m_walkSpeed + (m_runSpeed - m_walkSpeed) * 0.1f < vF)
            m_locoState = 3;
        else if (m_speedThreshold < vR && fabsf(vF) < fabsf(vR) &&
                 m_moveMode != 1 && m_moveMode != 2)
            m_locoState = 11;
        else if (vR < -m_speedThreshold && fabsf(vF) < fabsf(vR) &&
                 m_moveMode != 1 && m_moveMode != 2)
            m_locoState = 10;
        else if (m_speedThreshold < vR && fabsf(vF) * 1.5f < fabsf(vR))
            m_locoState = 11;
        else if (vR < -m_speedThreshold && fabsf(vF) * 1.5f < fabsf(vR))
            m_locoState = 10;
        else if (m_speedThreshold < vF)
            m_locoState = 1;
        else if (vF < -m_speedThreshold)
            m_locoState = 2;
        else if (m_turnRate * 0.5f < m_angularVel.x)
            m_locoState = 4;
        else if (m_angularVel.x < -(m_turnRate * 0.5f))
            m_locoState = 5;
        else
            m_locoState = 0;
    }
    else
    {
        // The liquid branch: QUARTER turn rate, not half, and the PITCH rate
        // counts as well as the heading rate -- a swimmer looking up and down
        // is animating.  The last comparison is on |vR| and its two outcomes
        // differ: fast sideways drift is state 12, not 13.
        if (m_turnRate * 0.25f < fabsf(m_angularVel.y))
            m_locoState = 12;
        else if (m_turnRate * 0.25f < fabsf(m_angularVel.x))
            m_locoState = 12;
        else if (m_speedThreshold < vF)
            m_locoState = 12;
        else if (m_speedThreshold < fabsf(vR))
            m_locoState = 12;
        else
            m_locoState = 13;
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
    VIVect3 *pos;
    VIVect3 *vel;
    VIVect3 *ang;
    VIVect3 *avel;
    float    damp, angDamp, dt2, k;
    float    heading, wrapped;

    // The +0x18C -> +0x104 decay.  The unconditional copy on the first line is
    // a BIT copy and it is what survives when neither branch runs -- which is
    // not only the exact-zero case: a DENORMAL +0x18C satisfies neither
    // comparison, because the EE flushes the operand before comparing, so the
    // denormal is written through untouched.
    m_field104 = m_field18C;
    if (0.0f < m_field18C)
    {
        // The clamp runs on the MEMBER, not on a local: the guest stores the
        // stepped value into +0x104, reads it back for the macro's third
        // comparison and stores again.
        m_field104 = m_field18C - dt * 6.0f;
        m_field104 = VIMIN(m_parm28, VIMAX(0.0f, m_field104));
    }
    else if (m_field18C < 0.0f)
    {
        m_field104 = m_field18C + dt * 6.0f;
        m_field104 = VIMIN(0.0f, VIMAX(-m_parm28, m_field104));
    }

    // ONE powf call with its argument selected, not three calls cross-jumped:
    // the guest loads the chosen damping into f12 in each arm's DELAY SLOT and
    // falls into a single `jal`.
    if (m_flags & 0x18FC)
        k = m_swimDamping;
    else if (((int)m_flags & 1) == 0)
        k = m_parm44;
    else
        k = m_moveDamping;
    damp = powf(k, dt);

    f = m_force;
    if (m_parm60)
    {
        if ((m_flags & 0x18FC) == 0)
        {
            f.x += Gravity.x;
            f.y += Gravity.y;
            f.z += Gravity.z;
        }
        else if (((int)m_flags & 0x10A8) == 0)
        {
            f.x += WGravity.x;
            f.y += WGravity.y;
            f.z += WGravity.z;
        }
    }

    dt2 = dt * dt;
    tq = m_torque;

    pos = &m_position;
    vel = &m_velocity;
    AddVec(*pos, f, dt2 * damp);
    AddVec(*pos, *vel, dt * damp);
    AddVec(*vel, f, dt);
    vel->x *= damp;
    vel->y *= damp;
    vel->z *= damp;

    angDamp = powf(m_turnDamping, dt);
    ang = &m_angles;
    avel = &m_angularVel;
    AddVec(*ang, tq, dt2 * angDamp);
    AddVec(*ang, *avel, dt * angDamp);
    AddVec(*avel, tq, dt);
    avel->x *= angDamp;
    avel->y *= angDamp;
    avel->z *= angDamp;

    // The OVERSHOOT DETECTOR, the other half of SetTargetHeading's bang-bang
    // turn.  The target is wrapped to within pi of the heading for the
    // comparison, but the value ASSIGNED is the unwrapped one, so a snap can
    // move the heading by a full turn.  Torque is read from the OBJECT and it
    // is the pre-step torque: nothing above writes +0xEC.
    if ((int)m_flags & 0x100)
    {
        heading = m_angles.x;
        wrapped = m_targetHeading;
        if (3.14159274f < m_targetHeading - heading)
            wrapped = m_targetHeading - 6.28318548f;
        else if (m_targetHeading - heading < -3.14159274f)
            wrapped = m_targetHeading + 6.28318548f;
        if ((0.0f < m_torque.x && wrapped < heading) ||
            (m_torque.x < 0.0f && heading < wrapped))
        {
            m_angles.x = m_targetHeading;
            m_torque.x = 0.0f;
            m_angularVel.x = 0.0f;
        }
    }

    m_angles.x = fmodf(m_angles.x, 6.28318548f);

    // The pitch limit, and it reads FLAGS2 -- the second flag word -- not the
    // Flags word every test above this line reads.  The three media disagree
    // about what the limit even is: at a surface the pitch is REPLACED by
    // PitchAlt outright; submerged it is clamped to a HARDCODED +-89.43,
    // ignoring MinPitch and MaxPitch entirely; dry it is clamped against them
    // but the value clamped TO is PitchAlt, and only when PitchAlt is itself
    // on the far side.
    if (m_flags2 & 0x10A8)
        m_angles.y = m_anglesAlt.y;
    else if (m_flags2 & 0x854)
        m_angles.y = VIMIN(1.5607964f, VIMAX(-1.5607964f, m_angles.y));
    else if (m_angles.y < m_minPitch && m_angles.y < m_anglesAlt.y)
        m_angles.y = m_anglesAlt.y;
    else if (m_maxPitch < m_angles.y && m_anglesAlt.y < m_angles.y)
        m_angles.y = m_anglesAlt.y;
    m_angles.z = 0.0f;
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
    float   mag, accel;

    if ((int)m_flags & 0x200)
    {
        Move(x, 0.0f);
        return 0;
    }
    if ((m_flags & 0x18FF) == 0)
        return 0;
    v.x = x;
    v.y = y;
    mag = v.Magnitude();
    v.Normalize();
    u.x = ref.x;
    u.y = 0.0f;
    u.z = ref.z;
    u.Normalize();
    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;
    Cross(w, u, up);
    w.Normalize();
    if (m_moveDeadzone < mag)
    {
        dir.x = u.x * v.x + w.x * v.y;
        dir.y = u.y * v.x + w.y * v.y;
        dir.z = u.z * v.x + w.z * v.y;
        if (m_runThreshold < mag && ((int)m_flags & 0x3800) == 0)
            accel = m_runAccel;
        else
            accel = m_walkAccel;
        AddVec(m_force, dir, accel);
        dir.HPR(&h, &p, &r);
    }
    else
        h = m_targetHeading;
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

    if ((int)m_flags & 0x200)
    {
        Walk(x, 0.0f);
        return 0;
    }
    if ((m_flags & 0x18FF) == 0)
        return 0;
    v.x = x;
    v.y = y;
    mag = v.Magnitude();
    v.Normalize();
    u.x = ref.x;
    u.y = 0.0f;
    u.z = ref.z;
    u.Normalize();
    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;
    Cross(w, u, up);
    w.Normalize();
    if (m_moveDeadzone < mag)
    {
        dir.x = u.x * v.x + w.x * v.y;
        dir.y = u.y * v.x + w.y * v.y;
        dir.z = u.z * v.x + w.z * v.y;
        AddVec(m_force, dir, m_walkAccel);
        dir.HPR(&h, &p, &r);
    }
    else
        h = m_targetHeading;
    SetTargetHeading(h);
    return 0;
}

// 0x00487C00 -- sprite type 5 is VICSprite, so the floor query passes through
// every player and NPC and lands on the terrain under them.
int VIWalkFloorFilter::FilterActor(VIScene &scene, int actor)
{
    return scene.GetActorSpriteType(actor) == 5;
}
