// vigame.cpp -- SUPPORT 0x004A2948 .. 0x004A2A80.  Phase 61.
//
// Three free-standing game helpers between VIFont's TU and VIGameOptions':
// the equip-type/body-slot compatibility test and the game-code string.  The
// next TU's `_GLOBAL_$I$VIGetGameOptions__Fv` says VIGetGameOptions is ITS
// first public symbol, so these three are a TU of their own.
#include "vi.h"

extern "C" char *strcpy(char *, const char *);

// Only the one static member is in the image.  The test takes no `this`: the
// equip type arrives in a0 and indexes the jump table directly.
class VIGame
{
public:
    static int CheckEquipTypeBodySlotId(int type, int slot);
};

// The jump table at 0x004ED0C0 (relocated dump) runs -1..27.  Case 0 points
// straight at the join and so has no label at all; 22..27 share the last body,
// which falls into the join.  The bodies are in SOURCE order, and case 2's
// sits between 19 and 20.
int VIGame::CheckEquipTypeBodySlotId(int type, int slot)
{
    int ok = 0;

    switch (type)
    {
    case -1: ok = 0; break;
    case 1:  ok = slot == 0; break;
    case 3:  ok = slot == 2 || slot == 3; break;
    case 4:  ok = slot == 4; break;
    case 5:  ok = slot == 5; break;
    case 6:  ok = slot == 6 || slot == 7; break;
    case 7:  ok = slot == 8; break;
    case 8:  ok = slot == 9 || slot == 10; break;
    case 9:  ok = slot == 11; break;
    case 10: ok = slot == 12; break;
    case 11: ok = slot == 13; break;
    case 12: ok = slot == 14; break;
    case 13: ok = slot == 17; break;
    case 14: ok = slot == 14 || slot == 15; break;
    case 15: ok = slot == 16; break;
    case 16: ok = slot == 18; break;
    case 17: ok = slot == 18; break;
    case 18: ok = slot == 19 || slot == 20; break;
    case 19: ok = slot == 1; break;
    case 2:  ok = slot == 21; break;
    case 20: ok = slot == 14; break;
    case 21: ok = slot == 15; break;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27: ok = slot == 19; break;
    }

    return ok;
}

static char s_gameCode[8] = "EQOA";

const char *VIGetGameCode()
{
    return s_gameCode;
}

void VISetGameCode(const char *code)
{
    strcpy(s_gameCode, code);
}
