#!/usr/bin/env python3
"""eqperm.py -- a PERMUTER: random source rewrites of ONE open function,
compiled and scored unattended, many thousands an hour.

Phase 61 ended on a residue that is search-shaped rather than insight-shaped:
every open function was diagnosed down to a register-allocation or scheduler
TIE, settled by a quantity summed over the whole function (a live range's
length, a pseudo's reference count, a dependent count).  An agent tries one
source variant per thought.  This tries several a second, for no tokens, and
a person reviews what it finds.  It is decomp-permuter's method
(github.com/simonlindholm/decomp-permuter), rebuilt for C++, which that
tool's C parser cannot read.

    ./eqperm.py setup SYMBOL [--tu STEM] [--line N] [--start FILE] [--name RUN]
    ./eqperm.py run   RUN [-j N] [--minutes M]      # RUN is the symbol unless --name
    ./eqperm.py score RUN FILE [--diff]             # score one function text
    ./eqperm.py reduce RUN FILE                     # revert what a find does not need
    ./eqperm.py best  RUN [-n K]                    # the best finds, as diffs

A run lives in ../decomp-perm/RUN/: `base.cpp` (the start text), `found/` (every
improvement, named score_wrong_serial), `best.cpp`, `log.txt`, `elites.json`.

How one candidate is built.  `setup` preprocesses the TU up to the function
ONCE (`prefix.ii`), expands the TU's own macros inside the function so the
mutations can reach into them, and strips its comments.  A candidate is that
prefix plus the mutated function, compiled by `cc1plus.exe` directly -- no
driver and no cpp, 0.12 s against 0.17 s -- and assembled by the same SN
`ps2eeas` eqcheck uses.  The prefix is kept whole rather than trimmed to what
the function needs: gcc 2.95 hashes some RTL by POINTER (cse's SYMBOL_REF),
so the code compiled before the function can matter.  `setup` refuses to go
on unless the unmutated candidate's assembly is IDENTICAL to the driver's
build of the real TU.

The score.  Instructions are aligned on their opcode skeleton (registers and
immediates masked), as decomp-permuter does: an aligned pair that differs
only in registers costs 5, in an immediate 5, an instruction present on both
sides but out of place 60, one present on one side only 100.  A register-only
difference is cheap, so the search can follow a gradient through allocation
ties that the byte count cannot see.  A candidate scoring 0 is re-scored by
`eqcheck.score` (relocation masks plus the CALLEE check) before it is called
a match.

What it may NOT do is decide that a hit is the original's source.  A hit is
bytes, which pins down the code's meaning but not its spelling: decide on
naturalness, and the FAKEMATCH policy, before anything reaches `src/`.
"""

from __future__ import annotations

import argparse
import collections
import difflib
import hashlib
import json
import math
import multiprocessing as mp
import os
import random
import re
import shutil
import signal
import struct
import subprocess
import tempfile
import sys
import textwrap
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E            # noqa: E402  (also sets the wine environment)
import eqcheck as C             # noqa: E402
import eqperm_mut as M          # noqa: E402

RUNS = HERE.parent / "decomp-perm"


# --- gcc 2.x mangled names ------------------------------------------------------

OPS = {"ml": "*", "pl": "+", "mi": "-", "dv": "/", "md": "%", "as": "=",
       "eq": "==", "ne": "!=", "lt": "<", "gt": ">", "le": "<=", "ge": ">=",
       "apl": "+=", "ami": "-=", "amu": "*=", "adv": "/=", "vc": "[]",
       "nw": "new", "dl": "delete", "vn": "new []", "vd": "delete []",
       "ls": "<<", "rs": ">>", "nt": "!", "aa": "&&", "oo": "||", "ad": "&",
       "or": "|", "er": "^", "co": "~", "pp": "++", "mm": "--", "cl": "()",
       "rf": "->", "aad": "&=", "aor": "|=", "aer": "^=", "als": "<<=",
       "ars": ">>="}


def _name(s: str, i: int) -> tuple[str, int]:
    j = i
    while j < len(s) and s[j].isdigit():
        j += 1
    n = int(s[i:j])
    return s[j:j + n], j + n


def _skip_type(s: str, i: int) -> int:
    while s[i] in "CVUSPR":
        i += 1
    c = s[i]
    if c.isdigit():
        return _name(s, i)[1]
    if c == "Q":
        i += 1
        if s[i] == "_":
            j = s.index("_", i + 1)
            cnt, i = int(s[i + 1:j]), j + 1
        else:
            cnt, i = int(s[i]), i + 1
        for _ in range(cnt):
            if s[i] == "t":
                raise ValueError("template in a qualified name")
            i = _name(s, i)[1]
        return i
    if c == "A":
        j = s.index("_", i)
        return _skip_type(s, j + 1)
    if c == "F":
        i += 1
        while s[i] != "_":
            i = _skip_type(s, i)
        return _skip_type(s, i + 1)
    if c == "M":
        return _skip_type(s, _skip_type(s, i + 1))
    if c == "t":
        raise ValueError("template argument")
    return i + 1


def mangled_params(args: str) -> int:
    """How many parameters a gcc 2.x argument string encodes."""
    if args in ("", "v"):
        return 0
    i, n = 0, 0
    while i < len(args):
        c = args[i]
        if c == "T":
            i += 2
            n += 1
        elif c == "N":
            n += int(args[i + 1])
            i += 3
        elif c == "e":
            i += 1
        else:
            i = _skip_type(args, i)
            n += 1
    return n


def mangled_names(args: str) -> set[str]:
    """The class and enum names a mangled argument string spells."""
    out, i = set(), 0
    while i < len(args):
        c = args[i]
        if c in "TN":
            i += 1
            while i < len(args) and (args[i].isdigit() or args[i] == "_"):
                i += 1
        elif c == "A":
            i = args.index("_", i) + 1
        elif c.isdigit():
            nm, i = _name(args, i)
            out.add(nm)
        else:
            i += 1
    return out


def split_mangled(sym: str) -> tuple[str, str, str]:
    """(function name, class or '', argument string)."""
    m = re.match(r"^__([a-z]{2,3})__(.*)$", sym)
    if m and m.group(1) in OPS:
        name, rest = "operator" + OPS[m.group(1)], m.group(2)
    elif sym.startswith("__") and sym[2:3].isdigit() or sym.startswith("__Q"):
        name, rest = "", sym[2:]                      # a constructor
    elif sym.startswith("_$_"):
        name, rest = "~", sym[3:]
    else:
        k = sym.find("__", 1)
        if k < 0:
            return sym, "", ""
        name, rest = sym[:k], sym[k + 2:]
    if rest.startswith("F"):
        return name, "", rest[1:]
    if rest.startswith("C") and rest[1:2].isdigit():
        rest = rest[1:]                               # a const member function
    cls = ""
    if rest[:1].isdigit():
        cls, i = _name(rest, 0)
        rest = rest[i:]
    elif rest.startswith("Q"):
        i = 1
        if rest[i] == "_":
            j = rest.index("_", i + 1)
            cnt, i = int(rest[i + 1:j]), j + 1
        else:
            cnt, i = int(rest[i]), i + 1
        parts = []
        for _ in range(cnt):
            p, i = _name(rest, i)
            parts.append(p)
        cls, rest = "::".join(parts), rest[i:]
    if name == "":
        name = cls.split("::")[-1]
    elif name == "~":
        name = "~" + cls.split("::")[-1]
    return name, cls, rest


# --- finding the function --------------------------------------------------------

def _func_declarator(n):
    d = n.child_by_field_name("declarator")
    for _ in range(6):
        if d is None or d.type == "function_declarator":
            return d
        d = d.child_by_field_name("declarator")
    return None


def find_function(text: str, sym: str, line: int | None):
    """(start_byte, end_byte) of the definition `sym` names in a TU."""
    name, cls, args = split_mangled(sym)
    want = f"{cls}::{name}" if cls else name
    try:
        nparams = mangled_params(args)
    except (ValueError, IndexError):
        nparams = None
    fn = M.Fn(text)
    hits = []
    for n in fn.of("function_definition"):
        d = _func_declarator(n)
        if d is None:
            continue
        nm = re.sub(r"\s+", "", fn.t(d.child_by_field_name("declarator")))
        if nm != want.replace(" ", ""):
            continue
        ps = d.child_by_field_name("parameters")
        pk = [k for k in ps.named_children if k.type != "comment"] if ps else []
        if len(pk) == 1 and fn.t(pk[0]).strip() == "void":
            pk = []
        start = n.start_byte
        if n.parent is not None and n.parent.type == "template_declaration":
            start = n.parent.start_byte
        ln = text.encode()[:n.start_byte].count(b"\n") + 1
        words = set(re.findall(r"[A-Za-z_]\w*", fn.t(ps))) if ps else set()
        hits.append((start, n.end_byte, len(pk), ln, words))
    if line is not None:
        hits = [h for h in hits if h[3] == line]
    else:
        if nparams is not None and len(hits) > 1:
            hits = [h for h in hits if h[2] == nparams]
        if len(hits) > 1:
            names = mangled_names(args)
            hits = [h for h in hits if names <= h[4]]
    if len(hits) != 1:
        found = ", ".join(f"line {h[3]} ({h[2]} params)" for h in hits) or "none"
        raise SystemExit(f"{sym}: {len(hits)} candidate definitions of {want} "
                         f"({found}); pass --line")
    return hits[0][0], hits[0][1]


def find_tu(sym: str) -> str:
    name, cls, _ = split_mangled(sym)
    pat = re.compile(re.escape(f"{cls}::{name}" if cls else name) + r"\s*\(")
    hits = [p.stem for p in sorted(C.SRC.glob("*.cpp"))
            if pat.search(p.read_text(errors="replace"))]
    if len(hits) != 1:
        raise SystemExit(f"{sym}: found in {hits or 'no TU'}; pass --tu")
    return hits[0]


# --- macros ---------------------------------------------------------------------

def local_macros(text: str, before: int) -> dict:
    """name -> (params or None, body) for the TU's own #defines."""
    fn = M.Fn(text)
    out = {}
    for n in fn.of("preproc_function_def", "preproc_def"):
        if n.start_byte >= before:
            continue
        nm = fn.t(n.child_by_field_name("name"))
        val = n.child_by_field_name("value")
        body = fn.t(val) if val is not None else ""
        body = re.sub(r"[ \t]*\\\n", "\n", body).strip("\n")
        params = None
        if n.type == "preproc_function_def":
            ps = n.child_by_field_name("parameters")
            params = [fn.t(k) for k in ps.named_children] if ps else []
        if "#" in body:
            raise SystemExit(f"macro {nm} uses # or ##; not expanded by eqperm")
        out[nm] = (params, body)
    return out


def _args(text: str, i: int) -> tuple[list[str], int]:
    """Split a macro call's arguments; text[i] is '('.  Returns (args, end)."""
    depth, cur, out = 0, [], []
    j = i
    while j < len(text):
        c = text[j]
        if c in "([{":
            depth += 1
            if depth > 1:
                cur.append(c)
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                out.append("".join(cur).strip())
                return out, j + 1
            cur.append(c)
        elif c == "," and depth == 1:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(c)
        j += 1
    raise ValueError("unbalanced macro call")


def expand_macros(text: str, macros: dict) -> str:
    if not macros:
        return text
    rx = re.compile(r"\b(" + "|".join(map(re.escape, macros)) + r")\b")
    for _ in range(1000):
        m = rx.search(text)
        if m is None:
            return text
        params, body = macros[m.group(1)]
        if params is None:
            text = text[:m.start()] + body + text[m.end():]
            continue
        k = m.end()
        while k < len(text) and text[k] in " \t\n":
            k += 1
        if k >= len(text) or text[k] != "(":
            raise ValueError(f"function-like macro {m.group(1)} without arguments")
        args, end = _args(text, k)
        if params == [] and args == [""]:
            args = []
        sub = dict(zip(params, args))
        if sub:
            prx = re.compile(r"\b(" + "|".join(map(re.escape, sub)) + r")\b")
            body = prx.sub(lambda x: sub[x.group(1)], body)
        # re-indent the body to the call site, for whoever reads the result
        line_start = text.rfind("\n", 0, m.start()) + 1
        ind = text[line_start:m.start()]
        if ind.strip() == "":
            # the first line follows `#define X(..)` on its own line; the
            # continuation lines carry the macro's indentation, so they are
            # dedented by their own common prefix
            first, *rest = body.split("\n")
            rest = textwrap.dedent("\n".join(rest)).split("\n") if rest else []
            body = "\n".join([first] + [ind + l if l else l for l in rest])
        text = text[:m.start()] + body + text[end:]
    raise ValueError("macro expansion did not terminate")


# --- object files and the score ---------------------------------------------------

FIELD = {2: 0xFFFFFFFF, 4: 0x03FFFFFF, 5: 0xFFFF, 6: 0xFFFF, 7: 0xFFFF,
         8: 0xFFFF, 10: 0xFFFF}      # R_MIPS_32, _26, HI16, LO16, GPREL16, LITERAL, PC16


def elf_function(path: Path, symbol: str) -> tuple[list[int], list[int]]:
    """(words, per-word relocation field masks) of one FUNC symbol, read
    straight out of an ELF32 little-endian object -- no readelf, no objcopy."""
    b = path.read_bytes()
    shoff, = struct.unpack_from("<I", b, 0x20)
    shentsize, shnum, _ = struct.unpack_from("<HHH", b, 0x2E)
    secs = [struct.unpack_from("<IIIIIIIIII", b, shoff + i * shentsize)
            for i in range(shnum)]
    symtab = next(s for s in secs if s[1] == 2)
    stroff = secs[symtab[6]][4]
    want = symbol.encode()
    hit = None
    for k in range(symtab[5] // 16):
        nm, val, size, info, _, shndx = struct.unpack_from("<IIIBBH", b, symtab[4] + 16 * k)
        if info & 0xF != 2:
            continue
        end = b.index(b"\0", stroff + nm)
        if b[stroff + nm:end] == want:
            hit = (val, size, shndx)
            break
    if hit is None:
        raise KeyError(symbol)
    val, size, shndx = hit
    off = secs[shndx][4] + val
    words = list(struct.unpack_from(f"<{size // 4}I", b, off))
    masks = [0] * len(words)
    for s in secs:
        if s[1] not in (9, 4) or s[7] != shndx:
            continue
        ent = 8 if s[1] == 9 else 12
        for k in range(s[5] // ent):
            r_off, r_info = struct.unpack_from("<II", b, s[4] + ent * k)
            if val <= r_off < val + size:
                masks[(r_off - val) // 4] |= FIELD.get(r_info & 0xFF, 0xFFFFFFFF)
    return words, masks


def decode(w: int) -> tuple[int, int, int]:
    """(register-field mask, immediate-field mask, kind): kind 0 plain,
    1 a branch (its immediate is an offset), 2 a jump (a relocated target)."""
    o = w >> 26
    if o in (0, 0x1C):                       # SPECIAL, MMI: rs rt rd
        return 0x03FFF800, 0, 0
    if o == 1:                               # REGIMM: rs, rt is the sub-opcode
        return 0x03E00000, 0xFFFF, 1
    if o in (2, 3):
        return 0, 0x03FFFFFF, 2
    if o in (4, 5, 6, 7, 20, 21, 22, 23):
        return 0x03FF0000, 0xFFFF, 1
    if o in (0x11, 0x12):                    # COP1, COP2
        fmt = (w >> 21) & 0x1F
        if fmt == 8:
            return 0, 0xFFFF, 1
        if o == 0x12 and w & (1 << 25):
            return 0x001FFFC0, 0, 0
        if fmt in (0, 1, 2, 4, 5, 6):
            return 0x001FF800, 0, 0
        return 0x001FFFC0, 0, 0
    if o == 0x10:
        return 0x001FF800, 0, 0
    return 0x03FF0000, 0xFFFF, 0             # I-type: loads, stores, immediates


PEN_REG, PEN_IMM, PEN_REORDER, PEN_INDEL = 5, 5, 60, 100


def grade(target: list[int], cand: list[int], cmask: list[int],
          pairs: list | None = None) -> tuple[int, int]:
    """(graded score, wrong instructions) -- 0 means identical after masking.
    With `pairs`, every aligned (target, candidate) word pair is appended to it."""
    if len(target) == len(cand) and pairs is None:
        same = True
        for t, c, m in zip(target, cand, cmask):
            if (t & ~m) != (c & ~m):
                rm, im, kind = decode(t)
                if kind == 2 and (t & ~im) == (c & ~im):
                    continue
                same = False
                break
        if same:
            return 0, 0
    dt = [decode(w) for w in target]
    dc = [decode(w) for w in cand]
    tk = [w & ~(d[0] | d[1]) for w, d in zip(target, dt)]
    ck = [w & ~(d[0] | d[1]) for w, d in zip(cand, dc)]
    sm = difflib.SequenceMatcher(None, tk, ck, autojunk=False)
    cost = wrong = 0
    t_only: list[int] = []
    c_only: list[int] = []
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag != "equal":
            t_only += [target[a] & ~(dt[a][1] if dt[a][2] else 0) for a in range(i1, i2)]
            c_only += [cand[b] & ~(dc[b][1] if dc[b][2] else 0) for b in range(j1, j2)]
            continue
        for a, b in zip(range(i1, i2), range(j1, j2)):
            t, c, m = target[a], cand[b], cmask[b]
            if pairs is not None:
                pairs.append((t, c))
            if (t & ~m) == (c & ~m):
                continue
            rm, im, kind = dt[a]
            pen = 0
            if (t & rm) != (c & rm):
                pen += PEN_REG
            if kind != 2 and (t & im & ~m) != (c & im & ~m):
                pen += PEN_IMM
            if pen:
                cost += pen
                wrong += 1
    if t_only or c_only:
        r = sum((collections.Counter(t_only) & collections.Counter(c_only)).values())
        cost += r * PEN_REORDER + (len(t_only) + len(c_only) - 2 * r) * PEN_INDEL
        wrong += max(len(t_only), len(c_only))
    return cost, wrong


# --- the allocation-order term -----------------------------------------------------
#
# Register allocation is all-or-nothing in the bytes: global.c hands out the
# callee-saved registers in allocno PRIORITY order, floor_log2(refs) * refs /
# live_length, so a pseudo that needs two more references shows no change at
# all until it has both.  The target's registers say what order its pseudos
# were allocated in (of two conflicting call-crossing pseudos, the one that
# went first took the lower register), and the candidate's own -dl/-dg dumps
# say what order they WERE allocated in and by how much.  The sum of the
# priority gaps over the pairs in the wrong order is a gradient the byte
# score cannot see.

S_REGS = [16, 17, 18, 19, 20, 21, 22, 23, 30]      # s0..s7, s8: reg_alloc_order


def gpr_shifts(w: int) -> tuple[int, ...]:
    """Bit positions of the GENERAL-register fields of an instruction."""
    o = w >> 26
    if o in (0, 0x1C):
        return (21, 16, 11)
    if o == 1:
        return (21,)
    if o in (2, 3):
        return ()
    if o in (0x11, 0x12):
        fmt = (w >> 21) & 0x1F
        return (16,) if fmt in (0, 1, 2, 4, 5, 6) and not (o == 0x12 and w & (1 << 25)) else ()
    if o in (0x31, 0x35, 0x36, 0x39, 0x3D, 0x3E):    # FP/VU loads and stores
        return (21,)
    return (21, 16)


def _last_function(text: str) -> str:
    k = text.rfind("\n;; Function ")
    return text[k:] if k >= 0 else text


def alloc_report(wd: Path, stem: str, pairs: list) -> tuple[float, list]:
    """(gap, [(A, B, prio A, prio B, reg A, reg B, wanted A, wanted B)])."""
    try:
        g = _last_function((wd / f"{stem}.cc.greg").read_text(errors="replace"))
        lr = _last_function((wd / f"{stem}.cc.lreg").read_text(errors="replace"))
    except OSError:
        return 0.0, []
    order, conf, disp = [], {}, {}
    lines = g.split("\n")
    for i, line in enumerate(lines):
        m = re.match(r";; \d+ regs to allocate:(.*)", line)
        if m:
            order = [int(t.split("+")[0]) for t in m.group(1).split() if t[0].isdigit()]
            continue
        m = re.match(r";; (\d+) conflicts:(.*)", line)
        if m:
            conf[int(m.group(1))] = {int(t) for t in m.group(2).split()}
            continue
        if line.startswith(";; Register dispositions:"):
            for l2 in lines[i + 1:]:
                if not l2.strip():
                    break
                for a, b in re.findall(r"(\d+) in (-?\d+)", l2):
                    disp[int(a)] = int(b)
    prio = {}
    for m in re.finditer(r"^Register (\d+) used (\d+) times across (\d+) insns", lr, re.M):
        r, n, live = int(m.group(1)), int(m.group(2)), int(m.group(3))
        if live > 0 and n > 0:
            prio[r] = (n.bit_length() - 1) * n / live
    votes: dict[int, collections.Counter] = collections.defaultdict(collections.Counter)
    for t, c in pairs:
        for sh in gpr_shifts(t):
            rc = (c >> sh) & 31
            if rc in S_REGS:
                votes[rc][(t >> sh) & 31] += 1
    want = {}
    for rc, ctr in votes.items():
        rt, n = ctr.most_common(1)[0]
        if n >= 2 and rt in S_REGS:
            want[rc] = rt
    pos = {r: i for i, r in enumerate(S_REGS)}
    seq = [p for p in order if disp.get(p) in want and p in prio]
    gap, bad = 0.0, []
    for i, A in enumerate(seq):
        wa = want[disp[A]]
        for B in seq[i + 1:]:
            if B not in conf.get(A, ()):
                continue
            wb = want[disp[B]]
            # a pair counts only when A, allocated first, is in the wrong
            # register AND took one at or below B's: A pushed B out.  Two
            # pseudos already in their target registers say nothing about
            # order -- a later allocno can find a lower register still free.
            if pos[wa] > pos[wb] and disp[A] != wa and pos[disp[A]] <= pos[wb]:
                pa, pb = prio[A], prio[B]
                gap += min(1.0, (pa - pb) / pa) if pa > pb else 0.02
                bad.append((A, B, pa, pb, disp[A], disp[B], wa, wb))
    return gap, bad


# --- one run directory ------------------------------------------------------------

class Ctx:
    """Everything needed to build and score a candidate for one symbol."""

    def __init__(self, rundir: Path, alloc: float = 0.0, length: float = 0.0):
        self.dir = rundir
        self.alloc = alloc
        self.length = length    # per-word penalty for a different instruction count
        self.meta = json.loads((rundir / "meta.json").read_text())
        self.prefix = (rundir / "prefix.ii").read_text()
        self.sym = self.meta["symbol"]
        self._st = self._img = None
        self._target = None

    def target(self) -> list[int]:
        if self._target is None:
            _, self._st, self._img = E.load()
            t = self._img.func(self._st.get(self.sym), trim=True)
            self._target = list(struct.unpack(f"<{len(t) // 4}I", t))
        return self._target

    def source(self, func: str) -> str:
        body = re.sub(r"\bdecltype\s*\(", "__typeof__(", func)
        if self.meta.get("cpp"):
            body = native_cpp(body, self.dir / "macros.h", self.meta["line"],
                              self.meta["file"])
        return f'{self.prefix}\n# {self.meta["line"]} "{self.meta["file"]}"\n{body}\n'

    def build(self, func: str, wd: Path) -> Path | None:
        wd.mkdir(parents=True, exist_ok=True)
        ii, s, o = wd / "c.ii", wd / "c.s", wd / "c.o"
        for f in (s, o):
            if f.exists():
                f.unlink()
        try:
            ii.write_text(self.source(func))
        except RuntimeError:          # the host cpp refused a mangled macro call
            return None
        try:
            r = subprocess.run(["wine", self.meta["cc1"], str(ii), "-quiet", "-dumpbase",
                                self.meta["stem"] + ".cc"] + self.meta["flags"] +
                               (["-dl", "-dg"] if self.alloc else []) + ["-o", str(s)],
                               capture_output=True, cwd=wd, timeout=60)
        except subprocess.TimeoutExpired:
            return None
        # cc1plus leaves a partial .s behind on an error; the exit code says
        if r.returncode or not s.exists():
            return None
        for _ in range(2):            # wine fails spuriously now and then
            try:
                subprocess.run(["wine", str(C.ASSEMBLER), "-o", str(o), str(s)],
                               capture_output=True, cwd=wd, timeout=60)
            except subprocess.TimeoutExpired:
                return None
            if o.exists():
                return o
        return None

    def evaluate(self, func: str, wd: Path) -> tuple[int, int, bool] | None:
        """(score, wrong, verified match) or None when it does not build."""
        o = self.build(func, wd)
        if o is None:
            return None
        try:
            words, masks = elf_function(o, self.sym)
        except (KeyError, ValueError, struct.error):
            return None
        pairs = [] if self.alloc else None
        score, wrong = grade(self.target(), words, masks, pairs)
        if score and self.alloc:
            gap, _ = alloc_report(o.parent, self.meta["stem"], pairs)
            score += int(round(self.alloc * gap))
        if score and self.length:
            # a residue that is pure ORDER cannot close at another length, and
            # the alignment grade can prefer a shorter function that aligns well
            score += int(round(self.length * abs(len(words) - len(self.target()))))
        if score == 0:
            kind, w, _, _ = C.score(self._st, self._img, o, self.sym)
            if kind != "match":
                return 1 + w, max(w, 1), False
            return 0, 0, True
        return score, wrong, False


def ee_predefines(drv: list[str]) -> list[str]:
    """The -D/-U arguments the EE driver hands its cpp, in order, read off `-v`."""
    with tempfile.TemporaryDirectory() as d:
        (Path(d) / "t.cpp").write_text("\n")
        r = subprocess.run(drv + ["-E", "-v"] + C.FLAGS + ["t.cpp", "-o", "t.i"],
                           capture_output=True, text=True, cwd=d)
    for line in r.stderr.splitlines():
        if "cpp" in line.lower() and " -D" in line:
            return [t for t in line.split() if t.startswith(("-D", "-U"))]
    raise SystemExit(f"cannot read the EE cpp's predefines off `-v`:\n{r.stderr}")


def _includes() -> list[str]:
    """The TU's own directory, and SUPPORT's headers for another module's TUs
    (the same path `eqcheck.compile_cmd` gives the compiler)."""
    inc = ["-I", str(C.SRC)]
    if E.MODULE != "SUPPORT":
        inc += ["-I", str(C.HERE / "src")]
    return inc


def dump_macros(drv: list[str], src: Path) -> str:
    """Every macro defined at the end of `src`, as `#define` lines.  SN's cpp
    writes NOTHING for `-dM`, to a file or to stdout (measured), so the host's
    cpp reads the same headers with the EE driver's own predefines; the names
    the host still predefines under `-undef` (`__STDC__` and kin) are dropped."""
    pre = ee_predefines(drv)
    cpp = ["cpp", "-x", "c", "-undef", "-nostdinc", "-dM", "-w"]
    own = set(re.findall(r"^#define\s+(\w+)", subprocess.run(
        cpp + ["/dev/null"], capture_output=True, text=True).stdout, re.M))
    ee = {re.match(r"-[DU](\w+)", p).group(1) for p in pre}
    r = subprocess.run(cpp + pre + _includes() + [str(src)],
                       capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(f"dumping the macros failed:\n{r.stderr}")
    keep = [l for l in r.stdout.splitlines()
            if (m := re.match(r"#define\s+(\w+)", l)) and (m.group(1) in ee or m.group(1) not in own)]
    return "\n".join(keep) + "\n"


def native_cpp(body: str, macros: Path, line: int, file: str) -> str:
    """Expand the header macros left in a function, with the host's cpp and
    the macro set `dump_macros` wrote (so `mips` is still 1)."""
    r = subprocess.run(["cpp", "-x", "c", "-P", "-undef", "-nostdinc", "-w",
                        "-imacros", str(macros), "-"],
                       input=f'#line {line} "{file}"\n{body}', capture_output=True,
                       text=True)
    if r.returncode:
        raise RuntimeError(r.stderr)
    return r.stdout


def _cc1(tc: str) -> str:
    hits = sorted((HERE / "toolchains" / tc).glob("lib/gcc-lib/*/*/cc1plus.exe"))
    if not hits:
        raise SystemExit(f"no cc1plus.exe in toolchain {tc}")
    return str(hits[0])


def _strip_file(s: str) -> str:
    return "\n".join(l for l in s.splitlines() if not l.lstrip().startswith(".file"))


def rundir_of(sym: str) -> Path:
    return RUNS / sym


def cmd_setup(a) -> int:
    sym = a.symbol
    stem = a.tu or find_tu(sym)
    if stem in C.TU_OVERRIDE:
        raise SystemExit(f"{stem} is compiled with an override; not supported")
    tu_text = (C.SRC / f"{stem}.cpp").read_text()
    start, end = find_function(tu_text, sym, a.line)
    tb = tu_text.encode()
    pre_text, func_orig = tb[:start].decode(), tb[start:end].decode()

    rd = rundir_of(a.name or sym)
    if rd.exists() and not a.force:
        raise SystemExit(f"{rd} exists; --force to rebuild it")
    if rd.exists():
        shutil.rmtree(rd)
    (rd / "ref").mkdir(parents=True)
    drv = C.driver(C.TOOLCHAIN)

    # the prefix, preprocessed once, and the macros it leaves defined
    (rd / f"{stem}.cpp").write_text(pre_text)
    inc = _includes()
    r = subprocess.run(drv + ["-E"] + C.FLAGS + inc + [f"{stem}.cpp", "-o", "prefix.ii"],
                       capture_output=True, text=True, cwd=rd)
    if r.returncode or not (rd / "prefix.ii").exists():
        raise SystemExit(f"preprocessing the prefix failed:\n{r.stderr}")
    (rd / "macros.h").write_text(dump_macros(drv, rd / f"{stem}.cpp"))
    (rd / f"{stem}.cpp").unlink()

    # the logical line and file name the function starts at
    files = set()
    first = re.search(r'^# \d+ "([^"]*)"', (rd / "prefix.ii").read_text(), re.M)
    if first:
        files.add(first.group(1))
    line, phys = None, pre_text.count("\n") + 1
    for m in re.finditer(r'^#\s*line\s+(\d+)(?:\s+"([^"]*)")?', pre_text, re.M):
        dline = pre_text[:m.start()].count("\n") + 1
        line = int(m.group(1)) + (phys - dline - 1)
        if m.group(2):
            files.add(m.group(2))
    if line is None:
        line = phys
    fname = None
    for m in re.finditer(r'^# \d+ "([^"]*)"', (rd / "prefix.ii").read_text(), re.M):
        if m.group(1) in files:
            fname = m.group(1)
    fname = fname or f"{stem}.cpp"

    macros = local_macros(tu_text, start)
    func = expand_macros(func_orig, macros)
    func = M.strip_comments(func)
    if M.Fn(func).root.has_error:
        print("warning: tree-sitter reports a parse error in the function; "
              "mutations near it will not apply", file=sys.stderr)
    names = set(re.findall(r"^#define\s+([A-Za-z_]\w*)", (rd / "macros.h").read_text(), re.M))
    left = sorted(names & set(re.findall(r"[A-Za-z_]\w*", func)))

    meta = {"symbol": sym, "stem": stem, "line": line, "file": fname,
            "cc1": _cc1(C.TOOLCHAIN), "flags": list(C.FLAGS), "cpp": bool(left),
            "header_macros": left, "local_macros": sorted(macros)}
    (rd / "meta.json").write_text(json.dumps(meta, indent=1) + "\n")
    (rd / "orig.cpp").write_text(func_orig + "\n")
    base = Path(a.start).read_text() if a.start else func
    (rd / "base.cpp").write_text(base)

    # the gate: the unmutated candidate must compile to the driver's assembly
    ref = rd / "ref"
    (ref / f"{stem}.cpp").write_text(tb[:end].decode() + "\n")
    subprocess.run(C.compile_cmd(stem) + inc + [f"{stem}.cpp", "-o", "ref.s"],
                   capture_output=True, cwd=ref)
    ctx = Ctx(rd)
    o = ctx.build(func, rd / "work" / "setup")
    if o is None or not (ref / "ref.s").exists():
        raise SystemExit("the base candidate or the reference does not build")
    same = _strip_file((rd / "work" / "setup" / "c.s").read_text()) == \
        _strip_file((ref / "ref.s").read_text())
    if not same:
        raise SystemExit("the base candidate's assembly differs from the driver's "
                         f"build of the TU (compare {rd}/work/setup/c.s and "
                         f"{ref}/ref.s); the prefix/macro reconstruction is unfaithful")
    print(f"{sym}\n  TU {stem}.cpp, line {line} of {fname}")
    print(f"  local macros expanded: {', '.join(sorted(macros)) or 'none'}; "
          f"header macros left: {', '.join(left) or 'none'}")
    print("  base candidate assembles IDENTICALLY to the driver's build")
    if a.start:
        print(f"  base text taken from {a.start}")
    s, w, ok = ctx.evaluate(base, rd / "work" / "setup")
    print(f"  base score {s}  ({w} wrong)" + ("  MATCH" if ok else ""))
    print(f"  {rd}")
    return 0


# --- the search -------------------------------------------------------------------

def _hash(t: str) -> bytes:
    return hashlib.blake2b(t.encode(), digest_size=12).digest()


def _elites(rd: Path) -> list:
    try:
        return json.loads((rd / "elites.json").read_text())
    except (OSError, ValueError):
        return []


def scratch(rd: Path, name: str) -> Path:
    """A worker's build directory: tmpfs when there is one -- a dump-enabled
    build writes 700 KB, twenty-five times a second."""
    shm = Path("/dev/shm")
    if shm.is_dir() and os.access(shm, os.W_OK):
        return shm / "eqperm" / rd.name / name
    return rd / "work" / name


def worker(wid, rd, start_text, start_score, q, stop, best, seed, temp, anchored, alloc,
           length=0.0):
    """One search process.  An ANCHORED worker never drifts: every candidate is
    one to three mutations from the start text, so the neighbourhood the search
    began in keeps being sampled after the others have fallen into a basin."""
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    rng = random.Random(seed)
    ctx = Ctx(rd, alloc, length)
    wd = scratch(rd, f"w{wid}")
    cur, cur_s, cur_trail = start_text, start_score, []
    seen = set()
    iters = errs = dups = 0
    stale = 0
    last = time.time()
    while not stop.is_set():
        if anchored:
            cur, cur_s, cur_trail = start_text, start_score, []
        elif stale > 400 or rng.random() < 0.003:
            el = _elites(rd)
            if el and rng.random() < 0.85:
                k = min(int(rng.expovariate(0.5)), len(el) - 1)
                cur, cur_s, cur_trail = el[k]["text"], el[k]["score"], el[k]["trail"]
            else:
                cur, cur_s, cur_trail = start_text, start_score, []
            stale = 0
        cand, names = cur, []
        n = 1
        while rng.random() < 0.45 and n < (3 if anchored else 6):
            n += 1
        for _ in range(n):
            new, nm = M.mutate(cand, rng)
            if new:
                cand = new
                names.append(nm)
        if not names:
            continue
        h = _hash(cand)
        if h in seen:
            dups += 1
            continue
        seen.add(h)
        res = ctx.evaluate(cand, wd)
        iters += 1
        if res is None:
            errs += 1
            stale += 1
        else:
            s, wrong, ok = res
            trail = (cur_trail + names)[-60:]
            # everything below the start is worth keeping, not only new bests:
            # the distinct improving moves are the map a person reads
            if s <= best.value or s < start_score:
                q.put(("cand", wid, s, wrong, ok, cand, trail))
                with best.get_lock():
                    if s < best.value:
                        best.value = s
            if s < cur_s or (s == cur_s and rng.random() < 0.5) or \
                    (s > cur_s and rng.random() < math.exp(-(s - cur_s) / temp)):
                stale = 0 if s < cur_s else stale + 1
                cur, cur_s, cur_trail = cand, s, trail
            else:
                stale += 1
        if time.time() - last > 10:
            q.put(("stat", wid, iters, errs, dups))
            iters = errs = dups = 0
            last = time.time()
    q.put(("stat", wid, iters, errs, dups))


def _diverse(elites: dict, n: int) -> list:
    """The best n elites, at most three per score: a pool of sixteen copies
    of one basin restarts every worker into that basin."""
    out, per = [], collections.Counter()
    for h, e in sorted(elites.items(), key=lambda kv: kv[1]["score"]):
        if per[e["score"]] < 3:
            per[e["score"]] += 1
            out.append((h, e))
            if len(out) == n:
                break
    return out


def cmd_run(a) -> int:
    rd = rundir_of(a.symbol)
    if not (rd / "meta.json").exists():
        raise SystemExit(f"no run directory {rd}; run `eqperm.py setup` first")
    ctx = Ctx(rd, a.alloc, a.length)
    if a.anchor is None:
        a.anchor = a.jobs // 3
    start = Path(a.start).read_text() if a.start else (rd / "base.cpp").read_text()
    res = ctx.evaluate(start, rd / "work" / "main")
    if res is None:
        raise SystemExit("the start text does not build")
    s0, w0, ok0 = res
    found = rd / "found"
    found.mkdir(exist_ok=True)
    log = open(rd / "log.txt", "a")

    def say(msg):
        line = time.strftime("%H:%M:%S ") + msg
        print(line, flush=True)
        log.write(line + "\n")
        log.flush()

    say(f"start {a.symbol}: score {s0} ({w0} wrong), {a.jobs} workers "
        f"({a.anchor} anchored), {a.minutes or 'no'} minute limit, temp {a.temp}"
        + (f", allocation-order weight {a.alloc}" if a.alloc else "")
        + (f", length weight {a.length}" if a.length else ""))
    best = mp.Value("i", s0)
    q = mp.Queue()
    stop = mp.Event()
    seed = a.seed if a.seed is not None else int(time.time())
    # workers 0..anchor-1 stay at the start; the rest anneal, at spread temperatures
    procs = [mp.Process(target=worker, args=(k, rd, start, s0, q, stop, best,
                                             seed * 1000 + k,
                                             a.temp * (1, 2, 4, 8)[k % 4],
                                             k < a.anchor, a.alloc, a.length),
                             daemon=True)
             for k in range(a.jobs)]
    for p in procs:
        p.start()

    halt = {"now": False}

    def on_signal(*_):
        halt["now"] = True
    signal.signal(signal.SIGTERM, on_signal)
    signal.signal(signal.SIGINT, on_signal)

    elites: dict[bytes, dict] = {_hash(start): {"score": s0, "text": start, "trail": []}}
    saved: dict[bytes, int] = {}
    per_score: collections.Counter = collections.Counter()
    # serials continue from what an earlier run left, so nothing is overwritten
    best_s, n_saved = s0, len(list(found.glob("*.cpp")))
    tot = collections.Counter()
    t0 = last_report = time.time()
    deadline = t0 + a.minutes * 60 if a.minutes else None
    while not halt["now"] and (deadline is None or time.time() < deadline):
        try:
            msg = q.get(timeout=1.0)
        except Exception:
            msg = None
        if msg is not None and msg[0] == "stat":
            _, _, it, er, du = msg
            tot["iters"] += it
            tot["errs"] += er
            tot["dups"] += du
        elif msg is not None:
            _, wid, s, wrong, ok, text, trail = msg
            h = _hash(text)
            if h not in saved and (s < best_s or per_score[s] < a.keep):
                n_saved += 1
                saved[h] = s
                per_score[s] += 1
                tag = "MATCH_" if ok else ""
                f = found / f"{tag}{s:06d}_{wrong:03d}_{n_saved:05d}.cpp"
                f.write_text(f"// score {s}, {wrong} wrong, worker {wid}\n"
                             f"// trail: {' '.join(trail)}\n{text}")
                if s < best_s:
                    best_s = s
                    shutil.copy(f, rd / "best.cpp")
                    say(f"NEW BEST {s} ({wrong} wrong){'  ** MATCH **' if ok else ''}"
                        f"  -> {f.name}")
                if ok and a.stop_on_match:
                    halt["now"] = True
            elites[h] = {"score": s, "text": text, "trail": trail}
            if len(elites) > 4 * a.elites:
                elites = dict(_diverse(elites, a.elites))
        if time.time() - last_report > 30:
            last_report = time.time()
            top = [e for _, e in _diverse(elites, a.elites)]
            tmp = rd / "elites.json.tmp"
            tmp.write_text(json.dumps(top))
            os.replace(tmp, rd / "elites.json")
            el = time.time() - t0
            say(f"{tot['iters']} built ({tot['iters'] / el:.1f}/s), "
                f"{tot['errs']} failed, {tot['dups']} repeats; best {best_s}")
    stop.set()
    for p in procs:
        p.join(timeout=20)
    for p in procs:
        if p.is_alive():
            p.terminate()
    while True:
        try:
            msg = q.get_nowait()
        except Exception:
            break
        if msg[0] == "stat":
            tot["iters"] += msg[2]
            tot["errs"] += msg[3]
    say(f"stopped: {tot['iters']} built, best {best_s}")
    return 0


def cmd_score(a) -> int:
    rd = rundir_of(a.symbol)
    ctx = Ctx(rd, a.alloc or 1.0)
    wd = rd / "work" / "score"
    res = ctx.evaluate(_body(Path(a.file)), wd)
    if res is None:
        print("does not build")
        return 1
    s, w, ok = res
    o = rd / "work" / "score" / "c.o"
    kind, wrong, tot, res = C.score(ctx._st, ctx._img, o, ctx.sym)
    print(f"score {s} ({w} wrong by alignment); eqcheck: {kind} {wrong}/{tot}")
    words, masks = elf_function(o, ctx.sym)
    pairs: list = []
    grade(ctx.target(), words, masks, pairs)
    gap, bad = alloc_report(wd, ctx.meta["stem"], pairs)
    print(f"allocation order: gap {gap:.4f} over {len(bad)} inverted pair(s)")
    for A, B, pa, pb, ra, rb, wa, wb in bad:
        print(f"  pseudo {A} (prio {pa:.4f}, got ${ra}, target ${wa}) went before "
              f"{B} (prio {pb:.4f}, got ${rb}, target ${wb})")
    if a.diff and res is not None and not res.ok:
        print("\n".join(res.text))
    return 0


def _body(path: Path) -> str:
    """A found file's function text, without eqperm's two header lines."""
    return "".join(l for l in path.read_text().splitlines(True)
                   if not l.startswith(("// score", "// trail")))


def _revert(bl: list, cl: list, hunks: list) -> str:
    """cl with the given hunks (opcodes against bl) put back to bl's text."""
    out, j = [], 0
    for tag, i1, i2, j1, j2 in sorted(hunks, key=lambda h: h[3]):
        out += cl[j:j1] + bl[i1:i2]
        j = j2
    return "".join(out + cl[j:])


def reduce_text(ctx: Ctx, base: str, cand: str, wd: Path, say=print,
                pair_cap: int = 40) -> tuple[str, int]:
    """Revert every hunk of cand (against base) that its score does not need.
    Greedy, one hunk at a time; a revert is kept when the score does not rise.
    When no single hunk goes, PAIRS are tried: an added `{` and its `}` land in
    two hunks, and either one alone does not build."""
    res = ctx.evaluate(cand, wd)
    if res is None:
        raise SystemExit("the candidate does not build")
    best = res[0]
    bl = base.splitlines(True)
    while True:
        cl = cand.splitlines(True)
        hunks = [h for h in difflib.SequenceMatcher(None, bl, cl, autojunk=False).get_opcodes()
                 if h[0] != "equal"]
        tries = [[h] for h in hunks]
        if len(hunks) <= pair_cap:
            tries += [[a, b] for k, a in enumerate(hunks) for b in hunks[k + 1:]]
            # a brace pair around an edited line is three hunks, all adjacent
            tries += [hunks[k:k + n] for n in range(3, len(hunks) + 1)
                      for k in range(len(hunks) - n + 1)]
        for t in tries:
            trial = _revert(bl, cl, t)
            r = ctx.evaluate(trial, wd)
            if r is not None and r[0] <= best:
                cand, best = trial, r[0]
                say(f"  reverted {len(t)} hunk(s) at base line(s) "
                    f"{', '.join(str(h[1] + 1) for h in t)}: score {best}")
                break
        else:
            return cand, best


def cmd_reduce(a) -> int:
    rd = rundir_of(a.symbol)
    ctx = Ctx(rd)
    base = (rd / "base.cpp").read_text()
    src = Path(a.file)
    cand, s = reduce_text(ctx, base, _body(src), rd / "work" / "reduce")
    out = src.with_name(src.stem + ".reduced.cpp")
    out.write_text(cand)
    print(f"score {s} with {out.name}:")
    sys.stdout.writelines(difflib.unified_diff(base.splitlines(True), cand.splitlines(True),
                                               "base", out.name, n=2))
    return 0


def cmd_best(a) -> int:
    rd = rundir_of(a.symbol)
    base = (rd / "base.cpp").read_text().splitlines(True)
    files = sorted((rd / "found").glob("*.cpp"),
                   key=lambda p: (not p.name.startswith("MATCH"), p.name.lstrip("MATCH_")))
    for f in files[:a.n]:
        lines = f.read_text().splitlines(True)
        head = [l for l in lines[:2] if l.startswith("//")]
        body = lines[len(head):]
        print(f"===== {f.name}  {head[0].strip() if head else ''}")
        sys.stdout.writelines(difflib.unified_diff(base, body, "base", f.name, n=2))
    return 0


def _best_find(rd: Path) -> Path | None:
    """The run's best saved candidate: a verified MATCH first, then the lowest score."""
    files = [f for f in (rd / "found").glob("*.cpp") if ".reduced" not in f.name] \
        if (rd / "found").is_dir() else []
    files.sort(key=lambda p: (not p.name.startswith("MATCH"), p.name.removeprefix("MATCH_")))
    return files[0] if files else None


def _eqcheck_of(out: str) -> str:
    m = re.search(r"eqcheck: (\w+) (\d+)/(\d+)", out)
    return f"{m.group(1)} {m.group(2)}/{m.group(3)}" if m else "?"


def cmd_batch(a) -> int:
    """Searches back to back, unattended: each symbol in the list is set up
    (unless its run directory exists) and searched for its minutes; then every
    run's best find is reduced, several at once, and one summary is written.
    A list line is `SYMBOL [--tu STEM] [--line N] [--name RUN] [--minutes M]`;
    `#` starts a comment.
    SIGTERM stops the current search cleanly and ends the batch."""
    me = [sys.executable, str(Path(__file__).resolve())]
    lp = argparse.ArgumentParser(prog="list line")
    lp.add_argument("symbol")
    lp.add_argument("--tu")
    lp.add_argument("--line", type=int)
    lp.add_argument("--name")
    lp.add_argument("--minutes", type=float)
    items = [lp.parse_args(l.split()) for l in Path(a.listfile).read_text().splitlines()
             if l.strip() and not l.lstrip().startswith("#")]
    stamp = time.strftime("%Y%m%d-%H%M")
    logf = open(RUNS / f"batch-{stamp}.log", "a")

    def say(msg):
        line = time.strftime("%H:%M:%S ") + msg
        print(line, flush=True)
        logf.write(line + "\n")
        logf.flush()

    state = {"child": None, "stop": False}

    def on_term(signum, frame):
        state["stop"] = True
        if state["child"] and state["child"].poll() is None:
            state["child"].send_signal(signal.SIGTERM)
    signal.signal(signal.SIGTERM, on_term)

    total = sum(it.minutes or a.minutes for it in items)
    say(f"batch of {len(items)}, {total:.0f} search minutes, -j {a.jobs}, alloc {a.alloc}, "
        f"length {a.length}, anchor {a.anchor}")
    rows = []
    for k, it in enumerate(items):
        if state["stop"]:
            break
        name = it.name or it.symbol
        rd = rundir_of(name)
        row = {"symbol": it.symbol, "name": name, "rd": rd}
        rows.append(row)
        if not (rd / "meta.json").exists():
            cmd = me + ["setup", it.symbol, "--name", name] + \
                (["--tu", it.tu] if it.tu else []) + (["--line", str(it.line)] if it.line else [])
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode:
                row["note"] = "setup failed: " + (r.stderr or r.stdout).strip().splitlines()[-1]
                say(f"{it.symbol}: {row['note']}")
                continue
        r = subprocess.run(me + ["score", name, str(rd / "base.cpp")],
                           capture_output=True, text=True)
        row["base"] = _eqcheck_of(r.stdout)
        minutes = it.minutes or a.minutes
        say(f"[{k + 1}/{len(items)}] {name}: base {row['base']}, {minutes:g} min")
        cmd = me + ["run", name, "-j", str(a.jobs), "--anchor", str(a.anchor),
                    "--alloc", str(a.alloc), "--length", str(a.length), "--keep", "5",
                    "--minutes", str(minutes), "--seed", str(a.seed * 1000 + k),
                    "--stop-on-match"]
        with open(rd / f"batch-{stamp}.out", "w") as out:
            state["child"] = subprocess.Popen(cmd, stdout=out, stderr=subprocess.STDOUT)
            state["child"].wait()
            state["child"] = None
        log = (rd / "log.txt").read_text().splitlines()
        tail = [l for l in log if "stopped:" in l]
        row["run"] = tail[-1].split("stopped: ", 1)[1] if tail else "?"
        starts = [int(m.group(1)) for l in log if (m := re.search(r" start .*: score (\d+)", l))]
        best = _best_find(rd)
        # a run also keeps candidates that only TIE the start; those are not finds
        if best and starts and not best.name.startswith("MATCH") and \
                int(best.name.split("_", 1)[0]) >= starts[-1]:
            best = None
        row["best"] = best
        say(f"    {row['run']}; best file {best.name if best else '(no improvement)'}")

    if not a.no_reduce and not state["stop"]:
        todo = [row for row in rows if row.get("best")]
        say(f"reducing {len(todo)} finds, {a.jobs} at a time")
        running = []
        while todo or running:
            while todo and len(running) < a.jobs:
                row = todo.pop(0)
                f = open(row["rd"] / f"batch-{stamp}.reduce", "w")
                p = subprocess.Popen(me + ["reduce", row["name"], str(row["best"])],
                                     stdout=f, stderr=subprocess.STDOUT)
                running.append((p, row, f))
            time.sleep(2)
            for p, row, f in list(running):
                if p.poll() is None:
                    continue
                running.remove((p, row, f))
                f.close()
                red = row["best"].with_name(row["best"].stem + ".reduced.cpp")
                if red.exists():
                    r = subprocess.run(me + ["score", row["name"], str(red), "--alloc",
                                             str(a.alloc)], capture_output=True, text=True)
                    row["reduced"], row["red"] = _eqcheck_of(r.stdout), red
                else:
                    row["reduced"] = "reduce failed"
                say(f"    reduced {row['name']}: {row['reduced']}")

    out = RUNS / f"batch-{stamp}.md"
    with open(out, "w") as f:
        f.write(f"# eqperm batch {stamp}\n\n| symbol | base | search | reduced find | file |\n"
                "|---|---|---|---|---|\n")
        for row in rows:
            red = row.get("red")
            f.write(f"| `{row['name']}` | {row.get('base', '-')} | "
                    f"{row.get('note') or row.get('run', '-')} | {row.get('reduced', '-')} | "
                    f"{red.relative_to(RUNS) if red else '-'} |\n")
    say(f"summary: {out}")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("setup")
    s.add_argument("symbol")
    s.add_argument("--tu")
    s.add_argument("--line", type=int)
    s.add_argument("--start", help="a function text to search from instead of the TU's")
    s.add_argument("--name", help="run directory name (default: the symbol), so one "
                   "symbol can be searched from several start texts")
    s.add_argument("--force", action="store_true")
    r = sub.add_parser("run")
    r.add_argument("symbol")
    r.add_argument("-j", "--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    r.add_argument("--minutes", type=float, default=0)
    r.add_argument("--start", help="search from this function text, not base.cpp")
    r.add_argument("--seed", type=int)
    r.add_argument("--temp", type=float, default=5.0,
                   help="how readily a worse candidate is kept (simulated annealing)")
    r.add_argument("--keep", type=int, default=20, help="files kept per equal score")
    r.add_argument("--elites", type=int, default=16)
    r.add_argument("--alloc", type=float, default=0.0,
                   help="weight of the allocation-order term (0 = off; try 200)")
    r.add_argument("--length", type=float, default=0.0,
                   help="add W per word of instruction-count difference from the target "
                        "(for a residue that is pure scheduling order)")
    r.add_argument("--anchor", type=int, default=None,
                   help="workers that never drift from the start (default: a third)")
    r.add_argument("--stop-on-match", action="store_true")
    c = sub.add_parser("score")
    c.add_argument("symbol")
    c.add_argument("file")
    c.add_argument("--diff", action="store_true", help="print eqdiff's report")
    c.add_argument("--alloc", type=float, default=0.0,
                   help="weight added to the printed score (the report prints regardless)")
    d = sub.add_parser("reduce", help="revert the hunks a find does not need")
    d.add_argument("symbol")
    d.add_argument("file")
    b = sub.add_parser("best")
    b.add_argument("symbol")
    b.add_argument("-n", type=int, default=3)
    t = sub.add_parser("batch", help="several searches back to back, then reduce and summarise")
    t.add_argument("listfile", help="one `SYMBOL [--tu STEM] [--line N] [--name RUN] "
                   "[--minutes M]` a line")
    t.add_argument("-j", "--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    t.add_argument("--minutes", type=float, default=25.0, help="per search, unless the line says")
    t.add_argument("--alloc", type=float, default=500.0)
    t.add_argument("--length", type=float, default=300.0)
    t.add_argument("--anchor", type=int, default=3)
    t.add_argument("--seed", type=int, default=1, help="pinned, so a batch can be replayed")
    t.add_argument("--no-reduce", action="store_true")
    a = p.parse_args(argv)
    return {"setup": cmd_setup, "run": cmd_run, "score": cmd_score,
            "reduce": cmd_reduce, "best": cmd_best, "batch": cmd_batch}[a.cmd](a)


if __name__ == "__main__":
    sys.exit(main())
