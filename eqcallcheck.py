#!/usr/bin/env python3
"""
eqcallcheck -- does a byte-exact function call the RIGHT functions?

The scorer masks every field the candidate's own relocation table names, so a
`jal` matches any `jal`: a function can be byte-exact while calling something
the original never called.  Phase 61 found one -- `VICamera::Update` scored
exact while calling `VIMatrix44::operator=`, which does not exist in SUPPORT;
the target calls `sceVu0CopyMatrix`.

The relocated in-world dump names every target callee (`eqcalls.py`), and the
candidate object names its own (an R_MIPS_26 against a symbol, or against a
section with the offset in the instruction).  This lines the two up by
instruction offset and reports each disagreement.

    ./eqcallcheck.py                 # every TU in build/, matched functions only
    ./eqcallcheck.py -a              # open functions as well
    ./eqcallcheck.py viscene vizone  # those TUs only

Builds nothing: it reads build/*.o as the last eqcheck/_tu run left them.
A target callee the symbol map cannot name exactly (a TU-local function with
no map entry reads `Sym+0x40`) is counted as UNNAMED and never as a mismatch.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E
import eqdiff as D
import eqcheck as C
import eqcalls as K


def obj_symbols(obj: Path) -> dict[tuple[str, int], str]:
    """(section name, offset) -> symbol, for every FUNC/NOTYPE symbol in the object."""
    secs = {}
    out = subprocess.run([D._tool("readelf"), "-SW", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        m = re.match(r"^\s*\[\s*(\d+)\]\s+(\S+)", line)
        if m:
            secs[m.group(1)] = m.group(2)
    out = subprocess.run([D._tool("readelf"), "-sW", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    syms = {}
    for line in out.splitlines():
        m = re.match(r"^\s*\d+:\s+([0-9a-f]+)\s+\d+\s+(FUNC|NOTYPE)\s+\S+\s+\S+\s+(\S+)\s+(\S+)\s*$", line)
        if m and m.group(3) in secs:
            syms.setdefault((secs[m.group(3)], int(m.group(1), 16)), m.group(4))
    return syms


def candidate_calls(obj: Path, symbol: str, syms) -> dict[int, str]:
    """byte offset within the function -> callee name, for every R_MIPS_26."""
    data, sec, at = D.elf_function_at(obj, symbol)
    out = {}
    for off, typ, name in D.elf_relocations(obj, sec):
        if typ != "R_MIPS_26" or not (at <= off < at + len(data)):
            continue
        if name.startswith(".") or not name:
            # against a section: the in-place addend is the target offset
            w = int.from_bytes(data[off - at:off - at + 4], "little")
            name = syms.get((name or sec, (w & 0x03FFFFFF) << 2), f"{name}+?")
        out[off - at] = name
    return out


def main(argv):
    allfuncs = "-a" in argv
    tus = [a for a in argv if not a.startswith("-")]
    _, st, img = E.load()
    d = K.relocated(img)
    matched = set(C.MATCHED.read_text().split()) if C.MATCHED.exists() else set()
    objs = [C.BUILD / f"{t}.o" for t in tus] if tus else \
           [C.BUILD / f"{s.stem}.o" for s in sorted(C.SRC.glob("*.cpp"))]
    checked = unnamed = 0
    bad = []
    for obj in objs:
        if not obj.exists():
            continue
        syms = obj_symbols(obj)
        for fn in C.obj_functions(obj):
            if not allfuncs and fn not in matched:
                continue
            try:
                sym = st.get(fn)
            except KeyError:
                continue
            if sym is None or sym.span == 0:
                continue
            want = {va - sym.addr: name for va, kind, name in K.calls(st, img, d, sym)
                    if kind in ("jal", "j")}
            try:
                have = candidate_calls(obj, fn, syms)
            except (KeyError, ValueError):
                continue
            for off, tname in sorted(want.items()):
                if off not in have:
                    continue        # a `j` within the function, or the shapes differ
                checked += 1
                if "+0x" in tname or tname.startswith("0x") or E.is_placeholder(tname):
                    unnamed += 1
                    continue
                if have[off] != tname:
                    bad.append((obj.stem, fn, off, tname, have[off]))
    for tu, fn, off, t, c in bad:
        print(f"{tu:20s} {fn}\n    +{off:#06x}  target calls {t}\n             ours calls   {c}")
    print(f"\n{checked} call sites checked, {unnamed} with an unnamed target, "
          f"{len(bad)} MISMATCHED in {len({b[1] for b in bad})} functions")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
