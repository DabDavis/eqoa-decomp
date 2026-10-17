#!/usr/bin/env python3
"""Hold spec_clip to the simulator over every clip in a vu1dump capture: the
polygon L1085/L0187 builds at 214 (checked at L1266's entry), and the vertex
list L1266 hands the layer emitter (checked at its return, L1347).

    ./test_spec_clip.py <vu1dump.bin> [--pcsx2]
"""
import collections
import sys

import spec_clip as SC
import vuconform as C
import vusim as S

HOOKS = {1085: False, 187: True}       # pc -> is a fan


def clip_runs(rec):
    """-> list of dicts: the snapshots of one clip, in order."""
    if C.program_of(rec)[0] != 'raster':
        return []
    st = rec.state()
    sim = S.Sim(st, rec.micro)
    sim.s_mac0, sim.s_clip0 = st.mac, st.clip
    runs, cur = [], {}
    orig = sim.step

    def snap():
        return [list(r) for r in sim.s.vf], list(sim.s.vi), list(sim.s.mem)

    def step(pc, low, up):
        if pc in HOOKS:
            cur.clear()
            cur['fan'] = HOOKS[pc]
            cur['hook'] = snap()
        elif pc == 1266 and 'hook' in cur:
            cur['entry'] = snap()
        elif pc == 1347 and 'entry' in cur:
            cur['exit'] = snap()
            runs.append(dict(cur))
            cur.clear()
        return orig(pc, low, up)
    sim.step = step
    sim.run(rec.addr if rec.addr != 0xFFFFFFFF else rec.tpc // 8)
    return runs


def read_poly(mem, base, n):
    out = []
    for k in range(n):
        a = (base + 5 * k) * 4
        out.append(([mem[a], mem[a + 1]], mem[a + 4: a + 20]))
    return out


def check(run, mod=SC):
    """-> (polygon ok, output ok, output vertices, vertices made)"""
    vf, vi, mem = run['hook']
    poly = mod.polygon(run['fan'], vf, vi, mem)
    evf, evi, emem = run['entry']
    got = read_poly(emem, 214, 4)
    poly_ok = [v.words() for v in poly] == got
    out = mod.clip(poly, evi[7], evf[27], emem)
    xvf, xvi, xmem = run['exit']
    n, base = xvi[11], xvi[12]
    want = [tuple(xmem[(base + 3 * k + j) * 4:(base + 3 * k + j) * 4 + 4] for j in range(3))
            for k in range(n)]
    have = [tuple(x) for x in out]
    return poly_ok, have == want, len(want), want, have


def main(argv):
    if '--pcsx2' in argv:
        S.MODE = 'pcsx2'
    tally = collections.Counter()
    shown = 0
    for k, rec in enumerate(S.load_records(argv[0])):
        for run in clip_runs(rec):
            pok, ook, n, want, have = check(run)
            tally['clips'] += 1
            tally['fan' if run['fan'] else 'strip'] += 1
            tally['polygon bad'] += not pok
            tally['output bad'] += not ook
            tally['culled whole'] += n == 0
            tally['vertices out'] += n
            if not ook and shown < 3:
                shown += 1
                print('launch #%d: %d vertices out, spec %d' % (k, n, len(have)))
                for j, (w, h) in enumerate(zip(want, have)):
                    if w != h:
                        print('  vertex %d\n   sim  %s\n   spec %s' % (j, [[hex(x) for x in q] for q in w],
                                                                     [[hex(x) for x in q] for q in h]))
                        break
    print(dict(tally))
    return 1 if tally['polygon bad'] or tally['output bad'] or not tally['clips'] else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
