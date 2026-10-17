#line 1 "C:/code-frontiers-gm/game/VIWndGoto.cpp"
// viwndgoto.cpp -- CLIENT 0x00C58BD8 .. 0x00C59D98: VIWndGoto, the debug teleporter.
// The window is new'd by VIWndGame.
#include "game.h"

// 0x00C58BD8.
VIWndGoto::VIWndGoto()
{
    m_pos = VIVect2I(0, 0);
    m_world = -1;
    m_y = 0;
    m_x = 0;
    m_x34 = 0;
}

// 0x00C58C40.
VIWndGoto::~VIWndGoto()
{
    Free();
}

// 0x00C58C88.
int VIWndGoto::Init(VIScene *scene)
{
    m_x34 = (int)scene;
    m_world = -1;
    return 0;
}

// 0x00C58CA0.
int VIWndGoto::Free()
{
    m_x34 = 0;
    return 0;
}

// 0x00C58CB0.
void VIWndGoto::OnDrawSelf()
{
    unsigned short buf[128];
    VIVect2I pos(100, 100);
    VIVect2I size(330, 70);
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    VIFont &font = ui.m_font;

    ui.Begin2D();
    font.SetTextColor(white);
    ui.StretchBlitEx((VIUITexId)61, pos, size, (VIAlphaBlendMode)3, 0.75f);
    pos += VIVect2I(10, 6);
    if (m_world < 0 || m_world >= 6)
        return;
    VIString::snprintf16(buf, sizeof(buf), "%S",
        ui.m_words.GetString(gWorldInfo[m_world].m_nameWord, ui.m_language));
    ui.DrawStringEx(buf, pos.x, pos.y, white, 1);
    pos.y += font.GetHeight(ui.m_raster, ui.m_dict) - 4;
    VIString::snprintf16(buf, sizeof(buf), "%S %d, %S %d",
        ui.m_words.GetString(982, ui.m_language), m_x,
        ui.m_words.GetString(983, ui.m_language), m_y);
    ui.DrawStringEx(buf, pos.x, pos.y, white, 1);
    pos.y += font.GetHeight(ui.m_raster, ui.m_dict) - 4;
    ui.DrawStringEx(ZoneName(m_world, m_y, m_x), pos.x, pos.y, white, 1);
    ui.End2D();
}

// 0x00C58F08.
void VIWndGoto::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
        app->SetP50308(m_world, m_x, m_y);
        m_parent->OnMessage(this, 260, 0, 0);
        break;
    case 19:
    case 138:
        ChangeColumn(1);
        break;
    case 20:
    case 139:
        ChangeColumn(-1);
        break;
    case 17:
    case 136:
        ChangeRow(-1);
        break;
    case 18:
    case 137:
        ChangeRow(1);
        break;
    case 22:
    case 133:
        ChangeWorld(-1);
        break;
    case 21:
    case 135:
        ChangeWorld(1);
        break;
    case 27:
    case 132:
        m_parent->OnMessage(this, 260, 0, 0);
        break;
    default:
        VIWnd::OnKeyDown(key);
    }
}

// 0x00C59038.
void VIWndGoto::OnKeyUp(int key)
{
}

// 0x00C59040.
void VIWndGoto::ChangeRow(int d)
{
    m_y += d;
    VIWorldInfo *w = &gWorldInfo[m_world];
    int y = m_y;
    y = y < w->m_p14 ? w->m_p14 : y;
    y = y > w->m_p18 ? w->m_p18 : y;
    m_y = y;
}

// 0x00C59080.
void VIWndGoto::ChangeColumn(int d)
{
    m_x += d;
    VIWorldInfo *w = &gWorldInfo[m_world];
    int x = m_x;
    x = x < w->m_p0C ? w->m_p0C : x;
    x = x > w->m_p10 ? w->m_p10 : x;
    m_x = x;
}

// 0x00C590C0.
void VIWndGoto::ChangeWorld(int d)
{
    int w = m_world;
    m_x = 0;
    w += d;
    m_y = 0;
    w = w < 0 ? 0 : w;
    w = w > 5 ? 5 : w;
    m_world = w;
}

// 0x00C590F0.
void VIWndGoto::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295 && p2 == (int)this)
    {
        m_world = app->m_worldId;
        m_y = 0;
        m_x = 0;
    }
}
