# Phase 57: the raster's DMA half

`decomp/src/viraster.cpp` — **24 more of `VIRaster`'s 167 functions written and
16 byte-exact**, taking the file from 104 written to **128**.  The tree moved
**2,777/3,153 → 2,793/3,177**.

Phase 56 wrote the object: the pools, the material list, the LRU, the
accessors, the light and fog setters.  What it left was the half that actually
talks to the hardware — every function that appends to the DMA chain, opens or
closes a VIF packet, or ships a GS register.  This phase takes the tractable
part of that: the three microcode uploads, the two per-frame uniform uploads,
the strip and billboard closers, the blit brackets, the GS register packets,
the skinning palette, the deferred free lists and the DMAC handler.

## What the packets are

Every one of these functions has the same shape, and it is worth writing down
once because it is the vocabulary of everything still open:

| Field | Meaning |
|-------|---------|
| `m_dmaTag->m_id = 16` | a DMAtag **CNT** — the data follows the tag |
| `m_dmaTag->m_id = 80` | a DMAtag **REF** — `m_addr` points at the data |
| `m_dmaTag->m_qwc` | quadwords of data, and the cursor advances `qwc*16 + 16` |
| `0x50000002` / `0x50000004` | VIFcode **DIRECT**, 2 or 4 quadwords to the GIF |
| `0x10000000` | VIFcode **FLUSHE** |
| `0x13000000` | VIFcode **MSCNT** |
| `0x14000000 \| RStripOffset` | VIFcode **MSCAL** at a microprogram label |
| `0x15000000 \| ((VIBbRender - VIBbInit) >> 3)` | **MSCALF**, and only the label DIFFERENCE is used |
| `0x6C..` | VIFcode **UNPACK V4-32**; NUM is bits 16-23 |
| `0x1000000000008001` | a GIF tag, NLOOP 1, EOP, NREG 1 |
| `14` | the REGS field: A+D |

So `SetFogRegisters` is a DIRECT of two quadwords carrying one A+D write of
**FOGCOL** (`61`), `SetGSDithering` one of **DTHE** (`69`), and
`InitGSRegisters` a DIRECT of four carrying three — **COLCLAMP** (`70`),
**TEST_1** (`71`, `ZTE`/`ZTST`) and the dither matrix **DIMX** (`68`,
`0x6071243571603524`).

`SetObjectBlends` ships the skinning palette in blocks of **32 matrices**,
which is 128 quadwords — exactly what one UNPACK's NUM field will carry — and
2,048 bytes per `memcpy`.  `UploadPreTrans` ships exactly 32 `VIVect3` with the
w of each left at zero.  `EndTriStrip`, `EndTriFan`, `BlitEnd` and
`EndBillboards` all do the same thing: rewind `m_vtx` to `m_vtxBase` and fill
in the header quadword the vertices were written after, then give the DMA tag
its count.

## Corrections to phase 56

**`VIRasterPointLight` is not four floats.**  It is
`VIVect3 m_position; float m_range; VIColor32F m_color;` — `SetPointLight`
assigns the first three with `VIVect3::operator=` and the fourth on its own,
and hands `VIMatrix44::Mul`, whose signature takes a `VIVect3`, the position.
Phase 56 read the 32-byte stride and a `VIVect4` off the four stores; the call
settles it.

**The dir-light array OVERLAPS the point-light array.**  `SetDirLight` indexes
`0x700001BC` with a stride of 28 and `SetPointLight` `0x700001C0` with a stride
of 32, so a frame uses one kind of local light or the other and never both.
Only the point array can be a member of `VIRasterSPR`; the dir array is
addressed by cast.

**`VISurface` HAS a destructor.**  `ReleaseDeferred`'s `delete s` calls
`_$_9VISurface(s, 3)` behind a null check; without the declaration gcc emits a
bare `jal __builtin_delete` and no check at all.

Five more `VIRaster` statics are named — `ObjectBlendsBuffer`,
`PreTransBuffer`, `PrimBuffer1`, `PrimBuffer2` and `ColorMatrix`, all VU1
addresses — plus **`DBuffer`, the double-buffered GS register block**:
`SetClearColor` patches RGBAQ at `+384` and `+752`, and `GetZBufferReg` is two
instructions that hand back the **ZBUF_1** setting at `+480`.

## The levers

**A `volatile` pointer local materialises a hardware address into a register.**
`*(int *)0x1000E010 = 1;` compiles to the assembler's macro — `lui at,0x1001` /
`sw v0,-8176(at)` — where the target loads the whole constant
(`lui`/`ori 0xe010`) and stores at `0(reg)`.  `volatile int *p = (volatile int
*)0x1000E010; *p = 1;` is the spelling that does that, and it closed
`InitDMACHandler` outright.

**A whole-object assignment written INLINE keeps its own address pseudo.**
`SetDirLight` has TWO `mult i,28` — one feeding the stores, one feeding
`SubMatrixMul` and `Normalize`.  Six spellings through a named pointer or
reference all cse them into one; `((VIRasterDirLight *)0x700001BC)[i] = l;`
followed by the two calls on the same inline subscript keeps both.

**Every SPR field is its OWN absolute constant.**  Written through the
`VIRASTER_SPR->` macro, cse hoists the shared `0x70000000` into a callee-saved
register and all twelve of `SetDefaultLights`' stores go off it.  The target
gives each *three-use* colour a pointer of its own (`lui`/`ori` to
`0x700001E0`, `0x700001AC`) and leaves every single-use field to the
assembler's `lui at` macro — which is what a per-field cast
(`((VIColor32F *)0x700001E0)->r`, `*(float *)0x700001EC`) produces.

**`VIVect3::Set()`, not three field assignments**, is what materialises a
local's address, reconfirming phase 32 — and `SetDefaultLights`' second vector
is **a dead local**: it is filled with `0.577f` and never read.

**A named `end` pointer decides which side of a pointer subtraction carries the
`+1`.**  `int n = (m_vtx + 1) - m_vtxBase;` gives `addiu base,-16` then `subu`;
`VIVect4 *end = m_vtx + 1; int n = end - m_vtxBase;` gives `addiu old,16` then
`subu`, which is the target.

**A named byte count keeps `(n << 4) + 16` together.**  Spelled inside the
pointer add, gcc splits the `+16` off onto the pointer at the far end of the
function; `int bytes = (n << 4) + 16;` closes both strip closers.

**A constant-size `memcpy` is a CALL only when an argument is a `void *`
VARIABLE.**  With `char *`, a typed pointer, or an inline `(void *)` cast, gcc
2.95 expands `memcpy(p, src, 2048)` into a block-move loop — and, when it
cannot prove the alignment, into a *pair* of loops behind a runtime
`andi 7` test.  A `void *` local or parameter is the only form that emits
`li a2,2048` and a `jal memcpy`, which is what `SetObjectBlends` has.  It is
worth 27 instructions and the whole frame.

**`((long *)base)[k]`, not `long *q = (long *)(base + 32)`.**  The `sd` stores
in `SetFogRegisters`, `SetGSDithering` and `InitGSRegisters` carry
displacements 32/40/48/56 off the DMA tag base; a named pointer materialises a
register of its own.  This alone took `SetGSDithering` to byte-exact.

**The `lui`s come out in SOURCE order** (phase 21), and here it settles the
order of two stores that carry no other evidence: `InitGSRegisters` writes the
DIRECT code before the FLUSHE (`p[3] = 0x50000004; p[2] = 0x10000000;`), which
is readable off which of the two constants got the callee-saved register.
Byte-exact on that one edit.

**ONE reused float temp serialises a run of conversions** — four independent
`(int)(x * 255.0f + 0.5f)` statements let gcc hoist all four loads to the top
of the block, where the target converts one component at a time.  It is worth
20 words in `SetFogRegisters` and still does not close it.

## Open

* `ReleaseDeferred` 14/73 — the shape is right and the second loop is exact;
  the first loop has the list pointer and the iterator in the opposite pair of
  callee-saved registers.  Seven loop spellings score the same 14.
* `SetObjectBlends` 60/91 — the same length as the target and the same
  instructions, with `src` and the block counter swapped between `s1` and `s2`.
* `UploadRasterMicro` 41/60 and `UploadBillboardMicro` 30/52 — the target needs
  one more callee-saved register than the candidate and schedules around it; a
  36-way sweep of the two packets' store orders moves it by one word.
* `SetFogRegisters` 32/73 and `UploadPerMaterial` 33/90 — gcc hoists the
  component loads that the target leaves in place.
* `EndBillboards` 31/55 — the multiply result lands in `a0` in the target and
  in `a2` here, which costs the reload of `this` the target has.
* `DrawPrimBuffer(int,int,int,unsigned int)` 11/53 — the dispatch is a
  comparison TREE with a dead `slti v1,4` in an annulled delay slot; every case
  list tried gives an if-chain instead.

**Not written**: `Init` (all three), `Clear`, `SetClearColor`, `BeginScene`,
`Flip`, both `Blit`s, `StretchBlit`, `BeginBillboards`, `AddMaterial`,
`CopyMaterial`, `CreateDefaultResources`, `DebugResources`, the four string
drawers and `CalcCenteredStringScreenRect`, the six `DrawPrimBuffer*VU*`
builders and `TessTriangle`, the six `PickPrimBuffer*`,
`Cull`/`Pick`/`PointInTriangle`/`CalcHitColor`, `SetGSScissoring`,
`SetGSDitherMatrix`, and `VIRasterTess`'s own seven.
