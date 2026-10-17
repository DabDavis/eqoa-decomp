// cl_wd.h -- windows: VIWndClassMastery, VIWndClassMasteryInspect, VIWndClassMasteryPoints, VIWndClassMasteryRequire, VIWndComposeGram, VIWndCSInbox, VIWndCSRequest.
// CLIENT 0x00C338F8..0x00C41A48.
// TUs (one window class each, ending with its __tf):
//   VIWndClassMastery          0x00C338F8..0x00C35F88
//   VIWndClassMasteryInspect   0x00C35F88..0x00C37B68
//   VIWndClassMasteryPoints    0x00C37B68..0x00C38DA0
//   VIWndClassMasteryRequire   0x00C38DA0..0x00C3A580
//   VIWndComposeGram           0x00C3A580..0x00C3C6A8
//   VIWndCSInbox               0x00C3C6A8..0x00C3E860
//   VIWndCSRequest             0x00C3E860..0x00C41A48
#ifndef CL_WD_H
#ifndef CHK
#define CHK(n, c)  typedef char n[(c) ? 1 : -1];
#endif
#define CL_WD_H

// Foreign functions this lane calls, by address (the coordinator renames them to the
// owner's names after the merge).

// Skeletons (lanes of 2026-10-16): fill each in as its TU is written.
// VIWndClassMastery: ctor 0x00C33928, 584 bytes (`new`), vtable 0x00CE4ED8;
//   slots [0]0x00C35F38 [1]0x00C33988 [2]OnDraw__5VIWnd [3]0x00C33B20 [4]0x00C34548 [5]0x00C34340 [6]0x00C345C0
// The Class Mastery window (claude-notes/cm-wire-format.md): five categories, the skills
// owned and the skills available in two VITextLists side by side, the points over them;
// Enter opens a skill in the inspector (or, not yet known, its requirements), Square the
// points window.  0x248 bytes.  The client is VIClientPart4EC58 (VIClient's slot 14), which
// shows the skill it opens through ShowNode.  Method names ours.
class VIWndClassMastery : public VIWnd
{
public:
    VIWndClassMastery();                                         // 0x00C33928
    virtual ~VIWndClassMastery();                                // 0x00C33988
    int  Init();                                                 // 0x00C33A00
    void Free();                                                 // 0x00C33B18: empty
    virtual void OnDrawSelf();                                   // 0x00C33B20
    virtual void OnKeyDown(int key);                             // 0x00C34340
    virtual void OnKeyUp(int key);                               // 0x00C34548
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C345C0
    void SetPos(const VIVect2I &p);                              // 0x00C34710
    void Refresh();                                              // 0x00C34728
    void SelectCategory(int category);                           // 0x00C34760
    void DrawLists();                                            // 0x00C34C70
    void SetColumn(int column);                                  // 0x00C34D70
    int  Chain(VIList<VICMSkill *> &in, VIList<VICMSkill *> &out);  // 0x00C34DC0: one rank chain, root first
    int  ShowNode(VICMNode *node);                               // 0x00C34F48: <0 when not up

    int                       m_category;   // +0x024: 0..4 (sCategoryWords)
    VIVect2I                  m_pos;        // +0x028
    VIUIWindow                m_window;     // +0x030
    VITextList                m_owned;      // +0x1B0
    VITextList                m_available;  // +0x1D4
    VIKeyFlash                m_keys;       // +0x1F8
    int                       m_column;     // +0x22C: 0 owned, 1 available
    int                       m_busy;       // +0x230: waiting for the client
    char                      m_dots;       // +0x234
    char                      m_ticks;      // +0x235
    VIWndClassMasteryPoints  *m_points;     // +0x238
    VIWndClassMasteryInspect *m_inspect;    // +0x23C
    VIWndClassMasteryRequire *m_require;    // +0x240
    int                       m_x244;       // +0x244
};
CHK(c_VIWndClassMastery, sizeof(VIWndClassMastery) == 0x248)

// VIWndClassMasteryInspect: ctor 0x00C35F88, 1712 bytes (`new`), vtable 0x00CE5430;
//   slots [0]0x00C37B18 [1]0x00C35FE0 [2]OnDraw__5VIWnd [3]0x00C366D8 [4]0x00C36C28 [5]0x00C368D0 [6]0x00C36CA0
// A skill opened from the Class Mastery window: its pages (the description, the stats it
// modifies, the abilities it adds, the abilities it removes) in a VITextList, Left/Right
// to turn them.  0x6B0 bytes.  Method names ours.
class VIWndClassMasteryInspect : public VIWnd
{
public:
    VIWndClassMasteryInspect();                                  // 0x00C35F88
    virtual ~VIWndClassMasteryInspect();                         // 0x00C35FE0
    int  Init();                                                 // 0x00C36058
    void Free();                                                 // 0x00C360B8
    void Show(VICMNode *node);                                   // 0x00C360F8
    int  ShowPage(int page);                                     // 0x00C36220
    virtual void OnDrawSelf();                                   // 0x00C366D8
    virtual void OnKeyDown(int key);                             // 0x00C368D0
    virtual void OnKeyUp(int key);                               // 0x00C36C28
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C36CA0
    void DrawHeader();                                           // 0x00C36E48

    int            m_pages[8];     // +0x024: page kinds, 0 text, 1 stats, 2 adds, 3 removes
    int            m_pageCount;    // +0x044
    int            m_page;         // +0x048
    int            m_x4C;          // +0x04C
    VIUIWindow     m_window;       // +0x050
    VICMNode      *m_node;         // +0x1D0
    VIKeyFlash     m_keys;         // +0x1D4
    VITextList     m_list;         // +0x208
    char           m_x22C[4];      // +0x22C
    VIWndMessage   m_message;      // +0x230
};
CHK(c_VIWndClassMasteryInspect, sizeof(VIWndClassMasteryInspect) == 0x6B0)

// VIWndClassMasteryPoints: ctor 0x00C37B68, 480 bytes (`new`), vtable 0x00CE57D0;
//   slots [0]0x00C38D50 [1]0x00C37BB0 [2]OnDraw__5VIWnd [3]0x00C37C20 [4]0x00C37FA0 [5]0x00C37E58 [6]0x00C38010
// The Class Mastery points window: how many points are available and the share of the
// experience that goes to them.  0x1E0 bytes.  Method names ours.
class VIWndClassMasteryPoints : public VIWnd
{
public:
    VIWndClassMasteryPoints();                                   // 0x00C37B68
    virtual ~VIWndClassMasteryPoints();                          // 0x00C37BB0
    int  Init();                                                 // 0x00C37C10: returns 0
    void Free();                                                 // 0x00C37C18: empty
    virtual void OnDrawSelf();                                   // 0x00C37C20
    virtual void OnKeyDown(int key);                             // 0x00C37E58
    virtual void OnKeyUp(int key);                               // 0x00C37FA0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C38010

    void Change(int d);                                          // 0x00C380F8: the share, clamped to 0..100

    int            m_x24;          // +0x024
    VIUIWindow     m_window;       // +0x028
    int            m_percent;      // +0x1A8: the share of experience to points
    VIKeyFlash     m_keys;         // +0x1AC
};
CHK(c_VIWndClassMasteryPoints, sizeof(VIWndClassMasteryPoints) == 0x1E0)

// VIWndClassMasteryRequire: ctor 0x00C38DA0, 560 bytes (`new`), vtable 0x00CE5C18;
//   slots [0]0x00C3A530 [1]0x00C38DF0 [2]OnDraw__5VIWnd [3]0x00C39468 [4]0x00C39708 [5]0x00C39580 [6]0x00C39780
// What a skill not yet known requires: the description, the general requirements, the
// masteries, abilities and statistics needed, the faction standing, as pages.  0x230 bytes.
// Method names ours.
class VIWndClassMasteryRequire : public VIWnd
{
public:
    VIWndClassMasteryRequire();                                  // 0x00C38DA0
    virtual ~VIWndClassMasteryRequire();                         // 0x00C38DF0
    int  Init();                                                 // 0x00C38E58
    void Free();                                                 // 0x00C38E88: empty
    void Show(VICMNode *node);                                   // 0x00C38E90
    int  ShowPage(int page);                                     // 0x00C38FE8
    virtual void OnDrawSelf();                                   // 0x00C39468
    virtual void OnKeyDown(int key);                             // 0x00C39580
    virtual void OnKeyUp(int key);                               // 0x00C39708
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C39780
    void DrawHeader();                                           // 0x00C39848

    int            m_pages[8];     // +0x024
    int            m_pageCount;    // +0x044
    int            m_page;         // +0x048
    int            m_x4C;          // +0x04C
    VIUIWindow     m_window;       // +0x050
    VICMNode      *m_node;         // +0x1D0
    VIKeyFlash     m_keys;         // +0x1D4
    VITextList     m_list;         // +0x208
    int            m_x22C;         // +0x22C
};
CHK(c_VIWndClassMasteryRequire, sizeof(VIWndClassMasteryRequire) == 0x230)

// VIWndComposeGram: ctor 0x00C3A580, 3176 bytes (`new`), vtable 0x00CE6048;
//   slots [0]0x00C3C658 [1]0x00C3A5D8 [2]OnDraw__5VIWnd [3]0x00C3A808 [4]OnKeyUp__5VIWndi [5]0x00C3B0C8 [6]0x00C3B238
// A tell-gram (the in-game mail) being written: To, Subject and the body, each typed in a
// VIWndEdit in turn, then Send.  The same shape as VIWndCSRequest (Init/Free/Next/Prev/
// Select/TakeText/Cancel/ShowMessage).  0xC68 bytes; VIWndReadGram (lane cl-wq) embeds one
// for its reply.  Method names ours (provisional).
class VIWndComposeGram : public VIWnd
{
public:
    VIWndComposeGram();                                          // 0x00C3A580
    virtual ~VIWndComposeGram();                                 // 0x00C3A5D8

    int  Init(int reply);                                        // 0x00C3A648
    void Free();                                                 // 0x00C3A748

    virtual void OnDrawSelf();                                   // 0x00C3A808
    virtual void OnKeyDown(int key);                             // 0x00C3B0C8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C3B238

    unsigned short *GetTo();                                     // 0x00C3B4A0
    void SetTo(const unsigned short *s);                         // 0x00C3B4A8
    unsigned short *GetSubject();                                // 0x00C3B4D0
    void SetSubject(const unsigned short *s);                    // 0x00C3B4D8
    unsigned short *GetBody();                                   // 0x00C3B500
    void SetBody(const unsigned short *s);                       // 0x00C3B508
    void Refresh();                                              // 0x00C3B530: back to the first field
    void End();                                                  // 0x00C3B538: empty
    void Next();                                                 // 0x00C3B540
    void Prev();                                                 // 0x00C3B5B0
    void Select();                                               // 0x00C3B618: edit the field, or send
    void TakeText();                                             // 0x00C3B8B0: the editor's text, back
    void Cancel();                                               // 0x00C3B950
    void ShowMessage(const unsigned short *text, int prompt,
                     int a, int b);                              // 0x00C3B9D8

    int            m_reply;          // +0x024  Init's argument: 1 = a reply (starts at the body)
    VIUIWindow     m_window;         // +0x028
    VIUIWindow     m_status;         // +0x1A8
    VIWndMessage   m_message;        // +0x328
    VIWndMBox     *m_mbox;           // +0x7A8
    VIWndEdit     *m_edit;           // +0x7AC
    int            m_sel;            // +0x7B0  0 To, 1 Subject, 2 body, 3 Send
    int            m_x7B4;           // +0x7B4
    unsigned short m_to[24];         // +0x7B8
    unsigned short m_subject[64];    // +0x7E8
    unsigned short m_body[512];      // +0x868
};
CHK(c_VIWndComposeGram, sizeof(VIWndComposeGram) == 0xC68)

// VIWndCSInbox: ctor 0x00C3C6A8, 856 bytes (`new`), vtable 0x00CE65C0;
//   slots [0]0x00C3E810 [1]0x00C3C700 [2]OnDraw__5VIWnd [3]0x00C3C8E8 [4]0x00C3D550 [5]0x00C3D3E8 [6]0x00C3D590
// STATION's class (stwndcs.h), the same source: the customer-service inbox, the message
// list over a status window.  0x358 bytes.  Addresses are CLIENT's; the names STATION's.
class VIWndCSInbox : public VIWnd
{
public:
    VIWndCSInbox();                                              // 0x00C3C6A8
    virtual ~VIWndCSInbox();                                     // 0x00C3C700

    int  Init();                                                 // 0x00C3C760
    int  Free();                                                 // 0x00C3C850

    virtual void OnDrawSelf();                                   // 0x00C3C8E8
    virtual void OnKeyDown(int key);                             // 0x00C3D3E8
    virtual void OnKeyUp(int key);                               // 0x00C3D550
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C3D590

    void Refresh();                                              // 0x00C3D738
    void End();                                                  // 0x00C3D7B8  empty
    void Next();                                                 // 0x00C3D7C0
    void Prev();                                                 // 0x00C3D850
    void NextColumn();                                           // 0x00C3D8B8
    void PrevColumn();                                           // 0x00C3D908
    void Select();                                               // 0x00C3D958
    void Read();                                                 // 0x00C3DA50
    int  FindCategory(int category, int subcategory,
                      unsigned short *catName, unsigned short *subName);   // 0x00C3DB10

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

// VIWndCSRequest: ctor 0x00C3E860, 2560 bytes (`new`), vtable 0x00CE6920;
//   slots [0]0x00C419F8 [1]0x00C3E8E0 [2]OnDraw__5VIWnd [3]0x00C3EB50 [4]OnKeyUp__5VIWndi [5]0x00C3FA10 [6]0x00C3FDE8
// STATION's declaration (stwndcs.h), copied by the coordinator because VIWndReadCS (wq)
// EMBEDS it by value; CLIENT's `new` says 2560 = 0xA00 too.  Addresses are STATION's.
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

#endif
