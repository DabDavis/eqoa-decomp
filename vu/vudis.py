#!/usr/bin/env python3
"""VU1 microcode disassembler AND assembler, so one is the other's test.

The opcode tables are PCSX2's interpreter tables (`pcsx2/VUops.cpp`), not its
debugger's (`DebugTools/DisVUmicro.h`), which labels the four SUBA{bc} opcodes
as SUB{bc}.  The operand syntax is dvp-as style:

    mulx.xyzw vf10, vf01, vf02x     lq.xyz vf03, 4(vi02)

one line per instruction PAIR, upper first, the way the hardware fetches it.
A pair whose re-encoding would not reproduce its 64 bits exactly (an unused
field set, a bad opcode) is written `.pair 0xLOWER, 0xUPPER` with the attempted
reading as a comment, so `assemble(disassemble(x)) == x` holds for ANY input.

Library:  decode_upper(w) / decode_lower(w) / encode_pair(text)
CLI:      vudis.py extract [--force]  write src/*.vsm from the SUPPORT image
          vudis.py check          assemble src/*.vsm, compare with the image
          vudis.py dis <hex...>   disassemble raw pairs (lower upper ...)
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
IMAGE = os.path.join(ROOT, 'claude-eqoa-iso/decrypted_modules/SUPPORT_decompressed_SNR1.bin')
SYMBOLS = os.path.join(ROOT, 'claude-eqoa-iso/decrypted_modules/SUPPORT_symbol_map.txt')
IMAGE_BASE = 0x003E3D00

# The two microprograms: (source name, DMA symbol, prefix of its label symbols).
# The labels are BYTE offsets into each program's own VU code section (the
# symbol map shows them relocated by IMAGE_BASE); only differences are used
# by the EE side, so an entry point is (label - first label) / 8.
PROGRAMS = [
    ('billboard', 'VIDMABillboardMicrocode', 'VIBb'),
    ('raster', 'VIDMARasterMicrocode', 'VIR'),
]

DEST = 'xyzw'
BC = 'xyzw'


class VUError(Exception):
    pass


def _dest(d):
    """dest bits 24..21 are x,y,z,w (x is the HIGH bit)."""
    s = ''.join(c for i, c in enumerate(DEST) if d & (8 >> i))
    return '.' + s if s else ''


def _parse_dest(s):
    d = 0
    for c in s:
        i = DEST.index(c)
        if d & (8 >> i):
            raise VUError('repeated dest component: ' + s)
        d |= 8 >> i
    return d


def _sext(v, bits):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def vf(n):
    return 'vf%02d' % n


def vi(n):
    return 'vi%02d' % n


# ------------------------------------------------------------------ upper ---

# op6 0x00..0x2F.  kind: bc = fd, fs, ft{bc};  q/i = fd, fs, Q|I;  full = fd, fs, ft
UPPER_MAIN = {}
for base, op in (('add', 0x00), ('sub', 0x04), ('madd', 0x08), ('msub', 0x0C),
                 ('max', 0x10), ('mini', 0x14), ('mul', 0x18)):
    for k in range(4):
        UPPER_MAIN[op + k] = (base + BC[k], 'bc')
UPPER_MAIN.update({
    0x1C: ('mulq', 'q'), 0x1D: ('maxi', 'i'), 0x1E: ('muli', 'i'), 0x1F: ('minii', 'i'),
    0x20: ('addq', 'q'), 0x21: ('maddq', 'q'), 0x22: ('addi', 'i'), 0x23: ('maddi', 'i'),
    0x24: ('subq', 'q'), 0x25: ('msubq', 'q'), 0x26: ('subi', 'i'), 0x27: ('msubi', 'i'),
    0x28: ('add', 'full'), 0x29: ('madd', 'full'), 0x2A: ('mul', 'full'), 0x2B: ('max', 'full'),
    0x2C: ('sub', 'full'), 0x2D: ('msub', 'full'), 0x2E: ('opmsub', 'full'), 0x2F: ('mini', 'full'),
})

# op6 0x3C..0x3F: bits 1..0 pick the column, bits 10..6 (fd) the row.
# kinds: accbc = ACC, fs, ft{bc};  accq/acci = ACC, fs, Q|I;  accfull = ACC, fs, ft;
#        cvt = ft, fs;  clip = fs, ft{w};  nop
UPPER_T3 = {}
for row, base in ((0, 'adda'), (1, 'suba'), (2, 'madda'), (3, 'msuba'), (6, 'mula')):
    for k in range(4):
        UPPER_T3[(row, k)] = (base + BC[k], 'accbc')
for k, n in enumerate((0, 4, 12, 15)):
    UPPER_T3[(4, k)] = ('itof%d' % n, 'cvt')
    UPPER_T3[(5, k)] = ('ftoi%d' % n, 'cvt')
UPPER_T3.update({
    (7, 0): ('mulaq', 'accq'), (7, 1): ('abs', 'cvt'), (7, 2): ('mulai', 'acci'), (7, 3): ('clipw', 'clip'),
    (8, 0): ('addaq', 'accq'), (8, 1): ('maddaq', 'accq'), (8, 2): ('addai', 'acci'), (8, 3): ('maddai', 'acci'),
    (9, 0): ('subaq', 'accq'), (9, 1): ('msubaq', 'accq'), (9, 2): ('subai', 'acci'), (9, 3): ('msubai', 'acci'),
    (10, 0): ('adda', 'accfull'), (10, 1): ('madda', 'accfull'), (10, 2): ('mula', 'accfull'),
    (11, 0): ('suba', 'accfull'), (11, 1): ('msuba', 'accfull'), (11, 2): ('opmula', 'accfull'), (11, 3): ('nop', 'nop'),
})
# 'minii' is MINI with I; dvp-as spells it minii.

UPPER_ENC = {}   # name -> list of (kind, encoder fields)
for op, (n, k) in UPPER_MAIN.items():
    UPPER_ENC.setdefault(n, []).append((k, op))
for (row, col), (n, k) in UPPER_T3.items():
    UPPER_ENC.setdefault(n, []).append((k, 0x3C | col | (row << 6)))

FLAGS = (('i', 31), ('e', 30), ('m', 29), ('d', 28), ('t', 27))


def decode_upper(w):
    """-> (text, kind).  Flags other than I are rendered as a [edmt] suffix."""
    op = w & 0x3F
    dest = (w >> 21) & 15
    ft, fs, fd = (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31
    if op >= 0x3C:
        ent = UPPER_T3.get((fd, op & 3))
    else:
        ent = UPPER_MAIN.get(op)
    if ent is None:
        raise VUError('bad upper opcode %08x' % w)
    name, kind = ent
    fl = ''.join(c for c, b in FLAGS[1:] if w >> b & 1)
    fl = '[%s]' % fl if fl else ''
    if kind == 'nop':
        return 'nop' + fl
    bc = BC[op & 3]
    ops = {
        'bc': (vf(fd), vf(fs), vf(ft) + bc),
        'q': (vf(fd), vf(fs), 'q'),
        'i': (vf(fd), vf(fs), 'i'),
        'full': (vf(fd), vf(fs), vf(ft)),
        'accbc': ('acc', vf(fs), vf(ft) + bc),
        'accq': ('acc', vf(fs), 'q'),
        'acci': ('acc', vf(fs), 'i'),
        'accfull': ('acc', vf(fs), vf(ft)),
        'cvt': (vf(ft), vf(fs)),
        'clip': (vf(fs), vf(ft) + 'w'),
    }[kind]
    return '%s%s%s %s' % (name, _dest(dest), fl, ', '.join(ops))


_VF = re.compile(r'^vf(\d+)([xyzw]?)$')
_VI = re.compile(r'^vi(\d+)$')


def _pvf(s, comp=None):
    m = _VF.match(s)
    if not m or int(m.group(1)) > 31:
        raise VUError('expected vf register: ' + s)
    if comp is not None:
        if m.group(2) == '':
            raise VUError('expected a component on ' + s)
        return int(m.group(1)), BC.index(m.group(2))
    if m.group(2):
        raise VUError('unexpected component on ' + s)
    return int(m.group(1))


def _pvi(s):
    m = _VI.match(s)
    if not m or int(m.group(1)) > 15:
        raise VUError('expected vi register: ' + s)
    return int(m.group(1))


def _split_mnem(m):
    """'mulx.xyzw[e]' -> ('mulx', 'xyzw', 'e')"""
    mm = re.match(r'^([a-z0-9]+)(?:\.([xyzw]*))?(?:\[([a-z]*)\])?$', m)
    if not mm:
        raise VUError('bad mnemonic: ' + m)
    return mm.group(1), mm.group(2) or '', mm.group(3) or ''


def encode_upper(mnem, ops):
    name, dest, fl = _split_mnem(mnem)
    if name not in UPPER_ENC:
        raise VUError('unknown upper op: ' + name)
    w = _parse_dest(dest) << 21
    for c in fl:
        b = dict(FLAGS).get(c)
        if b is None or c == 'i':
            raise VUError('bad flag %r' % c)
        w |= 1 << b
    errs = []
    for kind, enc in UPPER_ENC[name]:
        try:
            return w | enc | _upper_operands(kind, enc, ops)
        except VUError as e:
            errs.append(str(e))
    raise VUError('%s: %s' % (mnem, '; '.join(errs)))


def _upper_operands(kind, enc, ops):
    need = {'nop': 0, 'cvt': 2, 'clip': 2}.get(kind, 3)
    if len(ops) != need:
        raise VUError('%s takes %d operands' % (kind, need))
    if kind == 'nop':
        return 0
    if kind in ('bc', 'q', 'i', 'full'):
        fd, fs = _pvf(ops[0]), _pvf(ops[1])
        w = fd << 6 | fs << 11
    elif kind.startswith('acc'):
        if ops[0] != 'acc':
            raise VUError('expected acc')
        w = _pvf(ops[1]) << 11
    if kind in ('bc', 'accbc'):
        ft, c = _pvf(ops[2], comp=True)
        if c != (enc & 3):
            raise VUError('broadcast component does not match the mnemonic')
        return w | ft << 16
    if kind in ('q', 'accq', 'i', 'acci'):
        if ops[2] != kind[-1]:
            raise VUError('expected ' + kind[-1])
        return w
    if kind in ('full', 'accfull'):
        return w | _pvf(ops[2]) << 16
    if kind == 'cvt':
        return _pvf(ops[0]) << 16 | _pvf(ops[1]) << 11
    if kind == 'clip':
        ft, c = _pvf(ops[1], comp=True)
        if c != 3:
            raise VUError('clip compares against w')
        return _pvf(ops[0]) << 11 | ft << 16
    raise AssertionError(kind)


# ------------------------------------------------------------------ lower ---

# Formats.  t/s/d are bits 20..16 / 15..11 / 10..6.
#   lq     vf_t, imm11(vi_s)        sq    vf_s, imm11(vi_t)
#   ilw    vi_t, imm11(vi_s)        (isw the same)
#   iu15   vi_t, vi_s, imm15        fc24  [vi01,] imm24       fcget vi_t
#   fs12   [vi_t,] imm12            fm    vi_t, vi_s
#   b      label                    bal   vi_t, label         jr vi_s   jalr vi_t, vi_s
#   ib2    vi_t, vi_s, label        ib1   vi_s, label
#   i3     vi_d, vi_s, vi_t         iaddi vi_t, vi_s, imm5
#   mv     vf_t, vf_s               lqi vf_t, (vi_s++)   sqi vf_s, (vi_t++)
#   lqd    vf_t, (--vi_s)           sqd vf_s, (--vi_t)
#   div    q, vf_s{fsf}, vf_t{ftf}  sqrt q, vf_t{ftf}
#   mtir   vi_t, vf_s{fsf}          mfir vf_t, vi_s
#   ilwr   vi_t, (vi_s)             r1 r, vf_s{fsf}    r2 vf_t, r
#   mfp    vf_t, p                  xt vi_t   xgkick vi_s   wait
#   ep     p, vf_s                  epf p, vf_s{fsf}
LOWER_MAIN = {
    0x00: ('lq', 'lq'), 0x01: ('sq', 'sq'), 0x04: ('ilw', 'ilw'), 0x05: ('isw', 'ilw'),
    0x08: ('iaddiu', 'iu15'), 0x09: ('isubiu', 'iu15'),
    0x10: ('fceq', 'fc24r'), 0x11: ('fcset', 'fc24'), 0x12: ('fcand', 'fc24r'), 0x13: ('fcor', 'fc24r'),
    0x14: ('fseq', 'fs12r'), 0x15: ('fsset', 'fs12'), 0x16: ('fsand', 'fs12r'), 0x17: ('fsor', 'fs12r'),
    0x18: ('fmeq', 'fm'), 0x1A: ('fmand', 'fm'), 0x1B: ('fmor', 'fm'), 0x1C: ('fcget', 'fcget'),
    0x20: ('b', 'b'), 0x21: ('bal', 'bal'), 0x24: ('jr', 'jr'), 0x25: ('jalr', 'jalr'),
    0x28: ('ibeq', 'ib2'), 0x29: ('ibne', 'ib2'),
    0x2C: ('ibltz', 'ib1'), 0x2D: ('ibgtz', 'ib1'), 0x2E: ('iblez', 'ib1'), 0x2F: ('ibgez', 'ib1'),
}
LOWER_OP = {0x30: ('iadd', 'i3'), 0x31: ('isub', 'i3'), 0x32: ('iaddi', 'iaddi'),
            0x34: ('iand', 'i3'), 0x35: ('ior', 'i3')}
LOWER_T3 = {
    (0, 12): ('move', 'mv'), (0, 13): ('lqi', 'lqi'), (0, 14): ('div', 'div'), (0, 15): ('mtir', 'mtir'),
    (0, 16): ('rnext', 'r2'), (0, 25): ('mfp', 'mfp'), (0, 26): ('xtop', 'xt'), (0, 27): ('xgkick', 'xgkick'),
    (0, 28): ('esadd', 'ep'), (0, 29): ('eatanxy', 'ep'), (0, 30): ('esqrt', 'epf'), (0, 31): ('esin', 'epf'),
    (1, 12): ('mr32', 'mv'), (1, 13): ('sqi', 'sqi'), (1, 14): ('sqrt', 'sqrt'), (1, 15): ('mfir', 'mfir'),
    (1, 16): ('rget', 'r2'), (1, 26): ('xitop', 'xt'),
    (1, 28): ('ersadd', 'ep'), (1, 29): ('eatanxz', 'ep'), (1, 30): ('ersqrt', 'epf'), (1, 31): ('eatan', 'epf'),
    (2, 13): ('lqd', 'lqd'), (2, 14): ('rsqrt', 'div'), (2, 15): ('ilwr', 'ilwr'), (2, 16): ('rinit', 'r1'),
    (2, 28): ('eleng', 'ep'), (2, 29): ('esum', 'ep'), (2, 30): ('ercpr', 'epf'), (2, 31): ('eexp', 'epf'),
    (3, 13): ('sqd', 'sqd'), (3, 14): ('waitq', 'wait'), (3, 15): ('iswr', 'ilwr'), (3, 16): ('rxor', 'r1'),
    (3, 28): ('erleng', 'ep'), (3, 30): ('waitp', 'wait'),
}
LOWER_ENC = {}
for op, (n, k) in LOWER_MAIN.items():
    LOWER_ENC[n] = (k, op << 25)
for op, (n, k) in LOWER_OP.items():
    LOWER_ENC[n] = (k, 0x40 << 25 | op)
for (col, row), (n, k) in LOWER_T3.items():
    LOWER_ENC[n] = (k, 0x40 << 25 | row << 6 | 0x3C | col)
LOWER_NOP = 0x8000033C            # move vf00, vf00 with an empty dest
BRANCH_KINDS = ('b', 'bal', 'ib2', 'ib1')
DEST_KINDS = ('lq', 'sq', 'ilw', 'mv', 'lqi', 'sqi', 'lqd', 'sqd', 'mfir', 'ilwr', 'r2', 'mfp', 'ep')


def lower_entry(w):
    op = w >> 25
    if op == 0x40:
        sub = w & 0x3F
        if sub >= 0x3C:
            return LOWER_T3.get((sub & 3, (w >> 6) & 31))
        return LOWER_OP.get(sub)
    return LOWER_MAIN.get(op)


def branch_target(w, pc):
    """pc and the result are VU instruction indices."""
    return pc + 1 + _sext(w & 0x7FF, 11)


def decode_lower(w, pc=0, label=None):
    """-> text.  `label(target)` names a branch target (else a number)."""
    if w == LOWER_NOP:
        return 'nop'
    ent = lower_entry(w)
    if ent is None:
        raise VUError('bad lower opcode %08x' % w)
    name, kind = ent
    dest = (w >> 21) & 15
    t, s, d = (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31
    it, is_, id_ = t & 15, s & 15, d & 15
    fsf, ftf = BC[(w >> 21) & 3], BC[(w >> 23) & 3]
    imm11 = _sext(w & 0x7FF, 11)
    m = name + (_dest(dest) if kind in DEST_KINDS else '')
    if kind in BRANCH_KINDS:
        tgt = branch_target(w, pc)
        tl = label(tgt) if label else None
        tl = tl or str(tgt)
    o = {
        'lq': lambda: (vf(t), '%d(%s)' % (imm11, vi(is_))),
        'sq': lambda: (vf(s), '%d(%s)' % (imm11, vi(it))),
        'ilw': lambda: (vi(it), '%d(%s)' % (imm11, vi(is_))),
        'iu15': lambda: (vi(it), vi(is_), str(((w >> 10) & 0x7800) | (w & 0x7FF))),
        'fc24r': lambda: ('vi01', '0x%06x' % (w & 0xFFFFFF)),
        'fc24': lambda: ('0x%06x' % (w & 0xFFFFFF),),
        'fs12r': lambda: (vi(it), '0x%03x' % ((w >> 21 & 1) << 11 | (w & 0x7FF))),
        'fs12': lambda: ('0x%03x' % ((w >> 21 & 1) << 11 | (w & 0x7FF)),),
        'fm': lambda: (vi(it), vi(is_)),
        'fcget': lambda: (vi(it),),
        'b': lambda: (tl,),
        'bal': lambda: (vi(it), tl),
        'jr': lambda: (vi(is_),),
        'jalr': lambda: (vi(it), vi(is_)),
        'ib2': lambda: (vi(it), vi(is_), tl),
        'ib1': lambda: (vi(is_), tl),
        'i3': lambda: (vi(id_), vi(is_), vi(it)),
        'iaddi': lambda: (vi(it), vi(is_), str(_sext(d, 5))),
        'mv': lambda: (vf(t), vf(s)),
        'lqi': lambda: (vf(t), '(%s++)' % vi(is_)),
        'sqi': lambda: (vf(s), '(%s++)' % vi(it)),
        'lqd': lambda: (vf(t), '(--%s)' % vi(is_)),
        'sqd': lambda: (vf(s), '(--%s)' % vi(it)),
        'div': lambda: ('q', vf(s) + fsf, vf(t) + ftf),
        'sqrt': lambda: ('q', vf(t) + ftf),
        'mtir': lambda: (vi(it), vf(s) + fsf),
        'mfir': lambda: (vf(t), vi(is_)),
        'ilwr': lambda: (vi(it), '(%s)' % vi(is_)),
        'r1': lambda: ('r', vf(s) + fsf),
        'r2': lambda: (vf(t), 'r'),
        'mfp': lambda: (vf(t), 'p'),
        'xt': lambda: (vi(it),),
        'xgkick': lambda: (vi(is_),),
        'wait': lambda: (),
        'ep': lambda: ('p', vf(s)),
        'epf': lambda: ('p', vf(s) + fsf),
    }[kind]()
    return (m + ' ' + ', '.join(o)).rstrip()


def _imm(s, lo, hi):
    try:
        v = int(s, 0)
    except ValueError:
        raise VUError('bad immediate: ' + s)
    if not lo <= v <= hi:
        raise VUError('immediate out of range: ' + s)
    return v


def _mem(s, pat):
    m = re.match(pat, s)
    if not m:
        raise VUError('bad memory operand: ' + s)
    return m.groups()


def encode_lower(mnem, ops, pc=0, labels=None):
    if mnem == 'nop' and not ops:
        return LOWER_NOP
    name, dest, fl = _split_mnem(mnem)
    if fl or name not in LOWER_ENC:
        raise VUError('unknown lower op: ' + mnem)
    kind, w = LOWER_ENC[name]
    if dest and kind not in DEST_KINDS:
        raise VUError('%s takes no dest' % name)
    w |= _parse_dest(dest) << 21

    def tgt(s):
        if labels is not None and s in labels:
            v = labels[s]
        else:
            v = _imm(s, -(1 << 20), 1 << 20)
        off = v - pc - 1
        if not -1024 <= off <= 1023:
            raise VUError('branch out of range: ' + s)
        return off & 0x7FF

    def comp(s, shift):
        r, c = _pvf(s, comp=True)
        return r, c << shift

    def n(k):
        if len(ops) != k:
            raise VUError('%s takes %d operands' % (name, k))

    if kind == 'lq':
        n(2); imm, r = _mem(ops[1], r'^(-?\d+)\((vi\d+)\)$')
        return w | _pvf(ops[0]) << 16 | _pvi(r) << 11 | _imm(imm, -1024, 1023) & 0x7FF
    if kind == 'sq':
        n(2); imm, r = _mem(ops[1], r'^(-?\d+)\((vi\d+)\)$')
        return w | _pvf(ops[0]) << 11 | _pvi(r) << 16 | _imm(imm, -1024, 1023) & 0x7FF
    if kind == 'ilw':
        n(2); imm, r = _mem(ops[1], r'^(-?\d+)\((vi\d+)\)$')
        return w | _pvi(ops[0]) << 16 | _pvi(r) << 11 | _imm(imm, -1024, 1023) & 0x7FF
    if kind == 'iu15':
        n(3); v = _imm(ops[2], 0, 0x7FFF)
        return w | _pvi(ops[0]) << 16 | _pvi(ops[1]) << 11 | (v & 0x7800) << 10 | v & 0x7FF
    if kind == 'fc24r':
        n(2)
        if ops[0] != 'vi01':
            raise VUError('%s writes vi01' % name)
        return w | _imm(ops[1], 0, 0xFFFFFF)
    if kind == 'fc24':
        n(1); return w | _imm(ops[0], 0, 0xFFFFFF)
    if kind in ('fs12r', 'fs12'):
        n(2 if kind == 'fs12r' else 1)
        v = _imm(ops[-1], 0, 0xFFF)
        w |= (v >> 11) << 21 | v & 0x7FF
        return w | (_pvi(ops[0]) << 16 if kind == 'fs12r' else 0)
    if kind == 'fm':
        n(2); return w | _pvi(ops[0]) << 16 | _pvi(ops[1]) << 11
    if kind in ('fcget', 'xt'):
        n(1); return w | _pvi(ops[0]) << 16
    if kind == 'b':
        n(1); return w | tgt(ops[0])
    if kind == 'bal':
        n(2); return w | _pvi(ops[0]) << 16 | tgt(ops[1])
    if kind in ('jr', 'xgkick'):
        n(1); return w | _pvi(ops[0]) << 11
    if kind == 'jalr':
        n(2); return w | _pvi(ops[0]) << 16 | _pvi(ops[1]) << 11
    if kind == 'ib2':
        n(3); return w | _pvi(ops[0]) << 16 | _pvi(ops[1]) << 11 | tgt(ops[2])
    if kind == 'ib1':
        n(2); return w | _pvi(ops[0]) << 11 | tgt(ops[1])
    if kind == 'i3':
        n(3); return w | _pvi(ops[0]) << 6 | _pvi(ops[1]) << 11 | _pvi(ops[2]) << 16
    if kind == 'iaddi':
        n(3); return w | _pvi(ops[0]) << 16 | _pvi(ops[1]) << 11 | (_imm(ops[2], -16, 15) & 31) << 6
    if kind == 'mv':
        n(2); return w | _pvf(ops[0]) << 16 | _pvf(ops[1]) << 11
    if kind in ('lqi', 'lqd'):
        n(2); pat = r'^\((vi\d+)\+\+\)$' if kind == 'lqi' else r'^\(--(vi\d+)\)$'
        return w | _pvf(ops[0]) << 16 | _pvi(_mem(ops[1], pat)[0]) << 11
    if kind in ('sqi', 'sqd'):
        n(2); pat = r'^\((vi\d+)\+\+\)$' if kind == 'sqi' else r'^\(--(vi\d+)\)$'
        return w | _pvf(ops[0]) << 11 | _pvi(_mem(ops[1], pat)[0]) << 16
    if kind == 'div':
        n(3)
        if ops[0] != 'q':
            raise VUError('expected q')
        s, sf = comp(ops[1], 21); t, tf = comp(ops[2], 23)
        return w | s << 11 | t << 16 | sf | tf
    if kind == 'sqrt':
        n(2)
        if ops[0] != 'q':
            raise VUError('expected q')
        t, tf = comp(ops[1], 23)
        return w | t << 16 | tf
    if kind == 'mtir':
        n(2); s, sf = comp(ops[1], 21)
        return w | _pvi(ops[0]) << 16 | s << 11 | sf
    if kind == 'mfir':
        n(2); return w | _pvf(ops[0]) << 16 | _pvi(ops[1]) << 11
    if kind == 'ilwr':
        n(2); return w | _pvi(ops[0]) << 16 | _pvi(_mem(ops[1], r'^\((vi\d+)\)$')[0]) << 11
    if kind in ('r1', 'ep', 'epf'):
        n(2)
        if ops[0] != ('r' if kind == 'r1' else 'p'):
            raise VUError('bad first operand for ' + name)
        if kind == 'ep':
            return w | _pvf(ops[1]) << 11
        s, sf = comp(ops[1], 21)
        return w | s << 11 | sf
    if kind in ('r2', 'mfp'):
        n(2)
        if ops[1] != ('r' if kind == 'r2' else 'p'):
            raise VUError('bad second operand for ' + name)
        return w | _pvf(ops[0]) << 16
    if kind == 'wait':
        n(0); return w
    raise AssertionError(kind)


# ------------------------------------------------------------------- pairs --

def fmt_float(bits):
    """A LOI immediate as a decimal that round-trips to the same 32 bits,
    else as hex."""
    f = struct.unpack('<f', struct.pack('<I', bits))[0]
    if f == f and abs(f) != float('inf'):
        for p in range(1, 10):
            s = '%.*g' % (p, f)
            try:
                back = struct.unpack('<I', struct.pack('<f', float(s)))[0]
            except OverflowError:       # rounded past FLT_MAX
                continue
            if back == bits:
                if 'e' not in s and '.' not in s:
                    s += '.0'
                return s
    return '0x%08x' % bits


def _tokens(text):
    """Split `upper ops... lower ops...` into two (mnemonic, [operands])."""
    toks = text.split()
    insns, i = [], 0
    while i < len(toks):
        m, i = toks[i], i + 1
        ops = []
        base = _split_mnem(m)[0] if re.match(r'^[a-z]', m) else m
        if base not in ('nop', 'waitq', 'waitp'):
            while i < len(toks):
                t, i = toks[i], i + 1
                more = t.endswith(',')
                ops.extend(x for x in t.split(',') if x)
                if not more:
                    break
        insns.append((m, ops))
    return insns


def encode_pair(text, pc=0, labels=None):
    """-> (lower, upper)."""
    text = text.split(';')[0].strip()
    m = re.match(r'^\.pair\s+(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)$', text)
    if m:
        return int(m.group(1), 16), int(m.group(2), 16)
    insns = _tokens(text)
    if len(insns) != 2:
        raise VUError('expected an upper and a lower instruction: ' + text)
    (um, uo), (lm, lo) = insns
    up = encode_upper(um, uo)
    if lm == 'loi':
        if len(lo) != 1:
            raise VUError('loi takes one operand')
        s = lo[0]
        if s.startswith('0x'):
            low = int(s, 16)
        else:
            try:
                low = struct.unpack('<I', struct.pack('<f', float(s)))[0]
            except (OverflowError, ValueError):
                raise VUError('bad loi value: ' + s)
        return low, up | 1 << 31
    return encode_lower(lm, lo, pc, labels), up


def decode_pair(low, up, pc=0, label=None):
    """-> text that encode_pair turns back into exactly (low, up)."""
    try:
        u = decode_upper(up & ~(1 << 31))
        if up >> 31 & 1:
            lt = 'loi ' + fmt_float(low)
        else:
            lt = decode_lower(low, pc, label)
        text = '%-36s %s' % (u, lt)
    except VUError as e:
        return '.pair 0x%08x, 0x%08x      ; %s' % (low, up, e)
    # Every field we print must be every field there is.
    labels = _LabelMap(label)
    try:
        ok = encode_pair(text, pc, labels) == (low, up)
    except VUError:
        ok = False
    if not ok:
        return '.pair 0x%08x, 0x%08x      ; non-canonical: %s' % (low, up, text)
    return text


class _LabelMap(dict):
    """Inverse of a label function, built lazily over the targets it named."""
    def __init__(self, label):
        super().__init__()
        self.label = label

    def __contains__(self, k):
        return self.label is not None and self._find(k) is not None

    def __getitem__(self, k):
        return self._find(k)

    def _find(self, k):
        for tgt in range(0, 2048):
            if self.label(tgt) == k:
                return tgt
        return None


# --------------------------------------------------------- the programs -----

def load_symbols():
    syms = {}
    with open(SYMBOLS) as f:
        for line in f:
            p = line.split()
            if len(p) >= 3 and p[0].startswith('0x'):
                syms[p[-1]] = int(p[0], 16)
    return syms


def entry_labels(syms, prefix, dma_name):
    """{vu index: name} for a program's label symbols (VIR*/VIBb* in the data
    section, below the image's first code address)."""
    found = {n: a for n, a in syms.items()
             if n.startswith(prefix) and a < 0x003EE000 and n != dma_name}
    if not found:
        return {}
    lo = min(found.values())
    return {(a - lo) // 8: n for n, a in sorted(found.items(), key=lambda x: x[1])}


def split_blob(data, off):
    """Walk one DMA RET packet holding a VIF stream.  -> (tag, items, size)
    where items are ('vif', word) and ('mpg', vu_addr, [(low, up), ...])."""
    lo, hi = struct.unpack_from('<II', data, off)
    qwc, tid = lo & 0xFFFF, lo >> 16
    end = off + 16 + qwc * 16
    p, items = off + 8, []
    while p < end:
        w = struct.unpack_from('<I', data, p)[0]
        cmd = (w >> 24) & 0x7F
        if cmd == 0x4A:
            num = (w >> 16) & 0xFF or 256
            pairs = [struct.unpack_from('<II', data, p + 4 + 8 * k) for k in range(num)]
            items.append(('mpg', w & 0xFFFF, w & 0x80000000, pairs))
            p += 4 + 8 * num
        else:
            items.append(('vif', w))
            p += 4
    if p != end:
        raise VUError('VIF stream overruns its DMA packet')
    return (tid, hi), items, end - off


def build_blob(tag, items):
    tid, hi = tag
    words = []
    for it in items:
        if it[0] == 'vif':
            words.append(it[1])
        else:
            _, addr, extra, pairs = it
            words.append(0x4A000000 | (len(pairs) & 0xFF) << 16 | extra | addr)
            for low, up in pairs:
                words += [low, up]
    if (len(words) + 2) % 4:
        raise VUError('VIF stream does not end on a quadword (%d words)' % len(words))
    qwc = (len(words) + 2) // 4 - 1
    return struct.pack('<II', qwc | tid << 16, hi) + struct.pack('<%dI' % len(words), *words)


def write_vsm(name, dma_name, va, tag, items, labels):
    out = ['; %s -- VU1 microprogram, uploaded from %s (0x%08X).' % (name, dma_name, va),
           '; Generated by vudis.py extract; `vudis.py check` assembles it back',
           '; against the SUPPORT image.  One line per instruction pair, upper first.',
           '',
           '.dmatag 0x%04x, 0x%08x    ; tag bits 16..31, ADDR; QWC is derived' % tag]
    auto = {}
    targets = set()
    for it in items:
        if it[0] == 'mpg':
            for k, (low, up) in enumerate(it[3]):
                pc = it[1] + k
                if not up >> 31 & 1:
                    ent = lower_entry(low)
                    if ent and ent[1] in BRANCH_KINDS and low != LOWER_NOP:
                        targets.add(branch_target(low, pc))
    for t in sorted(targets):
        if t not in labels:
            auto[t] = 'L%04d' % t
    names = {**labels, **auto}
    label = names.get
    for it in items:
        if it[0] == 'vif':
            out.append('.vif 0x%08x' % it[1])
            continue
        _, addr, extra, pairs = it
        out.append('.mpg %d%s' % (addr, ', 0x%08x' % extra if extra else ''))
        for k, (low, up) in enumerate(pairs):
            pc = addr + k
            if pc in names:
                out.append('%s:' % names[pc])
            out.append('    %-72s ; %4d' % (decode_pair(low, up, pc, label), pc))
    return '\n'.join(out) + '\n'


def read_vsm(path):
    """-> (tag, items) built from a .vsm source."""
    lines = open(path).read().splitlines()
    # pass 1: labels
    labels, pc = {}, None
    for ln in lines:
        s = ln.split(';')[0].strip()
        if not s:
            continue
        if s.startswith('.mpg'):
            pc = int(s.split()[1].rstrip(','), 0)
        elif s.endswith(':'):
            labels[s[:-1]] = pc
        elif not s.startswith('.') or s.startswith('.pair'):
            pc += 1
    tag, items, pc = None, [], None
    for n, ln in enumerate(lines, 1):
        s = ln.split(';')[0].strip()
        if not s or s.endswith(':'):
            continue
        try:
            if s.startswith('.dmatag'):
                a, b = [int(x, 0) for x in s.split(None, 1)[1].split(',')]
                tag = (a, b)
            elif s.startswith('.vif'):
                items.append(('vif', int(s.split()[1], 0)))
            elif s.startswith('.mpg'):
                a = [int(x, 0) for x in s.split(None, 1)[1].split(',')]
                pc = a[0]
                items.append(('mpg', a[0], a[1] if len(a) > 1 else 0, []))
            else:
                items[-1][3].append(encode_pair(s, pc, labels))
                pc += 1
        except (VUError, ValueError, IndexError) as e:
            raise VUError('%s:%d: %s' % (path, n, e))
    return tag, items


def image():
    return open(IMAGE, 'rb').read()


def cmd_extract(force=False):
    """src/*.vsm become HAND-MAINTAINED source once annotated (comments are
    free: `check` ignores them), so an existing file is never overwritten
    without --force."""
    data, syms = image(), load_symbols()
    os.makedirs(os.path.join(HERE, 'src'), exist_ok=True)
    for name, dma, prefix in PROGRAMS:
        va = syms[dma]
        tag, items, size = split_blob(data, va - IMAGE_BASE)
        labels = entry_labels(syms, prefix, dma)
        text = write_vsm(name, dma, va, tag, items, labels)
        path = os.path.join(HERE, 'src', name + '.vsm')
        if os.path.exists(path) and not force:
            print('%-10s exists, not overwritten (--force)' % name)
            continue
        open(path, 'w').write(text)
        npairs = sum(len(i[3]) for i in items if i[0] == 'mpg')
        raw = text.count('\n    .pair')
        print('%-10s %5d pairs, %5d bytes, %d labels, %d raw -> %s'
              % (name, npairs, size, len(labels), raw, os.path.relpath(path)))


def cmd_check():
    data, syms = image(), load_symbols()
    bad = 0
    for name, dma, prefix in PROGRAMS:
        va = syms[dma]
        _, _, size = split_blob(data, va - IMAGE_BASE)
        want = data[va - IMAGE_BASE: va - IMAGE_BASE + size]
        tag, items = read_vsm(os.path.join(HERE, 'src', name + '.vsm'))
        got = build_blob(tag, items)
        if got == want:
            print('%-10s MATCH  %d bytes' % (name, size))
        else:
            bad += 1
            n = min(len(got), len(want))
            first = next((i for i in range(n) if got[i] != want[i]), n)
            print('%-10s DIFFER at byte %d (got %d bytes, want %d)' % (name, first, len(got), len(want)))
    return 1 if bad else 0


def main(argv):
    if not argv or argv[0] in ('-h', '--help'):
        print(__doc__)
        return 0
    if argv[0] == 'extract':
        cmd_extract('--force' in argv[1:])
        return 0
    if argv[0] == 'check':
        return cmd_check()
    if argv[0] == 'dis':
        vals = [int(x, 16) for x in argv[1:]]
        for k in range(0, len(vals) - 1, 2):
            print(decode_pair(vals[k], vals[k + 1], k // 2))
        return 0
    print('unknown command: ' + argv[0])
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
