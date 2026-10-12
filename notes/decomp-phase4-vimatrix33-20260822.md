# Phase 4 — `VIMatrix33`, and the one thing the toolchain on disk will not do

**2026-08-22.** `decomp/src/vimatrix33.cpp` is a second matched translation unit:
**20 of the 24 `VIMatrix33`-owned symbols are byte-exact**, 577 of ~855 instructions,
17 of them first try. Combined baseline is now **49 of 55** (`decomp/src/MATCHED.txt`),
up from 29 of 31.

```
./eqcheck.py                       # 49/55
./eqcheck.py -f VIMatrix33 -w      # the four that are open, with diffs
```

Nothing about the toolchain changed: `ee-gcc2.95.3-107 -O2 -G0`, `-S`, assembled with
`toolchains/ee-gcc2.95.3-114/.../ps2eeas.exe`. **Phase 3's settled toolchain reproduced a
second, independent class with no re-tuning**, which is the real result — the compiler
identity was established on `VIMatrix44` and it was not fitted to it.

## What matched

ctor · copy ctor · `operator=` · dtor · `Identity` · `Zero` · `Translate` · `Scale(float)` ·
`Scale(VIVect2)` · `Rotate(float)` · `Transform(float,float,VIVect2)` ·
`Transform(float,VIVect2,VIVect2)` · `Add(m,float)` · `Add(m,m)` · `Mul(m,float)` ·
`Mul(m,m)` · `Transpose(const&)` · `SwapRows` · `Store` · `Debug`.

Open: `Transpose()` · `Invert` · both `operator*` free functions.

## The layout

```cpp
class VIVect2 { public: float x, y; };

class VIMatrix33
{
public:
    VIVect3 m[3];      // 36 bytes, row stride 12 -- NOT aligned(16), no padding
    ...
};
```

Three constraints pin it, and all three are the `VIMatrix44` argument run again at a
different size:

* the default constructor's zero loop is **3 iterations of stride 12 with the counter
  walking down to −1** — gcc's array-construction loop over three zeroing `VIVect3`;
* `operator=` and the copy constructor copy **nine floats one at a time** (`lwc1`/`swc1`),
  where `VIMatrix44`'s is a 64-byte block move. So **`VIVect3` HAS a user `operator=`
  where `VIVect4` must not have one** — without it gcc emits an unaligned `ldl`/`ldr`
  block move for `m[i] = m[j]` and `SwapRows` misses by six words;
* `VIVect3` also needs a **user copy constructor**, for the same reason one step further
  out: `return r;` from the two `operator*` overloads copies the local into the caller's
  slot one float at a time.

Adding both to `VIVect3` **regressed nothing** — all 29 `VIMatrix44` matches survived.

## `Translate` writes OUT OF BOUNDS, and it is a bug in the game

`Translate__10VIMatrix33RC7VIVect2` calls `Identity()` and then stores to offsets **36 and
40** — one row past a 36-byte matrix. The source is `m[3].x = v.x; m[3].y = v.y;`, the
`VIMatrix44` translation row copied verbatim to a class that has only three rows. It
matches byte for byte written exactly that way.

It is not a different convention: `Transform(float, float, const VIVect2 &)`, five
functions later in the same file, puts the translation in **row 2** (offsets 24/28), which
is where a 3x3 row-vector affine wants it. So `Translate` corrupts the 8 bytes that follow
the matrix and never sets the translation. `go-eqoa-vi/vimath/matrix33.go` never ported it.

## Style facts, confirmed a second time

Everything in `decomp/README.md`'s Phase 3 list held. Two got sharper:

1. **Row pointers are variables** — `Zero`, `Add`x2, `Mul`x2 all took the
   `float *d0 = &m[0].x, *d1 = &m[1].x, ...` spelling and refused every other. `Zero`
   written as nine `m[i].x = 0.0f;` statements is 11 words wrong; as row pointers it is
   exact.
2. **The scalar in a matrix product is a LOCAL, and a FRESH one each time.**
   `Mul(m,m)` is `f = a.m[i].<comp>;` then three `d[j] op= t<k>[j] * f;`. One reused
   variable `f` for all nine gives the right instructions with the wrong FPRs (28 of 125
   wrong); nine separate locals is exact. Hoisting it is also why `a` is read through
   plain offsets off `a1` with **no base register materialised at all** — one load per
   element gives gcc nothing to CSE.

Two new ones:

3. **`Mul(m,m)` accumulates into `this` directly** — there is no temporary matrix, which
   is visible as rows 1 and 2 reloading their partial sums (the stores to row 0 may alias
   `a` or `b`). `go-eqoa-vi/vimath`'s `Matrix33.Mul` uses a temp, so **the Go port and the
   console disagree whenever the destination is also an operand.**
4. **`Invert` calls `SwapRows` unconditionally** — there is no `row != i` guard in the
   object code, only the two `jal`s.

## The open four are ONE phenomenon, and it is not a flag

All four differ from the target the same way: **the target's build does not merge two
textually identical memory references, and rematerialises where every compiler on disk
keeps the value live.**

| Symbol | Shape of the delta |
|---|---|
| `Transpose()` | target builds **three** induction pointers — a separate one for the load of `d[i][j]` and for the store of it — where the natural source builds two |
| `Invert` | `w.m[i][i]` is **loaded twice** through two addresses built identically from one copied base (`move a0,v1`); inside the pivot scan `w.m[k][i]` is loaded twice again, once for `abs.s` and once inside the `if` |
| `operator*` x2 | the local accumulator is written to its stack slot **after every statement**; ours sinks all three stores to the end and reorders every load above them |

This is the same shape as Phase 3's two leftovers (`VIMatrix44`'s copy constructor and the
third `ConvertTo`→quat), so **six functions now show it**.

What was tested and **refuted**:

* **It is not an optimisation flag.** `-fno-rerun-cse-after-loop`, `-fno-cse-follow-jumps`,
  `-fno-cse-skip-blocks`, `-fno-gcse`, `-fno-expensive-optimizations`, `-fno-thread-jumps`,
  `-fno-strength-reduce`, `-fno-force-mem`, `-fno-schedule-insns{,2}`, `-fno-peephole`,
  `-fno-defer-pop`, `-funroll-loops`, `-fno-omit-frame-pointer`, `-O1`, `-O3` and
  **`-fvolatile` / `-fvolatile-global` / `-fvolatile-static`** all leave `Transpose()` at
  33 of 34 wrong. `-fvolatile` was the strongest candidate — "treat all memory references
  through pointers as volatile" would explain every one of the four — and it changes
  nothing.
* **It is not a compiler on disk.** All eleven working EE toolchains (2.9-990721 through
  2.96) produce the two-pointer form.

What is **not** refuted: it is reachable from the SOURCE. A row pointer declared in the
outer loop splits `Transpose()`'s two givs into three and makes **the entire inner loop
byte-identical** — 34 words against 34, 19 wrong, all four of them in the outer preamble,
where gcc strength-reduces that pointer's `i*12` across the outer loop and emits a second
`li 12`. The target computes both `i*12` inside the guarded region from one constant. That
is the whole remaining delta on that function, and about twenty spellings have not closed
it.

`decomp/src/vimatrix33.cpp` keeps the row-pointer form with the delta written out in a
comment, because it is provably closer than the plain form and it records what was learnt.

## Files

* `decomp/src/vi.h` — `VIVect2`, `VIMatrix33`, and `VIVect3`'s `operator=` + copy ctor
* `decomp/src/vimatrix33.cpp` — the translation unit
* `decomp/src/MATCHED.txt` — 49 symbols, the regression baseline
