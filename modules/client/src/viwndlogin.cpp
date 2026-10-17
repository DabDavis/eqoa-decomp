#line 1 "C:/code-frontiers-gm/game/VIWndLogin.cpp"
// VIWndLogin (name the RTTI's) -- CLIENT 0x00C6AA30 .. 0x00C6BDE0.  The account window:
// the name and password rows open a VIWndEdit; the third row connects
// (VIClient::ShowLoginWindow).  Method names ours.
#include "game.h"

// 0x00C6AA30.
VIWndLogin::VIWndLogin()
{
    m_edit = 0;
}

// 0x00C6AA78.
VIWndLogin::~VIWndLogin()
{
    Free();
}

// 0x00C6AAD8.
int VIWndLogin::Init(const char *user, const char *pass)
{
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->m_visible = 0;
    m_edit->SetCenter(VIVect2I(320, 310));
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    VIString::strcpy8(m_user, sizeof(m_user), user);
    VIString::strcpy8(m_pass, sizeof(m_pass), pass);
    m_field = 0;
    return 0;
}

// 0x00C6AB98.
void VIWndLogin::Free()
{
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
}

// 0x00C6ABF0.  The dimmed screen, the window, its three rows (the password masked) and
// the tick at the current one.
void VIWndLogin::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short buf[64];
    if (app->m_p2BD40 != this)
        m_window.ResetTickPulse();
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 450), (VIAlphaBlendMode)3, 1.0f);
    m_window.Draw();
    VIString::snprintf16(buf, sizeof(buf), "%S:", ui->m_words.GetString(904, ui->m_language));
    m_window.Text(buf, 40, 1, (VIUICOLOR)(m_field == 0), 0);
    VIString::snprintf16(buf, sizeof(buf), "%S:", ui->m_words.GetString(901, ui->m_language));
    m_window.Text(buf, 40, 2, (VIUICOLOR)(m_field == 1), 0);
    m_window.Text(m_window.T((unsigned short)900), 40, 3, (VIUICOLOR)(m_field == 2), 0);
    m_window.TextBox(m_window.T(m_pass), 150, 2, m_window.Width() - 190, 1, (VIUICOLOR)0, 194);
    m_window.TextBox(m_window.T(m_user), 150, 1, m_window.Width() - 190, 1, (VIUICOLOR)0, 66);
    VIString::snprintf16(buf, sizeof(buf), "|x%S", ui->m_words.GetString(212, ui->m_language));
    m_window.Text(buf, 40, 5, (VIUICOLOR)0, 0);
    m_window.TickR(36, m_field + 1, true);
    ui->End2D();
}

// 0x00C6AE40.
void VIWndLogin::OnKeyUp(int key)
{
}

// 0x00C6AE48.  Up and down move between the rows; Enter edits a row or connects.
void VIWndLogin::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 19:
    case 138:
        if (m_field > 0)
        {
            m_field--;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        break;
    case 20:
    case 139:
        if (m_field < 2)
        {
            m_field++;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        break;
    case 13:
    case 134:
        if (m_field == 2)
        {
            app->ShowLoginWindow();
            break;
        }
        m_edit->SetX18C(m_field != 0);
        m_edit->SetRows(1);
        m_edit->SetMaxLength(32);
        m_edit->SetText(m_field == 0 ? m_user : m_pass);
        m_edit->SetTitle((const unsigned short *)(m_field == 0
                             ? UI()->m_words.GetString(904, UI()->m_language)
                             : UI()->m_words.GetString(901, UI()->m_language)));
        m_edit->m_visible = 1;
        app->SetActiveWindow(m_edit);
        break;
    }
}

// 0x00C6AFE0.  The edit closing (259 done, 260 cancelled), and the focus arriving.
void VIWndLogin::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if (p2 == (int)this && p1 != (int)m_edit)
        {
            VIVect2I c(320, 224);
            m_window.Close(true);
            m_window.Init(UI(), VIVect2I(0, 0), c, 350, 6, 0);
            m_window.SetTitle(m_window.T((unsigned short)900));
            m_window.Open(false);
            m_field = 2;
            m_info.Close(true);
            m_info.Init(UI(), VIVect2I(0, 0), c, 330, 3, 0);
            m_info.SetTitle(m_info.T((unsigned short)271));
            m_info.Open(false);
        }
        break;
    case 260:
        m_edit->m_visible = 0;
        app->SetActiveWindow(this);
        break;
    case 259:
        m_edit->m_visible = 0;
        VIString::str16to8(m_field ? m_pass : m_user, 64, m_edit->m_text);
        app->SetActiveWindow(this);
        break;
    }
}
