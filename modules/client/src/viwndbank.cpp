#line 1 "C:/code-frontiers-gm/game/VIWndBank.cpp"
// viwndbank.cpp (name ours) -- CLIENT 0x00C152A8 .. 0x00C16438: the bank window, the
// parent of the bank's four pages.  Method names are ours.
#include "game.h"

// 0x00C152A8.
VIWndBank::VIWndBank()
{
    m_menu = 0;
    m_item = 0;
    m_money = 0;
    m_auction = 0;
}

// 0x00C152F0.
VIWndBank::~VIWndBank()
{
    Free();
}

// 0x00C15338.
// @@BEGIN Init
int VIWndBank::Init()
{
    m_menu = new VIWndBankMenu;
    if (!m_menu)
        return -1;
    m_menu->SetAppPtr(GetAppPtr());
    m_menu->SetVisible(false);
    AddChild(m_menu);
    if (m_menu->Init())
        return -1;
    m_money = new VIWndBankMoney;
    if (!m_money)
        return -1;
    m_money->SetAppPtr(GetAppPtr());
    m_money->SetVisible(false);
    AddChild(m_money);
    if (m_money->Init())
        return -1;
    m_item = new VIWndBankItem;
    if (!m_item)
        return -1;
    m_item->SetAppPtr(GetAppPtr());
    m_item->SetVisible(false);
    AddChild(m_item);
    if (m_item->Init())
        return -1;
    m_auction = new VIWndAuction;
    if (!m_auction)
        return -1;
    m_auction->SetAppPtr(GetAppPtr());
    m_auction->SetVisible(false);
    AddChild(m_auction);
    return m_auction->Init() ? -1 : 0;
}
// @@END Init

// 0x00C154A8.
int VIWndBank::Free()
{
    if (m_menu)
    {
        RemoveChild(m_menu);
        delete m_menu;
        m_menu = 0;
    }
    if (m_item)
    {
        RemoveChild(m_item);
        delete m_item;
        m_item = 0;
    }
    if (m_money)
    {
        RemoveChild(m_money);
        delete m_money;
        m_money = 0;
    }
    if (m_auction)
    {
        RemoveChild(m_auction);
        delete m_auction;
        m_auction = 0;
    }
    return 0;
}

// 0x00C155B0.
void VIWndBank::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:
        if (w == m_item || w == m_money || w == m_auction)
        {
            m_parent->OnMessage(this, 288, 0, 0);
            ShowMenu(0);
        }
        break;
    case 289:
        m_parent->OnMessage(this, 288, 1, 0);
        m_auction->SetVisible(false);
        m_menu->SetVisible(false);
        m_money->SetVisible(false);
        m_item->SetVisible(true);
        m_item->Show();
        break;
    case 290:
        m_auction->SetVisible(false);
        m_menu->SetVisible(false);
        m_item->SetVisible(false);
        m_money->SetVisible(true);
        m_money->Reset();
        break;
    case 310:
        m_parent->OnMessage(this, 288, 1, 0);
        m_menu->SetVisible(false);
        m_item->SetVisible(false);
        m_money->SetVisible(false);
        m_auction->SetVisible(true);
        app->SetActiveWindow(m_auction);
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C157B0.  Back to the menu page.
void VIWndBank::ShowMenu(int unused)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_auction->SetVisible(false);
    m_item->SetVisible(false);
    m_money->SetVisible(false);
    m_menu->SetVisible(true);
    m_menu->ResetCursor();
    app->SetActiveWindow(m_menu);
}
