#!/usr/bin/env python3
"""eqmodule -- symbol maps for the modules that do not name their own functions.

SUPPORT's map is its own export table.  CLIENT, STATION, SETUP and MCERR export
only their entry points (MCERR six names), so their maps are built here, from
the image, and then kept in the repository as `modules/<name>/symbols.txt`:

  * Functions are found by a linear SWEEP over the code.  A function ends at a
    `jr ra`, or at a tail `j` to another function, plus its delay slot, unless a
    branch inside it reaches further.  Alignment nops between functions are
    skipped.  A `jal` target or an export inside a swept function splits it, which
    is what catches a function that never returns -- and so does a loop that
    never exits: an unconditional branch BACK that no branch reaches past, then
    padding and a prologue, ends the function even when nothing calls the next.
  * Exported names are kept.  The LIBRARY TAIL every module ends with (libgcc,
    soft float, crt: from the first function saving `ra` with `sd` after which no
    function saves it with `sq`) is tagged LIB and named after SUPPORT's copy
    where the bytes agree; it is GPL runtime and is not decompiled.
  * Everything else is `func_<VA>`.  Rename the entry to the function's mangled
    name when you write it: eqcheck finds a target by NAME, so a function cannot
    be scored until its entry carries the name your object defines.
  * A placeholder can carry a HINT, a trailing `# ...` the map parser ignores:
    the source file and line it asserts from, read off this image (`hints`), and
    for STATION and SETUP the old claude-eqoa-iso map's RTTI label.  A hint
    helps you find the TU; it is never taken for the name.

    ./eqmodule.py seed MCERR [--force]    # write modules/mcerr/symbols.txt (CLIENT, STATION, SETUP)
    ./eqmodule.py status [MODULE]         # functions, named, matched (default EQMODULE)
    ./eqmodule.py imports [MODULE]        # the import table, as the relocations use it
    ./eqmodule.py sweepcheck              # the sweep against SUPPORT's real map
"""

from __future__ import annotations

import argparse
import bisect
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E

JR_RA = 0x03E00008


def _w(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def _branch_target(off: int, w: int) -> int | None:
    """Image offset a PC-relative branch at `off` can reach, else None."""
    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    imm = w & 0xFFFF
    rel = off + 4 + ((imm - 0x10000 if imm & 0x8000 else imm) << 2)
    if op in (4, 5, 6, 7, 20, 21, 22, 23):              # beq bne blez bgtz + likely
        return rel
    if op == 1 and rt in (0, 1, 2, 3, 16, 17, 18, 19):  # REGIMM bltz bgez (al/l)
        return rel
    if op in (17, 18) and rs == 8:                      # bc1x / bc2x
        return rel
    return None


def _saves_ra(data: bytes, start: int, end: int) -> str | None:
    """'sq' / 'sd' when the prologue saves ra that way."""
    for off in range(start, min(end, start + 48), 4):
        w = _w(data, off)
        if (w >> 21) & 31 == 29 and (w >> 16) & 31 == 31:
            if w >> 26 == 0x1F:
                return "sq"
            if w >> 26 == 0x3F:
                return "sd"
    return None


def sweep(data: bytes, reach_rule: bool = True, split_calls: bool = True,
          loop_rule: bool = True) -> list[tuple[int, int]]:
    """(start, end) image offsets of every function in the code.  The three
    keywords switch a rule off, for the tests' negative controls."""
    rel = {site: (kind, imp, target) for site, kind, imp, target in E.module_relocations(data)}
    lo, hi = E.code_extent(data)
    calls = {target for site, (kind, imp, target) in rel.items()
             if kind == E.R_JAL and not imp and _w(data, site) >> 26 == 3}
    # SPAN_IGNORE names are exported labels INSIDE a SUPPORT function, not entries
    exports = {v for n, v in E.module_symbols(data) if v and n and n not in E.SPAN_IGNORE}
    starts = calls | exports

    funcs, pos = [], 0x40
    while pos < hi:
        while pos < hi and _w(data, pos) == 0:
            pos += 4
        if pos >= hi:
            break
        start, reach, off = pos, pos, pos
        while off < len(data) - 4:
            w = _w(data, off)
            # a prologue straight after a syscall: the code before it never
            # returned (the `Exit` stub runs into __do_global_dtors)
            if off > start and w >> 16 == 0x27BD and w & 0x8000 and reach <= off \
                    and _w(data, off - 4) & 0xFC00003F == 0x0000000C:
                off -= 8
                break
            t = _branch_target(off, w)
            if t is not None and t > reach:
                reach = t
            tail = False
            if w >> 26 == 2 and off in rel:                 # j: a label, or a tail call
                kind, imp, target = rel[off]
                if imp or target in starts and not start <= target <= off:
                    tail = True
                elif target > reach:
                    reach = target
            if (w == JR_RA or tail) and (reach < off + 8 or not reach_rule):
                break
            # a loop that never returns: an unconditional branch BACK that no
            # branch reaches past, then padding and a prologue -- the code after
            # it is the next function (SETUP +0x22C40 has no caller to split it)
            if loop_rule and (w >> 16 == 0x1000 or w >> 26 == 2 and off in rel and not rel[off][1]) \
                    and reach < off + 8:
                t = _branch_target(off, w) if w >> 16 == 0x1000 else rel[off][2]
                nxt = off + 8
                while nxt < hi and _w(data, nxt) == 0:
                    nxt += 4
                if t <= off and nxt < hi and _w(data, nxt) >> 16 == 0x27BD and _w(data, nxt) & 0x8000:
                    break
            off += 4
        funcs.append([start, off + 8])
        pos = off + 8

    # a call target or an export inside a swept function is a function start:
    # the sweep ran on through something that never returns
    out = []
    for start, end in funcs:
        cut = sorted(s for s in starts if start < s < end) if split_calls else []
        for s in cut:
            out.append((start, s))
            start = s
        out.append((start, end))
    return out


def _masked(data: bytes, start: int, end: int, rel: dict) -> bytes:
    """The bytes with every relocated field zeroed, so two copies of one
    function at different addresses compare equal."""
    field = {E.R_WORD: 0xFFFFFFFF, E.R_JAL: 0x03FFFFFF, E.R_HI16: 0xFFFF, E.R_LO16: 0xFFFF}
    b = bytearray(data[start:end])
    for off in range(start, end, 4):
        if off in rel:
            k = off - start
            w = _w(b, k) & ~field[rel[off]]
            struct.pack_into("<I", b, k, w & 0xFFFFFFFF)
    return E.trim_padding(bytes(b))


def _has_syscall(data: bytes, start: int, end: int) -> bool:
    return any(_w(data, o) & 0xFC00003F == 0x0000000C for o in range(start, end, 4))


def library_start(data: bytes, funcs: list[tuple[int, int]]) -> int:
    """Image offset where the library tail begins (len(data) if none): the first
    function after the last one saving ra with `sq` (SN's 2.95.3) that saves it
    with `sd` (the libgcc build) or makes a syscall (crt0, in a module linked as a
    program, as MCERR is)."""
    style = [_saves_ra(data, s, e) for s, e in funcs]
    last_sq = max((i for i, st in enumerate(style) if st == "sq"), default=-1)
    first = next((i for i in range(last_sq + 1, len(funcs))
                  if style[i] == "sd" or _has_syscall(data, *funcs[i])), None)
    return funcs[first][0] if first is not None else len(data)


def support_library_names() -> dict[bytes, str]:
    """SUPPORT's library tail, masked bytes -> name."""
    mod, st, img = E.load("SUPPORT")
    rel = {site: kind for site, kind, _, _ in E.module_relocations(img.data)}
    funcs = sweep(img.data)
    lib = library_start(img.data, funcs)
    seen: dict[bytes, tuple[int, str] | None] = {}
    ends = dict(funcs)
    for s in st.symbols:
        off = s.addr - mod.base
        if off >= lib and off in ends and s.name != "ENTRYPOINT":
            key = _masked(img.data, off, ends[off], rel)
            # two functions at different addresses with the same masked bytes
            # name neither; two names at one address are one function
            if key not in seen:
                seen[key] = (s.addr, s.name)
            elif seen[key] is not None and seen[key][0] != s.addr:
                seen[key] = None
    return {k: v[1] for k, v in seen.items() if v}


ASSERTS = ("_VIAssert", "_VIAssertf", "_VIAlert")
_SOURCE = re.compile(r"^[A-Za-z]:[/\\].*\.(?:cpp|c|h|hpp)$")


def _li(w: int, rt: int) -> int | None:
    """N for `addiu rt,zero,N` / `ori rt,zero,N`, else None."""
    if w >> 26 in (9, 0x0D) and (w >> 21) & 31 == 0 and (w >> 16) & 31 == rt:
        v = w & 0xFFFF
        return v - 0x10000 if w >> 26 == 9 and v & 0x8000 else v
    return None


def assert_sites(data: bytes, funcs: list[tuple[int, int]]) -> list[tuple[int, int, str | None, int | None]]:
    """(site, function start, source file, line) for every call to an assert.

    The file is the nearest LO16 before the call -- delay slot first, since gcc
    schedules `addiu a1,a1,%lo(file)` there -- whose string is a source path; it
    is read off the RELOCATION, so a `lui` hoisted into another register out of a
    loop still names it.  `_VIAssert` takes the line in a2; `_VIAssertf` and
    `_VIAlert` store it to the global `__VIAssertf_line` instead."""
    rel = {site: (kind, imp, target) for site, kind, imp, target in E.module_relocations(data)}
    names = E.module_imports(data)
    starts = [s for s, _ in funcs]
    calls = sorted(s for s, (k, imp, _) in rel.items()
                   if k == E.R_JAL and imp and names.get(imp) in ASSERTS)
    out = []
    for n, site in enumerate(calls):
        fn = starts[bisect.bisect_right(starts, site) - 1]
        prev = calls[n - 1] + 8 if n and calls[n - 1] >= fn else fn
        # this call's own setup lies between the previous assert and the call; a
        # file held in a saved register from earlier in the function is the fallback
        src = None
        for lo in (prev, fn):
            for off in range(site + 4, lo - 4, -4):
                r = rel.get(off)
                if r and r[0] == E.R_LO16 and not r[1] and _SOURCE.match(E._cstr(data, r[2])):
                    src = E._cstr(data, r[2]).replace("\\", "/").rsplit("/", 1)[-1]
                    break
            if src:
                break
        line = None
        if names[rel[site][1]] == "_VIAssert":
            line = next((v for off in range(site + 4, prev - 4, -4)
                         if (v := _li(_w(data, off), 6)) is not None), None)
        else:
            for off in range(site + 4, prev - 4, -4):
                r = rel.get(off)
                if r and r[1] and names.get(r[1]) == "__VIAssertf_line" and _w(data, off) >> 26 == 0x2B:
                    rt = (_w(data, off) >> 16) & 31
                    line = next((v for p in range(off, prev - 4, -4)
                                 if (v := _li(_w(data, p), rt)) is not None), None)
                    break
        out.append((site, fn, src, line))
    return out


# the earlier RE maps, claude-eqoa-iso's, made before this sweep existed
OLD_MAP = "claude-eqoa-iso/decrypted_modules/{}_symbol_map.txt"
_OLD_ROW = re.compile(r"^0x([0-9A-Fa-f]+)\s+\[([^\]]*)\]\s+(\S+)\s*$")
_OLD_PLACEHOLDER = re.compile(r"^(?:sub|station_func|setup_func|func)_[0-9A-Fa-f]+$")


def hints(mod, data: bytes, funcs: list[tuple[int, int]]) -> tuple[dict[int, list[str]], dict[str, int]]:
    """{function start: [hint, ...]} and the counts behind them.

      VIEntity.cpp:476   the function asserts from that file and line -- a FACT
                         (the lowest line, when it asserts more than once)
      VIEntity.cpp~      between the first and last function asserting from that
                         .cpp, so probably in it -- INFERRED (a header's asserts
                         land in every TU that inlines it, so headers make no span)
      old:<label>        the old RE map's label for it (STATION/SETUP: RTTI and
                         vtable guesses), unverified

    Hints are COMMENTS: the name stays a placeholder, and eqcheck never sees them.
    The old map's labels are NOT carried for CLIENT: they are file:line anchors
    made on its own jal-target boundaries, which put 27 of its 74 on the wrong
    function or the wrong file -- the asserts are re-anchored here instead.  A
    label inside a swept function is kept only where it is the prologue of a
    function gcc scheduled up to three instructions above (`li v0,-1`, `lui`);
    the rest are jump-table case labels its vtable pass took for methods."""
    out: dict[int, list[str]] = {}
    starts = [s for s, _ in funcs]
    count = {"assert": 0, "span": 0, "old": 0, "old_shifted": 0, "old_dropped": 0}
    by_fn: dict[int, list[tuple[str, int | None]]] = {}
    for site, fn, src, line in assert_sites(data, funcs):
        if src:
            by_fn.setdefault(fn, []).append((src, line))
    for fn, ss in by_fn.items():
        src = max(set(s for s, _ in ss), key=[s for s, _ in ss].count)
        lines = [ln for s, ln in ss if s == src and ln is not None]
        out.setdefault(fn, []).append(f"{src}:{min(lines)}" if lines else src)
        count["assert"] += 1
    span: dict[str, list[int]] = {}
    for fn, ss in by_fn.items():
        for s, _ in ss:
            if s.endswith(".cpp"):
                span.setdefault(s, []).append(fn)
    for src, fns in span.items():
        for fn in starts[bisect.bisect_left(starts, min(fns)):bisect.bisect_right(starts, max(fns))]:
            if fn not in by_fn:
                out.setdefault(fn, []).append(f"{src}~")
                count["span"] += 1
    old = E.ROOT / OLD_MAP.format(mod.name)
    if mod.name == "CLIENT" or not old.is_file():
        return out, count
    region = mod.name + "_CODE"
    owner = {s: e for s, e in funcs}
    for line in old.read_text(errors="replace").splitlines():
        m = _OLD_ROW.match(line)
        if not m or m.group(2).strip() != region or _OLD_PLACEHOLDER.match(m.group(3)):
            continue
        off = int(m.group(1), 16) - mod.base
        fn = off
        if fn not in owner:
            i = bisect.bisect_right(starts, off) - 1
            fn = starts[i] if i >= 0 else None
            if not (fn is not None and off - fn <= 12 and _w(data, off) >> 16 == 0x27BD
                    and _w(data, off) & 0x8000):
                count["old_dropped"] += 1
                continue
            count["old_shifted"] += 1
        out.setdefault(fn, []).append(f"old:{m.group(3)}")
        count["old"] += 1
    return out, count


def seed(name: str, force: bool) -> int:
    mod = E.MODULES[name]
    if mod.name == "SUPPORT":
        raise SystemExit("SUPPORT's map is its export table; nothing to seed")
    if mod.symbols.exists() and not force:
        raise SystemExit(f"{mod.symbols} exists and is curated by hand; --force overwrites it")
    data = mod.image.read_bytes()
    funcs = sweep(data)
    lib = library_start(data, funcs)
    rel = {site: kind for site, kind, _, _ in E.module_relocations(data)}
    exports = {v: n for n, v in E.module_symbols(data) if v and n}
    libnames = support_library_names()
    hint, hcount = hints(mod, data, funcs)
    rows, named_lib, used = [], 0, set()
    for start, end in funcs:
        va = mod.base + start
        if start >= lib:
            nm = libnames.get(_masked(data, start, end, rel))
            if nm in used:
                nm = None
            used.add(nm)
            named_lib += nm is not None
            rows.append((va, "LIB", nm or f"func_{va:08X}"))
        else:
            rows.append((va, "CODE", exports.get(start, f"func_{va:08X}")))
    # every data object a local relocation points at gets a placeholder, so a
    # transcript reads `data_<VA>+off` instead of an offset from the code's end
    code_end = funcs[-1][1]
    data_at = sorted({target for _, kind, imp, target in E.module_relocations(data)
                      if not imp and target >= code_end})
    rows += [(mod.base + t, "DATA", f"data_{mod.base + t:08X}") for t in data_at]
    end_va = mod.base + code_end
    own = sum(1 for r in rows if r[1] == "CODE")
    mod.symbols.parent.mkdir(parents=True, exist_ok=True)
    with open(mod.symbols, "w") as f:
        nlib = sum(1 for r in rows if r[1] == "LIB")
        f.write(f"# {mod.name} symbol map: {own} functions of its own + {nlib} library "
                f"(LIB) + {len(data_at)} data objects, base 0x{mod.base:08X}\n")
        f.write("# Seeded by `eqmodule.py seed`, then kept BY HAND: rename a func_ entry to\n"
                "# the mangled name your object defines when you write that function.\n"
                "# LIB is libgcc/soft-float/crt (GPL runtime), not decompiled.\n")
        if hint:
            f.write("# A trailing `# ...` is a HINT, never a name (eqmodule.hints): `File.cpp:N`\n"
                    "# the function asserts from there (a fact), `File.cpp~` it sits between two\n"
                    "# that do (inferred), `old:<label>` the old claude-eqoa-iso map's guess.\n"
                    f"# {hcount['assert']} assert-anchored, {hcount['span']} by span, "
                    f"{hcount['old']} old labels ({hcount['old_shifted']} moved up to the "
                    f"real start, {hcount['old_dropped']} jump-table labels dropped).\n")
        for va, tag, nm in sorted(rows + [(end_va, "END", f"{mod.name.lower()}_code_end")]):
            h = [x for x in hint.get(va - mod.base, []) if x != f"old:{nm}"] if tag == "CODE" else None
            f.write(f"0x{va:08X}  [{tag}]  {nm}" + (f"  # {'; '.join(h)}" if h else "") + "\n")
    print(f"{mod.symbols}: {own} functions, {sum(1 for r in rows if r[1] == 'LIB')} library "
          f"({named_lib} named after SUPPORT's copy), {len(data_at)} data objects, "
          f"code ends +{code_end:#x}")
    if hint:
        print(f"  hints: {hcount['assert']} assert-anchored, {hcount['span']} by span, "
              f"{hcount['old']} old labels ({hcount['old_shifted']} moved to the real start, "
              f"{hcount['old_dropped']} jump-table labels dropped)")
    return 0


def status(name: str) -> int:
    mod, st, img = E.load(name)
    src = HERE / "src" if mod.name == "SUPPORT" else HERE / "modules" / mod.name.lower() / "src"
    matched = set((src / "MATCHED.txt").read_text().split()) if (src / "MATCHED.txt").exists() else set()
    own = [s for s in st.sized if s.tag.strip() == "CODE"]
    if mod.name == "SUPPORT":
        own = [s for s in st.sized]
    size = lambda ss: sum(len(img.func(s, trim=True)) for s in ss)
    named = [s for s in own if not E.is_placeholder(s.name)]
    done = [s for s in own if s.name in matched]
    print(f"{mod.name}: {len(own)} functions ({size(own)} bytes), "
          f"{len(named)} named, {len(done)} byte-exact ({size(done)} bytes)")
    return 0


def imports(name: str) -> int:
    mod, st, img = E.load(name)
    use: dict[int, int] = {}
    for _, _, imp, _ in E.module_relocations(img.data):
        if imp:
            use[imp] = use.get(imp, 0) + 1
    for k, nm in E.module_imports(img.data).items():
        print(f"{k:4d}  {use[k]:4d}x  {nm}")
    return 0


def sweepcheck() -> int:
    """The sweep against the one module whose map is real.  Every named SUPPORT
    code symbol should be a sweep start; sweep starts the map lacks are its
    unnamed statics and template copies (eqlocal's territory)."""
    mod, st, img = E.load("SUPPORT")
    funcs = sweep(img.data)
    starts = {mod.base + s for s, _ in funcs}
    lib = mod.base + library_start(img.data, funcs)
    # VIRInit / VIBbInit sit at +0x0 / +0x8, inside the module header: labels, not code
    code = [s for s in st.sized if mod.base + 0x40 <= s.addr < lib and img.func(s, trim=True)]
    missed = [s for s in code if s.addr not in starts and s.name not in E.SPAN_IGNORE
              and s.name not in E.SPAN_ALIAS]
    extra = len(starts - {s.addr for s in st.symbols})
    print(f"sweep: {len(funcs)} functions; SUPPORT map: {len(code)} named code symbols, "
          f"{len(code) - len(missed)} found, {len(missed)} missed; "
          f"{extra} sweep starts the map does not name; library tail at {lib:#010x}")
    for s in missed[:20]:
        print(f"  missed {s.addr:#010x} {s.name}")
    return 1 if missed else 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("seed"); s.add_argument("module"); s.add_argument("--force", action="store_true")
    s = sub.add_parser("status"); s.add_argument("module", nargs="?", default=E.MODULE)
    s = sub.add_parser("imports"); s.add_argument("module", nargs="?", default=E.MODULE)
    sub.add_parser("sweepcheck")
    a = p.parse_args(argv)
    if a.cmd == "seed":
        return seed(a.module.upper(), a.force)
    if a.cmd == "status":
        return status(a.module.upper())
    if a.cmd == "imports":
        return imports(a.module.upper())
    return sweepcheck()


if __name__ == "__main__":
    sys.exit(main())
