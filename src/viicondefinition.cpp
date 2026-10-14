// VIIconDefinition -- SUPPORT 0x003E9140 .. 0x003E96D8, its own TU.
//
// The memory card's icon.sys, built in memory.  Everything here but the
// constructor is about the TITLE: the card shows it as two lines of SJIS, so
// the ASCII the caller hands in has to be converted a character at a time and
// its newline turned into a halfword offset.
#include "vi.h"

extern "C" int   strlen(const char *s);
extern "C" char *strcpy(char *d, const char *s);
extern "C" void *memset(void *d, int c, int n);

// The alphanumeric runs are contiguous in SJIS too, so three entries carry a
// base and the ASCII code it starts at: digits, upper case, lower case.
static VISjisMap sAlnum[3];

// The punctuation is not contiguous, so it is a dense table of the 33 codes
// between the runs -- 32..47, 58..64, 91..96 and 123..126 mapped onto 0..32 by
// subtracting a per-range bias.  That is what the four constants 1, 11, 37 and
// 63 are: `c - bias - 31` lands each range on the index after the last.
static VISjisMap sPunct[33];

VIIconDefinition::~VIIconDefinition()
{
}

unsigned short VIIconDefinition::Ascii2Sjis(char c)
{
    // `run` is NOT initialised: the digits arm assigns it 0 explicitly, and
    // hoisting that to the declaration lets gcc delete the arm's own store --
    // the `b`/`move a0,zero` pair the target has at 0x3E9560.
    int bias = 0;
    int run;

    if (c >= 32 && c <= 47)
        bias = 1;
    else if (c >= 48 && c <= 57)
        run = 0;
    else if (c >= 58 && c <= 64)
        bias = 11;
    else if (c >= 65 && c <= 90)
        run = 1;
    else if (c >= 91 && c <= 96)
        bias = 37;
    else if (c >= 97 && c <= 122)
        run = 2;
    else if (c >= 123 && c <= 126)
        bias = 63;
    else
        return 0;

    // A RESULT VARIABLE, punctuation first.  Returned straight from each
    // arm, the chain feeding the `lhu v0` ends where v0 is born, and
    // local-alloc's widened (INSN_SCHEDULING) lifetime then refuses it v0:
    // the index takes v1 and the table base v0.  Through `r` the chain ends
    // at a global pseudo instead (13 -> 0).  Phase 14's fresh local stays:
    // `c - (bias + 31)` written out reassociates to `(c - 31) - bias`.
    unsigned short r;

    if (bias != 0)
    {
        int i = bias + 31;
        r = sPunct[c - i].m_sjis;
    }
    else
        r = sAlnum[run].m_sjis + c - sAlnum[run].m_first;
    return r;
}

void VIIconDefinition::AsciiString2Sjis(char *src, unsigned short *dst)
{
    int i = 0;
    int n = strlen(src);

    // The loop variable is read AFTER the loop, which is why gcc restores the
    // counter to n on the way out and why the n <= 0 path writes dst[0].
    for (i = 0; i < n; i++)
    {
        unsigned short s = Ascii2Sjis(src[i]);
        dst[i] = (s >> 8) | ((s & 0xff) << 8);
    }
    dst[i] = 0;
}

int VIIconDefinition::SetTitle(const char *title)
{
    char buf[48];
    int  i;     // ONE counter for both loops; two scoped ones swap a0 and a1
    int  nl = -1;
    int  n = 0;
    int  len = strlen(title);

    for (i = 0; i < len; i++)
    {
        if (title[i] == '\n')
        {
            n++;
            if (n >= 2)
                return -1;
            nl = i;
        }
    }
    if (nl >= 17)
        return -1;

    strcpy(buf, title);

    if (n)
    {
        if (len >= 34)
            return -1;
        for (i = nl; i < len; i++)
            buf[i] = buf[i + 1];
        buf[len - 1] = 0;
    }
    else if (len >= 33)
        return -1;

    AsciiString2Sjis(buf, m_title);

    if (n)
        m_nlOffset = nl * 2;
    else
        m_nlOffset = 32;
    return 0;
}

void VIIconDefinition::SetIcon(void *data, int size)
{
    m_iconData = data;
    m_iconSize = size;
    strcpy(m_fnameView, "static.ico");
    strcpy(m_fnameCopy, "static.ico");
    strcpy(m_fnameDel, "static.ico");
}

// The whole of icon.sys, in field order.  The four background corners are
// INTEGER quads built on the stack; the three light directions and their
// colours are float.  Two of the three lights are left black -- only light 1
// and the ambient carry the 0.65 grey.
VIIconDefinition::VIIconDefinition()
{
    m_head = 0x44325350;        // 'P','S','2','D' little-endian
    m_reserved = 0;
    m_nlOffset = 0;
    m_unknown = 0;
    m_trans = 80;

    m_bgColor[0] = VIIconColor(0, 0, 64, 128);
    m_bgColor[1] = VIIconColor(0, 0, 64, 128);
    m_bgColor[2] = VIIconColor(0, 0, 0, 128);
    m_bgColor[3] = VIIconColor(0, 0, 0, 128);

    m_lightDir[0][0] = -1.0f;
    m_lightDir[0][1] = 0.0f;
    m_lightDir[0][2] = 0.0f;
    m_lightDir[0][3] = 1.0f;
    m_lightDir[1][0] = 0.0f;
    m_lightDir[1][1] = -1.0f;
    m_lightDir[1][2] = 0.0f;
    m_lightDir[1][3] = 1.0f;
    m_lightDir[2][0] = 0.0f;
    m_lightDir[2][1] = 0.0f;
    m_lightDir[2][2] = -1.0f;
    m_lightDir[2][3] = 1.0f;

    m_lightColor[0][0] = 0.0f;
    m_lightColor[0][1] = 0.0f;
    m_lightColor[0][2] = 0.0f;
    m_lightColor[0][3] = 0.0f;
    m_lightColor[1][0] = 0.65f;
    m_lightColor[1][1] = 0.65f;
    m_lightColor[1][2] = 0.65f;
    m_lightColor[1][3] = 0.65f;
    m_lightColor[2][0] = 0.0f;
    m_lightColor[2][1] = 0.0f;
    m_lightColor[2][2] = 0.0f;
    m_lightColor[2][3] = 0.0f;

    m_ambient[0] = 0.65f;
    m_ambient[1] = 0.65f;
    m_ambient[2] = 0.65f;
    m_ambient[3] = 0.65f;

    memset(m_title, 0, 68);
    memset(m_fnameView, 0, 64);
    memset(m_fnameCopy, 0, 64);
    memset(m_fnameDel, 0, 64);
    memset(m_reserved2, 0, 512);
}
