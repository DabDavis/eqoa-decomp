// mcerr.h -- MCERR's one class, on top of SUPPORT's vi.h.
//
// The names are the 2002 betas' own: their MCERR exports 41 symbols where the
// retail builds export six (decomp-builds/, `snr1map.py`).  Frontiers adds four
// screens the betas do not have (12, 17, 21, 22), drops 25 from the dispatcher
// while keeping its body, and gives McErrSyncIdle a keyboard.
#ifndef MCERR_H
#define MCERR_H

#include "vi.h"

extern "C" int   printf(const char *fmt, ...);
extern "C" void *memset(void *, int, int);

VIGameOptions *VIGetGameOptions();

// One WORDS.TXT entry in the current language.  `ui` is ONE value: the game's
// name (`Word(UI(), VIGetExpansionFlags() & 2 ? 1803 : 1802)`) loads it before
// the flag call and reads m_language off the same register after it.
inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The memory-card error and setup window: one static instance, driven either
// as a child of a real window (McErr) or synchronously by its caller's own
// loop (McErrSyncSet + McErrSyncIdle).  Every screen is one ProcessScreenNN,
// called with msg 0 to draw, 3 for a key going down and 4 for a key going up.
class VIWndMcErr : public VIWnd
{
public:
    VIWndMcErr();
    virtual ~VIWndMcErr();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void GotoScreen(int screen);
    void ExitMcErr(int result);

    int ProcessScreen(int msg, int key, int p);
    int ProcessScreen04(int msg, int key, int p);
    int ProcessScreen10(int msg, int key, int p);
    int ProcessScreen11(int msg, int key, int p);
    int ProcessScreen12(int msg, int key, int p);
    int ProcessScreen15(int msg, int key, int p);
    int ProcessScreen16(int msg, int key, int p);
    int ProcessScreen17(int msg, int key, int p);
    int ProcessScreen20(int msg, int key, int p);
    int ProcessScreen21(int msg, int key, int p);
    int ProcessScreen22(int msg, int key, int p);
    int ProcessScreen25(int msg, int key, int p);
    int ProcessScreen30(int msg, int key, int p);
    int ProcessScreen50(int msg, int key, int p);
    int ProcessScreen52(int msg, int key, int p);
    int ProcessScreen54(int msg, int key, int p);
    int ProcessScreen60(int msg, int key, int p);
    int ProcessScreen61(int msg, int key, int p);
    int ProcessScreen62(int msg, int key, int p);
    int ProcessScreen63(int msg, int key, int p);
    int ProcessScreen64(int msg, int key, int p);

    void DrawPrompt(int row, const unsigned short *a, const unsigned short *b);

    VIUIWindow   m_window;   // +0x028
    int          m_sel;      // +0x1A8  which of the two prompts is lit
    int          m_screen;   // +0x1AC
    int          m_count;    // +0x1B0  frames on this screen
    VIVect2I     m_center;   // +0x1B4
    VIWnd       *m_owner;    // +0x1BC  0 when driven by McErrSyncIdle
    int          m_mode;     // +0x1C0  McErrSetMode
    int          m_result;   // +0x1C4  what McErrSyncIdle returns
    VIConsolePad m_pad;      // +0x1C8  last frame's pad, for the edges
};

VIWndMcErr *McErrWnd();
int  McErr(VIWnd *owner, const VIVect2I &center, int screen);
void McErrSetMode(int mode);
int  McErrSyncSet(const VIVect2I &center, int screen);
int  McErrSyncIdle(VIPad *pad, VIKeyboard *kb);
void VISetMcSessionId(unsigned long id);
unsigned long VIGetMcSessionId();
int  VIIsMcSameSession();

#endif
