// VIActorFilter -- SUPPORT 0x004167D8 .. 0x00416880.
#include "vi.h"

VIActorFilter::VIActorFilter()
{
    Init();
}

int VIActorFilter::Init()
{
    m_actor = -1;
    return 0;
}

int VIActorFilter::Init(int actor)
{
    m_actor = actor;
    return 0;
}

int VIActorFilter::FilterActor(VIScene &scene, int actor)
{
    return actor == m_actor;
}

// WeatherFilter::FilterActor is at 0x00416880, the word before VIAtmosphere's
// constructor -- and `__tf13WeatherFilter` is at 0x0041C7A8, the word after
// ProcessLavastorm.  The class belongs to viatmosphere.cpp, not here.
