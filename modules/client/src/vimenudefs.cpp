#line 1 "C:/code-frontiers-gm/game/VIMenuDefs.cpp"
// The game window's menu definitions (file name ours) -- CLIENT 0x00C0B240 .. 0x00C0B350:
// nothing but data, and the static initialiser for the two definitions that need one.
// The definitions are 16-byte VIMenuDefs (cl_wl.h) in .data (0x00CD02A0 ..), most
// initialised statically; VIWndGame reads them.  The item tables they point at
// (0x00CD0138 .., 32 bytes an item) and the rest of the records are not transcribed yet.
// The TU's .rodata (0x00CE0230 ..) is the database table-name list, "backpack" and a
// bad_alloc string.
#include "game.h"

extern "C" void *memset(void *, int, int);

// 0x00CD02A0: the first definition -- its count is a variable, so it is set at start-up.
VIMenuDef gStartMenu = { -1, -1, gStartItemsBank, gStartCountBank };

// 0x00CD02F0: the backpack's -- its icon is a hashed name.
VIMenuDef gEquipMenu = { UIImageId("backpack"), -1, gEquipItems, 40 };
