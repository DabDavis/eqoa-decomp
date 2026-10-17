#line 1 "C:/code-frontiers-gm/game/VIWndMercMenu.cpp"
// VIWndMercMenu.cpp -- CLIENT 0x00C7A000 .. 0x00C7B320.  The merchant's first menu: the
// greeting, then Buy, Sell and Leave.
#include "game.h"

// 0x00C7A000.
VIWndMercMenu::VIWndMercMenu()
{
    m_sel = 0;
    m_rows = 0;
}

// 0x00C7A048.
VIWndMercMenu::~VIWndMercMenu()
{
    Free();
}

// 0x00C7A0A0.
int VIWndMercMenu::Init()
{
    m_sel = 0;
    m_text[0] = 0;
    m_rows = 0;
    return 0;
}

// 0x00C7A0B8.
int VIWndMercMenu::Free()
{
    m_sel = 0;
    m_text[0] = 0;
    m_rows = 0;
    return 0;
}

// 0x00C7A0D0.
void VIWndMercMenu::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    ui.Begin2D();
    VIUIWindow &win = m_window;
    VIColor32F white(1.0f, 1.0f, 1.0f, win.m_open);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, win.m_open);
    win.Draw();
    win.TextBox(m_text, 20, 0, win.Width() - 40, m_rows, white, 0);
    win.TextBox(win.T((unsigned short)63), 40, m_rows + 1, win.Width() - 60, 1, m_sel == 0 ? yellow : white, 0);
    win.TextBox(win.T((unsigned short)62), 40, m_rows + 2, win.Width() - 60, 1, m_sel == 1 ? yellow : white, 0);
    win.TextBox(win.T((unsigned short)161), 40, m_rows + 3, win.Width() - 60, 1, m_sel == 2 ? yellow : white, 0);
    unsigned short s[16];
    VIString::snprintf16(s, sizeof(s), "|t%S", (const unsigned short *)ui.m_words.GetString(161, ui.m_language));
    win.TextBox(s, 20, m_rows + 5, win.Width() - 40, 1, white, 0);
    switch (m_sel)
    {
    case 0:
        win.TickR(35, m_rows + 1, true);
        break;
    case 1:
        win.TickR(35, m_rows + 2, true);
        break;
    case 2:
        win.TickR(35, m_rows + 3, true);
        break;
    }
    ui.End2D();
}

// 0x00C7A350.
void VIWndMercMenu::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        app->MerchantClose();
        break;
    case 13:
    case 134:
        app->PlaySound(13);
        switch (m_sel)
        {
        case 0:
            GetParent()->OnMessage(this, 281, 0, 0);
            break;
        case 1:
            GetParent()->OnMessage(this, 280, 0, 0);
            break;
        case 2:
            app->MerchantClose();
            break;
        }
        break;
    case 19:
    case 138:
        Move(-1);
        break;
    case 20:
    case 139:
        Move(1);
        break;
    }
}

// 0x00C7A4E0.  Gaining the focus (not from the parent) sizes the window to the greeting.
void VIWndMercMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if (p2 == (int)this && p1 != (int)GetParent())
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 390, 10, 1);
            m_rows = m_window.CountLines(m_text, m_window.Width() - 40);
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), m_window.Width(), m_rows + 6, 1);
            m_window.Close(true);
            m_window.Open(false);
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C7A618.
void VIWndMercMenu::Move(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel + d < 0)
    {
        app->PlaySound(9);
        m_sel = 0;
    }
    else if (m_sel + d >= 3)
    {
        app->PlaySound(9);
        m_sel = 2;
    }
    else
    {
        app->PlaySound(11);
        m_sel += d;
    }
    m_window.ResetTickPulse();
}

// 0x00C7A6A0.
// VIClient::ExpandText (0x00BE3AC0) is an INLINE member in the original: called through
// an inline, its arguments are evaluated before `app->m_text`.  The out-of-line copy is
// viclient.cpp's, so the inline is restated here as a file-scope helper.
static inline int ExpandText(VIClient *app, unsigned short *dst, unsigned short size,
                             const unsigned short *src, int flag)
{
    return app->m_text.Expand(dst, size, src, flag);
}

void VIWndMercMenu::Refresh()
{
    VIClient *app = (VIClient *)GetAppPtr();
    ExpandText(app, m_text, 256, app->m_merchantName, 1);
}
