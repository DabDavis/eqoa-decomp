// VIQueryTriList -- SUPPORT 0x0045D9B8 .. 0x0045DF38.
#include "vi.h"

VIQueryTriList::VIQueryTriList()
{
    m_numTris = 0;
    m_maxTris = 0;
    m_tris = 0;
}

VIQueryTriList::~VIQueryTriList()
{
    Clear();
}

int VIQueryTriList::Init(int n, const VIBBox &b, const VIVect3 &dir, float angle)
{
    m_numTris = 0;
    m_maxTris = n;

    if (Allocate() < 0)
        return -1;

    if (m_clipper.Init(9) < 0)
        return -1;

    if (Volume(b) < 0)
        return -1;

    m_dir = dir;
    m_cosAngle = cosf(angle * 0.017453292f);

    return 0;
}

int VIQueryTriList::Reset(const VIBBox &b, const VIVect3 &dir, float angle)
{
    m_numTris = 0;

    if (Volume(b) < 0)
        return -1;

    m_dir = dir;
    m_cosAngle = cosf(angle * 0.017453292f);

    return 0;
}

int VIQueryTriList::Clear()
{
    if (m_tris)
    {
        delete [] m_tris;
        m_tris = 0;
    }

    m_numTris = 0;
    m_maxTris = 0;

    return 0;
}

int VIQueryTriList::Volume(const VIBBox &b)
{
    return b.StoreOutward(m_planes);
}

int VIQueryTriList::Allocate()
{
    if (m_maxTris)
    {
        m_tris = new VIQueryTri[m_maxTris];

        if (m_tris == 0)
            return -1;
    }

    return 0;
}

// The view direction through a reference: its +0 folds onto `this`, +4/+8
// come off a register.
static inline float Dot(const VIVect3 &a, const VIVect3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 0x0045DC50.  Keep a triangle that faces the query direction closely enough,
// clipped to the query box, as a fan.  Every path returns 0 -- the caller
// reads m_numTris, not the result.
int VIQueryTriList::Intersect(int actor, const VIVect3 &a, const VIVect3 &b,
                              const VIVect3 &c)
{
    VIVect3 tri[3];
    VIPlane pl;
    int numOut;
    VIVect3 *out;

    if (m_numTris == m_maxTris)
        return 0;

    tri[0] = a;
    tri[1] = b;
    tri[2] = c;
    pl(a, b, c);
    if (Dot(pl.n, m_dir) < m_cosAngle)
        return 0;
    if (m_clipper.Clip(3, tri, 6, m_planes, numOut, &out) == 0)
        return 0;

    for (int i = 2; i < numOut && m_numTris < m_maxTris; i++)
    {
        m_tris[m_numTris].m_actor = actor;
        m_tris[m_numTris].m_v0 = out[0];
        m_tris[m_numTris].m_v1 = out[i - 1];
        m_tris[m_numTris].m_v2 = out[i];
        m_numTris++;
    }
    return 0;
}
