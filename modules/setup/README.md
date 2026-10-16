# SETUP: network setup, DNAS, the patcher, the title screens

SETUP is 483,568 bytes of its own code in 1,805 functions, followed by the usual
library tail (185 `LIB` functions, crt0 onward). It comes in three blocks, in
link order:

| Block | Functions | Bytes | State |
|---|---|---|---|
| The game's screens: `VISetup`, `VIFader`, `VIWindowManager`, `VIWndSplash`/`Eula`/`Dnas`/`Connect`/`Patcher`/... | 395 | 90,632 | not started |
| Verant's patcher (`PatcherAPI`, `Http_get`, `VUrl`, the downloaders, delta files) and POSIX shims | 428 | 52,584 | not started |
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

## The rest of the map

- 904 code rows carry `# beta-export` names (see the decomp README): 620 in
  libxml, 266 in the patcher and only 18 in the game's screens. Those screens
  changed too much since the beta for names to transfer.
- STATION shares the patcher and UI code.
