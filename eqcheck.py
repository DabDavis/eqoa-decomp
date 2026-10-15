#!/usr/bin/env python3
"""
eqcheck -- build the decomp source tree and score every function in it.

This is the Phase 3 driver.  `eqsweep.py` searches for one function's source;
this one is the regression harness over everything already found: it compiles
`src/*.cpp` with the settled toolchain and diffs every global function symbol
in the objects against the target image.

  ./eqcheck.py                 # score everything
  ./eqcheck.py -f Rotate       # only symbols matching a regex
  ./eqcheck.py --wrong Sca     # and show the instruction diff for mismatches

A symbol that is in the source but NOT in the target image is reported as
`no-target`, never skipped silently -- a typo'd mangled name would otherwise
read as "nothing to do".  Likewise a function that fails to build is a loud
row, not an absent one.  MATCHED.txt records what is currently byte-exact;
`--baseline` rewrites it and a regression against it is a non-zero exit, so a
source change made for function N cannot quietly break function N-1.

A `wrong` row counts differing instructions POSITIONALLY while the candidate is
the target's length, and by DIFF ALIGNMENT (marked `~`) once it is not -- one
extra instruction shifts every later word, and a positional count then reads as
"everything wrong" for every variant alike.

A second total follows the byte-exact one: VERIFIED DUPLICATES, map entries this
tree does not compile whose relocated bytes (every call and data target included)
equal a function it matches.  gcc 2.95 had no COMDAT, so each TU kept its own copy
of every template member it used; CLIENT holds hundreds of such dead copies.  They
are credited to the source of the function they duplicate, counted apart, listed
with `--dups`, and never baselined.  A copy under four instructions with no
relocation is "too small to attribute": `jr ra; move v0,zero` is every function
that returns 0.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E
import eqdiff as D

# SUPPORT's source is src/; every other module's is modules/<name>/src, with its
# own build/ and MATCHED.txt, so two modules never share a scorer's directory.
if E.MODULE == "SUPPORT":
    SRC, BUILD = HERE / "src", HERE / "build"
else:
    SRC = HERE / "modules" / E.MODULE.lower() / "src"
    BUILD = HERE / "modules" / E.MODULE.lower() / "build"
MATCHED = SRC / "MATCHED.txt"

# Settled in Phase 1/2: ee-gcc 2.95.x at -O2.  -G0 follows from the fingerprint
# (zero gp-relative accesses in 219,910 instructions).
#
# Phase 3: the ASSEMBLER is SN Systems' ps2eeas, NOT the GNU `as` bundled with
# the 2.95 packages, and the VERSION of ps2eeas matters too.
#
#  * GNU as vs ps2eeas: after `mtc1` writes an FPR, an R5900 COP1 instruction
#    reading it needs one slot.  gcc emits `#nop` -- a COMMENT -- and leaves the
#    insertion to the assembler.  Every GNU as in toolchains/ drops it; every
#    ps2eeas inserts it.  Both ConvertToEuler functions are one instruction
#    short under GNU as and byte-exact under ps2eeas.
#  * ps2eeas 1.9.25.758 (in ee-gcc2.9-991111 and 2.95.3-136) auto-pads a `div`
#    that follows a branch delay slot with two nops -- the R5900 DIV errata,
#    `-Wdivbug-on` reports it.  The target has NO padding there, so the build
#    predates that fix.  The 2.95.3-114 copy is the one that agrees.
#    (2.95.2-273a's 1.8.19.316 does not pad either, but it breaks Transpose and
#    Debug, so it is not the one.)
TOOLCHAIN = "ee-gcc2.95.3-107"
ASSEMBLER = HERE / "toolchains/ee-gcc2.95.3-114/lib/gcc-lib/ee/2.95.3/ps2eeas.exe"
#
# Phase 5: -fno-exceptions.  VIQuat::ConvertTo has a stack VIMatrix44, which has
# a destructor, so with exceptions ON gcc 2.95 emits a cleanup region -- three
# copies of the `_$_10VIMatrix44(this, 2)` call reached by `b`.  The target has
# exactly one.  Nothing else in the tree has a local with a non-trivial dtor, so
# this is invisible until a function does.  RTTI stays ON: the module carries
# __rtti / __tf<n> symbols.
FLAGS = ["-O2", "-G0", "-fno-exceptions"]


# Phase 61: the one TU the original did NOT build with the settled compiler.
# libgcc's L__main object came prebuilt in the SDK's libgcc.a, compiled as C by
# the ee-gcc 2.9 (990721/991111) family: `__do_global_dtors` is byte-exact only
# there, at -O2 -G0 (2.95.3 gives 18 wrong as C, 19 as C++; 2.9 saves with `sd`
# into 16-byte slots where 2.95 uses `sq`).  Swept over every toolchain on disk,
# eight flag sets and both languages.  Pinned per file -- toolchain, flags and
# language -- and so immune to --flags/--toolchain.
TU_OVERRIDE = {"libgcc2_main": ("ee-gcc2.9-990721", ["-O2", "-G0", "-x", "c"])}


def compile_cmd(stem: str, tc: str | None = None, flags: list[str] | None = None) -> list[str]:
    """The `-S` command for src/<stem>.cpp, up to the source path."""
    if stem in TU_OVERRIDE and E.MODULE == "SUPPORT":
        tc, flags = TU_OVERRIDE[stem]
    cmd = driver(tc or TOOLCHAIN) + ["-S"] + list(FLAGS if flags is None else flags)
    if E.MODULE != "SUPPORT":
        # another module's classes derive from SUPPORT's (VIWnd, VIUIWindow), so
        # its TUs include SUPPORT's headers
        cmd.append("-I" + str(HERE / "src"))
    return cmd


def driver(name: str):
    d = HERE / "toolchains" / name
    for c in ("bin/ee-gcc", "bin/ee-gcc.exe", "ee/bin/gcc"):
        p = d / c
        if p.is_file():
            return ["wine", str(p)] if p.suffix == ".exe" else [str(p)]
    raise SystemExit(f"no driver in toolchain {name}")


def build(tc: str, flags: list[str], gnu_as: bool = False) -> tuple[list[Path], str]:
    BUILD.mkdir(exist_ok=True)
    srcs = sorted(SRC.glob("*.cpp"))
    if not srcs:
        raise SystemExit(f"no sources in {SRC}")
    objs, err = [], []
    for s in srcs:
        o = BUILD / (s.stem + ".o")
        if o.exists():
            o.unlink()
        objs.append(o)
        if gnu_as:
            r = subprocess.run(driver(tc) + ["-c"] + flags + [str(s), "-o", str(o)],
                               capture_output=True, text=True, cwd=BUILD)
            err.append(r.stderr)
            continue
        asm = BUILD / (s.stem + ".s")
        r = subprocess.run(compile_cmd(s.stem, tc, flags) + [str(s), "-o", str(asm)],
                           capture_output=True, text=True, cwd=BUILD)
        err.append(r.stderr)
        if asm.exists():
            # The assembler runs under wine, which fails spuriously often enough
            # to have produced a whole phantom "regression" -- eight functions
            # in one TU, all of them still byte-exact when that TU was rebuilt
            # on its own.  One retry; a second failure is still reported.
            for _ in range(2):
                r = subprocess.run(["wine", str(ASSEMBLER), "-o", str(o), str(asm)],
                                   capture_output=True, text=True, cwd=BUILD)
                if o.exists():
                    break
            err.append(r.stderr)
    return objs, "\n".join(x.strip() for x in err if x and x.strip())


def obj_locals(obj: Path) -> set[str]:
    """The object's LOCAL function symbols -- per-TU names (a static, the TU's
    `__static_initialization_and_destruction_0`) that another TU may reuse for a
    different function."""
    out = subprocess.run([E._find_objdump(), "-t", str(obj)],
                         capture_output=True, text=True).stdout
    # 00000000 l     F .text  000000b4 connection_init__FP12connection_t
    return {m.group(1) for m in re.finditer(
        r"^[0-9a-f]+\s+l\s+F\s+\S*\.text\S*\s+[0-9a-f]+\s+(\S+)$", out, re.M)}


def obj_functions(obj: Path) -> list[str]:
    """Global function symbols defined in a .text section of the object."""
    out = subprocess.run([E._find_objdump(), "-t", str(obj)],
                         capture_output=True, text=True).stdout
    names = []
    for line in out.splitlines():
        # 00000000 g     F .text  0000003c Scale__10VIMatrix44f
        m = re.match(r"^[0-9a-f]+\s+(\S*)\s+F\s+(\S+)\s+[0-9a-f]+\s+(\S+)", line)
        if m and ".text" in m.group(2):
            names.append(m.group(3))
    return names


def masked_body(obj: Path, symbol: str) -> bytes:
    """The function's bytes with every field its own relocations name zeroed."""
    data, sec, at = D.elf_function_at(obj, symbol)
    b = bytearray(data)
    for off, typ, _ in D.elf_relocations(obj, sec):
        if at <= off < at + len(data):
            k = off - at
            w = int.from_bytes(b[k:k + 4], "little") & ~D.RELOC_FIELD.get(typ, 0)
            b[k:k + 4] = (w & 0xFFFFFFFF).to_bytes(4, "little")
    return bytes(b)


_CALLEE = {"dump": None, "syms": {}}


def callee_mismatches(st, img, obj: Path, symbol: str, sym) -> list[tuple[int, str, str]]:
    """(offset, target callee, our callee) for every jal that names a different
    function from the target's -- empty when they agree.  The target's callees
    come from the module's relocated copy (`eqcalls.relocated`), which no longer
    needs a memory dump; an unnamed or placeholder callee is never a mismatch."""
    import eqcalls as K
    import eqcallcheck as CC
    if _CALLEE["dump"] is None:
        _CALLEE["dump"] = K.relocated(img)
    key = (str(obj), obj.stat().st_mtime_ns)
    syms = _CALLEE["syms"].get(key)
    if syms is None:
        syms = _CALLEE["syms"][key] = CC.obj_symbols(obj)
    want = {va - sym.addr: n for va, kind, n in K.calls(st, img, _CALLEE["dump"], sym)
            if kind in ("jal", "j")}
    have = CC.candidate_calls(obj, symbol, syms)
    return [(off, t, have[off]) for off, t in sorted(want.items())
            if off in have and "+0x" not in t and not t.startswith("0x")
            and not E.is_placeholder(t) and have[off] != t]


def score(st, img, obj: Path, symbol: str):
    """('match'|'wrong'|'no-target', wrong, total, diffreport)"""
    try:
        sym = st.get(symbol)
    except KeyError:
        return ("no-target", 0, 0, None)
    target = img.func(sym, trim=True)
    cand, sec, at = D.elf_function_at(obj, symbol)
    # the object's own relocation table says which fields the linker writes;
    # the image is linked and the object is not, so those words can only agree
    # by accident.  Masked sites are counted and shown, never hidden.
    relocs = D.elf_relocations(obj, sec)
    res = D.diff(target, cand, symbol, sym.addr, modes={"j26"},
                 relocs=relocs, span=(at, at + len(cand)))
    if res.ok:
        # Byte-exact is not enough: the masked jal fields hide WHICH function
        # is called, and phase 61 found 84 "exact" functions calling the wrong
        # one.  The relocated dump names the target's callees (eqcallcheck).
        bad = callee_mismatches(st, img, obj, symbol, sym)
        if bad:
            res.callee = bad
            return ("callee", len(bad), res.insns_target, res)
        return ("match", 0, res.insns_target, res)
    a = [w for _, w, _ in E.disasm(target, sym.addr)]
    b = [w for _, w, _ in E.disasm(cand, sym.addr)]
    same = sum(1 for x, y in zip(a, b) if x == y)
    wrong = max(len(a), len(b)) - same
    # The positional count is worthless once the candidate's length drifts: one
    # extra instruction shifts every later word and reports as "everything
    # wrong", so every source variant scores the same and a sweep cannot
    # discriminate.  When the lengths differ, count by DIFF ALIGNMENT instead --
    # the target lines the unified diff could not pair up.  (Measured on
    # VICollBuffer::CalcBVolumes: four source shapes all scored 180/258
    # positionally while the aligned count ran 64 -> 54 -> 27.)
    if len(a) != len(b):
        wrong = max(sum(1 for l in res.text if l[:1] == "-" and l[:3] != "---"),
                    sum(1 for l in res.text if l[:1] == "+" and l[:3] != "+++"))
    return ("wrong", wrong, max(len(a), len(b)), res)


def verified_duplicates(st, img, matched: list[str], scored: set[str]):
    """Map entries this tree does not compile whose bytes ARE a matched function's.

    gcc 2.95 has no COMDAT: every TU that used a template (or an out-of-line inline)
    emitted its own copy, and the linker kept them all and resolved each call to one.
    A copy is byte-identical to the one this tree matches, call and data targets
    included -- the image is compared RELOCATED, so two copies only agree if every
    jal and every address they carry agree too.  Counted separately from the
    byte-exact total and never baselined: a duplicate is credited to the source of the
    function it duplicates, and only while that function matches.

    A copy too small to attribute (under four instructions, with no relocation in it)
    is reported apart: `jr ra; move v0,zero` is every function that returns 0.
    Returns ({duplicate name: matched name}, {ambiguous name: matched name}).
    """
    import eqcalls
    rel = eqcalls.relocated(img)
    sites = {img.module.base + site for site, _, _, _ in E.module_relocations(img.data)}

    def body(sym):
        n = len(img.func(sym, trim=True))
        return bytes(rel[sym.addr:sym.addr + n])

    canon = {}
    for name in matched:
        try:
            sym = st.get(name)
        except KeyError:
            continue
        canon.setdefault(body(sym), name)
    dups, ambiguous = {}, {}
    for sym in st.sized:
        if sym.name in scored or not img.covers(sym.addr, sym.span):
            continue
        b = body(sym)
        if b not in canon:
            continue
        attributable = len(b) >= 16 or any(sym.addr <= x < sym.addr + len(b) for x in sites)
        (dups if attributable else ambiguous)[sym.name] = canon[b]
    return dups, ambiguous


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("-f", "--filter", default="", help="regex over symbol names")
    p.add_argument("-w", "--wrong", action="store_true", help="show diffs for mismatches")
    p.add_argument("-t", "--toolchain", default=TOOLCHAIN)
    p.add_argument("--flags", default=" ".join(FLAGS))
    p.add_argument("--baseline", action="store_true", help="rewrite MATCHED.txt")
    p.add_argument("--dups", action="store_true",
                   help="list the verified duplicates (map entries byte-identical to a matched function)")
    p.add_argument("--gnu-as", action="store_true",
                   help="assemble with the toolchain's GNU as instead of Ps2EeAs")
    a = p.parse_args(argv)

    objs, err = build(a.toolchain, a.flags.split(), a.gnu_as)
    if err:
        print(err, file=sys.stderr)
    _, st, img = E.load()

    rx = re.compile(a.filter) if a.filter else None
    rows, matched = [], []
    # gcc 2.x emits __tf<class> into EVERY TU that needs that class's RTTI, so
    # __tf7VISound and __tf10VIResource are defined by all three sound TUs.
    # One target function scored three times would inflate the total by two, so
    # a repeat is scored once -- and if two TUs disagree about the bytes, that
    # is a loud row, not a silent pick.
    seen: set[str] = set()
    seen_body: dict[str, bytes] = {}
    for o in objs:
        if not o.exists():
            print(f"BUILD FAILED  {o.stem}", file=sys.stderr)
            rows.append(("build-failed", 0, 0, o.stem, None))
            continue
        local = obj_locals(o)
        for name in obj_functions(o):
            if rx and not rx.search(name):
                continue
            if name in local:
                key = f"{o.stem}:{name}"      # a TU-local name is not shared
                if key in seen:
                    continue
                seen.add(key)
                kind, wrong, tot, res = score(st, img, o, name)
                rows.append((kind, wrong, tot, name, res))
                if kind == "match":
                    matched.append(name)
                continue
            if name in seen:
                # the same target function, defined twice; only report it if
                # the two definitions actually differ -- with each object's own
                # relocation fields masked, since two TUs' copies of one weak
                # function (`__tf7VISound`) differ in their addends and nothing else
                if masked_body(o, name) != seen_body[name]:
                    rows.append(("duplicate", 0, 0, name, None))
                continue
            seen.add(name)
            seen_body[name] = masked_body(o, name)
            kind, wrong, tot, res = score(st, img, o, name)
            rows.append((kind, wrong, tot, name, res))
            if kind == "match":
                matched.append(name)

    rows.sort(key=lambda r: (r[0] != "match", r[1], r[3]))
    ok = sum(1 for r in rows if r[0] == "match")
    for kind, wrong, tot, name, res in rows:
        if kind == "match":
            note = ""
            if res is not None and res.reloc is not None and res.reloc.masked:
                note = f"   [{res.reloc}]"
            print(f"  MATCH   {tot:>4}i  {name}{note}")
        elif kind == "no-target":
            print(f"  NO-TGT        {name}   (not a symbol in {E.MODULE})")
        elif kind == "build-failed":
            print(f"  BUILD FAILED  {name}")
        elif kind == "duplicate":
            print(f"  DUPLICATE     {name}   (two TUs emit different bytes)")
        elif kind == "callee":
            calls = ", ".join(f"+{o:#x} {c} != {t}" for o, t, c in res.callee)
            print(f"  CALLEE  {wrong:>3}/{tot:<3}  {name}   (bytes exact; {calls})")
        else:
            # `~` marks a length drift, so the count is diff-aligned not positional
            drift = "~" if res is not None and res.cand_len != res.target_len else " "
            print(f"  wrong{drift}{wrong:>3}/{tot:<3}   {name}")
    print(f"\n{ok}/{len(rows)} byte-exact")
    if not rx:
        dups, ambiguous = verified_duplicates(st, img, matched, {r[3] for r in rows})
        print(f"+ {len(dups)} verified duplicates (byte-identical to a matched function, call and data"
              f" targets included; counted apart){f', {len(ambiguous)} too small to attribute' if ambiguous else ''}")
        if a.dups:
            by = {}
            for d, m in dups.items():
                by.setdefault(m, []).append(d)
            for m in sorted(by):
                print(f"  {len(by[m]):4d} x {m}")

    if a.wrong:
        for kind, wrong, tot, name, res in rows:
            if kind == "wrong":
                print("\n" + "=" * 72)
                print(D.report(res))

    prev = set(MATCHED.read_text().split()) if MATCHED.exists() else set()
    if a.baseline:
        MATCHED.write_text("\n".join(sorted(matched)) + "\n")
        print(f"baseline written: {len(matched)} symbols")
        return 0
    lost = sorted(prev - set(matched) - ({n for _, _, _, n, _ in rows} - {r[3] for r in rows}))
    lost = [n for n in sorted(prev - set(matched)) if rx is None or rx.search(n)]
    if lost:
        print("\nREGRESSION -- these were byte-exact and are not now:")
        for n in lost:
            print(f"  {n}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
