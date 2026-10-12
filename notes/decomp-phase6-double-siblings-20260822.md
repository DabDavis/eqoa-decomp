# Decomp phase 6 — the double siblings, and what they audit

**84/98 → 134/156 byte-exact.** `VIMatrix44D` (41 functions), `VIQuatD` (13) and the two
free `operator*(const VIVect3D/VIVect4D &, const VIMatrix44D &)` were added on the toolchain
Phase 3 settled and Phases 4/5 reproduced — `ee-gcc 2.95.3 -O2 -G0 -fno-exceptions`, SN
`ps2eeas` 2.95.3-114 — with **no re-tuning of compiler, assembler or flags**. 35 of the 56
matched on the first build.

That is the cheap half. The expensive half is what the double width *told us about the float
source*, and it changed six functions that were already byte-exact.

## The double sibling is an ORACLE for the float spelling

The vector unit is single-precision only, so nothing in `VIMatrix44D`/`VIQuatD` is VU0 and
**every double arithmetic operator is a libgcc call**. gcc 2.95 does not CSE across a call.
So a hoisted temporary and its un-hoisted spelling — indistinguishable at float width, where
CSE folds the repeated subexpression into the one instruction the temporary would have
produced — come out with *different call counts* at double width.

Three source facts were wrong in the matched float tree and the double sibling exposed all
three. In each case the fix was applied to BOTH widths and **both still match**:

| Function(s) | Was | Is |
|---|---|---|
| `VIQuat::RotateX/Y/Z` | `float h = a * 0.5f;` then `sinf(h)`, `cosf(h)` | `sinf(a * 0.5f)` and `cosf(a * 0.5f)` — the half-angle is computed **twice** |
| `VIMatrix44::UnitRotate`, `Transform(VIQuat, …)` ×2 | `x2 = q.x + q.x; … xx = x2 * q.x;` | nine products, each `(q.c + q.c) * q.d` written out, in the order **wx, wy, wz, xx, xy, xz, yy, yz, zz** |
| `VIMatrix44::RotateHPR`, `Transform(VIVect3, …)` ×2 | `shsj = sh * sj; chsj = ch * sj;` | `sh * sj * si` inline; nothing hoisted |

The float target computes `q.w + q.w` once; the double target computes it **three times**,
once per product that needs it. That is not a different source — it is the same source seen
without CSE.

This generalises `feedback_settle_a_spelling_on_the_hardest_width`: the hardest width is not
just `double` for one function, it is **the whole double sibling class, and it audits the
float class**. Any VI class with a `…D` twin should be matched as a pair.

## The two vector classes have zeroing default constructors

`VIVect4D` — proved twice. `VIMatrix44D::SwapRows`' temp stores zero to +8/+16/+24 with the
+0 store dead (the exact mirror of `VIVect4`'s), and both `VIMatrix44D` constructors open
with gcc's array-ctor loop over `VIVect4D m[4]` (counter 3 down to −1, stride 32).
`VIVect3D` — the `VIVect3D inv;` in `ConvertTo(VIQuatD&, VIVect3D&, VIVect3D&)` zeroes +8
and +16 before storing 1.0 into all three, exactly as the float `VIVect3` does. Adding it
took that function from 320/337 wrong to 143/337 and made the LENGTH exact.

**And therefore `VIVect3D::Distance`/`DistanceSquared` and `VIVect4D`'s do NOT declare a
struct local.** gcc 2.95 does *not* delete those constructor stores even when every member
is assigned before it is read: a `VIVect4D d;` costs five words the target does not have
(measured — 61 → 66). The locals are plain `double d[3]` / `double d[4]`.

Copy-initialising instead (`VIVect4D d = *this;`) is worse, not better: it runs the copy
constructor and costs seven.

## `VIMatrix44D` has a destructor that is not in the symbol map

`VIQuatD::ConvertTo(VIVect3D&)` destroys its stack matrix with a `jal` carrying `a1 = 2` —
the GNU 2.x `__in_chrg` argument. There is **no `_$_11VIMatrix44D` entry** in
`SUPPORT_symbol_map.txt` (`_$_10VIMatrix33` and `_$_10VIMatrix44` are both there), so
nothing scores it; the call site is the only evidence. Declaring it is what made
`ConvertTo` match.

That function is also **not** the float one transcribed: the double body has no `m.Zero()`.
`VIMatrix44` has a trivial constructor and must be zeroed by hand; `VIMatrix44D` has a real
one (its member array constructs, then it calls `Identity()`), so the call is redundant and
absent.

## What the double class shows that VU0 hides

Ten `VIMatrix44` methods are VU0 macro-mode blocks and have never been attempted. Their
double siblings are ordinary C++ and are now matched or nearly so, which pins the algorithm:

- `Mul(const VIMatrix44D &, const VIMatrix44D &)` is the **no-temp accumulation**
  `VIMatrix33::Mul` uses, at 4×4: `this` is the destination and rows 1–3 reload their
  partial sums because the stores may alias `a` or `b`.
- The three vector products and both free `operator*` are row-vector, with `m`'s rows as
  pointers and the vector component hoisted into a fresh local per row. Without that local
  `v.x` is reloaded for all four terms — `out` and `v` may alias.
- `SubMatrixMul` is the same accumulation with **no row pointers at all**: plain offsets off
  `this` (0/8/16, 32/40/48, 64/72/80).
- `BlendAccum` multiplies the vector component by the weight **once per row** into a hoisted
  local — four calls in the whole function, not sixteen — and is **not** the shape
  `SubMatrixBlendAccum` uses, which left-associates `m * v * w` per element. Two neighbouring
  functions, two different spellings.
- **`InvertAffine` computes the determinant as the six triple products directly**
  (`r00*r11*r22`, `r01*r12*r20`, …), not as `r0 · (r1 × r2)` over cofactors already formed.
  `go-eqoa-vi/vimath/invert.go` documents the float VU0 path as the latter; the orders of
  rounding differ, so **that comment is worth re-checking against the console.**

## The tail-address spelling is width-dependent

`ConvertTo(VIQuatD&, double&, VIVect3D&)` matched only when the six element addresses in its
`else` branch are **pointer locals declared per statement**:

```cpp
const double *pjk = mj + k, *pkj = mk + j;
qq[3] = (*pjk - *pkj) * s * inv;
```

The `const double &mjk = mj[k];` reference bindings the FLOAT sibling needs hoist all six
across the calls (173/255 wrong); writing `mj[k]` inline stops the hoisting but produces
index-first `addu` where the target is base-first (8/250). Only the per-statement pointer
gets both. **At float width the element load is one `lwc1` with a scaled index and no
address is materialised at all, so the question does not arise** — which is why the two
siblings need different text for the same computation.

## Still open: 22, and it is still one phenomenon

The four new members are the smallest examples yet, and they are all ONE pair swapped
repeatedly:

| Symbol | Wrong / total | Shape |
|---|---|---|
| `BlendAccum__C11VIMatrix44DRC8VIVect3DdR8VIVect4D` | 6/152 | `move s1,v0` ↔ `ld a0,0(s2)`, ×3 |
| `BlendAccum__C11VIMatrix44DRC8VIVect4DdR8VIVect4D` | 8/156 | same, ×4 |
| `__ml__FRC8VIVect4DRC11VIMatrix44D` | 18/143 | `sd v0,N(sp)` ↔ `ld a0,N(sX)`, ×9 |
| `__ml__FRC8VIVect3DRC11VIMatrix44D` | 20/130 | same |

In every case **the target keeps the instruction that consumes the call's result glued to
the call and loads the next operand after it; this build schedules the load first.** Seven
more flags swept on top of Phase 4/5's twenty-two — `-fno-schedule-insns`,
`-fno-schedule-insns2`, `-fno-delayed-branch`, `-fno-strength-reduce`, `-fno-caller-saves`,
`-fno-peephole`, `-fno-function-cse` — and **none improves it**: `-fno-strength-reduce` is
neutral and every other one is worse. Taking the address of the destination local, moving
the declarations, and direct member access were all tried (direct member access scores
156/156 wrong — the row pointers are real).

`Invert__11VIMatrix44D` (191/220) and `InvertAffine__11VIMatrix44D` (297/315) are genuinely
unsolved rather than near-misses: the first differs in how the pivot's index arithmetic is
built (the target keeps `i*32` and `i*8` as two shifts where this build folds them to a
multiply by 40; a row-pointer local and the integer-cast idiom both score WORSE, 193 and
219), and the second has a different determinant expansion, described above.
`ConvertTo(VIQuatD&, VIVect3D&, VIVect3D&)` is now length-exact at 337 words with 143 wrong,
and its float sibling's open delta — the target materialises `&inv.x` into a temporary and
copies it — reproduces exactly.
