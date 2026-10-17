#line 1 "C:/code-frontiers-gm/game/VIWndExperiment.cpp"
// VIWndExperiment.cpp -- CLIENT 0x00C47E40 .. 0x00C49D90: the experiment (crafting)
// window (cl_we.h).  Names ours.
#include "game.h"

// 0x00C47E40.
VIWndExperiment::VIWndExperiment()
{
    m_mbox = 0;
    m_menu = 0;
    m_inspect = 0;
}

// 0x00C47E90.
VIWndExperiment::~VIWndExperiment()
{
    Free();
}

// 0x00C47EF0.
int VIWndExperiment::Init()
{
    m_menu = new VIWndMenu;
    if (!m_menu)
        return -1;
    m_menu->SetAppPtr(GetAppPtr());
    m_menu->SetPos(VIVect2I(16, 37));
    m_menu->SetX84(1);
    m_menu->Init(40);
    m_menu->SetMenu(&m_list);
    AddChild(m_menu);

    m_inspect = new VIWndInspect;
    if (!m_inspect)
        return -1;
    m_inspect->SetVisible(false);
    m_inspect->SetAppPtr(GetAppPtr());
    if (m_inspect->Init(1, -1))
        return -1;
    AddChild(m_inspect);

    m_mbox = new VIWndMBox;
    if (!m_mbox)
        return -1;
    m_mbox->SetAppPtr(GetAppPtr());
    int r = m_mbox->Init(1, 1);
    if (r < 0)
        return r;
    m_mbox->m_pos = VIVect2I(160, 115);
    AddChild(m_mbox);

    m_list.m_x00 = -1;
    m_list.m_x04 = -1;
    m_list.m_count = 40;
    m_list.m_entries = m_rows;
    m_info.SetClient((VIClient *)GetAppPtr());
    return 0;
}

// 0x00C48098.
int VIWndExperiment::Free()
{
    if (m_inspect)
    {
        RemoveChild(m_inspect);
        delete m_inspect;
        m_inspect = 0;
    }
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    if (m_menu)
    {
        RemoveChild(m_menu);
        delete m_menu;
        m_menu = 0;
    }
    return 0;
}

// 0x00C48168.  The title bar, the selected item's description, the recipe list
// and its legend; the menu draws itself last.
void VIWndExperiment::OnDrawSelf()
{
    unsigned short buf[512];
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    VIClientPart3834C *part = app->GetP3834C();
    Refresh();
    ui.Begin2D();
    m_menu->SetVisible(false);
    ui.BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    VIString::snprintf16(buf, sizeof(buf), "%S", ui.m_words.GetString(1546, ui.m_language));
    int w = ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, buf) + 64;
    ui.BlitBar(buf, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), VIVect2I(320 - w / 2, 10), (VIUITexId)33, (VIUITexId)34,
               ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, buf) + 64,
               (VIAlphaBlendMode)0, 1.0f, 0, 1);

    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(300, 180), 290, 14, 0);
    m_window.SetTitle("");
    m_window.Draw();
    buf[0] = 0;
    int sel = m_menu->GetId(m_menu->GetSel());
    m_info.DescribeInvCharged(buf, sizeof(buf), sel);
    m_window.TextBox(buf, 20, 0, m_window.Width() - 40, 14, (VIUICOLOR)0, 2);

    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(536, 180), 200, 14, 2);
    m_window.SetTitle("");
    m_window.Draw();
    VIVect2I p;
    p = m_window.TopLeft() + VIVect2I(14, 19);
    for (; i < 4; i++)
    {
        ui.BlitBar("", VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), p, (VIUITexId)53, (VIUITexId)55,
                   (VIUITexId)54, 175, (VIAlphaBlendMode)0, 1.0f, 0, 1);
        p.y += 34;
    }

    int row = 0;
    VIPool<VINetItem> &pool = part->m_combine;
    for (int k = pool.Begin(); k != pool.End(); k = pool.Next(k))
    {
        m_window.TextBox(m_window.T(pool.Value(k).m_name), 20, row * 2, m_window.Width() - 40, 1,
                         VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 3);
        row++;
    }

    VIString::snprintf16(buf, sizeof(buf), "|x%S", ui.m_words.GetString(212, ui.m_language));
    m_window.TextBox(m_window.T(buf), 20, 11, m_window.Width() - 40, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 1);
    VIString::snprintf16(buf, sizeof(buf), "|s%S", ui.m_words.GetString(1475, ui.m_language));
    m_window.TextBox(m_window.T(buf), 20, 12, m_window.Width() - 40, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 1);
    VIString::snprintf16(buf, sizeof(buf), "|t%S", ui.m_words.GetString(161, ui.m_language));
    m_window.TextBox(m_window.T(buf), 20, 13, m_window.Width() - 40, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 1);
    m_menu->OnDraw();
    ui.End2D();
}

// 0x00C48728.
void VIWndExperiment::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3834C *part = app->GetP3834C();
    switch (msg)
    {
    case 261:
        if (w == m_menu)
        {
            app->PlaySound(13);
            switch (p1)
            {
            case (int)0xA7030000:
                Use(p2);
                break;
            case (int)0xA7030001:
                Remove(p2);
                break;
            case (int)0xA7030002:
                Inspect(p2);
                break;
            }
        }
        break;
    case 268:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SetActiveWindow(m_menu);
            if (p1 == (int)0xA7030004)
                part->Send5(p2);
        }
        break;
    case 260:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SetActiveWindow(m_menu);
        }
        else if (w == m_inspect)
        {
            m_inspect->Clear();
            m_inspect->SetVisible(false);
            app->SetActiveWindow(m_menu);
        }
        else if (w == m_menu)
            app->ClosePanel70C();
        break;
    }
}

// 0x00C48908.
void VIWndExperiment::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3834C *part = app->GetP3834C();
    switch (key)
    {
    case 81:
    case 113:
    case 133:
        app->PlaySound(13);
        part->Combine();
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel70C();
        break;
    }
}

// 0x00C489C8.
void VIWndExperiment::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 400, 14, 0);
    m_window.Open(true);
    Refresh();
    m_mbox->SetVisible(false);
    m_menu->ResetScroll();
    m_menu->SetX64(1);
    app->SetActiveWindow(m_menu);
}

// 0x00C48A98.  Every row emptied, then one per usable item.
void VIWndExperiment::Refresh()
{
    int row = 0;
    VIClientPart3834C *part = ((VIClient *)GetAppPtr())->GetP3834C();
    for (; row < m_list.m_count; row++)
        SetRow(-1, row);
    row = 0;
    for (int i = 0; i < part->m_items.ItemCount && row < m_list.m_count; i++)
    {
        VINetItem *item = part->m_items.Item(i);
        if (item && item->m_p54 > 0 && item->m_p4C != 8)
        {
            SetRow(i, row);
            row++;
        }
    }
}

// 0x00C48BA8.
void VIWndExperiment::SetRow(int item, int row)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3834C *part = app->GetP3834C();
    if (item < 0)
    {
        m_list.m_entries[row].m_icon = -1;
        m_list.m_entries[row].m_icon2 = -1;
        m_list.m_entries[row].m_x08 = -1;
        m_list.m_entries[row].m_number = -1;
        m_list.m_entries[row].m_word = -1;
        m_list.m_entries[row].m_sub = 0;
        m_list.m_entries[row].m_msg = 0;
        m_list.m_entries[row].m_id = -1;
        return;
    }
    VINetItem *it = part->m_items.Item(item);
    m_list.m_entries[row].m_icon = app->ItemIcon(it);
    m_list.m_entries[row].m_icon2 = -1;
    m_list.m_entries[row].m_x08 = -1;
    m_list.m_entries[row].m_number = it->Quantity;
    m_list.m_entries[row].m_word = -1;
    m_list.m_entries[row].m_sub = &m_subs[row];
    m_list.m_entries[row].m_msg = 0;
    m_list.m_entries[row].m_id = item;
    if (it->m_maxHp > 0 && (float)it->m_curHp / (float)it->m_maxHp < 0.25f)
        m_list.m_entries[row].m_x08 = 1;
    m_subs[row].m_x00 = -1;
    m_subs[row].m_x04 = -1;
    m_subs[row].m_count = 3;
    m_subs[row].m_entries = m_opts[row];
    m_subs[row].m_entries[0].m_word = 1603;
    m_subs[row].m_entries[0].m_msg = 0xA7030000;
    m_subs[row].m_entries[1].m_word = 516;
    m_subs[row].m_entries[1].m_msg = 0xA7030001;
    m_subs[row].m_entries[2].m_word = 122;
    m_subs[row].m_entries[2].m_msg = 0xA7030002;
    for (int k = 0; k < 3; k++)
    {
        m_subs[row].m_entries[k].m_sub = 0;
        m_subs[row].m_entries[k].m_icon = -1;
        m_subs[row].m_entries[k].m_number = -1;
        m_subs[row].m_entries[k].m_id = item;
    }
}

// 0x00C48E30.
void VIWndExperiment::Use(int item)
{
    ((VIClient *)GetAppPtr())->GetP3834C()->AddToCombine(item);
}

// 0x00C48E78.
void VIWndExperiment::Remove(int item)
{
    ((VIClient *)GetAppPtr())->GetP3834C()->RemoveFromCombine(item);
}

// 0x00C48EC0.
void VIWndExperiment::Inspect(int item)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3834C *part = app->GetP3834C();
    unsigned short buf[512];
    int sel = m_menu->GetId(m_menu->GetSel());
    m_info.DescribeInvCharged(buf, sizeof(buf), sel);
    m_mbox->SetVisible(false);
    m_inspect->Init(1, -1);
    m_inspect->SetText(buf);
    m_inspect->SetVisible(true);
    app->SetActiveWindow(m_inspect);
}

// 0x00C48F60.
void VIWndExperiment::Notify(const unsigned short *text, int x)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_inspect->SetVisible(false);
    m_menu->SetMenu(&m_list);
    m_mbox->Init(0, 1);
    m_mbox->SetText(text);
    m_mbox->m_p1 = -1;
    m_mbox->m_p2 = -1;
    m_mbox->m_icon = x;
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}

// 0x00C49010.
void VIWndExperiment::Ask(const unsigned short *text, int p1, int p2, int x)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_inspect->SetVisible(false);
    m_menu->SetMenu(&m_list);
    m_mbox->Init(1, 1);
    m_mbox->SetText(text);
    m_mbox->m_p1 = p1;
    m_mbox->m_p2 = p2;
    m_mbox->m_icon = x;
    m_mbox->SetSel(0);
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}
