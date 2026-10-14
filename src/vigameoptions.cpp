// VIGameOptions -- SUPPORT 0x004A2A80 .. 0x004A2D38.
#include "vi.h"

extern "C" void *memset(void *d, int c, unsigned int n);
extern "C" char *strncpy(char *d, const char *s, unsigned int n);

extern int VIMemCardReadFile(const char *name, void *buf, int size);
extern int VIMemCardWriteFile(const char *name, const void *buf, int size);
extern unsigned int VIGetDiscFlags();

static const char OptionsFile[] = "game.opt";

// Phase 61.  The one global options object.  Its constructor is what makes the
// TU's `_GLOBAL_$I$VIGetGameOptions__Fv` -- named after VIGetGameOptions, the
// first PUBLIC symbol, because a static object never names it.
static VIGameOptions s_gameOptions;

VIGameOptions *VIGetGameOptions()
{
    return &s_gameOptions;
}

VIGameOptions::VIGameOptions()
{
    SetDefaults();
    m_language = 0;
}

int VIGameOptions::Load()
{
    memset(this, 0, sizeof(VIGameOptions));

    int r = VIMemCardReadFile(OptionsFile, this, sizeof(VIGameOptions));

    if (r < 0)
    {
        SetDefaults();
        return r;
    }

    if (r < (int)sizeof(VIGameOptions))
    {
        SetDefaults();
        return -1;
    }

    if (m_flags || m_magic != 0x01234567)
        SetDefaults();

    unsigned int f = VIGetDiscFlags();

    // A SWITCH and not an if/else chain, Phase 16's lever again: the chain puts
    // the one-instruction `m_region = 0` body inline and costs the target's
    // branch-likely, whichever way round the three arms are written (six
    // spellings measured).
    switch (f)
    {
    case 2:
        m_region = 1;
        break;

    case 0:
    case 1:
        m_region = 0;
        break;

    case 3:
        if (m_region < 1 || m_region > 3)
            m_region = 1;
        break;
    }

    return 0;
}

int VIGameOptions::Save()
{
    return VIMemCardWriteFile(OptionsFile, this, sizeof(VIGameOptions));
}

void VIGameOptions::SetDefaults()
{
    memset(this, 0, sizeof(VIGameOptions));

    m_flags = 0;
    m_language = 0;
    m_x05 = 1;
    m_x06 = 0;
    m_x07 = 0;
    m_musicVolume = 1.0f;
    m_effectVolume = 1.0f;
    m_x97 = 1;
    m_magic = 0x01234567;

    if (VIGetFeatureFlags() & 0x100)
        m_language = 9;
}

const char *VIGameOptions::Username() const
{
    return m_username;
}

void VIGameOptions::SetUsername(const char *s)
{
    strncpy(m_username, s, 63);
    m_username[63] = 0;
}

const char *VIGameOptions::Password() const
{
    return m_password;
}

void VIGameOptions::SetPassword(const char *s)
{
    strncpy(m_password, s, 63);
    m_password[63] = 0;
}

const unsigned short *VIGameOptions::HomeShardName() const
{
    return m_homeShard;
}

void VIGameOptions::SetHomeShardName(const unsigned short *s)
{
    VIString::strcpy16(m_homeShard, 64, s);
}
