// VIRFont -- SUPPORT 0x00414610 .. 0x00415CD8.
//
// The glyph atlas: a decoded font in one heap block, and an LRU cache of the
// glyphs currently resident in a 256x256 VISurface.  Everything the parser
// does goes through Lock / BeginChar / EndChar / Unlock; everything the
// renderer does goes through Cache.
//
// The four sections of m_data -- chars, slots, pixels, palette -- are byte
// OFFSETS while CalcDataSize is laying them out, and Init rebases all four
// onto the block in one pass once Allocate has run.  Slot() and Char() are
// integer arithmetic on those rebased values, offset first (Phase 24).
#include "vi.h"

extern "C" void *memcpy(void *, const void *, unsigned int);

VIRFont::VIRFont()
{
    m_data = 0;
    Clear();
}

VIRFont::~VIRFont()
{
    Clear();
}

int VIRFont::Clear()
{
    delete [] m_data;

    m_surface = -1;
    m_material = -1;
    m_locked = 0;
    m_charHeight = 0;
    m_maxWidth = 0;
    m_pixelStore = VIPixelStoreI8;
    m_palettePixelStore = VIPixelStoreRGBA32;
    m_numCharsLoaded = 0;
    m_numChars = 0;
    m_numSlots = 0;
    m_maxSlots = 0;
    m_paletteCount = 0;
    m_head = -1;
    m_tail = -1;
    m_chars = 0;
    m_slots = 0;
    m_palette = 0;
    m_pixels = 0;
    m_dataSize = 0;
    m_data = 0;
    return 0;
}

int VIRFont::Init(VIRaster &r, VIDictionary &d, int numChars, unsigned char *widths,
                  int height, VIBitDepth bd, int extra)
{
    m_numChars = numChars;
    m_charHeight = height;

    if (CalcMaxWidth(numChars, widths) < 0)
        return -1;
    if (CreateResources(r, d, numChars, m_charHeight, bd, extra) < 0)
        return -1;
    if (CalcDataSize(widths) < 0)
        return -1;
    if (Allocate() < 0)
        return -1;

    m_chars = (int)m_data + m_chars;
    m_slots = (int)m_data + m_slots;
    m_pixels = (int)m_data + m_pixels;
    m_palette = (int)m_data + m_palette;

    if (InitSlots(r) < 0)
        return -1;
    if (InitChars(widths) < 0)
        return -1;
    return 0;
}

int VIRFont::IsPalettized()
{
    return m_paletteCount != 0;
}

VIPixelStore VIRFont::PixelStore()
{
    return m_pixelStore;
}

VIPixelStore VIRFont::PalettePixelStore()
{
    return m_palettePixelStore;
}

int VIRFont::Lock(VIRaster &r)
{
    if (m_locked)
        return -1;
    m_locked = 1;
    return 0;
}

int VIRFont::Unlock(VIRaster &r)
{
    if (m_locked == 0)
        return -1;
    m_locked = 0;
    if (ResetCache(r) < 0)
        return -1;
    return 0;
}

int VIRFont::Palette(int &count, VIPixelStore &ps, unsigned char **p)
{
    if (m_paletteCount == 0)
        return -1;
    count = m_paletteCount;
    ps = m_palettePixelStore;
    *p = (unsigned char *)m_palette;
    return 0;
}

int VIRFont::BeginChar(unsigned short c, int &bytesPerRow, unsigned char **p)
{
    VIRFontChar *ch = Char(m_numCharsLoaded);

    ch->m_code = c;
    bytesPerRow = ch->m_bytesPerRow;
    *p = m_data + ch->m_offset;
    return 0;
}

int VIRFont::EndChar()
{
    m_numCharsLoaded++;
    return 0;
}

int VIRFont::CreateResources(VIRaster &r, VIDictionary &d, int numChars, int height,
                             VIBitDepth bd, int extra)
{
    m_surface = r.CreateSurface(0);
    if (m_surface < 0)
        return -1;
    if (r.ShareSurface(m_surface) < 0)
        return -1;

    int cell = VIMAX(m_maxWidth, height);
    m_maxSlots = (256 / cell) * (256 / height);
    if (m_maxSlots <= 0)
        return -1;

    VISurface *s = r.Surface(m_surface);
    if (s->Init(r, 256, 256, bd, 1) < 0)
        return -1;

    m_pixelStore = s->m_pixelStore;
    // The target's guard is `beqz` with a hoisted `li a0,1` -- the I8
    // comparand -- in its delay slot, and a separate `m_paletteCount = 0`
    // block after the arms.  Ours annuls instead (`beqzl`, the store in the
    // slot) and materialises the 1 inside the second test.  Same words, four
    // of them in different places; writing the zero case as the `if` aligns
    // more of them and gets the block order WRONG, so it is not the source.
    if (s->m_paletteCount)
    {
        m_palettePixelStore = s->m_paletteStore;
        if (m_pixelStore == VIPixelStoreI4)
            m_paletteCount = 16;
        else if (m_pixelStore == VIPixelStoreI8)
            m_paletteCount = 256;
        else
            return -1;
    }
    else
        m_paletteCount = 0;

    m_material = r.CreateMaterial(0);
    if (m_material < 0)
        return -1;
    if (r.ShareMaterial(m_material) < 0)
        return -1;

    VIMaterial *m = r.Material(m_material);
    m->Init(1);
    m->SetLayerTexture(0, m_surface, r, d);
    m->SetLayerWrapMode(0, (VIMaterialWrapMode)3);
    if (extra == 0)
        m->SetLayerBlendMode(0, VIAlphaBlendMode3);
    else
        m->SetLayerBlendMode(0, VIAlphaBlendModeNone);
    m->SetLayerColor(0, VIColor32(255, 255, 255, 255));
    m->SetDither(0);
    return 0;
}

int VIRFont::ReleaseResources(VIRaster &r, VIDictionary &d)
{
    if (r.ReleaseMaterial(m_material, d) < 0)
        return -1;
    if (r.ReleaseSurface(m_surface, d) < 0)
        return -1;
    return 0;
}

int VIRFont::FindChar(unsigned short c)
{
    int lo = 0;
    int hi = m_numCharsLoaded - 1;
    int mid;

    while (lo <= hi)
    {
        mid = (lo + hi) >> 1;
        if (Char(mid)->m_code == c)
            break;
        if (c < Char(mid)->m_code)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    if (lo > hi)
        return -1;
    return mid;
}

int VIRFont::GetWidth(char c)
{
    int i = FindChar((unsigned short)c);
    if (i < 0)
    {
        i = FindChar('?');
        if (i < 0)
            return -1;
    }
    return Char(i)->m_width;
}

int VIRFont::GetWidth(unsigned short c)
{
    int i = FindChar(c);
    if (i < 0)
    {
        i = FindChar('?');
        if (i < 0)
            return -1;
    }
    return Char(i)->m_width;
}

int VIRFont::GetStringWidth(const char *s, int spacing)
{
    int w = 0;

    while (*s)
    {
        int i = FindChar((unsigned char)*s);
        if (i < 0)
        {
            i = FindChar('?');
            if (i < 0)
                return -1;
        }
        w += Char(i)->m_width + spacing;
        s++;
    }
    return w;
}

int VIRFont::GetStringWidth(const unsigned short *s, int spacing)
{
    int w = 0;

    while (*s)
    {
        int i = FindChar(*s);
        if (i < 0)
        {
            i = FindChar('?');
            if (i < 0)
                return -1;
        }
        w += Char(i)->m_width + spacing;
        s++;
    }
    return w;
}

int VIRFont::CalcMaxWidth(int n, unsigned char *widths)
{
    m_maxWidth = 0;
    for (int i = 0; i < n; i++)
        m_maxWidth = VIMAX(m_maxWidth, widths[i]);
    return 0;
}

int VIRFont::CalcDataSize(unsigned char *widths)
{
    m_chars = 0;
    m_slots = m_numChars * (int)sizeof(VIRFontChar);
    m_pixels = m_dataSize = m_maxSlots * (int)sizeof(VIRFontSlot) + m_slots;

    for (int i = 0; i < m_numChars; i++)
    {
        int n;

        switch (m_pixelStore)
        {
        case VIPixelStoreI4:
            if (widths[i] & 1)
                n = (widths[i] >> 1) + 1;
            else
                n = widths[i] >> 1;
            break;
        case VIPixelStoreI8:
            n = widths[i];
            break;
        case VIPixelStoreRGB24:
            n = widths[i] * 3;
            break;
        case VIPixelStoreRGBA16:
            n = widths[i] * 2;
            break;
        case VIPixelStoreRGBA32:
        case VIPixelStore3:
        case VIPixelStore6:
            n = widths[i] * 4;
            break;
        default:
            return -1;
        }
        m_dataSize = m_dataSize + n * m_charHeight;
    }

    m_palette = m_dataSize;
    if (m_paletteCount)
    {
        if (m_palettePixelStore != VIPixelStoreRGBA32)
            return -1;
        m_dataSize = m_dataSize + m_paletteCount * 4;
    }
    return 0;
}

int VIRFont::Allocate()
{
    if (m_dataSize)
    {
        m_data = new unsigned char[m_dataSize];
        if (m_data == 0)
            return -1;
    }
    return 0;
}

int VIRFont::InitSlots(VIRaster &r)
{
    int           stride;
    VIPixelStore  ps;
    unsigned char *pixels;

    VISurface *s = r.Surface(m_surface);
    int sw = s->m_width;
    int sh = s->m_height;

    if (s->LockMipLevel(0, stride, ps, &pixels) < 0)
        return -1;

    int cell = VIMAX(m_maxWidth, m_charHeight);
    int rowBytes;

    if (ps == VIPixelStoreI4)
        cell = cell + (cell & 1);

    switch (ps)
    {
    case VIPixelStoreI4:
        rowBytes = cell >> 1;
        break;
    case VIPixelStoreI8:
        rowBytes = cell;
        break;
    case VIPixelStoreRGB24:
        rowBytes = cell * 3;
        break;
    case VIPixelStoreRGBA16:
        rowBytes = cell * 2;
        break;
    case VIPixelStoreRGBA32:
    case VIPixelStore3:
    case VIPixelStore6:
        rowBytes = cell * 4;
        break;
    default:
        return -1;
    }

    m_numSlots = (sw / cell) * (sh / cell);

    int i = 0;
    for (int y = 0; y + cell <= sh; y += cell)
    {
        int row = y * stride;

        for (int x = 0; x + cell <= sw; x += cell)
        {
            VIVect2I *pos = &Slot(i)->m_pos;

            pos->y = y;
            pos->x = x;
            Slot(i)->m_offset = row;
            Slot(i)->m_char = 0xFFFF;
            if (i)
                Slot(i)->m_prev = i - 1;
            else
            {
                m_head = 0;
                Slot(i)->m_prev = -1;
            }
            if (i != m_numSlots - 1)
                Slot(i)->m_next = i + 1;
            else
            {
                m_tail = i;
                Slot(i)->m_next = -1;
            }
            i++;
            row = row + rowBytes;
        }
    }

    if (s->UnlockMipLevel(0) < 0)
        return -1;
    return 0;
}

int VIRFont::InitChars(unsigned char *widths)
{
    int offset = m_pixels - (int)m_data;

    for (int i = 0; i < m_numChars; i++)
    {
        int n;

        switch (m_pixelStore)
        {
        case VIPixelStoreI4:
            if (widths[i] & 1)
                n = (widths[i] >> 1) + 1;
            else
                n = widths[i] >> 1;
            break;
        case VIPixelStoreI8:
            n = widths[i];
            break;
        case VIPixelStoreRGB24:
            n = widths[i] * 3;
            break;
        case VIPixelStoreRGBA16:
            n = widths[i] * 2;
            break;
        case VIPixelStoreRGBA32:
        case VIPixelStore3:
        case VIPixelStore6:
            n = widths[i] * 4;
            break;
        default:
            return -1;
        }

        Char(i)->m_bytesPerRow = n;
        Char(i)->m_width = widths[i];
        Char(i)->m_slot = 0xFFFF;
        Char(i)->m_offset = offset;
        offset = n * m_charHeight + offset;
    }
    return 0;
}

int VIRFont::ResetCache(VIRaster &r)
{
    int           count;
    VIPixelStore  ps;
    unsigned char *pixels;

    VISurface *s = r.Surface(m_surface);
    if (s->LockPalette(count, ps, &pixels) < 0)
        return -1;
    memcpy(pixels, (unsigned char *)m_palette, VIMIN(m_paletteCount, count) * 4);
    if (s->UnlockPalette() < 0)
        return -1;

    m_tail = m_head = -1;

    int i;
    for (i = 0; i < m_numSlots; i++)
    {
        Slot(i)->m_char = 0xFFFF;
        if (i)
            Slot(i)->m_prev = i - 1;
        else
        {
            m_head = 0;
            Slot(i)->m_prev = -1;
        }
        if (i != m_numSlots - 1)
            Slot(i)->m_next = i + 1;
        else
        {
            m_tail = i;
            Slot(i)->m_next = -1;
        }
    }

    for (i = 0; i < m_numCharsLoaded; i++)
        Char(i)->m_slot = 0xFFFF;
    return 0;
}

int VIRFont::Cache(VIRaster &r, const char *s, int max, int *out)
{
    int           stride;
    VIPixelStore  ps;
    unsigned char *pixels;
    int           n;
    int           locked = 0;

    n = 0;

    VISurface *surf = r.Surface(m_surface);
    float invW = 1.0f / (float)surf->m_width;
    float invH = 1.0f / (float)surf->m_height;

    while (*s && n < max)
    {
        int c = FindChar((unsigned char)*s);
        if (c < 0)
        {
            c = FindChar('?');
            if (c < 0)
                return -1;
        }

        int slot = Char(c)->m_slot;
        if (slot == 0xFFFF)
        {
            slot = m_head;
            if (Slot(slot)->m_char != 0xFFFF)
                Char(Slot(slot)->m_char)->m_slot = 0xFFFF;
            Char(c)->m_slot = slot;
            Slot(slot)->m_char = c;

            Slot(slot)->m_uv[0].Set((float)Slot(slot)->m_pos.x * invW,
                                    (float)Slot(slot)->m_pos.y * invH);
            Slot(slot)->m_uv[1].Set((float)(Slot(slot)->m_pos.x + Char(c)->m_width) * invW,
                                    (float)Slot(slot)->m_pos.y * invH);
            Slot(slot)->m_uv[2].Set((float)(Slot(slot)->m_pos.x + Char(c)->m_width) * invW,
                                    (float)(Slot(slot)->m_pos.y + m_charHeight) * invH);
            Slot(slot)->m_uv[3].Set((float)Slot(slot)->m_pos.x * invW,
                                    (float)(Slot(slot)->m_pos.y + m_charHeight) * invH);

            if (!locked)
            {
                if (surf->LockMipLevel(0, stride, ps, &pixels) < 0)
                    return -1;
                locked = 1;
            }

            unsigned char *src = m_data + Char(c)->m_offset;
            unsigned char *dst = pixels + Slot(slot)->m_offset;
            int rowBytes = Char(c)->m_bytesPerRow;

            for (int y = 0; y < m_charHeight; y++)
            {
                memcpy(dst, src, rowBytes);
                src = src + rowBytes;
                dst = dst + stride;
            }
        }

        if (Slot(slot)->m_prev != -1)
            Slot(Slot(slot)->m_prev)->m_next = Slot(slot)->m_next;
        else
            m_head = Slot(slot)->m_next;
        if (Slot(slot)->m_next != -1)
            Slot(Slot(slot)->m_next)->m_prev = Slot(slot)->m_prev;
        else
            m_tail = Slot(slot)->m_prev;

        Slot(m_tail)->m_next = slot;
        Slot(slot)->m_next = -1;
        Slot(slot)->m_prev = m_tail;
        m_tail = slot;

        out[n] = c;
        n++;
        s++;
    }

    if (m_numSlots < n)
        return -1;
    if (locked)
    {
        if (surf->UnlockMipLevel(0) < 0)
            return -1;
    }
    return n;
}

int VIRFont::Cache(VIRaster &r, const unsigned short *s, int max, int *out)
{
    int           stride;
    VIPixelStore  ps;
    unsigned char *pixels;
    int           n;
    int           locked = 0;

    n = 0;

    VISurface *surf = r.Surface(m_surface);
    float invW = 1.0f / (float)surf->m_width;
    float invH = 1.0f / (float)surf->m_height;

    while (*s && n < max)
    {
        int c = FindChar(*s);
        if (c < 0)
        {
            c = FindChar('?');
            if (c < 0)
                return -1;
        }

        int slot = Char(c)->m_slot;
        if (slot == 0xFFFF)
        {
            slot = m_head;
            if (Slot(slot)->m_char != 0xFFFF)
                Char(Slot(slot)->m_char)->m_slot = 0xFFFF;
            Char(c)->m_slot = slot;
            Slot(slot)->m_char = c;

            Slot(slot)->m_uv[0].Set((float)Slot(slot)->m_pos.x * invW,
                                    (float)Slot(slot)->m_pos.y * invH);
            Slot(slot)->m_uv[1].Set((float)(Slot(slot)->m_pos.x + Char(c)->m_width) * invW,
                                    (float)Slot(slot)->m_pos.y * invH);
            Slot(slot)->m_uv[2].Set((float)(Slot(slot)->m_pos.x + Char(c)->m_width) * invW,
                                    (float)(Slot(slot)->m_pos.y + m_charHeight) * invH);
            Slot(slot)->m_uv[3].Set((float)Slot(slot)->m_pos.x * invW,
                                    (float)(Slot(slot)->m_pos.y + m_charHeight) * invH);

            if (!locked)
            {
                if (surf->LockMipLevel(0, stride, ps, &pixels) < 0)
                    return -1;
                locked = 1;
            }

            unsigned char *src = m_data + Char(c)->m_offset;
            unsigned char *dst = pixels + Slot(slot)->m_offset;
            int rowBytes = Char(c)->m_bytesPerRow;

            for (int y = 0; y < m_charHeight; y++)
            {
                memcpy(dst, src, rowBytes);
                src = src + rowBytes;
                dst = dst + stride;
            }
        }

        if (Slot(slot)->m_prev != -1)
            Slot(Slot(slot)->m_prev)->m_next = Slot(slot)->m_next;
        else
            m_head = Slot(slot)->m_next;
        if (Slot(slot)->m_next != -1)
            Slot(Slot(slot)->m_next)->m_prev = Slot(slot)->m_prev;
        else
            m_tail = Slot(slot)->m_prev;

        Slot(m_tail)->m_next = slot;
        Slot(slot)->m_next = -1;
        Slot(slot)->m_prev = m_tail;
        m_tail = slot;

        out[n] = c;
        n++;
        s++;
    }

    if (m_numSlots < n)
        return -1;
    if (locked)
    {
        if (surf->UnlockMipLevel(0) < 0)
            return -1;
    }
    return n;
}
