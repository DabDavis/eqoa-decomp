#line 1 "C:/code-frontiers-gm/game/VIWndClassMasteryInspect.cpp"
// viwndclassmasteryinspect.cpp -- CLIENT 0x00C35F88 .. 0x00C37B68: a Class Mastery skill
// opened (VICMNode): its description, the statistics it modifies and the abilities it adds
// or removes, a page each; Enter asks to buy it, or to turn an owned one on or off.
// Method names ours.
#include "game.h"

// One WORDS.TXT entry in the current language, as SETUP's windows read them.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C35F88.
VIWndClassMasteryInspect::VIWndClassMasteryInspect()
{
}

// 0x00C35FE0.
VIWndClassMasteryInspect::~VIWndClassMasteryInspect()
{
    Free();
}

// 0x00C36058.
int VIWndClassMasteryInspect::Init()
{
    m_list.Init(UI(), 320, 6);
    m_message.Init();
    m_message.SetVisible(false);
    AddChild(&m_message);
    return 0;
}

// 0x00C360B8.
void VIWndClassMasteryInspect::Free()
{
    if (m_message.GetParent() == this)
    {
        RemoveChild(&m_message);
        m_message.Free();
    }
}

// 0x00C360F8.  The pages this skill has: the text, then the statistics, the abilities it
// adds and those it removes when it has any.
void VIWndClassMasteryInspect::Show(VICMNode *node)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_node = node;
    m_pages[0] = 0;
    m_pageCount = 1;
    if (node->m_reqs.m_size)
    {
        m_pages[1] = 1;
        m_pageCount = 2;
    }
    int adds = 0;
    int removes = 0;
    for (int i = m_node->m_entries.Begin(); i != m_node->m_entries.End(); i = m_node->m_entries.Next(i))
    {
        if (m_node->m_entries[i].m_x80 == 1)
            adds++;
        else
            removes++;
    }
    if (adds)
        m_pages[m_pageCount++] = 2;
    if (removes)
        m_pages[m_pageCount++] = 3;
    ShowPage(0);
}

// 0x00C36220.
int VIWndClassMasteryInspect::ShowPage(int page)
{
    unsigned short buf[256];
    int cols[2] = { 0, 220 };
    int line;
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
        for (i = m_node->m_reqs.Begin(); i != m_node->m_reqs.End(); i = m_node->m_reqs.Next(i))
        {
            VIRec8_BF9990 *r = &m_node->m_reqs[i];
            VIString::snprintf16(buf, sizeof(buf), "%s%d", r->m_x04 < 0 ? "" : "+", r->m_x04);
            switch (r->m_x00)
            {
            case 0:  line = m_list.AddLine(m_window.T("STR")); break;
            case 1:  line = m_list.AddLine(m_window.T("STA")); break;
            case 2:  line = m_list.AddLine(m_window.T("AGI")); break;
            case 3:  line = m_list.AddLine(m_window.T("DEX")); break;
            case 4:  line = m_list.AddLine(m_window.T("WIS")); break;
            case 5:  line = m_list.AddLine(m_window.T("INT")); break;
            case 6:  line = m_list.AddLine(m_window.T("CHA")); break;
            case 7:  line = m_list.AddLine(m_window.T("STR MAX")); break;
            case 8:  line = m_list.AddLine(m_window.T("STA MAX")); break;
            case 9:  line = m_list.AddLine(m_window.T("AGI MAX")); break;
            case 10: line = m_list.AddLine(m_window.T("DEX MAX")); break;
            case 11: line = m_list.AddLine(m_window.T("WIS MAX")); break;
            case 12: line = m_list.AddLine(m_window.T("INT MAX")); break;
            case 13: line = m_list.AddLine(m_window.T("CHA MAX")); break;
            case 14: line = m_list.AddLine(m_window.T("HP MAX")); break;
            case 15: line = m_list.AddLine(m_window.T("PWR MAX")); break;
            case 16: line = m_list.AddLine(m_window.T("HOT")); break;
            case 17: line = m_list.AddLine(m_window.T("POT")); break;
            case 18: line = m_list.AddLine(m_window.T("AC")); break;
            case 19: line = m_list.AddLine(m_window.T("OFF MOD")); break;
            case 20: line = m_list.AddLine(m_window.T("DEF MOD")); break;
            case 21: line = m_list.AddLine(m_window.T("HP FACTOR")); break;
            case 22: line = m_list.AddLine(m_window.T("FIRE RESIST")); break;
            case 23: line = m_list.AddLine(m_window.T("LIGHTNING RESIST")); break;
            case 24: line = m_list.AddLine(m_window.T("COLD RESIST")); break;
            case 25: line = m_list.AddLine(m_window.T("ARCANE RESIST")); break;
            case 26: line = m_list.AddLine(m_window.T("POISON RESIST")); break;
            case 27: line = m_list.AddLine(m_window.T("DISEASE RESIST")); break;
            case 28: line = m_list.AddLine(m_window.T("MOVEMENT RATE")); break;
            }
            m_list.SetColumns(line, 2, cols);
            m_list.SetText(line, 1, buf);
        }
        return 0;
    }
    if (kind == 2)
    {
        for (i = m_node->m_entries.Begin(); i != m_node->m_entries.End(); i = m_node->m_entries.Next(i))
        {
            if (m_node->m_entries[i].m_x80 == 1)
                m_list.AddLine(m_node->m_entries[i].m_text);
        }
        return 0;
    }
    if (kind == 3)
    {
        for (i = m_node->m_entries.Begin(); i != m_node->m_entries.End(); i = m_node->m_entries.Next(i))
        {
            if (!m_node->m_entries[i].m_x80)
                m_list.AddLine(m_node->m_entries[i].m_text);
        }
    }
    return 0;
}

// 0x00C366D8.
void VIWndClassMasteryInspect::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short buf[256];
    ui->Begin2D();
    m_window.Draw();
    if (m_node->m_x04)
        VIString::snprintf16(buf, sizeof(buf), "|x %S  |t %S", Word(ui, 516), Word(ui, 750));
    else if (m_node->m_x05)
        VIString::snprintf16(buf, sizeof(buf), "|x %S  |t %S", Word(ui, 63), Word(ui, 750));
    else
        VIString::snprintf16(buf, sizeof(buf), "|-|x %S|+  |t %S", Word(ui, 63), Word(ui, 750));
    m_window.Text(buf, 40, 10, (VIUICOLOR)0, 0);
    DrawHeader();
    m_list.Draw(&m_window, 40, 3);
    ui->End2D();
    if (m_keys.Pulse(138))
    {
        if (m_list.Scroll(-1))
            app->PlaySound(9);
        else
            app->PlaySound(11);
    }
    if (m_keys.Pulse(139))
    {
        if (m_list.Scroll(1))
            app->PlaySound(9);
        else
            app->PlaySound(11);
    }
}

// 0x00C368D0.  Left/Right turn the page; Enter asks to switch an owned skill or to buy one.
void VIWndClassMasteryInspect::OnKeyDown(int key)
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
    case 13:
    case 134:
        if (m_node->m_x04)
        {
            m_message.Setup(Word(UI(), 1665), Word(UI(), 1666), (VIMESSAGEPROMPT)2,
                            m_window.Center(), 300);
            m_message.SetVisible(true);
            app->PlaySound(15);
            app->SetActiveWindow(&m_message);
        }
        else if (m_node->m_x05)
        {
            unsigned short text[256];
            if (m_node->m_x10 == 4)
                VIString::snprintf16(text, sizeof(text), "%S %S", Word(UI(), 1807), Word(UI(), 1664));
            else
                VIString::snprintf16(text, sizeof(text), "%S", Word(UI(), 1664));
            m_message.Setup(Word(UI(), 1663), text, (VIMESSAGEPROMPT)2, m_window.Center(), 350);
            m_message.SetVisible(true);
            app->PlaySound(15);
            app->SetActiveWindow(&m_message);
        }
        break;
    case 27:
    case 132:
        SetVisible(false);
        app->PlaySound(8);
        app->SetActiveWindow(GetParent());
        break;
    }
}

// 0x00C36C28.
void VIWndClassMasteryInspect::OnKeyUp(int key)
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

// 0x00C36CA0.  295: the focus arrived; 307: the question's answer (0 = yes).
void VIWndClassMasteryInspect::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
    {
        if ((VIWnd *)p2 != this || (VIWnd *)p1 == &m_message)
            break;
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 400, 11, 0);
        unsigned short title[256];
        VIString::snprintf16(title, sizeof(title), "Inspect: %S", m_node->m_title);
        m_window.SetTitle(title);
        m_window.Open(true);
        break;
    }
    case 307:
        app->SetActiveWindow(this);
        if (w != &m_message || p1)
            break;
        if (m_node->m_x04)
            app->GetP4EC58()->Send03(m_node->m_x08);
        else
            app->GetP4EC58()->Send02(m_node->m_x08);
        SetVisible(false);
        app->PlaySound(8);
        app->SetActiveWindow(GetParent());
        break;
    }
}

// 0x00C36E48.
void VIWndClassMasteryInspect::DrawHeader()
{
    unsigned short buf[256];
    m_window.DrawFrame(40, 1, m_window.Width() - 80, 1, 32);
    switch (m_pages[m_page])
    {
    case 0:
        VIString::str8to16(buf, sizeof(buf), "Description");
        break;
    case 1:
        VIString::str8to16(buf, sizeof(buf), "Modifies Statistics");
        break;
    case 2:
        VIString::str8to16(buf, sizeof(buf), "Adds Abilities");
        break;
    case 3:
        VIString::str8to16(buf, sizeof(buf), "Removes Abilities");
        break;
    default:
        buf[0] = 0;
        break;
    }
    m_window.TextBox(buf, 40, 1, m_window.Width() - 80, 1, (VIUICOLOR)0, 8);
}
