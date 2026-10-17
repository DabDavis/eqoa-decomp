// vimemstreamarray.h -- VIMemStreamArray.h: a VIArray<T> written and read as a zigzag
// count and then its elements, for any T with its own stream operators (lane cl-char,
// 2026-10-16).  The reader asserts from line 71 (CLIENT 0x00CA9D18, 0x00CB3FF0 .. 0x00CB6020
// in VICharacter.cpp; 0x00BBD9E8, 0x00BBDA90, 0x00BEB738, 0x00BFC7E8, 0x00CBB590 in other TUs),
// so the #line below puts it there.  VIMemStreamArray.cpp has the same two bodies for int
// (vimemstreamarray.cpp).  The template mirrors vimemstreamvector.h.
#ifndef VIMEMSTREAMARRAY_H
#define VIMEMSTREAMARRAY_H

#include "vimemstream.h"
#include "viarray_t.h"

#line 59 "C:/code-frontiers-gm/game/VIMemStreamArray.h"
template <class T> VIMemStream &operator<<(VIMemStream &s, const VIArray<T> &a)
{
    s.WriteSigned(a.m_size);
    for (int i = 0; i < a.m_size; i++)
        s << a.m_data[i];
    return s;
}
template <class T> VIMemStream &operator>>(VIMemStream &s, VIArray<T> &a)
{
    int n;
    s.ReadSigned(n);
    int result = a.Init(n);
    VIASSERT(result == 0);
    for (int i = 0; i < a.m_size; i++)
        s >> a.m_data[i];
    return s;
}

#endif
