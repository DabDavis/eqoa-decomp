# Decomp phase 35 — the last sprite leaf, the customisation tables, the spell
# driver, the flora frame and `icon.sys`

**1624/1786 → 1671/1834.** Five classes, 45 of the 47 functions written, **33 of
them byte-exact on the first build**, on the settled toolchain with no
re-tuning — plus **two functions that had been open since phase 20 and phase
32, closed by one line each**.

| Class | written | exact | not attempted |
|---|---|---|---|
| `VIPointLight` | 11 | 11 | — |
| `VICSpriteCust` | 9 | 9 | `SetResources` (1009 insns) |
| `VISpellEffectDriver` | 14 | 14 | `ParticleCount` (650) |
| `VIRadialFloraSystem` | 7 | 7 | `Update`, `Render`, `DrawSwatch`, `CreateSwatch`, `CalcRadialLoc`, `CreateFloraInsts` |
| `VIIconDefinition` | 6 | 4 | — |

Reopened and closed: `VIPointSprite::Raster` and `VIStreamAudioSprite::Raster`.

## The lever that reached backwards: phase 7's if-direction, fifteen phases late

`VIPointSprite::Raster` has been wrong by exactly six words since phase 20, and
phase 32 found `VIStreamAudioSprite::Raster` wrong in the *same* six words and
recorded it as the standing register-allocation phenomenon. It is not. It is
phase 7's if-direction, and the source is the nested form:

```c
if (sc.GetPointUISprite() != -1)
{
    VIMatrix44 m;
    ...
    sc.RasterUISprite(cam, sc.GetPointUISprite(), flags);
}
return 0;
```

Written as the early `== -1` return — which reads far more naturally, and is
how all three were written — the last call's argument moves come out in the
wrong order and the `move v0,zero` lands on the wrong side of the `lq ra`.
Three functions, byte-exact, one line each.

The tell that it was worth re-testing: `Pick`, three functions down in the same
class, is byte-exact with the `!= -1` test, and its two halves are otherwise
the same shape. **Phase 20's "the if-direction is per FUNCTION" is right, and
the corollary is that a neighbouring function matching with one direction is
evidence for trying the other, not against it.**

`VICSpriteCust::GetArmorSetTexture` (10 of 12) and `VIIconDefinition::Ascii2Sjis`
are the same question again, in the same phase.

## `VIPointLight` — the last sprite leaf, and it IS `VIPointSprite`

SUPPORT 0x0045B878 .. 0x0045C950, its own TU, and the ninth and last leaf of the
sprite hierarchy. Five of its functions — `Raster`, `Pick`, both the sphere
`Collide` and `Release`, and `ClampScale` — are **byte-identical to
`VIPointSprite`'s in the target image**, word for word, so the whole TU came in
at nine of eleven on the first build off a copy of `vipointsprite.cpp`.

That identity is also the phase's clearest statement of what the un-relocated
image cannot say. The two classes forward to *different* scene sprites, and the
call sites are the same instruction because `jal` is zeroed. `eqcalls.py` reads
`GetLightUISprite__7VIScene` out of the relocated dump where `VIPointSprite`'s
reads `GetPointUISprite`; nothing in the image distinguishes them.

**`_vt$12VIPointLight` is 96 bytes of vtable** (128 in the map, the rest being
the RTTI name string), so phase 19's ten virtuals hold a ninth time and no leaf
in the engine adds one. And **`VIPointLight` overrides only ONE `Collide`** —
the swept sphere. Every other leaf overrides both; the ray one here is
`VISprite`'s.

Layout: `VISprite` + `VIColor32F m_color` at +0x40, so 0x50 bytes.
`GetColor` returns the colour **by value** (a0 is the sret slot) where
`SetColor` returns `int` — the same asymmetry `VISprite`'s own accessors have.

### The radius is read through the sub-object pointer

`Init(float, const VIColor32F &)` was 14 of 46 wrong on one question. The
target loads `m_bsphere.r` as `12(a2)` — off the pointer the `VIVect3` copy
already materialised for `m_bsphere.c` — where the member name gives
`44(this)`:

```c
VISphere *b = &m_bsphere;

m_bsphere.r = r;
m_bbox.min = b->c;
m_bbox.min -= b->r;
m_bbox.max = b->c;
m_bbox.max += b->r;
```

This is phase 9's pointer variable with the address pseudo created by
`operator=`'s reference parameter rather than by the declaration, and it is
why the same shape in `VIPointSprite::Init` needs no pointer at all: there,
`m_bsphere.c.Zero()` is an inline member whose `this` (phase 32) is the pseudo,
and it happens to be created before the radius is read. Three spellings are
byte-identical — the pointer, the pointer with the member name kept for the
store, and a reference.

## `VICSpriteCust` — the customisation tables, and two corrections

`claude-notes/vicspritecust-tables.md` read every VALUE in these tables out of
EE RAM years ago. What the object code adds is the SHAPES, because the index
arithmetic is the only thing an un-relocated image will say about a static
array — and it corrects that note twice.

* **`$Tints` is FIFTEEN entries, not fourteen.** The static initialiser's loop
  is `li v1,14` counting down through zero, so fifteen iterations of
  `{0,0,0,255}`.
* **`$Race`'s rows are `[4][2]`**, which is what makes
  `race*32 + face*8 + sex*4` one subscript chain: `Race[race][face][sex ? 1 : 0]`.

`$Robe` is `int[4][2]` (slot 0 and slot 3 pick the column, and anything else is
-1), `$Armor` is `int[9][5]`.

**`_GLOBAL_$I$_13VICSpriteCust$Tints` is phase 32's rule a second time**: the
only static member with a constructor is `Tints`, and its whole initialiser is
`VIColor32`'s opaque-black default constructor, fifteen times. `VIColor32
VICSpriteCust::Tints[15];` and nothing else.

**Shipped: `GetArmorSetTexture` ignores its race.** `li a0,20` sits in the
delay slot of the `armorSet == 0` guard, so the race argument is destroyed
before it can be read — the note had this from the decompilation, and here it
is again from the other direction, in the instruction that does it.

## `VISpellEffectDriver` — 14 of 14

One cast in flight: caster, target, eight secondary targets, a 64-bit trigger
time, and a `VILList` of `VISpellEventDriver` — a MEMBER, not a base, said by
its destructor's `__in_chrg` of 2 (phase 29). No vtable: nothing in the
constructor stores a vptr.

Three levers, all of them re-uses:

* **Phase 16's constant hoist reads a statement order straight out of the
  object code, and it is not the field order.** `Destroy`'s tail resets the
  same six fields the constructor does, but in a different order — all 720
  orderings were swept and exactly one is byte-exact:
  `m_effect, m_scene, m_cameraEffect, m_caster, m_target, m_numSecondary`.
  The rule is what makes that readable at all: the store emitted FIRST after
  the join is the source's LAST store of that constant.
* **A count read ONCE.** `AddSecondaryTarget` keeps `m_numSecondary` in a
  register for both the element address and the increment; re-reading the
  member costs 7 of 14 words and moves the address onto the count's own
  register.
* **`sizeof` off a `__builtin_new` immediate**: `VISpellEventDriver` is 100
  bytes, and 104 is one wrong word.

Shipped: **`Trigger` refuses while the driver list is non-empty** — one cast at
a time per driver — and returns -1 rather than restarting.

## `VIRadialFloraSystem` — the frame around the two big functions

`claude-notes/radial-flora-system.md` has the semantics. The object code
confirms the layout down to the last byte: `m_sorted` is 12-byte entries at
+0x48 capped at 700, and 0x48 + 700*12 = **0x2118**, exactly where the rebuild
flag is.

The grid loop runs `li 8` down THROUGH zero — nine iterations, +0x40 walking
back to +0x20 — so the pointer array is exactly nine wide and the sort count
at +0x44 sits immediately behind it.

* **The constructor's statement order is not the field order** either: phase
  16's hoist puts the source's last zero store first, and
  scene/raster/collide/zone/grid comes out 7 of 21 wrong. Of the six orderings
  of its three groups, three are byte-identical and three are 7 wrong, and what
  the three agree on is that **the pointers do not come FIRST** -- which is
  exactly the order the field list would suggest.
* **`Reset` returns void** where `Clear`, `Init` and `ReleaseSwatch` all return
  `int` — phase 32's "a `void` return frees `v0`", one word.

## `VIIconDefinition` — the memory card's `icon.sys`, built in memory

SUPPORT 0x003E9140 .. 0x003E96D8. The class IS the on-card structure: 0x3C4
bytes laid out exactly as the BIOS reads it, with the icon file's address and
length behind it at +0x3C4/+0x3C8. Nothing is guessed — the constructor writes
the magic at +0, memsets 68 bytes of title at +0xC0 and three 64-byte file
names at +0x104/+0x144/+0x184, and `SetIcon` `strcpy`s **"static.ico"** into all
three.

### A magic word is an IMMEDIATE, not a `memcpy`

The 133-instruction constructor went to byte-exact on one line. `'PS2D'` is
`lui 0x4432` + `ori 0x5350` + `sw` — a materialised constant — where
`memcpy(m_head, "PS2D", 4)` loads it out of `.rodata` with `lui`+`lw`. So the
header field is a WORD and the source assigns `0x44325350`, not a four-byte
array copied from the string.

### The background colour is a CLASS; the lights are bare floats

The constructor opens with a four-iteration, sixteen-byte zero loop over
+0x10 — a member array's own zeroing constructor — and then assigns each corner
from a stack TEMPORARY block-copied on with `ldl`/`ldr`. So `m_bgColor` is
`VIIconColor[4]` with a zeroing default constructor and a four-channel one, and
its channels are INTEGER. The three light directions, the three light colours
and the ambient have no such loop, so they are plain `float[]` written field by
field.

**Shipped: two of the three icon lights are BLACK.** Only light 1 and the
ambient carry the 0.65 grey; lights 0 and 2 are left at zero in all four
channels, and the light directions are the three negative axes.

### An uninitialised local is a source fact

`Ascii2Sjis` classifies a character into seven ranges and then indexes one of
two tables. Its `run` variable is **not** initialised at the declaration: the
digits arm assigns `run = 0` explicitly, and hoisting that to the declaration
lets gcc delete the arm's whole `b`/`move a0,zero` pair — 30 wrong words
against 13.

The punctuation table is worth recording as design: the four ranges 32..47,
58..64, 91..96 and 123..126 are mapped onto a dense 0..32 by subtracting a
per-range bias, which is what the constants **1, 11, 37 and 63** are —
`c - (bias + 31)` lands each range on the index after the last.

### The loop variable is read after the loop

`AsciiString2Sjis` writes its terminator as `dst[i]` with the FOR variable, not
`dst[n]`. That is why gcc restores the countdown counter to `n` on the loop's
normal exit, and why the empty-string path writes `dst[0]` — `i` never left
zero. Byte-exact on the first build.

## Harness

Two new `SPAN_CAP` entries, both the familiar cause — the last named symbol of
a TU followed by unnamed twins:

```
ClampScale__12VIPointLightR10VIMatrix44             492   (3008 derived)
GetTintColor__13VICSpriteCust13VICSpriteTint         20   (104,  gcc's own
                                                           static-init helper
                                                           for $Tints)
```

`test_span_caps_only_ever_shrink` passes on both.

## Open

Two, both in `VIIconDefinition`, and both the standing register-allocation
phenomenon:

* `Ascii2Sjis` 13/59 — the two tail blocks have the right instructions in the
  right order with three registers exchanged. Six spellings of the punctuation
  index and five of the alphanumeric arm were swept; all six of the first give
  byte-identical output, so the index expression is refuted as the cause.
* `SetTitle` 9/76 — the newline scan puts the loop index in `a0` and the
  hoisted `'\n'` in `a1`; the target has them the other way round. Five loop
  shapes and all twenty-four declaration orders of the four locals give exactly
  9/76, so neither is the source.
