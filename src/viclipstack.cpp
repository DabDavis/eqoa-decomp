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

// 0x0041FD60.  The box test: the standard positive-vertex cull -- per plane,
// the box corner furthest along the normal, tested against zero.  NOT a
// shipped bug, as first read: the single load feeding all four compares
// is the literal 0.0f, which cse2 materialises by re-reading the freshly
// zeroed `v.x` in the loop pre-header (loop has hoisted the constant first).
int VIClipStack::CullByTop(const VIBBox &b)
{
    VIVect3 v;
    int k = m_numEntries - 1;
    VIClipEntry *e = &m_entries.m_data[k];
    VIPlane *p = &m_planes.m_data[e->m_planeIndex];
    int n = e->m_numPlanes;

    for (int i = 0; i < n; i++)
    {
        if (0.0f <= p[i].n.x) v.x = b.min.x; else v.x = b.max.x;
        if (0.0f <= p[i].n.y) v.y = b.min.y; else v.y = b.max.y;
        if (0.0f <= p[i].n.z) v.z = b.min.z; else v.z = b.max.z;
        if (0.0f < v.x * p[i].n.x + v.y * p[i].n.y + v.z * p[i].n.z + p[i].d)
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
    int i;

    e->m_planeIndex = m_numPlanes;
    e->m_vertIndex = m_numVerts;

    for (i = 0; i < numVerts; i++)
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

    for (i = 0; i < numVerts; i++)
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
    int r = m_numEntries;

    m_numEntries = r + 1;
    return r;
}

// 0x0041E930.  The axis-aligned case, written out: six planes and eight
// corners, no loops.  Both arrays are named through a POINTER VARIABLE -- the
// `addiu t0,a0,28` and `addiu a3,a0,16` in the prologue, Phase 9's lever --
// and the element address is derived TWICE per plane, once for the three
// normal components and once for the distance, and once per vertex COMPONENT.
// The subscript is VIArray's inline operator[] through that pointer: written
// `pa->m_data[k]` the index and data loads swap registers in every element.
int VIClipStack::PushBBox(VIBBox &b)
{
    VIClipEntry *e = &m_entries.m_data[m_numEntries];
    VIArray<VIPlane> *pa = &m_planes;
    VIPlane *p;

    e->m_planeIndex = m_numPlanes;
    e->m_vertIndex = m_numVerts;

    p = &(*pa)[m_numPlanes];
    p->n.x = 1.0f;
    p->n.y = 0.0f;
    p->n.z = 0.0f;
    p = &(*pa)[m_numPlanes];
    p->d = -b.max.x;
    m_numPlanes++;

    p = &(*pa)[m_numPlanes];
    p->n.x = -1.0f;
    p->n.y = 0.0f;
    p->n.z = 0.0f;
    p = &(*pa)[m_numPlanes];
    p->d = b.min.x;
    m_numPlanes++;

    p = &(*pa)[m_numPlanes];
    p->n.x = 0.0f;
    p->n.y = 1.0f;
    p->n.z = 0.0f;
    p = &(*pa)[m_numPlanes];
    p->d = -b.max.y;
    m_numPlanes++;

    p = &(*pa)[m_numPlanes];
    p->n.x = 0.0f;
    p->n.y = -1.0f;
    p->n.z = 0.0f;
    p = &(*pa)[m_numPlanes];
    p->d = b.min.y;
    m_numPlanes++;

    p = &(*pa)[m_numPlanes];
    p->n.x = 0.0f;
    p->n.y = 0.0f;
    p->n.z = 1.0f;
    p = &(*pa)[m_numPlanes];
    p->d = -b.max.z;
    m_numPlanes++;

    p = &(*pa)[m_numPlanes];
    p->n.x = 0.0f;
    p->n.y = 0.0f;
    p->n.z = -1.0f;
    p = &(*pa)[m_numPlanes];
    p->d = b.min.z;
    m_numPlanes++;

    VIArray<VIVect3> *va = &m_verts;

    (*va)[m_numVerts].x = b.min.x;
    (*va)[m_numVerts].y = b.min.y;
    (*va)[m_numVerts].z = b.min.z;
    m_numVerts++;

    (*va)[m_numVerts].x = b.max.x;
    (*va)[m_numVerts].y = b.min.y;
    (*va)[m_numVerts].z = b.min.z;
    m_numVerts++;

    (*va)[m_numVerts].x = b.min.x;
    (*va)[m_numVerts].y = b.max.y;
    (*va)[m_numVerts].z = b.min.z;
    m_numVerts++;

    (*va)[m_numVerts].x = b.max.x;
    (*va)[m_numVerts].y = b.max.y;
    (*va)[m_numVerts].z = b.min.z;
    m_numVerts++;

    (*va)[m_numVerts].x = b.min.x;
    (*va)[m_numVerts].y = b.min.y;
    (*va)[m_numVerts].z = b.max.z;
    m_numVerts++;

    (*va)[m_numVerts].x = b.max.x;
    (*va)[m_numVerts].y = b.min.y;
    (*va)[m_numVerts].z = b.max.z;
    m_numVerts++;

    (*va)[m_numVerts].x = b.min.x;
    (*va)[m_numVerts].y = b.max.y;
    (*va)[m_numVerts].z = b.max.z;
    m_numVerts++;

    (*va)[m_numVerts].x = b.max.x;
    (*va)[m_numVerts].y = b.max.y;
    (*va)[m_numVerts].z = b.max.z;
    m_numVerts++;

    e->m_numPlanes = m_numPlanes - e->m_planeIndex;
    e->m_numVerts = m_numVerts - e->m_vertIndex;
    int n = m_numEntries;

    m_numEntries = n + 1;
    return n;
}

// 0x0041ED98.  The camera's view volume as a clip volume: frustum planes 1..5
// (the near plane is dropped) and five points -- the eye and the four far
// corners, so the volume is a pyramid from the eye rather than the frustum
// itself.  The vertex count is stored as a literal 5 through a re-derived
// entry pointer and then overwritten by the derived count.
int VIClipStack::PushFrustum(VICamera &cam)
{
    VIClipEntry *e = &m_entries.m_data[m_numEntries];
    // Named here for the corner copies only: the plane walk reads off `cam`.
    VIFrustum *f = &cam.m_frustum;

    e->m_planeIndex = m_numPlanes;
    e->m_vertIndex = m_numVerts;

    // Through VIArray::operator[], an address pseudo, where `.m_data[k]`
    // folds and renames every register in the copy.
    for (int i = 1; i < 6; i++)
    {
        m_planes[m_numPlanes] = cam.m_frustum.m_planes[i];
        m_numPlanes++;
    }

    m_verts[m_numVerts] = cam.m_position;
    m_numVerts++;
    m_verts[m_numVerts] = f->m_verts[4];
    m_numVerts++;
    m_verts[m_numVerts] = f->m_verts[5];
    m_numVerts++;
    m_verts[m_numVerts] = f->m_verts[6];
    m_numVerts++;
    m_verts[m_numVerts] = f->m_verts[7];
    m_numVerts++;

    m_entries[m_numEntries].m_numVerts = 5;
    e->m_numPlanes = m_numPlanes - e->m_planeIndex;
    e->m_numVerts = m_numVerts - e->m_vertIndex;
    int n = m_numEntries;
    m_numEntries = n + 1;
    return n;
}

// Through references: the direction's +0 folds onto the camera and its +4/+8
// come off a register the loop keeps, which is what spills the walk's
// constant 12 around the plane call.
static inline float Dot(const VIVect3 &a, const VIVect3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 0x0041EF70.  A portal seen from the camera: the camera's frustum plane 1,
// then one plane per portal edge through the eye, and the portal's corners
// pushed out along their rays from the eye to the far distance (measured
// along the view direction).  The eye itself is the first vertex.
int VIClipStack::PushPortal(VICamera &cam, int n, VIVect3 *p)
{
    VIVect3 d;
    VIPlane pl;
    VIClipEntry *e = &m_entries.m_data[m_numEntries];
    VIVect3 *prev = &p[n - 1];

    e->m_planeIndex = m_numPlanes;
    e->m_vertIndex = m_numVerts;

    m_planes[m_numPlanes] = cam.m_frustum.m_planes[1];
    m_numPlanes++;
    m_verts[m_numVerts] = cam.m_position;
    m_numVerts++;

    for (int i = 0; i < n; i++)
    {
        VIVect3 *a = prev;
        prev = &p[i];
        pl(*a, p[i], cam.m_position);
        m_planes[m_numPlanes] = pl;
        m_numPlanes++;

        d = p[i];
        d -= cam.m_position;
        // A NAMED reference: a flagged pointer, so the +4/+8 loads off it are
        // free to move above the stores to `d` (handed `cam.m_dir` the
        // inline's copy is unflagged and they stay behind them).
        const VIVect3 &dir = cam.m_dir;
        float t = Dot(d, dir);
        m_verts[m_numVerts] = d;
        m_verts[m_numVerts] *= cam.m_far / t;
        m_verts[m_numVerts] += cam.m_position;
        m_numVerts++;
    }

    e->m_numPlanes = m_numPlanes - e->m_planeIndex;
    e->m_numVerts = m_numVerts - e->m_vertIndex;
    int k = m_numEntries;
    m_numEntries = k + 1;
    return k;
}

// 0x0041F258.  The same for an arbitrary eye: the near plane is the caller's,
// flipped and moved out to `dist`, and the corners are pushed out to `dist`
// measured along that plane's normal.
int VIClipStack::PushPortal(VIVect3 &eye, float dist, int n, VIVect3 *p,
                            VIPlane &near)
{
    VIVect3 d;
    VIPlane pl;
    VIClipEntry *e = &m_entries.m_data[m_numEntries];
    VIVect3 *prev = &p[n - 1];

    e->m_planeIndex = m_numPlanes;
    e->m_vertIndex = m_numVerts;

    float t = near.Distance(eye);
    m_planes[m_numPlanes].n = VIVect3(-near.n.x, -near.n.y, -near.n.z);
    m_planes[m_numPlanes].d = -((dist - t) + near.d);
    m_numPlanes++;
    m_verts[m_numVerts] = eye;
    m_numVerts++;

    for (int i = 0; i < n; i++)
    {
        VIVect3 *a = prev;
        prev = &p[i];
        pl(*a, p[i], eye);
        m_planes[m_numPlanes] = pl;
        m_numPlanes++;

        d = p[i];
        d -= eye;
        float t = Dot(d, near.n);
        m_verts[m_numVerts] = d;
        m_verts[m_numVerts] *= dist / t;
        m_verts[m_numVerts] += eye;
        m_numVerts++;
    }

    e->m_numPlanes = m_numPlanes - e->m_planeIndex;
    e->m_numVerts = m_numVerts - e->m_vertIndex;
    int k = m_numEntries;
    m_numEntries = k + 1;
    return k;
}

// 0x0041F598.  PushFrustum moved by `t`: every plane pushed back along its
// own normal, every point moved.
int VIClipStack::PushTranslatedFrustum(VICamera &cam, const VIVect3 &t)
{
    VIClipEntry *e = &m_entries.m_data[m_numEntries];
    VIFrustum *f = &cam.m_frustum;

    e->m_planeIndex = m_numPlanes;
    e->m_vertIndex = m_numVerts;

    for (int i = 1; i < 6; i++)
    {
        m_planes[m_numPlanes] = cam.m_frustum.m_planes[i];
        VIPlane &p = m_planes[m_numPlanes];
        p.d -= t.x * p.n.x + t.y * p.n.y + t.z * p.n.z;
        m_numPlanes++;
    }

    m_verts[m_numVerts] = cam.m_position;
    m_verts[m_numVerts] += t;
    m_numVerts++;
    m_verts[m_numVerts] = f->m_verts[4];
    m_verts[m_numVerts] += t;
    m_numVerts++;
    m_verts[m_numVerts] = f->m_verts[5];
    m_verts[m_numVerts] += t;
    m_numVerts++;
    m_verts[m_numVerts] = f->m_verts[6];
    m_verts[m_numVerts] += t;
    m_numVerts++;
    m_verts[m_numVerts] = f->m_verts[7];
    m_verts[m_numVerts] += t;
    m_numVerts++;

    m_entries[m_numEntries].m_numVerts = 5;
    e->m_numPlanes = m_numPlanes - e->m_planeIndex;
    e->m_numVerts = m_numVerts - e->m_vertIndex;
    int n = m_numEntries;
    m_numEntries = n + 1;
    return n;
}

// 0x0041FAC8.  VIClipper::Clip's body again against the top volume's planes
// and this stack's own two scratch arrays -- there is no call to it -- with
// ONE difference: a point exactly on a plane is DROPPED here, where Clip
// keeps it.
int VIClipStack::ClipByTop(int n, VIVect3 *v, int &numOut, VIVect3 **out)
{
    int cnt = 0;
    int k = m_numEntries - 1;
    VIClipEntry *e = &m_entries.m_data[k];
    int np = e->m_numPlanes;
    VIPlane *planes = &m_planes.m_data[e->m_planeIndex];
    VIVect3 *dst = &m_clipA[0];
    int flip = 1;

    for (; np != 0; np--, planes++)
    {
        VIVect3 *prev = v + n - 1;
        float dPrev = planes->Distance(*prev);

        for (; n != 0; n--, v++)
        {
            float t;
            float dCur = planes->Distance(*v);

            if (dCur < 0.0f)
            {
                if (0.0f < dPrev)
                {
                    t = dPrev / (dPrev - dCur);
                    *dst = *v;
                    *dst -= *prev;
                    *dst *= t;
                    *dst += *prev;
                    cnt++;
                    dst++;
                }
                *dst = *v;
                cnt++;
                dst++;
            }
            else if (0.0f < dCur)
            {
                if (dPrev < 0.0f)
                {
                    t = dPrev / (dPrev - dCur);
                    *dst = *v;
                    *dst -= *prev;
                    *dst *= t;
                    *dst += *prev;
                    cnt++;
                    dst++;
                }
            }
            prev = v;
            dPrev = dCur;
        }

        if (cnt == 0)
        {
            numOut = 0;
            *out = 0;
            return 0;
        }
        if (flip)
        {
            v = &m_clipA[0];
            dst = &m_clipB[0];
        }
        else
        {
            v = &m_clipB[0];
            dst = &m_clipA[0];
        }
        n = cnt;
        flip = 1 - flip;
        cnt = 0;
    }

    numOut = n;
    *out = v;
    return 1;
}
