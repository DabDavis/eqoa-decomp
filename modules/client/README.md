# CLIENT: the game (3,849 of 3,934 byte-exact, 2026-10-17)

CLIENT is the game layer of EQOA Frontiers, 1,126,740 bytes of its own code in 5,973 functions,
followed by the usual library tail. It contains:
- the network opcode dispatcher and its handlers;
- the entities and combat;
- the client's sub-objects: bank, auction, crafting, class mastery, guild, mail, ignore and
  who lists, factions, fishing and the alarm;
- 75 game windows (`VIWnd*`);
- the server's own database layer, compiled in and never called (`VICharacter.cpp`).

It exports one name, `client_main`. Every other name in `symbols.txt` is read off the image:
- class names from RTTI strings and vtables;
- methods from their callers;
- field names from FrontiersForge where it has them;
- source file names and line numbers from the assert strings
  (`C:/code-frontiers-gm/game/<File>.cpp`).

Names in comments marked "ours" or "provisional" are guesses.

**State:** 3,849 of the 3,934 scored functions are byte-exact.
- **Duplicates and tiny bodies:** 1,921 more are VERIFIED DUPLICATES (dead per-TU template
  copies that are byte-identical to a matched function, calls and data included), and 112
  are too small to attribute.
- **Bytes:** exact, duplicate and tiny code is 1,008,788 of 1,126,740 bytes.
- **Opens:** 85 functions (4,862 wrong words). Each has its diagnosis in
  `claude-notes/decomp-phase62-polish-20261017.md`.
- **Unwritten:** 5, all dead per-TU copies of `VIItemList.h` members.
- **Checks:** `eqcallcheck` reports 0 mismatched callees and `eqrefcheck` 0 findings.

## How it is laid out

- **The game is ONE object, `VIClient`.** `client_main` builds it on its stack (0x50330
  bytes). Its constructor's member roll-call lays out the whole engine (`VIUI`, `VIRaster`,
  `VIScene`, ...) and about thirty CLIENT sub-objects inside it. It is FrontiersForge's
  "singleton" at EE 0x1FAF730.
- **`src/game.h`** declares the core classes. Each lane's classes live in `src/cl_<lane>.h`,
  included at game.h's end in by-value embedding order.
- **One TU per original file, in address order.**
  - A window TU is its methods, then its template copies, its `__tf`, and the class's inline
    members (a class's inlines are emitted after `__tf` in the TU that emits its vtable).
  - `src/client_vi*.cpp` hold the canonical template copies; every other TU's copies score
    as verified duplicates.
- **`src/viclient.cpp` is VIClient.cpp, and it is LINE-PADDED.** An assert's `__LINE__` is an
  immediate. Connect, ConnectShard, WriteQuantized, SendPosition, Msg000D, ProcessMessage and
  Tick sit on their original lines, with blank padding between them. After Tick nothing is
  line-sensitive. Work collapse -> edit -> repad, and keep each assert on its line.
- **Eight TUs are STATION's source, shared by symlink:** `vidatetime`, `vimemstream{array,ops,
  set,vector}`, `vinet`, `vicsclient`, `vicsrecords`. `vimemstream.cpp` goes the other way. An
  edit there changes BOTH modules, so rescore both.

## Working it

Every tool takes `EQMODULE=CLIENT`:
- `./_tu.py <tu>` builds and scores one TU.
- `./eqcheck.py [--baseline]` scores the whole module, against `src/MATCHED.txt`.
- `./eqcallcheck.py -a` and `./eqrefcheck.py -a` check callees and data references.
- `./eqasm.py modules/client/build/<tu>.o <sym>` shows target and candidate side by side.
- `./eqcallnames.py` proposes names for `func_` rows off our own call sites.
- `./eqperm.py` is the permuter.
- `./_lane.py new|merge <name>` makes a parallel lane.

**Merging lanes:** score each lane's tree and compare it per function against main. A shared
header taken owner-wins can silently break another lane's match.

## The record

- `claude-notes/decomp-phase62-client-20261015.md`: the phase, the dispatcher, the template
  copies, the accessors.
- `claude-notes/decomp-phase62-dispatcher-survey-20261015.md`: the opcode dispatcher case by
  case.
- `claude-notes/decomp-phase62-lanes-20261017.md`: the 18 lanes, 1,208 -> 3,761.
- `claude-notes/decomp-phase62-polish-20261017.md`: the polish pass, 3,761 -> 3,849; the fold
  of VIClient.cpp; the levers; what is open and why.
