# eqperm: a C++ permuter, and what it pinned down about TextBox (2026-10-14/15)

**Status:** tool built and tested, 36/36 tests; the first overnight batch over the other 20 opens is running. `VIUIWindow::TextBox` (the `VIColor32F`
overload) is still OPEN at 32/467 in `src/`. The search narrowed it to two facts about the
target's RTL. Neither is a natural spelling yet, and nothing reached `src/` except a comment.

## Why a permuter

Phase 61 stopped at 3,665/3,751 with 21 named functions open, 863 wrong instructions in
5,409. Every one is a register-allocation or scheduler TIE, and every compiler-knob
hypothesis is refuted on the real cc1plus (`decomp-phase61-finishing-support-20261013.md`).
What is left is search-shaped: many source spellings, each changing a priority or an
insn order by one. Agents tested about one variant per thought. `eqperm.py` tests about 25
per second, unattended.

## The tool (`decomp/eqperm.py`, `decomp/eqperm_mut.py`)

```
./eqperm.py setup SYMBOL [--line N] [--start FILE] [--name RUN]   # -> ../decomp-perm/RUN/
./eqperm.py run RUN -j 7 --anchor 3 --alloc 500 --length 300 --keep 5 --minutes 60
./eqperm.py score RUN FILE [--diff] [--alloc W]     # eqcheck's count + inverted pairs
./eqperm.py reduce RUN FILE                         # ALWAYS before reading a find
./eqperm.py best RUN [-n K]
```

- **Setup.** It finds the definition from the mangled name with tree-sitter (an overload
  is chosen by parameter count, then by class names). It expands the TU's own macros,
  strips comments, preprocesses the prefix once, and dumps the macro set with `-dM`.
  **GATE:** setup refuses unless the unmutated candidate's `.s` is identical, `.file`
  aside, to the driver's build of the TU. The prefix is kept whole because cse hashes a
  SYMBOL_REF by pointer, so earlier code in the TU can change a later function.
- **Build.** `cc1plus.exe` is called directly: 0.12 s against the driver's 0.17 s. A
  non-zero exit counts as a failure, because cc1plus leaves a partial `.s` behind. Then
  `ps2eeas` (0.02 s, with one retry for wine's spurious failures). Worker scratch is on
  `/dev/shm`. Speed: 25-28 builds/s on 7 workers, about 20-23 with `--alloc`.
- **Grade.** The opcode skeletons are aligned with difflib, using decomp-permuter's
  weights: register 5, immediate 5, reorder 60, indel 100. A grade of 0 is re-verified by
  `eqcheck.score`, which also checks the relocations and the callees.
- **`--alloc W`.** It adds `-dl -dg` and reads the last function of `.lreg`/`.greg`
  (refs, live length, allocno order, dispositions). A target register is voted for each
  call-crossing pseudo out of the aligned GPR fields. The score gains `W *` the sum of the
  priority gaps over pairs allocated in the wrong order. That gives the search a gradient
  where the bytes are flat.
- **`--length W`.** It adds `W` per word of instruction-count difference. **Needed:** the
  alignment grade can prefer a SHORTER function that aligns well. Twice the search sat
  for half an hour on `*(buf + n) = 0`, which folds the `+48` into the store's offset:
  graded 255 against 300, but 69/467 by eqcheck against 8.
- **Search.** Simulated annealing at spread temperatures (×1/2/4/8). ANCHORED workers
  (`--anchor`) always restart from the start text with one to three mutations, so the
  neighbourhood keeps being sampled after the others fall into a basin. Elites are kept
  diverse, at most three per score. Stop a run with SIGTERM to the python MAIN pid: killing
  the shell wrapper orphans it.
- **Mutations** (tree-sitter-cpp, text splices): swap, move, temporaries (`decltype`,
  reference and int), inline, commute, negate_if, compound, incdec, split/join declaration,
  add/remove block, pad, subscript, cond, assoc, ternary, sink_tail, hoist_tail, head.
- **`reduce`** greedily reverts each diff hunk the score does not need: singly, then in
  PAIRS, then in adjacent RUNS. An added brace pair around an edited line is two or three
  hunks that only build together. Before the group reverts, a find kept its braces and the
  edit inside them. Test: `test_permuter_reduce_reverts_brace_groups`, with a negative
  control.
- **Measured:** blank lines inside a function do not change its bytes, because line notes
  are inert. Truncating the TU after the target is score-identical.

## TextBox: what the runs found

| run | start | builds | best find (eqcheck) |
|---|---|---|---|
| natural run 3, `--alloc 500`, 60 min | natural (103) | 84,046 | **8/467** once reduced: `buf[n] = 0; i = fit;` in both copy arms of the fit site |
| h11 run 1, 30 min | h11 (8) | 41,721 | none: the 255 was the shorter `*(buf + n)` artefact, 69/467 |
| h11 run 2, `--length 300` | h11 (8) | 13,829 | none, stopped early |
| kwrap run 1, `--anchor 5 --length 300`, 45 min | t_k_wrap (41) | 62,523 | 4/467, deep fakery (`decltype` chains, pads, store and `i = fit` back in the arms) |

Hand-tested with `score` (eqcheck wrong words; the texts are in the session scratchpad):

| spelling | wrong |
|---|---|
| natural: space arm `i = x + 1; line = &s[i];`, fit arm `line = &s[fit]; i = fit;` | 103 |
| `src/` stepping stone: `line = &s[x + 1]` | 32 |
| fit arm `i = fit; line = &s[i];` (mirrors the space arm, NATURAL) | 28 |
| fit arm `i = fit;` before `buf[n] = 0` (no duplication) | 28 |
| **fit arm `{ copy; buf[n] = 0; i = fit; }` in BOTH arms, then `line = &s[fit];`** | **8** |
| only `i = fit` duplicated | 70 |
| only `buf[n] = 0` duplicated | 99 (470 words) |
| the terminator in both arms at ALL five sites | 318 (490 words) |
| `line =` inside the arms too (three orders) | 186 (472 words) |
| `line = &s[i]` once at the wrap join | 128 |
| the whole post-wrap tail duplicated into both wrap arms | 327 (511 words) |
| `k = n * 2;` in each copy arm at the two wrap sites + `*(ushort *)((char *)buf + k) = 0` | 41 |
| the same, plus `i = fit` in the fit arms | **6** |
| `k` form also at the newline / else-top / post-loop site | 6 / 12 / 16 |

## Fact 1: `i`'s extra weight is a cross-jumped copy

The `.lreg`/`.greg` diagnosis gives the numbers. `i` (pseudo 159, 30 refs / 498 insns,
priority 0.2410) loses to the -1 constant (182, 19 / 285, 0.2667) and to `s`. So the
three come out s7/s5/s6 where the target has s5/s6/s7. `i` needs one more in-loop ref.

The search found the mechanism wave 6 predicted. Writing `i = fit` (with the terminator)
into both arms of the copy-if counts twice in flow. jump2 then merges the copies, so the
final code references `i` exactly as often as the target does. **But the target keeps
`i = fit` in the JOIN.** Its fit-arm join schedules `lw s5,592(sp)` between the store's
two address adds, so the real source's extra refs to `i` are elsewhere. That is open.

## Fact 2: the target computes `n * 2` in each wrap arm (read off the dual-issue order)

The `.sched2` dump (`-dR`) prints each block's ready list per cycle and a per-unit table.
The R5900 issues two instructions per cycle (memory, alu, alu, ...). gcc 2.95 runs sched2
BEFORE jump2's cross-jump (already in `decomp/README.md`), so each arm is scheduled in its
own block, call included, before identical endings merge.

The target's space-arm join, after reorg, reads:

```
sll v0,s0,1 | addiu s5,s4,1 ; addu v0,v0,sp ; addiu v0,v0,48 ; sll v1,s5,1 ; sh zero,0(v0) ; b ; addu s3,s7,v1
```

**With `sll v0` in the join block, no schedule gives this order.** `sll v0` and
`addiu s5` are both ready in cycle 1, then `addu v0` and `sll v1` in cycle 2. Our build
issues exactly that and differs. **Without it** (the block starting at `addiu s5`), the
cycles are `{addiu s5, addu v0} {addiu v0, sll v1} {sh, addu s3} {b}`. reorg then moves
`addu s3` into the delay slot, and arm 1's `b` steals `sll v0`. That is the target,
word for word.

The fit arm agrees: `{addu v0, lw s5} {addiu v0, addu s3} {sh}` gives the target's
`addu, lw, addiu, addu, sh`. So at sched2 time both wrap sites had `n * 2` at the END of
each copy arm (later cross-jumped into one `sll`), and the store's two adds, the store,
`i =` and `line =` in the join. `k = n * 2;` per arm reproduces the whole space arm. It
is NOT uniform: at the else-top and post-loop sites the `k` form is worse, so it is not a
shared helper or macro.

**The reading technique is general.** Any open function with a pure ORDER residue can be
read this way. Take the target's order, try the cycle groupings the unit table allows, and
an instruction that would have paired but didn't was in a different sched2 block.

## Two bugs the first batch found, and the batch itself

Setting up all twenty other open functions turned up two faults that TextBox could not
show:

- **SN's cpp writes NOTHING for `-dM`**, to a file or to stdout. Every run's `macros.h`
  was empty, so a function using a header macro (`VIMAX`, `VIMIN`) could not build, and
  setup refused five of the twenty. Now the host's cpp reads the same headers with the
  EE driver's own predefines, read off `-v` (so `mips` is still 1). The names the host
  keeps under `-undef` (`__STDC__` and kin) are dropped. Test:
  `test_permuter_macro_dump_sees_header_macros`.
- **A clean parse was required of every mutant.** tree-sitter cannot read
  `float in[4] __attribute__((aligned(16)));`, so for `operator*(VIVect3, VIMatrix44)`
  every mutation was rejected: 0 built, 0 failed, 0 repeats. A mutant may now not parse
  WORSE than its input.
- **`eqperm.py batch LIST`** runs a list of searches back to back. A list line is
  `SYMBOL [--tu STEM] [--line N] [--name RUN] [--minutes M]`. Afterwards it reduces every
  run's best find, several at once, and skips finds that only TIE the start. It writes
  `../decomp-perm/batch-<stamp>.md`. SIGTERM to its python process stops the current
  search cleanly and ends the batch.

The first batch, `../decomp-perm/overnight-20261015.txt`, started at 01:36 on
2026-10-15. It covers the 20 opens other than TextBox, smallest residue first: 25 minutes
each for the 13 under 40 words and 15 for the 7 larger ones, at `--alloc 500 --length 300
--anchor 3`.

## Open, and next

- A natural source that puts `n * 2` in each wrap copy arm AND gives `i` extra refs with
  `i = fit` in the join. Cross-jumping explains both kinds of evidence, which suggests the
  real structure duplicates more at the wrap sites than ours does.
- Run eqperm on the other 20 opens: `--alloc` for the allocation ties, `--length` with
  anchored workers for the order ties. Overnight runs are cheap.
