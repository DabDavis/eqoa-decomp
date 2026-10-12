# Decomp phase 33 — the lightning bolt and the motion trail

**1,517/1,671 → 1,550/1,707.**  Two whole translation units, 35 symbols, **33
byte-exact**, on the settled toolchain (`ee-gcc 2.95.3 -O2 -G0 -fno-exceptions`
+ SN `ps2eeas` 2.95.3-114) with no re-tuning.  25 of them were exact on the
FIRST build, and the TU's `_GLOBAL_$I$`/`_GLOBAL_$D$` pair came free with it.

| TU | functions | exact |
|---|---|---|
| `vitrailfx.cpp` — `VITrailFx` + 5 free functions + the static ctor pair | 16 | **16** |
| `vilightning.cpp` — `VIBoltHarmonic`, `VIBolt`, `VILightning` + 2 free | 19 | 17 |

Both are self-contained effect layers with a file-scope pool in front of them,
which is why they were picked: everything they call (`VIRaster`, `VIMaterial`,
`VILList`, `VIMatrix44`, `VIVect3`) was already settled by earlier phases, so
every miss is codegen and nothing is a guess about semantics.

---

## What the two layers are

### `VITrailFx` — the motion trail

A ring of fixed-size `SEGMENT`s recycled between two intrusive lists, and above
it a file-scope pool of **16** trails moving between three more:

```
static int        sMaterial = -1;
static VITrailFx *sTrails;              // new VITrailFx[16]
static VILList    sUnusedList;          // not in use
static VILList    sActiveList;          // still emitting
static VILList    sEndingList;          // ended, fading out
```

`VITrailFxBegin` takes one off `sUnusedList`, `VITrailFxEnd` moves it to
`sEndingList`, and `VITrailFxRender` walks both live lists once a frame: it
`Update`s each trail and draws the ones that still have segments, and a trail on
the ending list that reports empty goes back to `sUnusedList`.

```
class VITrailFx : public VILNode {           // 56
    struct SEGMENT : public VILNode {        // 76
        VIVect3    m_pos[2];                 // +0x10 +0x1C
        VIColor32F m_color[2];               // +0x28 +0x38
        float      m_age;                    // +0x48
    };
    long     m_lastTime;                     // +0x10   (8 bytes)
    float    m_life;                         // +0x18
    VILList  m_used;                         // +0x1C
    VILList  m_free;                         // +0x28
    SEGMENT *m_segments;                     // +0x34
};
```

A segment is one quad edge — two points and two colours.  `Render` draws the
used list **twice**, once forward (colour 0 / pos 0 then colour 1 / pos 1) and
once with the pair swapped, i.e. front face and back face, splitting the strip
every **24** vertices and repeating the current segment's two vertices to bridge
the seam.  The alpha is `1 - age/life`, clamped, written into BOTH of the
segment's colours (and into `m_color[0]` twice — once unclamped, then clamped).

### `VIBoltHarmonic` / `VIBolt` / `VILightning` — the bolt

A bolt's shape is the sum of **three harmonics**, each a ring of 17 random 2-D
offsets interpolated toward a second ring; when the phase reaches 1 the second
ring becomes the first and a new one is drawn, so the bolt writhes continuously
instead of snapping.  `VIBolt::Reset` creates them with

| harmonic | amplitude | rate |
|---|---|---|
| 0 | 0.57 | 0.64 |
| 1 | 0.15 | 10.24 |
| 2 | 0.08 | 44.0 |

— a coarse sway plus two grades of jitter.

```
class VIBoltHarmonic {                       // 428
    int     m_pad;                           // +0x000   never read
    VIVect3 m_samples[2][17];                // +0x004 +0x0D0
    int     m_count;                         // +0x19C   written, never read
    float   m_phase;                         // +0x1A0
    float   m_rate;                          // +0x1A4
    float   m_amplitude;                     // +0x1A8
};
class VIBolt {                               // 1552
    VIBoltHarmonic m_harmonics[3];           // +0x000
    VIVect3        m_points[17];             // +0x504
    VIMatrix44     m_matrix;                 // +0x5D0
};
class VILightning {                          // 8832
    struct BRANCH { int m_count; VIVect3 m_points[4][17]; };   // 820
    VIVect3    m_start;                      // +0x0000
    VIVect3    m_end;                        // +0x000C
    VIMatrix44 m_matrix;                     // +0x0020
    int        m_strikeTimer;                // +0x0060
    int        m_boltCount;                  // +0x0064
    VIBolt     m_bolts[4];                   // +0x0070
    VIColor32F m_color;                      // +0x18B0
    int        m_maxBranchCount;             // +0x18C0
    float      m_lastCharge;                 // +0x18C4
    float      m_charge;                     // +0x18C8
    int        m_ringTimer;                  // +0x18CC
    int        m_ringHead;                   // +0x18D0
    int        m_ringCount;                  // +0x18D4
    BRANCH     m_branches[3];                // +0x18D8
};
```

`8832` matches `spell-effect-system.md`'s measured allocation size exactly, and
the whole layout falls out of the constructor: the member-construction loops
count `3 × (4 + 2×204 + 16)` for the harmonics, `17 × 12` for the point ring,
`4 × 16` + a VU0 `Zero` for each `VIMatrix44`, and `3 × (4 + 4×204)` for the
branch history.

`VIBolt::Update` is the shape generator: seed the 17 points as a straight line
from a to b, then subdivide three times — 4 spans with harmonic 0, 8 spans with
harmonic 1, and every interior point with harmonic 2 — each sample rotated into
world space through the caller's matrix.  `VILightning::Update` counts a strike
timer down, and on expiry re-seeds the branches, picks a new interval of
`VIMAX(rand() % 351, 70)` ms and a new branch count of
`rand() % (maxBranchCount + 1) + 1`, quartering the interval when there is only
one bolt.  Branch `i > 0` starts at **point `3*i` of bolt 0**, so the branches
fork off the main bolt rather than off the endpoints.

`VILightning::Render` keeps a **3-frame ring** of the bolts' point arrays and
draws all of them, oldest faintest (`RenderBoltVertices(..., 3 - i)`), which is
the bolt's after-image.  Each call draws **four** ribbons — up, down, the
matrix-transformed right, and its negation — as two 9-point tri-strips with a
`0..8` U coordinate.

---

## Levers

### A pointer copy of the array decides which of three loop shapes gcc emits

`VIBoltHarmonic::GenerateSamples` writes 17 samples.  The target walks a
pointer forward and exits on a **signed** compare against an end pointer, with
no entry guard.  Three plausible spellings give three different shapes:

| source | shape |
|---|---|
| `for (i = 0; i < 17; i++) m_samples[which][i].x = …` | reversed to a countdown, `bgez`, negative displacements |
| `for (s = m_samples[which]; s < e; s++)` | forward, **`sltu`**, plus an entry guard |
| `VIVect3 *s = m_samples[which]; for (i = 0; i < 17; i++) s[i].x = …` | forward, **`slt`**, no guard — **the target** |

The third is phase 30's lever with the consequence made explicit: naming the
array through a pointer lets the `loop` pass build the induction variable out of
the COPY, so the exit test is rewritten onto the pointer while keeping the
signed comparison of the original `int` bound — and because the trip count is
still a compile-time constant there is no guard.  The bare subscript instead
hands `check_dbra_loop` a loop it can reverse, and the explicit end pointer
hands it one it cannot prove non-empty.

### A returned temporary's stack slot is reused by the next declaration

`VILightning::RenderBoltVertices` opens with

```cpp
r.Color(VIColor32F(…, …, …, 1.0f));
VIVect3 p;
VIVect3 off;
```

The `VIColor32F` temporary lives at `sp+0`; gcc frees temp slots at the end of
the statement, so `p` — declared AFTER the call — lands on the same 16 bytes.
Declaring `p` and `off` at the top of the function instead keeps the colour a
slot of its own, and the frame comes out 272 bytes where the target's is 288,
with every local at a different offset: **127 wrong words against 0.**  Moving
the two declarations below the call is the only difference between the two
versions.

The same slot sharing explains something that reads like a bug in the
disassembly: the `VIVect2` handed to `VIRaster::UV` and the `VIVect3` handed to
`VIRaster::Vertex` are BOTH at `sp+32`, alternating, through one address pseudo.
They are two temporaries of two consecutive statements, not one overlapping
object.

### Two `if`s or an `if`/`else` — the ORDER of the two copies

`VITrailFx::Update` calls `ClearSegmentsForward` from two places and the target
does not cross-jump them.  Written as one `if` with `||`, gcc emits one call:

```cpp
if (p->m_prev == 0 || m_life <= p->m_age) { ClearSegmentsForward(s); break; }   /* 6 wrong */
```

Written as two `if`s it emits two calls but puts the null test's body FIRST:

```cpp
if (p->m_prev == 0) { … break; }
if (m_life <= p->m_age) { … break; }                                           /* still wrong */
```

Only the `if`/`else`, with the null test as the ELSE, puts the age test's block
inline and the null test's out of line where the target has them:

```cpp
if (p->m_prev) {
    if (m_life <= p->m_age) { ClearSegmentsForward(s); break; }
} else {
    ClearSegmentsForward(s);
    break;
}                                                                              /* byte-exact */
```

### `VIMIN`'s argument order, in the direction phase 23 did not measure

`VIMIN(3, x)` gives `slti x,4` + `movn` (the bound is the default, x moves in
when it is smaller); `VIMIN(x, 3)` gives `slti x,3` + `movz`.  `VILightning::
Render`'s ring-count clamp is the first form.  Same for `VIMAX(x, 70)` in
`::Update`, which comes out as `li 69; slt 69,x; movn` — the constant is
DECREMENTED because gcc canonicalises `x > 70` to `69 < x`, so a `69` in the
object code does not mean a 69 in the source.

### A macro with a call inside runs the call twice — again

`VIMAX(rand() % 351, 70)` emits **two** `rand()` calls.  The target has one, so
the source binds a local first.  Phase 31 found this with `VIMIN(1, VIMAX(-1,
d.Magnitude()))`; here the cost is not three calls but a wrong random stream.

### Statement order inside an `if` body decided a branch-likely

`VILightning::Render`'s ring advance is three assignments.  With
`m_ringHead = (m_ringHead + 1) % 3;` written LAST the function is 27 words
wrong; written FIRST it is **one** word wrong (a stray `move v0,zero`, i.e. the
function returns void).  The load of `m_ringHead` at the join can then no longer
be hoisted above the branch, so the delay slot is filled from the target and the
branch becomes `bnezl` — one extra instruction, and every offset after it.

### Where a value is computed relative to a CALL decides its register class

`VIBolt::Update` computes `start`/`end` per outer iteration and then calls
`Sample` and `VIMatrix44::Mul`.  With the two assignments written after the
calls they are dead across them and land in caller-saved registers; written
BEFORE the calls they are live across them, go callee-saved, and the pressure
pushes the `VIMatrix44 &` parameter out to a stack slot — which is exactly what
the target does (`sw t0,64(sp)` at entry, `lw a0,64(sp)` before each `Mul`).
**76 wrong words to 35**, on moving two lines up.

This is the same class of fact as phase 29's "where a global is READ is a source
fact": a call is a barrier, and which side of it an expression sits on is
visible in the object code.

---

## Shipped bugs

* **`VITrailFx::ClearSegmentsForward` retires at most TWO segments.**  It walks
  with a cursor but re-reads `m_next` through the ARGUMENT every iteration:

  ```cpp
  SEGMENT *n = s;
  do {
      SEGMENT *next = (SEGMENT *)s->m_next;   /* s, not n */
      n->Remove();
      m_free.AddTail(n);
      n = next;
  } while (n->m_next);
  ```

  After the first iteration `s` is already on the free list, so its `m_next`
  points at that list's tail sentinel, whose own `m_next` is 0 and the loop
  stops.  Matched byte for byte written that way.  `AddSegment` hides it by
  calling in a `while` until `RemHead` succeeds; `Update` does not, so a trail
  whose tail expires in a burst sheds it two segments a frame.

* **`VITrailFx::Render` writes the clamped alpha into `m_color[0]` twice** and
  the unclamped value once before that, so a segment older than its life span is
  briefly stored with a negative alpha.

* **`VIBoltHarmonic::Create`'s `count` argument is stored and never read.**  All
  three harmonics are created with 3 and the sample count is the hardcoded 17
  everywhere.  `+0x000` of the class is never touched at all.

* **`VILightning`'s fourth ribbon is the negation of the third**, computed by
  negating `off` in place — which only works because `off` is NOT re-zeroed per
  iteration (the loop head is after the two constructors).  Cases 0 and 1 write
  `off` outright, case 2 replaces it with the matrix-transformed
  `(0.04, 0, 0)`, and case 3 negates whatever case 2 left.  Reorder the cases
  and two of the four ribbons vanish.

---

## The two open functions

`VIBolt::Update` 35/294 and `VILightning::SetLocation` 42/111 (7 positions
register-blind) are both the standing phenomenon: identical instruction
sequences, different register numbers and a two-instruction scheduling swap.
Six spellings of the delta, four declaration orders and three forms of the
`start`/`end` pair were swept for the first; six statement orders and four
spellings of the cross product for the second.  Neither moves.

---

## Harness

* **`SPAN_CAP["ClearSegmentsForward__9VITrailFxPQ29VITrailFx7SEGMENT"] = 104`.**
  vitrailfx.cpp's three file-scope `VILList`s give the TU a static constructor,
  and gcc's unnamed `__sti` routine for it sits between the last named function
  and `_GLOBAL_$I$` — which calls it at `+0x68`, naming its own start.

* **`eqcalls.py` now reads STRING LITERALS out of the dump.**  A `lui`+`addiu`
  pair against a string holds only the addend in the un-relocated image, and
  `name_at` could only report the nearest preceding symbol (`_vt$9VIHSprite
  +0x90`).  `string_at` reads the bytes at the resolved address instead, so

  ```
  $ ./eqcalls.py calls VILightningFxInit__FR8VIRasterR12VIDictionary
    0x0044e208  addr  "lightning.tga"
    0x0044e218  jal   VIHashResourceID
  ```

  which is how the material's texture name was recovered rather than guessed.
  `./eqcalls.py at <addr>` does the same for a bare address.
