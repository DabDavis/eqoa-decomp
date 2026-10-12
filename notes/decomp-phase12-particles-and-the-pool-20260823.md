# Phase 12 — `VIParticleAttributes` and `VIPool<T>`: 263/302 → 451/516

*2026-08-23*

Two classes, and the second one is a **template**: the first container in this
tree whose source has to answer for thirteen instantiations at once. Same
toolchain as every phase since Phase 3 — `ee-gcc 2.95.3-107 -O2 -G0
-fno-exceptions`, compiled with `-S` and assembled with the `2.95.3-114`
`ps2eeas` — **no re-tuning**, no new flag.

| Class | Result |
|---|---|
| `VIParticleAttributes` | **55 / 55** byte-exact (49 first try) |
| `VIPool<T *>` × 13 instantiations | **133 / 159** (the 26 misses are two functions, repeated thirteen times) |
| **Tree total** | **451 / 516** |

---

## `VIParticleAttributes` — the layout is readable off the accessors

54 of its 57 symbols are two- to five-instruction accessor/setter pairs, so
the class is one of the few in this module whose whole shape can be read
without decompiling anything:

* every const accessor is `jr ra` + one load, and names its own offset;
* every setter is `field = x; m_version++`, where `m_version` is a change
  stamp at `+0x2CC` — **the stamp's load is the setter's FIRST instruction and
  the field's store its second**, which is the scheduler, not the source;
* `operator=` is the **implicit** one. It copies member by member in
  declaration order and returns `this`, which is what settles the *type* of
  every field: `+0x1C` is a `VIVect3` (three `lwc1`/`swc1` off a materialised
  base — `VIVect3::operator=`, which Phase 4 established the class has),
  the gradient is `VIColor32F[32]` (an `ldl`/`ldr` block move per entry, 32
  iterations), and **`+0x2C8` is a FLOAT** (`lwc1`/`swc1`) where its integer
  neighbours at `+0x2C4` and `+0x2CC` are `lw`/`sw`.

Total size `0x2D4` = **724**, which is exactly the length the constructor
hands to `memset` — the constructor is `memset(this, 0, sizeof(*this))`
followed by eighteen calls to its own setters, with the member sub-object
constructors gcc emits *before* the body already having zeroed the same bytes.

### What the phase turned on

**The five vector setters are an explicit `if`/`else`, not a `||` expression.**
`SetInnerOffset` and its four siblings write a companion flag from
"is any component non-zero". Written as
`m_flag = (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f);` the disjunction
materialises **0** first and stores it for the false case; the target
materialises **1** first, in the very first branch delay slot, and stores the
literal `$zero` for the false case. `if (...) m_flag = 1; else m_flag = 0;`
is byte-exact and the `||` form is 12 of 29 wrong — so this is a case where
Phase 9's "a multi-term rejection is one `||`" does **not** carry over: the
`||` is right in the *condition*, wrong as the *value*.

**A default that the object code only shows by counting.** The constructor's
float arguments run 0.0, 100.0, 0.0, 0.5, 0.0, 2.0, 0.0, **0.2, 0.0, 0.2,
0.0**, … — four calls in the size group, not three. Reading the delay slots as
if `mov.s $f12,$f22` belonged to the *following* `jal` merged two of them and
left `SetStartSizeVar` out; the constructor was 65 of 175 wrong for exactly
that reason and byte-exact with the one call restored. **In a
delay-slot-scheduled listing, an argument set-up instruction belongs to the
`jal` above it as often as to the one below.**

Everything else fell out of the first build: `new VIVect3[count]` with the
element constructors inline, `delete [] m_pattern` with ONE null test in the
source (`delete`'s own is the second — Phase 10's rule), and
`SetGradientColor(i, VIColor32F(1.0f, t, 0.0f, t))` with `t = 1 - i/31`
built as a temporary rather than a named local (Phase 5's rule).

---

## `VIPool<T>` — one source, thirteen instantiations

The engine's intrusive free-list pool. **236 `VIPool` symbols exist in
SUPPORT**, across 19 instantiations; 13 of them are pools of *pointers* and
are therefore the same code. One template source matched 133 of their 159
functions.

```
+0x00 m_size       live element count      Node { T m_data; int m_next; int m_prev; }
+0x04 m_capacity   node slots allocated    12 bytes when T is a pointer
+0x08 m_nodes      Node[]
+0x0C m_free       head of the free chain, -1 when exhausted
+0x10 m_first      head of the live chain
+0x14 m_last       tail of the live chain      -2 in m_prev == on the free chain
```

`End()` is **`0x7FFFFFFF`**, not -1 — `go-eqoa-vi` found that from the console
side; here it is `lui 0x7fff` / `ori 0xffff` in the object code.
`DestroyPool()` is `for (int i = Begin(); i != End(); i = Erase(i));` — so
`Erase` returns the *next* index, and `End()` when there is none.

**`Erase` matched byte-for-byte first try, 55 instructions**, which is worth
saying because it is the one function whose semantics were not obvious: it
unlinks the node, pushes it onto the free chain, decrements the count, and
returns the successor it read *before* the unlink — a local, while every other
read of the same field in the function is re-loaded off `m_nodes`.

### Levers this phase added

**A branch to a shared epilogue with the epilogue's own first instruction in
the delay slot is ONE return path, not two.** `Next()` looked like it had a
duplicated epilogue (`b D` with `lq ra` in the slot, and another `lq ra` at
`D-4`). It is the delay-slot filler stealing the branch target's first
instruction and retargeting past it — which only happens when the `if`-body is
**out of line**, i.e. when the *success* path is in the `if`
(`if (next != -1) return next; return End();`). The inverted spelling
falls straight through and emits no branch at all. Same lever as Phase 7's
"the success path goes in the `if`", read from the opposite side.

**A loop variable that is not the parameter is visible as two `move`s.**
`LinkFreeChain(int i)` copies its argument into another register at entry and
then into a third at the top of the loop. Writing the loop over `i` itself
gives one `move`; `int j = i;` before the loop gives both, and the function is
byte-exact. Two spare copies in a 41-instruction function are the whole
difference between 39 wrong and 0.

**`if (i >= m_capacity) ... else ...` with ONE return at the bottom.**
`LinkFreeChain` returns 0 on both paths and the target reaches the second
through a `b` to the single `jr ra`. An early `return 0;` inside the `if`
emits its own `jr ra` instead; the `if`/`else` with a single trailing return
is what cross-jumps.

**Two sites test a node address for null before writing through it** — in
`Add`, and per iteration in `Allocate`'s grow loop (`beqzl` on
`m_nodes + i*12`). Nothing in the compiler produces that: it is in the source.
It is *not* a reference conversion (`Node &n = *(m_nodes + m_free);` costs 44
of 66 instructions), so it is a written `if (n)` on a pointer that cannot be
null — defensive style, twice.

### Instantiated member by member, on purpose

**Not one of the thirteen instantiations carries the whole class.** The module
holds only the members the game reached: six have no `Init`, three no `Size`,
one no `Clear`. `template class VIPool<X *>;` emits all of them, and the ten
extra functions would each be reported as a function this decomp *cannot find*
rather than as one the compiler never emitted. gcc 2.95 accepts per-member
explicit instantiation (`template int VIPool<VISprite *>::Size() const;`), so
`src/vipool.cpp` mirrors the image exactly and the score has no free rows in
it.

### The two that are still open, ×13

* **`Add`, one word.** `addu v1,v0,v1` where ours is `addu v1,v1,v0` — same
  registers, same everything else. Seven spellings of the node address
  (`&m_nodes[m_free]`, `m_nodes + m_free`, `m_free + m_nodes`, a cast, a local
  index, a local base, `&m_free[m_nodes]`) all give the same word. This is the
  standing operand-order phenomenon from Phase 7, at its smallest.
* **`Allocate`, nine words** of 66: the two induction variables in the copy
  loop are created in the other order (two `move`s and two `addiu`s swapped),
  one `bltzl` is a `bltz`, and the closing `if (LinkFreeChain(m_size) < 0)
  return -1; return 0;` **if-converts to `slt`/`movn` where the target
  branches**. 30 spellings were swept — `goto`, accumulator variables, both
  guard polarities, both orderings of the `if (m_nodes)` arms, `Size()` in
  place of `m_size`, `>= 0` vs `< 0` vs `== -1` vs `!= 0`. The interesting
  control is `Init`, in the SAME class: its *first* check
  (`if (Clear() < 0) return -1;`) branches and its *last*
  (`if (Allocate() < 0) return -1; return 0;`) if-converts to `slti`/`movz` —
  and `Init` is byte-exact written the obvious way. So the idiom is right and
  `Allocate` is the standing phenomenon, not a misread source.

---

## A harness change: `SPAN_CAP`

Four `VIPool` symbols read as catastrophic failures (`Size` 140 of 142 wrong)
because the map derives a symbol's size from the gap to the next *named*
symbol, and the map does not name the function that follows. The worst,
`Size__Ct6VIPool1ZP15VIParticleMotif`, is two instructions and derives 568
bytes — the next 566 of which are `VIParticleAttributes::operator=`.

`eqdecomp.SPAN_CAP` is a **hand-audited table of four**, not a heuristic, and
`test_span_caps_only_ever_shrink` holds it to that: every entry must shrink
the derived span, land exactly one word after a `jr ra`, and leave at least
one further `jr ra` inside the derived span — the evidence that the gap really
did swallow another function. A rule that capped *every* span at its first
`jr ra` would silently truncate any function with two returns, and a truncated
target can only ever be compared against a candidate that agrees with the
prefix.
