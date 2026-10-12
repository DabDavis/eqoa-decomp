// VIRadialFloraSystem -- SUPPORT 0x0045E000 .. 0x0045F7A0, its own TU.
//
// The 3x3 swatch grid of ground flora.  Only the frame around the two big
// functions is written here: the constructor, the two teardowns, the single
// release, the zone lookup and the sort predicate.  Update (282 instructions),
// Render, DrawSwatch, CreateSwatch, CalcRadialLoc and CreateFloraInsts (552)
// are not attempted.
#include "vi.h"

VIRadialFloraSystem::VIRadialFloraSystem()
{
    // NOT field order.  Phase 16's hoist puts the source's last zero store
    // first, and written scene/raster/collide/zone/grid the block comes out 7
    // of 21 words wrong.  Three orderings are byte-identical -- this one, and
    // the two that keep the grid ahead of the pointers -- so the rule is
    // under-determined here; of the six orderings of the three groups, three
    // are byte-identical and three are 7 wrong, and what the three agree on is
    // that the pointers do not come FIRST -- which is where the field list
    // would put them.
    m_zone = -1;
    m_row = 0;
    m_row2 = 0;
    m_col = 0;
    m_col2 = 0;
    m_scene = 0;
    m_raster = 0;
    m_collide = 0;
    for (int i = 0; i < 9; i++)
        m_swatches[i] = 0;
}

int VIRadialFloraSystem::Init(VIScene &sc)
{
    if (Clear() < 0)
        return -1;

    m_scene = &sc;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_rebuild = 1;
    return 0;
}

int VIRadialFloraSystem::Clear()
{
    m_scene = 0;
    m_raster = 0;
    m_collide = 0;

    for (int i = 0; i < 9; i++)
    {
        VIRadialFloraSwatch *s = m_swatches[i];
        if (s)
        {
            delete s;
            m_swatches[i] = 0;
        }
    }
    m_numSorted = 0;
    return 0;
}

void VIRadialFloraSystem::Reset()
{
    for (int i = 0; i < 9; i++)
    {
        VIRadialFloraSwatch *s = m_swatches[i];
        if (s)
        {
            delete s;
            m_swatches[i] = 0;
        }
    }
    m_numSorted = 0;
    m_rebuild = 1;
}

int VIRadialFloraSystem::ReleaseSwatch(int i)
{
    if (m_swatches[i])
        delete m_swatches[i];
    m_swatches[i] = 0;
    return 0;
}

int VIRadialFloraSystem::CalcZoneIndex(VIRadialFloraLoc &loc)
{
    VIVect3 v;
    int     n;

    m_scene->GetWorldGridCenter(loc.m_col, loc.m_row, v);
    m_scene->m_world.FindLeaf(v, loc.m_zone, n);
    return 0;
}

int VIRadialFloraSystem::QSortCompare(const void *a, const void *b)
{
    if (*(float *)a < *(float *)b)
        return -1;
    if (*(float *)a == *(float *)b)
        return 0;
    return 1;
}

// 0x0045F798 -- and it is the whole function.  Non-zero rejects, so the radial
// flora walk never collides an actor at all: it reaches rooms and their own
// geometry and nothing else.
int VIRadialFloraFilter::FilterActor(VIScene &scene, int actor)
{
    return 1;
}
