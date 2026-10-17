#line 1 "C:/code-frontiers-gm/game/VIWndFaction.cpp"
// VIWndFaction.cpp -- CLIENT 0x00C49D90 .. 0x00C4BB28: the faction standings window
// (cl_we.h).  Names ours.
#include "game.h"

// 0x00C49D90.
VIWndFaction::VIWndFaction()
{
    m_app = 0;
    m_sel = 0;
    m_mode = 0;
}

// 0x00C49DF0.
VIWndFaction::~VIWndFaction()
{
}

// 0x00C49E68.
int VIWndFaction::Init()
{
    m_app = 0;
    m_sel = 0;
    m_mode = 0;
    return 0;
}

// 0x00C49E80.
void VIWndFaction::OnDrawSelf()
{
    unsigned short buf[256];
    VIUI &ui = m_app->m_ui;
    VIClientPart4F078 *part = m_app->GetP4F078();
    ui.Begin2D();
    ui.BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    VIString::strcpy16(buf, sizeof(buf), m_window.T("Factions"));
    unsigned short *title = buf;
    int w = ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, title) + 64;
    ui.BlitBar(title, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), VIVect2I(320 - w / 2, 15), (VIUITexId)33, (VIUITexId)34,
               ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, title) + 64,
               (VIAlphaBlendMode)0, 1.0f, 0, 1);
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 600, 14, 0);
    m_window.Open(true);
    m_window.Draw();
    VIVect2I pos;
    pos.x = 40;
    pos.y = 330;
    m_window.TextBox(m_window.T(509), pos.x, 1, pos.y - pos.x, 1, (VIUICOLOR)0, 0);
    m_window.TextBox(m_window.T(1769), pos.y, 1, m_window.Width() - pos.y - 25, 1, (VIUICOLOR)0, 8);
    if (part->IsReady())
    {
        VIArray<VIRec132_BEB670> *list = part->GetList();
        int row = 2;
        for (int i = 0; i < list->m_size && row < 12; row++, i++)
        {
            int sel = i == m_sel;
            bool on = m_mode == 0;
            VIUICOLOR col = (VIUICOLOR)0;
            if (sel)
                col = (VIUICOLOR)1;
            m_window.TextBox((*list)[i].m_name, pos.x, row, pos.y - pos.x, 1, col, 2);
            if (sel)
                m_window.TickR(pos.x, row, on);
            DrawStanding((*list)[i].m_value, pos.y, row, m_window.Width() - pos.y - 25);
        }
        if (m_mode == 1)
        {
            if (list->m_size > m_sel)
            {
                VIUIWindow &win = m_win3;
                VIString::snprintf16(buf, sizeof(buf), "%S: %d", (*list)[m_sel].m_name, (*list)[m_sel].m_value);
                int lines = win.CountLines(buf, 310);
                win.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 350, lines + 2, 1);
                win.Open(false);
                win.Draw();
                win.TextBox(buf, 20, 0, 310, lines, VIColor32F(1.0f, 1.0f, 1.0f, win.m_open), 1);
                VIString::snprintf16(buf, sizeof(buf), "|t%S", ui.m_words.GetString(161, ui.m_language));
                win.TextBox(buf, 20, lines + 1, 350, 1, VIColor32F(1.0f, 1.0f, 1.0f, win.m_open), 1);
            }
        }
        else
            m_win3.Close(true);
    }

    if (part->GetP14() < 2)
        VIString::snprintf16(buf, sizeof(buf), "|-|s%S  |c%S  |+|x%S  |t%S",
                             ui.m_words.GetString(1692, ui.m_language), ui.m_words.GetString(216, ui.m_language),
                             ui.m_words.GetString(212, ui.m_language), ui.m_words.GetString(161, ui.m_language));
    else if (part->GetP14() > 0 && part->GetP10() == part->GetP14() - 1)
        VIString::snprintf16(buf, sizeof(buf), "|s%S  |-|c%S  |+|x%S  |t%S",
                             ui.m_words.GetString(1692, ui.m_language), ui.m_words.GetString(216, ui.m_language),
                             ui.m_words.GetString(212, ui.m_language), ui.m_words.GetString(161, ui.m_language));
    else if (part->GetP14() > 0 && part->GetP10() == 0)
        VIString::snprintf16(buf, sizeof(buf), "|-|s%S  |+|c%S  |x%S  |t%S",
                             ui.m_words.GetString(1692, ui.m_language), ui.m_words.GetString(216, ui.m_language),
                             ui.m_words.GetString(212, ui.m_language), ui.m_words.GetString(161, ui.m_language));
    else
        VIString::snprintf16(buf, sizeof(buf), "|s%S  |c%S  |x%S  |t%S",
                             ui.m_words.GetString(1692, ui.m_language), ui.m_words.GetString(216, ui.m_language),
                             ui.m_words.GetString(212, ui.m_language), ui.m_words.GetString(161, ui.m_language));
    pos.y = 390;
    m_window.TextBox(buf, pos.x - 10, 13, pos.y - pos.x, 1, (VIUICOLOR)0, 0);
    if (part->IsReady())
    {
        int pages = part->GetP14();
        int page;
        if (pages > 0)
            page = part->GetP10() + 1;
        else
            page = 0;
        VIString::snprintf16(buf, sizeof(buf), (const unsigned short *)ui.m_words.GetString(1773, ui.m_language), page, pages);
        m_window.TextBox(buf, pos.y, 13, m_window.Width() - pos.y - 25, 1, (VIUICOLOR)0, 16);
    }
    if (!part->IsReady())
    {
        m_win2.Init(VIWnd::mUI, VIVect2I(0, 0), VIVect2I(320, 180), 300, 3, 0);
        m_win2.Open(false);
        m_win2.SetTitle(m_win2.T(271));
        m_win2.Draw();
        VIString::snprintf16(buf, sizeof(buf), "%S%s", m_win2.T(1771), "...");
        m_win2.TextBox(buf, 40, 1, m_window.Width() - 60, 1, (VIUICOLOR)0, 0);
    }
    else
        m_win2.Close(true);
    ui.End2D();
    if (m_flash.Pulse(138))
        Up();
    if (m_flash.Pulse(139))
        Down();
}

// 0x00C4A6A8.
void VIWndFaction::OnKeyDown(int key)
{
    VIClientPart4F078 *part = m_app->GetP4F078();
    switch (key)
    {
    case 27:
    case 132:
        m_app->PlaySound(8);
        if (m_mode == 0)
            part->Show();
        else if (m_mode == 1)
            m_mode = 0;
        break;
    case 81:
    case 113:
    case 133:
        if (m_mode == 0)
            PrevPage();
        break;
    case 69:
    case 101:
    case 135:
        if (m_mode == 0)
            NextPage();
        break;
    case 19:
    case 138:
        if (m_mode == 0)
            m_flash.Press(138);
        break;
    case 20:
    case 139:
        if (m_mode == 0)
            m_flash.Press(139);
        break;
    case 13:
    case 134:
        if (m_mode == 0)
            Select();
        break;
    }
}

// 0x00C4A7E0.
void VIWndFaction::OnKeyUp(int key)
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

// 0x00C4A858.
void VIWndFaction::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        m_app = app;
        if ((VIWnd *)p1 != this)
        {
            if ((VIWnd *)p2 == this)
            {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 600, 15, 0);
            m_window.Open(true);
            m_win2.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 300, 3, 0);
            m_win2.Close(true);
            m_win3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 300, 3, 0);
            m_win3.Close(true);
            m_sel = 0;
            m_mode = 0;
            m_flash.Reset();
            }
        }
        else if (p2 != p1)
            m_flash.Reset();
    }
}

// 0x00C4A9D8.
void VIWndFaction::NextPage()
{
    VIClientPart4F078 *part = m_app->GetP4F078();
    if (!part->IsReady() || part->GetP10() == part->GetP14() - 1)
        m_app->PlaySound(9);
    else
    {
        m_sel = 0;
        m_app->PlaySound(13);
        part->Request(part->GetP10() + 1);
    }
}

// 0x00C4AA88.
void VIWndFaction::PrevPage()
{
    VIClientPart4F078 *part = m_app->GetP4F078();
    if (!part->IsReady() || part->GetP10() == 0)
        m_app->PlaySound(9);
    else
    {
        m_sel = 0;
        m_app->PlaySound(13);
        part->Request(part->GetP10() - 1);
    }
}

// 0x00C4AB20.
void VIWndFaction::Up()
{
    VIClientPart4F078 *part = m_app->GetP4F078();
    if (!part->IsReady() || m_sel == 0)
        m_app->PlaySound(9);
    else
    {
        m_sel--;
        m_app->PlaySound(11);
    }
    m_window.ResetTickPulse();
}

// 0x00C4ABA0.
void VIWndFaction::Down()
{
    VIClientPart4F078 *part = m_app->GetP4F078();
    if (!part->IsReady())
        m_app->PlaySound(9);
    else if (m_sel >= part->GetList()->m_size - 1)
        m_app->PlaySound(9);
    else
    {
        m_sel++;
        m_app->PlaySound(11);
    }
    m_window.ResetTickPulse();
}

// 0x00C4AC40.
void VIWndFaction::Select()
{
    VIClientPart4F078 *part = m_app->GetP4F078();
    if (!part->IsReady() || m_sel >= part->GetList()->m_size)
        m_app->PlaySound(9);
    else
    {
        m_app->PlaySound(13);
        m_mode = 1;
    }
}

// 0x00C4ACD0.  A standing as a bar from the middle of its cell: left and red when
// negative, right and blue when positive, darker toward the extremes.
void VIWndFaction::DrawStanding(int value, int x, int row, int w)
{
    if (value == 0)
        return;
    int v = value >= -2000 ? value : -2000;
    value = v <= 2000 ? v : 2000;
    int half = w >> 1;
    int len = (int)(value < 0 ? (float)value / -2000.0f * (float)half : (float)value / 2000.0f * (float)half);
    if (len == 0)
        len = 1;
    int at = x + half;
    if (value < 0)
        at -= len;
    VIColor32F c;
    if (value < -700)
        c.Set(0.5f, 0.0f, 0.0f, 1.0f);
    else if (value < -350)
        c.Set(0.75f, 0.0f, 0.0f, 1.0f);
    else if (value < -150)
        c.Set(1.0f, 0.0f, 0.0f, 1.0f);
    else if (value < 0)
        c.Set(1.0f, 0.25f, 0.25f, 1.0f);
    else if (value < 150)
        c.Set(0.25f, 0.25f, 1.0f, 1.0f);
    else if (value < 350)
        c.Set(0.0f, 0.0f, 1.0f, 1.0f);
    else if (value < 700)
        c.Set(0.0f, 0.0f, 0.75f, 1.0f);
    else
        c.Set(0.0f, 0.0f, 0.5f, 1.0f);
    m_window.DrawFrame(x, row, w, 1, 0);
    m_window.DrawColorBar(at, row, len, c);
}
