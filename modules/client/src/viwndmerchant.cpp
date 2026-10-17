#line 1 "C:/code-frontiers-gm/game/VIWndMerchant.cpp"
// VIWndMerchant.cpp -- CLIENT 0x00C77750 .. 0x00C7A000.  The merchant: the buy/sell menu,
// the item list, the quantity picker and the confirmation box, and the selected item's
// text, stat changes, price and the player's coin.
#include "game.h"

// 0x00CF5AC8 / 0x00CF5AD8, built by the TU's static initialiser (0x00C79F38).
static VIColor32F sBlack(0.0f, 0.0f, 0.0f, 1.0f);
static VIColor32F sWhite(1.0f, 1.0f, 1.0f, 1.0f);

// 0x00C77750.
VIWndMerchant::VIWndMerchant()
{
    m_menu = 0;
    m_list = 0;
    m_quantity = 0;
    m_mbox = 0;
    m_coin = 0;
    m_mode = 0;
    m_x30 = VIVect2I(0, 0);
}

// 0x00C777F0.
VIWndMerchant::~VIWndMerchant()
{
    Free();
}

// 0x00C77850.
int VIWndMerchant::Init()
{
    int r;
    m_menu = new VIWndMercMenu;
    if (!m_menu)
        goto fail;
    m_menu->SetAppPtr(GetAppPtr());
    AddChild(m_menu);
    if (m_menu->Init())
        goto fail;
    m_list = new VIWndList;
    if (!m_list)
        goto fail;
    m_list->SetAppPtr(GetAppPtr());
    m_list->SetPos(VIVect2I(12, 48));
    m_list->SetStyle(1);
    AddChild(m_list);
    m_list->Init(40);
    m_quantity = new VIWndQuantity;
    if (!m_quantity)
        goto fail;
    m_quantity->SetAppPtr(GetAppPtr());
    AddChild(m_quantity);
    if (m_quantity->Init())
        goto fail;
    {
        VIVect2I pos(180, 90);
        m_mbox = new VIWndMBox;
        if (!m_mbox)
            goto fail;
        m_mbox->SetVisible(false);
        m_mbox->SetAppPtr(GetAppPtr());
        AddChild(m_mbox);
        r = m_mbox->Init(1, 1);
    }
    if (r)
    {
    fail:
        r = -1;
        goto out;
    }
    m_mbox->SetPos(VIVect2I(180, 115));
    m_mbox->SetParam1(-1);
    m_mbox->SetParam2(-1);
    m_x30 = VIVect2I(143, 10);
    m_mode = 0;
    ClearInfo();
    m_coin = 0;
    m_info.SetClient((VIClient *)GetAppPtr());
out:
    return r;
}

// 0x00C77A60.
int VIWndMerchant::Free()
{
    if (m_list)
    {
        RemoveChild(m_list);
        delete m_list;
        m_list = 0;
    }
    if (m_menu)
    {
        RemoveChild(m_menu);
        delete m_menu;
        m_menu = 0;
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
    m_mode = 0;
    ClearInfo();
    m_coin = 0;
    return 0;
}

// 0x00C77B78.  The panels behind the list and the item text, then the list's rows.
void VIWndMerchant::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    if (!app->IsMerchantOpen())
        return;
    UpdateInfo();
    if (m_mode)
    {
        ui.Begin2D();
        ui.BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
        VIUIFrameParam fp;
        VIUIFrameParam fp2;
        memset(&fp, 0, sizeof(fp));
        fp.m_bottom.m_id = (VIUITexId)49;
        fp.m_left.m_id = (VIUITexId)47;
        fp.m_br.m_id = (VIUITexId)50;
        fp.m_right.m_id = (VIUITexId)52;
        fp.m_top.m_id = (VIUITexId)45;
        fp.m_tl.m_id = (VIUITexId)44;
        fp.m_tr.m_id = (VIUITexId)46;
        fp.m_bl.m_id = (VIUITexId)48;
        ui.BlitRect((VIUITexId)51, VIVect2I(-1, 48), VIVect2I(156, 265), (VIAlphaBlendMode)0, 1.0f);
        ui.BlitFillFrame(VIVect2I(456, 48), VIVect2I(184, 202), fp, (VIUITexId)35,
                         (VIAlphaBlendMode)0, 1.0f, 1);
        ui.BlitFillFrame(VIVect2I(167, 48), VIVect2I(289, 202), fp, (VIUITexId)35,
                         (VIAlphaBlendMode)0, 1.0f, 1);
        ui.BlitFillFrame(VIVect2I(167, 250), VIVect2I(473, 65), fp, (VIUITexId)35,
                         (VIAlphaBlendMode)0, 1.0f, 1);
        memset(&fp, 0, sizeof(fp));
        memset(&fp2, 0, sizeof(fp2));
        fp.m_bottom.m_id = (VIUITexId)40;
        fp.m_left.m_id = (VIUITexId)42;
        fp.m_bl.m_id = (VIUITexId)39;
        fp.m_right.m_id = (VIUITexId)42;
        fp.m_br.m_id = (VIUITexId)39;
        fp.m_br.m_flags = 1;
        fp.m_right.m_flags = 1;
        fp.m_top.m_id = (VIUITexId)37;
        fp.m_tl.m_id = (VIUITexId)36;
        fp.m_tr.m_id = (VIUITexId)36;
        fp2.m_bottom.m_id = (VIUITexId)41;
        fp.m_tr.m_flags = 1;
        fp2.m_left.m_id = (VIUITexId)43;
        fp2.m_right.m_id = (VIUITexId)43;
        fp2.m_right.m_flags = 1;
        fp2.m_top.m_id = (VIUITexId)38;
        ui.BlitStuddedHorz(VIVect2I(-1, 37), (VIUITexId)37, (VIUITexId)38, 642, 5,
                           (VIAlphaBlendMode)0, 1.0f, 0);
        ui.BlitStuddedHorz(VIVect2I(-1, 313), (VIUITexId)40, (VIUITexId)41, 642, 5,
                           (VIAlphaBlendMode)0, 1.0f, 0);
        ui.End2D();
    }
    if (m_mode == 1)
        FillInvList();
    else if (m_mode == 2)
        FillMerchantList();
}

// 0x00C77EB8.  The list takes the pad's up/down; Enter waits while a request is out.
void VIWndMerchant::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        GetParent()->OnMessage(this, 288, 0, 0);
        Open();
        break;
    case 19:
    case 20:
    case 138:
    case 139:
        m_list->OnKeyDown(key);
        break;
    case 13:
    case 134:
        if (!app->m_p1B304)
            m_list->OnKeyDown(key);
        else
            app->PlaySound(9);
        break;
    }
}

// 0x00C77FF0.
void VIWndMerchant::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 20:
    case 138:
    case 139:
        m_list->OnKeyUp(key);
        break;
    default:
        VIWnd::OnKeyUp(key);
        break;
    }
}

// 0x00C78058.  256: a row picked; 280/281 from the menu (sell/buy); 293 the quantity;
// 268 the confirmation; 260 a child cancelled; 295 the focus.
void VIWndMerchant::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int price;
    switch (msg)
    {
    case 256:
        if (m_mode == 2 && app->GetMerchantItemMaxStack(p1))
        {
            app->PlaySound(13);
            price = app->GetMerchantPrice(p1);
            m_quantity->Open(app->GetMerchantItemMaxStack(p1), p1, price);
        }
        else if (m_mode == 1 && app->GetInvItemAmount(p1) >= 2)
        {
            app->PlaySound(13);
            price = app->GetInvItemSellPrice(p1);
            m_quantity->Open(app->GetInvItemAmount(p1), p1, price);
        }
        else if (m_mode == 2)
            Buy(p1, 1);
        else if (m_mode == 1)
            Sell(p1, 1);
        else
            m_quantity->Open(1, p1, -1);
        break;
    case 280:
        GetParent()->OnMessage(this, 288, 1, 0);
        m_mode = 1;
        ShowList();
        break;
    case 281:
        GetParent()->OnMessage(this, 288, 1, 0);
        m_mode = 2;
        ShowList();
        break;
    case 260:
        if (w == m_quantity || w == m_mbox)
            w->SetVisible(false);
        app->SetActiveWindow(this);
        break;
    case 293:
        if (p1 <= 0)
        {
            app->PlaySound(13);
            app->SetActiveWindow(this);
        }
        else if (m_mode == 2)
            Buy(p2, p1);
        else if (m_mode == 1)
            Sell(p2, p1);
        m_quantity->SetVisible(false);
        break;
    case 268:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SellItem(p1, p2);
        }
        app->SetActiveWindow(this);
        break;
    case 295:
        if (p2 == (int)this)
        {
            m_list->SetX84(1);
            if (p1 != (int)m_quantity)
            {
                m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(403, 180), 490, 15, 1);
                m_window.Open(true);
            }
        }
        else if (p1 == (int)this)
            m_list->SetX84(0);
        break;
    case 296:
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C783C8.
void VIWndMerchant::Buy(int item, int n)
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->MerchantRequest(item, n);
    app->SetActiveWindow(this);
}

// 0x00C78430.  Selling a worn (930), quick-slotted (931) or no-trade (932) item asks first.
void VIWndMerchant::Sell(int item, int n)
{
    int warn = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short name[128];
    unsigned short s[256];
    VIUI &ui = app->m_ui;
    VIString::strcpy16(s, sizeof(s), (const unsigned short *)ui.m_words.GetString(929, ui.m_language));
    app->GetInvItemName(item, name, sizeof(name));
    if (app->GetInvItemEquipSlot(item) != -1)
    {
        warn = 1;
        VIString::snprintf16(s, sizeof(s), (const unsigned short *)ui.m_words.GetString(930, ui.m_language), name);
    }
    else if (app->GetInvItemQuickSlot(item) >= 0)
    {
        warn = 1;
        VIString::snprintf16(s, sizeof(s), (const unsigned short *)ui.m_words.GetString(931, ui.m_language), name);
    }
    else if (app->GetInvItemNoTrade(item) > 0)
    {
        warn = 1;
        VIString::snprintf16(s, sizeof(s), (const unsigned short *)ui.m_words.GetString(932, ui.m_language));
    }
    if (warn)
    {
        m_mbox->SetText(s);
        m_mbox->SetParam1(item);
        m_mbox->SetParam2(n);
        m_mbox->SetVisible(true);
        app->SetActiveWindow(m_mbox);
    }
    else
    {
        app->SellItem(item, n);
        app->SetActiveWindow(this);
    }
}

// 0x00C785D0.  Back to the buy/sell menu.
void VIWndMerchant::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_mode = 0;
    m_list->SetX64(1);
    m_list->ResetScroll();
    m_list->SetVisible(false);
    m_quantity->SetVisible(false);
    m_mbox->SetVisible(false);
    m_menu->Refresh();
    m_menu->ResetSel();
    m_menu->SetVisible(true);
    app->SetActiveWindow(m_menu);
}

// 0x00C78670.
void VIWndMerchant::ShowList()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_list->SetX64(1);
    m_list->ResetScroll();
    m_list->SetVisible(true);
    m_quantity->SetVisible(false);
    m_mbox->SetVisible(false);
    m_menu->ResetSel();
    m_menu->SetVisible(false);
    app->SetActiveWindow(this);
}

// 0x00C78700.  A stack shows its count; a worn item (138) or one the player cannot use
// (140) gets an arrow.
void VIWndMerchant::FillInvList()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    DrawInfo();
    for (; i < app->GetInvCount(); i++)
    {
        unsigned int flags = 1;
        if (app->GetInvItemAmount(i) >= 2)
            flags = 5;
        int arrow = -1;
        if (app->GetInvItemEquipSlot(i) != -1)
        {
            arrow = 138;
            flags |= 8;
        }
        if (app->GetInvItemWearSlot(i) != -1 && !app->CanUseListItem(i, 0))
        {
            arrow = 140;
            flags |= 8;
        }
        int icon = app->GetInvItemIcon(i);
        m_list->SetItem(i, flags, icon, arrow, -1, app->GetInvItemAmount(i), -1);
    }
    for (; i < 40; i++)
        m_list->SetItem(i, 0, -1, -1, -1, 0, -1);
}

// 0x00C78870.
void VIWndMerchant::FillMerchantList()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    DrawInfo();
    for (; i < app->GetMerchantCount(); i++)
    {
        unsigned int flags = 1;
        int arrow = -1;
        if (app->GetMerchantItemWearSlot(i) != -1 && !app->CanUseListItem(i, 1))
        {
            arrow = 140;
            flags |= 8;
        }
        m_list->SetItem(i, flags, app->GetMerchantItemIcon(i), arrow, -1, 1, -1);
    }
    for (; i < 40; i++)
        m_list->SetItem(i, 0, -1, -1, -1, 0, -1);
}

// 0x00C78990.  The item text, its description, the price and the coin, the stat lines,
// and the Buy/Sell banner.
void VIWndMerchant::DrawInfo()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    unsigned short s[32];
    ui.Begin2D();
    ui.m_font.SetTextColor(sBlack);
    m_window.TextBox(m_stats, 20, 0, 270, 11, (VIUICOLOR)0, 2);
    m_window.TextBox(m_desc, 20, 12, 445, 3, (VIUICOLOR)0, 2);
    if (m_price != -1)
    {
        VIString::snprintf16(s, sizeof(s), "%S:", (const unsigned short *)ui.m_words.GetString(505, ui.m_language));
        m_window.Text(s, 310, 0, (VIUICOLOR)0, 0);
        VIString::snprintf16(s, sizeof(s), "%d %S", m_price,
                             (const unsigned short *)ui.m_words.GetString(479, ui.m_language));
        m_window.TextBox(s, 310, 1, 160, 1, (VIUICOLOR)0, 2);
    }
    VIString::snprintf16(s, sizeof(s), "%S:", (const unsigned short *)ui.m_words.GetString(506, ui.m_language));
    m_window.Text(s, 310, m_price != -1 ? 2 : 0, (VIUICOLOR)0, 0);
    VIString::snprintf16(s, sizeof(s), "%d %S", m_coin,
                         (const unsigned short *)ui.m_words.GetString(479, ui.m_language));
    m_window.TextBox(s, 310, m_price == -1 ? 1 : 3, 160, 1, (VIUICOLOR)0, 2);
    int row = 4;
    if (m_price == -1)
        row = 2;
    for (int k = 0; k < 6 && row < 11; k++)
    {
        if (m_lines[k].m_text[0])
        {
            int n = m_window.CountLines(m_lines[k].m_text, 160);
            if (row == 10)
                n = n < 2 ? n : 1;
            m_window.TextBox(m_lines[k].m_text, 310, row, 160, n, m_lines[k].m_color, 2);
            row += n;
        }
    }
    if (m_mode == 2)
    {
        int w = ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                                         (const unsigned short *)ui.m_words.GetString(63, ui.m_language)) + 64;
        ui.BlitBar((unsigned short)63, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), VIVect2I((640 - w) / 2, 8),
                   (VIUITexId)33, (VIUITexId)34, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    }
    else if (m_mode == 1)
    {
        int w = ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                                         (const unsigned short *)ui.m_words.GetString(62, ui.m_language)) + 64;
        ui.BlitBar((unsigned short)62, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), VIVect2I((640 - w) / 2, 8),
                   (VIUITexId)33, (VIUITexId)34, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    }
    ui.m_font.SetTextColor(sWhite);
    ui.End2D();
}

// 0x00C78DC0.
void VIWndMerchant::ClearInfo()
{
    m_stats[0] = 0;
    m_desc[0] = 0;
    for (int k = 0; k < 6; k++)
    {
        m_lines[k].m_text[0] = 0;
        m_lines[k].m_color = VIColor32F(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

// 0x00C78E28.  The selected row's text, description and price; for something wearable,
// each of its stats as it would change the player's.
void VIWndMerchant::UpdateInfo()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int sel;
    VIArray<VIItemStat> *stats;
    VIUI *ui = &app->m_ui;
    unsigned short s[256];
    unsigned short desc[512];
    ClearInfo();
    if (m_mode == 1 && !app->GetInvCount() || m_mode == 2 && !app->GetMerchantCount() || !m_mode)
    {
        SetPrice(-1);
        return;
    }
    sel = m_list->GetSel();
    if (m_mode == 1 && sel >= app->GetInvCount())
        return;
    if (m_mode == 2 && sel >= app->GetMerchantCount())
        return;
    if (m_mode == 1)
        m_info.DescribeListItem(m_stats, sizeof(m_stats), sel, 0);
    else if (m_mode == 2)
        m_info.DescribeListItem(m_stats, sizeof(m_stats), sel, 1);
    if (m_mode == 1)
    {
        app->GetInvItemDesc(sel, s, sizeof(s));
        VIString::strcpy16(desc, sizeof(desc), s);
    }
    else if (m_mode == 2)
    {
        app->GetMerchantItemDesc(sel, s, sizeof(s));
        VIString::strcpy16(desc, sizeof(desc), s);
    }
    VIString::strcpy16(m_desc, sizeof(m_desc), desc);
    int price;
    if (m_mode == 1)
        price = app->GetInvItemSellPrice(sel);
    else if (m_mode == 2)
        price = app->GetMerchantPrice(sel);
    SetPrice(price);
    int wearable = 1;
    if (m_mode == 1)
    {
        if (app->GetInvItemWearSlot(sel) == -1)
            wearable = 0;
        stats = app->GetInvItemStats(sel);
    }
    else if (m_mode == 2)
    {
        if (app->GetMerchantItemWearSlot(sel) == -1)
            wearable = 0;
        stats = app->GetMerchantItemStats(sel);
    }
    else
        return;
    if (!wearable)
        return;
    desc[0] = 0;
    int k = 0;
    for (int it = stats->Begin(); it != stats->End() && k < 6; it = stats->Next(it), k++)
    {
        VIItemStat *st = &stats->m_data[it];
        VIString::snprintf16(s, sizeof(s), "%d: %d ", st->m_id, st->m_value);
        int merchant = m_mode == 2;
        int now = app->GetStatTotal(st->m_id);
        int then = app->StatWithItem(sel, st->m_id, merchant);
        VIWordId *name = FindStatWord(st->m_id);
        if (name)
            VIString::snprintf16(m_lines[k].m_text, sizeof(m_lines[k].m_text), "%S: %d |} %d",
                                 (const unsigned short *)ui->m_words.GetString(name->m_word, ui->m_language),
                                 st->m_value, then);
        else
            VIString::strcpy16(m_lines[k].m_text, sizeof(m_lines[k].m_text), s);

        if (now < then)
            m_lines[k].m_color = VIColor32F(0.0392156877f, 0.196078435f, 0.274509817f, 1.0f);
        else if (then < now)
            m_lines[k].m_color = VIColor32F(0.372549027f, 0.0980392173f, 0.0980392173f, 1.0f);
        else
            m_lines[k].m_color = sBlack;
    }
}

// 0x00C79318.
void VIWndMerchant::SetPrice(int price)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_price = price;
    m_coin = app->m_player.m_p34;
}
