#line 1 "C:/code-frontiers-gm/game/VIWndList.cpp"
// VIWndList (name the RTTI's) -- CLIENT 0x00C685F8 .. 0x00C6A9D0 (+ its inline members to
// 0x00C6AA30).  The scrolling list a dozen windows `new`, and the base of VIWndMenu,
// VIWndMiniMenu and VIWndTradeList: up to six rows of VIRec24_C6A8B8 (an icon, a second
// icon, arrows, a label, a number) in a studded frame with a scroll bar.  Method names ours.
#include "game.h"

extern "C" int sprintf(char *, const char *, ...);

// 0x00C685F8.
VIWndList::VIWndList()
{
    m_x60 = 0;
    m_x64 = 1;
    m_sel = 0;
    m_top = 0;
    m_count = 0;
    SetPos(VIVect2I(0, 0));
    m_selRow = -1;
    m_style = 0;
    m_alpha = 1.0f;
    m_x80 = 1;
    m_x7C = 1.0f;
    m_x84 = 1;
    m_time = 0;
}

// 0x00C686A8.
VIWndList::~VIWndList()
{
}

// 0x00C68710.  One row at pos: its icon (through the client's icon cache when there is
// one), a second icon, the arrows, an "X", a label (or two words), a number, and the
// selection marker when `lit`.
void VIWndList::DrawItem(VIRec24_C6A8B8 *item, const VIVect2I &pos, int lit, float alpha)
{
    VIAlphaBlendMode mode = (VIAlphaBlendMode)0;
    VIVect2I mark;
    if (alpha != 1.0f)
        mode = (VIAlphaBlendMode)3;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIVect2I zero(0, 0);
    VIVect2I size(114, 44);
    VIVect2I iconSize(22, 22);
    VIVect2I iconAt(46, 13);
    if (item->m_flags & 1)
    {
        VIClientPart06D50 *icons = &app->m_p06D50;
        if (icons)
            DrawIcon(ui, icons, item->m_icon, pos, mode, alpha);
        else
            ui->Blit(item->m_icon, pos, size, zero, mode, alpha, 0);
    }
    if (item->m_flags & 0x10)
        ui->Blit(item->m_icon2, VIVect2I(pos.x + iconAt.x, pos.y + iconAt.y), iconSize, zero,
                 (VIAlphaBlendMode)3, alpha, 0);
    if (item->m_flags & 8)
        DrawArrows(item->m_icon2, pos);
    if (item->m_flags & 0x20)
    {
        int h = ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
        VIVect2I p = pos + VIVect2I(3, 44 - h);
        ui->DrawStringEx("X", p.x, p.y, VIColor32F(0.0f, 0.0f, 0.0f, alpha), 0);
    }
    if (item->m_flags & 2)
    {
        VIVect2I p = pos + VIVect2I(57, 22);
        ui->Blit((VIUITexId)1, pos);
        switch (item->m_word)
        {
        case 118:
            p.y -= 11;
            ui->DrawCenteredStringEx((unsigned short)600, p.x, p.y, VIColor32F(1.0f, 1.0f, 1.0f, alpha), 0, 1);
            p.y += 19;
            ui->DrawCenteredStringEx((unsigned short)301, p.x, p.y, VIColor32F(1.0f, 1.0f, 1.0f, alpha), 0, 1);
            break;
        case 119:
            p.y -= 11;
            ui->DrawCenteredStringEx((unsigned short)601, p.x, p.y, VIColor32F(1.0f, 1.0f, 1.0f, alpha), 0, 1);
            p.y += 19;
            ui->DrawCenteredStringEx((unsigned short)301, p.x, p.y, VIColor32F(1.0f, 1.0f, 1.0f, alpha), 0, 1);
            break;
        default:
            ui->DrawCenteredStringEx((unsigned short)item->m_word, p.x, p.y,
                                     VIColor32F(1.0f, 1.0f, 1.0f, alpha), 0, 1);
            break;
        }
    }
    if (item->m_flags & 4)
    {
        char buf[32];
        sprintf(buf, "%i", item->m_number);
        int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf);
        VIVect2I p = pos;
        p -= VIVect2I(w, 0);
        p.x += size.x;
        ui->DrawStringEx(buf, p.x, p.y, VIColor32F(1.0f, 1.0f, 1.0f, alpha), 1);
    }
    if (lit)
    {
        const VIUITexEntry *t = GetUITexture((VIUITexId)86);
        if (m_style == 0)
        {
            mark = pos + VIVect2I(-2, 22 - t->m_uv1.y / 2);
            ui->BlitEx((VIUITexId)t->m_x00, mark, (VIAlphaBlendMode)3, m_x7C < alpha ? m_x7C : alpha, 1);
        }
        else if (m_style == 1)
        {
            mark = pos + VIVect2I(118, 22 - t->m_uv1.y / 2);
            ui->BlitEx((VIUITexId)t->m_x00, mark, (VIAlphaBlendMode)3, m_x7C < alpha ? m_x7C : alpha, 0);
        }
    }
}

// 0x00C68C70.  The two-part arrow bar of a row (keys 138/140 pick the textures).
void VIWndList::DrawArrows(int key, VIVect2I pos)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    const VIUITexEntry *end;
    const VIUITexEntry *mid;
    VIVect2I p = pos + VIVect2I(2, 2);
    if (key == 138)
    {
        end = GetUITexture((VIUITexId)137);
        mid = GetUITexture((VIUITexId)138);
    }
    else if (key == 140)
    {
        end = GetUITexture((VIUITexId)139);
        mid = GetUITexture((VIUITexId)140);
    }
    ui->BlitEx((VIUITexId)end->m_x00, p, (VIAlphaBlendMode)3, 0.3f, 0);
    p.x += end->m_uv1.x;
    ui->StretchBlitEx((VIUITexId)mid->m_x00, p, VIVect2I(110 - end->m_uv1.x * 2, end->m_uv1.y),
                      (VIAlphaBlendMode)3, 0.3f);
    p.x += 110 - end->m_uv1.x * 2;
    ui->BlitEx((VIUITexId)end->m_x00, p, (VIAlphaBlendMode)3, 0.3f, 1);
}

// 0x00C68E10.  The frame (style 0), up to six rows from m_top (the selected one lit, and
// a ghost of Select's row beside it), empty slots to fill six, the scroll bar, then the
// fade and the up/down auto-repeat.
void VIWndList::OnDrawSelf()
{
    VIVect2I pos;
    VIVect2I step;
    int count;
    VIVect2I &at = m_pos;
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    float dt = Elapsed();
    count = Count();
    Fit(6);
    ui->Begin2D();
    pos = at;
    step.Set(114, 44);
    if (m_style == 0)
    {
        const VIUITexEntry *stud = GetUITexture((VIUITexId)36);
        const VIUITexEntry *fill = GetUITexture((VIUITexId)82);
        VIUIFrameParam fp;
        VIUIFrameParam studs;
        memset(&fp, 0, sizeof(fp));
        memset(&studs, 0, sizeof(studs));
        fp.m_bottom.m_id = (VIUITexId)37;
        fp.m_tl.m_id = (VIUITexId)36;
        fp.m_left.m_id = (VIUITexId)42;
        fp.m_bl.m_id = (VIUITexId)153;
        fp.m_tr.m_id = (VIUITexId)36;
        fp.m_br.m_flags = 1;
        fp.m_right.m_flags = 1;
        fp.m_right.m_id = (VIUITexId)42;
        fp.m_bottom.m_flags = 2;
        fp.m_top.m_id = (VIUITexId)37;
        fp.m_br.m_id = (VIUITexId)153;
        studs.m_top.m_id = (VIUITexId)38;
        studs.m_right.m_id = (VIUITexId)43;
        fp.m_tr.m_flags = 1;
        studs.m_right.m_flags = 1;
        studs.m_bottom.m_id = (VIUITexId)38;
        studs.m_bottom.m_flags = 2;
        studs.m_left.m_id = (VIUITexId)43;
        ui->BlitFillStuddedFrame(pos, VIVect2I(stud->m_uv1.x * 2 + fill->m_uv1.x + 114,
                                               stud->m_uv1.y * 2 + 264),
                                 fp, studs, (VIUITexId)51, 1, 3, (VIAlphaBlendMode)3, m_alpha, 1);
        pos += VIVect2I(stud->m_uv1.x, stud->m_uv1.y);
    }
    int i = 0;
    for (int row = m_top; row < count; row = i + m_top)
    {
        int lit = 0;
        if (GetX64() && row == m_sel && m_x60 == 0)
            lit = 1;
        DrawItem(&m_items[row], pos, lit, m_alpha);
        if (row == m_sel && m_x60)
            DrawItem(&m_cur, pos + VIVect2I(160, 0), 0, m_alpha < 0.5f ? m_alpha : 0.5f);
        i++;
        pos.y += step.y;
        if (i >= 6)
            break;
    }
    for (; i < 6; i++)
    {
        ui->BlitEx((VIUITexId)1, pos, (VIAlphaBlendMode)3, m_alpha, 0);
        pos.y += step.y;
    }
    int h;
    if (m_style == 0)
    {
        h = 268;
        pos = at + VIVect2I(126, 9);
    }
    else if (m_style == 1)
    {
        h = 270;
        pos = at + VIVect2I(143, -2);
    }
    m_window.Init(VIWnd::mUI, VIVect2I(0, 0), at + VIVect2I(57, 132), 114, 10, 1);
    if (m_alpha >= 1.0f)
        m_window.Open(true);
    float t = count < 7 ? 0.0f : (float)m_top / (float)(count - 6);
    m_window.DrawVScroll(pos, h, t, 0);
    ui->End2D();
    Update(dt);
    if (m_flash.Pulse(138))
        Scroll(-1);
    if (m_flash.Pulse(139))
        Scroll(1);
}

// 0x00C69368.  Enter tells the parent (256, the selection); up and down start the
// auto-repeat the draw runs.
void VIWndList::OnKeyDown(int key)
{
    GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
        GetParent()->OnMessage(this, 256, m_sel, 0);
        break;
    case 19:
    case 138:
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        m_flash.Press(139);
        break;
    default:
        VIWnd::OnKeyDown(key);
        break;
    }
}

// 0x00C69460.
void VIWndList::OnKeyUp(int key)
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
    default:
        VIWnd::OnKeyUp(key);
        break;
    }
}

// 0x00C694E8.
void VIWndList::SetX64(int v)
{
    m_x64 = v;
    m_flash.Reset();
}

// 0x00C69508.
int VIWndList::Init(int n)
{
    return m_items.Init(n);
}

// 0x00C69528.  Moves the selection by d, wrapping at both ends, and keeps it in the
// six rows shown.
void VIWndList::Scroll(int d)
{
    int count = Count();
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel + d >= count && count > 0)
    {
        app->PlaySound(11);
        m_sel = 0;
        m_top = 0;
    }
    else if (m_sel + d < 0)
    {
        app->PlaySound(11);
        m_sel = VIMAX(count - 1, 0);
        m_top = VIMAX(count - 6, 0);
    }
    else
    {
        m_sel += d;
        app->PlaySound(11);
        if (m_sel == 0 || m_sel == count - 1)
            m_flash.Reset();
    }
    if (m_top + 5 < m_sel)
        m_top = m_sel - 5;
    if (m_sel < m_top)
        m_top = m_sel;
    m_x80 = 1;
    m_x7C = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C69660.
void VIWndList::SetX3C(int a, int b, int c, int d)
{
    m_x3C = a;
    m_x40 = b;
    m_x4C = c;
    m_x50 = d;
}

// 0x00C69678.
void VIWndList::Select(int i)
{
    m_selRow = i;
    if (i == -1)
    {
        m_cur.m_flags = 0;
        m_cur.m_icon = 0;
        m_cur.m_icon2 = 0;
        m_cur.m_word = 0;
    }
    else
        m_cur = m_items[i];
}

// 0x00C696E0.
void VIWndList::SetItem(int i, int flags, int icon, int icon2, int word, int number,
                        int x0C)
{
    m_items[i].m_flags = flags;
    m_items[i].m_icon = icon;
    m_items[i].m_icon2 = icon2;
    m_items[i].m_word = word;
    m_items[i].m_number = number;
    m_items[i].m_x0C = x0C;
}

// 0x00C69738.
int VIWndList::Count()
{
    int n = 0;
    for (int i = 0; i < m_items.m_size; i++)
        if (m_items.m_data[i].m_flags)
            n++;
    return n;
}

// 0x00C69778.  After the list changed: keeps the selection and the top row inside the
// rows there are, and tells the parent (296) when the selection moved.
int VIWndList::Fit(int rows)
{
    int old = m_sel;
    int count = Count();
    if (count - rows < m_top && rows < count)
        m_top--;
    else if (count == rows)
        m_top = 0;
    if (m_top < 0)
        m_top = 0;
    if (m_sel >= count)
        m_sel--;
    if (m_sel < 0)
        m_sel = 0;
    if (m_sel != old || m_items.m_data[0].m_flags == 0 || count != m_count)
    {
        m_count = count;
        GetParent()->OnMessage(this, 296, old, m_sel);
    }
    return 0;
}

// 0x00C69870.  An inventory icon out of the client's icon cache.
int VIWndList::DrawIcon(VIUI *ui, VIClientPart06D50 *icons, int icon, const VIVect2I &pos,
                        int mode, float alpha)
{
    VIVect2I size(114, 44);
    VIVect2I zero(0, 0);
    int type;
    int tex;
    if (icons->Get(icon, type, tex) < 0)
        return -1;
    switch (type)
    {
    case 0:
        return ui->Blit(icon, pos, size, zero, (VIAlphaBlendMode)mode, alpha, 0);
    case 1:
        return ui->Blit((VIUITexId)1, pos, size);
    case 2:
        ui->Blit((VIUITexId)1, pos, size);
        return ui->Blit((VIUITexId)10, pos + VIVect2I(42, 0));
    }
    return -1;
}

// 0x00C69A38.
void VIWndList::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_alpha = 0.25f;
    m_time = app->m_p07568;
    m_window.Open(false);
}

// 0x00C69A80.  The fade in, and the selection marker's blink.
void VIWndList::Update(float dt)
{
    m_alpha += dt * 5.0f;
    m_alpha = m_alpha < 1.0f ? m_alpha : 1.0f;
    if (m_x84)
    {
        if (m_x80 == -1)
        {
            m_x7C -= dt + dt;
            m_x7C = m_x7C < 0.0f ? 0.0f : m_x7C;
            if (m_x7C == 0.0f)
                m_x80 = 1;
        }
        else
        {
            m_x7C += dt * 3.0f;
            m_x7C = m_x7C < 1.0f ? m_x7C : 1.0f;
            if (m_x7C == 1.0f)
                m_x80 = -1;
        }
    }
    else
        m_x7C = 1.0f;
}

// 0x00C69B68.
float VIWndList::Elapsed()
{
    long now = ((VIClient *)GetAppPtr())->m_p07568;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    return dt;
}
