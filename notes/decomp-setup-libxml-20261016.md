# SETUP's libxml: 953 functions byte-exact from upstream source (2026-10-16)

SETUP is 484 KB of its own code. **71% of it is libxml2 2.3.6**, the XML parser
behind Verant's patcher (the `VerantPatcher` manifests). An earlier survey
guessed the module was mostly Sony DNAS/EzNetCnf; that was wrong, because SETUP
only imports those calls. The release source compiled with the decomp's settled
toolchain matches the image function for function once the PS2 build's
configuration is reconstructed:
- 953 of 953 byte-exact (594 named by the beta transfer, 359 statics named here
  by position);
- callee check: 4,966 call sites, 0 mismatched;
- data-reference check: 4,165 references, 0 findings.

Source: `decomp/modules/setup/` (`libxml/fetch.py`, `libxml/eqoa.patch`,
`src/libxml/ps2/`, `src/cflags.txt`). The upstream files are fetched, not
committed.

## How it was pinned down

- **Version:** the string `"20306"` in the image is `xmlParserVersion`. The
  asserts name `C:/code-frontiers-gm/Patcher/libxml/{encoding,xpath,xpointer}.c`.
  2.3.7 rewrote `xmlXPathFormatNumber`, and the image has 2.3.6's.
- **Which files:** the beta's 1,371 code exports include ~650 libxml names. Each
  upstream file's globals were checked against them:
  - linked: everything except `nanoftp`, `nanohttp`, `trio` and `xmllint`;
  - the module's `main` parses `-xptr -debug -valid -expr -tree --input --file`,
    so it is **`testXPath.c`**.
- **First compile, guessed config:** 538 of 592 exact. Every remaining
  difference was then read off the image, never fitted.

## The configuration, and the evidence for each line

| Setting | Evidence |
|---|---|
| `HAVE_STDLIB_H` undefined, `HAVE_MALLOC_H` defined | `xmlInitMemory` is 5 instructions: no `getenv("XML_MEM_BREAKPOINT")`. Without some header declaring `malloc`, `xmlmemory.c` does not compile |
| `HAVE_ZLIB_H` undefined | `xmlIO`'s callback functions are `xmlCheckFilename`, `xmlNop`, `xmlFd*`, `xmlFile*` and nothing else, with no `xmlGzfile*`. `xmlSaveFile` has no `compression` test. `gzread` is not imported |
| `HAVE_FLOAT_H` undefined | `xmlXPathFormatNumber`'s buffer offsets and bounds are one higher: the file's own `DBL_DIG 16` fallback, not newlib's 15 |
| `HUGE_VAL` = `__infinity[0].d` | the module imports `__infinity`. With `HUGE_VAL` defined, `xpath.c` compiles its own global `isinf` (two `dpcmp`, one `dpsub`) |
| `snprintf(b, n, f, ...)` → `sprintf(b, f, ...)`, `vsnprintf` → `vsprintf` | `sprintf`/`vsprintf` are imported, `snprintf`/`vsnprintf` are not. `xlinkIsLink` calls `sprintf(buf, "%s:external-linkset", prefix)` and still stores `buf[199] = 0` |
| `read`/`write`/`close` are no-op macros | `xmlFdRead`/`xmlFdWrite` are `jr ra; move v0,zero`, and `xmlFdClose` is a bare `jr ra` |
| FTP/HTTP/iconv off, XPath/XPointer/XInclude/HTML/debug on | the function set |
| newlib `ctype.h` (`toupper` reads `_ctype_+1`) | `xmlParseCharEncoding`'s relocations |
| `stderr` is `_impure_ptr->_stderr` | SUPPORT's settled newlib shape |

## The game's own edits (`eqoa.patch`)

- **`xmlIO.c`:** a `getcwd` that returns its buffer, defined just before
  `xmlParserGetDirectory`, which calls it.
  - The map's gap is exactly one 8-byte function, between
    `xmlOutputBufferFlush` and `xmlParserGetDirectory`.
- **`parserInternals.c`:** `xmlInitParserCtxt` allocates 200 bytes for
  `inputTab` and 160 for `spaceTab` (upstream 20 and 40).
  - `inputMax` stays 5 and `spaceMax` 10, so this is over-allocation, not a
    resize.
- **`xpath.c`:** three extra lines somewhere above line 2680.
  - Every `__LINE__` from `xmlXPathCompareNodeSetValue` on reads 3 higher.
  - Nothing above that line carries one, so their content and position are
    unknowable and they are placed after the includes.
  - `xpointer.c` has no offset.
- **`#line 1 "C:/code-frontiers-gm/Patcher/libxml/<file>"`** in the three files
  that use `__FILE__`. This is SUPPORT's convention; without it, eqrefcheck
  reports 172 wrong strings.

## Method worth reusing

- **Name the statics by position.** Between two functions both sides name, take
  our object's functions in order and the image's placeholders in order. If the
  counts agree, pair them.
  - Every gap agreed except one, and that was the `getcwd` stub.
  - 359 statics were named this way, all byte-exact.
- **An absent function is evidence.** Several facts were read from the function
  SIZES in the image's `xmlIO` region: which callbacks exist (six 8-byte
  functions, i.e. `xmlCheckFilename`, `xmlNop`, the three hollow `xmlFd*` and
  `xmlFileMatch`), and that `xmlRegisterDefaultInputCallbacks` is 88 bytes,
  not 124.
- **The import table is a config file.** What libc and zlib calls a module
  imports, and what it doesn't, decides `HAVE_*` macros directly.

## Harness changes (all modules; SUPPORT/CLIENT/MCERR verified unchanged)

- `eqcheck.sources()` and `source_path()` build `*.c` alongside `*.cpp` (one
  directory deep).
- A module's `src/cflags.txt` adds flags for its C sources.
- A static that two TUs both define is mapped `<tu>:<name>`:
  - `eqcheck.score` and `map_name` look that form up first;
  - the callee check treats it as equal to the bare local name;
  - `MATCHED.txt` records the qualified form.
- `_publish.py` leaves the fetched upstream files out of the export.

## What is left in SETUP

- The game's screens: 395 functions, 90.6 KB, almost all unnamed. The beta's
  `VIWnd*` names did not transfer.
- The Verant patcher and its shims: 428 functions, 52.6 KB, 266 named from the
  beta. Its C++ is shared with STATION.
