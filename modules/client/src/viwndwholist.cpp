#line 1 "C:/code-frontiers-gm/game/VIWndWhoList.cpp"
// VIWndWhoList -- CLIENT 0x00CA26F0 .. 0x00CA3A2C (the TU runs to its __tf at 0x00CA4608).
// The /who list: the players the server sent (VIClientPart38334's list of 32-byte records,
// race/class/level bytes and a name), ten rows at a time; Enter offers tell, invite and
// tell-gram for the row, Triangle asks the server again.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00CA26F0.
VIWndWhoList::VIWndWhoList()
{
    m_sel = 0;
    m_top = 0;
    m_options = 0;
    m_choices = 0;
    m_edit = 0;
    m_compose = 0;
}

// 0x00CA2758.
VIWndWhoList::~VIWndWhoList()
{
    Free();
}

// 0x00CA27C8.
int VIWndWhoList::Init()
{
    Free();
    m_options = new VIWndOptionList;
    if (!m_options)
        return -1;
    m_options->SetVisible(false);
    m_options->SetAppPtr(GetAppPtr());
    m_options->Init();
    AddChild(m_options);
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->SetVisible(false);
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    m_compose = new VIWndComposeGram;
    if (!m_compose)
        return -1;
    m_compose->SetAppPtr(GetAppPtr());
    m_compose->Init(2);
    m_compose->SetVisible(false);
    AddChild(m_compose);
    m_choices = new unsigned short *[3];
    m_choices[0] = new unsigned short[32];
    m_choices[1] = new unsigned short[32];
    m_choices[2] = new unsigned short[32];
    m_choices[0][0] = 0;
    m_choices[1][0] = 0;
    m_choices[2][0] = 0;
    return 0;
}

// 0x00CA2930.
int VIWndWhoList::Free()
{
    if (m_options)
    {
        RemoveChild(m_options);
        delete m_options;
        m_options = 0;
    }
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    if (m_compose)
    {
        RemoveChild(m_compose);
        delete m_compose;
        m_compose = 0;
    }
    if (m_choices)
    {
        delete[] m_choices[0];
        delete[] m_choices[1];
        delete[] m_choices[2];
        delete[] m_choices;
    }
    return 0;
}

// The "..." after "searching": one dot more every 15 frames, three at most.
static char sDots;    // 0x00CD79B4
static char sTicks;   // 0x00CD79B5

// 0x00CA2A60.
void VIWndWhoList::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIClientPart38334 *who = app->GetP38334();
    VIVector<VIRec32_C035B0> *list = who->GetList();
    unsigned short buf[128];
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    int x[4];
    unsigned short level[8];
    unsigned short back[32];
    m_window.Draw();
    if (m_sel > list->m_size - 1)
        m_sel = list->m_size - 1;
    m_sel = m_sel < 0 ? 0 : m_sel;
    if (m_sel - m_top > 9)
        m_top = m_sel - 9;
    else if (m_sel < m_top)
        m_top = m_sel;
    float t;
    if (list->m_size == 10)
        t = 0.0f;
    else
        t = (float)m_top / (float)(list->m_size - 10);
    m_window.DrawFrameVScroll(30, 1, m_window.Width() - 65, 10, t);
    x[0] = 50;
    x[1] = 300;
    x[2] = 400;
    x[3] = 500;
    m_window.TextBox(m_window.T((unsigned short)509), x[0], 0, x[1] - x[0], 1, (VIUICOLOR)0, 0);
    m_window.TextBox(m_window.T((unsigned short)510), x[1], 0, x[2] - x[1], 1, (VIUICOLOR)0, 0);
    m_window.TextBox(m_window.T((unsigned short)511), x[2], 0, x[3] - x[2], 1, (VIUICOLOR)0, 0);
    m_window.TextBox(m_window.T((unsigned short)503), x[3], 0, m_window.Width() - x[3] - 40, 1,
                     (VIUICOLOR)0, 0);
    for (int i = m_top, row = 1; i < list->m_size && row < 11; i++, row++)
    {
        VIRec32_C035B0 *e = &list->m_data[i];
        const VIRaceDef *race = GetRace(e->m_x00);
        const VIClassDef *cls = GetClass(e->m_x01);
        VIString::snprintf16(level, sizeof(level), "%d", e->m_x02);
        m_window.TextBox(m_window.T(e->m_name), x[0], row, x[1] - x[0], 1, (VIUICOLOR)(i == m_sel), 2);
        m_window.TextBox(m_window.T((unsigned short)race->m_shortStrID), x[1], row, x[2] - x[1], 1,
                         (VIUICOLOR)(i == m_sel), 2);
        m_window.TextBox(m_window.T((unsigned short)cls->m_shortStrID), x[2], row, x[3] - x[2], 1,
                         (VIUICOLOR)(i == m_sel), 2);
        m_window.TextBox(level, x[3], row, m_window.Width() - x[3] - 40, 1, (VIUICOLOR)(i == m_sel), 2);
        if (i == m_sel)
            m_window.TickR(46, row, app->m_p2BD40 == this);
    }
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S          |s%S", Word(ui, 212), Word(ui, 161),
                         Word(ui, 811));
    m_window.TextBox(buf, 30, 12, m_window.Width() - 60, 1, (VIUICOLOR)0, 0);
    if (--sTicks <= 0)
    {
        sTicks = 15;
        if (++sDots == 3)
            sDots = 0;
    }
    VIString::snprintf16(back, sizeof(back), "|t%S", Word(ui, 161));
    if (who->m_pending == 1)
    {
        m_status.Open(false);
        m_status.Draw();
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 943));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.TextBox(buf, 40, 1, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        m_status.TextBox(back, 40, 3, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
    }
    ui->End2D();
    if (m_flash.Pulse(138))
        Up();
    if (m_flash.Pulse(139))
        Down();
}

// 0x00CA30B8.
void VIWndWhoList::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 19:
    case 138:
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        m_flash.Press(139);
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel704();
        break;
    case 13:
    case 134:
        Select();
        break;
    case 81:
    case 113:
    case 133:
        Refresh();
        break;
    }
}

// 0x00CA31F0.
void VIWndWhoList::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        m_flash.Release(138);
        break;
    case 20:
    case 139:
        m_flash.Release(139);
        break;
    }
}

// 0x00CA3268.
void VIWndWhoList::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_options && (VIWnd *)p1 != this &&
            (VIWnd *)p1 != m_compose && (VIWnd *)p1 != m_edit)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 600, 13, 0);
            m_window.SetTitle(m_window.T((unsigned short)730));
            m_window.Open(true);
            m_status.Close(true);
            m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 330, 4, 0);
            m_status.SetTitle(m_status.T((unsigned short)271));
            VIString::strcpy16(m_choices[0], 64, m_window.T((unsigned short)425));
            VIString::strcpy16(m_choices[1], 64, m_window.T((unsigned short)942));
            VIString::strcpy16(m_choices[2], 64, m_window.T((unsigned short)515));
            m_options->Setup(m_window.T((unsigned short)422), (const unsigned short **)m_choices, 3,
                             VIVect2I(320, 180), 256);
            m_options->SetVisible(false);
            m_edit->SetVisible(false);
            m_compose->SetVisible(false);
            m_sel = 0;
            m_top = 0;
            m_flash.Reset();
        }
        if ((VIWnd *)p1 == this && (VIWnd *)p2 != this && (VIWnd *)p2 != m_options &&
            (VIWnd *)p2 != m_compose && (VIWnd *)p2 != m_edit)
            m_flash.Reset();
        break;
    case 308:
        if (p2)
        {
            app->SetActiveWindow(this);
            switch (p1)
            {
            case 0:
                Tell();
                break;
            case 1:
                Invite();
                break;
            case 2:
                Mail();
                break;
            }
        }
        else
            app->SetActiveWindow(this);
        break;
    case 259:
        SendTell();
        break;
    case 309:
        if (w == m_compose)
            w->SetVisible(false);
        app->SetActiveWindow(this);
        break;
    case 260:
        if (w == m_compose || w == m_edit)
            w->SetVisible(false);
        app->SetActiveWindow(this);
        break;
    case 261:
        break;
    }
}

// 0x00CA35C0.
void VIWndWhoList::Down()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIVector<VIRec32_C035B0> *list = app->GetP38334()->GetList();
    if (m_sel < list->m_size - 1)
    {
        m_sel++;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00CA3650.
void VIWndWhoList::Up()
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

// 0x00CA36B8.
void VIWndWhoList::Select()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIVector<VIRec32_C035B0> *list = app->GetP38334()->GetList();
    if (m_sel < list->m_size)
    {
        app->PlaySound(11);
        m_options->SetVisible(true);
        app->SetActiveWindow(m_options);
    }
}

// 0x00CA3738.
void VIWndWhoList::Tell()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_edit->SetRows(1);
    m_edit->SetMultiLine(0);
    m_edit->SetCenter(m_window.Center());
    m_edit->SetMaxLength(128);
    m_edit->SetVisible(true);
    app->SetActiveWindow(m_edit);
}

// 0x00CA37C0.
void VIWndWhoList::SendTell()
{
    unsigned short buf[256];
    VIClient *app = (VIClient *)GetAppPtr();
    VIVector<VIRec32_C035B0> *list = app->GetP38334()->GetList();
    if (m_sel >= 0 && m_sel < list->m_size)
    {
        VIString::snprintf16(buf, sizeof(buf), "/t %s %S", list->m_data[m_sel].m_name, m_edit->m_text);
        app->ChatLine(buf);
        m_edit->SetVisible(false);
        app->SetActiveWindow(this);
    }
}

// 0x00CA3870.
void VIWndWhoList::Invite()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIVector<VIRec32_C035B0> *list = app->GetP38334()->GetList();
    if (m_sel >= 0 && m_sel < list->m_size)
        app->InviteToGroup((VIInviteRec *)&list->m_data[m_sel]);
}

// 0x00CA38E8.
void VIWndWhoList::Refresh()
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->GetP38334()->Open();
    m_sel = 0;
    m_top = 0;
    m_flash.Reset();
}

// 0x00CA3938.
void VIWndWhoList::Mail()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIVector<VIRec32_C035B0> *list = app->GetP38334()->GetList();
    if (m_sel >= 0 && m_sel < list->m_size)
    {
        VIRec32_C035B0 *e = &list->m_data[m_sel];
        m_compose->SetVisible(true);
        m_compose->Refresh();
        app->SetActiveWindow(m_compose);
        m_compose->SetTo(m_window.T(e->m_name));
        m_compose->SetSubject(m_window.T(""));
        m_compose->SetBody(m_window.T(""));
    }
}
