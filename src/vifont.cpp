// VIFont -- SUPPORT 0x004A2298 .. 0x004A2944.
//
// The drawing front end.  Five of the seven entry points do exactly the same
// three things -- hash the font's name, look it up in the dictionary, forward
// to the matching VIRaster call with the VIFont itself as the colour -- and
// each one stores the hash into a static of its own before using it.  Those
// stores are `lui`/`sw 0(reg)` pairs, i.e. relocation sites the differ masks,
// so one file-scope static here produces the same bytes as seven.
//
// The class is nothing but a VIColor32F: every VIRaster call takes `this`
// straight through as its `const VIColor32F &` argument, and the constructor
// initialises the member in its MEM-INITIALISER -- four stores, where a body
// assignment would leave VIColor32F's own zeroing constructor in front of it.
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);

char VIFont::sm_FontName[] = "ArialUI";

static unsigned int sm_FontID;

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
