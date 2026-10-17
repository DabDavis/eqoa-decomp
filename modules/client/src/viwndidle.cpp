#line 1 "C:/code-frontiers-gm/game/VIWndIdle.cpp"
// viwndidle.cpp -- CLIENT 0x00C5FA00 .. 0x00C60780: VIWndIdle, the window that holds the
// focus while a world loads.  Every handler is VIWnd's.
#include "game.h"

// 0x00C5FA00.
VIWndIdle::VIWndIdle()
{
}

// 0x00C5FA38.
VIWndIdle::~VIWndIdle()
{
}

// 0x00C5FA60.
void VIWndIdle::OnDrawSelf()
{
}

// 0x00C5FA68.
void VIWndIdle::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIWnd::OnKeyDown(key);
}

// 0x00C5FAA8.
void VIWndIdle::OnKeyUp(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIWnd::OnKeyUp(key);
}

// 0x00C5FAE8.
void VIWndIdle::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIWnd::OnMessage(w, msg, p1, p2);
}
