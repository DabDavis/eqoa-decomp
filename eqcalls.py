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
import bisect
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


def relocated(img=None):
    """The relocated copy of the current module (EQMODULE), indexed by VA.

    SUPPORT reads the in-world dump when there is one.  Without it, and for every
    other module, the copy is rebuilt from the image's own relocation table
    (`eqdecomp.relocate`): over SUPPORT's code that agrees with the dump word for
    word, and eqcallcheck and eqrefcheck print the same reports from either.
    Another module's imports land on stand-in addresses that `eqdecomp.load`
    names from the module's own import table."""
    if img is None:
        img = E.load()[2]
    if img.module.name == "SUPPORT" and DUMP.is_file():
        return load_dump(DUMP)
    return E.relocate(img.module, img.data)


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


def refs(st, img, va: int) -> list[tuple[int, str, str]]:
    """The reverse of `calls`: (site, kind, containing function) for every relocation
    in the module whose target is `va` -- read off the module's own relocation table,
    so it needs no dump and sees data references (`word`, `hi`/`lo`) as well as calls."""
    own = sorted((s.addr, s.name) for s in st.symbols if s.span > 0)
    starts = [a for a, _ in own]

    def owner(x):
        i = bisect.bisect_right(starts, x) - 1
        return own[i][1] if i >= 0 else "?"
    kinds = {E.R_WORD: "word", E.R_JAL: "jal", E.R_HI16: "hi", E.R_LO16: "lo"}
    return sorted((img.base + site, kinds.get(kind, str(kind)), owner(img.base + site))
                  for site, kind, imp, target in E.module_relocations(img.data)
                  if not imp and img.base + target == va)


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("-d", "--dump", type=Path,
                   help="a relocated dump to read (default: the module's own relocated copy)")
    sub = p.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("calls", help="resolve every relocated site in a function")
    c.add_argument("symbol")
    a = sub.add_parser("at", help="resolve one address")
    a.add_argument("addr")
    sub.add_parser("verify", help="prove the dump is this image's own link map")
    r = sub.add_parser("refs", help="every site that references a function or address")
    r.add_argument("target", nargs="+", help="symbol name or 0x address")
    args = p.parse_args(argv)

    if args.cmd == "refs":
        _, st, img = E.load()
        for t in args.target:
            va = int(t, 0) if t.startswith("0x") else st.get(t).addr
            hits = refs(st, img, va)
            sym = st.at(va)
            print(f"{va:#010x}  {sym.name if sym else '?'}: {len(hits)} refs")
            for site, kind, nm in hits:
                print(f"  {site:#010x}  {kind:5} in {nm}")
        return 0

    _, st, img = E.load()
    d = load_dump(args.dump) if args.dump else relocated(img)

    if args.cmd == "verify":
        if img.module.name != "SUPPORT" or not (args.dump or DUMP.is_file()):
            raise SystemExit("verify checks SUPPORT's in-world dump against the image")
        same, diff, kinds = verify(img, d)
        tot = same + diff
        print(f"{args.dump or DUMP}")
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
