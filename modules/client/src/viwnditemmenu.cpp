#line 1 "C:/code-frontiers-gm/game/VIWndItemMenu.cpp"
// viwnditemmenu.cpp -- CLIENT 0x00C658C8 .. 0x00C685F8: VIWndItemMenu, the inventory
// menu.  Four category tabs (VIWndMenu), the selected entry's description,
// and a quick-slot mode.
#include "game.h"

// The icon drawn on each quick slot (0x00CEAD08).
static int sSlotIcons[] = { 5, 5, 3, 2 };

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI &ui, int id)
{
    return (const unsigned short *)ui.m_words.GetString(id, ui.m_language);
}

// 0x00C658C8.
VIWndItemMenu::VIWndItemMenu()
{
    m_menuD = 0;
    m_menuB = 0;
    m_menuA = 0;
    m_inspect = 0;
    m_quantity = 0;
    m_mbox = 0;
    m_menuC = 0;
    m_tab = 0;
    m_p28 = 1;
    m_slot = 0;
    m_time = 0;
    m_menu = -2;
    m_item = -1;
    m_fadeIn = 1;
    m_alpha = 1.0f;
}

// 0x00C65960.
VIWndItemMenu::~VIWndItemMenu()
{
    Free();
}

// 0x00C659C0.  The four tab menus, the inspect window, the quantity picker and the
// confirmation box, all hidden.
int VIWndItemMenu::Init()
{
    int err;
    m_menuD = new VIWndMenu;
    if (!m_menuD)
    {
        err = -1;
        goto done;
    }
    m_menuD->SetVisible(false);
    m_menuD->SetAppPtr(GetAppPtr());
    m_menuD->SetPos(VIVect2I(32, 48));
    m_menuD->SetStyle(1);
    AddChild(m_menuD);
    m_menuB = new VIWndMenu;
    if (!m_menuB)
    {
        err = -1;
        goto done;
    }
    m_menuB->SetVisible(false);
    m_menuB->SetAppPtr(GetAppPtr());
    m_menuB->SetPos(VIVect2I(32, 48));
    m_menuB->SetStyle(1);
    AddChild(m_menuB);
    m_menuA = new VIWndMenu;
    if (!m_menuA)
    {
        err = -1;
        goto done;
    }
    m_menuA->SetVisible(false);
    m_menuA->SetAppPtr(GetAppPtr());
    m_menuA->SetPos(VIVect2I(32, 48));
    m_menuA->SetStyle(1);
    AddChild(m_menuA);
    m_menuC = new VIWndMenu;
    if (!m_menuC)
    {
        err = -1;
        goto done;
    }
    m_menuC->SetVisible(false);
    m_menuC->SetAppPtr(GetAppPtr());
    m_menuC->SetPos(VIVect2I(32, 48));
    m_menuC->SetStyle(1);
    AddChild(m_menuC);
    m_inspect = new VIWndInspect;
    if (!m_inspect)
    {
        err = -1;
        goto done;
    }
    m_inspect->SetVisible(false);
    m_inspect->SetAppPtr(GetAppPtr());
    AddChild(m_inspect);
    if (m_inspect->Init(1, -1))
    {
        err = -1;
        goto done;
    }
    m_quantity = new VIWndQuantity;
    if (!m_quantity)
    {
        err = -1;
        goto done;
    }
    m_quantity->SetAppPtr(GetAppPtr());
    AddChild(m_quantity);
    if (m_quantity->Init())
    {
        err = -1;
        goto done;
    }
    VIVect2I(190, 90);
    m_mbox = new VIWndMBox;
    if (!m_mbox)
    {
        err = -1;
        goto done;
    }
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    err = m_mbox->Init(1, 1);
    if (err)
    {
        err = -1;
        goto done;
    }
    m_mbox->SetPos(VIVect2I(190, 115));
    m_mbox->SetParam1(-1);
    m_mbox->SetParam2(-1);
    m_tab = 0;
    m_p28 = 1;
    m_slot = 0;
    m_time = 0;
    m_item = -1;
    m_menu = -2;
    m_fadeIn = 1;
    m_alpha = 1.0f;
    m_info.SetClient((VIClient *)GetAppPtr());
done:
    return err;
}

// 0x00C65CF8.
int VIWndItemMenu::Free()
{
    if (m_menuD)
    {
        RemoveChild(m_menuD);
        delete m_menuD;
        m_menuD = 0;
    }
    if (m_menuB)
    {
        RemoveChild(m_menuB);
        delete m_menuB;
        m_menuB = 0;
    }
    if (m_menuA)
    {
        RemoveChild(m_menuA);
        delete m_menuA;
        m_menuA = 0;
    }
    if (m_menuC)
    {
        RemoveChild(m_menuC);
        delete m_menuC;
        m_menuC = 0;
    }
    if (m_inspect)
    {
        RemoveChild(m_inspect);
        delete m_inspect;
        m_inspect = 0;
    }
    if (m_quantity)
    {
        RemoveChild(m_quantity);
        delete m_quantity;
        m_quantity = 0;
    }
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    m_tab = 0;
    m_p28 = 1;
    m_slot = 0;
    m_menu = -2;
    m_item = -1;
    m_fadeIn = 1;
    m_alpha = 1.0f;
    return 0;
}

// 0x00C65ED8.  The tab bar, then the description (or the quick slots).
void VIWndItemMenu::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    long now = app->m_p07568;
    VIUI &ui = app->m_ui;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    UpdateDesc();
    ui.Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIVect2I pos;
    VIVect2I lift;
    pos.Set(110, 20);
    lift = pos;
    ui.BlitBar("", black, VIVect2I(70, 8), (VIUITexId)33, (VIUITexId)34, 500,
               (VIAlphaBlendMode)0, 1.0f, 0, 1);
    ui.DrawCenteredStringEx((unsigned short)437, pos.x, pos.y, white, 1, 1);
    if (m_tab == 0)
        lift.x = pos.x - 25;
    pos.x += 100;
    ui.DrawCenteredStringEx((unsigned short)438, pos.x, pos.y, white, 1, 1);
    if (m_tab == 1)
        lift.x = pos.x - 25;
    pos.x += 140;
    ui.DrawCenteredStringEx((unsigned short)1473, pos.x, pos.y, white, 1, 1);
    if (m_tab == 2)
        lift.x = pos.x - 25;
    pos.x += 140;
    ui.DrawCenteredStringEx((unsigned short)439, pos.x, pos.y, white, 1, 1);
    if (m_tab == 3)
        lift.x = pos.x - 25;
    ui.BlitRect((VIUITexId)51, VIVect2I(-1, 48), VIVect2I(175, 265), (VIAlphaBlendMode)0, 1.0f);
    const VIUITexEntry *e = GetUITexture((VIUITexId)89);
    lift.y = pos.y - e->m_uv1.y / 2;
    ui.BlitEx((VIUITexId)e->m_x00, lift, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_menu != -1)
        m_window.TextBox(m_desc, 20, 0, m_window.Width() - 40, 15, white, 3);
    else
        DrawSlots();
    ui.End2D();
    if (m_menu == -1)
    {
        if (m_fadeIn == -1)
        {
            m_alpha -= dt * 2.0f;
            m_alpha = m_alpha < 0.0f ? 0.0f : m_alpha;
            if (m_alpha == 0.0f)
                m_fadeIn = 1;
        }
        else
        {
            m_alpha += dt * 3.0f;
            m_alpha = m_alpha < 1.0f ? m_alpha : 1.0f;
            if (m_alpha == 1.0f)
                m_fadeIn = -1;
        }
    }
    else
        m_alpha = 1.0f;
}

// 0x00C662A0.  The quick-slot mode: the slot column, the cursor pulsing on m_slot, and
// the slotted item's name.
void VIWndItemMenu::DrawSlots()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    VIVect2I base(522, 60);
    VIVect2I pos = base + VIVect2I(6, 8);
    const VIUITexEntry *top = GetUITexture((VIUITexId)158);
    const VIUITexEntry *mid = GetUITexture((VIUITexId)121);
    const VIUITexEntry *bot = GetUITexture((VIUITexId)120);
    ui.BlitEx((VIUITexId)top->m_x00, base, (VIAlphaBlendMode)0, 1.0f, 0);
    base.y += top->m_uv1.y;
    ui.BlitEx((VIUITexId)mid->m_x00, base, VIVect2I(34, mid->m_uv1.y), (VIAlphaBlendMode)0, 1.0f, 0);
    base.y += mid->m_uv1.y;
    ui.BlitEx((VIUITexId)mid->m_x00, base, VIVect2I(34, mid->m_uv1.y), (VIAlphaBlendMode)0, 1.0f, 0);
    base.y += mid->m_uv1.y;
    ui.BlitEx((VIUITexId)mid->m_x00, base, VIVect2I(34, mid->m_uv1.y), (VIAlphaBlendMode)0, 1.0f, 0);
    base.y += mid->m_uv1.y;
    ui.BlitEx((VIUITexId)mid->m_x00, base, VIVect2I(34, mid->m_uv1.y - 4), (VIAlphaBlendMode)0, 1.0f, 0);
    base.y += mid->m_uv1.y - 4;
    ui.BlitEx((VIUITexId)158, base, VIVect2I(top->m_uv1.x, 3), (VIAlphaBlendMode)0, 1.0f, 2);
    for (int i = 0; i < gHudMenu2.m_count; i++)
    {
        if (i == m_slot)
        {
            VIVect2I at = pos + VIVect2I(-25, 2);
            ui.BlitEx((VIUITexId)86, at, (VIAlphaBlendMode)3, m_alpha, 1);
        }
        ui.Blit((VIUITexId)sSlotIcons[i], pos);
        pos.y += 32;
    }
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    m_window.TextBox(m_slotName, 20, 11, m_window.Width() - 40, 2, white, 11);
}

// 0x00C66630.
void VIWndItemMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:
        if (w == m_inspect || w == m_quantity || w == m_mbox)
        {
            w->SetVisible(false);
            if (w == m_inspect)
            {
                m_inspect->SetVisible(false);
                m_inspect->Clear();
            }
            ShowMenu(0);
        }
        else
            m_parent->OnMessage(this, 260, 0, 0);
        break;
    case 293:
        if (p1 > 0)
            Use(p2, p1);
        else
            ShowMenu(0);
        m_quantity->SetVisible(false);
        break;
    case 268:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            app->UseItemOn(p1, p2);
        }
        ShowMenu(0);
        break;
    case 297:
        InspectBack(p2);
        break;
    case 277:
        if (w == m_menuD || w == m_menuB || w == m_menuA || w == m_menuC)
        {
            if (p1 != p2)
                app->SwapItems(p1, p2);
            ((VIWndList *)w)->SetX60(0);
            ((VIWndList *)w)->Select(-1);
        }
        break;
    case 261:
    {
        int sound = 13;
        if (app->m_p13B90)
            sound = 10;
        else
            switch (p1)
            {
            case 0x02030000:
                if (p2 < 0 || p2 >= app->GetInvCount() || app->m_p13B90)
                    sound = 10;
                else
                    app->UseItem(p2, 0);
                break;
            case 0x02020000:
                if (p2 < 0 || p2 >= app->GetInvCount() || app->m_p13B90)
                    sound = 10;
                else
                    Inspect(p2);
                break;
            case 0x02060000:
                if (p2 < 0 || p2 >= app->GetInvCount() || app->m_p13B90)
                    sound = 10;
                else if (app->GetInvItemAmount(p2) > 1)
                    m_quantity->Open(app->GetInvItemAmount(p2), p2, -1);
                else
                    Use(p2, 1);
                break;
            case 0x02040000:
                break;
            case 0x02070000:
                if (w == m_menuA || w == m_menuB)
                {
                    VIWndMenu *menu = (VIWndMenu *)w;
                    if (AssignSlot(menu, menu->GetId(menu->GetSel())) < 0)
                        sound = 10;
                }
                break;
            case 0x02050000:
                if (w == m_menuD || w == m_menuB || w == m_menuA || w == m_menuC)
                {
                    VIWndList *list = (VIWndList *)w;
                    if (list->Count() >= 2)
                    {
                        list->Select(list->GetSel());
                        list->SetX60(1);
                    }
                }
                break;
            }
        app->PlaySound(sound);
        break;
    }
    case 295:
        if ((VIWnd *)p2 == this)
        {
            m_fadeIn = 1;
            m_alpha = 1.0f;
            m_time = app->m_p07568;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(535, 180), 200, 15, 1);
            m_window.Open(true);
        }
        break;
    }
}

// 0x00C66AD0.  An equipped, quick-slotted or no-trade item asks first (268 comes back).
void VIWndItemMenu::Use(int item, int count)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    if (item < 0 || item >= app->GetInvCount() || app->m_p13B90)
    {
        ShowMenu(0);
        return;
    }
    unsigned short name[128];
    unsigned short buf[256];
    VIString::strcpy16(buf, sizeof(buf), Word(ui, 915));
    int ask = 0;
    app->GetInvItemName(item, name, sizeof(name));
    if (app->GetInvItemEquipSlot(item) != -1)
    {
        ask = 1;
        VIString::snprintf16(buf, sizeof(buf), Word(ui, 916), name);
    }
    else if (app->GetInvItemQuickSlot(item) >= 0)
    {
        ask = 1;
        VIString::snprintf16(buf, sizeof(buf), Word(ui, 917), name);
    }
    else if (app->GetInvItemNoTrade(item) > 0)
    {
        ask = 1;
        VIString::snprintf16(buf, sizeof(buf), Word(ui, 918));
    }
    if (ask)
    {
        m_mbox->SetText(buf);
        m_mbox->SetParam1(item);
        m_mbox->SetParam2(count);
        m_mbox->SetVisible(true);
        app->SetActiveWindow(m_mbox);
    }
    else
    {
        app->UseItemOn(item, count);
        ShowMenu(0);
    }
}

// 0x00C66CA8.
void VIWndItemMenu::ShowMenu(int reset)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_menuA->SetVisible(false);
    m_menuB->SetVisible(false);
    m_menuD->SetVisible(false);
    m_menuC->SetVisible(false);
    switch (m_menu)
    {
    case 0:
        if (reset)
            m_menuA->Reset();
        m_menuA->SetVisible(true);
        m_menuA->SetX64(1);
        app->SetActiveWindow(m_menuA);
        break;
    case 1:
        if (reset)
            m_menuB->Reset();
        m_menuB->SetVisible(true);
        m_menuB->SetX64(1);
        app->SetActiveWindow(m_menuB);
        break;
    case 3:
        if (reset)
            m_menuD->Reset();
        m_menuD->SetVisible(true);
        m_menuD->SetX64(1);
        app->SetActiveWindow(m_menuD);
        break;
    case 2:
        if (reset)
            m_menuC->Reset();
        m_menuC->SetVisible(true);
        m_menuC->SetX64(1);
        app->SetActiveWindow(m_menuC);
        break;
    }
}

// 0x00C66E50.
void VIWndItemMenu::Inspect(int item)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_menu)
    {
    case 0:
        InspectA(item);
        break;
    case 1:
        InspectB(item);
        break;
    case 3:
        InspectD(item);
        break;
    case 2:
        InspectC(item);
        break;
    }
    m_menuA->SetVisible(false);
    m_menuB->SetVisible(false);
    m_menuD->SetVisible(false);
    m_menuC->SetVisible(false);
    m_inspect->SetVisible(true);
    app->SetActiveWindow(m_inspect);
}

// 0x00C66F48.
void VIWndItemMenu::InspectA(int item)
{
    unsigned short buf[1024];
    m_info.DescribeInvUsable(buf, sizeof(buf), item);
    m_inspect->Init(1, -1);
    m_inspect->SetText(buf);
}

// 0x00C66F98.
void VIWndItemMenu::InspectB(int item)
{
    unsigned short buf[1024];
    m_info.DescribeInvEquipment(buf, sizeof(buf), item);
    m_inspect->Init(2, item);
    m_inspect->SetText(buf);
}

// 0x00C66FF8.
void VIWndItemMenu::InspectBack(int p)
{
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short buf[1024];
    m_info.DescribeInvStats(buf, sizeof(buf), p);
    m_inspect->SetText(buf);
    app->SetActiveWindow(m_inspect);
}

// 0x00C67058.
void VIWndItemMenu::InspectD(int item)
{
    unsigned short buf[1024];
    m_info.DescribeInvMisc(buf, sizeof(buf), item);
    m_inspect->Init(1, -1);
    m_inspect->SetText(buf);
}

// 0x00C670A8.
void VIWndItemMenu::InspectC(int item)
{
    unsigned short buf[1024];
    m_info.DescribeInvCharged(buf, sizeof(buf), item);
    m_inspect->Init(1, -1);
    m_inspect->SetText(buf);
}

// 0x00C670F8.  Tab 0's items go to slot 0 or 2 by type, tab 1's to slot 3.
int VIWndItemMenu::AssignSlot(VIWndMenu *menu, int item)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_item = item;
    if (menu == m_menuA)
    {
        switch (app->GetInvItemP4C(item))
        {
        case 2:
            m_slot = 0;
            break;
        case 7:
            m_slot = 2;
            break;
        default:
            return -1;
        }
    }
    else if (menu == m_menuB)
        m_slot = 3;
    else
        return -1;
    menu->SetX64(0);
    m_menu = -1;
    UpdateSlotName();
    app->SetActiveWindow(this);
    return 0;
}

// 0x00C671D8.
void VIWndItemMenu::BackToMenu()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_menuD->GetVisible())
        m_menu = 3;
    else if (m_menuA->GetVisible())
        m_menu = 0;
    else
        m_menu = 1;
    ShowMenu(0);
}

// 0x00C67238.
void VIWndItemMenu::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 17:
    case 136:
        if (m_menu != -1)
            ChangeTab(-1);
        break;
    case 18:
    case 137:
        if (m_menu != -1)
            ChangeTab(1);
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        if (m_menu != -1)
            m_parent->OnMessage(this, 260, 0, 0);
        else
            BackToMenu();
        break;
    case 13:
    case 134:
        app->PlaySound(13);
        if (m_menu == -1)
        {
            app->RequestQuickSlot(m_item, m_slot);
            BackToMenu();
        }
        break;
    case 19:
    case 138:
        if (m_menu == -1)
            ChangeSlot(-1);
        break;
    case 20:
    case 139:
        if (m_menu == -1)
            ChangeSlot(1);
        break;
    }
}

// 0x00C67428.
void VIWndItemMenu::Open(VIWnd *from)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_quantity->SetVisible(false);
    m_menuD->SetVisible(false);
    m_menuB->SetVisible(false);
    m_menuC->SetVisible(false);
    m_inspect->SetVisible(false);
    m_mbox->SetVisible(false);
    m_p28 = 1;
    SetVisible(true);
    m_tab = 0;
    ClearDesc();
    m_menu = 0;
    m_menuA->Reset();
    m_menuA->SetVisible(true);
    app->SetActiveWindow(m_menuA);
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(535, 180), 200, 15, 1);
    m_window.Open(true);
}

// 0x00C67520.
void VIWndItemMenu::SetMenuD(VIMenuDef *menu)
{
    m_menuD->SetMenu(menu);
}

// 0x00C67540.
void VIWndItemMenu::SetMenuB(VIMenuDef *menu)
{
    m_menuB->SetMenu(menu);
}

// 0x00C67560.
void VIWndItemMenu::SetMenuA(VIMenuDef *menu)
{
    m_menuA->SetMenu(menu);
}

// 0x00C67580.
void VIWndItemMenu::SetMenuC(VIMenuDef *menu)
{
    m_menuC->SetMenu(menu);
}

// 0x00C675A0.
void VIWndItemMenu::ChangeTab(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_menu + d < 0)
    {
        app->PlaySound(9);
        m_menu = 0;
        m_tab = 0;
    }
    else if (m_menu + d >= 4)
    {
        app->PlaySound(9);
        m_menu = 3;
        m_tab = 3;
    }
    else
    {
        app->PlaySound(11);
        m_menu += d;
        m_tab += d;
    }
    ShowMenu(1);
    m_fadeIn = 1;
    m_alpha = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C67670.
void VIWndItemMenu::ChangeSlot(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_slot + d < 0)
    {
        app->PlaySound(9);
        m_slot = 0;
    }
    else if (m_slot + d >= 4)
    {
        app->PlaySound(9);
        m_slot = 3;
    }
    else if (m_slot == 3 || app->GetInvItemP4C(m_item) == 7)
        app->PlaySound(9);
    else if (app->GetInvItemP4C(m_item) != 7)
    {
        if (m_slot + d < 2)
        {
            app->PlaySound(11);
            m_slot += d;
        }
        else
            app->PlaySound(9);
    }
    UpdateSlotName();
    m_fadeIn = 1;
    m_alpha = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C67790.
void VIWndItemMenu::ClearDesc()
{
    m_desc[0] = 0;
}

// 0x00C67798.
void VIWndItemMenu::UpdateSlotName()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_slotName[0] = 0;
    int i = app->FindInvByQuickSlot(m_slot);
    if (i >= 0)
        app->GetInvItemName(i, m_slotName, sizeof(m_slotName));
}

// 0x00C677F0.
void VIWndItemMenu::UpdateDesc()
{
    switch (m_menu)
    {
    case 3:
        DescD();
        break;
    case 0:
        DescA();
        break;
    case 1:
        DescB();
        break;
    case 2:
        DescC();
        break;
    case -2:
        ClearDesc();
        break;
    case -1:
        ClearDesc();
        break;
    }
}

// 0x00C67890.
void VIWndItemMenu::DescC()
{
    unsigned short buf[256];
    int id = m_menuC->GetId(m_menuC->GetSel());
    m_info.DescribeInvChargedShort(buf, sizeof(buf), id);
    VIString::strcpy16(m_desc, sizeof(m_desc), buf);
}

// 0x00C678E0.
void VIWndItemMenu::DescD()
{
    unsigned short buf[256];
    int id = m_menuD->GetId(m_menuD->GetSel());
    m_info.DescribeInvFood(buf, sizeof(buf), id);
    VIString::strcpy16(m_desc, sizeof(m_desc), buf);
}

// 0x00C67930.
void VIWndItemMenu::DescA()
{
    unsigned short buf[256];
    int id = m_menuA->GetId(m_menuA->GetSel());
    m_info.DescribeInvConsumable(buf, sizeof(buf), id);
    VIString::strcpy16(m_desc, sizeof(m_desc), buf);
}

// 0x00C67980.
void VIWndItemMenu::DescB()
{
    unsigned short buf[256];
    int id = m_menuB->GetId(m_menuB->GetSel());
    m_info.DescribeInvEquipmentShort(buf, sizeof(buf), id);
    VIString::strcpy16(m_desc, sizeof(m_desc), buf);
}
