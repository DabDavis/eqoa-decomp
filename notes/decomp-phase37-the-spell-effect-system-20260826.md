# Decomp phase 37 — the spell effect system, and a sweep of scene leaves

**1699/1864 → 1764/1930.** Sixty-six functions written, sixty-five byte-exact,
on the settled toolchain with no re-tuning — and one function open since
phase 31 closed on the way past.

| Group | written | exact | not attempted |
|---|---|---|---|
| `VISpellEventList` + `VISpellEffect` + `VISpellActiveEvent` | 11 | 11 | `VISpellEffect::GetLastImpactTime` (856 insns) |
| `VISpellEffectSystem` (+ its nested playback record) | 24 | 24 | — |
| `VIList<T>`: `Size` / `Begin` / `Next` / `~VIList` across five instantiations | 14 | 14 | — |
| The four remaining filter overrides (+ their `__tf`) | 8 | 8 | — |
| `VIEmitterQuery`, `VISceneSoundTrack(State)`, `VISceneDisplayElem::Compare` | 7 | 7 | `VISceneSoundTrack::SetTracks` (708 insns) |
| `VICapsule` / `VICylinder` `PointInSolid` | 2 | 0 | — |

Thirty-five of the sixty-six were byte-exact on the FIRST build.

## The nested class is named `VISpellEffectPbi`, and the mangled name says so

`DeletePlayback__19VISpellEffectSystemPQ219VISpellEffectSystem16VISpellEffectPbi`
splits as `Q2` + `19VISpellEffectSystem` + `16` + sixteen characters. The obvious
reading — a class `VISpellEffectPb` (fifteen characters) taking a trailing `int`
— is one character short, and it is also refuted by the code: both
`DeletePlayback` and `NextPlayback` touch `a0` and `a1` and nothing else, so
neither has a second parameter. The sixteen characters are `VISpellEffectPbi`,
and the argument list is just the pointer. Getting this wrong costs nothing in
instructions and everything in symbol names: the object simply has no symbol
`eqcheck` can find, which is a loud row rather than a silent miss.

Phase 34's rule again, from the other end — **the mangled name is the argument
list**, and it is also the class name; count the characters before inventing a
parameter to absorb the leftovers.

## A playback handle is not a slot index

`Play` finds a free slot `i` in a 24-entry table and returns

```
i | ((m_serial++ << 8) & 0x7fffff00)
```

with `m_serial` seeded at `0x16062` in `Init`. `Playback(handle)` takes the low
byte modulo 24 to find the slot and then **checks the whole handle against the
one the slot is holding**, so a handle kept past the end of its cast cannot
address the slot's next tenant. That is why `Stop`, `IsPlaying` and
`SpellEffectDriver` all take a handle and not an index.

The remainder is a real `div`, not a multiply-high sequence, and the operand
must be signed for it: `(handle & 0xff) % 24` gives `div`, while
`(unsigned char)handle % 24` lets gcc prove the value non-negative and emits
`divu`. Same value, different instruction.

## Three guards, one `||` — but not all three in one statement

`Play` opens with a raster test and two `-1` tests, and the target reaches ONE
shared `return -1` trampoline from all three, with the constant living in a
callee-saved register that the handle then inherits.

Two things are needed for that:

* **`handle` is declared and initialised before the guards.** Its `-1` is live
  across the `SpellEffect` call, so it goes callee-saved, and cse hands the same
  register to all three comparisons. Declared after the guards (the natural
  place) each guard materialises its own `-1` and only the first gets the
  trampoline — 13 of 120 words.
* **The guards are TWO statements, not one and not three.** All three in one
  `||` scores 3; three separate `if`s score 13; `if (raster == 0) return -1;`
  followed by `if (caster == -1 || target == -1) return -1;` is byte-exact —
  and so is the other split, `if (raster == 0 || caster == -1)` then
  `if (target == -1)`. The object code does not separate those two, so the
  source is recorded as the first and the ambiguity noted here rather than
  hidden.

## Two rules read backwards, both worth the same

**Phase 16's hoist, in Init.** The three constant stores come out `m_inited`,
`m_serial`, `m_playerActor`; the zero stores follow in source order. Read as
"the LAST constant store is hoisted to the front and the rest keep source
order", that says `m_inited = 1` **closes** the block rather than opening it —
which is what matches. The same reading fixes `VISceneSoundTrack::Clear`, where
15000 must be assigned before the 1.

**Phase 16's switch, in `VISpellEventList::Destroy`.** The event types 3 and 9
are tested back to back ahead of both bodies, each `beql` carrying its own copy
of the resource-id load in its delay slot. An `if`/`else if` chain puts the
second test after the first body: 16 of 63 words. A `switch` with two cases and
no default is byte-exact.

## A member that is zeroed twice, and one that is not

`VISpellActiveEvent`'s constructor zeroes six `VIVect3` — three arrays of two —
once in the array constructor loops and again in the body. Two more `VIVect3`
members at +0x14 and +0x20 are zeroed only ONCE, at the top, by their own
default constructors. Writing `.Zero()` for those two as well costs 22 of 95
words and moves every pointer register: the tell is that the second wave of
stores covers exactly the array elements and nothing else.

## Elapsed time is an `int`

`VISpellEffectSystem::Process` stops a playback that has been running for a
minute. The comparison is not on the 64-bit difference: the target follows the
`dsubu` with `dsll32`/`dsra32`, sign-extending the low word before `slt`. So the
source assigns the difference to an `int` first. On a 64-bit `long` the
truncation is not a detail — it is a different comparison.

## The `__tf` of a derived class is a TU artefact

Every TU that needs a class's RTTI emits its own `__tf<class>`, and the symbol
map names exactly one copy. Phase 19 measured that a derived `__tf` INLINES the
base's when the two share a TU (34 instructions against 20). That makes a
derived filter's `__tf` a statement about which TU the map's copy came from, not
about the class:

* `WeatherFilter::FilterActor` sits immediately after `VIActorFilter`'s in the
  image, but the named `__tf13WeatherFilter` is 20 instructions — the
  NOT-inlined form. Based on `VIActorFilter` (defined in that TU) ours comes out
  at 30; based on `VISceneFilter` it comes out at 20 and matches. `VISceneFilter`
  is also the right answer on semantics: these filters are handed to
  `VIScene::Collide`, which takes a `VISceneFilter &`.
* `VISceneOutdoorsFilter` needed the same treatment — moved out of the TU where
  `VISceneFilter` is defined.

So a `__tf` that will not match is worth one question before it is worth an
investigation: is the base defined in this TU, and was it defined in the one the
map named?

## Filters: the accepting direction is the one that branches

`VIWalkCameraFilter::FilterActor` has been open since phase 31 at 2 of 21 words.
It is not a guard chain returning 0. It is

```c
if (scene.GetActorSpriteType(actor) == 5 || actor == m_actor)
    return 1;
return 0;
```

Both `&&` spellings cost exactly two words — one if-converts the second test to
`xor`+`sltiu`, the other stops the two returns cross-jumping — and neither is
the target. Phase 20's "the if-direction is per function" with the value flipped
as well as the test.

The other three overrides fell out of `go-eqoa-vi/vicoll/filters.go`, which had
already read them off the console: `WeatherFilter` accepts every actor, the
owning one included; `VIRadialFloraFilter` rejects every actor, so the flora
walk never collides one at all; `VISceneOutdoorsFilter` accepts only static
actors; `VIWalkFloorFilter` rejects sprite type 5, the `VICSprite`, so the floor
query passes through every player and NPC.

## Shipped

* **`VISpellEventList::Destroy` is the only owner of an event's resources.** A
  CreateEmitter event holds a particle definition and a CreateSprite event a
  scene sprite, and the list hands both back — the driver that used them does
  not.
* **`ReleaseAllSpellEffects` re-reads `Begin()` every iteration and never calls
  `Next()`.** `Erase` takes the element out from under the walk, so it always
  releases the FIRST one; the `while` is what puts the entry branch on the test.
* **`VISpellEffectSystem::SetVibrateCallback` returns the previous callback**,
  where `VISpellEffectDriver`'s returns nothing.
* **`VISpellEffect::Destroy` returns -1 when it has no scene**, so releasing an
  effect that was never created reports failure rather than success.
* **`Play` hands `Trigger` a duration of 10000 and then times the playback out
  at 60000.** The two numbers are unrelated and only the second stops anything.

## Open

Two, both new: `VICylinder::PointInSolid` and `VICapsule::PointInSolid`.

The semantics are settled — `go-eqoa-vi/vicoll` ports both against the console,
including the finding that the axial coordinate is computed TWICE by different
routes and that folding them together does not agree — and the source here
reproduces that. What it does not reproduce is a schedule: the target STORES
each component of the delta before it needs it and then RE-LOADS the direction's
`.y` and `.z` through the sub-object pointer, where ours hoists all three loads
above all three stores and the early stores die. That is phase 27's rule
(a store through a register base kills cse's memory table) in a shape we cannot
provoke: seven spellings were swept — compound assignment, operand order,
component order, a separate scaled vector, `m_dir` versus the pointer for the
first term — and every one of them is 65 instructions where the target is 72,
with the same seven-word shortfall.

The `zero` local is real, though, and it is not a guess: the target compares the
axial coordinate against `0(sp)`, a stack slot three `sw zero` filled and
nothing else writes. A literal `0.0f` would be `mtc1 zero`. So the source
compares against a component of a zeroed `VIVect3`.

## Harness

* **`eqcheck.py` retries the assembler once.** `ps2eeas` runs under wine, and a
  spurious failure there produced a whole phantom regression — eight functions
  in `vicspritecust`, every one of them still byte-exact when that TU was
  rebuilt on its own. A second failure is still reported.
* Two new `SPAN_CAP` entries and no new `SPAN_IGNORE`:
  `NextPlayback__19VISpellEffectSystem...` (16 bytes against a derived 2528, the
  gap being `GetLastImpactTime`'s unnamed helpers) and
  `FilterActor__17VIWalkFloorFilter...` (40 against 2640). One more for
  `__tf19VIRadialFloraFilter` (80 against 2592).
