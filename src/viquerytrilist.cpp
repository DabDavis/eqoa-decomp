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
