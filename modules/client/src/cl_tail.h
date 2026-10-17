// cl_tail.h -- VIFileStream/VIDateTime leftovers, VIJournal.cpp, VIMemStream.cpp leftovers, VINetItem.cpp, VIRandom, VISyncTime.cpp, VISwing, VICreatureEntity.cpp, VIEntity.cpp, client_main.cpp neighbours.
// CLIENT 0x00CB6020..0x00CC8000.
#ifndef CL_TAIL_H
#define CL_TAIL_H

// ---- viplayerrecord.cpp (0x00CB7510 .. 0x00CB7C30): the record TU between
// VIFileStream.cpp and VIJournal.cpp.  Most of it is unreferenced in the image (dead
// code the linker kept); the names are ours.

// Twelve keyed records, known only by the key compare pair each one defines out of line
// (0x00CB7510 .. 0x00CB7688): `m_key < k` and `m_key > k` against a const int, the key
// at +0, +4 or +8.  Unreferenced.  Names provisional (the address of the first one).
struct VIRecKey_CB7510 { int m_p0; int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB7530 { int m_p0; int m_key; bool operator>(const int &k) const; bool operator<(const int &k) const; };
struct VIRecKey_CB7550 { int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB7570 { int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB7590 { int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB75B0 { int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB75D0 { int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB75E8 { int m_p0; int m_p4; int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB7608 { int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB7628 { int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB7648 { int m_p0; int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };
struct VIRecKey_CB7668 { int m_p0; int m_key; bool operator<(const int &k) const; bool operator>(const int &k) const; };

// A 48-byte record streamed by 0x00CB7688/0x00CB7710 (unreferenced): a 32-byte name, two
// zigzag ints and two raw words.  Name provisional.
struct VIRec48_CB7688
{
    char m_name[32];                    // +0x00
    int  m_p20;                         // +0x20: zigzag
    int  m_p24;                         // +0x24: zigzag
    int  m_p28;                         // +0x28: raw
    int  m_p2C;                         // +0x2C: raw
};
VIMemStream &operator<<(VIMemStream &s, const VIRec48_CB7688 &r);   // 0x00CB7688
VIMemStream &operator>>(VIMemStream &s, VIRec48_CB7688 &r);         // 0x00CB7710

// What VIPlayerRecord is made from by 0x00CB77D0/0x00CB77F8 (both unreferenced but for
// the one call): an id at +4, the name at +8, class/race/level at +0x28..+0x30.
// 52 bytes at least.  Name provisional.
struct VIRec52_CB77F8
{
    int  m_p00;
    int  m_id;                          // +0x04
    char m_name[24];                    // +0x08
    int  m_p20;
    int  m_p24;
    int  m_class;                       // +0x28
    int  m_race;                        // +0x2C
    int  m_level;                       // +0x30
};
VIMemStream &operator<<(VIMemStream &s, const VIPlayerRecord &r);    // 0x00CB78E8

// One language's name and description of a definition (an item's, at VIItemDef+0x5C8):
// 648 bytes.  The accessors want the entry whose language is 0.  Names provisional.
struct VIItemText
{
    int            m_p00;
    int            m_language;          // +0x04: 0 = the one shown
    unsigned short m_name[64];          // +0x08
    unsigned short m_desc[256];         // +0x88
};
// The same with a name only: 136 bytes (VIRecDef_CB7AB0+0x124).
struct VIRec136_CB7AB0
{
    int            m_p00;
    int            m_language;          // +0x04
    unsigned short m_name[64];          // +0x08
};
// One stat bonus of an item definition: 12 bytes; VIItem::SetFrom copies (+4, +8) into
// a VIItemStat.  Name provisional.
struct VIItemMod
{
    int m_p00;
    int m_id;                           // +0x04
    int m_value;                        // +0x08
};

// An item definition, 1,496 bytes (VINetItem.cpp's VIItem::SetFrom reads one; its assert
// names the parameter `pItem` and the bonus array `Mods`).  The fields are named after the
// VIItem fields SetFrom copies them into.  Names provisional.
class VIItemDef
{
public:
    const unsigned short *Name();       // 0x00CB7980: the language-0 text's name
    const unsigned short *Desc();       // 0x00CB7A18: ... its description
    int  m_id;                          // +0x000
    int  m_value;                       // +0x004
    int  m_p08;                         // +0x008
    int  m_icon;                        // +0x00C
    int  m_icon2;                       // +0x010
    int  m_p14[2];
    int  m_wearSlot;                    // +0x01C: -1 for the default
    int  m_p18;                         // +0x020
    int  m_rent;                        // +0x024
    int  m_noTrade;                     // +0x028
    int  m_p2C;
    int  m_p24;                         // +0x030
    int  m_attackType;                  // +0x034
    int  m_damage;                      // +0x038
    int  m_range;                       // +0x03C
    int  m_p40[2];
    int  m_levelReq;                    // +0x048
    int  m_p58;                         // +0x04C
    int  m_p4C;                         // +0x050
    int  m_maxStack;                    // +0x054: 999999 for the default
    int  m_p58b[3];
    int  m_classMask;                   // +0x064
    int  m_raceMask;                    // +0x068
    int  m_maxHp;                       // +0x06C
    int  m_durability;                  // +0x070
    int  m_p74[2];
    int  m_lore;                        // +0x07C
    int  m_p54;                         // +0x080
    char m_p084[0x5C8 - 0x84];
    VIArray<VIItemText> m_texts;        // +0x5C8
    VIArray<VIItemMod>  Mods;           // +0x5D0: the assert's name
};
// Two more definitions known only by the same accessors (0x00CB7AB0, 0x00CB7B00/0x00CB7B98).
class VIRecDef_CB7AB0
{
public:
    const unsigned short *Name();       // 0x00CB7AB0
    char m_p000[0x124];
    VIArray<VIRec136_CB7AB0> m_texts;   // +0x124
};
class VIRecDef_CB7B00
{
public:
    const unsigned short *Name();       // 0x00CB7B00
    const unsigned short *Desc();       // 0x00CB7B98
    char m_p000[0x258];
    VIArray<VIItemText> m_texts;        // +0x258
};

// ---- vinetitem.cpp (0x00CBA850 .. 0x00CBB6A8)

// The 132-byte element of the VIArray func_00BEB738 reads (VIClientPart3FC10's Recv0F02):
// a wide string and an int.  Placeholder name kept (client_viarray.cpp's copies carry it).
// VIRec132_BEB670 is cl-parts1's (game.h).
VIMemStream &operator<<(VIMemStream &s, const VIRec132_BEB670 &r);    // 0x00CBA950
VIMemStream &operator>>(VIMemStream &s, VIRec132_BEB670 &r);          // 0x00CBA990


VIMemStream &operator>>(VIMemStream &s, VIItem &item);                // 0x00CBACE8
VIMemStream &operator<<(VIMemStream &s, const VIItem &item);          // 0x00CBAE50
VIMemStream &operator>>(VIMemStream &s, VIItemStat &st);              // 0x00CBAFB8
VIMemStream &operator<<(VIMemStream &s, const VIItemStat &st);        // 0x00CBAFF8
VIMemStream &WriteNetItem(VIMemStream &s, const VINetItem &item);     // 0x00CBB0C8, name provisional (ReadNetItem's twin)

// ---- vishardentry.cpp (0x00CBBA98 .. 0x00CBBDD8; name ours): one server-select entry,
// 88 bytes, the element of VIShardInfo's m_ServerSelectList (its 64-bit address is -1 for
// none).  Placeholder name kept (client_viarray.cpp's copies and lane cl-parts2's
// VIShardInfo.cpp use it).
// VIElem_BFC798 is cl-parts2's (game.h); Address() is declared there.
VIMemStream &operator<<(VIMemStream &s, const VIElem_BFC798 &e);     // 0x00CBBAB8 (unreferenced)
VIMemStream &operator>>(VIMemStream &s, VIElem_BFC798 &e);           // 0x00CBBB40

// ---- viaiextrapolate.cpp (0x00CBCD10 .. 0x00CBDEF8; name ours): moving a creature
// between updates (claude-notes/interpolation-system.md, npc-extrapolation-limiter).
// The scene filter its ground probe uses: it skips the creature's own actor and every
// character sprite (type 5).  Its only virtual is inline, emitted after its __tf.
class VIAIExtrapolateFilter : public VISceneFilter
{
public:
    virtual int FilterActor(VIScene &scene, int actor)      // 0x00CBDEC0
    {
        if (actor == m_actor)
            return 1;
        return scene.GetActorSpriteType(actor) == 5;
    }
};
void ExtrapolateMove(VICreatureEntity *e, VIScene &scene, float dt);              // 0x00CBCD10
void ExtrapolateGround(VICreatureEntity *e, VIVect3 &pos, VIVect3 &hpr, VIScene &scene);  // 0x00CBCE40

// ---- vientity.cpp (0x00CC4608 .. 0x00CC61A8)
unsigned short ZoneName(int world, int row, int col);  // 0x00CC4608: the WORDS.TXT id of the zone at a grid cell (641 none); unsigned short: VIWndGoto hands it to DrawStringEx(unsigned short) with no andi

// ---- viclientutil.cpp (0x00CC61A8 .. 0x00CC68D8) and viexperience.cpp (0x00CC68D8 .. 0x00CC6EB0)
extern VIBBox gBoundsPos, gBoundsVel, gBoundsAccel, gBoundsHpr, gBoundsAVel, gBoundsAAccel;  // 0x00CD7A58.., the 0x40 encoder's ranges
unsigned int ItemIconId(const char *name);        // 0x00CC6350: "...\Item_Icons\%s.tga"
unsigned int FrontiersIconId(const char *name);   // 0x00CC6408: "...\Item_Icons\Frontiers\%s.tga"
unsigned int ExpForLevel(int level);                      // 0x00CC68D8
void WrapAngle(float &a);                         // 0x00CC6920: into [-PI, PI)
int  AnglesValid(VIVect3 &hpr);                   // 0x00CC6A10

#endif
