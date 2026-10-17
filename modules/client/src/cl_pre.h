// cl_pre.h -- before VIClient.cpp: VIClientPart382E8, VIBankControl.cpp (the bank + auction control), VIItemList members, VIClientText.
// CLIENT 0x00BB4A80..0x00BBF0D8.
#ifndef CL_PRE_H
#define CL_PRE_H

extern "C" int atoi(const char *s);
extern "C" void qsort(void *base, unsigned int n, unsigned int size,
                      int (*cmp)(const void *, const void *));
#ifndef VIALERT
// SUPPORT's non-fatal assert (STATION's stcs.h spells it the same): the file and line go
// to two globals first, then the message.
extern "C" void _VIAlert(const char *fmt, ...);
#define VIALERT(fmt, args...) \
    { __VIAssertf_file = __FILE__; __VIAssertf_line = __LINE__; _VIAlert(fmt, ## args); }
#endif

// VIMemStreamArray.h's reader, for any T with its own operator>> (its assert is line 71).
#include "vimemstreamarray.h"

// A VIPool<T> read as a zigzag count and then its elements, Add() by Add() (VIBankControl's
// two pools, 0x00BBCD10 and 0x00BBD0A8; vimemstreamops.cpp has the same body for int).
template <class T> VIMemStream &operator>>(VIMemStream &s, VIPool<T> &p)
{
    int n;
    s.ReadSigned(n);
    p.Clear();
    for (int i = 0; i < n; i++)
        s >> p.Value(p.Add());
    return s;
}

// A byte written in a chain, taken by CONST REFERENCE (the temporary lives to the end of
// the full expression) and BLKmode (two bytes, byte-aligned), so each one takes its own
// 16-byte slot: VIClientPart382E8::SendSimulation's layout (0x00BB5E50).  The byte twin
// of STATION's VIMemStreamU16 (vicsclient.h).  Name ours.
struct VIMemStreamU8
{
    explicit VIMemStreamU8(unsigned char v) : m_v(v) {}
    unsigned char m_v;
    unsigned char m_unused;
};
inline VIMemStream &operator<<(VIMemStream &s, const VIMemStreamU8 &v)
{
    s.Write(&v.m_v, 1, 1);
    return s;
}

// An int written zigzag in a chain, through a CONST REFERENCE: the {&v, &v} temporary then
// lives to the end of the full expression, where vimemstream.h's by-value VIMemStreamInt's
// slot is freed once an out-of-line call later in the chain (the query's writer, the
// VIVector<int> writer) is expanded -- and GetConn's result takes it.  VIBankControl's
// paged requests need the full-expression lifetime (0x00BB8F10 ..).  Switching
// vimemstream.h's operator<<(VIMemStreamInt) to a const reference instead is neutral on
// all of CLIENT (measured, lanes of 2026-10-16); STATION shares that header, so this
// stand-in stays until the coordinator checks it there.  Name ours.
struct VIMemStreamIntC
{
    int       *m_in;
    const int *m_out;
    explicit VIMemStreamIntC(int &v) : m_in(&v), m_out(&v) {}
};
inline VIMemStream &operator<<(VIMemStream &s, const VIMemStreamIntC &v)
{
    s.WriteSigned(*v.m_in);
    return s;
}

extern "C" int sprintf(char *, const char *, ...);   // VIClientText::Expand

// A flag written as one byte: the bool is loaded whole (lw) and narrowed by a local copy
// in the inline (the auction query's writer, 0x00BB4A80).  STATION's VIMemStreamBool
// narrows through WriteByte, a nested inline whose temp moves the frame by 16; CLIENT's
// VIMemStreamByte narrows the load itself (lbu).  Name ours.
struct VIMemStreamFlag
{
    explicit VIMemStreamFlag(bool v) : m_v(v) {}
    bool m_v;
};
inline VIMemStream &operator<<(VIMemStream &s, VIMemStreamFlag v)
{
    unsigned char b = v.m_v;
    s.Write(&b, 1, 1);
    return s;
}

#endif
