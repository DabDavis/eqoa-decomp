// virace.cpp -- SUPPORT 0x004A2F50 .. 0x004A4050.  Phase 61.
//
// The race descriptor table and its three readers.  `_GLOBAL_$I$GetRace__Fi`
// names GetRace as the TU's first public symbol, so the three icon hashes
// ahead of it are a TU of their own.
//
// gRaces is NOT initialised data: the model words are VIHashResourceID calls,
// so the whole aggregate is built at static-init time -- each element's
// CONSTRUCTOR expanded into one stack temporary (memset to zero, the non-zero
// fields stored) and block-copied into place, and the eleven empty slots
// copied from one constant record in .rodata.  That initialiser is the
// ~1000-instruction body the map folds into
// IsValidRaceClassCityComboExpansion's span (SPAN_CAP 112).
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);

#define EMPTY_RACE { -1, -1, 0, 0, { 0, 0, 0, 0, 0, 0, 0 }, -1, 0.0f, 0.0f, "", 0 }

static VIRaceDef gRaces[21] =
{
    { 183,  317, VIHashResourceID("HumanMale"),     VIHashResourceID("HumanFemale"),
      {  5,  5,  5,  5,  5,  5,  5 }, 0x7EFF,   0.0f, -26.0f, "bgm\\qeynos.bgm",   0 },
    { 185,  318, VIHashResourceID("ElfMale"),       VIHashResourceID("ElfFemale"),
      {  0,  0,  0, 10, 10,  5, 10 }, 0x76E6,   0.0f,   0.0f, "bgm\\elves.bgm",    0 },
    { 186,  319, VIHashResourceID("DarkElfMale"),   VIHashResourceID("DarkElfFemale"),
      {  0,  5,  0, 10,  5, 15,  0 }, 0x7E49, -13.0f,   0.0f, "bgm\\darkelve.bgm", 0 },
    { 187,  320, VIHashResourceID("GnomeMale"),     VIHashResourceID("GnomeFemale"),
      {  0,  0, 10, 10,  0, 15,  0 }, 0x7E41,  13.0f,   0.0f, "bgm\\gnomes.bgm",   0 },
    { 188,  321, VIHashResourceID("DwarfMale"),     VIHashResourceID("DwarfFemale"),
      { 10, 10,  0,  5, 10,  0,  0 }, 0x0245, -13.0f, -13.0f, "bgm\\dwarves.bgm",  0 },
    { 189,  322, VIHashResourceID("TrollMale"),     VIHashResourceID("TrollFemale"),
      { 20, 15,  0,  0,  0,  0,  0 }, 0x0109, -13.0f,  13.0f, "bgm\\trolls.bgm",   0 },
    { 191,  323, VIHashResourceID("BarbarianMale"), VIHashResourceID("BarbarianFemale"),
      { 15, 10,  0,  5,  0,  5,  0 }, 0x0141,  13.0f, -13.0f, "bgm\\barbaria.bgm", 0 },
    { 192,  324, VIHashResourceID("HalflingMale"),  VIHashResourceID("HalflingFemale"),
      {  0, 10, 10, 15,  0,  0,  0 }, 0x02C1,   0.0f, -13.0f, "bgm\\halfling.bgm", 0 },
    { 193,  325, VIHashResourceID("EruditeMale"),   VIHashResourceID("EruditeFemale"),
      {  0,  0,  0,  5, 10, 20,  0 }, 0x7E0C,   0.0f,  13.0f, "bgm\\erudites.bgm", 0 },
    { 190, 1191, VIHashResourceID("OgreMale"),      VIHashResourceID("OgreFemale"),
      { 20, 15,  0,  0,  0,  0,  0 }, 0x0909,  13.0f,  13.0f, "bgm\\ogre.bgm",     2 },
    EMPTY_RACE, EMPTY_RACE, EMPTY_RACE, EMPTY_RACE, EMPTY_RACE, EMPTY_RACE,
    EMPTY_RACE, EMPTY_RACE, EMPTY_RACE, EMPTY_RACE, EMPTY_RACE,
};

const VIRaceDef *GetRace(int r)
{
    if ((unsigned int)r >= 21)
        r = 0;
    return &gRaces[r];
}

// City is a HUMAN-only concept (0, and the menu's East 1 / West 2).  A city
// outside 0..2 for race 0 returns `ok` uninitialised: shipped.
int IsValidRaceClassCityCombo(int race, int c, int city)
{
    int ok;

    if ((unsigned int)race >= 21 || c < 0 || c >= 15)
        ok = 0;
    else if (race == 0)
    {
        switch (city)
        {
        case 0: ok = (gRaces[0].m_classMask >> c) & 1; break;
        case 1: ok = (0x7E69 >> c) & 1; break;
        case 2: ok = (0x76F7 >> c) & 1; break;
        }
    }
    else
        ok = (gRaces[race].m_classMask >> c) & 1;

    return ok;
}

int IsValidRaceClassCityComboExpansion(int race, int c, int city,
                                       unsigned int owned)
{
    if (!IsValidRaceClassCityCombo(race, c, city))
        return 0;

    const VIClassDef *cd = GetClass(c);
    const VIRaceDef  *rd = GetRace(race);
    unsigned int need = cd->m_expansion | rd->m_expansion;

    return (owned & need) == need;
}
