#line 1 "C:/code-frontiers-gm/game/VIWndMenu.cpp"
// VIWndMenu (name the RTTI's) -- CLIENT 0x00C71998 .. 0x00C72D18.  A VIWndList over a
// static VIMenuDef: each entry becomes a row, Enter sends the entry's message (or opens
// its submenu in a VIWndMiniMenu beside the row), Escape backs out.  Method names ours.
#include "game.h"

// 0x00C71998.
VIWndMenu::VIWndMenu()
{
    m_menu = 0;
    m_sub = 0;
}

// 0x00C719D8.  Rebuilds the rows from the menu every frame, then draws the list.
void VIWndMenu::OnDrawSelf()
{
    unsigned int flags;
    int icon2;
    int icon;
    int x0C;
    GetAppPtr();
    if (Init(m_menu->m_count))
        return;
    for (int i = 0; i < m_menu->m_count; i++)
    {
        flags = 0;
        if (m_menu->m_entries[i].m_word != -1)
        {
            flags = 2;
            icon = -1;
        }
        if (m_menu->m_entries[i].m_icon != -1)
        {
            icon = m_menu->m_entries[i].m_icon;
            flags |= 1;
        }
        if (m_menu->m_entries[i].m_icon2 != -1)
        {
            if (m_menu->m_entries[i].m_msg == 0x08050000)
            {
                icon2 = m_menu->m_entries[i].m_icon2;
                flags |= 0x10;
            }
            else
            {
                icon2 = m_menu->m_entries[i].m_icon2;
                flags |= 8;
            }
        }
        if (m_menu->m_entries[i].m_number >= 2)
        {
            icon = m_menu->m_entries[i].m_icon;
            flags |= 4;
        }
        if (m_menu->m_entries[i].m_x08 != -1)
        {
            x0C = m_menu->m_entries[i].m_x08;
            flags |= 0x20;
        }
        SetItem(i, flags, icon, icon2, m_menu->m_entries[i].m_word, m_menu->m_entries[i].m_number, x0C);
    }
    VIWndList::OnDrawSelf();
}

// 0x00C71B20.
void VIWndMenu::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
    {
        app->PlaySound(13);
        if (m_menu->m_entries[m_sel].m_msg)
            GetParent()->OnMessage(this, 261, m_menu->m_entries[m_sel].m_msg,
                                   m_menu->m_entries[m_sel].m_id);
        if (m_x60 == 1)
        {
            GetParent()->OnMessage(this, 277, m_menu->m_entries[m_selRow].m_id,
                                   m_menu->m_entries[m_sel].m_id);
            break;
        }
        if (m_menu->m_entries[m_sel].m_sub == 0)
            break;
        if (m_sub == 0)
        {
            m_sub = new VIWndMiniMenu;
            m_sub->SetAppPtr(GetAppPtr());
            AddChild(m_sub);
            if (m_sub == 0)
                break;
        }
        if (m_style == 1)
            m_sub->SetPos(m_pos + VIVect2I(155, (m_sel - m_top) * 44));
        else if (m_style == 0)
            m_sub->SetPos(m_pos + VIVect2I(139, (m_sel - m_top) * 44 + 10));
        m_sub->SetMenu(m_menu->m_entries[m_sel].m_sub);
        m_sub->ResetScroll();
        m_sub->m_visible = 1;
        app->SetActiveWindow(m_sub);
        break;
    }
    case 27:
    case 132:
        app->PlaySound(8);
        if (m_x60)
        {
            SetX60(0);
            Select(-1);
        }
        else
            GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 19:
    case 20:
    case 138:
    case 139:
        VIWndList::OnKeyDown(key);
        break;
    default:
        VIWnd::OnKeyDown(key);
        break;
    }
}

// 0x00C71E60.
void VIWndMenu::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 20:
    case 138:
    case 139:
        VIWndList::OnKeyUp(key);
        break;
    default:
        VIWnd::OnKeyUp(key);
        break;
    }
}

// 0x00C71EB0.  The submenu closing (260, 261 forwarded), and the focus moving onto or
// off this menu (295: p1 the old focus, p2 the new).
void VIWndMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:
        if (w == m_sub)
        {
            app->SetActiveWindow(this);
            m_sub->m_visible = 0;
        }
        break;
    case 261:
        if (w == m_sub)
        {
            app->SetActiveWindow(this);
            m_sub->m_visible = 0;
        }
        GetParent()->OnMessage(this, 261, p1, p2);
        break;
    case 295:
        if (p2 == (int)this)
        {
            m_x84 = 1;
            m_alpha = 1.0f;
            m_x80 = 1;
            m_x7C = 1.0f;
            m_time = app->m_p07568;
            m_flash.Reset();
        }
        if (p1 == (int)this && p2 != p1)
        {
            m_x84 = 0;
            m_flash.Reset();
        }
        break;
    default:
        GetParent()->OnMessage(this, msg, p1, p2);
        break;
    }
}

// 0x00C72028.
void VIWndMenu::SetMenu(VIMenuDef *menu)
{
    m_menu = menu;
    if (m_sub)
        m_sub->m_visible = 0;
}

// 0x00C72040.
void VIWndMenu::Reset()
{
    m_visible = 1;
    m_sel = 0;
    m_top = 0;
    SetX64(1);
    if (m_sub)
        m_sub->m_visible = 0;
}

// 0x00C72098.
int VIWndMenu::GetId(int i)
{
    if (!m_menu)
        return -1;
    if (i >= m_menu->m_count)
        return -1;
    return m_menu->m_entries[i].m_id;
}
