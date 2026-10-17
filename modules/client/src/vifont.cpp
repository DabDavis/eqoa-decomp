// vifont.cpp (CLIENT) -- CLIENT 0x00C04D48 .. 0x00C05418: CLIENT's own copy of SUPPORT's
// VIFont.cpp (src/vifont.cpp, 0x004A2298 ..), compiled into CLIENT, which calls these
// copies rather than SUPPORT's exports.  The functions are SUPPORT's text, verbatim; the
// TU also holds GetSexWord and its table (0x00CDFB48, right after "ArialUI"), and its data
// ends in a bad_alloc string SUPPORT's TU does not have -- a different file, not a share.
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);

char VIFont::sm_FontName[] = "ArialUI";


VIFont::VIFont()
    : m_color(1.0f, 1.0f, 1.0f, 1.0f)
{
}

void VIFont::SetTextColor(const VIColor32F &c)
{
    m_color = c;
}

void VIFont::SetTextColor(const VIColor32 &c)
{
    m_color.r = c.r * (1.0f / 255.0f);
    m_color.g = c.g * (1.0f / 255.0f);
    m_color.b = c.b * (1.0f / 255.0f);
    m_color.a = c.a * (1.0f / 255.0f);
}

// Shift-JIS to JIS X 0208.  Every value here is 64-bit -- `unsigned long` is
// eight bytes on this ABI -- so the whole function is d-form arithmetic.  The
// closing constant is 0x2021, not 0x2121, because the `+ 1` stays INSIDE the
// shift: gcc 2.95 does not distribute a constant add across a left shift.
unsigned long VIFont::SjisToJis(unsigned long c)
{
    unsigned long hi = (c >> 8) & 0xFF;
    unsigned long lo = c & 0xFF;
    unsigned long t;

    t = hi - 0x81;
    if (t < 31)
        hi = t;
    else if (hi - 0xE0 <= 15)
        hi = hi - 0xC1;
    hi = hi << 1;

    t = lo - 0x40;
    if (t < 63)
        lo = t;
    else if (lo - 0x80 < 31)
        lo = lo - 0x41;
    else if (lo - 0x9F < 94)
    {
        lo = lo - 0x9F;
        hi++;
    }
    return ((hi + 1) << 8) + lo + 0x2021;
}

// Phase 61.  A qsort/bsearch comparator for 16-bit character codes; nothing in
// SUPPORT takes its address, so its user is CLIENT.
int uint16_compare(const void *a, const void *b)
{
    return *(const unsigned short *)a - *(const unsigned short *)b;
}

int VIFont::DrawString(VIRaster *r, VIDictionary *d, const unsigned short *s, int x, int y)
{
    int i;

    if (d)
    {
        static unsigned int sm_FontID;
        sm_FontID = VIHashResourceID(sm_FontName);
        if (d->FindTyped(sm_FontID, VIRT_Font, i))
            r->DrawString(i, VIVect2I(x, y), m_color, s);
    }
    return 0;
}

int VIFont::DrawString(VIRaster *r, VIDictionary *d, const char *s, int x, int y)
{
    int i;

    if (d)
    {
        static unsigned int sm_FontID;
        sm_FontID = VIHashResourceID(sm_FontName);
        if (d->FindTyped(sm_FontID, VIRT_Font, i))
            r->DrawString(i, VIVect2I(x, y), m_color, s);
    }
    return 0;
}

int VIFont::DrawCenteredString(VIRaster *r, VIDictionary *d, const unsigned short *s,
                               int x, int y, unsigned int w)
{
    int i;

    if (d)
    {
        static unsigned int sm_FontID;
        sm_FontID = VIHashResourceID(sm_FontName);
        if (d->FindTyped(sm_FontID, VIRT_Font, i))
            r->DrawCenteredString(i, VIVect2I(x, y), m_color, s, w);
    }
    return 0;
}

int VIFont::DrawCenteredString(VIRaster *r, VIDictionary *d, const char *s,
                               int x, int y, unsigned int w)
{
    int i;

    if (d)
    {
        static unsigned int sm_FontID;
        sm_FontID = VIHashResourceID(sm_FontName);
        if (d->FindTyped(sm_FontID, VIRT_Font, i))
            r->DrawCenteredString(i, VIVect2I(x, y), m_color, s, w);
    }
    return 0;
}

int VIFont::GetHeight(VIRaster *r, VIDictionary *d)
{
    int n = 0;
    int i;

    if (d)
    {
        static unsigned int sm_FontID;
        sm_FontID = VIHashResourceID(sm_FontName);
        if (d->FindTyped(sm_FontID, VIRT_Font, i))
            n = r->GetPointSize(i);
    }
    return n;
}

int VIFont::GetStringWidth(VIRaster *r, VIDictionary *d, const char *s)
{
    int n = 0;
    int i;

    if (d)
    {
        static unsigned int sm_FontID;
        sm_FontID = VIHashResourceID(sm_FontName);
        if (d->FindTyped(sm_FontID, VIRT_Font, i))
            n = r->GetStringWidth(i, s);
    }
    return n;
}

int VIFont::GetStringWidth(VIRaster *r, VIDictionary *d, const unsigned short *s)
{
    int n = 0;
    int i;

    if (d)
    {
        static unsigned int sm_FontID;
        sm_FontID = VIHashResourceID(sm_FontName);
        if (d->FindTyped(sm_FontID, VIRT_Font, i))
            n = r->GetStringWidth(i, s);
    }
    return n;
}

int VIFont::GetCharacterWidth(VIRaster *r, VIDictionary *d, unsigned short c)
{
    int n = 0;
    int i;

    if (d)
    {
        static unsigned int sm_FontID;
        sm_FontID = VIHashResourceID(sm_FontName);
        if (d->FindTyped(sm_FontID, VIRT_Font, i))
            n = r->GetWidth(i, c);
    }
    return n;
}

int VIFont::FontLoaded(VIDictionary *d)
{
    int i;

    return d->FindTyped(VIHashResourceID(sm_FontName), VIRT_Font, i) != 0;
}

// 0x00C053F8.  The WORDS.TXT id of a sex's name: 163 "Male", 164 "Female"; out of range
// is Male.  (The character-creation screens' caller, 0x00C2B270, reads the word.)
static int sSexWords[] = { 163, 164 };

int *GetSexWord(int sex)
{
    if ((unsigned)sex >= 2)
        sex = 0;
    return &sSexWords[sex];
}
