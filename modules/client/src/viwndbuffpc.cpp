#line 1 "C:/code-frontiers-gm/game/VIWndBuffPC.cpp"
// viwndbuffpc.cpp -- CLIENT 0x00C1E278 .. 0x00C1FB90: a two-digit number picker.
// Left/right choose the digit, up/down change it, Enter gives the number to
// VIClient::ShowA110 and closes.  The tens run 0..7; the units 0..9, or 0..5 under 70.
#include "game.h"

// One WORDS.TXT entry drawn at (x, y).
static inline void DrawWord(VIUI *ui, VIFont *font, int id, int x, int y)
{
    font->DrawString(ui->m_raster, ui->m_dict,
                     (const unsigned short *)ui->m_words.GetString(id, ui->m_language), x, y);
}

// 0x00C1E278.
VIWndBuffPC::VIWndBuffPC()
{
    m_sel = 0;
    m_p40 = 1.0f;
    m_p28 = 2;
    m_time = 0;
    m_p44 = 1.0f;
    m_p48 = 1;
}

// 0x00C1E2E0.
VIWndBuffPC::~VIWndBuffPC()
{
    Free();
}

// 0x00C1E328.
int VIWndBuffPC::Init()
{
    m_p28 = 2;
    m_sel = 0;
    m_p40 = 1.0f;
    m_p44 = 1.0f;
    m_time = 0;
    m_p48 = 1;
    m_digits[0] = 0;
    m_digits[1] = 0;
    return 0;
}

// 0x00C1E360.
int VIWndBuffPC::Free()
{
    m_sel = 0;
    m_p28 = 2;
    return 0;
}

// 0x00C1E9F8.
void VIWndBuffPC::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 13:
    case 134:
        app->ShowA110(Value());
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 19:
    case 138:
        AdjustDigit(1);
        break;
    case 20:
    case 139:
        AdjustDigit(-1);
        break;
    case 17:
    case 136:
        MoveSel(-1);
        break;
    case 18:
    case 137:
        MoveSel(1);
        break;
    }
}

// 0x00C1EB80.
void VIWndBuffPC::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if ((VIWndBuffPC *)p2 == this)
        {
            m_p40 = 0.25f;
            m_p44 = 1.0f;
            m_p48 = 1;
            m_time = app->m_p07568;
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C1EC30.
void VIWndBuffPC::MoveSel(int delta)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel + delta < 2 - m_p28)
    {
        app->PlaySound(9);
        m_sel = 2 - m_p28;
    }
    else if (m_sel + delta >= 2)
    {
        app->PlaySound(9);
        m_sel = 1;
    }
    else
    {
        m_sel = m_sel + delta;
        app->PlaySound(11);
    }
    m_p48 = 1;
    m_p44 = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C1ECF8.
void VIWndBuffPC::ClearDigits()
{
    for (int i = 0; i < 2; i++)
        m_digits[i] = 0;
}

// 0x00C1ED28.
void VIWndBuffPC::AdjustDigit(int delta)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_digits[m_sel] = m_digits[m_sel] + delta;
    if (m_sel == 0)
    {
        if (m_digits[0] >= 8)
            m_digits[0] = 0;
        if (m_digits[m_sel] < 0)
            m_digits[m_sel] = 7;
    }
    else if (m_digits[0] == 7)
    {
        if (m_digits[1] >= 6)
            m_digits[1] = 0;
        if (m_digits[1] < 0)
            m_digits[1] = 5;
    }
    else
    {
        if (m_digits[1] >= 10)
            m_digits[1] = 0;
        if (m_digits[1] < 0)
            m_digits[1] = 9;
    }
    app->PlaySound(11);
    m_p48 = 1;
    m_p44 = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C1EE38.
int VIWndBuffPC::Value()
{
    int v = 0;
    for (int i = 1; i >= 0; i--)
        v += m_digits[1 - i] * (int)pow(10.0, (double)i);
    return v;
}

// 0x00C1EEB0.
void VIWndBuffPC::SetDefault()
{
    int v = 50;
    ClearDigits();
    for (int i = 1; i >= 0; i--)
    {
        m_digits[1 - i] = v / (int)pow(10.0, (double)i);
        v -= m_digits[1 - i] * (int)pow(10.0, (double)i);
    }
}

// 0x00C1E378.  The frame, the title, the two digits (the chosen one yellow with arrows
// over and under it, pulsing), and the two button hints.
void VIWndBuffPC::OnDrawSelf()
{
    unsigned short buf[32];
    buf[0] = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIColor32F color;
    VIVect2I pen;
    VIVect2I unused;
    VIVect2I pos = m_pos;
    VIVect2I size(290, 180);
    VIVect2I unused2;
    VIVect2I org = pos + VIVect2I(15, 15);
    VIUI *ui = &app->m_ui;
    long now = app->m_p07568;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    ui->Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, m_p40);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, m_p40);
    GetUITexture((VIUITexId)101);
    VIUIFrameParam frame;
    memset(&frame, 0, sizeof(frame));
    frame.m_tl.m_id = (VIUITexId)101;
    frame.m_top.m_id = (VIUITexId)102;
    frame.m_tr.m_id = (VIUITexId)103;
    frame.m_right.m_id = (VIUITexId)104;
    frame.m_br.m_id = (VIUITexId)106;
    frame.m_bottom.m_id = (VIUITexId)102;
    frame.m_bl.m_id = (VIUITexId)105;
    frame.m_left.m_id = (VIUITexId)104;
    ui->StretchBlitEx((VIUITexId)61, pos, size, (VIAlphaBlendMode)3, VIMIN(m_p40, 0.75f));
    ui->BlitFrame(pos, size, frame, (VIAlphaBlendMode)3, m_p40, 1);
    VIFont *font = &ui->m_font;
    org.y += 19;
    int w = font->GetCharacterWidth(ui->m_raster, ui->m_dict, 87) - 6;
    pen = org;
    font->SetTextColor(white);
    DrawWord(ui, font, 654, pen.x, pen.y);
    pen.x = pen.x + 40 + font->GetStringWidth(ui->m_raster, ui->m_dict,
                     (const unsigned short *)ui->m_words.GetString(654, ui->m_language));
    for (int i = 2 - m_p28; i < 2; i++)
    {
        color = i == m_sel ? yellow : white;
        font->SetTextColor(color);
        VIString::snprintf16(buf, 32, "%d", m_digits[i]);
        font->DrawString(ui->m_raster, ui->m_dict, buf, pen.x, pen.y);
        if (i == m_sel)
        {
            VIVect2I at = pen + VIVect2I(-2, -14);
            ui->BlitEx((VIUITexId)87, at, (VIAlphaBlendMode)3, VIMIN(m_p44, m_p40), 0);
            at += VIVect2I(0, 38);
            ui->BlitEx((VIUITexId)90, at, (VIAlphaBlendMode)3, VIMIN(m_p44, m_p40), 0);
        }
        pen.x += w;
    }
    font->SetTextColor(white);
    org.y += 19 * 2 + 19;
    ui->BlitEx((VIUITexId)70, pos + VIVect2I(25, 150), (VIAlphaBlendMode)3, m_p40, 0);
    DrawWord(ui, font, 214, pos.x + 50, pos.y + 146);
    ui->BlitEx((VIUITexId)69, pos + VIVect2I(125, 150), (VIAlphaBlendMode)3, m_p40, 0);
    DrawWord(ui, font, 217, pos.x + 150, pos.y + 146);
    ui->End2D();
    m_p40 += dt * 5.0f;
    m_p40 = VIMIN(m_p40, 1.0f);
    if (m_p48 == -1)
    {
        m_p44 -= dt * 2.0f;
        m_p44 = VIMAX(m_p44, 0.0f);
        if (m_p44 == 0.0f)
            m_p48 = 1;
    }
    else
    {
        m_p44 += dt * 3.0f;
        m_p44 = VIMIN(m_p44, 1.0f);
        if (m_p44 == 1.0f)
            m_p48 = -1;
    }
}
