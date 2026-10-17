#line 1 "C:/code-frontiers-gm/game/VIWndClassMasteryRequire.cpp"
// viwndclassmasteryrequire.cpp -- CLIENT 0x00C38DA0 .. 0x00C3A580: what a Class Mastery
// skill not yet known requires (VICMNode): the level, the masteries and abilities needed,
// the minimum statistics and the points spent, a page each.  Method names ours.
#include "game.h"

// One WORDS.TXT entry in the current language, as SETUP's windows read them.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C38DA0.
VIWndClassMasteryRequire::VIWndClassMasteryRequire()
{
}

// 0x00C38DF0.
VIWndClassMasteryRequire::~VIWndClassMasteryRequire()
{
}

// 0x00C38E58.
int VIWndClassMasteryRequire::Init()
{
    m_list.Init(UI(), 320, 6);
    return 0;
}

// 0x00C38E88.
void VIWndClassMasteryRequire::Free()
{
}

// 0x00C38E90.  The general requirements always; the masteries, the abilities and the
// statistics when the skill has any.
void VIWndClassMasteryRequire::Show(VICMNode *node)
{
    m_node = node;
    m_pages[0] = 1;
    m_pageCount = 1;
    if (node->m_lines.m_size)
    {
        m_pages[1] = 2;
        m_pageCount = 2;
    }
    int abilities = 0;
    for (int i = m_node->m_entries.Begin(); i != m_node->m_entries.End(); i = m_node->m_entries.Next(i))
    {
        if (!m_node->m_entries[i].m_x80)
        {
            abilities = 1;
            break;
        }
    }
    if (abilities)
        m_pages[m_pageCount++] = 3;
    if (m_node->m_x1C || m_node->m_x20 || m_node->m_x24 || m_node->m_x28
        || m_node->m_x2C || m_node->m_x30 || m_node->m_x34 || m_node->m_x18)
        m_pages[m_pageCount++] = 4;
    ShowPage(0);
}

// 0x00C38FE8.
int VIWndClassMasteryRequire::ShowPage(int page)
{
    unsigned short buf[256];
    int cols[2] = { 0, 220 };
    int i;
    if (page < 0)
        return -1;
    if (page >= m_pageCount)
        return -1;
    m_page = page;
    m_list.Clear();
    int kind = m_pages[m_page];
    if (kind == 0)
    {
        m_list.AddText(m_node->m_text);
        return 0;
    }
    if (kind == 1)
    {
        VIString::snprintf16(buf, sizeof(buf), "Level: %d", m_node->m_x11);
        m_list.AddLine(buf);
        return 0;
    }
    if (kind == 2)
    {
        for (i = m_node->m_lines.Begin(); i != m_node->m_lines.End(); i = m_node->m_lines.Next(i))
            m_list.AddLine(m_node->m_lines[i].m_text);
        return 0;
    }
    if (kind == 3)
    {
        for (i = m_node->m_entries.Begin(); i != m_node->m_entries.End(); i = m_node->m_entries.Next(i))
        {
            if (!m_node->m_entries[i].m_x80)
                m_list.AddLine(m_node->m_entries[i].m_text);
        }
        return 0;
    }
    if (kind == 4)
    {
        for (i = 0; i < 8; i++)
        {
            int line = -1;
            switch (i)
            {
            case 0:
                if (!m_node->m_x1C)
                    break;
                line = m_list.AddLine(m_window.T("STR:"));
                VIString::snprintf16(buf, sizeof(buf), "%d", m_node->m_x1C);
                break;
            case 1:
                if (!m_node->m_x20)
                    break;
                line = m_list.AddLine(m_window.T("STA:"));
                VIString::snprintf16(buf, sizeof(buf), "%d", m_node->m_x20);
                break;
            case 2:
                if (!m_node->m_x24)
                    break;
                line = m_list.AddLine(m_window.T("AGI:"));
                VIString::snprintf16(buf, sizeof(buf), "%d", m_node->m_x24);
                break;
            case 3:
                if (!m_node->m_x28)
                    break;
                line = m_list.AddLine(m_window.T("DEX:"));
                VIString::snprintf16(buf, sizeof(buf), "%d", m_node->m_x28);
                break;
            case 4:
                if (!m_node->m_x2C)
                    break;
                line = m_list.AddLine(m_window.T("WIS:"));
                VIString::snprintf16(buf, sizeof(buf), "%d", m_node->m_x2C);
                break;
            case 5:
                if (!m_node->m_x30)
                    break;
                line = m_list.AddLine(m_window.T("INT:"));
                VIString::snprintf16(buf, sizeof(buf), "%d", m_node->m_x30);
                break;
            case 6:
                if (!m_node->m_x34)
                    break;
                line = m_list.AddLine(m_window.T("CHA:"));
                VIString::snprintf16(buf, sizeof(buf), "%d", m_node->m_x34);
                break;
            case 7:
                if (!m_node->m_x18)
                    break;
                line = m_list.AddLine(m_window.T("Points Spent:"));
                VIString::snprintf16(buf, sizeof(buf), "%d", m_node->m_x18);
                break;
            }
            if (line != -1)
            {
                m_list.SetColumns(line, 2, cols);
                m_list.SetText(line, 1, buf);
            }
        }
    }
    return 0;
}

// 0x00C39468.
void VIWndClassMasteryRequire::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short buf[256];
    ui->Begin2D();
    m_window.Draw();
    VIString::snprintf16(buf, sizeof(buf), "|t %S", Word(ui, 750));
    m_window.Text(buf, 40, 10, (VIUICOLOR)0, 0);
    DrawHeader();
    m_list.Draw(&m_window, 40, 3);
    ui->End2D();
    if (m_keys.Pulse(138))
        m_list.Scroll(-1);
    if (m_keys.Pulse(139))
        m_list.Scroll(1);
}

// 0x00C39580.
void VIWndClassMasteryRequire::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 19:
    case 138:
        m_keys.Press(138);
        break;
    case 20:
    case 139:
        m_keys.Press(139);
        break;
    case 17:
    case 136:
        app->PlaySound(11);
        ShowPage((m_page + (m_pageCount - 1)) % m_pageCount);
        break;
    case 18:
    case 137:
        app->PlaySound(11);
        ShowPage((m_page + 1) % m_pageCount);
        break;
    case 27:
    case 132:
        SetVisible(false);
        app->PlaySound(8);
        app->SetActiveWindow(GetParent());
        break;
    }
}

// 0x00C39708.
void VIWndClassMasteryRequire::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        m_keys.Release(138);
        break;
    case 20:
    case 139:
        m_keys.Release(139);
        break;
    }
}

// 0x00C39780.  295: the focus arrived.
void VIWndClassMasteryRequire::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295 && (VIWnd *)p2 == this)
    {
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 400, 11, 0);
        unsigned short title[256];
        VIString::snprintf16(title, sizeof(title), "Requirements");
        m_window.SetTitle(title);
        m_window.Open(true);
    }
}

// 0x00C39848.
void VIWndClassMasteryRequire::DrawHeader()
{
    unsigned short buf[256];
    m_window.DrawFrame(40, 1, m_window.Width() - 80, 1, 32);
    switch (m_pages[m_page])
    {
    case 0:
        VIString::str8to16(buf, sizeof(buf), "Description");
        break;
    case 1:
        VIString::str8to16(buf, sizeof(buf), "General Requirements");
        break;
    case 2:
        VIString::str8to16(buf, sizeof(buf), "Required Masteries");
        break;
    case 3:
        VIString::str8to16(buf, sizeof(buf), "Required Abilities");
        break;
    case 4:
        VIString::str8to16(buf, sizeof(buf), "Minimum Statistics");
        break;
    case 5:
        VIString::str8to16(buf, sizeof(buf), "Faction Standing");
        break;
    default:
        buf[0] = 0;
        break;
    }
    m_window.TextBox(buf, 40, 1, m_window.Width() - 80, 1, (VIUICOLOR)0, 8);
}
