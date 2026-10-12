#include "vi.h"

// SUPPORT 0x0049D288 .. 0x0049D650.  The ADPCM sample resource.

VIWave::VIWave()
{
    m_buffer = 0;
    m_type = WAVE;
}

VIWave::~VIWave()
{
    Destroy();
}

int VIWave::Create(VISoundDevice &dev, int channels, int format, int frames, int loop)
{
    int c = channels > 0 ? channels : 1;

    m_channelCount = c < 3 ? c : 2;
    m_format = 1;
    m_sampleSize = 4;
    m_sampleFrameCount = frames;
    m_loop = loop;
    m_device = &dev;

    SetVolume(1.0f);
    SetPan(0.0f);

    if (Allocate(dev) < 0)
        return -1;
    m_cached = 0;
    return 0;
}

int VIWave::Destroy()
{
    if (Deallocate() < 0)
        return -1;
    return 0;
}

int VIWave::Format() const
{
    return m_format;
}

int VIWave::ChannelCount() const
{
    return m_channelCount;
}

int VIWave::SampleSize() const
{
    return m_sampleSize;
}

int VIWave::SampleFrameCount() const
{
    return m_sampleFrameCount;
}

int VIWave::LockBuffer(unsigned char **p)
{
    if (!m_buffer)
        return -1;
    *p = m_buffer;
    return 0;
}

int VIWave::UnlockBuffer()
{
    if (!m_buffer)
        return -1;
    m_cached = 0;
    return 0;
}

float VIWave::Volume() const
{
    return m_volume;
}

void VIWave::SetVolume(float v)
{
    m_volume = v;
}

float VIWave::Pan() const
{
    return m_pan;
}

void VIWave::SetPan(float p)
{
    m_pan = p;
}

int VIWave::IsCached()
{
    return m_cached != 0;
}

int VIWave::SampleRate() const
{
    return m_sampleRate;
}

void VIWave::SetSampleRate(int rate)
{
    m_sampleRate = rate;
}

// 28 ADPCM samples per 16-byte block, one block of leader and one more when
// the sample loops.  The divisor is in a REGISTER and the division is a real
// `div` with the assembler's divide-by-zero trap in front of it.
int VIWave::Allocate(VISoundDevice &dev)
{
    int blocks = (m_sampleFrameCount + 27) / 28 + 1 + (m_loop != 0);

    int bytes = m_channelCount * 16;

    m_size = blocks * bytes;
    m_spuAddr = dev.AllocSpuMem(m_size);
    if (!m_spuAddr)
        return -1;

    m_buffer = (unsigned char *)VIMemAlloc(m_size);
    m_p68 = 0;
    m_p80 = 0;
    m_p76 = 0;
    m_p84 = 1;
    return 0;
}

// Returns -1 on every path -- and VIWave::Destroy() tests it, so Destroy()
// always reports failure too.  Matched as written.
int VIWave::Deallocate()
{
    if (m_buffer)
        VIMemFree(m_buffer);
    if (m_spuAddr)
        m_device->FreeSpuMem(m_spuAddr, m_size);
    m_buffer = 0;
    return -1;
}
