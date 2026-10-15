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
extern "C" void *memset(void *, int, int);
extern "C" void  free(void *);
extern "C" void  _VIAssert(const char *expr, const char *file, int line);
extern "C" void  _VIAssertf(int cond, const char *fmt, ...);
extern "C" const char *__VIAssertf_file;
extern "C" int   __VIAssertf_line;

// SUPPORT's spelling (vistringtable.cpp): the file and line go to two globals
// first, unconditionally, then the condition is passed down.
#define VIASSERT(x)  if (!(x)) _VIAssert(#x, __FILE__, __LINE__);
#define VIASSERTF(x, fmt, args...) \
    { __VIAssertf_file = __FILE__; __VIAssertf_line = __LINE__; _VIAssertf(x, fmt, ## args); }

// SUPPORT's DRDP library, declared here rather than by including drdp.h beside
// vi.h (no SUPPORT TU includes both).  Same C++ linkage, so the same names.
struct drdp_t;
struct connection_t;
void connection_close(connection_t *pConnection);
void drdp_flush(drdp_t *pDrdp);
void drdp_destroy(drdp_t *pDrdp);

// VIMemStream.cpp: the stream every network message is read through.  32 bytes,
// all words (phase 62 survey, from Write/Read/the dispatcher's inline ctor).
// The constructor and destructor are INLINE: the dispatcher builds one on its
// stack and frees an owned buffer on the way out.
struct VIMemStreamInt;

class VIMemStream
{
public:
    VIMemStream(const void *data, int size)
    {
        m_wbuf = 0;
        m_rbuf = (const char *)data;
        m_capacity = size;
        m_size = size;
        m_rpos = 0;
        m_error = 0;
        m_dryRun = 0;
        m_owned = 0;
    }
    ~VIMemStream()
    {
        if (m_owned && m_wbuf)
            free(m_wbuf);
    }

    void Read(void *dst, int size, int count);        // 0x00CB8520
    void Skip(int n);                                 // 0x00CB85A8
    void ReadSigned(int &v);                          // 0x00CB8610: zigzag
    void ReadUnsigned(unsigned int &v);               // 0x00CB86B8: 7-bit varint
    void ReadString(char *dst, unsigned int MaxBytes);            // 0x00CB8740 (line 399)
    void ReadString(unsigned short *dst, unsigned int MaxBytes);  // 0x00CB87E8 (line 422)

    // An inline reader (0x03D5).  An inlined helper's locals are known addressable
    // when they are laid out, so they take block temporaries: 0x03D5's byte sits at
    // sp+224, after the record, not with the other cases' bytes at sp+2112.
    bool ReadBool() { unsigned char b; Read(&b, 1, 1); return b != 0; }
    // The same read as a chainable operator (the guild member's reader chains it).
    VIMemStream &operator>>(bool &v) { unsigned char b; Read(&b, 1, 1); v = b != 0; return *this; }

    // An inline reader that takes its target BY VALUE as an 8-byte object (see
    // VIMemStreamInt below).
    inline VIMemStream &operator>>(VIMemStreamInt v);

    char       *m_wbuf;                               // +0x00: Write's buffer, freed if owned
    const char *m_rbuf;                               // +0x04: Read's source
    int         m_capacity;                           // +0x08
    int         m_size;                               // +0x0C: Read's limit
    int         m_rpos;                               // +0x10
    int         m_error;                              // +0x14: sticky overrun
    int         m_dryRun;                             // +0x18: advance without copying
    int         m_owned;                              // +0x1C
};

// What an int field is read through in VIPlayerRecord's reader, the VIMemStreamArray.h element
// loop and the dispatcher's 0x03D6: the target's address stored TWICE into a
// block temporary just before ReadSigned, and never read back.  That is an
// inlined function's parameter living in memory: under EABI an 8-byte struct of
// two 4-byte words is BLKmode (its 4-byte alignment is below DImode's), so the
// inliner store_expr's it into the simulated argument area, built in place by
// the constructor, and cse forwards the read.  A 4-byte wrapper travels in a
// register and leaves no store.  Both words are the field's address, which
// suits a two-way description of a field (read into it, write from it); the
// names are provisional, only the shape is measured.
struct VIMemStreamInt
{
    int       *m_in;
    const int *m_out;
    VIMemStreamInt(int &v) : m_in(&v), m_out(&v) {}
};

inline VIMemStream &VIMemStream::operator>>(VIMemStreamInt v)
{
    ReadSigned(*v.m_in);
    return *this;
}

// The game window behind VIClient+0x2BB78 (its methods sit between
// VIWndGame.cpp:4007 and VIWndGuild.cpp:503).
class VIWndGame
{
public:
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
    int  IsModal();                                             // 0x00C56A38, name provisional (DamageFromOther, QuestStage)
};

// The character-select message window behind VIClient+0x2BB84.
class VIWndLogin : public VIWnd
{
public:
    void ShowWord(unsigned short id);                           // 0x00C21668, name provisional (CharSelectMsg masks to 16 bits)
    void ShowText(const unsigned short *s);                     // 0x00C215F8, name provisional
};

// The window behind VIClient+0x2BB88.
class VIWnd2BB88 : public VIWnd
{
public:
    void SetName(const char *name);                             // 0x00C23460, name provisional
};

// An item list read off a stream (corpse loot +0x13B98, merchant +0x1B310).
class VINetItem;

class VIItemList
{
public:
    void Read(VIMemStream &s);                                  // 0x00BBC8D0, name provisional
    void Get(int slot, VINetItem &item);                        // 0x00BBD398, name provisional (Msg0019)
    void Took(int slot, int where);                             // 0x00BBD430, name provisional (Msg0019)
    void Clear() { m_00 = 0; m_04 = 0; }                       // inline: 0x0016 stores through &m_corpse
    int  m_00;                                         // +0x00: 0x0016 zeroes it
    int  m_04;                                         // +0x04: 0x0016 zeroes it first
};

class VIInvItem;

// The inventory at VIClient+0xC428 (slots of 764 bytes).
class VINetItem;

class VIInvList
{
public:
    void Op003D(int a, int b);                                  // 0x00BE02F0, name provisional (0x003D)
    void Op0060(VIInvItem *item, int z);                        // 0x00BE05D0, name provisional (0x0060)
    int  AddItem(VINetItem &item, int a, int b);                // 0x00BBD130, name provisional: returns where it went
    VIInvItem *Find(int id);                                    // 0x00BDFFA8, name provisional
};

// The array member at VINetItem+0x2DC: an out-of-line ctor and dtor.
class VIItemArray
{
public:
    VIItemArray();                                              // 0x00BBB758, name provisional
    ~VIItemArray();                                             // 0x00BBB7B8, name provisional
    int   m_size;
    void *m_data;
};

// The item 0x003B reads: 0x2E4 bytes, its array member at +0x2DC.
class VINetItem
{
public:
    char        m_p000[0x38];
    int         m_p38;                                // +0x38: AddInvItem passes it on
    char        m_p03C[0x2DC - 0x3C];
    VIItemArray m_array;                              // +0x2DC
    int         m_p2E4[3];
    int         m_p2F0;                               // +0x2F0: Msg0019 stores -1
    int         m_p2F4[2];                            // to 0x2FC, FrontiersForge's inventory record size
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
// viclient-xref-20261015.md).  The REAL class copies member-wise through an out-of-line
// Node copy constructor (func_00BE0AB8, which calls func_00BE0A88 for one member), so it
// has a member with a copy constructor of its own: the map's Allocate/Insert/AddNode are
// not reproduced by this plain declaration yet.
struct VIAbility
{
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
    unsigned short m_desc[132];                    // +0xBC
};

// The player's own record, VIClient+0xC2DC (FrontiersForge player.lua; offsets here are
// the record's).  Its head is a VIPlayerRecord, as the buddy and guild records' are; it
// runs to the inventory list at +0xC428, so it is 0x14C bytes.
class VIPlayer : public VIPlayerRecord
{
public:
    int           m_exp;                           // +0x28 (+0xC304): 0x0020
    int           m_p2C;                           // +0x2C (+0xC308): 0x00D9
    unsigned char m_p30;                           // +0x30 (+0xC30C): a byte getter
    char          m_p31[3];
    int           m_p34;                           // +0x34 (+0xC310): 0x0052
    int           m_p38[2];
    int           m_p40;                           // +0x40 (+0xC31C): 0x0036
    int           m_p44;                           // +0x44 (+0xC320): 0x001C
    char          m_p48[0x80 - 0x48];              // stat totals at +0x64..+0x7C (FrontiersForge)
    int           m_hp;                            // +0x80 (+0xC35C)
    int           m_maxHp;                         // +0x84
    int           m_power;                         // +0x88
    int           m_maxPower;                      // +0x8C
    char          m_p90[0x14C - 0x90];
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
    float         m_pos[3];                        // +0x10
    char          m_name[24];                      // +0x1C
};

// One of two 32-byte objects at VIClient+0x2EC30/+0x2EC50 that SetTime feeds a time
// and the 64-bit +0x7568 (a clock base).  Name provisional.
class VIClientClock
{
public:
    void Set(long t, long base);                                // 0x00CBCBD8, name provisional
    long m_p00[4];
};

// The window embedded at VIClient+0x2BB98, 0x1A8 bytes: Notify sets its app pointer and
// makes it the active window, and Disconnect compares the active window against it.
class VIWnd2BB98 : public VIWnd
{
public:
    char m_p24[0x1A8 - sizeof(VIWnd)];
};

// The object at VIClient+0x4FD28 that DamageFromSelf reports through.  Name provisional.
class VIClientPart4FD28
{
public:
    void AddDamage(int defender, int amount, int z);            // 0x00BE90B8, name provisional
    char m_p000[1];
};

// SUPPORT's VIGameOptions (vi.h); CLIENT reads it through this accessor too.
VIGameOptions *VIGetGameOptions();

// The 32-byte element of the VIPool at VIClient+0x2BD58 (a placeholder: eqtmpl.py measured
// its size; Reset sends message 1572 with its first word before erasing each one).
struct VIRec32_BE2DE8 { int m_data[8]; };

// Two CLIENT objects the constructor builds as members (its roll-call), not yet known.
class VIClientPart00190 { public: char m_p000[1]; };
class VIClientPart06D50 { public: char m_p000[1]; };

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
    char m_p10[0x19 - 0x10];
    unsigned char m_p19;                              // +0x19: a byte the client reads for itself and its target
    char m_p1A[0x24 - 0x1A];
    int  m_actor;                                     // +0x24: the scene actor index
    char m_p28[0x54 - 0x28];
    virtual ~VIEntity();                              // slot 1: Disconnect's delete
    virtual void Virtual2();                          // slots 2..9: names unknown
    virtual int  Virtual3(int what);                  // slot 3: Msg1003Done asks it about 130
    virtual void Virtual4();
    virtual void Virtual5();
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
    char m_p58[0x2B8 - 0x58];
    int  m_p2B8;                                      // +0x2B8: Msg1003Done sets 1 on every entity but the player
};

class VIClientGuild
{
public:
    void Recv0468(VIMemStream &s);                              // 0x00BF0D38, name provisional
    void UpdateMember(VIMemStream &s);                          // 0x00BF0F50, name provisional

    char                      m_p000[0x288];
    VIMap<int, VIGuildMember> m_members;                        // +0x288: 68-byte nodes
};

class VIClientPart2EF24
{
public:
    void Recv057A(VIMemStream &s);                              // 0x00BEF6D8, name provisional
    void Recv0579(VIMemStream &s);                              // 0x00BEF788, name provisional
};

class VIClientPart33950
{
public:
    void Recv07B0(VIMemStream &s);                              // 0x00BE71B0, name provisional
};

class VIClientPart382E8
{
public:
    void Recv0700(VIMemStream &s);                              // 0x00BB5DD0, name provisional
};

class VIClientPart38300
{
public:
    void Recv(int opcode, VIMemStream &s);                      // 0x00BFA850, name provisional
};

class VIClientPart38334
{
public:
    void Recv0E02(VIMemStream &s);                              // 0x00C03168, name provisional
};

class VIClientPart3834C
{
public:
    void Recv1102(VIMemStream &s);                              // 0x00BFE118, name provisional
};

class VIClientPart3FC10
{
public:
    void Recv0F02(VIMemStream &s);                              // 0x00BEB900, name provisional
};

class VIBankControl
{
public:
    void Close();                                               // 0x00BB8C28, name provisional
    void Recv(int opcode, VIMemStream &s);                      // 0x00BB9CF0, name provisional
    void Recv1206(VIMemStream &s);                              // 0x00BB9D90, name provisional
};

class VIClientPart4EC58
{
public:
    void Recv1402(VIMemStream &s);                              // 0x00BF7240, name provisional
};

class VIClientPart4F078
{
public:
    void Recv07E2(VIMemStream &s);                              // 0x00BEA820, name provisional
};

class VIClientPart2EFD0
{
public:
    void Recv0D41(VIMemStream &s);                              // 0x00BF5670, name provisional
    void Reset();                                               // 0x00BF57A0, name provisional
};

// VIClient's base, RTTI "9VIClientI".  It has no data, so the vptr is at +0;
// the real vtable (0x00CDB4C0) has 18 virtuals, the destructor in slot 2 and
// one-line sub-object accessors in slots 3-18.  Declared only as far as the
// written functions need.
class VIClientI
{
public:
    virtual ~VIClientI();
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
    int  Init(const char *, char *, int, char *, int, int, int, int);  // 0x00BBFE80, 536
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
    VIClientPart00190 *GetP00190();                   // 0x00BE3958
    VIWalkCamera *GetWalkCamera();                    // 0x00BE3960
    VIDictionary *GetDictionary();                    // 0x00BE3968
    VIClientPart06D50 *GetP06D50();                   // 0x00BE3970
    void *GetActiveWindow();                          // 0x00BE3978
    long GetP07568();                                 // 0x00BE3990
    int  GetP06DC0();                                 // 0x00BE3998
    int  GetP06DC4();                                 // 0x00BE39A0
    unsigned int GetSelfId();                         // 0x00BE39A8
    VIPlayer *GetPlayer();                            // 0x00BE39B0
    int  IsUnderLiquid();                             // 0x00BE39C0
    unsigned int GetTargetId();                       // 0x00BE39E0
    int  GetEffectCount();                            // 0x00BE39F0
    unsigned int GetEffectIcon(int i);                // 0x00BE3A08
    unsigned short *GetEffectName(int i);             // 0x00BE3A28
    int  GetTargetInfoValid();                        // 0x00BE3A40
    int  GetTargetDisposition();                      // 0x00BE3A58
    int  GetP2BB70();                                 // 0x00BE3A70

    // The network opcode dispatcher (DRDP message type 0xFB), name provisional.
    void ProcessMessage(long src, const void *data, int size);  // 0x00BD0DC8

    // Its handlers (provisional names; MsgXXXX handles opcode XXXX).
    void Msg000D(int src, VIMemStream &s);                              // 0x00BD0630: 0x000D: world entry (DSP sections, memory dump)
    void Msg0A7B(VIMemStream &s);                                       // 0x00BD3600
    void Msg0A7A(VIMemStream &s);                                       // 0x00BD36D0
    void Msg0772(VIMemStream &s);                                       // 0x00BD31C0
    void Msg0773(VIMemStream &s);                                       // 0x00BD3458
    void Msg0774(VIMemStream &s);                                       // 0x00BD3548
    int  InGroup();                                                     // 0x00BD7A68: returns +0x2BD74
    void GroupChanged(int);                                             // 0x00BD9B48: the group-reset tail's call when +0x382E4 == 2
    int  PlaySound(int id);                                             // 0x00BD8E78: m_ui sound (returns SoundPlay's result)
    void Msg0034(VIMemStream &s);                                       // 0x00BDBD58
    void ShowDialog(int id, const unsigned short *text, int, int);      // 0x00BDBC70: 0x0046
    void CharacterList(VIMemStream &s);                                 // 0x00BD2E80: 0x002C
    void Msg0777();                                                     // 0x00BD89F0
    void AddInvItem(VINetItem &item);                                   // 0x00BCB0D8: 0x003B
    void Msg003C(int a, int c, int b);                                  // 0x00BCB138
    void Msg003D(int a, int b);                                         // 0x00BCB208
    void Msg003E(int a, int b);                                         // 0x00BCB228
    void Msg003F(int a, int b);                                         // 0x00BCB2F0
    void Msg0040(int a);                                                // 0x00BCB8A0
    void Msg0019(int a, int b);                                         // 0x00BD99A0
    void Msg001A();                                                     // 0x00BD98B0
    void LootNoWindow();                                                // 0x00BCB098: 0x0018's else arm
    void CastingCombat(int a, int b, int n, int *targets, int c, int d);// 0x00BD3738: 0x00B1
    void Msg00B3(VIMemStream &s);                                       // 0x00BCF3A0
    void Msg00B4(int a, int b);                                         // 0x00BCF0F8
    void Msg00FF(VIMemStream &s);                                       // 0x00BCF2D0
    void Msg00B5(int a, int b);                                         // 0x00BCF5A8
    void Msg00B6(int a, int b);                                         // 0x00BCF730
    void Msg00FD(VIMemStream &s);                                       // 0x00BCF8B0
    void Msg00B8(int z);                                                // 0x00BCF468
    void Msg00CD(int a, int z);                                         // 0x00BCF500
    void Msg00B2(VIMemStream &s);                                       // 0x00BD38C8
    void MerchantOpen();                                                // 0x00BD9A48: 0x00B7
    void Msg076F(VIMemStream &s);                                       // 0x00BDB3E8
    void Msg0770(const char *name);                                     // 0x00BDB650
    void Msg0771(int a, const char *name);                              // 0x00BDB6B8
    void Msg07E0(VIMemStream &s);                                       // 0x00BDC308
    void CharSelectMsg(int id);                                         // 0x00BD30E8: 0x07F0-0x07F4
    void Msg07A1(VIMemStream &s);                                       // 0x00BD3158: reads 8 bytes, Disconnect(13, 0)
    void Msg07A2(VIMemStream &s);                                       // 0x00BD31A0: Disconnect(14, 0)
    void QuestStage(unsigned char flag, int delta);                                  // 0x00BD9AA8: 0x0036
    void Msg001D(int z);                                                // 0x00BDCAF0
    void Msg000A(int a, int b);                                         // 0x00BD9C18
    void Msg0010(VIMemStream &s);                                       // 0x00BD9D60
    void Msg0776(int a, int b, int c);                                  // 0x00BD9E50
    int  HasInvSpace();                                                 // 0x00BCC368, name provisional (AddInvItem)
    void SetActiveWindow(VIWnd *w);                                     // 0x00BD7618, name provisional (FrontiersForge window.lua)
    void TargetInfo(int type, int disp, unsigned int id);               // 0x00BE4FC0, name provisional (Msg007B)
    void Msg0777Done(int z);                                            // 0x00BC2528, name provisional (Msg0777)
    void TradeName(char *buf);                                          // 0x00BCCCB8, name provisional (Trade20)
    void QuestHook();                                                   // 0x00BCE5D8, name provisional (QuestStage)
    void SendMsg(int op, int a, int b);                                 // 0x00BE4CF0, name provisional (Reset: op 1572)
    void Msg0790Done();                                                 // 0x00BDE050, name provisional (Msg0790)
    void TradeSetup();                                                  // 0x00BCCD58, name provisional (Trade02)
    void TradeReset();                                                  // 0x00BDA168, name provisional (Trade16)
    void StopAutoAttack();                                              // 0x00BD5B60: +0x71E4 = 0 (FrontiersForge)
    void Msg077A(int id);                                               // 0x00BE4B78
    void Msg0057(int a, int b);                                         // 0x00BDA1A8
    void Trade02(int id);                                               // 0x00BDA2C0: 0x0056 sub 2
    void Trade01();                                                     // 0x00BDA300: 0x0056 sub 1
    void Trade20();                                                     // 0x00BDA320: 0x0056 sub 20
    void Trade16();                                                     // 0x00BCCDC8: 0x0056 sub 16
    void Trade17();                                                     // 0x00BDA3B8: 0x0056 sub 17
    void Trade21();                                                     // 0x00BDA418: 0x0056 sub 21
    void Trade22();                                                     // 0x00BDA478: 0x0056 sub 22
    void Trade07();                                                     // 0x00BDA4D8: 0x0056 sub 7
    void Trade06();                                                     // 0x00BDA500: 0x0056 sub 6
    void TradeA(int sub, VIMemStream &s);                               // 0x00BDA528: 0x0056 default, +0x2BB04 set
    void TradeB(int sub, VIMemStream &s);                               // 0x00BDA688: 0x0056 default, +0x2BB04 clear
    void Msg005E();                                                     // 0x00BDAEB8
    void Msg0060(VIInvItem *item, int z);                               // 0x00BCB988
    void Msg0061(int a);                                                // 0x00BDB048
    void Msg0063();                                                     // 0x00BDB0A8
    void SetTime(long a, long b);                                       // 0x00BD0508: 0x0013
    void Msg0045(int z);                                                // 0x00BDB820
    void Msg006E();                                                     // 0x00BDB8F0: an empty function
    void Msg007B(unsigned int id);                                      // 0x00BDBAC8: -1 is lui+ori, so unsigned
    void Msg007C(const unsigned short *text);                           // 0x00BDBB30
    void Msg007D(int z);                                                // 0x00BDBB98
    void SetSpeed(float v);                                             // 0x00BDD540: 0x00F8
    void Msg00D3(const char *name, int c);                              // 0x00BDB0E8
    void Msg1003Done();                                                 // 0x00BC7218: after the 0x1003-0x1005 sub-object call
    void Msg0790(VIMemStream &s);                                       // 0x00BDC080
    void SetWeather(int w, int instant);                                // 0x00BD9F00: 0x00C5 (0) / 0x00C7 (1)
    void Msg00D7();                                                     // 0x00BD9FD8
    void DamageFromSelf(int defender, int amount);                      // 0x00BDC220: 0x00DB
    void DamageFromOther(int attacker, int amount);                     // 0x00BDC288: 0x00DB

    VIUI          m_ui;                                 // +0x00004: VIWnd::mUI points here
    char          m_g_m_p00058[0x00058 - (0x00004 + sizeof(VIUI))];
    int           m_p00058;                             // +0x00058: client state: Disconnect -1; 0x002C: 2..3 -> 3
    bool          m_quit;                               // +0x0005C: IsRunning's xori
    char          m_g_m_p00080[0x00080 - (0x0005C + sizeof(bool))];
    int           m_p00080;                             // +0x00080: Disconnect zeroes it
    char          m_g00084[0x00190 - (0x00084)];
    VIClientPart00190 m_p00190;                         // +0x00190: a CLIENT object (the roll-call)
    char          m_g_m_raster[0x00380 - (0x00190 + sizeof(VIClientPart00190))];
    VIRaster      m_raster;                             // +0x00380: m_ui.m_raster points here
    char          m_g_m_sound[0x04FE0 - (0x00380 + sizeof(VIRaster))];
    VISoundDevice m_sound;                              // +0x04FE0
    char          m_g_m_console[0x06390 - (0x04FE0 + sizeof(VISoundDevice))];
    VIConsole     m_console;                            // +0x06390: Shutdown closes it
    char          m_g_m_dict[0x06618 - (0x06390 + sizeof(VIConsole))];
    VIDictionary  m_dict;                               // +0x06618: m_ui.m_dict points here
    char          m_g_m_worldId[0x06630 - (0x06618 + sizeof(VIDictionary))];
    int           m_worldId;                            // +0x06630: the world; -1 = none (FrontiersForge util.lua)
    char          m_g_m_loader[0x06638 - (0x06634)];
    VILoader      m_loader;                             // +0x06638
    char          m_g_m_p06D50[0x06D50 - (0x06638 + sizeof(VILoader))];
    VIClientPart06D50 m_p06D50;                         // +0x06D50: a CLIENT object (the roll-call)
    char          m_g_m_p06DC0[0x06DC0 - (0x06D50 + sizeof(VIClientPart06D50))];
    int           m_p06DC0;                             // +0x06DC0
    int           m_p06DC4;                             // +0x06DC4
    int           m_p06DC8;                             // +0x06DC8: 0x00FC stores a byte here
    char          m_g06DCC[0x06DD0 - (0x06DCC)];
    VIWalkCamera  m_walkCam;                            // +0x06DD0: a whole VIWalkCamera (FrontiersForge camera.lua)
    char          m_g_m_autoAttack[0x071E4 - (0x06DD0 + sizeof(VIWalkCamera))];
    unsigned char m_autoAttack;                         // +0x071E4: auto-attack on; ClearP071E4 is `sb zero` (FrontiersForge player.lua)
    char          m_g071E5[0x071F8 - (0x071E5)];
    unsigned int  m_selfId;                             // +0x071F8: own entity id (0x0775, 0x00DB); FrontiersForge 0x1FB6928; -1 is lui+ori
    char          m_g071FC[0x07500 - (0x071FC)];
    VIEntity *    m_entities[24];                       // +0x07500: Disconnect deletes them; FindEntity
    drdp_t *      m_drdp;                               // +0x07560
    connection_t * m_conn;                              // +0x07564
    long          m_p07568;                             // +0x07568: 64-bit (ld)
    long          m_p07570;                             // +0x07570: 64-bit (sd of li -1)
    int           m_p07578;                             // +0x07578: 0x07B2; the ctor sets 100
    char          m_g0757C[0x07590 - (0x0757C)];
    VIScene       m_scene;                              // +0x07590
    unsigned int  m_target;                             // +0x0C140: the target's entity id, -1 = none (lui+ori; FrontiersForge)
    char          m_g0C144[0x0C188 - (0x0C144)];
    int           m_pC188;                              // +0x0C188: StopMusic zeroes it
    int           m_pC18C;                              // +0x0C18C: StopMusic stores -1 (li: a signed int)
    unsigned char m_pC190;                              // +0x0C190: StopMusic zeroes it
    char          m_g0C191[0x0C1B0 - (0x0C191)];
    int           m_pC1B0;                              // +0x0C1B0: Notify zeroes it (FrontiersForge: "dead")
    char          m_g_m_player[0x0C2DC - (0x0C1B4)];
    VIPlayer      m_player;                             // +0x0C2DC: the player's own record (FrontiersForge)
    VIInvList     m_inventory;                          // +0x0C428: 0x0060
    char          m_g_m_p13B90[0x13B90 - (0x0C428 + sizeof(VIInvList))];
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
    char          m_g22B7C[0x2BAEC - (0x22B7C)];
    int           m_p2BAEC;                             // +0x2BAEC
    char          m_g2BAF0[0x2BB00 - (0x2BAF0)];
    unsigned int  m_p2BB00;                             // +0x2BB00: Trade20 compares it with -1 (lui+ori)
    int           m_p2BB04;                             // +0x2BB04: 0x0056's default arm
    int           m_tradePartner;                       // +0x2BB08: 0x0056
    unsigned int  m_p2BB0C;                             // +0x2BB0C: 0x007C / 0x007D; -1 is lui+ori (0x00BC0680)
    unsigned int  m_p2BB10;                             // +0x2BB10: -1 (lui+ori) beside it at 0x00BC0680
    char          m_g2BB14[0x2BB2C - (0x2BB14)];
    VIMap<int, VIAbility> m_abilities;                  // +0x2BB2C: 0x00B3..0x00FF's ability map (FrontiersForge)
    char          m_g_m_p2BB60[0x2BB60 - (0x2BB2C + sizeof(VIMap<int, VIAbility>))];
    int           m_p2BB60;                             // +0x2BB60: 0x00B4/B5/B6/B8/CD
    int           m_targetInfoValid;                    // +0x2BB64: 0x0010 sets 1 (FrontiersForge entity.lua)
    int           m_p2BB68;                             // +0x2BB68
    int           m_targetDisposition;                  // +0x2BB6C: 0..6 (0x0010)
    int           m_p2BB70;                             // +0x2BB70: a reset sets 2
    int           m_p2BB74;                             // +0x2BB74: a reset sets 1
    VIWndGame *   m_wndGame;                            // +0x2BB78: ShowText's window
    char          m_g2BB7C[0x2BB84 - (0x2BB7C)];
    VIWndLogin *  m_wndLogin;                           // +0x2BB84: the character-select message window
    VIWnd2BB88 *  m_wnd2BB88;                           // +0x2BB88: 0x0012
    char          m_g2BB8C[0x2BB98 - (0x2BB8C)];
    VIWnd2BB98    m_p2BB98;                             // +0x2BB98: a window; Disconnect compares +0x2BD40 to it
    void *        m_p2BD40;                             // +0x2BD40
    VIWnd *       m_p2BD44;                             // +0x2BD44: Notify / CharSelectMsg store the window they activate
    char          m_g2BD48[0x2BD58 - (0x2BD48)];
    VIPool<VIRec32_BE2DE8> m_p2BD58;                    // +0x2BD58: Reset drains it; fills the gap to +0x2BD70
    unsigned int  m_p2BD70;                             // +0x2BD70: group reset: -1; unsigned (lui+ori, phase 39 lever)
    int           m_inGroup;                            // +0x2BD74: InGroup returns it
    int           m_p2BD78;                             // +0x2BD78: group reset: 0
    VIPartyMember m_group[4];                           // +0x2BD7C: exactly to +0x2BE4C (FrontiersForge group.lua)
    int           m_p2BE4C;                             // +0x2BE4C: group reset: 0
    unsigned int  m_p2BE50;                             // +0x2BE50: 0x007E / 0x007F (unsigned: its -1 is lui+ori)
    unsigned char m_p2BE54;                             // +0x2BE54: 0x007F
    char          m_g2BE55[0x2BE6C - (0x2BE55)];
    int           m_charCount;                          // +0x2BE6C: 0x002E / 0x0065 / 0x002F decrement it
    char          m_g2BE70[0x2C170 - (0x2BE70)];
    int           m_effectCount;                        // +0x2C170: active effects (state channel 0x43; FrontiersForge)
    unsigned int  m_effectIcon[8];                      // +0x2C174
    unsigned short m_effectName[8][64];                 // +0x2C194
    char          m_g2C594[0x2C6E0 - (0x2C594)];
    unsigned short m_questTitle[8][128];                // +0x2C6E0 (FrontiersForge quest_log.lua)
    int           m_questCount;                         // +0x2CEE0: 0x007C / 0x007D
    float         m_speed;                              // +0x2CEE4: 0x00F8
    char          m_g2CEE8[0x2E7F0 - (0x2CEE8)];
    char          m_p2E7F0;                             // +0x2E7F0: Disconnect zeroes it
    char          m_p2E7F1;                             // +0x2E7F1: Disconnect zeroes it
    char          m_g2E7F2[0x2EBFC - (0x2E7F2)];
    int           m_p2EBFC;                             // +0x2EBFC: Disconnect zeroes it
    char          m_g2EC00[0x2EC30 - (0x2EC00)];
    VIClientClock m_clockA;                             // +0x2EC30: SetTime
    VIClientClock m_clockB;                             // +0x2EC50: SetTime
    VIMap<int, VIBuddy> m_buddies;                      // +0x2EC70: 0x03D5 / 0x03D6
    char          m_g_m_guild[0x2EC84 - (0x2EC70 + sizeof(VIMap<int, VIBuddy>))];
    VIClientGuild m_guild;                              // +0x2EC84: vtable slot 4
    char          m_g_m_p2EF24[0x2EF24 - (0x2EC84 + sizeof(VIClientGuild))];
    VIClientPart2EF24 m_p2EF24;                         // +0x2EF24: slot 5
    char          m_g_m_p2EFD0[0x2EFD0 - (0x2EF24 + sizeof(VIClientPart2EF24))];
    VIClientPart2EFD0 m_p2EFD0;                         // +0x2EFD0: slot 7
    char          m_g_m_p33950[0x33950 - (0x2EFD0 + sizeof(VIClientPart2EFD0))];
    VIClientPart33950 m_p33950;                         // +0x33950: slot 8
    char          m_g_m_p382E4[0x382E4 - (0x33950 + sizeof(VIClientPart33950))];
    unsigned char m_p382E4;                             // +0x382E4: group reset: == 2 -> GroupChanged(0)
    char          m_g382E5[0x382E8 - (0x382E5)];
    VIClientPart382E8 m_p382E8;                         // +0x382E8: slot 10
    char          m_g_m_p38300[0x38300 - (0x382E8 + sizeof(VIClientPart382E8))];
    VIClientPart38300 m_p38300;                         // +0x38300: slot 9
    char          m_g_m_p38334[0x38334 - (0x38300 + sizeof(VIClientPart38300))];
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
    void *        m_p4F0B0;                             // +0x4F0B0: FreeBuffer frees it
    int           m_p4F0B4;                             // +0x4F0B4: FreeBuffer zeroes it
    unsigned short m_p4F0B8[24];                        // +0x4F0B8: 0x07A3 (str8to16)
    int           m_p4F0E8;                             // +0x4F0E8: Disconnect's reason; 0x002C sets 21
    int           m_p4F0EC;                             // +0x4F0EC: Shutdown stores -1
    long          m_p4F0F0;                             // +0x4F0F0: 0x0790 reads 8 bytes
    int           m_p4F0F8;                             // +0x4F0F8: 0x0790 reads 4
    long          m_p4F100;                             // +0x4F100: 0x0790 reads 8
    long          m_p4F108;                             // +0x4F108: 0x0790 stores -1 (sd)
    char          m_g4F110[0x4F12C - (0x4F110)];
    int           m_exitCode;                           // +0x4F12C: client_main returns it
    int           m_p4F130;                             // +0x4F130: Disconnect tests it
    char          m_g4F134[0x4FD28 - (0x4F134)];
    VIClientPart4FD28 m_p4FD28;                         // +0x4FD28: DamageFromSelf
    char          m_g_m_p50304[0x50304 - (0x4FD28 + sizeof(VIClientPart4FD28))];
    int           m_p50304;                             // +0x50304: 0x07D1
    char          m_g50308[0x50330 - 0x50308];
};

#endif
