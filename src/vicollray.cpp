// VICollRay -- SUPPORT 0x00420440 .. 0x00420B60.
//
// The first collision primitive, and the first class whose SEMANTICS were
// already settled before the decomp started: `go-eqoa-vi/vicoll/ray.go` is a
// bit-exact Go port of exactly these functions, checked against the console.
// So a mismatch here is codegen, never a misreading -- the VIMatrix44
// situation again, one layer up.
#include "vi.h"

VICollRay::VICollRay()
{
}

int VICollRay::Precalc()
{
    m_delta = m_end;
    m_delta -= m_start;
    m_dir = m_delta;
    float l = m_dir.Magnitude();
    m_length = l;
    // The zero-length block is the one gcc moves out of line, so it is the
    // ELSE: written the other way round the two blocks swap and the compare
    // comes out bc1f.
    if (l != 0.0f)
    {
        float inv = 1.0f / l;
        m_invLength = inv;
        m_dir *= inv;
    }
    else
    {
        m_invLength = 0.0f;
    }
    return 0;
}

int VICollRay::Init(const VIVect3 &start, const VIVect3 &end)
{
    m_start = start;
    m_end = end;
    Precalc();
    m_hitIdx = -1;
    m_hitTime = 1.0f;
    m_hitNormal.Zero();
    m_dir2 = m_dir;
    return 0;
}

int VICollRay::Init(const VICollRay &src, const VIMatrix44 &mat)
{
    VIVect4 a, b;
    a.Set(src.m_start * mat);
    m_start.x = a.x;
    m_start.y = a.y;
    m_start.z = a.z;
    // The SAME local, not a second one: `a` is address-taken (it is call 3's
    // argument), so gcc cannot kill the first Set's stores across call 2 --
    // but it does kill the second Set's, which are overwritten in the same
    // block.  Using `b` here makes three stores appear that the target has not
    // got.
    a.Set(src.m_end * mat);
    m_end.x = a.x;
    m_end.y = a.y;
    m_end.z = a.z;
    a.x = src.m_dir2.x;
    a.y = src.m_dir2.y;
    a.z = src.m_dir2.z;
    a.w = 0.0f;
    b.Set(a * mat);
    b.Normalize();
    m_dir2.x = b.x;
    m_dir2.y = b.y;
    m_dir2.z = b.z;
    Precalc();
    m_hitTime = src.m_hitTime;
    m_hitIdx = src.m_hitIdx;
    m_hitNormal = src.m_hitNormal;
    return 0;
}

static inline float Dot(const VIVect3 &a, const VIVect3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

int VICollRay::Skitter(VIVect3 &v)
{
    int hit = 0;
    VIVect3 t0, t1;
    if (m_hitTime < 1.0f)
    {
        // Every stage is `out = in; d = -dot(axis,in); out = in + axis*d;`
        // and the dot reads the SOURCE MEMBER through an inline helper, not
        // the copy: the helper's reference to m_hitNormal / m_dir2 is an
        // unflagged pseudo, so its y and z are re-read after every store to
        // the stack copy, and the last dot's loads stay behind t0's stores
        // (claude-notes/decomp-alias-rule-inline-ref-params-20261014.md).
        // A named `const VIVect3 &` or a dot over the copy folds them: 91/201.
        t1 = m_hitNormal;
        float d1 = -Dot(v, m_hitNormal);
        t1.x += v.x * d1;
        t1.y += v.y * d1;
        t1.z += v.z * d1;
        if (t1.Normalize())
        {
            t0 = m_dir2;
            float d2 = -Dot(t1, m_dir2);
            t0.x += t1.x * d2;
            t0.y += t1.y * d2;
            t0.z += t1.z * d2;
            if (t0.Normalize())
            {
                t0.x += t1.x * 0.01f;
                t0.y += t1.y * 0.01f;
                t0.z += t1.z * 0.01f;
                if (t0.Normalize())
                {
                    float d3 = -Dot(v, t0);
                    t0.x += v.x * d3;
                    t0.y += v.y * d3;
                    t0.z += v.z * d3;
                    float dist = (1.0f - m_hitTime) * Dot(t0, m_dir2) * m_length;
                    if (0.0f < dist)
                    {
                        VIVect3 hp = GetHitPoint();
                        hit = 1;
                        // m_start first: m_end's +4/+8 stores go through
                        // operator='s unflagged `this` and may alias m_start,
                        // so only this order lets sched pull m_start's stores
                        // ahead of m_end.y the way the guest has them.
                        m_start = hp;
                        m_end = t0;
                        m_end *= dist;
                        m_end += m_start;
                        Precalc();
                        m_hitTime = 1.0f;
                        m_hitIdx = -1;
                        m_hitNormal.Zero();
                    }
                }
            }
        }
    }
    return hit;
}

VIVect3 VICollRay::GetHitPoint()
{
    VIVect3 p;
    p = m_delta;
    p.x = p.x * m_hitTime + m_start.x;
    p.y = p.y * m_hitTime + m_start.y;
    p.z = p.z * m_hitTime + m_start.z;
    return p;
}

VIVect3 &VICollRay::GetHitNormal()
{
    return m_hitNormal;
}

float VICollRay::GetHitDistance()
{
    return m_hitTime * m_length;
}

float VICollRay::GetHitTime()
{
    return m_hitTime;
}

// ---------------------------------------------------------------- VICollSphere
//
// A VICollRay with a radius.  Its Init and Precalc are the base's bodies
// COPIED, with the radius work appended -- the base versions are out of line,
// so a call would be visible as a jal and neither has one.

VICollSphere::VICollSphere()
{
}

int VICollSphere::Precalc()
{
    m_delta = m_end;
    m_delta -= m_start;
    m_dir = m_delta;
    float l = m_dir.Magnitude();
    m_length = l;
    if (l != 0.0f)
    {
        float inv = 1.0f / l;
        m_invLength = inv;
        m_dir *= inv;
    }
    else
    {
        m_invLength = 0.0f;
    }
    if (m_radius == 0.0f)
        m_invRadius = 0.0f;
    else
        m_invRadius = 1.0f / m_radius;
    return 0;
}

int VICollSphere::Init(VIVect3 &start, VIVect3 &end, float radius)
{
    m_start = start;
    m_end = end;
    m_radius = radius;
    Precalc();
    m_hitIdx = -1;
    m_hitTime = 1.0f;
    m_hitNormal.Zero();
    m_dir2 = m_dir;
    return 0;
}

int VICollSphere::Init(const VICollSphere &src, const VIMatrix44 &mat)
{
    VIVect4 a, b;
    a.Set(src.m_start * mat);
    m_start.x = a.x;
    m_start.y = a.y;
    m_start.z = a.z;
    a.Set(src.m_end * mat);
    m_end.x = a.x;
    m_end.y = a.y;
    m_end.z = a.z;
    // The radius is scaled by the length of the matrix's first row, and the
    // square root is INLINE sqrtf with the NaN retest -- not VIVect3's
    // out-of-line Magnitude, which would be a jal.
    m_radius = src.m_radius *
        sqrtf(mat.m[0].x * mat.m[0].x + mat.m[0].y * mat.m[0].y + mat.m[0].z * mat.m[0].z);
    a.x = src.m_dir2.x;
    a.y = src.m_dir2.y;
    a.z = src.m_dir2.z;
    a.w = 0.0f;
    b.Set(a * mat);
    b.Normalize();
    m_dir2.x = b.x;
    m_dir2.y = b.y;
    m_dir2.z = b.z;
    Precalc();
    m_hitTime = src.m_hitTime;
    m_hitIdx = src.m_hitIdx;
    m_hitNormal = src.m_hitNormal;
    return 0;
}
