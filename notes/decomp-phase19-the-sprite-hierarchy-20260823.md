# Decomp phase 19 — the sprite hierarchy, and the vtable's own size

**584/666 → 613/697.** `VISprite` 15/16 and `VISimpleSprite` 13/13, on the
settled toolchain (`ee-gcc2.95.3-107 -O2 -G0 -fno-exceptions`, SN `ps2eeas`
from the `2.95.3-114` package) with **no re-tuning**. Twenty-seven of the
twenty-eight went byte-exact first try; the one open function is the standing
phenomenon with its pass named.

The base of the scene's renderable leaf hierarchy — and the class that settles
the polymorphic ABI at a depth Phase 17 could not reach, because `VISound`'s
three classes all had the same vtable length and this one has a derived class
that overrides seven of ten slots.

## The lever: a gcc 2.x vtable ends with a NULL entry

Measured on the settled toolchain, not inferred:

```c++
class B { public: int a,b; virtual ~B(); virtual int f1(); virtual int f2(); };
```

emits `.size _vt$1B,40` — one 8-byte header (`{0, 0, __tf1B}`), three 8-byte
entries, and a closing `.space 8`. So

> **a vtable of B bytes holds `B/8 - 2` virtuals.**

`_vt$8VISprite` runs 0x004EBAE0..0x004EBB40 — the RTTI name string
`"8VISprite"` starts exactly there — i.e. 96 bytes, i.e. **ten**. That is the
whole declaration, because ten is exactly what the class has bodies for once
the four accessors emitted after `__tf8VISprite` are set aside:

|slot|virtual|
|--|--|
|1|`~VISprite`|
|2|`GetInstType`|
|3|`Raster`|
|4|`Collide(VICollSphere&, …)`|
|5|`Collide(VICollRay&, …)`|
|6|`Pick`|
|7|`QueryEmitters`|
|8|`QueryIntersection`|
|9|`Copy`|
|10|`Release`|

**The console agrees independently.** `go-eqoa-vi/vicoll` reads the live
dispatch table at `sprite+0x3C` and calls the swept sphere through **entry 4**
and the ray through **entry 5** (`vtSphereFn = 0x24`), which is where this
order puts them — two oracles that have never seen each other's evidence.

## The correction it forces on Phase 17

`_vt$7VISound`, `_vt$6VIWave` and `_vt$4VIXm` are all **64** bytes, so they hold
**six** virtuals, not the seven `src/vi.h` declared. Phase 17 counted the
entries and forgot the terminator.

Which one is not virtual is settled by the dispatch sites. **The pfn is at
`8N+4`** — the `lh` at `8N` is the entry's this-delta — and every virtual call
into a `VISound` from `VISoundDevice` looks like

```
lw   v0,12(s1)      ; the vptr, VISound's is at +0x0C
lh   a0,48(v0)      ; entry 6's delta
lw   v1,52(v0)      ; entry 6's pfn
jalr v1
```

Collected over the whole class, the offsets used are 12, 20, 28, 36, 44 and 52
— slots **1 through 6 and no further**:

|slot|offset|caller|what it is|
|--|--|--|--|
|1|12|`ReleaseSound`, `CreateSound`, `Reset` (with `a1 = 3`)|`~VISound`|
|2|20|`Play`, `SetVolume`, `SetPan`, `Copy`|`Volume`|
|3|28|`Create`, with `$f12 = 1.0f`|`SetVolume`|
|4|36|`Play`, result stored with `swc1`|`Pan`|
|5|44|`Create`, with `$f12 = 0.0f`|`SetPan`|
|6|52|`Play`, `beqz v0` gate|`IsCached`|

So **`VISound::ChannelCount` is NOT virtual**. Making it (and its two
overrides) ordinary members takes all three vtables to 64 bytes and leaves the
38 matched functions untouched — it is last in the slot order, so nothing
shifts, which is exactly why the error was invisible for a phase.

## The layout, read off the constructor

```
+0x00  VIResource   m_id / m_link          (the base's inline zeroing ctor)
+0x08  VIBBox       m_bbox                 GetBBox returns a0+8
+0x20  VISphere     m_bsphere              GetBSphere returns a0+32
+0x30  float        m_offDist = 500.0f     GetOffDist
+0x34  int          m_type    = 0          GetType; go-eqoa-vi's VISpriteType
+0x38  int          m_actor   = -1
+0x3C  vptr                                 -- the END of the class's own data
```

Size 0x40. `VISimpleSprite` adds `m_primBuffer` +0x40, `m_objIdx` +0x44 (the
index into the collider's buffer pool, which `go-eqoa-vi` already calls
`ObjIdx`) and `m_materialPal` +0x48, and **adds no virtuals** — its vtable is
96 bytes too. `VISimpleSprite::Copy` carries every field across **except
+0x38**.

**`VISphere` needs a zeroing default constructor.** The sprite's constructor
zeroes the bounding sphere twice, exactly the way Phase 13 found `VIBBox` does:
the member `VIVect3`'s own constructor, then the body inlined. Five wrong words
became zero on `VISphere() { c.Zero(); r = 0.0f; }`, and nothing regressed.

## Three more levers

**A derived class's `__tf` inlines the base's when they share a TU.** With
`VISprite` and `VISimpleSprite` in one file, `__tf14VISimpleSprite` came out 34
instructions against the target's 20 — it builds the whole chain rather than
calling `__tf8VISprite`. They are two TUs in the image (`__tf8VISprite` at
0x0047ED10 after VISprite's members, `__tf14VISimpleSprite` at 0x00472C18 after
VISimpleSprite's), and splitting the file matched it byte for byte. This is the
cheapest available test for a TU boundary in a class hierarchy.

**`SetMaterialPal`'s `VIDictionary &` is DISCARDED** — Phase 16's lever, in the
form that is hardest to see. Both calls it makes take the dictionary's register
*by accident*: `a2` still holds the incoming argument and is never re-set, so
the two-argument reading and the three-argument reading produce the same first
call. The tell is the second call, where a three-argument source has to move it
back into `a2` and the target does not. 17 wrong words to 0.

**A local whose address is taken by a `const &` parameter is stored exactly
once.** `VISprite::QueryEmitters` computes a sort key and hands it to
`VIMultiMap<float, VIEmitterEntry>::Insert(const float &, const VIEmitterEntry &)`.
Written as one variable assigned twice, gcc spills the intermediate to the same
slot as well; split into a non-address-taken `t` and an address-taken `k`, `t`
lives in `$f20` across the second `sqrtf` and `k` is stored once, straight into
the slot `Insert` is handed. Computing `t` **before** `d` is what puts
`m_bsphere.r`'s load inside the second dot product and the multiply in the NaN
retest's delay slot.

## Two more `SPAN_CAP` entries

The sprite TUs carry their own template instantiations —
`VIMultiMap<float, VIEmitterEntry>`'s red-black tree (0x0047E198..0x0047ED10)
and the material-palette walkers — and the symbol map names none of the ones in
the gaps. So `Release__8VISprite`, a **two-instruction stub**, acquires a
3512-byte derived span, and `SetMaterialPalDither__14VISimpleSprite`, 33
instructions, acquires 2648. Capped at 8 and 132; the canary's shrink-only /
lands-one-word-after-a-`jr ra` / another-`jr ra`-beyond rules all hold.

## The one open function

`QueryEmitters`, 32/112. Everything about it is settled: the three locals and
their stack offsets (`p` at sp+0, `e` at sp+16, `mat` at sp+32 — VIMatrix44 is
`aligned(16)` — and the key at sp+96), the double zeroing of the matrix, the
guard `q.m_type == m_type`, both inline `sqrtf`s with the NaN retest, the
reciprocal (`t * (1.0f / d)`, not `t / d`), the entry's two words, the
`Invert`, the `Insert`, and the `Size()`/`Erase(Last())` trim.

What is left is the standing phenomenon, and this time the pass has a name:
**`gcse` merges the four `addiu a0,sp,32` into one pseudo**, which then needs a
callee-saved register and a 208-byte frame where the target rematerialises the
address at every use and gets by with 192. `-fno-gcse` takes the function to
18/110 — and costs the tree sixteen functions (613 → 597), so it is not the
build flag. The demonstration is cheaper than the flag: declaring `mat` FIRST
puts it at sp+0, where its address is a bare `move a0,sp` with nothing to
merge, and the register allocation then matches the target instruction for
instruction at the target's own length of 109 words — with every local at the
wrong offset.

## Not started

`VISkinSprite` (11), and the eight red-black-tree members of
`VIMultiMap<float, VIEmitterEntry>` that share the VISprite TU.
