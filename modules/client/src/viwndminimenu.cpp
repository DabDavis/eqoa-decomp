#line 1 "C:/code-frontiers-gm/game/VIWndMiniMenu.cpp"
// VIWndMiniMenu.cpp -- CLIENT 0x00C7BB48 .. 0x00C7D8E8.  The popup menu: a VIWndList over a
// VIMenuDef, drawn as a frame that grows out of m_pos (kind 0) or with a title bar and a
// button legend (kind 1).
#include "game.h"

// VIVect2I has no operator== in vi.h; the original's is an inline returning the && as a value.
static inline int operator==(const VIVect2I &a, const VIVect2I &b)
{
    return a.x == b.x && a.y == b.y;
}

// 0x00C7BB48.
VIWndMiniMenu::VIWndMiniMenu()
{
    m_menu = 0;
    memset(m_title, 0, sizeof(m_title));
    m_kind = 0;
    m_size.Set(0, 0);
    m_alpha = 1.0f;
}

// 0x00C7BBC0.
void VIWndMiniMenu::OnDrawSelf()
{
    if (m_kind == 0)
        DrawPlain();
    else if (m_kind == 1)
        DrawTitled();
    if (m_flash.Pulse(138))
        Scroll(-1);
    if (m_flash.Pulse(139))
        Scroll(1);
}

// 0x00C7BC50.  The rows' labels set the full size; the frame grows to it at three sizes a
// second, and the rows are drawn once it is there.
void VIWndMiniMenu::DrawPlain()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    if (Init(m_menu->m_count))
        return;
    int i;
    for (i = 0; i < m_menu->m_count; i++)
        SetItem(i, -1, -1, -1, m_menu->m_entries[i].m_word, -1, -1);
    float dt = Elapsed();
    ui.Begin2D();
    VIVect2I size;
    VIVect2I stretch;
    VIVect2I pos = m_pos;
    for (i = 0; i < m_items.m_size; i++)
        if (size.x < ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                       (const unsigned short *)ui.m_words.GetString(
                           (unsigned short)m_items[i].m_word, ui.m_language)))
            size.x = ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                       (const unsigned short *)ui.m_words.GetString(
                           (unsigned short)m_items[i].m_word, ui.m_language));
    size.x += 75;
    size.y = m_items.m_size * 19 + 25;
    m_alpha = 1.0f;
    float grow = dt * 3.0f;
    m_size.x = VIMIN(m_size.x + (int)(grow * size.x), size.x);
    m_size.y = VIMIN(m_size.y + (int)(grow * size.y), size.y);
    const VIUITexEntry *top = GetUITexture((VIUITexId)101);
    const VIUITexEntry *corner = GetUITexture((VIUITexId)106);
    if (pos.y + size.y > 320)
        pos.y = 320 - size.y;
    VIUIFrameParam fp;
    memset(&fp, 0, sizeof(fp));
    fp.m_tr.m_id = (VIUITexId)103;
    fp.m_br.m_id = (VIUITexId)106;
    fp.m_bl.m_id = (VIUITexId)105;
    fp.m_bottom.m_id = (VIUITexId)102;
    fp.m_top.m_id = (VIUITexId)102;
    fp.m_tl.m_id = (VIUITexId)101;
    fp.m_left.m_id = (VIUITexId)104;
    fp.m_right.m_id = (VIUITexId)104;
    int sx = m_size.x - corner->m_uv1.x;
    if (sx < 0)
        sx = 0;
    stretch.Set(sx, m_size.y - top->m_uv1.y - corner->m_uv1.y + 1);
    ui.StretchBlitEx((VIUITexId)61, pos + VIVect2I(0, top->m_uv1.y - 1), stretch,
                     (VIAlphaBlendMode)3, 0.75f);
    ui.BlitFrame(pos, m_size, fp, (VIAlphaBlendMode)0, 1.0f, 0);
    pos += VIVect2I(35, 20);
    if (m_size == size)
    {
        for (i = 0; i < m_items.m_size; i++)
        {
            DrawItem(&m_items.m_data[i], pos, GetX64() && i == m_sel);
            pos.y += 19;
        }
    }
    ui.m_font.SetTextColor(white);
    ui.End2D();
    Update(dt);
}

// 0x00C7C118.  The title, the rows, and the legend: the select and back buttons with
// their words (212, 161).
void VIWndMiniMenu::DrawTitled()
{
    VIClient *app = (VIClient *)GetAppPtr();
    const VIUITexEntry *button = GetUITexture((VIUITexId)70);
    VIUI &ui = app->m_ui;
    if (Init(m_menu->m_count))
        return;
    int h = 19;
    float dt = Elapsed();
    ui.Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, m_alpha);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, m_alpha);
    VIVect2I p;
    VIVect2I pos;
    VIVect2I a;
    VIVect2I b;
    pos = m_pos;
    VIVect2I stretch;
    VIVect2I size;
    size.x = ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                 (const unsigned short *)ui.m_words.GetString(161, ui.m_language))
           + ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                 (const unsigned short *)ui.m_words.GetString(212, ui.m_language))
           + 105 + button->m_uv1.x;
    size.x = VIMAX(size.x, ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, m_title) + 75);
    int i;
    for (i = 0; i < m_menu->m_count; i++)
    {
        SetItem(i, -1, -1, -1, m_menu->m_entries[i].m_word, -1, -1);
        size.x = VIMAX(size.x, ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                       (const unsigned short *)ui.m_words.GetString(
                           m_menu->m_entries[i].m_word, ui.m_language)) + 75);
    }
    size.y = (m_menu->m_count + 5) * h;
    m_size = size;
    const VIUITexEntry *top = GetUITexture((VIUITexId)101);
    const VIUITexEntry *corner = GetUITexture((VIUITexId)106);
    VIUIFrameParam fp;
    memset(&fp, 0, sizeof(fp));
    fp.m_bl.m_id = (VIUITexId)105;
    fp.m_br.m_id = (VIUITexId)106;
    fp.m_top.m_id = (VIUITexId)102;
    fp.m_left.m_id = (VIUITexId)104;
    fp.m_right.m_id = (VIUITexId)104;
    fp.m_tl.m_id = (VIUITexId)101;
    fp.m_tr.m_id = (VIUITexId)103;
    fp.m_bottom.m_id = (VIUITexId)102;
    int sx = m_size.x - corner->m_uv1.x;
    if (sx < 0)
        sx = 0;
    stretch.Set(sx, m_size.y - top->m_uv1.y - corner->m_uv1.y + 1);
    ui.StretchBlitEx((VIUITexId)61, pos + VIVect2I(0, top->m_uv1.y - 1), stretch,
                     (VIAlphaBlendMode)3, VIMIN(0.75f, m_alpha));
    ui.BlitFrame(pos, m_size, fp, (VIAlphaBlendMode)3, m_alpha, 1);
    ui.m_font.SetTextColor(white);
    if (m_size == size)
    {
        p = m_pos + VIVect2I(20, 10);
        ui.DrawStringEx(m_title, p.x, p.y, white, 1);
        p.x += h * 2;
        p.y += h * 2;
        for (i = 0; i < m_items.m_size; i++)
        {
            DrawItem(&m_items[i], p, GetX64() && i == m_sel);
            p.y += h;
        }
        ui.m_font.SetTextColor(white);
        pos = m_pos + VIVect2I(20, m_size.y - 32);
        p = pos + VIVect2I(22, button->m_uv1.y / 2 - 3);
        ui.BlitEx((VIUITexId)70, pos, (VIAlphaBlendMode)3, m_alpha, 0);
        ui.DrawCenteredStringEx((unsigned short)212, p.x, p.y, white, 1, 1);
        pos.x += ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                     (const unsigned short *)ui.m_words.GetString(212, ui.m_language))
               + 30 + button->m_uv1.x;
        p.x += ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict,
                   (const unsigned short *)ui.m_words.GetString(212, ui.m_language))
             + 30 + button->m_uv1.x;
        ui.BlitEx((VIUITexId)69, pos, (VIAlphaBlendMode)3, m_alpha, 0);
        ui.DrawCenteredStringEx((unsigned short)161, p.x, p.y, white, 1, 1);
    }
    Update(dt);
    ui.m_font.SetTextColor(VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
    ui.End2D();
}

// 0x00C7C818.  A row's label centred on pos, yellow with the pointer beside it when lit.
void VIWndMiniMenu::DrawItem(VIRec24_C6A8B8 *item, const VIVect2I &pos, int lit)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    VIVect2I p;
    VIColor32F c;
    VIColor32F yellow(1.0f, 1.0f, 0.0f, m_alpha);
    VIColor32F white(1.0f, 1.0f, 1.0f, m_alpha);
    if (lit)
    {
        const VIUITexEntry *t = GetUITexture((VIUITexId)89);
        p = pos - VIVect2I(25, t->m_uv1.y / 2);
        ui.BlitEx((VIUITexId)89, p, (VIAlphaBlendMode)3, VIMIN(m_alpha, m_x7C), 0);
        c = yellow;
    }
    else
        c = white;
    ui.DrawCenteredStringEx((const unsigned short *)ui.m_words.GetString(item->m_word, ui.m_language),
                            pos.x, pos.y, c, 1, 1);
}

// 0x00C7C9A8.
void VIWndMiniMenu::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
        if (m_menu->m_entries[m_sel].m_msg)
            GetParent()->OnMessage(this, 261, m_menu->m_entries[m_sel].m_msg,
                                   m_menu->m_entries[m_sel].m_id);
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 19:
    case 20:
    case 138:
    case 139:
        VIWndList::OnKeyDown(key);
        break;
    }
}

// 0x00C7CAD0.
void VIWndMiniMenu::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 20:
    case 138:
    case 139:
        VIWndList::OnKeyUp(key);
        break;
    }
}

// 0x00C7CB10.  295: the focus moving (p1 the old window, p2 the new); opening it from its
// parent starts the frame over.
void VIWndMiniMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:
        break;
    case 295:
        if (p2 == (int)this && p1 == (int)GetParent())
        {
            m_size.x = 0;
            m_size.y = 0;
            m_alpha = 0.3f;
            m_time = app->m_p07568;
            m_flash.Reset();
        }
        if (p1 == (int)this)
            m_flash.Reset();
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C7CBE0.
void VIWndMiniMenu::SetMenu(VIMenuDef *menu)
{
    m_menu = menu;
}

// 0x00C7CBE8.
void VIWndMiniMenu::SetTitle(const unsigned short *s)
{
    VIString::strcpy16(m_title, 128, s);
}

// 0x00C7CC10.
void VIWndMiniMenu::SetTitle(const char *s)
{
    VIString::str8to16(m_title, 128, s);
}

// 0x00C7CC38.
void VIWndMiniMenu::SetTitle(unsigned short word)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    VIString::strcpy16(m_title, 128, (const unsigned short *)ui->m_words.GetString(word, ui->m_language));
}
