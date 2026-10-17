#line 1 "C:/code-frontiers-gm/game/VIClientGuild.cpp"
// The guild object, VIClient+0x2EC84 (game.h, VIClientGuild) -- CLIENT 0x00BF0B18 ..
// 0x00BF124C, then this TU's template copies (0x00BF1250 .. 0x00BF2ED0): the
// VIMap<int, VIGuildMember> members below are the canonical ones.  The file name is
// provisional.
#include "game.h"

// 0x00BF0B18
VIClientGuild::VIClientGuild(VIClient *c)
    : m_client(c), m_inGuild(0)
{
    m_showChat = 0;
    m_name[0] = 0;
    m_motd[0] = 0;
}

// 0x00BF0B58
VIClientGuild::~VIClientGuild()
{
}

// 0x00BF0BA8
VIMap<int, VIGuildMember> *VIClientGuild::GetMembers()
{
    return &m_members;
}

// 0x00BF0BB0
int VIClientGuild::InGuild()
{
    return m_inGuild;
}

// 0x00BF0BB8
unsigned short *VIClientGuild::GetName()
{
    return m_name;
}

// 0x00BF0BC0
unsigned short *VIClientGuild::GetMotd()
{
    return m_motd;
}

// 0x00BF0BC8
VIGuildMember *VIClientGuild::Member(int n)
{
    int i = m_members.Begin();
    for (int k = 0; k < n; k++)
        i = m_members.Next(i);
    return &m_members.Value(i);
}

// 0x00BF0C38
char *VIClientGuild::GetMemberName(int n)
{
    return Member(n)->m_name;
}

// 0x00BF0C58
int VIClientGuild::GetMemberId(int n)
{
    return Member(n)->m_id;
}

// 0x00BF0C78
int VIClientGuild::GetMemberRank(int n)
{
    return Member(n)->m_rank;
}

// 0x00BF0C98
int VIClientGuild::GetMemberRace(int n)
{
    return Member(n)->m_race;
}

// 0x00BF0CB8
int VIClientGuild::GetMemberClass(int n)
{
    return Member(n)->m_class;
}

// 0x00BF0CD8
int VIClientGuild::GetMemberLevel(int n)
{
    return Member(n)->m_level;
}

// 0x00BF0CF8
int VIClientGuild::GetMemberOnline(int n)
{
    return Member(n)->m_online;
}

// 0x00BF0D18
int VIClientGuild::Count()
{
    return m_members.Size();
}

// 0x00BF0D38: the case bodies are in this order in the image.
void VIClientGuild::Recv0468(VIMemStream &s)
{
    unsigned char op;
    s.Read(&op, 1, 1);
    switch (op)
    {
    case 1:
        ReadInfo(s);
        break;
    case 2:
        Reset();
        break;
    case 3:
        UpdateMember(s);
        break;
    case 4:
        RemoveMember(s);
        break;
    case 6:
        Msg(578);
        break;
    case 7:
    case 8:
        Msg(579);
        break;
    case 9:
        Msg(580);
        break;
    case 10:
        Msg(581);
        break;
    case 11:
        Msg(582);
        break;
    case 16:
        Msg(1041);
        break;
    case 12:
        Msg(583);
        break;
    case 13:
        Msg(647);
        m_showChat = 1;
        break;
    case 14:
        Msg(648);
        m_showChat = 0;
        break;
    case 5:
        Chat(s);
        break;
    case 15:
        Added(s);
        break;
    }
}

// 0x00BF0EC8
void VIClientGuild::ReadInfo(VIMemStream &s)
{
    m_inGuild = 1;
    s.ReadString(m_name, 128);
    s.ReadString(m_motd, 512);
}

// 0x00BF0F20
void VIClientGuild::Reset()
{
    m_inGuild = 0;
    m_name[0] = 0;
    m_motd[0] = 0;
    m_members.Clear();
}

// 0x00BF0F50
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

// 0x00BF1040
void VIClientGuild::RemoveMember(VIMemStream &s)
{
    int id;
    s.ReadSigned(id);
    m_members.Erase(m_members.Find(id));
}

// 0x00BF1088
void VIClientGuild::Chat(VIMemStream &s)
{
    unsigned short text[256];
    int from;
    s.Read(&from, 4, 1);
    s.ReadString(text, 512);
    m_client->AddText(text, VIColor32F(0.25f, 1.0f, 0.8f, 1.0f));
}

// 0x00BF1118
void VIClientGuild::Msg(unsigned short word)
{
    m_client->ShowMsg(word);
}

// 0x00BF1138
void VIClientGuild::Added(VIMemStream &s)
{
    unsigned short buf[512];
    char who[24], whom[24];
    s.ReadString(who, 24);
    s.ReadString(whom, 24);
    VIUI &ui = m_client->m_ui;
    VIString::snprintf16(buf, sizeof(buf), (const unsigned short *)ui.m_words.GetString(717, ui.m_language),
                         who, whom);
    m_client->ShowText(buf);
}

// 0x00BF11D8
VIMemStream &operator>>(VIMemStream &s, VIGuildMember &m)
{
    s >> (VIPlayerRecord &)m >> m.m_rank >> m.m_online;
    return s;
}

// VIMap<int, VIGuildMember>: the members the image holds, from SUPPORT's own
// definitions (src/vimap_t.h).  A second Find, byte-identical and never called, sits
// at 0x00BF20A8: another TU's copy, which the linker kept and resolved nothing to.
#include "vimap_t.h"
template int VIMap<int, VIGuildMember>::End() const;  // 0x00BF0AA8
template int VIMap<int, VIGuildMember>::Find(const int &) const;  // 0x00BF0AB0
template VIMap<int, VIGuildMember>::VIMap();  // 0x00BF1E28
template int VIMap<int, VIGuildMember>::Begin() const;  // 0x00BF1E48
template int VIMap<int, VIGuildMember>::Next(int) const;  // 0x00BF1EA8
template int VIMap<int, VIGuildMember>::DestroyPool();  // 0x00BF1F60
template VIMap<int, VIGuildMember>::~VIMap();  // 0x00BF1FB8
template int VIMap<int, VIGuildMember>::Size() const;  // 0x00BF2020
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
