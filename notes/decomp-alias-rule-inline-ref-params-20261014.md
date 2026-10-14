# The alias rule: inline reference parameters and the stack (2026-10-14)

**When does ee-gcc 2.95 re-read a stack local after an indirect store?** When the
store, at offset +4/+8, goes through a register that has **no pointer flag** —
and an inline helper's pointer/reference parameter has none whenever the caller
hands it an **expression** (`m_right`, `&m_right`) instead of a **named pointer
variable**. Settled in `viwalkcontrol.cpp` (lane flora, phase 61), where it
closed all four open functions: 35/40 → 39/40.

## The mechanism (990721 source, `work/gccsrc/130/gcc`)

- `alias.c` `find_base_term((plus R k))` returns R's base only if
  `REGNO_POINTER_FLAG(R)`. Otherwise it accepts only a stack (`Pmode ADDRESS`)
  or symbol base, so a base derived from an incoming argument (`this` →
  `VOIDmode ADDRESS`) comes back **0 = unknown**. `base_alias_check` then says
  "may alias", `memrefs_conflict_p` on two different base registers says
  "conflict", and with both MEMs `/s` (struct) `fixed_scalar_and_varying_struct_p`
  does not save it. The store depends on every addressable stack local.
- `(mem R)` at **offset 0** goes through `find_base_term`'s `REG` case, which uses
  `REG_BASE_VALUE` with no flag test → a known argument base → a stack slot is
  never aliased. **So +0 kills nothing and +4/+8 kill everything on the stack.**
- Who has the flag: a pointer-typed local (`stmt.c` `expand_decl`), and an inline
  parameter whose argument is such a REG (it maps straight through).
- Who does not: an inline parameter whose argument is an expression. Reference
  formals (and `this`) are `TREE_READONLY` here, so the argument is not copied up
  front (the `mark_reg_pointer` at `integrate.c:1566` only runs on a REG), and
  `process_reg_param` makes a fresh `copy_to_mode_reg` pseudo with no flag. A
  non-const `T *` formal handed `&m_x` ends the same way: the first copy is
  flagged, then `process_reg_param` copies it again because the formal is a user
  variable and the copy is not.

## Three passes act on it

| Pass | Effect | Where it was seen |
|---|---|---|
| cse | forgets the local's known components → reload | `UpdateDirections` (up.x/up.y after right.y, NOT after right.x), `ViewRelativeMove` (dir.z after force.y), `UpdatePhysics` (f.z after pos.y) |
| flow | a store "killed" by a later store survives if a load through the pseudo sits between | `UpdateLocomotion`: `d`'s zeroing ctor kept |
| sched | loads through the pseudo stay behind stack stores | `UpdateLocomotion`: fwd.y/z, right.* after d's stores |

## Evidence: prediction → variant → outcome

- `UpdateDirections`: `Cross(*rgt, *fwd, up)` with named pointers → **9/84 wrong**;
  `Cross(m_right, m_forward, up)` → **exact**. Control: `CrossTo(VIVect3 *out, ...)`
  handed `&m_right` → also exact (the double copy above). RTL: the store went
  from `(reg/v 119)` (the pointer local) to `(reg/v 124)` (the inline copy).
- `UpdateLocomotion`: `d = pos - prev` through a by-value inline `operator-`
  handed the members → zeroing stores kept, and the "second local" `d2` turned
  out to be that operator's temporary (70 → 68). Then `Dot(d, m_forward)` → the
  first block exact (68 → 35).
- `UpdatePhysics`: `AddVec(m_phys.m_position, f, ...)` instead of named
  `pos`/`vel`/`ang`/`avel` → the whole linear half exact (153 → 121).
- `ViewRelativeMove`: dir.z reloaded after `AddVec(m_phys.m_force, ...)`'s +4
  store in the target, and in the matched build.

## It works in both directions (second pass, same day)

A STACK store also kills a known value that was LOADED through an unflagged
pseudo at +4/+8, and sched holds such a load behind stack stores. Predicted
from the rule, then confirmed:

| Function | Variant | Before → after |
|---|---|---|
| `VICylinder::PointInSolid` | dots and `d -= dir*t` through inline helpers handed `m_dir` (dir.y/dir.z re-read after every store to `d`; dir.x, `m_base` and the argument `p` stay in registers) | 66/72 → exact |
| `VICapsule::PointInSolid` | same, plus `d = m_base; AddScaled(d, m_dir, len)`: the copy-in stores of d.y/d.z survive flow, d.x's does not | 116/125 → exact |
| `VICollRay::Skitter` | each stage's dot reads the SOURCE member through `Dot(v, m_hitNormal)`, not the stack copy | 91/201 → exact |
| `VIHSprite::ProcessHierarchy` | the element through `NodeAt(m_nodes, i)`: the m_data load at +8 stays behind v's zeroing stores | 13/94 → exact |
| `VIMatrix33::Invert` | scale with `row *= q`, eliminate with `M33AddScaled(m[k], m[i], q)`: m[i]'s +4/+8 loads stay behind w[k]'s stores | 57/160 → 15 (→ exact with the `k` lever below) |
| `VIMatrix44D::Invert` | `M44DAddScaled(w.m[k], w.m[i], q)` | 2/218 → exact |

The parameter `p` of `PointInSolid` (a `const VIVect3 &` ARGUMENT) is flagged
by `assign_parms`, so its loads are never killed: "this-derived loads re-read,
argument loads kept" is exactly the rule.

Not the rule (tested, no address-taken stack locals involved): `VIFile::
BufferedRead/BufferedWrite` (register choice, a cross-jumped memcpy),
`VICSFFile::Compress/Decompress` (the second copy of `endian` is a live-range
split, not an alias reload), `VIMatrix44(D)::ConvertTo` (spill slots and an
extra pointer copy in the prologue), `VIMatrix44`'s copy constructor and
`operator*(VIVect3, VIMatrix44)` (register choice, frame order).

A side lever from the Inverts: the elimination loop REUSES the pivot search's
`k`. A fresh `int j` has half the references over a similar live range, so
global alloc ranks it below the strength-reduced row pointers and gives it the
last argument register: `VIMatrix33::Invert` 15 → 0, `VIMatrix44::Invert`
13 → 0 (after `M44At` row reads, 47 → 15 → 13).

## How to use it

A target that **re-reads a stack local** (or keeps a dead-looking stack store)
right after a store through `this+K` at a nonzero offset wants the member
handed to an inline helper (an inline method on the member counts too).
A target that **folds** the local wants a named pointer, or a direct member
store (`this` itself is flagged). Before guessing, find which register the
store goes through: the `addiu vN,s0,K` pseudo pattern (`+0` folded back to
`K(s0)`, `+4/+8` off `vN`) is an inline parameter copy.

Other levers from the same session (not about aliasing) are in
`decomp/src/viwalkcontrol.cpp`'s comments: cse's path-following needs each arm
of an if/else reached by one jump (nested ifs, cross-jumped tails); one
variable reassigned by two `powf` calls; three `powf` calls cross-jumped rather
than one with a selected argument.
