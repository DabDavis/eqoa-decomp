#!/usr/bin/env python3
"""Permutation sweep over a contiguous run of source lines.

  ./_perm.py <file.cpp|h> <src-tu> <symbol> <first-line> <last-line> [--max N]

Permutes the given (1-based, inclusive) line range of <file>, rebuilds <src-tu>
and scores <symbol>, then restores the file and reports the ranking.  The image
is loaded once, which is what makes a 24-way sweep four minutes instead of
twenty.
"""
import itertools, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E, eqcheck as C

def build_and_score(st, img, tu, syms):
    o = C.BUILD / (tu + ".o"); asm = C.BUILD / (tu + ".s")
    for f in (o, asm):
        if f.exists(): f.unlink()
    r = subprocess.run(C.compile_cmd(tu) +
                       [str(C.SRC/(tu+".cpp")), "-o", str(asm)],
                       capture_output=True, text=True, cwd=C.BUILD)
    if not asm.exists():
        return None, r.stderr
    for _ in range(2):
        r = subprocess.run(["wine", str(C.ASSEMBLER), "-o", str(o), str(asm)],
                           capture_output=True, text=True, cwd=C.BUILD)
        if o.exists(): break
    if not o.exists():
        return None, r.stderr
    out = []
    for s in syms:
        out.append(C.score(st, img, o, s))
    return out, ""

def main():
    f, tu, symlist, a, b = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
    syms = symlist.split(",")
    limit = int(sys.argv[sys.argv.index("--max")+1]) if "--max" in sys.argv else 10**9
    p = C.SRC / f
    orig = p.read_text().splitlines(True)
    block = orig[a-1:b]
    _, st, img = E.load()
    best = []
    n = 0
    try:
        for perm in itertools.permutations(range(len(block))):
            if n >= limit: break
            n += 1
            p.write_text("".join(orig[:a-1] + [block[i] for i in perm] + orig[b:]))
            res, err = build_and_score(st, img, tu, syms)
            if res is None:
                print("BUILD FAIL", perm, err[:200]); continue
            tot = sum(w for _, w, _, _ in res)
            best.append((tot, perm, [(w,t) for _,w,t,_ in res]))
            if tot == 0:
                print("EXACT", perm); break
    finally:
        p.write_text("".join(orig))
    best.sort()
    for tot, perm, det in best[:10]:
        print(f"  {tot:5d}  {perm}  {det}")

main()
