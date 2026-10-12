# Decomp phase 27 — eighteen small classes, and the assert that dates a file

**2026-08-24.  1138/1268 -> 1252/1384: 114 of the 116 functions written, on the
settled toolchain with no re-tuning, and 96 of them byte-exact on the first
build.**

(96 is counted from the FIRST score of each class after it was written, and it
excludes `SetHomeShardName`, whose only first-build miss was an un-capped span
rather than anything in the source.)

Every phase up to now took ONE class (or one template) and read it out in
depth.  This one is the opposite sweep: eighteen classes that between them are
smaller than `VIWalkControl` alone, picked because each is self-contained and
each answers a layout question a bigger class will need later — the scene
filters, the query result set, the widget base, the console, the options save,
the two BSP trees, the clipper, the string table and the clock.

| class | written | exact | notes |
|-------|--------:|------:|-------|
| `VICameraEffect` | 14 | 13 | shake + manual camera override |
| `VITimer`        |  4 |  4 | EE timer 0, all static |
| `VILanguage`     |  5 |  5 | the assert line numbers |
| `VIActorFilter`  |  5 |  5 | one virtual, no base |
| `VISceneFilter`  |  8 |  8 | one virtual, no base |
| `VISceneQuery`   |  8 |  8 | does NOT derive from VISceneFilter |
| `VIMotifBlender` |  5 |  5 | a `VILNode` with a weight |
| `VIEnvRay` + `VITransEnvRay` | 9 | 9 | first inheritance WITH a vtable |
| `VIWorldTree` / `VIZoneTree` | 8 | 8 | two `VIArray`s each |
| `VIGameOptions`  | 10 | 10 | the 1024-byte card save |
| `VIConsole`      |  4 |  4 | pad + keyboard |
| `VIWnd`          | 13 | 13 | SIX virtuals, all first try |
| `VIClipper`      |  2 |  2 | two scratch vertex arrays |
| `VIQueryTriList` |  7 |  7 | the only real `new T[n]` in the corpus |
| `VIStaticLighting` | 3 | 3 | both halves of the pointer lever |
| `VIStringTable`  |  6 |  6 | WORDS.TXT |
| `VITimeOfDay`    |  5 |  4 | all static, two jump tables |

Open: `VICameraEffect::Apply` at 56 of 312 and
`VITimeOfDay::GetLocalTime(char *, int)` at 9 of 80, both register allocation
and both described at the bottom.

---

## An assert's LINE NUMBER is an immediate, so the original file is dated

`VILanguage::GetName` and `::GetIsoCode` each carry a range check that calls
`_VIAssert(file, expr, line)`.  The first two arguments are `%hi`/`%lo`
relocation sites, so the strings' contents and their position in `.rodata` are
masked away by the differ and cannot be recovered — but the third is
`li a2,59` and `li a2,69`, plain immediates that have to match.

That is `__LINE__`, and it means the object code states where in the ORIGINAL
file each assert sat.  `src/vilanguage.cpp` is padded so its two asserts land on
lines 59 and 69, and with that both functions are byte-exact.  The padding is
marked as filler in the file itself: it is a measurement of the original's line
layout, not a claim about what those lines held.

Two smaller facts came out of the same two functions:

* **The bound is `sizeof(Name) / sizeof(Name[0])`, not the enum's count.**  The
  compare is `sltiu`, i.e. UNSIGNED, and an enum whose values fit in `int`
  promotes to `int` — so `id < VILANG_COUNT` gives a signed `slti` and only an
  unsigned right-hand side gives `sltiu`.  `sizeof/sizeof` is `unsigned`.
* **`GetID` returns `int`, not the enum.**  Its failure path is `li v0,-1`;
  `(VILanguageID)-1` on a gcc 2.95 enum is an unsigned 0xFFFFFFFF and needs
  `lui`+`ori`.

## A ternary VALUE gives `movn`; every `if` spelling gives `movz`

`VIStringTable::Load` ends with

```c
    r = (FixupTextBuffer() > -1) ? 0 : -1;
```

which materialises -1 into a register, compares with `slt v1,v0` and selects
with `movn`.  Six `if` spellings of the same thing — `> -1`, `>= 0`,
`!(x < 0)`, with and without a named intermediate — all fold the test to
`slti v0,v0,0` and select with `movz`, eight words wrong.

This is the exact dual of Phase 12's finding, where `VIParticleAttributes`'s
vector setters had to be an explicit `if`/`else` and NOT a `||` value.  Neither
form is "the" idiom; the two are distinguishable in the object code and have to
be measured per site.

## The element pointer's index goes in its INITIALISER

Phase 24 established that a `VIArray` element address comes out OFFSET first
because the index is added to an integer-cast base.  Two functions here need the
other direction — `VISceneQuery::Object` and `VIStaticLighting::ReleaseResources`
both produce a BASE-first `addu` — and the spelling that gives it is

```c
    int *p = m_objects.m_data + i;
    return *p;
```

Not `p[i]` with `p` naming the array (that is offset-first, three words wrong in
both), and not the subscript.  The index has to be folded into the pointer's
initialiser, which is what makes the address a materialised VARIABLE in Phase
3's sense rather than a MEM.

`VIStaticLighting::ReleaseResources` needs both halves at once: the ARRAY named
through a pointer so its `m_data` comes off that register while the count stays
on `this` (Phase 9), the pointer declared INSIDE the loop so its address is
computed after the guard rather than before it, and then the ELEMENT through a
second pointer with the index folded in.  Fourteen wrong words down to zero.

## A container's inline `Value(i)` is how `this` stays on the container

`VISceneQuery::EndQuery` walks a `VIMultiMap<float,int>` and adds each value to
a vector.  Written as `m_sorted.m_nodes[i].m_value` the node array is read off
the OUTER object (`24(s1)`), and the address of `m_sorted` is computed into the
pseudo before the call rather than after it — four words wrong, and the same
shape appears in `VIWnd`'s destructor and `OnDraw` over a `VISet<VIWnd *>`.

An inline `V &Value(int i) { return m_nodes[i].m_value; }` in the class body
fixes all three: `this` for the accessor is `&m_sorted`, which CSEs with the
`a0` of the `Begin`/`End`/`Next` calls, so the node array is read at `8(s1)` off
the same register.  No instantiation emits a symbol for it, which is the usual
tell for an inline member (Phase 24's `VIArray::Begin`).

## A member's own constructor already zeroes

`VICameraEffect`'s and `VIEnvRay`'s constructors zero their `VIVect3` members —
and writing `m_manualXyz.Zero()` on top of that emits every store TWICE, because
`VIVect3`'s default constructor has already run.  Obvious in hindsight, and it
cost two builds; what is not obvious is that the extra stores also change the
ORDER of the surviving ones, so the mismatch does not read as "duplicated" but
as "everything after the first six words is wrong".

## Read the switch's data table out of the RELOCATED dump

`VITimeOfDay::GetDayOfWeek(int,int,int,int&)` is a twelve-case switch whose
bodies are `dow += K`.  Ten case blocks exist for twelve months, and the
`.text` cannot say which months map to which block: the jump table is in
`.rodata` and the image is un-relocated.

`memory-dumps/inworld-eqoa.live.eememory` can.  Reading the relocated `lui`/
`addiu` pair at 0x003E888C gives the table at 0x004E9CE0, and its twelve entries
give the month offsets outright:

```
    { 6, 2, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }
```

May's entry points straight at the join — offset 0, so gcc emits no block at all
— and September and December share one block, which is exactly why ten bodies
answer for twelve months.  This is the same dump `eqcalls.py` uses as a link
map, used for the first time to recover a DATA table rather than a callee.

The leap-year test in the same function needed one more shape:
`if (year % 100 != 0) leap = 1; else leap = (year % 400 == 0);` inside the
`(year & 3) == 0` arm.  Writing `leap = 1;` first and then correcting it costs
four words and a dead `li v0,100`.  And its `month < 2` is `sltiu`, so the
comparison is unsigned where the parameter is `int`.

## A store through a REGISTER base kills gcc 2.95's whole memory table

The most useful finding in the phase came out of the one function that did not
match.  `VICameraEffect::Apply` builds a basis from a direction and an up
vector, and the guest RE-LOADS all six components from the stack immediately
after storing them.  Every natural spelling lets gcc forward the stores into the
loads and constant-fold the `(0,1,0)` up vector away, which is 130 words wrong.

What stops it is that ONE of the three stores goes through a register base
(`swc1 $f3,4(s4)`), because the address of the local was materialised for a
later `Normalize()` call.  gcc 2.95's `cse` cannot analyse a `(mem (plus (reg)
(const)))` against a frame slot, so it invalidates its ENTIRE memory table at
that store — and the six loads after it have to be re-issued.  Phase 9's
pointer-variable lever, reached for a reason that has nothing to do with
addressing:

```c
    VIVect3 *pu = &up;

    up.x = 0.0f;
    pu->y = 1.0f;
    up.z = 0.0f;
```

125 wrong words down to 68, and the cross-product block matches instruction for
instruction.  The same reasoning explains why the two crosses and the position
lerp all go through by-value expressions: the frame is exactly 32 bytes longer
than a component-wise version, i.e. two 16-byte temporaries, and the slot that
later holds `(m_manualXyz - pos) * k` is written with the SECOND cross's result
and never read — a dead store that survives only because a call follows it
before the slot is reused, which is Phase 11's rule stated again.

## Six virtuals read forwards off the call sites

`VIWnd` is the first class in the corpus whose declaration order was DERIVED
from vtable arithmetic rather than checked against it.  Phase 17 settled that a
virtual N sits at byte 8N with its function pointer at 8N+4, so the six distinct
`lh a0,8N(v0)` / `lw v1,8N+4(v0)` pairs in the five functions that dispatch give
the order outright: 1 = the destructor, 2 = `OnDraw`, 3 = `OnDrawSelf`,
4 = `OnKeyUp`, 5 = `OnKeyDown`, 6 = `OnMessage`.  Declared in that order all
thirteen functions matched on the first build.

`SetFocus` sends message **295** to the outgoing focus and then to the incoming
one, both with `(oldFocus, newFocus)` as the two integer parameters, and
`_5VIWnd$mFocus` is the one static.

## Layouts and facts the phase settles

* **`VIActorFilter`, `VISceneFilter` and `VISceneQuery` each have exactly ONE
  virtual and NO base class.**  All three `__tf`s take TWO arguments
  (`__rtti_user`), and all three vtables are 24 bytes — header, one entry, NULL.
  `VISceneQuery` is NOT a `VISceneFilter`: its own `m_maxObjects` is at +0x00,
  where a base would have put twenty bytes of filter first.
* **`VIEnvRay` is the corpus's first inheritance WITH a vtable.**
  `VITransEnvRay` adds no data, overrides the single virtual, and its `__tf` is
  the three-argument `__rtti_si`.
* **`VITimer` is the EE's timer 0**: mode 1473 at 0x10000010, compare 18750 at
  0x10000020, INTC cause 9, and `_7VITimer$SystemTime` is EIGHT bytes — `long`
  on this ABI, Phase 23's finding said a fourth way.
* **`VIQueryTri` is 40 bytes and is the only real `new T[n]` in the corpus**:
  gcc's own vec-init loop, nine zero stores (three `VIVect3`s) with the leading
  `int` left alone.  Phase 24 established that `VIArray` does NOT have this
  shape; here is the control.
* **`VIStringTable::GetString` takes a ONE-BASED row** (`addiu a1,a1,-1` is its
  second instruction) and falls back to a file-scope string on every
  out-of-range index INCLUDING a null entry.  `Unload` returns **void** where
  every function around it returns `int`.
* **`VIStringTable::LoadRawFile` and every member of `VITimeOfDay` are STATIC** —
  `this` never reaches `a0` at their call sites, the first real argument does.
* **`VIGameOptions` is a 1024-byte memory-card save** with a magic
  `0x01234567` at +0x3FC, `strncpy` bounds of 63 at +0x10 and +0x50, and a
  64-entry UCS-2 shard name at +0x98.  `Load` calls `SetDefaults` when the FIRST
  WORD IS NON-ZERO, which reads as "a non-zero version field is corruption".
* **`VIConsole`'s keyboard is 0x218 bytes**, sized by the gap between the pad at
  +0x00 and the first flag word at +0x278.

## Harness

Three new `SPAN_CAP` entries, all the same failure: a static function the map
does not name, sitting in the gap after a named one.
`SetHomeShardName__13VIGameOptionsPCUs` is followed by gcc's own
`__static_initialization_and_destruction` for the file-scope options object (the
Phase 14 case again), `EndQuery__12VISceneQuery` by an unnamed free-chain
walker, and `GetDayOfWeek__11VITimeOfDayiiiRi` by a two-instruction static that
returns 1.

## Open

`VICameraEffect::Apply` at 56 of 312.  Everything structural is settled — the
easing curve `cosf(t * pi * 0.5 + pi) + 1`, the basis, the quaternion slerp
between the derived euler angles and the manual ones, the position lerp, the
direction rebuilt from `(sinf(h)*cosf(p), -sinf(p), cosf(h)*cosf(p))`, the shake
table walk — and every remaining word is which of nine pointer pseudos loses a
callee-saved register.  The target spills `&hpr`, `&m_manualXyz` and
`&m_manualHpr` and keeps the six local addresses; ours spills a different three.
Eight source permutations move the boundary around without landing on it.

`VITimeOfDay::GetLocalTime(char *, int)` at 9 of 80: every register agrees, and
the one difference is whether `move t0,v0` (the `GetMonth` result) is emitted
immediately after its call or ends up in the `jal`'s delay slot.  Two spellings
with the results in locals do not move it.
