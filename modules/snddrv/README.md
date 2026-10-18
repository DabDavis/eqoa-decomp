# SNDDRV: the IOP sound driver ("VI Sound Driver X", build Apr 23 2003)

`MODULES2\SNDDRV.IRX` off the Frontiers disc: the IOP half of EQOA's sound. SUPPORT's
`VISoundDevice` (`src/visounddevice.cpp`) talks to this module and nothing else. It
calls one SIF RPC server, `0x2112`, with a packet of commands, and gets back a
512-byte status block. Everything the EE asks for, this module does through libsd:
SPU voices, ADPCM uploads, XM tracker modules and the streamed BGM. The BGM's disc
reads go through CDSTREAM.IRX (`vicds`).

**94 of 96 functions are byte-exact (2026-10-18)**, in four TUs:

| TU | range | what | exact |
|---|---|---|---|
| `bgm.cpp` | 0x0000–0x1578 | the BGM stream: its thread, the double-buffered ring, the disc file (`CdOpen`/`CdRead`/`CdSeek`/`CdClose`) | 15/15 |
| `driver.cpp` | 0x1578–0x402C | `_start`, the RPC server and command dispatcher, the status reply, the 100 Hz XM timer thread, every command handler, `gXmSlot[8]`, and the compiler's tail: the XmSlot ctor/dtor, static init, the out-of-line inline accessors, `_GLOBAL_$I$/$D$` | 34/34 |
| `xm.cpp` | 0x402C–0x87F8 | the XM player: `XmVoices` (the voice set of one slot), `XmChannel`, `XmPlayer` (rows, ticks, effects, envelopes, FT2 periods) | 41/42 |
| `module.cpp` | 0x87F8–0x8CB8 | `XmModule`: the relocation that makes a module usable in place | 4/5 |

The import stubs (`0x8CCC`–`0x8F5C`) are named in `symbols.txt` from ps2sdk's
`DECLARE_IMPORT` tables. The tail at `0x8F70`–`0x9188` is libgcc's -O2 runtime
(`__do_global_dtors`, `__do_global_ctors`, `__main`, `__builtin_new`/`delete`/`vec_delete`,
`set_new_handler`, `__default_new_handler`). It is tagged `LIB` and not decompiled.

Checks: `eqcallcheck -a`: 289 real call sites, 0 mismatched (the other 432 "unnamed"
are -O0's `j` to the function's own epilogue labels). `eqrefcheck -a`: 226 reference
sites, 0 findings.

**Open (2):** `XmPlayer::ReadRow` (18 words: in both envelope-enable tests the target
loads the flags into `a0` and keeps `v0` live) and `XmModule::Relocate` (a systematic
two-register shift in loops 1 and 2, `a2`/`a3`/`t0` for `a0`/`a1`). Both are register
choices; no spelling tried moves them.

## The toolchain: none of it is SCE's, and each piece is measured

`toolchain/build.sh` builds the compiler, which is stock **gcc 2.95.3 for mipsel-elf**
(C++), with one target constant changed: `BIGGEST_ALIGNMENT` 32. eqcheck runs it at
`-O0 -fno-weak -fno-exceptions`, and assembles with `toolchain/iop-as`, a model of SN
Systems' IOP assembler on the EE's `ps2eeas`. Every piece was picked by a measured
difference, and each wrong candidate was tried and failed:

- **C++, not C.** Every function ends `j L; nop` three times before its epilogue: the
  return, then cc1plus's cleanup and return labels. C gives one.
- **gcc 2.95, not 2.8.1, egcs 1.0.3a or egcs 1.1.2.** A field of a variably indexed
  array element folds its offset into the symbol (`la $3,gXmSlot+4; addu $2,$3,$2;
  sw $0,0($2)`). 2.8.1 and egcs 1.0.3a give `sw $0,4($2)`. `gBgmBuf[i]` is `la` + `addu`,
  never `lw $5,gBgmBuf($2)`. 2.8.1 needs `-mdebuga` for that, whose only effect is
  that one address rule.
- **`-fno-weak`.** driver.cpp's tail is ctor, dtor,
  `__static_initialization_and_destruction_0`, the inlines in declaration order, then
  `_GLOBAL_$I$/$D$`, all in `.text`. With weak support they go to `.gnu.linkonce.t.*`.
  egcs 1.1.2 emits them in reverse and has no `__static_initialization` function.
- **`-fno-exceptions`**, as on the EE. With exceptions on, the synthesized XmSlot ctor
  carries a cleanup region.
- **`BIGGEST_ALIGNMENT` 32.** A struct local's stack slot is not rounded up to 8.
  `SdInitBgmSystem`'s 20-byte `iop_thread_t` has the next int at +36, not +40. Every
  stock version pads. An R3000 has no 64-bit type.
- **An SN assembler, not GNU as.** `lw $3,sym+4` of an 8-byte `.sdata` object is
  absolute while `lw $2,sym` is `$gp`-relative (`SdSetBgmVolume`, `sBgmVoice`). No GNU
  as from 2.8.1 to 2.4x does that; ps2eeas does. A load followed by a two-instruction
  store macro gets no nop: ps2eeas none, every GNU as one. Delay slots are never
  filled. `move` is `addu` (old GNU as agrees; modern GNU as says `or`). ps2eeas is
  R5900-only (`.set mipsN` is parsed, then rejected or crashes, in every copy we
  have), so `iop-as` rewrites its input for an R3000:
  - `move` becomes `addu`;
  - a load-delay nop goes in when the next instruction reads the loaded register;
  - `div`/`divu` become SN's expansion (divide, result move, the zero check, no
    overflow check).

### What the assembler model gets from the data model

A file static or an uninitialised global is emitted at the END of the file
(`.lcomm`, or `.space` where it stands). SN's assembler does not relax forward
references, so the TU's code reaches those absolutely. An initialised global defined
above its use is `$gp`-relative. That is why `gBgmFd` (a `bgm.cpp` static, `.sbss`) is
`lui`+`lw`, while `sBgmQuit` (`= 0`, `.sdata`) is `-32748($gp)`. An `extern` is always
absolute.

## Working on it

```
modules/snddrv/toolchain/build.sh        # once: toolchains/iop-gcc2.95.3/
EQMODULE=SNDDRV ./_tu.py driver          # one TU, as for every module
EQMODULE=SNDDRV ./_tud.py driver SdRpc   # one TU, and the diffs of what does not match
EQMODULE=SNDDRV ./eqcheck.py             # the module; --baseline rewrites MATCHED.txt
EQMODULE=SNDDRV python3 modules/snddrv/name_tu.py <tu> <start VA>   # name a TU's map entries by order
```

- The image is the `.IRX` itself (`claude-eqoa-iso/decrypted_modules/SNDDRV.IRX`, the
  disc file untouched). `eqdecomp.irx_image` gives it an SNR1-shaped header and
  relocation table on load. VA = `0x000AAA30` + IRX address, where a running game
  holds it (`gSndStatus[0]` = `0xBBCC8` is its `0x11298`).
  `python3 ../re-tools/irxdis.py SNDDRV.IRX` is the annotated listing: import names,
  relocated strings, function starts.
- At -O0 a TU's functions are contiguous and in source order, so `name_tu.py` names
  map entries by order from the TU's start. Run it after adding a function anywhere in
  the TU.
- `_tud.py` builds ONE TU. `eqcheck.py` builds every TU even with `-f`, so never run it
  while another lane is scoring.

## What it does

**The protocol.** `0x80F0 | n` is the only function number. The packet is
`{count, {id, argc, args...}...}`, VISoundDevice's `SoundCmd`. Commands:

| id | handler | args |
|---|---|---|
| 0 | `SdInitSpu` | — (libsd, 100 Hz timer, the XM thread, the BGM system) |
| 1 | `SdAdpcmTransfer` | size, SPU address (from `gIopBuf`, where the EE DMA'd it) |
| 2 | `SdPlayWave` | voice, addr, voice2 (−1 mono), addr2, pitch, volume, pan |
| 3 | — | (a status poll: every call answers with the status) |
| 4 | `SdStopVoices` | lo, hi (KOFF masks) |
| 5 / 6 | `SdSetPitch` / `SdSetVolume` | voice, voice2, pitch / volume, pan |
| 7 | `SdXmLoaded` | module address, sample SPU address (`XmModule::Relocate`) |
| 8 | `SdXmPlay` | lo, hi, module, repeat, pitch, volume, pan |
| 9 / 10 / 11 | `SdXmStop` / `SdXmSetVolume` / `SdXmSetPitch` | lo, hi, ... |
| 12 | `SdFreeSpu` | — |
| 13 / 14 / 15 | `SdPlayBgm` / `SdStopBgm` / `SdSetBgmVolume` | 32-byte name + SPU address / — / volume |
| 99 | `SdTestTone` | addr, size (never sent: a stereo pair on core 0 voices 4/5) |

**The status block** (`gSndStatus`): [0] `gIopBuf`, [1] no transfer running, [2]/[3]
`gXmPool` and its 0x19000 bytes, [4]/[5] the voices with a non-zero envelope on each
core, [6] the BGM file is open, [8..23] each XM slot's {lo, hi} voice mask.

**Voices** are 0–47. libsd's entry is `ch < 24 ? ch << 1 : ((ch - 24) << 1) | 1`.
Volume is a linear pan law on 0..0x3FFF, clamped.

**The BGM** uses core 1 voices 22 and 23 at pitch `0xEB3` (44.1 kHz). Two 128 KB IOP
buffers each take one disc read: 64 KB of left channel, then 64 KB of right. They are
double-buffered into two 128 KB SPU rings at `spuAddr` and `spuAddr + 0x20000`, looped
by flags SNDDRV writes into the ADPCM blocks. A half is refilled when the left voice's
NAX crosses it. A failed read silences both voices (pitch 0) and retries every 33 ms.

**XM** has 8 slots, stepped by a thread the 100 Hz hardware timer wakes. Each wake adds
`tempo * pitch >> 12` to the slot's accumulator, and every 250 of it is one tick, which
is FT2's `BPM * 2/5` Hz. Each `Process()` is one tick. Channel n plays on the n-th set
bit of the slot's lo mask, then of the hi mask. Key-ons and key-offs are collected and
written with one switch per core. Volume, pan and pitch reach libsd as
`xm.cpp`/`xm.h` lay out.

**Shipped bugs, kept as they are:**
- A pattern break ignores its row.
- Unpacked notes are skipped as 6 bytes, not 5.
- `SetOffset` uses the loop TYPE as the loop offset.
- E1/E2 fine portamento run backwards.
- `XmPlayer::Play` always returns −1.
- `CdSeek`/`CdClose` fall off the end (return garbage).

The long form, with the toolchain hunt's refuted candidates, is
`claude-notes/decomp-snddrv-20261018.md`. The host port compiles this same source as
its sound server (`port/README.md`, "Sound").
