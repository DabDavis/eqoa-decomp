#line 1 "C:/code-frontiers-gm/game/VIWndDisconnect.cpp"
// VIWndDisconnect.cpp -- CLIENT 0x00C434B8 .. 0x00C44448: the disconnect notice,
// embedded in VIClient at +0x2BB98 (game.h).  Names ours.
#include "game.h"

// 0x00C434B8.
VIWndDisconnect::VIWndDisconnect()
{
}

// 0x00C434F8.
VIWndDisconnect::~VIWndDisconnect()
{
}

// 0x00C43548.
int VIWndDisconnect::Init()
{
    return 0;
}

// 0x00C43550.  A dark screen, the reason in a box sized to it, and "Ok".
void VIWndDisconnect::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 450), (VIAlphaBlendMode)3, 1.0f);
    int w = m_window.TextWidth(app->ConnectStatusString());
    int fits = w < 441;
    w = fits ? w : 440;
    int rows = 4;
    if (!fits)
        rows = 5;
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), w + 100, rows, 0);
    m_window.Open(true);
    m_window.Draw();
    m_window.TextBox(app->ConnectStatusString(), 40, 1, m_window.Width() - 80, 2, (VIUICOLOR)0, 8);
    unsigned short buf[32];
    VIString::snprintf16(buf, sizeof(buf), "|x%S", ui->m_words.GetString(786, ui->m_language));
    m_window.Text(buf, 40, fits ? 3 : 4, (VIUICOLOR)0, 0);
    ui->End2D();
}

// 0x00C436F8.
void VIWndDisconnect::OnKeyUp(int key)
{
}

// 0x00C43700.
void VIWndDisconnect::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (key == 13 || key == 134)
        app->Msg0777Done(app->DisconnectKind());
}

// 0x00C43758.
void VIWndDisconnect::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295 && (VIWnd *)p2 == this)
    {
        m_window.Close(true);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 350, 4, 0);
        m_window.SetTitle(m_window.T(272));
        m_window.Open(false);
    }
}
