// viguibutton.cpp (name ours) -- STATION 0x002BD5C0 .. 0x002BE3E8: a text that,
// when pressed, loads another page or runs a renderer action.  Then the
// item-header template copies and __tf11VIGUIButton.
#include "station.h"

// 0x002BD5C0.
VIGUIButton::VIGUIButton(const char *name, VIWndGenericRenderer *r, VIXMLParser *reader)
    : VIGUIText(name, 4, r, reader)
{
    m_nextPage = 0;
    m_action = 0;
}

// 0x002BD608.
VIGUIButton::~VIGUIButton()
{
    if (m_nextPage)
    {
        free(m_nextPage);
        m_nextPage = 0;
    }
    if (m_action)
    {
        free(m_action);
        m_action = 0;
    }
}

// 0x002BD688.
void VIGUIButton::SetNextPage(const char *s)
{
    if (m_nextPage)
        free(m_nextPage);
    if (s)
        m_nextPage = strdup(s);
    else
        m_nextPage = 0;
}

// 0x002BD6D8.
void VIGUIButton::SetAction(const char *s)
{
    if (m_action)
        free(m_action);
    if (s)
        m_action = strdup(s);
    else
        m_action = 0;
}

// 0x002BD728.  The pad's accept: the next page first, then the action.
int VIGUIButton::OnKeyDown(int key)
{
    if (!m_renderer)
        return 0;
    void *app = m_renderer->GetAppPtr();
    if (!app)
        return 0;
    VIUI *ui = ST_UI(app);
    if (key == 134)
    {
        if (m_nextPage)
        {
            ui->SoundPlay((VIUISoundId)13);
            m_renderer->LoadPage(m_nextPage);
        }
        if (m_action)
        {
            ui->SoundPlay((VIUISoundId)13);
            m_renderer->OnAction(m_action);
        }
    }
    return 0;
}

// 0x002BD7D0.
int VIGUIButton::OnKeyUp(int key)
{
    return 0;
}
