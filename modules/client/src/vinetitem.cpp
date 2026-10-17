#line 1 "C:/code-frontiers-gm/game/VINetItem.cpp"
// VINetItem.cpp -- CLIENT 0x00CBA9D8 .. 0x00CBB6A8: an item and its stream operators.
// SetFrom (unreferenced) fills a VIItem from an item definition; its assert, on line 58,
// names the definition `pItem` and both stat arrays `Mods` (this tree's VIItem calls its
// own m_stats).  The VIArray<VIItemStat> readers at the end are VIMemStreamArray.h's.
#include "game.h"
#include "vimemstreamarray.h"

extern "C" void *memset(void *, int, int);
// (blank lines below pad the assert to its original line)


// 0x00CBA9D8
void VIItem::SetFrom(const VIItemDef *pItem)
{
    VIItemDef def;
    if (!pItem)
    {
        pItem = &def;
        memset(&def, 0, sizeof(def));
        def.m_wearSlot = -1;
        def.m_maxStack = 999999;
    }
    m_id = pItem->m_id;
    m_value = pItem->m_value;
    m_p08 = pItem->m_p08;
    m_icon = pItem->m_icon;
    m_icon2 = pItem->m_icon2;
    m_wearSlot = pItem->m_wearSlot;
    m_p18 = pItem->m_p18;
    m_noTrade = pItem->m_noTrade;
    m_rent = pItem->m_rent;
    m_p24 = pItem->m_p24;
    m_attackType = pItem->m_attackType;
    m_damage = pItem->m_damage;
    m_range = pItem->m_range;
    m_levelReq = pItem->m_levelReq;
    m_maxStack = pItem->m_maxStack;
    m_maxHp = pItem->m_maxHp;
    m_durability = pItem->m_durability;
    m_classMask = pItem->m_classMask;
    m_raceMask = pItem->m_raceMask;
    m_p4C = pItem->m_p4C;
    m_lore = pItem->m_lore;
    m_p58 = pItem->m_p58;
    m_p54 = pItem->m_p54;
    VIString::snprintf16(m_name, sizeof(m_name), "%i", pItem->m_id);
    m_desc[0] = 0;
    for (int i = pItem->m_texts.Begin(); i != pItem->m_texts.End(); i = pItem->m_texts.Next(i))
    {
        if (!pItem->m_texts[i].m_language)
        {
            VIString::strcpy16(m_name, sizeof(m_name), pItem->m_texts[i].m_name);
            VIString::strcpy16(m_desc, sizeof(m_desc), pItem->m_texts[i].m_desc);
            break;
        }
    }
    // VIASSERT( 0 == Mods.Init( pItem->Mods.Size() ) ), spelled out: VIArray has no Size()
    if (!(0 == m_stats.Init(pItem->Mods.m_size))) _VIAssert("0 == Mods.Init( pItem->Mods.Size() )", __FILE__, __LINE__);
    for (int j = pItem->Mods.Begin(); j != pItem->Mods.End(); j = pItem->Mods.Next(j))
    {
        m_stats[j].m_id = pItem->Mods[j].m_id;
        m_stats[j].m_value = pItem->Mods[j].m_value;
    }
}

// 0x00CBACE8: the item, field by field (+0x58 before +0x54), then its stats.
VIMemStream &operator>>(VIMemStream &s, VIItem &item)
{
    s.ReadSigned(item.m_id);
    s.ReadSigned(item.m_value);
    s.ReadSigned(item.m_p08);
    s.ReadSigned(item.m_icon);
    s.ReadSigned(item.m_icon2);
    s.ReadSigned(item.m_wearSlot);
    s.ReadSigned(item.m_p18);
    s.ReadSigned(item.m_noTrade);
    s.ReadSigned(item.m_rent);
    s.ReadSigned(item.m_p24);
    s.ReadSigned(item.m_attackType);
    s.ReadSigned(item.m_damage);
    s.ReadSigned(item.m_range);
    s.ReadSigned(item.m_levelReq);
    s.ReadSigned(item.m_maxStack);
    s.ReadSigned(item.m_maxHp);
    s.ReadSigned(item.m_durability);
    s.ReadSigned(item.m_classMask);
    s.ReadSigned(item.m_raceMask);
    s.ReadSigned(item.m_p4C);
    s.ReadSigned(item.m_lore);
    s.ReadSigned(item.m_p58);
    s.ReadSigned(item.m_p54);
    s.ReadString(item.m_name, sizeof(item.m_name));
    s.ReadString(item.m_desc, sizeof(item.m_desc));
    s >> item.m_stats;
    return s;
}

// 0x00CBAE50
VIMemStream &operator<<(VIMemStream &s, const VIItem &item)
{
    s.WriteSigned(item.m_id);
    s.WriteSigned(item.m_value);
    s.WriteSigned(item.m_p08);
    s.WriteSigned(item.m_icon);
    s.WriteSigned(item.m_icon2);
    s.WriteSigned(item.m_wearSlot);
    s.WriteSigned(item.m_p18);
    s.WriteSigned(item.m_noTrade);
    s.WriteSigned(item.m_rent);
    s.WriteSigned(item.m_p24);
    s.WriteSigned(item.m_attackType);
    s.WriteSigned(item.m_damage);
    s.WriteSigned(item.m_range);
    s.WriteSigned(item.m_levelReq);
    s.WriteSigned(item.m_maxStack);
    s.WriteSigned(item.m_maxHp);
    s.WriteSigned(item.m_durability);
    s.WriteSigned(item.m_classMask);
    s.WriteSigned(item.m_raceMask);
    s.WriteSigned(item.m_p4C);
    s.WriteSigned(item.m_lore);
    s.WriteSigned(item.m_p58);
    s.WriteSigned(item.m_p54);
    s.WriteString(item.m_name);
    s.WriteString(item.m_desc);
    s << item.m_stats;
    return s;
}

// 0x00CBAFB8
VIMemStream &operator>>(VIMemStream &s, VIItemStat &st)
{
    s.ReadSigned(st.m_id);
    s.ReadSigned(st.m_value);
    return s;
}

// 0x00CBAFF8
VIMemStream &operator<<(VIMemStream &s, const VIItemStat &st)
{
    s.WriteSigned(st.m_id);
    s.WriteSigned(st.m_value);
    return s;
}

// 0x00CBB040: the inventory slot's six words (the last one raw), then the item.
VIMemStream &ReadNetItem(VIMemStream &s, VINetItem &item)
{
    s.ReadSigned(item.Quantity);
    s.ReadSigned(item.m_curHp);
    s.ReadSigned(item.m_charges);
    s.ReadSigned(item.m_equipSlot);
    s.ReadSigned(item.m_quickSlot);
    s.Read(&item.m_invIndex, 4, 1);
    s >> (VIItem &)item;
    return s;
}

// 0x00CBB0C8
VIMemStream &WriteNetItem(VIMemStream &s, const VINetItem &item)
{
    s.WriteSigned(item.Quantity);
    s.WriteSigned(item.m_curHp);
    s.WriteSigned(item.m_charges);
    s.WriteSigned(item.m_equipSlot);
    s.WriteSigned(item.m_quickSlot);
    s.Write(&item.m_invIndex, 4, 1);
    s << (const VIItem &)item;
    return s;
}

// 0x00CBB150: an auction lot: all seven words zigzag, the seller, then the item.
VIMemStream &operator>>(VIMemStream &s, VIAuctionItem &a)
{
    s.ReadSigned(a.m_lotId);
    s.ReadSigned(a.m_quantity);
    s.ReadSigned(a.m_curHp);
    s.ReadSigned(a.m_charges);
    s.ReadSigned(a.m_p2F4);
    s.ReadSigned(a.m_p2F8);
    s.ReadSigned(a.m_p2FC);
    s.ReadString(a.m_seller, sizeof(a.m_seller));
    s >> (VIItem &)a;
    return s;
}

// 0x00CBB1F0
VIMemStream &operator<<(VIMemStream &s, const VIAuctionItem &a)
{
    s.WriteSigned(a.m_lotId);
    s.WriteSigned(a.m_quantity);
    s.WriteSigned(a.m_curHp);
    s.WriteSigned(a.m_charges);
    s.WriteSigned(a.m_p2F4);
    s.WriteSigned(a.m_p2F8);
    s.WriteSigned(a.m_p2FC);
    s.WriteString(a.m_seller);
    s << (const VIItem &)a;
    return s;
}
