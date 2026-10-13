// ---------------------------------------------------------------------------
// viuitextscroll.cpp -- SUPPORT 0x004A8330 .. 0x004A8878.  Phase 42.
//
// The TU closes with GetUITexture, the 252-entry atlas lookup.
// ---------------------------------------------------------------------------
#include "vi.h"

// The table itself is an unnamed file static in the original; only its stride
// (36) and its bound (252) are asserted here.
extern VIUITexEntry sTextures[252];

VIUITextScroll::VIUITextScroll()
{
    m_style = 1;
    m_p184 = 0;
    m_p188 = 1;
    m_p18C = 30;
    m_width = 0;
    m_on = 1;
}

VIUITextScroll::~VIUITextScroll()
{
}

int VIUITextScroll::Init(VIUI *ui, const VIVect2I &at, int w, int x,
                         VIScrollStyle st)
{
    m_style = st;
    m_width = w;
    VIVect2I z(0, 0);
    return VIUIWindow::Init(ui, z, at, w, 1, x);
}

int VIUITextScroll::Reset()
{
    m_p188 = 1;
    m_p18C = 30;
    m_p184 = 0;
    return 0;
}

// Phase 61.  Text wider than the box scrolls: style 0 runs it right to left
// and wraps, style 1 bounces it between the two edges with a 30-frame pause at
// each end.  m_p184 is the offset, m_p188 the direction (+1/0/-1) and m_p18C
// the pause count.  It is drawn in 64-character pieces inside a viewport cut
// to the window.  Text that fits is placed by flag: 8 centred, 0x10 right.
int VIUITextScroll::TextScroll(const unsigned short *s, const VIColor32F &c,
                               int f)
{
    int y = Center().y - 2;

    if (m_ui)
    {
    int tw = TextWidth(s);
    VIVect2I vp[2];

    m_ui->m_raster->GetViewport(vp[0].x, vp[0].y, vp[1].x, vp[1].y);

    if (m_width < tw)
    {
        switch (m_style)
        {
        case 0:
            if (m_on || m_p184 != m_width)
                m_p184 -= 2;
            if (m_p184 + tw < 0 || m_width < m_p184)
                m_p184 = m_width;
            break;

        case 1:
            if (m_width >= m_p184 + tw)
            {
                if (--m_p18C > 0)
                    m_p188 = 0;
                else
                {
                    m_p188 = 1;
                    m_p18C = 30;
                }
            }
            else if (m_p184 >= 0)
            {
                m_p18C--;
                if (m_on == 0)
                {
                    m_p188 = 0;
                    m_p18C = 30;
                }
                else if (m_p18C <= 0)
                {
                    m_p188 = -1;
                    m_p18C = 30;
                }
                else
                    m_p188 = 0;
            }
            if (m_p188 > 0)
                m_p184 += 2;
            else if (m_p188 < 0)
                m_p184 -= 2;
            {
                int o = VIMAX(m_width - tw, m_p184);
                m_p184 = VIMIN(o, 0);
            }
            break;
        }

        int x = 0;

        // The raster is fetched into a local BEFORE the four temporaries are
        // built: called through m_ui->m_raster directly, the load sinks below
        // the temporaries' stores (20 wrong words).
        VIRaster *r = m_ui->m_raster;
        r->Viewport(TopLeft().x, TopLeft().y, BottomRight().x, BottomRight().y);

        int n = VIString::strlen16(s) / 64 + 1;
        for (int i = 0; i < n; i++)
        {
            unsigned short buf[65];

            VIString::strncpy16(buf, 130, s + i * 64, 64);
            DrawTextInternal(buf, m_p184 + x, 23, c, f);
            x += TextWidth(buf);
        }

        m_ui->m_raster->Viewport(vp[0].x, vp[0].y, vp[1].x, vp[1].y);
        return 0;
    }

    Reset();
    if (f & 8)
        DrawTextInternal(s, Center().x - (TextWidth(s) >> 1), y, c, f);
    else if (f & 0x10)
        DrawTextInternal(s, TopLeft().x + m_width - TextWidth(s), y, c, f);
    else
        DrawTextInternal(s, TopLeft().x, y, c, f);
    return 0;
    }
    return -1;
}

int VIUITextScroll::TextScroll(const unsigned short *s, VIUICOLOR c, int n)
{
    return TextScroll(s, m_colors[c], n);
}

void VIUITextScroll::SetOn(int v)
{
    m_on = v;
}

const VIUITexEntry *GetUITexture(VIUITexId id)
{
    if ((unsigned int)id < 252)
        return &sTextures[id];
    return &sTextures[0];
}
