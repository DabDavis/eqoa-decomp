// VISceneFilter -- SUPPORT 0x00470820 .. 0x00470900.
#include "vi.h"

VISceneFilter::VISceneFilter()
{
    Init();
}

int VISceneFilter::Init()
{
    m_actor = -1;
    m_hitActor = -1;
    m_hitSprite = -1;
    m_hitAttachment = -1;
    return 0;
}

int VISceneFilter::Init(int actor)
{
    m_actor = actor;
    m_hitActor = -1;
    m_hitSprite = -1;
    m_hitAttachment = -1;
    return 0;
}

int VISceneFilter::FilterActor(VIScene &scene, int actor)
{
    return actor == m_actor;
}

int VISceneFilter::GetActor()
{
    return m_hitActor;
}

int VISceneFilter::GetSprite()
{
    return m_hitSprite;
}

int VISceneFilter::GetAttachment()
{
    return m_hitAttachment;
}

