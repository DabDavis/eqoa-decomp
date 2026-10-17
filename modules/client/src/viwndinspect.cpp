#line 1 "C:/code-frontiers-gm/game/VIWndInspect.cpp"
// viwndinspect.cpp -- CLIENT 0x00C644C8 .. 0x00C658C8: VIWndInspect, the scrolling
// description viewer the item, auction, experiment and recipe windows open.
#include "game.h"

// 0x00C644C8.
VIWndInspect::VIWndInspect()
{
    m_depth = 0;
    m_p28 = 0;
    m_text[0] = 0;
    m_lines = 0;
}

// 0x00C64520.
VIWndInspect::~VIWndInspect()
{
    Free();
}

// 0x00C64580.
int VIWndInspect::Init(int depth, int p)
{
    Clear();
    m_depth = depth;
    m_p28 = p;
    m_text[0] = 0;
    m_lines = 0;
    return 0;
}

// 0x00C645D0.
int VIWndInspect::Free()
{
    Clear();
    m_depth = 0;
    m_p28 = 0;
    return 0;
}

// 0x00C64600.
void VIWndInspect::Clear()
{
    m_text[0] = 0;
    m_lines = 0;
}

// 0x00C64610.
void VIWndInspect::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    ui.Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    m_window.Draw();
    if (m_shown < m_lines)
    {
        m_window.DrawFrameVScroll(20, 0, 420, m_shown,
                                  (float)m_top / (float)(m_lines - m_shown));
        m_window.TextBox(m_text, 20, 0, 410, m_shown, white, (m_top << 24) | 1);
    }
    else
        m_window.TextBox(m_text, 20, 0, 410, m_shown, white, 1);
    unsigned short buf[64];
    if (m_depth > 1)
        VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S",
                             ui.m_words.GetString(77, ui.m_language),
                             ui.m_words.GetString(161, ui.m_language));
    else
        VIString::snprintf16(buf, sizeof(buf), "|t%S",
                             ui.m_words.GetString(161, ui.m_language));
    m_window.Text(buf, 20, m_rows - 1, white, 1);
    ui.End2D();
    if (m_flash.Pulse(139))
        Next();
    if (m_flash.Pulse(138))
        Prev();
}

// 0x00C64808.
void VIWndInspect::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        m_parent->OnMessage(this, 260, 0, 0);
        break;
    case 13:
    case 134:
        if (m_depth > 1)
        {
            m_depth--;
            m_parent->OnMessage(this, 297, m_depth, m_p28);
            app->PlaySound(13);
        }
        break;
    case 19:
    case 138:
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        m_flash.Press(139);
        break;
    }
}

// 0x00C64978.
void VIWndInspect::OnKeyUp(int key)
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

// 0x00C649F0.
void VIWndInspect::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x00C64A18.  The window is sized to the text when it takes the focus.
void VIWndInspect::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if ((VIWnd *)p2 == this)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 170), 410, 14, 1);
            m_lines = m_window.CountLines(m_text, 410);
            m_rows = m_lines + 2 > 17 ? 17 : m_lines + 2;
            m_top = 0;
            m_shown = m_rows - 2;
            if (m_shown < m_lines)
                m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 170), 470, m_rows, 1);
            else
                m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 170), 450, m_rows, 1);
            if (m_window.IsOpen())
                m_window.Close(true);
            m_window.Open(false);
            m_flash.Reset();
        }
        else if ((VIWnd *)p1 == this)
            m_flash.Reset();
    }
    else
        VIWnd::OnMessage(this, msg, p1, p2);
}

// 0x00C64BC0.
void VIWndInspect::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_lines > m_shown)
    {
        if (m_top < m_lines - m_shown)
        {
            app->PlaySound(11);
            m_top++;
        }
        else
            app->PlaySound(9);
    }
}

// 0x00C64C30.
void VIWndInspect::Prev()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_shown < m_lines)
    {
        if (m_top <= 0)
        {
            app->PlaySound(9);
            m_top = 0;
        }
        else
        {
            app->PlaySound(11);
            m_top--;
        }
    }
}
