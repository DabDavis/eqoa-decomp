#include "vi.h"

// SUPPORT 0x0049D650 .. 0x0049D918.  The tracker-module resource.

VIXm::VIXm()
{
    m_xm = 0;
    m_sample = 0;
    m_spuAddr = 0;
    m_xmAddr = 0;
    m_repeat = 0;
    m_type = XM;
}

VIXm::~VIXm()
{
    Destroy();
}

int VIXm::Create(VISoundDevice &dev, void *xm, int xmSize, void *sample, int spuSize)
{
    m_device = &dev;
    m_xm = (VIXmHeader *)xm;
    m_xmSize = xmSize;
    m_sample = sample;
    m_spuSize = spuSize;

    m_spuAddr = dev.AllocSpuMem(spuSize);
    if (!m_spuAddr)
    {
        Destroy();
        return -1;
    }
    m_xmAddr = m_device->AllocXmMem(m_xmSize);
    if (!m_xmAddr)
    {
        Destroy();
        return -1;
    }
    m_sampleCached = 0;
    m_xmCached = 0;
    return 0;
}

void VIXm::Destroy()
{
    if (m_xm)
        delete [] (char *)m_xm;
    if (m_sample)
        delete [] (char *)m_sample;
    if (m_xmAddr)
        m_device->FreeXmMem(m_xmAddr, m_xmSize);
    if (m_spuAddr)
        m_device->FreeSpuMem(m_spuAddr, m_spuSize);
}

float VIXm::Volume() const
{
    return m_volume;
}

void VIXm::SetVolume(float v)
{
    m_volume = v;
}

float VIXm::Pan() const
{
    return m_pan;
}

void VIXm::SetPan(float p)
{
    m_pan = p;
}

int VIXm::IsCached()
{
    if (m_sampleCached)
    {
        if (m_xmCached)
            return 1;
    }
    return 0;
}

void VIXm::SetRepeat(int r)
{
    m_repeat = r;
}

int VIXm::ChannelCount() const
{
    if (m_xm)
        return m_xm->m_channelCount;
    return 0;
}
