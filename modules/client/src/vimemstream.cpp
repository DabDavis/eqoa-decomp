#line 1 "C:/code-frontiers-gm/game/VIMemStream.cpp"
// VIMemStream -- CLIENT 0x00CB8260 .. 0x00CB8AB8: the byte stream every network
// message is read through (and the journal written through).  An overrun sets a
// sticky error and every later read is a no-op; a dry run advances the position
// without copying.
//
// Two functions assert ("MaxBytes >= 1" on line 399 and "MaxBytes >=
// sizeof(uint16)" on line 422, the two ReadString overloads), so those must sit
// on their original lines when written.
#include "game.h"

extern "C" void *memcpy(void *, const void *, int);

#define CHK(n, c)  typedef char n[(c) ? 1 : -1];
CHK(c_size, sizeof(VIMemStream) == 32)

extern "C" void *malloc(int);
extern "C" void *realloc(void *, int);
extern "C" int   strlen(const char *);

// 0x00CB8260
VIMemStream::VIMemStream(const VIMemStream &s)
{
    m_wbuf = s.m_wbuf;
    m_rbuf = s.m_rbuf;
    m_capacity = s.m_capacity;
    m_size = s.m_size;
    m_rpos = s.m_rpos;
    m_error = s.m_error;
    m_dryRun = s.m_dryRun;
    m_owned = s.m_owned;
    if (m_owned)
    {
        m_wbuf = (char *)malloc(s.m_capacity);
        memcpy(m_wbuf, s.m_wbuf, s.m_capacity);
        m_rbuf = m_wbuf;
    }
}

// 0x00CB82F8
void VIMemStream::SetBuffer(void *buf, int capacity)
{
    if (m_owned && m_wbuf)
    {
        free(m_wbuf);
        m_wbuf = 0;
    }
    m_owned = 0;
    m_wbuf = (char *)buf;
    m_rbuf = (const char *)buf;
    m_capacity = capacity;
    m_error = 0;
    m_dryRun = 0;
    m_rpos = 0;
    m_size = 0;
}

// 0x00CB8370
void VIMemStream::SetData(const void *data, int size)
{
    if (m_owned && m_wbuf)
    {
        free(m_wbuf);
        m_wbuf = 0;
    }
    m_wbuf = 0;
    m_capacity = size;
    m_size = size;
    m_rbuf = (const char *)data;
    m_error = 0;
    m_dryRun = 0;
    m_rpos = 0;
    m_owned = 0;
}

// 0x00CB83E8: a stream that does not own its buffer starts over as an empty owned one.
void VIMemStream::Grow(int capacity)
{
    if (!m_owned)
    {
        m_wbuf = 0;
        m_rbuf = 0;
        m_capacity = 0;
        m_error = 0;
        m_dryRun = 0;
        m_rpos = 0;
        m_size = 0;
        m_owned = 1;
    }
    char *p = (char *)realloc(m_wbuf, capacity);
    if (p)
    {
        m_wbuf = p;
        m_rbuf = p;
        m_capacity = capacity;
    }
}

// 0x00CB8460: an owned stream grows with 32 bytes to spare.
void VIMemStream::Write(const void *src, int size, int count)
{
    int n = size * count;
    if (m_owned && m_capacity - m_size < n)
        Grow(m_size + n + 32);
    if (!m_error && m_capacity - m_size >= n)
    {
        if (!m_dryRun)
            memcpy(m_wbuf + m_size, src, n);
        m_size += n;
    }
    else
        m_error = 1;
}

// 0x00CB8520
void VIMemStream::Read(void *dst, int size, int count)
{
    int n = size * count;
    if (!m_error && m_size - m_rpos >= n)
    {
        if (!m_dryRun)
            memcpy(dst, m_rbuf + m_rpos, n);
        m_rpos += n;
    }
    else
        m_error = 1;
}

// 0x00CB85A8
void VIMemStream::Skip(int n)
{
    int avail = m_size - m_rpos;
    if (!m_error && avail >= n)
        m_rpos += n;
    else
        m_error = 1;
}

// 0x00CB85E0
void VIMemStream::WriteSigned(int v)
{
    unsigned int neg = 0;
    if (v < 0)
    {
        neg = 1;
        v = ~v;
    }
    WriteUnsigned((v << 1) | neg);
}

// 0x00CB8610: zigzag -- the sign is the low bit.
void VIMemStream::ReadSigned(int &v)
{
    unsigned int u;
    ReadUnsigned(u);
    int neg = u & 1;
    u >>= 1;
    v = neg ? ~u : u;
}

// 0x00CB8658
void VIMemStream::WriteUnsigned(unsigned int v)
{
    do
    {
        unsigned char b = v & 0x7F;
        v >>= 7;
        if (v)
            b |= 0x80;
        WriteByte(b);
    } while (v);
}

// 0x00CB86B8: seven bits a byte, low first, until a byte without the top bit
// (or an error).
void VIMemStream::ReadUnsigned(unsigned int &v)
{
    unsigned int result = 0;
    int shift = 0;
    unsigned char b;
    do
    {
        Read(&b, 1, 1);
        result |= (b & 0x7F) << shift;
        shift += 7;
    } while ((b & 0x80) && !m_error);
    v = result;
}






































































































































































































// 0x00CB8740: a u32 length, then the bytes if they fit with their terminator;
// otherwise the payload is skipped and the result is empty.
void VIMemStream::ReadString(char *dst, unsigned int MaxBytes)
{
    unsigned int n;
    Read(&n, 4, 1);
    if (n + 1 <= MaxBytes)
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






// 0x00CB87E8: the same for a wide string; the length counts characters.
void VIMemStream::ReadString(unsigned short *dst, unsigned int MaxBytes)
{
    unsigned int n;
    Read(&n, 4, 1);
    if (n + 1 <= MaxBytes / 2)
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

// 0x00CB88A0
void VIMemStream::WriteString(const char *s)
{
    unsigned int n;
    if (!s)
        n = 0;
    else
        n = strlen(s);
    WriteU32(n);
    Write(s, 1, n);
}

// 0x00CB8918
void VIMemStream::WriteString(const unsigned short *s)
{
    unsigned int n;
    if (!s)
        n = 0;
    else
        n = VIString::strlen16(s);
    WriteU32(n);
    Write(s, 2, n);
}

// 0x00CB8990
void VIMemStream::SetDryRun(int on)
{
    m_dryRun = on;
}

// 0x00CB8998
int VIMemStream::GetDryRun()
{
    return m_dryRun;
}

// 0x00CB89A0: the accessors are CALLS here (this file's own out-of-line copies sit
// at 0x00CB8A98..0x00CB8AB0; the linker resolved every call to VIClient.cpp's).
VIMemStream &VIMemStream::operator<<(const VIMemStreamBlob &b)
{
    WriteSigned(b.m_stream->m_size - b.m_stream->m_rpos);
    VIMemStream *s = b.m_stream;
    Write(s->GetBuffer() + s->GetPos(), 1, s->Remaining());
    s->Skip(s->Remaining());
    return *this;
}

// 0x00CB8A38
VIMemStream &VIMemStream::operator>>(const VIMemStreamBlob &b)
{
    int n;
    ReadSigned(n);
    b.m_stream->Write(m_rbuf + m_rpos, 1, n);
    Skip(n);
    return *this;
}
