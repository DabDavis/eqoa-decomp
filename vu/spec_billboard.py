#!/usr/bin/env python3
"""The billboard microprogram (billboard.vsm, VIBbRender), as a function: the
reading of the microcode, as code that test_spec_billboard.py holds to the
simulator.

The EE side is VIRaster::BeginBillboards / EndBillboards (decomp/src/
viraster.cpp).  The constant block, when the matrices are dirty:
   0- 3  model-view (SPR+0x00)      -> vf01..vf04
   4- 7  projection x clip->screen  -> vf05..vf08   ("PS" below)
   8-11  model->clip (SPR+0x80)     -> vf09..vf12
   12    (1, 1, 1, 1)               -> vf13.w only (lq.w): the w every
                                       transform below multiplies row 3 by
   13    (offset x, offset y, 128, 128 or 64 when the layer blends)  -> vf14
   16    the GIF tag template: REGS RGBAQ ST XYZ2 XYZ2 ST XYZ2 ST XYZ2 ST XYZ2
         XYZ2 (11 a billboard)      -> vf15, stored at each packet's head
   17-18 a one-register A+D packet, kicked once a batch;  19  (8192,8192,8192)
   32-59 the corner (s, t, 1, 0) of seven sprite-sheet layouts, four each
A batch is UNPACKed with FLG (double-buffered, found with XTOP): a count,
then 48-byte VIBillboards -- (location, frame), (width, height, -, -), colour.
EndBillboards ORs 0x100 into the LAST one's frame: the end mark.

Per billboard (any number at once in the microcode's software pipeline; each
here start to finish):
  colour  ftoi0(rgb * 128, a * vf14.w)
  view    v = MV (loc, 1);  clip c = MC (loc, 1);  code = CLIPw(c)
  size    s = PS (width, height, v.z, 1) on x, y, w only;  half = s.xy / s.w
          - (offset x, offset y) -- the projected corner minus the screen
          centre; (0, 0) when ANY clip bit is set (fcand 0x3F): an off-screen
          billboard is a zero-sized quad, not a skipped one
  centre  p = PS v; p / p.w (all four lanes)
  corners D = p - h, A = (p.x - h.x, p.y + h.y), B = (p.x + h.x, p.y - h.y),
          C = p + h  (h.zw = 0), ftoi4 each
  out     RGBAQ, uv0, D, D, uv1, A, uv2, B, uv3, C, C  -- a strip whose doubled
          ends make the joins between quads degenerate; uvK = qword
          32 + (frame & 255) + K
Packets alternate between VU1 48 and 240.  One is kicked when it holds 16
billboards, or when a billboard's frame has bits above 255 (the end mark);
its tag's NLOOP is the count, with EOP.  The program ends ([e]) after the
kick that leaves no billboards.
"""
import vusim as S
import spec_strip as SP

ONE = 0x3F800000
f_add, f_sub, f_mul, ftoi = S.f_add, S.f_sub, S.f_mul, S.ftoi


def div(a, b):
    return S.clamp1(S.f_div(a, b))


def xform(rows, x, y, z, w, lanes=range(4)):
    out = [None] * 4
    for c in lanes:
        acc = f_mul(rows[0][c], x)
        acc = f_add(acc, f_mul(rows[1][c], y))
        acc = f_add(acc, f_mul(rows[2][c], z))
        out[c] = f_add(acc, f_mul(rows[3][c], w))
    return out


def sx16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


def billboard(vf, bb):
    """bb: three qwords (lists of words).  -> (colour, corners D A B C, frame)."""
    MV, PS, MC = vf[1:5], vf[5:9], vf[9:13]
    w13, k = vf[13][3], vf[14]
    loc, frame = bb[0][:3], bb[0][3] & 0xFFFF
    wid, hgt = bb[1][0], bb[1][1]
    C = bb[2]
    col = [ftoi(f_mul(C[c], k[2]), 0) for c in range(3)] + [ftoi(f_mul(C[3], k[3]), 0)]
    v = xform(MV, loc[0], loc[1], loc[2], w13)
    code = SP.clip_code(xform(MC, loc[0], loc[1], loc[2], w13))
    s = xform(PS, wid, hgt, v[2], w13, lanes=(0, 1, 3))
    qs = div(ONE, s[3])
    if code:
        h = [0, 0]                                       # sub.xy vf16, vf00, vf00
    else:
        h = [f_sub(f_mul(s[c], qs), k[c]) for c in range(2)]
    h = h + [0, 0]                                       # sub.zw vf16, vf00, vf00
    p = xform(PS, v[0], v[1], v[2], v[3])
    qp = div(ONE, p[3])
    p = [f_mul(p[c], qp) for c in range(4)]
    A = [f_sub(p[0], h[0]), f_add(p[1], h[1]), f_sub(p[2], h[2]), f_sub(p[3], h[3])]
    B = [f_add(p[0], h[0]), f_sub(p[1], h[1]), f_add(p[2], h[2]), f_add(p[3], h[3])]
    Cc = [f_add(p[c], h[c]) for c in range(4)]
    D = [f_sub(p[c], h[c]) for c in range(4)]
    return col, [[ftoi(x, 4) for x in q] for q in (D, A, B, Cc)], frame


def render(vf, mem, bbs):
    """vf: registers at VIBbRender (VIBbInit's).  mem: VU1 data memory as
    words, updated IN PLACE as the microcode writes it.  bbs: the batch, as
    lists of three qwords.  -> the packets in kick order, each a list of
    qwords: the tag, then 11 per billboard.

    THE UV TABLE OVERLAPS THE FIRST PACKET BUFFER: the table is qwords 32-59
    and a packet at 48 runs from 48 up, so layouts 4-6 (frame 16, 20, 24)
    read what the program has written there -- the tag and the first
    billboard's record of the packet being built, or the whole previous one
    when building at 240.  So the reads and writes go in the microcode's
    order (billboard j's record starts at s = base + 1 + 11j):
      j = 0, 1  read uv0-2, write them (s+1, s+4, s+6), read uv3, write
                uv3 and the colour (j = 0: uv3 first; j = 1: colour first)
      j >= 2    read uv0-3, write j-2's corners but B, j's four UVs, j-2's
                B, j's colour
      flush     the corners still unwritten, then the tag's NLOOP."""
    def rd(a):
        return list(mem[a * 4: a * 4 + 4])

    def wr(a, q):
        mem[a * 4: a * 4 + 4] = list(q)

    def corners(j):
        D, A, B, C = recs[j][1]
        s = base + 1 + 11 * j
        for off, q in ((2, D), (3, D), (5, A), (9, C), (10, C), (7, B)):
            wr(s + off, q)
    packets = []
    base, recs, written = 48, [], 0
    wr(base, rd(16))
    for k, bb in enumerate(bbs):
        col, corn, frame = billboard(vf, bb)
        j = len(recs)
        recs.append((col, corn))
        s = base + 1 + 11 * j
        t = 32 + (frame & 255)
        if j < 2:
            uv = [rd(t), rd(t + 1), rd(t + 2)]
            for off, q in zip((1, 4, 6), uv):
                wr(s + off, q)
            uv.append(rd(t + 3))
            for off, q in (((8, uv[3]), (0, col)) if j == 0 else ((0, col), (8, uv[3]))):
                wr(s + off, q)
        else:
            uv = [rd(t + k) for k in range(4)]
            D, A, B, C = recs[j - 2][1]
            s2 = s - 22
            for off, q in ((2, D), (3, D), (5, A), (9, C), (10, C)):
                wr(s2 + off, q)
            for off, q in zip((1, 4, 6, 8), uv):
                wr(s + off, q)
            wr(s2 + 7, B)
            wr(s, col)
            written = j - 1
        if sx16(len(recs) - 16 + sx16(frame - (frame & 255))) >= 0:
            for jj in range(written if len(recs) > 2 else 0, len(recs)):
                corners(jj)
            mem[base * 4] = 0x8000 | len(recs)          # iswr.x vi06|0x8000
            packets.append([rd(base + k) for k in range(1 + 11 * len(recs))])
            base, recs, written = (240 if base == 48 else 48), [], 0
            if k < len(bbs) - 1:                         # L0179: more to come
                wr(base, rd(16))
    # nothing in the microcode stops at the count: without the end mark it
    # runs on into whatever follows the batch
    assert not recs, 'batch ends without the end mark'
    return packets
