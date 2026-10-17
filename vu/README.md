# decomp/vu — the VU1 microcode

SUPPORT carries all of the game's VU microcode as two DMA blobs, each a RET
packet holding a VIF stream of `MPG` uploads:

| Program | Symbol | Bytes | Pairs | Entry points |
|---|---|---|---|---|
| `raster` | `VIDMARasterMicrocode` `0x004E5880` | 16,320 | 2,029 of VU1's 2,048 | `VIRInit` `VIRMaterial` `VIRFan` `VIRStrip`…`VIRStrip6` |
| `billboard` | `VIDMABillboardMicrocode` `0x004E5200` | 1,664 | 205 | `VIBbInit` `VIBbRender` |

There is no VU0 micro-mode program. VU0 macro mode is inline asm in the EE
source, so it is already part of the decomp.

## The source is `src/*.vsm`, and it MATCHES

```
./vudis.py check      # assemble src/*.vsm, compare with the SUPPORT image
./test_vudis.py       # the canary: 10 tests, ~125k checks
./test_vusim.py       # the simulator's canary (below)
./vudis.py extract    # regenerate src/ from the image (refuses to overwrite; --force)
./vudis.py dis <lower> <upper> ...   # one-off pairs
```

Both programs assemble back **byte for byte**, wrapper included (DMA tag, VIF
NOPs, every `MPG` header), with **no raw pairs**. Microcode is hand-written
assembly, so unlike the EE side there is no compiler to match. Matching is a
round trip, and every decision left is about *understanding* the code.

`.vsm` is one line per instruction **pair**, upper first, dvp-as spelling:

```
    mulax.xyzw acc, vf01, vf20x          iaddiu vi05, vi05, 11      ;   38
```

Comments are free (`check` ignores everything after `;`), so **the files are
hand-maintained source from now on**: annotate them in place, and run `check`.
Labels are free too. `extract` generates `L####` names from branch targets; every one of
them has since been renamed for what the code does (see "Labels" below).

## How the disassembler is held to the hardware

- **Tables against PCSX2's interpreter.** `test_tables_match_pcsx2_interpreter`
  parses `pcsx2/VUops.cpp` (the tracer's own source tree) and compares every
  slot of every table. It compares against the interpreter, not the debugger:
  `DebugTools/DisVUmicro.h` labels the four `SUBA{bc}` opcodes `SUB{bc}`. The
  negative control plants that bug and expects to see it.
- **Canonical or raw.** A pair decodes to text only if that text re-encodes to
  the same 64 bits, and anything else is written `.pair 0xLOWER, 0xUPPER`. So
  `assemble(disassemble(x)) == x` for any input (tested on 20,000 random
  pairs), and a field the syntax fails to show cannot hide. That check found
  the one real gap: `eleng`/`erleng` carry a dest (`.xyz`).
- **Labels against control flow.** Every entry point but the first comes right
  after an `[e]` or an unconditional `b`, or sits in its delay slot. A label
  mapping off by one fails that check (negative control included).

## The simulator, and the oracle it is held to

`vusim.py` runs a VU1 microprogram from a captured state. **Over a live capture of 2,999
launches (`VIRInit`, `VIRStrip2`, `VIRStrip3`, `VIRStrip4` → `VIRStrip`), every VF and VI
register, ACC, Q and P, and every one of the 611,537 memory words the programs wrote, agree
with PCSX2 bit for bit** (`./vuconform.py <capture> --pcsx2`). **Eight more captures
(2026-10-17, below) add 19,592 launches**, which match the same way. They cover every entry point
of both programs, the billboard program included.

```
./vuconform.py <vu1dump.bin> [--pcsx2] [--entry PC] [--first N]   # vs the next launch's state
./test_spec_strip.py <vu1dump.bin> --pcsx2     # VIRStrip's reading vs the simulator
./test_vusim.py                                # canary: float core vs exact rationals,
                                               # a 400-launch subset, controls (~40 s)
./test_vu_captures.py                          # canary for the 2026-10-17 captures (~3 min)
./vucov.py <capture> ... --pcsx2 [--pcs]       # which pairs of each routine a capture runs
```

**The data path is sequential** (microVU interlocks every VF/VI read-after-write). What is
not interlocked is modelled by CYCLE, transcribed from `pcsx2/x86/microVU_Analyze.inl`,
`_Compile.inl` and `_Flags.inl`, and every item below changed a result when it was wrong:

- **Q and P are two instances.** A DIV (7), RSQRT (13) or EFU op (ERLENG 24, ELENG 18 …)
  writes the other instance, which becomes current when its counter runs out. The strips
  read them at exactly that distance (`div` 343 / `mulq` 350, `erleng` 338 / `mfp` 362).
  The negative control is a DIV latency of 8; 6 breaks nothing, correctly.
- **MAC and clip flags are visible 4 cycles after the FMAC that set them.** The backface
  test `fmand` reads the `opmsub` cross product exactly four back.
- **VI latency is 1, but 4 after `ILW`/`ILWR`**, and that stall is what switches off the
  branch VI-delay rule at 1051 (microVU skips the rule on a branch that stalls).
- **OPMSUB uses OPMULA's index pattern**, so `opmula acc, a, b; opmsub d, b, a` is a × b.
  The first vusim had it reversed, which made every cross product exactly zero.

**Floats are IEEE single in round-toward-zero with denormals flushed**, done exactly
(`f_add` uses TwoSum, `f_div`/`f_sqrt` fall back to `Fraction` at an f32 boundary). The
test holds them to exact rational chopping.

### Where PCSX2 is not the console: `--pcsx2`

The default model is the CONSOLE's: no NaN and no infinity, so an exponent-255 operand is a
huge ordinary number and 0 × it is 0. PCSX2 computes on x86 SSE. In a skinned mesh, a bone
index past the uploaded palette reads integer data (a NaN to SSE) with weight 0, and the
padding vertex past a strip's end is garbage too. Matching PCSX2 there needs five things,
all in `MODE = 'pcsx2'` only: SSE NaN propagation, microVU's per-op operand clamps (NaN to
+FLT_MAX, transcribed from `microVU_Upper.inl`), the in-flight EFU result saved as P at
program end, DPPS adding y² before x², and **`ERLENG` of a zero vector giving +inf**:
`SSE_DIVSS` clamps only its operands, so 1/0 stays infinite on the host, where the console
saturates. It shows in every `VIRFan` drawn with no point light (all 598 at the EULA), in P
and nowhere else. Every remaining PCSX2 difference was one of these, and none touches a
finite vertex.

### Making a capture

```
pcsx2-eqoa-re/run.sh -nocheats -dns sandstorm -symbols claude-eqoa-iso/decrypted_modules \
    -savestate savestates/inworld-sandstorm.ps2savestate -elf eqoa-boot/out/EQOASAND.ELF <iso> &
# within a second of the socket answering:  vu1dump memory-dumps/vu1/inworld.bin 3000
```

The resumed session renders for about a second before it wedges (see `savestates/README.md`),
which is plenty: 3,000 launches in 100 MB. **`memory-dumps/vu1/inworld.bin` (2026-10-17)** is
the one the numbers above come from.

**A finished `vu1dump` does not re-arm.** Arming again reports `VU1 dump armed`, but the count
stays at 0 until you send `vu1dumpstop` first.

**The second set (2026-10-17, evening)** came from a cold boot of Sandstorm's own image
(`-display ~/.cache/ss_prod.iso`, no `-elf`). `EQOASAND.ELF` now goes black after login
against the live S2 (`savestates/README.md`). The set was taken live, in this order:

| File | Scene | Launches | What it adds |
|---|---|---|---|
| `eula.bin` | EULA | 600 | `VIRFan` (2D, no point light) |
| `title.bin`, `titlemenu.bin` | credits, title menu | 2,000 + 1,500 | `VIRFan`, layer types 1/3 on fans |
| `charsel.bin` | character select | 2,500 | `VIRMaterial`, the clipper, billboards |
| `dusk.bin` | in world, the lantern lit | 6,000 | `VIRStrip2`'s lit path (699), `VIRStrip5` |
| `rain.bin` | rain, forced | 4,000 | `VIRStrip6`, billboards |
| `wire.bin` | `VIRaster::SetWireframe(1)` | 1,500 | wire layers, strip and fan |
| `solid.bin` | `VIRaster::SetSolidFill(1)` | 1,500 | solid layers |

The last three are forced with the tracer's `call`. The scene is at `0x01FB6CC0`, its raster at
`*(scene+0x4BA0)`, and its atmosphere at `scene+0x640`. Rain is
`call 0x00417F78 <atmos> 1 1` (`VIAtmosphere::SetWeather(WXRAIN, true)`), and `<atmos> 0 1`
clears it. The raster overrides are `call 0x00404058 <raster> 1|0` (`SetWireframe`) and
`call 0x00404078 <raster> 1|0` (`SetSolidFill`). Weather particles are what the billboard
program draws (`VIAtmosphere::RenderWeather`/`RenderSplashes` → `BeginBillboards`).

**Coverage** (`vucov.py` over all nine captures): every pair of `VIRInit` … `VIRStrip6`, the
layer emitter, all four layer routines and the block at 1085 runs. The exceptions are four
pairs of `VIRFan`'s clip tail (183–186), four at 1346/1351–1353 and two of the billboard's
(203, 204, after its `[e]`). **1968–2028 can never run.** No branch or entry point reaches it, and it
follows a `jr vi14`. It is a register dump to VU1 100+ ending in `[e]`, followed by a row of
`lq vf30, N(vi01)` whose offsets are the data map's boundaries: 21, 314, 435, 612, 932 and 964.

## Labels

The `L####` names `vudis.py extract` generated are replaced by names for what the code does
(2026-10-17). Where code is entered without a branch, a label was added: the layer jump table
(`layer_jumptable`, 1075), the clipper's axis routines that `jr vi01` reaches (`clip_axis_x/y/z`,
1761, 1784, 1807) and the unreachable tail (`dead_regdump` 1968, `dead_map_offsets` 2023).
Labels are free in `check`, so this changed no byte. Each strip program has the same six:
`<prog>_loop`, `_exit` (pop and emit layers), `_end` (`[e]`), `_reject`, `_backface` and `_clip`.

| raster | pair | | raster | pair | | raster | pair |
|---|---|---|---|---|---|---|---|
| `fan_*` (`VIRFan`) | 101.. | | `layer_emitter` | 1037 | | `clip_polygon` | 1266 |
| `clip_fan` / `_return` | 187 / 281 | | `layer_next` / `_dispatch` / `_done` | 1043 / 1066 / 1083 | | `clip_plane_zneg` … `yneg` | 1282–1330 |
| `strip_*` (`VIRStrip`) | 343.. | | `layer_wire_tag` | 1058 | | `clip_to_screen_call` / `clip_polygon_return` | 1342 / 1347 |
| `strip2_*` (lit) | 458.. | | `layer_tex_wrap` (+`_loop`) | 1451 / 1478 | | `clip_swap` | 1684 |
| `strip2_unlit`, `strip2u_*` | 1354, 1376.. | | `layer_tex_clamp` (+`_loop`) | 1494 / 1508 | | `clip_plane_pass`, `clip_edge_*`, `clip_pass_return` | 1691–1749 |
| `strip3_*` | 577.. | | `layer_solid` (+`_loop`) | 1523 / 1525 | | `clip_intersect`, `clip_axis_x/y/z` | 1754, 1761/1784/1807 |
| `skin_loop` (`VIRStrip4`) | 733 | | `layer_wire`, `wire_strip_*` | 1545, 1573–1612 | | `clip_?_from_cur`, `clip_lerp_from_prev/cur`, `clip_emit_made` | 1777–1854 |
| `strip5_*` | 820.. | | `layer_wire_fan`, `wire_fan_*` | 1616, 1639–1680 | | `clip_to_screen`, `clip_first_out`, `clip_vertex_*` | 1876–1961 |
| `strip6_*` | 954.. | | `clip_strip` (+`_reversed`/`_run`/`_return`) | 1085 / 1172 / 1222 / 1232 | | `dead_regdump`, `dead_map_offsets` | 1968, 2023 |

Billboard: `bb_batch` 21, `bb_packet` 36 (a packet's first billboard), `bb0_size_kept` 73,
`bb_loop` 98, `bb_size_kept` 105, `bb_flush_one` / `bb_flush_two` 159 / 134 (the corners still
owed at a flush; `_size_kept` 164 / 141 skip the clip zeroing), `bb_kick` 179,
`bb_kick_and_flip` 186, `bb_last_kick` 196, `dead_restart` 203.

## What is read so far

**`VIRStrip` is read and verified.** `spec_strip.py` is the loop written as a per-vertex
function, with every float op in the microcode's order, and **it reproduces all 4,676
vertices of the 688 captured runs bit for bit, plus the path each vertex took** (2,090
drawn, 1,994 backface, 592 to the clip path). Its negative controls (a flipped sun term,
flipped winding parity) fail. In short:

- **out, per vertex, at VU1 142+**: XYZF2 (ftoi4 of x/w, y/w, z/w, fog; bit 15 = ADC),
  (s, t, q/w, fog), RGBA floats. **This is an intermediate buffer, not the GS packet**:
  s and t are not divided by w and the colour is float.
- **colour** = min(c + ambient + sun·max(−N·S, 0) + point·(1 − min(|L|/range, 1))·max(L·N/|L|, 0), 1),
  alpha = c.w. The sun is uploaded as (x, −z, y) because the dot product is computed with
  `opmula`/`madda.x`/`opmsub`, a cross-product idiom that only needs the x lane.
- **fog** = clamp(w·scale + offset, near, far), into F of XYZF2.
- **culling, per triangle (n−2, n−1, n)**: all three clip codes share a plane → reject;
  any outside → the clipper (from the third vertex on); screen-space winding against an
  alternating parity → backface. Each sets ADC rather than dropping the vertex.

**`VIRStrip2` and `VIRStrip3` are read and verified the same way** (`spec_strip.py`
variants `strip2`/`strip3`; 1,550 runs and 8,077 vertices, 669 runs and 9,652 vertices,
bit for bit with their paths; `test_vusim.py` carries a negative control for each
difference):

- **Both unpack integer vertices** (`itof0`, then × vf29's four pack scales).
- **`VIRStrip3` positions are relative to a pre-translation**: P = itof(pos)·scale.x +
  preTrans[pos.w], the 32-entry table `UploadPreTrans` puts at VU1 932. Lit like
  `VIRStrip`, point light included, but the sun term is × c.w and alpha is 1.
- **`VIRStrip2` has two paths on vf11.x**, which `DrawPrimBufferPackVUNC` sets to 1 only
  when the point light's sphere reaches the buffer's box. The capture took only the
  unlit one (1354): colour = min(c + ambient + sun·max(−(N·S)·static.w·c.w, 0) +
  static.rgb, 1), alpha 1. **On packed geometry c.w is a per-vertex sun-visibility
  factor, not an alpha**, and vf31 is the static lighting colour.
- **The lit path (435+) is read too** (`strip2lit`: 430 runs and 2,715 vertices in
  `rain.bin`, all four cull paths). It is the unlit colour with the point term added
  **last**, ((c + amb) − sun) + static.rgb + point, and the code multiplies the sun factor by
  c.w *before* static.w, the opposite order to the unlit path. **The captures verify the point
  term, not those two orders.** The static lighting `vf31` is (0, 0, 0, 1.0) in all 1,129 lit
  runs, so adding static.rgb and multiplying by static.w are both exact no-ops there. The
  orders are read off the code (479, 485, 502); the lit path has only been seen in one desert
  zone.

**`VIRFan` is the strip loop with the hub pinned** (`fan`: 4,898 runs and 19,668 vertices
over five captures, all four paths). `vf18`/`vf15`/`vi04`, vertex 0's screen position, clip
position and clip code, are never shifted, so triangle n is (0, n−1, n). The winding parity
`vi09` stays 32 instead of alternating. The prologue emits vertex 0 with no cull test, and the
clip path BALs `clip_fan`, which assembles the fan triangle for the clipper. Lighting, fog and
outputs are `VIRStrip`'s, alpha included. Its layer packets use the fan template (+11).

**`VIRStrip5`** (the tessellator's output) takes float vertices like `VIRStrip` and lights them
like `VIRStrip3` (sun × c.w, point light, alpha 1). It is not software-pipelined, which changes
no result (`strip5`: 106 runs, 400 vertices). **`VIRStrip6`** (PackVU) has two qwords a
vertex, position and (s, t, q), and no normal or colour. Its colour is
min(amb + sun colour·k + point colour·(1 − min(|L|/range, 1))·k, 1) with k = `vf11.y`, and its
alpha is `vf11.w`: qword 14, SPR+0x22C and SPR+0x228, 0.15 and 1.0 in the capture. It uses
`ELENG`, the distance itself (`strip6`: 21 runs, 126 vertices, all four paths).

**The layer emitter, `layer_emitter` (1037), and its routines are read and verified** (`spec_layer.py`:
**3,635 captured packets, 88,004 qwords, bit for bit**; `test_spec_layer.py`). It walks the
list `VIMaterial::ConstructDMAPacket` unpacks to VU1 435: a count, then 17 qwords per layer
(+0 the pass type; +1…9 an A+D packet with `TEST_1`, `ALPHA_1`, `CLAMP_1`, four texture
registers and `ZBUF_1`; +10/11/12 GIF tag templates for strip, fan and line strip; +13 the
layer colour × 128; +14…16 the UV transform, with the scroll offset in row 2). For each
layer it kicks the state packet, builds the layer's packet at 539 out of the 142+ buffer
through the jump table at 1075, and kicks that:

- **type 1, textured, wrap on both axes**: per vertex (u′q, v′q, q), with (u′, v′) the
  layer's transform of (s, t) **minus an integer offset taken from the first vertex**,
  int(u′₀ − 2047/4096). It's invisible under wrapping and keeps a scrolled or tiled
  strip's coordinates near [0, 1). Then ftoi0(colour × layer colour), then XYZF2.
- **type 3, textured, clamp on S or T**: the same with no offset, which a clamp would show.
- **type 0, solid fill**: ftoi0(colour × layer colour) and XYZF2, per vertex (1,186 packets,
  14,028 qwords in `solid.bin`).
- **type 2, wire**: a LINE strip in **white**, with 255 in every RGBAQ lane and the vertex colour
  ignored. It sends v1, then the four points v(n), v(n−2), v(n−1), v(n) for each triangle,
  which is NLOOP 4(N−2)+1 on template +12, set up by the special case at 1051. A fan keeps
  v(n−2) at the hub. The first two points have ADC cleared. When v(n) carries ADC, its two
  back-references go out with w = 0x8000 exactly (`ISW` overwrites the whole word), and v(n)
  is carried into later triangles with w = (w − 0x8000) + 16 (459 packets, 11,873 qwords,
  394 strips and 65 fans, 562 culled vertices, in `wire.bin`).

The controls show what the data covers. The row-2 scroll is non-zero only in type-1 layers,
so type 3's scroll add is shown only by the code it shares with type 1.

**`VIRStrip4` is skinning**: four palette matrices (612 + 4·bone) blended per vertex, then
the vertices rewritten as VUNC and `b VIRStrip`.

The header of `src/raster.vsm` has the VU1 data map, the resident registers, the
entry→caller map and the shared-code map. **Both programs are now read**, everything that can
run, each piece checked bit for bit against the simulator, which is checked against PCSX2.
`test_vu_captures.py` holds everything read since the first capture, each item with a negative
control.

**The billboard program is read and verified** (`spec_billboard.py`, `test_spec_billboard.py`:
all 6 captured `VIBbRender` launches, 181 billboards, 14 packets, bit for bit). It draws the
weather particles: rain, snow and splashes from `VIAtmosphere::RenderWeather`/`RenderSplashes`.

- **Input** (`BeginBillboards`): the constant block (model-view, projection→screen, model→clip,
  (offset x, offset y, 128, alpha scale), the GIF templates, 28 corner UVs for seven
  sprite-sheet layouts), then batches of up to 80 48-byte `VIBillboard`s. **`EndBillboards`
  ORs 0x100 into the last one's frame**, and that end mark is the only thing that stops the
  loop. The count is not used.
- **Per billboard**: the colour is ftoi0(rgb × 128, a × the alpha scale). The centre is projected to
  the screen. The half-size is the projection of (width, height, view z) minus the screen centre
  offset, and it is **(0, 0) when any clip bit is set**, so an off-screen particle is sent as a
  zero-sized quad, not skipped. Out go RGBAQ and then (ST, XYZ2) for corners D, A, B, C, with
  **D and C doubled**.
- **One strip per batch**: a PRE packet sets PRIM once (strip, textured, blended, flat, no fog)
  with a dummy vertex. The billboard packets carry no PRIM, and the doubled corners make the
  joins degenerate. The GS state is the current material's layer 0, kicked from 437.
- Packets alternate between 48 and 240 and are kicked at 16 billboards or at the end mark.
- **A bug the game ships: the UV table (32–59) overlaps the packet buffer at 48.** Layouts 4–6
  read the tag and first billboard of the packet being built, or the whole previous packet
  when building at 240. One rain splash (layout 4) in `rain.bin` does this, and the spec
  reproduces it by doing the reads and writes in the microcode's order. The control that reads
  the table as uploaded breaks exactly that launch.
- **Not testable in these captures**: billboards 0 and 1 load uv3 after storing their uv0–2
  (no captured layout reads its own fresh slot), and the 128 alpha scale of an unblended layer
  (every captured batch blends, at 64).

**The clipper is read and verified** (`spec_clip.py`, `test_spec_clip.py`): **3,151 clips over
the nine captures (2,927 strip, 224 fan), with the polygon it builds and the 10,996 vertices it
returns bit for bit**, and 31 clips that leave nothing. A vertex whose triangle straddles a
plane gets ADC in the strip, and the triangle is clipped on its own and drawn as a **fan**
through the layer emitter:

- **`clip_strip`** (strips) and **`clip_fan`** (fans' copy, which jumps back into the loop instead of
  returning) build a CLOSED polygon at 214, five qwords a vertex: a header (clip code, 0 =
  original / 1 = made), the clip-space position, STQ, RGBA and the screen position. The order is
  n−2, n−1, n, n−2, **reversed when the winding parity says so**, so every polygon winds the
  same way. A fan's is hub, n−1, n, hub.
- **`clip_polygon`** is Sutherland–Hodgman against z+, z−, x+, x−, y+, y− (bits 16, 32, 1, 2, 4, 8),
  each only if the running union of codes has its bit. **`clip_plane_pass`** is one plane pass (copy an
  inside vertex, add a crossing edge's intersection, close the result). The buffers ping-pong
  between 214 and 264.
- **The intersection** (1761 x, 1784 y, 1807 z) takes d = c ± w at both ends and **always
  interpolates from the end the sign of (prev.c − cur.c) picks**. That sign comes from the MAC
  flag of a `sub` four cycles back. So an edge clips to the same point whichever way round it is
  walked. t = d / (d − d′) by `DIV`, applied to the clip position, STQ and RGBA. The screen
  qword is left stale, and the new vertex's code joins the union.
- **`clip_to_screen`** writes the result in the 142 layout. A made vertex is projected through qwords
  8–11 (clip→screen, which only the clipper loads), and **its STQ.z becomes 1/w, discarding the
  interpolated q** (`mr32` puts 1.0 in z first). Its fog is the interpolated STQ.w. Each vertex
  from the second on is backface-culled as the fan triangle (first, previous, this). For the
  second vertex, "previous" is a **stale `vf27`** (lanes the intersections left), which no GS
  fan ever draws with.

The controls (`test_vu_captures.py`) break on the interpolation direction, the parity reorder,
the dropped q, the stale `vf27` and the cull. **Two things are read off the code only**: the
plane order, and a made vertex adding a plane to the union. No captured clip makes a vertex
outside a plane its triangle did not already cross. The coverage gaps at 1346, 1351–1353 and
`VIRFan`'s 183–186 are unreachable (after a `b` or `jr`).
