#line 1 "C:/code-frontiers-gm/game/VIBankControl.cpp"
// VIBankControl.cpp -- CLIENT 0x00BB8988 .. 0x00BBAB80, the bank and the auction house
// (VIClient+0x47388).  Its asserts name the file (Recv1206's alert is on line 593, so the
// file is padded to put it there).  Every request is one stream sent on GetConn() --
// message 0x1205 with a sub-op byte for the auction, 0x1254/0x1255 to move items and
// money -- and m_state says which reply is awaited.  The 0x1206 replies are sub-ops
// 128..137 (claude-notes/sandstorm-s2-live-20261015.md has the server side).  The TU
// ends with its template copies (the item list, both pools, the result arrays, the
// VIMemStreamArray.h readers) and VIItemList::Get, an inline of VIItemList.h emitted here.
#include "game.h"

// 0x00BB8988
VIBankControl::VIBankControl(VIClient *client)
    : m_client(client), m_p7860(1), m_page(0), m_p7868(-1), m_p7870(-1),
      m_p78B8(1), m_p78BC(-1)
{
    m_wnd = 0;
    m_query.m_type = 0;
    m_query.m_text[0] = 0;
    m_query.m_name[0] = 0;
    m_query.m_p9C = 0xFFFFFFFF;
    m_query.m_pA0 = 0xFFFFFFFF;
    m_query.m_pA4 = 1;
    m_query.m_pA8 = 50;
    m_query.m_pAC = 0;
    m_query.m_pB0 = 0x7FFFFFFF;
    m_query.m_pB4 = 0;
    m_query.m_pB8 = 0;
    m_query.m_pBC = true;
}

// 0x00BB8AD0
VIBankControl::~VIBankControl()
{
}

// 0x00BB8BE0
int VIBankControl::Count()
{
    return m_bank.ItemCount;
}

// 0x00BB8BE8
VINetItem *VIBankControl::Item(int i)
{
    return m_bank.Item(i);
}

// 0x00BB8C08
int VIBankControl::GetMoney()
{
    return m_client->GetPlayerP38();
}

// 0x00BB8C28
void VIBankControl::Close()
{
    m_state = 0;
}

// 0x00BB8C30
int VIBankControl::IsIdle()
{
    return m_state == 0;
}

// 0x00BB8C40
int VIBankControl::IsSearching()
{
    return m_state == 5;
}

// 0x00BB8C50: put a bank item up for auction.
void VIBankControl::Sell(int slot, int a, int b, int c)
{
    VIItemList &bank = m_bank;
    if (slot < bank.ItemCount)
    {
        {
            int count = 1;
            VIMemStream out;
            unsigned short op = 0x1205;
            out.Write(&op, 2, 1);
            out.WriteByte(5);
            out.WriteU32(m_client->m_target);
            out.WriteU32(bank.GetSlotId(slot));
            VINetSend(out << count << a << b << c, GetConn());
        }
        m_state = 3;
    }
}

// 0x00BB8DF0
void VIBankControl::Send06(int a, int b)
{
    {
        int count = 1;
        VIMemStream out;
        unsigned short op = 0x1205;
        out.Write(&op, 2, 1);
        out.WriteByte(6);
        VINetSend(out << a << b << count, GetConn());
    }
    m_state = 4;
}

// 0x00BB8F08
int VIBankControl::GetP78CC()
{
    return m_p78CC;
}

// 0x00BB8F10: a new search, from the first page.
void VIBankControl::Search(const VIAuctionQuery &q)
{
    int last = -1;
    m_query = q;
    ClearSaved();
    ClearResults();
    {
        VIMemStream out;
        VINetSend(out << VIMemStreamU16(0x1205) << VIMemStreamU8(7) << VIMemStreamIntC(m_page)
                      << VIMemStreamIntC(last) << VIMemStreamU8(1) << m_query, GetConn());
    }
    m_state = 5;
}

// 0x00BB9110: the page after the last lot shown.
void VIBankControl::NextPage()
{
    int last = -1;
    VIArray<VIAuctionItem> &r = m_results;
    if (r.m_size)
        last = r[r.m_size - 1].m_lotId;
    SaveResults();
    ClearResults();
    {
        VIMemStream out;
        VINetSend(out << VIMemStreamU16(0x1205) << VIMemStreamU8(7) << VIMemStreamIntC(m_page)
                      << VIMemStreamIntC(last) << VIMemStreamU8(1) << m_query, GetConn());
    }
    m_state = 5;
}

// 0x00BB9278: the page before the first lot shown.
void VIBankControl::PrevPage()
{
    int first = -1;
    VIArray<VIAuctionItem> &r = m_results;
    if (r.m_size)
        first = r[0].m_lotId;
    SaveResults();
    ClearResults();
    {
        VIMemStream out;
        VINetSend(out << VIMemStreamU16(0x1205) << VIMemStreamU8(7) << VIMemStreamIntC(m_page)
                      << VIMemStreamIntC(first) << VIMemStreamU8(0) << m_query, GetConn());
    }
    m_state = 5;
}

// 0x00BB93D0: ask for the lots the pool holds.
void VIBankControl::RequestLots()
{
    VIVector<int> ids;
    for (int i = m_lots.Begin(); i != m_lots.End(); i = m_lots.Next(i))
        ids.Add(m_lots.Value(i).m_lotId);
    ClearSaved();
    ClearResults();
    {
        VIMemStream out;
        VINetSend(out << VIMemStreamU16(0x1205) << VIMemStreamU8(8) << VIMemStreamIntC(m_page) << ids, GetConn());
    }
    m_state = 5;
}

// 0x00BB9548: the same for the 24-byte records.
void VIBankControl::RequestBids()
{
    VIVector<int> ids;
    for (int i = m_bids.Begin(); i != m_bids.End(); i = m_bids.Next(i))
        ids.Add(m_bids.Value(i).AUCTION_ID);
    ClearSaved();
    ClearResults();
    {
        VIMemStream out;
        VINetSend(out << VIMemStreamU16(0x1205) << VIMemStreamU8(8) << VIMemStreamIntC(m_page) << ids, GetConn());
    }
    m_state = 5;
}

// 0x00BB96C0
void VIBankControl::ClearResults()
{
    m_results.Init(0);
    m_p7880.Init(0);
    m_p7888.Init(0);
    m_p7890.Init(0);
    m_p7868 = -1;
    m_p7870 = -1;
    m_p7860 = 1;
    m_page++;
}

// 0x00BB9738
void VIBankControl::ClearSaved()
{
    m_saved.Init(0);
    m_p78A0.Init(0);
    m_p78A8.Init(0);
    m_p78B0.Init(0);
}

// 0x00BB9788
void VIBankControl::SaveResults()
{
    m_saved.Init(m_results);
    m_p78A0.Init(m_p7880);
    m_p78A8.Init(m_p7888);
    m_p78B0.Init(m_p7890);
}

// 0x00BB97D8
int VIBankControl::GetP7860()
{
    return m_p7860;
}

// 0x00BB97E0
VIArray<VIAuctionItem> *VIBankControl::GetResults()
{
    return &m_results;
}

// 0x00BB97E8
VIArray<int> *VIBankControl::GetP7880()
{
    return &m_p7880;
}

// 0x00BB97F0
VIArray<int> *VIBankControl::GetP7888()
{
    return &m_p7888;
}

// 0x00BB97F8
VIArray<int> *VIBankControl::GetP7890()
{
    return &m_p7890;
}

// 0x00BB9800: m_p7868 plus the minutes since the page arrived.
int VIBankControl::GetMinutes()
{
    return m_p7868 + (int)((m_client->m_p07568 - m_p7870) / 60000);
}

// 0x00BB9848
void VIBankControl::Send09(int n)
{
    {
        VIMemStream out;
        unsigned short op = 0x1205;
        out.Write(&op, 2, 1);
        out.WriteByte(9);
        VINetSend(out << n, GetConn());
    }
    m_state = 6;
}

// 0x00BB9928
void VIBankControl::ResetP78BC()
{
    m_p78BC = -1;
    m_p78C0.Init(0);
}

// 0x00BB9950
int VIBankControl::GetP78BC()
{
    return m_p78BC;
}

// 0x00BB9958
VIArray<VIDbRemoteCharacterRow> *VIBankControl::GetP78C0()
{
    return &m_p78C0;
}

// 0x00BB9960
int VIBankControl::FindBid(int auction, int *a, int *b)
{
    for (int i = m_bids.Begin(); i != m_bids.End(); i = m_bids.Next(i))
    {
        if (m_bids.Value(i).AUCTION_ID == auction)
        {
            *a = m_bids.Value(i).m_x08;
            *b = m_bids.Value(i).m_x10;
            return 1;
        }
    }
    return 0;
}

// 0x00BB9A18: an item between the bank and the inventory.
void VIBankControl::Move(unsigned char fromBank, int slot, int n)
{
    VIItemList *list;
    int count;
    if (fromBank)
    {
        count = m_bank.ItemCount;
        list = &m_bank;
    }
    else
    {
        list = &m_client->m_inventory;
        count = list->ItemCount;
    }
    if (slot < count)
    {
        int id = list->GetSlotId(slot);
        {
            VIMemStream out;
            unsigned short op = 0x1254;
            out.Write(&op, 2, 1);
            out.WriteU32(m_client->m_target);
            out.WriteByte(fromBank);
            out.WriteU32(id);
            VINetSend(out << n, GetConn());
        }
        m_state = 2;
    }
}

// 0x00BB9B78: money between the bank and the purse.
void VIBankControl::MoveMoney(unsigned char fromBank, int n)
{
    {
        VIMemStream out;
        unsigned short op = 0x1255;
        out.Write(&op, 2, 1);
        out.WriteU32(m_client->m_target);
        out.WriteByte(fromBank);
        VINetSend(out << n, GetConn());
    }
    m_state = 1;
}

// 0x00BB9C80
void VIBankControl::Read(VIMemStream &s)
{
    m_bank.Read(s);
    s >> m_lots;
    s >> m_bids;
    m_p78CC = s.ReadBool();
}

// 0x00BB9CF0
void VIBankControl::Recv(int opcode, VIMemStream &s)
{
    switch ((unsigned short)opcode)
    {
    case 0x1253:
    {
        int money;
        s.ReadSigned(money);
        m_client->PlaySound(3);
        m_client->SetPlayerP38(money);
        m_state = 0;
        break;
    }
    case 0x1251:
        RecvAdded(s);
        break;
    case 0x1250:
        RecvTook(s);
        break;
    }
}

// (padding: Recv1206's alert sits on the original's line 593)












































































































































































// 0x00BB9D90: the auction's replies.
void VIBankControl::Recv1206(VIMemStream &s)
{
    unsigned char op;
    s.Read(&op, 1, 1);
    switch (op)
    {
    case 129:
        RecvResults(s);
        break;
    case 137:
        RecvUpdateLot(s);
        break;
    case 128:
        RecvClear();
        break;
    case 130:
        RecvAddLot(s);
        break;
    case 131:
        RecvRemoveLot(s);
        break;
    case 132:
        RecvAddBid(s);
        break;
    case 133:
        RecvRemoveBid(s);
        break;
    case 134:
        RecvUpdateBid(s);
        break;
    case 135:
        RecvRemoteChars(s);
        break;
    case 136:
        RecvP78BC(s);
        break;
    default:
        VIALERT("unknown message");
        break;
    }
}

// 0x00BB9EE8: an item came into the bank.
void VIBankControl::RecvAdded(VIMemStream &s)
{
    VINetItem item;
    ReadNetItem(s, item);
    m_bank.AddItem(item, item.m_maxStack, 40);
    m_client->PlaySound(5);
    m_state = 0;
}

// 0x00BB9F68: n of slot left the bank.
void VIBankControl::RecvTook(VIMemStream &s)
{
    unsigned char slot;
    s.Read(&slot, 1, 1);
    int n;
    s.ReadSigned(n);
    VINetItem item;
    m_bank.Get(slot, item);
    m_bank.Took(slot, n);
    m_client->PlaySound(5);
    m_state = 0;
}

// 0x00BBA008
void VIBankControl::RecvAddLot(VIMemStream &s)
{
    VIAuctionItem lot;
    s >> lot;
    m_lots.Add(lot);
    m_client->PlaySound(5);
    m_state = 0;
}

// 0x00BBA080
void VIBankControl::RecvRemoveLot(VIMemStream &s)
{
    int id;
    s >> id;
    for (int i = m_lots.Begin(); i != m_lots.End(); i = m_lots.Next(i))
    {
        if (m_lots.Value(i).m_lotId == id)
        {
            m_lots.Erase(i);
            break;
        }
    }
    m_client->PlaySound(5);
}

// 0x00BBA138
void VIBankControl::RecvAddBid(VIMemStream &s)
{
    VIDbCharBid r;
    s >> r;
    m_bids.Add(r);
    m_client->PlaySound(5);
    m_state = 0;
}

// 0x00BBA180
void VIBankControl::RecvRemoveBid(VIMemStream &s)
{
    int id;
    s >> id;
    for (int i = m_bids.Begin(); i != m_bids.End(); i = m_bids.Next(i))
    {
        if (m_bids.Value(i).AUCTION_ID == id)
        {
            m_bids.Erase(i);
            break;
        }
    }
    m_client->PlaySound(5);
}

// 0x00BBA230
void VIBankControl::RecvUpdateBid(VIMemStream &s)
{
    VIDbCharBid r;
    s >> r;
    for (int i = m_bids.Begin(); i != m_bids.End(); i = m_bids.Next(i))
    {
        if (m_bids.Value(i).AUCTION_ID == r.AUCTION_ID)
        {
            m_bids.Value(i) = r;
            break;
        }
    }
    m_client->PlaySound(5);
}

// 0x00BBA2F8: a page of results, for the request m_page names.
void VIBankControl::RecvResults(VIMemStream &s)
{
    int page;
    s >> page;
    if (m_page != page)
    {
        m_state = 0;
        return;
    }
    s >> m_p7868;
    VIArray<VIAuctionItem> &results = m_results;
    s >> results;
    s >> m_p7880;
    s >> m_p7888;
    s >> m_p7890;
    VIUI &ui = m_client->m_ui;
    if (m_results.m_size == 0)
    {
        if (m_saved.m_size == 0)
        {
            m_client->AddText((const unsigned short *)ui.m_words.GetString(1787, ui.m_language),
                              VIColor32F(0.19f, 0.96f, 0.96f, 1.0f));
        }
        else
        {
            m_results.Init(m_saved);
            m_p7880.Init(m_p78A0);
            m_p7888.Init(m_p78A8);
            m_p7890.Init(m_p78B0);
            m_client->AddText((const unsigned short *)ui.m_words.GetString(1798, ui.m_language),
                              VIColor32F(0.19f, 0.96f, 0.96f, 1.0f));
        }
        ClearSaved();
    }
    else
    {
        int changed = 0;
        for (int i = 0; i < m_results.m_size && i < m_saved.m_size; i++)
        {
            if (results[i].m_lotId != m_saved[i].m_lotId)
            {
                changed = 1;
                goto done;
            }
        }
    done:
        if (m_results.m_size != m_saved.m_size)
            changed = 1;
        if (!changed)
            m_client->AddText((const unsigned short *)ui.m_words.GetString(1799, ui.m_language),
                              VIColor32F(0.19f, 0.96f, 0.96f, 1.0f));
        ClearSaved();
    }
    m_p7870 = m_client->m_p07568;
    m_p7860 = 0;
    m_state = 0;
    if (!m_wnd->IsAuctionMode())
    {
        ClearSaved();
        ClearResults();
    }
}

// 0x00BBA5D0: one lot changed; update it where either page shows it.
void VIBankControl::RecvUpdateLot(VIMemStream &s)
{
    VIAuctionItem lot;
    s >> lot;
    s >> m_p7868;
    int a, b, c;
    s >> a;
    s >> b;
    s >> c;
    for (int i = 0; i < m_saved.m_size; i++)
    {
        if (m_saved[i].m_lotId == lot.m_lotId)
        {
            m_saved[i] = lot;
            m_p78A0[i] = a;
            m_p78A8[i] = b;
            m_p78B0[i] = c;
            break;
        }
    }
    for (int i = 0; i < m_results.m_size; i++)
    {
        if (m_results[i].m_lotId == lot.m_lotId)
        {
            m_results[i] = lot;
            m_p7880[i] = a;
            m_p7888[i] = b;
            m_p7890[i] = c;
            break;
        }
    }
}

// 0x00BBA878
void VIBankControl::RecvClear()
{
    ClearResults();
    m_state = 0;
}

// 0x00BBA8A0
void VIBankControl::RecvRemoteChars(VIMemStream &s)
{
    s >> m_p78BC;
    VIArray<VIDbRemoteCharacterRow> &list = m_p78C0;
    s >> list;
    if (list.m_size > 0)
        qsort(list.m_data, list.m_size, sizeof(VIDbRemoteCharacterRow), CompareRemoteChars);
    m_p7860 = 0;
    m_p78B8 = 0;
    m_state = 0;
}

// 0x00BBA928: both records copied, then the key at +36, highest first.
int VIBankControl::CompareRemoteChars(const void *a, const void *b)
{
    VIDbRemoteCharacterRow x = *(const VIDbRemoteCharacterRow *)a;
    VIDbRemoteCharacterRow y = *(const VIDbRemoteCharacterRow *)b;
    if (x.m_x24 > y.m_x24)
        return -1;
    return x.m_x24 < y.m_x24;
}

// 0x00BBAA00
void VIBankControl::RecvP78BC(VIMemStream &s)
{
    s >> m_p78BC;
    m_state = 0;
    m_p7860 = 0;
    m_p78B8 = 1;
}

// 0x00BBAA48
int VIBankControl::IsCommand(unsigned int id)
{
    return (id & 0xFF000000) == 0xA8000000;
}

// 0x00BBAA60
void VIBankControl::Do(int id, int arg)
{
    switch (id)
    {
    case 0xA8010000:
    case 0xA8020000:
    case 0xA8030000:
        m_wnd->OpenAuctionSearch(id);
        break;
    }
}

// 0x00BBAAB0
int VIBankControl::HasLot(int lotId)
{
    for (int i = m_lots.Begin(); i != m_lots.End(); i = m_lots.Next(i))
        if (m_lots.Value(i).m_lotId == lotId)
            return 1;
    return 0;
}

// 0x00BBAB40
VIClientConn VIBankControl::GetConn()
{
    return m_client->GetConn(251);
}

// VIItemList.h's Read, emitted here (0x00BBC8D0, among the template copies): the count, the
// second word, then each item through VINetItem.cpp's reader.
void VIItemList::Read(VIMemStream &s)
{
    s.ReadSigned(ItemCount);
    s.Read(&m_p04, 4, 1);
    for (int i = 0; i < ItemCount; i++)
        ReadNetItem(s, ItemInternal[i]);
}

// VIItemList.h's Get, emitted here (0x00BBD398, among the template copies): copy an item
// out of a slot that holds one, else hand back an empty one.
void VIItemList::Get(int slot, VINetItem &item)
{
    if (slot < ItemCount && ItemInternal[slot].Quantity > 0)
        item = ItemInternal[slot];
    else
        item.Quantity = 0;
}
