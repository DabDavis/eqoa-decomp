// viclass.cpp -- SUPPORT 0x0049F668 .. 0x0049F6D0.  Phase 61.
//
// The class descriptor table and its two readers, a TU of its own between
// VIBattleMusic's and VICSFFile's.  Fifteen 44-byte records at 0x004EC2B0,
// initialised DATA (no constructor runs): the name and short-name string ids,
// a mask of the stats the create screen highlights, the seven stat bonuses
// (STR STA AGI DEX WIS INT CHA) and the expansion mask -- Alchemist is the one
// class that needs expansion bit 1, which IsValidRaceClassCityComboExpansion
// ORs with the race's.
#include "vi.h"

static VIClassDef s_classes[15] =
{
    { 165,  303,  3, {  10,  15,   0,   0,  -5, -10, -10 }, 0 },   // WAR
    { 166,  304, 12, {   0,   0,  10,  15,  -5, -10, -10 }, 0 },   // RAN
    { 167,  305,  3, {  10,  15,   0,   0,  -5, -10, -10 }, 0 },   // PAL
    { 168,  306,  3, {  10,  15,   0,   0, -10,  -5, -10 }, 0 },   // SK
    { 169,  307, 10, {   0,  10,   0,  15,  -5, -10, -10 }, 0 },   // MNK
    { 170,  308,  6, {  -5,  10,  15,   0, -10, -10,   0 }, 0 },   // BRD
    { 171,  309, 12, {   0,   0,  10,  15, -10, -10,  -5 }, 0 },   // RGE
    { 172,  310, 24, {   0, -10, -10,  10,  15,  -5,   0 }, 0 },   // DRD
    { 173,  311, 18, { -10,  10,   0,  -5,  15, -10,   0 }, 0 },   // SHA
    { 174,  312, 80, {   0,   0,  -5, -10,  15, -10,  10 }, 0 },   // CLR
    { 175,  313, 36, { -10,   0,  10,  -5,   0,  15, -10 }, 0 },   // MAG
    { 176,  314, 34, {   0,  10, -10,   0,  -5,  15, -10 }, 0 },   // NEC
    { 177,  315, 96, { -10, -10,   0,   0,  -5,  15,  10 }, 0 },   // ENC
    { 178,  316, 40, {   0,  -5,   0,  10, -10,  15, -10 }, 0 },   // WIZ
    { 179, 1190, 48, { -10,   0,  -5,   0,  10,  15, -10 }, 2 },   // ALC
};

// Out of range is class 0, not a null -- the same clamp as GetRace and the UI
// atlas lookups.
const VIClassDef *GetClass(int c)
{
    if ((unsigned int)c >= 15)
        c = 0;
    return &s_classes[c];
}

int IsStatHighlighted(int c, int stat)
{
    if ((unsigned int)stat >= 7 || c < 0 || c >= 15)
        return 0;
    return (s_classes[c].m_statMask >> stat) & 1;
}
