#line 1 "C:/code-frontiers-gm/game/VIWndDeposit.cpp"
// VIWndDeposit.cpp -- CLIENT 0x00C41A60 .. 0x00C43498: the money-amount entry
// (cl_we.h).  Names ours.
#include "game.h"

extern "C" double pow(double, double);

// 0x00C41A60.
VIWndDeposit::VIWndDeposit()
{
    m_cursor = 0;
    m_carried = 0;
    m_banked = 0;
    m_mode = 0;
    m_width = 1;
}

// 0x00C41AC0.
VIWndDeposit::~VIWndDeposit()
{
    Free();
}

// 0x00C41B20.
int VIWndDeposit::Init()
{
    m_cursor = 0;
    m_carried = 0;
    m_banked = 0;
    m_width = 1;
    return 0;
}

// 0x00C41B40.
int VIWndDeposit::Free()
{
    m_mode = 0;
    m_cursor = 0;
    m_carried = 0;
    m_banked = 0;
    m_width = 1;
    return 0;
}

// 0x00C41B60.  The title, the digits (the cursor's in yellow, with its arrows), what
// the player has, and the button legend.  A held up/down repeats through VIKeyFlash.
void VIWndDeposit::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    ui.Begin2D();
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 320, 7, 1);
    m_window.Draw();
    VIUIWindow &win = m_window;
    VIColor32F white(1.0f, 1.0f, 1.0f, win.m_open);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, win.m_open);
    if (m_mode == 1)
        m_window.TextBox(m_window.T(351), 20, 1,
                         ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                             (const unsigned short *)ui.m_words.GetString(351, ui.m_language)) + 20,
                         1, white, 0);
    else if (m_mode == 2)
        m_window.TextBox(m_window.T(352), 20, 1,
                         ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                             (const unsigned short *)ui.m_words.GetString(352, ui.m_language)) + 20,
                         1, white, 0);
    else if (m_mode == 3)
        m_window.TextBox(m_window.T(473), 20, 1,
                         ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                             (const unsigned short *)ui.m_words.GetString(473, ui.m_language)) + 20,
                         1, white, 0);

    unsigned short buf[128];
    int x = m_window.Width() / 2 - 10;
    int w = ui.m_font.GetCharacterWidth(ui.m_raster, ui.m_dict, 'W') - 6;
    for (int i = 9 - m_width; i < 9; i++)
    {
        VIString::snprintf16(buf, 256, "%d", m_digit[i]);
        m_window.TextBox(buf, x, 1, w, 1, i == m_cursor ? yellow : white, 0);
        if (i == m_cursor)
        {
            m_window.TickD(x + w / 2, 0, true);
            m_window.TickU(x + w / 2, 2, true);
        }
        x += w;
    }

    x = m_window.Width() / 2 - 10;
    buf[0] = 0;
    if (m_mode == 1 || m_mode == 3)
    {
        m_window.TextBox(m_window.T(479), 20, 4,
                         ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                             (const unsigned short *)ui.m_words.GetString(479, ui.m_language)) + 20,
                         1, white, 0);
        VIString::snprintf16(buf, 256, "%d", m_carried);
    }
    else if (m_mode == 2)
    {
        m_window.TextBox(m_window.T(300), 20, 4,
                         ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                             (const unsigned short *)ui.m_words.GetString(300, ui.m_language)) + 20,
                         1, white, 0);
        VIString::snprintf16(buf, 256, "%d", m_banked);
    }
    m_window.TextBox(buf, x, 4, ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, buf), 1, white, 0);
    VIString::snprintf16(buf, 256, "|x%S   |t%S", ui.m_words.GetString(214, ui.m_language),
                         ui.m_words.GetString(217, ui.m_language));
    m_window.TextBox(buf, 20, 6, m_window.Width() - 40, 1, white, 0);
    ui.End2D();
    if (m_flash.Pulse(138))
        ChangeDigit(1);
    if (m_flash.Pulse(139))
        ChangeDigit(-1);
}

// 0x00C42070.
void VIWndDeposit::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIBankControl *bank = app->GetBank();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        m_parent->OnMessage(this, 260, 0, 0);
        break;
    case 13:
    case 134:
    {
        int v = GetValue();
        if (v == 0)
        {
            app->PlaySound(3);
            m_parent->OnMessage(this, 260, 0, 0);
        }
        else if (m_mode == 1 && v <= m_carried)
        {
            if (bank->IsIdle())
            {
                bank->MoveMoney(0, v);
                m_parent->OnMessage(this, 260, 0, 0);
            }
            else
                app->PlaySound(9);
        }
        else if (m_mode == 2 && v <= m_banked)
        {
            if (bank->IsIdle())
            {
                bank->MoveMoney(1, v);
                m_parent->OnMessage(this, 260, 0, 0);
            }
            else
                app->PlaySound(9);
        }
        else if (m_mode == 3 && v <= m_carried)
        {
            app->PlaySound(13);
            m_parent->OnMessage(this, 294, v, 0);
        }
        break;
    }
    case 19:
    case 138:
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        m_flash.Press(139);
        break;
    case 17:
    case 136:
        MoveCursor(-1);
        break;
    case 18:
    case 137:
        MoveCursor(1);
        break;
    }
}

// 0x00C42310.
void VIWndDeposit::OnKeyUp(int key)
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

// 0x00C42388.
void VIWndDeposit::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != this)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 300, 7, 1);
            m_window.Close(true);
            m_window.Open(false);
            m_flash.Reset();
        }
        if ((VIWnd *)p1 == this)
            m_flash.Reset();
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C42488.
void VIWndDeposit::MoveCursor(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int c = m_cursor + d;
    if (c < 9 - m_width)
    {
        app->PlaySound(9);
        m_cursor = 9 - m_width;
    }
    else if (c >= 9)
    {
        app->PlaySound(9);
        m_cursor = 8;
    }
    else
    {
        m_cursor = c;
        app->PlaySound(11);
    }
    m_window.ResetTickPulse();
}

// 0x00C42528.
void VIWndDeposit::ClearDigits()
{
    for (int i = 0; i < 9; i++)
        m_digit[i] = 0;
}

// 0x00C42558.  Up/down on the cursor's digit, wrapping; past the limit it plays the
// error and snaps to the limit.
void VIWndDeposit::ChangeDigit(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_digit[m_cursor] += d;
    if (m_digit[m_cursor] >= 10)
        m_digit[m_cursor] = 0;
    if (m_digit[m_cursor] < 0)
        m_digit[m_cursor] = 9;
    int v = GetValue();
    if (((m_mode == 1 || m_mode == 3) && m_carried < v) || (m_mode == 2 && m_banked < v))
    {
        app->PlaySound(9);
        SetMax();
    }
    else
        app->PlaySound(11);
    m_window.ResetTickPulse();
}

// 0x00C42668.
int VIWndDeposit::GetValue()
{
    int v = 0;
    for (int i = 8; i >= 0; i--)
        v += m_digit[8 - i] * (int)pow(10.0, i);
    return v;
}

// 0x00C426E0.
void VIWndDeposit::SetMax()
{
    int v;
    if (m_mode == 1 || m_mode == 3)
        v = m_carried;
    else if (m_mode == 2)
        v = m_banked;
    ClearDigits();
    for (int i = 8; i >= 0; i--)
    {
        m_digit[8 - i] = v / (int)pow(10.0, i);
        v -= m_digit[8 - i] * (int)pow(10.0, i);
    }
}

// 0x00C427C0.
void VIWndDeposit::Setup(int mode)
{
    int v;
    m_mode = mode;
    m_width = 1;
    if (mode == 1 || mode == 3)
        v = m_carried;
    else if (mode == 2)
        v = m_banked;
    for (int i = 1; i < 9; i++)
        if (v / (int)pow(10.0, i) > 0)
            m_width++;
}
