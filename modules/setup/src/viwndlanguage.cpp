// viwndlanguage.cpp (name ours) -- SETUP 0x00574238 .. 0x00575300.  New in
// Frontiers: the language picker.  The method names follow VIWnd's.
#include "setup.h"

// One WORDS.TXT entry in the current language (MCERR's helper).
inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The language is VIGameOptions +0x96 (vi.h's m_region).  Through an inline
// setter, as the original evidently stored it: the parameter copy keeps the
// `lw` of m_sel that a direct store narrows to `lbu`.
static inline void SetLanguage(VIGameOptions *o, int l)
{
    o->m_region = l;
}

// Filled by Init, not initialised: the three names are stored there each time.
static const char *sLanguageNames[3];

// 0x00574238.
VIWndLanguage::VIWndLanguage()
{
}

// 0x00574278.
VIWndLanguage::~VIWndLanguage()
{
}

// 0x005742C8.
int VIWndLanguage::Init()
{
    m_sel = 0;
    sLanguageNames[0] = "English";
    sLanguageNames[1] = "Fran\xe7" "ais";
    sLanguageNames[2] = "Deutsch";
    m_count = 0;
    return 0;
}

// 0x00574308.
void VIWndLanguage::Free()
{
}

// 0x00574310.
void VIWndLanguage::OnDrawSelf()
{
    VIUI *ui = UI();
    if (m_count > 1)
    {
        if (--m_count == 1)
        {
            SetLanguage(VIGetGameOptions(), m_sel + 1);
            UI()->ReloadWords();
            SetFocus(0, true);
            return;
        }
    }
    ui->Begin2D();
    if (m_count == 0)
    {
        ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 513), (VIAlphaBlendMode)3, 1.0f);
        m_window.Draw();
        m_window.DrawFrame(40, 1, m_window.Width() - 80, 1, 96);
        m_window.TextBox(m_window.T(sLanguageNames[m_sel]), 40, 1, m_window.Width() - 80, 1, VIUICOLOR_0, 8);
        unsigned short buf[256];
        VIString::snprintf16(buf, sizeof(buf), "|x %S", Word(ui, 212));
        m_window.Text(buf, 40, 3, VIUICOLOR_0, 0);
    }
    ui->End2D();
}

// 0x005744B8.
void VIWndLanguage::OnKeyUp(int key)
{
}

// 0x005744C0.  17/136 (left) and 18/137 (right) step the choice, 13/134
// (Enter, X) takes it.
void VIWndLanguage::OnKeyDown(int key)
{
    GetAppPtr();
    switch (key)
    {
    case 17:
    case 136:
        if (m_count == 0)
        {
            m_sel = (m_sel + 2) % 3;
            UI()->SoundPlay((VIUISoundId)11);
            m_window.ResetTickPulse();
        }
        break;
    case 18:
    case 137:
        if (m_count == 0)
        {
            m_sel = (m_sel + 1) % 3;
            UI()->SoundPlay((VIUISoundId)11);
            m_window.ResetTickPulse();
        }
        break;
    case 13:
    case 134:
        m_count = 60;
        UI()->SoundPlay((VIUISoundId)15);
        break;
    }
}

// 0x005745E0.
void VIWndLanguage::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIUI *ui = UI();
    if (msg == 295 && p2 == (int)this)
    {
        m_window.Close(true);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 256), 300, 4, 0);
        m_window.SetTitle(Word(ui, 1614));
        m_window.Open(false);
        m_sel = (char)VIGetGameOptions()->m_region - 1;   // `lb`: signed
        if (m_sel < 0)
            m_sel = 0;
        else if (m_sel > 2)
            m_sel = 2;
    }
}
