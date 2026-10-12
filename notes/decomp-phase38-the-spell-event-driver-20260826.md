# Decomp phase 38 — the SPELL EVENT DRIVER

`VISpellEventDriver`, SUPPORT `0x00475E68 .. 0x0047A978`, one translation unit.
**28 of its 35 functions are byte-exact**, 20 of them on the FIRST build, on the
settled toolchain with no re-tuning: `1764/1930 -> 1792/1965`.

Phase 37 built the layer underneath (`VISpellEventList`, `VISpellEffect`,
`VISpellActiveEvent`, `VISpellEffectDriver`), and `claude-notes/spell-event-driver.md`
had already read the semantics off Ghidra, so almost every miss here was codegen.
The exceptions are the three big traversals — `ProcessProjectileEvent` (1187),
`ProcessCreateEmitter` (420) and `ProcessLightningEvent` (399) — and
`NewPlayTargetProjectile` (609), which are not attempted.

## What the class is

One event LIST of a spell effect while it is running: a cursor into the event
array, whatever that list has started that is still going, and the two animation
handles it waits on.  `VISpellEffectDriver` owns one per list (caster, target,
area), and `Process()` is a seven-phase state machine that dispatches exactly ONE
new event per call — so two consecutive non-Wait events are a frame apart.

## The destructor says what a pointer POINTS AT

The constructor gives the whole layout (it stores every field) and the destructor
gives the types.  `spell-event-driver.md` had +0x28 and +0x2C down as
`reserved1` / `reserved2`; `eqcalls.py` reads the two `jal`s that free them as
**`_$_11VILightning`**, not `_$_18VISpellActiveEvent`.  They are the bolt pair the
lightning event draws with.

```
+0x00  VILNode
+0x10  VISpellEffectDriver *m_driver
+0x14  VIScene             *m_scene
+0x18  int                  m_actor
+0x1C  int                  m_index          cursor into the event array
+0x20  VISpellActiveEvent  *m_wait
+0x24  VISpellActiveEvent  *m_projectile
+0x28  VILightning         *m_bolt0
+0x2C  VILightning         *m_bolt1
+0x30  VISpellActiveEvent  *m_lightning      NOT freed by the destructor
+0x34  int                  m_action         -1
+0x38  int                  m_anim[4]        "Spell FX 1".."Spell FX 4"
+0x48  VISpellEventList    *m_list
+0x4C  VILList              m_active
+0x58  VILList              m_inactive       100 bytes
```

**`m_anim` is an ARRAY, not four named words** — `ProcessWaitEvent`'s sub-type 3
indexes it (`sll 2` off the event's own field) rather than naming a member.

Two layouts came with it: **`VISpellActiveEvent`'s +0xB0 is a 64-bit start time**
(`NewWait` stores `VIScene::GetTime()` there with a single `sd`, which is what
forces the class's alignment to 8), and **`VIHSprite` reaches at least +0x188** —
`NewCreateSprite` hands `AddPlay` the word at `sprite + 0x184`, which is past
everything phase 25 modelled.

## The levers

**A bit test that becomes a 0/1 VALUE is `int x = 0; if (bits & m) x = 1;`.**
`NewShakeCamera` keeps two flags out of one word and the target materialises each
mask with `lui` and normalises with `movz reg,zero`.  Every spelling of the
predicate — `(bits & m) != 0`, `!!(bits & m)`, `(bits & m) ? 1 : 0`, a `bool`
initialised from the mask, an inline accessor returning the comparison — gives
`sra`+`andi 1` instead and never materialises the mask at all.  Only the
assign-1-into-a-zero-initialised-local form (or its `if`/`else` twin) produces
`li`+`move`+`movz`.  That is phase 18's rule with both halves visible in one
function, and it was worth 21 of 73 words.

**ONE variable, not a separate `t`.**  Both interpolators —
`ProcessSetEmitterMotif` and `ProcessFadeSprite` — sat two words short for a long
time on the destination of a single `div.s`.  Written

```c
float t = (float)(now - start) / (float)duration;
float f = from + (to - from) * t;
```

the division lands in the dividend's register; written

```c
float f = (float)(now - start) / (float)duration;
f = from + (to - from) * f;
```

it lands in the register the clamped result ends up in, which is what the target
has.  Both functions went to byte-exact on that one edit.  Six other spellings
(a named delta, a named elapsed, a named divisor, the multiply commuted, the sum
reassociated, the clamp bound to a local) are all two words short.

**The two guards of a function need not be the same shape.**  `NewShakeCamera`
tests the player actor and then the camera effect.  Written with both nested, or
both as early returns, the join's `move v0,zero` and `lq ra` come out the other
way round; the actor test is an **early return** and the camera test an `if`
whose body runs to the bottom.  Phase 23's "nested guards share one return at the
bottom" is not a property of a function, it is a property of each guard.

**A switch whose cases all retire the same object shares nothing after it.**
`ProcessWaitEvent` has five cases and every one of them ends in
`delete m_wait; m_wait = 0;`.  Written that way — five copies, which gcc
cross-joins down to one — the switch's own out-of-range `sltiu` test targets the
bare `return 0` that the retire falls THROUGH into.  Written with one shared tail
after the switch and a `default: return 0;`, the default's `return 0` becomes a
SECOND copy placed early and every branch to it is wrong: 5 wrong words against
69.  A tail after the switch is on the default's path, and here it must not be.

**The completion check is NESTED.**  `Process` ends with five conditions that all
mean "not finished".  As a flat chain of `if (x) return 1;` the LAST one
if-converts to `sltu v0,zero,v0`; nested,

```c
if (m_index == m_list->m_count)
    if (m_active.m_head->m_next == 0)
        ...
            if (m_lightning == 0)
                return 0;
return 1;
```

it branches, which is what the target does.  Phase 17's `VIXm::IsCached` shape,
and it took `Process` from 47 wrong words to 0.

**Reading a field TWICE is the else arm.**  `NewCreateSprite` looks the sprite up
by id and then copies it if it is hierarchical.  `int sprite = ev->m_id;` followed
by the `if` reads the id once into a register; `int sprite;` with
`else sprite = ev->m_id;` reads it twice — the second read is the branch-likely's
delay slot — and that is the difference between 30 wrong words and 0.

**A `bool` argument compared against 1.**  `ProcessCreateSprite` and
`ProcessCreateSound` take one, and the target tests it with `bne reg,1`, not
`beqz`: the source is `active == true`, not `active`.

**The jump table gives the case ORDER as well as the case LABELS.**  Phase 34's
rule, used twice: `Process`'s dispatch switch is read out of the relocated dump
at `0x004EBA70` and its bodies come out in source order, which is **1, 2, 3, 6,
5, 4, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17** — not sorted.  The active-event
switch at `0x004EBA40` is CreateEmitter, CreateSound, CreateSprite,
SetEmitterMotif, FadeSprite, i.e. cases 3, 11, 9, 6, 14.  Written in numeric
order both switches are wrong by tens of words.

**A float zero written to two adjacent fields is a CHAIN.**  `NewCreateEmitter`
ends `e->m_f9C = e->m_fA0 = 0.0f;` — two statements give `sw zero` twice (gcc
knows 0.0f is all-zero bits), a chain puts it in an FPR (phase 14).  The stores
come out LEFT to right here, where phase 7's three-deep `VIMatrix44D` chain came
out right to left, so the direction is worth testing rather than assuming.

**The success path in the `if`, a fifth time** (phase 7).  `NewFadeSprite` ends
with a null test on the material it just fetched.  As a guard returning -1 the
failure block lands AFTER the body; as `if (mat) { ...; return 0; } return -1;`
it lands in front of it, sharing the block the three earlier `bltz` returns
already put there.  Same question decided the transform pick in
`ProcessCreateSprite` (`!= 5 ||` inline, not `== 5 &&`) and the palette pick in
`ProcessFadeSprite`.

**A separate `pal` local.**  `ProcessFadeSprite` chooses a material palette out of
an `if`/`else` and then looks the entry up.  Assigning the *material* in each arm
scores 18; assigning the *palette index* in each arm and doing the lookup after
is byte-exact.

## Shipped

* **`ProcessCreateSound` builds a `VIMatrix33` that nothing reads.**  It copies
  the upper 3x3 of the actor's world transform into a local and then hands the
  scene only the translation.  gcc cannot delete it: the class has an out-of-line
  constructor and destructor.
* **`NewCreateSprite` calls `VIScene::GetActorSpritePtr` and throws the result
  away**, immediately after creating the actor.
* **`NewPlayTargetLightning` declares three zeroed `VIVect3` it never uses.**
* **`NewSetAction` and `NewPlayAttack` return -1 when the actor is not a
  `VICSprite`** — and reset the action and all four "Spell FX" handles on the way
  out — where every other failure in them returns 0.
* **`ProcessCreateSprite` and `ProcessCreateSound` do nothing at all unless their
  `active` flag is 1**, the auto-expire included, so an event on the INACTIVE
  list is only ever drained by `ProcessInactiveEvents` calling
  `ProcessCreateEmitter`.
* **`NewSetEmitterMotif` takes a blend over rather than restarting it**: an
  existing motif event on the same emitter hands its `VIMotifBlender` to the new
  event, which starts from the factor it had reached, and the old event is moved
  to the inactive list with its blender pointer cleared.
* **`ProcessSetEmitterMotif` retires the event when the blender has no motif**,
  not when the blend finishes.
* **`NewFadeSprite` CLONES the sprite** so its opacity can be driven
  independently — a simple sprite has a copy made and the actor re-pointed at it,
  a hierarchical one has its first attachment replaced — and the fade starts from
  whatever alpha the cloned material already had.
* **`NewPlayTargetLightning` loops `m_numSecondary + 1` times**, iteration 0
  being the primary target and the rest the secondaries, and skips any driver
  that already has a lightning event.

## Open

Four, and none of them is structural.

* `ProcessCreateSprite` 38/133 and `ProcessCreateSound` 34/148 — the twins.  Both
  are instruction-for-instruction right and differ only in that the target
  rematerialises the constant -1 three times where ours keeps it in one register,
  which shifts everything after it by one word.  The event's flag word lives in
  `a0` in the target and in `v1` in ours; seven spellings were swept (a named
  `int flags`, an `unsigned` one, the actor hoisted out of the guard, the guards
  nested, the two mask tests as `else if`, the field read through the event each
  time, the sprite as a `VICSprite *`) and every one is the same shape.
* `ProcessWaitEvent` 5/134 — four words, the sprite in `a2` against our `a0` in
  sub-type 0, plus one masked jump-table relocation the aligned counter still
  charges for.  Four case-0 spellings swept.

Not attempted: `ProcessProjectileEvent` (1187), `NewPlayTargetProjectile` (609),
`ProcessCreateEmitter` (420) and `ProcessLightningEvent` (399).

## Harness

Nothing new, and nothing needed: the translation unit is contiguous, so every
one of the 35 symbols' gap-derived spans is its own and no `SPAN_CAP` or
`SPAN_IGNORE` entry was added.
