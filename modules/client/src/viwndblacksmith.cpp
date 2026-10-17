#line 1 "C:/code-frontiers-gm/game/VIWndBlacksmith.cpp"
// viwndblacksmith.cpp -- CLIENT 0x00C1A510 .. 0x00C1C0E0: the repair window.  A list of
// the inventory beside a panel: the highlighted item's description, its repair cost
// against the player's money, and up to six stat lines coloured by how the item
// compares with what is worn.  256 from the list repairs an item (0x0062).
#include "game.h"

// 0x00C1A510.
VIWndBlacksmith::VIWndBlacksmith()
{
    m_money = 0;
    m_cost = 0;
    m_list = 0;
}

// 0x00C1A590.
VIWndBlacksmith::~VIWndBlacksmith()
{
    Free();
}

// 0x00C1A5F0.
int VIWndBlacksmith::Init()
{
    m_list = new VIWndList;
    if (m_list)
    {
        m_list->SetVisible(true);
        m_list->SetPos(VIVect2I(16, 37));
        m_list->SetAppPtr(GetAppPtr());
        AddChild(m_list);
        m_list->Init(40);
        Clear();
        m_money = 0;
        m_cost = 0;
        m_info.SetClient((VIClient *)GetAppPtr());
    }
    return 0;
}

// 0x00C1A6B0.
int VIWndBlacksmith::Free()
{
    if (m_list)
    {
        RemoveChild(m_list);
        delete m_list;
        m_list = 0;
    }
    Clear();
    m_money = 0;
    m_cost = 0;
    return 0;
}

// 0x00C1A720.  The banner, two panels, the item's description on the left, the cost,
// the money and the stat lines on the right; then the list's rows from the inventory.
void VIWndBlacksmith::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIVect2I unused;
    Refresh();
    VIUI *ui = &app->m_ui;
    ui->Begin2D();
    VIFont *font = &ui->m_font;
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    font->SetTextColor(black);
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict,
                                 (const unsigned short *)ui->m_words.GetString(609, ui->m_language)) + 64;
    ui->BlitBar((unsigned short)609, white, VIVect2I((640 - w) / 2, 8), (VIUITexId)33, (VIUITexId)34, w,
                (VIAlphaBlendMode)0, 1.0f, 0, 1);
    VIUIFrameParam frame;
    memset(&frame, 0, sizeof(frame));
    frame.m_bottom.m_id = (VIUITexId)49;
    frame.m_left.m_id = (VIUITexId)47;
    frame.m_bl.m_id = (VIUITexId)48;
    frame.m_br.m_id = (VIUITexId)50;
    frame.m_right.m_id = (VIUITexId)52;
    frame.m_top.m_id = (VIUITexId)45;
    frame.m_tl.m_id = (VIUITexId)44;
    frame.m_tr.m_id = (VIUITexId)46;
    ui->BlitFillFrame(VIVect2I(165, 48), VIVect2I(291, 267), frame, (VIUITexId)35, (VIAlphaBlendMode)0, 1.0f, 1);
    ui->BlitFillFrame(VIVect2I(456, 48), VIVect2I(184, 267), frame, (VIUITexId)35, (VIAlphaBlendMode)0, 1.0f, 1);
    ui->BlitStuddedHorz(VIVect2I(154, 37), (VIUITexId)37, (VIUITexId)38, 487, 4, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(154, 313), (VIUITexId)40, (VIUITexId)41, 487, 4, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitHorz(VIVect2I(16, 322), (VIUITexId)60, 138, (VIAlphaBlendMode)0, 1.0f, 0);
    m_window.TextBox(m_text, 20, 0, 270, 15, (VIUICOLOR)0, 2);
    unsigned short buf[32];
    if (m_cost != -1)
    {
        VIString::snprintf16(buf, 64, "%S:", (const unsigned short *)ui->m_words.GetString(523, ui->m_language));
        m_window.Text(buf, 310, 0, (VIUICOLOR)0, 0);
        VIString::snprintf16(buf, 64, "%d %S", m_cost,
                             (const unsigned short *)ui->m_words.GetString(479, ui->m_language));
        m_window.TextBox(buf, 310, 1, 160, 1, (VIUICOLOR)0, 2);
    }
    VIString::snprintf16(buf, 64, "%S:", (const unsigned short *)ui->m_words.GetString(506, ui->m_language));
    m_window.Text(buf, 310, m_cost != -1 ? 2 : 0, (VIUICOLOR)0, 0);
    VIString::snprintf16(buf, 64, "%d %S", m_money,
                         (const unsigned short *)ui->m_words.GetString(479, ui->m_language));
    m_window.TextBox(buf, 310, m_cost != -1 ? 3 : 1, 160, 1, (VIUICOLOR)0, 2);
    int row = m_cost == -1 ? 2 : 4;
    for (int k = 0; k < 6 && row < 13; k++)
    {
        if (!m_lines[k].m_text[0])
            continue;
        int n = m_window.CountLines(m_lines[k].m_text, 160);
        if (row == 12)
            n = VIMIN(n, 1);
        m_window.TextBox(m_lines[k].m_text, 310, row, 160, n, m_lines[k].m_color, 2);
        row += n;
    }
    VIString::snprintf16(buf, 64, "|x%S", (const unsigned short *)ui->m_words.GetString(523, ui->m_language));
    m_window.Text(buf, 310, 13, (VIUICOLOR)0, 0);
    VIString::snprintf16(buf, 64, "|t%S", (const unsigned short *)ui->m_words.GetString(161, ui->m_language));
    m_window.Text(buf, 310, 14, (VIUICOLOR)0, 0);
    ui->End2D();
    int i;
    for (i = 0; i < app->GetInvCount(); i++)
    {
        int flags = app->GetInvItemAmount(i) >= 2 ? 5 : 1;
        int color = -1;
        int x = -1;
        if (!app->IsInvItemUsable(i))
        {
            color = 140;
            flags |= 8;
        }
        else if (app->GetInvItemCondition(i) < 0.25f)
        {
            x = 1;
            flags |= 0x20;
        }
        m_list->SetItem(i, flags, app->GetInvItemIcon(i), color, x, app->GetInvItemAmount(i), -1);
    }
    for (; i < 40; i++)
        m_list->SetItem(i, 0, -1, -1, -1, 0, -1);
}

// 0x00C1ADF0.
void VIWndBlacksmith::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 13:
    case 134:
    case 19:
    case 20:
    case 138:
    case 139:
        m_list->OnKeyDown(key);
        break;
    }
}

// 0x00C1AEF8.
void VIWndBlacksmith::OnKeyUp(int key)
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

// 0x00C1AF60.
void VIWndBlacksmith::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWndBlacksmith *)p2 == this && p1 != (int)m_list)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(403, 180), 490, 15, 0);
            m_window.Open(true);
        }
        break;
    case 256:
        if (app->IsInvItemUsable(p1))
            app->Send0062(p1);
        else
            app->PlaySound(10);
        break;
    }
}

// 0x00C1B060.
void VIWndBlacksmith::Clear()
{
    m_text[0] = 0;
    for (int i = 0; i < 6; i++)
    {
        m_lines[i].m_text[0] = 0;
        m_lines[i].m_color = VIColor32F(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

// 0x00C1B0C8.
void VIWndBlacksmith::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_list->SetX64(1);
    m_list->ResetScroll();
    app->SetActiveWindow(this);
}

// 0x00C1B130.
void VIWndBlacksmith::Refresh()
{
    VIClient *app = (VIClient *)GetAppPtr();
    Clear();
    if (app->GetInvCount() == 0)
    {
        SetCost(-1);
        return;
    }
    int item = m_list->m_sel;
    m_info.DescribeInvItemShort(m_text, sizeof(m_text), item);
    SetCost(app->GetInvItemRepairCost(item));
    VIArray<VIItemStat> *stats = app->GetInvItemStats(item);
    if (app->GetInvItemWearSlot(item) != -1)
        ShowStats(item, stats);
}

// 0x00C1B200.  One line per stat bonus: blue when the item would raise the stat over
// what is worn now, red when it would lower it.
void VIWndBlacksmith::ShowStats(int item, VIArray<VIItemStat> *stats)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int n = 0;
    VIUI *ui = &app->m_ui;
    for (int i = stats->Begin(); i != stats->End() && n < 6; i = stats->Next(i), n++)
    {
        VIItemStat *s = &(*stats)[i];
        unsigned short buf[256];
        VIString::snprintf16(buf, sizeof(buf), "%d: %d ", s->m_id, s->m_value);
        int now = app->GetStatTotal(s->m_id);
        int then = app->StatWithItem(item, s->m_id, 0);
        VIWordId *name = FindStatWord(s->m_id);
        if (name)
            VIString::snprintf16(m_lines[n].m_text, sizeof(m_lines[n].m_text), "%S: %d |} %d",
                                 ui->m_words.GetString(name->m_word, ui->m_language), s->m_value, then);
        else
            VIString::strcpy16(m_lines[n].m_text, sizeof(m_lines[n].m_text), buf);
        VIColor32F c = now < then ? VIColor32F(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f)
                     : then < now ? VIColor32F(95.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f, 1.0f)
                     : VIColor32F(0.0f, 0.0f, 0.0f, 1.0f);
        m_lines[n].m_color = c;
    }
}

// 0x00C1B470.
void VIWndBlacksmith::SetCost(int cost)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_cost = cost;
    m_money = app->m_player.m_p34;
}
