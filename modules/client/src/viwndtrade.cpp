#line 1 "C:/code-frontiers-gm/game/VIWndTrade.cpp"
// VIWndTrade -- CLIENT 0x00C9A878 .. 0x00C9C97C (the TU runs to its __tf at 0x00C9D558).
// The trade window: the inventory (a VIWndList), both offers (VIWndTradeLists), both
// money amounts, the description of the item under the cursor, and the accept glow.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// A string's width in the UI font.
static inline int TextWidth(VIUI *ui, const unsigned short *s)
{
    return ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, s);
}

// 0x00C9A878.
VIWndTrade::VIWndTrade()
{
    m_focus = -1;
    m_fadeDir = 1;
    m_fade = 1.0f;
    m_list = 0;
    m_mine = 0;
    m_theirs = 0;
    m_mbox = 0;
    m_deposit = 0;
    m_quantity = 0;
    m_edit = 0;
    m_iAccepted = 0;
    m_theyAccepted = 0;
    m_time = 0;
}

// 0x00C9A908.
VIWndTrade::~VIWndTrade()
{
    Free();
}

// 0x00C9A978.
int VIWndTrade::Init()
{
    int r;
    m_list = new VIWndList;
    if (!m_list)
        goto fail;
    m_list->SetVisible(false);
    m_list->SetAppPtr(GetAppPtr());
    m_list->SetPos(VIVect2I(6, 37));
    AddChild(m_list);
    m_list->Init(40);
    m_mine = new VIWndTradeList;
    if (!m_mine)
        goto fail;
    m_mine->SetVisible(false);
    m_mine->SetAppPtr(GetAppPtr());
    m_mine->SetPos(VIVect2I(155, 37));
    AddChild(m_mine);
    m_mine->SetX64(0);
    m_mine->SetTitle(474);
    m_mine->VIWndList::Init(4);
    m_mine->SetRows(3);
    m_theirs = new VIWndTradeList;
    if (!m_theirs)
        goto fail;
    m_theirs->SetVisible(false);
    m_theirs->SetAppPtr(GetAppPtr());
    m_theirs->SetPos(VIVect2I(295, 37));
    AddChild(m_theirs);
    m_theirs->SetX64(0);
    m_theirs->SetTitle(475);
    m_theirs->VIWndList::Init(4);
    m_theirs->SetRows(3);
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        goto fail;
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    if (m_mbox->Init(1, 1))
        goto fail;
    m_deposit = new VIWndDeposit;
    if (!m_deposit)
        goto fail;
    m_deposit->SetAppPtr(GetAppPtr());
    m_deposit->SetVisible(false);
    AddChild(m_deposit);
    if (m_deposit->Init())
        goto fail;
    m_quantity = new VIWndQuantity;
    if (!m_quantity)
        goto fail;
    m_quantity->SetAppPtr(GetAppPtr());
    AddChild(m_quantity);
    r = m_quantity->Init();
    if (r)
    {
    fail:
        r = -1;
        goto out;
    }
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->SetVisible(false);
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    m_iAccepted = 0;
    m_theyAccepted = 0;
    m_time = 0;
    m_focus = -1;
    m_fadeDir = 1;
    m_fade = 1.0f;
    m_info.SetClient((VIClient *)GetAppPtr());
out:
    return r;
}

// 0x00C9ACC0.
int VIWndTrade::Free()
{
    if (m_list)
    {
        RemoveChild(m_list);
        delete m_list;
        m_list = 0;
    }
    if (m_mine)
    {
        RemoveChild(m_mine);
        delete m_mine;
        m_mine = 0;
    }
    if (m_theirs)
    {
        RemoveChild(m_theirs);
        delete m_theirs;
        m_theirs = 0;
    }
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    if (m_deposit)
    {
        RemoveChild(m_deposit);
        delete m_deposit;
        m_deposit = 0;
    }
    if (m_quantity)
    {
        RemoveChild(m_quantity);
        delete m_quantity;
        m_quantity = 0;
    }
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    m_focus = -1;
    m_fade = 1.0f;
    m_fadeDir = 1;
    m_theyAccepted = 0;
    m_iAccepted = 0;
    return 0;
}

// 0x00C9AE90.
void VIWndTrade::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    if (m_focus == -1)
        return;
    unsigned short buf[512];
    m_list->SetVisible(false);
    m_mine->SetVisible(false);
    m_theirs->SetVisible(false);
    m_mbox->SetPos(VIVect2I(180, 115));
    m_list->Nothing(VIVect2I(180, 115));
    RefreshLists();
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(395, 183), 480, 15, 0);
    m_window.Open(true);
    int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 473)) + 64;
    ui->BlitBar((unsigned short)473, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), VIVect2I((640 - w) / 2, 8),
                (VIUITexId)33, (VIUITexId)34, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    m_offer.Init(UI(), VIVect2I(0, 0), VIVect2I(554, 162), 240, 14, 1);
    m_offer.Open(true);
    m_offer.Draw();
    m_offer.Init(UI(), VIVect2I(0, 0), VIVect2I(300, 255), 310, 3, 1);
    m_offer.Open(true);
    m_offer.Draw();
    VIString::snprintf16(buf, sizeof(buf), "%d", app->GetTradeP2BAF0(1));
    m_window.TextBox(m_window.T((unsigned short)479), 30, 10, 105, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 1);
    m_window.TextBox(buf, 30, 11, 105, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 5);
    if (m_focus == 2)
        m_window.TickR(20, 11, app->m_p2BD40 == this);
    VIString::snprintf16(buf, sizeof(buf), "%d", app->GetTradeP2BAF0(0));
    m_window.TextBox(m_window.T((unsigned short)479), 170, 10, 105, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 1);
    m_window.TextBox(buf, 170, 11, 105, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 5);
    buf[0] = 0;
    switch (m_focus)
    {
    case 0:
        m_info.DescribeTradeItem(buf, sizeof(buf), m_list->m_sel, 4);
        break;
    case 1:
    case 2:
        m_info.DescribeTradeItem(buf, sizeof(buf), m_mine->m_sel, 5);
        break;
    case 3:
        m_info.DescribeTradeItem(buf, sizeof(buf), m_theirs->m_sel, 6);
        break;
    }
    m_window.TextBox(buf, 295, -1, 170, 14, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 3);
    VIString::snprintf16(buf, sizeof(buf), "|x%S  |s%S  |t%S  |(Chat", Word(ui, 212), Word(ui, 478),
                         Word(ui, 213));
    int w2 = TextWidth(ui, buf);
    ui->BlitBar("", VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), VIVect2I(165, 290), (VIUITexId)25,
                (VIUITexId)26, w2 + 64, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    m_window.TextBox(buf, 25, 14, w2 + 34, 1, (VIUICOLOR)0, 0);
    long now = app->m_p07568;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    m_mine->SetColor(m_iAccepted ? VIColor32F(0.5f, 1.0f, 0.5f, m_fade)
                                 : VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
    m_theirs->SetColor(m_theyAccepted ? VIColor32F(0.5f, 1.0f, 0.5f, m_fade)
                                      : VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
    m_list->SetX84(app->m_p2BD40 == this);
    m_mine->SetX84(app->m_p2BD40 == this);
    m_theirs->SetX84(app->m_p2BD40 == this);
    m_list->OnDrawSelf();
    m_mine->OnDrawSelf();
    m_theirs->OnDrawSelf();
    ui->End2D();
    if (m_fadeDir == -1)
    {
        m_fade -= dt * 2.0f;
        m_fade = m_fade < 0.0f ? 0.0f : m_fade;
        if (m_fade <= 0.0f)
            m_fadeDir = 1;
    }
    else
    {
        m_fade += dt * 3.0f;
        m_fade = m_fade < 1.0f ? m_fade : 1.0f;
        if (m_fade >= 1.0f)
            m_fadeDir = -1;
    }
}

// 0x00C9B698.
void VIWndTrade::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 19:
    case 138:
        Up(key);
        break;
    case 20:
    case 139:
        Down(key);
        break;
    case 17:
    case 136:
        Left(key);
        break;
    case 18:
    case 137:
        Right(key);
        break;
    case 13:
    case 134:
        Select(key);
        break;
    case 27:
    case 132:
        Cancel(key);
        break;
    case 81:
    case 113:
    case 133:
        Accept(key);
        break;
    case 32:
    case 147:
        Chat();
        break;
    }
    m_window.ResetTickPulse();
}

// 0x00C9B790.
void VIWndTrade::Up(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_focus)
    {
    case -1:
        break;
    case 0:
        m_list->OnKeyDown(key);
        break;
    case 1:
        m_mine->OnKeyDown(key);
        break;
    case 2:
        m_focus = 1;
        m_mine->SetX64(1);
        app->PlaySound(11);
        break;
    case 3:
        m_theirs->OnKeyDown(key);
        break;
    }
}

// 0x00C9B878.
void VIWndTrade::Down(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_focus)
    {
    case -1:
        break;
    case 0:
        m_list->OnKeyDown(key);
        break;
    case 1:
        if (m_mine->GetSel() >= app->GetTradeCount(1) - 1 || m_mine->Count() <= 0)
        {
            m_focus = 2;
            m_mine->SetX64(0);
            app->PlaySound(11);
        }
        else
            m_mine->OnKeyDown(key);
        break;
    case 2:
        app->PlaySound(9);
        break;
    case 3:
        m_theirs->OnKeyDown(key);
        break;
    }
}

// 0x00C9B9B0.
void VIWndTrade::Left(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_focus)
    {
    case -1:
        break;
    case 0:
        break;
    case 1:
    case 2:
        m_focus = 0;
        m_list->SetX64(1);
        m_mine->SetX64(0);
        app->PlaySound(11);
        break;
    case 3:
        app->PlaySound(11);
        m_focus = 1;
        m_theirs->SetX64(0);
        m_mine->SetX64(1);
        break;
    }
}

// 0x00C9BAA8.
void VIWndTrade::Right(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_focus)
    {
    case -1:
        break;
    case 3:
        break;
    case 0:
        m_focus = 1;
        m_list->SetX64(0);
        m_mine->ResetList();
        m_mine->SetX64(1);
        app->PlaySound(11);
        break;
    case 1:
    case 2:
        m_focus = 3;
        m_mine->SetX64(0);
        m_theirs->SetX64(1);
        m_theirs->ResetList();
        app->PlaySound(11);
        break;
    }
}

// 0x00C9BBC0.
void VIWndTrade::Select(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_focus)
    {
    case -1:
        break;
    case 3:
        break;
    case 0:
    {
        int i = m_list->m_sel;
        if (app->GetTradeInvItemMaxStack(i) && app->GetTradeInvItemAmount(i) >= 2)
            m_quantity->Open(app->GetTradeInvItemAmount(i), i, -1);
        else
            app->TradeSend3(i, 1);
        app->PlaySound(13);
        break;
    }
    case 1:
        app->TradeSend4(m_mine->m_sel);
        app->PlaySound(13);
        break;
    case 2:
        m_deposit->SetCarried(app->m_p2BAEC);
        m_deposit->SetBanked(app->GetTradeP2BAF0(1));
        m_deposit->ClearDigits();
        m_deposit->SetCursorLast();
        m_deposit->Setup(3);
        m_deposit->SetVisible(true);
        app->PlaySound(13);
        app->SetActiveWindow(m_deposit);
        break;
    }
}

// 0x00C9BD28.
void VIWndTrade::Cancel(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_focus)
    {
    case -1:
        app->PlaySound(8);
        app->TradeCancel();
        break;
    case 0:
    case 1:
    case 2:
    case 3:
        if (m_iAccepted)
        {
            m_iAccepted = 0;
            app->TradeSend7();
        }
        else
        {
            app->PlaySound(15);
            m_mbox->Init(1, 1);
            m_mbox->SetText((unsigned short)941);
            m_mbox->SetParam1(m_focus);
            m_mbox->SetVisible(true);
            app->SetActiveWindow(m_mbox);
        }
        break;
    }
}

// 0x00C9BE00.
void VIWndTrade::Accept(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_focus)
    {
    case -1:
        break;
    case 0:
    case 1:
    case 2:
    case 3:
        app->PlaySound(13);
        app->TradeSend6();
        m_iAccepted = 1;
        break;
    }
}

// 0x00C9BE70.
void VIWndTrade::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        UpReleased(key);
        break;
    case 20:
    case 139:
        DownReleased(key);
        break;
    default:
        VIWnd::OnKeyUp(key);
        break;
    }
}

// 0x00C9BEF0.
void VIWndTrade::UpReleased(int key)
{
    switch (m_focus)
    {
    case -1:
        break;
    case 2:
        break;
    case 0:
        m_list->OnKeyUp(key);
        break;
    case 1:
        m_mine->OnKeyUp(key);
        break;
    case 3:
        m_theirs->OnKeyUp(key);
        break;
    }
}

// 0x00C9BF78.
void VIWndTrade::DownReleased(int key)
{
    switch (m_focus)
    {
    case -1:
        break;
    case 2:
        break;
    case 0:
        m_list->OnKeyUp(key);
        break;
    case 1:
        m_mine->OnKeyUp(key);
        break;
    case 3:
        m_theirs->OnKeyUp(key);
        break;
    }
}

// 0x00C9C000.
void VIWndTrade::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(395, 183), 480, 15, 0);
            m_window.Open(true);
            m_time = app->m_p07568;
            m_fadeDir = 1;
            m_fade = 1.0f;
        }
        break;
    case 268:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            switch (p1)
            {
            case -1:
                app->TradeSetup();
                app->TradeSend2();
                Start();
                GetParent()->OnMessage(this, 288, 1, 0);
                break;
            case 0:
            case 1:
            case 2:
            case 3:
                app->TradeCancel();
                break;
            }
        }
        break;
    case 260:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            switch (p1)
            {
            case -1:
                app->TradeCancel();
                break;
            case 0:
            case 1:
            case 2:
            case 3:
                app->SetActiveWindow(this);
                break;
            default:
                app->SetActiveWindow(this);
                break;
            }
        }
        else if (w == m_deposit)
        {
            w->SetVisible(false);
            m_deposit->SetCarried(0);
            app->SetActiveWindow(this);
        }
        else if (w == m_quantity || w == m_edit)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 293:
        if (m_focus == 0 && p1 > 0)
            app->TradeSend3(p2, p1);
        m_quantity->SetVisible(false);
        app->SetActiveWindow(this);
        break;
    case 294:
        app->TradeSend5(p1);
        m_deposit->SetVisible(false);
        m_deposit->SetCarried(0);
        app->SetActiveWindow(this);
        break;
    case 259:
        SendChat();
        break;
    }
}

// 0x00C9C278.
void VIWndTrade::Ask()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    char name[64];
    unsigned short buf[128];
    app->TradeName(name);
    VIString::snprintf16(buf, sizeof(buf), "%s ", name);
    VIString::strcat16(buf, sizeof(buf), Word(ui, 472));
    m_quantity->SetVisible(false);
    m_list->SetVisible(false);
    m_mine->SetVisible(false);
    m_theirs->SetVisible(false);
    m_deposit->SetVisible(false);
    m_edit->SetVisible(false);
    m_mbox->SetPos(VIVect2I(80, 115));
    m_mbox->Init(1, 1);
    m_mbox->SetText(buf);
    m_mbox->SetParam1(-1);
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
    m_focus = -1;
}

// 0x00C9C3A8.
void VIWndTrade::Start()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_list->SetX64(1);
    m_list->ResetScroll();
    m_mine->SetX64(0);
    m_mine->ResetList();
    m_theirs->SetX64(0);
    m_theirs->ResetList();
    m_deposit->SetVisible(false);
    m_deposit->SetCarried(0);
    m_mbox->SetVisible(false);
    m_quantity->SetVisible(false);
    m_edit->SetVisible(false);
    app->SetActiveWindow(this);
    m_focus = 0;
    m_iAccepted = 0;
    m_theyAccepted = 0;
}

// 0x00C9C490.
void VIWndTrade::Hide()
{
    m_list->SetVisible(false);
    m_mine->SetVisible(false);
    m_theirs->SetVisible(false);
    m_deposit->SetVisible(false);
    m_mbox->SetVisible(false);
    m_edit->SetVisible(false);
    m_focus = 0;
}

// 0x00C9C4C8.
void VIWndTrade::TheyAccepted()
{
    m_theyAccepted = 1;
}

// 0x00C9C4D8.
void VIWndTrade::ClearAccepted()
{
    m_iAccepted = 0;
    m_theyAccepted = 0;
}

// 0x00C9C4E8.
void VIWndTrade::RefreshLists()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int i;
    unsigned int flags;
    int mark;
    for (i = 0; i < app->GetTradeInvCount(); i++)
    {
        flags = app->GetTradeInvItemAmount(i) >= 2 ? 5 : 1;
        mark = -1;
        if (app->GetTradeInvItemEquipSlot(i) != -1)
        {
            mark = 138;
            flags |= 8;
        }
        if (app->GetTradeInvItemWearSlot(i) != -1 && !app->CanUseListItem(i, 4))
        {
            mark = 140;
            flags |= 8;
        }
        m_list->SetItem(i, flags, app->GetTradeInvItemIcon(i), mark, -1,
                        app->GetTradeInvItemAmount(i), -1);
    }
    for (; i < 40; i++)
        m_list->SetItem(i, 0, -1, -1, -1, 0, -1);
    for (i = 0; i < app->GetTradeCount(1) && i < 4; i++)
    {
        flags = app->GetTradeOfferAmount(1, i) >= 2 ? 5 : 1;
        mark = -1;
        if (app->GetTradeOfferEquipSlot(1, i) != -1)
        {
            mark = 138;
            flags |= 8;
        }
        if (app->GetTradeOfferWearSlot(1, i) != -1 && !app->CanUseListItem(i, 5))
        {
            mark = 140;
            flags |= 8;
        }
        m_mine->SetItem(i, flags, app->GetTradeOfferIcon(1, i), mark, -1,
                        app->GetTradeOfferAmount(1, i), -1);
    }
    for (; i < 4; i++)
        m_mine->SetItem(i, 0, -1, -1, -1, 0, -1);
    for (i = 0; i < app->GetTradeCount(0) && i < 4; i++)
    {
        flags = app->GetTradeOfferAmount(0, i) >= 2 ? 5 : 1;
        mark = -1;
        if (app->GetTradeOfferEquipSlot(0, i) != -1)
        {
            mark = 138;
            flags |= 8;
        }
        if (app->GetTradeOfferWearSlot(0, i) != -1 && !app->CanUseListItem(i, 6))
        {
            mark = 140;
            flags |= 8;
        }
        m_theirs->SetItem(i, flags, app->GetTradeOfferIcon(0, i), mark, -1,
                          app->GetTradeOfferAmount(0, i), -1);
    }
    for (; i < 4; i++)
        m_theirs->SetItem(i, 0, -1, -1, -1, 0, -1);
}

// 0x00C9C8A0.
void VIWndTrade::Chat()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_edit->SetRows(1);
    m_edit->SetMultiLine(0);
    m_edit->SetCenter(VIVect2I(320, 180));
    m_edit->SetMaxLength(128);
    m_edit->SetVisible(true);
    app->SetActiveWindow(m_edit);
}

// 0x00C9C928.
void VIWndTrade::SendChat()
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->ChatLine(m_edit->m_text);
    m_edit->SetVisible(false);
    app->SetActiveWindow(this);
}
