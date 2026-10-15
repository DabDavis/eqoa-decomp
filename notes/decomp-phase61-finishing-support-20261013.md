# Phase 61 — finishing SUPPORT (closed 2026-10-15)

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

## Data references: applied

`eqrefcheck.py`'s findings were all applied in one lane and merged; **matched functions go
from 261 findings to 0**, with nothing lost.
- **The 14 data/behaviour bugs:**
  - `VIPixelStore3` shares the RGBA32 stride;
  - `VIBitDepth` is numbered 4=0, 8=1, 16=2, 24=3, 32=4, and 3 and 4 share the RGBA32 body;
  - `VIPrimBuffer::End`'s PackVU arm uses `RStrip6Offset`;
  - `sDMASyncSkip` starts at 1;
  - the camera-shake tables;
  - `~VISound` is PURE (all six vtable slots are `__pure_virtual`);
  - the `VILanguage` name/ISO tables;
  - the memory-card product id `BASLUS-20470EQONLINE` and title "EverQuest Online\nAdventures";
  - `game.opt`;
  - three-letter day and month names with a `"???"` tail;
  - a UTF-16 `(null)`;
  - the `hostname` global;
  - eight separate `VIFont::sm_FontID` statics, one per function.
- **Every debug/printf string.**
- **`#line 1 "C:/code-frontiers-gm/<dir>/<File>.cpp"`** at the top of the 12 TUs whose asserts
  carry `__FILE__`. The original was built from `C:/code-frontiers-gm/{core,drdp,game}/`.
- **Every assert's expression text.** The DRDP layer now uses the original identifiers the
  asserts reveal: `result`, `connection`, `route_table.entries`, `endpoint_block`,
  `time_last_flush`, `MESSAGE_TYPE_SYSTEM`, the `f( x )` spacing, and so on. The inline
  `find_leader_index` was `drdp_find`, in drdp.cpp itself.

Both of the last two were closed in wave 5 (0 findings whole-tree, both still byte-exact):
- `drdp_master_send_route_table`'s assert names `buffer.size`: our `position` (+8, the write
  cursor `buffer_get_position` returns) IS the original `size`. The capacity (+4) became
  `max_size`, a name of ours since no string reveals it. It was renamed first, so every old use
  failed to compile rather than silently changing meaning. The binary also keeps an unreferenced
  name table after connection.cpp's `cstat_text`, `{ 0, "MESSAGE_TYPE_SYSTEM",
  "MESSAGE_TYPE_FRAGMENT", "MESSAGE_TYPE_GUARANTEED", 0 }` = 249..251, so those are now
  `#define`s and the source uses them; only 248 and 252 keep names of ours.
- `VIPad::Read`'s never-read 32-byte local is a `const char *[8]` of libpad state names
  (`"DISCONNECT", "FINDPAD", "FINDCTP1", "", "", "EXECCMD", "STABLE", "ERROR"`) copied from a
  template at 0x4ED088. A block aligned below 8 is copied with `ldl`/`ldr` either way, which is
  why the bytes could not see the difference.

`eqcheck` fixes that came with it:
- The duplicate test now masks each copy's own relocation fields. A pure `~VISound` makes
  `viwave`/`vixm` emit weak `__tf7VISound` copies that differ only in their addends.
- TU-LOCAL symbols are keyed by TU. Twelve "duplicate" `__static_initialization_and_destruction_0`
  rows were twelve different functions.

### Unnamed code: down to one function

**64 of our 65 unnamed functions are now exact**, plus 6 per-TU copies of named RTTI
functions exact at their layout (`eqlocal.py` credits a named function scored at its own
TU's layout address inside unnamed code). The unique unmatched code is **620 bytes, all of
it `_setLoadImageTags`** in the PSS player. Everything else in the 124,832 unnamed bytes is
accounted for:
- 18,020 bytes are matched by our functions;
- 106,440 bytes are byte-identical copies of named template members that our TUs do not emit
  (implicit per-TU instantiation in the original build);
- 288 bytes are alignment nops.

The last pass found:
- the 396 "unknown" bytes after `uns64_range_t_cmp` are buffer.cpp's first two statics;
- function order within a TU is a source fact (sce_playpss);
- store order through one pointer is a source fact (`connection_init`).

### Unnamed code: complete

As of 2026-10-14, **all 65 of our unnamed functions are byte-exact**, the last being
`_setLoadImageTags` in the PSS player. loop.c's hoisting threshold decided it: a
post-increment argument costs a copy per call, so `t++` as its own statement grows the loop
past the count. **SUPPORT has no unique unmatched code left:** every one of the 124,832
unnamed bytes is either matched by one of our functions (18,636), a byte-identical copy of a
named function (106,440), or alignment padding (292).

eqrefcheck's MERGED class also caught a structural slip. `gSndXm` (the XM channel table) and
`gSndStatus` are ONE target object, because the channel pairs are words 8.. of the RPC status
buffer. It is now a view: `#define gSndXm (*(VISndXmState *)gSndStatus)`.

## Waves 5–7 (2026-10-14): the rules that closed the residue

### Wave 5: scheduler and jump

`ParseObject` 7 → 0, `VIEffectVolume::Render` 6 → 0.
- **Which `return -1` copies survive depends on what falls into the return label.** jump2's
  cross-jump compares each `b END`'s tail with the code just before `END` and needs ONE
  matching insn: a bottom `fail: return -1` absorbs every `li v0,-1; b END`, a bottom
  `return 0` leaves the -1 copies, and a labelled -1 block folds into an earlier unlabelled
  copy. `ParseObject`: every case `if (ParseX() < 0) return -1; break;`, the default
  `if (ReadEnd() < 0) return -1; break;`, and one `return 0;` after the switch.
- **A member store is flagged in-struct; a store through a cast pointer is not.** An in-struct
  `sp` store never conflicts with a fixed global's load, so sched1 lifts the load above it and
  the later stores' anti-dependences raise its priority. `VIEffectVolume::Render` keeps the
  dependence with `((float *)&b)[3] = ((float *)&b)[7] = ... = 0.0f`.
- **Ready-list order, checked on the compiler's own dumps (0 violations in 762 pairs):
  priority, then dependent count, then original order** (later rounds add sched1's register
  weight and class). A call is a clobber that does NOT clear earlier register uses, so
  `move sX,aY` gains a dependent per call before `aY` is reset, and `sq ra` gains one per call
  plus `lq ra`/epilogue/return. A move jump2 later deletes still counts in sched2; sched1 links
  carry into sched2; blocks outside loops are never scheduled together; `s8` is the hard frame
  pointer (reg 30), so a load through it has an unknown base in sched2
  (`decomp-scene/scratch/w5/`).
- Prologue ties (`Text`, `usocket_sendto`, `vsnprintf16`) would match if the epilogue were a
  separate block in sched2; no source construct was found that splits it. Wave 6 closed them by
  dropping `return` on the tail call.
- Open: `SetupTimeOfDayLights` (sched1 prefers the store where the address register DIES);
  `VIRaster::Flip` (combine keeps `& 1` only if `odd` is set twice, live at entry, or 64-bit);
  `CalcCacheInfo` (cse folds `int one = 1`; wave 6 traces it to `update_equiv_regs`);
  `CalcTerrainCellBVolume` (reorg fills the `bne` slot with `addiu sp,-16`; closed in wave 6).

### Wave 5: register allocation

Eight closed (`EndBillboards`, `DrawCenteredString` ×3, `CalcCenteredStringScreenRect`,
`TessTriangle`, `Vertex`, `VIEESurfCache::Allocate`), and `VIEESurfCache::Deallocate` 186 → 14.
- **madd fusion depends on the addend's first use** (`r5900_madd_profitable_p`): combine fuses
  only when the add is NOT the addend's first use after its load. Computing
  `p = (char *)(m_dmaTag + 1)` first fuses it, and reload splits it into `mult a0` + `addu`
  (`EndBillboards` 13 → 0).
- **An if/else round-up ends cse's extended block; a plain `if` does not** (cse skips an
  if-then and keeps its table). A target that recomputes after the round-up was an if/else, its
  `move` in the `beqz` delay slot the else-arm copy (`Allocate` 74 → 0, `Deallocate` 186 → 69).
- **Compare a rounded value inside its own `if`**: the target's `beqz` goes straight past the
  split with `if (s & 8191) { n = ...; if (n != s) ... }`.
- **Reuse ONE variable for results in the same role** (`Prev` and both `Insert` results in
  `k`, both `Next` results in `q`): the merged live range outranks its rival (`Deallocate`
  69 → 14).
- **A variable set in two loops is global**, never tied to the madd sum: one `glyph` shared by
  both loops gives the width loop's three locals `v0`/`v1`/`a0` (`DrawCenteredString` ×3).
- **Copy a struct through `operator=` to keep stores unforwarded**: in
  `CalcCenteredStringScreenRect` (29 → 0) `r.max = r.min; r.max.x += w;` re-reads `r.max`, and
  the extra refs raise the base's priority; a target `move rX,rY` right after storing `rY` is a
  re-read that reload_cse rewrote.
- **On a scheduler priority tie the insn emitted first in SOURCE wins**: move a statement up to
  shorten a pair's lifetime (`Vertex`: compute `hdr` before reading `g`, 20 → 0).
- **The register-pressure tie favours an insn whose input DIES over a load**, so source order
  alone cannot put a load ahead of such a shift (`SetSoundTrack` block 9).
- `TessTriangle` 88 → 0: a `TessUV(VIVect2 &, const VIVect2S16 &, float)` helper like the
  normal and colour ones.
- Open: `Deallocate` 14 (the global -8192 must take `a0` before `s & 8191`; gsim reproduces the
  target with it allocated earlier); `SetSoundTrack` 6 (block 36: no legal order gives `m_time`
  `a1`, so the code differs); `TextBox` 32 (`i`, 30 refs/498, loses to the split -1 constant,
  19/285; one more in-loop ref to `i` fixes it but changes the fit arm); `WriteBegin` 25 (two
  registers for the constant 1, split by role, which cse merges); `CalcProfileBlend` 37 (the FP
  `at.y`/`v.y`, 3125, must beat `origin.x`, 3333, and the int -1 constant overlaps the second
  multiply's scratch; closed in wave 6); `VICSprite::Copy` 182 (read then as registers only, six
  memcpy-loop addresses `a0`/`a1`/`s0`/`t5`/`t6`/`t7` vs `t5`/`t6`/`t7`/`s0`/`s1`/`s2`; wave 7
  finds the small copies' expansion differs too). Tools: `decomp-core/scratch/w5/` (`sdiff.py`,
  a register-masked structural diff; `lsimlib.py` re-runs local-alloc on reordered blocks).

### Wave 5: loop, gcse and cse

`ConvertTo` (float and double), `GlobalInit` 157 → 0, and `route_cache_touch` as a FAKEMATCH,
counted but labelled in the source and listed in the README.
- **Probe with one disabled pass at a time** (`decomp-fx/work/w5/tools/flagsweep.py`): a
  `-fno-*` that makes a function exact NAMES the pass (`-fno-force-mem`: `ProcessPlayList`
  130 → 36). It is not the answer: `-fno-thread-jumps` closes `route_cache_touch` but loses 20
  exact whole-tree, so it was not the DRDP library's build.
- **`thread_jumps` redirects `if (x) goto L` past a re-test of `x` unless an INSN sits between
  them at BOTH thread passes** (before cse1, after cse2); notes, unused labels and dead copies
  do not count.
- **A CONSTANT float argument keeps an inline setter/ctor's `this` pseudo for that store**
  (`try_constants` fails); a computed one folds onto `sp`, and at offset 0 cse folds either
  way. Where the target's stores are `sp`-based, pass a variable (`GlobalInit`: `-h`).
- **A pointer named once at the top is copy-propagated away**; taken afresh in a later block,
  PRE and cse2 give the target's `addiu X,sp,K; move Y,X` (`ConvertTo`).
- **Index (`(&inv.x)[i]`) where the target's pointers start as copies before the loop**:
  loop-reduced walkers, counter reversed, increments in reverse discovery order.
- **Spill slots go to pseudos in creation order, a reference's before its initialiser's
  temps**: declare a shared subexpression EARLIER for the lower slot.
- gsim: preheader start-value order drives register order (`ParseGroupSpriteMembers`: `y`
  before `x`); every spelling tried canonicalised the same.
- Open: `__ml__` `V3*M44` 7 (a surviving entry copy of `m` would fix sched, but `m` is used
  once and combine merges it); `ConstructTextPass` 62 (`if (fl & 4)` on an int copy gives the
  branch exactly but nets 76).

### Wave 5: structure, two rounds

`DrawPrimBufferTessPackVUNCG` 336 → 0, `VICSpriteCust::SetResources` 296 → 0,
`Collide(sphere,V3,V3)` 27 → 0, `SetAnimation` 168 → 0, `VIColorBuffer::Color` 186 → 14; in
round 2, both natural, `Color` 14 → 0 and `playPss` 33 → 0.
- **A pointer sum is built pointer-first whichever side it is written on**: cast the pointer and
  write the integer first (`BuildVQSource*16 + (unsigned)m_verts`: `Color` 104 → 14).
- **No `move v0,zero` plus a small length difference: the function returns `int 0`**
  (`SetResources` was `static void`; it has no callers in SUPPORT).
- **cse follow-jumps/skip-blocks need labels used EXACTLY ONCE**: `goto fail`/`goto done` and
  one `||` instead of returns, which also moves global-alloc's `floor_log2(refs)*refs/live`
  (`SetAnimation`).
- **sched1 tie-break: priority, register weight (sets minus deaths, lower first), dependents
  (more first), source order.** A post-increment adds dependents to its load; of equal-priority
  stores the one whose base DIES goes first (`[0],[1],[2],[3]` → `[0],[3],[1],[2]`).
- **local-alloc orders by `floor_log2(refs)*refs/life`, then birth order**: shorten a life for
  the lower register.
- **cse takes its 0.0 from the first-inserted MEM of equal cost**; default-construct-then-assign
  changes which (`Collide`: `e` default-constructed then assigned, one `k` reused for the
  projection).
- **A field store whose POINTER type matches a global's** reproduces the target's reloads
  of that global without blocking unrelated int loads (`Color`: tags through a pointer `next`).
- **Reload hands out spill registers ROUND-ROBIN** (`last_spill_reg`), so a structurally right
  variant can differ only in reload registers (`playPss`: the right movables order scored 50
  vs 33, reverted).
- `DrawPrimBufferTessPackVUNCG`: gcse PRE copies `i+1` into the arms in hash-bucket order, which
  sets spill-slot order; `prims++` at the END of the `k` body fixes it, and a static inline
  taking `VIBBox &` makes the `sp+0` box's address a shared pseudo. A real bug went with it: a
  prim is `32 + 32*n + 16` bytes.
- **A struct-member store at a register address does not invalidate a fixed scalar global in
  cse**: `x[g++] = w` reuses the loaded register where `x[g] = w; g++;` makes a NEW pseudo
  (`Color` 14 → 6; then integer-first `m_pad1C * 16 + (unsigned int)m_tags`, 6 → 0).
- **sched never grants that exemption**: alias.c's anti/output checks use `rtx_addr_varies_p`,
  which counts only an address containing a MEM as varying.
- **haifa issues an insn that becomes ready mid-cycle no earlier than the NEXT cycle**, even at
  dependence cost 0.
- **loop.c hoists a constant held in a named local declared AHEAD of an earlier statement** once
  its life clears the threshold (~50 × savings × life ≥ loop insns); with no callee-saved
  register left, reload REMATERIALISES it at its use (the target's `v0`/`a0` reuse). Inline it
  stays block-local (`playPss`: one block per circuit).
- **The first-moved movable (the earliest-declared pointer) is the one loop.c hoists and
  global-alloc spills.** `decomp-raster/work/w6/topo.py` replays local-alloc over every legal
  order of a block and prints the orders giving a requested register set.

### Wave 6: scheduler and jump

`Text` 3, `vsnprintf16` 2, `usocket_sendto` 2 and `CalcTerrainCellBVolume` 36, all → 0.
- **No `return` on a tail call** (all three prologue ties): `return f(...)` leaves a no-op
  `v0 = v0` before the final USE, which ends `add_branch_dependences`' trailing run. With
  `f(...);` every earlier body insn links to the call, so in sched2 each has one more dependent
  than the prologue insns and wins the tie with `sq ra`/`move s8,sp`. Suspect it in one-block
  functions ending `return f(...)` where the target puts a body insn before a prologue insn.
- **An inline helper's reference parameter bound to a member expression is computed AT THE
  CALL** as an unflagged pseudo, so reorg takes it for the delay slot ahead of the prologue's
  `addiu sp,-16` (`CalcTerrainCellBVolume`: static inline `CTBVOffset(bb.min, bb.max)`).
- **sched1's `adjust_priority`**: an insn made ready by a resolved dependence that sets a
  register LIVE AT BLOCK END, set once, jumps to the block's maximum priority (REG_DEAD notes
  are stripped first). The rank is priority, register weight (+1 per set, -1 per death note),
  class, dependents, source order, which is why gcse's address inserts run first at priority 1.
- **reorg's backward scan stops at a label, jump, barrier, asm or filled slot, and a volatile
  memory reference in between rejects all**; it then tries the predicted thread (`bne` taken
  unless it leaves a loop; EQ tries the fall-through first). After an empty asm or a volatile
  read it fills from the taken path.
- **`update_equiv_regs` replaces only the initialiser of a pseudo used across blocks and
  referenced exactly TWICE.** It runs after sched1, so sched1 never sees the constant in the
  use's block.
- **combine's three-insn merge of `i = 0` into a copied loop-exit test re-emits `i = 0` at the
  end of the block before the loop**, wherever the source puts it (narrowed in wave 7).
- Open: `SetupTimeOfDayLights` 5 (`sp+92`, the inline `VISphere` ctor's `this`, must be live
  out of block 0 and set once for `adjust_priority` to hoist it); `VIScene::Render` 45 (the
  camera store first gives the store order); `CalcCacheInfo` 23 (`update_equiv_regs` replaces
  the two-ref constant 1; a third ref spreads into the second switch); `VIRaster::Flip` 13 (a
  64-bit `odd` keeps the mask but adds `dsll32`/`dsra32`: 19). `VICdStreamInit` 6 and
  `ProcessTransitions` 6 closed in wave 7.

### Raster round 3: the last eqrefcheck findings, and libgcc2_main

The `buffer_t`, `MESSAGE_TYPE_*` and `VIPad::Read` fixes are under "Data references: applied".
Two more facts: lane code written before the rename that says `buffer.size` meaning CAPACITY
still compiles and reads the cursor (a MERGE TRAP), and the binary also names CONNECT 255 and
DISCONNECT 253, leaving only 252 (best effort) and 248 (the state-channel count) unnamed.
**`__do_global_dtors`/`__main` were built as C by the ee-gcc 2.9 family (990721/991111) at
`-O2 -G0`**: 2.95.3 gives 18 (C) or 19 (C++), and the tell is 990721's `sd`
into 16-byte save slots where 2.95 uses `sq`. Not a flags difference; it needed a per-file
toolchain and language override, now `eqcheck.TU_OVERRIDE`.

### Wave 6: register allocation

`CalcProfileBlend` 37 → 0; five diagnosed.
- **Win a sched1 tie by moving a LAST USE** (a tie goes to the insn freeing more registers,
  then to more dependents): `CalcProfileBlend` computes `s = (float)m_subdiv / m_cellSize`
  after the two half-cell adds, making the divide `m_cellSize`'s last use (dependents 6 vs 3),
  and the insn now between the two r5900 multiplies stops their HI/LO scratches overlapping.
- **Same-valued constants on one cse path take the FIRST register, permanently**; a split by
  role needs a path break plus a variable, not two literals (`WriteBegin`: a local `one` and a
  second jump into the first join give 18/146, one extra insn; no natural spelling found).
- **Back-to-back r5900 three-operand multiplies overlap their HI/LO/HILO scratches**, the second
  falling back to even GPRs; one insn between them avoids it.
- **A 24- (or 12-) byte copy is ONE block-move insn whose temps are picked AFTER allocation**;
  16 bytes or less become values local-alloc places (move ratio 2). Wave 7 refines this by
  known alignment.
- **global-alloc preferences come only from a value's FIRST operand and are dropped across
  calls**; a higher-priority value skips a register a lower-priority conflicting one prefers.
- **With `this`/`d` possibly aliasing, field copies run in SOURCE order**, so a constant store
  between a field's load and store comes after that field's assignment (`VICSprite::Copy`:
  `d->m_race = m_race;` before `d->m_materialPal = -1;` fixes the first loop but nets 230 until
  the second loop's source pointer avoids `v1`, which only splitting `m_attach` forces).
- **sched1 rewrites live lengths, which global-alloc's priorities use.**
- Open: `VIEESurfCache::Deallocate` 14 (-8192 or `i*24` must prefer `a0`: an insn the original
  had and a later pass deleted); `SetSoundTrack` 6 (block 9's `m_time` load always loses the
  tie on register weight, +1 vs 0, so the block's dependence chains must differ; in block 36
  none of 24 orders gives `a1`, so `v1` must be busy);
  `TextBox` 32 (with `line = &s[i]` our `i` refs equal the target's 18 sites; the missing
  priority comes from code a later pass deletes).

### Wave 6: loop, gcse and cse (a Sonnet trial)

`Intersects` 23 → 0; `ProcessPlayList` 130 → 17, `ParseGroupSpriteMembers` 29 → 14.
- **A pointer assigned INSIDE the loop blocks gcse copy propagation**: set once before the loop
  it is a copy of PRE's reaching register that pass-2 cprop folds into every use (given the copy
  around the back edge); set inside, loop.c hoists `n2 = n` as a real COPY and takes `this`/the
  walker off it (`move s4,s6`). `Intersects` mirrors its first loop's `((VIPlane *)n)` spelling.
- **A pointer sum's RTL operand order is a source fact**: `&base[i + k]` is `(plus base idx)`,
  `(T *)((i + k) * sizeof(T) + (int)base)` puts the scaled index first, cascading through sched1
  and the allocator. The `-fno-force-mem` lead was exactly this flip, on four sums and on
  fifteen the target keeps; `ProcessPlayList` flips the four.
- **Named-pointer DECLARATION order sets giv discovery order, so preheader init order, so
  registers** (`ParseGroupSpriteMembers`: `y, x, z, scale, hpr.y, hpr.x, hpr.z`, by sweep).
- **`eqcallcheck -a` on an OPEN function pairs calls by OFFSET**, so a length difference reads
  as a wrong callee: `ParseAdpcmObj`'s "`LockBuffer` vs `Read`" was ours being 16 bytes short,
  with identical call SEQUENCES (wave 7). Compare the sequences before believing a mismatch.
- In source comments: `VIMaterial::Process` (the target's WrapY block is its own cse block;
  ours is merged by cse-skip-blocks, the label having one use once jump1 inverts the branch);
  `CalcPortalPlane` (the target zeroes `prev`, `last`, `sum` and `p`, but `p` is at `sp+0`);
  `BufferedRead` (the tail's `v0`/`v1` swap needs one more ref to `need`); `Open` (an `andi`
  surviving to jump2: the target's arms were not empty at flow).
- Traps: never kill `_perm.py` (it leaves `src/` permuted); `vt.py -k` keeps the variant in
  `src/`. The trial (~548k tokens, 60 minutes: one closed, two large cuts, mechanisms right)
  was comparable to the Opus lanes on the same residue.

### Wave 7: loop, gcse and cse (Sonnet, two rounds)

Nothing closed; `ProcessPlayList` 17 → 12 and `CalcPortalPlane` 22 → 10, both natural. No
single `-fno-*` flag improves any of the lane's eight functions.
- **Fakematch policy, refined: an inert barrier may be ADDED (`route_cache_touch`); source may
  never be RE-TYPED or restructured.** `CalcPortalPlane` closes only with `p` re-typed as raw
  `float pb[4] __attribute__((aligned(16)))` behind `#define p (*(VIPlane *)pb)`, zeroed after
  the other locals (target: slot `sp+0`, zeroed LAST). Rejected.
- `CalcPortalPlane`, natural: `(VIZonePortal *)(i * 32 + (int)m_portals)` (alone it netted 24
  in wave 5) and `j = pt->m_firstVert` with no `first` copy, whose COPY preference for `v1`
  (priority 5000 over `end`'s 1818 in global.c) had swapped their registers.
- `ProcessPlayList`, the blend block's FP registers: the translation subtract PER COMPONENT
  (`x`, scale, `y`, `z`; best of all 24 orders) gives 12 but loses the target's
  `addiu v0,s1,36`, which an inline `operator-=` with a reference parameter supplies.
  Unexplained: why the target's local-alloc ranks the scale last (ours: scale 25000, `z` 20769).
- Open, in source comments:
  - `ParseAdpcmObj` 127, register allocation only: 8 callee-saved to our 7; the target builds
    `half+56` as `move s0,s3; addiu s0,s0,56` (twice set, so global), ours as a one-set temp
    in `v0` (wave 5 read it as the 56 constant, global in `s1`, ours local in `s0`). The
    `(loop+1)*56` bound gives the loop structure exactly but scores 154.
  - `ParseGroupSpriteMembers` 14: our `s2` init is hoisted into the `jal` delay slot (`m` set
    before the loop); the target's sits after the loop guard, its slot `sw zero,12(sp)`.
  - `VIFile::Open` 19: `dev` is set in two arms, so combine cannot fold its `lo_sum` into the
    `a3` move and the high part becomes a one-set temp in `v0`.
  - `VIFile::BufferedRead` 20: `topo.py` finds NO legal sched1 order giving the target's `slt`
    pair, so the pseudo structure differs.
  - `VIMaterial::Process` 39: the wrap label is already multi-use.
  - `ConstructTextPass` 62: the target's first insn is `move t0,a1`; `i` loses `a1` to the
    test constants.
  - `ConstructDMAPacket` 90: the structure matches, but the tag (36 refs/9090) beats the
    walker (3684) to `t3`; the target is the reverse.

### Wave 7: register allocation (Sonnet)

Nothing closed; sharper diagnoses.
- **global.c's `set_preference` maps local-alloc'd pseudos through `reg_renumber`, so locals
  give and take preferences.** `Deallocate`'s -8192 (pseudo 334, priority 937) needs an insn
  pairing it (first operand or copy) with the local `a0` (the -1 feeding `b.m_free`) that
  existed at greg time and was deleted after: a no-op move, a reload_cse'd `li`, a jump2 copy.
- **cse follows or skips a branch only if its label has ONE use and the path has fewer than 9
  entries**, then re-scans with the last branch flipped; the first path's rewrites are
  permanent. `WriteBegin`: no linear path keeps test 1's constant for the `m_writeOpen` store
  but not test 2's.
- **SN's block-move expander: known alignment 8 → by-pieces, 16 → the grouped insn (`lq`),
  below 8 → `ldl`/`ldr`.** `get_pointer_alignment` folds `&d->m_x` to `(cast)d + off` and takes
  `d`'s pointee alignment (16) capped by the offset, so 16 for any cast; locals and references
  give 4, a struct assignment the member type's (4: `ldl`/`ldr`), a user `operator=` goes
  member-wise. An offset of 8 mod 16 gives by-pieces (`ld sd lw sw`; 24 bytes `ld sd` ×3,
  never `lq`), and only separate moves of 16 bytes or less give `lq sq ld sd`. **So
  `VICSprite::Copy`'s by-pieces pairs (`m_hair` and `m_snd` 8+4, `m_attach` 16+8) were TWO
  moves each in the original**: separately typed units, or two statements (the only other
  `lq`/`sq` pairs in SUPPORT are `BeginBillboards`' `VIRasterQW` assignments). Splitting gives
  182 → 174, fakematch-class, not applied; probes in `decomp-core/scratch/w7/`.
- `SetSoundTrack` block 9: `m_time` and the difference tie into one quantity (5714) and take
  `a0`; sched1 issues the two shifts first, their inputs dying. `TextBox`: `i`'s priority
  factor is 5 vs 4, and its life (762 insns) is the lever.

### Wave 7: scheduler (Opus)

`VICdStreamInit` 6 → 0, `ProcessTransitions` 6 → 0.
- **A global's `lui` is emitted when its NAME is expanded, the stored constant when its VALUE
  is**: a run of stores to separate globals ties in both schedulers, so source shape decides
  (`VICdStreamInit`: `int *send = sCdSend;` first, the four resets, then `send[0] = 1;` right
  before `CdStreamCall(0)`, as every other caller does).
- **Register weight is +1 for ANY set, stores included, -1 per death note**: a store killing its
  address register (0) always beats a hard-register argument setup like `li a0,0` (+1).
- **combine re-emits a folded loop counter only when the copied entry test READS it**: a guarded
  `if (n > 0) do { ... } while (++i < n);` keeps `i = 0` where written (`ProcessTransitions`:
  `i` cleared before `fa`, so `lw a1` takes the call's delay slot). The front end turns `n > i`
  back into `i < n`.
- **In a single-block sched1 region "live at block end" means "no death note in the block"**:
  both of `SetupTimeOfDayLights`' features follow if r124 (`sp+92`) does not die at the +12
  store; a function-scope reference crosses calls (`s6`: 71), a fourth use leaves a move (22).
  OPEN at 5.
- **MIPS's soft frame pointer `$1` is call-clobbered**, so every frame-relative insn after a
  call depends on it and the whole post-call batch becomes ready at once.
- **flow deletes a frame store that an identical later store follows with no read** (`VIVect3`'s
  `x` store inside `VISphere`'s ctor).
- **`update_equiv_regs` leaves a constant alone only with 3+ loop-weighted refs or a use in a
  loop.** `CalcCacheInfo`: `do { } while (0)` around the switch (or case 3) reproduces
  `li a0,1` in the delay slot and `sw a0,4(a2)` exactly, but adds `.p2align 3` (194 vs 192
  words). OPEN at 23.
- `VIScene::Render` 45: the target's frustum arm reads a copy of `&m_world` made before the
  clamp's branch (`move a1,s6`, likely reload_cse rewriting `addiu a1,s4,4`; wave 6 guessed a
  gcse reaching-register copy); our cse merges both arms into one pseudo before gcse.

### Raster round 4 (Opus)

`VIRaster::Init` 58 → 46; `VIEESurfCache::Set` 72 and `VIRaster::Flip` 13 unchanged.
- **In a run of independent stores the LAST statement's store carries the base register's death
  (and a shared constant's), so sched1 issues it FIRST** (lower register weight): statement
  order picks the winner. The target kept source order; a 4,041-order sweep
  (`decomp-raster/work/w6/init_anneal.py`) gives `Init` 46, applied and labelled a sweep result.
  Open: what lets the target's last stores avoid the death preference.
- **A store whose value register is overwritten later in the block gains sched2 priority** and
  issues first (the target's `[76]` before `FlushCache`'s `move a0,zero`).
- **Two `if`s on one test**: cse1 threads a default-then-override `if` into the next same-test
  `if` (join label used exactly ONCE) and folds the arm constants; `thread_jumps` needs no label
  between the tests and no pending register equivalence. `Set`: `m1`..`m3` inside the second
  `if`'s arms gives the duplicated arms and cross-jumped tail but leaves a redundant test (85).
- **combine's `nonzero_bits` uses the block's LAST set, whatever the set count**; a use in a
  LATER block keeps `& 1` only if the register is set more than once or live at entry. `Flip`
  needs a code label between `odd`'s set and the first test plus a second set, and jump deletes
  an unused C label before combine.
- **Coordinator trap: after ANY main harness change (`eqcheck`/`compile_cmd`, a `MATCHED`
  rebaseline), re-run `_lane.py sync` on an idle lane before resuming its agent.** Raster and
  scene started on stale tools (3,660, not 3,661) because the sync preceded `TU_OVERRIDE`.

## How it closed (2026-10-15)

**3,665 of 3,751 SUPPORT functions byte-exact, plus 65 of 65 unnamed functions.** Whole tree:
eqrefcheck 0 findings, eqlocal 0 unique unmatched bytes. One disclosed FAKEMATCH:
`route_cache_touch`, an empty `__asm__ __volatile__ ("")`.

21 named functions stay open, each with a diagnosis comment in its source:
`SetupTimeOfDayLights` 5/738, `SetSoundTrack` 6/394, `__ml__(VIVect3, VIMatrix44)` 7/33,
`CalcPortalPlane` 10/139, `ProcessPlayList` 12/532, `VIRaster::Flip` 13/85,
`VIEESurfCache::Deallocate` 14/288, `ParseGroupSpriteMembers` 14/114, `VIFile::Open` 19/93,
`VIFile::BufferedRead` 20/233, `CalcCacheInfo` 23/192, `VIObjFile::WriteBegin` 25/145,
`VIUIWindow::TextBox` 32/467, `VIMaterial::Process` 39/82, `VIScene::Render` 42/224 (was 45;
see below), `VIRaster::Init` 46/164, `ConstructTextPass` 62/274, `VIEESurfCache::Set` 72/205,
`ConstructDMAPacket` 90/228, `ParseAdpcmObj` 127/245, `VICSprite::Copy` 182/534.

**The permuter closed none of them.** `decomp/eqperm.py` (long form
`claude-notes/decomp-eqperm-textbox-20261015.md`) was built for this residue. An unattended
overnight batch (2026-10-15 01:36–08:49) searched the 20 opens other than `TextBox`. Its 16
reduced finds, by eqcheck's count:
- 5 strictly better: `SetupTimeOfDayLights` 5 → 4, `ParseGroupSpriteMembers` 14 → 12,
  `BufferedRead` 20 → 12, `Render` 45 → 42, `ConstructDMAPacket` 90 → 71;
- 4 equal;
- 7 WORSE: `ProcessPlayList` 12 → 19, `Open` 19 → 30, `VIMaterial::Process` 39 → 55,
  `VIRaster::Init` 46 → 58, `ConstructTextPass` 62 → 64, `VIEESurfCache::Set` 72 → 89,
  `VICSprite::Copy` 182 → 275.

**The permuter's own score (the alignment grade plus the allocation and length terms) does not
rank the way eqcheck counts**, so a "best" find can be worse.

All five better finds were rejected under the fakematch policy (an inert barrier may be added;
source may never be re-typed or restructured). They needed `decltype` temporaries, single-use
references, dead `pad` locals, no-op expression statements, a duplicated store, a restructured
if/else. **`BufferedRead`'s find also SWAPPED `BufferFlush()` and `BufferClear()`**: a behaviour
change the byte score cannot see, because `jal` targets are masked, and exactly what
eqcallcheck exists to catch.

Their NATURAL subsets were scored separately (the controls reproduced 5, 45 and 90):
- `Render`'s three plain reorderings (`v.m_world` first, `d` read before the zeroing run, the
  test spelled `d > 500.0f`) give the full 45 → 42 and were APPLIED, labelled in the source as a
  statement order from the batch, not proven original.
- `SetupTimeOfDayLights`' plain store reorder gives 9 (worse); `ConstructDMAPacket`'s "two
  complete arms" gives 93 (worse). Both rejected.

**The decision: no new lever, so the phase closes.** Every remaining open is a
register-allocation or scheduler tie whose deciding fact is a source shape not yet found.

Multi-module tooling was merged into `decomp/` from lane `decomp-mm` the same day: `EQMODULE`,
`eqmodule.py`, and `modules/` with the CLIENT, STATION, SETUP and MCERR maps seeded. Verified
neutral: test_eqdecomp 46/46; whole-tree eqcheck identical line for line to the previous run
(3,665/3,751, 0 regressions); `eqcallcheck -a` and `eqrefcheck -a` byte-identical to the
reference outputs.

Next: phase 62 opens CLIENT (1,100 KB, 5,973 functions, one exported name), starting with the
opcode dispatcher.
