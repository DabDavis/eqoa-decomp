#!/usr/bin/env python3
"""eqd -- command line over eqdecomp/eqdiff.

  eqd.py list   <regex> [-m SUPPORT]        symbols matching, with size + VU0 mix
  eqd.py show   <symbol>                    disassemble the target function
  eqd.py extract <symbol> -o out.bin        raw bytes of the target function
  eqd.py census <regex>                     scalar / VU0-asm split over a set
  eqd.py diff   <symbol> --obj f.o [--as-symbol NAME]
  eqd.py diff   <symbol> --bin f.bin
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import eqdecomp as E
import eqdiff as D


def _load(a):
    return E.load(a.module)


def cmd_list(a):
    _, st, img = _load(a)
    hits = st.find(a.pattern)
    if not hits:
        print(f"no symbol matches {a.pattern!r}", file=sys.stderr)
        return 1
    print(f"{'symbol':<58}{'addr':>10}{'bytes':>7}{'insns':>7}{'VU0':>6}")
    for s in hits:
        if s.span <= 0 or not img.covers(s.addr, s.span):
            print(f"{s.name[:57]:<58}{s.addr:>#10x}{s.span:>7}{'?':>7}{'?':>6}")
            continue
        tot, vu = E.vu0_census(img.func(s))
        print(f"{s.name[:57]:<58}{s.addr:>#10x}{s.span:>7}{tot:>7}{vu:>6}")
    print(f"\n{len(hits)} symbols")
    return 0


def cmd_show(a):
    _, st, img = _load(a)
    s = st.get(a.symbol)
    data = img.func(s, trim=a.trim)
    modes = set(a.norm.split(",")) - {""} if a.norm else set()
    data, rep = E.normalise(data, modes)
    print(f"{s.name}  0x{s.addr:08X}  {len(data)} bytes  normalised {rep}")
    for addr, w, t in E.disasm(data, s.addr):
        print(f"  {addr:08X}  {w:08x}  {t}")
    return 0


def cmd_extract(a):
    _, st, img = _load(a)
    s = st.get(a.symbol)
    data = img.func(s, trim=a.trim)
    Path(a.out).write_bytes(data)
    print(f"{s.name}  0x{s.addr:08X}  {len(data)} bytes -> {a.out}")
    return 0


def cmd_census(a):
    _, st, img = _load(a)
    hits = [s for s in st.find(a.pattern)
            if s.span > 0 and img.covers(s.addr, s.span)]
    pure, mixed = [], []
    for s in hits:
        tot, vu = E.vu0_census(img.func(s))
        (mixed if vu else pure).append((s, tot, vu))
    def tally(rows):
        return len(rows), sum(s.span for s, _, _ in rows), sum(t for _, t, _ in rows)
    for label, rows in (("pure scalar (codegen test)", pure),
                        ("contains VU0 asm", mixed)):
        n, b, i = tally(rows)
        print(f"{label:<30} {n:>3} funcs {b:>6} bytes {i:>6} insns")
    n, b, i = tally(pure + mixed)
    print(f"{'TOTAL':<30} {n:>3} funcs {b:>6} bytes {i:>6} insns")
    if a.verbose:
        print("\npure scalar, smallest first:")
        for s, t, _ in sorted(pure, key=lambda r: r[0].span):
            print(f"  {s.span:>5}B {t:>4}i  {s.name}")
    return 0


def cmd_diff(a):
    _, st, img = _load(a)
    s = st.get(a.symbol)
    target = img.func(s, trim=a.trim)
    modes = set(a.norm.split(",")) - {""} if a.norm else set()

    relocs, span = None, None
    if a.obj:
        name = a.as_symbol or a.symbol
        cand, sec, at = D.elf_function_at(Path(a.obj), name)
        relocs = D.elf_relocations(Path(a.obj), sec)
        span = (at, at + len(cand))
        print(f"candidate: {a.obj} [{sec}] symbol {name}, "
              f"{len(relocs)} relocations in object")
        if a.no_reloc_mask:
            relocs = None
    else:
        cand = Path(a.bin).read_bytes()

    res = D.diff(target, cand, s.name, s.addr, modes, relocs=relocs, span=span)
    print(D.report(res))
    return 0 if res.ok else 1


def main(argv=None):
    p = argparse.ArgumentParser(prog="eqd", description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("-m", "--module", default="SUPPORT", choices=sorted(E.MODULES))
    sub = p.add_subparsers(dest="cmd", required=True)

    q = sub.add_parser("list"); q.add_argument("pattern"); q.set_defaults(fn=cmd_list)

    q = sub.add_parser("show"); q.add_argument("symbol")
    q.add_argument("--trim", action="store_true", help="drop trailing pad nops")
    q.add_argument("--norm", default="", help="j26,hi16lo16")
    q.set_defaults(fn=cmd_show)

    q = sub.add_parser("extract"); q.add_argument("symbol")
    q.add_argument("-o", "--out", required=True)
    q.add_argument("--trim", action="store_true")
    q.set_defaults(fn=cmd_extract)

    q = sub.add_parser("census"); q.add_argument("pattern")
    q.add_argument("-v", "--verbose", action="store_true")
    q.set_defaults(fn=cmd_census)

    q = sub.add_parser("diff"); q.add_argument("symbol")
    g = q.add_mutually_exclusive_group(required=True)
    g.add_argument("--obj", help="unlinked ELF object (preferred)")
    g.add_argument("--bin", help="raw bytes")
    q.add_argument("--as-symbol", help="symbol name inside the object, if different")
    q.add_argument("--trim", action="store_true")
    q.add_argument("--norm", default="j26", help="j26,hi16lo16 (default j26)")
    q.add_argument("--no-reloc-mask", action="store_true",
                   help="do not mask the fields the object's relocations name")
    q.set_defaults(fn=cmd_diff)

    a = p.parse_args(argv)
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
