#line 1 "C:/code-frontiers-gm/game/VIWeaponType.cpp"
// A one-function TU between VITextList's and VIWndAuction.cpp (file name ours): its
// .rodata is game.h's table-name strings and then this table, nothing else.
#include "game.h"

// 0x00CE0A20: each weapon type's word (WORDS.TXT 1HS, 2HS, 1HB, 2HB, 1HP, 2HP, RANGED,
// SHIELD), keyed by the type -- the bit VIWndSpellBook::AddEquipReq tests.
VIWeaponTypeWord gWeaponTypeWords[8] =
{
    { 342, 0 }, { 343, 1 }, { 344, 2 }, { 345, 3 },
    { 346, 4 }, { 348, 5 }, { 347, 6 }, { 339, 7 },
};

// 0x00C0D000: the row for a type, 0 if none.  Every row is visited, so the LAST match wins.
const VIWeaponTypeWord *FindWeaponTypeWord(int type)
{
    const VIWeaponTypeWord *r = 0;
    for (unsigned int i = 0; i < sizeof(gWeaponTypeWords) / sizeof(gWeaponTypeWords[0]); i++)
        if (gWeaponTypeWords[i].m_type == type)
            r = &gWeaponTypeWords[i];
    return r;
}
