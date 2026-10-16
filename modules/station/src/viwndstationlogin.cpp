// viwndstationlogin.cpp (name ours) -- STATION 0x002BC6A8 .. 0x002BCF80.  The
// plain Station login: Login, Password and Connect in one window, each field
// edited through a VIWndEdit.  Nothing in the module creates one (the account
// screens are GUI pages now); it is the betas' screen, still linked.
#include "station.h"

// One WORDS.TXT entry in the current language (MCERR's and SETUP's helper).
inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x002BC6A8.
VIWndStationLogin::VIWndStationLogin()
{
    m_edit = 0;
    strcpy(m_login, "joe");
    strcpy(m_password, "schmoe");
}

// 0x002BC718.
VIWndStationLogin::~VIWndStationLogin()
{
}

// 0x002BC768.
int VIWndStationLogin::Init()
{
    m_edit = new VIWndEdit;
    m_edit->SetVisible(false);
    m_edit->m_pos = VIVect2I(32, 190);
    AddChild(m_edit);
    m_sel = 2;
    return 0;
}

// 0x002BC7D8.
void VIWndStationLogin::Free()
{
}

// 0x002BC7E0.
void VIWndStationLogin::OnDrawSelf()
{
    VIColor32F normal(0.0f, 0.0f, 0.0f, m_window.m_open);
    VIColor32F select(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    VIUI *ui = UI();
    VIVect2I pos;
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 449), (VIAlphaBlendMode)3, 1.0f);
    m_window.Draw();
    ui->m_font.SetTextColor(normal);
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, "Station Login",
                                  m_window.RowCenter(0).x, m_window.RowCenter(0).y, 3);

    pos.Set(m_window.TopLeft().x + 40, m_window.RowCenter(2).y);
    if (m_sel == 0)
    {
        ui->m_font.SetTextColor(select);
        ui->BlitEx((VIUITexId)89, VIVect2I(pos.x - 18, pos.y - 5), (VIAlphaBlendMode)3,
                   m_window.m_open, 0);
    }
    else
        ui->m_font.SetTextColor(normal);
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, "Login:", pos.x, pos.y, 1);
    pos.x += 100;
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, m_login, pos.x, pos.y, 1);

    pos.Set(m_window.TopLeft().x + 40, m_window.RowCenter(3).y);
    if (m_sel == 1)
    {
        ui->m_font.SetTextColor(select);
        ui->BlitEx((VIUITexId)89, VIVect2I(pos.x - 18, pos.y - 5), (VIAlphaBlendMode)3,
                   m_window.m_open, 0);
    }
    else
        ui->m_font.SetTextColor(normal);
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, "Password:", pos.x, pos.y, 1);
    pos.x += 100;
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, m_password, pos.x, pos.y, 1);

    pos.Set(m_window.TopLeft().x + 40, m_window.RowCenter(4).y);
    if (m_sel == 2)
    {
        ui->m_font.SetTextColor(select);
        ui->BlitEx((VIUITexId)89, VIVect2I(pos.x - 18, pos.y - 5), (VIAlphaBlendMode)3,
                   m_window.m_open, 0);
    }
    else
        ui->m_font.SetTextColor(normal);
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, "Connect", pos.x, pos.y, 1);

    ui->m_font.SetTextColor(normal);
    pos.Set(m_window.m_center.x - (m_window.m_size.x >> 1) + 40, m_window.RowCenter(6).y - 4);
    ui->Blit((VIUITexId)66, VIVect2I(pos.x, pos.y - 6));
    ui->m_font.DrawString(ui->m_raster, ui->m_dict, Word(ui, 212), pos.x + 20, pos.y - 10);
    ui->End2D();
}

// 0x002BCDF0.
void VIWndStationLogin::OnKeyUp(int key)
{
}

// 0x002BCCC0.  Up and down step through the three rows; X edits a field or
// leaves with Connect.
void VIWndStationLogin::OnKeyDown(int key)
{
    switch (key)
    {
    case 138:
        m_sel = (m_sel + 2) % 3;
        break;
    case 139:
        m_sel = (m_sel + 1) % 3;
        break;
    case 134:
        switch (m_sel)
        {
        case 0:
        case 1:
            m_edit->SetVisible(true);
            SetFocus(m_edit, true);
            switch (m_sel)
            {
            case 0:
                m_edit->SetText(m_login);
                break;
            case 1:
                m_edit->SetText(m_password);
                break;
            }
            break;
        case 2:
            SetFocus(0, true);
            break;
        }
        break;
    }
}

// 0x002BCDF8.  259: the edit is done (its text goes into the field), 260:
// cancelled.  295: the window takes the focus.
void VIWndStationLogin::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    switch (msg)
    {
    case 295:
        if (p2 == (int)this && p1 != (int)m_edit)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 300, 7, 0);
            m_window.Open(false);
        }
        break;
    case 259:
        m_edit->SetVisible(false);
        SetFocus(this, true);
        switch (m_sel)
        {
        case 0:
            VIString::str16to8(m_login, 256, m_edit->m_text);
            break;
        case 1:
            VIString::str16to8(m_password, 256, m_edit->m_text);
            break;
        }
        break;
    case 260:
        m_edit->SetVisible(false);
        SetFocus(this, true);
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}
