// VIWorldTree -- SUPPORT 0x00493308 .. 0x00493438.
#include "vi.h"

VIWorldTree::VIWorldTree()
{
    m_state = 1;
}

VIWorldTree::~VIWorldTree()
{
    Clear();
}

int VIWorldTree::Init(int numNodes, int numLeaves)
{
    if (m_nodes.Init(numNodes) < 0)
        return -1;

    if (m_leaves.Init(numLeaves) < 0)
        return -1;

    return 0;
}

int VIWorldTree::Clear()
{
    if (m_nodes.Clear() < 0)
        return -1;

    if (m_leaves.Clear() < 0)
        return -1;

    return 0;
}
