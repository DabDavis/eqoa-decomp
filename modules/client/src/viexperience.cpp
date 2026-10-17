// Experience and angle helpers: CLIENT 0x00CC68D8 .. 0x00CC6EB0, one TU (file name ours;
// it emits no template copies). Only ExpForLevel (a window) and
// WrapAngle (VIClient's 0x40 encoder) are called; the group-experience half is dead.
#include "game.h"

#define PI      3.14159265358979323846f
#define TWOPI   (2.0f * PI)

// 0x00CC68D8: the experience a level needs.
unsigned int ExpForLevel(int level)
{
    if (level >= 51)
        return level * level * level * ((level - 1) * 27 + 20);
    return level * level * level * ((level - 1) * 28 + 25);
}

// 0x00CC6920: into [-PI, PI).
void WrapAngle(float &a)
{
    while (a < -PI)
        a += TWOPI;
    while (a >= PI)
        a -= TWOPI;
}

// 0x00CC69C8
void WrapAngles(VIVect3 &hpr)
{
    for (int i = 0; i < 3; i++)
        WrapAngle(hpr[i]);
}

// 0x00CC6A10
int AnglesValid(VIVect3 &hpr)
{
    for (int i = 0; i < 3; i++)
    {
        if (hpr[i] < -PI || hpr[i] >= PI)
            return 0;
    }
    return 1;
}

int MaxLevel(int n, const int *levels);
float KillExp(int mob, int n, const int *levels);

// 0x00CC6A70: one member's share of a kill.
int GroupExp(int mob, int level, int n, const int *levels)
{
    if (!n)
        return 0;
    int max = MaxLevel(n, levels);
    int limit = (int)(max * 0.75f);
    if (max - limit <= 5)
        limit = max - 6;
    if (level < limit)
        return 0;
    float exp = KillExp(mob, n, levels);
    if (exp == 0.0f)
        return 0;
    float lsq = level * level;
    float sum = 0.0f;
    for (int i = 0; i < n; i++)
        sum += levels[i] * levels[i];
    float share = lsq / sum;
    float cap = KillExp(level, 1, &level) * 3.0f;
    if (cap <= 0.0f)
        return 0;
    int e = (int)(exp * share);
    e = VIMAX(e, 0);
    return VIMIN((int)cap, e);
}

// 0x00CC6BF0
float LevelExp(int level)
{
    float base = (float)(level + 1) * (level + 1) * 40.0f;
    return level * 0.02f * base + base;
}

// 0x00CC6C38
float LevelDiffScale(int mob, int max)
{
    return (mob - max) * 0.05f + 1.0f;
}

// 0x00CC6C68
int MaxLevel(int n, const int *levels)
{
    int max = levels[0];
    for (int i = 1; i < n; i++)
    {
        if (max < levels[i])
            max = levels[i];
    }
    return max;
}

// 0x00CC6CA8
int MaxLevelPlus(int n, const int *levels)
{
    if (!n)
        return 0;
    return MaxLevel(n, levels) + n - 1;
}

// 0x00CC6CE8
float GroupScale(int n)
{
    return n * 0.8f + 0.2f;
}

// 0x00CC6D18
float KillExp(int mob, int n, const int *levels)
{
    int max = MaxLevel(n, levels);
    int r = (int)((float)max * 0.75);
    int limit = max - r > 2 ? r : max - 3;
    if (mob < limit)
        return 0.0f;
    float a = LevelExp(mob);
    float b = LevelDiffScale(mob, max);
    return a * b * GroupScale(n);
}

// 0x00CC6DF0
int ExpEligible(int level, int max)
{
    int limit = (int)(max * 0.75f);
    if (max - limit <= 5)
        limit = max - 6;
    return !(level < limit);
}

// 0x00CC6E30
unsigned int ExpCost(int level)
{
    if (level >= 6)
    {
        unsigned int e = (unsigned int)LevelExp(level);
        int k = level - 19;
        if (level <= 19)
            k = 0;
        return (k + 1) * e;
    }
    return 0;
}

// 0x00CC6E88
unsigned int ExpCost5(int level)
{
    return ExpCost(level) * 5;
}
