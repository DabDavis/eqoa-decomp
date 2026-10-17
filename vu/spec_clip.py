#!/usr/bin/env python3
"""The clipper (raster.vsm L0187, L1085, L1266, L1684..1967), as functions:
the reading of the microcode, as code that can be checked against the
simulator (test_spec_clip.py).

A strip or fan vertex whose triangle straddles a frustum plane takes the clip
path: the vertex gets ADC, and the triangle is handed here, clipped against
the planes it crosses, and drawn on its own as a FAN through the layer
emitter (L1037 with vi03 = 1, the fan template).

polygon()   L1085 (strips) / L0187 (fans): the triangle as a CLOSED polygon
            at VU1 214, five qwords a vertex --
              +0  header: x = clip code, y = 0 (an original vertex)
              +1  clip-space position          +2  (s, t, q, fog)
              +3  RGBA floats                  +4  screen (x/w, y/w, z/w, fog)
            strips: (n-2, n-1, n, n-2), or (n, n-1, n-2, n) when the winding
            parity vi09 != vi10, so every polygon goes round the same way;
            fans: (hub, n-1, n, hub).

clip()      L1266: Sutherland-Hodgman against the planes in the order
            z+ z- x+ x- y+ y- (clip bits 16 32 1 2 4 8), skipping a plane no
            code in the running union vi07 crosses; a vertex the clipper
            makes ORs its own code into that union.  The buffers ping-pong
            between 214 and 264 (L1684).  An empty pass returns nothing.
  L1691     one plane: walking edges (prev, cur), an inside prev is copied
            (header and all four qwords); a crossing edge adds the
            intersection; the output is closed by a copy of its first vertex.
  1761/1784/1807  the intersection on axis x/y/z, with s = -1 for the +
            plane and +1 for the - plane: d = c + s*w for both ends, then it
            always interpolates FROM the end chosen by the sign of
            (prev.c - cur.c) -- read through the MAC flag of the `sub` four
            cycles before the fmand -- t = d_from / (d_from - d_to) by DIV,
            v = (to - from) * t + from on the clip position, STQ and RGBA.
            The screen qword is NOT written (left stale), the header is the
            new clip code with y = 1 (a made vertex).  The `sub` also leaves
            prev.c - cur.c in that lane of vf27, which L1876 reads.
  L1876     to the 142 layout (xyzf, stq, rgba) at the other buffer: an
            original vertex keeps its screen qword; a made one is projected
            through qwords 8-11 (clip -> screen), with STQ.z = 1/w (mr32 puts
            vf00.w = 1 in z first: the interpolated q is DROPPED) and fog =
            its interpolated STQ.w.  From the second vertex on, the fan
            triangle (first, previous, this) is culled on the screen-space
            cross product's sign (ADC) -- for the second vertex "previous"
            is whatever vf27 holds, which no GS fan draws with.
"""
import spec_strip as SP
import vusim as S

ONE = 0x3F800000
f_add, f_sub, f_mul, ftoi = S.f_add, S.f_sub, S.f_mul, S.ftoi

# (clip bit, axis lane, vf23.x) in L1266's order
PLANES = ((16, 2, 0xBF800000), (32, 2, ONE), (1, 0, 0xBF800000),
          (2, 0, ONE), (4, 1, 0xBF800000), (8, 1, ONE))


def div(a, b):
    return S.clamp1(S.f_div(a, b))


class V:
    """One polygon vertex: code, flag, and the four qwords (lists of words)."""
    def __init__(self, code, flag, clip, stq, rgba, scr):
        self.code, self.flag = code, flag
        self.clip, self.stq, self.rgba, self.scr = clip, stq, rgba, scr

    def words(self):
        return [self.code, self.flag], self.clip + self.stq + self.rgba + (self.scr or [])


def polygon(fan, vf, vi, mem):
    """The closed polygon L1085 / L0187 builds, from the registers at its
    entry (vf15-20 clip and screen of the three vertices, vi04-06 their
    codes, vi03 the 142 buffer's write pointer, vi09/vi10 the parity)."""
    q = lambda a: mem[a * 4: a * 4 + 4]
    p = vi[3]
    if fan:
        slots = [(142, 4, 15, 18), (p - 6, 5, 16, 19), (p - 3, 6, 17, 20)]
        order = (0, 1, 2, 0)
    else:
        slots = [(p - 9, 4, 15, 18), (p - 6, 5, 16, 19), (p - 3, 6, 17, 20)]
        order = (0, 1, 2, 0) if vi[9] == vi[10] else (2, 1, 0, 2)
    out = []
    for k in order:
        b, code, c, s = slots[k]
        out.append(V(vi[code], 0, list(vf[c]), q(b + 1), q(b + 2), list(vf[s])))
    return out


def intersect(prev, cur, axis, s, vf27):
    pw = f_mul(prev.clip[3], s)                         # mulx.w vf24, vf15, vf23x
    cw = f_mul(cur.clip[3], s)                          # mulx.w vf25, vf19, vf23x
    vf27[axis] = f_sub(prev.clip[axis], cur.clip[axis])  # sub.c vf27: the MAC sign read
    d0 = f_add(prev.clip[axis], pw)
    d1 = f_add(cur.clip[axis], cw)
    if vf27[axis] >> 31:
        frm, to, t = cur, prev, div(d1, f_sub(d1, d0))   # L1777..: from cur
    else:
        frm, to, t = prev, cur, div(d0, f_sub(d0, d1))   # 1770..: from prev
    lerp = lambda a, b: [f_add(f_mul(f_sub(b[k], a[k]), t), a[k]) for k in range(4)]
    clip = lerp(frm.clip, to.clip)
    return V(SP.clip_code(clip), 1, clip, lerp(frm.stq, to.stq), lerp(frm.rgba, to.rgba), None)


def one_plane(poly, bit, axis, s, union, vf27):
    out = []
    cur = poly[0]
    for nxt in poly[1:]:
        prev, cur = cur, nxt
        if not prev.code & bit:
            out.append(prev)
            if cur.code & bit:
                out.append(intersect(prev, cur, axis, s, vf27))
                union[0] |= out[-1].code
        elif not cur.code & bit:
            out.append(intersect(prev, cur, axis, s, vf27))
            union[0] |= out[-1].code
    if out:
        out.append(out[0])
    return out


def transform4(rows, p):
    out = []
    for c in range(4):
        acc = f_mul(rows[0][c], p[0])
        acc = f_add(acc, f_mul(rows[1][c], p[1]))
        acc = f_add(acc, f_mul(rows[2][c], p[2]))
        out.append(f_add(acc, f_mul(rows[3][c], p[3])))
    return out


def clip(poly, union, vf27, mem):
    """poly: polygon() (closed).  union: vi07 at L1266.  vf27: the register
    at L1266 (only its stale lanes matter).  -> the vertex list at the output
    buffer as [(xyzf, stq, rgba)], or [] when nothing survives."""
    u = [union]
    vf27 = list(vf27)
    for bit, axis, s in PLANES:
        if u[0] & bit:
            poly = one_plane(poly, bit, axis, s, u, vf27)
            if not poly:
                return []
    rows = [mem[k * 4: k * 4 + 4] for k in range(8, 12)]
    out = []
    first = prev = None
    for n, v in enumerate(poly[:-1]):
        stq = list(v.stq)
        if v.flag:
            h = transform4(rows, v.clip)
            q = div(ONE, h[3])
            stq[2] = f_mul(ONE, q)                      # mr32.z vf16, vf00; mulq.z
            scr = [f_mul(h[k], q) for k in range(3)] + [v.stq[3]]
        else:
            scr = list(v.scr)
        xyzf = [ftoi(scr[k], 4) for k in range(4)]
        if n == 0:
            first, prev = scr, vf27
        else:
            e1 = [f_sub(prev[k], first[k]) for k in range(2)]
            e2 = [f_sub(scr[k], prev[k]) for k in range(2)]
            cross_z = f_sub(f_mul(e1[0], e2[1]), f_mul(e2[0], e1[1]))
            if cross_z >> 31:
                xyzf[3] = (xyzf[3] & 0xFFFF) | 0x8000
            prev = scr
        out.append((xyzf, stq, list(v.rgba)))
    return out
