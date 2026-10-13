// vihashicon.cpp -- SUPPORT 0x004A2D38 .. 0x004A2F50.  Phase 61.
//
// The three icon-name hashes: a format string, the name, VIHashResourceID.
// Their own TU -- they follow VIGameOptions' `_GLOBAL_$I$` and precede the
// race TU's first public symbol.
//
// Each one also copies its format string into a stack array it never reads:
// the frame holds the 38/46/56-byte copy at sp+0 and the path buffer above it,
// and the call is handed the .rodata literal itself (identical string
// constants are one label in a TU).  gcc 2.95 does not delete a dead
// initialised local array -- phase 36's VIPad::Read again.
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);

#define ICON_PATH      "P:\\studio\\eqps2\\Game_Assets\\UI\\%s.tga"
#define ITEM_PATH      "P:\\studio\\eqps2\\Game_Assets\\Item_Icons\\%s.tga"
#define FRONTIERS_PATH "P:\\studio\\eqps2\\Game_Assets\\Item_Icons\\Frontiers\\%s.tga"

unsigned int VIHashResourceID_Icon(const char *name)
{
    char fmt[] = ICON_PATH;
    char path[512];

    VIString::snprintf(path, 512, ICON_PATH, name);
    return VIHashResourceID(path);
}

unsigned int VIHashResourceID_ItemIcon(const char *name)
{
    char fmt[] = ITEM_PATH;
    char path[512];

    VIString::snprintf(path, 512, ITEM_PATH, name);
    return VIHashResourceID(path);
}

unsigned int VIHashResourceID_ItemIcon_Frontiers(const char *name)
{
    char fmt[] = FRONTIERS_PATH;
    char path[512];

    VIString::snprintf(path, 512, FRONTIERS_PATH, name);
    return VIHashResourceID(path);
}
