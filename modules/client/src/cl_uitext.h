// cl_uitext.h -- UI helpers before the windows: CLIENT's VIFont copy, the item/description text builders, VITextSet, CLIENT's VIStringTable.cpp copy, a VIVector<VIRec36> list widget.
// CLIENT 0x00C04D10..0x00C0D000.
#ifndef CL_UITEXT_H
#define CL_UITEXT_H

// A WORDS.TXT id and the game id it names: the lookup tables of FindStatWord (0x00CE0188),
// FindWearSlotWord (0x00CDFA50).  8 bytes.  Names provisional.
// VIWordId is declared in cl_parts2.h (included first).
VIWordId *FindStatWord(int stat);       // 0x00C0B010: the stat's abbreviation ("Str" ...)
VIWordId *FindWearSlotWord(int slot);   // 0x00C04D10: the wear slot's name ("HEAD" ...)
int      *GetSexWord(int sex);          // 0x00C053F8 (vifont.cpp): 163 "Male" / 164 "Female", out of range Male
VIWordId *FindAttackWord(int type);         // 0x00C03A48 (lane cl-parts2): an attack type's name (ATT)
VIWordId *FindTargetWord(int type);     // 0x00C0B880: a spell target's name (SELF/TARGET/GROUP/PET/CORPSE)

// vimenudefs.cpp: two menu definitions VIWndGame reads (0x00CD02A0 .. in .data); the types
// are lane cl-wl's (cl_wl.h): VIMenuDef {m_x00, m_x04, VIMenuEntry *m_entries, m_count}.
class VIMenuDef;
struct VIMenuEntry;
extern VIMenuDef   gStartMenu;        // 0x00CD02A0
extern VIMenuDef   gEquipMenu;        // 0x00CD02F0: the backpack
extern VIMenuEntry gStartItemsBank[];        // 0x00CD0138: gStartMenu's 11 entries (not transcribed)
extern int         gStartCountBank;          // 0x00CD0298: = 11, their count
extern VIMenuEntry gEquipItems[];        // 0x00CD2E90: the backpack's 40 (zero; filled at run time)


// ---- Pre-declared by the coordinator before the lanes started (2026-10-16), because the
// ---- window lanes EMBED these by value (11, 20 and 4 windows).  cl-uitext writes their
// ---- code; keep the class and method names (every window lane calls them), fill in fields.

// 0x00C05418..: a window's item-description builder, 4 bytes: the client.  Its methods
// (0x00C05460 ..) read m_client->GetInvCount()/GetInvItemName()/... and snprintf16 the
// wide description of an inventory/loot/merchant/bank/trade item.  VIWndAuction embeds
// one at +0x38.  Name provisional.
class VIInfoText
{
public:
    VIInfoText();                       // 0x00C05418: m_client = 0
    ~VIInfoText();                      // 0x00C05428: empty, out of line
    int SetClient(VIClient *c);         // 0x00C05450: returns 0
    // The one-line helpers: a word, a colon, the value(s), appended to dst (names = words).
    void AddLevel(unsigned short *dst, unsigned int size, int v);          // 0x00C094E8 "Level"
    void AddRefire(unsigned short *dst, unsigned int size, int v);         // 0x00C09570 "Refire"
    void AddWeight(unsigned short *dst, unsigned int size, int v);         // 0x00C095F8 "Weight"
    void AddEquipSlot(unsigned short *dst, unsigned int size, int slot);   // 0x00C09680 "Equip"
    void AddAttackType(unsigned short *dst, unsigned int size, int type);  // 0x00C09778 "ATT"
    void AddRange(unsigned short *dst, unsigned int size, int v);          // 0x00C09870 "RNG"
    void AddCharges(unsigned short *dst, unsigned int size, int v);        // 0x00C098F8 "Charges"
    void AddHitPoints(unsigned short *dst, unsigned int size, int v, int max);  // 0x00C09980 "HP" a/b
    void AddHp(unsigned short *dst, unsigned int size, int v);             // 0x00C09A18 "HP"
    void AddDamage(unsigned short *dst, unsigned int size, int v);         // 0x00C09AA0 "DMG"
    void AddDur(unsigned short *dst, unsigned int size, int v);            // 0x00C09B28 "DUR"
    void AddStack(unsigned short *dst, unsigned int size, int v, int max); // 0x00C09BB0 "Stack" a/b
    void AddMaxStack(unsigned short *dst, unsigned int size, int v);       // 0x00C09C48 "Max Stack"
    void AddFood(unsigned short *dst, unsigned int size, int v);           // 0x00C09CD0 "Food"
    void AddRaceMask(unsigned short *dst, unsigned int size, int mask);    // 0x00C0A060 "Race: ..."
    void AddClassMask(unsigned short *dst, unsigned int size, int mask);   // 0x00C0A1E0 "Class: ..."
    // The inventory's describers (0x00C05460 ..): item i, through the client's accessors.
    int  DescribeInvUsable(unsigned short *dst, unsigned int size, int i);         // 0x00C05460
    int  DescribeInvEquipment(unsigned short *dst, unsigned int size, int i);      // 0x00C057A8: DescribeInvItem(.., 1)
    int  DescribeInvStats(unsigned short *dst, unsigned int size, int i);          // 0x00C05828
    int  DescribeInvMisc(unsigned short *dst, unsigned int size, int i);           // 0x00C05950
    int  DescribeInvCharged(unsigned short *dst, unsigned int size, int i);        // 0x00C05CC8
    int  DescribeInvChargedShort(unsigned short *dst, unsigned int size, int i);   // 0x00C05F88
    int  DescribeInvConsumable(unsigned short *dst, unsigned int size, int i);     // 0x00C06110
    int  DescribeInvEquipmentShort(unsigned short *dst, unsigned int size, int i); // 0x00C06350
    int  DescribeInvFood(unsigned short *dst, unsigned int size, int i);           // 0x00C06628
    int  DescribeListItem(unsigned short *dst, unsigned int size, int i, int list);    // 0x00C067A8: 0 inventory, 1 merchant
    int  DescribeBankListItem(unsigned short *dst, unsigned int size, int i, int list); // 0x00C06868: 0 inventory, 2 bank
    int  DescribeInvItemShort(unsigned short *dst, unsigned int size, int i);      // 0x00C06920
    int  DescribeLootItem(unsigned short *dst, unsigned int size, int i);          // 0x00C069B8
    int  DescribeItem(unsigned short *dst, unsigned int size, VIItem *item);       // 0x00C06C50
    int  DescribeTradeItem(unsigned short *dst, unsigned int size, int i, int list);   // 0x00C06E70: 4 mine, 5/6 the offers
    int  DescribeMerchantItem(unsigned short *dst, unsigned int size, int i);      // 0x00C06F70
    int  DescribeInvItem(unsigned short *dst, unsigned int size, int i, int flags);    // 0x00C073F8: flags & 1 = in full
    int  DescribeInvItemQuick(unsigned short *dst, unsigned int size, int i);      // 0x00C07930
    int  DescribeBankItem(unsigned short *dst, unsigned int size, int i);          // 0x00C079B0
    int  DescribeTradeInvItem(unsigned short *dst, unsigned int size, int i);      // 0x00C07D90
    int  DescribeTradeOffer(unsigned short *dst, unsigned int size, int i, int side);  // 0x00C080F0
    int  DescribeRecipe(unsigned short *dst, unsigned int size);                   // 0x00C08498: the recipe being made
    // The auction list's describers (0x00C08970 ..).
    int  DescribeAuctionItem(unsigned short *dst, unsigned int size, VIAuctionItem *item);       // 0x00C08970
    int  DescribeAuctionEquipment(unsigned short *dst, unsigned int size, VIAuctionItem *item);  // 0x00C08A70
    int  DescribeAuctionMisc(unsigned short *dst, unsigned int size, VIAuctionItem *item);       // 0x00C08E20
    int  DescribeAuctionUsable(unsigned short *dst, unsigned int size, VIAuctionItem *item);     // 0x00C09090
    int  DescribeAuctionCharged(unsigned short *dst, unsigned int size, VIAuctionItem *item);    // 0x00C092F8
    void AddStats(unsigned short *dst, unsigned int size, int index, int source, int all, VIAuctionItem *item);  // 0x00C09D58: source -1/0 inventory, 1 merchant, 2 bank, 7 recipe, 8 item
    // What kind of item (0x00C0A360 ..).
    int  IsCharged(VIItem *item);      // 0x00C0A360
    int  IsWearable(VIItem *item);     // 0x00C0A390: a wear slot
    int  IsMisc(VIItem *item);         // 0x00C0A3A0
    int  IsUsable(VIItem *item);       // 0x00C0A3F0: types 2, 5, 7, 8
    int  IsRanged(VIItem *item);       // 0x00C0A428: attack types 7..10
    VIClient *m_client;                 // +0x00
};

// 0x00C0B048..: the pressed-button flash a window keeps for its pad keys: a count and 16
// slots of 3 bytes.  Press puts {16, 0, key} in the key's slot or the first free one;
// Release clears the key; Pulse counts a slot down.  52 bytes.  Names provisional.
struct VIKeyFlashSlot
{
    unsigned char m_time;               // +0: 16 when pressed; the next repeat interval
    unsigned char m_wait;               // +1: frames until the next repeat
    unsigned char m_key;                // +2: 0 = free
};
class VIKeyFlash
{
public:
    VIKeyFlash();                       // 0x00C0B048: Reset()
    ~VIKeyFlash();                      // 0x00C0B070: empty, out of line
    void Reset();                       // 0x00C0B098: m_count = 0, every m_key = 0
    void Press(unsigned char key);      // 0x00C0B0D0
    void Release(unsigned char key);    // 0x00C0B160
    int  Pulse(unsigned char key);      // 0x00C0B1B8
    int            m_count;             // +0x00
    VIKeyFlashSlot m_slots[16];         // +0x04
};

// One line of a VITextList, 36 bytes (eqtmpl.py measured it as VIRec36_C0CDC0): up to four
// columns, each a new[]'d wide string at its own x.  AddLine makes a one-column line.
struct VITextLine
{
    int             m_cols;             // +0x00: columns in use
    int             m_x[4];             // +0x04: each column's x (SetColumns)
    unsigned short *m_text[4];          // +0x14: each column's text, new[]'d
};

// 0x00C0C398..0x00C0D000: a scrolling, selectable list of text lines, 36 bytes.
// VIWndClassMastery embeds two (+432, +468), ClassMasteryInspect and ClassMasteryRequire
// one each.  Name provisional; method names ours.
class VITextList
{
public:
    VITextList();                                       // 0x00C0C398: m_lines.Init(0)
    ~VITextList();                                      // 0x00C0C3E0
    int  Init(VIUI *ui, int width, int rows);           // 0x00C0C430: Free, then the geometry
    void Free();                                        // 0x00C0C490: every line's text, then the lines
    int  Count();                                       // 0x00C0C578: m_lines.m_size
    void Clear();                                       // 0x00C0C580: Free()
    int  GetTop();                                      // 0x00C0C5A0
    void Nop();                                         // 0x00C0C5A8: empty
    int  AddLine(const unsigned short *text);           // 0x00C0C5B0: one column; Add's result
    int  GetShowSel();                                  // 0x00C0C638
    void SetShowSel(int on);                            // 0x00C0C640
    int  AddText(const unsigned short *text);           // 0x00C0C648: wrapped at '\n' and m_width - 24
    void SetColumns(int line, int n, const int *x);     // 0x00C0C838
    void SetText(int line, int col, const unsigned short *s);  // 0x00C0C880
    int  GetSel();                                      // 0x00C0C908
    void SetSel(int i);                                 // 0x00C0C910: clamped to the lines
    int  Scroll(int d);                                 // 0x00C0C940: 1 when already at that end
    void Draw(VIUIWindow *w, int x, int y);             // 0x00C0C9B0

    VIUI                *m_ui;                          // +0x00: its font measures the wrap
    int                  m_width;                       // +0x04
    int                  m_rows;                        // +0x08: lines shown
    int                  m_top;                         // +0x0C: the first line shown
    int                  m_sel;                         // +0x10: -1 = none
    int                  m_showSel;                     // +0x14: Init sets 1; the selection is ticked
    VIVector<VITextLine> m_lines;                       // +0x18
};

#endif
