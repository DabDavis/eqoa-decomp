#!/usr/bin/env python3
"""
Codegen fingerprints over a body of MIPS/EE code.

Purpose: narrow the compiler version and flag set BEFORE attempting byte-exact
matching.  Per-function diffing is an expensive way to discover that `-G0` was
wrong; comparing aggregate fingerprint profiles between the target and a
candidate build is cheap and rules out whole branches of the search at once.

Run it on the module region, on an unlinked object, or on both to compare:

    ./eqfingerprint.py                       # the SUPPORT engine region
    ./eqfingerprint.py --obj build/m44.o
    ./eqfingerprint.py --obj build/m44.o --compare '__C?10VIMatrix44'

Fingerprints chosen because they vary by version or by flag, and because each
is a decision rather than a preference:

  gp_rel        gp-relative addressing.  Zero means no small-data area, i.e.
                built -G0 (or a module format that cannot use $gp).  Decides
                whether the decomp.wiki `.lit4` matching hazard applies at all.
  likely        branch-likely (beql/bnel/...).  Heavy use points at the older
                2.9x/2.95x SN line; later GCC stopped favouring them.
  save_width    sq/lq vs sd/ld vs sw/lw for callee-saved registers.  128-bit
                saves are an EE ABI decision, not an optimisation.
  fp_inline     lui+mtc1, a float constant whose low half is zero, materialised
                in two instructions instead of loaded.
  fp_abs        lui+lwc1, a float literal loaded from memory without $gp.
  multhi        mult/multu, the division-by-constant strategy.
  vu0           COP2 macro mode -- hand-written asm, not compiler output.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import eqdecomp as E

GP, SP = 28, 29
LOADS = {0x20, 0x21, 0x23, 0x24, 0x25, 0x31, 0x35, 0x37, 0x1E}
STORES = {0x28, 0x29, 0x2B, 0x39, 0x3D, 0x3F, 0x1F}
LIKELY_OP = {0x14: "beql", 0x15: "bnel", 0x16: "blezl", 0x17: "bgtzl"}
LIKELY_RT = {0x02: "bltzl", 0x03: "bgezl", 0x12: "bltzall", 0x13: "bgezall"}

KEYS = ["insns", "gp_rel", "likely", "sq_lq", "sd_ld", "sw_lw",
        "fp_inline", "fp_abs", "multhi", "vu0", "jal"]


def fingerprint(chunks) -> dict:
    """chunks: iterable of bytes objects, one per function."""
    fp = {k: 0 for k in KEYS}
    fp["likely_kinds"] = {}
    for data in chunks:
        prev = None
        for i in range(0, len(data) - 3, 4):
            w = struct.unpack_from("<I", data, i)[0]
            if w == 0:
                prev = w
                continue
            fp["insns"] += 1
            op, base, rt = w >> 26, (w >> 21) & 0x1F, (w >> 16) & 0x1F

            if base == GP and (op in LOADS or op in STORES):
                fp["gp_rel"] += 1
            if op in LIKELY_OP:
                fp["likely"] += 1
                k = LIKELY_OP[op]
                fp["likely_kinds"][k] = fp["likely_kinds"].get(k, 0) + 1
            elif op == 0x01 and rt in LIKELY_RT:
                fp["likely"] += 1
                k = LIKELY_RT[rt]
                fp["likely_kinds"][k] = fp["likely_kinds"].get(k, 0) + 1
            if base == SP:
                if op in (0x1E, 0x1F):
                    fp["sq_lq"] += 1
                elif op in (0x37, 0x3F):
                    fp["sd_ld"] += 1
                elif op in (0x23, 0x2B):
                    fp["sw_lw"] += 1
            if op == 0x03:
                fp["jal"] += 1
            if op == 0x00 and (w & 0x3F) in (0x18, 0x19):
                fp["multhi"] += 1
            if op in (0x12, 0x36, 0x3E):
                fp["vu0"] += 1
            if prev is not None and (prev >> 26) == 0x0F:
                hi_rt = (prev >> 16) & 0x1F
                if op == 0x11 and ((w >> 21) & 0x1F) == 0x04 and rt == hi_rt:
                    fp["fp_inline"] += 1
                elif op == 0x31 and base == hi_rt:
                    fp["fp_abs"] += 1
            prev = w
    return fp


def per_kloc(fp: dict) -> dict:
    """Rates, so a 200k-instruction target compares with a 2k-instruction probe."""
    n = fp["insns"] or 1
    return {k: 1000.0 * fp[k] / n for k in KEYS if k != "insns"}


def render(name: str, fp: dict) -> str:
    r = per_kloc(fp)
    out = [f"{name}: {fp['insns']} instructions"]
    for k in KEYS[1:]:
        out.append(f"  {k:<10} {fp[k]:>8}   {r[k]:>8.2f} per kinsn")
    if fp["likely_kinds"]:
        kinds = " ".join(f"{k}={v}" for k, v in sorted(fp["likely_kinds"].items()))
        out.append(f"  likely kinds: {kinds}")
    return "\n".join(out)


def verdicts(fp: dict) -> list[str]:
    r = per_kloc(fp)
    v = []
    if fp["gp_rel"] == 0:
        v.append("no gp-relative addressing at all -> built without a small-data "
                 "area (-G0). The decomp.wiki .lit4/%gp_rel matching hazard does "
                 "NOT apply here.")
    else:
        v.append(f"gp-relative addressing present ({fp['gp_rel']}) -> .lit4 "
                 "literals are live; expect the nop-difference hazard.")
    if r["likely"] > 5:
        v.append(f"branch-likely used heavily ({r['likely']:.1f}/kinsn) -> the "
                 "older SN 2.9x/2.95x line, not a later GCC.")
    elif fp["likely"] == 0:
        v.append("no branch-likely at all -> a later GCC, or -mno-branch-likely.")
    # compare only the WIDE widths: sw/lw on sp is dominated by ordinary local
    # variable access and says nothing about the register-save convention
    if fp["sq_lq"] > 2 * max(fp["sd_ld"], 1):
        v.append(f"callee-saved registers are spilled 128-bit: sq/lq "
                 f"{fp['sq_lq']} vs sd/ld {fp['sd_ld']} -> the EE 128-bit GPR "
                 "ABI. A candidate that saves with sd or sw will mismatch every "
                 "non-leaf prologue and epilogue, i.e. almost everything.")
    if fp["vu0"]:
        v.append(f"{fp['vu0']} COP2 instructions -> hand-written VU0 macro mode "
                 "is present; those functions are transcription, not codegen, "
                 "and must be scored separately.")
    return v


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("-m", "--module", default="SUPPORT", choices=sorted(E.MODULES))
    p.add_argument("--region", default="SUPPORT_DATA",
                   help="symbol tag to scan (default the engine region)")
    p.add_argument("--pattern", help="restrict to symbols matching this regex")
    p.add_argument("--obj", help="fingerprint an unlinked object instead")
    p.add_argument("--compare", metavar="PATTERN",
                   help="with --obj: also fingerprint this target set and diff")
    a = p.parse_args(argv)

    def target_fp(pattern=None):
        _, st, img = E.load(a.module)
        syms = [s for s in st.sized if s.tag == a.region]
        if pattern:
            keep = {s.name for s in st.find(pattern)}
            syms = [s for s in syms if s.name in keep]
        if not syms:
            raise SystemExit(f"no symbols in region {a.region!r}"
                             + (f" matching {pattern!r}" if pattern else ""))
        label = f"{a.module}/{a.region}" + (f" ~ {pattern}" if pattern else "")
        return label, fingerprint(img.func(s) for s in syms)

    if a.obj:
        import eqdiff as D
        import subprocess, tempfile, os
        objp = Path(a.obj)
        out = subprocess.run([D._tool("objcopy"), "-O", "binary",
                              "--only-section=.text", str(objp),
                              (tmp := tempfile.mktemp(suffix=".bin"))],
                             capture_output=True)
        if out.returncode:
            raise SystemExit(out.stderr.decode())
        data = Path(tmp).read_bytes()
        os.unlink(tmp)
        print(render(f"candidate {objp.name} [.text]", fingerprint([data])))
        if a.compare:
            label, tfp = target_fp(a.compare)
            print()
            print(render(f"target {label}", tfp))
            print("\ndifferences per kinsn (candidate - target):")
            cr, tr = per_kloc(fingerprint([data])), per_kloc(tfp)
            for k in KEYS[1:]:
                d = cr[k] - tr[k]
                flag = "  <-- " if abs(d) > 1.0 else ""
                print(f"  {k:<10} {d:>+9.2f}{flag}")
        return 0

    label, fp = target_fp(a.pattern)
    print(render(f"target {label}", fp))
    print("\nreadings:")
    for v in verdicts(fp):
        print(f"  * {v}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
