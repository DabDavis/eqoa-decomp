// VIPickSegment -- SUPPORT 0x00412900 .. 0x00412D18.
//
// The ray's sibling: the same segment/direction/length front half, but the hit
// state carries a material id and a colour instead of a second direction copy,
// and Precalc -- not Init -- is what resets it.
#include "vi.h"

VIPickSegment::VIPickSegment()
{
    m_hitMaterial = -1;
}

int VIPickSegment::Precalc()
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
    m_hitTime = 1.0f;
    m_hitMaterial = -1;
    return 0;
}

int VIPickSegment::Init(const VIVect3 &start, const VIVect3 &end)
{
    m_start = start;
    m_end = end;
    m_hitNormal.Zero();
    m_hitColor.Set(0.0f, 0.0f, 0.0f, 1.0f);
    // Phase 9's shape again: the BRANCHING form is what gcc if-converts into
    // slti+movz.  Every ternary spelling gives slt+movn instead -- 6 words
    // wrong, and none of them is the source.
    if (Precalc() < 0)
        return -1;
    return 0;
}

VIVect3 VIPickSegment::GetHitPoint()
{
    VIVect3 p;
    p = m_delta;
    p.x = p.x * m_hitTime + m_start.x;
    p.y = p.y * m_hitTime + m_start.y;
    p.z = p.z * m_hitTime + m_start.z;
    return p;
}

VIVect3 VIPickSegment::GetHitNormal()
{
    return m_hitNormal;
}

float VIPickSegment::GetHitDistance()
{
    return m_hitTime * m_length;
}

float VIPickSegment::GetHitTime()
{
    return m_hitTime;
}

int VIPickSegment::GetHitMaterial()
{
    return m_hitMaterial;
}

VIColor32F &VIPickSegment::GetHitColor()
{
    return m_hitColor;
}

int VIPickSegment::Init(const VIPickSegment &src, const VIMatrix44 &mat)
{
    VIVect4 a, b;
    a.Set(src.m_start * mat);
    b.Set(src.m_end * mat);
    m_start.x = a.x;
    m_start.y = a.y;
    m_start.z = a.z;
    m_end.x = b.x;
    m_end.y = b.y;
    m_end.z = b.z;
    mat.SubMatrixMul(src.m_hitNormal, m_hitNormal);
    m_hitNormal.Normalize();
    m_hitColor = src.m_hitColor;
    if (Precalc() < 0)
        return -1;
    m_hitTime = src.m_hitTime;
    m_hitMaterial = src.m_hitMaterial;
    return 0;
}
