// VISceneQuery -- SUPPORT 0x00470900 .. 0x00471150.
#include "vi.h"

int VISceneQuery::Init()
{
    m_maxObjects = 0;
    return 0;
}

int VISceneQuery::Init(int n)
{
    m_maxObjects = n;

    if (m_objects.Init(n) < 0)
        return -1;

    if (m_sorted.Init(m_maxObjects + 1) < 0)
        return -1;

    return 0;
}

int VISceneQuery::Clear()
{
    if (m_objects.Clear() < 0)
        return -1;

    if (m_sorted.Clear() < 0)
        return -1;

    return 0;
}

int VISceneQuery::FilterActor(VIScene &scene, int actor)
{
    return 0;
}

int VISceneQuery::EndQuery()
{
    for (int i = m_sorted.Begin(); i != m_sorted.End(); i = m_sorted.Next(i))
        m_objects.Add(m_sorted.Value(i));

    return 0;
}

int VISceneQuery::NumObjects()
{
    return m_objects.m_size;
}

int VISceneQuery::Object(int i)
{
    // Base-first `addu`, which the subscript cannot give: the index has to be
    // folded into the POINTER's initialiser (Phase 3/24).
    int *p = m_objects.m_data + i;

    return *p;
}
