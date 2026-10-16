// vimemstreamops.cpp (name ours) -- STATION 0x002E3908 .. 0x002E4090, CLIENT
// 0x00CB8D68 .. 0x00CB94F0: the stream operators for the engine's small value
// types and for an int pool, between VIMemStreamArray.cpp and VIMemStreamSet.cpp.
// No assert names the file.  VIPool<int>'s members follow, as this TU's copies.
#include "vimemstreamx.h"
#include "vipool_t.h"

// 0x002E3908
VIMemStream &operator<<(VIMemStream &s, const VIVect3 &v)
{
    s.Write(&v, 4, 3);
    return s;
}

// 0x002E3938
VIMemStream &operator>>(VIMemStream &s, VIVect3 &v)
{
    s.Read(&v, 4, 3);
    return s;
}

// 0x002E3968
VIMemStream &operator<<(VIMemStream &s, const VIBBox &b)
{
    s << b.min;
    s << b.max;
    return s;
}

// 0x002E39A8
VIMemStream &operator>>(VIMemStream &s, VIBBox &b)
{
    s >> b.min;
    s >> b.max;
    return s;
}

// 0x002E39E8
VIMemStream &operator<<(VIMemStream &s, const VISphere &sp)
{
    s << sp.c;
    float r = sp.r;
    s.Write(&r, 4, 1);
    return s;
}

// 0x002E3A38
VIMemStream &operator>>(VIMemStream &s, VISphere &sp)
{
    s >> sp.c;
    s.Read(&sp.r, 4, 1);
    return s;
}

// 0x002E3A80
VIMemStream &operator<<(VIMemStream &s, const VIRGB &c)
{
    s.WriteByte(c.r);
    s.WriteByte(c.g);
    s.WriteByte(c.b);
    return s;
}

// 0x002E3B00
VIMemStream &operator>>(VIMemStream &s, VIRGB &c)
{
    s.Read(&c.r, 1, 1);
    s.Read(&c.g, 1, 1);
    s.Read(&c.b, 1, 1);
    return s;
}

// 0x002E3B68: the count, then each element in pool order.
VIMemStream &operator<<(VIMemStream &s, const VIPool<int> &p)
{
    s.WriteSigned(p.Size());
    for (int i = p.Begin(); i != p.End(); i = p.Next(i))
    {
        const int &v = p.Value(i);
        s << v;
    }
    return s;
}

// 0x002E3C10: the pool is cleared and refilled.
VIMemStream &operator>>(VIMemStream &s, VIPool<int> &p)
{
    int n;
    s.ReadSigned(n);
    p.Clear();
    for (int i = 0; i < n; i++)
        s >> p.Value(p.Add());
    return s;
}
