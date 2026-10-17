#line 1 "C:/code-frontiers-gm/game/VIWndBankItem.cpp"
// viwndbankitem.cpp (name ours) -- CLIENT 0x00C16438 .. 0x00C18178: the bank's item page,
// the inventory and the bank side by side.  Method names are ours.
#include "game.h"

// 0x00C16438.
VIWndBankItem::VIWndBankItem()
{
    m_from = -1;
    m_inv = 0;
    m_bank = 0;
    m_qty = 0;
    m_text[0] = 0;
}

// 0x00C16498.
VIWndBankItem::~VIWndBankItem()
{
    Free();
}

// 0x00C164F8.
// @@BEGIN Init
int VIWndBankItem::Init()
{
    m_inv = new VIWndList;
    if (!m_inv)
        goto fail;
    m_inv->SetAppPtr(GetAppPtr());
    m_inv->SetPos(VIVect2I(16, 37));
    m_inv->SetX84(1);
    AddChild(m_inv);
    m_bank = new VIWndList;
    if (!m_bank)
        goto fail;
    m_bank->SetAppPtr(GetAppPtr());
    m_bank->SetPos(VIVect2I(475, 37));
    m_bank->SetX84(1);
    AddChild(m_bank);
    m_qty = new VIWndQuantity;
    if (!m_qty)
        goto fail;
    m_qty->SetAppPtr(GetAppPtr());
    AddChild(m_qty);
    int r;
    r = m_qty->Init();
    if (r)
    {
fail:
        r = -1;
    }
    else
    {
        VIVect2I(180, 90);
        m_inv->Init(40);
        m_bank->Init(40);
        m_from = -1;
        ClearText();
        m_info.SetClient((VIClient *)GetAppPtr());
    }
    return r;
}
// @@END Init

// 0x00C16690.
int VIWndBankItem::Free()
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
    if (m_qty)
    {
        RemoveChild(m_qty);
        delete m_qty;
        m_qty = 0;
    }
    ClearText();
    m_from = -1;
    return 0;
}

// 0x00C16770.
// @@BEGIN OnDrawSelf
void VIWndBankItem::OnDrawSelf()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIUI *ui = &app->m_ui;
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict,
                                      (const unsigned short *)ui->m_words.GetString(300, ui->m_language)) + 64;
    ui->BlitBar((unsigned short)300, white, VIVect2I((640 - w) / 2, 8), (VIUITexId)33, (VIUITexId)34, w,
                (VIAlphaBlendMode)0, 1.0f, 0, 1);
    VIUIFrameParam fp;
    VIUIFrameParam studs;
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
    memset(&fp, 0, 64);
    memset(&studs, 0, 64);
    fp.m_bottom.m_id = (VIUITexId)40;
    fp.m_left.m_id = (VIUITexId)42;
    fp.m_bl.m_id = (VIUITexId)39;
    fp.m_br.m_flags = 1;
    fp.m_br.m_id = (VIUITexId)39;
    fp.m_right.m_flags = 1;
    fp.m_top.m_id = (VIUITexId)37;
    fp.m_tl.m_id = (VIUITexId)36;
    fp.m_right.m_id = (VIUITexId)42;
    fp.m_tr.m_id = (VIUITexId)36;
    fp.m_tr.m_flags = 1;
    studs.m_bottom.m_id = (VIUITexId)41;
    studs.m_left.m_id = (VIUITexId)43;
    studs.m_right.m_id = (VIUITexId)43;
    studs.m_right.m_flags = 1;
    studs.m_top.m_id = (VIUITexId)38;
    ui->BlitStuddedHorz(VIVect2I(165, 37), (VIUITexId)37, (VIUITexId)38, 310, 2, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(165, 313), (VIUITexId)40, (VIUITexId)41, 310, 2, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitHorz(VIVect2I(16, 322), (VIUITexId)60, 608, (VIAlphaBlendMode)0, 1.0f, 0);
    UpdateText();
    m_window.TextBox(m_text, 0, 0, 280, 14, black, 2);
    ui->End2D();
    for (; i < app->GetInvCount(); i++)
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
    for (; i < 40; i++)
        m_inv->SetItem(i, 0, -1, -1, -1, 0, -1);
    VIBankControl *bank = app->GetBank();
    for (i = 0; i < bank->Count(); i++)
    {
        int flags = 1;
        int color = -1;
        VINetItem *item = bank->Item(i);
        if (item->Quantity >= 2)
            flags = 5;
        if (item->m_wearSlot != -1 && !app->CanUseListItem(i, 2))
        {
            color = 140;
            flags |= 8;
        }
        m_bank->SetItem(i, flags, app->ItemIcon(item), color, -1, item->Quantity, -1);
    }
    for (; i < 40; i++)
        m_bank->SetItem(i, 0, -1, -1, -1, 0, -1);
}
// @@END OnDrawSelf

// 0x00C16CC8.
void VIWndBankItem::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        m_parent->OnMessage(this, 260, 0, 0);
        break;
    case 17:
    case 136:
        if (m_bank->GetX64())
            app->PlaySound(11);
        else
            app->PlaySound(9);
        m_inv->SetX64(1);
        m_bank->SetX64(0);
        break;
    case 18:
    case 137:
        if (m_inv->GetX64())
            app->PlaySound(11);
        else
            app->PlaySound(9);
        m_inv->SetX64(0);
        m_bank->SetX64(1);
        break;
    case 19:
    case 20:
    case 138:
    case 139:
        if (m_inv->GetX64())
            m_inv->OnKeyDown(key);
        else if (m_bank->GetX64())
            m_bank->OnKeyDown(key);
        break;
    case 13:
    case 134:
        if (m_inv->GetX64())
            m_inv->OnKeyDown(key);
        else if (m_bank->GetX64())
            m_bank->OnKeyDown(key);
        break;
    }
}

// 0x00C16F70.
void VIWndBankItem::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 20:
    case 138:
    case 139:
        if (m_inv->GetX64())
            m_inv->OnKeyUp(key);
        else if (m_bank->GetX64())
            m_bank->OnKeyUp(key);
        break;
    default:
        VIWnd::OnKeyUp(key);
        break;
    }
}

// 0x00C17038.  256 from a list (p1 the row), 293 from the quantity (p1 the count, p2 the row).
void VIWndBankItem::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    char buf[512];
    VIClient *app = (VIClient *)GetAppPtr();
    VIBankControl *bank = app->GetBank();
    VIMemStream s(buf, 512);
    switch (msg)
    {
    case 256:
        if (!bank->IsIdle())
        {
            app->PlaySound(9);
            break;
        }
        app->PlaySound(13);
        m_from = -1;
        if (w == m_bank)
        {
            if (p1 < 0 || p1 >= bank->Count())
                break;
            VINetItem *item = bank->Item(p1);
            if (item->m_maxStack && item->Quantity >= 2)
            {
                m_from = 1;
                m_qty->Open(item->Quantity, p1, -1);
            }
            else
                bank->Move(1, p1, 1);
        }
        else
        {
            if (p1 < 0 || p1 >= app->GetInvCount())
                break;
            if (app->GetInvItemMaxStack(p1) && app->GetInvItemAmount(p1) >= 2)
            {
                m_from = 0;
                m_qty->Open(app->GetInvItemAmount(p1), p1, -1);
            }
            else
                bank->Move(0, p1, 1);
        }
        break;
    case 296:
        if ((w == m_bank && ((VIWndList *)w)->GetX64()) || (w == m_inv && ((VIWndList *)w)->GetX64()))
            ;
        break;
    case 260:
        if (w == m_qty)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 293:
        if (m_from != -1 && p1 > 0)
            bank->Move((unsigned char)m_from, p2, p1);
        m_qty->SetVisible(false);
        app->SetActiveWindow(this);
        break;
    case 295:
        if ((VIWnd *)p2 == this)
        {
            m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(323, 188), 290, 15, 1);
            m_window.Open(1);
        }
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C17380.  The page opened: the inventory column active, both at the top.
void VIWndBankItem::Show()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_inv->SetX64(1);
    m_inv->ResetScroll();
    m_bank->SetX64(0);
    m_bank->ResetScroll();
    m_qty->SetVisible(false);
    app->SetActiveWindow(this);
}

// 0x00C17418.
void VIWndBankItem::UpdateText()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIBankControl *bank = app->GetBank();
    ClearText();
    if (m_inv->GetX64() && !app->GetInvCount())
        return;
    if (m_bank->GetX64() && !bank->Count())
        return;
    if (m_inv->GetX64())
        m_info.DescribeBankListItem(m_text, 1024, m_inv->m_sel, 0);
    else if (m_bank->GetX64())
        m_info.DescribeBankListItem(m_text, 1024, m_bank->m_sel, 2);
}

// 0x00C17548.
void VIWndBankItem::ClearText()
{
    m_text[0] = 0;
}
