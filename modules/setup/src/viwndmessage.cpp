// viwndmessage.cpp (name ours) -- SETUP 0x005755D8 .. 0x00575DD8.  The message
// box the other windows embed: a title, the text, and a prompt row of one or
// two choices.  Closing it sends the parent 307 with the selection.
#include "setup.h"

// One WORDS.TXT entry in the current language, as MCERR's.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x005755D8.
VIWndMessage::VIWndMessage()
{
}

// 0x00575620.
VIWndMessage::~VIWndMessage()
{
}

// 0x00575670.
int VIWndMessage::Init()
{
    m_sel = 0;
    return 0;
}

// 0x00575680.
void VIWndMessage::Free()
{
}

// 0x00575688.  The two choices sit centred on the window's thirds.  `s` is
// never used; it is still in the frame.
void VIWndMessage::OnDrawSelf()
{
    VIUI *ui = UI();
    int col[2];
    unsigned short s[256];
    ui->Begin2D();
    col[0] = m_window.Width() / 3;
    col[1] = col[0] * 2;
    m_window.Draw();
    m_window.TextBox(m_text, 40, 1, m_window.Width() - 80, m_rows - 2, VIUICOLOR_0, 8);
    switch (m_prompt)
    {
    case VIMESSAGEPROMPT_OK:
        m_window.TextBox(Highlight(Word(UI(), 786), 1), 0, m_rows - 1, m_window.Width(), 1,
                         VIUICOLOR_0, 8);
        break;
    case VIMESSAGEPROMPT_OKCANCEL:
        m_window.TextBox(Highlight(Word(UI(), 786), m_sel == 0), col[0] - 100, m_rows - 1, 200, 1,
                         VIUICOLOR_0, 8);
        m_window.TextBox(Highlight(Word(UI(), 753), m_sel == 1), col[1] - 100, m_rows - 1, 200, 1,
                         VIUICOLOR_0, 8);
        break;
    case VIMESSAGEPROMPT_YESNO:
        m_window.TextBox(Highlight(Word(UI(), 21), m_sel == 0), col[0] - 100, m_rows - 1, 200, 1,
                         VIUICOLOR_0, 8);
        m_window.TextBox(Highlight(Word(UI(), 22), m_sel == 1), col[1] - 100, m_rows - 1, 200, 1,
                         VIUICOLOR_0, 8);
        break;
    }
    ui->End2D();
}

// 0x005758F0.  Left/right move the selection, Enter or X closes.
void VIWndMessage::OnKeyDown(int key)
{
    switch (key)
    {
    case 17:
    case 136:
        if (m_sel > 0)
        {
            m_sel--;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        break;
    case 18:
    case 137:
        if (m_sel < (m_prompt ? 1 : 0))
        {
            m_sel++;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        break;
    case 13:
    case 134:
        SetVisible(false);
        GetParent()->OnMessage(this, 307, m_sel, 0);
        if (m_prompt == VIMESSAGEPROMPT_OK)
            UI()->SoundPlay((VIUISoundId)13);
        else
            UI()->SoundPlay((VIUISoundId)(m_sel ? 8 : 13));
        break;
    }
}

// 0x00575A40.
void VIWndMessage::OnKeyUp(int key)
{
}

// 0x00575A48.  Gaining the focus opens the box; a two-choice box starts on the
// second choice.
void VIWndMessage::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_window.Close(true);
            m_window.Init(UI(), VIVect2I(0, 0), m_center, m_width, m_rows, 0);
            m_window.Open(false);
            m_window.SetTitle(m_window.T(m_title));
            if (m_prompt == VIMESSAGEPROMPT_OK)
                m_sel = 0;
            else
                m_sel = 1;
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00575B08.
void VIWndMessage::Setup(const char *title, const char *text, VIMESSAGEPROMPT prompt,
                         const VIVect2I &center, int width)
{
    unsigned short s[256];
    VIString::str8to16(s, sizeof(s), text);
    Setup(title, s, prompt, center, width);
}

// 0x00575B80.
void VIWndMessage::Setup(const char *title, const unsigned short *text, VIMESSAGEPROMPT prompt,
                         const VIVect2I &center, int width)
{
    unsigned short s[256];
    VIString::str8to16(s, sizeof(s), title);
    Setup(s, text, prompt, center, width);
}

// 0x00575C00.
void VIWndMessage::Setup(const unsigned short *title, const unsigned short *text,
                         VIMESSAGEPROMPT prompt, const VIVect2I &center, int width)
{
    VIString::strcpy16(m_title, sizeof(m_title), title);
    VIString::strcpy16(m_text, sizeof(m_text), text);
    m_prompt = prompt;
    m_center = center;
    m_width = width;
    m_window.Init(UI(), VIVect2I(0, 0), m_center, width, 20, 0);
    m_rows = m_window.CountLines(m_text, width - 80) + 3;
    m_sel = 0;
}

// 0x00575CF0.
unsigned short *VIWndMessage::Highlight(const unsigned short *s, int on)
{
    if (on)
        VIString::str8to16(m_buf, sizeof(m_buf), "|q");
    else
        m_buf[0] = 0;
    VIString::strcat16(m_buf, sizeof(m_buf), s);
    if (on)
        VIString::strcat16(m_buf, sizeof(m_buf), "|w");
    return m_buf;
}
