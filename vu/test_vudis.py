#!/usr/bin/env python3
"""
Canary for vudis.py, in the style of ../test_eqdecomp.py: every check that
can be paired with a negative control is, and the runner prints how many
assertions actually ran.

Run: ./test_vudis.py   (also works under pytest)
"""

import os
import random
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import vudis as V

CHECKS = 0
# PCSX2's interpreter tables: $PCSX2_SRC, else the checkout beside decomp/
PCSX2_VUOPS = os.path.join(os.environ.get('PCSX2_SRC') or os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'pcsx2'),
    'pcsx2', 'VUops.cpp')


class Skipped(Exception):
    pass


def check(cond, msg):
    global CHECKS
    CHECKS += 1
    assert cond, msg


# ---------------------------------------------- tables against PCSX2 -------

def _pcsx2_tables():
    if not os.path.exists(PCSX2_VUOPS):
        raise Skipped('no PCSX2 source at ' + PCSX2_VUOPS)
    src = open(PCSX2_VUOPS).read()
    out = {}
    for m in re.finditer(r'FNTYPE PREFIX##(\w+)\[(\d+)\] = \{(.*?)\};', src, re.S):
        names = re.findall(r'PREFIX##(\w+)', m.group(3))
        assert len(names) == int(m.group(2)), m.group(1)
        out[m.group(1)] = [n[3:].lower() if n.startswith('MI_') else None for n in names]
    return out


# PCSX2's interpreter names where the dvp-as spelling differs.
_ALIAS = {'clip': 'clipw'}


def _ours():
    """Our tables laid out the way PCSX2 lays out its own."""
    t = {}
    t['_UPPER_OPCODE'] = [V.UPPER_MAIN.get(op, (None,))[0] for op in range(0x3C)]
    for col, suf in enumerate(('00', '01', '10', '11')):
        t['_UPPER_FD_%s_TABLE' % suf] = [V.UPPER_T3.get((row, col), (None,))[0] for row in range(32)]
        t['LowerOP_T3_%s_OPCODE' % suf] = [V.LOWER_T3.get((col, row), (None,))[0] for row in range(32)]
    t['LowerOP_OPCODE'] = [V.LOWER_OP.get(op, (None,))[0] for op in range(0x3C)]
    t['_LOWER_OPCODE'] = [V.LOWER_MAIN.get(op, (None,))[0] for op in range(0x40)]
    return t


def _diff_tables(ours, theirs):
    bad = []
    for name, mine in ours.items():
        ref = theirs[name][:len(mine)]
        for i, (a, b) in enumerate(zip(mine, ref)):
            if a != _ALIAS.get(b, b):
                bad.append('%s[%d]: ours %s, PCSX2 %s' % (name, i, a, b))
    return bad


def test_tables_match_pcsx2_interpreter():
    theirs = _pcsx2_tables()
    check(set(_ours()) <= set(theirs), 'PCSX2 table names moved')
    bad = _diff_tables(_ours(), theirs)
    check(not bad, '\n'.join(bad))
    # the upper table's 0x3C..0x3F and LowerOP's 0x3C..0x3F dispatch to the
    # sub-tables in PCSX2 too, and LOWER 0x40 is LowerOP
    check(theirs['_LOWER_OPCODE'][0x40] is None and theirs['_LOWER_OPCODE'][0x41:] == [None] * 63,
          'lower 0x40 should be the only entry above 0x3F')


def test_tables_negative_control():
    theirs = _pcsx2_tables()
    t = _ours()
    t['_UPPER_FD_00_TABLE'][1] = 'subx'       # PCSX2's DEBUGGER has this bug
    t['LowerOP_T3_10_OPCODE'][28] = 'erleng'  # swap with T3_11
    check(len(_diff_tables(t, theirs)) == 2, 'a corrupted table must show both edits')


# ------------------------------------------------------- round trips -------

LO_DEST, LO_T5, LO_S5, LO_IT, LO_IS, LO_ID, LO_IMM = (
    0x01E00000, 0x001F0000, 0x0000F800, 0x000F0000, 0x00007800, 0x000003C0, 0x7FF)
LOWER_FIELDS = {
    'lq': LO_DEST | LO_T5 | LO_IS | LO_IMM, 'sq': LO_DEST | LO_S5 | LO_IT | LO_IMM,
    'ilw': LO_DEST | LO_IT | LO_IS | LO_IMM, 'iu15': LO_DEST | LO_IT | LO_IS | LO_IMM,
    'fc24r': 0xFFFFFF, 'fc24': 0xFFFFFF, 'fs12r': LO_IT | 0x00200000 | LO_IMM,
    'fs12': 0x00200000 | LO_IMM, 'fm': LO_IT | LO_IS, 'fcget': LO_IT, 'xt': LO_IT,
    'b': LO_IMM, 'bal': LO_IT | LO_IMM, 'jr': LO_IS, 'xgkick': LO_IS, 'jalr': LO_IT | LO_IS,
    'ib2': LO_IT | LO_IS | LO_IMM, 'ib1': LO_IS | LO_IMM, 'i3': LO_ID | LO_IS | LO_IT,
    'iaddi': LO_IT | LO_IS | 0x7C0, 'mv': LO_DEST | LO_T5 | LO_S5, 'lqi': LO_DEST | LO_T5 | LO_IS,
    'lqd': LO_DEST | LO_T5 | LO_IS, 'sqi': LO_DEST | LO_S5 | LO_IT, 'sqd': LO_DEST | LO_S5 | LO_IT,
    'div': LO_DEST | LO_T5 | LO_S5, 'sqrt': 0x01800000 | LO_T5, 'mtir': 0x00600000 | LO_IT | LO_S5,
    'mfir': LO_DEST | LO_T5 | LO_IS, 'ilwr': LO_DEST | LO_IT | LO_IS, 'r1': 0x00600000 | LO_S5,
    'r2': LO_DEST | LO_T5, 'mfp': LO_DEST | LO_T5, 'wait': 0, 'ep': LO_DEST | LO_S5,
    'epf': 0x00600000 | LO_S5,
}
UP_FLAGS, UP_DEST, UP_FT, UP_FS, UP_FD = 0x78000000, 0x01E00000, 0x1F0000, 0xF800, 0x7C0
UPPER_FIELDS = {
    'bc': UP_DEST | UP_FD | UP_FS | UP_FT, 'full': UP_DEST | UP_FD | UP_FS | UP_FT,
    'q': UP_DEST | UP_FD | UP_FS, 'i': UP_DEST | UP_FD | UP_FS,
    'accbc': UP_DEST | UP_FS | UP_FT, 'accfull': UP_DEST | UP_FS | UP_FT,
    'accq': UP_DEST | UP_FS, 'acci': UP_DEST | UP_FS,
    'cvt': UP_DEST | UP_FS | UP_FT, 'clip': UP_DEST | UP_FS | UP_FT, 'nop': 0,
}
NOP_UP = 0x000002FF


def _lower_bases():
    for name, (kind, base) in V.LOWER_ENC.items():
        yield name, kind, base


def _upper_bases():
    for name, encs in V.UPPER_ENC.items():
        for kind, base in encs:
            yield name, kind, base


def test_every_lower_op_round_trips():
    rng = random.Random(1)
    for name, kind, base in _lower_bases():
        for _ in range(300):
            w = base | rng.getrandbits(32) & LOWER_FIELDS[kind]
            text = V.decode_pair(w, NOP_UP, pc=1024)
            if w != V.LOWER_NOP:
                check(not text.startswith('.pair') and (' ' + name) in text,
                      '%s %08x decoded as %r' % (name, w, text))
            check(V.encode_pair(text, 1024) == (w, NOP_UP), '%s %08x: %r' % (name, w, text))


def test_every_upper_op_round_trips():
    rng = random.Random(2)
    for name, kind, base in _upper_bases():
        for _ in range(300):
            w = base | rng.getrandbits(32) & (UPPER_FIELDS[kind] | UP_FLAGS)
            text = V.decode_pair(V.LOWER_NOP, w)
            check(not text.startswith('.pair') and text.startswith(name),
                  '%s %08x decoded as %r' % (name, w, text))
            check(V.encode_pair(text) == (V.LOWER_NOP, w), '%s %08x: %r' % (name, w, text))


def test_loi_round_trips_every_float_shape():
    rng = random.Random(3)
    for bits in [0, 0x80000000, 0x3F800000, 0x7F800000, 0x7FC00000, 0x00000001, 0x7F7FFFFF] + \
            [rng.getrandbits(32) for _ in range(3000)]:
        up = NOP_UP | 1 << 31
        text = V.decode_pair(bits, up)
        check(' loi ' in text, text)
        check(V.encode_pair(text) == (bits, up), '%08x: %r' % (bits, text))


def test_unused_bits_are_refused_negative_control():
    """A canonical pair with one bit set outside its fields must NOT decode
    to text -- otherwise the check that keeps `.pair` honest compares nothing."""
    rng = random.Random(4)
    hit = 0
    for name, kind, base in _lower_bases():
        free = ~(LOWER_FIELDS[kind] | 0xFE000000 | (0x7FF if base >> 25 == 0x40 else 0)) & 0x01FFFFFF
        bits = [b for b in range(25) if free >> b & 1]
        if not bits:
            continue
        w = base | 1 << rng.choice(bits)
        text = V.decode_pair(w, NOP_UP)
        check(text.startswith('.pair'), '%s %08x should be raw: %r' % (name, w, text))
        check(V.encode_pair(text) == (w, NOP_UP), 'raw pair must still round-trip')
        hit += 1
    check(hit > 20, 'the control exercised only %d ops' % hit)


def test_random_words_always_round_trip():
    rng = random.Random(5)
    for _ in range(20000):
        low, up = rng.getrandbits(32), rng.getrandbits(32)
        check(V.encode_pair(V.decode_pair(low, up, 100), 100) == (low, up), '%08x %08x' % (low, up))


# -------------------------------------------------- against the image ------

def _need_image():
    if not os.path.exists(V.IMAGE):
        raise Skipped('no SUPPORT image at ' + V.IMAGE)


def test_sources_assemble_to_the_image():
    _need_image()
    data, syms = V.image(), V.load_symbols()
    for name, dma, _ in V.PROGRAMS:
        off = syms[dma] - V.IMAGE_BASE
        _, _, size = V.split_blob(data, off)
        tag, items = V.read_vsm(os.path.join(V.HERE, 'src', name + '.vsm'))
        got = V.build_blob(tag, items)
        check(got == data[off:off + size], name + ' does not assemble to the image')
        src = open(os.path.join(V.HERE, 'src', name + '.vsm')).read()
        check('.pair' not in src, name + ' has raw pairs')
        # negative control: one changed register must change the bytes
        bad = src.replace('vf01,', 'vf02,', 1)
        path = os.path.join(V.HERE, 'src', '.ctl.vsm')
        open(path, 'w').write(bad)
        try:
            check(V.build_blob(*V.read_vsm(path)) != got, 'an edit did not change the build')
        finally:
            os.remove(path)


def _ends_flow(text):
    """The pair ends control flow: an [e] or an unconditional `b`."""
    if not text:
        return False
    up = text.split()[0]
    return up.endswith('[e]') or re.search(r'\sb\s', ' ' + text + ' ') is not None


def _entry_check(labels, pairs):
    bad = []
    for pc, name in labels.items():
        if pc == 0:
            continue
        # the end of flow is the pair before the entry or the one before
        # that; VIBbRender is itself the delay slot of VIBbInit's [e]
        if not any(_ends_flow(pairs.get(pc - k, '')) for k in (1, 2)):
            bad.append('%s at %d: pairs %d..%d do not end flow' % (name, pc, pc - 2, pc - 1))
    return bad


def _program(name, dma, prefix):
    data, syms = V.image(), V.load_symbols()
    _, items, _ = V.split_blob(data, syms[dma] - V.IMAGE_BASE)
    pairs = {}
    for it in items:
        if it[0] == 'mpg':
            for k, (lo, up) in enumerate(it[3]):
                pairs[it[1] + k] = V.decode_pair(lo, up, it[1] + k)
    return V.entry_labels(syms, prefix, dma), pairs


def test_entry_labels_follow_an_end_of_flow():
    """Every entry point but the first sits right after an [e] or a `b`
    (or its delay slot) -- which is what pins the label -> VU address mapping."""
    _need_image()
    n = 0
    for prog in V.PROGRAMS:
        labels, pairs = _program(*prog)
        bad = _entry_check(labels, pairs)
        check(not bad, '\n'.join(bad))
        n += len(labels) - 1
    check(n == 9, 'expected 9 non-first entry points, saw %d' % n)


def test_entry_labels_negative_control():
    _need_image()
    labels, pairs = _program(*V.PROGRAMS[1])
    shifted = {pc + 1: n for pc, n in labels.items()}
    check(len(_entry_check(shifted, pairs)) >= 6, 'labels shifted by one should fail')


def main():
    tests = [(n, f) for n, f in sorted(globals().items()) if n.startswith('test_')]
    skipped = failed = 0
    for n, f in tests:
        try:
            f()
            print('ok    ' + n)
        except Skipped as e:
            skipped += 1
            print('SKIP  %s: %s' % (n, e))
        except Exception as e:
            failed += 1
            print('FAIL  %s: %s: %s' % (n, type(e).__name__, e))
    print('%d tests, %d failed, %d skipped, %d checks' % (len(tests), failed, skipped, CHECKS))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
