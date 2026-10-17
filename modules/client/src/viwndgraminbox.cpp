#line 1 "C:/code-frontiers-gm/game/VIWndGramInbox.cpp"
// viwndgraminbox.cpp -- CLIENT 0x00C59D98 .. 0x00C5B958: VIWndGramInbox, the in-game
// mail inbox (lane cl-wh).  CLIENT's twin of STATION's VIWndCSInbox: the same
// methods in the same order, reading the gram client (VIClient vtable slot 7) where
// STATION reads its customer-service client.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C59D98.
VIWndGramInbox::VIWndGramInbox()
{
    m_read = 0;
}

// 0x00C59DE8.
VIWndGramInbox::~VIWndGramInbox()
{
    Free();
}

// 0x00C59E58.  The reader, hidden.
int VIWndGramInbox::Init()
{
    Free();
    m_read = new VIWndReadGram;
    if (!m_read)
        return -1;
    m_read->SetVisible(false);
    m_read->SetAppPtr(GetAppPtr());
    m_read->Init();
    AddChild(m_read);
    return 0;
}

// 0x00C59ED0.
void VIWndGramInbox::Free()
{
    if (m_read)
    {
        RemoveChild(m_read);
        delete m_read;
        m_read = 0;
    }
}

// The "..." after a status line: one dot more every 15 frames, three at most.
static char sDots;    // 0x00CD7998
static char sTicks;   // 0x00CD7999

// 0x00C59F28.
void VIWndGramInbox::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart2EFD0 *grams = app->GetP2EFD0();
    if (app->m_p2BD40 != this)
        m_window.ResetTickPulse();
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
    int x[3];
    x[0] = 58;
    x[1] = 160;
    x[2] = m_window.Width() - 150;
    m_window.Text(m_window.T(882), x[0], 0, (VIUICOLOR)0, 0);
    m_window.Text(m_window.T(883), x[1], 0, (VIUICOLOR)0, 0);
    m_window.Text(m_window.T(884), x[2], 0, (VIUICOLOR)0, 0);
    if (m_sel > grams->GetCount() - 1)
        m_sel = grams->GetCount() - 1;
    m_sel = m_sel < 0 ? 0 : m_sel;
    if (m_sel - m_top > 10)
        m_top = m_sel - 10;
    else if (m_sel < m_top)
        m_top = m_sel;
    float t;
    if (grams->GetCount() == 11)
        t = 0.0f;
    else
        t = (float)m_top / (float)(grams->GetCount() - 11);
    m_window.DrawFrameVScroll(40, 1, m_window.Width() - 80, 11, t);
    for (int i = m_top; i < (m_top + 11 > grams->GetCount() ? grams->GetCount() : m_top + 11); i++)
    {
        unsigned short date[64];
        m_window.TextBox(m_window.T(grams->GetHeader(i)->m_from), x[0], i - m_top + 1,
                         x[1] - x[0] - 10, 1, (VIUICOLOR)(m_sel == i), 2);
        m_window.TextBox(grams->GetHeader(i)->m_subject, x[1], i - m_top + 1,
                         x[2] - x[1] - 10, 1, (VIUICOLOR)(m_sel == i), 2);
        grams->GetHeader(i)->m_date.Format(date, sizeof(date));
        date[10] = 0;
        m_window.TextBox(date, x[2], i - m_top + 1, m_window.Width() - 44 - x[2], 1,
                         (VIUICOLOR)(m_sel == i), 2);
        if (m_sel == i)
            m_window.TickR(56, i - m_top + 1, true);
    }
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S           |s%S",
                         Word(ui, 913), Word(ui, 161), Word(ui, 843));
    m_window.Text(buf, 40, 13, (VIUICOLOR)0, 0);
    unsigned short back[32];
    VIString::snprintf16(back, sizeof(back), "|t%S", Word(ui, 161));
    char d;
    switch (grams->GetState())
    {
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
        m_open = 1;
        break;
    case 3:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 888));
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
    case 0:
        if (m_open)
        {
            m_open = 0;
            m_read->SetText(grams->GetMessage()->m_text);
            m_read->SetVisible(true);
            app->SetActiveWindow(m_read);
        }
        break;
    }
    ui->End2D();
    if (m_flash.Pulse(138))
        Prev();
    if (m_flash.Pulse(139))
        Next();
}

// 0x00C5A730.  Nothing moves while the client is busy; the arrows repeat through
// the key flash (OnDrawSelf).
void VIWndGramInbox::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart2EFD0 *grams = app->GetP2EFD0();
    switch (key)
    {
    case 19:
    case 138:
        if (grams->GetState() > 0)
            break;
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        if (grams->GetState() > 0)
            break;
        m_flash.Press(139);
        break;
    case 13:
    case 134:
        if (grams->GetState() > 0)
            break;
        Read();
        break;
    case 81:
    case 113:
    case 133:
        if (grams->GetState() > 0)
            break;
        Refresh();
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel6E4();
        break;
    }
}

// 0x00C5A8C0.
void VIWndGramInbox::OnKeyUp(int key)
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

// 0x00C5A938.
void VIWndGramInbox::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 560, 14, 0);
            m_window.SetTitle(m_window.T(914));
            m_window.Open(true);
            m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 375, 4, 0);
            m_status.SetTitle(m_status.T(271));
            m_status.Open(true);
            m_open = 0;
            m_flash.Reset();
            if ((VIWnd *)p2 != m_read)
                m_read->SetVisible(false);
        }
        if ((VIWnd *)p1 == this)
            m_flash.Reset();
        break;
    case 260:
        if (w == m_read)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    }
}

// 0x00C5AAC8.
void VIWndGramInbox::Refresh()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (app->GetP2EFD0()->GetState() <= 0)
        app->GetP2EFD0()->RequestInbox();
    m_top = 0;
    m_sel = 0;
}

// 0x00C5AB40.
void VIWndGramInbox::End()
{
}

// 0x00C5AB48.
void VIWndGramInbox::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel < app->GetP2EFD0()->GetCount() - 1)
    {
        m_sel++;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C5ABD8.
void VIWndGramInbox::Prev()
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

// 0x00C5AC40.  The selected gram's header goes to the reader now; its text when it
// arrives (OnDrawSelf, state 0).
void VIWndGramInbox::Read()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart2EFD0 *grams = app->GetP2EFD0();
    if (!grams->GetCount())
        return;
    unsigned short buf[64];
    grams->RequestMessage(grams->GetHeader(m_sel)->m_id);
    m_read->SetFrom(m_window.T(grams->GetHeader(m_sel)->m_from));
    m_read->SetSubject(grams->GetHeader(m_sel)->m_subject);
    m_read->m_id = grams->GetHeader(m_sel)->m_id;
    grams->GetMessage()->m_date.Format(buf, sizeof(buf));
    m_read->SetDate(buf);
}
