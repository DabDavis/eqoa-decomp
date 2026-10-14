#!/usr/bin/env python3
"""One-TU regression check: build src/<tu>.cpp alone, score every function it
defines, and report any symbol in MATCHED.txt that this TU defines and no
longer matches.  The whole-tree `eqcheck.py` is the final word; this is the
check to run after each edit, in seconds instead of ten minutes.

  ./_tu.py viesfparse            # summary + regressions
  ./_tu.py viesfparse -a         # every row
  ./_tu.py viesfparse -f Parse   # only symbols matching a regex
"""
import re, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E, eqcheck as C

def main():
    tu = sys.argv[1]
    allrows = "-a" in sys.argv
    rx = re.compile(sys.argv[sys.argv.index("-f") + 1]) if "-f" in sys.argv else None
    o = C.BUILD / (tu + ".o"); asm = C.BUILD / (tu + ".s")
    C.BUILD.mkdir(exist_ok=True)
    # What the LAST build of this TU defined: a matched function that stops
    # being emitted (an unused template member is never instantiated) is lost
    # just as surely as one that stops matching, and the row loop cannot see it.
    before = set(C.obj_functions(o)) if o.exists() else set()
    for f in (o, asm):
        if f.exists(): f.unlink()
    r = subprocess.run(C.compile_cmd(tu) + [str(C.SRC / (tu + ".cpp")), "-o", str(asm)],
                       capture_output=True, text=True, cwd=C.BUILD)
    if not asm.exists():
        print("BUILD FAILED (compile)\n" + r.stderr); return 2
    for _ in range(2):
        r = subprocess.run(["wine", str(C.ASSEMBLER), "-o", str(o), str(asm)],
                           capture_output=True, text=True, cwd=C.BUILD)
        if o.exists(): break
    if not o.exists():
        print("BUILD FAILED (assemble)\n" + r.stderr); return 2
    _, st, img = E.load()
    prev = set(C.MATCHED.read_text().split()) if C.MATCHED.exists() else set()
    rows = []
    for n in C.obj_functions(o):
        if rx and not rx.search(n): continue
        k, w, t, res = C.score(st, img, o, n)
        rows.append((k, w, t, n))
    rows.sort(key=lambda x: (x[0] != "match", x[1], x[3]))
    ok = sum(1 for x in rows if x[0] == "match")
    for k, w, t, n in rows:
        if k == "match":
            if allrows: print(f"  MATCH   {t:>4}i  {n}")
        elif k == "no-target":
            print(f"  NO-TGT        {n}")
        else:
            print(f"  wrong {w:>4}/{t:<4} {n}")
    lost = [n for k, w, t, n in rows if k != "match" and n in prev]
    if not rx:
        gone = (before - set(C.obj_functions(o))) & prev
        lost += sorted(f"{n}   (no longer emitted)" for n in gone)
    print(f"{tu}: {ok}/{len(rows)} byte-exact")
    if lost:
        print("REGRESSION -- in MATCHED.txt and not exact now:")
        for n in lost: print("  " + n)
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
