# MCERR decompiled: 44 of 44 byte-exact (2026-10-16)

MCERR is the smallest EE module: 13,144 bytes of its own code, one window class
(`VIWndMcErr`, the memory-card status and setup window) and the 8-byte session key
it keeps on the card. All 44 functions are written and byte-exact in one session.
`eqcallcheck -a` reports 390 call sites, all named, 0 mismatched. `eqrefcheck -a`
reports 129 data references, 0 findings. Source: `decomp/modules/mcerr/src/`
(`mcerr.h`, `viwndmcerr.cpp`). Baseline: `modules/mcerr/src/MATCHED.txt` (44).

## Names came from the 2002 betas, not from guessing

Retail MCERR exports 6 symbols. The two 2002 USA beta discs export 41
(`decomp-builds/tools/snr1map.py modules/eqoa-beta-20021020/MCERR.snr1`). That
list gives the whole class: ctor, dtor, `Init`, `Free`, the four `VIWnd`
overrides, `GotoScreen`, `ExitMcErr`, `ProcessScreen`, `ProcessScreenNN` for
each screen, `DrawPrompt`, and the session-id pair.

- **The screen numbers are read off Frontiers' own dispatcher.** Its jump table
  covers 4..64 and its cases call 19 screen bodies: 04 10 11 12 15 16 17 20 21
  22 30 50 52 54 60 61 62 63 64. Frontiers adds 12, 17, 21 and 22 to the betas'
  set.
- **One body is never called.** `0x002A9A00` sits between 22 and 30, which is
  where the beta's `ProcessScreen25` sits, and no case reaches it. The body
  survived after its case was removed.
- `McErrSyncIdle` gained a `VIKeyboard *` after the betas
  (`McErrSyncIdle__FP5VIPad` there, `...P10VIKeyboard` here).
- Layout: `VIWnd` (0x20), its vptr at +0x20, `VIUIWindow m_window` +0x28,
  `m_sel` +0x1A8, `m_screen` +0x1AC, `m_count` +0x1B0, `VIVect2I m_center` +0x1B4
  (its default ctor is the ctor's two zero stores), `m_owner` +0x1BC, `m_mode`
  +0x1C0, `m_result` +0x1C4, `VIConsolePad m_pad` +0x1C8. Size is 0x21C. One static
  instance is at `0x002B1080`, and the session key is at `0x002B12A0`.

## What the residue taught (each one closed a family of screens)

1. **`VIConsolePad` is 84 bytes, not 96.** MCERR copies and clears exactly 84
   (two 32-byte `ldl/sdl` rounds + 20). `vi.h`'s `m_pad[3]` was invented. The 96
   was `VIConsole::Init`'s frame slot, which is 84 rounded to 16. SUPPORT's
   `viconsole` (6/6) and `vipad` (14/14) are unchanged.
2. **A screen is `switch (msg) { case 0: ...; case 3: ... }`, not an if/else.**
   Both tests come before both bodies (10 screens).
3. **The window's width is read off the window's own address.** The target reads
   it as `lw t0,20(sN)`, where `sN` is `&m_window`; `m_window.m_size.x` gives
   `this+60`. A reference variable fixes most screens. In 62 and 63, though, the
   later uses are a gcse COPY of `&m_window` made right after its first
   computation. That copy only appears when no variable exists and every call
   recomputes `this+40`, which an inline accessor does
   (`VIUIWindow::Width()`, added to `vi.h`). 63 went 40 → 8 words.
4. **One `VIUI *` value per word fetch.** For the game's name
   (`VIGetExpansionFlags() & 2 ? 1803 : 1802`), the target loads `mUI` BEFORE
   the flag call and reads `m_language` off that same register after it. An
   inline member `mUI->Word(id)`, or any macro, evaluates the call first. A
   free `Word(VIUI *ui, int id)` called as `Word(UI(), ...)` gets the order
   right: a call-containing argument is precomputed in order. This took 21/22
   from 37 words to 2.
5. **`VIWnd::UI()` is INLINE, and SUPPORT proves it.** Its five accessors
   (`GetParent`, `SetVisible`, `GetVisible`, `UI`, `GetFocus`) have 0 callers in
   SUPPORT and sit after `__tf5VIWnd`. That is where gcc 2.95 emits a class's
   inline members in the TU that emits its vtable. Moving them into the class
   in `vi.h` and deleting their out-of-line definitions keeps `viwnd` 18/18:
   gcc now emits them itself, weak and in the image's order.
   - In MCERR, `Init(UI(), ...)` routes the load through the inline's return
     pseudo. That flips sched2's tie with the `CountLines` result copy, which
     closed seven screens and `OnMessage`.
   - The permuter found the shape: `decltype(mUI) pt1 = mUI;` before `Init`.
   - A `VIWndMcErr`-local stand-in fails a different way: it is emitted out of
     line, because MCERR emits that class's vtable.
6. **`VIUI::Begin2D()` returns VOID.** After `ui->Begin2D()`, five screens load
   `m_count` straight into `v0`. local-alloc only hands out `v0` there if the
   call did not set it. `return m_raster->Begin2D()` compiles the same either
   way, so SUPPORT's `viui` stays 43/43. This is the same fact phase 61 found
   for the rectangle fills.
7. **Cross-jumping keeps the LAST copy.** In 62, every `GotoScreen(64); b draw`
   merges into the copy in `Save()`'s failure arm. So `case -1` is written ahead
   of `case 1`, and the frame-5 failures `goto draw` (they skip the
   `m_count >= 6` test that the success path falls into).
8. **A dead division leaves its trap.** In 62, `case 0` holds only
   `li v0,20; beqzl v0; break 7`: gcc deleted the unused `/ 20` and kept the
   zero-divisor trap.
9. Smaller ones:
   - `if (id != x) return 0; return 1;` is `movn`; the `==` spelling is
     `sltiu`/`movz`.
   - A string in a local, declared after the `printf`, is loaded before the
     `VIMemCardProductId()` call.
   - In `McErrSyncIdle`, the window pointer is made inside the loop from
     `&sMcErrWnd`, so the calls keep `this` in a register and the pad pointer
     is `this + 460`.

## Header changes and their check

`vi.h` changes:
- `VIConsolePad` is now 84 bytes.
- The memory-card API is declared.
- `VIUIWindow::Width()` is new.
- The five `VIWnd` accessors are inline.
- `VIUI::Begin2D()` is void.

`viwnd.cpp` lost its five accessor definitions, and `viui.cpp`'s `Begin2D`
became void.

**Check:** every SUPPORT TU (154) and every CLIENT TU (14) was compiled against
the old and the new headers, and the assemblies were diffed with label numbers
and static-name suffixes normalised. Only `viwnd.s` differs, and it is still
18/18. SUPPORT's `eqcallcheck -a` and `eqrefcheck -a` reports are byte-identical
to `decomp-coord/kit/ref_*.txt`. `test_eqdecomp.py` passes 48/48.

The library tail (crt0, libgcc, the C++ runtime from `_start` at `0x002AB638`)
is not decompiled, by policy. One of its names was applied from the betas:
`__rtti_si`, which `__tf10VIWndMcErr` calls.
