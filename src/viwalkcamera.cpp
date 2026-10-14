// VIWalkCamera -- SUPPORT 0x00482020 .. 0x00483EB8.  Phase 31.
#include "vi.h"

extern "C" float powf(float, float);
extern "C" float fmodf(float, float);

static inline void AddVec(VIVect3 &accum, const VIVect3 &dir, float k)
{
    accum.x += dir.x * k;
    accum.y += dir.y * k;
    accum.z += dir.z * k;
}

// The damping is a VECTOR here, and only its x is ever written: the pitch and
// roll velocities are multiplied by the zero its constructor left.
static inline void MulVec(VIVect3 &v, const VIVect3 &k)
{
    v.x *= k.x;
    v.y *= k.y;
    v.z *= k.z;
}

// AddVec's mirror, for mode 2's eye BEHIND the target.  MoveCamera's two
// non-orbit arms build `to` through these and not member by member: the
// helper's `k` is its own pseudo, and that is what puts the arms' FP registers
// where the target has them (25 wrong words to byte-exact, both together;
// either one alone is worse than neither).
static inline void SubVec(VIVect3 &accum, const VIVect3 &dir, float k)
{
    accum.x -= dir.x * k;
    accum.y -= dir.y * k;
    accum.z -= dir.z * k;
}

// 0x00483780.
int VIWalkCamera::ClearCameraForces()
{
    m_cur.m_orbitForce.Zero();
    m_cur.m_distForce = 0.0f;
    return 0;
}

// 0x004830A8.
int VIWalkCamera::SetOrbitPitchAngle(float p)
{
    m_last.m_orbit.y = p;
    m_cur.m_orbit.y = p;
    return 0;
}

// 0x00483090.
int VIWalkCamera::SetDefOrbitPitchAngle(float p)
{
    m_defOrbitPitch = p;
    m_last.m_orbit.y = p;
    m_cur.m_orbit.y = p;
    return 0;
}

// 0x004830B8.  The zoom clamp, and the ceiling it reads is the 25.0 the QOL
// patch moves.
int VIWalkCamera::SetOrbitDistance(float d)
{
    m_cur.m_dist = VIMIN(m_maxOrbitDistance, VIMAX(m_minOrbitDistance, d));
    m_cur.m_distVel = 0.0f;
    m_last.m_distVel = 0.0f;
    m_cur.m_distForce = 0.0f;
    m_last.m_dist = m_cur.m_dist;
    m_last.m_distForce = 0.0f;
    return 0;
}

// 0x00483330.  Behind the character, in the sense the ALTERNATE angles triple
// holds -- +0x144, not +0xBC.
int VIWalkCamera::SetOrbitAngleBehind()
{
    return SetOrbitAngle(m_committed.m_angles.x + 3.14159274f);
}

// 0x00483360.
int VIWalkCamera::ChooseOrbitAngle(float a, VIScene &scene)
{
    float best;

    FindBestOrbitAngle(fmodf(a, 6.28318548f), scene, best);
    SetOrbitAngle(best);
    return 0;
}

// 0x004833C0.  Orbit input, and it only answers in the two orbiting modes.
int VIWalkCamera::OrbitRight(float t)
{
    VIVect3 dir;
    VIVect3 axis;
    float   a;

    if (m_cameraMode == 1 || m_cameraMode == 3)
    {
        t = VIMIN(1.0f, VIMAX(-1.0f, t));
        a = m_angleAccel;
        axis.x = 1.0f;
        axis.y = 0.0f;
        axis.z = 0.0f;
        dir.x += a * axis.x;
        dir.y += a * axis.y;
        dir.z += a * axis.z;
        AddVec(m_cur.m_orbitForce, dir, t);
    }
    return 0;
}

// 0x004834D0.  Zoom, and it carries OrbitRight's two stack vectors with it --
// declared, zeroed and never read.
int VIWalkCamera::OrbitIn(float t)
{
    VIVect3 dir;
    VIVect3 axis;

    if (m_cameraMode == 1 || m_cameraMode == 3)
    {
        float k = -VIMIN(1.0f, VIMAX(-1.0f, t));
        m_cur.m_distForce += m_distAccel * k;
    }
    return 0;
}

// 0x00483130.  Stop everything: the character body's four vectors, its second
// set, and the camera's own force and velocity.
int VIWalkCamera::Halt()
{
    m_committed.m_force.Zero();
    m_committed.m_velocity.Zero();
    m_committed.m_torque.Zero();
    m_committed.m_angularVel.Zero();
    m_last.m_orbitForce.Zero();
    m_last.m_orbitVel.Zero();
    m_last.m_distForce = 0.0f;
    m_last.m_distVel = 0.0f;
    m_phys.m_force.Zero();
    m_phys.m_velocity.Zero();
    m_phys.m_torque.Zero();
    m_phys.m_angularVel.Zero();
    m_cur.m_orbitForce.Zero();
    m_cur.m_orbitVel.Zero();
    m_cur.m_distForce = 0.0f;
    m_cur.m_distVel = 0.0f;
    return 0;
}

// 0x004837A0.
int VIWalkCamera::InitCameraIntermediate(VIScene &scene)
{
    float dt;
    float angleD, distD;

    dt = (float)m_tickMS * 0.001f;
    angleD = powf(m_angleDamping, dt);
    distD = powf(m_distDamping, dt);

    m_angleAccel = (m_defOrbitAngle - m_defOrbitAngle * angleD) / (angleD * dt);
    m_distAccel = (m_defOrbitDistance - m_defOrbitDistance * distD) /
                  (distD * dt);

    MoveCamera(scene);

    // Through a pointer to each half: the +0 field folds back onto `this` and
    // every other one comes off the pointer, which is the whole shape of the
    // target's copy.
    State *a = &m_cur;
    State *b = &m_last;

    b->m_time = a->m_time;
    b->m_flags = a->m_flags;
    b->m_orbit = a->m_orbit;
    b->m_orbitVel = a->m_orbitVel;
    b->m_orbitForce = a->m_orbitForce;
    b->m_dist = a->m_dist;
    b->m_distVel = a->m_distVel;
    b->m_distForce = a->m_distForce;
    b->m_cameraPos = a->m_cameraPos;
    b->m_snapAngle = a->m_snapAngle;
    b->m_snapPitch = a->m_snapPitch;
    return 0;
}

// 0x00482368.  Mode 3 is the only one that turns the character's own
// target-heading tracking on; every other mode clears it.
int VIWalkCamera::SetCameraMode(VIWalkCameraMode mode)
{
    m_cameraMode = mode;
    if (mode == 3)
    {
        // Both flag words are touched 32 BITS AT A TIME here -- `lw`/`sw` on
        // the low half of a field Phase 18 proved is a `long` -- and the clear
        // mask is one UNSIGNED constant shared by both, `lui 0xffff` + `ori`,
        // where a signed -257 would be a single `addiu`.
        m_phys.m_flags |= 0x100;
        m_committed.m_flags |= 0x100;
        m_phys.m_targetHeading = m_phys.m_angles.x;
        m_committed.m_targetHeading = m_committed.m_angles.x;
    }
    else
    {
        m_phys.m_flags &= ~0x100;
        m_committed.m_flags &= ~0x100;
    }
    return 0;
}

// 0x004821D8.  Put the character body there, then rebuild the committed orbit
// state from scratch and copy it BACK onto the live one -- the opposite
// direction from InitCameraIntermediate.
int VIWalkCamera::SetLocation(const VIVect3 &pos, const VIVect3 &angles,
                              VICamera &cam, VIScene &scene)
{
    VIWalkControl::SetLocation(pos, angles, scene);

    m_last.m_time = scene.GetTime();
    m_last.m_orbit.y = m_defOrbitPitch;
    m_last.m_orbit.x = 0.0f;
    m_last.m_flags = 0;
    m_last.m_orbitVel.Zero();
    m_last.m_orbitForce.Zero();
    m_last.m_distVel = 0.0f;
    m_last.m_distForce = 0.0f;
    m_last.m_cameraPos = pos;
    m_last.m_snapAngle = 0.0f;
    m_last.m_snapPitch = 0.0f;

    {
        State *a = &m_last;
        State *b = &m_cur;

        b->m_time = a->m_time;
        b->m_flags = a->m_flags;
        b->m_orbit = a->m_orbit;
        b->m_orbitVel = a->m_orbitVel;
        b->m_orbitForce = a->m_orbitForce;
        b->m_dist = a->m_dist;
        b->m_distVel = a->m_distVel;
        b->m_distForce = a->m_distForce;
        b->m_cameraPos = a->m_cameraPos;
        b->m_snapAngle = a->m_snapAngle;
        b->m_snapPitch = a->m_snapPitch;
    }

    InitCameraIntermediate(scene);
    SetCamera(cam, scene, 0);
    return 0;
}

// 0x00482090.
int VIWalkCamera::Init(const VIVect3 &pos, const VIVect3 &angles,
                       VICamera &cam, VIScene &scene)
{
    InitParms();

    // Read off the `lui` sequence (Phase 21), which is the only order this
    // block preserves: the stores are rescheduled past recognition.
    m_lookAtHeight = 1.68f;
    m_autoOrbit = 1;
    m_minOrbitDistance = 1.5f;
    m_angleDamping = 0.00999999978f;
    m_cameraMode = 0;
    m_defOrbitAngle = 1.57079637f;
    m_maxOrbitDistance = 25.0f;
    m_minCollisionDist = 3.0f;
    m_distSnapSpeed = 23.5f;
    m_collisionRadius = 0.5f;
    m_defOrbitDistance = 11.75f;
    m_defOrbitPitch = 0.523598790f;
    m_altOrbitPitch = 1.04719758f;
    m_targetHeight = 1.10000002f;
    m_lookAtHeightAbove = 1.68f;
    m_snapSpeed = 3.14159274f;
    m_last.m_dist = 1.5f;
    m_distDamping = 0.00999999978f;

    SetLocation(pos, angles, cam, scene);
    m_effect.Reset();
    return 0;
}

// 0x00482020.
int VIWalkCamera::Init(VICamera &cam, VIScene &scene)
{
    VIVect3 hpr;

    cam.m_dir.HPR(&hpr.x, &hpr.y, &hpr.z);
    return Init(cam.m_position, hpr, cam, scene);
}

// 0x00483208.  Set the orbit angle, and in third-person mode move the camera
// to the point that angle names straight away rather than letting the spring
// find it.  fmodf is called TWICE on the same argument -- once per copy.
int VIWalkCamera::SetOrbitAngle(float a)
{
    m_cur.m_orbit.x = fmodf(a, 6.28318548f);
    m_last.m_orbit.x = fmodf(a, 6.28318548f);

    if (m_cameraMode == 3)
    {
        VIVect3 dir;
        VIVect3 p;

        dir.HPR(m_cur.m_orbit.x, -m_cur.m_orbit.y, 0.0f);

        p = m_phys.m_position;
        p.x = p.x + dir.x * m_cur.m_dist;
        p.y = p.y + m_targetHeight + dir.y * m_cur.m_dist + m_phys.m_heightOffset;
        p.z = p.z + dir.z * m_cur.m_dist;

        m_last.m_cameraPos = p;
        m_cur.m_cameraPos = p;
    }
    return 0;
}

// The commit, written into both arms of Process and again in
// InitCameraIntermediate: there is no symbol for it, so it is an inline the
// compiler expanded everywhere.
static inline void Commit(VIWalkCamera *c)
{
    VIWalkCamera::State *b = &c->m_last;
    VIWalkCamera::State *a = &c->m_cur;

    b->m_time = a->m_time;
    b->m_flags = a->m_flags;
    b->m_orbit = a->m_orbit;
    b->m_orbitVel = a->m_orbitVel;
    b->m_orbitForce = a->m_orbitForce;
    b->m_dist = a->m_dist;
    b->m_distVel = a->m_distVel;
    b->m_distForce = a->m_distForce;
    b->m_cameraPos = a->m_cameraPos;
    b->m_snapAngle = a->m_snapAngle;
    b->m_snapPitch = a->m_snapPitch;
}

// 0x004823D0.  Run the character body, re-derive the orbit angle from where
// the camera actually ENDED UP last frame, then catch the orbit body up to the
// scene clock.  The re-derivation flattens the offset in y first, so the
// heading is the ground-plane one.
int VIWalkCamera::Process(VICamera &cam, VIScene &scene, int flags)
{
    long dt;
    long step;

    VIWalkControl::Process(0, scene);

    if (m_cameraMode == 3)
    {
        VIVect3 d;
        float   p;
        float   r;

        d = m_cur.m_cameraPos;
        d -= m_phys.m_position;
        d.y = 0.0f;
        d.HPR(&m_cur.m_orbit.x, &p, &r);
    }

    dt = scene.GetTime() - m_last.m_time;
    m_effect.Update((float)dt * 0.001f);

    if (dt == 0)
    {
        MoveCamera(scene);
        Commit(this);
    }
    else
    {
        while (dt > 0)
        {
            if (m_tickAltMS < dt)
            {
                step = dt - m_tickAltMS;
                m_cur.m_time = m_last.m_time + step;
            }
            else
            {
                step = VIMIN((long)m_tickMS, dt);
                m_cur.m_time = m_last.m_time + step;
                UpdateCameraPhysics((float)(int)step * 0.001f);
                MoveCamera(scene);
            }

            m_last.m_time = m_cur.m_time;
            Commit(this);
            dt -= step;
        }
    }

    ClearCameraForces();
    SetCamera(cam, scene, flags);
    return 0;
}

// 0x00482758.  The orbit body's step, and it is NOT VIWalkControl's: the
// angular part is done on x alone -- the damping vector's y and z are never
// written, so the pitch and roll velocities are multiplied by ZERO every
// frame -- while the distance is a scalar body of its own with two snap modes
// over it.
int VIWalkCamera::UpdateCameraPhysics(float dt)
{
    VIVect3 damp;

    damp.x = powf(m_angleDamping, dt);

    m_cur.m_orbit.x = m_cur.m_orbit.x +
                      m_cur.m_orbitForce.x * dt * dt * damp.x +
                      m_cur.m_orbitVel.x * dt * damp.x;
    AddVec(m_cur.m_orbitVel, m_cur.m_orbitForce, dt);
    MulVec(m_cur.m_orbitVel, damp);

    if (m_cur.m_flags & 1)
    {
        float tgt = m_cur.m_snapAngle;

        // Phase 18's overshoot shape, verbatim: the MEMBERS carry the
        // arithmetic and the local only ever holds the wrapped result.  And
        // `cur` is taken AFTER the wrap: the wrap's subtraction then reads
        // the loaded value and `cur` is a COPY of it that lives on (the
        // target's `mov.s $f3,$f1`); declared first, cur IS the load and
        // every register after it shifts (52 wrong words to 2).
        if (3.14159274f < m_cur.m_snapAngle - m_cur.m_orbit.x)
            tgt = m_cur.m_snapAngle - 6.28318548f;
        else if (m_cur.m_snapAngle - m_cur.m_orbit.x < -3.14159274f)
            tgt = m_cur.m_snapAngle + 6.28318548f;

        float cur = m_cur.m_orbit.x;

        if (cur < tgt)
        {
            m_cur.m_orbit.x = cur + m_snapSpeed * dt;
            if (tgt < m_cur.m_orbit.x)
            {
                m_cur.m_orbit.x = tgt;
                m_cur.m_flags &= ~1;
                m_phys.m_flags &= ~0x200;
            }
        }
        else if (tgt <= cur)
        {
            m_cur.m_orbit.x = cur - m_snapSpeed * dt;
            if (m_cur.m_orbit.x <= tgt)
            {
                m_cur.m_orbit.x = tgt;
                m_cur.m_flags &= ~1;
                m_phys.m_flags &= ~0x200;
            }
        }
    }

    m_cur.m_orbit.x = fmodf(m_cur.m_orbit.x, 6.28318548f);

    if (m_cur.m_flags & 8)
    {
        // The target first, then the current pitch: the two loads come out
        // in declaration order (the last 2 words).
        float tgt = m_cur.m_snapPitch;
        float cur = m_cur.m_orbit.y;

        if (cur < tgt)
        {
            m_cur.m_orbit.y = cur + m_snapSpeed * dt;
            if (tgt < m_cur.m_orbit.y)
            {
                m_cur.m_orbit.y = tgt;
                m_cur.m_flags &= ~8;
            }
        }
        else if (tgt <= cur)
        {
            m_cur.m_orbit.y = cur - m_snapSpeed * dt;
            if (m_cur.m_orbit.y <= tgt)
            {
                m_cur.m_orbit.y = tgt;
                m_cur.m_flags &= ~8;
            }
        }
    }

    damp.x = powf(m_distDamping, dt);

    m_cur.m_dist = m_cur.m_dist +
                   m_cur.m_distForce * dt * dt * damp.x +
                   m_cur.m_distVel * dt * damp.x;
    m_cur.m_distVel += m_cur.m_distForce * dt;
    m_cur.m_distVel *= damp.x;

    if (m_cur.m_flags & 2)
    {
        m_cur.m_dist = m_cur.m_dist - m_distSnapSpeed * dt;
        if (m_cur.m_dist <= m_minOrbitDistance)
            m_cur.m_flags &= ~2;
    }
    else if (m_cur.m_flags & 4)
    {
        m_cur.m_dist = m_cur.m_dist - m_distSnapSpeed * dt;
        if (m_cur.m_dist <= m_minCollisionDist)
        {
            m_cur.m_dist = m_minCollisionDist;
            m_cur.m_flags &= ~4;
        }
    }

    m_cur.m_dist = VIMIN(m_maxOrbitDistance,
                         VIMAX(m_minOrbitDistance, m_cur.m_dist));
    return 0;
}

// 0x00482AF0.  Place the camera.  In mode 3 the orbit is swept against the
// world first: a clear sweep at the default pitch snaps the pitch back to it;
// a blocked one zooms in while there is distance to give, then tries the
// alternate pitch, and only then hunts for another orbit angle (zooming to
// the minimum when there is none).  Modes 0-2 sweep straight from the target
// to the eye, and only m_autoOrbit lets the hit point move the eye in.
int VIWalkCamera::MoveCamera(VIScene &scene)
{
    VICollSphere       sphere;
    VIWalkCameraFilter filter;
    VIVect3            from;
    VIVect3            to;
    VIVect3            dir;

    if (m_cameraMode == 3)
    {
        if (!(m_cur.m_bits.snapAngle || m_cur.m_bits.zoomIn ||
              m_cur.m_bits.snapPitch))
        {
            from = m_phys.m_position;
            from.y = from.y + m_targetHeight;
            // Read ONCE: the second sweep passes the same value after the
            // first HPR has rewritten dir.
            float roll = dir.z;
            dir.HPR(m_cur.m_orbit.x, -m_defOrbitPitch, roll);

            to = m_phys.m_position;
            to.y = to.y + m_targetHeight;
            AddVec(to, dir, m_cur.m_dist);
            from.y = from.y + m_phys.m_heightOffset;
            to.y = to.y + m_phys.m_heightOffset;

            if (sphere.Init(from, to, m_collisionRadius) < 0)
                return -1;
            filter.Init(m_parm64);
            if (scene.Collide(sphere, filter) == 0)
            {
                if (m_cur.m_orbit.y != m_defOrbitPitch)
                {
                    m_cur.m_snapPitch = m_defOrbitPitch;
                    m_cur.m_flags |= 8;
                }
            }
            else if (m_cur.m_dist > m_minCollisionDist)
                m_cur.m_flags |= 4;
            else
            {
                from = m_phys.m_position;
                from.y = from.y + m_targetHeight;
                dir.HPR(m_cur.m_orbit.x, -m_altOrbitPitch, roll);

                to = m_phys.m_position;
                to.x = to.x + dir.x * m_cur.m_dist;
                to.y = to.y + m_targetHeight + dir.y * m_cur.m_dist;
                to.z = to.z + dir.z * m_cur.m_dist;
                from.y = from.y + m_phys.m_heightOffset;
                to.y = to.y + m_phys.m_heightOffset;

                if (sphere.Init(from, to, m_collisionRadius) < 0)
                    return -1;
                if (scene.Collide(sphere, filter) == 0 &&
                    m_phys.m_landscapeNormal.y < 0.9f)
                {
                    if (m_cur.m_orbit.y != m_altOrbitPitch)
                    {
                        m_cur.m_snapPitch = m_altOrbitPitch;
                        m_cur.m_flags |= 8;
                    }
                }
                else if (FindOrbitAngle(m_cur.m_orbit.x, scene,
                                        m_cur.m_snapAngle) == 0)
                    m_cur.m_flags |= 2;
                else if (m_cur.m_orbit.x != m_cur.m_snapAngle)
                {
                    m_cur.m_flags |= 1;
                    m_phys.m_flags |= 0x200;
                }
            }
        }

        dir.HPR(m_cur.m_orbit.x, -m_cur.m_orbit.y, 0.0f);
        m_cur.m_cameraPos = m_phys.m_position;
        m_cur.m_cameraPos.y = m_cur.m_cameraPos.y + m_targetHeight;
        AddVec(m_cur.m_cameraPos, dir, m_cur.m_dist);
        m_cur.m_cameraPos.y = m_cur.m_cameraPos.y + m_phys.m_heightOffset;
    }
    else
    {
        if (m_cameraMode == 2)
        {
            from = m_phys.m_position;
            from.y = from.y + m_targetHeight;
            dir.HPR(m_phys.m_angles.x, m_phys.m_angles.y, dir.z);

            to = m_phys.m_position;
            to.y = to.y + m_targetHeight;
            SubVec(to, dir, m_cur.m_dist);
        }
        else
        {
            from = m_phys.m_position;
            from.y = from.y + m_targetHeight;
            dir.HPR(m_cur.m_orbit.x, -m_cur.m_orbit.y, dir.z);

            to = m_phys.m_position;
            to.y = to.y + m_targetHeight;
            AddVec(to, dir, m_cur.m_dist);
        }
        from.y = from.y + m_phys.m_heightOffset;
        to.y = to.y + m_phys.m_heightOffset;

        if (m_autoOrbit)
        {
            if (sphere.Init(from, to, m_collisionRadius) < 0)
                return -1;
            filter.Init(m_parm64);
            scene.Collide(sphere, filter);
            m_cur.m_cameraPos = sphere.GetHitPoint();
        }
        else
            m_cur.m_cameraPos = to;
    }
    return 0;
}

// 0x00483578.  Turn the orbit state into an eye point and a look direction and
// hand them to the camera, through the shake.  The four modes are an if/else
// chain tested 0, 2, then 1-or-3 -- not a switch.
int VIWalkCamera::SetCamera(VICamera &cam, VIScene &scene, int flags)
{
    VIVect3 pos;
    VIVect3 dir;

    if (m_cameraMode == 0)
    {
        pos = m_phys.m_position;
        pos.y = pos.y + m_lookAtHeight;
        dir.HPR(m_phys.m_angles.x, m_phys.m_angles.y, 0.0f);
    }
    else if (m_cameraMode == 2)
    {
        pos = m_cur.m_cameraPos;
        dir.HPR(m_phys.m_angles.x, m_phys.m_angles.y, 0.0f);
    }
    else if (m_cameraMode == 1 || m_cameraMode == 3)
    {
        pos = m_cur.m_cameraPos;
        dir = m_phys.m_position;
        dir.y = dir.y + m_phys.m_heightOffset + m_lookAtHeightAbove;
        dir -= m_cur.m_cameraPos;
    }

    {
        VIVect3 p = pos;
        VIVect3 d = dir;

        m_effect.Apply(p, d);
        if (flags)
            cam(*scene.m_raster, p, d, scene.CalcFarPlane(pos));
        else
            cam(p, d);
    }
    return 0;
}

// 0x00483910.  Sweep the camera sphere from the character out to the orbit
// distance at eight candidate angles -- the caller's, then +-45, +-90, +-135
// and 180 degrees off it -- and answer with the first one that is clear.
int VIWalkCamera::FindOrbitAngle(float a, VIScene &scene, float &out)
{
    VIVect3            from;
    VIVect3            to;
    VIVect3            dir;
    float              angles[8];
    VICollSphere       sphere;
    VIWalkCameraFilter filter;
    int                i;

    angles[0] = a;
    angles[1] = fmodf(a + 0.785398185f, 6.28318548f);
    angles[2] = fmodf(a - 0.785398185f, 6.28318548f);
    angles[3] = fmodf(a + 1.57079637f, 6.28318548f);
    angles[4] = fmodf(a - 1.57079637f, 6.28318548f);
    angles[5] = fmodf(a + 2.35619450f, 6.28318548f);
    angles[6] = fmodf(a - 2.35619450f, 6.28318548f);
    angles[7] = fmodf(a + 3.14159274f, 6.28318548f);

    out = angles[0];

    for (i = 0; i < 8; i++)
    {
        from = m_committed.m_position;
        from.y = from.y + m_targetHeight;
        dir.HPR(angles[i], -m_last.m_orbit.y, 0.0f);

        to = m_committed.m_position;
        to.x = to.x + dir.x * m_last.m_dist;
        to.y = to.y + m_targetHeight + dir.y * m_last.m_dist;
        to.z = to.z + dir.z * m_last.m_dist;
        // The step-up offset goes on FROM first and TO second: the other order
        // is the same 172 instructions with two registers renamed.
        from.y = from.y + m_committed.m_heightOffset;
        to.y = to.y + m_committed.m_heightOffset;

        if (sphere.Init(from, to, m_collisionRadius) < 0)
            return -1;

        filter.Init(m_parm64);
        if (scene.Collide(sphere, filter) == 0)
        {
            out = angles[i];
            break;
        }
    }

    return i != 8;
}

// 0x00483BC0.  The same eight-angle sweep, but instead of taking the first
// clear angle it keeps the one whose sweep got FURTHEST -- and takes a clear
// one immediately if it finds it.
int VIWalkCamera::FindBestOrbitAngle(float a, VIScene &scene, float &out)
{
    VIVect3            from;
    VIVect3            to;
    VIVect3            dir;
    float              angles[8];
    VICollSphere       sphere;
    VIWalkCameraFilter filter;
    int                i;
    int                best;
    float              bestDist;

    best = -1;
    bestDist = 0.0f;

    angles[0] = a;
    angles[1] = fmodf(a + 0.785398185f, 6.28318548f);
    angles[2] = fmodf(a - 0.785398185f, 6.28318548f);
    angles[3] = fmodf(a + 1.57079637f, 6.28318548f);
    angles[4] = fmodf(a - 1.57079637f, 6.28318548f);
    angles[5] = fmodf(a + 2.35619450f, 6.28318548f);
    angles[6] = fmodf(a - 2.35619450f, 6.28318548f);
    angles[7] = fmodf(a + 3.14159274f, 6.28318548f);

    for (i = 0; i < 8; i++)
    {
        from = m_committed.m_position;
        from.y = from.y + m_targetHeight;
        dir.HPR(angles[i], -m_last.m_orbit.y, 0.0f);

        to = m_committed.m_position;
        to.x = to.x + dir.x * m_last.m_dist;
        to.y = to.y + m_targetHeight + dir.y * m_last.m_dist;
        to.z = to.z + dir.z * m_last.m_dist;
        from.y = from.y + m_committed.m_heightOffset;
        to.y = to.y + m_committed.m_heightOffset;

        if (sphere.Init(from, to, m_collisionRadius) < 0)
            return -1;

        filter.Init(m_parm64);
        if (scene.Collide(sphere, filter) == 0)
        {
            best = i;
            break;
        }

        if (best == -1 || bestDist < sphere.GetHitDistance())
        {
            best = i;
            bestDist = sphere.GetHitDistance();
        }
    }

    out = angles[best];
    return 0;
}

// 0x004848D8.
int VIWalkCameraFilter::FilterActor(VIScene &scene, int actor)
{
    // Phase 37 closes this: it is one `||` whose body RETURNS 1, not an `&&`
    // guard returning 0.  Either `&&` spelling costs two words -- the second
    // test if-converts to xor+sltiu, or the two returns stop cross-jumping --
    // and the accepting direction is the one the target branches on.
    if (scene.GetActorSpriteType(actor) == 5 || actor == m_actor)
        return 1;
    return 0;
}
