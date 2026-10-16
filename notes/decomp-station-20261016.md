# STATION decompiled in one session: 1,983 of 2,339 byte-exact, every function written but three (2026-10-16)

STATION is the module between SETUP and CLIENT. It covers:
- the SOE Station login;
- account creation and billing, as `STATION.PAK`'s `PS2GUIPage` XML pages;
- customer service;
- the account server.

It is 596,668 bytes of its own code in 2,339 functions and exports one name, `station_main`.
It went from 0 to 1,983 byte-exact in one session, out of 2,007 written:
504,780 of the 596,668 bytes. Another 311 functions are verified duplicates of matched ones,
and 18 are stubs under four instructions. Only three functions are not written.
`eqcallcheck` reports 0 mismatched and `eqrefcheck` 0 findings. Source: `decomp/modules/station/src/`. The module README
is `decomp/modules/station/README.md`.

The headline: **more than half of STATION was already written, in two other modules.** The
same session took **CLIENT from 1,136 to 1,208** by sharing eight STATION TUs back to it.

## What was already somebody else's code

The first move was a masked-body comparison: every STATION function against every SETUP,
CLIENT and SUPPORT function, with relocated fields masked. It found:

| Block | Source | Functions | How it is shared |
|---|---|---|---|
| libxml2 2.3.6 | SETUP's build, same order, minus `testXPath.c` | 950 | `src/libxml/*.c` are symlinks to SETUP's (fetched) files |
| `VUrl` | SETUP's `vurl.cpp` | 24 | symlink, after splitting `vurl.h` out of `patcher.h` |
| `VIFader` | SETUP's `vifader.cpp` | 7 | symlink, after splitting `vifader.h` out of `setup.h` |
| `VIMemStream` | CLIENT's `vimemstream.cpp` | 23 | symlink, after splitting `vimemstream.h` out of `game.h` |
| `VIMap<int,int>`, `VIArray<VIItemStat>` copies | SETUP-identical template copies | ~330 | one copy written in `main.cpp`, the rest verified duplicates |

Then the lanes found more, and shared it the other way, STATION's file and CLIENT's
symlink:
- `vidatetime`, `vimemstreamarray`, `vimemstreamops`, `vimemstreamset`, `vimemstreamvector`
  and `vinet` (lane net);
- `vicsclient` and `vicsrecords` (lane cs).

**Symlinks, not copies.** A file linked into two modules is ONE source file. Each module's
`src/` holds a relative symlink, `_publish.py` exports symlinks as symlinks (`symlinks=True`),
and every split was verified neutral on all five modules.

The header splits needed one SUPPORT fix. **`src/vi.h`'s include guard closed at line 7354 of
9,537.** Every class phases had added after that line was unguarded, so a second
`#include "vi.h"` redefined them. The old `#endif` is now a comment line, which keeps every
line number, and the guard closes at the end of the file.

## The crypto is three upstream libraries, all built from source

Lane crypto took the whole crypto block, 122 functions, byte-exact:
- **DES** is Eric Young's libdes 3.24, from SSLeay 0.6.3. The image's version strings name it,
  and `des_options()` returns the fixed string `"des(idx,long)"`. No SSLeay 0.6.3 tarball
  survives, but FreeBSD imported exactly that DES (`freebsd-src` 43e844cb). Built unmodified.
- **Rijndael and MPI** are LibTomCrypt. The image keeps the per-file ID tags and the string
  errors (`"Invalid key size for rijndael."`) that 0.67 removed, so it is a release no later
  than 0.66. Nothing older than 0.75 survives, so `fetch.py` takes 0.75 and
  `station-ltc.patch` walks it back. The patch:
  - restores string errors with `CRYPT_ERROR`;
  - makes the key words `unsigned int`, while the `UL` literals stay 64-bit (the key
    schedule's 64-bit multiply shows it);
  - sets MPI to `MP_MACRO`/`MP_LOGTAB`/`MP_IOFUNC` 0.
- **The 4,176-byte `symmetric_key` union** on `station_main`'s stack is Blowfish's schedule:
  the whole cipher set was configured.
- **When it arrived:** neither 2002 beta disc has rijndael or MPI, so SOE added LibTomCrypt
  between October 2002 and retail. libdes is in every build.
- **SOE's own five wrappers** are C, names ours: `des_soe_*` around the key string
  `"LPSS-V1.0$@Z>.ADJ"`, and `aes_ecb_{en,de}crypt_buffer`.
- **What `station_main` does with it:** it AES-256-encrypts the password it hands back to SLUS,
  under the 32-byte key at `0x00353B10`.
- **The upstream files are not in the repository.** `modules/station/crypto/fetch.py` pins
  each one's SHA-256, and `_publish.py` reads its `FILES` list to keep them out of the export.

## Finding the classes when the module names nothing

STATION exports one symbol. The classes came out of the image in four steps:
1. **RTTI strings.** A `__tf` function references a mangled class name (`17VIWndStationLogin`).
   Finding those by shape gave every polymorphic class, and the base `__tf` each calls gave the
   hierarchy.
2. **Vtables.** Each vtable is 8-byte entries with `__tf` at slot 0 and ends in a zero entry.
   For a `VIWnd` subclass the slots are `VIWnd`'s seven. A secondary vtable has non-zero deltas:
   VIWndGenericRenderer's, at `0x00357800`, has delta `-0x24`, the `VIStationListener` base.
3. **Constructors and destructors** are the functions that store a vtable's address. The store
   offset is the vptr's position, and "the vptr goes at the end of the class's own data" gives
   the base size.
4. **Sizes** come from `new` + constructor pairs.

The work was split into ten lanes, each owning one area header (`st*.h`) included by
`station.h` in dependency order. Every merge conflict had one shape: a lane's placeholder
declarations for a class it did not own, against the owner's real ones. The fix was always
the owner's side, plus renaming the calls in the other lane's TUs. One trap: a blanket
`->Setup(` rename also hit `VIWndOptionList::Setup`.

## A struct passed by value travels as a pointer, and the callee copies it

Lanes misc and page wrote the two sides of the XML reader's callback. Misc wrote the caller,
`VIXMLParser::Walk(&ev)`, and page wrote the handlers, `Handler(VIXmlElement e)` by value.
Both matched. Making the handlers take a pointer broke each one by 75 to 400 words.

The reason is the MIPS EABI on the EE:
- An argument larger than 8 bytes travels by invisible reference, and
  `FUNCTION_ARG_CALLEE_COPIES` is set.
- So the caller passes `&ev` with no copy, whether the source says `cb(ev)` or `cb(&ev)`.
- The callee's prologue copies the struct into its own frame (`ldl/ldr` + `sdl/sdr` off
  `a0`) and calls its methods on the copy.

**The callback type is by value.** Names built on it mangle `PFG10VIXMLEvent_v`, not
`PFP...`. Memory: `feedback_eabi_callee_copies_byvalue.md`.

## Small fixes to shared code

- **`src/viset_t.h`'s `VISet::Size()` was `return m_size;`.** SUPPORT never emits it, so the
  body was never checked. STATION and CLIENT both use VIMap's form: `m_size - 1`, clamped at 0.
- **`src/vilist_t.h` now defines `VIList<T>::Insert`.** It was declared in `vi.h` and defined
  nowhere. Two lanes had each defined it locally, with different statement orders, and page's
  order matches both users.
- **`src/vi.h` gained `VIVector<T>::operator[] const` (lane cswnd) and the `VIVector` copy
  constructor (lane cs).** Both are inline templates nothing else instantiates, and SUPPORT,
  CLIENT and SETUP are unchanged.
- **CLIENT's placeholder template types, now named:**

  | Placeholder | Real type |
  |---|---|
  | `VIPtr_CB9308 *` | `int` |
  | `VIRec4_BBBF08` | `int` |
  | `VIRec4_BBC218` | `int` |
  | `VIRec136_BE8808` | `VICSSubcategory` |
  | `VIRec24_BE84C8` | `VICSMessageHeader` |
  | `VIElem_BE8C48` | `VICSCategory` |
  | `VIClientPart33950` | `VICSClient` |

## The renderer

`VIWndGenericRenderer` (0x002C98B8 .. 0x002D1F68, 95 functions, 88 exact) is the window that
loads `STATION.PAK`, runs the pages and answers the account server.
- **It is 0x1FD20 bytes, and `VIStation` embeds it last** (lane core measured it from
  `VIStation`'s dtor). It ends in a `VICSInterface` at +0x1A490 (0x5890 bytes).
- **Its `VIStationListener` base is PRIVATE:**
  `class VIWndGenericRenderer : public VIWnd, VIStationListener`. `eqrefcheck` found it.
  - In g++ 2.95 `__rtti_class`'s base_info word holds offset:29, is_virtual:1 and access:2.
  - The listener entry reads `0xC0000024` (access 3, private, at +0x24), where a `public`
    base gives `0x40000024`.
  - The code is the same either way; only the RTTI data shows it.
- **`OnAction(const char *)` is the whole page action table:** 28 names in a `strcmp` chain,
  2,719 instructions, 171 words off. Every `SetText(Word(UI(), n))` tail is cross-jumped into
  one copy.
- **The form checks are written in one shape:**
  `bool ok = CheckName(...); bool valid; valid = CheckName(...); ok &= valid;`.
  Writing `ok &= CheckName(...) != 0` instead turns into a `movz`.
- **Case order comes from the jump tables, read off the image.** A `code > 0 && code < 4` that
  is not range-folded is `switch { case 1: case 2: case 3: }`.
- **`0x002D1F28` was misnamed.** The beta transfer gave it SETUP's
  `_GLOBAL_$I$VISplashFxInit...`. It is this TU's static initialiser
  (`_GLOBAL_$I$__20VIWndGenericRenderer`, for one static `VIUIWindow`), and `0x002D1EF0` is
  `GetDialog`, not `NeedDelete__17PatcherDownloader`.

## Not written (3)

- **Two copies of `__tf14VIGUIDataModel`** (0x002BD340, 0x002BD550), in the string and toggle
  data-model TUs. They are byte-identical to the named copy except that each TU points at its
  OWN copy of the string literal `"14VIGUIDataModel"`. So the relocated bodies differ, they
  are not verified duplicates, and one global name can be scored only once.
- **`VIMemStream::Remaining`'s out-of-line copy** (0x002E3640, 4 instructions), in CLIENT's
  shared `vimemstream.cpp`. Emitting it needs the accessor defined inline after its use in
  that file, while CLIENT defines the same accessor out of line in `VIClient.cpp`.

## Open (24)

All are register or scheduling ties, or are written and not yet polished:
- **`VUrl::OpenSocket`** (24): SETUP's tie, the same file.
- **VIWndEdit** (lane core):
  - `OnKeyUp` (6): a shared delay-slot store becomes branch-likely.
  - `WrapLines` (32): a loop-test load is reused in the target.
  - `OnDrawSelf` (827 of 1,448): written, not polished.
- **`VIWndStationLogin::OnDrawSelf`** (161): a string pointer held in a saved register. It is
  dead code: nothing creates this window.
- **`VIWndOptionList::OnDrawSelf`** (5): argument-move rank.
- **`VIWndMBox::OnDrawSelf`** (18): the y argument is loaded late.
- **VIStationAPI** (lane api):
  - `ChangePassword` (6): store order; 9,301 permuter builds, no find.
  - `Request76` (4).
  - `OnReply84` (17): the block-copy template.
- **`VINetBuffer::Read`** (12): branch-likely.
- **`ExpandText`** (27): a peeled loop; 5,221 permuter builds, no find.
- **The CS windows** (lane cswnd), all register choice:
  - `VIWndCSInbox::OnDrawSelf`;
  - `VIWndCSRequest::OnDrawSelf` and `OnMessage`;
  - `VIWndReadCS::OnDrawSelf` and `Wrap`.
- **The renderer** (written in its 200th turn, not polished):
  - `Load` (38 of 267);
  - `SaveAccount` (51);
  - `DrawBackground` (109);
  - `OnAction` (171 of 2,719);
  - `OnValidateLogin` (189);
  - `OnDrawSelf` (320);
  - `PushPage` (499 of 728).

## Method notes

- **`sdis.py` truncated strings at 100 characters, so long strings were guessed.** Lane api
  guessed two error messages, and `eqrefcheck` caught both as `WRONG STRING`. Run `eqrefcheck`
  on every merge.
- **A lane that hits the 200-turn cap is still mergeable** if it keeps NOTES.md current.
  - Lane cswnd stopped mid-TU with all 75 functions written, 70 exact.
  - Lane renderer stopped with all 95 written, 88 exact.
  - Merge such a lane BY HAND, taking only the files it owns. A lane that copied main's TUs
    to build against would otherwise fold stale copies back over main.
- **Four functions fell between two lanes' ranges.** `VIWndStationLogin`'s inline members sit
  after its `__tf` at the very start of the next lane's range. Declaring them inline in the
  class emits them in order. A TU-to-lane split should give a TU's tail to its owner.
- **Scratch tools** (not in the repo) are in `~/.cache/station/`:
  - `sdis.py` disassembles with names and full strings;
  - `ren.py` renames map rows;
  - `cmpmod.py` / `cmp2.py` do the masked cross-module comparison;
  - `rtti2.py`, `vtables.py`, `vtrefs.py` and `newsizes.py` are the class survey;
  - `postmerge.sh` checks after each merge.
