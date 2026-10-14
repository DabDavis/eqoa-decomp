// VIZoneRoom -- SUPPORT 0x00499778 .. 0x00499EC0.  A room and the portals that
// leave it.  One `new char[]` holds a 32-byte portal record per portal followed
// by the portals' vertices; the two section pointers are byte OFFSETS until
// Init rebases them onto the block, which is VISurface's trick and VIPrimBuffer's.
#include "vi.h"

VIZoneRoom::VIZoneRoom()
{
    m_data = 0;
    Clear();
}

VIZoneRoom::~VIZoneRoom()
{
    Clear();
}

int VIZoneRoom::Clear()
{
    if (m_a3C.Clear() < 0)
        return -1;
    if (m_a44.Clear() < 0)
        return -1;
    if (m_data)
        delete [] (char *)m_data;
    m_flags = 0;
    m_bbox.min.Zero();
    m_bbox.max.Zero();
    m_data = 0;
    m_p1C = -1;
    m_p20 = 0;
    m_p24 = 0;
    m_p28 = 0;
    m_p2C = -1;
    m_p30 = 0;
    m_p34 = -1;
    m_p38 = -1;
    m_numPortals = 0;
    m_maxPortals = 0;
    m_numVerts = 0;
    m_maxVerts = 0;
    m_portals = 0;
    m_verts = 0;
    m_destRoom = -1;
    m_p68 = -1;
    m_dataSize = 0;
    m_p6C = -1;
    return 0;
}

int VIZoneRoom::CalcDataSize()
{
    m_portals = 0;
    m_verts = m_maxPortals * sizeof(VIZonePortal);
    m_dataSize = m_maxVerts * sizeof(VIVect3) + m_maxPortals * sizeof(VIZonePortal);
    return 0;
}

int VIZoneRoom::Allocate()
{
    if (m_dataSize)
    {
        m_data = new char[m_dataSize];
        if (m_data == 0)
            return -1;
    }
    return 0;
}

int VIZoneRoom::Init(int maxPortals, int maxVerts, unsigned int a,
                     unsigned int b, VIBBox &bb)
{
    m_flags |= 2;
    m_bbox = bb;
    m_numPortals = 0;
    m_numVerts = 0;
    m_p1C = -1;
    m_p28 = a;
    m_p30 = b;
    m_maxPortals = maxPortals;
    m_maxVerts = maxVerts;
    if (CalcDataSize() < 0)
        return -1;
    if (Allocate() < 0)
        return -1;
    m_portals = (int)m_data + m_portals;
    m_verts = (int)m_data + m_verts;
    return 0;
}

int VIZoneRoom::DestRoom(int i)
{
    m_destRoom = i;
    return 0;
}

int VIZoneRoom::Begin()
{
    ((VIZonePortal *)m_portals)[m_numPortals].m_destRoom = m_destRoom;
    ((VIZonePortal *)m_portals)[m_numPortals].m_numVerts = 0;
    ((VIZonePortal *)m_portals)[m_numPortals].m_firstVert = m_numVerts;
    ((VIZonePortal *)m_portals)[m_numPortals].m_p1C = 0;
    return 0;
}

int VIZoneRoom::Vertex(VIVect3 &v)
{
    ((VIVect3 *)m_verts)[m_numVerts] = v;
    m_numVerts++;
    ((VIZonePortal *)m_portals)[m_numPortals].m_numVerts++;
    return 0;
}

int VIZoneRoom::End()
{
    CalcPortalPlane(m_numPortals);
    m_numPortals++;
    return 0;
}

// Newell's method over the portal's vertex ring, with the centroid accumulated
// in the same loop: the plane's distance is the normal dotted into the average
// vertex, negated.  A degenerate ring -- Normalize returning zero -- gets d = 0
// and keeps whatever normal Normalize left behind.
// OPEN, 22/139 (wave 6).  The target zeroes prev, last, sum and THEN p (sched1
// ties go by source order after prev's WAW dependents), yet p sits at sp+0 and
// last/prev/sum at 16/32/48 -- declared first, constructed last.  Declaration
// order moves the slots (p first scores best of all 24), the ctor insns
// follow it; `(VIZonePortal *)(i * 32 + (int)m_portals)` fixes the portals/
// verts load order but nets 24.
int VIZoneRoom::CalcPortalPlane(int i)
{
    VIPlane p;
    VIVect3 last;
    VIVect3 prev;
    VIVect3 sum;
    int j;
    VIZonePortal *pt = &((VIZonePortal *)m_portals)[i];
    VIVect3 *v = (VIVect3 *)m_verts;
    int first = pt->m_firstVert;
    int end = first + pt->m_numVerts;
    // Phase 21's fresh intermediate, in integer code: written `v[end - 1]` the
    // -1 is distributed into the address as `end*12 + base - 12`, where the
    // target multiplies the decremented index.
    int lastIdx = end - 1;

    prev = v[lastIdx];
    for (j = first; j != end; j++)
    {
        last = prev;
        prev = v[j];
        p.n.x = p.n.x + (prev.y - last.y) * (prev.z + last.z);
        p.n.y = p.n.y + (prev.z - last.z) * (prev.x + last.x);
        p.n.z = p.n.z + (prev.x - last.x) * (prev.y + last.y);
        sum.x = sum.x + prev.x;
        sum.y = sum.y + prev.y;
        sum.z = sum.z + prev.z;
    }

    if ((float)p.n.Normalize() == 0.0f)
    {
        p.d = 0.0f;
    }
    else
    {
        float k = 1.0f / (float)((VIZonePortal *)m_portals)[i].m_numVerts;

        sum.x = sum.x * k;
        sum.y = sum.y * k;
        sum.z = sum.z * k;
        p.d = -(p.n.x * sum.x + p.n.y * sum.y + p.n.z * sum.z);
    }

    ((VIZonePortal *)m_portals)[i].m_plane = p;
    return 0;
}
