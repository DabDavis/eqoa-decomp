# Decomp phase 7 — VIPlane, and InvertAffine decoded by its OPERANDS

**134/156 → 141/164.** `VIPlane` and `VIPlaneD` added (8 symbols, 5 exact) and
`VIMatrix44D::InvertAffine` taken from 297/315 wrong to **byte-exact** — the last of the
three genuinely-unsolved functions Phase 6 left.

## The tool: `re-tools/dblflow.py`

The image is un-relocated, so every `jal` target is zero and **the callee cannot be read off
the instruction**. But a double-precision EE function is nothing *but* calls — every operator
is libgcc — and the OPERANDS are readable. `dblflow.py` symbolically executes the loads,
moves, stores and delay slots and prints one line per call:

```
c20  = f(a.m[1].y, a.m[2].z)
c21  = f(a.m[1].z, a.m[2].y)
c22  = f(c20, c21)
c23  = f(c22, c19)
  this.m[0].x := c23
```

Which libgcc routine each `f` is follows from the shape (a sum of products is not ambiguous),
and 315 instructions of `InvertAffine` came out as a readable algorithm in one pass. The
rewritten source scored **2 of 315 wrong on the first build.**

## `InvertAffine`, in full

```
det = a00*a11*a22 + a01*a12*a20 + a02*a10*a21 - a02*a11*a20 - a01*a10*a22 - a00*a12*a21
```

Four things a reimplementation gets wrong, and the corpus does not forgive any of them:

- **It is the six triple products written out and left-associated**, not `r0 · (r1 × r2)`
  over cofactors already formed. The float sibling runs on VU0 and *does* use the cross
  product; `go-eqoa-vi/vimath/invert.go` documents that form. **The two round differently,
  and the double path is a different computation** — the Go note describes the VU0 one and
  should not be carried over to `VIMatrix44D`.
- **A zero determinant writes NOTHING.** The entire body is inside `if (det != 0.0)`, so a
  singular matrix leaves `this` exactly as the caller had it — it is not zeroed, not set to
  the identity, and there is no return value to check.
- **Every negation is a subtraction from zero** (`__subdf3(0.0, x)`). The EE has no
  double-precision negate, so `-(a10*a22 - a12*a20)` is a call, and `0.0 - (-0.0)` is `+0.0`.
- The translation row uses the **already-divided** inverse rows:
  `m[3].x = -((m[0].x*a.m[3].x + m[1].x*a.m[3].y) + m[2].x*a.m[3].z)`, and it reads them back
  out of `this` (gcc forwards its own stores) rather than recomputing.

The last two words of the match were **a chained assignment**: the three `m[i].w = 0.0`
stores land in the order 2, 1, 0 because `m[0].w = m[1].w = m[2].w = 0.0;` groups right to
left. Six permutations of three separate statements were swept; only the chain's order is
right, and the chain is the natural way to write it.

## `VIPlane` / `VIPlaneD`

`VIVect3 n; float d;` — and **the normal is the FIRST member**, proved by
`operator()(a,b,c)`: `n.Normalize()` compiles to a call with `a0 = this`, so `&n == this`.

`operator()` builds the plane from three points: `u = b - a`, `v = c - b`,
`n = v × u` (that order — `n.x = v.y*u.z - v.z*u.y`), then `Normalize()`; on success
`d = -((n.x*a.x + n.y*a.y) + n.z*a.z)` and it returns 1, on failure `d = 0.0` and 0.

`Project` is one source text at both widths and it is **not** the obvious one:

```cpp
VIVect3 r = n;                                        // copy the normal
float t = -(((v.x * n.x + v.y * n.y) + v.z * n.z) + d);
r.x = r.x * t;  r.y = r.y * t;  r.z = r.z * t;
r.x = r.x + v.x;  r.y = r.y + v.y;  r.z = r.z + v.z;
return r;
```

Writing `r.x = n.x * t + v.x` instead is 13 of 30 wrong at float width and 76 of 81 at
double: at double the copy is visible as three `ld`/`sd` into the slot `r` later occupies.

### The success path goes in the `if`, and the failure falls through

Three of these five functions were fixed by ONE change: **inverting the guard.** The target's
layout is failure-inline, success at the branch target — and that is what you get by writing

```cpp
if (den != 0.0f) { ...success...; return 1; }
t = 0.0f; out.x = 0.0f; out.y = 0.0f; out.z = 0.0f;
return 0;
```

The `if (den == 0.0f) { fail; return 0; } success;` spelling — which reads like the target's
layout — produces the *opposite* block order. gcc 2.95 moves the `if`-body out of line when
it ends in a return, so the source's `if` holds the block that ends up LAST.

### A reference read three times needs a local copy

`RayIntersection` and `LineIntersection` both scale the output vector by `t`, a `float &`
out-parameter. Reading `t` three times costs three reloads, because each store to `out.x`
may alias it — and worse, it forces store/reload/store instead of the target's three
multiplies followed by three stores. `float s = t;` before the group is the whole fix
(+3 words each without it).

## Still open: 23

`Invert__11VIMatrix44D` (191/220) is the interesting one. The target keeps `i*32` and `i*8`
as **two separate live values** — a row base in a callee-saved register and the column offset
spilled — where this build folds `s[i][i]` into one multiply by 40. A row-pointer local
scores 193/227 and the integer-cast idiom 219/225, both worse.

The rest are the standing phenomenon, and Phase 7 added a sharper reading of it: **the
`addu` operand order of a computed address is decided by the allocated register numbers, not
by the source.** `ConvertTo__C11VIMatrix44DR7VIQuatD` matched with a plain `mj[k]`, then lost
the match when an unrelated function in the same translation unit was rewritten — the
allocation shifted and every `addu` flipped from base-first to index-first. The
per-statement pointer-local spelling (`const double *pjk = mj + k;`) is stable across that,
and all three double `ConvertTo` tails now use it.
