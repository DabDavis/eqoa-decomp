// VIClipper -- SUPPORT 0x0041E370 .. 0x0041E408.
#include "vi.h"

int VIClipper::Init(int n)
{
    if (m_in.Init(n) < 0)
        return -1;

    if (m_out.Init(n) < 0)
        return -1;

    return 0;
}

int VIClipper::Clear()
{
    if (m_in.Clear() < 0)
        return -1;

    if (m_out.Clear() < 0)
        return -1;

    return 0;
}
