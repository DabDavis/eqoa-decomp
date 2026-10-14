// VICSFFile -- SUPPORT 0x0049F6D0 .. 0x004A0094.  The compressed container:
// a 40-byte header, then one zlib stream per segment.  Decompress inflates each
// into its own block and hands the set to VIFile's scatter/gather memory open,
// so VIObjFile reads a .CSF exactly as it reads a flat .ESF.
#include "vi.h"

// CONST: the byte order is fixed for the run, so every read is unchanging
// memory.  gcse then turns the in-loop read into a copy of the first one and
// loop.c hoists that copy -- the target's second register for the same value
// (`move s5,s3` in Compress, a stack slot stored at the loop entry in
// Decompress).  Through one `int endian` local it is a single pseudo.
extern const int VIEndian;
void VIBufferSwap(int n, unsigned char *p);

extern "C" int printf(const char *f, ...);
extern "C" int compress2(unsigned char *dst, unsigned int *dstLen,
                         const unsigned char *src, unsigned int srcLen, int level);
extern "C" int uncompress(unsigned char *dst, unsigned int *dstLen,
                          const unsigned char *src, unsigned int srcLen);

unsigned int VICSFHeader::CSFFileID;

int VICSFFile::Compress(const char *src, const char *dst)
{
    VIFile in;
    VIFile out;
    VICSFHeader h;
    unsigned int destLen;
    // Set TWICE (= 0, then the allocation), so the pointer loses its alias
    // base: the +4 store through the loop's walker may alias the stack, and
    // destLen is re-read for the 64-bit total -- the target's `lwu`.
    VICSFSegm *recs = 0;
    char **blocks;
    int i;

    if (in.Open(src, 33) < 0) return -1;
    if (in.Seek(0, VIFile::Seek_End) < 0) return -1;
    unsigned int size = in.Tell();
    char *buf = new char[size];
    if (buf == 0) return -1;
    if (in.Seek(0, VIFile::Seek_Set) < 0) return -1;
    if (in.Read(buf, size) < 0) return -1;
    if (in.Close() < 0) return -1;

    h.m_numSegms = size >> 18;
    if (size & 0x3FFFF) h.m_numSegms++;
    recs = new VICSFSegm[h.m_numSegms];
    if (recs == 0) return -1;
    blocks = new char *[h.m_numSegms];
    if (blocks == 0) return -1;

    long remaining = size;
    char *p = buf;
    h.m_totalCompressed = 0;
    h.m_totalSize = remaining;
    h.m_maxCompressed = 0;
    for (i = 0; i < h.m_numSegms; i++)
    {
        int n = VIMIN(262144, remaining);

        destLen = n + (n >> 1);
        blocks[i] = new char[n + (n >> 1)];
        if (blocks[i] == 0) return -1;
        if (compress2((unsigned char *)blocks[i], &destLen,
                      (const unsigned char *)p, n, 9)) return -1;
        recs[i].m_compressed = destLen;
        recs[i].m_size = n;
        h.m_totalCompressed += destLen;
        h.m_maxCompressed = VIMAX((int)destLen, h.m_maxCompressed);
        p += n;
        remaining -= n;
    }
    if (buf) delete [] buf;
    if (in.Close() < 0) return -1;

    h.m_id = 0x46534543;
    h.m_offset = 40;
    if (out.Open(dst, 42) < 0) return -1;
    if (VIEndian != 1)
    {
        VIBufferSwap(4, (unsigned char *)&h.m_id);
        VIBufferSwap(4, (unsigned char *)&h.m_numSegms);
        VIBufferSwap(8, (unsigned char *)&h.m_totalCompressed);
        VIBufferSwap(8, (unsigned char *)&h.m_totalSize);
        VIBufferSwap(8, (unsigned char *)&h.m_offset);
        VIBufferSwap(4, (unsigned char *)&h.m_maxCompressed);
    }
    if (out.Write(&h, 40) < 0) return -1;
    if (VIEndian != 1)
    {
        VIBufferSwap(4, (unsigned char *)&h.m_id);
        VIBufferSwap(4, (unsigned char *)&h.m_numSegms);
        VIBufferSwap(8, (unsigned char *)&h.m_totalCompressed);
        VIBufferSwap(8, (unsigned char *)&h.m_totalSize);
        VIBufferSwap(8, (unsigned char *)&h.m_offset);
        VIBufferSwap(4, (unsigned char *)&h.m_maxCompressed);
    }
    for (i = 0; i < h.m_numSegms; i++)
    {
        if (VIEndian != 1)
        {
            VIBufferSwap(4, (unsigned char *)&recs[i].m_compressed);
            VIBufferSwap(4, (unsigned char *)&recs[i].m_size);
        }
        if (out.Write(&recs[i], 8) < 0) return -1;
        if (out.Write(blocks[i], recs[i].m_compressed) < 0) return -1;
        if (VIEndian != 1)
        {
            VIBufferSwap(4, (unsigned char *)&recs[i].m_compressed);
            VIBufferSwap(4, (unsigned char *)&recs[i].m_size);
        }
    }
    for (i = 0; i < h.m_numSegms; i++)
        if (blocks[i]) delete [] blocks[i];
    if (recs) delete [] recs;
    if (blocks) delete [] blocks;
    if (out.Close() < 0) return -1;
    return 0;
}

VICSFFile::VICSFFile()
{
    m_progress = 0;
    m_progressArg = 0;
}

int VICSFFile::Parse(const char *path, VIESFParse &p, VIScene &sc)
{
    VIFile f;

    printf("Decompressing %s\n", path);
    if (f.Open(path, 33) < 0) return -1;
    if (Parse(f, p, sc) < 0) return -1;
    return 0;
}

int VICSFFile::Parse(void *mem, long size, VIESFParse &p, VIScene &sc)
{
    VIFile f;

    printf("Decompressing memory file (%d bytes)\n", size);
    if (f.Open(mem, size, 33, size) < 0) return -1;
    if (Parse(f, p, sc) < 0) return -1;
    return 0;
}

void VICSFFile::SetProgressCallback(void (*fn)(void *), void *arg)
{
    m_progress = fn;
    m_progressArg = arg;
}

int VICSFFile::Parse(VIFile &f, VIESFParse &p, VIScene &sc)
{
    VIFile::MemSegm *segms = 0;
    VIObjFile o;
    int n;
    int i;

    if (Decompress(f, n, &segms) < 0) return -1;
    if (m_progress) m_progress(m_progressArg);
    long total = 0;
    for (i = 0; i < n; i++) total += segms[i].m_size;
    if (o.Open(n, segms, total, VIObjFile::Mode_Read) < 0) return -1;
    if (p.Parse(o, sc) < 0) return -1;
    for (i = 0; i < n; i++) delete (char *)segms[i].m_addr;
    if (segms) delete [] segms;
    segms = 0;
    return 0;
}

int VICSFFile::Decompress(VIFile &f, int &n, VIFile::MemSegm **segms)
{
    VICSFHeader h;

    if (f.Read(&h, 40) < 0) return -1;
    if (VIEndian != 1)
    {
        VIBufferSwap(4, (unsigned char *)&h.m_id);
        VIBufferSwap(4, (unsigned char *)&h.m_numSegms);
        VIBufferSwap(8, (unsigned char *)&h.m_totalCompressed);
        VIBufferSwap(8, (unsigned char *)&h.m_totalSize);
        VIBufferSwap(8, (unsigned char *)&h.m_offset);
        VIBufferSwap(4, (unsigned char *)&h.m_maxCompressed);
    }
    if (h.m_id != 0x46534543) return -1;
    if (m_progress) m_progress(m_progressArg);
    n = h.m_numSegms;
    *segms = new VIFile::MemSegm[n];
    if (*segms == 0) return -1;
    char *buf = new char[h.m_maxCompressed];
    if (buf == 0) return -1;
    if (f.Seek(h.m_offset, VIFile::Seek_Set) < 0) return -1;
    for (int i = 0; i < n; i++)
    {
        VICSFSegm s;

        if (f.Read(&s, 8) < 0) return -1;
        if (VIEndian != 1)
        {
            VIBufferSwap(4, (unsigned char *)&s.m_compressed);
            VIBufferSwap(4, (unsigned char *)&s.m_size);
        }
        if (f.Read(buf, s.m_compressed) < 0) return -1;
        if (m_progress) m_progress(m_progressArg);
        (*segms)[i].m_size = s.m_size;
        (*segms)[i].m_addr = new char[(int)(*segms)[i].m_size];
        if ((*segms)[i].m_addr == 0) return -1;
        unsigned int len = s.m_size;
        if (uncompress((unsigned char *)(*segms)[i].m_addr, &len,
                       (const unsigned char *)buf, s.m_compressed)) return -1;
        if (len != s.m_size) return -1;
        if (m_progress) m_progress(m_progressArg);
    }
    if (buf) delete [] buf;
    return 0;
}
