# Phase 18 — VIWalkControl, the first class whose SEMANTICS were already bit-exact

**563/638 -> 584/666.** `VIWalkControl` 21 of 27, same toolchain
(`ee-gcc 2.95.x -O2 -G0 -fno-exceptions`, SN `ps2eeas` 2.95.3-114), **no re-tuning**,
eleven of them first try.

This is the tier-2 simulation class: the thing that turns a stick sample into a
position. It was picked next for one reason — `go-eqoa-vi/viwalk` already ports
24 of its 33 methods **bit-exact against the console**, so the behaviour was
settled before a line of C++ was written and every mismatch below is CODEGEN.
That is the Phase 11/13 position (`VICollRay`, `VICollBuffer`) and it holds up:
the functions that came out right came out right immediately, and the ones that
did not are the standing phenomenon, not a misreading.

## The layout is proved by the constructor

`__13VIWalkControl` is 78 instructions of nothing but inlined `VIVect3::Zero()`,
and it went byte-exact on the first build. That is a **complete check on the
field map**: the eighteen `VIVect3` members are exactly the eighteen offsets it
zeroes (0x74 0x80 0x8C 0x98 0xB0 0xBC 0xC8 0xD4 0xE0 0xEC 0x10C 0x138 0x144
0x150 0x15C 0x168 0x174 0x194), and every gap between them is therefore a member
of some other type. Two things fall out that the Go port did not have:

* **+0x148 is not a float member.** It is the `.y` of a `VIVect3` at +0x144 —
  the alternate pitch lives in the middle of a **second angles triple**. The
  constructor zeroes +0x144 through its own base register, which is what
  `VIVect3::Zero()` gives and three plain floats do not.
* **+0xA4..+0xAF is twelve bytes that are NOT a VIVect3** — the constructor skips
  them, so whatever lives there has no zeroing default constructor.

`_$_13VIWalkControl` is the ordinary GNU 2.x `__in_chrg` destructor with an empty
body, and there is no vtable: nothing in the constructor stores one.

## The clamp is two MACROS, and the double evaluation is the evidence

Every input method clamps its argument with the same inlined nested compare, and
the guest tests **`v < lo` TWICE around a single `hi < v`**:

```c
#define VIMIN(a, b) ((a) < (b) ? (a) : (b))
#define VIMAX(a, b) ((a) > (b) ? (a) : (b))
    t = VIMIN(hi, VIMAX(lo, t));
```

`VIMAX(lo, v)` is evaluated twice by the outer macro, which is where the second
`v < lo` comes from. A helper function gives two tests, and so does either
order of a min/max pair. Where `lo` and `hi` are literals the middle test
degenerates to `hi < lo` and gcc folds it away entirely — which is why
`TurnLeft` skips straight past it on the low path and `SetPitchRange`, whose
bounds are members, does not.

## `m_flags` is a `long` and the `(int)` cast is PER SITE

+0xF8 is eight bytes: four of the six predicates load it with `ld`. The other
two load it with `lw`, and so does `Run`'s slow-run test — **off the same field,
four instructions after an `ld` of it**. The cast is on the field, not on the
mask:

```c
    return ((int)m_flags & 0x10A8) != 0;   // lw   -- (int)m_flags & mask
    if (m_flags & 0x30) return 1;          // ld
```

`(int)(m_flags & mask)` and `int f = m_flags;` both score 3/6; only the cast
applied to the field before the mask is byte-exact.

## Two if-conversion levers, not one

Phase 13 established that `if (x) { … } return 0;` defeats the if-conversion
that gives `movn`. This class splits the other half apart:

* `return (m_flags & 0x30) != 0;` -> `ld` + **`sltu v0,zero,v0`**
* `if (m_flags & 0x30) return 1; return 0;` -> `ld` + `li v0,1` + **`movz`**

Same expression, same type, two shapes. All six predicates are one or the other
and the choice is visible in one instruction.

## One `AddVec` helper serves every input method

```c
static inline void AddVec(VIVect3 &accum, const VIVect3 &dir, float k)
{
    accum.x += dir.x * k;
    accum.y += dir.y * k;
    accum.z += dir.z * k;
}
```

Taking both vectors by REFERENCE is what makes the store to `accum` alias `dir`
on the stack, so the guest writes each `dir` component back and reloads it —
which a version reading `dir` out of registers cannot produce. It is also what
puts `&accum` in a register with its +0 folded back onto `this` (Phase 9).
`static` alone is not enough: gcc 2.95 does not inline it, and the frame grows a
saved `ra`.

**The asymmetric operand order `go-eqoa-vi/viwalk` records is gcc canonicalising,
not the source.** `SwimUp`'s note there says the x term is `k * up.x` and the z
term is `up.z * k`; writing them that way scores 16/48, and the plain
`AddVec(m_force, up, k)` — the same helper `TurnLeft` needs — is byte-exact.
Two functions settle the argument order independently.

## …and a `Cross` helper, which is where the `* 1.0f` fold goes

`UpdateDirections` written with its cross products spelled out term by term
scores 26/84. Through

```c
static inline void Cross(VIVect3 &out, const VIVect3 &a, const VIVect3 &b)
{
    out.x = a.y * b.z - a.z * b.y;
    out.y = -a.x * b.z + a.z * b.x;
    out.z = a.x * b.y - a.y * b.x;
}
```

it scores **9/84**. The difference is that a reference parameter stops gcc
constant-propagating the stack-held `up` vector into the multiplies, so the
guest really does compute `x * 1.0f` against a register instead of folding it
away. The hand-written form folds every one of them and cannot be talked out of
it. (Even through the helper the fold survives in the FIRST component and dies
in the third, inside one inlined body — so the last nine words are the standing
phenomenon and not a spelling.)

## `SetPitchRange`: one reused `float *`, written through, READ through the member

The two clamped pitches are stored through a computed address
(`addiu v0,a0,192` then `swc1 f0,0(v0)`) where a plain member assignment gives
`swc1 f0,192(a0)`. But an inline helper taking `float &` scores 40/97 and a
reference local 26/97, because reading `*p` lets gcc CSE the macro's two loads
and collapses the shape. Byte-exact is the mixture:

```c
    float *p;
    p = &m_angles.y;
    *p = VIMIN(m_maxPitch, VIMAX(m_minPitch, m_angles.y));
    p = &m_anglesAlt.y;
    *p = VIMIN(m_maxPitch, VIMAX(m_minPitch, m_anglesAlt.y));
```

**One** pointer variable, assigned twice. Two pointers, or a reference, both
score 26.

## A selected ARGUMENT, and a duplicated CALL

Two opposite shapes, one lever apart:

* `UpdatePhysics` picks its linear damping into a local and calls `powf` **once**:
  the guest loads the chosen field into `f12` in each arm's delay slot and falls
  into a single `jal`.
* `Walk` does the reverse — the direction is not a selected pointer but **two
  `AddVec` calls in two arms**. A pointer picked with `dir = cond ? &a : &b`
  if-converts to `movn`; the guest BRANCHES and duplicates the whole call into
  both arms, and cross-jumping merges only the tail. That took `Walk` from
  31/116 to byte-exact, and `Run` (170 instructions, four accelerations chosen
  across two nested tests) matched **first try** written the same way.

## `InitParms`: the register numbers are a readout of source order

93 instructions, every one a constant store, and the first build was five words
wrong: two FPRs swapped and one store pair out of order. The fix was a single
source swap — `m_parm38 = 1.2f;` is written **before** `m_parm34 = 0.6f;`, one
break in an otherwise strictly field-ordered function — and it corrected **both**
the store order and the FPR assignment of a constant twelve statements away.
Phase 7's rule ("operand order follows the allocated registers") read backwards:
a mis-ordered pair shows up twice, so the register numbers can be used to FIND
the source order rather than merely to explain it.

The store order otherwise confirms Phase 16's rule exactly: for each distinct
constant the LAST store is hoisted to the front and the earlier ones drop to the
bottom in source order.

## `UpdatePhysics`: the clamp runs on the MEMBER

The +0x18C -> +0x104 decay stores the stepped value into +0x104, **reads it back**
for the macro's third comparison and stores again:

```c
    m_field104 = m_field18C - dt * 6.0f;
    m_field104 = VIMIN(m_parm28, VIMAX(0.0f, m_field104));
```

Through a local it is 225/375; on the member, 164/375 and the whole 80-instruction
decay block is byte-exact. The two gravities are **static data members**
(`VIWalkControl::Gravity`, `VIWalkControl::WGravity`), which is what
`_GLOBAL_$I$_13VIWalkControl$Gravity` is the constructor run for — defined at the
TOP of the TU, or gcc names that initializer after the wrong symbol.

## A correction to `go-eqoa-vi/viwalk`

`locomotion.go`'s `swimState` ends with a comment saying the last comparison's
"BOTH of its outcomes fall into the same store: … It is compiled, not decided."
**It is decided.** At 0x0048741C the guest has

```
    abs.s  $f0,$f3          ; |vR|
    c.lt.s $f1,$f0          ; SpeedThreshold < |vR|
    bc1fl  0x487424         ; branch on FALSE, LIKELY
    li     v0,13            ; delay slot -- runs only when taken
    sw     v0,256(a2)
```

`bc1fl` is a LIKELY branch: its delay slot is annulled when the branch is not
taken. So the two outcomes differ.

* `SpeedThreshold < |vR|` **false** -> branch taken -> the delay slot runs ->
  `v0` = 13.
* `SpeedThreshold < |vR|` **true** -> branch not taken -> the delay slot is
  annulled -> falls through with `v0` still **12**, set unconditionally at
  0x004873E0 (that one is a plain `bc1t`, so its delay slot always runs).

A swimmer drifting sideways faster than the speed threshold animates as MOVING.
The Go port has no `|vR|` test at all and returns `LocoSwimIdle` there.

Also `InitIntermediate` derives the accelerations in the order walk, run, turn,
**swim, pitch** — `intermediate.go` has pitch before swim. Swapping those two
statements is the difference between 20/73 and byte-exact.

## What is still open

Six functions, all structurally right (correct length, correct control flow) and
all the same standing phenomenon — the target keeps a memory reference or a
duplicated computation where ours forwards a value:

| function | wrong/total | shape |
|---|---|---|
| `UpdateDirections` | 9/84 | one `* 1.0f` fold in the third cross term |
| `ViewRelativeWalk` | 40/130 | the up-vector assignment order |
| `PitchDown` | 49/112 | `1.0f * a` not folded; twelve spellings swept, none below 44 |
| `ViewRelativeMove` | 64/139 | as `ViewRelativeWalk` |
| `UpdateLocomotion` | 70/175 | the delta vector is copied TWICE and neither copy is read |
| `UpdatePhysics` | 164/375 | the force copy is reloaded between the three `AddVec`s |

The nine methods that take a `VIScene &` (`Process`, `Move(int, VIScene&)`,
`Init`, `SetLocation`, `SetUpdatePhysics`, `UpdateSprite`, `CalcFit`,
`CalcFloor`) are not started — they are also the nine `go-eqoa-vi/viwalk` has not
ported, which is not a coincidence: the same thing that makes them
unconformable against the console (they need a world) makes their semantics
unread.
