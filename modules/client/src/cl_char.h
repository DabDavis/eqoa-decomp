// cl_char.h -- after the windows: the record classes (operator==, compare), VICharacter.cpp, VIKeyDiff.h instantiations, VIMemStreamArray.h copies.
// CLIENT 0x00CA4658..0x00CB6020.
#ifndef CL_CHAR_H
#define CL_CHAR_H

// VICharacter.cpp's database rows.  One key struct and one row struct per
// table of the server's schema; the row derives from its key (every row operator hands
// the row itself to the key's), and each key carries its column names (KeyNames, the
// tables at 0x00CD79B8 ..).  The class names are ours, the table and key column names
// the image's.  The client never builds a character from these: they are the server's
// own code compiled in, reached only through VICharacter's stream and diff functions.
extern "C" int strcmp(const char *, const char *);
#include "vimemstreamarray.h"

// Two arrays are equal when they are the same size and equal element by element (CLIENT
// 0x00CA9A78, for the chat menus' item lists).  An int, which CHAR_CHAT_MENUS' operator==
// normalises (`sltu s2,zero,v0`).
template <class T> int operator==(const VIArray<T> &a, const VIArray<T> &b)
{
    if (a.m_size != b.m_size)
        return 0;
    for (int i = 0; i < a.m_size; i++)
        if (!(a.m_data[i] == b.m_data[i]))
            return 0;
    return 1;
}

// EQA.USERS
struct VIDbUserKey
{
    char STATION_NAME[32];
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbUser : public VIDbUserKey
{
    int m_x20;
};
bool operator<(const VIDbUserKey &a, const VIDbUserKey &b);
bool operator==(const VIDbUserKey &a, const VIDbUserKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbUserKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbUserKey &k);
bool operator==(const VIDbUser &a, const VIDbUser &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbUser &r);
VIMemStream &operator>>(VIMemStream &s, VIDbUser &r);

// CHARACTER_ITEMS
struct VIDbCharItemKey
{
    int CHARACTER_ID;
    int INVENTORY_SLOT_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharItem : public VIDbCharItemKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
    int m_x14;
    int m_x18;
    int m_x1C;
    unsigned int m_x20;
    int m_x24;
};
bool operator<(const VIDbCharItemKey &a, const VIDbCharItemKey &b);
bool operator==(const VIDbCharItemKey &a, const VIDbCharItemKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharItemKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharItemKey &k);
bool operator==(const VIDbCharItem &a, const VIDbCharItem &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharItem &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharItem &r);

// CHARACTER_BANK_ITEMS
struct VIDbCharBankItemKey
{
    int CHARACTER_ID;
    int BANK_SLOT_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharBankItem : public VIDbCharBankItemKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
    int m_x14;
};
bool operator<(const VIDbCharBankItemKey &a, const VIDbCharBankItemKey &b);
bool operator==(const VIDbCharBankItemKey &a, const VIDbCharBankItemKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharBankItemKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharBankItemKey &k);
bool operator==(const VIDbCharBankItem &a, const VIDbCharBankItem &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharBankItem &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharBankItem &r);

// CHARACTER_QUEST_TOKENS
struct VIDbCharQuestTokenKey
{
    int CHARACTER_ID;
    int TOKEN_SLOT_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharQuestToken : public VIDbCharQuestTokenKey
{
    int m_x08;
};
bool operator<(const VIDbCharQuestTokenKey &a, const VIDbCharQuestTokenKey &b);
bool operator==(const VIDbCharQuestTokenKey &a, const VIDbCharQuestTokenKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharQuestTokenKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharQuestTokenKey &k);
bool operator==(const VIDbCharQuestToken &a, const VIDbCharQuestToken &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharQuestToken &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharQuestToken &r);

// CHARACTER_QUEST_HISTORY_TOKENS
struct VIDbCharQuestHistoryKey
{
    int CHARACTER_ID;
    int QUEST_HISTORY_TOKEN_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharQuestHistory : public VIDbCharQuestHistoryKey
{
    int m_x08;
};
bool operator<(const VIDbCharQuestHistoryKey &a, const VIDbCharQuestHistoryKey &b);
bool operator==(const VIDbCharQuestHistoryKey &a, const VIDbCharQuestHistoryKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharQuestHistoryKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharQuestHistoryKey &k);
bool operator==(const VIDbCharQuestHistory &a, const VIDbCharQuestHistory &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharQuestHistory &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharQuestHistory &r);

// CHARACTER_FACTION_RATINGS
struct VIDbCharFactionKey
{
    int CHARACTER_ID;
    int FACTION_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharFaction : public VIDbCharFactionKey
{
    int m_x08;
};
bool operator<(const VIDbCharFactionKey &a, const VIDbCharFactionKey &b);
bool operator==(const VIDbCharFactionKey &a, const VIDbCharFactionKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharFactionKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharFactionKey &k);
bool operator==(const VIDbCharFaction &a, const VIDbCharFaction &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharFaction &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharFaction &r);

// CHARACTER_ABILITIES
struct VIDbCharAbilityKey
{
    int CHARACTER_ID;
    int ABILITY_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharAbility : public VIDbCharAbilityKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
    int m_x14;
};
bool operator<(const VIDbCharAbilityKey &a, const VIDbCharAbilityKey &b);
bool operator==(const VIDbCharAbilityKey &a, const VIDbCharAbilityKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAbilityKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharAbilityKey &k);
bool operator==(const VIDbCharAbility &a, const VIDbCharAbility &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAbility &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharAbility &r);

// CHARACTER_ATTRIBUTES
struct VIDbCharAttributeKey
{
    int CHARACTER_ID;
    int ATTRIBUTE_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharAttribute : public VIDbCharAttributeKey
{
    int m_x08;
};
bool operator<(const VIDbCharAttributeKey &a, const VIDbCharAttributeKey &b);
bool operator==(const VIDbCharAttributeKey &a, const VIDbCharAttributeKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAttributeKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharAttributeKey &k);
bool operator==(const VIDbCharAttribute &a, const VIDbCharAttribute &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAttribute &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharAttribute &r);

// CHARACTER_INFLUENCES
struct VIDbCharInfluenceKey
{
    int CHARACTER_ID;
    int SORT;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharInfluence : public VIDbCharInfluenceKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
    int m_x14;
    int m_x18;
    int m_x1C;
    int m_x20;
};
bool operator<(const VIDbCharInfluenceKey &a, const VIDbCharInfluenceKey &b);
bool operator==(const VIDbCharInfluenceKey &a, const VIDbCharInfluenceKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharInfluenceKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharInfluenceKey &k);
bool operator==(const VIDbCharInfluence &a, const VIDbCharInfluence &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharInfluence &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharInfluence &r);

// CHAR_CHAT_MENU_ITEM
struct VIDbCharChatMenuItemKey
{
    int CHARACTER_ID;
    int MENU_ID;
    int POSITION;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharChatMenuItem : public VIDbCharChatMenuItemKey
{
    VIDbCharChatMenuItem() { m_text[0] = 0; m_label[0] = 0; }
    int m_x0C;
    unsigned short m_text[128];
    unsigned short m_label[32];
};
bool operator<(const VIDbCharChatMenuItemKey &a, const VIDbCharChatMenuItemKey &b);
bool operator==(const VIDbCharChatMenuItemKey &a, const VIDbCharChatMenuItemKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharChatMenuItemKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharChatMenuItemKey &k);
bool operator==(const VIDbCharChatMenuItem &a, const VIDbCharChatMenuItem &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharChatMenuItem &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharChatMenuItem &r);

// CHAR_CHAT_MENUS
struct VIDbCharChatMenuKey
{
    int CHARACTER_ID;
    int MENU_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharChatMenu : public VIDbCharChatMenuKey
{
    VIArray<VIDbCharChatMenuItem> MenuItems;
};
bool operator<(const VIDbCharChatMenuKey &a, const VIDbCharChatMenuKey &b);
bool operator==(const VIDbCharChatMenuKey &a, const VIDbCharChatMenuKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharChatMenuKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharChatMenuKey &k);
bool operator==(const VIDbCharChatMenu &a, const VIDbCharChatMenu &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharChatMenu &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharChatMenu &r);

// CHAR_ABILITY_EFFECTS
struct VIDbCharAbilityEffectKey
{
    int CHARACTER_ID;
    int SORT;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharAbilityEffect : public VIDbCharAbilityEffectKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
    int m_x14;
    int m_x18;
    int m_x1C;
};
bool operator<(const VIDbCharAbilityEffectKey &a, const VIDbCharAbilityEffectKey &b);
bool operator==(const VIDbCharAbilityEffectKey &a, const VIDbCharAbilityEffectKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAbilityEffectKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharAbilityEffectKey &k);
bool operator==(const VIDbCharAbilityEffect &a, const VIDbCharAbilityEffect &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAbilityEffect &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharAbilityEffect &r);

// CHAR_IGNORE_ENTRIES
struct VIDbCharIgnoreKey
{
    int CHARACTER_ID;
    int SEQUENCER;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharIgnore : public VIDbCharIgnoreKey
{
    int m_x08;
};
bool operator<(const VIDbCharIgnoreKey &a, const VIDbCharIgnoreKey &b);
bool operator==(const VIDbCharIgnoreKey &a, const VIDbCharIgnoreKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharIgnoreKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharIgnoreKey &k);
bool operator==(const VIDbCharIgnore &a, const VIDbCharIgnore &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharIgnore &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharIgnore &r);

// CHARACTER_AUCTIONS
struct VIDbCharAuctionKey
{
    int CHARACTER_ID;
    int AUCTION_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharAuction : public VIDbCharAuctionKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
    int m_x14;
    int m_x18;
    int m_x1C;
    int m_x20;
    char m_x24; VI_HOST_ONLY(char m_portTail[3];)  // no Itanium tail reuse: VIDbAuction
};
bool operator<(const VIDbCharAuctionKey &a, const VIDbCharAuctionKey &b);
bool operator==(const VIDbCharAuctionKey &a, const VIDbCharAuctionKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAuctionKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharAuctionKey &k);
bool operator==(const VIDbCharAuction &a, const VIDbCharAuction &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharAuction &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharAuction &r);

// An auction listing with its seller's name (AUCTIONS?): CHARACTER_AUCTIONS' row and 24 chars.
struct VIDbAuction : public VIDbCharAuction
{
    char m_name[24];                    // +0x28
};
VIMemStream &operator<<(VIMemStream &s, const VIDbAuction &r);
VIMemStream &operator>>(VIMemStream &s, VIDbAuction &r);

// CHARACTER_BIDS
struct VIDbCharBidKey
{
    int CHARACTER_ID;
    int AUCTION_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharBid : public VIDbCharBidKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
    char m_x14; VI_HOST_ONLY(char m_portTail[3];)  // no Itanium tail reuse: VIDbBidHistory
};
bool operator<(const VIDbCharBidKey &a, const VIDbCharBidKey &b);
bool operator==(const VIDbCharBidKey &a, const VIDbCharBidKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharBidKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharBidKey &k);
bool operator==(const VIDbCharBid &a, const VIDbCharBid &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharBid &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharBid &r);

// A bid with its bidder's name (BID_HISTORY?): CHARACTER_BIDS' row and 24 chars.
struct VIDbBidHistory : public VIDbCharBid
{
    char m_name[24];                    // +0x18
};
VIMemStream &operator<<(VIMemStream &s, const VIDbBidHistory &r);
VIMemStream &operator>>(VIMemStream &s, VIDbBidHistory &r);

// CHARACTER_SKILLS
struct VIDbCharSkillKey
{
    int CHARACTER_ID;
    int SKILL_TYPE;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharSkill : public VIDbCharSkillKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
};
bool operator<(const VIDbCharSkillKey &a, const VIDbCharSkillKey &b);
bool operator==(const VIDbCharSkillKey &a, const VIDbCharSkillKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharSkillKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharSkillKey &k);
bool operator==(const VIDbCharSkill &a, const VIDbCharSkill &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharSkill &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharSkill &r);

// CHARACTER_RECIPES
struct VIDbCharRecipeKey
{
    int CHARACTER_ID;
    int RECIPE_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharRecipe : public VIDbCharRecipeKey
{
    int m_x08;
    int m_x0C;
    int m_x10;
};
bool operator<(const VIDbCharRecipeKey &a, const VIDbCharRecipeKey &b);
bool operator==(const VIDbCharRecipeKey &a, const VIDbCharRecipeKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharRecipeKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharRecipeKey &k);
bool operator<(const VIDbCharRecipe &a, const VIDbCharRecipe &b);
bool operator==(const VIDbCharRecipe &a, const VIDbCharRecipe &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharRecipe &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharRecipe &r);

// CHARACTER_MASTERIES
struct VIDbCharMasteryKey
{
    int CHARACTER_ID;
    int MASTERY_ID;
    static const char *KeyNames[];
    static const char *KeyName(int i);
};
struct VIDbCharMastery : public VIDbCharMasteryKey
{
    int m_x08;
};
bool operator<(const VIDbCharMasteryKey &a, const VIDbCharMasteryKey &b);
bool operator==(const VIDbCharMasteryKey &a, const VIDbCharMasteryKey &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharMasteryKey &k);
VIMemStream &operator>>(VIMemStream &s, VIDbCharMasteryKey &k);
bool operator<(const VIDbCharMastery &a, const VIDbCharMastery &b);
bool operator==(const VIDbCharMastery &a, const VIDbCharMastery &b);
VIMemStream &operator<<(VIMemStream &s, const VIDbCharMastery &r);
VIMemStream &operator>>(VIMemStream &s, VIDbCharMastery &r);


// ---- VICharacter: the CHARACTERS row and the sixteen child tables (296 bytes) ----
// Its own columns are written by Write (0x00CA6FF0): +0x00, +0x04, the name, +0x20 ..
// +0x78, then +0x110 .. +0x11C (four columns the class appends after its arrays), then
// +0x7C and the two raw triples.  Its arrays are written in member order by
// operator<< (0x00CA8ED8) and diffed by Diff (0x00CA8CF0).  Names ours; the class name is
// the file's.
class VICharacter
{
public:
    void Write(VIMemStream &s) const;                           // 0x00CA6FF0
    void Read(VIMemStream &s);                                  // 0x00CA71B8

    int  m_x00;                                     // +0x00
    int  CHARACTER_ID;                              // +0x04: operator< (0x00CA6FE0)
    char m_name[24];                                // +0x08
    int  m_x20;                                         // +0x20
    int  m_x24;                                         // +0x24
    int  m_x28;                                         // +0x28
    int  m_x2C;                                         // +0x2C
    int  m_x30;                                         // +0x30
    int  m_x34;                                         // +0x34
    int  m_x38;                                         // +0x38
    int  m_x3C;                                         // +0x3C
    int  m_x40;                                         // +0x40
    int  m_x44;                                         // +0x44
    int  m_x48;                                         // +0x48
    int  m_x4C;                                         // +0x4C
    int  m_x50;                                         // +0x50
    int  m_x54;                                         // +0x54
    int  m_x58;                                         // +0x58
    int  m_x5C;                                         // +0x5C
    int  m_x60;                                         // +0x60
    int  m_x64;                                         // +0x64
    int  m_x68;                                         // +0x68
    int  m_x6C;                                         // +0x6C
    int  m_x70;                                         // +0x70
    int  m_x74;                                         // +0x74
    int  m_x78;                                         // +0x78
    int  m_x7C;                                     // +0x7C
    int  m_x80[3];                                  // +0x80: raw
    int  m_x8C[3];                                  // +0x8C: raw
    VIArray<VIDbCharItem>          Items;           // +0x98
    VIArray<VIDbCharAbility>       Abilities;       // +0xA0
    VIArray<VIDbCharAttribute>     Attributes;      // +0xA8
    VIArray<VIDbCharInfluence>     Influences;      // +0xB0
    VIArray<VIDbCharQuestToken>    QuestTokens;     // +0xB8
    VIArray<VIDbCharQuestHistory>  QuestHistory;    // +0xC0
    VIArray<VIDbCharFaction>       Factions;        // +0xC8
    VIArray<VIDbCharBankItem>      BankItems;       // +0xD0
    VIArray<VIDbCharChatMenu>      ChatMenus;       // +0xD8
    VIArray<VIDbCharAbilityEffect> AbilityEffects;  // +0xE0
    VIArray<VIDbCharIgnore>        Ignores;         // +0xE8
    VIArray<VIDbCharAuction>       Auctions;        // +0xF0
    VIArray<VIDbCharBid>           Bids;            // +0xF8
    VIArray<VIDbCharSkill>         Skills;          // +0x100
    VIArray<VIDbCharRecipe>        Recipes;         // +0x108
    int  m_x110;                                        // +0x110: written after +0x78
    int  m_x114;                                        // +0x114: written after +0x78
    int  m_x118;                                        // +0x118: written after +0x78
    int  m_x11C;                                        // +0x11C: written after +0x78
    VIArray<VIDbCharMastery>       Masteries;       // +0x120
};
bool operator<(const VICharacter &a, const VICharacter &b);                     // 0x00CA6FE0
// The chat menus as two flat sorted arrays: one key per menu, VICHAR_CHAT_MENU_ITEM_COUNT
// items per menu (the diff compares those, not the nested arrays).
#define VICHAR_CHAT_MENU_ITEM_COUNT 4
void FlattenChatMenus(const VIArray<VIDbCharChatMenu> &ChatMenus,
                      VIArray<VIDbCharChatMenuKey> &ChatMenuKeys,
                      VIArray<VIDbCharChatMenuItem> &ChatMenuItems);               // 0x00CA73D8
void Diff(const VICharacter &New, const VICharacter &Old, VIMemStream &s);      // 0x00CA8CF0
VIMemStream &operator<<(VIMemStream &s, const VICharacter &c);                  // 0x00CA8ED8
VIMemStream &operator>>(VIMemStream &s, VICharacter &c);                        // 0x00CA8FD8

// A name as the client shows it: the first letter upper case, the rest lower (CLIENT
// 0x00CA90D8; the character-creation name window and the gram composer call it).
void CapitalizeName(char *name);                                                 // 0x00CA90D8

// ---- The tables the server keeps beside the character (the DB list's order after
// ---- CHARACTERS: BUDDYLINKS, GUILD_MEMBERS, GUILDS, MAIL, EQA.CLUSTERS, R_CHARACTERS,
// ---- BID_HISTORY, AUCTIONS).  Names and the table attributions are ours.
struct VIDbBuddyLinkRow                                     // 8 bytes
{
    int m_x00;
    int m_x04;
};
VIMemStream &operator>>(VIMemStream &s, VIDbBuddyLinkRow &r);                   // 0x00CA9110
VIMemStream &operator<<(VIMemStream &s, const VIDbBuddyLinkRow &r);             // 0x00CA9168

struct VIDbGuildMemberRow                                   // 516 bytes
{
    int            m_id;                        // +0x000
    unsigned short m_name[64];                  // +0x004
    unsigned short m_text[256];                 // +0x084
};
VIMemStream &operator>>(VIMemStream &s, VIDbGuildMemberRow &r);                 // 0x00CA91C0
VIMemStream &operator<<(VIMemStream &s, const VIDbGuildMemberRow &r);           // 0x00CA9218

struct VIDbGuildRow                                         // 16 bytes
{
    int m_x00;
    int m_x04;
    int m_x08;
    int m_x0C;
};
VIMemStream &operator>>(VIMemStream &s, VIDbGuildRow &r);                       // 0x00CA9270
VIMemStream &operator<<(VIMemStream &s, const VIDbGuildRow &r);                 // 0x00CA92F8

// MAIL: VIGramHeader / VIGramMessage are declared in game.h; their operators are here.
VIMemStream &operator>>(VIMemStream &s, VIGramMessage &m);                      // 0x00CA94E8
VIMemStream &operator<<(VIMemStream &s, const VIGramMessage &m);                // 0x00CA9528

struct VIDbClusterRow                                       // 364 bytes
{
    char           m_address[256];              // +0x000
    char           m_name[32];                  // +0x100
    unsigned short m_title[32];                 // +0x120
    char           m_x160;                      // +0x160
    char           m_x161[8];                   // +0x161
};
VIMemStream &operator<<(VIMemStream &s, const VIDbClusterRow &r);               // 0x00CA9630
VIMemStream &operator>>(VIMemStream &s, VIDbClusterRow &r);                     // 0x00CA96A8
bool operator==(const VIDbClusterRow &a, const VIDbClusterRow &b);              // 0x00CA9728

// 44 bytes; the VIMemStreamArray.h reader at 0x00BBDA90 reads an array of them.
struct VIDbRemoteCharacterRow
{
    int  m_x00;                                 // +0x00
    int  m_x04;                                 // +0x04
    int  m_x08;                                 // +0x08
    char m_name[24];                            // +0x0C
    int  m_x24;                                 // +0x24
    int  m_x28;                                 // +0x28
};
VIMemStream &operator<<(VIMemStream &s, const VIDbRemoteCharacterRow &r);       // 0x00CA97A8
VIMemStream &operator>>(VIMemStream &s, VIDbRemoteCharacterRow &r);             // 0x00CA9820

// 60 bytes: an auction as a listing shows it, filled from CHARACTER_AUCTIONS' row.
struct VIDbAuctionListingRow
{
    VIDbAuctionListingRow &operator=(const VIDbAuction &a);                     // 0x00CA9898
    int  IsValid();                                                             // 0x00CA9920: 1
    int  m_x00;                                 // +0x00: the auction id
    int  m_x04;                                 // +0x04: the character id
    char m_name[24];                            // +0x08
    int  m_x20;                                     // +0x20
    int  m_x24;                                     // +0x24
    int  m_x28;                                     // +0x28
    int  m_x2C;                                     // +0x2C
    int  m_x30;                                     // +0x30
    int  m_x34;                                     // +0x34
    int  m_x38;                                     // +0x38
};
VIMemStream &operator<<(VIMemStream &s, const VIDbAuctionListingRow &r);        // 0x00CA9928
VIMemStream &operator>>(VIMemStream &s, VIDbAuctionListingRow &r);              // 0x00CA99D0

#endif
