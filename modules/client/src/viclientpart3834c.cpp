// The crafting (tradeskill) client, VIClient+0x3834C (game.h, VIClientPart3834C; vtable
// slot 12), CLIENT 0x00BFCFB0 .. 0x00C00290, then its template copies to 0x00C02E68:
// requests on 0x1101 (a sub-op byte, the server's token, arguments), replies on 0x1102.
// The file name is ours.
#include "game.h"

extern "C" void qsort(void *base, unsigned int n, unsigned int size,
                      int (*compare)(const void *, const void *));

// VIMemStreamArray.cpp's writer for int elements, 0x00CB8AB8 (vimemstreamx.h declares only
// the reader).
VIMemStream &operator<<(VIMemStream &s, const VIArray<int> &a);

// A value written through a REFERENCE (see viclientpart2efd0.cpp).
template <class T> static inline void WriteRef(VIMemStream &s, const T &v)
{
    s.Write(&v, sizeof(T), 1);
}

static int CompareRecipes(const void *a, const void *b);

// Declared before their first use: the two VIPool<VINetItem> members that destroy their
// element (defined after vipool_t.h, at the end).
template <> int VIPool<VINetItem>::DestroyPool();
template <> int VIPool<VINetItem>::Erase(int i);
template <> int VIPool<VINetItem>::Add(const VINetItem &v);

// 0x00BFCFB0
VIClientPart3834C::VIClientPart3834C(VIClient *owner)
    : m_owner(owner)
{
    m_wnd = 0;
    m_wndRecipe = 0;
    m_wndExperiment = 0;
    m_pending = 0;
    m_p78C0 = 0;
}

// 0x00BFD060
VIClientPart3834C::~VIClientPart3834C()
{
}

// 0x00BFD110
int VIClientPart3834C::Init(VIClientPart4F078 *factions)
{
    m_pending = 0;
    m_p78C0 = 0;
    m_recipes.Init(0);
    return 0;
}

// 0x00BFD140
int VIClientPart3834C::Clear()
{
    m_recipes.Clear();
    m_detail.Clear();
    return 0;
}

// 0x00BFD178: sub-op 14, the six slots.
int VIClientPart3834C::ReadSlots(VIMemStream &s)
{
    int i = 0;
    for (;;)
    {
        s.Read(&m_slots[i].m_x00, 4, 1);
        s.Read(&m_slots[i].m_x04, 4, 1);
        s.Read(&m_slots[i].m_x08, 4, 1);
        s.Read(&m_slots[i].m_x0C, 4, 1);
        if (++i >= 6)
            break;
    }
    return 0;
}


// 0x00BFD248: the recipe list; a short read empties it.
int VIClientPart3834C::ReadList(VIMemStream &s)
{
    int n;
    s.Read(&n, 4, 1);
    int r = m_recipes.Init(0);
    if (r != 0)
        goto fail;
    for (int i = 0; i < n; i++)
    {
        VIRec144_C017C0 k;
        if ((r = k.Read(s)) != 0)
            goto fail;
        if ((r = m_recipes.Add(k)) < 0)
            goto fail;
    }
    r = 0;
    goto done;
fail:
    m_recipes.Init(0);
done:
    m_pending = 0;
    return r;
}

// 0x00BFD318
int VIClientPart3834C::GetSlot04(unsigned int i)
{
    if (i >= 6)
        return 0;
    return m_slots[i].m_x04;
}


// 0x00BFD338
int VIClientPart3834C::GetSlot08(unsigned int i)
{
    if (i >= 6)
        return 0;
    return m_slots[i].m_x08;
}


// 0x00BFD358
VIRec144_C017C0 *VIClientPart3834C::GetRecipe(int id)
{
    VIRec144_C017C0 *r = 0;
    int i = Find(id);
    if (i >= 0)
        r = &m_recipes.Value(i);
    return r;
}


// 0x00BFD3A8: the recipes of one (a, b) group, in m_order.
int VIClientPart3834C::Select(int a, int b, VIArray<VIRec144_C017C0> &out)
{
    out.Init(0);
    int n = 0;
    int i;
    for (i = m_recipes.Begin(); i != m_recipes.End(); i = m_recipes.Next(i))
        if (m_recipes.Value(i).m_x0D == a && m_recipes.Value(i).m_x0E == b)
            n++;
    out.Init(n);
    int k = 0;
    for (i = m_recipes.Begin(); i != m_recipes.End() && k < n; i = m_recipes.Next(i))
        if (m_recipes.Value(i).m_x0D == a && m_recipes.Value(i).m_x0E == b)
        {
            out[k] = m_recipes.Value(i);
            k++;
        }
    if (out.m_size > 0)
        qsort(out.m_data, out.m_size, sizeof(VIRec144_C017C0), CompareRecipes);
    return 0;
}

// 0x00BFD5E8
static int CompareRecipes(const void *a, const void *b)
{
    VIRec144_C017C0 x = *(const VIRec144_C017C0 *)a;
    VIRec144_C017C0 y = *(const VIRec144_C017C0 *)b;
    if (x.m_order < y.m_order)
        return -1;
    return y.m_order < x.m_order;
}

// 0x00BFD778
int VIClientPart3834C::IsMine(unsigned int id)
{
    return (id & 0xFF000000) == 0xA7000000;
}

// 0x00BFD790
void VIClientPart3834C::Use(unsigned int id, int x)
{
    switch (id)
    {
    case 0xA7010000:
        m_pending = 0;
        Send6(x);
        m_wnd->OpenRecipe(x);
        break;
    case 0xA7020000:
        m_pending = 0;
        m_items.CopyFrom(m_owner->m_inventory);
        ClearCombine();
        m_wnd->OpenExperiment();
        break;
    }
}

// 0x00BFD828
void VIClientPart3834C::Send1(int a)
{
    if (m_pending)
        return;
    {
        VIMemStream s;
        s.WriteU16(0x1101);
        unsigned char sub = 1;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p78C0) << VIMemStreamRaw(a), m_owner->GetConn(251));
    }
    m_pending = 1;
}

// 0x00BFD948
void VIClientPart3834C::Send2(int a, int b)
{
    if (m_pending)
        return;
    {
        VIMemStream s;
        s.WriteU16(0x1101);
        unsigned char sub = 2;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p78C0) << VIMemStreamRaw(a) << VIMemStreamRaw(b),
                  m_owner->GetConn(251));
    }
    m_pending = 2;
}

// 0x00BFDA88
void VIClientPart3834C::Send3(int a, int b)
{
    if (m_pending)
        return;
    {
        VIMemStream s;
        s.WriteU16(0x1101);
        unsigned char sub = 3;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p78C0) << VIMemStreamRaw(a) << VIMemStreamRaw(b),
                  m_owner->GetConn(251));
    }
    m_pending = 3;
}

// 0x00BFDBC8: the ids of the items in the combine.
void VIClientPart3834C::Combine()
{
    if (m_pending)
        return;
    if (m_combine.Size() == 0)
        return;
    VIArray<int> ids;
    ids.Init(m_combine.Size());
    int i = m_combine.Begin();
    for (int k = 0; k < ids.m_size; k++)
    {
        ids[k] = m_combine.Value(i).m_id;
        i = m_combine.Next(i);
    }
    {
        VIMemStream s;
        s.WriteU16(0x1101);
        unsigned char sub = 4;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p78C0) << ids, m_owner->GetConn(251));
    }
    m_items.CopyFrom(m_owner->m_inventory);
    ClearCombine();
    m_pending = 4;
}

// 0x00BFDDC8
void VIClientPart3834C::Send5(int a)
{
    if (m_pending)
        return;
    VIMemStream s;
    s.WriteU16(0x1101);
    unsigned char sub = 5;
    WriteRef(s, sub);
    VINetSend(s << VIMemStreamRaw(m_p78C0) << VIMemStreamRaw(a), m_owner->GetConn(251));
}

// 0x00BFDED8
void VIClientPart3834C::Send6(int a)
{
    if (m_pending)
        return;
    Clear();
    {
        VIMemStream s;
        s.WriteU16(0x1101);
        unsigned char sub = 6;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p78C0) << VIMemStreamRaw(a), m_owner->GetConn(251));
    }
    m_pending = 5;
}

// 0x00BFDFF8
void VIClientPart3834C::Send7(int a)
{
    if (m_pending)
        return;
    m_detail.Clear();
    {
        VIMemStream s;
        s.WriteU16(0x1101);
        unsigned char sub = 7;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamRaw(m_p78C0) << VIMemStreamRaw(a), m_owner->GetConn(251));
    }
    m_pending = 6;
}

// 0x00BFE118
void VIClientPart3834C::Recv1102(VIMemStream &s)
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
    case 8:
        Recv08(s);
        break;
    case 7:
        Recv07(s);
        break;
    case 9:
        Recv09(s);
        break;
    case 10:
        Recv10(s);
        break;
    case 11:
        Recv11(s);
        break;
    case 12:
        Recv12(s);
        break;
    case 13:
        Recv13(s);
        break;
    case 14:
        ReadSlots(s);
        break;
    case 15:
        Recv15(s);
        break;
    case 16:
        Recv16(s);
        break;
    }
}

// 0x00BFE2A8
void VIClientPart3834C::Recv07(VIMemStream &s)
{
    m_pending = 0;
}

// 0x00BFE2B0: recipe `from` moves to recipe `to`'s position; the rest close up.
void VIClientPart3834C::Recv08(VIMemStream &s)
{
    s.Read(&m_p78C0, 4, 1);
    int from, to;
    s.Read(&from, 4, 1);
    s.Read(&to, 4, 1);
    int f = Find(from);
    int t = Find(to);
    if (f < 0)
    {
        m_pending = 0;
        return;
    }
    if (t >= 0)
    {
        int at = m_recipes.Value(t).m_order;
        int moved = -1;
        int i;
        for (i = m_recipes.Begin(); i != m_recipes.End(); i = m_recipes.Next(i))
        {
            VIRec144_C017C0 &r = m_recipes.Value(i);
            if (r.m_order == m_recipes.Value(f).m_order)
                moved = i;
            if (r.m_order > m_recipes.Value(f).m_order)
                r.m_order--;
        }
        for (i = m_recipes.Begin(); i != m_recipes.End(); i = m_recipes.Next(i))
        {
            VIRec144_C017C0 &r = m_recipes.Value(i);
            if (r.m_order >= at)
                r.m_order++;
        }
        if (moved != -1)
            m_recipes.Value(moved).m_order = at;
    }
    m_pending = 0;
}

// 0x00BFE450
void VIClientPart3834C::Recv01(VIMemStream &s)
{
    s.Read(&m_p78C0, 4, 1);
    m_owner->ShowMsg(1544);
}

// 0x00BFE490
void VIClientPart3834C::Recv02(VIMemStream &s)
{
    int id;
    s.Read(&id, 4, 1);
    s.Read(&m_p78C0, 4, 1);
    int i = Find(id);
    if (i >= 0)
        m_recipes.Erase(i);
    m_pending = 0;
}

// 0x00BFE508
void VIClientPart3834C::Recv03(VIMemStream &s)
{
    m_pending = 0;
}

// 0x00BFE510
void VIClientPart3834C::Recv04(VIMemStream &s)
{
    m_pending = 0;
    s.Read(&m_p78C0, 4, 1);
}

// 0x00BFE540
void VIClientPart3834C::Recv05(VIMemStream &s)
{
    VIUI &ui = m_owner->m_ui;
    m_pending = 0;
    s.Read(&m_p78C0, 4, 1);
    m_owner->AddText((const unsigned short *)ui.m_words.GetString(1567, ui.m_language),
                     VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
}

// 0x00BFE5C8: a skill-up report: the slot's new values, then a line of text.
void VIClientPart3834C::Recv09(VIMemStream &s)
{
    VIUI &ui = m_owner->m_ui;
    m_pending = 0;
    s.Read(&m_p78C0, 4, 1);
    int ok, slot, x04, x08, x0C, rise, fail, x2C;
    s.Read(&ok, 4, 1);
    s.Read(&slot, 4, 1);
    s.Read(&x04, 4, 1);
    s.Read(&x08, 4, 1);
    s.Read(&x0C, 4, 1);
    s.Read(&rise, 4, 1);
    s.Read(&fail, 4, 1);
    s.Read(&x2C, 4, 1);
    unsigned short name[64];
    s.ReadString(name, sizeof(name));
    unsigned short what[128], failed[128], skill[128], level[128], next[128];
    what[0] = 0;
    failed[0] = 0;
    skill[0] = 0;
    level[0] = 0;
    next[0] = 0;
    if (ok >= 0 && (unsigned int)slot < 6)
    {
        VICraftSlot &sl = m_slots[slot];
        int old04 = sl.m_x04;
        int old08 = sl.m_x08;
        sl.m_x04 = x04;
        sl.m_x08 = x08;
        sl.m_x0C = x0C;
        const unsigned short *f1, *f2, *f3;
        switch (slot)
        {
        case 1:
            f1 = (const unsigned short *)ui.m_words.GetString(1555, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1561, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1618, ui.m_language);
            break;
        case 0:
            f1 = (const unsigned short *)ui.m_words.GetString(1556, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1562, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1619, ui.m_language);
            break;
        case 2:
            f1 = (const unsigned short *)ui.m_words.GetString(1558, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1564, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1622, ui.m_language);
            break;
        case 3:
            f1 = (const unsigned short *)ui.m_words.GetString(1554, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1560, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1620, ui.m_language);
            break;
        case 4:
            f1 = (const unsigned short *)ui.m_words.GetString(1559, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1565, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1621, ui.m_language);
            break;
        case 5:
            f1 = (const unsigned short *)ui.m_words.GetString(1557, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1563, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1623, ui.m_language);
            break;
        }
        if (f1 && sl.m_x04 != old04)
            VIString::snprintf16(level, 256, f1, sl.m_x04);
        if (f2)
            VIString::snprintf16(skill, 256, f2, rise);
        if (f3 && sl.m_x08 != old08 && sl.m_x08)
            VIString::snprintf16(next, 256, f3);
    }
    VIString::snprintf16(what, 256, (const unsigned short *)ui.m_words.GetString(1588, ui.m_language), name);
    if (fail)
        VIString::snprintf16(failed, 256, (const unsigned short *)ui.m_words.GetString(1589, ui.m_language));
    unsigned short text[512];
    VIString::strcpy16(text, 1024, what);
    if (VIString::strlen16(failed))
    {
        VIString::strcat16(text, 1024, "  ");
        VIString::strcat16(text, 1024, failed);
    }
    if (VIString::strlen16(skill))
    {
        VIString::strcat16(text, 1024, "  ");
        VIString::strcat16(text, 1024, skill);
    }
    if (VIString::strlen16(next))
    {
        VIString::strcat16(text, 1024, "  ");
        VIString::strcat16(text, 1024, next);
    }
    if (VIString::strlen16(level))
    {
        VIString::strcat16(text, 1024, "  ");
        VIString::strcat16(text, 1024, level);
    }
    m_wndRecipe->Message(text, x2C);
}

// 0x00BFEA68
void VIClientPart3834C::Recv10(VIMemStream &s)
{
    VIUI &ui = m_owner->m_ui;
    m_pending = 0;
    s.Read(&m_p78C0, 4, 1);
    int fail;
    s.Read(&fail, 4, 1);
    unsigned short failed[128];
    failed[0] = 0;
    if (fail)
        VIString::snprintf16(failed, 256, (const unsigned short *)ui.m_words.GetString(1589, ui.m_language));
    unsigned short text[256];
    VIString::snprintf16(text, 512, "%S", ui.m_words.GetString(1569, ui.m_language));
    if (VIString::strlen16(failed))
    {
        VIString::strcat16(text, 512, "  ");
        VIString::strcat16(text, 512, failed);
    }
    m_wndRecipe->Message(text, -1);
}

// 0x00BFEB70
void VIClientPart3834C::Recv11(VIMemStream &s)
{
    VIUI &ui = m_owner->m_ui;
    m_pending = 0;
    s.Read(&m_p78C0, 4, 1);
    int fail;
    s.Read(&fail, 4, 1);
    unsigned short failed[128];
    failed[0] = 0;
    if (fail)
        VIString::snprintf16(failed, 256, (const unsigned short *)ui.m_words.GetString(1589, ui.m_language));
    unsigned short text[256];
    VIString::snprintf16(text, 512, "%S", ui.m_words.GetString(1570, ui.m_language));
    if (VIString::strlen16(failed))
    {
        VIString::strcat16(text, 512, "  ");
        VIString::strcat16(text, 512, failed);
    }
    m_wndRecipe->Message(text, -1);
}

// 0x00BFEC78: the experiment results.
void VIClientPart3834C::Recv12(VIMemStream &s)
{
    VIUI &ui = m_owner->m_ui;
    unsigned char op;
    s.Read(&op, 1, 1);
    s.Read(&m_p78C0, 4, 1);
    switch (op)
    {
    case 1:
        break;
    case 3:
        if (m_wnd->IsExperimentMode())
            m_wndExperiment->Notify((const unsigned short *)ui.m_words.GetString(1598, ui.m_language), -1);
        break;
    case 4:
        TookItems(s);
        break;
    case 5:
        TookItems2(s);
        break;
    case 2:
        if (m_wnd->IsExperimentMode())
            m_wndExperiment->Notify((const unsigned short *)ui.m_words.GetString(1599, ui.m_language), -1);
        break;
    case 6:
        ItemResult(s, 0);
        break;
    case 7:
        ItemResult(s, 1);
        break;
    }
    m_pending = 0;
}

// 0x00BFEDA8
void VIClientPart3834C::Recv13(VIMemStream &s)
{
    s.Read(&m_p78C0, 4, 1);
    int id, hp;
    s.Read(&id, 4, 1);
    s.Read(&hp, 4, 1);
    m_items.Op0060(m_items.Find(id), hp);
}

// 0x00BFEE30
void VIClientPart3834C::Recv15(VIMemStream &s)
{
    s.Read(&m_p78C0, 4, 1);
    if (m_wnd->IsRecipeMode())
        ReadList(s);
    else
        m_recipes.Init(0);
    m_pending = 0;
}

// 0x00BFEEA0
void VIClientPart3834C::Recv16(VIMemStream &s)
{
    s.Read(&m_p78C0, 4, 1);
    m_detail.Read(s);
    if (m_wnd->IsRecipeMode())
        m_wndRecipe->Inspect();
    m_pending = 0;
}

// 0x00BFEF08: what a combine made: the components it took, the item, the skill line.
void VIClientPart3834C::ItemResult(VIMemStream &s, int experiment)
{
    VIUI &ui = m_owner->m_ui;
    int ok, slot, x04, x08, x0C, rise, fail, x2C, n;
    s.Read(&ok, 4, 1);
    s.Read(&slot, 4, 1);
    s.Read(&x04, 4, 1);
    s.Read(&x08, 4, 1);
    s.Read(&x0C, 4, 1);
    s.Read(&rise, 4, 1);
    s.Read(&fail, 4, 1);
    s.Read(&x2C, 4, 1);
    s.Read(&n, 4, 1);
    for (int k = 0; k < n; k++)
    {
        int id, qty;
        s.Read(&id, 4, 1);
        s.Read(&qty, 4, 1);
        for (int j = 0; j < m_items.ItemCount; j++)
            if (m_items.Item(j)->m_id == id)
            {
                m_items.Took(j, qty);
                break;
            }
    }
    VINetItem item;
    ReadNetItem(s, item);
    m_items.AddItem(item, item.m_maxStack, 40);
    unsigned short what[128], failed[128], skill[128], level[128], next[128];
    what[0] = 0;
    failed[0] = 0;
    skill[0] = 0;
    level[0] = 0;
    next[0] = 0;
    if (ok >= 0 && (unsigned int)slot < 6)
    {
        VICraftSlot &sl = m_slots[slot];
        int old04 = sl.m_x04;
        int old08 = sl.m_x08;
        sl.m_x04 = x04;
        sl.m_x08 = x08;
        sl.m_x0C = x0C;
        const unsigned short *f1, *f2, *f3;
        switch (slot)
        {
        case 1:
            f1 = (const unsigned short *)ui.m_words.GetString(1555, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1561, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1618, ui.m_language);
            break;
        case 0:
            f1 = (const unsigned short *)ui.m_words.GetString(1556, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1562, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1619, ui.m_language);
            break;
        case 2:
            f1 = (const unsigned short *)ui.m_words.GetString(1558, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1564, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1622, ui.m_language);
            break;
        case 3:
            f1 = (const unsigned short *)ui.m_words.GetString(1554, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1560, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1620, ui.m_language);
            break;
        case 4:
            f1 = (const unsigned short *)ui.m_words.GetString(1559, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1565, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1621, ui.m_language);
            break;
        case 5:
            f1 = (const unsigned short *)ui.m_words.GetString(1557, ui.m_language);
            f2 = (const unsigned short *)ui.m_words.GetString(1563, ui.m_language);
            f3 = (const unsigned short *)ui.m_words.GetString(1623, ui.m_language);
            break;
        }
        if (f1 && sl.m_x04 != old04)
            VIString::snprintf16(level, 256, f1, sl.m_x04);
        if (f2)
            VIString::snprintf16(skill, 256, f2, rise);
        if (f3 && sl.m_x08 != old08 && sl.m_x08)
            VIString::snprintf16(next, 256, f3);
    }
    VIString::snprintf16(what, 256, (const unsigned short *)ui.m_words.GetString(1588, ui.m_language),
                         item.m_name);
    if (fail)
        VIString::snprintf16(failed, 256, (const unsigned short *)ui.m_words.GetString(1589, ui.m_language));
    unsigned short text[512];
    if (experiment)
    {
        VIString::snprintf16(text, 1024, (const unsigned short *)ui.m_words.GetString(1600, ui.m_language),
                             item.m_name, skill);
        if (m_wnd->IsExperimentMode())
            m_wndExperiment->Ask(text, 0xA7030004, x2C, m_owner->ItemIcon(&item));
    }
    else
    {
        VIString::strcpy16(text, 1024, what);
        if (VIString::strlen16(failed))
        {
            VIString::strcat16(text, 1024, "  ");
            VIString::strcat16(text, 1024, failed);
        }
        if (VIString::strlen16(skill))
        {
            VIString::strcat16(text, 1024, "  ");
            VIString::strcat16(text, 1024, skill);
        }
        if (VIString::strlen16(next))
        {
            VIString::strcat16(text, 1024, "  ");
            VIString::strcat16(text, 1024, next);
        }
        if (VIString::strlen16(level))
        {
            VIString::strcat16(text, 1024, "  ");
            VIString::strcat16(text, 1024, level);
        }
        if (m_wnd->IsExperimentMode())
            m_wndExperiment->Notify(text, m_owner->ItemIcon(&item));
    }
}

// 0x00BFF4F8: the components an experiment took.
void VIClientPart3834C::TookItems(VIMemStream &s)
{
    VIUI &ui = m_owner->m_ui;
    int fail, n;
    s.Read(&fail, 4, 1);
    s.Read(&n, 4, 1);
    for (int k = 0; k < n; k++)
    {
        int id, qty;
        s.Read(&id, 4, 1);
        s.Read(&qty, 4, 1);
        for (int j = 0; j < m_items.Count(); j++)
            if (m_items.Item(j)->m_id == id)
            {
                m_items.Took(j, qty);
                break;
            }
    }
    unsigned short text[256];
    if (fail)
        VIString::snprintf16(text, 512, "%S  %S", ui.m_words.GetString(1569, ui.m_language),
                             ui.m_words.GetString(1589, ui.m_language));
    else
        VIString::snprintf16(text, 512, "%S", ui.m_words.GetString(1569, ui.m_language));
    if (m_wnd->IsExperimentMode())
        m_wndExperiment->Notify(text, -1);
}

// 0x00BFF6A8
void VIClientPart3834C::TookItems2(VIMemStream &s)
{
    VIUI &ui = m_owner->m_ui;
    int fail, n;
    s.Read(&fail, 4, 1);
    s.Read(&n, 4, 1);
    for (int k = 0; k < n; k++)
    {
        int id, qty;
        s.Read(&id, 4, 1);
        s.Read(&qty, 4, 1);
        for (int j = 0; j < m_items.Count(); j++)
            if (m_items.Item(j)->m_id == id)
            {
                m_items.Took(j, qty);
                break;
            }
    }
    unsigned short text[256];
    if (fail)
        VIString::snprintf16(text, 512, "%S  %S", ui.m_words.GetString(1570, ui.m_language),
                             ui.m_words.GetString(1589, ui.m_language));
    else
        VIString::snprintf16(text, 512, "%S", ui.m_words.GetString(1570, ui.m_language));
    if (m_wnd->IsExperimentMode())
        m_wndExperiment->Notify(text, -1);
}

// 0x00BFF858: one of the inventory slot's items into the combine (at most four, and no
// more of one item than the slot holds).
void VIClientPart3834C::AddToCombine(int slot)
{
    VIUI &ui = m_owner->m_ui;
    if (slot < 0 || slot >= m_items.Count())
        return;
    VINetItem *it = m_items.Item(slot);
    if (!it->m_p54)
        return;
    int n = 0;
    for (int i = m_combine.Begin(); i != m_combine.End(); i = m_combine.Next(i))
        if (it->m_id == m_combine.Value(i).m_id)
            n++;
    if (n >= it->Quantity)
        m_owner->AddText((const unsigned short *)ui.m_words.GetString(1601, ui.m_language),
                         VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    else if (m_combine.Size() >= 4)
        m_owner->AddText((const unsigned short *)ui.m_words.GetString(1602, ui.m_language),
                         VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    else
    {
        VINetItem one;
        one = *it;
        one.Quantity = 1;
        m_combine.Add(one);
    }
}

// 0x00BFFA00
void VIClientPart3834C::RemoveFromCombine(int slot)
{
    if (slot < 0 || slot >= m_items.Count())
        return;
    VINetItem *it = m_items.Item(slot);
    if (!it->m_p54)
        return;
    for (int i = m_combine.Begin(); i != m_combine.End(); i = m_combine.Next(i))
        if (it->m_id == m_combine.Value(i).m_id)
        {
            m_combine.Erase(i);
            break;
        }
}

// 0x00BFFAB8
void VIClientPart3834C::ClearCombine()
{
    m_combine.Clear();
}

// 0x00BFFAD8
int VIClientPart3834C::Find(int id)
{
    for (int i = m_recipes.Begin(); i != m_recipes.End(); i = m_recipes.Next(i))
        if (m_recipes.Value(i).m_id == id)
            return i;
    return -1;
}

// 0x00BFFB68
VICraftDetail::VICraftDetail()
{
    m_desc = 0;
}

// 0x00BFFBA8
VICraftDetail::VICraftDetail(const VICraftDetail &d)
{
    m_desc = 0;
    *this = d;
}

// 0x00BFFC00
VICraftDetail::~VICraftDetail()
{
    if (m_desc)
    {
        delete[] m_desc;
        m_desc = 0;
    }
}

// 0x00BFFC78
int VICraftDetail::operator=(const VICraftDetail &d)
{
    m_x00 = d.m_x00;
    m_x04 = d.m_x04;
    m_x08 = d.m_x08;
    m_x0C = d.m_x0C;
    m_x0D = d.m_x0D;
    m_x0E = d.m_x0E;
    m_rarity = d.m_rarity;
    m_count = d.m_count;
    m_level = d.m_level;
    memcpy(m_name, d.m_name, sizeof(m_name));
    if (m_desc)
    {
        delete[] m_desc;
        m_desc = 0;
    }
    if (d.m_desc)
    {
        int len = VIString::strlen16(d.m_desc) + 1;
        m_desc = new unsigned short[len];
        VIString::strcpy16(m_desc, len * 2, d.m_desc);
    }
    m_stats.Init(d.m_stats);
    m_ingredients.Init(d.m_ingredients);
    m_tools.Init(d.m_tools);
    return 0;
}

// 0x00BFFE68
int VICraftDetail::Read(VIMemStream &s)
{
    int i, n, r;
    s.ReadSigned(m_x00);
    s.ReadSigned(m_x04);
    s.ReadSigned(m_x08);
    s.Read(&m_x0C, 1, 1);
    s.Read(&m_x0D, 1, 1);
    s.Read(&m_x0E, 1, 1);
    s.Read(&m_rarity, 1, 1);
    s.Read(&m_count, 2, 1);
    s.Read(&m_level, 1, 1);
    s.ReadString(m_name, sizeof(m_name));
    unsigned short buf[256];
    s.ReadString(buf, sizeof(buf));
    int len = VIString::strlen16(buf) + 1;
    if (m_desc)
        delete[] m_desc;
    m_desc = new unsigned short[len];
    VIString::strcpy16(m_desc, len * 2, buf);
    s.ReadSigned(n);
    if ((r = m_stats.Init(n)) != 0)
        goto done;
    for (i = 0; i < m_stats.m_size; i++)
    {
        s.ReadSigned(m_stats[i].m_id);
        s.ReadSigned(m_stats[i].m_value);
    }
    s.ReadSigned(n);
    if ((r = m_ingredients.Init(n)) != 0)
        goto done;
    for (i = 0; i < m_ingredients.m_size; i++)
    {
        s.ReadString(m_ingredients[i].m_name, sizeof(m_ingredients[i].m_name));
        s.Read(&m_ingredients[i].m_count, 2, 1);
    }
    s.ReadSigned(n);
    if ((r = m_tools.Init(n)) != 0)
        goto done;
    for (i = 0; i < m_tools.m_size; i++)
    {
        s.ReadString(m_tools[i].m_name, sizeof(m_tools[i].m_name));
        s.Read(&m_tools[i].m_have, 1, 1);
    }
    r = 0;
done:
    return r;
}

// 0x00C00158
int VICraftDetail::Clear()
{
    m_x00 = -1;
    m_x04 = -1;
    m_x08 = -1;
    m_x0C = 0;
    m_x0D = 0;
    m_x0E = 0;
    m_rarity = 0;
    m_count = 0;
    m_level = 0;
    m_name[0] = 0;
    if (m_desc)
    {
        delete[] m_desc;
        m_desc = 0;
    }
    m_stats.Clear();
    m_ingredients.Clear();
    m_tools.Clear();
    return 0;
}

// 0x00C001D8
int VIRec144_C017C0::Read(VIMemStream &s)
{
    s.ReadSigned(m_x00);
    s.ReadSigned(m_order);
    s.ReadSigned(m_id);
    s.Read(&m_level, 1, 1);
    s.Read(&m_x0D, 1, 1);
    s.Read(&m_x0E, 1, 1);
    s.Read(&m_difficulty, 1, 1);
    s.ReadString(m_name, sizeof(m_name));
    return 0;
}

// The two pools' members, from SUPPORT's definitions (src/vipool_t.h).  The map had them
// as VIPool<VIPtr_C00F58 *> and VIPool<VIPtr_C01508 *> until the node strides (772, 152)
// showed the element types.
#include "vipool_t.h"

// SUPPORT's VIPool (src/vipool_t.h) never destroys an element -- no SUPPORT pool holds a type
// with a destructor.  VINetItem has one (VIItem's VIArray), and the shipped DestroyPool and
// Erase run it, the way vimap_t.h's DestroyPool and FreeNode do for VIMap.
template <> int VIPool<VINetItem>::DestroyPool()  // 0x00C00FE0
{
    for (int i = Begin(); i != End(); )
    {
        int j = i;
        i = Next(i);
        m_nodes[j].m_data.~VINetItem();
    }
    return 0;
}

// ... and Add COPY-CONSTRUCTS the element where vipool_t.h assigns it: the call is VIItem's
// copy constructor (0x00BBC490), not its operator= (indistinguishable for a POD element).
template <> int VIPool<VINetItem>::Add(const VINetItem &v)  // 0x00C02518
{
    if (m_free == -1)
    {
        if (m_capacity)
            m_capacity = m_capacity * 2;
        else
            m_capacity = 1;
        if (Allocate() < 0)
            return -1;
    }

    Node *n = (Node *)(m_free * sizeof(Node) + (unsigned int)m_nodes);
    if (n)
        new (&n->m_data) VINetItem(v);

    m_nodes[m_free].m_prev = m_last;
    if (m_last != -1)
    {
        m_nodes[m_last].m_next = m_free;
    }
    else
    {
        m_first = m_free;
        m_last = m_free;
    }
    m_last = m_free;
    m_free = m_nodes[m_free].m_next;
    m_nodes[m_last].m_next = -1;
    m_size++;
    return m_last;
}

template <> int VIPool<VINetItem>::Erase(int i)  // 0x00C02660
{
    int n = m_nodes[i].m_next;

    if (n != -1)
        m_nodes[n].m_prev = m_nodes[i].m_prev;
    else
        m_last = m_nodes[i].m_prev;

    if (m_nodes[i].m_prev != -1)
        m_nodes[m_nodes[i].m_prev].m_next = m_nodes[i].m_next;
    else
        m_first = m_nodes[i].m_next;

    m_nodes[i].m_data.~VINetItem();
    m_nodes[i].m_next = m_free;
    m_nodes[i].m_prev = -2;
    m_free = i;
    m_size--;

    if (n == -1)
        return End();
    return n;
}

template VIPool<VINetItem>::VIPool();  // 0x00C00F30
template int VIPool<VINetItem>::End() const;  // 0x00C00F58
template int VIPool<VINetItem>::Begin() const;  // 0x00C00F68
template int VIPool<VINetItem>::Next(int) const;  // 0x00C00F98
template VIPool<VINetItem>::~VIPool();  // 0x00C01068
template int VIPool<VINetItem>::Size() const;  // 0x00C01950
template int VIPool<VINetItem>::LinkFreeChain(int);  // 0x00C02318
template int VIPool<VINetItem>::Allocate();  // 0x00C023C0
template int VIPool<VINetItem>::Clear();  // 0x00C02770
template VIPool<VIRec144_C017C0>::VIPool();  // 0x00C010D0
template int VIPool<VIRec144_C017C0>::End() const;  // 0x00C010F8
template int VIPool<VIRec144_C017C0>::Begin() const;  // 0x00C01108
template int VIPool<VIRec144_C017C0>::Next(int) const;  // 0x00C01138
template int VIPool<VIRec144_C017C0>::DestroyPool();  // 0x00C01180
template VIPool<VIRec144_C017C0>::~VIPool();  // 0x00C011D8
template int VIPool<VIRec144_C017C0>::Clear();  // 0x00C01240
template int VIPool<VIRec144_C017C0>::LinkFreeChain(int);  // 0x00C012A8
template int VIPool<VIRec144_C017C0>::Allocate();  // 0x00C01350
template int VIPool<VIRec144_C017C0>::Init(int);  // 0x00C01508
template int VIPool<VIRec144_C017C0>::Add(const VIRec144_C017C0 &);  // 0x00C01558
template int VIPool<VIRec144_C017C0>::Erase(int);  // 0x00C01B48
