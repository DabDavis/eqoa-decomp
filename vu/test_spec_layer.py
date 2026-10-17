#!/usr/bin/env python3
"""Hold spec_layer.layer() to the simulator: for every layer routine run in a
vu1dump capture, the GIF packet at 539 at the moment it is XGKICKed.

    ./test_spec_layer.py <vu1dump.bin> [--pcsx2]
"""
import collections
import sys

import spec_layer as SL
import vuconform as C
import vusim as S

# routine entry -> (pass type, pc of the XGKICK that sends its packet)
ROUTINES = {1523: (0, 1544), 1451: (1, 1493), 1494: (3, 1522), 1545: (2, 1615), 1616: (2, 1683)}


def layer_runs(rec):
    """-> list of (ptype, entry snapshot, packet memory at the kick)."""
    if C.program_of(rec)[0] != 'raster':
        return []
    st = rec.state()
    sim = S.Sim(st, rec.micro)
    sim.s_mac0, sim.s_clip0 = st.mac, st.clip
    runs, cur = [], {}
    orig = sim.step

    def step(pc, low, up):
        if pc in ROUTINES:
            cur['entry'] = (ROUTINES[pc][0], [list(r) for r in sim.s.vf], list(sim.s.vi),
                            list(sim.s.mem), ROUTINES[pc][1])
        if 'entry' in cur and pc == cur['entry'][4]:
            runs.append((cur.pop('entry'), list(sim.s.mem)))
        return orig(pc, low, up)
    sim.step = step
    sim.run(rec.addr if rec.addr != 0xFFFFFFFF else rec.tpc // 8)
    return runs


def check(entry_snap, kick_mem, mod=SL):
    ptype, vf, vi, mem, _ = entry_snap
    q = lambda a: mem[a * 4: a * 4 + 4]
    base, bufp, n, tagoff = vi[5], vi[11], vi[6], vi[3]
    entry = [q(base + k) for k in range(17)]
    buf = [(q(bufp + 3 * j), q(bufp + 3 * j + 1), q(bufp + 3 * j + 2)) for j in range(n)]
    out = mod.layer(ptype, entry, buf, n, vf[22], tagoff)
    want = [kick_mem[(539 + k) * 4:(539 + k) * 4 + 4] for k in range(len(out))]
    bad = [k for k in range(len(out)) if out[k] != want[k]]
    return len(out), bad, out, want


def main(argv):
    if '--pcsx2' in argv:
        S.MODE = 'pcsx2'
    recs = S.load_records(argv[0])
    tally = collections.Counter()
    qw = badq = 0
    shown = 0
    for k, rec in enumerate(recs):
        for snap, kmem in layer_runs(rec):
            nq, bad, out, want = check(snap, kmem)
            tally[(snap[0], 'ok' if not bad else 'DIFF')] += 1
            qw += nq
            badq += len(bad)
            if bad and shown < 4:
                shown += 1
                j = bad[0]
                print('launch #%d type %d qword %d: spec %s sim %s'
                      % (k, snap[0], j, [hex(x) for x in out[j]], [hex(x) for x in want[j]]))
    for key in sorted(tally):
        print('type %d %-4s %d' % (key[0], key[1], tally[key]))
    print('packet qwords %d, mismatched %d' % (qw, badq))
    return 1 if badq or not qw else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
