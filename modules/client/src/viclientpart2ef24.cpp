#line 1 "C:/code-frontiers-gm/game/VIClientPart2EF24.cpp"
// VIClientPart2EF24 (name provisional) -- CLIENT 0x00BEE5A8 .. 0x00BEFECC: the guild
// commands at VIClient+0x2EF24, then this TU's template copies (0x00BEFED0 .. 0x00BF0A90,
// verified duplicates) and the guild map's End/Find (0x00BF0AA8/0x00BF0AB0, canonical, from
// viclientguild.cpp's instantiation).
#include "game.h"

// 0x00CD7A50
int gGuildCost = 5000;

// 0x00BEE5A8
VIClientPart2EF24::VIClientPart2EF24(VIClient *c)
{
    m_client = c;
    m_wnd = 0;
    m_inviter = 0xFFFFFFFF;
    m_name[0] = 0;
}

// 0x00BEE5C8
VIClientPart2EF24::~VIClientPart2EF24()
{
}

// 0x00BEE5F0
void VIClientPart2EF24::SetWnd(VIWndGame *w)
{
    m_wnd = w;
}

// 0x00BEE5F8
void VIClientPart2EF24::Reset()
{
    m_inviter = 0xFFFFFFFF;
    m_name[0] = 0;
}

// 0x00BEE610
char *VIClientPart2EF24::GetInviterName()
{
    return m_name;
}

// 0x00BEE618
unsigned short *VIClientPart2EF24::GetInviteGuild()
{
    return m_guild;
}

// 0x00BEE620
void VIClientPart2EF24::ShowInvite()
{
    m_wnd->OpenGuildMenu();
}

// 0x00BEE640
void VIClientPart2EF24::HideInvite()
{
    m_wnd->CloseGuildMenu();
    m_inviter = 0xFFFFFFFF;
}

// 0x00BEE678
int VIClientPart2EF24::CanInvite()
{
    VIClientGuild *g = m_client->GetGuild();
    VIPlayer *p = m_client->GetPlayer();
    if (!g->InGuild())
        return 3;
    if (g->Count() >= 128)
        return 11;
    if (GetRank(p->m_id) < 1)
        return 5;
    return 0;
}

// 0x00BEE720
int VIClientPart2EF24::CanCreate()
{
    VIClientGuild *g = m_client->GetGuild();
    VIPlayer *p = m_client->GetPlayer();
    if (g->InGuild())
        return 4;
    if (p->m_p34 < gGuildCost)
        return 6;
    if (!m_client->IsGroupFull() || !m_client->IsGroupLeader())
        return 7;
    return 0;
}

// 0x00BEE7D8
int VIClientPart2EF24::CanDisband()
{
    VIClientGuild *g = m_client->GetGuild();
    VIPlayer *p = m_client->GetPlayer();
    if (!g->InGuild())
        return 3;
    if (GetRank(p->m_id) != 3)
        return 5;
    return 0;
}

// 0x00BEE868
int VIClientPart2EF24::CanQuit()
{
    if (!m_client->GetGuild()->InGuild())
        return 3;
    return 0;
}

// 0x00BEE8A8
int VIClientPart2EF24::CanPromote(int id)
{
    VIClientGuild *g = m_client->GetGuild();
    VIPlayer *p = m_client->GetPlayer();
    if (!g->InGuild())
        return 3;
    int mine = GetRank(p->m_id);
    int theirs = GetRank(id);
    if (theirs + 1 < mine || (mine == 3 && theirs < 3))
        return 0;
    return 5;
}

// 0x00BEE980
int VIClientPart2EF24::CanDemote(int id)
{
    VIClientGuild *g = m_client->GetGuild();
    VIPlayer *p = m_client->GetPlayer();
    if (!g->InGuild())
        return 3;
    int mine = GetRank(p->m_id);
    int theirs = GetRank(id);
    if (theirs == 0)
        return 12;
    if (theirs > 0 && theirs < mine)
        return 0;
    return 5;
}

// 0x00BEEA48
int VIClientPart2EF24::CanKick(int id)
{
    VIClientGuild *g = m_client->GetGuild();
    VIPlayer *p = m_client->GetPlayer();
    if (!g->InGuild())
        return 3;
    int mine = GetRank(p->m_id);
    int theirs = GetRank(id);
    if (theirs < mine)
        return 0;
    return 5;
}

// 0x00BEEAF8
int VIClientPart2EF24::CanList()
{
    if (!m_client->GetGuild()->InGuild())
        return 3;
    return 0;
}

// 0x00BEEB38
int VIClientPart2EF24::CanSetMotd()
{
    VIClientGuild *g = m_client->GetGuild();
    VIPlayer *p = m_client->GetPlayer();
    if (!g->InGuild())
        return 3;
    if (GetRank(p->m_id) != 3)
        return 5;
    return 0;
}

// 0x00BEEBC8
int VIClientPart2EF24::CanChat()
{
    if (!m_client->GetGuild()->InGuild())
        return 3;
    return 0;
}

// 0x00BEEC08
void VIClientPart2EF24::Create(const unsigned short *name)
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    out.WriteByte(1);
    out.WriteString(name);
    VINetSend(out, GetConn());
}

// 0x00BEECD8
void VIClientPart2EF24::Disband()
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    out.WriteByte(2);
    VINetSend(out, GetConn());
}

// 0x00BEED98
void VIClientPart2EF24::Quit()
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    out.WriteByte(3);
    VINetSend(out, GetConn());
}

// 0x00BEEE58
void VIClientPart2EF24::Promote(int id)
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamByte(4) << id, GetConn());
}

// 0x00BEEF30
void VIClientPart2EF24::Demote(int id)
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamByte(5) << id, GetConn());
}

// 0x00BEF008
void VIClientPart2EF24::Kick(int id)
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamByte(6) << id, GetConn());
}

// 0x00BEF0E0
void VIClientPart2EF24::SetMotd(const unsigned short *s)
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    out.WriteByte(11);
    out.WriteString(s);
    VINetSend(out, GetConn());
}

// 0x00BEF1B0
void VIClientPart2EF24::Say(const unsigned short *s)
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    out.WriteByte(12);
    out.WriteString(s);
    VINetSend(out, GetConn());
}

// 0x00BEF280
void VIClientPart2EF24::Decline()
{
    m_wnd->CloseGuild();
    {
        VIMemStream out;
        unsigned short o = 1400;
        out.Write(&o, 2, 1);
        out.WriteByte(9);
        out.WriteU32(m_inviter);
        VINetSend(out, GetConn());
    }
    m_inviter = 0xFFFFFFFF;
}

// 0x00BEF370
void VIClientPart2EF24::Accept()
{
    m_wnd->CloseGuild();
    {
        VIMemStream out;
        unsigned short o = 1400;
        out.Write(&o, 2, 1);
        out.WriteByte(10), out.WriteU32(m_inviter), out.WriteByte(1);
        VINetSend(out, GetConn());
    }
    m_inviter = 0xFFFFFFFF;
}

// 0x00BEF478
void VIClientPart2EF24::Invite(const char *name)
{
    char me[24];
    m_client->GetPlayerName(me, 24);
    VIUI &ui = m_client->m_ui;
    if (!VIString::strcasecmp(name, me))
        m_wnd->AddText((const unsigned short *)ui.m_words.GetString(679, ui.m_language),
                       VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    else
    {
        VIClientGuild *g = m_client->GetGuild();
        VIMemStream out;
        out.WriteU16(1400), out.WriteByte(8);   // one statement: the opcode's temp outlives the byte's
        out.WriteString(name);
        out.WriteString(g->GetName());
        VINetSend(out, GetConn());
    }
}

// 0x00BEF600
void VIClientPart2EF24::SetChat(int on)
{
    VIMemStream out;
    unsigned short o = 1400;
    out.Write(&o, 2, 1);
    out.WriteByte(13), out.WriteByte(on);
    VINetSend(out, GetConn());
}

// 0x00BEF6D8
void VIClientPart2EF24::Recv057A(VIMemStream &s)
{
    unsigned char op;
    s.Read(&op, 1, 1);
    switch (op)
    {
    case 1:
        InviteReceived(s);
        break;
    case 2:
        InviteAccepted(s);
        break;
    case 3:
        InviteDeclined(s);
        break;
    }
}

// 0x00BEF788: the operation, then its result (0 = it worked).
void VIClientPart2EF24::Recv0579(VIMemStream &s)
{
    unsigned char op, result;
    VIUI &ui = m_client->m_ui;
    s.Read(&op, 1, 1);
    s >> result;
    switch (op)
    {
    case 1:
        if (!result)
            break;
        Msg(563);
        switch (result)
        {
        case 1:
            Msg(565);
            break;
        case 2:
            Msg(566);
            break;
        case 6:
        {
            unsigned short buf[256];
            VIString::snprintf16(buf, sizeof(buf),
                                 (const unsigned short *)ui.m_words.GetString(567, ui.m_language), gGuildCost);
            Text(buf);
            break;
        }
        case 7:
            Msg(568);
            break;
        }
        break;
    case 2:
        if (!result)
            break;
        Msg(564);
        switch (result)
        {
        case 5:
            Msg(546);
            break;
        case 8:
            Msg(570);
            break;
        }
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 11:
    case 12:
        if (!result)
            break;
        switch (result)
        {
        case 8:
            Msg(570);
            break;
        case 5:
            Msg(546);
            break;
        case 11:
            Msg(584);
            break;
        case 12:
            Msg(745);
            break;
        case 6:
            break;
        }
        break;
    }
}

// 0x00BEF9A0
void VIClientPart2EF24::InviteReceived(VIMemStream &s)
{
    VIUI &ui = m_client->m_ui;
    VIClientGuild *g = m_client->GetGuild();
    int x;
    s >> x;
    s.Read(&m_inviter, 4, 1);
    s.ReadString(m_name, 24);
    s.ReadString(m_guild, 128);
    if (!m_wnd->IsLootable())
    {
        unsigned short buf[128];
        VIString::snprintf16(buf, sizeof(buf), (const unsigned short *)ui.m_words.GetString(650, ui.m_language),
                             m_name);
        m_client->ShowText(buf);
        {
            VIMemStream out;
            out.WriteU16(1400), out.WriteByte(10), out.WriteU32(m_inviter), out.WriteByte(2);
            VINetSend(out, GetConn());
        }
        m_inviter = 0xFFFFFFFF;
    }
    else if (g->InGuild())
    {
        {
            VIMemStream out;
            out.WriteU16(1400), out.WriteByte(10), out.WriteU32(m_inviter), out.WriteByte(4);
            VINetSend(out, GetConn());
        }
        m_inviter = 0xFFFFFFFF;
    }
    else
        m_wnd->OpenGuild();
}

// 0x00BEFC48
void VIClientPart2EF24::InviteAccepted(VIMemStream &s)
{
    VIUI &ui = m_client->m_ui;
    int id;
    char name[24];
    unsigned short buf[256];
    s.ReadSigned(id);
    s.ReadString(name, 24);
    VIString::snprintf16(buf, sizeof(buf), (const unsigned short *)ui.m_words.GetString(557, ui.m_language),
                         name);
    m_wnd->AddText(buf);
}

// 0x00BEFCD8
void VIClientPart2EF24::InviteDeclined(VIMemStream &s)
{
    VIUI &ui = m_client->m_ui;
    char name[24];
    unsigned char why;
    s.Read(&why, 1, 1);
    s.ReadString(name, 24);
    int word;
    switch (why)
    {
    case 2:
        word = 559;
        break;
    case 3:
        word = 560;
        break;
    case 4:
        word = 576;
        break;
    case 5:
        word = 587;
        break;
    case 6:
        word = 589;
        break;
    case 7:
        word = 642;
        break;
    case 1:
        word = 558;
        break;
    default:
        word = 558;
        break;
    }
    unsigned short buf[256];
    VIString::snprintf16(buf, sizeof(buf), (const unsigned short *)ui.m_words.GetString(word, ui.m_language),
                         name);
    m_wnd->AddText(buf);
}

// 0x00BEFDC8
int VIClientPart2EF24::GetRank(int id)
{
    VIMap<int, VIGuildMember> *m = m_client->GetGuild()->GetMembers();
    int i = m->Find(id);
    if (i == m->End())
        return 0;
    return m->Value(i).m_rank;
}

// 0x00BEFE50
VIClientConn VIClientPart2EF24::GetConn()
{
    return m_client->GetConn(251);
}

// 0x00BEFE90
void VIClientPart2EF24::Msg(unsigned short word)
{
    m_client->ShowMsg(word);
}

// 0x00BEFEB0
void VIClientPart2EF24::Text(const unsigned short *s)
{
    m_client->ShowText(s);
}
