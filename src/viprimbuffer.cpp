// VIPrimBuffer -- SUPPORT 0x003FD7F0 .. 0x003FF2E8.
//
// VICollBuffer's render-side sibling: the same VIBuffer/VIBBox skeleton and the
// same three-section allocation, but what it builds is a GS display list.  Every
// vertex is one or more QUADWORDS carrying a GIFtag, so the vertex section is
// indexed by `<< 4` and CalcDataSize counts quadwords.
//
// The twelve file-scope statics are STATIC MEMBERS of the class -- the symbol
// map spells them `_12VIPrimBuffer$PrimUV0` -- and they are the current vertex
// attributes: a setter stamps one, and Vertex() reads whichever the packing
// needs.  gcc 2.x constructs them from a STATIC
// __static_initialization_and_destruction, which is absent from the symbol map
// and so has to be capped out of Allocate's derived span.
#include "vi.h"

VIVect2    VIPrimBuffer::PrimUV0;
VIVect2S16 VIPrimBuffer::PrimUV0S16;
VIVect2    VIPrimBuffer::PrimUV1;
VIVect2S16 VIPrimBuffer::PrimUV1S16;
VIVect3    VIPrimBuffer::PrimNormal;
VIVect3S8  VIPrimBuffer::PrimNormalS8;
VIColor32  VIPrimBuffer::PrimColor;
VIVect4U8  VIPrimBuffer::PrimBones;
VIVect4    VIPrimBuffer::PrimWeights;
VIVect4S8  VIPrimBuffer::PrimWeightsS8;
int        VIPrimBuffer::PrimVQStart;
unsigned short VIPrimBuffer::PrimVertexGroup;

// `if (n & 15) n += 16 - (n & 15);` written the long way: the target computes
// the mask, branches on it, and subtracts it from n+16.  A `(n + 15) & ~15`
// gives two instructions and no branch.
inline static int Align16(int n)
{
    // A FRESH local for `n + 16`, and it is load-bearing.  Written
    // `r = n + 16; r = r - m;` the allocator coalesces r with n and the whole
    // diamond collapses to three in-place instructions; with `t` the target's
    // shape falls out -- n keeps its own register, the else arm's `move` and
    // its branch survive, and r takes the MASK's register.  Phase 9's
    // fresh-local-per-term rule, in integer code.
    int m = n & 15;
    int t;
    int r;
    if (m)
    {
        t = n + 16;
        r = t - m;
    }
    else
    {
        r = n;
    }
    return r;
}

VIPrimBuffer::VIPrimBuffer()
{
    m_data = 0;
    Clear();
}

VIPrimBuffer::~VIPrimBuffer()
{
    Clear();
}

int VIPrimBuffer::Clear()
{
    if (m_data)
        delete [] m_data;
    m_type = 0;
    m_locked = 0;
    m_userFlags = 0;
    VIVect4 *sc = &m_scale;
    sc->x = 0.0f;
    sc->y = 0.0f;
    sc->z = 0.0f;
    sc->w = 0.0f;
    // Two levers here.  The float statics are CHAINED, last member first, and
    // the chain is what makes them `swc1` off a register instead of `sw $0`:
    // one temporary holds the 0.0f for all eleven, and cse then replaces that
    // register's CONSTANT source with the cheaper equivalent MEM it has just
    // seen -- m_scale.y, zeroed three statements up.  Written as separate
    // statements each store is `(set (mem) (const_double 0))`, mem-to-mem is
    // invalid, and every one comes out `sw $0`.  The byte and halfword statics
    // are NOT chained: an integer zero is free, so they need no temporary, and
    // their store order is the plain source order.
    PrimVQStart = 0;
    PrimUV0.y = PrimUV0.x = 0.0f;
    PrimUV0S16.x = 0;
    PrimUV0S16.y = 0;
    PrimUV1.y = PrimUV1.x = 0.0f;
    PrimUV1S16.x = 0;
    PrimUV1S16.y = 0;
    PrimNormal.z = PrimNormal.y = PrimNormal.x = 0.0f;
    PrimNormalS8.x = 0;
    PrimNormalS8.y = 0;
    PrimNormalS8.z = 0;
    PrimColor.r = 255;
    PrimColor.g = 255;
    PrimColor.b = 255;
    PrimColor.a = 255;
    PrimBones.x = 255;
    PrimBones.y = 255;
    PrimBones.z = 255;
    PrimBones.w = 255;
    PrimWeights.w = PrimWeights.z = PrimWeights.y = PrimWeights.x = 0.0f;
    PrimWeightsS8.x = 0;
    PrimWeightsS8.y = 0;
    PrimWeightsS8.z = 0;
    PrimWeightsS8.w = 0;
    m_numMaterials = 0;
    m_maxMaterials = 0;
    m_numPrims = 0;
    m_maxPrims = 0;
    m_numVerts = 0;
    m_maxVerts = 0;
    m_materials = 0;
    m_prims = 0;
    m_verts = 0;
    m_dataSize = 0;
    m_data = 0;
    return 0;
}

// The VIRaster & is DISCARDED, the way CalcBVolumes discards its int.
int VIPrimBuffer::Init(VIRaster &r, VIPrimBufferType t, int mats, int prims, int verts)
{
    m_type = t;
    m_maxMaterials = mats;
    m_maxPrims = prims;
    if (CalcDataSize(mats, prims, verts) < 0)
        return -1;
    if (Allocate() < 0)
        return -1;
    m_materials = (VIPrimMaterial *)((int)m_data + (int)m_materials);
    m_prims = (VIPrimPrim *)((int)m_data + (int)m_prims);
    m_verts = (char *)((int)m_data + (int)m_verts);
    return 0;
}

int VIPrimBuffer::GetNumVertices() const
{
    int n = 0;
    switch (m_type)
    {
    case VIPrimBufferTypeVUNC:
    case VIPrimBufferTypePackVUNC:
    case VIPrimBufferTypePackVUNCG:
    case VIPrimBufferTypePackVUNBW:
    case VIPrimBufferTypePackVU:
    {
        for (int i = 0; i < m_numPrims; i++)
            n += m_prims[i].m_numVerts;
        break;
    }
    default:
        return -1;
    }
    return n;
}

int VIPrimBuffer::GetNumTriangles() const
{
    int n = 0;
    switch (m_type)
    {
    case VIPrimBufferTypeVUNC:
    {
        for (int i = 0; i < m_numPrims; i++)
            n += m_prims[i].m_numVerts - 2;
        break;
    }
    default:
        return -1;
    }
    return n;
}

int VIPrimBuffer::Lock()
{
    if (m_locked)
        return -1;
    m_locked = 1;
    return 0;
}

int VIPrimBuffer::Unlock()
{
    if (!m_locked)
        return -1;
    switch (m_type)
    {
    case VIPrimBufferTypePackVUNC:
        m_scale.w = 1.0f / 128.0f;
        break;
    case VIPrimBufferTypePackVUNCG:
        m_scale.w = 1.0f / 128.0f;
        break;
    case VIPrimBufferTypePackVUNBW:
        m_scale.w = 1.0f / 127.0f;
        break;
    case VIPrimBufferTypePackVU:
        // z BEFORE w, and it is the store order that proves it: gcc emits this
        // pair in reverse, so w's store lands FIRST and the block's last insn
        // is z's.  Written w-then-z, the trailing `m_scale.w = ...` matches the
        // default arm's and cross-jumps into it, losing the store entirely.
        m_scale.z = 1.0f / 128.0f;
        m_scale.w = 1.0f / 128.0f;
        break;
    default:
        m_scale.w = 1.0f;
        break;
    }
    m_locked = 0;
    return 0;
}

int VIPrimBuffer::SetPacking(int v, int uv, int n)
{
    m_scale.x = 1.0f / powf(2.0f, (float)v);
    m_scale.y = 1.0f / powf(2.0f, (float)uv);
    m_scale.z = 1.0f / powf(2.0f, (float)n);
    return 0;
}

int VIPrimBuffer::BeginMaterial(int mat)
{
    m_materials[m_numMaterials].m_matIdx = mat;
    m_materials[m_numMaterials].m_firstVert = m_numVerts;
    m_materials[m_numMaterials].m_numVerts = 0;
    m_materials[m_numMaterials].m_numPrims = 0;
    return 0;
}

int VIPrimBuffer::EndMaterial()
{
    m_materials[m_numMaterials].m_numVerts =
        m_numVerts - m_materials[m_numMaterials].m_firstVert;
    m_numMaterials++;
    return 0;
}

// The `int` parameter is UNUSED -- a1 is never read.  Two quadwords are
// reserved for the GIFtag pair End() comes back and fills in, and PrimVQStart
// remembers where they are.
int VIPrimBuffer::Begin(int prim)
{
    switch (m_type)
    {
    case VIPrimBufferTypeVUNC:
    case VIPrimBufferTypePackVUNC:
    case VIPrimBufferTypePackVUNCG:
    case VIPrimBufferTypePackVUNBW:
    case VIPrimBufferTypePackVU:
        m_prims[m_numPrims].m_numVerts = 0;
        PrimVQStart = m_numVerts;
        ((VIVect4 *)m_verts)[m_numVerts].x = 0.0f;
        ((VIVect4 *)m_verts)[m_numVerts].y = 0.0f;
        ((VIVect4 *)m_verts)[m_numVerts].z = 0.0f;
        ((VIVect4 *)m_verts)[m_numVerts].w = 0.0f;
        m_numVerts++;
        ((VIVect4 *)m_verts)[m_numVerts].x = 0.0f;
        ((VIVect4 *)m_verts)[m_numVerts].y = 0.0f;
        ((VIVect4 *)m_verts)[m_numVerts].z = 0.0f;
        ((VIVect4 *)m_verts)[m_numVerts].w = 0.0f;
        m_numVerts++;
        break;
    default:
        return -1;
    }
    return 0;
}

int VIPrimBuffer::Vertex(VIVect3 &v)
{
    switch (m_type)
    {
    case VIPrimBufferTypeVUNC:
    {
        // Declared INSIDE the case: the zeroing constructor's four stack stores
        // land after the switch test, where the target has them.  At function
        // scope they are hoisted above it.
        VIVect4 c;
        m_prims[m_numPrims].m_numVerts++;
        ((VIVect4 *)m_verts)[m_numVerts].x = v.x;
        ((VIVect4 *)m_verts)[m_numVerts].y = v.y;
        ((VIVect4 *)m_verts)[m_numVerts].z = v.z;
        ((VIVect4 *)m_verts)[m_numVerts].w = 1.0f;
        m_numVerts++;
        ((VIVect4 *)m_verts)[m_numVerts].x = PrimUV0.x;
        ((VIVect4 *)m_verts)[m_numVerts].y = PrimUV0.y;
        ((VIVect4 *)m_verts)[m_numVerts].z = 1.0f;
        ((VIVect4 *)m_verts)[m_numVerts].w = 0.0f;
        m_numVerts++;
        ((VIVect4 *)m_verts)[m_numVerts].x = PrimNormal.x;
        ((VIVect4 *)m_verts)[m_numVerts].y = PrimNormal.y;
        ((VIVect4 *)m_verts)[m_numVerts].z = PrimNormal.z;
        ((VIVect4 *)m_verts)[m_numVerts].w = 0.0f;
        m_numVerts++;
        c.x = (float)PrimColor.r * (1.0f / 255.0f);
        c.y = (float)PrimColor.g * (1.0f / 255.0f);
        c.z = (float)PrimColor.b * (1.0f / 255.0f);
        c.w = (float)PrimColor.a * (1.0f / 255.0f);
        ((VIVect4 *)m_verts)[m_numVerts].x = c.x;
        ((VIVect4 *)m_verts)[m_numVerts].y = c.y;
        ((VIVect4 *)m_verts)[m_numVerts].z = c.z;
        ((VIVect4 *)m_verts)[m_numVerts].w = c.w;
        m_numVerts++;
        break;
    }
    default:
        return -1;
    }
    return 0;
}

int VIPrimBuffer::Vertex(VIVect3S16 &v)
{
    switch (m_type)
    {
    case VIPrimBufferTypePackVUNC:
    {
        char *p = m_verts + m_numVerts * 16;
        *(int *)(p + 0) = (m_prims[m_numPrims].m_numVerts << 2) | 0x6D028001;
        *(short *)(p + 4) = v.x;
        *(short *)(p + 6) = v.y;
        *(short *)(p + 8) = v.z;
        *(short *)(p + 10) = (short)(1.0f / m_scale.x);
        *(short *)(p + 12) = PrimUV0S16.x;
        *(short *)(p + 14) = PrimUV0S16.y;
        *(short *)(p + 16) = (short)(1.0f / m_scale.y);
        *(short *)(p + 18) = 0;
        *(int *)(p + 20) = (m_prims[m_numPrims].m_numVerts << 2) | 0x6E028003;
        *(signed char *)(p + 24) = PrimNormalS8.x;
        *(signed char *)(p + 25) = PrimNormalS8.y;
        *(signed char *)(p + 26) = PrimNormalS8.z;
        *(signed char *)(p + 27) = 0;
        unsigned char *pc = (unsigned char *)(p + 28);
        pc[0] = PrimColor.r >> 1;
        pc[1] = PrimColor.g >> 1;
        pc[2] = PrimColor.b >> 1;
        pc[3] = PrimColor.a >> 1;
        m_prims[m_numPrims].m_numVerts++;
        m_numVerts += 2;
        break;
    }
    case VIPrimBufferTypePackVUNCG:
    {
        char *p = m_verts + m_numVerts * 16;
        unsigned short g = PrimVertexGroup;
        *(int *)(p + 0) = (m_prims[m_numPrims].m_numVerts << 2) | 0x6D028001;
        VIVect3S16 *pv = (VIVect3S16 *)(p + 4);
        pv->x = v.x;
        pv->y = v.y;
        pv->z = v.z;
        *(short *)(p + 10) = g;
        *(short *)(p + 12) = PrimUV0S16.x;
        *(short *)(p + 14) = PrimUV0S16.y;
        *(short *)(p + 16) = (short)(1.0f / m_scale.y);
        *(short *)(p + 18) = 0;
        *(int *)(p + 20) = (m_prims[m_numPrims].m_numVerts << 2) | 0x6E028003;
        *(signed char *)(p + 24) = PrimNormalS8.x;
        *(signed char *)(p + 25) = PrimNormalS8.y;
        *(signed char *)(p + 26) = PrimNormalS8.z;
        *(signed char *)(p + 27) = 0;
        unsigned char *pc = (unsigned char *)(p + 28);
        pc[0] = PrimColor.r >> 1;
        pc[1] = PrimColor.g >> 1;
        pc[2] = PrimColor.b >> 1;
        pc[3] = PrimColor.a >> 1;
        m_prims[m_numPrims].m_numVerts++;
        m_numVerts += 2;
        break;
    }
    case VIPrimBufferTypePackVUNBW:
    {
        char *p = m_verts + m_numVerts * 16;
        *(int *)(p + 0) = (m_prims[m_numPrims].m_numVerts * 5 + 1) | 0x69028000;
        VIVect3S16 *pv = (VIVect3S16 *)(p + 4);
        pv->x = v.x;
        pv->y = v.y;
        pv->z = v.z;
        *(short *)(p + 10) = PrimUV0S16.x;
        *(short *)(p + 12) = PrimUV0S16.y;
        *(short *)(p + 14) = (short)(1.0f / m_scale.y);
        *(int *)(p + 16) = (m_prims[m_numPrims].m_numVerts * 5 + 3) | 0x6E038000;
        *(signed char *)(p + 20) = PrimNormalS8.x;
        *(signed char *)(p + 21) = PrimNormalS8.y;
        *(signed char *)(p + 22) = PrimNormalS8.z;
        *(signed char *)(p + 23) = 0;
        *(unsigned char *)(p + 28) = PrimBones.x == 255 ? 0 : PrimBones.x;
        *(unsigned char *)(p + 29) = PrimBones.y == 255 ? 0 : PrimBones.y;
        *(unsigned char *)(p + 30) = PrimBones.z == 255 ? 0 : PrimBones.z;
        *(unsigned char *)(p + 31) = PrimBones.w == 255 ? 0 : PrimBones.w;
        signed char *pw = (signed char *)(p + 24);
        pw[0] = PrimWeightsS8.x;
        pw[1] = PrimWeightsS8.y;
        pw[2] = PrimWeightsS8.z;
        pw[3] = PrimWeightsS8.w;
        m_prims[m_numPrims].m_numVerts++;
        m_numVerts += 2;
        break;
    }
    case VIPrimBufferTypePackVU:
    {
        char *p = m_verts + m_numVerts * 16;
        *(int *)(p + 0) = (m_prims[m_numPrims].m_numVerts << 1) | 0x69028001;
        *(short *)(p + 4) = v.x;
        *(short *)(p + 6) = v.y;
        *(short *)(p + 8) = v.z;
        *(short *)(p + 10) = PrimUV0S16.x;
        *(short *)(p + 12) = PrimUV0S16.y;
        *(short *)(p + 14) = (short)(1.0f / m_scale.y);
        m_prims[m_numPrims].m_numVerts++;
        m_numVerts += 1;
        break;
    }
    default:
        return -1;
    }
    return 0;
}

int VIPrimBuffer::UV0(VIVect2 &v)
{
    PrimUV0 = v;
    return 0;
}

int VIPrimBuffer::UV0(VIVect2S16 &v)
{
    PrimUV0S16 = v;
    return 0;
}

int VIPrimBuffer::UV1(VIVect2 &v)
{
    PrimUV1 = v;
    return 0;
}

int VIPrimBuffer::UV1(VIVect2S16 &v)
{
    PrimUV1S16 = v;
    return 0;
}

int VIPrimBuffer::Normal(VIVect3 &v)
{
    PrimNormal = v;
    return 0;
}

int VIPrimBuffer::Normal(VIVect3S8 &v)
{
    PrimNormalS8 = v;
    return 0;
}

int VIPrimBuffer::Color(VIColor32 &c)
{
    PrimColor = c;
    return 0;
}

int VIPrimBuffer::Bones(VIVect4U8 &b)
{
    PrimBones = b;
    return 0;
}

int VIPrimBuffer::Weights(VIVect4 &w)
{
    PrimWeights.Set(w);
    return 0;
}

int VIPrimBuffer::Weights(VIVect4S8 &w)
{
    PrimWeightsS8 = w;
    return 0;
}

int VIPrimBuffer::VertexGroup(short g)
{
    PrimVertexGroup = g;
    return 0;
}

// Three locals per case, and their ORDER is what shapes the cross-jump.  The
// VU1 microprogram offset must be the LAST case-specific value read: the
// backward walk stops on it, so everything before it -- the quadword start, the
// VIFcode, the vertex-section load and the shift -- stays duplicated in the
// five prefixes and everything after merges into one tail.  Read `prog` before
// `tag` and the constant merges too, leaving prefixes two instructions short.
//
// `tag` is a VIFcode: 0x6C = UNPACK V4-32, NUM in bits 16-23.  The last word
// written is `prog | 0x14000000`, and 0x14 is MSCAL -- so `prog` names the VU1
// microprogram that will consume the vertices, one per format.
int VIPrimBuffer::End()
{
    switch (m_type)
    {
    case VIPrimBufferTypeVUNC:
    {
        int s = PrimVQStart;
        unsigned int tag = ((m_prims[m_numPrims].m_numVerts * 4 + 1) << 16) | 0x6C008000;
        int prog = VIRaster::RStripOffset;
        *(int *)(m_verts + s * 16 + 12) = tag;
        *(int *)(m_verts + s * 16 + 16) = m_prims[m_numPrims].m_numVerts;
        *(int *)(m_verts + s * 16 + 20) = 0;
        *(int *)(m_verts + s * 16 + 24) = 0;
        *(int *)(m_verts + s * 16 + 28) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 0) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 4) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 8) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 12) = prog | 0x14000000;
        m_numVerts++;
        m_materials[m_numMaterials].m_numPrims++;
        m_numPrims++;
        break;
    }
    case VIPrimBufferTypePackVUNC:
    {
        int s = PrimVQStart;
        unsigned int tag = 0x6C018000;
        int prog = VIRaster::RStrip2Offset;
        *(int *)(m_verts + s * 16 + 12) = tag;
        *(int *)(m_verts + s * 16 + 16) = m_prims[m_numPrims].m_numVerts;
        *(int *)(m_verts + s * 16 + 20) = 0;
        *(int *)(m_verts + s * 16 + 24) = 0;
        *(int *)(m_verts + s * 16 + 28) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 0) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 4) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 8) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 12) = prog | 0x14000000;
        m_numVerts++;
        m_materials[m_numMaterials].m_numPrims++;
        m_numPrims++;
        break;
    }
    case VIPrimBufferTypePackVUNCG:
    {
        int s = PrimVQStart;
        unsigned int tag = 0x6C018000;
        int prog = VIRaster::RStrip3Offset;
        *(int *)(m_verts + s * 16 + 12) = tag;
        *(int *)(m_verts + s * 16 + 16) = m_prims[m_numPrims].m_numVerts;
        *(int *)(m_verts + s * 16 + 20) = 0;
        *(int *)(m_verts + s * 16 + 24) = 0;
        *(int *)(m_verts + s * 16 + 28) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 0) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 4) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 8) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 12) = prog | 0x14000000;
        m_numVerts++;
        m_materials[m_numMaterials].m_numPrims++;
        m_numPrims++;
        break;
    }
    case VIPrimBufferTypePackVUNBW:
    {
        int s = PrimVQStart;
        unsigned int tag = 0x6C018000;
        int prog = VIRaster::RStrip4Offset;
        *(int *)(m_verts + s * 16 + 12) = tag;
        *(int *)(m_verts + s * 16 + 16) = m_prims[m_numPrims].m_numVerts;
        *(int *)(m_verts + s * 16 + 20) = 0;
        *(int *)(m_verts + s * 16 + 24) = 0;
        *(int *)(m_verts + s * 16 + 28) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 0) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 4) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 8) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 12) = prog | 0x14000000;
        m_numVerts++;
        m_materials[m_numMaterials].m_numPrims++;
        m_numPrims++;
        break;
    }
    case VIPrimBufferTypePackVU:
    {
        int s = PrimVQStart;
        unsigned int tag = 0x6C018000;
        int prog = VIRaster::RStrip5Offset;
        *(int *)(m_verts + s * 16 + 12) = tag;
        *(int *)(m_verts + s * 16 + 16) = m_prims[m_numPrims].m_numVerts;
        *(int *)(m_verts + s * 16 + 20) = 0;
        *(int *)(m_verts + s * 16 + 24) = 0;
        *(int *)(m_verts + s * 16 + 28) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 0) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 4) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 8) = 0;
        *(int *)(m_verts + m_numVerts * 16 + 12) = prog | 0x14000000;
        m_numVerts++;
        m_materials[m_numMaterials].m_numPrims++;
        m_numPrims++;
        break;
    }
    default:
        return -1;
    }
    return 0;
}

int VIPrimBuffer::CalcBVolumes(int n, VIVect3 *p)
{
    VIBBox gb;
    VIBBox pb;
    int mi, pi, first;

    first = 1;
    pi = 0;
    // ONE walk over the vertex data for the whole buffer: the materials' prims
    // are contiguous, so vp is not reset per material.
    char *vp = m_verts;

    for (mi = 0; mi < m_numMaterials; mi++)
    {
        int fg = 1;
        int np = m_materials[mi].m_numPrims;
        while (np != 0)
        {
            int fp = 1;
            int nv = m_prims[pi].m_numVerts;

            switch (m_type)
            {
            case VIPrimBufferTypeVUNC:
            case VIPrimBufferTypePackVUNC:
            case VIPrimBufferTypePackVUNCG:
            case VIPrimBufferTypePackVUNBW:
            case VIPrimBufferTypePackVU:
                vp += 32;
                break;
            default:
                return -1;
            }

            while (nv != 0)
            {
                switch (m_type)
                {
                case VIPrimBufferTypeVUNC:
                {
                    // `v` is constructed in EACH case -- its three zeroing
                    // stores appear in all five bodies -- and Bound is written
                    // in with it and left to cross-jump into one call.
                    VIVect3 v;
                    v.x = ((VIVect4 *)vp)->x;
                    v.y = ((VIVect4 *)vp)->y;
                    v.z = ((VIVect4 *)vp)->z;
                    vp += 64;
                    pb.Bound(fp, v);
                    break;
                }
                case VIPrimBufferTypePackVUNC:
                {
                    VIVect3 v;
                    VIVect3S16 *pv = (VIVect3S16 *)(vp + 4);
                    v.x = (float)pv->x * m_scale.x;
                    v.y = (float)pv->y * m_scale.x;
                    v.z = (float)pv->z * m_scale.x;
                    vp += 32;
                    pb.Bound(fp, v);
                    break;
                }
                case VIPrimBufferTypePackVUNCG:
                {
                    VIVect3 v;
                    VIVect3S16 *pv = (VIVect3S16 *)(vp + 4);
                    v.x = (float)pv->x * m_scale.x;
                    v.y = (float)pv->y * m_scale.x;
                    v.z = (float)pv->z * m_scale.x;
                    // the group index is read AFTER the three scalings: the
                    // target's multiplies all retire before the table lookup
                    short g = *(short *)(vp + 10);
                    vp += 32;
                    v += p[g];
                    pb.Bound(fp, v);
                    break;
                }
                case VIPrimBufferTypePackVUNBW:
                {
                    VIVect3 v;
                    VIVect3S16 *pv = (VIVect3S16 *)(vp + 4);
                    v.x = (float)pv->x * m_scale.x;
                    v.y = (float)pv->y * m_scale.x;
                    v.z = (float)pv->z * m_scale.x;
                    vp += 32;
                    pb.Bound(fp, v);
                    break;
                }
                case VIPrimBufferTypePackVU:
                {
                    VIVect3 v;
                    VIVect3S16 *pv = (VIVect3S16 *)(vp + 4);
                    v.x = (float)pv->x * m_scale.x;
                    v.y = (float)pv->y * m_scale.x;
                    v.z = (float)pv->z * m_scale.x;
                    vp += 16;
                    pb.Bound(fp, v);
                    break;
                }
                default:
                    return -1;
                }

                fp = 0;
                nv--;
            }

            switch (m_type)
            {
            case VIPrimBufferTypeVUNC:
            case VIPrimBufferTypePackVUNC:
            case VIPrimBufferTypePackVUNCG:
            case VIPrimBufferTypePackVUNBW:
            case VIPrimBufferTypePackVU:
                vp += 16;
                break;
            default:
                return -1;
            }

            m_prims[pi].m_bbox = pb;
            if (fg)
            {
                gb = pb;
                fg = 0;
            }
            else
            {
                gb.Union(gb, pb);
            }
            pi++;
            np--;
        }
        if (first)
        {
            m_bbox = gb;
            first = 0;
        }
        else
        {
            m_bbox.Union(m_bbox, gb);
        }
    }
    return 0;
}

// The closing three statements are written into EACH case and left to
// cross-jump.  Written once after the switch there is no tail to merge, so the
// FIRST thing cross-jumping finds is the whole identical body of PackVUNC /
// PackVUNCG / PackVUNBW and all three collapse into one -- 87 instructions
// against the target's 153.  Merged from the back instead, the pass takes the
// shared tail and does NOT come back for the now-identical prefixes.
int VIPrimBuffer::CalcDataSize(int mats, int prims, int verts)
{
    int nv;

    switch (m_type)
    {
    case VIPrimBufferTypeVUNC:
        m_materials = 0;
        m_dataSize = mats * 16;
        m_prims = (VIPrimPrim *)Align16(m_dataSize);
        m_dataSize = (int)m_prims + prims * 28;
        m_verts = (char *)Align16(m_dataSize);
        nv = prims * 3 + verts * 4;
        m_maxVerts = nv;
        m_dataSize = (int)m_verts + nv * 16;
        m_dataSize = Align16(m_dataSize);
        break;
    case VIPrimBufferTypePackVUNC:
        m_materials = 0;
        m_dataSize = mats * 16;
        m_prims = (VIPrimPrim *)Align16(m_dataSize);
        m_dataSize = (int)m_prims + prims * 28;
        m_verts = (char *)Align16(m_dataSize);
        nv = prims * 3 + verts * 2;
        m_maxVerts = nv;
        m_dataSize = (int)m_verts + nv * 16;
        m_dataSize = Align16(m_dataSize);
        break;
    case VIPrimBufferTypePackVUNCG:
        m_materials = 0;
        m_dataSize = mats * 16;
        m_prims = (VIPrimPrim *)Align16(m_dataSize);
        m_dataSize = (int)m_prims + prims * 28;
        m_verts = (char *)Align16(m_dataSize);
        nv = prims * 3 + verts * 2;
        m_maxVerts = nv;
        m_dataSize = (int)m_verts + nv * 16;
        m_dataSize = Align16(m_dataSize);
        break;
    case VIPrimBufferTypePackVUNBW:
        m_materials = 0;
        m_dataSize = mats * 16;
        m_prims = (VIPrimPrim *)Align16(m_dataSize);
        m_dataSize = (int)m_prims + prims * 28;
        m_verts = (char *)Align16(m_dataSize);
        nv = prims * 3 + verts * 2;
        m_maxVerts = nv;
        m_dataSize = (int)m_verts + nv * 16;
        m_dataSize = Align16(m_dataSize);
        break;
    case VIPrimBufferTypePackVU:
        m_materials = 0;
        m_dataSize = mats * 16;
        m_prims = (VIPrimPrim *)Align16(m_dataSize);
        m_dataSize = (int)m_prims + prims * 28;
        m_verts = (char *)Align16(m_dataSize);
        nv = prims * 3 + verts;
        m_maxVerts = nv;
        m_dataSize = (int)m_verts + nv * 16;
        m_dataSize = Align16(m_dataSize);
        break;
    default:
        return -1;
    }
    return 0;
}

int VIPrimBuffer::Allocate()
{
    if (m_dataSize)
    {
        m_data = new char[m_dataSize];
        if (m_data == 0)
            return -1;
    }
    return 0;
}
