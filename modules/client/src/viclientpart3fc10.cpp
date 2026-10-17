#line 1 "C:/code-frontiers-gm/game/VIClientPart3FC10.cpp"
// VIClientPart3FC10 (name provisional) -- CLIENT 0x00BEB7E0 .. 0x00BEC3E0: fishing, at
// VIClient+0x3FC10.  Then this TU's template copies (0x00BEC3E8 .. 0x00BED208, verified
// duplicates) and its out-of-line VIItemList.h copies (0x00BED350 Get, 0x00BED650 Took,
// 0x00BED780 Read, 0x00BED810 GetSlotId -- dead: every call goes to the first copy linked;
// not written here, see NOTES.md).
#include "game.h"

// 0x00BEB7E0
VIClientPart3FC10::VIClientPart3FC10(VIClient *c)
{
    m_client = c;
    m_catch.Clear();
    m_wnd = 0;
    m_state = 0;
}

// 0x00BEB878
VIClientPart3FC10::~VIClientPart3FC10()
{
}

// 0x00BEB900
void VIClientPart3FC10::Recv0F02(VIMemStream &s)
{
    unsigned char op;
    s.Read(&op, 1, 1);
    switch (op)
    {
    case 1:
        Start();
        break;
    case 2:
        Stop();
        break;
    case 3:
        Ready();
        break;
    case 4:
        Caught(s);
        break;
    case 5:
        Waiting();
        break;
    case 6:
        Bite();
        break;
    case 7:
        Loot(s);
        break;
    case 8:
        Missed();
        break;
    case 9:
        Lost();
        break;
    case 10:
        Damaged();
        break;
    }
}

// 0x00BEBA10
void VIClientPart3FC10::Start()
{
    VIUI &ui = m_client->m_ui;
    m_catch.Clear();
    m_state = 1;
    m_wnd->OpenFishing();
    m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1431, ui.m_language));
}

// 0x00BEBA78
void VIClientPart3FC10::Stop()
{
    m_state = 0;
    m_catch.Clear();
    m_wnd->CloseFishing();
}

// 0x00BEBAA8
void VIClientPart3FC10::Ready()
{
    m_state = 1;
}

// 0x00BEBAB8: the catch at `slot` of the list goes into the inventory.
void VIClientPart3FC10::Caught(VIMemStream &s)
{
    VINetItem item;
    int slot, x;
    s.Read(&slot, 4, 1);
    s.Read(&x, 4, 1);
    m_catch.Get(slot, item);
    item.m_equipSlot = -1;
    int n = m_client->m_inventory.AddItem(item, item.m_maxStack, 40);
    m_catch.Took(slot, n);
    m_state = 3;
}

// 0x00BEBB88
void VIClientPart3FC10::Waiting()
{
    m_state = 3;
}

// 0x00BEBB98
void VIClientPart3FC10::Loot(VIMemStream &s)
{
    VIUI &ui = m_client->m_ui;
    m_catch.Read(s);
    m_wnd->FishingA();
    m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1434, ui.m_language));
    m_state = 3;
}

// 0x00BEBC00
void VIClientPart3FC10::Missed()
{
    VIUI &ui = m_client->m_ui;
    m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1435, ui.m_language));
    m_state = 1;
}

// 0x00BEBC48
void VIClientPart3FC10::Lost()
{
    VIUI &ui = m_client->m_ui;
    m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1436, ui.m_language));
    m_state = 1;
}

// 0x00BEBC90
void VIClientPart3FC10::Damaged()
{
    VIUI &ui = m_client->m_ui;
    m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1437, ui.m_language));
    if (HasEquipped(20))
    {
        VINetItem *pole = FindEquipped(14);
        if (pole && pole->m_curHp <= 0)
            m_wnd->AddText((const unsigned short *)ui.m_words.GetString(1437, ui.m_language));
    }
    m_state = 1;
}

// 0x00BEBD30
void VIClientPart3FC10::Bite()
{
    m_wnd->FishingB();
    m_state = 1;
}

// 0x00BEBD60
void VIClientPart3FC10::Cast(int quality)
{
    VIUI &ui = m_client->m_ui;
    if (!HasEquipped(20))
    {
        m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1464, ui.m_language));
        return;
    }
    if (!HasEquipped(21))
    {
        m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1465, ui.m_language));
        return;
    }
    VINetItem *pole = FindEquipped(14);
    if (pole && pole->m_curHp <= 0)
    {
        m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1466, ui.m_language));
        return;
    }
    if (!quality)
        m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1433, ui.m_language));
    else
        m_wnd->FishingC((const unsigned short *)ui.m_words.GetString(1432, ui.m_language));
    {
        VIMemStream out;
        unsigned short o = 3841;
        out.Write(&o, 2, 1);
        out.WriteByte(1);
        out.WriteU32(quality);
        VINetSend(out, m_client->GetConn(251));
    }
    m_state = 2;
}

// 0x00BEBF18
void VIClientPart3FC10::Cancel()
{
    VIMemStream out;
    unsigned short o = 3841;
    out.Write(&o, 2, 1);
    out.WriteByte(2);
    VINetSend(out, m_client->GetConn(251));
}

// 0x00BEBFE8
void VIClientPart3FC10::Reel()
{
    VIMemStream out;
    unsigned short o = 3841;
    out.Write(&o, 2, 1);
    out.WriteByte(3);
    VINetSend(out, m_client->GetConn(251));
}

// 0x00BEC0B8
void VIClientPart3FC10::Pull()
{
    VIMemStream out;
    unsigned short o = 3841;
    out.Write(&o, 2, 1);
    out.WriteByte(4);
    VINetSend(out, m_client->GetConn(251));
}

// 0x00BEC188
void VIClientPart3FC10::Keep(int slot, int x)
{
    if (m_state == 4)
        return;
    int id = m_catch.GetSlotId(slot);
    {
        VIMemStream out;
        unsigned short o = 3841;
        out.Write(&o, 2, 1);
        out.WriteByte(5);
        out.WriteU32(id);
        out.WriteU32(x);
        VINetSend(out, m_client->GetConn(251));
    }
    m_state = 4;
}

// 0x00BEC2B8
int VIClientPart3FC10::HasEquipped(int wear)
{
    VIItemList &inv = m_client->m_inventory;
    for (int i = 0; i < inv.Count(); i++)
    {
        VINetItem *it = inv.Item(i);
        if (it && it->m_wearSlot == wear && it->m_equipSlot != -1)
            return 1;
    }
    return 0;
}

// 0x00BEC358
VINetItem *VIClientPart3FC10::FindEquipped(int equip)
{
    VIItemList &inv = m_client->m_inventory;
    for (int i = 0; i < inv.Count(); i++)
    {
        VINetItem *it = inv.Item(i);
        if (it && it->m_equipSlot == equip)
            return it;
    }
    return 0;
}
