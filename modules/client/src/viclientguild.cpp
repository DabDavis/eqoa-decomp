// The guild object, VIClient+0x2EC84 (game.h, VIClientGuild).  Its functions sit
// around 0x00BF0D38 (the 0x0468 handler); the file name is provisional.
#include "game.h"

VIMemStream &operator>>(VIMemStream &s, VIGuildMember &m)
{
    s >> (VIPlayerRecord &)m >> m.m_rank >> m.m_online;
    return s;
}

void VIClientGuild::UpdateMember(VIMemStream &s)
{
    VIGuildMember m;
    s >> m;
    const int i = m_members.Find(m.m_id);
    if (i == m_members.End())
        m_members.Insert(m.m_id, m);
    else
        m_members.Value(i) = m;
}
