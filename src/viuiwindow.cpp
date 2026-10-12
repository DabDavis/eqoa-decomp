// ---------------------------------------------------------------------------
// viuiwindow.cpp -- SUPPORT 0x004A8878 .. 0x004AB818.  Phase 42.
// ---------------------------------------------------------------------------
#include "vi.h"

extern "C" char *strcpy(char *d, const char *s);
extern "C" float sinf(float);

VIUIWindow::VIUIWindow()
{
    m_inited = 0;
    m_open = 0.0f;
    m_dir = 0.0f;
    m_title[0] = 0;
    m_pulseTime = 0;
}

VIUIWindow::~VIUIWindow()
{
}

int VIUIWindow::Init(VIUI *ui, const VIVect2I &pos, const VIVect2I &center,
                     int w, int rows, int x)
{
    m_ui = ui;
    int h = ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    m_pos = pos;
    m_center = center;
    m_size.x = w;
    m_rows = rows;
    m_x178 = x;
    m_titleH = 18;
    m_size.y = rows * (h - 4) + 36;
    m_titleH = 18;
    m_size.y = rows * (h - 4) + 36;
    SetUiColors();
    m_inited = 1;
    return 0;
}

int VIUIWindow::Init(VIUI *ui, const VIVect2I &pos, const VIVect2I &center,
                     const VIVect2I &size, int x)
{
    m_ui = ui;
    ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    m_pos = pos;
    m_center = center;
    m_size = size;
    m_x178 = x;
    m_titleH = 0;
    m_rows = 0;
    SetUiColors();
    m_inited = 1;
    return 0;
}

void VIUIWindow::Free()
{
    m_inited = 0;
}

VIVect2I VIUIWindow::RowCenter(int row)
{
    VIVect2I v;
    int h = m_ui->m_font.GetHeight(m_ui->m_raster, m_ui->m_dict);
    int t = (h - 4) * row + 5;
    v.x = m_center.x - 8;
    v.y = m_center.y - (m_size.y >> 1) + m_titleH + t;
    return v;
}

void VIUIWindow::SetTitle(const char *s)
{
    int h = 18;
    if (s != 0)
        h = 24;
    int old = m_titleH;
    m_titleH = h;
    m_size.y = m_size.y - old * 2 + h * 2;
    strcpy(m_title, s);
}

void VIUIWindow::SetTitle(const unsigned short *s)
{
    int h = 18;
    if (s != 0)
        h = 24;
    int old = m_titleH;
    m_titleH = h;
    m_size.y = m_size.y - old * 2 + h * 2;
    VIString::str16to8(m_title, 256, s);
}

void VIUIWindow::Resize(int w, int rows)
{
    int h = m_ui->m_font.GetHeight(m_ui->m_raster, m_ui->m_dict);
    m_size.x = w;
    m_size.y = rows * (h - 4) + m_titleH * 2;
}

const unsigned short *VIUIWindow::T(const char *s)
{
    static unsigned short sBuf[1024];
    VIString::str8to16(sBuf, 2048, s);
    return sBuf;
}

const unsigned short *VIUIWindow::T(const unsigned short *s)
{
    return s;
}

const unsigned short *VIUIWindow::T(unsigned short id)
{
    return (const unsigned short *)m_ui->m_words.GetString(id, m_ui->m_language);
}

int VIUIWindow::Text(const unsigned short *s, int x, int y,
                     const VIColor32F &c, unsigned int f)
{
    VIVect2I at = RowCenter(y);
    VIColor32F col = c;
    at.x = x + (m_center.x - (m_size.x >> 1));
    float d = 1.0f - m_open;
    d = 1.0f - d * d;
    col.a = col.a * d;
    return TextBox(s, x, y, 640, 1, c, f);
}

int VIUIWindow::Text(const unsigned short *s, int x, int y, VIUICOLOR c,
                     unsigned int f)
{
    return Text(s, x, y, m_colors[c], f);
}

int VIUIWindow::TextBox(const unsigned short *s, int x, int y, int w, int h,
                        VIUICOLOR c, unsigned int f)
{
    return TextBox(s, x, y, w, h, m_colors[c], f);
}

int VIUIWindow::CountLines(const unsigned short *s, int w)
{
    return TextBox(s, 0, 0, w, 256, (VIUICOLOR)0, 0xffffffff);
}

void VIUIWindow::DrawVScroll(VIVect2I at, int h, float t, unsigned int flags)
{
    VIVect2I p = at;
    const VIUITexEntry *e = GetUITexture((VIUITexId)84);
    m_ui->BlitVertBar(p, (VIUITexId)81, (VIUITexId)83, (VIUITexId)82, h,
                      (VIAlphaBlendMode)3, m_open);
    if (t >= 0.0f)
    {
        if (t <= 1.0f)
        {
            p.x = p.x - 1;
            p.y = p.y + (int)(t * (float)(h - e->m_uv1.y));
            m_ui->BlitEx((VIUITexId)e->m_x00, p, (VIAlphaBlendMode)3, m_open, 0);
        }
    }
}

int VIUIWindow::TickR(int x, int row, bool pulse)
{
    VIVect2I at = RowCenter(row);
    at.x = m_center.x - (m_size.x >> 1) + x - 16;
    at.y = at.y - 5;
    return m_ui->BlitEx((VIUITexId)89, at, (VIAlphaBlendMode)3,
                        pulse ? TickPulse() : 1.0f, 0);
}

int VIUIWindow::TickU(int x, int row, bool pulse)
{
    VIVect2I at = RowCenter(row);
    at.x = m_center.x - (m_size.x >> 1) + x - 8;
    at.y = at.y - 5;
    return m_ui->BlitEx((VIUITexId)90, at, (VIAlphaBlendMode)3,
                        pulse ? TickPulse() : 1.0f, 0);
}

int VIUIWindow::TickD(int x, int row, bool pulse)
{
    VIVect2I at = RowCenter(row);
    at.x = m_center.x - (m_size.x >> 1) + x - 8;
    at.y = at.y - 5;
    return m_ui->BlitEx((VIUITexId)87, at, (VIAlphaBlendMode)3,
                        pulse ? TickPulse() : 1.0f, 0);
}

float VIUIWindow::TickPulse() const
{
    float d = 1.0f - m_open;
    d = 1.0f - d * d;
    float t = (float)(VITimer::GetTime() - m_pulseTime) * 0.00999999978f;
    if (t > 6.28318548f)
        t -= 6.28318548f;
    return d * (sinf(t + 1.57079637f) * 0.5f + 0.5f);
}

void VIUIWindow::ResetTickPulse()
{
    m_pulseTime = VITimer::GetTime();
}

int VIUIWindow::IsOpening() const
{
    if (m_open < 1.0f)
    {
        if (m_dir == 1.0f)
            return 1;
    }
    return 0;
}

int VIUIWindow::IsOpen() const
{
    if (m_open == 1.0f)
        return 1;
    return 0;
}

void VIUIWindow::Open(bool now)
{
    if (now)
        m_open = 1.0f;
    m_dir = 1.0f;
}

int VIUIWindow::IsClosing() const
{
    if (m_open > 0.0f)
    {
        if (m_dir == -1.0f)
            return 1;
    }
    return 0;
}

int VIUIWindow::IsClosed() const
{
    if (m_open == 0.0f)
        return 1;
    return 0;
}

void VIUIWindow::Close(bool now)
{
    if (now)
        m_open = 0.0f;
    m_dir = -1.0f;
}

void VIUIWindow::CopyAsteriks(unsigned short *d, int n, int max)
{
    max = VIMIN(max, n);
    int i = max;
    if (i > 0)
    {
        do
        {
            *d = '*';
            i--;
            d++;
        } while (i);
    }
}

void VIUIWindow::AppendEllipsis(unsigned short *s, int w)
{
    int n = VIString::strlen16(s);
    int ew = m_ui->m_font.GetStringWidth(m_ui->m_raster, m_ui->m_dict, "...");
    if (ew < w)
    {
        unsigned short *p = (unsigned short *)(n * 2 + (unsigned int)s);
        while (TextWidth(s) + ew >= w)
        {
            p--;
            *p = 0;
        }
        VIString::strcat16(s, (255 - VIString::strlen16(s)) * 2, "...");
    }
}

void VIUIWindow::SetUiColors()
{
    m_colors[0] = VIColor32F(0.0f, 0.0f, 0.0f, 1.0f);
    m_colors[1] = VIColor32F(0.0392156877f, 0.196078435f, 0.274509817f, 1.0f);
    m_colors[2] = VIColor32F(0.400000006f, 0.400000006f, 0.400000006f, 1.0f);
    m_colors[3] = VIColor32F(0.899999976f, 0.899999976f, 0.899999976f, 1.0f);
}

void VISetWindowUI(VIUI *ui)
{
    VIWnd::mFocus = 0;
    VIWnd::mUI = ui;
}
