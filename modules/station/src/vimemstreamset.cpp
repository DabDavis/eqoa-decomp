#line 1 "C:/code-frontiers-gm/game/VIMemStreamSet.cpp"
// VIMemStreamSet.cpp -- STATION 0x002E4090 .. 0x002E4A98, CLIENT 0x00CB94F0 ..
// 0x00CB9EF8: an int set written and read zigzag.  The reader inserts into the
// set as it stands; it does not clear it first.  VISet<int>'s members follow, as
// this TU's own template copies.
#include "vimemstreamx.h"
#include "viset_t.h"
// (VISet<int>::Size, the size less the NIL sentinel clamped at 0, comes from
// viset_t.h; these lines are kept blank so line 69's assert stays put.)






































// 0x002E4090: the count, then each element in tree order.
VIMemStream &operator<<(VIMemStream &s, const VISet<int> &t)
{
    s.WriteSigned(t.Size());
    for (int i = t.Begin(); i != t.End(); i = t.Next(i))
    {
        const int &v = t.m_nodes[i].m_data;
        s << v;
    }
    return s;
}

// 0x002E4138: the assert is on line 69.
VIMemStream &operator>>(VIMemStream &s, VISet<int> &t)
{
    int n;
    s.ReadSigned(n);
    for (int i = 0; i < n; i++)
    {
        int v;
        s >> v;
        int result = t.Insert(v);
        VIASSERT(result != -1);
    }
    return s;
}
