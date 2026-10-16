// vidatetime.cpp (name ours) -- STATION 0x002E1FF8 .. 0x002E2258, CLIENT
// 0x00CB6DA8 .. 0x00CB7008: a customer-service timestamp, formatted and
// streamed.  The times are the servers' own, so the zone is printed, not
// converted.  The name sorts where the link puts it: between VICSInterface.cpp
// and VIFlatFile.cpp in STATION, just before VIFileStream.cpp in CLIENT.
#include "vidatetime.h"

// 0x002E1FF8.  No caller in STATION.
void VITime::Format(unsigned short *buf, int size) const
{
    VIString::snprintf16(buf, size, "%02i:%02i PST", m_hour, m_minute);
}

// 0x002E2030
VIMemStream &operator>>(VIMemStream &s, VITime &t)
{
    s.Read(&t.m_hour, 1, 1);
    s.Read(&t.m_minute, 1, 1);
    s.Read(&t.m_second, 1, 1);
    return s;
}

// 0x002E2098
VIMemStream &operator<<(VIMemStream &s, const VITime &t)
{
    s.WriteByte(t.m_hour);
    s.WriteByte(t.m_minute);
    s.WriteByte(t.m_second);
    return s;
}

// 0x002E2118
void VIDate::Format(unsigned short *buf, int size) const
{
    VIString::snprintf16(buf, size, "%04i-%02i-%02i %02i:%02i PST", m_year, m_month, m_day,
                         m_time.m_hour, m_time.m_minute);
}

// 0x002E2158
VIMemStream &operator>>(VIMemStream &s, VIDate &d)
{
    s.Read(&d.m_year, 2, 1);
    s.Read(&d.m_month, 1, 1);
    s.Read(&d.m_day, 1, 1);
    s >> d.m_time;
    return s;
}

// 0x002E21C8
VIMemStream &operator<<(VIMemStream &s, const VIDate &d)
{
    s.WriteU16(d.m_year);
    s.WriteByte(d.m_month);
    s.WriteByte(d.m_day);
    s << d.m_time;
    return s;
}
