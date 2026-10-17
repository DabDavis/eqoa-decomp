#line 1 "C:/code-frontiers-gm/game/VIWndMenu4Edit.cpp"
// VIWndMenu4Edit (name the RTTI's) -- CLIENT 0x00C75460 .. 0x00C77748.  The editor a held
// VIWndMenu4 direction opens over one choice: a menu (edit the name, edit the line or drop
// the sub-menu, make a sub-menu, done), a "say or command" window, and a list of the 34
// quick commands that glides a row at a time.  Method names ours.
#include "game.h"

extern "C" int sscanf(const char *, const char *, ...);
extern "C" int sprintf(char *, const char *, ...);

// VIVect2I's operator- is cl_wm.h's free inline (DrawCommands subtracts two row centres).

// The quick commands' names, by command id (the ids VIWndMenu4::OnKeyUp maps to messages).
static unsigned short sCommandWords[34] = {
    41, 991, 992, 31, 86, 993, 994, 995, 996, 997, 998, 999, 1008, 1009, 106, 107, 108,
    512, 534, 425, 522, 1000, 1010, 737, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1194,
    1197, 1198,
};

// 0x00C75460.
VIWndMenu4Edit::VIWndMenu4Edit()
{
    m_pos.Set(0, 0);
    m_center.Set(320, 240);
    m_kindCenter = m_center;
    m_cmdCenter = m_center;
    m_sel = 0;
    m_kind = 0;
    m_title[0] = 0;
    m_text[0] = 0;
    m_hasSub = 0;
    m_hidden = 0;
    m_x17C = 0;
    m_edit = 0;
    m_kindOpen = 0;
    m_cmdOpen = 0;
    m_cmd = 0;
    m_scroll = 0;
    m_scrollDir = 0;
    m_depth = 0;
}

// 0x00C75568.
VIWndMenu4Edit::~VIWndMenu4Edit()
{
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
}

// 0x00C75608.  The menu, then the say-or-command window, then the command list.
void VIWndMenu4Edit::OnDrawSelf()
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIVect2I unused;
    ui->Begin2D();
    unsigned short hint[64];
    VIString::snprintf16(hint, sizeof(hint), "|x%S   |t%S",
                         ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(213, ui->m_language));
    if (!m_hidden)
    {
        if (m_hasSub)
        {
            VIUIWindow &w = m_window;
            w.Init(ui, m_pos, m_center, 220, 7, 0);
            w.Open(false);
            if (w.Draw())
            {
                w.TextBox(w.T(920), 20, 0, w.Width() - 40, 1, (VIUICOLOR)0, 8);
                w.TextBox(w.T(921), 20, 2, w.Width() - 40, 1, RowColor(0), 8);
                w.TextBox(w.T(922), 20, 3, w.Width() - 40, 1, RowColor(1), 8);
                w.TextBox(w.T(280), 20, 4, w.Width() - 40, 1, RowColor(2), 8);
                w.TextBox(hint, 20, 6, w.Width() - 40, 1, (VIUICOLOR)0, 0);
            }
        }
        else
        {
            VIUIWindow &w = m_window;
            w.Init(ui, m_pos, m_center, 220, m_depth < 3 ? 8 : 7, 0);
            w.Open(false);
            if (w.Draw())
            {
                w.TextBox(w.T(923), 20, 0, w.Width() - 40, 1, (VIUICOLOR)0, 8);
                w.TextBox(w.T(921), 20, 2, w.Width() - 40, 1, RowColor(0), 8);
                w.TextBox(w.T(924), 20, 3, w.Width() - 40, 1, RowColor(1), 8);
                if (m_depth < 3)
                    w.TextBox(w.T(925), 20, 4, w.Width() - 40, 1, RowColor(2), 8);
                w.TextBox(w.T(280), 20, m_depth < 3 ? 5 : 4, w.Width() - 40, 1,
                          RowColor(m_depth < 3 ? 3 : 2), 8);
                w.TextBox(hint, 20, m_depth < 3 ? 7 : 6, w.Width() - 40, 1, (VIUICOLOR)0, 0);
            }
        }
    }
    if (m_kindOpen)
    {
        VIUIWindow &w = m_kindWindow;
        w.Init(ui, m_center, m_kindCenter, 230, 4, 0);
        w.Draw();
        if (w.IsOpen())
        {
            w.TextBox(w.T(926), 20, 0, w.Width() - 40, 1, KindColor(0), 8);
            w.TextBox(w.T(927), 20, 1, w.Width() - 40, 1, KindColor(1), 8);
            w.TextBox(hint, 20, 3, w.Width() - 40, 1, (VIUICOLOR)0, 0);
        }
        if (w.IsClosed())
            m_kindOpen = 0;
    }
    if (m_cmdOpen)
    {
        VIUIWindow &w = m_cmdWindow;
        w.Init(ui, m_center, m_cmdCenter, 230, 9, 0);
        w.Draw();
        if (w.IsOpen())
        {
            w.TextBox(w.T(928), 20, 0, w.Width() - 40, 1, (VIUICOLOR)0, 8);
            DrawCommands();
            w.TextBox(hint, 20, 8, w.Width() - 40, 1, (VIUICOLOR)0, 0);
        }
        if (w.IsClosed())
            m_cmdOpen = 0;
    }
    ui->End2D();
}

// 0x00C75C18.  The command list glides: a press starts a step, a second press while
// one is under way finishes it.
void VIWndMenu4Edit::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_cmdOpen)
    {
        switch (key)
        {
        case 19:
        case 138:
            if (m_cmd > 0)
            {
                app->PlaySound(11);
                if (m_cmd == 1 && m_scroll == 0.0f && m_scrollDir == 0)
                {
                    m_scroll = 0.0f;
                    m_scrollDir = -1;
                }
                else if (m_cmd == 1 && m_scroll > 0.0f && m_scrollDir == -1)
                {
                    m_scroll = 0.0f;
                    m_scrollDir = 0;
                    m_cmd = 0;
                }
                if (m_cmd > 1)
                {
                    if (m_scrollDir == -1)
                        m_cmd = m_cmd - 1;
                    m_scrollDir = -1;
                    m_scroll = 0;
                }
            }
            break;
        case 20:
        case 139:
            if (m_cmd < 33)
            {
                app->PlaySound(11);
                if (m_cmd == 32 && m_scroll == 0.0f && m_scrollDir == 0)
                {
                    m_scroll = 0.0f;
                    m_scrollDir = 1;
                }
                else if (m_cmd == 32 && m_scroll > 0.0f && m_scrollDir == 1)
                {
                    m_scroll = 0.0f;
                    m_cmd = 33;
                    m_scrollDir = 0;
                }
                if (m_cmd < 32)
                {
                    if (m_scrollDir == 1)
                        m_cmd = m_cmd + 1;
                    m_scrollDir = 1;
                    m_scroll = 0;
                }
            }
            break;
        case 13:
        case 134:
            PickCommand();
            app->PlaySound(13);
            break;
        case 27:
        case 132:
            m_cmdWindow.Close(false);
            m_kindOpen = 1;
            m_kindWindow.Open(false);
            app->PlaySound(8);
            break;
        }
    }
    else if (m_kindOpen && m_kindWindow.IsOpen())
    {
        switch (key)
        {
        case 19:
        case 138:
            m_kind = VIMAX(m_kind - 1, 0);
            app->PlaySound(11);
            break;
        case 20:
        case 139:
            m_kind = VIMIN(m_kind + 1, 1);
            app->PlaySound(11);
            break;
        case 13:
        case 134:
            AcceptKind();
            app->PlaySound(13);
            break;
        case 27:
        case 132:
            m_kindWindow.Close(false);
            app->PlaySound(8);
            break;
        }
    }
    else
    {
        switch (key)
        {
        case 19:
        case 138:
            m_sel = VIMAX(m_sel - 1, 0);
            app->PlaySound(11);
            break;
        case 20:
        case 139:
            // The last row: done, at 2 with a sub-menu or at depth 3, else at 3.  The
            // macro evaluates it twice, and fold() pushes the compare into each arm.
            m_sel = VIMIN(m_hasSub ? 2 : (m_depth < 3 ? 3 : 2), m_sel + 1);
            app->PlaySound(11);
            break;
        case 13:
        case 134:
            Accept();
            app->PlaySound(13);
            break;
        case 27:
        case 132:
            VIWnd::OnMessage(this, 260, 0, 0);
            app->PlaySound(8);
            break;
        }
    }
}

// 0x00C76158.
void VIWndMenu4Edit::OnKeyUp(int key)
{
}

// 0x00C76160.  The text editor's done (259) and cancel (260).
void VIWndMenu4Edit::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        break;
    case 259:
        if (!m_edit)
            return;
        if (m_sel == 0)
            VIString::strcpy16(m_title, sizeof(m_title), m_edit->m_text);
        else if (m_sel == 1 && !m_hasSub)
            VIString::strcpy16(m_text, sizeof(m_text), m_edit->m_text);
    case 260:
        if (m_edit)
        {
            app->SetActiveWindow(this);
            m_edit->m_visible = 0;
        }
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C76280.  Centred, over a black shadow; yellow when lit.
void VIWndMenu4Edit::DrawShadowText(const unsigned short *s, const VIVect2I &at, int lit)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIVect2I p = at;
    VIColor32F c(0.0f, 0.0f, 0.0f, 1.0f);
    ui->m_font.SetTextColor(c);
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, s, p.x + 2, p.y + 2, 0);
    if (lit)
        c.Set(1.0f, 1.0f, 0.0f, 1.0f);
    else
        c.Set(1.0f, 1.0f, 1.0f, 1.0f);
    ui->m_font.SetTextColor(c);
    int x = p.x;
    int y = p.y;
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, s, x, y, 0);
}

// 0x00C76398.
void VIWndMenu4Edit::DrawText(const unsigned short *s, const VIVect2I &at, const VIColor32F &c)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIVect2I p = at;
    ui->m_font.SetTextColor(c);
    int x = p.x;
    int y = p.y;
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, s, x, y, 0);
}

// 0x00C76430.
void VIWndMenu4Edit::SetPos(const VIVect2I &p)
{
    m_pos = p;
}

// 0x00C76448.
unsigned short *VIWndMenu4Edit::GetTitle()
{
    return m_title;
}

// 0x00C76450.
void VIWndMenu4Edit::SetTitle(const unsigned short *s)
{
    if (s)
        VIString::strcpy16(m_title, sizeof(m_title), s);
}

// 0x00C76478.
unsigned short *VIWndMenu4Edit::GetText()
{
    return m_text;
}

// 0x00C76480.
void VIWndMenu4Edit::SetText(const unsigned short *s)
{
    if (s)
        VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x00C764A8.
int VIWndMenu4Edit::GetHasSub()
{
    return m_hasSub;
}

// 0x00C764B0.
void VIWndMenu4Edit::SetHasSub(int v)
{
    m_hasSub = v;
}

// 0x00C764B8.  X on the menu.  Its rows: the name, then the line (or, with a sub-menu,
// "drop it"), then "make a sub-menu" below depth 3, then done.
void VIWndMenu4Edit::Accept()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel == 0)
    {
        if (!m_edit)
        {
            m_edit = new VIWndEdit;
            if (!m_edit)
                return;
            AddChild(m_edit);
        }
        if (m_sel == 0)
            m_edit->SetText(m_title);
        m_edit->m_visible = 1;
        m_edit->SetPos(VIVect2I(32, 190));
        m_edit->SetAppPtr(GetAppPtr());
        app->SetActiveWindow(m_edit);
    }
    else if (!m_hasSub && m_sel == 1)
    {
        m_kindOpen = 1;
        if (m_text[0] == '|')
            m_kind = 1;
        else
            m_kind = 0;
        m_kindWindow.Open(false);
    }
    else if (m_hasSub == 1)
    {
        if (m_sel == 1)
        {
            m_hasSub = 0;
            m_sel = 2;
        }
        else if (m_sel == 2)
            VIWnd::OnMessage(this, 259, 0, 0);
    }
    else if (m_sel == 2 && m_depth < 3)
    {
        m_hasSub = 1;
        m_sel = 1;
        m_text[0] = 0;
    }
    else if (m_sel == (m_depth < 3 ? 3 : 2))
        VIWnd::OnMessage(this, 259, 0, 0);
}

// 0x00C76680.  X on say-or-command: say edits the line as text, command opens the list
// at the line's command.
void VIWndMenu4Edit::AcceptKind()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int id = 0;
    char buf[256];
    VIString::str16to8(buf, sizeof(buf), m_text);
    switch (m_kind)
    {
    case 0:
        if (buf[0] == '|')
            m_text[0] = 0;
        if (!m_edit)
        {
            m_edit = new VIWndEdit;
            if (!m_edit)
                return;
            AddChild(m_edit);
        }
        m_edit->SetText(m_text);
        m_edit->m_visible = 1;
        m_edit->SetPos(VIVect2I(32, 190));
        m_edit->SetAppPtr(GetAppPtr());
        app->SetActiveWindow(m_edit);
        m_kindWindow.Close(false);
        break;
    case 1:
        if (buf[0] == '|')
            sscanf(buf, "|%d", &id);
        m_kindWindow.Close(false);
        m_cmdOpen = 1;
        m_cmdWindow.Open(false);
        ResetCommand();
        m_cmd = id;
        break;
    }
}

// 0x00C767F8.
void VIWndMenu4Edit::PickCommand()
{
    GetAppPtr();
    char buf[256];
    sprintf(buf, "|%d", m_cmd);
    VIString::str8to16(m_text, sizeof(m_text), buf);
    m_cmdWindow.Close(false);
}

// 0x00C76850.
void VIWndMenu4Edit::ResetCommand()
{
    m_cmd = 0;
    m_scroll = 0;
    m_scrollDir = 0;
}

// 0x00C76860.  Seven rows around the current command, fading out toward the window's
// edges, gliding by m_scroll while a step is under way.
void VIWndMenu4Edit::DrawCommands()
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIColor32F c;
    int k = m_cmd - 3;
    int glide = (int)((float)(m_cmdWindow.RowCenter(1) - m_cmdWindow.RowCenter(0)).y
                      * (1.0f - (1.0f - m_scroll) * (1.0f - m_scroll)));
    VIVect2I top = m_cmdWindow.RowCenter(1);
    VIVect2I mid = m_cmdWindow.RowCenter(4);
    VIVect2I bottom = m_cmdWindow.RowCenter(7);
    unsigned short *word = &sCommandWords[k];
    for (int i = 0; i < 7; i++)
    {
        if ((unsigned int)k < 34)
        {
            VIVect2I p = m_cmdWindow.RowCenter(i + 1);
            p.x += 5;
            p.y -= glide * m_scrollDir;
            if (p.y <= top.y || p.y >= bottom.y)
                c = VIColor32F(0.0f, 0.0f, 0.0f, 0.0f);
            else
            {
                int num, den;
                if (p.y < mid.y)
                {
                    num = p.y - top.y;
                    den = mid.y - top.y;
                }
                else
                {
                    num = p.y - bottom.y;
                    den = mid.y - bottom.y;
                }
                float t = (float)num / (float)den;
                c = VIColor32F(0.0f, 0.0f, 0.0f, t * t);
            }
            DrawText((const unsigned short *)ui->m_words.GetString(*word, ui->m_language), p, c);
        }
        word++;
        k++;
    }
    if (m_scrollDir)
    {
        m_scroll += 0.2f;
        if (m_scroll >= 1.0f)
        {
            int cmd = m_cmd + m_scrollDir;
            m_scrollDir = 0;
            m_scroll = 0;
            m_cmd = VIMIN(VIMAX(cmd, 0), 33);
        }
    }
}

// 0x00C76B00.
VIUICOLOR VIWndMenu4Edit::RowColor(int i)
{
    return (VIUICOLOR)(m_sel == i);
}

// 0x00C76B10.
VIUICOLOR VIWndMenu4Edit::KindColor(int i)
{
    return (VIUICOLOR)(m_kind == i);
}
