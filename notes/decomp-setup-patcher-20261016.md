# SETUP's Verant patcher: 454 of 462 byte-exact (2026-10-16)

The second block of SETUP after libxml2 ([decomp-setup-libxml-20261016.md](decomp-setup-libxml-20261016.md)).
It holds the patcher and everything it drives:
- the `PatcherAPI` handle classes and their `_Impl`s;
- the four downloaders;
- `Http_get`, the `VUrl` HTTP client and `minigzip`;
- the delta coder;
- `VMemCardFile`, the time formatters and the uthread shim;
- the Win32 → PS2 POSIX stubs.

Every function in it is written: **462, of which 454 are byte-exact.** That
brings SETUP to **1,407 of 1,805**. The 390 screen functions (`VISetup`,
`VIWnd*`, `VIFader`, ...) are what remain.

Source: `decomp/modules/setup/src/`, 20 TUs plus `patcher.h`. Scorer:
`EQMODULE=SETUP python3 eqcheck.py`.
- `eqcallcheck`: 6,328 call sites, 0 mismatched.
- `eqrefcheck`: 0 findings in 1,405 functions.

## What the code is

- **The Verant patcher was ported from Win32, not rewritten.**
  - `ps2port.cpp` stubs `rmdir`/`chdir`/`mkdir`/`getcwd`. Its
    `Sleep(int ms)` is `SleepThread()`, which ignores the milliseconds.
  - `GetCurrentExecutableName` returns `host0:/main.elf`.
  - The thread layer is `uthread_*` over `CreateThread`, with a 16 KB
    memaligned stack, priority 100 and `_gp`.
  - `defMain` spins `RotateThreadReadyQueue(100)`.
  - `long` is 64 bits throughout, so every size getter returns 64 bits.
- **The delta format** (`delta.cpp`): a delta is the big-endian int `1`, the new
  size, then runs.
  - A negative length `-n` is followed by `n` literal bytes.
  - A positive length `n` is followed by an offset in the old data to copy `n`
    bytes from.
  - `MakeDeltaData` indexes the old data every `step` bytes by its first 16
    bytes. It sorts that index with a quicksort whose cursors are FILE STATICS
    (`data_005E4CC0`/`C4`, pivot and swap temp in `.bss`).
  - Matching uses a binary search (`DeltaIBSearch`) that walks back to the
    first equal key, then extends each candidate. Matches stop at 512 bytes
    and must be at least 16.
  - `UnmakeDeltaDataFile` renames the old file to `delta.hld` and streams it
    back through a 256 KB buffer.
- **`VUrl`** is 1,128 bytes, shared with STATION. The layout is in `patcher.h`:
  - libnet client at `+0x418`, a 0x10200-byte net buffer at `+0x454`;
  - memory card fd at `+0x460`, listener at `+0x464`.
  - It sends `GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n`, or `HTTP/1.0` for
    `GetToMemoryCard`. It splits the header on the first CR after an LF.
  - It returns success only if `Content-Length` was present and met.
- **`0x005DCFC0` is Frontiers-only** and unnamed in every build. We call it
  `GetPoll`; STATION has it at `0x002DD248`.
  - It is a resumable `Get`: one `sceInetRecv` per call, its state in eleven
    statics (`state` starts at 3, `contentLength` at -1).
  - The 2002 betas have a single 1,064-byte `Get` in its place.
- **Two shipped bugs worth knowing:**
  - `VUrl::InitMemCard` tests `if (!free)`, the C library's `free`, not the
    free-space count `sceMcGetInfo` wrote (gcc warns it is always true).
  - `VUrl::GetToMemoryCard`'s second receive writes at `data + total` into a
    1,460-byte local.
- **The `Call*` callbacks are inline members of `PatcherDistributionInfo_Impl`**
  (`CallError`, `CallFinish`, `CallForceRestart`, `CallForceComputerRestart`,
  `CallFatalError`, `IncrementScanCount`).
  - gcc emits them weak after `__tf28PatcherDistributionInfo_Impl`, because
    that TU holds the vtable, and the map names those copies.
  - Writing them as members rather than free helpers both matched the callers
    and produced the six out-of-line copies.

## Levers (the generalisable ones are in memory)

- **A parameter stored at entry and never loaded, in a slot ABOVE every local
  array, had its address taken and that use folded away.**
  - ADDRESSOF forces it to the stack at `purge_addressof`, which runs after
    expansion, so the slot lands above the locals.
  - `DownloadFile`'s `port` is never read (the URL uses `m_port`), but
    `if (!NotNull(&port)) return 0;` reproduces the store and the 4-byte shift
    of every later slot.
  - `(void)&port` does not reproduce it, and `*(volatile int*)&port` adds a
    load.
- **Spill-slot order is declaration order**, because reload assigns slots in
  pseudo order. A 16-byte-aligned slot is a pseudo that is also referenced
  64-bit wide.
- **Reload registers are handed out round-robin** (`last_spill_reg`). One early
  allocation difference changes every later reload register, so fix the
  global allocation first.
- **A call inside an argument list makes gcc precompute every argument into a
  pseudo.** `sceInetSend(&m_client, …, strlen(req), …)` kept `&m_client` live
  across `strlen` in a callee-saved register. The target computes
  `int len = strlen(req);` first.
- **A `move tX,vY` after a load, whose copy is used much later, is a GCSE
  reaching register.** The source reloads the same thing (`bestPtr = *e`)
  rather than keeping a local. That took MakeDeltaData from 31 words to 0.
- **The early-return form puts its block last.** `if (!s) return 0; …` gives
  a `beqz` to a zero block placed before the epilogue. `if (s) {…} return 0;`
  places it first.
- **gcc 2.9x cross-jumps a jump's tail into the code before its target label
  first** (`jump.c`, `find_cross_jump(insn, JUMP_LABEL, 1)`), and only then
  into other jumps. Which error tails share code says what precedes the return
  label.
- Smaller ones:
  - An indexed loop vs a pointer loop is visible in pre-header order.
  - `if (A||B||C||D) v = 1; else v = !E;` vs `v = A||…||E`.
  - Statement order after a call decides which update sched hoists into the
    delay slot.
  - A walk-back loop with its test at the top is `for (;;) { if (x) return; … }`.

## Open: 8 functions, all register or scheduling ties

| Function | Words | What differs |
|---|---|---|
| `PatcherAPI_Impl::DownloadFile` | 172 (66 aligned) | the `url` reaching register needs `s0` (it loses on priority to the loop constants); `file`/`savedDone` swap `s6`/`s7` |
| `PatcherAPI_Impl::DownloadToMemory` | 25 | `status`/`server` swap `s3`/`s6`: `status` must outrank `server` |
| `VUrl::OpenSocket` | 24 | the open- and resolve-error tails should share `sprintf`; mine share `PrintError` with the NULL block instead (sched2 order inside the blocks) |
| `PatcherAPI_Impl::RescanRoutine` | 7 | `s4`/`s5` swap |
| `Http_get::Get_thread` | 7 | sched2 puts `sd m_bytesDownloaded` before `sw m_status` |
| `Http_get::Get` | 6 | `url`/`c` swap `s3`/`s4` |
| `PatcherAPI_Impl::FindRoots` | 3 | `sprintf` argument scheduling |
| `PatcherAPI_Impl::GetEstimate` | 2 | `li a0,50` scheduling |

## Naming

- The statics are ours, marked in `symbols.txt`: `SortIndex__FPPci` and
  `QSort__FPPcT0` (unnamed in every build), and `GetPoll__4VUrlPci`.
- Applied from the beta order: `__tf11VIWndSplash`, five `VIWndSplash`
  accessors, the `rmdir`/`chdir`/`mkdir`/`getcwd` stubs, `GetError__4VUrl` and
  `Get__4VUrlPci`.
- `gPatcherCancel` (`0x005E4CC8`) is our name.

## Correction

`modules/setup/README.md` used to say "STATION shares the patcher". It shares
only libxml, `VUrl` and some UI.
