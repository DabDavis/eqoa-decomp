#!/usr/bin/env python3
"""The strip microprograms written as one per-vertex function: the reading
of the microcode, as code that can be checked.  Three variants share one
pipeline and differ in how a vertex is decoded and lit:

  'strip'    VIRStrip   317  VUNC: float P, (s,t,q), N, colour
  'strip2'   VIRStrip2  1354 PackVUNC with vf11.x == 0 (no point light):
             ints * the pack scales in vf29
  'strip3'   VIRStrip3  549  PackVUNCG: ints * scales, the position offset
             by the pre-translation table at VU1 932 that pos.w indexes
  'fan'      VIRFan     26   VUNC like 'strip', as a triangle FAN: vertex 0
             stays put (vf18/vf15/vi04 are never shifted), so triangle n is
             (0, n-1, n), the winding parity vi09 stays 32 instead of
             alternating, and vertex 0 is emitted by the prologue with no
             cull test (path 'first').  The clip path BALs L0187, which
             assembles the fan triangle for the clipper, instead of L1085.

  'strip2lit' VIRStrip2 435  PackVUNC with vf11.x == 1 (the point light's
             sphere meets the buffer's box): 'strip2' plus the point term,
             added LAST -- ((c + amb) - sun) + static.rgb + point -- and the
             sun factor scaled by c.w BEFORE static.w (the unlit path does
             static.w first).  Both orders are read off the code (479, 485,
             502); no capture shows them: vf31 is (0, 0, 0, 1) in every lit run
  'strip5'   VIRStrip5  807  the tessellator's output (DrawPrimBufferTess-
             PackVUNCG): FLOAT vertices like 'strip', lit like 'strip3'
             (sun x c.w, point light, alpha 1); not software-pipelined, which
             changes no result
  'strip6'   VIRStrip6  924  PackVU: TWO qwords a vertex, position and
             (s, t, q), ints * vf29.x / vf29.y.  No normal and no vertex
             colour, so nothing is lit by direction: colour =
             min(amb + sunC * k + ptC * (1 - min(|L| / range, 1)) * k, 1),
             alpha = vf11.w, with k = vf11.y (qword 14: SPR+0x22C and
             SPR+0x228).  ELENG, not ERLENG: the distance itself

The microprogram is software-pipelined -- one vertex's loads and transform
overlap the previous one's lighting and output -- so its instructions read in
an order no single vertex follows.  Here each vertex is computed start to
finish, with every float op in the order the microcode issues it (rounding is
per op, so order is part of the result).  test_spec_strip.py holds it to the
simulator, which is held to PCSX2.

Inputs (VU1 data memory at entry, from VIRInit's registers):
  M   = vf01..vf04  model->screen rows      Mc = vf05..vf08 model->clip rows
  S   = vf09 sun direction, stored (x, -z, y, 0) so that the three-lane
        cross-product idiom below computes a plain dot product
  Lp  = vf10 point light position, w = 1/range
  sunC vf12, ptC vf13, amb vf14 (ambient + the material's sun tint)
  fog = vf30 (scale, offset, near clamp, far clamp)
Each input vertex is four qwords at TOP+1: position, (s, t, q), normal,
colour.  The vertex count is the first word at TOP.

Each output vertex is three qwords at VU1 142:
  XYZF2  ftoi4 of (x/w, y/w, z/w, fog) -- bit 15 of w is ADC (do not draw)
  STQ    (s, t, q * 1/w, fog)    s and t are NOT divided here; the layer
         routines behind L1037 apply each layer's UV transform and the 1/w
  RGBA   floats, clamped to 1:  colour + ambient + sun * max(-N.S, 0)
         + point * (1 - min(|L|/range, 1)) * max(L.N / |L|, 0), alpha = colour.w
"""
import vusim as S

ONE = 0x3F800000
f_add, f_sub, f_mul, f_div, f_sqrt, f_max, ftoi = (S.f_add, S.f_sub, S.f_mul, S.f_div,
                                                    S.f_sqrt, S.f_max, S.ftoi)
c1 = S.clamp1                    # identity except in pcsx2 mode, see vusim


def mini(a, b):
    return f_max(a, b, mini=True)


def transform(rows, p):
    """mulax / madday / maddaz / maddw against vf00.w: the product goes into
    ACC, each step chopped."""
    out = []
    for c in range(4):
        acc = f_mul(c1(rows[0][c]), p[0])
        acc = f_add(acc, f_mul(c1(rows[1][c]), p[1]))
        acc = f_add(acc, f_mul(c1(rows[2][c]), p[2]))
        out.append(f_add(acc, f_mul(c1(rows[3][c]), ONE)))
    return out


def clip_code(v):
    """CLIPw.xyz: +x>|w|, -x<-|w|, ... six bits, x lowest."""
    w = abs(S.u2f(v[3] & 0x7FFFFFFF))
    bits = 0
    for k in range(3):
        x = S.u2f(v[k])
        if x > w:
            bits |= 1 << (2 * k)
        if x < -w:
            bits |= 1 << (2 * k + 1)
    return bits


VARIANTS = ('strip', 'strip2', 'strip3', 'fan', 'strip2lit', 'strip5', 'strip6')
STRIDE = {'strip6': 2}            # input qwords per vertex; 4 otherwise
PRETRANS = 932                   # VU1 address of UploadPreTrans's 32 entries


def itof(u):
    return S.itof(u, 0)


def decode(variant, raw, u, mem):
    """One input vertex, four qwords, as the variant's prologue/loop tail
    turns it into floats -> (P, T, N, C)."""
    if variant == 'strip6':
        sc = u['vf29']
        p = [f_mul(c1(itof(raw[0][k])), sc[0]) for k in range(3)]
        t = [f_mul(c1(itof(raw[1][k])), sc[1]) for k in range(3)]
        return p, t, None, None
    P, T, N, C = raw
    if variant in ('strip', 'fan', 'strip5'):
        return P, T, N, C
    sc = u['vf29']
    if variant in ('strip2', 'strip2lit'):
        p = [f_mul(c1(itof(P[k])), sc[0]) for k in range(3)]     # mulx.xyz vf21, vf21, vf29x
    else:
        idx = (S.sx16(P[3]) + PRETRANS) & (S.MEMQ - 1)        # ilw.w vi08; lq.xyz 932(vi08)
        pt = mem[idx * 4: idx * 4 + 3]
        # mulax.xyz acc, vf21, vf29x; maddw.xyz vf21, vf28, vf00w
        p = [f_add(f_mul(c1(itof(P[k])), sc[0]), f_mul(c1(pt[k]), ONE)) for k in range(3)]
    t = [f_mul(c1(itof(T[k])), sc[1]) for k in range(3)]         # muly.xyz
    n = [f_mul(c1(itof(N[k])), sc[2]) for k in range(3)]         # mulz.xyz
    c = [f_mul(c1(itof(C[k])), c1(sc[3])) for k in range(4)]     # mulw.xyzw (both clamp)
    return p, t, n, c


def colour(variant, u, P, N, C, L, sq, invlen):
    """The lit colour of one vertex (every variant but strip6), RGBA words."""
    Sd, Lp = u['vf09'], u['vf10']
    sunC, ptC, amb, static = u['vf12'], u['vf13'], u['vf14'], u['vf31']
    # N.S through opmula / madda.x / opmsub: the x lane only
    acc = f_add(f_mul(N[1], Sd[2]), f_mul(N[0], Sd[0]))
    ndots = f_sub(acc, f_mul(Sd[1], N[2]))
    # the sun term: VIRStrip uses N.S as it is; VIRStrip3 scales it by the
    # colour's w; VIRStrip2 by the static colour's w and the colour's w, in
    # an order that differs between its two paths
    if variant in ('strip', 'fan'):
        sunf = ndots
    elif variant in ('strip3', 'strip5'):
        sunf = f_mul(c1(ndots), C[3])                        # mulw.x vf25, vf25, vf24w
    elif variant == 'strip2lit':
        sunf = f_mul(c1(f_mul(c1(ndots), C[3])), static[3])  # 479 by vf24w, 485 by vf31w
    else:
        sunf = f_mul(c1(f_mul(c1(ndots), static[3])), C[3])  # mulw.x by vf31w, then vf24w
    sun = mini(sunf, 0)
    rgb = [f_add(C[k], amb[k]) for k in range(3)]
    rgb = [f_sub(rgb[k], f_mul(sunC[k], sun)) for k in range(3)]
    if variant in ('strip2', 'strip2lit'):
        rgb = [f_add(rgb[k], f_mul(c1(static[k]), ONE)) for k in range(3)]  # maddaw vf31
    if L is not None:
        lensq = f_add(f_add(f_mul(ONE, sq[2]), f_mul(ONE, sq[1])), f_mul(ONE, sq[0]))
        ln = [f_mul(c1(L[k]), N[k]) for k in range(3)]
        ldotn = f_add(f_add(f_mul(ONE, ln[2]), f_mul(ONE, ln[1])), f_mul(ONE, ln[0]))
        dr = f_mul(c1(lensq), Lp[3])
        d = mini(f_mul(c1(dr), invlen), ONE)
        cosv = f_max(f_mul(c1(ldotn), invlen), 0)
        pw = f_mul(c1(f_sub(ONE, d)), cosv)
        rgb = [f_add(rgb[k], f_mul(c1(ptC[k]), pw)) for k in range(3)]
    rgb = [mini(x, ONE) for x in rgb]
    return rgb + [C[3] if variant in ('strip', 'fan') else ONE]


def colour6(u, Lp, dist):
    """VIRStrip6: no normal, no vertex colour.  k = vf11.y scales both the
    sun colour and the point light's distance falloff."""
    sunC, ptC, amb, k6 = u['vf12'], u['vf13'], u['vf14'], u['vf11'][1]
    d = mini(f_mul(c1(Lp[3]), dist), ONE)                    # mulw.w vf25, vf10, vf27w
    pw = f_mul(c1(f_sub(ONE, d)), k6)                        # subw.w; muly.w by vf11y
    rgb = [f_add(0, amb[k]) for k in range(3)]               # adda.xyz acc, vf00, vf14
    rgb = [f_add(rgb[k], f_mul(c1(sunC[k]), k6)) for k in range(3)]
    rgb = [mini(f_add(rgb[k], f_mul(c1(ptC[k]), pw)), ONE) for k in range(3)]
    return rgb + [u['vf11'][3]]                              # move.w vf23, vf11


def strip(u, verts, prev, variant='strip', mem=None):
    """u: dict of uniform registers (lists of 4 u32).  verts: the RAW input
    vertices, four qwords each.  prev: the state the strip inherits for its
    first two vertices -- vf19/vf20 (the two previous screen positions) and
    vi05/vi06 (their clip codes), whatever the last launch left.  mem: VU1
    data memory as words (strip3 reads its pre-translations from it).
    -> list of (xyzf, stq, rgba, adc, path), and whether anything was drawn."""
    assert variant in VARIANTS
    M = [u['vf01'], u['vf02'], u['vf03'], u['vf04']]
    Mc = [u['vf05'], u['vf06'], u['vf07'], u['vf08']]
    Lp, fog = u['vf10'], u['vf30']
    s_prev2, s_prev1 = prev['vf19'], prev['vf20']   # vf18 <- vf19 <- vf20 at the loop top
    code2, code1 = prev['vi05'], prev['vi06']
    point = variant != 'strip2'
    fan = variant == 'fan'
    parity = 0
    drawn = False
    out = []
    for n, raw in enumerate(verts):
        P, T, N, C = decode(variant, raw, u, mem)
        h = transform(M, P)
        if point:
            L = [f_sub(Lp[k], P[k]) for k in range(3)]
            # ERLENG: P = 1 / sqrt((Ly^2 + Lx^2) + Lz^2); ELENG without the 1/
            sq = [f_mul(c1(L[k]), L[k]) for k in range(3)]
            dist = f_sqrt(f_add(f_add(sq[1], sq[0]), sq[2]))
            invlen = f_div(ONE, dist)
        q = c1(f_div(ONE, h[3]))
        fg = f_add(f_mul(c1(h[3]), fog[0]), f_mul(ONE, fog[1]))
        qz = f_mul(c1(T[2]), q)
        fg = f_max(fg, fog[2])
        scr = [f_mul(c1(h[k]), q) for k in range(3)]
        cl = transform(Mc, P)
        fogc = mini(fg, fog[3])
        # edges in screen space: e1 = s(n-1) - s(n-2), e2 = s(n) - s(n-1)
        e2 = [f_sub(scr[k], s_prev1[k]) for k in range(3)]
        e1 = [f_sub(s_prev1[k], s_prev2[k]) for k in range(3)]
        code = clip_code(cl)
        xyzf = [ftoi(scr[0], 4), ftoi(scr[1], 4), ftoi(scr[2], 4), ftoi(fogc, 4)]
        cross_z = f_sub(f_mul(e1[0], e2[1]), f_mul(e2[0], e1[1]))
        stq = [T[0], T[1], qz, fogc]
        if variant == 'strip6':
            rgba = colour6(u, Lp, dist)
        else:
            rgba = colour(variant, u, P, N, C, L if point else None,
                          sq if point else None, invlen if point else None)
        if fan and n == 0:
            # the prologue: no cull test; vertex 0 is the fan's hub for good
            out.append((xyzf, stq, rgba, False, 'first'))
            s_prev2, code2 = scr + [fogc], code
            continue
        # culling: the triangle (n-2, n-1, n), or (0, n-1, n) for a fan
        parity = 32 if fan else 32 - parity
        both = code2 & code1 & code
        either = code2 | code1 | code
        back = 32 if cross_z >> 31 else 0
        if both:
            path, adc = 'reject', True       # all three outside one plane
        elif either:
            path, adc = 'clip', True         # handed to the clipper (n >= 2)
        elif back == parity:
            path, adc = 'backface', True
        else:
            path, adc = 'draw', False
            drawn = True
        if adc:
            xyzf[3] = (xyzf[3] & 0xFFFF) | 0x8000
        out.append((xyzf, stq, rgba, adc, path))
        if fan:
            s_prev1, code1 = scr + [fogc], code
        else:
            s_prev2, s_prev1 = s_prev1, scr + [fogc]
            code2, code1 = code1, code
    return out, drawn
