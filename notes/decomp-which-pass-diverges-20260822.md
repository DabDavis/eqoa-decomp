# Which PASS diverges — the 23 open decomp functions are THREE phenomena, not one

**2026-08-22.** Asked whether we could patch gcc ourselves to close the 23 functions the
matching decomp still misses. Step 1 of that is naming what to patch, and the answer moves
the problem a long way: **the divergences are made by three different passes, all of them
cost-model driven, and all three cost models are target macros in `config/mips/`.**

This **refutes the standing "ONE phenomenon" reading** recorded in `decomp/README.md`
Phases 4-7 and in `feedback_the_target_does_not_cse_identical_refs`. The functions do share
a *description* — "the target does not keep a value live where ours does" — but they do not
share a cause.

## The method: RTL dumps, with no compiler source

The shipped `cc1plus.exe` accepts **`-da`** under wine. It writes one dump per pass
(`.rtl .jump .cse .addressof .gcse .loop .cse2 .flow .combine .regmove .sched .lreg .greg
.flow2 .sched2 .dbr .mach .jump2`), and gcc 2.95 prints every insn as
`(insn[:MODE] N PREV NEXT (...)`. **That linked list is the emission order.** Follow one
insn's neighbours across the dumps and the pass that reordered it is the pass whose row
changes.

`decomp/eqrtl.py` does this:

```bash
./eqrtl.py dumps src/vimatrix44d.cpp
./eqrtl.py funcs build/rtl/vimatrix44d BlendAccum
./eqrtl.py find  build/rtl/vimatrix44d 'BlendAccum(const VIVect3D' 'reg:DF 2 v0'
./eqrtl.py chain build/rtl/vimatrix44d 'BlendAccum' 25,27,48,50
```

Two traps it had to be built around, both of which produced confidently wrong answers first:

* **The chain runs THROUGH notes.** Walking `next` from insn to insn lands on a
  `NOTE_INSN_*` constantly. A walk that only records real insns stops after one element and
  reports every pass as "these insns are absent" — which reads exactly like a true negative.
* **The dumps do not agree on the function's NAME.** `.rtl` prints the full C++ signature;
  `.gcse` and `.loop` print the bare `;; Function BlendAccum`, so two overloads are
  indistinguishable by name in half the passes. `eqrtl.py` picks the body that actually
  contains the insns asked about, and refuses when two bodies hold them in different orders.
  Insn numbers restart per function, so "the body that has insn 25" is never enough on its own.

## Result 1 — the scheduler: `sched` AND `sched2`, same model, two runs

`BlendAccum__C11VIMatrix44DRC8VIVect3DdR8VIVect4D`, 6 of 152 wrong, all six one adjacent
pair repeated three times. Target:

```
jal   <dpmul>
<delay>
move  s1,v0          <- consume the call's result
ld    a0,0(s2)       <- load the next argument
```

ours emits the `ld` first. `eqrtl.py chain` on that pair:

```
   regmove    25 -> 27 -> 48 -> 50
   sched      25 -> 27 -> 48 -> 50
   lreg       25 -> 27 -> 48 -> 50
   greg       25 -> 27 -> 48 -> 50
   flow2      25 -> 27 -> 48 -> 50      <- still the TARGET's order
>> sched2     25 -> 48 -> 27 -> 50      <- the post-reload scheduler swaps it
```

`__ml__FRC8VIVect4DRC11VIMatrix44D` (18/143) shows the *same* signature —
`sd v0,0(sp)` held next to the call vs. hoisting `ld a0,8(s2)` above it — and the same
trace names a **different pass**:

```
   regmove    130 -> 134 -> 140 -> 142
>> sched      130 -> 140 -> 142 -> 134   <- the PRE-reload scheduler this time
   lreg ... sched2   (unchanged thereafter)
```

One cost model, two invocations; whichever run gets the chance does it.

**The scheduler is not to be turned off.** It is doing the work that earns most of the 141:

| build | tree |
|---|---:|
| `-O2 -G0 -fno-exceptions` | **141**/164 |
| `+ -fno-schedule-insns2` | 15/164 (and BlendAccum gets *worse*, 6 -> 17) |
| `+ -fno-schedule-insns` | 39/164 |

So the target's build ran both schedulers and agreed with ours everywhere except this one
shape. That is a priority/tie-break difference inside haifa-sched, not a pass that was off.

## Result 2 — local register allocation (`lreg`): the six double `Distance`s

`Distance__C8VIVect4DRC8VIVect4D` (62/63) is not a scheduling difference at all —
`-fno-schedule-insns` and `-fno-schedule-insns2` both leave it at 62. The frames:

```
target:      addiu sp,sp,-64     sq ra,48(sp)   sq s0,32(sp)
candidate:   addiu sp,sp,-128    sq ra,112(sp)  sq s4..s0
```

The `.lreg` dump shows five pseudos marked `crosses 3 calls` / `crosses 4 calls` given
callee-saved hard registers; **`.greg` then reports `;; 0 regs to allocate`**, so global
allocation is not involved — local-alloc made the whole decision. The target declined it and
left those values in their own stack slots.

This is the one the old note described literally: *"the target does not keep a value live in
a register where ours does."* It is true, and it is `local-alloc.c`, not CSE.

## Result 3 — loop strength reduction (`loop`): `Transpose__10VIMatrix33`

19 of 34 wrong, unchanged by either scheduler flag. The `.loop` dump prints the decisions
with their costs:

```
Insn 57: giv reg 130 src reg 126 ... mult 4 add 0
giv of insn 57 not worth while, -924 vs 17.
biv 182 can be eliminated.   biv 182 was eliminated.
biv 188 can be eliminated.   biv 188 was eliminated.
```

Ours eliminates two bivs and finishes with two induction pointers; the target keeps three.
Same shape as Phase 4 recorded — now with the pass named and its arithmetic visible.

## Nulls measured on the way

* **`-fno-strict-aliasing` is not it.** It had only ever been swept on one function
  (`sweeps/swap_d.py`, Phase 3). Over the whole tree, both directions: **141/164, unchanged.**
  It was the strongest remaining flag candidate for "does not merge two identical memory
  references".
* **The SN patchlevel is not the dial.** Scoring the whole tree per compiler (assembler held
  at `ps2eeas` from the 114 package):

  | compiler | tree | closes any of the 23 |
  |---|---:|---|
  | 2.95.3 SN v1.07 | 141/164 | — |
  | 2.95.3 SN v1.14 | 141/164 | none |
  | 2.95.3 SN v1.36 | **40**/164 | none |
  | 2.95.2 SN v2.73a | 137/164 | none |

  v2.73a, v1.07 and v1.14 give **identical wrong-counts on all 23** while v1.36 rewrites
  codegen everywhere. A dial that stays fixed while everything around it moves is not the
  dial.
* **`-mcpu=` IS a lever on exactly this class.** Swapping the machine model wrecks the tree
  (`r5000` 64/164, `r4600` 63/164, `r4300` 55/164, `r4000` 42/164) while moving several of
  the 23 *toward* the target: `Distance__C7VIVect4` 32/35 -> **22/34**, all four double
  `Distance`/`DistanceSquared` improve by 1-2, `Invert__11VIMatrix44D` 191 -> 188. `-mcpu`
  selects the function-unit model and the `mips_cpu`-dependent costs, which is precisely the
  input to results 1 and 2.

## What this means for patching gcc

The three passes are driven by three cost models, and all three live in the **machine
description**, not the middle end:

| divergence | pass | knob |
|---|---|---|
| call result vs next argument | `sched` / `sched2` | `define_function_unit` in `mips.md`, `ADJUST_COST` / `ISSUE_RATE` |
| call-crossing pseudo in a callee-saved reg | `lreg` | `MEMORY_MOVE_COST`, `REGISTER_MOVE_COST`, `REG_ALLOC_ORDER` |
| giv/biv elimination | `loop` | `rtx_cost` / `ADDRESS_COST` feeding `loop.c`'s benefit threshold |

That is a bounded, data-like surface in `config/mips/` — which is what makes "patch gcc" a
real option rather than a hack, and it is consistent with the `-mcpu` measurement above.

**It does not say the target is a patched gcc.** Every compiler in `decomp/toolchains/`
announces `SN BUILD` in its version string; SCE's own `ee-gcc` is a different fork and would
plausibly carry a different machine description. Finding that binary beats patching one.

**The control, before any patch is believed:** a gcc built from source must first reproduce
`ee-gcc2.95.3-107` byte-for-byte over the tree — **141/164, same objects**. Without it,
patching stops being reverse engineering and becomes curve-fitting, and the 141 already
matched become unfalsifiable. Acceptance for any change: one principled edit, closes several
of the 23, regresses **none** of the 141 (`eqcheck.py --baseline` enforces the last part).
