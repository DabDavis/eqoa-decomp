#!/usr/bin/env python3
"""One-TU diff: build src/<tu> alone (as _tu.py does) and print the
instruction diff for every symbol matching a regex that is not byte-exact.
Builds nothing else, so it is safe beside another lane's _tu.py on another TU.

  EQMODULE=SNDDRV ./_tud.py driver SdCommand
"""
import re, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E, eqcheck as C

tu, rx = sys.argv[1], re.compile(sys.argv[2] if len(sys.argv) > 2 else ".")
o = C.BUILD / (tu + ".o"); asm = C.BUILD / (tu + ".s")
C.BUILD.mkdir(exist_ok=True)
for f in (o, asm):
    if f.exists(): f.unlink()
r = subprocess.run(C.compile_cmd(tu) + [str(C.source_path(tu)), "-o", str(asm)],
                   capture_output=True, text=True, cwd=C.BUILD)
if not asm.exists():
    sys.exit("compile failed\n" + r.stderr)
r = subprocess.run(C.assemble_cmd(asm, o), capture_output=True, text=True, cwd=C.BUILD)
if not o.exists():
    sys.exit("assemble failed\n" + r.stderr)
_, st, img = E.load()
for n in C.obj_functions(o):
    if not rx.search(n):
        continue
    k, w, t, res = C.score(st, img, o, n)
    print(f"== {n}: {k} {w}/{t}")
    if k != "match" and res is not None:
        print("\n".join(res.text))
