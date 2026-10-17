#line 1 "C:/code-frontiers-gm/game/VIWndRecipe.cpp"
// VIWndRecipe (name RTTI's) -- CLIENT 0x00C8B588 .. 0x00C8E360: the tradeskill window.
// The left panel lists the components of the current skill, the right one the recipes
// for the selected component (VIClientPart3834C, VIClient's vtable slot 12, supplies
// them: name, level, difficulty), sorted by one of three columns.  X on a recipe opens a
// pop-up menu (VIWndMiniMenu) whose four items come back as message 261.  The VIMap<int,
// int> and VIArray<VIItemStat> copies between Inspect and __tf are template
// instantiations this TU does not write.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// Each skill's component words (0x00CEF118 ..), zero-terminated; Components() says how
// many.  Two skills share the first list.
static unsigned short sComponents1[] = { 1492, 1482, 1483, 1493, 1484, 1495, 1490, 1491, 1496, 1497, 1481, 0 };
static unsigned short sComponents0[] = { 1492, 1482, 1483, 1493, 1484, 1495, 1490, 1491, 1496, 1497, 1481, 0 };
static unsigned short sComponents2[] = { 1508, 1509, 1510, 1511, 1512, 1513, 1514, 1515, 1516, 1517, 1518, 1519,
                                         1520, 1521, 1522, 1523, 1524, 1494, 1481, 0 };
static unsigned short sComponents3[] = { 1482, 1483, 1484, 1485, 1486, 1487, 1488, 1489, 1490, 1491, 1481, 0 };
static unsigned short sComponents4[] = { 1498, 1499, 1500, 1501, 1502, 1503, 1504, 1505, 1506, 1507, 1481, 0 };
static unsigned short sComponents5[] = { 1526, 1527, 1528, 1529, 1530, 1531, 1532, 1533, 1534, 1535, 1536, 1537,
                                         1481, 0 };

// 0x00C8B588.
VIWndRecipe::VIWndRecipe()
{
    m_inspect = 0;
    m_menu = 0;
    m_mbox = 0;
    m_skill = 0;
    m_comp = 0;
    m_sel = 0;
    m_sort = 0;
    m_top = 0;
    m_compTop = 0;
}

// 0x00C8B608.
VIWndRecipe::~VIWndRecipe()
{
    Free();
}

// 0x00C8B690.  The message box, the inspect window and the pop-up menu, all hidden.
int VIWndRecipe::Init()
{
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        return -1;
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    if (m_mbox->Init(0, 1))
        return -1;
    m_mbox->SetParam1(-1);
    m_mbox->SetParam2(-1);
    m_inspect = new VIWndInspect;
    if (!m_inspect)
        return -1;
    m_inspect->SetVisible(false);
    m_inspect->SetAppPtr(GetAppPtr());
    if (m_inspect->Init(1, -1))
        return -1;
    AddChild(m_inspect);
    m_menu = new VIWndMiniMenu;
    if (!m_menu)
        return -1;
    m_menu->SetVisible(false);
    m_menu->SetAppPtr(GetAppPtr());
    m_menu->m_kind = 1;
    m_menu->SetMenu(&m_menuDef);
    AddChild(m_menu);
    m_menuDef.m_count = 4;
    m_menuDef.m_entries = m_menuItems;
    m_menuDef.m_x00 = -1;
    m_menuDef.m_x04 = -1;
    for (int k = 0; k < 4; k++)
    {
        m_menuItems[k].m_icon = -1;
        m_menuItems[k].m_icon2 = -1;
        m_menuItems[k].m_x08 = -1;
        m_menuItems[k].m_id = -1;
        m_menuItems[k].m_sub = 0;
        m_menuItems[k].m_number = -1;
    }
    m_menuItems[0].m_word = 1525;
    m_menuItems[0].m_msg = 0xA7040000;
    m_menuItems[1].m_word = 122;
    m_menuItems[1].m_msg = 0xA7040002;
    m_menuItems[2].m_word = 200;
    m_menuItems[2].m_msg = 0xA7040003;
    m_menuItems[3].m_word = 516;
    m_menuItems[3].m_msg = 0xA7040001;
    m_info.SetClient((VIClient *)GetAppPtr());
    m_skill = 0;
    m_comp = 0;
    m_sel = 0;
    m_sort = 0;
    m_mode = 0;
    m_top = 0;
    m_compTop = 0;
    return 0;
}

// 0x00C8B8D0.
int VIWndRecipe::Free()
{
    if (m_inspect)
    {
        RemoveChild(m_inspect);
        delete m_inspect;
        m_inspect = 0;
    }
    if (m_menu)
    {
        RemoveChild(m_menu);
        delete m_menu;
        m_menu = 0;
    }
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    return 0;
}

// 0x00C8B9A0.
void VIWndRecipe::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIClientPart3834C *part = app->GetP3834C();
    part->Select(m_skill, m_comp, m_list);
    if (m_list.m_size <= 0)
        m_mode = 0;
    m_mbox->SetPos(VIVect2I(160, 115));
    unsigned short buf[128];
    char name[64];
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    app->GetPlayerName(name, sizeof(name));
    VIString::snprintf16(buf, sizeof(buf), "%s: %S %d, %S %d %S", name, Word(ui, 503),
                         part->GetSlot04(m_skill), Word(ui, 537), part->GetSlot08(m_skill),
                         SkillName(m_skill));
    ui->BlitBar(buf, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f),
                VIVect2I(320 - (ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf) + 64) / 2, 10),
                (VIUITexId)33, (VIUITexId)34,
                ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf) + 64,
                (VIAlphaBlendMode)0, 1.0f, 0, 1);
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(410, 180), 430, 14, 0);
    m_window.SetTitle("");
    m_window.Draw();
    int cols[3] = { 35, 305, 345 };
    VIString::snprintf16(buf, sizeof(buf), "Component: %S", ComponentName(m_comp));
    m_window.TextBox(buf, cols[0], 0, m_window.Width() - 60, 1, (VIUICOLOR)0, 2);
    if (m_sort == 0)
        m_window.TextBox(m_window.T(509), cols[0], 1, cols[1] - cols[0], 1, (VIUICOLOR)0, 0);
    else
        m_window.TextBox(m_window.T(509), cols[0], 1, cols[1] - cols[0], 1, (VIUICOLOR)0, 0);
    if (m_sort == 1)
    {
        VIString::snprintf16(buf, sizeof(buf), "|}%S|{", Word(ui, 299));
        m_window.TextBox(buf, cols[1] - 12, 1, cols[2] - cols[1] + 12, 1, (VIUICOLOR)0, 0);
    }
    else
        m_window.TextBox(m_window.T(299), cols[1], 1, cols[2] - cols[1], 1, (VIUICOLOR)0, 0);
    if (m_sort == 2)
    {
        VIString::snprintf16(buf, sizeof(buf), "|}%S|{", Word(ui, 1542));
        m_window.TextBox(buf, cols[2] - 12, 1, 115, 1, (VIUICOLOR)0, 0);
    }
    else
        m_window.TextBox(m_window.T(1542), cols[2], 1, 115, 1, (VIUICOLOR)0, 0);
    int last = VIMAX(0, m_list.m_size - 1);
    m_sel = VIMIN(last, m_sel < 0 ? 0 : m_sel);
    if (m_sel - m_top >= 12)
        m_top = m_sel - 11;
    else if (m_sel < m_top)
        m_top = m_sel;
    else if (m_top > m_list.m_size - 12)
        m_top = m_top - 1 < 0 ? 0 : m_top - 1;
    float t;
    if (m_list.m_size == 12)
        t = 0.0f;
    else
    {
        t = (float)m_top / (float)(m_list.m_size - 12);
        t = VIMIN(1.0f, VIMAX(0.0f, t));
    }
    int row = 2;
    VIVect2I sp(m_window.TopLeft().x + m_window.Width() - 22, m_window.TopLeft().y + 10);
    int h = m_window.BottomRight().y - m_window.TopLeft().y - 19;
    m_window.DrawFrameVScroll(22, 2, m_window.Width() - 55, 12, t);
    int i = m_top;
    for (int n = 0; i < m_list.m_size && n < 12; n++, row++)
    {
        VIRec144_C017C0 *rec = &m_list.m_data[i];
        int hl = 0;
        if (i == m_sel)
            hl = m_mode == 1;
        if (hl)
            m_window.TickR(cols[0], row, app->m_p2BD40 == this);
        else if (i == m_sel && m_mode == 2)
        {
            VIVect2I p = m_window.RowCenter(row);
            p.x = m_window.TopLeft().x + cols[0];
            p.y -= 8;
            ui->StretchBlitEx((VIUITexId)61, p, VIVect2I(m_window.Width() - 80, 18), (VIAlphaBlendMode)3, 0.25f);
        }
        int c = hl ? 1 : 0;
        m_window.TextBox(rec->m_name, cols[0], row, cols[1] - cols[0], 1, (VIUICOLOR)c, 2);
        i++;
        VIString::snprintf16(buf, sizeof(buf), "%d", rec->m_level);
        m_window.TextBox(buf, cols[1], row, cols[2] - cols[1], 1, (VIUICOLOR)c, 0);
        m_window.TextBox(Difficulty(rec->m_difficulty), cols[2], row, 110, 1, (VIUICOLOR)c, 2);
    }
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(110, 180), 190, 14, 0);
    m_window.SetTitle("");
    m_window.Draw();
    if (m_comp - m_compTop >= 11)
        m_compTop = m_comp - 10;
    else if (m_comp < m_compTop)
        m_compTop = m_comp;
    i = m_compTop;
    for (row = 0; i < Components() && row < 11; row++)
    {
        if (i == m_comp)
            m_window.TickR(30, row, m_mode == 0);
        m_window.TextBox(ComponentName(i), 30, row, m_window.Width() - 55, 1, (VIUICOLOR)(i == m_comp), 2);
        i++;
    }
    if (Components() == 11)
        t = 0.0f;
    else
        t = (float)m_compTop / (float)(Components() - 11);
    sp.Set(m_window.TopLeft().x + m_window.Width() - 22, m_window.TopLeft().y + 10);
    h = m_window.BottomRight().y - m_window.TopLeft().y - 19;
    m_window.DrawVScroll(sp, h, t, 0);
    if (part->m_pending)
        VIString::snprintf16(buf, sizeof(buf), "|-|x%S", Word(ui, 212));
    else
        VIString::snprintf16(buf, sizeof(buf), "|x%S", Word(ui, 212));
    m_window.Text(buf, 20, 12, (VIUICOLOR)0, 0);
    VIString::snprintf16(buf, sizeof(buf), "|t%S", Word(ui, 161));
    m_window.Text(buf, 20, 13, (VIUICOLOR)0, 0);
    if (part->m_pending == 5 || part->m_pending == 6)
    {
        if (--m_ticks <= 0)
        {
            m_ticks = 15;
            if (++m_dots == 3)
                m_dots = 0;
        }
        if (part->m_pending == 5)
            VIString::strcpy16(buf, sizeof(buf), m_status.T(1668));
        else if (part->m_pending == 6)
            VIString::strcpy16(buf, sizeof(buf), m_status.T(1667));
        for (char d = 0; d < m_dots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.Open(false);
        m_status.Draw();
        m_status.SetTitle(m_status.T(271));
        m_status.TextBox(buf, 40, 1, m_status.Width() - 50, 1, (VIUICOLOR)0, 0);
    }
    else
        m_status.Close(true);
    ui->End2D();
    if (m_keys.Pulse(138))
        Prev();
    if (m_keys.Pulse(139))
        Next();
}

// 0x00C8C630.
void VIWndRecipe::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 19:
    case 138:
        m_keys.Press(138);
        break;
    case 20:
    case 139:
        m_keys.Press(139);
        break;
    case 17:
    case 136:
        Left();
        break;
    case 18:
    case 137:
        Right();
        break;
    case 13:
    case 134:
        Select();
        break;
    case 27:
    case 132:
        Back();
        break;
    }
}

// 0x00C8C768.
void VIWndRecipe::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        m_keys.Release(138);
        break;
    case 20:
    case 139:
        m_keys.Release(139);
        break;
    }
}

// 0x00C8C7E0.  295: the focus moved (p1 from, p2 to); 261: a menu item (p1 its id, p2
// the recipe); 260: a child closed; 268: the message box said yes.
void VIWndRecipe::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3834C *part = app->GetP3834C();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != this && (VIWnd *)p1 != m_inspect &&
            (VIWnd *)p1 != m_menu && (VIWnd *)p1 != m_mbox)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 400, 12, 0);
            m_window.Open(true);
            m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 310, 3, 0);
            m_status.Close(true);
            m_inspect->SetVisible(false);
            m_menu->SetVisible(false);
            m_mbox->SetVisible(false);
            m_list.Clear();
            m_sel = 0;
            m_sort = 0;
            m_comp = 0;
            m_mode = 0;
            m_top = 0;
            m_compTop = 0;
            m_dots = 0;
            m_pick = -1;
            m_ticks = 15;
            m_keys.Reset();
        }
        if ((VIWnd *)p1 == this && (VIWnd *)p2 != this)
            m_keys.Reset();
        break;
    case 261:
        w->SetVisible(false);
        app->PlaySound(13);
        switch (p1)
        {
        case (int)0xA7040000:
            Learn(p2);
            break;
        case (int)0xA7040001:
            Confirm(p2);
            break;
        case (int)0xA7040002:
            Forget(p2);
            break;
        case (int)0xA7040003:
            StartPick(p2);
            break;
        }
        break;
    case 260:
        if (w == m_inspect)
        {
            w->SetVisible(false);
            m_inspect->Clear();
            app->SetActiveWindow(this);
        }
        else if (w == m_menu)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        else if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        else
        {
            part->Clear();
            app->ClosePanel708();
        }
        break;
    case 268:
        if (w == m_mbox)
        {
            if (p1 == (int)0xA7040001)
                part->Send1(p2);
            m_mbox->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    }
}

// 0x00C8CB20.  Up: the component list, or the recipe list.
void VIWndRecipe::Prev()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int snd = 11;
    if (m_mode == 0)
    {
        m_sel = 0;
        if (m_comp <= 0)
            m_comp = VIMAX(0, Components() - 1);
        else if (--m_comp == 0)
            m_keys.Reset();
    }
    else if (m_mode == 1 || m_mode == 2)
    {
        if (m_sel <= 0)
        {
            m_sel = 0;
            snd = 9;
        }
        else
            m_sel--;
    }
    app->PlaySound(snd);
}

// 0x00C8CBF0.  Down.
void VIWndRecipe::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3834C *part = app->GetP3834C();
    int snd = 11;
    if (m_mode == 0)
    {
        m_sel = 0;
        if (m_comp >= Components() - 1)
            m_comp = 0;
        else
        {
            m_comp++;
            if (m_comp == Components() - 1)
                m_keys.Reset();
        }
    }
    else if (m_mode == 1 || m_mode == 2)
    {
        if (m_sel >= m_list.m_size - 1)
        {
            m_sel = m_list.m_size - 1;
            snd = 9;
        }
        else
            m_sel++;
    }
    app->PlaySound(snd);
}

// 0x00C8CCE0.
void VIWndRecipe::Left()
{
}

// 0x00C8CCE8.
void VIWndRecipe::Right()
{
}

// 0x00C8CCF0.  X: components -> recipes -> the recipe's menu -> pick.
void VIWndRecipe::Select()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int snd = 13;
    if (!app->GetP3834C()->m_pending)
    {
        if (m_mode == 0)
        {
            if (m_list.m_size > 0)
                m_mode = 1;
            else
                snd = 9;
        }
        else if (m_mode == 1)
        {
            if (m_list.m_size > 0)
                ShowMenu();
        }
        else if (m_mode == 2)
        {
            if (m_list.m_size > 0)
                Pick();
        }
    }
    else
        snd = 9;
    app->PlaySound(snd);
}

// 0x00C8CDC8.  Triangle: back a step, or close.
void VIWndRecipe::Back()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_mode == 0)
        app->ClosePanel708();
    else if (m_mode == 1)
        m_mode = 0;
    else if (m_mode == 2)
        m_mode = 1;
    app->PlaySound(8);
}

// 0x00C8CE38.  The pop-up menu on the selected recipe.
void VIWndRecipe::ShowMenu()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIRec144_C017C0 *rec = &m_list.m_data[m_sel];
    if (rec)
    {
        m_menu->SetTitle(rec->m_name);
        for (int k = 0; k < 4; k++)
            m_menuItems[k].m_id = rec->m_id;
        m_inspect->SetVisible(false);
        m_mbox->SetVisible(false);
        m_menu->SetPos(VIVect2I(170, 100));
        m_menu->ResetScroll();
        m_menu->SetVisible(true);
        app->SetActiveWindow(m_menu);
    }
}

// 0x00C8CF10.
void VIWndRecipe::Pick()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3834C *part = app->GetP3834C();
    VIRec144_C017C0 *rec = &m_list.m_data[m_sel];
    if (rec)
    {
        if (m_pick != rec->m_id)
            part->Send3(m_pick, rec->m_id);
        m_mode = 1;
    }
}

// 0x00C8CF88.
int VIWndRecipe::Unused8CF88()
{
    return 0;
}

// 0x00C8CF90.
int VIWndRecipe::Components()
{
    switch (m_skill)
    {
    case 2:
        return 19;
    case 5:
        return 13;
    }
    return 11;
}

// 0x00C8CFC8.
const unsigned short *VIWndRecipe::ComponentName(int i)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    const unsigned short *s = Word(ui, 413);
    if (i >= 0 && i < Components())
    {
        unsigned short *tab = 0;
        switch (m_skill)
        {
        case 1:
            tab = sComponents1;
            break;
        case 0:
            tab = sComponents0;
            break;
        case 2:
            tab = sComponents2;
            break;
        case 3:
            tab = sComponents3;
            break;
        case 4:
            tab = sComponents4;
            break;
        case 5:
            tab = sComponents5;
            break;
        }
        if (tab)
            s = Word(ui, tab[i]);
    }
    return s;
}

// 0x00C8D0D0.
const unsigned short *VIWndRecipe::Difficulty(int d)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    const unsigned short *s = Word(ui, 413);
    if (d >= 0 && d < 4)
    {
        switch (d)
        {
        case 0:
            return Word(ui, 1648);
        case 1:
            return Word(ui, 1649);
        case 2:
            return Word(ui, 1650);
        case 3:
            return Word(ui, 1651);
        }
    }
    return s;
}

// 0x00C8D198.
const unsigned short *VIWndRecipe::SkillName(int skill)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    const unsigned short *s;
    switch (skill)
    {
    case 1:
        s = Word(ui, 1583);
        break;
    case 0:
        s = Word(ui, 1584);
        break;
    case 2:
        s = Word(ui, 1586);
        break;
    case 3:
        s = Word(ui, 1585);
        break;
    case 4:
        s = Word(ui, 1587);
        break;
    case 5:
        s = Word(ui, 179);
        break;
    default:
        return Word(ui, 413);
    }
    return s;
}

// 0x00C8D248.
void VIWndRecipe::Learn(int id)
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->GetP3834C()->Send2(id, 0);
    app->SetActiveWindow(this);
}

// 0x00C8D2B8.
void VIWndRecipe::Forget(int id)
{
    ((VIClient *)GetAppPtr())->GetP3834C()->Send7(id);
    m_menu->SetVisible(false);
    m_mbox->SetVisible(false);
}

// 0x00C8D318.  Ask before combining.
void VIWndRecipe::Confirm(int id)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIRec144_C017C0 *rec = app->GetP3834C()->GetRecipe(id);
    unsigned short buf[128];
    if (rec)
        VIString::snprintf16(buf, sizeof(buf), Word(ui, 1590), rec->m_name);
    else
        VIString::snprintf16(buf, sizeof(buf), Word(ui, 1591));
    Ask(buf, 0xA7040001, id);
}

// 0x00C8D3E8.
void VIWndRecipe::StartPick(int id)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_pick = id;
    m_mode = 2;
    app->SetActiveWindow(this);
}

// 0x00C8D430.
void VIWndRecipe::SetSkill(int skill)
{
    m_skill = skill;
}

// 0x00C8D438.  A one-button message.
void VIWndRecipe::Message(const unsigned short *text, int x)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_menu->SetVisible(false);
    m_inspect->SetVisible(false);
    m_mbox->Init(0, 1);
    m_mbox->SetText(text);
    m_mbox->SetParam1(-1);
    m_mbox->SetParam2(-1);
    m_mbox->m_icon = x;
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}

// 0x00C8D4E0.  A yes/no question; 268 comes back with p1, p2.
void VIWndRecipe::Ask(const unsigned short *text, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_menu->SetVisible(false);
    m_inspect->SetVisible(false);
    m_mbox->Init(1, 1);
    m_mbox->SetText(text);
    m_mbox->SetParam1(p1);
    m_mbox->SetParam2(p2);
    m_mbox->m_icon = -1;
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}

// 0x00C8D598.
void VIWndRecipe::Inspect()
{
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short buf[1024];
    m_info.DescribeRecipe(buf, sizeof(buf));
    m_inspect->Init(1, -1);
    m_inspect->SetText(buf);
    m_inspect->SetVisible(true);
    app->SetActiveWindow(m_inspect);
}
