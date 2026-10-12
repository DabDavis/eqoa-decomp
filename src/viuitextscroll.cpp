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
