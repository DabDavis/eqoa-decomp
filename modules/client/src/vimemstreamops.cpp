// CLIENT 0x00CB8AB8.., after VIMemStream.cpp and before VIMemStreamSet.cpp (which asserts
// from line 69 at 0x00CB9598): stream readers for small types.  The file's name is not
// known; this one is ours.
#include "game.h"

// 0x00CB8F60
VIMemStream &operator>>(VIMemStream &s, VIRGB &c)
{
    s.Read(&c.r, 1, 1);
    s.Read(&c.g, 1, 1);
    s.Read(&c.b, 1, 1);
    return s;
}

// 0x00CB8D98
VIMemStream &operator>>(VIMemStream &s, VIVect3 &v)
{
    s.Read(&v, 4, 3);
    return s;
}
