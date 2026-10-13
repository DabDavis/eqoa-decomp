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
