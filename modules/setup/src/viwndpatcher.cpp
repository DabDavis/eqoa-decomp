// viwndpatcher.cpp (name ours) -- SETUP 0x00578DF8 .. 0x0057A988.  The patch
// download screen: progress, status, the release notes, and the cancel prompt.
#include "setup.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00578DF8.
VIWndPatcher::VIWndPatcher()
{
}

// 0x00578E58.
VIWndPatcher::~VIWndPatcher()
{
}

// 0x00578ED8.
int VIWndPatcher::Init()
{
    m_sel = 0;
    m_message.Init();
    m_message.SetVisible(false);
    AddChild(&m_message);
    m_notes.Init();
    m_notes.SetVisible(false);
    AddChild(&m_notes);
    m_state = 0;
    return 0;
}

// 0x00578F50.
void VIWndPatcher::Free()
{
    RemoveChild(&m_notes);
    RemoveChild(&m_message);
    m_message.Free();
}

// 0x00578F98.  Phase -1 returns with the 2D pass still open.
void VIWndPatcher::OnDrawSelf()
{
    VIUI *ui = UI();
    const unsigned short *status = 0;
    ui->Begin2D();
    DrawFakeSplashInterface();
    m_window3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 430, 8, 0);
    switch (m_state)
    {
    case -1:
        return;
    case 0:
        status = Word(UI(), 757);
        break;
    case 1:
        status = Word(UI(), 754);
        break;
    case 3:
        status = Word(UI(), 812);
        break;
    case 4:
        status = Word(UI(), 764);
        m_window3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 430, 9, 0);
        m_window3.Open(true);
        break;
    case 5:
        status = Word(UI(), 792);
        break;
    case 6:
        status = Word(UI(), 814);
        break;
    case 2:
    case 7:
        status = Word(UI(), 768);
        break;
    }
    m_window3.Draw();
    unsigned short buf[512];
    if (m_state == 4)
    {
        if (*m_notes.Message())
            m_window3.Text(Word(UI(), 813), 40, 5, (VIUICOLOR)(m_sel == 0), 0);
        else
        {
            VIString::str8to16(buf, sizeof(buf), "|-");
            VIString::strcat16(buf, sizeof(buf), Word(UI(), 813));
            m_window3.Text(buf, 40, 5, VIUICOLOR_0, 0);
            m_sel = 1;
        }
        m_window3.Text(m_restartRequired ? Word(UI(), 793) : Word(UI(), 790), 40, 6,
                       (VIUICOLOR)(m_sel == 1), 0);
        VIString::str8to16(buf, sizeof(buf), "|x ");
        VIString::strcat16(buf, sizeof(buf), Word(UI(), 212));
        if (m_restartRequired)
        {
            VIString::strcat16(buf, sizeof(buf), "   |-|t ");
            VIString::strcat16(buf, sizeof(buf), Word(UI(), 750));
            m_window3.Text(buf, 40, 8, VIUICOLOR_0, 0);
        }
        else
        {
            VIString::strcat16(buf, sizeof(buf), "   |t ");
            VIString::strcat16(buf, sizeof(buf), Word(UI(), 750));
            m_window3.Text(buf, 40, 8, VIUICOLOR_0, 0);
        }
        m_window3.TickR(36, m_sel + 5, true);
    }
    else
        m_window3.TextBox(Word(UI(), 763), 40, 5, m_window3.Width() - 80, 2, VIUICOLOR_0, 8);

    // The "..." after a busy status: one more dot every 15 frames, up to three.
    static char sDots = 0;
    static char sTick = 15;
    if (--sTick <= 0)
    {
        sTick = 15;
        if (++sDots == 3)
            sDots = 0;
    }
    if (status)
    {
        VIString::strcpy16(buf, sizeof(buf), status);
        switch (m_state)
        {
        case 3:
        case 5:
        case 6:
            for (char i = 0; i < sDots + 1; i++)
                VIString::strcat16(buf, sizeof(buf), ".");
            break;
        }
        // Formats over the dots it just added: `status`, not `buf`.
        VIString::snprintf16(buf, sizeof(buf), "%S %S", Word(UI(), 803), status);
    }
    else
        VIString::snprintf16(buf, sizeof(buf), "%S", Word(UI(), 803));
    m_window3.TextBox(buf, 40, 1, m_window3.Width() - 80, 2, VIUICOLOR_0, 0);
    m_window3.DrawProgress(40, 3, m_window3.Width() - 80, m_progress);
    if (m_state == 2)
    {
        m_window2.Draw();
        m_window2.TextBox(Word(UI(), 769), 40, 1, m_window2.Width() - 80, 3, VIUICOLOR_0, 8);
        VIString::str8to16(buf, sizeof(buf), "|x ");
        VIString::strcat16(buf, sizeof(buf), Word(UI(), 793));
        m_window2.Text(buf, 40, 5, VIUICOLOR_0, 0);
    }
    if (m_state == 7)
    {
        m_window2.Draw();
        m_window2.TextBox(Word(UI(), 787), 40, 1, m_window2.Width() - 80, 3, VIUICOLOR_0, 8);
        VIString::str8to16(buf, sizeof(buf), "|x ");
        VIString::strcat16(buf, sizeof(buf), Word(UI(), 793));
        m_window2.Text(buf, 40, 5, VIUICOLOR_0, 0);
    }
    if (m_showMessage)
    {
        m_showMessage = 0;
        m_confirm = 0;
        UI()->SoundPlay((VIUISoundId)15);
        if (VIGetExpansionFlags() & 2)
            VIString::snprintf16(buf, sizeof(buf), Word(UI(), 770), Word(UI(), 1803));
        else
            VIString::snprintf16(buf, sizeof(buf), Word(UI(), 770), Word(UI(), 1802));
        m_message.Setup(Word(UI(), 785), buf, (VIMESSAGEPROMPT)0, m_window3.Center(), 500);
        m_message.SetVisible(true);
        SetFocus(&m_message, true);
    }
    ui->End2D();
}

// 0x00579750.  Keyboard and pad: up 19/138, down 20/139, select 13/134 (Enter,
// cross), cancel 27/132 (Escape, triangle).
void VIWndPatcher::OnKeyDown(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        if (m_state == 4 && *m_notes.Message())
        {
            if (m_sel > 0)
            {
                m_sel--;
                m_window3.ResetTickPulse();
                UI()->SoundPlay((VIUISoundId)11);
            }
            else
                UI()->SoundPlay((VIUISoundId)9);
        }
        break;
    case 20:
    case 139:
        if (m_state == 4 && *m_notes.Message())
        {
            if (m_sel < 1)
            {
                m_sel++;
                m_window3.ResetTickPulse();
                UI()->SoundPlay((VIUISoundId)11);
            }
            else
                UI()->SoundPlay((VIUISoundId)9);
        }
        break;
    case 13:
    case 134:
        if (m_state == 4 && !m_showMessage)
        {
            if (m_sel == 0)
            {
                m_notes.SetVisible(true);
                SetFocus(&m_notes, true);
                UI()->SoundPlay((VIUISoundId)15);
            }
            else if (m_restartRequired)
                SetFocus(0, true);
            else
            {
                UI()->SoundPlay((VIUISoundId)11);
                SetFocus(0, true);
            }
        }
        else if (m_state == 2 || m_state == 7)
        {
            m_restartRequired = 1;
            UI()->SoundPlay((VIUISoundId)9);
            SetFocus(0, true);
        }
        break;
    case 27:
    case 132:
        if (m_state == 4 && !m_showMessage && !m_restartRequired)
        {
            m_confirm = 1;
            UI()->SoundPlay((VIUISoundId)15);
            m_message.Setup(Word(UI(), 785), Word(UI(), 810), (VIMESSAGEPROMPT)2,
                            m_window3.Center(), 400);
            m_message.SetVisible(true);
            SetFocus(&m_message, true);
        }
        break;
    }
}

// 0x00579A20.
void VIWndPatcher::OnKeyUp(int key)
{
}

// 0x00579A28.  307 is the message box closing.
void VIWndPatcher::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    switch (msg)
    {
    case 295:
        if (p2 == (int)this && p1 != (int)&m_notes && p1 != (int)&m_message)
        {
            m_window.Init(UI(), VIVect2I(0, 350), VIVect2I(304, 360), 440, 1, 0);
            m_window.Open(true);
            m_window3.Close(true);
            m_window3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 430, 4, 0);
            m_window3.Open(false);
            m_window3.SetTitle(Word(UI(), 811));
            m_window2.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 480, 6, 0);
            m_window2.Open(false);
            m_window2.SetTitle(Word(UI(), 767));
            m_notes.Clear();
            m_sel = 0;
            m_confirm = -1;
            m_progress = 0.0f;
            m_restartRequired = 0;
            m_showMessage = 0;
            m_restartShown = 0;
        }
        if (p2 == (int)this && p1 == (int)&m_notes && m_restartRequired == 1 && !m_restartShown)
        {
            m_showMessage = 1;
            m_restartShown = 1;
        }
        break;
    case 307:
        if (m_confirm == 0)
            SetFocus(this, true);
        else if (m_confirm == 1)
        {
            if (m_message.m_sel == 1)
                SetFocus(this, true);
            else
            {
                m_state = -1;
                SetFocus(0, true);
            }
        }
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00579CA8.
void VIWndPatcher::SetProgress(float f)
{
    m_progress = f;
}

// 0x00579CB0.
void VIWndPatcher::AppendMessage(const unsigned short *s)
{
    m_notes.AppendMessage(s);
}

// 0x00579CD0.
void VIWndPatcher::ShowPatchMessage()
{
    m_sel = 1;
    m_notes.SetVisible(true);
    SetFocus(&m_notes, true);
    UI()->SoundPlay((VIUISoundId)15);
}

// 0x00579D10.  The title screen's menu bar behind the patch window, with the
// first entry lit.  VIWndDnas draws the same thing.
void VIWndPatcher::DrawFakeSplashInterface()
{
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, 1.0f);
    VIVect2I pos;
    VIUI *ui = UI();
    ui->Blit(VIHashResourceID("splash00.tga"), pos, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->Blit(VIHashResourceID("splash10.tga"), VIVect2I(320, 0), (VIAlphaBlendMode)0, 1.0f, 0);
    VISplashFxRender(1);
    VIUIWindow &w = m_window;
    int third = w.Width() / 3;
    int x[5] = { 0, third, third * 2, third * 3, third * 4 };
    unsigned short buf[256];
    VIString::str8to16(buf, sizeof(buf), "|>");
    VIString::strcat16(buf, sizeof(buf), Word(UI(), 790));
    VIString::strcat16(buf, sizeof(buf), "|<");
    w.TextBox(w.T(buf), x[0] - 10, 0, x[1] - x[0] + 20, 1, yellow, 9);
    w.TextBox(Word(UI(), 1635), x[1] - 20, 0, x[2] - x[1] + 20, 1, white, 9);
    w.TextBox(Word(UI(), 422), x[2] - 10, 0, x[3] - x[2] + 20, 1, white, 9);
}
