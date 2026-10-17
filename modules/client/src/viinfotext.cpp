#line 1 "C:/code-frontiers-gm/game/VIInfoText.cpp"
// VIInfoText (name ours) -- CLIENT 0x00C05418 .. 0x00C0A438, then its VIMap<int, 4-byte> and
// VIArray<8-byte> template copies to 0x00C0B008.  The wide description of an item, a
// spell, a stat, built with snprintf16/strcat16 out of WORDS.TXT through the client's
// accessors.  Data: 0x00CDFB58 .. 0x00CDFF3A (the format strings), then bad_alloc.
#include "game.h"

extern "C" void *memset(void *, int, int);

// 0x00C05418.
VIInfoText::VIInfoText()
{
    m_client = 0;
}

// 0x00C05428.
VIInfoText::~VIInfoText()
{
}

// 0x00C05450.
int VIInfoText::SetClient(VIClient *c)
{
    m_client = c;
    return 0;
}



// The inventory's describers (0x00C05460 ..): item i of the player's inventory, read
// through the client's accessors, into dst.  -1 when there is no client or no such item.

// 0x00C05460: an item with charges.
int VIInfoText::DescribeInvUsable(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    VIUI &ui = m_client->m_ui;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    m_client->GetInvItemName(i, line, 512);
    VIString::snprintf16(dst, size, "%S\n \n", line);
    int n = m_client->GetInvItemCharges(i);
    if (n > 0)
    {
        AddCharges(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetInvItemAmount(i);
    if (m_client->GetInvItemMaxStack(i) >= 2)
    {
        AddStack(dst, size, n, m_client->GetInvItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    AddRaceMask(dst, size, m_client->GetInvItemRaceMask(i));
    VIString::strcat16(dst, size, "\n");
    AddClassMask(dst, size, m_client->GetInvItemClassMask(i));
    VIString::strcat16(dst, size, "\n");
    AddLevel(dst, size, m_client->GetInvItemLevelReq(i));
    VIString::strcat16(dst, size, "\n");
    if (m_client->GetInvItemNoTrade(i) > 0)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!m_client->GetInvItemRent(i))
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetInvItemLore(i) > 0)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetInvItemNoTrade(i) || !m_client->GetInvItemRent(i) || m_client->GetInvItemLore(i))
        VIString::strcat16(dst, size, "\n");
    m_client->GetInvItemDesc(i, line, 512);
    VIString::strcat16(dst, size, line);
    return 0;
}

// 0x00C057A8: equipment, in full.
int VIInfoText::DescribeInvEquipment(unsigned short *dst, unsigned int size, int i)
{
    memset(dst, 0, size);
    if (!m_client)
        return -1;
    return DescribeInvItem(dst, size, i, 1);
}

// 0x00C05828: the name, the stat bonuses, the description.
int VIInfoText::DescribeInvStats(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    m_client->GetInvItemName(i, line, 512);
    VIString::snprintf16(dst, size, "%S\n \n", line);
    VIArray<VIItemStat> *stats = m_client->GetInvItemStats(i);
    AddStats(dst, size, i, -1, 1, 0);
    if (stats->m_size > 0)
        VIString::strcat16(dst, size, " \n");
    m_client->GetInvItemDesc(i, line, 512);
    VIString::strcat16(dst, size, line);
    return 0;
}

// 0x00C05950: the charges, the masks and the level only for an item with a type.
int VIInfoText::DescribeInvMisc(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    VIUI &ui = m_client->m_ui;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    int typed = m_client->GetInvItemP4C(i) != 0;
    m_client->GetInvItemName(i, line, 512);
    VIString::snprintf16(dst, size, "%S\n \n", line);
    int n = m_client->GetInvItemCharges(i);
    if (n > 0 && typed)
    {
        AddCharges(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetInvItemAmount(i);
    if (m_client->GetInvItemMaxStack(i) >= 2)
    {
        AddStack(dst, size, n, m_client->GetInvItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    if (typed)
    {
        AddRaceMask(dst, size, m_client->GetInvItemRaceMask(i));
        VIString::strcat16(dst, size, "\n");
        AddClassMask(dst, size, m_client->GetInvItemClassMask(i));
        int level = m_client->GetInvItemLevelReq(i);
        if (level > 0)
        {
            VIString::strcat16(dst, size, "\n");
            AddLevel(dst, size, level);
        }
        VIString::strcat16(dst, size, "\n");
    }
    if (m_client->GetInvItemNoTrade(i) > 0)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!m_client->GetInvItemRent(i))
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetInvItemLore(i) > 0)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetInvItemNoTrade(i) || !m_client->GetInvItemRent(i) || m_client->GetInvItemLore(i))
        VIString::strcat16(dst, size, "\n");
    m_client->GetInvItemDesc(i, line, 512);
    VIString::strcat16(dst, size, line);
    return 0;
}

// 0x00C05CC8: the stack and the HP.
int VIInfoText::DescribeInvCharged(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    VIUI &ui = m_client->m_ui;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    m_client->GetInvItemName(i, line, 512);
    VIString::snprintf16(dst, size, "%S\n \n", line);
    int amount = m_client->GetInvItemAmount(i);
    if (m_client->GetInvItemMaxStack(i) >= 2)
    {
        AddStack(dst, size, amount, m_client->GetInvItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    int hp = m_client->GetInvItemCurHp(i);
    int max = m_client->GetInvItemMaxHp(i);
    if (max > 0)
    {
        AddHitPoints(dst, size, hp, max);
        VIString::strcat16(dst, size, "\n");
    }
    if (m_client->GetInvItemNoTrade(i) > 0)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!m_client->GetInvItemRent(i))
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetInvItemLore(i) > 0)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetInvItemNoTrade(i) || !m_client->GetInvItemRent(i) || m_client->GetInvItemLore(i))
        VIString::strcat16(dst, size, "\n");
    m_client->GetInvItemDesc(i, line, 512);
    VIString::strcat16(dst, size, line);
    return 0;
}

// 0x00C05F88: the same, short, ending "Inspect for more info".
int VIInfoText::DescribeInvChargedShort(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    VIUI &ui = m_client->m_ui;
    m_client->GetInvItemName(i, line, 512);
    VIString::snprintf16(dst, size, "%S\n \n", line);
    int n = m_client->GetInvItemAmount(i);
    if (m_client->GetInvItemMaxStack(i) >= 2)
    {
        AddStack(dst, size, n, m_client->GetInvItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetInvItemCurHp(i);
    int max = m_client->GetInvItemMaxHp(i);
    if (max > 0)
    {
        AddHitPoints(dst, size, n, max);
        VIString::strcat16(dst, size, "\n");
    }
    VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(492, ui.m_language));
    return 0;
}

// 0x00C06110: the player's HP and power, then the item (food, drink: its charges and stack).
int VIInfoText::DescribeInvConsumable(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    VIUI &ui = m_client->m_ui;
    int a = m_client->GetHP();
    int b = m_client->GetMaxHP();
    VIString::snprintf16(dst, size, "%S %d/%d\n", ui.m_words.GetString(233, ui.m_language), a, b);
    a = m_client->GetPower();
    b = m_client->GetMaxPower();
    VIString::snprintf16(line, 512, "%S %d/%d\n \n", ui.m_words.GetString(235, ui.m_language), a, b);
    VIString::strcat16(dst, size, line);
    m_client->GetInvItemName(i, line, 512);
    VIString::strcat16(dst, size, line);
    VIString::strcat16(dst, size, "\n \n");
    a = m_client->GetInvItemCharges(i);
    if (a > 0)
    {
        AddCharges(dst, size, a);
        VIString::strcat16(dst, size, "\n");
    }
    a = m_client->GetInvItemAmount(i);
    if (m_client->GetInvItemMaxStack(i) >= 2)
    {
        AddStack(dst, size, a, m_client->GetInvItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(492, ui.m_language));
    return 0;
}

// 0x00C06350: a weapon or armour, short: slot, HP, durability, damage, attack, range, stack,
// six stat lines, "Inspect for more info".
int VIInfoText::DescribeInvEquipmentShort(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    VIUI &ui = m_client->m_ui;
    m_client->GetInvItemName(i, line, 512);
    VIString::snprintf16(dst, size, "%S\n \n", line);
    AddEquipSlot(dst, size, m_client->GetInvItemWearSlot(i));
    VIString::strcat16(dst, size, "\n");
    int n = m_client->GetInvItemCurHp(i);
    AddHitPoints(dst, size, n, m_client->GetInvItemMaxHp(i));
    VIString::strcat16(dst, size, "\n");
    AddDur(dst, size, m_client->GetInvItemDurability(i));
    VIString::strcat16(dst, size, "\n");
    n = m_client->GetInvItemDamage(i);
    if (n > 0)
    {
        AddDamage(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetInvItemAttackType(i);
    if (n)
    {
        AddAttackType(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    if (m_client->IsRangedItem(i, 0))
    {
        n = m_client->GetInvItemRange(i);
        if (n > 0)
        {
            AddRange(dst, size, n);
            VIString::strcat16(dst, size, "\n");
        }
    }
    n = m_client->GetInvItemAmount(i);
    if (m_client->GetInvItemMaxStack(i) >= 2)
    {
        AddStack(dst, size, n, m_client->GetInvItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    AddStats(dst, size, i, 0, 1, 0);
    VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(492, ui.m_language));
    return 0;
}


// 0x00C06628: food: the food value and the stack.
int VIInfoText::DescribeInvFood(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    VIUI &ui = m_client->m_ui;
    m_client->GetInvItemName(i, line, 512);
    VIString::snprintf16(dst, size, "%S\n \n", line);
    int n = m_client->GetInvItemP58(i);
    if (n > 0)
    {
        AddFood(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetInvItemAmount(i);
    if (m_client->GetInvItemMaxStack(i) >= 2)
    {
        AddStack(dst, size, n, m_client->GetInvItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(492, ui.m_language));
    return 0;
}

// 0x00C067A8: item i of list 0 (the inventory) or 1 (the merchant's).
int VIInfoText::DescribeListItem(unsigned short *dst, unsigned int size, int i, int list)
{
    memset(dst, 0, size);
    if (!m_client)
        return -1;
    switch (list)
    {
    case 0:
        return DescribeInvItem(dst, size, i, 0);
    case 1:
        return DescribeMerchantItem(dst, size, i);
    }
    return -1;
}

// 0x00C06868: item i of list 0 (the inventory) or 2 (the bank).
int VIInfoText::DescribeBankListItem(unsigned short *dst, unsigned int size, int i, int list)
{
    memset(dst, 0, size);
    if (!m_client)
        return -1;
    switch (list)
    {
    case 0:
        return DescribeInvItemQuick(dst, size, i);
    case 2:
        return DescribeBankItem(dst, size, i);
    }
    return -1;
}

// 0x00C06920.
int VIInfoText::DescribeInvItemShort(unsigned short *dst, unsigned int size, int i)
{
    memset(dst, 0, size);
    if (!m_client)
        return -1;
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    DescribeInvItem(dst, size, i, 0);
    return 0;
}

// 0x00C069B8: item i of the corpse being looted -- the name straight into dst.
int VIInfoText::DescribeLootItem(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[128];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    VIUI &ui = m_client->m_ui;
    if (i < 0 || i >= m_client->GetLootCount())
        return -1;
    m_client->GetLootItemName(i, dst, size);
    if (m_client->GetLootItemP4C(i) || m_client->GetLootItemWearSlot(i) != -1)
    {
        VIString::strcat16(dst, size, "\n");
        AddRaceMask(dst, size, m_client->GetLootItemRaceMask(i));
        VIString::strcat16(dst, size, "\n");
        AddClassMask(dst, size, m_client->GetLootItemClassMask(i));
        VIString::strcat16(dst, size, "\n");
        AddLevel(dst, size, m_client->GetLootItemLevelReq(i));
    }
    if (m_client->GetLootItemNoTrade(i) || !m_client->GetLootItemRent(i) || m_client->GetLootItemLore(i))
        VIString::strcat16(dst, size, "\n");
    if (m_client->GetLootItemNoTrade(i) > 0)
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!m_client->GetLootItemRent(i))
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetLootItemLore(i) > 0)
    {
        VIString::snprintf16(line, 256, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    return 0;
}

// 0x00C06C50: an item record the caller holds.
int VIInfoText::DescribeItem(unsigned short *dst, unsigned int size, VIItem *item)
{
    unsigned short line[256];

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    VIUI &ui = m_client->m_ui;
    if (!item)
        return -1;
    VIString::strcpy16(dst, size, item->m_name);
    if (item->m_p4C || item->m_wearSlot != -1)
    {
        VIString::strcat16(dst, size, "\n");
        AddRaceMask(dst, size, item->m_raceMask);
        VIString::strcat16(dst, size, "\n");
        AddClassMask(dst, size, item->m_classMask);
        VIString::strcat16(dst, size, "\n");
        AddLevel(dst, size, item->m_levelReq);
    }
    if (item->m_noTrade || !item->m_rent || item->m_lore)
        VIString::strcat16(dst, size, "\n");
    if (item->m_noTrade)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!item->m_rent)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_lore)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    return 0;
}

// 0x00C06E70: item i of list 4 (the player's side of a trade) or 5 / 6 (the two offers).
int VIInfoText::DescribeTradeItem(unsigned short *dst, unsigned int size, int i, int list)
{
    memset(dst, 0, size);
    if (!m_client)
        return -1;
    switch (list)
    {
    case 4:
        return DescribeTradeInvItem(dst, size, i);
    case 5:
        return DescribeTradeOffer(dst, size, i, 1);
    case 6:
        return DescribeTradeOffer(dst, size, i, 0);
    }
    return -1;
}

// 0x00C06F70: item i of the merchant's list.
int VIInfoText::DescribeMerchantItem(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[128];
    VIUI &ui = m_client->m_ui;

    if (i < 0 || i >= m_client->GetMerchantCount())
        return -1;
    m_client->GetMerchantItemName(i, line, 256);
    VIString::snprintf16(dst, size, "%S\n", line);
    int n = m_client->GetMerchantItemDamage(i);
    if (n > 0)
    {
        AddDamage(dst, size, n);
        VIString::strcat16(dst, size, " ");
    }
    n = m_client->GetMerchantItemAttackType(i);
    if (n)
    {
        AddAttackType(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetMerchantItemWearSlot(i);
    if (n != -1)
    {
        AddEquipSlot(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    if (m_client->IsRangedItem(i, 1))
    {
        n = m_client->GetMerchantItemRange(i);
        if (n > 0)
        {
            AddRange(dst, size, n);
            VIString::strcat16(dst, size, "\n");
        }
    }
    n = m_client->GetMerchantItemMaxHp(i);
    if (n)
    {
        AddHp(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetMerchantItemDurability(i);
    if (n)
    {
        AddDur(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    if (m_client->GetMerchantItemMaxStack(i) >= 2)
    {
        AddMaxStack(dst, size, m_client->GetMerchantItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetMerchantItemCharges(i);
    if (n > 0)
    {
        AddCharges(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    AddRaceMask(dst, size, m_client->GetMerchantItemRaceMask(i));
    VIString::strcat16(dst, size, "\n");
    AddClassMask(dst, size, m_client->GetMerchantItemClassMask(i));
    n = m_client->GetMerchantItemLevelReq(i);
    if (n > 0 && (m_client->GetMerchantItemWearSlot(i) != -1 || m_client->GetMerchantItemP4C(i)))
    {
        VIString::strcat16(dst, size, "\n");
        AddLevel(dst, size, n);
    }
    if (m_client->GetMerchantItemNoTrade(i) || !m_client->GetMerchantItemRent(i) || m_client->GetMerchantItemLore(i))
        VIString::strcat16(dst, size, "\n");
    if (m_client->GetMerchantItemNoTrade(i) > 0)
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!m_client->GetMerchantItemRent(i))
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetMerchantItemLore(i) > 0)
    {
        VIString::snprintf16(line, 256, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    return 0;
}

// 0x00C073F8: item i of the inventory, in full when flags & 1 (the stack instead of the
// max stack, no charges).  An int of FLAGS, tested by bit: `!(flags & 1)` is the target's
// xori/andi pair, and a bool parameter loses the andi of the first test.
int VIInfoText::DescribeInvItem(unsigned short *dst, unsigned int size, int i, int flags)
{
    unsigned short line[256];
    VIUI &ui = m_client->m_ui;

    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    m_client->GetInvItemName(i, line, 512);
    if (flags & 1)
        VIString::snprintf16(dst, size, "%S\n \n", line);
    else
        VIString::snprintf16(dst, size, "%S\n", line);
    int n = m_client->GetInvItemDamage(i);
    if (n > 0)
    {
        AddDamage(dst, size, n);
        VIString::strcat16(dst, size, " ");
    }
    n = m_client->GetInvItemAttackType(i);
    if (n)
    {
        AddAttackType(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetInvItemWearSlot(i);
    if (n != -1)
    {
        AddEquipSlot(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    if (m_client->IsRangedItem(i, 0))
    {
        n = m_client->GetInvItemRange(i);
        if (n > 0)
        {
            AddRange(dst, size, n);
            VIString::strcat16(dst, size, "\n");
        }
    }
    n = m_client->GetInvItemCurHp(i);
    int max = m_client->GetInvItemMaxHp(i);
    if (max)
    {
        AddHitPoints(dst, size, n, max);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetInvItemDurability(i);
    if (n)
    {
        AddDur(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    if (flags & 1)
    {
        n = m_client->GetInvItemAmount(i);
        if (m_client->GetInvItemMaxStack(i) >= 2)
        {
            AddStack(dst, size, n, m_client->GetInvItemMaxStack(i));
            VIString::strcat16(dst, size, "\n");
        }
    }
    else if (m_client->GetInvItemMaxStack(i) >= 2)
    {
        AddMaxStack(dst, size, m_client->GetInvItemMaxStack(i));
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetInvItemCharges(i);
    if (n > 0 && !(flags & 1))
    {
        AddCharges(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    AddRaceMask(dst, size, m_client->GetInvItemRaceMask(i));
    VIString::strcat16(dst, size, "\n");
    AddClassMask(dst, size, m_client->GetInvItemClassMask(i));
    n = m_client->GetInvItemLevelReq(i);
    if (n > 0 && (m_client->GetInvItemWearSlot(i) != -1 || m_client->GetInvItemP4C(i)))
    {
        VIString::strcat16(dst, size, "\n");
        AddLevel(dst, size, n);
    }
    if (m_client->GetInvItemNoTrade(i) || !m_client->GetInvItemRent(i) || m_client->GetInvItemLore(i))
        VIString::strcat16(dst, size, "\n");
    if (m_client->GetInvItemNoTrade(i) > 0)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!m_client->GetInvItemRent(i))
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetInvItemLore(i) > 0)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    return 0;
}

// 0x00C07930.
int VIInfoText::DescribeInvItemQuick(unsigned short *dst, unsigned int size, int i)
{
    if (i < 0 || i >= m_client->GetInvCount())
        return -1;
    DescribeInvItem(dst, size, i, 0);
    return 0;
}


// 0x00C079B0: item i of the bank: the record itself, a VINetItem.
int VIInfoText::DescribeBankItem(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[128];
    VIUI &ui = m_client->m_ui;
    VIBankControl *bank = m_client->GetBank();

    if (i < 0 || i >= bank->Count())
        return -1;
    VINetItem *item = bank->Item(i);
    VIString::snprintf16(dst, size, "%S\n", item->m_name);
    int n = item->m_damage;
    if (n > 0)
    {
        AddDamage(dst, size, n);
        VIString::strcat16(dst, size, " ");
    }
    n = item->m_attackType;
    if (n)
    {
        AddAttackType(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = item->m_wearSlot;
    if (n != -1)
    {
        AddEquipSlot(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    if (m_client->IsRangedItem(i, 2))
    {
        n = item->m_range;
        if (n > 0)
        {
            AddRange(dst, size, n);
            VIString::strcat16(dst, size, "\n");
        }
    }
    n = item->m_curHp;
    if (item->m_maxHp)
    {
        AddHitPoints(dst, size, n, item->m_maxHp);
        VIString::strcat16(dst, size, "\n");
    }
    n = item->m_durability;
    if (n)
    {
        AddDur(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_maxStack >= 2)
    {
        AddMaxStack(dst, size, item->m_maxStack);
        VIString::strcat16(dst, size, "\n");
    }
    n = item->m_charges;
    if (n > 0)
    {
        AddCharges(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    AddRaceMask(dst, size, item->m_raceMask);
    VIString::strcat16(dst, size, "\n");
    AddClassMask(dst, size, item->m_classMask);
    n = item->m_levelReq;
    if (n > 0 && (item->m_wearSlot != -1 || item->m_p4C))
    {
        VIString::strcat16(dst, size, "\n");
        AddLevel(dst, size, n);
    }
    if (item->m_noTrade || !item->m_rent || item->m_lore)
        VIString::strcat16(dst, size, "\n");
    n = item->m_noTrade;
    if (n > 0)
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    n = item->m_rent;
    if (!n)
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    n = item->m_lore;
    if (n > 0)
    {
        VIString::snprintf16(line, 256, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    return 0;
}

// 0x00C07D90: item i of the player's side of a trade.
int VIInfoText::DescribeTradeInvItem(unsigned short *dst, unsigned int size, int i)
{
    unsigned short line[128];
    VIUI &ui = m_client->m_ui;

    if (i < 0 || i >= m_client->GetTradeInvCount())
        return -1;
    m_client->GetTradeInvItemName(i, line, 256);
    VIString::snprintf16(dst, size, "%S\n", line);
    int n = m_client->GetTradeInvItemAttackType(i);
    if (n)
    {
        AddAttackType(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    int none = -1;
    n = m_client->GetTradeInvItemWearSlot(i);
    if (n != none)
    {
        AddEquipSlot(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetTradeInvItemCurHp(i);
    int max = m_client->GetTradeInvItemMaxHp(i);
    if (max)
    {
        AddHitPoints(dst, size, n, max);
        VIString::strcat16(dst, size, "\n");
    }
    AddRaceMask(dst, size, m_client->GetTradeInvItemRaceMask(i));
    VIString::strcat16(dst, size, "\n");
    AddClassMask(dst, size, m_client->GetTradeInvItemClassMask(i));
    n = m_client->GetTradeInvItemLevelReq(i);
    if (n > 0 && (m_client->GetTradeInvItemWearSlot(i) != none || m_client->GetTradeInvItemP4C(i)))
    {
        VIString::strcat16(dst, size, "\n");
        AddLevel(dst, size, n);
    }
    if (m_client->GetTradeInvItemNoTrade(i) || !m_client->GetTradeInvItemRent(i) || m_client->GetTradeInvItemLore(i))
        VIString::strcat16(dst, size, "\n");
    if (m_client->GetTradeInvItemNoTrade(i) > 0)
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!m_client->GetTradeInvItemRent(i))
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetTradeInvItemLore(i) > 0)
    {
        VIString::snprintf16(line, 256, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    return 0;
}

// 0x00C080F0: item i of a trade offer (side 1 the other player's, 0 the player's).
int VIInfoText::DescribeTradeOffer(unsigned short *dst, unsigned int size, int i, int side)
{
    unsigned short line[128];
    VIUI &ui = m_client->m_ui;

    if (i < 0 || i >= m_client->GetTradeCount(side))
        return -1;
    m_client->GetTradeOfferName(side, i, line, 256);
    VIString::snprintf16(dst, size, "%S\n", line);
    int n = m_client->GetTradeOfferAttackType(side, i);
    if (n)
    {
        AddAttackType(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetTradeOfferWearSlot(side, i);
    if (n != -1)
    {
        AddEquipSlot(dst, size, n);
        VIString::strcat16(dst, size, "\n");
    }
    n = m_client->GetTradeOfferCurHp(side, i);
    int max = m_client->GetTradeOfferMaxHp(side, i);
    if (max)
    {
        AddHitPoints(dst, size, n, max);
        VIString::strcat16(dst, size, "\n");
    }
    AddRaceMask(dst, size, m_client->GetTradeOfferRaceMask(side, i));
    VIString::strcat16(dst, size, "\n");
    AddClassMask(dst, size, m_client->GetTradeOfferClassMask(side, i));
    n = m_client->GetTradeOfferLevelReq(side, i);
    if (n > 0 && (m_client->GetTradeOfferWearSlot(side, i) != -1 || m_client->GetTradeOfferP4C(side, i)))
    {
        VIString::strcat16(dst, size, "\n");
        AddLevel(dst, size, n);
    }
    if (m_client->GetTradeOfferNoTrade(side, i) || !m_client->GetTradeOfferRent(side, i) || m_client->GetTradeOfferLore(side, i))
        VIString::strcat16(dst, size, "\n");
    if (m_client->GetTradeOfferNoTrade(side, i) > 0)
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!m_client->GetTradeOfferRent(side, i))
    {
        VIString::snprintf16(line, 256, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (m_client->GetTradeOfferLore(side, i) > 0)
    {
        VIString::snprintf16(line, 256, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    return 0;
}


// 0x00C08498: the recipe being made: the product, its ingredients and tools, its level, its
// rarity, its stat bonuses and its description.
int VIInfoText::DescribeRecipe(unsigned short *dst, unsigned int size)
{
    unsigned short line[256];
    int i;

    memset(dst, 0, size);
    if (!m_client)
        return -1;
    VIUI &ui = m_client->m_ui;
    VICraftDetail *r = &m_client->GetP3834C()->m_detail;
    if (!r)
        return -1;
    if (r->m_count == 1)
        VIString::snprintf16(dst, size, "%S\n", r->m_name);
    else
        VIString::snprintf16(dst, size, "%S (%d)\n", r->m_name, r->m_count);
    if (r->m_ingredients.m_size > 0)
    {
        VIString::snprintf16(line, 512, "%S:\n", ui.m_words.GetString(1571, ui.m_language));
        VIString::strcat16(dst, size, line);
        for (i = r->m_ingredients.Begin(); i != r->m_ingredients.End(); i = r->m_ingredients.Next(i))
        {
            VIRecipeIngredient &g = r->m_ingredients[i];
            if (g.m_count)
            {
                VIString::snprintf16(line, 512, "  %S (%d)\n", g.m_name, g.m_count);
                VIString::strcat16(dst, size, line);
            }
        }
    }
    if (r->m_tools.m_size > 0)
    {
        VIString::snprintf16(line, 512, "%S:\n", ui.m_words.GetString(1572, ui.m_language));
        VIString::strcat16(dst, size, line);
        for (i = r->m_tools.Begin(); i != r->m_tools.End(); i = r->m_tools.Next(i))
        {
            VIRecipeTool &o = r->m_tools[i];
            if (o.m_have)
                VIString::snprintf16(line, 512, "  %S (%S)\n", o.m_name, ui.m_words.GetString(1573, ui.m_language));
            else
                VIString::snprintf16(line, 512, "  %S\n", o.m_name);
            VIString::strcat16(dst, size, line);
        }
    }
    VIString::snprintf16(line, 512, "%S: %d\n", ui.m_words.GetString(1574, ui.m_language), r->m_level);
    VIString::strcat16(dst, size, line);
    switch (r->m_rarity)
    {
    case 0:
        VIString::snprintf16(line, 512, "%S: %S\n", ui.m_words.GetString(1542, ui.m_language),
                             ui.m_words.GetString(1538, ui.m_language));
        VIString::strcat16(dst, size, line);
        break;
    case 1:
        VIString::snprintf16(line, 512, "%S: %S\n", ui.m_words.GetString(1542, ui.m_language),
                             ui.m_words.GetString(1539, ui.m_language));
        VIString::strcat16(dst, size, line);
        break;
    case 2:
        VIString::snprintf16(line, 512, "%S: %S\n", ui.m_words.GetString(1542, ui.m_language),
                             ui.m_words.GetString(1540, ui.m_language));
        VIString::strcat16(dst, size, line);
        break;
    case 3:
        VIString::snprintf16(line, 512, "%S: %S\n", ui.m_words.GetString(1542, ui.m_language),
                             ui.m_words.GetString(1541, ui.m_language));
        VIString::strcat16(dst, size, line);
        break;
    }
    if (r->m_stats.m_size > 0)
    {
        VIString::snprintf16(line, 512, "%S:\n", ui.m_words.GetString(1575, ui.m_language));
        VIString::strcat16(dst, size, line);
        AddStats(dst, size, 0, 7, 1, 0);
    }
    if (r->m_desc)
        VIString::strcat16(dst, size, r->m_desc);
    return 0;
}

// The auction list's describers (0x00C08970 ..): the record's own fields, not the
// client's accessors.  DescribeAuctionItem picks one by the item's kind.

// 0x00C08970.
int VIInfoText::DescribeAuctionItem(unsigned short *dst, unsigned int size, VIAuctionItem *item)
{
    memset(dst, 0, size);
    if (!m_client)
        return -1;
    if (IsWearable(item))
        return DescribeAuctionEquipment(dst, size, item);
    if (IsMisc(item))
        return DescribeAuctionMisc(dst, size, item);
    if (IsUsable(item))
        return DescribeAuctionUsable(dst, size, item);
    if (IsCharged(item))
        return DescribeAuctionCharged(dst, size, item);
    return -1;
}

// 0x00C08A70.
int VIInfoText::DescribeAuctionEquipment(unsigned short *dst, unsigned int size, VIAuctionItem *item)
{
    unsigned short line[256];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(dst, size, "%S\n\n", item->m_name);
    if (item->m_damage > 0)
    {
        AddDamage(dst, size, item->m_damage);
        VIString::strcat16(dst, size, " ");
    }
    if (item->m_attackType)
    {
        AddAttackType(dst, size, item->m_attackType);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_wearSlot != -1)
    {
        AddEquipSlot(dst, size, item->m_wearSlot);
        VIString::strcat16(dst, size, "\n");
    }
    if (IsRanged(item))
    {
        if (item->m_range > 0)
        {
            AddRange(dst, size, item->m_range);
            VIString::strcat16(dst, size, "\n");
        }
    }
    if (item->m_maxHp > 0)
    {
        AddHitPoints(dst, size, item->m_curHp, item->m_maxHp);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_durability)
    {
        AddDur(dst, size, item->m_durability);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_maxStack >= 2)
    {
        AddStack(dst, size, item->m_quantity, item->m_maxStack);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_charges > 0)
    {
        AddCharges(dst, size, item->m_charges);
        VIString::strcat16(dst, size, "\n");
    }
    AddRaceMask(dst, size, item->m_raceMask);
    VIString::strcat16(dst, size, "\n");
    AddClassMask(dst, size, item->m_classMask);
    VIString::strcat16(dst, size, "\n");
    AddLevel(dst, size, item->m_levelReq);
    VIString::strcat16(dst, size, "\n");
    if (item->m_noTrade > 0)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!item->m_rent)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_lore > 0)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_noTrade || item->m_lore || !item->m_rent)
        VIString::strcat16(dst, size, "\n");
    if (item->m_stats.m_size > 0)
    {
        AddStats(dst, size, 0, 8, 1, item);
        VIString::strcat16(dst, size, " \n");
    }
    VIString::strcat16(dst, size, item->m_desc);
    return 0;
}

// 0x00C08E20.
int VIInfoText::DescribeAuctionMisc(unsigned short *dst, unsigned int size, VIAuctionItem *item)
{
    unsigned short line[256];
    VIUI &ui = m_client->m_ui;
    int typed = item->m_p4C != 0;

    VIString::snprintf16(dst, size, "%S\n\n", item->m_name);
    if (item->m_maxStack >= 2)
    {
        AddStack(dst, size, item->m_quantity, item->m_maxStack);
        VIString::strcat16(dst, size, "\n");
    }
    if (typed)
    {
        if (item->m_charges > 0)
        {
            AddCharges(dst, size, item->m_charges);
            VIString::strcat16(dst, size, "\n");
        }
        AddRaceMask(dst, size, item->m_raceMask);
        VIString::strcat16(dst, size, "\n");
        AddClassMask(dst, size, item->m_classMask);
        VIString::strcat16(dst, size, "\n");
        AddLevel(dst, size, item->m_levelReq);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_noTrade > 0)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!item->m_rent)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_lore > 0)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_noTrade || item->m_lore || !item->m_rent)
        VIString::strcat16(dst, size, "\n");
    VIString::strcat16(dst, size, item->m_desc);
    return 0;
}

// 0x00C09090.
int VIInfoText::DescribeAuctionUsable(unsigned short *dst, unsigned int size, VIAuctionItem *item)
{
    unsigned short line[256];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(dst, size, "%S\n\n", item->m_name);
    if (item->m_maxStack >= 2)
    {
        AddStack(dst, size, item->m_quantity, item->m_maxStack);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_charges > 0)
    {
        AddCharges(dst, size, item->m_charges);
        VIString::strcat16(dst, size, "\n");
    }
    AddRaceMask(dst, size, item->m_raceMask);
    VIString::strcat16(dst, size, "\n");
    AddClassMask(dst, size, item->m_classMask);
    VIString::strcat16(dst, size, "\n");
    AddLevel(dst, size, item->m_levelReq);
    VIString::strcat16(dst, size, "\n");
    if (item->m_noTrade > 0)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!item->m_rent)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_lore > 0)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_noTrade || item->m_lore || !item->m_rent)
        VIString::strcat16(dst, size, "\n");
    VIString::strcat16(dst, size, item->m_desc);
    return 0;
}

// 0x00C092F8.
int VIInfoText::DescribeAuctionCharged(unsigned short *dst, unsigned int size, VIAuctionItem *item)
{
    unsigned short line[256];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(dst, size, "%S\n\n", item->m_name);
    if (item->m_maxStack >= 2)
    {
        AddStack(dst, size, item->m_quantity, item->m_maxStack);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_maxHp > 0)
    {
        AddHitPoints(dst, size, item->m_curHp, item->m_maxHp);
        VIString::strcat16(dst, size, "\n");
    }
    if (item->m_noTrade > 0)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(553, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (!item->m_rent)
    {
        VIString::snprintf16(line, 512, "%S ", ui.m_words.GetString(554, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_lore > 0)
    {
        VIString::snprintf16(line, 512, "%S", ui.m_words.GetString(555, ui.m_language));
        VIString::strcat16(dst, size, line);
    }
    if (item->m_noTrade || item->m_lore || !item->m_rent)
        VIString::strcat16(dst, size, "\n");
    VIString::strcat16(dst, size, item->m_desc);
    return 0;
}

// The one-line helpers (0x00C094E8 ..): a word, a colon and a number appended to dst.
// Each formats into a 64-character line first.  The names are the words'.

// 0x00C094E8: "%S: %d" with word 503.
void VIInfoText::AddLevel(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(503, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09570: "%S: %d " with word 294.
void VIInfoText::AddRefire(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d ", ui.m_words.GetString(294, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C095F8: "%S: %d " with word 298.
void VIInfoText::AddWeight(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d ", ui.m_words.GetString(298, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09680: "%S: %S" with word 490 and the name FindWearSlotWord finds, else "%S: %d".
void VIInfoText::AddEquipSlot(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;
    VIWordId *w = FindWearSlotWord(v);

    if (w)
        VIString::snprintf16(line, 128, "%S: %S", ui.m_words.GetString(490, ui.m_language),
                             ui.m_words.GetString(w->m_word, ui.m_language));
    else
        VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(490, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09778: "%S: %S" with word 497 and the name FindAttackWord finds, else "%S: %d".
void VIInfoText::AddAttackType(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;
    VIWordId *w = FindAttackWord(v);

    if (w)
        VIString::snprintf16(line, 128, "%S: %S", ui.m_words.GetString(497, ui.m_language),
                             ui.m_words.GetString(w->m_word, ui.m_language));
    else
        VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(497, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09870: "%S: %d" with word 504.
void VIInfoText::AddRange(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(504, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C098F8: "%S: %d" with word 297.
void VIInfoText::AddCharges(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(297, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09980: "%S: %d/%d" with word 233.
void VIInfoText::AddHitPoints(unsigned short *dst, unsigned int size, int v, int max)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d/%d", ui.m_words.GetString(233, ui.m_language), v, max);
    VIString::strcat16(dst, size, line);
}

// 0x00C09A18: "%S: %d" with word 233.
void VIInfoText::AddHp(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(233, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09AA0: "%S: %d" with word 493.
void VIInfoText::AddDamage(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(493, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09B28: "%S: %d" with word 495.
void VIInfoText::AddDur(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(495, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09BB0: "%S: %d/%d" with word 491.
void VIInfoText::AddStack(unsigned short *dst, unsigned int size, int v, int max)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d/%d", ui.m_words.GetString(491, ui.m_language), v, max);
    VIString::strcat16(dst, size, line);
}

// 0x00C09C48: "%S: %d" with word 502.
void VIInfoText::AddMaxStack(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(502, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}

// 0x00C09CD0: "%S: %d" with word 11.
void VIInfoText::AddFood(unsigned short *dst, unsigned int size, int v)
{
    unsigned short line[64];
    VIUI &ui = m_client->m_ui;

    VIString::snprintf16(line, 128, "%S: %d", ui.m_words.GetString(11, ui.m_language), v);
    VIString::strcat16(dst, size, line);
}


// 0x00C09D58: an item's stat bonuses, a line each -- "Str: 5", and against what the player
// has now, "Str: 5 -> 7".  (index, source) pick the item: -1 and 0 the inventory, 1 the
// merchant, 2 the bank, 7 the recipe being made, 8 `item` itself.  Six lines at most
// unless `all`.
void VIInfoText::AddStats(unsigned short *dst, unsigned int size, int index, int source, int all,
                          VIAuctionItem *item)
{
    unsigned short line[256];
    VIArray<VIItemStat> *stats = 0;
    VIUI &ui = m_client->m_ui;

    switch (source)
    {
    case -1:
    case 0:
        if (index >= 0 && index < m_client->GetInvCount())
            stats = m_client->GetInvItemStats(index);
        break;
    case 1:
        if (index >= 0 && index < m_client->GetMerchantCount())
            stats = m_client->GetMerchantItemStats(index);
        break;
    case 2:
    {
        VIBankControl *bank = m_client->GetBank();
        if (index >= 0 && index < bank->Count())
            stats = &bank->Item(index)->m_stats;
        break;
    }
    case 7:
    {
        VICraftDetail *r = &m_client->GetP3834C()->m_detail;
        stats = &r->m_stats;
        if (r)
            break;
        return;
    }
    case 8:
        stats = item ? &item->m_stats : 0;
        break;
    default:
        return;
    }
    if (!stats)
        return;
    int i, n;
    for (i = stats->Begin(), n = 0; i != stats->End() && (all || n < 6); i = stats->Next(i), n++)
    {
        VIItemStat *s = &(*stats)[i];
        VIWordId *w = FindStatWord(s->m_id);
        if (w)
        {
            if (source == -1 || source == 8)
            {
                const char *name = ui.m_words.GetString(w->m_word, ui.m_language);
                VIString::snprintf16(line, 512, "%S: %d\n", name, s->m_value);
            }
            else if (source == 7)
            {
                const char *name = ui.m_words.GetString(w->m_word, ui.m_language);
                VIString::snprintf16(line, 512, "  %S: %d\n", name, s->m_value);
            }
            else
            {
                int to = m_client->StatWithItem(index, s->m_id, source);
                const char *name = ui.m_words.GetString(w->m_word, ui.m_language);
                VIString::snprintf16(line, 512, "%S: %d -> %d\n", name, s->m_value, to);
            }
        }
        else
            VIString::snprintf16(line, 512, "%d: %d\n", s->m_id, s->m_value);
        VIString::strcat16(dst, size, line);
    }
}

// 0x00C0A060: "Race: " then ALL, NONE, or the short name of every race in the
// mask, each followed by a space.
void VIInfoText::AddRaceMask(unsigned short *dst, unsigned int size, int mask)
{
    VIUI &ui = m_client->m_ui;
    int all = 1;
    int none = 1;

    int bit;
    for (int i = 0; i < 10; i++)
    {
        bit = 1 << i;
        if (bit & mask)
            none = 0;
        else
            all = 0;
    }
    VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(289, ui.m_language));
    VIString::strcat16(dst, size, ": ");
    if (all)
        VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(301, ui.m_language));
    else if (none)
        VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(302, ui.m_language));
    else
    {
        for (bit = 0; bit < 10; bit++)
        {
            const VIRaceDef *d = GetRace(bit);
            if ((mask >> bit) & 1)
            {
                VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(d->m_shortStrID, ui.m_language));
                VIString::strcat16(dst, size, " ");
            }
        }
        dst[VIString::strlen16(dst)] = 0;
    }
}

// 0x00C0A1E0: "Class: " then ALL, NONE, or the short name of every class in the
// mask, each followed by a space.
void VIInfoText::AddClassMask(unsigned short *dst, unsigned int size, int mask)
{
    VIUI &ui = m_client->m_ui;
    int all = 1;
    int none = 1;

    int bit;
    for (int i = 0; i < 15; i++)
    {
        bit = 1 << i;
        if (bit & mask)
            none = 0;
        else
            all = 0;
    }
    VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(288, ui.m_language));
    VIString::strcat16(dst, size, ": ");
    if (all)
        VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(301, ui.m_language));
    else if (none)
        VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(302, ui.m_language));
    else
    {
        for (bit = 0; bit < 15; bit++)
        {
            const VIClassDef *d = GetClass(bit);
            if ((mask >> bit) & 1)
            {
                VIString::strcat16(dst, size, (const unsigned short *)ui.m_words.GetString(d->m_shortStrID, ui.m_language));
                VIString::strcat16(dst, size, " ");
            }
        }
        dst[VIString::strlen16(dst)] = 0;
    }
}

// 0x00C0A360 .. 0x00C0A428: what kind of item the record is (by its wear slot, the type at
// +0x4C and the count at +0x54).  Names provisional.

// 0x00C0A360.
int VIInfoText::IsCharged(VIItem *item)
{
    return item->m_p54 > 0 && item->m_p4C != 8 && item->m_wearSlot == -1;
}

// 0x00C0A390.
int VIInfoText::IsWearable(VIItem *item)
{
    return item->m_wearSlot != -1;
}

// 0x00C0A3A0.
int VIInfoText::IsMisc(VIItem *item)
{
    return item->m_p4C != 7 && item->m_p4C != 2 && item->m_p4C != 5 && item->m_p4C != 8
        && !item->m_p54 && item->m_wearSlot == -1;
}

// 0x00C0A3F0.
int VIInfoText::IsUsable(VIItem *item)
{
    return item->m_p4C == 7 || item->m_p4C == 2 || item->m_p4C == 5 || item->m_p4C == 8;
}

// 0x00C0A428: attack types 7 .. 10.
int VIInfoText::IsRanged(VIItem *item)
{
    return (unsigned)(item->m_attackType - 7) < 4;
}
