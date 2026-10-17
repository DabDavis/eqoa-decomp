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
class VIClientI;                                   // the subsystems' constructors take it
class VIClientPart4F078;
class VIClient;

// Every window class CLIENT's RTTI names (vtables 0x00CE0D58 .. 0x00CF1BE0), so any header
// can name any other by pointer.  Each is defined in its TU's header
// (cl_w*.h, included at the end of this file), except VIWndDisconnect, which VIClient
// embeds by value and so is defined here.
class VIWndAuction; class VIWndAuctionSearch; class VIWndBank; class VIWndBankItem;
class VIWndBankMenu; class VIWndBankMoney; class VIWndBlacksmith; class VIWndBuddy;
class VIWndBuffPC; class VIWndCharacterSelect; class VIWndCharCreateBlank; class VIWndCharCreateClass;
class VIWndCharCreateMisc; class VIWndCharCreateName; class VIWndCharCreateRace; class VIWndCharCreateReview;
class VIWndCharCustFrontiers; class VIWndChat; class VIWndClassMastery; class VIWndClassMasteryInspect;
class VIWndClassMasteryPoints; class VIWndClassMasteryRequire; class VIWndComposeGram; class VIWndCSInbox;
class VIWndCSRequest; class VIWndDeposit; class VIWndDisconnect; class VIWndEdit;
class VIWndExperiment; class VIWndFaction; class VIWndFishing; class VIWndGame;
class VIWndGoto; class VIWndGramInbox; class VIWndGuild; class VIWndHUDMenu;
class VIWndIdle; class VIWndIgnoreList; class VIWndInfluence; class VIWndInitFromMc;
class VIWndInspect; class VIWndItemMenu; class VIWndList; class VIWndLogin;
class VIWndLoot; class VIWndMainMenu; class VIWndMBox; class VIWndMenu;
class VIWndMenu4; class VIWndMenu4Edit; class VIWndMerchant; class VIWndMercMenu;
class VIWndMessage; class VIWndMiniMenu; class VIWndMoneyMenu; class VIWndNetStat;
class VIWndOptionList; class VIWndOptions; class VIWndPrivChat; class VIWndQuantity;
class VIWndQuestLog; class VIWndReadCS; class VIWndReadGram; class VIWndRecipe;
class VIWndReply; class VIWndResponse; class VIWndServerSelect; class VIWndSpellBook;
class VIWndStartMenu; class VIWndTell; class VIWndTrade; class VIWndTradeList;
class VIWndTrainer; class VIWndWhoList;
class VIClient;   // the sub-objects' owner (VIClientPart4EC58, VIClientPart2EFD0)






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
class VIItemDef;                                  // cl_tail.h
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
    VIArray<VIItemStat> m_stats;                  // +0x2DC: the original's `Mods` (VINetItem.cpp:58)
    void SetFrom(const VIItemDef *pItem);         // 0x00CBA9D8: unreferenced
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
    // an inline ctor -- every VIItemList member's construction (VIClient's
    // four lists, VIClientPart3FC10's) ends with two zero stores through the list's own
    // address after its 40 items: the body, so Clear() (the last store, +4, comes first).
    VIItemList() { Clear(); }
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
struct VIRec52_CB77F8;                             // cl_tail.h
class VIPlayerRecord
{
public:
    VIPlayerRecord();                              // 0x00CB7788, name provisional
    VIPlayerRecord(const VIRec52_CB77F8 &r);   // 0x00CB77D0: unreferenced
    VIPlayerRecord &operator=(const VIRec52_CB77F8 &r);   // 0x00CB77F8
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
class VIPlayer
{
public:
    // NOT derived from VIPlayerRecord: VIClient's roll-call constructs only the two VIVect3s
    // below, so nothing with a user constructor is in here.  The record's fields, copied.
    int  m_id;                                     // +0x00
    char m_name[24];                               // +0x04
    int  m_class;                                  // +0x1C
    int  m_race;                                   // +0x20
    int  m_level;                                  // +0x24 (+0xC300)
    int CanUse(VINetItem *it);                     // 0x00BCA8F0: class and race masks, then the level
    void Read(VIMemStream &s);                     // 0x00CBA850: the 0x000D dump's player block
    int           m_exp;                           // +0x28 (+0xC304): 0x0020
    int           m_p2C;                           // +0x2C (+0xC308): 0x00D9
    unsigned char m_p30;                           // +0x30 (+0xC30C): a byte getter
    char          m_p31[3];
    int           m_p34;                           // +0x34 (+0xC310): 0x0052
    int           m_p38;                           // +0x38 (+0xC314)
    int           m_p3C;
    int           m_p40;                           // +0x40 (+0xC31C): 0x0036
    int           m_p44;                           // +0x44 (+0xC320): 0x001C
    int           m_world;                         // +0x48: the world to enter (Read's zigzag ints end here)
    VIVect3       m_pos;                           // +0x4C: where; Read takes it raw, three words.  VIClient's roll-call zeroes it
    VIVect3       m_hpr;                           // +0x58: ... and the heading, the same
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
    void Add(int ms);                               // 0x00CC64C0: clamped to 0xFFFF
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
    VISyncTime() { m_bReady = 0; }                              // inline: VIClient's ctor stores one word
    void Set(long t, long base);                                // 0x00CBCBD8: VISyncTime.cpp:77
    long GetTime(long now);                                     // 0x00CBCC80: VISyncTime.cpp:95 asserts m_bReady
    void Reset();                                               // 0x00CBCD08, name provisional (Connect)
    int  m_bReady;                                              // +0x00 (the asserts' names)
    long m_Blend;                                               // +0x08: what is left to blend in
    long m_local;                                               // +0x10: the local clock at the last Set
    long m_remote;                                              // +0x18: the synced time then
};

// VIWndDisconnect (RTTI "15VIWndDisconnect", viwnddisconnect.cpp) is declared HERE because
// VIClient embeds it by value: its ctor 0x00C434B8 is in VIClient's roll-call.
// The window embedded at VIClient+0x2BB98, 0x1A8 bytes: Notify sets its app pointer and
// makes it the active window, and Disconnect compares the active window against it.
class VIWndDisconnect : public VIWnd
{
public:
    VIWndDisconnect();                                           // 0x00C434B8
    virtual ~VIWndDisconnect();                                  // 0x00C434F8
    int  Init();                                                 // 0x00C43548: returns 0
    virtual void OnDrawSelf();                                   // 0x00C43550: the status line, "Ok"
    virtual void OnKeyUp(int key);                               // 0x00C436F8: empty
    virtual void OnKeyDown(int key);                             // 0x00C43700: Enter/X: Msg0777Done(DisconnectKind())
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C43758: focus opens it, titled 272

    int        m_p24;           // +0x024: unused by its own code
    VIUIWindow m_window;        // +0x028
};

class VIEntity;
// One floating damage number, 44 bytes, on VIClientPart4FD28's free list or its active one.
// Names ours.
struct VIDamageNumber : public VILNode
{
    int     m_amount;                                           // +0x10: > 0 green (a heal), else the hit
    VIVect3 m_offset;                                           // +0x14: from the actor's position (a character's slot 4)
    short   m_z;                                                // +0x20: 0 another's hit, 1 the player's own (yellow)
    short   m_slot;                                             // +0x22: m_slot[m_z] when added, 0..2: the bounce height
    int     m_defender;                                         // +0x24: the entity id it floats over
    float   m_time;                                             // +0x28: 0 when added; gone past 1 (1.1 per second)
};

// The floating damage numbers ("damage_font.tga", a 4 x 4 grid of digits) at VIClient+0x4FD28,
// 0x5AC bytes to +0x502D4 (its own TU, 0x00BE8DF0 .. 0x00BE9CA0).  Name
// provisional; DamageFromSelf / DamageFromOther report through AddDamage.
class VIClientPart4FD28
{
public:
    VIClientPart4FD28();                                        // 0x00BE8DF0
    ~VIClientPart4FD28();                                       // 0x00BE8E88
    int  Init(VIScene &sc, VIEntity **ents, int n);             // 0x00BE8EE0 (EnterGameLoad: the 24 entity slots)
    void Release();                                             // 0x00BE9068 (UnloadWorld): its material
    int  AddDamage(int defender, int amount, int z);            // 0x00BE90B8: 0, or -1 with no scene
    int  Process(VICamera &cam, float dt);                      // 0x00BE9198: moves and draws them all
    void Draw(const VIVect2 &pos, int amount, const VIColor32F &col, float t);  // 0x00BE96A8
    void GlyphUV(int digit, VIVect2 *uv);                       // 0x00BE9BD8: four corners in the 4 x 4 font
    VIEntity *FindEntity(int id);                               // 0x00BE9C50: in m_ents[0 .. m_n)
    VIScene        *m_scene;                                    // +0x000: 0 until Init
    VIEntity      **m_ents;                                     // +0x004
    int             m_n;                                        // +0x008
    short           m_slot[2];                                  // +0x00C: per m_z, cycles 0..2
    int             m_material;                                 // +0x010: -1 = none
    VILList         m_active;                                   // +0x014
    VILList         m_free;                                     // +0x020
    VIDamageNumber  m_numbers[32];                              // +0x02C
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
// EnterGameLoad's and LoadWorld's unused local (sp+1072) is NOT a bare VIArray: it is an
// object HOLDING one, built and destroyed by its implicit inline ctor/dtor.  Through the
// inline, the array's address reaches the out-of-line VIArray ctor/dtor as a constant
// (a0 = sp+1072, no pseudo), so gcse has no occurrence to PRE-copy into s7 and every
// cleanup recomputes it.  Name provisional.
struct VIElemList_BDF7A0 { VIArray<VIElem_BDF7A0> m_list; };
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

// VIFileStream.cpp (0x00CB7008 .. 0x00CB7510): a VIFile with a sticky error, read and
// written in elements like VIMemStream.  VIJournal's m_File (+0x08); its asserts read it as
// `m_File.GetError()`.  It HAS a VIFile (VIJournal's dtor destroys it with __in_chrg 2).
// 200 bytes.
class VIFileStream
{
public:
    void Open(const char *name, int read);                      // 0x00CB7008: VIFileStream.cpp:57
    void Write(const void *src, int size, int count);           // 0x00CB70E0
    void Read(void *dst, int size, int count);                  // 0x00CB7188
    VIFileStream();                                             // 0x00CB7230
    int  Eof();                                                 // 0x00CB7260
    void Flush();                                               // 0x00CB7280
    void ReadString(char *dst, unsigned int MaxBytes);          // 0x00CB72A0: VIFileStream.cpp:196
    void ReadString(unsigned short *dst, unsigned int MaxBytes);    // 0x00CB7348: VIFileStream.cpp:219
    void WriteString(const char *s);                            // 0x00CB7400
    void WriteString(const unsigned short *s);                  // 0x00CB7478
    void Skip(int n);                                           // 0x00CB74F0
    int  GetError() { return m_Error; }
    // Inline: the length is computed in a register and stored only for the call.
    void WriteU32(unsigned int v) { Write(&v, 4, 1); }
    VIFile m_File;                                              // +0x00
    int    m_Error;                                             // +0xC0
};
typedef VIFileStream VIJournalFile;   // the earlier placeholder name

// VIJournal.cpp: a record of console input and network packets, VIClient+0x90 (its
// asserts name `m_File` and `m_MemStream`), 0x100 bytes to +0x190.
class VIJournal
{
public:
    int  Open(int mode, const char *path, int size);            // 0x00CB7C30: 0 memory only, 1 records to `path`, 2 plays it back
    VIJournal();                                                // 0x00CB7CE8
    ~VIJournal();                                               // 0x00CB7D48
    void Write(const void *p, int size, int count);             // 0x00CB7DB0: VIJournal.cpp:99/104
    // One value BY VALUE (JournalTick): a promoted byte lives in the inline's frame (a
    // 16-byte slot at +0) and a long behind an ADDRESSOF after it.
    template <class T> void Write(T v) { Write(&v, sizeof(T), 1); }
    void Read(void *p, int size, int count);                    // 0x00CB7E88: a played-back journal's bytes
    // One value BY REFERENCE (Tick): the reading twin of Write(T).
    template <class T> void Read(T &v) { Read(&v, sizeof(T), 1); }
    void WriteString(const char *s);                            // 0x00CB7F38: VIJournal.cpp:158
    void ReadString(char *dst, unsigned int MaxBytes);          // 0x00CB7FD8
    int  Eof();                                                 // 0x00CB8068
    static void FreeName(char *buf, const char *fmt);           // 0x00CB80B0, name provisional: the first unused fmt %03u
    VIJournal &operator<<(VIMemStream &s);                      // 0x00CB8140: the rest of s, length first (unreferenced)
    VIJournal &operator>>(VIMemStream &s);                      // 0x00CB81D8: the reverse, into s (unreferenced)
    int  Playback() { return m_mode == 2; }                     // inline: Connect asks it twice
    int  GetError() { return m_File.GetError() || m_MemStream.GetError(); } // inline: Tick
    int           m_mode;                                       // +0x00: Open's mode; 2 plays back
    int           m_p04;
    VIFileStream  m_File;                                       // +0x08
    VIMemStream   m_MemStream;                                  // +0xD0
    int           m_pF0;
    int           m_written;                                    // +0xF4: Write adds size * count
    int           m_read;                                       // +0xF8: Read adds size * count
    int           m_pFC;
    template <class T> void Read(T *p, int n);                  // INLINE, defined in viclient.cpp before Tick (see below)
};
// Read(T *, int) is defined INLINE in viclient.cpp, after InitBody and before Tick.  InitBody's
// three calls (through the inline ReadFlag) come before the body and stay calls to the
// out-of-line copy 0x00BE3680, while Tick's are integrated -- a1 = sp+16 straight, with no
// address pseudo for gcse's LCM to hoist across the assert.  ReadFlag reads its unsigned byte
// through Read<char> (a cast): a specialization CALLED before its inline definition is never
// integrated later in the TU, so InitBody must not use Tick's Read<unsigned char>.  viclient.cpp
// instantiates <char> explicitly at its end to emit the copy.

// A VIArray element the spellbook's ability list is built in (GetAbilityList stores map
// iterators into it).  Placeholder name kept: client_viarray.cpp's copies carry it.

// A per-race camera setting (the race camera lookup, GetRaceCamera: race x expansion).
// Only the floats ApplyRaceCamera and SetCameraStyle read are named.  Name provisional.
struct VIRaceCamera
{
    int   m_race;                                    // +0x00 GetRaceCamera matches it
    float m_reviewHeight;                            // +0x04 the review page's camera:
    float m_reviewLookAt;                            // +0x08 target height, look-at height,
    float m_reviewPitch;                             // +0x0C default pitch, distance
    float m_reviewDistance;                          // +0x10
    float m_custHeight;                              // +0x14 the appearance pages' camera
    float m_custLookAt;                              // +0x18
    float m_custPitch;                               // +0x1C
    float m_custDistance;                            // +0x20
    float m_targetHeight;                            // +0x24
    float m_lookAtHeightAbove;                       // +0x28
    float m_pitch;                                   // +0x2C
    float m_distance;                                // +0x30
    float m_p34;                                     // +0x34: SetCameraStyle stores it into the camera
};

VIRaceCamera *GetRaceCamera(int race, int expansion);          // 0x00BFC048, name provisional (11 callers)
extern VIRaceCamera gRaceCameras[21];                           // 0x00CDD8A0
extern VIRaceCamera gRaceCamerasEx[21];                         // 0x00CDDD38
// A 36-byte per-race record the character-creation windows read
// (0x00C287D8, 0x00C2C4F0): two 21-entry tables, picked the same way.  Fields unmeasured.
struct VIRaceInfo
{
    int m_p00;
    int m_male[4];       // +0x04 face, hair style, length, colour (names ours)
    int m_female[4];     // +0x14
};
extern VIRaceInfo gRaceInfo[21];                                // 0x00CDE428
extern VIRaceInfo gRaceInfoEx[21];                              // 0x00CDE720
VIRaceInfo *GetRaceInfo(int race, int expansion);              // 0x00BFC0A8: out of range -> entry 0

// The swing an auto-attack makes with what is worn in slots 14..20, by class and level:
// a swing type 0..12 (8..11 are the ranged ones, 12 none), its spell effect (the hash of
// "Attack1".."Range4", 0 for none) and its delay in ms.  [these were a
// VISwing pointer and two methods; the type is an int]
int GetSwing(int cls, int level, int b14, int b15, int b16, int b18, int b17,
             int b19, int b20);                                 // 0x00CBDEF8
unsigned int SwingEffect(int swing);                            // 0x00CBE0A8
int SwingDelay(int swing);                                      // 0x00CBE190
int SwingDelay2(int type);                                      // 0x00CBE1E8: unreferenced; 1000 for 12/14, 1500 for 15/16
int AttackAnim(int attackType, int alt);                        // 0x00CC5190 (VIEntity.cpp): the swing animation
class VIClientPart06D00;
class VIEntity;
VIEntity *CreateEntity(unsigned char type, unsigned int id, VIScene &sc, VIClientPart06D00 &res);  // 0x00CC5098: VIEntity.cpp:476, name provisional; the type is masked to a byte
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

// The quick-chat menu tree (vitextset.cpp, 0x00C0B350 .. 0x00C0B880):
// up to 256 nodes in a static table (data_00CF5048), one per id, each four (label, text)
// wide-string pairs; the 0x000D dump fills it from VIRec6A records (LoadTextSets) and
// SetTextSet adds one.  An id is the node's PATH: the low two bits are its depth (0..3) and
// each level above adds a 2-bit child index at bits 2, 4 and 6 (MakeId, ChildId), so the
// root is 0 and a depth-3 node has no children.  VIWndMenu4 walks it.  Names provisional.
struct VITextPair
{
    VITextPair();                                               // 0x00C0B870
    ~VITextPair() { delete [] m_p00; delete [] m_p04; }         // inline: FreeAll / Remove
    unsigned short *m_p00;                                      // VIRec388::m_p104's copy (the label)
    unsigned short *m_p04;                                      // VIRec388::m_p04's copy (the text)
};
class VITextSet
{
public:
    static void Reset();                                        // 0x00C0B350: all 256 slots, no free
    static void FreeAll();                                      // 0x00C0B388: VIWndGame::WorldUnloaded
    static VITextSet **GetTable();                              // 0x00C0B460
    static VITextSet *GetRoot();                                // 0x00C0B470: id 0
    VITextSet *Child(int k);                                    // 0x00C0B480: 0 at depth 3 or unset
    static int MakeId(int a, int b, int c);                     // 0x00C0B4D0: -1 ends the path
    int  ChildId(int k);                                        // 0x00C0B528
    static VITextSet *Get(int id);                              // 0x00C0B550: new'd on first use
    VITextSet *GetChild(int k);                                 // 0x00C0B628: Get(ChildId(k))
    void Mark(int deep);                                        // 0x00C0B708: m_p01 = 2, children first when deep
    void Remove();                                              // 0x00C0B7C8: out of the table, deleted
    unsigned char m_id;                                         // +0x00: the path id
    unsigned char m_p01;                                        // +0x01: Mark sets 2
    unsigned char m_p02;
    unsigned char m_p03;
    VITextPair    m_pairs[4];                                   // +0x04
};

// A weapon type's display word (viweapontype.cpp): {word, type}, eight rows.
struct VIWeaponTypeWord
{
    int m_word;                                                 // +0x00: WORDS.TXT id
    int m_type;                                                 // +0x04: 0 1HS .. 6 RANGED, 7 SHIELD
};
extern VIWeaponTypeWord gWeaponTypeWords[8];                    // 0x00CE0A20
const VIWeaponTypeWord *FindWeaponTypeWord(int type);           // 0x00C0D000

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
// The character resources (VIClient+0x6D00, 80 bytes): a pool of 524-byte records, one per
// character actor, each a committed and a pending appearance; the setters write the pending
// one, Commit applies it.  Code 0x00CBE3D8 .. 0x00CC0128 (viswing.cpp).  Names provisional.

// The character resources, CLIENT 0x00CBE240.. 0x00CC20F8 (viswing.cpp).
// Every player and NPC model is a pooled VICharRes: the state the actor shows (m_cur) and
// the state being loaded (m_new; flag 1 = edited).  Each frame Update ranks the entries by
// (frames unseen, distance squared) and loads only the nearest m_maxFull; past m_maxLoaded
// an entry's resources are dropped.  All names ours; the members are measured.
struct VIResHandle                                   // 16 bytes, ctor 0x00CBE240
{
    VIResHandle();
    unsigned int m_id;                               // +0x00: the resource (0 = none)
    unsigned int m_fallback;                         // +0x04: the stand-in shared instead, or 0
    int          m_res;                              // +0x08: the dictionary index, -1 unresolved
    int          m_load;                             // +0x0C: the VILoader request, -1 none
};
struct VICharResItem                                 // 36 bytes, ctor 0x00CBE260
{
    VICharResItem();
    int         m_type;                              // +0x00: the attack type AttachItem takes
    VIResHandle m_model;                             // +0x04
    VIResHandle m_model2;                            // +0x14: the projectile
};
struct VICharResBody                                 // 128 bytes, ctor 0x00CBE298
{
    VICharResBody();
    char        m_name[32];                          // +0x00
    int         m_nameColor;                         // +0x20
    int         m_face;                              // +0x24: -1 unset
    VIResHandle m_faceRes;                           // +0x28: VICSpriteCust::GetFaceResourceID
    int         m_hair[3];                           // +0x38: VICSprite::SetHair
    int         m_robe[3];                           // +0x44: VICSprite::SetRobe
    int         m_armorSet[6];                       // +0x50: VICSprite::SetArmorSlot
    int         m_armorTint[6];                      // +0x68
};
struct VICharResState                                // 256 bytes, ctor 0x00CBE318
{
    VICharResState();
    unsigned int  m_flags;                           // +0x00: 1 edited/loaded, 2 hidden
    VIResHandle   m_model;                           // +0x04: the CSprite
    VICharResBody m_body;                            // +0x14
    VICharResItem m_items[3];                        // +0x94: the attach slots
};
struct VICharRes                                     // 524 bytes, ctor 0x00CBE390
{
    VICharRes();
    int            m_actor;                          // +0x00: -1 none
    int            m_stamp;                          // +0x04: the frame it was last ranked visible
    int            m_rank;                           // +0x08: Rank's order, nearest first
    VICharResState m_cur;                            // +0x0C
    VICharResState m_new;                            // +0x10C
};
struct VICharResKey                                  // Rank's sort key (VIMultiMap<VICharResKey, int>)
{
    int   m_age;
    float m_dist2;
    int operator<(const VICharResKey &k) const
    {
        if (m_age < k.m_age)
            return 1;
        if (m_age == k.m_age && m_dist2 < k.m_dist2)
            return 1;
        return 0;
    }
};
class VICamera;
class VIClientPart06D00
{
public:
    VIClientPart06D00();                                        // 0x00CBE3D8
    ~VIClientPart06D00();                                       // 0x00CBE458
    int  Init(VIScene &sc, VILoader &ld);                       // 0x00CBE4B8 (OpenResources)
    int  Clear();                                               // 0x00CBE520 (UnloadWorld, the dtor)
    int  Create(int model, const VIVect3 &pos, const VIVect3 &hpr, float scale);  // 0x00CBE640: a slot and its actor
    int  Release(int slot);                                     // 0x00CBE728: its actor
    int  GetActor(int slot);                                    // 0x00CBE790
    int  SetVisible(int slot, int visible);                     // 0x00CBE7A8
    int  SetName(int slot, const char *name, int color);        // 0x00CBE898
    int  SetModel(int slot, int model);                         // 0x00CBE9C0
    int  SetItem(int slot, int i, int model, int type, int model2);   // 0x00CBEAB8
    int  SetFace(int slot, int face);                           // 0x00CBEC80
    int  SetHair(int slot, int a, int b, int c);                // 0x00CBED78
    int  SetRobe(int slot, int a, int b, int c);                // 0x00CBEE90
    int  SetArmorSlot(int slot, int i, int set, int tint);      // 0x00CBEFA8
    int  Update(const VICamera &cam);                           // 0x00CBF0C0: Rank, Process, frame++
    int  BeginEdit(int slot);                                   // 0x00CBF118: m_new = m_cur, sharing
    int  Apply(int slot);                                       // 0x00CBF340: m_new onto the sprite
    int  Rank(const VIVect3 &eye);                              // 0x00CBF818
    int  Process();                                             // 0x00CBFA08
    int  LoadState(VICharResState &s, int &ready);              // 0x00CBFD08
    int  ReleaseState(VICharResState &s);                       // 0x00CBFE60
    int  LoadRes(VIResHandle &h, int pack, unsigned int fallback, int &ready);  // 0x00CBFF18
    int  ReleaseRes(VIResHandle &h);                            // 0x00CC0090

    VILoader     *m_loader;                                     // +0x00
    VIScene      *m_scene;                                      // +0x04
    VIRaster     *m_raster;                                     // +0x08
    VIDictionary *m_dict;                                       // +0x0C
    int           m_frame;                                      // +0x10
    unsigned int  m_charType;                                   // +0x14: "!Character", the stand-in model
    unsigned int  m_itemType;                                   // +0x18: "!Item"
    int           m_maxFull;                                    // +0x1C: 16 loaded and shown
    int           m_maxLoaded;                                  // +0x20: 24 kept
    VIPool<VICharRes> m_pool;                                   // +0x24
    VIMultiMap<VICharResKey, int> m_lru;                        // +0x3C: a MULTImap (Allocate copies from node 0)
};

// One icon of VIClientPart06D50's cache, 12 bytes: its id, the surface once it is in, and the
// loader slot while it loads.  Names ours.
struct VIIconSlot
{
    VIIconSlot();                                               // 0x00BF2ED8: out of line, this TU's first function
    unsigned int m_id;                                          // +0x00
    int          m_surface;                                     // +0x04: -1 until loaded
    int          m_load;                                        // +0x08: the loader slot, -1 when idle
};

// The icon cache at VIClient+0x6D50, 0x40 bytes to +0x6D90 (its own TU, 0x00BF2ED8 .. 0x00BF3644):
// up to m_n icons (OpenResources: 48) loaded through the loader, the oldest
// dropped when full, a hit moved to the back.  m_index maps an icon id to its slot.  Names
// provisional.
class VIClientPart06D50
{
public:
    VIClientPart06D50();                                        // 0x00BF2EF0
    ~VIClientPart06D50();                                       // 0x00BF2F38
    int  Init(int n, VIScene &sc, VILoader &ld);                // 0x00BF2F98 (OpenResources: 48)
    int  Clear();                                               // 0x00BF3038 (UnloadWorld, the dtor)
    int  Update();                                              // 0x00BF3108: every slot still loading
    void Preload(int icon);                                     // 0x00BF31B8 (LoadInvIcons)
    int  Get(unsigned int id, int &state, int &surface);        // 0x00BF31D8: state 0 in, 1 loading, 2 unavailable
    int  Load(unsigned int id);                                 // 0x00BF32E8: a new slot (the oldest goes when full)
    int  Touch(int slot);                                       // 0x00BF3400: to the back of the pool
    int  Resolve(VIIconSlot &s);                                // 0x00BF34D0
    int  Unload(VIIconSlot &s);                                 // 0x00BF35D8
    VILoader          *m_loader;                                // +0x00
    VIRaster          *m_raster;                                // +0x04
    VIDictionary      *m_dict;                                  // +0x08
    VIScene           *m_scene;                                 // +0x0C
    int                m_n;                                     // +0x10
    VIPool<VIIconSlot> m_slots;                                 // +0x14
    VIMap<unsigned int, int> m_index;                           // +0x2C: id -> slot
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

VIMemStream &ReadNetItem(VIMemStream &s, VINetItem &item);      // 0x00CBB040, name provisional: returns s

// An entity: CLIENT's VIEntity.cpp (0x00CC4608 .. 0x00CC61A8).  88 bytes; the vptr sits at
// +0x54, after the class's own data.  An update carries the position as three 24-bit
// fixed-point coordinates (1/128 m) and the heading and pitch as a byte each; a full
// write carries the floats.  [the layout and every name below]
class VIEntity
{
public:
    VIEntity(VIScene *scene);                         // 0x00CC4728
    virtual ~VIEntity();                              // slot 1, 0x00CC47A0: Disconnect's delete
    // Inline, emitted after __tf8VIEntity in this order (0x00CC6190 ..).
    virtual int  GetType() { return 129; }            // slot 2: ChannelEntity compares it
    virtual int  IsType(unsigned char type);          // slot 3, 0x00CC5180: Msg1003Done asks it about 130
    virtual void WriteUpdate(VIMemStream &s);         // slot 4, 0x00CC4B00
    virtual void ReadUpdate(VIMemStream &s);          // slot 5, 0x00CC4CC8: ChannelEntity hands it the rest of the channel
    virtual void Write(VIMemStream &s);               // slot 6, 0x00CC4FA0
    virtual void Read(VIMemStream &s);                // slot 7, 0x00CC5000
    virtual int  IsSelectable();                      // slot 8, 0x00CC47F0: 1
    virtual int  Refresh();                           // slot 9, 0x00CC4DE8: 0 (Msg1003Done calls it on every entity)
    unsigned char GetP19() { return m_p19; }
    unsigned char GetP19b() { return m_p19; }

    const VIVect3 &GetActorPos();                     // 0x00CC47F8: the scene actor's location
    void SetActorPos(const VIVect3 &v);               // 0x00CC4818: ... if it has one
    const VIVect3 &GetActorHpr();                     // 0x00CC4848
    void SetActorHpr(const VIVect3 &v);               // 0x00CC4868
    int  GetDimension();                              // 0x00CC4898
    VIVect3 *GetPos();                                // 0x00CC48A0: +0x40
    void StorePos(const VIVect3 &p);                  // 0x00CC48A8: its own only
    VIVect3 *GetHpr();                                // 0x00CC48C8: +0x00, the object itself
    void StoreHpr(const VIVect3 &h);                  // 0x00CC48D0
    void SetDimension(int d);                         // 0x00CC48F0
    void SetPos(const VIVect3 &p);                    // 0x00CC4918: its own, then the actor's
    void SetHpr(const VIVect3 &hpr);                  // 0x00CC4958 (ResetView)
    int  GetModel();                                  // 0x00CC4998: m_model2 if set, else m_model
    void SetModel(int m);                             // 0x00CC49B0
    void SetModel2(int m);                            // 0x00CC49B8
    float GetScale();                                 // 0x00CC49C0
    static void Quantize(float v, unsigned char *Value);          // 0x00CC49C8: 24-bit, 1/128
    static void Dequantize(const unsigned char *Value, float &f); // 0x00CC4A78: VIEntity.cpp:298
    void WriteState(VIMemStream &s);                  // 0x00CC4DF0
    void ReadState(VIMemStream &s);                   // 0x00CC4EE0
    int  CreateActor();                               // 0x00CC5238
    int  ReleaseActor();                              // 0x00CC5338
    void OnWorldLoaded();                             // 0x00CC53C0: LoadWorld calls it on every entity
    int  Changed(int oldDim, int oldModel, float oldScale);       // 0x00CC5438: VIEntity.cpp:681
    int  GetSceneDimension();                         // 0x00CC5580: the scene's +0x4BAC

    VIVect3        m_hpr;                             // +0x00
    int            m_id;                              // +0x0C: FindEntity's key
    long           m_p10;                             // +0x10: on the wire
    unsigned char  m_p18;                             // +0x18: flags; 2 = not a creature (ToggleAutoAttack sends 0x0015 instead)
    unsigned char  m_p19;                             // +0x19: a byte the client reads for itself and its target
    float          m_scale;                           // +0x1C
    int            m_p20;                             // +0x20: CreateEntity zeroes it
    int            m_actor;                           // +0x24: the scene actor index, -1 none
    long           m_p28;                             // +0x28: -1
    VIScene       *m_scene;                           // +0x30
    VIClientPart06D00 *m_res;                         // +0x34: the character resources (CreateEntity)
    int            m_resSlot;                         // +0x38: its slot there, -1 none
    int            m_DimensionId;                     // +0x3C: the assert's name; -1 none
    VIVect3        m_pos;                             // +0x40
    int            m_model;                           // +0x4C
    int            m_model2;                          // +0x50: used instead of m_model when set
};

// VICreatureEntity.cpp's class (RTTI "16VICreatureEntity").  The vptr stays at +0x54, the
// end of VIEntity's OWN data, so anything past +0x58 is the derived class's.
class VICreatureEntity : public VIEntity
{
public:
    // VICreatureEntity.cpp, CLIENT 0x00CC20F8.. 0x00CC4608.  RTTI
    // 16VICreatureEntity, vtable 0x00CF3600 (ten virtuals: VIEntity's nine and slot 10).
    VICreatureEntity(VIScene *scene);                 // 0x00CC20F8
    virtual ~VICreatureEntity();                      // slot 1, 0x00CC22A8
    virtual int  GetType() { return 130; }            // slot 2 (inline, 0x00CC45E8)
    virtual int  IsType(unsigned char type);          // slot 3, 0x00CC3850: 130 or VIEntity's
    virtual void WriteUpdate(VIMemStream &s);         // slot 4, 0x00CC28D0 (VICreatureEntity.cpp:364/371)
    virtual void ReadUpdate(VIMemStream &s);          // slot 5, 0x00CC2DB0
    virtual void Write(VIMemStream &s);               // slot 6, 0x00CC3150
    virtual void Read(VIMemStream &s);                // slot 7, 0x00CC3538
    virtual int  IsSelectable();                      // slot 8, 0x00CC22F8: m_p256 == 2 or VIEntity's
    virtual int  Refresh();                           // slot 9, 0x00CC27D0: 0
    virtual void WriteIdentity(VIMemStream &s);       // slot 10, 0x00CC3AB8: id, byte +0x19, name
    void Sync();                                      // 0x00CC2340: items, look, physics into m_walk
    void ApplyItems();                                // 0x00CC2590
    void ApplyLook();                                 // 0x00CC2690
    int  Flag1();                                     // 0x00CC38A0: +0x262 & 1 (hidden)
    int  Flag2();                                     // 0x00CC38B0
    int  Flag4();                                     // 0x00CC38C0, name provisional: +0x262 & 4
    int  Flag8();                                     // 0x00CC38D0, name provisional: +0x262 & 8
    int  Flag10();                                    // 0x00CC38E0
    unsigned char Byte263();                          // 0x00CC38F0, name provisional: +0x263
    void Extrapolate(long dt);                        // 0x00CC38F8: ExtrapolateMove over dt ms
    void Walk();                                      // 0x00CC3938, name provisional: m_walk.Process while m_p255
    void SnapToGround();                              // 0x00CC3B30: ExtrapolateGround, own copy only
    const char *GetName() { return m_name; }          // 0x00CC45F0 (inline)
    void SetLevel(unsigned char v) { m_level = v; }         // 0x00CC45F8 (inline)
    unsigned int GetWalkFlags() { return m_walkFlags; }  // 0x00CC4600 (inline)

    char m_name[24];                                  // +0x58: TradeName copies it (FrontiersForge: name)
    unsigned char m_level;                            // +0x70: DrawTargetInfo prints it (name ours)
    unsigned int  m_p74;                              // +0x74: 0xFFFFFFFF
    VIWalkControl m_walk;                             // +0x78: UpdateCreatures drives the sprite from it while m_p255
    VIVect3 m_p218;                                   // +0x218: the velocity; ChannelEntity: none when it did not move
    VIVect3 m_avel;                                   // +0x224: angular velocity (z must be 0)
    VIVect3 m_accel;                                  // +0x230
    VIVect3 m_aaccel;                                 // +0x23C: angular acceleration (z must be 0)
    unsigned int m_walkFlags;                         // +0x248: VIWalkPhysics::m_flags
    float m_floor;                                    // +0x24C: VIWalkPhysics::m_floor
    float m_targetHeading;                            // +0x250: VIWalkPhysics::m_targetHeading
    unsigned char m_state;                            // +0x254: 14 = dead; otherwise the locomotion animation
    unsigned char m_p255;                             // +0x255: Extrapolate keeps pitch and roll at 0 while set; walking
    unsigned char m_p256;                             // +0x256: GetTargetPlayerName wants 2 (2 or 3: a player)
    unsigned char m_p257;
    unsigned char m_race;                             // +0x258: DrawTargetInfo, GetRace (name ours)
    unsigned char m_class;                            // +0x259: DrawTargetInfo, GetClass (name ours)
    unsigned short m_npcFlags;                        // +0x25A: InteractWithTarget: 1 op 0x4C, 2 op 0x124D, 8 op 0x61; 0x200 invisible
    int  m_p25C;                                      // +0x25C: -1
    unsigned char m_p260;
    unsigned char m_p261;
    unsigned char m_flags262;                         // +0x262: Flag1..Flag10
    unsigned char m_p263;
    unsigned char m_p264;                             // +0x264: InviteEntity wants it set (word 1014 when not)
    int  m_item[3];                                   // +0x268: the attach-slot models
    int  m_itemOver[3];                               // +0x274: wins over m_item when set
    unsigned char m_itemType[2];                      // +0x280: slots 0 and 1's attack types
    unsigned char m_armorSet[6];                      // +0x282
    unsigned char m_armorTint[6];                     // +0x288
    unsigned char m_hair[3];                          // +0x28E
    unsigned char m_face;                             // +0x291
    signed char   m_robe;                             // +0x292: -1 none
    unsigned char m_robeTint;                         // +0x293
    int  m_p294;                                      // +0x294: zigzag in Write
    unsigned char m_p298;
    unsigned char m_p299;                             // +0x299: bit 0 makes the name colour 2
    long m_extrapolated;                              // +0x2A0: the scene time Walk last extrapolated to
    int  m_p2A8;                                      // +0x2A8: SetDeath's second argument; UpdateCreatures zeroes it
    long m_deathTime;                                 // +0x2B0: Msg005E and UpdateCreatures store the scene time
    int  m_p2B8;                                      // +0x2B8: Msg1003Done sets 1 on every entity but the player
};

// The guild at VIClient+0x2EC84, 0x2A0 bytes (vtable slot 4; viclientguild.cpp, 0x00BF0B18 ..
// 0x00BF124C).  0x0468 is the server's side, sub-ops 1..16.  Names provisional.
class VIClient;
class VIClientGuild
{
public:
    VIClientGuild(VIClient *c);                                 // 0x00BF0B18
    ~VIClientGuild();                                           // 0x00BF0B58
    VIMap<int, VIGuildMember> *GetMembers();                    // 0x00BF0BA8
    int  InGuild();                                             // 0x00BF0BB0
    unsigned short *GetName();                                  // 0x00BF0BB8
    unsigned short *GetMotd();                                  // 0x00BF0BC0
    VIGuildMember *Member(int n);                               // 0x00BF0BC8: the n-th in key order
    char *GetMemberName(int n);                                 // 0x00BF0C38
    int  GetMemberId(int n);                                    // 0x00BF0C58
    int  GetMemberRank(int n);                                  // 0x00BF0C78
    int  GetMemberRace(int n);                                  // 0x00BF0C98
    int  GetMemberClass(int n);                                 // 0x00BF0CB8
    int  GetMemberLevel(int n);                                 // 0x00BF0CD8
    int  GetMemberOnline(int n);                                // 0x00BF0CF8: int, not bool
    int  Count();                                               // 0x00BF0D18: m_members.Size()
    void Recv0468(VIMemStream &s);                              // 0x00BF0D38
    void ReadInfo(VIMemStream &s);                              // 0x00BF0EC8: 1, the name and the message of the day
    void Reset();                                               // 0x00BF0F20: 2, clears m_members
    void UpdateMember(VIMemStream &s);                          // 0x00BF0F50: 3
    void RemoveMember(VIMemStream &s);                          // 0x00BF1040: 4
    void Chat(VIMemStream &s);                                  // 0x00BF1088: 5, a line in the guild colour
    void Msg(unsigned short word);                              // 0x00BF1118
    void Added(VIMemStream &s);                                 // 0x00BF1138: 15, "%s has added %s to the guild."

    VIClient                 *m_client;                         // +0x000
    int                       m_inGuild;                        // +0x004
    unsigned short            m_name[64];                       // +0x008
    unsigned short            m_motd[256];                      // +0x088
    VIMap<int, VIGuildMember> m_members;                        // +0x288: 68-byte nodes
    int                       m_showChat;                       // +0x29C: 13 sets it ("visible"), 14 clears it
};

// The guild commands at VIClient+0x2EF24, 0xA4 bytes (vtable slot 5; its own TU, 0x00BEE5A8 ..
// 0x00BEFECC): the Can* checks VIWndGame's chat commands and VIWndGuild ask
// first (0 = allowed, else the reason: 3 not in a guild, 4 already in one, 5 rank, 6 tunar, 7
// group, 11 full, 12 lowest rank), the 0x0578 senders (the byte is the sub-op), a pending
// invitation, and the 0x057A / 0x0579 replies.  Names provisional.
extern int gGuildCost;                                          // 0x00CD7A50: 5000 tunar to found a guild
class VIClient;
class VIClientPart2EF24
{
public:
    VIClientPart2EF24(VIClient *c);                             // 0x00BEE5A8
    ~VIClientPart2EF24();                                       // 0x00BEE5C8
    void SetWnd(VIWndGame *w);                                  // 0x00BEE5F0 (InitBody)
    void Reset();                                               // 0x00BEE5F8 (EnterGameLoad)
    char *GetInviterName();                                     // 0x00BEE610
    unsigned short *GetInviteGuild();                           // 0x00BEE618
    void ShowInvite();                                          // 0x00BEE620
    void HideInvite();                                          // 0x00BEE640
    int  CanInvite();                                           // 0x00BEE678
    int  CanCreate();                                           // 0x00BEE720
    int  CanDisband();                                          // 0x00BEE7D8
    int  CanQuit();                                             // 0x00BEE868
    int  CanPromote(int id);                                    // 0x00BEE8A8
    int  CanDemote(int id);                                     // 0x00BEE980
    int  CanKick(int id);                                       // 0x00BEEA48
    int  CanList();                                             // 0x00BEEAF8
    int  CanSetMotd();                                          // 0x00BEEB38
    int  CanChat();                                             // 0x00BEEBC8
    void Create(const unsigned short *name);                    // 0x00BEEC08: 1
    void Disband();                                             // 0x00BEECD8: 2
    void Quit();                                                // 0x00BEED98: 3
    void Promote(int id);                                       // 0x00BEEE58: 4
    void Demote(int id);                                        // 0x00BEEF30: 5
    void Kick(int id);                                          // 0x00BEF008: 6
    void SetMotd(const unsigned short *s);                      // 0x00BEF0E0: 11
    void Say(const unsigned short *s);                          // 0x00BEF1B0: 12
    void Decline();                                             // 0x00BEF280: 9
    void Accept();                                              // 0x00BEF370: 10, answer 1
    void Invite(const char *name);                              // 0x00BEF478: 8
    void SetChat(int on);                                       // 0x00BEF600: 13
    void Recv057A(VIMemStream &s);                              // 0x00BEF6D8
    void Recv0579(VIMemStream &s);                              // 0x00BEF788: an operation's result
    void InviteReceived(VIMemStream &s);                        // 0x00BEF9A0: 057A 1
    void InviteAccepted(VIMemStream &s);                        // 0x00BEFC48: 057A 2
    void InviteDeclined(VIMemStream &s);                        // 0x00BEFCD8: 057A 3
    int  GetRank(int id);                                       // 0x00BEFDC8: 0 when not a member
    VIClientConn GetConn();                                     // 0x00BEFE50
    void Msg(unsigned short word);                              // 0x00BEFE90
    void Text(const unsigned short *s);                         // 0x00BEFEB0
    unsigned int   m_inviter;                                   // +0x00: -1 none
    char           m_name[24];                                  // +0x04: who invited us
    unsigned short m_guild[64];                                 // +0x1C: to which guild
    VIClient      *m_client;                                    // +0x9C
    VIWndGame     *m_wnd;                                       // +0xA0
};

// The customer-service client (0x00BE7120 .. 0x00BE8DF0): STATION's VICSClient,
// the same source (vicsclient.h, shared).  VIClient embeds it at +0x33950.
#include "vicsclient.h"

class VIClient;
// The GM/debug command channel at VIClient+0x382E8 (viadmin.cpp, CLIENT
// 0x00BB4C70 .. 0x00BB7DA8).  Commands are ids 0xA1xx0000 (Show(id, x), DoText(id, text))
// and 0xA2xx0000 (Do2): each sends message 0x0701 with a sub-op byte and its fields (0x0702
// for Do2).  The slash parser hands "/create", "/mastery", "/tradelvl" to DoText while
// IsEnabled(); 0x0700 sub-op 35 sets that flag, 50 prompts with a name.  The three
// simulation fields feed connection_set_simulated (packet loss %, latency, jitter).
// 24 bytes (the gap to VIClient+0x38300).  Method names are ours unless noted.
class VIClientPart382E8
{
public:
    VIClientPart382E8(VIClient *client);                        // 0x00BB4C70
    ~VIClientPart382E8();                                       // 0x00BB4C98
    void SendText13(const unsigned short *text);                // 0x00BB4CC0: sub-op 13
    int  IsCommand(unsigned int id);                            // 0x00BB4DA0: 0xA1xx0000
    int  IsCommand2(unsigned int id);                           // 0x00BB4DB8: 0xA2xx0000
    void Show(int id, int x);                                   // 0x00BB4DD0, name provisional (ShowA110): run command `id`
    void DoText(int id, const unsigned short *text);            // 0x00BB5400: a command with a text argument
    void Do2(unsigned int id, int x);                           // 0x00BB5830: 0xA2010000 sends 0x0702
    void ApplySimulation(connection_t *c);                      // 0x00BB5928
    int  GetP0C();                                              // 0x00BB5958: VIClient.cpp's 0x00BC6500 asks it
    void Reset();                                               // 0x00BB5960, name provisional (ResetWorld)
    void Send04(const char *s);                                 // 0x00BB5968
    void Send06(const char *s);                                 // 0x00BB5A48
    void Send53(const char *s);                                 // 0x00BB5B28
    void Send54(const char *s);                                 // 0x00BB5C08
    void Send55(const char *s);                                 // 0x00BB5CE8
    int  IsEnabled();                                           // 0x00BB5DC8: m_enabled
    void Recv0700(VIMemStream &s);                              // 0x00BB5DD0, name provisional
    void SendSimulation();                                      // 0x00BB5E50: sub-op 18
    void Send16(unsigned int id);                               // 0x00BB5F78: nothing for -1
    void Send17(unsigned int id);                               // 0x00BB6070: nothing for -1
    void Send14();                                              // 0x00BB6168
    void Toggle19();                                            // 0x00BB6238: flips m_p0C, sub-op 19
    void Send15();                                              // 0x00BB6318
    void Send01();                                              // 0x00BB63E8
    void Send02(int a, int b);                                  // 0x00BB64B8
    void Send03(int a);                                         // 0x00BB65C8
    void Send25(int a, int b);                                  // 0x00BB66B8
    void Send22(int a);                                         // 0x00BB67C8
    void Send05();                                              // 0x00BB68B8
    void Send07(int a);                                         // 0x00BB6988
    void Send08(int a);                                         // 0x00BB6A78
    void Send23(int a);                                         // 0x00BB6B68
    void Send24(int a);                                         // 0x00BB6C58
    void Send09(const char *s);                                 // 0x00BB6D48
    void Send10(const char *s);                                 // 0x00BB6E28
    void Send51(const char *s);                                 // 0x00BB6F08
    void Send11(const char *s);                                 // 0x00BB6FE8
    void Send12(const char *s);                                 // 0x00BB70C8
    void Send20(const char *s);                                 // 0x00BB71A8
    void Send21(const char *s);                                 // 0x00BB7288
    void Send32();                                              // 0x00BB7368
    void Send33(int a, const char *s);                          // 0x00BB7438
    void Send34(int a);                                         // 0x00BB7540
    void Send37();                                              // 0x00BB7630
    void Send40(int a);                                         // 0x00BB7700
    void Send38(int a);                                         // 0x00BB77F0
    void Send39(int a);                                         // 0x00BB78E0
    void Send41(const char *s);                                 // 0x00BB79D0
    void Send52(int a);                                         // 0x00BB7AB0
    void Send0701a();                                           // 0x00BB7BA0, name provisional: sub-op 48
    void Send0701b();                                           // 0x00BB7C70, name provisional: sub-op 49
    void RecvEnabled(VIMemStream &s);                           // 0x00BB7D40: 0x0700 sub-op 35
    void RecvPrompt(VIMemStream &s);                            // 0x00BB7D70: 0x0700 sub-op 50

    VIClient      *m_client;                       // +0x00
    unsigned char  m_loss;                         // +0x04: simulated packet loss, % (0..50)
    unsigned short m_latency;                      // +0x06: simulated latency (0..8192)
    unsigned short m_jitter;                       // +0x08
    int            m_p0C;                          // +0x0C: Toggle19 flips it, Reset clears it
    int            m_p10;                          // +0x10
    int            m_enabled;                      // +0x14: RecvEnabled
};

// One ignored player: a name and an id, 36 bytes (the placeholder name is
// client_vipool.cpp's, kept: its VIPool members carry it).
struct VIRec36_BFBCB8
{
    char m_name[32];                                // +0x00
    int  m_id;                                      // +0x20
};

// The ignore list, VIClient+0x38300 (vtable slot 9): 0x1000 adds a name,
// 0x1001 asks by id, 0x1002 removes; the server answers 0x1003 (added), 0x1004 (removed),
// 0x1005 (the whole list) and 0x1006 (renamed).  RefreshCollision asks Find.  The panel
// it opens and closes is the game window's VIWndIgnoreList (+0x6E8).  32 bytes, exactly to
// VIShardInfo at +0x38320.  The class name is kept; method names provisional.
class VIClientPart38300
{
public:
    VIClientPart38300(VIClient *owner);                         // 0x00BFA450
    ~VIClientPart38300();                                       // 0x00BFA4A8
    int  Reset();                                               // 0x00BFA4F8, name provisional (ResetWorld): clears the pool
    void OpenPanel();                                           // 0x00BFA518: the game window's ignore-list panel
    void ClosePanel();                                          // 0x00BFA540
    void Send1001(int id);                                      // 0x00BFA568
    void Add(const char *name);                                 // 0x00BFA628: sends 0x1000
    void RemoveAt(int i);                                       // 0x00BFA6D8: sends 0x1002 with entry i's id
    void Remove(const char *name);                              // 0x00BFA7A8: case-blind
    void Recv(unsigned short op, VIMemStream &s);               // 0x00BFA850, name provisional
    void Recv2(VIMemStream &s);                                 // 0x00BFAA38: two bytes, unused
    VIClientConn GetConn();                                     // 0x00BFAA80: the owner's 251
    void ReadList(VIMemStream &s);                              // 0x00BFAAC0
    void GetIndices(VIArray<int> &a);                           // 0x00BFAB90: every pool index
    void GetName(int i, char *dst);                             // 0x00BFAC30
    int  Count();                                               // 0x00BFAC60
    int  Find(const char *name);                                // 0x00BFAC80, name provisional: -1 = not listed (RefreshCollision)

    VIClient              *m_owner;                 // +0x00
    VIWndGame             *m_wnd;                   // +0x04
    VIPool<VIRec36_BFBCB8> m_list;                  // +0x08
    void SetGameWindow(VIWndGame *w) { m_wnd = w; }             // inline: InitBody stores through the part's own address
};

// One shard, 88 bytes: its name, a flag, the 64-bit id (-1 none) and its
// region.  The placeholder name is kept (client_viarray.cpp's VIArray members carry it).
// vishardentry.cpp holds its constructor (0x00CBBA98) and stream operators
// (<< 0x00CBBAB8, >> 0x00CBBB40); it is declared here because VIShardInfo indexes it.
struct VIElem_BFC798
{
    VIElem_BFC798();                                // 0x00CBBA98: name "", flag 0, id -1, region -1
    const char *Address();                          // 0x00CBBBC8: drdp_format_endpoint1 (unreferenced)
    unsigned short m_name[32];                      // +0x00
    bool           m_x40;                           // +0x40: one byte on the wire
    long           m_id;                            // +0x48
    int            m_region;                        // +0x50: a signed byte on the wire
};
VIMemStream &operator>>(VIMemStream &s, VIElem_BFC798 &e);       // 0x00CBBB40
VIMemStream &operator<<(VIMemStream &s, const VIElem_BFC798 &e); // 0x00CBBAB8

// VIShardInfo.cpp: the server-select (shard) list, VIClient+0x38320.  Its asserts name
// `m_ServerSelectList` (`!m_ServerSelectList.Init( Match )`): the shards of the current
// region, filtered out of every shard 0x07B3 lists.  20 bytes.  Method names provisional.
class VIShardInfo
{
public:
    VIShardInfo();                                              // 0x00BFC0F0: two VIArray ctors
    int  GetCount();                                            // 0x00BFC128: m_ServerSelectList's size
    int  IsValid(int i);                                        // 0x00BFC130: entry i's id != -1
    VIElem_BFC798 *GetEntry(int i);                             // 0x00BFC150
    int  FirstValid();                                          // 0x00BFC168: the first flagged, valid entry; -1
    long GetId(int i);                                          // 0x00BFC1F0: entry i's 64-bit id
    void Read(VIMemStream &s);                                  // 0x00BFC208: 0x07B3's payload
    void Clear();                                               // 0x00BFC240: both arrays
    void SetRegion(int region);                                 // 0x00BFC270, name provisional
    void Filter();                                              // 0x00BFC290: VIShardInfo.cpp:166, :175
    VIArray<VIElem_BFC798> m_shards;                // +0x00
    VIArray<VIElem_BFC798> m_ServerSelectList;      // +0x08
    int                    m_region;                // +0x10: -1 until SetRegion
};

// One row of the who list (32 bytes): three bytes, a name, a word.
// Placeholder name kept (client_vivector.cpp's VIVector members carry it); fields unnamed.
struct VIRec32_C035B0
{
    unsigned char m_x00;                            // +0x00
    unsigned char m_x01;                            // +0x01
    unsigned char m_x02;                            // +0x02
    char          m_name[24];                       // +0x03
    int           m_x1C;                            // +0x1C
};
VIMemStream &operator<<(VIMemStream &s, const VIRec32_C035B0 &r);   // 0x00C038E0
VIMemStream &operator>>(VIMemStream &s, VIRec32_C035B0 &r);         // 0x00C03988

// The WHO LIST client, VIClient+0x38334 (vtable slot 11), 24 bytes: Open
// (VIClient::OpenPanel35) asks with 0x0E01 (sub-op 1 and the request number m_serial); a
// 0x0E02 reply whose number is not the current one is dropped; Close bumps the number.
// The class name is kept; method names provisional.
class VIClientPart38334
{
public:
    VIClientPart38334(VIClient *owner);                         // 0x00C02FB0
    ~VIClientPart38334();                                       // 0x00C02FE8
    void Open();                                                // 0x00C03038
    VIVector<VIRec32_C035B0> *GetList();                        // 0x00C03128
    void Close();                                               // 0x00C03130
    void Recv0E02(VIMemStream &s);                              // 0x00C03168
    void Recv01(VIMemStream &s);                                // 0x00C031C8: the list

    VIClient                *m_owner;               // +0x00
    int                      m_serial;              // +0x04: the request number
    VIVector<VIRec32_C035B0> m_list;                // +0x08
    int                      m_pending;             // +0x14
};

// The crafting client's records.  A recipe, 144 bytes (the VIArray placeholder
// name is kept: client_viarray.cpp's members carry it).  m_id is the key, m_order the list
// position Recv08 rearranges; Select filters on m_x0D/m_x0E.
struct VIRec144_C017C0
{
    int            m_x00;                           // +0x00
    int            m_order;                         // +0x04: qsort's key
    int            m_id;                            // +0x08
    unsigned char  m_level;                         // +0x0C: drawn as %d (name ours)
    unsigned char  m_x0D;                           // +0x0D
    unsigned char  m_x0E;                           // +0x0E
    unsigned char  m_difficulty;                    // +0x0F: 0..3, words 1648..1651 (name ours)
    unsigned short m_name[64];                      // +0x10
    int Read(VIMemStream &s);                       // 0x00C001D8: returns 0
};
// VICraftDetail's two lists, 130 bytes each: an ingredient (a name and how many; 0 is not
// shown) and a tool (a name and whether the player has it, shown "(...)").
struct VIRecipeIngredient
{
    unsigned short m_name[64];                      // +0x00
    unsigned short m_count;                         // +0x80
};
struct VIRecipeTool
{
    unsigned short m_name[64];                      // +0x00
    unsigned char  m_have;                          // +0x80
};
// One of the six skill slots the crafting replies carry: four words.
struct VICraftSlot
{
    int m_x00;                                      // +0x00
    int m_x04;                                      // +0x04: Recv09 shows a rise
    int m_x08;                                      // +0x08
    int m_x0C;                                      // +0x0C
};
// The recipe opened (sub-op 16), 180 bytes: ids, flags, a name, an owned text, and three
// lists.  Names provisional.
class VICraftDetail
{
public:
    VICraftDetail();                                // 0x00BFFB68
    VICraftDetail(const VICraftDetail &d);          // 0x00BFFBA8
    ~VICraftDetail();                               // 0x00BFFC00
    int  operator=(const VICraftDetail &d);         // 0x00BFFC78: returns 0
    int  Read(VIMemStream &s);                      // 0x00BFFE68: <0 when a list will not allocate
    int  Clear();                                   // 0x00C00158
    int            m_x00;                           // +0x00
    int            m_x04;                           // +0x04
    int            m_x08;                           // +0x08
    unsigned char  m_x0C, m_x0D, m_x0E;             // +0x0C
    unsigned char  m_rarity;                        // +0x0F: 0 Common .. 3 Ultra Rare
    unsigned short m_count;                         // +0x10: what one make yields
    unsigned short *m_desc;                         // +0x14: new[], freed by Clear
    VIArray<VIRecipeIngredient> m_ingredients;      // +0x18
    VIArray<VIRecipeTool>       m_tools;            // +0x20
    unsigned short m_name[64];                      // +0x28: the product
    unsigned char  m_level;                         // +0xA8: "Product Item Lvl"
    VIArray<VIItemStat> m_stats;                    // +0xAC
};

// The crafting (tradeskill) client, VIClient+0x3834C (vtable slot 12):
// requests go out on 0x1101 as a sub-op byte, the server's token (m_p78C0) and arguments,
// one at a time (m_pending); replies arrive on 0x1102.  It drives the game window's craft
// panels, the recipe window (m_wndRecipe) and the experiment window (m_wndExperiment).
// Ids 0xA7xxxxxx are its own (IsMine/Use).  0x78C4 bytes, exactly to +0x3FC10.  The class
// name is kept; method names provisional.
class VIClientPart3834C
{
public:
    VIClientPart3834C(VIClient *owner);                         // 0x00BFCFB0
    ~VIClientPart3834C();                                       // 0x00BFD060
    int  Init(VIClientPart4F078 *factions);                     // 0x00BFD110: InitBody passes &m_p4F078 (unused)
    int  Clear();                                               // 0x00BFD140
    int  ReadSlots(VIMemStream &s);                             // 0x00BFD178: sub-op 14
    int  ReadList(VIMemStream &s);                              // 0x00BFD248
    int  GetSlot04(unsigned int i);                             // 0x00BFD318: 0 out of range
    int  GetSlot08(unsigned int i);                             // 0x00BFD338
    VIRec144_C017C0 *GetRecipe(int id);                         // 0x00BFD358
    int  Select(int a, int b, VIArray<VIRec144_C017C0> &out);   // 0x00BFD3A8: sorted by m_order
    int  IsMine(unsigned int id);                               // 0x00BFD778: a 0xA7xxxxxx id
    void Use(unsigned int id, int x);                           // 0x00BFD790
    void Send1(int a);                                          // 0x00BFD828
    void Send2(int a, int b);                                   // 0x00BFD948
    void Send3(int a, int b);                                   // 0x00BFDA88
    void Combine();                                             // 0x00BFDBC8: sub-op 4, the combine's item ids
    void Send5(int a);                                          // 0x00BFDDC8
    void Send6(int a);                                          // 0x00BFDED8
    void Send7(int a);                                          // 0x00BFDFF8
    void Recv1102(VIMemStream &s);                              // 0x00BFE118, name provisional
    void Recv07(VIMemStream &s);                                // 0x00BFE2A8
    void Recv08(VIMemStream &s);                                // 0x00BFE2B0: a recipe moved
    void Recv01(VIMemStream &s);                                // 0x00BFE450
    void Recv02(VIMemStream &s);                                // 0x00BFE490
    void Recv03(VIMemStream &s);                                // 0x00BFE508
    void Recv04(VIMemStream &s);                                // 0x00BFE510
    void Recv05(VIMemStream &s);                                // 0x00BFE540
    void Recv09(VIMemStream &s);                                // 0x00BFE5C8
    void Recv10(VIMemStream &s);                                // 0x00BFEA68
    void Recv11(VIMemStream &s);                                // 0x00BFEB70
    void Recv12(VIMemStream &s);                                // 0x00BFEC78
    void Recv13(VIMemStream &s);                                // 0x00BFEDA8
    void Recv15(VIMemStream &s);                                // 0x00BFEE30
    void Recv16(VIMemStream &s);                                // 0x00BFEEA0
    void ItemResult(VIMemStream &s, int experiment);            // 0x00BFEF08
    void TookItems(VIMemStream &s);                             // 0x00BFF4F8
    void TookItems2(VIMemStream &s);                            // 0x00BFF6A8
    void AddToCombine(int slot);                                // 0x00BFF858
    void RemoveFromCombine(int slot);                           // 0x00BFFA00
    void ClearCombine();                                        // 0x00BFFAB8
    int  Find(int id);                                          // 0x00BFFAD8: the recipe's pool index, -1
    void ResetP78C0() { m_p78C0 = 0; }                          // inline: EnterGameLoad stores through the part's own address
    void SetP78C0(int v) { m_p78C0 = v; }                       // inline: Msg000D, the same way

    VIClient          *m_owner;                     // +0x0000
    VIWndGame         *m_wnd;                       // +0x0004
    VIWndRecipe       *m_wndRecipe;                 // +0x0008
    VIWndExperiment   *m_wndExperiment;             // +0x000C
    VICraftSlot        m_slots[6];                  // +0x0010
    VIItemList         m_items;                     // +0x0070: the inventory as the combine sees it
    VIPool<VINetItem>  m_combine;                   // +0x77D8
    VIPool<VIRec144_C017C0> m_recipes;              // +0x77F0
    VICraftDetail      m_detail;                    // +0x7808
    int                m_pending;                   // +0x78BC
    int                m_p78C0;                     // +0x78C0: the server's token; it ends at VIClient+0x3FC10
    void SetGameWindow(VIWndGame *w) { m_wnd = w; }             // inline: InitBody stores through the part's own address
};

// FISHING, at VIClient+0x3FC10, 0x7774 bytes (its own TU, 0x00BEB7E0 .. 0x00BEC3E0):
// the catch list and the state; 0x0F02 is the server's side (sub-ops 1..10),
// 0x0F01 ours (the byte is the sub-op).  The words are 1431.. ("Let's go fishing!").
// VIWndFishing (0x00C4BAD8 ..) drives it through the game window.  Names provisional.
class VIClient;
class VIClientPart3FC10
{
public:
    VIClientPart3FC10(VIClient *c);                             // 0x00BEB7E0
    ~VIClientPart3FC10();                                       // 0x00BEB878
    void Recv0F02(VIMemStream &s);                              // 0x00BEB900: a byte, then one of the ten below
    void Start();                                               // 0x00BEBA10: 1, "Let's go fishing!"
    void Stop();                                                // 0x00BEBA78: 2
    void Ready();                                               // 0x00BEBAA8: 3
    void Caught(VIMemStream &s);                                // 0x00BEBAB8: 4, one catch into the inventory
    void Waiting();                                             // 0x00BEBB88: 5
    void Loot(VIMemStream &s);                                  // 0x00BEBB98: 7, the catch list, "You caught something!"
    void Missed();                                              // 0x00BEBC00: 8, "You fail to catch anything"
    void Lost();                                                // 0x00BEBC48: 9, "The fish got away with your bait"
    void Damaged();                                             // 0x00BEBC90: 10, "You damaged your pole"
    void Bite();                                                // 0x00BEBD30: 6
    void Cast(int quality);                                     // 0x00BEBD60: pole (wear 20) and bait (21) checks, then 0x0F01 1
    void Cancel();                                              // 0x00BEBF18: 0x0F01 2 (Msg00B2, when active)
    void Reel();                                                // 0x00BEBFE8: 0x0F01 3
    void Pull();                                                // 0x00BEC0B8: 0x0F01 4
    void Keep(int slot, int x);                                 // 0x00BEC188: 0x0F01 5, the catch's slot id
    int  HasEquipped(int wear);                                 // 0x00BEC2B8: an equipped inventory item of that wear slot
    VINetItem *FindEquipped(int equip);                         // 0x00BEC358: the inventory item in that equipment slot
    int  IsActive() { return m_state; }                         // inline: Msg00B2 reads it through the part's own address
    VIClient   *m_client;                                       // +0x0000
    VIWndGame  *m_wnd;                                          // +0x0004: InitBody stores m_wndGame
    VIItemList  m_catch;                                        // +0x0008
    int         m_state;                                        // +0x7770: 0 off, 1 waiting, 2 cast, 3 caught, 4 keeping
    void SetGameWindow(VIWndGame *w) { m_wnd = w; }             // inline: InitBody stores through the part's own address
};

// The auction query, 192 bytes: VIBankControl keeps the current one at
// +0x77A0 (its ctor's defaults: -1, -1, 1, 50, 0, 0x7FFFFFFF, 0, 0, true), and the auction
// senders stream it whole through 0x00BB4A80 / 0x00BB4B80.  Field names provisional.
struct VIAuctionQuery
{
    unsigned char  m_type;                         // +0x00
    char           m_name[24];                     // +0x01: ReadString(.., 24)
    unsigned short m_text[64];                     // +0x1A: ReadString(.., 128)
    unsigned int   m_p9C;                          // +0x9C: raw; the ctor's -1 is lui+ori
    unsigned int   m_pA0;                          // +0xA0: raw
    int            m_pA4;                          // +0xA4: zigzag, and the five after it
    int            m_pA8;
    int            m_pAC;
    int            m_pB0;
    int            m_pB4;
    int            m_pB8;
    bool           m_pBC;                          // +0xBC: a byte on the wire
};
VIMemStream &operator<<(VIMemStream &s, const VIAuctionQuery &q);   // 0x00BB4A80
VIMemStream &operator>>(VIMemStream &s, VIAuctionQuery &q);         // 0x00BB4B80

// One auction listing, 792 bytes: an item and the lot around it.  NOT a
// VINetItem: its reader (0x00CBB150, VINetItem.cpp) reads seven zigzag words at
// +0x2E4.. and the seller, then the item (0x00CBACE8), where ReadNetItem reads VINetItem's.
// The synthesized copy (VIItem's out-of-line copy, then the words, then a byte loop for the
// name) is what every VIArray/VIPool of these inlines.  Field names provisional.
class VIAuctionItem : public VIItem
{
public:
    int  m_lotId;                                  // +0x2E4: what the 0x1206 replies match on
    int  m_quantity;                               // +0x2E8: the stack count
    int  m_curHp;                                  // +0x2EC
    int  m_charges;                                // +0x2F0
    int  m_p2F4;
    int  m_p2F8;
    int  m_p2FC;
    char m_seller[24];                             // +0x300
};
VIMemStream &operator>>(VIMemStream &s, VIAuctionItem &a);       // 0x00CBB150 (VINetItem.cpp)
VIMemStream &operator<<(VIMemStream &s, const VIAuctionItem &a); // 0x00CBB1F0 (VINetItem.cpp)

// The bank's two other record types are VICharacter.cpp's database rows (cl_char.h, included
// at the end of this file): a pool member and an array member need only the name here.
struct VIDbCharBid;                                // CHARACTER_BIDS' row, 24 bytes (0x00CA6848 reads one)
struct VIDbRemoteCharacterRow;                     // 44 bytes (0x00CA9820 reads one)

class VIWndGame;
// VIBankControl.cpp: the bank and the auction (VIClient+0x47388, 0x78D0 bytes to +0x4EC58).
// Requests go out as message 0x1205 with a sub-op byte (5 sell .. 9), 0x1254/0x1255 move
// items and money; replies come back as 0x1250/0x1251/0x1253 (Recv) and 0x1206 (Recv1206,
// sub-ops 128..137).  Method names are ours unless marked.
class VIBankControl
{
public:
    VIBankControl(VIClient *client);                            // 0x00BB8988
    ~VIBankControl();                                           // 0x00BB8AD0
    int  Count();                                               // 0x00BB8BE0, name provisional: the bank list's size
    VINetItem *Item(int i);                                     // 0x00BB8BE8, name provisional: the bank's item list
    int  GetMoney();                                            // 0x00BB8C08: the client's GetPlayerP38
    void Close();                                               // 0x00BB8C28, name provisional
    int  IsIdle();                                              // 0x00BB8C30: no request waiting
    int  IsSearching();                                         // 0x00BB8C40: state 5
    void Sell(int slot, int a, int b, int c);                   // 0x00BB8C50: 0x1205 sub-op 5
    void Send06(int a, int b);                                  // 0x00BB8DF0: 0x1205 sub-op 6
    int  GetP78CC();                                            // 0x00BB8F08
    void Search(const VIAuctionQuery &q);                       // 0x00BB8F10: 0x1205 sub-op 7, the first page
    void NextPage();                                            // 0x00BB9110: sub-op 7 after the last lot
    void PrevPage();                                            // 0x00BB9278: sub-op 7 before the first lot
    void RequestLots();                                         // 0x00BB93D0: sub-op 8, the lots m_lots holds
    void RequestBids();                                         // 0x00BB9548: sub-op 8, the auctions m_bids holds
    void ClearResults();                                        // 0x00BB96C0
    void ClearSaved();                                          // 0x00BB9738
    void SaveResults();                                         // 0x00BB9788
    int  GetP7860();                                            // 0x00BB97D8
    VIArray<VIAuctionItem> *GetResults();                       // 0x00BB97E0
    VIArray<int> *GetP7880();                                   // 0x00BB97E8
    VIArray<int> *GetP7888();                                   // 0x00BB97F0
    VIArray<int> *GetP7890();                                   // 0x00BB97F8
    int  GetMinutes();                                          // 0x00BB9800: m_p7868 + minutes since m_p7870
    void Send09(int n);                                         // 0x00BB9848: 0x1205 sub-op 9
    void ResetP78BC();                                          // 0x00BB9928
    int  GetP78BC();                                            // 0x00BB9950
    VIArray<VIDbRemoteCharacterRow> *GetP78C0();                // 0x00BB9958
    int  FindBid(int auction, int *a, int *b);                  // 0x00BB9960
    void Move(unsigned char fromBank, int slot, int n);         // 0x00BB9A18: 0x1254
    void MoveMoney(unsigned char fromBank, int n);              // 0x00BB9B78: 0x1255
    void Read(VIMemStream &s);                                  // 0x00BB9C80
    void Recv(int opcode, VIMemStream &s);                      // 0x00BB9CF0, name provisional
    void Recv1206(VIMemStream &s);                              // 0x00BB9D90, name provisional (VIBankControl.cpp:593)
    void RecvAdded(VIMemStream &s);                             // 0x00BB9EE8: 0x1251
    void RecvTook(VIMemStream &s);                              // 0x00BB9F68: 0x1250
    void RecvAddLot(VIMemStream &s);                            // 0x00BBA008: 0x1206 sub-op 130
    void RecvRemoveLot(VIMemStream &s);                         // 0x00BBA080: 131
    void RecvAddBid(VIMemStream &s);                            // 0x00BBA138: 132
    void RecvRemoveBid(VIMemStream &s);                         // 0x00BBA180: 133
    void RecvUpdateBid(VIMemStream &s);                         // 0x00BBA230: 134
    void RecvResults(VIMemStream &s);                           // 0x00BBA2F8: 129
    void RecvUpdateLot(VIMemStream &s);                         // 0x00BBA5D0: 137
    void RecvClear();                                           // 0x00BBA878: 128
    void RecvRemoteChars(VIMemStream &s);                       // 0x00BBA8A0: 135
    static int CompareRemoteChars(const void *a, const void *b); // 0x00BBA928: qsort, descending by m_x24
    void RecvP78BC(VIMemStream &s);                             // 0x00BBAA00: 136
    int  IsCommand(unsigned int id);                            // 0x00BBAA48: 0xA8xx0000
    void Do(int id, int arg);                                   // 0x00BBAA60: 0xA801..0xA803 open the window (arg unused; DoAction hands every part (id, arg))
    int  HasLot(int lotId);                                     // 0x00BBAAB0
    VIClientConn GetConn();                                     // 0x00BBAB40: the client's GetConn(251)

    VIClient      *m_client;                       // +0x0000
    VIWndGame     *m_wnd;                          // +0x0004: InitBody stores the game window
    VIItemList     m_bank;                         // +0x0008
    VIPool<VIAuctionItem> m_lots;                  // +0x7770
    VIPool<VIDbCharBid> m_bids;                    // +0x7788: CHARACTER_BIDS rows
    VIAuctionQuery m_query;                        // +0x77A0: Search keeps the last query
    int            m_p7860;                        // +0x7860: 1 until a results page arrives
    int            m_page;                         // +0x7864: the request serial, ClearResults bumps it
    int            m_p7868;                        // +0x7868
    long           m_p7870;                        // +0x7870: the client clock at the last page
    VIArray<VIAuctionItem> m_results;              // +0x7878: the page shown
    VIArray<int>   m_p7880;
    VIArray<int>   m_p7888;
    VIArray<int>   m_p7890;
    VIArray<VIAuctionItem> m_saved;                // +0x7898: the page before
    VIArray<int>   m_p78A0;
    VIArray<int>   m_p78A8;
    VIArray<int>   m_p78B0;
    int            m_p78B8;                        // +0x78B8
    int            m_p78BC;                        // +0x78BC
    VIArray<VIDbRemoteCharacterRow> m_p78C0;       // +0x78C0
    int            m_state;                        // +0x78C8: the reply waited for (0 none)
    int            m_p78CC;                        // +0x78CC: Read's flag
    void SetGameWindow(VIWndGame *w) { m_wnd = w; }             // inline: InitBody stores through the part's own address
};

// The Class Mastery client's records (claude-notes/cm-wire-format.md).
// The three VIArray element types keep their placeholder names (client_viarray.cpp's
// instantiations carry them); their fields are read off VICMNode::Read.
struct VIRec8_BF9990                                // a requirement: 8 bytes
{
    unsigned char  m_x00;                           // +0x00
    int            m_x04;                           // +0x04: zigzag
};
struct VIRec130_BF9AB8_A2                           // a named entry: 130 bytes
{
    unsigned short m_text[64];                      // +0x00
    unsigned char  m_x80;                           // +0x80
};
struct VIRec128_BF9BE0                              // a line of text: 128 bytes
{
    unsigned short m_text[64];
};
// One skill of the 0x1402 list (sub-op 8), 152 bytes: the key is m_id.  Names provisional.
struct VICMSkill
{
    int  Read(VIMemStream &s);                      // 0x00BF80A0: returns 0
    int            m_x00;                           // +0x00
    unsigned char  m_x04;                           // +0x04: sub-op 5 sets it, 6 clears it
    unsigned char  m_x05;                           // +0x05: sub-op 7 and the flag lists set it
    int            m_id;                            // +0x08
    int            m_x0C;                           // +0x0C
    unsigned char  m_x10;                           // +0x10
    int            m_x14;                           // +0x14
    unsigned short m_name[64];                      // +0x18
};
// The skill opened (sub-op 9), 852 bytes: its fields, name, title and text, then three
// lists.  Names provisional.
class VICMNode
{
public:
    int  Clear();                                   // 0x00BF8150: ids -1, the rest 0, the lists emptied
    int  Read(VIMemStream &s);                      // 0x00BF81F0: <0 when a list will not allocate
    int            m_id;                            // +0x000
    unsigned char  m_x04;                           // +0x004
    unsigned char  m_x05;                           // +0x005
    int            m_x08;                           // +0x008
    int            m_x0C;                           // +0x00C
    unsigned char  m_x10;                           // +0x010
    unsigned char  m_x11;                           // +0x011
    int            m_x14;                         // +0x014
    int            m_x18;                         // +0x018
    int            m_x1C;                         // +0x01C
    int            m_x20;                         // +0x020
    int            m_x24;                         // +0x024
    int            m_x28;                         // +0x028
    int            m_x2C;                         // +0x02C
    int            m_x30;                         // +0x030
    int            m_x34;                         // +0x034
    unsigned short m_name[64];                      // +0x038
    int            m_xB8;                           // +0x0B8
    unsigned short m_title[64];                     // +0x0BC
    unsigned short m_text[256];                     // +0x13C
    VIArray<VIRec8_BF9990>      m_reqs;             // +0x33C
    VIArray<VIRec130_BF9AB8_A2> m_entries;          // +0x344
    VIArray<VIRec128_BF9BE0>    m_lines;            // +0x34C
};

// The Class Mastery client, VIClient+0x4EC58 (vtable slot 14): requests
// go out on 0x1401 as a sub-op byte, the server's token (+0x41C) and an argument; one
// request at a time (m_pending is the sub-op waited for).  Replies arrive on 0x1402.
// 0x420 bytes, exactly to +0x4F078.  The class name is kept; method names provisional.
class VIClientPart4EC58
{
public:
    VIClientPart4EC58(VIClient *owner);                         // 0x00BF6E98
    ~VIClientPart4EC58();                                       // 0x00BF6F20
    int  Init();                                                // 0x00BF6F90 (InitBody)
    void ReadBonuses(VIMemStream &s);                           // 0x00BF6FF0: 7 zigzag ints
    void ReadStats(VIMemStream &s);                             // 0x00BF7060: 29 zigzag ints
    void ReadFlags(VIMemStream &s);                             // 0x00BF70D0: (id, flag) pairs into the skills
    int  ReadState(VIMemStream &s);                             // 0x00BF71A0
    void Recv1402(VIMemStream &s);                              // 0x00BF7240, name provisional
    void Recv01(VIMemStream &s);                                // 0x00BF7358
    void Recv02(VIMemStream &s);                                // 0x00BF73B0
    void Recv03(VIMemStream &s);                                // 0x00BF73E8: points gained, a notice
    void Recv04(VIMemStream &s);                                // 0x00BF74E0
    void Recv05(VIMemStream &s);                                // 0x00BF7510: a skill on
    void Recv06(VIMemStream &s);                                // 0x00BF7600: a skill off
    void Recv07(VIMemStream &s);                                // 0x00BF76C8: one skill's flag
    void Recv08(VIMemStream &s);                                // 0x00BF7748: the skill list
    int  ReadList(VIMemStream &s);                              // inline (viclientpart4ec58.cpp): Recv08's body, 3834C's ReadList's twin
    void Recv09(VIMemStream &s);                                // 0x00BF7810: one skill opened
    int  Find(int id);                                          // 0x00BF78A8: the skill's pool index, -1
    int  IsMine(unsigned int id);                               // 0x00BF7938: a 0xA9xxxxxx id
    void Use(unsigned int id, int arg);                         // 0x00BF7950: empty (DoAction's (id, arg) for this part; 3834C's Use twin)
    void Select(int n);                                         // 0x00BF7958: sub-op 1, n clamped to 0..100
    void Send02(int x);                                         // 0x00BF7A90: sub-op 2
    void Send03(int x);                                         // 0x00BF7BA8: sub-op 3
    void Open();                                                // 0x00BF7CC0, name provisional: Close, then sub-op 4
    void Browse(int id);                                        // 0x00BF7DC8: sub-op 5, the node cleared first
    void Close();                                               // 0x00BF7EE8, name provisional
    int  IsKnown(int id);                                       // 0x00BF7F18: either flag of the skill
    int  GetBonus(int i);                                       // 0x00BF7F80: m_bonus[i], 0 out of range (was "Count")
    int  GetStat(int id);                                       // 0x00BF7FA8: m_stats by a 29-entry id table
    void ResetP41C() { m_p41C = 0; }                            // inline: EnterGameLoad stores through the part's own address
    int  Points() { return m_x0C; }                             // inline: ClassMastery::OnDrawSelf's
                                                                //   "%d/%d" loads it before the 2nd GetP4EC58() call

    VIClient          *m_owner;                     // +0x000
    VIWndClassMastery *m_wnd;                       // +0x004: shown the opened skill (0x00C34F48)
    int                m_x08;                       // +0x008
    int                m_x0C;                       // +0x00C: points (Recv03 announces a rise)
    int                m_x10;                       // +0x010
    int                m_x14;                       // +0x014
    int                m_bonus[7];                  // +0x018
    int                m_stats[29];                 // +0x034
    float              m_speed;                     // +0x0A8: handed to VIClient::SetSpeed
    VICMNode           m_node;                      // +0x0AC
    VIPool<VICMSkill>  m_skills;                    // +0x400
    int                m_pending;                   // +0x418: the sub-op waited for, 0 none
    int                m_p41C;                      // +0x41C: the server's token; Open sends it
};

// The game alarm at VIClient+0x4F098 (24 bytes, its own TU: 0x00BE3C10 .. 0x00BE3F98, then
// that TU's template copies).  The chat command sets it ("Alarm set for %d hour(s), %d
// minute(s)." word 1200); Tick prompts word 1201 once the time passes.  VIClient
// embeds it as m_alarm.  Name ours.
class VIClient;
class VIAlarm
{
public:
    VIAlarm();                                                  // 0x00BE3C10
    int  IsOn();                                                // 0x00BE3C20
    void Command(VIClient *c, const unsigned short *arg);       // 0x00BE3C28: "Off" (word 552) or "H[:MM]"
    void Set(VIClient *c, int on, int hours, int minutes);      // 0x00BE3DF0
    int  Tick(VIClient *c);                                     // 0x00BE3F18
    int  m_on;                                                  // +0x00
    int  m_hours;                                               // +0x04
    int  m_minutes;                                             // +0x08
    long m_when;                                                // +0x10: VIClient::m_p07568 at which it rings
};

// One faction standing as 0x07E2 carries it, 132 bytes: a wide name and a value (VIWndFaction
// lists them).  Placeholder name kept: client_viarray.cpp's VIArray copies carry it.
// Its reader is 0x00CBA990: ReadString(128) then a zigzag int.  Field names ours.
struct VIRec132_BEB670
{
    unsigned short m_name[64];                                  // +0x00
    int            m_value;                                     // +0x80
};
VIMemStream &operator>>(VIMemStream &s, VIRec132_BEB670 &r);    // 0x00CBA990

// The faction standings at VIClient+0x4F078, 32 bytes (its own TU, 0x00BEA670 .. 0x00BEB7DC).
// Request sends 0x0AC4; 0x07E2 (byte 119) answers with m_seq, two ints and the
// list.  VIWndFaction (0x00C49D40 ..) is its user.  Names provisional.
class VIClientPart4F078
{
public:
    VIClientPart4F078(VIClient *c);                             // 0x00BEA670
    void Request(int x);                                        // 0x00BEA6B8: Reset, then 0x0AC4 (byte 102, m_seq, x)
    void Reset();                                               // 0x00BEA7C0: a new m_seq, the list emptied
    int  IsReady();                                             // 0x00BEA800
    int  GetP10();                                              // 0x00BEA808
    int  GetP14();                                              // 0x00BEA810
    VIArray<VIRec132_BEB670> *GetList();                        // 0x00BEA818
    void Recv07E2(VIMemStream &s);                              // 0x00BEA820
    void RecvList(VIMemStream &s);                              // 0x00BEA880: the reply to the last Request only
    int  IsFactionId(unsigned int id);                          // 0x00BEA910: top byte 0xAA
    void Command(unsigned int id, int arg);                     // 0x00BEA928: 0xAA000001 opens the game window's panel (arg unused)
    void Show();                                                // 0x00BEA950
    VIClient  *m_client;                                        // +0x00
    VIWndGame *m_wnd;                                           // +0x04: InitBody stores m_wndGame
    int        m_seq;                                           // +0x08: bumped by every Reset
    int        m_ready;                                         // +0x0C: set by RecvList
    int        m_p10;                                           // +0x10
    int        m_p14;                                           // +0x14: -1 until a reply
    VIArray<VIRec132_BEB670> m_list;                            // +0x18
    void SetGameWindow(VIWndGame *w) { m_wnd = w; }             // inline: InitBody stores through the part's own address
};

class VIClient;
// The gram (in-game mail) records, 0x00CA9380.. (VICharacter.cpp writes their
// operators: >> 0x00CA9380, << 0x00CA9428, the message's 0x00CA94E8/0x00CA9528 and its
// operator= 0x00CA9568).  Defined here because VIClientPart2EFD0 embeds 101 of them and
// VIClient embeds that by value.  An inbox row, 176 bytes; field names provisional except
// m_id (every lookup keys on +0x04).  Wire order: +0x04, +0x00, +0x08, then the rest.
struct VIGramHeader
{
    int            m_x00;           // +0x00
    int            m_id;            // +0x04
    int            m_x08;           // +0x08
    char           m_from[24];      // +0x0C: ReadString(char *, 24)
    unsigned short m_subject[64];   // +0x24: ReadString(wide, 128)
    VIDate         m_date;          // +0xA4
    unsigned char  m_read;          // +0xAC: RecvMessage sets 1 when the text arrives
};
// The gram opened: its row, then its text (512 characters).  1,200 bytes.
struct VIGramMessage : public VIGramHeader
{
    VIGramMessage &operator=(const VIGramHeader &h);  // 0x00CA9568: the row, an empty text
    unsigned short m_text[512];     // +0xB0
};
VIMemStream &operator>>(VIMemStream &s, VIGramHeader &h);           // 0x00CA9380
VIMemStream &operator<<(VIMemStream &s, const VIGramHeader &h);     // 0x00CA9428

// The gram client, VIClient+0x2EFD0, vtable slot 7: VICSClient's twin (vicsclient.h) on
// message 0x0D41, each request wrapped in a 0x0D01 envelope (a blob) with the same sub-op
// byte.  Sub-ops: 1 send, 2 inbox, 3 open a gram, 4 delete, 5 (in) new mail.  0x4980
// bytes, exactly to VICSClient at +0x33950.  Method names provisional.
class VIClientPart2EFD0
{
public:
    VIClientPart2EFD0(VIClient *owner);                         // 0x00BF5628
    ~VIClientPart2EFD0();                                       // 0x00BF5648
    void Recv0D41(VIMemStream &s);                              // 0x00BF5670, name provisional
    void OnConnect();                                           // 0x00BF5738, name provisional (the pump's 0xFF)
    void Reset();                                               // 0x00BF57A0, name provisional
    void RequestInbox();                                        // 0x00BF57B0: sub-op 2
    void RequestMessage(int id);                                // 0x00BF5920: sub-op 3
    void DeleteMessage(int id);                                 // 0x00BF5B08: sub-op 4
    void SendGram(const char *to, const unsigned short *subject,
                  const unsigned short *text);                  // 0x00BF5DE8: sub-op 1
    int  GetState();                                            // 0x00BF5F98
    int  GetX08();                                              // 0x00BF5FA0
    int  GetCount();                                            // 0x00BF5FA8
    VIGramHeader *GetHeader(int i);                             // 0x00BF5FB0
    VIGramMessage *GetMessage();                                // 0x00BF5FC8
    void RecvInbox(VIMemStream &s);                             // 0x00BF5FD0
    void RecvMessage(VIMemStream &s);                           // 0x00BF60A8
    void RecvSend(VIMemStream &s);                              // 0x00BF6180
    void RecvDelete(VIMemStream &s);                            // 0x00BF6240

    VIClient     *m_owner;          // +0x0000
    int           m_state;          // +0x0004: the reply waited for; <0 an error
    int           m_x08;            // +0x0008: set by sub-op 5, cleared by an inbox
    int           m_count;          // +0x000C: -1 until the first inbox
    VIGramHeader  m_headers[100];   // +0x0010
    VIGramMessage m_message;        // +0x44D0
};

// Three sub-objects only the vtable names (slots 6, 16, 18).  The first two are four
// bytes apart, just below +0x2EFD0.  Names provisional.
class VIClientPart2EFC8
{
public:
    VIClientPart2EFC8(VIClient *c);                             // 0x00BED878
    ~VIClientPart2EFC8();                                       // 0x00BED888
    void Send(int x);                                           // 0x00BED8B0 (func_00BD6250)
    VIClientConn GetConn();                                     // 0x00BED990: the client's slot-1 GetConn(251)
    VIClient *m_client;                                         // +0x00
};
// VIClient+0x2EFCC (slot 6), 4 bytes: the client, and two senders of message 0x0C01 (sub 2
// with the rest of a slash command's text, sub 1 with a byte).  0x00CBBDD8 ..; names
// provisional.
class VIClientI;
class VIClientPart2EFCC
{
public:
    VIClientPart2EFCC(VIClientI *client);           // 0x00CBBDD8
    ~VIClientPart2EFCC();                           // 0x00CBBDE8
    void SendText(const unsigned short *text);      // 0x00CBBE10: 0x0C01, 2, text
    void SendFlag(int b);                           // 0x00CBBEE0: 0x0C01, 1, b (a byte on the wire)
    VIClientConn GetConn();                         // 0x00CBBFC0: m_client->GetConn(251)
    VIClientI *m_client;                            // +0x00
};
// A Mersenne twister, VIClient+0x4F234: STATION's VIRandomMT, the same source (Shawn
// Cokus's MT19937 with its globals made members; virandom.cpp, byte-identical in both
// modules).  2,508 bytes that run exactly to +0x4FC00.
class VIRandomMT
{
public:
    VIRandomMT();                           // 0x00CBB6A8: left = -1
    void         Seed(unsigned int seed);   // 0x00CBB6B8
    unsigned int Reload();                  // 0x00CBB700
    unsigned int Random();                  // 0x00CBB8B0
    float        RandomFloat();             // 0x00CBB928: [0, 1), 24 bits
    double       RandomDouble();            // 0x00CBB988: [0, 1), 53 bits
    int          RandomRange(int lo, int hi);   // 0x00CBBA48: [lo, hi)

    unsigned int  m_state[625];   // +0x000
    unsigned int *m_next;         // +0x9C4
    int           m_left;         // +0x9C8
};
typedef VIRandomMT VIRandom;      // VIClient::m_random's type (slot 16)
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
    VIClientText();                                             // 0x00BBDEE8: m_client = 0
    ~VIClientText();                                            // 0x00BBDEF8: m_client = 0
    int SetClient(VIClient *client);                            // 0x00BBDF20: returns 0 (VIInfoText's shape)
    int Expand(unsigned short *dst, unsigned short size, const unsigned short *src, int flag);  // 0x00BBDF30
    // 0x00BBE450: one token after a '%': a decimal number, or one letter / & * ( %.
    const unsigned short *ParseToken(const unsigned short *p, int *tok);
    VIClient *m_client;                                         // +0x00
};
// A queued dialog line: 218 wide characters (ShowDialog's buffer).
struct VIRec436_BDF6D8 { unsigned short m_text[218]; };
// Two more known only by an accessor returning their address.  Names provisional.
// The pad state VIClient keeps at +0x7190, 84 bytes: PadKeys copies the new one over it
// after turning each changed analog key into SetKey(128 + i, ...).  Names provisional.
class VIClientPart07190
{
public:
    unsigned char m_p00;                             // +0x00: JournalConsole records data[0] and data[1]
    unsigned char m_p01;                             // +0x01  (Tick reads each back as a byte)
    float m_keys[20];                                // +0x04: 0.0 = up
};

// VIClientConn (slot 1's result) and VIClientDrdpRef are in vinet.h, shared with STATION.
void ReleaseMat(VIRaster &r, VIDictionary &d, int mat);                     // 0x00BBFE00, name provisional: VIClient.cpp's guard round ReleaseMaterial

// 16 of these at VIClient+0x2C5A0, 20 bytes: two words and a position.  Names provisional.
struct VIRec20_2C5A0
{
    int     m_p00;
    int     m_p04;
    VIVect3 m_pos;                                  // +0x08
};

// VIClient+0x4F098 is a VIAlarm.

// VIClient's base, RTTI "9VIClientI".  It has no data, so the vptr is at +0.  The
// vtable (0x00CDB4C0, 160 bytes) is entry 0 __tf8VIClient, these 18 virtuals in this
// order, and a NULL end: the destructor is slot 2 and slots 3-18 return sub-objects,
// the ones the dispatcher hands each subsystem's opcodes to.  Names provisional.
class VIClientI
{
public:
    // ONE slot, as STATION's (src/viclienti.h there): no destructor, so VIClient's dtor calls
    // no base dtor and does its own delete; the dtor and the 16 getters are VIClient's own
    // virtuals, slots 2..18 in declaration order.
    virtual VIClientConn GetConn(int type) = 0;                 // slot 1
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
    virtual ~VIClient();                              // 0x00BC2638, 1,760 bytes: vtable slot 2 (the first of VIClient's own virtuals)
    int  Init(const char *cmd, char *a, const char *c, char *b, int record, int f6, int f7, int f8);  // 0x00BBFE80: the journal's start-up record, then InitBody
    int  InitBody();                                            // 0x00BC0098, not yet written
    bool IsRunning();                                 // 0x00BC2D18: `!m_quit`
    void Process();                                   // 0x00BC7BF0: one frame, 2,568
    void SendPosition();                              // 0x00BC2F30, name provisional: the 0x40 (Process, when the update interval passed)
    void SendReport(const char *to);                  // 0x00BCDE88, name provisional: 0x00F0 with the game window's text ring ("Text=" + lines joined by |)
    void ChatLine(const unsigned short *text);        // 0x00BCFE78, name provisional: a typed line, /commands or the current channel
    void DoAction(int id, int arg);                   // 0x00BD6250, name provisional: the action dispatcher (menus, /commands)
    int  StatWithItem(int index, int stat, int source); // 0x00BCA2E8, name provisional: stat's total with item index of source (0 inventory, 1 merchant, 2 bank) equipped
    void DefaultTextSets();                           // 0x00BDD5B0, name provisional: the quick-chat menus' built-in sets (custom ones dropped, each sent as 0x006B)
    void UpdateGame();                                // 0x00BC6500, name provisional: one in-game frame (VIWndGame calls it)
    VINetItem *GetInvItem(int i);                     // 0x00BE3B38, name provisional: m_inventory.Item(i)
    void UpdateControls();                            // 0x00BC3AE8, name provisional (not yet written): UpdateGame's control/camera step
    void TrackTarget(const VIVect3 &pos, unsigned int id, int b, VIMatrix44 *m); // 0x00BC4B18, name provisional (not yet written)
    int  InitPlayback(const char *path);              // 0x00BBFE30, name provisional: Open(2) the journal, then InitBody
    int  PlayLoop(const char *name);                  // 0x00BD8EB8, name provisional: an XM by name, unless m_loopHandle still plays
    void SendRaw(unsigned long endpoint);             // 0x00BDE1A0, name provisional: two 0xFF bytes to an endpoint's address
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
    VIAlarm      *GetP4F098();                   // 0x00BE3918
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

    // The rest of the vtable (VIClientI's order).
    VIClientConn GetConn(int type);                   // 0x00BD5EE8: slot 1
    virtual VIBankControl     *GetBank();                     // 0x00BCCB58: slot 3
    virtual VIClientGuild     *GetGuild();                    // 0x00BCDE38: slot 4
    virtual VIClientPart2EF24 *GetP2EF24();                   // 0x00BCDE48: slot 5
    virtual VIClientPart2EFCC *GetP2EFCC();                   // 0x00BCDE58: slot 6
    virtual VIClientPart2EFD0 *GetP2EFD0();                   // 0x00BCDE68: slot 7
    virtual VICSClient *GetP33950();                   // 0x00BCDE78: slot 8
    virtual VIClientPart38300 *GetP38300();                   // 0x00BCE1A0: slot 9
    virtual VIClientPart382E8 *GetP382E8();                   // 0x00BCE1B0: slot 10
    virtual VIClientPart38334 *GetP38334();                   // 0x00BCE1C0: slot 11
    virtual VIClientPart3834C *GetP3834C();                   // 0x00BCE1D0: slot 12
    virtual VIClientPart3FC10 *GetP3FC10();                   // 0x00BCE1E0: slot 13
    virtual VIClientPart4EC58 *GetP4EC58();                   // 0x00BCE1F0: slot 14
    virtual VIClientPart4F078 *GetP4F078();                   // 0x00BCE200: slot 15
    virtual VIRandom *GetP4F234();                            // 0x00BCE7A0: slot 16
    virtual VIPlayer *GetPlayer();                            // 0x00BE39B0: slot 17
    virtual VIClientPart2EFC8 *GetP2EFC8();                   // 0x00BD7BD8: slot 18
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
    int  GetP44Total(int i);                                    // 0x00BD60C0 [takes the index GetBonus reads; its callers pass a1]

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

    int  LoadModels();                                       // 0x00BD4E60
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
    int  ReleaseGame();                                                 // 0x00BC0D80, name provisional (the dtor, Msg000D): the game's resources, the dictionary and scene made new
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
    int           m_p00084;                             // +0x00084: the ctor zeroes it
    int           m_p00088;                             // +0x00088: the ctor zeroes it
    int           m_loopHandle;                         // +0x0008C: PlayLoop's sound handle, -1 none (the ctor)
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
    VIClientPart06D00 m_p06D00;                         // +0x06D00: LoadModels inits it
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
    unsigned int  m_p07578;                             // +0x07578: 0x07B2; the ctor sets 100; unsigned (Process compares it lwu)
    char          m_g0757C[0x07580 - (0x0757C)];
    long          m_p07580;                             // +0x07580: Connect: the clock
    long          m_p07588;                             // +0x07588: Connect: the clock
    VIScene       m_scene;                              // +0x07590
    unsigned int  m_target;                             // +0x0C140: the target's entity id, -1 = none (lui+ori; FrontiersForge)
    int           m_pC144;                              // +0x0C144: EnterGameLoad sets -1
    int           m_reticleTex;                         // +0x0C148: DrawReticle's texture, -1 until AT_Reticle.tga is found
    int           m_ringsIndex;                         // +0x0C14C: "Selection_Rings"'s dictionary index
    float         m_pC150;                              // +0x0C150: the ctor sets 1.2f
    int           m_pC154;                              // +0x0C154: the ctor zeroes it
    VIColor32F    m_pC158;                              // +0x0C158: the ctor sets (0.5, 0.5, 0.5, 0)
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
    VIWndLogin *  m_wndLogin;                           // +0x2BB7C: ReturnToSelect activates it; ConnectShard reads the account
    VIWndServerSelect *m_wndServerSelect;                  // +0x2BB80: ShowLoginWindow activates it
    VIWndCharacterSelect *m_wndCharSelect;              // +0x2BB84: the character-select message window
    VIWndCharCreateBlank *m_wndCharCreate;              // +0x2BB88: 0x0012
    VIWndIdle *   m_wndIdle;                            // +0x2BB8C: SetActiveWindow: the loader progress callback runs while it is active
    VIWndInitFromMc *m_wndInitFromMc;                   // +0x2BB90: InitBody news it (432 bytes)
    char          m_g2BB94[0x2BB98 - (0x2BB94)];
    VIWndDisconnect m_wndDisconnect;                    // +0x2BB98: a window; Disconnect compares +0x2BD40 to it
    VIWnd *       m_p2BD40;                             // +0x2BD40: the active window (SetActiveWindow)
    VIWnd *       m_p2BD44;                             // +0x2BD44: Notify / CharSelectMsg store the window they activate
    int           m_p2BD48;                             // +0x2BD48: UpdateCreatures walks the creatures while set
    int           m_p2BD4C;                             // +0x2BD4C: Extrapolate moves nothing while it is clear
    int           m_p2BD50;                             // +0x2BD50: UnloadWorld zeroes it
    int           m_p2BD54;                             // +0x2BD54: InitBody stores a journal byte != 0  //
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
    VIRec20_2C5A0 m_p2C5A0[16];                         // +0x2C5A0: the ctor zeroes each VIVect3
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
    long          m_frameStats[5];                      // +0x2EC00: Process zeroes all five, prints [0] [2] [1] [3]
    int           m_p2EC28;                             // +0x2EC28: the ctor zeroes it
    int           m_p2EC2C;
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
    VIAlarm       m_alarm;                              // +0x4F098: GetP4F098 returns its address; ctor 0x00BE3C10
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
    int           m_p502FC;                             // +0x502FC: the ctor zeroes it
    int           m_p50300;                             // +0x50300: CharacterList: set, the login window gets Option(1)
    int           m_p50304;                             // +0x50304: 0x07D1
    int           m_p50308;                             // +0x50308: SetP50308 sets 1
    int           m_p5030C;                             // +0x5030C: SetP50308
    int           m_p50310;                             // +0x50310: SetP50308
    int           m_p50314;                             // +0x50314: SetP50308
    int           m_p50318;                             // +0x50318: the ctor zeroes it
    int           m_p5031C;
    long          m_p50320;                             // +0x50320: SetKey ignores keys until VITimer::GetTime reaches it
    char          m_g50328[0x50330 - (0x50328)];

    // The senders' TU (0x00BE4B78 .. 0x00BE6540) also holds these; only 0x00BE6188 has a
    // caller in the image.
    void SendOpByteByteInt(int op, unsigned char a, unsigned char b, int c);   // 0x00BE5900: no caller in the image
    void SendOpByteU16U16(int op, unsigned char a, unsigned short b, unsigned short c);  // 0x00BE5A10: no caller
    void SendOpByteIntInt(int op, unsigned char a, int b, int c);       // 0x00BE5E00: no caller
    void SendOpU16(int op, unsigned short v);                           // 0x00BE6000: no caller
    void SendOpWString(int op, const unsigned short *s);                // 0x00BE60C8: no caller
    void SendOpWStringInt(int op, const unsigned short *s, int v);      // 0x00BE6188: func_00BCFE78 x5
    void SendOpStrStr(int op, const char *a, const char *b);            // 0x00BE6470: no caller
};

// One header per group of TUs (cl_*.h), each declaring what those TUs define.
// Order = what embeds what BY VALUE: uitext's helpers before every
// window; wl (VIWndList, the base of Menu/MiniMenu/TradeList) and wm (VIWndMessage, embedded
// by ClassMasteryInspect/ComposeGram/ReadCS/ReadGram/Tell) before the other windows; wb
// (VIWndCharCreateBlank) before wc (VIWndCharCreateRace derives from it).
#include "cl_vic.h"
#include "cl_pre.h"
#include "cl_parts1.h"
#include "cl_parts2.h"
#include "cl_uitext.h"
#include "cl_char.h"
#include "cl_tail.h"
#include "cl_wl.h"
#include "cl_wm.h"
#include "cl_wa.h"
#include "cl_wb.h"
#include "cl_wc.h"
#include "cl_wd.h"
#include "cl_we.h"
#include "cl_wg.h"
#include "cl_wh.h"
#include "cl_wq.h"
#include "cl_ws.h"

#endif
