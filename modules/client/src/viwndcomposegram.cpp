#line 1 "C:/code-frontiers-gm/game/VIWndComposeGram.cpp"
// viwndcomposegram.cpp -- CLIENT 0x00C3A580 .. 0x00C3C6A8: writing a tell-gram (the in-game
// mail): To, Subject and the body, each typed in a VIWndEdit, then Send, through the gram
// client (VIClient's slot 7, VIClientPart2EFD0).  The same shape as VIWndCSRequest, and a
// status window that reports what the gram client is doing; a refusal is a message box
// (the embedded VIWndMessage).  Method names ours.
#include "game.h"

// One WORDS.TXT entry in the current language, as SETUP's windows read them.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C3A580.
VIWndComposeGram::VIWndComposeGram()
{
    m_mbox = 0;
    m_edit = 0;
}

// 0x00C3A5D8.
VIWndComposeGram::~VIWndComposeGram()
{
    Free();
}

// 0x00C3A648.  The editor for the fields, the message box (hidden) and the error box.
int VIWndComposeGram::Init(int reply)
{
    Free();
    m_reply = reply;
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->SetVisible(false);
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    m_message.Init();
    m_message.SetVisible(false);
    AddChild(&m_message);
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        return -1;
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    m_mbox->Init(1, 1);
    m_mbox->m_pos = VIVect2I(100, 100);
    return 0;
}

// 0x00C3A748.
void VIWndComposeGram::Free()
{
    if (m_edit)
    {
        RemoveChild(&m_message);
        m_message.Free();
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    m_sel = 0;
    m_to[0] = 0;
    m_subject[0] = 0;
    m_body[0] = 0;
}

// The "..." after a status line: one dot more every 15 frames, three at most.
static char sDots;    // 0x00CD7540
static char sTicks;   // 0x00CD7541

// 0x00C3A808.
void VIWndComposeGram::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart2EFD0 *gram = app->GetP2EFD0();
    if (--sTicks <= 0)
    {
        sTicks = 15;
        if (++sDots == 3)
            sDots = 0;
    }

    unsigned short buf[128];
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 332), (VIAlphaBlendMode)3, 1.0f);
    m_window.Draw();
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 881));
    m_window.Text(buf, 40, 1, (VIUICOLOR)(m_sel == 0), 0);
    m_window.TextBox(m_window.T(m_to), 140, 1, m_window.Width() - 180, 1, (VIUICOLOR)0, 6);
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 883));
    m_window.Text(buf, 40, 3, (VIUICOLOR)(m_sel == 1), 0);
    m_window.TextBox(m_window.T(m_subject), 140, 3, m_window.Width() - 180, 1, (VIUICOLOR)0, 6);
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 522));
    m_window.Text(buf, 40, 5, (VIUICOLOR)(m_sel == 2), 0);
    m_window.TextBox(m_window.T(m_body), 140, 5, m_window.Width() - 180, 5, (VIUICOLOR)0, 6);
    VIString::snprintf16(buf, sizeof(buf), "%S", Word(ui, 885));
    m_window.Text(buf, 40, 11, (VIUICOLOR)(m_sel == 3), 0);
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 213));
    m_window.Text(buf, 40, 13, (VIUICOLOR)0, 0);
    switch (m_sel)
    {
    case 0:
        m_window.TickR(36, 1, true);
        break;
    case 1:
        m_window.TickR(36, 3, true);
        break;
    case 2:
        m_window.TickR(36, 5, true);
        break;
    case 3:
        m_window.TickR(36, 11, true);
        break;
    }

    unsigned short back[32];
    VIString::snprintf16(back, sizeof(back), "|t%S", Word(ui, 161));
    char d;
    switch (gram->GetState())
    {
    case 3:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 886));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.Draw();
        m_status.TextBox(buf, 40, 1, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        m_status.TextBox(back, 40, 3, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        m_x7B4 = 1;
        break;
    case 1:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 844));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.Draw();
        m_status.TextBox(buf, 40, 1, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        m_status.TextBox(back, 40, 3, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        break;
    case 2:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 845));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.Draw();
        m_status.TextBox(buf, 40, 1, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        m_status.TextBox(back, 40, 3, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        break;
    case 4:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 887));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.Draw();
        m_status.TextBox(buf, 40, 1, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        m_status.TextBox(back, 40, 3, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        break;
    case -1:
        if (m_x7B4 != 1)
            break;
        m_x7B4 = -1;
        m_message.Setup(Word(ui, 889), Word(ui, 890), (VIMESSAGEPROMPT)0, VIVect2I(320, 180), 300);
        m_message.SetVisible(true);
        app->SetActiveWindow(&m_message);
        break;
    case -2:
        if (m_x7B4 != 1)
            break;
        m_x7B4 = -2;
        m_message.Setup(Word(ui, 889), Word(ui, 891), (VIMESSAGEPROMPT)0, VIVect2I(320, 180), 300);
        m_message.SetVisible(true);
        app->SetActiveWindow(&m_message);
        break;
    case -3:
        if (m_x7B4 != 1)
            break;
        m_x7B4 = -3;
        m_message.Setup(Word(ui, 889), Word(ui, 892), (VIMESSAGEPROMPT)0, VIVect2I(320, 180), 300);
        m_message.SetVisible(true);
        app->SetActiveWindow(&m_message);
        break;
    case -4:
        if (m_x7B4 != 1)
            break;
        m_x7B4 = -4;
        m_message.Setup(Word(ui, 889), m_window.T((unsigned short)1806), (VIMESSAGEPROMPT)0,
                        VIVect2I(320, 180), 300);
        m_message.SetVisible(true);
        app->SetActiveWindow(&m_message);
        break;
    case -5:
        if (m_x7B4 != 1)
            break;
        m_x7B4 = -3;
        m_message.Setup(Word(ui, 889), Word(ui, 893), (VIMESSAGEPROMPT)0, VIVect2I(320, 180), 300);
        m_message.SetVisible(true);
        app->SetActiveWindow(&m_message);
        break;
    case 0:
        if (!m_x7B4)
            break;
        m_x7B4 = 0;
        if (m_reply)
        {
            app->PlaySound(8);
            GetParent()->OnMessage(this, 309, 0, 0);
        }
        else
        {
            app->PlaySound(8);
            app->ClosePanel6E0();
        }
        break;
    }
    ui->End2D();
}

// 0x00C3B0C8.  Nothing moves while the gram client is busy.
void VIWndComposeGram::OnKeyDown(int key)
{
    VIClientPart2EFD0 *gram = ((VIClient *)GetAppPtr())->GetP2EFD0();
    switch (key)
    {
    case 19:
    case 138:
        if (gram->GetState() > 0)
            break;
        Prev();
        break;
    case 20:
    case 139:
        if (gram->GetState() > 0)
            break;
        Next();
        break;
    case 13:
    case 134:
        if (gram->GetState() > 0)
            break;
        Select();
        break;
    case 27:
    case 132:
        if (!VIString::strlen16(m_body))
            Cancel();
        else
            ShowMessage(m_window.T((unsigned short)866), 1, 0, 0);
        break;
    }
}

// 0x00C3B238.  259: the editor closed with a text; 260: the editor or the message box
// closed; 268: the message box's choice; 295: the focus arrived; 307: the error box closed.
void VIWndComposeGram::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 != this || (VIWnd *)p1 == m_edit || (VIWnd *)p1 == m_mbox
            || (VIWnd *)p1 == &m_message || (VIWnd *)p1 == this)
            break;
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 500, 14, 0);
        if (m_reply == 1)
            m_window.SetTitle(m_window.T((unsigned short)894));
        else
            m_window.SetTitle(m_window.T((unsigned short)895));
        m_window.Open(true);
        m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 375, 4, 0);
        m_status.SetTitle(m_status.T((unsigned short)271));
        m_status.Open(true);
        m_x7B4 = 0;
        m_mbox->SetVisible(false);
        m_edit->SetVisible(false);
        m_message.SetVisible(false);
        if (m_reply == 1)
            m_sel = 2;
        else
            m_sel = 0;
        m_subject[0] = 0;
        m_body[0] = 0;
        m_to[0] = 0;
        break;
    case 259:
        if (m_edit)
            TakeText();
    case 260:
        if (m_edit)
        {
            app->SetActiveWindow(this);
            m_edit->SetVisible(false);
        }
        if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 268:
        if (w == m_mbox)
            Cancel();
        break;
    case 307:
        if (m_x7B4)
            m_x7B4 = 0;
        app->SetActiveWindow(this);
        break;
    }
}

// 0x00C3B4A0.
unsigned short *VIWndComposeGram::GetTo()
{
    return m_to;
}

// 0x00C3B4A8.
void VIWndComposeGram::SetTo(const unsigned short *s)
{
    VIString::strcpy16(m_to, sizeof(m_to), s);
}

// 0x00C3B4D0.
unsigned short *VIWndComposeGram::GetSubject()
{
    return m_subject;
}

// 0x00C3B4D8.
void VIWndComposeGram::SetSubject(const unsigned short *s)
{
    VIString::strcpy16(m_subject, sizeof(m_subject), s);
}

// 0x00C3B500.
unsigned short *VIWndComposeGram::GetBody()
{
    return m_body;
}

// 0x00C3B508.
void VIWndComposeGram::SetBody(const unsigned short *s)
{
    VIString::strcpy16(m_body, sizeof(m_body), s);
}

// 0x00C3B530.  Back to the first field.
void VIWndComposeGram::Refresh()
{
    m_sel = 0;
}

// 0x00C3B538.
void VIWndComposeGram::End()
{
}

// 0x00C3B540.
void VIWndComposeGram::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel < 3)
    {
        m_sel++;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C3B5B0.
void VIWndComposeGram::Prev()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel > 0)
    {
        m_sel--;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C3B618.  Edit the field under the cursor, or send: the name is capitalised
// (CapitalizeName) and handed to the gram client with the subject and the text.
void VIWndComposeGram::Select()
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_sel)
    {
    case 0:
        m_edit->SetRows(1);
        m_edit->SetMultiLine(0);
        m_edit->SetCenter(m_window.Center());
        m_edit->SetMaxLength(24);
        m_edit->SetText(m_to);
        m_edit->SetVisible(true);
        app->SetActiveWindow(m_edit);
        break;
    case 1:
        m_edit->SetRows(1);
        m_edit->SetMultiLine(0);
        m_edit->SetCenter(m_window.Center());
        m_edit->SetMaxLength(32);
        m_edit->SetText(m_subject);
        m_edit->SetVisible(true);
        app->SetActiveWindow(m_edit);
        break;
    case 2:
        m_edit->SetRows(4);
        m_edit->SetMultiLine(1);
        m_edit->SetCenter(m_window.Center());
        m_edit->SetMaxLength(512);
        m_edit->SetText(m_body);
        m_edit->SetVisible(true);
        app->SetActiveWindow(m_edit);
        break;
    case 3:
        if (!VIString::strlen16(m_to))
            app->AddText(m_window.T((unsigned short)1632), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
        else if (!VIString::strlen16(m_subject))
            app->AddText(m_window.T((unsigned short)1808), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
        else if (!VIString::strlen16(m_body))
            app->AddText(m_window.T((unsigned short)873), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
        else
        {
            char to[24];
            VIString::str16to8(to, sizeof(to), m_to);
            CapitalizeName(to);
            app->GetP2EFD0()->SendGram(to, m_subject, m_body);
        }
        break;
    }
}

// 0x00C3B8B0.  The editor's text, back into the field it edited.
void VIWndComposeGram::TakeText()
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_sel)
    {
    case 0:
        VIString::strcpy16(m_to, sizeof(m_to), m_edit->m_text);
        break;
    case 1:
        VIString::strcpy16(m_subject, sizeof(m_subject), m_edit->m_text);
        break;
    case 2:
        VIString::strcpy16(m_body, sizeof(m_body), m_edit->m_text);
        break;
    }
}

// 0x00C3B950.  A reply goes back to the reader; a new gram closes the panel.
void VIWndComposeGram::Cancel()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_reply)
    {
        app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
    }
    else
    {
        app->PlaySound(8);
        app->ClosePanel6E0();
    }
}

// 0x00C3B9D8.  The message box over this window, with the focus.
void VIWndComposeGram::ShowMessage(const unsigned short *text, int prompt, int a, int b)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_mbox->Init(prompt, 1);
    m_mbox->SetText(text);
    m_mbox->m_p1 = a;
    m_mbox->m_p2 = b;
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}
