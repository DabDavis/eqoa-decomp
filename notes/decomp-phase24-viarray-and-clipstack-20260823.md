# Decomp phase 24 — `VIArray<T>`, the second template, and `VIClipStack`

**750/848 → 883/985.** `VIArray<T>` 131 of 132 across twenty-two instantiations,
and `VIClipStack` 7 of the 10 written, on the settled toolchain
(`ee-gcc 2.95.3-107 -O2 -G0 -fno-exceptions`, assembled with SN `ps2eeas` out of
the `2.95.3-114` package) with no re-tuning.

`VIArray` is the plainest container in the engine and the second template after
`VIPool` (phase 12): two words, a heap block, and no growth path.

```c++
template <class T> class VIArray
{
public:
    int m_size;     // +0x00
    T  *m_data;     // +0x04
    ...
};
```

`VIZoneRadialFlora`'s field map in `claude-notes/radial-flora-system.md` — read
years before this phase, off the *callers* — already had three VIArrays at +0,
+8 and +0x10 with `m_data` at +4 of each. That is the same eight bytes, said
independently.

Six members: the constructor, the destructor, `Init(int)`, `Allocate()`,
`Clear()` and `DestroyPool()`, plus one `Init(const VIArray<T> &)` instantiated
for `VIHSpriteTrigger` alone. Twenty-two element types, so one source answers
for 132 symbols.

## The allocation is a RAW char block, and the elements are placed into it

The tell is what is *missing*. `m_data = new T[m_size]` makes gcc emit its own
vec-init constructor loop — a countdown over a walking pointer, no reload of
either member, no null test — and the target has no such loop for any of the
twenty-two. What it has instead is a *count-up* loop that reloads `m_data` and
`m_size` on every iteration and null-tests the element address. That is a
source-level loop over placement new:

```c++
template <class T> int VIArray<T>::Allocate()
{
    if (m_size == 0)
        return 0;
    m_data = (T *)new char[m_size * sizeof(T)];
    if (m_data == 0)
        return -1;
    for (int i = 0; i < m_size; i++)
    {
        T *p = (T *)(i * sizeof(T) + (unsigned int)m_data);
        if (p)
            new (p) T();
    }
    return 0;
}
```

`Clear()` and the destructor are the same decision the other way round:
`delete [] (char *)m_data`, not `delete [] m_data`. With the typed spelling a
`T` that has a destructor drags gcc's array-cookie destroy loop into `Clear`,
and `VIArray<VIZoneRoom>::Clear` grows from 24 instructions to 40. The engine
destroys its elements itself, in `DestroyPool`, and frees the block as bytes.

The null test is written out: placement new does not generate one.

## The five nops in an empty loop are the ASSEMBLER

`VIArray<int>::Allocate` ends with a loop whose body is five `nop`s:

```
    addiu   a0,a0,-1
    nop                     <- .p2align 3
L:  nop
    nop
    nop
    nop
    nop
    bnezl   a0,L
    addiu   a0,a0,-1
```

Nothing in the source or in gcc puts them there. `ps2eeas` pads a short loop
that ends in a branch-likely — the R5900 short-loop erratum — and it does it to
*our* object too: the candidate built from a genuinely empty loop body is
byte-identical, nops included. Ten of the twenty-two instantiations have this
shape, and all ten match.

That also settles why the loop survives at all. gcc 2.95 deletes
`for (i = 0; i < n; i++) ;` outright — measured — but keeps this one, because
the address computation and its null test are uses of `i`. Once the body is
empty the index is dead and the loop reverses to a countdown, which is why the
`int` form counts down while the `VIVect3` form counts up.

## The lever: `i * sizeof(T) + (unsigned int)m_data`, offset FIRST

Every spelling that keeps the pointer as a pointer —

```c++
    &m_data[i]
    m_data + i
    (T *)((char *)m_data + i * sizeof(T))
    (T *)(i * sizeof(T) + (char *)m_data)
```

— comes out `addu p, base, offset`, and the target is `addu p, offset, base`.
Pointer arithmetic canonicalises with the pointer first however it is written;
an integer add does not. Casting the pointer to `unsigned int` and putting the
offset on the left fixed `VIVect3`, `VIPlane`, `VIHSpriteTrigger`, `VIZoneNode`
and `VIWorldNode` in one edit, with no regression.

It degenerates at `sizeof(T) == 1`: with the multiply gone the two operands are
an index and a base and gcc canonicalises again. `VIArray<VIWorldRegion>::Allocate`
is the one open function in the template, by that single `addu`.

## The lever: `DestroyPool`'s index is a COPY, and `Next()` runs BEFORE the body

`DestroyPool` is `VIPool`'s Begin/End/Next walk again — `End()` is `0x7FFFFFFF`,
`Begin()` is `End()` on an empty array — with the element destructor in the
body. All three are inline in the class body: no `Begin`/`End`/`Next` symbol
exists for any instantiation, where `VIPool`'s three are emitted out of line.

Written the obvious way,

```c++
    for (int i = Begin(); i != End(); i = Next(i))
        m_data[i].~T();
```

gcc strength-reduces `i * sizeof(T)` into an induction variable and hoists the
multiply out of the loop. The target multiplies inside the loop, every
iteration, and destroys the element the index held *before* `Next` advanced it:

```c++
    int i = Begin();
    while (i != End())
    {
        int j = i;
        i = Next(i);
        m_data[j].~T();
    }
```

`j` is not an induction variable, so no giv is formed and the multiply stays.
This is invisible in eighteen of the twenty-two instantiations, whose element
has no destructor and whose loop body is therefore empty; only `VIZoneRoom`,
`VIZone`, `VIRadialFloraInst` and `VICSpritePartEmitter` can see it. It costs
nothing in the other eighteen.

`Next` itself needed phase 7's if-direction: `if (i != m_size - 1) return i + 1;
return End();` gives the target's `bne`, and the inverted form gives `bnel`.

## The element types are read off the two loops

Neither loop needs the element class to exist. `Allocate`'s address arithmetic
states `sizeof(T)`; its loop body IS the default constructor, inlined;
`DestroyPool`'s body is the destructor or nothing. That is enough to model
every one of the twenty-two from the object code alone:

| element | size | constructor | destructor |
|---|---|---|---|
| `char` / `int` / `unsigned long` | 1 / 4 / **8** | none | none |
| `VIVect3` | 12 | three zero stores | none |
| `VIPlane` | 16 | **four** zero stores | none |
| `VIZoneNode`, `VIWorldNode` | 24 | a `VIPlane` first | none |
| `VIHSpriteTrigger` | 16 | `sd` at +8 plus two words | none |
| `VIWorldRegion` | 1 | one `sb` | none |
| `VIZoneRoom`, `VIZone` | 120, 592 | out of line | out of line |
| `VIRadialFloraModel` | 28 | a `VISphere` at +12 | none |
| `VIRadialFloraInst` | 96 | `VIMatrix44` + `VISphere` | `~VIMatrix44` |
| `VICSpritePartEmitter` | 1056 | inline, calls a member's | the member's |
| the other seven | 4..32 | none | none |

Four of those are worth stating on their own.

**`long` is eight bytes, said a third way.** `VIArray<unsigned long>` shifts by
3, and `VIHSpriteTrigger`'s constructor zeroes +8 with `sd`. Phase 16 saw it in
one `sd`-zeroed slot and phase 23 made it decide every 64-bit member; here it is
in the address arithmetic.

**`VIPlane` has a zeroing default constructor.** Its element loop stores four
zeroes; the implicit constructor would run `VIVect3`'s and leave `d` alone. The
store hoisted to the front is the last one, so the emitted 12/0/4/8 reads back
as `n.x, n.y, n.z, d`. Adding it regressed nothing.

**`VIParticleEmitter` is a MEMBER of `VICSpritePartEmitter`, not a base.** The
destructor call in `DestroyPool` lands on `_$_17VIParticleEmitter` at element+16
and the constructor loop calls a constructor at element+16 — a base subobject
would sit at offset 0. So the class holds an `int` at +0 (set to -1), twelve
bytes of padding, the 992-byte emitter at +16, seven words and a `long` at
+0x3F0..+0x410, and its own inline constructor whose *last* store, the `sd`, is
hoisted to the front. Modelled that way all six of its symbols matched first
build, the 53-instruction `Allocate` included.

**`VIRadialFloraInst` constructs its first 64 bytes twice and its next 16 twice
again.** `VIMatrix44`'s `m` is `VIVect4 m[4]` (phase 22) with a zeroing
`VIVect4()`, so the member-array constructors run first as sixteen `sw`s, and
then `VIMatrix44`'s own body zeroes the same quadwords with `vsub`+four `sqc2`;
`VISphere` at +64 does phase 19's double-zero underneath it. Its implicit
destructor inlines to `~VIMatrix44`, which is what puts a `jal` in its
`DestroyPool` where `VIRadialFloraModel`'s has none.

## `VIClipStack`

The portal/frustum clip stack, and `VIArray`'s biggest consumer. Its layout
reads straight off `GetEntry`: three running counts, each one word in front of
the array it counts, plus two scratch vertex arrays.

```
    +0x00 int                  m_numEntries
    +0x04 VIArray<VIClipEntry> m_entries     500
    +0x0C int                  m_numVerts
    +0x10 VIArray<VIVect3>     m_verts      2000
    +0x18 int                  m_numPlanes
    +0x1C VIArray<VIPlane>     m_planes     2000
    +0x24 VIArray<VIVect3>     m_clipA        32
    +0x2C VIArray<VIVect3>     m_clipB        32

    VIClipEntry { int m_numPlanes, m_planeIndex, m_numVerts, m_vertIndex; }  // 16
```

`Init`, `Clear`, `Reset`, `Pop`, `GetEntry`, `GetTop` and `CullByTop(VISphere)`
are byte-exact.

**`Push*` returns the index of the entry it pushed**, not 0 — and the spelling
matters. `return m_numEntries++;` costs an extra `move v0,v1`; the target loads
the old value straight into `v0`, which is

```c++
    int n = m_numEntries;

    m_numEntries = n + 1;
    return n;
```

That one change took `PushBBox` from 264 wrong words to 153 *and* from 283
instructions to the target's 282, and `PushTranslatedVolume` from 19 to 10.

**A `&m_entries.m_data[m_numEntries - 1]` is written with the decrement first.**
`m_entries.m_data[m_numEntries - 1]` gives `sll` then `addiu -16`; the target
gives `addiu -1` then `sll`, so the index is a named local. Three functions.

**The arrays are named through a pointer variable** — phase 9's lever, and phase
15's again: `addiu t0,a0,28` with `lw v1,4(t0)` for `m_planes.m_data`, rather
than `lw v1,32(a0)`. `PushBBox` and `PushTranslatedVolume` both do it, for both
arrays.

**`PushTranslatedVolume` never reads its `numPlanes` parameter.** Its first
instruction is `move a3,a0` — `this` into the register holding `numPlanes` —
and both loops are bounded by `numVerts`. For a portal volume the two counts are
equal, so the bug is invisible in the only caller shape that exists; a volume
with more planes than vertices would copy the wrong number of planes.

**`CullByTop(VIBBox)` compares against `v.x`, once.** It is the standard
positive-vertex cull — pick, per plane, the box corner furthest along the
normal, and test that one point — but the scalar every component is compared
against, and the threshold the plane equation is finally tested against, is
`v.x` read at the top of the loop body, and gcc hoists that read out of the loop
even though the loop stores to that slot. Read once, `v.x` is the 0.0f the
constructor left, so the test is right; the source says `v.x` and not `0.0f`,
and our build keeps the load inside the loop where the target has it in the
preheader. The whole function otherwise matches instruction for instruction —
one `.p2align 3` pad apart.

## Harness

Eighteen new `SPAN_CAP` entries. Every TU that uses a `VIArray` emits its own
*static* copies of the pieces gcc chose not to make weak — a second
`DestroyPool` beside the constructor, a second `Clear`/`Allocate` beside `Init`
— so ten constructors, three destructors and five `Init`s acquire the span of an
unnamed twin: `__t7VIArray1Z14VIResourceElem` derived 1272 bytes for a
four-instruction function, `Init__t7VIArray1Z16VIHSpriteTriggeri` 1160 for
twenty. All eighteen shrink only, land one word past a `jr ra`, and leave
another `jr ra` behind, which is what `test_span_caps_only_ever_shrink` checks.

## Open

* `VIArray<VIWorldRegion>::Allocate`, one `addu` — the `sizeof(T) == 1`
  degeneration above.
* `VIClipStack::CullByTop(VIBBox)` 7/64, one `.p2align 3` pad: the target hoists
  the `sp+0` load out of the loop and ours does not.
* `VIClipStack::PushTranslatedVolume` 10/98 — the second loop's counter and
  array pointer swap `a0` and `a1`; every memory access and every branch agrees.
* `VIClipStack::PushBBox` 153/282, and the count is misleading: normalise the
  register names and the two functions differ by **one adjacent-pair swap**.
  The allocator gives the plane pointer `a2` where the target gives it `t0`, and
  the two constants shift with it. Declaration order does not move it (all six
  permutations scored identically) — the standing phenomenon.
* Not attempted: `ClipByTop` (the Sutherland–Hodgman clipper), both `PushPortal`
  overloads, and `PushFrustum` / `PushTranslatedFrustum`, which read `VICamera`
  and want that class's layout first.
