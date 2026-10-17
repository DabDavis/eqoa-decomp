#line 1 "C:/code-frontiers-gm/game/VIWndTell.cpp"
// VIWndTell -- CLIENT 0x00C991B0 .. 0x00C99C50 (the TU runs to its __tf at 0x00C9A828).
// The /tell form: a name and a line of text, each typed in a VIWndEdit, then Send, which
// hands "/t <name> <text>" to the chat line.  With no name it says so in a VIWndMessage.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C991B0.
VIWndTell::VIWndTell()
{
    m_edit = 0;
}

// 0x00C991F8.
VIWndTell::~VIWndTell()
{
    Free();
}

// 0x00C99258.
int VIWndTell::Init()
{
    Free();
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->SetVisible(false);
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    m_msg.Init();
    m_msg.SetVisible(false);
    AddChild(&m_msg);
    return 0;
}

// 0x00C992E8.
void VIWndTell::Free()
{
    if (m_edit)
    {
        RemoveChild(&m_msg);
        m_msg.Free();
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    m_mode = 0;
    m_name[0] = 0;
    m_text[0] = 0;
}

// 0x00C99368.
void VIWndTell::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short buf[128];
    ui->Begin2D();
    m_window.Draw();
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 881));
    m_window.Text(buf, 40, 1, (VIUICOLOR)(m_mode == 0), 0);
    m_window.TextBox(m_window.T(m_name), 140, 1, m_window.Width() - 180, 1, (VIUICOLOR)0, 6);
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 522));
    m_window.Text(buf, 40, 3, (VIUICOLOR)(m_mode == 1), 0);
    m_window.TextBox(m_window.T(m_text), 140, 3, m_window.Width() - 180, 1, (VIUICOLOR)0, 6);
    m_window.Text(m_window.T((unsigned short)885), 40, 5, (VIUICOLOR)(m_mode == 2), 0);
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 161));
    m_window.Text(buf, 40, 7, (VIUICOLOR)0, 0);
    m_window.TickR(36, m_mode * 2 + 1, true);
    ui->End2D();
}

// 0x00C99578.
void VIWndTell::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->GetP2EFD0();
    switch (key)
    {
    case 19:
    case 138:
        Up();
        break;
    case 20:
    case 139:
        Down();
        break;
    case 13:
    case 134:
        Select();
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel6F4();
        break;
    case 146:
        VIWnd::OnKeyDown(146);
        break;
    }
}

// 0x00C996B0.
void VIWndTell::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_edit && (VIWnd *)p1 != &m_msg)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 500, 8, 0);
            m_window.SetTitle(m_window.T((unsigned short)425));
            m_window.Open(true);
            m_text[0] = 0;
            m_edit->SetVisible(false);
            m_msg.SetVisible(false);
        }
        break;
    case 259:
        if (!m_edit)
            break;
        TakeText();
    case 260:
        if (m_edit)
        {
            app->SetActiveWindow(this);
            m_edit->SetVisible(false);
        }
        break;
    case 307:
        app->SetActiveWindow(this);
        break;
    case 261:
        break;
    }
}

// 0x00C997F8.
unsigned short *VIWndTell::GetName()
{
    return m_name;
}

// 0x00C99800.
void VIWndTell::SetName(const unsigned short *s)
{
    VIString::strcpy16(m_name, sizeof(m_name), s);
}

// 0x00C99828.
unsigned short *VIWndTell::GetText()
{
    return m_text;
}

// 0x00C99830.
void VIWndTell::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x00C99858.
void VIWndTell::Open(const unsigned short *name)
{
    m_name[0] = 0;
    if (name && name[0])
    {
        VIString::strcpy16(m_name, sizeof(m_name), name);
        m_mode = 1;
    }
    else
        m_mode = 0;
}

// 0x00C998A8: empty.
void VIWndTell::Close()
{
}

// 0x00C998B0.
void VIWndTell::Down()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_mode < 2)
    {
        m_mode++;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C99920.
void VIWndTell::Up()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_mode > 0)
    {
        m_mode--;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C99988.
void VIWndTell::Select()
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_mode)
    {
    case 0:
        m_edit->SetRows(1);
        m_edit->SetMultiLine(0);
        m_edit->SetCenter(m_window.Center());
        m_edit->SetMaxLength(24);
        m_edit->SetText(m_name);
        m_edit->SetVisible(true);
        app->SetActiveWindow(m_edit);
        break;
    case 1:
        m_edit->SetRows(2);
        m_edit->SetMultiLine(0);
        m_edit->SetCenter(m_window.Center());
        m_edit->SetMaxLength(64);
        m_edit->SetText(m_text);
        m_edit->SetVisible(true);
        app->SetActiveWindow(m_edit);
        break;
    case 2:
    {
        unsigned short sep[2] = { ' ', 0 };
        unsigned short buf[120];
        if (GetName()[0] == 0)
        {
            m_msg.Setup(Word(UI(), 1631), Word(UI(), 1632), (VIMESSAGEPROMPT)0,
                        m_window.Center(), 300);
            m_msg.SetVisible(true);
            app->SetActiveWindow(&m_msg);
        }
        else
        {
            VIString::str8to16(buf, sizeof(buf), "/t ");
            VIString::strcat16(buf, sizeof(buf), GetName());
            VIString::strcat16(buf, sizeof(buf), sep);
            VIString::strcat16(buf, sizeof(buf), GetText());
            app->ChatLine(buf);
            app->PlaySound(8);
            app->ClosePanel6F4();
        }
        break;
    }
    }
}

// 0x00C99BD8.
void VIWndTell::TakeText()
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_mode)
    {
    case 0:
        VIString::strcpy16(m_name, sizeof(m_name), m_edit->m_text);
        m_mode = 1;
        break;
    case 1:
        VIString::strcpy16(m_text, sizeof(m_text), m_edit->m_text);
        m_mode = 2;
        break;
    }
}
