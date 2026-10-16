// game.h -- CLIENT's own classes, on top of SUPPORT's vi.h.
//
// The original tree is C:/code-frontiers-gm/game/ (its asserts name client_main.cpp,
// VIClient.cpp, VIEntity.cpp, VICreatureEntity.cpp, VICharacter.cpp, VIJournal.cpp,
// VIFileStream.cpp, VIMemStream*.cpp, VINet.h, VIKeyDiff.h, VIItemList.h, ...).
// A layout here is only what the written functions have MEASURED; `m_pNNNNN` is a
// field known only by its offset and `m_gNNNNN` unknown space.  Addresses are CLIENT
// VAs at its map's base, 0x00BB4980.  Method names marked provisional are ours.
//
// Bootstrapped from a declaration table in phase 62; maintained BY HAND from here.
// viclient.cpp's CHKs check the VIClient offsets, so a wrong edit fails the build.
#ifndef GAME_H
#define GAME_H

#include "vi.h"

// CLIENT's own integer names (an assert quotes `sizeof(uint16)`); SUPPORT's DRDP
// layer says uns16.
typedef unsigned short uint16;

extern "C" void  pf(const char *fmt, ...);
extern "C" int   printf(const char *fmt, ...);
extern "C" unsigned int VIHashResourceID(const char *s);
extern "C" void *memset(void *, int, int);
extern "C" void  free(void *);
extern "C" char *strchr(const char *, int);
extern "C" int   sscanf(const char *, const char *, ...);
extern "C" int   ezNetCtlDownInterface(int);
extern "C" void  exit(int);
extern "C" void  _VIAssert(const char *expr, const char *file, int line);
extern "C" void  _VIAssertf(int cond, const char *fmt, ...);
extern "C" const char *__VIAssertf_file;
extern "C" int   __VIAssertf_line;

// SUPPORT's spelling (vistringtable.cpp): the file and line go to two globals
// first, unconditionally, then the condition is passed down.
#define VIASSERT(x)  if (!(x)) _VIAssert(#x, __FILE__, __LINE__);
#define VIASSERTF(x, fmt, args...) \
    { __VIAssertf_file = __FILE__; __VIAssertf_line = __LINE__; _VIAssertf(x, fmt, ## args); }

#include "drdpapi.h"      // SUPPORT's DRDP library as CLIENT and STATION call it, shared

#include "vimemstream.h"   // VIMemStream.cpp's classes, shared with STATION

#include "vimemstreamx.h"  // VIRGB and the VIMemStream* TUs' operators, shared with STATION
#include "vinet.h"         // VINet.cpp: VIClientConn, VIClientDrdpRef, the senders, shared
#include "vidatetime.h"    // the CS time and date and their operators, shared

struct VIDialogBox;

// The game window behind VIClient+0x2BB78 (its methods sit between
// VIWndGame.cpp:4007 and VIWndGuild.cpp:503).
class VIWndGame
{
public:
    char          m_p000[0x5C];
    void WorldUnloaded();                                       // 0x00C4FF00, name provisional (UnloadWorld)
    void ManualCameraBegun();                                   // 0x00C56A18, name provisional (BeginManualCamera)
    int           m_p05C;                           // +0x05C: ToggleAutoAttack sends 0x0015 for an object while set
    char          m_p060[0x678 - 0x60];
    int           m_p678;                           // +0x678: InteractWithTarget sets it for a player target
    void Trade02();                                             // 0x00C55E28, name provisional
    void Trade16();                                             // 0x00C55E60, name provisional
    void Trade07();                                             // 0x00C55F38, name provisional
    void Trade06();                                             // 0x00C55F58, name provisional
    void Msg0063();                                             // 0x00C55FB8, name provisional
    void Msg001D();                                             // 0x00C55990, name provisional
    void Msg0060();                                             // 0x00C55FF8, name provisional
    void Msg0061();                                             // 0x00C55F78, name provisional
    void MerchantOpen();                                        // 0x00C55740, name provisional
    void AddText(const unsigned short *s);                      // 0x00C56BC0, name provisional
    void AddText(const unsigned short *s, const VIColor32F &c); // 0x00C56BE0, name provisional
    void AddText(const char *s);                                // 0x00C56BA0, name provisional
    void LootOption(int id);                                    // 0x00C56C20, name provisional
    void AddWord(int id, const VIColor32F &c);                  // 0x00C56C40, name provisional
    void LootOpen();                                            // 0x00C556D8, name provisional
    void LootClose();                                           // 0x00C55710, name provisional
    void BankClose();                                           // 0x00C55888, name provisional
    int  IsLootable();                                          // 0x00C56CA8, name provisional
    int  DialogAllowed();                                       // 0x00C56CB8, name provisional (Msg0034)
    void ShowDialogBox(VIDialogBox *d);                         // 0x00C557C8, name provisional (Msg0034, loot window up)
    int  IsModal();                                             // 0x00C56A38, name provisional (DamageFromOther, QuestStage)
    void OpenDialog(const unsigned short *text, int b);         // 0x00C55820, name provisional (ShowDialog)
    void Prompt(const unsigned short *text, int a, int b, int c);  // 0x00C55D88, name provisional: a mode-14 prompt box
    void ShowInvite(const char *name);                          // 0x00C563A8, name provisional (Msg0773)
    void TradeOffer(unsigned int id);                           // 0x00C55EC0, name provisional (Msg0057)
    void SetMode(int mode);                                     // 0x00C4FF80, name provisional (Msg005E, Msg00B2, Prompt)
    void Refresh();                                             // 0x00C56A48, name provisional (ResetView)
    void ShowBuddyRequest();                                    // 0x00C56018, name provisional (Msg076F)
    void HideBuddyRequest();                                    // 0x00C56050, name provisional (AnswerBuddyRequest)
    void ClosePanel708();                                       // 0x00C56718
    void ClosePanel6AC();                                       // 0x00C558F0: mode 0, hides +0x6AC
    void ClosePanel718();                                       // 0x00C56848: two app calls first
    void ClosePanel6BC();                                       // 0x00C55950: mode 0 and a call on +0x688 first
    void HidePanel6F0();                                        // 0x00C564B0, name provisional: hides +0x6F0, mode 0
    int  IsPanel6F0Open();                                      // 0x00C564D8, name provisional: +0x6F0's +0x1C != 0
    void ShowName(int a, int b, int max, const unsigned short *name); // 0x00C56AE8, name provisional
    void ShowNetStats(long age, unsigned long history, long x, unsigned short number, int ping); // 0x00C56C88, name provisional
    void OpenPanel34();                                         // 0x00C56478
    void OpenPanel40();                                         // 0x00C559B0: also a call on its +0x6C0 first
    void ClosePanel6C0();                                       // 0x00C559F8
    void SetPanel35();                                          // 0x00C56698: mode 35 with no SetMode(0)
    void ClosePanel704();                                       // 0x00C566B8
    void ClosePanel6D0();                                       // 0x00C560B0: also a call on +0x6D0 first
    void ClosePanel70C();                                       // 0x00C56760
    void OpenPanel18();                                         // 0x00C56078
    // Panels: OpenPanelN is SetMode(0) then mode N; ClosePanelX hides the sub-window at
    // +X.  Names are the mode number / the offset until the panels are known.
    void ClosePanel6FC();                                       // 0x00C565F8
    void ClosePanel700();                                       // 0x00C56658
    void OpenPanel27();                                         // 0x00C56418
    void ClosePanel6EC();                                       // 0x00C56450
    void OpenPanel22();                                         // 0x00C561C8
    void ClosePanel6DC();                                       // 0x00C56200
    void OpenPanel24();                                         // 0x00C56240
    void ClosePanel6E4();                                       // 0x00C56278
    void OpenPanel23();                                         // 0x00C562B8
    void ClosePanel6E0();                                       // 0x00C562F0
    void OpenPanel28();                                         // 0x00C564E8
    void ClosePanel6F4();                                       // 0x00C56520
    void OpenPanel29();                                         // 0x00C56560
    void ClosePanel6F8();                                       // 0x00C56598
    void MerchantClose();                                       // 0x00C557A0, name provisional
};

// The character-select message window behind VIClient+0x2BB84.
class VIWndLogin : public VIWnd
{
public:
    void ShowWord(unsigned short id);                           // 0x00C21668, name provisional (CharSelectMsg masks to 16 bits)
    void ShowText(const unsigned short *s);                     // 0x00C215F8, name provisional
    void Option(int on);                                        // 0x00C21C28, name provisional (CharacterList, when +0x50300 is set)
    void Connecting();                                          // 0x00C1FF08, name provisional (ConnectShard)
    void ShowCharacter(unsigned int model, int race, int cls, int hairColor, int hairLength,
                       int hairStyle, int face, int robe, int a, int b, int c, int d, int e,
                       int f);                                  // 0x00C216A8, name provisional: CreateCharacter's preview
    char m_pLogin[0x428 - sizeof(VIWnd)];
    int  m_p428;                                                // +0x428: CreateCharacter: the new character's slot
};

// The account window behind VIClient+0x2BB7C: ConnectShard sends its two strings.  Names
// provisional.
class VIWnd2BB7C : public VIWnd
{
public:
    char m_p2BB7Cpad[0x32C - sizeof(VIWnd)];
    char m_user[32];                                            // +0x32C
    char m_pass[32];                                            // +0x34C: empty means "send VIClient+0x502DC"
};

// The window behind VIClient+0x2BB88.
class VIWnd2BB88 : public VIWnd
{
public:
    void ShowList();                                            // 0x00C23398, name provisional (CharacterList)
    void SetName(const char *name);                             // 0x00C23460, name provisional
    VIWnd *Focus();                                             // 0x00C23360, name provisional: what SelectCharacter activates
};


// One of an item's stat bonuses, (id, value) (FrontiersForge item.lua: the "stat count"
// at +0x2DC and the "stat array" at +0x2E0 are this VIArray's m_size and m_data).
struct VIItemStat
{
    int m_id;
    int m_value;
};

// The item proper, 0x2E4 bytes; field names are FrontiersForge's (item.lua, checked against
// the getters at 0x00BC98F0..), `m_pNN` where it has none.  Its copy assignment is OUT
// of line (0x00BBDB38: the 23 words, the two wide strings element by element, then
// m_stats.Init(src.m_stats)), so VINetItem's synthesized one calls it and copies the six
// slot words itself -- which is what VIItemList::CopyFrom's loop body is.
class VIItem
{
public:
    // No user-declared copy or assignment: both are SYNTHESIZED, and too big to inline
    // (g++ 2.95 turns DECL_INLINE off on a synthesized method and checks it against
    // INTEGRATE_THRESHOLD, 8 * (8 + args) = 80 insns), so every TU that uses one emits it
    // out of line and the first copy linked takes every call: the copy constructor at
    // 0x00BBC490 and the assignment at 0x00BBDB38, with dead VIClient.cpp copies at
    // 0x00BE01A8 and 0x00BE36A0.
    int            m_id;                          // +0x00
    int            m_value;                       // +0x04
    int            m_p08;                         // +0x08
    int            m_icon;                        // +0x0C
    int            m_icon2;                       // +0x10: the Frontiers icon, used under expansion flag 2
    int            m_wearSlot;                    // +0x14: -1 none, 0 empty
    int            m_p18;                         // +0x18
    int            m_noTrade;                     // +0x1C
    int            m_rent;                        // +0x20
    int            m_p24;                         // +0x24
    int            m_attackType;                  // +0x28
    int            m_damage;                      // +0x2C
    int            m_range;                       // +0x30
    int            m_levelReq;                    // +0x34
    int            m_maxStack;                    // +0x38: AddInvItem passes it on
    int            m_maxHp;                       // +0x3C
    int            m_durability;                  // +0x40
    int            m_classMask;                   // +0x44
    int            m_raceMask;                    // +0x48
    int            m_p4C;                         // +0x4C
    int            m_lore;                        // +0x50
    int            m_p54;                         // +0x54
    int            m_p58;                         // +0x58
    unsigned short m_name[64];                    // +0x5C
    unsigned short m_desc[256];                   // +0xDC
    VIArray<VIItemStat> m_stats;                  // +0x2DC
};

// An item in an inventory slot: 0x2FC bytes, FrontiersForge's inventory record size.
class VINetItem : public VIItem
{
public:
    // Six scalars, not arrays: the synthesized copy moves them word by word.
    int         Quantity;                             // +0x2E4 (FrontiersForge)
    int         m_curHp;                              // +0x2E8 (FrontiersForge)
    int         m_charges;                            // +0x2EC: UseCharge counts it down
    int         m_equipSlot;                               // +0x2F0: Msg0019 stores -1 (equipped index)
    int         m_quickSlot;
    int         m_invIndex;                           // +0x2F8 (FrontiersForge)
};

// VIItemList.h's class (two of its inline functions assert from lines 315 and 326): one
// type for the inventory (+0xC428), the corpse (+0x13B98), the merchant (+0x1B310), the
// trade copy (+0x22B7C) and the bank (VIBankControl+8) -- every one of them is handed to
// the SAME Item().  A count, a second count CopyFrom bumps per item, and 40 slots of 764
// bytes: exactly 0x7768, and the corpse list ends where +0x1B300 begins.
class VIItemList
{
public:
    void Read(VIMemStream &s);                                  // 0x00BBC8D0, name provisional
    void Get(int slot, VINetItem &item);                        // 0x00BBD398, name provisional (Msg0019)
    void Took(int Slot, int Quantity);                          // 0x00BBD430: VIItemList.h:326 names both (VIClient.cpp copy 0x00BE0048)
    void UseCharge(int slot);                                   // 0x00BE0478, name provisional: --m_charges, Took at 0
    void Remove(int slot);                                      // 0x00BE04C0, name provisional: Took(slot, 1)
    int  IsEquipped(int equip);                                 // 0x00BE04E0, name provisional
    void ReplaceEquipped(int from, int to);                     // 0x00BE0530, name provisional
    int  Unequip(int equip);                                    // 0x00BE0568, name provisional: -1 if nothing was there
    void Clear() { ItemCount = 0; m_p04 = 0; }                   // inline: 0x0016 stores through &m_corpse
    int  Count() { return ItemCount; }
    VINetItem *SafeItem(int i) { if (i < Count()) return Item(i); return 0; }  // inline: CanUseListItem                            // inline: GetInvCount keeps &m_inventory in a register
    void Op003D(int a, int b);                                  // 0x00BE02F0, name provisional (0x003D)
    void Op0060(int slot, int hp);                              // 0x00BE05D0, name provisional (0x0060): m_curHp, floored at 0
    int  AddItem(VINetItem &item, int a, int b);                // 0x00BBD130, name provisional: returns where it went
    int  Merge(VINetItem &item, int limit, int capacity);       // 0x00BDF9D0, name provisional: returns how many went in
    int  Find(int id);                                          // 0x00BDFFA8, name provisional: the slot whose m_invIndex is id, or -1
    VINetItem *Item(int i);                                     // 0x00BB8970, name provisional: &ItemInternal[i]
    int  GetSlotId(int SlotIndex);                              // 0x00BBBDD0 (VIItemList.h:315): ItemInternal[SlotIndex].m_invIndex; name provisional
    void CopyFrom(VIItemList &src);                             // 0x00BDF870, name provisional (TradeSetup)
    void SetEquipSlot(int i, int v);                                 // 0x00BE05B8, name provisional (Msg003F)
    void SetQuickSlot(int i, int v);                                 // 0x00BE31F0, name provisional (Msg000A)

    int         ItemCount;                            // +0x00: the asserts' name (VIItemList.h:315, 326); 0x0016 zeroes it
    int         m_p04;                                // +0x04: CopyFrom bumps it per item; 0x0016 zeroes it first
    VINetItem   ItemInternal[40];                     // +0x08: the asserts' name (FrontiersForge: +0xC430 is the first item)
};

// One offered item in a trade window: an item and one more word (0x300 bytes, the
// stride TradeItem multiplies by).  VIClient keeps two arrays of four.
struct VITradeItem
{
    VINetItem m_item;
    int       m_id;                                // +0x2FC: TradeRemoveItem's key
};

// A 40-byte player record, shared by two larger records: VIBuddy below and a
// 48-byte one (+0x28 an int, +0x2C a bool) that func_00BF0F50 keeps in its own
// map, read by func_00BF11D8.  Its constructor is OUT of line and covers these
// 40 bytes only, so a record that embeds it gets a SYNTHESIZED inline
// constructor: that is why 0x03D5 hands the call a fresh `sp+48` (an inlined
// `this` is substituted as a constant) while every later use of the record
// shares one pseudo.  The class name is provisional; the fields are named from
// FrontiersForge's read of VIClient's own record at +0xC2DC, which has this head
// (claude-notes/frontiersforge-viclient-xref-20261015.md).
class VIPlayerRecord
{
public:
    VIPlayerRecord();                              // 0x00CB7788, name provisional
    int  m_id;                                     // +0x00: -1; the buddy map's key
    char m_name[24];                               // +0x04: zeroed
    int  m_class;                                  // +0x1C: -1 (FrontiersForge player.lua: class)
    int  m_race;                                   // +0x20: -1 (race)
    int  m_level;                                  // +0x24: -1 (level; VIClient's own record has it at +0xC300)
};

// Its reader returns the stream, as every reader here does (VIMemStreamArray.h's
// element loop reads `s >> a[i]` for any T), so the readers are operator>>.
VIMemStream &operator>>(VIMemStream &s, VIPlayerRecord &r);    // 0x00CB7850, name provisional

// A buddy-list record: 172 bytes, the value of VIClient::m_buddies.
class VIBuddy : public VIPlayerRecord
{
public:
    int            m_online;                       // +0x28: 0x03D5 stores (byte != 0)
    unsigned short m_wname[64];                    // +0x2C: 128 bytes, read wide
};

// An ability record: the value of VIClient's ability map (+0x2BB2C, 472-byte nodes), which
// the 0x00B3/B4/B5/B6/B8/CD/FD/FF handlers all work on.  Field names are FrontiersForge's
// (ability.lua: node offsets there, value offsets here; claude-notes/frontiersforge-
// viclient-xref-20261015.md).  Its last member is a VIArray, whose copy constructor is why
// the map's Node copy is out of line (0x00BE0AB8 -> the VIArray copy ctor 0x00BE0A88).
// One of an ability's 144-byte sub-records (VIAbility::Read reads four zigzag ints and a
// wide name per record).  Name and meaning provisional.
struct VIRec144_BE08C8
{
    int            m_p00;
    int            m_p04;
    int            m_p08;
    int            m_p0C;
    unsigned short m_name[64];                     // +0x10
};

struct VIAbility
{
    int  Read(VIMemStream &s);                     // 0x00BDCBE0: the record off the wire; Init's error, else 0

    int            m_p00;                          // +0x00 (node +0x14)
    int            m_displayIndex;                 // +0x04: the server's key for it; 0xFC.. are items
    int            m_valid;                        // +0x08
    int            m_slot;                         // +0x0C: spellbook position
    int            m_lockout;                      // +0x10: cooldown lockout in ms (0x00CD)
    int            m_p14;                          // +0x14
    int            m_level;                        // +0x18
    float          m_range;                        // +0x1C: the copy moves it through $f0
    int            m_castTime;                     // +0x20
    int            m_powerCost;                    // +0x24
    unsigned int   m_iconBack;                     // +0x28: UI texture dictionary hashes
    unsigned int   m_iconFront;                    // +0x2C
    int            m_scope;                        // +0x30
    int            m_cooldown;                     // +0x34
    unsigned int   m_equipReq;                     // +0x38: a bitmask
    unsigned short m_name[64];                     // +0x3C
    unsigned short m_desc[128];                    // +0xBC (FrontiersForge's 132 ran into the array)
    VIArray<VIRec144_BE08C8> m_p1BC;               // +0x1BC: a counted list of records; the member with a ctor,
                                                   // copy ctor and dtor that makes the map's node copy out of line
};

// The player's own record, VIClient+0xC2DC (FrontiersForge player.lua; offsets here are
// the record's).  Its head is a VIPlayerRecord, as the buddy and guild records' are; it
// runs to the inventory list at +0xC428, so it is 0x14C bytes.
class VIPlayer : public VIPlayerRecord
{
public:
    int CanUse(VINetItem *it);                     // 0x00BCA8F0: class and race masks, then the level
    int           m_exp;                           // +0x28 (+0xC304): 0x0020
    int           m_p2C;                           // +0x2C (+0xC308): 0x00D9
    unsigned char m_p30;                           // +0x30 (+0xC30C): a byte getter
    char          m_p31[3];
    int           m_p34;                           // +0x34 (+0xC310): 0x0052
    int           m_p38;                           // +0x38 (+0xC314)
    int           m_p3C;
    int           m_p40;                           // +0x40 (+0xC31C): 0x0036
    int           m_p44;                           // +0x44 (+0xC320): 0x001C
    char          m_p48[0x64 - 0x48];
    // The stat channel (DRDP type 0x42, 233 bytes) is these two blocks and m_p30, read by
    // ReadStats in that order.  Indices per claude-notes/sandstorm-stat-channel.md: 0-6 STR
    // STA AGI DEX WIS INT CHA (totals), 7 HP, 8 max HP, 9 power, 10 max power.
    int           m_stat[29];                      // +0x64 (+0xC340): GetPlayerStats
    int           m_baseStat[29];                  // +0xD8 (+0xC3B4): indexed by stat id (func_00BC9F88); GetPlayerPD8
};

// A ring of the last 64 round-trip times in ms, the newest at m_samples[m_head - 1]: the
// tail of a DRDP state channel 0x44..0x6B.  Names provisional (DrawNetStats).
struct VIPingRing
{
    unsigned char  m_head;                          // +0x00
    unsigned short m_samples[64];                   // +0x02
    unsigned short Last();                          // 0x00CC6500
    unsigned short Max();                           // 0x00CC6528: reads m_samples[0] 64 times
};

// One of the 40 records at VIClient+0x2CEE8, a DRDP state channel each (types 0x44..0x6B,
// 160 bytes on the wire and in memory).  Channel44 reads them; what they describe is not
// yet known.  Name provisional.
struct VIChannelRec
{
    long           m_id;                            // +0x00: eight bytes on the wire
    int            m_p08;
    int            m_p0C;
    int            m_p10;
    int            m_p14;
    int            m_p18;
    unsigned short m_size;                          // +0x1C: the last message's size; 0 = empty (DrawNetStats' lhu)
    VIPingRing     m_ping;                          // +0x1E
};

// A group (party) member, VIClient+0x2BD7C x4, 52 bytes (FrontiersForge group.lua).  Not
// vi.h's VIGroupMember, which is a sprite group's element.
struct VIPartyMember
{
    unsigned int  m_id;                            // +0x00
    int           m_p04;
    unsigned char m_hp;                            // +0x08
    unsigned char m_hpKnown;                       // +0x09
    unsigned char m_active;                        // +0x0A
    unsigned char m_p0B;
    int           m_p0C;
    VIVect3       m_pos;                           // +0x10: VIClient's ctor runs VIVect3() over all four
    char          m_name[24];                      // +0x1C
};

// One of two 32-byte objects at VIClient+0x2EC30/+0x2EC50 that SetTime feeds a time
// and the 64-bit +0x7568 (a clock base).  Name provisional.
class VISyncTime
{
public:
    void Set(long t, long base);                                // 0x00CBCBD8, name provisional
    long GetTime(long now);                                     // 0x00CBCC80: VISyncTime.cpp:95 asserts m_bReady
    void Reset();                                               // 0x00CBCD08, name provisional (Connect)
    long m_p00[4];
};

// The window embedded at VIClient+0x2BB98, 0x1A8 bytes: Notify sets its app pointer and
// makes it the active window, and Disconnect compares the active window against it.
class VIWnd2BB98 : public VIWnd
{
public:
    char m_p24[0x1A8 - sizeof(VIWnd)];
};

class VIEntity;
// The object at VIClient+0x4FD28 that DamageFromSelf reports through.  Name provisional.
class VIClientPart4FD28
{
public:
    void AddDamage(int defender, int amount, int z);            // 0x00BE90B8, name provisional
    void Release();                                             // 0x00BE9068, name provisional (UnloadWorld): its material
    int  Init(VIScene &sc, VIEntity **ents, int n);             // 0x00BE8EE0, name provisional (EnterGameLoad: the 24 entity slots)
    char m_p000[1];
};

// SUPPORT's VIGameOptions (vi.h); CLIENT reads it through this accessor too.
VIGameOptions *VIGetGameOptions();
const char *VIGetGameCode();                                       // SUPPORT, not yet written
int  CanWearIn(int wear, int slot);                               // 0x00CC61A8, name provisional: a wear slot's valid equipment slots
extern int gEntityUpdateRate;                                    // 0x00CD7AE8: 100, name provisional
int  VIHasDiscIdChanged();                                       // SUPPORT (viglobalee.cpp)
int  VICdStreamCancel();                                         // SUPPORT (viglobalee.cpp)
void VIDisplayScreen16(const char *text, int a, bool b);         // SUPPORT, not yet written
void Rumble(unsigned char motor, unsigned char level, int ms, VIPad *pad);  // 0x00BBF5F8, name provisional
int  ReleaseTargetEffect(VIScene &sc);                          // 0x00C4D610, name provisional: a global emitter and its definition
int  LoadTargetEffect(VIScene &sc);                             // 0x00C4D350, name provisional (EnterGameLoad)
int  VIMemCardReadFile(const char *name, void *buf, int size);  // SUPPORT, not yet written
struct VIElem_BDF7A0 { int m_data[1]; };                        // placeholder element: EnterGameLoad's and LoadWorld's local VIArray
void WriteQuantized(VIMemStream &s, int iBytesPerDimension, float v, float lo, float hi);              // 0x00BC2D28
void WriteQuantized(VIMemStream &s, const VIVect3 &v, int iBytesPerDimension, const VIBBox &box);     // 0x00BC2EB0

// The 32-byte element of the VIPool at VIClient+0x2BD58 (a placeholder: eqtmpl.py measured
// its size; Reset sends message 1572 with its first word before erasing each one).
// A pending group invite (Msg0773 queues one, Msg0774 drops one by id, Reset declines
// every one with op 1572).  Placeholder name kept: the map's VIPool members carry it.
struct VIRec32_BE2DE8
{
    int  m_id;                                     // +0x00: the inviter
    int  m_p04;
    char m_name[24];                               // +0x08
};

// Two CLIENT objects the constructor builds as members (its roll-call), not yet known.
// One of VIClient+0x4F0B0's entries, 34 bytes (SetNameEntry).  Name provisional.
struct VINameEntry
{
    VINameEntry() { m_name[0] = 0; m_p20 = 0; }     // AllocNames' new[] runs it
    char           m_name[32];
    unsigned short m_p20;
};

// VIJournal's file, +0x08: 200 bytes with a sticky error at +0xC0 (VIJournal's asserts read
// it as `m_File.GetError()`).  Its methods are 0x00CB70E0/0x00CB7188/0x00CB7280.  Name
// provisional; the bytes before the flag are not mapped.
class VIJournalFile
{
public:
    int  GetError() { return m_error; }
    long m_p00[0x18];
    int  m_error;                                               // +0xC0
    int  m_pC4;
};

// VIJournal.cpp: a record of console input and network packets, VIClient+0x90 (its
// asserts name `m_File` and `m_MemStream`), 0x100 bytes to +0x190.
class VIJournal
{
public:
    int Write(const void *p, int size, int count);              // 0x00CB7DB0: VIJournal.cpp:99
    // One value BY VALUE (JournalTick): a promoted byte lives in the inline's frame (a
    // 16-byte slot at +0) and a long behind an ADDRESSOF after it.
    template <class T> int Write(T v) { return Write(&v, sizeof(T), 1); }
    int  Open(int mode, const char *path, int size);            // 0x00CB7C30, name provisional: 1 records to `path`, 2 plays it
    void WriteString(const char *s);                            // 0x00CB7F38: VIJournal.cpp:158
    static void FreeName(char *buf, const char *fmt);           // 0x00CB80B0, name provisional: the first unused fmt %03u
    int  Read(void *p, int size, int count);                    // 0x00CB7E88, name provisional: a played-back journal's bytes
    // One value BY REFERENCE (Tick): the reading twin of Write(T).
    template <class T> int Read(T &v) { return Read(&v, sizeof(T), 1); }
    int  Playback() { return m_mode == 2; }                     // inline: Connect asks it twice
    int  GetError() { return m_File.GetError() || m_MemStream.GetError(); } // inline: Tick
    int           m_mode;                                       // +0x00: Open's mode; 2 plays back
    int           m_p04;
    VIJournalFile m_File;                                       // +0x08
    VIMemStream   m_MemStream;                                  // +0xD0
    int           m_pF0;
    int           m_written;                                    // +0xF4: Write adds size * count
    int           m_read;                                       // +0xF8: Read adds size * count
    int           m_pFC;
};

// A VIArray element the spellbook's ability list is built in (GetAbilityList stores map
// iterators into it).  Placeholder name kept: client_viarray.cpp's copies carry it.

// A per-race camera setting (the race camera lookup, GetRaceCamera: race x expansion).
// Only the floats ApplyRaceCamera and SetCameraStyle read are named.  Name provisional.
struct VIRaceCamera
{
    char  m_p00[0x24];
    float m_targetHeight;                            // +0x24
    float m_lookAtHeightAbove;                       // +0x28
    float m_pitch;                                   // +0x2C
    float m_distance;                                // +0x30
    float m_p34;                                     // +0x34: SetCameraStyle stores it into the camera
};

VIRaceCamera *GetRaceCamera(int race, int expansion);          // 0x00BFC048, name provisional (11 callers)

// The swing an auto-attack makes with what is worn in slots 14..20, by class and level:
// AutoAttackTick asks it for a spell effect and a delay.  Names provisional.
struct VISwing
{
    unsigned int Effect();                                      // 0x00CBE0A8
    int          Delay();                                       // 0x00CBE190
};
VISwing *GetSwing(int cls, int level, int b14, int b15, int b16, int b18, int b17,
                  int b19, int b20);                            // 0x00CBDEF8
int AttackAnim(int attackType, int alt);                        // 0x00CC5190 (VIEntity.cpp): the swing animation
class VIClientPart06D00;
class VIEntity;
VIEntity *CreateEntity(int type, unsigned int id, VIScene &sc, VIClientPart06D00 &res);  // 0x00CC5098: VIEntity.cpp:476, name provisional
void WrapAngles(VIVect3 &hpr);                                 // 0x00CC69C8, name provisional: each of the three
unsigned int UIImageId(const char *name);                       // 0x00CC62A8: VIHashResourceID of "P:\\studio\\eqps2\\Game_Assets\\UI\\%s.tga"

// One world: CLIENT's table at 0x00CF3738, 32 bytes an entry ("tunaria",
// "data\\tunaria.esf", word 0x413, ...).  Field names after the first three provisional.
struct VIWorldInfo
{
    const char *m_name;                              // +0x00: "tunaria"
    const char *m_path;                              // +0x04: LoadWorld parses it
    unsigned short m_nameWord;                       // +0x08: the WORDS.TXT id of its display name (lhu)
    unsigned short m_p0A;
    int         m_p0C;
    int         m_p10;
    int         m_p14;
    int         m_p18;
    int         m_p1C;
};
extern VIWorldInfo gWorldInfo[];


int strlen16(const unsigned short *s);                      // 0x00BCF988, name provisional: no caller in the image

// Send006A's records: a header int and a counted list whose first FOUR entries go out,
// each an int and two wide strings (388 bytes).  Names provisional.
struct VIRec388
{
    int            m_p00;
    unsigned short m_p04[128];                       // +0x004
    unsigned short m_p104[64];                       // +0x104
};
struct VIRec6A
{
    int               m_p00;
    VIArray<VIRec388> m_p04;                         // +0x04: Send006A reads m_data, always four
};

// A static table of 256 text sets (data_00CF5048; the word after it, data_00CF5448, is
// zeroed with it), one per id, each four wide-string pairs; the 0x000D dump fills it from
// VIRec6A records (LoadTextSets).  Its code is another TU's.  Names provisional.
struct VITextPair
{
    VITextPair();                                               // 0x00C0B870
    unsigned short *m_p00;                                      // VIRec388::m_p104's copy
    unsigned short *m_p04;                                      // VIRec388::m_p04's copy
};
class VITextSet
{
public:
    static void Reset();                                        // 0x00C0B350: all 256 slots
    static VITextSet *Get(int id);                              // 0x00C0B550: new'd on first use
    unsigned char m_id;
    VITextPair    m_pairs[4];                                   // +0x04
};

// The character-creation form, 88 bytes: func_00C230A0 fills it and CreateCharacter sends
// it as 0x002B (SendNewChar).  Names provisional; claude-notes/character-data-formats.md.
struct VINewChar
{
    char m_name[24];                              // +0x00
    int  m_p18;                                   // +0x18
    int  m_race;                                  // +0x1C
    int  m_class;                                 // +0x20
    int  m_sex;                                   // +0x24: indexes VIRaceDef's two models
    int  m_hairColor;                             // +0x28
    int  m_hairLength;                            // +0x2C
    int  m_hairStyle;                             // +0x30
    int  m_face;                                  // +0x34
    int  m_p38;                                   // +0x38
    int  m_stats[7];                              // +0x3C: written raw
};

// An NPC dialog, 0x408 bytes: 0x0034 reads one, VIClient keeps the one waiting for an
// answer at +0x2E7F4 (ending exactly at the +0x2EBFC pending flag).  Names provisional.
struct VIDialogBox
{
    int            m_id;                          // +0x000: the answer's key (0x0035)
    unsigned short m_text[128];                   // +0x004: expanded
    unsigned char  m_count;                       // +0x104
    unsigned short m_options[6][64];              // +0x106: expanded
};

// What func_00CA3870 hands InviteToGroup: a name three bytes in and an entity id at +0x1C.
// Name and the leading bytes provisional.
struct VIInviteRec
{
    unsigned char  m_p00[3];
    char           m_name[25];                    // +0x03
    unsigned int   m_id;                          // +0x1C
};

// The object at VIClient+0x6D00, right after the loader (80 bytes to +0x6D50).
class VIClientPart06D00
{
public:
    int  Init(VIScene &sc, VILoader &ld);                       // 0x00CBE4B8, name provisional (OpenResources)
    int  Clear();                                               // 0x00CBE520, name provisional (UnloadWorld, the dtor)
    char m_p00[0x50];
};

class VIClientPart06D50
{
public:
    int  Init(int n, VIScene &sc, VILoader &ld);                // 0x00BF2F98, name provisional (OpenResources: 48)
    int  Clear();                                               // 0x00BF3038, name provisional (UnloadWorld, the dtor)
    void Preload(int icon);                                     // 0x00BF31B8, name provisional (LoadInvIcons)
    char m_p000[1];
};

// A walk-camera setting as ApplyCameraPreset hands it to VIWalkCamera, 24 bytes
// (VIClient+0x6D90).  Field names are the camera's own.  Name provisional.
struct VICameraPreset
{
    VIWalkCameraMode m_mode;                         // +0x00: SetCameraMode
    float m_targetHeight;                            // +0x04: stored straight into the camera
    float m_lookAtHeightAbove;                       // +0x08
    float m_pitch;                                   // +0x0C: SetOrbitPitchAngle
    float m_distance;                                // +0x10: SetOrbitDistance
    float m_angle;                                   // +0x14: SetOrbitAngle
};

// A guild member: 48 bytes, the value of VIClientGuild::m_members.  The same
// record plus an int and a bool (4 bytes on this ABI).  Names provisional.
class VIGuildMember : public VIPlayerRecord
{
public:
    int  m_rank;                                   // +0x28
    bool m_online;                                 // +0x2C: a byte != 0
};

VIMemStream &operator>>(VIMemStream &s, VIGuildMember &m);      // 0x00BF11D8, name provisional

void ReadNetItem(VIMemStream &s, VINetItem &item);              // 0x00CBB040, name provisional

// An entity: CLIENT's VIEntity.cpp.  The vptr sits at +0x54, after the class's
// own data, and its first virtual is the destructor (a `delete` calls vtable
// entry 1 with __in_chrg 3).
class VIEntity
{
public:
    char m_p00[0x0C];
    int  m_id;                                        // +0x0C: FindEntity's key
    char m_p10[0x18 - 0x10];
    unsigned char m_p18;                              // +0x18: flags; 2 = not a creature (ToggleAutoAttack sends 0x0015 instead)
    unsigned char m_p19;                              // +0x19: a byte the client reads for itself and its target
    char m_p1A[0x24 - 0x1A];
    int  m_actor;                                     // +0x24: the scene actor index
    char m_p28[0x54 - 0x28];
    void SetHpr(const VIVect3 &hpr);                  // 0x00CC4958, name provisional (ResetView)
    VIVect3 *GetPos();                                // 0x00CC48A0: +0x40
    const VIVect3 &GetActorPos();                     // 0x00CC47F8: the scene actor's location (+0x30 scene, +0x24 actor)
    VIVect3 *GetHpr();                                // 0x00CC48C8: +0x00, the object itself
    const VIVect3 &GetActorHpr();                     // 0x00CC4848: the scene actor's heading
    void SetPos(const VIVect3 &p);                    // 0x00CC4918, name provisional: its own, then the actor's
    void SetActorPos(const VIVect3 &v);               // 0x00CC4818: the scene actor's location, if it has one
    void SetActorHpr(const VIVect3 &v);               // 0x00CC4868: ... and its heading
    void OnWorldLoaded();                             // 0x00CC53C0, name provisional: LoadWorld calls it on every entity
    virtual ~VIEntity();                              // slot 1: Disconnect's delete
    virtual int  Virtual2();                          // slots 2..9: names unknown; slot 2 is the type ChannelEntity compares
    virtual int  Virtual3(int what);                  // slot 3: Msg1003Done asks it about 130
    virtual void Virtual4();
    virtual void Virtual5(VIMemStream &s);            // slot 5: ChannelEntity hands it the rest of the channel
    virtual void Virtual6();
    virtual void Virtual7();
    virtual void Virtual8();
    virtual int  Virtual9();                          // slot 9: Msg1003Done calls it on every entity (returns int: its caller's registers say so)
};

// VICreatureEntity.cpp's class (RTTI "16VICreatureEntity").  The vptr stays at +0x54, the
// end of VIEntity's OWN data, so anything past +0x58 is the derived class's.
class VICreatureEntity : public VIEntity
{
public:
    int  Flag4();                                     // 0x00CC38C0, name provisional: +0x262 & 4
    int  Flag8();                                     // 0x00CC38D0, name provisional: +0x262 & 8
    unsigned char Byte263();                          // 0x00CC38F0, name provisional: +0x263
    void Walk();                                      // 0x00CC3938, name provisional: m_walk.Process while m_p255
    char m_name[24];                                  // +0x58: TradeName copies it (FrontiersForge: name)
    char m_p70[0x78 - 0x70];
    VIWalkControl m_walk;                             // +0x78: UpdateCreatures drives the sprite from it while m_p255
    VIVect3 m_p218;                                   // +0x218: ChannelEntity: no velocity when it did not move
    char m_p224[0x254 - 0x224];
    unsigned char m_state;                            // +0x254: 14 = dead; otherwise the locomotion animation
    unsigned char m_p255;                             // +0x255: Extrapolate keeps pitch and roll at 0 while set; walking
    unsigned char m_p256;                             // +0x256: GetTargetPlayerName wants 2 (2 or 3: a player)
    char m_p257[0x25A - 0x257];
    unsigned short m_npcFlags;                        // +0x25A: InteractWithTarget: 1 op 0x4C, 2 op 0x124D, 8 op 0x61
    char m_p25C[0x264 - 0x25C];
    unsigned char m_p264;                             // +0x264: InviteEntity wants it set (word 1014 when not)
    char m_p265[0x2A8 - 0x265];
    int  m_p2A8;                                      // +0x2A8: SetDeath's second argument; UpdateCreatures zeroes it
    char m_p2AC[0x2B0 - 0x2AC];
    long m_deathTime;                                 // +0x2B0: Msg005E and UpdateCreatures store the scene time
    int  m_p2B8;                                      // +0x2B8: Msg1003Done sets 1 on every entity but the player
};

class VIClientGuild
{
public:
    void Reset();                                               // 0x00BF0F20, name provisional: clears m_members
    void Recv0468(VIMemStream &s);                              // 0x00BF0D38, name provisional
    void UpdateMember(VIMemStream &s);                          // 0x00BF0F50, name provisional

    char                      m_p000[0x288];
    VIMap<int, VIGuildMember> m_members;                        // +0x288: 68-byte nodes
};

class VIClientPart2EF24
{
public:
    void Reset();                                               // 0x00BEE5F8, name provisional (EnterGameLoad)
    void Recv057A(VIMemStream &s);                              // 0x00BEF6D8, name provisional
    void Recv0579(VIMemStream &s);                              // 0x00BEF788, name provisional
};

// The customer-service client (0x00BE7120 .. 0x00BE8DF0): STATION's VICSClient,
// the same source (vicsclient.h, shared).  VIClient embeds it at +0x33950.
#include "vicsclient.h"

class VIClientPart382E8
{
public:
    void Show(unsigned int id, int x);                          // 0x00BB4DD0, name provisional (ShowA110)
    void Send0701a();                                           // 0x00BB7BA0, name provisional: sends 0x0701
    void Send0701b();                                           // 0x00BB7C70, name provisional: sends 0x0701 too
    void Reset();                                               // 0x00BB5960, name provisional (ResetWorld)
    void Recv0700(VIMemStream &s);                              // 0x00BB5DD0, name provisional
};

class VIClientPart38300
{
public:
    int  Find(const char *name);                                // 0x00BFAC80, name provisional: -1 = not listed (RefreshCollision)
    void Reset();                                               // 0x00BFA4F8, name provisional (ResetWorld): clears a pool
    void Recv(int opcode, VIMemStream &s);                      // 0x00BFA850, name provisional
};

// VIShardInfo.cpp: the server-select (shard) list, VIClient+0x38320.  Its asserts name
// `m_ServerSelectList` (`!m_ServerSelectList.Init( Match )`); the 88-byte entries carry a
// 64-bit id at +0x48, -1 for none.  Method names provisional.
class VIShardInfo
{
public:
    VIShardInfo();                                              // 0x00BFC0F0: two VIArray ctors
    int  GetCount();                                            // 0x00BFC128: m_ServerSelectList's size
    int  IsValid(int i);                                        // 0x00BFC130: entry i's id != -1
    unsigned long GetId(int i);                                 // 0x00BFC1F0: entry i's 64-bit id
    void Read(VIMemStream &s);                                  // 0x00BFC208: 0x07B3's payload
    void SetRegion(int region);                                 // 0x00BFC270, name provisional: +0x10, then func_00BFC290
    void Clear();                                               // 0x00BFC240: both arrays
    char m_p00[0x14];
};

class VIClientPart38334
{
public:
    void Open();                                                // 0x00C03038, name provisional: Close, then sends 0x0E01 (byte 1, zigzag +4)
    void Close();                                               // 0x00C03130, name provisional: clears +8, bumps +4
    void Recv0E02(VIMemStream &s);                              // 0x00C03168, name provisional
};

class VIClientPart3834C
{
public:
    void Recv1102(VIMemStream &s);                              // 0x00BFE118, name provisional
    void ResetP78C0() { m_p78C0 = 0; }                          // inline: EnterGameLoad stores through the part's own address
    char m_p0000[0x78C0];
    int  m_p78C0;                                               // +0x78C0: the part's last word; it ends at VIClient+0x3FC10
};

class VIClientPart3FC10
{
public:
    void Cancel();                                              // 0x00BEBF18, name provisional (Msg00B2, when +0x7770 is set)
    int  IsActive() { return m_p7770; }                         // inline: Msg00B2 reads it through the part's own address
    void Recv0F02(VIMemStream &s);                              // 0x00BEB900, name provisional
    char m_p0000[0x7770];
    int  m_p7770;                                               // +0x7770: Msg00B2 cancels when it is set
};

class VIBankControl
{
public:
    int  Count();                                               // 0x00BB8BE0, name provisional: the bank list's size
    VINetItem *Item(int i);                                     // 0x00BB8BE8, name provisional: the bank's item list
    void Close();                                               // 0x00BB8C28, name provisional
    void Recv(int opcode, VIMemStream &s);                      // 0x00BB9CF0, name provisional
    void Recv1206(VIMemStream &s);                              // 0x00BB9D90, name provisional
};

class VIClientPart4EC58
{
public:
    void Open();                                                // 0x00BF7CC0, name provisional: Close, then sends 0x1401 (byte 4, +0x41C)
    void Close();                                               // 0x00BF7EE8, name provisional
    int  Count();                                               // 0x00BF7F80, name provisional
    void Recv1402(VIMemStream &s);                              // 0x00BF7240, name provisional
    void ResetP41C() { m_p41C = 0; }                            // inline: EnterGameLoad stores through the part's own address
    char m_p000[0x41C];
    int  m_p41C;                                                // +0x41C: Open sends it; the part ends at VIClient+0x4F078
};

class VIClientPart4F078
{
public:
    void Recv07E2(VIMemStream &s);                              // 0x00BEA820, name provisional
};

class VIClientPart2EFD0
{
public:
    void OnConnect();                                           // 0x00BF5738, name provisional (the pump's 0xFF)
    void Recv0D41(VIMemStream &s);                              // 0x00BF5670, name provisional
    void Reset();                                               // 0x00BF57A0, name provisional
};

// Three sub-objects only the vtable names (slots 6, 16, 18).  The first two are four
// bytes apart, just below +0x2EFD0.  Names provisional.
class VIClientPart2EFC8 { public: char m_p00[4]; };
class VIClientPart2EFCC { public: char m_p00[4]; };
// A Mersenne twister, VIClient+0x4F234: 624 words seeded by x69069, then the index, 2,508
// bytes that run exactly to +0x4FC00.  Names provisional (GetShardServer, Connect).
class VIRandom
{
public:
    VIRandom();                                     // 0x00CBB6A8: m_index = -1
    unsigned int m_state[624];                      // +0x000
    int          m_p9C0[2];                         // +0x9C0
    int          m_index;                           // +0x9C8
    void         Seed(unsigned int s);              // 0x00CBB6B8
    unsigned int Next();                            // 0x00CBB8B0
    int          Range(int lo, int hi);             // 0x00CBBA48: lo + (Next() * (hi - lo) >> 32)
};
// One character-select entry, 0x60 bytes (CharacterList reads up to eight into
// VIClient+0x2BE70).  The zigzag fields follow the reference server's order
// (claude-notes/character-data-formats.md); the raw tail is the client's own reads.
struct VICharRecord
{
    char          m_name[24];                     // +0x00
    int           m_serverId;                     // +0x18
    int           m_modelId;                      // +0x1C
    int           m_class;                        // +0x20
    int           m_race;                         // +0x24
    int           m_level;                        // +0x28
    int           m_hairColor;                    // +0x2C
    int           m_hairLength;                   // +0x30
    int           m_hairStyle;                    // +0x34
    int           m_face;                         // +0x38
    int           m_robe;                         // +0x3C: raw
    unsigned char m_p40;                          // +0x40
    int           m_weapons[3];                   // +0x44: primary, secondary, shield (raw)
    unsigned char m_p50[3];                       // +0x50
    unsigned char m_armor[6];                     // +0x53
    unsigned char m_p59[6];                       // +0x59
};
// The text expander at VIClient+0x4FC24: one pointer (its first word is followed, +4, by
// what it formats from), and a 1,312-byte formatter of '%' escapes and '|c' colour codes
// (0x00BBDF30, not written).  Names provisional.
class VIClientText
{
public:
    int Expand(unsigned short *dst, unsigned short size, const unsigned short *src, int flag);  // 0x00BBDF30
    void *m_p00;
};
// A queued dialog line: 218 wide characters (ShowDialog's buffer).
struct VIRec436_BDF6D8 { unsigned short m_text[218]; };
// Two more known only by an accessor returning their address.  Names provisional.
// The pad state VIClient keeps at +0x7190, 84 bytes: PadKeys copies the new one over it
// after turning each changed analog key into SetKey(128 + i, ...).  Names provisional.
class VIClientPart07190
{
public:
    int   m_p00;
    float m_keys[20];                                // +0x04: 0.0 = up
};

// VIClientConn (slot 1's result) and VIClientDrdpRef are in vinet.h, shared with STATION.
void ReleaseMat(VIRaster &r, VIDictionary &d, int mat);                     // 0x00BBFE00, name provisional: VIClient.cpp's guard round ReleaseMaterial

// VIClient's base, RTTI "9VIClientI".  It has no data, so the vptr is at +0.  The
// vtable (0x00CDB4C0, 160 bytes) is entry 0 __tf8VIClient, these 18 virtuals in this
// order, and a NULL end: the destructor is slot 2 and slots 3-18 return sub-objects,
// the ones the dispatcher hands each subsystem's opcodes to.  Names provisional.
class VIClientI
{
public:
    virtual VIClientConn GetConn(int type) = 0;                 // slot 1
    virtual ~VIClientI();                                       // slot 2
    virtual VIBankControl     *GetBank() = 0;                   // slot 3: +0x47388
    virtual VIClientGuild     *GetGuild() = 0;                  // slot 4: +0x2EC84
    virtual VIClientPart2EF24 *GetP2EF24() = 0;                 // slot 5
    virtual VIClientPart2EFCC *GetP2EFCC() = 0;                 // slot 6
    virtual VIClientPart2EFD0 *GetP2EFD0() = 0;                 // slot 7
    virtual VICSClient *GetP33950() = 0;                 // slot 8
    virtual VIClientPart38300 *GetP38300() = 0;                 // slot 9
    virtual VIClientPart382E8 *GetP382E8() = 0;                 // slot 10
    virtual VIClientPart38334 *GetP38334() = 0;                 // slot 11
    virtual VIClientPart3834C *GetP3834C() = 0;                 // slot 12
    virtual VIClientPart3FC10 *GetP3FC10() = 0;                 // slot 13
    virtual VIClientPart4EC58 *GetP4EC58() = 0;                 // slot 14
    virtual VIClientPart4F078 *GetP4F078() = 0;                 // slot 15
    virtual VIRandom          *GetP4F234() = 0;                 // slot 16
    virtual VIPlayer          *GetPlayer() = 0;                 // slot 17: +0xC2DC
    virtual VIClientPart2EFC8 *GetP2EFC8() = 0;                 // slot 18
};

// The game.  ONE instance, a local of client_main, whose frame is 0x50380:
// 16 bytes of outgoing arguments + this + 64 of `sq` saves, so sizeof is
// 0x50321..0x50330.  Its constructor builds the engine as members (VIUI,
// VIRaster, VISoundDevice, VICollide, VIPad, VIKeyboard, VIDictionary, VILoader,
// VIWalkControl, ...) and its destructor closes them.  The FrontiersForge
// singleton (claude-notes/frontiersforge-layouts-verified-20260819.md, EE
// 0x1FAF730) is this object.
class VIClient : public VIClientI
{
public:
    VIClient();                                       // 0x00BBF698, 1,896 bytes
    ~VIClient();                                      // 0x00BC2638, 1,760 bytes
    int  Init(const char *cmd, char *a, const char *c, char *b, int record, int f6, int f7, int f8);  // 0x00BBFE80: the journal's start-up record, then InitBody
    int  InitBody();                                            // 0x00BC0098, not yet written
    bool IsRunning();                                 // 0x00BC2D18: `!m_quit`
    void Process();                                   // 0x00BC7BF0: one frame, 2,568
    void Shutdown();                                  // 0x00BC16D0, 80
    void Disconnect(int reason, int quiet);           // 0x00BC1720, 416
    void FreeBuffer();                                // 0x00BDD1E8, 72
    void StopMusic();                                 // 0x00BD90C8, name provisional
    void Notify();                                    // 0x00BC21E0, name provisional
    void Reset();                                     // 0x00BD3A60, name provisional
    VIEntity *FindEntity(int id);                     // 0x00BC9870: the LAST match
    void ShowMsg(unsigned short id);                  // 0x00BD90F8: word `id`, shown
    void ShowText(const unsigned short *s);           // 0x00BD9138: to the game window

    // Accessors (names provisional; FrontiersForge supplies most of the meanings).
    void SetP06DC0(int a, int b);                     // 0x00BD7708
    void SetP06DC0(int a);                            // 0x00BD7718
    void SetP06DC4(int b);                            // 0x00BD7720
    int  GetClass();                                  // 0x00BD7798
    int  GetRace();                                   // 0x00BD77A8
    int  GetLevel();                                  // 0x00BD77B8
    int  GetSelfP19();                                // 0x00BD77C8
    int  GetHP();                                     // 0x00BD77F0
    int  GetMaxHP();                                  // 0x00BD7800
    int  GetPlayerP30();                              // 0x00BD7810
    int  GetPower();                                  // 0x00BD7820
    int  GetMaxPower();                               // 0x00BD7830
    int  GetExp();                                    // 0x00BD7840
    unsigned int GetGroupMemberId(int i);             // 0x00BD7850
    int  GetGroupMemberHP(int i);                     // 0x00BD7870
    int  GetGroupMemberHPKnown(int i);                // 0x00BD7890
    int  GetGroupMemberActive(int i);                 // 0x00BD78B0
    char *GetGroupMemberName(int i);                  // 0x00BD78D0
    int  GetTargetP19();                              // 0x00BD78F0
    int  IsGroupLeader();                             // 0x00BD7A50
    int  GetP2BAEC();                                 // 0x00BE3900
    char *GetP4F098();                                // 0x00BE3918
    int  GetWorldId();                                // 0x00BE3928
    unsigned short *GetReplyName();                   // 0x00BE3930
    VIUI *GetUI();                                    // 0x00BE3940
    VIRaster *GetRaster();                            // 0x00BE3948
    VIScene *GetScene();                              // 0x00BE3950
    VICamera *GetP00190();                            // 0x00BE3958: &m_camera
    VIWalkCamera *GetWalkCamera();                    // 0x00BE3960
    VIDictionary *GetDictionary();                    // 0x00BE3968
    VIClientPart06D50 *GetP06D50();                   // 0x00BE3970
    void *GetActiveWindow();                          // 0x00BE3978
    long GetP07568();                                 // 0x00BE3990
    int  GetP06DC0();                                 // 0x00BE3998
    int  GetP06DC4();                                 // 0x00BE39A0
    unsigned int GetSelfId();                         // 0x00BE39A8
    VIPlayer *GetPlayer();                            // 0x00BE39B0: vtable slot 17

    // The rest of the vtable (VIClientI's order).
    VIClientConn GetConn(int type);                   // 0x00BD5EE8: slot 1
    VIBankControl     *GetBank();                     // 0x00BCCB58: slot 3
    VIClientGuild     *GetGuild();                    // 0x00BCDE38: slot 4
    VIClientPart2EF24 *GetP2EF24();                   // 0x00BCDE48: slot 5
    VIClientPart2EFCC *GetP2EFCC();                   // 0x00BCDE58: slot 6
    VIClientPart2EFD0 *GetP2EFD0();                   // 0x00BCDE68: slot 7
    VICSClient *GetP33950();                   // 0x00BCDE78: slot 8
    VIClientPart38300 *GetP38300();                   // 0x00BCE1A0: slot 9
    VIClientPart382E8 *GetP382E8();                   // 0x00BCE1B0: slot 10
    VIClientPart38334 *GetP38334();                   // 0x00BCE1C0: slot 11
    VIClientPart3834C *GetP3834C();                   // 0x00BCE1D0: slot 12
    VIClientPart3FC10 *GetP3FC10();                   // 0x00BCE1E0: slot 13
    VIClientPart4EC58 *GetP4EC58();                   // 0x00BCE1F0: slot 14
    VIClientPart4F078 *GetP4F078();                   // 0x00BCE200: slot 15
    VIRandom *GetP4F234();                            // 0x00BCE7A0: slot 16
    VIClientPart2EFC8 *GetP2EFC8();                   // 0x00BD7BD8: slot 18
    int  IsUnderLiquid();                             // 0x00BE39C0
    unsigned int GetTargetId();                       // 0x00BE39E0
    int  GetEffectCount();                            // 0x00BE39F0
    unsigned int GetEffectIcon(int i);                // 0x00BE3A08
    unsigned short *GetEffectName(int i);             // 0x00BE3A28
    int  GetTargetInfoValid();                        // 0x00BE3A40
    int  GetTargetDisposition();                      // 0x00BE3A58
    int  GetP2BB70();                                 // 0x00BE3A70

    // The item and ability getters the UI windows call directly (names provisional: the
    // list, then FrontiersForge's name for the field).  Inventory +0xC428, loot (the
    // corpse) +0x13B98, merchant +0x1B310, the trade copy of the inventory +0x22B7C,
    // the two trade offers through TradeItem, the abilities through the map.
    int  GetInvCount();                                         // 0x00BC98A8
    int  GetInvItemIcon(int i);                                 // 0x00BC98B8
    int  GetInvItemName(int i, unsigned short *buf, int size);  // 0x00BC9918
    int  GetInvItemDesc(int i, unsigned short *buf, int size);  // 0x00BC9A10
    int  ItemIcon(VINetItem *item);                             // 0x00BC9DD0: the Frontiers icon under expansion flag 2
    VITradeItem *TradeItem(int side, int i);                    // 0x00BDC438
    int  GetInvItemAmount(int i);                             // 0x00BC98F0
    int  GetInvItemEquipSlot(int i);                               // 0x00BC9A60
    int  GetInvItemWearSlot(int i);                           // 0x00BC9A88
    int  GetInvItemAttackType(int i);                         // 0x00BC9AB0
    int  GetInvItemLevelReq(int i);                           // 0x00BC9AD8
    int  GetInvItemP24(int i);                                // 0x00BC9B00
    int  GetInvItemP08(int i);                                // 0x00BC9B28
    int  GetInvItemClassMask(int i);                          // 0x00BC9B50
    int  GetInvItemRaceMask(int i);                           // 0x00BC9B78
    int  GetInvItemCharges(int i);                               // 0x00BC9BA0
    int  GetInvItemP4C(int i);                                // 0x00BC9BC8
    int  GetInvItemDamage(int i);                             // 0x00BC9BF0
    int  GetInvItemCurHp(int i);                              // 0x00BC9C18
    int  GetInvItemMaxHp(int i);                              // 0x00BC9C40
    int  GetInvItemRange(int i);                              // 0x00BC9C68
    int  GetInvItemMaxStack(int i);                           // 0x00BC9C90
    int  GetInvItemP58(int i);                                // 0x00BC9CB8
    int  GetInvItemDurability(int i);                         // 0x00BC9CE0
    int  GetInvItemP18(int i);                                // 0x00BC9D08
    int  GetInvItemNoTrade(int i);                            // 0x00BC9D30
    int  GetInvItemRent(int i);                               // 0x00BC9D58
    int  GetInvItemLore(int i);                               // 0x00BC9D80
    int  GetInvItemP54(int i);                                // 0x00BC9DA8
    int  GetInvItemQuickSlot(int i);                               // 0x00BC9F60
    int  GetLootItemAmount(int i);                            // 0x00BCAE60
    int  GetLootItemLevelReq(int i);                          // 0x00BCAED8
    int  GetLootItemClassMask(int i);                         // 0x00BCAF00
    int  GetLootItemRaceMask(int i);                          // 0x00BCAF28
    int  GetLootItemNoTrade(int i);                           // 0x00BCAF50
    int  GetLootItemRent(int i);                              // 0x00BCAF78
    int  GetLootItemLore(int i);                              // 0x00BCAFA0
    int  GetLootItemP4C(int i);                               // 0x00BCAFC8
    int  GetLootItemWearSlot(int i);                          // 0x00BCAFF0
    int  GetAbilityIconBack(int i);                           // 0x00BCBA80
    int  GetAbilityIconFront(int i);                          // 0x00BCBAA8
    int  GetAbilitySlot(int i);                               // 0x00BCBAD0
    int  GetAbilityDisplayIndex(int i);                       // 0x00BCBAF8
    int  GetAbilityLevel(int i);                              // 0x00BCBB20
    int  GetAbilityCastTime(int i);                           // 0x00BCBB48
    int  GetAbilityPowerCost(int i);                          // 0x00BCBB70
    int  GetAbilityCooldown(int i);                           // 0x00BCBBC0
    int  GetAbilityScope(int i);                              // 0x00BCBBE8
    int  GetAbilityEquipReq(int i);                           // 0x00BCBC10
    int  GetAbilityValid(int i);                              // 0x00BCBDF0
    int  GetAbilityP14(int i);                                // 0x00BCBE18
    int  GetMerchantItemP08(int i);                           // 0x00BCC4D0
    int  GetMerchantItemWearSlot(int i);                      // 0x00BCC4F8
    int  GetMerchantItemP18(int i);                           // 0x00BCC520
    int  GetMerchantItemP24(int i);                           // 0x00BCC548
    int  GetMerchantItemAttackType(int i);                    // 0x00BCC570
    int  GetMerchantItemDamage(int i);                        // 0x00BCC598
    int  GetMerchantItemRange(int i);                         // 0x00BCC5C0
    int  GetMerchantItemLevelReq(int i);                      // 0x00BCC5E8
    int  GetMerchantItemMaxHp(int i);                         // 0x00BCC610
    int  GetMerchantItemDurability(int i);                    // 0x00BCC638
    int  GetMerchantItemClassMask(int i);                     // 0x00BCC660
    int  GetMerchantItemRaceMask(int i);                      // 0x00BCC688
    int  GetMerchantItemP58(int i);                           // 0x00BCC6B0
    int  GetMerchantItemMaxStack(int i);                      // 0x00BCC6D8
    int  GetMerchantItemCharges(int i);                          // 0x00BCC700
    int  GetMerchantItemCurHp(int i);                         // 0x00BCC728
    int  GetMerchantItemNoTrade(int i);                       // 0x00BCC750
    int  GetMerchantItemRent(int i);                          // 0x00BCC778
    int  GetMerchantItemLore(int i);                          // 0x00BCC7A0
    int  GetMerchantItemP4C(int i);                           // 0x00BCC7C8
    int  GetTradeInvItemWearSlot(int i);                      // 0x00BCCF00
    int  GetTradeInvItemEquipSlot(int i);                          // 0x00BCCF28
    int  GetTradeInvItemP18(int i);                           // 0x00BCCF50
    int  GetTradeInvItemAttackType(int i);                    // 0x00BCCF78
    int  GetTradeInvItemDamage(int i);                        // 0x00BCCFA0
    int  GetTradeInvItemRange(int i);                         // 0x00BCCFC8
    int  GetTradeInvItemLevelReq(int i);                      // 0x00BCCFF0
    int  GetTradeInvItemMaxHp(int i);                         // 0x00BCD018
    int  GetTradeInvItemDurability(int i);                    // 0x00BCD040
    int  GetTradeInvItemClassMask(int i);                     // 0x00BCD068
    int  GetTradeInvItemRaceMask(int i);                      // 0x00BCD090
    int  GetTradeInvItemP58(int i);                           // 0x00BCD0B8
    int  GetTradeInvItemMaxStack(int i);                      // 0x00BCD0E0
    int  GetTradeInvItemAmount(int i);                        // 0x00BCD108
    int  GetTradeInvItemCurHp(int i);                         // 0x00BCD130
    int  GetTradeInvItemCharges(int i);                          // 0x00BCD158
    int  GetTradeInvItemNoTrade(int i);                       // 0x00BCD180
    int  GetTradeInvItemRent(int i);                          // 0x00BCD1A8
    int  GetTradeInvItemLore(int i);                          // 0x00BCD1D0
    int  GetTradeInvItemP4C(int i);                           // 0x00BCD1F8
    int  GetTradeOfferWearSlot(int side, int i);              // 0x00BCD308
    int  GetTradeOfferEquipSlot(int side, int i);                  // 0x00BCD328
    int  GetTradeOfferP18(int side, int i);                   // 0x00BCD348
    int  GetTradeOfferAttackType(int side, int i);            // 0x00BCD368
    int  GetTradeOfferDamage(int side, int i);                // 0x00BCD388
    int  GetTradeOfferRange(int side, int i);                 // 0x00BCD3A8
    int  GetTradeOfferLevelReq(int side, int i);              // 0x00BCD3C8
    int  GetTradeOfferMaxHp(int side, int i);                 // 0x00BCD3E8
    int  GetTradeOfferDurability(int side, int i);            // 0x00BCD408
    int  GetTradeOfferClassMask(int side, int i);             // 0x00BCD428
    int  GetTradeOfferRaceMask(int side, int i);              // 0x00BCD448
    int  GetTradeOfferP58(int side, int i);                   // 0x00BCD468
    int  GetTradeOfferMaxStack(int side, int i);              // 0x00BCD488
    int  GetTradeOfferAmount(int side, int i);                // 0x00BCD4A8
    int  GetTradeOfferCurHp(int side, int i);                 // 0x00BCD4C8
    int  GetTradeOfferCharges(int side, int i);                  // 0x00BCD4E8
    int  GetTradeOfferNoTrade(int side, int i);               // 0x00BCD508
    int  GetTradeOfferRent(int side, int i);                  // 0x00BCD528
    int  GetTradeOfferLore(int side, int i);                  // 0x00BCD548
    int  GetTradeOfferP4C(int side, int i);                   // 0x00BCD568

    // More accessors (0x00BC2118..0x00BE3B28; names provisional).
    VIClientPart07190 *GetP07190();                           // 0x00BE3880
    int  IsP00058Unset();                                     // 0x00BC2118
    void ClearPC1B0();                                        // 0x00BC6108
    VIColor32F *GetPC178();                                   // 0x00BC70E0: &m_fadeColor
    int *GetPlayerStats();                                    // 0x00BD6208
    int *GetPlayerPD8();                                      // 0x00BD6218
    int  IsConnected();                                       // 0x00BD83E0
    int  IsNotP0662C();                                       // 0x00BD90E8
    char *GetPetName();                                       // 0x00BD9510
    int  GetP13B90();                                         // 0x00BE3888
    void SetP13B90(int v);                                    // 0x00BE3898
    unsigned short *GetMerchantName();                        // 0x00BE38F0
    unsigned short *GetP4FC28();                              // 0x00BE3AA0
    unsigned short *GetP4FCA8();                              // 0x00BE3AB0
    int  GetP50304();                                         // 0x00BE3B28
    int  GetLootCount();                                      // 0x00BCAE08
    int  GetMerchantCount();                                  // 0x00BCC3D8
    int  GetTradeInvCount();                                  // 0x00BCCE08
    VIClientDrdpRef GetDrdpRef(long addr);                    // 0x00BD5F00
    void SetP2BAF4(int v);                                    // 0x00BDAEA0
    void SetP4F0EC(int v);                                    // 0x00BE3868
    int  GetP1B304();                                         // 0x00BE38D8
    int  GetP382E4();                                         // 0x00BE3B10
    int  GetPlayerP38();                                      // 0x00BCCB68
    int  GetPlayerP40();                                      // 0x00BD60B0
    int  GetPlayerP34();                                      // 0x00BE38C8
    int  GetQuestCount();                                     // 0x00BCEFA8
    int  GetCharCount();                                      // 0x00BD83F0
    int  GetP2BB74();                                         // 0x00BE3A88
    int  GetExitCode();                                       // 0x00BE3838
    int  GetDisconnectReason();                               // 0x00BE3850
    VIItemList *GetInventory();                               // 0x00BE38A8
    VIItemList *GetInventory2();                              // 0x00BE38B8
    VIBuddy *GetBuddy(int n);                                 // 0x00BDB788
    int  GetBuddyCount();                                     // 0x00BCDE10
    char *GetBuddyName(int n);                                // 0x00BCDD50
    unsigned short *GetBuddyWName(int n);                     // 0x00BCDD70
    int  GetBuddyRace(int n);                                 // 0x00BCDD90
    int  GetBuddyClass(int n);                                // 0x00BCDDB0
    int  GetBuddyLevel(int n);                                // 0x00BCDDD0
    int  GetBuddyOnline(int n);                               // 0x00BCDDF0
    int  GetTradeCount(int side);                             // 0x00BCD220
    int  GetTradeP2BAF0(int side);                            // 0x00BCD588
    int  IsAbilityLocked(int i);                              // 0x00BCBB98
    float GetAbilityRange(int i);                             // 0x00BCBC38
    VIArray<VIItemStat> *GetMerchantItemStats(int i);         // 0x00BCC7F0

    void RequestBuddy(int n);                                   // 0x00BCDA18: sends 0x0369 with buddy n's id

    void SendOpcodeByte(int op, unsigned char b);               // 0x00BE5748: an opcode and one byte

    int  PlayCue(const char *name, float vol);                  // 0x00BD8E98: m_ui.SoundPlay by name
    void ResetBattleMusic();                                    // 0x00BCE8E0, name provisional (Msg005E)
    void ResetCamera();                                         // 0x00BC5F78, name provisional (Msg005E)
    void Respawn(int world, VIVect3 &pos, VIVect3 &dir);        // 0x00BD5628, name provisional (Msg00B2): no re-extension, so int
    void TradeCancel();                                         // 0x00BCD980: (86, 1, partner), then Trade16
    int  MerchantClose();                                       // 0x00BCC388: the window, then +0x1B300 = -1
    void SendOpSigned(int op, int v);                           // 0x00BE52C8: an opcode and a zigzag int

    void SendOpInt(int op, int v);                              // 0x00BE4C28
    void RequestLootAll();                                      // 0x00BCB0B8
    void Quit();                                                // 0x00BC2248
    void SendOp000F(int v);                                     // 0x00BC8B08
    void SendOp0015(int v);                                     // 0x00BCB018
    void SendOp0626(int v);                                     // 0x00BD8348
    VICharRecord *GetChar(int i);                               // 0x00BD8408
    VIPartyMember *GetGroupMember(int i);                       // 0x00BD7B10
    void SetP50308(int a, int b, int c);                        // 0x00BD50C8
    void SendRaw(const void *data, int size);                   // 0x00BD5F18

    void ClosePanel6FC();                                       // 0x00BCE150: m_wndGame->ClosePanel6FC()
    void ClosePanel700();                                       // 0x00BCE178: m_wndGame->ClosePanel700()
    void OpenPanel27();                                         // 0x00BCE7B0: m_wndGame->OpenPanel27()
    void ClosePanel6EC();                                       // 0x00BCE7D8: m_wndGame->ClosePanel6EC()
    void OpenPanel22();                                         // 0x00BCEC80: m_wndGame->OpenPanel22()
    void ClosePanel6DC();                                       // 0x00BCECA8: m_wndGame->ClosePanel6DC()
    void OpenPanel24();                                         // 0x00BCED60: m_wndGame->OpenPanel24()
    void ClosePanel6E4();                                       // 0x00BCED88: m_wndGame->ClosePanel6E4()
    void OpenPanel23();                                         // 0x00BCEDB0: m_wndGame->OpenPanel23()
    void ClosePanel6E0();                                       // 0x00BCEDD8: m_wndGame->ClosePanel6E0()
    void OpenPanel28();                                         // 0x00BCEE00: m_wndGame->OpenPanel28()
    void ClosePanel6F4();                                       // 0x00BCEE28: m_wndGame->ClosePanel6F4()
    void OpenPanel29();                                         // 0x00BCEE50: m_wndGame->OpenPanel29()
    void ClosePanel6F8();                                       // 0x00BCEE78: m_wndGame->ClosePanel6F8()
    VIArray<VIItemStat> *GetInvItemStats(int i);                // 0x00BCAB78

    void ClosePanel708();                                       // 0x00BCEF30: m_wndGame->ClosePanel708()
    void ClosePanel70C();                                       // 0x00BCEF58: m_wndGame->ClosePanel70C()
    void OpenPanel18();                                         // 0x00BCDCE8: m_wndGame->OpenPanel18()
    void ClearTarget();                                         // 0x00BD5F38
    void SendOp0035(int v, unsigned char b);                    // 0x00BD6088
    void ShowText(const char *s);                               // 0x00BD91B0
    int  ExpandText(unsigned short *dst, unsigned short size, const unsigned short *src, int flag);// 0x00BE3AC0
    int  IsTargetPet();                                         // 0x00BE3AE8
    VINetItem *GetLootItem(int i);                              // 0x00BE3B58
    VINetItem *GetMerchantItem(int i);                          // 0x00BE3B80
    VINetItem *GetTradeInvItem(int i);                          // 0x00BE3BA8

    void SetPC178(const VIColor32F &v);                         // 0x00BC70F0: the fade colour, a synthesized (block) copy
    int  ProcessPacket(const void *data, int size);             // 0x00BC8AD8: stamps +0x2BB50, then drdp_process_packet
    void Send0063();                                            // 0x00BCCB88: +0x22B78 = -1, op 99
    int  GetTradeOfferIcon(int side, int i);                    // 0x00BCD248
    void GetBuddyRequestName(char *buf);                        // 0x00BCD9C8
    void MaybeStopAutoAttack();                                 // 0x00BD5B30: unless game option +0x95
    void PromptText(const unsigned short *text, int a);         // 0x00BD91D8

    int  SetMusic(const char *name);                            // 0x00BD8FB8: +0xC18C = -1, the name copied
    int  ExpansionFlags();                                      // 0x00BDD4E8: VIGetExpansionFlags() & +0x50304
    int  GetLootItemIcon(int i);                                // 0x00BCAE20
    int  GetMerchantItemIcon(int i);                            // 0x00BCC3F0
    int  GetTradeInvItemIcon(int i);                            // 0x00BCCE20
    void GetAbilityName(int i, int unused, unsigned short *buf, unsigned int size);  // 0x00BCBD70
    void GetAbilityDesc(int i, int unused, unsigned short *buf, unsigned int size);  // 0x00BCBDB0
    int  GetP2BE4C();                                           // 0x00BD7AD0: 0 unless grouped

    void SendOpSigned2Int(int op, int a, int b, unsigned int c);         // 0x00BE5478
    void MerchantRequest(int a, int b);                         // 0x00BCCA38
    void Tick(int net);                                         // 0x00BD4630
    void TickLocal();                                           // 0x00BC85F8
    void TickNet();                                             // 0x00BC8618
    int  IsInvItemUsable(int i);                                // 0x00BCCBB8
    int  GetTradeOfferName(int side, int i, unsigned short *buf, unsigned int size);// 0x00BCD278
    int  GetTradeOfferDesc(int side, int i, unsigned short *buf, unsigned int size);// 0x00BCD2C0

    int  GetLootItemName(int i, unsigned short *buf, unsigned int size);// 0x00BCAE88
    int  GetMerchantItemName(int i, unsigned short *buf, unsigned int size);// 0x00BCC430
    int  GetMerchantItemDesc(int i, unsigned short *buf, unsigned int size);// 0x00BCC480
    int  GetTradeInvItemName(int i, unsigned short *buf, unsigned int size);// 0x00BCCE60
    int  GetTradeInvItemDesc(int i, unsigned short *buf, unsigned int size);// 0x00BCCEB0
    void GetAbilityRecordName(int i, int j, unsigned short *buf, unsigned int size);// 0x00BCBD20
    void TradeSend2();                                          // 0x00BCD5B0
    void TradeSend6();                                          // 0x00BCD8E0
    void TradeSend7();                                          // 0x00BCD930

    void SendOpIntIntByte(int op, int a, unsigned int b, unsigned char c);// 0x00BE50B0
    void SendItemPair(int op, int a, int b);                    // 0x00BE4DE0
    void SendLoot(int op, int slot, int x);                     // 0x00BE4ED0
    void SendOpIntSignedInt(int op, int a, int b, unsigned int c);// 0x00BE51C0
    void RequestQuickSlot(int i, int pos);            // 0x00BCADB8
    void Send0062(int i);                                       // 0x00BCCC70
    void UseItem(int i, unsigned char b);                       // 0x00BCABA0
    void SwapItems(int i, int j);                               // 0x00BCAC08
    void LootOne(int i, int x);                                 // 0x00BCB038
    void SellItem(int i, int n);                                // 0x00BCCA80

    int  DisconnectKind();                                      // 0x00BC24C8
    int  CanUseItem(VINetItem *it);                             // 0x00BCA950
    int  SplitDigits(int v, int *thousands, int *hundreds, int *tens, int *ones);// 0x00BCAB18
    void ShowNotice(const unsigned short *text, int sound);     // 0x00BCE210
    int  GetQuestTitle(int i, unsigned short *buf);             // 0x00BCEFC0

    void BackToLogin();                                         // 0x00BD8990
    int  IsTimedOut();                                          // 0x00BDD488
    unsigned short *NewWide(const char *s);                     // 0x00BDDFF0
    void SendOpByteIntSigned(int op, unsigned char b, int c, int d);// 0x00BE5B20
    void TradeSend5(int x);                                     // 0x00BCD888

    void ProcessMessageF8(long src, VIMemStream &s);            // 0x00BD0D00
    void Msg0993(VIMemStream &s);                               // 0x00BDC120

    void ReadStats(VIMemStream &s, VIPlayer &p);                // 0x00BDCB30
    void Channel42(const void *data, int size);                 // 0x00BD3CB0
    void ProcessMessageF9(const void *data, int size);          // 0x00BD0D60

    void Channel43(const void *data, int size);                 // 0x00BD3D18

    void UpdateGroupMember(unsigned int id, unsigned char known, unsigned char hp, int x, const VIVect3 &pos);// 0x00BD4138
    void Channel40(const void *data, int size);                 // 0x00BD3FA0

    void Channel44(int type, const void *data, int size);       // 0x00BD3E40

    void ChannelEntity(int ch, const void *data, int size);     // 0x00BD41B8
    void ProcessStateChannel(int type, const void *data, int size);// 0x00BD3C18

    void ConnectShard(unsigned long id);                        // 0x00BC1DD0
    void ProcessMessageFC(const void *data, int size);          // 0x00BD3AF8

    void ResetWorld();                                          // 0x00BD0560
    void PumpMessages();                                        // 0x00BD4C10

    void ClosePanel6D0();                                       // 0x00BCDD10
    void OpenPanel34();                                         // 0x00BCE800
    void OpenPanel40();                                         // 0x00BCECD0
    void ClosePanel6C0();                                       // 0x00BCED18
    void OpenPanel35();                                         // 0x00BCEEA0
    void ClosePanel704();                                       // 0x00BCEEE8
    int  GetP44Total();                                         // 0x00BD60C0

    void MoveTo(int a, const VIVect3 &pos);                     // 0x00BD53A0
    void ReturnToSelect();                                      // 0x00BC2128
    void AddText(const unsigned short *text, VIColor32F color); // 0x00BD9160
    int  IsGroupFull();                                         // 0x00BD7A80
    void MoveToOrigin();                                        // 0x00BD5018
    void MoveToGrid(int a, int col, int row);                   // 0x00BD5068

    int  GetShardServer(char *host, unsigned short *port);      // 0x00BDD2B8
    void Connect(const char *host, unsigned short port);        // 0x00BC18C0
    int  ConnectRandomShard();                                  // 0x00BC20A8
    int  HasPet();                                              // 0x00BD93F0
    int  PetIsCreature();                                       // 0x00BD95C8
    int  GetPlayerName(char *buf, int size);                    // 0x00BD7728
    int  GetMerchantPrice(int i);                               // 0x00BCC950
    float GetInvItemCondition(int i);                           // 0x00BCC9C0
    int  GetInvItemRepairCost(int i);                           // 0x00BCCC00
    void ApplyCameraPreset(const VICameraPreset &p);            // 0x00BD5978
    void ApplyDefaultCamera();                                   // 0x00BC7B38

    int  SetCameraStyle(int style);                             // 0x00BD8B40: returns 0
    void SaveCameraPreset(VICameraPreset &p);                   // 0x00BD5940
    void ToggleCameraStyle();                                   // 0x00BD58C8
    int  LoadInvIcons();                                        // 0x00BD4FA0
    void LeaveGroup();                                          // 0x00BD7E00
    void DisbandGroup();                                        // 0x00BD8368

    int  FindInvByQuickSlot(int slot);                          // 0x00BC9EE0
    int  PetFlag4();                                            // 0x00BD9410
    int  PetFlag8();                                            // 0x00BD9490
    void UpdateTimeOfDay();                                     // 0x00BC7B70
    void ShowNetStats();                                        // 0x00BD4B90

    int  GetTargetName(char *buf);                              // 0x00BD7920
    int  GetTargetPlayerName(char *buf);                        // 0x00BD79B0
    void ShowTargetPlayerName(int a, int b);                    // 0x00BDD408
    int  SetNameEntry(int i, const char *name, unsigned short v);// 0x00BDD230
    float GetInvItemWear(int i);                                // 0x00BCC8C0
    int  FindEquipped(VIItemList *list, int slot, int *index);  // 0x00BDD0B0

    int  JournalConsole();                                      // 0x00BC8700
    int  JournalNetwork();                                      // 0x00BC89D0
    void JournalTick(int flags);                                // 0x00BC8660
    void ApplyOptions();                                        // 0x00BCE848
    int  GroupMemberFlag4(int i);                               // 0x00BD9208
    int  GroupMemberFlag8(int i);                               // 0x00BD92A8
    int  LeaderByte263();                                       // 0x00BD9350

    int  GetObjectName(unsigned int type, int i, unsigned short *buf, unsigned int size);// 0x00BC9968
    void GetAbilityList(VIArray<int> &out);           // 0x00BCB9D8
    int  GetInvItemSellPrice(int i);                            // 0x00BCC818
    void DropInvites(int id);                                   // 0x00BD7B30

    int  IsAbilitySlotSpell(int slot);                          // 0x00BCBE40

    int  GetPetHealth(float *hp);                               // 0x00BD9520
    void SendD4Off();                                            // 0x00BD9658
    int  CheckPower();                                          // 0x00BDC850
    int  AllocNames(int n);                                     // 0x00BDD140

    void Send006B(int x);                                       // 0x00BCEAF0
    void PlayAction(int action);                                // 0x00BCFDC8
    void InitCameraStyle();                                     // 0x00BD5E38

    void SendD4On();                                            // 0x00BD9700
    void PromptWithName(const unsigned short *name);            // 0x00BD97B0
    void ApplyRaceCamera();                                     // 0x00BC7A80

    void UpdateMusic();                                         // 0x00BD9010
    void GetAbilityRecords(int i, VIArray<int> &out); // 0x00BCBC60
    void SetKey(int key, int down);                   // 0x00BCF9C0
    void ToggleFollow();                                        // 0x00BD5D70

    void Send00FE();                                            // 0x00BCC118
    void Send007D(int x);                                       // 0x00BCF020
    int  GetStatTotal(int stat);                                // 0x00BC9F88
    int  IsRangedItem(int i, int list);                         // 0x00BC9E10

    void SendOpSignedSigned(int op, int a, int b);              // 0x00BE5390
    void SendOpSignedByte(int op, int a, unsigned char b);      // 0x00BE5578
    void SendOpSignedRaw(int op, int a, int b);                 // 0x00BE5660
    void SendOpByteSigned(int op, unsigned char a, int b);      // 0x00BE5D18
    void SendOpByteByte(int op, unsigned char a, unsigned char b);// 0x00BE5810
    void SendOpByteRaw(int op, unsigned char a, int b);         // 0x00BE5F10

    void LoadWorld(int world);                                  // 0x00BD50E8
    int  ReadAbilities(VIMemStream &s, VIMap<int, VIAbility> &map);// 0x00BDCDC0
    void Send006C(int a, const VIRec388 *r);                    // 0x00BCEBA0: one text-set entry (SetTextSet, id 0)
    void AnswerBuddyRequest();                                  // 0x00BCDBF8

    void EmptyBCDAE8();                                         // 0x00BCDAE8
    void Pointer(int x, int y);                                 // 0x00BCFA80: empty; Tick replays journal type 4 into it
    void TickHookB();                                           // 0x00BD4B88
    int  ReturnZero();                                          // 0x00BD9348
    void SetPlayerP38(int v);                                   // 0x00BCCB78
    void DeclineBuddyRequest();                                 // 0x00BCD9F8
    int  ClearMusic();                                          // 0x00BD8FF0
    void Send0041();                                            // 0x00BD9638
    int  GetFrameRate();                                        // 0x00BDD520
    void ClosePanel718();                                       // 0x00BCEF80
    void ClosePanel6BC();                                       // 0x00BD6228
    void Send0701a();                                           // 0x00BD9860
    void Send0701b();                                           // 0x00BD9888

    void CycleTarget(int group);                                // 0x00BC8B98: group mode cycles the player, the group and the pet
    void TargetNearest(int group);                              // 0x00BC8B68: no caller in the image; hands its argument on
    int  ClosePanel6AC();                                       // 0x00BCCB28
    void ShowA110(int x);                                       // 0x00BCE990
    void Send0012(int a, int b);                                // 0x00BD8A60
    void ShowNameClear(int a, int b);                           // 0x00BDD3D8

    void ToggleP74();                                           // 0x00BD5C70
    void DeclineInvite();                                       // 0x00BD7D00
    void RefreshCollision();                                    // 0x00BC7120

    void RejectBuddyRequest();                                  // 0x00BCDAF0
    void Send0033(int a, int b);                                // 0x00BCC010

    void SendOpU32(int op, unsigned int v);                     // 0x00BE63A8
    void InteractWithTarget();                                  // 0x00BD5B68
    void TrainStats(int *points);                               // 0x00BD6100
    void AcceptInvite();                                        // 0x00BD7BE8

    void UseItemOn(int i, int x);                               // 0x00BCAC90
    void Send0032(int i);                                       // 0x00BCBEE8
    void PrintPosition();                                       // 0x00BD5F60

    void Send006A(const VIRec6A *r);                            // 0x00BCE9C0
    void TradeSend4(int i);                                     // 0x00BCD750

    void SelectTarget(int which);                               // 0x00BC9730
    void DrawReticle(int flash);                                // 0x00BC95F0

    int  OpenResources();                                       // 0x00BD4E60
    void DeleteCharacter(int i);                                // 0x00BD8630

    void RequestTrade();                                        // 0x00BDA028
    void ToggleAutoAttack();                                    // 0x00BD59E8

    void TradeSend3(int i, int n);                              // 0x00BCD600
    void Send003A(int slot, int index);                         // 0x00BCC1F0

    void PlayEmote(int which);                                  // 0x00BCFC38
    int  CanUseListItem(int i, int list);                       // 0x00BCA970

    void PadKeys(const VIClientPart07190 *pad);                 // 0x00BCFA88

    void SetCharLook(int i, int face, int hairStyle, int hairLength, int hairColor);// 0x00BD8A90

    void LoadTextSets(VIArray<VIRec6A> &sets);                  // 0x00BDB8F8

    void EnterWorld(int world, const VIVect3 &pos);             // 0x00BD51C0
    void PetCommand(int cmd);                                   // 0x00BD7430

    int  HasAmmoFor(VINetItem *weapon);                         // 0x00BDC8F8

    int  SelectCharacter(int i);                                // 0x00BD8428

    int  EquipSlotFor(VIItemList *list, VINetItem *item);       // 0x00BDCEA0

    void AddBuddy(const char *name);                            // 0x00BDB1D0

    void InviteToGroup(VIInviteRec *who);                       // 0x00BD8128

    void SendNewChar(int op, const VINewChar *c);               // 0x00BE6268
    void CreateCharacter(VINewChar *c);                         // 0x00BD8770

    void Extrapolate();                                         // 0x00BC7478

    const unsigned short *ConnectStatusString();                // 0x00BC2268

    void DrawFade(int force);                                   // 0x00BC55D8

    void DrawPosition();                                        // 0x00BC5840

    int  GearStat(VIItemList *list, int stat);                  // 0x00BCA060

    void EquipItem(VIItemList *list, int i, int slot);          // 0x00BCB610

    void InviteEntity(int id);                                  // 0x00BD7E78

    int  SetTextSet(int id, const char *l0, const char *t0, const char *l1, const char *t1,
                    const char *l2, const char *t2, const char *l3, const char *t3);   // 0x00BDDCF0

    void TargetCentre();                                        // 0x00BC92B0

    void DrawNetStats();                                        // 0x00BC5C38

    void BeginManualCamera();                                   // 0x00BCE270

    int  TargetInRange(int slot);                               // 0x00BDC4B8

    void AutoAttackTick(VIEntity *me);                          // 0x00BC76A0

    void DrawPingBar(const char *image, const VIVect2I &at, float h);    // 0x00BC5AC8: no caller in the image
    int  FindFullStack(VIItemList &list, VINetItem *item, int *slot); // 0x00BCA840

    void UpdateCreatures();                                     // 0x00BC72C0

    void DeathCamera();                                         // 0x00BC6118

    int  EnterGameLoad();                                       // 0x00BC0F28

    // The network opcode dispatcher (DRDP message type 0xFB), name provisional.
    void ProcessMessage(long src, const void *data, int size);  // 0x00BD0DC8

    // Its handlers (provisional names; MsgXXXX handles opcode XXXX).
    void Msg000D(int src, VIMemStream &s);                              // 0x00BD0630: 0x000D: world entry (DSP sections, memory dump)
    void Msg0A7B(VIMemStream &s);                                       // 0x00BD3600: a wide string in an RGB colour
    void Msg0A7A(VIMemStream &s);                                       // 0x00BD36D0
    void Msg0772(VIMemStream &s);                                       // 0x00BD31C0
    void Msg0773(VIMemStream &s);                                       // 0x00BD3458: a group invite
    void Msg0774(VIMemStream &s);                                       // 0x00BD3548: drops one m_p2BD58 record by id, word 635
    int  InGroup();                                                     // 0x00BD7A68: returns +0x2BD74
    void GroupChanged(unsigned char state);                             // 0x00BD9B48: stores +0x382E4, sends op 14 with it, word 595+state; the group-reset tail's call when it is 2
    int  PlaySound(int id);                                             // 0x00BD8E78: m_ui sound (returns SoundPlay's result)
    void Msg0034(VIMemStream &s);                                       // 0x00BDBD58
    void ShowDialog(int id, const unsigned short *text, int a, int b);  // 0x00BDBC70: 0x0046; queued (12 deep) while the game window is not up; id and a unused
    void CharacterList(VIMemStream &s);                                 // 0x00BD2E80: 0x002C: up to 8 records, then the login window
    void Msg0777();                                                     // 0x00BD89F0
    void AddInvItem(VINetItem &item);                                   // 0x00BCB0D8: 0x003B
    void Msg003C(int slot, int n, unsigned char sound);                 // 0x00BCB138: takes n of a slot; the merchant sound when asked
    void Msg003D(int a, int b);                                         // 0x00BCB208
    void Msg003E(int slot, int charge);                                 // 0x00BCB228: use up a charge, or remove; sound 5 for a weapon, 1 otherwise
    void Msg003F(int a, int b);                                         // 0x00BCB2F0
    void Msg0040(int slot);                                             // 0x00BCB8A0: unequips equip slot n, with its sound
    void Msg0019(int a, int b);                                         // 0x00BD99A0
    void Msg001A();                                                     // 0x00BD98B0: loot everything
    void LootNoWindow();                                                // 0x00BCB098: 0x0018's else arm
    void CastingCombat(unsigned int caster, unsigned int target, unsigned char n, int *targets, int time, int effect);  // 0x00BD3738: 0x00B1
    void Msg00B3(VIMemStream &s);                                       // 0x00BCF3A0
    void Msg00B4(int a, int b);                                         // 0x00BCF0F8
    void Msg00FF(VIMemStream &s);                                       // 0x00BCF2D0: invalidates every ability (display index < 0xFC)
    void Msg00B5(int from, int to);                                     // 0x00BCF5A8: moves a spellbook slot, shifting the rest
    void Msg00B6(int from, int to);                                     // 0x00BCF730: moves a display index (< 0xFC), shifting the rest
    void Msg00FD(VIMemStream &s);                                       // 0x00BCF8B0: sets one ability's +0x14 by id; 0 invalidates it
    void Msg00B8(int z);                                                // 0x00BCF468
    void Msg00CD(int a, int z);                                         // 0x00BCF500
    void Msg00B2(VIMemStream &s);                                       // 0x00BD38C8: respawn at a world/position; answers 73
    void MerchantOpen();                                                // 0x00BD9A48: 0x00B7
    void Msg076F(VIMemStream &s);                                       // 0x00BDB3E8: a buddy request; answered 872 (1: no window, 8: list full)
    void Msg0770(const char *name);                                     // 0x00BDB650
    void Msg0771(unsigned char type, const char *name);                 // 0x00BDB6B8: a word by type (3 and 4 leave it unset), formatted with the name
    void Msg07E0(VIMemStream &s);                                       // 0x00BDC308
    void CharSelectMsg(int id);                                         // 0x00BD30E8: 0x07F0-0x07F4
    void Msg07A1(VIMemStream &s);                                       // 0x00BD3158: reads 8 bytes, Disconnect(13, 0)
    void Msg07A2(VIMemStream &s);                                       // 0x00BD31A0: Disconnect(14, 0)
    void QuestStage(unsigned char flag, int delta);                                  // 0x00BD9AA8: 0x0036
    void Msg001D(int z);                                                // 0x00BDCAF0
    void Msg000A(unsigned char slot, unsigned char pos);                // 0x00BD9C18: puts an item in quick slot 0..3, clearing the slot's last holder
    int  FitsSlot3(VINetItem *it);                                      // 0x00BDC470, name provisional: by wear slot (12, 14-16, 18, 20)
    void Msg0010(VIMemStream &s);                                       // 0x00BD9D60: target info, kept only for the current target
    void Msg0776(int a, int b, int c);                                  // 0x00BD9E50
    int  IsMerchantOpen();                                              // 0x00BCC368, name provisional: m_p1B300 != -1 (AddInvItem)
    void SetActiveWindow(VIWnd *w);                                     // 0x00BD7618, name provisional (FrontiersForge window.lua)
    void SendOpIntInt(int op, int a, unsigned int b);                   // 0x00BE4FC0, name provisional: opcode and two ints (Msg007B: 16, target info)
    void SendOpByteInt(int op, unsigned char b, int v);                 // 0x00BE5C28, name provisional: opcode, a byte, an int
    void SetFade(float to, float time);                                 // 0x00BC7098, name provisional
    int  UnloadWorld();                                                 // 0x00BC1498, name provisional
    int  LoadWorld();                                                   // 0x00BC0730, name provisional: < 0 is fatal
    void ShowLoginWindow();                                             // 0x00BC2180, name provisional
    static void LoadingProgress(void *client);                          // 0x00BBF0D8, name provisional: VILoader's progress callback
    int  Msg0777Done(int mode);                                         // 0x00BC2528, name provisional: 2 back to login, 3 quit, 4 quit and down the interface
    void TradeName(char *buf);                                          // 0x00BCCCB8, name provisional (Trade20)
    void ResetView();                                                   // 0x00BCE5D8, name provisional: the walker and camera back to the view's heading (QuestStage, while modal)
    void SendMsg(int op, int v, unsigned char b);                       // 0x00BE4CF0, name provisional: opcode, an int, a byte (Reset: op 1572)
    void Msg0790Done();                                                 // 0x00BDE050, name provisional (Msg0790): sends 0x0992 to the stored address, at most once per connection timeout
    void TradeSetup();                                                  // 0x00BCCD58, name provisional (Trade02)
    void TradeReset();                                                  // 0x00BDA168, name provisional (Trade16)
    void StopAutoAttack();                                              // 0x00BD5B60: +0x71E4 = 0 (FrontiersForge)
    void SendOpcode(int op);                                            // 0x00BE4B78: an empty message, just the opcode (0x077A sends 1915, LootNoWindow 22)
    void Msg0057(int partner, unsigned int id);                         // 0x00BDA1A8: a trade request; no window, decline (86, 20)
    void Trade02(int id);                                               // 0x00BDA2C0: 0x0056 sub 2
    void Trade01();                                                     // 0x00BDA300: 0x0056 sub 1
    void Trade20();                                                     // 0x00BDA320: 0x0056 sub 20
    void Trade16();                                                     // 0x00BCCDC8: 0x0056 sub 16
    void Trade17();                                                     // 0x00BDA3B8: 0x0056 sub 17
    void Trade21();                                                     // 0x00BDA418: 0x0056 sub 21
    void Trade22();                                                     // 0x00BDA478: 0x0056 sub 22
    void Trade07();                                                     // 0x00BDA4D8: 0x0056 sub 7
    void Trade06();                                                     // 0x00BDA500: 0x0056 sub 6
    void TradeA(unsigned char sub, VIMemStream &s);                     // 0x00BDA528: 0x0056 default, +0x2BB04 set
    void TradeB(unsigned char sub, VIMemStream &s);                     // 0x00BDA688: 0x0056 default, +0x2BB04 clear
    void TradeAddSlot(int a, int b);                                    // 0x00BDA7E8, name provisional (TradeA 3, TradeB 8)
    void TradeRemoveSlot(int a);                                        // 0x00BDAB68, name provisional (TradeA 4, TradeB 9)
    void TradeSetMoney(int money);                                      // 0x00BDAE78, name provisional: +0x2BAF0, and what is left at +0x2BAEC
    void TradeAddItem(VINetItem &item);                                 // 0x00BDAA10, name provisional (TradeA 8, TradeB 3)
    void TradeRemoveItem(int id);                                       // 0x00BDAD08, name provisional: by the trade item's id
    void Msg005E();                                                     // 0x00BDAEB8: the player died
    void Msg0060(int slot, int hp);                                     // 0x00BCB988
    void Msg0061(int a);                                                // 0x00BDB048
    void Msg0063();                                                     // 0x00BDB0A8
    void SetTime(long a, long b);                                       // 0x00BD0508: 0x0013
    void Msg0045(int on);                                               // 0x00BDB820: "<word 550>: " + word 551 or 552
    void Msg006E();                                                     // 0x00BDB8F0: an empty function
    void Msg007B(unsigned int id);                                      // 0x00BDBAC8: -1 is lui+ori, so unsigned
    void Msg007C(const unsigned short *text);                           // 0x00BDBB30
    void Msg007D(int i);                                                // 0x00BDBB98: removes quest title i
    void SetSpeed(float v);                                             // 0x00BDD540: 0x00F8
    void Msg00D3(const char *name, unsigned char c);                    // 0x00BDB0E8: "<name> " + word 610/611 in a prompt
    void Msg1003Done();                                                 // 0x00BC7218: after the 0x1003-0x1005 sub-object call
    void Msg0790(VIMemStream &s);                                       // 0x00BDC080
    void SetWeather(unsigned char w, bool instant);                     // 0x00BD9F00: 0x00C5 (0) / 0x00C7 (1); 7+ is clear
    void Msg00D7();                                                     // 0x00BD9FD8
    void DamageFromSelf(int defender, int amount);                      // 0x00BDC220: 0x00DB
    void DamageFromOther(int attacker, int amount);                     // 0x00BDC288: 0x00DB

    VIUI          m_ui;                                 // +0x00004: VIWnd::mUI points here
    char          m_g_m_p00058[0x00058 - (0x00004 + sizeof(VIUI))];
    int           m_p00058;                             // +0x00058: client state: Disconnect -1; 0x002C: 2..3 -> 3
    bool          m_quit;                               // +0x0005C: IsRunning's xori
    char          m_g00060[0x00064 - (0x0005C + sizeof(bool))];
    int           m_p00064;                             // +0x00064: UpdateTimeOfDay runs only when set
    int           m_p00068;                             // +0x00068: EnterGameLoad zeroes it
    float         m_timeOfDay;                          // +0x0006C: hours, from m_clockB modulo one real hour
    int           m_p00070;                             // +0x00070: MoveTo zeroes it with the next two
    int           m_p00074;                             // +0x00074: Msg005E zeroes it
    int           m_p00078;                             // +0x00078: Msg005E zeroes it
    unsigned int  m_p0007C;                             // +0x0007C: ToggleFollow stores the group member it follows
    int           m_p00080;                             // +0x00080: Disconnect zeroes it
    char          m_g00084[0x00090 - (0x00084)];
    VIJournal     m_journal;                            // +0x00090: JournalTick records into it
    char          m_g_m_p00190[0x00190 - (0x00090 + sizeof(VIJournal))];
    VICamera      m_camera;                             // +0x00190: MoveTo / SetSky; its synthesized ctor is out of line (0x00BE3398)
    char          m_g_m_raster[0x00380 - (0x00190 + sizeof(VICamera))];
    VIRaster      m_raster;                             // +0x00380: m_ui.m_raster points here
    char          m_g_m_sound[0x04FE0 - (0x00380 + sizeof(VIRaster))];
    VISoundDevice m_sound;                              // +0x04FE0
    char          m_g_m_collide[0x06160 - (0x04FE0 + sizeof(VISoundDevice))];
    VICollide     m_collide;                            // +0x06160: UnloadWorld resets it; the scene's collision
    char          m_g_m_console[0x06390 - (0x06160 + sizeof(VICollide))];
    VIConsole     m_console;                            // +0x06390: Shutdown closes it
    char          m_g_m_dict[0x06618 - (0x06390 + sizeof(VIConsole))];
    VIDictionary  m_dict;                               // +0x06618: m_ui.m_dict points here
    bool          m_p0662C;                             // +0x0662C: IsNotP0662C: `!m_p0662C` (xori)
    int           m_worldId;                            // +0x06630: the world; -1 = none (FrontiersForge util.lua)
    char          m_g_m_loader[0x06638 - (0x06634)];
    VILoader      m_loader;                             // +0x06638
    VIClientPart06D00 m_p06D00;                         // +0x06D00: OpenResources inits it
    char          m_g_m_p06D50[0x06D50 - (0x06D00 + sizeof(VIClientPart06D00))];
    VIClientPart06D50 m_p06D50;                         // +0x06D50: a CLIENT object (the roll-call)
    char          m_g06D51[0x06D90 - (0x06D50 + sizeof(VIClientPart06D50))];
    VICameraPreset m_camPreset;                         // +0x06D90: ApplyDefaultCamera applies it
    VICameraPreset m_camSaved;                          // +0x06DA8: ToggleCameraStyle saves the camera into it
    char          m_g_m_p06DC0[0x06DC0 - (0x06DA8 + sizeof(VICameraPreset))];
    int           m_p06DC0;                             // +0x06DC0
    int           m_p06DC4;                             // +0x06DC4
    int           m_p06DC8;                             // +0x06DC8: 0x00FC stores a byte here
    char          m_g06DCC[0x06DD0 - (0x06DCC)];
    VIWalkCamera  m_walkCam;                            // +0x06DD0: a whole VIWalkCamera (FrontiersForge camera.lua)
    unsigned char m_keys[256];                          // +0x07090: SetKey records each key's state (the camera ends here)
    VIClientPart07190 m_p07190;                         // +0x07190: GetP07190 returns its address
    char          m_g07194[0x071E4 - (0x07190 + sizeof(VIClientPart07190))];
    unsigned char m_autoAttack;                         // +0x071E4: auto-attack on; ClearP071E4 is `sb zero` (FrontiersForge player.lua)
    char          m_g071E5[0x071E8 - (0x071E5)];
    long          m_p071E8;                             // +0x071E8: ToggleAutoAttack's last 0x000F time
    long          m_p071F0;                             // +0x071F0: AutoAttackTick: the time a weapon's effect last played
    unsigned int  m_selfId;                             // +0x071F8: own entity id (0x0775, 0x00DB); FrontiersForge 0x1FB6928; -1 is lui+ori
    VIVect3       m_entVel[24];                         // +0x071FC: Extrapolate adds it, scaled, to an entity's position
    VIVect3       m_entRot[24];                         // +0x0731C: ... and this to its heading
    long          m_entStamp[24];                       // +0x07440: ... by (this - m_p07568) / (2 * gEntityUpdateRate)
    VIEntity *    m_entities[24];                       // +0x07500: Disconnect deletes them; FindEntity
    drdp_t *      m_drdp;                               // +0x07560
    connection_t * m_conn;                              // +0x07564
    long          m_p07568;                             // +0x07568: 64-bit (ld)
    long          m_p07570;                             // +0x07570: 64-bit (sd of li -1)
    int           m_p07578;                             // +0x07578: 0x07B2; the ctor sets 100
    char          m_g0757C[0x07580 - (0x0757C)];
    long          m_p07580;                             // +0x07580: Connect: the clock
    long          m_p07588;                             // +0x07588: Connect: the clock
    VIScene       m_scene;                              // +0x07590
    unsigned int  m_target;                             // +0x0C140: the target's entity id, -1 = none (lui+ori; FrontiersForge)
    int           m_pC144;                              // +0x0C144: EnterGameLoad sets -1
    int           m_reticleTex;                         // +0x0C148: DrawReticle's texture, -1 until AT_Reticle.tga is found
    int           m_ringsIndex;                         // +0x0C14C: "Selection_Rings"'s dictionary index
    char          m_g0C150[0x0C168 - (0x0C150)];
    float         m_fadeCur;                            // +0x0C168: SetFade
    float         m_fadeTo;                             // +0x0C16C: SetFade
    float         m_fadeRate;                           // +0x0C170: SetFade: (to - cur) / time, 0 when immediate
    int           m_fadeMat;                            // +0x0C174: DrawFade's solid-fill material, -1 until made
    VIColor32F    m_fadeColor;                          // +0x0C178: GetPC178 returns its address
    float         m_musicFade;                          // +0x0C188: StopMusic zeroes it; UpdateMusic starts the track at <= 0
    int           m_pC18C;                              // +0x0C18C: StopMusic stores -1 (li: a signed int)
    char          m_musicName[32];                      // +0x0C190: SetMusic copies it; StopMusic clears it
    int           m_pC1B0;                              // +0x0C1B0: Notify zeroes it (FrontiersForge: "dead"); ResetCamera sets it
    VIVect3       m_camPos;                             // +0x0C1B4: ResetCamera: the camera's position ...
    VIVect3       m_camDir;                             // +0x0C1C0: ... and direction, kept
    float         m_camDist;                            // +0x0C1CC: ... the player's level distance from it
    float         m_pC1D0;                              // +0x0C1D0: ... zeroed
    float         m_camYaw;                             // +0x0C1D4: ... -atan2(dir.z, dir.x)
    int           m_loadSprite;                         // +0x0C1D8: LoadingProgress renders this scene sprite
    char          m_worldPath[0x100];                   // +0x0C1DC: LoadWorld's path when m_p0662C is set (a world not on the disc)
    char          m_g_m_player[0x0C2DC - (0x0C2DC)];
    VIPlayer      m_player;                             // +0x0C2DC: the player's own record (FrontiersForge)
    VIItemList    m_inventory;                          // +0x0C428: 0x0060
    char          m_g_m_p13B90[0x13B90 - (0x0C428 + sizeof(VIItemList))];
    int           m_p13B90;                             // +0x13B90: 0x00D7 zeroes it; window code reads and writes it
    unsigned char HaveCorpse;                           // +0x13B94: named by the assert (0x0018)
    char          m_g13B95[0x13B98 - (0x13B95)];
    VIItemList    m_corpse;                             // +0x13B98: 0x0018 / 0x0016
    char          m_g_m_p1B300[0x1B300 - (0x13B98 + sizeof(VIItemList))];
    unsigned int  m_p1B300;                             // +0x1B300: 0x00B7; the merchant's entity id, -1 = none (lui+ori; FrontiersForge)
    int           m_p1B304;                             // +0x1B304: zeroed by 0x00D7, AddInvItem, MerchantOpen
    int           m_p1B308;                             // +0x1B308: 0x00B7
    int           m_p1B30C;                             // +0x1B30C: 0x00B7
    VIItemList    m_merchant;                           // +0x1B310: 0x00B7
    char          m_g_m_merchantName[0x22A78 - (0x1B310 + sizeof(VIItemList))];
    unsigned short m_merchantName[128];                 // +0x22A78: 0x00B7
    unsigned int  m_p22B78;                             // +0x22B78: 0x0063 stores -1 (lui+ori: unsigned)
    VIItemList    m_tradeInv;                           // +0x22B7C: TradeSetup copies m_inventory here
    VITradeItem   m_tradeB[4];                          // +0x2A2E4: TradeItem(side != 0, i)
    int           m_tradeBCount;                        // +0x2AEE4: TradeSetup/TradeReset zero it
    VITradeItem   m_tradeA[4];                          // +0x2AEE8: TradeItem(0, i)
    int           m_tradeACount;                        // +0x2BAE8: TradeSetup/TradeReset zero it
    int           m_p2BAEC;                             // +0x2BAEC: TradeSetup copies m_player.m_p34 here
    int           m_p2BAF0;                             // +0x2BAF0..+0x2BAFC: TradeSetup/TradeReset zero them
    int           m_p2BAF4;
    int           m_p2BAF8;                             // +0x2BAF8: the next running id on m_tradeB (TradeAddSlot)
    int           m_p2BAFC;                             // +0x2BAFC: the next running id on m_tradeA (TradeAddItem)
    unsigned int  m_p2BB00;                             // +0x2BB00: TradeName's entity; -1 (lui+ori)
    int           m_p2BB04;                             // +0x2BB04: 0x0056's default arm
    unsigned int  m_tradePartner;                       // +0x2BB08: 0x0056; TradeReset shares the lui+ori -1 with +0x2BB00
    unsigned int  m_p2BB0C;                             // +0x2BB0C: 0x007C / 0x007D; -1 is lui+ori (0x00BC0680)
    unsigned int  m_p2BB10;                             // +0x2BB10: Msg076F's buddy requester; -1 (lui+ori) once answered
    char          m_buddyRequestName[24];               // +0x2BB14: Msg076F: the requester
    VIMap<int, VIAbility> m_abilities;                  // +0x2BB2C: 0x00B3..0x00FF's ability map (FrontiersForge)
    long          m_p2BB40;                             // +0x2BB40: Msg005E zeroes it
    long          m_p2BB48;                             // +0x2BB48: Msg005E zeroes it
    long          m_p2BB50;                             // +0x2BB50: ProcessPacket stamps the clock here
    long          m_p2BB58;                             // +0x2BB58: ShowNetStats passes it
    int           m_p2BB60;                             // +0x2BB60: 0x00B4/B5/B6/B8/CD
    int           m_targetInfoValid;                    // +0x2BB64: 0x0010 sets 1 (FrontiersForge entity.lua)
    int           m_p2BB68;                             // +0x2BB68
    int           m_targetDisposition;                  // +0x2BB6C: 0..6 (0x0010)
    int           m_p2BB70;                             // +0x2BB70: a reset sets 2
    int           m_p2BB74;                             // +0x2BB74: a reset sets 1
    VIWndGame *   m_wndGame;                            // +0x2BB78: ShowText's window
    VIWnd2BB7C *  m_wnd2BB7C;                           // +0x2BB7C: ReturnToSelect activates it; ConnectShard reads the account
    VIWnd *       m_wnd2BB80;                           // +0x2BB80: ShowLoginWindow activates it
    VIWndLogin *  m_wndLogin;                           // +0x2BB84: the character-select message window
    VIWnd2BB88 *  m_wnd2BB88;                           // +0x2BB88: 0x0012
    VIWnd *       m_p2BB8C;                             // +0x2BB8C: SetActiveWindow: the loader progress callback runs while it is active
    char          m_g2BB90[0x2BB98 - (0x2BB90)];
    VIWnd2BB98    m_p2BB98;                             // +0x2BB98: a window; Disconnect compares +0x2BD40 to it
    VIWnd *       m_p2BD40;                             // +0x2BD40: the active window (SetActiveWindow)
    VIWnd *       m_p2BD44;                             // +0x2BD44: Notify / CharSelectMsg store the window they activate
    int           m_p2BD48;                             // +0x2BD48: UpdateCreatures walks the creatures while set
    int           m_p2BD4C;                             // +0x2BD4C: Extrapolate moves nothing while it is clear
    int           m_p2BD50;                             // +0x2BD50: UnloadWorld zeroes it
    char          m_g2BD54[0x2BD58 - (0x2BD54)];
    VIPool<VIRec32_BE2DE8> m_p2BD58;                    // +0x2BD58: Reset drains it; fills the gap to +0x2BD70
    unsigned int  m_p2BD70;                             // +0x2BD70: group reset: -1; unsigned (lui+ori, phase 39 lever)
    int           m_inGroup;                            // +0x2BD74: InGroup returns it
    int           m_p2BD78;                             // +0x2BD78: group reset: 0
    VIPartyMember m_group[4];                           // +0x2BD7C: exactly to +0x2BE4C (FrontiersForge group.lua)
    int           m_p2BE4C;                             // +0x2BE4C: group reset: 0
    unsigned int  m_petId;                              // +0x2BE50: 0x007E / 0x007F (unsigned: its -1 is lui+ori; FrontiersForge: the pet)
    char          m_petName[24];                        // +0x2BE54: 0x007F clears it; GetPetName; exactly to +0x2BE6C
    int           m_charCount;                          // +0x2BE6C: 0x002E / 0x0065 / 0x002F decrement it
    VICharRecord  m_chars[8];                           // +0x2BE70: CharacterList
    int           m_effectCount;                        // +0x2C170: active effects (state channel 0x43; FrontiersForge)
    unsigned int  m_effectIcon[8];                      // +0x2C174
    unsigned short m_effectName[8][64];                 // +0x2C194
    char          m_g2C594[0x2C598 - (0x2C594)];
    long          m_p2C598;                             // +0x2C598: EnterGameLoad stores the clock
    char          m_g2C5A0[0x2C6E0 - (0x2C5A0)];
    unsigned short m_questTitle[8][128];                // +0x2C6E0 (FrontiersForge quest_log.lua)
    int           m_questCount;                         // +0x2CEE0: 0x007C / 0x007D
    float         m_speed;                              // +0x2CEE4: 0x00F8
    VIChannelRec  m_channels[40];                       // +0x2CEE8: DRDP state channels 0x44..0x6B, one each (Channel44)
    VIArray<VIRec436_BDF6D8> m_dialogs;                 // +0x2E7E8: ShowDialog queues 12 lines here (head/tail below)
    unsigned char m_dialogHead;                         // +0x2E7F0: the dialog queue's read index; Disconnect zeroes it
    unsigned char m_dialogTail;                         // +0x2E7F1: its write index (ShowDialog); Disconnect zeroes it
    char          m_g2E7F2[0x2E7F4 - (0x2E7F2)];
    VIDialogBox   m_dialog;                             // +0x2E7F4: the dialog waiting for an answer (Msg0034)
    int           m_p2EBFC;                             // +0x2EBFC: set while m_dialog waits; Disconnect zeroes it
    char          m_g2EC00[0x2EC30 - (0x2EC00)];
    VISyncTime    m_clockA;                             // +0x2EC30: SetTime
    VISyncTime    m_clockB;                             // +0x2EC50: SetTime
    VIMap<int, VIBuddy> m_buddies;                      // +0x2EC70: 0x03D5 / 0x03D6
    char          m_g_m_guild[0x2EC84 - (0x2EC70 + sizeof(VIMap<int, VIBuddy>))];
    VIClientGuild m_guild;                              // +0x2EC84: vtable slot 4
    char          m_g_m_p2EF24[0x2EF24 - (0x2EC84 + sizeof(VIClientGuild))];
    VIClientPart2EF24 m_p2EF24;                         // +0x2EF24: slot 5
    char          m_g_m_p2EFC8[0x2EFC8 - (0x2EF24 + sizeof(VIClientPart2EF24))];
    VIClientPart2EFC8 m_p2EFC8;                         // +0x2EFC8: slot 18
    VIClientPart2EFCC m_p2EFCC;                         // +0x2EFCC: slot 6
    VIClientPart2EFD0 m_p2EFD0;                         // +0x2EFD0: slot 7
    char          m_g_m_p33950[0x33950 - (0x2EFD0 + sizeof(VIClientPart2EFD0))];
    VICSClient m_p33950;                         // +0x33950: slot 8
    char          m_g_m_p382E4[0x382E4 - (0x33950 + sizeof(VICSClient))];
    unsigned char m_p382E4;                             // +0x382E4: group reset: == 2 -> GroupChanged(0)
    char          m_g382E5[0x382E8 - (0x382E5)];
    VIClientPart382E8 m_p382E8;                         // +0x382E8: slot 10
    char          m_g_m_p38300[0x38300 - (0x382E8 + sizeof(VIClientPart382E8))];
    VIClientPart38300 m_p38300;                         // +0x38300: slot 9
    char          m_g38301[0x38320 - (0x38300 + sizeof(VIClientPart38300))];
    VIShardInfo   m_shardInfo;                          // +0x38320: VIClient's ctor builds it there
    char          m_g_m_p38334[0x38334 - (0x38320 + sizeof(VIShardInfo))];
    VIClientPart38334 m_p38334;                         // +0x38334: slot 11
    char          m_g_m_p3834C[0x3834C - (0x38334 + sizeof(VIClientPart38334))];
    VIClientPart3834C m_p3834C;                         // +0x3834C: slot 12
    char          m_g_m_p3FC10[0x3FC10 - (0x3834C + sizeof(VIClientPart3834C))];
    VIClientPart3FC10 m_p3FC10;                         // +0x3FC10: slot 13
    char          m_g_m_bank[0x47388 - (0x3FC10 + sizeof(VIClientPart3FC10))];
    VIBankControl m_bank;                               // +0x47388: slot 3; VIBankControl.cpp
    char          m_g_m_p4EC58[0x4EC58 - (0x47388 + sizeof(VIBankControl))];
    VIClientPart4EC58 m_p4EC58;                         // +0x4EC58: slot 14
    char          m_g_m_p4F078[0x4F078 - (0x4EC58 + sizeof(VIClientPart4EC58))];
    VIClientPart4F078 m_p4F078;                         // +0x4F078: slot 15
    char          m_g_m_p4F098[0x4F098 - (0x4F078 + sizeof(VIClientPart4F078))];
    char          m_p4F098[0x4F0B0 - 0x4F098];          // +0x4F098: GetP4F098 returns its address
    VINameEntry * m_p4F0B0;                             // +0x4F0B0: FreeBuffer frees it
    int           m_p4F0B4;                             // +0x4F0B4: FreeBuffer zeroes it
    unsigned short m_p4F0B8[24];                        // +0x4F0B8: 0x07A3 (str8to16)
    int           m_p4F0E8;                             // +0x4F0E8: Disconnect's reason; 0x002C sets 21
    int           m_p4F0EC;                             // +0x4F0EC: Shutdown stores -1; the pump's 0xFD stores the reason byte
    long          m_p4F0F0;                             // +0x4F0F0: 0x0790 reads 8 bytes
    int           m_p4F0F8;                             // +0x4F0F8: 0x0790 reads 4; a type-0xF8 0x0993 carrying it is echoed as 0x0791
    long          m_p4F100;                             // +0x4F100: 0x0790 reads 8
    long          m_p4F108;                             // +0x4F108: 0x0790 stores -1 (sd)
    unsigned char m_p4F110;                             // +0x4F110: Msg0777Done: set, the world is not reloaded
    unsigned char m_worldLoaded;                        // +0x4F111: Msg0777Done: set, UnloadWorld first
    char          m_g4F112[0x4F12C - (0x4F112)];
    int           m_exitCode;                           // +0x4F12C: client_main returns it
    int           m_p4F130;                             // +0x4F130: Disconnect tests it
    char          m_shardOverride[256];                 // +0x4F134: "host[:port]", used before the shard list when set
    VIRandom      m_random;                             // +0x4F234: slot 16; GetShardServer seeds it with the clock
    VIBattleMusic m_battleMusic;                        // +0x4FC00: Unload / StopNow
    int           m_p4FC20;                             // +0x4FC20: Init stores b != 0; ConnectShard sends it (lw then sb in the target), failure timeouts when set
    VIClientText  m_text;                               // +0x4FC24: ShowDialog expands through it
    unsigned short m_p4FC28[64];                        // +0x4FC28: GetP4FC28 returns it
    unsigned short m_p4FCA8[64];                        // +0x4FCA8: GetP4FCA8 returns it
    VIClientPart4FD28 m_p4FD28;                         // +0x4FD28: DamageFromSelf
    char          m_g_m_p502D4[0x502D4 - (0x4FD28 + sizeof(VIClientPart4FD28))];
    int           m_p502D4;                             // +0x502D4: SelectCharacter sets it
    int           m_p502D8;                             // +0x502D8: SelectCharacter sets it
    unsigned char m_p502DC[32];                         // +0x502DC: ConnectShard sends it in place of an empty password
    char          m_g502FC[0x50300 - (0x502FC)];
    int           m_p50300;                             // +0x50300: CharacterList: set, the login window gets Option(1)
    int           m_p50304;                             // +0x50304: 0x07D1
    int           m_p50308;                             // +0x50308: SetP50308 sets 1
    int           m_p5030C;                             // +0x5030C: SetP50308
    int           m_p50310;                             // +0x50310: SetP50308
    int           m_p50314;                             // +0x50314: SetP50308
    char          m_g50318[0x50320 - (0x50318)];
    long          m_p50320;                             // +0x50320: SetKey ignores keys until VITimer::GetTime reaches it
    char          m_g50328[0x50330 - (0x50328)];
};

#endif
