# SETUP's ties: 7 of 12 closed, SETUP 1,661 of 1,805; `snprintf16` is void (2026-10-16)

After the screens ([decomp-setup-screens-20261016.md](decomp-setup-screens-20261016.md))
SETUP stood at 1,654 of 1,805 with 12 open functions, all filed as register or scheduling
ties (8 patcher, 4 screens). Seven are now byte-exact, so **SETUP is 1,661 of 1,805**: the
patcher 458 of 462, the screens 250 of 251. None of the seven was a compiler tie. Each one
was a source shape.

One fix is a shared-header change. **Both `VIString::snprintf16` overloads return `void`.**
That closed three SETUP screens and **eight CLIENT functions** that were open for the same
reason. CLIENT's eqcheck count went from 1,128 to 1,136.

Checks, run after the change:
- SUPPORT: 3,665 of 3,751, unchanged.
- MCERR: 44 of 44.
- MATCHED rebaselined for SETUP (1,661) and CLIENT (1,136).
- `eqcallcheck`: 0 mismatched in either module.
- `eqrefcheck`: 0 findings in either module.
- Tool tests: 48 of 48.

## How the shipped scheduler ranks a tie

All seven came down to the order of a few instructions. The shipped `cc1plus` answers
that question directly:
- `-fsched-verbose-9` (with a dash, not `=`) makes `.sched`/`.sched2` print, for every
  insn, its priority, cost and dependents, and the ready list and pick order per cycle.
- Its `rank_for_schedule` was disassembled at `0x4e9df0`. It is the 990721 source's
  function exactly.

The comparison order:
1. **Priority.** The longest latency path to the end of the block.
2. **Register weight (sched1 only).** The weight is the number of SETs minus the number of
   REG_DEAD/REG_UNUSED notes, counted on the notes as they stand BEFORE sched1. That is
   `find_pre_sched_live`, which the shipped binary carries at `0x4e6dd0`.
3. **Class against the last insn scheduled.** Anti and output links always cost 1, so
   class 2 never happens.
4. **The number of dependents.**
5. **sched1's output order.** In sched2 this is the LUID.

Which dependents an argument move picks up:
- every later call, up to the next explicit set of its register;
- the epilogue restore of a callee-saved register it reads;
- the block's last call before the epilogue, which depends on everything;
- a two-insn `lui`/`addiu` next set, which counts twice.

A move from `v0` loses the restore dependent that the moves from `s`-registers get. It
also loses every later call if the call it feeds sets `v0`.

## The seven

| Function | Was | Fix | Why it works |
|---|---|---|---|
| `VIWndController::OnDrawSelf` | 3 | `snprintf16` returns `void` | A `call_value` sets `v0`, which clears the last-use list, so `move a3,v0` lost its later-call dependents and ranked below `move a2,s0`. A plain `call` keeps them. |
| `VIWndOptions::OnDrawSelf` | 2 | the same | the same, for the `t0 ← v0` move |
| `VIWndEula::OnKeyDown` | 3 | the same; its calls use the `const unsigned short *` overload | the same |
| `PatcherAPI_Impl::FindRoots` | 3 | `if (…) sprintf(a) else sprintf(b); return 1;` | With `return 1` in each arm, the arm ends `call; li v0,1; jump`. That jump, followed by a barrier, depends on every live use, which gives the `lb` an extra dependent. |
| `PatcherAPI_Impl::GetEstimate` | 2 | `average = sum / 51;` and `average = sum / (m_estimateIndex + 1);`, one in each arm | The `__divdi3` call is in each arm. In sched2 the result copy ends each arm's block, so `li a0,50` (in the join) cannot pair with it. jump2 merges the identical tails afterwards. |
| `PatcherAPI_Impl::RescanRoutine` | 7 | early return: the empty-names path ends `if (!m_root) return 1; chdir(cwd); return 1;` itself, with no `else` | This changes gcse's PRE copy of `m_root` (`this+36`) and the live lengths. The PRE copy now outranks `copy` for `s4`. |
| `Http_get::Get` | 6 | the member stores in declaration order: `m_mem, m_url, m_x1374, m_dest, m_x137C, …` | Statement order reaches sched1, and sched1 rewrites `REG_LIVE_LENGTH`. Global-alloc's tie (`url` 32 insns, `c` 31) then goes the other way. |

The permuter (`eqperm.py`) found the last three, in 821, 1,189 and 3,634 builds.
`reduce` reverts whole hunks, so each find was narrowed by hand to its natural form.

### The CLIENT functions the `void` closed

These eight now match. With `int snprintf16` all eight are wrong by 2 to 9 words:
- `ConnectStatusString`
- `InviteEntity`
- `Msg0057`
- `Msg0770`
- `Msg0771`
- `PetCommand`
- `QuestStage`
- `RequestTrade`

`PetCommand` had been a permuter target in phase 61. No caller in any module reads the
result. SUPPORT's two definitions are byte-identical either way, since they end in a tail
call. Phase 61 found the same thing for `VIString::snprintf`.

**To apply elsewhere:** when the target copies a call's result first and the candidate
copies it later, check the return type of the NEXT call in the block.

## Open: 5

| Function | Words | What is known |
|---|---|---|
| `Http_get::Get_thread` | 7 | The target is `li v1,4; sw m_status; trn bytes; sd m_bytesDownloaded`. Mine issues the `sd` at t=3: it depends only on the `GetBytes` call. Casting both stores volatile orders `sd` after `sw`, but `trn` then rises above `sw`. The fields are not volatile in the class (that breaks 6 functions). Type variants and statement orders do not move it. |
| `VIWndEula::ComputeTextRows` | 16 | sched1 dual-issues `i = space + 1` at t=3, the only other ready insn. `i*2` is then born right where the `a0` address dies. local-alloc's one-insn overlap gives it `a1`, and the two arms end identically, so jump2 merges them. The `m_numRows` reload depends on the `m_len` store, so it cannot take that slot. The permuter reached 7 words, with unnatural code. |
| `VUrl::OpenSocket` | 24 | jump2 tries the code before the return label first. In mine that is the NULL block's tail, `a0←s0; PrintError; v0←0`, so connect-fail merges into it. The target merges connect-fail with resolve-fail at `sprintf`, so in the target the NULL tail did not match. An `int` `PrintError` makes no difference. Putting the NULL check first gives the target's merge but the wrong layout. |
| `PatcherAPI_Impl::DownloadToMemory` | 25 | Needs two changes at once: `status` (7 loop-weighted refs over 194 insns, priority 721) above `port` (833), AND `server` (857) below `buf` (819). Moving `bytes` and `status` does nothing. No find in 10k permuter builds. |
| `PatcherAPI_Impl::DownloadFile` | 172 | gcse's reaching register for `url` (`sp+5056`) needs `s0`. In mine it loses to the hoisted loop constants (92, 47) and is rematerialized, which shifts everything after it. |

## Method notes

- **Scratch tools live in `~/.cache/setup-ties/`**, not in the repository: `/tmp` is full.
  - `rtl.sh` writes one TU's RTL dumps.
  - `fn.sh` cuts one function out of a dump.
  - `tryfn.py` compiles a function variant and scores it. `--hdr` puts a modified header
    first on the include path, which is how the `void` was tested before `vi.h` was touched.
- **`.greg` prints global-alloc's order** (`;; N regs to allocate: …`).
- **`.lreg` prints each pseudo's references and live length.** Priority is
  `floor_log2(refs) * refs / live_length * 10000`, and references inside a loop count
  double.
