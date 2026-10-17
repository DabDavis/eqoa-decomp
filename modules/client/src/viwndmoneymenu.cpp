#line 1 "C:/code-frontiers-gm/game/VIWndMoneyMenu.cpp"
// VIWndMoneyMenu.cpp -- CLIENT 0x00C7D8E8 .. 0x00C7EB10.  A three-way money menu.
#include "game.h"

// 0x00C7D8E8.
VIWndMoneyMenu::VIWndMoneyMenu()
{
    m_sel = 0;
}

// 0x00C7D928.
VIWndMoneyMenu::~VIWndMoneyMenu()
{
    Free();
}

// 0x00C7D980.
int VIWndMoneyMenu::Init()
{
    m_sel = 0;
    return 0;
}

// 0x00C7D990.
int VIWndMoneyMenu::Free()
{
    m_sel = 0;
    return 0;
}

// 0x00C7D9A0.
void VIWndMoneyMenu::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUIWindow &win = m_window;
    VIUI &ui = app->m_ui;
    ui.Begin2D();
    win.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 250, 7, 1);
    win.Draw();
    VIColor32F white(1.0f, 1.0f, 1.0f, win.m_open);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, win.m_open);
    win.TextBox(win.T((unsigned short)349), 20, 0, win.Width() - 40, 1, white, 0);
    win.TextBox(win.T((unsigned short)351), 40, 2, win.Width() - 60, 1, m_sel == 0 ? yellow : white, 0);
    win.TextBox(win.T((unsigned short)352), 40, 3, win.Width() - 60, 1, m_sel == 1 ? yellow : white, 0);
    win.TextBox(win.T((unsigned short)217), 40, 4, win.Width() - 60, 1, m_sel == 2 ? yellow : white, 0);
    win.TickR(31, m_sel + 2, true);
    unsigned short s[128];
    VIString::snprintf16(s, sizeof(s), "|x%S   |t%S",
                         (const unsigned short *)ui.m_words.GetString(212, ui.m_language),
                         (const unsigned short *)ui.m_words.GetString(217, ui.m_language));
    win.TextBox(s, 20, 6, win.Width() - 40, 1, white, 0);
    ui.End2D();
}

// 0x00C7DBF0.
void VIWndMoneyMenu::OnKeyDown(int key)
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
        app->PlaySound(13);
        switch (m_sel)
        {
        case 0:
            GetParent()->OnMessage(this, 291, 0, 0);
            break;
        case 1:
            GetParent()->OnMessage(this, 292, 0, 0);
            break;
        case 2:
            GetParent()->OnMessage(this, 260, 0, 0);
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

// 0x00C7DD70.
void VIWndMoneyMenu::Move(int d)
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

// 0x00C7DDF8.
void VIWndMoneyMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if (p2 == (int)this && p1 != (int)this)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 250, 7, 1);
            m_window.Close(true);
            m_window.Open(false);
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}
