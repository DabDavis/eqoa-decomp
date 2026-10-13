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

// Phase 61.  Every table to its empty value, then the armour, hair, robe and
// helm resources looked up by name hash (in the order CHARCUST.CSF loaded
// them, which is why each set goes chest, boots, bracer, legs, gloves), then
// the face hashes and the fifteen tints as constants.  Race 9 has no faces.
void VICSpriteCust::SetResources(VIRaster &r, VIDictionary &d)
{
    int i, j;

    for (i = 0; i < 9; i++)
        for (j = 4; j >= 0; j--)
            Armor[i][j] = -1;
    for (i = 0; i < 11; i++)
        for (j = 0; j < 4; j++)
        {
            Race[i][j][0] = 0;
            Race[i][j][1] = 0;
        }
    for (i = 7; i >= 0; i--)
        Hair[i] = -1;
    HairNull = -1;
    for (i = 0; i < 4; i++)
    {
        Robe[i][0] = -1;
        Robe[i][1] = -1;
    }
    for (i = 8; i >= 0; i--)
        Helm[i] = -1;

    d.FindTyped(0xBF4B7E0A, VIRT_Surface, Armor[1][0]);
    d.FindTyped(0x7AD7D1B1, VIRT_Surface, Armor[1][4]);
    d.FindTyped(0x62444D1A, VIRT_Surface, Armor[1][1]);
    d.FindTyped(0x2CB53288, VIRT_Surface, Armor[1][3]);
    d.FindTyped(0x84D2F275, VIRT_Surface, Armor[1][2]);
    d.FindTyped(0x72D073EB, VIRT_Surface, Armor[2][0]);
    d.FindTyped(0xF55FAC7C, VIRT_Surface, Armor[2][4]);
    d.FindTyped(0xB99BD955, VIRT_Surface, Armor[2][1]);
    d.FindTyped(0xA73D0D53, VIRT_Surface, Armor[2][3]);
    d.FindTyped(0x3857E856, VIRT_Surface, Armor[2][2]);
    d.FindTyped(0x581046F4, VIRT_Surface, Armor[3][0]);
    d.FindTyped(0x54ECDCFF, VIRT_Surface, Armor[3][4]);
    d.FindTyped(0xC0EC54C8, VIRT_Surface, Armor[3][1]);
    d.FindTyped(0x06CA3DD6, VIRT_Surface, Armor[3][3]);
    d.FindTyped(0x1D97BB5F, VIRT_Surface, Armor[3][2]);
    d.FindTyped(0x40B0DDA7, VIRT_Surface, Armor[4][0]);
    d.FindTyped(0x4328B710, VIRT_Surface, Armor[4][4]);
    d.FindTyped(0xEB416A29, VIRT_Surface, Armor[4][1]);
    d.FindTyped(0xF50617E7, VIRT_Surface, Armor[4][3]);
    d.FindTyped(0x06385212, VIRT_Surface, Armor[4][2]);
    d.FindTyped(0x3FDB0DDB, VIRT_Surface, Armor[8][0]);
    d.FindTyped(0x5C8EA7CC, VIRT_Surface, Armor[8][4]);
    d.FindTyped(0x81343DA5, VIRT_Surface, Armor[8][1]);
    d.FindTyped(0x0E6C08A3, VIRT_Surface, Armor[8][3]);
    d.FindTyped(0x05628246, VIRT_Surface, Armor[8][2]);
    d.FindTyped(0x39BCF85F, VIRT_Surface, Armor[5][0]);
    d.FindTyped(0xC41543F8, VIRT_Surface, Armor[5][4]);
    d.FindTyped(0xE60EEF91, VIRT_Surface, Armor[5][1]);
    d.FindTyped(0x75F2A4CF, VIRT_Surface, Armor[5][3]);
    d.FindTyped(0xFF446CCA, VIRT_Surface, Armor[5][2]);
    d.FindTyped(0x2603BB5F, VIRT_Surface, Armor[7][0]);
    d.FindTyped(0xDB6204F8, VIRT_Surface, Armor[7][4]);
    d.FindTyped(0x238E0091, VIRT_Surface, Armor[7][1]);
    d.FindTyped(0x8D3F65CF, VIRT_Surface, Armor[7][3]);
    d.FindTyped(0xEB8B2FCA, VIRT_Surface, Armor[7][2]);
    d.FindTyped(0xF94BD113, VIRT_Surface, Armor[6][0]);
    d.FindTyped(0xBDBA8234, VIRT_Surface, Armor[6][4]);
    d.FindTyped(0xA0A2488D, VIRT_Surface, Armor[6][1]);
    d.FindTyped(0x6F97E30B, VIRT_Surface, Armor[6][3]);
    d.FindTyped(0xBED3457E, VIRT_Surface, Armor[6][2]);
    d.FindTyped(0x98F86489, VIRT_Surface, Hair[0]);
    d.FindTyped(0x221E7298, VIRT_Surface, Hair[1]);
    d.FindTyped(0x6DCEF2D0, VIRT_Surface, Hair[2]);
    d.FindTyped(0xFEBCE4D5, VIRT_Surface, Hair[3]);
    d.FindTyped(0xD499494B, VIRT_Surface, Hair[4]);
    d.FindTyped(0x8931CEE6, VIRT_Surface, Hair[5]);
    d.FindTyped(0x9F87C7AF, VIRT_Surface, Hair[6]);
    d.FindTyped(0xDC79B7C3, VIRT_Surface, Hair[7]);
    d.FindTyped(0x431B69AA, VIRT_Surface, HairNull);
    d.FindTyped(0x455F0D60, VIRT_Surface, Robe[0][0]);
    d.FindTyped(0x7DDA79FA, VIRT_Surface, Robe[0][1]);
    d.FindTyped(0xA89BE30B, VIRT_Surface, Robe[1][0]);
    d.FindTyped(0x0D4477B3, VIRT_Surface, Robe[1][1]);
    d.FindTyped(0x5D2F97E1, VIRT_Surface, Robe[2][0]);
    d.FindTyped(0x455D0BA5, VIRT_Surface, Robe[2][1]);
    d.FindTyped(0x2720732E, VIRT_Surface, Robe[3][0]);
    d.FindTyped(0xB64B5794, VIRT_Surface, Robe[3][1]);
    d.FindTyped(0x0C6F2FDA, VIRT_Sprite, Helm[1]);
    d.FindTyped(0x330D43A7, VIRT_Sprite, Helm[2]);
    d.FindTyped(0x079EAC92, VIRT_Sprite, Helm[3]);
    d.FindTyped(0x03B8E453, VIRT_Sprite, Helm[4]);
    d.FindTyped(0x1E25A25F, VIRT_Sprite, Helm[8]);
    d.FindTyped(0x5A51ACC1, VIRT_Sprite, Helm[5]);
    d.FindTyped(0x91E411BB, VIRT_Sprite, Helm[7]);
    d.FindTyped(0x7184EC3D, VIRT_Sprite, Helm[6]);

    Race[0][0][0] = 0x4A7AB44E;
    Race[0][0][1] = 0x765CFD7D;
    Race[0][1][0] = 0x5C086A9F;
    Race[0][1][1] = 0x87EAB3CE;
    Race[0][2][0] = 0x6D9620F0;
    Race[0][2][1] = 0x99786A1F;
    Race[0][3][0] = 0x7F23D741;
    Race[0][3][1] = 0xAB062070;
    Race[1][0][0] = 0x74732F1C;
    Race[1][0][1] = 0xF83D38BB;
    Race[1][1][0] = 0x8600E56D;
    Race[1][1][1] = 0x09CAEF0C;
    Race[1][2][0] = 0x978E9BBE;
    Race[1][2][1] = 0x1B58A55D;
    Race[1][3][0] = 0xA91C520F;
    Race[1][3][1] = 0x2CE65BAE;
    Race[2][0][0] = 0x42FB8FD2;
    Race[2][0][1] = 0xEC584121;
    Race[2][1][0] = 0x54894623;
    Race[2][1][1] = 0xFDE5F772;
    Race[2][2][0] = 0x6616FC74;
    Race[2][2][1] = 0x0F73ADC3;
    Race[2][3][0] = 0x77A4B2C5;
    Race[2][3][1] = 0x21016414;
    Race[3][0][0] = 0x99B0BB43;
    Race[3][0][1] = 0x63155B1A;
    Race[3][1][0] = 0xAB3E7194;
    Race[3][1][1] = 0x74A3116B;
    Race[3][2][0] = 0xBCCC27E5;
    Race[3][2][1] = 0x8630C7BC;
    Race[3][3][0] = 0xCE59DE36;
    Race[3][3][1] = 0x97BE7E0D;
    Race[4][0][0] = 0x6F7E7049;
    Race[4][0][1] = 0xBDB14A50;
    Race[4][1][0] = 0x810C269A;
    Race[4][1][1] = 0xCF3F00A1;
    Race[4][2][0] = 0x9299DCEB;
    Race[4][2][1] = 0xE0CCB6F2;
    Race[4][3][0] = 0xA427933C;
    Race[4][3][1] = 0xF25A6D43;
    Race[5][0][0] = 0x12A88832;
    Race[5][0][1] = 0x7E6E1C81;
    Race[5][1][0] = 0x24363E83;
    Race[5][1][1] = 0x8FFBD2D2;
    Race[5][2][0] = 0x35C3F4D4;
    Race[5][2][1] = 0xA1898923;
    Race[5][3][0] = 0x4751AB25;
    Race[5][3][1] = 0xB3173F74;
    Race[6][0][0] = 0xD9EC0301;
    Race[6][0][1] = 0x28F19AC8;
    Race[6][1][0] = 0xEB79B952;
    Race[6][1][1] = 0x3A7F5119;
    Race[6][2][0] = 0xFD076FA3;
    Race[6][2][1] = 0x4C0D076A;
    Race[6][3][0] = 0x0E9525F4;
    Race[6][3][1] = 0x5D9ABDBB;
    Race[7][0][0] = 0x6D7EAC68;
    Race[7][0][1] = 0xBB6F8467;
    Race[7][1][0] = 0x7F0C62B9;
    Race[7][1][1] = 0xCCFD3AB8;
    Race[7][2][0] = 0x909A190A;
    Race[7][2][1] = 0xDE8AF109;
    Race[7][3][0] = 0xA227CF5B;
    Race[7][3][1] = 0xF018A75A;
    Race[8][0][0] = 0xD4259B41;
    Race[8][0][1] = 0x07D0B508;
    Race[8][1][0] = 0xE5B35192;
    Race[8][1][1] = 0x195E6B59;
    Race[8][2][0] = 0xF74107E3;
    Race[8][2][1] = 0x2AEC21AA;
    Race[8][3][0] = 0x08CEBE34;
    Race[8][3][1] = 0x3C79D7FB;
    Race[10][0][0] = 0xFD04B372;
    Race[10][0][1] = 0xDAFD61C1;
    Race[10][1][0] = 0x0E9269C3;
    Race[10][1][1] = 0xEC8B1812;
    Race[10][2][0] = 0x20202014;
    Race[10][2][1] = 0xFE18CE63;
    Race[10][3][0] = 0x31ADD665;
    Race[10][3][1] = 0x0FA684B4;

    Tints[0].Set(255, 255, 255, 255);
    Tints[1].Set(90, 90, 90, 255);
    Tints[2].Set(0, 180, 255, 255);
    Tints[3].Set(150, 100, 50, 255);
    Tints[4].Set(0, 0, 140, 255);
    Tints[5].Set(0, 200, 50, 255);
    Tints[6].Set(250, 175, 50, 255);
    Tints[7].Set(80, 100, 0, 255);
    Tints[8].Set(240, 60, 60, 255);
    Tints[9].Set(165, 245, 255, 255);
    Tints[10].Set(160, 170, 180, 255);
    Tints[11].Set(220, 190, 150, 255);
    Tints[12].Set(0, 200, 180, 255);
    Tints[13].Set(150, 0, 60, 255);
    Tints[14].Set(240, 240, 0, 255);
}
