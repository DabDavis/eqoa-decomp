// ---------------------------------------------------------------------------
// viuiwindow.cpp -- SUPPORT 0x004A8878 .. 0x004AB818.  Phase 42.
// ---------------------------------------------------------------------------
#include "vi.h"

extern "C" char *strcpy(char *d, const char *s);
extern "C" float sinf(float);
extern "C" void *memset(void *d, int c, unsigned int n);

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

// Phase 61.  One frame of the window: four frame-parameter sets (only three
// are used -- fp[2] is filled and never drawn), the open/close animation
// stepped (+0.15 opening, -0.2 closing, eased as 1-(1-t)^2), the body drawn
// in one of three styles by the controller type at +0x178, and the title as a
// captioned bar above it.  Returns 1 once the animation has settled.
int VIUIWindow::Draw()
{
    VIVect2I tl;
    VIVect2I size;
    VIVect2I ctr;
    VIUIFrameParam fp[4];

    memset(fp, 0, sizeof(fp));

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

    fp[1].m_tl.m_id = (VIUITexId)44;
    fp[1].m_tl.m_flags = 0;
    fp[1].m_top.m_id = (VIUITexId)45;
    fp[1].m_top.m_flags = 0;
    fp[1].m_tr.m_id = (VIUITexId)46;
    fp[1].m_tr.m_flags = 0;
    fp[1].m_right.m_id = (VIUITexId)52;
    fp[1].m_right.m_flags = 0;
    fp[1].m_br.m_id = (VIUITexId)50;
    fp[1].m_br.m_flags = 0;
    fp[1].m_bottom.m_id = (VIUITexId)49;
    fp[1].m_bottom.m_flags = 0;
    fp[1].m_bl.m_id = (VIUITexId)48;
    fp[1].m_bl.m_flags = 0;
    fp[1].m_left.m_id = (VIUITexId)47;
    fp[1].m_left.m_flags = 0;

    fp[2].m_bottom.m_id = (VIUITexId)41;
    fp[2].m_left.m_id = (VIUITexId)43;
    fp[2].m_right.m_id = (VIUITexId)43;
    fp[2].m_right.m_flags = 1;
    fp[2].m_top.m_id = (VIUITexId)38;

    fp[3].m_bottom.m_id = (VIUITexId)102;
    fp[3].m_bottom.m_flags = 2;
    fp[3].m_bl.m_id = (VIUITexId)105;
    fp[3].m_br.m_id = (VIUITexId)106;
    fp[3].m_left.m_id = (VIUITexId)104;
    fp[3].m_right.m_id = (VIUITexId)104;
    fp[3].m_right.m_flags = 1;
    fp[3].m_top.m_id = (VIUITexId)102;
    fp[3].m_tl.m_id = (VIUITexId)101;
    fp[3].m_tr.m_id = (VIUITexId)103;

    if (m_dir > 0.0f)
        m_open = VIMIN(m_open + 0.15f, 1.0f);
    else
        m_open = (m_open - 0.2f < 0.0f) ? 0.0f : m_open - 0.2f;

    float k = 1.0f - m_open;
    k = 1.0f - k * k;

    // The two copies come FIRST and the corner is computed from them, so the
    // four loads of this feed both (any other order reloads two of them);
    // `ipos` is a COPY of tl offset in place, which is why tl.y is read back
    // through tl's constructor pointer (4(s7)) while tl.x is not.
    ctr = m_center;
    size = m_size;
    tl.x = ctr.x - (size.x >> 1);
    tl.y = ctr.y - (size.y >> 1);

    VIVect2I ipos = tl;
    ipos.x += 10;
    ipos.y += 10;
    VIVect2I isize(size.x - 20, size.y - 20);

    switch (m_x178)
    {
    case 0:
        m_ui->BlitFillFrame(ipos, VIVect2I(isize.x + 1, isize.y), fp[1],
                            (VIUITexId)35, (VIAlphaBlendMode)3, k, 1);
        m_ui->BlitFrame(tl, size, fp[0], (VIAlphaBlendMode)3, k, 1);
        break;

    case 1:
        m_ui->BlitFillFrame(ipos, isize, fp[3], (VIUITexId)61,
                            (VIAlphaBlendMode)3, k * 0.75f, 1);
        break;

    case 2:
        m_ui->BlitRect((VIUITexId)51, ipos, VIVect2I(isize.x + 1, isize.y),
                       (VIAlphaBlendMode)3, k);
        m_ui->BlitFrame(tl, size, fp[0], (VIAlphaBlendMode)3, k, 1);
        break;
    }

    if (m_title[0])
    {
        int tw = m_ui->m_font.GetStringWidth(m_ui->m_raster, m_ui->m_dict,
                                             m_title) + 50;
        m_ui->BlitBar(m_title, VIColor32F(1.0f, 1.0f, 1.0f, k),
                      VIVect2I(ctr.x - (tw >> 1), ipos.y - 21), (VIUITexId)33,
                      (VIUITexId)34, tw, (VIAlphaBlendMode)3, k, 3, 1);
    }

    int r = 1;
    if (m_open == 1.0f)
        return r;
    if (m_open != 0.0f)
        r = 0;
    return r;
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

// Phase 61.  Word-wrapped text in a block of rows: a NEWLINE ends a line,
// and a line that would come within 8 pixels of the right edge breaks after
// its last space, or failing that at the last character that changed the
// width.  Returns the number of lines drawn.  f == 0xFFFFFFFF only MEASURES
// (CountLines); otherwise bit 2 draws a frame, 2 ellipsizes the last row
// when text remains, 8 / 0x10 centre / right-align, 0x80 masks with '*',
// and the top byte is a count of lines to skip.
#define TEXTBOX_COPY(n)                                                     \
    if (f != 0xFFFFFFFF && (f & 0x80))                                      \
        CopyAsteriks(buf, 512, n);                                          \
    else                                                                    \
        VIString::strncpy16(buf, 512, line, n);                             \
    buf[n] = 0;

#define TEXTBOX_DRAW()                                                      \
    if (count >= skip)                                                      \
    {                                                                       \
        if (f != 0xFFFFFFFF)                                                \
        {                                                                   \
            if (f & 8)                                                      \
                DrawTextInternal(buf,                                       \
                    pos.x + (size.x >> 1) - (TextWidth(buf) >> 1),          \
                    RowCenter(row + drawn).y, c, f);                        \
            else if (f & 0x10)                                              \
                DrawTextInternal(buf, pos.x + size.x - TextWidth(buf),      \
                    RowCenter(row + drawn).y, c, f);                        \
            else                                                            \
                DrawTextInternal(buf, pos.x, RowCenter(row + drawn).y,      \
                    c, f);                                                  \
        }                                                                   \
        drawn++;                                                            \
    }

int VIUIWindow::TextBox(const unsigned short *s, int x, int row, int w,
                        int rows, const VIColor32F &c, unsigned int f)
{
    int lh = m_ui->m_font.GetHeight(m_ui->m_raster, m_ui->m_dict) - 4;
    VIVect2I pos(TopLeft().x + x, RowCenter(row).y - 4);
    VIVect2I size(w, rows * lh);
    unsigned short buf[256];
    int n;

    if (f != 0xFFFFFFFF && (f & 4))
        DrawFrame(x, row, w, rows, f);

    const unsigned short *line = s;
    int lastw = 0;
    int skip = (f == 0xFFFFFFFF) ? 0 : f >> 24;
    int i = 0;
    int space = 0;
    int fit = 0;
    int drawn = 0;
    int count = 0;

    while (s[i])
    {
        if (s[i] == '\n')
        {
            n = &s[i] - line;
            TEXTBOX_COPY(n)
            if (f != 0xFFFFFFFF && (f & 2) && drawn + 1 == rows &&
                VIString::strlen16(&s[i + 1]))
                AppendEllipsis(buf, size.x);
            TEXTBOX_DRAW()
            count++;
            i++;
            line = &s[i];
            space = 0;
            fit = 0;
            if (drawn >= rows)
                break;
        }
        else
        {
            n = &s[i] - line;
            TEXTBOX_COPY(n)
            int pw = lastw;
            lastw = TextWidth(buf);
            if (lastw < size.x - 8)
            {
                if (pw != lastw)
                    fit = i;
                if (s[i] == ' ')
                    space = i;
                i++;
            }
            else
            {
                if (space)
                {
                    n = &s[space + 1] - line;
                    TEXTBOX_COPY(n)
                    i = space + 1;
                    line = &s[i];
                }
                else
                {
                    n = &s[fit] - line;
                    TEXTBOX_COPY(n)
                    i = fit;
                    line = &s[fit];
                }
                if (f != 0xFFFFFFFF && (f & 2) && drawn + 1 == rows &&
                    VIString::strlen16(line))
                    AppendEllipsis(buf, size.x);
                TEXTBOX_DRAW()
                count++;
                if (drawn >= rows)
                    break;
                space = 0;
                fit = 0;
            }
        }
    }

    n = &s[i] - line;
    if (n)
    {
        TEXTBOX_COPY(n)
        TEXTBOX_DRAW()
    }
    return drawn;
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

// Phase 61.  The inset frame the menus draw a row block in: a 10% wash of
// texture 61 behind the standard frame (101..106).  The eight frame-part
// assignments are in a FOUND order (a hill-climb over all of them took the
// function from 21 wrong words to 0); the arrow's width+gap is its own local.  Flag 0x40 drops the four
// extra pixels at the bottom; flag 0x20 adds the pulsing arrows either side.
void VIUIWindow::DrawFrame(int x, int row, int w, int rows, unsigned int flags)
{
    int h = m_ui->m_font.GetHeight(m_ui->m_raster, m_ui->m_dict) - 4;
    VIVect2I pos(TopLeft().x + x, RowCenter(row).y - 4);
    VIVect2I size(w, rows * h);
    VIVect2I at(pos.x - 4, pos.y - 3);
    VIVect2I sz(size.x + 8, (flags & 0x40) ? size.y : size.y + 4);
    VIUIFrameParam fp;

    memset(&fp, 0, sizeof(fp));
    GetUITexture((VIUITexId)101);
    GetUITexture((VIUITexId)106);
    fp.m_bottom.m_id = (VIUITexId)102;
    fp.m_bl.m_id = (VIUITexId)105;
    fp.m_br.m_id = (VIUITexId)106;
    fp.m_left.m_id = (VIUITexId)104;
    fp.m_right.m_id = (VIUITexId)104;
    fp.m_top.m_id = (VIUITexId)102;
    fp.m_tl.m_id = (VIUITexId)101;
    fp.m_tr.m_id = (VIUITexId)103;
    m_ui->StretchBlitEx((VIUITexId)61, at, sz, (VIAlphaBlendMode)3,
                        m_open * 0.1f);
    m_ui->BlitFrame(at, sz, fp, (VIAlphaBlendMode)0, m_open, 1);

    const VIUITexEntry *e = GetUITexture((VIUITexId)128);
    if (flags & 0x20)
    {
        int aw = e->m_uv1.x + 3;
        m_ui->BlitEx((VIUITexId)128,
                     VIVect2I(at.x - aw,
                              at.y + (sz.y >> 1) - (e->m_uv1.y >> 1)),
                     (VIAlphaBlendMode)3, TickPulse(), 0);
        m_ui->BlitEx((VIUITexId)128,
                     VIVect2I(at.x + sz.x + 3,
                              at.y + (sz.y >> 1) - (e->m_uv1.y >> 1)),
                     (VIAlphaBlendMode)3, TickPulse(), 1);
    }
}

// Phase 61.  The frame with a vertical scroll bar down its right edge, the
// thumb at t (0..1); out of range draws no bar.
void VIUIWindow::DrawFrameVScroll(int x, int row, int w, int rows, float t)
{
    int h = m_ui->m_font.GetHeight(m_ui->m_raster, m_ui->m_dict) - 4;
    VIVect2I pos(TopLeft().x + x, RowCenter(row).y - 4);
    VIVect2I size(w, rows * h);
    VIVect2I at(pos.x - 4, pos.y - 3);
    VIVect2I sz(size.x + 8, size.y + 4);

    DrawFrame(x, row, w, rows, 0);
    if (t >= 0.0f && t <= 1.0f)
    {
        GetUITexture((VIUITexId)84);
        VIVect2I p(at.x + sz.x - 4, at.y);
        DrawVScroll(p, sz.y, t, 0);
    }
}

// Phase 61.  A horizontal slider: the bar (164/165) with the knob (166) at t.
void VIUIWindow::DrawHScroll(int x, int row, int w, float t, unsigned int flags)
{
    VIVect2I pos(TopLeft().x + x, RowCenter(row).y - 4);
    VIVect2I size(w, 16);

    m_ui->BlitBar("", VIColor32F(), pos, (VIUITexId)165, (VIUITexId)164,
                  size.x, (VIAlphaBlendMode)3, m_open, 0, 1);

    const VIUITexEntry *e = GetUITexture((VIUITexId)166);
    if (flags & 0x20)
    {
        const VIUITexEntry *a = GetUITexture((VIUITexId)128);
        int aw = a->m_uv1.x + 3;
        m_ui->BlitEx((VIUITexId)128, VIVect2I(pos.x - aw, pos.y - 3),
                     (VIAlphaBlendMode)3, TickPulse(), 0);
        m_ui->BlitEx((VIUITexId)128, VIVect2I(pos.x + size.x + 3, pos.y - 3),
                     (VIAlphaBlendMode)3, TickPulse(), 1);
    }
    pos.x = pos.x + (int)(t * (float)(size.x - e->m_uv1.x));
    m_ui->BlitEx((VIUITexId)e->m_x00, pos, (VIAlphaBlendMode)3, m_open, 0);
}

// Phase 61.  The inset frame with a bar filled to t, in a fixed blue.
void VIUIWindow::DrawProgress(int x, int row, int w, float t)
{
    int h = m_ui->m_font.GetHeight(m_ui->m_raster, m_ui->m_dict);
    VIVect2I pos(TopLeft().x + x, RowCenter(row).y - 4);
    VIVect2I size(w, h - 8);
    VIVect2I at(pos.x - 4, pos.y - 3);
    VIVect2I sz;
    sz.y = h - 4;
    sz.x = w + 8;
    VIUIFrameParam fp;

    memset(&fp, 0, sizeof(fp));
    GetUITexture((VIUITexId)101);
    GetUITexture((VIUITexId)106);
    fp.m_bottom.m_id = (VIUITexId)102;
    fp.m_bl.m_id = (VIUITexId)105;
    fp.m_br.m_id = (VIUITexId)106;
    fp.m_left.m_id = (VIUITexId)104;
    fp.m_right.m_id = (VIUITexId)104;
    fp.m_top.m_id = (VIUITexId)102;
    fp.m_tl.m_id = (VIUITexId)101;
    fp.m_tr.m_id = (VIUITexId)103;
    m_ui->StretchBlitEx((VIUITexId)61, at, sz, (VIAlphaBlendMode)3,
                        m_open * 0.1f);
    m_ui->BlitFrame(at, sz, fp, (VIAlphaBlendMode)0, m_open, 1);

    size.x = (int)((float)size.x * t);

    VIVect2I r[2];

    r[0] = pos;
    r[1].x = pos.x + size.x;
    r[1].y = pos.y + size.y;

    VIColor32F c(0.0f, 0.0f, 0.8f, 1.0f);

    m_ui->FillRectangle(r, &c);
}

// Phase 61.  A solid bar in the caller's colour, six pixels short of a row.
void VIUIWindow::DrawColorBar(int x, int row, int w, const VIColor32F &c)
{
    int h = m_ui->m_font.GetHeight(m_ui->m_raster, m_ui->m_dict);
    VIVect2I pos(TopLeft().x + x, RowCenter(row).y - 4);
    VIVect2I size(w, h - 6);
    VIVect2I r[2];

    r[0] = pos;
    r[1].x = pos.x + size.x;
    r[1].y = pos.y + size.y;
    m_ui->FillRectangle(r, &c);
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

// Phase 61.  The width of a string as drawn: `|x` escapes are stripped, and
// the button glyphs they stand for add their own widths ('||' is a literal
// bar).  Strings of 1024 characters or more measure 0.
int VIUIWindow::TextWidth(const unsigned short *s)
{
    unsigned short buf[1024];

    if (VIString::strlen16(s) >= sizeof(buf) / sizeof(buf[0]))
        return 0;

    const unsigned short *p = s;
    unsigned short *d = buf;
    int extra = 0;

    while (*p)
    {
        if (*p == '|')
        {
            if (p[1] == 0)
                break;
            switch (p[1])
            {
            case 'c': case 's': case 't': case 'x':
                extra += 18;
                break;
            case '<': case '>': case 'e': case 'q': case 'r': case 'w':
                extra += 16;
                break;
            case 'i': case 'o': case 'u': case 'y': case '{': case '}':
                extra += 12;
                break;
            case '|':
                *d++ = '|';
                break;
            case 'A': case 'a':
                extra += 27;
                break;
            case '&': case '(': case '*': case '7': case '8': case '9':
                extra += 32;
                break;
            case 'V': case 'Z': case 'v': case 'z':
                extra += 17;
                break;
            }
            p++;
        }
        else
            *d++ = *p;
        p++;
    }
    *d = 0;
    return m_ui->m_font.GetStringWidth(m_ui->m_raster, m_ui->m_dict, buf) + extra;
}

// Phase 61.  Text with `|x` escapes: the plain runs are drawn through
// DrawCenteredStringEx a piece at a time, and each escape is a pad-button
// glyph from the atlas -- which glyph depends on the controller type at
// +0x178 (0, or 1/2) -- or an alpha change ('-' to 0.3, '+' back to the
// caller's), or a literal bar ('||').  'q'/'e' and their kin pulse in
// opposite phase.  A carriage return draws as a space.
void VIUIWindow::DrawTextInternal(const unsigned short *s, int x, int y,
                                  const VIColor32F &c, unsigned int f)
{
    unsigned short buf[1024];
    VIColor32F col = c;
    unsigned short *d = buf;
    float pulse = TickPulse();
    float alpha = 1.0f;

    while (*s)
    {
        if (*s == '|')
        {
            if (s[1] == 0)
                break;
            *d = 0;
            if (d - buf)
            {
                m_ui->DrawCenteredStringEx(buf, x, y,
                    VIColor32F(col.r, col.g, col.b, col.a * alpha), 1, f & 1);
                x += m_ui->m_font.GetStringWidth(m_ui->m_raster, m_ui->m_dict, buf);
            }
            d = buf;
            switch (s[1])
            {
            case 't':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)65, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)69, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 18;
                break;
            case 's':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)67, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)71, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 18;
                break;
            case 'c':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)68, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)72, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 18;
                break;
            case 'x':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)66, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)70, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 18;
                break;
            case '>':
                m_ui->BlitEx((VIUITexId)89, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 16;
                break;
            case 'q':
                m_ui->BlitEx((VIUITexId)89, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, pulse * alpha, 0);
                x += 16;
                break;
            case 'e':
                m_ui->BlitEx((VIUITexId)89, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, (1.0f - pulse) * alpha, 0);
                x += 16;
                break;
            case '<':
                m_ui->BlitEx((VIUITexId)88, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 16;
                break;
            case 'w':
                m_ui->BlitEx((VIUITexId)88, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, pulse * alpha, 0);
                x += 16;
                break;
            case 'r':
                m_ui->BlitEx((VIUITexId)88, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, (1.0f - pulse) * alpha, 0);
                x += 16;
                break;
            case '}':
                m_ui->BlitEx((VIUITexId)128, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, m_open * alpha, 1);
                x += 12;
                break;
            case 'y':
                m_ui->BlitEx((VIUITexId)128, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, pulse * alpha, 1);
                x += 12;
                break;
            case 'i':
                m_ui->BlitEx((VIUITexId)128, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, (1.0f - pulse) * alpha, 1);
                x += 12;
                break;
            case '{':
                m_ui->BlitEx((VIUITexId)128, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 12;
                break;
            case 'u':
                m_ui->BlitEx((VIUITexId)128, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, pulse * alpha, 0);
                x += 12;
                break;
            case 'o':
                m_ui->BlitEx((VIUITexId)128, VIVect2I(x, y - 6),
                             (VIAlphaBlendMode)3, (1.0f - pulse) * alpha, 0);
                x += 12;
                break;
            case '-':
                alpha = 0.3f;
                break;
            case '+':
                alpha = c.a;
                break;
            case '|':
                *d++ = '|';
                break;
            case 'a':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)190, VIVect2I(x, y - 4),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)196, VIVect2I(x, y - 4),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 27;
                break;
            case 'A':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)189, VIVect2I(x, y - 4),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)195, VIVect2I(x, y - 4),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 27;
                break;
            case '7':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)185, VIVect2I(x, y - 6),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)191, VIVect2I(x, y - 7),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 32;
                break;
            case '&':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)186, VIVect2I(x, y - 6),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)192, VIVect2I(x, y - 7),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 32;
                break;
            case '8':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)187, VIVect2I(x, y - 6),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)193, VIVect2I(x, y - 7),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 32;
                break;
            case '*':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)188, VIVect2I(x, y - 6),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)194, VIVect2I(x, y - 7),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 32;
                break;
            case '9':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)214, VIVect2I(x, y - 6),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)218, VIVect2I(x, y - 7),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 32;
                break;
            case '(':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)215, VIVect2I(x, y - 6),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)219, VIVect2I(x, y - 7),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 32;
                break;
            case 'z':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)212, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)216, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 17;
                break;
            case 'Z':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)212, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 2);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)216, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 2);
                x += 17;
                break;
            case 'v':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)213, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)217, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 0);
                x += 17;
                break;
            case 'V':
                if (m_x178 == 0)
                    m_ui->BlitEx((VIUITexId)213, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 1);
                else if (m_x178 == 1 || m_x178 == 2)
                    m_ui->BlitEx((VIUITexId)217, VIVect2I(x, y - 5),
                                 (VIAlphaBlendMode)3, m_open * alpha, 1);
                x += 17;
                break;
            }
            s++;
        }
        else
        {
            if (*s == '\r')
                *d = ' ';
            else
                *d = *s;
            d++;
        }
        s++;
    }

    *d = 0;
    if (d - buf)
        m_ui->DrawCenteredStringEx(buf, x, y,
            VIColor32F(col.r, col.g, col.b, col.a * alpha), 1, f & 1);
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
