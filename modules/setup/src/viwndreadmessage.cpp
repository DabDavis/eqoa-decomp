// viwndreadmessage.cpp (name ours) -- SETUP 0x0057AC60 .. 0x0057B1A0.  A page of
// text that scrolls seven rows at a time: the patcher's "Update Messages".
#include "setup.h"

// One WORDS.TXT entry in the current language, as MCERR's.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x0057AC60.
VIWndReadMessage::VIWndReadMessage()
{
    m_x1A8 = 0;
    m_dirty = 1;
}

// 0x0057ACB0.
VIWndReadMessage::~VIWndReadMessage()
{
    Clear();
}

// 0x0057AD08.
int VIWndReadMessage::Init()
{
    Clear();
    return 0;
}

// 0x0057AD28.
void VIWndReadMessage::Clear()
{
    m_text[0] = 0;
}

// 0x0057AD30.  The row count is redone after any change to the text.  The
// row offset rides in the top byte of TextBox's flags.
void VIWndReadMessage::OnDrawSelf()
{
    VIUI *ui = UI();
    unsigned short s[256];
    ui->Begin2D();
    m_window.Draw();
    if (m_dirty)
    {
        m_rows = m_window.CountLines(m_text, 360);
        m_top = 0;
        m_dirty = 0;
    }
    float t;
    if (m_rows < 8)
        t = -1.0f;
    else
        t = (float)m_top / (float)(m_rows - 7);
    m_window.DrawFrameVScroll(40, 1, m_window.Width() - 80, 7, t);
    m_window.TextBox(m_text, 40, 1, m_window.Width() - 80, 7, VIUICOLOR_0, m_top << 24);
    VIString::str8to16(s, sizeof(s), "|t ");
    VIString::strcat16(s, sizeof(s), Word(UI(), 750));
    m_window.Text(s, 40, 9, VIUICOLOR_0, 0);
    ui->End2D();
}

// 0x0057AEA0.  Up/down scroll; Escape or triangle goes back.
void VIWndReadMessage::OnKeyDown(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        Prev();
        break;
    case 20:
    case 139:
        Next();
        break;
    case 27:
    case 132:
        UI()->SoundPlay((VIUISoundId)8);
        SetVisible(false);
        SetFocus(GetParent(), true);
        break;
    }
}

// 0x0057AF68.
void VIWndReadMessage::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295 && p2 == (int)this)
    {
        m_window.Close(true);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 440, 10, 0);
        m_window.SetTitle(Word(UI(), 789));
        m_window.Open(false);
        m_dirty = 1;
    }
}

// 0x0057B030.
const unsigned short *VIWndReadMessage::Message() const
{
    return m_text;
}

// 0x0057B038.
void VIWndReadMessage::SetMessage(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
    m_dirty = 1;
}

// 0x0057B078.
void VIWndReadMessage::AppendMessage(const unsigned short *s)
{
    VIString::strcat16(m_text, sizeof(m_text), s);
}

// 0x0057B0A0.
void VIWndReadMessage::Prev()
{
    if (m_top > 0)
    {
        m_top--;
        UI()->SoundPlay((VIUISoundId)11);
    }
    else
        UI()->SoundPlay((VIUISoundId)9);
}

// 0x0057B0F0.
void VIWndReadMessage::Next()
{
    if (m_top < m_rows - 7)
    {
        m_top++;
        UI()->SoundPlay((VIUISoundId)11);
    }
    else
        UI()->SoundPlay((VIUISoundId)9);
}
