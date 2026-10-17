#!/usr/bin/env python3
"""Hold spec_billboard.render() to the simulator: every packet VIBbRender
XGKICKs (pcs 190 and 200), in a vu1dump capture.

    ./test_spec_billboard.py <vu1dump.bin> [--pcsx2]
"""
import collections
import sys

import spec_billboard as SB
import vuconform as C
import vusim as S

KICKS = (190, 200)


def kicks(rec):
    """-> (registers at entry, memory at entry, [packet memory at each kick])
    for a VIBbRender launch, else None."""
    if C.program_of(rec)[0] != 'billboard' or rec.addr != 20:
        return None
    st = rec.state()
    sim = S.Sim(st, rec.micro)
    sim.s_mac0, sim.s_clip0 = st.mac, st.clip
    vf0, mem0 = [list(r) for r in st.vf], list(st.mem)
    out = []
    orig = sim.step

    def step(pc, low, up):
        if pc in KICKS:
            out.append((sim.s.vi[4], list(sim.s.mem)))
        return orig(pc, low, up)
    sim.step = step
    sim.run(20)
    return vf0, mem0, st.top, out


def check(got, mod=SB):
    vf, mem, top, ks = got
    n = mem[top * 4] & 0xFFFF
    q = lambda m, a: m[a * 4: a * 4 + 4]
    bbs = [[q(mem, top + 1 + 3 * j + i) for i in range(3)] for j in range(n)]
    packets = mod.render(vf, list(mem), bbs)
    want = []
    for base, m in ks:
        cnt = m[base * 4] & 0x7FFF
        want.append([q(m, base + k) for k in range(1 + 11 * cnt)])
    return n, packets == want, packets, want


def main(argv):
    if '--pcsx2' in argv:
        S.MODE = 'pcsx2'
    t = collections.Counter()
    shown = 0
    for k, rec in enumerate(S.load_records(argv[0])):
        got = kicks(rec)
        if got is None:
            continue
        n, ok, have, want = check(got)
        t['launches'] += 1
        t['billboards'] += n
        t['packets'] += len(want)
        t['bad'] += not ok
        if not ok and shown < 2:
            shown += 1
            print('launch #%d: %d packets, spec %d' % (k, len(want), len(have)))
            for pi, (h, w) in enumerate(zip(have, want)):
                for qi, (a, b) in enumerate(zip(h, w)):
                    if a != b:
                        print('  packet %d qword %d: spec %s sim %s' % (pi, qi, [hex(x) for x in a], [hex(x) for x in b]))
                        break
    print(dict(t))
    return 1 if t['bad'] or not t['launches'] else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
