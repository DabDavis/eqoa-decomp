// The Class Mastery client, VIClient+0x4EC58 (game.h, VIClientPart4EC58; vtable slot
// 14), CLIENT 0x00BF6E98 .. 0x00BF84E0, then its template copies to 0x00BF9C58.
// Requests go out on 0x1401: a sub-op byte, the server's token and an argument
// (claude-notes/cm-wire-format.md); replies come back on 0x1402.  The file name is ours.
#include "game.h"

// A value written or read through a REFERENCE: a named local goes into the stack when
// the inline binds it, and its address is substituted straight into the call.
template <class T> static inline void WriteRef(VIMemStream &s, const T &v)
{
    s.Write(&v, sizeof(T), 1);
}

// 0x00BF6E98
VIClientPart4EC58::VIClientPart4EC58(VIClient *owner)
    : m_owner(owner)
{
    m_wnd = 0;
    memset(m_bonus, 0, sizeof(m_bonus));
    memset(m_stats, 0, sizeof(m_stats));
    m_pending = 0;
    m_speed = 8.833333f;
    m_p41C = 0;
}

// 0x00BF6F20
VIClientPart4EC58::~VIClientPart4EC58()
{
}

// 0x00BF6F90
int VIClientPart4EC58::Init()
{
    memset(m_bonus, 0, sizeof(m_bonus));
    memset(m_stats, 0, sizeof(m_stats));
    m_pending = 0;
    m_speed = 8.833333f;
    m_p41C = 0;
    return 0;
}

// 0x00BF6FF0
void VIClientPart4EC58::ReadBonuses(VIMemStream &s)
{
    memset(m_bonus, 0, sizeof(m_bonus));
    for (int i = 0; i < 7; i++)
        s.ReadSigned(m_bonus[i]);
}

// 0x00BF7060
void VIClientPart4EC58::ReadStats(VIMemStream &s)
{
    memset(m_stats, 0, sizeof(m_stats));
    for (int i = 0; i < 29; i++)
        s.ReadSigned(m_stats[i]);
}

// 0x00BF70D0: a count, then (id, flag) pairs for the skills listed.
void VIClientPart4EC58::ReadFlags(VIMemStream &s)
{
    int n;
    s.Read(&n, 4, 1);
    for (int i = 0; i < n; i++)
    {
        int id, flag;
        s.Read(id);
        s.Read(flag);
        int k = Find(id);
        if (k >= 0)
            m_skills.Value(k).m_x05 = flag;
    }
}

// 0x00BF71A0
int VIClientPart4EC58::ReadState(VIMemStream &s)
{
    s.Read(&m_p41C, 4, 1);
    s.ReadSigned(m_x0C);
    s.ReadSigned(m_x10);
    s.ReadSigned(m_x14);
    s.ReadSigned(m_x08);
    s.Read(&m_speed, 4, 1);
    ReadStats(s);
    ReadBonuses(s);
    return 0;
}

// 0x00BF7240
void VIClientPart4EC58::Recv1402(VIMemStream &s)
{
    unsigned char op;
    s.Read(&op, 1, 1);
    switch (op)
    {
    case 1:
        Recv01(s);
        break;
    case 2:
        Recv02(s);
        break;
    case 3:
        Recv03(s);
        break;
    case 4:
        Recv04(s);
        break;
    case 5:
        Recv05(s);
        break;
    case 6:
        Recv06(s);
        break;
    case 7:
        Recv07(s);
        break;
    case 8:
        Recv08(s);
        break;
    case 9:
        Recv09(s);
        break;
    }
}

// 0x00BF7358
void VIClientPart4EC58::Recv01(VIMemStream &s)
{
    s.Read(&m_p41C, 4, 1);
    s.Read(&m_x08, 4, 1);
    m_pending = 0;
}

// 0x00BF73B0
void VIClientPart4EC58::Recv02(VIMemStream &s)
{
    s.Read(&m_p41C, 4, 1);
    m_pending = 0;
}

// 0x00BF73E8: a rise in points is announced (word 1627 for several, 1628 for one).
void VIClientPart4EC58::Recv03(VIMemStream &s)
{
    int old = m_x0C;
    VIUI &ui = m_owner->m_ui;
    s.Read(&m_x0C, 4, 1);
    s.Read(&m_x10, 4, 1);
    if (old < m_x0C)
    {
        unsigned short buf[256];
        buf[0] = 0;
        int d = m_x0C - old;
        if (d > 1)
            VIString::snprintf16(buf, 512, (const unsigned short *)ui.m_words.GetString(1627, ui.m_language),
                                 m_x0C - old);
        else if (d == 1)
            VIString::snprintf16(buf, 512, (const unsigned short *)ui.m_words.GetString(1628, ui.m_language));
        m_owner->ShowNotice(buf, 7);
    }
}

// 0x00BF74E0
void VIClientPart4EC58::Recv04(VIMemStream &s)
{
    s.Read(&m_x14, 4, 1);
}

// 0x00BF7510
void VIClientPart4EC58::Recv05(VIMemStream &s)
{
    s.Read(&m_p41C, 4, 1);
    int id;
    s.Read(&id, 4, 1);
    s.Read(&m_x0C, 4, 1);
    s.Read(&m_x10, 4, 1);
    s.Read(&m_speed, 4, 1);
    ReadStats(s);
    ReadBonuses(s);
    ReadFlags(s);
    int i = Find(id);
    if (i >= 0)
        m_skills.Value(i).m_x04 = 1;
    m_owner->SetSpeed(m_speed);
    m_pending = 0;
}

// 0x00BF7600
void VIClientPart4EC58::Recv06(VIMemStream &s)
{
    s.Read(&m_p41C, 4, 1);
    int id;
    s.Read(&id, 4, 1);
    s.Read(&m_speed, 4, 1);
    ReadStats(s);
    ReadBonuses(s);
    ReadFlags(s);
    int i = Find(id);
    if (i >= 0)
        m_skills.Value(i).m_x04 = 0;
    m_owner->SetSpeed(m_speed);
    m_pending = 0;
}

// 0x00BF76C8
void VIClientPart4EC58::Recv07(VIMemStream &s)
{
    int id, flag;
    s.Read(&id, 4, 1);
    s.Read(&flag, 4, 1);
    int i = Find(id);
    if (i >= 0)
        m_skills.Value(i).m_x05 = flag;
}

// No address: INLINE, Recv08's whole body.  VIClientPart3834C::ReadList (0x00BFD248) is the
// same function out of line.  The result has to reach `return r` at the END (not
// `return 0`): cse keeps the longest-lived register of an equivalence class as its
// canonical one, so only then does the loop pretest compare the Init result.
inline int VIClientPart4EC58::ReadList(VIMemStream &s)
{
    int n;
    s.Read(&n, 4, 1);
    int r = m_skills.Init(0);
    if (r != 0)
        goto fail;
    for (int i = 0; i < n; i++)
    {
        VICMSkill k;
        if ((r = k.Read(s)) != 0)
            goto fail;
        m_skills.Add(k);
    }
    r = 0;
    goto done;
fail:
    m_skills.Init(0);
done:
    m_pending = 0;
    return r;
}

// 0x00BF7748: the whole list again; a short read empties it.
void VIClientPart4EC58::Recv08(VIMemStream &s)
{
    s.Read(&m_p41C, 4, 1);
    ReadList(s);
}

// 0x00BF7810: the opened skill, shown in the window; emptied if either step fails.
void VIClientPart4EC58::Recv09(VIMemStream &s)
{
    s.Read(&m_p41C, 4, 1);
    m_node.Clear();
    if (m_node.Read(s) >= 0)
    {
        if (m_wnd->ShowNode(&m_node) < 0)
            m_node.Clear();
    }
    else
        m_node.Clear();
    m_pending = 0;
}

// 0x00BF78A8
int VIClientPart4EC58::Find(int id)
{
    for (int i = m_skills.Begin(); i != m_skills.End(); i = m_skills.Next(i))
        if (m_skills.Value(i).m_id == id)
            return i;
    return -1;
}

// 0x00BF7938
int VIClientPart4EC58::IsMine(unsigned int id)
{
    return (id & 0xFF000000) == 0xA9000000;
}

// 0x00BF7950: empty; DoAction hands every part (id, arg).
void VIClientPart4EC58::Use(unsigned int id, int arg)
{
}

// 0x00BF7958
void VIClientPart4EC58::Select(int n)
{
    if (m_pending)
        return;
    int v = n;
    if (v < 0)
        v = 0;
    if (v > 100)
        v = 100;
    {
        VIMemStream s;
        s.WriteU16(0x1401);
        unsigned char sub = 1;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p41C) << VIMemStreamRaw(v), m_owner->GetConn(251));
    }
    m_pending = 1;
}

// 0x00BF7A90
void VIClientPart4EC58::Send02(int x)
{
    if (m_pending)
        return;
    {
        VIMemStream s;
        s.WriteU16(0x1401);
        unsigned char sub = 2;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p41C) << VIMemStreamRaw(x), m_owner->GetConn(251));
    }
    m_pending = 2;
}

// 0x00BF7BA8
void VIClientPart4EC58::Send03(int x)
{
    if (m_pending)
        return;
    {
        VIMemStream s;
        s.WriteU16(0x1401);
        unsigned char sub = 3;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p41C) << VIMemStreamRaw(x), m_owner->GetConn(251));
    }
    m_pending = 3;
}

// 0x00BF7CC0
void VIClientPart4EC58::Open()
{
    if (m_pending)
        return;
    Close();
    {
        VIMemStream s;
        s.WriteU16(0x1401);
        unsigned char sub = 4;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p41C), m_owner->GetConn(251));
    }
    m_pending = 4;
}

// 0x00BF7DC8
void VIClientPart4EC58::Browse(int id)
{
    if (m_pending)
        return;
    m_node.Clear();
    {
        VIMemStream s;
        s.WriteU16(0x1401);
        unsigned char sub = 5;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p41C) << VIMemStreamRaw(id), m_owner->GetConn(251));
    }
    m_pending = 5;
}

// 0x00BF7EE8
void VIClientPart4EC58::Close()
{
    m_skills.Clear();
    m_node.Clear();
}

// 0x00BF7F18
int VIClientPart4EC58::IsKnown(int id)
{
    int i = Find(id);
    if (i < 0)
        return 0;
    VICMSkill &k = m_skills.Value(i);
    bool on = k.m_x04;
    return k.m_x05 || on;
}

// 0x00BF7F80
int VIClientPart4EC58::GetBonus(int i)
{
    if (i < 7)
    {
        if (i >= 0)
            return m_bonus[i];
    }
    return 0;
}

// 0x00BF7FA8: a stat by its id, through a table of the 29 slots' ids (-1 none).
int VIClientPart4EC58::GetStat(int id)
{
    struct { int slot; int id; } map[29] =
    {
        { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 },
        { 7, -1 }, { 8, -1 }, { 9, -1 }, { 10, -1 }, { 11, -1 }, { 12, -1 }, { 13, -1 },
        { 14, 8 }, { 15, 10 }, { 16, 12 }, { 17, 13 }, { 18, 14 }, { 19, -1 }, { 20, -1 },
        { 21, -1 }, { 22, 24 }, { 23, 26 }, { 24, 25 }, { 25, 27 }, { 26, 22 }, { 27, 23 },
        { 28, -1 },
    };
    for (int i = 0; i < 29; i++)
        if (map[i].id == id)
            return m_stats[i];
    return 0;
}

// 0x00BF80A0
int VICMSkill::Read(VIMemStream &s)
{
    s.ReadSigned(m_x00);
    s.Read(&m_x04, 1, 1);
    s.Read(&m_x05, 1, 1);
    s.ReadSigned(m_id);
    s.ReadSigned(m_x0C);
    s.Read(&m_x10, 1, 1);
    s.ReadSigned(m_x14);
    s.ReadString(m_name, sizeof(m_name));
    return 0;
}

// 0x00BF8150
int VICMNode::Clear()
{
    m_x04 = 0;
    m_x05 = 0;
    m_x10 = 0;
    m_x11 = 0;
    m_x14 = 0;
    m_x18 = 0;
    m_x1C = 0;
    m_x20 = 0;
    m_x24 = 0;
    m_x28 = 0;
    m_x2C = 0;
    m_x30 = 0;
    m_x34 = 0;
    m_name[0] = 0;
    m_xB8 = 0;
    m_title[0] = 0;
    m_text[0] = 0;
    m_id = -1;
    m_x08 = -1;
    m_x0C = -1;
    m_reqs.Init(0);
    m_entries.Init(0);
    m_lines.Init(0);
    return 0;
}

// 0x00BF81F0
int VICMNode::Read(VIMemStream &s)
{
    int i, n, r;
    s.ReadSigned(m_id);
    s.Read(&m_x04, 1, 1);
    s.Read(&m_x05, 1, 1);
    s.ReadSigned(m_x08);
    s.ReadSigned(m_x0C);
    s.Read(&m_x10, 1, 1);
    s.Read(&m_x11, 1, 1);
    s.ReadSigned(m_x14);
    s.ReadSigned(m_x18);
    s.ReadSigned(m_x1C);
    s.ReadSigned(m_x20);
    s.ReadSigned(m_x24);
    s.ReadSigned(m_x28);
    s.ReadSigned(m_x2C);
    s.ReadSigned(m_x30);
    s.ReadSigned(m_x34);
    s.ReadString(m_name, sizeof(m_name));
    s.ReadSigned(m_xB8);
    s.ReadString(m_title, sizeof(m_title));
    s.ReadString(m_text, sizeof(m_text));
    s.ReadSigned(n);
    if ((r = m_reqs.Init(n)) != 0)
        goto done;
    for (i = 0; i < m_reqs.m_size; i++)
    {
        s.Read(&m_reqs[i].m_x00, 1, 1);
        s.ReadSigned(m_reqs[i].m_x04);
    }
    s.ReadSigned(n);
    if ((r = m_entries.Init(n)) != 0)
        goto done;
    for (i = 0; i < m_entries.m_size; i++)
    {
        s.ReadString(m_entries[i].m_text, sizeof(m_entries[i].m_text));
        s.Read(&m_entries[i].m_x80, 1, 1);
    }
    s.ReadSigned(n);
    if ((r = m_lines.Init(n)) != 0)
        goto done;
    for (i = 0; i < m_lines.m_size; i++)
        s.ReadString(m_lines[i].m_text, sizeof(m_lines[i].m_text));
    r = 0;
done:
    return r;
}


// VIPool<VICMSkill>: the skill list's members, from SUPPORT's definitions.  Its node is
// 160 bytes; the map called it VIPool<VIPtr_BF9718 *> until the node stride showed.
#include "vipool_t.h"
template int VIPool<VICMSkill>::End() const;  // 0x00BF9338
template int VIPool<VICMSkill>::Begin() const;  // 0x00BF9348
template int VIPool<VICMSkill>::Next(int) const;  // 0x00BF9378
template int VIPool<VICMSkill>::DestroyPool();  // 0x00BF93C0
template VIPool<VICMSkill>::~VIPool();  // 0x00BF9418
template int VIPool<VICMSkill>::Clear();  // 0x00BF9480
template int VIPool<VICMSkill>::LinkFreeChain(int);  // 0x00BF94E8
template int VIPool<VICMSkill>::Allocate();  // 0x00BF9590
template int VIPool<VICMSkill>::Init(int);  // 0x00BF9718
template int VIPool<VICMSkill>::Add(const VICMSkill &);  // 0x00BF9768
