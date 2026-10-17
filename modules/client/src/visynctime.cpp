#line 1 "C:/code-frontiers-gm/game/VISyncTime.cpp"
// VISyncTime -- CLIENT 0x00CBCBD8 .. 0x00CBCD10: a clock synced to the server's.  Set
// moves it to a new reading without jumping: the difference is blended in over
// BLEND_INTERVAL ms of local time (GetTime).  VIClient has two (+0x2EC30, +0x2EC50).
// Asserts on lines 77 and 95.
#include "game.h"

#define VITIME_MAX      0x7FFFFFFFFFFFFFFF
#define BLEND_INTERVAL  60000
// (blank lines below pad the asserts to their original lines)


























































// 0x00CBCBD8
void VISyncTime::Set(long t, long base)
{
    if (m_bReady)
    {
        long cur = GetTime(base);
        m_Blend = t - cur;
        m_remote = cur;
        m_local = base;
        VIASSERT(m_Blend < ( VITIME_MAX / BLEND_INTERVAL ));
    }
    else
    {
        m_bReady = 1;
        m_local = base;
        m_remote = t;
        m_Blend = 0;
    }
}





// 0x00CBCC80
long VISyncTime::GetTime(long now)
{
    VIASSERT(m_bReady);
    long dt = now - m_local;
    long m = dt > BLEND_INTERVAL ? BLEND_INTERVAL : dt;
    long r = m_Blend * m / BLEND_INTERVAL;
    return m_remote + dt + r;
}

// 0x00CBCD08
void VISyncTime::Reset()
{
    m_bReady = 0;
}
