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

  ./eqtmpl.py solve 'int, VIBuddy' Find=0x00BE1F68 [Insert=0x...] [--family VIMap] [--decls 'C++']
  ./eqtmpl.py scan OUT.json [--family VIArray]   # every live instantiation, types solved
  ./eqtmpl.py resolve A.json B.json ...           # settle addresses two families both claimed
  ./eqtmpl.py emit A.resolved.json TU.cpp --family F   # write the TU, rename the map
  ./eqtmpl.py name-implicit build/*.o             # name members pulled in implicitly

`solve` takes seeds (member=address) of an instantiation whose types are known.  `scan`
seeds itself from every REFERENCED function shaped like a member of the family (SUPPORT's
matched instantiations are the shapes), and solves placeholder types: for each seed, the
immediates in its own code give candidate element sizes; every (size, alignment) is
compiled in one batch, the candidates that reproduce the seed are walked, and the one that
names the most members wins.  Placeholders are VIRec<size>_<seed> (a struct of the right
size and alignment) or VIPtr_<seed> * (a pointer).  Either way, the walk is exact:
  - the candidate class is compiled whole (`template class F<args>;`);
  - a member's callees are named off the candidate's OWN relocations -- a call to X at
    offset o means the target's jal at o lands on X -- so no name is ever guessed;
  - a function is accepted only byte-exact (relocation fields masked) with every one of
    its calls landing where the callee was assigned; one level of CALLERS (a destructor,
    Init, Clear: only user code calls them) is accepted on the same two tests, which is
    what rejects another instantiation's identical-looking destructor;
  - an address is never assigned twice, and the dead copies are never reached, because
    nothing the walk follows calls them (eqcheck counts them apart, as duplicates).
A type whose copy or destruction is not trivial (an out-of-line Node copy constructor,
as the 472-byte ability VIMap has) is not reproduced by a placeholder: it needs the real
class.
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

# family: (header, mangled prefix of the class, template arity, node overhead per element)
FAMILIES = {
    "VIMap":      ("vimap_t.h",      "t5VIMap2Z",       2, 20),
    "VIMultiMap": ("vimultimap_t.h", "t10VIMultiMap2Z", 2, 20),
    "VISet":      ("viset_t.h",      "t5VISet1Z",       1, 16),
    "VIArray":    ("viarray_t.h",    "t7VIArray1Z",     1, 0),
    "VIVector":   ("vivector_t.h",   "t8VIVector1Z",    1, 0),
    "VIPool":     ("vipool_t.h",     "t6VIPool1Z",      1, 8),
    "VIList":     ("vilist_t.h",     "t6VIList1Z",      1, 8),
}


def member_of(name: str, prefix: str) -> str | None:
    """`Find` for `Find__Ct5VIMap2ZiZi...`, 'dtor'/'ctor' for the special members."""
    if name.startswith("_$_" + prefix):
        return "dtor"
    if name.startswith("__" + prefix):
        return "ctor"
    m = re.match(r"^(\w+?)__C?" + re.escape(prefix), name)
    return m.group(1) if m else None


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


def compile_classes(work: Path, family: str, instances: list[str], decls: str = "") -> dict | None:
    """{member: (bytes, section, offset, relocations)} for `template class F<a>;` per a."""
    work.mkdir(parents=True, exist_ok=True)
    hdr = FAMILIES[family][0]
    body = "".join(f"template class {family}<{a}>;\n" for a in instances)
    src = work / "inst.cpp"
    src.write_text(f'#include "game.h"\n#include "{hdr}"\n{decls}\n{body}')
    r = subprocess.run(C.compile_cmd("inst") + ["-I" + str(C.SRC), str(src), "-o", str(work / "inst.s")],
                       capture_output=True, text=True, cwd=work)
    if r.returncode:
        return None
    subprocess.run(["wine", str(C.ASSEMBLER), "-o", str(work / "inst.o"), str(work / "inst.s")],
                   capture_output=True, cwd=work)
    obj = work / "inst.o"
    syms = subprocess.run([D._tool("readelf"), "-sW", str(obj)], capture_output=True, text=True).stdout
    relocs = D.elf_relocations(obj)
    secs = {}
    out = subprocess.run([D._tool("readelf"), "-SW", str(obj)], capture_output=True, text=True).stdout
    for line in out.splitlines():
        m = re.match(r"\s*\[\s*(\d+)\]\s+(\S+)", line)
        if m:
            secs[int(m.group(1))] = m.group(2)
    data = obj.read_bytes()
    shdr = {}
    import struct
    e_shoff, = struct.unpack_from("<I", data, 0x20)
    e_shentsize, e_shnum = struct.unpack_from("<HH", data, 0x2E)
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        _, _, _, _, sh_offset, sh_size = struct.unpack_from("<IIIIII", data, off)
        shdr[i] = (sh_offset, sh_size)
    rel_by_sec = {}
    out = subprocess.run([D._tool("readelf"), "-rW", str(obj)], capture_output=True, text=True).stdout
    cur = None
    for line in out.splitlines():
        h = re.match(r"^Relocation section '\.rel(\S+)'", line)
        if h:
            cur = h.group(1)
            continue
        m = re.match(r"^([0-9a-f]{8,16})\s+[0-9a-f]+\s+(\S+)\s+[0-9a-f]*\s*(\S*)", line)
        if m and cur and m.group(2).startswith("R_MIPS"):
            rel_by_sec.setdefault(cur, []).append((int(m.group(1), 16), m.group(2), m.group(3)))
    cand = {}
    for line in syms.splitlines():
        parts = line.split()
        if len(parts) >= 8 and parts[3] == "FUNC" and parts[6].isdigit():
            value, size, ndx, name = int(parts[1], 16), int(parts[2]), int(parts[6]), parts[7]
            sec = secs.get(ndx)
            if sec is None or size == 0:
                continue
            off, _ = shdr[ndx]
            b = data[off + value: off + value + size]
            rels = [(o - value, t, n) for o, t, n in rel_by_sec.get(sec, []) if value <= o < value + size]
            cand[name] = (b, sec, value, rels)
    del relocs
    return cand


def walk(m: Module, cand: dict, seeds: list[tuple[int, str]], taken) -> dict[int, str]:
    """The exact walk described in the module docstring.  TAKEN holds addresses already
    named by an earlier walk ({addr: name}, or a set): they are never re-assigned, and a
    member whose call lands on one under a DIFFERENT name is rejected."""
    taken_names = taken if isinstance(taken, dict) else {}
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

    def consistent(a: int, name: str) -> bool:
        for n, t in calls(a, name):
            if t in taken_names and taken_names[t] != n:
                return False
            if t in assigned and assigned[t] != n:
                return False
        return True

    def down():
        while todo:
            a, name = todo.pop()
            if a in assigned or name in by_name or not bytes_match(a, name) or not consistent(a, name):
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
                if name in by_name or not bytes_match(a, name) or not consistent(a, name):
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


# --- placeholder types --------------------------------------------------------------

def _vdecl(name: str, align: int, size: int) -> str:
    elem, unit = {1: ("char", 1), 2: ("short", 2), 4: ("int", 4), 8: ("long", 8), 16: ("int", 4)}[align]
    attr = " __attribute__((aligned(16)))" if align == 16 else ""
    return f"struct {name} {{ {elem} m_data[{size // unit}]; }}{attr};"


def _immediates(img, addr: int, span: int) -> set[int]:
    out = set()
    for _, _, t in E.disasm(img.read(addr, span), addr):
        if re.match(r"(sll|sra|srl)\s", t):
            mm = re.search(r",(0x[0-9a-f]+|\d+)$", t)
            if mm and 1 <= int(mm.group(1), 0) <= 12:
                out.add(1 << int(mm.group(1), 0))
            continue
        for v in re.findall(r"(?<![\w(])(0x[0-9a-f]+|\d+)(?![\w(])", t):
            v = int(v, 0)
            if 1 <= v <= 4096:
                out.add(v)
    return out


def _sizes(family: str, imms: set[int]) -> list[tuple[int, int]]:
    """(alignment, size) candidates for a one-argument family's element type."""
    over = FAMILIES[family][3]
    sizes = set()
    for v in imms:
        for d in (0, over, over + 4, over + 8):
            if 1 <= v - d <= 4096:
                sizes.add(v - d)
    out = []
    for n in sorted(sizes):
        for a in (4, 8, 16, 2, 1):
            if n % a == 0:
                out.append((a, n))
    return out


def _seeds(m: Module, family: str) -> list[tuple[int, str]]:
    """Referenced functions with the exact shape of a matched SUPPORT member of FAMILY."""
    prefix = FAMILIES[family][1]
    smod, sst, simg = E.load("SUPPORT")
    matched = {line.split()[0] for line in open(HERE / "src" / "MATCHED.txt")
               if line.strip() and not line.startswith("#")}

    def shape(img, addr, span):
        out = [re.sub(r"0x[0-9a-f]+|-?\b\d+\b", "#", t) for _, _, t in E.disasm(img.read(addr, span), addr)]
        while out and out[-1] == "nop":
            out.pop()
        return tuple(out)
    ref = {}
    for s in sst.sized:
        mem = member_of(s.name, prefix)
        if mem and s.name in matched and simg.covers(s.addr, s.span):
            ref.setdefault(shape(simg, s.addr, s.span), mem)
    out = []
    for s in m.st.sized:
        if 16 <= s.span <= 4096 and m.refs[s.addr] and (E.is_placeholder(s.name)):
            mem = ref.get(shape(m.img, s.addr, s.span))
            if mem:
                out.append((s.addr, mem))
    return out


def _find_member(cand: dict, mem: str, prefix: str, tmangle: str) -> str | None:
    for n in cand:
        if member_of(n, prefix) == mem and tmangle in n:
            return n
    return None


def _mangle_type(t: str) -> str:
    """g++ 2.95 mangling of the placeholder/argument types used here."""
    simple = {"int": "i", "unsigned int": "Ui", "float": "f", "char": "c", "short": "s"}
    if t in simple:
        return simple[t]
    if t.endswith(" *"):
        return "P" + _mangle_type(t[:-2])
    return f"{len(t)}{t}"


def _batch(work: Path, family: str, cands: list[tuple[str, str]]) -> dict:
    """Compile [(argument text, declarations)] in one TU, halving on a compile error."""
    if not cands:
        return {}
    decls = "\n".join(d for _, d in cands if d)
    got = compile_classes(work, family, [a for a, _ in cands], decls)
    if got is not None:
        return got
    if len(cands) == 1:
        return {}
    mid = len(cands) // 2
    out = _batch(work / "a", family, cands[:mid])
    out.update(_batch(work / "b", family, cands[mid:]))
    return out


def solve_seed(m: Module, family: str, addr: int, mem: str, taken: set[int], work: Path):
    """Best (args, decls, assigned) for one seed, or None."""
    header, prefix, arity, over = FAMILIES[family]
    s = m.st.at(addr)
    tag = f"{addr & 0xFFFFFF:06X}"
    trials = []                      # (args text, decls, preference)
    if arity == 1:
        if family in ("VISet",):
            for i, t in enumerate(("int", "unsigned int", "float")):
                trials.append((t, "", i))
            trials.append((f"VIPtr_{tag} *", f"struct VIPtr_{tag};", 0))
        else:
            trials.append((f"VIPtr_{tag} *", f"struct VIPtr_{tag};", 0 if family in ("VIPool", "VIList") else 5))
            for i, (a, n) in enumerate(_sizes(family, _immediates(m.img, addr, s.span))):
                name = f"VIRec{n}_{tag}_A{a}"
                trials.append((name, _vdecl(name, a, n), 1 + (a != 4) + i / 1000))
    else:
        S = [v for v in _immediates(m.img, addr, s.span) if v >= 24]
        for v in S:
            for a, n in ((4, v - 20), (8, v - 24), (16, v - 32)):
                if n > 0 and n % a == 0:
                    for k in ("int", "unsigned int", "float"):
                        name = f"VIRec{n}_{tag}_A{a}"
                        trials.append((f"{k}, {name}", _vdecl(name, a, n), (a != 4) + (k != "int") / 10))
    seen_args, uniq = set(), []
    for t in trials:
        if t[0] not in seen_args:
            seen_args.add(t[0])
            uniq.append(t)
    trials = sorted(uniq, key=lambda t: t[2])[:120]
    cand = _batch(work / tag, family, [(a, d) for a, d, _ in trials])
    passing = []
    for a, d, pref in trials:
        tm = _mangle_type(a.split(", ")[-1]) if arity == 2 else _mangle_type(a)
        n = _find_member(cand, mem, prefix, tm)
        if n and walk(m, {k: v for k, v in cand.items() if tm in k}, [(addr, n)], taken):
            passing.append((pref, a, d, tm))
    best = None
    for pref, a, d, tm in sorted(passing)[:6]:
        sub = {k: v for k, v in cand.items() if tm in k}
        got = walk(m, sub, [(addr, _find_member(sub, mem, prefix, tm))], taken)
        if best is None or len(got) > len(best[2]):
            best = (a, d, got)
    return best


def _final_name(args: str, size_hint: str, tag: str) -> str:
    return args


def cmd_scan(args) -> int:
    family = args.family
    header, prefix, arity, over = FAMILIES[family]
    m = Module()
    seeds = _seeds(m, family)
    taken = {int(line.split()[0], 16): line.split()[2] for line in open(C.SRC.parent / "symbols.txt")
             if line.startswith("0x") and not E.is_placeholder(line.split()[2])}
    # a seed whose code depends on the element size claims its instantiation first, so the
    # instantiation is measured whole rather than split by a size-blind member
    INFORMATIVE = ("Allocate", "Insert", "Find", "Add", "AddNode", "Init")
    seeds = sorted(seeds, key=lambda x: (x[1] not in INFORMATIVE, x[0]))
    print(f"{family}: {len(seeds)} referenced seeds", flush=True)
    work, results = Path(args.work) / family, []
    for addr, mem in seeds:
        if addr in taken:
            continue
        best = solve_seed(m, family, addr, mem, taken, work)
        if not best or len(best[2]) < args.min:
            print(f"{addr:#010x} {mem:13s} no solution" + (f" ({len(best[2])} members)" if best else ""), flush=True)
            continue
        a, d, got = best
        # rename the placeholder to its final form: drop the alignment tag when it is 4,
        # and when a DIFFERENT size reproduces the same members, the size was never
        # measured -- say so (VIElem_<seed>) instead of claiming one
        final_a, final_d = a, d
        mm = re.search(r"(VIRec(\d+)_([0-9A-F]{6}))_A(\d+)", a)
        if mm:
            n, al = int(mm.group(2)), int(mm.group(4))
            probe = f"VIProbe_{mm.group(3)}"
            pa, pd = a.replace(mm.group(0), probe), _vdecl(probe, 4, n + 4 if al != 4 or n % 4 == 0 else n + 3)
            pc = compile_classes(work / f"{addr:08X}_probe", family, [pa], pd)
            ptm = _mangle_type(pa.split(", ")[-1]) if arity == 2 else _mangle_type(pa)
            pseed = _find_member(pc or {}, mem, prefix, ptm)
            pgot = walk(m, pc, [(addr, pseed)], taken) if pc and pseed else {}
            if len(pgot) >= len(got):
                final = f"VIElem_{mm.group(3)}"
                final_a, final_d = a.replace(mm.group(0), final), f"struct {final} {{ int m_data[1]; }};"
            else:
                final = mm.group(1) if al == 4 else mm.group(0)
                final_a, final_d = a.replace(mm.group(0), final), d.replace(mm.group(0), final)
        if final_a != a:
            cand = compile_classes(work / f"{addr:08X}_final", family, [final_a], final_d)
            tm = _mangle_type(final_a.split(", ")[-1]) if arity == 2 else _mangle_type(final_a)
            seed_name = _find_member(cand or {}, mem, prefix, tm)
            got = walk(m, cand, [(addr, seed_name)], taken) if cand and seed_name else {}
        if len(got) < args.min:
            print(f"{addr:#010x} {mem:13s} final type failed ({len(got)} members)", flush=True)
            continue
        if final_a.startswith("VIElem_") or ", VIElem_" in final_a:
            # A size-blind seed: re-solve from a member it CALLS but did not match (an
            # Allocate, whose own immediates carry the element size).  Kept if it names more.
            fc = compile_classes(work / f"{addr:08X}_final2", family, [final_a], final_d)
            for x, nm in list(got.items()):
                for n, t in [(n, m.jal_target(x + o)) for o, ty, n in (fc or {}).get(nm, (0, 0, 0, []))[3]
                             if ty == "R_MIPS_26" and n in fc]:
                    if t and t not in got and t not in taken:
                        better = solve_seed(m, family, t, member_of(n, prefix), taken, work)
                        if better and len(better[2]) > len(got) and set(got) <= set(better[2]):
                            print(f"{addr:#010x} {mem:13s} refined from {t:#010x}: {len(got)} -> {len(better[2])}", flush=True)
                            a2, d2, g2 = better
                            mm2 = re.search(r"(VIRec(\d+)_([0-9A-F]{6}))_A(\d+)", a2)
                            if mm2:
                                fin = mm2.group(1) if mm2.group(4) == "4" else mm2.group(0)
                                a3, d3 = a2.replace(mm2.group(0), fin), d2.replace(mm2.group(0), fin)
                                c3 = compile_classes(work / f"{t:08X}_final", family, [a3], d3)
                                tm3 = _mangle_type(a3.split(", ")[-1]) if arity == 2 else _mangle_type(a3)
                                s3 = _find_member(c3 or {}, member_of(n, prefix), prefix, tm3)
                                g3 = walk(m, c3, [(t, s3)], taken) if c3 and s3 else {}
                                if len(g3) >= len(g2):
                                    final_a, final_d, got = a3, d3, g3
                            break
                else:
                    continue
                break
        taken.update(got)
        results.append({"family": family, "args": final_a, "decls": final_d, "seed": hex(addr),
                        "assigned": {hex(x): n for x, n in got.items()}})
        print(f"{addr:#010x} {mem:13s} {family}<{final_a}> -> {len(got)} members", flush=True)
        Path(args.out).write_text(json.dumps(results, indent=1))
    print(f"{family}: {len(results)} instantiations, {sum(len(r['assigned']) for r in results)} functions")
    return 0


FAMILY_SOURCE = {"VIArray": "viarray.cpp", "VIVector": "vivector.cpp", "VIPool": "vipool.cpp",
                 "VIList": "vilist.cpp", "VISet": "viset.cpp", "VIMultiMap": "vimultimap.cpp",
                 "VIMap": "vimap.cpp"}


# Members the image holds out of line that SUPPORT never instantiated explicitly (they are
# inline in the class body there).  An explicit instantiation still emits them.
EXTRA_DECLS = {
    "VIList": ["template int VIList<{T}>::End() const;"],
    "VIVector": ["template int VIVector<{T}>::End() const;", "template int VIVector<{T}>::Begin() const;",
                 "template int VIVector<{T}>::Next(int) const;"],
    "VIArray": ["template int VIArray<{T}>::End() const;", "template int VIArray<{T}>::Begin() const;",
                "template int VIArray<{T}>::Next(int) const;"],
    "VISet": [], "VIPool": [], "VIMultiMap": [], "VIMap": [],
}


def decl_catalogue(family: str) -> list[str]:
    """Every member SUPPORT instantiates for FAMILY, as a template over {T} (one argument)
    or {K}/{V} (two): read off SUPPORT's own explicit-instantiation lines (macro bodies
    included), so the syntax is the one this toolchain already accepted.  A two-argument
    line is used only when neither argument is `int` -- `int` is also every return type."""
    arity = FAMILIES[family][2]
    out = []
    for line in (HERE / "src" / FAMILY_SOURCE[family]).read_text().splitlines():
        line = line.strip().rstrip("\\").strip()
        if not re.match(r"^template\s+(.*?)\b" + family + r"<(.+?)>::(.*);$", line):
            continue
        rest = line[line.index(family + "<") + len(family) + 1:]
        depth, i = 1, 0
        while depth:
            depth += {"<": 1, ">": -1}.get(rest[i], 0)
            i += 1
        arg = rest[:i - 1].strip()
        if arity == 1:
            if not (arg == "T" or re.fullmatch(r"\w+(\s*\*)?|unsigned \w+", arg)):
                continue
            t = re.sub(r"\bT\b" if arg == "T" else re.escape(arg), "{T}", line)
        else:
            parts = [x.strip() for x in arg.split(",")]
            if len(parts) != 2:
                continue
            k, v = parts
            if "int" in (k, v) or k == v:
                continue
            t = line
            for x, ph in ((k, "{K}"), (v, "{V}")):
                t = re.sub(r"(?<![\w:])" + re.escape(x) + r"(?![\w:])", ph, t)
        if t not in out:
            out.append(t)
    # `VIWnd *const &` loses its space with the type: `intconst &` would not compile
    out = [re.sub(r"(\{[TKV]\})const\b", r"\1 const", x) for x in out]
    return out + [x for x in EXTRA_DECLS.get(family, []) if x not in out]


def member_decls(work: Path, family: str, args: dict, decls: str, tm: str) -> dict[str, str]:
    """{mangled name with the dummy arguments: declaration template} -- one compile per
    member.  ARGS fills the template ({T}, or {K}/{V}); TM is the dummy's mangling, which
    every symbol this table keys must contain."""
    # Instantiating one member also emits, implicitly, the members it calls (both come out
    # WEAK on this toolchain), so the declaration's own symbol is the one emitted symbol
    # that IS the member it names; an overload left ambiguous is refused, never guessed.
    prefix = FAMILIES[family][1]
    out = {}
    for i, d in enumerate(decl_catalogue(family)):
        mm = re.search(r"::(~?\w+)\(", d)
        if not mm:
            continue
        mem = {"~" + family: "dtor", family: "ctor"}.get(mm.group(1), mm.group(1))
        text = d
        for k, v in args.items():
            text = text.replace("{" + k + "}", v)
        cand = compile_classes_raw(work / f"m{i}", family, decls + "\n" + text)
        own = [n for n in cand or {} if tm in n and member_of(n, prefix) == mem]
        if len(own) == 1 and own[0] not in out:
            out[own[0]] = d
        elif len(own) > 1:
            print(f"ambiguous: {d} emits {own}", file=sys.stderr)
    return out


def compile_classes_raw(work: Path, family: str, body: str) -> dict | None:
    """compile_classes for arbitrary instantiation text."""
    work.mkdir(parents=True, exist_ok=True)
    hdr = FAMILIES[family][0]
    src = work / "inst.cpp"
    src.write_text(f'#include "game.h"\n#include "{hdr}"\n{body}\n')
    r = subprocess.run(C.compile_cmd("inst") + ["-I" + str(C.SRC), str(src), "-o", str(work / "inst.s")],
                       capture_output=True, text=True, cwd=work)
    if r.returncode:
        return None
    subprocess.run(["wine", str(C.ASSEMBLER), "-o", str(work / "inst.o"), str(work / "inst.s")],
                   capture_output=True, cwd=work)
    syms = subprocess.run([D._tool("readelf"), "-sW", str(work / "inst.o")], capture_output=True, text=True).stdout
    return {l.split()[7]: None for l in syms.splitlines() if len(l.split()) >= 8 and l.split()[3] == "FUNC"}


def cmd_emit(args) -> int:
    family = args.family
    arity = FAMILIES[family][2]
    results = json.loads(Path(args.results).read_text())
    work = Path(args.work) / ("emit_" + family)
    dummy = "VIDummyQ"
    tables = {}

    BUILTIN = {"int", "unsigned int", "float", "char", "short"}

    def table(T: str):
        """(lookup table, dummy mangling) for an argument list shaped like T.  A placeholder
        argument is distinctive, so one dummy table serves them all by renaming its
        mangling; a BUILTIN one (`i`) is not -- its letter is in every name -- so its table is
        built with the argument itself and keys need no renaming (dummy mangling = its own)."""
        V0 = T.split(", ")[-1]
        if V0 in BUILTIN or V0.rstrip(" *") in BUILTIN:
            if T not in tables:
                fill = {"T": T} if arity == 1 else {"K": T.split(", ")[0], "V": V0}
                tables[T] = (member_decls(work / ("b_" + re.sub(r"\W", "_", T)), family, fill, "",
                                          _mangle_type(V0)), _mangle_type(V0))
            return tables[T]
        if arity == 1:
            ptr = T.endswith("*")
            key = "ptr" if ptr else "val"
            if key not in tables:
                d, dd = (dummy + " *", f"struct {dummy};") if ptr else (dummy, f"struct {dummy} {{ int m_data[1]; }};")
                tables[key] = (member_decls(work / key, family, {"T": d}, dd, _mangle_type(d)), _mangle_type(d))
            return tables[key]
        K = T.split(", ")[0]
        if K not in tables:
            tables[K] = (member_decls(work / re.sub(r"\W", "_", K), family, {"K": K, "V": dummy},
                                      f"struct {dummy} {{ int m_data[1]; }};", _mangle_type(dummy)),
                         _mangle_type(dummy))
        return tables[K]

    lines = [args.head.rstrip() + "\n", '#include "game.h"', f'#include "{FAMILIES[family][0]}"']
    sym = C.SRC.parent / "symbols.txt"
    text = sym.read_text()
    renamed = 0
    for r in sorted(results, key=lambda r: min(int(a, 16) for a in r["assigned"])):
        T = r["args"]
        V = T.split(", ")[-1]
        addrs = sorted(int(a, 16) for a in r["assigned"])
        what = ("size not measurable from these members" if "VIElem" in V
                else "pointer argument" if V.endswith("*") else "size measured")
        lines.append(f"\n// {addrs[0]:#010x}..{addrs[-1]:#010x}: {len(addrs)} members; {what}.")
        lines.append(r["decls"])
        tbl, dm = table(T)
        fill = {"T": T} if arity == 1 else {"K": T.split(", ")[0], "V": V}
        for a in addrs:
            name = r["assigned"][hex(a)]
            key = name.replace(_mangle_type(V), dm)
            if key not in tbl:
                raise SystemExit(f"no declaration for {name} (looked up {key})")
            decl = tbl[key]
            for k, v in fill.items():
                decl = decl.replace("{" + k + "}", v)
            lines.append(decl + f"  // {a:#010x}")
            pat = re.compile(r"^" + f"0x{a:08X}" + r"  \[CODE\]  (\S+)(.*)$", re.M)
            mm = pat.search(text)
            if not mm:
                raise SystemExit(f"{a:#010x} is not a CODE entry in the map")
            text = (text[:mm.start()] + f"0x{a:08X}  [CODE]  {name}  # name provisional: placeholder"
                    f" element type (phase 62, template copy)" + text[mm.end():])
            renamed += 1
    Path(args.tu).write_text("\n".join(lines) + "\n")
    sym.write_text(text)
    print(f"{len(results)} instantiations written to {args.tu}; {renamed} map entries renamed")
    return 0


def cmd_resolve(args) -> int:
    """Settle addresses two families' scans both claimed.  A member whose code does not
    depend on its family (a POD DestroyPool, a destructor) can be byte-identical across
    VIPool/VISet/VIMultiMap; instantiations are taken largest first, and one that shares ANY
    address with a larger one is dropped WHOLE -- a member kept from it could call a struck
    one, which would then carry the other family's name.  Writes <scan>.resolved.json."""
    allr = []
    for path in args.scans:
        for x in json.loads(Path(path).read_text()):
            allr.append((path, x))
    allr.sort(key=lambda px: -len(px[1]["assigned"]))
    taken, out, dropped = {}, {p: [] for p in args.scans}, 0
    for path, x in allr:
        if any(a in taken for a in x["assigned"]) or len(x["assigned"]) < 2:
            dropped += 1
            continue
        taken.update({a: path for a in x["assigned"]})
        out[path].append(x)
    for path in args.scans:
        dest = Path(path).with_suffix(".resolved.json")
        dest.write_text(json.dumps(out[path], indent=1))
        print(f"{dest}: {len(out[path])} instantiations, {sum(len(x['assigned']) for x in out[path])} functions")
    print(f"dropped {dropped} instantiations")
    return 0


def cmd_name_implicit(args) -> int:
    """Name, in the map, every function an object emits that the map lacks, at the one
    address its MATCHED callers' jals reach -- a member an explicit instantiation pulls in
    implicitly (DestroyPool for a Clear, Next for a DestroyPool).  It then scores as what it
    is, exact or wrong, never as NO-TGT.  Refused when callers disagree, or the address is
    already named."""
    import eqcalls
    m = Module()
    rel = eqcalls.relocated(m.img)
    names = {s.name for s in m.st.sized}
    sym = C.SRC.parent / "symbols.txt"
    text = sym.read_text()
    done = 0
    for obj in args.objs:
        funcs = {}
        for line in subprocess.run([D._tool("readelf"), "-sW", obj], capture_output=True, text=True).stdout.splitlines():
            p = line.split()
            if len(p) >= 8 and p[3] == "FUNC":
                funcs[p[7]] = int(p[1], 16)
        relocs = D.elf_relocations(Path(obj), ".text")
        for n in [n for n in funcs if n not in names]:
            tg = set()
            for off, typ, callee in relocs:
                if typ == "R_MIPS_26" and callee == n:
                    caller = max((v, k) for k, v in funcs.items() if v <= off)[1]
                    if caller in names:
                        a = m.st.get(caller).addr + (off - funcs[caller])
                        w = int.from_bytes(rel[a:a + 4], "little")
                        tg.add((w & 0x03FFFFFF) << 2)
            if len(tg) != 1:
                continue
            t = tg.pop()
            pat = re.compile(r"^" + f"0x{t:08X}" + r"  \[CODE\]  (\S+)(.*)$", re.M)
            mm = pat.search(text)
            if not mm or not E.is_placeholder(mm.group(1)):
                continue
            text = (text[:mm.start()] + f"0x{t:08X}  [CODE]  {n}  # name provisional: placeholder element type"
                    " (phase 62, template copy; named off its caller)" + text[mm.end():])
            names.add(n)
            done += 1
            print(f"named {t:#010x} {n}")
    sym.write_text(text)
    print(f"{done} named")
    return 0


def cmd_solve(args) -> int:
    m = Module()
    work = Path(args.work)
    cand = compile_classes(work, args.family, [args.targs], args.decls)
    if cand is None:
        raise SystemExit("the candidate class did not compile")
    prefix = FAMILIES[args.family][1]
    seeds = []
    for s in args.seeds:
        member, addr = s.split("=")
        names = [n for n in cand if member_of(n, prefix) == member]
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


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("solve", help="name every member of one instantiation from seeds")
    s.add_argument("targs", help="the template arguments, e.g. 'int, VIBuddy'")
    s.add_argument("seeds", nargs="+", help="member=address, e.g. Find=0x00BE1F68 (dtor/ctor for those)")
    s.add_argument("--family", default="VIMap", choices=sorted(FAMILIES))
    s.add_argument("--decls", default="", help="C++ declarations the arguments need")
    s.add_argument("--work", default=str(HERE / "build" / "eqtmpl"))
    s.add_argument("--json")
    s.set_defaults(fn=cmd_solve)
    c = sub.add_parser("scan", help="find and solve every live instantiation of a family")
    c.add_argument("out")
    c.add_argument("--family", default="VIMap", choices=sorted(FAMILIES))
    c.add_argument("--min", type=int, default=2, help="members an instantiation needs to be kept")
    c.add_argument("--work", default=str(HERE / "build" / "eqtmpl"))
    c.set_defaults(fn=cmd_scan)
    e = sub.add_parser("emit", help="write a TU and rename the map from a scan's results")
    e.add_argument("results")
    e.add_argument("tu")
    e.add_argument("--family", required=True, choices=sorted(FAMILIES))
    e.add_argument("--head", default="// CLIENT's own copies of a SUPPORT container (eqtmpl.py scan).")
    e.add_argument("--work", default=str(HERE / "build" / "eqtmpl"))
    e.set_defaults(fn=cmd_emit)
    r = sub.add_parser("resolve", help="settle addresses claimed by two families' scans")
    r.add_argument("scans", nargs="+")
    r.set_defaults(fn=cmd_resolve)
    ni = sub.add_parser("name-implicit", help="name members an object emits implicitly, off their callers")
    ni.add_argument("objs", nargs="+")
    ni.set_defaults(fn=cmd_name_implicit)
    args = p.parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
