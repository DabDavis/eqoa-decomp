# Decomp phase 28 — the particle system

**1,252/1,384 -> 1,329/1,468.** Five classes and 77 of the 84 functions written,
on the settled toolchain (`ee-gcc 2.95.3-114 -O2 -G0 -fno-exceptions`, SN
`ps2eeas`) with no re-tuning, and 60 of them byte-exact on the FIRST build.

| class | SUPPORT | score |
|---|---|---|
| `VIParticleMotif` + `VIParticleDefinitionEx` | 0x0045A3D8 | 24/24 |
| `VIParticleDefinition` | 0x004565A0 | 10/10 |
| `VIParticleSystem` | 0x00458780 | 10/12 |
| `VIParticleEmitter` | 0x00456910 | 23/25 |
| `VIParticleSprite` | 0x00454E38 | 10/13 |

The six big ones are deliberately not attempted: `VIParticleEmitter::Update`
(696), `::RecomputeStaticBBox` (510), `VIParticleSprite::Release` (660) and
`::Raster` (394), `VIParticleSystem::Render` (332) and `VIParticleSprite::Pick`
(174).

## `sizeof(bool)` IS FOUR HERE

`VIParticleDefinitionEx::SetZWriteEx(bool)` stores its parameter with `sw`, and
`VIParticleDefinition::Create` passes the member straight to another `bool`
parameter with **no `sltu`** in between.  Those two facts cannot both hold for an
`int` member, and they cannot hold at all if `bool` is a byte.  Measured on the
compiler itself:

```
struct S { int pad[3]; bool b; };
int  sizeofbool()          { return sizeof(bool); }   ->  li $2,4
void setb(S *s, bool v)    { s->b = v; }              ->  sw $5,12($4)
void g(T *t)               { f(t->b); }  // int -> bool:  sltu $4,$0,$4
```

So `m_zWrite` and `m_zTest` are `bool`, four bytes each, and the `sltu` that
*does* appear -- inside `VIParticleDefinition::SetZWrite`, feeding
`VIMaterial::SetZWrite(int)` -- is the source's own `m_zWrite != 0`.  Getting
this wrong costs seven words in `Create` and nothing anywhere else, which is why
it survived twenty-seven phases: no earlier class had a `bool` member.

## The inline `Value(i)` lever, at four sites at once

`VIParticleDefinitionEx` walks its `VIPool<VIParticleMotif *>` four times
(`DestroyMotif`, `FindMotif(unsigned)`, `FirstMotif`, `NextMotif`).  Written out
as Phase 22's pointer variable --

```cpp
VIPool<VIParticleMotif *> *p = &m_motifs;
VIPool<VIParticleMotif *>::Node *n = &p->m_nodes[i];
```

-- every one of them comes out with `addiu sN,a0,724` followed by `move a0,sN`,
where the target has **`addiu a0,a0,724` followed by `move sN,a0`**: two
instructions either way, six words of prologue apart, and no declaration order,
reference-vs-pointer spelling or loop shape moves it (twelve swept).  What moves
it is Phase 27's lever -- an inline `Value(i)` **in the class body**:

```cpp
T &Value(int i) const { return m_nodes[i].m_data; }
```

The inline's own `this` is a SECOND pseudo, a copy of the argument temporary, so
the address is computed into `a0` for the call and copied out to the
callee-saved register the loop needs.  All four went byte-exact in one edit.
(`FirstMotif` is the exception that proves it: with no loop it matches from the
pointer-variable spelling too.)

## The hoisted constants keep their SOURCE order

Phase 16 established that **the last store of each distinct constant is hoisted
to the front of a constructor's store block**.  `VIParticleDefinition`'s
constructor pins down the rest of the rule: the hoisted stores come out in the
order their *source statements* do, and everything else follows in source order
with those two removed.  Reading the target's nine stores that way gives the
source outright, and three of the six insertions that satisfy it are
byte-exact.

## `VIBBox` has a user COPY constructor

`StaticBounds`, `DynamicBounds` and `Bounds` all return a `VIBBox` by value, and
all three **zero the caller's slot before writing the six floats into it**.  A
synthesized copy constructor would call `VIVect3`'s (which Phase 4 established
is user-declared) and zero nothing.  So:

```cpp
VIBBox(const VIBBox &b) { min = b.min; max = b.max; }
```

-- the members' own default constructors run first, which is the zeroing, and
the body is the copy.  `StaticBounds` is 106 instructions and matched on the
first build with it.

## A `VIResource` MEMBER, not a base

`VIParticleSystem::ReleaseParticleDefinition` hands `d + 0x2FC` to
`VIDictionary::Remove(VIResource &)` and decrements `(d + 764) + 4`, so
`VIParticleDefinition` carries a **`VIResource` at +0x2FC** rather than deriving
from one.  Its inline zeroing constructor -- `m_id = m_link = 0`, a chained
assignment, so `+4` stores before `+0` -- is exactly the 0x300/0x2FC pair that
opens `VIParticleDefinition`'s own constructor, and modelling it as two plain
`int` members with two body statements scores the same but says the wrong thing.
The refcount reads and writes need Phase 9's pointer variable (`VIResource *r =
&p->m_resource;`), or the offsets fold to 768 and the function is six words
wrong.

## `NextMotif` does not return early

When the motif is found and it is the LAST one, the target does not
`return 0` -- it falls out of the `if` and keeps walking the pool, which reaches
the same answer one iteration later.  Writing the early return costs 17 of 56
words, because it makes `this` dead on that path and the register allocator
collapses two live ranges that the target keeps apart.

Phase 7's "the success path goes in the `if`" paid twice more:
`VIParticleEmitter::LinkMotifBlender` (2 words) and `FirstMotif` (5), and Phase
13's nesting -- `if (guard) { ...; } return 0;` -- took
`ReleaseParticleDefinition` from 10 wrong to 0 by letting its two `return 0`
sites cross-jump.

## A by-value `Cross`, and its surviving temporary

`VIParticleEmitter::SetDirection(const VIVect3 &)` builds an orthonormal basis
in 120 instructions, and 12 bytes of its 160-byte frame are **written three
times and never read**.  That slot is the compiler's temporary for the two
statements whose right-hand side is a class value:

```cpp
if (fabsf(v.x*up.x + v.y*up.y + v.z*up.z) > 0.999000013f)
    up = VIVect3(0.0f, 0.0f, 1.0f);
...
up = Cross(v, a);
```

The temporary's address is taken by the reference `VIVect3::operator=` takes, so
its stores are not dead-eliminated even after the values are forwarded.  The
initialisation `a = Cross(up, v)` two lines up produces no such slot, and
`VIVect3 c = v;` none either.  Written that way the function matched on the
first build -- unlike `viwalkcontrol.cpp`'s `Cross`, which is the
reference-out form.

## Layouts settled

* **`VIParticleMotif`** is `VIParticleAttributes` + `char m_name[32]` +
  `unsigned m_nameHash`, **760 bytes** -- what `CreateMotif` hands
  `__builtin_new`.  `SetName` is `strcpy` then `VIHashResourceID`.
* **`VIParticleDefinitionEx`** adds a `VIPool<VIParticleMotif *>` at +0x2D4 and
  the four render flags at +0x2EC..+0x2F8; **764**.
* **`VIParticleDefinition`** adds the `VIResource`, a `VIRaster *`, a material
  index and a texture index; **784**.
* **`VIParticleEmitter`** is a `VILNode` with two `VIVect3`, a `VIMatrix33` at
  +0x28, a `VIMatrix44` at +0x60 (whose `aligned(16)` is what pads +0x58), two
  `VILList`, a whole `VIParticleAttributes` at +0xBC -- 724 bytes, so +0x390
  closes it exactly -- then a static and a dynamic `VIBBox` at +0x394/+0x3AC,
  the attractor at +0x3C4 and three words; **992**.  Phase 24 had already
  measured that number the other way round, off the `VIArray` instantiation for
  `VICSpritePartEmitter`, and stood a 992-byte `char m_body[]` stub in
  `viarray.cpp` for it.  The stub is gone: the real class drops straight into
  its place and all four of that instantiation's functions still match, which is
  an independent check on both derivations.
* **`VIParticleSprite`** is a `VISprite` with the emitter EMBEDDED at +0x50 and
  one more word at +0x430; **1076**.  `m_type` is 10.  Both `Collide` overrides
  are `return 0` stubs, exactly like `VISkinSprite`'s and `VIFloraSprite`'s
  (Phase 20).
* **`VIParticleSystem`** is 108 bytes: the raster, a 64-bit clock at +8, the
  smoothed frame delta at +0x10 and the eight-deep history `Render` averages it
  out of at +0x14, the emitter and free-particle lists, the particle array and
  the definition pool.  **It is EMBEDDED in `VIScene` at +0x548** --
  `VIParticleSprite::Create` and `::Copy` both reach it as `scene + 1352` and
  then null-test the result, which is an inline accessor returning its address.
* **A particle is 104 bytes on a `VILNode`**, with an inline constructor zeroing
  fourteen words at +0x20..+0x54 and sixteen uninitialised bytes either side --
  read straight off gcc's own `new VIParticle[n]` array-init loop.
* **`VIParticleAttributes::m_unk2C8` is the MOTIF WEIGHT.**  Phase 12 could only
  say it was a float between two ints; `VIParticleDefinitionEx::BlendMotifs`
  reads it as the blend factor of each motif and
  `VIParticleEmitter::BlendMotifs` writes it from the linked
  `VIMotifBlender::Factor()`.

## Shipped oddity

`VIParticleSprite::Copy` calls `SetLocation(src.Location())` **twice**.  It is
not a mistake: `SetLocation` shifts current into last, so the second call leaves
the copy with both fields equal to the source's *current* location and the
source's *last* location is silently dropped.

## Open

`VIParticleSystem::Init` at 33 of 84 -- everything structural is settled and the
difference is that the target's `new VIParticle[n]` loop carries a SECOND
induction variable biased by +72, so its fourteen stores address at -40..+12
while the array pointer advances ahead of them; ours reuses the one pointer.
`ReleaseAllParticleDefinitions` at 15 of 36 -- one extra callee-saved copy of
the pool pointer, the standing register-allocation phenomenon, with seven
spellings swept.
