# Phase 53 — the CHARACTER SPRITE (`VICSprite`)

**2026-08-28.  44 of 50 written, 30 byte-exact; the tree is 2,538 of 2,895.**

`VICSprite` (SUPPORT `0x00420F20 .. 0x004283C8`) is what every player and every
NPC in EQOA actually *is*: a `VIHSprite` with the whole customisation layer
bolted on top of it — sixty-six animation slots, nineteen texture slots, six
attach points, a collision capsule, a weapon trail, one sound, and an array of
node-bound particle emitters.  At **4,624 bytes it is the biggest object in the
corpus**, and the class is what this phase is really about: the layout was read
off the constructor and `Clear`, then checked against a `this`-offset census
over all fifty methods (`re-tools/thisscan.py`), which touches nothing outside
the members below.

## The class

```
+0x190 int                    m_flags        1   bit0 NPC, bit1 skins, bit2 player
+0x194 int                    m_skelType
+0x198 int                    m_race         9   (VICSpriteRaceNone)
+0x19C int                    m_sex
+0x1A0 int                    m_materialPal -1
+0x1A4 int                    m_refMap      -1   its OWN, not VIHSprite's +0x180
+0x1A8 VICSpriteAnimEntry     m_entries[66]      52 bytes each
+0xF10 int                    m_body[1]     -1
+0xF14 int                    m_node[9]     -1   3 = name plate, 8 = helm
+0xF38 VICSpriteTextSlotEntry m_slots[19]        24 bytes each
+0x1100 VICSpriteHair         m_hair             { 0, 2, 1 }
+0x110C VICSpriteItem         m_items[3]         12 bytes each
+0x1130 int                   m_attach[6]   -1   0/1 hands, 3 skin, 4 name, 5 helm
+0x1148 VICapsule             m_capsule
+0x1168 int                   m_dead
+0x116C VICSpritePlay         m_prev[2]          16 bytes each
+0x118C VICSpritePlay         m_cur[2]
+0x11B0 long                  m_animTime
+0x11B8 long                  m_idleTime
+0x11C0 int                   m_trailSlot    1 in the ctor, 0 in Clear
+0x11C4 VIColor32F            m_trailColor
+0x11D8 long                  m_trailTime
+0x11E0 VITrailFx            *m_trail
+0x11E4 int                   m_deathAnim   -1
+0x11E8 int                   m_action      -1
+0x11EC int                   m_actionSeq    0
+0x11F0 VICSpriteSoundState   m_snd              sound, volume, channel, time
+0x1208 VIArray<VICSpritePartEmitter> m_emitters
```

**Its own data starts at +0x190, not at +0x188 where `VIHSprite`'s last member
ends.**  `VIHSprite` carries a `VIMatrix44`, so it is `aligned(16)` and its SIZE
rounds up; the eight bytes of the base's tail padding are why nothing in the
class ever touches +0x188.  This is the first place in the corpus where a base
class's tail padding is visible, and it would have shifted every field by two
words if it had been missed.

## The constructor is a MEMBER-CONSTRUCTOR ROLL-CALL

gcc 2.95 emits, in order: the base constructor, the vptr store, **every member
constructor in declaration order**, and only then the body.  So the run between
the vptr store and the last `jal` is a LIST of which members have constructors
of their own, and reading it is what settles four of the field groups:

* `m_entries[66]` and `m_slots[19]` are arrays whose ELEMENT has a constructor —
  gcc's vec-init is an ascending pointer with the counter counting down from
  `n-1` through `-1`, which is exactly what both loops do (65 and 18).
* `m_hair` is a CLASS, not three ints: its three stores sit between `m_slots`'
  vec-init and `VICapsule`'s constructor, where no body statement can go.
* the same argument makes `m_snd` a class — its three stores and the eight-byte
  zero sit between `VIColor32F`'s constructor and `VIArray`'s — and **`Clear`
  never touches any of those four fields**, which is the other half of the
  evidence: they are initialised once, at construction.

The four `-1` loops at the END of the constructor are body code, and they come
in two shapes that the object code separates every time: a DESCENDING pointer
(`m_body`, `m_node`, `m_attach`) is a source-level `for (i = N-1; i >= 0; i--)`,
where an ASCENDING pointer with a down-counting register (`InitTextSlots`) is
the ordinary `for (i = 0; i < N; i++)` gcc reversed.

## Levers

**The priority table is written LAYER BY LAYER, not entry by entry.**
`SetAnimPriorities` is 122 straight-line stores into `m_entries[i].m_priority[0]`
and `[1]`.  Writing them as 56 interleaved pairs in index order scores 119/122;
writing the object code's own emitted order scores 70/122; writing **all 56
`[0]`s in index order and then all 56 `[1]`s** is byte-exact on the first build.
The stray `[1]` stores that appear early in the target (entries 0 and 9) are the
scheduler hoisting them into the `[0]` block, not source order.

**A two-case if-chain on a sprite type is a SWITCH.**  `SetSkinsVisible`'s
`type == 3` / `type == 7` pair written as `if / else if` gives two ordinary
branches; written as a `switch` it gives the target's two `beql`s and is
byte-exact (17 wrong to 0).  Same lever as phase 47, now with the values held in
callee-saved registers hoisted out of the loop.

**`~4` is one `li`, `~4u` is `lui`+`ori`.**  Three flag clears in this class
(`SetIsPlayer`, `Init`, `SetSkinsVisible`) and one in `SetRobe` all need the
UNSIGNED complement; the signed form is one instruction shorter and wrong.

**A hoisted `this` for a call argument costs a callee-saved register.**
`CreateParticleEmitters` written as
`m_emitters[i].m_emitter.Create(ps, ps->ParticleDefinition(...))` computes the
emitter's address BEFORE the call and parks it callee-saved — 16 more bytes of
frame and 30 wrong words.  Assigning the definition to a local first
(`d = ps->ParticleDefinition(...); ...Create(ps, d)`) puts the address
computation after the call, where the target has it, and closes the function.
The same phase also needed `VIParticleSystem *ps = sc.ParticleSystem();` hoisted
into a local — the target calls the inline accessor once, not once per use.

**A local's declaration ORDER decides the prologue.**  `Share` declares the
sound device before the particle system; the other way round costs 12 words of
prologue shuffling (34 wrong to 22).

**A `return f() < 0 ? -1 : 0` tail is `slti`, and both spellings of the test
give the OTHER one.**  `SetHelm`'s last statement wants `slti v1,v0,0` +
`li v0,-1` + `movz`; `x < 0 ? -1 : 0` and `x >= 0 ? 0 : -1` both give
`li v1,-1` + `slt v0,v1,v0` + `movn` + a copy.  Still open.

**The constructor's element-constructor field order is the SOURCE order.**  gcc
is stable for a run of independent stores here: writing `VICSpriteAnimEntry`'s
thirteen initialisers in the object code's own order (`m_animation[1]` before
`[0]`, `m_channel` before `m_pitch`/`m_volume`) reproduces it exactly, and the
naive index order does not.  24 wrong words to 11.

## Shipped bugs

**`SetHelm` tints material palette entry 0, N times.**  The outer loop runs
`for (i = 0; i < r->GetMaterialPalSize(pal); i++)` and the body calls
`GetMaterialPalEntry(pal, 0)` — the constant is in the object code, `a2` is
`zero` at the call.  Every other entry of a copied helm palette keeps its
original colour.

**`SetHelm` calls `VIRaster::MaterialPal(pal)` and throws the answer away.**
The `jal` at `0x00422B4C` is `MaterialPal`, its `v0` is never read, and the
loop's own bound comes from a *different* function (`GetMaterialPalSize`) at the
loop tail — so it is a statement, not the loop test.

**`AttachItem` does nothing for a player.**  Its first test is
`if (!(m_flags & 1)) return 0;`, and bit 0 is the NPC flag `Init` sets from its
last argument.

## Open

`SetHair` 192/235 (three slot-range loops and a tail that reuses the third
loop's index at 14), `AttachItem` 110/162, `Release` 61/177 and `Share` 22/155
(both cross-jump their early `return -1`s into one out-of-line trampoline where
ours branches straight to the epilogue), `SetDefaults` 35/94, `SetItemAction`
32/79 and `IsActionComplete` 6/18 (both if-convert a `return 1`/`return 0` pair
to `sltu` where the target branches — the -1 sentinel is CSE'd into a
callee-saved register in ours and rematerialised in the target, which is what
makes the conditional move cheap enough to win), `SetAnimSoundChannels` 35/80
(one instruction: `move v0,zero` in the `jr` delay slot rather than scheduled
into the middle), `SetArmorSlot` 29/94, the constructor 11/214 and `Clear`
11/128 (both the same two `li` orderings inside the `-1` loops plus the sound
group's store order), `Init` 7/45 (a parameter-to-callee-saved assignment that a
24-way statement sweep does not move), `SetHelm` 8/131 and `SetDeath` 2/122 (one
parameter spill).

**Not attempted:** `SetName`, `SetDefaultEntries`, `SetAnimation`, `Process`,
`CalcVolumeAndPan`, `Copy`, `ProcessWeaponTrails`, `UpdateParticleEmitters` —
six of them over 230 instructions, and `SetDefaultEntries` is a 66-way switch
whose jump table is read out of the dump at
`_9VICollRay$PerturbMult+0xE0` (six distinct case bodies: a copy of entry 0, a
copy of entry 41, and four more).  The two attack-action jump tables at +0x20
and +0x50 ARE decoded and both functions are byte-exact.

## Harness

`decomp/_perm.py` is new: a permutation sweep over a contiguous run of source
lines that rebuilds one TU and scores one symbol, loading the target image once
so a 24-way sweep is four minutes instead of twenty.

`viarray.cpp`'s `VICSpritePartEmitter` stub is gone — the real class is in
`vi.h` now, with the constructor phase 24 had measured off `VIArray<T>::Init`
kept verbatim.  Leaving both in place fails the whole `viarray` TU and reads as
a six-function regression (phase 41's rule, third occurrence).

`VICapsule`/`VICylinder` moved UP in `vi.h`, above `VICSprite`, because
`m_capsule` is a member by value; `VITrailFx` only needed a forward declaration.
