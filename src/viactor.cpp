// VIActor -- SUPPORT 0x00415CD8 .. 0x00415E08.  One placed thing in a scene:
// its transform, the pose that builds it, and the sprite and collision indices
// that hang off it.  The whole class is three functions.
#include "vi.h"

VIActor::VIActor()
{
    m_p40 = 0;
    m_p60 = -1;
    m_p64 = 0;
    m_p68 = -1;
    m_p80 = 0;
    m_p84 = 0;
    m_p98 = -1;
    m_p9C = -1;
}

VIActor::~VIActor()
{
}

void VIActor::CalcObjectToWorld()
{
    m_objectToWorld.Transform(m_hpr, m_scale, m_location);
}
