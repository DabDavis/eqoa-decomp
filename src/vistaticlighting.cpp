// VIStaticLighting -- SUPPORT 0x0047EDC0 .. 0x0047EF18.
#include "vi.h"

int VIStaticLighting::Init(int n)
{
    if (m_buffers.Init(n) < 0)
        return -1;

    return 0;
}

int VIStaticLighting::Clear(VIRaster &r, VIDictionary &d)
{
    return ReleaseResources(r, d);
}

int VIStaticLighting::ReleaseResources(VIRaster &r, VIDictionary &d)
{
    for (int i = 0; i < m_buffers.m_size; i++)
    {
        // Both halves of the pointer lever: the ARRAY named through a pointer
        // (Phase 9), declared INSIDE the loop so its address is computed after
        // the guard, and the ELEMENT through a second one with the index folded
        // into the initialiser, which is what makes the `addu` base-first.
        VIArray<int> *p = &m_buffers;
        int *e = p->m_data + i;

        r.ReleaseColorBuffer(*e, d);
    }

    return 0;
}

// 0x0047EE98.  ReleaseResources' walk, taking a reference instead.
int VIStaticLighting::ShareResources(VIRaster &r)
{
    for (int i = 0; i < m_buffers.m_size; i++)
    {
        VIArray<int> *p = &m_buffers;
        int *e = p->m_data + i;

        r.ShareColorBuffer(*e);
    }

    return 0;
}
