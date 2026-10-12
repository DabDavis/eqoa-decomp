# Phase 32 — the remaining sprite leaves

**1464/1610 -> 1517/1671.**  The four `VISprite` leaves phases 19/20 did not
reach: `VINameSprite` 17 written / 16 exact, `VISoundSprite` 15 / 13,
`VIStreamAudioSprite` 16 / 14, `VIEffectVolumeSprite` 12 / 10 — 53 of 60 on the
settled toolchain (`ee-gcc 2.95.3 -O2 -G0 -fno-exceptions`, SN `ps2eeas`
2.95.3-114) with no re-tuning, and **41 of them byte-exact on the first build**.

The hierarchy above them was settled in phase 19 and every base class these
touch (`VIBBox`, `VISphere`, `VIColor32F`, `VIMatrix44`, `VIRect`, `VIString`,
`VIMultiMap`) was settled earlier still, so almost every miss here was codegen
and the few that were not are recorded as layout facts below.

---

## The vtable is 96 bytes in all four, again

`_vt$12VINameSprite` 128, `_vt$20VIEffectVolumeSprite` 136,
`_vt$19VIStreamAudioSprite` 152, `_vt$13VISoundSprite` **520** — and read out of
the relocated dump every one of them is the same ten virtuals, a NULL entry at
+88, and the RTTI name string starting at byte 96 exactly.  VISoundSprite's 520
is the name string plus a neighbouring unnamed table.  Phase 19's `B/8 - 2` rule
holds a fifth, sixth, seventh and eighth time, and **no leaf in the engine adds a
virtual**.

Three of the four override `GetInstType` (all `return 1`); only
`VIStreamAudioSprite` overrides `QueryEmitters`; none overrides
`QueryIntersection`.

## Four new `SPAN_CAP` entries

The same twelve unnamed template twins close every sprite TU (phase 20), so the
last named symbol in each of these four acquires a two-to-seven kilobyte span:

| symbol | cap | derived |
|---|---|---|
| `RasterDeferred__12VINameSpriteR8VIRaster` | 540 | 3328 |
| `EnablePan__13VISoundSpriteb` | 8 | 2520 |
| `ClampScale__19VIStreamAudioSpriteR10VIMatrix44` | 492 | 6936 |
| `Release__20VIEffectVolumeSprite…` | 88 | 2600 |

`EnablePan` is the extreme case: a two-instruction setter with a 2,520-byte
derived span.

---

## Levers

### `int k = 32; while (k--) *d++ = *s++;` is not any `for` loop

`VINameSprite::RasterDeferred` compacts its 24-entry table in place, and the
32-byte name field is copied by a hand-written byte loop:

```
lbu   v0,0(v1)
addiu a1,a1,-1
addiu v1,v1,1
sb    v0,0(a0)
bne   a1,t0,loop      ; t0 == -1
addiu a0,a0,1
```

Three spellings were measured against it:

| source | result |
|---|---|
| `for (k = 0; k < 32; k++) d->m_name[k] = s->m_name[k];` | 92 wrong (no strength reduction at all) |
| `for (k = 0; k < 32; k++) *dn++ = *sn++;` | 12 wrong (`bgez`, counter to 0) |
| `for (k = 31; k >= 0; k--) *dn++ = *sn++;` | 12 wrong (same) |
| **`int k = 32; while (k--) *dn++ = *sn++;`** | **byte-exact** |

gcc's `check_dbra_loop` reverses the first two into a countdown that exits on
`bgez`; only a source that *already* counts down through zero produces the
`bne` against the final value −1.  This is the counterpart to phase 30's
"let the `loop` pass make the walking pointer": there the subscript was right,
here the explicit `*p++` is, and the difference is which of the two the loop
pass is allowed to build the induction variable from.

### A record copy goes through POINTERS, not subscripts

Written as `NameDefers[keep].m_z = NameDefers[i].m_z;` (and so on for six
fields), gcc re-does the `mult`/`madd` index arithmetic per field.  Written as

```cpp
VINameDefer *d = &NameDefers[keep];
VINameDefer *s = &NameDefers[i];
d->m_z = s->m_z;
```

every field is a displacement off one base register, which is what the target
has.  Phase 15's layer-pointer rule at a new site — and here it is worth 71 of
92 wrong words on its own.

### An inline member's `this` is the address pseudo a named field is not

`VINameSprite::Raster` builds a `VIVect3` anchor early and passes it to
`VIRaster::CalcCenteredStringScreenRect` two calls later.  The target parks
`&at` in **s7** — a callee-saved register the function otherwise would not need
(the frame is 16 bytes larger for it).

* `at.x = 0.0f; at.y = 0.6f; at.z = 0.0f;` — every store on `sp`, no pseudo, no
  s7, `addiu a2,sp,16` at the call site.  42 wrong.
* `at.Set(0.0f, 0.6f, 0.0f);` — the inlined member takes `this`, that address
  becomes a pseudo, its live range crosses two calls, and it goes callee-saved.
  **14 wrong.**

This is phase 9's pointer-variable lever reached through an inline member rather
than a named variable, and it is the exact inverse of phase 31's "an address
NAMED before a call cannot be coalesced": there the reference was a mistake,
here it is required.  `VIVect3::Set(float,float,float)` was added to `vi.h` for
it.

### `float d[3]` is not `VIVect3 d`

`VIEffectVolumeSprite::Raster` computes three size deltas into a stack object it
then never reads — the compares use the registers.  A `VIVect3` carries a
zeroing constructor whose three stores gcc will **not** delete, because the two
`VIBBox::Size()` calls sit between them and the assignments and a call
invalidates cse's memory table (phase 27).  The target has no zeroing at all, so
the object has no constructor: a plain `float d[3]`, which gcc still forces to
memory and still never deletes the dead stores to.  106 wrong -> 37.

### Phase 31's braced blocks, at a second site

The same function rebuilds a transposed matrix's translation twice, in two
textually identical blocks.  Sharing three function-scope `float x, y, z` across
both renames every FP register in both (37 wrong); giving each block its own
braced scope makes the temporaries distinct pseudos and both blocks go
byte-exact.  **37 -> 3.**

### A `void` return frees `v0`

`VIEffectVolumeSprite::Release` calls `m_volume.Free()` and then loads `m_id`.
With `Free()` declared `int`, gcc puts the `m_id` load in `v1` and the function
is two words wrong; with `Free()` declared **`void`** it takes `v0` and the
function is byte-exact.  Nothing else in the function changes.  A new lever, and
a cheap one to try whenever a load right after an ignored call lands in the
wrong return-class register.

### The if-direction is per function, a fifth time

* `VIStreamAudioSprite::Copy` wants the plain guard `if (i < 0) return -1;`
  (44 wrong the other way).
* `VISoundSprite::Pick` wants the success path in the `if`
  (`if (x != -1) { … return …; } return 0;`), which is the opposite of
  `VINameSprite::Copy`'s.

### Phase 14's chained float zero, shared across two blocks

`q.m[0].w = q.m[1].w = q.m[2].w = 0.0f;` emits one `mtc1 zero,$f20` and three
`swc1 $f20` — and because `$f20` is callee-saved the single materialisation is
reused by the *second* block as well.  Three separate statements give `sw $0`
six times.  The store order is 2, 1, 0, phase 7's chained-assignment shape.

### `_GLOBAL_$I$` is named after the first global OBJECT

The map has `_GLOBAL_$I$_12VINameSprite$NumNameDefers` and
`_GLOBAL_$D$_…$NumNameDefers`.  Written as `int VINameSprite::NumNameDefers;`
the variable goes to `.comm`, never claims `first_global_object_name`, and gcc
names the pair after `NameDefers` instead — two symbols that then do not exist
in the target at all.  Written `= 0` it is a real definition, lands ahead of
`NameDefers` in `.bss` (which is also the order the map has them in, 0x4E2460
before 0x4E2470), and both functions match.

---

## Layouts

### `VINameSprite` — a TWO-PHASE sprite

```
VISprite               +0x00 .. 0x3F
int        m_font      +0x40
char       m_name[32]  +0x44          strncpy(…, 31) then m_name[31] = 0
VIColor32F m_color     +0x64
                                      size 0x74
static int         NumNameDefers;
static VINameDefer NameDefers[24];    24 x 144 = 3456
```

```
class VINameDefer                     // 144 bytes
{
    float      m_z;        // +0x00   view-space depth, the qsort key
    int        m_font;     // +0x04
    char       m_name[32]; // +0x08
    VIColor32F m_color;    // +0x28
    VIRect     m_rect;     // +0x38   (+0x48..0x4F is the matrix's alignment)
    VIMatrix44 m_mat;      // +0x50
};
```

`::Raster` does not draw.  It transforms `(0, 0.6, 0, 1)` by the model-view,
rejects the plate on distance, and **enqueues** a record — depth, font, name,
colour, model-view and the screen rectangle the string will occupy.
`::RasterDeferred`, a **static** taking only a `VIRaster &`, runs once at the end
of the frame: `qsort` by depth, then a compaction pass that keeps a plate only if
its rectangle clears every plate already kept, then one
`SetModelView` + `DrawCenteredString` per survivor, then `NumNameDefers = 0`.
So overlapping name plates are resolved by *dropping* the farther one, and the
24 cap is silent.

Two shipped oddities:

* **The off-distance test is not a distance.**  It compares `m_offDist²`
  against `x² + y² + z² + w²` of the *homogeneous* view-space anchor — the `w`
  term is in the sum, so `Clear()`'s 22.0f is not 22 units of anything.
* `Clear()` sets `m_type = 6` and `m_offDist = 22.0f`, and is called from both
  the constructor and the destructor.

### `VISoundSprite`

```
VISprite                        +0x00
int             m_sound         +0x40   -1
int             m_channel       +0x44   -1
int             m_flags         +0x48   0x10000000
ATTENUATIONMODE m_attenuation   +0x4C   0
bool            m_pan           +0x50   true   (four bytes — phase 28)
float           m_volume        +0x54   1.0f
                                        m_type 9
```

`VIPointSprite::ClampScale` is **written out inline** in both `::Raster` and
`::Pick` here — the same nine multiplies twice — where `VIStreamAudioSprite`
keeps its copy out of line.

Shipped bug: `::Release` calls `sd->IsPlaying(m_channel)` and `sd->Stop(...)`
**before** it null-tests `sd`; only `ReleaseSound` is guarded.  Releasing a sound
sprite with a live channel and no device calls through a null pointer.

### `VIStreamAudioSprite`

```
VISprite            +0x00
bool m_looping      +0x40   true
int  m_priority     +0x44   0
char m_pathname[128] +0x48                m_type 12, size 0xC8
```

`::QueryEmitters` is the only `QueryEmitters` override in the corpus with a
body worth reading:

```cpp
if (q.m_type == m_type) {
    VIVect3 p;  VIEmitterEntry e;  VIMatrix44 inv;  float k;
    inv.InvertAffine(m);
    p = inv's translation row;
    if (m_bbox.Contains(p)) {
        switch (m_priority) {
        case 0: k = 1.0f; break;
        case 1: k = 2.0f; break;
        case 2: k = 3.0f; break;
        default: return -1;              // and it destroys both matrices first
        }
        p -= centre of m_bbox;
        e.m_a = q.m_a;  e.m_b = q.m_b;
        k -= p.Magnitude() * 0.0001f;
        sceVu0CopyMatrix(e.m_matrix, m);
        q.m_map.Insert(k, e);
        if (q.m_limit < q.m_map.Size())
            q.m_map.Erase(q.m_map.Begin());
    }
}
return 0;
```

Two things fall out of it.  **A priority outside 0..2 is not clamped — the
function returns −1.**  And the eviction is `Begin()`, the **smallest** key, so
the map is ascending and a higher priority sorts later; `visprite.cpp`'s
still-open `VISprite::QueryEmitters` guesses `Last()` there and should be
re-checked against this.

### `VIEffectVolumeSprite`

```
VISprite                        +0x00
VIBBox         m_localBBox      +0x40   the box m_volume was last built for
VIEffectVolume m_volume         +0x58   a MEMBER (its ctor takes this+88)
                                        m_type 14
```

`::Init` sets `m_bbox`, then `m_bsphere.Init(m_bbox)`, then `m_localBBox`, then
`m_volume.Init(b)`.  `::Raster` compares `m_bbox.Size()` against
`m_localBBox.Size()` and re-seeds the volume (`m_localBBox = m_bbox;
m_volume.OnResize(m_localBBox)`) as soon as any axis has drifted more than
**one unit** in either direction — a six-term `||`.  Bit 1 of the raster flags
then draws the effect in the camera's own space: the camera's `m_view` is
transposed, its translation rebuilt by hand, composed with the model-view,
transposed again, and handed to `m_volume.Render(cam, q)` after
`m_volume.Process(1.0f / 30.0f)` — **a hardcoded 30 fps step**, not a measured
`dt`.

Shipped oddities:

* **Its debug marker is the STREAM AUDIO one.**  Both `::Raster` and `::Pick`
  call `VIScene::GetStreamAudioUISprite()`, not a marker of their own — read out
  of the relocated dump by `eqcalls.py`, so it is the linker's word and not a
  guess.
* `::Copy` writes the volume's density **twice**: once through
  `SetType(Type(), Density())` and again as a direct field copy past
  `SetFlags`.
* `VIEffectVolume::Free()` returns **void** (see the lever above), and
  `::Release` calls it before the dictionary removal, unconditionally.

`VIEffectVolume` itself is not decompiled here; only the surface this class
needs.  Its field offsets are `effect-volume-system.md`'s, and the four this
class touches by name (+0x28 density, +0x30 view range, +0x34/+0x38 the
time-of-day pair) are exactly the four `::Copy` copies past the setters.

---

## Open — seven functions, and two of them are already-known phenomena

| function | wrong | why |
|---|---|---|
| `VIStreamAudioSprite::QueryEmitters` | 62/170 | phase 19's **`gcse` merge**: the four `addiu sp,32` the target rematerialises become one callee-saved pseudo, and the frame grows by a register.  `VISprite::QueryEmitters` has been open on the same merge since phase 19 — **both** `QueryEmitters` in the corpus, and nothing else, fail this way. |
| `VISoundSprite::Raster` | 1 insn | the target keeps `s.z` in an FPR across both earlier clamp blocks; ours reloads it from the stack because `$f0` was reused for the `4.0f` constant.  Pure FP allocation — `VIStreamAudioSprite::ClampScale`, the same body out of line, is byte-exact and allocates the way ours does. |
| `VISoundSprite::Pick` | 1 insn | the same one. |
| `VIStreamAudioSprite::Raster` | 6/62 | **identical, word for word, to `VIPointSprite::Raster`**, open since phase 20: the four argument moves before `RasterUISprite` come out in argument order in the target and dependency order in ours.  One phenomenon, two functions. |
| `VINameSprite::Raster` | 14/130 | one instruction.  The target computes `&NameDefers[n].m_mat` **after** the `GetModelView()` call and can put it straight in `a0`; ours computes it before and needs a `move`.  Nine spellings of the argument (pointer form, `[0]`, a cast, a named reference, a nested scope) all reproduce ours. |
| `VIEffectVolumeSprite::Raster` | 3/295 | the target rematerialises `addiu a0,sp,48` and `addiu a0,sp,112` at the first `sceVu0CopyMatrix` after each constructor where ours uses the pseudo it already has — the `gcse` merge again, at two sites. |
| `VIEffectVolumeSprite::Copy` | 2/77 | the last call's `a0`/`a1` setup is ordered the other way round. |

Everything else in the four classes is byte-exact.
