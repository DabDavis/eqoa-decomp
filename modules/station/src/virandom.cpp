// virandom.cpp (name ours) -- STATION 0x002E5CB8 .. 0x002E60A8: Shawn Cokus's
// MT19937 ("cokus.c": seedMT, reloadMT, randomMT), its three globals made
// members, plus three ways to read it.  Nothing in STATION constructs one.
#include "station.h"

#define N              (624)
#define M              (397)
#define K              (0x9908B0DFU)
#define hiBit(u)       ((u) & 0x80000000U)
#define loBit(u)       ((u) & 0x00000001U)
#define loBits(u)      ((u) & 0x7FFFFFFFU)
#define mixBits(u, v)  (hiBit(u)|loBits(v))

// 0x002E5CB8.
VIRandomMT::VIRandomMT()
{
    m_left = -1;
}

// 0x002E5CC8.
void VIRandomMT::Seed(unsigned int seed)
{
    register unsigned int x = (seed | 1U) & 0xFFFFFFFFU, *s = m_state;
    register int j;

    for (m_left = 0, *s++ = x, j = N; --j;
         *s++ = (x *= 69069U) & 0xFFFFFFFFU);
}

// 0x002E5D10.
unsigned int VIRandomMT::Reload()
{
    register unsigned int *p0 = m_state, *p2 = m_state + 2, *pM = m_state + M, s0, s1;
    register int j;

    if (m_left < -1)
        Seed(4357U);

    m_left = N - 1, m_next = m_state + 1;

    for (s0 = m_state[0], s1 = m_state[1], j = N - M + 1; --j; s0 = s1, s1 = *p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    for (pM = m_state, j = M; --j; s0 = s1, s1 = *p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    s1 = m_state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
    s1 ^= (s1 >> 11);
    s1 ^= (s1 << 7) & 0x9D2C5680U;
    s1 ^= (s1 << 15) & 0xEFC60000U;
    return (s1 ^ (s1 >> 18));
}

// 0x002E5EC0.
unsigned int VIRandomMT::Random()
{
    unsigned int y;

    if (--m_left < 0)
        return (Reload());

    y  = *m_next++;
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;
    return (y ^ (y >> 18));
}

// 0x002E5F38.
float VIRandomMT::RandomFloat()
{
    unsigned int r = Random();
    r >>= 8;
    return r / 16777216.0f;
}

// 0x002E5F98.
double VIRandomMT::RandomDouble()
{
    double d = Random();
    d *= 2097152.0;
    d += Random() & 0x1FFFFF;
    return d * (1.0 / 9007199254740992.0);
}

// 0x002E6058.
int VIRandomMT::RandomRange(int lo, int hi)
{
    unsigned int range = hi - lo;
    unsigned long r = Random();
    return (int)((r * range) >> 32) + lo;
}
