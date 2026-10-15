# Phase 62 — decompiling CLIENT (opened 2026-10-15)

CLIENT is the game layer: the network opcode dispatcher and its handlers, entities, combat,
the game UI. SUPPORT closed at phase 61 (3,665 of 3,751), and every port plan (Rust, a
native build of the decomp) and every server/Go-client fix needs CLIENT's semantics, so
the phase starts here. **State at the end of day one: 13 of 5,973 functions byte-exact
(1,728 bytes), 139 named, and the opcode dispatcher written in full** (2,093 instructions;
its compare tree matches the target exactly; 611 words wrong, the rest of the residue
diagnosed below). **Later the same day: 17 of 5,973 exact, and the dispatcher at 17 wrong
words**, all in one case (`0x00B1`, a register-allocation tie measured to the pseudo
below); findings 4-8.

The dispatcher's map, case by case, is `decomp-phase62-dispatcher-survey-20261015.md`.

## How CLIENT is worked

Everything is SUPPORT's tooling with `EQMODULE=CLIENT` in front (merged into `decomp/` the
same day, see the phase 61 note): `_tu.py`, `eqcheck.py --baseline`, `eqcallcheck.py -a`,
`eqrefcheck.py -a`, `eqcalls.py calls`, `eqd.py show`, and
`eqasm.py modules/client/build/<tu>.o <sym>`. Source lives in
`decomp/modules/client/src/` and builds into `modules/client/build/`, so a CLIENT scorer
never collides with a SUPPORT one.

- **The map is renamed as functions are written or called.** `modules/client/symbols.txt`
  starts as `func_<VA>` placeholders. A name taken from a caller's use carries a
  `# ... provisional` hint. Renaming the callees of a written function turns
  `eqcallcheck` into a check that each call lands on the function it was attributed to.
- **There is no reverse-reference tool; build one from the relocation table.**
  `eqdecomp.module_relocations(data)` gives `(site, type, import, target)` with a
  module-relative target. "Who references this string/table/function" is a filter over it.
- **The original tree is `C:/code-frontiers-gm/game/`.** TUs carry
  `#line 1 "C:/code-frontiers-gm/game/<File>.cpp"`, as SUPPORT's DRDP files do for
  `drdp/`. Assert hints in the map name the files: `VIClient.cpp` (code
  0x00BBF0D8–0x00BE3C0F, ~150 KB, 14 asserts on lines 1705..11727), `VIMemStream.cpp`
  (0x00CB8260–0x00CB8AB8), `VIEntity.cpp`, `VIFileStream.cpp`, `VIJournal.cpp`, `VINet.h`,
  `VIKeyDiff.h`, `VIItemList.h`, `client_main.cpp` and a dozen more.
- **A header's static initialiser leaves unreferenced strings in every TU that includes
  it.** gcc 2.95 emits a string literal when it parses an initialiser, even when the
  static array itself is then dropped as unused. CLIENT's database table-name list
  ("EQA.USERS", "CHARACTER_ITEMS", ...) and `VINet.h`'s assert text recur once per TU, so
  they mark TU boundaries in `.rodata`.

## The game is one object: `VIClient`

`client_main` (0x00CC6EB0) builds a single `VIClient` on its stack: 0x50321..0x50330 bytes,
from a frame of 0x50380. It then calls `Init`, loops `while (IsRunning()) Process();`, calls
`Shutdown()`, and returns the exit code at +0x4F12C.

- **The FrontiersForge "singleton" (EE 0x1FAF730) is this object.** Its `VIScene` at
  +0x7590 and entity list at +0x7500 are offsets of this layout.
- **`VIClient : VIClientI`.** RTTI names both. `VIClientI` has no data, so the vptr is at
  +0. The vtable (0x00CDB4C0) has 18 virtuals: the destructor in slot 2, and slots 3-18
  one-line accessors returning sub-objects. `VIUI m_ui` is at +0x4, which is why
  `VIWnd::mUI` points at singleton+4.
- **The constructor's member roll-call is the layout.** In declaration order: VIUI +0x4,
  CLIENT objects +0x90 (`VIJournal`) and +0x190, VIRaster +0x380, VISoundDevice +0x4FE0,
  VICollide +0x6160, VIConsole +0x6390 (VIPad, VIKeyboard), VIDictionary +0x6618, VILoader
  +0x6638, VIWalkControl +0x6DD0, VICameraEffect +0x705C, VIScene +0x7590. About thirty
  CLIENT sub-objects follow, up to VIBattleMusic at +0x4FC00. The full list is in the
  survey §3.1.
- Fields measured so far are declared in `game.h`; `viclient.cpp`'s `CHK`s fail the build
  if an offset moves. Notable ones:
  - +0x58: client state (2..3 → 3 on the character list; Disconnect sets −1).
  - +0x5C: `bool m_quit`.
  - +0x71F8: own entity id.
  - +0x7560/+0x7564: `drdp_t *` and `connection_t *`.
  - +0xC300: level.
  - +0x13B94: `unsigned char HaveCorpse`, named by the assert text.
  - +0x2BB78: the game window.
  - +0x2BD70: an **`unsigned int`**, see below.
  - +0x2EC70: `VIMap<int, VIBuddy>`, with 172-byte values and 192-byte nodes.
  - +0x4F0E8: the disconnect reason.

## Matched, and what decided each

| Function | Bytes | The source fact |
|---|---|---|
| `client_main` | 328 | `int end = (int)_end;` as a NAMED local: inline, `(int)_end - 0x100000` folds into one `%hi/%lo` of `_end-0x100000`, where the target adds `0xFFF00000` at run time (27 → 15). `char *cmd = "";` declared after the `pf` and before the object, so the address sits in `s0` across the constructor (15 → 0) |
| `VIClient::Shutdown`, `IsRunning` | 80, 16 | first build. `IsRunning` is `!m_quit` on a 4-byte `bool` (`xori`) |
| `VIClient::Disconnect` | 416 | three zero stores in ADDRESS order (one of six orders matches) |
| `VIClient::FindEntity` | 56 | first build: every slot visited, so it returns the LAST match (`movz`) |
| `VIClient::ShowMsg` | 60 | `VIUI &ui = m_ui;`: `m_language` loads off `&m_ui`, not `this` (SUPPORT's "a named reference materialises its address") |
| `VIClient::ShowText` | 40 | first build; text is `const unsigned short *`, as SUPPORT's `VIUIWindow::T` casts it |
| `VIMemStream::Read`, `ReadSigned`, `ReadUnsigned` | 136, 72, 136 | first build: success branch first; zigzag; 7-bit varint |
| `VIMemStream::Skip` | 56 | `int avail = m_size - m_rpos;` computed BEFORE the error test |
| `VIMemStream::ReadString` ×2 | 168, 184 | first build, asserts on lines 399 and 422 (`MaxBytes >= sizeof(uint16)`: CLIENT has its own `uint16`) |

## The dispatcher (0x00BD0DC8, `VIClient::ProcessMessage`, name provisional)

It is DRDP message type 0xFB: a `VIMemStream` on the stack, a u16 opcode, one `switch`
with 122 values on 114 bodies, and the inline stream destructor at the exit. The survey
reads every case. Writing it took four findings.

**1. The compare tree was the first thing wrong, and it took a switch skeleton to fix.**
gcc 2.95 lowers a sparse switch to a balanced compare tree. The pivot of each sublist is
the node where `(n + ranges + 1) / 2` runs out, so one extra node shifts every pivot
below the root. With the right 122 values the tree still came out different (second pivot
`0x5E` against the target's `0x60`).

The instrument was a throwaway TU holding only the switch: the same case values, one call
per body, about a quarter of a second per build. It reproduced our tree exactly, so it is
an oracle for tree shape. Searching it found the answer, which matches all 102 tests of the
target's tree:

- **Two EMPTY cases.** One is in `0x00ED..0x00F7`, the other in `0x0778..0x0779`. A
  `case X: break;` leaves no code, because jump optimisation deletes a test whose both arms
  reach the exit. It is still a node in the tree. Which value in each gap is not
  recoverable; the gap's first value is used.
- **`0x07F0`, `0x07F1` and `0x07F3` as SEPARATE cases** with identical bodies. Cross-jumping
  merges the bodies later, but stacked labels would have been grouped into a range node at
  tree time.
- **`0x1250/51/53` and `0x1003/04/05` stacked.**

Method, for the next switch:
- Hold the root with a far dummy case and search one half at a time. A lone half is dense
  and compiles to a jump table.
- Try one value per GAP between existing values, not every value.

**2. An all-ones store is `lui 0xffff` + `ori 0xffff` only into an `unsigned int`.** This is
the phase 39 lever: the group field at +0x2BD70 is unsigned. Its one missing instruction
shifted every case after it (821 → 708).

**3. Inline helpers decide where locals live.**
- The corpse list's two zero stores go through `&m_corpse` (an inline `VIItemList::Clear()`),
  not `this`.
- `0x03D5`'s online byte sits at `sp+224`, in the block-temporary region after the 172-byte
  record. All the other cases' bytes are at `sp+2112+`. An inlined function's locals are
  known to be address-taken when they are laid out, so they get block temporaries; that is
  why the read is an inline `VIMemStream::ReadBool()` (639 → 611).

**4. `0x03D6`'s two dead `&id` stores are an inline reader taking an 8-byte object BY VALUE
(611 → 390).** The target stores `&id` to `sp+48` and `sp+52` around `jal ReadSigned` and
never reads them. The same pair sits before every int read in `ReadBuddy` (now
`operator>>(VIMemStream &, VIPlayerRecord &)`), in the `VIMemStreamArray.h` element loop
(`func_00CB8B40`) and in `func_00BF11D8`, so it is one shared helper. `integrate.c` says
which parameter does it: one whose RTL is a MEM is `store_expr`'d into the inlined
function's simulated argument area, a block temporary, and cse forwards the read. Under
EABI an 8-byte struct of two 4-byte words is BLKmode (its 4-byte alignment is below
DImode's, and MIPS is STRICT_ALIGNMENT), so it lives in memory; a 4-byte wrapper travels in
a register and leaves no store; brace-initialising one adds a `memset` and a copy, while a
CONSTRUCTOR builds it in place, which is the target's two `sw`. So: `struct VIMemStreamInt
{ int *m_in; const int *m_out; VIMemStreamInt(int &v) }` and an inline
`VIMemStream::operator>>(VIMemStreamInt)`, and the case reads `s >> id;`. Both words being
the field's address suits a two-way field description (read into it, write from it); the
names are provisional, the shape is measured.

**5. `+0x2BE50` is an `unsigned int` too (390 → 55)** -- finding 2's lever again: 0x007F
stores -1 there as `lui`+`ori`.

**6. `VIBuddy` is not one class: a 40-byte record with an out-of-line constructor, and a
SYNTHESIZED inline constructor around it (55 → 28).** The function called at 0x03D5's start
(`0x00CB7788`, mapped as `__7VIBuddy`) initialises only bytes 0-0x27 -- `m_id` -1, a
24-byte name zeroed, three words -1 -- exactly what `ReadBuddy` reads. It has a second
caller, `func_00BF0F50`, building a 48-byte record (the same 40 + an int + a bool, read by
`func_00BF11D8`, kept in a map with a 68-byte node). Why it matters to the code: an
argument costing more than 2 is copied into a pseudo before a call (`calls.c`,
`precompute_register_parameters`, on whenever `flag_expensive_optimizations`), and cse then
shares that pseudo with every later `&r` -- our `addiu s2,sp,48` before the constructor.
Inside an INLINED function, `this` is a register; integration substitutes its constant
equivalent, so the call gets a fresh `addiu a0,sp,48` and the dead parameter copy is
deleted. The target is the mix of the two -- fresh at the constructor, one shared pseudo
after -- which is a synthesized inline `VIBuddy::VIBuddy()` calling an out-of-line base
constructor. The same change put the 172-byte copy's source in `a1`, closing the copy's
register swap too. The map now says `__14VIPlayerRecord` and
`__rs__FR11VIMemStreamR14VIPlayerRecord` (it returns the stream, as every reader here does).

**7. `0x0036` reads the old value BEFORE the byte (28 → 17).** The target's `lw s1,17180(s0)`
sits in the delay slot of the byte read's `jal`, and reorg only fills a call's slot from
before the call. `int old = m_pC31C;` moves above `s.Read(&a, 1, 1)`.

**8. The record readers confirm 4 and 6 from outside the dispatcher: four more functions
exact, three on the first build** (`modules/client/src/viplayerrecord.cpp`,
`viclientguild.cpp`).
- `VIPlayerRecord`'s reader is `s >> r.m_id; s.ReadString(r.m_name, 24); s >> r.m_p1C;`
  `s >> r.m_p20; s >> r.m_p24; return s;`, with the by-value int helper: first build.
- Its constructor needs all four `-1` stores BEFORE the `memset`. The first store emitted is
  the last written (phase 16's "the last store of a constant is hoisted").
- The guild's 48-byte record, `VIGuildMember : VIPlayerRecord` (+0x28 an int, +0x2C a
  `bool`), reads as ONE chain: `s >> (VIPlayerRecord &)m >> m.m_rank >> m.m_online;` then
  `return s;`. The second and third reads take the stream the first call RETURNED (`move
  s0,v0`), which is what a chain does. That needed an inline `operator>>(bool &)`: first
  build.
- `VIClientGuild::UpdateMember` (insert-or-update into the map at guild+0x288) keeps the
  `Find` result in TWO callee-saved registers: `s0` for the `== End()` test, a copy in `s2`
  for the update arm. **`const int i = m_members.Find(m.m_id);`** (16 → 0). With `const`,
  g++ expands the call into a temporary and copies it into `i`, where a plain `int` takes
  the call's result directly. cse folds the test's use onto the temporary, but the `else`
  arm is a branch target where that equivalence is not known, so it keeps `i`. Six other
  spellings (helpers with and without the index, a reference, a pointer, `unsigned`, the
  arms swapped) left it at 16 or worse.

**Open: `0x00B1`, `&targets` and `&d` in `s5`/`s3` swapped (17 words).** Measured off the
`-da` dumps: gcse's PRE creates both address pseudos (950 `sp+48`, 951 `sp+2176`, 952
`sp+2180`) ahead of the loop, in expression-index order. Global allocation ranks by
`floor_log2(refs) * refs / live_length`: 950 is 3 refs over 30 insns and 952 is 2 over 20,
both exactly 0.1, so the lower pseudo number wins `s3`. The target gave `&d` the first pick,
so either one live range differs by an insn that is gone by the final code, or the refs
differ. The PRE order cannot be the lever: expression 63 (`sp+48`) is first set by the very
first case's wide-string buffer, which dozens of cases share. Tried and unchanged at 17:
declaration order, `for`/`while`/pointer loops, `targets + i`, `(int)n`, a nested block, a
pointer copy for the call; a pointer used in the loop makes it worse.

## What the notes had wrong (from the survey)

- **`client-mega-function.md` / `mega-function-handlers.md`: "the late handlers fall through
  to each other" is refuted.** Each case ends in its own branch to the exit. What looked like
  fall-through is four cross-jumped tails.
- **Several "state N" / login opcodes are `Disconnect(reason, 0)`.** These are the reasons
  the client disconnects with, which the Go server can use:

  | Opcode(s) | Reason |
  |---|---|
  | `0x0729` | 3 |
  | `0x072B` | 4 |
  | `0x0728` | 6 |
  | `0x072A` | 7 |
  | `0x00EC` / `0x07A4` | 8 |
  | `0x07A1` | 13 |
  | `0x07A2` | 14 |
  | `0x0000` (version not 18) | 16 |
  | `0x07C0` | 17 |
  | `0x07C1` | 18 |
  | `0x07D2` | 19 |

- **The DRDP receive loop routes other message types elsewhere.** It sends 0xF8, 0xF9, 0xFC
  and the state-channel types to other handlers, and handles 0xFD (a disconnect) and 0xFF
  (a new connection) itself.
- **Smaller corrections:**
  - `0x0020` carries the level, not HP.
  - `0x002E` shows word 269.
  - `0x07F0..0x07F4` are character-select messages, not DSP requests.
  - The full list is in the survey §5.

## Files

- `decomp/modules/client/src/game.h`: CLIENT's classes. It was bootstrapped from a
  declaration table and is maintained by hand from here.
- `client_main.cpp`, `vimemstream.cpp`.
- `viclient.cpp`: line-padded. The dispatcher's `VIASSERT(!HaveCorpse)` must stay on
  **line 10306**; edit the blank padding above it when anything before it changes.
