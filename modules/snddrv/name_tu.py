#!/usr/bin/env python3
"""Name a TU's functions in symbols.txt by ORDER: the object's functions, in
.text order, onto the map's entries from a start address.  At -O0 a TU's
functions are emitted in source order and linked contiguously, so the n-th
function of the object is the n-th entry from where the TU starts.

    EQMODULE=SNDDRV ./modules/snddrv/name_tu.py bgm 0x000AAA30
"""
import fcntl, os, re, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent))
os.environ.setdefault("EQMODULE", "SNDDRV")
import eqcheck as C

tu, start = sys.argv[1], int(sys.argv[2], 16)
o = C.BUILD / (tu + ".o")
asm = C.BUILD / (tu + ".s")
C.BUILD.mkdir(exist_ok=True)
subprocess.run(C.compile_cmd(tu) + [str(C.source_path(tu)), "-o", str(asm)], check=True, cwd=C.BUILD)
subprocess.run(C.assemble_cmd(asm, o), check=True, cwd=C.BUILD)
out = subprocess.run(["mipsel-linux-gnu-objdump", "-t", str(o)], capture_output=True, text=True).stdout
funcs = sorted((int(m.group(1), 16), m.group(2)) for m in
               re.finditer(r"^([0-9a-f]+)\s+\S+\s+F\s+\.text\s+[0-9a-f]+\s+(\S+)$", out, re.M))
p = HERE / "symbols.txt"
lock = open(HERE / ".symbols.lock", "w")      # two lanes may rename at once
fcntl.flock(lock, fcntl.LOCK_EX)
lines = p.read_text().splitlines()
rows = [i for i, l in enumerate(lines) if l.startswith("0x") and "[" not in l]
idx = [i for i in rows if int(lines[i].split()[0], 16) >= start]
for (_, name), i in zip(funcs, idx):
    addr = lines[i].split()[0]
    print(f"{addr} {name}")
    lines[i] = f"{addr} {name}"
p.write_text("\n".join(lines) + "\n")
