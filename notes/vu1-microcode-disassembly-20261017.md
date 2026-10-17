# The VU1 microcode, disassembled and matched (2026-10-17)

**All of the game's VU microcode is 2,234 instruction pairs in two SUPPORT blobs,
and both now round-trip byte-exact from `decomp/vu/src/*.vsm`.** Tool:
`decomp/vu/vudis.py` (a disassembler and an assembler in one, each the other's
test); canary `decomp/vu/test_vudis.py`.

## Measured

- `VIDMARasterMicrocode` (`0x004E5880`): DMA RET, QWC 1019, eight `MPG`s at VU
  addresses 0, 256, … 1792 (each preceded by a VIF NOP so its data is 8-byte
  aligned, and two trailing NOPs pad the packet to a quadword),
  **2,029 pairs**, which fills VU1's 2,048-slot micro memory to within 19.
- `VIDMABillboardMicrocode` (`0x004E5200`): QWC 103, one `MPG` of **205** pairs.
- No other `MPG` upload exists, so there is no VU0 micro-mode program.
- The `VIR*`/`VIBb*` symbols are byte offsets into each program's VU code
  section, relocated by the image base in the symbol map: entry = (label −
  first label) / 8. `VIBbInit` is 8 bytes in its section but is VU address 0,
  so do not read the symbol's low bits as an address.

## The disassembler, and what held it honest

- The opcode tables are PCSX2's **interpreter** tables (`VUops.cpp`). Its
  **debugger** (`DebugTools/DisVUmicro.h`) labels `SUBA{x,y,z,w}` as `SUB{bc}`.
  Anyone reading VU code in the PCSX2 debugger sees that bug.
- Canonical-or-raw: text is emitted only when it re-encodes to the same 64
  bits. The first extract had 15 raw pairs, all `eleng`/`erleng`, which carry a
  real `.xyz` dest field. After that fix there were 0.
- A float `loi` immediate near FLT_MAX, rounded to fewer digits, overflows
  `struct.pack('<f')`. The random-word test caught it.

## What the code is (first read)

The VU1 memory map, register conventions and entry→caller map are in the
header of `decomp/vu/src/raster.vsm`. In short:

- **0–20 is the per-frame uniform block** `UploadMatrices` ships:
  model→screen, model→clip, clip→screen (not loaded by `VIRInit`), sun
  direction in model space `(x, −z, y, 0)`, point light with w = 1/range,
  sun/point/ambient colours, pack scales, fog (scale, offset, near, far),
  static lighting. `VIRMaterial` reloads only qword 17 (ambient + sun tint).
- **One strip program per prim-buffer format**, MSCALed from
  `VIPrimBuffer::End()`: VUNC → `VIRStrip`, PackVUNC → `VIRStrip2` (also
  `VIColorBuffer`), PackVUNCG → `VIRStrip3`, PackVUNBW → `VIRStrip4`, tessellated
  PackVUNCG → `VIRStrip5`, PackVU → `VIRStrip6`. `VIRFan` serves immediate mode
  and blits.
- **`VIRStrip4` is skinning, not lighting**: four bone indices per vertex
  index a palette at VU1 612 (stride 4 qwords), the four matrices are blended
  by weight, the normal is renormalised with `rsqrt`, the vertices are
  rewritten in place in VUNC layout, and it ends `b VIRStrip`. This corrects
  `go-client-wasm-feasibility-20261016.md`'s "character lighting is `VIRStrip4`".
- 1037–2028 is shared code reached by `BAL`. `L1037` walks a GIF packet list
  at VU1 435 and `XGKICK`s each packet through a four-way jump table (`jr vi10`
  into 1075). `L1085` spills vf15–vf28 and vi01–vi14 to a stack at vi15
  (growing down from 1024) and rebuilds triangles. It is probably the clipper
  (unconfirmed).

## Second session: a simulator held to PCSX2, and VIRStrip read

- **The tracer already had VU1 capture**, undocumented: `vu1dump <path> <count> [entry]`
  writes the complete VU1 state at every launch. Now in `pcsx2-eqoa-re/README.md`. A
  resumed in-world savestate renders for about a second, which is enough for 3,000 launches
  (`memory-dumps/vu1/inworld.bin`, 100 MB).
- **`decomp/vu/vusim.py` agrees with PCSX2 bit for bit on all 2,999 captured launches**: every
  register, Q, P, ACC and all 611,537 memory words written. Record N+1's registers are
  record N's end state, because VIF never touches them in between.
- What it took, each one measured by a failure: Q/P as two cycle-counted instances (DIV 7,
  ERLENG 24); flags visible 4 cycles after their FMAC; **VI latency 4 after ILW/ILWR**, a
  stall that turns off microVU's branch VI-delay rule at 1051; and **OPMSUB uses OPMULA's
  index pattern** (`opmula acc,a,b; opmsub d,b,a` = a×b).
- **PCSX2 is not the console on NaNs.** A skinned vertex whose bone index is past the
  uploaded palette multiplies integer data (a NaN to SSE) by weight 0. On a PS2 that is 0;
  on PCSX2 it is NaN. The default model is the console's, and `--pcsx2` adds SSE NaN rules,
  microVU's per-op operand clamps, the in-flight EFU result saved as P at program end, and
  DPPS's y²-first sum.
- **`VIRStrip` is read and verified**: `spec_strip.py` (a per-vertex function) reproduces
  4,676 of 4,676 captured vertices, and their paths, with negative controls. Lighting =
  colour + ambient + sun·max(−N·S,0) + point·(1−min(|L|/r,1))·max(L·N/|L|,0), clamped to 1;
  fog clamp(w·scale+offset, near, far); per-triangle reject / clip / backface by ADC.
- **The strips write an intermediate buffer** (VU1 142+: XYZF2, s,t,q/w,fog, float RGBA).
  **`L1037` is the layer emitter**: one GS-state packet and one GIF strip packet per material
  layer, built through the jump table at 1075.

## Third session: VIRStrip2 and VIRStrip3

- **Both read and verified with the same harness**: 8,077 and 9,652 captured vertices, bit
  for bit, with their paths. Each difference from `VIRStrip` has a negative control that fails.
- **`VIRStrip3` (PackVUNCG) positions are relative to a pre-translation**: itof(pos)·scale.x +
  preTrans[pos.w], where the table is the 32 entries `UploadPreTrans` writes at VU1 932.
- **`VIRStrip2` picks its path on vf11.x = `m_p45C8`**, which `DrawPrimBufferPackVUNC` sets
  only when the point light's sphere reaches the buffer's bounding box. Every captured run
  was unlit (1354): `c + amb + sun·max(−(N·S)·static.w·c.w, 0) + static.rgb`, alpha 1.
  **On packed geometry the vertex alpha is a sun-visibility factor**, and the static
  lighting colour (qword 20) is added flat.
- A trap for whoever tests next: a negative control drawn from the first 40 runs passed
  when it should have failed, because static.w is exactly 1.0 in 1,394 of 1,550 runs. A
  control must come from runs where the factor varies.

## Fourth session: the layer routines

- **Read and verified**: `spec_layer.py` reproduces all 3,635 captured layer packets
  (88,004 qwords) at the moment they are XGKICKed. Each output field has a negative
  control that fails.
- **The layer list is `VIMaterial::ConstructDMAPacket`'s**, 17 qwords a layer at VU1 435:
  the pass type; an A+D packet (`TEST_1`, `ALPHA_1` by blend mode, `CLAMP_1` from layer
  flags 0x2/0x4, four texture registers, `ZBUF_1`); GIF tag templates for strip, fan and
  line strip; the layer colour × 128, alpha halved when blending; the UV transform with
  the scroll offset folded into row 2.
- **Pass types**: 0 solid, 1 textured with wrap on both axes, 2 wire, 3 textured with a clamp.
  **Type 1 subtracts an integer UV offset taken from the strip's first vertex**
  (int(u′₀ − 2047/4096)), which is invisible under wrapping; type 3 cannot. Per vertex:
  (u′q, v′q, q), ftoi0(colour × layer colour), XYZF2.
- With the strip specs, **the captured path from VU1 input vertex to the GIF packet the GS
  receives is now readable code**, checked bit for bit.
- Coverage limit: the row-2 scroll term is non-zero only in type-1 layers.

## Fifth session: eight more captures, and every remaining strip program

- **The captures** (`memory-dumps/vu1/`, table and recipe in `decomp/vu/README.md`): EULA,
  credits, title menu, character select, in world at dusk with the lantern lit, forced rain,
  forced wireframe, forced solid fill. That adds **19,592 launches, all matching PCSX2**, with
  every entry point of both programs.
  Rain is `VIAtmosphere::SetWeather(WXRAIN, true)` called through the tracer. Wireframe and
  solid fill are `VIRaster::SetWireframe`/`SetSolidFill`, debug overrides still in the retail
  build. They render on the console as described: the whole scene as white line strips, or
  as flat shaded colour.
- **`EQOASAND.ELF` can no longer reach a world on live Sandstorm**: it goes black at CLIENT's
  load, twice, while Sandstorm's own `ss_prod.iso` goes in. That fits the §5 hazard in
  `sandstorm-s2-live-20261015.md`, but the cause is not traced.
- **A finished `vu1dump` does not re-arm**: it needs `vu1dumpstop` first, and the count
  otherwise stays 0 with no error.
- **PCSX2-only, a fifth item: `ERLENG` of a zero vector is +inf on the host.** `SSE_DIVSS`
  clamps operands only. Every no-point-light `VIRFan` ends with P = +inf, where the console
  saturates to FLT_MAX.
- **Read and verified** (`spec_strip.py`, `spec_layer.py`, `test_vu_captures.py`, each with a
  control that fails):
  - **`VIRFan`** is the strip loop with vertex 0 pinned as the hub: the triangle is
    (0, n−1, n), the winding parity stays 32, vertex 0 gets no cull test, and the clip path
    goes through `L0187`.
  - **`VIRStrip2`'s lit path** adds the point term after the static colour. The code orders
    its sun factor c.w then static.w. **The data checks only the point term**: `vf31` is
    (0, 0, 0, 1.0) in all 1,129 lit runs, so both orders are read off the code alone.
  - **`VIRStrip5`** (the tessellator's output) takes float input and is lit like
    `VIRStrip3`.
  - **`VIRStrip6`** (PackVU) has position and UV only. It is lit by ambient, the sun colour
    × k and the point light's distance falloff × k, with k = qword 14's y and alpha its w.
    It uses `ELENG`, not `ERLENG`.
  - **Solid layer**: colour × layer colour and XYZF2.
  - **Wire layer**: a white line strip, four points a triangle, with a fan's back-reference
    pinned to the hub and ADC stamped as a bare 0x8000.
- **Coverage, all nine captures**: every pair that can run does, except 4 in `VIRFan`'s clip
  tail, 4 near 1346 and 2 at the billboard's end. **1968–2028 is unreachable**: a register dump
  and a row of `lq` whose offsets are the data map's boundaries (21, 314, 435, 612, 932, 964).

## Sixth session: the clipper

- **Read and verified** (`spec_clip.py`): **3,151 clips over nine captures (2,927 strip,
  224 fan); the polygon at 214 and the 10,996 returned vertices are exact.** The controls that
  break are the interpolation direction, the parity reorder, the dropped q, the stale `vf27` and
  the cull.
- The shape: the triangle becomes a closed polygon, its order reversed by the winding parity so
  all polygons wind one way. It is clipped with Sutherland–Hodgman against z+, z−, x+, x−, y+,
  y−, only the planes the running union of codes crosses. The result goes back in the 142
  layout and is drawn as a fan through the layer emitter.
- **The intersection always interpolates from the end picked by the sign of
  (prev.c − cur.c)**, read through a MAC flag four cycles back. So an edge clips the same
  whichever way it is walked, and shared edges stay watertight.
- **A made vertex's STQ.z is 1/w, not the interpolated q**: `mr32` writes 1.0 first. That is
  exact only when the input q is 1. Its fog is the interpolated STQ.w.
- The fan cull of the second output vertex reads a **stale `vf27`**. That is harmless to the
  GS, but it is in the output bits.
- **Read off the code only**: the plane order, and a made vertex growing the plane union. No
  captured clip makes a vertex outside a plane its triangle had not already crossed.
- The remaining coverage gaps are unreachable: 1346 and 1351–1353 sit after a `b` or `jr`, and
  `VIRFan` 183–186 follow `L0187`, which jumps back into the loop instead of returning.

## Seventh session: the billboard program — and with it, all of the microcode

- **Read and verified** (`spec_billboard.py`): all 6 captured `VIBbRender` launches, 181 billboards
  and 14 packets are exact. It draws the weather particles. The inputs are `VIBillboard`s from
  `BeginBillboards`, and **`EndBillboards`' 0x100 in the last frame is the only stop**.
- Per particle: a projected centre and a projected half-size, which is **zero when any clip bit
  is set**. Out go RGBAQ and (ST, XYZ2) for four corners, the first and last doubled. A PRE packet
  sets PRIM once, so **a batch is one degenerate-joined triangle strip**, flat-shaded with the
  material's layer-0 GS state.
- **The game ships an overlap: the corner-UV table (32–59) shares 48–59 with the first packet
  buffer.** Sprite layouts 4–6 read the packet being built, and a captured rain splash does. The
  spec reproduces it by modelling the read/write order.
- Untested here: the first two billboards' uv3 load order, and the unblended alpha scale.

## Next

1. **Port**: everything VU1 runs is now readable code (`spec_strip`, `spec_layer`, `spec_clip`,
   `spec_billboard`), each checked bit for bit. A native or web renderer can follow it directly.
2. Rename the remaining `L####` labels in the `.vsm` files after what they are now known to do.
3. **Hand-scheduled VU code can read a register before an earlier write to it has
   finished**, so a port must model pipeline latency where the code relies on it. Find
   those spots first.
