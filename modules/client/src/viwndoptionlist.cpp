#line 1 "C:/code-frontiers-gm/game/VIWndOptionList.cpp"
// VIWndOptionList.cpp -- CLIENT 0x00C7FFC8 .. 0x00C80620 (STATION 0x002F25E0 .. 0x002F2C38).  A popup
// list of choices under a title, the chosen one ticked; the customer-service
// reader (VIWndReadCS) news one.  Closing it sends the parent 308 with the
// selection and whether it was accepted.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x002F25E0.
VIWndOptionList::VIWndOptionList()
{
}

// 0x002F2628.
VIWndOptionList::~VIWndOptionList()
{
}

// 0x002F2678.
int VIWndOptionList::Init()
{
    m_sel = 0;
    return 0;
}

// 0x002F2688.
void VIWndOptionList::Free()
{
}

// 0x002F2690.  The choices, the tick on the chosen row, and the button legend.
void VIWndOptionList::OnDrawSelf()
{
    VIUI *ui = UI();
    unsigned short s[128];
    if (GetFocus() != this)
        m_window.ResetTickPulse();
    ui->Begin2D();
    m_window.Draw();
    for (int i = 0; i < m_count; i++)
        m_window.Text(m_options[i], 40, i + 1, (VIUICOLOR)(m_sel == i), 0);
    m_window.TickR(36, m_sel + 1, true);
    VIString::snprintf16(s, sizeof(s), "|x%S   |t%S", Word(ui, 212), Word(ui, 213));
    m_window.Text(s, 40, m_rows - 1, VIUICOLOR_0, 0);
    ui->End2D();
}

// 0x002F27E0.  Up/down move the tick (a buzz at either end), Enter accepts,
// Triangle cancels.
void VIWndOptionList::OnKeyDown(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        if (m_sel > 0)
        {
            m_sel--;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
        break;
    case 20:
    case 139:
        if (m_sel < m_count - 1)
        {
            m_sel++;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
        break;
    case 13:
    case 134:
        SetVisible(false);
        GetParent()->OnMessage(this, 308, m_sel, 1);
        UI()->SoundPlay((VIUISoundId)13);
        break;
    case 27:
    case 132:
        SetVisible(false);
        GetParent()->OnMessage(this, 308, m_sel, 0);
        UI()->SoundPlay((VIUISoundId)8);
        break;
    }
}

// 0x002F2970.
void VIWndOptionList::OnKeyUp(int key)
{
}

// 0x002F2978.  Gaining the focus opens the list on the first choice.
void VIWndOptionList::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_window.Close(true);
            m_window.Init(UI(), VIVect2I(0, 0), m_center, m_width, m_rows, 0);
            m_window.Open(false);
            m_window.SetTitle(m_window.T(m_title));
            m_sel = 0;
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x002F2A20.
void VIWndOptionList::Setup(const unsigned short *title, const unsigned short **options,
                            int count, const VIVect2I &center, int width)
{
    VIString::strcpy16(m_title, sizeof(m_title), title);
    m_options = options;
    m_count = count;
    m_center = center;
    m_width = width < 300 ? 300 : width;
    m_width = VIMAX(m_width, UI()->m_font.GetStringWidth(UI()->m_raster, UI()->m_dict, title) + 80);
    for (int i = 0; i < m_count; i++)
        m_width = VIMAX(m_width, UI()->m_font.GetStringWidth(UI()->m_raster, UI()->m_dict,
                                                             m_options[i]) + 80);
    m_rows = count + 3;
    m_window.Init(UI(), VIVect2I(0, 0), m_center, m_width, m_rows, 0);
    m_window.SetTitle(title);
    m_sel = 0;
}
