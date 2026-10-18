// VIPad -- SUPPORT 0x004A1AC8 .. 0x004A2298, its own TU.
//
// One controller port over libpad.  The interesting half is the vibration
// deadline: VibrateLf / VibrateHf stamp an expiry and set a dirty flag, and
// Process is what actually talks to the pad -- at most one scePadSetActDirect
// a frame, and only when something changed.
#include "vi.h"

extern "C" int scePadPortOpen(int port, int slot, void *buf);
extern "C" int scePadPortClose(int port, int slot);
extern "C" int scePadSetActDirect(int port, int slot, const unsigned char *act);
extern "C" int scePadGetReqState(int port, int slot);
extern "C" int scePadGetState(int port, int slot);
extern "C" int scePadInfoMode(int port, int slot, int term, int offs);
extern "C" int scePadInfoAct(int port, int slot, int actno, int term);
extern "C" int scePadSetActAlign(int port, int slot, const unsigned char *align);
extern "C" int scePadSetMainMode(int port, int slot, int offs, int lock);
extern "C" int scePadRead(int port, int slot, unsigned char *data);

static unsigned char sPadBuf[256] __attribute__((aligned(64)));
static unsigned char sActAlign[6] = { 0, 1, 0xff, 0xff, 0xff, 0xff };

VIPad::VIPad()
{
    m_open = 0;
    m_port = 0;
    m_slot = 0;
    m_mode = 0;
}

VIPad::~VIPad()
{
    Close();
}

int VIPad::Init(int port, int slot)
{
    int r = scePadPortOpen(port, slot, sPadBuf);

    if (r != 1)
        return -1;

    m_open = r;
    m_port = port;
    m_slot = slot;
    m_time = 0;
    m_lfEnd = 0;
    m_hfEnd = 0;
    m_state = 0;
    m_actAligned = 0;
    m_act[0] = 0;
    m_act[1] = 0;
    m_act[2] = 0;
    m_act[3] = 0;
    m_act[4] = 0;
    m_act[5] = 0;
    m_dirty = 0;
    // Phase 16's hoist reads this out: m_mode closes the block, not m_dirty,
    // and moved up beside m_slot -- where the field list would put it -- the
    // whole run comes out in the wrong order.
    m_mode = 0;
    return 0;
}

int VIPad::Close()
{
    if (m_open)
    {
        m_open = 0;
        scePadPortClose(m_port, m_slot);
    }
    m_mode = 0; VI_HOST_ONLY(return 0;)
}

int VIPad::Process(long now)
{
    m_time = now;

    if (Active())
    {
        if (m_lfEnd && m_lfEnd < now)
        {
            m_act[1] = 0;
            m_dirty = 1;
            m_lfEnd = 0;
        }
        if (m_hfEnd && m_hfEnd < now)
        {
            m_act[0] = 0;
            m_dirty = 1;
            m_hfEnd = 0;
        }
        if (m_dirty)
        {
            scePadSetActDirect(m_port, m_slot, m_act);
            m_dirty = 0;
        }
    }
    return 0;
}

int VIPad::VibrateLf(unsigned char level, int ms)
{
    if (Active())
    {
        // The same statement order as VibrateHf below.  Three of the six
        // orders are byte-exact and three are 6 words wrong; this is the one
        // that makes the two functions read alike.
        m_act[1] = level;
        m_lfEnd = ms + m_time;
        m_dirty = 1;
    }
VI_HOST_ONLY(return 0;) }

int VIPad::VibrateHf(int ms)
{
    if (Active())
    {
        m_act[0] = 1;
        m_hfEnd = ms + m_time;
        m_dirty = 1;
    }
VI_HOST_ONLY(return 0;) }

void VIPad::StopVibration()
{
    m_act[0] = 0;
    m_act[1] = 0;
    m_lfEnd = 0;
    m_hfEnd = 0;
    scePadSetActDirect(m_port, m_slot, m_act);
}

// 88 and 167 bound a dead zone in the middle of the 0..255 range, and 1/88 is
// the reciprocal written out -- gcc will not turn a divide into a multiply.
float VIPad::ScaleStick(unsigned char v)
{
    // Phase 7's if-direction: the POSITIVE half is the one gcc moves out of
    // line, so it is the one written in the `if`, and the dead zone's 0.0f is
    // the fall-through.  Written as `if (v < 168) return 0.0f;` the two blocks
    // swap and the second comparison stops being hoisted into the first
    // branch's delay slot: 11 of 27 words.
    if (v < 88)
        return (88 - v) * -0.0113636364f;
    if (v >= 168)
        return (v - 167) * 0.0113636364f;
    return 0.0f;
}

static unsigned int gExpansionFlags;
static unsigned int gFeatureFlags;

int VIGetExpansionFlags()
{
    return gExpansionFlags;
}

void VISetExpansionFlags(unsigned int f)
{
    gExpansionFlags = f;
}

int VIGetFeatureFlags()
{
    return gFeatureFlags;
}

void VISetFeatureFlags(unsigned int f)
{
    gFeatureFlags = f;
}

// The libpad state machine, the actuator handshake, and the unpack.  Every
// button is the COMPLEMENT of one bit -- libpad reports them active-low -- and
// gcc turns `(~b >> 7) & 1` into `(b ^ 0x80) >> 7` for the top bit only, which
// is why the two bit-7 buttons look like a different idiom.
int VIPad::Read(VIConsolePad &p)
{
    // libpad's state names, indexed by scePadGetState's result.  Nothing reads
    // them in this build -- they are what a debug print would have used -- but
    // the array is still initialised: a 32-byte copy from the .rodata template
    // at 0x4ED088, whose eight words point at the strings just below it ("" is
    // one string for both unused states 3 and 4).
    const char *stateName[8] =
    {
        "DISCONNECT", "FINDPAD", "FINDCTP1", "", "", "EXECCMD", "STABLE", "ERROR"
    };

    if (m_actAligned)
    {
        int rs = scePadGetReqState(m_port, m_slot);

        if (rs != 0)
        {
            if (rs == 2)
                return 0;
            m_state = 0;
        }
        m_actAligned = 0;
    }

    int st = scePadGetState(m_port, m_slot);

    // The whole ready path is the `if` BODY and the not-ready teardown is the
    // function's tail, which is what puts the teardown last in the object code
    // -- written the other way round (an early `st != 2 && st != 6` return)
    // gcc leaves it inline where the test is.
    if (st == 2 || st == 6)
    {
        int id = scePadInfoMode(m_port, m_slot, 1, 0);

        if (id)
        {
            int ex = scePadInfoMode(m_port, m_slot, 2, 0);
            if (ex > 0)
                id = ex;
        }

        if (m_state == -1)
        {
            m_state = scePadInfoAct(m_port, m_slot, -1, 0);
            if (m_state != 2)
                m_state = 0;
            else if (id != 7)
                m_state = 0;
            else if (scePadInfoAct(m_port, m_slot, 0, 3) != 0)
                m_state = 0;
            else if (scePadInfoAct(m_port, m_slot, 1, 3) <= 0)
                m_state = 0;
            else
            {
                int r = scePadSetActAlign(m_port, m_slot, sActAlign);
                if (r == 1)
                    m_actAligned = r;
                else
                    m_state = 0;
            }
        }

        m_mode = 0;

        // Phase 7 again, and this one decides the whole tail layout: with the
        // UNPACK in the `if`, the SetMainMode arm lands after it and the
        // not-ready teardown after that, which is the target's block order.
        if (id == 7)
        {
        // Phase 23's nested-guard rule, the `||` half: two `if (X) return 0;`
        // statements branch AROUND the first test and cost a `b` to the shared
        // epilogue; one disjunction sends both straight there.
        if (scePadRead(m_port, m_slot, m_data) <= 0 || m_data[0])
            return 0;

        p.m_rx = ScaleStick(m_data[4]);
        p.m_ry = ScaleStick(m_data[5]);
        p.m_lx = ScaleStick(m_data[6]);
        p.m_ly = ScaleStick(m_data[7]);

        p.m_triangle = (float)((~m_data[3] >> 4) & 1);
        p.m_square   = (float)((~m_data[3] >> 7) & 1);
        p.m_cross    = (float)((~m_data[3] >> 6) & 1);
        p.m_circle   = (float)((~m_data[3] >> 5) & 1);

        p.m_left  = (float)((~m_data[2] >> 7) & 1);
        p.m_down  = (float)((~m_data[2] >> 6) & 1);
        p.m_right = (float)((~m_data[2] >> 5) & 1);
        p.m_up    = (float)((~m_data[2] >> 4) & 1);

        p.m_l1 = (float)((~m_data[3] >> 2) & 1);
        p.m_r1 = (float)((~m_data[3] >> 3) & 1);
        p.m_l2 = (float)((~m_data[3] >> 0) & 1);
        p.m_r2 = (float)((~m_data[3] >> 1) & 1);

        p.m_start  = (float)((~m_data[2] >> 3) & 1);
        p.m_select = (float)((~m_data[2] >> 0) & 1);
        p.m_l3     = (float)((~m_data[2] >> 1) & 1);
        p.m_r3     = (float)((~m_data[2] >> 2) & 1);

        m_mode = 1;
        return 1;
        }

        scePadSetMainMode(m_port, m_slot, 1, 3);
        return 0;
    }

    // The reset is a do/while(0) (a macro's shape): its LOOP_END note is a
    // sched barrier that keeps the return's `move v0,zero` after the stores
    // (6 wrong words without).  The `if` must be INSIDE it -- the first label
    // after LOOP_BEG is aligned, and the if's end label is 8-aligned already
    // where the return label is not (a stores-only wrap costs a `nop`).
    do
    {
        if (Active())
        {
            m_act[0] = 0;
            m_act[1] = 0;
            scePadSetActDirect(m_port, m_slot, m_act);
        }
        m_state = -1;
        m_lfEnd = 0;
        m_hfEnd = 0;
        m_act[0] = 0;
        m_act[1] = 0;
        m_dirty = 0;
        m_mode = 0;
    } while (0);
    return 0;
}
