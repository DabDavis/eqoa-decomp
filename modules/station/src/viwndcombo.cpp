// viwndcombo.cpp (name ours) -- STATION 0x003474D8 .. 0x00349630, the TU linked
// behind libxml: a page option's state (VIOptionItem, name ours) and the
// dropdown window that picks an option's value (VIWndCombo).
#include "station.h"

// ---- VIOptionItem ------------------------------------------------------------

// 0x003474D8.
VIOptionItem::VIOptionItem()
{
    m_type = 0;
    m_value = 0;
    m_text = 0;
    m_x5C = 0;
}

// 0x003474F8.
VIOptionItem::~VIOptionItem()
{
}

// 0x00347520.  A button.
int VIOptionItem::Init(const unsigned short *label, const VIVect2I &pos, int x4C, int x60)
{
    m_type = 1;
    VIString::strcpy16(m_label, sizeof(m_label), label);
    m_pos = pos;
    m_x60 = x60;
    m_x4C = x4C;
    m_x70 = 0;
    return 0;
}

// 0x003475A0.  A text field, empty.
int VIOptionItem::Init(const unsigned short *label, int x50, const VIVect2I &pos, int x4C,
                       int x60)
{
    unsigned short empty[1];
    m_type = 2;
    VIString::strcpy16(m_label, sizeof(m_label), label);
    m_pos = pos;
    m_x50 = x50;
    m_x4C = x4C;
    m_x60 = x60;
    m_x70 = 0;
    empty[0] = 0;
    SetText(empty);
    return 0;
}

// 0x00347640.  A toggle.
int VIOptionItem::Init(const unsigned short *label, int x50, const VIVect2I &pos)
{
    m_type = 3;
    VIString::strcpy16(m_label, sizeof(m_label), label);
    m_pos = pos;
    m_x50 = x50;
    m_x4C = 0;
    m_x70 = 0;
    m_x60 = 0;
    return 0;
}

// 0x003476B8.  A list of `count` choices.
int VIOptionItem::Init(const unsigned short *label, int x50, const VIVect2I &pos, int x4C,
                       int min, int count, int value)
{
    m_type = 4;
    VIString::strcpy16(m_label, sizeof(m_label), label);
    m_x60 = 0;
    m_pos = pos;
    m_x50 = x50;
    m_x4C = x4C;
    m_value = value;
    m_min = min;
    m_count = count;
    m_rows = VIMIN(count, 3);
    m_x70 = 0;
    m_sel = 0;
    m_x6C = 0;
    return 0;
}

// 0x00347780.  The numbers min .. max.
int VIOptionItem::InitRange(const unsigned short *label, int x50, const VIVect2I &pos, int x4C,
                            int min, int max, int value)
{
    m_type = 4;
    VIString::strcpy16(m_label, sizeof(m_label), label);
    m_x60 = 2;
    int count = max - min + 1;
    m_pos = pos;
    m_x50 = x50;
    m_x4C = x4C;
    m_value = value;
    m_min = min;
    m_count = count;
    m_rows = VIMIN(count, 3);
    m_x70 = 0;
    m_sel = 0;
    m_x6C = 0;
    return 0;
}

// 0x00347858.
void VIOptionItem::Free()
{
    if (m_text)
        delete [] m_text;
    m_text = 0;
}

// 0x00347890.
void VIOptionItem::ResetX70()
{
    m_x70 = 0;
}

// 0x00347898.
int VIOptionItem::GetType()
{
    return m_type;
}

// 0x003478A0.
unsigned short *VIOptionItem::GetLabel()
{
    return m_label;
}

// 0x003478A8.
void VIOptionItem::SetLabel(const unsigned short *s)
{
    VIString::strcpy16(m_label, sizeof(m_label), s);
}

// 0x003478D0.
unsigned short *VIOptionItem::GetText()
{
    return m_text;
}

// 0x003478D8.
void VIOptionItem::SetText(const unsigned short *s)
{
    if (s)
    {
        if (m_text)
            delete [] m_text;
        m_text = new unsigned short[VIString::strlen16(s) + 1];
        VIString::strcpy16(m_text, (VIString::strlen16(s) + 1) * 2, s);
    }
}

// 0x00347950.
int VIOptionItem::GetValue()
{
    return m_value;
}

// 0x00347958.
void VIOptionItem::SetValue(int v)
{
    m_value = v;
}

// 0x00347960.
int VIOptionItem::GetCount()
{
    return m_count;
}

// 0x00347968.
int VIOptionItem::GetCurrent()
{
    return m_value;
}

// 0x00347970.
void VIOptionItem::SetCurrent(int v)
{
    m_value = v;
}

// 0x00347978.
int VIOptionItem::GetSel()
{
    return m_sel;
}

// 0x00347980.
void VIOptionItem::SetSel(int s)
{
    m_sel = VIMIN(VIMAX(s, 0), m_count - 1);
}

// 0x003479A8.
int VIOptionItem::GetX5C()
{
    return m_x5C;
}

// 0x003479B0.
void VIOptionItem::SetX5C(int v)
{
    m_x5C = v;
}

// 0x003479B8.  Where the selection arrow goes: left of the label.
VIVect2I VIOptionItem::GetArrowPos(VIUI *ui)
{
    VIVect2I off(-22, 6);
    switch (m_type)
    {
    case 1:
        return m_pos + off;
    case 2:
    case 3:
    case 4:
        if (m_x50 < 0)
            return VIVect2I(m_pos.x + m_x50 -
                                ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, m_label),
                            m_pos.y) + off;
        return VIVect2I(m_x50, m_pos.y) + off;
    }
    return VIVect2I(0, 0);
}

// 0x00347AB0.  One option on the page.  The focus glow (m_x70) steps toward 30
// while focused and back to 0 otherwise; a field slides left by half of it.
void VIOptionItem::Draw(VIUI *ui, int focus)
{
    VIVect2I p;
    VIColor32F label;
    VIColor32F text;
    label = m_x5C ? VIColor32F(0.8f, 0.3f, 0.1f, 1.0f) : VIColor32F(1.0f, 1.0f, 0.2f, 1.0f);
    text = focus ? VIColor32F(0.0f, 0.0f, 1.0f, 1.0f) : VIColor32F(0.0f, 0.0f, 0.0f, 1.0f);
    int step = m_type == 1 ? 1 : 2;
    if (focus)
        m_x70 = VIMIN(m_x70 + step, 30);
    else
        m_x70 = VIMAX(m_x70 - step, 0);
    switch (m_type)
    {
    case 1:
    {
        float alpha = (float)m_x70 / 30.0f * 0.5f + 0.4f;
        ui->BlitHorz(m_pos, (VIUITexId)184, m_x4C, (VIAlphaBlendMode)3, alpha, 0);
        ui->BlitEx((VIUITexId)182, m_pos, (VIAlphaBlendMode)3, alpha, 0);
        ui->BlitEx((VIUITexId)183, m_pos + VIVect2I(m_x4C - 6, 0), (VIAlphaBlendMode)3, alpha, 0);
        unsigned short *s = m_label;
        int x = m_pos.x + 8;
        if (m_x60 & 4)
            x += m_x4C >> 1;
        ui->DrawCenteredStringEx(s, x, m_pos.y + 11, label, (m_x60 & 4) ? 3 : 1, 1);
        break;
    }
    case 2:
        p = m_pos + VIVect2I(-m_x70 >> 1, 0);
        ui->BlitHorz(p, (VIUITexId)179, m_x4C, (VIAlphaBlendMode)3, 1.0f, 0);
        ui->BlitEx((VIUITexId)177, p, (VIAlphaBlendMode)3, 1.0f, 0);
        ui->BlitEx((VIUITexId)178, p + VIVect2I(m_x4C - 6, 0), (VIAlphaBlendMode)3, 1.0f, 0);
        if (m_x50 < 0)
            ui->DrawStringEx(m_label, m_pos.x + m_x50 -
                                 ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, m_label),
                             m_pos.y + 2, label, 1);
        else
            ui->DrawStringEx(m_label, m_x50, m_pos.y + 2, label, 1);
        ui->DrawStringEx(FitText(ui, m_text, m_x4C - 16, m_x60 & 1),
                         m_pos.x + 8 - (m_x70 >> 1), m_pos.y + 3, text, 0);
        break;
    case 3:
        p = m_pos + VIVect2I(-m_x70 >> 1, 0);
        ui->BlitEx((VIUITexId)(m_value ? 181 : 180), p, (VIAlphaBlendMode)3, 1.0f, 0);
        if (m_x50 < 0)
            ui->DrawStringEx(m_label, m_pos.x + m_x50 -
                                 ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, m_label),
                             m_pos.y + 2, label, 1);
        else
            ui->DrawStringEx(m_label, m_x50, m_pos.y + 2, label, 1);
        break;
    case 4:
        p = m_pos + VIVect2I(-m_x70 >> 1, 0);
        ui->BlitHorz(p, (VIUITexId)179, m_x4C, (VIAlphaBlendMode)3, 1.0f, 0);
        ui->BlitEx((VIUITexId)177, p, (VIAlphaBlendMode)3, 1.0f, 0);
        ui->BlitEx((VIUITexId)178, p + VIVect2I(m_x4C - 6, 0), (VIAlphaBlendMode)3, 1.0f, 0);
        if (m_x50 < 0)
            ui->DrawStringEx(m_label, m_pos.x + m_x50 -
                                 ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, m_label),
                             m_pos.y + 2, label, 1);
        else
            ui->DrawStringEx(m_label, m_x50, m_pos.y + 2, label, 1);
        if (m_x60 & 2)
        {
            unsigned short num[32];
            VIString::snprintf16(num, sizeof(num), "%d", m_min + m_value);
            ui->DrawStringEx(FitText(ui, num, m_x4C - 16, 0),
                             m_pos.x + 8 - (m_x70 >> 1), m_pos.y + 3, text, 0);
        }
        else
            ui->DrawStringEx(FitText(ui, ((const unsigned short **)m_min)[m_value], m_x4C - 16, 0),
                             m_pos.x + 8 - (m_x70 >> 1), m_pos.y + 3, text, 0);
        break;
    }
}

// 0x00348200.  The open list: up to three rows from m_x6C, kept around the
// selection, in a framed grey box with a scrollbar on the right.
void VIOptionItem::DrawList(VIUI *ui)
{
    if (m_type != 4)
        return;
    VIVect2I pos = m_pos + VIVect2I(-m_x70 >> 1, 0);
    VIUIFrameParam fp;
    VIVect2I r[2];
    int h = ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    if (m_sel < m_x6C)
        m_x6C = m_sel;
    if (m_sel >= m_x6C + m_rows)
        m_x6C = m_sel - m_rows + 1;
    int listh = m_rows * h + 8;
    memset(&fp, 0, sizeof(fp));
    fp.m_tl.m_id = (VIUITexId)101;
    fp.m_top.m_id = (VIUITexId)102;
    fp.m_tr.m_id = (VIUITexId)103;
    fp.m_right.m_id = (VIUITexId)104;
    fp.m_br.m_id = (VIUITexId)106;
    fp.m_bottom.m_id = (VIUITexId)102;
    fp.m_bl.m_id = (VIUITexId)105;
    fp.m_left.m_id = (VIUITexId)104;
    r[0].x = pos.x;
    r[0].y = pos.y + 26;
    r[1].x = r[0].x + m_x4C;
    r[1].y = r[0].y + listh;
    VIColor32F c(0.5f, 0.5f, 0.5f, 0.5f);
    VIColor32F blue(0.0f, 0.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    ui->FillRectangle(r, &c);
    ui->BlitFrame(r[0], VIVect2I(m_x4C, listh), fp, (VIAlphaBlendMode)3, 1.0f, 1);
    for (int i = 0; i < m_rows; i++)
    {
        if (m_x60 & 2)
        {
            unsigned short num[32];
            VIString::snprintf16(num, sizeof(num), "%d", m_min + m_x6C + i);
            ui->DrawStringEx(FitText(ui, num, m_x4C - 16, 0), r[0].x + 8, r[0].y + 3 + i * h,
                             m_x6C + i == m_sel ? blue : black, 0);
        }
        else
            ui->DrawStringEx(FitText(ui, ((const unsigned short **)m_min)[m_x6C + i], m_x4C - 16, 0),
                             r[0].x + 8, r[0].y + 3 + i * h, m_x6C + i == m_sel ? blue : black, 0);
    }
    r[0].x = r[1].x - 8;
    c.Set(0.4f, 0.4f, 0.4f, 1.0f);
    ui->FillRectangle(r, &c);
    int y0 = r[0].y + 2;
    int len = r[1].y - r[0].y - 4;
    int thumb = len * m_rows / m_count;
    thumb = thumb < 8 ? 8 : thumb;
    int at = (len - thumb) * m_x6C / (m_count - m_rows);
    r[0].x += 2;
    r[1].x -= 2;
    r[0].y = y0 + at;
    r[1].y = y0 + at + thumb;
    c.Set(0.0f, 0.0f, 0.0f, 1.0f);
    ui->FillRectangle(r, &c);
}

// 0x003486A0.
unsigned short *VIOptionItem::FitText(VIUI *ui, const unsigned short *s, int width, int mask)
{
    static unsigned short buf[128];
    if (!s)
    {
        buf[0] = 0;
        return buf;
    }
    VIString::strcpy16(buf, sizeof(buf), s);
    int n = VIString::strlen16(buf);
    while (ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf) > width)
        buf[--n] = 0;
    if (mask)
        for (int i = 0; i < n; i++)
            buf[i] = '*';
    return buf;
}

// ---- VIWndCombo ---------------------------------------------------------------

// 0x003487A0.
VIWndCombo::VIWndCombo()
{
}

// 0x003487D8.
VIWndCombo::~VIWndCombo()
{
}

// 0x00348800.  The list opens on the item's value.
int VIWndCombo::SetItem(VIOptionItem *item)
{
    m_item = item;
    m_saved = m_item->GetCurrent();
    m_item->SetSel(m_item->GetCurrent());
    return 0;
}

// 0x00348848.
void VIWndCombo::Free()
{
}

// 0x00348850.
void VIWndCombo::OnDrawSelf()
{
    VIUI *ui = UI();
    ui->Begin2D();
    m_item->DrawList(ui);
    ui->End2D();
}

// 0x003488A0.
void VIWndCombo::OnKeyDown(int key)
{
    switch (key)
    {
    case 138:
        m_item->SetSel(m_item->GetSel() - 1);
        break;
    case 139:
        m_item->SetSel(m_item->GetSel() + 1);
        break;
    case 132:
        GetParent()->OnMessage(this, 308, 0, 0);
        break;
    case 134:
        m_item->SetCurrent(m_item->GetSel());
        GetParent()->OnMessage(this, 308, 0, 0);
        break;
    }
}

// 0x003489A8.
void VIWndCombo::OnKeyUp(int key)
{
}

// 0x003489B0.
void VIWndCombo::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if (p2 == (int)this)
            m_item->SetSel(m_item->GetCurrent());
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00348A08.
VIOptionItem *VIWndCombo::GetItem()
{
    return m_item;
}

// 0x00348A10.
void VIWndCombo::Init()
{
}
