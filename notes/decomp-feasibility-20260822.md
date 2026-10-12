# A matching decomp of EQOA is feasible — two functions are byte-exact (2026-08-22)

**Status: PROVED, at the smallest honest scale.** Two `VIMatrix44` methods now rebuild
byte-for-byte from C++ source, with the original compiler, verified by `cmp` and by a
differ that applied **zero** normalisation. That settles the feasibility question the
`go-eqoa-vi` hand-port has been implicitly betting against. It does **not** yet say the
approach scales; see *What this does not establish*.

Tooling and the reproduction recipe live in [`decomp/README.md`](../decomp/README.md).
This note is the findings, including what was refuted.

## Why this was worth asking

Prompted by reading a friend's GoldenEye 007 decomp+port
(see `goldeneye-port-lessons-20260822.md`). That project keeps 100% of the game's C
unmodified and shims only the N64 hardware — possible because a matching decomp is its
oracle. The question here was whether EQOA could have the same thing.

The `go-eqoa-vi` port currently pays a **per-function trap tax**: each ported function needs
its own conformance corpus plus discovery of whatever float-model quirk it trips (the EE
operand clamp, the two square roots, this build's pi split, `_LIB_VERSION`). After months it
covers ~156 of SUPPORT's 4,294 exported symbols, ~3.6%. A matching decomp makes those the
compiler's problem instead of ours.

## The compiler is `ee-gcc 2.95.x`, and it was never in doubt after the first look

Determined from the symbol tables alone, before any compilation:

- **GNU C++ 2.x mangling** — `Add__10VIMatrix33RC10VIMatrix33T1`: no ARM `F` between class
  and arguments, `T1` back-reference, `Ui` for unsigned. CodeWarrior would emit
  `Add__10VIMatrix33FRC...`.
- `_GLOBAL_$I$` / `_GLOBAL_$D$` static init/destruct, `_$_<len><Class>` destructors,
  `__tf<n>` typeinfo, `__rtti` — all GNU 2.x only, and RTTI is **on**.
- libgcc's `__divdi3`, `__fixsfdi`, `__do_global_ctors` in the module.

**GCC 3.2 is excluded conclusively and not by scoring**: it emits Itanium ABI
(`_ZN10VIMatrix449TransposeERKS_`) where 2.95.3 emits `Transpose__10VIMatrix44RC10VIMatrix44`,
character for character the target symbol.

**No compiler had to be built.** [decompme/compilers](https://github.com/decompme/compilers/releases)
ships 15 prebuilt EE toolchains (2.9x / 2.95x / 2.96 / 3.2, plus CodeWarrior). Seven run
natively as i386 ELF; the rest under wine. The container-build plan was unnecessary — worth
remembering before anyone budgets a week for it.

## The two matches

| Function | Bytes | Result |
|---|---:|---|
| `Scale__10VIMatrix44f` | 60 | exact, first attempt |
| `Transpose__10VIMatrix44RC10VIMatrix44` | 84 | exact, after finding the pointer hoist |

`ee-gcc2.95.2-273a` / `2.95.3-107` / `2.95.3-114`, `-O2`, each using **its own bundled
assembler**. Source facts recovered, which are the real product:

- **`VIMatrix44::m` is `float m[4][4]`**, not `float m[16]`.
- **`Transpose` hoists `m` into a local `float (*d)[4]`.** Without the hoist one `addu`
  comes out with its operands reversed, and *nothing else fixes it* — five 2.95 patchlevels,
  four 2.9 builds, 2.96, 3.2, ~25 source forms and ~20 flag combinations were swept to
  establish that.
- **`Scale` writes the diagonal in index order** (`[0][0], [1][1], [2][2]`); the object code
  emits `[2][2], [0][0], [1][1]`. That reordering is the compiler's scheduler, and it
  reproduced exactly — do not read an emitted order back as source order.

## Traps, each of which cost real time

**Use each toolchain's bundled assembler, never modern GAS.** `move` is an assembler macro:
modern GAS expands it to `or` (`0x25`), the period assembler to `daddu` (`0x2d`), which is
what the target holds. Switching assemblers alone fixed every `move`, the load-delay `nop`
and every branch offset at once. On modern GAS this reads as pervasive codegen failure, and
no compiler flag touches it — I burned a flag sweep looking in the wrong place.

**Trailing zero words are not all padding.** A "strip trailing zeros" trim eats the `nop` in
the delay slot of the terminating `jr ra`. That made a byte-exact `Transpose` report as a
one-instruction failure for several rounds. The function ends one word after the last
`jr ra`; only zeros beyond that are alignment.

**Most non-zero `lui` immediates are float constants, not relocations.** `0x3F80` is 1.0f,
`0x3F00` is 0.5f, `0x4000` is 2.0f, loaded `lui`+`mtc1` because their low half is zero. A
blanket hi/lo relocation-normalisation pass rewrites 1.0f to 0.0f and makes genuinely
different functions compare equal. Keep such normalisation opt-in and counted **by kind**.

**A compiler emitting `li.s $f1,1.0` hides the pattern.** Grepping `.s` text for `mtc1`
found zero and suggested the compiler was wrong; the macro expands to `lui`+`mtc1` only at
assembly. Fingerprint the machine code, not the assembly text.

**The symbol map is merged and its tags are backwards.** `[SUPPORT_DATA]` (3,700 symbols) is
the *engine code*; `[SUPPORT_CODE]` (399) is *libgcc and crt*. Both are code. A further 195
`[SLUS]`/`[OTHER]` symbols live in the main executable's address space and, left alone,
acquire derived spans reaching into the next region — `_stack_size` came out 512 KB long.

**`pkill -f` self-kill, again.** The pattern matched the invoking shell's own command line.
Kill by PID. (Already in memory; re-learned anyway.)

## Refuted along the way

- **"A matching decomp needs a compiler we cannot get."** False — 15 are prebuilt.
- **"`-G0` and the flag set are the hard part."** No flag closed either function; the
  assembler and the source shape did. Flags were settled in one sweep (`-O2`, and `-O1` is
  ~2/20 everywhere).
- **"The `.lit4` literal-pool hazard applies."** decomp.wiki documents a nasty trap where
  substituting an `extern float` for a literal changes codegen. **There is not one
  gp-relative access in this module** (0 in 219,910 instructions), so Phase 3 does not need
  TU-wide literal migration before individual functions can match.
- **"19/20 means the compiler is slightly wrong."** It meant the *source* was slightly wrong.
  Testing a second function is what distinguished those, and it is the method to reuse: one
  function cannot tell "wrong compiler" from "unusual function".

## Fingerprints of the target build (`decomp/eqfingerprint.py`)

Over the whole engine region, 3,700 functions, 219,910 instructions:

| Fingerprint | Target | Reading |
|---|---:|---|
| gp-relative addressing | **0** | no small-data area (`-G0`) |
| branch-likely | 2,486 | the older 2.9x/2.95x line |
| `sq`/`lq` vs `sd`/`ld` on sp | **22,766 vs 483** | EE 128-bit GPR save ABI |
| `lui`+`mtc1` inline float | 839 | high-half-only constants |
| COP2 (VU0 macro mode) | 909 | hand-written asm, score separately |

A candidate that saves registers with `sd`/`sw` mismatches nearly every non-leaf function in
its prologue, before its body is reached. Check that first.

## What this does not establish

Two small functions from one class. `VIMatrix44` alone has 31 pure-scalar methods /
1,707 instructions, and `Store__C10VIMatrix44Pf` is at 14 wrong on a first guess (its inner
loop is unrolled x4 and that shape is not matched yet). SUPPORT has 4,294 symbols; the five
modules together have 14,722 functions and ~2.9 MB of code, in C++. **A full-tree matching
decomp is not something this project finishes.**

The useful target is narrower and worth pricing: **SUPPORT alone**, which is what
`go-eqoa-vi` is hand-porting one function at a time. Note also that scoring must keep the
**pure-scalar and VU0-asm subsets separate** — 13 of VIMatrix44's 44 methods are
hand-written VU0 macro mode, and folding transcribed assembly into one percentage turns a
null result into a claimed win.

## If the port is ever the goal rather than the reference

The GoldenEye shape — decomp plus a hardware shim — would give the real EQOA client running
natively, obsoleting the EQOABOOT/EQOA-QOL instruction-patch line. Two honest costs: the
GS/VU shim is a much bigger job than the RDP one (their `fast3d.c` is 3.8 KLOC of a 10.9
KLOC shim), and **the EE float model comes back as a host divergence** the way byte order did
for them — chopping, denormal flush, saturation instead of infinity, the exponent-255 operand
clamp. That lands in one place rather than per function, and `vimath` already *is* that
model, but it does not disappear.

## Next

Phase 3: the remaining 29 pure-scalar `VIMatrix44` methods. Sweep with
`decomp/eqsweep.py`. **Amber if the set plateaus below ~80%** — that would mean per-function
permuting is needed, which restores a linear cost curve and weakens the case against simply
continuing the hand-port.
