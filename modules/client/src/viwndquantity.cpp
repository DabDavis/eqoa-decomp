#line 1 "C:/code-frontiers-gm/game/VIWndQuantity.cpp"
// VIWndQuantity.cpp -- CLIENT 0x00C84880 .. 0x00C860C8.  The quantity picker the item
// windows open: up to four digits under a title, the total when a unit price is given,
// and the button legend.
#include "game.h"

// 0x00C84880.
VIWndQuantity::VIWndQuantity()
{
    m_cursor = 0;
    m_max = 0;
    m_price = -1;
    m_digitsShown = 1;
    m_id = 0;
}

// 0x00C848E0.
VIWndQuantity::~VIWndQuantity()
{
    Free();
}

// 0x00C84940.
int VIWndQuantity::Init()
{
    m_cursor = 0;
    m_max = 0;
    m_price = -1;
    m_id = 0;
    m_digitsShown = 1;
    return 0;
}

// 0x00C84968.
int VIWndQuantity::Free()
{
    m_cursor = 0;
    m_max = 0;
    m_price = -1;
    m_id = 0;
    m_digitsShown = 1;
    return 0;
}

// 0x00C84990.  The digits (the one under the cursor yellow, with up/down ticks), the
// total when there is a price, and the legend; then the repeating pad keys.
void VIWndQuantity::OnDrawSelf()
{
    unsigned short s[64];
    int rows = 5;
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_price >= 0)
        rows = 6;
    VIUI &ui = app->m_ui;
    ui.Begin2D();
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 270, rows, 1);
    m_window.Draw();
    VIUIWindow &win = m_window;
    VIColor32F white(1.0f, 1.0f, 1.0f, win.m_open);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, win.m_open);
    VIVect2I col(20, 100);
    int w = ui.m_font.GetCharacterWidth(ui.m_raster, ui.m_dict, 'W') - 6;
    m_window.TextBox(m_window.T((unsigned short)353), col.x, 1, col.y - col.x, 1, white, 3);
    int x = 100;
    for (int i = 4 - m_digitsShown; i < 4; i++)
    {
        unsigned short num[4];
        VIString::snprintf16(num, sizeof(num), "%d", m_digits[i]);
        m_window.TextBox(num, x, 1, w, 1, i == m_cursor ? yellow : white, 0);
        if (i == m_cursor)
        {
            m_window.TickD(x + w / 2, 0, true);
            m_window.TickU(x + w / 2, 2, true);
        }
        x += w;
    }
    if (m_price >= 0)
    {
        int total = m_price * GetValue();
        m_window.TextBox(m_window.T((unsigned short)286), col.x, 3, col.y - col.x, 1, white, 3);
        VIString::snprintf16(s, sizeof(s), "%d", total);
        m_window.TextBox(s, col.y, 3, m_window.Width() - col.y - 20, 1, white, 1);
    }
    VIString::snprintf16(s, sizeof(s), "|x%S   |t%S",
                         (const unsigned short *)ui.m_words.GetString(214, ui.m_language),
                         (const unsigned short *)ui.m_words.GetString(213, ui.m_language));
    m_window.TextBox(s, 20, rows - 1, m_window.Width() - 40, 1, white, 1);
    ui.End2D();
    if (m_keys.Pulse(136))
        MoveCursor(-1);
    if (m_keys.Pulse(137))
        MoveCursor(1);
    if (m_keys.Pulse(138))
        ChangeDigit(1);
    if (m_keys.Pulse(139))
        ChangeDigit(-1);
}

// 0x00C84D58.  Enter hands the parent the value, Triangle cancels; the pad keys only
// start their repeat here (OnDrawSelf acts on them).
void VIWndQuantity::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
    {
        int v = GetValue();
        if (v < 0)
            v = 0;
        GetParent()->OnMessage(this, 293, VIMIN(m_max, v), m_id);
        break;
    }
    case 27:
    case 132:
        app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 17:
    case 136:
        m_keys.Press(136);
        break;
    case 18:
    case 137:
        m_keys.Press(137);
        break;
    case 19:
    case 138:
        m_keys.Press(138);
        break;
    case 20:
    case 139:
        m_keys.Press(139);
        break;
    }
}

// 0x00C84F08.
void VIWndQuantity::OnKeyUp(int key)
{
    switch (key)
    {
    case 17:
    case 136:
        m_keys.Release(136);
        break;
    case 18:
    case 137:
        m_keys.Release(137);
        break;
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

// 0x00C84FD8.  Opens on the units digit set to 1, as many digits as the maximum needs.
void VIWndQuantity::Open(int max, int id, int price)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_max = max;
    m_id = id;
    m_price = price;
    m_cursor = 3;
    memset(m_digits, 0, sizeof(m_digits));
    m_digits[3] = 1;
    m_digitsShown = 1;
    int n = m_max;
    for (int i = 1; i < 4; i++)
        if (n / (int)pow(10, i) > 0)
            m_digitsShown++;
    SetVisible(true);
    app->SetActiveWindow(this);
}

// 0x00C850D8.
void VIWndQuantity::MoveCursor(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_cursor += d;
    if (m_cursor < 4 - m_digitsShown)
    {
        app->PlaySound(9);
        m_cursor = 4 - m_digitsShown;
    }
    else if (m_cursor >= 4)
    {
        app->PlaySound(9);
        m_cursor = 3;
    }
    else
        app->PlaySound(11);
    m_window.ResetTickPulse();
}

// 0x00C85180.  A digit wraps 0..9; past the maximum the digits snap to it.
void VIWndQuantity::ChangeDigit(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_digits[m_cursor] += d;
    if (m_digits[m_cursor] >= 10)
        m_digits[m_cursor] = 0;
    if (m_digits[m_cursor] < 0)
        m_digits[m_cursor] = 9;
    if (m_max < GetValue())
    {
        app->PlaySound(9);
        SetMax();
    }
    else
        app->PlaySound(11);
    m_window.ResetTickPulse();
}

// 0x00C85248.
int VIWndQuantity::GetValue()
{
    int v = 0;
    for (int i = 3; i >= 0; i--)
        v += m_digits[3 - i] * (int)pow(10, i);
    return v;
}

// 0x00C852C8.
void VIWndQuantity::SetMax()
{
    int v = m_max;
    memset(m_digits, 0, sizeof(m_digits));
    m_digits[3] = 1;
    for (int i = 3; i >= 0; i--)
    {
        m_digits[3 - i] = v / (int)pow(10, i);
        v -= m_digits[3 - i] * (int)pow(10, i);
    }
}

// 0x00C853A0.  Gaining the focus opens the window; losing it drops the pad repeat.
void VIWndQuantity::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 300, 6, 1);
            m_window.Close(true);
            m_window.Open(false);
            m_keys.Reset();
        }
        if (p1 == (int)this && p2 != (int)this)
            m_keys.Reset();
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}
