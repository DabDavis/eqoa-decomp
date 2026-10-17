#line 1 "C:/code-frontiers-gm/game/VIWndHUDMenu.cpp"
// viwndhudmenu.cpp -- CLIENT 0x00C5DE08 .. 0x00C5FA00: VIWndHUDMenu, VIWndGame's quick
// menus (lane cl-wh).
#include "game.h"

// The slot icons of the stacked menu (0x00CE9960), as VIWndItemMenu's.
static int sSlotIcons[] = { 5, 5, 3, 2 };

// 0x00C5DE08.
VIWndHUDMenu::VIWndHUDMenu()
{
    m_lit = -1;
    m_alpha = 0.3f;
    m_sel = 0;
    m_time = 0;
    m_menu = 0;
    m_title[0] = 0;
}

// 0x00C5DE78.
VIWndHUDMenu::~VIWndHUDMenu()
{
    Free();
}

// 0x00C5DED0.
int VIWndHUDMenu::Init()
{
    m_lit = -1;
    m_alpha = 0.3f;
    m_sel = 0;
    m_time = 0;
    m_index = 0;
    m_title[0] = 0;
    return 0;
}

// 0x00C5DF00.
int VIWndHUDMenu::Free()
{
    m_lit = -1;
    m_alpha = 0.3f;
    m_sel = 0;
    m_time = 0;
    m_menu = 0;
    m_title[0] = 0;
    return 0;
}

// 0x00C5DF30.
void VIWndHUDMenu::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_time < app->m_p07568)
        m_lit = -1;
    Clamp();
    if (m_lit > 0)
        m_alpha = VIMIN(m_alpha + 0.32f, 1.0f);
    else
        m_alpha = VIMAX(0.3f, m_alpha - 0.105f);
    m_text.Init(UI(), m_pos + VIVect2I(81, 10), 158, 0, (VIScrollStyle)1);
    m_text.Open(true);
    m_text.SetOn(m_alpha > 0.35f ? 1 : 0);
    if (m_index == 0 || m_index == 1)
        DrawList();
    else if (m_index == 2)
        DrawStack();
}

// 0x00C5E0B0.  The title bar, then a column of slot icons beside a frame.
void VIWndHUDMenu::DrawStack()
{
    unsigned short name[64];
    VIVect2I at;
    VIVect2I pos;
    VIVect2I icon;
    VIVect2I list;
    VIVect2I bar;
    VIVect2I size;
    VIVect2I gap;
    VIVect2I zero;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    ui->Begin2D();
    at = m_pos;
    pos = m_pos;
    size.Set(22, 22);
    gap.Set(6, 8);
    zero.Set(0, 0);
    bar = at + VIVect2I(75, 9);
    name[0] = 0;
    if (m_menu->m_entries[m_sel].m_id >= 0 && m_menu->m_entries[m_sel].m_msg)
        app->GetObjectName(m_menu->m_entries[m_sel].m_msg, m_menu->m_entries[m_sel].m_id, name, 64);
    ui->BlitBar("", VIColor32F(1.0f, 1.0f, 1.0f, m_alpha), at, (VIUITexId)27, (VIUITexId)28, 162,
                (VIAlphaBlendMode)3, m_alpha, 0, 1);
    m_text.TextScroll(name, VIColor32F(1.0f, 1.0f, 1.0f, m_alpha), 9);
    pos.x += 146;
    icon = pos + VIVect2I(17, -5);
    const VIUITexEntry *top = GetUITexture((VIUITexId)158);
    const VIUITexEntry *mid = GetUITexture((VIUITexId)121);
    const VIUITexEntry *bot = GetUITexture((VIUITexId)120);
    if (!top || !mid || !bot)
    {
        ui->End2D();
        return;
    }
    ui->BlitEx((VIUITexId)top->m_x00, icon, (VIAlphaBlendMode)3, m_alpha, 0);
    icon.y += top->m_uv1.y;
    ui->BlitEx((VIUITexId)mid->m_x00, icon, VIVect2I(34, mid->m_uv1.y), (VIAlphaBlendMode)3, m_alpha, 0);
    icon.y += mid->m_uv1.y;
    ui->BlitEx((VIUITexId)mid->m_x00, icon, VIVect2I(34, mid->m_uv1.y), (VIAlphaBlendMode)3, m_alpha, 0);
    icon.y += mid->m_uv1.y;
    ui->BlitEx((VIUITexId)mid->m_x00, icon, VIVect2I(34, mid->m_uv1.y), (VIAlphaBlendMode)3, m_alpha, 0);
    icon.y += mid->m_uv1.y;
    ui->BlitEx((VIUITexId)mid->m_x00, icon, VIVect2I(34, mid->m_uv1.y - 4), (VIAlphaBlendMode)3, m_alpha, 0);
    icon.y += mid->m_uv1.y - 4;
    ui->BlitEx((VIUITexId)158, icon, VIVect2I(top->m_uv1.x, 3), (VIAlphaBlendMode)3, m_alpha, 2);
    list = pos + VIVect2I(17, -5) + gap;
    int n = m_sel;
    for (int i = 0; i < m_menu->m_count; i++, n++)
    {
        if (n >= m_menu->m_count)
            n = 0;
        float alpha = m_alpha;
        ui->BlitEx((VIUITexId)sSlotIcons[n], list, (VIAlphaBlendMode)3, alpha, 0);
        list.y += 32;
    }
    if (VIString::strlen16(m_title))
    {
        int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, m_title) - 170;
        ui->DrawCenteredStringEx(m_title, m_pos.x - w, m_pos.y + 28,
                                 VIColor32F(1.0f, 1.0f, 1.0f, m_alpha), 0, 1);
    }
    ui->End2D();
}

// 0x00C5E638.  The title bar, then the entries down a frame that grows past five.
void VIWndHUDMenu::DrawList()
{
    unsigned short name[64];
    VIVect2I at;
    VIVect2I pos;
    VIVect2I icon;
    VIVect2I bar;
    int h = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    ui->Begin2D();
    at = m_pos;
    bar = m_pos + VIVect2I(75, 9);
    name[0] = 0;
    if (m_menu->m_entries[m_sel].m_icon2 != -1 && m_menu->m_entries[m_sel].m_id >= 0
        && m_menu->m_entries[m_sel].m_msg)
        app->GetObjectName(m_menu->m_entries[m_sel].m_msg, m_menu->m_entries[m_sel].m_id, name, 64);
    ui->BlitBar("", VIColor32F(1.0f, 1.0f, 1.0f, m_alpha), at, (VIUITexId)27, (VIUITexId)28, 162,
                (VIAlphaBlendMode)3, m_alpha, 0, 1);
    m_text.TextScroll(name, VIColor32F(1.0f, 1.0f, 1.0f, m_alpha), 9);
    pos = m_pos + VIVect2I(159, -12);
    ui->BlitEx((VIUITexId)30, pos, (VIAlphaBlendMode)3, m_alpha, 0);
    if (m_menu->m_count > 5)
        h = (m_menu->m_count - 5) * 31;
    pos.y += 159 + h;
    ui->BlitEx((VIUITexId)31, pos, (VIAlphaBlendMode)3, m_alpha, 2);
    pos += VIVect2I(4, -151 - h);
    int n = m_sel;
    for (int i = 0; i < m_menu->m_count; i++, n++)
    {
        if (n >= m_menu->m_count)
            n = 0;
        icon = pos + VIVect2I(7, 2);
        ui->BlitEx((VIUITexId)29, pos, (VIAlphaBlendMode)3, m_alpha, 0);
        if (m_menu->m_entries[n].m_icon2 != -1)
            ui->Blit(m_menu->m_entries[n].m_icon2, icon, VIVect2I(22, 22), VIVect2I(0, 0),
                     (VIAlphaBlendMode)3, m_alpha, 0);
        if (m_menu->m_entries[n].m_x08 == 61)
            ui->BlitEx((VIUITexId)61, icon - VIVect2I(2, 2), VIVect2I(26, 26), (VIAlphaBlendMode)3,
                       m_alpha * 0.5f, 0);
        pos.y += 31;
    }
    if (VIString::strlen16(m_title))
    {
        int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, m_title) - 170;
        ui->DrawCenteredStringEx(m_title, m_pos.x - w, m_pos.y + 28,
                                 VIColor32F(1.0f, 1.0f, 1.0f, m_alpha), 0, 1);
    }
    ui->End2D();
}

// 0x00C5EAE8.  Any key the menu takes keeps it lit for 20 seconds.
void VIWndHUDMenu::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 'E':
    case 'e':
    case 135:
        if (m_menu->m_entries[m_sel].m_msg && m_menu->m_entries[m_sel].m_id >= 0)
            m_parent->OnMessage(this, 261, m_menu->m_entries[m_sel].m_msg, m_menu->m_entries[m_sel].m_id);
        m_lit = 1;
        m_time = app->m_p07568 + 20000;
        break;
    case 'W':
    case 'w':
    case 138:
        Move(-1);
        m_lit = 1;
        m_time = app->m_p07568 + 20000;
        break;
    case 'S':
    case 's':
    case 139:
        Move(1);
        m_lit = 1;
        m_time = app->m_p07568 + 20000;
        break;
    case 'A':
    case 'a':
    case 'D':
    case 'd':
    case 136:
    case 137:
        m_lit = 1;
        m_time = app->m_p07568 + 20000;
        VIWnd::OnKeyDown(key);
        break;
    default:
        VIWnd::OnKeyDown(key);
        break;
    }
}

// 0x00C5EC08.  The focus passing to this menu from one of its two siblings relights it.
void VIWndHUDMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if ((VIWnd *)p2 == this && p1 != p2)
        {
            VIWndGame *game = (VIWndGame *)m_parent;
            if (game->IsHud0((VIWndHUDMenu *)p1) || game->IsHud1((VIWndHUDMenu *)p1)
                || game->IsHud2((VIWndHUDMenu *)p1))
            {
                m_lit = 1;
                m_alpha = 0.3f;
                m_time = app->m_p07568 + 20000;
            }
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C5ECE0.
void VIWndHUDMenu::Move(int d)
{
    ((VIClient *)GetAppPtr())->PlaySound(11);
    if (m_menu)
    {
        int n = m_sel + d;
        if (n < 0)
            m_sel = m_menu->m_count - 1;
        else if (n >= m_menu->m_count)
            m_sel = 0;
        else
            m_sel = n;
    }
}

// 0x00C5ED58.
void VIWndHUDMenu::SetMenu(VIMenuDef *menu)
{
    m_menu = menu;
}

// 0x00C5ED60.
void VIWndHUDMenu::Clamp()
{
    if (m_sel >= m_menu->m_count)
        m_sel = m_menu->m_count - 1;
}

// 0x00C5ED88.
void VIWndHUDMenu::SetTitle(const unsigned short *s)
{
    VIString::strcpy16(m_title, sizeof(m_title), s);
}
