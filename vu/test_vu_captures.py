#!/usr/bin/env python3
"""
Canary for what the 2026-10-17 captures added: VIRFan, VIRStrip2's lit path,
VIRStrip5, VIRStrip6, the solid and wire layer routines, the clipper, and the
billboard program's conformance.  Every check is paired with a negative control: a
planted bug that must break it, so a pass means the data reaches that line.

The captures are not in the repo (memory-dumps/vu1/, see README.md, "Making
a capture"); a test whose capture is missing SKIPs, loudly.

    ./test_vu_captures.py          (~3 min)
"""
import importlib.util
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import vuconform as C
import vusim as S

CHECKS = 0
CAPS = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
                    'memory-dumps', 'vu1')


class Skipped(Exception):
    pass


def check(cond, msg):
    global CHECKS
    CHECKS += 1
    assert cond, msg


def capture(name):
    path = os.path.join(CAPS, name)
    if not os.path.exists(path):
        raise Skipped('no capture at ' + path)
    return path


def select(name, want, limit, pairs=False):
    """Records of a capture that `want(rec)` accepts, parsing only those
    (a capture is 100-200 MB).  pairs: also the record after each, which
    holds PCSX2's end state."""
    out = []
    with open(capture(name), 'rb') as f:
        prev = None
        while len(out) < limit:
            b = f.read(S.REC)
            if len(b) < S.REC:
                break
            if prev is not None:
                out.append((prev, S.Record(b, 0)))
                prev = None
            addr = struct.unpack_from('<I', b, 4)[0]
            if want.__code__.co_argcount == 1 and not want(addr):
                continue
            rec = S.Record(b, 0)
            if want.__code__.co_argcount == 2 and not want(addr, rec):
                continue
            if pairs:
                prev = rec
            else:
                out.append(rec)
    return out


def raster(rec):
    return C.program_of(rec)[0] == 'raster'


def patched(module, old, new=None):
    """`module` with source text replaced: a negative control.  `old` may be
    a list of (old, new) edits applied together."""
    src = open(module.__file__).read()
    for o, n in ([(old, new)] if new is not None else old):
        check(o in src, 'the control edit no longer applies: ' + o)
        src = src.replace(o, n)
    m = importlib.util.module_from_spec(importlib.util.spec_from_loader('ctl', loader=None))
    exec(compile(src, 'ctl', 'exec'), m.__dict__)
    return m


class pcsx2:
    def __enter__(self):
        self.old, S.MODE = S.MODE, 'pcsx2'

    def __exit__(self, *a):
        S.MODE = self.old


def conform(pairs):
    """Registers, plus every memory word the launch wrote -- except, when the
    NEXT launch is the other program, qwords 0..20: UploadMatrices re-sends
    the uniform block there before VIRInit, over the billboard's scratch
    word 0 (`isw.x vi01, 0(vi00)`)."""
    bad = 0
    for rec, nxt in pairs:
        sim = S.run_record(rec)
        lo = 21 * 4 if C.program_of(rec)[0] != C.program_of(nxt)[0] else 0
        mem = sum(1 for a in sim.writes if a >= lo and sim.s.mem[a] != nxt.mem[a])
        bad += bool(C.compare(sim, nxt) or mem)
    return bad


# ------------------------------------------------------------ VIRFan --------

def test_virfan_conformance_and_the_erleng_infinity():
    """The EULA's fans run with no point light: ERLENG of a zero vector.  The
    host's DIVSS makes that +inf and microVU saves it as P; the console
    saturates.  pcsx2 mode must match every launch, the console model must
    differ in P alone."""
    pairs = select('eula.bin', lambda a: a == 26, 60, pairs=True)
    with pcsx2():
        check(conform(pairs) == 0, 'VIRFan launches differ from PCSX2')
    bad = 0
    for rec, nxt in pairs:
        diffs = C.compare(S.run_record(rec), nxt)
        check({d[0] for d in diffs} <= {'P'}, 'console model differs beyond P: %s' % diffs[:3])
        bad += bool(diffs)
    check(bad >= 50, 'the console P differs in only %d launches' % bad)


def _strip_runs(recs, mod, variant):
    import test_spec_strip as T
    bad = runs = 0
    paths = set()
    for rec in recs:
        got = T.snapshots(rec, variant)
        if got is None:
            continue
        runs += 1
        nv, b, pdiff, p, _ = T.check_run(got, variant, mod)
        bad += b + pdiff
        paths |= set(p)
    return runs, bad, paths


def test_spec_fan():
    import spec_strip as SP
    eula = select('eula.bin', lambda a, r: a == 26 and raster(r), 80)
    sel = select('charsel.bin', lambda a, r: a == 26 and raster(r), 700)
    with pcsx2():
        for recs in (eula, sel):
            runs, bad, paths = _strip_runs(recs, SP, 'fan')
            check(runs >= 50 and bad == 0, 'fan: %d runs, %d mismatches' % (runs, bad))
        check('backface' in paths, 'no backfacing fan triangle in character select')
        for old, new in (('parity = 32 if fan else 32 - parity', 'parity = 32 - parity'),
                         ('        if fan:\n            s_prev1, code1', '        if False:\n            s_prev1, code1')):
            runs, bad, _ = _strip_runs(sel, patched(SP, old, new), 'fan')
            check(bad > 5, 'fan control %r broke only %d' % (new, bad))


# --------------------------------------------- VIRStrip2 lit, 5 and 6 -------

def test_spec_strip2lit_strip5_strip6():
    import spec_strip as SP
    lit = select('rain.bin', lambda a, r: a == 429 and r.vf[11][0] == 1 and raster(r), 60)
    s5 = select('rain.bin', lambda a, r: a == 807 and raster(r), 40)
    s6 = select('rain.bin', lambda a, r: a == 924 and raster(r), 40)
    controls = {
        'strip2lit': [("point = variant != 'strip2'", "point = variant not in ('strip2', 'strip2lit')")],
                      # Nothing else of the lit path is testable here: vf31
                      # (the static lighting) is (0, 0, 0, 1.0) in all 1,129 lit
                      # runs of rain.bin and dusk.bin, so neither the sun factor's
                      # c.w-then-static.w order nor static.rgb's place in the sum
                      # can change a result.
        'strip5': [("    if variant in ('strip', 'fan'):\n        sunf = ndots",
                    "    if variant in ('strip', 'fan', 'strip5'):\n        sunf = ndots")],
        'strip6': [('pw = f_mul(c1(f_sub(ONE, d)), k6)', 'pw = f_sub(ONE, d)'),
                   ('f_mul(c1(sunC[k]), k6)', 'sunC[k]')],
    }
    with pcsx2():
        for recs, variant in ((lit, 'strip2lit'), (s5, 'strip5'), (s6, 'strip6')):
            runs, bad, paths = _strip_runs(recs, SP, variant)
            check(runs >= 20 and bad == 0, '%s: %d runs, %d mismatches' % (variant, runs, bad))
            for old, new in controls[variant]:
                _, broke, _ = _strip_runs(recs, patched(SP, old, new), variant)
                check(broke > 5, '%s control %r broke only %d' % (variant, new or old, broke))


# ------------------------------------------------ solid and wire layers -----

def _layers(name, ptype, entries, nrec):
    import test_spec_layer as T
    out = []
    for rec in select(name, lambda a, r: a in entries and raster(r), nrec):
        out += [x for x in T.layer_runs(rec) if x[0][0] == ptype]
    return out


def test_spec_layer_solid_and_wire():
    import spec_layer as SL
    import test_spec_layer as T
    with pcsx2():
        for name, ptype, entries, ctl in (
                ('solid.bin', 0, (549, 807), [('out += [col, list(xyzf)]', 'out += [list(rgba), list(xyzf)]')]),
                ('wire.bin', 2, (26, 549, 807, 924), [('back2[:3] + [0x8000]', 'back2'),
                                 ('back2 = hub if fan else prev2', 'back2 = prev2'),
                                 ('            v = v[:3] + [S.sx16((w - 0x8000 + 16) & 0xFFFF) & 0xFFFFFFFF]',
                                  '            pass')])):
            runs = _layers(name, ptype, entries, 250)
            check(len(runs) >= 25, '%s: only %d type-%d packets' % (name, len(runs), ptype))
            bad = sum(1 for snap, km in runs if T.check(snap, km)[1])
            check(bad == 0, '%s: %d of %d packets differ' % (name, bad, len(runs)))
            for old, new in ctl:
                m = patched(SL, old, new)
                broke = sum(1 for snap, km in runs if T.check(snap, km, m)[1])
                check(broke > 3, '%s control %r broke only %d' % (name, new, broke))


# ------------------------------------------------------------ the clipper --

def test_spec_clip():
    """The polygon L1085/L0187 builds and the fan L1266 returns, over the
    clips in character select (strips and fans, all four side planes).
    Not testable in any capture: the order of the planes and a made vertex
    growing the plane set -- no made vertex carries a bit the triangle had not."""
    import spec_clip as SC
    import test_spec_clip as T
    runs = []
    with pcsx2():
        for rec in select('charsel.bin', lambda a, r: raster(r), 600):
            runs += T.clip_runs(rec)
        check(len(runs) >= 80, 'only %d clips' % len(runs))
        check(any(r['fan'] for r in runs), 'no fan clip')
        bad = sum(1 for r in runs if not all(T.check(r)[:2]))
        check(bad == 0, '%d of %d clips differ' % (bad, len(runs)))
        for old, new in (('    if vf27[axis] >> 31:\n', '    if False:\n'),
                         ('order = (0, 1, 2, 0) if vi[9] == vi[10] else (2, 1, 0, 2)', 'order = (0, 1, 2, 0)'),
                         ('            stq[2] = f_mul(ONE, q)', '            stq[2] = f_mul(stq[2], q)'),
                         ('            first, prev = scr, vf27', '            first, prev = scr, scr'),
                         ('            if cross_z >> 31:', '            if False:')):
            m = patched(SC, old, new)
            broke = sum(1 for r in runs if not all(T.check(r, m)[:2]))
            check(broke > 10, 'clip control %r broke only %d' % (new, broke))


# ---------------------------------------------------- billboard program ----

def test_billboard_conformance():
    """The control is a DIV one cycle slower: 2 of the 7 launches read Q at
    exactly 7 cycles."""
    pairs = select('rain.bin', lambda a, r: not raster(r), 20, pairs=True)
    check(len(pairs) >= 5, 'only %d billboard launches' % len(pairs))
    with pcsx2():
        check(conform(pairs) == 0, 'billboard launches differ from PCSX2')
        S.Q_LAT['div'] = 8
        try:
            broke = conform(pairs)
        finally:
            S.Q_LAT['div'] = 7
    check(broke >= 2, 'a slower DIV broke only %d billboard launches' % broke)


def test_spec_billboard():
    """Every VIBbRender packet in rain and character select.  The first
    control is the UV-table overlap: one rain splash (layout 4) reads the
    packet being built at 48.  Not testable here: billboards 0 and 1 loading
    uv3 after storing uv0-2 (no captured layout reads its own fresh slot),
    and the 128 alpha scale of an unblended layer (every batch blends: 64)."""
    import spec_billboard as SB
    import test_spec_billboard as T
    with pcsx2():
        runs = [T.kicks(r) for name in ('rain.bin', 'charsel.bin')
                for r in select(name, lambda a, r: a == 20 and not raster(r), 50)]
        check(len(runs) >= 5, 'only %d VIBbRender launches' % len(runs))
        check(all(T.check(r)[1] for r in runs), 'billboard packets differ')
        controls = (
            [("    def rd(a):\n", "    m0 = list(mem)\n\n    def rd0(a):\n        return list(m0[a * 4: a * 4 + 4])\n\n    def rd(a):\n"),
             ("uv = [rd(t), rd(t + 1), rd(t + 2)]", "uv = [rd0(t), rd0(t + 1), rd0(t + 2)]"),
             ("uv.append(rd(t + 3))", "uv.append(rd0(t + 3))"),
             ("uv = [rd(t + k) for k in range(4)]", "uv = [rd0(t + k) for k in range(4)]")],
            [("    if code:\n        h = [0, 0]", "    if False:\n        h = [0, 0]")],
            [("for q in (D, A, B, Cc)]", "for q in (D, B, A, Cc)]")],
            [("if sx16(len(recs) - 16 + sx16(frame - (frame & 255))) >= 0:", "if frame & 0xFF00:")],
            [("ftoi(f_mul(C[3], k[3]), 0)", "ftoi(f_mul(C[3], k[2]), 0)")],
            [("h = [f_sub(f_mul(s[c], qs), k[c]) for c in range(2)]", "h = [f_mul(s[c], qs) for c in range(2)]")])
        for eds in controls:
            m = patched(SB, eds)
            broke = 0
            for r in runs:
                try:
                    broke += not T.check(r, m)[1]
                except AssertionError:
                    broke += 1
            check(broke >= 1, 'billboard control %r broke nothing' % eds[-1][1])


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
