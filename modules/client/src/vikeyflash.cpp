#line 1 "C:/code-frontiers-gm/game/VIKeyFlash.cpp"
// VIKeyFlash (name ours) -- CLIENT 0x00C0B010 .. 0x00C0B240: the stat-name lookup and the
// pad-key auto-repeat tracker most windows embed.  The TU's data is the database
// table-name list every game TU carries, the stat table and bad_alloc (0x00CDFF40 ..
// 0x00CE022A).
#include "game.h"

// 0x00CE0188: a stat id and the WORDS.TXT id of its abbreviation ("Str", "Sta", ...,
// "Fish"), 19 entries.
static VIWordId sStatWords[] =
{
    { 226,  0 },    // Str
    { 227,  1 },    // Sta
    { 228,  2 },    // Agi
    { 229,  3 },    // Dex
    { 230,  4 },    // Wis
    { 231,  5 },    // Int
    { 232,  6 },    // Cha
    { 234,  8 },    // HPMax
    { 496, 10 },    // POW
    { 240, 12 },    // HoT
    { 241, 13 },    // PoT
    { 242, 14 },    // AC
    { 447, 24 },    // FR
    { 451, 25 },    // CR
    { 450, 26 },    // LR
    { 452, 27 },    // AR
    { 449, 23 },    // DR
    { 448, 22 },    // PR
    { 1203, 28 },   // Fish
};

// 0x00C0B010.  The LAST entry for the stat (no break).
VIWordId *FindStatWord(int stat)
{
    VIWordId *found = 0;

    for (int i = 0; i < sizeof(sStatWords) / sizeof(sStatWords[0]); i++)
        if (sStatWords[i].m_id == stat)
            found = &sStatWords[i];
    return found;
}

// 0x00C0B048.
VIKeyFlash::VIKeyFlash()
{
    Reset();
}

// 0x00C0B070.
VIKeyFlash::~VIKeyFlash()
{
}

// 0x00C0B098.
void VIKeyFlash::Reset()
{
    m_count = 0;
    for (int i = 0; i < 16; i++)
        m_slots[i].m_key = 0;
}

// 0x00C0B0D0: the key's slot, or the first free one, or a new one.
void VIKeyFlash::Press(unsigned char key)
{
    VIKeyFlashSlot *s = 0;

    for (int i = 0; i < 16; i++)
    {
        if (m_slots[i].m_key == key || m_slots[i].m_key == 0)
        {
            s = &m_slots[i];
            break;
        }
    }
    if (!s && m_count < 16)
        s = &m_slots[m_count++];
    if (s)
    {
        s->m_key = key;
        s->m_time = 16;
        s->m_wait = 0;
    }
}

// 0x00C0B160.  `break`, not `return`: with return the loop is rotated and strength-reduced
// (16 words); with break it keeps its test on top and slot 0 checked before it.
void VIKeyFlash::Release(unsigned char key)
{
    for (int i = 0; i < 16; i++)
    {
        if (m_slots[i].m_key == key)
        {
            m_slots[i].m_key = 0;
            break;
        }
    }
}

// 0x00C0B1B8: 1 when the held key should fire now.  The first pulse fires at once, the
// next after 15 frames, then 8, 7, ... down to every frame.  The slot is tested through
// the index and only THEN named: `s` is a second pseudo, which is the target's
// `move a1,a0` for the store after the if/else join.
int VIKeyFlash::Pulse(unsigned char key)
{
    for (int i = 0; i < 16; i++)
    {
        if (m_slots[i].m_key == key)
        {
            VIKeyFlashSlot *s = &m_slots[i];
            if (!s->m_wait)
            {
                if (s->m_time == 15)
                    s->m_time = 8;
                else
                {
                    int t = s->m_time - 1;
                    if (t < 1)
                        t = 1;
                    s->m_time = t;
                }
                s->m_wait = s->m_time;
                return 1;
            }
            s->m_wait--;
            break;
        }
    }
    return 0;
}
