#line 1 "C:/code-frontiers-gm/game/VIWndTradeList.cpp"
// VIWndTradeList -- CLIENT 0x00C9D5A8 .. 0x00C9DDF0 (the TU runs to its __tf at 0x00C9E9C8,
// then its inline members).  One side of a trade: a VIWndList of m_rows rows in a plain
// frame under a centred title.
#include "game.h"

// 0x00C9D5A8.
VIWndTradeList::VIWndTradeList()
{
    SetX84(1);
    m_title = 0xFFFF;
    m_rows = 0;
    m_x7C = 1.0f;
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
}

// 0x00C9D620.
VIWndTradeList::~VIWndTradeList()
{
    Free();
}

// 0x00C9D668.
int VIWndTradeList::Init()
{
    SetX84(1);
    m_title = 0xFFFF;
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    return 0;
}

// 0x00C9D6A0.
int VIWndTradeList::Free()
{
    m_title = 0xFFFF;
    m_rows = 0;
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    return 0;
}

// 0x00C9D6D0.
void VIWndTradeList::OnDrawSelf()
{
    VIVect2I pos;
    VIVect2I size;
    VIVect2I step;
    VIVect2I unused;
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    int count = Count();
    step.Set(114, 44);
    float dt = Elapsed();
    Fit(m_rows);
    ui->Begin2D();
    pos = m_pos;
    const VIUITexEntry *stud = GetUITexture((VIUITexId)36);
    const VIUITexEntry *fill = GetUITexture((VIUITexId)82);
    VIUIFrameParam fp;
    VIUIFrameParam studs;
    memset(&fp, 0, sizeof(fp));
    memset(&studs, 0, sizeof(studs));
    fp.m_bottom.m_id = (VIUITexId)37;
    fp.m_left.m_id = (VIUITexId)42;
    fp.m_bl.m_id = (VIUITexId)153;
    fp.m_br.m_id = (VIUITexId)153;
    fp.m_tr.m_id = (VIUITexId)36;
    fp.m_br.m_flags = 1;
    fp.m_right.m_flags = 1;
    fp.m_right.m_id = (VIUITexId)42;
    fp.m_bottom.m_flags = 2;
    fp.m_top.m_id = (VIUITexId)37;
    fp.m_tr.m_flags = 1;
    fp.m_tl.m_id = (VIUITexId)36;
    studs.m_right.m_flags = 1;
    studs.m_bottom.m_id = (VIUITexId)38;
    studs.m_top.m_id = (VIUITexId)38;
    studs.m_bottom.m_flags = 2;
    studs.m_left.m_id = (VIUITexId)43;
    studs.m_right.m_id = (VIUITexId)43;
    ui->BlitFillFrame(pos, VIVect2I(stud->m_uv1.x * 2 + fill->m_uv1.x + 114,
                                    stud->m_uv1.y * 2 + m_rows * 44 + 30),
                      fp, (VIUITexId)51, (VIAlphaBlendMode)3, m_alpha, 1);
    pos += VIVect2I(stud->m_uv1.x, stud->m_uv1.y);
    if (count == 0 && GetX64())
    {
        const VIUITexEntry *e = GetUITexture((VIUITexId)86);
        VIVect2I p = pos + VIVect2I(-2, 44);
        ui->BlitEx((VIUITexId)e->m_x00, p, (VIAlphaBlendMode)3, m_x7C, 1);
    }
    VIVect2I &at = m_pos;
    m_color.a *= m_alpha;
    ui->DrawCenteredStringEx(m_title, pos.x + 57, pos.y + 14, m_color, 0, 1);
    pos.y += 30;
    int i = 0;
    while (i < m_rows)
    {
        int row = i + m_top;
        if (row >= count)
            break;
        int lit = 0;
        if (GetX64() && row == m_sel)
            lit = 1;
        DrawItem(&m_items[row], pos, lit, 1.0f);
        i++;
        pos.y += step.y;
    }
    pos = at + VIVect2I(126, 9);
    m_window.Init(VIWnd::mUI, VIVect2I(0, 0), at + VIVect2I(57, m_rows * 22), 114, 10, 1);
    m_window.Open(true);
    float t = m_rows < count ? (float)m_top / (float)(count - m_rows) : 0.0f;
    m_window.DrawVScroll(pos, m_rows * 44 + 34, t, 0);
    ui->End2D();
    Update(dt);
    if (m_flash.Pulse(138))
        Move(-1);
    if (m_flash.Pulse(139))
        Move(1);
}

// 0x00C9DBD8.
void VIWndTradeList::OnKeyDown(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        m_flash.Press(139);
        break;
    default:
        VIWndList::OnKeyDown(key);
        break;
    }
}

// 0x00C9DC60.
void VIWndTradeList::OnKeyUp(int key)
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
        VIWnd::OnKeyDown(key);
        break;
    }
}

// 0x00C9DCE8.
void VIWndTradeList::Move(int d)
{
    int count = Count();
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel + d >= count && count > 0)
    {
        app->PlaySound(9);
        m_sel = count - 1;
    }
    else if (m_sel + d < 0)
    {
        app->PlaySound(9);
        m_sel = 0;
    }
    else
    {
        m_sel += d;
        app->PlaySound(11);
    }
    if (m_top + m_rows - 1 < m_sel)
        m_top = m_sel - m_rows + 1;
    if (m_sel < m_top)
        m_top = m_sel;
    m_x80 = 1;
    m_x7C = 1.0f;
    m_time = app->m_p07568;
}
