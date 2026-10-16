// viguipage.cpp (name ours) -- STATION 0x002C2610 .. 0x002C45F8.  A page of
// GUI objects: the selection grid the pad moves over, the objects in depth
// order, and a pulsing selector drawn over the selected one.
#include "station.h"
#include "vilist_t.h"

// VIStation embeds its VIUI at +0xAFE0 (lane core owns VIStation's layout).
static inline VIUI *StationUI(void *app)
{
    return (VIUI *)((char *)app + 0xAFE0);
}

// ---------------------------------------------------------------------------
// VIGUISelectionGrid

// 0x002C2610.
VIGUISelectionGrid::VIGUISelectionGrid()
{
    m_cells = 0;
    m_col = 0;
    m_row = 0;
}

// 0x002C2630.
void VIGUISelectionGrid::Init(int cols, int rows)
{
    m_rows = rows;
    m_cols = cols;
    m_cells = new VIGUIObject **[cols];
    for (int i = 0; i < m_cols; i++)
    {
        m_cells[i] = new VIGUIObject *[m_rows];
        memset(m_cells[i], 0, m_rows * sizeof(VIGUIObject *));
    }
}

// 0x002C26D8.
VIGUISelectionGrid::~VIGUISelectionGrid()
{
    if (m_cells)
    {
        for (int i = 0; i < m_cols; i++)
        {
            delete [] m_cells[i];
            m_cells[i] = 0;
        }
        delete [] m_cells;
    }
    m_cols = -1;
    m_rows = -1;
    m_col = 0;
    m_row = 0;
    m_cells = 0;
}

// 0x002C27B8.
void VIGUISelectionGrid::Set(VIGUIObject *o, int col, int row)
{
    if (col >= 0 && col < m_cols && row >= 0 && row < m_rows && m_cells)
    {
        m_cells[col][row] = o;
        if (col == m_col && row == m_row)
            o->SetSelected(1);
    }
}

// 0x002C2840.
void VIGUISelectionGrid::Select(const char *name)
{
    if (!name)
        return;
    for (int c = 0; c < m_cols; c++)
    {
        for (int r = 0; r < m_rows; r++)
        {
            VIGUIObject *o = m_cells[c][r];
            if (o)
            {
                const char *n = o->GetName();
                if (n && !strcmp(name, n))
                {
                    VIGUIObject *cur = GetSelected();
                    m_col = c;
                    m_row = r;
                    if (cur != o && cur)
                        cur->SetSelected(0);
                    o->SetSelected(1);
                    return;
                }
            }
        }
    }
}

// 0x002C2950.
VIGUIObject *VIGUISelectionGrid::GetSelected()
{
    return m_cells ? m_cells[m_col][m_row] : 0;
}

// 0x002C2988.  Down searches the columns to the left of an empty cell (the
// LAST hit wins: the leftmost); left steps again while the next cell holds the
// same object; right does neither.
int VIGUISelectionGrid::Move(int key)
{
    int result = 2;
    if (!m_cells)
        return 2;
    VIGUIObject *old = GetSelected();
    if (key == 139)
    {
        result = 1;
        if (m_row + 1 < m_rows)
        {
            if (m_cells[m_col][m_row + 1])
                m_row++;
            else
            {
                int row = m_row + 1;
                for (int c = m_col - 1; c >= 0; c--)
                {
                    if (m_cells[c][row])
                    {
                        m_row = row;
                        m_col = c;
                    }
                }
            }
        }
    }
    else if (key == 138)
    {
        result = 1;
        if (m_row - 1 >= 0)
        {
            if (m_cells[m_col][m_row - 1])
                m_row--;
        }
    }
    else if (key == 136)
    {
        result = 1;
        if (m_col - 1 >= 0)
        {
            if (m_cells[m_col - 1][m_row])
            {
                m_col--;
                if (m_cells[m_col][m_row] == m_cells[m_col + 1][m_row])
                    Move(136);
            }
        }
    }
    else if (key == 137)
    {
        result = 1;
        if (m_col + 1 < m_cols)
        {
            if (m_cells[m_col + 1][m_row])
                m_col++;
        }
    }
    VIGUIObject *cur = GetSelected();
    if (old != cur)
    {
        result = 0;
        if (old)
            old->SetSelected(0);
        if (cur)
            cur->SetSelected(1);
    }
    return result;
}

// ---------------------------------------------------------------------------
// VIGUIPage

// 0x002C2BB8.
VIGUIPage::VIGUIPage()
{
    Reset();
}

// 0x002C2C00.
VIGUIPage::VIGUIPage(const char *name)
{
    Reset();
    SetName(name);
}

// 0x002C2C60.
VIGUIPage::~VIGUIPage()
{
    printf("Page destructor for %s\n", m_name);
    int i = m_objects.Begin();
    while (i != m_objects.End())
    {
        VIGUIObject *o = *(VIGUIObject **)i;
        int next = m_objects.Erase(i);
        if (o)
            delete o;
        i = next;
    }
    if (m_selector)
    {
        delete m_selector;
        m_selector = 0;
    }
    Reset();
}

// 0x002C2D80.
void VIGUIPage::Init(VIWndGenericRenderer *owner, int cols, int rows)
{
    m_owner = owner;
    m_grid.Init(cols, rows);
}

// 0x002C2DA8.
void VIGUIPage::Reset()
{
    memset(m_name, 0, sizeof(m_name));
    m_owner = 0;
    m_selector = 0;
    m_selectorX = -1;
    m_selectorY = -1;
    m_pulse = 1.0f;
    m_pulseStep = 0.03f;
}

// 0x002C2E08.
const char *VIGUIPage::GetName()
{
    return m_name;
}

// 0x002C2E10.
void VIGUIPage::Select(const char *name)
{
    m_grid.Select(name);
}

// 0x002C2E30.
void VIGUIPage::SetSelector(VIGUIObject *o)
{
    m_selector = o;
    o->SetSelectEffect(3);
    m_selector->SetAlphaStep(0.05f);
}

// 0x002C2E78.
void VIGUIPage::SetSelectorX(short x)
{
    m_selectorX = x;
}

// 0x002C2E80.
void VIGUIPage::SetSelectorY(short y)
{
    m_selectorY = y;
}

// 0x002C2E88.
void VIGUIPage::SetName(const char *name)
{
    strncpy(m_name, name, 63);
}

// 0x002C2EA8.  Before the first object drawn over it.
void VIGUIPage::AddObject(VIGUIObject *o)
{
    if (!o)
        return;
    m_objects.Size();
    int depth = o->GetDepth();
    int i;
    for (i = m_objects.Begin(); i != m_objects.End(); i = m_objects.Next(i))
    {
        VIGUIObject *p = *(VIGUIObject **)i;
        if (p && depth < p->GetDepth())
            break;
    }
    m_objects.Insert(o, i);
}

// 0x002C2F58.
void VIGUIPage::AddObject(VIGUIObject *o, int col, int row)
{
    AddObject(o);
    m_grid.Set(o, col, row);
}

// 0x002C2FB8.
VIGUIObject *VIGUIPage::FindObject(const char *name)
{
    for (int i = m_objects.Begin(); i != m_objects.End(); i = m_objects.Next(i))
    {
        VIGUIObject *o = *(VIGUIObject **)i;
        if (o)
        {
            const char *n = o->GetName();
            if (n && !strcmp(name, n))
                return o;
        }
    }
    return 0;
}

// 0x002C3058.
const char *VIGUIPage::GetText(const char *name)
{
    VIGUIObject *o = FindObject(name);
    if (o)
    {
        int t = o->GetType();
        if (t == 1 || t == 5)
        {
            VIGUIStringDataModel *m = ((VIGUIText *)o)->GetModel();
            if (m)
                return m->GetString();
        }
    }
    return 0;
}

// 0x002C30C8.
int VIGUIPage::GetToggle(const char *name)
{
    VIGUIObject *o = FindObject(name);
    if (o)
    {
        if (o->GetType() == 2)
        {
            VIGUIToggleDataModel *m = ((VIGUIToggleButton *)o)->GetModel();
            if (m)
                return m->GetValue();
        }
    }
    return 0;
}

// 0x002C3130.
void VIGUIPage::SetToggle(const char *name, int on)
{
    VIGUIObject *o = FindObject(name);
    if (o)
    {
        if (o->GetType() == 2)
        {
            VIGUIToggleDataModel *m = ((VIGUIToggleButton *)o)->GetModel();
            if (m)
                m->SetValue(on);
        }
    }
}

// 0x002C3198.
void VIGUIPage::ClearOptions(const char *name)
{
    VIGUIObject *o = FindObject(name);
    if (o)
    {
        if (o->GetType() == 5)
            ((VIGUIComboBox *)o)->ClearItems();
    }
}

// 0x002C31E8.
void VIGUIPage::AddOption(const char *name, const char *s)
{
    VIGUIObject *o = FindObject(name);
    if (o)
    {
        if (o->GetType() == 5)
            ((VIGUIComboBox *)o)->AddItem(s);
    }
}

// 0x002C3240.
void VIGUIPage::SetText(const char *name, const char *s)
{
    VIGUIObject *o = FindObject(name);
    if (o)
    {
        int t = o->GetType();
        if (t == 1 || t == 5)
        {
            VIGUIStringDataModel *m = ((VIGUIText *)o)->GetModel();
            if (m)
                m->SetString(s);
        }
    }
}

// 0x002C32B0.
void VIGUIPage::SetTextColor(const char *name, VIColor32F c)
{
    VIGUIObject *o = FindObject(name);
    if (o)
    {
        int t = o->GetType();
        if (t == 1 || t == 5)
            ((VIGUIText *)o)->SetTextColor(c);
    }
}

// 0x002C3328.
void VIGUIPage::Draw()
{
    if (!m_owner)
        return;
    void *app = m_owner->GetAppPtr();
    if (!app)
        return;
    VIUI *ui = StationUI(app);
    ui->Begin2D();
    VIGUIObject *sel = m_grid.GetSelected();
    for (int i = m_objects.Begin(); i != m_objects.End(); i = m_objects.Next(i))
    {
        VIGUIObject *o = *(VIGUIObject **)i;
        if (o)
        {
            if (o == sel || o->GetButtonX() < 0)
                o->Draw(1.0f);
            else
                o->Draw(1.0f);
        }
    }
    if (m_selector && sel)
    {
        short x = sel->GetX();
        short y = sel->GetY();
        m_selector->SetPos(m_selectorX == -1 ? x : m_selectorX,
                           m_selectorY == -1 ? y : m_selectorY);
        m_selector->Draw(m_pulse);
    }
    ui->End2D();
    m_pulse += m_pulseStep;
    if (m_selector)
        m_selector->SetPulse(m_pulse);
    if (m_pulse > 1.0f)
    {
        m_pulse = 1.0f;
        m_pulseStep = -m_pulseStep;
    }
    else if (m_pulse < 0.3f)
    {
        m_pulse = 0.3f;
        m_pulseStep = -m_pulseStep;
    }
}

// 0x002C3518.
void VIGUIPage::OnKeyUp(int key)
{
    VIGUIObject *o = m_grid.GetSelected();
    if (o)
        o->OnKeyUp(key);
}

// 0x002C3560.  What the selection does not take: X goes back, the arrows
// move the selection.
void VIGUIPage::OnKeyDown(int key)
{
    if (!m_owner)
        return;
    void *app = m_owner->GetAppPtr();
    if (!app)
        return;
    VIUI *ui = StationUI(app);
    int handled = 0;
    VIGUIObject *o = m_grid.GetSelected();
    if (o)
        handled = o->OnKeyDown(key);
    if (handled)
        return;
    if (key == 132)
    {
        ui->SoundPlay((VIUISoundId)8);
        m_owner->PageBack();
        return;
    }
    int r = m_grid.Move(key);
    if (r == 0)
        ui->SoundPlay((VIUISoundId)11);
    else if (r == 1)
        ui->SoundPlay((VIUISoundId)9);
}

// 0x002C3668.
void VIGUIPage::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIGUIObject *o = m_grid.GetSelected();
    if (o)
        o->OnMessage(w, msg, p1, p2);
}

// ---------------------------------------------------------------------------
// VIList<VIGUIObject *> (0x002C42A0 .. 0x002C4578): this TU's copy, the one the
// linker resolved every call to.  vilist_t.h has no Insert; this is the body
// the image carries: a node before `i`, or Add at End.
// (VIList<T>::Insert is in src/vilist_t.h.)


















