// VISurface -- SUPPORT 0x0040DE50 .. 0x0040E780.
//
// The texture surface, and VIMaterial's other half: a mip chain plus an
// optional CLUT carved out of ONE allocation, each section preceded by a
// 64-byte VIF/GIF header so an upload is a pointer and a qword count.  The
// headers are what make this a new corner of the same build -- 64-bit `sd`
// stores of GIFtags built from `dsll32` constants, next to the ordinary
// integer code of the offset walk.
#include "vi.h"

VISurface::VISurface()
{
    // Field order, and the same value-hoist as Deallocate: `m_lockLevel = -1`
    // is the last of the three -1 stores and `m_dirty = 1` the only 1, so the
    // three constants' stores come out at the head of the block whatever else
    // is written between them.  m_paletteHeader is the one field out of order.
    m_width = 0;
    m_height = 0;
    m_pixelStore = VIPixelStoreRGBA32;
    m_index = -1;
    m_cacheIndex = -1;
    m_lockLevel = -1;
    m_dirty = 1;
    m_pad24 = 0;
    m_numMipLevels = 0;
    m_dataSize = 0;
    m_data = 0;
    m_paletteHeader = 0;
    for (int i = 9; i >= 0; i--)
        m_levels[i].m_header = 0;
}

VISurface::~VISurface()
{
    Deallocate();
}

int VISurface::Init(VIRaster &r, int w, int h, VIBitDepth bd, int levels)
{
    // The VIRaster & is DISCARDED, exactly as VIPrimBuffer::Init discards
    // its own (Phase 14): the first call takes `this` alone and nothing ever
    // reads `r`.  It has to be Deallocate() rather than anything taking the
    // raster, and that is not a guess -- with `r` live to a call the four
    // parameters allocate s1/s2 the other way round and six words go wrong.
    if (Deallocate() < 0)
        return -1;
    m_width = w;
    m_height = h;
    m_numMipLevels = levels;
    switch (bd)
    {
    case VIBitDepth32:
        m_pixelStore = VIPixelStoreRGBA32;
        m_paletteStore = VIPixelStoreRGBA32;
        m_paletteCount = 0;
        break;
    case VIBitDepth16:
        m_pixelStore = VIPixelStoreRGBA16;
        m_paletteStore = VIPixelStoreRGBA32;
        m_paletteCount = 0;
        break;
    case VIBitDepth8:
        m_pixelStore = VIPixelStoreI8;
        m_paletteCount = 256;
        m_paletteStore = VIPixelStoreRGBA32;
        break;
    case VIBitDepth4:
        m_pixelStore = VIPixelStoreI4;
        m_paletteCount = 16;
        m_paletteStore = VIPixelStoreRGBA32;
        break;
    case VIBitDepthNone:
        break;
    default:
        return -1;
    }
    if (CalcSurfaceOffsets() < 0)
        return -1;
    if (Allocate() < 0)
        return -1;
    if (SetDMAHeaders() < 0)
        return -1;
    return 0;
}

int VISurface::LockMipLevel(int level, int &stride, VIPixelStore &ps, unsigned char **p)
{
    if (m_lockLevel != -1)
        return -1;
    m_dirty = 1;
    m_lockLevel = level;
    ps = m_pixelStore;
    stride = m_levels[level].m_stride;
    *p = (unsigned char *)((int)m_data + m_levels[level].m_pixels);
    return 0;
}

int VISurface::UnlockMipLevel(int level)
{
    if (m_lockLevel != level)
        return -1;
    m_lockLevel = -1;
    return 0;
}

int VISurface::LockPalette(int &count, VIPixelStore &ps, unsigned char **p)
{
    if (m_lockLevel != -1)
        return -1;
    ReorderPalette();
    m_dirty = 1;
    m_lockLevel = m_numMipLevels;
    if (m_paletteCount == 0)
    {
        count = 0;
        ps = m_pixelStore;
        *p = 0;
    }
    else
    {
        count = m_paletteCount;
        ps = m_paletteStore;
        *p = (unsigned char *)((int)m_data + m_paletteOffset);
    }
    return 0;
}

int VISurface::UnlockPalette()
{
    if (m_lockLevel != m_numMipLevels)
        return -1;
    ReorderPalette();
    m_lockLevel = -1;
    return 0;
}

int VISurface::Allocate()
{
    if (m_dataSize)
    {
        m_data = new unsigned char[m_dataSize];
        if (m_data == 0)
            return -1;
    }
    return 0;
}

int VISurface::Deallocate()
{
    if (m_data)
    {
        delete [] m_data;
        // Field order, and the two closing stores the other way round.  What
        // decides it is the value-hoist: gcc emits the LAST store of each
        // distinct constant first, in source order, and the rest after it in
        // source order -- so `m_lockLevel = -1` has to come after both -1
        // stores and `m_dataSize = 0` after every other zero.  (Phase 13 saw
        // the tail of this rule as "the block's last store is hoisted to the
        // front"; there was only one constant in that block.)
        m_width = 0;
        m_height = 0;
        m_pixelStore = VIPixelStoreRGBA32;
        m_index = -1;
        m_cacheIndex = -1;
        m_lockLevel = -1;
        m_dirty = 1;
        m_numMipLevels = 0;
        m_data = 0;
        m_dataSize = 0;
    }
    return 0;
}

// The one open function.  Its shape is settled -- the decision tree, the case
// bodies, the 4-term loop condition and its exit order, the two mip cursors and
// the tail -- but the target computes `align(m_paletteCount * bpp)` TWICE, with
// a fresh load of the count after the first align's if/else joins, where ours
// computes it once and carries it across the join in a spare register.  That is
// the standing phenomenon of Phases 4-6 verbatim ("neither merges two identical
// memory references nor keeps a value live where ours does"), and it is not a
// flag: -fno-gcse, -fno-cse-follow-jumps, -fno-rerun-cse-after-loop,
// -fno-expensive-optimizations, -fno-thread-jumps, -fno-cse-skip-blocks and
// -fno-regmove all leave it alone.  The same divergence folds the loop's
// `size + 16` back onto the raw product (`+80`) and moves the first mip
// cursor's base from +76 to +84 -- one cause, three symptoms.
int VISurface::CalcSurfaceOffsets()
{
    int offset = 0;
    switch (m_paletteStore)
    {
    case VIPixelStoreRGBA32:
        m_paletteSize = 64;
        offset = 64;
        m_paletteOffset = offset;
        m_paletteHeader = 0;
        // The align-up as one ternary per use, the way a macro writes it: gcc
        // distributes the +64 into both arms (+80 / +64), and the second use
        // re-reads m_paletteCount after the m_paletteSize store.
        m_paletteSize = ((m_paletteCount * 4) & 15
                         ? (m_paletteCount * 4) - ((m_paletteCount * 4) & 15) + 16
                         : (m_paletteCount * 4)) + 64;
        offset = ((m_paletteCount * 4) & 15
                  ? (m_paletteCount * 4) - ((m_paletteCount * 4) & 15) + 16
                  : (m_paletteCount * 4)) + 64;
        m_paletteQWC = m_paletteSize >> 4;
        break;
    case VIPixelStoreRGBA16:
        m_paletteSize = 64;
        offset = 64;
        m_paletteOffset = offset;
        m_paletteHeader = 0;
        m_paletteSize = ((m_paletteCount * 2) & 15
                         ? (m_paletteCount * 2) - ((m_paletteCount * 2) & 15) + 16
                         : (m_paletteCount * 2)) + 64;
        offset = ((m_paletteCount * 2) & 15
                  ? (m_paletteCount * 2) - ((m_paletteCount * 2) & 15) + 16
                  : (m_paletteCount * 2)) + 64;
        m_paletteQWC = m_paletteSize >> 4;
        break;
    default:
        m_paletteHeader = 0;
        m_paletteOffset = 0;
        m_paletteSize = 0;
        m_paletteQWC = 0;
        break;
    }
    int level = 0;
    int w = m_width;
    int h = m_height;
    // Four terms, and the level cap is one of them rather than a `break` at
    // the bottom: the exit tests come out w, h, 10, m_numMipLevels in exactly
    // that order, and `level < 10` folds away in the loop's entry guard.
    while (w >= 16 && h >= 16 && level < 10 && level < m_numMipLevels)
    {
        int stride = CalcMipLevelStride(w);
        if (stride < 0)
            return -1;
        int size = stride * h;
        size += 64;
        m_levels[level].m_header = offset;
        m_levels[level].m_pixels = offset + 64;
        m_levels[level].m_stride = stride;
        m_levels[level].m_width = w;
        m_levels[level].m_height = h;
        // Phase 14's fresh-local-for-the-intermediate, in a diamond: the
        // result goes to its OWN local and back, which is what puts a
        // `move` in each arm.  A plain `if (size & 15) size = ...;` writes
        // size in place and loses both.
        int t;
        if (size & 15)
        {
            int u = size + 16;
            t = u - (size & 15);
        }
        else
            t = size;
        size = t;
        m_levels[level].m_qwc = size >> 4;
        w = w >> 1;
        h = h >> 1;
        if (w < 1)
            w = 1;
        if (h < 1)
            h = 1;
        offset = offset + size;
        level++;
    }
    m_numMipLevels = level;
    m_dataSize = offset;
    return 0;
}

int VISurface::SetDMAHeaders()
{
    unsigned int *p;
    if (m_paletteCount)
    {
        unsigned char *h = m_data + m_paletteHeader;
        m_paletteHeader = (int)h;
        p = (unsigned int *)h;
        p[0] = 0;
        p[1] = 0;
        p[2] = 0;
        p[3] = (m_paletteQWC - 1) | 0x50000000;
        p += 4;
        *(unsigned long *)&p[0] = 0x1000000000000001;
        *(unsigned long *)&p[2] = 14;
        p += 4;
        *(unsigned long *)&p[0] = 0;
        *(unsigned long *)&p[2] = 83;
        p += 4;
        *(unsigned long *)&p[0] = (unsigned long)((m_paletteQWC - 4) | 0x8000) | 0x0800000000000000;
        *(unsigned long *)&p[2] = 0;
    }
    for (int i = 0; i < m_numMipLevels; i++)
    {
        unsigned char *h = m_data + m_levels[i].m_header;
        m_levels[i].m_header = (int)h;
        p = (unsigned int *)h;
        p[0] = 0;
        p[1] = 0;
        p[2] = 0;
        p[3] = (m_levels[i].m_qwc - 1) | 0x50000000;
        p += 4;
        *(unsigned long *)&p[0] = 0x1000000000000001;
        *(unsigned long *)&p[2] = 14;
        p += 4;
        *(unsigned long *)&p[0] = 0;
        *(unsigned long *)&p[2] = 83;
        p += 4;
        *(unsigned long *)&p[0] = (unsigned long)((m_levels[i].m_qwc - 4) | 0x8000) | 0x0800000000000000;
        *(unsigned long *)&p[2] = 0;
    }
    return 0;
}

int VISurface::CalcMipLevelStride(int w)
{
    switch (m_pixelStore)
    {
    case VIPixelStoreI4:
        if (w & 1)
            return (w >> 1) + 1;
        return w >> 1;
    case VIPixelStoreI8:
        return w;
    case VIPixelStoreRGBA32:
        return w << 2;
    case VIPixelStoreRGB24:
        return (w << 1) + w;
    case VIPixelStoreRGBA16:
        return w << 1;
    default:
        return -1;
    }
}

int VISurface::ReorderPalette()
{
    // The CLUT swizzle: entries 8..15 of every 32 trade places with 16..23,
    // which is the order the GS reads a 256-entry PSMCT32 CLUT in.
    //
    // Case 16 is written FIRST even though the decision tree tests 2 first --
    // a switch emits its comparisons in value order and its BODIES in source
    // order, and the target's halfword loop is the one that falls out of the
    // tree.  The three `return 0`s are one `break` to the bottom: an
    // unconditional branch to a return block is turned back into a return
    // (so each loop ends `jr ra; move v0,zero`), while the two conditional
    // ones stay branches -- which is what lets reorg steal `move v0,zero` out
    // of the exit block into the first bne's delay slot.
    if (m_pixelStore == VIPixelStoreI8)
    {
        switch (m_paletteStore)
        {
        case VIPixelStoreRGBA16:
        for (int i = 0; i < m_paletteCount; i += 32)
        {
            unsigned short *a = (unsigned short *)((int)m_data + (m_paletteOffset + (i + 8) * 2));
            unsigned short *b = (unsigned short *)((int)m_data + (m_paletteOffset + (i + 16) * 2));
            for (int j = 7; j >= 0; j--)
            {
                unsigned short t = *a;
                *a = *b;
                *b = t;
                a++;
                b++;
            }
        }
        break;
        case VIPixelStoreRGBA32:
        for (int i = 0; i < m_paletteCount; i += 32)
        {
            unsigned int *a = (unsigned int *)((int)m_data + (m_paletteOffset + (i + 8) * 4));
            unsigned int *b = (unsigned int *)((int)m_data + (m_paletteOffset + (i + 16) * 4));
            for (int j = 7; j >= 0; j--)
            {
                unsigned int t = *a;
                *a = *b;
                *b = t;
                a++;
                b++;
            }
        }
        break;
        default:
            return -1;
        }
    }
    return 0;
}
