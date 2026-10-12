# The double `Distance` family is `cse`, not `lreg` — and one line explains all three columns

**2026-08-22, step 4.** Phase 8c named the R5900 load latency as the parameter behind the
`sched` divergence and left `lreg` and `loop` open. This pass went after `lreg` with the same
rig and found that **the attribution was wrong**: the six double `Distance`/`DistanceSquared`
functions diverge in `cse`, the register allocation is the *consequence*, and a single change
— not marking the soft-double libcalls as const calls — reproduces the target's frame, its
register assignment and its instruction order together.

It also **produced a source finding**, which is the part that survives regardless of which
compiler is ever found.

## The source: copy then subtract in place

`src/vivect.cpp` had

```cpp
double d[3];
d[0] = x - v.x;  d[1] = y - v.y;  d[2] = z - v.z;
```

The target opens like this (`Distance__C8VIVect3DRC8VIVect3D`, 45 instructions, 64-byte frame,
**`s0` the only callee-saved register**):

```
addiu sp,sp,-64 | sq ra,48(sp) | sq s0,32(sp) | move s0,a1
ld a2,16(a0) | ld v1,0(a0) | ld v0,8(a0)      <- all three of *this*, up front
ld a1,0(s0)  | move a0,v1
sd v0,8(sp)  | sd a2,16(sp)                   <- spilled into d's own slots
jal          | sd v1,0(sp)
ld a0,8(sp)  | ld a1,8(s0)                    <- and RELOADED before each call
```

The array form cannot produce that under any compiler setting. It loads `a[i]` lazily and so
keeps **both** pointers live across the calls; the target keeps only `v` and lets `this` die
after three loads. Loading all three up front, storing them, and reloading them is the
signature of a **struct copy**:

```cpp
VIVect3D d = *this;
d.x -= v.x;  d.y -= v.y;  d.z -= v.z;
```

`VIVect3D` already has the member-wise copy constructor this needs (Phase 6). Compiled on the
rig with the one change below, this spelling gives **39 of the target's 45 instructions in
order, register for register** — and 2 of the remaining 6 are the prologue's `ra`/`s0` store
order, a 2.9-ee versus 2.95.3 convention. The real residue is **two swapped argument-load
pairs**, 4 positions.

The array form under the same compiler gives neither the frame nor the shape. **It is refuted
as source.**

**The float siblings keep the old form.** `VIVect2::Distance` and `DistanceSquared` are
byte-exact today and the copy spelling *breaks both*, so the float functions really are
`VIVect2 d; d.x = x - v.x;`. Only the doubles were changed.

## The pass: `cse`, and what it forwards

Real-toolchain `-da` dumps (`eqrtl.py dumps src/vivect.cpp`), memory references inside
`double VIVect3D::Distance(const VIVect3D &) const`:

| rtl | cse | loop | cse2 | flow | combine | sched | lreg |
|---|---|---|---|---|---|---|---|
| 30 | **21** | 21 | 21 | 21 | 21 | 21 | 21 |

**Nine loads deleted, all in `cse`.** Each is a reload of a `d[]` slot that `cse` forwards from
the store that filled it. The surviving pseudos then span the `dpsub`/`dpmul`/`dpadd` calls, so
`lreg` must give them callee-saved registers — the rig's `.lreg` dump allocates all nine
pseudos to hard regs 16–20 and `.greg` reports `;; 0 regs to allocate`. **The 112-byte frame
saving `s0`–`s4` is downstream of `cse`, not a decision `lreg` made.** Phase 8's table lists
this family under `lreg`; that row should read `cse`.

## Why `cse` can forward across a libcall at all

From the `.rtl` dump:

```
(call_insn/u 42 ... (call (mem:SI (symbol_ref:SI ("dpsub")) 0)   <- /u = CONST_CALL_P
(call_insn   138 ... (call (mem:SI (symbol_ref:SI ("sqrt"))  0)   <- not const
```

`calls.c` sets `CONST_CALL_P` on every libcall emitted for an arithmetic optab. A const call
does not invalidate memory, so `d[0]`'s value is still available to `cse` after three calls
have run. `sqrt` is not const and is not forwarded across.

## The rig: one line, all three columns

`exp.sh` now takes a fourth argument, a `sed` expression on `calls.c`, and reports a third
column — whether the copy-form proxy reloads `d.y` from its slot (target: yes) or reuses a
forwarded register (ours: no).

| change | sched | alloc | cse-reloads |
|---|---|---|---|
| baseline | `ld,move` | 128B / 5 saved | 0 |
| R5900 `load` ready-delay 2 → 1 | **`move,ld`** | 128B / 5 saved | 0 |
| **libcalls not const** (`if (is_const)` → `if (0)`) | **`move,ld`** | **80B / 2 saved** | **2** |
| both | `move,ld` | 80B / 2 saved | 2 |

*Target wants `move,ld`, a 64B/1-saved frame on the 3-double function, and reloads.*

**The const-call change subsumes Phase 8c.** It produces the target's `sched` order too, and
for a reason that needs no cost model: a load cannot be hoisted above a call that might have
written the memory it reads. Phase 8c's load-latency change reaches the same order by a
different route — and **the two together are worse than the const-call change alone**: on the
full function, const-call alone scores 39/45 and latency-1-plus-const-call scores **32/45**,
because latency 1 pulls an argument load into the delay slot where the target has the result
store. Latency 1 is a mechanism that fits one 4-instruction sample; the const-call change fits
a whole function.

## What this does and does not establish

**Established.** The double `Distance` family's source is copy-then-subtract-in-place; the
divergence is `cse` forwarding a store to a later load of the same stack slot; and a compiler
whose soft-double libcalls are not const reproduces the target's whole function bar four
positions.

**Not established.** That SN's build marked them non-const. This is a mechanism demonstration
of the same kind as Phase 8c's, on a rig that is the wrong fork (2.9-ee against a 2.95.3
decomp). Nothing here closes any of the 23; `eqcheck.py` stays at **141/164**.

**And the "one phenomenon" is now three.** The other two open families were checked with the
same mem-per-pass method and they are *not* this:

| family | pass | evidence |
|---|---|---|
| double `Distance` x6 | **`cse`** | 30 → 21 mems, 9 loads forwarded |
| `operator*` x2 | **`flow`** | 27 → 18 mems: ours deletes the accumulator's intermediate stores as dead, the target keeps them |
| `VIMatrix33::Invert` | **`cse`** | 41 → 39 mems, and **there is no call between the two loads of `w.m[i][i]`**, so const-calls cannot explain it |
| `Transpose()` | `loop` | mem count flat at 4; it is the induction variables (Phase 4) |

`Invert` is the one that resists: same pass as `Distance`, but the const-call lever cannot
reach it. Whatever weakens the target's `cse` there is broader than call const-ness.

**Flags are refuted for this family too.** The 29-flag sweep in Phase 4 was scored against
`Transpose()`. Re-run against the `Distance` filter, thirteen flags — `-fno-gcse`,
`-fno-cse-follow-jumps`, `-fno-cse-skip-blocks`, `-fno-rerun-cse-after-loop`,
`-fno-caller-saves`, `-fcaller-saves`, `-fno-regmove`, `-fno-force-mem`, `-fforce-mem`,
`-fno-defer-pop` and the rest — all give the identical 285 wrong words.
`-fno-expensive-optimizations` gives 281 and `-fno-schedule-insns` 326.

## Reproducing

```bash
cd decomp/toolchains/src
./build.sh /tmp/rig
EQ_RIG=/tmp/rig ./exp.sh "baseline" ""
EQ_RIG=/tmp/rig ./exp.sh "libcalls not const" "" "" 's|  if (is_const)|  if (0) /* EXPERIMENT */|'
```
