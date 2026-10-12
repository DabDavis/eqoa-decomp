# Phase 9 — the shape classes: `VIBBox`, `VISphere`, `VIBRect`, `VIRect`

**141 of 164 → 176 of 212 byte-exact.** Four new classes on the toolchain Phase 3
settled and Phases 4–7 reproduced (`ee-gcc2.95.3-107 -O2 -G0 -fno-exceptions`,
`-S`, `ps2eeas` from the `2.95.3-114` package), **with no re-tuning of compiler,
assembler or flags** — and, for the first time in this project, with *integer*
code in the corpus.

```
VIBBox      20 symbols   16 exact
VISphere     3 symbols    2 exact
VIBRect     13 symbols   12 exact
VIRect      13 symbols    9 exact
```

Everything matched to date had been float math on VU0-adjacent classes. `VIRect`
is `int`: `slt` / `movz` / `movn` / the R5900 three-operand `mult` / `sra`. It
matched from the same build with no adjustment, which is a much stronger
reproduction of the compiler identity than another matrix class would have been.

## The lever this phase found: a MATERIALISED SUB-OBJECT POINTER

Three functions were stuck at "right instructions, wrong addressing" until this
was understood, and it is now a style fact on the same footing as Phase 3's
*row pointers are variables*:

> When the source names a sub-object through a **pointer variable**, gcc keeps
> that pointer in a register and reads the sub-object's **+4 and +8** members off
> it, while folding the **+0** (and, for a 16-byte object, the +12) access back
> into an offset off the outer base.

The target of `VIBBox::Bound` reads `max.x` as `12(a0)` but `max.y` / `max.z` as
`4(v0)` / `8(v0)` off an `addiu v0,a0,12`. **No spelling of `max.y` reaches that**
— 18 were swept. `VIVect3 *mx = &max;` reaches it exactly.

Same shape, three places:

| function | the pointer |
|---|---|
| `VIBBox::Bound`, both `Scale`s | `VIVect3 *mx = &max;` |
| `VISphere::Init(const VIBBox &)` | `const VIVect3 *mx = &b.max;` |
| `VIBBox::StoreOutward` / `StoreInward` | `VIVect3 *n0 = &p[0].n;` … one per plane, with `d` still written as `p[i].d` |

`StoreOutward` is the clean demonstration: `p[i].n.x = …` scores 42 of 44 wrong,
one reusable `VIPlane *q` with `q++` scores 34, six `VIPlane *` scores 17, and
**a `VIVect3 *` per plane's normal — with `d` addressed by index — is byte-exact.**

## Source facts

1. **Assign-then-modify-in-place.** This author writes vector work one operation
   per statement — `c = min; c.x = c.x + max.x; c.x = c.x * 0.5f;` — which shows
   up as three separate stores to the same slot, because a store through `this`
   may alias the const reference the next statement reads. A single expression
   collapses them and never matches.
2. **`if (!(cond)) return 0; return 1;`** is a real shape, distinct from
   `if (cond) return 1; return 0;` and from `return (cond);`. It decides which
   value is the preset in `v0`. `VISphere::PointInSolid` went from 22 of 27 wrong
   to byte-exact on that alone, once `d = v; d.x = d.x - c.x;` was in place.
3. **A six-term rejection is one `||`, not six `if`s.** `VIBBox::Intersects`,
   `CullIntersect` and both `Intersects(VIBRect/VIRect)` all branch to a single
   shared exit; six separate `if (…) return 0;` statements inline the first one.
4. **A fresh local per squared distance**, exactly as Phase 4 found for the
   scalar in `VIMatrix33::Mul`: `VISphere::Init(a,b,c)` scores 3 of 99 wrong with
   one reused `t` and byte-exact with `t` and `u`.
5. **`VIVect2` needs the zeroing default constructor, the two-float constructor
   AND a user `operator=`** — the last for the same reason `VIVect3` does: without
   it `min = v` and `c = min` come out as an unaligned `ldl/ldr/sdl/sdr` 8-byte
   block move. `VIVect2I` needs `operator=` and the two-int constructor but **no
   zeroing default constructor**: `VIRect::DistanceSquared` compares against a
   literal 0 (`slti v0,v1,1`) where `VIBRect`'s reads its own zeroed slot back.
6. **`VIVect3` gains a three-float constructor**, earned by `VIBBox::Size()` /
   `Center()`, which build the return value straight into the caller's sret slot.
7. **`v = v * m` in `VIBRect::Transform`, field-by-field in `VIBBox::Transform`.**
   Both assign the free `operator*`'s result back into the input vector; the
   stores are dead and are visible only in the LAST corner, where nothing
   overwrites it again. `VIVect3` has a user `operator=` so the 2D twin can write
   `v = v * m`; `VIVect4` must not have one (Phase 3), so the 3D one spells the
   four fields out or it becomes a 16-byte block move.

## Two more shipped bugs, both matched byte for byte

* **`VIBBox::Scale` scales about `min + max`, not the centre.** There is no
  `0.5f` anywhere in either `Scale`, while `Center()` and `VISphere::Init` both
  materialise `0x3F000000`. A box away from the origin is therefore MOVED by
  `Scale`, not just resized. Both `VIBRect::Scale`s and both `VIRect::Scale`s
  carry it too.
* **`VIRect::Store`, `VIBRect::Store` and `VIBRect::Transform` emit a degenerate
  rectangle**: the `y` of all four corners is `min.y`, and `+12` (`max.y`) is
  never read at all. `VIBBox::Store`'s eight corners are correct, so this is a
  2D copy-paste, and it is in all three functions.

Precedent: `VIMatrix33::Translate` writing one row past the matrix (Phase 4).
Matching keeps finding these because a decomp cannot round a shipped mistake off.

## Open: thirteen new, and every one is a named phenomenon

| symbols | delta | family |
|---|---|---|
| both `VIBBox::Scale`, both `VIBRect::Scale`, both `VIRect::Scale` | the target reloads the stack local `c` after every store through `this` and keeps two of its three copy stores; this build forwards all of them | **`cse`** memory forwarding — Phase 8d's family |
| `VISphere::Init(const VIBBox &)` | the target keeps `d`'s zeroing constructor and two of the three `d = *mx` stores; this build deletes all of them. Eight spellings (declaration position, field-wise vs `operator=`, pointers on `c` and on `b.min`) give the identical 43 | same |
| `VIBBox::Bound`, `VIBRect::Bound` | one instruction: the target shares ONE `lwc1` of `v.x` between both arms of the `if` — a non-annulled delay slot that `redundant_insn` merged with the head of the fall-through block. This build allocates `v.x` to `f2` in one arm and `f1` in the other, so the two loads are not the same insn, cannot merge, and gcc annuls (`bnezl`) and duplicates the load | **`lreg`** |
| both `VIRect::Union`, `VIRect::Intersect` | right instructions, `movz` where the target has `movn` (and vice versa) — the conditional move's destination is the register the *other* operand was loaded into. **All sixteen ternary spellings are byte-identical**, so it is not the source | `lreg` / `REG_ALLOC_ORDER` |
| `VIRect::DistanceSquared` | two words: the target clamps with `slti v0,v1,1` + `movn zero`, this build materialises `li v0,-1` and compares the other way. **Eight spellings identical**, `if`-forms worse | `combine` / conditional-move condition choice |

Nothing here needs a new explanation; the three passes Phase 8 named cover all
of it. `VIBBox::Intersects(const VIVect3 &, const VIVect3 &)` (233 instructions,
five stack vectors and a call per box plane) is the one function in the four
classes not yet attempted.

## Harness

`eqsweep.py --keep DIR` writes each variant's object out for `eqasm.py`. A sweep
scores POSITIONALLY, so a variant whose layout is right but whose length is off
by one word reads as "everything wrong" — `VIBBox::Bound`'s winning form scored
61 of 62 and `VIBRect::Transform`'s scored 44 of 44. **The count ranks; the
listing decides.** Two of this phase's matches were found by looking at a
variant the ranking had put last.
