// The 40-byte player record (game.h, VIPlayerRecord) and its reader.  Both sit
// between VIFileStream.cpp and VIJournal.cpp in CLIENT's image, so the original
// file name falls in that range; this one is provisional.
#include "game.h"

VIPlayerRecord::VIPlayerRecord()
{
    m_id = -1;
    m_class = -1;
    m_race = -1;
    m_level = -1;
    memset(m_name, 0, sizeof(m_name));
}

VIMemStream &operator>>(VIMemStream &s, VIPlayerRecord &r)
{
    s >> r.m_id;
    s.ReadString(r.m_name, sizeof(r.m_name));
    s >> r.m_class;
    s >> r.m_race;
    s >> r.m_level;
    return s;
}
