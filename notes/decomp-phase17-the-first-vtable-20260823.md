# Decomp phase 17 — the sound resources, and the first VTABLE

**525/600 → 563/638.** `VISound` 6/6, `VIWave` 20/20 and `VIXm` 12/12 — the
whole family byte-exact on the settled toolchain with **no re-tuning**, 29 of
the 38 on the first build of their translation unit.

Everything matched before this either had no vtable at all or, like
`VICollSphere` in Phase 11, inherited without one. `VIWave` is the first class
in the corpus with `_vt$`, `__tf` and `__ti` symbols, so this phase settles the
polymorphic ABI — which is the gate on `VISprite`, `VIRaster`, `VIScene` and
essentially every class left in the engine.

```
VIResource                       VISound : VIResource            (abstract)
+0x00 unsigned m_id              +0x08 TYPE m_type   WAVE=0 XM=1
+0x04 int      m_link            +0x0C vptr

VIWave : VISound                          VIXm : VISound
+0x10 m_format          = 1               +0x10 VISoundDevice *m_device
+0x14 m_channelCount    clamp(n,1,2)      +0x14 m_xmAddr
+0x18 m_sampleSize      = 4               +0x18 m_spuAddr
+0x1C m_sampleFrameCount                  +0x1C +0x20  the cache pair
+0x20 m_sampleRate                        +0x24 VIXmHeader *m_xm
+0x24 m_loop                              +0x28 m_spuSize
+0x28 m_cached                            +0x2C void *m_sample
+0x2C float m_volume  +0x30 m_pan         +0x30 m_xmSize
+0x34 unsigned char *m_buffer             +0x34 m_repeat
+0x38 m_size          +0x3C m_spuAddr     +0x38 float m_volume  +0x3C m_pan
+0x44 +0x4C +0x50 +0x54                   
+0x58 VISoundDevice *m_device
```

## The vptr goes at the END of the class's own data

Not at offset 0. Measured directly with the settled compiler:

```cpp
class N { public: virtual ~N(); int a,b,c,m; };   // sizeof 20, m at 12, vptr 16
class D3 : public B3 /* 3 ints */ { virtual ~D3(); int m; };  // m at 12, vptr 16
```

So `VISound`'s vptr at `+0x0C` says the class carries exactly twelve bytes of
data ahead of it — an eight-byte non-polymorphic `VIResource` base plus its own
`m_type` — and it is `VISound`, not `VIResource`, that introduces the vtable.
It also says `VIWave`'s members can start at `+0x10` with **no second vptr**:
a derived class adds none.

Getting this backwards is the trap. Reading the vptr as offset 0 would have put
`VIResource` at +4 and shifted every field in both derived classes by four.

## The vtable, and how the slot order was read

A gcc 2.x vtable entry is **eight bytes** — `{short delta; short index; void
(*pfn)();}` — and there are no thunks, so a virtual call is five instructions:

```
lw   v0, 12(this)      ; vptr
lh   a0, 8N(v0)        ; delta
lw   v1, 8N+4(v0)      ; pfn
jalr v1
addu a0, this, a0      ; this + delta   (in the delay slot)
```

Entry 0 is the header and its `pfn` slot holds `__tf<class>`, so **the Nth
virtual declared sits at byte 8N**. `_vt$6VIWave`, `_vt$4VIXm` and
`_vt$7VISound` are each **64 bytes** in the image (the RTTI name strings start
exactly 64 bytes in) — and our build emits `.rodata` of exactly `0x40` for all
three, which is the independent check.

> **CORRECTED IN PHASE 19: 64 bytes is SIX virtuals, not seven.** A gcc 2.x
> vtable ends with a **NULL entry** — `class B { int a,b; virtual ~B();
> virtual int f1(); virtual int f2(); }` emits `.size _vt$1B,40`, i.e. header +
> three entries + a closing `.space 8` — so a vtable of B bytes holds
> `B/8 - 2` virtuals. `VISound::ChannelCount` is therefore **not virtual**.
> The table below is right about slots 1..6 and wrong about slot 7; every
> dispatch site in `VISoundDevice` reaches slots 1 through 6 and no further.
> The error was invisible because the slot is LAST, so nothing shifts and all
> 38 functions matched either way. See
> `decomp-phase19-the-sprite-hierarchy-20260823.md`.

**`VIXm::SetRepeat` is NOT virtual**, or the table would be longer. The slots
were read off real call sites:

| slot | function | evidence |
|---|---|---|
| 1 | `~VISound` | `VISoundDevice::Reset` / `ReleaseSound` |
| 2 | `Volume` | result feeds `mul.s` — a float |
| 3 | `SetVolume` | `$f12 = 1.0f` in `VIWave::Create` |
| 4 | `Pan` | result stored with `swc1` — a float |
| 5 | `SetPan` | `$f12 = 0.0f` in `VIWave::Create` |
| 6 | `IsCached` | `beqz v0` gate in `VISoundDevice::Play` |
| ~~7~~ | ~~`ChannelCount`~~ | refuted in Phase 19 — not virtual |

The scan that found them is worth keeping: look for `lw rB,12(rA)` followed
within a few instructions by `lh rt,imm(rB)`, and the `imm` is the slot. The
`+12` is what makes it VISound-family-specific.

None of the accessors has an out-of-line `VISound` body anywhere in the
image, and none can be inline (an inline one would leave a copy in the TU that
emits the vtable), so they are **pure virtual** and `VISound` is abstract. Our
build agrees: `visound.o`'s vtable carries `__pure_virtual` relocations for the
five that remained virtual after the Phase 19 correction.

## The RTTI name strings ARE the class hierarchy

`__tf<C>` is a guarded singleton builder: read a flag word, and if it is zero
call the BASE's `__tf` **with no arguments**, then the builder with
`(this, name, &__ti<base>)`. Two forms:

* three arguments (`__rttiSi`) — single inheritance, and `__ti<C>` is 16 bytes;
* two arguments (`__rttiUser`) — no base, and `__ti<C>` is 8 bytes.

The `name` arguments point into a run of consecutive strings in `.rodata`, so a
class's whole chain reads straight out of the image with no decompilation at
all. At 0x004EBF18: `6VIWave`, `7VISound`, `10VIResource`. At 0x004EBF90:
`4VIXm`, `7VISound`, `10VIResource`. That is where the base class came from.

**Every TU that needs a class's RTTI emits its own `__tf`.** The image holds
three copies each of `__tf7VISound` and `__tf10VIResource` — one per sound TU —
and the symbol map names one address for each, so the derived spans of
`AsXm__7VISound`, `__tf7VISound` and `__tf4VIXm` swallow whole neighbours.
Three new `SPAN_CAP` entries (48, 80, 80 against 192, 144, 224). Our build
emits fewer copies — it references the base's `__tf` as undefined — and it
costs nothing, because every copy is byte-identical. A linkage difference, not
codegen.

## `m_type` is set LAST, in each derived constructor's body

There is no `__7VISound` symbol in the image and no gap that could hold one:
`VISound` has **no user-declared constructor**. So the type is stamped by the
derived class, and the store order says where.

Reading a constructor's store order needs one rule, and it is cheaper than the
"hoist" the earlier phases described:

> **The vptr store lands after the block's FIRST member store, and everything
> else keeps source order.**

The reason is the R5900 scheduler, not any hoisting pass. The vtable address is
materialised `lui` + `addiu`, and the store that uses it needs one slot of
separation; the scheduler fills the gap with the first otherwise-ready store in
the block. `VIWave`'s constructor emits `[4] [12] [0] [52] [8]` — filler `[4]`,
vptr, then `[0] [52] [8]` — so the source is

```cpp
VIWave::VIWave()      // VIResource() runs first: m_id = m_link = 0;
{
    m_buffer = 0;
    m_type   = WAVE;  // AFTER the body's own store
}
```

`VIXm` is the same rule with the constant pulled up too: `li v1,1` covers its
own gap, `sw v1,8` covers the vptr's, and the rest — `[4] [0] [36] [44] [20]
[24] [52]` — is source order exactly.

The same mechanism explains `VIWave::Allocate`'s tail. `li v0,1` needs a slot
before `sw v0,84`, so `sw zero,68` fills it and the remaining zeros follow in
source order:

```cpp
m_p68 = 0;  m_p80 = 0;  m_p76 = 0;  m_p84 = 1;   // emits 68, 84, 80, 76
```

Note this is the **first** store of the repeated value that gets pulled up, not
the last — which is the opposite of how Phase 16 phrased the same phenomenon on
`VISurface::Deallocate`. Both readings fit their own block; the gap-filling
account fits every block in this phase and costs nothing to test, so Phase 13's
and Phase 16's blocks are worth re-reading through it.

`VIResource`'s constructor is a **chained assignment** — `m_id = m_link = 0;`,
+4 stored before +0 — which is Phase 13's `VIBuffer` finding holding for a
second, unrelated base class.

## A shipped bug: `VIWave::Deallocate` always fails

Every path returns −1:

```cpp
int VIWave::Deallocate()
{
    if (m_buffer)  VIMemFree(m_buffer);
    if (m_spuAddr) m_device->FreeSpuMem(m_spuAddr, m_size);
    m_buffer = 0;
    return -1;
}
```

and `VIWave::Destroy()` is `if (Deallocate() < 0) return -1; return 0;` — so
**`Destroy()` reports failure on every call**, on a wave that was freed
correctly. Matched byte for byte written that way. (`VIXm::Destroy` returns
void and has no such problem.)

## Levers re-used, and one contrast worth keeping

* **The success path goes in the `if`** (Phase 7). `LockBuffer`, `UnlockBuffer`
  and `VIXm::ChannelCount` all had the guard the wrong way round; gcc 2.95
  moves an `if`-body ending in `return` out of line, so the source's `if` holds
  the block that lands LAST.
* **`if (x) { … } return 0;` defeats the if-conversion** (Phase 13). This phase
  has both halves side by side: `VIWave::IsCached` is `return m_cached != 0;`
  and comes out as one `sltu`, while `VIXm::IsCached` tests two fields and the
  target BRANCHES — which only the nested form produces. The flat
  `if (!a) return 0; if (b) return 1; return 0;` if-converts to a `sltu` and is
  seven words wrong.
* **A fresh local for the intermediate** (Phase 14), in integer code again.
  `m_size = blocks * (m_channelCount * 16)` is reassociated to
  `(blocks * 16) * channels` and puts the `sll` on the wrong operand;
  `int bytes = m_channelCount * 16; m_size = blocks * bytes;` is byte-exact.

## Harness

* Three `SPAN_CAP` entries, all held by `test_span_caps_only_ever_shrink`
  (shrink only, land one word past a `jr ra`, leave another `jr ra` behind).
* `eqcheck.py` now scores a symbol **once** even when two objects define it,
  and reports a `DUPLICATE` row if the two definitions differ. gcc puts a
  class's `__tf` only in the TU that owns its vtable, so nothing triggers it
  today — but the image proves the other arrangement exists, and one target
  function scored twice would have inflated the total.
* `src/vi.h` had a stranded `#endif` at line 1324 with three phases of content
  after it. Moved to the end of the file.
