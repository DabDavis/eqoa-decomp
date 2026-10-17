#line 1 "C:/code-frontiers-gm/game/VIWndChat.cpp"
// VIWndChat -- CLIENT 0x00C31F00 .. 0x00C33928: the chat box (cl_wc.h).  Lane cl-wc.
#include "game.h"

// 0x00CF5458: the colour plain text is added in (the static initialiser 0x00C33870).
static VIColor32F sWhite(1.0f, 1.0f, 1.0f, 1.0f);

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C31F00.
VIWndChat::VIWndChat()
{
    m_mode = 0;
    m_height = 0;
    m_target = 0;
    m_lines = 0;
    m_scroll = 0;
    m_top = 0;
    m_bottom = 0;
    m_heldKey = 0;
    m_repeat = 0;
}

// 0x00C31F68.
VIWndChat::~VIWndChat()
{
    Free();
}

// 0x00C31FC8.
int VIWndChat::Init()
{
    m_mode = 0;
    m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(0, 0), 462, 1, 1);
    m_text.Init((VIClient *)GetAppPtr(), m_window.Width() - 45);
    m_height = 0;
    m_target = 0;
    m_lines = 0;
    m_scroll = 0;
    m_top = 0;
    m_bottom = 0;
    m_heldKey = 0;
    m_repeat = 0;
    return 0;
}

// 0x00C32078.
int VIWndChat::Free()
{
    m_mode = 0;
    m_height = 0;
    m_target = 0;
    m_lines = 0;
    m_scroll = 0;
    m_text.Shutdown();
    m_top = 0;
    m_bottom = 0;
    m_heldKey = 0;
    m_repeat = 0;
    return 0;
}

// 0x00C320C8.
void VIWndChat::Reset()
{
    m_text.Reset();
    m_height = 0;
    m_target = 0;
    m_lines = 0;
    m_top = 0;
    m_bottom = 0;
}

// 0x00C32108.  The box slides its height toward the target at 200 pixels a second; a held
// scroll key repeats every 1/20 s after the first 0.45 s.
void VIWndChat::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_text.Age();
    VIUI *ui = &app->m_ui;
    float dt = app->m_scene.ParticleSystem()->DeltaTime();
    switch (m_heldKey)
    {
    case 19:
    case 20:
    case 138:
    case 139:
        m_repeat -= dt;
        if (m_repeat <= 0.0f)
        {
            m_repeat = 0.05f;
            if (m_heldKey == 138 || m_heldKey == 19)
                ScrollUp();
            else if (m_heldKey == 139 || m_heldKey == 20)
                ScrollDown();
        }
        break;
    }
    ui->Begin2D();
    m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(422, 390), 462, 6, 1);
    if (m_mode)
        ui->BlitEx((VIUITexId)32, VIVect2I(200, 331), VIVect2I(442, 118), (VIAlphaBlendMode)0, 1.0f, 0);
    int it;
    if (m_scroll == 0)
    {
        it = m_text.End();
        m_lines = CountLines();
    }
    else if (m_scroll == 1)
    {
        it = m_bottom;
        m_lines = 5;
    }
    else
    {
        it = m_text.End();
        m_lines = CountLines();
    }
    if (m_mode)
    {
        m_target = 85;
        m_height = 85;
        m_window.Open(true);
    }
    else if (m_lines == 0)
    {
        m_target = -((ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 4) * 2);
        m_window.Close(false);
    }
    else
    {
        m_target = (ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 4) * m_lines;
        m_window.Open(false);
    }
    m_window.m_center = VIVect2I(422, 475 - m_height);
    if (m_window.IsOpen() || m_window.IsOpening() || m_window.IsClosing())
        m_window.Draw();
    int row = m_lines - 1;
    if (row < 0)
        row = 0;
    for (it = m_text.Prev(it); it != m_text.Prev(m_text.First()) && row >= 0;
         it = m_text.Prev(it))
    {
        VIChatLine *l = m_text.Line(it);
        if (l && l->m_text[0] && (m_scroll == 1 || (m_scroll == 0 && l->m_type == 1) || m_mode))
        {
            if (row < 0)
                break;
            m_window.TextBox(l->m_text, 25, row, m_window.Width() - 45, 1, l->m_color, 0);
            row--;
        }
    }
    if (m_scroll == 1)
    {
        int n = m_text.Count();
        int i = m_text.Index(m_top);
        float t = n > 5 ? (float)i / (float)(n - 5) : 0.0f;
        VIVect2I at = m_window.TopLeft() + VIVect2I(10, 10);
        m_window.DrawVScroll(at, m_window.BottomRight().y - m_window.TopLeft().y - 40, t, 0);
    }
    ui->End2D();
    if (m_height < m_target)
    {
        int h = m_height + (int)(dt * 200.0f);
        m_height = h < m_target ? h : m_target;
    }
    else if (m_target < m_height)
    {
        int h = m_height - (int)(dt * 200.0f);
        m_height = h < m_target ? m_target : h;
    }
}

// 0x00C32620.  Up and down scroll, repeating while held.
void VIWndChat::OnKeyDown(int key)
{
    GetAppPtr();
    m_heldKey = 0;
    switch (key)
    {
    case 19:
    case 138:
        m_heldKey = key;
        m_repeat = 0.45f;
        ScrollUp();
        break;
    case 20:
    case 139:
        m_heldKey = key;
        m_repeat = 0.45f;
        ScrollDown();
        break;
    case 146:
        VIWnd::OnKeyDown(146);
        break;
    }
}

// 0x00C326F0.
void VIWndChat::OnKeyUp(int key)
{
    if (key == m_heldKey)
        m_heldKey = 0;
    switch (key)
    {
    case 19:
    case 20:
    case 138:
    case 139:
        break;
    default:
        m_heldKey = 0;
        VIWnd::OnKeyUp(key);
        break;
    }
}

// 0x00C32748.
void VIWndChat::AddText(const char *s)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (app->m_ui.m_font.FontLoaded(app->m_ui.m_dict))
        m_text.AddText(s, sWhite);
}

// 0x00C327A0.
void VIWndChat::AddText(const unsigned short *s)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (app->m_ui.m_font.FontLoaded(app->m_ui.m_dict))
        m_text.AddText(s, sWhite);
}

// 0x00C327F8.
void VIWndChat::AddText(const unsigned short *s, VIColor32F c)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (app->m_ui.m_font.FontLoaded(app->m_ui.m_dict))
        m_text.AddText(s, c);
}

// 0x00C32868.
void VIWndChat::AddWord(unsigned short id)
{
    VIClient *app = (VIClient *)GetAppPtr();
    AddText(Word(&app->m_ui, id));
}

// 0x00C328B8.
void VIWndChat::AddWord(unsigned short id, VIColor32F c)
{
    VIClient *app = (VIClient *)GetAppPtr();
    AddText(Word(&app->m_ui, id), c);
}

// 0x00C32928.  Mode 0 sizes the box to the lines it shows (two rows high when empty); any
// other mode is the fixed 85-pixel box.
void VIWndChat::SetMode(int mode)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    int h;
    m_mode = mode;
    if (mode == 0)
    {
        m_lines = CountLines();
        if (m_lines == 0)
            h = -((app->m_ui.m_font.GetHeight(ui->m_raster, ui->m_dict) - 4) * 2);
        else
            h = (app->m_ui.m_font.GetHeight(ui->m_raster, ui->m_dict) - 4) * m_lines;
        m_target = h;
        m_height = h;
    }
    else
    {
        m_target = 85;
        m_height = 85;
    }
}

// 0x00C329D8.  The lines the box shows: every live line, every line while scrolling, and
// five in a fixed mode.
int VIWndChat::CountLines()
{
    int n = 0;
    if (m_mode)
        n = 5;
    else
    {
        for (int i = m_text.First(); i != m_text.End(); i = m_text.Next(i))
        {
            VIChatLine *l = m_text.Line(i);
            if (l && (l->m_type == 1 || m_scroll == 1))
                n++;
        }
    }
    if (n < 0)
        n = 0;
    return n > 5 ? 5 : n;
}

// 0x00C32AA0.  Scrolling opens the view on the last five lines.
void VIWndChat::SetScroll(int on)
{
    GetAppPtr();
    m_scroll = on;
    if (on == 0)
        m_bottom = 0;
    else if (on == 1)
    {
        m_target = 85;
        m_height = 85;
        m_top = m_bottom = m_text.End();
        for (int i = 0; i < 5 && m_top != m_text.First(); i++)
            m_top = m_text.Prev(m_top);
    }
    else
        m_bottom = 0;
}

// 0x00C32B50.
void VIWndChat::ScrollDown()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_bottom != m_text.End())
    {
        app->PlaySound(11);
        m_bottom = m_text.Next(m_bottom);
        m_top = m_text.Next(m_top);
    }
    else
        app->PlaySound(9);
}

// 0x00C32BD8.
void VIWndChat::ScrollUp()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_top != m_text.First())
    {
        app->PlaySound(11);
        m_top = m_text.Prev(m_top);
        m_bottom = m_text.Prev(m_bottom);
    }
    else
        app->PlaySound(9);
}

// 0x00C32C60.  Losing the focus ends the scrolling.
void VIWndChat::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295 && p1 == (int)this && p2 != (int)this)
    {
        m_heldKey = 0;
        SetScroll(0);
    }
}
