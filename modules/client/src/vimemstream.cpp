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

// 0x00CB8610: zigzag -- the sign is the low bit.
void VIMemStream::ReadSigned(int &v)
{
    unsigned int u;
    ReadUnsigned(u);
    int neg = u & 1;
    u >>= 1;
    v = neg ? ~u : u;
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
