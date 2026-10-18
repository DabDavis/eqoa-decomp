// The XM module in IOP memory: SNDDRV 0x87F8 .. 0x8CB8.
//
// A module arrives as one blob of offsets (VISoundDevice's BeginXmTransfer)
// and is never constructed: Relocate turns its offsets into pointers in place
// once both it and its ADPCM have arrived (command 7).  The constructor and
// destructor are compiled but nothing calls them.
#include "xm.h"

XmModule::XmModule()
{
    m_18 = 0;
}

XmModule::~XmModule()
{
}

// Patterns' rows at +0x891C, instruments' sample lists into m_sample[], and
// each sample's SPU address offset by where its ADPCM went; lengths and loop
// points become bytes: 28 samples to a 16-byte ADPCM block.
int XmModule::Relocate(u32 spuAddr, int flags)
{
    int i;
    int j;

    for (i = 0; i < m_patterns; i++)
    {
        XmPattern *p = &m_pattern[i];
        p->m_data = (u8 *)this + (*(int *)&p->m_data + 0x891c);
    }
    for (i = 0; i < m_instruments; i++)
    {
        XmInstrument *in = &m_instrument[i];
        in->m_sample = in->m_firstSample == -1 ? 0 : &m_sample[in->m_firstSample];
    }
    for (i = 0; i < m_instruments; i++)
    {
        for (j = 0; j < m_instrument[i].m_samples; j++)
        {
            XmSample *s = &m_instrument[i].m_sample[j];
            u32 spu = s->m_spuAddr;
            int len = s->m_length;
            int ls = s->m_loopStart;
            int le = s->m_loopEnd;

            if (len)
                s->m_spuAddr = spuAddr + spu;
            else
                s->m_spuAddr = 0;
            s->m_length = (len - ((s->m_flags & 3) ? 1 : 2)) * 28;
            if (ls == -1)
                s->m_loopStart = -1;
            else
                s->m_loopStart = (ls - 1) * 28;
            if (le == -1)
                s->m_loopEnd = -1;
            else
                s->m_loopEnd = (le - 1) * 28;
        }
    }
    return 0;
}

void XmModule::Nop()
{
}

int XmModule::Channels()
{
    return m_channels;
}
