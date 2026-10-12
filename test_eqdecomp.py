#!/usr/bin/env python3
"""
Canary for the extraction and diff harness.

A checker that cannot fail reports success having compared nothing.  Every
test here therefore pairs a positive control with a negative one where that is
possible, and the runner prints the number of assertions actually executed --
an inert suite shows up as a low count rather than as a pass.

Run: ./test_eqdecomp.py   (also works under pytest)
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import eqdecomp as E
import eqdiff as D
import eqfingerprint as F
import eqcalls as C

CHECKS = 0


class Skipped(Exception):
    """A test that could not run.  Reported loudly: a silent skip is a test
    that never ran, which reads exactly like a test that passed."""


def ck(cond, msg):
    global CHECKS
    CHECKS += 1
    assert cond, msg


# a function with no calls and no address pairs, so nothing should normalise
SCALAR = "Transpose__10VIMatrix44RC10VIMatrix44"
# hand-written VU0 macro mode; the base-address positive control
VU0 = "Identity__10VIMatrix44"
# three calls, all of them relocation sites left at zero in the image
CALLS = "RotateX__10VIMatrix44f"


def test_toolchain_decodes_vu0():
    """A generic mips objdump silently mis-decodes COP2; half of VIMatrix44 is VU0."""
    word = struct.pack("<I", 0x4BE0012C)  # vsub.xyzw vf4,vf0,vf0
    rows = E.disasm(word)
    ck(len(rows) == 1, "objdump produced no instruction")
    ck("vsub" in rows[0][2],
       f"objdump does not decode VU0 macro mode: got {rows[0][2]!r}. "
       "Use the EE binutils (set PS2DEV or EQ_OBJDUMP).")


def test_disasm_does_not_collapse_repeated_words():
    """objdump abbreviates a run of identical words to `...` unless -z is given.

    Without it a function's instruction list silently loses whole nops -- and
    `Debug__10VIMatrix44` is FIVE consecutive nops inside an empty loop, so it
    decoded as 6 instructions in 44 bytes.  Every index-wise compare (the
    sweeper's score, eqasm's side-by-side) is then misaligned from the run
    onwards.  Positive control plus the real function.
    """
    blob = struct.pack("<I", 0x24020003) + b"\0" * 20 + struct.pack("<I", 0x03E00008)
    rows = E.disasm(blob)
    ck(len(rows) == 7, f"disasm collapsed a zero run: {len(rows)} rows, expected 7")
    ck(all(r[2] == "nop" for r in rows[1:6]), "the collapsed words are not nops")
    _, st, img = E.load()
    n = len(E.disasm(img.func(st.get("Debug__10VIMatrix44"), trim=True)))
    ck(n == 11, f"Debug decoded as {n} instructions, expected 11")


def test_base_address_is_correct():
    """Positive control: if the base drifts, this decodes as garbage."""
    _, st, img = E.load()
    s = st.get(VU0)
    text = [t for _, _, t in E.disasm(img.func(s), s.addr)]
    ck(any("sqc2" in t for t in text), f"no sqc2 in {VU0}; base address wrong?")
    ck(any(t.startswith("jr") for t in text), f"no return in {VU0}")
    ck(text[0].startswith("vsub"),
       f"{VU0} does not begin with vsub: {text[0]!r} -- base address wrong")


def test_base_address_negative_control():
    """The same read one word off must NOT look like that function."""
    _, st, img = E.load()
    s = st.get(VU0)
    shifted = img.read(s.addr + 4, s.span)
    ck(shifted != img.func(s), "a shifted read matched; the check is inert")


def test_symbol_sizes_are_sane():
    _, st, img = E.load()
    ck(len(st.sized) > 4000, f"only {len(st.sized)} sized symbols; parse broken?")
    ck(all(s.span > 0 for s in st.sized), "a sized symbol has a non-positive span")
    outside = [s for s in st.sized if not img.covers(s.addr, s.span)]
    ck(not outside, f"sized symbols outside the image: {outside[:3]}")


def test_foreign_symbols_are_excluded_and_accounted_for():
    """
    The map is a MERGED map: 191 SLUS symbols and 4 OTHER live in the main
    executable's address space, not in this module image.  They must be
    excluded from extraction and must not silently acquire a span reaching
    into the next region.
    """
    _, st, _ = E.load()
    ck(st.foreign, "no foreign symbols found; the bounds filter is inert")
    tags = {s.tag for s in st.foreign}
    ck(tags <= {"SLUS"} | {t for t in tags if t.startswith("OTHER")},
       f"unexpected tags among foreign symbols: {tags}")
    ck(all(s.span == 0 for s in st.foreign), "a foreign symbol kept a span")
    engine = [s for s in st.sized if s.tag == "SUPPORT_DATA"]
    libgcc = [s for s in st.sized if s.tag == "SUPPORT_CODE"]
    ck(len(engine) > 3000 and len(libgcc) > 300,
       f"region split looks wrong: {len(engine)} engine, {len(libgcc)} libgcc")


def test_span_caps_only_ever_shrink():
    """
    SPAN_CAP is a hand-audited table, not a heuristic, and it must stay one.
    Every entry has to (a) SHRINK the derived span -- a cap that grew one would
    be inventing bytes -- (b) end exactly one word after a `jr ra`, so the cap
    is the function's own last instruction plus its delay slot, and (c) leave
    at least one FURTHER `jr ra` inside the derived span, which is the evidence
    that the gap to the next named symbol really did swallow another function
    rather than that the function is simply long.

    The negative control is the table itself being reachable: an empty or
    inert SPAN_CAP would make every assertion below vacuous, so the count is
    checked first.
    """
    ck(len(E.SPAN_CAP) > 0, "SPAN_CAP is empty; this test is inert")
    _, st, img = E.load()
    for name, cap in E.SPAN_CAP.items():
        sym = st.get(name)
        ck(sym.capped, f"{name}: cap did not fire")
        ck(sym.span == cap, f"{name}: span {sym.span} != cap {cap}")
        derived = _derived_span(st, name)
        ck(cap < derived, f"{name}: cap {cap} does not shrink derived {derived}")
        words = _words(img.read(sym.addr, derived))
        ck(words[cap // 4 - 2] == E.JR_RA,
           f"{name}: cap does not land one word after a jr ra")
        beyond = words[cap // 4:]
        ck(E.JR_RA in beyond,
           f"{name}: nothing beyond the cap returns, so the gap swallowed no "
           f"function and the cap is unjustified")


def test_span_ignore_names_land_inside_a_function():
    """
    SPAN_IGNORE claims a map entry is NOT a function start.  That is a strong
    claim -- getting it wrong hides a real function -- so each name is held to
    three measurements: (a) the address is in the map, (b) the word two before
    it is NOT a `jr ra`, i.e. the address does not follow a return and its
    delay slot the way a real function start does, and (c) the entry now has
    span 0 and is listed in SymbolTable.interior.

    The negative control is a name known to BE a function start: it must FAIL
    test (b), otherwise the check cannot discriminate and every ignore passes.
    """
    ck(len(E.SPAN_IGNORE) > 0, "SPAN_IGNORE is empty; this test is inert")
    _, st, img = E.load()
    ck({s.name for s in st.interior} == set(E.SPAN_IGNORE),
       "SymbolTable.interior does not match SPAN_IGNORE")
    for name in E.SPAN_IGNORE:
        sym = st.get(name)
        ck(sym is not None, f"{name}: not in the map at all")
        ck(sym.span == 0, f"{name}: span {sym.span} != 0")
        before = _words(img.read(sym.addr - 12, 12))
        ck(E.JR_RA not in before,
           f"{name}: one of the three words before it is a jr ra, so this "
           f"address looks like a real function start and must not be ignored")

    for ctrl in ("Clear__t5VIMap2ZUiZQ212VIDictionary11VIDictEntry",
                 "LinkFreeChain__t5VIMap2ZUiZQ212VIDictionary11VIDictEntryi"):
        before = _words(img.read(st.get(ctrl).addr - 12, 12))
        ck(E.JR_RA in before,
           f"control {ctrl}: a real function start follows a jr ra, its delay "
           f"slot and any padding; the check above cannot discriminate")


def _derived_span(st, name):
    """The span the gap rule WOULD have produced, ignoring SPAN_CAP."""
    sym = st.get(name)
    later = sorted({s.addr for s in st.symbols if s.addr > sym.addr})
    return later[0] - sym.addr if later else 0


def _words(b):
    return [struct.unpack_from("<I", b, i)[0] for i in range(0, len(b) // 4 * 4, 4)]


def test_trim_keeps_the_final_delay_slot():
    """
    Regression: trim used to strip the `nop` in the delay slot of the
    terminating `jr ra` along with the alignment padding, which turned a
    byte-exact match into a reported one-instruction failure.
    """
    import struct
    _, st, img = E.load()
    s = st.get(SCALAR)
    raw = img.func(s)
    trimmed = img.func(s, trim=True)
    words = [struct.unpack_from("<I", trimmed, i)[0]
             for i in range(0, len(trimmed), 4)]
    ck(len(words) >= 2, "trim produced too few instructions")
    ck(words[-2] == E.JR_RA,
       f"trimmed function does not end with jr ra + delay slot: "
       f"{words[-2]:#010x} {words[-1]:#010x}")
    ck(len(trimmed) < len(raw), "trim removed nothing; padding is still there")
    ck(len(trimmed) == 84,
       f"{SCALAR} should trim to 84 bytes, got {len(trimmed)}")


def test_trim_keeps_a_terminating_branchs_delay_slot():
    """
    A function that never returns has no `jr ra` for trim to anchor on, and the
    trailing-zero rule then ate the delay slot of the branch that DOES end it --
    `_uassert`'s `for (;;) ;` read as one instruction short of the target.

    The negative control is the ordinary case: a function whose last non-zero
    word is not a branch must still lose every trailing zero.
    """
    import struct
    _, st, img = E.load()
    s = st.get("_uassert")
    trimmed = img.func(s, trim=True)
    words = [struct.unpack_from("<I", trimmed, i)[0]
             for i in range(0, len(trimmed), 4)]
    ck(E.JR_RA not in words, "_uassert returns after all; pick another anchor")
    ck(len(words) >= 2, "trim produced too few instructions")
    ck(E.is_branch(words[-2]),
       f"expected a terminating branch, got {words[-2]:#010x}")
    ck(words[-1] == 0, f"expected the delay slot, got {words[-1]:#010x}")
    ck(len(trimmed) == 76, f"_uassert should trim to 76 bytes, got {len(trimmed)}")

    # Negative control: the branch rule must not run on a function that ends in
    # a `jr ra`, and must not resurrect padding beyond one word.
    s = st.get(SCALAR)
    tail = img.func(s, trim=True)[-8:]
    w = struct.unpack_from("<I", tail, 0)[0]
    ck(w == E.JR_RA, "the control no longer ends in jr ra")
    ck(not E.is_branch(0x24020001), "is_branch called `li` a branch")
    ck(E.is_branch(0x0c000000) and E.is_branch(0x1000fffa),
       "is_branch missed jal or b")


def test_extract_is_deterministic():
    _, st, img = E.load()
    s = st.get(SCALAR)
    ck(img.func(s) == img.func(s), "two extractions of one function differ")


def test_differ_accepts_an_identical_function():
    _, st, img = E.load()
    s = st.get(SCALAR)
    t = img.func(s)
    r = D.diff(t, t, s.name, s.addr)
    ck(r.ok, "a function did not match itself")
    ck(r.raw_identical, "identical bytes reported as needing normalisation")


def test_differ_rejects_a_single_flipped_bit():
    """Negative control.  Without this the differ is decorative."""
    _, st, img = E.load()
    s = st.get(SCALAR)
    t = img.func(s)
    for off in (0x00, 0x20, len(t) - 8):
        bad = bytearray(t)
        bad[off] ^= 0x40
        r = D.diff(t, bytes(bad), s.name, s.addr)
        ck(not r.ok, f"a flipped byte at 0x{off:X} was reported as a MATCH")
        ck(r.first_diff is not None, "a failing diff reported no offset")


def test_differ_rejects_a_length_change():
    _, st, img = E.load()
    s = st.get(SCALAR)
    t = img.func(s)
    r = D.diff(t, t[:-4], s.name, s.addr)
    ck(not r.ok, "a truncated candidate was reported as a MATCH")


def test_normalisation_does_not_fire_on_a_scalar_function():
    """If normalisation touched everything, every diff would pass."""
    _, st, img = E.load()
    s = st.get(SCALAR)
    _, rep = E.normalise(img.func(s), {"j26", "hi16lo16"})
    ck(rep.total == 0,
       f"normalisation altered {rep} words of a function with no relocations")


def test_normalisation_does_fire_when_there_is_something_to_zero():
    """Positive control for the normaliser itself."""
    code = struct.pack("<I", 0x0C123456)  # jal 0x048D158
    out, rep = E.normalise(code, {"j26"})
    ck(rep.j26 == 1, f"j26 normalisation did not fire: {rep}")
    ck(struct.unpack("<I", out)[0] == 0x0C000000, "jal target not zeroed")
    lui_lo = struct.pack("<II", 0x3C048012, 0x24843456)  # lui a0 / addiu a0
    out, rep = E.normalise(lui_lo, {"hi16lo16"})
    ck(rep.hi16 == 1 and rep.lo16 == 1, f"hi16/lo16 pairing did not fire: {rep}")


def test_image_is_unrelocated():
    """
    The whole compare-against-an-unlinked-.o strategy rests on this: call
    targets in the image are zero, exactly as they are in a fresh object.  If
    this ever fails, the candidate must be linked (or normalised) instead.
    """
    _, st, img = E.load()
    s = st.get(CALLS)
    jals = [w for _, w, _ in E.disasm(img.func(s), s.addr) if (w >> 26) == 0x03]
    ck(len(jals) == 3, f"expected 3 calls in {CALLS}, found {len(jals)}")
    ck(all(w == 0x0C000000 for w in jals),
       f"call targets are not zero: {[hex(w) for w in jals]} -- the image is "
       "relocated and the diff strategy needs revisiting")


def test_dump_is_this_images_own_link_map():
    """
    eqcalls resolves a call by reading its target out of a RESIDENT SUPPORT --
    an EE memory dump taken in-world -- and looking the address up in the same
    symbol map.  That is only sound if the dump holds the SAME BUILD as the
    un-relocated image, so this asserts the whole of the engine-code range:
    every word must either be identical or sit at a relocation shape.  A dump
    from another build would light up with `beq`/`addu`/COP1 differences, and
    the resolved names would be plausible-looking garbage.

    The negative control is the count: a comparison that found nothing to
    differ would also pass a "no strays" test, so the number of relocation
    sites is required to be large.
    """
    if not C.DUMP.is_file():
        raise Skipped(f"no resident dump at {C.DUMP}")
    _, _, img = E.load()
    d = C.load_dump(C.DUMP)
    same, diff, kinds = C.verify(img, d)
    stray = sum(n for op, n in kinds.items() if op not in C.RELOC_OPS)
    ck(stray == 0,
       f"{stray} differing words are not a relocation shape: the dump is a "
       "DIFFERENT BUILD and every name eqcalls resolves is wrong")
    ck(diff > 10000,
       f"only {diff} relocation sites; a dump that agreed everywhere would be "
       "the un-relocated image itself and would resolve nothing")
    ck(same > 200000, f"only {same} words agree; the dump does not cover SUPPORT")


def test_calls_resolves_a_known_callee_and_refuses_a_wrong_dump():
    """
    Positive: VILODSprite::Raster's four calls are named, and two of them are
    the pair that could not be inferred from the argument shape at all --
    VIScene::GetSpriteType and VIScene::GetNextColorBuffer, the second of which
    takes nothing but `this`.

    Negative: reading the SAME site out of the un-relocated image must produce
    a target of 0, i.e. the dump is doing real work; and `function_agrees` must
    reject a function whose non-relocation words have been corrupted.
    """
    if not C.DUMP.is_file():
        raise Skipped(f"no resident dump at {C.DUMP}")
    _, st, img = E.load()
    d = C.load_dump(C.DUMP)
    sym = st.get("Raster__11VILODSpriteR8VICameraR7VISceneR8VIRasterUi")
    ck(C.function_agrees(img, d, sym) == [], "the dump disagrees with the image here")
    names = [n for _, kind, n in C.calls(st, img, d, sym) if kind == "jal"]
    ck("GetSpriteType__7VIScenei" in names, f"GetSpriteType not resolved: {names}")
    ck("GetNextColorBuffer__7VIScene" in names,
       f"GetNextColorBuffer not resolved: {names}")
    ck("RasterSprite__7VISceneR8VICameraiUi" in names,
       f"RasterSprite not resolved: {names}")

    # negative control 1: the image itself resolves nothing
    jals = [w for _, w, _ in E.disasm(img.func(sym), sym.addr) if (w >> 26) == 0x03]
    ck(jals and all(w == 0x0C000000 for w in jals),
       "the image's own calls are not zero, so the dump added nothing")

    # negative control 2: corrupt one non-relocation word and it must be caught
    bad = bytearray(d)
    va = sym.addr
    while (E.disasm(bytes(bad[va:va + 4]), va)[0][1] >> 26) in C.RELOC_OPS:
        va += 4
    bad[va] ^= 0xFF
    ck(C.function_agrees(img, bytes(bad), sym) == [va],
       "a corrupted non-relocation word was not reported")


def test_vu0_census_splits_the_target_class():
    """Regression lock on the Phase 2/3 target set."""
    _, st, img = E.load()
    hits = [s for s in st.find(r"__C?10VIMatrix44")
            if s.span > 0 and img.covers(s.addr, s.span)]
    pure = [s for s in hits if E.vu0_census(img.func(s))[1] == 0]
    ck(len(hits) == 44, f"VIMatrix44 method count moved: {len(hits)} != 44")
    ck(len(pure) == 31, f"pure-scalar count moved: {len(pure)} != 31")
    ck(sum(E.vu0_census(img.func(s))[0] for s in pure) == 1707,
       "scalar instruction count moved")


def test_vu0_census_counts_something():
    """The census must not report zero VU0 everywhere -- that would hide the split."""
    _, st, img = E.load()
    tot, vu = E.vu0_census(img.func(st.get(VU0)))
    ck(vu > 0, f"{VU0} reported 0 VU0 instructions; the census is inert")
    ck(vu < tot or tot == vu, "census inconsistent")


# --- the unlinked-object path -----------------------------------------------

def _ee_tool(name):
    import os, shutil
    from pathlib import Path as P
    cand = f"{os.environ.get('PS2DEV', os.path.expanduser('~/ps2dev/ps2dev'))}/ee/bin/mips64r5900el-ps2-elf-{name}"
    if P(cand).is_file():
        return cand
    if shutil.which(f"mips64r5900el-ps2-elf-{name}"):
        return f"mips64r5900el-ps2-elf-{name}"
    raise Skipped(f"no EE {name}; set PS2DEV")


def _assemble(text, out):
    import subprocess, tempfile, os
    as_ = _ee_tool("as")
    with tempfile.NamedTemporaryFile("w", suffix=".S", delete=False) as f:
        f.write(text)
        src = f.name
    try:
        subprocess.run([as_, "-march=r5900", "-EL", "-o", out, src],
                       check=True, capture_output=True)
    finally:
        os.unlink(src)


def test_object_roundtrip_matches():
    """
    End-to-end over the .o path: real target bytes, assembled into an object,
    read back through elf_function and diffed.  Exercises everything the
    candidate side will use before any compiler is involved.
    """
    import struct, tempfile, os
    _, st, img = E.load()
    sym = st.get(SCALAR)
    data = img.func(sym)
    words = "\n".join(
        f"\t.word 0x{struct.unpack_from('<I', data, i)[0]:08x}"
        for i in range(0, len(data), 4))
    src = (f"\t.set noreorder\n\t.text\n\t.globl {SCALAR}\n"
           f"\t.type {SCALAR}, @function\n{SCALAR}:\n{words}\n"
           f"\t.size {SCALAR}, .-{SCALAR}\n")
    obj = tempfile.mktemp(suffix=".o")
    try:
        _assemble(src, obj)
        from pathlib import Path as P
        cand, sec = D.elf_function(P(obj), SCALAR)
        ck(sec == ".text", f"function landed in {sec!r}, not .text")
        ck(len(cand) == len(data), f"read {len(cand)} bytes, wrote {len(data)}")
        r = D.diff(data, cand, SCALAR, sym.addr)
        ck(r.raw_identical, f"object roundtrip did not match: {r.verdict}")
    finally:
        if os.path.exists(obj):
            os.unlink(obj)


def test_reloc_mask_is_section_scoped_and_field_exact():
    """Masking is driven by the object's OWN relocation table, and only its own.

    `readelf -r` offsets are relative to the section each relocation applies
    to, so `.rel.data` entries land inside `.text` functions' byte ranges: the
    first version of this masked 14 real instruction words in functions that
    had no text relocations at all.  Three checks with controls:

      * a .text function's sites come only from .rel.text (a .data relocation
        at the same numeric offset must not be counted);
      * the LO16 field is cleared and the rest of the word is not;
      * a difference OUTSIDE a relocation site still FAILS.
    """
    import struct, tempfile, os
    from pathlib import Path as P
    src = ("\t.set noreorder\n\t.data\nd:\t.word 0\n\t.word 0\n"
           "\t.word 0\n\t.word 0\n\t.word d\n"
           "\t.text\n\t.globl probe\n\t.type probe, @function\nprobe:\n"
           "\tlui $2,%hi(d)\n\tnop\n\tnop\n\tnop\n"
           "\taddiu $3,$2,%lo(d)\n\tjr $31\n\tnop\n"
           "\t.size probe, .-probe\n")
    obj = tempfile.mktemp(suffix=".o")
    try:
        _assemble(src, obj)
        cand, sec, at = D.elf_function_at(P(obj), "probe")
        all_r = D.elf_relocations(P(obj))
        text_r = D.elf_relocations(P(obj), sec)
        ck(len(all_r) > len(text_r),
           "the .data relocation is missing; this test proves nothing")
        ck(all(t.startswith("R_MIPS_") for _, t, _ in text_r), "bad reloc parse")
        ck(any(t == "R_MIPS_LO16" for _, t, _ in text_r), "no LO16 in .text")

        # a "target" that differs only in the LO16 field, as a linked image does
        tgt = bytearray(cand)
        off = [o for o, t, _ in text_r if t == "R_MIPS_LO16"][0] - at
        w = struct.unpack_from("<I", tgt, off)[0]
        struct.pack_into("<I", tgt, off, (w & ~0xFFFF) | 0x08C8)
        r = D.diff(bytes(tgt), cand, "probe", 0, {"j26"},
                   relocs=text_r, span=(at, at + len(cand)))
        ck(r.ok, f"reloc masking did not reconcile the LO16: {r.verdict}")
        ck(r.reloc.masked == 1, f"masked {r.reloc.masked} words, expected 1")
        ck(not r.reloc.unknown, f"unknown relocation types {r.reloc.unknown}")

        # negative control: same length, difference NOT at a relocation site
        bad = bytearray(cand)
        struct.pack_into("<I", bad, 4, 0x24020001)   # the nop after the lui
        r2 = D.diff(bytes(bad), cand, "probe", 0, {"j26"},
                    relocs=text_r, span=(at, at + len(cand)))
        ck(not r2.ok, "a non-relocation difference was masked away")

        # and the .data relocation must not be counted as a site in .text
        r3 = D.diff(cand, cand, "probe", 0, {"j26"},
                    relocs=text_r, span=(at, at + len(cand)))
        ck(r3.reloc.sites == len(text_r),
           f"{r3.reloc.sites} sites for {len(text_r)} text relocations")
    finally:
        if os.path.exists(obj):
            os.unlink(obj)


def test_unlinked_object_has_the_image_reloc_shape():
    """
    The reason the primary compare mode needs no normalisation: an unlinked
    object holds zero in exactly the fields the un-relocated image holds zero
    in.  If a future toolchain stops doing this, linked-vs-normalised becomes
    the only option and that is a strategy change, not a tweak.
    """
    import tempfile, os
    from pathlib import Path as P
    src = ("\t.set noreorder\n\t.text\n\t.globl probe\n"
           "\t.type probe, @function\nprobe:\n"
           "\tjal\tsinf\n\tnop\n"
           "\tlui\t$4, %hi(g_table)\n\taddiu\t$4, $4, %lo(g_table)\n"
           "\tjr\t$31\n\tnop\n\t.size probe, .-probe\n")
    obj = tempfile.mktemp(suffix=".o")
    try:
        _assemble(src, obj)
        data, _ = D.elf_function(P(obj), "probe")
        words = [w for _, w, _ in E.disasm(data)]
        ck(words[0] == 0x0C000000, f"jal encoded as {words[0]:#010x}, not 0x0c000000")
        ck(words[2] == 0x3C040000, f"lui %hi encoded as {words[2]:#010x}")
        ck(words[3] == 0x24840000, f"addiu %lo encoded as {words[3]:#010x}")
        relocs = D.elf_relocations(P(obj))
        kinds = {k for _, k, _ in relocs}
        ck({"R_MIPS_26", "R_MIPS_HI16", "R_MIPS_LO16"} <= kinds,
           f"expected the three relocation kinds, got {kinds}")
    finally:
        if os.path.exists(obj):
            os.unlink(obj)


def test_hi16lo16_does_not_eat_float_constants():
    """
    Most non-zero lui immediates in this engine are float constants loaded via
    lui+mtc1 (0x3F80 = 1.0f, 0x3F00 = 0.5f, 0x4000 = 2.0f), NOT address
    relocations.  A blanket hi/lo pass would rewrite 1.0f to 0.0f and make two
    different functions compare equal.
    """
    _, st, img = E.load()
    total = E.NormReport()
    for f in [x for x in st.find(r"__C?10VIMatrix44") if x.span > 0]:
        _, rep = E.normalise(img.func(f), {"hi16lo16"})
        total.hi16 += rep.hi16
        total.lo16 += rep.lo16
    ck(total.total == 0,
       f"hi16lo16 altered {total} across VIMatrix44; it is eating float "
       "constants, which would make unequal functions compare equal")


# --- codegen fingerprints ----------------------------------------------------

def test_fingerprint_is_not_inert():
    """Positive control: a synthetic chunk with one of each must count one."""
    import struct
    words = [
        0x0C000000,  # jal
        0x50000001,  # beqzl         -> likely
        0x7FBF0010,  # sq ra,16(sp)  -> sq_lq
        0x8F820000,  # lw v0,0(gp)   -> gp_rel
        0x4BE0012C,  # vsub.xyzw     -> vu0
        0x00000018,  # mult          -> multhi
    ]
    fp = F.fingerprint([b"".join(struct.pack("<I", w) for w in words)])
    for key, want in (("jal", 1), ("likely", 1), ("sq_lq", 1),
                      ("gp_rel", 1), ("vu0", 1), ("multhi", 1)):
        ck(fp[key] == want, f"fingerprint {key} counted {fp[key]}, expected {want}")
    ck(fp["insns"] == len(words), "instruction count wrong")


def test_fingerprint_rates_survive_empty_input():
    fp = F.fingerprint([b""])
    r = F.per_kloc(fp)
    ck(fp["insns"] == 0 and all(v == 0.0 for v in r.values()),
       "empty input did not produce a zero profile")


def test_engine_fingerprint_profile():
    """
    Regression lock on the build-flag readings.  These are the facts Phase 1
    and 2 select a compiler and a flag set from; if any of them moves, the
    conclusions drawn from them need revisiting rather than reusing.
    """
    _, st, img = E.load()
    engine = [s for s in st.sized if s.tag == "SUPPORT_DATA"]
    fp = F.fingerprint(img.func(s) for s in engine)
    ck(fp["insns"] > 200000, f"only {fp['insns']} instructions scanned")
    ck(fp["gp_rel"] == 0,
       f"gp-relative addressing appeared ({fp['gp_rel']}); the -G0 reading and "
       "the 'no .lit4 hazard' conclusion both need revisiting")
    ck(fp["likely"] > 2000, f"branch-likely count collapsed to {fp['likely']}")
    ck(fp["sq_lq"] > 2 * fp["sd_ld"],
       "128-bit register saves no longer dominate; the ABI reading was wrong")
    ck(fp["vu0"] > 500, f"VU0 count collapsed to {fp['vu0']}")
    ck(len(F.verdicts(fp)) >= 4, "verdict generation went quiet")


def main():
    tests = [(n, f) for n, f in sorted(globals().items())
             if n.startswith("test_") and callable(f)]
    failed, skipped = [], []
    for name, fn in tests:
        try:
            fn()
            print(f"  ok    {name}")
        except Skipped as e:
            skipped.append(name)
            print(f"  SKIP  {name}\n          {e}")
        except AssertionError as e:
            failed.append(name)
            print(f"  FAIL  {name}\n          {e}")
        except Exception as e:  # a broken test is a failure, not a skip
            failed.append(name)
            print(f"  ERROR {name}\n          {type(e).__name__}: {e}")
    passed = len(tests) - len(failed) - len(skipped)
    print(f"\n{passed}/{len(tests)} tests passed, {len(skipped)} skipped, "
          f"{CHECKS} assertions executed")
    if skipped:
        print(f"SKIPPED (these verified nothing): {', '.join(skipped)}")
    if CHECKS < 40:
        print("WARNING: assertion count is suspiciously low -- suite may be inert")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
