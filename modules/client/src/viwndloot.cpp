#line 1 "C:/code-frontiers-gm/game/VIWndLoot.cpp"
// VIWndLoot (name the RTTI's) -- CLIENT 0x00C6BDE0 .. 0x00C6D2D8.  The corpse window: the
// inventory (40 rows) and the corpse (42: "take all" or "empty", the items, "done") as two
// VIWndLists, rebuilt every frame from the client, and the selected item's description.
// Method names ours.
#include "game.h"

// 0x00C6BDE0.
VIWndLoot::VIWndLoot()
{
    m_inv = 0;
    m_loot = 0;
}

// 0x00C6BE30.
VIWndLoot::~VIWndLoot()
{
}

// 0x00C6BE90.
int VIWndLoot::Init()
{
    m_inv = new VIWndList;
    if (!m_inv)
        goto fail;
    m_inv->SetAppPtr(GetAppPtr());
    m_inv->SetPos(VIVect2I(5, 10));
    AddChild(m_inv);
    m_loot = new VIWndList;
    if (!m_loot)
    {
    fail:
        return -1;
    }
    m_loot->SetAppPtr(GetAppPtr());
    m_loot->SetPos(VIVect2I(144, 10));
    AddChild(m_loot);
    m_inv->Init(40);
    m_loot->Init(42);
    m_info.SetClient((VIClient *)GetAppPtr());
    return 0;
}

// 0x00C6BFA8.
void VIWndLoot::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    int i;
    for (i = 0; i < app->GetInvCount(); i++)
    {
        unsigned int flags = 1;
        if (app->GetInvItemAmount(i) >= 2)
            flags = 5;
        int arrows = -1;
        if (app->GetInvItemEquipSlot(i) != -1)
        {
            arrows = 138;
            flags |= 8;
        }
        int icon = app->GetInvItemIcon(i);
        m_inv->SetItem(i, flags, icon, arrows, -1, app->GetInvItemAmount(i), -1);
    }
    for (; i < 40; i++)
        m_inv->SetItem(i, 0, -1, -1, -1, 0, -1);
    int n = app->GetLootCount();
    if (n > 0)
        m_loot->SetItem(0, 2, -1, -1, 119, 0, -1);
    else
        m_loot->SetItem(0, 2, -1, -1, 641, 0, -1);
    for (i = 0; i < n; i++)
    {
        unsigned int flags = 1;
        if (app->GetLootItemAmount(i) >= 2)
            flags = 5;
        int icon = app->GetLootItemIcon(i);
        m_loot->SetItem(i + 1, flags, icon, -1, -1, app->GetLootItemAmount(i), -1);
    }
    if (n > 0)
        m_loot->SetItem(n + 1, 2, -1, -1, 118, 0, -1);
    else
        m_loot->SetItem(n + 1, 0, -1, -1, -1, 0, -1);
    for (i = n + 2; i < 42; i++)
        m_loot->SetItem(i, 0, -1, -1, -1, 0, -1);
    UpdateDesc();
    ui->Begin2D();
    if (!VIString::strlen16(m_desc))
        m_window.Close(true);
    else
    {
        int rows = m_window.CountLines(m_desc, m_window.Width() - 40);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(460, 152), 340, rows, 1);
        m_window.Open(false);
        m_window.Draw();
        VIUIWindow &w = m_window;
        m_window.TextBox(m_desc, 20, 0, m_window.Width() - 40, rows,
                         VIColor32F(1.0f, 1.0f, 1.0f, w.m_open), 3);
    }
    ui->End2D();
}

// 0x00C6C358.
void VIWndLoot::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        app->LootNoWindow();
        break;
    case 134:
        break;
    }
}

// 0x00C6C3B8.  Enter on the corpse list: row 0 closes, an item row takes it, the row
// after the items takes everything.
void VIWndLoot::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned char buf[512];
    VIMemStream out(buf, sizeof(buf));
    if (msg == 256)
    {
        app->PlaySound(13);
        if (w == m_loot)
        {
            if (p1 == 0)
                app->LootNoWindow();
            else if (p1 < app->GetLootCount() + 1)
            {
                if (p1 - 1 < app->GetLootCount())
                    app->LootOne(p1 - 1, app->GetLootItemAmount(p1 - 1));
            }
            else if (p1 == app->GetLootCount() + 1)
                app->RequestLootAll();
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C6C518.
void VIWndLoot::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_inv->SetX64(0);
    m_inv->VIWndList::Open();
    m_inv->ResetScroll();
    m_loot->SetX64(1);
    m_loot->VIWndList::Open();
    m_loot->ResetScroll();
    m_desc[0] = 0;
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(460, 152), 340, 10, 1);
    if (m_window.IsOpen())
        m_window.Close(true);
    m_window.Open(false);
    app->SetActiveWindow(m_loot);
}

// 0x00C6C628.
void VIWndLoot::UpdateDesc()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_desc[0] = 0;
    if (app->GetLootCount() > 0)
    {
        int i = m_loot->m_sel - 1;
        if (i >= 0 && i < app->GetLootCount())
            m_info.DescribeLootItem(m_desc, 1024, i);
    }
}
