# SETUP's screens: 247 of 251 byte-exact; SETUP 1,654 of 1,805 (2026-10-16)

The third and last block of SETUP, after libxml2
([decomp-setup-libxml-20261016.md](decomp-setup-libxml-20261016.md)) and the Verant
patcher ([decomp-setup-patcher-20261016.md](decomp-setup-patcher-20261016.md)): the
program the module runs and its screens, `0x00566C00 .. 0x0057CDD8`.

Every function in it is written: **251 in 18 TUs, of which 247 are byte-exact.** That
brings SETUP to **1,654 of 1,805** (450,284 of 483,568 bytes). What is left of the
module is 12 register or scheduling ties (8 patcher, 4 screens) and the dead template
copies, which `eqcheck` counts apart as verified duplicates (129).

Source: `decomp/modules/setup/src/`, classes in `setup.h`. Scorer:
`EQMODULE=SETUP python3 eqcheck.py`.
- `eqcallcheck`: 7,855 call sites in matched functions, 0 mismatched.
- `eqrefcheck`: 5,541 data references, 0 findings.
- SUPPORT, CLIENT and MCERR re-scored after the two `vi.h` changes below: 3,665/3,751,
  1,128 (+1,951 duplicates) and 44/44, no regressions. Tool tests 48/48.

## The TUs

The link order is one TU per class. Each ends in its template copies, then its `__tf`,
then the class's inline members in declaration order (the TU that emits a vtable
emits the class's inlines). After `main` and `visetup` the `viwnd*` files run in
alphabetical order, which is what told the TU boundaries apart.

| TU | Exact | What |
|---|---|---|
| `main` | 14/14 | `setup_main` and the live `VIMap<int, int>` copy |
| `visetup` | 39/39 | `VISetup`: the program, the patch flow, the card I/O, the patcher callbacks |
| `vifader` | 7/7 | the screen fade |
| `visplashfx` | 7/7 | the title's bird, flock and (Frontiers) island layers |
| `viwindowmanager` | 8/8 | routes keyboard and pad to the focused window, draws the root |
| `viwndbeta` | 16/16 | the betas' "Welcome", still linked; the live `VIArray<VIItemStat>` copy |
| `viwndconnect` | 18/18 | the IRX chain and the ezNetCnf combination |
| `viwndcontroller` | 8/9 | new in Frontiers: "connect an analog controller" |
| `viwnddnas` | 13/13 | DNAS authentication |
| `viwndeula` | 10/12 | the licence, off EULA.WAD |
| `viwndlanguage` | 9/9 | new in Frontiers: the language picker |
| `viwndlegal`, `viwndpresents` | 9/9, 9/9 | 7.5 s splashes; the same code with different art |
| `viwndmessage` | 13/13 | the message box five windows embed |
| `viwndoptions` | 19/20 | options: Game, Interface, Sound pages and their save |
| `viwndpatcher` | 17/17 | the patch screen, its progress and the release notes |
| `viwndreadmessage` | 13/13 | the 64 KB scrolling text page |
| `viwndsplash` | 18/18 | the title: menu, network configuration scan |

Names: the 2002 betas export all of these classes' methods. The beta's link order is
the same, so its names transfer slot by slot where the body changed. Names that are
ours are commented in the source. The three classes Frontiers adds (`VIWndController`,
`VIWndLanguage`, `VIWndPresents`) are named by their RTTI strings.

## What the code does

- **`setup_main` returns which scene SLUS runs next.**
  - 2: the title faded out (go on).
  - 5: restart. A patched SUPPORT or SETUP sets `RestartRequired()`, and the IRX
    modules SETUP loaded (`sIrx[16]`) are stopped and unloaded first.
  - 0: neither.
- **`VISetup` is 0x21AE0 bytes, `new`ed by `setup_main`.** Its constructor builds, in
  order: `PatcherAPI` (the base); `VIRaster`, `VICollide`, `VICamera`, `VIDictionary`,
  `VISoundDevice`, `VIScene`, `VIFont`, `VIUI`, `VIFader`, `VIPad`; two `VIConsolePad`s;
  the window manager; then the nine windows and `VIKeyboard`. The offsets are in
  `setup.h`.
- **The intro is a state machine in `Process`.**
  1. With no analog pad, the controller screen comes first.
  2. Language (behind feature flag `0x400`). A change re-runs `Init(0)`.
  3. Legal, then EULA.
  4. `movies\eqoa.pss`, stoppable with START, X or ESC.
  5. A second `Init(0)`, then the title.
- **The title's "Connect" is `RunPatcher`.**
  1. DNAS authentication (`VIDnas*`). It is retried from the top when its status has
     not moved for 60 seconds ("FORCED TIMEOUT!").
  2. The card's patch images are read (`ReadPatchData`).
  3. `data\psvrpath.dat` supplies the manifest path. The host is hard-coded:
     `patch.everquestonlineadventures.com:7000`.
  4. `Initialize`, `DownloadAll`, `DeleteOldFiles`.
  5. The release notes go to the patcher window, and the changed images go back to
     the card (`WritePatchData`).
  6. It returns 3 (go on), 4 (back to the title) or 5 (failed for good).
- **Modules are kept personalised on the card.** `EncryptRel` runs
  `sceDNAS2InstPersonalizeData_nohdd` over an `.AUT` and stores it as
  `[size][crc][body]` under `.ENC`. `ReadPatchData` maps it back: it keeps the
  recorded size and CRC and leaves the body zero.
- **New STATION and CLIENT images are handed to SLUS.** `WritePatchData` puts them in
  `gRelInfo[3]` and `[4]` (SLUS's module table, 12-byte entries), to load instead of
  the disc's.
- **Three shipped bugs:**
  - `VISetup::OnError` prints `"ERROR CODE %d / '%s'"` with only the text.
  - `EncryptRel` never frees the copy it personalises.
  - `VIFader::Init` never stores its dictionary, so `Free` releases the material
    against whatever `+0x04` held.

## Header and tool changes

- **`vi.h` (SUPPORT's header), two changes, both re-scored module-wide:**
  - `VIUIWindow::Text`'s overloads return `void`. With `int`, the call sets v0, and
    local-alloc's one-instruction lifetime extension keeps the next constant out of
    v0. `void` closed several screens; `viuiwindow`, `viui` and `viuitextscroll` are
    unchanged.
  - `VIGameOptions`: the byte fields the options screen tests are `unsigned char`.
    The setting names are added as comments, layout unchanged. `+0x08` is the effects
    volume and `+0x0C` the music volume, the opposite of their names; they are not
    renamed, because CLIENT uses both.
- **`patcher.h`:** `VIPatchImage` gets an inline destructor (`free(m_data)`). The patcher
  TUs never `delete` one, and score the same.
- **`_lane.py` handles module lanes** (`EQMODULE=SETUP ./_lane.py new <name>`). It copies
  `modules/<m>/src` and `symbols.txt`. `symbols.txt` merges per ADDRESS, so two lanes
  renaming neighbouring rows never collide. Eight lanes ran at once this session.
- **`eqtmpl.py` works on any module.** The candidate TU includes `vi.h` where there is
  no `game.h`.
- **`eqrefcheck.py` and `eqperm.py`:**
  - `eqrefcheck` looks a static up as `<tu>:<name>` first, as `eqcheck` does.
    `sKeyboard` is two different statics: `visetup:sKeyboard` and
    `viwndoptions:sKeyboard`.
  - `eqperm` adds the module's `cflags.txt` include path, so it can preprocess SETUP.

## Levers (the generalisable ones are in memory)

- **An unused member read at the top of a function forms an address base early.**
  `MainLoopBegin` had its `this + 0x8000` base before the first call. Only a member
  access past `0x8000` produces that; the dead load is deleted after cse shares the
  base. Which member it was is not recoverable.
- **Which retry-loop shape matches is visible in the code:**
  - `do { if (ok()) break; …; if (bad()) { give up } } while (1)` keeps the test at
    the top.
  - `for (;;)` with the same body, and `while (!ok())`, come out bottom-tested.
  - The give-up written out at every site is one copy after cross-jumping. A `goto`
    to one shared copy lays the loops out differently.
- **loop.c moves a loop's exit block** to the nearest BARRIER at the target's loop
  depth, searching back from the jump's target (the return label). A trailing
  `fail:` block therefore pulls every inner `return` block to the function's end. With
  none, they go after the first early return at the top.
- **jump.c only turns a branch into a conditional move** when the branch skips exactly
  one register set. `if (r == 3) return -1;` sharing the function's single `return 0`
  with an earlier early-out keeps the branch (`ValidatePatchMemCard`).
- **A 16-byte local copy as `*(long long *)&a = *(long long *)&b` is an `lq`/`sq`
  pair**, because `long long` is 128-bit on the EE. `memcpy` and struct assignment
  give `ldl`/`sdl`. This took `RunPatcher` from 205 words to 32.
- **One shared `int i` for every counted loop in a function** changes the whole
  allocation (`RunPatcher` 311 → 205). Separate `for (int i …)` scopes do the same for
  `setup_main`'s two loops: one counter is a caller-saved temp, the other lives
  across calls.
- **Per-arm calls rather than a `next` pointer.** Each arm calling `SetRoot`/`SetFocus`
  itself, with cross-jumping sharing the tail, fixed `Process`'s register allocation
  from 104 words to 0.
- **A `goto` restart, not a loop, when the constants are not hoisted.** `setup_main`'s
  `Process`/`RunPatcher` loop kept `li 3` and `li 4` inside the loop.
- **An inline accessor reads off the member's address as a value:**
  - `VIFader::Alpha()`: `lwc1 8(&m_fader)`, not `this + 0x8000 + …`;
  - `PadAnalog(VIPad &)`;
  - `VIUI &ui = m_ui` in the ambient-track pair.
- **Reuse the parameter.** `OnTick` assigns the percentage back to `n`, and computes
  `GetTime() - sTickTime` into its own local before the `||`.
- Smaller ones, mostly from the lanes:
  - `x ? a : b` puts the THEN arm in the destination first (`movz` vs `movn`).
  - A destructor that `jal`s with the vptr store in its delay slot, before the member
    destructors, calls that member itself.
  - An unused local array still takes its frame slot.
  - `(x + (n - 1)) % n` is not `(x - 1 + n) % n`.
  - A copy-constructed `VIVect2I(tbl[k])` reads through one address.
  - An aggregate initialiser `{ VIVect2(..) }` memsets each element, where a
    mem-initialiser constructor does not.

## Open: 4 screen functions, all register or scheduling ties

**Update, same evening:** the three `snprintf16` rows were not ties. `VIString::snprintf16`
returns `void`, and the `call_value` was what pushed the result copy down. See
[decomp-setup-ties-20261016.md](decomp-setup-ties-20261016.md). `ComputeTextRows` is
still open.

| Function | Words | What differs |
|---|---|---|
| `VIWndEula::ComputeTextRows` | 16 | the two word-wrap arms end identically in mine and jump2 merges them; the target's space arm puts `i*2` in a0, mine in a1 (local-alloc's lifetime extension) |
| `VIWndController::OnDrawSelf` | 3 | after `GetString(786)` the target moves `snprintf16`'s arguments a3, a2, a0; mine a2, a0, a3 |
| `VIWndEula::OnKeyDown` | 3 | the same tie, on the second of two `snprintf16` calls |
| `VIWndOptions::OnDrawSelf` | 2 | the same tie: `t0 ← v0` before `a2 ← s0` |

The three `snprintf16` ties are one phenomenon. None of these changed them: a local for
the `Word()` value, `Word(UI(), …)` in place of the cached `ui`, a macro `Word`, a
`static inline` one, or a `void *` cast.
