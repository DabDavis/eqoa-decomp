#line 1 "C:/code-frontiers-gm/game/VIWndClassMasteryPoints.cpp"
// viwndclassmasterypoints.cpp -- CLIENT 0x00C37B68 .. 0x00C38DA0: the Class Mastery points
// window: the points available, and the share of the experience earned that goes to
// points, Left/Right in steps of one (0..100), Enter sends it (sub-op 1).  Method names ours.
#include "game.h"

// One WORDS.TXT entry in the current language, as SETUP's windows read them.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C37B68.
VIWndClassMasteryPoints::VIWndClassMasteryPoints()
{
}

// 0x00C37BB0.
VIWndClassMasteryPoints::~VIWndClassMasteryPoints()
{
}

// 0x00C37C10.
int VIWndClassMasteryPoints::Init()
{
    return 0;
}

// 0x00C37C18.
void VIWndClassMasteryPoints::Free()
{
}

// 0x00C37C20.
void VIWndClassMasteryPoints::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short buf[256];
    ui->Begin2D();
    m_window.Draw();
    m_window.TextBox(m_window.T("Available Mastery Points:"), 40, 1, m_window.Width() - 80, 1,
                     (VIUICOLOR)0, 0);
    VIString::snprintf16(buf, sizeof(buf), "%d", app->GetP4EC58()->m_x0C);
    m_window.TextBox(buf, 40, 1, m_window.Width() - 80, 1, (VIUICOLOR)0, 16);
    m_window.TextBox(m_window.T("Percentage of experience points to apply towards mastery points:"),
                     40, 3, m_window.Width() - 80, 3, (VIUICOLOR)0, 8);
    m_window.DrawFrame(150, 6, m_window.Width() - 300, 1, 32);
    VIString::snprintf16(buf, sizeof(buf), "%d%%", m_percent);
    m_window.TextBox(buf, 40, 6, m_window.Width() - 80, 1, (VIUICOLOR)0, 8);
    VIString::snprintf16(buf, sizeof(buf), "|x %S  |t %S", Word(ui, 786), Word(ui, 753));
    m_window.Text(buf, 40, 8, (VIUICOLOR)0, 0);
    ui->End2D();
    if (m_keys.Pulse(136))
        Change(-1);
    if (m_keys.Pulse(137))
        Change(1);
}

// 0x00C37E58.
void VIWndClassMasteryPoints::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 18:
    case 137:
        m_keys.Press(137);
        break;
    case 17:
    case 136:
        m_keys.Press(136);
        break;
    case 13:
    case 134:
        app->GetP4EC58()->Select(m_percent);
        SetVisible(false);
        app->PlaySound(8);
        app->SetActiveWindow(GetParent());
        break;
    case 27:
    case 132:
        SetVisible(false);
        app->PlaySound(8);
        app->SetActiveWindow(GetParent());
        break;
    }
}

// 0x00C37FA0.
void VIWndClassMasteryPoints::OnKeyUp(int key)
{
    switch (key)
    {
    case 18:
    case 137:
        m_keys.Release(137);
        break;
    case 17:
    case 136:
        m_keys.Release(136);
        break;
    }
}

// 0x00C38010.  295: the focus arrived; the share starts at the client's.
void VIWndClassMasteryPoints::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295 && (VIWnd *)p2 == this)
    {
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 400, 9, 0);
        m_window.SetTitle(m_window.T("Class Mastery Points"));
        m_window.Open(true);
        m_percent = app->GetP4EC58()->m_x08;
    }
}

// 0x00C380F8.
void VIWndClassMasteryPoints::Change(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_percent += d;
    if (m_percent < 0)
    {
        m_percent = 0;
        app->PlaySound(9);
    }
    else if (m_percent > 100)
    {
        m_percent = 100;
        app->PlaySound(9);
    }
    else
        app->PlaySound(11);
}
