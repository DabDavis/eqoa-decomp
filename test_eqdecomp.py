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


def test_span_alias_names_shadow_a_real_function():
    """
    SPAN_ALIAS claims a map entry is a non-code value that equals a real
    function's entry point.  Held to: (a) the alias and the named function are
    at the SAME address, (b) the alias has span 0 and is in SymbolTable.alias,
    (c) the function keeps a non-zero span ending one word past a `jr ra`.

    Negative control: a name that is NOT at its partner's address must fail (a)
    -- VIRStrip5 against VIEEUnloadIrx__Fi.
    """
    ck(len(E.SPAN_ALIAS) > 0, "SPAN_ALIAS is empty; this test is inert")
    _, st, img = E.load()
    ck({s.name for s in st.alias} == set(E.SPAN_ALIAS),
       "SymbolTable.alias does not match SPAN_ALIAS")
    for name, real in E.SPAN_ALIAS.items():
        a, f = st.get(name), st.get(real)
        ck(a is not None and f is not None, f"{name}/{real}: not in the map")
        ck(a.addr == f.addr, f"{name} is at {a.addr:#x}, {real} at {f.addr:#x}")
        ck(a.span == 0, f"{name}: span {a.span} != 0")
        ck(f.span > 0, f"{real}: span 0 -- the alias swallowed it")
        tail = _words(img.read(f.addr + f.span - 8, 4))
        ck(E.JR_RA in tail, f"{real}: its span does not end after a jr ra")
    ck(st.get("VIRStrip5").addr != st.get("VIEEUnloadIrx__Fi").addr,
       "control: VIRStrip5 shares VIEEUnloadIrx's address; check (a) cannot discriminate")


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
    # The build profile covers ALL compiled code, so it is read with SPAN_CAP
    # off: a cap stops a TU's unnamed local code counting as part of the named
    # function before it, which is right for scoring and wrong here (phase 61's
    # caps took the scan from 219,896 to 189,583 instructions and the
    # branch-likely count from 2,486 to 1,822).
    caps = dict(E.SPAN_CAP)
    E.SPAN_CAP.clear()
    try:
        _, st, img = E.load()
    finally:
        E.SPAN_CAP.update(caps)
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


def _perm():
    try:
        import eqperm
    except ImportError as e:          # tree-sitter is the permuter's only extra
        raise Skipped(f"eqperm does not import ({e}); pip install tree-sitter tree-sitter-cpp")
    return eqperm


def test_permuter_elf_reader_agrees_with_readelf():
    """eqperm reads objects itself, to skip four subprocesses per candidate.
    Its words must be eqdiff's bytes, and its masks must sit exactly on the
    relocation sites -- a mask on a plain word would hide a real difference."""
    import tempfile, os
    from pathlib import Path as P
    perm = _perm()
    src = ("\t.set noreorder\n\t.data\nd:\t.word 0\n\t.word d\n"
           "\t.text\n\t.globl filler\n\t.type filler, @function\nfiller:\n"
           "\tjr $31\n\tnop\n\t.size filler, .-filler\n"
           "\t.globl probe\n\t.type probe, @function\nprobe:\n"
           "\tlui $2,%hi(d)\n\tnop\n\taddiu $3,$2,%lo(d)\n\tjr $31\n\tnop\n"
           "\t.size probe, .-probe\n")
    obj = tempfile.mktemp(suffix=".o")
    try:
        _assemble(src, obj)
        words, masks = perm.elf_function(P(obj), "probe")
        data, _, _ = D.elf_function_at(P(obj), "probe")
        ck(struct.pack(f"<{len(words)}I", *words) == data,
           "eqperm's reader disagrees with readelf/objcopy")
        ck(masks[0] == 0xFFFF and masks[2] == 0xFFFF, f"HI16/LO16 sites unmasked: {masks}")
        ck(masks[1] == masks[3] == masks[4] == 0, f"a plain word was masked: {masks}")
        try:
            perm.elf_function(P(obj), "absent")
            ck(False, "a missing symbol was found")
        except KeyError:
            ck(True, "")
    finally:
        if os.path.exists(obj):
            os.unlink(obj)


def test_permuter_grade_is_zero_only_for_a_match():
    """The graded score: 0 for identical words, the register penalty for a
    register-only change, and a relocation field is free ONLY where the
    candidate's own relocation says so."""
    perm = _perm()
    _, st, img = E.load()
    t = list(struct.unpack(f"<{len(img.func(st.get(SCALAR), trim=True)) // 4}I",
                           img.func(st.get(SCALAR), trim=True)))
    none = [0] * len(t)
    ck(perm.grade(t, t, none) == (0, 0), "identical words did not score 0")
    # a register-only change: rewrite the rt field of the first load or store
    k = next(i for i, w in enumerate(t) if (w >> 26) in (0x23, 0x2B, 0x31, 0x39))
    c = list(t)
    c[k] ^= 1 << 16
    ck(perm.grade(t, c, none) == (perm.PEN_REG, 1), f"register change scored {perm.grade(t, c, none)}")
    c = list(t)
    c[k] ^= 0x4                                   # its offset
    ck(perm.grade(t, c, none)[0] == perm.PEN_IMM, "immediate change misgraded")
    masked = [0] * len(t)
    masked[k] = 0xFFFF
    ck(perm.grade(t, c, masked) == (0, 0), "a relocated immediate was not forgiven")
    ck(perm.grade(t, t[:k] + [0x24020001] + t[k:], none + [0])[0] >= perm.PEN_INDEL,
       "an inserted instruction cost less than an insertion")


def test_permuter_mutations_parse_and_vary():
    """Every mutant parses, differs from its input, and many kinds fire --
    an engine whose mutations all fail would search nothing, silently."""
    perm = _perm()
    import random
    import eqperm_mut as M
    text = ("int A::F(const short *s, int n)\n{\n    int i = 0;\n    int t;\n"
            "    t = 0;\n    while (s[i])\n    {\n        if (s[i] == 10)\n"
            "            t += n * 2;\n        else\n            t = t + 1;\n"
            "        i++;\n    }\n    int m = &s[i] - s;\n"
            "    return t != 0 ? m : t;\n}\n")
    ck(not M.Fn(text).root.has_error, "the probe function does not parse")
    ck(M.Fn("int f( { return; ").root.has_error, "a broken text parsed clean")
    rng = random.Random(3)
    kinds = set()
    for _ in range(300):
        new, kind = M.mutate(text, rng)
        if new is None:
            continue
        ck(new != text, f"{kind} returned its input")
        ck(not M.Fn(new).root.has_error, f"{kind} produced a parse error:\n{new}")
        kinds.add(kind)
    ck(len(kinds) >= 12, f"only {len(kinds)} mutation kinds fired: {sorted(kinds)}")
    # a function tree-sitter cannot fully read (an attribute on an array) is
    # still searched everywhere else, and no mutant parses WORSE than it did
    odd = ("VIVect4 operator*(const VIVect3 &v, const VIMatrix44 &m)\n{\n"
           "    float in[4] __attribute__((aligned(16)));\n"
           "    VIVect4 r(v.x, v.y, v.z, 1.0f);\n    *(VIVect4 *)in = r;\n"
           "    sceVu0ApplyMatrix(r, m, *(VIVect4 *)in);\n    return r;\n}\n")
    base = M._nerr(M.Fn(odd).root)
    ck(base > 0, "control: the attribute line now parses clean, so this case tests nothing")
    hits = [M.mutate(odd, rng) for _ in range(40)]
    ck(sum(n is not None for n, _ in hits) >= 30, "a function with one unreadable line "
       "gets no mutations (every candidate was rejected for the base's own parse error)")
    ck(all(M._nerr(M.Fn(n).root) <= base for n, _ in hits if n), "a mutant parses worse")


def test_permuter_alloc_order_term():
    """The allocation-order term on TextBox's own numbers: the -1 constant
    (19 refs / 285 insns) allocated before `i` (30 / 498) takes the s5 `i`
    wants, and the gap is their relative priority difference.  Control: the
    same dumps with every pseudo already in its target register owe nothing."""
    import tempfile
    from pathlib import Path as P
    perm = _perm()
    d = P(tempfile.mkdtemp())
    (d / "t.cc.greg").write_text(
        ";; Function int f()\n;; 3 regs to allocate: 182 117 159\n"
        ";; 182 conflicts: 182 117 159\n;; 117 conflicts: 182 117 159\n"
        ";; 159 conflicts: 182 117 159\n;; Register dispositions:\n"
        "159 in 23  182 in 21  117 in 22  \n\n")
    (d / "t.cc.lreg").write_text(
        ";; Function int f()\nRegister 159 used 30 times across 498 insns; user var.\n"
        "Register 182 used 19 times across 285 insns.\n"
        "Register 117 used 21 times across 346 insns; user var.\n")

    def addu(r):                      # addu r,r,zero: two votes for r
        return (r << 21) | (r << 11) | 0x21
    # candidate i/-1/s in s7/s5/s6, the target's in s5/s6/s7
    pairs = [(addu(21), addu(23)), (addu(22), addu(21)), (addu(23), addu(22))]
    gap, bad = perm.alloc_report(d, "t", pairs)
    want = (4 * 19 / 285 - 4 * 30 / 498) / (4 * 19 / 285)
    ck(abs(gap - want) < 1e-9, f"gap {gap}, expected {want}")
    ck([(a, b) for a, b, *_ in bad] == [(182, 159)], f"inverted pairs {bad}")
    gap, bad = perm.alloc_report(d, "t", [(addu(r), addu(r)) for r in (21, 22, 23)])
    ck(gap == 0 and not bad, f"pseudos in their target registers owed {gap}: {bad}")


def test_permuter_names_and_macros():
    """The mangled-name reader finds the right definition, and the TU's macros
    expand at word boundaries only."""
    perm = _perm()
    ck(perm.split_mangled("TextBox__10VIUIWindowPCUsiiiiRC10VIColor32FUi")[:2]
       == ("TextBox", "VIUIWindow"), "member function name/class")
    ck(perm.mangled_params("PCUsiiiiRC10VIColor32FUi") == 7, "seven parameters")
    ck(perm.mangled_params("") == 0 and perm.mangled_params("v") == 0, "no parameters")
    ck(perm.split_mangled("__ml__FRC7VIVect3RC10VIMatrix44")[0] == "operator*",
       "an operator's name")
    ck(perm.mangled_names("PCUsiiiiRC10VIColor32FUi") == {"VIColor32F"}, "class names")
    m = {"CP": (["a", "b"], "f(a, b);\n    g(b);"), "K": (None, "42")}
    ck(perm.expand_macros("    CP(x + 1, K)\n", m) == "    f(x + 1, 42);\n    g(42);\n",
       "two-argument expansion, re-indented")
    ck(perm.expand_macros("CPX(1) + KK", m) == "CPX(1) + KK",
       "a macro name inside a longer identifier was expanded")


def test_permuter_macro_dump_sees_header_macros():
    """setup's macro set: SN's cpp writes NOTHING for `-dM`, so the host's cpp
    reads the same headers with the EE driver's predefines.  A header macro and
    an EE predefine must be in it; the host's own `__STDC__` family must not,
    and the dump must not be empty (the bug: every run's macros.h was)."""
    import re
    import tempfile
    from eqcheck import driver, TOOLCHAIN, SRC
    perm = _perm()
    try:
        drv = driver(TOOLCHAIN)
    except Exception as e:                       # noqa: BLE001 -- no toolchain here
        raise Skipped(f"no EE driver ({e})")
    if not (SRC / "vi.h").exists():
        raise Skipped("no src/vi.h")
    with tempfile.TemporaryDirectory() as d:
        t = Path(d) / "t.cpp"
        t.write_text('#include "vi.h"\n#define LOCAL_ONE 1\n')
        text = perm.dump_macros(drv, t)
    names = set(re.findall(r"^#define\s+(\w+)", text, re.M))
    ck(len(names) > 10, f"the macro dump is (nearly) empty: {len(names)} names")
    ck({"VIMAX", "VIMIN", "LOCAL_ONE"} <= names, "a header or TU macro is missing")
    ck(re.search(r"^#define mips 1$", text, re.M) is not None, "the EE predefine `mips` is not 1")
    ck("__STDC_VERSION__" not in names and "__STDC_HOSTED__" not in names,
       "the host cpp's own predefines leaked into the EE macro set")
    ck("NOT_A_MACRO_ANYWHERE" not in names, "control: an undefined name was reported")


def test_permuter_reduce_reverts_brace_groups():
    """`reduce` strips a find to the hunks its score needs, including a brace
    pair around an edited line, which is three hunks and builds only whole.
    The stub scorer: unbalanced braces do not build, and only the KEY line
    lowers the score."""
    perm = _perm()

    class Stub:
        def evaluate(self, text, wd):
            if text.count("{") != text.count("}"):
                return None
            # the edited line and its braces stand or fall together, as an
            # `if (...)` rewritten on the same line as the `{` it opens does
            if ("x2();" in text) != ("{\nx();" in text):
                return None
            return (0 if "    key();\n" in text else 10), 0, True

    base = "a();\nif (p)\n    b();\nc();\nd();\nw();\nx();\ny();\n"
    cand = ("a();\nif (p)\n{\n    b();\n    key();\n}\nc();\n"   # needed: a pair + key
            "{\nd();\n}\n"                                     # drift: a PAIR of hunks
            "{\nx();\nx2();\ny();\n}\n")                         # drift: THREE hunks
    ck(cand.count("{") == 3, "fixture")
    out, s = perm.reduce_text(Stub(), base, cand, Path("."), say=lambda *a: None)
    ck(s == 0 and "key();" in out, "the needed line was reverted")
    ck("x2();" not in out and out.count("{") == 1, f"neutral drift survived: {out!r}")
    # negative control: singles only, and neither brace group can go
    out1, _ = perm.reduce_text(Stub(), base, cand, Path("."), say=lambda *a: None,
                               pair_cap=0)
    ck("x2();" in out1 and out1.count("{") == 3,
       f"the control removed a brace group without group reverts: {out1!r}")


# --- other modules (MCERR first) ----------------------------------------------

def _sweep_vs_map(funcs, mod, st, img, lib):
    """(missed, split): named SUPPORT code symbols with no sweep start, and named
    functions a sweep start falls strictly inside."""
    import bisect
    starts = sorted({mod.base + s for s, _ in funcs})
    named = [s for s in st.sized if mod.base + 0x40 <= s.addr < lib and img.func(s, trim=True)]
    have = set(starts)
    missed = [s.name for s in named if s.addr not in have]
    split = []
    for s in named:
        i = bisect.bisect_right(starts, s.addr)
        if i < len(starts) and starts[i] < s.addr + len(img.func(s, trim=True)):
            split.append(s.name)
    return missed, split


def test_module_sweep_finds_every_support_function():
    """
    The sweep that seeds MCERR's (and later CLIENT's) map, checked on the one
    module whose map is REAL: every named SUPPORT code symbol must be a sweep
    start, no start may fall inside a named function, and the library tail must
    begin at crt0's `_start`.

    Negative controls, one per rule: without the branch-reach rule a function
    with two returns ends at the first (128 named functions split, measured);
    without the call-target split AND the loop rule a function that never
    returns swallows the next (2 missed: `_VIAssertf` and `_uassertf`, which
    follow a loop, so either rule alone recovers both).
    """
    import eqmodule as M
    mod, st, img = E.load("SUPPORT")
    funcs = M.sweep(img.data)
    lib = mod.base + M.library_start(img.data, funcs)
    ck(lib == 0x004C1AD8, f"library tail at {lib:#x}, not crt0's _start at 0x004C1AD8")
    missed, split = _sweep_vs_map(funcs, mod, st, img, lib)
    ck(not missed, f"no sweep start for {len(missed)} named functions: {missed[:5]}")
    ck(not split, f"the sweep splits {len(split)} named functions: {split[:5]}")
    _, split = _sweep_vs_map(M.sweep(img.data, reach_rule=False), mod, st, img, lib)
    ck(len(split) > 50, f"without the reach rule only {len(split)} functions split")
    missed, _ = _sweep_vs_map(M.sweep(img.data, split_calls=False, loop_rule=False),
                              mod, st, img, lib)
    ck(missed, "without the call-target split and the loop rule no function was missed")


def test_relocate_rebuilds_the_dump_from_the_modules_own_table():
    """
    `eqdecomp.relocate` applies SUPPORT's own relocation table at its base, and
    every word of its code must equal the in-world dump.  That is what lets the
    callee and data-reference checks run with no dump, and on modules that no
    dump holds at all.

    Negative control: the un-relocated image must differ from the dump at more
    than 10,000 code words, or agreeing proved nothing.
    """
    if not C.DUMP.is_file():
        raise Skipped(f"no resident dump at {C.DUMP}")
    mod, _, img = E.load("SUPPORT")
    d = C.load_dump(C.DUMP)
    ram = E.relocate(mod, img.data)
    _, hi = E.code_extent(img.data)
    words = range(mod.base + 0x40, mod.base + hi, 4)
    bad = [hex(va) for va in words if ram[va:va + 4] != d[va:va + 4]]
    ck(not bad, f"{len(bad)} code words differ from the dump, first at {bad[:3]}")
    raw = sum(1 for va in words if img.read(va, 4) != d[va:va + 4])
    ck(raw > 10000, f"the un-relocated image differs from the dump at only {raw} words")


def test_irx_image_relocates_every_section():
    """
    An IOP module is loaded from its .IRX (`eqdecomp.irx_image`), and its ELF
    relocations must land where loadcore puts them.  An IRX's `r_offset` is a
    MODULE address, not section-relative: SNDDRV's dispatcher jump table is
    .rodata at 0x92C0, and after relocation its first word must be the absolute
    address of case 0's body (module 0x1898), and the 100-entry table must hold
    only code addresses inside the module.

    Negative control: the un-relocated image holds the bare offset 0x1898 there,
    so agreeing is the relocation's doing; and the table site must carry a
    relocation entry, as it would not if the offset were section-relative
    (0x91A0 + 0x92C0 is past the end of .rodata).
    """
    try:
        mod, st, img = E.load("SNDDRV")
    except FileNotFoundError as e:
        raise Skipped(str(e))
    ram = E.relocate(mod, img.data)
    base = mod.base + E.IRX_HEADER                  # the module's address 0
    word = lambda va: int.from_bytes(ram[va:va + 4], "little")
    ck(word(base + 0x92C0) == base + 0x1898,
       f"jump table[0] = {word(base + 0x92C0):#x}, want {base + 0x1898:#x}")
    bad = [i for i in range(100) if not base <= word(base + 0x92C0 + 4 * i) < base + 0x91A0]
    ck(not bad, f"jump table entries outside the module's code: {bad[:5]}")
    ck(img.read(base + 0x92C0, 4) == (0x1898).to_bytes(4, "little"),
       "the un-relocated image does not hold the bare offset: the check proves nothing")
    sites = {site for site, _, _, _ in E.module_relocations(img.data)}
    ck(E.IRX_HEADER + 0x92C0 in sites, "no relocation at the jump table's site")


def test_module_imports_name_every_import_call():
    """
    MCERR names none of its own functions, but its import table names every call
    INTO SUPPORT and SLUS.  Relocated with stand-in addresses, each such call in
    its own code must resolve to that import's name through `eqcalls.calls`.

    Negative control: relocating with a stride other than the one `load` named
    the stand-ins at must lose every one of those names.
    """
    try:
        mod, st, img = E.load("MCERR")
    except FileNotFoundError as e:
        raise Skipped(str(e))
    names = E.module_imports(img.data)
    own = [s for s in st.sized if s.tag == "CODE"]
    want = {mod.base + site: names[imp] for site, kind, imp, _ in E.module_relocations(img.data)
            if kind == E.R_JAL and imp and img.data[site + 3] >> 2 == 3
            and any(s.addr <= mod.base + site < s.end for s in own)}

    def resolved(d):
        return {va: nm for s in own for va, kind, nm in C.calls(st, img, d, s) if kind == "jal"}

    got = resolved(E.relocate(mod, img.data))
    ck(len(want) > 20, f"only {len(want)} import calls in MCERR's own code")
    wrong = [(hex(va), nm, got.get(va)) for va, nm in want.items() if got.get(va) != nm]
    ck(not wrong, f"{len(wrong)} import calls misnamed: {wrong[:3]}")
    stride = E.IMPORT_STRIDE
    try:
        E.IMPORT_STRIDE = 8
        bad = resolved(E.relocate(mod, img.data))
    finally:
        E.IMPORT_STRIDE = stride
    ck(all(bad.get(va) != nm for va, nm in want.items()),
       "a mismatched stride still named the imports: the names do not come from the relocations")


def test_mcerr_base_agrees_with_its_region_dump():
    """
    MCERR's base, 0x002A8180, is where it sat in the one dump that holds it.
    Relocated there, every LOCAL relocation site in its code must equal that
    dump's word (imports are stand-ins here, so they are not compared).

    Negative control: 0x10010 bytes off, which moves every field kind, no site
    may still agree.
    """
    path = E.ROOT / "claude-eqoa-iso/decrypted_modules/MCERR_SNR1_0x002A8180.bin"
    mod = E.MODULES["MCERR"]
    if not path.is_file() or not mod.image.is_file():
        raise Skipped(f"no MCERR image or region dump ({path.name})")
    data, region = mod.image.read_bytes(), path.read_bytes()
    local = [site for site, kind, imp, _ in E.module_relocations(data)
             if not imp and kind in (E.R_JAL, E.R_HI16, E.R_LO16)]

    def agree(base):
        ram = E.relocate(E.Module(mod.name, mod.image, mod.symbols, base), data)
        return sum(1 for s in local if ram[base + s:base + s + 4] == region[s:s + 4])

    ck(len(local) > 200, f"only {len(local)} local code relocations")
    n = agree(mod.base)
    ck(n == len(local), f"{len(local) - n} of {len(local)} local sites disagree at {mod.base:#x}")
    ck(agree(mod.base + 0x10010) == 0, "a wrong base still agrees: the comparison is vacuous")


def test_eqmodule_selects_the_modules_paths():
    """
    EQMODULE=MCERR moves SRC/BUILD/MATCHED under modules/mcerr and puts SUPPORT's
    headers on the compiler's path.  Unset, SUPPORT is untouched.  An unknown
    module is refused, never quietly scored against SUPPORT.
    """
    import os
    import subprocess
    here = Path(__file__).resolve().parent
    probe = "import eqcheck as C; print(C.SRC); print(' '.join(C.compile_cmd('x')))"

    def run(module):
        env = {k: v for k, v in os.environ.items() if k != "EQMODULE"}
        if module:
            env["EQMODULE"] = module
        return subprocess.run([sys.executable, "-c", probe], cwd=here, env=env,
                              capture_output=True, text=True)

    r = run("MCERR")
    ck(r.returncode == 0, f"EQMODULE=MCERR does not import: {r.stderr[-300:]}")
    src, cmd = r.stdout.splitlines()[:2]
    ck(src == str(here / "modules/mcerr/src"), f"MCERR's SRC is {src}")
    ck(f"-I{here / 'src'}" in cmd.split(), "MCERR's TUs do not get SUPPORT's headers")
    src, cmd = run(None).stdout.splitlines()[:2]
    ck(src == str(here / "src") and " -I" not in cmd, "unset EQMODULE moved SUPPORT's paths")
    ck(run("NOSUCH").returncode != 0, "an unknown EQMODULE was accepted")


def test_placeholder_names_are_recognised_exactly():
    """A seeded `func_<VA>` / `data_<VA>` is unnamed to the checkers; a real name never is."""
    ck(E.is_placeholder("func_002A8280") and E.is_placeholder("data_002B1080"),
       "a seeded placeholder is not recognised")
    ck(not E.is_placeholder("func_2A8280"), "a short address passed as a placeholder")
    ck(not E.is_placeholder("Draw__10VIWndMcErr"), "a real name taken for a placeholder")


def test_module_sweep_ends_a_loop_that_never_returns():
    """
    SETUP +0x22C18 calls a function in a loop that never exits (`b` back, no
    `jr ra`), and nothing calls the function after it, so only the loop rule can
    start one at +0x22C40.

    Negative control: without the loop rule +0x22C40 is not a start.  Positive
    control on SUPPORT, with the call/export splits switched off so the rule has
    work to do: every start it adds must be a real start -- a named function, or
    one the full sweep starts (six, measured; one is a static after playPss's).
    """
    import eqmodule as M
    mod = E.MODULES["SETUP"]
    if not mod.image.is_file():
        raise Skipped(f"no SETUP image at {mod.image}")
    data = mod.image.read_bytes()
    ck(0x22C40 in {s for s, _ in M.sweep(data)}, "the loop rule did not start SETUP +0x22C40")
    ck(0x22C40 not in {s for s, _ in M.sweep(data, loop_rule=False)},
       "+0x22C40 is a start without the loop rule: this test no longer tests it")
    smod, st, img = E.load("SUPPORT")
    real = {s.addr - smod.base for s in st.symbols} | {s for s, _ in M.sweep(img.data)}
    off = {s for s, _ in M.sweep(img.data, split_calls=False, loop_rule=False)}
    added = {s for s, _ in M.sweep(img.data, split_calls=False)} - off
    ck(len(added) >= 5, f"the loop rule adds only {len(added)} starts to an unsplit SUPPORT")
    wrong = sorted(hex(smod.base + s) for s in added - real)
    ck(not wrong, f"the loop rule starts a function where SUPPORT has none: {wrong[:5]}")


def test_assert_hints_read_the_file_off_the_relocation():
    """
    A seeded hint names the file an assert passes, read off the LO16 relocation:
    CLIENT +0xF5BE0 asserts `Old(k-1) < Old(k)` from VIKeyDiff.h:118 through a
    `lui` hoisted out of its loop (the old claude-eqoa-iso map said
    VIMemStreamArray.h:118), and +0x57408 is a `_VIAssertf` whose line goes to
    `__VIAssertf_line`, not a2 (VIStringTable.cpp:245; the old map said
    VIMemStreamVector.h).

    Negative control: +0x2B628 holds no assert -- the old map anchored one there
    because its function ran on into the next -- and must get no assert hint.
    """
    import eqmodule as M
    mod = E.MODULES["CLIENT"]
    if not mod.image.is_file():
        raise Skipped(f"no CLIENT image at {mod.image}")
    data = mod.image.read_bytes()
    h, count = M.hints(mod, data, M.sweep(data))
    ck(count["assert"] > 70, f"only {count['assert']} CLIENT functions assert")
    ck("VIKeyDiff.h:118" in h.get(0xF5BE0, []), f"+0xF5BE0 hints {h.get(0xF5BE0)}")
    ck("VIStringTable.cpp:245" in h.get(0x57408, []), f"+0x57408 hints {h.get(0x57408)}")
    ck(not any(":" in x for x in h.get(0x2B628, [])), f"+0x2B628 hints {h.get(0x2B628)}")


def test_old_map_labels_skip_jump_table_cases():
    """
    The old STATION map's vtable pass read the jump tables at +0x9D520 as a
    VIWindowManager vtable, so its `vfunc3` is a case label at +0x3D64, inside
    the function at +0x3190.  Such a label must be dropped, not moved onto the
    function; a label that IS a start is kept.

    Negative control: the label kept is one the old map placed on a real start.
    """
    import eqmodule as M
    mod = E.MODULES["STATION"]
    if not mod.image.is_file() or not (E.ROOT / M.OLD_MAP.format("STATION")).is_file():
        raise Skipped("no STATION image or old map")
    data = mod.image.read_bytes()
    h, count = M.hints(mod, data, M.sweep(data))
    ck(count["old_dropped"] > 300, f"only {count['old_dropped']} old labels dropped")
    ck(not any("VIWindowManager::vfunc3" == x[4:] for x in h.get(0x3190, [])),
       "a jump-table case label was moved onto the function holding the switch")
    ck("old:VIWnd::method_002B7978" in h.get(0x2B7978 - mod.base, []),
       "a label on a real start was not kept")


def test_symbol_map_row_takes_a_trailing_hint():
    """
    The parser SKIPS a row it cannot match, so a hint the regex refused would
    drop the row without a word.  A `# hint` after the name must parse to the
    same address and name as the row without it.

    Negative control: a row with a second bare word is still refused.
    """
    rx = E.SymbolTable._LINE
    plain = rx.match("0x00BBD430  [CODE]  func_00BBD430")
    hinted = rx.match("0x00BBD430  [CODE]  func_00BBD430  # VIItemList.h:326; old:X::y")
    ck(plain and hinted, "a hinted row does not parse")
    ck(plain.groups() == hinted.groups(), f"the hint changed the row: {hinted.groups()}")
    ck(not rx.match("0x00BBD430  [CODE]  func_00BBD430  stray"), "a two-word row was accepted")


def _client_ready():
    mod = E.MODULES.get("CLIENT")
    if mod is None or not mod.image.is_file():
        raise Skipped("no CLIENT image")
    return mod


def test_eqtmpl_names_a_known_map_and_rejects_the_wrong_type():
    """
    eqtmpl's walk names every member of CLIENT's VIMap<int, VIBuddy> from its Find alone:
    16 members, each byte-exact with every call landing on the member it names.

    Negative control: the same seed solved as VIMap<int, VIGuildMember> (a 68-byte node
    against the buddy map's 192) must name nothing -- not even the seed.
    """
    import json, os, subprocess, tempfile
    _client_ready()
    env = dict(os.environ, EQMODULE="CLIENT")
    here = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory() as tmp:
        def solve(targs):
            out = Path(tmp) / (targs.replace(", ", "_") + ".json")
            subprocess.run([sys.executable, str(here / "eqtmpl.py"), "solve", targs, "Find=0x00BE1F68",
                            "--work", str(Path(tmp) / targs.replace(", ", "_")), "--json", str(out)],
                           env=env, capture_output=True, text=True, timeout=600)
            return json.loads(out.read_text()) if out.is_file() else {}
        got = solve("int, VIBuddy")
        ck(len(got) == 16, f"VIMap<int, VIBuddy> named {len(got)} members, not 16")
        ck(got.get("0xbe1f68") == "Find__Ct5VIMap2ZiZ7VIBuddyRCi", "the seed itself was not named")
        ck(got.get("0xbe2c10") == "Erase__t5VIMap2ZiZ7VIBuddyi", "Erase was not reached")
        ck(len(solve("int, VIGuildMember")) == 0, "a wrong value type still named members")


def test_duplicates_are_exact_copies_only():
    """
    eqcheck credits a dead TU copy to the function it duplicates: the guild map's matched
    Find (0x00BF0AB0) has a byte-identical, never-called copy at 0x00BF20A8.

    Negative control: the buddy map's Find has the same code with another node stride
    (192, not 68) and must claim nothing at 0x00BF20A8.
    """
    import eqcheck
    _client_ready()
    _, st, img = E.load("CLIENT")
    dups, _ = eqcheck.verified_duplicates(st, img, ["Find__Ct5VIMap2ZiZ13VIGuildMemberRCi"], set())
    copy = st.at(0x00BF20A8).name
    ck(dups.get(copy) == "Find__Ct5VIMap2ZiZ13VIGuildMemberRCi", f"{copy} not credited to the guild Find")
    other, _ = eqcheck.verified_duplicates(st, img, ["Find__Ct5VIMap2ZiZ7VIBuddyRCi"], set())
    ck(copy not in other, "a Find with another stride claimed the guild copy")


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
