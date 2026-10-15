# MCERR: the memory-card status window and the session key

The smallest EE module: 13,144 bytes of its own code in 44 functions. It is one
window class built on SUPPORT's `VIWnd` and `VIUIWindow`, plus the session-key
file it keeps on the memory card. Everything here was measured from the image;
nothing is source yet.

## Working on it

```
EQMODULE=MCERR ./_tu.py <tu>             # every tool takes the module from EQMODULE
EQMODULE=MCERR ./eqcalls.py calls <sym>  # callees, strings and data by name
./eqmodule.py status MCERR               # functions, named, byte-exact
```

- **Source** goes in `modules/mcerr/src/*.cpp`, built into `modules/mcerr/build/`, with
  its own `MATCHED.txt`. TUs `#include "vi.h"`: the compile command adds SUPPORT's
  `src/` to the include path.
- **`symbols.txt` is kept by hand.** The seed named every function `func_<VA>` and every
  referenced data object `data_<VA>`. eqcheck finds a target by NAME, so a function is
  scored only once its entry is renamed to the mangled name your object defines. A
  placeholder is never a callee mismatch. Some `data_` entries are fields of a larger
  object (a reference to `obj + 0x1C0` seeds its own entry), so merge them as the
  layout becomes known.
- **The image** is `MCERR_decompressed_SNR1.bin` and the base is `0x002A8180`, where it
  sat in the dump its addresses were checked against.
- **The library tail is not decompiled.** It starts at crt0's `_start`
  (`0x002AB638`): 134 functions of crt0, libgcc, soft float and the C++ runtime, all
  tagged `LIB`. 41 are named after SUPPORT's identical copies.

## What the image already says

| Export | VA | Size |
|---|---|---|
| `McErrWnd()` | `0x002AB190` | 16: returns the address of `data_002B1080`, the static window |
| `McErr(VIWnd *, const VIVect2 &, int)` | `0x002AB1A0` | 184 |
| `McErrSetMode(int)` | `0x002AB258` | 16: one store to `data_002B1240` |
| `McErrSyncSet(const VIVect2 &, int)` | `0x002AB268` | 104 |
| `McErrSyncIdle(VIPad *, VIKeyboard *)` | `0x002AB2D0` | 528 |
| `VIIsMcSameSession()` | `0x002AB500` | 96 |

- **The class.** The RTTI names one: `VIWndMcErr`, a `VIWnd`. Its constructor
  (`func_002A8280`) runs `VIWnd::VIWnd`, stores the vptr at `+0x20` (the vtable is
  `data_002B0A98`), constructs a `VIUIWindow` MEMBER at `+0x28` and zeroes `+0x1B4` and
  `+0x1B8`. The destructor follows at `func_002A82C8`.
- **`VIIsMcSameSession()`** returns -1 if `VIMemCardStatus(0)` fails, or if reading
  `"session.id"` gives fewer than 8 bytes. Otherwise it returns 1 when the 8 bytes
  equal what `func_002AB4F0` returns, and 0 when they don't.
- **Strings:** `"SAME SESSION"`, `"DIFFERENT SESSION"`, `"NOT FORMATTED"`,
  `"FINISHED: %d"`, `"Writing session key (%ld) to memory card."`, `"session.id"`.
- **Everything it calls in SUPPORT is already matched there:** `VIUIWindow`'s `Init`,
  `Open`, `Close`, `Draw`, `TextBox`, `DrawProgress` and `CountLines`; `VIString`'s
  16-bit string functions; `VIStringTable::GetString`; `VIGameOptions::Save`;
  `VITimer::GetTime`; `VIPad::Read`; `VIKeyboard::Read`; `VIUI::Begin2D` / `End2D`.
- **The memory-card functions** (`VIMemCardStatus`, `VIMemCardReadFile`,
  `VIMemCardWriteFile`, `VIMemCardIsPrepared`, `VIMemCardFormat`, `VIMemCardPrepare`,
  `VIMemCardPadAsync`, `VIMemCardAsyncIdle`, `VIMemCardProductId`) are defined in
  SUPPORT's `src/vimemcard.cpp` but declared only there. Add their declarations to
  `vi.h`, additively, rather than in an MCERR TU.
- **No asserts**, so the image gives no source file names. In a TU, functions sit in
  source order.
