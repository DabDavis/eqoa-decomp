// VIZoneTree -- SUPPORT 0x00499EC0 .. 0x00499FE8.
#include "vi.h"

VIZoneTree::VIZoneTree()
{
}

VIZoneTree::~VIZoneTree()
{
    Clear();
}

int VIZoneTree::Init(int numNodes, int numLeaves)
{
    if (m_nodes.Init(numNodes) < 0)
        return -1;

    if (m_leaves.Init(numLeaves) < 0)
        return -1;

    return 0;
}

int VIZoneTree::Clear()
{
    if (m_nodes.Clear() < 0)
        return -1;

    if (m_leaves.Clear() < 0)
        return -1;

    return 0;
}
