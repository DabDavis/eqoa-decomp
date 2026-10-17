#line 1 "C:/code-frontiers-gm/game/VICharacter.cpp"
// VICharacter.cpp -- CLIENT 0x00CA4658 .. 0x00CB6358: the character
// record, its database rows and their stream, compare and diff operators.
#include "game.h"
#include "vikeydiff.h"
#include "visort.h"

// ---- EQA.USERS ----
bool operator<(const VIDbUserKey &a, const VIDbUserKey &b)
{
    if (strcmp(a.STATION_NAME, b.STATION_NAME) < 0)
        return 1;
    return 0;
}
bool operator==(const VIDbUserKey &a, const VIDbUserKey &b)
{
    return strcmp(a.STATION_NAME, b.STATION_NAME) == 0;
}
const char *VIDbUserKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbUserKey &k)
{
    s.WriteString(k.STATION_NAME);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbUserKey &k)
{
    s.ReadString(k.STATION_NAME, 32);
    return s;
}
bool operator==(const VIDbUser &a, const VIDbUser &b)
{
    return (const VIDbUserKey &)a == (const VIDbUserKey &)b &&
           a.m_x20 == b.m_x20;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbUser &r)
{
    s << (const VIDbUserKey &)r;
    s.WriteSigned(r.m_x20);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbUser &r)
{
    s >> (VIDbUserKey &)r;
    s.ReadSigned(r.m_x20);
    return s;
}
const char *VIDbUserKey::KeyNames[] = { "STATION_NAME" };

// ---- CHARACTER_ITEMS ----
bool operator<(const VIDbCharItemKey &a, const VIDbCharItemKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.INVENTORY_SLOT_ID < b.INVENTORY_SLOT_ID))
        return 1;
    return 0;
}
bool operator==(const VIDbCharItemKey &a, const VIDbCharItemKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.INVENTORY_SLOT_ID == b.INVENTORY_SLOT_ID;
}
const char *VIDbCharItemKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharItemKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.INVENTORY_SLOT_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharItemKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.INVENTORY_SLOT_ID);
    return s;
}
bool operator==(const VIDbCharItem &a, const VIDbCharItem &b)
{
    return (const VIDbCharItemKey &)a == (const VIDbCharItemKey &)b &&
           a.m_x08 == b.m_x08 &&
           a.m_x0C == b.m_x0C &&
           a.m_x10 == b.m_x10 &&
           a.m_x14 == b.m_x14 &&
           a.m_x18 == b.m_x18 &&
           a.m_x1C == b.m_x1C;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharItem &r)
{
    s << (const VIDbCharItemKey &)r;
    s.WriteSigned(r.m_x08);
    s.WriteSigned(r.m_x0C);
    s.WriteSigned(r.m_x10);
    s.WriteSigned(r.m_x14);
    s.WriteSigned(r.m_x18);
    s.WriteSigned(r.m_x1C);
    s.WriteU32(r.m_x20);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharItem &r)
{
    s >> (VIDbCharItemKey &)r;
    s.ReadSigned(r.m_x08);
    s.ReadSigned(r.m_x0C);
    s.ReadSigned(r.m_x10);
    s.ReadSigned(r.m_x14);
    s.ReadSigned(r.m_x18);
    s.ReadSigned(r.m_x1C);
    s.Read(&r.m_x20, 4, 1);
    return s;
}
const char *VIDbCharItemKey::KeyNames[] = { "CHARACTER_ID", "INVENTORY_SLOT_ID" };

// ---- CHARACTER_BANK_ITEMS ----
bool operator<(const VIDbCharBankItemKey &a, const VIDbCharBankItemKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.BANK_SLOT_ID < b.BANK_SLOT_ID))
        return 1;
    return 0;
}
bool operator==(const VIDbCharBankItemKey &a, const VIDbCharBankItemKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.BANK_SLOT_ID == b.BANK_SLOT_ID;
}
const char *VIDbCharBankItemKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharBankItemKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.BANK_SLOT_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharBankItemKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.BANK_SLOT_ID);
    return s;
}
bool operator==(const VIDbCharBankItem &a, const VIDbCharBankItem &b)
{
    return (const VIDbCharBankItemKey &)a == (const VIDbCharBankItemKey &)b &&
           a.m_x08 == b.m_x08 &&
           a.m_x0C == b.m_x0C &&
           a.m_x10 == b.m_x10 &&
           a.m_x14 == b.m_x14;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharBankItem &r)
{
    s << (const VIDbCharBankItemKey &)r;
    s.WriteSigned(r.m_x08);
    s.WriteSigned(r.m_x0C);
    s.WriteSigned(r.m_x10);
    s.WriteSigned(r.m_x14);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharBankItem &r)
{
    s >> (VIDbCharBankItemKey &)r;
    s.ReadSigned(r.m_x08);
    s.ReadSigned(r.m_x0C);
    s.ReadSigned(r.m_x10);
    s.ReadSigned(r.m_x14);
    return s;
}
const char *VIDbCharBankItemKey::KeyNames[] = { "CHARACTER_ID", "BANK_SLOT_ID" };

// ---- CHARACTER_QUEST_TOKENS ----
bool operator<(const VIDbCharQuestTokenKey &a, const VIDbCharQuestTokenKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.TOKEN_SLOT_ID < b.TOKEN_SLOT_ID))
        return 1;
    return 0;
}
bool operator==(const VIDbCharQuestTokenKey &a, const VIDbCharQuestTokenKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.TOKEN_SLOT_ID == b.TOKEN_SLOT_ID;
}
const char *VIDbCharQuestTokenKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharQuestTokenKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.TOKEN_SLOT_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharQuestTokenKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.TOKEN_SLOT_ID);
    return s;
}
bool operator==(const VIDbCharQuestToken &a, const VIDbCharQuestToken &b)
{
    return (const VIDbCharQuestTokenKey &)a == (const VIDbCharQuestTokenKey &)b &&
           a.m_x08 == b.m_x08;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharQuestToken &r)
{
    s << (const VIDbCharQuestTokenKey &)r;
    s.WriteSigned(r.m_x08);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharQuestToken &r)
{
    s >> (VIDbCharQuestTokenKey &)r;
    s.ReadSigned(r.m_x08);
    return s;
}
const char *VIDbCharQuestTokenKey::KeyNames[] = { "CHARACTER_ID", "TOKEN_SLOT_ID" };

// ---- CHARACTER_QUEST_HISTORY_TOKENS ----
bool operator==(const VIDbCharQuestHistoryKey &a, const VIDbCharQuestHistoryKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.QUEST_HISTORY_TOKEN_ID == b.QUEST_HISTORY_TOKEN_ID;
}
bool operator<(const VIDbCharQuestHistoryKey &a, const VIDbCharQuestHistoryKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.QUEST_HISTORY_TOKEN_ID < b.QUEST_HISTORY_TOKEN_ID))
        return 1;
    return 0;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharQuestHistoryKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.QUEST_HISTORY_TOKEN_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharQuestHistoryKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.QUEST_HISTORY_TOKEN_ID);
    return s;
}
const char *VIDbCharQuestHistoryKey::KeyName(int i)
{
    return KeyNames[i];
}
bool operator==(const VIDbCharQuestHistory &a, const VIDbCharQuestHistory &b)
{
    return (const VIDbCharQuestHistoryKey &)a == (const VIDbCharQuestHistoryKey &)b &&
           a.m_x08 == b.m_x08;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharQuestHistory &r)
{
    s << (const VIDbCharQuestHistoryKey &)r;
    s.WriteSigned(r.m_x08);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharQuestHistory &r)
{
    s >> (VIDbCharQuestHistoryKey &)r;
    s.ReadSigned(r.m_x08);
    return s;
}
const char *VIDbCharQuestHistoryKey::KeyNames[] = { "CHARACTER_ID", "QUEST_HISTORY_TOKEN_ID" };

// ---- CHARACTER_FACTION_RATINGS ----
bool operator==(const VIDbCharFactionKey &a, const VIDbCharFactionKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.FACTION_ID == b.FACTION_ID;
}
bool operator<(const VIDbCharFactionKey &a, const VIDbCharFactionKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.FACTION_ID < b.FACTION_ID))
        return 1;
    return 0;
}
const char *VIDbCharFactionKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharFactionKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.FACTION_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharFactionKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.FACTION_ID);
    return s;
}
bool operator==(const VIDbCharFaction &a, const VIDbCharFaction &b)
{
    return (const VIDbCharFactionKey &)a == (const VIDbCharFactionKey &)b &&
           a.m_x08 == b.m_x08;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharFaction &r)
{
    s << (const VIDbCharFactionKey &)r;
    s.WriteSigned(r.m_x08);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharFaction &r)
{
    s >> (VIDbCharFactionKey &)r;
    s.ReadSigned(r.m_x08);
    return s;
}
const char *VIDbCharFactionKey::KeyNames[] = { "CHARACTER_ID", "FACTION_ID" };

// ---- CHARACTER_ABILITIES ----
bool operator==(const VIDbCharAbilityKey &a, const VIDbCharAbilityKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.ABILITY_ID == b.ABILITY_ID;
}
bool operator<(const VIDbCharAbilityKey &a, const VIDbCharAbilityKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.ABILITY_ID < b.ABILITY_ID))
        return 1;
    return 0;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAbilityKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.ABILITY_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharAbilityKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.ABILITY_ID);
    return s;
}
const char *VIDbCharAbilityKey::KeyName(int i)
{
    return KeyNames[i];
}
bool operator==(const VIDbCharAbility &a, const VIDbCharAbility &b)
{
    return (const VIDbCharAbilityKey &)a == (const VIDbCharAbilityKey &)b &&
           a.m_x08 == b.m_x08 &&
           a.m_x0C == b.m_x0C &&
           a.m_x10 == b.m_x10 &&
           a.m_x14 == b.m_x14;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAbility &r)
{
    s << (const VIDbCharAbilityKey &)r;
    s.WriteSigned(r.m_x08);
    s.WriteSigned(r.m_x0C);
    s.WriteSigned(r.m_x10);
    s.WriteSigned(r.m_x14);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharAbility &r)
{
    s >> (VIDbCharAbilityKey &)r;
    s.ReadSigned(r.m_x08);
    s.ReadSigned(r.m_x0C);
    s.ReadSigned(r.m_x10);
    s.ReadSigned(r.m_x14);
    return s;
}
const char *VIDbCharAbilityKey::KeyNames[] = { "CHARACTER_ID", "ABILITY_ID" };

// ---- CHARACTER_ATTRIBUTES ----
bool operator==(const VIDbCharAttributeKey &a, const VIDbCharAttributeKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.ATTRIBUTE_ID == b.ATTRIBUTE_ID;
}
bool operator<(const VIDbCharAttributeKey &a, const VIDbCharAttributeKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.ATTRIBUTE_ID < b.ATTRIBUTE_ID))
        return 1;
    return 0;
}
const char *VIDbCharAttributeKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAttributeKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.ATTRIBUTE_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharAttributeKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.ATTRIBUTE_ID);
    return s;
}
bool operator==(const VIDbCharAttribute &a, const VIDbCharAttribute &b)
{
    return (const VIDbCharAttributeKey &)a == (const VIDbCharAttributeKey &)b &&
           a.m_x08 == b.m_x08;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAttribute &r)
{
    s << (const VIDbCharAttributeKey &)r;
    s.WriteSigned(r.m_x08);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharAttribute &r)
{
    s >> (VIDbCharAttributeKey &)r;
    s.ReadSigned(r.m_x08);
    return s;
}
const char *VIDbCharAttributeKey::KeyNames[] = { "CHARACTER_ID", "ATTRIBUTE_ID" };

// ---- CHARACTER_INFLUENCES ----
bool operator==(const VIDbCharInfluenceKey &a, const VIDbCharInfluenceKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.SORT == b.SORT;
}
bool operator<(const VIDbCharInfluenceKey &a, const VIDbCharInfluenceKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.SORT < b.SORT))
        return 1;
    return 0;
}
const char *VIDbCharInfluenceKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharInfluenceKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.SORT);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharInfluenceKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.SORT);
    return s;
}
bool operator==(const VIDbCharInfluence &a, const VIDbCharInfluence &b)
{
    return (const VIDbCharInfluenceKey &)a == (const VIDbCharInfluenceKey &)b &&
           a.m_x08 == b.m_x08 &&
           a.m_x0C == b.m_x0C &&
           a.m_x10 == b.m_x10 &&
           a.m_x14 == b.m_x14 &&
           a.m_x18 == b.m_x18 &&
           a.m_x1C == b.m_x1C &&
           a.m_x20 == b.m_x20;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharInfluence &r)
{
    s << (const VIDbCharInfluenceKey &)r;
    s.WriteSigned(r.m_x08);
    s.WriteSigned(r.m_x0C);
    s.WriteSigned(r.m_x10);
    s.WriteSigned(r.m_x14);
    s.WriteSigned(r.m_x1C);
    s.WriteSigned(r.m_x20);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharInfluence &r)
{
    s >> (VIDbCharInfluenceKey &)r;
    s.ReadSigned(r.m_x08);
    s.ReadSigned(r.m_x0C);
    s.ReadSigned(r.m_x10);
    s.ReadSigned(r.m_x14);
    s.ReadSigned(r.m_x1C);
    s.ReadSigned(r.m_x20);
    return s;
}
const char *VIDbCharInfluenceKey::KeyNames[] = { "CHARACTER_ID", "SORT" };

// ---- CHAR_CHAT_MENU_ITEM ----
bool operator==(const VIDbCharChatMenuItemKey &a, const VIDbCharChatMenuItemKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.MENU_ID == b.MENU_ID && a.POSITION == b.POSITION;
}
bool operator<(const VIDbCharChatMenuItemKey &a, const VIDbCharChatMenuItemKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.MENU_ID < b.MENU_ID) ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.MENU_ID == b.MENU_ID && a.POSITION < b.POSITION))
        return 1;
    return 0;
}
const char *VIDbCharChatMenuItemKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharChatMenuItemKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.MENU_ID);
    s.WriteSigned(k.POSITION);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharChatMenuItemKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.MENU_ID);
    s.ReadSigned(k.POSITION);
    return s;
}
bool operator==(const VIDbCharChatMenuItem &a, const VIDbCharChatMenuItem &b)
{
    return (const VIDbCharChatMenuItemKey &)a == (const VIDbCharChatMenuItemKey &)b &&
           a.m_x0C == b.m_x0C &&
           !VIString::strcmp16(a.m_text, b.m_text) &&
           !VIString::strcmp16(a.m_label, b.m_label);
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharChatMenuItem &r)
{
    s << (const VIDbCharChatMenuItemKey &)r;
    s.WriteSigned(r.m_x0C);
    s.WriteString(r.m_text);
    s.WriteString(r.m_label);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharChatMenuItem &r)
{
    s >> (VIDbCharChatMenuItemKey &)r;
    s.ReadSigned(r.m_x0C);
    s.ReadString(r.m_text, 256);
    s.ReadString(r.m_label, 64);
    return s;
}
const char *VIDbCharChatMenuItemKey::KeyNames[] = { "CHARACTER_ID", "MENU_ID", "POSITION" };

// ---- CHAR_CHAT_MENUS ----
bool operator==(const VIDbCharChatMenuKey &a, const VIDbCharChatMenuKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.MENU_ID == b.MENU_ID;
}
bool operator<(const VIDbCharChatMenuKey &a, const VIDbCharChatMenuKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.MENU_ID < b.MENU_ID))
        return 1;
    return 0;
}
const char *VIDbCharChatMenuKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharChatMenuKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.MENU_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharChatMenuKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.MENU_ID);
    return s;
}
bool operator==(const VIDbCharChatMenu &a, const VIDbCharChatMenu &b)
{
    return (const VIDbCharChatMenuKey &)a == (const VIDbCharChatMenuKey &)b &&
           a.MenuItems == b.MenuItems;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharChatMenu &r)
{
    s << (const VIDbCharChatMenuKey &)r;
    s << r.MenuItems;
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharChatMenu &r)
{
    s >> (VIDbCharChatMenuKey &)r;
    s >> r.MenuItems;
    return s;
}
const char *VIDbCharChatMenuKey::KeyNames[] = { "CHARACTER_ID", "MENU_ID" };

// ---- CHAR_ABILITY_EFFECTS ----
bool operator==(const VIDbCharAbilityEffectKey &a, const VIDbCharAbilityEffectKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.SORT == b.SORT;
}
bool operator<(const VIDbCharAbilityEffectKey &a, const VIDbCharAbilityEffectKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.SORT < b.SORT))
        return 1;
    return 0;
}
const char *VIDbCharAbilityEffectKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAbilityEffectKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.SORT);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharAbilityEffectKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.SORT);
    return s;
}
bool operator==(const VIDbCharAbilityEffect &a, const VIDbCharAbilityEffect &b)
{
    return (const VIDbCharAbilityEffectKey &)a == (const VIDbCharAbilityEffectKey &)b &&
           a.m_x08 == b.m_x08 &&
           a.m_x0C == b.m_x0C &&
           a.m_x10 == b.m_x10 &&
           a.m_x14 == b.m_x14 &&
           a.m_x1C == b.m_x1C;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAbilityEffect &r)
{
    s << (const VIDbCharAbilityEffectKey &)r;
    s.WriteSigned(r.m_x08);
    s.WriteSigned(r.m_x0C);
    s.WriteSigned(r.m_x10);
    s.WriteSigned(r.m_x14);
    s.WriteSigned(r.m_x18);
    s.WriteSigned(r.m_x1C);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharAbilityEffect &r)
{
    s >> (VIDbCharAbilityEffectKey &)r;
    s.ReadSigned(r.m_x08);
    s.ReadSigned(r.m_x0C);
    s.ReadSigned(r.m_x10);
    s.ReadSigned(r.m_x14);
    s.ReadSigned(r.m_x18);
    s.ReadSigned(r.m_x1C);
    return s;
}
const char *VIDbCharAbilityEffectKey::KeyNames[] = { "CHARACTER_ID", "SORT" };

// ---- CHAR_IGNORE_ENTRIES ----
bool operator==(const VIDbCharIgnoreKey &a, const VIDbCharIgnoreKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.SEQUENCER == b.SEQUENCER;
}
bool operator<(const VIDbCharIgnoreKey &a, const VIDbCharIgnoreKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.SEQUENCER < b.SEQUENCER))
        return 1;
    return 0;
}
const char *VIDbCharIgnoreKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharIgnoreKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.SEQUENCER);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharIgnoreKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.SEQUENCER);
    return s;
}
bool operator==(const VIDbCharIgnore &a, const VIDbCharIgnore &b)
{
    return (const VIDbCharIgnoreKey &)a == (const VIDbCharIgnoreKey &)b &&
           a.m_x08 == b.m_x08;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharIgnore &r)
{
    s << (const VIDbCharIgnoreKey &)r;
    s.WriteSigned(r.m_x08);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharIgnore &r)
{
    s >> (VIDbCharIgnoreKey &)r;
    s.ReadSigned(r.m_x08);
    return s;
}
const char *VIDbCharIgnoreKey::KeyNames[] = { "CHARACTER_ID", "SEQUENCER" };

// ---- CHARACTER_AUCTIONS ----
bool operator<(const VIDbCharAuctionKey &a, const VIDbCharAuctionKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.AUCTION_ID < b.AUCTION_ID))
        return 1;
    return 0;
}
bool operator==(const VIDbCharAuctionKey &a, const VIDbCharAuctionKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.AUCTION_ID == b.AUCTION_ID;
}
const char *VIDbCharAuctionKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAuctionKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.AUCTION_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharAuctionKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.AUCTION_ID);
    return s;
}
bool operator==(const VIDbCharAuction &a, const VIDbCharAuction &b)
{
    return (const VIDbCharAuctionKey &)a == (const VIDbCharAuctionKey &)b &&
           a.m_x08 == b.m_x08 &&
           a.m_x0C == b.m_x0C &&
           a.m_x10 == b.m_x10 &&
           a.m_x14 == b.m_x14 &&
           a.m_x18 == b.m_x18 &&
           a.m_x1C == b.m_x1C &&
           a.m_x20 == b.m_x20 &&
           a.m_x24 == b.m_x24;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAuction &r)
{
    s << (const VIDbCharAuctionKey &)r;
    s.WriteSigned(r.m_x08);
    s.WriteSigned(r.m_x0C);
    s.WriteSigned(r.m_x10);
    s.WriteSigned(r.m_x14);
    s.WriteSigned(r.m_x18);
    s.WriteSigned(r.m_x1C);
    s.WriteSigned(r.m_x20);
    s.WriteByte(r.m_x24);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharAuction &r)
{
    s >> (VIDbCharAuctionKey &)r;
    s.ReadSigned(r.m_x08);
    s.ReadSigned(r.m_x0C);
    s.ReadSigned(r.m_x10);
    s.ReadSigned(r.m_x14);
    s.ReadSigned(r.m_x18);
    s.ReadSigned(r.m_x1C);
    s.ReadSigned(r.m_x20);
    s.Read(&r.m_x24, 1, 1);
    return s;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbAuction &r)
{
    s << (const VIDbCharAuction &)r;
    s.WriteString(r.m_name);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbAuction &r)
{
    s >> (VIDbCharAuction &)r;
    s.ReadString(r.m_name, 24);
    return s;
}
const char *VIDbCharAuctionKey::KeyNames[] = { "CHARACTER_ID", "AUCTION_ID" };

// ---- CHARACTER_BIDS ----
bool operator<(const VIDbCharBidKey &a, const VIDbCharBidKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.AUCTION_ID < b.AUCTION_ID))
        return 1;
    return 0;
}
bool operator==(const VIDbCharBidKey &a, const VIDbCharBidKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.AUCTION_ID == b.AUCTION_ID;
}
const char *VIDbCharBidKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharBidKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.AUCTION_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharBidKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.AUCTION_ID);
    return s;
}
bool operator==(const VIDbCharBid &a, const VIDbCharBid &b)
{
    return (const VIDbCharBidKey &)a == (const VIDbCharBidKey &)b &&
           a.m_x08 == b.m_x08 &&
           a.m_x0C == b.m_x0C &&
           a.m_x10 == b.m_x10 &&
           a.m_x14 == b.m_x14;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharBid &r)
{
    s << (const VIDbCharBidKey &)r;
    s.WriteSigned(r.m_x08);
    s.WriteSigned(r.m_x0C);
    s.WriteSigned(r.m_x10);
    s.WriteByte(r.m_x14);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharBid &r)
{
    s >> (VIDbCharBidKey &)r;
    s.ReadSigned(r.m_x08);
    s.ReadSigned(r.m_x0C);
    s.ReadSigned(r.m_x10);
    s.Read(&r.m_x14, 1, 1);
    return s;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbBidHistory &r)
{
    s << (const VIDbCharBid &)r;
    s.WriteString(r.m_name);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbBidHistory &r)
{
    s >> (VIDbCharBid &)r;
    s.ReadString(r.m_name, 24);
    return s;
}
const char *VIDbCharBidKey::KeyNames[] = { "CHARACTER_ID", "AUCTION_ID" };

// ---- CHARACTER_SKILLS ----
bool operator<(const VIDbCharSkillKey &a, const VIDbCharSkillKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.SKILL_TYPE < b.SKILL_TYPE))
        return 1;
    return 0;
}
bool operator==(const VIDbCharSkillKey &a, const VIDbCharSkillKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.SKILL_TYPE == b.SKILL_TYPE;
}
const char *VIDbCharSkillKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharSkillKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.SKILL_TYPE);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharSkillKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.SKILL_TYPE);
    return s;
}
bool operator==(const VIDbCharSkill &a, const VIDbCharSkill &b)
{
    return (const VIDbCharSkillKey &)a == (const VIDbCharSkillKey &)b &&
           a.m_x08 == b.m_x08 &&
           a.m_x0C == b.m_x0C &&
           a.m_x10 == b.m_x10;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharSkill &r)
{
    s << (const VIDbCharSkillKey &)r;
    s.WriteSigned(r.m_x08);
    s.WriteSigned(r.m_x0C);
    s.WriteSigned(r.m_x10);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharSkill &r)
{
    s >> (VIDbCharSkillKey &)r;
    s.ReadSigned(r.m_x08);
    s.ReadSigned(r.m_x0C);
    s.ReadSigned(r.m_x10);
    return s;
}
const char *VIDbCharSkillKey::KeyNames[] = { "CHARACTER_ID", "SKILL_TYPE" };

// ---- CHARACTER_RECIPES ----
bool operator<(const VIDbCharRecipeKey &a, const VIDbCharRecipeKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.RECIPE_ID < b.RECIPE_ID))
        return 1;
    return 0;
}
bool operator==(const VIDbCharRecipeKey &a, const VIDbCharRecipeKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.RECIPE_ID == b.RECIPE_ID;
}
const char *VIDbCharRecipeKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharRecipeKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.RECIPE_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharRecipeKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.RECIPE_ID);
    return s;
}
bool operator<(const VIDbCharRecipe &a, const VIDbCharRecipe &b)
{
    if ((const VIDbCharRecipeKey &)a == (const VIDbCharRecipeKey &)b)
        return a.m_x08 < b.m_x08;
    return (const VIDbCharRecipeKey &)a < (const VIDbCharRecipeKey &)b;
}
bool operator==(const VIDbCharRecipe &a, const VIDbCharRecipe &b)
{
    return (const VIDbCharRecipeKey &)a == (const VIDbCharRecipeKey &)b &&
           a.m_x08 == b.m_x08;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharRecipe &r)
{
    s << (const VIDbCharRecipeKey &)r;
    s.WriteSigned(r.m_x08);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharRecipe &r)
{
    s >> (VIDbCharRecipeKey &)r;
    s.ReadSigned(r.m_x08);
    return s;
}
const char *VIDbCharRecipeKey::KeyNames[] = { "CHARACTER_ID", "RECIPE_ID" };

// ---- CHARACTER_MASTERIES ----
bool operator<(const VIDbCharMasteryKey &a, const VIDbCharMasteryKey &b)
{
    if (a.CHARACTER_ID < b.CHARACTER_ID ||
        (a.CHARACTER_ID == b.CHARACTER_ID && a.MASTERY_ID < b.MASTERY_ID))
        return 1;
    return 0;
}
bool operator==(const VIDbCharMasteryKey &a, const VIDbCharMasteryKey &b)
{
    return a.CHARACTER_ID == b.CHARACTER_ID && a.MASTERY_ID == b.MASTERY_ID;
}
const char *VIDbCharMasteryKey::KeyName(int i)
{
    return KeyNames[i];
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharMasteryKey &k)
{
    s.WriteSigned(k.CHARACTER_ID);
    s.WriteSigned(k.MASTERY_ID);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharMasteryKey &k)
{
    s.ReadSigned(k.CHARACTER_ID);
    s.ReadSigned(k.MASTERY_ID);
    return s;
}
bool operator<(const VIDbCharMastery &a, const VIDbCharMastery &b)
{
    if ((const VIDbCharMasteryKey &)a == (const VIDbCharMasteryKey &)b)
        return a.m_x08 < b.m_x08;
    return (const VIDbCharMasteryKey &)a < (const VIDbCharMasteryKey &)b;
}
bool operator==(const VIDbCharMastery &a, const VIDbCharMastery &b)
{
    return (const VIDbCharMasteryKey &)a == (const VIDbCharMasteryKey &)b &&
           a.m_x08 == b.m_x08;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbCharMastery &r)
{
    s << (const VIDbCharMasteryKey &)r;
    s.WriteSigned(r.m_x08);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbCharMastery &r)
{
    s >> (VIDbCharMasteryKey &)r;
    s.ReadSigned(r.m_x08);
    return s;
}
const char *VIDbCharMasteryKey::KeyNames[] = { "CHARACTER_ID", "MASTERY_ID" };


// ---- CHARACTERS: VICharacter's own columns ----
bool operator<(const VICharacter &a, const VICharacter &b)
{
    return a.CHARACTER_ID < b.CHARACTER_ID;
}
void VICharacter::Write(VIMemStream &s) const
{
    s.WriteSigned(m_x00);
    s.WriteSigned(CHARACTER_ID);
    s.WriteString(m_name);
    s.WriteSigned(m_x20);
    s.WriteSigned(m_x24);
    s.WriteSigned(m_x28);
    s.WriteSigned(m_x2C);
    s.WriteSigned(m_x30);
    s.WriteSigned(m_x34);
    s.WriteSigned(m_x38);
    s.WriteSigned(m_x3C);
    s.WriteSigned(m_x40);
    s.WriteSigned(m_x44);
    s.WriteSigned(m_x48);
    s.WriteSigned(m_x4C);
    s.WriteSigned(m_x50);
    s.WriteSigned(m_x54);
    s.WriteSigned(m_x58);
    s.WriteSigned(m_x5C);
    s.WriteSigned(m_x60);
    s.WriteSigned(m_x64);
    s.WriteSigned(m_x68);
    s.WriteSigned(m_x6C);
    s.WriteSigned(m_x70);
    s.WriteSigned(m_x74);
    s.WriteSigned(m_x78);
    s.WriteSigned(m_x110);
    s.WriteSigned(m_x114);
    s.WriteSigned(m_x118);
    s.WriteSigned(m_x11C);
    s.WriteSigned(m_x7C);
    s.Write(m_x80, 4, 3);
    s.Write(m_x8C, 4, 3);
}
void VICharacter::Read(VIMemStream &s)
{
    s.ReadSigned(m_x00);
    s.ReadSigned(CHARACTER_ID);
    s.ReadString(m_name, 24);
    s.ReadSigned(m_x20);
    s.ReadSigned(m_x24);
    s.ReadSigned(m_x28);
    s.ReadSigned(m_x2C);
    s.ReadSigned(m_x30);
    s.ReadSigned(m_x34);
    s.ReadSigned(m_x38);
    s.ReadSigned(m_x3C);
    s.ReadSigned(m_x40);
    s.ReadSigned(m_x44);
    s.ReadSigned(m_x48);
    s.ReadSigned(m_x4C);
    s.ReadSigned(m_x50);
    s.ReadSigned(m_x54);
    s.ReadSigned(m_x58);
    s.ReadSigned(m_x5C);
    s.ReadSigned(m_x60);
    s.ReadSigned(m_x64);
    s.ReadSigned(m_x68);
    s.ReadSigned(m_x6C);
    s.ReadSigned(m_x70);
    s.ReadSigned(m_x74);
    s.ReadSigned(m_x78);
    s.ReadSigned(m_x110);
    s.ReadSigned(m_x114);
    s.ReadSigned(m_x118);
    s.ReadSigned(m_x11C);
    s.ReadSigned(m_x7C);
    s.Read(m_x80, 4, 3);
    s.Read(m_x8C, 4, 3);
}
// Padding: FlattenChatMenus asserts from VICharacter.cpp:2309, 2310 and 2315, so its body sits on the original's lines (the ten empty functions the image has here, 0x00CA7388 .., are not written).














































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































void FlattenChatMenus(const VIArray<VIDbCharChatMenu> &ChatMenus,
                      VIArray<VIDbCharChatMenuKey> &ChatMenuKeys,
                      VIArray<VIDbCharChatMenuItem> &ChatMenuItems)
{
    VIASSERT(!ChatMenuKeys.Init( ChatMenus.Size() ));
    VIASSERT(!ChatMenuItems.Init( ChatMenus.Size() * VICHAR_CHAT_MENU_ITEM_COUNT ));
    for (int i = 0; i < ChatMenus.Size(); i++)
    {
        ChatMenuKeys(i).CHARACTER_ID = ChatMenus(i).CHARACTER_ID;
        ChatMenuKeys(i).MENU_ID = ChatMenus(i).MENU_ID;
        VIASSERT(ChatMenus(i).MenuItems.Size() == VICHAR_CHAT_MENU_ITEM_COUNT);
        for (int j = 0; j < VICHAR_CHAT_MENU_ITEM_COUNT; j++)
            ChatMenuItems(i * VICHAR_CHAT_MENU_ITEM_COUNT + j) = ChatMenus(i).MenuItems(j);
    }
    if (ChatMenuKeys.Size())
        sort(&ChatMenuKeys(0), &ChatMenuKeys(0) + ChatMenuKeys.Size());
    if (ChatMenuItems.Size())
        sort(&ChatMenuItems(0), &ChatMenuItems(0) + ChatMenuItems.Size());
}

void Diff(const VICharacter &New, const VICharacter &Old, VIMemStream &s)
{
    New.Write(s);
    VIArray<VIDbCharChatMenuKey>  OldChatMenuKeys;
    VIArray<VIDbCharChatMenuItem> OldChatMenuItems;
    VIArray<VIDbCharChatMenuKey>  NewChatMenuKeys;
    VIArray<VIDbCharChatMenuItem> NewChatMenuItems;
    FlattenChatMenus(Old.ChatMenus, OldChatMenuKeys, OldChatMenuItems);
    FlattenChatMenus(New.ChatMenus, NewChatMenuKeys, NewChatMenuItems);
    KeyDiff<VIDbCharItem, VIDbCharItemKey>(New.Items, Old.Items, s);
    KeyDiff<VIDbCharBankItem, VIDbCharBankItemKey>(New.BankItems, Old.BankItems, s);
    KeyDiff<VIDbCharQuestToken, VIDbCharQuestTokenKey>(New.QuestTokens, Old.QuestTokens, s);
    KeyDiff<VIDbCharQuestHistory, VIDbCharQuestHistoryKey>(New.QuestHistory, Old.QuestHistory, s);
    KeyDiff<VIDbCharFaction, VIDbCharFactionKey>(New.Factions, Old.Factions, s);
    KeyDiff<VIDbCharAbility, VIDbCharAbilityKey>(New.Abilities, Old.Abilities, s);
    KeyDiff<VIDbCharAttribute, VIDbCharAttributeKey>(New.Attributes, Old.Attributes, s);
    KeyDiff<VIDbCharInfluence, VIDbCharInfluenceKey>(New.Influences, Old.Influences, s);
    KeyDiff<VIDbCharChatMenuKey, VIDbCharChatMenuKey>(NewChatMenuKeys, OldChatMenuKeys, s);
    KeyDiff<VIDbCharChatMenuItem, VIDbCharChatMenuItemKey>(NewChatMenuItems, OldChatMenuItems, s);
    KeyDiff<VIDbCharAbilityEffect, VIDbCharAbilityEffectKey>(New.AbilityEffects, Old.AbilityEffects, s);
    KeyDiff<VIDbCharIgnore, VIDbCharIgnoreKey>(New.Ignores, Old.Ignores, s);
    KeyDiff<VIDbCharAuction, VIDbCharAuctionKey>(New.Auctions, Old.Auctions, s);
    KeyDiff<VIDbCharBid, VIDbCharBidKey>(New.Bids, Old.Bids, s);
    KeyDiff<VIDbCharSkill, VIDbCharSkillKey>(New.Skills, Old.Skills, s);
    KeyDiff<VIDbCharRecipe, VIDbCharRecipeKey>(New.Recipes, Old.Recipes, s);
    KeyDiff<VIDbCharMastery, VIDbCharMasteryKey>(New.Masteries, Old.Masteries, s);
}

VIMemStream &operator<<(VIMemStream &s, const VICharacter &c)
{
    c.Write(s);
    s << c.Items;
    s << c.Abilities;
    s << c.Attributes;
    s << c.Influences;
    s << c.QuestTokens;
    s << c.QuestHistory;
    s << c.Factions;
    s << c.BankItems;
    s << c.ChatMenus;
    s << c.AbilityEffects;
    s << c.Ignores;
    s << c.Auctions;
    s << c.Bids;
    s << c.Skills;
    s << c.Recipes;
    s << c.Masteries;
    return s;
}

VIMemStream &operator>>(VIMemStream &s, VICharacter &c)
{
    c.Read(s);
    s >> c.Items;
    s >> c.Abilities;
    s >> c.Attributes;
    s >> c.Influences;
    s >> c.QuestTokens;
    s >> c.QuestHistory;
    s >> c.Factions;
    s >> c.BankItems;
    s >> c.ChatMenus;
    s >> c.AbilityEffects;
    s >> c.Ignores;
    s >> c.Auctions;
    s >> c.Bids;
    s >> c.Skills;
    s >> c.Recipes;
    s >> c.Masteries;
    return s;
}

void CapitalizeName(char *name)
{
    VIString::strupr(name);
    if (*name)
        VIString::strlwr(name + 1);
}

// ---- BUDDYLINKS ----
VIMemStream &operator>>(VIMemStream &s, VIDbBuddyLinkRow &r)
{
    s >> r.m_x00;
    s >> r.m_x04;
    return s;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbBuddyLinkRow &r)
{
    s << r.m_x00;
    s << r.m_x04;
    return s;
}

// ---- GUILD_MEMBERS ----
VIMemStream &operator>>(VIMemStream &s, VIDbGuildMemberRow &r)
{
    s.ReadSigned(r.m_id);
    s.ReadString(r.m_name, 128);
    s.ReadString(r.m_text, 512);
    return s;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbGuildMemberRow &r)
{
    s << r.m_id;
    s.WriteString(r.m_name);
    s.WriteString(r.m_text);
    return s;
}

// ---- GUILDS ----
VIMemStream &operator>>(VIMemStream &s, VIDbGuildRow &r)
{
    s >> r.m_x00;
    s >> r.m_x04;
    s >> r.m_x08;
    s >> r.m_x0C;
    return s;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbGuildRow &r)
{
    s << r.m_x00;
    s << r.m_x04;
    s << r.m_x08;
    s << r.m_x0C;
    return s;
}

// ---- MAIL: the gram records (game.h) ----
VIMemStream &operator>>(VIMemStream &s, VIGramHeader &h)
{
    s.Read(&h.m_id, 4, 1);
    s.Read(&h.m_x00, 4, 1);
    s.Read(&h.m_x08, 4, 1);
    s.ReadString(h.m_from, 24);
    s.ReadString(h.m_subject, 128);
    s >> h.m_date;
    s.Read(&h.m_read, 1, 1);
    return s;
}
VIMemStream &operator<<(VIMemStream &s, const VIGramHeader &h)
{
    s.WriteU32(h.m_id);
    s.WriteU32(h.m_x00);
    s.WriteU32(h.m_x08);
    s.WriteString(h.m_from);
    s.WriteString(h.m_subject);
    s << h.m_date;
    s.WriteByte(h.m_read);
    return s;
}
// The original's reader WRITES the text after reading the row (WriteString, not
// ReadString): kept.
VIMemStream &operator>>(VIMemStream &s, VIGramMessage &m)
{
    s >> (VIGramHeader &)m;
    s.WriteString(m.m_text);
    return s;
}
VIMemStream &operator<<(VIMemStream &s, const VIGramMessage &m)
{
    s << (const VIGramHeader &)m;
    s.WriteString(m.m_text);
    return s;
}
VIGramMessage &VIGramMessage::operator=(const VIGramHeader &h)
{
    (VIGramHeader &)*this = h;
    m_text[0] = 0;
    return *this;
}

// ---- EQA.CLUSTERS ----
VIMemStream &operator<<(VIMemStream &s, const VIDbClusterRow &r)
{
    s.WriteString(r.m_address);
    s.WriteString(r.m_name);
    s.WriteString(r.m_title);
    s.WriteByte(r.m_x160);
    s.WriteString(r.m_x161);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbClusterRow &r)
{
    s.ReadString(r.m_address, 256);
    s.ReadString(r.m_name, 32);
    s.ReadString(r.m_title, 64);
    s.Read(&r.m_x160, 1, 1);
    s.ReadString(r.m_x161, 8);
    return s;
}
bool operator==(const VIDbClusterRow &a, const VIDbClusterRow &b)
{
    if (strcmp(a.m_address, b.m_address))
        return 0;
    if (strcmp(a.m_name, b.m_name))
        return 0;
    if (VIString::strcmp16(a.m_title, b.m_title))
        return 0;
    if (a.m_x160 != b.m_x160)
        return 0;
    return strcmp(a.m_x161, b.m_x161) == 0;
}

// ---- R_CHARACTERS ----
VIMemStream &operator<<(VIMemStream &s, const VIDbRemoteCharacterRow &r)
{
    s.WriteSigned(r.m_x00);
    s.WriteSigned(r.m_x04);
    s.WriteSigned(r.m_x08);
    s.WriteString(r.m_name);
    s.WriteSigned(r.m_x24);
    s.WriteSigned(r.m_x28);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbRemoteCharacterRow &r)
{
    s.ReadSigned(r.m_x00);
    s.ReadSigned(r.m_x04);
    s.ReadSigned(r.m_x08);
    s.ReadString(r.m_name, 24);
    s.ReadSigned(r.m_x24);
    s.ReadSigned(r.m_x28);
    return s;
}

// ---- AUCTIONS (the listing) ----
VIDbAuctionListingRow &VIDbAuctionListingRow::operator=(const VIDbAuction &a)
{
    m_x00 = a.AUCTION_ID;
    m_x04 = a.CHARACTER_ID;
    VIString::strcpy8(m_name, 24, a.m_name);
    m_x20 = a.m_x08;
    m_x24 = a.m_x0C;
    m_x28 = a.m_x10;
    m_x2C = a.m_x14;
    m_x30 = a.m_x18;
    m_x34 = a.m_x1C;
    m_x38 = a.m_x20;
    return *this;
}
int VIDbAuctionListingRow::IsValid()
{
    return 1;
}
VIMemStream &operator<<(VIMemStream &s, const VIDbAuctionListingRow &r)
{
    s.WriteSigned(r.m_x00);
    s.WriteSigned(r.m_x04);
    s.WriteString(r.m_name);
    s.WriteSigned(r.m_x20);
    s.WriteSigned(r.m_x24);
    s.WriteSigned(r.m_x28);
    s.WriteSigned(r.m_x2C);
    s.WriteSigned(r.m_x30);
    s.WriteSigned(r.m_x34);
    s.WriteSigned(r.m_x38);
    return s;
}
VIMemStream &operator>>(VIMemStream &s, VIDbAuctionListingRow &r)
{
    s.ReadSigned(r.m_x00);
    s.ReadSigned(r.m_x04);
    s.ReadString(r.m_name, 24);
    s.ReadSigned(r.m_x20);
    s.ReadSigned(r.m_x24);
    s.ReadSigned(r.m_x28);
    s.ReadSigned(r.m_x2C);
    s.ReadSigned(r.m_x30);
    s.ReadSigned(r.m_x34);
    s.ReadSigned(r.m_x38);
    return s;
}
