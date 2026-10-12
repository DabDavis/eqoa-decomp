# Decomp phase 22 — VICollide, and the constructor that reached back eight functions

**685/781 → 711/803.**  Twenty-two `VICollide` functions written, **eighteen
byte-exact**, on the settled toolchain with no re-tuning — and one finding made
inside it corrected **eight already-open functions in four other classes**, none
of which are collision code.

`VICollide` is the fourth class in this tree whose *semantics* were settled from
the console before the decomp started: `go-eqoa-vi/vicoll` ports its transform
pair, both `CollideObj` entry points, `QueryObj`, the packing dispatchers and the
buffer walk, all bit-exact against a live SUPPORT.  As in Phases 11, 13 and 18,
that makes every mismatch **codegen and nothing else** — there is no question of
having misread what the function does.

    ./eqcheck.py -f VICollide

## The layout

```
class VICollide                              size 0x228
+0x000  int                    m_count       a statistic; the ctor does NOT
                                             initialise it, and 12 bytes of
                                             padding follow to 16-align the
                                             first VIMatrix44
+0x010  VIMatrix44             m_world       object -> world
+0x050  VIMatrix44             m_invWorld    world -> object, DERIVED
+0x090  VIVect3                m_preTranslations[32]
+0x210  VIPool<VICollBuffer *> m_buffers
```

The 32 is the constructor's own loop bound (`li v1,31`, stride 12) and it closes
the gap to the pool exactly.  `+0x218` — the offset every table-driven method
reads — is the pool's `m_nodes`, i.e. `m_buffers + 8`.

**The reference count a collision buffer is shared and released on is
`VIBuffer::m_userData`, not a field of its own.**  `CreateCollBuffer` bumps
`4(b)` and `ReleaseCollBuffer` decrements it, and `sizeof(VICollBuffer) = 92` —
the constructor's own `li a0,92` into `__builtin_new` — closes the declared
layout to the byte.

## The finding: VIMatrix44's default constructor ZEROES

For twenty-one phases `VIMatrix44() {}` was empty and every use site wrote
`VIMatrix44 m; m.Zero();`.  That is indistinguishable from a zeroing constructor
**as long as every VIMatrix44 is a local** — both spellings emit the VIVect4
array-ctor loop followed by the five-word VU0 `Zero()`.

`VICollide` is the first class in the corpus to hold one as a **member**, and
there the two separate.  The target's constructor is

```
    loop (m_world's VIVect4[4])      vsub/sqc2 x4 on m_world
    loop (m_invWorld's VIVect4[4])   vsub/sqc2 x4 on m_invWorld
    loop (m_preTranslations[32])
    jal VIPool<VICollBuffer *>::VIPool
```

— the array constructor and the asm **interleaved per member**.  Nothing in a
constructor *body* can produce that: member construction runs to completion
first, so a body statement puts both `Zero()`s after all four members.  Only a
constructor on `VIMatrix44` itself can interleave them.

**Phase 20 listed "moving `Zero()` into the constructor" as refuted.  It was the
wrong spelling.**  `VIMatrix44() { Zero(); }` emits `jal Zero__10VIMatrix44`:
gcc 2.95 does not re-inline an inline member called from another inline member.
Writing the five-word asm **directly in the constructor body** does inline, and
that is what the target has.

Cost of the change: every `VIMatrix44 m;` site had its now-redundant explicit
`m.Zero()` removed (20 sites across four files).  Benefit:

| newly byte-exact | class |
|---|---|
| `Raster`, `Pick`, `QueryIntersection`, `QueryEmitters`, `Collide(sphere)`, `Collide(ray)` | `VIGroupSprite` (6 → 12 of 13) |
| `Pick` | `VIPointSprite` |
| `ConvertTo(VIVect3&)` | `VIQuat` (now 13/13) |

Which is also the answer to Phase 20's standing open note — *"the target
materialises a fresh register for `VIMatrix44::Zero()`'s asm operand where ours
uses the object's existing base (5 words x 3 matrices x 6 functions)"*.  The
constructor is what materialises it.

## The other levers

**A loop whose test consumes a CALL must call it in the INIT and the
INCREMENT.**  `VICollide::Clear` walks the pool by repeatedly asking for
`Begin()`.  Written `while ((i = m_buffers.Begin()) != m_buffers.End())` it is 13
words wrong; written

```cpp
for (int i = m_buffers.Begin(); i != m_buffers.End(); i = m_buffers.Begin())
```

it is byte-exact, and so is the equivalent `i = Begin(); while (...) { ...; i =
Begin(); }`.  The condition form leaves gcc one copy of the call to place and it
lands in the wrong block.

**Phase 9's pointer variable, four more times.**  `m_buffers.m_nodes[i].m_data`
as a subscript gives `addu off,off,base` — offset-first, result in the offset's
register.  The target is base-first.  Naming the node

```cpp
VIPool<VICollBuffer *>::Node *n = &m_buffers.m_nodes[i];
```

fixes `CollBuffer`, `ShareCollBuffer`, `ReleaseCollBuffer` and `DebugResources`
at a stroke.  `ReleaseCollBuffer` needs the *pool* named too, because it also
calls `Erase` on it — but `DebugResources` gets 25 words WORSE if the pool is
named, since that takes `this` out of `s2`.  The lever is per site, not per file.

**`VIVect4::Set` is how a returned temporary is copied.**  `CollideObj`'s hit
normal comes back from `operator*(const VIVect4&, const VIMatrix44&)` in an sret
slot.  `wn = nrm * m_world;` copies it with `lq`/`sq` — VIVect4 has no user
`operator=` (Phase 4).  The target copies **component by component**, which is
the `Set(const VIVect4 &)` that Phase 3 already earned for `VIMatrix44::SwapRows`:
`wn.Set(nrm * m_world);`.

**A pair of statements can be in the reverse of the emitted order.**
`DebugResources` accumulates a prim count and a byte total; the object code adds
the prims first, and the source that produces it writes the **bytes** first.
The byte total also needs Phase 14's fresh intermediate in integer code —
`t = bytes + 92; bytes = t + size;`, because one statement reassociates to
`bytes + (92 + size)` and puts the `addiu` on the wrong operand.

**The three dispatchers are a `switch`, and it is Phase 16's rule again.**
`Collide(sphere, buffer)`, `Collide(ray, buffer)` and `Query` all test
`VICollBuffer::m_type` and gcc builds the tree in VALUE order (`==1`, then `<2`,
then `==3`) while emitting the bodies in SOURCE order (V, PackV, PackVGF).  An
`if`/`else if` chain cannot make that shape.

**Three more guessed callee names corrected by the link map.**  `eqcalls.py`
reads each `jal`'s target out of a resident SUPPORT, and `VISimpleSprite`'s
three calls into `VICollide` were all wrong in the stub declaration:
`SetTransform` is **`SetWorld`**, `CopyObj` is **`ShareCollBuffer`**, and
`ReleaseObj` is **`ReleaseCollBuffer`**.  They cost nothing while they stood —
a `jal` target is zeroed in the un-relocated image — but they are wrong in the
same way `GetSprite`/`Release` were before Phase 20.

## Shipped facts

* `SetPreTranslations`'s `int` is a **count**, not an index: the body is one
  `memcpy` of `n * 12` bytes.  There is no bounds check, so `n > 32` walks into
  the pool header.
* `QueryObj` hands `Query` the **world** box first and the object-space box
  second, and only the second one culls — which is why `Query` carries a
  parameter no walker reads.
* `CollideObj` returns only three fields to the caller: `m_hitTime`, `m_hitIdx`
  and `m_hitNormal`.  Start, End, Delta and Dir are left exactly as they were, so
  the same sweep can go straight to the next object.
* The hit normal goes back through the world matrix as a `VIVect4` with `w = 0`
  and is then normalised **as a VIVect4** — all four components.

## Open

| function | wrong/total | what is left |
|---|---:|---|
| `Cull(VICollRay&, VIBBox&)` | 30/68 | the standing phenomenon.  The source is settled and is NOT the variable: `&&`, a nested `if`, an accumulator and pointer variables for either vector pair all give the same 30 words, and the function has **no stack frame at all**, so there are no locals to spell differently.  The target keeps `m_start.x/y/z` in `$f4/$f3/$f2` for the whole function and puts the shared `return 1` LAST; ours rematerialises the loads and leaves the first `return 1` in line. |
| `Cull(VICollRay&, VISphere&)` | 39/74 | shape settled (the nesting, the four tests, the pointer variable for `m_dir`, the `sqrt.s` NaN retest).  The target holds `0.0f` in `$f7` from the top and the constant 1 in a **callee-saved** `$s1` — it is live across the `sqrtf` call.  Naming the zero as a local scores better (35) but gives it a stack slot the target does not have, so it is not the source. |
| `Cull(VICollSphere&, VISphere&)` | 38/76 | the same, one radius sum earlier. |
| `DebugResources` | 4/61 | **one word.**  The target reads the node table as `8(a0)` off a register still holding `this + 528` from the entry `Begin()` call; ours folds it to `536(this)`. |

Not attempted: `Cull(VICollSphere&, VIBBox&)` (its grown box is a local whose
six live words sit at +0/4/8 and +16/20/24 with +12 untouched — not a `VIBBox`
and not a `VIVect4` pair, and unread), `CollideObj(VICollSphere&, VICapsule&)`,
`PointInTriangle`, the four sweep-vs-primitive `Collide` overloads,
`CollideNoCaps`, the six buffer walkers and the three query walkers.
