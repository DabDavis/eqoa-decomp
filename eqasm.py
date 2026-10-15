#!/usr/bin/env python3
"""eqasm -- disassemble one symbol out of an unlinked object, side by side
with the target if it exists.  The everyday "what did my candidate actually
emit" view; `eqd.py diff` shows only the first difference in context."""
import argparse, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E, eqdiff as D

p = argparse.ArgumentParser()
p.add_argument("obj"); p.add_argument("symbol")
p.add_argument("-m", "--module", default=E.MODULE)
p.add_argument("--no-target", action="store_true")
a = p.parse_args()

cand, _ = D.elf_function(Path(a.obj), a.symbol)
cn = [t for _, _, t in E.disasm(cand, 0)]
cw = [w for _, w, _ in E.disasm(cand, 0)]
tn, tw = [], []
if not a.no_target:
    try:
        _, st, img = E.load(a.module)
        s = st.get(a.symbol)
        tgt = img.func(s, trim=True)
        tn = [t for _, _, t in E.disasm(tgt, s.addr)]
        tw = [w for _, w, _ in E.disasm(tgt, s.addr)]
    except KeyError:
        pass
for i in range(max(len(cn), len(tn))):
    t = tn[i] if i < len(tn) else ""
    c = cn[i] if i < len(cn) else ""
    same = i < len(tw) and i < len(cw) and tw[i] == cw[i]
    print(f"{i:>3} {'  ' if same else '<>'} {t:<34}| {c}")
