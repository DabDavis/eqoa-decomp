#line 1 "C:/code-frontiers-gm/game/VIWndStartMenu.cpp"
// VIWndStartMenu -- CLIENT 0x00C97DA8 .. 0x00C98588 (the TU runs to its __tf at
// 0x00C99160).  The start menu: a main menu and an item menu, framed.
#include "game.h"

// 0x00C97DA8.
VIWndStartMenu::VIWndStartMenu()
{
    m_main = 0;
    m_items = 0;
}

// 0x00C97DE8.
VIWndStartMenu::~VIWndStartMenu()
{
    Free();
}

// 0x00C97E30.
int VIWndStartMenu::Init()
{
    m_main = new VIWndMainMenu;
    if (!m_main)
        goto fail;
    m_main->SetVisible(false);
    m_main->SetAppPtr(GetAppPtr());
    AddChild(m_main);
    int r;
    r = m_main->Init(0);
    if (r < 0)
        return r;
    m_main->m_pos = VIVect2I(16, 11);
    m_items = new VIWndItemMenu;
    if (!m_items)
    {
    fail:
        return -1;
    }
    m_items->SetVisible(false);
    m_items->SetAppPtr(GetAppPtr());
    AddChild(m_items);
    r = m_items->Init();
    if (r < 0)
        return r;
    return 0;
}

// 0x00C97F10.
int VIWndStartMenu::Free()
{
    if (m_main)
    {
        RemoveChild(m_main);
        delete m_main;
        m_main = 0;
    }
    if (m_items)
    {
        RemoveChild(m_items);
        delete m_items;
        m_items = 0;
    }
    return 0;
}

// 0x00C97FA8.
void VIWndStartMenu::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:
        if (w == m_main)
            GetParent()->OnMessage(this, msg, p1, p2);
        else
            ShowMain(0);
        break;
    case 261:
        if (p1 == 0x2000000)
            ShowItems();
        else
        {
            w->SetVisible(false);
            GetParent()->OnMessage(this, msg, p1, p2);
        }
        break;
    default:
        GetParent()->OnMessage(this, msg, p1, p2);
        break;
    }
}

// 0x00C980C0.
void VIWndStartMenu::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (key == 27 || key == 144)
    {
        app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
    }
}

// 0x00C98138.
void VIWndStartMenu::OnDrawSelf()
{
    VIVect2I origin(0, 0);
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 48), (VIAlphaBlendMode)0, 1.0f);
    VIUIFrameParam fp, studs;
    memset(&fp, 0, sizeof(fp));
    fp.m_bottom.m_id = (VIUITexId)49;
    fp.m_left.m_id = (VIUITexId)47;
    fp.m_br.m_id = (VIUITexId)50;
    fp.m_right.m_id = (VIUITexId)52;
    fp.m_top.m_id = (VIUITexId)45;
    fp.m_tl.m_id = (VIUITexId)44;
    fp.m_bl.m_id = (VIUITexId)48;
    fp.m_tr.m_id = (VIUITexId)46;
    ui->BlitFillFrame(VIVect2I(-1, 48), VIVect2I(176, 265), fp, (VIUITexId)35,
                      (VIAlphaBlendMode)0, 1.0f, 1);
    ui->BlitRect((VIUITexId)51, VIVect2I(451, 48), VIVect2I(199, 265), (VIAlphaBlendMode)0, 1.0f);
    memset(&fp, 0, sizeof(fp));
    memset(&studs, 0, sizeof(studs));
    fp.m_bottom.m_id = (VIUITexId)40;
    fp.m_br.m_flags = 1;
    fp.m_right.m_flags = 1;
    fp.m_top.m_id = (VIUITexId)37;
    fp.m_tr.m_flags = 1;
    fp.m_left.m_id = (VIUITexId)42;
    fp.m_bl.m_id = (VIUITexId)39;
    fp.m_br.m_id = (VIUITexId)39;
    fp.m_tl.m_id = (VIUITexId)36;
    fp.m_right.m_id = (VIUITexId)42;
    fp.m_tr.m_id = (VIUITexId)36;
    studs.m_bottom.m_id = (VIUITexId)41;
    studs.m_left.m_id = (VIUITexId)43;
    studs.m_right.m_flags = 1;
    studs.m_right.m_id = (VIUITexId)43;
    studs.m_top.m_id = (VIUITexId)38;
    ui->BlitStuddedHorz(VIVect2I(-1, 37), (VIUITexId)37, (VIUITexId)38, 177, 1, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(-1, 313), (VIUITexId)40, (VIUITexId)41, 177, 1, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(451, 37), (VIUITexId)37, (VIUITexId)38, 200, 1, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(451, 313), (VIUITexId)40, (VIUITexId)41, 200, 1, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedFrame(VIVect2I(176, 37), VIVect2I(275, 294), fp, studs, 2, 3,
                         (VIAlphaBlendMode)0, 1.0f, 0);
    ui->End2D();
}

// 0x00C98460.
void VIWndStartMenu::ShowMain(int a)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_items->SetVisible(false);
    m_main->Activate(a);
}

// 0x00C984A8.
void VIWndStartMenu::ShowItems()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_main->SetVisible(false);
    m_items->Open(this);
    app->m_p13B90 = 0;
}

// 0x00C984F8.
void VIWndStartMenu::SetMainX24(VIMenuDef *menu)
{
    m_main->m_menu = menu;
}

// 0x00C98508.
void VIWndStartMenu::Items34(VIMenuDef *menu)
{
    m_items->SetMenuD(menu);
}

// 0x00C98528.
void VIWndStartMenu::Items38(VIMenuDef *menu)
{
    m_items->SetMenuB(menu);
}

// 0x00C98548.
void VIWndStartMenu::Items3C(VIMenuDef *menu)
{
    m_items->SetMenuA(menu);
}

// 0x00C98568.
void VIWndStartMenu::Items40(VIMenuDef *menu)
{
    m_items->SetMenuC(menu);
}
