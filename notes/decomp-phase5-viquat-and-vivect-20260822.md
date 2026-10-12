# Decomp phase 5 — VIQuat and the whole VIVect family, and a settled build flag

**2026-08-22.** Two more translation units on the toolchain Phase 3 settled and Phase 4
reproduced, with **no re-tuning at all**: `src/viquat.cpp` (13 symbols) and `src/vivect.cpp`
(32 symbols, both the float and the software-double halves). The combined baseline in
`decomp/src/MATCHED.txt` goes from **49 of 55 to 84 of 98**.

```bash
cd decomp && ./eqcheck.py            # 84/98 byte-exact
./eqcheck.py -f VIQuat -w            # one class, with diffs for the misses
./test_eqdecomp.py                   # 23 tests, 71 assertions
```

Twenty-one of the thirty-five new functions matched **first try**, which is the real result:
the compiler identity, the assembler identity and the style rules recovered from two matrix
classes predict a quaternion class and a vector class written by the same people.

## The one new build flag: `-fno-exceptions`

> **`ee-gcc2.95.3-107 -O2 -G0 -fno-exceptions`, compiled with `-S`, assembled with
> `toolchains/ee-gcc2.95.3-114/lib/gcc-lib/ee/2.95.3/ps2eeas.exe`.**

`VIQuat::ConvertTo(VIVect3 &)` is the first function anywhere in the tree with a **local whose
class has a destructor** — a stack `VIMatrix44`. With exceptions on (gcc 2.95's C++ default)
the function grows a cleanup region: **three** copies of the `_$_10VIMatrix44(this, 2)` call
reached by `b`, 44 words against the target's 33. With `-fno-exceptions` the length is exactly
right and only the seven words of one asm operand are left.

**Nothing before this could see the flag.** Every function in `vimatrix44.cpp` /
`vimatrix33.cpp` / the rest of `viquat.cpp` is identical with and without it, which is worth
remembering as a shape: *a flag is invisible until some function's code depends on it, and the
one that does may be the 90th you write.* RTTI stays **on** — the module carries `__rtti` and
`__tf<n>` symbols.

That trailing call with `a1 = 2` is not a mystery either: it is the GNU 2.x destructor ABI's
`__in_chrg` argument, so a `jal` whose second argument is a small constant on a class object
is a destructor, not a method.

## Source facts recovered

**`operator*` returns a TEMPORARY, and that is worth ten words.**
`VIQuat::operator*(const VIQuat &) const` writes its four results straight into the caller's
slot with **no stack frame at all**. `VIQuat r; ...; return r;` cannot do that — gcc 2.95 has
no NRVO for a named local, so it builds `r` on the stack and block-copies it out with
`ldl/ldr/sdl/sdr`. `return VIQuat(a, b, c, d);` is elided into the return slot, which means
**the class has a four-float constructor** it would otherwise be impossible to justify.

**The class also has the zeroing default constructor** — earned separately, by
`VIMatrix44::ConvertTo(VIVect3&, VIVect3&, VIVect3&)`, whose local `VIQuat q;` zeroes all four
slots before `ConvertTo(q)` overwrites them. The two constructors are not redundant: with the
zeroing one and a *named* return value the zero stores alias `a0` and survive; with a returned
temporary they never exist.

**Field order decides the schedule.** `VIQuat::RotateX/Y/Z` differ only in where the two
`sw zero` land relative to the `sinf` call: both before it, one either side, or both after.
That is not three source shapes — it is one, written in **x, y, z, w declaration order**, with
the scheduler doing the rest. Writing the zeros first (which is how anyone would naturally
write "set two components to zero and two from trig") reproduces exactly one of the three.

**`Normalize` is one shape across all six widths** — `VIVect2/3/4`, `VIVect3D/4D`, `VIQuat`:

```cpp
if (d != 0.0f) { float r = 1.0f / sqrtf(d); ...; return 1; }
return 0;
```

The `if (d == 0.0f) return 0;` spelling is **indistinguishable at float width** and one word
wrong at double width (`beqzl` where the target has `beqz` with the same delay slot). One
function in the family settles the spelling for all six — and it is the one where the
comparison is a libgcc call, not an FPU compare.

**`NormalizeMagn` has ONE exit** and returns the local `d`, reassigned from the length: the
zero case falls through to the shared epilogue with `d` still 0.0 (`mov.s $f0,$f12`). It is
not `return 0.0f;` early, and the difference is visible as a whole extra epilogue.

**`VIQuat::Slerp` returns void.** Nothing writes `v0` on any path. Its negative-dot half is a
**full textual duplicate** of the positive one, not "negate `b` and recurse": it negates the
dot, takes the same two branches, and negates only the weight applied to `b`.

**`VIQuat::ConvertTo(VIVect3&)` goes the long way round** — a stack `VIMatrix44`, `Zero()`,
`UnitRotate(*this)`, `ConvertToEulerXYZ(a)` — rather than converting directly.
`VIMatrix44::Zero()` is **defined in the class body**: SUPPORT carries the out-of-line VU0
copy at 0x003F01D0 *and* inlines its five words here.

**Distance and DistanceSquared build a real struct difference on the stack.** The `swc1` into
the frame are dead stores gcc keeps because the local is a struct — they are the evidence that
the source is `VIVect3 d; d.x = x - v.x; ...` and not three scalars.

**`VIVect3::HPR` calls `cosf(p)` twice** (gcc cannot CSE across a call it does not know is
pure) and never reads roll; the const reader **stores** roll as `0.0f`. Both confirm
`go-eqoa-vi/vimath` from the other direction.

**The double siblings are entirely libgcc.** `VIVect3D::Magnitude` is six `jal`s — no
`sqrt.s`, no NaN retest, because gcc's inline square root is single-precision only. The same
source text at `double` width therefore produces completely different code, and it still
matches.

## The open set is one phenomenon, and it now has fourteen members

| Symbol | Delta |
|---|---|
| `Distance__C7VIVect3`, `Distance__C7VIVect4`, `DistanceSquared__C7VIVect4` | the multiplies start before all the subtractions are done; one store moves out of a delay slot |
| the four `Distance`/`DistanceSquared` on `VIVect3D`/`VIVect4D` | ours holds four doubles in **s0–s3** with a 112-byte frame; the target spills them to `d`'s own slots and saves **s0 alone**, 64 bytes |
| `ConvertTo__C6VIQuatR7VIVect3` | the inlined VU0 `Zero()` takes `$sp` as its asm operand where the target copies it to `a2` first — 7 words, all of them that copy |
| the six from phases 3 and 4 | unchanged |

Every one is the same sentence: **the target's build does not keep a value live in a register
where ours does, and does not merge two textually identical memory references.** Re-tested
this phase and still refuted:

* **Not a flag.** `-fno-schedule-insns`, `-fno-schedule-insns2`, `-fno-delayed-branch`,
  `-fno-cse-follow-jumps`, `-fno-gcse`, `-fno-expensive-optimizations` — on top of Phase 4's
  sixteen, `-fvolatile` included.
* **Not a compiler on disk.** Ten of the eleven working EE toolchains give our form
  (`2.9-990721` no longer builds the tree at all); GCC 3.2 stays excluded by Itanium mangling.
* **Not the source, for these.** Six spellings of `VIVect3::Distance` — sum in a local, no
  parentheses, row pointer, `this->` qualified, an inline `DistanceSquared` helper — give
  **byte-identical output**. That is a stronger null than Phase 4 had: there the source *was*
  a lever and only the right spelling was missing.

## What this does and does not prove

Phase 3 settled a toolchain against one class. Phase 4 reproduced it on a second with no
re-tuning, which made it a finding rather than a fit. Phase 5 adds a third and a fourth kind
of code — a quaternion class with real control flow, and a vector family whose double half is
all libgcc calls — and adds a *flag* that the first two could not see. The compiler identity
survived the addition of a flag it had never been tested against, which is the useful part.

The fourteen open functions are not a matching problem any more; they are one unexplained
register-allocation policy, reproduced fourteen times.
