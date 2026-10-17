#!/usr/bin/env python3
"""Run every captured VU1 launch through vusim and compare its end state with
the NEXT launch's start state, which PCSX2 captured after VU1 finished:
registers are untouched by VIF in between, so they compare exactly.

    ./vuconform.py <vu1dump.bin> [--first N] [--show K] [--entry PC] [--pcsx2]

--pcsx2 runs the simulator with x86 NaN semantics, which is what the capture
was made with; the default is the console's (no NaN, no infinity).
"""
import collections
import sys

import vudis as D
import vusim as S

NAMES = {0: 'VIRInit', 22: 'VIRMaterial', 26: 'VIRFan', 317: 'VIRStrip', 429: 'VIRStrip2',
         549: 'VIRStrip3', 668: 'VIRStrip4', 807: 'VIRStrip5', 924: 'VIRStrip6'}
_PROGRAMS = None


def program_of(rec):
    """'raster' or 'billboard': which upload micro memory holds at this launch.
    The billboard program overwrites the raster's first 205 pairs, so an entry
    index alone does not say which program ran (0 is VIRInit AND VIBbInit)."""
    global _PROGRAMS
    if _PROGRAMS is None:
        data, syms = D.image(), D.load_symbols()
        _PROGRAMS = []
        for name, dma, prefix in D.PROGRAMS:
            _, items, _ = D.split_blob(data, syms[dma] - D.IMAGE_BASE)
            pairs = {}
            for it in items:
                if it[0] == 'mpg':
                    for k, pr in enumerate(it[3]):
                        pairs[it[1] + k] = pr
            _PROGRAMS.append((name, pairs, D.entry_labels(syms, prefix, dma)))
    for name, pairs, labels in _PROGRAMS:
        if all(tuple(rec.micro[k]) == pr for k, pr in pairs.items()):
            return name, labels
    return '?', {}


def entry_name(rec):
    prog, labels = program_of(rec)
    if prog == 'raster':
        return NAMES.get(rec.addr, rec.addr)
    return labels.get(rec.addr, '%s:%d' % (prog, rec.addr))


def compare(sim, nxt):
    s = sim.s
    bad = []
    for r in range(1, 32):
        for c in range(4):
            if s.vf[r][c] != nxt.vf[r][c]:
                bad.append(('vf%02d%s' % (r, 'xyzw'[c]), s.vf[r][c], nxt.vf[r][c]))
    for r in range(1, 16):
        if s.vi[r] != nxt.vi[r] & 0xFFFF:
            bad.append(('vi%02d' % r, s.vi[r], nxt.vi[r] & 0xFFFF))
    for c in range(4):
        if s.acc[c] != nxt.acc[c]:
            bad.append(('acc' + 'xyzw'[c], s.acc[c], nxt.acc[c]))
    for nm, mine, theirs in (('Q', s.q, nxt.vi[S.REG_Q]), ('P', s.p, nxt.vi[S.REG_P])):
        if mine != theirs:
            bad.append((nm, mine, theirs))
    return bad


def main(argv):
    path = argv[0]
    first = int(argv[argv.index('--first') + 1]) if '--first' in argv else None
    show = int(argv[argv.index('--show') + 1]) if '--show' in argv else 5
    entry = int(argv[argv.index('--entry') + 1]) if '--entry' in argv else None
    if '--pcsx2' in argv:
        S.MODE = 'pcsx2'
    recs = S.load_records(path)
    if first:
        recs = recs[:first + 1]
    tally = collections.Counter()
    fields = collections.Counter()
    shown = 0
    for k in range(len(recs) - 1):
        rec, nxt = recs[k], recs[k + 1]
        name = entry_name(rec)
        if entry is not None and rec.addr != entry:
            continue
        try:
            sim = S.run_record(rec)
        except Exception as e:
            tally[(name, 'CRASH')] += 1
            if shown < show:
                print('#%d %s CRASH %s' % (k, name, e)); shown += 1
            continue
        bad = compare(sim, nxt)
        tally[(name, 'ok' if not bad else 'DIFF')] += 1
        for b in bad:
            fields[(name, b[0])] += 1
        if bad and shown < show:
            shown += 1
            print('#%d %s: %d diffs, e.g. %s' % (k, name, len(bad),
                  ', '.join('%s %08x!=%08x' % b for b in bad[:6])))
    for key in sorted(tally, key=str):
        print('%-12s %-5s %d' % (key[0], key[1], tally[key]))
    if fields:
        print('most frequent differing fields:', fields.most_common(15))


if __name__ == '__main__':
    main(sys.argv[1:])
