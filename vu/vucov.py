#!/usr/bin/env python3
"""Which parts of the raster microprogram a capture actually executes.

    ./vucov.py <vu1dump.bin> [...] [--pcsx2] [--pcs]

Runs every launch through vusim and reports, per region of src/raster.vsm,
how many launches entered it and how many of its pairs ran.  This is what
says whether a capture can verify a routine at all: a region with no
launches cannot be read against this data.  --pcs prints the pairs of each
region that never ran.
"""
import collections
import sys

import vuconform as C
import vusim as S

# [start, end) in instruction pairs, from the header of src/raster.vsm
REGIONS = [
    ('VIRInit', 0, 22), ('VIRMaterial', 22, 26), ('VIRFan', 26, 317),
    ('VIRStrip', 317, 429), ('VIRStrip2', 429, 549), ('VIRStrip3', 549, 668),
    ('VIRStrip4', 668, 807), ('VIRStrip5', 807, 924), ('VIRStrip6', 924, 1037),
    ('layer_emitter', 1037, 1085), ('clip_strip', 1085, 1266),
    ('clip_polygon', 1266, 1354), ('strip2_unlit', 1354, 1451),
    ('layer_tex_wrap', 1451, 1494), ('layer_tex_clamp', 1494, 1523),
    ('layer_solid', 1523, 1545), ('layer_wire', 1545, 1684),
    ('clip_swap..dead', 1684, 2029),
]


def region(pc):
    for name, a, b in REGIONS:
        if a <= pc < b:
            return name
    return '?'


def main(argv):
    paths = [a for a in argv if not a.startswith('--')]
    if '--pcsx2' in argv:
        S.MODE = 'pcsx2'
    ran = collections.Counter()           # pc -> launches that ran it
    launches = collections.Counter()      # region -> launches entering it
    bbran = set()                         # billboard pairs run
    step = S.Sim.step

    for path in paths:
        recs = S.load_records(path)
        for rec in recs:
            seen = set()
            prog = C.program_of(rec)[0]

            def traced(self, pc, low, up, _seen=seen):
                _seen.add(pc)
                return step(self, pc, low, up)
            S.Sim.step = traced
            try:
                S.run_record(rec)
            except Exception as e:
                print('%s: launch at %d: %s' % (path, rec.addr, e))
            finally:
                S.Sim.step = step
            if prog != 'raster':
                launches[prog] += 1
                bbran.update(seen)
                continue
            for pc in seen:
                ran[pc] += 1
            for name in {region(pc) for pc in seen}:
                launches[name] += 1
        print('%s: %d launches' % (path, len(recs)))

    for name, a, b in REGIONS:
        hit = sum(1 for pc in range(a, b) if ran[pc])
        print('%-20s %5d-%-5d launches %6d  pairs run %4d of %4d'
              % (name, a, b, launches[name], hit, b - a))
        if '--pcs' in argv and hit < b - a:
            miss = [pc for pc in range(a, b) if not ran[pc]]
            print('    not run:', ' '.join(map(str, miss)))
    print('%-20s %5d-%-5d launches %6d  pairs run %4d of %4d'
          % ('billboard', 0, 205, launches['billboard'], len(bbran), 205))
    if '--pcs' in argv and len(bbran) < 205:
        print('    not run:', ' '.join(str(pc) for pc in range(205) if pc not in bbran))


if __name__ == '__main__':
    main(sys.argv[1:])
