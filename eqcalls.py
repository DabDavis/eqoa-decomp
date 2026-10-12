#!/usr/bin/env python3
"""
eqcalls -- resolve a target function's CALLEES by name.

The decomp oracle (`SUPPORT_decompressed_SNR1.bin`) is UN-RELOCATED: every
`jal` holds 0x0C000000 and every `lui %hi` holds zero, which is exactly what
makes an unlinked candidate object comparable to it with no normalisation.  The
price is that the image cannot say WHICH function a call goes to, so until now a
callee was inferred from its argument shape and the plausibility of the name --
`VISprite::QueryEmitters`'s `Insert`/`Erase`, `VISurface::Init`'s `Deallocate`.

It does not have to be.  SUPPORT is a resident module in a running game, loaded
at the SAME virtual addresses this image is indexed by, so any EE memory dump
taken in-world holds the RELOCATED copy of the very same build:

    209,620 of 227,306 words identical (92.22%), and every one of the 17,686
    differences is a relocation site -- 14,488 `jal`, 1,451 `lui`, 1,133
    `addiu`, 328 `lw`, 218 `sw` and a 21-word tail.  Nothing else moved.

So the dump is the oracle's own link map.  `resolve` reads a call's target out of
it and names it from the symbol table; `--verify` re-proves the pairing on the
spot rather than trusting this docstring.

    ./eqcalls.py calls Raster__11VILODSpriteR8VICameraR7VISceneR8VIRasterUi
    ./eqcalls.py at 0x452b18
    ./eqcalls.py verify

The default dump is memory-dumps/inworld-eqoa.live.eememory; -d takes another.
A dump from a DIFFERENT build would resolve to plausible-looking garbage, so
`calls` refuses to run until the function's own non-relocated words match the
image, and says so by name when they do not.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E

DUMP = E.ROOT / "memory-dumps" / "inworld-eqoa.live.eememory"

# The engine-code range of the SUPPORT image; see eqdecomp's module table.
CODE_LO, CODE_HI = 0x003E3D00, 0x004C1CA8
# The module's own header sits in front of the first function and holds six
# pointers the loader rebases; it is data, not code, so it is skipped.
HEADER_HI = 0x003E3D40

# Every opcode the linker writes a %lo into.  The narrow set (jal, lui, addiu,
# lw, sw) leaves 50 stray words that all turn out to be relocations as well --
# `sb`/`sh`/`lbu`/`lhu`/`lq`/`sq` %lo forms in VIPrimBuffer's packed-vertex
# writers and VIRaster's GS register loads -- so counting them as unexplained
# would understate the agreement.  With them included the code range is
# 100.000% accounted for.
RELOC_OPS = frozenset({2, 3, 15, 9, 13,
                       32, 33, 35, 36, 37, 40, 41, 43, 49, 55, 57, 63})


def load_dump(path: Path) -> bytes:
    if not path.is_file():
        raise SystemExit(f"no dump at {path}")
    return path.read_bytes()


def word(d: bytes, va: int) -> int:
    return struct.unpack_from("<I", d, va)[0]


def string_at(d: bytes, addr: int, limit: int = 96) -> str | None:
    """The C string at `addr` in the dump, or None if that is not what is there.

    The image is un-relocated, so a `lui`+`addiu` pair against a string literal
    holds only the addend and the string itself is unreachable from it.  The
    DUMP is the same build relocated, so the pair resolves to a real address --
    and a string constant lands in the module's .rodata among the RTTI names,
    where `name_at` can only report the nearest preceding symbol plus an
    offset.  This reads the bytes instead.  That is how `"lightning.tga"` was
    recovered for VILightningFxInit's VIHashResourceID call.
    """
    if addr + limit > len(d):
        return None
    end = d.find(b"\0", addr, addr + limit)
    if end <= addr:                       # empty, or no terminator in range
        return None
    body = d[addr:end]
    if any(c < 0x20 or c > 0x7E for c in body):
        return None
    return body.decode("ascii")


def name_at(st, addr: int) -> str:
    s = st.at(addr)
    if s is None:
        return f"{addr:#010x}"
    off = addr - s.addr
    return s.name if off == 0 else f"{s.name}+{off:#x}"


def verify(img, d: bytes) -> tuple[int, int, dict]:
    """Word-for-word agreement between the image and the dump over the code range."""
    same = diff = 0
    kinds: dict[int, int] = {}
    for va in range(HEADER_HI, CODE_HI, 4):
        a = word(img.read(va, 4), 0)
        b = word(d, va)
        if a == b:
            same += 1
        else:
            diff += 1
            kinds[a >> 26] = kinds.get(a >> 26, 0) + 1
    return same, diff, kinds


def function_agrees(img, d: bytes, sym) -> list[int]:
    """VAs where image and dump differ on a word that is NOT a relocation site."""
    bad = []
    for va in range(sym.addr, sym.end, 4):
        a = word(img.read(va, 4), 0)
        b = word(d, va)
        if a == b:
            continue
        op = a >> 26
        if op in RELOC_OPS:
            continue
        bad.append(va)
    return bad


def calls(st, img, d: bytes, sym) -> list[tuple[int, str, str]]:
    """(va, kind, resolved) for every relocated site in the function."""
    out = []
    hi: dict[int, tuple[int, int]] = {}       # register -> (va, hi16 from dump)
    for va in range(sym.addr, sym.end, 4):
        w = word(d, va)
        op = w >> 26
        if op in (2, 3):
            tgt = ((va + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
            out.append((va, "jal" if op == 3 else "j", name_at(st, tgt)))
        elif op == 15:                                    # lui rt, imm
            hi[(w >> 16) & 0x1F] = (va, w & 0xFFFF)
        elif op in (9, 13, 35, 43, 49, 57):               # addiu/ori/lw/sw/lwc1/swc1
            rs = (w >> 21) & 0x1F
            if rs in hi:
                hva, h = hi.pop(rs)
                lo = w & 0xFFFF
                if op in (9, 35, 43, 49, 57):
                    lo = lo - 0x10000 if lo & 0x8000 else lo
                addr = ((h << 16) + lo) & 0xFFFFFFFF
                if addr:
                    txt = string_at(d, addr)
                    label = f'"{txt}"' if txt else name_at(st, addr)
                    out.append((hva, "addr", label))
    return out


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("-d", "--dump", type=Path, default=DUMP)
    sub = p.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("calls", help="resolve every relocated site in a function")
    c.add_argument("symbol")
    a = sub.add_parser("at", help="resolve one address")
    a.add_argument("addr")
    sub.add_parser("verify", help="prove the dump is this image's own link map")
    args = p.parse_args(argv)

    _, st, img = E.load("SUPPORT")
    d = load_dump(args.dump)

    if args.cmd == "verify":
        same, diff, kinds = verify(img, d)
        tot = same + diff
        print(f"{args.dump}")
        print(f"  {same}/{tot} words identical ({100*same/tot:.2f}%), {diff} differ")
        for op, n in sorted(kinds.items(), key=lambda x: -x[1]):
            nm = {2: "j", 3: "jal", 15: "lui", 9: "addiu", 13: "ori",
                  35: "lw", 43: "sw", 49: "lwc1", 57: "swc1"}.get(op, f"op{op}")
            print(f"    {n:6}  {nm}")
        stray = sum(n for op, n in kinds.items() if op not in RELOC_OPS)
        print(f"  {stray} differing words are NOT a relocation shape"
              + ("  <-- NOT the same build" if stray else ""))
        return 0 if stray == 0 else 1

    if args.cmd == "at":
        va = int(args.addr, 0)
        w = word(d, va)
        if (w >> 26) in (2, 3):
            tgt = ((va + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
            print(f"{va:#010x}  {w:#010x}  jal {name_at(st, tgt)}")
        elif (w >> 26) == 15:
            print(f"{va:#010x}  {w:#010x}  lui {(w & 0xFFFF):#06x}"
                  "  (the %hi half; use `calls` for the pair)")
        else:
            txt = string_at(d, va)
            if txt:
                print(f'{va:#010x}  "{txt}"')
            else:
                print(f"{va:#010x}  {w:#010x}  (not a jal)")
        return 0

    sym = st.get(args.symbol)
    bad = function_agrees(img, d, sym)
    if bad:
        print(f"REFUSED: {sym.name} differs from the dump at "
              f"{len(bad)} non-relocation words ({', '.join(hex(v) for v in bad[:6])})"
              " -- that dump is not this image's build", file=sys.stderr)
        return 1
    print(f"{sym.name}  {sym.addr:#010x}  {sym.span} bytes")
    for va, kind, nm in calls(st, img, d, sym):
        print(f"  {va:#010x}  {kind:5} {nm}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
