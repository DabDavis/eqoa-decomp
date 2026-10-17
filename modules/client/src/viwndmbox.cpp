#line 1 "C:/code-frontiers-gm/game/VIWndMBox.cpp"
// VIWndMBox (name the RTTI's) -- CLIENT 0x00C6FA18 .. 0x00C719A0.  STATION's message box
// (modules/station/src/viwndmbox.cpp) as CLIENT builds it: the same layout plus an item
// icon at +0x5D8, sounds through VIClient::PlaySound and the client's clock for the fade
// and the countdown.  Method names STATION's.
#include "game.h"

// 0x00C6FA18.
VIWndMBox::VIWndMBox()
{
    SetPos(VIVect2I(0, 0));
    m_timeout = -1;
    m_icon = -1;
    m_text[0] = 0;
    m_type = 0;
    m_sel = 0;
    m_p1 = 0;
    m_p2 = 0;
    m_time = 0;
}

// 0x00C6FAA0.
VIWndMBox::~VIWndMBox()
{
}

// 0x00C6FAF0.
int VIWndMBox::Init(int type, int style)
{
    m_type = type;
    m_timeout = -1;
    m_style = style;
    m_icon = -1;
    m_p1 = 0;
    m_p2 = 0;
    m_time = 0;
    Clear();
    return 0;
}

// 0x00C6FB38.
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

// 0x00C6FB60.  As STATION's, with the icon: it takes the two top rows of the text box.
void VIWndMBox::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
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
    int step;
    do
    {
        step = width / 6;
        width -= step;
        n = m_window.CountLines(m_text, width);
    } while (width > 100 && lines == n);
    width += step;
    n = m_window.CountLines(m_text, width);
    size.x = width + 32;
    if (m_icon != -1)
        n += 2;
    size.y = (n + 2) * (ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 4) + 32;
    m_window.Init(VIWnd::mUI, VIVect2I(0, 0),
                  VIVect2I(pos.x + size.x / 2, size.y / 2 + pos.y - 16), width, n, 1);
    m_window.Open(true);
    Update();
    ui->Begin2D();
    ui->Blit(UIImageId("black"), pos, size, VIVect2I(0, 0), (VIAlphaBlendMode)3,
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
    if (m_icon != -1)
        DrawIcon(VIVect2I(pos.x - 57 + size.x / 2, pos.y + 4));
    m_window.TextBox(m_text, 0, m_icon != -1 ? 2 : 0, width, m_icon != -1 ? n - 2 : n,
                     VIColor32F(1.0f, 1.0f, 1.0f, m_alpha), 9);
    int dy = (ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 4) * (n + 1) + 22;
    int y = pos.y + dy;
    int col[3] = { pos.x + (size.x >> 2), pos.x + (size.x >> 1), pos.x + (size.x >> 2) * 3 };
    if (m_timeout != -1 && m_timeout - app->m_p07568 < 11000)
    {
        char num[8];
        VIString::snprintf(num, sizeof(num), "%d", (int)((m_timeout - app->m_p07568) / 1000));
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

// 0x00C70498.  The item icon out of the client's icon cache (VIWndList::DrawIcon's code
// with the box's own icon, fade and blends).
int VIWndMBox::DrawIcon(const VIVect2I &pos)
{
    if (m_icon == -1)
        return -1;
    VIClient *app = (VIClient *)GetAppPtr();
    VIVect2I size(114, 44);
    VIUI *ui = &app->m_ui;
    VIVect2I zero(0, 0);
    VIClientPart06D50 *icons = &app->m_p06D50;
    int type;
    int tex;
    if (!icons)
    {
        ui->Blit(m_icon, pos, size, zero, (VIAlphaBlendMode)3, m_alpha, 0);
        return 0;
    }
    if (icons->Get(m_icon, type, tex) < 0)
        return -1;
    int ret;
    switch (type)
    {
    case 0:
        ret = ui->Blit(m_icon, pos, size, zero, (VIAlphaBlendMode)3, m_alpha, 0);
        break;
    case 1:
        ret = ui->BlitEx((VIUITexId)1, pos, size, (VIAlphaBlendMode)3, m_alpha, 0);
        break;
    case 2:
        ui->BlitEx((VIUITexId)1, pos, size, (VIAlphaBlendMode)3, m_alpha, 0);
        ret = ui->BlitEx((VIUITexId)10, pos + VIVect2I(42, 0), (VIAlphaBlendMode)3, m_alpha, 0);
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

// 0x00C706D8.  The fade runs to 1 in a fifth of a second; a pending timeout that has
// passed cancels the box.
void VIWndMBox::Update()
{
    VIClient *app = (VIClient *)GetAppPtr();
    long now = app->m_p07568;
    m_alpha += (float)(now - m_time) * 0.001f * 5.0f;
    m_time = now;
    m_alpha = VIMIN(m_alpha, 1.0f);
    if (m_timeout != -1 && m_timeout < app->m_p07568)
    {
        GetParent()->OnMessage(this, 260, m_p1, m_p2);
        m_timeout = -1;
    }
}

// 0x00C707B0.
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

// 0x00C708A8.
void VIWndMBox::Accept()
{
    ((VIClient *)GetAppPtr())->PlaySound(13);
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

// 0x00C709F8.
void VIWndMBox::Cancel()
{
    ((VIClient *)GetAppPtr())->PlaySound(8);
    GetParent()->OnMessage(this, 260, m_p1, m_p2);
    ResetSel();
}

// 0x00C70A58.  Left/right between the two buttons; a one-button box buzzes.
void VIWndMBox::Move(int dir)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_type)
    {
    case 0:
    case 2:
        app->PlaySound(9);
        m_sel = 0;
        break;
    case 1:
        m_sel += dir;
        if (m_sel < 0 || m_sel > 1)
            app->PlaySound(9);
        else
            app->PlaySound(11);
        m_sel = VIMIN(VIMAX(m_sel, 0), 1);
        break;
    }
}

// 0x00C70B18.
void VIWndMBox::SetSel(int sel)
{
    switch (m_type)
    {
    case 0:
    case 2:
        m_sel = 0;
        break;
    case 1:
        m_sel = VIMIN(VIMAX(sel, 0), 1);
        break;
    }
}

// 0x00C70B78.
void VIWndMBox::OnKeyUp(int key)
{
    VIWnd::OnKeyUp(key);
}

// 0x00C70B98.  Gaining the focus starts the fade.
void VIWndMBox::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_alpha = 0;
            m_time = app->m_p07568;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(0, 0), 300, 3, 0);
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C70C60.
void VIWndMBox::SetText(const char *s)
{
    VIString::str8to16(m_text, sizeof(m_text), s);
}

// 0x00C70C88.
void VIWndMBox::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x00C70CB0.
void VIWndMBox::SetText(unsigned short id)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    SetText((const unsigned short *)ui->m_words.GetString(id, ui->m_language));
}

// 0x00C70D00.  A Yes/No box starts on No.
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
