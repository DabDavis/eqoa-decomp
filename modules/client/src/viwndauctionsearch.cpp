#line 1 "C:/code-frontiers-gm/game/VIWndAuctionSearch.cpp"
// viwndauctionsearch.cpp (name ours) -- CLIENT 0x00C0F9F0 .. 0x00C152A8: the auction search.
// A form (name, class, race, level and price ranges, wear slot, attack type, a flag,
// seller) handed to VIBankControl::Search; the pages that come back in a VIWndMenu, each
// lot with Bid and Inspect options; a lot's detail, the bidders, and the bid form.
// Method names are ours.
#include "game.h"

// The pickers' rows (0x00CE1380 .. 0x00CE148C): the values a row stands for.
static int sRaces[12] = { -1, -2, 6, 2, 4, 1, 8, 3, 7, 0, 9, 5 };
static int sClasses[16] = { -1, 14, 5, 9, 7, 12, 10, 4, 11, 2, 1, 6, 3, 8, 0, 13 };
static int sWears[27] = { -2, -1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
                          19, 20, 22, 23, 24, 25, 27 };
static int sAttacks[11] = { -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 10 };

// 0x00C0F9F0.
VIWndAuctionSearch::VIWndAuctionSearch()
{
    m_edit = 0;
    m_inspect = 0;
    m_menu = 0;
    m_mbox = 0;
    m_field = 0;
    m_cursor = 0;
}

// 0x00C0FA60.
VIWndAuctionSearch::~VIWndAuctionSearch()
{
    Free();
}

// 0x00C0FAD8.
// @@BEGIN Init
int VIWndAuctionSearch::Init()
{
    m_menu = new VIWndMenu;
    if (!m_menu)
        return -1;
    m_menu->SetAppPtr(GetAppPtr());
    m_menu->SetPos(VIVect2I(16, 37));
    m_menu->SetMenu(&m_def);
    AddChild(m_menu);
    m_menu->Init(40);
    m_inspect = new VIWndInspect;
    if (!m_inspect)
        return -1;
    m_inspect->SetVisible(false);
    m_inspect->SetAppPtr(GetAppPtr());
    if (m_inspect->Init(1, -1))
        return -1;
    AddChild(m_inspect);
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->SetVisible(false);
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        return -1;
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    if (m_mbox->Init(0, 1))
        return -1;
    m_mbox->SetParam1(-1);
    m_mbox->SetParam2(-1);
    m_def.m_x00 = -1;
    m_def.m_x04 = -1;
    m_def.m_count = 40;
    m_def.m_entries = m_rows;
    m_info.SetClient((VIClient *)GetAppPtr());
    m_state = 0;
    m_field = 0;
    m_cursor = 0;
    m_mode = 0;
    return 0;
}
// @@END Init

// 0x00C0FCB8.
int VIWndAuctionSearch::Free()
{
    if (m_inspect)
    {
        RemoveChild(m_inspect);
        delete m_inspect;
        m_inspect = 0;
    }
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    if (m_menu)
    {
        RemoveChild(m_menu);
        delete m_menu;
        m_menu = 0;
    }
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    return 0;
}

// 0x00C0FDC0.
// @@BEGIN OnDrawSelf
void VIWndAuctionSearch::OnDrawSelf()
{
    unsigned short buf[256];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIBankControl *bank = app->GetBank();
    FillMenu();
    m_mbox->SetPos(VIVect2I(175, 115));
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    if (m_mode == 1)
        VIString::snprintf16(buf, 512, (const unsigned short *)ui->m_words.GetString(1674, ui->m_language));
    else if (m_mode == 2)
        VIString::snprintf16(buf, 512, (const unsigned short *)ui->m_words.GetString(1675, ui->m_language));
    else
        VIString::snprintf16(buf, 512, (const unsigned short *)ui->m_words.GetString(1678, ui->m_language));
    unsigned short *title = buf;
    ui->BlitBar(title, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f),
                VIVect2I(320 - (ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, title) + 64) / 2, 10),
                (VIUITexId)33, (VIUITexId)34, ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, title) + 64,
                (VIAlphaBlendMode)0, 1.0f, 0, 1);
    m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(300, 180), 290, 14, 0);
    m_window.SetTitle("");
    m_window.Draw();
    int cols[2];
    cols[0] = 30;
    cols[1] = 140;
    if (m_mode == 0)
    {
        unsigned short word = 413;
        DrawRow(&m_window, 0, m_window.T(1679), 0, cols[0], m_window.Width() - 20, 0, 0, 1);
        DrawRow(&m_window, 1, m_window.T(1680), m_name, cols[0], cols[1], m_field == 0, m_state == 0, 1);
        m_class = VIMIN(VIMAX(m_class, -1), 15);
        if (m_class == -1)
            word = 471;
        else
        {
            const VIClassDef *c = GetClass(m_class);
            if (c)
                word = c->m_nameStrID;
        }
        DrawRow(&m_window, 2, (const unsigned short *)ui->m_words.GetString(288, ui->m_language),
                (const unsigned short *)ui->m_words.GetString(word, ui->m_language), cols[0], cols[1],
                m_field == 1, m_state == 0, 1);
        word = 413;
        m_race = VIMIN(VIMAX(m_race, -2), 10);
        if (m_race == -1)
            word = 471;
        else if (m_race == -2)
            word = 302;
        else
        {
            const VIRaceDef *r = GetRace(m_race);
            if (r)
                word = r->m_nameStrID;
        }
        DrawRow(&m_window, 3, (const unsigned short *)ui->m_words.GetString(289, ui->m_language),
                (const unsigned short *)ui->m_words.GetString(word, ui->m_language), cols[0], cols[1],
                m_field == 2, m_state == 0, 1);
        m_minLevel = VIMIN(VIMAX(m_minLevel, 0), 60);
        VIString::snprintf16(buf, 512, "%d", m_minLevel);
        word = 413;
        DrawRow(&m_window, 4, m_window.T(1681), buf, cols[0], cols[1], m_field == 3, m_state == 0, 1);
        int maxl = VIMAX(m_minLevel, m_maxLevel);
        if (maxl > 60)
            maxl = 60;
        m_maxLevel = maxl;
        VIString::snprintf16(buf, 512, "%d", m_maxLevel);
        DrawRow(&m_window, 5, m_window.T(1682), buf, cols[0], cols[1], m_field == 4, m_state == 0, 1);
        if (m_wear == -2)
            word = 471;
        else
        {
            VIWordId *w = FindWearSlotWord(m_wear);
            if (w)
                word = w->m_word;
        }
        DrawRow(&m_window, 6, m_window.T(1683), (const unsigned short *)ui->m_words.GetString(word, ui->m_language),
                cols[0], cols[1], m_field == 5, m_state == 0, 1);
        word = 413;
        if (m_attack == -1)
            word = 471;
        else
        {
            VIWordId *w = FindAttackWord(m_attack);
            if (w)
                word = w->m_word;
        }
        DrawRow(&m_window, 7, m_window.T(1684), (const unsigned short *)ui->m_words.GetString(word, ui->m_language),
                cols[0], cols[1], m_field == 6, m_state == 0, 1);
        m_minPrice = VIMIN(VIMAX(m_minPrice, 0), 999999999);
        VIString::snprintf16(buf, 512, "%d", m_minPrice);
        DrawRow(&m_window, 8, m_window.T(1685), buf, cols[0], cols[1], m_field == 7, m_state == 0, 1);
        int maxp = VIMAX(m_minPrice, m_maxPrice);
        if (maxp > 999999999)
            maxp = 999999999;
        m_maxPrice = maxp;
        VIString::snprintf16(buf, 512, "%d", m_maxPrice);
        DrawRow(&m_window, 9, m_window.T(1686), buf, cols[0], cols[1], m_field == 8, m_state == 0, 1);
        VIString::snprintf16(buf, 512, (const unsigned short *)ui->m_words.GetString(803, ui->m_language));
        DrawRow(&m_window, 10, buf, m_window.T(m_x4F4 ? 1688 : 1689), cols[0], cols[1], m_field == 9, m_state == 0, 1);
        VIString::snprintf16(buf, 512, (const unsigned short *)ui->m_words.GetString(1690, ui->m_language));
        DrawRow(&m_window, 11, buf, m_window.T(m_seller), cols[0], cols[1], m_field == 10, m_state == 0, 1);
        DrawRow(&m_window, 12, m_window.T(509), 0, cols[0], cols[1], m_field == 10, 0, 1);
        DrawRow(&m_window, 13, m_window.T(1673), 0, cols[0], cols[1], m_field == 11, m_state == 0, 1);
    }
    m_menu->SetVisible(false);
    m_menu->OnDraw();
    m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(536, 180), 200, 14, 2);
    m_window.SetTitle("");
    m_window.Draw();
    ui->BlitHorz(VIVect2I(16, 322), (VIUITexId)60, 619, (VIAlphaBlendMode)0, 1.0f, 0);
    if ((unsigned int)(m_state - 1) < 4)
        DrawSummary();
    if (m_mode == 0)
    {
        if (m_state == 1)
            VIString::snprintf16(buf, 512, "|s%S |c%S", ui->m_words.GetString(1692, ui->m_language),
                                 ui->m_words.GetString(216, ui->m_language));
        else
            VIString::snprintf16(buf, 512, "|-|s%S |c%S", ui->m_words.GetString(1692, ui->m_language),
                                 ui->m_words.GetString(216, ui->m_language));
        m_window.TextBox(buf, 16, 12, m_window.Width() - 32, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 1);
    }
    VIString::snprintf16(buf, 512, "|x%S |t%S", ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(161, ui->m_language));
    m_window.TextBox(buf, 16, 13, m_window.Width() - 32, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 1);
    switch (m_state)
    {
    case 2:
        DrawLot();
        break;
    case 3:
        DrawRankings();
        break;
    case 4:
        DrawBid();
        break;
    case 6:
        DrawClasses();
        break;
    case 5:
        DrawRaces();
        break;
    case 7:
        DrawTypes();
        break;
    case 8:
        DrawSlots();
        break;
    }
    if (bank->IsSearching())
    {
        m_status.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 300, 3, 0);
        m_status.SetTitle(m_status.T(271));
        m_status.Open(0);
        m_status.Draw();
        if (--m_ticks <= 0)
        {
            m_ticks = 15;
            if (++m_dots == 3)
                m_dots = 0;
        }
        VIString::strcpy16(buf, 512, m_status.T(1693));
        for (char d = 0; d < m_dots + 1; d++)
            VIString::strcat16(buf, 512, ".");
        m_status.TextBox(buf, 40, 1, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
    }
    else
        m_status.Close(1);
    ui->End2D();
}
// @@END OnDrawSelf

// 0x00C10AD8.  The pad, per state: the form, the result pages, a lot, the bidders, the
// bid form, and the four pickers (6 classes, 5 races, 7 wear slots, 8 attack types).
// @@BEGIN OnKeyDown
void VIWndAuctionSearch::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIBankControl *bank = app->GetBank();
    if (m_state == 0)
    {
        switch (key)
        {
        case 17:
        case 136:
            if (m_menu->Count() > 0)
            {
                app->PlaySound(11);
                SetState(1);
            }
            else
                app->PlaySound(9);
            break;
        case 19:
        case 138:
            if (m_field <= 0)
            {
                m_field = 11;
                app->PlaySound(11);
            }
            else
            {
                m_field--;
                app->PlaySound(11);
            }
            m_window.ResetTickPulse();
            break;
        case 20:
        case 139:
            if (m_field >= 11)
            {
                m_field = 0;
                app->PlaySound(11);
            }
            else
            {
                m_field++;
                app->PlaySound(11);
            }
            m_window.ResetTickPulse();
            break;
        case 13:
        case 134:
            Select();
            break;
        case 27:
        case 132:
            app->PlaySound(8);
            app->ClosePanel718();
            break;
        }
    }
    else if (m_state == 1)
    {
        switch (key)
        {
        case 18:
        case 137:
            if (m_mode == 0)
            {
                app->PlaySound(11);
                SetState(0);
            }
            else
                app->PlaySound(9);
            m_window.ResetTickPulse();
            break;
        case 81:
        case 113:
        case 133:
            if (m_mode == 0)
            {
                app->PlaySound(13);
                bank->PrevPage();
                m_menu->ResetScroll();
            }
            break;
        case 69:
        case 101:
        case 135:
            if (m_mode == 0)
            {
                app->PlaySound(13);
                bank->NextPage();
                m_menu->ResetScroll();
            }
            break;
        case 27:
        case 132:
            if (m_mode == 1 || m_mode == 2)
            {
                app->PlaySound(8);
                app->ClosePanel718();
            }
            break;
        }
    }
    else if (m_state == 2)
    {
        switch (key)
        {
        case 27:
        case 132:
            app->PlaySound(8);
            SetState(1);
            break;
        case 81:
        case 113:
        case 133:
            app->PlaySound(13);
            bank->Send09(m_lot);
            SetState(3);
            break;
        case 13:
        case 134:
            {
                VIAuctionItem *lot = &bank->GetResults()->m_data[m_menu->m_sel];
                if (lot->m_p2FC - bank->GetMinutes() < 0)
                {
                    app->AddText(m_window.T(1776), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
                    app->PlaySound(9);
                }
                else if (bank->HasLot(lot->m_lotId))
                    app->PlaySound(9);
                else
                {
                    app->PlaySound(13);
                    SetState(4);
                }
            }
            break;
        }
    }
    else if (m_state == 3)
    {
        switch (key)
        {
        case 19:
        case 138:
            if (m_cursor <= 0)
            {
                m_cursor = 0;
                app->PlaySound(9);
            }
            else
            {
                m_cursor--;
                app->PlaySound(11);
            }
            m_list.ResetTickPulse();
            break;
        case 20:
        case 139:
            if (bank->IsIdle() && !bank->GetP7860())
            {
                VIArray<VIDbRemoteCharacterRow> *list = bank->GetP78C0();
                if (list->m_size == 0)
                {
                    m_cursor = 0;
                    app->PlaySound(9);
                }
                else if (m_cursor >= list->m_size - 1)
                {
                    m_cursor = list->m_size - 1;
                    app->PlaySound(9);
                }
                else
                {
                    m_cursor++;
                    app->PlaySound(11);
                }
                m_list.ResetTickPulse();
            }
            break;
        case 27:
        case 132:
            app->PlaySound(8);
            SetState(2);
            break;
        }
    }
    else if (m_state == 4)
    {
        switch (key)
        {
        case 19:
        case 138:
            if (m_cursor <= 0)
            {
                app->PlaySound(9);
                m_cursor = 0;
            }
            else
            {
                app->PlaySound(11);
                m_cursor--;
            }
            m_list.ResetTickPulse();
            break;
        case 20:
        case 139:
            if (m_cursor >= 2)
            {
                app->PlaySound(9);
                m_cursor = 2;
            }
            else
            {
                app->PlaySound(11);
                m_cursor++;
            }
            m_list.ResetTickPulse();
            break;
        case 27:
        case 132:
            app->PlaySound(8);
            SetState(2);
            break;
        case 13:
        case 134:
            app->PlaySound(13);
            switch (m_cursor)
            {
            case 0:
                EditField();
                break;
            case 1:
                if (bank->HasLot(m_lot))
                    app->AddText(m_window.T(1775), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
                else
                {
                    unsigned short text[256];
                    VIAuctionItem *lot = &bank->GetResults()->m_data[m_menu->m_sel];
                    VIString::snprintf16(text, 512, "%S: %S\n%S: %d %S\n%S",
                                         ui->m_words.GetString(1723, ui->m_language), lot->m_name,
                                         ui->m_words.GetString(1728, ui->m_language), m_bid,
                                         ui->m_words.GetString(479, ui->m_language),
                                         ui->m_words.GetString(1791, ui->m_language));
                    ShowMessage(text, 0, 0);
                }
                break;
            case 2:
                if (bank->HasLot(m_lot))
                    app->AddText(m_window.T(1775), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
                else
                {
                    unsigned short text[256];
                    VIAuctionItem *lot = &bank->GetResults()->m_data[m_menu->m_sel];
                    VIString::snprintf16(text, 512, "%S: %S\n%S: %d %S\n%S",
                                         ui->m_words.GetString(1723, ui->m_language), lot->m_name,
                                         ui->m_words.GetString(1697, ui->m_language), lot->m_p2F8,
                                         ui->m_words.GetString(479, ui->m_language),
                                         ui->m_words.GetString(1791, ui->m_language));
                    ShowMessage(text, 1, 0);
                }
                break;
            }
            break;
        }
    }
    else if (m_state == 6)
    {
        switch (key)
        {
        case 19:
        case 138:
            if (m_cursor <= 0)
            {
                app->PlaySound(9);
                m_cursor = 0;
            }
            else
            {
                app->PlaySound(11);
                m_cursor--;
            }
            m_list.ResetTickPulse();
            break;
        case 20:
        case 139:
            if (m_cursor >= 15)
            {
                app->PlaySound(9);
                m_cursor = 15;
            }
            else
            {
                app->PlaySound(11);
                m_cursor++;
            }
            m_list.ResetTickPulse();
            break;
        case 13:
        case 134:
            m_class = sClasses[m_cursor];
            app->PlaySound(13);
            SetState(0);
            break;
        case 27:
        case 132:
            app->PlaySound(8);
            SetState(0);
            break;
        }
    }
    else if (m_state == 5)
    {
        switch (key)
        {
        case 19:
        case 138:
            if (m_cursor <= 0)
            {
                app->PlaySound(9);
                m_cursor = 0;
            }
            else
            {
                app->PlaySound(11);
                m_cursor--;
            }
            m_list.ResetTickPulse();
            break;
        case 20:
        case 139:
            if (m_cursor >= 11)
            {
                app->PlaySound(9);
                m_cursor = 11;
            }
            else
            {
                app->PlaySound(11);
                m_cursor++;
            }
            m_list.ResetTickPulse();
            break;
        case 13:
        case 134:
            m_race = sRaces[m_cursor];
            app->PlaySound(13);
            SetState(0);
            break;
        case 27:
        case 132:
            app->PlaySound(8);
            SetState(0);
            break;
        }
    }
    else if (m_state == 7)
    {
        switch (key)
        {
        case 19:
        case 138:
            if (m_cursor <= 0)
            {
                app->PlaySound(9);
                m_cursor = 0;
            }
            else
            {
                app->PlaySound(11);
                m_cursor--;
            }
            m_list.ResetTickPulse();
            break;
        case 20:
        case 139:
            if (m_cursor >= 26)
            {
                app->PlaySound(9);
                m_cursor = 26;
            }
            else
            {
                app->PlaySound(11);
                m_cursor++;
            }
            m_list.ResetTickPulse();
            break;
        case 13:
        case 134:
            m_wear = sWears[m_cursor];
            app->PlaySound(13);
            SetState(0);
            break;
        case 27:
        case 132:
            app->PlaySound(8);
            SetState(0);
            break;
        }
    }
    else if (m_state == 8)
    {
        switch (key)
        {
        case 19:
        case 138:
            if (m_cursor <= 0)
            {
                app->PlaySound(9);
                m_cursor = 0;
            }
            else
            {
                app->PlaySound(11);
                m_cursor--;
            }
            m_list.ResetTickPulse();
            break;
        case 20:
        case 139:
            if (m_cursor >= 10)
            {
                app->PlaySound(9);
                m_cursor = 10;
            }
            else
            {
                app->PlaySound(11);
                m_cursor++;
            }
            m_list.ResetTickPulse();
            break;
        case 27:
        case 132:
            app->PlaySound(8);
            SetState(0);
            break;
        case 13:
        case 134:
            m_attack = sAttacks[m_cursor];
            app->PlaySound(13);
            SetState(0);
            break;
        }
    }
}
// @@END OnKeyDown

// 0x00C117A0.  259/260 the editor's done/cancel, 261 a menu option (bid, inspect), 268
// the message box's choice, 295 a focus change.
// @@BEGIN OnMessage
void VIWndAuctionSearch::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != this && (VIWnd *)p1 != m_edit &&
            (VIWnd *)p1 != m_inspect && (VIWnd *)p1 != m_menu && (VIWnd *)p1 != m_mbox)
        {
            m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(300, 180), 290, 14, 0);
            m_window.Open(1);
            m_list.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(300, 180), 290, 10, 0);
            m_list.Close(1);
            m_status.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(300, 180), 300, 3, 0);
            m_status.Close(1);
            m_edit->SetVisible(false);
            m_inspect->SetVisible(false);
            m_menu->SetVisible(false);
            m_menu->SetX64(0);
            m_mbox->SetVisible(false);
            m_state = 0;
            m_field = 0;
            m_cursor = 0;
            m_class = -1;
            m_race = -1;
            m_minLevel = 1;
            m_maxLevel = 50;
            m_minPrice = 0;
            m_maxPrice = 0x7FFFFFFF;
            m_wear = -2;
            m_attack = -1;
            m_x4F4 = 1;
            m_name[0] = 0;
            m_seller[0] = 0;
            m_lot = -1;
            m_bid = 0;
            m_dots = 0;
            m_ticks = 15;
        }
        else if ((VIWnd *)p1 == this && (VIWnd *)p2 != this && (VIWnd *)p2 != m_menu &&
                 (VIWnd *)p2 != m_inspect && (VIWnd *)p2 != m_edit)
            m_mode = 0;
        break;
    case 260:
        if (w == m_edit || w == m_inspect)
        {
            w->SetVisible(false);
            SetState(m_state);
        }
        else if (w == m_menu)
            app->ClosePanel718();
        else if (w == m_mbox)
        {
            w->SetVisible(false);
            SetState(m_state);
        }
        break;
    case 268:
        if (w == m_mbox)
            Confirm(p1, p2);
        break;
    case 259:
        TakeText();
        break;
    case 261:
        if (w == m_menu)
        {
            app->PlaySound(13);
            switch (p1)
            {
            case (int)0xA8040000:
                Bid(p2);
                break;
            case (int)0xA8040001:
                Inspect(p2);
                break;
            }
        }
        break;
    }
}
// @@END OnMessage

// 0x00C11B00.  The transitions between the states: which window opens, closes and takes
// the pad.
void VIWndAuctionSearch::SetState(int state)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int old = m_state;
    m_state = state;
    switch (old)
    {
    case 0:
        if (state == 1)
        {
            m_menu->SetX64(1);
            app->SetActiveWindow(m_menu);
        }
        else if (state >= 5 && state <= 8)
        {
            m_cursor = 0;
            m_top = 0;
            m_list.Close(1);
            m_list.Open(0);
        }
        else if (state == 0)
            app->SetActiveWindow(this);
        break;
    case 1:
        if (state == 0)
        {
            m_menu->SetX64(0);
            app->SetActiveWindow(this);
        }
        else if (state == 2)
        {
            m_menu->SetX64(0);
            m_cursor = 0;
            m_top = 0;
            m_list.Close(1);
            m_list.Open(0);
            app->SetActiveWindow(this);
        }
        else if (state == 1)
        {
            m_menu->SetX64(1);
            app->SetActiveWindow(m_menu);
        }
        break;
    case 2:
        if (state == 1)
        {
            m_list.Close(1);
            m_menu->SetX64(1);
            app->SetActiveWindow(m_menu);
        }
        else if (state == 3)
        {
            m_cursor = 0;
            m_top = 0;
            m_list.Close(1);
            m_list.Open(0);
        }
        else if (state == 4)
        {
            m_bid = 0;
            m_cursor = 0;
            m_top = 0;
            m_list.Close(1);
            m_list.Open(0);
        }
        break;
    case 3:
        if (state == 2)
        {
            m_cursor = 0;
            m_top = 0;
            m_list.Close(1);
            m_list.Open(0);
        }
        break;
    case 4:
        if (state == 2)
        {
            m_cursor = 0;
            m_top = 0;
            m_list.Close(1);
            m_list.Open(0);
        }
        else if (state == 4)
            app->SetActiveWindow(this);
        else if (state == 1)
        {
            m_list.Close(1);
            m_menu->SetX64(1);
            app->SetActiveWindow(m_menu);
        }
        break;
    case 5:
    case 6:
    case 7:
    case 8:
        m_list.Close(1);
        break;
    }
}

// 0x00C11D08.  The menu's 40 rows: emptied, then one per lot of the page shown.
void VIWndAuctionSearch::FillMenu()
{
    VIBankControl *bank = ((VIClient *)GetAppPtr())->GetBank();
    int i;
    for (i = 0; i < m_def.m_count; i++)
    {
        VIAuctionItem none;
        none.m_lotId = -1;
        FillRow(&none, i);
    }
    if (bank->IsSearching() || bank->GetP7860())
        return;
    VIArray<VIAuctionItem> *results = bank->GetResults();
    for (i = 0; i < results->m_size; i++)
        FillRow(&results->m_data[i], i);
}

// 0x00C11E20.  One row: the lot's icon and count, its two options (bid, inspect).
void VIWndAuctionSearch::FillRow(VIAuctionItem *lot, int row)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (lot->m_lotId < 0)
    {
        m_def.m_entries[row].m_icon = -1;
        m_def.m_entries[row].m_icon2 = -1;
        m_def.m_entries[row].m_x08 = -1;
        m_def.m_entries[row].m_number = -1;
        m_def.m_entries[row].m_word = -1;
        m_def.m_entries[row].m_sub = 0;
        m_def.m_entries[row].m_msg = 0;
        m_def.m_entries[row].m_id = -1;
        return;
    }
    m_def.m_entries[row].m_icon = app->ItemIcon((VINetItem *)lot);
    m_def.m_entries[row].m_x08 = -1;
    m_def.m_entries[row].m_number = lot->m_quantity;
    m_def.m_entries[row].m_word = -1;
    m_def.m_entries[row].m_sub = &m_subs[row];
    m_def.m_entries[row].m_msg = 0;
    m_def.m_entries[row].m_id = lot->m_lotId;
    m_def.m_entries[row].m_icon2 = -1;
    if (!app->CanUseItem((VINetItem *)lot))
        m_def.m_entries[row].m_icon2 = 140;
    m_subs[row].m_x00 = -1;
    m_subs[row].m_x04 = -1;
    m_subs[row].m_count = 2;
    m_subs[row].m_entries = m_opts[row];
    m_subs[row].m_entries[0].m_word = 1676;
    m_subs[row].m_entries[0].m_msg = 0xA8040001;
    m_subs[row].m_entries[1].m_word = 1677;
    m_subs[row].m_entries[1].m_msg = 0xA8040000;
    for (int i = 0; i < 2; i++)
    {
        m_subs[row].m_entries[i].m_sub = 0;
        m_subs[row].m_entries[i].m_icon = -1;
        m_subs[row].m_entries[i].m_number = -1;
        m_subs[row].m_entries[i].m_id = lot->m_lotId;
    }
}

// 0x00C12058.  State 2: the selected lot.
// @@BEGIN DrawLot
void VIWndAuctionSearch::DrawLot()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIBankControl *bank = app->GetBank();
    if (!bank->IsIdle() || bank->GetP7860())
        return;
    int sel = m_menu->m_sel;
    if (sel < 0 || sel >= bank->GetResults()->m_size)
        return;
    VIAuctionItem *lot = &bank->GetResults()->m_data[m_menu->m_sel];
    bank->GetP7880();
    bank->GetP7888();
    bank->GetP7890();
    int left = lot->m_p2FC - bank->GetMinutes();
    m_list.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 375, 10, 0);
    if (m_list.IsOpen() || m_list.IsOpening() || m_list.IsClosing())
    {
        m_list.SetTitle(m_list.T(1677));
        m_list.Draw();
    }
    unsigned short text[512];
    unsigned short line[128];
    int w = m_list.Width() - 40;
    VIString::snprintf16(text, 1024, "%S\n", lot->m_name);
    VIString::snprintf16(line, 256, "%S: %s\n", m_list.T(1777), lot->m_seller);
    VIString::strcat16(text, 1024, line);
    VIString::snprintf16(line, 256, "%S: %d/%d\n", m_list.T(233), lot->m_curHp, lot->m_maxHp);
    VIString::strcat16(text, 1024, line);
    VIString::snprintf16(line, 256, "%S: %d\n", m_list.T(1685), lot->m_p2F4);
    VIString::strcat16(text, 1024, line);
    VIString::snprintf16(line, 256, "%S: %d\n", m_list.T(1686), lot->m_p2F8);
    VIString::strcat16(text, 1024, line);
    FormatTime(line, 256, left, 0);
    VIString::strcat16(text, 1024, line);
    m_list.TextBox(text, 20, 0, w, 8, (VIUICOLOR)0, 2);
    if (left < 0 && m_mode != 1)
        VIString::snprintf16(text, 1024, "|s%S  |-|x%S  |+|t%S", ui->m_words.GetString(1694, ui->m_language),
                             ui->m_words.GetString(1695, ui->m_language), ui->m_words.GetString(161, ui->m_language));
    else if (m_mode == 1 || bank->HasLot(lot->m_lotId))
        VIString::snprintf16(text, 1024, "|s%S  |t%S", ui->m_words.GetString(1694, ui->m_language),
                             ui->m_words.GetString(161, ui->m_language));
    else
        VIString::snprintf16(text, 1024, "|s%S  |x%S  |t%S", ui->m_words.GetString(1694, ui->m_language),
                             ui->m_words.GetString(1695, ui->m_language), ui->m_words.GetString(161, ui->m_language));
    m_list.TextBox(text, 20, 9, m_list.Width() - 40, 1, (VIUICOLOR)0, 0);
}
// @@END DrawLot

// 0x00C124C0.  State 3: the bidders, seven rows at a time.
// @@BEGIN DrawRankings
void VIWndAuctionSearch::DrawRankings()
{
    unsigned short buf[256];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    m_list.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 450, 10, 0);
    if (m_list.IsOpen() || m_list.IsOpening() || m_list.IsClosing())
    {
        m_list.SetTitle(m_list.T(1696));
        m_list.Draw();
    }
    VIBankControl *bank = app->GetBank();
    if (!bank->IsIdle() || bank->GetP7860())
        return;
    VIArray<VIDbRemoteCharacterRow> *list = bank->GetP78C0();
    if (m_cursor - m_top >= 7)
        m_top = m_cursor - 6;
    else if (m_cursor < m_top)
        m_top = m_cursor;
    int cols[2];
    cols[0] = 40;
    cols[1] = 270;
    DrawRow(&m_list, 0, (const unsigned short *)ui->m_words.GetString(509, ui->m_language),
            (const unsigned short *)ui->m_words.GetString(1697, ui->m_language), cols[0], cols[1], 0, 0, 0);
    float t;
    if (list->m_size == 7)
        t = 0.0f;
    else
        t = (float)m_top / (float)(list->m_size - 7);
    m_list.DrawFrameVScroll(22, 1, m_list.Width() - 55, 7, t);
    int row = 1;
    for (int i = m_top; i < list->m_size && row < 8; i++, row++)
    {
        unsigned short name[24];
        unsigned short num[16];
        int sel = i == m_cursor;
        VIString::str8to16(name, 48, (const char *)(*list)[i].m_name);
        VIString::snprintf16(num, 32, "%d", (*list)[i].m_x24);
        DrawRow(&m_list, row, name, num, cols[0], cols[1], sel, sel, 0);
    }
    VIString::snprintf16(buf, 512, "|t%S", ui->m_words.GetString(161, ui->m_language));
    m_list.TextBox(buf, 20, 9, m_list.Width() - 40, 1, (VIUICOLOR)0, 0);
}
// @@END DrawRankings

// 0x00C12808.  State 4: the bid form.
// @@BEGIN DrawBid
void VIWndAuctionSearch::DrawBid()
{
    unsigned short buf[256];
    VIClient *app = (VIClient *)GetAppPtr();
    m_list.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 430, 8, 0);
    VIUI *ui = &app->m_ui;
    if (m_list.IsOpen() || m_list.IsOpening() || m_list.IsClosing())
    {
        m_list.SetTitle(m_list.T(1698));
        m_list.Draw();
    }
    int cols[2];
    cols[0] = 30;
    cols[1] = 150;
    VIBankControl *bank = app->GetBank();
    if (!bank->IsIdle() || bank->GetP7860())
        return;
    int sel = m_menu->m_sel;
    if (sel < 0 || sel >= bank->GetResults()->m_size)
        return;
    VIAuctionItem *lot = &bank->GetResults()->m_data[m_menu->m_sel];
    bank->GetP7880();
    int high = (*bank->GetP7888())[m_menu->m_sel];
    bank->GetP7890();
    int min = lot->m_p2F4;
    if (min < high + 1)
        min = high + 1;
    int bid = m_bid;
    if (bid < min)
        bid = min;
    if (bid > lot->m_p2F8)
        bid = lot->m_p2F8;
    m_bid = bid;
    VIString::snprintf16(buf, 512, "%d", bid);
    DrawRow(&m_list, 1, m_list.T(1728), buf, cols[0], cols[1], m_cursor == 0, 1, 1);
    DrawRow(&m_list, 3, m_list.T(1699), 0, cols[0], m_list.Width() - 40, m_cursor == 1, 1, 1);
    DrawRow(&m_list, 4, m_list.T(1700), 0, cols[0], cols[1], 0, 0, 1);
    VIString::snprintf16(buf, 512, (const unsigned short *)ui->m_words.GetString(1701, ui->m_language), lot->m_p2F8);
    DrawRow(&m_list, 5, buf, 0, cols[0], m_list.Width() - 40, m_cursor == 2, 1, 1);
    VIString::snprintf16(buf, 512, "|x%S   |t%S", ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(161, ui->m_language));
    m_list.TextBox(buf, 20, 7, m_list.Width() - 40, 1, (VIUICOLOR)0, 0);
}
// @@END DrawBid

// 0x00C12B78.  State 6: the classes.
// @@BEGIN DrawClasses
void VIWndAuctionSearch::DrawClasses()
{
    unsigned short buf[256];
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    m_list.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 290, 10, 0);
    if (m_list.IsOpen() || m_list.IsOpening() || m_list.IsClosing())
    {
        m_list.SetTitle((const unsigned short *)ui->m_words.GetString(1702, ui->m_language));
        m_list.Draw();
    }
    if (m_cursor - m_top >= 7)
        m_top = m_cursor - 6;
    else if (m_cursor < m_top)
        m_top = m_cursor;
    m_list.DrawFrameVScroll(22, 1, m_list.Width() - 55, 7, (float)m_top / 9.0f);
    int row = 1;
    for (int i = m_top; i < 16 && row < 8; i++, row++)
    {
        bool sel = i == m_cursor;
        if (sel)
            m_list.TickR(36, row, 1);
        unsigned short word = 413;
        if (sClasses[i] == -1)
            word = 471;
        else
        {
            const VIClassDef *c = GetClass(sClasses[i]);
            if (c)
                word = c->m_nameStrID;
        }
        m_list.TextBox(m_list.T(word), 40, row, m_list.Width() - 75, 1, (VIUICOLOR)(sel ? 1 : 0), 2);
    }
    VIString::snprintf16(buf, 512, "|x%S   |t%S", ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(161, ui->m_language));
    m_list.TextBox(buf, 20, 9, m_list.Width() - 40, 1, (VIUICOLOR)0, 0);
}
// @@END DrawClasses

// 0x00C12E18.  State 5: the races.
void VIWndAuctionSearch::DrawRaces()
{
    unsigned short buf[256];
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    m_list.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 290, 10, 0);
    if (m_list.IsOpen() || m_list.IsOpening() || m_list.IsClosing())
    {
        m_list.SetTitle((const unsigned short *)ui->m_words.GetString(1703, ui->m_language));
        m_list.Draw();
    }
    if (m_cursor - m_top >= 7)
        m_top = m_cursor - 6;
    else if (m_cursor < m_top)
        m_top = m_cursor;
    m_list.DrawFrameVScroll(22, 1, m_list.Width() - 55, 7, (float)m_top / 5.0f);
    int row = 1;
    for (int i = m_top; i < 12 && row < 8; i++, row++)
    {
        bool sel = i == m_cursor;
        if (sel)
            m_list.TickR(36, row, 1);
        unsigned short word = 413;
        if (sRaces[i] == -1)
            word = 471;
        else if (sRaces[i] == -2)
            word = 302;
        else
        {
            const VIRaceDef *r = GetRace(sRaces[i]);
            if (r)
                word = r->m_nameStrID;
        }
        m_list.TextBox(m_list.T(word), 40, row, m_list.Width() - 75, 1, (VIUICOLOR)(sel ? 1 : 0), 2);
    }
    VIString::snprintf16(buf, 512, "|x%S   |t%S", ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(161, ui->m_language));
    m_list.TextBox(buf, 20, 9, m_list.Width() - 40, 1, (VIUICOLOR)0, 0);
}

// 0x00C130C8.  State 7: the wear slots.
void VIWndAuctionSearch::DrawTypes()
{
    unsigned short buf[256];
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    m_list.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 290, 10, 0);
    if (m_list.IsOpen() || m_list.IsOpening() || m_list.IsClosing())
    {
        m_list.SetTitle((const unsigned short *)ui->m_words.GetString(1704, ui->m_language));
        m_list.Draw();
    }
    if (m_cursor - m_top >= 7)
        m_top = m_cursor - 6;
    else if (m_cursor < m_top)
        m_top = m_cursor;
    m_list.DrawFrameVScroll(22, 1, m_list.Width() - 55, 7, (float)m_top / 20.0f);
    int row = 1;
    for (int i = m_top; i < 27 && row < 8; i++, row++)
    {
        bool sel = i == m_cursor;
        if (sel)
            m_list.TickR(36, row, 1);
        unsigned short word = 471;
        if (i)
        {
            word = 413;
            VIWordId *w = FindWearSlotWord(sWears[i]);
            if (w)
                word = w->m_word;
        }
        m_list.TextBox(m_list.T(word), 40, row, m_list.Width() - 75, 1, (VIUICOLOR)(sel ? 1 : 0), 2);
    }
    VIString::snprintf16(buf, 512, "|x%S   |t%S", ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(161, ui->m_language));
    m_list.TextBox(buf, 20, 9, m_list.Width() - 40, 1, (VIUICOLOR)0, 0);
}

// 0x00C13358.  State 8: the attack types.
void VIWndAuctionSearch::DrawSlots()
{
    unsigned short buf[256];
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    m_list.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(320, 180), 290, 10, 0);
    if (m_list.IsOpen() || m_list.IsOpening() || m_list.IsClosing())
    {
        m_list.SetTitle((const unsigned short *)ui->m_words.GetString(1705, ui->m_language));
        m_list.Draw();
    }
    if (m_cursor - m_top >= 7)
        m_top = m_cursor - 6;
    else if (m_cursor < m_top)
        m_top = m_cursor;
    m_list.DrawFrameVScroll(22, 1, m_list.Width() - 55, 7, (float)m_top / 4.0f);
    int row = 1;
    for (int i = m_top; i < 11 && row < 8; i++, row++)
    {
        bool sel = i == m_cursor;
        if (sel)
            m_list.TickR(36, row, 1);
        unsigned short word = 471;
        if (i)
        {
            word = 413;
            VIWordId *w = FindAttackWord(sAttacks[i]);
            if (w)
                word = w->m_word;
        }
        m_list.TextBox(m_list.T(word), 40, row, m_list.Width() - 75, 1, (VIUICOLOR)(sel ? 1 : 0), 2);
    }
    VIString::snprintf16(buf, 512, "|x%S   |t%S", ui->m_words.GetString(212, ui->m_language),
                         ui->m_words.GetString(161, ui->m_language));
    m_list.TextBox(buf, 20, 9, m_list.Width() - 40, 1, (VIUICOLOR)0, 0);
}

// 0x00C135E8.  States 1..4: the selected lot's summary on the left.
// @@BEGIN DrawSummary
void VIWndAuctionSearch::DrawSummary()
{
    unsigned short line[64];
    unsigned short text[256];
    VIClient *app = (VIClient *)GetAppPtr();
    VIBankControl *bank = app->GetBank();
    if (bank->IsSearching())
        return;
    int none = bank->GetP7860();
    if (none)
        return;
    int sel = m_menu->m_sel;
    if (sel < 0 || sel >= bank->GetResults()->m_size)
        return;
    VIAuctionItem *lot = &bank->GetResults()->m_data[m_menu->m_sel];
    int owner = (*bank->GetP7880())[m_menu->m_sel];
    int bid = (*bank->GetP7888())[m_menu->m_sel];
    int bids = (*bank->GetP7890())[m_menu->m_sel];
    int left = lot->m_p2FC - bank->GetMinutes();
    int w = m_window.Width() - 28;
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    text[0] = 0;
    VIString::snprintf16(text, 512, "%S\n", lot->m_name);
    VIString::snprintf16(line, 128, "%S:\n", m_window.T(left <= none ? 1706 : 1707));
    VIString::strcat16(text, 512, line);
    if (bid == -1)
        VIString::snprintf16(line, 128, "%S\n", m_window.T(1708));
    else if (owner == app->GetPlayer()->m_id)
        VIString::snprintf16(line, 128, "%d %S\n", bid, m_window.T(1709));
    else
        VIString::snprintf16(line, 128, "%d\n", bid);
    VIString::strcat16(text, 512, line);
    if (left > 0)
    {
        VIString::snprintf16(line, 128, "%S:\n%d\n", m_window.T(1710), bids);
        VIString::strcat16(text, 512, line);
    }
    FormatTime(line, 128, left, 1);
    VIString::strcat16(text, 512, line);
    int a, b;
    if (bank->FindBid(lot->m_lotId, &a, &b))
    {
        VIString::snprintf16(line, 128, "%S:\n%d\n", m_window.T(1711), a);
        VIString::strcat16(text, 512, line);
        VIString::snprintf16(line, 128, "%S:\n%d\n", m_window.T(1712), b);
        VIString::strcat16(text, 512, line);
    }
    m_window.TextBox(text, 16, 0, w, 12, white, 3);
}
// @@END DrawSummary

// 0x00C139B0.  Enter on a form row: a picker, the editor, the flag, or the search.
void VIWndAuctionSearch::Select()
{
    VIAuctionQuery q;
    VIClient *app = (VIClient *)GetAppPtr();
    VIBankControl *bank = app->GetBank();
    switch (m_field)
    {
    case 1:
        SetState(6);
        break;
    case 2:
        SetState(5);
        break;
    case 0:
    case 3:
    case 4:
    case 7:
    case 8:
    case 10:
        EditField();
        break;
    case 5:
        SetState(7);
        break;
    case 6:
        SetState(8);
        break;
    case 9:
        m_x4F4 = 1 - m_x4F4;
        break;
    case 11:
        {
            q.m_type = 0;
            if (m_maxPrice < 999999999)
                q.m_type = 16;
            if (m_wear >= -1)
                q.m_type |= 2;
            if (m_attack >= 0)
                q.m_type |= 1;
            if (VIString::strlen16(m_name))
                q.m_type |= 12;
            VIString::strcpy16(q.m_text, 128, m_name);
            VIString::strcpy8(q.m_name, 24, m_seller);
            if (m_race == -2)
                q.m_pA0 = 0;
            else if (m_race == -1)
                q.m_pA0 = 0xFFFFFFFF;
            else
                q.m_pA0 = 1 << m_race;
            unsigned int classes;
            if (m_class != -1)
                classes = 1 << m_class;
            else
                classes = 0xFFFFFFFF;
            q.m_pBC = m_x4F4 != 0;
            q.m_p9C = classes;
            q.m_pA4 = m_minLevel;
            q.m_pA8 = m_maxLevel;
            q.m_pAC = m_minPrice;
            q.m_pB0 = m_maxPrice;
            q.m_pB4 = m_wear;
            q.m_pB8 = m_attack;
            bank->Search(q);
            m_menu->ResetScroll();
        }
        break;
    }
    app->PlaySound(13);
}

// 0x00C13BE8.  The editor over a text or number row of the form, or the bid.
void VIWndAuctionSearch::EditField()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_mbox->SetVisible(false);
    m_edit->SetRows(1);
    m_edit->SetMultiLine(0);
    m_edit->SetCenter(VIVect2I(320, 180));
    m_edit->SetMaxLength(128);
    if (m_state == 0)
    {
        switch (m_field)
        {
        case 0:
            m_edit->SetMaxLength(64);
            m_edit->SetTitle(m_window.T(1680));
            break;
        case 3:
            m_edit->SetMaxLength(5);
            m_edit->SetTitle(m_window.T(1713));
            break;
        case 4:
            m_edit->SetMaxLength(5);
            m_edit->SetTitle(m_window.T(1714));
            break;
        case 7:
            m_edit->SetMaxLength(10);
            m_edit->SetTitle(m_window.T(1715));
            break;
        case 8:
            m_edit->SetMaxLength(10);
            m_edit->SetTitle(m_window.T(1716));
            break;
        case 10:
            m_edit->SetMaxLength(24);
            m_edit->SetTitle(m_window.T(1717));
            break;
        default:
            return;
        }
    }
    else if (m_state == 4)
    {
        m_edit->SetMaxLength(10);
        m_edit->SetTitle(m_window.T(1698));
    }
    else
        return;
    m_edit->SetVisible(true);
    app->SetActiveWindow(m_edit);
}

// 0x00C13D80.  The editor's text into the row it edited.
void VIWndAuctionSearch::TakeText()
{
    char buf[256];
    VIString::str16to8(buf, 256, m_edit->GetText());
    if (m_state == 0)
    {
        switch (m_field)
        {
        case 0:
            VIString::strcpy16(m_name, 128, m_edit->GetText());
            break;
        case 3:
            m_minLevel = atoi(buf);
            break;
        case 4:
            m_maxLevel = atoi(buf);
            break;
        case 7:
            m_minPrice = atoi(buf);
            break;
        case 8:
            m_maxPrice = atoi(buf);
            break;
        case 10:
            VIString::strcpy8(m_seller, 24, buf);
            break;
        default:
            return;
        }
    }
    else if (m_state == 4)
        m_bid = atoi(buf);
    m_edit->SetVisible(false);
    SetState(m_state);
}

// 0x00C13E88.  One row of a page: a label, a (framed) value, the tick when selected.
void VIWndAuctionSearch::DrawRow(VIUIWindow *w, int row, const unsigned short *label,
                                 const unsigned short *value, int xa, int xb, int sel, int tick,
                                 int framed)
{
    if (label)
        w->TextBox(label, xa, row, xb - xa, 1, (VIUICOLOR)(sel != 0), 0);
    if (value)
    {
        if (framed)
            w->DrawFrame(xb, row, w->m_size.x - xb - 25, 1, 64);
        w->TextBox(value, xb, row, w->m_size.x - xb - 25, 1, (VIUICOLOR)(sel != 0), 2);
    }
    if (sel && tick)
        w->TickR(xa, row, 1);
}

// 0x00C13FA0.
void VIWndAuctionSearch::Bid(int lot)
{
    m_lot = lot;
    SetState(2);
}

// 0x00C13FC0.  The lot's description in the inspect window.
void VIWndAuctionSearch::Inspect(int lot)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_lot = lot;
    VIInfoText info;
    unsigned short text[512];
    info.SetClient(app);
    VIBankControl *bank = app->GetBank();
    if (!bank->IsIdle() || bank->GetP7860())
        return;
    int sel = m_menu->m_sel;
    if (sel < 0 || sel >= bank->GetResults()->m_size)
        return;
    VIAuctionItem *it = &bank->GetResults()->m_data[m_menu->m_sel];
    bank->GetP7880();
    bank->GetP7888();
    bank->GetP7890();
    info.DescribeAuctionItem(text, 1024, it);
    m_mbox->SetVisible(false);
    m_edit->SetVisible(false);
    m_inspect->Init(1, -1);
    m_inspect->SetText(text);
    m_inspect->SetVisible(true);
    app->SetActiveWindow(m_inspect);
}

// 0x00C14130.  Buy the lot out at its buyout price.
// @@BEGIN Buyout
void VIWndAuctionSearch::Buyout(int lot)
{
    VIBankControl *bank = ((VIClient *)GetAppPtr())->GetBank();
    if (!bank->IsIdle() || bank->GetP7860())
        return;
    int sel = m_menu->m_sel;
    if (sel < 0 || sel >= bank->GetResults()->m_size)
        return;
    VIAuctionItem *it = &bank->GetResults()->m_data[m_menu->m_sel];
    bank->Send06(lot, it->m_p2F8);
}
// @@END Buyout

// 0x00C14200.  "Time left: 1h 5m", or "Ended: ..." for a negative time.
void VIWndAuctionSearch::FormatTime(unsigned short *buf, int size, int minutes, int lines)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    int h = minutes / 60;
    int m = minutes % 60;
    if (minutes <= 0)
    {
        if (h)
            VIString::snprintf16(buf, size, lines ? "%S:\n%dh %dm\n" : "%S: %dh %dm", m_window.T(1718), -h, -m);
        else
            VIString::snprintf16(buf, size, lines ? "%S:\n%dm\n" : "%S: %dm", m_window.T(1718), -m);
    }
    else if (h)
        VIString::snprintf16(buf, size, lines ? "%S:\n%dh %dm\n" : "%S: %dh %dm", m_window.T(1719), h, m);
    else
        VIString::snprintf16(buf, size, lines ? "%S:\n%S %dm\n" : "%S: %S %dm",
                             ui->m_words.GetString(1719, ui->m_language),
                             ui->m_words.GetString(1720, ui->m_language), m);
}

// 0x00C143D0.  The bank's command opens the window as "my lots" (1) or "my bids" (2).
// @@BEGIN SetMode
void VIWndAuctionSearch::SetMode(int id)
{
    switch (id)
    {
    case (int)0xA8020000:
        m_mode = 1;
        break;
    case (int)0xA8030000:
        m_mode = 2;
        break;
    case (int)0xA8010000:
    case (int)0xA8040000:
    default:
        m_mode = 0;
        break;
    }
}
// @@END SetMode

// 0x00C14418.
void VIWndAuctionSearch::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_mode == 1 || m_mode == 2)
    {
        m_menu->ResetScroll();
        SetState(1);
    }
}

// 0x00C14468.
void VIWndAuctionSearch::ShowMessage(const unsigned short *text, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_edit->SetVisible(false);
    m_inspect->SetVisible(false);
    m_mbox->Init(1, 1);
    m_mbox->SetText(text);
    m_mbox->SetParam1(p1);
    m_mbox->SetParam2(p2);
    m_mbox->SetIcon(-1);
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}

// 0x00C14520.  The message box's choice: 0 bids, 1 buys out.
// @@BEGIN Confirm
void VIWndAuctionSearch::Confirm(int p1, int p2)
{
    VIBankControl *bank = ((VIClient *)GetAppPtr())->GetBank();
    switch (p1)
    {
    case 0:
        bank->Send06(m_lot, m_bid);
        break;
    case 1:
        Buyout(m_lot);
        break;
    }
    m_mbox->SetVisible(false);
    SetState(1);
}
// @@END Confirm
