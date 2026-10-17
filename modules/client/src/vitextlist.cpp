#line 1 "C:/code-frontiers-gm/game/VITextList.cpp"
// VITextList (name ours) -- CLIENT 0x00C0C398 .. 0x00C0CC88, then its VIVector<VITextLine>
// copies (client_vivector.cpp holds the canonical ones) to 0x00C0D000.  A scrolling list
// of text lines in up to four columns, drawn into a VIUIWindow: the class-mastery
// windows' lists.
#include "game.h"

extern "C" void *memset(void *, int, int);
extern "C" void *memcpy(void *, const void *, unsigned int);

// 0x00C0C398.
VITextList::VITextList()
{
    m_lines.Init(0);
}

// 0x00C0C3E0.
VITextList::~VITextList()
{
    Free();
}

// 0x00C0C430.
int VITextList::Init(VIUI *ui, int width, int rows)
{
    Free();
    m_ui = ui;
    m_width = width;
    m_rows = rows;
    m_showSel = 1;
    return 0;
}

// 0x00C0C490.
void VITextList::Free()
{
    for (int i = m_lines.Begin(); i != m_lines.End(); i = m_lines.Next(i))
        for (int j = 0; j < 4; j++)
            if (m_lines[i].m_text[j])
                delete [] m_lines[i].m_text[j];
    m_lines.Clear();
    m_top = 0;
    m_sel = -1;
}

// 0x00C0C578.
int VITextList::Count()
{
    return m_lines.m_size;
}

// 0x00C0C580.
void VITextList::Clear()
{
    Free();
}

// 0x00C0C5A0.
int VITextList::GetTop()
{
    return m_top;
}

// 0x00C0C5A8.
void VITextList::Nop()
{
}

// 0x00C0C5B0.
int VITextList::AddLine(const unsigned short *text)
{
    VITextLine line;

    memset(&line, 0, sizeof(line));
    line.m_cols = 1;
    line.m_text[0] = new unsigned short[VIString::strlen16(text) + 1];
    VIString::strcpy16(line.m_text[0], (VIString::strlen16(text) + 1) * 2, text);
    return m_lines.Add(line);
}

// 0x00C0C638.
int VITextList::GetShowSel()
{
    return m_showSel;
}

// 0x00C0C640.
void VITextList::SetShowSel(int on)
{
    m_showSel = on;
}

// 0x00C0C648: one line per '\n', and a break at the last space (or at the character)
// where the line would be wider than m_width - 24.  Returns the last AddLine's result,
// uninitialised when it added nothing.
int VITextList::AddText(const unsigned short *text)
{
    unsigned short buf[256];
    int space = 0;
    const unsigned short *start = text;
    int r;
    int i = 0;

    while (text[i])
    {
        if (text[i] == '\n')
        {
            memcpy(buf, start, (&text[i + 1] - start) * sizeof(unsigned short));
            buf[&text[i + 1] - start] = 0;
            i++;
            space = 0;
            r = AddLine(buf);
            start = &text[i];
        }
        else
        {
            unsigned short c = text[i];
            if (c == ' ')
                space = i;
            memcpy(buf, start, (&text[i] - start) * sizeof(unsigned short));
            buf[&text[i] - start] = 0;
            if (m_ui->m_font.GetStringWidth(m_ui->m_raster, m_ui->m_dict, buf) > m_width - 24)
            {
                if (space)
                {
                    memcpy(buf, start, (&text[space + 1] - start) * sizeof(unsigned short));
                    buf[&text[space + 1] - start] = 0;
                    i = space + 1;
                    start = &text[i];
                }
                else
                {
                    memcpy(buf, start, (&text[i] - start) * sizeof(unsigned short));
                    buf[&text[i] - start] = 0;
                    start = &text[i];
                }
                r = AddLine(buf);
                space = 0;
            }
            else
                i++;
        }
    }
    if (&text[i] - start)
    {
        memcpy(buf, start, (&text[i] - start) * sizeof(unsigned short));
        buf[&text[i] - start] = 0;
        r = AddLine(buf);
    }
    return r;
}

// 0x00C0C838.
void VITextList::SetColumns(int line, int n, const int *x)
{
    VITextLine &l = m_lines[line];

    l.m_cols = n;
    for (int i = 0; i < n; i++)
        l.m_x[i] = x[i];
}

// 0x00C0C880.  Through a POINTER to the line, each use spelled `l->m_text[col]`: the
// address is ((data + line*36) + 20) + col*4; a reference to the slot folds the 20 into
// the column offset.
void VITextList::SetText(int line, int col, const unsigned short *s)
{
    VITextLine *l = &m_lines[line];

    if (l->m_text[col])
        delete [] l->m_text[col];
    l->m_text[col] = new unsigned short[VIString::strlen16(s) + 1];
    VIString::strcpy16(l->m_text[col], (VIString::strlen16(s) + 1) * 2, s);
}

// 0x00C0C908.
int VITextList::GetSel()
{
    return m_sel;
}

// 0x00C0C910.
void VITextList::SetSel(int i)
{
    m_sel = i;
    if (i < 0)
        m_sel = 0;
    if (m_sel > m_lines.m_size - 1)
        m_sel = m_lines.m_size - 1;
}

// 0x00C0C940.
int VITextList::Scroll(int d)
{
    if (d < 0 && m_top == 0)
        return 1;
    if (d > 0 && !(m_top < m_lines.m_size - m_rows))
        return 1;
    int t = m_top + d;
    m_top = t;
    if (t < 0)
        m_top = 0;
    else if (t > m_lines.m_size - m_rows)
        m_top = m_lines.m_size - m_rows;
    return 0;
}

// 0x00C0C9B0.
void VITextList::Draw(VIUIWindow *w, int x, int y)
{
    if (m_sel != -1)
    {
        if (m_sel < m_top)
            m_top = m_sel;
        if (!(m_sel < m_top + m_rows))
            m_top = m_sel - m_rows + 1;
    }
    if (m_lines.m_size > m_rows || m_top > 0)
        w->DrawFrameVScroll(x, y, m_width, m_rows, (float)m_top / (float)(m_lines.m_size - m_rows));
    else
        w->DrawFrame(x, y, m_width, m_rows, 0);
    if (m_showSel && !m_lines.m_size)
        w->TickR(x, y, false);
    for (int row = 0; row < m_rows; row++)
    {
        if (m_lines.m_size <= m_top + row)
            break;
        if (m_sel != -1 && m_top + row == m_sel && m_showSel)
        {
            if (m_lines.m_size)
                w->TickR(x, y + row, true);
        }
        VITextLine &l = m_lines[m_top + row];
        for (int col = 0; col < l.m_cols; col++)
        {
            if (!l.m_text[col])
                continue;
            int width;
            if (col + 1 == l.m_cols)
                width = m_width - l.m_x[col];
            else
                width = l.m_x[col + 1] - l.m_x[col];
            int sel = m_sel == -1 ? 0 : m_top + row == m_sel;
            int color;
            if (!m_showSel)
                color = 2;
            else
                color = sel;
            w->TextBox(l.m_text[col], x + l.m_x[col], y + row, width, 1, (VIUICOLOR)color, 2);
        }
    }
}
