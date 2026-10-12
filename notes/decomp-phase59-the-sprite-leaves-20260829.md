# Phase 59 — the sprite leaves, and the rest of the zone parser

**2026-08-29.  18 functions written, 16 byte-exact.  2,852 → 2,868 of 3,268.**

Phase 58 opened `viesfparse.cpp` and left 44 of its 117 functions unwritten.  This
phase takes the twelve `Parse*SpriteObj` leaves — the dispatcher's whole
right-hand side, the code that turns an ESF tag into a live scene object — plus
six more of the zone parser.  Eleven of the twelve leaves were byte-exact on the
first or second build, because they are the same six steps every time:

    read the id and the bounds
    ask the dictionary whether that id is already live (if so: ReadEnd, return it)
    VIScene::CreateSprite
    VIDictionary::Add
    initialise the sprite from what was read
    walk whatever sub-objects the leaf owns

Only two lines differ between them — the `VISpriteType` and the
`VIResourceType` — so once `ParseStreamAudioSpriteObj` matched, `ParseGroup`,
`ParseSkinLOD`, `ParseLOD`, `ParseFlora`, `ParseSimple`, `ParseSkinSub`,
`ParseSimpleSub` and `ParseEffectVolume` came out of the same template.

## Byte-exact

| function | insns | builds |
|---|---|---|
| `ParseStreamAudioSpriteObj` | 98 | 1 |
| `ParsePointLightObj` | 119 | 3 |
| `ParseSoundSpriteObj` | 143 | 3 |
| `ParseGroupSpriteObj` | 123 | 1 |
| `ParseSkinLODSpriteObj` | 123 | 1 |
| `ParseLODSpriteObj` | 147 | 1 |
| `ParseFloraSpriteObj` | 155 | 2 |
| `ParseSkinSubSpriteObj` | 155 | 2 |
| `ParseSimpleSubSpriteObj` | 168 | 2 |
| `ParseSimpleSpriteObj` | 202 | 1 |
| `ParseParticleSpriteObj` | 175 | 2 |
| `ParseEffectVolumeSpriteObj` | 228 | 2 |
| `ParseStaticLightingObj` | 118 | 1 |
| `ParseMaterialPalObj` | 117 | 1 |
| `ParseZoneRoom` | 152 | 1 |
| `ParseZoneFloraSets` | 135 | 4 |

## The levers

**The order of `m_bsphere.r = radius` and `m_bsphere.c.Zero()` is decided by
whether the radius is a CONSTANT or a VARIABLE.**  `VIPointSprite::Init` stores a
literal `0.1f` and reads `c.Zero(); r = 0.1f;`.  The three parsers that read a
radius out of the file — point light, sound sprite, particle sprite — need the
statements the other way round: with the load first the scheduler puts
`sw zero,32(this)` into the load's shadow and the radius store between the
first and second zero, which is exactly what the target does.  Written the
`VIPointSprite` way the two loads hoist together and eleven words are wrong.
Both spellings also need phase 9's `VISphere *b = &m_bsphere;` so the two
compound operators read the radius as `12(b)` and not `44(this)`.

**`VIObjFile` has no `Read(bool &)`, so a bool member is read through a cast.**
`VIStreamAudioSprite::m_looping` is at +0x40 and the target reads it with
`Read(int &)` straight into the object.  `m_file->Read((int &)p->m_looping)` is
the only spelling that compiles, and `sizeof(bool)` being four on this ABI
(phase 28) makes it identical.  The function was byte-exact first build.

**Hand the CALL the member, not the local it was just stored from.**

    p->m_materialPal = j;
    if (m_raster->ShareMaterialPal(p->m_materialPal) < 0) return -1;

`j` is an address-taken local (it was `VIDictionary::FindTyped`'s out-param) and
the store through `p` may alias it, so `ShareMaterialPal(j)` reloads the stack
slot into a second register.  Reading the member back instead lets cse hand the
call the register the store already used — one load, one register.  The same
edit closed `ParseSkinSubSpriteObj` (23/156) and `ParseSimpleSubSpriteObj`
(29/169) at once.  The same shape appears as
`p->m_sound = ParseSound(); if (p->m_sound < 0) return -1;` — there the store is
what fills the branch's delay slot, where a local `int s` puts the *argument
move* in it instead.

**A store between two calls is a source fact.**  gcc will not move a store
across a call, so where a store lands relative to the `jal`s is where the source
put it.  Three parsers write

    m_file->ReadBegin(type, version);
    p->m_primBuffer = -1;
    m_file->ReadEnd();

with the `-1` between the two calls and the matching `m_materialPal = -1` AFTER
its own pair.  Written symmetrically the flora sprite is three words wrong.

**A named `VIEffectVolume *v` for the plain member stores.**  The method calls
in `ParseEffectVolumeSpriteObj` already materialise `&p->m_volume` into a
register, but four plain member assignments fold back onto the sprite base and
address `128(s3)` instead of `40(s0)`.  Naming the pointer and using it for the
calls too is the whole difference — 4 words.

**Two nested objects want two (type, version) pairs.**  `ParseZoneFloraSets`
opens a 0x32F0 wrapper and then a 0x32F4 and a 0x32F8 inside it; the target
keeps the inner pair in its own stack slots (sp+4/sp+6, both parked in
callee-saved registers because they are used twice).  Reusing the outer pair is
59 of 135 words wrong.  With the second pair it drops to 10, and phase 58's
**ONE shared loop counter** closes the rest: the dist loop and the set loop are
the same `i`.

**A dead local is a source fact** (phase 55).  `ParseZoneRoom`'s prologue zeroes
seven words: the `VIBBox` at sp+0 twice (its constructor, phase 22) and a
four-word object at sp+32 that nothing ever touches, sitting between the box and
the `VIVect3` the portal loop reads into.  A `VIColor32F` local reproduces it
exactly, and the function was byte-exact first build.  Its bounding box is a
version 2 addition, so the zeroing `VIBBox` constructor is load-bearing: an
older file hands `VIZoneRoom::Init` a box of zeroes.

**A dispatcher that returns a RESULT VARIABLE needs `break`, not `return`.**
`ParseZoneResource` keeps an `int i = 0` that only some cases assign.  Written
`case X: i = ParseXObj(); if (i >= 0) return i; goto fail;` gcc proves the value
on every path and deletes the variable — no `s1`, and 169 of 215 words wrong.
Written `if (i < 0) goto fail; break;` with ONE `return i;` under the switch,
the variable survives and the two tail shapes the target has (`bgez s1` for the
assigning cases, `bgez v0` for the rest) fall out of cross-jumping.

## Field-map corrections

* **`VIStreamAudioSprite::m_looping` is at +0x40 and `m_priority` at +0x44** —
  confirmed off `IsLooping`/`GetPriority`, which are a bare `lw 64(a0)` and
  `lw 68(a0)`.  The parser reads the pathname FIRST, then those two.
* **`VIRadialFloraDist` is not `int m_a[7]`** — `ParseZoneFloraSets` names four
  of the seven words: a `unsigned int` DictID at +0x00, an `int` at +0x0C and
  three floats at +0x10/+0x14/+0x18.  +0x04 and +0x08 are never read from the
  file.  `VIRadialFloraSet`'s first two words are a model and a dist index.
* **`VIStaticLighting::ShareResources(VIRaster &)` exists** — `vi.h` had only
  `Clear` and `ReleaseResources`.
* **`VIRaster::MaterialPal` returns the pointer as an `int`**, so the parser
  casts it back before handing it to `VIDictionary::Add`.

## Open

**`ParseGroupSpriteMembers` — 46 of 114.**  The loop fills a fixed five-entry
array of 32-byte members, and every field access is its own induction variable:
eight addresses, and the target has NINE registers for them.  The extra one is a
duplicate of the member's +0 word — `move s3,s2` in the loop preheader — because
`VIDictionary::Find`'s reference argument is a DEST_REG giv (the address as a
value) and `VIScene::ShareSprite`'s argument is a DEST_ADDR giv (a load from the
same address), and the target's `combine_givs` left them apart.  Every spelling
tried — a named `int *`, a `VIGroupMember &`, a `*(int *)&` cast on either side,
an explicit walking pointer — collapses them to eight, and the missing register
shifts the parameter from `s1` to `s2` and every induction variable with it, so
the count sits at 46 regardless of which one is tried.

**`ParseZoneResource` — 50 of 197.**  The structure is right; what is left is
that the target keeps THREE separate `jal ReadEnd` blocks — one in the
`0x1001` case's no-raster arm, one in `0x1110`'s, and one in the default — where
gcc cross-jumps ours into a single copy.  The two case arms are textually
identical, so no spelling separates them; inverting either test moves the count
by one.

## Not written

`ParsePrimBuffer`, `ParseSkinPrimBuffer`, `ParseFloraPrimBuffer` and both
`*ObjV0`, `ParseCollBuffer`, `AdpcmToPcm` and the five `Convert*` pixel
converters, `ParseHSpriteAnimObj`, `ParseHSpriteHierarchy`, `ParseHSpriteObj`,
`ParseCSpriteObj`, `ParseCSpritePlayList`, `ParseCSpritePartEmitters`,
`ParseSurfaceObj`, `ParseMaterial(VIMaterial *)`, `ParseFontObj`,
`ParseAdpcmObj`, `ParseXmObj`, `ParseSpellEffectObj`,
`ParseParticleDefinitionObj`, `ParseParticleDefinition(VIObjFile &, ...)` and
`ParseColorBuffer`'s siblings.
