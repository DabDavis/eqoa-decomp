#!/usr/bin/env python3
"""
Canary for vusim.py and spec_strip.py, in the style of test_vudis.py.

The float core is checked against EXACT rational arithmetic; the pipeline and
the spec are checked against a live vu1dump capture, which is not in the repo:

    memory-dumps/vu1/inworld.bin   (see README.md, "The oracle")

Without it those tests SKIP, loudly.  Run: ./test_vusim.py
"""
import os
import random
import struct
import sys
from fractions import Fraction

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import vusim as S

CHECKS = 0
CAPTURE = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
                       'memory-dumps', 'vu1', 'inworld.bin')
SUBSET = 400


class Skipped(Exception):
    pass


def check(cond, msg):
    global CHECKS
    CHECKS += 1
    assert cond, msg


# --------------------------------------------------- the float core ---------

def chop_exact(x):
    """Round an exact rational toward zero to single, with FTZ and saturation."""
    if x == 0:
        return 0
    sign = 0x80000000 if x < 0 else 0
    a = abs(x)
    e = a.numerator.bit_length() - a.denominator.bit_length()
    while Fraction(2) ** e > a:
        e -= 1
    while Fraction(2) ** (e + 1) <= a:
        e += 1
    if e + 127 >= 255:
        return sign | S.FMAX
    if e + 127 <= 0:
        return sign
    m = int(a / Fraction(2) ** (e - 23))          # floor: toward zero
    return sign | (e + 127) << 23 | (m & 0x7FFFFF)


def rnd_float(rng):
    """Normal singles, biased toward cancellation and wide exponent gaps."""
    e = rng.choice([rng.randint(1, 254), rng.randint(100, 154), rng.randint(120, 134)])
    return rng.getrandbits(1) << 31 | e << 23 | rng.getrandbits(23)


def test_float_ops_are_exact_chop():
    rng = random.Random(7)
    for _ in range(20000):
        a, b = rnd_float(rng), rnd_float(rng)
        if rng.random() < 0.2:                     # near-cancellation
            b = (a ^ 0x80000000) + rng.randint(-3, 3)
        fa, fb = Fraction(S.u2f(a)), Fraction(S.u2f(b))
        check(S.f_add(a, b) == chop_exact(fa + fb) or fa + fb == 0, 'add %08x %08x' % (a, b))
        check(S.f_mul(a, b) == chop_exact(fa * fb), 'mul %08x %08x' % (a, b))
        if fb:
            check(S.f_div(a, b) == chop_exact(fa / fb), 'div %08x %08x' % (a, b))
    for _ in range(5000):
        a = rnd_float(rng) & 0x7FFFFFFF
        r = S.f_sqrt(a)
        fr, fa = Fraction(S.u2f(r)), Fraction(S.u2f(a))
        nxt = Fraction(S.u2f(r + 1))
        check(fr * fr <= fa < nxt * nxt, 'sqrt %08x -> %08x' % (a, r))


def test_float_negative_control():
    """Round-to-nearest must NOT pass the chop check -- else it compares nothing."""
    rng = random.Random(8)
    differ = 0
    for _ in range(2000):
        a = rnd_float(rng) & 0x807FFFFF | 127 << 23    # [1, 2): no overflow
        b = rnd_float(rng) & 0x807FFFFF | 127 << 23
        near = struct.unpack('<I', struct.pack('<f', S.u2f(a) * S.u2f(b)))[0]
        if near != S.f_mul(a, b):
            differ += 1
    check(differ > 500, 'round-to-nearest agreed with chop %d/2000 times' % (2000 - differ))


# ------------------------------------------------ against the capture -------

def _capture():
    if not os.path.exists(CAPTURE):
        raise Skipped('no capture at ' + CAPTURE)
    return S.load_records_prefix(CAPTURE, SUBSET + 1)


def _conform(recs):
    import vuconform as C
    bad = 0
    for k in range(len(recs) - 1):
        sim = S.run_record(recs[k])
        diffs = C.compare(sim, recs[k + 1])
        mem = sum(1 for a in sim.writes if sim.s.mem[a] != recs[k + 1].mem[a])
        if diffs or mem:
            bad += 1
    return bad


def test_conformance_against_pcsx2():
    recs = _capture()
    old, S.MODE = S.MODE, 'pcsx2'
    try:
        bad = _conform(recs)
    finally:
        S.MODE = old
    check(bad == 0, '%d of %d launches differ from PCSX2' % (bad, len(recs) - 1))


def test_conformance_negative_control():
    """A DIV one cycle SLOWER must break launches: the strips read Q at
    exactly seven cycles (div 343, mulq 350), so the timing model is
    load-bearing.  (One cycle faster breaks nothing, correctly: no read
    comes before seven.)"""
    recs = _capture()
    old, S.MODE = S.MODE, 'pcsx2'
    S.Q_LAT['div'] = 8
    try:
        bad = _conform(recs[:60])
    finally:
        S.Q_LAT['div'] = 7
        S.MODE = old
    check(bad > 10, 'a wrong DIV latency broke only %d launches' % bad)


def _strip_runs(recs, mod, variant='strip'):
    import test_spec_strip as T
    bad = runs = 0
    for rec in recs:
        got = T.snapshots(rec, variant)
        if got is None:
            continue
        runs += 1
        nv, b, pdiff, _, _ = T.check_run(got, variant, mod)
        bad += b + pdiff
    return runs, bad


_ALL = []


def _all():
    if not os.path.exists(CAPTURE):
        raise Skipped('no capture at ' + CAPTURE)
    if not _ALL:
        _ALL.extend(S.load_records(CAPTURE))
    return _ALL


def _skinned():
    return [r for r in _all() if r.addr == 668][:60]


def _patched(old, new):
    """spec_strip with one line replaced: the negative controls."""
    import importlib.util
    import spec_strip
    src = open(spec_strip.__file__).read()
    check(old in src, 'the control edit no longer applies: ' + old)
    mod = importlib.util.module_from_spec(importlib.util.spec_from_loader('spec_ctl', loader=None))
    exec(compile(src.replace(old, new), 'spec_ctl', 'exec'), mod.__dict__)
    return mod


def test_spec_strip2_strip3_match():
    import spec_strip
    old, S.MODE = S.MODE, 'pcsx2'
    try:
        for variant, entry in (('strip2', 429), ('strip3', 549)):
            runs, bad = _strip_runs([r for r in _all() if r.addr == entry][:60], spec_strip, variant)
            check(runs >= 50, '%s: only %d runs' % (variant, runs))
            check(bad == 0, '%s: %d mismatches' % (variant, bad))
    finally:
        S.MODE = old


def test_spec_strip2_strip3_negative_controls():
    """Each lighting/decode difference between the variants is load-bearing.
    The static.w control needs runs where it is NOT 1.0 (156 of the 1,550)."""
    old, S.MODE = S.MODE, 'pcsx2'
    try:
        s3 = [r for r in _all() if r.addr == 549][:15]
        s2 = [r for r in _all() if r.addr == 429 and r.vf[31][3] != 0x3F800000][:15]
        check(len(s2) == 15, 'no VIRStrip2 run with static.w != 1 in the capture')
        for recs, variant, o, n in (
                (s3, 'strip3', 'p = [f_add(f_mul(c1(itof(P[k])), sc[0]), f_mul(c1(pt[k]), ONE)) for k in range(3)]',
                 'p = [f_mul(c1(itof(P[k])), sc[0]) for k in range(3)]'),
                (s3, 'strip3', "return rgb + [C[3] if variant in ('strip', 'fan') else ONE]", 'return rgb + [C[3]]'),
                (s2, 'strip2', 'sunf = f_mul(c1(f_mul(c1(ndots), static[3])), C[3])', 'sunf = f_mul(c1(ndots), C[3])')):
            runs, bad = _strip_runs(recs, _patched(o, n), variant)
            check(bad > 20, '%s control %r broke only %d' % (variant, n, bad))
    finally:
        S.MODE = old


def test_spec_strip_matches():
    import spec_strip
    old, S.MODE = S.MODE, 'pcsx2'
    try:
        runs, bad = _strip_runs(_skinned(), spec_strip)
    finally:
        S.MODE = old
    check(runs >= 50, 'only %d VIRStrip runs in the subset' % runs)
    check(bad == 0, '%d mismatches' % bad)


def test_spec_strip_negative_control():
    import importlib.util
    import spec_strip
    src = open(spec_strip.__file__).read()
    bad_src = src.replace('rgb = [f_sub(rgb[k], f_mul(sunC[k], sun))',
                          'rgb = [f_add(rgb[k], f_mul(sunC[k], sun))')
    check(bad_src != src, 'the control edit no longer applies')
    spec = importlib.util.spec_from_loader('spec_ctl', loader=None)
    mod = importlib.util.module_from_spec(spec)
    exec(compile(bad_src, 'spec_ctl', 'exec'), mod.__dict__)
    old, S.MODE = S.MODE, 'pcsx2'
    try:
        runs, bad = _strip_runs(_skinned()[:20], mod)
    finally:
        S.MODE = old
    check(bad > 20, 'a flipped sun term broke only %d vertices' % bad)


def _layer_runs(n):
    import test_spec_layer as T
    out = []
    for rec in _all()[:n]:
        out += T.layer_runs(rec)
    return out


def test_spec_layer_matches_and_controls():
    """The layer routines' packets at 539, plus a control per output field:
    the integer UV offset (type 1), the STQ w lane, the layer colour."""
    import importlib.util
    import spec_layer
    import test_spec_layer as T
    old, S.MODE = S.MODE, 'pcsx2'
    try:
        runs = _layer_runs(120)
        types = {snap[0] for snap, _ in runs}
        check(types == {1, 3}, 'layer types in the subset: %s' % sorted(types))
        bad = sum(1 for snap, km in runs if T.check(snap, km)[1])
        check(bad == 0, '%d of %d layer packets differ' % (bad, len(runs)))
        src = open(spec_layer.__file__).read()
        for o, n in (('        k = [S.itof(ftoi(x, 0), 0) for x in lanes]', '        k = [0, 0, 0, 0]'),
                     ('q, w]', 'q, q]'),
                     ('        col = [ftoi(f_mul(c1(rgba[c]), c1(mul[c])), 0) for c in range(4)]',
                      '        col = [ftoi(rgba[c], 0) for c in range(4)]')):
            check(o in src, 'control edit no longer applies: ' + o)
            m = importlib.util.module_from_spec(importlib.util.spec_from_loader('ctl', loader=None))
            exec(compile(src.replace(o, n), 'ctl', 'exec'), m.__dict__)
            broke = sum(1 for snap, km in runs if T.check(snap, km, m)[1])
            check(broke > 50, 'control %r broke only %d packets' % (n, broke))
    finally:
        S.MODE = old


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
