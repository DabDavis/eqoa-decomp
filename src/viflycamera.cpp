// VIFlyCamera -- SUPPORT 0x00441210 .. 0x004422F8.  Phase 31.
#include "vi.h"

extern "C" float powf(float, float);
extern "C" float fmodf(float, float);

// Phase 18's two helpers, verbatim: taking both vectors by REFERENCE is what
// makes the store alias the direction on the stack, so the guest writes each
// component back and reloads it.
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

// 0x00442168.
int VIFlyCamera::ClearCameraForces()
{
    m_force.Zero();
    m_torque.Zero();
    return 0;
}

// 0x00441A88.  A stub: the fly camera collides with nothing.
int VIFlyCamera::MoveCamera(VIScene &)
{
    return 0;
}

// 0x00442068.  The only input that bypasses the rigid body entirely.
int VIFlyCamera::FreeLook(float h, float p)
{
    m_angles.x += h;
    m_angles.y += p;
    return 0;
}

// 0x00441A90.
int VIFlyCamera::TurnLeft(float t)
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

// 0x00441B78.
int VIFlyCamera::PitchDown(float t)
{
    VIVect3 dir;
    VIVect3 axis;

    t = VIMIN(1.0f, VIMAX(-1.0f, t));
    axis.x = 0.0f;
    axis.y = 1.0f;
    axis.z = 0.0f;
    AddVec(dir, axis, m_pitchAccel);
    AddVec(m_torque, dir, t);
    return 0;
}

// 0x00441C70.  Fly along the FACING vector -- pitch included, so this is the
// one that climbs.  The magnitude clamp is a shipped no-op: its value is
// computed, clamped and dropped, and the normalise that follows ignores it.
int VIFlyCamera::Forward(float x, float y)
{
    VIVect3 d;

    d.x = VIMIN(1.0f, VIMAX(-1.0f, x));
    d.y = VIMIN(1.0f, VIMAX(-1.0f, y));
    d.z = 0.0f;
    // The magnitude IS bound to a local first: VIMIN/VIMAX are macros, so
    // writing the call inside them would evaluate it three times -- and the
    // clamped value is then dropped on the floor, which is the shipped bug.
    float m = d.Magnitude();
    m = VIMIN(1.0f, VIMAX(-1.0f, m));
    d.Normalize();

    AddVec(m_force, m_facing, m_moveAccel * d.x);
    AddVec(m_force, m_right, m_moveAccel * d.y);
    return 0;
}

// 0x00441E18.  The same, along the FLAT forward, with world up for the third
// axis.
int VIFlyCamera::Hover(float x, float y, float z)
{
    VIVect3 d;

    d.x = VIMIN(1.0f, VIMAX(-1.0f, x));
    d.y = VIMIN(1.0f, VIMAX(-1.0f, y));
    d.z = VIMIN(1.0f, VIMAX(-1.0f, z));
    // The magnitude IS bound to a local first: VIMIN/VIMAX are macros, so
    // writing the call inside them would evaluate it three times -- and the
    // clamped value is then dropped on the floor, which is the shipped bug.
    float m = d.Magnitude();
    m = VIMIN(1.0f, VIMAX(-1.0f, m));
    d.Normalize();

    AddVec(m_force, m_forward, m_moveAccel * d.x);
    AddVec(m_force, m_right, m_moveAccel * d.y);
    {
        VIVect3 up(0.0f, 1.0f, 0.0f);
        AddVec(m_force, up, m_moveAccel * d.z);
    }
    return 0;
}

// 0x00442088.  Hand the position and the heading to the camera, through the
// shake.
int VIFlyCamera::SetCamera(VICamera &cam, VIScene &)
{
    VIVect3 pos;
    VIVect3 dir;

    pos = m_position;
    // The roll argument is a literal 0.0f, and gcc materialises it by loading
    // dir.z -- a slot its own constructor has just zeroed.
    dir.HPR(m_angles.x, m_angles.y, 0.0f);

    // COPY-constructed, not default-constructed and assigned: the effect is
    // free to move them and the camera must not see the object's own vectors.
    VIVect3 p = pos;
    VIVect3 d = dir;

    m_effect.Apply(p, d);
    cam(p, d);
    return 0;
}

// 0x00441788.  The rigid-body step: force into position, torque into angles,
// each damped by its own powf.
int VIFlyCamera::UpdateCameraPhysics(float dt)
{
    VIVect3 f;
    VIVect3 tq;
    float   damp, dt2;

    dt2 = dt * dt;
    f = m_force;
    tq = m_torque;

    damp = powf(m_moveDamping, dt);

    AddVec(m_position, f, dt2 * damp);
    AddVec(m_position, m_velocity, dt * damp);
    AddVec(m_velocity, f, dt);
    {
        // Phase 9's pointer variable, and it must be declared HERE: a pointer
        // held from the top of the function would still be live across the
        // fmodf call below and land in a callee-saved register, where the
        // target rematerialises the address after it.
        VIVect3 *vel = &m_velocity;
        vel->x *= damp;
        vel->y *= damp;
        vel->z *= damp;
    }

    damp = powf(m_turnDamping, dt);

    AddVec(m_angles, tq, dt2 * damp);
    AddVec(m_angles, m_angularVel, dt * damp);

    m_angles.x = fmodf(m_angles.x, 6.28318548f);
    // The member is read into a LOCAL first: the macro re-reads its argument
    // three times, and with the member written out gcc allocates a fresh
    // destination register where the target lets the upper bound's own
    // register be the default.
    float p = m_angles.y;
    float c = VIMIN(1.5607964f, VIMAX(-1.5607964f, p));
    m_angles.y = c;

    AddVec(m_angularVel, tq, dt);
    {
        VIVect3 *avel = &m_angularVel;
        avel->x *= damp;
        avel->y *= damp;
        avel->z *= damp;
    }

    m_angles.z = 0.0f;
    return 0;
}

// The direction rebuild, written out at both sites that need it: there is no
// symbol for it, so it is either a static gcc inlined everywhere or two copies
// in the source, and the two are indistinguishable in the object code.
//
//   Facing  = HPR(heading, pitch, roll)
//   Forward = HPR(heading,     0, roll)
//   Right   = normalize(Facing x up)     -- FACING, not Forward: the fly
//                                           camera strafes in its own plane
// 0x00442190.  Turn the three speeds and the three dampings into the three
// accelerations the input methods add, then rebuild the basis.
int VIFlyCamera::InitCameraIntermediate()
{
    float dt;
    float moveD, turnD, pitchD;

    dt = (float)m_tickMS * 0.001f;
    moveD = powf(m_moveDamping, dt);
    turnD = powf(m_turnDamping, dt);
    pitchD = powf(m_pitchDamping, dt);

    m_moveAccel = (m_moveSpeed - m_moveSpeed * moveD) / (moveD * dt);
    m_turnAccel = (m_turnRate - m_turnRate * turnD) / (turnD * dt);
    m_pitchAccel = (m_pitchRate - m_pitchRate * pitchD) / (pitchD * dt);

    m_facing.HPR(m_angles.x, m_angles.y, m_angles.z);
    m_forward.HPR(m_angles.x, 0.0f, m_angles.z);
    {
        VIVect3 up(0.0f, 1.0f, 0.0f);
        Cross(m_right, m_facing, up);
    }
    m_right.Normalize();
    return 0;
}

// 0x004412C8.  The real constructor: stamp the parameters, seed the committed
// copy from the caller's position and heading, and copy it forward into the
// live one.  m_lastVelocity and m_lastForce are never written -- a shipped
// hole, since the copy below carries them into the live body.
int VIFlyCamera::Init(const VIVect3 &pos, const VIVect3 &angles,
                      VICamera &cam, VIScene &scene)
{
    m_tickMS = 32;
    m_maxTickMS = 200;
    m_moveSpeed = 45.0f;
    m_turnRate = 1.5707964f;
    m_pitchRate = 1.5707964f;
    m_moveDamping = 0.00999999978f;
    m_pitchDamping = 0.00999999978f;
    m_turnDamping = 0.00999999978f;
    m_pF8 = -1;

    m_lastTime = scene.GetTime();
    m_lastPosition = pos;
    m_lastAngles = angles;
    m_lastAngularVel.Zero();
    m_lastTorque.Zero();

    m_time = m_lastTime;
    m_position = m_lastPosition;
    m_angles = m_lastAngles;
    m_velocity = m_lastVelocity;
    m_angularVel = m_lastAngularVel;
    m_force = m_lastForce;
    m_torque = m_lastTorque;

    InitCameraIntermediate();
    SetCamera(cam, scene);
    m_effect.Reset();
    return 0;
}

// 0x00441210.  Take the position and heading off a camera that is already
// aimed -- the const HPR overload writes its three angles through pointers.
int VIFlyCamera::Init(VICamera &cam, VIScene &scene)
{
    VIVect3 pos = cam.m_position;
    VIVect3 dir = cam.m_dir;
    VIVect3 hpr;

    dir.HPR(&hpr.x, &hpr.y, &hpr.z);
    return Init(pos, hpr, cam, scene);
}

// 0x004414A8.  Catch up to the scene clock in fixed sub-steps.  A gap larger
// than m_maxTickMS is not simulated at all -- the clock is advanced past it and
// the body left where it was.
int VIFlyCamera::Process(VICamera &cam, VIScene &scene)
{
    long dt;
    long step;

    dt = scene.GetTime() - m_lastTime;
    m_effect.Update((float)dt * 0.001f);

    while (dt > 0)
    {
        if (m_maxTickMS < dt)
        {
            step = dt - m_maxTickMS;
            m_time = m_lastTime + step;
        }
        else
        {
            step = VIMIN((long)m_tickMS, dt);
            m_time = m_lastTime + step;
            UpdateCameraPhysics((float)step * 0.001f);
            MoveCamera(scene);
        }

        m_lastTime = m_time;
        m_lastPosition = m_position;
        m_lastAngles = m_angles;
        m_lastVelocity = m_velocity;
        m_lastAngularVel = m_angularVel;
        m_lastForce = m_force;
        m_lastTorque = m_torque;
        dt -= step;
    }

    ClearCameraForces();

    m_facing.HPR(m_angles.x, m_angles.y, m_angles.z);
    m_forward.HPR(m_angles.x, 0.0f, m_angles.z);
    {
        VIVect3 up(0.0f, 1.0f, 0.0f);
        Cross(m_right, m_facing, up);
    }
    m_right.Normalize();

    SetCamera(cam, scene);
    return 0;
}
