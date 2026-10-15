// The 40-byte player record (game.h, VIPlayerRecord) and its reader.  Both sit
// between VIFileStream.cpp and VIJournal.cpp in CLIENT's image, so the original
// file name falls in that range; this one is provisional.
#include "game.h"

VIPlayerRecord::VIPlayerRecord()
{
    m_id = -1;
    m_p1C = -1;
    m_p20 = -1;
    m_p24 = -1;
    memset(m_name, 0, sizeof(m_name));
}

VIMemStream &operator>>(VIMemStream &s, VIPlayerRecord &r)
{
    s >> r.m_id;
    s.ReadString(r.m_name, sizeof(r.m_name));
    s >> r.m_p1C;
    s >> r.m_p20;
    s >> r.m_p24;
    return s;
}
