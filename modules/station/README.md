# STATION: the SOE Station account screens (1,983 of 2,339 byte-exact, 2026-10-16)

STATION is 596,668 bytes of its own code in 2,339 functions, followed by the usual
library tail (183 `LIB` functions, crt0 onward). It runs after SETUP and before
CLIENT. It covers:
- the Station login;
- account creation and billing, as `STATION.PAK`'s `PS2GUIPage` XML pages;
- customer service;
- the account-server connection, over DRDP and libinet.

It exports one name, `station_main(char *login, char *password)`. Every other name in
`symbols.txt` is read off the image:
- class names from the RTTI strings;
- methods from the vtable slots of a known base;
- SETUP and CLIENT twins;
- names in comments marked "ours".

**State:** 1,983 of the 2,007 written functions are byte-exact (504,780 of 596,668 bytes).
Another 311 are verified duplicates and 18 are stubs under four instructions. Only three are
not written. `eqcallcheck` reports 0 mismatched callees and `eqrefcheck` 0 findings. The 24
opens and the 3 unwritten, each with its diagnosis, are in
`claude-notes/decomp-station-20261016.md`.

## What is shared, and how

Three blocks of STATION are another module's source, compiled again. They are
byte-identical with relocations masked. `src/` holds **relative symlinks** to those
files rather than copies:

| STATION | Source | Functions | Link |
|---|---|---|---|
| libxml2 2.3.6, `0x002F54F0 .. 0x003474D8` | SETUP's build, in the same order, minus `testXPath.c`'s three | 950 | `src/libxml/*.c`, `ps2/` and `include/` link to `modules/setup/src/libxml/` |
| `VUrl`, `0x002DC7F0 .. 0x002DDC48` | SETUP's `vurl.cpp` | 24 | `src/vurl.cpp` and `src/vurl.h` |
| `VIFader`, `0x002B95B8 .. 0x002B98D0` | SETUP's `vifader.cpp` | 7 | `src/vifader.cpp` and `src/vifader.h` |
| `VIMemStream`, `0x002E2E00 .. 0x002E36E0` | CLIENT's `vimemstream.cpp` | 23 (19 named) | `src/vimemstream.cpp` and `src/vimemstream.h` |

The other direction: **eight STATION TUs are CLIENT's code too**, and CLIENT's `src/` links to
STATION's files. They are `vidatetime`, `vimemstreamarray`, `vimemstreamops`,
`vimemstreamset`, `vimemstreamvector`, `vinet`, `vicsclient` and `vicsrecords`, with the
headers `drdpapi.h`, `vinet.h`, `vimemstreamx.h`, `vidatetime.h`, `vimemstreamvector.h` and
`vicsclient.h`. CLIENT went from 1,136 to 1,208 byte-exact. `viclienti.h` is per module, NOT
shared: the CS client calls slot 1 of a `VIClientI`, which has one slot in STATION and 18 in
CLIENT.

Each link needed its declarations in a header of their own:
- `vurl.h` came out of SETUP's `patcher.h`;
- `vifader.h` came out of SETUP's `setup.h`;
- `vimemstream.h` came out of CLIENT's `game.h`.

Each split was checked on all five modules: SUPPORT 3,665, CLIENT 1,136, MCERR 44
and SETUP 1,661 were unchanged.

The libxml sources are not in the repository. Run `modules/setup/libxml/fetch.py`
once, and both modules build. The crypto libraries are fetched the same way:
`modules/station/crypto/fetch.py` puts SSLeay 0.6.3's libdes 3.24 (from FreeBSD's import) and
LibTomCrypt 0.75 in `src/crypto/`, SHA-256 pinned, and `station-ltc.patch` takes LibTomCrypt
back to the <=0.66 API the image shows. Our own `des_soe.c`, `aes_soe.c` and `ps2/` sit
beside them. `_publish.py` exports symlinks as symlinks, so the
export never carries upstream libxml under STATION's name.

Making the links work also needed `src/vi.h`'s include guard fixed. The guard closed
at line 7354 of 9,537, so a second `#include "vi.h"` redefined every class added after
it. The old `#endif` is now a comment line, which keeps every line number, and the
guard closes at the end of the file.

## The rest

`main.cpp` holds `station_main` and the first copies of `VIMap<int, int>` and
`VIArray<VIItemStat>`. Every TU that includes the item header carries its own copy of
both (no COMDAT), so the other 300-odd copies are verified duplicates.

`station_main` makes a `VIStation`, runs it, and copies the login and the password
back out. The password goes out AES-256-encrypted, with LibTomCrypt's rijndael under
the 32-byte key at `0x00353B10`.

The classes, from the RTTI strings, the vtables and the constructors' vtable stores:

| Area | Classes | Header |
|---|---|---|
| The program | `VIStation` (0x2B130 bytes), `VIWindowManager` (older than SETUP's), `VIWndStationLogin`, `VIWndEdit` (CLIENT has it too) | `stcore.h`, `stedit.h` |
| GUI objects | `VIGUIDataModel` and its String/Toggle subclasses, `VIGUIObject` 0x98, `VIGUIText` 0xFC, `VIGUIButton` 0x104, `VIGUIComboBox` 0x120, `VIGUIToggleButton` 0x9C | `stgui.h` |
| Pages | `VIGUIPage` 0x7C, `VIGUIDialog` 0xE18, `VIGUISelectionGrid`, the PS2GUIPage parser | `stpage.h` |
| The renderer | `VIWndGenericRenderer` (`VIWnd` + `VIStationListener`) | `strenderer.h` |
| Crypto (C) | Eric Young's libdes, LibTomCrypt 0.x rijndael, Fromberger's MPI | `stcrypto.h` |
| Account server | `VINetBuffer`, `VITCPConnection`, `VIStationAPI`, `VIConnectionListener`, `VIStationListener` (22 callbacks) | `stapi.h` |
| Customer service | the CS client (CLIENT's code too), `VIClientI`, `VICSInterface`, the CS records | `stcs.h` |
| Containers, files | CLIENT's `VIMemStream*` containers, `VIFlatFile`/`Directory`, the time formatters, the send helpers | `stnet.h` |
| CS windows | `VIWndCSInbox`, `VIWndCSRequest`, `VIWndReadCS` 0xB760 | `stwndcs.h` |
| Other windows | `VIWndMBox` 0x5D8, `VIWndMessage` (mostly SETUP's), `VIWndOptionList` 0x208, `VIWndCombo`, the XML helpers | `stwnd.h` |

The work was split into ten lanes, one per area header. `station.h` includes the area
headers in dependency order. A class that embeds
another by value needs the complete type first: the renderer embeds a
`VIStationAPI`, a `VICSInterface`, a `VIWndEdit` and a `VIWndCombo`, and `VIStation`
embeds the renderer.

```
EQMODULE=STATION ../../eqcheck.py        # from decomp/: EQMODULE=STATION ./eqcheck.py
```
