# Phase 15 — `VIMaterial`, and the layer pointer that is not a subscript

**2026-08-23. 496/565 → 514/588.** `VIMaterial` is 18 of 31 on the settled
toolchain with **no re-tuning**, six of them on the first build. It is the
second property bag in the corpus after `VIParticleAttributes`, and like that
one its whole layout reads off its own accessors.

## The layout, read off the accessors

Every layer setter multiplies the index by **144** and adds it to `this`;
almost every one closes with `m_dirty = 1` at +0x20. `SetNumLayers` refuses an
argument that is not `< 4`, and the constructor builds **three** layers.

```
VIMaterial : public VIBuffer            VIMaterialLayer            (144 bytes)
+0x00  int          m_id       (base)   +0x00  int          m_texture   -1 = none
+0x04  int          m_userData (base)   +0x04  unsigned int m_flags
+0x08  int          m_numLayers         +0x08  VIColor32    m_color
+0x0C  unsigned int m_flags             +0x0C  int          m_blendMode
+0x10  VIColor32    m_emissive          +0x10  VIMatrix33   m_uvTransform
+0x18  double       m_time              +0x34  float        m_lodBias
+0x20  int          m_dirty             +0x38  VIVect2      m_uvRate
+0x24  int          m_handle   = -1     +0x40  VIVect2      m_uvOffset
+0x30  VIMaterialLayer m_layers[3]      +0x48  int          m_pass      = -1
```

Material flags: 1 ZTest, 2 ZWrite, 4 UV animated, 0x10 dither, 0x20 tessellate;
the constructor and all three `Init` forms set **19** = ZTest|ZWrite|dither.
Layer flags: 1 modulate, 6 wrap mode, 8 point sampling, 0x10 UV set, 0x20 solid
fill.

**It derives from `VIBuffer`.** The constructor's first two stores are +0x04
then +0x00 — the base's chained `m_id = m_userData = 0`, the same signature
`VICollBuffer`, `VIPrimBuffer` and `VIColorBuffer` carry.

## The lever: `this + i * 144`, NOT `&m_layers[i]`

Every layer accessor in the target computes `this + i * 144` into one register
and reads its field at a displacement of **48, 52, 56, 60, 64, 100 or 104** —
the array base is folded into the DISPLACEMENT, not into the pointer:

```
li   v1,144
mult v0,a1,v1
addu v1,v0,a3        <- this + i*144
lw   v0,52(v1)       <- m_layers[i].m_flags
```

A subscript cannot produce that. gcc folds the array's +48 and the field's +4
together onto `this` and leaves the access at `0(reg)`:

```
addiu a0,a3,52       <- this + 52
mult  v1,v0,v1
addu  a0,a0,v1
lw    v0,0(a0)
```

Four spellings were swept in the real function — compound `|=`, explicit
`x = x | 1`, a pointer local before the `if`, a pointer local inside each arm —
and a standalone toy repeated all four. Every one folds the constant onto
`this`. It is not the read-modify-write either: single-read accessors like
`GetLayerTexture` match a subscript exactly, because with one use the constant
lands in the MEM displacement whichever way it was associated. Only when the
address is used twice does CSE bind it, and then WHERE the constant sits
becomes visible.

`Init` says the same thing from the other side: it walks ONE register forward by
144 per layer and uses displacements 48..59 off it.

So the source forms a pointer whose value is `this` advanced by whole layers,
with the fields living 48 bytes into it. The decomp models that with a second
type, `VIMaterialLayerView` — the same 144 bytes with the material's header in
front — and a file-scope inline `Layer(this, i)`. The real member array stays
declared as `VIMaterialLayer m_layers[3]`, because the constructor constructs
three of them: that is where the `VIMatrix33` constructor call in its loop comes
from. **Twelve accessors went from wrong to byte-exact in one edit.**

## Two more type facts, both measured

**The flag words are `unsigned`.** `GetLayerWrapMode` bounds its switch with
`sltiu`, not `slti`, and every clear-bit mask is materialised as `lui 0xffff` +
`ori` rather than `li -N` — which is what gcc does for a 32-bit unsigned
constant it will not sign-extend. One `unsigned int` on each flag member fixed
`SetZWrite`, `SetZTest`, `SetDither`, `SetTessellate` and `GetLayerWrapMode` at
once.

**`VIColor32` has a default constructor, and it is opaque black.** The material
constructor zeroes `m_emissive` TWICE — once with `a = 255` and once with all
four zero. The first pass is the member's own constructor, the second the body's
assignment. Same evidence shape as `VIBBox`'s zeroing constructor in Phase 13.

**`VIColor32::Set` is an inline member, not four field assignments.** The
constructor and all three `Init` forms materialise `&m_emissive` into a register
and fold its +0 back into the outer base — Phase 9's pointer lever, reached
through an inlined `this`.

## A shipped bug: `SetLayerUVSet` ORs the complement

```
lw   v0,52(a1)
lui  v1,0xffff ; ori v1,v1,0xffef      v1 = ~0x10
and  v0,v0,v1
beqz a2,skip
sw   v0,52(a1)
or   v0,v0,v1                          <- ORs ~0x10, not 0x10
sw   v0,52(a1)
```

Clearing the bit uses the complement, and so does SETTING it: asking for UV set
1 turns on every flag except the one meant — modulate, both wrap bits, point
sampling and solid fill all come on together. Written `|= 0x10` the function is
two instructions different, so this is matched as written. It joins
`VIMatrix33::Translate` (Phase 4) and the `Scale`/`Store` pair (Phase 9) as a
bug that shipped and that the decomp has to reproduce.

## A harness fact: `long long` is not 8 bytes here

The constructor zeroes +0x18 with a single `sd`. Modelling that member as
`long long` moved every field after it by 16 bytes — `m_dirty` landed at +0x30
instead of +0x20 and thirteen already-matching accessors each went one
instruction wrong, all of them the closing `m_dirty` store. `double` gives the
right offsets. Whatever ee-gcc does with `long long` on this ABI, it is not an
8-byte 8-aligned member; do not use it to model a 64-bit slot.

## Open: five, and none of them structural

| function | wrong | what is left |
|---|---|---|
| `Init` | 2/60 | one delay-slot fill |
| `InitWireframe` | 4/42 | statement interleave around `m_dirty` |
| `InitSolidFill` | 4/41 | the same |
| `SetLayerUVRate` | 5/26 | the target materialises `&m_uvRate` where ours folds +104 into the two displacements; three pointer and reference spellings all fold |
| `VIMaterial()` | 6/60 | `m_time = 0.0` costs a `move v1,zero` the target does not have, plus ordering |

Not started: `SetNumLayers`, `SetLayerTexture`, `CopyLayer`, `Process`, and the
four `Construct*Pass` DMA builders (268, 176, 94 and 92 instructions), which
need `VIRaster` and `VIQWord` and are a phase of their own.

---

**Corrected by phase 40 (2026-08-27).** The layer's real name is
**`VIMaterialPass`** — `VIEESurfCache::Set`'s third parameter mangles
`R14VIMaterialPass` and its stores land on exactly this class — and its
`+0x48`, filed here as `m_pass`, is the **CACHE STAMP**: `VIEESurfCache::Cache`
compares it against the resident `VIEESurfCacheInfo`'s own `+0x50` and skips
the whole texture upload when they agree.  The 68-byte `m_packet` at `+0x4C`
is four bytes of alignment plus **four GS A+D pairs** at `+0x50..+0x8F` —
TEX0_1 (reg 6), TEX1_1 (0x14), MIPTBP1_1 (0x34) and MIPTBP2_1 (0x36) — which
`Set` writes.  `VIMaterialLayerView` keeps its name; it is the decomp's own
device, not the original's.
