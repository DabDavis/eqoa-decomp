// VIEmitterQuery -- SUPPORT 0x00430A90, at the head of the TU that carries the
// VIMultiMap<float, VIEmitterEntry> instantiation it is built on.
#include "vi.h"

int VIEmitterQuery::Init(int limit, VISpriteType type)
{
    m_type = type;
    m_limit = limit;
    m_a = -1;
    m_b = -1;
    if (m_map.Init(limit) < 0)
        return -1;
    return 0;
}

int VIEmitterQuery::Clear()
{
    m_a = -1;
    m_b = -1;
    if (m_map.Clear() < 0)
        return -1;
    return 0;
}
