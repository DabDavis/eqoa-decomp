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
