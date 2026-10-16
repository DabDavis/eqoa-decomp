# SETUP: network setup, DNAS, the patcher, the title screens

SETUP is 483,568 bytes of its own code in 1,805 functions, followed by the usual
library tail (185 `LIB` functions, crt0 onward). It comes in three blocks, in
link order:

| Block | Functions | Bytes | State |
|---|---|---|---|
| The game's screens: `VISetup`, `VIFader`, `VIWindowManager`, `VIWndSplash`/`Eula`/`Dnas`/`Connect`/`Patcher`/... | 251 written + dead template copies | 90,632 | **247 byte-exact (2026-10-16)**, 4 ties |
| Verant's patcher (`PatcherAPI`, `Http_get`, `VUrl`, the downloaders, delta files) and POSIX shims | 462 written | 52,584 | **454 byte-exact (2026-10-16)**, 8 register/scheduling ties |
| **libxml2 2.3.6** | **953** | **335,492** | **953 byte-exact (2026-10-16)** |

(The libxml block is 982 functions by address. 29 of them are the patcher's
`PullXMLAttribute*` helpers in front of it and the `VUrl` class behind it.)

The Sony DNAS and EzNetCnf code is NOT in SETUP. It imports
`sceDNAS2InstPersonalizeData_nohdd`, `ezNetCnfGetCombination` and the rest.

## libxml: upstream source, not hand decomp

`xmlParserVersion` is `"20306"`. The asserts name the game's copy as
`C:/code-frontiers-gm/Patcher/libxml/`. So the source is the libxml2 2.3.6
release, compiled with the decomp's settled toolchain against the configuration
the PS2 build used. Every function in the block is byte-exact:
- all 594 that the beta transfer named;
- all 359 statics, named here by their position between those.

```
./libxml/fetch.py          # once: libxml2-2.3.6.tar.gz (SHA-256 pinned) + eqoa.patch -> src/libxml/
EQMODULE=SETUP ../../eqcheck.py
```

The upstream files are not in the repository; `fetch.py` puts them in place.

**What is ours:**
- `libxml/eqoa.patch`: the game's edits, plus the `#line` that gives `__FILE__`
  the original path.
- `src/libxml/ps2/`: `config.h`, and the newlib-shaped libc headers it compiles
  against.
- `src/cflags.txt`: the include order.

Every setting was read off the image. None was guessed into place.

- **No `HAVE_STDLIB_H`.** `xmlInitMemory` has no `getenv("XML_MEM_BREAKPOINT")`.
  `HAVE_MALLOC_H` supplies `malloc`.
- **No `HAVE_ZLIB_H`.** `xmlIO` has no `xmlGzfile*` functions, and
  `xmlSaveFile` does no compression. `gzread` is not even imported.
- **No `HAVE_FLOAT_H`.** `xmlXPathFormatNumber` uses the file's own
  `DBL_DIG 16` fallback.
- **`HUGE_VAL` is newlib's `__infinity[0].d`,** which the module imports.
  `xpath.c` therefore compiles its own `isinf`.
- **`snprintf`/`vsnprintf` become `sprintf`/`vsprintf`, with the size dropped.**
  Only those two are imported.
- **`read`/`write`/`close` are no-ops.** `xmlFdRead` and `xmlFdWrite` are
  `return 0`, and `xmlFdClose` is empty.
- **FTP, HTTP and iconv are off.** No `nanoftp`/`nanohttp`.
- **`testXPath.c` is linked.** Its `main` is the module's `main`.
- **The game's own source edits** (the patch):
  - a `getcwd` stub (`return buf`) in `xmlIO.c`;
  - 200- and 160-byte input and space stacks in `xmlInitParserCtxt`, with the
    `Max` fields left at 5 and 10;
  - `xpath.c` three lines longer above line 2680, read off every `__LINE__`
    after it.

**Harness support added for this:**
- `eqcheck.sources()` builds `*.c` too, one directory deep.
- `src/cflags.txt` holds a module's C flags.
- A static that two TUs both define is mapped as `<tu>:<name>`
  (`parser:areBlanks`, `HTMLparser:areBlanks`). The scorer and the callee check
  understand that form.

Long form: `claude-notes/decomp-setup-libxml-20261016.md`.

## The patcher: 454 of 462

All 462 functions are written, in 20 TUs, with every class in `src/patcher.h`.
The TU names are all ours; only libxml's files are named by asserts in the image.

| TU | Exact | What |
|---|---|---|
| `patcherapi` | 53/53 | the `PatcherAPI` handle, all forwarding wrappers |
| `patcherapi_impl` | 59/64 | the patcher proper, `Patcher_thread`, the estimates |
| `patcherchanges(_impl)`, `patcherdistributioninfo(_impl)` | 11, 12, 43, 57 | the manifest tree; the `_Impl`'s inline `Call*` members are emitted after its `__tf` |
| `patcherdownloader`, `directorydownloader`, `distributiondownloader`, `filedownloader`, `filedeltadownloader` | 41, 16, 24, 28, 10 | the downloaders |
| `http_get` | 25/27 | one HTTP download into memory, optionally gzip |
| `vurl` | 23/24 | the HTTP/1.x client over libinet (shared with STATION) |
| `delta` | 6/6 | the delta coder, its index sort and binary search |
| `patcherutil` | 11/11 | time formatters, `FormatString`, `GetTempName`, `uthread_*` |
| `vmemcardfile` | 9/9 | a memory card file over `sceMc*` |
| `pullxml` | 6/6 | `PullXMLAttribute*` |
| `log`, `minigzip.c`, `ps2port` | 5, 5, 10 | the log that formats and drops; gzip; the POSIX stubs |

The eight open functions are register-allocation and scheduling ties.
`claude-notes/decomp-setup-patcher-20261016.md` has the table of what differs,
the delta format, `VUrl`'s layout, the Frontiers-only `GetPoll`, and the levers.

## The screens: 247 of 251

All 251 functions are written, in 18 TUs, with every class in `src/setup.h`. The
other 139 functions in the block are dead template copies. gcc 2.95 has no COMDAT, so
every TU that includes the header keeps its own `VIMap<int, int>` (ten copies) and
some keep a `VIArray<VIItemStat>` (four). Only the first copy of each is written:
`main.cpp` holds the map's and `viwndbeta.cpp` the array's. `eqcheck` counts the rest
as verified duplicates.

| TU | Exact | What |
|---|---|---|
| `main` | 14/14 | `setup_main` and the live `VIMap<int, int>` |
| `visetup` | 39/39 | `VISetup`, 0x21AE0 bytes: the boot state machine, `RunPatcher`, the card I/O, the patcher callbacks |
| `vifader`, `visplashfx`, `viwindowmanager` | 7, 7, 8 | the fade, the title's animated layers, the window driver |
| `viwndbeta`, `viwndconnect`, `viwndcontroller`, `viwnddnas` | 16, 18, 8/9, 13 | the windows, one TU each, in alphabetical link order |
| `viwndeula`, `viwndlanguage`, `viwndlegal`, `viwndmessage` | 10/12, 9, 9, 13 | |
| `viwndoptions`, `viwndpatcher`, `viwndpresents`, `viwndreadmessage`, `viwndsplash` | 19/20, 17, 9, 13, 18 | |

Each TU ends with its template copies, then `__tf`, then its inline members, in
declaration order. Those positions are what told the TU boundaries apart. The names
are the 2002 betas', slot by slot. `claude-notes/decomp-setup-screens-20261016.md`
covers the layouts, the boot and patch flow, the three shipped bugs, the four ties and
the levers.

## The rest of the map

- 904 code rows carry `# beta-export` names (see the decomp README): 620 in
  libxml, 266 in the patcher and only 18 in the game's screens. Most of the
  screens' bodies changed since the beta, so their names came over by slot order
  instead (the beta links the same files in the same order).
- STATION shares libxml, `VUrl` and some UI code. It does NOT share the patcher.
