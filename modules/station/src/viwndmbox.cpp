// viwndmbox.cpp (name ours) -- STATION 0x002F0128 .. 0x002F1DB8.  The customer
// service message box: a black frame fading in, the text, an optional
// countdown and one or two choice buttons.  It answers its parent 268 (accept)
// or 260 (cancel, or the countdown running out), with the two parameters the
// opener set.
#include "station.h"

// The app's VIUI: VIStation::m_ui, +0xAFE0 of the window's app pointer.  This
// box draws with it rather than with VIWnd::UI().  (VIStation's layout is the
// core lane's; the offset is measured here.)
static inline VIUI *AppUI(VIWnd *w)
{
    return (VIUI *)((char *)w->GetAppPtr() + 0xAFE0);
}

// The same, off an app pointer already fetched.
static inline VIUI *UIOf(void *app)
{
    return (VIUI *)((char *)app + 0xAFE0);
}

// 0x002F0128.
VIWndMBox::VIWndMBox()
{
    SetPos(VIVect2I(0, 0));
    m_timeout = -1;
    m_text[0] = 0;
    m_type = 0;
    m_sel = 0;
    m_p1 = 0;
    m_p2 = 0;
    m_time = 0;
}

// 0x002F01A8.
VIWndMBox::~VIWndMBox()
{
}

// 0x002F01F8.
int VIWndMBox::Init(int type, int style)
{
    m_type = type;
    m_timeout = -1;
    m_style = style;
    m_p1 = 0;
    m_p2 = 0;
    m_time = 0;
    Clear();
    return 0;
}

// 0x002F0238.
void VIWndMBox::Clear()
{
    ResetSel();
    m_text[0] = 0;
}

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x002F0260.  The text box is narrowed by sixths from 375 wide until the text
// takes more lines (or it would be under 101 wide), then widened one step back.
// Over it: the black backdrop at three quarters of the fade, the frame (the
// same part sets as VIUIWindow::Draw's fp[3] and fp[0]), the text, the seconds
// left when a timeout is under 11 s away, and the buttons with the selection
// arrows (89, 88) around the chosen one.
void VIWndMBox::OnDrawSelf()
{
    VIUI *ui = AppUI(this);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, m_alpha);
    VIColor32F white(1.0f, 1.0f, 1.0f, m_alpha);
    VIVect2I pos;
    VIVect2I ctr;
    VIVect2I size;
    VIUIFrameParam fp[4];

    memset(fp, 0, sizeof(fp));
    int width = 375;
    pos = m_pos;
    int lines = m_window.CountLines(m_text, width);
    int n;
    do
    {
        width -= width / 6;
        n = m_window.CountLines(m_text, width);
    } while (width > 100 && lines == n);
    width += width / 6;
    n = m_window.CountLines(m_text, width);
    size.x = width + 32;
    size.y = (n + 2) * (ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 4) + 32;
    m_window.Init(UI(), VIVect2I(0, 0),
                  VIVect2I(pos.x + size.x / 2, size.y / 2 + pos.y - 16), width, n, 1);
    m_window.Open(true);
    Update();
    ui->Begin2D();
    ui->Blit(VIHashResourceID_Icon("black"), pos, size, VIVect2I(0, 0), (VIAlphaBlendMode)3,
             m_alpha * 0.75f, 0);
    switch (m_style)
    {
    case 1:
        fp[0].m_bottom.m_id = (VIUITexId)102;
        fp[0].m_bottom.m_flags = 2;
        fp[0].m_bl.m_id = (VIUITexId)105;
        fp[0].m_br.m_id = (VIUITexId)106;
        fp[0].m_left.m_id = (VIUITexId)104;
        fp[0].m_right.m_id = (VIUITexId)104;
        fp[0].m_right.m_flags = 1;
        fp[0].m_top.m_id = (VIUITexId)102;
        fp[0].m_tl.m_id = (VIUITexId)101;
        fp[0].m_tr.m_id = (VIUITexId)103;
        ui->BlitFrame(pos, size, fp[0], (VIAlphaBlendMode)3, m_alpha, 1);
        break;
    case 2:
        fp[0].m_bottom.m_id = (VIUITexId)37;
        fp[0].m_bottom.m_flags = 0;
        fp[0].m_left.m_id = (VIUITexId)42;
        fp[0].m_bl.m_id = (VIUITexId)36;
        fp[0].m_br.m_id = (VIUITexId)36;
        fp[0].m_br.m_flags = 1;
        fp[0].m_right.m_id = (VIUITexId)42;
        fp[0].m_right.m_flags = 1;
        fp[0].m_top.m_id = (VIUITexId)37;
        fp[0].m_tl.m_id = (VIUITexId)36;
        fp[0].m_tr.m_id = (VIUITexId)36;
        fp[0].m_tr.m_flags = 1;
        ui->BlitFrame(pos, size, fp[0], (VIAlphaBlendMode)3, m_alpha, 1);
        break;
    }
    m_window.TextBox(m_text, 0, 0, width, n, VIColor32F(1.0f, 1.0f, 1.0f, m_alpha), 9);
    int dy = (ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 4) * (n + 1) + 22;
    int y = pos.y + dy;
    int col[3] = { pos.x + (size.x >> 2), pos.x + (size.x >> 1), pos.x + (size.x >> 2) * 3 };
    if (m_timeout != -1 && m_timeout - VITimer::GetTime() < 11000)
    {
        unsigned short num[8];
        VIString::snprintf16(num, sizeof(num), "%d",
                             (int)((m_timeout - VITimer::GetTime()) / 1000));
        ui->m_font.DrawString(ui->m_raster, ui->m_dict, num, pos.x + 16, y);
    }
    int w;
    switch (m_type)
    {
    case 1:
        ui->DrawCenteredStringEx((unsigned short)21, col[0], y, m_sel == 0 ? yellow : white, 3, 1);
        ui->DrawCenteredStringEx((unsigned short)22, col[2], y, m_sel == 1 ? yellow : white, 3, 1);
        if (m_sel == 0)
        {
            w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 21)) >> 1;
            y -= 5;
            ui->BlitEx((VIUITexId)89, VIVect2I(col[0] + (-20 - w), y), (VIAlphaBlendMode)3, m_alpha, 0);
            ui->BlitEx((VIUITexId)88, VIVect2I(col[0] + w + 8, y), (VIAlphaBlendMode)3, m_alpha, 0);
        }
        else
        {
            w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 22)) >> 1;
            y -= 5;
            ui->BlitEx((VIUITexId)89, VIVect2I(col[2] + (-20 - w), y), (VIAlphaBlendMode)3, m_alpha, 0);
            ui->BlitEx((VIUITexId)88, VIVect2I(col[2] + w + 8, y), (VIAlphaBlendMode)3, m_alpha, 0);
        }
        break;
    case 0:
        ui->DrawCenteredStringEx((unsigned short)214, col[1], y, yellow, 3, 1);
        w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 214)) >> 1;
        y -= 5;
        ui->BlitEx((VIUITexId)89, VIVect2I(col[1] + (-20 - w), y), (VIAlphaBlendMode)3, m_alpha, 0);
        ui->BlitEx((VIUITexId)88, VIVect2I(col[1] + w + 8, y), (VIAlphaBlendMode)3, m_alpha, 0);
        break;
    case 2:
        ui->DrawCenteredStringEx((unsigned short)213, col[1], y, yellow, 3, 1);
        w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 213)) >> 1;
        y -= 5;
        ui->BlitEx((VIUITexId)89, VIVect2I(col[1] + (-20 - w), y), (VIAlphaBlendMode)3, m_alpha, 0);
        ui->BlitEx((VIUITexId)88, VIVect2I(col[1] + w + 8, y), (VIAlphaBlendMode)3, m_alpha, 0);
        break;
    }
    ui->End2D();
}

// 0x002F0B50.  The fade runs to 1 in a fifth of a second; a pending timeout
// that has passed cancels the box.
void VIWndMBox::Update()
{
    GetAppPtr();
    long now = VITimer::GetTime();
    m_alpha += (float)(now - m_time) * 0.001f * 5.0f;
    m_time = now;
    m_alpha = VIMIN(m_alpha, 1.0f);
    if (m_timeout != -1 && m_timeout < VITimer::GetTime())
    {
        GetParent()->OnMessage(this, 260, m_p1, m_p2);
        m_timeout = -1;
    }
}

// 0x002F0C30.
void VIWndMBox::OnKeyDown(int key)
{
    GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
        Accept();
        break;
    case 27:
    case 132:
        Cancel();
        break;
    case 17:
    case 136:
        Move(-1);
        break;
    case 18:
    case 137:
        Move(1);
        break;
    }
}

// 0x002F0D28.
void VIWndMBox::Accept()
{
    AppUI(this)->SoundPlay((VIUISoundId)13);
    switch (m_type)
    {
    case 0:
        GetParent()->OnMessage(this, 268, m_p1, m_p2);
        ResetSel();
        m_timeout = -1;
        break;
    case 1:
        switch (m_sel)
        {
        case 0:
            GetParent()->OnMessage(this, 268, m_p1, m_p2);
            break;
        case 1:
            GetParent()->OnMessage(this, 260, m_p1, m_p2);
            break;
        }
        ResetSel();
        m_timeout = -1;
        break;
    case 2:
        GetParent()->OnMessage(this, 260, m_p1, m_p2);
        m_timeout = -1;
        break;
    }
}

// 0x002F0E78.
void VIWndMBox::Cancel()
{
    AppUI(this)->SoundPlay((VIUISoundId)8);
    GetParent()->OnMessage(this, 260, m_p1, m_p2);
    ResetSel();
}

// 0x002F0EE0.  Left/right between the two buttons; a one-button box buzzes.
void VIWndMBox::Move(int dir)
{
    void *app = GetAppPtr();
    switch (m_type)
    {
    case 0:
    case 2:
        UIOf(app)->SoundPlay((VIUISoundId)9);
        m_sel = 0;
        break;
    case 1:
        m_sel += dir;
        if (m_sel < 0 || m_sel > 1)
            UIOf(app)->SoundPlay((VIUISoundId)9);
        else
            UIOf(app)->SoundPlay((VIUISoundId)11);
        m_sel = VIMIN(VIMAX(m_sel, 0), 1);
        break;
    }
}

// 0x002F0FB8.
void VIWndMBox::OnKeyUp(int key)
{
    VIWnd::OnKeyUp(key);
}

// 0x002F0FD8.  Gaining the focus starts the fade.
void VIWndMBox::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    GetAppPtr();
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_alpha = 0;
            m_time = VITimer::GetTime();
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(0, 0), 300, 3, 0);
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x002F10A0.
void VIWndMBox::SetText(const char *s)
{
    VIString::str8to16(m_text, sizeof(m_text), s);
}

// 0x002F10C8.
void VIWndMBox::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x002F10F0.
void VIWndMBox::SetText(unsigned short id)
{
    VIUI *ui = AppUI(this);
    SetText((const unsigned short *)ui->m_words.GetString(id, ui->m_language));
}

// 0x002F1140.  A Yes/No box starts on No.
void VIWndMBox::ResetSel()
{
    switch (m_type)
    {
    case 0:
        m_sel = 0;
        break;
    case 1:
        m_sel = 1;
        break;
    case 2:
        m_sel = 0;
        break;
    default:
        m_sel = 0;
        break;
    }
}
