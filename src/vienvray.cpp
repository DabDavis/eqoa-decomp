// VIEnvRay / VITransEnvRay -- SUPPORT 0x00430FC8 .. 0x00431290.
#include "vi.h"

VIEnvRay::VIEnvRay()
{
    m_firstCollide = 1;
    m_mask = 15;
    m_hits = 0;
}

int VIEnvRay::Init(const VIVect3 &a, const VIVect3 &b, unsigned int mask)
{
    m_start.x = a.x;
    m_start.y = a.y;
    m_start.z = a.z;

    m_end = b;
    m_delta = m_end;
    m_delta -= m_start;

    m_length = m_delta.NormalizeMagn();

    if (m_length != 0.0f)
        m_invLength = 1.0f / m_length;
    else
        m_invLength = 0.0f;

    m_mask = mask;
    m_hitTime = 1.0f;
    m_hits = 0;
    m_firstCollide = 1;

    return 0;
}

int VIEnvRay::Collide(unsigned int mask, float t, VIVect3 &n)
{
    return (mask & m_mask) != 0;
}

VIVect3 VIEnvRay::GetHitPoint() const
{
    VIVect3 p;

    p = m_end;

    // Phase 11's compound-operator lever: written out component by component
    // the same arithmetic is 14 words wrong, whichever order the three
    // statements are in (all six measured).
    p -= m_start;
    p *= m_hitTime;
    p += m_start;

    return p;
}

float VIEnvRay::GetHitTime() const
{
    return m_hitTime;
}

int VITransEnvRay::Collide(unsigned int mask, float t, VIVect3 &n)
{
    return m_firstCollide == 0;
}

int VIEnvRay::IsFirstCollide() const
{
    return m_firstCollide;
}
