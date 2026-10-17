#line 1 "C:/code-frontiers-gm/game/VIWndInfluence.cpp"
// viwndinfluence.cpp -- CLIENT 0x00C62310 .. 0x00C63200: VIWndInfluence, the active-effect
// icons (lane cl-wh).
#include "game.h"

// 0x00C62310.
VIWndInfluence::VIWndInfluence()
{
    m_pos = VIVect2I(0, 0);
}

// 0x00C62360.
VIWndInfluence::~VIWndInfluence()
{
}

// 0x00C62388.
int VIWndInfluence::Init()
{
    return 0;
}

// 0x00C62390.  A left cap, one 34-high cell per effect with its icon, a right cap.
void VIWndInfluence::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIVect2I pos = m_pos;
    VIVect2I off(3, 6);
    VIUI &ui = app->m_ui;
    int count = app->m_effectCount;
    ui.Begin2D();
    if (count > 0)
    {
        const VIUITexEntry *e = GetUITexture((VIUITexId)159);
        ui.BlitEx((VIUITexId)e->m_x00, pos, (VIAlphaBlendMode)0, 1.0f, 0);
        pos.x += e->m_uv1.x;
    }
    for (int i = 0; i < count; i++)
    {
        const VIUITexEntry *e = GetUITexture((VIUITexId)157);
        int icon = app->m_effectIcon[i];
        int w;
        if (i == count - 1)
            w = e->m_uv1.x - 3;
        else
            w = e->m_uv1.x;
        ui.BlitEx((VIUITexId)e->m_x00, pos, VIVect2I(w, 34), (VIAlphaBlendMode)0, 1.0f, 0);
        ui.Blit(icon, pos + off, (VIAlphaBlendMode)0, 1.0f, 0);
        pos.x += w;
    }
    if (count > 0)
        ui.BlitEx((VIUITexId)160, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    ui.End2D();
}

// 0x00C625B0.
void VIWndInfluence::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
}

// 0x00C625D0.
void VIWndInfluence::OnKeyUp(int key)
{
}
