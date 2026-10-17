// stwndcs.h -- the customer-service windows: VIWndCSInbox
// (0x002EADA0 .. 0x002ECF00), VIWndCSRequest (0x002ECF00 .. 0x002F0128),
// VIWndReadCS (0x002F2C38 .. 0x002F4EB8).  VICSInterface embeds a request and an
// inbox (+0x4AC0, +0x54C0); the inbox news a VIWndReadCS, which embeds a
// VIWndMessage and a VIWndCSRequest of its own (the reply).
//
// CLIENT links the same three classes (ctor/dtor/Init/Free are byte-identical:
// inbox Free 0x00C3C850, request 0x00C3E860.., read 0x00C87820..), but every
// function that reaches the program differs: CLIENT asks its VIClient through a
// virtual (slot 8) for the CS client and reads the clock off it, where STATION
// goes through VIStation::m_renderer and VITimer.  So the TUs are not shared.
//
// Every window's app pointer is the VIStation.  The names are ours: nothing in
// either module names a method of these classes.
#ifndef STWNDCS_H
#define STWNDCS_H

#include "stwnd.h"         // VIWndMBox, VIWndMessage, VIWndOptionList
#include "vidatetime.h"    // VIDate

// The customer-service inbox: the message list (one row per header, newest
// first, eleven visible) over a status window that says what the CS client is
// doing.  0x358 bytes; VICSInterface embeds one at +0x54C0.
class VIWndCSInbox : public VIWnd
{
public:
    VIWndCSInbox();                                              // 0x002EADA0
    virtual ~VIWndCSInbox();                                     // 0x002EADF8

    int  Init();                                                 // 0x002EAE58
    int  Free();                                                 // 0x002EAF48

    virtual void OnDrawSelf();                                   // 0x002EAFE0
    virtual void OnKeyDown(int key);                             // 0x002EBA98
    virtual void OnKeyUp(int key);                               // 0x002EBC10
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002EBC50

    void Refresh();                                              // 0x002EBDE8
    void End();                                                  // 0x002EBE50  empty
    void Next();                                                 // 0x002EBE58
    void Prev();                                                 // 0x002EBEE8
    void NextColumn();                                           // 0x002EBF58
    void PrevColumn();                                           // 0x002EBFB0
    void Select();                                               // 0x002EC008
    void Read();                                                 // 0x002EC0F8
    int  FindCategory(int category, int subcategory,
                      unsigned short *catName, unsigned short *subName);   // 0x002EC1C8

    VIUIWindow    m_window;    // +0x028  the list
    VIUIWindow    m_status;    // +0x1A8
    VIWndMBox    *m_mbox;      // +0x328
    VIWndReadCS  *m_read;      // +0x32C
    int           m_top;       // +0x330  first row shown
    int           m_sel;       // +0x334
    int           m_column;    // +0x338  0 or 1: which header column is lit
    int           m_read_;     // +0x33C  open the message once it is in
    int           m_delete;    // +0x340
    int           m_reload;    // +0x344
    int           m_key;       // +0x348  the arrow held down
    float         m_repeat;    // +0x34C  seconds to its next repeat
    long          m_time;      // +0x350  OnDrawSelf's clock
};
CHK(c_VIWndCSInbox, sizeof(VIWndCSInbox) == 0x358)

// A request to customer service (or, with m_reply set, the reply to one):
// a category, a subcategory and the text, each picked in turn, then Submit.
// 0xA00 bytes; VICSInterface embeds one at +0x4AC0 and VIWndReadCS at +0x7A8.
class VIWndCSRequest : public VIWnd
{
public:
    VIWndCSRequest();                                            // 0x002ECF00
    virtual ~VIWndCSRequest();                                   // 0x002ECF80

    int  Init(int reply);                                        // 0x002ED008
    int  Free();                                                 // 0x002ED138

    virtual void OnDrawSelf();                                   // 0x002ED1F0
    virtual void OnKeyDown(int key);                             // 0x002EE098
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002EE490

    void Refresh();                                              // 0x002EE798
    void End();                                                  // 0x002EE808  empty
    void Next();                                                 // 0x002EE810
    void Prev();                                                 // 0x002EE880
    void Select();                                               // 0x002EE8F8
    void TakeText();                                             // 0x002EEB90
    void Cancel();                                               // 0x002EEBE8
    void PickCategory();                                         // 0x002EEC78
    void PickSubcategory();                                      // 0x002EEDF0
    void SetCategory(const unsigned short *s);                   // 0x002EEF48
    void SetSubcategory(const unsigned short *s);                // 0x002EEF70
    void SetText(const unsigned short *s);                       // 0x002EEF98
    int  SetReplyTo(int id);                                     // 0x002EEFC0
    void CheckFull();                                            // 0x002EEFE8
    VICSCategory    *FindCategory(int id);                       // 0x002EF098
    VICSSubcategory *FindSubcategory(int category, int id);      // 0x002EF170
    void ShowMessage(const unsigned short *text, int prompt,
                     int a, int b);                              // 0x002EF258

    // Inline: their copies follow __tf14VIWndCSRequest (0x002F0110, 0x002F0118).
    void SetCategoryId(int id) { m_categoryId = id; }
    void SetCategoryIds(int id, int sub) { m_categoryId = id; m_subcategoryId = sub; }

    VIUIWindow          m_window;          // +0x028
    VIUIWindow          m_list;            // +0x1A8  the category/subcategory picker
    VIUIWindow          m_status;          // +0x328
    VIWndEdit          *m_edit;            // +0x4A8
    VIWndMBox          *m_mbox;            // +0x4AC
    int                 m_sel;             // +0x4B0  0 category, 1 subcategory, 2 text, 3 Submit
    int                 m_catSel;          // +0x4B4
    int                 m_catCount;        // +0x4B8
    int                 m_subSel;          // +0x4BC
    int                 m_subCount;        // +0x4C0
    int                 m_picking;         // +0x4C4  the category list is up
    int                 m_pickingSub;      // +0x4C8  the subcategory list is up
    int                 m_x4CC;            // +0x4CC
    int                 m_x4D0;            // +0x4D0
    int                 m_x4D4;            // +0x4D4
    int                 m_x4D8;            // +0x4D8
    int                 m_reply;           // +0x4DC  Init's argument
    int                 m_replyTo;         // +0x4E0  -1 for a new request
    unsigned short      m_category[64];    // +0x4E4
    unsigned short      m_subcategory[64]; // +0x564
    unsigned short      m_text[512];       // +0x5E4
    int                 m_categoryId;      // +0x9E4
    int                 m_subcategoryId;   // +0x9E8
    VIArray<int>        m_catIds;          // +0x9EC
    VIArray<int>        m_subIds;          // +0x9F4
};
CHK(c_VIWndCSRequest, sizeof(VIWndCSRequest) == 0xA00)

// One row of VIWndReadCS's wrapped text: where it starts in m_full and how many
// characters it takes.  8 bytes; Wrap fills 1024 of them.
struct VIReadCSLine
{
    unsigned short *m_start;   // +0x00  0 past the last row
    short           m_len;     // +0x04
    short           m_break;   // +0x06  1 when a line break or a wrap ends the row
};
CHK(c_VIWndCSRequest, sizeof(VIWndCSRequest) == 0xA00)

// One message read from the inbox: its category, subcategory and date over the
// text, wrapped and scrolled eight rows at a time; Enter offers Reply and
// Delete.  0xB760 bytes (the inbox news one).
class VIWndReadCS : public VIWnd
{
public:
    VIWndReadCS();                                               // 0x002F2C38
    virtual ~VIWndReadCS();                                      // 0x002F2CA8

    int  Init();                                                 // 0x002F2D20
    int  Free();                                                 // 0x002F2E68

    virtual void OnDrawSelf();                                   // 0x002F2F80
    virtual void OnKeyDown(int key);                             // 0x002F38A0
    virtual void OnKeyUp(int key);                               // 0x002F3A88
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002F3AD8

    void Next();                                                 // 0x002F3E68
    void Prev();                                                 // 0x002F3EE0
    const unsigned short *Category();                            // 0x002F3F48
    void SetCategory(const unsigned short *s);                   // 0x002F3F50
    const unsigned short *Subcategory();                         // 0x002F3F90
    void SetSubcategory(const unsigned short *s);                // 0x002F3F98
    const unsigned short *Text();                                // 0x002F3FD8
    void SetText(const unsigned short *s);                       // 0x002F3FE0
    const unsigned short *Date();                                // 0x002F4020
    void SetDate(const unsigned short *s);                       // 0x002F4028
    // Splits text into rows no wider than width (a break at the last space, a
    // CR read as a space) and returns how many; reads at most len - 1 characters.
    int  Wrap(unsigned short *text, VIReadCSLine *lines, int len,
              int maxLines, int width);                          // 0x002F4068

    // Inline: their copies follow __tf11VIWndReadCS (0x002F4EB8 .. 0x002F4EEC),
    // in this order.
    void SetCategoryId(int id) { m_categoryId = id; }
    void SetCategoryIds(int id, int sub) { m_categoryId = id; m_subcategoryId = sub; }
    int  GetId() { return m_id; }
    void SetId(int id) { m_id = id; }

    VIUIWindow          m_window;          // +0x0028
    VIUIWindow          m_status;          // +0x01A8
    VIWndMessage        m_message;         // +0x0328
    VIWndCSRequest      m_request;         // +0x07A8  the reply
    VIWndOptionList    *m_options;         // +0x11A8  Reply / Delete
    unsigned short      m_category[64];    // +0x11AC
    unsigned short      m_subcategory[64]; // +0x122C
    unsigned short      m_date[64];        // +0x12AC
    unsigned short      m_text[8192];      // +0x132C
    int                 m_categoryId;      // +0x532C
    int                 m_subcategoryId;   // +0x5330
    unsigned short      m_full[8704];      // +0x5334  the labels and the text, Wrap's input
    VIReadCSLine        m_lines[1024];     // +0x9734
    int                 m_top;             // +0xB734  first row shown
    int                 m_rows;            // +0xB738
    int                 m_deleting;        // +0xB73C  a delete is under way
    int                 m_id;              // +0xB740  the message's
    int                 m_key;             // +0xB744
    float               m_repeat;          // +0xB748
    long                m_time;            // +0xB750
    int                 m_dirty;           // +0xB758  re-wrap the text
};
CHK(c_VIWndReadCS, sizeof(VIWndReadCS) == 0xB760)

#endif
