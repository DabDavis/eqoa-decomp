#line 1 "C:/code-frontiers-gm/game/VIWndClassMastery.cpp"
// viwndclassmastery.cpp -- CLIENT 0x00C33928 .. 0x00C35F88: the Class Mastery window.  The
// client (VIClientPart4EC58, VIClient's slot 14) holds the skills; this window sorts the
// category's skills into rank chains and shows the highest rank owned and the lowest rank
// available of each, in two VITextLists.  Method names ours.
#include "game.h"
#include "vilist_t.h"

// One WORDS.TXT entry in the current language, as SETUP's windows read them.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00CD7528: the five categories' names (WORDS.TXT).
static int sCategoryWords[] = { 1653, 1654, 1655, 511, 1656 };

// 0x00C33928.
VIWndClassMastery::VIWndClassMastery()
{
}

// 0x00C33988.
VIWndClassMastery::~VIWndClassMastery()
{
}

// 0x00C33A00.  The three child windows, hidden; the client shows its skills through us.
int VIWndClassMastery::Init()
{
    m_points = new VIWndClassMasteryPoints;
    m_points->SetAppPtr(GetAppPtr());
    m_points->Init();
    m_points->SetVisible(false);
    AddChild(m_points);
    m_inspect = new VIWndClassMasteryInspect;
    m_inspect->SetAppPtr(GetAppPtr());
    m_inspect->Init();
    m_inspect->SetVisible(false);
    AddChild(m_inspect);
    m_require = new VIWndClassMasteryRequire;
    m_require->SetAppPtr(GetAppPtr());
    m_require->Init();
    m_require->SetVisible(false);
    AddChild(m_require);
    m_busy = 0;
    ((VIClient *)GetAppPtr())->GetP4EC58()->m_wnd = this;
    return 0;
}

// 0x00C33B18.  Empty (the child windows are never deleted).
void VIWndClassMastery::Free()
{
}

// The skills shown: the highest rank owned of each chain, and the lowest available.
// ONE object: the image's immediates are symbol-relative, and each 'available' reference reads +800.
static VICMSkill *sSkills[2][200];   // 0x00CF5468: [0] owned, [1] available (0x00CF5788)
static int        sCount[2];         // 0x00CF5AA8

// 0x00C33B20.
void VIWndClassMastery::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short buf[256];
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 332), (VIAlphaBlendMode)0, 1.0f);
    m_window.Draw();
    m_window.Text(m_window.T("Points Available/Spent:"), 40, 1, (VIUICOLOR)0, 0);
    VIString::snprintf16(buf, sizeof(buf), "%d/%d", app->GetP4EC58()->Points(),
                         app->GetP4EC58()->m_x10);
    m_window.Text(buf, 260, 1, (VIUICOLOR)0, 0);
    m_window.Text(m_window.T("Category:"), 40, 2, (VIUICOLOR)0, 0);
    m_window.DrawFrame(260, 2, m_window.Width() - 300, 1, 32);
    m_window.TextBox(Word(ui, sCategoryWords[m_category]), 260, 2, m_window.Width() - 300, 1,
                     (VIUICOLOR)0, 8);
    DrawLists();
    switch (m_column)
    {
    case 0:
        if (m_owned.GetSel() != -1)
            VIString::snprintf16(buf, sizeof(buf), "|x %S  |t %S  |s %S", Word(ui, 1657),
                                 Word(ui, 750), Word(ui, 1658));
        else
            VIString::snprintf16(buf, sizeof(buf), "|t %S  |s %S", Word(ui, 750), Word(ui, 1658));
        break;
    case 1:
        if (m_available.GetSel() != -1)
        {
            VICMSkill *&s = sSkills[1][m_available.GetSel()];
            if (app->GetP4EC58()->IsKnown(s->m_id))
                VIString::snprintf16(buf, sizeof(buf), "|x %S  |t %S  |s %S", Word(ui, 1657),
                                     Word(ui, 750), Word(ui, 1658));
            else
                VIString::snprintf16(buf, sizeof(buf), "|x %S  |t %S  |s %S", Word(ui, 1659),
                                     Word(ui, 750), Word(ui, 1658));
        }
        else
            VIString::snprintf16(buf, sizeof(buf), "|t %S  |s %S", Word(ui, 750), Word(ui, 1658));
        break;
    }
    m_window.Text(buf, 40, 13, (VIUICOLOR)0, 0);

    // What the client is waiting for, in a box over the lists.
    const unsigned short *msg = 0;
    unsigned short text[64];
    text[0] = 0;
    if (--m_ticks <= 0)
    {
        m_ticks = 15;
        if (++m_dots == 3)
            m_dots = 0;
    }
    switch (app->GetP4EC58()->m_pending)
    {
    case 0:
        if (m_busy)
        {
            SelectCategory(m_category);
            m_busy = 0;
        }
        break;
    case 1:
        msg = Word(UI(), 1766);
        break;
    case 2:
        msg = Word(UI(), 1767);
        m_busy = 1;
        break;
    case 3:
        msg = Word(UI(), 1768);
        m_busy = 1;
        break;
    case 4:
        VIString::strcpy16(text, sizeof(text), Word(UI(), 1811));
        for (char d = 0; d < m_dots + 1; d++)
            VIString::strcat16(text, sizeof(text), ".");
        msg = text;
        m_busy = 1;
        break;
    case 5:
        VIString::strcpy16(text, sizeof(text), Word(UI(), 1812));
        for (char d = 0; d < m_dots + 1; d++)
            VIString::strcat16(text, sizeof(text), ".");
        msg = text;
        m_busy = 0;
        break;
    }
    if (msg)
    {
        VIUIWindow box;
        int w;
        if (app->GetP4EC58()->m_pending == 4)
        {
            VIUI *u = UI();
            w = u->m_font.GetStringWidth(u->m_raster, u->m_dict, Word(u, 1811));
        }
        else if (app->GetP4EC58()->m_pending == 5)
        {
            VIUI *u = UI();
            w = u->m_font.GetStringWidth(u->m_raster, u->m_dict, Word(u, 1812));
        }
        else
            w = UI()->m_font.GetStringWidth(UI()->m_raster, UI()->m_dict, msg);
        box.Init(UI(), VIVect2I(0, 0), m_window.Center(), w + 120, 3, 0);
        box.SetTitle(box.T((unsigned short)271));
        box.Open(true);
        box.Draw();
        box.Text(msg, 40, 1, (VIUICOLOR)0, 0);
    }
    ui->End2D();

    // The held arrows, through the key repeat.
    if (m_keys.Pulse(138))
    {
        if (m_column == 1 && m_available.GetSel() > 0)
        {
            app->PlaySound(11);
            m_available.SetSel(m_available.GetSel() - 1);
        }
        else if (m_column == 0 && m_owned.GetSel() > 0)
        {
            app->PlaySound(11);
            m_owned.SetSel(m_owned.GetSel() - 1);
        }
        else
            app->PlaySound(9);
    }
    if (m_keys.Pulse(139))
    {
        if (m_column == 1 && m_available.Count() != 1)
        {
            app->PlaySound(11);
            m_available.SetSel(m_available.GetSel() + 1);
        }
        else if (m_column == 0 && m_owned.Count() != 1)
        {
            app->PlaySound(11);
            m_owned.SetSel(m_owned.GetSel() + 1);
        }
        else
            app->PlaySound(9);
    }
}

// 0x00C34340.  Left/Right turn the category, Up/Down repeat through the key flash,
// Square the points, Triangle the other list, Enter opens the skill.
void VIWndClassMastery::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_busy)
        return;
    switch (key)
    {
    case 17:
    case 136:
        app->PlaySound(11);
        SelectCategory((m_category + 1) % 5);
        break;
    case 18:
    case 137:
        app->PlaySound(11);
        SelectCategory((m_category + 4) % 5);
        break;
    case 19:
    case 138:
        m_keys.Press(138);
        break;
    case 20:
    case 139:
        m_keys.Press(139);
        break;
    case 9:
    case 142:
        app->PlaySound(11);
        SetColumn(1 - m_column);
        break;
    case 81:
    case 113:
    case 133:
        m_points->SetVisible(true);
        app->PlaySound(8);
        app->SetActiveWindow(m_points);
        break;
    case 13:
    case 134:
        if (m_column == 0)
        {
            if (m_owned.GetSel() == -1)
                break;
            VICMSkill *&s = sSkills[0][m_owned.GetSel()];
            app->PlaySound(15);
            app->GetP4EC58()->Browse(s->m_id);
        }
        else
        {
            if (m_available.GetSel() == -1)
                break;
            VICMSkill *&s = sSkills[1][m_available.GetSel()];
            app->PlaySound(15);
            app->GetP4EC58()->Browse(s->m_id);
        }
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel6C0();
        break;
    }
}

// 0x00C34548.
void VIWndClassMastery::OnKeyUp(int key)
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

// 0x00C345C0.  295: the focus arrived.
void VIWndClassMastery::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg != 295)
        return;
    if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_points && (VIWnd *)p1 != m_inspect
        && (VIWnd *)p1 != m_require)
    {
        m_points->SetVisible(false);
        m_inspect->SetVisible(false);
        m_require->SetVisible(false);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 480, 14, 0);
        m_window.SetTitle(m_window.T("Class Mastery"));
        m_window.Open(true);
        m_owned.Init(UI(), 400, 7);
        m_available.Init(UI(), 400, 7);
        SetColumn(0);
        m_keys.Reset();
        SelectCategory(0);
        m_dots = 0;
        m_ticks = 15;
    }
    if ((VIWnd *)p1 == this)
        m_keys.Reset();
}

// 0x00C34710.
void VIWndClassMastery::SetPos(const VIVect2I &p)
{
    m_pos = p;
}

// 0x00C34728.
void VIWndClassMastery::Refresh()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_busy = 0;
    SelectCategory(0);
}

// 0x00C34760.  The category's skills: owned ones into the first list, known ones and then
// the rest into the second, each list chained by rank.
void VIWndClassMastery::SelectCategory(int category)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_category = category;
    memset(sSkills, 0, sizeof(sSkills));
    memset(sCount, 0, sizeof(sCount));
    VIPool<VICMSkill> &skills = app->GetP4EC58()->m_skills;
    VIList<VICMSkill *> lists[2];
    int i;
    for (i = skills.Begin(); i != skills.End(); i = skills.Next(i))
    {
        VICMSkill &s = skills.Value(i);
        if (s.m_x04)
        {
            if (s.m_x10 == m_category)
                lists[0].Add(&s);
        }
        else if (app->GetP4EC58()->IsKnown(s.m_id))
        {
            if (skills.Value(i).m_x10 == m_category)
                lists[1].Add(&s);
        }
    }
    for (i = skills.Begin(); i != skills.End(); i = skills.Next(i))
    {
        VICMSkill &s = skills.Value(i);
        if (!s.m_x04 && !app->GetP4EC58()->IsKnown(s.m_id) && skills.Value(i).m_x10 == m_category)
            lists[1].Add(&s);
    }
    VIList<VICMSkill *> chain;
    while (Chain(lists[0], chain))
    {
        sSkills[0][sCount[0]++] = chain.Value(chain.Prev(chain.End()));
        chain.Clear();
    }
    while (Chain(lists[1], chain))
    {
        sSkills[1][sCount[1]++] = chain.Value(chain.Begin());
        chain.Clear();
    }
    chain.Clear();
    int cols[2] = { 0, 330 };
    m_owned.Clear();
    for (i = 0; i < sCount[0]; i++)
        m_owned.AddLine(sSkills[0][i]->m_name);
    m_available.Clear();
    unsigned short buf[256];
    for (i = 0; i < sCount[1]; i++)
    {
        if (app->GetP4EC58()->IsKnown(sSkills[1][i]->m_id))
        {
            VIString::snprintf16(buf, sizeof(buf), "%S", sSkills[1][i]->m_name);
            int line = m_available.AddLine(buf);
            m_available.SetColumns(line, 2, cols);
            VIString::snprintf16(buf, sizeof(buf), "%d", sSkills[1][i]->m_x14);
            m_available.SetText(line, 1, buf);
        }
        else
        {
            VIString::snprintf16(buf, sizeof(buf), "???");
            int line = m_available.AddLine(buf);
            m_available.SetColumns(line, 2, cols);
            VIString::snprintf16(buf, sizeof(buf), "Locked");
            m_available.SetText(line, 1, buf);
        }
    }
    m_owned.SetSel(0);
    m_available.SetSel(0);
}

// 0x00C34C70.
void VIWndClassMastery::DrawLists()
{
    if (m_column == 0)
    {
        m_owned.Draw(&m_window, 40, 5);
        m_window.Text(m_window.T("Owned (|8)"), 40, 4, (VIUICOLOR)(m_column == 0 ? 0 : 2), 0);
    }
    else
    {
        m_available.Draw(&m_window, 40, 5);
        m_window.Text(m_window.T("Available (|8)"), 40, 4, (VIUICOLOR)(m_column == 1 ? 0 : 2), 0);
        m_window.Text(m_window.T("Cost"), 370, 4, (VIUICOLOR)0, 0);
    }
}

// 0x00C34D70.  The selection shows in the column that has the focus.
void VIWndClassMastery::SetColumn(int column)
{
    m_column = column;
    m_owned.SetShowSel(1 - column);
    m_available.SetShowSel(column);
}

// 0x00C34DC0.  Moves one rank chain from `in` to `out`: the skill with no parent in the
// list, then each child in turn (a child's m_x0C is its parent's m_x00).  0 when empty.
int VIWndClassMastery::Chain(VIList<VICMSkill *> &in, VIList<VICMSkill *> &out)
{
    if (!in.Size())
        return 0;
    int root = -1;
    int i;
    for (i = in.Begin(); i != in.End(); i = in.Next(i))
    {
        VICMSkill *a = in.Value(i);
        int parent = 0;
        int j;
        for (j = in.Begin(); j != in.End(); j = in.Next(j))
        {
            if (a->m_x0C == in.Value(j)->m_x00)
            {
                parent = 1;
                break;
            }
        }
        if (!parent)
        {
            root = i;
            break;
        }
    }
    if (root == -1)
        return 0;
    int id = in.Value(root)->m_x00;
    out.Add(in.Value(root));
    in.Erase(root);
    int found;
    do
    {
        found = 0;
        for (i = in.Begin(); i != in.End(); i = in.Next(i))
        {
            if (in.Value(i)->m_x0C == id)
            {
                id = in.Value(i)->m_x00;
                out.Add(in.Value(i));
                found = 1;
                in.Erase(i);
                break;
            }
        }
    } while (found);
    return 1;
}

// 0x00C34F48.  The client opened a skill: the inspector shows an owned or known one, the
// requirements one that is not known yet.
int VIWndClassMastery::ShowNode(VICMNode *node)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (!m_visible && app->m_p2BD40 != this)
        return -1;
    if (m_column == 0)
    {
        if (m_owned.GetSel() == -1)
            return 0;
        m_inspect->Show(node);
        m_inspect->SetVisible(true);
        app->SetActiveWindow(m_inspect);
        return 0;
    }
    if (m_available.GetSel() == -1)
        return 0;
    if (app->GetP4EC58()->IsKnown(node->m_x08))
    {
        m_inspect->Show(node);
        m_inspect->SetVisible(true);
        app->SetActiveWindow(m_inspect);
    }
    else
    {
        m_require->Show(node);
        m_require->SetVisible(true);
        app->SetActiveWindow(m_require);
    }
    return 0;
}
