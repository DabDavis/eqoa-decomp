// vidatetime.h -- a time of day and a date as customer service sends them, and their
// stream operators.  One source file (vidatetime.cpp, name ours), linked into two
// modules: STATION 0x002E1FF8 .. 0x002E2258 and CLIENT 0x00CB6DA8 .. 0x00CB7008
// (in front of VIFileStream.cpp), byte-identical in both.  The type and method
// names are ours: nothing in either module names them.
#ifndef VIDATETIME_H
#define VIDATETIME_H

#include "vimemstream.h"

// Three bytes on the wire and in memory.
struct VITime
{
    unsigned char m_hour;     // +0
    unsigned char m_minute;   // +1
    unsigned char m_second;   // +2

    void Format(unsigned short *buf, int size) const;   // "%02i:%02i PST"
};

// Seven bytes on the wire; the time sits at +4.
struct VIDate
{
    short         m_year;     // +0
    unsigned char m_month;    // +2
    unsigned char m_day;      // +3
    VITime        m_time;     // +4

    void Format(unsigned short *buf, int size) const;   // "%04i-%02i-%02i %02i:%02i PST"
};

VIMemStream &operator>>(VIMemStream &s, VITime &t);
VIMemStream &operator<<(VIMemStream &s, const VITime &t);
VIMemStream &operator>>(VIMemStream &s, VIDate &d);
VIMemStream &operator<<(VIMemStream &s, const VIDate &d);

#endif
