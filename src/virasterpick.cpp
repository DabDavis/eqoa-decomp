// VIRaster's picking -- SUPPORT 0x0040C760 .. 0x0040DE50 (the PickPrimBuffer
// family, its own TU) and 0x00412D18 .. 0x004138C8 (Cull, Pick,
// PointInTriangle and CalcHitColor, which the image places after
// VIPickSegment's own functions), phase 61.
//
// A pick takes the world-space segment into the buffer's object space through
// the inverse model-view, culls it against the buffer's and then each prim's
// box, and walks every strip triangle by triangle -- alternating the winding
// on odd triangles -- through Pick.  A hit records the plane normal, the
// parametric time and the material, and CalcHitColor interpolates the three
// vertex colours at the hit point; the caller adds the static lighting.
#include "vi.h"

// The 64-byte unpacked vertex: four quadwords, of which the pick reads the
// position and the colour.
class VIPrimVertexVUNC
{
public:
    VIVect4 m_pos;      // +0x00
    VIVect4 m_uv;       // +0x10
    VIVect4 m_normal;   // +0x20
    VIVect4 m_color;    // +0x30
};

// The 32-byte packed vertex without pre-translation.
class VIPrimVertexPackVUNC
{
public:
    int        m_p00;      // +0x00
    VIVect3S16 m_pos;      // +0x04
    short      m_p0A;      // +0x0A
    int        m_p0C[4];   // +0x0C
    VIVect4U8  m_color;    // +0x1C
};

// The vertex unpackers, written as helpers taking REFERENCES: each one is what
// gives the vertex's position and colour sub-objects addresses of their own
// (`addiu v1,s3,4` / `addiu a1,s3,28`), where field accesses fold every offset
// into the vertex pointer.
static inline void PickPos(VIVect3 &p, const VIVect3S16 &v, float s)
{
    p.x = (float)v.x * s;
    p.y = (float)v.y * s;
    p.z = (float)v.z * s;
}

static inline void PickColor(VIColor32 &c, const VIVect4U8 &k)
{
    c.r = k.x << 1;
    c.g = k.y << 1;
    c.b = k.z << 1;
    c.a = k.w << 1;
}

static inline void PickColorF(VIColor32 &c, const VIVect4 &f)
{
    c.r = (int)(f.x * 255.0f + 0.5f);
    c.g = (int)(f.y * 255.0f + 0.5f);
    c.b = (int)(f.z * 255.0f + 0.5f);
    c.a = (int)(f.w * 255.0f + 0.5f);
}


int VIRaster::PickPrimBuffer(VIPickSegment &seg, int prim, int pal)
{
    VIPickSegment local;
    int hit = 0;
    ((VIMatrix44 *)VI_SPR(0x70000100))->Invert(*(VIMatrix44 *)VI_SPR(0x70000000));
    if (local.Init(seg, *(VIMatrix44 *)VI_SPR(0x70000100)) < 0)
        return -1;
    VIMaterialPal *mp = m_materialPals.Value(pal);
    VIPrimBuffer *pb = PrimBuffer(prim);
    if (Cull(local, pb->m_bbox) == 0)
    {
        switch (PrimBuffer(prim)->m_type)
        {
        case VIPrimBufferTypeVUNC:
            hit = PickPrimBufferVUNC(local, *pb, *mp);
            if (hit)
            {
                if (seg.Init(local, *(VIMatrix44 *)VI_SPR(0x70000000)) < 0)
                    return -1;
                hit = 1;
            }
            break;
        case VIPrimBufferTypePackVUNC:
            hit = PickPrimBufferPackVUNC(local, *pb, *mp);
            if (hit)
            {
                if (seg.Init(local, *(VIMatrix44 *)VI_SPR(0x70000000)) < 0)
                    return -1;
                hit = 1;
            }
            break;
        case VIPrimBufferTypePackVUNCG:
            hit = PickPrimBufferPackVUNCG(local, *pb, *mp);
            if (hit)
            {
                if (seg.Init(local, *(VIMatrix44 *)VI_SPR(0x70000000)) < 0)
                    return -1;
                hit = 1;
            }
            break;
        case VIPrimBufferTypePackVUNBW:
            hit = PickPrimBufferPackVUNBW(local, *pb, *mp);
            if (hit)
            {
                if (seg.Init(local, *(VIMatrix44 *)VI_SPR(0x70000000)) < 0)
                    return -1;
                hit = 1;
            }
            break;
        }
    }
    if (hit)
    {
        seg.m_hitColor.r += *(float *)VI_SPR(0x70000200);
        seg.m_hitColor.g += *(float *)VI_SPR(0x70000204);
        seg.m_hitColor.b += *(float *)VI_SPR(0x70000208);
        seg.m_hitColor.a *= *(float *)VI_SPR(0x7000020C);
    }
    return hit;
}

int VIRaster::PickPrimBuffer(VIPickSegment &seg, int prim, int cbi, int pal)
{
    VIPickSegment local;
    int hit = 0;
    ((VIMatrix44 *)VI_SPR(0x70000100))->Invert(*(VIMatrix44 *)VI_SPR(0x70000000));
    if (local.Init(seg, *(VIMatrix44 *)VI_SPR(0x70000100)) < 0)
        return -1;
    VIMaterialPal *mp = m_materialPals.Value(pal);
    VIPrimBuffer *pb = PrimBuffer(prim);
    VIColorBuffer *cb = ColorBuffer(cbi);
    if (Cull(local, pb->m_bbox) == 0)
    {
        switch (PrimBuffer(prim)->m_type)
        {
        case VIPrimBufferTypePackVUNC:
            hit = PickPrimBufferPackVUNC(local, *pb, *cb, *mp);
            if (hit)
            {
                if (seg.Init(local, *(VIMatrix44 *)VI_SPR(0x70000000)) < 0)
                    return -1;
                hit = 1;
            }
            break;
        }
    }
    if (hit)
    {
        seg.m_hitColor.r += *(float *)VI_SPR(0x70000200);
        seg.m_hitColor.g += *(float *)VI_SPR(0x70000204);
        seg.m_hitColor.b += *(float *)VI_SPR(0x70000208);
        seg.m_hitColor.a *= *(float *)VI_SPR(0x7000020C);
    }
    return hit;
}

// Each prim is a strip behind a 32-byte header and ahead of a 16-byte tail;
// the first two vertices prime the window and every further one closes a
// triangle.
int VIRaster::PickPrimBufferVUNC(VIPickSegment &seg, VIPrimBuffer &pb,
                                 VIMaterialPal &pal)
{
    VIVect3 p0;
    VIVect3 p1;
    VIVect3 p2;
    VIColor32 c0;
    VIColor32 c1;
    VIColor32 c2;
    int hit = 0;
    char *verts = pb.m_verts;
    int i;
    int n;
    int flip;
    int prim = 0;
    for (i = 0; i < pb.m_numMaterials; i++)
    {
        int mat = pb.m_materials[i].m_matIdx;
        mat = pal.m_mats[mat];
        for (n = pb.m_materials[i].m_numPrims; n; )
        {
            if (!Cull(seg, pb.m_prims[prim].m_bbox))
            {
                verts += 32;
                VIPrimVertexVUNC *v = (VIPrimVertexVUNC *)verts;
                p1.Set(v->m_pos.x, v->m_pos.y, v->m_pos.z);
                PickColorF(c1, v->m_color);
                verts += 64;
                v = (VIPrimVertexVUNC *)verts;
                p2.Set(v->m_pos.x, v->m_pos.y, v->m_pos.z);
                PickColorF(c2, v->m_color);
                verts += 64;
                flip = 0;
                int k = pb.m_prims[prim].m_numVerts;
                for (k -= 2; k; k--)
                {
                    p0 = p1;
                    p1 = p2;
                    c0 = c1;
                    c1 = c2;
                    v = (VIPrimVertexVUNC *)verts;
                    p2.Set(v->m_pos.x, v->m_pos.y, v->m_pos.z);
                    PickColorF(c2, v->m_color);
                    verts += 64;
                    if (!flip)
                    {
                        if (Pick(seg, mat, p0, p1, p2))
                        {
                            CalcHitColor(seg, p0, p1, p2, c0, c1, c2);
                            hit = 1;
                        }
                    }
                    else
                    {
                        if (Pick(seg, mat, p1, p0, p2))
                        {
                            CalcHitColor(seg, p1, p0, p2, c1, c0, c2);
                            hit = 1;
                        }
                    }
                    flip = 1 - flip;
                }
                verts += 16;
                prim++;
                n--;
            }
            else
            {
                verts += (pb.m_prims[prim].m_numVerts << 6) + 48;
                prim++;
                n--;
            }
        }
    }
    return hit;
}

int VIRaster::PickPrimBufferPackVUNC(VIPickSegment &seg, VIPrimBuffer &pb,
                                     VIMaterialPal &pal)
{
    VIVect3 p0;
    VIVect3 p1;
    VIVect3 p2;
    VIColor32 c0;
    VIColor32 c1;
    VIColor32 c2;
    int hit = 0;
    char *verts = pb.m_verts;
    int i;
    int n;
    int flip;
    int prim = 0;
    for (i = 0; i < pb.m_numMaterials; i++)
    {
        int mat = pb.m_materials[i].m_matIdx;
        mat = pal.m_mats[mat];
        for (n = pb.m_materials[i].m_numPrims; n; )
        {
            if (!Cull(seg, pb.m_prims[prim].m_bbox))
            {
                verts += 32;
                VIPrimVertexPackVUNC *v = (VIPrimVertexPackVUNC *)verts;
                PickPos(p1, v->m_pos, pb.m_scale.x);
                PickColor(c1, v->m_color);
                verts += 32;
                v = (VIPrimVertexPackVUNC *)verts;
                PickPos(p2, v->m_pos, pb.m_scale.x);
                PickColor(c2, v->m_color);
                verts += 32;
                flip = 0;
                int k = pb.m_prims[prim].m_numVerts;
                for (k -= 2; k; k--)
                {
                    p0 = p1;
                    p1 = p2;
                    c0 = c1;
                    c1 = c2;
                    v = (VIPrimVertexPackVUNC *)verts;
                    PickPos(p2, v->m_pos, pb.m_scale.x);
                    PickColor(c2, v->m_color);
                    verts += 32;
                    if (!flip)
                    {
                        if (Pick(seg, mat, p0, p1, p2))
                        {
                            CalcHitColor(seg, p0, p1, p2, c0, c1, c2);
                            hit = 1;
                        }
                    }
                    else
                    {
                        if (Pick(seg, mat, p1, p0, p2))
                        {
                            CalcHitColor(seg, p1, p0, p2, c1, c0, c2);
                            hit = 1;
                        }
                    }
                    flip = 1 - flip;
                }
                verts += 16;
                prim++;
                n--;
            }
            else
            {
                verts += (pb.m_prims[prim].m_numVerts << 5) + 48;
                prim++;
                n--;
            }
        }
    }
    return hit;
}

int VIRaster::PickPrimBufferPackVUNCG(VIPickSegment &seg, VIPrimBuffer &pb,
                                      VIMaterialPal &pal)
{
    VIVect3 p0;
    VIVect3 p1;
    VIVect3 p2;
    VIColor32 c0;
    VIColor32 c1;
    VIColor32 c2;
    int hit = 0;
    char *verts = pb.m_verts;
    int i;
    int n;
    int flip;
    int prim = 0;
    for (i = 0; i < pb.m_numMaterials; i++)
    {
        int mat = pb.m_materials[i].m_matIdx;
        mat = pal.m_mats[mat];
        for (n = pb.m_materials[i].m_numPrims; n; )
        {
            if (!Cull(seg, pb.m_prims[prim].m_bbox))
            {
                verts += 32;
                VIPrimVertexPackVUNCG *v = (VIPrimVertexPackVUNCG *)verts;
                PickPos(p1, v->m_pos, pb.m_scale.x);
                p1 += m_preTrans[v->m_preTrans];
                PickColor(c1, *(VIVect4U8 *)&v->m_color);
                verts += 32;
                v = (VIPrimVertexPackVUNCG *)verts;
                PickPos(p2, v->m_pos, pb.m_scale.x);
                p2 += m_preTrans[v->m_preTrans];
                PickColor(c2, *(VIVect4U8 *)&v->m_color);
                verts += 32;
                flip = 0;
                int k = pb.m_prims[prim].m_numVerts;
                for (k -= 2; k; k--)
                {
                    p0 = p1;
                    p1 = p2;
                    c0 = c1;
                    c1 = c2;
                    v = (VIPrimVertexPackVUNCG *)verts;
                    PickPos(p2, v->m_pos, pb.m_scale.x);
                    p2 += m_preTrans[v->m_preTrans];
                    PickColor(c2, *(VIVect4U8 *)&v->m_color);
                    verts += 32;
                    if (!flip)
                    {
                        if (Pick(seg, mat, p0, p1, p2))
                        {
                            CalcHitColor(seg, p0, p1, p2, c0, c1, c2);
                            hit = 1;
                        }
                    }
                    else
                    {
                        if (Pick(seg, mat, p1, p0, p2))
                        {
                            CalcHitColor(seg, p1, p0, p2, c1, c0, c2);
                            hit = 1;
                        }
                    }
                    flip = 1 - flip;
                }
                verts += 16;
                prim++;
                n--;
            }
            else
            {
                verts += (pb.m_prims[prim].m_numVerts << 5) + 48;
                prim++;
                n--;
            }
        }
    }
    return hit;
}

int VIRaster::PickPrimBufferPackVUNBW(VIPickSegment &seg, VIPrimBuffer &pb,
                                      VIMaterialPal &pal)
{
    return 0;
}

// The baked-colour form takes its colours from the colour buffer instead:
// four to a quadword, the first two of each strip in the second half of its
// first quadword, and every strip padded out to a whole quadword.
int VIRaster::PickPrimBufferPackVUNC(VIPickSegment &seg, VIPrimBuffer &pb,
                                     VIColorBuffer &cb, VIMaterialPal &pal)
{
    VIVect3 p0;
    VIVect3 p1;
    VIVect3 p2;
    VIColor32 c0;
    VIColor32 c1;
    VIColor32 c2;
    int hit = 0;
    char *verts = pb.m_verts;
    unsigned char *cq = (unsigned char *)cb.m_tags;
    int i;
    int n;
    int flip;
    int prim = 0;
    for (i = 0; i < pb.m_numMaterials; i++)
    {
        int mat = pb.m_materials[i].m_matIdx;
        // A named element pointer: the address is MATERIALISED, base first
        // and into the base's register, where a subscript is index-first.
        int *mp = &pal.m_mats.m_data[mat];
        mat = *mp;
        for (n = pb.m_materials[i].m_numPrims; n; )
        {
            if (!Cull(seg, pb.m_prims[prim].m_bbox))
            {
                verts += 32;
                VIPrimVertexPackVUNC *v = (VIPrimVertexPackVUNC *)verts;
                PickPos(p1, v->m_pos, pb.m_scale.x);
                unsigned char *c = cq + 8;
                PickColor(c1, *(VIVect4U8 *)c);
                verts += 32;
                v = (VIPrimVertexPackVUNC *)verts;
                PickPos(p2, v->m_pos, pb.m_scale.x);
                c = cq + 12;
                PickColor(c2, *(VIVect4U8 *)c);
                c = cq + 16;
                verts += 32;
                flip = 0;
                int q = 0;
                int k = pb.m_prims[prim].m_numVerts;
                // The copy just before the loop: the sum stays in c (the
                // walker) and cq takes the copy, as the target has them.
                cq = c;
                for (k -= 2; k; k--)
                {
                    p0 = p1;
                    p1 = p2;
                    c0 = c1;
                    c1 = c2;
                    v = (VIPrimVertexPackVUNC *)verts;
                    PickPos(p2, v->m_pos, pb.m_scale.x);
                    PickColor(c2, *(VIVect4U8 *)c);
                    c += 4;
                    verts += 32;
                    q++;
                    if (q == 4)
                    {
                        cq += 16;
                        q = 0;
                    }
                    if (!flip)
                    {
                        if (Pick(seg, mat, p0, p1, p2))
                        {
                            CalcHitColor(seg, p0, p1, p2, c0, c1, c2);
                            hit = 1;
                        }
                    }
                    else
                    {
                        if (Pick(seg, mat, p1, p0, p2))
                        {
                            CalcHitColor(seg, p1, p0, p2, c1, c0, c2);
                            hit = 1;
                        }
                    }
                    flip = 1 - flip;
                }
                q += 2;
                if (q >= 5)
                {
                    q -= 4;
                    cq += 16;
                }
                // `q = 0` is dead (flow deletes it), but at the first jump
                // pass it makes the arm two insns, so jump.c does not turn
                // it into a conditional move: reorg's bnezl instead.
                if (q)
                {
                    cq += 16;
                    q = 0;
                }
                verts += 16;
                prim++;
                n--;
            }
            else
            {
                int nv = pb.m_prims[prim].m_numVerts;
                verts += (nv << 5) + 48;
                if (((nv + 4) & 3) == 0)
                    cq += (nv + 4) / 4 * 16;
                else
                    cq += (nv + 4) / 4 * 16 + 16;
                prim++;
                n--;
            }
        }
    }
    return hit;
}

// ----------------------------------------------------------------- culling --

// A segment against a bounding sphere: 1 when it cannot reach the sphere
// before the current hit, 0 when it starts inside or might.
int VIRaster::Cull(VIPickSegment &seg, VISphere &s)
{
    int r = 1;
    VIVect3 d(seg.m_start);
    d -= s.c;
    float c = d.x * d.x + d.y * d.y + d.z * d.z - s.r * s.r;
    if (c > 0.0f)
    {
        VIVect3 &dir = seg.m_dir;
        float b = d.x * dir.x + d.y * dir.y + d.z * dir.z;
        if (b < 0.0f)
        {
            float disc = b * b - c;
            if (disc > 0.0f)
            {
                if (-(b + sqrtf(disc)) * seg.m_invLength < seg.m_hitTime)
                    r = 0;
            }
        }
    }
    else
        r = 0;
    return r;
}

// Both ends of the segment on the far side of one face of the box.
int VIRaster::Cull(VIPickSegment &seg, VIBBox &b)
{
    if ((seg.m_start.x > b.max.x && seg.m_end.x > b.max.x) ||
        (seg.m_start.y > b.max.y && seg.m_end.y > b.max.y) ||
        (seg.m_start.z > b.max.z && seg.m_end.z > b.max.z) ||
        (seg.m_start.x < b.min.x && seg.m_end.x < b.min.x) ||
        (seg.m_start.y < b.min.y && seg.m_end.y < b.min.y) ||
        (seg.m_start.z < b.min.z && seg.m_end.z < b.min.z))
        return 1;
    return 0;
}

// The segment against one triangle: front-facing crossings only, and only
// nearer than the current hit.
int VIRaster::Pick(VIPickSegment &seg, int mat, VIVect3 &a, VIVect3 &b,
                   VIVect3 &c)
{
    VIPlane pl;
    VIVect3 p;
    int hit = 0;
    if (pl(a, b, c))
    {
        float d0 = pl.Distance(seg.m_start);
        if (d0 > 0.0f)
        {
            float d1 = pl.Distance(seg.m_end);
            if (d1 < 0.0f)
            {
                float t = d0 / (d0 - d1);
                if (t < seg.m_hitTime)
                {
                    p = seg.m_delta;
                    p *= t;
                    p += seg.m_start;
                    if (PointInTriangle(p, pl, a, b, c))
                    {
                        seg.m_hitTime = t;
                        seg.m_hitMaterial = mat;
                        seg.m_hitNormal = pl.n;
                        hit = 1;
                    }
                }
            }
        }
    }
    return hit;
}

// Inside when the point is on the inner side of all three edges, measured
// against the plane normal.
int VIRaster::PointInTriangle(VIVect3 &p, VIPlane &pl, VIVect3 &a, VIVect3 &b,
                              VIVect3 &c)
{
    VIVect3 e;
    VIVect3 w;
    VIVect3 x;
    e.x = b.x - a.x;
    e.y = b.y - a.y;
    e.z = b.z - a.z;
    w.x = p.x - b.x;
    w.y = p.y - b.y;
    w.z = p.z - b.z;
    x.x = e.y * w.z - e.z * w.y;
    x.y = -e.x * w.z + e.z * w.x;
    x.z = e.x * w.y - e.y * w.x;
    if (x.x * pl.n.x + x.y * pl.n.y + x.z * pl.n.z > 0.0f)
        return 0;
    e.x = c.x - b.x;
    e.y = c.y - b.y;
    e.z = c.z - b.z;
    w.x = p.x - c.x;
    w.y = p.y - c.y;
    w.z = p.z - c.z;
    x.x = e.y * w.z - e.z * w.y;
    x.y = -e.x * w.z + e.z * w.x;
    x.z = e.x * w.y - e.y * w.x;
    if (x.x * pl.n.x + x.y * pl.n.y + x.z * pl.n.z > 0.0f)
        return 0;
    e.x = a.x - c.x;
    e.y = a.y - c.y;
    e.z = a.z - c.z;
    w.x = p.x - a.x;
    w.y = p.y - a.y;
    w.z = p.z - a.z;
    x.x = e.y * w.z - e.z * w.y;
    x.y = -e.x * w.z + e.z * w.x;
    x.z = e.x * w.y - e.y * w.x;
    if (x.x * pl.n.x + x.y * pl.n.y + x.z * pl.n.z > 0.0f)
        return 0;
    return 1;
}

// A VIColor32 as the 0..1 floats of a VIColor32F.
static inline void PickColorTo(VIColor32F &d, const VIColor32 &c)
{
    d.r = c.r * (1.0f / 255.0f);
    d.g = c.g * (1.0f / 255.0f);
    d.b = c.b * (1.0f / 255.0f);
    d.a = c.a * (1.0f / 255.0f);
}

// Barycentric colour: the triangle's two edges and its normal make a basis,
// its inverse takes each colour channel's two deltas to a gradient, and the
// hit point's offset from `a` dotted with each gradient is the channel's
// change.  A degenerate triangle just takes `ca`.
int VIRaster::CalcHitColor(VIPickSegment &seg, VIVect3 &a, VIVect3 &b,
                           VIVect3 &c, VIColor32 &ca, VIColor32 &cb,
                           VIColor32 &cc)
{
    VIVect3 u;
    VIVect3 d;
    VIPlane pl;
    VIMatrix33 m;
    VIMatrix33 inv;
    VIVect3 hit;
    VIColor32F c0;
    VIColor32F c1;
    VIColor32F c2;
    VIVect3 k0;
    VIVect3 k1;
    VIVect3 k2;
    VIVect3 k3;
    hit = seg.GetHitPoint();
    m.m[0].x = b.x - a.x;
    m.m[1].x = b.y - a.y;
    m.m[2].x = b.z - a.z;
    u.x = c.x - a.x;
    u.y = c.y - a.y;
    u.z = c.z - a.z;
    m.m[0].y = u.x;
    m.m[1].y = u.y;
    m.m[2].y = u.z;
    if (pl(a, b, c))
    {
        m.m[0].z = pl.n.x;
        m.m[1].z = pl.n.y;
        m.m[2].z = pl.n.z;
        if (inv.Invert(m) >= 0)
        {
            c0.r = ca.r * (1.0f / 255.0f);
            c0.g = ca.g * (1.0f / 255.0f);
            c0.b = ca.b * (1.0f / 255.0f);
            c0.a = ca.a * (1.0f / 255.0f);
            c1.r = cb.r * (1.0f / 255.0f);
            c1.g = cb.g * (1.0f / 255.0f);
            c1.b = cb.b * (1.0f / 255.0f);
            c1.a = cb.a * (1.0f / 255.0f);
            c2.r = cc.r * (1.0f / 255.0f);
            c2.g = cc.g * (1.0f / 255.0f);
            c2.b = cc.b * (1.0f / 255.0f);
            c2.a = cc.a * (1.0f / 255.0f);
            d.x = c1.r - c0.r;
            d.y = c2.r - c0.r;
            d.z = 0.0f;
            k0 = d * inv;
            d.x = c1.g - c0.g;
            d.y = c2.g - c0.g;
            d.z = 0.0f;
            k1 = d * inv;
            d.x = c1.b - c0.b;
            d.y = c2.b - c0.b;
            d.z = 0.0f;
            k2 = d * inv;
            d.x = c1.a - c0.a;
            d.y = c2.a - c0.a;
            d.z = 0.0f;
            k3 = d * inv;
            u.x = hit.x - a.x;
            u.y = hit.y - a.y;
            u.z = hit.z - a.z;
            seg.m_hitColor.r = c0.r + (u.x * k0.x + u.y * k0.y + u.z * k0.z);
            seg.m_hitColor.g = c0.g + (u.x * k1.x + u.y * k1.y + u.z * k1.z);
            seg.m_hitColor.b = c0.b + (u.x * k2.x + u.y * k2.y + u.z * k2.z);
            seg.m_hitColor.a = c0.a + (u.x * k3.x + u.y * k3.y + u.z * k3.z);
            return 1;
        }
        // The fallback is written in BOTH failing arms: each copy ends its
        // own block, so the join's destructor calls cannot be scheduled into
        // the stores, and jump2 cross-jumps the two copies into one after
        // sched2 -- the surviving copy is the last in source order.
        PickColorTo(seg.m_hitColor, ca);
    }
    else
        PickColorTo(seg.m_hitColor, ca);
    return 0;
}
