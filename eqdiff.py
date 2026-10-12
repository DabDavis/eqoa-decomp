#!/usr/bin/env python3
"""
Normalising differ: one target function from the module image against a
candidate build.

The candidate may be raw bytes or an unlinked ELF object.  Prefer the object:
an unlinked `.o` holds zero in the same fields the un-relocated module image
holds zero in (jal targets, hi/lo address pairs), so the two agree with NO
normalisation, and a clean result is earned rather than manufactured.  Every
normalised word is counted by kind for exactly that reason.
"""

from __future__ import annotations

import difflib
import os
import struct
import re
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

import eqdecomp as E


# --- reading a function out of an unlinked object ---------------------------

def _tool(suffix: str) -> str:
    ps2dev = os.environ.get("PS2DEV", os.path.expanduser("~/ps2dev/ps2dev"))
    cand = f"{ps2dev}/ee/bin/mips64r5900el-ps2-elf-{suffix}"
    if Path(cand).is_file():
        return cand
    import shutil
    for c in (f"mips64r5900el-ps2-elf-{suffix}", f"mips-linux-gnu-{suffix}"):
        if shutil.which(c):
            return c
    raise RuntimeError(f"no {suffix} for the EE target; set PS2DEV")


_SYM = re.compile(
    r"^\s*\d+:\s+([0-9a-f]+)\s+(\d+)\s+FUNC\s+\S+\s+\S+\s+(\S+)\s+(\S+)\s*$")


def elf_function(path: Path, symbol: str) -> tuple[bytes, str]:
    """(bytes, section) for one FUNC symbol in an unlinked object."""
    data, sec, _ = elf_function_at(path, symbol)
    return data, sec


def elf_function_at(path: Path, symbol: str) -> tuple[bytes, str, int]:
    """(bytes, section, offset-within-section) for one FUNC symbol."""
    out = subprocess.run([_tool("readelf"), "-sW", str(path)],
                         capture_output=True, text=True, check=True).stdout
    hit = None
    for line in out.splitlines():
        m = _SYM.match(line)
        if m and m.group(4) == symbol:
            hit = (int(m.group(1), 16), int(m.group(2)), m.group(3))
            break
    if hit is None:
        raise KeyError(f"no FUNC symbol {symbol!r} in {path}")
    value, size, ndx = hit
    if size == 0:
        raise ValueError(f"{symbol} has size 0 in {path}; assemble with .size")

    shs = subprocess.run([_tool("readelf"), "-SW", str(path)],
                         capture_output=True, text=True, check=True).stdout
    secname = None
    for line in shs.splitlines():
        m = re.match(r"^\s*\[\s*(\d+)\]\s+(\S+)", line)
        if m and m.group(1) == ndx:
            secname = m.group(2)
            break
    if secname is None:
        raise ValueError(f"cannot resolve section index {ndx} in {path}")

    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        tmp = f.name
    try:
        subprocess.run([_tool("objcopy"), "-O", "binary",
                        f"--only-section={secname}", str(path), tmp],
                       check=True, capture_output=True)
        data = Path(tmp).read_bytes()
    finally:
        os.unlink(tmp)
    return data[value:value + size], secname, value


def elf_relocations(path: Path, section: str | None = None
                    ) -> list[tuple[int, str, str]]:
    """(offset, type, symbol) for relocations, optionally one section's only.

    Offsets are RELATIVE TO THE RELOCATED SECTION, so they are only comparable
    with a function's offset once the section matches -- `.rel.data` entries
    landed inside `.text` functions' byte ranges and masked 14 real instruction
    words before this took a section argument.
    """
    out = subprocess.run([_tool("readelf"), "-rW", str(path)],
                         capture_output=True, text=True, check=True).stdout
    want = f".rel{section}" if section else None
    rows, cur = [], None
    for line in out.splitlines():
        h = re.match(r"^Relocation section '(\S+)'", line)
        if h:
            cur = h.group(1)
            continue
        m = re.match(r"^([0-9a-f]{8,16})\s+[0-9a-f]+\s+(\S+)\s+[0-9a-f]*\s*(\S*)",
                     line)
        if m and m.group(2).startswith("R_MIPS"):
            if want is not None and cur not in (want, want.replace(".rel", ".rela")):
                continue
            rows.append((int(m.group(1), 16), m.group(2), m.group(3)))
    return rows


# --- relocation masking -------------------------------------------------------

# What the linker overwrites, per relocation type.  Masking exactly these bits,
# at exactly the offsets the object's OWN relocation table names, is safe by
# construction -- unlike the hi16lo16 heuristic, which guesses at pairs and can
# erase a float constant.  Anything not in this table is left alone and the
# site is counted as unmasked, so an unknown relocation cannot pass silently.
RELOC_FIELD = {
    "R_MIPS_26": 0x03FFFFFF,
    "R_MIPS_HI16": 0xFFFF,
    "R_MIPS_LO16": 0xFFFF,
    "R_MIPS_GPREL16": 0xFFFF,
    "R_MIPS_LITERAL": 0xFFFF,
    "R_MIPS_PC16": 0xFFFF,
    "R_MIPS_32": 0xFFFFFFFF,
}


@dataclass
class RelocMask:
    sites: int = 0            # relocation sites inside the function
    masked: int = 0           # words actually cleared (in either side)
    unknown: list = field(default_factory=list)   # types with no field entry

    def __str__(self) -> str:
        s = f"reloc sites={self.sites} masked={self.masked}"
        if self.unknown:
            s += f" UNKNOWN={sorted(set(self.unknown))}"
        return s


def mask_relocs(target: bytes, cand: bytes, relocs, lo: int, hi: int):
    """Zero the linker-written field of both sides at each relocation site.

    `relocs` is elf_relocations() output, `lo`/`hi` the function's byte range
    within its section.  Returns (target, cand, RelocMask).
    """
    rep = RelocMask()
    if not relocs:
        return target, cand, rep
    t, c = bytearray(target), bytearray(cand)
    for off, typ, _sym in relocs:
        if not (lo <= off < hi) or (off - lo) + 4 > min(len(t), len(c)):
            continue
        rep.sites += 1
        field_mask = RELOC_FIELD.get(typ)
        if field_mask is None:
            rep.unknown.append(typ)
            continue
        i = off - lo
        for buf in (t, c):
            w = struct.unpack_from("<I", buf, i)[0]
            if w & field_mask:
                struct.pack_into("<I", buf, i, w & ~field_mask)
                rep.masked += 1
    return bytes(t), bytes(c), rep


# --- the diff ----------------------------------------------------------------

@dataclass
class DiffResult:
    symbol: str
    target_len: int
    cand_len: int
    raw_identical: bool
    norm_identical: bool
    norm_target: E.NormReport
    norm_cand: E.NormReport
    first_diff: int | None
    reloc: "RelocMask | None" = None
    insns_target: int = 0
    insns_cand: int = 0
    text: list[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return self.norm_identical

    @property
    def verdict(self) -> str:
        if self.raw_identical:
            return "MATCH (raw, no normalisation needed)"
        if self.norm_identical:
            extra = ""
            if self.reloc is not None and self.reloc.masked:
                extra = f", {self.reloc}"
            return f"MATCH (after normalisation: {self.norm_target}{extra})"
        if self.target_len != self.cand_len:
            return f"FAIL (length {self.cand_len} vs target {self.target_len})"
        return f"FAIL (first difference at byte 0x{self.first_diff:X})"


def diff(target: bytes, candidate: bytes, symbol: str, vma: int = 0,
         modes: set[str] | None = None, context: int = 3,
         relocs=None, span: tuple[int, int] | None = None) -> DiffResult:
    modes = modes if modes is not None else {"j26"}

    raw_identical = target == candidate
    rmask = None
    if relocs and span is not None:
        target, candidate, rmask = mask_relocs(target, candidate, relocs,
                                               span[0], span[1])
    nt, rt = E.normalise(target, modes)
    nc, rc = E.normalise(candidate, modes)
    norm_identical = nt == nc

    first = None
    if not norm_identical:
        for i in range(min(len(nt), len(nc))):
            if nt[i] != nc[i]:
                first = i
                break
        if first is None:
            first = min(len(nt), len(nc))

    dt = E.disasm(nt, vma)
    dc = E.disasm(nc, vma)
    res = DiffResult(
        symbol=symbol, target_len=len(target), cand_len=len(candidate),
        raw_identical=raw_identical, norm_identical=norm_identical,
        norm_target=rt, norm_cand=rc, first_diff=first, reloc=rmask,
        insns_target=len(dt), insns_cand=len(dc),
    )
    if not norm_identical:
        a = [f"{w:08x}  {t}" for _, w, t in dt]
        b = [f"{w:08x}  {t}" for _, w, t in dc]
        res.text = list(difflib.unified_diff(
            a, b, fromfile=f"target {symbol}", tofile=f"candidate {symbol}",
            lineterm="", n=context))
    return res


def report(res: DiffResult) -> str:
    out = [
        f"{res.symbol}",
        f"  target    {res.target_len:>5} bytes  {res.insns_target:>4} insns"
        f"   normalised {res.norm_target}",
        f"  candidate {res.cand_len:>5} bytes  {res.insns_cand:>4} insns"
        f"   normalised {res.norm_cand}",
        f"  {res.verdict}",
    ]
    if res.text:
        out.append("")
        out.extend(res.text)
    return "\n".join(out)
