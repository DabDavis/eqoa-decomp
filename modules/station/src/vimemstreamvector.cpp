#line 1 "C:/code-frontiers-gm/game/VIMemStreamVector.cpp"
// VIMemStreamVector.cpp -- STATION 0x002E4A98 .. 0x002E4E18, CLIENT 0x00CB9EF8 ..
// 0x00CBA278: an int vector written and read zigzag.  VIVector<int>'s members
// follow, as this TU's own template copies.
#include "vimemstreamx.h"
#include "vivector_t.h"










































// 0x002E4A98
VIMemStream &operator<<(VIMemStream &s, const VIVector<int> &v)
{
    s.WriteSigned(v.m_size);
    for (int i = 0; i < v.m_size; i++)
    {
        const int &e = v.m_data[i];
        s << e;
    }
    return s;
}

// 0x002E4B20: the assert is on line 66.
VIMemStream &operator>>(VIMemStream &s, VIVector<int> &v)
{
    int n;
    s.ReadSigned(n);
    int result = v.Init(n);
    VIASSERT(result == 0);
    for (int i = 0; i < n; i++)
    {
        int &e = v.m_data[v.Add()];
        s >> e;
    }
    return s;
}
