#line 1 "C:/code-frontiers-gm/game/VIWndBankMenu.cpp"
// viwndbankmenu.cpp (name ours) -- CLIENT 0x00C18178 .. 0x00C194C0: the bank's first page.
// Method names are ours.
#include "game.h"

// 0x00C18178.
VIWndBankMenu::VIWndBankMenu()
{
    m_cursor = 0;
    m_auction = 0;
}

// 0x00C181C0.
VIWndBankMenu::~VIWndBankMenu()
{
    Free();
}

// 0x00C18218.
int VIWndBankMenu::Init()
{
    m_cursor = 0;
    m_auction = 0;
    return 0;
}

// 0x00C18228.
int VIWndBankMenu::Free()
{
    m_cursor = 0;
    m_auction = 0;
    return 0;
}

// 0x00C18238.
// @@BEGIN OnDrawSelf
void VIWndBankMenu::OnDrawSelf()
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    int rows = m_auction ? 8 : 7;
    ui->Begin2D();
    m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 250, rows, 1);
    m_window.Draw();
    VIUIWindow &win = m_window;
    VIColor32F white(1.0f, 1.0f, 1.0f, win.m_open);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, win.m_open);
    m_window.TextBox(m_window.T(300), 20, 0, m_window.Width() - 40, 1, white, 0);
    m_window.TextBox(m_window.T(349), 40, 2, m_window.Width() - 60, 1, m_cursor == 0 ? yellow : white, 0);
    m_window.TextBox(m_window.T(350), 40, 3, m_window.Width() - 60, 1, m_cursor == 1 ? yellow : white, 0);
    if (!m_auction)
        m_window.TextBox(m_window.T(161), 40, 4, m_window.Width() - 60, 1, m_cursor == 2 ? yellow : white, 0);
    else
    {
        m_window.TextBox(m_window.T(1617), 40, 4, m_window.Width() - 60, 1, m_cursor == 2 ? yellow : white, 0);
        m_window.TextBox(m_window.T(161), 40, 5, m_window.Width() - 60, 1, m_cursor == 3 ? yellow : white, 0);
    }
    m_window.TickR(31, m_cursor + 2, true);
    unsigned short buf[128];
    VIString::snprintf16(buf, 256, "|x%S   |t%S", ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(161, ui->m_language));
    m_window.TextBox(buf, 20, rows - 1, m_window.Width() - 40, 1, white, 0);
    ui->End2D();
}
// @@END OnDrawSelf

// 0x00C18530.
void VIWndBankMenu::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel6AC();
        break;
    case 13:
    case 134:
        app->PlaySound(13);
        switch (m_cursor)
        {
        case 0:
            m_parent->OnMessage(this, 290, 0, 0);
            break;
        case 1:
            m_parent->OnMessage(this, 289, 0, 0);
            break;
        case 2:
            if (m_auction)
                m_parent->OnMessage(this, 310, 0, 0);
            else
                app->ClosePanel6AC();
            break;
        case 3:
            app->ClosePanel6AC();
            break;
        }
        break;
    case 19:
    case 138:
        MoveCursor(-1);
        break;
    case 20:
    case 139:
        MoveCursor(1);
        break;
    }
}

// 0x00C186E0.
void VIWndBankMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIBankControl *bank = ((VIClient *)GetAppPtr())->GetBank();
    if (msg == 295)
    {
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != this)
        {
            m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 224), 250, 8, 1);
            m_window.Close(1);
            m_window.Open(0);
            m_auction = bank->GetP78CC();
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C187F0.
void VIWndBankMenu::MoveCursor(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int last = m_auction ? 3 : 2;
    m_cursor += d;
    if (m_cursor < 0)
    {
        app->PlaySound(9);
        m_cursor = 0;
    }
    else if (m_cursor > last)
    {
        app->PlaySound(9);
        m_cursor = last;
    }
    else
        app->PlaySound(11);
    m_window.ResetTickPulse();
}
