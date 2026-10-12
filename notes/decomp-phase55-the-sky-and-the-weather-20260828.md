# Phase 55: the sky and the weather

`decomp/src/viatmosphere.cpp` — **30 of `VIAtmosphere`'s 32 functions written and
26 byte-exact**, plus `WeatherFilter::FilterActor`, which turns out to live in
this TU and not in `viactorfilter.cpp`.  The tree moved **2,653/3,020 →
2,679/3,049**.

`VIAtmosphere` is SUPPORT `0x00416880 .. 0x0041C7F8`, its own translation unit,
and `VIScene` embeds one at `+0x640`.  It is the sky dome and its three rotation
matrices, 128 stars and a shooting star, the lightning flash and its 32 patches,
the rain/snow particle field with its splash pool, and the Lavastorm fireballs —
4,528 bytes.

## The layout

The constructor is a member roll-call (phase 53's lever) and the destructor is
that roll-call in reverse, so the object's shape is read straight off them:

* `VILList` at **+0x14** (active splashes), **+0x24** (free splashes),
  **+0x1160** (active fireballs) and **+0x1170** (free fireballs).
* `VIMatrix44` at **+0x500**, **+0x540** and **+0x580** — sky, corona and moon.
* Zeroing loops give `VIVect3[2]` at **+0x50** and **+0x68**, `VIVect3[64]` at
  **+0xA8**, `VIVect4[128]` at **+0x620** (the stars) and `VIVect4[32]` at
  **+0xF60** (the lightning patches).
* The class is `aligned(16)` through `VIVect4`, which is why the last field at
  `+0x11A8` rounds the size to `0x11B0`.

The whole 143-instruction constructor and the 37-instruction destructor matched
on the FIRST build, which is what proves the layout rather than asserting it.

Three nested classes come out of the same reading:

* **`VIAtmosphere::VIWeatherParticle`** — 36 bytes, `VIVect3` location at +0,
  `VIColor32F` at **+0x0C** (`RenderWeather` block-copies exactly those sixteen
  bytes into the billboard), a float at +0x1C (the ground height under the
  particle) and an int at +0x20 (sheltered).  It is NOT a `VILNode`: `Init`
  allocates the field as one `new char[36 * n]` and `Clear` deletes it with no
  array-new cookie.
* **`VIAtmosphere::VISplash`** — a `VILNode` plus location, brightness and a
  frame counter that runs 0..4.  36 bytes, the stride `ClearAllSplashes` walks
  the pool with.
* **`VIAtmosphere::VIFireball`** — a `VILNode`, 48 bytes of its own, and then
  **two** `VIParticleEmitter`s at +0x40 and +0x420.  `Clear`'s `delete []` is
  the proof: the 16-byte array-new cookie, a backwards destructor loop with a
  2048-byte stride, and both emitter destructors in reverse declaration order.
  It also fixes `sizeof(VIParticleEmitter)` at 992.

## What the old Ghidra note got wrong

`claude-notes/weather-system-decompiled.md` was a good map and most of it holds,
but four entries are corrected here:

* **+0xA8 is `VIVect3[64]`, not three interleaved `float[200]` tables.**  The
  constructor's zeroing loop is 64 iterations of 12 bytes, ending exactly at
  +0x3A8.
* **+0xF60 is `VIVect4[32]`, not `float[14*4]`.**  Only 14 of the 32 are ever
  filled, which is what the note's count was reading.
* **+0x4E4 is a two-entry array** — `{"sky", "EQOA_Skydome_360"}` — and
  `SetPlaneOfSky` indexes it; +0x4E0 is the sprite currently selected out of it.
  The note had +0x4E8 as a standalone field.
* **`ResetLightning` CLAMPS, it does not skip.**  The note read the `< 0.1`
  branch as "10% chance of no flash"; it is `VIMIN(VIMAX(0.1f, r), 0.8f)`, with
  the source storing to `m_p11A8` twice over.
* The sky sprites are **Sun at +0x4EC, Corona at +0x4F0, Moon at +0x4F4** —
  `SetResources` binds them in that order, and `RenderSky` renders them under
  `m_skyRot`, `m_coronaRot` and `m_moonRot` respectively.

## One oddity, recorded rather than diagnosed

`RenderStars` builds the shooting star's start and end points from
`((rand() * 131) & 0xFF) / 255.0f * pi * 3.9f` fed to `sinf`/`cosf` — an angle
scaled by 3.9 rather than wrapped, so the "random direction" walks the circle
about twelve times over and is not uniform in any obvious way.  That is what the
instructions say; whether it is a bug or a deliberate cheap hash is not settled
here, and nothing was changed.

## Levers

**A function-scope local and a block-scope local get different FP registers.**
`RenderWeather`'s `sz = m_p88[...] * m_particleScale` is used four times inside
a loop; declared inside the loop's block it lands in `f2`'s slot one way and
declared at function scope the other, and only the second is byte-exact.  The
same lever run the other way closed `RenderStars`: `b` and the two loop counters
declared inside EACH of the two `if` blocks rather than once at the top forced
`idx` into a stack slot and put the second loop's counter in `s7`, which is what
the target does — 141 wrong words to 48.

**`VIMIN(VIMAX(k, x), K)` threads the constant arm.**  Both clamps in this TU
(`ResetLightning`, `GetLightning`) come out as three compares, not two: gcc
proves `K < k` on the arm where the inner max returned the constant and jumps
straight to the join, then re-tests the inner condition on the other arm.  The
argument ORDER is what the branch directions read back — `VIMAX(0.1f, x)` gives
`c.lt.s x, 0.1` where `VIMAX(x, 0.1f)` gives the operands the other way round.

**Name the clamp result, then multiply in a SECOND statement.**  `GetLightning`
ends `VIMIN(VIMAX(0.0f, b), 1.0f) * (1 - m_p11A8) * 0.8f`; written as one
expression gcc distributes the multiply into all three arms of the clamp (24
wrong words), and written as `c = clamp; b = c * (...); b = b * 0.8f;` it is
byte-exact.  The second split is the phase-38 lever — the destination register
of the first multiply must be the RESULT's, which only happens when the result
variable is assigned twice.

**A `VIVect4` four-float constructor hoists its constants.**
`RecomputeLightningPatches` builds `(0, 100, 0, 1)` inside a 14-iteration loop;
as four member assignments the two non-zero constants are rebuilt every
iteration, and as a constructor call they are hoisted into callee-saved FP
registers.  `VIVect4` earned a four-float ctor for it, the way `VIVect3` earned
one in phase 9.

**A dead local is a source fact.**  `RenderWeather` constructs a `VIBBox` it
never reads — 24 bytes zeroed TWICE, which is that class's double-zeroing
constructor (phase 13), and nothing else in the corpus has that shape at that
size.  `ProcessWeather` does the same with a `VIVect3`.

**`Reload` assigns registers in DECLARATION order.**  `Clear` reads
`sc.m_sound`, `sc.m_raster` and `sc.m_dict` into `s0`, `s4` and `s3`; declaring
the dictionary before the raster is the whole difference between 10 wrong words
and byte-exact.

**A subscript with a ternary index scales the ternary, not the index.**
`RenderWeather`'s single-pass arm indexes four arrays by `k ? 1 : 0`; a named
`int i = k ? 1 : 0;` gives `sltu` + `sll`, and the ternary written out at each
subscript gives `li 4` + `movz` — the byte offset selected directly.

**One statement's position decides a whole loop's register allocation.**
`RenderSplashes` matched once `num = m_numSplashes;` moved ABOVE the
`SetMaterial` call; `RenderSky` matched once `r = sc.m_raster;` moved above the
`sceVu0CopyMatrix`.  Both are one-line moves with no semantic content.

## Harness

**Grep for the class before declaring it** — for the third time.  `WeatherFilter`
was already declared in `vi.h` (phase 54) AND defined in `viactorfilter.cpp`,
and re-declaring it in `viatmosphere.cpp` failed the whole TU.  The definition
belongs here: `FilterActor__13WeatherFilter` is at `0x00416880`, the word before
`VIAtmosphere`'s constructor, and `__tf13WeatherFilter` at `0x0041C7A8`, the
word after `ProcessLavastorm` — the class opens and closes this TU.
`viactorfilter.cpp` now carries a comment saying so instead of the function.

## Open

* `Update` 4/128 — the two `sd` stores of the frame clock schedule ahead of the
  `VIVect3` construction where the target interleaves them.  A four-way
  permutation sweep of the block finds nothing below 4.
* `RenderStars` 15/464 — the `if (1.0f <= m_pF2C)` guard comes out as a likely
  branch where the target fills the delay slot from the then-block, and the
  patch loop's clamp keeps its constant in a different register.
* `ProcessWeather` 278/626, written as a DRAFT: the shape is right (two passes
  over the field, a downward `VICollRay` and a `VIEnvRay` per particle, the wrap
  at +-range/2, the splash at the ground) but the expression forms are not
  settled.  Read it for semantics.
* **`Init` (705 instructions), `ProcessTransitions` (858) and `ProcessLavastorm`
  (976) are NOT WRITTEN.**  They are the three biggest functions in the class
  and the phase ran out before them.
