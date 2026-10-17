#line 1 "C:/code-frontiers-gm/game/VIWndAuction.cpp"
// viwndauction.cpp (name ours) -- CLIENT 0x00C0D038 .. 0x00C0F9F0: the auction window.
// The bank's items in a list; picking one opens a sell form (minimum bid, buyout,
// duration, Ok) whose numbers are typed into a VIWndEdit and whose Ok is confirmed in a
// VIWndMBox before VIBankControl sends it.  Method names are ours.
#include "game.h"

// 0x00C0D038.
VIWndAuction::VIWndAuction()
{
    m_inv = 0;
    m_bank = 0;
    m_edit = 0;
    m_mbox = 0;
    m_selling = 0;
}

// 0x00C0D098.
VIWndAuction::~VIWndAuction()
{
    Free();
}

// 0x00C0D108.  The two lists, the editor and the message box, all children.
int VIWndAuction::Init()
{
    m_inv = new VIWndList;
    if (!m_inv)
        return -1;
    m_inv->SetAppPtr(GetAppPtr());
    m_inv->SetX64(0);
    AddChild(m_inv);
    m_bank = new VIWndList;
    if (!m_bank)
        return -1;
    m_bank->SetAppPtr(GetAppPtr());
    m_bank->SetX64(1);
    AddChild(m_bank);
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->SetVisible(false);
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    m_inv->Init(40);
    m_bank->Init(80);
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
    m_field = 0;
    m_selling = 0;
    m_info.SetClient((VIClient *)GetAppPtr());
    return 0;
}

// 0x00C0D2C0.
int VIWndAuction::Free()
{
    if (m_inv)
    {
        RemoveChild(m_inv);
        delete m_inv;
        m_inv = 0;
    }
    if (m_bank)
    {
        RemoveChild(m_bank);
        delete m_bank;
        m_bank = 0;
    }
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    return 0;
}

// 0x00C0D3C8.
void VIWndAuction::OnDrawSelf()
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    FillLists();
    m_mbox->SetPos(VIVect2I(175, 115));
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    DrawLists();
    if (m_selling == 1)
        DrawSell();
    ui->End2D();
}

// 0x00C0D490.  Both lists, row by row: the inventory's 40 and the bank's 80 cleared, then
// filled.  A stack shows its count (flag 4); what cannot be sold or worn is greyed (8).
void VIWndAuction::FillLists()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIBankControl *bank = app->GetBank();
    int i;
    for (i = 0; i < 40; i++)
        m_inv->SetItem(i, 0, -1, -1, -1, 0, -1);
    for (i = 0; i < 80; i++)
        m_bank->SetItem(i, 0, -1, -1, -1, 0, -1);
    for (i = 0; i < app->GetInvCount(); i++)
    {
        int flags = 1;
        int color = -1;
        if (app->GetInvItemAmount(i) >= 2)
            flags = 5;
        if (app->GetInvItemEquipSlot(i) != -1)
        {
            color = 138;
            flags |= 8;
        }
        if (app->GetInvItemWearSlot(i) != -1 && !app->CanUseListItem(i, 0))
        {
            color = 140;
            flags |= 8;
        }
        m_inv->SetItem(i, flags, app->GetInvItemIcon(i), color, -1, app->GetInvItemAmount(i), -1);
    }
    for (i = 0; i < bank->Count(); i++)
    {
        int flags = 1;
        int color = -1;
        VINetItem *item = bank->Item(i);
        if (item->Quantity >= 2)
            flags = 5;
        if (!CanSell(item))
        {
            color = 140;
            flags |= 8;
        }
        m_bank->SetItem(i, flags, app->ItemIcon(item), color, -1, item->Quantity, -1);
    }
}

// 0x00C0D6E8.  The title bar, the frame, the selected item's description and the bank list.
void VIWndAuction::DrawLists()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIUIWindow &win = m_window;
    const unsigned short *title = win.T(1721);
    int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, title) + 64;
    ui->BlitBar(win.T(1721), VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), VIVect2I((640 - w) / 2, 8),
                (VIUITexId)33, (VIUITexId)34, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    VIUIFrameParam fp;
    memset(&fp, 0, 64);
    fp.m_bottom.m_id = (VIUITexId)49;
    fp.m_left.m_id = (VIUITexId)47;
    fp.m_br.m_id = (VIUITexId)50;
    fp.m_right.m_id = (VIUITexId)52;
    fp.m_top.m_id = (VIUITexId)45;
    fp.m_tl.m_id = (VIUITexId)44;
    fp.m_tr.m_id = (VIUITexId)46;
    fp.m_bl.m_id = (VIUITexId)48;
    ui->BlitFillFrame(VIVect2I(165, 48), VIVect2I(310, 267), fp, (VIUITexId)35, (VIAlphaBlendMode)0, 1.0f, 1);
    ui->BlitStuddedHorz(VIVect2I(165, 37), (VIUITexId)37, (VIUITexId)38, 310, 2, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(165, 313), (VIUITexId)40, (VIUITexId)41, 310, 2, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitHorz(VIVect2I(16, 322), (VIUITexId)60, 608, (VIAlphaBlendMode)0, 1.0f, 0);
    unsigned short buf[512];
    m_info.DescribeBankListItem(buf, 1024, m_bank->m_sel, 2);
    win.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(390, 180), 470, 14, 0);
    win.SetTitle("");
    win.Draw();
    win.TextBox(buf, 20, 0, win.m_size.x - 40, 14, (VIUICOLOR)0, 2);
    m_inv->m_pos = VIVect2I(475, 37);
    m_inv->SetVisible(false);
    m_bank->m_pos = VIVect2I(16, 37);
    m_bank->SetVisible(false);
    m_bank->SetX84(m_selling != 1);
    m_bank->OnDraw();
}

// 0x00C0DA48.  The sell form over the list: the item, minimum bid, buyout, duration, Ok,
// and the fee line.  The numbers are clamped as they are drawn.
// @@BEGIN DrawSell
void VIWndAuction::DrawSell()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIBankControl *bank = app->GetBank();
    if (bank->Count() == 0)
        return;
    m_sell.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 475, 9, 0);
    m_sell.SetTitle(m_sell.T(1722));
    m_sell.Draw();
    int cols[3];
    cols[0] = 40;
    cols[1] = 180;
    cols[2] = 240;
    VINetItem *item = bank->Item(m_bank->m_sel);
    int p = m_price;
    if (p < item->m_value)
        p = item->m_value;
    m_price = p > 999999999 ? 999999999 : p;
    int b = m_buyout;
    if (b < m_price)
        b = m_price;
    if (b > 999999999)
        b = 999999999;
    m_buyout = b;
    if (!InMinutes())
        m_duration = VIMIN(VIMAX(m_duration, 720), 4320);
    else
        m_duration = VIMIN(VIMAX(m_duration, 2), 4320);
    unsigned short buf[128];
    buf[0] = 0;
    if (item)
        VIString::strcpy16(buf, 256, item->m_name);
    DrawRow(&m_sell, 1, m_sell.T(1723), cols[0], buf, cols[1], 0, cols[2], m_field == 0, 1);
    VIString::snprintf16(buf, 256, "%d", m_price);
    DrawRow(&m_sell, 2, m_sell.T(1685), cols[0], buf, cols[1], 0, cols[2], m_field == 1, 1);
    VIString::snprintf16(buf, 256, "%d", m_buyout);
    DrawRow(&m_sell, 3, m_sell.T(1686), cols[0], buf, cols[1], 0, cols[2], m_field == 2, 1);
    unsigned short unit[16];
    VIString::snprintf16(unit, 32, (const unsigned short *)(InMinutes() ? ui->m_words.GetString(1724, ui->m_language)
                                                                         : ui->m_words.GetString(1725, ui->m_language)));
    VIString::snprintf16(buf, 256, "%d", InMinutes() ? m_duration : m_duration / 60);
    DrawRow(&m_sell, 4, m_sell.T(1726), cols[0], buf, cols[1], unit, cols[2], m_field == 3, 1);
    VIString::snprintf16(buf, 256, "%S*", ui->m_words.GetString(218, ui->m_language));
    DrawRow(&m_sell, 5, buf, cols[0], 0, cols[1], 0, cols[2], m_field == 4, 1);
    int fee = VIMAX(1, m_price / 100);
    VIString::snprintf16(buf, 256, "*%S: %d %S", m_sell.T(1788), fee,
                         ui->m_words.GetString(479, ui->m_language));
    DrawRow(&m_sell, 7, buf, cols[0], 0, m_sell.Width() - 30 - cols[0], 0, cols[2], 0, 0);
    VIString::snprintf16(buf, 256, "|x%S   |t%S", ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(161, ui->m_language));
    m_sell.TextBox(buf, 20, 8, m_sell.Width() - 40, 1, (VIUICOLOR)0, 0);
}
// @@END DrawSell

// 0x00C0DF68.  One row of a form: a label, a framed value and a unit, the row ticked
// when selected.
void VIWndAuction::DrawRow(VIUIWindow *w, int row, const unsigned short *a, int xa,
                           const unsigned short *b, int xb, const unsigned short *c, int xc,
                           int sel, int tick)
{
    if (a)
        w->TextBox(a, xa, row, xb - xa, 1, (VIUICOLOR)(sel != 0), 0);
    if (b)
    {
        int width;
        if (c)
            width = xc - xb - 12;
        else
            width = w->m_size.x - xb - 30;
        w->DrawFrame(xb, row, width, 1, 64);
        w->TextBox(b, xb, row, width, 1, (VIUICOLOR)(sel != 0), 2);
    }
    if (c)
        w->TextBox(c, xc, row, w->m_size.x - xb - 30, 1, (VIUICOLOR)(sel != 0), 0);
    if (sel && tick)
        w->TickR(xa, row, 1);
}

// 0x00C0E0C0.
void VIWndAuction::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_selling == 0)
    {
        switch (key)
        {
        case 13:
        case 134:
            Select();
            break;
        case 19:
        case 138:
            m_bank->OnKeyDown(138);
            break;
        case 20:
        case 139:
            m_bank->OnKeyDown(139);
            break;
        case 27:
        case 132:
            m_parent->OnMessage(this, 260, 0, 0);
            break;
        }
    }
    else if (m_selling == 1)
    {
        switch (key)
        {
        case 13:
        case 134:
            Accept();
            break;
        case 19:
        case 138:
            Prev();
            break;
        case 20:
        case 139:
            Next();
            break;
        case 27:
        case 132:
            app->PlaySound(8);
            m_selling = 0;
            break;
        }
    }
}

// 0x00C0E2C8.
void VIWndAuction::OnKeyUp(int key)
{
    if (m_selling == 0)
    {
        switch (key)
        {
        case 19:
        case 138:
            m_bank->OnKeyUp(138);
            break;
        case 20:
        case 139:
            m_bank->OnKeyUp(139);
            break;
        default:
            VIWnd::OnKeyUp(key);
            break;
        }
    }
    else
        VIWnd::OnKeyUp(key);
}

// 0x00C0E370.  259/260 are the editor's done/cancel, 268 the message box's Ok; 295 is
// this window becoming active from outside, which resets it.
// @@BEGIN OnMessage
void VIWndAuction::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != this && (VIWnd *)p1 != m_bank &&
            (VIWnd *)p1 != m_inv && (VIWnd *)p1 != m_edit && (VIWnd *)p1 != m_mbox)
        {
            m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 330, 14, 0);
            m_window.Open(1);
            m_sell.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 600, 9, 0);
            m_sell.Close(1);
            m_selling = 0;
            m_field = 0;
            m_edit->SetVisible(false);
            m_mbox->SetVisible(false);
        }
        break;
    case 260:
        if (w == m_edit)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        else if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 268:
        if (w == m_mbox)
            Confirm(p1, p2);
        break;
    case 259:
        TakeText();
        break;
    }
}
// @@END OnMessage

// 0x00C0E550.  Open the sell form for the selected bank item, if it can be sold.
void VIWndAuction::Select()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIBankControl *bank = app->GetBank();
    if (bank->Count() == 0)
        return;
    VINetItem *item = bank->Item(m_bank->m_sel);
    if (CanSell(item))
    {
        app->PlaySound(13);
        m_price = item->m_value;
        m_buyout = m_price * 2;
        m_duration = InMinutes() ? 2 : 720;
        m_field = 1;
        m_selling = 1;
        m_sell.Close(1);
        m_sell.Open(0);
    }
    else
    {
        app->AddText(m_window.T(1727), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
        app->PlaySound(9);
    }
}

// 0x00C0E688.
void VIWndAuction::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_field >= 4)
    {
        m_field = 4;
        app->PlaySound(9);
    }
    else
    {
        m_field++;
        app->PlaySound(11);
    }
    m_sell.ResetTickPulse();
}

// 0x00C0E6F0.
void VIWndAuction::Prev()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_field < 2)
    {
        m_field = 1;
        app->PlaySound(9);
    }
    else
    {
        m_field--;
        app->PlaySound(11);
    }
    m_sell.ResetTickPulse();
}

// 0x00C0E758.  Ok on the form's last row asks for confirmation; any other row is edited.
void VIWndAuction::Accept()
{
    int sound = 13;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIBankControl *bank = app->GetBank();
    if (m_field == 4)
    {
        VINetItem *item = bank->Item(m_bank->m_sel);
        if (CanSell(item))
        {
            unsigned short buf[256];
            VIString::snprintf16(buf, 512, "%S: %S\n%S: %d %S\n%S: %d %S\n%S",
                                 ui->m_words.GetString(1723, ui->m_language), item->m_name,
                                 ui->m_words.GetString(1789, ui->m_language), m_price,
                                 ui->m_words.GetString(479, ui->m_language),
                                 ui->m_words.GetString(1726, ui->m_language),
                                 InMinutes() ? m_duration : m_duration / 60,
                                 ui->m_words.GetString(InMinutes() ? 1724 : 1725, ui->m_language),
                                 ui->m_words.GetString(1790, ui->m_language));
            ShowMessage(buf, 0, 0);
        }
        else
            sound = 9;
    }
    else
        EditField();
    app->PlaySound(sound);
}

// 0x00C0E940.  The editor over the selected row: minimum bid, buyout or duration.
void VIWndAuction::EditField()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    m_edit->SetRows(1);
    m_edit->SetMultiLine(0);
    m_edit->SetCenter(VIVect2I(320, 180));
    m_edit->SetMaxLength(128);
    m_edit->SetVisible(true);
    m_mbox->SetVisible(false);
    switch (m_field)
    {
    case 1:
        m_edit->SetMaxLength(10);
        m_edit->SetTitle(m_window.T(1715));
        break;
    case 2:
        m_edit->SetMaxLength(10);
        m_edit->SetTitle(m_window.T(1716));
        break;
    case 3:
        {
            unsigned short title[32];
            VIString::snprintf16(title, 64, "%S %S", ui->m_words.GetString(1726, ui->m_language),
                                 InMinutes() ? ui->m_words.GetString(1687, ui->m_language)
                                             : ui->m_words.GetString(1691, ui->m_language));
            m_edit->SetMaxLength(5);
            m_edit->SetTitle(title);
        }
        break;
    default:
        return;
    }
    app->SetActiveWindow(m_edit);
}

// 0x00C0EB00.  The editor's text into the row it edited.
void VIWndAuction::TakeText()
{
    char buf[128];
    VIClient *app = (VIClient *)GetAppPtr();
    VIString::str16to8(buf, 128, m_edit->GetText());
    switch (m_field)
    {
    case 1:
        m_price = atoi(buf);
        break;
    case 2:
        m_buyout = atoi(buf);
        break;
    case 3:
        if (InMinutes())
            m_duration = atoi(buf);
        else
            m_duration = atoi(buf) * 60;
        break;
    }
    m_edit->SetVisible(false);
    app->SetActiveWindow(this);
}

// 0x00C0EBE8.
int VIWndAuction::CanSell(VINetItem *item)
{
    if (!item || item->m_noTrade || item->m_lore)
        return 0;
    if (!item->m_rent)
        return 0;
    return item->m_maxStack < 2;
}

// 0x00C0EC30.
int VIWndAuction::InMinutes()
{
    return ((VIClient *)GetAppPtr())->GetP382E8()->IsEnabled() > 0;
}

// 0x00C0EC70.
void VIWndAuction::ShowMessage(const unsigned short *text, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_edit->SetVisible(false);
    m_mbox->Init(1, 1);
    m_mbox->SetText(text);
    m_mbox->SetParam1(p1);
    m_mbox->SetParam2(p2);
    m_mbox->SetIcon(-1);
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}

// 0x00C0ED20.  The message box's Ok: put the item up.
void VIWndAuction::Confirm(int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIBankControl *bank = app->GetBank();
    if (CanSell(bank->Item(m_bank->m_sel)))
    {
        bank->Sell(m_bank->m_sel, m_price, m_buyout, m_duration);
        m_selling = 0;
    }
    m_mbox->SetVisible(false);
    app->SetActiveWindow(this);
}
