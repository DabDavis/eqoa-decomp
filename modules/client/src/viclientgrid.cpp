// VIMaskIndex (cl_parts2.h), CLIENT 0x00BF9CA8 .. 0x00BF9F70, then its VIPool<record *>
// copies to 0x00BFA450.  Unreferenced in the image.  The file name is ours.
#include "game.h"

// 0x00BF9CA8
VIMaskIndex::VIMaskIndex(void *owner)
{
    m_owner = owner;
}

// 0x00BF9D48
VIMaskIndex::~VIMaskIndex()
{
}

// 0x00BF9DD0: every pool emptied, then each record filed under every (j, k) its two
// masks both have.
int VIMaskIndex::Build(VIPtr_BFA1F0 *recs, int n)
{
    int i, j, k;
    for (j = 0; j < 10; j++)
        for (k = 0; k < 15; k++)
            m_pools[j][k].Init(0);
    for (i = 0; i < n; i++)
    {
        VIPtr_BFA1F0 *r = &recs[i];
        for (j = 0; j < 10; j++)
        {
            int in = r->m_mask20 & (1 << j);
            for (k = 0; k < 15; k++)
            {
                int on = r->m_mask24 & (1 << k);
                if (in && on)
                    m_pools[j][k].Add(r);
            }
        }
    }
    return 0;
}

// 0x00BF9F30
VIPool<VIPtr_BFA1F0 *> *VIMaskIndex::Get(unsigned int j, int k)
{
    VIPool<VIPtr_BFA1F0 *> *p = 0;
    if (j < 10)
        if (k >= 0)
            if (k < 15)
                p = &m_pools[j][k];
    return p;
}
