# Phase 56: the raster

`decomp/src/viraster.cpp` — **104 of `VIRaster`'s 167 functions written and 98
byte-exact.**  The tree moved **2,679/3,049 → 2,777/3,153**.

`VIRaster` is SUPPORT `0x003FF2E8 .. 0x0040C758`, its own translation unit, and
it was the largest class in the corpus that had never been touched: 167
functions and 12,384 instructions against `VIScene`'s 140 and 19,372 bytes.  It
is the renderer — five resource pools (prim buffers, colour buffers, surfaces,
material palettes, fonts), the material list with an LRU over the resident ones,
the DMA double buffer every draw call appends to, and the immediate-mode vertex
path.  `VIScene` holds one at `+0x4BA0`.

## The object, and where its state actually lives

The constructor is the phase-53 member roll-call again — a `VIVect2`, a
`VIVect3` and a `VIColor32F` inlined, one `jal` for the tessellator at `+0x40`,
nine more for the pools and lists at `+0x4B78 .. +0x4C34`, then a run of plain
stores — and the destructor is that roll-call in reverse behind a `Close()`.
The class is **0x4C54 bytes**, and its three biggest pieces are:

* **`VIRasterTess` at `+0x40`, 17,480 bytes** — the tessellator, by far the
  largest member.  Its seven functions are NOT written; the body is a block so
  the enclosing layout is right and the constructor call survives.
* **`int m_glyphs[256]` at `+0x45EC`** — the per-string glyph cache
  `DrawString` fills before it walks the run.
* **`VIVect3 m_preTrans[32]` at `+0x49F8`**, with the point they are relative to
  at `+0x49EC`.  `UploadPreTrans` ships exactly 32 of them.

**The per-frame uniform state is NOT in the object.**  `GetModelView` is two
instructions — `jr ra` and `lui v0,0x7000` — and every light, fog and matrix
setter writes an absolute scratchpad address.  The whole block lives in SPR at
`0x70000000`, which is what VU1's unpack reads:

| Offset | What |
|--------|------|
| `+0x000` | model-view `VIMatrix44` |
| `+0x040` | projection `VIMatrix44` |
| `+0x080`, `+0x0C0`, `+0x100`, `+0x140`, `+0x180`, `+0x190` | the derived matrices `CalcMatrices` builds |
| `+0x1A0` | sun direction, `+0x1AC` sun colour |
| `+0x1BC` | directional lights, stride 28 — and `MaxDirLights()` returns **0** |
| `+0x1C0` | point lights, stride 32 — `MaxPointLights()` returns **1** |
| `+0x1E0` | ambient `VIColor32F` |
| `+0x1F0` | fog scale, `+0x1F4` fog offset, `+0x1F8`/`+0x1FC` the fog Z range (0 and 255) |
| `+0x200` | static lighting `VIColor32F` |
| `+0x220` | ambient enable (a byte), `+0x221` sun enable, `+0x222` per-light enables |

The engine therefore ships with the sun, the ambient term and **one** point
light; `EnableAllPointLights` is a loop that runs exactly once.

## What the old Ghidra note got wrong

`claude-notes/raster-system.md` is a good map of the frame lifecycle and most of
its offsets hold, but three entries are corrected here:

* **`+0x3298` is not a `VIEESurfCache`.**  The surface cache is a FILE-SCOPE
  object: `Init` hands `VIEESurfCache::Init` a `lui`/`addiu` pair, not
  `this + N`, and `NumTextureSwaps` reads `+0x1C` off the same global.  What
  actually occupies `+0x40 .. +0x4488` is the tessellator.
* **`+0x4488` is a FLOAT, not a flags word** — it is the Z bias, written by
  `SetZBias` as `z * 166.0f` and read by `CalcClipToScreen` with `lwc1`.
* **`+0x0C .. +0x2F` are the immediate-mode vertex attributes**, not header
  fields: `UV()` writes the `VIVect2` at `+0x0C`, `Normal()` the `VIVect3` at
  `+0x14` and `Color()` the `VIColor32F` at `+0x20`, and `Vertex()` flushes all
  four rows into the DMA chain.

## The immediate-mode vertex

`Vertex()` is FOUR quadwords per call, written through `m_vtx` (a `VIVect4 *`
at `+0x34`) which is re-read from the member at every statement:

    position (x, y, z, 1.0)   uv (u, v, 1.0, 0.0)
    normal   (x, y, z, 0.0)   colour (r, g, b, a)

and `m_numVerts` at `+0x30` counts them.  The `VIVect2I` and `VIVect2` overloads
are the same body with the position mapped through the viewport:
`(v - half) + 0.5` scaled by the reciprocal, with the Y reciprocal negated.

## Levers

**An SPR array subscript keeps the member offset in the DISPLACEMENT.**
`EnableDirLight` and `EnablePointLight` are `lui v0,0x7000` + `addu a1,a1,v0` +
`sb a2,546(a1)`: the base is the block, the index is added to it and the
member's own offset stays a displacement.  A struct pointer folds `0x70000000 +
546` into one constant and adds the index to THAT, which is two instructions
different.  `EnableAllPointLights` is the same fact one level up -- the target
strength-reduces the subscript into a DECREMENTING pointer, so the source walks
`*p-- = on` rather than `m_lightOn[i] = on`.

**A field's absolute address is its own constant, not a struct base.**  The SPR
block is addressed per FIELD in the source, not through one struct pointer:
`SetFog` reads `0x700001F8` and `0x700001FC` twice each and gets a `lui`+`ori`
POINTER for each, where `SPR->m_fogNear` off a common base gives one `lui` and
two displacements.  `Store2D` is the same shape on the two enable bytes.  Where
a field is touched ONCE the two spellings are identical, which is why the
struct form matched for eleven functions before `SetFog` exposed it.

**A mem-initialiser list runs in DECLARATION order, interleaved with the member
constructors; a constructor BODY runs after all of them.**  `VIMaterialPal`'s
two zero stores land BEFORE its `VIArray<int>` member's constructor call in both
`CreateMaterialPal` and `CopyMaterialPal`, and only `VIMaterialPal() :
m_p00(0), m_refCount(0) {}` produces that — written as a body the member ctor
goes first, which is 44 wrong words.

**Five of these functions return VOID** among a class of `int`s: `Frustum`,
`Orthographic`, `EnableTessellation`, `SetZBias` and `SetClipAndLight` have no
`move v0,zero` at all.  Every call site discards the result, so the declaration
is free to change — but a `return 0` is one whole instruction, and on
`EnableTessellation` (2 instructions) it is two thirds of the function.

**A store pair comes out in reverse source order** — phase 54's lever, four more
times in one phase: `BeginTriStrip`'s `m_vtx`/`m_vtxBase`, `Flush`'s two-arm
buffer swap, `InitSPRDoubleBuffer`'s three stores and `InitDMADoubleBuffer`'s
tail.  Each was one swap from byte-exact.

**Nesting the body decides whether the two `return 0`s cross-jump.**  All four
reference-counted releases (`ReleaseFont`, `ReleaseMaterial`,
`ReleaseMaterialPal`, and `ReleasePrimBuffer`/`ReleaseSurface`) want
`if (--refcount <= 0) { ... }` with ONE `return 0` at the bottom; written flat
as `if (refcount > 0) return 0;` the early return does not fold into the late
one and costs two instructions.

**A named result variable keeps the branch where the bare call if-converts.**
`int r = m_materials.Erase(i); if (r < 0) return -1;` gives `move v1,v0` +
`bltz`; `if (m_materials.Erase(i) < 0)` gives `slti` + `movz` — phase 48's
lever, and the difference only shows when there is no OTHER `return -1` for the
branch to cross-jump with.

**A run of same-constant stores keeps source order, and the odd one out is a
source fact.**  `Frustum` zeroes nine matrix fields; the target's run starts at
`+0x3C` and then ascends from `+0x04`, and writing `m.m[3].w = 0.0f;` FIRST
(with the lone `1.0f` store second) is the whole difference between 10 wrong
words and byte-exact.  `Orthographic`'s run is plainly ascending and needed no
such move.

**The element POINTER, not the subscript, when a call sits between.**
`int *e = &p->m_mats.m_data[i]; *e = CreateMaterial(0);` computes the address
BEFORE the call, where `p->m_mats.m_data[i] = CreateMaterial(0)` computes it
after — 44 wrong words to 7 in `CreateMaterialPal`, 51 to 8 in
`CopyMaterialPal`.

**An inline `operator[]` on a member `VIArray` materialises the array's
address** (phase 51), reconfirmed twice: `p->m_mats[i]` gives
`addiu s3,s1,8` + `lw v0,4(s3)` where `p->m_mats.m_data[i]` folds to
`lw 12(s1)`.  `MaterialPalHasDepthSort` and `ReleaseMaterialPal` both need the
first form and `CreateMaterialPal` the second, in the same TU.

**`VIVect3::operator=`, not three field assignments.**  `Normal()` written out
component by component reads all three off the argument first and stores them
after; the assignment operator interleaves load and store and materialises
`&m_normal` as an address pseudo — 3 wrong words to 0.

**`n * 848` is TWO operations in the source.**  `InitMaterialLRU` allocates
`n * 53 * 16` bytes and the target keeps `li 53` / `mult` / `sll 4`; folded to
`li 848` / `mult` it is two instructions different.  Split across two
statements (`int q = n * 53;` then `q * 16`) it matches.

## Open

* `Viewport` 4/59 — the four viewport integers are stored in the same order but
  interleaved differently with the FP stream.  **A 60-way permutation sweep of
  the eight assignment statements scores 4 on every one of them**, so the lever
  is not statement order; something in how the half-width and the aspect terms
  are spelled decides it.
* `CalcMatrices` 3/43 — the light-enable byte lands in `v1` in the target and
  `v0` here; three spellings tried, all `v0`.
* `GetMaterialPalEntry` 8/15 — instruction for instruction identical and off by
  one register pair, plus the operand order of the final `addu`; four spellings
  of the subscript all score 8.
* `CreateMaterialPal` 7/70 and `CopyMaterialPal` 8/85 — two of the residue are
  the `VIMaterialPal` constructor's store ORDER (the target writes `+4` then
  `+0` where the mem-initialiser list gives `+0` then `+4`), and the rest is
  which register survives the loop's re-read.
* `InitMaterialLRU` 4/54 — the `off = 0` lands in the entry block here and in
  the loop preheader in the target.
* **Not written**: the whole GS-packet half — `Blit` (×2, 453 and 658
  instructions), `StretchBlit` (660), `BeginBillboards` (326), `EndTriStrip` /
  `EndTriFan` / `EndBillboards` / `BlitEnd`, the six `DrawPrimBuffer*VU*`
  builders and `TessTriangle`, `AddMaterial`, the five `Upload*`, the four
  `SetGS*`, `Init` (both forms), `Clear`, `BeginScene`, `Flip`, `SetClearColor`,
  `CreateDefaultResources`, `DebugResources`, `CopyMaterial`, the four
  `DrawString`/`DrawCenteredString` and `CalcCenteredStringScreenRect`, the six
  `PickPrimBuffer*`, `Cull`/`Pick`/`PointInTriangle`/`CalcHitColor`, and
  `VIRasterTess`'s own seven.

## Harness

`VIRaster`'s class body had to MOVE to the bottom of `vi.h`.  It had lived at
line 1218 as a shape assertion — method declarations only, no data — because a
dozen other TUs call its methods; the real class has `VIPool` and `VIList`
members and those templates are declared 1,000 lines below it.  Nothing between
the two points calls a `VIRaster` method from an inline body, so the move is
free, and the forward declaration at line 44 is what the intervening classes
were using all along.
