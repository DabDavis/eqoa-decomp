#line 1 "C:/code-frontiers-gm/game/VITextSet.cpp"
// VITextSet (name ours) -- CLIENT 0x00C0B350 .. 0x00C0B880: the quick-chat menu tree, a
// static table of 256 nodes (game.h).  No .rodata of its own; the table and a word after it
// are .bss (0x00CF5048, 0x00CF5448).
#include "game.h"

static VITextSet *sSets[256];          // 0x00CF5048
static int        sPending;            // 0x00CF5448: only Reset touches it

// 0x00C0B350.
void VITextSet::Reset()
{
    for (int i = 0; i < 256; i++)
        sSets[i] = 0;
    sPending = 0;
}

// 0x00C0B388.
void VITextSet::FreeAll()
{
    for (int i = 0; i < 256; i++)
    {
        if (sSets[i])
            delete sSets[i];
        sSets[i] = 0;
    }
}

// 0x00C0B460.
VITextSet **VITextSet::GetTable()
{
    return sSets;
}

// 0x00C0B470.
VITextSet *VITextSet::GetRoot()
{
    return sSets[0];
}

// 0x00C0B480.
VITextSet *VITextSet::Child(int k)
{
    if ((m_id & 3) == 3)
        return 0;
    return sSets[ChildId(k)];
}

// 0x00C0B4D0.  The terms in this order, and ONE return: with a return per case the last
// two blocks swap, and in any other order gcc re-associates the depth onto a<<2 (3 words).
int VITextSet::MakeId(int a, int b, int c)
{
    int id;

    if (a == -1)
        id = 0;
    else if (b == -1)
        id = (a << 2) | 1;
    else if (c == -1)
        id = (a << 2) | 2 | (b << 4);
    else
        id = (a << 2) | 3 | (b << 4) | (c << 6);
    return id;
}

// 0x00C0B528.  `& ~3` on the promoted byte is the target's andi 0xff + andi 0xfc.
int VITextSet::ChildId(int k)
{
    int depth = (m_id & 3) + 1;
    return (m_id & ~3) | depth | (k << (depth * 2));
}

// 0x00C0B550.  Tested through the table, then a reference for the store; the byte is
// converted before the `new` (its andi fills new's delay slot), and the three zero
// stores are written 2, 3, 1 -- the last store of a constant goes first.
VITextSet *VITextSet::Get(int id)
{
    if (!sSets[id])
    {
        unsigned char b = id;
        VITextSet *&s = sSets[id];
        s = new VITextSet;
        s->m_id = b;
        s->m_p02 = 0;
        s->m_p03 = 0;
        s->m_p01 = 0;
    }
    return sSets[id];
}

// 0x00C0B628: Get's body again, for a child's id.
VITextSet *VITextSet::GetChild(int k)
{
    int id = ChildId(k);

    if (!sSets[id])
    {
        unsigned char b = id;
        VITextSet *&s = sSets[id];
        s = new VITextSet;
        s->m_id = b;
        s->m_p02 = 0;
        s->m_p03 = 0;
        s->m_p01 = 0;
    }
    return sSets[id];
}

// 0x00C0B708.
void VITextSet::Mark(int deep)
{
    VITextSet *c;

    if ((c = Child(0)) && deep)
        c->Mark(1);
    if ((c = Child(1)) && deep)
        c->Mark(1);
    if ((c = Child(2)) && deep)
        c->Mark(1);
    if ((c = Child(3)) && deep)
        c->Mark(1);
    m_p01 = 2;
}

// 0x00C0B7C8.
void VITextSet::Remove()
{
    if (sSets[m_id] == this)
    {
        sSets[m_id] = 0;
        delete this;
    }
}

// 0x00C0B870.
VITextPair::VITextPair()
{
    m_p00 = 0;
    m_p04 = 0;
}
