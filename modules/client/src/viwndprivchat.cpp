#line 1 "C:/code-frontiers-gm/game/VIWndPrivChat.cpp"
// VIWndPrivChat.cpp -- CLIENT 0x00C83858 .. 0x00C84878.  A private-chat notice.
#include "game.h"

// 0x00C83858.
VIWndPrivChat::VIWndPrivChat()
{
    Clear();
    m_rows = 0;
}

// 0x00C838A0.
VIWndPrivChat::~VIWndPrivChat()
{
}

// 0x00C838F0.
int VIWndPrivChat::Init()
{
    m_rows = 0;
    return 0;
}

// 0x00C83900.
void VIWndPrivChat::Clear()
{
    m_text[0] = 0;
    m_rows = 0;
}

// 0x00C83910.
void VIWndPrivChat::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    ui.Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F grey(0.8f, 0.8f, 0.8f, 1.0f);
    m_window.Draw();
    m_window.TextBox(m_text, 20, 0, 360, m_rows - 2, grey, 3);
    unsigned short s[32];
    VIString::snprintf16(s, sizeof(s), "|x%S", (const unsigned short *)ui.m_words.GetString(280, ui.m_language));
    m_window.Text(s, 20, m_rows - 1, white, 1);
    ui.End2D();
}

// 0x00C83A28.
void VIWndPrivChat::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 13:
    case 134:
        GetParent()->OnMessage(this, 279, 0, 0);
        break;
    }
}

// 0x00C83AF0.
void VIWndPrivChat::SetText(const unsigned short *s)
{
    VIClient *app = (VIClient *)GetAppPtr();
    Clear();
    VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x00C83B38.  Gaining the focus sizes the window to the text.  Anything else goes to
// VIWnd with this window, not w, as the sender.
void VIWndPrivChat::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 400, 16, 1);
            m_rows = m_window.CountLines(m_text, 360) + 2;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 400, m_rows, 1);
            if (m_window.IsOpen())
                m_window.Close(true);
            m_window.Open(false);
        }
    }
    else
        VIWnd::OnMessage(this, msg, p1, p2);
}
