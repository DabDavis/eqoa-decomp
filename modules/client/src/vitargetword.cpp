#line 1 "C:/code-frontiers-gm/game/VITargetWord.cpp"
// A spell's target names (file name ours) -- CLIENT 0x00C0B880 .. 0x00C0B8B8, one function
// and its table; the TU's data (0x00CE04A8 ..) is the database table-name list, the table
// and a bad_alloc string.
#include "game.h"

// 0x00CE06F0: a spell's target type and the WORDS.TXT id of its name, 5 entries.
static VIWordId sTargetWords[] =
{
    { 144, 0 },     // SELF
    { 145, 1 },     // TARGET
    {  79, 2 },     // GROUP
    { 469, 3 },     // PET
    { 470, 4 },     // CORPSE
};

// 0x00C0B880.  The LAST entry for the type (no break).  The spell book calls it.
VIWordId *FindTargetWord(int type)
{
    VIWordId *found = 0;

    for (int i = 0; i < sizeof(sTargetWords) / sizeof(sTargetWords[0]); i++)
        if (sTargetWords[i].m_id == type)
            found = &sTargetWords[i];
    return found;
}
