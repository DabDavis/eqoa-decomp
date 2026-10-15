#!/usr/bin/env python3
"""
eqtmpl -- find a module's own copies of SUPPORT's container templates, and name them.

gcc 2.95 emitted a template instantiation in EVERY translation unit that used it: there
is no COMDAT on this toolchain, so the linker kept every copy and resolved each call to
one of them.  CLIENT therefore carries its own VIMap/VIArray/VIPool/... members, most of
them many times over (one VIMap<int, 4-byte> has 97 identical copies of Find, Insert and
the rest, every one calling the same Begin/End/Next).  The definitions are SUPPORT's own,
shared through src/<x>_t.h, so a copy compiles byte-exact once its type arguments are
right.  This tool finds those arguments and names the members:

  ./eqtmpl.py solve 'int, VIBuddy' Find=0x00BE1F68 [Insert=0x...] [--decls 'C++']
  ./eqtmpl.py scan  OUT.json          # every live VIMap instantiation, value types solved

`solve` takes one or more seeds (member=address) of an instantiation whose types are
known; `scan` seeds itself from every REFERENCED function shaped like a VIMap member that
carries the node stride, and tries placeholder value types (VIRec<size>_<seed>) whose
size and alignment give that stride.  Either way, the walk is exact:
  - the candidate class is compiled whole (`template class VIMap<K, V>;`);
  - a member's callees are named off the candidate's OWN relocations -- a call to X at
    offset o means the target's jal at o lands on X -- so no name is ever guessed;
  - a function is accepted only byte-exact (relocation fields masked) with every one of
    its calls landing where the callee was assigned; one level of CALLERS (a destructor,
    Init, Clear: only user code calls them) is accepted on the same two tests, which is
    what rejects another instantiation's identical-looking destructor;
  - an address is never assigned twice, and the dead copies are never reached, because
    nothing the walk follows calls them.
Only VIMap is solved so far.  Value types with a non-trivial copy (an out-of-line Node
copy constructor, as the 472-byte ability map has) need the real class.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

os.environ.setdefault("EQMODULE", "CLIENT")
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E  # noqa: E402
import eqdiff as D    # noqa: E402
import eqcheck as C   # noqa: E402

STRIDE_MEMBERS = {"Find", "Begin", "Next", "DestroyPool", "LinkFreeChain", "Allocate",
                  "Insert", "AddNode", "Erase", "EraseFixup"}


class Module:
    def __init__(self):
        self.mod, self.st, self.img = E.load()
        self.rel = E.relocate(self.img.module, self.img.data)
        self.callers, self.refs = {}, Counter()
        base = self.mod.base
        for site, typ, imp, tgt in E.module_relocations(self.img.data):
            if imp:
                continue
            if typ in (E.R_WORD, E.R_JAL):
                self.refs[base + tgt] += 1
            if typ == E.R_JAL:
                f = self.st.at(base + site)
                if f:
                    self.callers.setdefault(base + tgt, set()).add(f.addr)

    def jal_target(self, addr: int) -> int | None:
        w = int.from_bytes(self.rel[addr:addr + 4], "little")
        return (w & 0x03FFFFFF) << 2 if (w >> 26) == 3 else None


def compile_class(work: Path, targs: str, decls: str = "") -> dict | None:
    """{member: (bytes, section, offset, relocations)} for `template class VIMap<targs>;`."""
    work.mkdir(parents=True, exist_ok=True)
    src = work / "inst.cpp"
    src.write_text(f'#include "game.h"\n#include "vimap_t.h"\n{decls}\ntemplate class VIMap<{targs}>;\n')
    r = subprocess.run(C.compile_cmd("inst") + ["-I" + str(C.SRC), str(src), "-o", str(work / "inst.s")],
                       capture_output=True, text=True, cwd=work)
    if r.returncode:
        return None
    subprocess.run(["wine", str(C.ASSEMBLER), "-o", str(work / "inst.o"), str(work / "inst.s")],
                   capture_output=True, cwd=work)
    obj = work / "inst.o"
    out = subprocess.run([D._tool("readelf"), "-sW", str(obj)], capture_output=True, text=True).stdout
    cand = {}
    for line in out.splitlines():
        if " FUNC " in line:
            name = line.split()[-1]
            b, sec, at = D.elf_function_at(obj, name)
            rels = [(o - at, t, n) for o, t, n in D.elf_relocations(obj, sec) if at <= o < at + len(b)]
            cand[name] = (b, sec, at, rels)
    return cand


def walk(m: Module, cand: dict, seeds: list[tuple[int, str]], taken: set[int]) -> dict[int, str]:
    """The exact walk described in the module docstring."""
    st, img = m.st, m.img

    def bytes_match(a: int, name: str) -> bool:
        s = st.at(a)
        if not s or s.addr != a or a in taken:
            return False
        tgt = img.func(s, trim=True)
        b, sec, at, rels = cand[name]
        if len(b) != len(tgt):
            return False
        return D.diff(tgt, b, name, a, modes={"j26"}, relocs=[(o + at, t, n) for o, t, n in rels],
                      span=(at, at + len(b))).ok

    def calls(a: int, name: str) -> list[tuple[str, int | None]]:
        return [(n, m.jal_target(a + o)) for o, t, n in cand[name][3] if t == "R_MIPS_26" and n in cand]

    assigned, by_name, todo = {}, {}, list(seeds)

    def down():
        while todo:
            a, name = todo.pop()
            if a in assigned or name in by_name or not bytes_match(a, name):
                continue
            assigned[a] = name
            by_name[name] = a
            todo.extend((t, n) for n, t in calls(a, name) if t)

    down()
    grew = True
    while grew:
        grew = False
        for a in sorted({c for x in list(assigned) for c in m.callers.get(x, ())} - set(assigned)):
            s = st.at(a)
            # a placeholder, or a function already named as one of this class's members
            if not s or s.span > 1024 or not (E.is_placeholder(s.name) or s.name in cand):
                continue
            for name in cand:
                if name in by_name or not bytes_match(a, name):
                    continue
                cs = calls(a, name)
                anchored = any(by_name.get(n) == t for n, t in cs)
                rest = [(n, t) for n, t in cs if n not in by_name]
                if (cs and anchored and all(by_name.get(n) == t for n, t in cs if n in by_name)
                        and all(t and bytes_match(t, n) for n, t in rest)):
                    todo.append((a, name))
                    todo.extend((t, n) for n, t in rest)
                    down()
                    grew = True
                    break
    return assigned


def _shape(img, addr: int, span: int) -> tuple:
    out = [re.sub(r"0x[0-9a-f]+|-?\b\d+\b", "#", t) for _, _, t in E.disasm(img.read(addr, span), addr)]
    while out and out[-1] == "nop":
        out.pop()
    return tuple(out)


def _stride(img, addr: int, span: int) -> int | None:
    li = {}
    for _, _, t in E.disasm(img.read(addr, span), addr):
        mm = re.match(r"li\s+(\w+),(-?\d+|0x[0-9a-f]+)$", t)
        if mm:
            li[mm.group(1)] = int(mm.group(2), 0)
        mm = re.match(r"mult\s+\w+,(\w+),(\w+)$", t)
        if mm:
            for r in (mm.group(2), mm.group(1)):
                if r in li:
                    return li[r]
    return None


def _candidates(S: int) -> list[tuple[int, int]]:
    """(alignment, size) of a value type whose VIMap node (4-byte key) is S bytes."""
    out = []
    if (S - 20) % 4 == 0:
        out.append((4, S - 20))
    if (S - 24) % 8 == 0 and S - 24 >= 8:
        out.append((8, S - 24))
    out += [(1, n) for n in (S - 20, S - 21, S - 22, S - 23) if n > 0]
    if (S - 32) % 16 == 0 and S - 32 >= 16:
        out.append((16, S - 32))
    return out


def _vdecl(name: str, align: int, size: int) -> str:
    elem, unit = {1: ("char", 1), 2: ("short", 2), 4: ("int", 4), 8: ("long", 8), 16: ("int", 4)}[align]
    attr = " __attribute__((aligned(16)))" if align == 16 else ""
    return f"struct {name} {{ {elem} m_data[{size // unit}]; }}{attr};"


def cmd_solve(args) -> int:
    m = Module()
    work = Path(args.work)
    cand = compile_class(work, args.targs, args.decls)
    if cand is None:
        raise SystemExit("the candidate class did not compile")
    seeds = []
    for s in args.seeds:
        member, addr = s.split("=")
        names = [n for n in cand if n.split("__")[0] == member or (member == "dtor" and n.startswith("_$_"))]
        if len(names) != 1:
            raise SystemExit(f"no unique compiled member {member!r}")
        seeds.append((int(addr, 16), names[0]))
    got = walk(m, cand, seeds, set())
    for a in sorted(got):
        print(f"{a:#010x}  {m.st.at(a).name:45s} -> {got[a]}")
    print(f"{len(got)} members of {len(cand)} compiled")
    if args.json:
        Path(args.json).write_text(json.dumps({hex(a): n for a, n in got.items()}, indent=1))
    return 0


def cmd_scan(args) -> int:
    m = Module()
    smod, sst, simg = E.load("SUPPORT")
    matched = {line.split()[0] for line in open(HERE / "src" / "MATCHED.txt")
               if line.strip() and not line.startswith("#")}
    ref = {}
    for s in sst.sized:
        mm = re.match(r"^(\w+?)__C?t5VIMap2Z", s.name)
        if mm and mm.group(1) in STRIDE_MEMBERS and s.name in matched:
            ref[_shape(simg, s.addr, s.span)] = mm.group(1)
    seeds = []
    for s in m.st.sized:
        if 8 <= s.span <= 4096 and m.refs[s.addr]:
            lab = ref.get(_shape(m.img, s.addr, s.span))
            if lab:
                S = _stride(m.img, s.addr, s.span)
                if S and S >= 24:
                    seeds.append((s.addr, lab, S))
    taken = {int(line.split()[0], 16) for line in open(C.SRC.parent / "symbols.txt")
             if line.startswith("0x") and "VIMap" in line.split()[2]}
    work, results = Path(args.work), []
    for addr, lab, S in sorted(seeds):
        if addr in taken:
            continue
        best = None
        for align, size in _candidates(S):
            for K in ("int", "unsigned int"):
                vname = f"VIRec{size}_{addr & 0xFFFFFF:06X}"
                cand = compile_class(work / f"{addr:08X}_{align}_{size}_{K[0]}", f"{K}, {vname}",
                                     _vdecl(vname, align, size))
                if not cand:
                    continue
                names = [n for n in cand if n.startswith(lab + "__")]
                if len(names) != 1:
                    continue
                got = walk(m, cand, [(addr, names[0])], taken)
                if got and (best is None or len(got) > len(best[3])):
                    best = (K, align, size, got, vname)
            if best and len(best[3]) >= 8:
                break
        if not best:
            print(f"{addr:#010x} {lab:13s} S={S}: no solution")
            continue
        K, align, size, got, vname = best
        taken |= set(got)
        results.append({"seed": hex(addr), "K": K, "align": align, "size": size, "vname": vname,
                        "assigned": {hex(a): n for a, n in got.items()}})
        print(f"{addr:#010x} {lab:13s} S={S}: VIMap<{K}, {vname}> -> {len(got)} members")
        Path(args.out).write_text(json.dumps(results, indent=1))
    print(f"{len(results)} instantiations, {sum(len(r['assigned']) for r in results)} functions")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("solve", help="name every member of one instantiation from seeds")
    s.add_argument("targs", help="the template arguments, e.g. 'int, VIBuddy'")
    s.add_argument("seeds", nargs="+", help="member=address, e.g. Find=0x00BE1F68")
    s.add_argument("--decls", default="", help="C++ declarations the arguments need")
    s.add_argument("--work", default=str(HERE / "build" / "eqtmpl"))
    s.add_argument("--json")
    s.set_defaults(fn=cmd_solve)
    c = sub.add_parser("scan", help="find and solve every live VIMap instantiation")
    c.add_argument("out")
    c.add_argument("--work", default=str(HERE / "build" / "eqtmpl"))
    c.set_defaults(fn=cmd_scan)
    args = p.parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
