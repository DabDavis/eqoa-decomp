# Phase 62 — decompiling CLIENT (opened 2026-10-15)

CLIENT is the game layer: the network opcode dispatcher and its handlers, entities, combat,
the game UI. SUPPORT closed at phase 61 (3,665 of 3,751), and every port plan (Rust, a
native build of the decomp) and every server/Go-client fix needs CLIENT's semantics, so
the phase starts here. **State at the end of day one: 13 of 5,973 functions byte-exact
(1,728 bytes), 139 named, and the opcode dispatcher written in full** (2,093 instructions;
its compare tree matches the target exactly; 611 words wrong, the rest of the residue
diagnosed below).

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

**4. Open: `0x03D6` stores `&id` to `sp+48` and `sp+52` before reading it, and never reads
them back.**
- Emulating the two stores (a diagnostic, not kept) re-aligns the rest of the function:
  real differences fall from ~650 to 188. So these two words are the main remaining source
  fact.
- Inline wrappers taking `id` by reference produce no such stores; neither a read wrapper
  nor a map-remove wrapper does.
- The other open items:
  - `0x03D5`: the record's address reaches the constructor fresh where ours comes from
    `s2`, and the source/temp registers of its 172-byte copy are swapped.
  - `0x00B1`: a two-register tie.
  - Several later cases, to be read once `0x03D6` aligns them.

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
