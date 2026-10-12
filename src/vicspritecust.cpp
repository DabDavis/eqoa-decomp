// VICSpriteCust -- SUPPORT 0x004283C8 .. 0x00429570.  Its own TU.
//
// The character-customisation tables and the seven lookups into them.  Every
// value here was already known from EE RAM (claude-notes/vicspritecust-tables.md);
// what the object code settles is the SHAPE of each table, because the index
// arithmetic is the only thing an un-relocated image will tell you about a
// static array.
#include "vi.h"

VIColor32 VICSpriteCust::Tints[15];
int       VICSpriteCust::Armor[9][5];
int       VICSpriteCust::Race[11][4][2];
int       VICSpriteCust::Hair[8];
int       VICSpriteCust::Robe[4][2];
int       VICSpriteCust::Helm[10];
int       VICSpriteCust::HairNull;

// The race is destroyed by `li a0,20` in the guard's delay slot: this reads
// Armor[set][slot] and nothing else.
int VICSpriteCust::GetArmorSetTexture(VICSpriteRace race, VICSpriteArmorSet set,
                                      VICSpriteTextSlot slot)
{
    // Phase 7's if-direction: the lookup goes IN the `if`, and gcc moves that
    // returning body out of line, leaving the `return -1` as the fallthrough.
    // Written as an early `set == 0` guard the two halves swap, 10 of 12 words.
    if (set != 0)
        return Armor[set][slot];
    return -1;
}

int VICSpriteCust::GetHairTexture(int hair)
{
    return Hair[hair];
}

int VICSpriteCust::GetFaceTexture(int face, int sex, VICSpriteRace race, VIDictionary &d)
{
    int i;

    if (d.FindTyped(GetFaceResourceID(face, sex, race), VIRT_Surface, i))
        return i;
    return -1;
}

int VICSpriteCust::GetFaceResourceID(int face, int sex, VICSpriteRace race)
{
    return Race[race][face][sex ? 1 : 0];
}

int VICSpriteCust::GetRobeTexture(int robe, VICSpriteTextSlot slot)
{
    if (robe >= 4)
        robe = 0;
    if (slot == 0)
        return Robe[robe][0];
    if (slot == 3)
        return Robe[robe][1];
    return -1;
}

int VICSpriteCust::GetHelm(VICSpriteArmorSet set)
{
    return Helm[set];
}

VIColor32 *VICSpriteCust::GetTintColor(VICSpriteTint tint)
{
    return &Tints[tint];
}
