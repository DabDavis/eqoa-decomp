#!/usr/bin/env python3
"""Quick single-TU score: compile src/<file>.cpp with the settled toolchain and
score one symbol.  eqcheck.py rebuilds the whole tree; this is for iterating."""
import subprocess, sys, re
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E, eqdiff as D
import eqcheck as C

def run(src: str, syms: list[str], show=False):
    o = C.BUILD / (src + ".o"); asm = C.BUILD / (src + ".s")
    for f in (o, asm):
        if f.exists(): f.unlink()
    r = subprocess.run(C.compile_cmd(src) + [str(C.SRC/(src+".cpp")), "-o", str(asm)],
                       capture_output=True, text=True, cwd=C.BUILD)
    if not asm.exists():
        print(r.stderr); return
    r = subprocess.run(["wine", str(C.ASSEMBLER), "-o", str(o), str(asm)], capture_output=True, text=True, cwd=C.BUILD)
    if not o.exists():
        print(r.stderr); return
    _, st, img = E.load()
    for s in syms:
        st_, w, t, res = C.score(st, img, o, s)
        print(f"  {st_:9s} {w:4d}/{t:<5d} {s}")
        if show and res is not None and not res.ok:
            print("\n".join(res.text))

if __name__ == "__main__":
    run(sys.argv[1], sys.argv[2].split(","), "-w" in sys.argv)
