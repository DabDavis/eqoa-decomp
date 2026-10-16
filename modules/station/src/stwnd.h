// stwnd.h -- OWNER: lane misc.  VIWndMBox (0x002F0128 .. 0x002F1DB8),
// VIWndMessage (0x002F1DB8 .. 0x002F25E0; most of it is SETUP's
// viwndmessage.cpp, byte for byte), VIWndOptionList (0x002F25E0 ..
// 0x002F2C38), the XML attribute helpers in front of libxml (0x002F4EB8 ..
// 0x002F54F0; cf. SETUP's pullxml.cpp) and VIWndCombo with the code behind
// libxml (0x003474D8 .. 0x00349630).
#ifndef STWND_H
#define STWND_H

// A UI icon's dictionary id (SUPPORT; SETUP's setup.h declares the same).
unsigned int VIHashResourceID_Icon(const char *name);

#include <libxml/tree.h>
#include <libxml/parser.h>

// ---- vixmlparser.cpp -- 0x002F4EF0 .. 0x002F54F0 (names ours) ------------------
// A SAX-like walk over a libxml tree, the page parser's front end: Parse runs
// xmlParseMemory and calls the current callback with a VIXMLEvent at each
// element's start (type 0) and end (type 1); a "text" child sets the event's
// text instead of being walked.  The callbacks are a stack (Push/Pop).
class VIXMLEvent;
class VIXMLParser;
// BY VALUE: under the EE's EABI a 20-byte argument travels as a pointer and the
// CALLEE copies it (every handler's prologue copies five words off a0), so Walk
// passes &ev with no copy of its own (lanes misc and page, reconciled).
typedef void (*VIXMLCallback)(VIXMLEvent e);

// 0x14 bytes, on Walk's stack.
class VIXMLEvent
{
public:
    VIXMLEvent(VIXMLParser *parser, int type, void *data, xmlNode *node,
               const char *text);                                // 0x002F4EF0
    VIXMLParser *GetParser();                                    // 0x002F4F50
    void SetType(int type);                                      // 0x002F4F58
    int  GetType();                                              // 0x002F4F60
    void *GetData();                                             // 0x002F4F68
    const char *GetName();                                       // 0x002F4F70
    void SetText(const char *s);                                 // 0x002F4F90
    const char *GetText();                                       // 0x002F4F98
    char *GetProp(const char *name);                             // 0x002F4FA0: xmlGetProp's copy
    int  GetPropInt(const char *name, int def);                  // 0x002F4FD0

    int          m_type;     // +0x00  0 an element starts, 1 it ends
    void        *m_data;     // +0x04  the parser's user data
    xmlNode     *m_node;     // +0x08
    const char  *m_text;     // +0x0C  a "text" child's content
    VIXMLParser *m_parser;   // +0x10
};

// 0x14 bytes.
class VIXMLParser
{
public:
    VIXMLParser();                                               // 0x002F5020
    void Walk(xmlNode *node);                                    // 0x002F5050
    int  Parse(char *buf, int len);                              // 0x002F5118: 1 if it is not XML
    void Push(VIXMLCallback cb);                                 // 0x002F5180
    void Pop();                                                  // 0x002F51C8
    void SetData(void *data);                                    // 0x002F5230
    void *GetData();                                             // 0x002F5238

    VIList<VIXMLCallback> m_stack;      // +0x00
    VIXMLCallback         m_callback;   // +0x0C  the stack's top
    void                 *m_data;       // +0x10
};

// The customer-service message box: a black frame fading in over the window,
// the text, an optional countdown, and one or two choice buttons.  0x5D8 bytes
// (VIWndCSInbox and VIWndCSRequest new one).  It draws and plays its sounds
// through the APP's VIUI (VIStation::m_ui, +0xAFE0 of GetAppPtr()), not UI().
// Its parent hears 268 (accept) or 260 (cancel, or the timeout running out),
// with m_p1 and m_p2.  m_type: 0 one button (Ok), 1 two (Yes/No, default No),
// 2 one (Cancel).  m_style picks the frame: 1 or 2, else none.
class VIWndMBox : public VIWnd
{
public:
    VIWndMBox();                                                 // 0x002F0128
    virtual ~VIWndMBox();                                        // 0x002F01A8

    int  Init(int type, int style);                              // 0x002F01F8
    void Clear();                                                // 0x002F0238 (name ours)

    virtual void OnDrawSelf();                                   // 0x002F0260
    void Update();                                               // 0x002F0B50 (name ours): fade, timeout
    virtual void OnKeyDown(int key);                             // 0x002F0C30
    void Accept();                                               // 0x002F0D28 (name ours)
    void Cancel();                                               // 0x002F0E78 (name ours)
    void Move(int dir);                                          // 0x002F0EE0 (name ours)
    virtual void OnKeyUp(int key);                               // 0x002F0FB8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002F0FD8

    void SetText(const char *s);                                 // 0x002F10A0
    void SetText(const unsigned short *s);                       // 0x002F10C8
    void SetText(unsigned short id);                             // 0x002F10F0: a WORDS.TXT id
    void ResetSel();                                             // 0x002F1140 (name ours)

    // Inline: their copies are emitted after __tf9VIWndMBox (0x002F1D88 ..).
    // Names ours.
    void SetPos(const VIVect2I &p) { m_pos = p; }
    void SetParam1(int p) { m_p1 = p; }
    void SetParam2(int p) { m_p2 = p; }
    void SetTimeout(long t) { m_timeout = t; }

    VIVect2I       m_pos;          // +0x024  the window's centre offset
    VIUIWindow     m_window;       // +0x030
    unsigned short m_text[512];    // +0x1B0
    int            m_type;         // +0x5B0
    int            m_sel;          // +0x5B4
    int            m_p1;           // +0x5B8  sent with 268/260
    int            m_p2;           // +0x5BC
    long           m_timeout;      // +0x5C0  VITimer ticks, -1 = none
    long           m_time;         // +0x5C8  the last Update
    float          m_alpha;        // +0x5D0  the fade, 0 .. 1
    int            m_style;        // +0x5D4
};
CHK(c_VIWndMBox, sizeof(VIWndMBox) == 0x5D8)

// The message box the other windows embed (VIWndReadCS at +0x328): a title, the
// text, and a prompt row of one or two choices.  Closing it sends the parent
// 307 with the selection.  0x480 bytes, SETUP's layout; every function but
// OnDrawSelf is byte-identical to SETUP's viwndmessage.cpp, and OnDrawSelf is
// the older one: it formats the choices with snprintf16 where SETUP's calls
// Highlight (which STATION does not have; m_buf is still in the class).
enum VIMESSAGEPROMPT
{
    VIMESSAGEPROMPT_OK = 0,         // "Ok" (786)
    VIMESSAGEPROMPT_OKCANCEL = 1,   // "Ok" (786) / 213
    VIMESSAGEPROMPT_YESNO = 2       // "Yes" (21) / "No" (22)
};

class VIWndMessage : public VIWnd
{
public:
    VIWndMessage();                                              // 0x002F1DB8
    virtual ~VIWndMessage();                                     // 0x002F1E00

    int  Init();                                                 // 0x002F1E50
    void Free();                                                 // 0x002F1E60

    virtual void OnDrawSelf();                                   // 0x002F1E68
    virtual void OnKeyDown(int key);                             // 0x002F2188
    virtual void OnKeyUp(int key);                               // 0x002F22D8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002F22E0

    void Setup(const char *title, const char *text, VIMESSAGEPROMPT prompt,
               const VIVect2I &center, int width);               // 0x002F23A0
    void Setup(const char *title, const unsigned short *text, VIMESSAGEPROMPT prompt,
               const VIVect2I &center, int width);               // 0x002F2418
    void Setup(const unsigned short *title, const unsigned short *text,
               VIMESSAGEPROMPT prompt, const VIVect2I &center, int width);   // 0x002F2498

    // Inline: its copy is emitted after __tf12VIWndMessage (0x002F25D8).
    int Selection() const { return m_sel; }

    VIUIWindow      m_window;     // +0x028
    unsigned short  m_title[32];  // +0x1A8
    unsigned short  m_text[256];  // +0x1E8
    int             m_sel;        // +0x3E8  0 = the first choice
    VIVect2I        m_center;     // +0x3EC
    int             m_width;      // +0x3F4
    int             m_rows;       // +0x3F8  text rows + 3
    VIMESSAGEPROMPT m_prompt;     // +0x3FC
    unsigned short  m_buf[64];    // +0x400  SETUP's Highlight buffer; unused here
};
CHK(c_VIWndMessage, sizeof(VIWndMessage) == 0x480)

// A popup list of choices under a title, one row each, the chosen one ticked.
// Enter sends the parent 308 (the selection, 1), Triangle/Escape 308 (the
// selection, 0).  0x208 bytes (VIWndReadCS news one).
class VIWndOptionList : public VIWnd
{
public:
    VIWndOptionList();                                           // 0x002F25E0
    virtual ~VIWndOptionList();                                  // 0x002F2628

    int  Init();                                                 // 0x002F2678
    void Free();                                                 // 0x002F2688

    virtual void OnDrawSelf();                                   // 0x002F2690
    virtual void OnKeyDown(int key);                             // 0x002F27E0
    virtual void OnKeyUp(int key);                               // 0x002F2970
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002F2978

    // 0x002F2A20 (name ours): at least 300 wide, and wide enough for the title
    // and every choice plus 80.
    void Setup(const unsigned short *title, const unsigned short **options, int count,
               const VIVect2I &center, int width);

    // Inline: its copy is emitted after __tf15VIWndOptionList (0x002F2C30).
    int Selection() const { return m_sel; }

    VIUIWindow             m_window;     // +0x028
    unsigned short         m_title[32];  // +0x1A8
    const unsigned short **m_options;    // +0x1E8  the caller's
    int                    m_count;      // +0x1EC
    int                    m_sel;        // +0x1F0
    VIVect2I               m_center;     // +0x1F4
    int                    m_width;      // +0x1FC
    int                    m_rows;       // +0x200  m_count + 3
};
CHK(c_VIWndOptionList, sizeof(VIWndOptionList) == 0x208)

// ---- viwndcombo.cpp -- 0x003474D8 .. 0x00349630, behind libxml ----------------
// A page option's state (name ours; no RTTI, no vtable): a label at a position,
// a value and, for a list (type 4), the visible window over m_count choices.
// Types: 1 a button, 2 a text field (m_text), 3 a toggle, 4 a list or a range.
// STATION calls little of it (VIWndCombo, the parser); the rest is linked whole.
class VIOptionItem
{
public:
    VIOptionItem();                                                  // 0x003474D8
    ~VIOptionItem();                                                 // 0x003474F8
    int  Init(const unsigned short *label, const VIVect2I &pos, int x4C, int x60);   // 0x00347520: type 1
    int  Init(const unsigned short *label, int x50, const VIVect2I &pos, int x4C,
              int x60);                                              // 0x003475A0: type 2
    int  Init(const unsigned short *label, int x50, const VIVect2I &pos);            // 0x00347640: type 3
    int  Init(const unsigned short *label, int x50, const VIVect2I &pos, int x4C,
              int min, int count, int value);                        // 0x003476B8: type 4, a list
    int  InitRange(const unsigned short *label, int x50, const VIVect2I &pos, int x4C,
                   int min, int max, int value);                     // 0x00347780: type 4, min..max
    void Free();                                                     // 0x00347858
    void ResetX70();                                                 // 0x00347890
    int  GetType();                                                  // 0x00347898
    unsigned short *GetLabel();                                      // 0x003478A0
    void SetLabel(const unsigned short *s);                          // 0x003478A8
    unsigned short *GetText();                                       // 0x003478D0
    void SetText(const unsigned short *s);                           // 0x003478D8: a new[] copy
    int  GetValue();                                                 // 0x00347950
    void SetValue(int v);                                            // 0x00347958
    int  GetCount();                                                 // 0x00347960
    int  GetCurrent();                                               // 0x00347968: m_value again
    void SetCurrent(int v);                                          // 0x00347970
    int  GetSel();                                                   // 0x00347978
    void SetSel(int s);                                              // 0x00347980: clamped
    int  GetX5C();                                                   // 0x003479A8
    void SetX5C(int v);                                              // 0x003479B0
    VIVect2I GetArrowPos(VIUI *ui);                                  // 0x003479B8
    void Draw(VIUI *ui, int focus);                                  // 0x00347AB0
    void DrawList(VIUI *ui);                                         // 0x00348200: VIWndCombo's
    // 0x003486A0 (name ours): s cut until it fits `width`, '*'s when `mask`; a
    // static buffer.
    unsigned short *FitText(VIUI *ui, const unsigned short *s, int width, int mask);

    int             m_type;        // +0x00
    unsigned short  m_label[32];   // +0x04
    VIVect2I        m_pos;         // +0x44
    int             m_x4C;         // +0x4C
    int             m_x50;         // +0x50  the label's x; < 0 = right-aligned at m_pos.x + it
    unsigned short *m_text;        // +0x54
    int             m_value;       // +0x58
    int             m_x5C;         // +0x5C
    int             m_x60;         // +0x60
    int             m_rows;        // +0x64  min(m_count, 3)
    int             m_sel;         // +0x68
    int             m_x6C;         // +0x6C
    int             m_x70;         // +0x70
    int             m_min;         // +0x74  a range's first number; a list's choices
                                   //        (const unsigned short **) when !(m_x60 & 2)
    int             m_count;       // +0x78
};

// The dropdown for one VIOptionItem.  0x2C bytes; VIWndGenericRenderer embeds
// one.  Up/down move the item's selection, X makes it the value; both X and
// Triangle send the parent 308.
class VIWndCombo : public VIWnd
{
public:
    VIWndCombo();                                                // 0x003487A0
    virtual ~VIWndCombo();                                       // 0x003487D8
    int  SetItem(VIOptionItem *item);                            // 0x00348800 (name ours)
    void Free();                                                 // 0x00348848
    virtual void OnDrawSelf();                                   // 0x00348850
    virtual void OnKeyDown(int key);                             // 0x003488A0
    virtual void OnKeyUp(int key);                               // 0x003489A8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x003489B0
    VIOptionItem *GetItem();                                     // 0x00348A08
    void Init();                                                 // 0x00348A10 (name ours)

    int           m_saved;   // +0x24  the value when the item was set
    VIOptionItem *m_item;    // +0x28
};
CHK(c_VIWndCombo, sizeof(VIWndCombo) == 0x2C)


#endif
