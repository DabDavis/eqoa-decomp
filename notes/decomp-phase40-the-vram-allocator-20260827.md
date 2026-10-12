# Phase 40 — the GS VRAM allocator (`VIEESurfCache`)

**1841/2029 -> 1857/2052.** One new translation unit, `vieesurfcache.cpp`:
`VIEESurfCache` 15 of its 21 functions byte-exact, 12 of them on the FIRST
build, plus the two container pieces it needed —
`VIMultiMap<unsigned,int>::Key` and `::LowerBound` — and the two destructors
(`~VIMap<int,VIEESurfCacheInfo>`, `~VIMultiMap<unsigned,int>`) that had never
been instantiated. Same toolchain, same flags, no re-tuning.

## What the class is

`VIRaster + 0x3298`. The EE half of every texture upload: it owns one span of
GS video memory and hands out runs of it, then builds the VIF/GIF DMA chain
that fills them.

```
+0x00 m_base        +0x04 m_end          the span Init is handed
+0x08 m_lruFirst    +0x0C m_lruLast      an LRU over the resident surfaces
+0x10 m_noDownloads (EnableDownloads stores the COMPLEMENT)
+0x14 m_used        +0x18 m_size         +0x1C m_numDownloads
+0x20 VIPool<VIEEBucket>            m_buckets   the runs, in ADDRESS order
+0x38 VIMap<int, VIEESurfCacheInfo> m_infos     surface index -> where it is
+0x4C VIMultiMap<unsigned, int>     m_freeMap   (psm << 24) | size -> bucket
```

**The free map's key is the whole design.** A bucket carries the GS PIXEL
STORAGE MODE it is currently formatted for — 0 PSMCT32, 1 PSMCT24, 2 PSMCT16,
19 PSMT8, 20 PSMT4 — or **255, meaning virgin**, a run that has never been
handed out. `FindFree` does a `LowerBound` on `(psm, size)`, which finds the
smallest run ALREADY in the right format, and falls back to a second
`LowerBound` on `(255, size)`, which finds untouched memory that may be
reformatted. A GS page is 8192 bytes and its pixel layout depends on the psm,
so a run may only change format on page boundaries — and every split, merge
and alignment in `Allocate` and `Deallocate` follows from that one rule.

Layouts read off the code:

```
VIEEBucket        16   m_addr, m_size, m_psm, m_free (its free-map handle, -1 when used)
VIEESurfAlloc     12   m_bucket, m_psm, m_size
VIEESurfCacheInfo 84   VIEESurfAlloc m_clut; int m_clutAddr;
                       VIEESurfAlloc m_tex;  int m_numLevels;
                       int m_levelAddr[10];  int m_lruNext, m_lruPrev, m_stamp;
_sceDmaTag        16   unsigned short m_qwc; ...; unsigned char m_id; unsigned int m_addr;
```

## Corrections to the existing docs

**`VIMaterialLayer` is really `VIMaterialPass`** — `VIEESurfCache::Set`'s third
parameter mangles `R14VIMaterialPass`, and its stores land at +0x04, +0x34,
+0x48 and +0x50..+0x8F: exactly the class phase 15 modelled from
`VIMaterial`'s accessors. Its +0x48, guessed as "m_pass, -1 until
ConstructDMAPacket runs", is the **cache stamp**: `Cache` compares it against
the resident `VIEESurfCacheInfo`'s own +0x50 and skips the entire upload when
they agree. The 68-byte `m_packet` is four bytes of alignment plus **four GS
A+D pairs** — TEX0_1 (reg 6), TEX1_1 (0x14), MIPTBP1_1 (0x34), MIPTBP2_1
(0x36).

**`VISurface + 0x14` and `+0x18` are named now.** +0x18 is the cache index (-1
when not resident); +0x14 is the surface's own slot in the raster's pool,
which is the KEY of `m_infos` and what `FindLRU` hands back to
`VIRaster::Surface` to reach the evicted surface and clear its +0x18.

**`VIPixelStore` 3 and 4 are settled, and phase 30's correction was half
right.** `CalcCacheInfo`'s jump table (0x004EA590) maps store 3 to GS PSMCT24
and REJECTS store 4 outright, while `CalcMipLevelStride`'s table (0x004EA560)
gives 3 a four-byte stride and 4 a three-byte one. Both are true: 3 is 24-bit
colour in a 32-bit slot (uploadable as PSMCT24), 4 is the PACKED 24-bit form,
which has a stride but no upload path.

## Levers

**Which of two adjacent stores ends the block decides whether cross-jumping
eats it.** `MoveToLRUEnd`'s "empty list" arm and its main arm both end with
`m_lruLast = i`. Written `m_lruLast = i; m_lruFirst = i;` the delay-slot
filler steals the LAST store, leaving `m_lruLast = i` as the block's tail —
which matches the other arm's tail, and cross-jumping merges it away, one word
short. Written the other way round the block ends with `m_lruFirst = i`, no
common suffix exists, and the function is byte-exact. Phase 14 saw the same
rule from the other side.

**The alignment idiom is an if/ELSE, and the value is a fresh variable.** The
`p = (p + 8192) - (p & 8191)` shape appears five times in the class, always as

```c
int a;
if (m_base & 8191)
{
    int t = m_base + 8192;
    a = t - (m_base & 8191);
}
else
    a = m_base;
```

Written as `int a = m_base; if (a & 8191) a = ...;` gcc reassociates the
addition into `a - (m & -8192)` and coalesces the two pseudos — 27 wrong words
in `Clear` against 6. The `int t` is phase 14's fresh-local-for-the-
intermediate; the `else` is what keeps the unaligned value in its own
register.

**A SECOND LOCAL FOR THE SAME INDEX, three times.** `Decache`, `Cache` and
`FindLRU` all read an index once and then use it two ways: as a call argument
and as a container subscript. Writing `int j = i;` and using `j` for the
`Value(i)` reads while the calls take `i` reproduces the target's spare `move`
exactly — `Decache` 23 wrong words to 0, `Cache` 11 to 7. It is phase 12's
"a loop variable that is not the parameter is two spare moves", outside a
loop.

**A shared temporary is one stack slot.** `FindFree` builds two different
free-map keys; the target passes both from `sp+0`, so the source assigns to
ONE named `unsigned int k` rather than passing two expressions by const
reference.

**Two assignments per arm is what blocks if-conversion.** `Set`'s magnification
filter is an `if (p.m_flags & 8)` whose arms each set the filter bits AND the
LOD multiplier — and both arms load the same `-127.0f`. Written with one
assignment per arm gcc if-converts to `movn` and shares the constant; written
with two it branches and materialises `lui 0xc2fe; mtc1` in each arm, which is
what the target has. The same shape is why `Set`'s minimum-TBW selection
carries TWO locals (2/1 and 1/0) instead of one.

**`VIMAX(x, k)` emits `li k-1`** — phase 33's rule, five more times: gcc
weakens `x > k` to `x >= k` when the else value is `k` itself, so the constant
in the object code is one below the source's.

**A guard chain shares ONE `return 0` at the bottom** (phase 23) — `Decache`
and `MoveToLRUEnd` both needed the nested form, worth 25 and 36 words.

## Shipped oddities

- **`FindLRU` returns 1, not 0, on success**: it returns `FindFree`'s own
  answer. Every caller only tests `< 0`, so nothing notices.
- **`Cache` returns 1 when it uploaded and 0 when the surface was already
  resident** — the only place the distinction is visible.
- **`DebugVerify` is `return 0;`** and nothing else. Its 400-byte derived span
  is this TU's own local copies of `VIMultiMap<unsigned,int>`'s
  End/Size/Begin/Next/Prev/DestroyPool, so it needed a `SPAN_CAP` of 8; the
  destructor at the other end of the same TU needed one of 100.
- **`Set` writes MIPTBP2 from levels 1..3 for every non-indexed texture**
  instead of 4..6 — the indexed arm reads `m_levelAddr[4..6]` and the direct
  arm repeats MIPTBP1's `m_levelAddr[1..3]`. Matched as written.
- **`Set` puts TEX1's LOD constant K in bits 0..7**, on top of MMAG and MMIN,
  rather than the 32..43 the GS reads it from: the value is
  `(long)(signed char)(int)(lodBias * -127.0f)` with no shift. Harmless only
  because the bias is normally zero.
- **`CalcCacheInfo` ignores `this`** entirely.
- **`Deallocate` writes the coalesce twice**: once with the run's own psm and
  again after the page arithmetic has turned whole pages inside it back to
  virgin, where neighbours that could not merge before now can.

## Open

`Set` 78/204, `CalcCacheInfo` 23/192, `Cache` 7/179, `FindFree` 7/57 —
each one instruction of placement away. `Allocate` 99/135 and `Deallocate`
186/289 are structurally identical to the target and differ only in register
allocation: the target carries one more callee-saved register, a second
pseudo for `&m_buckets`. `ConstructDMAPacket` 160/228 is the standing
question below.

**The flag that will not stop being an `sltu`.** Twice — `Set`'s "has a
palette" and `ConstructDMAPacket`'s per-level twin — the target computes
`int f = 1; if (s.m_paletteCount == 0) f = 0;` as a COPY of a register holding
1 followed by `movz f,zero,count`, and then branches on `f`. Every spelling
tried (nine: the `if`, the `else` form, a ternary, `VIMIN`, `f--`, `long`,
`unsigned`, `bool`, and the count in a local) collapses to a single
`sltu f,zero,count` and the branch folds onto the count directly. It is worth
one instruction in each function and both are otherwise aligned instruction
for instruction, so it is the whole remaining gap in `Set`.

Measured on the compiler rather than guessed: `int f = 1; if (s.a == 0) f = 0;`
gives `sltu`, and `int f = k; if (s.a == 0) f = 0;` with `k` a PARAMETER gives
exactly `movz f,zero,a; bne f,zero` — the target's shape. So the target's
initial value reaches the flag through a register whose contents gcc does not
fold, even though it materialises it with a plain `li 1` (which the copy then
survives, because that register is used again by the `VIMAX(x, 2)` comparison
and so does not die at the copy). Sharing the constant with the VIMAX bound is
not enough on its own: our source shares it too and still folds. What the
source does to keep the value opaque at the point of the `if` is the open
question.
