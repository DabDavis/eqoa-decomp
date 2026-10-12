# Decomp phase 36 — the device classes

**1671/1834 → 1699/1864.** Three classes, 28 of the 30 functions written, on the
settled toolchain with no re-tuning. The pad, the keyboard and the combat-music
layer: the three places where the engine talks to hardware it does not own.

| Class | written | exact | not attempted |
|---|---|---|---|
| `VIPad` (+ the four flag accessors) | 14 | 13 | — |
| `VIKeyboard` (+ the two driver entry points) | 8 | 8 | `Read` (286), `CombineDeadkey` (174), `PrepareQueueKeyEvent` (150), `TranslateEvent` (82) |
| `VIBattleMusic` | 8 | 7 | `Unload` (660), `Process` (106) |

## An `ld` is not proof of a 64-bit member

`VIPad::VibrateLf`, `::VibrateHf` and `::Process` all open the same way:

```
ld    v0,32(a0)
xori  v0,v0,0x2
sltiu v0,v0,1
dsll32 v0,v0,0x0
dsra32 v0,v0,0x0
beqz  v0,...
```

An eight-byte load at +0x20, compared against 2. So +0x20 is a `long` — except
that `Read` reads the same offset with `lw` and writes it with `sw`, and `Init`
zeroes +0x20 **and** +0x24 with two separate `sw zero`. Declared as a `long`,
those three functions match and `Init` does not; declared as two `int`s, `Init`
matches and the three do not.

Both are right. **gcc 2.95 merges two adjacent `int` compares into one 64-bit
load**: `m_state == 2 && m_actAligned == 0` is, on little-endian, "the eight
bytes at +0x20 equal 2", and that is exactly what it emits. Four spellings of
the predicate — `&&` either way round, `!m_actAligned`, and an explicit
`*(const long *)&m_state` cast — give byte-identical output, so the merge is the
compiler's and not the source's.

The lesson generalises past this class: **an access WIDER than the field it
lands on says the compiler merged something, not that the field is wide.**

## An inline predicate materialises the comparison

The same five instructions carry a second finding. Written out as
`if (m_state == 2 && m_actAligned == 0)` at each of the three call sites, gcc
emits `ld` + `li 2` + `bne` — a direct branch. The target instead materialises
the comparison as an `int` (`sltiu`, then `dsll32`/`dsra32` to narrow it) and
branches on the value, which is what an inline member returning `int` produces:

```c
int Active() const { return m_state == 2 && m_actAligned == 0; }
```

Three call sites, one shape. The `dsll32`/`dsra32` pair is the 64-bit-to-`int`
conversion, and it is the tell.

## Phase 7's if-direction, five more times

Phase 35 closed two functions that had been open since phase 20 on this rule.
This phase needed it five more times, and once it decided the whole layout of a
290-instruction function:

* `VIPad::ScaleStick` — the POSITIVE half is the `if` and the dead zone's
  `0.0f` is the fall-through (11 of 27 words, and it is also what gets the
  second comparison hoisted into the first branch's delay slot).
* `VIPad::Read`, twice. **The ready path is the `if` body and the not-ready
  teardown is the function's TAIL** — written as an early
  `if (st != 2 && st != 6) { … return 0; }` gcc leaves the teardown inline
  where the test is, and 86 of 290 words move. Then, inside it, **the unpack is
  the `if` body and the SetMainMode arm the fall-through**, which is what puts
  the three trailing blocks in the target's order.
* `VIKeyboard::DequeueKeyEvent` — the drain is the `if` and the empty case is
  the tail; the early `== 0` return keeps it inline behind a branch-likely
  (27 of 29 words).
* `VIBattleMusic::Stop` — the arm that lands last is the source's `if`.

## Three more shapes, each worth a line

**Phase 23's `||`.** Two `if (X) return 0;` statements in a row branch AROUND
the first test and cost a `b` to the shared epilogue; one disjunction sends both
straight there. `VIPad::Read`'s `scePadRead(...) <= 0 || m_data[0]` — 17 wrong
words to 6, and the length goes from 291 back to 290.

**A nested ternary, not two returns.** `VIBattleMusic::Preload` ends with
`return track == 0 ? 0 : (m_sound > -1 ? 0 : -1);`. Both arms feed one pseudo
and the function closes on `move v0,a1`; written as `if (track == 0) return 0;`
followed by the inner ternary, that final move disappears and 21 of 62 words
move.

**The guard before the assignment.** `VIBattleMusic::Play` reads
`if (m_track == -1) return -1; track = m_track;`, and gcc steals the assignment
into the guard's delay slot (it is dead on the returning path). Assigning first
and re-testing costs a reload and 14 of 90 words.

And two re-uses: **the count read ONCE** in `VIKeyboard::QueueKeyEvent` (13 of
15 words — the same lever as phase 35's `AddSecondaryTarget`), and **phase 16's
constant hoist** in three constructors, where once again it is not field order.
`VIBattleMusic::Init` is the sharpest: the first `-1` store emitted is +0x1C, so
`m_channel` is the source's last `-1` store, and the float `m_rate` closes the
block after it.

## Layouts

**`VIPad` is 96 bytes and the last field closes it exactly.** Phase 27 measured
96 off `VIConsole`'s own offsets and left it as `char m_data[96]`. `scePadRead`
is handed `this + 60`, the buffer is 32 bytes, and 0x3C + 32 = 0x5C — which
rounds to 96 on the eight-alignment the three `long`s force. Nothing is left
over.

**`VIConsoleKey` is TWO bytes, not sixteen.** Phase 27 guessed 16 from the
`VIConsole` frame; the keyboard's own queue settles it — indexed by
`sll a1,a1,1`, copied with two `lb`/`sb` pairs. `viconsole.cpp` still matches
after the correction.

**`VIScene + 0x4BA4` is the `VISoundDevice *`**, read by all four of
`VIBattleMusic`'s device paths. It sits between the raster at +0x4BA0 and the
collide at +0x4BA8, filling the last unnamed word of that block.

`VIKeyboard` is 0x218 with the event queue reached through a POINTER to a file
static rather than embedded. `VIBattleMusic` is 0x20: scene, track, state,
loaded, a 0.01 rate, a stop flag, a sound index and a channel.

## Shipped

* **`VIPad::Read` initialises a 32-byte local it never reads.** Four
  `ldl`/`ldr` pairs copy a constant off `.rodata` into the frame at every call,
  and the actuator alignment the function actually sends is a *file static*.
  gcc 2.95 does not delete a dead initialised local array, so the source has one
  — and our own build reproduces it instruction for instruction.
* **Every button is the COMPLEMENT of one bit**, and gcc rewrites
  `(~b >> 7) & 1` as `(b ^ 0x80) >> 7` for the top bit only — which is why the
  two bit-7 buttons look like a different idiom from the other fourteen.  (The
  bit numbers and the destination offsets are measured; the NAMES in
  `VIConsolePad` are libpad's conventional mapping and are not evidence from
  this TU.  What the object code does settle is the source ORDER, which is not
  monotonic — left, down, right, up, then L1, R1, L2, R2 — and that is the only
  thing separating the pairs.)
* **The pad's dead zone is 88..167** of the 0..255 stick range, and the scale is
  the reciprocal 1/88 written out — gcc will not turn a divide into a multiply,
  so the constant in the object code is the source's.
* **`VIBattleMusic` has two entirely different back ends** behind feature bit
  0x100: a streamed `bgm\battle%02d.bgm` where **track 9 means "pick one of
  eight at random"**, or an XM parsed out of `bgm\battle%d.esf` and marked
  repeating. Only the second path uses the sound device's channel at all.

## Open

Two.

`VIPad::Read` at 6/290 — every instruction is right and one of them is in the
wrong place: the `move v0,zero` that materialises the teardown's return value
lands four instructions early, shifting five others. The whole tail block was
swept (every ordering that keeps the actuator pair adjacent) and all of them
score exactly 6, so the statement order is refuted as the cause. The standing
register-allocation/scheduling phenomenon.

`VIBattleMusic::Free` at 2/17 — the `-1` constant is in `v1` where the target
has `v0`. All six orderings of the three stores and both `int`/`void` return
types give exactly 2.
