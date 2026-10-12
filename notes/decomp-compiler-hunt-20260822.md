# The compiler hunt — every fork scored, and SN's missing patchlevels are not preserved

**2026-08-22, step 2 of the "can we patch gcc" question.** Step 1 named the passes
(`claude-notes/decomp-which-pass-diverges-20260822.md`); this looked for a *binary* that
already behaves the way the target does, because a found compiler beats a patched one — it
cannot be accused of fitting. **No such binary exists publicly.** What the hunt did produce
is a full-tree score for every compiler on disk, a hard classification of the two forks, an
exact pin on the ProDG release, and the first EE gcc **source** drop.

## Every toolchain, scored on the whole tree

Never done before — Phase 1 ranked candidates on a single 20-instruction function.
Compiler varied, assembler held at the `ps2eeas` from the 114 package, flags
`-O2 -G0 -fno-exceptions`:

| toolchain | fork | tree |
|---|---|---:|
| `ee-gcc2.95.3-107` | SN | **141**/164 |
| `ee-gcc2.95.3-114` | SN | **141**/164 |
| `ee-gcc2.95.2-273a` | SN | 137/164 |
| `ee-gcc2.95.2-274` | SN | 137/164 |
| `ee-gcc2.95.3-136` | SN | 40/164 |
| `ee-gcc2.9-991111`, `-991111a` | SCE | 25/164 |
| `ee-gcc3.2-030210-beta2` | SCE | 25/164 |
| `ee-gcc2.9-991111-01`, `-dtls13010` | SCE | 24/164 |
| `ee-gcc2.96` | SCE | 6/164 |
| `2.9-990721`, `3.2-030926`, `3.2-040921` | SCE | do not build this tree |

## The two forks, and the hypothesis that dies

`cc1plus` announces which fork it is:

* **SN Systems**: `2.95.3 SN BUILD v1.07 for Playstation2` — the 2.95.2 and 2.95.3 builds.
* **SCE / Cygnus GNUPro**: `2.9-ee-990721`, `2.96-ee-001003-1`, `3.2-ee-030926`, with paths
  under `/usr/local/sce/`.

**SCE never shipped a 2.95.x at all** — its line runs 2.9-ee -> 2.96-ee -> 3.2-ee. So the
`decomp-which-pass-diverges` note's closing hypothesis, that the target might be SCE's fork
carrying a different machine description, is **refuted twice**: there is no 2.95 to be, and
every SCE build scores 6-25/164 against SN 2.95.3's 141.

Also confirmed by md5: the `2.96-ee-001003-1` `cc1plus` inside the official SCE package
(`archive.org/download/ps2_sdks/ee001003_1.tgz`) is **byte-identical** to the copy already in
`toolchains/ee-gcc2.96/`. The SCE line is fully covered.

## The assembler pins the ProDG release

The `ps2eeas.exe` in the 114 package has no readable `--version` (it stack-overflows under
wine) but its PE VERSIONINFO does: **`1.9.6.516`**. Against SN's published release notes:

| ProDG | EE compiler | EE assembler |
|---|---|---|
| 2.0 | 2.95.2 **v2.73** | 1.8.19.0 |
| 2.2 | 2.95.3 **v1.13** | **1.9.6.516** |
| 3.0 | 2.95.3 **v1.36** | 1.9.25.758 |

So the settled assembler is **ProDG 2.2's**, which is consistent with the Phase 3 finding
that the target has no post-branch `div` padding (that arrived with 1.9.25.758 in ProDG 3.0).
And since builds **1.07 and 1.14 are codegen-identical on all 164 functions** while 1.36 is
not, the change we need happened somewhere in **v1.15 - v1.35, i.e. ProDG 2.3 through 2.9**.

## The hunt itself — exhausted

* **decompme/compilers** has 13 EE compilers and we have all 13. `ps2_compilers.tar.xz`
  adds only CodeWarrior (`mwcps2`), excluded by mangling. Their Dockerfiles fetch from their
  own release blobs, so there is no upstream provenance to follow.
* **archive.org** holds exactly two ProDG PS2 dumps: `SNSystemsProDGPs2` and the PS2 folder
  of `sn_sys_consoles_2`. Downloaded and checked: ProDG **2.0** (`2.95.2 SN BUILD v2.73a`,
  already on disk) and **3.01** (v1.36, already on disk). Nothing between them.
* **Official SCE SDK 3.0.2** (2004-07-28) ships `gcc-ee-3.2-ee-030926` — already on disk.
* **Every public PS2 decomp project** — `LucasPicoli/god-hand-decomp`, `fmil95/santamonica`,
  `jacobleeharris/tmhc`, `ieee802dot11ac/amp` — uses `ee-gcc2.95.3-136`, `ee-gcc2.96` or
  `3.2`. Nobody has an unlisted patchlevel.

**SN builds 1.08-1.13 and 1.15-1.35 are not publicly preserved.**

## The artifact cannot name its own compiler

gcc 2.95 stamps `.ident "GCC: (GNU) ..."` into `.comment`, which would have settled the
patchlevel outright. `SLUS_207.44`, `NTGUI2.ELF` and **every** decrypted module image
(`CLIENT/SUPPORT/STATION/SETUP/MCERR_decompressed_SNR1.bin`) are stripped of it. Checked,
negative — worth recording so nobody checks again.

## SN's source was mail-order only — which kills step 3 as specified

`GnuSrcPS2.txt`, shipped inside ProDG 3.01:

> A machine-readable copy of the source code ... is available on request from SN Systems
> provided that the End User: a) pays SN Systems' reasonable costs of distributing the source
> code b) provides SN Systems with a shipping address

There was never a download. SN Systems was acquired by Sony in 2005. So the control this
project set for any compiler patch — *build from source and reproduce `ee-gcc2.95.3-107`
byte-for-byte over the tree, 141/164, before believing anything* — **cannot be met**, because
the source that would reproduce it is not obtainable.

## But an EE gcc SOURCE drop does exist, and it makes the knobs readable

`sn_sys_consoles_2/PS2/130src.zip` is a **complete gcc source tree for `2.9-ee-990721`**, 735
files, `config/mips/` included. Wrong fork for matching — but it pairs with a binary we have,
and it turns Phase 8's named cost models into text. Staged as
`decomp/toolchains/src/ee-gcc-2.9-ee-990721-src.zip` (with SCE's 5,540-line `sce-patch` from
the 2.96 package beside it):

| knob | where | value |
|---|---|---|
| `ISSUE_RATE` | `mips.h:3924` | **2** for R5900 — dual issue |
| R5900 `define_function_unit` | `mips.md:559-607` | load **2**/0, store **2**/1, xfer 2/0, arith 1/0, hilo 1/3, imul 4/2, idiv 37/37, fp 3/0, fdiv+fsqrt 6/0 |
| `ADJUST_COST` | `mips.h:3917` | zeroes anti- and output-dependencies |
| `MEMORY_MOVE_COST` | `mips.h:3891` | 4 + `memory_move_secondary_cost` |
| `REGISTER_MOVE_COST` | `mips.h:3859` | 2 GR->GR, 4 GR<->FP, 6 HI/LO, else 12 |

That is the whole surface Phase 8 pointed at: about ten numbers in two files. `ISSUE_RATE 2`
is directly the parameter that decides which of a ready pair issues first, which is the
`sched`/`sched2` divergence; `MEMORY_MOVE_COST` is directly the local-alloc one.

## Where this leaves the question

1. **Accept 141/164 and record the 23 as toolchain-limited.** The split is now *explained*
   rather than open: three named passes, cost models whose shape we can read, and a compiler
   build that was not preserved. This is the honest default.
2. **A weaker step 3 that keeps its control.** Build `2.9-ee-990721` from the source above and
   verify it reproduces the shipped binary byte-for-byte — a control we *can* meet — then use
   it as a rig to prove the mechanism: show that changing `ISSUE_RATE` or the load latency
   flips exactly the `BlendAccum` pair. That confirms the diagnosis. **It cannot close the 23**,
   and a patched 2.9 that happened to match would be a fit, not a finding.
3. **Keep looking for ProDG 2.3-2.9** in places the public archives do not reach — dev-kit HDD
   dumps, prototype leaks, studio archives. Low probability, and the only thing that would
   actually close them.
