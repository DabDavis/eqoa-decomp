# Phase 16 — `VISurface`, and the store order a constant decides

**2026-08-23. 514/588 → 525/600.** `VISurface` is **11 of 12** on the settled
toolchain with **no re-tuning**, six of them on the first build. It is
`VIMaterial`'s other half — the thing a material layer's `m_texture` names —
and the first class in the corpus that builds **GS display-list headers**: four
qwords of VIF and GIF tags written in front of every mip level and of the CLUT,
so an upload is a pointer and a quadword count.

## The layout

`LockMipLevel`'s `li a1,24` mult gives the stride, the constructor's own loop
bound (`li a2,9`, ten stores at +0x54 descending by 24) gives the count, and
`m_dataSize` at +0x134 = 0x44 + 10 × 24 closes it.

```
VISurface : public VIBuffer                VIMipLevel               (24 bytes)
+0x00  int   m_id        (base)            +0x00  int m_width
+0x04  int   m_userData  (base)            +0x04  int m_height
+0x08  int   m_width                       +0x08  int m_pixels   byte offset
+0x0C  int   m_height                      +0x0C  int m_stride
+0x10  VIPixelStore m_pixelStore   = 2     +0x10  int m_header   offset, then
+0x14  int   m_tbp       = -1                            a POINTER
+0x18  int   m_cbp       = -1              +0x14  int m_qwc
+0x1C  int   m_lockLevel = -1
+0x20  int   m_dirty     = 1
+0x24  int   m_pad24
+0x28  int   m_paletteCount     0 / 16 / 256
+0x2C  int   m_paletteOffset
+0x30  int   m_paletteSize
+0x34  VIPixelStore m_paletteStore
+0x38  int   m_paletteHeader    offset, then a POINTER
+0x3C  int   m_paletteQWC
+0x40  int   m_numMipLevels
+0x44  VIMipLevel m_levels[10]
+0x134 int   m_dataSize
+0x138 unsigned char *m_data
```

`m_pixels` and `m_header` are both byte offsets into the one allocation;
`m_header` (and `m_paletteHeader`) are rebased onto `m_data` **in place** by
`SetDMAHeaders`, `m_pixels` never is — `LockMipLevel` adds `m_data` every time.
`m_pixels` is always `m_header + 64`, which is the header's own size.

**The `VIPixelStore` enum is read off `CalcMipLevelStride`'s jump table**, whose
five case blocks give the bytes per row: 0 = I4 (`w/2` rounded up), 1 = I8
(`w`), 2 = RGBA32 (`w << 2`), 3 = RGB24 (`w * 3`), 5 = RGBA16 (`w << 1`). The
bound is `sltiu 6`, so **4 has no case at all** and its table entry is the
default's `return -1`. `LockPalette` parks `m_numMipLevels` in `m_lockLevel`,
so the CLUT is "level N" and `UnlockPalette` tests for exactly that.

## The header: VIF DIRECT, then three GIFtags

Both copies of the writer lay down the same 64 bytes:

```
qword 0   0, 0, 0, (qwc-1) | 0x50000000      three VIF NOPs then DIRECT
qword 1   0x1000000000000001                 GIFtag NLOOP=1 NREG=1 PACKED
          0x0E                               REGS = A+D
qword 2   0                                  the A+D datum ...
          0x53                               ... written to TRXDIR: host->local
qword 3   (qwc-4) | 0x8000 | 1<<59           GIFtag NLOOP=qwc-4 EOP FLG=IMAGE
          0
```

Which pins the other half of Phase 15's ABI finding: `long long` is **16**
bytes here, but **`long` is 8**, so a 64-bit `sd` constant is an
`unsigned long`.

## The lever: the LAST store of each constant is hoisted to the front

Phase 13 recorded that "the block's LAST store is hoisted to the FRONT". That
block had one constant in it. The rule is per **value**:

> gcc emits, at the head of the block, the last store of each distinct
> constant — in source order — and everything else after it, in source order.

`Deallocate` writes ten fields with four values (2, -1, 1, 0) and comes out
`16, 28, 32, 308, 8, 12, 20, 24, 64, 312`. Reading that as source order gives
nine wrong words. Reading it through the rule: the head is `{16, 28, 32, 308}`,
one store per value; the tail is the other six in source order; therefore
`m_lockLevel = -1` is written after both other `-1`s and `m_dataSize = 0` after
every other zero. The source is **field order with the last two swapped**, and
it is byte-exact:

```c
m_width = 0; m_height = 0; m_pixelStore = VIPixelStoreRGBA32;
m_tbp = -1; m_cbp = -1; m_lockLevel = -1; m_dirty = 1;
m_numMipLevels = 0; m_data = 0; m_dataSize = 0;
```

The constructor falls out of the same rule — plain field order, with
`m_paletteHeader` the one field written out of place, then the mip loop.

## `Init`'s `VIRaster &` is discarded, and the first call is `Deallocate()`

Phase 14 found that `VIPrimBuffer::Init` ignores the `VIRaster &` it takes.
`VISurface::Init(VIRaster &, int, int, VIBitDepth, int)` does the same, and
here it is **measurable**: written `if (Attach(r) < 0) return -1;` — any call
that keeps `r` live to it — the four parameters allocate the other way round
(`s1` gets the bit depth and `s2` the width, where the target has them
swapped) and six words go wrong. Written `if (Deallocate() < 0) return -1;`,
with `r` never read at all, the function is byte-exact. An unused reference
parameter costs nothing: it arrives in `a1` and dies there.

## The lever: a walking pointer needs its origin opaque

`SetDMAHeaders` writes its four qwords through one pointer bumped by 16:

```
sw zero,0(t1) … sw v0,12(t1) ; addiu t1,t1,16 ; sd a1,0(t1) ; sd a2,8(t1) ; …
```

Written the obvious way — rebase the member, then read it back —

```c
m_paletteHeader = (int)m_data + m_paletteHeader;
p = (unsigned int *)m_paletteHeader;
```

every `p += 4` is folded into the displacement and the three `addiu`s vanish
(`sd a0,16(v1)`, `24`, `32`, `40`, `48`, `56`). **Compute the pointer first and
store it back** and they survive:

```c
p = (unsigned int *)(m_data + m_paletteHeader);
m_paletteHeader = (int)p;
```

A parameter behaves the same way, which is the tell: gcc folds a `p + 16` whose
`p` it can see as `base + const`, and does not fold one it cannot.

The second half is scope. Declaring `p` **once at function scope**, shared by
the palette block and the mip loop, is what puts `this` in `t2` and `p` in `t1`
with the `move t1,v0` the target has; declared per-block it is one pseudo
fewer and everything shifts a register. That single move took the function from
57 wrong words to 12, and a four-way sweep of the two "high half / low half"
store pairs closed the rest.

## The lever: a switch orders comparisons by VALUE and bodies by SOURCE

`ReorderPalette` tests `m_paletteStore == 2` first and then falls into the
**16-bit** loop, with the 32-bit loop out of line — so the halfword case is
written FIRST in the source even though the decision tree tests 2 first. An
`if`/`else if` chain cannot produce that shape; a `switch` can, because gcc
builds the tree in value order and emits the case bodies in source order.

The three `return 0`s are one `break` to a `return 0` at the bottom of the
function. That matters: `jump_optimize` turns an **unconditional** branch to a
return block back into a return, so each loop ends `jr ra; move v0,zero`
inline, while the **conditional** ones — the `m_pixelStore != I8` test and the
two empty-palette `blez`s — stay branches into the block. Which is what leaves
`move v0,zero` sitting at a label for reorg to steal into the first `bne`'s
delay slot, with the branch redirected past it. Every other spelling puts the
exit block early and fills the slot from the fall-through.

## The one open function

`CalcSurfaceOffsets` is 87 of 143. Its shape is settled — the decision tree,
both case bodies, the shared `m_paletteQWC` tail, the four-term loop condition
`w >= 16 && h >= 16 && level < 10 && level < m_numMipLevels` (the cap is a term,
not a `break`, and the exit tests come out in exactly that order), the two mip
cursors, and the closing pair. What is left is the **standing phenomenon** of
Phases 4–6, in one function and in three forms:

* the target computes `align(m_paletteCount * bpp)` **twice**, reloading the
  count after the first align's if/else joins; ours computes it once and
  carries it across the join in a spare register (`move a2,v1`);
* the target keeps `size + 16` on `size`; ours folds it back onto the raw
  product as `+80`;
* the first mip cursor's base is +76 in the target and +84 in ours.

It is not a flag: `-fno-gcse`, `-fno-cse-follow-jumps`,
`-fno-rerun-cse-after-loop`, `-fno-expensive-optimizations`,
`-fno-thread-jumps`, `-fno-force-mem`, `-fno-caller-saves`, `-fno-regmove`,
`-fno-peephole` and `-fno-cse-skip-blocks` all leave it exactly where it is
(`-fno-strength-reduce` makes it much worse). One cause, three symptoms, and
the same one that owns the other 74 open functions.

## Not started

`VISurface` is complete. Its neighbours in the render group —
`VIEESurfCache` (19 functions), `VIClipStack` (15) and `VIRaster` itself (173)
— are untouched, as are `VIMaterial`'s five open accessors and its four
`Construct*Pass` DMA builders.
