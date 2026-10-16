// viwndmessage.cpp (name ours) -- STATION 0x002F1DB8 .. 0x002F25E0.  The message
// box the other windows embed: a title, the text, and a prompt row of one or
// two choices.  Closing it sends the parent 307 with the selection.  SETUP's
// viwndmessage.cpp is the same file one revision later: every function here
// but OnDrawSelf is byte-identical to SETUP's, and SETUP's OnDrawSelf calls a
// Highlight helper this one does not have.
#include "station.h"

// One WORDS.TXT entry in the current language, as MCERR's and SETUP's.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x002F1DB8.
VIWndMessage::VIWndMessage()
{
}

// 0x002F1E00.
VIWndMessage::~VIWndMessage()
{
}

// 0x002F1E50.
int VIWndMessage::Init()
{
    m_sel = 0;
    return 0;
}

// 0x002F1E60.
void VIWndMessage::Free()
{
}

// 0x002F1E68.  The two choices sit centred on the window's thirds, the chosen
// one wrapped in |q..|w.  A box without the focus keeps its tick pulse reset.
void VIWndMessage::OnDrawSelf()
{
    VIUI *ui = UI();
    int col[2];
    unsigned short s[64];
    if (GetFocus() != this)
        m_window.ResetTickPulse();
    ui->Begin2D();
    col[0] = m_window.Width() / 3;
    col[1] = col[0] * 2;
    m_window.Draw();
    m_window.TextBox(m_text, 40, 1, m_window.Width() - 80, m_rows - 2, VIUICOLOR_0, 8);
    switch (m_prompt)
    {
    case VIMESSAGEPROMPT_OK:
        VIString::snprintf16(s, sizeof(s), "|q%S|w", Word(ui, 786));
        m_window.TextBox(s, 0, m_rows - 1, m_window.Width(), 1, VIUICOLOR_0, 8);
        break;
    case VIMESSAGEPROMPT_OKCANCEL:
        VIString::snprintf16(s, sizeof(s), m_sel == 0 ? "|q%S|w" : "%S", Word(ui, 786));
        m_window.TextBox(s, col[0] - 100, m_rows - 1, 200, 1, VIUICOLOR_0, 8);
        VIString::snprintf16(s, sizeof(s), m_sel == 1 ? "|q%S|w" : "%S", Word(ui, 213));
        m_window.TextBox(s, col[1] - 100, m_rows - 1, 200, 1, VIUICOLOR_0, 8);
        break;
    case VIMESSAGEPROMPT_YESNO:
        VIString::snprintf16(s, sizeof(s), m_sel == 0 ? "|q%S|w" : "%S", Word(ui, 21));
        m_window.TextBox(s, col[0] - 100, m_rows - 1, 200, 1, VIUICOLOR_0, 8);
        VIString::snprintf16(s, sizeof(s), m_sel == 1 ? "|q%S|w" : "%S", Word(ui, 22));
        m_window.TextBox(s, col[1] - 100, m_rows - 1, 200, 1, VIUICOLOR_0, 8);
        break;
    }
    ui->End2D();
}

// 0x002F2188.  Left/right move the selection, Enter or X closes.
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

// 0x002F22D8.
void VIWndMessage::OnKeyUp(int key)
{
}

// 0x002F22E0.  Gaining the focus opens the box; a two-choice box starts on the
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

// 0x002F23A0.
void VIWndMessage::Setup(const char *title, const char *text, VIMESSAGEPROMPT prompt,
                         const VIVect2I &center, int width)
{
    unsigned short s[256];
    VIString::str8to16(s, sizeof(s), text);
    Setup(title, s, prompt, center, width);
}

// 0x002F2418.
void VIWndMessage::Setup(const char *title, const unsigned short *text, VIMESSAGEPROMPT prompt,
                         const VIVect2I &center, int width)
{
    unsigned short s[256];
    VIString::str8to16(s, sizeof(s), title);
    Setup(s, text, prompt, center, width);
}

// 0x002F2498.
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
