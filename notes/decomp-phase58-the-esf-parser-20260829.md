# Phase 58 — the ESF PARSER

`decomp/src/viesfparse.cpp`. **The largest class in the corpus that had never
been scored**: 117 functions, 55,984 bytes, and a flat **0 byte-exact** in
`MATCHED.txt` when the phase opened — 21 of them had been sketched in an earlier
phase and never baselined, and 15 of those turned out to be exact on the first
build of this one.

**73 of the 117 are now written and 59 are byte-exact**, and the tree moved
**2,808/3,198 -> 2,852/3,250**.

`VIESFParse` is the one piece of code that turns an `.ESF`/`.CSF` object tree
into live engine objects. Everything the game draws, hears or collides with
comes through it, so its shapes are the shapes of every wire record in
`claude-notes/authoring-a-zone.md`, `esf-object-parsers-decompiled.md` and
`esf-resource-table.md` — read off the code that consumes them rather than off
a Ghidra decompilation of one parser at a time.

## What the phase settled

**`eqcalls.py` is what made this phase cheap.** Every function here is a
straight-line run of calls into `VIObjFile` and back into itself, so naming the
callees off the resident SUPPORT turned each disassembly into a transcript:
`ReadBegin`, a tag compare, N `Read`s at known displacements, `ReadEnd`. Where
earlier phases guessed a callee from its argument shape, this one read the name
and wrote the line.

**There are exactly three shapes, and they repeat all the way down:**

* the **wrapper** — `ReadBegin`, check the tag, hand off to the `*Obj` that does
  the work, forward its result;
* the **walker** — `ReadBegin`, check the tag, loop over the sub-objects calling
  one parse each, `ReadEnd`, with `NumSubObjects()` re-read every iteration;
* the **array walker** — the same, but the count is hoisted into a local because
  the array being filled was sized from it.

A wrapper that hands back an INDEX ends `movn`; one that hands back a STATUS
ends `movz`, and which constant the conditional move puts in `v0` is what tells
them apart.

## Corrections to the field map

**`VIZone` +0x08 is an eight-byte `long`, not two ints.** `ParseWorldZones`
loads it with `ld` and hands it straight to the
`ReadBegin(unsigned long offset, ...)` overload: it is the file offset of the
zone's BASE object, where `m_offset` at +0x10 is the offset of its streamed
data. Recorded as `m_baseOffset`.

**`VIZoneRoom::m_a3C` is the room's ACTOR array and `m_a44` its STATIC LIGHTING
array, one entry per actor.** `ParseZoneRoomStaticLightingsObj` sizes `m_a44`
from the sub-object count and hands `ParseStaticLighting` the room's `m_a3C`,
which is what names both.

**`VIZoneStaticTableElem` is two `(unsigned long offset, unsigned int size)`
pairs**, not the `int m_a[8]` placeholder phase 51 left: `ParseZoneStaticTable`
reads at +0 (`Read(unsigned long&)`), +8, +16 and +24.

**`VICSprite::m_body` and `m_node` are ONE array of ten.**
`ParseCSpriteSkinList` writes `m_body[slot]` for every slot the file names, and
the base it computes is +0xF10 — `m_node`'s nine entries at +0xF14 are the rest
of it.

## Shipped behaviour worth knowing

* **`ParseWorldBase` ignores its `VIWorld` argument entirely** — all four
  sub-parses are handed `m_scene->m_world`.
* **`ParseActorObj` reads a `VIColor32` out of a version-2 file and never uses
  it**: `CreateActor` does not take one. A dead local, the phase-55 shape.
* **`ParseParticleDefinition` does not fail on a foreign tag** — it closes the
  object and returns -1 as a legitimate "no definition here".
* **`ParseCSpriteNodeIDList` forces every entry of a version-0 file into slot
  3**; the `movz` against the version is the whole of that rule.
* **`ParseWorldZoneProxies` reads the zone's NAME out of a version-1 file into a
  32-byte stack buffer and throws it away.**
* `ParseLODSpriteLevels` sets the sprite's own `m_offDist` from the LAST level's
  switch distance, so a LOD sprite's cull distance is data, not a constant.

## Levers

**An index-returning wrapper is TWO RETURNS, not a clamp.**
`int i = f(); if (i < 0) return -1; return i;` gives `slti v1,v0,0` + `movn`.
Every other spelling loses: the ternary `i < 0 ? -1 : i` lets gcc weaken the
test to `i < -1` (`li v1,-2`, `slt`), and `if (i < 0) i = -1; return i;` reuses
the -1 already in a register as the comparand (`slt v1,a0,v0`). Three wrappers
closed on the one edit.

**Nest the body under one bottom return to keep a tail guard from
if-converting.** `ParseWorldObj`'s last guard folded to `slti`/`movz` written as
`if (i) return 0; if (f() < 0) return -1; return 0;`; written
`if (i == 0) { if (f() < 0) return -1; } return 0;` it stays a branch and the
function is byte-exact. This is phase 54's lever run the other way.

**A reference into a container is declared BEFORE the call whose argument setup
it shares.** `VIZoneRoom &r = m_zone->m_rooms[room];` placed above the
`ReadBegin` puts `addu s0,v1,v0` in that call's delay slot; placed after the tag
test it becomes a block of its own. `ParseZoneRoomActorsObj` 55/94 wrong to
byte-exact and `ParseZoneRoomStaticLightingsObj` 63/95 to byte-exact.

**ONE shared loop counter, not two** (phase 41's lever again). `ParseWorldTree`
has two loops; `for (int i...)` followed by `for (int j...)` permutes three
callee-saved registers, `int i;` declared once and reused is byte-exact — 23
wrong words to 0. `ParseZoneTree`, whose extra `m_progress` call changes the
pressure, matches either way.

**A scratch buffer used in ONE branch belongs in that branch's block scope.**
`char name[32]` at function scope lands at `sp+0` and shifts every other slot;
inside the `if (version == 1)` block it lands at `sp+16` with `type` at `sp+0`,
which is the target. Permuting the declarations does nothing — phase 49 already
said block scope is the lever and declaration order is not.

**Two identical tails beat one shared tail** (phase 54, again).
`ParseCSpriteContSound` writes `c.m_snd.m_sound = i;` in BOTH arms of the
lookup; cross-jumping merges them into one store fed by `li v0,-1` on one path
and `lw v0,8(sp)` on the other. Written once after the `if`, the stack local is
stored and reloaded and the function is eight words wrong.

**`a[i]`, not `a.m_data[i]`.** `ParseSurfaces` closed on that single edit — the
subscript's address pseudo puts the shift in `v1` and the load in `v0`, the
member spelling swaps them.

**A switch whose default is the LAST block in the layout is `default: goto
dflt;` with the `dflt:` block written after the function's `fail:` label.**
`ParseSprite` 104/118 wrong to 11, `ParseSubSprite` 36/54 to 8, `ParseSound`
25/38 to 7.

## Open

* **The three dispatchers' default arm.** `ParseSound` 7/38, `ParseSubSprite`
  8/54, `ParseSprite` 11/118, `ParseParticleDefinition` 13/30 — all one
  phenomenon. The target's default block is
  `jal ReadEnd; li s0,-1; move v1,v0; bltz v1,epi (li v0,-1); move v0,s0`: a
  real BRANCH between two exits that both produce -1. Every spelling tried
  folds, because gcc propagates `i == -1` into `return i` and then collapses the
  branch entirely; substituting -2 for one exit proves the fold is
  value-driven (the branch comes back, as a `movz`). `ParseObject`, whose two
  exits are 0 and -1 and therefore cannot fold, is 7/230 wrong on which of two
  `-1` blocks each guard jumps to.
* **The `VICSprite` list parsers** — `ParseCSpriteASlotList` 27/56,
  `ParseCSpriteSkinList` 35/65, `ParseCSpriteNodeIDList` 36/58,
  `ParseCSpriteTSlotList` 38/65. One phenomenon: the target keeps `&c` in its
  own callee-saved register and computes the element base from it, then reuses
  that register as loop scratch; the candidate coalesces the two and needs one
  register fewer. A hoisted `T *p = c.m_array;` moves `ParseCSpriteASlotList`
  from 35 to 27 and no further. Same shape as phase 57's open raster packets.
* `ParseRefMap` 50/96, `ParseHSpriteAttachments` 22/81,
  `ParsePointSpriteObj` 25/61, `ParseLODSpriteLevels` /
  `ParseSkinLODSpriteLevels` 12/72 each.

**Not written** (44 of the 117): `ParsePrimBuffer`, `ParseSkinPrimBuffer`,
`ParseFloraPrimBuffer` and both `*ObjV0`, `ParseCollBuffer`, `AdpcmToPcm` and
the five `Convert*` pixel converters, `ParseHSpriteAnimObj`,
`ParseHSpriteHierarchy`, `ParseHSpriteObj`, `ParseCSpriteObj`,
`ParseCSpritePlayList`, `ParseCSpritePartEmitters`, `ParseSimpleSpriteObj`,
`ParseSurfaceObj`, `ParseMaterialPalObj`, `ParseMaterial(VIMaterial *)`,
`ParseFontObj`, `ParseAdpcmObj`, `ParseXmObj`, `ParseSpellEffectObj`,
`ParseZoneResource`, `ParseZoneRoom`, `ParseZoneFloraSets`,
`ParseStaticLightingObj`, `ParseGroupSpriteMembers` and the six remaining
sprite `*Obj` leaves.
