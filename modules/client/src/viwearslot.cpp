#line 1 "C:/code-frontiers-gm/game/VIWearSlot.cpp"
// The wear-slot names (file name ours) -- CLIENT 0x00C04D10 .. 0x00C04D48, one function and
// its table; the TU's data (0x00CDF7BA ..) ends with the table and a bad_alloc string.
#include "game.h"

// 0x00CDFA50: a wear slot and the WORDS.TXT id of its name, 28 entries.
static VIWordId sWearSlotWords[] =
{
    {  302, -1 },   // NONE
    {  327,  1 },   // HEAD
    {  329,  3 },   // EAR
    {  330,  4 },   // NECK
    {  331,  5 },   // TORSO
    {  332,  6 },   // FOREARM
    {  333,  7 },   // 2FOREARM
    {  405,  8 },   // RING
    {  335,  9 },   // WAIST
    {  336, 10 },   // LEGS
    {  337, 11 },   // FEET
    {  338, 12 },   // PRIMARY
    {  339, 13 },   // SHIELD
    {  340, 14 },   // SECONDARY
    {  341, 15 },   // 2HAND
    {  404, 19 },   // GLOVES
    {  403,  2 },   // ROBE
    {  398, 16 },   // BOW
    {  399, 17 },   // THROWN
    {  402, 18 },   // HELD
    { 1204, 20 },   // FISHING
    { 1206, 21 },   // BAIT
    { 1550, 22 },   // WEAPON CRAFT
    { 1549, 23 },   // ARMOR CRAFT
    { 1552, 24 },   // TAILORING
    { 1548, 25 },   // JEWEL CRAFT
    { 1553, 26 },   // CARPENTRY
    { 1551, 27 },   // ALCHEMY
};

// 0x00C04D10.  The LAST entry for the slot (no break).
VIWordId *FindWearSlotWord(int slot)
{
    VIWordId *found = 0;

    for (int i = 0; i < sizeof(sWearSlotWords) / sizeof(sWearSlotWords[0]); i++)
        if (sWearSlotWords[i].m_id == slot)
            found = &sWearSlotWords[i];
    return found;
}
