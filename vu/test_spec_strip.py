#!/usr/bin/env python3
"""Hold spec_strip.strip() to the simulator over every strip run in a vu1dump
capture: the 142+ buffer at the loop's exit, vertex by vertex, and the path
each vertex took.

    ./test_spec_strip.py <vu1dump.bin> [--pcsx2] [--variant strip|strip2|strip3|fan|strip2lit|strip5|strip6]
"""
import collections
import sys

import spec_strip as SP
import vuconform as C
import vusim as S

# variant -> (entry pc, loop-exit pc, {first pc of a culled path: name}, pc of the drawn store)
VARIANTS = {
    'strip': (317, 392, {401: 'reject', 411: 'backface', 417: 'clip'}, 389),
    'strip2': (1354, 1416, {1425: 'reject', 1434: 'backface', 1440: 'clip'}, 1413),
    'strip3': (549, 633, {642: 'reject', 651: 'backface', 657: 'clip'}, 630),
    'fan': (26, 150, {159: 'reject', 169: 'backface', 175: 'clip'}, 147),
    'strip2lit': (435, 514, {523: 'reject', 532: 'backface', 538: 'clip'}, 511),
    'strip5': (807, 881, {890: 'reject', 907: 'backface', 913: 'clip'}, 878),
    'strip6': (924, 1000, {1009: 'reject', 1019: 'backface', 1026: 'clip'}, 996),
}


def snapshots(rec, variant='strip'):
    """Run a launch; -> (state at the variant's entry, memory at its loop exit,
    the per-vertex path the microcode took, the sim) or None if it never ran."""
    entry, exit_, culled, drawn_pc = VARIANTS[variant]
    if C.program_of(rec)[0] != 'raster':
        return None
    st = rec.state()
    sim = S.Sim(st, rec.micro)
    sim.s_mac0, sim.s_clip0 = st.mac, st.clip
    snap = {}
    paths = []
    orig = sim.step

    def step(pc, low, up):
        if pc == entry and 'entry' not in snap:
            snap['entry'] = ([list(r) for r in sim.s.vf], list(sim.s.vi), list(sim.s.mem))
        if pc == exit_ and 'entry' in snap and 'exit' not in snap:
            snap['exit'] = list(sim.s.mem)
        if 'entry' in snap and 'exit' not in snap:
            if pc in culled:
                paths.append(culled[pc])
            elif pc == drawn_pc:
                paths.append('draw')
        return orig(pc, low, up)
    sim.step = step
    sim.run(rec.addr if rec.addr != 0xFFFFFFFF else rec.tpc // 8)
    if 'entry' not in snap or 'exit' not in snap:
        return None
    return snap['entry'], snap['exit'], paths, sim


def check_run(got, variant, mod=SP):
    """-> (vertices, mismatched vertices, path differs, path counter, first diffs)."""
    (vf, vi, mem), out_mem, mpaths, sim = got
    u = {'vf%02d' % r: vf[r] for r in range(32)}
    top = sim.s.top
    n = mem[top * 4] & 0xFFFF
    vs = []
    stride = SP.STRIDE.get(variant, 4)
    for j in range(n):
        base = (top + 1 + stride * j) * 4
        vs.append(tuple(mem[base + 4 * c: base + 4 * c + 4] for c in range(stride)))
    prev = {'vf19': vf[19], 'vf20': vf[20], 'vi05': vi[5], 'vi06': vi[6]}
    out, drawn = mod.strip(u, vs, prev, variant, mem)
    bad, diffs = 0, []
    paths = collections.Counter()
    for j, (xyzf, stq, rgba, adc, path) in enumerate(out):
        paths[path] += 1
        a = (142 + 3 * j) * 4
        want = out_mem[a:a + 12]
        if xyzf + stq + rgba != want:
            bad += 1
            diffs.append((j, [hex(x) for x in xyzf + stq + rgba], [hex(x) for x in want]))
    return len(out), bad, [o[4] for o in out if o[4] != 'first'] != mpaths, paths, diffs


def main(argv):
    if '--pcsx2' in argv:
        S.MODE = 'pcsx2'
    only = argv[argv.index('--variant') + 1] if '--variant' in argv else None
    recs = S.load_records(argv[0])
    fail = 0
    for variant in VARIANTS:
        if only and variant != only:
            continue
        runs = verts = bad_v = bad_path = 0
        paths = collections.Counter()
        first = []
        for k, rec in enumerate(recs):
            got = snapshots(rec, variant)
            if got is None:
                continue
            runs += 1
            nv, bad, pdiff, p, diffs = check_run(got, variant)
            verts += nv
            bad_v += bad
            bad_path += pdiff
            paths.update(p)
            for d in diffs[:1]:
                if len(first) < 3:
                    first.append((k,) + d)
        print('%-7s runs %4d, vertices %6d, mismatched vertices %d, runs with a different path %d  %s'
              % (variant, runs, verts, bad_v, bad_path, dict(paths)))
        for b in first:
            print('   launch #%d vertex %d\n     spec %s\n     sim  %s' % b)
        fail |= bool(bad_v or bad_path or not runs)
    return 1 if fail else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
