#line 1 "C:/code-frontiers-gm/game/VIWndResponse.cpp"
// VIWndResponse (name RTTI's) -- CLIENT 0x00C8F950 .. 0x00C910D8: an NPC dialog's
// answers.  The dialog's text, then three of its options a page with More and Back to
// page, and Close; X sends the choice (0x0035, VIClient::SendOp0035).  The VIMap<int,
// int> and VIArray<VIItemStat> copies between Build and __tf are template instantiations
// this TU does not write.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C8F950.
VIWndResponse::VIWndResponse()
{
    m_sel = 0;
    m_first = 0;
    SetPos(VIVect2I(0, 0));
    Clear();
}

// 0x00C8F9C0.
VIWndResponse::~VIWndResponse()
{
}

// 0x00C8FA10.
int VIWndResponse::Init()
{
    return 0;
}

// 0x00C8FA18.
void VIWndResponse::Clear()
{
    m_sel = 0;
    memset(m_text, 0, sizeof(m_text));
    for (int i = 0; i < 6; i++)
    {
        m_option[i].m_index = -1;
        m_option[i].m_type = 0;
        memset(m_option[i].m_text, 0, sizeof(m_option[i].m_text));
    }
}

// 0x00C8FAA0.
void VIWndResponse::OnDrawSelf()
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    int n = 0;
    ui->Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    m_window.Draw();
    int lines = m_window.CountLines(m_text, 460);
    VIColor32F c(0.8f, 0.8f, 0.8f, 1.0f);
    m_window.TextBox(m_text, 20, 0, 460, lines, c, 3);
    unsigned short buf[64];
    int row = lines + 1;
    for (; n < 6 && m_option[n].m_text[0]; n++)
    {
        if (n + m_first == m_sel)
            c.Set(1.0f, 1.0f, 0.0f, 1.0f);
        else
            c = white;
        lines = m_window.CountLines(m_option[n].m_text, 430);
        m_window.TextBox(m_option[n].m_text, 50, row, 430, lines, c, 3);
        if (n + m_first == m_sel)
            m_window.TickR(40, row, true);
        row += lines;
    }
    row++;
    VIString::snprintf16(buf, sizeof(buf), "|t%S", Word(ui, 161));
    m_window.Text(buf, 20, row, white, 1);
    ui->End2D();
}

// 0x00C8FCF0.
void VIWndResponse::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        app->SendOp0035(m_dialog.m_id, 255);
        GetParent()->OnMessage(this, 260, 0, 0);
        m_sel = 0;
        m_first = 0;
        break;
    case 13:
    case 134:
        for (int i = 0; m_option[i].m_text[0] && i < 6; i++)
        {
            if (m_option[i].m_index == m_sel)
            {
                switch (m_option[i].m_type)
                {
                case 0:
                    break;
                case 1:
                    app->SendOp0035(m_dialogId, m_option[i].m_index);
                    GetParent()->OnMessage(this, 276, 0, 0);
                    m_sel = 0;
                    m_first = 0;
                    break;
                case 4:
                    app->SendOp0035(m_dialogId, 255);
                    GetParent()->OnMessage(this, 260, 0, 0);
                    m_sel = 0;
                    m_first = 0;
                    break;
                case 2:
                    m_first += 3;
                    Clear();
                    Build(m_dialog);
                    Move(1);
                    break;
                case 3:
                    m_first -= 3;
                    Clear();
                    Build(m_dialog);
                    Move(-1);
                    break;
                }
            }
        }
        break;
    case 19:
    case 138:
        Move(-1);
        break;
    case 20:
    case 139:
        Move(1);
        break;
    }
}

// 0x00C8FF78.
void VIWndResponse::OnKeyUp(int key)
{
}

// 0x00C8FF80.  Up/down among the options shown.
void VIWndResponse::Move(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int n = 0;
    for (int i = 0; m_option[i].m_text[0] && i < 6; i++)
        n++;
    int at = m_sel - m_first + d;
    if (at < 0)
    {
        app->PlaySound(9);
        m_sel = m_first;
    }
    else if (at >= n)
    {
        app->PlaySound(9);
        m_sel = m_first + n - 1;
    }
    else
    {
        m_sel = m_sel + d;
        app->PlaySound(11);
    }
    m_window.ResetTickPulse();
}

// 0x00C90058.
void VIWndResponse::SetDialog(const VIDialogBox &d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_dialog = d;
    Clear();
    Build(d);
}

// 0x00C90148.
void VIWndResponse::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if ((VIWnd *)p2 == this)
        {
            m_window.Close(true);
            m_window.Open(false);
        }
    }
    else
        VIWnd::OnMessage(this, msg, p1, p2);
}

// 0x00C901A0.  The page of options from m_first, then More / Back / Close, and the window
// sized to fit.
void VIWndResponse::Build(const VIDialogBox &d)
{
    int n = 0;
    m_count = d.m_count;
    m_dialogId = d.m_id;
    VIString::strcpy16(m_text, sizeof(m_text), d.m_text);
    int i;
    for (i = m_first; n < 6 && i < VIMIN(d.m_count, m_first + 3); i++, n++)
    {
        m_option[n].m_index = i;
        m_option[n].m_type = 1;
        VIString::strcpy16(m_option[n].m_text, sizeof(m_option[n].m_text), d.m_options[i]);
    }
    if (m_first + 3 < d.m_count)
    {
        m_option[n].m_index = i;
        m_option[n].m_type = 2;
        VIString::strcpy16(m_option[n].m_text, sizeof(m_option[n].m_text), Word(UI(), 245));
        n++;
        i++;
    }
    if (m_first >= 3)
    {
        m_option[n].m_index = i;
        m_option[n].m_type = 3;
        VIString::strcpy16(m_option[n].m_text, sizeof(m_option[n].m_text), Word(UI(), 217));
        n++;
        i++;
    }
    m_option[n].m_type = 4;
    m_option[n].m_index = i;
    VIString::strcpy16(m_option[n].m_text, sizeof(m_option[n].m_text), Word(UI(), 161));
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 500, 32, 1);
    int rows = m_window.CountLines(m_text, 460) + 1;
    for (n = 0; m_option[n].m_text[0] && n < 6; n++)
        rows += m_window.CountLines(m_option[n].m_text, 430);
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 200), 500, rows + 2, 1);
}
