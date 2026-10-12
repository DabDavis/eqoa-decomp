# The scheduler rig — one number in the machine description flips the divergence

**2026-08-22, steps 3 then 2 of the "can we patch gcc" question.** Step 1 named the passes,
step 2's hunt found no better binary. This is the last two options: keep hunting in places the
public archives do not reach (**failed, and now conclusively**), then build an EE gcc from
source as an experimental rig (**built, controlled, and it identified the parameter**).

## Option 3: the hunt is over

Checked and exhausted, all yielding the same five SN builds (v2.73a, v2.74, v1.07, v1.14,
v1.36):

* every EE compiler in decompme/compilers -- all 13 already on disk;
* every PS2 item on archive.org -- `SNSystemsProDGPs2`, `sn_sys_consoles_2` (PS2 folder, file
  by file), `ps2_sdks`, `ps2-sdk-3.0.2`, `PS2_SDK_CD-ROM_DUMPS` (all four ISOs are 1999-2000,
  ee-gcc 2.9 era);
* the **PlayStation 2 July 2005 SDK (3.0.3)** bundled inside a leaked game-source item, read
  over HTTP range requests without downloading its 974 MB -- its toolchain installer is SCE's
  own, i.e. `3.2-ee`;
* eight public PS2 decomp projects' pinned toolchains;
* AngheloAlf's five mirror repos.

**`sce_ps2_sdk_24` is where v2.74 and v1.14 came from** -- SCE's SDK *bundles* SN's compiler,
which made "find SDK 2.5-2.7, get ProDG 2.3-2.9" the right idea. Those dumps do not exist:
`sce_ps2_sdk_25` through `_30` all return empty metadata.

## Option 2: an EE gcc built from source, with a control

`toolchains/src/build.sh` builds `cc1` for `mips64r5900el-elf` out of
`ee-gcc-2.9-ee-990721-src.zip` plus stock gcc-2.95.3's libiberty. Five fixes, each
load-bearing and each recorded in the script:

1. **the zip is CRLF throughout** -- configure, the shell scripts, and `mips.md`, whose RTL
   reader aborts on it with a truncated-comment error that looks like a corrupt file;
2. **`config.sub` must be the 2.9 one.** Stock 2.95.3's does not know the r5900, and the
   failure is silent: configure falls back to the host, `tm.h` comes out `i386/linux.h`, and
   the whole thing builds -- as an i386 compiler;
3. **`config/host_wint.h` forces a 64-bit `HOST_WIDE_INT` spelled `__int64`**, and
   `machmode.h` then selects the **`"%I64d"`** print format. That is MSVC -- **the shipped EE
   compilers were built on a Windows host.** glibc reads `%I64d` as flag `I` plus width 64, so
   every constant prints inside 64 columns of padding: `sd $19,` then 62 spaces then `48($sp)`.
   A wrong-host build announces itself that way;
4. `include/obstack.h` uses cast-as-lvalue, rejected since gcc 4;
5. **`host_wint.h` is in no dependency list**, so a fix to it silently does not apply until
   `rm *.o`.

The host is built as **i686** (`gcc -m32`) to match the shipped binaries, which are i386 ELF.

### The control

Twelve C cases chosen to exercise the three passes, each at `-O2 -G0`, `-O1 -G0` and
`-O2 -G0 -mcpu=r5000`, our `cc1` against the shipped `ee-gcc2.9-990721` `cc1`:

**33 of 36 instruction-identical.** The three misses are all one function and one mnemonic:
the shipped binary emits `c.olt.s` where this source emits `c.lt.s`. So `130src.zip` is
*near* but not exactly the revision that built the shipped binary -- worth knowing, and it
leaves scheduling and register allocation reproducing exactly, which is what the experiment
needs.

### The rig reproduces the divergence

`corpus/libcall_pair.c` is the `BlendAccum` shape written in C. Baseline:

```
	jal	dpmul
	ld	$4,0($17)      <- delay slot
	ld	$4,8($17)      <- next argument, hoisted ABOVE the result copy
	move	$18,$2         <- the call's result
```

Exactly the 2.95.3 build's behaviour on `BlendAccum`. The target wants `move` then `ld`.

### What flips it

| change | result |
|---|---|
| `ADJUST_COST` stops zeroing **anti-dependence** cost | **no change** -- refuted |
| `alu` unit multiplicity 2 -> 1 | no change |
| `ISSUE_RATE` 2 -> 1 | schedule changes, but not into the target's order |
| **R5900 `load` ready-delay 2 -> 1** (`mips.md:558`) | **exactly the target's order** |

```
	jal	dpmul
	ld	$4,0($17)
	move	$18,$2         <- result copy, glued to the call
	ld	$4,8($17)
```

The mechanism is now plain. A load sits in the `memory` unit at ready-delay **2** while
`move`/`call` sit in a dual `alu` unit at **1**, so the load has the longer downstream path,
wins priority in haifa's ready list, and issues first. At latency 1 it has no advantage and
the natural order survives.

**And it is surgical**: 10 of the 12 corpus files are byte-identical either way. The two that
change are the libcall pair (flipped to the target's shape) and one store move in `transpose`.
It does **not** touch the call-crossing-locals case -- consistent with step 1's finding that
local allocation is a separate cause.

## What this establishes, and what it does not

**Established.** The `sched`/`sched2` family of divergences is decided by the modelled R5900
load latency: **1 gives the target's order, 2 gives ours**, and it is one number in
`config/mips/mips.md`. Step 1's diagnosis is confirmed -- it is the scheduler, driven by the
machine description -- and the parameter is now named rather than guessed.

**Not established.** That SN's v1.15-v1.35 used 1. There is a counter-indication: **v1.36 also
produces our order**, so a change to 1 inside the 1.2x window would have had to be reverted
before 1.36. The honest reading is that the target's compiler models an EE load as
single-cycle and every compiler we have models it as two -- not that we know which build did.

**Cannot close any of the 23.** The rig is 2.9-ee and the decomp is 2.95.3; the two disagree
on 140 of 164 functions for unrelated reasons. Closing them still needs SN's actual build,
which is not preserved, or its source, which was mail-order only. `eqcheck.py` stays at
**141/164** and the 23 remain toolchain-limited -- but they are now explained down to a single
parameter rather than filed as one mysterious phenomenon.

## Reproducing

```bash
cd decomp/toolchains/src
./build.sh /tmp/rig                       # ~5 min
EQ_RIG=/tmp/rig ./exp.sh "baseline" ""
EQ_RIG=/tmp/rig ./exp.sh "latency 1" \
  's|(eq_attr "type" "load") (eq_attr "cpu" "r5900")) 2 0)|(eq_attr "type" "load") (eq_attr "cpu" "r5900")) 1 0)|'
```
