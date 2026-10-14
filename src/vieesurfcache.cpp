// VIEESurfCache -- SUPPORT 0x0040E780.., the GS VRAM allocator and the EE
// half of every texture upload.  Phase 40.
//
// Three containers over one span of video memory:
//
//   m_buckets   VIPool<VIEEBucket>            the runs, in ADDRESS order
//   m_infos     VIMap<int, VIEESurfCacheInfo> resident surface -> where it is
//   m_freeMap   VIMultiMap<unsigned, int>     (psm << 24) | size -> bucket
//
// plus an LRU threaded through the infos so a miss can evict.  The free map's
// key is what makes the whole thing work: a LowerBound on (psm, size) finds
// the smallest run ALREADY formatted for the requested pixel mode, and a
// second LowerBound on (255, size) falls back to memory that has never been
// handed out and so may be reformatted.  A GS page is 8192 bytes and its
// pixel layout depends on the psm, which is why every split and every merge
// in here is page-aware.
//
// Open: Set, CalcCacheInfo and ConstructDMAPacket; Deallocate is registers
// only (see its comment).  Allocate is byte-exact.
#include "vi.h"

int VIEESurfCache::Key;

int VIEESurfCache::Init(int base, int end)
{
    m_base = base;
    m_end = end;
    m_noDownloads = 0;
    if (Clear() < 0)
        return -1;
    return 0;
}

int VIEESurfCache::Clear()
{
    if (m_freeMap.Clear() < 0)
        return -1;
    if (m_buckets.Clear() < 0)
        return -1;
    if (m_infos.Clear() < 0)
        return -1;

    int a;
    if (m_base & 8191)
    {
        int t = m_base + 8192;
        a = t - (m_base & 8191);
    }
    else
        a = m_base;
    if ((m_end >> 13) < (a >> 13))
        return -1;
    int n = ((m_end >> 13) - (a >> 13)) << 13;

    m_size = n;
    VIEEBucket b;
    b.m_addr = a;
    b.m_size = n;
    b.m_psm = 255;
    b.m_free = -1;
    if (m_buckets.Add(b) < 0)
        return -1;
    m_lruFirst = -1;
    m_lruLast = -1;
    m_used = 0;
    m_numDownloads = 0;
    if (AddToFreeMap(m_buckets.Begin()) < 0)
        return -1;
    return 0;
}

// EnableDownloads(1) CLEARS the flag: the member is the negative.
int VIEESurfCache::EnableDownloads(int on)
{
    m_noDownloads = (on == 0);
    return 0;
}

// Bit count, not a logarithm: Log2(1) is 1 and Log2(0) is 0.  Set subtracts
// one from each result to get TEX0's TW/TH.
int VIEESurfCache::Log2(int n)
{
    int r = 0;
    while (n > 0)
    {
        n >>= 1;
        r++;
    }
    return r;
}

int VIEESurfCache::AddToFreeMap(int i)
{
    if (m_buckets.Value(i).m_free >= 0)
        return -1;
    unsigned int k = ((unsigned int)m_buckets.Value(i).m_psm << 24)
                   | m_buckets.Value(i).m_size;
    int h = m_freeMap.Insert(k, i);
    if (h < 0)
        return -1;
    m_buckets.Value(i).m_free = h;
    return 0;
}

int VIEESurfCache::RemoveFromFreeMap(int i)
{
    if (m_buckets.Value(i).m_free < 0)
        return -1;
    if (m_freeMap.Erase(m_buckets.Value(i).m_free) < 0)
        return -1;
    m_buckets.Value(i).m_free = -1;
    return 0;
}

int VIEESurfCache::MoveToLRUEnd(int i)
{
    if (m_lruLast != i)
    {
        if (m_lruFirst == -1)
        {
            m_infos.Value(i).m_lruNext = -1;
            m_infos.Value(i).m_lruPrev = -1;
            m_lruFirst = i;
            m_lruLast = i;
        }
        else
        {
            if (m_lruFirst == i)
            {
                m_infos.Value(m_infos.Value(i).m_lruNext).m_lruPrev = -1;
                m_lruFirst = m_infos.Value(i).m_lruNext;
            }
            else
            {
                if (m_infos.Value(i).m_lruPrev != -1)
                    m_infos.Value(m_infos.Value(i).m_lruPrev).m_lruNext =
                        m_infos.Value(i).m_lruNext;
                if (m_infos.Value(i).m_lruNext != -1)
                    m_infos.Value(m_infos.Value(i).m_lruNext).m_lruPrev =
                        m_infos.Value(i).m_lruPrev;
            }
            m_infos.Value(m_lruLast).m_lruNext = i;
            m_infos.Value(i).m_lruPrev = m_lruLast;
            m_infos.Value(i).m_lruNext = -1;
            m_lruLast = i;
        }
    }
    return 0;
}

int VIEESurfCache::RemoveFromLRU(int i)
{
    if (m_lruFirst == i)
        m_lruFirst = m_infos.Value(i).m_lruNext;
    else
        m_infos.Value(m_infos.Value(i).m_lruPrev).m_lruNext =
            m_infos.Value(i).m_lruNext;
    if (m_lruLast != i)
        m_infos.Value(m_infos.Value(i).m_lruNext).m_lruPrev =
            m_infos.Value(i).m_lruPrev;
    else
        m_lruLast = m_infos.Value(i).m_lruPrev;
    m_infos.Value(i).m_lruNext = -1;
    m_infos.Value(i).m_lruPrev = -1;
    return 0;
}

// The entry point VIRaster::Process calls per material pass.  Returns 0 when
// the surface was already resident and 1 when it had to be uploaded.
int VIEESurfCache::Cache(VISurface &s, VIMaterial &m, VIMaterialPass &p,
                         VIRaster &r, _sceDmaTag **tag, int noLRU)
{
    if (s.m_dirty)
        if (Decache(s) < 0)
            return -1;
    int i = s.m_cacheIndex;
    if (i != -1)
    {
        int n = i;
        if (noLRU == 0)
            MoveToLRUEnd(i);
        else
            RemoveFromLRU(i);
        if (m.m_dirty != 0 || m_infos.Value(n).m_stamp != p.m_stamp)
        {
            if (Set(s, m_infos.Value(n), p) < 0)
                return -1;
            m.m_dirty = 1;
            p.m_stamp = m_infos.Value(n).m_stamp;
        }
        return 0;
    }

    VIEESurfCacheInfo c;
    if (CalcCacheInfo(s, c) < 0)
        return -1;
    if (s.m_paletteCount != 0)
    {
        if (FindLRU(c.m_clut, r) < 0)
            return -1;
        c.m_clutAddr += m_buckets.Value(c.m_clut.m_bucket).m_addr;
        if (Allocate(c.m_clut) < 0)
            return -1;
    }
    if (FindLRU(c.m_tex, r) < 0)
        return -1;
    for (int k = 0; k < c.m_numLevels; k++)
        c.m_levelAddr[k] += m_buckets.Value(c.m_tex.m_bucket).m_addr;
    if (Allocate(c.m_tex) < 0)
        return -1;

    int key = s.m_index;
    int j = m_infos.Insert(key, c);
    if (j < 0)
        return -1;
    s.m_cacheIndex = j;
    if (noLRU == 0)
        MoveToLRUEnd(j);
    if (m_noDownloads == 0)
        if (Download(s, m_infos.Value(j), tag) < 0)
            return -1;
    m.m_dirty = 1;
    p.m_stamp = m_infos.Value(j).m_stamp;
    if (Set(s, m_infos.Value(j), p) < 0)
        return -1;
    s.m_dirty = 0;
    return 1;
}

int VIEESurfCache::Decache(VISurface &s)
{
    int i = s.m_cacheIndex;
    if (i != -1)
    {
        int j = i;
        if (m_infos.Value(j).m_clut.m_size != 0)
            if (Deallocate(m_infos.Value(j).m_clut) < 0)
                return -1;
        if (Deallocate(m_infos.Value(j).m_tex) < 0)
            return -1;
        if (RemoveFromLRU(i) < 0)
            return -1;
        s.m_cacheIndex = -1;
        if (m_infos.Erase(i) < 0)
            return -1;
    }
    return 0;
}

// 0 when nothing in the map can hold the request.  The fallback key's psm is
// 255, which orders every virgin run after every formatted one.
int VIEESurfCache::FindFree(VIEESurfAlloc &a)
{
    // Two copies of the success tail, which jump2 cross-jumps into one: each
    // reads Value() through its own path's &m_freeMap.
    unsigned int k = ((unsigned int)a.m_psm << 24) | a.m_size;
    int i = m_freeMap.LowerBound(k);
    if (i != m_freeMap.End() && (m_freeMap.Key(i) >> 24) == (unsigned int)a.m_psm)
    {
        a.m_bucket = m_freeMap.Value(i);
        return 1;
    }
    k = a.m_size | 0xff000000;
    i = m_freeMap.LowerBound(k);
    if (i != m_freeMap.End())
    {
        a.m_bucket = m_freeMap.Value(i);
        return 1;
    }
    return 0;
}

// Evict from the head of the LRU until FindFree succeeds.
int VIEESurfCache::FindLRU(VIEESurfAlloc &a, VIRaster &r)
{
    // Returns FindFree's OWN answer, so a success is 1 and not 0.
    int f;
    while ((f = FindFree(a)) == 0)
    {
        if (m_lruFirst == -1)
            return -1;
        if (m_infos.Value(m_lruFirst).m_clut.m_size != 0)
            if (Deallocate(m_infos.Value(m_lruFirst).m_clut) < 0)
                return -1;
        if (Deallocate(m_infos.Value(m_lruFirst).m_tex) < 0)
            return -1;
        int i = m_lruFirst;
        if (RemoveFromLRU(i) < 0)
            return -1;
        r.Surface(m_infos.Key(i))->m_cacheIndex = -1;
        if (m_infos.Erase(i) < 0)
            return -1;
    }
    return f;
}

// Carve a.m_size out of the bucket FindFree picked.  A virgin run is split at
// the first page boundary past the allocation so the rest stays virgin; what
// is left inside the page takes the allocation's own psm.
int VIEESurfCache::Allocate(VIEESurfAlloc &a)
{
    int i = a.m_bucket;
    int n = m_freeMap.Key(m_buckets.Value(i).m_free) & 0x00ffffff;
    if (RemoveFromFreeMap(i) < 0)
        return -1;
    if (m_buckets.Value(i).m_psm == 255)
    {
        if (n - a.m_size >= 8192)
        {
            // An if/ELSE, not `if (p & 8191) p = ...`: the join label then
            // has two uses, cse's extended block ends there instead of
            // skipping the arm, and everything after it is recomputed --
            // gcse's reaching register for &m_buckets is the extra s1.
            int q = m_buckets.Value(i).m_addr + a.m_size;
            int p;
            if (q & 8191)
            {
                int t = q + 8192;
                p = t - (q & 8191);
            }
            else
                p = q;
            VIEEBucket b;
            b.m_addr = p;
            b.m_size = n - (p - m_buckets.Value(i).m_addr);
            b.m_psm = 255;
            b.m_free = -1;
            int k = m_buckets.Insert(b, m_buckets.Next(i));
            if (k < 0)
                return -1;
            if (AddToFreeMap(k) < 0)
                return -1;
            n -= b.m_size;
        }
    }
    if (a.m_size < n)
    {
        VIEEBucket b;
        b.m_addr = m_buckets.Value(i).m_addr + a.m_size;
        b.m_size = n - a.m_size;
        b.m_psm = a.m_psm;
        b.m_free = -1;
        int k = m_buckets.Insert(b, m_buckets.Next(i));
        if (k < 0)
            return -1;
        if (AddToFreeMap(k) < 0)
            return -1;
    }
    m_buckets.Value(i).m_size = a.m_size;
    m_buckets.Value(i).m_psm = a.m_psm;
    m_buckets.Value(i).m_free = -1;
    m_used += a.m_size;
    return 0;
}

// Give the run back and coalesce.  The neighbour merge is written out TWICE
// -- once with the run's own psm, and again after the page arithmetic has
// turned the whole pages inside it back into virgin memory, where neighbours
// that were not mergeable before now are.
//
// OPEN, 14/288, registers only (phase 61 wave 6, gsim): in the second page
// test the target has -8192 in a0, `addr & ~8191` in a2 and `s & 8191` in a1;
// we give a2/a1/a0.  gsim reproduces the target exactly when the -8192
// allocno (or `i * 24`, the bucket scale) carries an a0 PREFERENCE: then the
// two higher-priority allocnos skip a0 as "preferred by someone" and the
// constant takes it.  Nothing in the final code can supply that preference --
// both are only ever the SECOND operand, and set_preference reads the first
// -- so the original has an insn involving a0 that a later pass deletes.
int VIEESurfCache::Deallocate(VIEESurfAlloc &a)
{
    int i = a.m_bucket;
    // ONE `k` for the first Prev() and both Insert() results, ONE `q` for
    // both Next() results: merged, each outranks its rival in global-alloc
    // (k takes s0 ahead of &m_buckets, q takes s1 ahead of gcse's copy of it).
    int k;

    if (i != m_buckets.Begin())
    {
        k = m_buckets.Prev(i);
        if (m_buckets.Value(k).m_free != -1
            && m_buckets.Value(k).m_psm == m_buckets.Value(i).m_psm)
        {
            if (RemoveFromFreeMap(k) < 0)
                return -1;
            m_buckets.Value(k).m_size += m_buckets.Value(i).m_size;
            if (m_buckets.Erase(i) < 0)
                return -1;
            i = k;
        }
    }

    int q = m_buckets.Next(i);
    if (q != m_buckets.End())
    {
        if (m_buckets.Value(q).m_free != -1
            && m_buckets.Value(q).m_psm == m_buckets.Value(i).m_psm)
        {
            if (RemoveFromFreeMap(q) < 0)
                return -1;
            m_buckets.Value(i).m_size += m_buckets.Value(q).m_size;
            if (m_buckets.Erase(q) < 0)
                return -1;
        }
    }

    if ((m_buckets.Value(i).m_addr & ~8191) != m_buckets.Value(i).m_addr)
    {
        // An if/ELSE (the target's `move` in the `beqz` delay slot): its join
        // ends cse's extended block, so &m_buckets.Value(i) is recomputed
        // after it.  The end is named so the test and b.m_size share it.
        int addr = m_buckets.Value(i).m_addr;
        int p;
        if (addr & 8191)
        {
            int t = addr + 8192;
            p = t - (addr & 8191);
        }
        else
            p = addr;
        int end = m_buckets.Value(i).m_addr + m_buckets.Value(i).m_size;
        if (p < end)
        {
            VIEEBucket b;
            b.m_addr = p;
            b.m_size = end - p;
            b.m_psm = a.m_psm;
            b.m_free = -1;
            m_buckets.Value(i).m_size -= b.m_size;
            k = m_buckets.Insert(b, m_buckets.Next(i));
            if (k < 0)
                return -1;
            if (AddToFreeMap(i) < 0)
                return -1;
            i = k;
        }
    }

    if ((m_buckets.Value(i).m_addr & ~8191) == m_buckets.Value(i).m_addr)
    {
        if (m_buckets.Value(i).m_size >= 8192)
        {
            // The rounded size is only computed when there is a remainder,
            // and compared against the size still in its register: the
            // target's `beqz` goes straight past the split.
            int s = m_buckets.Value(i).m_size;
            if (s & 8191)
            {
                int t = s + 8192;
                int n = t - (s & 8191);
                if (n != s)
                {
                    VIEEBucket b;
                    int end = m_buckets.Value(i).m_addr + m_buckets.Value(i).m_size;
                    b.m_addr = end & ~8191;
                    b.m_size = end - b.m_addr;
                    b.m_psm = a.m_psm;
                    b.m_free = -1;
                    m_buckets.Value(i).m_psm = 255;
                    m_buckets.Value(i).m_size -= b.m_size;
                    k = m_buckets.Insert(b, m_buckets.Next(i));
                    if (k < 0)
                        return -1;
                    if (AddToFreeMap(k) < 0)
                        return -1;
                }
            }
            m_buckets.Value(i).m_psm = 255;
        }

        if (m_buckets.Value(i).m_psm == 255)
        {
            if (i != m_buckets.Begin())
            {
                int p = m_buckets.Prev(i);
                if (m_buckets.Value(p).m_free != -1
                    && m_buckets.Value(p).m_psm == m_buckets.Value(i).m_psm)
                {
                    if (RemoveFromFreeMap(p) < 0)
                        return -1;
                    m_buckets.Value(p).m_size += m_buckets.Value(i).m_size;
                    if (m_buckets.Erase(i) < 0)
                        return -1;
                    i = p;
                }
            }
            q = m_buckets.Next(i);
            if (q != m_buckets.End())
            {
                if (m_buckets.Value(q).m_free != -1
                    && m_buckets.Value(q).m_psm == m_buckets.Value(i).m_psm)
                {
                    if (RemoveFromFreeMap(q) < 0)
                        return -1;
                    m_buckets.Value(i).m_size += m_buckets.Value(q).m_size;
                    if (m_buckets.Erase(q) < 0)
                        return -1;
                }
            }
        }
    }

    if (AddToFreeMap(i) < 0)
        return -1;
    m_used -= a.m_size;
    return 0;
}

// `this` is unused: everything comes off the surface.  The GS pixel storage
// mode is what the whole class allocates in, so this is where a VIPixelStore
// becomes a PSM -- 20 PSMT4, 19 PSMT8, 2 PSMCT16, 1 PSMCT24, 0 PSMCT32.  And
// it settles VIPixelStore 3 against 4: 3 takes PSMCT24 here while 4 is
// rejected outright, so 3 is 24-bit colour in a 32-bit slot (VISurface::
// CalcMipLevelStride gives it a four-byte stride) and 4 is the PACKED 24-bit
// form, which has a stride but cannot be uploaded.
int VIEESurfCache::CalcCacheInfo(VISurface &s, VIEESurfCacheInfo &c)
{
    if (s.m_paletteCount != 0)
    {
        switch (s.m_paletteStore)
        {
        case VIPixelStoreRGBA16:
            c.m_clutAddr = 0;
            c.m_clut.m_psm = 2;
            c.m_clut.m_size = s.m_paletteCount * 2;
            break;
        case VIPixelStore3:
            c.m_clut.m_psm = 1;
            c.m_clutAddr = 0;
            c.m_clut.m_size = s.m_paletteCount * 4;
            break;
        case VIPixelStoreRGBA32:
            c.m_clutAddr = 0;
            c.m_clut.m_psm = 0;
            c.m_clut.m_size = s.m_paletteCount * 4;
            break;
        default:
            return -1;
        }
        int p = c.m_clut.m_size;
        if (p & 255)
        {
            int t = p + 256;
            p = t - (p & 255);
        }
        c.m_clut.m_size = p;
    }
    else
    {
        c.m_clutAddr = 0;
        c.m_clut.m_psm = 0;
        c.m_clut.m_size = 0;
    }

    switch (s.m_pixelStore)
    {
    case VIPixelStoreI4:
        c.m_tex.m_psm = 20;
        break;
    case VIPixelStoreI8:
        c.m_tex.m_psm = 19;
        break;
    case VIPixelStoreRGBA16:
        c.m_tex.m_psm = 2;
        break;
    case VIPixelStore3:
        c.m_tex.m_psm = 1;
        break;
    case VIPixelStoreRGBA32:
        c.m_tex.m_psm = 0;
        break;
    default:
        return -1;
    }

    c.m_tex.m_size = 0;
    c.m_lruNext = -1;
    c.m_lruPrev = -1;
    c.m_stamp = -1;
    c.m_numLevels = 0;

    int w = s.m_width;
    int h = s.m_height;
    for (int i = 0; i < s.m_numMipLevels; i++)
    {
        if (w < 16 || h < 16)
            break;
        c.m_levelAddr[c.m_numLevels] = c.m_tex.m_size;
        int n, tw, th;
        switch (c.m_tex.m_psm)
        {
        case 20:
        {
            tw = VIMAX(w, 128);
            if (w > 128)
                th = VIMAX(h, 128);
            else
                th = h;
            n = (tw * th) >> 1;
            break;
        }
        case 19:
        {
            tw = VIMAX(w, 128);
            if (w > 128)
                th = VIMAX(h, 64);
            else
                th = h;
            n = tw * th;
            break;
        }
        case 2:
        {
            tw = VIMAX(w, 64);
            if (w > 64)
                th = VIMAX(h, 64);
            else
                th = h;
            n = (tw * th) << 1;
            break;
        }
        case 1:
        {
            tw = VIMAX(w, 64);
            if (w > 64)
                th = VIMAX(h, 32);
            else
                th = h;
            n = (tw * th) << 2;
            break;
        }
        case 0:
        {
            tw = VIMAX(w, 64);
            if (w > 64)
                th = VIMAX(h, 32);
            else
                th = h;
            n = (tw * th) << 2;
            break;
        }
        default:
            return -1;
        }
        int m;
        if (n & 255)
        {
            int t = n + 256;
            m = t - (n & 255);
        }
        else
            m = n;
        c.m_tex.m_size += m;
        c.m_numLevels++;
        w >>= 1;
        h >>= 1;
    }

    int q = c.m_tex.m_size;
    if (q & 8191)
    {
        int t = q + 8192;
        q = t - (q & 8191);
    }
    c.m_tex.m_size = q;
    return 0;
}

// Build the four GS registers the pass needs: TEX0_1, TEX1_1 and the two
// MIPTBPs, each as an A+D pair.  The mip-map base pointers are the only
// place the level addresses are used, and the psm decides the minimum TBW --
// an indexed texture's buffer is two 64-pixel units wide at least.
int VIEESurfCache::Set(VISurface &s, VIEESurfCacheInfo &c, VIMaterialPass &p)
{
    int clut;
    int h = s.m_height;
    int w = s.m_width;
    if (s.m_paletteCount)
        clut = 1;
    else
        clut = 0;

    int tbw;
    if (c.m_tex.m_psm != 19 && c.m_tex.m_psm != 20)
        tbw = VIMAX(w >> 6, 1);
    else
        tbw = VIMAX(w >> 6, 2);

    int tw = Log2(w);
    int th = Log2(h);

    long tex0 = ((long)c.m_levelAddr[0] >> 8)
              | ((long)tbw << 14)
              | ((long)c.m_tex.m_psm << 20)
              | ((long)(tw - 1) << 26)
              | ((long)(th - 1) << 30)
              | 0x400000000;
    if ((p.m_flags & 1) == 0)
        tex0 |= 0x800000000;
    long tex0c;
    if (clut)
        tex0c = tex0 | (((long)c.m_clutAddr >> 8) << 37)
                     | ((long)c.m_clut.m_psm << 51);
    else
        tex0c = tex0;
    long tex0d;
    if (clut)
        tex0d = tex0c | 0x2000000000000000;
    else
        tex0d = tex0c;
    p.m_reg[0] = tex0d;
    p.m_reg[1] = 6;

    // Both arms load the same -127.0f, which is what keeps this a BRANCH:
    // two assignments per arm, so gcc will not if-convert it to a movn.
    long mag;
    float k;
    if ((p.m_flags & 8) == 0)
    {
        k = -127.0f;
        mag = 352;
    }
    else
    {
        k = -127.0f;
        mag = 320;
    }
    // K is TEX1 bits 32..43: the LOD bias as a signed byte, shifted up.
    p.m_reg[2] = ((long)(c.m_numLevels - 1) << 2)
               | (((long)(char)(int)(p.m_lodBias * k) << 32) | mag);
    p.m_reg[3] = 20;

    int lo, lo1;
    int m1, m2, m3;
    if (c.m_tex.m_psm != 19 && c.m_tex.m_psm != 20)
    {
        lo = 1;
        lo1 = 0;
    }
    else
    {
        lo = 2;
        lo1 = 1;
    }
    m1 = ((w >> 7) > lo1 ? (w >> 7) : lo);
    m2 = ((w >> 8) > lo1 ? (w >> 8) : lo);
    m3 = ((w >> 9) > lo1 ? (w >> 9) : lo);
    p.m_reg[4] = ((long)c.m_levelAddr[1] >> 8)
               | ((long)m1 << 14)
               | (((long)c.m_levelAddr[2] >> 8) << 20)
               | ((long)m2 << 34)
               | (((long)c.m_levelAddr[3] >> 8) << 40)
               | ((long)m3 << 54);
    p.m_reg[5] = 52;

    if (c.m_tex.m_psm != 19 && c.m_tex.m_psm != 20)
    {
        lo = 1;
        lo1 = 0;
    }
    else
    {
        lo = 2;
        lo1 = 1;
    }
    m1 = ((w >> 10) > lo1 ? (w >> 10) : lo);
    m2 = ((w >> 11) > lo1 ? (w >> 11) : lo);
    m3 = ((w >> 12) > lo1 ? (w >> 12) : lo);
    if (c.m_tex.m_psm != 19 && c.m_tex.m_psm != 20)
        p.m_reg[6] = ((long)c.m_levelAddr[1] >> 8)
                   | ((long)m1 << 14)
                   | (((long)c.m_levelAddr[2] >> 8) << 20)
                   | ((long)m2 << 34)
                   | (((long)c.m_levelAddr[3] >> 8) << 40)
                   | ((long)m3 << 54);
    else
        p.m_reg[6] = ((long)c.m_levelAddr[4] >> 8)
                   | ((long)m1 << 14)
                   | (((long)c.m_levelAddr[5] >> 8) << 20)
                   | ((long)m2 << 34)
                   | (((long)c.m_levelAddr[6] >> 8) << 40)
                   | ((long)m3 << 54);
    p.m_reg[7] = 54;
    return 0;
}

// ConstructDMAPacket does the work; this stamps the info so a VIMaterialPass
// that already carries the same stamp can skip the whole path next frame.
int VIEESurfCache::Download(VISurface &s, VIEESurfCacheInfo &c,
                            _sceDmaTag **tag)
{
    if (ConstructDMAPacket(s, c, tag) < 0)
        return -1;
    c.m_stamp = Key++;
    m_numDownloads++;
    return 0;
}

// Build the VIF/GIF chain that uploads the palette and every mip level.  Each
// entry is a CNT tag carrying one VIF DIRECT of four quadwords -- a GIFtag
// plus three A+D pairs writing BITBLTBUF, TRXPOS and TRXREG -- followed by a
// REF tag pointing at the surface's own header, which is where TRXDIR and the
// IMAGE GIFtag live (VISurface::SetDMAHeaders built those).  The chain closes
// with a TEXFLUSH.
// Wave 7 (90, open): the structure matches; what is left is allocation.  The
// target's `tag` parameter copy is t4 and the packet walker t3; ours the other
// way round.  gsim: allocno 119 (tag, 36 refs, priority 9090) is taken before
// 128 (3684), so it gets t3; the target needs 128 first or t3/t1 busy for 119.
// Both put nine constants in s0..s8; ours hoists `lui 0x1100` into the loop.
int VIEESurfCache::ConstructDMAPacket(VISurface &s, VIEESurfCacheInfo &c,
                                      _sceDmaTag **tag)
{
    if (s.m_paletteCount != 0)
    {
        (*tag)->m_id = 16;
        (*tag)->m_addr = 0;
        (*tag)->m_qwc = 5;
        // VIRaster's packet idiom: the first VIF word and the GIF/A+D longs
        // off the base, the other VIF words through p -- the target folds the
        // base-relative ones and keeps p for the rest.
        char *base = (char *)*tag;
        *tag = (_sceDmaTag *)(base + 96);
        ((unsigned int *)base)[4] = 0;
        char *p = base + 16;
        ((unsigned int *)p)[3] = 0x50000004;
        ((unsigned int *)p)[2] = 0x11000000;
        ((unsigned int *)p)[1] = 0;
        ((long *)base)[4] = 0x1000000000008003;
        ((long *)base)[5] = 14;
        p = base + 48;
        if (c.m_tex.m_psm == 20)
        {
            ((long *)p)[0] = (((long)c.m_clutAddr >> 8) << 32) |
                             (((long)c.m_clut.m_psm << 56) | 0x1000000000000);
            ((long *)p)[1] = 80;
            ((long *)base)[8] = 0;
            ((long *)base)[9] = 81;
            ((long *)base)[10] = 0x200000008;
            ((long *)base)[11] = 82;
        }
        else
        {
            ((long *)p)[0] = (((long)c.m_clutAddr >> 8) << 32) |
                             (((long)c.m_clut.m_psm << 56) | 0x1000000000000);
            ((long *)p)[1] = 80;
            ((long *)base)[8] = 0;
            ((long *)base)[9] = 81;
            ((long *)base)[10] = 0x1000000010;
            ((long *)base)[11] = 82;
        }

        int ha = s.m_paletteHeader;
        int hq = s.m_paletteQWC;
        (*tag)->m_id = 48;
        (*tag)->m_addr = ha;
        (*tag)->m_qwc = hq;
        *tag = *tag + 1;
    }

    int w = s.m_width;
    int h = s.m_height;
    for (int k = 0; k < c.m_numLevels; k++)
    {
        (*tag)->m_id = 16;
        (*tag)->m_addr = 0;
        (*tag)->m_qwc = 5;
        _sceDmaTag *d = *tag;
        *tag = d + 6;
        VIEEXferPacket *x = (VIEEXferPacket *)(d + 1);
        x->m_vif[0] = 0;
        x->m_vif[1] = 0;
        // The flag survives as a register (`movz` on the count) and the test
        // is on it, false arm first; `f = 1; if (!count) f = 0;` is threaded
        // away instead.
        int f = 0;
        if (s.m_paletteCount)
            f = 1;
        if (!f)
            x->m_vif[2] = 0x11000000;
        else
            x->m_vif[2] = 0;
        x->m_vif[3] = 0x50000004;
        x->m_gif = 0x1000000000008003;
        x->m_regs = 14;
        // The A+D walker is set before the TBW choice: the target computes
        // it in that branch's delay slot.  The whole TEX0 value is written
        // in BOTH arms: each arm loads its own level address and compares,
        // and jump2 cross-jumps the identical tails from the `movn` on.
        VIGifAD *ad = x->m_ad;
        if (c.m_tex.m_psm != 19 && c.m_tex.m_psm != 20)
            ad->m_value = (((long)c.m_levelAddr[k] >> 8) << 32)
                        | ((long)VIMAX(w >> 6, 1) << 48)
                        | ((long)c.m_tex.m_psm << 56);
        else
            ad->m_value = (((long)c.m_levelAddr[k] >> 8) << 32)
                        | ((long)VIMAX(w >> 6, 2) << 48)
                        | ((long)c.m_tex.m_psm << 56);
        ad->m_reg = 80;
        ad++;
        ad->m_value = 0;
        ad->m_reg = 81;
        ad++;
        ad->m_value = (long)w | ((long)h << 32);
        ad->m_reg = 82;
        w >>= 1;
        h >>= 1;
        int la = s.m_levels[k].m_header;
        int lq = s.m_levels[k].m_qwc;
        (*tag)->m_id = 48;
        (*tag)->m_addr = la;
        (*tag)->m_qwc = lq;
        *tag = *tag + 1;
    }

    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 3;
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 64);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[3] = 0x50000002;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[1] = 0;
    ((long *)base)[4] = 0x1000000000008001;
    ((long *)base)[5] = 14;
    ((long *)base)[6] = 1;
    ((long *)base)[7] = 63;
    return 0;
}

// Shipped empty.
int VIEESurfCache::DebugVerify()
{
    return 0;
}
