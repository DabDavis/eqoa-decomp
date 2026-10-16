// viguicombobox.cpp (name ours) -- STATION 0x002BE3E8 .. 0x002BFD18: a text whose
// value is picked from a popup list of options.  Then the item-header template
// copies, the VIList<char *> copies and __tf13VIGUIComboBox.
#include "station.h"
#include "vilist_t.h"

#define OPTION(it) (((VIList<char *>::Node *)(it))->m_data)

// 0x002BE3E8.
VIGUIComboBox::VIGUIComboBox(const char *name, VIWndGenericRenderer *r, VIXMLParser *reader)
    : VIGUIText(name, 5, r, reader)
{
    m_selected = 0;
    m_top = 0;
    m_open = 0;
    m_rows = 1;
    m_popupW = -1;
    m_popupH = 75;
}

// 0x002BE458.
VIGUIComboBox::~VIGUIComboBox()
{
    int it = m_items.Begin();
    while (it != m_items.End())
    {
        char *s = OPTION(it);
        int next = m_items.Erase(it);
        if (s)
            free(s);
        it = next;
    }
}

// 0x002BE520.
void VIGUIComboBox::AddItem(const char *s)
{
    if (s)
    {
        char *copy = strdup(s);
        if (copy)
            m_items.Add(copy);
    }
}

// 0x002BE568.  Keeps the selection inside the visible rows.
void VIGUIComboBox::MoveSelection(int delta)
{
    int top = m_top;
    int bottom = top + m_rows - 1;
    int sel = m_selected + delta;
    if (sel >= 0 && sel < m_items.Size())
    {
        m_selected += delta;
        if (m_selected < top)
            m_top -= top - m_selected;
        else if (bottom < m_selected)
            m_top += m_selected - bottom;
    }
}

// 0x002BE618.  Option `index` into the model.
void VIGUIComboBox::Select(int index)
{
    VIGUIStringDataModel *model = GetModel();
    if (!model)
        return;
    int i = 0;
    for (int it = m_items.Begin(); it != m_items.End(); it = m_items.Next(it), i++)
    {
        if (i == index)
        {
            if (OPTION(it))
                model->SetString(OPTION(it));
            return;
        }
    }
}

// 0x002BE6C8.  Selects the option the model already holds.
void VIGUIComboBox::SyncSelection()
{
    VIGUIStringDataModel *model = GetModel();
    if (!model)
        return;
    const char *cur = model->GetString();
    if (!cur)
        return;
    int i = 0;
    for (int it = m_items.Begin(); it != m_items.End(); it = m_items.Next(it), i++)
    {
        if (OPTION(it) && !strcmp(OPTION(it), cur))
        {
            m_selected = i;
            MoveSelection(0);
            return;
        }
    }
}

// 0x002BE790.
void VIGUIComboBox::ClearItems()
{
    int it = m_items.Begin();
    while (it != m_items.End())
    {
        char *s = OPTION(it);
        int next = m_items.Erase(it);
        if (s)
            free(s);
        it = next;
    }
    m_items.Clear();
}

// 0x002BE818.  Closed, accept opens it; open, up and down move, accept takes the
// option and cancel closes it.
int VIGUIComboBox::OnKeyDown(int key)
{
    if (!m_renderer)
        return 0;
    if (m_open)
    {
        if (key == 138)
            MoveSelection(-1);
        else if (key == 139)
            MoveSelection(1);
        else if (key == 134)
        {
            Select(m_selected);
            m_open = 0;
        }
        else if (key == 132)
            m_open = 0;
        return 1;
    }
    if (key == 134)
    {
        SyncSelection();
        m_open ^= 1;
    }
    return m_open;
}

// 0x002BE8E0.
void VIGUIComboBox::SetPopupSize(int w, int h)
{
    m_popupW = w;
    m_popupH = h;
}

// 0x002BE8F0.
int VIGUIComboBox::OnKeyUp(int key)
{
    return 0;
}

// 0x002BE8F8.  The box as a text; open, the popup under it: a grey fill, a
// frame, the rows from m_top in the text colour (the selected one in the roll
// colour), and a scrollbar when the options do not fit.  Unlike VIGUIText::Draw
// a long option IS drawn from its heap buffer.
void VIGUIComboBox::Draw(float alpha)
{
    if (!m_renderer)
        return;
    void *app = m_renderer->GetAppPtr();
    if (!app)
        return;
    VIUI *ui = ST_UI(app);
    m_rows = m_popupH / ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    VIGUIText::Draw(alpha);
    if (!m_open)
        return;
    int x = m_x + m_imageOffX;
    int y = m_y + m_imageOffY;
    VIUIFrameParam fp;
    memset(&fp, 0, 64);
    fp.m_tl.m_id = (VIUITexId)101;
    fp.m_top.m_id = (VIUITexId)102;
    fp.m_tr.m_id = (VIUITexId)103;
    fp.m_right.m_id = (VIUITexId)104;
    fp.m_br.m_id = (VIUITexId)106;
    fp.m_bottom.m_id = (VIUITexId)102;
    fp.m_bl.m_id = (VIUITexId)105;
    fp.m_left.m_id = (VIUITexId)104;
    VIVect2I r[2];
    int w = m_width;
    if (m_popupW != -1)
        w = m_popupW;
    r[0].x = x;
    r[0].y = y + m_height;
    r[1].x = x + w;
    r[1].y = y + m_popupH + m_height;
    VIColor32F grey(0.5f, 0.5f, 0.5f, 0.5f);
    int i = 0;
    ui->FillRectangle(r, &grey);
    ui->BlitFrame(VIVect2I(x, y + m_height), VIVect2I(w, m_popupH), fp, VIAlphaBlendMode3,
                  m_alpha, 1);
    int it;
    for (it = m_items.Begin(); it != m_items.End() && i < m_top; it = m_items.Next(it))
        i++;
    int h = ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    int row = 0;
    int ty = m_y + m_height + 5;
    for (; row < m_rows && it != m_items.End(); it = m_items.Next(it), row++)
    {
        VIColor32F c = (row + i == m_selected) ? m_rollColor : m_textColor;
        ui->m_font.SetTextColor(c);
        char *s = OPTION(it);
        if (s)
        {
            int n = strlen(s);
            if (n >= 1024)
            {
                n++;
                unsigned short *wide = new unsigned short[n];
                VIString::str8to16(wide, n, s);
                ui->m_font.DrawString(ui->m_raster, ui->m_dict, wide, m_x, ty + row * h);
                if (wide)
                    delete[] wide;
            }
            else
            {
                char *f = Fit(s, w - 16);
                unsigned short wbuf[1024];
                VIString::str8to16(wbuf, 2048, f);
                ui->m_font.DrawString(ui->m_raster, ui->m_dict, wbuf, m_x, ty + row * h);
                free(f);
            }
        }
    }
    if (m_rows < m_items.Size())
    {
        r[0].x = x + w - 10;
        r[0].y = y + m_height + 3;
        r[1].x = x + w - 5;
        r[1].y = y + m_popupH + m_height - 3;
        VIColor32F track(0.7f, 0.7f, 0.7f, 1.0f);
        ui->FillRectangle(r, &track);
        int thumb = (int)((float)m_popupH / (float)(m_items.Size() - m_rows + 1));
        int top = y + m_height + 3;
        int bottom = y + m_popupH + m_height - 3;
        if (thumb <= 1)
            thumb = 2;
        int range = bottom - thumb;
        float t = (float)m_top / (float)(m_items.Size() - m_rows);
        int pos = (int)(range * t + top * (1.0 - t));
        r[0].x = x + w - 10;
        r[0].y = pos;
        r[1].x = x + w - 5;
        r[1].y = pos + thumb;
        VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
        ui->FillRectangle(r, &black);
    }
}
