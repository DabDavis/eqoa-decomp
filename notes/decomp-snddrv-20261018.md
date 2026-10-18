# SNDDRV.IRX decompiled (94/96 byte-exact) and running as the port's sound server

2026-10-18. The IOP sound driver, `MODULES2\SNDDRV.IRX` ("VI Sound Driver X", build
Apr 23 2003), is decomp module `SNDDRV` (`decomp/modules/snddrv/`). **94 of 96 functions
are byte-exact.** The native port compiles the same source as its sound server, so
the port has sound: UI effects, the streamed BGM and XM ambience, all through the
decompiled driver.

## Which SNDDRV

The disc has two copies. `MODULES\SNDDRV.IRX` (81,808 bytes) is the 2002 one.
`MODULES2\SNDDRV.IRX` (82,228 bytes, md5 `557c978198b4afc00c8de52ac1dac7af`) is the one
Frontiers loads. SLUS's `0x101478` is the `MODULES2` loader (format string at
`0x15A7C8`), and it is the one passed `"SNDDRV.IRX"`. `0x101380` (`MODULES`) loads
`LIBSD.IRX`.

## The toolchain, and what was refuted on the way

All of it was settled on bgm.cpp (15 functions) and driver.cpp (34) before the XM
player was written. Each row is a measured difference:

| hypothesis | result |
|---|---|
| gcc 2.8.1 C (SCE's IOP compiler of the era) | wrong: C emits ONE `j` before the epilogue, the target THREE. cc1plus's cleanup + return labels give three |
| GNU as -O1 (modern) | wrong: `move` is `or` (target `addu`), and a load before a 2-insn store macro gets a nop the target lacks |
| GNU as 2.8.1 / 2.9.1 / 2.10.1 … 2.15 | wrong: `addu` right, but every one pads before `lui $at`, and `-O0` FILLS delay slots: 2.9.1 tests `arg[1] == '0'`, so `-O0` is full optimisation, and `-O00` is what reaches `mips_optimize = 1` |
| `lw sym+4` of an 8-byte `.sdata` object, absolute in the target | NO GNU as does it (2.8.1–modern all go `$gp`). **SN's ps2eeas does.** That made the assembler SN's |
| ps2eeas switched to MIPS I | impossible: `.set mipsN` is parsed, then "Unexpected text at end of line" (1.9.25) or a wine stack overflow (1.8.19, 2.95.3-114) for every N. Modelled instead: `toolchain/iop-as` |
| gcc 2.8.1 `-mdebuga` | fixed `gBgmBuf[i]` (its only effect is to reject `symbol + reg` addresses), but not the field-offset fold |
| egcs 1.0.3a | like 2.8.1 |
| egcs 1.1.2 | folds `la gXmSlot+4` (expr.c's BLKmode bitpos block), no `-mdebuga` needed, but emits the TU's inline/ctor tail in REVERSE and has no `__static_initialization_and_destruction` function |
| **gcc 2.95.3, `-fno-weak -fno-exceptions`, `BIGGEST_ALIGNMENT` 32** | **right**: the tail order exactly (ctor, dtor, static init, inlines in declaration order, `_GLOBAL_$I$/$D$`). With weak support it goes to `.gnu.linkonce.t.*`; with exceptions on, the ctor carries a cleanup region |
| `BIGGEST_ALIGNMENT` 64 (stock) | wrong in every version: a 20-byte struct local is padded to 24, so the next int is at +40 where the target has +36 |

The XM lane found the TU boundary from the inlines. An in-class inline at -O0 keeps
its argument in a register (`move v0,a0; move v1,a1`); an out-of-line one stores it
(`sw $5,12($fp)`). The target's `0x3E3C` is the inline form. g++ emits an un-inlined
inline once, at the END of each TU that uses it, so one TU runs `0x1578`–`0x402C`: the
driver plus every command handler. driver.cpp and spu.cpp were one file.

## The data model the assembler exposes

With SN's assembler (no relaxation of forward references), the addressing tells
where a variable was defined:
- **initialised, above its use, small** → `$gp`-relative. `sBgmQuit = 0`,
  `sBgmVoice = {0x2D, 0x2F}`.
- **`sym+N` of a `$gp` object** → absolute. `sBgmVoice.right`; SN's rule.
- **file static** → `.lcomm` at the end of the file → absolute, in `.sbss`/`.bss`.
  bgm.cpp's `gBgmFd`, `gBgmBuf[2]`, ...
- **uninitialised non-static global** → `.space` where it stands (g++ 2.95 has no
  common), so in `.data`/`.sdata`. `gCdFile`, `gXmSlot[8]`, `gSdTimer`, `gSndStatus[128]`.
- **`extern`** → absolute. bgm.cpp's view of `gBgmState`, which driver.cpp defines.

That put every global in its TU without a linked image to compare: `.sbss` `0x11248`
onward is bgm's statics, then driver's `gXmThread`, and so on.

## Levers (beyond the XM lane's, which are in `modules/snddrv/README.md`)

- `int n = *p++;` and `n = *p++;` differ. The statement spends a callee-saved register
  on the post-increment temp, as the target does (`SdCommand`).
- A two-way `switch (x) { case K: ...; break; default: ...; }` gives `beq; j default`.
  `if (x == K)` gives `bne`. That is `SdRpc`'s `fno & 0xfff0` test.
- `while (1)`, not `for (;;)`: the target's loop head is `j body; nop; j exit; nop`.
- `while (!f() && x != 6) g();` puts the test in the condition. A `break` in the body is
  a different shape.
- `fold` turns `a - (b + 1)` into `(a - 1) - b`. Spell `a - 1 - b` to get the target's
  `a - (b + 1)` with b loaded first.
- Clamps are `MIN(MAX(x, 0), 0x3FFF)` as ternary macros that fold into
  MAX_EXPR/MIN_EXPR (`bgez; move v0,zero; slti; li`). A nested ternary on `x` does not
  fold.
- libsd's own entry macro is `core | voice << 1 | param`, with core FIRST
  (`SdTestTone`). SNDDRV's handlers use the ternary `SD_VOICE(ch)` instead.
- A search loop that does its work on a match ends with `break`, and the one
  `return 0` after the loop is shared. An in-loop `return 0` adds a `move v0,zero`.

## The harness

- `eqdecomp.irx_image` loads an IRX (MIPS ELF type 0xFF80, no symbols) as an
  SNR1-shaped image: a 0x40-byte header, the sections at their link addresses, ELF
  relocations converted (a HI16 takes its LO16's value). Every existing tool works on
  it unchanged. **An IRX `r_offset` is a module address, not section-relative.**
  Missing that put `.rodata`/`.data` relocations at doubled addresses; the XM lane
  caught it from the jump tables.
- `re-tools/irxdis.py` is the annotated listing. Ghidra headless on the relocated
  image (BinaryLoader, MIPS:LE:32) gives readable C for -O0 code, and was the
  starting point for every function. Its function list must be `jal` targets, not
  `j` targets: both are R_MIPS_26.
- `decomp/_tud.py <tu> <regex>` builds one TU and prints the diffs. `eqcheck -f`
  still builds every TU, so it is unsafe beside another lane.

## On the host

`port/` builds the four TUs as `libsnddrv.so`, at the host's layout (the IOP's `long`
is 4 bytes) with `iop/iopnames.h` renaming the IOP kernel calls. Around it:
- `platform/iop.c`: IOP threads under one lock, the 100 Hz timer, vblank, SIF RPC
  registration, and CDSTREAM over the ISO.
- `platform/libsd.c`: libsd's entry encoding.
- `platform/spu2.c`: 48 voices, ADPCM, the Gaussian table, ADSR and volume sweep, after
  OpenGOAL's ISC-licensed voice; libsd's default routing; no reverb (SNDDRV turns it
  off).
- `platform/audio.c`: SDL2 at 48 kHz, or a clock, plus `VI_AUDIO_WAV`.

`sif.c` routes `0x2112` to the server. Its DMAs copy into the two regions SNDDRV's
status reply names (it is a 32-bit build, so those are host pointers).

Measured:
- `test/snd_bgm` streams `BGM/THEME.BGM`. Against an independent PS-ADPCM decode at the
  same pitch step (`0xEB3/0x1000`), the correlation is 0.91–0.96 in every one-second
  window over 8 s and three ring refills.
- In the game, login → world on Sandstorm: character select's `bgm\erudites.bgm` plays
  (core 1 voices 22/23), UI sounds go out as command 2, and the world loads its
  ambience XM (commands 7, 8, then 10s). The XM player keys voices inside its 16-voice
  mask.

Not yet: a comparison against PCSX2's SPU2 output for the same scene. The ADSR,
interpolation and sweep code follows OpenGOAL's and is not checked against hardware
here.

Two pre-existing port bugs turned up:
- `vi_cd_lookup` read the root directory's place before opening the ISO. The game
  always called `sceCdInit` first, which hid it.
- `vi_iso_read` shared one `FILE` position between threads; it now uses `pread`.
