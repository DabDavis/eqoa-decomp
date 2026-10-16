// viwndcontroller.cpp (name ours) -- SETUP 0x0056FF80 .. 0x00570F20.  New in
// Frontiers: a one-panel notice (WORDS.TXT 785 title, 1814 text, 786 the "|x"
// prompt) that X or Enter dismisses.  The method names follow VIWnd's.
#include "setup.h"

// One WORDS.TXT entry in the current language (MCERR's helper).
inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x0056FF80.
VIWndController::VIWndController()
{
}

// 0x0056FFC0.
VIWndController::~VIWndController()
{
}

// 0x00570010.
int VIWndController::Init()
{
    return 0;
}

// 0x00570018.
void VIWndController::Free()
{
}

// 0x00570020.  The window is re-initialised and re-opened every frame.
void VIWndController::OnDrawSelf()
{
    VIUI *ui = UI();
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 449), (VIAlphaBlendMode)3, 1.0f);
    m_window.Draw();
    int rows = m_window.CountLines(Word(UI(), 1814), m_window.Width() - 80);
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 450, rows + 3, 0);
    m_window.SetTitle(Word(UI(), 785));
    m_window.Open(true);
    m_window.TextBox(Word(UI(), 1814), 40, 1, m_window.Width() - 80, rows, VIUICOLOR_0, 8);
    unsigned short buf[256];
    VIString::snprintf16(buf, sizeof(buf), "|x %S", Word(ui, 786));
    m_window.Text(buf, 40, rows + 2, VIUICOLOR_0, 0);
    ui->End2D();
}

// 0x005701D8.
void VIWndController::OnKeyUp(int key)
{
}

// 0x005701E0.  Enter or X.
void VIWndController::OnKeyDown(int key)
{
    GetAppPtr();
    if (key == 13 || key == 134)
    {
        UI()->SoundPlay((VIUISoundId)15);
        SetFocus(0, true);
    }
}

// 0x00570248.  `ui` is dead, but its `lui` stays at the top for the two UI()
// loads cse folds onto it.
void VIWndController::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIUI *ui = UI();
    if (msg == 295 && p2 == (int)this)
    {
        m_window.Close(true);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 450, 4, 0);
        m_window.SetTitle(Word(UI(), 785));
        m_window.Open(true);
    }
}
