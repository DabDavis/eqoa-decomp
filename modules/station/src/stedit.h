// stedit.h -- VIWndEdit, the on-screen text editor
// (0x002B98D0 .. 0x002BC6A8).  CLIENT links the same source (0x00C44448 ..):
// 28 of its 31 functions are byte-identical there, OnDrawSelf (5,688 bytes vs
// 5,680) and OnMessage (232 vs 164) are not, and CLIENT's TU also carries the
// VIMap<int, int> template copies -- one file with a module #ifdef, not yet
// shared.
#ifndef STEDIT_H
#define STEDIT_H

// One wrapped line of the text: where it starts in m_text, how many
// characters, and whether it ends in a break (a newline, or a wrap).
struct VIEditLine
{
    unsigned short *m_start;   // +0x0
    short           m_len;     // +0x4
    short           m_break;   // +0x6
};

// 0x1C0 bytes (new in 0x002BC768), vptr at +0x20.  ctor 0x002B98D0, dtor
// 0x002B9A28, __tf 0x002BC5A0.  A text box above an 11 x 6 soft keyboard;
// the pad walks the keyboard, X types the key under the cursor, a USB
// keyboard types straight in.  Done sends the parent 259 (p1, p2 = m_x24,
// m_x28), cancel 260.  The method names are ours.
class VIWndEdit : public VIWnd
{
public:
    VIWndEdit();
    virtual ~VIWndEdit();

    void SetMaxLength(int n);                    // 0x002B9A90
    void SetRows(int n);                         // 0x002B9B68
    void SetMultiLine(int on);                   // 0x002B9B70  shows the soft keyboard's newline key
    int  MultiLine();                            // 0x002B9B98
    void SetAllowSpaces(int on);                 // 0x002B9BA0
    int  AllowSpaces();                          // 0x002B9BA8
    void SetTitle(const char *s);                // 0x002B9BB0
    void SetTitle(const unsigned short *s);      // 0x002B9BE0

    virtual void OnDrawSelf();                   // 0x002B9C10

    int  InsertChar(unsigned short c);           // 0x002BB240
    void Clear();                                // 0x002BB350
    void SetCenter(const VIVect2I &p);           // 0x002BB378
    void SetText(const unsigned short *s);       // 0x002BB3A8
    void SetText(const char *s);                 // 0x002BB3F0
    int  CursorRight();                          // 0x002BB438
    int  CursorLeft();                           // 0x002BB478
    int  Backspace();                            // 0x002BB4B8
    void PlaySound(int ok);                      // 0x002BB568

    virtual void OnKeyDown(int key);                             // 0x002BB598
    virtual void OnKeyUp(int key);                               // 0x002BBAC0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002BBB10

    int  GetCursor();                            // 0x002BBBB8
    int  SetCursor(int pos);                     // 0x002BBBC0  returns whether it moved
    void WrapLines();                            // 0x002BBC30
    int  CursorLineCol(VIVect2I &out);           // 0x002BBF08  out = (column, line); -1 past the end
    int  LineUp();                               // 0x002BBFF8
    int  LineDown();                             // 0x002BC1D8
    void MoveKey(int dx, int dy);                // 0x002BC3D0  the soft keyboard's cursor

    // Inline: emitted after __tf9VIWndEdit in this order (0x002BC5F0 ..).
    float KeyGlow() { return m_x168; }
    void  SetPos(const VIVect2I &p) { m_pos = p; }
    unsigned short *GetText() { return m_text; }
    void  SetX24(int v) { m_x24 = v; }
    void  SetX28(int v) { m_x28 = v; }
    void  SetX18C(int v) { m_x18C = v; }
    void  ClearX24() { m_x24 = 0; m_x28 = 0; }
    VIVect2I GetPos() { return m_pos; }
    VIVect2I GetEnd() { return m_pos + m_size; }   // CLIENT's VIWndCharCreateName needs operator+
    void  Touch() { m_lastInput = VITimer::GetTime(); }

    int             m_x24;          // +0x024  handed back with 259
    int             m_x28;          // +0x028
    int             m_x2C;          // +0x02C  a shift
    int             m_shift;        // +0x030  R1 held
    int             m_caps;         // +0x034  the caps key's toggle
    VIVect2I        m_pos;          // +0x038
    VIVect2I        m_size;         // +0x040
    unsigned short  m_title[128];   // +0x048
    VIVect2I        m_key;          // +0x148  the soft keyboard's cursor (column, row)
    VIVect2I        m_lastKey;      // +0x150
    VIVect2I        m_pressed;      // +0x158  the key X last typed
    float           m_press;        // +0x160  its flash, 1 when typed
    float           m_glide;        // +0x164  the keyboard cursor's glide, 0..1
    float           m_x168;         // +0x168
    float           m_pulse;        // +0x16C  the Done key's glow, 0..1 and back
    int             m_x170;         // +0x170
    int             m_x174;         // +0x174
    long            m_x178;         // +0x178  when the focus arrived
    int             m_repeatKey;    // +0x180  the pad direction auto-repeating
    float           m_repeatDelay;  // +0x184
    int             m_rows;         // +0x188
    int             m_x18C;         // +0x18C
    int             m_x190;         // +0x190
    int             m_cursor;       // +0x194
    int             m_wantX;        // +0x198  LineUp/LineDown's pixel column, -1 none
    int             m_x19C;         // +0x19C
    long            m_lastInput;    // +0x1A0
    unsigned short *m_text;         // +0x1A8
    int             m_maxLen;       // +0x1AC
    VIEditLine     *m_lines;        // +0x1B0
    int             m_numLines;     // +0x1B4
    int             m_multiLine;    // +0x1B8
    int             m_allowSpaces;  // +0x1BC
};
CHK(c_VIWndEdit, sizeof(VIWndEdit) == 0x1C0)

#endif
