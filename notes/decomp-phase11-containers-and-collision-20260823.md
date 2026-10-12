# Decomp phase 11 — the intrusive list and the collision primitives (2026-08-23)

**222/259 → 263/302 byte-exact.** Four new classes on the settled toolchain
(`ee-gcc 2.95.3-107 -O2 -G0 -fno-exceptions`, assembled with
`toolchains/ee-gcc2.95.3-114/.../ps2eeas.exe`) with **no re-tuning of anything**:

| Class | Result | What it is |
|---|---:|---|
| `VILNode` / `VILList` | **19 / 20** | the engine's intrusive doubly-linked list — the first CONTAINER |
| `VICollRay` | **7 / 8** | the collision ray — the first class whose SEMANTICS were settled before the decomp |
| `VICollSphere` | **4 / 4** | `VICollRay` + a radius — the first INHERITANCE in the tree |
| `VIPickSegment` | **11 / 11** | the ray's sibling, with a material id and a colour |

Open: `VILList::Node` (16/20 wrong) and `VICollRay::Skitter` (171/201). Both are
the standing phenomenon, not a misread source — see the last section.

Sources: `decomp/src/villist.cpp`, `decomp/src/vicollray.cpp` (which also holds
`VICollSphere`), `decomp/src/vipicksegment.cpp`.

## `VILList` is Amiga Exec's list, transliterated

`Init` is `NewList()` character for character:

```cpp
void VILList::Init()
{
    m_head     = (VILNode *)&m_tail;    // +0 lh_Head
    m_tail     = 0;                     // +4 lh_Tail, always 0
    m_tailPred = (VILNode *)&m_head;    // +8 lh_TailPred
}
```

so both ends of a walk terminate on a null the header itself provides, and
`AddHead`/`AddTail`/`Remove` need no special case for an empty list. `VILNode`
is `{ next, prev, int pri, unsigned id }` — `pri` is read by
`VILList::PriorityInsert` alone, and `id` is `VIHashResourceID(name)`, set by
`SetName` and searched by `Find(unsigned)`; `Find(const char *)` is one line
that hashes and calls the other.

`VILNode::List()` walks `prev` to the end and returns the header cast back to a
`VILList *` — the header IS the terminal node, so `Index()` counts by testing
`p->m_prev->m_prev`.

### Levers this file earned

**A forwarded field write is not a reload.** `AddHead` reads `m_head` once and
uses the stored value again:

```cpp
n->m_next = m_head;
n->m_next->m_prev = n;      // <- forwards the value just stored
```

Spelling the second line `m_head->m_prev = n` reloads `m_head` — one extra
`lw`, and the function is wrong. `AddTail` needs the opposite shape (a named
local `VILNode *t = m_tailPred;`) because there the target loads *before* the
first store, which a forwarded read cannot produce.

**A guarded do/while, again.** `Length`, `Index()` and `List()` are all
`if (cond) { do { … } while (cond); }`, Phase 10's `VIString` lever verbatim:
a plain `while` leaves the exit test at the top where gcc reuses its load.

**An accumulator beats an early return** when the target has one exit through a
callee-saved register. `RemHead`/`RemTail` are

```cpp
VILNode *r = 0;
VILNode *n = m_head;
if (n->m_next) { r = n; n->Remove(); }
return r;
```

and not `if (…) return 0; …; return n;`, which splits the exit.

**`PriorityInsert` is a `switch`, and both comparisons put `p` on the LEFT.**
The two orders are `p->m_pri >= n->m_pri` (ascending) and
`p->m_pri <= n->m_pri` (descending) — written the other way round the two
`lw`s swap, and that is the whole difference between 2 words wrong and exact.
An `if/else if` chain with an `else { advance; continue; }` scores 18; the
`switch` with two cases and no default scores 2.

## The collision primitives: semantics known BEFORE the decomp

`go-eqoa-vi/vicoll/ray.go` is already a bit-exact Go port of `VICollRay`,
checked against the console, so its layout and behaviour were not in question
here — the same position `VIMatrix44` was in at Phase 3, one layer up. Every
mismatch was codegen.

```
+0x00 m_start  +0x0C m_end  +0x18 m_delta  +0x24 m_dir  +0x30 m_length
+0x34 m_invLength  +0x38 m_dir2  +0x44 m_hitTime  +0x48 m_hitIdx  +0x4C m_hitNormal
```

The default constructor is **out of line and EMPTY** — everything it emits is
`VIVect3`'s zeroing ctor run over the six vector members, which is why
`0x30/0x34/0x44/0x48` are the only words it leaves alone. That is also the
proof of the layout: the four gaps are exactly the four scalars.

### New levers

**Compound operators are how a sub-object gets a base register.** Phase 9's
"name a sub-object through a POINTER VARIABLE" lever has a natural spelling:

```cpp
m_delta = m_end;      // operator= takes a reference -> &m_end in a register
m_delta -= m_start;   // inline operator-= -> &m_delta in a register, +0 folded
m_dir = m_delta;
m_dir *= inv;
```

Writing the components out by hand (`m_delta.x -= m_start.x; …`) reads
everything off `this` and lets gcc forward the subtraction results straight
into the next statement's stores — 62 of 62 words wrong. With the operators,
`VICollRay::Precalc`, `VICollSphere::Precalc` and `VIPickSegment::Precalc` are
all byte-exact. `VIVect3` therefore has inline `operator-=`, `operator+=`,
`operator*=(float)` and `Zero()`; adding them regressed nothing.

**A returned VIVect3 is a named local ASSIGNED from a member, not built
component-wise.** `GetHitPoint` is

```cpp
VIVect3 p;
p = m_delta;                             // <- the assignment is load-bearing
p.x = p.x * m_hitTime + m_start.x;
…
return p;
```

The `p = m_delta` puts `&m_delta` in a register with `+0` folded, and the
ctor's `sw zero` at +4/+8 survive while the one at +0 does not — the same
dead-store asymmetry `vi.h` already records for `VIVect3()`.

**The zero-length block is the ELSE.** `Precalc`'s guard is written
`if (l != 0.0f) { divide } else { m_invLength = 0.0f; }`. Written the other way
round the two blocks swap and the compare comes out `bc1f` — the only
difference between 16 words wrong and byte-exact.

**Phase 9's `if (!(cond)) return 0;` shape converts to `movz`.**
`VIPickSegment::Init` ends `if (Precalc() < 0) return -1; return 0;`. Every
ternary spelling — `Precalc() < 0 ? -1 : 0`, `>= 0 ? 0 : -1`, a local first,
`-(r < 0)`, `>> 31` — gives `slt`+`movn` and is 6 words wrong. The BRANCHING
form is what gcc if-converts into `slti`+`movz`.

**A dead store to an address-taken local survives ACROSS A CALL and not
otherwise.** The three `Init(src, matrix)` overloads (90 + 113 + 83
instructions, all byte-exact) transform a ray by a `VIMatrix44`:

```cpp
VIVect4 a, b;
a.Set(src.m_start * mat);
m_start.x = a.x; m_start.y = a.y; m_start.z = a.z;
a.Set(src.m_end * mat);          // <- the SAME local, not a second one
m_end.x = a.x; …
```

`a` is address-taken (it is the third `operator*`'s argument), so gcc cannot
kill the first `Set`'s stores across the intervening call — but it does kill
the second's, which are overwritten in the same block. Using `b` for the second
makes three stores appear that the target has not got.
`VIPickSegment`'s version, which has no third call, genuinely uses two locals.

## Two shipped facts about the classes themselves

**`VICollSphere` inherits from `VICollRay` with no vtable** — the derived
constructor is nothing but the base constructor call — and **both `Init` and
`Precalc` REPEAT the base's body rather than calling it.** The base versions
are out of line, so a call would be visible as a `jal`, and neither has one.
It is copy-paste in the original.

**`VICollSphere::Init(src, matrix)` scales the radius by the length of the
matrix's first row, and the square root is INLINE `sqrtf` with the NaN retest**
(`sqrt.s`, `c.eq.s f1,f1`, then the libm call only if it produced a NaN) — not
`VIVect3::Magnitude`, which is out of line and would be a `jal`.

**`VIPickSegment` is NOT a `VICollRay`.** Same first 0x38 bytes by coincidence
of purpose, but it has no second direction copy; `+0x38` is the hit time,
`+0x3C` a material id (`-1` for none, set by the constructor body and by
`Precalc`, never by `Init`), `+0x40` the hit normal and `+0x4C` a
`VIColor32F`. Its `GetHitNormal` returns **by value** where `VICollRay`'s
returns a reference. Its `m_hitColor = src.m_hitColor` is a 16-byte
**unaligned block move** (`ldl`/`ldr`/`sdl`/`sdr`), so `VIColor32F` has no
`operator=` and no alignment attribute — the same story `VIVect4` tells.

## The two open functions are the standing phenomenon

`VILList::Node` (16/20) and `VICollRay::Skitter` (best 113 of 201 words aligned)
both come down to **register allocation and the CSE/store-alias interaction**,
the family the other 37 open functions are in.

**`Node` is `greg`, and it is nameable.** The target caches `p->m_next` in a
register across the loop and keeps `p` in `a0`; every source spelling that
caches the next pointer makes gcc allocate `p` to **`v0`** — the return
register — which then lets `p = nx` ride a delay slot, inverts the loop's exit
branch, and splits the three `return 0` exits that the target cross-jumps into
one. `decomp/eqrtl.py`'s `-da` dumps name it exactly: after `greg`, pseudo 119
(`p`) is `reg/v:SI 2 v0` and `a0` holds no user variable at all. Twenty source
spellings were swept (`sweeps/villist_node*.py`); the best that keeps `p` in
`a0` cannot cache the next pointer, and the best that caches it cannot keep
`p` in `a0`.

**`Skitter` is the CSE-vs-store-alias half.** All five stages are
`out = in; d = -dot(axis, in); out += axis*d;`. The target, in stage 1,
**reloads** `m_hitNormal.y` and `.z` after the copy's stores invalidate them,
and reuses the copy's already-loaded registers for the result; ours reuses one
set of loads for both. Making the dot read the source instead makes gcc
dead-store-eliminate the copy entirely (which the target keeps); making it read
the copy makes gcc forward instead of reload. Fifteen spellings — plain member
access, a `const VIVect3 &`, a pointer, element-wise copy, result-from-source —
land between 78 and 113 aligned words and none of them is both.

Two things about `Skitter` DID fall out and are now in the source: a
`const VIVect3 &e = m_dir2;` declared once and used by **both** stage 2 and the
distance dot is what puts `&m_dir2` in a callee-saved register (`s1`) and takes
the frame from 160 to the target's 176 bytes; and the final ray is built
`m_end = t0; m_end *= dist; m_end += m_start;` through the compound operators,
not component-wise.
