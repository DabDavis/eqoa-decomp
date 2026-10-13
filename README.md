# `decomp/` — matching decomp of the EQOA PS2 modules (**3,508 of 3,756 byte-exact**, phase 61 in progress)

Phase 0 of the experiment scoped in the VIMatrix44 discussion: extract any
function out of a decrypted module image, disassemble it, and diff it against
a candidate build. **No compiler is required for anything in this directory** —
it is offline tooling, and it is useful for ordinary RE work whether or not the
decomp goes ahead.

```bash
./test_eqdecomp.py                                  # 23 tests, 71 assertions
./eqd.py census '__C?10VIMatrix44' -v               # scalar / VU0-asm split
./eqd.py show   Transpose__10VIMatrix44RC10VIMatrix44
./eqd.py list   'VIMatrix44'                        # size + VU0 mix per symbol
./eqd.py extract RotateX__10VIMatrix44f -o /tmp/f.bin
./eqd.py diff   Transpose__10VIMatrix44RC10VIMatrix44 --obj build/matrix44.o
```

| File | What |
|---|---|
| `eqdecomp.py` | symbol table, image access, relocation normalisation, disassembly |
| `eqdiff.py` | the differ; reads a function out of an unlinked ELF object |
| `eqd.py` | CLI: `list` `show` `extract` `census` `diff` |
| `eqfingerprint.py` | codegen fingerprints — narrows compiler + flags before matching |
| `eqrtl.py` | which PASS made a difference — gcc's own `-da` dumps, no source needed |
| `eqcalls.py` | resolves a function's CALLEES by name, out of a resident SUPPORT |
| `test_eqdecomp.py` | the canary — see below, it is the point |
| `_q.py` | one TU, named symbols, 4.5 s — for sweeping source variants |
| `_tu.py` | one TU, EVERY function in it, plus regressions against `MATCHED.txt` |
| `_perm.py` | permute a run of source lines, rebuild one TU, score one symbol |
| `_lane.py` | isolated copies of `src/` + `build/`, so several functions are ground at once |

## Running it outside this project tree

This directory is also published on its own, as
[github.com/DabDavis/eqoa-decomp](https://github.com/DabDavis/eqoa-decomp), with the
phase notes the text cites as `claude-notes/decomp-*.md` under `notes/`. The
repository carries **no game code or data from the disc, no memory dump and no
compiler** — the inputs it is scored against are supplied by whoever runs it:

| Input | Put it at | Comes from |
|---|---|---|
| `SUPPORT_decompressed_SNR1.bin`, `SUPPORT_symbol_map.txt` | `$EQOA_ROOT/claude-eqoa-iso/decrypted_modules/` | the SUPPORT module, decrypted and decompressed off your own *EverQuest Online Adventures: Frontiers* disc (SLUS-20744) |
| the compiler and the assembler | `toolchains/ee-gcc2.95.3-107/` and `toolchains/ee-gcc2.95.3-114/` | the [decompme/compilers](https://github.com/decompme/compilers/releases) releases (the second is only for its SN `ps2eeas.exe`) |
| an EE memory dump taken in-world | `$EQOA_ROOT/memory-dumps/inworld-eqoa.live.eememory` | PCSX2; only `eqcalls.py` and two tests read it, and they skip loudly without it |
| `objdump` for the R5900 | `$PS2DEV/ee/bin/` (default `~/ps2dev/ps2dev`) | the ps2dev toolchain |
| `wine` | on `PATH` | runs the SN assembler |

`EQOA_ROOT` defaults to this directory's parent. `./test_eqdecomp.py` is the first
thing to run: it fails loudly if the image is not the one these results were
measured against.

**License scope.** The MIT license (`LICENSE`) covers this repository's own work —
the tools, the reconstructed source and the notes. It grants no rights in
*EverQuest Online Adventures* itself: the game, its binaries and its data belong to
their owners, and none of them are distributed here.

**For the rights holder.** This is a preservation project. The game's original
source is understood to be lost, and this repository is an attempt to recover it —
function by function, each one verified byte-for-byte against the shipped module,
with the notes recording how. It carries no disc data, no binaries and no SDK
code. If you would like it taken down, or would find it useful, please
[open an issue](https://github.com/DabDavis/eqoa-decomp/issues); a takedown request
will be honoured promptly.

**One paragraph per phase, all 56 of them, is
[`claude-notes/decomp-phase-log.md`](../claude-notes/decomp-phase-log.md)** — the roll-up
that used to sit inline in the root `CLAUDE.md`. Use it to find *which* phase settled a
thing; the section for that phase below, and the phase's own note in `claude-notes/`, are
where the working is. Phases 20, 29 and 37 have no section here — see the log and their
note files.

## Facts about the oracle, established and asserted

**The image is `SUPPORT_decompressed_SNR1.bin`, indexed by `VA − 0x003E3D00`.**
Verified: `Identity__10VIMatrix44` (0x003F01F0) decodes to VU0 macro mode ending
in `jr ra`. `SUPPORT_code.bin` is *not* this and does not index that way — it
decodes to a repeating pattern at the same offset. Use the SNR1.

**The image is UN-RELOCATED, and that decides the compare strategy.** Call
targets are `0x0C000000` and hi/lo address pairs hold zero. An unlinked `.o`
holds *exactly the same bytes* at those sites (measured: `jal sinf` →
`0c000000` + `R_MIPS_26`; `lui %hi` → `3c040000` + `R_MIPS_HI16`). So compare
against an **unlinked object** and no normalisation is needed at all — a clean
result is earned rather than manufactured. `test_image_is_unrelocated` and
`test_unlinked_object_has_the_image_reloc_shape` fail loudly if either half of
that stops being true.

**The symbol map's tags do not mean what they say.** `[SUPPORT_DATA]` (3,700
symbols, 0x003E3D00–0x004C1CA8) is the *engine code*; `[SUPPORT_CODE]` (399,
0x004C1D00–) is *libgcc and crt* (`__do_global_ctors`, `__divdi3`, `__fixsfdi`).
Both are code. Do not filter on the tag.

**The map is merged, and 195 symbols are not in this image.** 191 `[SLUS]` plus
4 `[OTHER]` live in the main executable's address space. Left alone they
acquire a derived span reaching into the next region — `_stack_size` came out
512 KB long. They are given span 0 and listed in `SymbolTable.foreign`. The
canary caught this on its first run.

**Sizes are derived from the gap to the next symbol,** so a span includes any
alignment padding. `--trim` drops trailing zero words; both forms are available
because which one is right depends on whether the candidate is padded too.

## The dump is the image's own link map

The image being un-relocated costs one thing: it cannot say WHICH function a
call goes to, so for nineteen phases a callee was named from its argument shape
and the plausibility of the name.  That price does not have to be paid.
**SUPPORT is a resident module loaded at the same virtual addresses this image
is indexed by**, so an EE memory dump taken in-world holds the RELOCATED copy of
the very same build:

```
$ ./eqcalls.py verify
  209610/227290 words identical (92.22%), 17680 differ
     14488  jal    1451  lui    1133  addiu    328  lw    218  sw   ... 
  0 differing words are NOT a relocation shape
$ ./eqcalls.py calls Raster__11VILODSpriteR8VICameraR7VISceneR8VIRasterUi
  0x00452a94  jal   GetModelView__C8VIRaster
  0x00452b08  jal   GetSpriteType__7VIScenei
  0x00452b18  jal   GetNextColorBuffer__7VIScene
  0x00452b70  jal   RasterSprite__7VISceneR8VICameraiUi
```

Every one of the 17,680 differences is a relocation site, so "same build" is a
measurement rather than an assumption, and `test_dump_is_this_images_own_link_map`
fails loudly if a dump from another build is pointed at it.  `calls` additionally
refuses a function whose non-relocation words disagree.  It corrected two names
that had stood since Phase 10 — `VIScene::GetSprite` is `Sprite`, and
`VIDictionary::Release` is `Remove` — and named a call that could not have been
guessed at all, `VIScene::GetNextColorBuffer`, which takes nothing but `this`.

## Normalisation, and why it is opt-in

`j26` (zero `j`/`jal` targets) is safe by construction and is the default for
`diff`. `hi16lo16` is a **heuristic pairing pass and must stay opt-in**:

> Most non-zero `lui` immediates in this engine are *float constants*, not
> address relocations — `0x3F80` is 1.0f, `0x3F00` is 0.5f, `0x4000` is 2.0f,
> loaded via `lui`+`mtc1`. A blanket hi/lo pass rewrites 1.0f to 0.0f and makes
> two genuinely different functions compare equal.

The pairing set deliberately excludes COP1, so it touches none of the 25 float
constants in VIMatrix44 today — `test_hi16lo16_does_not_eat_float_constants`
locks that in. **Every normalised word is counted by kind, never as one total**,
so a diff that only passed because normalisation erased the difference is
visible in the report.

## The canary

A checker that cannot fail reports success having compared nothing. Each test
pairs a positive control with a negative one, the runner prints the number of
assertions actually executed, and a skip is reported loudly rather than
counting as a pass. Notable pairs:

- base address correct (`Identity` decodes to `vsub`/`sqc2`/`jr ra`) **and** a
  read one word off must *not* match — otherwise the check is vacuous;
- an identical function matches **and** a single flipped bit at three
  different offsets must FAIL, **and** a truncated candidate must FAIL;
- normalisation must not fire on a function with no relocations **and** must
  fire on a synthetic `jal`/`lui`+`addiu` that does have them;
- the VU0 census must report >0 VU0 in `Identity` — a census stuck at zero
  would hide the scalar/asm split entirely.

`test_vu0_census_splits_the_target_class` is a regression lock on the Phase 2/3
target set (44 methods / 31 pure-scalar / 1,707 scalar instructions).

## The target set

```
pure scalar (codegen test)       31 funcs   7144 bytes   1707 insns
contains VU0 asm                 13 funcs   1808 bytes    431 insns
TOTAL                            44 funcs   8952 bytes   2138 insns
```

**Report those two rows separately, always.** The VU0 functions are transcribed
hand-written assembly — matching them proves nothing about the compiler, and
folding them into one percentage would turn a null result into a claimed win.

Phase 2's calibration function is `Transpose__10VIMatrix44RC10VIMatrix44`:
88 bytes, 18 instructions, a real double loop, no calls, no relocations, no
asm. Semantics are not in question — `go-eqoa-vi/vimath` has 30 `Matrix44`
methods already proven bit-exact against the console — so a mismatch means
codegen, not misunderstanding.

## The address interleave — asked in Phase 0, and it does not matter

`VIMatrix44` is split across two address ranges with the whole of `VIMatrix33`
between them: 0x3F0178–0x3F0790, then VIMatrix33, then 0x3F1590–0x3F3300.
Either a shared `VIMatrix.cpp`, or GCC 2.x emitting out-of-line copies of
in-class-defined functions into a separate section.

**Phase 4 settled the practical half without settling the question**: both classes
are matched from SEPARATE translation units (`src/vimatrix44.cpp`,
`src/vimatrix33.cpp`) and 49 symbols are byte-exact that way, so whatever the
real file layout is, it does not reach the code generator for these functions.
Nothing seen so far distinguishes the two readings.

## Build-flag readings (`./eqfingerprint.py`)

Measured over the whole engine region — 3,700 functions, 219,910 instructions.
Per-function diffing is an expensive way to discover a wrong flag; these
constrain the search first, and `eqfingerprint.py --obj X.o --compare PATTERN`
scores a candidate build against the target the same way.

| Fingerprint | Target | Reading |
|---|---:|---|
| gp-relative addressing | **0** | built with no small-data area (`-G0`) |
| branch-likely | 2,486 | the older SN 2.9x/2.95x line |
| `sq`/`lq` vs `sd`/`ld` on sp | 22,766 vs 483 | EE 128-bit GPR save ABI |
| `lui`+`mtc1` inline float | 839 | high-half-only constants materialised |
| `lui`+`lwc1` absolute float | 58 | a literal pool, reached without `$gp` |
| COP2 (VU0 macro mode) | 909 | hand-written asm, scored separately |

Three of those change the plan:

**The `.lit4` hazard does not apply to us.** decomp.wiki documents a nasty
matching trap where floating-point literals migrate to `.lit4` and are loaded
`%gp_rel`, and substituting an `extern float` for the literal *changes codegen*
(a missing `nop`), so a function cannot be matched until the whole TU's literal
pool is migrated. **There is not one gp-relative access in this module.** Float
constants are materialised inline where the low half is zero — 1.0f `0x3F80`,
0.5f `0x3F00`, 2.0f `0x4000` — and otherwise loaded absolutely. Phase 3 does
not need TU-wide literal migration before individual functions can match.

**`-G0` is a settled flag, not a guess**, and it follows from the same
measurement.

**A candidate that saves registers with `sd` or `sw` will mismatch nearly every
non-leaf function**, in the prologue, before any of its body is even reached.
`sq`/`lq` outnumber `sd`/`ld` 47:1. Get this right before reading any diff.

Also worth carrying into Phase 3: decomp.wiki's C++ note that `bool` and `int`
produce *different load/store ordering* in this GCC line, so a mismatched
operand order around a call is worth trying as a `bool` field before it is
worth restructuring the code.

## Phase 1 result: the compiler is `ee-gcc 2.95.x`, and it is on disk

`toolchains/` holds 13 prebuilt EE compilers from
[decompme/compilers](https://github.com/decompme/compilers/releases) — the whole
2.9x / 2.95x / 2.96 / 3.2 range. **No compiler had to be built**: 7 are native
Linux i386 ELF (this host has `/lib/ld-linux.so.2`) and 5 are Windows PE32 run
under the installed `wine`. 11 of 12 have a working C++ path; the 12th
(`3.2-030210-beta2`) wants `cygwin1.dll` and is redundant.

**Never modern GAS** (`move` expands to `or` `0x25`, where the period assemblers
give `daddu` `0x2d`, which is what the target holds; on the first real function
switching assemblers alone fixed every `move`, the load-delay `nop` and every
branch offset). **And not the bundled GNU `as` either — see Phase 3 below: the
assembler is SN Systems' `Ps2EeAs`.**

### Scored on a real function

`Transpose__10VIMatrix44RC10VIMatrix44`, 20 instructions, against this source:

```cpp
class VIMatrix44 { public: float m[4][4]; void Transpose(const VIMatrix44 &a); };
void VIMatrix44::Transpose(const VIMatrix44 &a)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            m[j][i] = a.m[i][j];
}
```

| Toolchain | `-O2` result |
|---|---|
| **2.95.2-273a / 2.95.3-114 / 2.95.3-136** | **19 of 20 instructions exact** |
| 2.9-990721 / 2.9-991111 (all variants) | ~16 of 20 |
| 2.96 | ~11 of 20 |
| 3.2-030926 / 3.2-040921 | **ruled out — wrong mangling** |

`-O1` is far worse across the board (~2 of 20), so `-O2` is settled.

**GCC 3.2 is excluded conclusively and not by scoring.** It emits Itanium ABI
mangling — `_ZN10VIMatrix449TransposeERKS_` — where 2.95.3 emits
`Transpose__10VIMatrix44RC10VIMatrix44`, character for character the target
symbol. It also reproduces the `_GLOBAL_$F$...` companion symbol of the same
shape as the target's `_GLOBAL_$I$` / `_GLOBAL_$D$`.

The single remaining instruction is operand order in an address computation —
target `addu v0,a0,v0` (`this + offset`), candidate `addu v0,v0,a0`. That is a
source-shape question for Phase 2, not a flag.

Two source facts fell out on the way, both worth keeping: **`VIMatrix44::m` is
declared `float m[4][4]`, not `float m[16]`** — the 2D form dropped the diff
from 7 lines to 3 by fixing the order of the two `sll`s on its own — and the
loop is a plain forward `for (j = 0; j < 4; j++)` even though the compiler
emits a countdown counter with forward pointers.

### Reproducing

```bash
cd /tmp && cat > v.cpp <<'EOF'
... the source above ...
EOF
wine decomp/toolchains/ee-gcc2.95.3-114/bin/ee-gcc.exe -c -O2 -o v.o -x c++ v.cpp
decomp/eqd.py diff Transpose__10VIMatrix44RC10VIMatrix44 --obj /tmp/v.o --trim
```

`tools/` holds a venv with **asm-differ** and **m2c** (m2c supports the exact
`mipsee` + `gcc` + C++ triple this project needs).

## Phase 2 result: TWO functions match byte-for-byte

Both verified independently with `cmp`, and both report `MATCH (raw, no
normalisation needed)` — zero words normalised, so the match is earned rather
than manufactured.

| Function | Bytes | Result |
|---|---:|---|
| `Scale__10VIMatrix44f` | 60 | **exact** — including the `sq`/`lq` prologue, the `jal` relocation and a store order the compiler *rescheduled* |
| `Transpose__10VIMatrix44RC10VIMatrix44` | 84 | **exact** |

```cpp
class VIMatrix44 {
public:
    float m[4][4];
    void Identity();
    void Scale(float s);
    void Transpose(const VIMatrix44 &a);
};

void VIMatrix44::Scale(float s)
{
    Identity();
    m[0][0] = s;  m[1][1] = s;  m[2][2] = s;
}

void VIMatrix44::Transpose(const VIMatrix44 &a)
{
    float (*d)[4] = m;                    /* the hoist is load-bearing */
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            d[j][i] = a.m[i][j];
}
```

Built with any of `ee-gcc2.95.2-273a` / `2.95.3-107` / `2.95.3-114` at `-O2`,
each using its own bundled assembler.

**Source facts recovered, and they are the real product of this phase:**
`VIMatrix44::m` is `float m[4][4]`. `Scale` calls `Identity()` and writes the
three diagonal terms **in index order** — the object code emits them
`[2][2], [0][0], [1][1]`, which is the compiler's scheduling, not the source's.
And `Transpose` **hoists `m` into a local `float (*d)[4]`**: without that hoist
one `addu` comes out with its operands the other way round, and no flag, no
optimisation level and no other source phrasing fixes it. Five 2.95
patchlevels, four 2.9 builds, 2.96, 3.2, ~25 source forms and ~20 flag
combinations were swept to establish that.

### Two harness bugs this phase found

**`trim_padding` was eating the final delay slot.** It stripped trailing zero
words, which includes the `nop` in the delay slot of the terminating `jr ra` —
so a byte-exact `Transpose` was reported as a one-instruction failure for
several rounds. It now ends the function one word after the last `jr ra`.
`test_trim_keeps_the_final_delay_slot` locks it.

**`eqsweep.py` paid process startup per build.** A wine-hosted driver costs
about a second to start and milliseconds to compile 20 instructions, so a
120-build sweep took minutes. Compiling every variant in one driver invocation
(gcc accepts many sources with `-c`) made the same sweep finish in seconds.

`./eqsweep.py <variants.py> <symbol>` sweeps sources x toolchains x flags and
ranks by instructions wrong. It reports build failures and missing symbols
separately: a variant whose symbol is absent is **not** evidence about the
source, and a missing symbol usually means the wrong compiler (3.2's Itanium
mangling) rather than a bad variant.

## Superseded: the original Phase 1 plan

Acquire an `ee-gcc 2.9x`. Evidence for the compiler identity is in the symbol
tables: GNU C++ 2.x mangling (`Add__10VIMatrix33RC10VIMatrix33T1` — no ARM `F`,
`T1` back-reference), `_GLOBAL_$I$` / `_GLOBAL_$D$`, `_$_<len><Class>`
destructors, `__tf<n>` typeinfo, `__rtti`, and libgcc's `__divdi3` / `__fixsfdi`.
That is SN ProDG / the Sony EE GNU toolchain, not CodeWarrior.

decomp.wiki's PS2/EE list gives the candidate versions — 2.9.x (Fatal Frame,
Parappa 2, Twisted Metal Black), 2.95.x (Tales of Rebirth), 2.96 (Kingdom
Hearts), 3.2 — all SN Systems forks. Frontiers is 2005, but the heavy
branch-likely usage above argues for the **2.9x/2.95x** end rather than 3.2.
Score each candidate with `eqfingerprint.py --obj ... --compare` before
attempting a byte-exact match with any of them.


## Phase 3 result: 28 of 31, with the toolchain AND the layout settled

```bash
./eqcheck.py                  # build src/ and score every function against the image
./eqcheck.py -f Rotate -w     # one regex, with the instruction diff for misses
./eqcheck.py --baseline       # rewrite src/MATCHED.txt
./eqasm.py build/x.o SYMBOL   # target | candidate, side by side, one row per word
./eqsweep.py sweeps/rotate.py Rotate__10VIMatrix44RC6VIQuat
```

`src/` is the decomp: `vi.h` plus `vimatrix44.cpp`, **28 of the 31 pure-scalar `VIMatrix44`
methods byte-exact** (1,434 of 1,707 instructions) plus the destructor. `src/MATCHED.txt` is
the baseline and `eqcheck.py` exits non-zero if a symbol that was exact stops being exact, so
a source change made for one function cannot quietly break another. `sweeps/` holds the
variant files that found each shape — they are the record of what was tried, not scratch.

### The toolchain

> **`ee-gcc 2.95.3-107 -O2 -G0 -fno-exceptions`, compiled with `-S`, assembled with
> `toolchains/ee-gcc2.95.3-114/lib/gcc-lib/ee/2.95.3/ps2eeas.exe`.**
>
> (`-fno-exceptions` was added in Phase 5 — nothing in Phases 1-4 could see it. See below.)

The compiler and the assembler come from *different packages*, and the assembler's **version**
matters as much as its identity:

* **GNU `as` vs `ps2eeas`.** After `mtc1` writes an FPR, an R5900 COP1 instruction reading it
  needs one slot. **gcc emits `#nop` — a COMMENT — and leaves the insertion to the
  assembler.** Every GNU `as` in `toolchains/` drops it; every `ps2eeas` inserts it. Both
  `ConvertToEuler` functions are one instruction short under GNU as.
* **`ps2eeas` 1.9.25.758 vs earlier.** The later one (in `ee-gcc2.9-991111` and `2.95.3-136`)
  pads a `div` that follows a branch delay slot with two nops — the R5900 DIV errata,
  announced by `-Wdivbug-on`. The target has no padding, so the build predates that fix. The
  `2.95.3-114` copy agrees with all 27 functions; `2.95.2-273a`'s 1.8.19.316 does not pad
  either but breaks `Transpose` and `Debug`.

None of this is reachable by a flag: `-Wa,-O0`, `-Wa,-mips1/2/3`, `-m5900`, `-mmips-as`,
`-mfix4300` and `-fno-delayed-branch` were all swept first.

### The layout

```cpp
class VIVect4
{
public:
    float x, y, z, w;

    VIVect4() { x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; }
    void Set(const VIVect4 &v) { x = v.x; y = v.y; z = v.z; w = v.w; }
};

class VIMatrix44
{
public:
    VIVect4 m[4];          // NOT float m[4][4]
    ...
} __attribute__((aligned(16)));
```

Three independent constraints pin this, and `SwapRows` — which had absorbed ~30 spellings of
its body and stayed at 32 of 34 — matched first try once it was in place, from
`t.Set(m[i]); m[i].Set(m[j]); m[j].Set(t);` and nothing else:

* the copy must go through a function taking a **reference**, or gcc forwards the values in
  registers instead of round-tripping them through the stack;
* it must be `Set` and **not** `operator=`, because an `operator=` on `VIVect4` would make
  `VIMatrix44`'s implicit assignment element-wise, and the copy constructor's is a 64-byte
  block move (which in turn needs `aligned(16)`, or it comes out `ldl/ldr`);
* `VIVect4` must have the **zeroing default constructor** — it is where `SwapRows`' three
  surviving `sw zero`s come from, and the copy constructor's entire zero loop is that ctor run
  over `m[4]` (gcc's array-construction loop, counter down to −1 with `bne`).

All 28 functions matched before the migration still match after it: `m[i][j]` became
`m[i].x/y/z/w`, row pointers became `&m[i].x`, and the two loops that index with variables go
through an explicit `(float (*)[4])m` cast.

### Style facts — assume these before sweeping anything

1. **Row pointers are variables.** `float *d0 = m[0], *d1 = m[1], ...` and then
   `d0[0] = s0[0] + f;`. No spelling of `m[i][j]` reproduces it.
2. **Loops hoist the matrix**: `float (*d)[4] = m;`, then index `d`.
3. **Inner loops are unrolled in the SOURCE.** `-funroll-loops` is off and changes nothing;
   write the loop structure the object shows, literally.
4. **Four routines are Graphics Gems verbatim** (Shoemake): `RotateHPR` is `Eul_ToHMatrix`,
   `ConvertToEulerXYZ`/`ZXY` are `Eul_FromHMatrix`, `ConvertTo(VIQuat&)` is the
   matrix→quaternion gem with the `int nxt[3] = {1,2,0}` stack copy, and `Rotate` is the
   quaternion→matrix gem — whose temp order `wx,wy,wz,xx,xy,xz,yy,yz,zz` is load-bearing.
5. **An address that reaches a MEM directly comes out INDEX-first; one materialised into a
   variable comes out BASE-first.** `return p[i];` gives `addu $5,$5,$4`;
   `const float *q = p + i; return *q;` gives `addu $4,$4,$5`. This is the rule behind the
   whole `ConvertTo`→quat family, and behind Phase 2's "hoist" discovery.
6. A pure scheduling difference is a **declaration-order** problem far more often than a
   store-order one.

### Relocation masking

An unlinked object holds zero where the linked image holds a resolved value, so
`eqd.py diff --obj` and `eqcheck.py` mask **exactly the fields the object's own relocation
table names** (`R_MIPS_26`, `HI16`, `LO16`, `32`, …), count them, and print the count beside
the MATCH. Two things to know:

* `readelf -r` offsets are **relative to the section each relocation applies to**. Ignoring
  that let `.rel.data` entries land inside `.text` functions and mask 14 real instruction
  words. `elf_relocations(path, section)` filters; a canary locks it.
* This is principled where `--norm hi16lo16` is a heuristic: only the linker-written field of
  a named site is touched. `--no-reloc-mask` turns it off.

### Open

| Symbol | Instr | State |
|---|---:|---|
| `ConvertTo__C10VIMatrix44R6VIQuatR7VIVect3T2` | 231 | 71 of 245 words wrong with the **length exactly right** — the scale loop and both branch bodies match. One register-allocation choice in the prologue (`addiu v0,sp,16` + `move s7,v0` vs `addiu s7,sp,16`) renames registers through the rest. Fifteen spellings of the local and its pointers all coalesce the same way. |
| `__10VIMatrix44RC10VIMatrix44` | 20 | 19 of 21 words, and every part is accounted for: the zero loop is exact and the 64-byte block move is exact. The same register choice again — the target keeps `this` in v0 across the loop and walks v1. A self-assignment guard reproduces the allocation exactly (at the cost of its own two instructions), so the mechanism is "something keeps `this` live across the loop"; fifteen spellings that should have done it all coalesce the same way. |
| `__as__10VIMatrix44RC11VIMatrix44D` | 22 | needs the `VIMatrix44D` (double) class |

**Caveat on the three `ConvertTo`→quat sources.** They match, but their spelling — an integer
cast for the row pointer and `const float &` bindings for the elements — is a *lever* on
gcc's address canonicalisation (fact 5 above), not something anyone writes. The likeliest
reading is that the real build's compiler canonicalises the other way, i.e. a 2.95 patchlevel
not in `toolchains/`. Recorded as "reproduced, spelling unexplained".

**The layout question these were left on is closed** — the migration to `VIVect4 m[4]`,
`aligned(16)` happened inside Phase 3 and all 28 matches survived it, so it is not the cause
of the register-allocation divergences above. Phase 4 identifies what is: see
*The four that are open are ONE phenomenon* below, where the same shape shows up in four more
functions and is shown to be neither a flag nor any compiler in `toolchains/`.

## Phase 4 result: a SECOND class, on the same settled toolchain

```bash
./eqcheck.py                       # 49/55 byte-exact
./eqcheck.py -f VIMatrix33 -w      # the four that are open, with diffs
```

`src/vimatrix33.cpp` is the second translation unit: **20 of the 24 `VIMatrix33`-owned
symbols byte-exact**, 577 of ~855 instructions, 17 of them first try. The combined baseline
in `src/MATCHED.txt` is **49 of 55**.

**Nothing about the toolchain changed** — `ee-gcc2.95.3-107 -O2 -G0`, `-S`, `ps2eeas` from
the `2.95.3-114` package. Phase 3's conclusion reproduced an independent class with no
re-tuning, which is what turns it from a fit into a finding.

### The layout, argued the same way at a different size

```cpp
class VIVect2 { public: float x, y; };

class VIMatrix33 { public: VIVect3 m[3]; ... };   // 36 bytes, stride 12, NOT aligned(16)
```

* the default ctor's zero loop is **3 iterations of stride 12, counter down to −1** — the
  implicit construction of three zeroing `VIVect3`;
* **`VIVect3` HAS a user `operator=` where `VIVect4` must not** — `VIMatrix33`'s assignment
  and copy ctor are nine `lwc1`/`swc1` pairs where `VIMatrix44`'s is a 64-byte block move.
  Without it gcc emits an unaligned `ldl`/`ldr` for `m[i] = m[j]`;
* **`VIVect3` also needs a user copy constructor** — `return r;` out of either `operator*`
  copies the local into the caller's slot one float at a time.
  Both additions regressed **none** of the 29 `VIMatrix44` matches.

`operator=` is user-written, not synthesized: gcc's synthesized array assignment is a loop
and the target is straight-line. The copy ctor spells the same three row assignments rather
than calling it.

### A shipped bug, found by matching

`Translate(const VIVect2 &)` calls `Identity()` and then stores to offsets **36 and 40** —
one row past a three-row matrix. The source is `m[3].x = v.x; m[3].y = v.y;`, `VIMatrix44`'s
translation row copied to a class that has only three. It matches byte for byte written that
way. It is not a convention difference: `Transform(float, float, const VIVect2 &)` puts the
translation in **row 2** (24/28), where a 3x3 row-vector affine wants it. So `Translate`
corrupts the eight bytes after the matrix and never translates.

### Style facts, second pass

Phase 3's list held. Two sharpened, two are new:

1. **Row pointers are variables** — again. `Zero`, both `Add`s and both `Mul`s take
   `float *d0 = &m[0].x, ...` and refuse every other spelling; `Zero` written as nine
   `m[i].x = 0.0f;` is 11 words wrong.
2. **The scalar in a matrix product is a FRESH local per term.** `Mul(m,m)` is
   `f = a.m[i].<comp>;` then three `d[j] op= t<k>[j] * f;`. One reused variable gives the
   right instructions with the wrong FPRs (28/125 wrong); nine separate locals is exact.
   Hoisting it is also why `a` is read through plain offsets off `a1` with **no base
   register materialised** — one load per element gives gcc nothing to CSE.
3. **`Mul(m,m)` accumulates into `this` directly**, no temporary — visible as rows 1 and 2
   reloading their partial sums. `go-eqoa-vi/vimath`'s `Matrix33.Mul` uses a temp, so it
   differs from the console when the destination is also an operand.
4. **`Invert` calls `SwapRows` unconditionally** — no `row != i` guard, just the two `jal`s.

### The four that are open are ONE phenomenon

> **Corrected 2026-08-22 — they are three.** The description below is accurate and the
> cause is not: `Transpose()` is the `loop` pass, the `operator*` pair is `sched`, and the
> Phase 5 double `Distance` family is `lreg`. See *Phase 8* at the end and
> `claude-notes/decomp-which-pass-diverges-20260822.md`.

The target's build **neither merges two textually identical memory references nor keeps a
value live where ours does**:

| Symbol | Delta |
|---|---|
| `Transpose()` | target builds **three** induction pointers (separate ones for the load and the store of `d[i][j]`) where the natural source builds two |
| `Invert` | `w.m[i][i]` loaded **twice** through two addresses built from one copied base (`move a0,v1`); the pivot scan loads `w.m[k][i]` twice again |
| `operator*` x2 | the local accumulator is stored to its stack slot after **every** statement; ours sinks all three to the end |

Same shape as Phase 3's two leftovers, so **six functions now show it**. Tested and refuted:

* **Not a flag.** `-fno-rerun-cse-after-loop`, `-fno-cse-follow-jumps`, `-fno-cse-skip-blocks`,
  `-fno-gcse`, `-fno-expensive-optimizations`, `-fno-thread-jumps`, `-fno-strength-reduce`,
  `-fno-force-mem`, `-fno-schedule-insns{,2}`, `-fno-peephole`, `-fno-defer-pop`,
  `-funroll-loops`, `-fno-omit-frame-pointer`, `-O1`, `-O3` and **`-fvolatile` /
  `-fvolatile-global` / `-fvolatile-static`** all leave `Transpose()` at 33 of 34 wrong.
  `-fvolatile` was the strongest candidate and changes nothing.
* **Not a compiler on disk.** All eleven working EE toolchains give the two-pointer form.

Not refuted: it is reachable from the **source**. A row pointer declared in the outer loop
splits `Transpose()`'s givs and makes the **entire inner loop byte-identical** — 34 words
against 34, 19 wrong, every one of them in the outer preamble, where gcc strength-reduces
that pointer's `i*12` across the outer loop and emits a second `li 12` while the target
computes both `i*12` inside the guarded region from one constant. ~20 spellings have not
closed it. `src/vimatrix33.cpp` keeps that form with the delta written out in a comment.

Long form: `claude-notes/decomp-phase4-vimatrix33-20260822.md`.


## Phase 5 result: two more classes, and the flag they exposed

```bash
./eqcheck.py                       # 84/98 byte-exact
./eqcheck.py -f VIQuat -w
```

`src/viquat.cpp` (13 symbols, **12 exact**) and `src/vivect.cpp` (32 symbols — `VIVect2/3/4`
and the software-double `VIVect3D/4D`, **26 exact**). **21 of the 35 matched first try**, on
the toolchain Phase 3 settled and Phase 4 reproduced, with no re-tuning.

### `-fno-exceptions` — a flag that stayed invisible for four phases

`VIQuat::ConvertTo(VIVect3 &)` is the first function in the tree with a **local whose class has
a destructor** (a stack `VIMatrix44`). With exceptions on — gcc 2.95's C++ default — it grows
a cleanup region: three copies of the `_$_10VIMatrix44(this, 2)` call reached by `b`, 44 words
against the target's 33. With the flag the length is exactly right. **Every other function in
the tree is byte-identical either way**, which is the lesson: a flag is invisible until some
function's code depends on it, and that function may be the 90th you write. RTTI stays on
(`__rtti` / `__tf<n>` are in the module).

That trailing call with `a1 = 2` is the GNU 2.x destructor ABI's `__in_chrg` argument — a
`jal` whose second argument is a small constant on a class object is a destructor.

### Source facts

1. **`operator*` returns a TEMPORARY, and that is worth ten words.** `return VIQuat(a,b,c,d);`
   is elided into the caller's slot with **no stack frame at all**; `VIQuat r; ...; return r;`
   builds `r` on the stack and block-copies it out (`ldl/ldr/sdl/sdr`) — gcc 2.95 has no NRVO
   for a named local. So the class has a **four-float constructor**, and separately the
   **zeroing default constructor** earned by `VIMatrix44::ConvertTo`'s local `VIQuat q;`.
2. **Field order decides the schedule.** `RotateX/Y/Z` differ only in where their two
   `sw zero` land relative to the `sinf` call. That is one source shape written in **x, y, z, w
   declaration order**, not three; writing the zeros first reproduces exactly one of the three.
3. **`Normalize` is ONE shape across all six widths** (`VIVect2/3/4`, `VIVect3D/4D`, `VIQuat`):
   `if (d != 0.0f) { ...; return 1; } return 0;`. The `== 0.0f` early-return spelling is
   indistinguishable at float width and one word wrong at double (`beqzl` vs `beqz`) — the
   function that settles it is the one whose comparison is a libgcc call.
4. **`NormalizeMagn` has one exit** and returns the local `d` reassigned from the length; the
   zero case falls through with `d` still 0.0.
5. **`VIQuat::Slerp` returns void** — nothing writes `v0` on any path — and its negative-dot
   half is a full textual duplicate, not "negate `b` and recurse".
6. **`VIMatrix44::Zero()` is defined IN the class body**: SUPPORT carries the out-of-line VU0
   copy at 0x003F01D0 *and* inlines its five words into `VIQuat::ConvertTo`.
7. **`Distance`/`DistanceSquared` build a real struct difference on the stack** — the `swc1`
   into the frame are dead stores gcc keeps because the local is a struct.
8. **The double siblings are entirely libgcc** — `VIVect3D::Magnitude` is six `jal`s, no
   `sqrt.s` and no NaN retest, because gcc's inline square root is single-precision only. The
   same source text at `double` width matches anyway.

### Open: fourteen functions, still one phenomenon

The eight new ones are the same sentence as the six before them — **the target does not keep a
value live in a register where ours does**:

| Symbol | Delta |
|---|---|
| `Distance__C7VIVect3`, `Distance__C7VIVect4`, `DistanceSquared__C7VIVect4` | the multiplies start before all the subtractions finish; one store leaves a delay slot |
| the four `Distance`/`DistanceSquared` on `VIVect3D`/`VIVect4D` | ours holds four doubles in **s0-s3** with a 112-byte frame; the target spills them to the local's own slots and saves **s0 alone**, 64 bytes |
| `ConvertTo__C6VIQuatR7VIVect3` | the inlined VU0 `Zero()` takes `$sp` as its asm operand where the target copies it to `a2` first — 7 words, all of them that copy |

Refuted again this phase: **not a flag** (`-fno-schedule-insns{,2}`, `-fno-delayed-branch`,
`-fno-cse-follow-jumps`, `-fno-gcse`, `-fno-expensive-optimizations`, on top of Phase 4's
sixteen), **not a compiler on disk** (ten of eleven agree with ours; `2.9-990721` no longer
builds the tree), and — new, and stronger than Phase 4's null — **not the source**: six
spellings of `VIVect3::Distance` give **byte-identical** output.

Long form: `claude-notes/decomp-phase5-viquat-and-vivect-20260822.md`.


## Phase 6 result: the double siblings, and the audit they run on the float source

`VIMatrix44D` (41 symbols), `VIQuatD` (13) and the two free
`operator*(const VIVect3D/VIVect4D &, const VIMatrix44D &)` — **50 of 56 exact**, taking the
tree from **84 of 98 to 134 of 156**. 35 of the 56 matched on the first build, on the
toolchain Phase 3 settled and Phases 4 and 5 reproduced, **with no re-tuning of compiler,
assembler or flags**. A third and fourth class reproducing the same build is what turns the
compiler identity from a fit into a fact.

### The double sibling is an ORACLE for the FLOAT spelling

The vector unit is single-precision only, so **nothing in these two classes is VU0** and
every double arithmetic operator is a libgcc call. gcc 2.95 does not CSE across a call. So a
hoisted temporary and its un-hoisted spelling -- which at float width produce the *same
single instruction*, because CSE folds the repeat -- come out with **different call counts**
at double width.

That audit found three wrong source facts in the already-matched float tree. Each was fixed
at both widths, and **every float function still matches**:

| Function(s) | Was believed | Actually |
|---|---|---|
| `VIQuat::RotateX/Y/Z` | `float h = a * 0.5f;` | `sinf(a * 0.5f)` / `cosf(a * 0.5f)` -- halved **twice** |
| `VIMatrix44::UnitRotate`, both `Transform(const VIQuat &, ...)` | `x2 = q.x + q.x; xx = x2 * q.x; ...` | nine products, each `(q.c + q.c) * q.d`, declared **wx, wy, wz, xx, xy, xz, yy, yz, zz** |
| `RotateHPR`, both `Transform(const VIVect3 &, ...)` | `shsj = sh * sj; chsj = ch * sj;` | `sh * sj * si` written inline |

The float target doubles `q.w` once. The double target doubles it **three times** -- once per
product that needs it. Same source, one of them without CSE.

**Match a `...D` class as a pair with its float twin, and settle every hoisted local there.**

### Two more zeroing default constructors, and the local they forbid

`VIVect4D` has one -- `VIMatrix44D::SwapRows`' temp stores zero to +8/+16/+24 with the +0
store dead, and both `VIMatrix44D` constructors open with gcc's array-ctor loop over
`VIVect4D m[4]` (counter 3 down to -1, stride 32). `VIVect3D` has one -- the `VIVect3D inv;`
in `ConvertTo(VIQuatD &, VIVect3D &, VIVect3D &)` zeroes +8/+16 before storing 1.0 into all
three. Adding it took that function from 320/337 wrong to **143/337 with the length exact**.

And therefore **`VIVect3D::Distance`/`DistanceSquared` and `VIVect4D`'s do not declare a
struct local**: gcc 2.95 does *not* delete a constructor's stores even when every member is
assigned before it is read, so a `VIVect4D d;` costs five words the target does not have
(measured, 61 -> 66). They are `double d[3]` / `double d[4]`. Copy-initialising
(`VIVect4D d = *this;`) is worse still -- it runs the copy constructor, seven words.

### A destructor that is not in the symbol map

`VIQuatD::ConvertTo(VIVect3D &)` destroys its stack `VIMatrix44D` with a `jal` carrying
`a1 = 2` -- the GNU 2.x `__in_chrg` argument. There is **no `_$_11VIMatrix44D` entry** in the
map (`_$_10VIMatrix33` and `_$_10VIMatrix44` are both there), so nothing scores it and only
the call site says it exists. Declaring it is what made `ConvertTo` match.

That function is also not the float one transcribed: **the double body has no `m.Zero()`**.
`VIMatrix44`'s constructor is trivial and the matrix must be zeroed by hand; `VIMatrix44D`
has a real one, so the call is redundant and absent.

### What the double class shows that VU0 hides

Ten `VIMatrix44` methods are VU0 macro-mode blocks and have never been attempted. Their
double siblings are ordinary C++:

- **`Mul(const VIMatrix44D &, const VIMatrix44D &)` is `VIMatrix33::Mul`'s no-temp
  accumulation at 4x4** -- `this` is the destination, so rows 1-3 reload their partial sums
  because the stores may alias `a` or `b`.
- The three vector products and both free `operator*` are row-vector, with `m`'s rows as
  pointers (their first element still folding to an offset off `this`) and the vector
  component hoisted into a fresh local per row. Without that local `v.x` is reloaded for all
  four terms -- `out` and `v` may alias.
- `SubMatrixMul` uses **no row pointers at all**: plain offsets off `this`.
- `BlendAccum` multiplies the component by the weight **once per row** into a hoisted local --
  four calls in the function, not sixteen -- and is **not** `SubMatrixBlendAccum`'s shape,
  which left-associates `m * v * w` per element. Neighbouring functions, different spellings.
- **`InvertAffine` expands the determinant as the six triple products** (`r00*r11*r22`,
  `r01*r12*r20`, ...), not as `r0 . (r1 x r2)` over cofactors already formed.
  `go-eqoa-vi/vimath/invert.go` documents the float VU0 path as the latter; the rounding
  orders differ, so that comment is worth re-checking against the console.

### The tail-address spelling is width-dependent

`ConvertTo(VIQuatD &, double &, VIVect3D &)` matched only with **pointer locals declared per
statement**:

```cpp
const double *pjk = mj + k, *pkj = mk + j;
qq[3] = (*pjk - *pkj) * s * inv;
```

The `const float &mjk = mj[k];` reference bindings its FLOAT sibling needs hoist all six
across the calls (173/255 wrong); plain `mj[k]` stops the hoisting but comes out index-first
where the target is base-first (8/250). At float width the element load is one `lwc1` with a
scaled index and **no address is materialised at all**, so the question never arises -- which
is why the same computation needs different text at the two widths.

### Open: twenty-two, and the four smallest examples yet

| Symbol | Wrong / total | Shape |
|---|---|---|
| `BlendAccum__C11VIMatrix44DRC8VIVect3DdR8VIVect4D` | 6/152 | `move s1,v0` <-> `ld a0,0(s2)`, x3 |
| `BlendAccum__C11VIMatrix44DRC8VIVect4DdR8VIVect4D` | 8/156 | same, x4 |
| `__ml__FRC8VIVect4DRC11VIMatrix44D` | 18/143 | `sd v0,N(sp)` <-> `ld a0,N(sX)`, x9 |
| `__ml__FRC8VIVect3DRC11VIMatrix44D` | 20/130 | same |

Every one is ONE adjacent pair swapped repeatedly: **the target keeps the instruction that
consumes a libcall's result glued to the call and loads the next operand after it; this build
schedules the load first.** Seven more flags swept on top of Phase 4/5's twenty-two --
`-fno-schedule-insns`, `-fno-schedule-insns2`, `-fno-delayed-branch`, `-fno-strength-reduce`,
`-fno-caller-saves`, `-fno-peephole`, `-fno-function-cse` -- and **none improves it**:
`-fno-strength-reduce` is neutral, every other one is worse. Address-taking the destination,
moving the declarations and direct member access were all tried (direct member access scores
156/156 wrong, so the row pointers are real).

`Invert__11VIMatrix44D` (191/220) and `InvertAffine__11VIMatrix44D` (297/315) are genuinely
unsolved rather than near-misses. `Invert` differs in how the pivot's index arithmetic is
built -- the target keeps `i*32` and `i*8` as two shifts where this build folds them into a
multiply by 40; a row-pointer local (193) and the integer-cast idiom (219) both score worse.
`InvertAffine` has the different determinant expansion described above.
`ConvertTo(VIQuatD &, VIVect3D &, VIVect3D &)` is length-exact at 337 words with 143 wrong,
and reproduces its float sibling's open delta exactly: the target materialises `&inv.x` into
a temporary and copies it where this build writes the `addiu` straight into the callee-saved
register.

Long form: `claude-notes/decomp-phase6-double-siblings-20260822.md`.


## Phase 7 result: VIPlane, and InvertAffine decoded by its OPERANDS

`VIPlane` and `VIPlaneD` (8 symbols, **5 exact**) plus `VIMatrix44D::InvertAffine` from
297/315 wrong to **byte-exact** — **134 of 156 to 141 of 164**.

### The lever: `re-tools/dblflow.py`

The image is un-relocated, so every `jal` target is zero and **the callee cannot be read off
the instruction**. But a double-precision EE function is nothing but calls -- every operator
is libgcc -- and the OPERANDS are readable. `dblflow.py` symbolically executes the loads,
moves, stores and delay slots and prints one line per call:

```
c20  = f(a.m[1].y, a.m[2].z)
c21  = f(a.m[1].z, a.m[2].y)
c22  = f(c20, c21)
c23  = f(c22, c19)
  this.m[0].x := c23
```

Which libgcc routine each `f` is follows from the shape -- a sum of products is not
ambiguous. 315 instructions came out as a readable algorithm in one pass, and the rewritten
source scored **2 of 315 wrong on the first build**.

### `InvertAffine`

```
det = a00*a11*a22 + a01*a12*a20 + a02*a10*a21 - a02*a11*a20 - a01*a10*a22 - a00*a12*a21
```

- **The six triple products written out and left-associated**, NOT `r0 . (r1 x r2)` over
  cofactors already formed. The float sibling runs on VU0 and does use the cross product;
  `go-eqoa-vi/vimath/invert.go` documents that form, and **it is a different computation with
  different rounding** -- do not carry the Go note over to `VIMatrix44D`.
- **A zero determinant writes NOTHING.** The whole body is inside `if (det != 0.0)`, so a
  singular matrix leaves `this` exactly as the caller had it.
- **Every negation is a subtraction from zero** (`__subdf3(0.0, x)`): the EE has no
  double-precision negate, so `0.0 - (-0.0)` is `+0.0`.
- The translation row uses the already-divided inverse rows and reads them back out of
  `this` -- gcc forwards its own stores.

The last two words were **a chained assignment**: `m[0].w = m[1].w = m[2].w = 0.0;` stores
2, 1, 0 because `=` groups right to left. All six permutations of three separate statements
were swept; only the chain's order is right.

### `VIPlane` / `VIPlaneD`

`VIVect3 n; float d;` -- and **the normal is the FIRST member**, proved by `operator()`:
`n.Normalize()` compiles to a call with `a0 = this`, so `&n == this`. `operator()(a,b,c)` is
`u = b - a`, `v = c - b`, `n = v x u` (that order), `Normalize()`, then
`d = -((n.x*a.x + n.y*a.y) + n.z*a.z)` and return 1; on failure `d = 0` and return 0.

`Project` is ONE source text at both widths, and not the obvious one:

```cpp
VIVect3 r = n;                                        // copy the normal
float t = -(((v.x * n.x + v.y * n.y) + v.z * n.z) + d);
r.x = r.x * t;  r.y = r.y * t;  r.z = r.z * t;
r.x = r.x + v.x;  r.y = r.y + v.y;  r.z = r.z + v.z;
return r;
```

`r.x = n.x * t + v.x` instead is 13/30 wrong at float and 76/81 at double, where the copy is
visible as three `ld`/`sd` into the slot `r` later occupies.

### Two findings that are not about planes

**The success path goes in the `if`.** gcc 2.95 moves an `if`-body that ends in a `return`
out of line, so the block the source puts INSIDE the `if` is the one that lands LAST. The
target's layout is failure-inline with success at the branch target, and the source that
produces it is

```cpp
if (den != 0.0f) { ...success...; return 1; }
t = 0.0f; out.x = 0.0f; out.y = 0.0f; out.z = 0.0f;
return 0;
```

Inverting the guard fixed three functions at once.

**A reference out-parameter read three times needs a local copy.** Both intersections scale
the output by `t`, a `float &`. Read three times it reloads three times -- each store to
`out.x` may alias it -- and that forces store/reload/store where the target does three
multiplies then three stores. `float s = t;` before the group is the whole fix.

### Open: twenty-three

`Invert__11VIMatrix44D` (191/220) is the one with a real question left: the target keeps
`i*32` and `i*8` as **two separate live values** -- a row base in a callee-saved register and
the column offset spilled -- where this build folds `s[i][i]` into one multiply by 40. A
row-pointer local scores 193/227 and the integer-cast idiom 219/225, both worse.

The rest are the standing phenomenon, with a sharper reading of it: **the `addu` operand
order of a computed address is decided by the ALLOCATED REGISTER NUMBERS, not by the
source.** `ConvertTo__C11VIMatrix44DR7VIQuatD` matched with a plain `mj[k]` and then LOST
the match when an unrelated function in the same translation unit was rewritten -- the
allocation shifted and every `addu` flipped from base-first to index-first. The
per-statement pointer-local spelling is stable across that, and all three double
`ConvertTo` tails now use it.

Long form: `claude-notes/decomp-phase7-viplane-and-invertaffine-20260822.md`.


## Phase 8: which PASS — the 23 are three phenomena, not one

```bash
./eqrtl.py dumps src/vimatrix44d.cpp
./eqrtl.py chain build/rtl/vimatrix44d 'BlendAccum' 25,27,48,50
```

`eqcheck.py` says a function is N words wrong and `eqasm.py` says which words; **`eqrtl.py`
says which of gcc's passes decided it**, which is what a fix has to name. The shipped
`cc1plus.exe` accepts **`-da`**, so this needs no compiler source: gcc 2.95 prints every insn
as `(insn[:MODE] N PREV NEXT (...)` and that linked list is the emission order. Follow one
insn's neighbours across the pass dumps; the pass whose row changes is the one that did it.

**The standing "one phenomenon" reading is refuted.** The 23 share a description — the target
does not keep a value live where ours does — and not a cause:

| divergence | pass | example |
|---|---|---|
| the call's result copy held next to the call, vs. the next argument load hoisted above it | `sched2` | `BlendAccum` (6/152) |
| the same shape, decided one pass earlier | `sched` | `__ml__FRC8VIVect4DRC11VIMatrix44D` (18/143) |
| call-crossing pseudos given callee-saved registers — 128-byte frame saving s0-s4 where the target's is 64 saving s0 alone (`.greg` reports `0 regs to allocate`, so global alloc is not involved) | `lreg` | the six double `Distance`/`DistanceSquared` |
| giv/biv elimination — two induction pointers where the target keeps three | `loop` | `Transpose__10VIMatrix33` (19/34) |

**The scheduler ran in the target build and must not be turned off**: `-fno-schedule-insns2`
takes the tree to 15/164 (and `BlendAccum` from 6 wrong to 17), `-fno-schedule-insns` to
39/164. It agreed with ours everywhere except this one shape, so the difference is a
priority/tie-break inside haifa-sched, not a pass that was off.

Two traps in reading the dumps, both of which give confidently wrong answers:

* **the chain runs THROUGH notes** — walking `next` lands on a `NOTE_INSN_*` constantly, and a
  walk that only records real insns stops after one element and reports every pass as "absent",
  which reads exactly like a true negative;
* **the dumps do not agree on the function's name** — `.rtl` prints the full C++ signature,
  `.gcse` and `.loop` print the bare name, so two overloads are indistinguishable in half the
  passes. `eqrtl.py` picks the body that holds the insns asked about and refuses when two
  bodies hold them in different orders; insn numbers restart per function, so membership alone
  is not enough.

### Two more nulls, and the one lever

* **`-fno-strict-aliasing` is not it** — swept over the whole tree for the first time (it had
  only ever been tried on one function, `sweeps/swap_d.py`): **141/164, unchanged** in both
  directions.
* **The SN patchlevel is not the dial.** Whole tree per compiler, assembler held at the 114
  `ps2eeas`: v2.95.2-273a 137/164, v2.95.3-107 141/164, v2.95.3-114 141/164, v2.95.3-136
  **40**/164 — and v2.73a, v1.07 and v1.14 give **identical wrong-counts on all 23** while
  v1.36 rewrites codegen everywhere and closes none of them.
* **`-mcpu=` IS a lever on exactly this class.** `r5000` 64/164, `r4600` 63/164, `r4300`
  55/164, `r4000` 42/164 — all far worse overall, yet each moves several of the 23 *toward*
  the target (`Distance__C7VIVect4` 32/35 -> **22/34**; the four double `Distance`s improve;
  `Invert__11VIMatrix44D` 191 -> 188). `-mcpu` selects the function-unit model and the
  `mips_cpu` costs, i.e. the inputs to the `sched` and `lreg` decisions above.

So all three cost models are in the **machine description** (`define_function_unit` /
`ADJUST_COST` for the scheduler, `MEMORY_MOVE_COST` / `REGISTER_MOVE_COST` /
`REG_ALLOC_ORDER` for local-alloc, `rtx_cost` / `ADDRESS_COST` under `loop.c`'s benefit
threshold) — a bounded, data-like surface in `config/mips/`.

**This does not say the target is a patched gcc.** Every compiler in `toolchains/` announces
`SN BUILD`; SCE's own `ee-gcc` is a different fork and would plausibly carry a different
machine description, so finding that binary beats patching one. And before any patch is
believed, the control: a gcc built from source must first reproduce `ee-gcc2.95.3-107`
**byte-for-byte over the tree, 141/164**. Without that, matching stops being reverse
engineering and becomes curve-fitting. Acceptance for any change: one principled edit, closes
several of the 23, regresses **none** of the 141.

Long form: `claude-notes/decomp-which-pass-diverges-20260822.md`.


## Phase 8b: the compiler hunt — no better binary exists

A found compiler beats a patched one, so before touching gcc: is there a build that already
behaves like the target? **No.** First full-tree score for everything on disk (compiler varied,
assembler held at the 114 `ps2eeas`, `-O2 -G0 -fno-exceptions`):

| toolchain | fork | tree |
|---|---|---:|
| `ee-gcc2.95.3-107` / `-114` | SN | **141**/164 |
| `ee-gcc2.95.2-273a` / `-274` | SN | 137/164 |
| `ee-gcc2.95.3-136` | SN | 40/164 |
| `ee-gcc2.9-991111`, `-991111a`, `3.2-030210-beta2` | SCE | 25/164 |
| `ee-gcc2.9-991111-01`, `-dtls13010` | SCE | 24/164 |
| `ee-gcc2.96` | SCE | 6/164 |
| `2.9-990721`, `3.2-030926`, `3.2-040921` | SCE | do not build this tree |

**The two forks are distinguishable in the binary.** SN announces `2.95.3 SN BUILD v1.07 for
Playstation2`; SCE/Cygnus announces `2.9-ee-990721` / `2.96-ee-001003-1` / `3.2-ee-030926` with
`/usr/local/sce/` paths. **SCE never shipped a 2.95.x** — its line is 2.9-ee -> 2.96-ee ->
3.2-ee — so the "maybe the target is SCE's fork" reading dies twice: there is no 2.95 to be, and
the SCE builds score 6-25. (Confirmed by md5 that the `2.96` `cc1plus` in the official SCE
package is byte-identical to `toolchains/ee-gcc2.96/`'s.)

**The assembler pins the release.** The 114 package's `ps2eeas.exe` PE VERSIONINFO reads
**1.9.6.516** — exactly ProDG 2.2's EE assembler (2.0 shipped 1.8.19.0, 3.0 shipped
1.9.25.758, which is the one that pads a post-branch `div`). ProDG 2.2's compiler is v1.13.
Since **1.07 and 1.14 are codegen-identical on all 164** and 1.36 is not, the build that would
close the 23 is in **v1.15-v1.35 = ProDG 2.3 through 2.9**.

**Those are not publicly preserved.** decompme's 13 EE compilers are all on disk; archive.org
holds only ProDG 2.0 (v2.73a) and 3.01 (v1.36), both already here; the official SCE SDK 3.0.2
ships `3.2-ee-030926`, already here; and every public PS2 decomp project uses
`2.95.3-136` / `2.96` / `3.2`.

**The artifact cannot name its own compiler** — `.comment` (`GCC: (GNU) ...`) is stripped from
`SLUS_207.44`, `NTGUI2.ELF` and every `*_decompressed_SNR1.bin`. Checked, negative.

**And SN's source was mail-order only.** `GnuSrcPS2.txt` in ProDG 3.01 offers it "on request"
against shipping costs; SN was bought by Sony in 2005. So the control this project set for any
compiler patch — rebuild `ee-gcc2.95.3-107` from source and reproduce 141/164 byte-for-byte
before believing anything — **cannot be met**, and step 3 as specified is dead.

**One real find**: `sn_sys_consoles_2/PS2/130src.zip` is a complete gcc source tree for
`2.9-ee-990721`, `config/mips/` included. Wrong fork to match with, but it pairs with a binary
we have and it turns Phase 8's cost models into readable text — see
`toolchains/src/README.md`. `ISSUE_RATE` is **2** for the R5900, the R5900 function units are
load 2/0 and store 2/1, `MEMORY_MOVE_COST` is 4 + secondary. Ten numbers in two files: the
entire surface a patch would touch.

Long form: `claude-notes/decomp-compiler-hunt-20260822.md`.


## Phase 8c: the scheduler rig — the parameter, named and demonstrated

> **Qualified by Phase 8d below.** Not marking the soft-double libcalls const gives the
> same `sched` order *and* fixes `alloc` and `cse`, and the two changes together are worse
> than the const-call change alone (39/45 -> 32/45 on `VIVect3D::Distance`).

```bash
cd toolchains/src && ./build.sh /tmp/rig          # ~5 min, builds cc1 for mips64r5900el-elf
EQ_RIG=/tmp/rig ./exp.sh "latency 1" \
  's|(eq_attr "type" "load") (eq_attr "cpu" "r5900")) 2 0)|(eq_attr "type" "load") (eq_attr "cpu" "r5900")) 1 0)|'
```

The hunt for a better binary is closed (Phase 8b, plus the 2005 SDK inside a game-source leak
read over HTTP range requests, and `sce_ps2_sdk_25`-`_30`, which do not exist). So the only EE
gcc **source** that survives — `2.9-ee-990721` — was built as an experimental rig. Not the
decomp's compiler and it closes nothing; it exists to answer *why*.

**The control**: 12 C cases x 3 flag sets against the shipped `ee-gcc2.9-990721` `cc1` —
**33 of 36 instruction-identical**. The three misses are one mnemonic (`c.olt.s` shipped vs
`c.lt.s` from this source), so the zip is near but not exactly the revision that built the
binary. Scheduling and allocation reproduce exactly, which is what the experiment needs.

**The rig reproduces the divergence**: `corpus/libcall_pair.c` gives
`jal | ld (delay) | ld a0,8 | move v0` — the same load-before-copy the 2.95.3 build gives on
`BlendAccum`.

| change | result |
|---|---|
| `ADJUST_COST` stops zeroing anti-dependence cost | **no change — refuted** |
| `alu` unit multiplicity 2 -> 1 | no change |
| `ISSUE_RATE` 2 -> 1 | schedule changes, not into the target's order |
| **R5900 `load` ready-delay 2 -> 1** | **exactly the target's order** |

A load sits in the `memory` unit at ready-delay 2 while `move` and `call` sit in a dual `alu`
unit at 1; the load therefore has the longer downstream path, wins haifa's ready-list priority
and issues first. At latency 1 it has no advantage and the natural order survives. **Surgical**:
10 of 12 corpus files byte-identical either way, and the call-crossing-locals case is untouched
— consistent with local allocation being a separate cause.

**It does not establish that SN's v1.15-v1.35 used 1** — v1.36 also produces our order, so such
a change would have had to be reverted. And it closes none of the 23: the rig is 2.9-ee, the
decomp is 2.95.3. `eqcheck.py` stays at **141/164**.

**Incidental, and it explains an old oddity**: `config/host_wint.h` spells `HOST_WIDE_INT` as
`__int64` and `machmode.h` then picks `"%I64d"` — **the shipped EE compilers were built on a
Windows/MSVC host**. Under glibc that format means flag `I` plus width 64, so every constant
prints inside 62 spaces of padding; a wrong-host build announces itself that way.

Long form: `claude-notes/decomp-scheduler-rig-20260822.md`.


## Phase 8d: the double `Distance` family — `cse`, and a source finding

```bash
cd toolchains/src && ./build.sh /tmp/rig
EQ_RIG=/tmp/rig ./exp.sh "libcalls not const" "" "" 's|  if (is_const)|  if (0) /* EXPERIMENT */|'
```

**The source.** The six double `Distance`/`DistanceSquared` are **copy then subtract in
place** — `VIVect3D d = *this; d.x -= v.x; ...` — not `d[i] = x - v.x`. The target loads all
three components of `*this` up front, stores them into `d`'s own slots and **reloads** each one
before its call, keeping only `v` live in `s0` (64-byte frame, one callee-saved register). The
array form loads `a[i]` lazily and keeps BOTH pointers live, which no compiler setting can
undo: it is **refuted as source**. The float siblings are unchanged — the copy spelling breaks
`VIVect2::Distance`/`DistanceSquared`, which are byte-exact, so those really are
`VIVect2 d; d.x = x - v.x;`.

**The pass is `cse`, and Phase 8's table said `lreg`.** Memory references inside
`VIVect3D::Distance` across the real toolchain's `-da` dumps: rtl **30**, cse **21**, and flat
at 21 through `lreg`. Nine loads deleted in `cse`, every one a reload of a `d[]` slot forwarded
from the store that filled it. The surviving pseudos then span the calls, so `lreg` allocates
them 16-20 and `.greg` says `;; 0 regs to allocate`. **The 112-byte frame saving `s0`-`s4` is
downstream of `cse`, not a decision `lreg` made.**

**Why `cse` may forward across three libcalls**: `calls.c` sets `CONST_CALL_P` on every
arithmetic optab libcall, and a const call does not invalidate memory. The `.rtl` dump shows it
directly — `(call_insn/u ... "dpsub")` against `(call_insn ... "sqrt")`, which is not const and
is not forwarded across.

`exp.sh` takes a fourth argument now, a `sed` on `calls.c`, and reports a third column: whether
the copy-form proxy reloads `d.y` from its slot (target) or reuses a forwarded register (ours).

| change | sched | alloc | cse-reloads |
|---|---|---|---|
| baseline | `ld,move` | 128B / 5 saved | 0 |
| R5900 `load` ready-delay 2 -> 1 | **`move,ld`** | 128B / 5 saved | 0 |
| **libcalls not const** | **`move,ld`** | **80B / 2 saved** | **2** |
| both | `move,ld` | 80B / 2 saved | 2 |

**The const-call change subsumes Phase 8c.** It gives the target's `sched` order as well, for a
reason that needs no cost model — a load cannot be hoisted above a call that may have written
what it reads — and on the whole function it scores **39 of 45 positions identical, register
for register** (2 of the 6 misses are the prologue's `ra`/`s0` order, a 2.9-ee convention; the
real residue is two swapped argument-load pairs). **Latency 1 on top makes it worse, 32/45**,
by pulling an argument load into the delay slot where the target has the result store. Phase
8c's parameter fits one 4-instruction sample; this one fits a function.

**It does not establish that SN's build marked them non-const** — same standing as Phase 8c,
on a rig that is the wrong fork — and it closes none of the 23. `eqcheck.py` stays at
**141/164**. The four changed functions score *worse* here (46->51, 48->53, 60->66, 62->68
wrong) precisely because our `cse` forwards; that metric is measured with the wrong compiler
and is not a guide for these six.

**And the "one phenomenon" is three.** Same mem-per-pass method on the other open families:

| family | pass | evidence |
|---|---|---|
| double `Distance` x6 | **`cse`** | 30 -> 21 mems, nine loads forwarded |
| `operator*` x2 | **`flow`** | 27 -> 18 mems — ours deletes the accumulator's intermediate stores as dead, the target keeps them |
| `VIMatrix33::Invert` | **`cse`** | 41 -> 39, and **no call sits between the two loads of `w.m[i][i]`**, so call const-ness cannot reach it |
| `Transpose()` | `loop` | mem count flat at 4; induction variables, as Phase 4 found |

**Flags refuted for this family too.** Phase 4's sweep was scored on `Transpose()`. Re-scored
on the `Distance` filter, thirteen flags — `-fno-gcse`, `-fno-cse-follow-jumps`,
`-fno-cse-skip-blocks`, `-fno-rerun-cse-after-loop`, `-fno-caller-saves`, `-fcaller-saves`,
`-fno-regmove`, `-fno-force-mem`, `-fforce-mem`, `-fno-defer-pop`, `-fno-schedule-insns` and
the rest — give the identical 285 wrong words, bar `-fno-expensive-optimizations` (281) and
`-fno-schedule-insns` (326).

Long form: `claude-notes/decomp-cse-memory-forwarding-20260822.md`.


## Phase 9 result: the shape classes, and the first INTEGER code in the corpus

```bash
./eqcheck.py -f 'VIBBox|VISphere|VIBRect|VIRect'
./eqsweep.py /tmp/variants.py StoreOutward__C6VIBBoxP7VIPlane --keep /tmp/objs
./eqasm.py /tmp/objs/nptr.o StoreOutward__C6VIBBoxP7VIPlane
```

`src/vibbox.cpp` (`VIBBox` 16 of 20, `VISphere` 2 of 3) and `src/vibrect.cpp` (`VIBRect` 12
of 13, `VIRect` 9 of 13) take the tree from **141 of 164 to 176 of 212**, on the toolchain
Phase 3 settled and Phases 4-7 reproduced, **with no re-tuning**.

**`VIRect` is `int`.** Everything matched before this phase was float math on VU0-adjacent
classes; this one is `slt` / `movz` / `movn` / the R5900 three-operand `mult` / `sra`,
matching from the same build with no adjustment. A different *instruction set corner* of the
same compiler is a stronger control than another matrix class would have been.

### The lever: a MATERIALISED SUB-OBJECT POINTER

> When the source names a sub-object through a **pointer variable**, gcc keeps that pointer
> in a register and reads the sub-object's **+4 and +8** members off it, folding the **+0**
> (and, for a 16-byte object, the +12) access back into an offset off the outer base.

`VIBBox::Bound`'s target reads `max.x` as `12(a0)` but `max.y` / `max.z` as `4(v0)` / `8(v0)`
off an `addiu v0,a0,12`. No spelling of `max.y` reaches that -- 18 were swept.
`VIVect3 *mx = &max;` reaches it exactly. Same shape in `VISphere::Init(const VIBBox &)`
(`const VIVect3 *mx = &b.max;`) and in `StoreOutward` / `StoreInward`, where it goes in four
measured steps: `p[i].n.x = ...` 42 of 44 wrong, one reusable `VIPlane *q` with `q++` 34,
six `VIPlane *` 17, and **a `VIVect3 *` per plane's NORMAL, with `d` still written `p[i].d`,
byte-exact.**

### Style facts, fourth pass

1. **Assign-then-modify-in-place.** One operation per statement --
   `c = min; c.x = c.x + max.x; c.x = c.x * 0.5f;` -- visible as three separate stores to
   the same slot, because a store through `this` may alias the const reference the next
   statement reads. A single expression collapses them and never matches.
2. **`if (!(cond)) return 0; return 1;`** is a shape distinct from `if (cond) return 1;
   return 0;` and from `return (cond);`: it decides which value is preset into `v0`.
   `VISphere::PointInSolid` went 22-of-27-wrong to exact on that alone.
3. **A multi-term rejection is one `||`,** not N `if`s -- N `if`s inline the first exit
   where the target branches every test to one shared `return 0`.
4. **A fresh local per squared distance** (Phase 4's per-term scalar again):
   `VISphere::Init(a,b,c)` is 3 of 99 wrong with one reused `t`, exact with `t` and `u`.
5. **`VIVect2`** needs the zeroing default ctor, the two-float ctor AND a user `operator=`
   (without it `min = v` is an unaligned `ldl/ldr/sdl/sdr`). **`VIVect2I` needs `operator=`
   and the two-int ctor but NO zeroing default ctor** -- `VIRect::DistanceSquared` compares
   against a literal 0 (`slti v0,v1,1`) where `VIBRect`'s reads its own zeroed slot back.
   **`VIVect3`** gains a three-float ctor, earned by `Size()` / `Center()` building their
   return value straight into the caller's sret slot.
6. **`v = v * m`** in `VIBRect::Transform`, spelled field by field in `VIBBox::Transform`:
   both assign the free `operator*`'s result back into the input vector, and the dead stores
   are visible only in the LAST corner where nothing overwrites it again. `VIVect3` has a
   user `operator=` so the 2D twin can write the assignment; `VIVect4` must not have one
   (Phase 3), so the 3D one spells the four fields out.

### Two more shipped bugs, matched byte for byte

* **`VIBBox::Scale` scales about `min + max`, not the centre.** There is no `0.5f` anywhere
  in either `Scale`, while `Center()` and `VISphere::Init` both materialise `0x3F000000`, so
  a box away from the origin is MOVED by `Scale` rather than resized. All four 2D twins
  carry it.
* **`VIRect::Store`, `VIBRect::Store` and `VIBRect::Transform` build a degenerate
  rectangle**: the `y` of all four corners is `min.y`, and `+12` (`max.y`) is never read.
  `VIBBox::Store`'s eight corners are correct, so it is a 2D copy-paste.

Precedent: `VIMatrix33::Translate` (Phase 4). Matching keeps finding these because a decomp
cannot round a shipped mistake off.

### Open: thirteen new, all in named families

| symbols | delta | pass |
|---|---|---|
| six `Scale`s (`VIBBox`/`VIBRect`/`VIRect` x2) | the target reloads the stack local `c` after every store through `this` and keeps two of its three copy stores; this build forwards all of them | **`cse`** (Phase 8d) |
| `VISphere::Init(const VIBBox &)` | the target keeps `d`'s zeroing ctor and two `d = *mx` stores; this build deletes all of them. Eight spellings give the identical 43 | **`cse`** |
| `VIBBox::Bound`, `VIBRect::Bound` | ONE instruction: the target shares one `lwc1` of `v.x` between both arms -- a non-annulled delay slot `redundant_insn` merged with the fall-through head. This build puts `v.x` in `f2` in one arm and `f1` in the other, so the loads are not the same insn, cannot merge, and gcc annuls (`bnezl`) and duplicates | **`lreg`** |
| both `VIRect::Union`, `VIRect::Intersect` | right instructions, `movz` where the target has `movn`: the conditional move's destination is whichever operand's register. **All 16 ternary spellings byte-identical** | `lreg` / `REG_ALLOC_ORDER` |
| `VIRect::DistanceSquared` | two words: target clamps with `slti v0,v1,1` + `movn zero`, this build materialises `li v0,-1`. **Eight spellings identical** | `combine` |

`VIBBox::Intersects(const VIVect3 &, const VIVect3 &)` (233 instructions, five stack vectors
and a call per box plane) is the only function in the four classes not yet attempted.

### Harness: `eqsweep.py --keep DIR`

A sweep scores **positionally**, so a variant whose layout is right but whose length is off
by one word reads as "everything wrong": `VIBBox::Bound`'s winning `mx` form scored 61 of 62
and `VIBRect::Transform`'s scored 44 of 44. `--keep` writes each variant's object out so
`eqasm.py` can be pointed at it. **The count ranks; the listing decides** -- two of this
phase's matches came from looking at the variant the ranking had put last.

Long form: `claude-notes/decomp-phase9-shapes-20260822.md`.


## Phase 10 result: `VIString` — the first non-mathematical class

```bash
./eqcheck.py -f VIString
./eqsweep.py sweeps/strcmp16.py strcmp16__8VIStringPCUsT1
```

`src/vistring.cpp` is **46 of `VIString`'s 47 symbols byte-exact**, 26 of them on the first
build, taking the tree from **176 of 212 to 222 of 259** — on the toolchain Phase 3 settled
and every phase since has reproduced, **with no re-tuning**.

Phase 9 added integer code; this one adds everything else the compiler has: `new[]` /
`delete[]`, `char` and `unsigned short` pointer walks, C varargs, `alloca`, a static member
array, and member functions that return a class **by value**. None of it needed a flag the
float math did not.

```cpp
class VIString { public: int m_len; int m_size; char *m_str; };   // +0 / +4 / +8
```

### The lever: a GUARDED DO/WHILE

> **A loop whose condition is also read in the body is an explicit guard around a
> `do`/`while`** — `if (*a) { do { … } while (*a); }` — not a plain `while (*a)`.

A `while` leaves the exit test at the TOP, where gcc reuses its load for the body; the
guarded form emits **two** loads and a `bnezl` whose annulled delay slot carries the body's
first instruction. `strcmp16` scored 14 of 14 wrong through six `while` spellings and matched
**first try** in the guarded form; `strcasecmp16` (33 wrong) and `strstr16`'s outer loop went
the same way. Where the condition's value is *not* wanted in the body — `strstr16`'s inner
loop — both spellings are byte-identical, so the idiom is load-bearing exactly where the
reuse would otherwise happen.

### Second lever: a copy loop WALKS THE PARAMETER

> The moving pointer is the **parameter**; the base is the local (`char *r = d;` …
> `return r;`), never `char *d = dst;` walking a local.

Five functions (`strcpy8`, `strcpy16`, `strncpy16`, `str16to8`, `str8to16`) were "right
instructions, wrong registers" until this was applied. `strcat16`, whose walker starts at
`dst + strlen16(dst)`, is the opposite case and keeps the base in the parameter — so the rule
is about which variable is *initialised from* which.

### Source facts

1. **`operator=` returns by VALUE**: `a0` is the caller's sret slot, `a1` is `this`, `a2` the
   argument, and the tail is a copy constructor into `a0` plus the temporary's destructor.
   So do the three `operator+`s — build a local, `Init`, `Concat`, return it: five calls, no
   NRVO (Phase 5's rule).
2. **`delete []`, not `free`.** `Allocate` tests `m_str` twice — once in the source, once as
   `delete []`'s own `if (p) operator delete(p)`, which gcc cannot fold because `memcpy`
   intervenes and `m_str` is reloaded through `this`. `Deallocate`, where nothing intervenes,
   folds the same `delete []` to one test. The PAIR identifies the operator.
3. **`str8to16`'s loop variable is `unsigned char`**: the target loads `lbu`, tests THAT, and
   sign-extends (`sll`/`sra` 24) only to pass it to `char8to16(char)`. A `char` local loads
   `lb` and needs a second `lbu` for the call.
4. **`c - 'a' + 'A'`, not `c - 32`**: gcc folds the pair into the unsigned 16-bit 65504, which
   does not fit a signed `addiu`, so `strupr16` is a hoisted `li a1,0xffe0` plus `addu`.
   `strlwr16`'s `c - 'A' + 'a'` folds to +32 and is an `addiu` either way.
5. **`if (cond) return 1; return 0;`** (Phase 9) — `operator<` returns the `strcmp` sign that
   way; `return strcmp(...) < 0;` gives the `srl 31` sign-bit trick instead.
6. **Two `if`s, not one `||`**, in `Init(const VIString &, int, int)`: the target branches the
   first test to a shared `return -1` and falls the second into it; the `||` form threads the
   second test straight to the epilogue.

### `__builtin_next_arg` is NOT `va_start` on this target

The three `snprintf` wrappers land **40 bytes too high** with bare `__builtin_next_arg(f)` —
`sp+128` where the target has `sp+88`. `SETUP_INCOMING_VARARGS` (gcc `config/mips/abi64.h`)
puts the register save block **below** `virtual_incoming_args` for the EABI, and the EE is an
EABI target with 8 GPR argument registers, so with three named arguments the gap is
`(8 - 3) * 8`. gcc's own `va-mips.h` closes it with `__builtin_args_info`, and so does this:

```c
typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)
```

`__builtin_args_info(2)` is `CUM.arg_words` and folds at compile time, so `va_start` stays a
single `addiu`. No flag reaches this (`-mabi=eabi/32/o64/n32`, `-mgp32`, `-mfp32`, `-mlong64`,
`-mint64`, `-msingle-float`, `-mno-abicalls` swept) and `__builtin_saveregs()` emits a real
`jal` here.

### A shipped inconsistency

**`strcat16(unsigned short *, unsigned int, const char *)` widens its bytes with a plain
sign-extending assignment** — no call in its loop at all — where `str8to16`, the same loop one
line different, runs every byte through `char8to16`. 0x80..0xFF land as negative UCS-2 values
instead of their CP1252 code points, and it matches byte for byte written that way.

### Open: one, and it is not the source

`vsnprintf16(unsigned short *, int, const char *, char *)`, **two words**: the target sets the
alloca frame pointer (`move s8,sp`) in the delay slot of the `strlen` call and materialises
that call's argument before it; this build does the opposite. **Eleven spellings** are
byte-identical, so it is the instruction scheduler — the family carried since Phase 5.

Long form: `claude-notes/decomp-phase10-vistring-20260823.md`.


## Phase 11 result: the intrusive list, and the collision primitives

**222/259 → 263/302.** Four classes, same toolchain, **no re-tuning**:

| Class | Result | Source |
|---|---:|---|
| `VILNode` / `VILList` | **19 / 20** | `src/villist.cpp` |
| `VICollRay` | **7 / 8** | `src/vicollray.cpp` |
| `VICollSphere` | **4 / 4** | `src/vicollray.cpp` |
| `VIPickSegment` | **11 / 11** | `src/vipicksegment.cpp` |

### `VILList` is Amiga Exec's list

`Init` is `NewList()` transliterated — `m_head` starts pointing at `m_tail` and
`m_tailPred` at `m_head`, so both ends of a walk terminate on a null the header
itself provides and no operation needs an empty-list case. `VILNode` is
`{ next, prev, int pri, unsigned id }`; `id` is `VIHashResourceID(name)` and
`pri` is read by `PriorityInsert` alone. `VILNode::List()` walks `prev` to the
end and casts the header back to a `VILList *`.

### `VICollRay` is the first class whose semantics were settled first

`go-eqoa-vi/vicoll/ray.go` is a console-checked bit-exact Go port of exactly
these functions, so the layout and behaviour were not in question — the
`VIMatrix44` situation one layer up, and every mismatch was codegen. The
default constructor is **out of line and EMPTY**: everything it emits is
`VIVect3`'s zeroing ctor over the six vector members, which is why
`0x30`/`0x34`/`0x44`/`0x48` are the only words it leaves alone — the four gaps
are exactly the four scalars.

`VICollSphere` **inherits** from it with no vtable, and both its `Init` and its
`Precalc` **repeat the base's body** rather than calling it (the base versions
are out of line; a call would be a `jal`, and neither has one).

### Style facts — five new ones

1. **A forwarded field write is not a reload.** `n->m_next = m_head;` then
   `n->m_next->m_prev = n;` reuses the value just stored; spelling the second
   line `m_head->m_prev = n` emits a reload. `AddTail` needs the opposite — a
   named local — because there the target loads before the first store.
2. **An accumulator beats an early return** where the target has one exit
   through a callee-saved register (`RemHead`, `RemTail`).
3. **Compound operators are how a sub-object gets a base register.** Phase 9's
   pointer-variable lever, spelled naturally: `m_delta -= m_start;` puts
   `&m_delta` in a register with `+0` folded. Writing the three components by
   hand reads everything off `this` and lets gcc forward the results into the
   next statement's stores — 62 of 62 words wrong. `VIVect3` therefore has
   inline `operator-=`, `operator+=`, `operator*=(float)` and `Zero()`; adding
   them regressed nothing.
4. **The zero-length block is the ELSE.** `Precalc` is
   `if (l != 0.0f) { divide } else { m_invLength = 0.0f; }` — written the other
   way round the two blocks swap and the compare comes out `bc1f`.
5. **A dead store to an address-taken local survives across a CALL and not
   otherwise.** The three `Init(src, VIMatrix44)` overloads reuse ONE `VIVect4`
   local for two transforms: gcc cannot kill the first `Set`'s stores across the
   intervening call, but does kill the second's, which are overwritten in the
   same block. Using a second local makes three stores appear that the target
   has not got.

And Phase 9's `if (!(cond)) return 0;` shape recurs with teeth:
`VIPickSegment::Init` ends `if (Precalc() < 0) return -1; return 0;`. Every
ternary spelling — `? -1 : 0`, `>= 0 ? 0 : -1`, a local first, `-(r < 0)`,
`>> 31` — gives `slt`+`movn` and is six words wrong; the **branching** form is
what gcc if-converts into `slti`+`movz`.

Two shipped facts: `VICollSphere::Init(src, matrix)` scales the radius by
**inline `sqrtf` with the NaN retest**, not `VIVect3::Magnitude` (which is out
of line); and `VIPickSegment::m_hitColor` copies as an **unaligned 16-byte
`ldl`/`ldr` block move**, so `VIColor32F` has no `operator=` and no alignment
attribute — the same story `VIVect4` tells.

### The two open functions are the standing phenomenon

`VILList::Node` is **`greg`, and nameable**: the target caches `p->m_next`
across the loop and keeps `p` in `a0`; every spelling that caches the next
pointer makes gcc allocate `p` to **`v0`**, the return register, which lets
`p = nx` ride a delay slot, inverts the exit branch, and splits the three
`return 0` exits the target cross-jumps into one. `eqrtl.py`'s dumps say it
outright — after `greg`, pseudo 119 is `reg/v:SI 2 v0`.

`VICollRay::Skitter` is the CSE-vs-store-alias half. Its five stages are all
`out = in; d = -dot(axis, in); out += axis*d;`; the target **reloads** the
source after the copy's stores invalidate it and reuses the copy's registers
for the result, where ours does one or the other. Fifteen spellings land
between 78 and 113 of 201 words aligned and none is both. Two things did fall
out and are in the source: a `const VIVect3 &e = m_dir2;` used by **both**
stage 2 and the distance dot is what puts `&m_dir2` in a callee-saved register
and takes the frame from 160 to the target's 176 bytes, and the final ray is
built with the compound operators.

Long form: `claude-notes/decomp-phase11-containers-and-collision-20260823.md`.


## Phase 12 result: the property bag, and the first TEMPLATE

**263/302 → 451/516.** Two classes, same toolchain, **no re-tuning** — and the second is a
template, so one source has to answer for thirteen instantiations at once.

| Class | Result | Source |
|---|---:|---|
| `VIParticleAttributes` | **55 / 55** (49 first try) | `src/viparticleattributes.cpp` |
| `VIPool<T *>` × 13 | **133 / 159** | `src/vipool.cpp` |

### `VIParticleAttributes` — read the layout off the accessors

54 of its 57 symbols are two- to five-instruction accessor/setter pairs, which makes the
class one of the few whose whole shape can be recovered without decompiling anything:

* every const accessor is `jr ra` + one load, naming its own offset;
* every setter is `field = x; m_version++`, with the stamp at `+0x2CC`. **The stamp's load
  is the setter's FIRST instruction and the field's store its second** — the scheduler, not
  the source;
* `operator=` is the **implicit** one, and it is what settles every field's TYPE: it copies
  member by member in declaration order, so `+0x1C` is a `VIVect3` (three `lwc1`/`swc1` off
  a materialised base — `VIVect3::operator=`), the gradient is `VIColor32F[32]` (an
  `ldl`/`ldr` block move per entry, 32 iterations), and **`+0x2C8` is a FLOAT** where its
  integer neighbours at `+0x2C4`/`+0x2CC` are `lw`/`sw`.

Size is `0x2D4` = **724**, exactly the length the constructor hands to `memset` — the body
is `memset(this, 0, sizeof(*this))` followed by eighteen calls to its own setters, on top of
the member sub-object constructors gcc emits before it.

**Two facts the phase turned on.**

1. **The five vector setters are an explicit `if`/`else`, not a `||` value.** Written
   `m_flag = (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f);` the disjunction materialises
   **0** first and stores it for the false case; the target materialises **1** in the very
   first branch delay slot and stores literal `$zero` for the false case. The `if`/`else` is
   byte-exact, the `||` is 12 of 29 wrong. Phase 9's "a multi-term rejection is one `||`" is
   right in a CONDITION and wrong as a VALUE.
2. **An argument set-up instruction belongs to the `jal` ABOVE it as often as the one
   below.** The constructor's float arguments are 0.0, 100.0, 0.0, 0.5, 0.0, 2.0, 0.0,
   **0.2, 0.0, 0.2, 0.0**, … — four calls in the size group. Reading the delay slots the
   other way merged two of them, dropped `SetStartSizeVar`, and was the whole of a 65-of-175
   miss; restoring the one call made it byte-exact.

### `VIPool<T>` — one source, thirteen instantiations

**236 `VIPool` symbols exist in SUPPORT** across 19 instantiations; 13 are pools of
*pointers* and are therefore the same code.

```
+0x00 m_size       live element count     Node { T m_data; int m_next; int m_prev; }
+0x04 m_capacity   node slots allocated   12 bytes when T is a pointer
+0x08 m_nodes      Node[]
+0x0C m_free       head of the free chain, -1 when exhausted
+0x10 m_first      head of the live chain
+0x14 m_last       tail of the live chain    -2 in m_prev == on the free chain
```

`End()` is **`0x7FFFFFFF`**, not -1 — `go-eqoa-vi` found that from the console side and
here it is `lui 0x7fff`/`ori 0xffff`. `DestroyPool()` is
`for (int i = Begin(); i != End(); i = Erase(i));`, so `Erase` returns the successor.
**`Erase` matched byte-for-byte first try at 55 instructions**, reading the successor into a
local *before* the unlink while re-loading every other field off `m_nodes`.

**New levers.**

* **A branch to a shared epilogue carrying that epilogue's own first instruction in its
  delay slot is ONE return path, not two.** `Next()` looks like it has a duplicated
  epilogue; it is the delay-slot filler stealing the branch target's first instruction and
  retargeting past it, which only happens when the `if`-body is out of line — i.e. when the
  **success path is in the `if`** (`if (next != -1) return next; return End();`). The
  inverted spelling falls through and emits no branch at all.
* **A loop variable that is not the parameter is two spare `move`s.** `LinkFreeChain(int i)`
  copies its argument at entry and again at the top of the loop; `int j = i;` before the
  loop took it from 39 wrong to 0.
* **`if`/`else` with ONE return at the bottom is what cross-jumps.** An early `return 0;`
  inside the `if` emits its own `jr ra`; the target reaches the single one through a `b`.
* **Two sites test a node address for null before writing through it** — in `Add`, and per
  iteration in `Allocate`'s grow loop. No compiler behaviour produces that; it is written,
  and it is **not** a reference conversion (`Node &n = *(m_nodes + m_free);` costs 44 of
  66).

**The instantiations are declared member by member, on purpose.** Not one of the thirteen
carries the whole class — six have no `Init`, three no `Size`, one no `Clear`.
`template class VIPool<X *>;` emits all of them, and the ten extra functions would each read
as a function this decomp cannot find rather than as one the compiler never emitted. gcc
2.95 accepts `template int VIPool<VISprite *>::Size() const;`, so `src/vipool.cpp` mirrors
the image exactly and the score has no free rows in it.

### The two open functions, ×13

`Add` is **one word**: `addu v1,v0,v1` where ours is `addu v1,v1,v0`, same registers and
same everything else — Phase 7's standing operand-order phenomenon at its smallest, seven
address spellings swept.

`Allocate` is **nine of 66**: the copy loop's two induction variables are created in the
other order, one `bltzl` is a `bltz`, and the closing
`if (LinkFreeChain(m_size) < 0) return -1; return 0;` **if-converts to `slt`/`movn` where
the target branches**. 30 spellings were swept (`goto`, accumulators, both guard polarities,
both orderings of the `if (m_nodes)` arms, `Size()` for `m_size`). The control is `Init`, in
the SAME class: its *first* check branches and its *last* if-converts to `slti`/`movz` in
the **target** as well, and `Init` is byte-exact written the obvious way — so the idiom is
right and this is the standing phenomenon, not a misread source.

### Harness: `SPAN_CAP`

Four `VIPool` symbols read as catastrophic failures because a size derived from the gap to
the next *named* symbol swallowed whole neighbouring functions —
`Size__Ct6VIPool1ZP15VIParticleMotif` is two instructions and derives 568 bytes, 566 of them
`VIParticleAttributes::operator=`. `eqdecomp.SPAN_CAP` is a **hand-audited table of four**,
not a heuristic, and `test_span_caps_only_ever_shrink` holds it to that: every entry must
shrink the derived span, land exactly one word after a `jr ra`, and leave at least one
further `jr ra` inside the derived span — the evidence that the gap really did swallow
another function. Capping *every* span at its first `jr ra` would silently truncate any
function with two returns.

Long form: `claude-notes/decomp-phase12-particles-and-the-pool-20260823.md`.

## Phase 13 result: the geometry buffer, and two levers that reached backwards

**451/516 -> 468/534.** `VICollBuffer` is **18 of 19** on the toolchain Phase 3 settled,
with no re-tuning; the nineteenth (`CalcBVolumes`, 247 instructions) is two allocator
decisions away. It is the third class whose **semantics were settled before the decomp
started** — `go-eqoa-vi/vicoll/collbuffer.go` is a console-checked port of the READER half —
so, as with `VICollRay` in Phase 11, every mismatch was codegen and never a misreading. What
is new is the WRITER: a cursor pair per section, the packing switch, and the single
allocation the three sections are carved out of.

Two of the three levers fixed code written in **earlier** phases.

### The layout

```
+0x00 m_id          +0x04 m_userData      VIBuffer, see below
+0x08 m_type        +0x0C m_locked        +0x10 m_scale
+0x14 m_bbox                              VIBBox, 24 bytes
+0x2C m_numGroups   +0x30 m_maxGroups
+0x34 m_numPrims    +0x38 m_maxPrims
+0x3C m_numVerts    +0x40 m_maxVerts
+0x44 m_groups      +0x48 m_prims         +0x4C m_verts
      -- byte OFFSETS until Init adds m_data to all three
+0x50 m_vertexGroup +0x51 m_floraType     signed char
+0x54 m_dataSize    +0x58 m_data
```

`VICollGroup` is 32 bytes, `VICollPrim` 28, and the vertex section is 12 / 6 / 8 bytes a
vertex for the three packings. The num/max pair per section is `VIPool`'s shape at a
different size.

### `VIBuffer` — the two words every buffer class starts with

`VICollBuffer`, `VIPrimBuffer` and `VIColorBuffer` all zero +0x00 and +0x04 in their
constructors and never touch either again; `Clear()` does not. Three facts pin it down: the
store to **+0x04 comes FIRST** in all three, which is Phase 7's chained assignment
(`m_id = m_userData = 0;`); `VIColorBuffer`'s entire constructor is those two stores, the
`Clear()` call and `m_data = 0`, so they belong to no one class; and in `VICollBuffer` they
are emitted **among the `m_bbox` member-constructor stores**, i.e. before the body. Written
in the body they land after all twelve; written as an initialiser list they come out +0 then
+4. Only a base constructor gives both the position and the order. A first data member with
the same constructor is codegen-identical and the object code cannot separate the two; the
name is ours.

### `VIBBox` has a zeroing default constructor, and it calls `VIVect3::Zero()`

Both buffer constructors zero `min` and `max` **twice**, and *both* passes address through
`&min` and `&max` with the +0 folded onto the outer object. That is two nested inlines: the
members' own `VIVect3()`, then a `VIBBox()` body. Writing that body as six component stores
on `this` puts all six on ONE base register; `min.Zero(); max.Zero();` gives each vector a
base of its own, which is the target. `VIVect3::Zero()` was already in the header from an
earlier phase and had never been called.

### Lever: the block's LAST store is hoisted to the FRONT

`Clear()` is sixteen independent member stores in one basic block. gcc's post-reload
scheduler moves the block's **final** store to the head of the run, so the first store
emitted after the join names the source's *last* statement. Sweeping `m_dataSize = 0;`
through all eight positions changed nothing — the statement that moved was always the last
one, whichever it was:

| source ends with | first store emitted |
|---|---|
| `m_dataSize = 0;` | `sw zero,84` |
| `m_verts = 0;`    | `sw zero,76` |
| `m_data = 0;`     | `sw zero,88`  <- the target |

So `m_data = 0;` closes `Clear()`. Nothing else about the function reveals that.

### Lever: `if (x) { … } return 0;` defeats the if-conversion

`VICollBuffer::Allocate` written flat compiles its closing `if (m_data == 0) return -1;
return 0;` to `li v0,-1` + `movn v0,zero,v1`; the target branches. **Five spellings of that
test are byte-identical** (member-then-test, local-then-store, success-in-the-`if`,
assign-in-the-condition, `!p`), so the test is not where the difference lives. Nesting the
body under the first test with ONE `return 0` at the bottom is byte-exact:

```c
if (m_dataSize)
{
    m_data = new char[m_dataSize];
    if (m_data == 0)
        return -1;
}
return 0;
```

Phase 12 left exactly this shape as an unexplained miss in `VIPool<T>::Allocate`. The same
nesting there — the whole body under `if (m_capacity)` — takes it **from 9 wrong words to 4,
in all thirteen instantiations**. The remaining 4 are the copy loop's induction-variable
numbering, which six loop spellings do not move: the `loop` pass, i.e. the standing
phenomenon.

### Lever: a shared tail written into EACH case cross-jumps

`Vertex(VIVect3S16 &)` switches on the packing and then bumps two counters. Written once
after the switch, the `li 28` those counters need stays in the merged tail and **every
register in the function moves by one**. Written into each case and left to cross-jump, the
constant is hoisted into both predecessors — two `li a2,28` — which is the target. The
single-case `Vertex(VIVect3 &)` directly above wants the opposite. The rule is about how
many predecessors the tail has, not about the source reading tidily.

### Two small ones

**`VIVect3S16` needs a user `operator=`** — the implicit one is an unaligned `lwl`/`lwr` +
`lh` block move where the target copies three halfwords. Same lever as `VIVect3` in Phase 4:
suspect the type, not the statement. And **`Begin`/`End` list packings 0, 1 and 3 as three
duplicated case bodies**: a fall-through group lets gcc merge 0 and 1 into a range test
(`bltz` + `slti 2`), where the target keeps three labels that cross-jump into one block.

### Open: `CalcBVolumes`, 27 words

The structure is confirmed — two `VIBBox` accumulators on the stack, two independent "first"
flags (one spanning the group, one reset per prim, which is why gcc keeps them in *two*
registers and re-establishes `a1` after every call), the packing switch inside the vertex
loop, and `m_bbox = gb` on the first group with `m_bbox.Union(m_bbox, gb)` after. Facts that
fell out: the `int` first parameter is **UNUSED**; `p` is VICollide's vertex-group offset
table and `p[g]` is reached with the R5900 three-operand `madd` off `mtlo`; the stack slots
go in **declaration** order and the initialising stores in **statement** order, and here
those differ; and the packed cases read their vertex as an array SUBSCRIPT (index-first
`addu`) where the V case materialises the address into a variable (base-first) — Phase 3's
rule with both halves in one function. What is left is `s4` and `s5` swapping roles (the
per-prim flag vs the x12 induction variable) and one cross-jump downstream of that. Fifteen
source shapes moved neither.

### Harness: the wrong-count was lying

`score()` counted differing instructions **positionally**. One extra instruction in the
candidate shifts every later word, so a structurally correct function reads as almost
entirely wrong — and, worse, *every* source variant scores the same, which makes a sweep
useless. Four `CalcBVolumes` shapes all scored `180/258` while the diff-aligned count ran
64 -> 54 -> 27.

It now counts by **diff alignment** whenever the lengths differ, and marks the row `~`.
Which functions MATCH is unchanged; what changes is that several open functions were
overstated by nearly a factor of two:

```
  wrong 171/201   Skitter__9VICollRay…       ->  wrong~ 91/201
  wrong 153/160   Invert__10VIMatrix33…      ->  wrong~108/160
  wrong  66/66    Scale__6VIBBoxf            ->  wrong~ 51/66
  wrong  47/47    Scale__6VIRectRC8VIVect2I  ->  wrong~ 28/47
```

Two `Scale`s had read as "100% wrong", which is exactly the signal that sends you looking
for a layout error that is not there.

### Groundwork: `VIPrimBuffer` is a phase of its own

Same skeleton — `VIBuffer` base, `VIBBox` at +0x14, three num/max cursor pairs, three
section pointers rebased by `m_data` — but **four floats at +0x30..+0x3C**, three written by
a `SetPacking(int,int,int)` that calls `powf` three times and the fourth set by `Unlock`
from a five-way switch (1/128, 1/127, 1.0). Its
**`Init(VIRaster &, VIPrimBufferType, int, int, int)` DISCARDS the raster argument** the way
`CalcBVolumes` discards its `int`. The type enum has **9 values dispatched through a JUMP
TABLE**, in `Begin`, `Vertex`, `End` and `GetNumVertices`. Twelve file-scope statics hold the
current vertex attributes; the ten setters each write one and `Clear()` resets eleven,
reading `0.0f` out of `m_scale` which it has just zeroed rather than materialising it. They
are constructed at load by gcc 2.x's `__static_initialization_and_destruction(1, 0xffff)`,
which is **static and therefore absent from the symbol map** — so
`Allocate__12VIPrimBuffer`'s 360-byte derived span swallows it whole and will need a
`SPAN_CAP` entry of 72 (its last word is one past a `jr ra`, and the swallowed function has
a `jr ra` of its own).

Long form: `claude-notes/decomp-phase13-collbuffer-20260823.md`.


## Phase 14 result: `VIPrimBuffer`, the GS display-list builder

**468/534 → 496/565.** `VIPrimBuffer` is 29 of 31 on the settled toolchain with
no re-tuning, fourteen of them on the first build. It is `VICollBuffer`'s
render-side sibling — same `VIBuffer` base, same `VIBBox`, same three num/max
cursor pairs and three sections rebased by `m_data` — but what it fills is a GS
display list: every vertex is one or more QUADWORDS carrying a VIF tag, so the
vertex section is indexed by `<< 4` and `CalcDataSize` counts quadwords.

### `VIVect4` is `aligned(16)`, and this class is what proves it

The four packing scales are one `VIVect4` member and it sits at **+0x30**, four
bytes past the end of the `VIBBox` above it. Every offset in the class is short
by one word without the hole. Adding `__attribute__((aligned(16)))` to
`VIVect4` fixed **nine** functions at once and regressed nothing in the 468
already matched — which also retires the question of why `VIMatrix44` comes out
16-aligned: its `VIVect4 m[4]` is, and the attribute on the matrix class is belt
and braces.

### The type enum is read off the DISPATCHERS

`VIRaster::PickPrimBuffer`'s if-chain tests **0, 2, 4, 5** in that order and
calls `PickPrimBufferVUNC` / `PackVUNC` / `PackVUNCG` / `PackVUNBW`;
`VIRaster::DrawPrimBuffer(int,int,float,unsigned)` tests **8** alone and calls
`DrawPrimBufferPackVU`. So `VUNC = 0, PackVUNC = 2, PackVUNCG = 4, PackVUNBW =
5, PackVU = 8`, and the `sltiu v0,v1,9` in every table dispatch follows.
`CalcDataSize` agrees independently: `prims*3 + verts*K` with K = 4, 2, 2, 2, 1
— four quadwords a vertex unpacked, two packed, one for position+UV only.

### The tags are VIFcodes, and that explains the non-merging cases

`0x6C008000` and friends are VIFcodes (CMD in 24-31: `0x6C` UNPACK V4-32,
`0x6D` V4-16, `0x6E` V4-8, `0x69` V2-16). The last word `End()` writes is
`X | 0x14000000`, and `0x14` is **MSCAL** — so `X` is a VU1 microprogram entry
offset, and the symbol map has the family: `_8VIRaster$RStripOffset`,
`RStrip2Offset` … `RStrip6Offset`, `RFanOffset`.

**That is why `End`'s four packed cases look byte-identical in the image and
still do not cross-jump.** The image is un-relocated, so the symbol each `lw`
names is zeroed; at compile time they are five DIFFERENT symbols, the backward
walk in `find_cross_jump` stops on the first instruction, and the merge covers
only what follows — the shared block the target has. One shared load merged all
four bodies away; five distinct ones restored the length to 166 exactly. Which
offset goes with which format is unverifiable from the image (the relocation
masks it); the ascending assignment follows the data symbols' order.

### Lever: a chained assignment is what makes a float zero a REGISTER

`Clear()` stores eleven static float fields with `swc1 $f0`, and `$f0` came from
`lwc1` of `m_scale.y` — which the same function zeroed three statements up.
Written as separate statements each store is `(set (mem:SF) (const_double 0))`;
cse would rather use a cheaper equivalent than the constant, but mem-to-mem is
not a valid insn, so it falls back and every store is `sw $0`. Written as a
CHAIN there is one temporary for the whole chain, cse replaces **that
register's** constant source with the equivalent MEM, and the stores become
`swc1` off it. The byte and halfword statics are NOT chained — an integer zero
is free in `$0`. The chains run last-member-first; the other direction scored
70/103 against 0/103.

### Lever: a local declared INSIDE the case

`Vertex(VIVect3 &)`'s stack `VIVect4` has its zeroing constructor hoisted above
the switch test at function scope and below it inside the case block, which is
where the target has it: 43 wrong → byte-exact. `CalcBVolumes` wants the same
for its `VIVect3` — constructed in all five decode arms, with `pb.Bound(fp, v)`
written in beside it and left to cross-jump into one call.

### Lever: which of two identical stores ends the block

`Unlock`'s PackVU arm sets two scales. Written `w` then `z`, gcc emits the pair
in reverse, the block's last instruction is `swc1 $f0,60(a0)` — the same as the
default arm's — and cross-jumping deletes it, losing the store. Written `z` then
`w` the last instruction is `swc1 $f0,56(a0)`, nothing matches, and the function
is byte-exact. The store order decides whether a block is a cross-jump
candidate, and the source order that produces it is the reverse of what the
emitted code reads like.

### Discarded parameters, again

`Begin(int)` never reads its argument and `Init(VIRaster &, …)` never reads its
raster — the same shape as `VICollBuffer::CalcBVolumes`'s unused `int`, which
`VIPrimBuffer::CalcBVolumes` also has.

### Harness: a fifth `SPAN_CAP`, and `_q.py`

gcc 2.x's `__static_initialization_and_destruction(1, 0xffff)` constructs the
ten class statics that have zeroing constructors. It is STATIC and so absent
from the symbol map, and it sits between `Allocate` and
`_GLOBAL_$I$_12VIPrimBuffer$PrimUV0` — `Allocate`'s gap-derived span swallows it
whole, 360 bytes for an 18-instruction function. `SPAN_CAP` gets an entry of 72.

`_q.py` compiles one translation unit and scores named symbols out of it in
4.5 s against ~60 s for a full `eqcheck.py` pass. Several levers above came out
of thirty-two-variant sweeps that are not affordable at the whole-tree price.

### Lever: cross-jumping does not come back for a second pass

`CalcDataSize` has five case bodies of which THREE are byte-identical. With the
closing three statements written once after the switch, our build collapses all
three into one — 87 instructions against the target's 153. Written **into each
case** and left to cross-jump, the length is 153 exactly and the bodies stay
apart: `find_cross_jump` walks backward from a block's jump, merges the shared
closing statements it finds first, and does NOT come back for the prefixes that
are now identical. Written after the switch there is no tail to merge and the
first thing it finds is the whole body. Phase 13's lever turned around — there
it explained a duplicated constant, here whole bodies. `CalcBVolumes` comes out
at the right length for the same reason.

### Lever: a fresh local for the intermediate, in INTEGER code

The 16-byte alignment helper is four lines and the middle two decide the
function:

```c
int m = n & 15;
int t;                       /* <- this */
int r;
if (m) { t = n + 16; r = t - m; } else { r = n; }
return r;
```

Written `r = n + 16; r = r - m;` the allocator coalesces `r` with `n`, the else
arm's `move` and its branch vanish, and the diamond collapses to three in-place
instructions. With `t`, `n` keeps its own register, the `move`/`b` survive, and
`r` takes the MASK's register — the target, byte for byte, at all ten inline
sites. Seven spellings swept; the two that fold `+ 16 - m` into `- (m - 16)` are
the furthest away. Phase 9's fresh-local-per-term rule, in integer code.

### Lever: the ORDER of a case's locals decides where the split lands

`End`'s arms each read three case-specific values, and declared `s`, `tag`,
`prog` in that order the function is byte-exact. Move `prog` above `tag` and it
is 59 wrong: the backward walk stops on the symbol load, so everything before it
stays duplicated in the five prefixes and everything after merges. Read last,
the prefixes keep the quadword start, the VIFcode, the vertex-section load and
the shift — the nine instructions the target has. Ten orders swept; both that
match have `prog` last.

### `PrimVertexGroup` is `unsigned short`, and one pointer base per byte group

`Vertex(VIVect3S16&)`'s PackVUNCG arm reads the vertex group with **`lhu`**. The
setter's mangled name says its PARAMETER is a signed `short`, but the static it
writes into is unsigned — a signed static read into any local type gives `lh`.
The same arm wants it in a local read at the TOP of the case.

The function writes four groups into a 32-byte record and the target
materialises a base register for some of them — Phase 9's rule applied per
group: PackVUNC colour at `p+28`; PackVUNCG position at `p+4` and colour at
`p+28`; PackVUNBW position at `p+4` and weights at `p+24`; PackVU none. Reading
that table off the target's `addiu`s took the function from 218 wrong to 31.

### Open: two, both allocation

`Vertex(VIVect3S16&)` is 20/328 — one register shift confined to the PackVUNCG
arm, where the target keeps the vertex-group address live and ours reuses it for
the position base. `CalcBVolumes` is 66/273 — scheduling inside the PackVUNCG
decode arm plus two unfilled load-delay slots at the head of the material loop
that our schedule fills. Neither is structural: the block boundaries, the three
switch tables, the pointer bases and the call placement all align.

Long form: `claude-notes/decomp-phase14-viprimbuffer-20260823.md`.


## Phase 15 result: `VIMaterial`, and the layer pointer that is not a subscript

**496/565 → 514/588.** `VIMaterial` is 18 of 31 on the settled toolchain with no
re-tuning, six of them on the first build — the second property bag after
`VIParticleAttributes`, and like that one its whole layout reads off its own
accessors. **It derives from `VIBuffer`**: the constructor's first two stores are
+0x04 then +0x00, the base's chained `m_id = m_userData = 0`.

Every layer setter multiplies the index by 144 and adds it to `this`;
`SetNumLayers` refuses an argument that is not `< 4` and the constructor builds
three layers. Material flags are 1 ZTest, 2 ZWrite, 4 UV animated, 0x10 dither,
0x20 tessellate — 19 by default; layer flags are 1 modulate, 6 wrap, 8 point,
0x10 UV set, 0x20 solid fill.

### The lever: `this + i * 144`, NOT `&m_layers[i]`

The target computes `this + i * 144` into one register and reads the field at a
displacement of 48, 52, 56, 60, 64, 100 or 104 — the array base is folded into
the DISPLACEMENT, not into the pointer. A subscript cannot produce that: gcc
folds the array's +48 and the field's +4 together onto `this` and leaves the
access at `0(reg)`. Four spellings were swept in the real function and a
standalone toy repeated all four; every one folds the constant onto `this`.

It is not the read-modify-write either — single-read accessors like
`GetLayerTexture` match a subscript exactly, because with one use the constant
lands in the MEM displacement whichever way it was associated. Only when the
address is used twice does CSE bind it, and then WHERE the constant sits becomes
visible. `Init` says the same from the other side: it walks one register forward
by 144 per layer and uses displacements 48..59 off it.

The decomp models that with a second type — `VIMaterialLayerView`, the same 144
bytes with the material's header in front — and a file-scope inline
`Layer(this, i)`. The real member array stays declared, because the constructor
constructs three of them: that is where the `VIMatrix33` constructor call in its
loop comes from. **Twelve accessors went from wrong to byte-exact in one edit.**

### Two type facts, both measured

**The flag words are `unsigned`**: `GetLayerWrapMode` bounds its switch with
`sltiu`, and every clear-bit mask is `lui 0xffff` + `ori` rather than `li -N`.
One `unsigned int` on each fixed five functions at once.

**`VIColor32` has a default constructor and it is opaque black** — the material
constructor zeroes `m_emissive` twice, once with `a = 255` (the member's own
constructor) and once with all four zero (the body). And **`VIColor32::Set` is
an inline member, not four field assignments**: `&m_emissive` is materialised
into a register with its +0 folded back into the outer base.

### A shipped bug: `SetLayerUVSet` ORs the complement

Clearing the bit uses `~0x10` — and so does SETTING it. Asking for UV set 1
turns on every layer flag except the one meant: modulate, both wrap bits, point
sampling and solid fill all come on together. Written `|= 0x10` the function is
two instructions different, so it is matched as written. It joins
`VIMatrix33::Translate` (Phase 4) and the `Scale`/`Store` pair (Phase 9).

### A harness fact: `long long` is not an 8-byte member here

The constructor zeroes +0x18 with a single `sd`. Modelling that slot as
`long long` moved every field after it by 16 bytes — `m_dirty` landed at +0x30
instead of +0x20 and thirteen already-matching accessors each went one
instruction wrong, all of them the closing `m_dirty` store. `double` gives the
right offsets.

### Open: five, none structural

`Init` 2/60, `InitWireframe` 4/42 and `InitSolidFill` 4/41 are delay-slot fills
and statement interleave around `m_dirty`. `SetLayerUVRate` is 5/26: the target
materialises `&m_uvRate` where ours folds +104 into the two displacements, and
three pointer and reference spellings all fold. The constructor is 6/60 —
`m_time = 0.0` costs a `move v1,zero` the target does not have.

Not started: `SetNumLayers`, `SetLayerTexture`, `CopyLayer`, `Process` and the
four `Construct*Pass` DMA builders (268, 176, 94 and 92 instructions), which
need `VIRaster` and `VIQWord` and are a phase of their own.

Long form: `claude-notes/decomp-phase15-vimaterial-20260823.md`.

## Phase 21 result: the view frustum and the baked-lighting buffer

**668/763 -> 685/781.** `VIFrustum` 8/9 and `VIColorBuffer` 9/9 of the nine written,
same toolchain, no re-tuning, 12 of 18 first try.  Both classes sit directly under
`VIRaster` and `VIScene` and needed nothing the corpus did not already have.

### The layout, off two accessors

```cpp
class VIFrustum { public: VIVect3 m_verts[8]; VIPlane m_planes[6]; };   // 0xC0
```

`Vertex(i)` is `this + i*12` and `Plane(i)` is `this + 96 + i*16`; `Init()` is
`return 0;`.  Both setters are **whole-object assignments** (`m_planes[i] = p;`)
and both return **void** — written field by field they cost 6 and 7 words, and
neither sets `v0` on any path.

### The lever: a VIVect3 copy is written OUT, component by component

`CalcPlanes` scored **209 of 222 wrong** as `v[0] = m_verts[3];` and **0 of 200**
as `v[0].x = m_verts[3].x; v[0].y = ...; v[0].z = ...;`.  `VIVect3::operator=`
takes a `const VIVect3 &`, so the inlined copy has a **pointer variable** —
Phase 9's lever — and gcc materialises `&m_verts[k]`, reads `.y`/`.z` off it at
+4/+8, CSEs the six such addresses across all six blocks and parks them in
`s0`-`s5` for the whole function.  Where the target has no base register, the
source must not name one.

The four locals are an **array** (`VIVect3 v[4]`): four separate locals emit four
inlined zeroing constructors in a row (Phase 20), the target emits the counted
array-constructor loop.  And **the fourth corner of each quad face is copied and
never passed** — six dead stores that survive because the array's address is
taken (Phase 11).

### The lever: the `lui`s come out in SOURCE order, the stores do not

`VIColorBuffer::Lock` and `::Unlock` each write eight file-scope statics as
`lui rX,%hi(sym)` + `sw rV,%lo(sym)(rX)`, and the two halves schedule
independently.  **Four different source orderings gave byte-identical output**,
so the store sequence says nothing about the source; reading the order off the
`lui` sequence instead — with `eqcalls.py` naming each relocation — put both
functions byte-exact at once.  For a block of writes to statics, diff the
`lui`s.

### `CalcDataSize`, 61 wrong to 0

* **A third answer to the if-direction question.**  `if (c) { …; return 0; }
  return -1;` lays the `return 0` epilogue first; `if (!c) return -1;` puts it
  inline at the top; only `if (c) { … } else return -1; return 0;` produces the
  target's `b` over an out-of-line `-1` into the final `move v0,zero`.
* **Both stores of an aligned value are unconditional** —
  `m_dataSize = a; if (a & 15) a = …; m_dataSize = a;`, not
  `m_dataSize = a; if (m_dataSize & 15) m_dataSize += …;`.  The target's branch
  lands ON the second store.
* **Phase 14's fresh intermediate, in integer code again**: `a = a + 16 - (a & 15)`
  reassociates to `addiu v0,v0,-16`; `int t = a + 16; a = t - (a & 15);` does not.
* **The tag count is an `if`/`else` with the whole expression in each arm** —
  one store and a cross-jumped tail, where `if (n & 3) m_numTags++;
  m_numTags += n / 4;` gives two.
* Phase 13's `if (x) { … } return 0;` again for `Allocate`, whose flat
  `if (m_data == 0) return -1;` if-converts to `movn` in five spellings.

### Open

`VIFrustum::Cull(const VIBBox &, const VIMatrix44 &)` at 19/66: the target does
not hoist the four plane loads out of the inner vertex loop and **enters that
loop by jumping into the MIDDLE of its body**, the first iteration's head
duplicated in the preheader with its vertex addresses folded to `sp`.  A
`return 1` from inside the loop reproduces the un-hoisted body instruction for
instruction and loses the post-loop `if (j == 8)`; the two cannot be had
together.  Refuted: the natural `for`/`break` (31/72), `v[j]` subscripts
(49/71), `m_planes[i]` subscripts (35/66), a source-level peel (37/73),
`while (j < 8 && …)` (33/66) and thirteen `-fno-` flags.
`VIColorBuffer::Color` (292 instructions of GS display-list building against all
eight statics) is not attempted.

`sweeps/vifrustum_cull_bbox.py` is the record, and it carries a harness caveat:
every variant in it is the target's own length and the positional sweep STILL
ranks them wrongly — `and-while` 42 over `return-1-inside` 46, where the
diff-aligned count reads 33 against 19. The divergence starts at instruction 20
and shifts everything after it, so the positional count measures how EARLY a
variant diverges, not how much of it is wrong. `--keep DIR` plus `eqasm.py`.

## Phase 19 result: the sprite hierarchy, and the vtable's own SIZE

**584/666 → 613/697.** `VISprite` 15/16 and `VISimpleSprite` 13/13 on the
settled toolchain with **no re-tuning**, 27 of the 28 byte-exact first try.

The base of the scene's renderable leaf hierarchy, and the class that settles
the polymorphic ABI at a depth Phase 17 could not reach: `VISound`'s three
classes all had the same vtable length, and this one has a derived class that
overrides seven of ten slots.

### The lever: a gcc 2.x vtable ends with a NULL entry

Measured, not inferred. `class B { int a,b; virtual ~B(); virtual int f1();
virtual int f2(); }` emits `.size _vt$1B,40` — an 8-byte header
(`{0, 0, __tf1B}`), three entries and a closing `.space 8`. So

> **a vtable of B bytes holds `B/8 - 2` virtuals.**

`_vt$8VISprite` runs 0x004EBAE0..0x004EBB40 (the RTTI name string `"8VISprite"`
starts exactly there), i.e. 96 bytes, i.e. **ten** — and ten is exactly what
the class has bodies for: `~VISprite`, `GetInstType`, `Raster`, both `Collide`
overloads, `Pick`, `QueryEmitters`, `QueryIntersection`, `Copy`, `Release`.
The four accessors emitted after `__tf8VISprite` are not virtual.

**The console agrees independently.** `go-eqoa-vi/vicoll` reads the live
dispatch table at `sprite+0x3C` and calls the swept sphere through **entry 4**
and the ray through **entry 5** — which is where this declaration order puts
them. Two oracles that have never seen each other's evidence.

It also **corrects Phase 17**: `_vt$7VISound`, `_vt$6VIWave` and `_vt$4VIXm`
are 64 bytes, i.e. SIX virtuals, so `VISound::ChannelCount` is not one. Every
dispatch site in `VISoundDevice` confirms it — they reach slots 1..6 and no
further. Note the pfn is at **`8N+4`**; the `lh` at `8N` is the this-delta,
so `lw v1,52(vptr)` is virtual six.

### The layout

```
+0x00  VIResource   m_id / m_link          (the base's inline zeroing ctor)
+0x08  VIBBox       m_bbox                 GetBBox returns a0+8
+0x20  VISphere     m_bsphere              GetBSphere returns a0+32
+0x30  float        m_offDist = 500.0f     GetOffDist
+0x34  int          m_type    = 0          GetType; go-eqoa-vi's VISpriteType
+0x38  int          m_actor   = -1         and Copy does NOT carry it over
+0x3C  vptr                                 -- the END of the class's own data
```

`VISimpleSprite` adds `m_primBuffer` +0x40, `m_objIdx` +0x44 (go-eqoa-vi's
`ObjIdx`, the index into the collider's buffer pool) and `m_materialPal` +0x48,
and **adds no virtuals** — its vtable is 96 bytes too.

**`VISphere` needs a zeroing default constructor.** The sprite's constructor
zeroes the sphere twice, the way Phase 13 found `VIBBox` does: the member
`VIVect3`'s own constructor, then the body inlined.

### Three more levers

**A derived class's `__tf` inlines the base's when they share a TU.** With both
classes in one file `__tf14VISimpleSprite` came out 34 instructions against the
target's 20 — it builds the whole chain instead of calling `__tf8VISprite`.
Splitting the file matched it byte for byte. Cheapest available test for a TU
boundary in a class hierarchy.

**`SetMaterialPal`'s `VIDictionary &` is DISCARDED** — Phase 16's lever in its
least visible form. Both calls it makes get the dictionary's register *by
accident*, because `a2` still holds the incoming argument. The tell is the
SECOND call: a three-argument source has to move it back into `a2` and the
target does not. 17 wrong words to 0.

**A local whose address is taken by a `const &` parameter is stored exactly
once.** Written as one variable assigned twice, gcc spills the intermediate to
that same slot as well; split into a non-address-taken `t` and an
address-taken `k`, `t` lives in `$f20` across the second `sqrtf` and `k` is
stored once, straight into the slot `Insert` is handed.

### Two more `SPAN_CAP` entries

The sprite TUs carry their own template instantiations —
`VIMultiMap<float, VIEmitterEntry>`'s red-black tree at 0x0047E198..0x0047ED10,
and the palette walkers — and the map names none of the ones in the gaps. So
`Release__8VISprite`, a **two-instruction stub**, acquires a 3512-byte derived
span, and `SetMaterialPalDither__14VISimpleSprite`, 33 instructions, acquires
2648.

### The one open function

`QueryEmitters`, 32/112. Everything about it is settled — three locals and
their offsets (`p` sp+0, `e` sp+16, `mat` sp+32, the key sp+96), the double
zeroing, the `q.m_type == m_type` guard, both inline `sqrtf`s with the NaN
retest, the reciprocal (`t * (1.0f / d)`, not `t / d`), the `Invert`, the
`Insert`, the `Size()`/`Erase(Last())` trim. What is left is the standing
phenomenon with its pass NAMED: **`gcse` merges the four `addiu a0,sp,32` into
one pseudo**, which then needs a callee-saved register and a 208-byte frame
where the target rematerialises and gets by with 192. `-fno-gcse` takes it to
18/110 and costs the tree sixteen functions (613 → 597), so it is not the build
flag. Declaring the matrix FIRST puts it at sp+0, where the address is a bare
`move a0,sp` with nothing to merge, and the allocation then matches the target
instruction for instruction at its own length of 109 words — with every local
at the wrong offset.

Long form: `claude-notes/decomp-phase19-the-sprite-hierarchy-20260823.md`.

## Phase 18 result: VIWalkControl, the first class whose SEMANTICS were already bit-exact

**563/638 -> 584/666.** `VIWalkControl` 21 of 27 on the settled toolchain with **no
re-tuning**, eleven first try. Long form:
`claude-notes/decomp-phase18-viwalkcontrol-20260823.md`.

This is the tier-2 simulation class — the thing that turns a stick sample into a
position — and it was picked next for one reason: `go-eqoa-vi/viwalk` already
ports 24 of its 33 methods **bit-exact against the console**, so the behaviour
was settled before a line of C++ was written and every mismatch is CODEGEN.
That is the Phase 11/13 position and it held: the functions that came out right
came out right immediately, and `Run` — 170 instructions with four
accelerations chosen across two nested tests — matched on the first build.

**The 78-instruction constructor is a complete check on the field map.** It is
nothing but inlined `VIVect3::Zero()`, so the eighteen `VIVect3` members are
exactly the eighteen offsets it zeroes and every gap between them is a member of
some other type. Two facts the console port did not have fall out: **+0x148 is
the `.y` of a VIVect3 at +0x144** — the alternate pitch lives inside a second
angles triple, not in a float member — and **+0xA4 is twelve bytes that are NOT
a VIVect3**.

New levers, all of them cheap to reuse:

* **The clamp is `VIMIN(hi, VIMAX(lo, v))` as MACROS.** The guest tests
  `v < lo` TWICE around a single `hi < v`, which is the outer macro
  re-evaluating its argument; with literal bounds the middle `hi < lo` folds
  away entirely. A helper function, or either order of a min/max pair, gives
  two tests and not three.
* **`m_flags` is a `long` and the `(int)` cast is PER SITE** — `ld` where the
  masked result feeds `movz`/`movn`, `lw` where it feeds a branch or `sltu`,
  and `Run` does both off the same field four instructions apart. It is
  `((int)m_flags & mask)`; `(int)(m_flags & mask)` and a narrowing local both
  score 3/6.
* **`return (x & m) != 0;` gives `sltu`; `if (x & m) return 1; return 0;` gives
  `li`+`movz`.** Phase 13's if-conversion rule with its other half attached.
* **One `AddVec(accum, dir, k)` helper, both vectors BY REFERENCE**, serves
  every input method — settled independently by `TurnLeft` and `SwimUp`. The
  references are what make the store to `accum` alias `dir` on the stack, so
  each `dir` component is written back and reloaded. **The asymmetric multiply
  operand order `viwalk` records is gcc canonicalising, not the source.**
* **A `Cross(out, a, b)` helper** takes `UpdateDirections` from 26 wrong to 9:
  a reference parameter stops gcc constant-propagating the stack-held `up`
  vector, so the guest computes `x * 1.0f` against a register instead of
  folding it away. The hand-written form folds every one and cannot be talked
  out of it.
* **`SetPitchRange` is ONE reused `float *`, written through and READ THROUGH
  THE MEMBER.** Reading `*p` instead lets gcc CSE the macro's two loads and
  collapses the shape (37/97); two pointers or a reference score 26.
* **A selected ARGUMENT versus a duplicated CALL.** `UpdatePhysics` picks its
  damping into a local and calls `powf` once, with each arm's load in a delay
  slot; `Walk` is the reverse and needs **two `AddVec` calls in two arms**,
  because a pointer picked with `dir = cond ? &a : &b` if-converts to `movn`.
* **`InitParms`'s register numbers are a readout of SOURCE ORDER.** 93 constant
  stores, five words wrong, fixed by one swap — `m_parm38` is written before
  `m_parm34`, the single break in an otherwise field-ordered function — which
  corrected both the store order and the FPR assignment of a constant twelve
  statements away. Phase 7's rule read backwards: a mis-ordered pair shows up
  twice, so the registers can be used to FIND the order.
* **In `UpdatePhysics` the clamp runs on the MEMBER** — store, read back for the
  macro's third comparison, store again. The two gravities are **static data
  members**, defined at the TOP of the TU or `_GLOBAL_$I$` gets the wrong name.

Two corrections back to `go-eqoa-vi/viwalk`: **`swimState`'s last comparison is
decided, not compiled** — `bc1fl` is a LIKELY branch, so `SpeedThreshold < |vR|`
true falls through with state **12** where `locomotion.go` has no `|vR|` test at
all and answers 13 — and `InitIntermediate` derives the SWIM acceleration before
the PITCH one, which is the difference between 20/73 and byte-exact.

Open: six functions, all structurally right (correct length, correct control
flow) and all the standing phenomenon — `UpdateDirections` 9/84, `ViewRelativeWalk`
40/130, `PitchDown` 49/112, `ViewRelativeMove` 64/139, `UpdateLocomotion` 70/175,
`UpdatePhysics` 164/375. The nine methods taking a `VIScene &` are not started,
which is exactly the nine `viwalk` has not ported: what makes them unconformable
against the console (they need a world) is what leaves their semantics unread.

## Phase 17 result: the sound resources, and the FIRST VTABLE

**525/600 → 563/638.** `VISound` 6/6, `VIWave` 20/20 and `VIXm` 12/12 — the
whole family byte-exact on the settled toolchain with **no re-tuning**, 29 of
the 38 on the first build of their translation unit.

Everything matched before this either had no vtable at all or, like
`VICollSphere` in Phase 11, inherited without one. This is the first class in
the corpus with `_vt$`, `__tf` and `__ti` symbols, so the phase settles the
polymorphic ABI — the gate on `VISprite`, `VIRaster`, `VIScene` and essentially
every class the engine has left.

### The vptr goes at the END of the class's own data

Not at offset 0. Measured with the settled compiler: `class N { virtual ~N();
int a,b,c,m; }` puts `a,b,c,m` at 0,4,8,12 and the **vptr at 16**; a derived
class adds none of its own. So `VISound`'s vptr at `+0x0C` says the class
carries exactly twelve bytes of data ahead of it — an eight-byte
non-polymorphic `VIResource` base plus its own `m_type` — and `VIWave`'s
members start at `+0x10`. Reading the vptr as offset 0 would have shifted every
field in both derived classes by four.

### The vtable, and how the slot order was read

An entry is **eight bytes** — `{short delta; short index; void (*pfn)();}` —
and there are no thunks, so a virtual call is `lw v0,12(this)` /
`lh a0,8N(v0)` / `lw v1,8N+4(v0)` / `jalr v1` / `addu a0,this,a0`. Entry 0 is
the header and its `pfn` slot holds `__tf<class>`, so **the Nth virtual
declared sits at byte 8N**. All three vtables are **64 bytes** in the image (the
RTTI name strings begin exactly 64 bytes in) and our `.rodata` comes out `0x40`
— which is also what caps the interface: **`VIXm::SetRepeat` is NOT virtual**,
or the table would be longer. The slots were read off call sites — scan for
`lw rB,12(rA)` followed by `lh rt,imm(rB)`, and `imm/8` is the slot:
1 `~VISound`, 2 `Volume` (feeds `mul.s`), 3 `SetVolume` (`$f12 = 1.0f` in
`VIWave::Create`), 4 `Pan` (stored `swc1`), 5 `SetPan`, 6 `IsCached` (a `beqz`
gate in `VISoundDevice::Play`). None of those accessors has a `VISound` body
anywhere in the image and none can be inline, so they are **pure virtual** and
`VISound` is abstract.

> **Phase 19 corrects the arithmetic here.** A gcc 2.x vtable ends with a NULL
> entry, so 64 bytes is header + SIX virtuals + terminator, not seven.
> `ChannelCount` was declared virtual for two phases and is not; the slot is
> last, so nothing shifted and all 38 functions matched either way.

### The RTTI name strings ARE the class hierarchy

`__tf<C>` reads a guard word, calls the BASE's `__tf` with no arguments, then
the builder with `(this, name, &__ti<base>)` — three arguments and a 16-byte
`__ti` for single inheritance, two and 8 bytes for a base-less class. The names
are consecutive in `.rodata`, so the whole chain reads out of the image with no
decompilation: `6VIWave`, `7VISound`, `10VIResource` at 0x004EBF18. That is
where the base class came from. **Every TU that needs a class's RTTI emits its
own `__tf`** — three copies each of `__tf7VISound` and `__tf10VIResource`, one
per sound TU, and the map names one address for each, so three new `SPAN_CAP`
entries. Our build emits fewer copies (the base's `__tf` stays undefined) and
it costs nothing: every copy is byte-identical, a linkage difference and not
codegen.

### The lever: the vptr store lands after the block's FIRST store

There is no `__7VISound` symbol and no gap that could hold one, so `VISound`
has **no user-declared constructor** and each derived class stamps its own
`m_type`. Where it stamps it is decided by one rule, cheaper than the "hoist"
of Phases 13 and 16: the vtable address is materialised `lui`+`addiu`, the
store that uses it needs a slot of separation, and the R5900 scheduler fills
the gap with the block's first otherwise-ready store — **everything else keeps
source order**. `VIWave`'s constructor emits `[4] [12] [0] [52] [8]`, so the
source is `m_buffer = 0; m_type = WAVE;` with `m_type` LAST; `VIXm`'s emits
`[8] [12] [4] [0] [36] [44] [20] [24] [52]`, the same rule with `li v1,1`
covering its own gap first. `VIWave::Allocate`'s tail is the same mechanism
around a `li v0,1`: written `68, 80, 76, 84` it emits `68, 84, 80, 76`. Note it
is the **first** store of the repeated value that gets pulled up, not the last —
the opposite of Phase 16's phrasing of the same phenomenon, and the gap-filling
account is the one that fits every block here. `VIResource`'s constructor is a
**chained assignment** (`m_id = m_link = 0;`, +4 before +0), which is Phase 13's
`VIBuffer` finding holding for a second, unrelated base.

### A shipped bug, and a contrast worth keeping

**`VIWave::Deallocate` returns −1 on every path**, and `Destroy()` is
`if (Deallocate() < 0) return -1; return 0;` — so `Destroy()` reports failure on
every call, on a wave that was freed correctly. Matched as written.

The phase also puts both halves of Phase 13's if-conversion lever side by side:
`VIWave::IsCached` is `return m_cached != 0;` and comes out as one `sltu`,
while `VIXm::IsCached` tests two fields and the target BRANCHES — which only
the nested `if (a) { if (b) return 1; } return 0;` produces; the flat
`if (!a) return 0; if (b) return 1; return 0;` if-converts and is seven words
wrong. `LockBuffer`, `UnlockBuffer` and `VIXm::ChannelCount` all needed Phase
7's "the success path goes in the `if`", and `Allocate` needed Phase 14's fresh
local: `m_size = blocks * (m_channelCount * 16)` is reassociated to
`(blocks * 16) * channels` and puts the `sll` on the wrong operand.

Harness: three `SPAN_CAP` entries (held by `test_span_caps_only_ever_shrink`),
and `eqcheck.py` now scores a symbol **once** when two objects define it,
reporting a `DUPLICATE` row if they disagree — one target function scored twice
would inflate the total.

Long form: `claude-notes/decomp-phase17-the-first-vtable-20260823.md`.

## Phase 22 result: `VICollide`, and a constructor that reached back eight functions

**685/781 → 711/803.**  Twenty-two `VICollide` functions written, **eighteen
byte-exact** on the settled toolchain with no re-tuning — and one finding inside
it corrected **eight already-open functions in four other classes**, none of them
collision code.

`VICollide` is the fourth class whose SEMANTICS were settled from the console
first (`go-eqoa-vi/vicoll`), so every mismatch is codegen.

### The layout

```
+0x000 int                    m_count      not initialised by the ctor; 12 bytes
                                           of padding follow, to 16-align the
                                           first VIMatrix44
+0x010 VIMatrix44             m_world      object -> world
+0x050 VIMatrix44             m_invWorld   world -> object, DERIVED by SetWorld
+0x090 VIVect3                m_preTranslations[32]
+0x210 VIPool<VICollBuffer *> m_buffers    its m_nodes is the 0x218 every
                                           table-driven method reads
```

**The refcount a buffer is shared and released on is `VIBuffer::m_userData`**,
not a field of its own, and `sizeof(VICollBuffer) = 92` — `CreateCollBuffer`'s
own `li a0,92` — closes the Phase 13 layout to the byte.

### The lever: `VIMatrix44`'s default constructor ZEROES

For twenty-one phases `VIMatrix44() {}` was empty and every site wrote
`VIMatrix44 m; m.Zero();`.  Those are the same words **as long as every
VIMatrix44 is a LOCAL**.  `VICollide` is the first class to hold one as a
**MEMBER**, and there they separate: the target's constructor is loop, `Zero`,
loop, `Zero`, loop, pool-ctor — the array constructor and the asm **interleaved
per member**, which no constructor *body* can produce, because member
construction runs to completion first.

**Phase 20 listed "moving `Zero()` into the constructor" as refuted; it was the
wrong spelling.**  `VIMatrix44() { Zero(); }` emits `jal Zero__10VIMatrix44` —
gcc 2.95 will not re-inline an inline member called from another inline member.
The five-word asm written **directly in the constructor body** does inline, and
it is what the target has.  Newly byte-exact off the back of it: `VIGroupSprite`
`Raster` / `Pick` / `QueryIntersection` / `QueryEmitters` / both `Collide`s
(6 → 12 of 13), `VIPointSprite::Pick`, and `VIQuat::ConvertTo(VIVect3&)`
(13/13) — which is also the answer to Phase 20's open note about the fresh
register for `Zero()`'s asm operand.

### The other levers

**A loop whose test consumes a CALL calls it in the INIT and the INCREMENT.**
`Clear` walks the pool by re-asking for `Begin()`.  As
`while ((i = m_buffers.Begin()) != m_buffers.End())` it is 13 words wrong; as
`for (int i = Begin(); i != End(); i = Begin())` it is byte-exact.

**Phase 9's pointer variable, four more times** — `m_buffers.m_nodes[i].m_data`
as a subscript gives `addu off,off,base` where the target is base-first; naming
the NODE fixes `CollBuffer`, `ShareCollBuffer`, `ReleaseCollBuffer` and
`DebugResources`.  `ReleaseCollBuffer` needs the POOL named as well; naming it in
`DebugResources` costs 25 words, because it takes `this` out of `s2`.  **The
lever is per site.**

**`VIVect4::Set` is how a returned temporary is copied.**  `wn = nrm * m_world;`
block-moves the sret slot with `lq`/`sq`; the target copies component by
component, i.e. `wn.Set(nrm * m_world);` — the same `Set` Phase 3 earned for
`SwapRows`.

**Two accumulators can be in the reverse of the emitted order** —
`DebugResources` adds prims first in the object code and bytes first in the
source — and the byte total needs Phase 14's fresh intermediate
(`t = bytes + 92; bytes = t + size;`).

**The three dispatchers are a `switch`** on `VICollBuffer::m_type`, Phase 16's
value-order/source-order rule again.

**Three more guessed callee names corrected by `eqcalls.py`**:
`VICollide::SetTransform` is **`SetWorld`**, `CopyObj` is **`ShareCollBuffer`**,
`ReleaseObj` is **`ReleaseCollBuffer`**.

### Open

`Cull(VICollRay&, VIBBox&)` 30/68 — and it is **not the source**: `&&`, a nested
`if`, an accumulator and pointer variables for either vector pair all give the
same 30 words, and the function has **no stack frame**, so there are no locals
to spell differently.  `Cull(ray, VISphere)` 39/74 and `Cull(sphere, VISphere)`
38/76 — the target holds `0.0f` in `$f7` from the top and the constant 1 in a
**callee-saved** `$s1`, because it is live across the `sqrtf` call.
`DebugResources` 4/61, **one word**: the target reads the node table as `8(a0)`
off a register still holding `this + 528` from the entry `Begin()` call.

Long form: `claude-notes/decomp-phase22-vicollide-20260823.md`.

## Phase 23 result: the FILE LAYER — `VIFile` 29/36 and `VIWadFile` 9/9

**711/803 → 750/848.**  Forty-five functions written, **thirty-nine byte-exact**
on the settled toolchain with no re-tuning; nineteen of the first nineteen
matched on the first build.

`VIFile` is the engine's one file abstraction and it is **four back ends behind
one interface, with no virtuals at all** — a CD/host file with a 32 KB
read-ahead buffer and an asynchronous `sceCdRead` path, a flat memory image, a
scatter/gather segment list with a binary search over it, and nothing.  Every
entry point re-spells the same ladder by hand (`m_fd >= 0`, then `m_memory`,
then `m_segms`, then -1).  `VIWadFile` derives from it — single inheritance at
offset 0, no vtable — and adds a flat archive: three header words, `count`
16-byte directory records, data at a fixed 256 bytes in, and a **linear** walk
over a `hash * 131 + c` case-folded hash to find an entry.

### `long` is EIGHT bytes here; `long long` is SIXTEEN

Phase 16 recorded this about one `sd`-zeroed slot.  In this class it decides
every 64-bit member, and getting it wrong does not merely mis-size the class —
`m_position = m_position + offset` fails to COMPILE (`unsupported wide integer
operation`), because `long long` is TImode on this ABI.  One word took the
constructor, `Tell`, `Seek`, `MemoryOpen` and `MemoryRead` from "does not build"
to byte-exact.

### Nested guards with ONE `return 0` at the bottom

`BufferFlush` opens with two rejections.  As two `if (X) return 0;` statements,
or as `if (X || Y) return 0;`, gcc emits the second test as a `bnel` to the
*continue* label plus a `b` back — two words too many.  Nested, with the
function falling out through a single `return 0`, both guards become a plain
`beqz`/`beq` straight to the shared exit and the function is byte-exact.  This
is the complement of Phase 13's "`if (x) { … } return 0;` defeats the
if-conversion": one level deeper, it decides branch DIRECTION as well.

### `VIMIN`/`VIMAX` with the expression written out beats a named local

A clamp spelled `long avail = …; got = (n < avail ? n : avail);` comes out as
`movz` with the FIRST value as the register default.  The macro — which
re-evaluates its second operand and lets gcc CSE it — comes out as `movn` with
the SECOND as the default, which is what the target has.  `BufferedReadCopy`
went 7 wrong to 0 on that one line and the `MemorySegm` loops 14 to 6.  Which
argument is first also decides the `slt` operand order, so `VIMAX(end, x)` and
`VIMAX(x, end)` are different code and both appear in the file.

### A walking pointer's local is declared at FUNCTION scope

Phase 10's copy-loop lever in its sharpest form: `MemorySegmRead` and
`MemorySegmWrite` were 6 wrong **entirely in the prologue**, in the order the
callee-saved registers are pushed, until `char *d = (char *) dst;` moved above
the guard it is used inside.  Both went to 0.

### The DISC path is the `if`; the IOP path is the tail

All three CD functions exist twice over — once against `sceCd*` and once against
the engine's own `VICdStream*` when `VIFile::sIopStream` is set.  Written
`if (sIopStream) { iop } disc…` gcc puts the IOP half inline; the target
branches `bnez` to it, so the disc half is the `if`.  Worth 28 words in
`CDSync` and 30 in `CDRead`.

### A retry loop's CONDITION carries the calls; its BODY is the wait

`CDSearchFile`'s wait-for-the-tray block sits FIRST in the target, entered by a
`b` over it to the call.  That is what gcc emits when both calls are in the
condition — `while ((r = sceCdSearchFile(f, name)) == 0 && sceCdGetError())` —
and only the printf/delay is the body.  25 wrong to 9, and the short-circuit
`&&` produces both of the target's exits for free.

### A new `SPAN_CAP` entry

`CDRead`'s derived span is 336 bytes; the function ends at 292.  The ten
instructions after it are a **static interrupt handler** (`iSignalSema`, then
`sync` + `ei`) the map does not name, so the last quarter of every diff was
another function.

### Shipped behaviour

* **`VIFile::Close` tests the FILE DESCRIPTOR for bit 6, not `m_flags`** — the
  fields are at +0x00 and +0x04 — so the asynchronous drain it guards never runs
  on a real handle.  Matched as written.
* **`VIWadFile::Load` returns 0, not -1, when the caller's buffer is too small**;
  -1 is "no such entry" or an I/O failure.  `VIWadFile::Free` returns **void**.
* **`MemorySeek` rejects a seek to exactly EOF** (`pos < length` is required).
* **`BufferedWrite`'s post-flush salvage is degenerate**: it keeps
  `VIMIN(0, m_bufLength)` bytes — zero for any non-negative length — and the loop
  it guards copies every byte to `m_bufferAligned[0]`.
* The read-ahead buffer is `new char[0x8010]` aligned up to 16, and flag bit
  `0x40` turns it **off**.  Reads are issued in at most 16 sectors, clamped
  `sectors < 17 ? sectors : 16`.  `Seek_End` measures against `m_cdFile.size`.

### Open (6)

`BufferedRead` 47/233 — the three read strategies, the 16-sector loop and every
store order are right; what is left is which of `return -1` / `return got` / the
eof block lands where, which moves every branch offset in the function.
`BufferedWrite` 23/126 — the target CROSS-JOINS its two `memcpy` call sites (its
fast path is a `b` to a shared `jal memcpy` whose delay slot is a `nop`, the
signature of a merge) and ours does not.  `Open(const char *, int)` 35/93 — the
access-mode guard is three SEPARATE `andi 1/2/4` tests where every spelling
tried folds to `andi 7`, and the join block then loads `m_flags` and ANDs it
with 8 for a value **nothing reads**.  `CDSearchFile` 9/63, `CDSync` 8/102 and
`CDRead` 6/73 are delay-slot and callee-saved-register choices.

Long form: `claude-notes/decomp-phase23-vifile-20260823.md`.


## Phase 16 result: `VISurface`, and the store order a constant decides

**514/588 → 525/600.** `VISurface` is **11 of 12** on the settled toolchain with
**no re-tuning**, six on the first build. It is `VIMaterial`'s other half — what
a layer's `m_texture` names — and the first class that builds **GS display-list
headers**: 64 bytes of VIF and GIF tags in front of every mip level and of the
CLUT, so an upload is a pointer and a quadword count.

### The layout

```
VISurface : public VIBuffer                VIMipLevel               (24 bytes)
+0x08  int   m_width                       +0x00  int m_width
+0x0C  int   m_height                      +0x04  int m_height
+0x10  VIPixelStore m_pixelStore   = 2     +0x08  int m_pixels   byte offset
+0x14  int   m_tbp = -1  +0x18 m_cbp = -1  +0x0C  int m_stride
+0x1C  int   m_lockLevel = -1              +0x10  int m_header   offset, then
+0x20  int   m_dirty     = 1                             a POINTER
+0x28  int   m_paletteCount   0 / 16 / 256 +0x14  int m_qwc
+0x2C  int   m_paletteOffset
+0x30  int   m_paletteSize   +0x34 m_paletteStore
+0x38  int   m_paletteHeader +0x3C m_paletteQWC
+0x40  int   m_numMipLevels
+0x44  VIMipLevel m_levels[10]
+0x134 int   m_dataSize      +0x138 unsigned char *m_data
```

The stride is `LockMipLevel`'s `li a1,24`; the count is the constructor's own
loop bound; `0x44 + 10 × 24 = 0x134` closes it exactly. `m_pixels` is always
`m_header + 64` and is never rebased — `LockMipLevel` adds `m_data` every time —
while `m_header` and `m_paletteHeader` are rebased **in place** by
`SetDMAHeaders`. **The `VIPixelStore` enum reads off `CalcMipLevelStride`'s jump
table**: 0 I4 (`w/2` rounded up), 1 I8, 2 RGBA32, 3 RGB24, 5 RGBA16 — bound
`sltiu 6`, and **4 has no case at all**, so its table entry is the default's
`return -1`. `LockPalette` parks `m_numMipLevels` in `m_lockLevel`, making the
CLUT "level N".

The header is a VIF DIRECT (three NOPs then the code), a GIFtag `NLOOP=1
NREG=1` with `REGS = A+D`, that datum written to `TRXDIR` (0x53) = host→local,
and a GIFtag `NLOOP=qwc-4 EOP FLG=IMAGE`. Which pins the other half of Phase
15's ABI finding: `long long` is 16 bytes here but **`long` is 8**, so those
`sd` constants are `unsigned long`.

### The lever: the LAST store of each CONSTANT is hoisted to the front

Phase 13 recorded "the block's LAST store is hoisted to the FRONT". That block
had one constant in it. The rule is per **value**: gcc emits the last store of
each distinct constant at the head of the block, in source order, and the rest
after it in source order. `Deallocate` writes ten fields with four values and
comes out `16, 28, 32, 308, 8, 12, 20, 24, 64, 312`; taking that for source
order is nine words wrong. Through the rule the head is `{16, 28, 32, 308}` —
one store per value — so `m_lockLevel = -1` must follow both other `-1`s and
`m_dataSize = 0` every other zero, and the source is **field order with the
last two swapped**. Byte-exact, and the constructor falls out of the same rule.

### `Init`'s `VIRaster &` is discarded, and the first call is `Deallocate()`

The Phase 14 precedent repeats, and here it is measurable. Any first call that
keeps `r` live — `Attach(r)`, or anything taking the raster — allocates the
four parameters the other way round (`s1` gets the bit depth, `s2` the width;
the target has them swapped) and costs six words. Written
`if (Deallocate() < 0) return -1;`, with `r` never read, `Init` is byte-exact.
An unused reference parameter costs nothing: it arrives in `a1` and dies there.

### The lever: a walking pointer needs its origin opaque

`SetDMAHeaders` bumps one pointer by 16 three times. Rebase the member and read
it back and every `p += 4` folds into the displacement (16/24/32/40/48/56) and
all three `addiu`s vanish. **Compute the pointer first and store it back** —
`p = (unsigned int *)(m_data + m_header); m_header = (int)p;` — and they
survive. A parameter behaves the same way, which is the tell: gcc folds a
`p + 16` whose `p` it can see as `base + const`, and does not fold one it
cannot. The second half is scope: declaring `p` **once at function scope**,
shared by the palette block and the mip loop, is what puts `this` in `t2` and
`p` in `t1` with the target's `move t1,v0`. 57 wrong words → 12; a four-way
sweep of the two high-half/low-half store pairs closed the rest.

### The lever: a switch orders comparisons by VALUE, bodies by SOURCE

`ReorderPalette` tests `m_paletteStore == 2` first and then falls into the
**16-bit** loop, the 32-bit loop being out of line — so the halfword case is
written first in the source. An `if`/`else if` chain cannot make that shape; a
`switch` can, because gcc builds the decision tree in value order and emits the
bodies in source order. Its three `return 0`s are one `break` to the bottom of
the function: `jump_optimize` turns an **unconditional** branch to a return
block back into a return, so each loop ends `jr ra; move v0,zero` inline, while
the **conditional** ones stay branches into the block — which is what leaves
`move v0,zero` at a label for reorg to steal into the first `bne`'s delay slot.

### The one open function

`CalcSurfaceOffsets`, 87/143. Its shape is settled: the decision tree, both
case bodies, the shared `m_paletteQWC` tail, the four-term loop condition
`w >= 16 && h >= 16 && level < 10 && level < m_numMipLevels` (the cap is a term,
not a `break`, and the exit tests come out in that order), the two mip cursors
and the closing pair. What is left is the standing phenomenon in three forms:
the target computes `align(m_paletteCount * bpp)` **twice**, reloading the count
after the first align's join, where ours carries it across in a spare register;
the target keeps `size + 16` on `size` where ours folds it onto the raw product
as `+80`; and the first mip cursor is based at +76 where ours is at +84. Ten
flags swept (`-fno-gcse`, `-fno-cse-follow-jumps`, `-fno-rerun-cse-after-loop`,
`-fno-expensive-optimizations`, `-fno-thread-jumps`, `-fno-force-mem`,
`-fno-caller-saves`, `-fno-regmove`, `-fno-peephole`, `-fno-cse-skip-blocks`) —
none moves it.

Long form: `claude-notes/decomp-phase16-visurface-20260823.md`.


## Phase 25 result: the ANIMATION CLIP, the THIRD TEMPLATE, and the VALUE POOLS

**883/985 → 1004/1098.**  `VIHSpriteAnim` 19/19, `VIVector<T>` 27/28 across four
instantiations, `VIHSprite`'s twenty-six readers 25/26, and the three VALUE
`VIPool<T>` instantiations `VIHSprite` owns — on the settled toolchain with no
re-tuning.  One word found in the pools reached BACKWARDS and closed phase 12's
`Add` in all thirteen POINTER instantiations at the same time.

The fourth class group whose semantics were settled from the console first:
`go-eqoa-vi/vianim` reads both animation frame formats and the whole playback
pool out of a live game, so every miss here was codegen — and twice the Go port
and the decomp checked each other.

### `VIHSpriteAnim` — one clip, carved out of one block

The third carve-one-block container after `VICollBuffer` (13) and `VISurface`
(16): `CalcDataSize` writes byte OFFSETS into the three section pointers,
`Allocate` takes one `new char[]`, `Init` rebases all three onto `m_data`.

```
  +0x08 m_format    +0x0C m_numNodes   +0x10 m_numFrames  +0x14 m_numTriggers
  +0x18 m_playRate  +0x1C m_playSpeed  +0x20 m_playbackType
  +0x24 m_nodes     +0x28 m_frames     +0x2C m_triggers
  +0x30 m_dataSize  +0x34 m_data
```

Two shipped bugs, matched as written: **`GetPlaySpeed` returns the play RATE**
(it reads +0x18, where `SetPlaySpeed` writes +0x1C), and **`Clear` never zeroes
`m_numTriggers`** — ten fields are reset and +0x14 is not one of them.

**`m_frames` is read BEFORE the guard and the frame index accumulates INTO the
parameter**, seven words each:

```c++
    char *fb = m_frames;                       /* ahead of the format test */
    if (m_format != VIHSpriteAnimFormatFrame)
        return -1;
    ...
    frame += n->m_firstFrame;                  /* into the PARAMETER */
```

The `lw` sits in the FIRST basic block, ahead of the branch, where no scheduler
could have put it — gcc 2.95 does not speculate loads across a conditional.

**A local declared INSIDE each case is worth 22 of 35 words** in
`CalcDataSize`.  With the sizing statements after the switch the two arms
cannot hold their own `sll` and the whole tail changes shape; with them inside
but the running offset SHARED at function scope the instruction stream is
identical and every register is different.  Phase 14's lever, with the register
allocator as the thing it decides.

**The two frame formats are not the same shape.**  `VIHSpriteFrame` is
`{VIQuat, float, VIVect3}` and keeps the scale and the translation apart;
`VIHSpritePackFrame` is `{VIVect4S16, VIVect4S16}` and does not — its second
four shorts are (scale, x, y, z), which is what `vianim` reads off the console.
Both share the 1/512 quantisation and `GetNodeFrame` walks those four as ONE
run on each side, off a pointer, which is phase 9's lever: the first of the
four folds back onto the outer base and the other three come off the pointer.

### `VIVector<T>` — the third template, and the only one that grows

```c++
template <class T> class VIVector { int m_size; int m_capacity; T *m_data; };
```

`Allocate()` is the growth step, written for a vector that already holds
elements: a SECOND raw block, copy-construct the live ones across, destroy and
free the first, install the second.  **Both `Add`s return the INDEX they added
at** — phase 24's `VIClipStack::Push*` idiom, spelled the same way.

**`m_data = 0` is OUTSIDE the `if` in `Clear()`, and two branch-likely delay
slots say so**: each holds a copy of that store stolen from the head of the
join block, which reorg can only do when the join is on both paths.

**`Allocate` needs ONE `return 0` at the bottom** (phase 12/13's lever a third
time): a `return 0` written into the first arm if-converts its null test to
`movn` and the two exits stop cross-jumping.

**`m_trans.Zero()`, not three component stores — 59 of 129 words** in
`VIVector<VIHSpriteNode>::Add`, where `VIHSpriteNode`'s constructor is inlined.
The member call materialises `&m_trans` and folds its +0 back onto the frame's
base; written out, all three land on the ROTATION's base register.
`VIHSpriteNode` itself is 256 bytes and every field of it is read off that one
inlined constructor — three `VIHSpriteFrame`s, two `VIMatrix44`s (each
double-constructed, phase 24's `VIRadialFloraInst` pattern), a `VIVect3`, and
four ints written LAST.

### `VIHSprite`'s readers — and the OTHER address spelling

**Every indexed reader names the ELEMENT through a pointer**
(`VIHSpriteNode *n = &m_nodes.m_data[i];`): `m_data[i].field` puts the address
sum in the INDEX's register, the target puts it in the base's.  Three words of
five, in every one of them — and `FindTrigger` shows the lever one level up,
naming the CONTAINER through a pointer so `m_size` folds back onto `this` while
`m_data` comes off the pointer.

That is the interesting half of this phase: `VIHSpriteAnim` and `VIVector` need
phase 24's INTEGER form for exactly the same shape of access, and `VIHSprite`
needs the pointer form.  Both spellings sit in one phase and the object code
separates them every time.

**Phase 10's guarded do/while in `Last`**: written as a `while` the sibling is
read twice per turn, gcc keeps both loads, and the loop comes out as three
copies of its body — 24 of 37 words.

Two more shipped bugs: **`Parent(i)` is inverted** (it answers only for node 0,
whose parent is nothing, and returns `End()` for every other node), and
**`Prev(i)` walks node i's OWN child chain** where the previous sibling lives
under i's parent.

### The value pools, and the word that reached back to phase 12

`VIHSprite` owns three VALUE `VIPool<T>`s — `VIHSpriteAttachment` (8 bytes),
`VIHSpritePlay` (96) and `VIHSpritePlayNode` (44) — needing three members the
pointer ones never ask for: `Prev(int) const`, `Add()`, and
`Init(const VIPool<T> &)`.

**`Add`'s node address is an integer add with the offset on the left.**
`m_nodes + m_free` canonicalises base-first however it is written; the cast to
`unsigned int` does not.  Phase 24's `VIArray` lever again — and it closed the
one `addu` phase 12 recorded as open in ALL THIRTEEN pointer instantiations.

**The allocation is a RAW char block here too** — invisible for every pointer
instantiation, where the Node has no constructor, and worth 53 of 125 in
`VIPool<VIHSpritePlayNode>`, whose element carries a `VIHSpriteFrame`.

**`VIHSpritePlay`'s three 64-bit clock fields are load-bearing at COMPILE
time**: they give the class alignment 8, and without them gcc guards the
104-byte node copy with a runtime `(src|dst)&7` test the target does not have
(48 of 100 words).  The field map is `vianim`'s, read off a live console, and
this is the decomp confirming it by a route the console could not.

**`VIHSpriteFrame` has a user `operator=`** — the implicit one block-moves
`m_rot` with `ldl`/`ldr`, where the target copies the rotation and the scale as
five floats off one pointer and the translation as three off another.
`VIHSpriteAnim::SetNodeFrame` still block-moves the rotation, so it is written
out by hand there rather than as `*d = f`.

**`Init(const VIPool<T> &)` uses ONE index for both walks**, declared at
function scope, so the free-chain loop keeps it in the callee-saved register
the live walk used.  Five words in the smallest instantiation, and the
difference between three matches and none.

### Harness

Fourteen new `SPAN_CAP`s: `VIHSpriteAnim::Allocate` (72 bytes derived 2,584 —
its TU closes with an unnamed `VIMap<int,int>` instantiation), six for
`VIVector` and seven for the value pools.

### Open

`Allocate` in `VIVector<VISceneDisplayElem>` (4/60),
`VIPool<VIHSpritePlay>` (16/75), `VIPool<VIHSpritePlayNode>` (18/89) and the
thirteen pointer `VIPool::Allocate`s (4/66) — all one phenomenon, and NOT the
source: the loop's two induction variables are initialised and incremented in
the opposite order, and six spellings of the body leave it unchanged.
`VIHSprite::GetNextAttach` (6/25) is prologue scheduling.
`__10VIMatrix44RC10VIMatrix44` — gcc's own implicit copy constructor, visible
now that something uses it — is 19/21, right instructions and wrong registers.

Long form: `claude-notes/decomp-phase25-vihspriteanim-vivector-and-the-pools-20260823.md`.

## Phase 24 result: the SECOND TEMPLATE — `VIArray<T>` 131/132 and `VIClipStack` 7/10

**750/848 → 883/985.**  `VIArray<T>` across **twenty-two instantiations**, plus
the clip stack that is its biggest consumer, on the settled toolchain with no
re-tuning.

`VIArray` is the plainest container in the engine: two words, a heap block, and
no growth path.  `Init(n)` sizes and allocates, `Clear()` destroys and frees,
and there is no `Add`, no count separate from the capacity, and no way to resize
without throwing the block away.

```c++
template <class T> class VIArray
{
public:
    int m_size;     // +0x00
    T  *m_data;     // +0x04
    ...
};
```

`claude-notes/radial-flora-system.md` had that eight-byte shape years ago, read
off the *callers* — three VIArrays at +0/+8/+0x10 of `VIZoneRadialFlora` with
`m_data` at +4 of each.  Same layout, said independently.

### The allocation is a RAW char block, and the elements are PLACED into it

The tell is what is missing.  `new T[m_size]` makes gcc emit its own vec-init
constructor loop — a countdown over a walking pointer, no reload of either
member, no null test — and **the target has no such loop for any of the
twenty-two**.  What it has is a *count-up* loop that reloads `m_data` and
`m_size` every iteration and null-tests the element address, which is a
source-level loop over placement new:

```c++
    m_data = (T *)new char[m_size * sizeof(T)];
    if (m_data == 0)
        return -1;
    for (int i = 0; i < m_size; i++)
    {
        T *p = (T *)(i * sizeof(T) + (unsigned int)m_data);
        if (p)
            new (p) T();
    }
```

`Clear()` and the destructor are the same decision the other way: `delete []
(char *)m_data`.  With the typed spelling, an element type that has a destructor
drags gcc's array-cookie destroy loop into `Clear`, and
`VIArray<VIZoneRoom>::Clear` grows from 24 instructions to 40.  The engine
destroys its elements itself, in `DestroyPool`, and frees the block as bytes.

### The five nops in an empty loop are the ASSEMBLER

`VIArray<int>::Allocate` ends with a loop whose body is five `nop`s.  Nothing in
the source or in gcc puts them there: **`ps2eeas` pads a short loop that ends in
a branch-likely** — the R5900 short-loop erratum — and it does it to our object
too, byte for byte, from a genuinely empty body.  Ten of the twenty-two
instantiations have that shape and all ten match.

It also settles why an empty loop survives at all.  gcc 2.95 deletes
`for (i = 0; i < n; i++) ;` outright — measured — and keeps this one because the
address computation and its null test are uses of `i`.  Once the body is empty
the index is dead and the loop **reverses** to a countdown, which is why the
`int` form counts down while the `VIVect3` form counts up.

### The lever: `i * sizeof(T) + (unsigned int)m_data`, OFFSET FIRST

Every spelling that keeps the pointer a pointer — `&m_data[i]`, `m_data + i`,
`(T *)((char *)m_data + i * sizeof(T))`, and the same with the terms swapped —
comes out `addu p, base, offset`, and the target is `addu p, offset, base`.
Pointer arithmetic canonicalises with the pointer first however it is written;
an **integer** add does not.  Casting the pointer to `unsigned int` and putting
the offset on the left fixed `VIVect3`, `VIPlane`, `VIHSpriteTrigger`,
`VIZoneNode` and `VIWorldNode` in one edit.

It degenerates at `sizeof(T) == 1`, where the multiply is gone and the two
operands are just an index and a base: `VIArray<VIWorldRegion>::Allocate` is the
template's one open function, by that single `addu`.

### The lever: `DestroyPool`'s index is a COPY, and `Next()` runs BEFORE the body

`DestroyPool` is `VIPool`'s Begin/End/Next walk again — `End()` is `0x7FFFFFFF`
— with the element destructor in the body, and all three helpers are **inline in
the class body**: no `Begin`/`End`/`Next` symbol exists for any instantiation,
where `VIPool`'s three are emitted out of line.

Written the obvious way, gcc strength-reduces `i * sizeof(T)` into an induction
variable and hoists the multiply out.  The target multiplies inside the loop and
destroys the index the loop variable held *before* `Next` advanced it:

```c++
    int i = Begin();
    while (i != End())
    {
        int j = i;
        i = Next(i);
        m_data[j].~T();
    }
```

`j` is not an induction variable, so no giv is formed.  Invisible in eighteen of
the twenty-two, whose element has no destructor and whose body is therefore
empty.  `Next` itself needed Phase 7's if-direction: `if (i != m_size - 1)
return i + 1; return End();` gives `bne`, the inverted form gives `bnel`.

### The element types are read off the two loops

Neither loop needs the element class to exist.  `Allocate`'s address arithmetic
states `sizeof(T)`; its loop body IS the default constructor, inlined;
`DestroyPool`'s body is the destructor or nothing.  That was enough to model all
twenty-two from the object code alone.  Four are worth stating:

* **`long` is eight bytes, said a third way** — `VIArray<unsigned long>` shifts
  by 3, and `VIHSpriteTrigger`'s constructor zeroes +8 with `sd`.
* **`VIPlane` has a zeroing default constructor** — four zero stores, where the
  implicit one would run `VIVect3`'s and leave `d` alone.  Regressed nothing.
* **`VIParticleEmitter` is a MEMBER of `VICSpritePartEmitter`, not a base** —
  the destructor call lands at element+16 and a base subobject would sit at 0.
  Modelled that way, all six of its symbols matched first build, the
  53-instruction `Allocate` included.
* **`VIRadialFloraInst` constructs its first 64 bytes twice and its next 16
  twice again** — `VIVect4[4]`'s constructors, then `VIMatrix44`'s own VU0
  `vsub`+four `sqc2` over the same quadwords (Phase 22), then Phase 19's
  `VISphere` double-zero underneath.  Its implicit destructor inlines to
  `~VIMatrix44`.

### `VIClipStack`

Three running counts, each one word in front of the array it counts, plus two
scratch vertex arrays; `VIClipEntry` is four ints.

```
    +0x00 int                  m_numEntries
    +0x04 VIArray<VIClipEntry> m_entries     500
    +0x0C int                  m_numVerts
    +0x10 VIArray<VIVect3>     m_verts      2000
    +0x18 int                  m_numPlanes
    +0x1C VIArray<VIPlane>     m_planes     2000
    +0x24 VIArray<VIVect3>     m_clipA        32
    +0x2C VIArray<VIVect3>     m_clipB        32
```

**`Push*` returns the index of the entry it pushed**, and the spelling matters:
`return m_numEntries++;` costs an extra `move v0,v1`, where `int n =
m_numEntries; m_numEntries = n + 1; return n;` loads the old value straight into
`v0`.  That took `PushBBox` from 264 wrong words to 153 *and* from 283
instructions to the target's 282.

**`&m_entries.m_data[m_numEntries - 1]` is written with the decrement first** —
the subscript form gives `sll` then `addiu -16`, the target gives `addiu -1`
then `sll`, so the index is a named local.  Three functions.

**`PushTranslatedVolume` never reads its `numPlanes` parameter** — its first
instruction is `move a3,a0`, `this` into that parameter's register, and both
loops are bounded by `numVerts`.  For a portal volume the two counts are equal,
so the bug is invisible in the only caller shape that exists.

### Harness

Eighteen new `SPAN_CAP` entries.  Every TU that uses a `VIArray` emits its own
**static** copies of the pieces gcc chose not to make weak — a second
`DestroyPool` beside the constructor, a second `Clear`/`Allocate` beside `Init`
— so ten constructors, three destructors and five `Init`s acquire the span of an
unnamed twin (`__t7VIArray1Z14VIResourceElem` derived 1272 bytes for a
four-instruction function).

### Open

`VIArray<VIWorldRegion>::Allocate` (one `addu`); `CullByTop(VIBBox)` 7/64 (one
`.p2align 3` pad — the target hoists the `sp+0` load out of the loop and ours
does not); `PushTranslatedVolume` 10/98 (the second loop swaps `a0` and `a1`);
`PushBBox` 153/282, **and that count is misleading** — normalise the register
names and the two differ by ONE adjacent-pair swap, the allocator giving the
plane pointer `a2` where the target gives it `t0`.  All six permutations of the
declaration order scored identically.  Not attempted: `ClipByTop`, both
`PushPortal` overloads, and the two `VICamera`-reading `Push*Frustum`.

Long form: `claude-notes/decomp-phase24-viarray-and-clipstack-20260823.md`.


## Phase 26 result: the RED-BLACK TREE, four templates of it, and the LIST

**1004/1098 -> 1138/1268**, and 134 of the 166 symbols in this phase.  The engine's ordered containers: one CLRS red-black tree
written out three times (`VISet<T>`, `VIMultiMap<K,V>`, `VIMap<K,V>`) over
eleven instantiations, the heap-allocated `VIList<T>` over five, and
`VIDictionary`, which is nothing but a name for one of the maps.  Same
toolchain, no re-tuning; 11 of `VISet`'s 16 were byte-exact on the first build,
including the 238-word `EraseFixup`.

### The tree

CLRS verbatim, laid over VIPool's index-addressed free list rather than over
pointers: every link is an `int` index into one `new char[]` block, so a node
keeps its identity across a reallocation.

```
    +0x00 m_size      live count, the SENTINEL included
    +0x04 m_capacity  node slots allocated
    +0x08 m_nodes     Node[]
    +0x0C m_root      0 when the tree is empty
    +0x10 m_free      head of the free chain, -1 when exhausted

    Node { int m_color; int m_parent; int m_left; int m_right; <payload>; }
```

**Slot 0 is the NIL sentinel**, said three ways: `AddNode`'s first growth step
is to **TWO** slots, `Allocate` zeroes `m_nodes[0]`'s four link words on every
path, and `End()` returns **0** where VIPool's returns `0x7FFFFFFF` — the
sentinel index IS the past-the-end iterator, so `Begin()` on an empty tree and
`End()` agree by construction.  A free node re-uses `m_parent` as the chain
link and carries **-2** in `m_left`, exactly as VIPool marks `m_prev`;
`m_color` is 1 for RED.

`VIMap` is `VIMultiMap` with a `Find` and a `Key`; `VISet` is the same tree
with one payload instead of two.  The three are separate source, and they
disagree in ways one template with different arguments could not: **`VISet`'s
and `VIMap`'s reallocation loop copies from index 1 and `VIMultiMap`'s from
0** (`slt 1,m_size` against `blez m_size`).

### The levers

**The fixup loop's parent is a LOCAL, and `EraseFixup`'s is not.**  `Insert`'s
red-black fixup reads `m_nodes[x].m_parent` into a local once per iteration:

```c
int q = NODE(x)->m_parent;
if (q == NODE(NODE(q)->m_parent)->m_left) { ... }
```

Written out as `NODE(NODE(x)->m_parent)` at every use — the way `EraseFixup`
IS written, and byte-exact there at 238 words in five instantiations — each
colour store forces the parent index to be re-loaded, because the store may
alias the very word it came from.  With the local the target keeps
`q * sizeof(Node)` in one register across the whole body and **the two
uncle-is-red arms cross-join into a single tail**; without it they do not, and
the function is **20 words longer**.  Per function, like phase 20's
if-direction.

**Placement new is what null-tests the payload address.**  `Insert` writes the
value through a pointer it has just null-tested, and the tested pointer is
**node+16** — VIPool::Add tests the node itself only because its payload is at
offset 0.

**The pseudo-destructor is invisible until it isn't.**  `FreeNode` runs
`NODE(i)->m_value.~V();` and `DestroyPool` runs it AGAIN, after `Erase` has
already freed the node, indexing by a COPY of the loop variable taken before
the erase.  For every POD instantiation both calls vanish and the functions are
the 16 and 21 words they were without them; for `VIMultiMap<float,
VIEmitterEntry>`, whose value carries a `VIMatrix44`, they are 33 and 34.

**`Size()` discounts the sentinel** — `int n = m_size - 1; if (n < 0) n = 0;
return n;`, six words of `slt v1,-1,v0` + `movz`.  **`Key` returns a
REFERENCE**: its last instruction is `addiu v0,v0,16`, an address, not a load.

**`Clear`'s closing stores** are phase 16's rule again: the block's LAST store
of a given constant is the one hoisted to the front.  `VIList`'s is `m_tail`,
and getting the order right fixed all five instantiations in one edit.

### `VIList<T>` — the SIXTH template, and 25 of 25

The one container in the engine that is not laid over a block of slots: every
node is its own `new`, and **the iterator is the node ADDRESS carried in an
`int`**, so the interface still looks like VIPool's.  `+0x00 m_size`,
`+0x04 m_head`, `+0x08 m_tail`; `Node { T m_data; Node *m_next; Node *m_prev; }`.
`End()` is **0x7FFFFFFF** here and not the tree's 0 — a null pointer is not
available as a sentinel when the iterator is a pointer.

**`new Node` is not null-checked by gcc 2.95** (that is `-fcheck-new`), so the
element's initialiser — the copy for a pointer element, the constructor call
for a value one — lands AHEAD of the source's own `if (n == 0) return -1;`.

Two class sizes came out of the two VALUE instantiations: **`VIActor` is 160
bytes with alignment 16** (its node is 176, not the 168 the members account
for) and **`VIMaterial` is `aligned(16)` too** (node 496, not 488) — an
attribute that moves no member, which is exactly why phase 15 could not see it.

### `VIEmitterEntry` was two words and is eighty

`vi.h` had carried a guess since phase 19.  `VIMultiMap<float,VIEmitterEntry>`
refutes it four ways at once: the node stride is **112**, the key sits at +16
and the value at **+32**, `Insert` copies the value as two `sw`s plus a `jal`
on `(dst+16, src+16)`, and `FreeNode` destroys node+**48**.  All four are
satisfied by `{ int m_a; int m_b; VIMatrix44 m_matrix; }` — 80 bytes,
alignment 16, with eight bytes of padding at +8 that **the implicit
`operator=` does not copy**, which is how they are known to be padding rather
than members.  That in turn says **`VIMatrix44` has a user-declared
`operator=`**: the memberwise copy reaches the matrix as a CALL where a
synthesised trivial assignment would be a `lq`/`sq` block move.

### `VIDictionary` is a name

Every member forwards to `VIMap<unsigned, VIDictEntry>`, its FIRST and only
member, so `a0` goes straight through at every call.  `VIDictEntry` is
`{ VIResourceType m_type; int m_index; }` at node +20/+24.

### Harness: `SPAN_IGNORE`

Four map entries — `VIRStrip` … `VIRStrip4` — have addresses that are **not
function starts**: they land in the MIDDLE of a body (`VIRStrip` is the last
six instructions of `VIMap<unsigned,VIDictEntry>::Clear`, `VIRStrip2` is
`AddNode`'s `jr ra` and its delay slot).  Left in the address list they
truncate their host by exactly their own length and the host reads as "six
words short" forever — `EraseFixup` came back as 178 words instead of 238.
`eqdecomp.SPAN_IGNORE` drops them from the gap derivation and gives them span
0, the treatment `foreign` already gets;
`test_span_ignore_names_land_inside_a_function` holds each one to the claim and
checks two known function starts FAIL the same test so it cannot pass
vacuously.  Three new `SPAN_CAP` entries as well.

**Phase 61 added the other case, `SPAN_ALIAS`.** The `VIRStrip*` names are VU1
microprogram labels, and `VIRStrip6`'s value happens to EQUAL a real entry
point, `VIEEUnloadIrx`'s. In SPAN_IGNORE it would drop that address and take
the function's span with it, so an alias keeps the address and only the alias
gets span 0. Until then the census counted it as the last unwritten
SUPPORT function. `test_span_alias_names_shadow_a_real_function` checks
three things: the two share an address, the real function keeps a span that
ends after a `jr ra`, and `VIRStrip5` fails the address check as a control.
The same phase moved `test_engine_fingerprint_profile` onto UNCAPPED spans. A
build profile covers all compiled code, and phase 61's 110 caps had cut the scan
from 219,896 instructions to 189,583, under its own 200,000 floor.

### Open

`Insert` in every instantiation (95–105 wrong of ~180): the shape is right to
the instruction and what differs is which register the parent local lands in
(`a1` in the target, `a2` in ours), which costs two `move a1,sN` before the
rotate calls — six source permutations move none of it.  `Erase` at 34 of 117:
CLRS's `y = (left == nil || right == nil) ? z : successor(z)` **branches** in
the target, and all six spellings measured — the conjunction, the nested
if/else, initialise-then-overwrite, the three-armed chain — if-convert to one
`movz`/`movn`.  `Allocate` at 4 of ~86 everywhere (two strength-reduced
pointers whose `addiu`s come out in the opposite order; seven spellings, all
identical), `Next` at 6 of 45, and the two `Allocate`s whose element copy is
not a block move.

Long form: `claude-notes/decomp-phase26-the-tree-templates-20260824.md`.


## Phase 28 result: the PARTICLE SYSTEM, and `sizeof(bool)`

**1,252/1,384 -> 1,329/1,468.**  Five classes, 77 of the 84 functions written,
60 of them byte-exact on the FIRST build, on the settled toolchain with no
re-tuning:

| class | SUPPORT | score |
|---|---|---|
| `VIParticleMotif` + `VIParticleDefinitionEx` | 0x0045A3D8 | 24/24 |
| `VIParticleDefinition` | 0x004565A0 | 10/10 |
| `VIParticleSystem` | 0x00458780 | 10/12 |
| `VIParticleEmitter` | 0x00456910 | 23/25 |
| `VIParticleSprite` | 0x00454E38 | 10/13 |

The six large ones are deliberately not attempted: `VIParticleEmitter::Update`
(696 instructions), `::RecomputeStaticBBox` (510), `VIParticleSprite::Release`
(660) and `::Raster` (394), `VIParticleSystem::Render` (332) and
`VIParticleSprite::Pick` (174).

### `sizeof(bool)` is FOUR on this ABI

`VIParticleDefinitionEx::SetZWriteEx(bool)` stores its parameter with `sw`, and
`VIParticleDefinition::Create` hands that same member to another `bool`
parameter with **no `sltu`** in between.  Neither fact survives an `int` member,
and neither survives a one-byte `bool`.  Measured on the compiler:
`sizeof(bool)` is `li $2,4`, a `bool` member stores with `sw`, and an
`int`-to-`bool` argument conversion is `sltu $4,$0,$4`.  So the two render flags
are `bool`, four bytes each, and the `sltu` that *does* appear -- inside
`VIParticleDefinition::SetZWrite`, feeding `VIMaterial::SetZWrite(int)` -- is
the source's own `!= 0`.  Invisible for twenty-seven phases because no earlier
class had a `bool` member.

### The inline `Value(i)` lever, at four sites at once

`VIParticleDefinitionEx` walks its `VIPool<VIParticleMotif *>` four times.
Phase 22's pointer-variable spelling gives `addiu sN,a0,724` + `move a0,sN`
where the target has **`addiu a0,a0,724` + `move sN,a0`** -- six words of
prologue apart, and no declaration order, reference-vs-pointer form or loop
shape moves it (twelve swept).  Phase 27's lever does: an inline
`T &Value(int i) const { return m_nodes[i].m_data; }` in the class body, whose
own `this` is a SECOND pseudo copied out of the argument temporary.  All four
went byte-exact in one edit.

### The hoisted constants keep their SOURCE order

Phase 16: the last store of each distinct constant is hoisted to the front of a
constructor's store block.  `VIParticleDefinition`'s nine stores pin the rest of
it down -- **the hoisted stores come out in the order their source statements
do**, and the remainder follows in source order with those two removed.  Reading
the target that way gives the source's statement order outright.

### `VIBBox` has a user COPY constructor

`VIParticleEmitter`'s three by-value bounds accessors all **zero the caller's
slot before writing the six floats into it**, where a synthesized copy
constructor would go straight to `VIVect3`'s (Phase 4) and zero nothing.
`VIBBox(const VIBBox &b) { min = b.min; max = b.max; }` -- the members' own
default constructors are the zeroing.  The 106-instruction `StaticBounds`
matched on the first build with it.

### A `VIResource` MEMBER, not a base

`ReleaseParticleDefinition` hands `d + 0x2FC` to
`VIDictionary::Remove(VIResource &)` and decrements `(d + 764) + 4`, so
`VIParticleDefinition` *carries* a `VIResource` rather than deriving from one.
Its inline zeroing constructor -- a chained `m_id = m_link = 0`, so `+4` stores
first -- is exactly the pair that opens `VIParticleDefinition`'s own
constructor.  Both refcount sites need Phase 9's pointer variable or the offsets
fold to 768.

### `NextMotif` does not return early

Found-and-last falls out of the `if` and keeps walking the pool rather than
returning 0.  Writing the early return costs 17 of 56 words: it makes `this`
dead on that path and collapses two live ranges the target keeps apart.  Phase
7's "the success path goes in the `if`" paid twice more, and Phase 13's
`if (guard) { ... } return 0;` nesting took `ReleaseParticleDefinition` from 10
wrong to 0 by letting its two `return 0` sites cross-jump.

### A by-value `Cross`, and its surviving temporary

`VIParticleEmitter::SetDirection(const VIVect3 &)` writes 12 bytes of its frame
three times and never reads them.  That slot is the compiler's temporary for the
two statements whose right-hand side is a class VALUE (`up = VIVect3(0,0,1);`
and `up = Cross(v, a);`); its address is taken by the reference
`VIVect3::operator=` takes, so the stores survive after the values are
forwarded.  The initialisation two lines up produces no such slot.  120
instructions, first build.  Note this is the by-VALUE cross, not
`viwalkcontrol.cpp`'s reference-out `Cross`.

### Layouts settled

`VIParticleMotif` = attributes + `char[32]` + hash, **760** (what `CreateMotif`
hands `__builtin_new`).  `VIParticleDefinitionEx` adds the motif pool at +0x2D4
and four flags, **764**.  `VIParticleDefinition` adds the `VIResource`, a
raster, a material and a texture index, **784**.  **`VIParticleEmitter` is 992**
-- a `VILNode`, two `VIVect3`, a `VIMatrix33` at +0x28, a `VIMatrix44` at +0x60
(whose `aligned(16)` pads +0x58), two `VILList`, a whole `VIParticleAttributes`
at +0xBC so +0x390 closes it exactly, then a static and a dynamic `VIBBox`.
**`VIParticleSprite` is 1076**, a `VISprite` with the emitter EMBEDDED at +0x50;
`m_type` is 10 and both `Collide` overrides are `return 0` stubs, as
`VISkinSprite`'s and `VIFloraSprite`'s are.  **`VIParticleSystem` is 108 and is
EMBEDDED IN `VIScene` at +0x548** -- `VIParticleSprite::Create` and `::Copy`
reach it as `scene + 1352` and null-test the result.  **A particle is 104 bytes
on a `VILNode`** with fourteen words zeroed at +0x20..+0x54, read straight off
gcc's own `new VIParticle[n]` array-init loop.  And **Phase 12's unnamed float
at `VIParticleAttributes::+0x2C8` is the MOTIF WEIGHT**: the two `BlendMotifs`
write it from `VIMotifBlender::Factor()` and read it as each motif's blend
factor.

Shipped oddity: `VIParticleSprite::Copy` calls `SetLocation(src.Location())`
**twice** -- `SetLocation` shifts current into last, so the copy ends with both
equal and the source's last location silently dropped.

### Open

`VIParticleSystem::Init` at 33 of 84: the target's `new VIParticle[n]` loop
carries a SECOND induction variable biased by +72, so its fourteen stores
address at -40..+12 while the array pointer advances ahead of them; ours reuses
the one pointer.  `ReleaseAllParticleDefinitions` at 15 of 36: one extra
callee-saved copy of the pool pointer, the standing phenomenon, seven spellings
swept.

Long form: `claude-notes/decomp-phase28-the-particle-system-20260824.md`.

## Phase 27 result: eighteen SMALL CLASSES, and the assert that dates a file

**1138/1268 -> 1252/1384: 114 of the 116 written, same toolchain, no
re-tuning, 96 of them byte-exact on the first build.**

Every phase before this took one class or one template and read it out in
depth.  This one is the opposite sweep -- eighteen classes that between them
are smaller than `VIWalkControl` alone, each self-contained, each settling a
layout a bigger class will need: `VICameraEffect` 13/14, `VITimer` 4/4,
`VILanguage` 5/5, `VIActorFilter` 5/5, `VISceneFilter` 8/8, `VISceneQuery` 8/8,
`VIMotifBlender` 5/5, `VIEnvRay`+`VITransEnvRay` 9/9, `VIWorldTree` and
`VIZoneTree` 8/8, `VIGameOptions` 10/10, `VIConsole` 4/4, `VIWnd` 13/13,
`VIClipper` 2/2, `VIQueryTriList` 7/7, `VIStaticLighting` 3/3,
`VIStringTable` 6/6 and `VITimeOfDay` 4/5.

### An assert's LINE NUMBER is an immediate

`VILanguage`'s two range checks call `_VIAssert(file, expr, line)`.  The first
two arguments are relocation sites and the differ masks them away, but the
third is `li a2,59` and `li a2,69` -- `__LINE__`, plain immediates that have to
match.  `src/vilanguage.cpp` is padded so its asserts land on those lines, and
the padding is marked as filler in the file: it measures the original's line
layout, it does not claim to reproduce its contents.

Two smaller facts from the same functions.  **The bound is
`sizeof(Name)/sizeof(Name[0])`**: the compare is `sltiu`, and an enum whose
values fit in `int` promotes to `int`, so only an unsigned right-hand side is
unsigned.  And **`GetID` returns `int`, not the enum** -- its failure path is
`li v0,-1`, where `(VILanguageID)-1` needs `lui`+`ori`.

### A ternary VALUE gives `movn`; every `if` spelling gives `movz`

`r = (FixupTextBuffer() > -1) ? 0 : -1;` materialises -1 and selects with
`movn`.  Six `if` spellings of the same test fold to `slti v0,v0,0` and select
with `movz`.  The exact dual of Phase 12, where the vector setters had to be an
`if`/`else` and NOT a `||` value: neither form is "the" idiom, and the two are
distinguishable per site.

### The element pointer's index goes in its INITIALISER

Phase 24's element address is OFFSET-first; two functions here need BASE-first,
and the spelling that gives it is `int *p = m_data + i; return *p;` -- not
`p[i]` with `p` naming the array, and not the subscript.  The index folded into
the pointer's initialiser is what makes the address a materialised VARIABLE in
Phase 3's sense.  `VIStaticLighting::ReleaseResources` needs both halves at
once: the ARRAY through a pointer declared INSIDE the loop (so its address is
computed after the guard) and the ELEMENT through a second one.

### A container's inline `Value(i)` keeps `this` on the container

`m_sorted.m_nodes[i].m_value` reads the node array off the OUTER object.  An
inline `V &Value(int i)` in the class body makes `this` for the accessor
`&m_sorted`, which CSEs with the `a0` of the `Begin`/`End`/`Next` calls -- the
same fix in `VISceneQuery::EndQuery` and in `VIWnd`'s destructor and `OnDraw`.
No instantiation emits a symbol for it, the usual tell for an inline member.

### A store through a REGISTER base kills gcc 2.95's whole memory table

The most useful finding came out of the one function that did not match.
`VICameraEffect::Apply` RE-LOADS all six components of two vectors immediately
after storing them, and every natural spelling lets gcc forward the stores and
constant-fold the `(0,1,0)` up vector away -- 130 words wrong.  What stops it is
that ONE of the three stores goes through a register base, because the local's
address was materialised for a later `Normalize()`: gcc 2.95's `cse` cannot
relate `(mem (plus (reg) (const)))` to a frame slot, so it invalidates its
ENTIRE memory table there and the six loads have to be re-issued.  Phase 9's
pointer-variable lever, reached for a reason that has nothing to do with
addressing.

### Read a switch's data table out of the RELOCATED dump

`VITimeOfDay::GetDayOfWeek` is a twelve-case switch with ten bodies, and the
`.text` cannot say which month maps to which -- the jump table is in `.rodata`
and the image is un-relocated.  `memory-dumps/inworld-eqoa.live.eememory` can:
the relocated `lui`/`addiu` pair gives the table at 0x004E9CE0 and its twelve
entries give the month offsets outright, `{6,2,2,5,0,3,5,1,4,6,2,4}`.  May's
entry points straight at the join (offset 0, no block at all) and September and
December share one.  The same dump `eqcalls.py` uses as a link map, used for the
first time to recover a DATA table rather than a callee.

### Layouts settled

**`VIActorFilter`, `VISceneFilter` and `VISceneQuery` each have exactly ONE
virtual and NO base** -- all three `__tf`s take TWO arguments and all three
vtables are 24 bytes; `VISceneQuery` is NOT a `VISceneFilter`.  **`VIEnvRay` is
the first inheritance WITH a vtable** (`VITransEnvRay`'s `__tf` is the
three-argument `__rtti_si`).  **`VIWnd` has SIX virtuals**, and this is the
first class whose declaration order was DERIVED from Phase 17's `8N` slot
arithmetic rather than checked against it -- all thirteen functions matched on
the first build.  **`VIQueryTri` is the corpus's only real `new T[n]`**, gcc's
own vec-init loop, which is the control for Phase 24's finding that `VIArray`
has no such loop.  **`_7VITimer$SystemTime` is EIGHT bytes**, `long` said a
fourth way.  **`VIStringTable::GetString` takes a ONE-BASED row** and `Unload`
returns **void** where everything around it returns `int`; **`LoadRawFile` and
every member of `VITimeOfDay` are STATIC** (`this` never reaches `a0`).
**`VIGameOptions` is a 1024-byte card save** with magic `0x01234567` at +0x3FC,
and `Load` calls `SetDefaults` when the FIRST WORD IS NON-ZERO.

Harness: three new `SPAN_CAP` entries, all the same failure -- an unnamed static
in the gap after a named function, twice gcc's own
`__static_initialization_and_destruction` (the Phase 14 case again).
`test_span_caps_only_ever_shrink` rejected two of them for ending one word too
late, which is what the rule is for.

### Open

`VICameraEffect::Apply` at 56 of 312: everything structural is settled and every
remaining word is which of nine pointer pseudos loses a callee-saved register
(eight permutations measured).  `VITimeOfDay::GetLocalTime(char *, int)` at 9 of
80: all registers agree and the one difference is whether `move t0,v0` lands in
the `jal`'s delay slot.

Long form: `claude-notes/decomp-phase27-the-small-classes-20260824.md`.

## Phase 34 result: the effect volume, the zone room and the last value pools

**1550/1707 -> 1624/1786.**  Four groups on the settled toolchain with no
re-tuning, 74 of the 79 written byte-exact and 53 of them exact on the FIRST
build:

| group | symbols | byte-exact |
|---|---|---|
| `vieffectvolume.cpp` — `VIEffectVolume` (24 of 38) + `VIBoid` (5 of 5) | 29 | **28** |
| `vipool.cpp` — three new VALUE instantiations, plus `Insert` and `MoveToLast` | 38 | 35 |
| `vizoneroom.cpp` — `VIZoneRoom`, the whole class | 11 | 10 |
| `vieffectvolumesprite.cpp` — `Copy`, closed by a layout correction | 1 | **1** |

**A delay slot reads out which statements are OUTSIDE the `if`.**
`VIEffectVolume::SetType`'s `bc1f` is not annulled and carries the store of
`m_type` in its slot, so that store runs on both paths — `m_type = t;` is above
the `density >= 0` guard, not inside it.  Written inside, the function is 9 of
24 words wrong **and the two parameters swap callee-saved registers**: the
correct spelling kills `t` right after the call, its live range collapses, and
its allocation priority (refs / live-length) takes `s0` from `this`.  Five
spellings of the condition moved neither symptom; one moved statement fixed
both.  `VIBoid::Free` and `VIEffectVolume::Process` are the same shape.

**The mangled name is the argument list.**  `Init__6VIBoid` takes nothing.
Modelled as `Init(VIEffectVolume &, const VIBBox &)` on the strength of what a
boid plainly needs, `AllocFlock` came out 17 of 43 words wrong — every one of
them the same `s0`/`s1` swap — and `FreeFlock` 1.  Deleting the arguments made
both byte-exact, and the 156-instruction `VIBoid::Init` matched on its first
build afterwards.

**The jump table decides the case labels**, phase 30's rule used forward.
`AttachSurface`'s five-entry table at `0x004EABF0` says cases 1, 3 and 4 share
one body and case 2 is separate; three distinct bodies written as an `if`/`else`
chain (or a `switch` with a `default`) emit comparisons and no table at all —
21 of 96 words.  `Process` additionally needs an EMPTY `case 0: break;` so the
table starts at zero instead of gcc subtracting one.

**The sentinel spelled out, not `End()`.**  `VIPool<T>::Insert` compares `i`
against `lui 0x7fff`/`ori 0xffff` with **no call**, where every other use of
`End()` in `vipool.cpp` is a `jal` — and `Prev`, four functions down the same
instantiation, still calls it.  One line, 12 wrong words to 0.

**A layout correction closed a function in ANOTHER TU.**
`VIEffectVolume::Density()` is `lw v0,44(a0)`, so `m_density` is +0x2C;
`claude-notes/effect-volume-system.md` has it at +0x28 and every field after it
one slot early.  `VIEffectVolumeSprite::Copy` had been open since phase 32 for
that reason alone: renaming the four fields it copies took it to byte-exact with
no other change.  New with it, **`VIScene+0x4B9C` is the `VIDictionary *`**,
immediately in front of the raster pointer phase 31 already had at +0x4BA0.

**Layouts.**  `VIEffectVolume` is **112 bytes** (a `long` timestamp, a `VIBBox`
whose members zero themselves twice, the type/density pair, three resource
indices and a wind vector), with `mScene` a file-scope `static VIScene *`;
`VIBoid` is **176**; `VIZoneRoom` is **120** — exactly the stub size phase 24
measured off the `VIArray<VIZoneRoom>` node stride, so the stub could be
deleted.  A zone room is `VICollBuffer`'s shape a third time: ONE `new char[]`
holding a 32-byte portal record per portal (a `VIPlane`, then first/count/dest)
followed by the portals' vertices, with both section pointers held as byte
OFFSETS until `Init` rebases them.  `CalcPortalPlane` is **Newell's method with
the centroid accumulated in the same loop**, and its `Normalize` result is
compared against `0.0f` — the object code runs `cvt.s.w` on the `int` first.
The three new pool elements were modelled from their own two loops: `VIEEBucket`
and `VIRasterMaterialLRU` are 16 bytes and POD (`ldl`/`ldr` copy),
`VILoaderTrans` is **48 with alignment 8** (seven aligned `ld`/`sd` pairs), and
none of the three takes phase 25's `VIPOOL_VALUE_COMMON` — each asks for a
different subset.  `MoveToLast` (the LRU touch) matched byte-exact first build
at 55 instructions.

**Shipped.**  `VIBoid::SeekGoal` is EMPTY — `jr ra; nop`.  `VIBoid::Apply` and
`VIEffectVolume::Process` both ignore their `dt`: Process takes the time from
`VITimer::GetTime()`, skips below 1/60 s and clamps at 0.1 s.  `AllocParticles`
calls `FreeParticles()` and then calls it again in the `case 0` arm.
`AttachSprite(-1)` returns -1 without detaching.  And a boid seeds every field
from **two `rand()` draws differenced and scaled by 2^-31** — sixteen calls in
one `Init`.

**Harness.**  Four new `SPAN_CAP` entries, all a template's unnamed twins in the
gap after a class's last named symbol (`Clear__t6VIPool1Z10VIEEBucket` 104,
`Init__t6VIPool1Z13VILoaderTransi` 80, `Erase__t6VIPool1Z13VILoaderTransi` 220,
`Allocate__10VIZoneRoom` 72).  Without the first, a 26-instruction `Clear` reads
as 117 and scores 91 wrong.

### Open

Five.  `AllocParticles` 5/103: twelve spellings swept and the only difference
left is `slt`+`movn` against `slti`+`movz` on a clamp — phase 27's "a ternary
gives `movn`, an `if` gives `movz`" is not enough, because the ternary gives
`movn` only when gcc cannot coalesce the result with the value, and here it
always can.  `CalcPortalPlane` 37/145: the loop body is byte-exact once Newell's
three terms are written BEFORE the centroid accumulation (91 wrong -> 37) and
everything left is `this` and `i` in each other's callee-saved registers.  And
the three pool `Allocate`s, which are phase 25's standing miss.

Long form: `claude-notes/decomp-phase34-effect-volume-zone-room-and-the-last-pools-20260826.md`.

## Phase 33 result: the lightning bolt and the motion trail

**1517/1671 -> 1550/1707.**  Two whole translation units on the settled
toolchain with no re-tuning, 33 of 35 byte-exact and 25 of them exact on the
FIRST build:

| TU | symbols | byte-exact |
|---|---|---|
| `vitrailfx.cpp` — `VITrailFx` + 5 free functions + `_GLOBAL_$I$`/`$D$` | 16 | **16** |
| `vilightning.cpp` — `VIBoltHarmonic`, `VIBolt`, `VILightning` + 2 free | 19 | 17 |

Both are self-contained effect layers with a file-scope pool in front of them,
so everything under them (`VIRaster`, `VIMaterial`, `VILList`, `VIMatrix44`,
`VIVect3`) was already settled and every miss is codegen.

**The layouts.**  `VITrailFx` is 56 bytes on a `VILNode` — a `long` timestamp,
a life span, a used list, a free list and a `SEGMENT *` — and a `SEGMENT` is 76:
`VILNode`, `VIVect3 m_pos[2]`, `VIColor32F m_color[2]`, `float m_age`.
`VIBoltHarmonic` is **428** (`VIVect3 m_samples[2][17]` at +4, four scalars from
+0x19C, and four unread bytes at +0), `VIBolt` **1552** (three harmonics, a
17-point ring, a `VIMatrix44`) and `VILightning` **8832**, which is exactly what
`claude-notes/spell-effect-system.md` measured the allocation to be.  The whole
of it falls out of the constructor's member-construction loops.

**Six levers.**

* **A pointer copy of the array decides which of three loop shapes gcc emits.**
  `for (i = 0; i < 17; i++) m_samples[which][i].x = …` is REVERSED into a
  countdown on `bgez`; `for (s = base; s < e; s++)` is forward but `sltu` with
  an entry guard; `VIVect3 *s = m_samples[which]; … s[i].x = …` is forward,
  **`slt`**, no guard — the target.  Naming the array through a pointer lets the
  `loop` pass build the induction variable out of the COPY, which keeps the
  signed comparison of the original `int` bound and keeps the trip count
  constant.
* **A returned temporary's stack slot is reused by the next declaration.**
  `r.Color(VIColor32F(…));` frees `sp+0` at the end of the statement, so a
  `VIVect3 p;` declared AFTER the call lands on it.  Declared before it, the
  frame comes out 272 bytes where the target's is 288 and every local moves:
  `RenderBoltVertices` **127 wrong words to 0** on moving two declarations down.
  The same sharing is why a `VIVect2` handed to `VIRaster::UV` and a `VIVect3`
  handed to `VIRaster::Vertex` both live at `sp+32`.
* **Two `if`s or an `if`/`else` decides which copy of a duplicated call goes out
  of line.**  `VITrailFx::Update` has two paths to `ClearSegmentsForward`; an
  `||` emits one call, two `if`s emit two in the wrong order, and only the
  `if`/`else` with the null test as the ELSE matches.
* **`VIMIN(3, x)` gives `slti x,4`+`movn` where `VIMIN(x, 3)` gives
  `slti x,3`+`movz`** — phase 23's argument-order rule in the other direction.
  And `VIMAX(x, 70)` comes out as `li 69; slt 69,x`, because gcc canonicalises
  `x > 70` to `69 < x`: **a 69 in the object code is not a 69 in the source.**
* **Statement order inside an `if` body decided a branch-likely.**  Moving
  `m_ringHead = (m_ringHead + 1) % 3;` to the FRONT of `VILightning::Render`'s
  three-assignment ring advance took it from 27 wrong words to one, by stopping
  gcc hoisting the join's reload above the branch — so the delay slot is filled
  from the target and the branch becomes `bnezl`.
* **Where a value is computed relative to a CALL decides its register class.**
  `VIBolt::Update`'s `start`/`end` written AFTER the `Sample`/`Mul` calls are
  dead across them and go caller-saved; written BEFORE they are live across
  them, go callee-saved, and the pressure pushes the `VIMatrix44 &` parameter
  out to a stack slot — which is what the target does.  **76 wrong to 35** on
  moving two lines up.

**Shipped bugs.**  `VITrailFx::ClearSegmentsForward` re-reads `m_next` through
its ARGUMENT rather than the cursor, so after the first node has been moved to
the free list the second read returns that list's tail sentinel and the loop
stops: it retires **at most two** segments per call, whatever its name says.
`VITrailFx::Render` stores the UNCLAMPED alpha before the clamped one.
`VIBoltHarmonic::Create`'s `count` argument is stored at +0x19C and never read
(the sample count is a hardcoded 17 everywhere), and +0x000 of that class is
never touched at all.  `VILightning`'s four ribbons only work because `off` is
NOT re-zeroed per iteration: case 3 negates whatever case 2 left in it.

**Harness.**  One new `SPAN_CAP` (`ClearSegmentsForward`, 104 — the TU's three
file-scope `VILList`s give it a static constructor whose unnamed `__sti`
routine sits in the gap), and **`eqcalls.py` now reads STRING LITERALS out of
the dump**: a `lui`+`addiu` pair against a string holds only the addend in the
un-relocated image, and `name_at` could only report the nearest preceding
symbol, so `string_at` reads the bytes at the resolved address instead —

```
$ ./eqcalls.py calls VILightningFxInit__FR8VIRasterR12VIDictionary
  0x0044e208  addr  "lightning.tga"
  0x0044e218  jal   VIHashResourceID
```

Open: `VIBolt::Update` 35/294 and `VILightning::SetLocation` 42/111 (7 positions
register-blind), both the standing phenomenon.

Long form: `claude-notes/decomp-phase33-lightning-and-trails-20260826.md`.

## Phase 32 result: the remaining sprite leaves

**1464/1610 -> 1517/1671.**  The four `VISprite` leaves phases 19/20 did not
reach, on the settled toolchain with no re-tuning:

| class | written | byte-exact | first build |
|---|---|---|---|
| `VINameSprite` (+ `VINameDeferSort`, the two `_GLOBAL_$`) | 17 | 16 | 12 |
| `VISoundSprite` | 15 | 13 | 11 |
| `VIStreamAudioSprite` | 16 | 14 | 12 |
| `VIEffectVolumeSprite` | 12 | 10 | 9 |

The vtable is **96 bytes in all four** — phase 19's `B/8 - 2` rule a fifth
through eighth time, and no leaf in the engine adds a virtual.  Four new
`SPAN_CAP`s (`EnablePan` is a two-instruction setter with a 2,520-byte derived
span).

Levers:

* **`int k = 32; while (k--) *d++ = *s++;` is not any `for` loop.**  gcc's
  `check_dbra_loop` reverses `for (k = 0; k < 32; k++)` into a countdown exiting
  on `bgez`; only a source that already counts down through zero gives the
  target's `bne` against −1.  Four spellings measured; with the record copied
  through **pointers** rather than `NameDefers[i]` subscripts (phase 15's rule at
  a new site) `RasterDeferred` goes 92 wrong -> 0.
* **An inline member's `this` is the address pseudo a named field is not.**
  `at.Set(0.0f, 0.6f, 0.0f)` makes `&at` a pseudo whose live range crosses the
  two calls before its use, so it goes callee-saved exactly as the target has it;
  `at.x = 0.0f; …` keeps every store on `sp` and loses the register.  42 -> 14.
  Phase 9's pointer-variable lever through an inline member — the inverse of
  phase 31's "an address NAMED before a call".
* **`float d[3]` is not `VIVect3 d`.**  An array has no constructor, and gcc will
  not delete a constructor's zero stores across an intervening call.  106 -> 37.
* **Phase 31's braced blocks, at a second site** — two textually identical blocks
  need their own scopes or every FP register in both is renamed.  37 -> 3.
* **A `void` return frees `v0`.**  `VIEffectVolume::Free()` declared `int` puts
  the `m_id` load that follows in `v1`; declared `void` it takes `v0` and the
  function is byte-exact.  A new lever.
* **The if-direction is per function**, a fifth time: `VIStreamAudioSprite::Copy`
  wants the plain `if (i < 0) return -1;` guard (44 words the other way) and
  `VISoundSprite::Pick` wants the opposite of `VINameSprite::Copy`'s.
* **`_GLOBAL_$I$` is named after the first global OBJECT.**  `int C::N;` goes to
  `.comm` and never claims the name; `= 0` makes it a definition, puts it ahead
  of the next variable in `.bss` (which is the order the map has), and matches
  both `_GLOBAL_$I$`/`$D$`.

Shipped facts: **a name plate is a two-phase sprite** (`::Raster` enqueues into a
24-entry table; a STATIC `::RasterDeferred` sorts by depth, drops every plate
whose screen rect overlaps one already kept, and draws the rest), its
off-distance test sums the homogeneous `w²` so 22.0f is not a distance,
`VISoundSprite::Release` calls `IsPlaying`/`Stop` before null-testing the device,
`VIStreamAudioSprite::QueryEmitters` **returns −1** for a priority outside 0..2
and evicts `Begin()` — the SMALLEST key, which puts `VISprite::QueryEmitters`'
guessed `Last()` in doubt — and `VIEffectVolumeSprite` draws *and* picks with the
**stream-audio** debug marker, copies its volume's density twice, and steps the
effect on a hardcoded 1/30 s.

### Open

Seven, and two are named phenomena.  **Both** `QueryEmitters` in the corpus —
`VISprite`'s since phase 19 and `VIStreamAudioSprite`'s now — fail on the same
`gcse` merge of four `addiu sp,<n>` the target rematerialises, and nothing else
in the tree fails that way.  `VIStreamAudioSprite::Raster` is wrong in exactly
the same six words as `VIPointSprite::Raster` has been since phase 20 (the four
argument moves before `RasterUISprite`).  The rest are one to three instructions
of register allocation: `VISoundSprite::Raster` and `::Pick` reload `s.z` where
the target keeps it in an FPR (and `VIStreamAudioSprite::ClampScale`, the same
body out of line, is byte-exact allocating the way ours does),
`VINameSprite::Raster` computes an argument address before a call where the
target computes it after, and `VIEffectVolumeSprite::Raster`/`::Copy` differ by
three and two words of the same remat-vs-pseudo choice.

Long form: `claude-notes/decomp-phase32-the-remaining-sprite-leaves-20260826.md`.

## Phase 31 result: the CAMERA group, and an address named before a call

**1427/1567 -> 1464/1610.**  Three classes and a filter, on the settled
toolchain with no re-tuning:

| class | written | byte-exact | first build |
|---|---|---|---|
| `VICamera`           0x0041C7F8 |  8 of 10 |  8 | 4 |
| `VIFlyCamera`        0x00441210 | 13 of 13 | 10 | 4 |
| `VIWalkCamera`       0x00482020 | 20 of 21 | 19 | 8 |
| `VIWalkCameraFilter` 0x00484888 |  2 of  2 |  1 | 1 |

`VIWalkCamera` derives from `VIWalkControl`, the class Phase 18 matched at
21/27 and whose behaviour `go-eqoa-vi/viwalk` had already checked bit-for-bit
against the console — so the character half of this class was settled before a
line was written, and every miss below is codegen.

### The layouts

`VICamera` is 0x1F0 bytes and reads straight off `Init` and `Update`:

```
+0x000 float      m_near        +0x008 VIVect3 m_dir
+0x004 float      m_far         +0x014 float   m_fov        DEGREES
+0x018 float      m_aspect      +0x01C VIVect3 m_position
+0x030 VIMatrix44 m_view        right/up/fwd as COLUMNS, row 3 the dots
+0x070 VIMatrix44 m_proj        copied wholesale from Init's local
+0x0B0 VIMatrix44 m_world       right/up/fwd/pos as ROWS -- the inverse
+0x0F0 VIMatrix44 m_viewProj    m_view * m_proj
+0x130 VIFrustum  m_frustum     eight corners in WORLD space
```

The two thin `operator()` overloads name the four scalars: each forwards
`(m_fov, m_aspect, m_near, <its own float>)` into `Init`, so `Init`'s own four
stores ARE those fields, in that order.  `VICamera::Init` is the only caller of
`VIRaster::Frustum` and hands it
`(-t*n*aspect, t*n*aspect, t*n, -t*n, n, f)` with `t = tanf(fov * pi/180 * 0.5)`
— which is what makes the `aspect` argument the HORIZONTAL half-extent the QOL
widescreen patch scales, and it passes the y extents INVERTED.

`VIFlyCamera` is `VIWalkControl`'s underived little brother: the same
six-vector rigid body integrated by the same `powf`-damped step, with no
liquid, no gravity and no locomotion state, carried twice (live at +0x28,
committed at +0x88).  `this` IS `&m_facing`, which is why `Process`'s first
`HPR` call passes a0 unchanged.

`VIWalkCamera`'s own state starts at +0x1A0 and **the orbit body is a real
nested TYPE**:

```
struct State {                       // 0x50 bytes, alignment 8
    long         m_time;      +0x00
    unsigned int m_flags;     +0x08   1 snap-angle, 2/4 zoom, 8 snap-pitch
    VIVect3      m_orbit;     +0x0C   (angle, pitch, -)
    VIVect3      m_orbitVel;  +0x18
    VIVect3      m_orbitForce;+0x24
    float        m_dist;      +0x30
    float        m_distVel;   +0x34
    float        m_distForce; +0x38
    VIVect3      m_cameraPos; +0x3C
    float        m_snapAngle; +0x48
    float        m_snapPitch; +0x4C
};
State m_cur;  +0x1A0    m_angleAccel +0x1F0   m_distAccel +0x1F4
State m_last; +0x1F8    m_autoOrbit  +0x248 ... m_minCollisionDist +0x288
VICameraEffect m_effect +0x28C
```

`InitCameraIntermediate` copies the whole of it forward through a POINTER to
each half, so the +0 field folds back onto `this` (Phase 9) and every other one
comes off the pointer — a flat member list gives displacements off `this` for
all of them and cannot be made to do otherwise.  `camera-system.md`'s constant
table is confirmed (25.0 really is the zoom ceiling) but its field names are off
by the two-copy structure it did not see.

### An address NAMED before a call cannot be coalesced into the argument register

`VICamera::Update` ends with four calls taking `&m_view`, `&m_proj` and
`&m_world`.  Written naturally, gcc rematerialises `addiu a1,this,48` in the
call's delay slot; the target has `move a1,s1` with `addiu s1,this,48`
scheduled a hundred instructions earlier.  The lever is a local reference
declared at the TOP of the function:

```c
VIMatrix44 &view = m_view;
VIMatrix44 &wld  = m_world;
...
m_view.m[0].x = right.x;        // the STORES stay on the member
m_viewProj.Mul(view, m_proj);   // the CALL takes the reference
```

The reference's live range then crosses the intervening `operator=` call, so
the address goes to a callee-saved register, while every field store keeps its
displacement because it is written through the member name.  161 wrong words to
0.  Writing the stores through the reference as well is Phase 9's lever and is
WRONG here — it gives `0(s0)` / `4(s0)` for the fields.

### An inline helper's temporary is a NEW temp each expansion

`VICamera::CalcFrustum` runs the same eight-statement block over eight NDC cube
corners.  As an inline helper, the sret temporary of `v * invProj` is a fresh
temp per expansion: gcc gives them all the same STACK SLOT but a distinct
address pseudo, so `addiu a0,sp,16` is rematerialised eight times.  Written out
as eight braced blocks at function scope the eight `t`s share one pseudo and
the address is hoisted into a callee-saved register once — the target's shape,
and byte-exact.  Phase 19's `gcse` merge, with the roles the other way round.

### A store-to-load forward is not a re-read of the source

`VICamera::Update`'s three view translations read the COLUMN back out of
`m_view`, not the vector it came from:

```c
m_view.m[3].x = -pos.x * m_view.m[0].x + -pos.y * m_view.m[1].x +
                -pos.z * m_view.m[2].x;
```

A store and a load of the same slot forward for free, where a second read of
`right.x` has to be re-loaded because the stores above it may alias the
caller's vector.  Six instructions, and it is the whole gap.

### `VIMIN`/`VIMAX` are MACROS, so a call inside one is evaluated three times

`VIFlyCamera::Forward` clamps the magnitude of its input vector and then throws
the result away.  Written `VIMIN(1.0f, VIMAX(-1.0f, d.Magnitude()))` the call
appears THREE times in the expansion, and gcc emits three `jal`s plus a fourth
copy of the following `Normalize` cross-jumped into the clamp's arms; bound to
a local first there is one of each and both `Forward` and `Hover` go
byte-exact.

### A pointer variable must be declared where it is USED

`VIFlyCamera::UpdateCameraPhysics` needs Phase 9's pointer variable for the two
`v *= damp` runs, but one held from the top of the function is live across the
`fmodf` call in the middle and lands in a callee-saved register, where the
target rematerialises the address after it.  Declared inside a brace pair at
each site, both come out right.  The same function uses **ONE** local for both
`powf` results: reused, its live range crosses the second call, so both
definitions need `mov.s f21,f0` — two separate locals leave the first result in
`f0` and the function is two instructions short.

### The flag words are touched 32 bits at a time, unsigned

`VIWalkCamera::SetCameraMode` does `lw`/`ori`/`sw` on +0xF8 — a field Phase 18
proved is a `long` — and its clear mask is `lui 0xffff` + `ori 0xfeff`, ONE
constant shared by both flag words, where a signed `~0x100` would be a single
`addiu -257`.  So the site casts, and `VIWalkCamera::State::m_flags` is
`unsigned`.

### Phase 16 and Phase 21 disagree, and only one of them is source order

`VIWalkCamera::Init` stamps eighteen constants.  Phase 16's "the LAST store of
each distinct constant is hoisted to the front" predicts the store sequence
exactly from one source order; Phase 21's "the `lui`s come out in SOURCE order"
predicts the materialisation sequence exactly from a DIFFERENT one, and the two
cannot both be right (the first wants 11.75 before 0.5, the second wants 0.5
before 11.75).  The `lui` order is kept — 29 wrong words against 40 — and the
function is open.  **The 1.68 in that block is `1.68f` (0x3FD70A3D)**, not
1.67968750f: a bare `lui 0x3fd7` with no `ori` is the tell.

### Shipped oddities

* **`VIWalkCamera::UpdateCameraPhysics` damps the orbit PITCH by ZERO.**  The
  damping is a `VIVect3` whose x alone is ever written, and the angular
  velocity is multiplied by it component-wise, so `m_orbitVel.y` and `.z` are
  zeroed on every step.
* **`VIFlyCamera::Forward` and `::Hover` clamp a magnitude and drop it.**
* **`VIWalkCamera::OrbitIn` declares `OrbitRight`'s two stack vectors** —
  constructed, zeroed, never read.
* **`VIFlyCamera::Init` never initialises the committed velocity or force** and
  then copies them forward into the live body.
* **`SetOrbitAngle` calls `fmodf` twice on the same argument**, once per copy.
* **The zoom snap is asymmetric**: the `& 2` arm steps the distance and clears
  its flag without clamping; the `& 4` arm clamps to `m_minCollisionDist`.
* **`VIWalkCameraFilter` rejects every `VICSprite`** (sprite type 5) as well as
  the character's own actor, so the third-person camera swings through players
  and NPCs and stops only on world geometry.

### Open

`VIFlyCamera::PitchDown` 22/62 (the target does not fold `axis.y * rate` where
its own `TurnLeft` does — five spellings swept); `VIFlyCamera::Init(VICamera &,
VIScene &)` 26/45 (the standing phenomenon: same instructions, same length, the
target keeps six loads live where ours serialises them);
`VIFlyCamera::UpdateCameraPhysics` 20/192 and
`VIWalkCamera::UpdateCameraPhysics` 52/230, each ONE instruction — an address
materialisation and a `mov.s` respectively; `VIWalkCamera::Init` 29/82 above;
`VIWalkCameraFilter::FilterActor` 2/21, `v0` against `v1`.  Not attempted:
`CalcPick` (188) and `CalcPickFrustum` (343), both VU0, and `MoveCamera` (355).

### Harness

Two new `SPAN_CAP` entries.  `VIFlyCamera`'s last named function ends at
0x004422F8 and the next symbol is `VIFrustum`'s 2.5 KB later; `VIWalkCamera`'s
ends at 0x00483EB8 and the next is `__tf18VIWalkCameraFilter`, also 2.5 KB
later.  Both gaps are their TUs' unnamed statics, and without the caps
`FindBestOrbitAngle` reads as 804 instructions instead of 188.

## Phase 30 result: the FONT LAYER, and the enum Phase 16 read off the wrong thing

**1390/1527 → 1427/1567.**  `VIRFont` is **24 of 27** and `VIFont` **13 of 13**, on
the settled toolchain with **no re-tuning** — 30 of the 40 byte-exact on the
first build, including both 257-instruction `Cache` twins once one line was
right.

`VIRFont` owns two things at once.  It is the decoded font — every glyph's
pixels in a single `new unsigned char[]` block whose four sections (chars,
slots, pixels, palette) are byte OFFSETS until `Init` rebases all four onto the
block in one pass — and it is an **LRU cache** of the glyphs currently resident
in a 256x256 `VISurface`.  `VIFont` is the front end and is nothing but the text
colour: five of its seven entry points hash a name, look the font up in the
dictionary and forward to `VIRaster` with `this` as the `const VIColor32F &`.

### The layout

```
VIRFont : public VIBuffer            VIRFontChar          (12 bytes)
+0x08 m_locked                       +0x00 int   m_offset       into m_data
+0x0C m_surface   = -1               +0x04 ushort m_bytesPerRow
+0x10 m_material  = -1               +0x06 ushort m_code        BeginChar stamps
+0x14 m_charHeight                   +0x08 ushort m_slot        0xFFFF == absent
+0x18 m_maxWidth                     +0x0A uchar  m_width
+0x1C m_pixelStore        = I8
+0x20 m_palettePixelStore = RGBA32   VIRFontSlot          (56 bytes)
+0x24 m_numCharsLoaded               +0x00 int  m_offset   into the locked level
+0x28 m_numChars                     +0x04 int  m_next
+0x2C m_numSlots                     +0x08 int  m_prev
+0x30 m_maxSlots                     +0x0C ushort m_char   0xFFFF == free
+0x34 m_paletteCount                 +0x10 VIVect2I m_pos
+0x38 m_head = -1  +0x3C m_tail = -1 +0x18 VIVect2 m_uv[4]
+0x40 m_chars  +0x44 m_slots
+0x48 m_palette +0x4C m_pixels
+0x50 m_dataSize +0x54 m_data
```

The character table is sorted by code and `FindChar` is a binary search over
`m_numCharsLoaded`, which `EndChar` bumps once per glyph as the parser fills the
table in order.  `Cache` promotes each slot to the tail of the `m_head`/`m_tail`
chain and blits the glyph into the surface only on a miss, precomputing the four
corner UVs as it goes; the character INDEX, not the slot, is what it hands back.

### The correction: `VIPixelStore` was read off the case-block order

Phase 16 recorded the enum as "0 I4, 1 I8, 2 RGBA32, 3 RGB24, 5 RGBA16 — and 4
has no case at all".  That is the order of `CalcMipLevelStride`'s case BODIES,
which is source order and says nothing about the values.  The jump table itself
(0x004EA560, six words) says

```
0 -> w/2 rounded up   1 -> w   2 -> w*4   3 -> w*4   4 -> w*3   5 -> w*2
```

so **RGB24 is 4, not 3**, and 3 is a second four-byte store rather than the
default.  `VIRFont`'s two switches are the same shape with a **seventh** entry
(`sltiu 7`), value 6, also four bytes.  The error survived Phase 16 byte-exact
because **the table is data and the differ compares instructions**: a wrong
label set with the right number of blocks, the right block order and the right
bound produces exactly the same code.  Fixing it regressed nothing.

### The lever: `out[n]` is not `*p++`

`Cache`'s two twins are 257 instructions each and sat at 32 wrong for four
spellings of the output cursor.  `int *p = out; ... *p = c; p++;` coalesces `p`
with the parameter and loses the target's `move s8,s0`; the subscript
`out[n] = c` **lets the loop pass build the induction variable**, and it builds
it out of a COPY of the parameter, which is that `move`.  One line, 32 wrong → 0
in both twins.  The general form: where the loop already carries the count, do
not hand-write the walking pointer — `loop` makes a better one.

### The lever: the slot and the store order are separately controllable

Phase 13 recorded "stack slots go in DECLARATION order while the initialising
stores go in STATEMENT order".  `Cache` needs both halves at once: `n` at sp+16
with its store SECOND, `locked` at sp+20 with its store FIRST.  Declared
`int n = 0; int locked = 0;` the two move together and the prologue is two words
wrong; split — `int n; int locked = 0; ... n = 0;` — and it is byte-exact.

### The lever: the I4 rounding is a BRANCH

`n = w >> 1; if (w & 1) n++;` if-converts to `andi`+`srl`+`addu`, which is
shorter and is not what the target does.  Both `CalcDataSize` and `InitChars`
test the low bit and branch, so the source names the shift in BOTH arms:
`if (w & 1) n = (w >> 1) + 1; else n = w >> 1;` — the same spelling
`VISurface::CalcMipLevelStride` has carried since Phase 16.

### The lever: the binary search computes its exit test twice

`FindChar` ends `slt v1,hi,lo; li v0,-1; movz v0,mid,v1`, and the loop's own
back-edge test is a SECOND `slt` of the same two registers, sitting in the
branch's delay slot.  Folded into one condition — `while (lo <= hi &&
Char(mid)->m_code != c)` with `mid` computed before the loop — gcc CSEs them and
11 of 36 words are wrong.  Written with `mid` inside the loop and an explicit
`break`, the return block owns its own copy, reorg duplicates it into the delay
slot, and it is byte-exact.  A `break` and a folded condition are the same
program and not the same object code.

### `VIFont`: a mem-initialiser, and a static per entry point

The constructor is four `swc1`s of 1.0f and nothing else, so the colour is
initialised in the **mem-initialiser list**: written in the body, `VIColor32F`'s
own zeroing constructor lands in front of it and the function is twice the
length.  Every lookup stores the name hash into a static before using it —
`VIHashResourceID(sm_FontName)` on every call, cached nowhere — and each entry
point has a static of its OWN (0xF40, 0xF44, 0xF48 … four bytes apart in the
relocated dump).  Those stores are `lui`/`sw 0(reg)` pairs, i.e. relocation
sites the differ masks, so one file-scope static here reproduces all seven.
`sm_FontName` is **"ArialUI"**, read out of the relocated dump at 0x004ED0B8.

`SjisToJis` is the corpus's first function that is 64-bit end to end — `dsrl`,
`daddiu`, `dsll`, `daddu` — because `unsigned long` is eight bytes on this ABI
(Phase 23).  Its closing constant is **0x2021, not 0x2121**, because gcc 2.95
does not distribute a constant add across a left shift: the `+ 1` stays INSIDE
the shift and the `0x20 << 8` folds into the tail.  35 instructions, first
build.

### Shipped oddities

* **`VIRFont::Lock(VIRaster &)` never reads its raster.**  Phase 16's discarded
  reference again, and here the parameter exists only to match `Unlock`.
* **`Cache` reports the overflow after doing the damage.**  `if (m_numSlots < n)
  return -1;` is tested once the whole string has been cached, so a string
  longer than the atlas has already evicted its own earlier glyphs — and the
  slot indices it wrote into the caller's array are the stale ones.
* **`m_maxSlots` and `m_numSlots` are computed by different formulas.**
  `CreateResources` sizes the slot TABLE as `(256/cell) * (256/height)` while
  `InitSlots` counts what actually fits as `(256/cell) * (256/cell)`, with the
  cell rounded up to even for I4.  The allocation is therefore never short, but
  it over-allocates whenever the widest glyph is wider than the line is tall.
* **`InitSlots` walks its rows by `stride` and its columns by `rowBytes`**, and
  `rowBytes` is derived from the LOCKED level's pixel store, not from
  `m_pixelStore` — the two agree today only because `CreateResources` copies one
  into the other.

### The three open functions

`BeginChar` 7/14 — the same instructions in different registers: the target
parks the character's address in `t0` and ours in `v1`.  Five spellings swept
(reference, index local, statement order, a local for `m_data`); none moves it.

`InitSlots` 27/132 — one word, and everything after it is branch offsets.  The
target writes `m_pos.y` and `m_pos.x` through an explicit `addiu v0,v0,16` and
ours folds the 16 into the two displacements.  Eight spellings swept (pointer,
reference, `char *` cast, integer expression, two-statement, slot local, inline
`Set` on the member, inline `SetPos` on the slot) and gcc folds every one; a
probe TU reproduces the target's shape only when the pointer has **two reaching
definitions**, which no plausible source here has.

`CreateResources` 25/127 — one word, and the rest is the shift it causes.  The
target fills the palette guard's delay slot with a hoisted `li a0,1` — the
`VIPixelStoreI8` comparand, materialised before a branch that does not use it —
which leaves `m_paletteCount = 0` as a block of its own after the arms.  Ours
has nothing to hoist, so it annuls instead (`beqzl`, the store in the slot) and
materialises the 1 inside the second test: 126 words against the target's 127.
Nine spellings swept.  Inverting the guard — the zero case as the `if` — gets
the length right and scores 13, but it puts the zero block FIRST where the
target puts it last, so the lower score is measuring alignment and not fidelity;
the `if (paletteCount)` form is kept.

## Phase 35 result: the last sprite leaf, and four more classes

**1624/1786 -> 1671/1834.**  Five classes on the settled toolchain with no
re-tuning, 45 of the 47 written byte-exact and 33 of them exact on the FIRST
build — plus **two functions that had been open since phase 20 and phase 32,
closed by one line each**:

| class | written | byte-exact | not attempted |
|---|---|---|---|
| `vipointlight.cpp` — the ninth and last sprite leaf | 11 | **11** | — |
| `vicspritecust.cpp` — the customisation tables | 9 | **9** | `SetResources` (1009 insns) |
| `vispelleffectdriver.cpp` — one cast in flight | 14 | **14** | `ParticleCount` (650) |
| `viradialflorasystem.cpp` — the frame round the flora grid | 7 | **7** | six, incl. `CreateFloraInsts` (552) |
| `viicondefinition.cpp` — the memory card's `icon.sys` | 6 | 4 | — |

**Phase 7's if-direction, fifteen phases late.**  `VIPointSprite::Raster` has
been wrong by exactly six words since phase 20, and phase 32 found
`VIStreamAudioSprite::Raster` wrong in the *same* six words and filed it under
the standing register-allocation phenomenon.  It is not that.  The source is the
nested form — `if (sc.GetPointUISprite() != -1) { … }  return 0;` — where all
three were written with an early `== -1` return.  One line each, byte-exact.
The tell that it was worth re-testing: `Pick`, three functions down in the same
class, is byte-exact with the `!= -1` test and its two halves are otherwise the
same shape.  Phase 20's "the if-direction is per FUNCTION" is right, and the
corollary is that a neighbour matching with one direction is a reason to try the
other, not a reason not to.

**`VIPointLight` IS `VIPointSprite`.**  Five of its functions are byte-identical
to `VIPointSprite`'s *in the target image* — `Raster`, `Pick`, `Collide`,
`Release` and `ClampScale`, word for word — which is also this phase's clearest
statement of what an un-relocated image cannot say: the two classes forward to
DIFFERENT scene sprites and the call sites are the same instruction, because
`jal` is zeroed.  `eqcalls.py` reads `GetLightUISprite` out of the relocated
dump where the other reads `GetPointUISprite`.  `_vt$12VIPointLight` is 96 bytes
of vtable, so phase 19's ten virtuals hold a ninth time; and this leaf overrides
only ONE `Collide` (the swept sphere) where every other leaf overrides both.

**The radius is read THROUGH the sub-object pointer** (14 of 46 words):
`m_bsphere.r` comes off the pointer the `VIVect3` copy already materialised for
`m_bsphere.c`, not off `this`.  Phase 9's lever, with the address pseudo created
by `operator=`'s reference parameter — which is why `VIPointSprite::Init` needs
no pointer at all: there `m_bsphere.c.Zero()` is an inline member whose `this`
(phase 32) makes the same pseudo, earlier.

**A magic word is an IMMEDIATE, not a `memcpy`.**  `VIIconDefinition`'s
133-instruction constructor went to byte-exact on one line: `'PS2D'` is
`lui 0x4432`+`ori 0x5350`+`sw`, where `memcpy(m_head, "PS2D", 4)` loads it out
of `.rodata`.  So the header field is a WORD.

**An uninitialised local is a source fact.**  `Ascii2Sjis`'s `run` is not
initialised at its declaration — the digits arm assigns `run = 0` explicitly,
and hoisting that to the declaration lets gcc delete the arm's whole `b`/`move`
pair (30 wrong words against 13).

**Phase 16's constant hoist reads a statement order out of the object code, and
twice in this phase it is NOT the field order.**  `VISpellEffectDriver::Destroy`
resets the same six fields its constructor does, in a different order: all 720
orderings were swept and exactly one is byte-exact.  `VIRadialFloraSystem`'s
constructor is 7 of 21 wrong written in field order; of the six orderings of
its three groups, three are byte-identical and what they agree on is that the
three pointers do not come FIRST -- which is where the field list puts them.

Two corrections to `claude-notes/vicspritecust-tables.md`, both from the index
arithmetic (which is all an un-relocated image says about a static array):
**`$Tints` is FIFTEEN entries, not fourteen** (the static initialiser's loop is
`li v1,14` counting down through zero) and **`$Race`'s rows are `[4][2]`**.
`_GLOBAL_$I$_13VICSpriteCust$Tints` is phase 32's rule a second time: the only
static member with a constructor is `Tints`, and its whole initialiser is
`VIColor32`'s opaque-black default constructor, fifteen times.

Shipped: **`GetArmorSetTexture` ignores its race** (`li a0,20` in the guard's
delay slot destroys it), `VIRadialFloraSystem::Reset` returns **void** among
`int`s, `VISpellEffectDriver::Trigger` refuses while the driver list is
non-empty, and **two of the three lights in a card icon are BLACK** — only light
1 and the ambient carry the 0.65 grey.

Two new `SPAN_CAP` entries, both the familiar cause (a TU's last named symbol
followed by unnamed twins): `ClampScale__12VIPointLight` at 492, and
`GetTintColor__13VICSpriteCust13VICSpriteTint` at 20, where the tail is gcc's
own static-initialisation helper for `$Tints`.

### Open

Two, both in `VIIconDefinition` and both the standing register-allocation
phenomenon.  `Ascii2Sjis` 13/59 — the two tail blocks have the right
instructions in the right order with three registers exchanged; six spellings of
the punctuation index give byte-identical output, so the expression is refuted
as the cause.  `SetTitle` 9/76 — the newline scan puts the loop index in `a0`
and the hoisted `'\n'` in `a1` where the target has them the other way round;
five loop shapes and all twenty-four declaration orders of the four locals give
exactly 9/76.

Long form:
`claude-notes/decomp-phase35-the-last-sprite-leaf-and-four-more-classes-20260826.md`.

## Phase 36 result: the device classes

**1671/1834 -> 1699/1864.**  Three classes on the settled toolchain with no
re-tuning, 28 of the 30 written byte-exact: the pad, the keyboard and the
combat-music layer -- the three places where the engine talks to hardware it
does not own.

| class | written | byte-exact | not attempted |
|---|---|---|---|
| `vipad.cpp` — one controller port, plus the four flag accessors | 14 | 13 | — |
| `vikeyboard.cpp` — the USB keyboard, plus the two driver entry points | 8 | **8** | `Read` (286), `CombineDeadkey` (174), `PrepareQueueKeyEvent` (150), `TranslateEvent` (82) |
| `vibattlemusic.cpp` — combat music | 8 | 7 | `Unload` (660), `Process` (106) |

**An `ld` is not proof of a 64-bit member.**  Three `VIPad` methods load eight
bytes at +0x20 and compare them against 2, while `Read` reads the same offset
with `lw` and `Init` zeroes +0x20 AND +0x24 with two separate `sw zero`.
Declared as a `long`, the three match and `Init` does not; declared as two
`int`s, the reverse.  Both readings are right: **gcc 2.95 merges two adjacent
`int` compares into one 64-bit load**, because `m_state == 2 &&
m_actAligned == 0` IS "the eight bytes at +0x20 equal 2" on little-endian.
Four spellings of the predicate — `&&` either way round, `!m_actAligned`, and an
explicit `*(const long *)&m_state` cast — give byte-identical output, so the
merge is the compiler's and not the source's.  The rule generalises: **an access
WIDER than the field it lands on says something was merged, not that the field
is wide.**

**An inline predicate materialises the comparison.**  The same five instructions
carry a second finding: the target computes the comparison into a register
(`sltiu`, then `dsll32`/`dsra32` to narrow the 64-bit result to `int`) and
branches on the value, where the test written out at each call site gives a
direct `ld`+`li`+`bne`.  That is what an inline member returning `int` produces,
and three call sites share the shape.

**Phase 7's if-direction, five more times, and once it decided a 290-instruction
function's whole layout.**  In `VIPad::Read` the ready path is the `if` BODY and
the not-ready teardown is the function's TAIL — written as an early
`if (st != 2 && st != 6)` with an early return, gcc leaves the teardown where
the test is and 86 of 290 words move; then, inside it, the unpack is the `if`
body and the SetMainMode arm the fall-through, which is what puts the three
trailing blocks in the target's order.  Also `ScaleStick` (11 of 27),
`VIKeyboard::DequeueKeyEvent` (27 of 29) and `VIBattleMusic::Stop`.

Three more shapes: **phase 23's `||`** (two `if (X) return 0;` statements branch
AROUND the first test and cost a `b` to the shared epilogue; one disjunction
sends both straight there — 17 wrong words to 6, and the length goes from 291
back to 290); **a nested ternary, not two returns** (`Preload` closes on
`move v0,a1`, which two `return`s cannot produce — 21 of 62); and **the guard
before the assignment** (`Play` reads `if (m_track == -1) return -1;
track = m_track;` and gcc steals the assignment into the guard's delay slot,
where assigning first and re-testing costs a reload and 14 of 90).

Layouts.  **`VIPad` is 96 bytes and the last field closes it exactly** —
phase 27 measured 96 off `VIConsole`'s offsets and left it opaque; `scePadRead`
is handed `this + 60`, the buffer is 32 bytes, and 0x3C + 32 = 0x5C, which
rounds to 96 on the eight-alignment the three `long`s force.  **`VIConsoleKey`
is TWO bytes, not sixteen** (phase 27 guessed 16 from a frame; the keyboard's
queue is indexed by `sll a1,a1,1` and copied with two `lb`/`sb` pairs, and
`viconsole.cpp` still matches after the correction).  **`VIScene + 0x4BA4` is
the `VISoundDevice *`**, filling the last unnamed word between the raster and
the collide.

Shipped: **`VIPad::Read` initialises a 32-byte local it never reads** — four
`ldl`/`ldr` pairs copy a constant off `.rodata` into the frame at every call,
and the actuator alignment it actually sends is a file static; gcc 2.95 does not
delete a dead initialised local array, so our own build reproduces all sixteen
instructions.  Every button is the COMPLEMENT of one bit, and gcc rewrites
`(~b >> 7) & 1` as `(b ^ 0x80) >> 7` for the top bit ONLY.  The stick dead zone
is 88..167 and the scale is the reciprocal 1/88 written out.  And
**`VIBattleMusic` has two entirely different back ends behind feature bit
0x100** — a streamed `bgm\battle%02d.bgm` in which **track 9 means "pick one of
eight at random"**, or an XM parsed out of `bgm\battle%d.esf` and marked
repeating; only the second uses the sound device's channel at all.

### Open

Two.  `VIPad::Read` at 6/290 — every instruction is right and one is in the
wrong place: the `move v0,zero` that materialises the teardown's return value
lands four instructions early and shifts five others.  Every ordering of the
tail block that keeps the actuator pair adjacent scores exactly 6, so the
statement order is refuted as the cause.  `VIBattleMusic::Free` at 2/17 — the
`-1` constant is in `v1` where the target has `v0`; all six orderings of the
three stores and both `int`/`void` return types score exactly 2.

Long form: `claude-notes/decomp-phase36-the-device-classes-20260826.md`.

## Phase 38 result: the spell event driver

**1764/1930 -> 1792/1965.**  One class in one translation unit,
`VISpellEventDriver` (SUPPORT `0x00475E68 .. 0x0047A978`), on the settled
toolchain with no re-tuning: **28 of its 35 functions byte-exact, 20 of them on
the first build.**  Phase 37 built everything underneath it and
`claude-notes/spell-event-driver.md` had already read the semantics off Ghidra,
so every miss is codegen.

| written | byte-exact | not attempted |
|---:|---:|---|
| 31 | 28 | `ProcessProjectileEvent` (1187), `NewPlayTargetProjectile` (609), `ProcessCreateEmitter` (420), `ProcessLightningEvent` (399) |

**The destructor says what a pointer POINTS AT.**  The constructor stores every
field and so gives the layout; the destructor gives the types.  `eqcalls.py`
reads the two deletes at +0x28 and +0x2C as `_$_11VILightning`, not
`_$_18VISpellActiveEvent`, so the two words `spell-event-driver.md` calls
"reserved" are the bolt pair the lightning event draws with — and +0x30, which
IS a `VISpellActiveEvent`, is not freed at all.  The class is 100 bytes;
`m_anim` is an ARRAY of four, said by `ProcessWaitEvent` indexing it with
`sll 2`; `VISpellActiveEvent + 0xB0` is a 64-bit start time (one `sd`), which is
what forces that class's alignment to 8; and `VIHSprite` reaches at least
+0x188.

**A bit test that becomes a 0/1 VALUE is `int x = 0; if (bits & m) x = 1;`.**
Every predicate spelling — `(bits & m) != 0`, `!!(...)`, the ternary, a `bool`
initialised from the mask, an inline accessor returning the comparison — gives
`sra`+`andi 1` and never materialises the mask; only the
assign-into-a-zero-initialised-local form (or its `if`/`else` twin) gives the
target's `lui`+`and`+`movz reg,zero`.  Phase 18's rule with both halves in one
function, worth 21 of `NewShakeCamera`'s 73 words.

**ONE variable, not a separate `t`.**  Two interpolators sat two words short on
the destination register of a single `div.s`.  `float t = a / b; float f = ... * t;`
puts the quotient in the dividend's register; `float f = a / b; f = ... * f;`
puts it in the register the result ends up in, which is the target.  Six other
spellings are all two words short; this one closed both functions.

**The two guards of a function need not be the same shape.**  `NewShakeCamera`'s
actor test is an early return and its camera-effect test an `if` whose body runs
to the bottom; with both nested, or both early, the join's `move v0,zero` and
`lq ra` come out the other way round.

**A switch whose cases all retire the same object shares nothing after it.**
`ProcessWaitEvent`'s five cases each write `delete m_wait; m_wait = 0;` out and
gcc cross-joins them; the switch's own out-of-range `sltiu` test then targets the
bare `return 0` that the retire falls THROUGH into.  A shared tail after the
switch plus a `default: return 0;` puts a second copy of that return early and
misdirects every branch: 5 wrong words against 69.

**The completion check is NESTED.**  Five conditions that all mean "not
finished": as a flat chain of `if (x) return 1;` the last one if-converts to
`sltu`, nested it branches (phase 17's `VIXm::IsCached`).  47 wrong words to 0.

**Reading a field TWICE is the else arm** (`int sprite;` with
`else sprite = ev->m_id;` reads the id into the branch-likely's delay slot,
where an initialiser reads it once into a register — 30 words); **a `bool`
argument is compared against 1** (`active == true`, not `active`); **the jump
table gives the case ORDER as well as the labels** (phase 34, twice — the
dispatch switch runs 1, 2, 3, 6, 5, 4, 7, 8, 11, 12, 9, 10, 13..17); **a float
zero written to two adjacent fields is a chain**, and here the stores come out
LEFT to right where phase 7's came out right to left; **the success path in the
`if`**, three more times.

Shipped: `ProcessCreateSound` builds a `VIMatrix33` nothing reads;
`NewCreateSprite` calls `GetActorSpritePtr` and drops the result;
`NewPlayTargetLightning` declares three zeroed `VIVect3` it never uses;
`NewSetAction` and `NewPlayAttack` return **-1** when the actor is not a
`VICSprite`; and `NewSetEmitterMotif` takes an in-flight blend OVER rather than
restarting it.

### Open

Four, none structural.  `ProcessCreateSprite` 38/133 and `ProcessCreateSound`
34/148 are the same twins: instruction for instruction right, but the target
rematerialises the constant -1 three times where ours keeps it in one register,
and the one-word shift charges the rest; seven spellings swept.
`ProcessWaitEvent` 5/134 is four words of `a2` against `a0` in sub-type 0, plus
one masked jump-table relocation the aligned counter still charges for.

Long form: `claude-notes/decomp-phase38-the-spell-event-driver-20260826.md`.

## Phase 39 result: the sound device

**1792/1965 -> 1841/2029.**  One class in one translation unit,
`VISoundDevice` (SUPPORT `0x0049A3E0 .. 0x0049D288`), sixty symbols, on the
settled toolchain with no re-tuning: **50 byte-exact, 40 of them on the first
build.**  The class is the EE half of the sound system — 46 playback records,
128 loaded `VISound`s, two first-fit allocators, one SIF DMA in flight and the
BGM cross-fade — and it makes no `sceSpu` call at all: every operation is
appended to a packet by a file static and shipped over SIF RPC.

| written | byte-exact | open |
|---:|---:|---|
| 60 | 50 | `Process` 65/190, `Play` 33/250, `StreamBgm` 28/118, `SetPitch` 22/89, `AllocateChannels` 17/103, `DebugResources` 15/77, `AllocSpuMem` 13/47, `BeginXmTransfer` 12/13, `IdleTransfer` 7/117, `BeginAdpcmTransfer` 5/13 |

**A new `SPAN_CAP`, and the canary picked the number.**  The TU closes with five
unnamed SIF RPC statics, so `DebugResources` — the last named symbol — acquires a
936-byte span.  The real function is **308**: `test_span_caps_only_ever_shrink`
refuses 312, because a cap must land exactly one word past a `jr ra`, i.e. take
the delay slot and not the assembler's padding nop.

**`BgmBufferSize` is NOT static and `SetBgmVersion` is.**  Both read the same
file-scope static and both mangle without a `this`, so the name cannot say which;
the CALL does.  `Init`/`Reset` pass `this` in `a0` to `BgmBufferSize`, and
`SetBgmVersion` stores `a0`, its first argument.  Getting it wrong made `Init`
come out one instruction SHORT with every other word identical.

**A float stored into an `int` is `cvt.w.s`+`mfc1`; into an `unsigned int` it is
a CALL to `fptoui`.**  Every volume word this class sends is unsigned and every
pan word signed — four functions, eight sites.

**The stereo gate is an `if` on the LOCAL, and both directions appear.**
`if (m_stereo == 0) pan = 0; gSndArgs[3] = pan;` gives `movz`;
`int p = 0; if (m_stereo) p = pan; gSndArgs[6] = p;` gives `movn`.  Two STORES in
an `if`/`else`, or a ternary, give a branch.  Phase 27's rule applied to the
destination rather than the test; it closed `SetVolume` and `SetPan` outright.

**The clamp is `VIMIN(VIMAX(lo, v), hi)`.**  `VIMIN(hi, VIMAX(lo, v))` emits the
same six instructions in the same order and only reverses the first `c.lt.s`'s
operands — Phase 23's VIMIN rule, one macro deeper.

**The `-1` sentinel is a signed `int`**: `li reg,-1` against `lui 0xffff`+`ori`
for an `unsigned int` field, because gcc materialises a 32-bit unsigned constant
as a zero-extended 64-bit value on this ABI.

**The same expression is 32-bit in one function and 64-bit in another** —
`(rate << 12) / 48000` is `sll`+`div` in `SetPitch` and `dsll`+`__divdi3` in
`Play`.

**The type test is two `if`s in four functions and an `if`/`else if` in three**,
per function, measured: `Process`, `SetPitch`, `SetVolume` and `SetPan` call
`Type()` a second time after the WAVE arm.  In `SetPitch` it decides the frame
size, because `else if` lets the `pitch` parameter die in the WAVE arm.

**A shared address base is what puts a search on one walking pointer**, and the
loop has to be a `do`/`while`: two subscripts give two induction variables, and a
`for` over a constant bound peels its first iteration onto constant
displacements.

**`Reset` is not `Init` with a prefix** — the same eighteen-statement field block
appears three times with three different subsets, and Phase 16's hoist reads the
subset back: the hoisted `1` is `m_stereo`'s in `Init` and `m_xferIdle`'s in the
constructor, so the constructor's block cannot contain an `m_stereo` store.

Layouts: `VIPlayback` **56** deriving from `VILNode`, `VISpuBlock` **24**,
`VISoundDevice` at least **0x117C** with four 256-byte BGM name buffers;
`sizeof(VIWave)` **92** and `sizeof(VIXm)` **64** straight off `CreateSound`'s
two `__builtin_new` arguments, the first independent confirmation of Phase 17's
layouts.  **The playback handle is packed exactly as `VISpellEffectSystem`'s**,
serial seeded at **0x16062**, `(handle & 0xff) % 46` and not a cast.

Shipped: **`EnableBgmFading` is empty**; **`SetBgmVolume` ignores its slot**, so
`SetSoundClassVolume`'s closing pair of calls is two self-assignments;
**`IsBgmPlaying` ignores its slot**; **`Stop` returns nothing**; the XM allocator
floors every request at `0xC000` when `sBgmVersion == 1` and `FreeXmMem` applies
the identical floor; and `Process` starts ONE transfer per frame while
restarting its 128-entry scan from zero, so a sound low in the table waits for
every sound above it.

Long form: `claude-notes/decomp-phase39-the-sound-device-20260826.md`.


## Phase 40 result: the GS VRAM allocator

**1841/2029 -> 1857/2052.**  One class in one translation unit,
`VIEESurfCache` (SUPPORT `0x0040E780 .. 0x00410290`), twenty-one symbols on
the settled toolchain with no re-tuning: **15 byte-exact, 12 of them on the
first build**, plus the two container pieces it needed —
`VIMultiMap<unsigned,int>::Key` and `::LowerBound` — and two destructors that
had never been instantiated.

The class is `VIRaster + 0x3298`: it owns one span of GS video memory, hands
out runs of it, and builds the VIF/GIF chain that fills them.  Three
containers over that span — `VIPool<VIEEBucket>` in address order, a
`VIMap<int,VIEESurfCacheInfo>` from surface to residency, and a
`VIMultiMap<unsigned,int>` free list — plus an LRU threaded through the infos
so a miss can evict.

**The free map's key is the whole design.**  A bucket carries the GS PIXEL
STORAGE MODE it is currently formatted for (0 CT32, 1 CT24, 2 CT16, 19 T8,
20 T4) or **255, meaning virgin** — never handed out, and so reformattable.
The key is `(psm << 24) | size`, so `FindFree` does a `LowerBound` on
`(psm, size)` to find the smallest run already in the right format and falls
back to `(255, size)` for untouched memory.  A GS page is 8192 bytes and its
pixel layout depends on the psm, so a run may only change format on page
boundaries — which is where every split, merge and alignment in `Allocate` and
`Deallocate` comes from.

Layouts: `VIEEBucket` 16 (`addr`, `size`, `psm`, free-map handle),
`VIEESurfAlloc` **12**, `VIEESurfCacheInfo` 84 (two allocs, the CLUT address,
ten level addresses, the two LRU links and a stamp), `_sceDmaTag` 16.

Three corrections to standing docs.  **`VIMaterialLayer` is really
`VIMaterialPass`** — `Set`'s third parameter mangles `R14VIMaterialPass` and
its stores land on exactly the class phase 15 modelled — and its +0x48, filed
as "m_pass", is the **cache stamp**: `Cache` compares it against the resident
info's own +0x50 and skips the whole upload when they agree.  **`VISurface +
0x18` is the cache index and +0x14 the surface's own pool slot**, which is the
info map's key.  And **`VIPixelStore` 3 and 4 are both correct at once**: 3 is
24-bit colour in a 32-bit slot (PSMCT24, four-byte stride) and 4 is the packed
three-byte form, which `CalcCacheInfo` rejects outright — so phase 30's table
and the pre-phase-30 enum were each half right.

Levers.  **Which of two adjacent stores ends the block decides whether
cross-jumping eats it**: `MoveToLRUEnd`'s two arms both end with
`m_lruLast = i`, and only ordering the empty-list arm `m_lruFirst = i;
m_lruLast = i;` — so the delay-slot filler steals the last store and leaves a
different tail — keeps them apart.  **The align-up idiom is an if/ELSE with a
fresh intermediate** (`int t = p + 8192; a = t - (p & 8191);` in the `if`,
`a = p;` in the `else`), five sites, 27 wrong words to 6 in `Clear` alone.
**A SECOND LOCAL for the same index**, three times: an index read once and
used both as a call argument and as a container subscript needs `int j = i;`
to reproduce the target's spare `move` (`Decache` 23 wrong words to 0).
**Two assignments per arm is what blocks if-conversion** — `Set`'s
magnification filter branches and loads the same `-127.0f` in each arm because
each arm sets two things.  Plus phase 33's `VIMAX(x, k)` emitting `li k-1`
five more times, and phase 23's nested guards twice.

Shipped: **`FindLRU` returns 1 on success** (it returns `FindFree`'s own
answer), **`Cache` returns 1 when it uploaded and 0 when the surface was
already resident**, **`DebugVerify` is `return 0;`**, **`Set` writes MIPTBP2
from levels 1..3 for every non-indexed texture** instead of 4..6, its TEX1 LOD
constant lands in bits 0..7 on top of MMAG and MMIN, and `CalcCacheInfo`
ignores `this`.  Two new `SPAN_CAP` entries: this TU emits its own local
copies of `VIMultiMap<unsigned,int>`'s walkers, so `DebugVerify` (8) and
`_$_t10VIMultiMap2ZUiZi` (100) both acquire spans reaching into them.

Open: `Set` 78/204, `CalcCacheInfo` 23/192, `Cache` 7/179 and `FindFree` 7/57
are each one instruction of placement; `Allocate` 99/135 and `Deallocate`
186/289 are instruction-for-instruction identical and differ only by one
callee-saved register.  `ConstructDMAPacket` 160/228.  **And one standing
question**: `int f = 1; if (count == 0) f = 0;` comes out as a register copy
plus `movz f,zero,count` in the target and as a single `sltu` in ours, in both
functions that use it — nine spellings swept, none of them reproduces it, and
it is the whole remaining gap in `Set`.

Long form: `claude-notes/decomp-phase40-the-vram-allocator-20260827.md`.

## Phase 55 result: the sky and the weather

`src/viatmosphere.cpp` is **30 of `VIAtmosphere`'s 32 functions written and 26
byte-exact**, plus `WeatherFilter::FilterActor`, and the tree moved
2,653/3,020 -> 2,679/3,049.  `VIAtmosphere` is SUPPORT 0x00416880 ..
0x0041C7F8, its own TU, and `VIScene` embeds one at +0x640: the sky dome and its
three rotation matrices, 128 stars and a shooting star, the lightning flash and
its 32 patches, the rain/snow particle field with its splash pool, and the
Lavastorm fireballs, in 4,528 bytes.

### Structures

The constructor is a member roll-call and the destructor is that roll-call in
reverse, so the layout is READ and not guessed: `VILList` at +0x14, +0x24,
+0x1160 and +0x1170; `VIMatrix44` at +0x500, +0x540 and +0x580; and the zeroing
loops give `VIVect3[2]` at +0x50 and +0x68, `VIVect3[64]` at +0xA8,
`VIVect4[128]` at +0x620 and `VIVect4[32]` at +0xF60.  The class is
`aligned(16)` through `VIVect4`, which is why the last field at +0x11A8 rounds
the size to 0x11B0.  **Both the 143-instruction constructor and the destructor
matched on the first build**, which is what proves it.

Three nested classes come out of the same reading.  **`VIWeatherParticle`** is
36 bytes with a `VIColor32F` at +0x0C -- the sixteen bytes `RenderWeather`
block-copies into the billboard -- and is NOT a `VILNode`, because `Init`
allocates the field as one `new char[36 * n]` and `Clear` deletes it with no
array-new cookie.  **`VISplash`** is a `VILNode` plus a location, a brightness
and a 0..4 frame counter.  **`VIFireball`** is a `VILNode`, 48 bytes of its own
and then TWO `VIParticleEmitter`s, at +0x40 and +0x420; `Clear`'s `delete []`
proves it (the 16-byte cookie, a backwards loop with a 2048-byte stride, both
emitter destructors in reverse declaration order) and fixes
`sizeof(VIParticleEmitter)` at 992.

`claude-notes/weather-system-decompiled.md`'s Ghidra field map is corrected in
four places: +0xA8 is `VIVect3[64]` and not three `float[200]` tables, +0xF60 is
`VIVect4[32]` and not `float[14*4]`, +0x4E4 is a two-entry array
(`"sky"`, `"EQOA_Skydome_360"`) that `SetPlaneOfSky` indexes, and
`ResetLightning` CLAMPS to [0.1, 0.8] where the note read a 10% skip.

### Levers

**A function-scope local and a block-scope local get different FP registers**,
and both directions are load-bearing in one phase.  `RenderWeather`'s
`sz = m_p88[i] * m_particleScale`, used four times inside a loop, is byte-exact
only when declared at FUNCTION scope.  `RenderStars` needs the opposite: `b` and
the two loop counters declared inside EACH of its two `if` blocks -- separate
variables, not one pair shared -- is what forces `idx` into a stack slot and
puts the second loop's counter in `s7`, 141 wrong words to 48.

**`VIMIN(VIMAX(k, x), K)` threads the constant arm.**  Both clamps in this TU
come out as THREE compares: gcc proves `K < k` on the arm where the inner max
returned the constant and jumps to the join, then re-tests the inner condition
on the other arm.  The argument ORDER is what the branch directions read back --
`VIMAX(0.1f, x)` gives `c.lt.s x, 0.1` where `VIMAX(x, 0.1f)` gives the operands
the other way round.

**Name the clamp result, then multiply in a SECOND statement.**  As one
expression gcc distributes the following multiply into all three arms; as
`c = clamp; b = c * (...); b = b * 0.8f;` it is byte-exact, and the split is
needed because the first multiply's destination has to be the RESULT's register.

**A `VIVect4` four-float constructor hoists its constants.**
`RecomputeLightningPatches` builds `(0, 100, 0, 1)` inside a 14-iteration loop;
four member assignments rebuild the two non-zero constants every iteration and a
constructor call hoists them into callee-saved FP registers.  `VIVect4` earned a
four-float ctor for it, the way `VIVect3` earned one in phase 9.

**A dead local is a source fact.**  `RenderWeather` constructs a `VIBBox` it
never reads -- 24 bytes zeroed TWICE, which is that class's double-zeroing
constructor and nothing else in the corpus has that shape at that size.

**A subscript with a ternary index scales the TERNARY.**  A named
`int i = k ? 1 : 0;` gives `sltu` + `sll`; the ternary written out at each
subscript gives `li 4` + `movz`, the byte offset selected directly.

**One statement's position decides a loop's whole allocation**: `RenderSplashes`
closed on moving `num = m_numSplashes;` above the `SetMaterial` call, and
`RenderSky` on moving `r = sc.m_raster;` above the `sceVu0CopyMatrix`.

### Harness

**Grep for the class before declaring it**, for the third time: `WeatherFilter`
was already in `vi.h` (phase 54) AND defined in `viactorfilter.cpp`, and
re-declaring it here failed the whole TU.  The definition belongs in this TU --
`FilterActor__13WeatherFilter` is at 0x00416880, the word before `VIAtmosphere`'s
constructor, and `__tf13WeatherFilter` at 0x0041C7A8, the word after
`ProcessLavastorm`.  `viactorfilter.cpp` now carries a comment saying so.

Open: `Update` 4/128 (two `sd`s of the frame clock schedule ahead of a
`VIVect3` construction; a permutation sweep finds nothing below 4),
`RenderStars` 15/464, and `ProcessWeather` 278/626 written as a DRAFT --
the shape is right, the expression forms are not.  **`Init` (705 instructions),
`ProcessTransitions` (858) and `ProcessLavastorm` (976) are NOT WRITTEN**; the
phase ran out before the three biggest functions in the class.

Long form: `claude-notes/decomp-phase55-the-sky-and-the-weather-20260828.md`.

## Phase 54 result: the scene

`src/viscene.cpp` is **125 of `VIScene`'s 140 functions written and 114
byte-exact**, and the tree moved 2,538/2,895 -> 2,653/3,020.  `VIScene` is the
object the whole engine hangs off: it EMBEDS `VIWorld`, `VIClipStack`,
`VIParticleSystem`, `VIAtmosphere`, `VIQueryTriList`, `VISceneSoundTrack`,
`VISpellEffectSystem` and `VIRadialFloraSystem` by value and is **19,372 bytes**,
four times anything else in the corpus.  An "actor" everywhere in it is a
`VIActor *` carried in an `int` and dereferenced with no bounds check, the same
convention `VIZone`'s room lists use.

The phase opened by **repairing the tree**.  A previous sitting had correctly
retyped `VIScene::m_world` from `VIWorld *` to an embedded `VIWorld` and
`CreateSprite`'s first parameter from `int` to `VISpriteType`, and left thirteen
translation units not compiling — which `eqcheck.py` reports as a 131-symbol
regression list spanning every sprite leaf.  Both changes are right; the callers
were not updated.  This is phase 41's rule (a failed TU reads as a mass
regression) in its most expensive form yet, and the fix was a `(VISpriteType)`
cast at thirteen call sites plus one member spelling in
`viradialflorasystem.cpp`.

### Structures

* **`VISceneIntersectVars` was wrong in five of its eight fields.**
  `QueryIntersection` BUILDS one, so every offset is visible: the caller's box is
  copied to **+0x04**, the pointers go in at **+0x1C `m_scene`, +0x20
  `m_filter`, +0x24 `m_collide`, +0x28 `m_tris`**, `VIBBox::Store` fills eight
  corners at +0x2C and `VISphere::Init(bbox)` the sphere at +0x8C.
  `VIZone::QueryIntersectionRoom` confirms all four pointers independently.
* **`VISceneInsertVars`'s +0x70 is a `VISphere`.**  `UpdateActor` zeroes four
  words there twice over, which is that class's double-zeroing constructor; a
  `VIVect3` plus a bare `float` leaves the radius alone.
* **`VISceneStreamVars::m_p14` is a `VICollide *`**, and **+0x10 is never
  written by anything.**
* **`VISceneStreamRadialVars::m_p2C` and `::m_p30` are FLOATS.**  Both were
  `int`; the cache primers store stream radii into them with `swc1` and
  `VIZone::StreamLeaf` squares each against a float distance.  The retype took
  `VIZone::StreamLeaf` from 29 wrong words to 23 with no source change.
* **`VIActor::m_p70` is a `VIColor32F`**, the baked static-lighting colour:
  `CreateStaticProxyActor` fills it from a `VIColor32` over 255 and
  `RasterActor` hands it to `VIRaster::SetStaticLighting`.
* **`VIScene`'s two streaming-radius arrays are PAIRS** — `m_p2A48[4]` is
  `{outer, inner, outer2, inner2}` (950/900/550/500) and `m_p2A58[5]` the same
  four behind a 1050 that the streaming loop resets `v.m_radius` to on every
  pass.  The copy between them is **`[2]=[1], [1]=[0], [4]=[3], [3]=[2]`**,
  inner before outer twice, which a 24-way `_perm.py` sweep found.
* **`VIZone::Load` takes `(id, zone, VILoader &, file)`** — read off
  `StreamInStaticActor`.

### Levers

**A guard that `return 0`s lets gcc if-convert the LAST test; a guard that
`goto done`s does not.**  `QueryEmittersActor` ends `if (Sprite(n)->QueryEmitters(...)
< 0) return -1;` over a shared `return 0`.  With three `return 0` guards above
it gcc collapses the tail to `slti` + `li -1` + `movz`; rewriting the guards as
`goto done` with ONE `return 0` at the bottom leaves the target's `move v1,v0` +
`bltz` and is byte-exact.  The same edit closed `QueryEmittersAtLocationActor`
and `StreamInStaticActor`.  What blocks the if-conversion is the number of
reaching definitions at the exit, not the shape of the test.

**Two `return -1`s cross-jump only when they are the SAME statement.**
`CreateActor` has one at the top and one at the bottom; as two `return -1`s the
second lands inline as `b` + `li`, and as `goto bad` over one bottom `return -1`
both branch to it.

**A subscript at each use gives TWO induction pointers; a named reference gives
one pointer and a counter.**  `StopSoundTrack` walks `m_soundTrack.m_state[0..1]`
and the target carries `this + 40k` for the fields AND `&m_state[k]` for the
`Clear()` call.  A `VISceneSoundTrackState &st` is 37 of 55 words wrong;
spelling the subscript at all four uses is byte-exact.

**`a[k]` and `a.m_data[k]` are not the same to the strength reducer.**
`ReleaseStatic`'s inner loop reads `z.m_rooms[i].m_a3C[k]`.  Written
`.m_a3C.m_data[k]`, gcc creates a second reduced induction variable for the
outer room address and spills it (43 of 189 wrong); through `operator[]` it
recomputes `i * 120` with a `mult` at the top of the outer body, as the target
does, and the function is one word away.

**A scratch local keeps its constructor only if it is declared with the
others.**  `StreamFillCache(VIBBox)` has a `VIBBox` at sp+192 whose six zero
stores survive in the target although the next statement overwrites all six.
Declared just before its first assignment they are dead-eliminated; declared at
the top beside the vars struct they stay.  Phase 48's "declared FIRST lands at
sp+0" picks which local gets sp+0; this is the other half.

**Name the clamp, or gcc distributes the add into all three arms.**
`CalcOutdoorsIntensity` ends `m_p30 = m_p30 + VIMIN(lim, VIMAX(-lim, d));`.  As
one expression gcc if-converts the clamp into three branch-likely arms each with
its own `add.s`; assigning it to a `float c` first leaves the target's three
`mov.s` and one `add.s` at the join (31 wrong -> 9).

**Two identical tails beat one shared tail** when the target duplicates the
argument setup.  The same function's `m_p28 = GetTime()` is reached from both
arms; written once after an if/else gcc shares `{store; move a0; jal}`, written
out in BOTH arms it cross-jumps to `{jal}` with the store as the delay-slot
filler and `move a0,s1` duplicated (9 -> 3).

**An early `return -1` costs the entry-block constant.**  `GetNextColorBuffer`
guards `if (m_p444 == -1) return -1;` and has a second `return -1` below.
Nesting the body under `if (m_p444 != -1)` with ONE `return -1` at the bottom
lets gcc materialise `li v0,-1` in the ENTRY block, use it as the comparand AND
as the return value, and fill the guard's delay slot with the epilogue's
`lq ra` — byte-exact where the flat form is two words wrong.

**`(int)` on a `long` difference is three instructions of evidence** —
`dsll32` / `dsra32` / `cvt.s.w`, where `(float)(long)` is a different
conversion; worth nine words in `CalcOutdoorsIntensity`.

**A store PAIR is emitted in reverse source order**: `RasterActor` writes
`m_p444` then `m_p448` and the object code stores 1096 then 1092.

**Phase 9's pointer lever twice more** — `RasterActor`'s distance needs
`VIVect3 &at` and `VIVect3 &eye`, `CalcOutdoorsIntensity` needs
`VIVect3 &eye = cam.m_position`, and a `VIColor32F &q = actor->m_p70` is what
puts the w-store on its own base in both actor constructors.

### Shipped

**`ReleaseStatic` leaks a room's material palette when there is no raster.**
The palette release and the `m_flags &= ~4` that records it are BOTH inside
`if (m_raster)`, so a scene built without a raster keeps bit 2 set on every room
for ever and never retries.  That nesting was the single word between the
function and byte-exact, so it is measured, not inferred.

**`CreateActor` and `StreamInStaticActor` write the SOURCE sprite's index into
the COPY's `VISprite::m_actor`** — the field every other site treats as the
owning actor.

### Open and not attempted

`Collide(sphere, filter, query)` 2/48 and `CalcOutdoorsIntensity` 3/107 are each
one register pair apart; then `RemoveActor` 6/71, `SetTimeOfDay` 6/23, `Clear`
8/198, `InsertActor` 8/60, `QueryVisibleActor` 14/89, `Collide(sphere, filter)`
19/44, `CollideActor` 34/71, `Init` 51/225 and `RemoveActorsFromZone` 54/107.

Not attempted (15): `InitTimeOfDay` (1,165 instructions), the three
`SetupTimeOfDay*` (714/499/336), `StreamOutStaticActor` (666), `RasterActorBBox`
(657), `RenderShadow` (559), `SetSoundTrack` (384), `UpdateSoundEmitters` (349),
`DebugResources` (286), `Render` (222), `RenderDeferredList` (159), both
`SetSky`s and `SetupTimeOfDay`.  The `SetSky` pair shares a file static and
needs `VIAtmosphere`'s entry points; the time-of-day group is a phase of its own.

Long form: `claude-notes/decomp-phase54-the-scene-20260828.md`.

## Phase 53 result: the character sprite

**2,508/2,851 -> 2,538/2,895.  One class, `vicsprite.cpp`: 44 of `VICSprite`'s
50 functions written, 30 byte-exact.**  SUPPORT `0x00420F20 .. 0x004283C8`.

`VICSprite` is what every player and every NPC in EQOA actually *is*: a
`VIHSprite` with the whole customisation layer bolted on top — sixty-six
animation slots, nineteen texture slots, six attach points, a collision capsule,
a weapon trail, one sound, and an array of node-bound particle emitters.  At
**4,624 bytes it is the biggest object in the corpus**, and the class is what
the phase is really about; the layout was read off the constructor and `Clear`
and then checked against a `this`-offset census over all fifty methods
(`re-tools/thisscan.py`), which touches nothing outside it.

**Its own data starts at +0x190 and not at +0x188, where `VIHSprite`'s last
member ends.**  `VIHSprite` carries a `VIMatrix44`, so it is `aligned(16)` and
its SIZE rounds up; the eight bytes of the base's tail padding are why nothing
in the class ever touches +0x188.  This is the first place in the corpus where a
base class's tail padding is visible, and missing it would have shifted every
field by two words.

**The constructor is a member-constructor roll-call.**  gcc 2.95 emits the base
constructor, the vptr store, then EVERY member constructor in declaration order,
and only then the body — so the run between the vptr store and the last `jal` is
a LIST of which field groups are members with constructors of their own.  That
is what makes `m_entries[66]` and `m_slots[19]` arrays with an element
constructor (gcc's vec-init: ascending pointer, counter counting down from `n-1`
through `-1`), `m_hair` a class rather than three ints, and `m_snd` a class
rather than four fields — and `Clear` never touching those last two is the other
half of the evidence, because they are initialised once, at construction.  The
four `-1` loops at the END of the constructor are body code, and their pointer
DIRECTION separates the two source shapes every time: descending is
`for (i = N-1; i >= 0; i--)`, ascending with a down-counting register is the
ordinary `for (i = 0; i < N; i++)` gcc reversed.

Levers.  **The priority table is written LAYER BY LAYER, not entry by entry** —
`SetAnimPriorities` is 112 straight-line stores, and 56 interleaved pairs in
index order score 119/122, the object code's own emitted order 70/122, and all
56 `[0]`s followed by all 56 `[1]`s byte-exact on the first build.  **A two-case
if-chain on a sprite type is a SWITCH** (phase 47's lever again; 17 wrong to 0
in `SetSkinsVisible`).  **`~4` is one `li` where `~4u` is `lui`+`ori`**, four
times in this class.  **A hoisted `this` for a call argument costs a
callee-saved register**: `m_emitters[i].m_emitter.Create(ps, ps->Definition(…))`
computes the emitter address before the call and parks it callee-saved, where
naming the definition in a local first puts it after — 30 wrong words to 0 and
16 bytes of frame.  **A local's declaration ORDER decides the prologue**
(`Share`, 34 wrong to 22).  And **the element constructors' field order IS the
source order**: gcc is stable over a run of independent stores here, so writing
`VICSpriteAnimEntry`'s thirteen initialisers in the object code's own order
(`m_animation[1]` before `[0]`, `m_channel` before the float pair) reproduces it
and the naive index order does not.

Shipped: **`SetHelm` tints material palette entry 0, N times** — the outer loop
runs over `GetMaterialPalSize(pal)` and the body passes a literal `0` as the
entry index, so every other entry of a copied helm palette keeps its original
colour; the same function calls `VIRaster::MaterialPal(pal)` and throws the
answer away; and **`AttachItem` does nothing for a player**, because its first
test is the NPC flag `Init` sets from its last argument.

Harness: **`_perm.py`** is new — a permutation sweep over a contiguous run of
source lines that rebuilds one TU and scores one symbol, loading the target
image once so a 24-way sweep is four minutes instead of twenty.  `viarray.cpp`'s
`VICSpritePartEmitter` stub is gone, the real class carrying phase 24's measured
constructor into `vi.h` (phase 41's grep-for-the-stub rule, third occurrence),
and `VICapsule`/`VICylinder` moved up in `vi.h` above `VICSprite`, which has one
by value; `VITrailFx` only needed a forward declaration.

Open: `SetHair` 192/235, `AttachItem` 110/162, `Release` 61/177, `SetDefaults`
35/94, `SetAnimSoundChannels` 35/80 (one instruction — `move v0,zero` in the
`jr` delay slot rather than scheduled into the middle), `SetItemAction` 32/79
and `IsActionComplete` 6/18 (both if-convert a `return 1`/`return 0` pair to
`sltu` where the target branches; the `-1` sentinel is CSE'd into a
callee-saved register in ours and rematerialised in the target, which is what
makes the conditional move cheap enough to win), `SetArmorSlot` 29/94, `Share`
22/155, the constructor 11/214, `Clear` 11/128, `SetHelm` 8/131, `Init` 7/45
and `SetDeath` 2/122.  Not attempted: `SetName`, `SetDefaultEntries`,
`SetAnimation`, `Process`, `CalcVolumeAndPan`, `Copy`, `ProcessWeaponTrails`
and `UpdateParticleEmitters` — six of them over 230 instructions, and
`SetDefaultEntries` is a 66-way switch whose jump table is read out of the dump
at `_9VICollRay$PerturbMult+0xE0`.  The two attack-action jump tables at +0x20
and +0x50 ARE decoded, and both those functions are byte-exact.

Long form: `claude-notes/decomp-phase53-the-character-sprite-20260828.md`.

## Phase 52 result: the zone tree

**2,484/2,798 -> 2,508/2,851.  One translation unit, `vizone.cpp`: 50 of
`VIZone`'s 67 functions written, 24 byte-exact.**  SUPPORT
`0x00493830 .. 0x00498CC0`.  The sibling of phase 51, one level down: a
`VIWorld` is a BSP over ZONES and a `VIZone` is a BSP over ROOMS, and the room
is where the scene's actual work — raster, collide, pick, stream, query —
finally happens.

**The two descents are ONE function, and this phase measured it.**  Disassemble
`Collide__7VIWorldiR15VISceneCollVars` and `Collide__6VIZoneiR15VISceneCollVars`
and diff them by mnemonic: they agree instruction for instruction except for the
register that holds `node` (`t0` in the world, `a3` in the zone, because the
world's leaf arm needs more of them), `m_tree`'s offset (52 vs 68), and the leaf
arm itself.  The same holds for `Pick`, `QueryProximal`, `QueryEmitters`,
`Stream` and `QueryVolume`.  So the descent half of this file is phase 51's
source with one line changed — `m_zones[m_tree.m_leaves[node].m_a].Collide(v)`
becomes `CollideLeaf(node & 0x0FFFFFFF, v)` — and it lands byte-exact on the
first build: the three biggest (212, 204 and 196 instructions) all matched
untouched.

**The leaf end is what is new**, and it is four facts about `VIZoneRoom`.
`m_p20` is the DESCENT stamp, compared against `v.m_stamp` (the world tree's own
`m_state`, threaded down through the Vars) so a room reached through two leaves
is walked once.  `m_p24` is the SCENE stamp, on a different clock: the room
walkers compare it against a counter at **`VIScene + 0x544`**, the last word
before the embedded `VIParticleSystem`.  `m_p1C` is the room's own static
sprite, `-1` when it has none, drawn and collided through
**`VIZone::IdentityMatrix`** — a static `VIMatrix44` the constructor
re-`Identity()`s on every zone it builds, because a zone's geometry is already
in world space.  And `m_p68` is the head of the room's ACTOR list: a pointer
kept in an `int`, the actor id at `+0x04`, the successor at `+0x10`, and the
list ends at `-1` rather than at null.

**`VIZone::Find` is a BINARY SEARCH.**  The resource directory
(`VIArray<VIResourceElem>`, 16 bytes an entry, the DictID at `+0x0C`) is sorted,
and `Find(id, off, size)` bisects it: `lo = 0`, `hi = m_size - 1`,
`mid = (lo + hi) >> 1` with an ARITHMETIC shift, `sltu` on the unsigned key, and
a re-test of `hi < lo` after the loop deciding between "found" and 0.  A zone
resolves a DictID in log time with no dictionary at all.

**The room streamer is a six-way decision on ONE distance.**  `StreamLeaf` takes
the squared distance from the room's bbox to the camera (or, in the bbox
overload, the overlap of two boxes) and drives two independent triples off it:
the room's own geometry (`In`/`Halt`/`Out`, gated on `flags & 2`) and the room's
STATIC actors (`In`/`Halt`/`Out`, gated on `(flags & 0x38) == 0x38`).  The two
use different radii — `m_inner`/`m_outer` for the room, two more for the statics
— so statics come and go on their own hysteresis inside the band where the room
itself is already resident.

Levers.  **A NAMED REFERENCE to a member array is what materialises its
address.**  Three wrappers hand `m_pretrans`'s size and block to
`SetPreTranslations`, and the target reads the size folded (`lw a1, 84(s0)`) and
the block through an address pseudo (`addiu v0, s0, 84` / `lw a2, 4(v0)`).
`m_pretrans.m_data`, `&m_pretrans[0]` and an inline `T *Data()` **all three fold
to one `lw a2, 88(s0)`** — gcc simplifies `&*(m_data + 0)` back to the member and
then folds the base.  A named `VIArray<VIVect3> &pt = m_pretrans;` does not, and
`pt.m_size` still comes out folded because its own `+0` folds back.  That one
line took `Collide` (both overloads) and `Pick` from 13/33, 13/33 and 12/37
wrong to byte-exact.  It corrects phase 51's rule: an inline subscript is an
address pseudo only with a VARIABLE index.

**Phase 51's cross-jump lever, run BACKWARDS.**  The world's wrappers have ONE
`jal` because gcc cross-jumps the two arms of `if (size) return Q(0,v); return
Q(LEAF,v);` back together; the zone's have TWO, because the world's tail is
longer (it also bumps `m_tree.m_state`) and with nothing after the call the
merge is what gcc prefers rather than what the target has.  Writing it as
`int hit; if (size) hit = Q(0,v) != 0; else hit = Q(LEAF,v) != 0; return hit;`
blocks the merge.  So the rule is not "two calls with the test inside each arm":
it is that the if/else-into-a-variable form and the two-returns form differ, and
**which one the target used has to be read off the number of `jal`s**, per file.

**A `-1` sentinel in a POINTER costs two instructions; in an `int` it costs
one.**  The four room walkers end in a walk of the actor list, and
`a != (VISceneActorRef *)-1` builds the constant with `lui 0xffff` + `ori
0xffff` at both the entry test and the loop test.  Keeping the cursor an `int`
and casting inside the body took `CollideRoom`, its ray twin and `PickRoom` from
30/65 wrong to byte-exact.

**Phase 51's Vars structs were wrong in eight places.**  It named only the
fields `viworld.cpp` touched and guessed at the rest; the zone's leaf and room
walkers hand every one of them to a `VIScene` method, so they are read now:
`VISceneInsertVars` `+0x04` is the ACTOR and `+0x0C` the scene (the leaf calls
`v.m_scene->InsertActor(v.m_actor, v.m_zone, room)`); `VIScenePickVars` `+0x10`
is the RASTER; `VISceneRendVars` `+0x34` is a float view distance, not a room
index; `VISceneProxQueryVars`, `VISceneVisQueryVars` and
`VISceneEmitterQueryVars` carry a `VISphere` and a query object where phase 51
had a loose point and radius, `VISceneBBoxQueryVars` a `VIBBox`, and
`VISceneStreamVars` `+0x08`/`+0x0C` are the dictionary and the raster.  Every
rename is offset-for-offset and `viworld.cpp` was migrated with them and
re-scored: nothing that was byte-exact stopped being so.

Open: almost every miss is **one phenomenon, and it is phase 51's** — a function
whose last statement is `if (f() < 0) return -1; return 0;` if-converts in our
build (`slti`/`li -1`/`movz`) where the target branches (`move v1,v0`/`bltz`/
`li -1`/`move v0,zero`).  Nine spellings were swept and every one if-converts;
it costs 4 words on each of five `*Leaf` functions and 10 on each of seven
wrappers, and it is the same wall that left phase 51's `RenderByVolume(int)`,
`Stream(int, radial)` and `QueryProximal`/`QueryEmitters` open.
`SetPretranslations` (33/47) writes each pre-translation to `m_p[i]` **and** to a
dead 12-byte stack slot from the same three registers, with the `m_p` stores
FIRST; seven spellings were tried and a `VIVect3` local always emits its zeroing
constructor (which the target has not) and always writes the temp first.

Not attempted: `RenderRoom`, `RenderThroughPortals`, `QueryThroughPortals`,
`QueryVisible(int)`, `QueryIntersection(int)`, `QueryIntersectionRoom` (719
instructions), `StreamRoomIn`, `StreamRoomStaticsIn`/`Halt`/`Out`, the three
`VIEnvRay` collide functions and `Load`.

Long form: `claude-notes/decomp-phase52-the-zone-tree-20260828.md`.

## Phase 51 result: the world tree

**2,457/2,761 -> 2,484/2,798.  One translation unit, `viworld.cpp`: 37 of
`VIWorld`'s 44 functions written, 27 byte-exact.**  SUPPORT
`0x0048E380 .. 0x004924D0`.

`VIWorld` is the top of the spatial hierarchy: a rectangular TERRAIN grid, an
array of zones, and a BSP tree over the zones.  Every scene-level query --
render, collide, pick, insert, stream, and the five `Query*` -- arrives here as
ONE `Vars` reference and is walked down the same descent, which hands each leaf
to the `VIZone` that owns it.  `go-eqoa-vi`'s `vicoll/scene.go` walks this same
tree against the console (`claude-notes/scene-collide-live-oracle-20260816.md`)
and was the semantic reference; this is the source that produced it, and the
two agree.  The sibling TU `vizone.cpp` -- whose descent is instruction for
instruction the same function -- is phase 52.

**976 bytes**, and the size is not a guess: `SetProfiles`' last store is `+964`,
inside the 22nd and last 40-byte terrain profile, and `96 + 22*40` is 976.

    +0x00  VIBBox      m_bbox        -1000 .. +1000 from the constructor
    +0x18  int         m_numRows     1
    +0x1C  int         m_numCols     1
    +0x20  float       m_cellSize    2000.0
    +0x24  VIVect3     m_origin
    +0x30  int         m_subdiv      4
    +0x34  VIWorldTree m_tree
    +0x48  VIArray<VIZone>        m_zones      stride 592
    +0x50  VIArray<VIWorldRegion> m_regions    ONE PROFILE BYTE per sub-cell
    +0x58  float, float                        250.0 and 800.0
    +0x60  VITerrainProfile m_profiles[22]     40 bytes each

**`VIWorldTree::m_state` IS the pass stamp.**  Phase 27 named it off the
constructor's `= 1` and left it there; every traversal wrapper here copies
`m_tree.m_state` into the Vars, runs the walk, and increments it.  The world's
visit counter lives inside its tree object, at `world+0x44`.

**A `VIWorldRegion` is one byte and the region array is the terrain-profile
grid.**  `Init` sizes it `m_numRows * m_numCols * m_subdiv * m_subdiv` and
`GetTerrainProfile` ends in an `lbu`.

**A child index is TAGGED.**  Any bit of `0xF0000000` says "not an ordinary
interior index"; `0x40000000` on top of that says LEAF; **any other tag is
stripped and used as an interior index anyway**, so the tag space is not a
simple leaf/interior bit.

**Three descent shapes, not one.**  A SEGMENT walk (collide sphere, collide
ray, pick, the env ray) takes the signed distance of BOTH sweep endpoints; a
SPHERE walk (radial stream, proximal query, emitter query) takes one point and
a radius; a VOLUME walk (render, bbox stream, volume query) tests EIGHT corners
and sets a front/back flag pair.  What separates the three segment walks is
only what the slack `r` is -- `m_radius + 0.03125` for the sphere collide, the
bare `0.03125` for the ray, `0.0` for the pick.

### The levers

**A constant-selecting wrapper is TWO CALLS with the test inside each arm.**
Twelve wrappers choose between node `0` and the leaf tag `0x40000000`.  Written
as a select feeding one call, gcc if-converts it (`movz`) and the branch is
gone; written as two calls with the `!= 0` / `< 0` test INSIDE each arm, gcc
cross-jumps them back into ONE `jal` and the branch survives -- which is what
the target has.  **Eleven of the twelve went byte-exact on that one edit.**

**`m_tree.m_nodes[node]` is not `m_tree.m_nodes.m_data[node]`.**  Written out,
the chain folds to one `lw <d>, 56(this)`; the target has `addiu <t>, this, 52`
+ `lw <d>, 4(<t>)`, an ADDRESS PSEUDO the recursion keeps callee-saved.  Phase
32's rule, reached by giving `VIArray<T>` an inline `operator[]`.  58 wrong
words -> 17 on the node array, 17 -> 0 on the leaf array.  It is per SITE:
`m_zones.m_data[...]` in the same leaf dispatch stays written out.

**Two distances computed before the first test are LOCALS, reassigned.**  The
descent computes both endpoint distances interleaved in one block and
recomputes both after each recursive call.  Spelling the expression out gives
short-circuit evaluation with the second dot in the successor block (207 of 227
wrong); locals filled once and ASSIGNED AGAIN after each call give the target's
shape (100), and the `else if` chain with `hit = 0; if (call) hit = 1;` closed
it.

**`int r = -1; if (0 <= X()) r = 0; return r;`** is the tail of every
`Clear`-like function: `slti`, `li -1`, `movz`.  `return X() < 0 ? -1 : 0;`
materialises `-1`, compares `-1 < r` and uses `movn` -- six words apart.

**A clamp on a REFERENCE parameter is ONE read-modify-write.**  Two `if`s on an
`int &` give a branch and a store each; the target has one `lw`, two
conditional moves and one `sw`, which means the source clamps a LOCAL and
assigns the reference once.  The lower clamp is a ternary (`movz`) and the
upper an `if` (`movn`).

**The clamp ORDER is a register-allocation fact.**  `GetTerrainProfile` clamps
COL fully and then ROW; row-first, negatives-first, bounds-in-locals and
`VIMIN`/`VIMAX` all cost eleven words of register naming and nothing else.

**A local whose constructor emits code belongs in the scope that uses it.**
`QueryVisibleWithPortals`' `VIBBox` is constructed after the early returns --
declared at the top, the frame grows from 160 to 176 bytes and the zeroing
stores move to the entry block.

### Harness

`VIWorld` moved to the BOTTOM of `vi.h`: it holds `VIArray`s and the template
is declared 2,000 lines below `VIScene`, which owns a `VIWorld` at `+0x04`.
`VIScene` carries it as `char m_world[976]` and
`VIRadialFloraSystem::CalcZoneIndex` casts -- the cast emits the same
`scene + 4` the member did, and that function still matches.  The eight element
classes `viarray.cpp` kept as local stubs (`VIWorldNode`, `VIWorldLeaf`,
`VIZoneNode`, `VIZoneLeaf`, `VIWorldRegion`, `VIRadialFloraModel`/`Dist`/`Set`,
`VIZoneStaticTableElem`) moved to `vi.h` with them, and `viarray.cpp` still
scores 132/132.

### Open

`SetProfiles` (600 words) and `CalcProfileBlend` (166) are the terrain
time-of-day table and are not attempted; `QueryVisible(int)`,
`QueryIntersection(int)`, `StreamLeaf(int, VISceneBBoxStreamVars&)` and
`Collide(int, VIEnvRay&, VIVect3&, VIVect3&)` are declared but not written.
Of the ten written-but-open, **six are one phenomenon**: the last recursive
call of the two arms cross-jumps in ours and does not in the target
(`RenderByVolume` 15/143, `Stream(radial)` 17/116, `QueryProximal` and
`QueryEmitters` 15/110).  `FindLeaf` 28/73 wants `&m_tree.m_leaves` and
`&m_zones` materialised in BOTH predecessors of the join;
`CalcTerrainCellBVolume` 21/63 needs `&m_origin` as an address pseudo while the
three components are negated into a stack `VIVect3`; the constructor is TWO
words of scheduling in the member array's implicit construction loop.

Long form: `claude-notes/decomp-phase51-the-world-tree-20260828.md`.

## Phase 50 result: the drdp core, and the last of the library

**2,357/2,649 -> 2,457/2,761.  One translation unit, `drdp.cpp`: 112 functions
written, 100 byte-exact, 72 of them on the FIRST build.**  With this the DRDP
network library is COMPLETE -- all twenty-one of its translation units are in
`src/`.

A `drdp_t` is one PROCESS on the network: the sockets it listens on, every
endpoint, route and connection it knows about, the address<->route cache and
the leaky bucket that paces the whole machine.  544 bytes off `malloc`, tagged
`"DRDP"`.  Three ROLES share the file -- MASTER 0, LEADER 1, CLIENT 2 -- and
the system messages that arbitrate between them are a 48-entry jump table in
`drdp_process_system_message`, the largest function in the library at 524
instructions.

**An endpoint id IS an address**: four octets, the UDP port and the drdp port
packed big-end first into 64 bits, so routing to a hardware endpoint needs no
lookup at all.  A local hardware endpoint is `0xffffffffffff0000 | drdpPort`
and a SOFTWARE endpoint is a plain number in `[2048, 0xffffffff]`.  **The
system endpoint's drdp port is the low half of the CLOCK**, stepped off the
three reserved values (`0xffff` -> 0, `0xfffe` -> 1, `0xfffd` -> 2), which is
the whole of the local port allocation.  **The CRC carries the protocol
version**: the sender XORs `drdp_crc_table[version]` into the packet crc and
`drdp_consume_crc_get_protocol_version` recovers it by looking the difference
up in the same table, searched outwards from where version 2 sits -- so a
packet from an incompatible build reports its version instead of looking like
corruption.  **A packet's segments are walked TWICE**, once to check every
header parses and names two real endpoints and only then to deliver, so a bad
packet is reported to every connection on the route rather than half-applied.
And **the route table goes out delta-coded**, every range's owner an INDEX into
a leader array sent ahead of it, with `n` meaning the master and `n+1` nobody.

Levers.  **An inline helper's key is a CONST REFERENCE** -- the debug-checked
binary search that turns an owner id into a leader index re-reads its key out
of memory on every iteration, which a by-value `uns64` never does; `const uns64
&` took `drdp_master_send_route_table` from 219 wrong words to 53, and it is
also what puts the caller's temporary on the stack.  **An assert whose
expression CALLS the helper keeps the result in `v0`** where an assignment
followed by a VASSERT parks it in `s0` and shifts every register below it (53
-> 23).  **A line number inside a multi-line macro invocation is the CLOSING
line**, so every assert in a padded file has to be one line.  **A
default-callback if/else puts the DEFAULT on the fall-through** -- `if (pfn ==
0) X = default; else X = pfn;`, 60 wrong words to 5.  **Where one store sits
among a run of zero stores decides the schedule**: `drdp_create_addr`'s last
five closed on moving `drdp->role = role;` up ONE statement.  **A buffer rebase
must not zero the position first**, or gcc folds the zero and deletes both
adds.  **An aligned six-byte struct copy is a `memcpy`, not an assignment** --
a struct assignment uses `TYPE_ALIGN` (2 here) and gives `lwl`/`lwr`, where
`memcpy(&to,&from,4)` plus a field copy gives `lw`/`sw` + `lhu`/`sh`.
**Splitting `x = c | (x << 16)` into two statements** turns a shift into a
temporary into an in-place one.  **`int t = field;` gives `slti` where the
field compared in place gives `sltiu`.**  **`if (bits) r = 1;` and `r = bits ?
1 : 0;` are not interchangeable** -- the first gives `li 1` + `movz`, the second
`sltu` plus a `dsll32`/`dsra32` pair, because the DImode comparison has to be
truncated to `int`.  **A three-arm chain whose middle two arms share a value is
one `||`.**  And **a walk that re-PEEKS is not a get_next walk**:
`drdp_reconcile_local_endpoints` calls `vtree_peek_first_in_range` every
iteration because the body takes the element off the tree.

Shipped: `drdp_handle_segment_endpoint_not_local` calls `buffer_skip` on BOTH
arms of a test whose sides are otherwise identical -- the forward that used to
sit on one of them is gone and nothing in the recovered library calls
`route_forward_segment`.  Seven functions evaluate a `drdp_format_endpoint*`, a
`message_get_size` or a `connection_get_dst` and throw the answer away, the
same trace residue phase 49 found in `route_flush`.  `find_leader_index`'s two
asserts quote a DIFFERENT source file from every other assert in the object,
which is the proof it is an inline function in an include.

Harness: the working loop for a padded file is **collapse -> edit -> repad** --
squeeze every run of blank lines to one so a pattern match sees the real text,
edit, then re-insert exactly the blanks each line number needs.  A failed
pattern in a multi-edit script throws away the edits that already succeeded, so
apply one edit per script.

Open: `drdp_process_system_message` 370/529 and `drdp_process_udp` 84/223 are
one phenomenon and it is stack LAYOUT -- the original gives every local a
16-byte-aligned slot in declaration order, so a two-byte `uns16` sits between
two aggregates, where this build assigns all the BLKmode locals first and then
PACKS the address-taken scalars after them.  Nothing about the code shape
differs.  Plus ten smaller residues, all register allocation or one word of
if-conversion.

Long form: `claude-notes/decomp-phase50-the-drdp-core-20260828.md`.

## Phase 49 result: the route layer

**2,310/2,598 -> 2,357/2,649.  Three whole translation units: `route.cpp` 31/32,
`route_cache.cpp` 9/10, `route_table.cpp` 7/9 — 51 written, 47 byte-exact, 39 of
them on the FIRST build.**  With this the DRDP library is complete except for
`drdp.cpp` itself.

A ROUTE is one UDP peer: the socket address every connection to that machine
shares, the leaky bucket that paces it, and a tree of connections sorted by when
each wants to go out next.  **`route_get_key_offset` is the OPPOSITE way round
from the connection's** — key 0 is the flush time and key 1 the id, where the
connection puts the far id first.  `route_flush` is **the only place in the whole
library that builds a packet**: two ports, the resets, the forwarded bytes, then
as many connection segments as fit, walked in flush-time order so the connection
that has waited longest goes first; the flush number is bumped at the end and
**skipped when it would become -1**, because -1 is the "never flushed" value.
`route_cache.cpp` is the NAT table -- address to route and back, five-minute
expiry renewed on use, one record on THREE containers.  `route_table.cpp` is a
tree of RANGES whose comparator returns 0 for any two that OVERLAP, so a
degenerate-range lookup finds the range containing an id.

Levers.  **`vset_remove_first` returns 0 on SUCCESS**, so both flush-reset loops
test `== 0` and the plain truth test inverts the branch.  **A runtime-sized stack
buffer is `alloca`, and the give-away is the frame pointer** — a fixed
`addiu sp,sp,-N` plus `move s8,sp` plus a SECOND `addiu sp,sp,-M`;
`route_forward_segment` 27/55 wrong to byte-exact on one `__builtin_alloca`.
**A 40-byte local is placed before a 2-byte one whatever the declaration order**,
so the only way to get `usPort` ahead of a `segment_header_t` is to declare the
header inside the block that uses it (18 words).  **A comparison tree with a
`slti` for "below the range" is an if-CHAIN, not a switch** — phase 47's rule run
backwards.  **A three-way comparator's last `return 1` goes out of line**: written
`if (a > b) return 1; ...` gcc cross-jumps it into the shared block, and
`if (!(a > b)) return a < b ? -1 : 0; return 1;` is what the target has (three
comparators, 9 words).  And **`route_create` writes `m_uiId` AFTER the clock
read** — 39 wrong words to 0 on moving one line past a call.

Shipped: `route_flush`'s forwarded-message loop evaluates `message_get_size` an
extra time and throws it away, the argument of a trace whose call is gone.

Harness: phase 48's trap again, in a form it does not name — **two `eqcheck.py`
invocations in ONE shell pipeline** collide in `decomp/build` exactly the way a
backgrounded one does.  One scorer at a time.

Open: `route_flush` 59/234, `route_cache_touch` 82/166,
`route_table_update_range` 97/201 and `route_table_find` 11/30 — all four are
the largest function in their file, and all four are register allocation or
branch layout rather than shape.

Long form: `claude-notes/decomp-phase49-the-route-layer-20260828.md`.

## Phase 48 result: the connection

**2,236/2,493 -> 2,310/2,598.  One translation unit, `connection.cpp`: 105
functions written, 74 of its 116 symbols byte-exact.**

`connection_t` is the record for ONE conversation between two endpoints, 4,576
bytes, the largest struct in the library by a factor of seven — and the layout
`endpoint.cpp` said in its own header comment it was blocked on.  It is on
THREE containers at once (`connection_get_vnode_offset(i)` is `i*16+4`): the
endpoint's tree keyed by the far id, the drdp's, and the ROUTE's tree keyed by
the desired flush time, which is the one `route_flush` walks in order.

**`+0x120` is `uns32 m_uiFlushTime[1024]`, not a byte buffer** — a ring of send
timestamps indexed by `flush & 1023`, and the round trip is the difference
between the ring entry and now.  **The timeout is the LARGEST of the sixteen
samples and the reported ping is the SMALLEST**: two loops over the same array
with opposite comparisons.  248 state channels of 60 bytes are malloc'd in ONE
block on first use, so a connection that never sends state never pays for them;
a state channel is a SLOT, and `send_state` sends nothing when the bytes have
not changed and replaces the pending message wherever it is sitting when they
have.  A guaranteed send is FRAGMENTED at 1,300 bytes into type-250 messages
with their own sequence numbers, up to 0x514000 bytes, built into a local list
first and only queued once every piece succeeded.

**The terminate notification is built in `connection_create`** and kept for the
life of the connection, so a terminate can be delivered when there is no memory
left to allocate one.

Levers.  **A range check folds to ONE unsigned compare**: `sltiu v0,a0,248` is
`0 <= t && t < 248`, not `t < 248`, which gives `slti`.  **A stack object
declared FIRST lands at offset 0 and its address is then free** — the five
`send_uns8_*` helpers went from 20/23/30/33 wrong to byte-exact on swapping
`buffer_t buffer;` ahead of `char data[64];`.  **A result variable that shares a
register with a parameter must be UNINITIALISED**: `int iResult = 0;` at the top
of `send_internal` costs a whole callee-saved register and 16 bytes of frame.
**A member compared against a CALL's result wants a local** hoisted above the
`if`.  **Which of two constants ends up in the destination is decided by the
INITIALISER**, not by if-versus-ternary — put the value you want in the
destination register first and the `movn`/`movz` follows.  **Three stores to
adjacent fields are a permutation sweep**: only one of six orders matches
`connection_set_failure` and it is not the one the delay-slot rule predicts.

Shipped: **`VASSERT(!"this makes no sense")` can never fire** — gcc 2.95 does not
fold a string literal's address to a constant truth, so what ships is a runtime
`beqz` on a link-time address and the branch always passes; every such assert in
this library is dead the same way.  And **filling the 4,096-message guaranteed
window KILLS the connection**: `send_guaranteed` returns -1 and `send_internal`
turns any non-zero result into a terminate.

Harness: two new `SPAN_CAP`s (`state_channel_init` 640 -> 180 for the TU-local
`connection_init`; `connection_get_ack_mask` 304 -> 172 for the two XOR statics).
And **a backgrounded `eqcheck.py -f <regex>` is still `eqcheck.py`** — phase 41's
rule names `_q.py`, but the hazard is the shared `decomp/build`, and a filtered
run left running reported 17 phantom regressions in a file this phase never
touched.

Not attempted: `parse_segment_body` (3,064 bytes), `flush` (1,392),
`create_buffer` (1,304), `destroy_buffer` (928), the three flush-time-history
codecs and the two `message_*_load` serialisers — the save-and-restore half of
the file, which needs `segment_body_t`'s parse order settled first.

Long form: `claude-notes/decomp-phase48-the-connection-20260828.md`.

## Phase 47 result: the hierarchical sprite

**Fifty-five functions written, forty-two more byte-exact: the translation
unit is 81 symbols and 67 of them are exact.
2,194/2,438 -> 2,236/2,493.**

`VIHSprite` is the class every animated character in the engine is: a node tree,
a palette of blend matrices for the skinner, a trigger table, and three pools --
attachments, plays, and the per-node entries a play owns.  Phases 19/20 took
nineteen of the twenty-six readers it already had; this phase is the rest of
the translation unit --
constructor and destructor, the whole play-list surface (`AddPlay` x4,
`AddInterpolatedPlay` x2, `ErasePlay`, `StopPlayback`, `FreezePlayback`,
`SyncTiming`, `PrecalcPlayback`, `SetPlayFrame`), the pose evaluator
(`Process`, `ProcessPlayList`, `ProcessPlayTiming`, `SetBindPose`,
`ProcessHierarchy` x2, `ProcessUpHierarchy`, `SetBlendMatrices`), the
centre-transform cache (`SetCenterTransform`, `BlendToCenterTransform`,
`InterpTransAboutCenter`, `UpdateCenterTransform`, `CalcTransAboutCenter`,
`StopCenterTransformBlend`), both `CalcLODLevelIndex` overloads and all eight
virtuals.

As in Phases 11/13/18/22/46 the SEMANTICS were settled from the console first:
`go-eqoa-vi/vianim` is a bit-exact Go port of the pose path, checked against a
live SUPPORT through the tracer's `call`.  So every mismatch here was codegen --
and, as in Phase 18, the decomp paid the port back.

### The layout, and the two names that were wrong

```
+0x040 VIVector<VIHSpriteNode>     m_nodes
+0x04C VIVector<VIMatrix44>        m_blend        the skinning palette
+0x058 VIArray<VIHSpriteTrigger>   m_triggers
+0x060 VIPool<VIHSpriteAttachment> m_attach
+0x078 VIPool<VIHSpritePlay>       m_plays
+0x090 VIPool<VIHSpritePlayNode>   m_playNodes
+0x0A8 VIHSpriteLOD                m_lod          count + three distances
+0x0B8 long  m_playTime     ProcessPlayList's cache stamp
+0x0C0 long  m_centerTime   CalcTransAboutCenter's
+0x0C8 long  m_poseTime     Process's
+0x0D0 float m_defaultScale
+0x0D4 VIVect3        m_center[2]      [0] blend from, [1] live
+0x0EC VIHSpriteFrame m_transform[2]   [0] blend from, [1] live
+0x12C int   m_blendMS      0 == no blend in progress
+0x130 long  m_blendStart
+0x140 VIMatrix44 m_centerMatrix
+0x180 int   m_refMap       VIScene::Share/ReleaseRefMap
+0x184 int   m_defaultAnim  GetDefaultAnim
```

**`VIHSpriteNode`'s two frames were the wrong way round.**  Phases 19/20 read
+0x10 as the bind pose; `SetNodeBindPose` (0x00446058) and `GetNodeBindPose`
write and read +0x7C, +0x8C and +0x90, so the BIND pose is the second frame and
+0x10 is the ANIMATED one -- which is exactly what `go-eqoa-vi/vianim`'s
console-read map (`NodeRot 0x10`, `NodeBindRot 0x7C`) says.  And **+0xA0 is not
a second world matrix**: `SetNodeBindPose` builds `Invert(Transform(q, s, t))`
there and multiplies a child's world bind by its parent's copy to recover the
child's local rest transform, so it is the INVERSE bind-world matrix -- which is
what `SetBlendMatrices` hands `VIRaster::SetObjectBlends` as
`blend[i] = invBindWorld[i] * world[i]`.

**`m_center` and `m_transform` are ARRAYS of two.**  The constructor zeroes them
in LOOPS -- two turns of twelve bytes at +0xD4, two turns of thirty-two at
+0xEC -- and no pair of separate constructors gives a loop.

### Six levers

**A comparison's operand order decides which side is loaded first, and that
decides whether the guard's load survives a CALL.**  Three functions open the
same way: `m_blendMS == 0 || <the blend has expired>`.  Written
`m_blendMS < sc.GetTime() - m_blendStart` the member is the compare's LEFT
operand, gcc evaluates it first, cse ties it to the guard's own load and the
allocator parks it in a callee-saved register across `GetTime` -- one extra
saved register, one extra stack slot, thirteen wrong words, and a `move sN,v0`
in the `jal`'s delay slot where the target has `nop`.  Written
`sc.GetTime() - m_blendStart > m_blendMS` the subtraction goes first and the
member is re-read after the call.  `IsCenterTransformBlendComplete` 13/21 -> 0,
and the same edit closed `SetCenterTransform` (33/51 -> 0).

**A member whose constructor emits ONE store, landing between the last
out-of-line member constructor and the first inline array loop, is a MEMBER.**
Member constructors run in declaration order before the body, so `sw zero,168`
sitting between the sixth pool's ctor call and the `m_center` array's zeroing
loop cannot be body code.  `Copy` says what shape it is: it carries
+0xA8..+0xB8 across as two unaligned eight-byte block moves, which is the
implicit `operator=` of a class with an ARRAY member -- written field by field
it would be a word and a loop.  Hence `VIHSpriteLOD`.

**Naming a pool pointer can COST registers** -- Phase 22's pointer-variable
lever run backwards.  `VIPool<VIHSpritePlay> *q = &m_plays; for (i = q->Begin();
…)` hoists the 104-byte element stride into a callee-saved register (a 96-byte
frame where the target has 80); the member spelling `m_plays.Value(i)`
rematerialises it inside the loop the way the target does.  `Release` went 43 of
79 wrong to byte-exact on that edit alone, and `Collide(VICollSphere &)` and
`QueryIntersection` closed with it.  It is not "never name the pool":
`ProcessPlayTiming` and `ProcessPlayList` both NEED the pointer (the target's own
`addiu s0,this,120` is right there).  The rule is that a pointer whose only uses
are the three walk calls costs more than it saves.

**A container pointer declared INSIDE the loop.**  Phase 41's "ONE shared local"
run the other way: `SetBindPose` reads `m_nodes.m_size` off `this` and
`m_nodes.m_data` off a materialised `&m_nodes`, and that `addiu` must land after
the loop guard -- which it does only when the pointer is declared in the loop
BODY, where gcc's loop pass hoists it to the preheader.  Declared above the
`for` it is an entry-block expression and is scheduled with the guard instead.
8 wrong words to 0.

**The outer clamp macro's argument ORDER, a second sighting.**  Phase 39 found
that `VIMAX(x, k)`'s first argument decides the compare; here it decides which of
two identical-looking clamps comes out.  `VIMIN(VIMAX(t, 0.0f), 1.0f)` gives
`c.lt.s 0,t` and `c.lt.s t,1.0`; `VIMIN(1.0f, VIMAX(0.0f, t))` gives
`c.lt.s t,0` and `c.lt.s 1.0,t`, which is the target -- and it was the last
eleven words of `InterpTransAboutCenter`'s 209.

**A two-case if/else-if on a small int is a SWITCH.**  `if (mode == 0) … else if
(mode == 1) …` if-converts one arm and reorders the blocks; `switch (mode)`
emits the comparison tree the target has (`beqz` to case 0, `beq 1` to case 1,
fall through to the join).  `ProcessPlayTiming` 137 of 332 wrong to 26, and the
same shape appears again in `ProcessPlayList`'s blend dispatch.

And a seventh, small: **a second store through the same element wants a SECOND
named pointer.**  The trigger fire writes `m_c` and then `m_b`; the target
re-derives the address for the second, so reusing the first pointer keeps it in
a register (offset-first `addu`, one extra `move`) where
`VIHSpriteTrigger *tb = &ta->m_data[idx]; tb->m_b = 1;` reloads and goes
base-first.  8 words.

### Shipped

`CalcNodeTransform` (0x00446340) constructs and destroys a **second
`VIMatrix44` it never uses** -- 64 bytes of stack and two constructor loops per
call, on the path every attachment and every helm goes through.

`InterpTransAboutCenter` re-tests `m_blendMS` **three times** after an early
return that has already handled the zero case, so three whole branches of it are
dead.  The shape is not a guess: each test is a real `lw`/`bnez` pair in the
object code, and the function does not match without them.

### Harness

A new `SPAN_CAP`.  `vihsprite.cpp` closes with the TU-local copies of
`VIMap<int,int>`'s walkers, its red-black insert and the two rotations -- a run
of unnamed statics -- so `CalcLODLevelIndex(VIPickSegment &, VIRaster &)`, the
last named symbol in the file, acquires a 2,784-byte span.  268 is the function,
and `test_span_caps_only_ever_shrink` refuses any cap that does not land one word
past a `jr ra`.

Also: **renaming a field in `vi.h` breaks the TUs that already used the old
name.**  `m_defaultPlay` became `m_defaultAnim` here, and
`vispelleventdriver.cpp` -- which Phase 38 had written against the padded
declaration -- stopped building, which reads as 28 regressions in a class this
phase never touched.  Grep the whole of `src/` for a member name before renaming
it, the same way Phase 36's rule says to grep for an `extern` before adding one.

### Open

`ProcessPlayList` 306/532.  The target SPILLS `this`, the LOD level, the scene
and the play index to the stack at entry (sp+96, +100, +104, +108) where the
candidate keeps three of them in callee-saved registers, and every later
divergence follows from that one allocation difference.  `ProcessPlayTiming` is
14 of 332 -- the register the 64-bit clock difference lands in, twice, and one
`addu` operand order in the trigger fire.  The constructor is 36 of 125: the
target materialises base pointers for BOTH `m_transform` entries where the
candidate folds the frame copy onto `this`, and six source spellings all score
worse.  `Copy` is 183 of 213, `ProcessHierarchy(int, VIMatrix44 &)` 45 of 94 and
`ProcessUpHierarchy` 49 of 93 -- all three are register-allocation, not shape.

Long form: `claude-notes/decomp-phase47-the-hierarchical-sprite-20260827.md`.

## Phase 46 result: the collision core

**Twenty functions written, eight byte-exact, and one of Phase 22's open four
closed on the way past -- nine in all.
2,185/2,418 -> 2,194/2,438.**

Phase 22 settled `VICollide` the *class*.  This is the other twenty symbols in
the same translation unit, which are the collision itself: the six buffer
walkers (ray and swept sphere times three packings), the three query walkers,
the per-primitive tests (triangle, edge, vertex, sphere, capsule, cylinder), the
point-in-triangle test and the capsule entry point.

The semantics were settled from the console FIRST — `go-eqoa-vi/vicoll` is a
bit-exact Go port of every one of these, checked against a live SUPPORT through
the tracer's `call` — so, as in Phases 11/13/18/22, every mismatch is codegen.

Byte-exact: `PointInTriangle` (130 words, first build), `Collide(VICollRay &,
int, tri)` (124), `Cull(VICollSphere &, VIBBox &)` (86), `Cull(VICollRay &,
VIBBox &)` (67 — **open since Phase 22**), `CollideObj(VICollSphere &,
VICapsule &)` (70, first build), `CollideBufferPackV` x2 (241 each),
`QueryBufferV` (167) and `QueryBufferPackV` (226).

### The levers

**A chain of six rejections is one `||`, not six `if`s.**  `Cull(VICollRay &,
VIBBox &)` had been open since Phase 22 at 30/68 with a comment asserting the
shape was settled.  It was not.  Six separate `if (a && b) return 1;` statements
leave the FIRST `return 1` in line and jump to it from the other five; one
`if ((a && b) || (c && d) || ...) return 1; return 0;` puts the single `return 1`
after all twelve compares, which is the target.  Byte-exact, and the swept form
was byte-exact the moment it used the same spelling.

**`n = numVerts; n = n - 2;` is not `n = numVerts - 2`.**  Split into two
statements the `lh` goes straight into the loop counter's register; folded into
the initialiser it lands in a scratch and the subtract reads it.  Twelve of
`QueryBufferV`'s sixteen remaining words, and `CollideBufferPackV` 65 -> 14.

**A skip's statement order decides which of its two loads is issued first.**
Every walker's group-culled path advances two cursors.  Written
`prim += ...m_numPrims;` FIRST the loads come out numVerts-first, which is the
target's; the two ADDs stay vert-then-prim either way.

**Reload assigns stack slots in DECLARATION order, not first-assignment
order.**  `CollideBufferPackV` sat at 10 words wrong, all of them `48(sp)` and
`52(sp)` swapped between `b.m_verts` and the hit accumulator.  Declaring
`char *vp;` before `int hit = 0;` and assigning `vp = b.m_verts;` after the two
early returns — the same code, the declaration moved — is byte-exact.

**An early exit written `goto done` keeps the accumulator; written `return hit`
it folds to a constant.**  A `return hit;` gives gcc a block whose only reaching
definition is the initial 0, so it emits `move v0,zero`; the shared exit has two
and keeps `move v0,s2`.  It matters wherever an exit has to skip code that a
nested `if` cannot express — here the `t < m_hitTime` rejection that must not run
the three edge tests.

**A vector copy followed by in-place component updates keeps the first two
stores and loses the third.**  `hp = r.m_delta; hp.x = hp.x * t + r.m_start.x;`
emits the copy's store for x and y and dead-eliminates z's — the 2/2/1 pattern
at three separate sites.  Through a pointer variable the function is two words
short and everything after shifts.

### Two facts about the source

**`VICapsule` and `VICylinder` have a zeroing default constructor**, and the
zeroing comes out TWICE for the two vectors and ONCE for the two floats: the
members' own `VIVect3()` first, then the body.  Same two-pass signature as
`VIBBox` (Phase 28).

**Two of these functions carry a `VIPlane` local that does not look like a
plane.**  `CollideNoCaps` and the edge collide both have sixteen zeroed bytes
where three vectors would be twelve.  The normalised cross product is `pl.n`,
the negated dot with the base is `pl.d`, and the slab distance the two
rejections test is `pl.Distance(start)` — which is why the target NEGATES and
ADDS where a subtraction would be shorter.  252 -> 212 in one edit.
`VIPlane::Distance` is inline (no symbol of its own) and reads the POINT first.

### What is open, and why it is two phenomena

Five walkers differ by ONE spilled pseudo.  This is not the shape: the
byte-exact `CollideBufferPackV` is the SAME body with only the vertex fetch
substituted.  Under packing 0 the target keeps `b.m_verts` in a callee-saved
register and multiplies by 12 with `mult`+`addu`; ours spills it, and gcc folds
the reload into `mtlo`+`madd`.  One register, and every callee-saved name after
it shifts — which is why 121 of 208 words "differ".

The three point/sphere collides differ in the ENTRY BLOCK's schedule, all three
in the same place: the target issues all six loads before the first subtract and
stores the difference vector before the direction loads.  Permuting the
component statements, pointer variables, compound operators and declaration
position all move it by at most one word.  `Collide(VICollSphere &, VISphere &)`
is the control: hoisting `rad = s.m_radius + v.r` out of the guard — its delay
slot carries it, so it is unconditional — took it 37 -> 23 and changed nothing
else.  This is the load-hoisting shape `decomp-scheduler-rig-20260822.md`
attributes to the R5900 `load` ready-delay, where 2 gives ours and 1 gives the
target's order.

Long form: `claude-notes/decomp-phase46-the-collision-core-20260827.md`.

## Phase 45 result: the socket layer

**The floor under phases 43 and 44 — `SUPPORT 0x004acc90..0x004ae088`, five more
translation units of the same C-compiled-as-C++ plus one function added to a
sixth. 33 named functions written, 32 byte-exact; five file statics written and
not scored (they are `NO-TGT` rows, the same way this tree's other sixteen
unnamed statics have been since phase 39).**

**2,152/2,379 -> 2,185/2,418.**

| TU | span | written | exact | first build |
|---|---|---:|---:|---:|
| `uassert.cpp`   | `0x004acc90..0x004acdb8` | 3  | 3  | 2 |
| `uerror.cpp`    | `0x004acdb8..0x004ace90` | 4  | 4  | 3 |
| `ipaddress.cpp` | `0x004ace90..0x004acf58` | 4  | 4  | 4 |
| `unet.cpp`      | `0x004acf58..0x004ad3f0` | 8  | 8  | 3 |
| `unetutil.cpp`  | `0x004ad3f0` (one added) | 1  | 1  | 1 |
| `usocket.cpp`   | `0x004ad480..0x004ae088` | 14 | 13 | 5 |

The one open function is `usocket_sendto`, and it is **two words of PROLOGUE
SCHEDULE**: `move s2,a2` and `sq ra,112(sp)` in the other order, every other
instruction identical. Nine spellings — a named result, a named length, a named
pointer, the pointer arithmetic hoisted, the parameters reassigned, the call
result compared — all score exactly 2/33, which is what makes it the standing
`sched2` family rather than a source question.

### What the layer is

`ipaddress_t` is **six bytes**, four octets and a `uns16` port, with three
encodings around it: SCE's `sceInetAddress` carries the IPv4 as a host-order word
at +4, so the two converters count DOWN through it (octet 0 is byte 7) and
neither touches the port; the WIRE form is the octets in order followed by the
port **little-endian**, which is not network order. Those six bytes are what
`usocket_sendto` and `usocket_recvfrom` prepend to every datagram, which is why
every size on that path moves by six.

`unet.cpp` is the only PS2-specific file in the library: one EE thread doing
`sceSifMInitRpc`, `sceLibnetInitialize` over a `memalign(64, 0x10200)`, a handler
registration and a blocking `sceLibnetWaitGetAddress` — and the local address it
gets back is cached as a **STRING**, `g_szHostName[128]`. Nothing keeps an
`ipaddress_t`, so `unet_get_local_address` asks for the text and resolves its own
hostname through `sceInetName2Address`. Every failure here prints and **hangs**,
and so do both asserts: `for (;;) ;`, not `abort`, which is why a tripped
`VASSERT` on a console is a freeze.

`usocket_t` is 200 bytes and this file owns eleven — the tag, the sceInet
descriptor, the type, a listen flag and one byte of readiness bits (bit 0
readable, bit 1 writable, each cleared by the operation that fails). The rest
belongs to `VIInitUSocketEx`: for datagrams the port does not call sceInet from
the caller's thread at all, it hands the work to a pair of EE threads living in
the back of the same allocation. `usocket_recv`, the stream path, is the
exception.

**`forward` is a lag-and-loss simulator, shipped in the retail binary.**
`usocket_sendto_addr` drops the packet outright when
`rand() * 100.0 / 2147483648.0 < iDrop`; and when
`iDelay + (int)((float)iJitter * (float)rand() / 2147483648.0)` is non-zero it
does not send to the destination at all. It `alloca`s `iSize + 8`, copies the
payload in, and appends an eight-byte trailer — the real destination's four
octets, its port in NETWORK order, the delay in 32 ms units clamped to 255, and a
zero — and sends the whole thing to a global relay address.
`usocket_recvfrom_addr` strips the same trailer off anything arriving FROM that
address, walking `*piRead` back twice by four. The in-world dump still holds
**64.37.152.90:10164** in that global.

`usocket_select` returns 1 unconditionally. `usocket_accept` asserts
`!"usocket_accept: not implemented"` and then goes on to create a socket anyway,
behind a hang. `uerror.cpp` carries the entire SCE INET error enum as strings —
`sceINETE_TIMEOUT` through `sceINETE_NO_ROUTE`, fifteen entries — and maps
exactly two of them; the default arm still runs the lookup and throws the answer
away.

### The harness lost an instruction on a function that never returns

`trim_padding` finds the last `jr ra` and keeps one word after it, because the
delay slot is part of the function. `_uassert` **has no `jr ra` at all**, so the
fallback rule — strip trailing zero words — ate the delay slot of the `b` that
closes its `for (;;)` and reported a byte-exact function as one instruction
short. `eqdecomp.is_branch` is new and the rule only fires when there is no
`jr ra` in the span; `test_trim_keeps_a_terminating_branchs_delay_slot` pairs it
with the control that the ordinary case still loses every trailing zero.

Three new `SPAN_CAP` entries, all the usual cause: `usocket.cpp` has five file
statics and the symbol map names none of them, so `usocket_is_valid` (three
statics, 536 bytes derived), `usocket_destroy` (one, 296) and `usocket_sendto`
(one, 504) each acquire a span reaching into the next.

### Levers

* **A switch's source order is the OPPOSITE of its test order.**
  `uerror_remap_os_error_code` tests -511 first and lays the -500 body out first.
  Phase 27 had the rule in two pieces — comparisons by VALUE, bodies by SOURCE —
  and this is the first function where they point in opposite directions, so the
  source has `case -500:` above `case -511:`. As an if/else chain it is 6 or 7
  words wrong in *either* direction.
* **The shift align-up wants an unsigned divide AND a fresh local.**
  `(x + 15) / 16 * 16` on a signed `int` brings gcc's round-toward-zero
  correction; cast to `unsigned` and it is `srl`+`sll`. But written in place,
  `((unsigned)g_aStack + 15) / 16 * 16` lets gcc fold the `+ 15` into the
  symbol's relocation ADDEND and the `addiu v0,v0,15` disappears — assigning the
  address to an `unsigned` local first is what keeps it.
* **A constant serving two purposes is materialised once.**
  `unet_startup_async_status` returns **1** when the thread is not yet dormant,
  and it is the same `1` that then goes into `g_iStarted`, emitted into the
  branch's delay slot and reused. `return iStatus`, the more natural reading,
  costs two `move`s.
* **Five struct-field assignments are a permutation with exactly one answer.**
  `unet_startup_async` fills five of `ee_thread_t`'s nine fields and the stores
  come out 16, 12, 4, 8, 20. All 120 orders score: one is 0, the next is 2. The
  order is `stack_size, gp_reg, func, stack, initial_priority`. Sweep it
  mechanically — the whole thing is under four minutes.
* **A two-byte copy is a `memcpy`, not two `char` assignments.** The target's
  shape is `lb`, `lb`, `sb`, `sb` — both loads then both stores, in offset order,
  which is gcc's inline block move. `((char *)&usPort)[0] = p[i];` gives `lbu`,
  reverses the pair and takes a different register class with it.
* **The byte swap needs its mask to order the shifts.** `(x << 8) | (x >> 8)`
  emits `sll` before `srl` whichever way round the `|` is written; the target
  emits `srl` first. `((x << 8) & 0xff00) | (x >> 8)` does it, and so does
  `(x * 256) | (x / 256)`; five other spellings do not.
* **A copy gcc can coalesce away has to be made to CONFLICT.** This is the one
  that took `usocket_sendto_addr` from 31 wrong words to 1. The function needs a
  send pointer that starts as `pData` and becomes the `alloca`'d buffer; written
  `memcpy(pBuffer, pData, iSize); pSend = pBuffer;` gcc puts both in ONE register
  (the initial copy is dead on one path and an identity on the other, so
  `regmove` ties them). The target keeps them apart and pays for it by
  **spilling the socket pointer to the stack** — nine live values into eight
  callee-saved registers. Swapping the two statements so `pSend` takes its new
  value while `pData` is still live makes the ranges conflict, the copy survives,
  and every register in the function falls into place with it.
* **A drop test returns its own comparison.** `if (... < iDrop) return 1;` makes
  gcc materialise the 1 in the branch's delay slot; the target's delay slot is a
  `nop`, because the value it returns *is* the `slt` result.
* **`goto done` is a shape, not a smell.** `usocket_create_internal` has four
  exits into one cleanup, all forward `goto`s, and the first guard is
  `if ((unsigned)iType >= 4) { iResult = -1; goto done; }` — error INLINE, body
  out of line. The `if (iType < 4) { … } else` form branches the other way, adds
  a jump over the else, and swaps which of `pSocket`/`iResult` gets `s0`. All 41
  wrong words went at once.

`usocket.cpp`'s five asserts land on lines **553, 729, 945, 1116 and 1208** —
roughly 1,300 lines for nineteen functions, far more than they fill, so the
original very probably carried the Windows and the PS2 back ends side by side
under an `#ifdef`. That is also the only thing that explains a `usocket_select`
that returns 1 and an `accept` that is a hang.

Not attempted: the `viusocket.cpp` TU immediately after this one, and the same
three bodies phase 44 left — `connection` (101), `drdp` (105), `route` (40) —
plus the twelve `endpoint` functions that read `connection_t` and
`segment_header_t` directly.

Long form: `claude-notes/decomp-phase45-the-socket-layer-20260827.md`.

## Phase 44 result: the DRDP transport layer

**Five more translation units out of the region phase 43 opened: 74 functions
written, 74 byte-exact, 69 of them on the FIRST build. The first phase in the
campaign where everything written matched.**

**2,078/2,305 -> 2,152/2,379.**

| TU | written | exact | first build |
|---|---:|---:|---:|
| `message.cpp`   | 27 | 27 | 22 |
| `endpoint.cpp`  | 23 | 23 | 22 |
| `bandwidth.cpp` | 12 | 12 | 10 |
| `close_set.cpp` | 10 | 10 |  8 |
| `unetutil.cpp`  |  2 |  2 |  0 |

`bandwidth.cpp` and `close_set.cpp` are complete. `message.cpp` is complete.
`endpoint.cpp` is deliberately partial: twelve of its thirty-five functions read
and write `connection_t` and `segment_header_t` fields directly, and those two
layouts are settled in `connection.cpp` and `drdp.cpp`, neither of which is
started. `unetutil.cpp` is the two generic comparators that sit between `unet`
and `usocket` in the map.

**The fields name themselves.** `bandwidth_process` carries four asserts, and
`eqcalls.py` reads their stringized expressions out of the relocated dump:
`pBandwidth->m_iBytesSentSinceLastSample`, `m_iBytesRecvSinceLastSample`,
`TimeSinceLastSample`, `DRDP_INT64_MAX`. That settles the `m_` prefix, the
Hungarian, and the fact that locals are unprefixed CamelCase — for the whole
library, not just this file. `message_get_key_offset`'s assert is
`!"unknown key index"` at `__LINE__` **397**, so `message.cpp` is padded to that
layout (phase 27's lever); the padding is a documented block covering the wire
record field by field, the container wiring, and the two hazards below.

Every `*_get_vnode_offset`, `*_get_key_offset` and `*_get_vlink_offset` function
in this library is a compiled-in table of its own struct's offsets — which means
the layouts are not inferred from the loads and stores, they are stated:
`close_get_key_offset(0)` **is** 24 and `(1)` **is** 40,
`message_get_vlink_offset(i)` **is** `4 + i * 8`, `endpoint_get_key_offset(0)`
**is** 32.

### The layouts

```
bandwidth_t   48   m_fBpsLimit +0 / m_fQueueSize +4 / m_fQueueSizeMax +8 /
                   m_uiLastProcessTime +C / m_iBpsSent +10 / m_iBpsRecv +14 /
                   m_uiLastSampleTime +18 / m_iBytesSentSinceLastSample +20 /
                   m_iBytesRecvSinceLastSample +28
close_key_t   16   id (uns64) +0 / instance (uns32) +8
close_t       56   vnode +0 / vlink +10 / key +18 / time +28 / file +2C / line +30
close_set_t   60   vtree_t +0 / vlist_t +24
message_t     72   "MESS" +0 / m_link[2] +4 / m_pEndpoint +14 / m_pConnection +18 /
                   m_uiSrc +20 / m_uiDst +28 / m_uiSize +30 / m_usSeqnum +34 /
                   m_ucType +36 / m_ucRefnumOffset +37 / m_usFlush +38 /
                   m_uiCreated +3C / m_uiTime +40
endpoint_t   176   "ENDP" +0 / m_vnode +4 / m_pDrdp +14 / m_pAppPtr +18 /
                   m_uiId +20 / m_uiInstance +28 / m_iFlags +2C /
                   m_connections (vtree) +30 / m_arrivals (vlist) +54 /
                   m_closeSet +6C / +A8 / +AC
```

A message's payload is the malloc slack behind the header: `message_get_data`
is `addiu v0,a0,72`, i.e. `this + 1`, and every allocation in the file is
`m_uiSize + sizeof(message_t)`. There is no flexible array and no `m_data[1]` —
the latter would have made `sizeof` 80 on this ABI and every allocation eight
bytes too big.

### The new levers

**A char ARRAY, not a string literal, is what makes a four-byte `memcpy`
unaligned.** Three functions copy a four-byte tag and the target does each with
`lwl`/`lwr` into `swl`/`swr` — alignment 1. From a string literal used in place,
this compiler aligns the constant to a word and emits `lw`/`sw`. Declaring
`static const char MESSAGE_TAG[] = "MESSAGE";` at file scope is the whole
difference, and it is worth four instructions in each of `message_create`,
`message_destroy_buffer` and `endpoint_create`. It is also the only evidence in
either file for how the constants were written.

**The allocation check has no house style; it is per function.** Four functions
here test a `malloc`, and the target lays all four out identically — the failure
block inline at the branch with the `-2` in a delay slot. They do not get there
from the same source:

- `message_create`, `message_create_buffer`, `close_set_create_buffer`:
  `if (!p) return -2;`
- `message_duplicate`: `if (p) { ...; return 0; } return -2;`

Written the other way round, each of them moves the `-2` block to the bottom of
the function and every later word shifts. Two spellings, four sites, measured
one at a time. This is phase 7's if-direction rule reaching the error path, and
nothing observed predicts which a given function wants.

**ONE variable for a difference and its result.** `uns32_cmp_wrap` is
`int diff = *a - *b; if (diff > 0) diff = 1; else if (diff < 0) diff = -1;
return diff;`. With a separate `result` gcc allocates a second pseudo and every
later word shifts — 7 wrong to 0.

**A guard around the whole body beats an early return, when a LATER return
would otherwise move out of line.** `close_set_insert` was 15 words wrong
through both spellings of `if (expired) return 0;` and byte-exact as
`if ((int)(uiExpire - uiNow) >= 0) { ... }` with one `return 0` at the bottom.

**Two pointer stores before the payload read, not after.**
`message_create_buffer` writes `m_pEndpoint` and `m_pConnection` BEFORE the
`buffer_readm_uns8` that fills the payload. Written after — the order the struct
suggests — the two callee-saved registers holding `ppMessage` and `pEndpoint`
swap and twelve words shift. A statement order, visible only as a register
allocation.

**A comparison's operand order survives to the `xor`.**
`endpoint_has_connection` returns `pConnection == vtree_lookup(...)`, not the
reverse: the target's `xor v0,s1,v0` names which operand is first.

**A validity check reuses the first test's result as the return value.**
`endpoint_is_valid` and `endpoint_has_connection` are both
`int r = <first test>; if (r) r = <second test>; return r;` — an early
`return 0` costs a separate `move v0,zero` block the target does not have.

**A walk that caches the next node BEFORE the body.**
`endpoint_connection_get_arrival_count` calls `vlist_get_next` ahead of
`message_get_connection`, which a `for` loop's increment cannot produce because
calls do not reorder.

### What the layer does, and what it ships broken

`bandwidth_t` is a leaky bucket and a sample window in one struct.
`m_fQueueSize` is bytes owed to the wire; `process` drains it at `m_fBpsLimit`
bytes a second and clamps at zero; `get_flush_time` answers "when may I send
again" by dividing the overflow over the cap by the limit;
`get_process_interval_max` returns SECONDS — how long a full queue takes to
drain, and therefore the longest a caller may go between two `process` calls.
The elapsed-time subtraction is unsigned, so a wrapped millisecond clock still
gives the right interval, and the conversion to float is gcc's unsigned
expansion (halve, round, double), not a `cvt.s.w`.

The close set holds a closed connection's `(id, instance)` for exactly
120000 ms, on two containers at once — a tree keyed by the pair for the lookup,
a list keyed by the expiry for the sweep. A record whose two minutes are already
up when it is inserted is dropped rather than inserted. Each record also carries
the FILE and LINE that closed it, and nothing ever reads them back.

An endpoint's "in sync" is a **latch**: `endpoint_is_in_sync` walks the
connections only until they are all in sync once, sets bit 1 of `m_iFlags`, and
never walks again. It answers "has it ever caught up", not "is it caught up
now". `endpoint_find_connection` refuses a pair of software endpoints even when
the tree found the connection. `endpoint_get_instance` is a post-increment on a
counter seeded from `drdp_get_time`, so instance numbers are unique across a
restart with no state kept anywhere.

Three things to know before writing a server against this:

- **`message_create` leaves `m_ucRefnumOffset` and `m_usFlush`
  uninitialised.** The allocation is a plain `malloc`, neither field is
  written, and both go out on the wire in `destroy_buffer`. The ten stores in
  `create` are the complete list.
- **`message_duplicate` copies the source's `m_pEndpoint` and
  `m_pConnection`** — two pointers into structures the copy is not registered
  with. `message_set_connection` re-derives the endpoint from the connection
  rather than taking it as a second argument, precisely so the two cannot be
  left disagreeing; the caller is expected to use it.
- **The tag test reads freed memory on purpose.** `destroy_buffer` writes
  `"NULL"` over the tag BEFORE the `free` so the poison lands while the block
  is still ours, and `endpoint_is_valid` follows its own memcmp with a question
  to the drdp, because a freed block keeps its tag until the allocator reuses
  it.

Long form: `claude-notes/decomp-phase44-the-drdp-transport-layer-20260827.md`.

## Phase 43 result: the DRDP network library

**Six whole translation units, all of them from the part of SUPPORT no phase had
touched: 144 functions written, 137 byte-exact, 98 of them on the FIRST build.**

**1,941/2,156 -> 2,078/2,305.**

| TU | written | exact | first build |
|---|---:|---:|---:|
| `buffer.cpp`  | 80 | 78 | 66 |
| `vtree.cpp`   | 32 | 30 | 19 |
| `vlist.cpp`   | 16 | 14 | 7 |
| `vset.cpp`    |  7 |  7 | 4 |
| `ll.cpp`      |  7 |  7 | 2 |
| `rle.cpp`     |  2 |  1 | 0 |

`SUPPORT` 0x004acb50-0x004c1ca8 is 586 symbols and 86 KB of **C compiled as
C++** — `unet`, `usocket`, `bandwidth`, `close_set`, `connection`, `endpoint`,
`route`, `message`, `drdp` and the containers under them.  It is the biggest
unmatched body left in the module, and it is the layer the whole game protocol
rides on: `buffer_read_packed_uns64` IS the 7-bit varint
`claude-notes/protocol-reference.md` calls `Write7BitEncodedInt64`, and
`run_length_encode`/`decode` ARE the state channel's RLE.

**The library names its own source tree.**  `_uassert`'s second argument is
`__FILE__` — a relocation site the differ masks, and `eqcalls.py` reads string
literals out of the relocated dump, so it comes back readable:
`C:/code-frontiers-gm/drdp/vtree.cpp`, and eight more (`connection`, `drdp`,
`endpoint`, `route`, `route_table`, `message`, `usocket`, `bandwidth`).  Its
THIRD argument is `__LINE__`, an immediate the differ compares, so `vtree.cpp`
is padded to the original's line layout — 215, 283, 331, 365.

Long form, with the wire format, the layouts, the shipped oddities and the five
open functions: `claude-notes/decomp-phase43-the-drdp-network-library-20260827.md`.

## Phase 42 result: the 2D UI layer

**1886/2085 -> 1941/2156.**  Three translation units on the settled toolchain
with no re-tuning: **`VIUI` 22 of 33 written, `VIUIWindow` 26 of 28 written and
`VIUITextScroll` 7 of 7 — 71 written, 55 byte-exact, 38 of them on the FIRST
build.**  This is the layer every menu, panel and HUD element is drawn through,
and it stands entirely on classes earlier phases settled, so almost every miss
was codegen.

`VIUI` is the drawing device: three `VIStringTable`s (the localised words, the
texture-name table and the sound-name table), a `VIFont`, and pointers to the
raster, the sound device and the dictionary.  **Every atlas entry is named by a
ROW of the texture table and a COLUMN that is the current language**, so a blit
is "look the id up in a static table, turn its row into a name, hash the name,
hand the raster the resource".  The two static tables are unnamed file statics
reached only through two free functions, and their shape is read straight off
those functions: **`GetUITexture`'s bound is 252 and its stride 36;
`GetUISound`'s are 17 and 8** — and both **return entry 0 out of range rather
than a null**, so the `if (e == 0) return -1;` in all fourteen callers is dead
code that still has to be written.

`VIUIWindow` is 0x17C bytes: position, centre and size in screen pixels, a
256-byte title, a four-entry palette that `VIUICOLOR` indexes (the three
`9VIUICOLOR` forwarders are all `sll 4; addiu 304; addu this`), and the
open/close animation, whose two floats are the whole state machine — `m_open`
runs 0..1 and `m_dir` is +1.0f while opening and -1.0f while closing.
`VIUITextScroll` derives from it and adds six ints.

Levers.  **A by-value class parameter that is MODIFIED is a separate local in
the source**: gcc gives an invisible-reference parameter no home slot, so
`at.x += d` writes through the caller's object and the argument temporary is
merged with it, where the target copies at entry and makes a fresh temporary per
call (`BlitAbilityIcon` 65 wrong -> 0, on one line plus moving the copy ahead of
the first call).  **A named local for the size in one arm, an inline temporary in
the other** — count the writes of a constructed value: two slots means a named
local, one means a temporary in the argument list (`BlitHorz`/`BlitVert`, 58 -> 0).
**`VIVect2I` needs a zeroing default ctor AND a user copy ctor**, correcting
phase 9: the first because a `VIVect2I r[2]` local emits gcc's array-ctor loop,
the second because six rectangle copies are `lw`/`sw` pairs and not `ldl`/`ldr`
block moves — and neither moves a word in the three older TUs that use the class.
**The statement order inside a loop tail** decided `BlitRect` outright (four
orderings scored 20, 39, 26 and 0; the induction step goes FIRST).  **The ternary
inside the call, not a named float**, is what builds the argument temporary
before the branch (`TickR`/`TickU`/`TickD`, 17 words each).  Phase 24's
offset-first element address in a plain pointer walk, phase 9's
assign-then-modify-in-place on a float chain, phase 14's fresh local for the
intermediate, and phase 21's `lui` order all reappear.  **Phase 16's hoist gets
its missing half: a ZERO store needs no register, so it is never hoisted** —
read that way `VIUITextScroll`'s constructor is plain declaration order.

Shipped: **`VIUIWindow::Text` computes two values and throws both away** — it
row-centres the text and then passes the original `x`, and it fades the colour's
alpha by the open-animation curve and then passes the CALLER's colour, so
neither reaches the screen.  The four-vector `VIUIWindow::Init` calls
`VIFont::GetHeight` and drops the answer.  `VIUI::Init` returns whatever
`VIStringTable::Load` returned rather than 0/-1.  `VIUI::SoundPlay(const char *,
float)` returns 0 on every path including both failures.  `AppendEllipsis`
measures `"..."` with the font and the text with `TextWidth`, which strips the
`|`-escapes, so a string made only of escapes never shrinks.  `CopyAsteriks`
ignores `this`.

Open: the six `DrawStringEx`/`DrawCenteredStringEx` overloads are **14 words
each and ONE instruction apart** — gcc's `cse` merges the address of the saved
font colour, formed by `move_by_pieces`' `copy_addr_to_reg` at the block copy,
with the same address at the final `SetTextColor`; the merged pseudo is then live
across the function and takes a callee-saved register where the target
rematerialises it.  Confirmed in the RTL dumps and **not fixed by `-fno-gcse` or
`-fno-cse-follow-jumps`**, nor by five spellings of the save.  Phase 19's
standing phenomenon with the pass named.  Plus five smaller scheduling residues.

Not attempted: the four frame builders (`VIUIFrameParam` is **64 bytes, eight
`{VIUITexId id; unsigned int flags}` pairs**), the six `BlitBar` overloads,
`ReloadWords`, and `VIUIWindow`'s `Draw`/`TextBox`/`DrawTextInternal` family.

Long form: `claude-notes/decomp-phase42-the-2d-ui-layer-20260827.md`.

## Phase 41 result: the asynchronous loader, the actor and the compressed container

**1857/2052 -> 1886/2085.**  Three whole translation units on the settled
toolchain with no re-tuning: **`VILoader` 21 of 23, `VIActor` 3 of 3 and
`VICSFFile` 5 of 7 — 33 written, 29 byte-exact, 20 of them on the FIRST
build.**

`VILoader` is the engine's **asynchronous resource loader** and everything the
streaming zone loader pulls in goes through it: a `VIPool` of transactions, ONE
`VIFile::ReadAsynch` in flight, and a single bounce buffer they all read into.
`Process` advances at most one transaction per call — if the current one is
reading and its file reports the read complete it hands the buffer to
`VIESFParse` and walks on, then issues the next read if the (possibly new)
current transaction has not started.  `Align` rounds the read out to the file's
own `AsynchAlignment()` at both ends and carries the slack in `m_offset`, so the
object lands somewhere inside a sector-aligned read and `Complete` opens it with
`VIObjFile::OpenObject(m_buffer + m_offset, m_size)`.  A `VIMap<unsigned,int>`
dedups by DictID: a second `Load` of the same resource just bumps a refcount and
returns the existing index.

`VICSFFile` is the **compressed container**.  A `.CSF` is a 40-byte header
(magic `'CESF'`, `m_offset` always 40) followed by N zlib streams of at most
256 KB each; `Decompress` inflates every stream into its own `new char[]` and
opens the whole SET as one `VIFile` scatter/gather **memory** file, which is why
`VIObjFile` reads a `.CSF` byte for byte as it reads a flat `.ESF`.  `Compress`
is the authoring end, at level 9 into `n + n/2` of scratch.

`VIActor` is one placed thing in a scene and it is three functions; its
48-instruction constructor matched byte-exact on the first build, which is what
confirms the whole 160-byte layout, and `CalcObjectToWorld` — one call to
`VIMatrix44::Transform(m_hpr, m_scale, m_location)` — is what says the ROTATION
is at +0x50 and the translation at +0x44 and not the other way round.

**`VIESFParse` is 1064 bytes, not 1072.**  Phase 36 measured it off
`VIBattleMusic::Preload`'s frame, where the parser is a stack local and the `sq`
register saves above it round the number up.  `VILoader` holds one as a MEMBER,
between the `VIObjFile` ending at `+0x258` and the `int` at `+0x680`, and
0x680 − 0x258 is 1064.  A member settles a size a local cannot.

**`VIResourceDir` is a real class that emits nothing.**
`ParseResourceFile`'s mangled name spells `R13VIResourceDir`, so it is not a
typedef; but it derives from `VIArray<VIResourceElem>` and adds no member, so its
synthesized constructor and destructor inline to direct calls of the base's and
no `__13VIResourceDir` symbol exists in the module at all.  An empty derived
class is invisible except in a mangled parameter.

**`new T[n]` for an element WITH a destructor carries a 16-byte cookie.**
`VILoader::Allocate` is the corpus's second real typed array-new and the first
whose element has one: `__builtin_vec_new(n * 200 + 16)`, the count at offset 0,
the elements from +16, and a forward constructor loop counting `n-1` down
through `-1`.  `Clear`'s `delete []` reads that count at `m_files[-16]` and
destroys BACKWARDS.  Phase 27's `VIQueryTri` is the control: no destructor, no
cookie at all.

Levers.  **ONE shared local, not two** — phase 40's second-local rule run
backwards, twice: a single `int f;` at function scope assigned in both blocks of
`Process`, and a single `int i` for both loops of `VICSFFile::Parse`.
**Two guards are one `||`**: `if (i < 0) return -1; if (i >= m_numFiles) return
-1;` is one INSTRUCTION longer than the target, because the first guard's
`return -1` then fails to cross-jump with a later one — and `||`, `!(&&)` and
`|| !(<)` are all byte-exact, so it is the statement count and not the operator.
**Nested guards share ONE return at the bottom**: `Release` written flat has its
early `return 0` inline AND if-converts its last guard to `slti`+`movz`, and
nesting the body fixes both.  **The success path in the `if`**, twice — once so
gcc can reuse a `v0` that already holds zero, once so the `return 0` block lands
ahead of the success block.  **A declaration order decides which store fills a
constructor's delay slot**: `VIFile::MemSegm *segms = 0;` must be declared BEFORE
the `VIObjFile` local.  **A remainder written back into ITSELF** (`if (m) m = a -
m; x += m;`) is what makes an align-up a `movz` where `if (m) x += a - m;`
branches.  **`& ~63u` is `lui 0xffff`+`ori 0xffc0` where `& -64` is one `li`**,
and building the constant is also what pushes the freshly-`new`ed pointer into
the other register.  And **`sw` on a zlib length says `uLongf` is FOUR bytes
here**, not the eight this ABI otherwise gives a `long`.

Shipped: **`VILoader::Load`'s capacity check is inverted** — `size > m_bufSize +
align * 2` where it should be `size + 2*align > m_bufSize`, so a large enough
resource overruns the bounce buffer; `Complete` discards
`VIObjFile::FileType(0xAB4F)`'s answer; `VICSFFile::Compress` closes its input
file twice; `Decompress` leaks the segment array, every block already inflated
and the compressed bounce buffer on every failure path; both
`SetProgressCallback`s return void among classes of `int`s; and `VIActor` never
initialises `m_scale`, which `CalcObjectToWorld` reads on the first call.

Harness: **grep for an existing STUB before adding a class to `vi.h`** —
`viarray.cpp` had modelled `VIResourceElem` as four bare `int`s for phase 24's
instantiation, the real declaration collided with it, and the whole TU then
failed to build, which took 126 `VIArray` symbols out of the score and read as a
regression list spanning nine element types (phase 36's `extern` rule, with a
class).  Two new `SPAN_CAP` entries (`Align` 2704 -> 192,
`Decompress` 3136 -> 620) — both TUs close with the same run of unnamed
`VIMap<int,int>` walkers.  **Do not run `_q.py` while `eqcheck.py` is running**:
they share `decomp/build`, and a parallel run reported 1646/1841 with 200
phantom regressions in classes this phase never touched.

Open: `VILoader::Find` 17/49 and `VILoader::Align` 17/48 are **instruction for
instruction identical** and differ only in which register holds what — twelve
statement orderings, five element-access spellings, a pointer-to-dir, a guarded
do/while and an explicit element pointer all score the same.  `Decompress`
6/155: the target keeps `VIEndian`'s local copy in `s0` across the six header
swaps where ours never gets a hard register.  `Compress` 26/286: the target
reads `destLen` from its stack slot TWICE, once with `lw` and once with `lwu`,
where gcc gives us one `lw` and a `dsll32`/`dsrl32` pair.

Long form: `claude-notes/decomp-phase41-the-async-loader-20260827.md`.

## Phase 56 result: the raster

`src/viraster.cpp` is **104 of `VIRaster`'s 167 functions written and 98
byte-exact**, and the tree moved 2,679/3,049 -> 2,777/3,153.  `VIRaster` is
SUPPORT 0x003FF2E8 .. 0x0040C758, its own TU, and it was the largest class in
the corpus that had never been touched -- 167 functions and 12,384 instructions
against `VIScene`'s 140.  It is the renderer: five resource pools (prim
buffers, colour buffers, surfaces, material palettes, fonts), the material list
with an LRU over the resident ones, the DMA double buffer every draw call
appends to, and the immediate-mode vertex path.  `VIScene` holds one at +0x4BA0.

**The per-frame uniform state is NOT in the object.**  `GetModelView` is two
instructions -- `jr ra` and `lui v0,0x7000` -- and every light, fog and matrix
setter writes an absolute scratchpad address, so the model-view and projection
matrices, the derived matrices, the sun, the ambient term, the one point light,
the fog parameters, the static lighting and the per-light enable bytes all live
in SPR at 0x70000000 where VU1's unpack can reach them.  The class itself is
0x4C54 bytes, and its biggest member by far is the tessellator at +0x40:
`VIRasterTess` is 17,480 bytes, which is why `raster-system.md`'s
"+0x3298 VIEESurfCache" cannot be right -- the surface cache is a FILE-SCOPE
object, handed to `VIEESurfCache::Init` as a `lui`/`addiu` pair.

Levers.  **A field's absolute address is its own constant, not a struct base**:
`SetFog` reads 0x700001F8 and 0x700001FC twice each and gets a `lui`+`ori`
POINTER for each, where one struct base gives one `lui` and two displacements;
`Store2D` is the same on the two enable bytes.  Where a field is touched ONCE
the two spellings are identical, which is why the struct form matched eleven
functions before `SetFog` exposed it.  **An SPR array subscript keeps the
member offset in the DISPLACEMENT** -- `lui v0,0x7000` + `addu a1,a1,v0` +
`sb a2,546(a1)` -- and `EnableAllPointLights` strength-reduces the same
subscript into a decrementing pointer.  **A mem-initialiser list runs in
DECLARATION order, interleaved with the member constructors, where a ctor BODY
runs after all of them**: `VIMaterialPal`'s two zero stores land BEFORE its
`VIArray<int>` member's constructor call, and only the list form produces that
(44 wrong words otherwise).  **Five of these functions return VOID** among a
class of `int`s -- `Frustum`, `Orthographic`, `EnableTessellation`, `SetZBias`
and `SetClipAndLight` have no `move v0,zero` at all.  **Nesting the body decides
whether the two `return 0`s cross-jump**: every reference-counted release wants
`if (--refcount <= 0) { ... }` with ONE bottom return.  **The element POINTER,
not the subscript, when a call sits between**: `int *e = &a.m_data[i]; *e =
f();` computes the address before the call (44 wrong words to 7).  And
**`n * 848` is two operations in the source** -- the target keeps `li 53` /
`mult` / `sll 4`.

Harness: **`VIRaster`'s class body had to MOVE to the bottom of `vi.h`.**  It
had lived at line 1218 as a shape assertion -- method declarations only, no
data -- because a dozen other TUs call its methods; the real class has `VIPool`
and `VIList` members and those templates are declared 1,000 lines below it.
Nothing between the two points calls a `VIRaster` method from an inline body,
so the move is free.  Also: **`build/` had three stale objects** (`sw.o`,
`t2.o`, `t3.o`, left over from old sweeps) whose symbols SHADOWED the real ones
in a straight walk of `build/*.o`, which is why a hand-rolled rescore reported
five fewer matches than `eqcheck.py`; `eqcheck.py` itself is safe because it
enumerates `src/*.cpp`, not the build directory.

Open: `Viewport` 4/59 (a 60-way permutation sweep of its eight assignments
scores 4 on every one, so statement order is not the lever), `CalcMatrices`
3/43, `GetMaterialPalEntry` 8/15, `CreateMaterialPal` 7/70, `CopyMaterialPal`
8/85 and `InitMaterialLRU` 4/54.  NOT written: the whole GS-packet half --
both `Blit`s, `StretchBlit`, `BeginBillboards`, the `End*`/`BlitEnd` GIF-tag
closers, the six `DrawPrimBuffer*VU*` builders, `TessTriangle`, `AddMaterial`,
the five `Upload*`, the four `SetGS*`, `Init`/`Clear`/`BeginScene`/`Flip`,
`SetClearColor`, `CreateDefaultResources`, `DebugResources`, `CopyMaterial`,
the four string drawers, the six `PickPrimBuffer*`, `Cull`/`Pick`/
`PointInTriangle`/`CalcHitColor`, and `VIRasterTess`'s own seven.

Long form: `claude-notes/decomp-phase56-the-raster-20260828.md`.

## Phase 57 result: the raster's DMA half

`src/viraster.cpp` is now **128 of `VIRaster`'s 167 functions written**, 24 of
them this phase and 16 of those byte-exact, and the tree moved
2,777/3,153 -> 2,793/3,177.  Phase 56 wrote the object -- the
pools, the material list, the LRU, the accessors, the light and fog setters.
What it left was the half that talks to the hardware, and this phase takes the
tractable part of it: the three microcode uploads (`UploadRasterMicro`,
`UploadBillboardMicro`, `UploadPreTrans`), the per-material uniform block, the
strip and billboard closers (`EndTriStrip`, `EndTriFan`, `EndBillboards`), the
blit brackets (`BlitBegin`, `BlitEnd`, `Begin2D`), the GS register packets
(`SetFogRegisters`, `SetGSDithering`, `InitGSRegisters`), the skinning palette
(`SetObjectBlends`), the deferred free lists (`ReleaseColorBuffer`,
`ReleaseDeferred`), the four remaining light setters and the DMAC handler.

**The packet vocabulary, once, because it is what every still-open function is
made of**: `m_id = 16` is a DMAtag CNT and `80` a REF; `0x50000002` /
`0x50000004` are VIFcode DIRECT for 2 or 4 quadwords; `0x10000000` is FLUSHE,
`0x13000000` MSCNT, `0x14000000 | RStripOffset` MSCAL, `0x15000000 |
((VIBbRender - VIBbInit) >> 3)` MSCALF -- **only the label DIFFERENCE is used**
-- and `0x6C..` UNPACK V4-32 with NUM in bits 16-23.  `0x1000000000008001` is a
GIF tag (NLOOP 1, EOP, NREG 1) and `14` its A+D REGS field, so
`SetFogRegisters` is one A+D write of FOGCOL (`61`), `SetGSDithering` one of
DTHE (`69`), and `InitGSRegisters` three -- COLCLAMP (`70`), TEST_1 (`71`, `ZTE`/`ZTST`) and
the dither matrix DIMX (`68`, `0x6071243571603524`).  `SetObjectBlends` ships
the skinning palette in blocks of **32 matrices**, which is 128 quadwords --
exactly what one UNPACK's NUM field carries -- and 2,048 bytes per `memcpy`.

**Three corrections to phase 56.**  `VIRasterPointLight` is
`VIVect3 m_position; float m_range; VIColor32F m_color;`, not a `VIVect4` and a
colour: `SetPointLight` assigns the first three components with
`VIVect3::operator=` and the fourth on its own, and hands `VIMatrix44::Mul`,
whose signature takes a `VIVect3`, the position.  The dir-light array at
`0x700001BC` (stride 28) **OVERLAPS** the point-light array at `0x700001C0`
(stride 32), so a frame uses one kind of local light or the other and the dir
array cannot be a member of `VIRasterSPR`.  And `VISurface` **has a
destructor** -- `ReleaseDeferred`'s `delete s` calls `_$_9VISurface(s, 3)`
behind a null check, where without the declaration gcc emits a bare
`jal __builtin_delete` and no check at all.  Six more `VIRaster` statics are
named: `ObjectBlendsBuffer`, `PreTransBuffer`, `PrimBuffer1`, `PrimBuffer2`,
`ColorMatrix` (all VU1 addresses) and **`DBuffer`, the double-buffered GS
register block** -- `SetClearColor` patches RGBAQ at `+384` and `+752`, and
`GetZBufferReg` hands back the ZBUF_1 setting at `+480`.

The levers: **a `volatile` pointer local materialises a hardware address into a
register** (`*(int *)0x1000E010 = 1` gives the assembler's `lui at` macro; the
target loads the whole constant and stores at `0(reg)`) -- **a whole-object
assignment written INLINE keeps its own address pseudo**, which is the only way
to keep `SetDirLight`'s two `mult i,28` alive -- **every SPR field is its own
absolute constant**, so `SetDefaultLights` casts per field rather than going
through the `VIRASTER_SPR->` macro, which would let cse hoist `0x70000000` into
one callee-saved register -- **`VIVect3::Set()`, not three field assignments**,
is what materialises a local's address, and `SetDefaultLights`' second vector
is a **dead local** -- **a named `end` pointer decides which side of a pointer
subtraction carries the `+1`** -- **a named byte count keeps `(n << 4) + 16`
together** -- **`((long *)base)[k]`, not a named `long *`**, for `sd` stores
into a DMA packet -- and **the `lui`s come out in SOURCE order**, which settles
the order of `InitGSRegisters`' two VIFcode stores and closed it outright.

**A constant-size `memcpy` is a CALL only when an argument is a `void *`
VARIABLE.**  Handed a `char *`, a typed pointer, or an inline `(void *)` cast,
gcc 2.95 expands `memcpy(p, src, 2048)` into a block-move loop -- and, when it
cannot prove the alignment, into a *pair* of loops behind a runtime `andi 7`
test.  Only a `void *` local or parameter emits `li a2,2048` and a
`jal memcpy`.  In `SetObjectBlends` it is worth 27 instructions and the whole
stack frame.

Open: `ReleaseDeferred` 14/73 (the second loop is exact; the first has the list
pointer and the iterator in the opposite pair of callee-saved registers, and
seven loop spellings score the same), `SetObjectBlends` 60/91 (same length,
same instructions, `s1`/`s2` swapped), `UploadRasterMicro` 41/60 and
`UploadBillboardMicro` 30/52 (the target needs one more callee-saved register
and schedules around it; a 36-way store-order sweep moves it one word),
`SetFogRegisters` 32/73 and `UploadPerMaterial` 33/90 (gcc hoists component
loads the target leaves in place), `EndBillboards` 31/55, and the four-argument
`DrawPrimBuffer` 11/53 (a comparison TREE with a dead `slti v1,4` in an
annulled delay slot; every case list tried gives an if-chain).

NOT written: `Init` (all three), `Clear`, `SetClearColor`, `BeginScene`,
`Flip`, both `Blit`s, `StretchBlit`, `BeginBillboards`, `AddMaterial`,
`CopyMaterial`, `CreateDefaultResources`, `DebugResources`, the four string
drawers and `CalcCenteredStringScreenRect`, the six `DrawPrimBuffer*VU*`
builders and `TessTriangle`, the six `PickPrimBuffer*`,
`Cull`/`Pick`/`PointInTriangle`/`CalcHitColor`, `SetGSScissoring`,
`SetGSDitherMatrix`, and `VIRasterTess`'s own seven.

Long form: `claude-notes/decomp-phase57-the-rasters-dma-half-20260828.md`.

## Phase 58 result: the ESF parser

`src/viesfparse.cpp` — **73 of `VIESFParse`'s 117 functions written, 59
byte-exact**, and the tree moved 2,808/3,198 -> 2,852/3,250.  This is the
largest class in the corpus that had never been scored: 55,984 bytes, and the
one piece of code that turns an `.ESF`/`.CSF` object tree into live engine
objects.  21 functions had been sketched in an earlier phase and never
baselined; 15 of those were already exact on this phase's first build.

**`eqcalls.py` is what made it cheap.**  Every function here is a straight-line
run of calls into `VIObjFile` and back into itself, so naming the callees off
the resident SUPPORT turns each disassembly into a transcript: `ReadBegin`, a
tag compare, N `Read`s at known displacements, `ReadEnd`.  Where earlier phases
guessed a callee from its argument shape, this one reads the name.

**Three shapes repeat all the way down** — the WRAPPER (`ReadBegin`, check the
tag, hand off to the `*Obj`, forward its result), the WALKER (loop over the
sub-objects, `NumSubObjects()` re-read every iteration) and the ARRAY WALKER
(the same with the count hoisted, because the array being filled was sized from
it).  A wrapper that hands back an INDEX ends `movn`, one that hands back a
STATUS ends `movz`, and which constant the conditional move puts in `v0` is what
tells them apart.

Corrections to the field map: **`VIZone` +0x08 is an eight-byte `long`**
(`m_baseOffset`, handed to the `ReadBegin(offset, …)` overload) and not two
ints; **`VIZoneRoom::m_a3C` is the ACTOR array and `m_a44` the STATIC LIGHTING
array**, one entry per actor; **`VIZoneStaticTableElem` is two
`(unsigned long, unsigned int)` pairs**, not `int[8]`; **`VICSprite::m_body` and
`m_node` are ONE array of ten**.

Levers: an index-returning wrapper is TWO RETURNS and not a clamp · nest the
body under one bottom return to keep a tail guard from if-converting · a
reference into a container is declared BEFORE the call whose argument setup it
shares · ONE shared loop counter, not two · a scratch buffer used in one branch
belongs in that branch's block scope · two identical tails beat one shared tail.

Long form, with the open list: `claude-notes/decomp-phase58-the-esf-parser-20260829.md`.

## Phase 59 result: the sprite leaves

**18 written, 16 byte-exact.  2,852/3,250 → 2,868/3,268.**

The twelve `Parse*SpriteObj` leaves — the dispatcher's whole right-hand side,
the code that turns an ESF tag into a live scene object — plus six more of the
zone parser (`ParseStaticLightingObj`, `ParseMaterialPalObj`, `ParseZoneRoom`,
`ParseZoneFloraSets`, `ParseGroupSpriteMembers`, `ParseZoneResource`).

Eleven of the twelve leaves matched on the first or second build.  A leaf is
always the same six steps — read the id and the bounds, ask the dictionary
whether that id is already live (if so `ReadEnd` and hand its index straight
back), `VIScene::CreateSprite`, `VIDictionary::Add`, initialise, walk the
sub-objects — and only the `VISpriteType` and the `VIResourceType` differ, so
the template is written once and instantiated.

Levers:

* **The order of `m_bsphere.r = radius` and `m_bsphere.c.Zero()` is decided by
  whether the radius is a CONSTANT or a VARIABLE.**  With a literal
  (`VIPointSprite::Init`) the source is `c.Zero(); r = k;`.  With a value loaded
  off the stack the store goes FIRST, so the scheduler fills the load's shadow
  with `sw zero,32(this)` and puts the radius store between the first and second
  zero.  Both spellings still need phase 9's `VISphere *b = &m_bsphere;` so the
  two compound operators read the radius as `12(b)` and not `44(this)`.
* **`VIObjFile` has no `Read(bool &)`.**  A `bool` member is read through an
  `(int &)` cast; `sizeof(bool)` being four on this ABI (phase 28) makes it
  identical to an `int` member.
* **Hand the CALL the member, not the local it was just stored from.**  An
  address-taken out-param may alias the store through the object pointer, so
  `f(j)` after `p->m_x = j;` reloads the stack slot into a second register where
  `f(p->m_x)` re-uses the one cse already has.
* **A store between two calls is a source fact.**  gcc will not move a store
  across a `jal`, so where one lands relative to the calls is where the source
  put it.
* **Two nested objects want two (type, version) pairs.**
* **A dispatcher that returns a result VARIABLE needs `break` to one shared
  `return i`.**  With `return i` inside each case gcc proves the value on every
  path and deletes the variable entirely.

Corrections to the field map: **`VIStreamAudioSprite::m_looping` is at +0x40 and
`m_priority` at +0x44** (off `IsLooping`/`GetPriority`, both a bare `lw`);
**`VIRadialFloraDist` is not `int m_a[7]`** but a `unsigned int` DictID, an
`int` at +0x0C and three floats; **`VIStaticLighting::ShareResources(VIRaster &)`
exists**; **`VIRaster::MaterialPal` hands the pointer back as an `int`**.

Long form, with the open list:
`claude-notes/decomp-phase59-the-sprite-leaves-20260829.md`.

## Phase 60 result: the asset parsers

**16 written, 23 byte-exact.  2,868/3,268 → 2,891/3,284**, and `VIESFParse`
itself 78 → 101 of its 120 functions.

The ASSET half of `VIESFParse` — the code that turns the file's bytes into a
texture, a font, an XM module, an ADPCM wave, a skeleton, a material or a
particle definition — plus the five pixel converters underneath all of it.
Fifteen of the sixteen new functions are byte-exact; eight drafts phases 58/59
left open closed as well, four of them on one edit.

**A jump table's case order can be READ, not guessed.**  The image is
un-relocated, so a switch's table is five zero words and the case→body mapping
has to be inferred from body order — which is SOURCE order, not value order.
The relocated copy in `memory-dumps/inworld-eqoa.live.eememory` has the real
targets, and `eqcalls.py` already prints the table's symbol
(`addr _vt$8VIEnvRay+0x48`); five words out of the dump at that address settled
`ParseSurfaceObj`, `ParseFontObj` and `Convert24` at once.

Levers: a converter's cases `break` to ONE shared `return 0` with
`default: return -1`, so reorg duplicates the `move v0,zero` into each arm's
delay slot · local copies of the walked pointers, and `for (; n > 0; n--)` on the
count parameter itself · a named local for the first component orders a
read-modify-write's two loads · `0x8000u`, because the signed spelling hoists a
register out of the loop · **a copy of the index into a second local forces its
load BEFORE the next call**, which closed all four `ParseCSprite*List` walkers ·
the MEMBER spelling beats a named `VIArray &` here, the counterpoint to phase 52
· a chained assignment stops an FP constant reaching `$f20` · **a dispatcher's
shared `return i` must be ONE statement**, which closed four more · an if/else
whose trivial arm comes FIRST keeps the object's own register for the call in the
other arm · three named `VIVect3 &` row references make a matrix's row bases
loop-invariant · `if (!(x & 1))` is not `if ((x & 1) == 0)` · a second local for a
stack-resident field keeps it in a callee-saved register.

Harness trap: **`mips` is a predefined macro** in ee-gcc 2.95, so `int mips;` is
a parse error four lines from where the name is.

Corrections to the field map: `VICSpriteAnimEntry::m_particle` and `m_trigger`
are FLOATS; `ParseParticleDefinition` has a second overload taking
`(VIObjFile &, VIParticleDefinition &)`; the surface and font parsers' depth code
is a 0..4 index over 4/8/16/24/32 bpp and NOT `VIBitDepth`'s numbering; and both
`VISurface` and `VIRFont` are handed to `VIDictionary::Add` as a `VIResource &`
though they derive from `VIBuffer`.

Open: `ParseAdpcmObj` 140/245 (one callee-saved register short, so the whole
function shifts) and — worth its own phase — **the `return -1` block's POSITION**,
which is now the only thing between four functions and byte-exact
(`ParseObject` 7/230, both `Parse*LODSpriteLevels` 12/72, `ParseHSpriteAttachments`
18/81).

Long form, with the not-written list:
`claude-notes/decomp-phase60-the-asset-parsers-20260829.md`.
