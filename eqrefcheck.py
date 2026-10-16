#!/usr/bin/env python3
"""
eqrefcheck -- does a byte-exact function reference the RIGHT data?

eqcallcheck.py's companion for DATA.  The scorer masks the 16-bit field of
every R_MIPS_HI16/R_MIPS_LO16 the candidate's own relocation table names, so a
`lui`+`addiu` against "foo" matches a `lui`+`addiu` against "bar", a `lwc1`
of 1.0f matches a `lwc1` of 0.5f, and a `lw` of one static matches a `lw` of
another.  The relocated in-world dump (eqcalls.py) holds every target
reference resolved, so each one can be read back and compared.

    ./eqrefcheck.py                  # every TU in build/, matched functions only
    ./eqrefcheck.py -a               # open functions too (same-length ones;
                                     #  sites whose opcode/registers differ skip)
    ./eqrefcheck.py viscene vizone   # those TUs only
    ./eqrefcheck.py -v               # also list unnamed/skipped sites

Builds nothing: it reads build/*.o as the last eqcheck/_tu run left them.

How a site is lined up.  WHICH words are relocations, and against WHAT, comes
from our object's relocation table only.  The HI16/LO16 PAIRING cannot come
from table order: SN's ps2eeas writes HI16 entries out of order (drdp.o lists
the HI16 at 0x3850 seventeen entries after the LO16 it feeds), emits TWO HI16s
for one LO16 when a `lui` sits in a branch-likely delay slot, and several LO16s
after one HI16 when a `lui` is shared.  So each LO16 is paired with the HI16
sites that reach it by register data flow -- a backward walk over the
function's control-flow graph to the `lui` that last wrote the LO16's base
register, which must be a HI16 site against the same symbol (see pair_hilo).
Our address is symbol + ((hi << 16) + sext(lo)).  The TARGET address is the
same (hi, lo) offsets read out of the relocated dump -- the function is
byte-exact, so the register flow is the same.  eqcalls.calls' own register
pairing over the dump is run as an independent cross-check; a disagreement is
a finding (PAIRING CROSS-CHECK DISAGREES), and sites it cannot pair are counted.

What is compared, by what OUR reloc is against:
  .rodata, string   (addiu)  -> C string text, ours vs the dump's
  .rodata, a load   (lwc1/lw/ld/lq/...) -> the loaded bytes (float VALUE)
  .rodata, jump table (R_MIPS_32 vs .text at the addend) -> every entry,
                               as an offset within the function
  .rodata, other data (addiu) -> the bytes up to the next item, our relocated
                               words masked, trailing zeros taken as padding
  a symbol the map names     -> the address exactly (symbol AND offset)
  .text / a function         -> the function name
  anything else (.data/.bss statics, globals the map does not name)
                             -> UNNAMED, and fed to a CONSISTENCY check: one of
                               our objects must resolve to one target object
                               everywhere (SPLIT otherwise), and two of ours
                               must not resolve to the same one (MERGED).
                               An initialised .data object is also compared
                               byte for byte with the un-relocated image.
  pointer words INSIDE a referenced object (an R_MIPS_32 in a .data/.rodata
  table, a vtable defined in this TU) -> resolved on both sides like a site:
                               a string by text, a function or a map symbol
                               by name (vtable slot order, string tables).

Controls (2026-10-14): "lightning.tga"->"lightnine.tga" in VILightningFxInit,
-500->-499 in g_aErrnoConstants, RStrip3Offset->RStrip4Offset in
VIPrimBuffer::End -- all three still scored byte-exact, all three reported.
"""

from __future__ import annotations

import bisect
import struct
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E
import eqcheck as C
import eqcalls as K

R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16 = 2, 4, 5, 6
SHT_NOBITS, SHT_REL, SHT_RELA, SHT_SYMTAB = 8, 9, 4, 2
SHF_EXEC = 4
STT_FUNC, STT_SECTION = 2, 3

# opcode -> bytes the instruction loads or stores (None: not a plain access)
ACCESS = {32: 1, 36: 1, 40: 1,            # lb lbu sb
          33: 2, 37: 2, 41: 2,            # lh lhu sh
          35: 4, 39: 4, 43: 4, 49: 4, 57: 4,   # lw lwu sw lwc1 swc1
          55: 8, 63: 8,                   # ld sd
          30: 16, 31: 16}                 # lq sq
LOADS = {32, 36, 33, 37, 35, 39, 49, 55, 30}
ADDR_OPS = {9, 13}                        # addiu ori: the address itself
OPNAME = {9: "addiu", 13: "ori", 32: "lb", 36: "lbu", 40: "sb", 33: "lh",
          37: "lhu", 41: "sh", 35: "lw", 39: "lwu", 43: "sw", 49: "lwc1",
          57: "swc1", 55: "ld", 63: "sd", 30: "lq", 31: "sq", 34: "lwl",
          38: "lwr", 26: "ldl", 27: "ldr"}

TEXT_CHARS = set(range(0x20, 0x7F)) | {9, 10, 13} | set(range(0xA0, 0x100))


# --- the candidate object ----------------------------------------------------

class Obj:
    """A minimal ELF32 little-endian REL reader: sections, symbols, relocs."""

    def __init__(self, path: Path):
        self.path = path
        b = path.read_bytes()
        if b[:4] != b"\x7fELF" or b[4] != 1 or b[5] != 1:
            raise ValueError(f"{path}: not an ELF32 LE object")
        shoff, = struct.unpack_from("<I", b, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", b, 0x2E)
        hdrs = [struct.unpack_from("<10I", b, shoff + i * shentsize) for i in range(shnum)]
        strtab = hdrs[shstrndx]
        names = b[strtab[4]:strtab[4] + strtab[5]]

        def cstr(buf, o):
            return buf[o:buf.index(b"\0", o)].decode("latin-1")

        self.secs = []
        for h in hdrs:
            name = cstr(names, h[0])
            data = bytes(h[5]) if h[1] == SHT_NOBITS else b[h[4]:h[4] + h[5]]
            self.secs.append({"name": name, "type": h[1], "flags": h[2],
                              "size": h[5], "data": data, "link": h[6], "info": h[7]})
        self.syms = []      # (name, value, size, type, bind, shndx)
        for h in hdrs:
            if h[1] != SHT_SYMTAB:
                continue
            ss = hdrs[h[6]]
            sst = b[ss[4]:ss[4] + ss[5]]
            for i in range(h[5] // 16):
                n, v, sz, info, _o, ndx = struct.unpack_from("<IIIBBH", b, h[4] + i * 16)
                self.syms.append((cstr(sst, n), v, sz, info & 15, info >> 4, ndx))
        # relocations, per TARGET section index, in table order
        self.rels: dict[int, list[tuple[int, int, int, int]]] = defaultdict(list)
        for h in hdrs:
            if h[1] not in (SHT_REL, SHT_RELA):
                continue
            ent = 8 if h[1] == SHT_REL else 12
            for i in range(h[5] // ent):
                off, info = struct.unpack_from("<II", b, h[4] + i * ent)
                add = struct.unpack_from("<i", b, h[4] + i * ent + 8)[0] if ent == 12 else 0
                self.rels[h[7]].append((off, info & 0xFF, info >> 8, add))
        # covering-symbol index per section, for naming a section-relative site
        self._cover: dict[int, tuple[list[int], list]] = {}
        for s in self.syms:
            if s[3] in (STT_SECTION, 4) or s[5] == 0 or s[5] >= 0xFF00 or not s[0]:
                continue
            self._cover.setdefault(s[5], ([], []))
        for ndx in self._cover:
            rows = sorted((s[1], s) for s in self.syms
                          if s[5] == ndx and s[3] not in (STT_SECTION, 4) and s[0])
            self._cover[ndx] = ([r[0] for r in rows], [r[1] for r in rows])
        # R_MIPS_32 sites per data section, offset -> (type, symidx)
        self.word_relocs = {ndx: {off: (typ, si) for off, typ, si, _ in rl}
                            for ndx, rl in self.rels.items()}

    def sec(self, ndx):
        return self.secs[ndx]

    def functions(self):
        """(name, shndx, value, size) for every FUNC in an executable section."""
        out = []
        for name, v, sz, typ, _bind, ndx in self.syms:
            if typ == STT_FUNC and 0 < ndx < len(self.secs) and \
               self.secs[ndx]["flags"] & SHF_EXEC and sz:
                out.append((name, ndx, v, sz))
        return out

    def covering(self, ndx: int, off: int):
        """(name, start, size) of the named symbol covering `off`, or None."""
        if ndx not in self._cover:
            return None
        addrs, rows = self._cover[ndx]
        # in code, a FUNC at exactly `off` beats a label there (gcc2_compiled.)
        if self.secs[ndx]["flags"] & SHF_EXEC:
            for s in rows[bisect.bisect_left(addrs, off):bisect.bisect_right(addrs, off)]:
                if s[3] == STT_FUNC:
                    return s[0], s[1], s[2]
        i = bisect.bisect_right(addrs, off) - 1
        while i >= 0:
            s = rows[i]
            if s[1] == off or (s[1] <= off < s[1] + s[2]):
                return s[0], s[1], s[2]
            if s[1] < off and s[2] == 0:
                i -= 1
                continue
            break
        return None

    def ref_points(self, ndx: int) -> list[int]:
        """Every offset in section `ndx` that anything in the object references
        or names -- the item boundaries a data blob is cut at."""
        pts = {0, self.secs[ndx]["size"]}
        for tsec, rl in self.rels.items():
            groups = pair_hilo(self, tsec) if self.secs[tsec]["flags"] & SHF_EXEC else []
            for g in groups:
                si = g["si"]
                if self.syms[si][5] == ndx:
                    pts.add(self.syms[si][1] + g["addend"])
            for off, typ, si, _ in rl:
                if typ == R_MIPS_32 and self.syms[si][5] == ndx:
                    w = struct.unpack_from("<I", self.secs[tsec]["data"], off)[0]
                    pts.add(self.syms[si][1] + w)
        for s in self.syms:
            if s[5] == ndx and s[3] != STT_SECTION:
                pts.add(s[1])
        return sorted(pts)


def sext16(x: int) -> int:
    return x - 0x10000 if x & 0x8000 else x


CALLER_SAVED = frozenset(range(1, 16)) | {24, 25, 31}


def _branch(w: int):
    """(kind, displacement in words) for a control transfer, else None.
    kind: cond | likely | uncond | jump (absolute) | jr | call"""
    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    imm = sext16(w & 0xFFFF)
    if op in (4, 5, 6, 7):
        return ("uncond" if op == 4 and rs == 0 and rt == 0 else "cond", imm)
    if op in (20, 21, 22, 23):
        return ("likely", imm)
    if op == 1:
        if rt in (0, 1):
            return ("uncond" if rt == 1 and rs == 0 else "cond", imm)
        if rt in (2, 3):
            return ("likely", imm)
        if rt in (16, 17, 18, 19):
            return ("call", None)
    if op in (17, 18) and rs == 8:            # bc1x / bc2x
        return ("likely" if rt & 2 else "cond", imm)
    if op == 2:
        return ("jump", None)
    if op == 3:
        return ("call", None)
    if op == 0 and (w & 63) == 8:
        return ("jr", None)
    if op == 0 and (w & 63) == 9:
        return ("call", None)
    return None


def _writes(w: int) -> frozenset:
    """The general registers an instruction writes.  A call writes only its
    link register HERE: its delay slot runs before the callee, so the
    caller-saved clobber is applied between the return point and the delay
    slot, by the walk."""
    op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    out: set = set()
    if op == 0:
        f = w & 63
        if f == 9:
            out = {rd}
        elif f not in (8, 0x0C, 0x0D, 0x0F, 0x11, 0x13):
            out = {rd}
    elif op == 28:                             # MMI
        out = {rd}
    elif op == 3:
        out = {31}
    elif op == 1 and rt in (16, 17, 18, 19):
        out = {31}
    elif op in (8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 30,
                32, 33, 34, 35, 36, 37, 38, 39, 55):
        out = {rt}
    elif op in (16, 17, 18) and rs in (0, 1, 2):   # mfc / dmfc / cfc
        out = {rt}
    out.discard(0)
    return frozenset(out)


def pair_hilo(obj: Obj, ndx: int):
    """Pair every LO16 in section `ndx` with the HI16(s) that feed it.

    WHICH words are relocations, and against WHAT, comes from the relocation
    table only.  The PAIRING cannot: SN's ps2eeas writes HI16 entries out of
    table order (drdp.o lists the HI16 at 0x3850 seventeen entries after the
    LO16 it feeds), so a LO16 is paired with the HI16 sites that reach it by
    register data flow -- a backward walk over the function's control flow
    (fall-through, branch and branch-likely delay slots, jump-table cases,
    a call clobbering the caller-saved set, `addu` index adds passed through)
    to the `lui` that last wrote the LO16's base register.  That `lui` must
    itself be a HI16 site against the same symbol.

    Returns dicts {lo, his, si, addend, unpaired, why}; `addend` is ours.
    """
    cache = getattr(obj, "_pairs", None)
    if cache is None:
        cache = obj._pairs = {}
    if ndx in cache:
        return cache[ndx]
    data = obj.secs[ndx]["data"]
    rl = obj.rels.get(ndx, [])
    his_all = {off: si for off, typ, si, _ in rl if typ == R_MIPS_HI16}
    los_all = sorted((off, si) for off, typ, si, _ in rl if typ == R_MIPS_LO16)
    # jump-table case labels in this section, from the .rodata tables
    jt = set()
    for dndx, drl in obj.rels.items():
        if obj.secs[dndx]["flags"] & SHF_EXEC:
            continue
        for off, typ, si, _ in drl:
            if typ == R_MIPS_32 and obj.syms[si][5] == ndx:
                jt.add(struct.unpack_from("<I", obj.secs[dndx]["data"], off)[0]
                       + obj.syms[si][1])
    out = []
    orphans = []
    for name, fndx, at, size in obj.functions():
        if fndx != ndx:
            continue
        n = size // 4
        W = list(struct.unpack_from(f"<{n}I", data, at))
        BR = [_branch(x) for x in W]
        preds: list[list[int]] = [[] for _ in range(n)]
        jrs = [i for i in range(n) if BR[i] and BR[i][0] == "jr"
               and (W[i] >> 21) & 31 != 31]
        for i in range(1, n):
            b2 = BR[i - 2] if i >= 2 else None
            if b2 and b2[0] == "likely":
                preds[i].append(i - 2)           # not taken: delay slot nullified
            elif b2 and b2[0] in ("uncond", "jump", "jr"):
                pass                             # no fall-through
            else:
                preds[i].append(i - 1)
        for b in range(n):
            if BR[b] and BR[b][0] in ("cond", "likely", "uncond"):
                t = b + 1 + BR[b][1]
                if 0 <= t < n and b + 1 < n:
                    preds[t].append(b + 1)
        for t_off in jt:
            t = (t_off - at) // 4
            if 0 <= t < n:
                preds[t].extend(j + 1 for j in jrs if j + 1 < n)
        used = set()
        for lo, si in los_all:
            if not (at <= lo < at + size):
                continue
            li = (lo - at) // 4
            r = (W[li] >> 21) & 31
            found, fails = set(), []
            stack = [(p, r, False) for p in preds[li]]
            if not preds[li] and li == 0:
                fails.append("entry")
            seen = set()
            while stack:
                i, reg, alt = stack.pop()
                if (i, reg) in seen:
                    continue
                seen.add((i, reg))
                w = W[i]
                if i >= 1 and BR[i - 1] and BR[i - 1][0] == "call" and reg in CALLER_SAVED:
                    if not alt:                  # walked back across the callee
                        fails.append(f"{reg} clobbered by the call at {at + 4 * (i - 1) - at:#x}")
                    continue
                if reg in _writes(w):
                    off = at + 4 * i
                    if w >> 26 == 15 and off in his_all:
                        if his_all[off] == si:
                            found.add(off)
                        elif not alt:
                            fails.append(f"HI16 against {obj.syms[his_all[off]][0]} at {off - at:#x}")
                        continue
                    if w >> 26 == 0 and (w & 63) in (0x20, 0x21, 0x2D):
                        a_, b_ = (w >> 21) & 31, (w >> 16) & 31
                        nxt = [(reg, alt)] if reg in (a_, b_) else [(a_, True), (b_, True)]
                        for p in preds[i]:
                            for rg, al in nxt:
                                stack.append((p, rg, al))
                        continue
                    if not alt:
                        fails.append(f"{reg} written at {off - at:#x}")
                    continue
                if not preds[i] and i == 0 and not alt:
                    fails.append("entry")        # (no preds and i > 0: dead code)
                for p in preds[i]:
                    stack.append((p, reg, alt))
            his = sorted(found)
            used.update(his)
            hv = {W[(h - at) // 4] & 0xFFFF for h in his}
            hi = min(hv) if hv else 0
            out.append({"lo": lo, "his": his, "si": si,
                        "addend": (hi << 16) + sext16(W[li] & 0xFFFF),
                        "unpaired": not his,
                        "why": ("; ".join(sorted(set(fails))) if fails else "")
                               + (" [our HI16s disagree]" if len(hv) > 1 else "")})
        orphans += [(name, h - at) for h in his_all if at <= h < at + size and h not in used]
    obj._orphans = getattr(obj, "_orphans", {})
    obj._orphans[ndx] = orphans
    cache[ndx] = out
    return out


# --- the target side ---------------------------------------------------------

class Names:
    """Fast address<->name over the SUPPORT symbol map (st.at is a linear scan)."""

    def __init__(self, st):
        self.st = st
        rows = sorted((s.addr, s.name) for s in st.symbols)
        self.addrs = [r[0] for r in rows]
        self.rows = rows
        self.by_name: dict[str, int] = {}
        self.at_addr: dict[int, list[str]] = defaultdict(list)
        for a, n in rows:
            self.by_name.setdefault(n, a)
            self.at_addr[a].append(n)

    def exact(self, addr: int) -> list[str]:
        return self.at_addr.get(addr, [])

    def near(self, addr: int) -> str:
        i = bisect.bisect_right(self.addrs, addr) - 1
        if i < 0:
            return f"{addr:#010x}"
        a, n = self.rows[i]
        return n if a == addr else f"{n}+{addr - a:#x}"

    # duck-types the SymbolTable for eqcalls.name_at
    def at(self, addr: int):
        i = bisect.bisect_right(self.addrs, addr) - 1
        if i < 0:
            return None
        a, n = self.rows[i]
        return type("S", (), {"addr": a, "name": n})


def k_pairs(d: bytes, sym) -> dict[tuple[int, int], int]:
    """eqcalls.calls' register pairing over the dump, returning ADDRESSES:
    (hi va, lo va) -> address.  The same loop, kept independent of ours."""
    out = {}
    hi: dict[int, tuple[int, int]] = {}
    for va in range(sym.addr, sym.end, 4):
        w = K.word(d, va)
        op = w >> 26
        if op == 15:
            hi[(w >> 16) & 0x1F] = (va, w & 0xFFFF)
        elif op in (9, 13, 35, 43, 49, 57):
            rs = (w >> 21) & 0x1F
            if rs in hi:
                hva, h = hi.pop(rs)
                lo = w & 0xFFFF
                if op != 13:
                    lo = sext16(lo)
                out[(hva, va)] = ((h << 16) + lo) & 0xFFFFFFFF
    return out


def cstr_at(buf: bytes, off: int, limit: int = 512) -> str | None:
    """A NUL-terminated text string at `off` (may be empty), or None."""
    if off < 0 or off >= len(buf):
        return None
    end = buf.find(b"\0", off, off + limit)
    if end < 0:
        return None
    body = buf[off:end]
    if any(c not in TEXT_CHARS for c in body):
        return None
    return body.decode("latin-1")


def fmt_bytes(b: bytes, op: int | None) -> str:
    if op == 49 and len(b) == 4:
        return f"{struct.unpack('<f', b)[0]!r}f ({b[::-1].hex()})"
    if len(b) == 8:
        return f"{struct.unpack('<d', b)[0]!r} ({b[::-1].hex()})"
    if len(b) == 4:
        return f"{struct.unpack('<I', b)[0]:#010x} / {struct.unpack('<f', b)[0]!r}f"
    return b.hex()


# --- the check ---------------------------------------------------------------

class Report:
    def __init__(self):
        self.n = defaultdict(int)            # counters
        self.bad = defaultdict(list)         # category -> rows
        self.notes = defaultdict(list)       # verbose: category -> rows
        self.ident = defaultdict(list)       # consistency: key -> [(base, site)]
        self.data_seen = set()

    def bump(self, k, row=None):
        self.n[k] += 1
        if row is not None:
            self.notes[k].append(row)


def check_function(rep: Report, tu: str, obj: Obj, fn, names: Names, st, img, d,
                   allfuncs: bool):
    name, ndx, at, size = fn
    try:
        sym = st.get(name)
    except KeyError:
        rep.bump("fn: no target symbol")
        return
    if sym.span == 0:
        rep.bump("fn: target unsized")
        return
    tlen = len(img.func(sym, trim=True))
    if tlen != size and not allfuncs:
        rep.bump("fn: length differs (skipped)")
        return
    rep.n["functions"] += 1
    text = obj.secs[ndx]["data"]
    ow = lambda o: struct.unpack_from("<I", text, o)[0]
    tw = lambda va: K.word(d, va)
    iw = lambda va: struct.unpack_from("<I", img.read(va, 4))[0]
    kp = None

    for g in pair_hilo(obj, ndx):
        lo = g["lo"]
        if not (at <= lo < at + size):
            continue
        rel = lo - at
        site = f"{tu}:{name}+{rel:#x}"
        rep.n["sites"] += 1
        if rel + 4 > tlen:
            rep.bump("site: beyond target length", site)
            continue
        if g["unpaired"]:
            rep.bump("site: LO16 with no HI16 reaching it", (site, g["why"]))
            continue
        if g["why"]:
            rep.bump("note: some path to the LO16 is not a HI16", (site, g["why"]))
        his = [h for h in g["his"] if at <= h < at + size]
        if not his:
            rep.bump("site: HI16 outside the function", site)
            continue
        # the instruction itself must be the same on both sides
        if any((ow(o) & 0xFFFF0000) != (tw(sym.addr + o - at) & 0xFFFF0000)
               for o in his + [lo]):
            rep.bump("site: opcode/registers differ (skipped)", site)
            continue
        # target: was the lui relocated at all?
        thi = {tw(sym.addr + h - at) & 0xFFFF for h in his}
        if any(tw(sym.addr + h - at) == iw(sym.addr + h - at) for h in his):
            rep.bad["TARGET NOT RELOCATED"].append(
                (site, f"ours is a relocation against {obj.syms[g['si']][0]}"
                       f"+{g['addend']:#x}; the target's lui holds the literal "
                       f"{sorted(thi)}"))
            continue
        if len(thi) != 1:
            rep.bad["TARGET HI16s DISAGREE"].append((site, f"{sorted(thi)}"))
            continue
        lw_ = tw(sym.addr + rel)
        op = lw_ >> 26
        T = ((thi.pop() << 16) + (sext16(lw_ & 0xFFFF) if op != 13 else lw_ & 0xFFFF)) & 0xFFFFFFFF

        # cross-check against the eqcalls-style pairing over the dump
        if kp is None:
            kp = k_pairs(d, sym)
        kv = [kp[(sym.addr + h - at, sym.addr + rel)] for h in his
              if (sym.addr + h - at, sym.addr + rel) in kp]
        if kv:
            if all(v == T for v in kv):
                rep.n["xcheck: agrees"] += 1
            else:
                rep.bad["PAIRING CROSS-CHECK DISAGREES"].append(
                    (site, f"table pairing {T:#x}, eqcalls pairing {[hex(v) for v in kv]}"))
        else:
            rep.n["xcheck: eqcalls pairing has no pair here"] += 1

        compare_site(rep, tu, obj, ndx, at, size, sym, g, op, T, site, names, img, d)


def compare_site(rep, tu, obj, fndx, at, size, sym, g, op, T, site, names, img, d):
    sname, sval, ssize, styp, sbind, sndx = obj.syms[g["si"]]
    A = g["addend"]
    opn = OPNAME.get(op, f"op{op}")
    tname = names.near(T)

    # ---- a symbol the map can place: compare the ADDRESS -------------------
    if styp != STT_SECTION and sname in names.by_name:
        want = names.by_name[sname] + A
        if T == want:
            rep.n["ok: named (address)"] += 1
            # defined HERE (a vtable, a table): its pointer words too, once
            if 0 < sndx < len(obj.secs) and ssize and \
               not obj.secs[sndx]["flags"] & SHF_EXEC and \
               ("named", tu, sname) not in rep.data_seen:
                rep.data_seen.add(("named", tu, sname))
                compare_pointer_words(rep, obj, sndx, sval, sval + ssize, T - A, site,
                                      names, d, sname)
            return
        ex = names.exact(T - A)
        if tname.split("+")[0] == sname or (ex and sname in ex):
            rep.bad["WRONG OFFSET"].append(
                (site, f"{opn}: ours {sname}+{A:#x}, target {tname}"))
        else:
            rep.bad["WRONG GLOBAL"].append(
                (site, f"{opn}: ours {sname}+{A:#x}, target {tname}"
                       + (f' ("{K.string_at(d, T)}")' if K.string_at(d, T) else "")))
        return

    # where our reference lands inside OUR object
    if styp == STT_SECTION or 0 < sndx < len(obj.secs):
        rndx = sndx
        roff = (0 if styp == STT_SECTION else sval) + A
    else:
        rndx = None                      # undefined or .comm, and the map does not know it
        roff = A
    rsec = obj.secs[rndx]["name"] if rndx else None

    # ---- code: a function pointer ------------------------------------------
    if rndx and obj.secs[rndx]["flags"] & SHF_EXEC:
        cov = obj.covering(rndx, roff)
        ours = cov[0] if cov and cov[1] == roff else f"{rsec}+{roff:#x}"
        ex = names.exact(T)
        if ours in ex:
            rep.n["ok: function pointer"] += 1
        elif ex:
            rep.bad["WRONG FUNCTION POINTER"].append((site, f"ours {ours}, target {ex[0]}"))
        elif cov and cov[1] == roff and cov[2]:
            # a TU-local callback the map does not export: compare its CODE
            nd = code_differs(obj, rndx, cov[1], cov[2], img, T)
            if nd == 0:
                rep.n["ok: function pointer (code identical)"] += 1
            else:
                rep.bump("unnamed: function pointer target, code differs",
                         (site, f"ours {ours}, target {tname}: {nd}/{cov[2] // 4} words differ"))
        else:
            rep.bump("unnamed: function pointer target", (site, f"ours {ours}, target {tname}"))
        return

    # ---- .rodata (read-only, so the dump holds the original bytes) ----------
    if rsec and rsec.startswith(".rodata"):
        sec = obj.secs[rndx]
        wr = obj.word_relocs.get(rndx, {})
        # a jump table: R_MIPS_32 entries against .text
        if roff in wr and wr[roff][0] == R_MIPS_32 and \
           obj.syms[wr[roff][1]][5] == fndx:
            compare_jump_table(rep, obj, rndx, roff, fndx, at, size, sym, T, site, d)
            return
        if op in LOADS:
            n = ACCESS[op]
            ours = sec["data"][roff:roff + n]
            theirs = d[T:T + n]
            masked = any(o in wr for o in range(roff - roff % 4, roff + n, 4))
            if masked:
                rep.bump("skipped: load of a relocated word", (site, opn))
                return
            if ours == theirs:
                rep.n["ok: constant"] += 1
            else:
                rep.bad["WRONG CONSTANT"].append(
                    (site, f"{opn}: ours {fmt_bytes(ours, op)}, target {fmt_bytes(theirs, op)}"
                           f"   [ours .rodata+{roff:#x}, target {T:#x}]"))
            return
        if op in ADDR_OPS:
            prev_nul = roff == 0 or sec["data"][roff - 1] == 0
            s_ours = cstr_at(sec["data"], roff) if prev_nul else None
            if s_ours:                      # a non-empty string literal
                s_t = cstr_at(d, T)
                if s_t == s_ours:
                    rep.n["ok: string"] += 1
                else:
                    callee = next_call(obj, fndx, at + int(site.rsplit("+", 1)[1], 16), at + size)
                    rep.bad[string_class(s_ours, s_t, callee)].append(
                        (site, f'ours "{esc(s_ours)}"\n        targ "'
                               + (f'{esc(s_t)}"' if s_t is not None else f'" -- not a string ({tname})')
                               + f"   -> {callee}"))
                return
            compare_blob(rep, obj, rndx, roff, T, site, d, tname)
            return
        rep.bump(f"skipped: {opn} into .rodata", site)
        return

    # ---- everything else: statics, globals the map does not name ------------
    if rndx is None or (styp != STT_SECTION and sbind != 0):
        key, base, ours = ("*", sname), T - A, f"{sname}+{A:#x}"
    else:
        cov = obj.covering(rndx, roff)
        if cov:
            key, base = (tu, rsec, cov[0]), T - (roff - cov[1])
            ours = cov[0] + (f"+{roff - cov[1]:#x}" if roff != cov[1] else "")
        else:
            key, base, ours = (tu, rsec, roff), T, f"{rsec}+{roff:#x}"
    ex = names.exact(base)
    if ex:
        # the map names the target object and does not know ours
        rep.bump("unnamed: ours not in map, target named",
                 (site, f"{opn}: ours {ours}, target {names.near(T)}"))
    else:
        rep.bump("unnamed: neither side in the map", (site, f"{opn}: ours {ours}, target {tname}"))
    # .data initial contents are in the un-relocated image
    if rndx and obj.secs[rndx]["type"] != SHT_NOBITS and op in ACCESS and \
       img.covers(T, ACCESS[op]):
        n = ACCESS[op]
        wr = obj.word_relocs.get(rndx, {})
        if not any(o in wr for o in range(roff - roff % 4, roff + n, 4)):
            a, b = obj.secs[rndx]["data"][roff:roff + n], img.read(T, n)
            rep.n["data: initial value compared"] += 1
            if a != b:
                rep.bad["WRONG INITIAL VALUE (.data)"].append(
                    (site, f"{opn}: ours {ours} = {fmt_bytes(a, op)}, target {tname} = {fmt_bytes(b, op)}"))
    rep.ident[key].append((base, site, ours, tname, opn))
    # a whole initialised object: its contents are in the un-relocated image
    if rndx and cov_obj_ok(obj, rndx) and key not in rep.data_seen:
        cov = obj.covering(rndx, roff) if not (rndx is None) else None
        if cov and cov[2] and img.covers(base, cov[2]):
            rep.data_seen.add(key)
            compare_object(rep, obj, rndx, cov, base, site, names, img)


def code_differs(obj: Obj, ndx: int, start: int, size: int, img, T: int) -> int:
    """Instruction words that differ between our function and the image at T,
    with every field our own relocation table names masked."""
    if not img.covers(T, size):
        return size // 4
    mask = {off: RELOC_MASK.get(typ, 0xFFFFFFFF) for off, typ, _si, _ in obj.rels.get(ndx, [])
            if start <= off < start + size}
    ours = obj.secs[ndx]["data"][start:start + size]
    theirs = img.read(T, size)
    n = 0
    for k in range(0, size, 4):
        m = ~mask.get(start + k, 0) & 0xFFFFFFFF
        a = struct.unpack_from("<I", ours, k)[0] & m
        b = struct.unpack_from("<I", theirs, k)[0] & m
        n += a != b
    return n


RELOC_MASK = {R_MIPS_26: 0x03FFFFFF, R_MIPS_HI16: 0xFFFF, R_MIPS_LO16: 0xFFFF,
              R_MIPS_32: 0xFFFFFFFF}


def cov_obj_ok(obj: Obj, ndx: int) -> bool:
    sec = obj.secs[ndx]
    return sec["type"] != SHT_NOBITS and not sec["flags"] & SHF_EXEC \
        and not sec["name"].startswith(".rodata")


def compare_object(rep, obj, rndx, cov, base, site, names, img):
    """Compare an initialised .data object's bytes with the image, words our
    .rel.data names masked (pointers are un-relocated on both sides)."""
    name, start, size = cov
    data = obj.secs[rndx]["data"][start:start + size]
    theirs = img.read(base, size)
    wr = obj.word_relocs.get(rndx, {})
    diffs = []
    for k in range(size):
        o = start + k
        if (o - o % 4) in wr:
            continue
        if data[k] != theirs[k]:
            diffs.append(k)
    rep.n["data: objects compared"] += 1
    rep.n["data: object bytes compared"] += size
    compare_pointer_words(rep, obj, rndx, start, start + size, base, site, names,
                          rep.dump, name)
    if diffs:
        k0 = diffs[0] - diffs[0] % 4
        rep.bad["WRONG INITIAL VALUE (.data)"].append(
            (site, f"object {name} ({size} bytes) vs target {names.near(base)}: "
                   f"{len(diffs)} bytes differ, first at +{diffs[0]:#x}: "
                   f"ours {data[k0:k0 + 16].hex()} target {theirs[k0:k0 + 16].hex()}"))


def compare_pointer_words(rep, obj, rndx, start, end, base, site, names, d, what):
    """Every R_MIPS_32 word inside [start, end) of our section `rndx` against
    the relocated pointer the dump holds at the same offset from `base`:
    a string by its TEXT, a symbol the map names (or a function, or a vtable
    slot) by NAME.  Jump-table entries are left to compare_jump_table."""
    wr = obj.word_relocs.get(rndx, {})
    sec = obj.secs[rndx]
    for o in range(start - start % 4, end, 4):
        if o not in wr or wr[o][0] != R_MIPS_32:
            continue
        si = wr[o][1]
        pname, pval, _psz, ptyp, _pb, pndx = obj.syms[si]
        add = struct.unpack_from("<I", sec["data"], o)[0]
        theirs = K.word(d, base + (o - start))
        here = f"{what}+{o - start:#x}"
        if 0 < pndx < len(obj.secs):
            tsec = obj.secs[pndx]
            toff = (0 if ptyp == STT_SECTION else pval) + add
            if tsec["flags"] & SHF_EXEC:
                cov = obj.covering(pndx, toff)
                if cov and cov[1] == toff:
                    ex = names.exact(theirs)
                    if not ex:
                        rep.n["ptr: function unnamed in map"] += 1
                    elif cov[0] in ex:
                        rep.n["ok: pointer in data (function)"] += 1
                    else:
                        rep.bad["WRONG POINTER IN DATA"].append(
                            (site, f"{here}: ours -> {cov[0]}, target -> {ex[0]}"))
                else:
                    rep.n["ptr: into code, not a function start (jump table)"] += 1
                continue
            if tsec["name"].startswith(".rodata") and ptyp == STT_SECTION:
                so, stt = cstr_at(tsec["data"], toff), cstr_at(d, theirs)
                if so is not None and (toff == 0 or tsec["data"][toff - 1] == 0):
                    if so == stt:
                        rep.n["ok: pointer in data (string)"] += 1
                    else:
                        rep.bad["WRONG STRING (pointer in data)"].append(
                            (site, f'{here}: ours "{esc(so)}"\n        targ '
                                   + (f'"{esc(stt)}"' if stt is not None else f"{names.near(theirs)}")))
                    continue
            if ptyp == STT_SECTION:
                rep.n["ptr: section-relative data, not compared"] += 1
                continue
        if pname in names.by_name:
            want = names.by_name[pname] + add
            if theirs == want:
                rep.n["ok: pointer in data (named)"] += 1
            else:
                rep.bad["WRONG POINTER IN DATA"].append(
                    (site, f"{here}: ours -> {pname}+{add:#x}, target -> {names.near(theirs)}"))
        else:
            rep.n["ptr: symbol not in map"] += 1


def compare_jump_table(rep, obj, rndx, roff, fndx, at, size, sym, T, site, d):
    sec = obj.secs[rndx]
    wr = obj.word_relocs.get(rndx, {})
    o, k, bad = roff, 0, []
    while o in wr and wr[o][0] == R_MIPS_32 and obj.syms[wr[o][1]][5] == fndx:
        add = struct.unpack_from("<I", sec["data"], o)[0] + obj.syms[wr[o][1]][1]
        ours = add - at
        theirs = K.word(d, T + 4 * k) - sym.addr
        if not (0 <= ours < size):
            break                       # the next table, for another function
        if ours != theirs:
            bad.append(f"[{k}] ours +{ours:#x} target +{theirs:#x}")
        o += 4
        k += 1
    if bad:
        rep.bad["WRONG JUMP TABLE"].append((site, f"{k} entries, {len(bad)} differ: " + "; ".join(bad[:8])))
    else:
        rep.n["ok: jump table"] += 1
        rep.n["ok: jump table entries"] += k


def compare_blob(rep, obj, rndx, roff, T, site, d, tname):
    """A non-string .rodata item taken by address: compare its bytes.

    A named object (a `static const` table has a local symbol with a size) is
    compared whole from `roff` to its end.  Otherwise the item runs to the next
    offset anything in the object references, and trailing zeros past the first
    are taken as alignment padding and not compared."""
    sec = obj.secs[rndx]
    cov = obj.covering(rndx, roff)
    if cov and cov[2]:
        end, named = cov[1] + cov[2], cov[0]
    else:
        pts = getattr(obj, "_pts", {}).get(rndx)
        if pts is None:
            obj._pts = getattr(obj, "_pts", {})
            pts = obj._pts[rndx] = obj.ref_points(rndx)
        i = bisect.bisect_right(pts, roff)
        end, named = (pts[i] if i < len(pts) else sec["size"]), None
    end = min(end, roff + 512)
    blob = bytearray(sec["data"][roff:end])
    theirs = bytearray(d[T:T + len(blob)])
    wr = obj.word_relocs.get(rndx, {})
    for o in range(roff - roff % 4, end, 4):
        if o in wr:
            for j in range(max(o, roff), min(o + 4, end)):
                blob[j - roff] = theirs[j - roff] = 0
    if not blob:
        rep.bump("skipped: empty .rodata item", site)
        return
    n = len(blob)
    if named is None:
        while n > 1 and blob[n - 1] == 0 and blob[n - 2] == 0:
            n -= 1
    if any(o in wr for o in range(roff - roff % 4, end, 4)) and \
       ("ro", id(obj), rndx, roff) not in rep.data_seen:
        rep.data_seen.add(("ro", id(obj), rndx, roff))
        compare_pointer_words(rep, obj, rndx, roff, end, T, site, rep.names, d,
                              named or f".rodata+{roff:#x}")
    if blob[:n] == theirs[:n]:
        rep.n["ok: .rodata data"] += 1
        rep.n["ok: .rodata data bytes"] += n
        return
    s_t = cstr_at(d, T)
    if named is None and bytes(blob[:n]) == b"\0" and s_t:
        rep.bad["WRONG STRING"].append((site, f'ours ""\n        targ "{esc(s_t)}"'))
        return
    what = f"{named} ({n} bytes)" if named else f".rodata+{roff:#x} ({n} bytes)"
    first = next(k for k in range(n) if blob[k] != theirs[k])
    k0 = max(0, first - first % 4)
    to, tt = as_text(blob), as_text(d[T:T + 64])
    rep.bad["WRONG .rodata DATA"].append(
        (site, f"ours {what} vs target {T:#x} ({tname}): first difference at +{first:#x}\n"
               f"        ours {words(blob[k0:k0 + 32])}" + (f"  = {to}" if to else "") +
               f"\n        targ {words(theirs[k0:k0 + 32])}" + (f"  = {tt}" if tt else "")))


def as_text(b: bytes) -> str | None:
    """A NUL-terminated ASCII or UTF-16LE string at the start of `b`, quoted."""
    t = cstr_at(bytes(b), 0, len(b))
    if t and len(t) > 1:
        return f'"{esc(t)}"'
    if len(b) >= 4 and b[1] == 0 and b[3] == 0 and 0x20 <= b[0] < 0x7F:
        u = []
        for k in range(0, len(b) - 1, 2):
            c = b[k] | b[k + 1] << 8
            if c == 0:
                return 'L16"' + esc("".join(u)) + '"'
            if not 0x20 <= c < 0x7F:
                return None
            u.append(chr(c))
    return f'"{esc(t)}"' if t else None


def words(b) -> str:
    """Little-endian words, with the float reading when it looks like one."""
    out = []
    for k in range(0, len(b) - len(b) % 4, 4):
        w = struct.unpack_from("<I", b, k)[0]
        f = struct.unpack_from("<f", b, k)[0]
        out.append(f"{f:g}f" if 0x30000000 <= (w & 0x7FFFFFFF) < 0x50000000 else f"{w:#x}")
    return "[" + ", ".join(out) + "]"


def next_call(obj: Obj, ndx: int, lo: int, end: int) -> str:
    """The callee of the first R_MIPS_26 after `lo` -- what the string feeds."""
    best = None
    for off, typ, si, _ in obj.rels.get(ndx, []):
        if typ == R_MIPS_26 and lo <= off < end and (best is None or off < best[0]):
            best = (off, obj.syms[si][0])
    return best[1] if best else "?"


def is_path(x: str | None) -> bool:
    return bool(x) and ("/" in x or "\\" in x) and x.lower().endswith((".cpp", ".c", ".h"))


def string_class(ours: str, theirs: str | None, callee: str) -> str:
    if is_path(ours) and is_path(theirs):
        return "WRONG STRING: __FILE__"
    if "ssert" in callee:
        return "WRONG STRING: assert expression text"
    return "WRONG STRING"


def esc(s: str) -> str:
    return s.encode("unicode_escape").decode("ascii").replace('"', '\\"')


def consistency(rep: Report):
    """One of our objects -> one target object, and never two of ours -> one."""
    by_base = defaultdict(set)
    for key, rows in rep.ident.items():
        bases = defaultdict(list)
        for base, site, ours, tname, opn in rows:
            bases[base].append((site, ours, tname, opn))
        for b in bases:
            by_base[b].add(key)
        if len(bases) > 1:
            ranked = sorted(bases.items(), key=lambda kv: -len(kv[1]))
            lines = [f"{len(v)} site(s) -> {b:#x} ({v[0][2]}): " +
                     ", ".join(s for s, *_ in v[:6]) + (" ..." if len(v) > 6 else "")
                     for b, v in ranked]
            rep.bad["SPLIT (one of ours -> several targets)"].append((fmt_key(key), "\n        ".join(lines)))
        else:
            rep.n["consistency: object resolves to one target"] += 1
    for b, keys in by_base.items():
        if len(keys) > 1:
            rep.bad["MERGED (several of ours -> one target)"].append(
                (f"{b:#x}", ", ".join(sorted(fmt_key(k) for k in keys))))


def fmt_key(k):
    if k[0] == "*":
        return k[1]
    tu, sec, what = k
    return f"{tu}:{sec}:{what:#x}" if isinstance(what, int) else f"{tu}:{what}"


def main(argv):
    allfuncs = "-a" in argv
    verbose = "-v" in argv
    tus = [a for a in argv if not a.startswith("-")]
    _, st, img = E.load()
    d = K.relocated(img)
    names = Names(st)
    matched = set(C.MATCHED.read_text().split()) if C.MATCHED.exists() else set()
    objs = [C.BUILD / f"{t}.o" for t in tus] if tus else \
           [C.BUILD / f"{s.stem}.o" for s in C.sources()]
    rep = Report()
    rep.names, rep.dump = names, d
    for path in objs:
        if not path.exists():
            print(f"MISSING  {path}", file=sys.stderr)
            rep.n["tu: object missing"] += 1
            continue
        obj = Obj(path)
        rep.n["tus"] += 1
        for fn in obj.functions():
            if not allfuncs and fn[0] not in matched:
                continue
            check_function(rep, path.stem, obj, fn, names, st, img, d, allfuncs)
    consistency(rep)

    order = ["WRONG STRING", "WRONG STRING: assert expression text",
             "WRONG STRING: __FILE__", "WRONG CONSTANT", "WRONG GLOBAL", "WRONG OFFSET",
             "WRONG FUNCTION POINTER", "WRONG JUMP TABLE", "WRONG .rodata DATA",
             "WRONG INITIAL VALUE (.data)", "WRONG STRING (pointer in data)",
             "WRONG POINTER IN DATA", "SPLIT (one of ours -> several targets)",
             "MERGED (several of ours -> one target)", "TARGET NOT RELOCATED",
             "TARGET HI16s DISAGREE", "PAIRING CROSS-CHECK DISAGREES"]
    nbad = 0
    for cat in order + sorted(set(rep.bad) - set(order)):
        rows = rep.bad.get(cat)
        if not rows:
            continue
        print(f"== {cat}: {len(rows)}")
        for site, what in rows:
            print(f"  {site}\n        {what}")
        nbad += len(rows)
        print()
    if verbose:
        for cat, rows in sorted(rep.notes.items()):
            print(f"-- {cat}: {len(rows)}")
            for r in rows:
                print(f"  {r}")
            print()
    print("counts:")
    for k in sorted(rep.n):
        print(f"  {rep.n[k]:7d}  {k}")
    ok = sum(v for k, v in rep.n.items() if k.startswith("ok:")
             and not k.endswith(("entries", "bytes")) and "pointer in data" not in k)
    okp = sum(v for k, v in rep.n.items() if k.startswith("ok: pointer in data"))
    unp = sum(v for k, v in rep.n.items() if k.startswith("ptr:"))
    un = sum(v for k, v in rep.n.items() if k.startswith("unnamed:"))
    sk = sum(v for k, v in rep.n.items() if k.startswith(("skipped:", "site: ")))
    print(f"\n{rep.n['sites']} reference sites in {rep.n['functions']} functions: "
          f"{ok} compared OK, {un} unnamed (consistency-checked), {sk} skipped, "
          f"{nbad} FINDINGS")
    print(f"pointer words inside referenced objects: {okp} compared OK, {unp} not resolvable")
    print(f"pairing cross-check vs eqcalls.calls: {rep.n['xcheck: agrees']} agree, "
          f"{len(rep.bad.get('PAIRING CROSS-CHECK DISAGREES', []))} disagree, "
          f"{rep.n['xcheck: eqcalls pairing has no pair here']} not paired by it")
    return 1 if nbad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
