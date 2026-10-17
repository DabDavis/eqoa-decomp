// cl_parts2.h -- more VIClient sub-objects: VIClientPart2EFD0, 4EC58, 38300, VIShardInfo.cpp, 3834C (Recv1102), 38334, ....
// CLIENT 0x00BF5628..0x00C04D10.
#ifndef CL_PARTS2_H
#define CL_PARTS2_H

// A 632-byte record that VIMaskIndex files by two bit masks (the pool's
// element is a POINTER to one; the placeholder name is client_vipool.cpp's).  Only the
// masks are measured.
struct VIPtr_BFA1F0
{
    char         m_p00[0x20];
    unsigned int m_mask20;                          // +0x20: bit j = column j of the grid
    unsigned int m_mask24;                          // +0x24: bit k = row k
    char         m_p28[632 - 0x28];
};

// 0x00BF9CA8 .. 0x00BFA448: a 10 x 15 grid of pools of record pointers, filled by
// Build from the two masks of each record.  Nothing in CLIENT calls it (dead code, kept
// by the linker).  3,604 bytes.  Names provisional.
class VIMaskIndex
{
public:
    VIMaskIndex(void *owner);                                   // 0x00BF9CA8
    ~VIMaskIndex();                                             // 0x00BF9D48
    int Build(VIPtr_BFA1F0 *recs, int n);                       // 0x00BF9DD0
    VIPool<VIPtr_BFA1F0 *> *Get(unsigned int j, int k);         // 0x00BF9F30: 0 out of range
    void                  *m_owner;                 // +0x000
    VIPool<VIPtr_BFA1F0 *> m_pools[10][15];         // +0x004
};

// 0x00C03A10 .. 0x00C03AB0: three word lookups over {WORDS.TXT id, game id} tables (the
// same record as cl-uitext's VIWordId; this placeholder until the merge).  The last match
// wins.  Names provisional.
struct VIWordId
{
    int m_word;                         // +0x00: the WORDS.TXT id
    int m_id;                           // +0x04: the stat / wear slot / attack type
};
extern VIWordId gWords_CDF428[4];              // 0x00CDF428: words 302, 498..500
extern VIWordId gAttackWords[11];              // 0x00CDF6A0: the attack types (ATT)
extern VIWordId gEquipSlotWords[23];           // 0x00CDF6F8: the equip slots, -1 first
VIWordId *FindWord_CDF428(int id);             // 0x00C03A10 (no caller)
VIWordId *FindAttackWord(int id);              // 0x00C03A48 (cl-uitext: FindAttackWord)
VIWordId *FindEquipSlotWord(int id);           // 0x00C03A80 (cl-ws: FindEquipSlotWord)

// The chat text VIWndChat embeds at +0x1A8 (cl-wc: C_00C03AB8), 5,136 bytes: a ring of 32
// lines, each kept on screen for 20 s of VIClient's clock (+0x7568).  AddText word-wraps
// into the ring at m_width pixels.  Names provisional.
struct VIChatLine
{
    unsigned short m_text[64];                      // +0x00
    VIColor32F     m_color;                         // +0x80
    int            m_type;                          // +0x90: 1 shown, 0 once aged out
    long           m_time;                          // +0x98: VIClient::m_p07568 when added
};
class VIChatText
{
public:
    VIChatText();                                   // 0x00C03AB8
    ~VIChatText();                                  // 0x00C03B18
    int  Init(VIClient *app, int width);            // 0x00C03B60: m_width = width - 30
    int  Shutdown();                                // 0x00C03B90
    void Reset();                                   // 0x00C03BB8
    void Age();                                     // 0x00C03C28
    int  First();                                   // 0x00C03C78
    int  End();                                     // 0x00C03C80
    int  Next(int i);                               // 0x00C03C88: mod 32
    int  Prev(int i);                               // 0x00C03CA0
    int  Count();                                   // 0x00C03CB0
    int  Index(int i);                              // 0x00C03CD8: i's distance from First
    int  AddText(const char *s, VIColor32F c);      // 0x00C03CF8
    int  AddText(const unsigned short *s, VIColor32F c);  // 0x00C03D68
    int  AddLine(VIColor32F c);                     // 0x00C04020: commits the tail line
    VIChatLine *Line(unsigned int i);               // 0x00C04110: 0 past 31

    int        m_head;                              // +0x00
    int        m_tail;                              // +0x04
    int        m_width;                             // +0x08
    VIClient  *m_app;                               // +0x0C
    VIChatLine m_lines[32];                         // +0x10
};

#endif
