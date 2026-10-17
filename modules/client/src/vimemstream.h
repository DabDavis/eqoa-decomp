// vimemstream.h -- VIMemStream.cpp's classes.  One source file, linked into two
// modules: CLIENT (0x00CB8260) and STATION (0x002E2E00), byte-identical in both.
// game.h includes this; STATION's src/vimemstream.cpp and src/vimemstream.h are
// symlinks to CLIENT's.
#ifndef VIMEMSTREAM_H
#define VIMEMSTREAM_H

#include "vi.h"

// What VIMemStream.cpp asserts with (game.h has the same, first).
typedef unsigned short uint16;
extern "C" void free(void *);
extern "C" void _VIAssert(const char *expr, const char *file, int line);
#ifndef VIASSERT
#define VIASSERT(x)  if (!(x)) _VIAssert(#x, __FILE__, __LINE__);
#endif

// VIMemStream.cpp: the stream every network message is read through.  32 bytes,
// all words (phase 62 survey, from Write/Read/the dispatcher's inline ctor).
// The constructor and destructor are INLINE: the dispatcher builds one on its
// stack and frees an owned buffer on the way out.
struct VIMemStreamInt;
struct VIMemStreamByte
{
    explicit VIMemStreamByte(unsigned char v) : m_v(v) {}
    unsigned char m_v;
};
struct VIMemStreamRaw
{
    explicit VIMemStreamRaw(unsigned int v) : m_v(v) {}
    unsigned int m_v;
};

// A whole stream wrapped for operator<< / >>: the nested message func_00BF57B0 builds
// (an inner stream inside an outer one).  Passed by reference; name provisional.
class VIMemStream;
struct VIMemStreamBlob
{
    VIMemStreamBlob(VIMemStream &s) : m_stream(&s) {}
    VIMemStream *m_stream;
};

class VIMemStream
{
public:
    VIMemStream(const void *data, int size)
    {
        m_wbuf = 0;
        m_rbuf = (const char *)data;
        m_capacity = size;
        m_size = size;
        m_rpos = 0;
        m_error = 0;
        m_dryRun = 0;
        m_owned = 0;
    }
    // A stream that WRITES into the caller's buffer (Send006B, a char[32] on the stack):
    // the non-const overload of the reader above.
    VIMemStream(void *buf, int capacity)
    {
        m_wbuf = (char *)buf;
        m_rbuf = (const char *)buf;
        m_capacity = capacity;
        m_size = 0;
        m_rpos = 0;
        m_error = 0;
        m_dryRun = 0;
        m_owned = 0;
    }
    // An empty stream that owns (and grows) its own buffer: what every sender builds.
    VIMemStream()
    {
        m_wbuf = 0;
        m_rbuf = 0;
        m_capacity = 0;
        m_size = 0;
        m_rpos = 0;
        m_error = 0;
        m_dryRun = 0;
        m_owned = 1;
    }
    ~VIMemStream()
    {
        if (m_owned && m_wbuf)
            free(m_wbuf);
    }

    VIMemStream(const VIMemStream &s);                // 0x00CB8260: deep-copies an owned buffer
    void SetBuffer(void *buf, int capacity);          // 0x00CB82F8: write into a caller's buffer
    void SetData(const void *data, int size);         // 0x00CB8370: read from a caller's buffer
    void Grow(int capacity);                          // 0x00CB83E8: becomes owned, then realloc
    void Write(const void *src, int size, int count); // 0x00CB8460
    void Read(void *dst, int size, int count);        // 0x00CB8520
    void Skip(int n);                                 // 0x00CB85A8
    // Inline: the value is computed in a register and stored to the stack only for the
    // call (WriteUnsigned's byte, WriteString's length).
    void WriteByte(unsigned char b) { Write(&b, 1, 1); }
    void WriteU16(unsigned short v) { Write(&v, 2, 1); }
    void WriteU32(unsigned int v) { Write(&v, 4, 1); }
    void WriteU64(unsigned long v) { Write(&v, 8, 1); }
    void WriteF32(float v) { Write(&v, 4, 1); }     // VIEntity::WriteState's scale
    void WriteSigned(int v);                          // 0x00CB85E0: zigzag
    void ReadSigned(int &v);                          // 0x00CB8610: zigzag
    void WriteUnsigned(unsigned int v);               // 0x00CB8658: 7-bit varint
    void ReadUnsigned(unsigned int &v);               // 0x00CB86B8: 7-bit varint
    void ReadString(char *dst, unsigned int MaxBytes);            // 0x00CB8740 (line 399)
    void ReadString(unsigned short *dst, unsigned int MaxBytes);  // 0x00CB87E8 (line 422)
    void WriteString(const char *s);                  // 0x00CB88A0: u32 length, then the bytes
    void WriteString(const unsigned short *s);        // 0x00CB8918: u32 length in characters
    void SetDryRun(int on);                           // 0x00CB8990
    int  GetDryRun();                                 // 0x00CB8998
    VIMemStream &operator<<(const VIMemStreamBlob &b);  // 0x00CB89A0: a zigzag length, then the rest of b's stream
    VIMemStream &operator>>(const VIMemStreamBlob &b);  // 0x00CB8A38: the reverse, into b's stream

    // An inline reader (0x03D5).  An inlined helper's locals are known addressable
    // when they are laid out, so they take block temporaries: 0x03D5's byte sits at
    // sp+224, after the record, not with the other cases' bytes at sp+2112.
    bool ReadBool() { unsigned char b; Read(&b, 1, 1); return b != 0; }
    // Its writing twin (the server-select entry's writer, 0x00CBBAB8): a bool PARAMETER is
    // loaded as a whole word and truncated by the byte store, where WriteByte(flag)
    // narrows the load to an lbu.
    void WriteBool(bool v) { WriteByte(v); }
    // The same read as a chainable operator (the guild member's reader chains it).
    VIMemStream &operator>>(bool &v) { unsigned char b; Read(&b, 1, 1); v = b != 0; return *this; }
    // A byte, the same way (Msg0010: the address goes through the reference, so `addiu`
    // where a direct `&local` would be `ori`).
    VIMemStream &operator>>(unsigned char &v) { Read(&v, 1, 1); return *this; }

    // An inline reader that takes its target BY VALUE as an 8-byte object (see
    // VIMemStreamInt below).
    inline VIMemStream &operator>>(VIMemStreamInt v);
    inline VIMemStream &operator<<(VIMemStreamInt v);
    // A raw word (SendOpSigned2Int's last field): its own wrapper type, since a plain
    // operator<<(unsigned int) would take every int ahead of the zigzag wrapper.
    VIMemStream &operator<<(VIMemStreamRaw v) { Write(&v, 4, 1); return *this; }
    // The byte is copied to a local, so it lives in the inline's FRAME (a 16-byte temp
    // allocated in chain order) and not behind an ADDRESSOF at the end of the frame.
    VIMemStream &operator<<(VIMemStreamByte v) { unsigned char b = v.m_v; Write(&b, 1, 1); return *this; }

    // One value by reference.  Binding the reference keeps `&a[k].f` an ADDR_EXPR, which
    // expands as (this + k*size) + offset; `Read(&a[k].f, 4, 1)` folds the offset into
    // the array base instead (Channel44, 40 records of 160 bytes).
    template <class T> void Read(T &v) { Read(&v, sizeof(T), 1); }
    template <class T, int N> void Read(T (&v)[N]) { Read(v, sizeof(T), N); }

    // Out of line: Msg07E0 calls them (0x00BE3660..0x00BE3678, VIClient.cpp's tail).
    int  GetPos();                                    // 0x00BE3660: m_rpos
    int  Remaining();                                 // 0x00BE3668: m_size - m_rpos
    const char *GetBuffer();                          // 0x00BE3678: m_rbuf

    char       *m_wbuf;                               // +0x00: Write's buffer, freed if owned
    const char *m_rbuf;                               // +0x04: Read's source
    int         m_capacity;                           // +0x08
    int         m_size;                               // +0x0C: Read's limit
    int         m_rpos;                               // +0x10
    int         m_error;                              // +0x14: sticky overrun
    int         m_dryRun;                             // +0x18: advance without copying
    int         m_owned;                              // +0x1C

    int GetError() { return m_error; }                 // inline: VIJournal's asserts
};

// What an int field is read through in VIPlayerRecord's reader, the VIMemStreamArray.h element
// loop and the dispatcher's 0x03D6: the target's address stored TWICE into a
// block temporary just before ReadSigned, and never read back.  That is an
// inlined function's parameter living in memory: under EABI an 8-byte struct of
// two 4-byte words is BLKmode (its 4-byte alignment is below DImode's), so the
// inliner store_expr's it into the simulated argument area, built in place by
// the constructor, and cse forwards the read.  A 4-byte wrapper travels in a
// register and leaves no store.  Both words are the field's address, which
// suits a two-way description of a field (read into it, write from it); the
// names are provisional, only the shape is measured.
struct VIMemStreamInt
{
    int       *m_in;
    const int *m_out;
    VIMemStreamInt(int &v) : m_in(&v), m_out(&v) {}
    VIMemStreamInt(const int &v) : m_in((int *)&v), m_out(0) {}
};

inline VIMemStream &VIMemStream::operator>>(VIMemStreamInt v)
{
    ReadSigned(*v.m_in);
    return *this;
}

// The write side goes through the same wrapper: a non-const int builds {&v, &v}
// (SendOpSigned's parameter), a const one {&v, 0} (RequestBuddy's const record).
inline VIMemStream &VIMemStream::operator<<(VIMemStreamInt v)
{
    WriteSigned(*v.m_in);
    return *this;
}

#endif
