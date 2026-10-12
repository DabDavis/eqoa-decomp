// VIClipStack -- SUPPORT 0x0041E818 .. 0x0041FFA0.
//
// The portal / frustum clip stack: a stack of clip volumes, each a run of
// planes and a run of vertices in two flat VIArrays, with an entry array
// recording where each volume's runs start and how long they are.  Push*
// appends, Pop rewinds the two running totals by the top entry's counts, and
// the Cull/Clip pair test against the top volume alone.
//
// The layout comes straight off GetEntry: the entry array is indexed by 16,
// the vertex array by 12 and the plane array by 16, and the three running
// counts sit one word IN FRONT of the VIArray they count.
#include "vi.h"

// 0x0041E818.  Five VIArrays, five identical guards; the fifth's `li v0,-1`
// rides its own delay slot instead of the shared epilogue's because Reset()
// follows it.  Reset()'s return value is discarded.
int VIClipStack::Init()
{
    if (m_entries.Init(500) < 0)
        return -1;
    if (m_verts.Init(2000) < 0)
        return -1;
    if (m_planes.Init(2000) < 0)
        return -1;
    if (m_clipA.Init(32) < 0)
        return -1;
    if (m_clipB.Init(32) < 0)
        return -1;
    Reset();
    return 0;
}

// 0x0041E8A0.  The same five in the same order, and the last one if-converts
// where the first four branch.
int VIClipStack::Clear()
{
    if (m_entries.Clear() < 0)
        return -1;
    if (m_verts.Clear() < 0)
        return -1;
    if (m_planes.Clear() < 0)
        return -1;
    if (m_clipA.Clear() < 0)
        return -1;
    if (m_clipB.Clear() < 0)
        return -1;
    return 0;
}

// 0x0041E918.  Nothing is freed -- the arrays keep their blocks and only the
// three counts go back to zero.
int VIClipStack::Reset()
{
    m_numEntries = 0;
    m_numPlanes = 0;
    m_numVerts = 0;
    return 0;
}

// 0x0041FA88.  The entry is read but never cleared: a Push after a Pop
// overwrites it in place.
int VIClipStack::Pop()
{
    int i = m_numEntries - 1;
    VIClipEntry *e = &m_entries.m_data[i];

    m_numVerts -= e->m_numVerts;
    m_numPlanes -= e->m_numPlanes;
    m_numEntries = i;
    return 0;
}

// 0x0041FF00.
int VIClipStack::GetEntry(int i, int &numVerts, VIVect3 **verts,
                          int &numPlanes, VIPlane **planes)
{
    VIClipEntry *e = &m_entries.m_data[i];

    numVerts = e->m_numVerts;
    *verts = &m_verts.m_data[e->m_vertIndex];
    numPlanes = e->m_numPlanes;
    *planes = &m_planes.m_data[e->m_planeIndex];
    return 0;
}

// 0x0041FF50.
int VIClipStack::GetTop(int &numVerts, VIVect3 **verts,
                        int &numPlanes, VIPlane **planes)
{
    int i = m_numEntries - 1;
    VIClipEntry *e = &m_entries.m_data[i];

    numVerts = e->m_numVerts;
    *verts = &m_verts.m_data[e->m_vertIndex];
    numPlanes = e->m_numPlanes;
    *planes = &m_planes.m_data[e->m_planeIndex];
    return 0;
}

// 0x0041FE60.  Outside if the sphere's centre is further from any one plane
// than its own radius.  The four centre/radius loads are hoisted out of the
// loop; the plane's are not.
int VIClipStack::CullByTop(const VISphere &s)
{
    int k = m_numEntries - 1;
    VIClipEntry *e = &m_entries.m_data[k];
    VIPlane *p = &m_planes.m_data[e->m_planeIndex];
    int n = e->m_numPlanes;

    for (int i = 0; i < n; i++)
    {
        if (s.r < s.c.x * p[i].n.x + s.c.y * p[i].n.y + s.c.z * p[i].n.z + p[i].d)
            return 1;
    }
    return 0;
}

// 0x0041FD60.  The box test, and a SHIPPED BUG.  It is the standard
// positive-vertex cull -- pick, per plane, the box corner furthest along the
// normal and test that one point -- except that the value each component is
// compared against, and the value the plane equation is finally tested
// against, is `v.x` READ AT THE TOP OF THE ITERATION.  On the first plane
// that is 0.0f from the constructor and the test is right; from the second
// plane on it is whatever corner x the previous plane chose, so the corner
// selection and the cull threshold both drift with the box.  The single load
// feeding all four compares is what says so: gcc would have to reload after
// the store to sp+0 if the source named `v.x` again.
int VIClipStack::CullByTop(const VIBBox &b)
{
    VIVect3 v;
    int k = m_numEntries - 1;
    VIClipEntry *e = &m_entries.m_data[k];
    VIPlane *p = &m_planes.m_data[e->m_planeIndex];
    int n = e->m_numPlanes;

    for (int i = 0; i < n; i++)
    {
        float t = v.x;

        if (t <= p[i].n.x) v.x = b.min.x; else v.x = b.max.x;
        if (t <= p[i].n.y) v.y = b.min.y; else v.y = b.max.y;
        if (t <= p[i].n.z) v.z = b.min.z; else v.z = b.max.z;
        if (t < v.x * p[i].n.x + v.y * p[i].n.y + v.z * p[i].n.z + p[i].d)
            return 1;
    }
    return 0;
}

// 0x0041F900.  Append a volume, translating it as it is copied: the vertices
// by +t, and each plane by pushing its distance back along its own normal.
// Both loops re-read the array's `m_data` and the running count on every
// iteration -- a store through VIVect3 * may alias either -- and the entry's
// two counts are DERIVED at the end from the indices written at the start.
int VIClipStack::PushTranslatedVolume(int numVerts, VIVect3 *verts,
                                      int numPlanes, VIPlane *planes,
                                      const VIVect3 &t)
{
    VIClipEntry *e = &m_entries.m_data[m_numEntries];

    e->m_planeIndex = m_numPlanes;
    e->m_vertIndex = m_numVerts;

    for (int i = 0; i < numVerts; i++)
    {
        VIArray<VIVect3> *va = &m_verts;
        VIVect3 *v = &va->m_data[m_numVerts];

        v->x = verts->x;
        v->y = verts->y;
        v->z = verts->z;
        verts++;
        v = &va->m_data[m_numVerts];
        v->x += t.x;
        v->y += t.y;
        v->z += t.z;
        m_numVerts++;
    }

    for (int j = 0; j < numVerts; j++)
    {
        VIArray<VIPlane> *pa = &m_planes;
        VIPlane *p = &pa->m_data[m_numPlanes];

        p->n.x = planes->n.x;
        p->n.y = planes->n.y;
        p->n.z = planes->n.z;
        p->d = planes->d;
        planes++;
        p = &pa->m_data[m_numPlanes];
        p->d -= t.x * p->n.x + t.y * p->n.y + t.z * p->n.z;
        m_numPlanes++;
    }

    e->m_numPlanes = m_numPlanes - e->m_planeIndex;
    e->m_numVerts = m_numVerts - e->m_vertIndex;
    return m_numEntries++;
}

// 0x0041E930.  The axis-aligned case, written out: six planes and eight
// corners, no loops.  Both arrays are named through a POINTER VARIABLE -- the
// `addiu t0,a0,28` and `addiu a3,a0,16` in the prologue, Phase 9's lever --
// and the element address is derived TWICE per plane, once for the three
// normal components and once for the distance, and once per vertex COMPONENT.
int VIClipStack::PushBBox(VIBBox &b)
{
    VIClipEntry *e = &m_entries.m_data[m_numEntries];
    VIArray<VIPlane> *pa = &m_planes;
    VIPlane *p;

    e->m_planeIndex = m_numPlanes;
    e->m_vertIndex = m_numVerts;

    p = &pa->m_data[m_numPlanes];
    p->n.x = 1.0f;
    p->n.y = 0.0f;
    p->n.z = 0.0f;
    p = &pa->m_data[m_numPlanes];
    p->d = -b.max.x;
    m_numPlanes++;

    p = &pa->m_data[m_numPlanes];
    p->n.x = -1.0f;
    p->n.y = 0.0f;
    p->n.z = 0.0f;
    p = &pa->m_data[m_numPlanes];
    p->d = b.min.x;
    m_numPlanes++;

    p = &pa->m_data[m_numPlanes];
    p->n.x = 0.0f;
    p->n.y = 1.0f;
    p->n.z = 0.0f;
    p = &pa->m_data[m_numPlanes];
    p->d = -b.max.y;
    m_numPlanes++;

    p = &pa->m_data[m_numPlanes];
    p->n.x = 0.0f;
    p->n.y = -1.0f;
    p->n.z = 0.0f;
    p = &pa->m_data[m_numPlanes];
    p->d = b.min.y;
    m_numPlanes++;

    p = &pa->m_data[m_numPlanes];
    p->n.x = 0.0f;
    p->n.y = 0.0f;
    p->n.z = 1.0f;
    p = &pa->m_data[m_numPlanes];
    p->d = -b.max.z;
    m_numPlanes++;

    p = &pa->m_data[m_numPlanes];
    p->n.x = 0.0f;
    p->n.y = 0.0f;
    p->n.z = -1.0f;
    p = &pa->m_data[m_numPlanes];
    p->d = b.min.z;
    m_numPlanes++;

    VIArray<VIVect3> *va = &m_verts;

    va->m_data[m_numVerts].x = b.min.x;
    va->m_data[m_numVerts].y = b.min.y;
    va->m_data[m_numVerts].z = b.min.z;
    m_numVerts++;

    va->m_data[m_numVerts].x = b.max.x;
    va->m_data[m_numVerts].y = b.min.y;
    va->m_data[m_numVerts].z = b.min.z;
    m_numVerts++;

    va->m_data[m_numVerts].x = b.min.x;
    va->m_data[m_numVerts].y = b.max.y;
    va->m_data[m_numVerts].z = b.min.z;
    m_numVerts++;

    va->m_data[m_numVerts].x = b.max.x;
    va->m_data[m_numVerts].y = b.max.y;
    va->m_data[m_numVerts].z = b.min.z;
    m_numVerts++;

    va->m_data[m_numVerts].x = b.min.x;
    va->m_data[m_numVerts].y = b.min.y;
    va->m_data[m_numVerts].z = b.max.z;
    m_numVerts++;

    va->m_data[m_numVerts].x = b.max.x;
    va->m_data[m_numVerts].y = b.min.y;
    va->m_data[m_numVerts].z = b.max.z;
    m_numVerts++;

    va->m_data[m_numVerts].x = b.min.x;
    va->m_data[m_numVerts].y = b.max.y;
    va->m_data[m_numVerts].z = b.max.z;
    m_numVerts++;

    va->m_data[m_numVerts].x = b.max.x;
    va->m_data[m_numVerts].y = b.max.y;
    va->m_data[m_numVerts].z = b.max.z;
    m_numVerts++;

    e->m_numPlanes = m_numPlanes - e->m_planeIndex;
    e->m_numVerts = m_numVerts - e->m_vertIndex;
    int n = m_numEntries;

    m_numEntries = n + 1;
    return n;
}
