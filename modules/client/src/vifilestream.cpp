#line 1 "C:/code-frontiers-gm/game/VIFileStream.cpp"
// VIFileStream -- CLIENT 0x00CB7008 .. 0x00CB7510: VIMemStream's interface over a VIFile,
// with a sticky error.  Each element passes through a 16-byte buffer on its way (Swap:
// a plain copy on this little-endian machine).  The journal records into one.
//
// Three functions assert, on lines 57 (Open), 196 and 219 (the two ReadStrings: the same
// 23 lines apart as VIMemStream.cpp's pair), so those sit on their original lines.
#include "game.h"

extern "C" void *memcpy(void *, const void *, int);
extern "C" int   strlen(const char *);

// VIFile's open flags, as Open's assert text spells them.
#define VIFILE_RONLY   1
#define VIFILE_WONLY   2
#define VIFILE_CREATE  8
#define VIFILE_BINARY  32
// (blank lines below pad the asserts to their original lines)



































// 0x00CB7008
void VIFileStream::Open(const char *name, int read)
{
    int result = m_File.Open(name, read ? VIFILE_RONLY | VIFILE_BINARY : VIFILE_WONLY | VIFILE_BINARY | VIFILE_CREATE);
    VIASSERTF(result == 0, "File.Open(\"%s\", %s ) failed %i\n", name, read ? "VIFILE_RONLY | VIFILE_BINARY" : "VIFILE_WONLY | VIFILE_BINARY | VIFILE_CREATE", result);
    if (result)
        m_Error = 1;
}

// 0x00CB70C0: an element on its way to or from the file (byte order would be fixed here).
void Swap(void *dst, const void *src, int size)
{
    memcpy(dst, src, size);
}

// 0x00CB70E0
void VIFileStream::Write(const void *src, int size, int count)
{
    if (m_Error)
        return;
    const char *p = (const char *)src;
    for (int i = 0; i < count; i++)
    {
        char buf[16];
        Swap(buf, p, size);
        if (m_File.Write(buf, size))
        {
            m_Error = 1;
            break;
        }
        p += size;
    }
}

// 0x00CB7188
void VIFileStream::Read(void *dst, int size, int count)
{
    if (m_Error)
        return;
    char *p = (char *)dst;
    for (int i = 0; i < count; i++)
    {
        char buf[16];
        if (m_File.Read(buf, size))
        {
            m_Error = 1;
            break;
        }
        Swap(p, buf, size);
        p += size;
    }
}

// 0x00CB7230
VIFileStream::VIFileStream()
{
    m_Error = 0;
}

// 0x00CB7260
int VIFileStream::Eof()
{
    return m_File.Eof() != 0;
}

// 0x00CB7280
void VIFileStream::Flush()
{
    m_File.Flush();
}




























































// 0x00CB72A0: a u32 length, then the bytes if they fit with their terminator;
// otherwise the payload is skipped and the result is empty.
void VIFileStream::ReadString(char *dst, unsigned int MaxBytes)
{
    unsigned int n;
    Read(&n, 4, 1);
    if (n + 1 < MaxBytes)
    {
        Read(dst, 1, n);
        dst[n] = 0;
    }
    else
    {
        VIASSERT(MaxBytes >= 1);
        Skip(n);
        dst[0] = 0;
    }
}






// 0x00CB7348: the same for a wide string; the length counts characters.
void VIFileStream::ReadString(unsigned short *dst, unsigned int MaxBytes)
{
    unsigned int n;
    Read(&n, 4, 1);
    if (n + 1 < MaxBytes / 2)
    {
        Read(dst, 2, n);
        dst[n] = 0;
    }
    else
    {
        VIASSERT(MaxBytes >= sizeof(uint16));
        Skip(n * 2);
        dst[0] = 0;
    }
}

// 0x00CB7400
void VIFileStream::WriteString(const char *s)
{
    unsigned int n;
    if (!s)
        n = 0;
    else
        n = strlen(s);
    WriteU32(n);
    Write(s, 1, n);
}

// 0x00CB7478
void VIFileStream::WriteString(const unsigned short *s)
{
    unsigned int n;
    if (!s)
        n = 0;
    else
        n = VIString::strlen16(s);
    WriteU32(n);
    Write(s, 2, n);
}

// 0x00CB74F0
void VIFileStream::Skip(int n)
{
    m_File.Seek(n, VIFile::Seek_Cur);
}
