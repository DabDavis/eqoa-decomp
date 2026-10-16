// vimemstreamx.h (name ours) -- the stream operators of VIMemStreamArray.cpp,
// the small-type TU after it (vimemstreamops.cpp, name ours), VIMemStreamSet.cpp
// and VIMemStreamVector.cpp: STATION 0x002E36E0 .. 0x002E4E18, CLIENT
// 0x00CB8B40 .. 0x00CBA278, byte-identical, the same four TUs in the same order.
// Each container's reader and writer here is for int elements, read and written
// zigzag through VIMemStreamInt; VIMemStreamArray.h and VIMemStreamVector.h hold
// the templates for every other element type (their asserts are on line 71).
#ifndef VIMEMSTREAMX_H
#define VIMEMSTREAMX_H

#include "vimemstream.h"

// A colour as the wire carries it: three bytes.  (CLIENT's game.h declares this
// today; when the file is shared, this is the one declaration.)
struct VIRGB
{
    unsigned char r, g, b;
};

// vimemstreamops.cpp
VIMemStream &operator<<(VIMemStream &s, const VIVect3 &v);
VIMemStream &operator>>(VIMemStream &s, VIVect3 &v);
VIMemStream &operator<<(VIMemStream &s, const VIBBox &b);
VIMemStream &operator>>(VIMemStream &s, VIBBox &b);
VIMemStream &operator<<(VIMemStream &s, const VISphere &sp);
VIMemStream &operator>>(VIMemStream &s, VISphere &sp);
VIMemStream &operator<<(VIMemStream &s, const VIRGB &c);
VIMemStream &operator>>(VIMemStream &s, VIRGB &c);
VIMemStream &operator<<(VIMemStream &s, const VIPool<int> &p);
VIMemStream &operator>>(VIMemStream &s, VIPool<int> &p);

// VIMemStreamArray.cpp
VIMemStream &operator>>(VIMemStream &s, VIArray<int> &a);

// VIMemStreamSet.cpp
VIMemStream &operator<<(VIMemStream &s, const VISet<int> &t);
VIMemStream &operator>>(VIMemStream &s, VISet<int> &t);

// VIMemStreamVector.cpp
VIMemStream &operator<<(VIMemStream &s, const VIVector<int> &v);
VIMemStream &operator>>(VIMemStream &s, VIVector<int> &v);

#endif
