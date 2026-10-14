// VISoundDevice -- SUPPORT 0x0049A3E0 .. 0x0049D288, its own translation unit.
//
// The EE half of the sound system.  It owns 46 playback records, 128 loaded
// VISounds, a first-fit allocator over SPU RAM and a second one over the IOP's
// XM region, one SIF DMA in flight at a time, and the BGM cross-fade.
//
// It says nothing to the IOP directly: every operation is a small command
// appended to a packet by SoundCmd() and shipped by SoundFlush() over SIF RPC,
// which is why there is not one sceSpu call in the class.  Those five helpers
// are file statics with no symbols of their own, so eqdecomp caps
// DebugResources' derived span at its real 312 bytes.
#include "vi.h"

extern "C" int    printf(const char *, ...);
extern "C" char  *strcpy(char *d, const char *s);
extern "C" int    strcmp(const char *a, const char *b);
extern "C" int    FlushCache(int mode);
extern "C" int    sceSifDmaStat(unsigned int id);
extern "C" unsigned int sceSifSetDma(void *sdd, int len);
extern "C" int    sceSifBindRpc(void *client, unsigned int number, unsigned int mode);
extern "C" int    sceSifCallRpc(void *client, unsigned int fno, unsigned int mode,
                                void *send, int ssize, void *recv, int rsize,
                                void *endfunc, void *efarg);
extern "C" int    sceSifCheckStatRpc(void *client);

// The IOP's view of the machine, DMA'd back every flush: [4]/[5] are the two
// halves of the 46-bit voice mask, [6] says whether the BGM stream is still
// running.
static unsigned int gSndStatus[128];

// The XM modules the IOP currently has playing -- eight {lo, hi} channel-mask
// pairs.  The two halves share one address base, which is what puts the whole
// search on a single walking pointer with a -4 displacement -- loop.c's, from
// an indexed source loop.
static struct VISndXmState
{
    int m_pad[8];
    int m_ch[8][2];
} gSndXm;

// One command's arguments, filled by the caller and copied into the packet.
static int gSndArgs[16];

// The packet itself: word 0 is the command count, then {id, argc, args...}.
static int gSndPacket[128];
static int gSndWords;

static int   SoundBind();
static int   SoundCall(int fno, void *send, int async);
static int   SoundFlush(int wait);
static void  SoundCmd(int id, int argc);
static int  *SoundFindCmd(int id);

int VISoundDevice::sBgmVersion;

VISoundDevice::VISoundDevice()
{
    int i;

    for (i = 0; i < 128; i++)
        m_sounds[i] = 0;
    for (i = 0; i < 46; i++)
        m_playbackSlots[i] = 0;
    m_freePlaybacks.Init();
    for (i = 0; i < 46; i++)
        m_freePlaybacks.AddTail(&m_playbacks[i]);

    m_channels = 0;
    m_frame = 0;
    m_xferRemain = 0;
    m_xferDma = 0;
    m_xferLast = 0;
    m_xferIdle = 1;
    m_bgmAddr = 0;
    m_bgmPlaying[0] = 0;
    m_handleSeed = 0x16062;
    m_xferFree = 0;
}

VISoundDevice::~VISoundDevice()
{
}

void VISoundDevice::SetBgmVersion(int v)
{
    sBgmVersion = v;
}

int VISoundDevice::BgmVersion()
{
    return sBgmVersion;
}

int VISoundDevice::Init()
{
    int i;

    if (CreateDevice() < 0)
        return -1;

    m_activePlaybacks.Init();
    m_freePlaybacks.Init();
    for (i = 0; i < 128; i++)
        m_sounds[i] = 0;
    for (i = 0; i < 46; i++)
        m_playbackSlots[i] = 0;
    for (i = 0; i < 46; i++)
        m_freePlaybacks.AddTail(&m_playbacks[i]);

    m_channels = 0;
    m_frame = 0;
    m_xferRemain = 0;
    m_xferDma = 0;
    m_xferLast = 0;
    m_xferIdle = 1;
    m_bgmAddr = 0;
    m_bgmRequest[0][0] = 0;
    m_bgmRequest[1][0] = 0;
    m_bgmPlaying[0] = 0;
    m_bgmPending[0] = 0;
    m_bgmVolume = 1.0f;
    m_bgmFadeDir = 0;
    m_bgmFade = 0.0f;
    m_handleSeed = 0x16062;
    m_stereo = 1;
    for (i = 0; i < 2; i++)
        m_classVolume[i] = 1.0f;
    m_xferFree = 0;

    m_bgmAddr = AllocSpuMem(BgmBufferSize() * 2);
    if (m_bgmAddr)
        return 0;
    return -1;
}

int VISoundDevice::Reset()
{
    int i;

    if (m_xferFree)
    {
        delete [] (char *)m_xferFree;
        m_xferFree = 0;
    }

    VIPlayback *pb = FirstPlayback();
    while (pb)
    {
        VIPlayback *next = NextPlayback(pb);
        if (Sound(pb->m_sound)->Type() == VISound::XM)
            Stop(pb->m_handle);
        pb = next;
    }

    StopAllBgm();
    StopChannels(-1);
    SoundFlush(0);

    for (i = 0; i < 128; i++)
    {
        if (m_sounds[i])
            delete m_sounds[i];
    }

    if (m_bgmAddr)
    {
        FreeSpuMem(m_bgmAddr, BgmBufferSize() * 2);
        m_bgmAddr = 0;
    }

    CloseXmMem();
    InitXmMem(m_xmBase, m_xmSize);
    CloseSpuMem();
    InitSpuMem();

    m_activePlaybacks.Init();
    m_freePlaybacks.Init();
    for (i = 0; i < 128; i++)
        m_sounds[i] = 0;
    for (i = 0; i < 46; i++)
        m_playbackSlots[i] = 0;
    for (i = 0; i < 46; i++)
        m_freePlaybacks.AddTail(&m_playbacks[i]);

    m_channels = 0;
    m_frame = 0;
    m_xferRemain = 0;
    m_xferDma = 0;
    m_xferLast = 0;
    m_xferIdle = 1;
    m_bgmAddr = 0;
    m_bgmRequest[0][0] = 0;
    m_bgmRequest[1][0] = 0;
    m_bgmPlaying[0] = 0;
    m_bgmPending[0] = 0;
    m_bgmVolume = 1.0f;
    m_bgmFadeDir = 0;
    m_bgmFade = 0.0f;
    m_stereo = 1;
    for (i = 0; i < 2; i++)
        m_classVolume[i] = 1.0f;

    m_bgmAddr = AllocSpuMem(BgmBufferSize() * 2);
    if (m_bgmAddr)
        return 0;
    return -1;
}

int VISoundDevice::Close()
{
    Reset();
    if (ReleaseDevice() < 0)
        return -1;
    return 0;
}

// One transfer is started per call and the scan stops there, so a big wave
// takes as many frames as it takes.  The 128-entry sweep restarts from zero
// every frame, which is why a sound low in the table can starve.
// OPEN, 57/189 (65 before).  The XM search is INDEXED, not a walking pointer:
// loop.c's strength reduction makes the pointer, and the giv's initial value
// is `&gSndXm + 36` built from a hoisted full address -- the target's
// `addiu v1,s4,36` -- where a source pointer hoists only the `lui`.  What is
// left is loop.c hoisting the `== XM` constant 1 into s5 on its second pass:
// the move is decided at exactly threshold (62 real insns vs 62), and the
// target's loop is evidently one insn longer there.
int VISoundDevice::Process()
{
    int started = 0;
    int i;

    unsigned long playing = gSndStatus[4] | ((unsigned long)gSndStatus[5] << 24);

    if (m_xferIdle == 0)
    {
        IdleTransfer();
    }
    else
    {
        for (i = 0; i < 128; i++)
        {
            VISound *s = m_sounds[i];
            if (s)
            {
                switch (s->Type())
                {
                case VISound::WAVE:
                    if (s->AsWave()->m_cached == 0)
                    {
                        started = 1;
                        VIWave *w = s->AsWave();
                        BeginAdpcmTransfer((void **)&w->m_buffer, w->m_spuAddr,
                                           w->m_size, &w->m_cached);
                    }
                    break;
                case VISound::XM:
                    if (s->AsXm()->m_sampleCached == 0)
                    {
                        started = 1;
                        VIXm *x = s->AsXm();
                        BeginAdpcmTransfer((void **)&x->m_sample, x->m_spuAddr,
                                           x->m_spuSize, &x->m_sampleCached);
                    }
                    else if (s->AsXm()->m_xmCached == 0)
                    {
                        started = 1;
                        VIXm *x = s->AsXm();
                        BeginXmTransfer((void **)&x->m_xm, x->m_xmAddr,
                                        x->m_spuAddr, x->m_xmSize, &x->m_xmCached);
                    }
                    break;
                }
            }
            if (started)
                break;
        }
    }

    VIPlayback *pb = FirstPlayback();
    while (pb)
    {
        VIPlayback *next = NextPlayback(pb);
        VISound *s = Sound(pb->m_sound);

        if (s->Type() == VISound::WAVE)
        {
            if ((pb->m_channels & playing) == 0)
            {
                if (m_frame - pb->m_frame >= 3)
                    Stop(pb->m_handle);
            }
        }
        if (s->Type() == VISound::XM)
        {
            int lo = (int)(pb->m_channels & 0xffffff);
            int hi = (int)(pb->m_channels >> 24);
            int found = 0;
            int j;

            j = 0;
            do {
                if (gSndXm.m_ch[j][0] == lo && gSndXm.m_ch[j][1] == hi)
                {
                    found = 1;
                    break;
                }
                j++;
            } while (j < 8);
            if (!found)
            {
                if (m_frame - pb->m_frame >= 3)
                    Stop(pb->m_handle);
            }
        }
        pb = next;
    }

    if (gSndStatus[6] == 0)
        m_bgmPlaying[0] = 0;

    StreamBgm();
    SoundCmd(3, 0);
    SoundFlush(1);
    m_frame++;
    return 0;
}

int VISoundDevice::CreateSound(VISound::TYPE type, int share)
{
    VISound *s;
    int i;

    switch (type)
    {
    case VISound::WAVE:
        s = new VIWave;
        break;
    case VISound::XM:
        s = new VIXm;
        break;
    default:
        return -1;
    }
    if (s == 0)
        return -1;

    for (i = 0; i < 128; i++)
    {
        if (m_sounds[i] == 0)
        {
            m_sounds[i] = s;
            break;
        }
    }
    if (i == 128)
    {
        delete s;
        return -1;
    }
    if (share)
        s->m_link++;
    return i;
}

VIXm *VISoundDevice::Sound(int i)
{
    if (i == -1)
        return 0;
    return (VIXm *)m_sounds[i];
}

int VISoundDevice::ShareSound(int i)
{
    VISound *s = m_sounds[i];

    if (s == 0)
        return -1;
    s->m_link++;
    return 0;
}

int VISoundDevice::ReleaseSound(int i, VIDictionary &d)
{
    VISound *s = m_sounds[i];

    if (--s->m_link <= 0)
    {
        if (s->m_id)
        {
            if (d.Remove(*s) < 0)
                return -1;
        }

        VIPlayback *pb = FirstPlayback();
        while (pb)
        {
            VIPlayback *next = NextPlayback(pb);
            if (pb->m_sound == i)
                Stop(pb->m_handle);
            pb = next;
        }

        if (s)
            delete s;
        m_sounds[i] = 0;
    }
    return 0;
}

int VISoundDevice::PlayBgm(int slot, const char *path)
{
    strcpy(m_bgmRequest[slot], path);
    return 0;
}

void VISoundDevice::StopBgm(int slot)
{
    if (m_bgmRequest[slot][0])
    {
        if (strcmp(m_bgmPlaying, m_bgmRequest[slot]) == 0)
            m_bgmFadeDir = -1;
    }
    m_bgmRequest[slot][0] = 0;
}

void VISoundDevice::StopAllBgm()
{
    m_bgmRequest[0][0] = 0;
    m_bgmRequest[1][0] = 0;
    if (m_bgmPlaying[0])
    {
        gSndArgs[0] = 0;
        SoundCmd(14, 1);
        m_bgmPlaying[0] = 0;
    }
}

int VISoundDevice::IsBgmPlaying(int slot)
{
    return m_bgmPlaying[0] != 0;
}

// The slot is ignored: every caller writes the one master BGM volume.
void VISoundDevice::SetBgmVolume(int slot, float v)
{
    m_bgmVolume = v;
}

void VISoundDevice::EnableBgmFading(int on)
{
}

// Request slot 0 wins over slot 1, and a change of request cross-fades: the
// running track is faded out first and the new one only starts once it is gone.
int VISoundDevice::StreamBgm()
{
    if (m_bgmPlaying[0])
    {
        // A guard and then a SWITCH with no case 0: the guard is the
        // `beqz`, and two leaf cases are two `beq`s straight to bodies laid
        // out after the dispatch.  An if/else-if chain inverts the tests and
        // keeps the bodies in line; a switch WITH `case 0` is a balanced tree
        // rooted at 0 and gives a `bgtz`.
        if (m_bgmFadeDir)
        {
            switch (m_bgmFadeDir)
            {
            case -1:
                m_bgmFade -= 0.05f;
                if (m_bgmFade <= 0.0f)
                {
                    m_bgmFade = 0.0f;
                    m_bgmFadeDir = 0;
                    gSndArgs[0] = 0;
                    SoundCmd(14, 1);
                    m_bgmPlaying[0] = 0;
                    return 0;
                }
                break;
            case 1:
                m_bgmFade += 0.05f;
                if (m_bgmFade >= 1.0f)
                {
                    m_bgmFade = 1.0f;
                    m_bgmFadeDir = 0;
                }
                break;
            }
        }
    }

    m_bgmPending[0] = 0;
    if (m_bgmRequest[0][0])
        strcpy(m_bgmPending, m_bgmRequest[0]);
    else if (m_bgmRequest[1][0])
        strcpy(m_bgmPending, m_bgmRequest[1]);

    if (strcmp(m_bgmPending, m_bgmPlaying))
    {
        if (m_bgmPlaying[0])
        {
            m_bgmFadeDir = -1;
        }
        else
        {
            strcpy(m_bgmPlaying, m_bgmPending);
            strcpy((char *)gSndArgs, m_bgmPlaying);
            gSndArgs[8] = m_bgmAddr;
            SoundCmd(13, 9);
            SoundFlush(0);
            m_bgmFadeDir = 0;
            m_bgmFade = 1.0f;
        }
    }

    gSndArgs[0] = (int)(m_bgmVolume * m_bgmFade * m_classVolume[1] * 16383.0f);
    SoundCmd(15, 1);
    return 0;
}

int VISoundDevice::BgmBufferSize()
{
    int n = 16384;

    if (sBgmVersion)
    {
        if (sBgmVersion == 1)
            n = 0x10000;
    }
    return n * 2;
}

// OPEN, 28/249.  The target CROSS-JUMPS the two SoundCmd calls -- the WAVE arm
// ends `b <XM arm's jal>` -- and keeps the shared `return -1` block at the very
// end, just above the epilogue.  `else if (XM) ... else return -1`, a `goto
// fail` at the bottom, and folding the first two tests into one `||` all
// leave both untouched (27-28).
int VISoundDevice::Play(int sound, int priority, VISOUNDCLASS cls)
{
    if (cls >= 2)
        return -1;
    if (sound == -1)
        return -1;

    VISound *s = Sound(sound);
    if (s == 0)
        return -1;
    if (s->IsCached() == 0)
        return -2;

    int count;
    if (s->Type() == VISound::WAVE)
        count = s->AsWave()->ChannelCount();
    else
        count = s->AsXm()->ChannelCount();

    unsigned long channels = AllocateChannels(count, priority);
    if (channels == 0)
        return -1;

    int handle = -1;
    VIPlayback *pb;
    int i;

    for (i = 0; i < 46; i++)
    {
        if (m_playbackSlots[i] == 0)
        {
            handle = i | ((m_handleSeed++ << 8) & 0x7fffff00);
            pb = NewPlayback();
            m_playbackSlots[i] = pb;
            break;
        }
    }
    if (handle == -1)
        return -1;

    pb->m_priority = priority;
    pb->m_sound = sound;
    pb->m_channels = channels;
    pb->m_handle = handle;
    pb->m_frame = m_frame;
    pb->m_class = cls;

    if (s->Type() == VISound::WAVE)
    {
        VIWave *w = s->AsWave();
        int first = FirstChannel(channels);
        int second = NextChannel(channels, first);

        pb->m_pitch = 4096;
        pb->m_volume = 1.0f;
        pb->m_pan = w->Pan();
        int pan = (int)(pb->m_pan * 16383.0f);

        gSndArgs[0] = first;
        gSndArgs[1] = w->m_spuAddr;
        gSndArgs[2] = second;
        int right;
        if (second != -1)
            right = w->m_spuAddr + (w->m_size >> 1);
        else
            right = 0;
        gSndArgs[3] = right;
        gSndArgs[4] = (int)(((long)w->m_sampleRate << 12) / 48000);
        gSndArgs[5] = (unsigned int)(pb->m_volume * w->Volume() * m_classVolume[cls] * 16383.0f);
        int p = 0;
        if (m_stereo)
            p = pan;
        gSndArgs[6] = p;
        SoundCmd(2, 7);
    }
    else
    {
        if (s->Type() != VISound::XM)
            return -1;

        VIXm *x = s->AsXm();

        pb->m_pitch = 4096;
        pb->m_volume = 1.0f;
        pb->m_pan = x->Pan();
        int pan = (int)((pb->m_pan * 0.5f + 0.5f) * 255.0f);

        gSndArgs[0] = (int)(channels & 0xffffff);
        gSndArgs[1] = (int)(channels >> 24);
        gSndArgs[2] = x->m_xmAddr;
        gSndArgs[3] = x->m_repeat;
        gSndArgs[4] = pb->m_pitch;
        gSndArgs[5] = (unsigned int)(pb->m_volume * x->Volume() * m_classVolume[cls] * 255.0f);
        if (m_stereo == 0)
            pan = 0;
        gSndArgs[6] = pan;
        SoundCmd(8, 7);
    }
    return handle;
}

int VISoundDevice::IsPlaying(int handle)
{
    return Playback(handle) != 0;
}

void VISoundDevice::Stop(int handle)
{
    VIPlayback *pb = Playback(handle);

    if (pb)
    {
        VISound *s = Sound(pb->m_sound);
        if (s)
        {
            if (s->Type() == VISound::WAVE)
            {
            }
            else if (s->Type() == VISound::XM)
            {
                gSndArgs[0] = (int)(pb->m_channels & 0xffffff);
                gSndArgs[1] = (int)(pb->m_channels >> 24);
                SoundCmd(9, 2);
            }
        }
        StopChannels(pb->m_channels);
        DeallocateChannels(pb->m_channels);
        m_playbackSlots[(pb->m_handle & 0xff) % 46] = 0;
        DeletePlayback(pb);
    }
}

int VISoundDevice::Pitch(int handle)
{
    VIPlayback *pb = Playback(handle);

    if (pb)
        return pb->m_pitch;
    return 4096;
}

void VISoundDevice::SetPitch(int handle, int pitch)
{
    VIPlayback *pb = Playback(handle);

    if (pb == 0)
        return;
    pb->m_pitch = pitch;

    VISound *s = Sound(pb->m_sound);
    if (s == 0)
        return;

    if (s->Type() == VISound::WAVE)
    {
        int rate = (s->AsWave()->SampleRate() << 12) / 48000;
        int scaled = (rate * pb->m_pitch) / 4096;

        gSndArgs[0] = FirstChannel(pb->m_channels);
        gSndArgs[1] = NextChannel(pb->m_channels, gSndArgs[0]);
        gSndArgs[2] = scaled;
        SoundCmd(5, 3);
    }
    if (s->Type() == VISound::XM)
    {
        s->AsXm();
        gSndArgs[0] = (int)(pb->m_channels & 0xffffff);
        gSndArgs[1] = (int)(pb->m_channels >> 24);
        gSndArgs[2] = pitch;
        SoundCmd(11, 3);
    }
}

float VISoundDevice::Volume(int handle)
{
    VIPlayback *pb = Playback(handle);

    if (pb)
        return pb->m_volume;
    return 0.0f;
}

void VISoundDevice::SetVolume(int handle, float v)
{
    VIPlayback *pb = Playback(handle);
    float vol = VIMIN(VIMAX(0.0f, v), 1.0f);

    if (pb == 0)
        return;
    pb->m_volume = vol;

    VISound *s = Sound(pb->m_sound);
    if (s == 0)
        return;

    if (s->Type() == VISound::WAVE)
    {
        VIWave *w = s->AsWave();
        float g = w->Volume() * vol * m_classVolume[pb->m_class];
        int pan = (int)(pb->m_pan * 16383.0f);

        gSndArgs[0] = FirstChannel(pb->m_channels);
        gSndArgs[1] = NextChannel(pb->m_channels, gSndArgs[0]);
        gSndArgs[2] = (unsigned int)(g * 16383.0f);
        if (m_stereo == 0)
            pan = 0;
        gSndArgs[3] = pan;
        SoundCmd(6, 4);
    }
    if (s->Type() == VISound::XM)
    {
        VIXm *x = s->AsXm();
        float g = x->Volume() * vol * m_classVolume[pb->m_class];
        int pan = (int)((pb->m_pan * 0.5f + 0.5f) * 255.0f);

        gSndArgs[0] = (int)(pb->m_channels & 0xffffff);
        gSndArgs[1] = (int)(pb->m_channels >> 24);
        gSndArgs[2] = (unsigned int)(g * 255.0f);
        if (m_stereo == 0)
            pan = 0;
        gSndArgs[3] = pan;
        SoundCmd(10, 4);
    }
}

float VISoundDevice::Pan(int handle)
{
    VIPlayback *pb = Playback(handle);

    if (pb)
        return pb->m_pan;
    return 0.0f;
}

void VISoundDevice::SetPan(int handle, float p)
{
    VIPlayback *pb = Playback(handle);
    float pan = VIMIN(VIMAX(-1.0f, p), 1.0f);

    if (pb == 0)
        return;
    pb->m_pan = pan;

    VISound *s = Sound(pb->m_sound);
    if (s == 0)
        return;

    if (s->Type() == VISound::WAVE)
    {
        VIWave *w = s->AsWave();
        float g = w->Volume() * pb->m_volume * m_classVolume[pb->m_class];
        int pv = (int)(pb->m_pan * 16383.0f);

        gSndArgs[0] = FirstChannel(pb->m_channels);
        gSndArgs[1] = NextChannel(pb->m_channels, gSndArgs[0]);
        gSndArgs[2] = (unsigned int)(g * 16383.0f);
        if (m_stereo == 0)
            pv = 0;
        gSndArgs[3] = pv;
        SoundCmd(6, 4);
    }
    if (s->Type() == VISound::XM)
    {
        VIXm *x = s->AsXm();
        float g = x->Volume() * pb->m_volume * m_classVolume[pb->m_class];
        int pv = (int)((pb->m_pan * 0.5f + 0.5f) * 255.0f);

        gSndArgs[0] = (int)(pb->m_channels & 0xffffff);
        gSndArgs[1] = (int)(pb->m_channels >> 24);
        gSndArgs[2] = (unsigned int)(g * 255.0f);
        if (m_stereo == 0)
            pv = 0;
        gSndArgs[3] = pv;
        SoundCmd(10, 4);
    }
}

// Runs the whole upload queue to completion, one DMA at a time.
int VISoundDevice::Flush()
{
    int done = 0;
    int i;

    while (done == 0)
    {
        if (m_xferIdle == 0)
        {
            IdleTransfer();
            SoundCmd(3, 0);
            SoundFlush(0);
        }
        else
        {
            done = 1;
            for (i = 0; i < 128; i++)
            {
                VISound *s = m_sounds[i];
                if (s)
                {
                    switch (s->Type())
                    {
                    case VISound::WAVE:
                        if (s->AsWave()->m_cached == 0)
                        {
                            done = 0;
                            VIWave *w = s->AsWave();
                            BeginAdpcmTransfer((void **)&w->m_buffer, w->m_spuAddr,
                                               w->m_size, &w->m_cached);
                        }
                        break;
                    case VISound::XM:
                        if (s->AsXm()->m_sampleCached == 0)
                        {
                            done = 0;
                            VIXm *x = s->AsXm();
                            BeginAdpcmTransfer((void **)&x->m_sample, x->m_spuAddr,
                                               x->m_spuSize, &x->m_sampleCached);
                        }
                        else if (s->AsXm()->m_xmCached == 0)
                        {
                            done = 0;
                            VIXm *x = s->AsXm();
                            BeginXmTransfer((void **)&x->m_xm, x->m_xmAddr,
                                            x->m_spuAddr, x->m_xmSize, &x->m_xmCached);
                        }
                        break;
                    }
                }
                if (done == 0)
                    break;
            }
        }
    }
    return 0;
}

void VISoundDevice::SetSoundClassVolume(VISOUNDCLASS cls, float v)
{
    m_classVolume[cls] = v;

    for (VIPlayback *pb = FirstPlayback(); pb; pb = NextPlayback(pb))
    {
        if (pb->m_class == cls)
            SetVolume(pb->m_handle, pb->m_volume);
    }
    SetBgmVolume(0, m_bgmVolume);
    SetBgmVolume(1, m_bgmVolume);
}

void VISoundDevice::SetOutputMode(int stereo)
{
    m_stereo = stereo;
    for (VIPlayback *pb = FirstPlayback(); pb; pb = NextPlayback(pb))
        SetPan(pb->m_handle, pb->m_pan);
}

int VISoundDevice::CreateDevice()
{
    InitSpuMem();
    if (SoundBind() < 0)
        return -1;
    SoundCmd(0, 0);
    SoundFlush(0);
    m_iopBuf = gSndStatus[0];
    m_xmBase = gSndStatus[2];
    m_xmSize = gSndStatus[3];
    InitXmMem(m_xmBase, m_xmSize);
    return 0;
}

int VISoundDevice::ReleaseDevice()
{
    SoundCmd(12, 0);
    SoundFlush(0);
    CloseXmMem();
    CloseSpuMem();
    return 0;
}

// Two passes over the playback list: the first only counts what could be
// stolen, the second actually stops it, so a request that cannot be met leaves
// every running sound alone.
unsigned long VISoundDevice::AllocateChannels(int count, int priority)
{
    unsigned long got = 0;
    int free = 46 - ChannelCount(m_channels);
    int need;
    int want = 0;
    VIPlayback *pb;

    if (free < count)
    {
        need = count - free;
        want = need;
        pb = FirstPlayback();
        while (pb)
        {
            if (priority >= pb->m_priority)
            {
                need -= ChannelCount(pb->m_channels);
                if (need <= 0)
                    break;
            }
            pb = NextPlayback(pb);
        }
        if (need > 0)
            return 0;

        if (free < count)
        {
            need = want;
            pb = FirstPlayback();
            while (pb)
            {
                VIPlayback *next = NextPlayback(pb);
                if (priority >= pb->m_priority)
                {
                    need -= ChannelCount(pb->m_channels);
                    Stop(pb->m_handle);
                    if (need <= 0)
                        break;
                }
                pb = next;
            }
        }
    }

    int i;
    int n = 0;

    for (i = 0; i < 46; i++)
    {
        unsigned long bit = (unsigned long)1 << i;
        if ((m_channels & bit) == 0)
        {
            got |= bit;
            n++;
            if (n == count)
                break;
        }
    }
    m_channels |= got;
    return got;
}

void VISoundDevice::DeallocateChannels(unsigned long mask)
{
    m_channels &= ~mask;
}

int VISoundDevice::ChannelCount(unsigned long mask)
{
    int n = 0;

    for (int i = 0; i < 46; i++)
        n = ((mask >> i) & 1) + n;
    return n;
}

// A stop already queued this frame is merged into rather than re-sent.
void VISoundDevice::StopChannels(unsigned long mask)
{
    int *cmd = SoundFindCmd(4);

    if (cmd)
    {
        cmd[0] |= (int)(mask & 0xffffff);
        cmd[1] |= (int)(mask >> 24);
    }
    else
    {
        gSndArgs[0] = (int)(mask & 0xffffff);
        gSndArgs[1] = (int)(mask >> 24);
        SoundCmd(4, 2);
    }
}

int VISoundDevice::FirstChannel(unsigned long mask)
{
    for (int i = 0; i < 46; i++)
    {
        int on = (mask >> i) & 1;
        if (on)
            return i;
    }
    return -1;
}

int VISoundDevice::NextChannel(unsigned long mask, int channel)
{
    for (int i = channel + 1; i < 46; i++)
    {
        int on = (mask >> i) & 1;
        if (on)
            return i;
    }
    return -1;
}

VIPlayback *VISoundDevice::NewPlayback()
{
    VIPlayback *pb = (VIPlayback *)m_freePlaybacks.RemHead();

    if (pb)
        m_activePlaybacks.AddTail(pb);
    return pb;
}

void VISoundDevice::DeletePlayback(VIPlayback *pb)
{
    pb->Remove();
    m_freePlaybacks.AddTail(pb);
}

VIPlayback *VISoundDevice::Playback(int handle)
{
    VIPlayback *pb = m_playbackSlots[(handle & 0xff) % 46];

    if (pb)
    {
        if (pb->m_handle == handle)
            return pb;
    }
    return 0;
}

VIPlayback *VISoundDevice::FirstPlayback()
{
    VIPlayback *pb = (VIPlayback *)m_activePlaybacks.m_head;

    if (pb->m_next == 0)
        return 0;
    return pb;
}

VIPlayback *VISoundDevice::NextPlayback(VIPlayback *pb)
{
    VIPlayback *n = (VIPlayback *)pb->m_next;

    if (n->m_next == 0)
        return 0;
    return n;
}

int VISoundDevice::InitSpuMem()
{
    int i;

    m_spuBlocks = new VISpuBlock[128];
    for (i = 0; i < 128; i++)
        m_spuNodes.AddTail(&m_spuBlocks[i]);

    VISpuBlock *b = (VISpuBlock *)m_spuNodes.RemHead();
    b->m_addr = 0x5040;
    b->m_size = 0x1f9fc0;
    m_spuMem.AddTail(b);
    return 0;
}

void VISoundDevice::CloseSpuMem()
{
    if (m_spuBlocks)
        delete [] m_spuBlocks;
    m_spuMem.Init();
    m_spuNodes.Init();
}

unsigned int VISoundDevice::AllocSpuMem(int size)
{
    VISpuBlock *b;
    int n = ((size + 63) / 64) * 64;

    for (b = (VISpuBlock *)m_spuMem.m_head; b->m_next; b = (VISpuBlock *)b->m_next)
    {
        if (b->m_size >= n)
        {
            unsigned int addr = b->m_addr;
            b->m_size -= n;
            b->m_addr = addr + n;
            if (b->m_size == 0)
            {
                b->Remove();
                m_spuNodes.AddTail(b);
            }
            return addr;
        }
    }
    // Shipped: the %d has no argument -- a1 is whatever the walk left there.
    printf("AllocSpuMem( %d ) request failed.\n");
    return 0;
}

void VISoundDevice::FreeSpuMem(unsigned int addr, int size)
{
    int n = ((size + 63) / 64) * 64;
    VISpuBlock *b = (VISpuBlock *)m_spuNodes.RemHead();
    int inserted = 0;

    b->m_addr = addr;
    b->m_size = n;
    for (VILNode *p = m_spuMem.m_head; p->m_next; p = p->m_next)
    {
        if (b->m_addr < ((VISpuBlock *)p)->m_addr)
        {
            b->InsertAfter(p->m_prev);
            inserted = 1;
            break;
        }
    }
    if (!inserted)
        m_spuMem.AddTail(b);
    MergeFreeSpuMem();
}

void VISoundDevice::MergeFreeSpuMem()
{
    VISpuBlock *p = (VISpuBlock *)m_spuMem.m_head;

    while (p->m_next)
    {
        VISpuBlock *n = (VISpuBlock *)p->m_next;

        if (n->m_next)
        {
            if (p->m_addr + p->m_size == n->m_addr)
            {
                n->m_addr = p->m_addr;
                n->m_size += p->m_size;
                p->Remove();
                m_spuNodes.AddTail(p);
            }
        }
        p = n;
    }
}

int VISoundDevice::InitXmMem(unsigned int addr, int size)
{
    int i;

    addr = ((addr + 63) / 64) * 64;
    size = ((size - 65) / 64) * 64;

    m_xmBlocks = new VISpuBlock[128];
    for (i = 0; i < 128; i++)
        m_xmNodes.AddTail(&m_xmBlocks[i]);

    VISpuBlock *b = (VISpuBlock *)m_xmNodes.RemHead();
    b->m_addr = addr;
    b->m_size = size;
    m_xmMem.AddTail(b);
    return 0;
}

void VISoundDevice::CloseXmMem()
{
    if (m_xmBlocks)
        delete [] m_xmBlocks;
    m_xmBlocks = 0;
    m_xmMem.Init();
    m_xmNodes.Init();
}

unsigned int VISoundDevice::AllocXmMem(int size)
{
    int n = ((size + 63) / 64) * 64;

    if (sBgmVersion == 1)
    {
        if (n < 0xc000)
            n = 0xc000;
    }
    printf("Allocating %d bytes for XM\n", size);

    for (VISpuBlock *b = (VISpuBlock *)m_xmMem.m_head; b->m_next;
         b = (VISpuBlock *)b->m_next)
    {
        if (b->m_size >= n)
        {
            unsigned int addr = b->m_addr;
            b->m_size -= n;
            b->m_addr = addr + n;
            if (b->m_size == 0)
            {
                b->Remove();
                m_xmNodes.AddTail(b);
            }
            int total = 0;
            for (VISpuBlock *f = (VISpuBlock *)m_xmMem.m_head; f->m_next;
                 f = (VISpuBlock *)f->m_next)
                total += f->m_size;
            printf("Total remaining: %d\n", total);
            return addr;
        }
    }
    printf("AllocXmMem() FAILED\n");
    return 0;
}

void VISoundDevice::FreeXmMem(unsigned int addr, int size)
{
    int n = ((size + 63) / 64) * 64;

    if (sBgmVersion == 1)
    {
        if (n < 0xc000)
            n = 0xc000;
    }

    VISpuBlock *b = (VISpuBlock *)m_xmNodes.RemHead();
    int inserted = 0;

    b->m_addr = addr;
    b->m_size = n;
    for (VILNode *p = m_xmMem.m_head; p->m_next; p = p->m_next)
    {
        if (b->m_addr < ((VISpuBlock *)p)->m_addr)
        {
            b->InsertAfter(p->m_prev);
            inserted = 1;
            break;
        }
    }
    if (!inserted)
        m_xmMem.AddTail(b);
    MergeFreeXmMem();
}

void VISoundDevice::MergeFreeXmMem()
{
    VISpuBlock *p = (VISpuBlock *)m_xmMem.m_head;

    while (p->m_next)
    {
        VISpuBlock *n = (VISpuBlock *)p->m_next;

        if (n->m_next)
        {
            if (p->m_addr + p->m_size == n->m_addr)
            {
                n->m_addr = p->m_addr;
                n->m_size += p->m_size;
                p->Remove();
                m_xmNodes.AddTail(p);
            }
        }
        p = n;
    }
}

// The source pointer is taken from the caller's field AND zeroed there, so the
// sound no longer owns its EE copy -- IdleTransfer deletes it when the last
// block has gone across.
int VISoundDevice::BeginAdpcmTransfer(void **src, unsigned int spuAddr, int size,
                                      int *flag)
{
    m_xferRemain = size;
    m_xferSrc = *src;
    m_xferSpuAddr = spuAddr;
    m_xferXmAddr = -1;
    m_xferFlag = flag;
    m_xferIdle = 0;
    m_xferFree = *src;
    *src = 0;
    return 0;
}

int VISoundDevice::BeginXmTransfer(void **src, unsigned int xmAddr,
                                   unsigned int sampleAddr, int size, int *flag)
{
    m_xferRemain = size;
    m_xferSrc = *src;
    m_xferSpuAddr = -1;
    m_xferXmAddr = xmAddr;
    m_xferXmSpu = xmAddr;
    m_xferSampleSpu = sampleAddr;
    m_xferFlag = flag;
    m_xferIdle = 0;
    m_xferFree = 0;
    return 0;
}

int VISoundDevice::IdleTransfer()
{
    if (m_xferDma)
    {
        if (sceSifDmaStat(m_xferDma) >= 0)
            return 0;
        m_xferDma = 0;
        if (m_xferSpuAddr != -1)
        {
            gSndArgs[0] = m_xferLast;
            gSndArgs[1] = m_xferSpuAddr;
            m_xferSpuAddr = m_xferSpuAddr + m_xferLast;
            SoundCmd(1, 2);
        }
        if (m_xferRemain)
            return 0;
        m_xferIdle = 1;
        if (m_xferFree)
        {
            delete [] (char *)m_xferFree;
            m_xferFree = 0;
        }
        if (m_xferXmAddr != -1)
        {
            gSndArgs[0] = m_xferXmSpu;
            gSndArgs[1] = m_xferSampleSpu;
            SoundCmd(7, 2);
        }
        if (m_xferFlag)
            *m_xferFlag = 1;
        return 0;
    }

    if (m_xferRemain > 0)
    {
        int n;
        int dma[4];

        if (gSndPacket[1] == 1)
            return 0;
        if (m_xferRemain > 65535)
        {
            n = 0x10000;
        }
        else
        {
            n = m_xferRemain + 63;
            n = n & -64;
        }
        FlushCache(0);

        dma[0] = (int)m_xferSrc;
        dma[1] = m_xferSpuAddr == -1 ? (int)m_xferXmAddr : (int)m_iopBuf;
        dma[2] = n;
        dma[3] = 0;
        FlushCache(0);
        m_xferDma = sceSifSetDma(dma, 1);
        if (m_xferDma == 0)
        {
            printf("Sample data transfer failed..\n");
            return -1;
        }
        m_xferRemain -= n;
        m_xferSrc = (char *)m_xferSrc + n;
        m_xferLast = n;
        if (m_xferSpuAddr == -1)
            m_xferXmAddr = m_xferXmAddr + n;
    }
    if (m_xferRemain < 0)
        m_xferRemain = 0;
    return 0;
}

int VISoundDevice::DebugResources()
{
    int waves = 0;
    int xms = 0;
    int bytes = 0;
    int i;

    for (i = 0; i < 128; i++)
    {
        if (m_sounds[i])
        {
            if (m_sounds[i]->Type() == VISound::WAVE)
            {
                waves++;
                bytes += 92;
                if (((VIWave *)m_sounds[i])->m_buffer)
                    bytes += ((VIWave *)m_sounds[i])->m_size;
            }
            else
            {
                int n;
                xms++;
                if (((VIXm *)m_sounds[i])->m_xm)
                    n = ((VIXm *)m_sounds[i])->m_xmSize + 64;
                else
                    n = 64;
                if (((VIXm *)m_sounds[i])->m_sample)
                    bytes = bytes + (n + ((VIXm *)m_sounds[i])->m_spuSize);
                else
                    bytes = bytes + n;
            }
        }
    }

    printf("Sound Device Memory Usage\n");
    printf("---------------------------------------------\n");
    printf("Waves             : %d\n", waves);
    printf("XMs               : %d\n", xms);
    printf("Total Mem         : %d Bytes\n", bytes);
    printf("Sound Device Ovhd : %d Bytes\n", 6016);
    printf("---------------------------------------------\n");
    return 0;
}

// ---------------------------------------------------------------------------
// The SIF RPC back end.  Five file statics, no symbols, all of them after
// DebugResources in the image.

static int gSndClient[10];

static int SoundBind()
{
    int spin;

    for (;;)
    {
        if (sceSifBindRpc(gSndClient, 0x2112, 0) < 0)
        {
            printf("Error: sceSifBindRpc \n");
            return -1;
        }
        for (spin = 9999; spin != -1; spin--)
            ;
        if (gSndClient[9])
            break;
    }
    gSndWords = 0;
    gSndPacket[0] = 0;
    return 0;
}

static int SoundCall(int fno, void *send, int async)
{
    sceSifCallRpc(gSndClient, fno, async != 0, send, 512, gSndStatus, 512, 0, 0);
    return gSndStatus[0];
}

// Ship the packet if there is anything in it; returns the words it held.
static int SoundFlush(int wait)
{
    int n;

    if (sceSifCheckStatRpc(gSndClient))
        return -1;
    if (gSndPacket[0] == 0)
        return -2;
    SoundCall(0x80f0, gSndPacket, wait);
    gSndPacket[0] = 0;
    n = gSndWords;
    gSndWords = 0;
    return n;
}

static void SoundCmd(int id, int argc)
{
    int n = gSndWords;
    int i;

    if (n + argc + 3 < 128)
    {
        gSndPacket[n + 1] = id;
        gSndPacket[n + 2] = argc;
        for (i = 0; i < argc; i++)
            gSndPacket[n + 3 + i] = gSndArgs[i];
        gSndWords += 2 + argc;
        gSndPacket[0] = gSndPacket[0] + 1;
    }
}

static int *SoundFindCmd(int id)
{
    int *p = gSndPacket;
    int n = *p++;
    int i;

    for (i = 0; i < n; i++)
    {
        int cmd = *p++;
        int argc = *p++;
        if (id == cmd)
            return p;
        p += argc;
    }
    return 0;
}
