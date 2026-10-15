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

// VIMap<int, VIGuildMember>: the members the image holds, from SUPPORT's own
// definitions (src/vimap_t.h).  A second Find, byte-identical and never called, sits
// at 0x00BF20A8: another TU's copy, which the linker kept and resolved nothing to.
#include "vimap_t.h"
template int VIMap<int, VIGuildMember>::End() const;  // 0x00BF0AA8
template int VIMap<int, VIGuildMember>::Find(const int &) const;  // 0x00BF0AB0
template int VIMap<int, VIGuildMember>::Begin() const;  // 0x00BF1E48
template int VIMap<int, VIGuildMember>::Next(int) const;  // 0x00BF1EA8
template int VIMap<int, VIGuildMember>::DestroyPool();  // 0x00BF1F60
template VIMap<int, VIGuildMember>::~VIMap();  // 0x00BF1FB8
template int VIMap<int, VIGuildMember>::Clear();  // 0x00BF2038
template int VIMap<int, VIGuildMember>::LinkFreeChain(int);  // 0x00BF2110
template int VIMap<int, VIGuildMember>::Allocate();  // 0x00BF21B8
template int VIMap<int, VIGuildMember>::AddNode(int, int);  // 0x00BF2380
template int VIMap<int, VIGuildMember>::LeftRotate(int);  // 0x00BF2478
template int VIMap<int, VIGuildMember>::RightRotate(int);  // 0x00BF2530
template int VIMap<int, VIGuildMember>::Insert(const int &, const VIGuildMember &);  // 0x00BF25E8
template int VIMap<int, VIGuildMember>::EraseFixup(int);  // 0x00BF2908
template int VIMap<int, VIGuildMember>::FreeNode(int);  // 0x00BF2CC0
template int VIMap<int, VIGuildMember>::Erase(int);  // 0x00BF2D00
