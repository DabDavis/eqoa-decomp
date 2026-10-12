# Decomp phase 34 — the effect volume, the boid, the zone room and the last three value pools

**1550/1707 -> 1624/1786.**  Four groups on the settled toolchain
(`ee-gcc 2.95.3 -O2 -G0 -fno-exceptions`, SN `ps2eeas` 2.95.3-114) with no
re-tuning, **74 of the 79 written byte-exact and 53 of them exact on the FIRST
build**:

| group | symbols | byte-exact |
|---|---|---|
| `vieffectvolume.cpp` — `VIEffectVolume` (24 of its 38) + `VIBoid` (5 of 5) | 29 | **28** |
| `vipool.cpp` — three new VALUE instantiations, plus `Insert` and `MoveToLast` | 38 | 35 |
| `vizoneroom.cpp` — `VIZoneRoom`, the whole class | 11 | 10 |
| `vieffectvolumesprite.cpp` — `Copy`, closed by the layout correction | 1 | **1** |

Everything under these was already settled — `VIRaster`, `VIMaterial`,
`VIMatrix44`, `VIArray<int>`, `VIBBox`, `VIPlane` — so almost every miss was
codegen, and the five that are still open are two named phenomena.

## The lever: a delay slot reads out which statements are inside the `if`

`VIEffectVolume::SetType` branches on `density >= 0.0f` and its **non-annulled**
`bc1f` carries `sw s0,36(s1)` — the store of `m_type` — in its delay slot.  A
non-annulled delay slot executes on BOTH paths, so that store cannot be inside
the `if`:

```c
void VIEffectVolume::SetType(VIEffectVolumeType t, float density)
{
    Free();
    m_type = t;                    // <- above the guard, not in it
    if (density >= 0.0f)
        m_density = density;
    m_dirty = 1;
}
```

Written with both stores inside the block the function is 9 of 24 words wrong,
**and the two parameters swap callee-saved registers**: `t` dies immediately
after the call in the correct spelling, its live range collapses, its
allocation priority (refs / live-length) overtakes `this`, and it takes `s0`
where `this` takes `s1`.  Five spellings of the condition were swept and none
of them moved either symptom; one moved statement fixed both.

The same shape appears twice more in the phase and both were read the same way:
`VIBoid::Free` (`m_sprite = -1` is unconditional, hence the `beql` with the
store in its slot) and `VIEffectVolume::Process` (`m_time = now` sits above the
`if (t > 0.1f) t = 0.1f;` clamp).

**Read the delay slot before deciding where a statement lives.**  It is cheaper
evidence than the branch structure and it is unambiguous.

## Three arguments that were never there

`AllocFlock`'s per-element loop calls `Init__6VIBoid` — and the mangled name has
no argument list at all.  Modelled as `Init(VIEffectVolume &, const VIBBox &)`
on the strength of what a boid plainly needs, `AllocFlock` came out 17 of 43
words wrong, every one of them the `s0`/`s1` swap again, and `FreeFlock` 1
wrong.  Deleting the two arguments made **both byte-exact**, and the 156-instruction
`VIBoid::Init` matched on its first build afterwards.

An extra argument is not a small error: it lengthens a live range across the
call and re-orders the whole callee-saved assignment.  **The mangled name is the
argument list; do not add to it from the semantics.**

## `void` frees `v0`

Phase 32's lever, five more times: `SetType`, `SetDensity`, `OnResize`,
`DetachAllResources`, `FreeParticles`, `FreeFlock` and `VIBoid`'s four are all
`void`, and the tell is the absence of `move v0,zero` before the epilogue.
`OnResize` was 13 of 20 words wrong purely from returning `int` — the extra
pseudo pushed every other register up by one.

## The jump table decides the case labels

Phase 30 read `VIPixelStore` OFF a jump table after fourteen phases of having
it wrong.  Here the table is used forward.  `VIEffectVolume::AttachSurface`
switches on `m_type` with a five-entry table at `0x004EABF0`:

```
case 0 -> 0x0042a144      SetLayerBlendMode(0, 0)
case 1 -> 0x0042a15c      SetLayerBlendMode(0, 3)
case 2 -> 0x0042a174      SetLayerBlendMode(0, 1)
case 3 -> 0x0042a15c      (the same block as 1)
case 4 -> 0x0042a15c
```

Three distinct bodies written as an `if`/`else` chain (or as a `switch` with a
`default`) emits comparisons and no table at all — 21 of 96 words wrong.
Written with **`case 1: case 3: case 4:` sharing one arm** the bound comes out
`sltiu 5`, the table is emitted, and the function is byte-exact.  `Process` and
`AllocParticles` needed the same treatment, including an EMPTY `case 0: break;`
in `Process` so the table starts at zero instead of gcc subtracting one.

The tables are read out of the relocated dump: the un-relocated image holds only
the addend, so `lui 0x0` / `addiu 0x1370` is `0x004EABF0` once the dump supplies
the `lui`.

## The sentinel spelled out, not `End()`

`VIPool<T>::Insert(const T &v, int i)` compares `i` against `lui 0x7fff` /
`ori 0xffff` **with no call**, where every other use of `End()` in `vipool.cpp`
is a `jal` — and `Prev`, four functions further down the same instantiation,
still calls it.  `End()` is not inline (it has its own out-of-line symbol in all
nineteen instantiations), so this is a source fact: `Insert` writes the constant.
One line, 12 wrong words to 0.

## The layout correction that closed a function in another TU

`VIEffectVolume::Density()` is `lw v0,44(a0)`, so `m_density` is **+0x2C**;
`claude-notes/effect-volume-system.md` puts it at +0x28 and every field after it
one slot early.  `vieffectvolumesprite.cpp` had been written against that table
since phase 32, and `VIEffectVolumeSprite::Copy` had been open ever since.
Renaming the four fields it copies to the corrected offsets took `Copy` from
open to **byte-exact** with no other change — the miss was never codegen.

## Still open: which way the clamp if-converts

`VIEffectVolume::AllocParticles` is 5 of 103 words wrong, in two places, and
both are the same two instructions:

```
target     slt  v0,zero,v1 ; movn a1,v1,v0      a1 preset to 1
ours       slti v0,v1,1    ; movz a1,v1,v0      a1 preset to 1
```

Semantically identical; gcc reverses the comparison to reach `movz`.  Twelve
spellings were swept — the ternary in both argument orders, the `if` form, a
separate boolean, `VIMAX` both ways, the expression written out inside the
ternary (phase 23's lever), the default as a named local — and the closest is
`n = 1; c = (int)(...); if (c > 0) n = c;`, which fixes the register assignment
(`a1` holds the 1, `v1` the conversion) and leaves only the direction.  Phase
27's rule ("a ternary VALUE gives `movn` where every `if` gives `movz`") is not
enough here: the ternary gives `movn` only when gcc cannot coalesce the result
with the value, and here it always can.

## Layouts and semantics shipped

**`VIEffectVolume` is 112 bytes**, and the constructor reads out its own
statement order through phase 16's hoist rule (last store of each distinct
non-zero constant to the front, in source order):

```
  +0x00 long           m_time          `sd zero`, eight bytes
  +0x08 VIBBox         m_bbox          members zero themselves TWICE
  +0x20 int            m_reseed        1
  +0x24 VIEffectVolumeType m_type      0 none, 1 leaves, 2 shafts, 3 fog,
  +0x28 float                          4 dust storm, 5 flock
  +0x2C float          m_density       1.0f
  +0x30 float                          20.0f
  +0x3C unsigned       m_flags
  +0x40 int            m_surface       -1
  +0x44 int            m_sprite        -1
  +0x48 int            m_dirty         1
  +0x4C int            m_material      -1
  +0x50 int            m_numParticles
  +0x54 void          *m_particles
  +0x60 VIVect3        m_wind
```

`mScene` is a file-scope `static VIScene *`, and everything the class needs off
the scene is two words: **`VIScene+0x4B9C` is the `VIDictionary *`** (new — it
sits immediately in front of the raster pointer phase 31 already had at
+0x4BA0), reached only by `AttachSurface`/`DetachSurface` handing it to
`VIMaterial::SetLayerTexture` and `VIRaster::ReleaseMaterial`.

**`VIBoid` is 176 bytes** — a sprite index, an HPR at +0x04, a direction at
+0x14, a position at +0x20 and a `VIMatrix44` at +0x60 — and every one of its
seeded fields comes from **two `rand()` draws differenced and scaled by
2^-31**: `((float)rand() - (float)rand()) * 4.65661287e-10f`, a signed unit
random costing two calls a component.  `Init` spends **sixteen** `rand()` calls
and ends with `m_xform.Identity()`.

Shipped oddities:

* **`VIBoid::SeekGoal(float, VIVect3 &)` is EMPTY** — `jr ra; nop`.  The flock
  has a goal-seeking entry point and it does nothing.
* **`VIBoid::Apply(float dt)` never reads `dt`**, and neither does
  **`VIEffectVolume::Process(float dt)`**: Process takes the time from
  `VITimer::GetTime()`, skips entirely below 1/60 s and clamps at 0.1 s, so the
  effect ticks on its own clock whatever the caller passes.
* **`AllocParticles` calls `FreeParticles()` and then calls it again** in the
  `case 0` arm.
* **`AttachSprite(-1)` returns -1 without detaching**, so the only way to clear
  the sprite is `DetachSprite`.
* `AllocFlock` sizes the flock as `(int)(m_density * 8.0f) + 1` and does **not**
  clamp it.

**`VIZoneRoom` is 120 bytes** -- exactly the size phase 24 measured off the
`VIArray<VIZoneRoom>` node stride and left in `viarray.cpp` as a `char[120]`
stub, so writing the real class out is an independent confirmation of that
measurement and the stub is gone.  It is `VICollBuffer`'s shape a third time:
one `new char[]` holding a 32-byte portal record per portal followed by the
portals' vertices, with the two section pointers held as **byte OFFSETS** until
`Init` rebases them onto the block (`VISurface`'s trick, phase 16).  A portal
record opens with a `VIPlane`, then `m_firstVert`, `m_numVerts`, `m_destRoom`.
`Begin`/`Vertex`/`End` are a write cursor: `Begin` stamps the header, `Vertex`
appends a vertex and bumps the portal's count, `End` computes the plane and
advances.

`CalcPortalPlane` is **Newell's method with the centroid accumulated in the same
loop** — `n.x += (y2-y1)*(z2+z1)` and its two rotations, `sum += v`, then
`d = -(n . sum/count)`.  A degenerate ring (`Normalize` returning zero) gets
`d = 0` and **keeps whatever normal `Normalize` left in the vector**.  Its
`Normalize` result is compared against `0.0f`, not `0`: the object code converts
the `int` with `cvt.s.w` before a `c.eq.s`.

**The three new VALUE pools** were modelled from their own two loops the way
phase 24 modelled `VIArray`'s elements — node stride for the size, the copy for
the alignment:

* `VIEEBucket` 16 bytes, `ldl`/`ldr` copy, so 4-byte aligned and POD;
* `VIRasterMaterialLRU` 16 bytes, likewise;
* `VILoaderTrans` **48 bytes with alignment 8** — its node copy is seven aligned
  `ld`/`sd` pairs, which only happens if the element carries a 64-bit field.

None of the three takes the phase-25 `VIPOOL_VALUE_COMMON` macro: each asks for
a different subset, and between them `Erase` and `Init(const VIPool &)` appear
in only one.  Two template members are new: **`Insert(const T &, int)`** splices
in front of `i` (and its `i == End()` case is the whole of `Add`), and
**`MoveToLast(int)`** is the LRU touch — unlink, append at the tail — which is
the only reason `VIPool<VIRasterMaterialLRU>` exists.  `MoveToLast` matched
byte-exact on its first build at 55 instructions.

The three `Allocate`s are the phase-25 open: two induction variables emitted in
the opposite order, 4 words each (9 for `VILoaderTrans`, whose element is
bigger).

## Harness

Four new `SPAN_CAP` entries, all the same cause — a template's unnamed twins
sitting in the gap after the last named symbol of a class:

```
Clear__t6VIPool1Z10VIEEBucket        104   (472 derived, 3+ functions)
Init__t6VIPool1Z13VILoaderTransi      80   (264, 2+)
Erase__t6VIPool1Z13VILoaderTransi    220   (240, 1 static)
Allocate__10VIZoneRoom                72   (368, 3+)
```

Without them `Clear__t6VIPool1Z10VIEEBucket` reads as 117 instructions instead of
26 and scores 91 wrong.  `test_span_caps_only_ever_shrink` passes on all four.

## Open

Five: `AllocParticles` 5/103 (the clamp direction above),
`CalcPortalPlane` 37/145, and the three pool `Allocate`s.  `CalcPortalPlane`'s loop
body is byte-exact once the normal terms are written BEFORE the centroid
accumulation (91 wrong -> 37); everything left is `this` and `i` in each
other's callee-saved registers and the schedule that follows from it — the
standing phenomenon, and four source shapes were swept without moving it.
