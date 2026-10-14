#!/usr/bin/env python3
"""
eqlocal -- score the functions the symbol map does NOT name.

The map names one address per symbol, so everything else in SUPPORT is invisible
to `eqcheck.py`: a TU's file-static helpers, functions whose names the map lost,
and the per-TU COPIES of template members that the original build emitted
wherever they were used (only one copy per name is mapped).  Phase 61 measured
124,832 bytes of such code behind the 110 SPAN_CAPs alone -- about 105 KB of
it byte-identical duplicates of named functions, the rest unique.

A TU's .text is laid out in the image exactly as the object lays it out, so an
unnamed function sits at a fixed distance from its named neighbours.  This
anchors on every function whose name the map has, and between two anchors A and
B it places the unnamed functions at `addr(A) + (offset - offset(A))` -- but
only where the stretch from A to B is the SAME LENGTH in both, which is what
proves the layout agrees.  Each placed function is then scored the way
`eqcheck.score` scores a named one (the object's own relocations masked).

A stretch whose lengths differ is reported with both sizes: code the target has
and we do not (unwritten, or an un-emitted template copy) or the reverse.

    ./eqlocal.py                  # every TU in build/
    ./eqlocal.py connection drdp  # those TUs
    ./eqlocal.py -v               # also list each placed function

Reads build/*.o as the last eqcheck/_tu run left them; builds nothing.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import eqdecomp as E
import eqdiff as D
import eqcheck as C


def text_functions(obj: Path):
    """[(offset, size, name)] for every FUNC symbol in .text, by offset."""
    secs = {}
    out = subprocess.run([D._tool("readelf"), "-SW", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        m = re.match(r"^\s*\[\s*(\d+)\]\s+(\S+)", line)
        if m:
            secs[m.group(1)] = m.group(2)
    out = subprocess.run([D._tool("readelf"), "-sW", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    fns = []
    for line in out.splitlines():
        m = re.match(r"^\s*\d+:\s+([0-9a-f]+)\s+(\d+)\s+FUNC\s+\S+\s+\S+\s+(\S+)\s+(\S+)\s*$", line)
        if m and secs.get(m.group(3)) == ".text" and int(m.group(2)) > 0:
            fns.append((int(m.group(1), 16), int(m.group(2)), m.group(4)))
    return sorted(set(fns))


def score_at(img, obj: Path, name: str, addr: int):
    """('match'|'wrong', wrong, total) for our `name` scored against the image at addr."""
    cand, sec, at = D.elf_function_at(obj, name)
    target = img.read(addr, len(cand))
    relocs = D.elf_relocations(obj, sec)
    res = D.diff(target, cand, name, addr, modes={"j26"}, relocs=relocs,
                 span=(at, at + len(cand)))
    if res.ok:
        return "match", 0, len(cand) // 4
    a = [w for _, w, _ in E.disasm(target, addr)]
    b = [w for _, w, _ in E.disasm(cand, addr)]
    return "wrong", sum(1 for x, y in zip(a, b) if x != y), len(b)


def unnamed_regions(st, img):
    """[(lo, hi, host)] -- the code the map does not name: what each SPAN_CAP cut
    off its host symbol's derived span."""
    caps = dict(E.SPAN_CAP)
    E.SPAN_CAP.clear()
    try:
        _, st2, _ = E.load()
    finally:
        E.SPAN_CAP.update(caps)
    full = {x.name: x.span for x in st2.symbols}
    return [(x.addr + x.span, x.addr + full[x.name], x.name)
            for x in st.symbols if x.capped and full[x.name] > x.span]


def masked_function(obj, name):
    """(words, keep-masks) for our function: each word with the bits its own
    relocation names cleared, so a compare is one AND per word."""
    cand, sec, at = D.elf_function_at(obj, name)
    keep = [0xFFFFFFFF] * (len(cand) // 4)
    for off, typ, _ in D.elf_relocations(obj, sec):
        if at <= off < at + len(cand):
            keep[(off - at) // 4] &= ~D.RELOC_FIELD.get(typ, 0) & 0xFFFFFFFF
    words = [int.from_bytes(cand[k:k + 4], "little") & keep[k // 4]
             for k in range(0, len(cand), 4)]
    return words, keep


def find_in(image_words, words, keep, regions):
    """Every address inside the unnamed regions where our function's unmasked
    bits all agree with the image."""
    n = len(words)
    hits = []
    for lo, hi, _ in regions:
        for a in range(lo, hi - 4 * n + 4, 4):
            base = (a - lo) // 4
            seg = image_words[lo]
            if all((seg[base + k] & keep[k]) == words[k] for k in range(n)):
                hits.append(a)
    return hits


def main(argv):
    verbose = "-v" in argv
    tus = [a for a in argv if not a.startswith("-")]
    _, st, img = E.load()
    regions = unnamed_regions(st, img)
    image_words = {lo: [int.from_bytes(img.read(a, 4), "little") for a in range(lo, hi, 4)]
                   for lo, hi, _ in regions}
    objs = [C.BUILD / f"{t}.o" for t in tus] if tus else \
           [C.BUILD / f"{s.stem}.o" for s in sorted(C.SRC.glob("*.cpp"))]
    covered = []                  # (addr, size) of unnamed target code we now match
    n_unnamed = n_match = 0
    for obj in objs:
        if not obj.exists():
            continue
        fns = text_functions(obj)
        relocs = {}
        rows = []
        last = None                           # (our offset, target address) of the last placed function
        for off, size, n in fns:
            try:
                s_ = st.get(n)
            except KeyError:
                s_ = None
            if s_ is not None and s_.span > 0:
                last = (off, s_.addr)
                continue                      # named: eqcheck scores it
            n_unnamed += 1
            words, keep = masked_function(obj, n)
            hits = find_in(image_words, words, keep, regions)
            if hits:
                n_match += 1
                covered.extend((h, 4 * len(words)) for h in hits)
                rows.append(("match", n, hits))
                last = (off, hits[0])
            else:
                # not found anywhere: score it where the layout puts it, after the
                # last function this object placed, so the miss has a diff target
                guess = None
                if last is not None:
                    addr = last[1] + (off - last[0])
                    k, w, t = score_at(img, obj, n, addr)
                    guess = (addr, w, t)
                rows.append(("none", n, guess))
        if rows:
            m = sum(1 for r in rows if r[0] == "match")
            print(f"{obj.stem}: {m}/{len(rows)} unnamed functions found exact in the unnamed code")
            for k, n, hits in rows:
                if k == "match":
                    if verbose:
                        print(f"    {'MATCH ' + ', '.join(hex(h) for h in hits):40s} {n}")
                elif hits:
                    addr, w, t = hits
                    print(f"    {f'not found; at layout {addr:#x}: {w}/{t} wrong':40s} {n}")
                else:
                    print(f"    {'not found; no anchor before it':40s} {n}")
    # what is left of the unnamed code, classified
    cov = set()
    for a, sz in covered:
        cov.update(range(a, a + sz, 4))
    bodies = {}
    for x in st.symbols:
        if x.span > 8:
            bodies.setdefault(img.read(x.addr, x.span), x.name)
    total = sum(hi - lo for lo, hi, _ in regions)
    left_dup = left_uniq = 0
    uniq = []
    for lo, hi, host in regions:
        pos = lo
        while pos < hi:
            if pos in cov:
                pos += 4; continue
            hit = None
            for b, nm in bodies.items():
                if len(b) <= hi - pos and img.read(pos, len(b)) == b:
                    if hit is None or len(b) > len(hit):
                        hit = b
            if hit:
                left_dup += len(hit); pos += len(hit)
            else:
                left_uniq += 4
                if uniq and uniq[-1][1] == pos:
                    uniq[-1] = (uniq[-1][0], pos + 4, host)
                else:
                    uniq.append((pos, pos + 4, host))
                pos += 4
    print(f"\nunnamed functions in our objects: {n_unnamed}, {n_match} found exact in the image")
    print(f"unnamed target code: {total} bytes -- {len(cov) * 4} matched by those, "
          f"{left_dup} byte-identical COPIES of named functions (not emitted by our TUs), "
          f"{left_uniq} unique and unmatched")
    if verbose:
        for lo, hi, host in sorted(uniq, key=lambda u: u[0] - u[1])[:40]:
            print(f"    {hi - lo:6d}  {lo:#x}  after {host}")


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
