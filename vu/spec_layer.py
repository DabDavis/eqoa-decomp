#!/usr/bin/env python3
"""The layer routines behind layer_emitter's jump table (raster.vsm 1451..1544), as
functions: each turns the strip programs' 142+ buffer into one GIF packet for
one material layer, at VU1 539, which layer_emitter then XGKICKs.

The layer list is VIMaterial::ConstructDMAPacket's (decomp/src/vimaterial.cpp),
unpacked to VU1 435: a count, then 17 qwords per layer --
  +0      .x = the pass type
  +1..+9  an A+D GS-state packet (TEST_1, ALPHA_1, CLAMP_1, four texture
          registers, ZBUF_1), kicked as it is
  +10     GIF tag template, triangle strip (prim 28 | FGE/ABE), REGS ST RGBAQ XYZF2
  +11     the same for a fan (prim 29)
  +12     a line strip (prim 10), REGS RGBAQ XYZF2 -- the wire pass's
  +13     the layer colour x 128 (the GS's 1.0), alpha halved when blending
  +14..16 the layer's UV transform; row 2 carries the UV scroll offset
Pass types (ConstructTextPass / ConstructSolidFillPass / ConstructWirePass):
  0  solid fill    layer_solid  RGBAQ, XYZF2
  1  textured, wrap on both axes          layer_tex_wrap  -- subtracts an integer UV
     offset taken from the first vertex, invisible under wrapping, which keeps
     the coordinates near [0, 1)
  2  wire          layer_wire  a LINE strip, white (255 in every lane of RGBAQ,
     the vertex colour is ignored): v1, then per triangle n >= 2 the four
     points v(n), v(n-2), v(n-1), v(n) -- NLOOP 4(N-2)+1, template +12.  A
     fan (vi03 != 0) keeps v(n-2) at the hub.  The first two have ADC
     cleared; when v(n) carries ADC, the two back-references go out with w =
     0x8000 exactly (ISW over the whole word), and v(n) is remembered with w =
     (w - 0x8000) + 16 for the triangles after it
  3  textured, clamp on S or T            layer_tex_clamp  -- the same with no offset

Per vertex out, textured: (S, T, Q) = (u'*q, v'*q, q) with (u', v') the layer
transform of (s, t); then ftoi0(colour * layer colour); then XYZF2 as is.
"""
import vusim as S

ONE = 0x3F800000
f_add, f_sub, f_mul, ftoi = S.f_add, S.f_sub, S.f_mul, S.ftoi
c1 = S.clamp1


def uv(rows, s, t, k=None):
    """mulax.xy / madday.xy [/ msubaw.xy k] / maddw.xy: ((r0 s + r1 t) - k) + r2."""
    out = []
    for c in range(2):
        acc = f_mul(c1(rows[0][c]), s)
        acc = f_add(acc, f_mul(c1(rows[1][c]), t))
        if k is not None:
            acc = f_sub(acc, f_mul(k[c], ONE))
        out.append(f_add(acc, f_mul(c1(rows[2][c]), ONE)))
    return out


def layer(ptype, entry, buf, n, vf22, tag_offset=0):
    """entry: the layer's 17 qwords (lists of 4 words).  buf: the 142+ buffer
    as n x (xyzf, stq, rgba).  vf22: the register's value at the routine's
    entry -- its w goes out unchanged in every STQ (S/T/Q fill only x..z).
    -> the packet at 539 as a list of qwords."""
    if ptype == 2:
        return wire(entry, buf, n, tag_offset)
    tag = list(entry[10 + tag_offset])
    tag[0] = 0x8000 | n                    # iswr.x: EOP | NLOOP, upper half cleared
    rows = entry[14:17]
    mul = entry[13]
    out = [tag]
    w = vf22[3]
    k = None
    if ptype == 1:
        # 1456..1471: k = itof0(ftoi0(uv'(v0) - 2047/4096)), every lane
        half = S.itof(2047, 12)
        u0 = uv(rows, buf[0][1][0], buf[0][1][1])
        lanes = [u0[0], u0[1], vf22[2], vf22[3]]
        lanes = [f_sub(c1(x), c1(half)) for x in lanes]          # sub.xyzw: both clamp
        k = [S.itof(ftoi(x, 0), 0) for x in lanes]
        w = lanes[3]
    for xyzf, stq, rgba in buf[:n]:
        col = [ftoi(f_mul(c1(rgba[c]), c1(mul[c])), 0) for c in range(4)]   # mul.xyzw: both clamp
        if ptype == 0:
            out += [col, list(xyzf)]
            continue
        u = uv(rows, stq[0], stq[1], k)
        q = stq[2]
        out += [[f_mul(c1(u[0]), q), f_mul(c1(u[1]), q), q, w], col, list(xyzf)]
    return out


def wire(entry, buf, n, fan):
    """layer_wire..1683 (strip) and layer_wire_fan.. (fan)."""
    tag = list(entry[12])
    tag[0] = 0x8000 | (((n - 2) * 4 + 1) & 0xFFFF)   # layer_wire_tag: ior vi08, ((N-2)*4)+1
    white = [255, 255, 255, 255]                     # mfir.xyzw vf23, vi07 = 255
    out = [tag]

    def first(xyzf):                                 # ilwr.w; iand 0x7FFF; mfir.w
        return list(xyzf[:3]) + [xyzf[3] & 0x7FFF]
    hub = prev2 = first(buf[0][0])
    prev1 = first(buf[1][0])
    out += [white, prev1]
    for xyzf, _, _ in buf[2:n]:
        v = list(xyzf)
        w = v[3] & 0xFFFF
        back2 = hub if fan else prev2
        if (w - 0x7FFF) & 0xFFFF and not (w - 0x7FFF) & 0x8000:   # iblez on 16 bits
            out += [white, v, white, back2[:3] + [0x8000], white, prev1[:3] + [0x8000], white, v]
            v = v[:3] + [S.sx16((w - 0x8000 + 16) & 0xFFFF) & 0xFFFFFFFF]
        else:
            out += [white, v, white, back2, white, prev1, white, v]
        prev2, prev1 = prev1, v
    return out
