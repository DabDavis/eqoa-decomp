#line 1 "C:/code-frontiers-gm/game/VIWndBankMoney.cpp"
// viwndbankmoney.cpp (name ours) -- CLIENT 0x00C194C0 .. 0x00C1A510: the bank's money
// page, a VIWndMoneyMenu over a VIWndDeposit.  Method names are ours.
#include "game.h"

// 0x00C194C0.
VIWndBankMoney::VIWndBankMoney()
{
    m_menu = 0;
    m_deposit = 0;
}

// 0x00C19500.
VIWndBankMoney::~VIWndBankMoney()
{
    Free();
}

// 0x00C19548.
int VIWndBankMoney::Init()
{
    m_menu = new VIWndMoneyMenu;
    if (!m_menu)
        return -1;
    m_menu->SetAppPtr(GetAppPtr());
    m_menu->SetVisible(false);
    AddChild(m_menu);
    if (m_menu->Init())
        return -1;
    m_deposit = new VIWndDeposit;
    if (!m_deposit)
        return -1;
    m_deposit->SetAppPtr(GetAppPtr());
    m_deposit->SetVisible(false);
    AddChild(m_deposit);
    return m_deposit->Init() ? -1 : 0;
}

// 0x00C19618.
int VIWndBankMoney::Free()
{
    if (m_menu)
    {
        RemoveChild(m_menu);
        delete m_menu;
        m_menu = 0;
    }
    if (m_deposit)
    {
        RemoveChild(m_deposit);
        delete m_deposit;
        m_deposit = 0;
    }
    return 0;
}

// 0x00C196B0.  291 deposit, 292 withdraw (the money menu), 260 back.
void VIWndBankMoney::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:
        if (w == m_menu)
            m_parent->OnMessage(this, 260, 0, 0);
        else
            Reset();
        break;
    case 291:
        m_menu->ResetSel();
        m_menu->SetVisible(false);
        m_deposit->SetCarried(app->m_player.m_p34);
        m_deposit->SetBanked(app->GetPlayerP38());
        m_deposit->ClearDigits();
        m_deposit->SetCursorLast();
        m_deposit->Setup(1);
        m_deposit->SetVisible(true);
        app->SetActiveWindow(m_deposit);
        break;
    case 292:
        m_menu->ResetSel();
        m_menu->SetVisible(false);
        m_deposit->SetCarried(app->m_player.m_p34);
        m_deposit->SetBanked(app->GetPlayerP38());
        m_deposit->ClearDigits();
        m_deposit->SetCursorLast();
        m_deposit->Setup(2);
        m_deposit->SetVisible(true);
        app->SetActiveWindow(m_deposit);
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C19860.  The deposit form put away, the menu again.
void VIWndBankMoney::Reset()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_deposit->SetCursorLast();
    m_deposit->ClearDigits();
    m_deposit->SetVisible(false);
    m_deposit->SetCarried(0);
    m_deposit->SetBanked(0);
    m_deposit->Setup(0);
    m_menu->SetVisible(true);
    m_menu->ResetSel();
    app->SetActiveWindow(m_menu);
}
