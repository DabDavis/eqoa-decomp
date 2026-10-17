#line 1 "C:/code-frontiers-gm/game/VIWndMainMenu.cpp"
// VIWndMainMenu (name the RTTI's) -- CLIENT 0x00C6D2D8 .. 0x00C6FA18.  The in-game menu: up
// to seven entries of a VIMenuDef, the name/class/level bar, the stats column with the
// location and the active effects, and two key hints; an entry with a submenu opens another
// VIWndMainMenu over this one.  Method names ours.
#include "game.h"

// 0x00CEB978: a class's icon in the stats column (alchemist's needs the expansion).
static struct { int m_class; int m_icon; } sClassIcons[15] =
{
    { 0, 23 }, { 1, 19 }, { 2, 18 }, { 3, 21 }, { 4, 16 }, { 5, 11 }, { 6, 20 }, { 7, 13 },
    { 8, 22 }, { 9, 12 }, { 10, 15 }, { 11, 17 }, { 12, 14 }, { 13, 24 }, { 14, 251 },
};

// 0x00C6D2D8.
VIWndMainMenu::VIWndMainMenu()
{
    m_top = 0;
    m_sel = 0;
    m_x34 = 1;
    m_pos.Set(0, 0);
    m_blink = 1.0f;
    m_blinkDir = 1;
    m_menu = 0;
    m_sub = 0;
    m_sub2 = 0;
    m_x44 = 0;
    m_x48 = 0;
    m_time = 0;
}

// 0x00C6D378.
VIWndMainMenu::~VIWndMainMenu()
{
    Free();
}

// 0x00C6D3E8.
int VIWndMainMenu::Init(int sub)
{
    m_sub2 = sub;
    m_blink = 1.0f;
    m_top = 0;
    m_sel = 0;
    m_x34 = 1;
    m_blinkDir = 1;
    m_pos.Set(0, 0);
    m_x44 = 0;
    m_x48 = 0;
    m_time = 0;
    return 0;
}

// 0x00C6D428.
int VIWndMainMenu::Free()
{
    m_blink = 1.0f;
    m_top = 0;
    m_sel = 0;
    m_x34 = 1;
    m_blinkDir = 1;
    m_pos.Set(0, 0);
    m_x48 = 0;
    if (m_sub)
    {
        RemoveChild(m_sub);
        delete m_sub;
        m_sub = 0;
    }
    return 0;
}

// 0x00C6D4B0.  A submenu closing (260 back here, 261 forwarded), the focus arriving
// (295: p2) or leaving (p1).
void VIWndMainMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:
        if (w == m_sub)
        {
            SetScrollPos(m_sub->GetScrollPos());
            app->SetActiveWindow(this);
            m_sub->m_visible = 0;
            m_sub->SetX44(0);
            m_x44 = 1;
        }
        break;
    case 261:
        if (w == m_sub)
        {
            app->SetActiveWindow(this);
            m_sub->m_visible = 0;
        }
        GetParent()->OnMessage(this, 261, p1, p2);
        break;
    case 295:
        if (p2 == (int)this)
        {
            m_x48 = 1;
            m_blink = 1.0f;
            m_blinkDir = 1;
            m_time = app->m_p07568;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 100, 10, 0);
            m_window.Open(true);
            m_flash.Reset();
        }
        if (p1 == (int)this)
        {
            m_x48 = 0;
            m_flash.Reset();
        }
        break;
    default:
        GetParent()->OnMessage(this, msg, p1, p2);
        break;
    }
}

// 0x00C6D680.
void VIWndMainMenu::OnKeyDown(int key)
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
        app->PlaySound(13);
        Select();
        break;
    case 19:
    case 138:
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        m_flash.Press(139);
        break;
    case 144:
        GetParent()->OnKeyDown(144);
        break;
    }
}

// 0x00C6D7D8.
void VIWndMainMenu::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        m_flash.Release(138);
        break;
    case 20:
    case 139:
        m_flash.Release(139);
        break;
    }
}

// 0x00C6D850.  Enter: an entry's message goes to the parent; a submenu opens.
void VIWndMainMenu::Select()
{
    GetAppPtr();
    if (m_menu->m_entries[m_sel].m_msg)
    {
        if (m_sub)
        {
            RemoveChild(m_sub);
            delete m_sub;
            m_sub = 0;
        }
        GetParent()->OnMessage(this, 261, m_menu->m_entries[m_sel].m_msg,
                               m_menu->m_entries[m_sel].m_id);
    }
    else if (m_menu->m_entries[m_sel].m_sub)
    {
        if (!m_sub)
        {
            m_sub = new VIWndMainMenu;
            m_sub->SetAppPtr(GetAppPtr());
            AddChild(m_sub);
            m_sub->Init(1);
            VIVect2I p = m_pos;
            m_sub->SetPos(p);
        }
        m_x44 = 0;
        m_sub->SetScrollPos(GetScrollPos());
        m_sub->SetX44(1);
        m_sub->SetMenu(m_menu->m_entries[m_sel].m_sub);
        m_sub->Activate(1);
    }
}

// 0x00C6D9D0.
void VIWndMainMenu::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    if (!m_x48)
        return;
    long now = app->m_p07568;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    ui->Begin2D();
    VIColor32F lit(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIVect2I pos;
    pos.Set(18, 56);
    VIVect2I p;
    ui->m_font.SetTextColor(black);
    int i = 0;
    if (m_menu->m_count > 0)
    {
        for (int row = m_top; row < m_menu->m_count; row = i + m_top)
        {
            int sel = 0;
            if (m_x34 && row == m_sel)
                sel = 1;
            ui->BlitBar((unsigned short)m_menu->m_entries[row].m_word, sel ? lit : black, pos,
                        (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 140,
                        (VIAlphaBlendMode)0, 1.0f, 0, 0);
            if (sel)
            {
                const VIUITexEntry *a = GetUITexture((VIUITexId)85);
                const VIUITexEntry *b = GetUITexture((VIUITexId)150);
                p = pos + VIVect2I(0, b->m_uv1.y / 2 - a->m_uv1.y / 2 - 3);
                ui->BlitEx((VIUITexId)a->m_x00, p, (VIAlphaBlendMode)3, m_blink, 0);
            }
            i++;
            pos.y += 27;
            if (i >= m_menu->m_count || i >= 7)
                break;
        }
    }
    ui->m_font.SetTextColor(black);
    float t = m_menu->m_count < 8 ? 0.0f : (float)m_top / (float)(m_menu->m_count - 7);
    m_window.DrawVScroll(VIVect2I(175, 46), 270, t, 0);
    if (m_menu->m_x04 > 0)
    {
        if (m_x44)
        {
            int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict,
                        (const unsigned short *)ui->m_words.GetString((unsigned short)m_menu->m_x04,
                                                                      ui->m_language)) + 64;
            ui->BlitBar((unsigned short)m_menu->m_x04, white, VIVect2I((640 - w) / 2, 8),
                        (VIUITexId)33, (VIUITexId)34, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
        }
    }
    else if (m_sub2 == 0)
    {
        unsigned short buf[128];
        char name[24];
        buf[0] = 0;
        name[0] = 0;
        const VIClassDef *c = GetClass(app->GetClass());
        app->GetPlayerName(name, sizeof(name));
        VIString::snprintf16(buf, sizeof(buf), "%s: %S %d %S", name,
                             ui->m_words.GetString(503, ui->m_language), app->GetLevel(),
                             ui->m_words.GetString(c->m_nameStrID, ui->m_language));
        int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf) + 64;
        ui->BlitBar(buf, white, VIVect2I((640 - w) / 2, 8), (VIUITexId)33, (VIUITexId)34, w,
                    (VIAlphaBlendMode)0, 1.0f, 0, 1);
    }
    DrawStats();
    ui->m_font.SetTextColor(black);
    VIVect2I at(20, 260);
    ui->BlitEx((VIUITexId)66, at, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->m_font.DrawString(ui->m_raster, ui->m_dict,
                          (const unsigned short *)ui->m_words.GetString(212, ui->m_language),
                          at.x + 22, at.y - 5);
    at.y += 19;
    ui->BlitEx((VIUITexId)65, at, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->m_font.DrawString(ui->m_raster, ui->m_dict,
                          (const unsigned short *)ui->m_words.GetString(161, ui->m_language),
                          at.x + 22, at.y - 5);
    ui->End2D();
    if (m_blinkDir == -1)
    {
        m_blink -= dt + dt;
        m_blink = m_blink < 0.0f ? 0.0f : m_blink;
        if (m_blink == 0.0f)
            m_blinkDir = 1;
    }
    else
    {
        m_blink += dt * 3.0f;
        m_blink = m_blink < 1.0f ? m_blink : 1.0f;
        if (m_blink == 1.0f)
            m_blinkDir = -1;
    }
    if (m_flash.Pulse(138))
        Scroll(-1);
    if (m_flash.Pulse(139))
        Scroll(1);
}

// 0x00C6E098.
void VIWndMainMenu::Scroll(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int i = m_sel + d;
    if (i >= m_menu->m_count)
    {
        app->PlaySound(11);
        m_sel = 0;
        m_top = 0;
    }
    else if (i < 0)
    {
        app->PlaySound(11);
        m_sel = VIMAX(m_menu->m_count - 1, 0);
        m_top = VIMAX(m_menu->m_count - 7, 0);
    }
    else if (m_top + 6 < i)
    {
        app->PlaySound(11);
        m_sel += d;
        m_top = m_sel - 6;
    }
    else if (i < m_top)
    {
        app->PlaySound(11);
        m_top = m_sel + d;
        m_sel += d;
    }
    else
    {
        app->PlaySound(11);
        m_sel += d;
    }
    if ((m_sel == 0 && d < 0) || (m_sel == m_menu->m_count - 1 && d > 0))
        m_flash.Reset();
    m_blinkDir = 1;
    m_blink = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C6E210.  Takes the focus (a reset starts at the top and closes the submenu), or hands
// it down to the open submenu.
void VIWndMainMenu::Activate(int reset)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (reset)
    {
        m_sel = 0;
        m_top = 0;
        m_visible = 1;
        if (m_sub)
            m_sub->m_visible = 0;
        app->SetActiveWindow(this);
        m_x44 = 1;
    }
    else
    {
        m_visible = 1;
        if (m_sub)
            m_sub->Activate(0);
        else
        {
            app->SetActiveWindow(this);
            m_x44 = 1;
        }
    }
    UpdateLocation();
}

// 0x00C6E298.  The stats column.
void VIWndMainMenu::DrawStats()
{
    VIVect2I at;
    unsigned short buf[256];
    unsigned short num[32];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, 1.0f);
    int h = ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 4;
    ui->m_font.SetTextColor(white);
    at.Set(545, 48);
    VIVect2I icon(485, 48);
    int cls = app->GetClass();
    int i;
    for (i = 0; i < 15; i++)
    {
        if (sClassIcons[i].m_class != cls)
            continue;
        const VIUITexEntry *t = GetUITexture((VIUITexId)sClassIcons[i].m_icon);
        ui->Blit((VIUITexId)130, icon);
        icon.x += 57 - t->m_uv1.x / 2;
        if (cls == 14)
        {
            if (!(VIGetExpansionFlags() & 2))
                break;
            icon.y++;
        }
        ui->Blit((VIUITexId)sClassIcons[i].m_icon, icon);
    }
    VIFont &font = ui->m_font;
    at.y += 48;
    buf[0] = 0;
    ui->DrawCenteredStringEx((unsigned short)919, at.x, at.y, yellow, 0, 1);
    at.y += h;
    VIString::snprintf16(num, sizeof(num), "%d/%d", app->GetHP(), app->GetMaxHP());
    ui->DrawCenteredStringEx(num, at.x, at.y, white, 0, 1);
    at.y += h;
    ui->DrawCenteredStringEx((unsigned short)466, at.x, at.y, yellow, 0, 1);
    at.y += h;
    VIString::snprintf16(num, sizeof(num), "%d/%d", app->GetPower(), app->GetMaxPower());
    ui->DrawCenteredStringEx(num, at.x, at.y, white, 0, 1);
    at.y += h;
    ui->DrawCenteredStringEx((unsigned short)242, at.x, at.y, yellow, 0, 1);
    at.y += h;
    VIString::snprintf16(num, sizeof(num), "%d", app->GetPlayer()->m_stat[14]);
    ui->DrawCenteredStringEx(num, at.x, at.y, white, 0, 1);
    at.y += h;
    ui->DrawCenteredStringEx((unsigned short)18, at.x, at.y, yellow, 0, 1);
    at.y += h;
    for (i = 0; i < 2; i++)
    {
        if (m_lines[i][0])
        {
            ui->DrawCenteredStringEx(m_lines[i], at.x, at.y, white, 0, 1);
            at.y += h;
        }
    }
    ui->DrawCenteredStringEx((unsigned short)479, at.x, at.y, yellow, 0, 1);
    at.y += h;
    VIString::snprintf16(buf, sizeof(buf), "%d", app->m_player.m_p34);
    ui->DrawCenteredStringEx(buf, at.x, at.y, white, 0, 1);
    at.y += h;
    ui->DrawCenteredStringEx((unsigned short)15, at.x, at.y, yellow, 0, 1);
    at.y += h;
    buf[0] = 0;
    for (i = 0; i < app->m_effectCount; i++)
    {
        VIString::strcat16(buf, sizeof(buf), app->m_effectName[i]);
        if (app->m_effectCount >= 2 && i < app->m_effectCount - 1)
            VIString::strcat16(buf, sizeof(buf), "    ");
    }
    m_scroll.Init(UI(), at + VIVect2I(0, 2), 160, 0, (VIScrollStyle)0);
    m_scroll.Open(true);
    if (!VIString::strlen16(buf))
        m_scroll.Reset();
    m_scroll.TextScroll(buf, white, 9);
    font.SetTextColor(white);
}

// 0x00C6E868.  Word-wraps text into `rows` lines of `chars` (each line 32 wide characters),
// breaking at spaces or after a hyphen when a line grows past `width` pixels.
void VIWndMainMenu::Wrap(unsigned short *lines, int width, int chars, int rows,
                         const unsigned short *text)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    int src = 0;
    int line = 0;
    int len = 0;
    int brk = 0;
    int hyph = 0;
    int w = 0;
    unsigned short *start = lines;
    if (!text[0])
        return;
    while (text[src])
    {
        unsigned short *out = &lines[line * 32];
        if (width > 0 && chars - 1 > 0)
        {
            const unsigned short *s = &text[src];
            while (len < chars - 1 && line < rows)
            {
                if (!*s)
                    return;
                if (*s == ' ')
                {
                    brk = len;
                    hyph = 0;
                }
                else if (*s == '-')
                {
                    hyph = s[1] != 0;
                    brk = len + hyph;
                }
                if (*s == '\n')
                {
                    out += 32;
                    start += 32;
                    line++;
                    s++;
                    src++;
                    len = 0;
                    w = 0;
                    brk = 0;
                    hyph = 0;
                }
                if (*s)
                {
                    out[len++] = *s;
                    out[len] = 0;
                    s++;
                    src++;
                    w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, out);
                }
                if (w >= width)
                    break;
            }
        }
        if (!text[src])
            return;
        if (brk > 0)
        {
            start[brk] = 0;
            src = hyph ? src - (len - brk) : src + 1 - (len - brk);
        }
        line++;
        if (line >= rows)
            return;
        start += 32;
        len = 0;
        w = 0;
        brk = 0;
        hyph = 0;
    }
}

// 0x00C6EAD8.  Where the player is: the world grid cell's name and the ninth of it.
void VIWndMainMenu::UpdateLocation()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIVect3 pos;
    VIVect3 off;
    memset(m_lines, 0, sizeof(m_lines));
    pos = app->m_walkCam.m_committed.m_position;
    if (app->m_scene.GetWorldGridNumCols() == 1 && app->m_scene.GetWorldGridNumRows() == 1)
    {
        VIString::snprintf16(m_lines[0], sizeof(m_lines[0]), "***");
        return;
    }
    int col;
    int row;
    unsigned short buf[128];
    unsigned short name[64];
    unsigned short dir[16];
    app->m_scene.CalcWorldGridOffset(pos, col, row, off);
    buf[0] = 0;
    name[0] = 0;
    dir[0] = 0;
    VIUI *ui = UI();
    int zone = ZoneName(app->m_worldId, row - 1, col - 1);
    VIString::snprintf16(name, sizeof(name),
                         (const unsigned short *)ui->m_words.GetString(zone, ui->m_language));
    unsigned short dirs[3][3] = { { 1426, 1422, 1427 }, { 1425, 1430, 1424 }, { 1428, 1423, 1429 } };
    int x;
    if (off.x < -333.34f)
        x = 0;
    else if (off.x >= -333.34f && off.x <= 333.34f)
        x = 1;
    else
        x = 2;
    int z;
    if (off.z < -333.34f)
        z = 0;
    else if (off.z >= -333.34f && off.z <= 333.34f)
        z = 1;
    else
        z = 2;
    VIString::snprintf16(dir, sizeof(dir), "%S",
                         UI()->m_words.GetString(dirs[z][x], UI()->m_language));
    if (!VIString::strlen16(dir))
        VIString::snprintf16(buf, sizeof(buf), "%S", name);
    else
        VIString::snprintf16(buf, sizeof(buf), "%S (%S)", name, dir);
    Wrap(m_lines[0], 150, 32, 2, buf);
}
