#line 1 "C:/code-frontiers-gm/game/VIWndSpellBook.cpp"
// VIWndSpellBook -- CLIENT 0x00C93680 .. 0x00C96AC0 (the TU runs to its __tf at 0x00C97D30,
// then three inline members).  The ability book: a level tab bar, six rows of abilities with
// their description, a popup menu per ability, and the hot-bar slots (five, ten with the
// expansion) the selected one is assigned to.
#include "game.h"

extern "C" void qsort(void *, unsigned int, unsigned int, int (*)(const void *, const void *));

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// An array's size read through a reference: the original's VIArray accessor (its `this`
// is what keeps `addiu t,this,124; lw d,4(t)` for the row after the bounds test).
template <class T>
static inline int Size(const VIArray<T> &a)
{
    return a.m_size;
}

// A class's crest in the header: {class, UI texture}.
struct VIClassIcon
{
    int       m_class;
    VIUITexId m_tex;
};

static VIClassIcon sClassIcons[15] = {      // 0x00CEFFB8
    { 0, (VIUITexId)23 }, { 1, (VIUITexId)19 }, { 2, (VIUITexId)18 }, { 3, (VIUITexId)21 },
    { 4, (VIUITexId)16 }, { 5, (VIUITexId)11 }, { 6, (VIUITexId)20 }, { 7, (VIUITexId)13 },
    { 8, (VIUITexId)22 }, { 9, (VIUITexId)12 }, { 10, (VIUITexId)15 }, { 11, (VIUITexId)17 },
    { 12, (VIUITexId)14 }, { 13, (VIUITexId)24 }, { 14, (VIUITexId)251 },
};

// 0x00C93680.
VIWndSpellBook::VIWndSpellBook()
{
    m_x2C = -1;
    m_sel = 0;
    m_top = 0;
    m_tab = 0;
    m_x3C = 1;
    SetPos(VIVect2I(0, 0));
    m_popup = 0;
    m_mode = 0;
    m_time = 0;
    m_fadeDir = 1;
    m_fade = 1.0f;
    ClearText();
}

// 0x00C93738.
VIWndSpellBook::~VIWndSpellBook()
{
    Free();
}

// 0x00C937B0.
int VIWndSpellBook::Init()
{
    m_time = 0;
    m_tabCount = 0;
    m_fade = 1.0f;
    m_fadeDir = 1;
    m_spells.Init(0);
    m_slots.Init(0);
    m_popup = new VIWndMiniMenu;
    if (!m_popup)
        return -1;
    m_popup->SetVisible(false);
    m_popup->SetAppPtr(GetAppPtr());
    m_popup->SetMenu(&m_menu);
    AddChild(m_popup);
    m_menu.m_x00 = -1;
    m_menu.m_x04 = -1;
    m_menu.m_count = 4;
    m_menu.m_entries = m_entries;
    for (int i = 0; i < 4; i++)
    {
        m_entries[i].m_icon = -1;
        m_entries[i].m_icon2 = -1;
        m_entries[i].m_x08 = -1;
        m_entries[i].m_id = -1;
        m_entries[i].m_sub = 0;
        m_entries[i].m_number = -1;
    }
    m_entries[0].m_word = 1016;
    m_entries[0].m_msg = 0x08040000;
    m_entries[1].m_word = 200;
    m_entries[1].m_msg = 0x08080000;
    m_entries[2].m_word = 1800;
    m_entries[2].m_msg = 0x08090000;
    m_entries[3].m_word = 161;
    m_entries[3].m_msg = 0x08020000;
    return 0;
}

// 0x00C93910.
int VIWndSpellBook::Free()
{
    if (m_popup)
    {
        RemoveChild(m_popup);
        delete m_popup;
        m_popup = 0;
    }
    return 0;
}

// 0x00C93970.
void VIWndSpellBook::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int expansion = app->m_p50304;
    VIUI *ui = &app->m_ui;
    BuildSpells();
    BuildSlots();
    BuildText();
    unsigned short buf[64];
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIVect2I row(114, 44);
    VIVect2I at = m_pos;
    VIVect2I right = at + VIVect2I(449, 0);
    long now = app->m_p07568;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    VIUIFrameParam fp, studs;
    memset(&fp, 0, sizeof(fp));
    fp.m_bottom.m_id = (VIUITexId)49;
    fp.m_left.m_id = (VIUITexId)47;
    fp.m_br.m_id = (VIUITexId)50;
    fp.m_bl.m_id = (VIUITexId)48;
    fp.m_right.m_id = (VIUITexId)52;
    fp.m_top.m_id = (VIUITexId)45;
    fp.m_tl.m_id = (VIUITexId)44;
    fp.m_tr.m_id = (VIUITexId)46;
    ui->BlitFillFrame(VIVect2I(150, 50), VIVect2I(300, 177), fp, (VIUITexId)35,
                      (VIAlphaBlendMode)0, 1.0f, 1);
    ui->BlitFillFrame(VIVect2I(150, 227), VIVect2I(300, 88), fp, (VIUITexId)35,
                      (VIAlphaBlendMode)0, 1.0f, 1);
    memset(&fp, 0, sizeof(fp));
    memset(&studs, 0, sizeof(studs));
    fp.m_left.m_id = (VIUITexId)42;
    fp.m_bottom.m_id = (VIUITexId)40;
    fp.m_br.m_flags = 1;
    fp.m_right.m_flags = 1;
    fp.m_bl.m_id = (VIUITexId)39;
    fp.m_br.m_id = (VIUITexId)39;
    fp.m_tl.m_id = (VIUITexId)36;
    fp.m_tr.m_flags = 1;
    fp.m_right.m_id = (VIUITexId)42;
    fp.m_top.m_id = (VIUITexId)37;
    fp.m_tr.m_id = (VIUITexId)36;
    studs.m_bottom.m_id = (VIUITexId)41;
    studs.m_left.m_id = (VIUITexId)43;
    studs.m_right.m_flags = 1;
    studs.m_right.m_id = (VIUITexId)43;
    studs.m_top.m_id = (VIUITexId)38;
    ui->BlitRect((VIUITexId)51, VIVect2I(-1, 50), VIVect2I(140, 265), (VIAlphaBlendMode)0, 1.0f);
    ui->BlitRect((VIUITexId)51, VIVect2I(461, 50), VIVect2I(189, 265), (VIAlphaBlendMode)0, 1.0f);
    ui->BlitStuddedHorz(VIVect2I(-1, 39), (VIUITexId)37, (VIUITexId)38, 140, 1, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(-1, 315), (VIUITexId)40, (VIUITexId)41, 140, 1, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(461, 39), (VIUITexId)37, (VIUITexId)38, 190, 1, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(461, 315), (VIUITexId)40, (VIUITexId)41, 190, 1, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedFrame(VIVect2I(139, 39), VIVect2I(322, 294), fp, studs, 2, 3,
                         (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel - m_top >= 6)
        m_top = m_sel - 5;
    else if (m_sel < m_top)
        m_top = m_sel;
    float t;
    if (m_spells.m_size >= 7)
        t = (float)m_top / (float)(m_spells.m_size - 6);
    else
        t = 0.0f;
    m_window.DrawVScroll(VIVect2I(137, 47), 270, t, 0);
    if (expansion & 2)
        DrawSlots10();
    else
        DrawSlots();
    DrawTabs();
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(300, 135), 280, 9, 0);
    m_window.TextBox(m_text, 0, 0, 280, 9, (VIUICOLOR)0, 2);
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(300, 270), 280, 4, 0);
    if (m_mode == 0)
        m_window.TextBox(m_desc, 0, 0, 280, 4, (VIUICOLOR)0, 2);
    else if (m_mode == 2)
    {
        VIString::snprintf16(buf, sizeof(buf), "|x %S", Word(ui, 268));
        m_window.TextBox(buf, 0, 0, 280, 4, (VIUICOLOR)0, 2);
    }
    VIVect2I p = m_pos + VIVect2I(4, 50);
    int i, n;
    for (i = m_top, n = 0; i < m_spells.m_size && n < 6; i++, n++)
    {
        int lit = 0;
        if (m_x3C && i == m_sel && m_mode == 0)
            lit = 1;
        DrawIcon(&m_spells[i], p, lit, 1.0f);
        if (i == m_sel && m_mode == 1)
        {
            VIVect2I off(150, 0);
            DrawIcon(&m_held, p + off, 0, 0.5f);
        }
        p.y += row.y;
    }
    for (; i < 6; i++)
    {
        VIRec32_C97888 empty;
        memset(&empty, 0, sizeof(empty));
        empty.m_back = -1;
        empty.m_flags = 1;
        DrawIcon(&empty, p, 0, 1.0f);
        p.y += row.y;
    }
    p = right + VIVect2I(60, 262);
    int cls = app->GetClass();
    for (i = 0; i < 15; i++)
    {
        if (sClassIcons[i].m_class != cls)
            continue;
        const VIUITexEntry *tex = GetUITexture(sClassIcons[i].m_tex);
        ui->Blit((VIUITexId)130, p);
        p.x += 57 - tex->m_uv1.x / 2;
        if (cls == 14)
        {
            if (!(VIGetExpansionFlags() & 2))
                break;
            p.y += 1;
        }
        ui->Blit(sClassIcons[i].m_tex, p);
    }
    p = right + VIVect2I(25, 260);
    ui->Blit((VIUITexId)73, p);
    p += VIVect2I(10, 34);
    ui->m_font.SetTextColor(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
    ui->DrawCenteredStringEx((unsigned short)161, p.x, p.y, white, 0, 1);
    ui->m_font.SetTextColor(white);
    ui->End2D();
    if (m_x44)
    {
        if (m_fadeDir == -1)
        {
            m_fade -= dt * 2.0f;
            m_fade = m_fade < 0.0f ? 0.0f : m_fade;
            if (m_fade == 0.0f)
                m_fadeDir = 1;
        }
        else
        {
            m_fade += dt * 3.0f;
            m_fade = m_fade < 1.0f ? m_fade : 1.0f;
            if (m_fade == 1.0f)
                m_fadeDir = -1;
        }
    }
    else
        m_fade = 1.0f;
    if (m_flash.Pulse(138))
        Up();
    if (m_flash.Pulse(139))
        Down();
}

// 0x00C943E0.
void VIWndSpellBook::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        ResetFade();
        app->PlaySound(8);
        if (m_mode == 0)
            GetParent()->OnMessage(this, 260, 0, 0);
        else if (m_mode == 2)
            ShowList();
        else if (m_mode == 1)
        {
            m_x3C = 1;
            m_mode = 0;
        }
        break;
    case 17:
    case 136:
        if (m_mode == 0)
            Tab(-1);
        break;
    case 18:
    case 137:
        if (m_mode == 0)
            Tab(1);
        break;
    case 19:
    case 138:
        if (m_mode == 0 || m_mode == 1)
            m_flash.Press(138);
        else if (m_mode == 2)
            Slot(-1);
        break;
    case 20:
    case 139:
        if (m_mode == 0 || m_mode == 1)
            m_flash.Press(139);
        else if (m_mode == 2)
            Slot(1);
        break;
    case 13:
    case 134:
        if (m_mode == 0)
        {
            app->PlaySound(13);
            Pick();
        }
        else if (m_mode == 1)
        {
            app->PlaySound(13);
            Place();
        }
        else if (m_mode == 2)
            Assign();
        break;
    }
}

// 0x00C94660.
void VIWndSpellBook::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        m_flash.Release(138);
        break;
    case 20:
    case 139:
        m_flash.Release(139);
        break;
    }
}

// 0x00C946D8.
void VIWndSpellBook::Pick()
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    if (m_sel >= 0 && m_sel < Size(m_spells))
    {
        VIRec32_C97888 *r = &m_spells[m_sel];
        for (int i = 0; i < 4; i++)
            m_entries[i].m_id = r->m_id;
        VIVect2I at = m_pos + VIVect2I(150, (m_sel - m_top) * 44 + 45);
        m_popup->SetPos(at);
        m_popup->ResetScroll();
        m_popup->SetVisible(true);
        app->SetActiveWindow(m_popup);
    }
}

// 0x00C947D0.
void VIWndSpellBook::Place()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_x2C != m_sel && m_x2C >= 0 && m_x2C < Size(m_spells) && m_sel >= 0 &&
        m_sel < Size(m_spells))
        app->Send003A(m_spells[m_x2C].m_index, m_spells[m_sel].m_index);
    m_mode = 0;
    m_x3C = 1;
    ResetFade();
}

// 0x00C94860.
void VIWndSpellBook::Assign()
{
    if (m_sel >= 0 && m_sel < Size(m_spells))
    {
        VIRec32_C97888 *r = &m_spells[m_sel];
        GetParent()->OnMessage(this, 273, r->m_index, m_x34);
        r->m_valid = 1;
        ShowList();
    }
}

// 0x00C948E8.
void VIWndSpellBook::Down()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int last = m_spells.m_size - 1;
    if (m_sel >= last)
        m_sel = 0;
    else if (++m_sel == last)
        m_flash.Reset();
    app->PlaySound(11);
    ResetFade();
}

// 0x00C94960.
void VIWndSpellBook::Up()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel <= 0)
    {
        int n = 0;
        if (m_spells.m_size - 1 >= 0)
            n = m_spells.m_size - 1;
        m_sel = n;
    }
    else if (--m_sel == 0)
        m_flash.Reset();
    app->PlaySound(11);
    ResetFade();
}

// 0x00C949D8.
void VIWndSpellBook::Slot(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int count = app->m_p50304 & 2 ? 10 : 5;
    if (m_x34 + d >= count)
    {
        app->PlaySound(9);
        m_x34 = count - 1;
    }
    else if (m_x34 + d < 0)
    {
        app->PlaySound(9);
        m_x34 = 0;
    }
    else
    {
        app->PlaySound(11);
        m_x34 += d;
    }
    ResetFade();
    m_window.ResetTickPulse();
}

// 0x00C94A98.
void VIWndSpellBook::Tab(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_sel = 0;
    m_top = 0;
    m_tab += d;
    if (m_tab < 0)
        m_tab = m_tabCount - 1;
    else if (m_tab >= m_tabCount)
        m_tab = 0;
    app->PlaySound(11);
    ResetFade();
}

// 0x00C94B18.
void VIWndSpellBook::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    switch (msg)
    {
    case 261:
        switch (p1)
        {
        case 0x08080000:
            Hold();
            m_x3C = 0;
            m_popup->SetVisible(false);
            app->SetActiveWindow(this);
            m_mode = 1;
            break;
        case 0x08040000:
            m_popup->SetVisible(false);
            if (m_sel >= 0 && m_sel < Size(m_spells))
            {
                if (m_spells[m_sel].m_valid)
                    VIWnd::OnMessage(this, 272, 0, 0);
                else if (!app->GetAbilityP14(p2))
                {
                    unsigned short buf[64];
                    VIString::snprintf16(buf, sizeof(buf), Word(ui, 1629));
                    app->AddText(buf, VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
                    app->SetActiveWindow(this);
                }
                else
                    Equip();
            }
            break;
        case 0x08090000:
            m_popup->SetVisible(false);
            if (m_sel >= 0 && m_sel < Size(m_spells))
            {
                app->Send00FE();
                app->PlaySound(13);
            }
            app->SetActiveWindow(this);
            break;
        case 0x08020000:
            m_popup->SetVisible(false);
            app->SetActiveWindow(this);
            break;
        case 0x08030000:
            break;
        default:
            VIWnd::OnMessage(w, msg, p1, p2);
            break;
        }
        break;
    case 260:
        m_popup->SetVisible(false);
        app->SetActiveWindow(this);
        break;
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != this && (VIWnd *)p1 != m_popup)
        {
            m_tabCount = app->m_p50304 & 2 ? 6 : 5;
            m_popup->SetVisible(false);
            m_flash.Reset();
        }
        if ((VIWnd *)p2 == this)
        {
            m_fade = 1.0f;
            m_x44 = 1;
            m_fadeDir = 1;
            m_time = app->m_p07568;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 300, 1, 0);
            m_window.Open(true);
        }
        if ((VIWnd *)p1 == this)
        {
            m_x44 = 0;
            m_flash.Reset();
        }
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// The rows' order: by display index, -1 last.
static int CompareSpells(const void *a, const void *b);
// The slots' order: by slot.
static int CompareSlots(const void *a, const void *b);

// 0x00C94E68.
void VIWndSpellBook::BuildSpells()
{
    int i = 0;
    int count = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    m_spells.Clear();
    VIArray<int> list;
    app->GetAbilityList(list);
    for (i = 0; i < list.m_size; i++)
        if (Shown(list[i]))
            count++;
    m_spells.Init(count);
    int k;
    for (i = 0, k = 0; i < list.m_size && k < m_spells.m_size; i++)
    {
        if (Shown(list[i]))
        {
            VIRec32_C97888 r;
            memset(&r, 0, sizeof(r));
            r.m_id = list[i];
            r.m_valid = app->GetAbilityValid(list[i]);
            r.m_slot = app->GetAbilitySlot(list[i]);
            r.m_index = app->GetAbilityDisplayIndex(list[i]);
            r.m_flags = 17;
            r.m_back = app->GetAbilityIconBack(list[i]);
            r.m_front = app->GetAbilityIconFront(list[i]);
            m_spells[k] = r;
            k++;
        }
    }
    if (m_spells.m_size > 0)
        qsort(m_spells.Data(), m_spells.m_size, sizeof(VIRec32_C97888), CompareSpells);
}

// 0x00C95098.
static int CompareSpells(const void *a, const void *b)
{
    VIRec32_C97888 x = *(const VIRec32_C97888 *)a;
    VIRec32_C97888 y = *(const VIRec32_C97888 *)b;
    if (x.m_index == -1)
        return 1;
    if (y.m_index == -1)
        return -1;
    if (x.m_index < y.m_index)
        return -1;
    return y.m_index < x.m_index;
}

// 0x00C95158.
void VIWndSpellBook::BuildSlots()
{
    int i = 0;
    int count = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    m_slots.Clear();
    VIArray<int> list;
    app->GetAbilityList(list);
    for (i = 0; i < list.m_size; i++)
        if (Slotted(list[i]))
            count++;
    m_slots.Init(count);
    int k;
    for (i = 0, k = 0; i < list.m_size && k < m_slots.m_size; i++)
    {
        if (Slotted(list[i]))
        {
            VIRec12_C979B0 r;
            memset(&r, 0, sizeof(r));
            r.m_id = list[i];
            r.m_slot = app->GetAbilitySlot(list[i]);
            r.m_index = app->GetAbilityDisplayIndex(list[i]);
            m_slots[k] = r;
            k++;
        }
    }
    if (m_slots.m_size > 0)
        qsort(m_slots.Data(), m_slots.m_size, sizeof(VIRec12_C979B0), CompareSlots);
}

// 0x00C95318.
static int CompareSlots(const void *a, const void *b)
{
    VIRec12_C979B0 x = *(const VIRec12_C979B0 *)a;
    VIRec12_C979B0 y = *(const VIRec12_C979B0 *)b;
    if (x.m_slot < y.m_slot)
        return -1;
    return y.m_slot < x.m_slot;
}

// 0x00C95370.
void VIWndSpellBook::DrawIcon(VIRec32_C97888 *r, const VIVect2I &pos, int lit, float alpha)
{
    VIAlphaBlendMode mode = (VIAlphaBlendMode)3;
    if (alpha == 1.0f)
        mode = (VIAlphaBlendMode)0;
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIVect2I at;
    VIVect2I zero;
    VIVect2I row(114, 44);
    VIVect2I size(22, 22);
    VIVect2I front(62, 12);
    if (r->m_flags & 1)
    {
        if (r->m_back == -1)
            ui->BlitAbilityIcon(pos, 1, mode, alpha);
        else
        {
            ui->BlitAbilityIcon(pos, 0, mode, alpha);
            ui->Blit(r->m_back, pos + VIVect2I(49, 6), mode, alpha, 0);
        }
    }
    if (r->m_flags & 0x10)
        ui->Blit(r->m_front, pos + front, size, zero, mode, alpha, 0);
    if ((r->m_flags & 0x20) && r->m_x1C == 140)
    {
        VIVect2I p = pos + VIVect2I(2, 2);
        alpha = 0.3f;
        const VIUITexEntry *cap = GetUITexture((VIUITexId)139);
        const VIUITexEntry *mid = GetUITexture((VIUITexId)140);
        ui->BlitEx((VIUITexId)cap->m_x00, p, (VIAlphaBlendMode)3, alpha, 0);
        p.x += cap->m_uv1.x;
        ui->StretchBlitEx((VIUITexId)mid->m_x00, p, VIVect2I(110 - cap->m_uv1.x * 2, cap->m_uv1.y),
                          (VIAlphaBlendMode)3, alpha);
        p.x += 110 - cap->m_uv1.x * 2;
        ui->BlitEx((VIUITexId)cap->m_x00, p, (VIAlphaBlendMode)3, alpha, 1);
    }
    if (lit)
    {
        at = pos + VIVect2I(111, 14);
        ui->BlitEx((VIUITexId)86, at, (VIAlphaBlendMode)3, m_fade, 0);
    }
}

// 0x00C956E8.
void VIWndSpellBook::BuildText()
{
    VIClient *app = (VIClient *)GetAppPtr();
    ClearText();
    VIUI *ui = &app->m_ui;
    if (Size(m_spells) != 0 && m_sel >= 0 && m_sel < Size(m_spells))
    {
        VIRec32_C97888 *r = &m_spells[m_sel];
        unsigned short buf[512];
        app->GetAbilityName(r->m_id, ui->m_language, buf, sizeof(buf));
        VIString::strcat16(buf, sizeof(buf), "\n");
        AddScope(buf, sizeof(buf), app->GetAbilityScope(r->m_id));
        AddCastTime(buf, sizeof(buf), app->GetAbilityCastTime(r->m_id));
        VIString::strcat16(buf, sizeof(buf), "\n");
        AddCooldown(buf, sizeof(buf), app->GetAbilityCooldown(r->m_id));
        AddPowerCost(buf, sizeof(buf), app->GetAbilityPowerCost(r->m_id));
        VIString::strcat16(buf, sizeof(buf), "\n");
        AddEquipReq(buf, sizeof(buf), app->GetAbilityEquipReq(r->m_id));
        VIString::strcat16(buf, sizeof(buf), "\n");
        AddLevel(buf, sizeof(buf), app->GetAbilityLevel(r->m_id));
        AddRange(buf, sizeof(buf), app->GetAbilityRange(r->m_id));
        VIString::strcat16(buf, sizeof(buf), "\n");
        AddRecords(buf, sizeof(buf), r->m_id);
        VIString::strcpy16(m_text, sizeof(m_text), buf);
        app->GetAbilityDesc(r->m_id, ui->m_language, buf, sizeof(buf));
        VIString::strcpy16(m_desc, sizeof(m_desc), buf);
    }
}

// 0x00C95900.
void VIWndSpellBook::ClearText()
{
    m_desc[0] = 0;
    m_text[0] = 0;
}

// 0x00C95910.
void VIWndSpellBook::Hold()
{
    m_x2C = m_sel;
    if (m_sel >= 0 && m_sel < Size(m_spells))
    {
        VIRec32_C97888 *r = &m_spells[m_sel];
        m_held.m_id = r->m_id;
        m_held.m_flags = r->m_flags;
        m_held.m_back = r->m_back;
        m_held.m_front = r->m_front;
        m_held.m_x1C = r->m_x1C;
        m_held.m_valid = r->m_valid;
        m_held.m_slot = r->m_slot;
        m_held.m_index = r->m_index;
    }
}

// 0x00C95988.
void VIWndSpellBook::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_popup)
        m_popup->SetVisible(false);
    m_mode = 0;
    m_sel = 0;
    m_top = 0;
    m_tab = 0;
    SetVisible(true);
    m_x3C = 1;
    app->SetActiveWindow(this);
}

// 0x00C959E0.
void VIWndSpellBook::AddScope(unsigned short *buf, unsigned int size, int v)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIString::strcat16(buf, size, Word(ui, 465));
    VIString::strcat16(buf, size, ": ");
    unsigned short tmp[64];
    VIString::snprintf16(tmp, sizeof(tmp), "%d", v);
    VIWordId *w = FindTargetWord(v);
    if (w)
        VIString::strcpy16(tmp, sizeof(tmp), Word(ui, w->m_word));
    VIString::strcat16(buf, size, tmp);
    VIString::strcat16(buf, size, " ");
}

// 0x00C95AC8.
void VIWndSpellBook::AddCastTime(unsigned short *buf, unsigned int size, int v)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIString::strcat16(buf, size, Word(ui, 463));
    unsigned short tmp[64];
    VIString::snprintf16(tmp, sizeof(tmp), ": %d ", v);
    VIString::strcat16(buf, size, tmp);
}

// 0x00C95B50.
void VIWndSpellBook::AddCooldown(unsigned short *buf, unsigned int size, int v)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIString::strcat16(buf, size, Word(ui, 464));
    unsigned short tmp[64];
    VIString::snprintf16(tmp, sizeof(tmp), ": %d ", v);
    VIString::strcat16(buf, size, tmp);
}

// 0x00C95BD8.
void VIWndSpellBook::AddPowerCost(unsigned short *buf, unsigned int size, int v)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIString::strcat16(buf, size, Word(ui, 466));
    unsigned short tmp[64];
    VIString::snprintf16(tmp, sizeof(tmp), ": %d ", v);
    VIString::strcat16(buf, size, tmp);
}

// 0x00C95C60.
void VIWndSpellBook::AddRange(unsigned short *buf, unsigned int size, float v)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIString::strcat16(buf, size, Word(ui, 457));
    unsigned short tmp[64];
    VIString::snprintf16(tmp, sizeof(tmp), ": %.1f", v);
    VIString::strcat16(buf, size, tmp);
}

// 0x00C95CF8.
void VIWndSpellBook::AddEquipReq(unsigned short *buf, unsigned int size, int mask)
{
    int all = 255;
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIString::strcat16(buf, size, Word(ui, 467));
    VIString::strcat16(buf, size, ": ");
    if (mask == 0)
    {
        VIString::strcat16(buf, size, Word(ui, 302));
        VIString::strcat16(buf, size, " ");
    }
    else if ((mask & 0xFF) == all)
    {
        VIString::strcat16(buf, size, Word(ui, 471));
        VIString::strcat16(buf, size, " ");
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            const VIWeaponTypeWord *w = FindWeaponTypeWord(i);
            if ((mask >> i) & 1)
            {
                VIString::strcat16(buf, size, Word(ui, w->m_word));
                VIString::strcat16(buf, size, " ");
            }
        }
    }
}

// 0x00C95E38.
void VIWndSpellBook::AddRecords(unsigned short *buf, unsigned int size, int id)
{
    VIArray<int> recs;
    VIClient *app = (VIClient *)GetAppPtr();
    app->GetAbilityRecords(id, recs);
    if (recs.m_size > 0)
    {
        VIUI *ui = &app->m_ui;
        VIString::strcat16(buf, size, Word(ui, 468));
        VIString::strcat16(buf, size, ": ");
    }
    unsigned short name[64];
    for (int k = recs.Begin(); k != recs.End(); k = recs.Next(k))
    {
        memset(name, 0, sizeof(name));
        app->GetAbilityRecordName(id, k, name, sizeof(name));
        VIString::strcat16(buf, size, name);
        if (recs.Next(k) != recs.End())
            VIString::strcat16(buf, size, ", ");
    }
}

// 0x00C95FC8.
void VIWndSpellBook::AddLevel(unsigned short *buf, unsigned int size, int v)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIString::strcat16(buf, size, Word(ui, 503));
    unsigned short tmp[64];
    VIString::snprintf16(tmp, sizeof(tmp), ": %d ", v);
    VIString::strcat16(buf, size, tmp);
}

// 0x00C96050.
void VIWndSpellBook::Equip()
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    m_x3C = 0;
    m_x34 = 0;
    m_mode = 2;
    app->SetActiveWindow(this);
}

// 0x00C960A8.
void VIWndSpellBook::ShowList()
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    m_x3C = 1;
    m_x34 = 0;
    m_mode = 0;
}

// 0x00C960E8.
void VIWndSpellBook::ResetFade()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_fadeDir = 1;
    m_fade = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C96128.
int VIWndSpellBook::Shown(int id)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (app->GetAbilityDisplayIndex(id) < 252 && app->GetAbilityP14(id))
    {
        int lo = m_tab * 10 + 1;
        int hi = m_tab * 10 + 10;
        int level = app->GetAbilityLevel(id);
        if (level >= lo && level <= hi)
            return 1;
    }
    return 0;
}

// 0x00C961C8.
int VIWndSpellBook::Slotted(int id)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (app->GetAbilityValid(id) && app->GetAbilityDisplayIndex(id) < 252)
        return 1;
    return 0;
}

// 0x00C96228.
void VIWndSpellBook::DrawTabs()
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    int i;
    float t = 0.0f;
    VIVect2I p(0, 5);
    ui->Blit((VIUITexId)62, p);
    p.x += 37;
    for (i = 0; i < m_tabCount; i++)
    {
        ui->BlitEx((VIUITexId)63, p, (VIAlphaBlendMode)0, 1.0f, 0);
        ui->StretchBlitEx((VIUITexId)61, p, VIVect2I(73, 34), (VIAlphaBlendMode)3, t);
        p.x += 73;
        t += 0.35f / (float)(m_tabCount - 1);
    }
    p.x -= 12;
    ui->BlitEx((VIUITexId)64, p, (VIAlphaBlendMode)0, 1.0f, 0);
    int tab = 1729;
    VIArray<VIRec2_C97C68_A2> names;
    names.Init(m_tabCount);
    for (i = 0; i < names.m_size; i++, tab++)
        names[i].m_word = tab;
    VIVect2I q = m_pos;
    tab = m_tab;
    for (i = 0; i < names.m_size && tab < names.m_size; i++)
    {
        ui->DrawCenteredStringEx(names[tab].m_word, q.x + 45, q.y + 27,
                                 VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 1, 1);
        tab++;
        tab %= m_tabCount;
        q.x += 72;
    }
}

// 0x00C964E8.
void VIWndSpellBook::DrawSlots()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    unsigned short name[64];
    VIColor32F c;
    VIVect2I p(464, 60);
    int i;
    for (i = 0; i < 5; i++)
    {
        int lit = m_mode == 2 && i == m_x34;
        c = lit ? VIColor32F(1.0f, 1.0f, 0.0f, 1.0f) : VIColor32F(1.0f, 1.0f, 1.0f, 1.0f);
        if (lit)
        {
            ui->BlitBar("", c, p, (VIUITexId)56, (VIUITexId)58, (VIUITexId)57, 175,
                        (VIAlphaBlendMode)0, 1.0f, 0, 1);
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(p.x + 87, p.y + 13), 175, 1, 0);
            m_window.TickR(17, 0, true);
        }
        else
            ui->BlitBar("", c, p, (VIUITexId)53, (VIUITexId)55, (VIUITexId)54, 175,
                        (VIAlphaBlendMode)0, 1.0f, 0, 1);
        p.y += 39;
    }
    for (i = 0; i < m_slots.m_size; i++)
    {
        VIRec12_C979B0 *s = &m_slots[i];
        name[0] = 0;
        app->GetAbilityName(s->m_id, ui->m_language, name, sizeof(name));
        if (s->m_slot < 5)
        {
            int lit = m_mode == 2 && s->m_slot == m_x34;
            c = lit ? VIColor32F(1.0f, 1.0f, 0.0f, 1.0f) : VIColor32F(1.0f, 1.0f, 1.0f, 1.0f);
            p.Set(464, 60);
            p.y += s->m_slot * 39;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(p.x + 87, p.y + 13), 175, 1, 0);
            m_window.TextBox(name, 5, 0, 165, 1, c, 11);
            if (lit)
                m_window.TickR(17, 0, true);
        }
    }
}

// 0x00C96888.
void VIWndSpellBook::DrawSlots10()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    unsigned short name[64];
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(545, 155), 190, 10, 1);
    m_window.Open(true);
    int i;
    for (i = 0; i < 10; i++)
        m_window.TextBox(m_window.T(""), 20, i, m_window.Width() - 40, 1, (VIUICOLOR)0, 4);
    for (i = 0; i < m_slots.m_size; i++)
    {
        VIRec12_C979B0 *s = &m_slots[i];
        name[0] = 0;
        app->GetAbilityName(s->m_id, ui->m_language, name, sizeof(name));
        if (s->m_slot < 10)
        {
            int lit = m_mode == 2 && s->m_slot == m_x34;
            VIColor32F c = lit ? VIColor32F(1.0f, 1.0f, 0.0f, 1.0f)
                               : VIColor32F(1.0f, 1.0f, 1.0f, 1.0f);
            m_window.TextBox(name, 20, s->m_slot, m_window.Width() - 40, 1, c, 11);
        }
    }
    for (i = 0; i < 10; i++)
        if (m_mode == 2 && i == m_x34)
        {
            m_window.TickR(32, i, true);
            break;
        }
}
