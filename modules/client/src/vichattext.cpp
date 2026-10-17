// CLIENT 0x00C03A10 .. 0x00C04CF8: three word lookups and the chat window's line ring
// (cl_parts2.h, VIChatText), then this TU's own template copies.  The file name is ours.
#include "game.h"

// 0x00C03A10
VIWordId *FindWord_CDF428(int id)
{
    VIWordId *found = 0;
    for (unsigned int i = 0; i < 4; i++)
        if (gWords_CDF428[i].m_id == id)
            found = &gWords_CDF428[i];
    return found;
}

// 0x00C03A48
VIWordId *FindAttackWord(int id)
{
    VIWordId *found = 0;
    for (unsigned int i = 0; i < 11; i++)
        if (gAttackWords[i].m_id == id)
            found = &gAttackWords[i];
    return found;
}

// 0x00C03A80
VIWordId *FindEquipSlotWord(int id)
{
    VIWordId *found = 0;
    for (unsigned int i = 0; i < 23; i++)
        if (gEquipSlotWords[i].m_id == id)
            found = &gEquipSlotWords[i];
    return found;
}

// 0x00C03AB8
VIChatText::VIChatText()
{
    m_app = 0;
    m_width = 0;
    Reset();
}

// 0x00C03B18
VIChatText::~VIChatText()
{
    Shutdown();
}

// 0x00C03B60
int VIChatText::Init(VIClient *app, int width)
{
    m_app = app;
    m_width = width - 30;
    Reset();
    return 0;
}

// 0x00C03B90
int VIChatText::Shutdown()
{
    m_app = 0;
    m_width = 0;
    Reset();
    return 0;
}

// 0x00C03BB8
void VIChatText::Reset()
{
    for (int i = 0; i < 32; i++)
    {
        m_lines[i].m_text[0] = 0;
        m_lines[i].m_color = VIColor32F(1.0f, 1.0f, 1.0f, 1.0f);
        m_lines[i].m_type = 0;
        m_lines[i].m_time = 0;
    }
    m_head = 0;
    m_tail = 0;
}

// 0x00C03C28
void VIChatText::Age()
{
    if (m_app)
        for (int i = 0; i < 32; i++)
            if (m_app->m_p07568 - m_lines[i].m_time > 20000)
                m_lines[i].m_type = 0;
}

// 0x00C03C78
int VIChatText::First()
{
    return m_head;
}

// 0x00C03C80
int VIChatText::End()
{
    return m_tail;
}

// 0x00C03C88
int VIChatText::Next(int i)
{
    return i == 31 ? 0 : i + 1;
}


// 0x00C03CA0
int VIChatText::Prev(int i)
{
    return i ? i - 1 : 31;
}


// 0x00C03CB0
int VIChatText::Count()
{
    if (m_tail < m_head)
        return 31;
    return m_tail - m_head;
}

// 0x00C03CD8
int VIChatText::Index(int i)
{
    if (i < m_head)
        return 32 - m_head + i;
    return i - m_head;
}


// 0x00C03CF8
int VIChatText::AddText(const char *s, VIColor32F c)
{
    unsigned short buf[512];
    VIString::str8to16(buf, 1024, s);
    return AddText(buf, c);
}

// 0x00C03D68: wraps at m_width pixels, at most 63 characters a line, at the last space
// when there is one; a line of its own starts with a space.
int VIChatText::AddText(const unsigned short *s, VIColor32F c)
{
    int w = 0;
    int space = 0;
    int pos = 0;
    int n = 0;
    VIUI &ui = m_app->m_ui;
    while (s[pos])
    {
        while (w < m_width && n < 63 && m_tail < 32 && s[pos])
        {
            if (s[pos] == ' ')
                space = n;
            if (s[pos] == '\n' && s[pos + 1])
            {
                pos++;
                AddLine(c);
                space = 0;
                w = 0;
                n = 1;
                m_lines[m_tail].m_text[space] = ' ';
                m_lines[m_tail].m_text[n] = 0;
            }
            if (s[pos])
            {
                m_lines[m_tail].m_text[n++] = s[pos++];
                m_lines[m_tail].m_text[n] = 0;
                w = ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, m_lines[m_tail].m_text);
            }
        }
        if (!s[pos])
            break;
        if (m_lines[m_tail].m_text[n - 1] != ' ' && space > 0)
        {
            m_lines[m_tail].m_text[space] = 0;
            pos -= n - space - 1;
        }
        AddLine(c);
        space = 0;
        n = 1;
        m_lines[m_tail].m_text[space] = ' ';
        m_lines[m_tail].m_text[n] = 0;
        w = 0;
    }
    AddLine(c);
    return 0;
}

// 0x00C04020
int VIChatText::AddLine(VIColor32F c)
{
    m_lines[m_tail].m_color = c;
    m_lines[m_tail].m_type = 1;
    m_lines[m_tail].m_time = m_app->m_p07568;
    if (m_head == 0 && m_tail == 0)
        m_tail++;
    else if (m_head == 0 && m_tail < 31)
        m_tail++;
    else
    {
        m_head = Next(m_head);
        m_tail = Next(m_tail);
    }
    return 0;
}

// 0x00C04110
VIChatLine *VIChatText::Line(unsigned int i)
{
    if (i >= 32)
        return 0;
    return &m_lines[i];
}
