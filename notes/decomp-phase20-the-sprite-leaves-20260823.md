# Phase 20 — the sprite leaves, and a link map for the whole corpus

*2026-08-23*

Continues [decomp-phase19-the-sprite-hierarchy-20260823.md](decomp-phase19-the-sprite-hierarchy-20260823.md).
`VISprite` and `VISimpleSprite` settled the polymorphic base; this phase does the
five leaves that hang off it — `VISkinSprite`, `VIFloraSprite`, `VILODSprite`,
`VIPointSprite`, `VIGroupSprite` — on the same toolchain with no re-tuning.

But the finding that matters most is not a class.

## A resident SUPPORT is the image's own link map

The decomp oracle, `SUPPORT_decompressed_SNR1.bin`, is **un-relocated**, and that
is the whole reason an unlinked `.o` can be compared to it with no normalisation:
every `jal` is `0x0C000000` and every `lui %hi` is zero in *both*. The price has
been paid every phase since the first — the image cannot say which function a
call goes to, so a callee was named from its argument shape and the plausibility
of the name. `VISprite::QueryEmitters`'s `Insert`/`Erase`, `VISurface::Init`'s
`Deallocate`, `VIScene::GetSprite`.

It does not have to be paid at all. **SUPPORT is a resident module loaded at the
same virtual addresses this image is indexed by**, so any EE memory dump taken
in-world holds the relocated copy of the very same build:

```
209,610 of 227,290 words identical (92.22%)
 17,680 differ — and every single one is a relocation site:
   14,488 jal      1,451 lui      1,133 addiu      328 lw      218 sw
      13 sb           12 swc1         8 sh           8 lbu       5 lhu
       4 lq            4 lwc1         3 lb           3 sq        2 j
    0 differing words are NOT a relocation shape
```

Nothing else moved. `decomp/eqcalls.py` reads a call's target out of the dump and
looks it up in the same symbol map:

```
$ ./eqcalls.py calls Raster__11VILODSpriteR8VICameraR7VISceneR8VIRasterUi
  0x00452a94  jal   GetModelView__C8VIRaster
  0x00452b08  jal   GetSpriteType__7VIScenei
  0x00452b18  jal   GetNextColorBuffer__7VIScene
  0x00452b70  jal   RasterSprite__7VISceneR8VICameraiUi
```

`verify` re-proves the pairing on demand; `calls` refuses to run on a function
whose non-relocation words disagree, so a dump from another build is reported
rather than silently resolving to plausible garbage. Two canary tests lock both
halves, with the corrupted-word negative control.

**It corrected two names that had stood for four phases**: `VIScene::GetSprite`
is `Sprite`, and `VIDictionary::Release` is `Remove`. Neither changes a byte —
`jal` targets are masked — but every `Release` in the corpus said the wrong
thing, and the second call in `VILODSprite::Raster` could not have been guessed
at all: it takes nothing but `this`.

The narrow relocation set (jal/lui/addiu/lw/sw) leaves 50 stray words, and all 50
are relocations too — `sb`/`sh`/`lbu`/`lhu`/`lq`/`sq` `%lo` forms in
`VIPrimBuffer`'s packed-vertex writers and `VIRaster`'s GS register loads, plus
six pointers in the module's own header. With them included the code range is
100.000% accounted for, which is what makes "same build" a measurement.

## The sprite vtable is 96 bytes, all the way down

The symbol map gives the leaves' `_vt$` symbols sizes of 128, 136 and 144 bytes,
which by Phase 19's rule (`B/8 - 2` virtuals) would make them 14, 15 and 16
virtuals — four to six new ones each, against three or four candidate members.
It does not add up, and it is wrong: **read the bytes and every one of the six
vtables is twelve entries followed by ASCII.**

```
_vt$12VISkinSprite   [12] "12VISkinSprite\0"   at byte 96
_vt$13VIFloraSprite  [12] "13VIFloraSprite\0"  at byte 96
_vt$11VILODSprite    [12] "11VILODSprite\0"    at byte 96
```

The size is the gap-derived span swallowing the RTTI name string that follows.
So `VISprite`'s ten virtuals are the entire dispatch surface of the hierarchy,
**not one leaf adds a virtual**, and everything a leaf declares beyond an
override is a plain member function. Every slot the leaves dispatch through
confirms Phase 19's order independently: `lh a0,32` (entry 4) for
`Collide(sphere)`, 40 for `Collide(ray)`, 48 for `Pick`, 56 for `QueryEmitters`,
64 for `QueryIntersection`, 16 for `GetInstType`.

## Five SPAN_CAP entries, and why they are all the same twelve functions

Every sprite TU carries its own copy of the same twelve unnamed template
functions — the `VIMultiMap`/`VIPool` walkers Phase 19 already met — so the LAST
named symbol in each leaf acquires a two-to-three kilobyte span. The evidence
that it is the same twelve is the shape: the `jr ra` offsets after the cap are
**identical in all six TUs**, shifted only by the capped function's own length.

```
SetMaterialPal__12VISkinSprite      112   (span 2624)
SetMaterialPalDither__13VIFlora     132   (span 2648)
Release__11VILODSprite              152   (span 2664)
Release__13VIGroupSprite            152   (span 2664)
ClampScale__13VIPointSprite         492   (span 3008)
```

## What the classes turned out to be

**`VISkinSprite` (12/12, first try)** — prim buffer, material palette and a flags
word at +0x40 whose **bit 0 is HIDDEN, not visible**: `SetVisible(1)` clears it
and `Raster` draws when it is clear. It has **no collision object at all** — both
`Collide` overloads are two-instruction `return 0` stubs — because a skinned mesh
moves every frame and the collider holds nothing for it.

**`VIFloraSprite` (13/13, first try)** — `VISimpleSprite` with the collision half
deleted: no `m_objIdx`, both `Collide`s stubs, and no `QueryIntersection`
override. The radial flora system puts hundreds on a terrain patch and nothing in
the world may ever collide with one.

**`VILODSprite` (13/14)** — five `{float m_dist; int m_sprite;}` levels at +0x48.
Every virtual walks them from level 0 while the level's switch distance still
covers the viewer and forwards through the vtable; running off the end
(`i == m_numLevels`) is "too far to draw" and returns 0. **`Raster`'s walk has a
body and it is a colour-buffer cursor**: for every level it SKIPS whose sprite is
a `VISimpleSprite` (`GetSpriteType() == 1`) it calls `VIScene::GetNextColorBuffer`,
because a simple sprite consumes one per frame and the cursor would otherwise
slide for everything drawn after it. That call is unguessable from its argument
shape — it takes nothing but the scene — and came out of the dump.

**`VIPointSprite` (9/11)** — the editor's marker for an object that draws nothing
of its own. It owns no geometry: it derives a 0.1-unit bounding sphere, clamps
the model-view, and hands the whole draw to whatever sprite the SCENE has
registered as its point UI sprite; if the scene has none (-1) every virtual is a
no-op. `ClampScale` (123 instructions, matched first try) computes the
**reciprocal** of each of the three row lengths and, where that reciprocal is
below 0.25 — i.e. the row is longer than 4 — scales the row by `1/len * 4`,
landing it at exactly 4.

**`VIGroupSprite` (8/15)** — five `{int m_sprite; VIVect3 m_loc; float m_scale;
VIVect3 m_hpr;}` members, stride 32, and the only leaf that COMPOSES matrices:
each virtual reads the caller's current transform, and per member builds the
member transform, multiplies it on, pushes the product back and forwards. Which
transform "the caller's current" is depends on the query — raster and pick go
through `VIRaster`'s model-view, the collide paths through `VICollide`'s world,
and the two `Query` paths take one as an argument and touch neither.

## Levers

**A count is read twice or not at all.** The first draft of `VIGroupSprite` had
four members. Two independent numbers say five: the constructor's array-ctor loop
is `li v1,4` counting down to -1, and `Copy`'s block-move loop ends at
`this + 228` = `0x44 + 5*32`. `VILODSprite`'s five is `Copy`'s `ldl/ldr` run over
72..111 inclusive. Getting it wrong cost one word in the constructor and
eighteen in `Copy` — a cheap symptom for a layout error.

**A member array is copied by `memcpy`, and the size decides the shape.** 40
bytes goes by pieces (straight `ldl`/`ldr`, no loop); 160 bytes becomes gcc's
runtime-alignment-checked block move — `or`/`andi 7`/`beqz` dispatching between
an `ldl`/`ldr` loop and an `ld`/`sd` one. The crossover is `2 * MAX_MOVE_BYTES`
= 64 on this ABI.

**Matrix locals are declared and zeroed ONE AT A TIME.** `VIGroupSprite`'s three
locals emit ctor-loop, `Zero()`, ctor-loop, `Zero()`, ctor-loop, `Zero()` — not
three ctors then three `Zero()`s. So the source is
`VIMatrix44 base; base.Zero(); VIMatrix44 mem; mem.Zero(); ...`, and the
destructor order confirms the declaration order is base, member, product.

**`VIVect3` needs `operator-=(float)` and `operator+=(float)`.** Phase 11's
compound-operator lever with a scalar on the right: written component by
component, `VIPointSprite::Init` re-reads the radius off `this` every time and
puts both bbox corners on ONE base register; the compound form gives each vector
a base of its own and reads the radius through the SPHERE's (`12(v1)`).
33 wrong words to 23.

**Phase 7's if-direction is per function, and it must be measured.**
`VILODSprite::Raster` and `::Pick` end with the same two-line shape and want
OPPOSITE spellings: `Raster` is byte-exact as
`if (i != m_numLevels) return sc.RasterSprite(...); return 0;` and 6 wrong the
other way, while `Pick` is 23 wrong as
`if (i == m_numLevels) return 0; return sc.Sprite(...)->Pick(...);` and 35 wrong
— and one instruction SHORT — the other way. The largest single win of the phase
came from the same question: `VIPointSprite::Pick` went from **41 wrong to 8** by
moving the whole body inside the `if` and leaving `return 0` at the bottom.

## Open — and it is one phenomenon in eight functions

**The target materialises a fresh register for `VIMatrix44::Zero()`'s asm
operand where our build uses the object's existing base register.** At `sp+0` the
target emits `move a1,sp` and stores `sqc2 $vf4,0x30(a1)`; ours stores
`sqc2 $vf4,0x30(sp)`. Where the address is already in `s5` or `s8` the target
still copies it to `a0` first. It costs five words per matrix, and it is every
remaining difference in `VIGroupSprite::QueryEmitters` and most of them in the
other five matrix functions.

Refuted: the operand's spelling (`"r"(m)`, `"r"(&m[0])`, `"r"(this)`,
`"r"(&m[0].x)` and an `(unsigned)` cast are all byte-identical); an in-out
`"+r"` on a local pointer (74/130 against 42); a `volatile` local (77/130); and
**moving `Zero()` into the constructor**, which produces the right *interleaving*
for free and is worse everywhere (44/130, 60/148) — so the ctor really is empty
and the `Zero()` really is a statement.

Also open: `VIPointSprite::Init` at 23/45 — the same shape as the target with
`&m_bsphere.c` and `&m_bbox.min` in each other's registers, unmoved by six
statement orderings and three spellings of the copy; `VILODSprite::Pick` at
23/84, the scheduler placing two float loads a slot apart; `VIGroupSprite::Copy`
at 18/157, entirely one register (`s3` where the target has `s4`).
