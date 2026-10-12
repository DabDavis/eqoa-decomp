// VIMotifBlender -- SUPPORT 0x004564E0 .. 0x004565A0.
#include "vi.h"

VIMotifBlender::VIMotifBlender()
{
    m_motif = 0;
    m_factor = 0.0f;
    m_changed = 1;
}

VIMotifBlender::~VIMotifBlender()
{
    if (m_motif)
        Remove();
}

float VIMotifBlender::Factor() const
{
    return m_factor;
}

void VIMotifBlender::SetFactor(float f)
{
    if (m_factor != f)
    {
        m_factor = f;
        m_changed = 1;
    }
}

VIParticleMotif *VIMotifBlender::Motif()
{
    return m_motif;
}
