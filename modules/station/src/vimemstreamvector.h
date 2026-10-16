// vimemstreamvector.h -- VIMemStreamVector.h: a VIVector<T> written and read as
// a zigzag count and then its elements, for any T with its own stream
// operators.  The reader asserts from line 71 (STATION 0x002DF1A8, 0x002DF860,
// 0x002EACE8; CLIENT 0x00BE8680, 0x00BE8D38, 0x00BFCEF8), so the #line below
// puts it there.  VIMemStreamVector.cpp has the same two bodies for int.
#ifndef VIMEMSTREAMVECTOR_H
#define VIMEMSTREAMVECTOR_H

#include "vimemstream.h"
#include "vivector_t.h"

#line 59 "C:/code-frontiers-gm/game/VIMemStreamVector.h"
template <class T> VIMemStream &operator<<(VIMemStream &s, const VIVector<T> &v)
{
    s.WriteSigned(v.m_size);
    for (int i = 0; i < v.m_size; i++)
        s << v.m_data[i];
    return s;
}
template <class T> VIMemStream &operator>>(VIMemStream &s, VIVector<T> &v)
{
    int n;
    s.ReadSigned(n);
    int result = v.Init(n);
    VIASSERT(result == 0);
    for (int i = 0; i < n; i++)
        s >> v.m_data[v.Add()];
    return s;
}

#endif
