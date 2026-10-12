// bandwidth.cpp -- SUPPORT 0x004aeb80..0x004aef90, the DRDP rate limiter.
//
// Two mechanisms in one struct.  The first is a leaky bucket: `m_fQueueSize`
// is how many bytes the route still owes the wire, `bandwidth_process` drains
// it at `m_fBpsLimit` bytes a second, and `get_flush_time` asks when it drains.
// The file names its own fields: three of the asserts below are quoted out of
// the relocated dump by `eqcalls.py`, and they say `pBandwidth->m_iBytesSent-
// SinceLastSample`, `TimeSinceLastSample` and `DRDP_INT64_MAX`.  Their
// __LINE__ is an immediate (phase 27), so the four of them have to land on
// lines 42, 43, 44 and 45 -- which is what the spacing in this file is for.
#include "drdp.h"

void bandwidth_init(bandwidth_t *pBandwidth, int iBpsLimit, int iQueueSizeMax)
{
    pBandwidth->m_fBpsLimit = (float)iBpsLimit;
    pBandwidth->m_fQueueSize = 0;
    pBandwidth->m_fQueueSizeMax = (float)iQueueSizeMax;
    pBandwidth->m_uiLastProcessTime = 0;
    pBandwidth->m_uiLastSampleTime = 0;
    pBandwidth->m_iBpsSent = 0;
    pBandwidth->m_iBpsRecv = 0;
    pBandwidth->m_iBytesSentSinceLastSample = 0;
    pBandwidth->m_iBytesRecvSinceLastSample = 0;
}

// The subtraction is UNSIGNED, so a wrapped millisecond clock still gives the
// elapsed time -- and the conversion to float is gcc's unsigned expansion, the
// halve-round-double sequence, not a plain `cvt.s.w`.
void bandwidth_process(bandwidth_t *pBandwidth, uns32 uiTime)
{
    float fSeconds = (float)(uns32)(uiTime - pBandwidth->m_uiLastProcessTime) * 0.001f;

    pBandwidth->m_fQueueSize -= pBandwidth->m_fBpsLimit * fSeconds;
    if (pBandwidth->m_fQueueSize < 0.0f)
        pBandwidth->m_fQueueSize = 0.0f;
    pBandwidth->m_uiLastProcessTime = uiTime;

    uns32 TimeSinceLastSample = uiTime - pBandwidth->m_uiLastSampleTime;

    if (TimeSinceLastSample > 1000)
    {
        VASSERT(pBandwidth->m_iBytesSentSinceLastSample < DRDP_INT64_MAX / 1000);
        VASSERT(pBandwidth->m_iBytesRecvSinceLastSample < DRDP_INT64_MAX / 1000);
        VASSERT(( 1000 * pBandwidth->m_iBytesSentSinceLastSample ) / TimeSinceLastSample < DRDP_INT_MAX);
        VASSERT(( 1000 * pBandwidth->m_iBytesRecvSinceLastSample ) / TimeSinceLastSample < DRDP_INT_MAX);
        pBandwidth->m_iBpsSent = (int)((1000 * pBandwidth->m_iBytesSentSinceLastSample) / TimeSinceLastSample);
        pBandwidth->m_iBpsRecv = (int)((1000 * pBandwidth->m_iBytesRecvSinceLastSample) / TimeSinceLastSample);
        pBandwidth->m_uiLastSampleTime = uiTime;
        pBandwidth->m_iBytesRecvSinceLastSample = 0;
        pBandwidth->m_iBytesSentSinceLastSample = 0;
    }
}

void bandwidth_send(bandwidth_t *pBandwidth, uns32 uiTime, int iSize)
{
    bandwidth_process(pBandwidth, uiTime);
    pBandwidth->m_fQueueSize += (float)iSize;
    pBandwidth->m_iBytesSentSinceLastSample += iSize;
}

void bandwidth_recv(bandwidth_t *pBandwidth, uns32 uiTime, int iSize)
{
    bandwidth_process(pBandwidth, uiTime);
    pBandwidth->m_iBytesRecvSinceLastSample += iSize;
}

// How long the part of the queue that is OVER the cap takes to drain.  The
// cast is unsigned, which is a libcall (`fptoui`) and not a `cvt.w.s` -- phase
// 39's lever, and the only reason this function calls anything at all.
uns32 bandwidth_get_flush_time(bandwidth_t *pBandwidth, uns32 uiTime)
{
    bandwidth_process(pBandwidth, uiTime);

    float fOverflow = pBandwidth->m_fQueueSize - pBandwidth->m_fQueueSizeMax;

    if (fOverflow < 0.0f)
        fOverflow = 0.0f;
    return uiTime + (uns32)(fOverflow * 1000.0f / pBandwidth->m_fBpsLimit);
}

void bandwidth_set_queue_size_max(bandwidth_t *pBandwidth, int iQueueSizeMax)
{
    pBandwidth->m_fQueueSizeMax = (float)iQueueSizeMax;
}

int bandwidth_get_queue_size_max(bandwidth_t *pBandwidth)
{
    return (int)pBandwidth->m_fQueueSizeMax;
}

// Seconds, not milliseconds: a full queue at the limit takes this long to
// drain, so it is the longest a caller may leave between two `process` calls.
float bandwidth_get_process_interval_max(bandwidth_t *pBandwidth)
{
    return pBandwidth->m_fQueueSizeMax / pBandwidth->m_fBpsLimit;
}

void bandwidth_set_bps_limit(bandwidth_t *pBandwidth, uns32 uiTime, int iBpsLimit)
{
    bandwidth_process(pBandwidth, uiTime);
    pBandwidth->m_fBpsLimit = (float)iBpsLimit;
}

int bandwidth_get_bps_limit(bandwidth_t *pBandwidth)
{
    return (int)pBandwidth->m_fBpsLimit;
}

int bandwidth_get_bps_sent(bandwidth_t *pBandwidth)
{
    return pBandwidth->m_iBpsSent;
}

int bandwidth_get_bps_recv(bandwidth_t *pBandwidth)
{
    return pBandwidth->m_iBpsRecv;
}
