#!/usr/bin/env python3
"""
Sweep candidate C++ sources x toolchains x flags against one target function.

Matching is a search over three axes at once, and holding two fixed while
tuning the third finds local minima: a source tuned against one compiler can
look best only because the compiler is wrong.  This runs the product and ranks
by instructions wrong.

    ./eqsweep.py variants.py Transpose__10VIMatrix44RC10VIMatrix44

The variants file defines HEADERS, BODIES and optionally TOOLCHAINS / FLAGS.
Each toolchain is driven with `-c` so it uses its OWN period assembler -- never
modern GAS, which expands `move` to `or` where the period one gives `daddu`.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E
import eqdiff as D

TCROOT = HERE / "toolchains"
DEFAULT_FLAGS = ["-O2", "-G0"]
# The settled assembler -- see eqcheck.py.  GNU as drops an R5900 mtc1 hazard
# nop that ps2eeas inserts, and the later ps2eeas pads a post-branch `div` with
# two nops that the target does not have, so a sweep through either one can
# score the RIGHT source as wrong.
ASSEMBLER = HERE / "toolchains/ee-gcc2.95.3-114/lib/gcc-lib/ee/2.95.3/ps2eeas.exe"


def toolchains(names=None):
    out = []
    for d in sorted(TCROOT.glob("ee-gcc*")):
        if names and d.name not in names:
            continue
        drv = None
        for c in ("bin/ee-gcc", "bin/ee-gcc.exe", "ee/bin/gcc"):
            if (d / c).is_file():
                drv = d / c
                break
        if drv:
            out.append((d.name, ["wine", str(drv)] if drv.suffix == ".exe" else [str(drv)]))
    return out


def build_batch(driver, flags, variants, workdir):
    """Compile every variant in ONE driver invocation.

    Process startup dominates -- a wine-hosted driver costs ~1 s to start and
    milliseconds to compile 20 instructions -- so paying it per variant made a
    120-build sweep take minutes.  gcc accepts several sources with -c and
    writes one .o per input, so the whole variant set costs one startup.
    """
    names, srcs = [], []
    for i, (name, source) in enumerate(variants.items()):
        f = workdir / f"v{i}.cpp"
        f.write_text(source)
        o = workdir / f"v{i}.o"
        asm = workdir / f"v{i}.s"
        for stale in (o, asm):
            if stale.exists():
                stale.unlink()
        names.append((name, o, asm))
        srcs.append(str(f))
    # one driver invocation for the whole variant set: process startup (~1 s
    # under wine) dominates, compilation of 20 instructions does not
    subprocess.run(driver + ["-S"] + flags + ["-x", "c++"] + srcs,
                   capture_output=True, cwd=workdir)
    for _, o, asm in names:
        if asm.exists():
            subprocess.run(["wine", str(ASSEMBLER), "-o", str(o), str(asm)],
                           capture_output=True, cwd=workdir)
    return [(n, o if o.exists() else None) for n, o, _ in names]


def score(target: bytes, obj: Path, symbol: str, vma: int):
    """(wrong_instructions, total, matched) or None if the symbol is absent."""
    try:
        cand, sec, at = D.elf_function_at(obj, symbol)
    except Exception:
        return None
    res = D.diff(target, cand, symbol, vma, modes={"j26"},
                 relocs=D.elf_relocations(obj, sec), span=(at, at + len(cand)))
    if res.raw_identical or res.norm_identical:
        return (0, res.insns_target, res.insns_target)
    a = [w for _, w, _ in E.disasm(target, vma)]
    b = [w for _, w, _ in E.disasm(cand, vma)]
    same = sum(1 for x, y in zip(a, b) if x == y)
    return (max(len(a), len(b)) - same, len(a), same)


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("variants", help="python file defining HEADERS and BODIES")
    p.add_argument("symbol")
    p.add_argument("-m", "--module", default="SUPPORT")
    p.add_argument("--trim", action="store_true", default=True)
    p.add_argument("-n", "--top", type=int, default=15)
    # A sweep scores POSITIONALLY, so a variant whose layout is right but whose
    # length is off by one word reads as "everything wrong".  --keep writes each
    # variant's object out as <dir>/<variant>.o so `eqasm.py` can be pointed at
    # it: the count ranks, the listing decides.
    p.add_argument("--keep", metavar="DIR",
                   help="write each variant's object to DIR/<name>.o for eqasm.py")
    a = p.parse_args(argv)

    ns: dict = {}
    exec(Path(a.variants).read_text(), ns)
    # VARIANTS maps name -> complete translation unit.  A header x body cross
    # product generates combinations that cannot compile and wastes the sweep
    # on them, so variants are self-contained.
    variants = ns["VARIANTS"]
    tcs = toolchains(ns.get("TOOLCHAINS"))
    flagsets = ns.get("FLAGS", [DEFAULT_FLAGS])

    _, st, img = E.load(a.module)
    sym = st.get(a.symbol)
    target = img.func(sym, trim=a.trim)
    ntarget = len(target) // 4

    total = len(variants) * len(tcs) * len(flagsets)
    print(f"target {a.symbol}: {ntarget} instructions")
    print(f"sweeping {len(variants)}v x {len(tcs)}tc x {len(flagsets)}f "
          f"= {total} builds\n", flush=True)

    results, absent, failed = [], 0, 0
    with tempfile.TemporaryDirectory() as td:
        wd = Path(td)
        for tcn, drv in tcs:
            for flags in flagsets:
                for vn, obj in build_batch(drv, flags, variants, wd):
                    if obj is None:
                        failed += 1
                        continue
                    if a.keep:
                        kd = Path(a.keep)
                        kd.mkdir(parents=True, exist_ok=True)
                        (kd / f"{vn}.o").write_bytes(obj.read_bytes())
                    s = score(target, obj, a.symbol, sym.addr)
                    if s is None:
                        absent += 1
                        continue
                    results.append((s[0], s[2], vn, tcn, " ".join(flags)))
                print(f"  {tcn} {' '.join(flags)}: done", flush=True)

    results.sort(key=lambda r: (r[0], -r[1]))
    print(f"{'wrong':>5} {'ok':>4}  {'variant':<24} {'toolchain':<24} flags")
    for wrong, ok, vn, tcn, fl in results[:a.top]:
        mark = "  *** FULL MATCH ***" if wrong == 0 else ""
        print(f"{wrong:>5} {ok:>4}  {vn:<24} {tcn:<24} {fl}{mark}")
    # never silent about what did not run: a build that failed or whose symbol
    # was missing is not evidence about the source, and must not read as one
    print(f"\n{len(results)} scored, {failed} build failures, "
          f"{absent} missing the symbol (wrong mangling => wrong compiler)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
