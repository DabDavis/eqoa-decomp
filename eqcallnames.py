#!/usr/bin/env python3
"""
eqcallnames -- name placeholder map rows off the build's own call sites.

A map row still called `func_<VA>` is often a function OUR build already calls by a
real name: a constructor of a container a class embeds, an empty inline the vtable
TU emits, a libgcc RTTI helper.  The relocated dump names every TARGET callee
(`eqcalls.py`); the candidate object names its own.  Where the target's callee at a
call site is a placeholder and ours at the same site is a real name, that name is a
proposal for the placeholder row.

    EQMODULE=CLIENT ./eqcallnames.py          # every TU in build/ (run eqcheck first)

Same-length functions are aligned by instruction offset; a drifted function only
when both sides make the same number of calls (k-th call to k-th call).  A
placeholder proposed two different names is printed as CONFLICT.  Nothing is
renamed: read each proposal against the target body (`eqd.py show`) and rename
with care.  Phase 62's polish pass named 14 VIArray/VIPool constructors, the
VIArray<VIItemStat> copy constructor and libgcc's `__rtti_si`/`__rtti_user` this way.
"""
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E
import eqdiff as D
import eqcheck as C
import eqcalls as K
import eqcallcheck as CC


def main():
    _, st, img = E.load()
    d = K.relocated(img)
    props = defaultdict(set)
    for src in C.sources():
        obj = C.BUILD / f"{src.stem}.o"
        if not obj.exists():
            continue
        syms = CC.obj_symbols(obj)
        for fn in C.obj_functions(obj):
            try:
                sym = st.get(fn)
            except KeyError:
                continue
            if sym is None or sym.span == 0:
                continue
            tc = [(va - sym.addr, name) for va, kind, name in K.calls(st, img, d, sym) if kind == "jal"]
            try:
                have = CC.candidate_calls(obj, fn, syms)
            except (KeyError, ValueError):
                continue
            data, _, _ = D.elf_function_at(obj, fn)
            same = len(data) == sym.span
            hv = sorted(have.items())
            for k, (off, tname) in enumerate(tc):
                if not ("+0x" in tname or tname.startswith("0x") or E.is_placeholder(tname)):
                    continue
                ours = None
                if same and off in have:
                    ours = have[off]
                elif not same and len(hv) == len(tc):
                    ours = hv[k][1]
                if ours and not E.is_placeholder(ours):
                    props[tname].add((ours, fn))
    for t in sorted(props):
        names = sorted({o for o, _ in props[t]})
        flag = "" if len(names) == 1 else "  CONFLICT"
        print(f"{t:28s} -> {', '.join(names)}{flag}   (via {', '.join(sorted({f for _, f in props[t]})[:3])})")
    print(f"\n{len(props)} placeholder callees with a proposed name")
    return 0


if __name__ == "__main__":
    sys.exit(main())
