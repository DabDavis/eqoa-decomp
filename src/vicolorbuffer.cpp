// VIColorBuffer -- SUPPORT 0x003FB878 .. 0x003FC170.
#include "vi.h"

VIPrimBuffer *VIColorBuffer::pBuildSource;
unsigned int *VIColorBuffer::pBuildDMATag;
int VIColorBuffer::BuildVQSource;
int VIColorBuffer::BuildI;
int VIColorBuffer::BuildM;
int VIColorBuffer::BuildMLeft;
int VIColorBuffer::BuildP;
int VIColorBuffer::BuildPLeft;

VIColorBuffer::VIColorBuffer()
{
    m_data = 0;
    Clear();
}

VIColorBuffer::~VIColorBuffer()
{
    Clear();
}

int VIColorBuffer::Clear()
{
    if (m_data)
        delete [] m_data;

    m_locked = 0;
    m_primBuffer = -1;
    m_pad10 = 0;
    m_vertCursor = 0;
    m_numVerts = 0;
    m_pad1C = 0;
    m_numTags = 0;
    m_headers = 0;
    m_verts = 0;
    m_tags = 0;
    m_dataSize = 0;
    m_data = 0;
    return 0;
}

int VIColorBuffer::Init(int i, VIRaster &r)
{
    m_primBuffer = i;

    if (r.SharePrimBuffer(i) < 0)
        return -1;

    if (CalcDataSize(*r.PrimBuffer(i)) < 0)
        return -1;

    if (Allocate() < 0)
        return -1;

    m_headers = m_data + (int)m_headers;
    m_verts = m_data + (int)m_verts;
    m_tags = m_data + (int)m_tags;
    return 0;
}

int VIColorBuffer::Clear(VIRaster &r, VIDictionary &d)
{
    if (m_primBuffer != -1)
    {
        r.ReleasePrimBuffer(m_primBuffer, d);
        m_primBuffer = -1;
    }
    return Clear();
}

int VIColorBuffer::Lock(VIRaster &r)
{
    if (m_locked)
        return -1;

    m_locked = 1;
    pBuildSource = r.PrimBuffer(m_primBuffer);
    pBuildDMATag = (unsigned int *)(m_verts + m_vertCursor * 16);
    BuildVQSource = 0;
    BuildI = 0;
    BuildM = -1;
    BuildMLeft = 0;
    BuildP = -1;
    BuildPLeft = 0;
    return 0;
}

int VIColorBuffer::Unlock()
{
    if (!m_locked)
        return -1;

    m_locked = 0;
    pBuildSource = 0;
    pBuildDMATag = 0;
    BuildVQSource = 0;
    BuildI = 0;
    BuildM = -1;
    BuildMLeft = 0;
    BuildP = -1;
    BuildPLeft = 0;
    return 0;
}

int VIColorBuffer::CalcDataSize(VIPrimBuffer &pb)
{
    if (pb.m_type == 2)
    {
        m_dataSize = 0;
        m_headers = 0;

        int n = pb.m_numMaterials * 8;
        int off;

        m_dataSize = n;
        if (n & 15)
        {
            int t = n + 16;

            off = t - (n & 15);
        }
        else
            off = n;
        m_dataSize = off;
        m_numVerts = pb.m_numPrims * 2 + pb.m_numMaterials;
        m_verts = (char *)off;
        int a = off + m_numVerts * 16;

        m_dataSize = a;
        if (a & 15)
        {
            int t = a + 16;

            a = t - (a & 15);
        }
        m_dataSize = a;

        m_numTags = 0;
        for (int i = 0; i < pb.m_numPrims; i++)
        {
            int n = pb.m_prims[i].m_numVerts + 4;

            if ((n & 3) == 0)
                m_numTags += n / 4;
            else
                m_numTags += n / 4 + 1;
        }

        int b = m_dataSize + m_numTags * 16;

        m_tags = (char *)m_dataSize;
        m_dataSize = b;
        if (b & 15)
        {
            int t = b + 16;

            b = t - (b & 15);
        }
        m_dataSize = b;

    }
    else
        return -1;

    return 0;
}

int VIColorBuffer::Allocate()
{
    if (m_dataSize)
    {
        m_data = new char[m_dataSize];
        if (m_data == 0)
            return -1;
    }
    return 0;
}

// One word into the VIF stream at m_tags, four to a quadword: m_pad1C is the
// quadword cursor and BuildI the word within it.  Indexed through VIQWord
// (160 -> 104): the cast-and-add spelling expands the three address terms in
// another order and the first EMIT's registers swap.  The increment is its
// OWN statement (phase 61 wave 6, 14 -> 6): the store is in-struct at a
// varying address, which never conflicts with a fixed scalar global in cse,
// so BuildI is not reloaded, and `BuildI + 1` lands in a NEW pseudo.  A
// post-increment in the subscript instead reuses the loaded register, so the
// add anti-depends on the shift that reads it; sched1 then issues it beside
// the m_tags sum and local-alloc ranks BuildI*4 and the m_tags load equal --
// the two tail EMITs' registers swapped.  (With the old cast store, which is
// NOT in-struct, a separate increment did reload BuildI.)
#define EMIT(w)                                                       \
    {                                                                 \
        ((VIQWord *)m_tags)[m_pad1C].m_w[BuildI] = (w);               \
        BuildI++;                                                     \
        if (BuildI == 4)                                              \
        {                                                             \
            BuildI = 0;                                               \
            m_pad1C++;                                                \
        }                                                             \
    }

// A DMA tag at pBuildDMATag: the id byte, the address, the quadword count --
// with both values computed before the first byte is stored.
// The tag is written through sceDmaTag's own shape -- a POINTER `next`, the
// type pBuildDMATag itself is read through -- not cast stores.  Phase 61
// wave 5, read off the alias checks (186 -> 176): the target reloads
// pBuildDMATag after the address store (a store of the pointer's own type
// may alias it) yet schedules the BuildPLeft reload ABOVE that store (a
// struct field at a varying address never conflicts with a fixed int
// global).  Cast `unsigned int` stores give neither.
struct VICBTag
{
    unsigned short m_qwc;
    unsigned char  m_mark;
    unsigned char  m_id;
    VICBTag       *m_next;
    unsigned int   m_p[2];
};
#define BTAG (*(VICBTag **)&pBuildDMATag)

#define SetTag(id_, qwc, addr)                                        \
    {                                                                 \
        int q_ = (qwc);                                               \
        unsigned int a_ = (addr);                                     \
        BTAG->m_id = (id_);                                           \
        BTAG->m_next = (VICBTag *)a_;                                 \
        BTAG->m_qwc = q_;                                             \
    }

// The material table at m_headers: two words per material, the material and
// the vertex cursor its run starts at.  m_pad10 is its cursor.
struct VIColorBufferHeader
{
    int m_mat;
    int m_vert;
};

// 0x003FBAF0.  One baked vertex colour, walking the locked prim buffer's
// materials and strips in step: a new material writes a header; a new strip
// writes a REF tag for its vertices and one for the colours that follow, and
// opens a GIF strip of its length; the colour itself goes in at half
// intensity; the end of a strip closes it with an MSCAL of RStrip2Offset and
// pads the quadword; the end of a material writes a tag of id 6 (RET).
int VIColorBuffer::Color(VIColor32 &c)
{
    if (BuildMLeft == 0)
    {
        if (m_pad10 == pBuildSource->m_numMaterials)
            return -1;
        BuildM++;
        BuildMLeft = pBuildSource->m_materials[BuildM].m_numPrims;
        ((VIColorBufferHeader *)m_headers)[m_pad10].m_mat =
            pBuildSource->m_materials[BuildM].m_matIdx;
        ((VIColorBufferHeader *)m_headers)[m_pad10].m_vert = m_vertCursor;
    }

    if (BuildPLeft == 0)
    {
        BuildP++;
        BuildPLeft = pBuildSource->m_prims[BuildP].m_numVerts;
        if (pBuildSource->m_type != 2)
            return -1;

        // The INTEGER is the first operand (176 -> 14, and the s0 frame
        // gone): `m_verts + i*16` is a pointer sum, which C++ always builds
        // pointer-first, so local-alloc ties the sum to the m_verts load and
        // the `n & 3` test further down is left only t0 -- n then needs a
        // callee-saved register.  Integer-first, the sum ties to the shift
        // (expand_binop keeps a REG first operand) and m_verts dies early.
        SetTag(48, BuildPLeft * 2 + 2,
               BuildVQSource * 16 + (unsigned int)pBuildSource->m_verts);
        BuildVQSource += 2;
        BTAG++;
        m_vertCursor++;

        // The tag step is written in BOTH arms (jump2 cross-jumps them after
        // scheduling): as one join the hi parts of pBuildDMATag live across
        // the if/else in a gcse copy, and the extra register costs a frame.
        // Integer-first here as well (6 -> 0), for the same tie as m_verts.
        int n = BuildPLeft + 4;
        if ((n & 3) == 0)
        {
            SetTag(48, n / 4, m_pad1C * 16 + (unsigned int)m_tags);
            BTAG++;
        }
        else
        {
            SetTag(48, n / 4 + 1, m_pad1C * 16 + (unsigned int)m_tags);
            BTAG++;
        }
        m_vertCursor++;

        EMIT(0x01000104);
        EMIT((pBuildSource->m_prims[BuildP].m_numVerts << 16) | 0x6E008004);
    }

    unsigned char *p = (unsigned char *)(m_tags + m_pad1C * 16 + BuildI * 4);
    p[0] = c.r >> 1;
    p[1] = c.g >> 1;
    p[2] = c.b >> 1;
    p[3] = c.a >> 1;
    BuildI++;
    if (BuildI == 4)
    {
        BuildI = 0;
        m_pad1C++;
    }

    if (pBuildSource->m_type != 2)
        return -1;
    BuildPLeft--;
    BuildVQSource += 2;
    if (BuildPLeft == 0)
    {
        BuildMLeft--;
        EMIT(VIRaster::RStrip2Offset | 0x14000000);
        EMIT(0x01000404);
        while (BuildI != 0)
        {
            EMIT(0);
        }
        BuildVQSource++;
    }

    if (BuildMLeft == 0)
    {
        SetTag(96, 0, 0);
        BTAG++;
        m_vertCursor++;
        m_pad10++;
    }
    return 0;
}
