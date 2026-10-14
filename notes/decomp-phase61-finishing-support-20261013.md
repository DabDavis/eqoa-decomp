# Phase 61 — finishing SUPPORT (DRAFT, in progress)

Started 2026-10-12 on "keep going until SUPPORT is finished". The census that night:
**2,891 of 3,285 scored functions byte-exact; 431 functions (~276 KB) unwritten and
364 (~162 KB) written but open** — 80% of functions but only 47% of bytes, because what
was left was the big functions.

## Method: lanes

Every scorer shares `decomp/build` and `MATCHED.txt`, so two at once invent regressions.
`decomp/_lane.py new <name>` makes `../decomp-<name>/`: the tools copied (so `HERE` is
the lane), the toolchains symlinked, its own `src/` and `build/`, and a `.base/`
snapshot so `_lane.py merge` is a three-way `git merge-file` back into main. `_tu.py
<tu>` scores one whole TU plus its regressions in seconds. Eight lanes ran as agents
(raster, scene, drdp, ui, fx, flora, core, sys) and the coordinator worked the ESF
lane; each merge was followed by one whole-tree `eqcheck.py` in main.

The assembler now runs under a private wine prefix (`claude-eqoa/.wine`, no display,
no winemenubuilder), held up by one idle `wine cmd.exe` so every assembler call does
not cold-start a wineserver.

## VIESFParse — all nine remaining functions byte-exact

`ParsePrimBuffer` (774), `ParseSkinPrimBuffer` (722), `ParseFloraPrimBuffer` (355),
`ParseCollBuffer` (440), `ParsePrimBufferObjV0`, `ParseSkinPrimBufferObjV0`,
`ParseCSpriteObj` (first build), `ParseHSpriteAnimObj` (450) and `AdpcmToPcm`, plus the
older `ParsePointSpriteObj` and `ParseRefMap`. 107 → **109/117** in the TU.

`AdpcmToPcm` is not 758 instructions: it is the standard PlayStation VAG decoder, 129
instructions in **double** (GOFAST `dpmul`/`dpadd`/`litodp`/`dptoli`), and its derived
span had swallowed this TU's own unnamed copies of the `VIMap<int,int>` members (the
named ones live at 0x00415E08). It needed a `SPAN_CAP` of 516 bytes.

### What decided them

- **Block-scope struct locals share stack slots by an exact rule.** gcc 2.95's
  `expand_decl` gives a fixed-size BLKmode local `assign_stack_temp(..., keep=1)`:
  exact size first, else the smallest larger free slot (split when 16+ bytes remain),
  and slots freed at a block's end are combined with free neighbours. With each switch
  arm's locals in their own block, in order, every offset in all four buffer parsers
  came out right — including two locals the demoted Flora arm declares and never uses.
  A promoted `short`/`char` local goes to the stack the moment its address is taken;
  an `int` is ADDRESSOF'd and lands at the end of the frame in first-use order. (Read
  out of the 990721 gcc source in `decomp/toolchains/src/`.)
- **`End()` once per case, before `break`** — cross-jumped into one call with the `this`
  move in the JAL's delay slot. One `End()` after the switch lets the earlier branches
  steal that move. Where `Begin()` precedes the switch, `End()` follows it.
- **Enum fields come off the wire through a scratch int** (`Read(t); packing =
  (VIPrimBufferType)t;`), and a default goes on the else path.
- **The pre-translation array pointer is assigned inside the condition**:
  `if (m_zone && (pt = &m_zone->m_pretrans)->m_size)`.
- **`VIColor32 c(0, 0, 0, 255)` stores `li -1`; `VIColor32 c` stores `li 255`.**
- **A case-0 arm turns `k = 0` into the switch register** (cse knows the index is 0);
  counters declared once at function scope avoid it.
- **Read a run off a pointer, store by member** (`ParseHSpriteAnimObj`'s 1/512 run).
- **The packed vectors (`VIVect3S16`, `VIVect2S16`, `VIVect3S8`, `VIVect4S8`,
  `VIVect4U8`, `VIVect4S16`) have zeroing constructors** — proved by the unnamed
  VIPrimBuffer static initialiser and by every packed local's `sh zero` at block entry.

## Orphan TUs closed by the coordinator

- **`VIPool<T>::Allocate`, 17 instantiations** at once: the element copy is placement
  new **declared `throw()`** — gcc 2.95 null-checks a nothrow operator new — and not a
  hand-written `if (d) *d = ...`, which creates the two induction pointers in the other
  order. Declaring `throw()` in `viarray.cpp`/`vivector.cpp` regresses them, so each
  template TU keeps its own placement new. `VISet<VIWnd *>::Allocate` closed the same way.
- `VIFile::CDRead`, `CDSync`, `CDSearchFile`: one shared return at the bottom (if/else
  instead of an early return) moves the `li v0,1` the scheduler was placing first.
- `VIRFont::BeginChar`: the base pointer first in the address add.

## Open in VIESFParse

`ParseSpellEffectObj` 44/276 (pure register renaming; declaration order changes
nothing, so the pseudo priorities genuinely differ), `ParseObject` 7, both
`Parse*LODSpriteLevels` 12, `ParseHSpriteAttachments` 18 (the in-loop `return -1`
blocks — see below), `ParseGroupSpriteMembers` 46, `ParseZoneResource` 41 (was 50:
the no-raster arms call `ReadEnd()` and ignore it), `ParseAdpcmObj` 140.

**The `return -1` placement, narrowed:** the RTL dumps show loop.c's
`find_and_verify_loops` moving every in-loop `return -1` block out of the loop, after
which jump2's cross-jump keeps the moved copy. The target behaves as if the motion never
ran — its last in-loop block stays inline and earlier failures cross-jump into it. The
motion is skipped when something else jumps into the block, but a `goto` into the loop
also invalidates the loop (strength reduction disappears), so that is not the source.

## Tier 1 complete (2026-10-13, ~23:55 UTC)

After every lane was merged and synced (fx for the first time, flora a second time),
the whole tree read **3,508 of 3,756 byte-exact, nothing lost**, and the census found
exactly one unwritten SUPPORT function: `VIRStrip6`, 14 instructions. It is not code.
The `VIRStrip*` names are VU1 microprogram labels (`VIRaster::RStrip*Offset` subtracts
them from `VIRInit`), and `VIRStrip6`'s value equals the entry point of
`VIEEUnloadIrx`, which has been byte-exact since phase 27. Putting it in `SPAN_IGNORE`
would have removed the shared address and the function's span with it, so the harness
gained a second table, `SPAN_ALIAS` (name → the real function it shadows), with its own
test and a control. **Every SUPPORT function is now written**; 178 remain open
(140 KB), and the rest of the phase is the grind.

`test_engine_fingerprint_profile` had been failing since the caps landed: 110 SPAN_CAPs
stop a TU's unnamed local code counting as part of the named function before it, which
cut the scan from 219,896 instructions to 189,583 and branch-likely from 2,486 to
1,822, both under the test's floors. The test now reads the profile with SPAN_CAP off,
since a build profile covers all compiled code.

`_lane.py sync` copies the tools over a lane's own, and the 3-way merge never carries
SPAN entries back, so `sync` now refuses when a lane's `eqdecomp.py` holds a SPAN entry
main lacks (negative control: a fake cap is refused, and the restore syncs).

## Byte-exact is not correct: the callee check

The scorer masks every field the candidate's own relocation table names, so any `jal`
matches any `jal`. The UI lane found the first case by accident: `VICamera::Update` was
"exact" while calling `VIMatrix44::operator=`, which does not exist in SUPPORT. The target
calls `sceVu0CopyMatrix`. `eqcallcheck.py` lines up every R_MIPS_26 in our objects with the
callee the relocated dump names at the same offset (`eqcalls.calls`). Its first
whole-tree run found **110 mismatched call sites in 84 matched functions**: 59 call a
different SUPPORT function, and 51 call a name the map does not have. All are fixed and
still byte-exact. The real errors:

- **Every template `DestroyPool` walked the pool with `Erase`.** The original walks with
  `Next` and destroys in place. Three `Erase`/`EraseFixup` instantiations we emitted were
  NO-TGT (the target never had them), and they vanished with the fix, which confirms it
  independently.
- **VIXm's `spu`/`xm` fields were swapped.** `Create` allocates SPU memory first; +16 is
  freed by `FreeSpuMem`, and the device's `BeginAdpcmTransfer` sends the sample there.
- **`VISimpleSprite::Raster`/`Pick`'s extra argument is a colour buffer**, the scene's
  next STATIC-LIGHTING buffer (`VIScene::GetNextColorBuffer`). It was read as a LOD/dither
  level through a function that does not exist.
- **VIRaster:**
  - `EndScene` ends in `ReleaseDeferred`, not `BeginScene`, and its second call is
    `Flush`, not `Flip`;
  - `Flush` is `FlushCache(0)` + `VIEEDMASend`;
  - `Viewport` ends in `SetGSScissoring`;
  - `BeginTriStrip`/`Fan` recompute and upload the matrices, not the fog;
  - `CalcClipToScreen`'s identity is `VIMatrix44::Identity`;
  - `DrawPrimBuffer`'s PackVU arm calls `DrawPrimBufferPackVU`;
  - three `delete`s are `delete []`.
- `VIQuat(D)::ConvertTo(euler)` is `Rotate` + `ConvertToEulerZXY`, not `UnitRotate` + XYZ.
  `VIMatrix33`/`44D`'s default ctors call `Zero`. `VIPickSegment::Init` uses `SubMatrixMul`.
- **`new []`/`delete []`** (`__builtin_vec_new/delete`) where we had an invented
  `VIMemAlloc`/`VIMemFree`. The old comment said `delete []` would add a second null test;
  it does not.
- `VIString`: `strncpy` not `memcpy` (three sites), `strcat` in `Concat(const VIString &)`,
  `malloc` in `strdup16`, and the libc names without the `_` prefix.
- Smaller fixes:
  - `ChooseOrbitAngle` calls `FindBestOrbitAngle`.
  - `SetCamera` calls `CalcFarPlane`; `GetViewDistance` does not exist.
  - `~VIWave` calls `Deallocate`.
  - `VILNode`'s hash is `VIHashRefID`.
  - `ParseSoundSpriteObj` reads an `int`.
  - `VIHashResourceID_Icon` and `VI(Init|Free)USocketExApi` have C++ linkage.
  - The sprites' palette calls are `Share*`/`GetMaterialPal*`, and `ReleaseMaterialPal`
    takes the dictionary, which is already in `a2`.

A call's extra arguments can cost nothing. `ConvertTo(q)` was really `ConvertTo(q, b, c)`:
`b` and `c` arrive in `a2`/`a3` and are never moved, so the bytes cannot tell the two
apart. Only the relocation can.

`_tu.py` now also reports a matched function that stops being EMITTED. A template member
is only instantiated when it is used, so a source change can silently drop a matched
function that the row loop never sees.

## The latency hypothesis, tested on the real compiler

Several lanes had filed their stuck functions under "the R5900 load ready-delay divergence"
(phase 8c). A rig agent patched a COPY of the real `ee-gcc2.95.3-107` `cc1plus.exe`.
`insn_cost`'s call to `result_ready_cost` now goes to a 56-byte stub that returns 1 for the
r5900 load rows only. That was verified by interpreting the patched function over every insn
code × alternative: 653 rows change and nothing else. Then it scored the whole tree.
**Latency 1 breaks 1,520 of 3,510 matched functions and closes none of the 176 open.**
Store, xfer and FPU single-number changes were all strictly worse. The settled compiler's
latency model is the target's. The remaining instruction-order residue has a narrower
cause: ready-list tie-breaks, or how argument loads feeding a call are costed.
`decomp/README.md` (Phase 8c addendum) has the table and `decomp/rig_lat/` the tools.

## Alias analysis (the viwalkcontrol question) and ParseSpellEffectObj

`ParseSpellEffectObj` 44 → 22. One function-scope `int k` serves both the ParseObject
sub-loop and the three-list loop. The target's allocation puts the first loop's counter in
the same register as the OUTER loop's, which is what one pseudo does. Two swapped pairs
remain (`ver` against the hoisted constant 1, and `&version`/-1 against `l`). Declaration
placement and four spellings of the version tests do not move them.

## The code the map does not name

The map names one address per symbol, and the census counted only named functions. So
"every SUPPORT function written" said nothing about the 110 SPAN_CAP'd regions:
**124,832 bytes**, about 14% of SUPPORT's code, that nothing scored. `eqlocal.py` searches
those regions for each of our unnamed functions with the scorer's masked compare. It also
classifies what is left by byte identity against the named functions:

- **~107 KB are byte-identical COPIES of named template members.** The original build
  emitted every implicitly-instantiated `VIMap`/`VIMultiMap`/`VIPool`/`VIArray` member
  into each TU that used it, and the map names one copy. Our TUs call a single explicit
  instantiation instead, so they emit none. This is a build-structure difference, not
  unknown code. A relinkable build would need implicit instantiation per TU in the
  original order.
- **13 KB is matched by 41 of our 65 unnamed functions** (drdp statics, `AppendString`×4,
  `icmp`, static initialisers...).
- **~5.5 KB is unique and unmatched**, and it lines up with our 24 unnamed functions not
  found yet: the PSS player's helpers and callbacks, the sound device's
  `SoundCall`/`SoundCmd` statics, two `usocket` and two `buffer` internals,
  `connection_init`, `CdStreamDone`, and two static initialisers. That is the real
  remainder of SUPPORT beyond the named opens.
