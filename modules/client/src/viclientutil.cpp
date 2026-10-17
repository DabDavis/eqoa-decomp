// Client helpers: CLIENT 0x00CC61A8 .. 0x00CC68D8, one TU (file name ours; it ends with its
// static initialiser and emits no template copies).
// The six boxes are the ranges VIClient's 0x40 encoder (0x00BC2F30) quantizes against.
#include "game.h"

VIBBox gBoundsPos(VIVect3(-4000.0f, -1000.0f, -4000.0f), VIVect3(32000.0f, 1000.0f, 32000.0f));     // 0x00CD7A58
VIBBox gBoundsVel(VIVect3(-15.3f, -84.5f, -15.3f), VIVect3(15.3f, 3.0f, 15.3f));                    // 0x00CD7A70
VIBBox gBoundsAccel(VIVect3(-62.6f, -12.51f, -62.52f), VIVect3(62.6f, 12.51f, 62.52f));             // 0x00CD7A88
VIBBox gBoundsHpr(VIVect3(-3.1415927f, -1.6f, 0.0f), VIVect3(3.1415927f, 1.6f, 0.0f));              // 0x00CD7AA0
VIBBox gBoundsAVel(VIVect3(-6.4f, -0.8f, 0.0f), VIVect3(6.4f, 0.8f, 0.0f));                        // 0x00CD7AB8
VIBBox gBoundsAAccel(VIVect3(-31.2f, -3.9f, 0.0f), VIVect3(31.2f, 3.9f, 0.0f));                    // 0x00CD7AD0
int gEntityUpdateRate = 100;                                                                       // 0x00CD7AE8

// 0x00CC61A8: may an item of wear type `wear` go in equipment slot `slot`.
int CanWearIn(int wear, int slot)
{
    int ok = 0;
    switch (wear)
    {
    case -1:
        ok = 0;
        break;
    case 0:
        break;
    case 1:
        ok = slot == 0;
        break;
    case 3:
        ok = slot == 2 || slot == 3;
        break;
    case 4:
        ok = slot == 4;
        break;
    case 5:
        ok = slot == 5;
        break;
    case 6:
        ok = slot == 6 || slot == 7;
        break;
    case 7:
        ok = slot == 8;
        break;
    case 8:
        ok = slot == 9 || slot == 10;
        break;
    case 9:
        ok = slot == 11;
        break;
    case 10:
        ok = slot == 12;
        break;
    case 11:
        ok = slot == 13;
        break;
    case 12:
        ok = slot == 14;
        break;
    case 13:
        ok = slot == 17;
        break;
    case 14:
        ok = slot == 14 || slot == 15;
        break;
    case 15:
        ok = slot == 16;
        break;
    case 16:
        ok = slot == 18;
        break;
    case 17:
        ok = slot == 18;
        break;
    case 18:
        ok = slot == 19 || slot == 20;
        break;
    case 19:
        ok = slot == 1;
        break;
    case 2:
        ok = slot == 21;
        break;
    case 20:
        ok = slot == 14;
        break;
    case 21:
        ok = slot == 15;
        break;
    case 22: case 23: case 24: case 25: case 26: case 27:
        ok = slot == 19;
        break;
    }
    return ok;
}

// 0x00CC62A8.  `fmt` is never read: its initialising copy is in the image, and the call
// takes the literal (the same string, merged).
unsigned int UIImageId(const char *name)
{
    char fmt[] = "P:\\studio\\eqps2\\Game_Assets\\UI\\%s.tga";
    char path[512];
    VIString::snprintf(path, 512, "P:\\studio\\eqps2\\Game_Assets\\UI\\%s.tga", name);
    return VIHashResourceID(path);
}

// 0x00CC6350
unsigned int ItemIconId(const char *name)
{
    char fmt[] = "P:\\studio\\eqps2\\Game_Assets\\Item_Icons\\%s.tga";
    char path[512];
    VIString::snprintf(path, 512, "P:\\studio\\eqps2\\Game_Assets\\Item_Icons\\%s.tga", name);
    return VIHashResourceID(path);
}

// 0x00CC6408
unsigned int FrontiersIconId(const char *name)
{
    char fmt[] = "P:\\studio\\eqps2\\Game_Assets\\Item_Icons\\Frontiers\\%s.tga";
    char path[512];
    VIString::snprintf(path, 512, "P:\\studio\\eqps2\\Game_Assets\\Item_Icons\\Frontiers\\%s.tga", name);
    return VIHashResourceID(path);
}

// 0x00CC64C0
void VIPingRing::Add(int ms)
{
    m_samples[m_head++] = ms > 0xFFFF ? 0xFFFF : ms;
    m_head &= 63;
}

// 0x00CC6500
unsigned short VIPingRing::Last()
{
    int i = m_head - 1;
    if (i < 0)
        i = m_head + 63;
    return m_samples[i];
}

// 0x00CC6528: the largest sample -- of m_samples[0], 64 times (as shipped).
unsigned short VIPingRing::Max()
{
    unsigned short m = m_samples[0];
    for (int i = 1; i < 64; i++)
        m = VIMAX(m, m_samples[0]);
    return m;
}
