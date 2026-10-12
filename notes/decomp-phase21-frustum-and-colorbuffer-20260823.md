# Phase 21 — the view frustum and the baked-lighting buffer

**668/763 -> 685/781.** `VIFrustum` 8/9 and `VIColorBuffer` 9/9 (of the nine
written; the tenth, `Color`, is not attempted), same toolchain as every phase
since 3 — `ee-gcc 2.95.3-107 -O2 -G0 -fno-exceptions`, assembled with the
`2.95.3-114` `ps2eeas` — with **no re-tuning**, and **12 of the 18 byte-exact
first try**.

Both classes were picked for the same reason: they sit directly under `VIRaster`
and `VIScene`, the two largest classes left, and neither needed anything the
corpus did not already have. `VIFrustum` is built on `VIPlane`, `VIVect3`,
`VIBBox` and `VIMatrix44`; `VIColorBuffer` derives from Phase 13's `VIBuffer`
and reads Phase 14's `VIPrimBuffer` field by field.

## VIFrustum — the layout is two accessors

```
Vertex(i)  ->  sll v0,a1,?  mult i,12   addu v0,a2,a0     this + i*12
Plane(i)   ->  sll v0,a1,4  addiu v0,96 addu v0,a0,v0     this + 96 + i*16
```

so

```cpp
class VIFrustum
{
public:
    VIVect3 m_verts[8];     // +0x00, 8 x 12 = 96
    VIPlane m_planes[6];    // +0x60, 6 x 16 = 96, size 0xC0
};
```

`Init()` is `return 0;` and nothing else. Both setters are **whole-object
assignments** — `m_planes[i] = p;` and `m_verts[i] = v;` — not field writes:
written field by field they put `&m_planes[i]` on its own register and fold the
array base the other way round (6 and 7 wrong words); written as one assignment
they are byte-exact through the implicit `operator=`, which is itself
member-wise because `VIVect3` has a user `operator=` (Phase 4). Both setters
**return void** — no `move v0,zero` on any path — where `Plane`/`Vertex` return
pointers out of const methods.

### The lever: a copy is written OUT, component by component

`CalcPlanes` is 200 instructions in six identical blocks, and the first draft
scored **209 of 222 wrong** — a bigger frame, nine callee-saved registers, and
an address spilled to the stack. The block is four `VIVect3` copies followed by
`m_planes[k](v[2], v[1], v[0])`, and everything turned on how the copy is
spelled:

* `v[0] = m_verts[3];` — `VIVect3::operator=` takes a `const VIVect3 &`, so
  after inlining gcc has a **pointer variable** for the source (Phase 9's
  lever). It materialises `&m_verts[3]` into a register, reads `.y`/`.z` off it
  at +4/+8, CSEs the six such addresses across all six blocks and keeps them in
  `s0`-`s5` for the whole function.
* `v[0].x = m_verts[3].x; v[0].y = ...; v[0].z = ...;` — twelve plain
  displacement loads off `this` and twelve plain stores off `sp`. **Byte-exact,
  first build, all 200 instructions.**

That is the same fact Phase 9 recorded from the other side: naming a sub-object
through a pointer is how you *get* a base register, so where the target has no
base register the source must not name one.

**The array is proved by the zeroing loop.** Four locals would emit four
inlined `VIVect3::VIVect3()` bodies in a row (Phase 20: matrix locals are
declared and zeroed ONE AT A TIME); the target emits a **counted loop**
(`li v1,3` … `addiu v0,v0,12`), which is the array default-constructor loop, so
it is `VIVect3 v[4]`.

**And the fourth vertex is copied and never used.** Every block copies four
corners of a quad face and passes three to `VIPlane::operator()`; the fourth
slot (`sp+36`) is written six times and read never. It survives because the
array's address is taken (Phase 11: a dead store to an address-taken local is
not deleted). The faces read, in copy order:

| plane | copied | passed |
|---|---|---|
| 0 | 3,2,1,0 | 1,2,3 |
| 1 | 6,7,4,5 | 4,7,6 |
| 2 | 7,3,0,4 | 0,3,7 |
| 3 | 2,6,5,1 | 5,6,2 |
| 4 | 0,1,5,4 | 5,1,0 |
| 5 | 7,6,2,3 | 2,6,7 |

Each row is one quad face of the frustum in cyclic order — near, far, and the
four sides — so the source really does name all four corners and build the
plane from three of them.

### Cull

Both point forms are byte-exact from the obvious walking-pointer loop:

```cpp
const VIPlane *pl = m_planes;
for (int i = 0; i < 6; i++) {
    float d = p.x*pl->n.x + p.y*pl->n.y + p.z*pl->n.z + pl->d;
    if (r < d) return 1;
    pl++;
}
return 0;
```

`Cull(const VISphere &)` is the same function with `s.c` and `s.r` — it is not
a call to the point form, it is written out again.

## VIColorBuffer — and the `lui` order IS the source order

```cpp
class VIColorBuffer : public VIBuffer   // m_id +0x00, m_userData +0x04
{
    int   m_locked;      // +0x08
    int   m_primBuffer;  // +0x0C   -1 when none
    int   m_pad10;
    int   m_vertCursor;  // +0x14   read by Lock, written by Color
    int   m_numVerts;    // +0x18   numPrims*2 + numMaterials
    int   m_pad1C;
    int   m_numTags;     // +0x20
    char *m_headers;     // +0x24 } laid down as OFFSETS by CalcDataSize and
    char *m_verts;       // +0x28 } rebased onto m_data, in place, by Init
    char *m_tags;        // +0x2C }
    int   m_dataSize;    // +0x30
    char *m_data;        // +0x34
    static VIPrimBuffer *pBuildSource;   static unsigned int *pBuildDMATag;
    static int BuildVQSource, BuildI, BuildM, BuildMLeft, BuildP, BuildPLeft;
};
```

`Init` is `VISurface`'s in-place rebase (Phase 16) at 32 bits, and matched
first try.

**The new lever.** `Lock` and `Unlock` each write eight file-scope statics. Each
write is `lui rX,%hi(sym)` + `sw rV,%lo(sym)(rX)`, and the two halves are
scheduled independently: **the `lui`s come out in SOURCE order while the stores
are rescheduled.** Our first draft put the two `-1` stores adjacent where the
target has them separated by two zeroes, and *four different source orderings
produced byte-identical output* — the store order is not the source's. Reading
the source order off the **`lui` sequence** instead (with `eqcalls.py` naming
each relocation) gave

```cpp
BuildVQSource = 0;  BuildI = 0;  BuildM = -1;
BuildMLeft = 0;     BuildP = -1; BuildPLeft = 0;
```

and both functions went byte-exact at once. So for a block of stores to
statics, **diff the `lui`s, not the `sw`s**.

### CalcDataSize — 61 wrong to 0, in five moves

The three section offsets are laid down with a 16-byte alignment step repeated
three times, and each step needed a different spelling:

1. **`if (x) { ... } return 0;` for the allocation guard.** `Allocate`'s
   `if (m_data == 0) return -1; return 0;` if-converts to `movn` in five
   spellings; wrapping the whole body in `if (m_dataSize) { ... }` is Phase 13's
   lever and is byte-exact.
2. **The `else return -1;` is what puts the two epilogues in the target's
   order.** `if (t == 2) { ...; return 0; } return -1;` lays the `return 0`
   block first; `if (t != 2) return -1;` puts it inline at the top; only
   `if (t == 2) { ... } else return -1; return 0;` produces the target's
   `b` over an out-of-line `-1` into the final `move v0,zero`. Worth 6 words,
   and it is a THIRD answer to the if-direction question Phase 20 said must be
   measured per function.
3. **A fresh local per aligned value.** `m_dataSize = n; if (n & 15) n += 16 -
   (n & 15);` merges the two stores into one register and drops the target's
   `b`/`move` phi; two locals plus Phase 14's fresh intermediate
   (`int t = n + 16; off = t - (n & 15);`) reproduce it exactly. And the
   intermediate is load-bearing in integer code again: `a = a + 16 - (a & 15)`
   reassociates to `addiu v0,v0,-16`, which the target does not have.
4. **Both stores of an aligned value are unconditional.** The target stores
   `m_dataSize` before the test *and* after it, on both paths — its branch
   lands ON the second store, ours skipped it. That is
   `m_dataSize = a; if (a & 15) a = ...; m_dataSize = a;`, not
   `m_dataSize = a; if (m_dataSize & 15) m_dataSize += ...;`.
5. **The tag count is an `if`/`else` with the whole expression in each arm.**
   `if (n & 3) m_numTags++; m_numTags += n / 4;` gives two stores;
   `if ((n & 3) == 0) m_numTags += n / 4; else m_numTags += n / 4 + 1;` gives
   one, with the load and the signed-divide bias duplicated into both arms and
   cross-jumped at the tail. 44 wrong to 27.

Shipped semantics worth keeping: the tag section is sized as
`sum over prims of ceil((prim->m_numVerts + 4) / 4)` 16-byte tags, the vertex
section holds `numPrims*2 + numMaterials` quadwords, and the header section is
`numMaterials * 8` bytes — every section 16-byte aligned, and `VIPrimBuffer`
type **2** (`PackVUNC`) is the only one accepted.

## Open

**`VIFrustum::Cull(const VIBBox &, const VIMatrix44 &)` — 19/66.** The target
does **not** hoist the four plane loads out of the inner vertex loop, and it
enters that loop by **jumping into the middle of its body**, with the first
iteration's head duplicated in the preheader and its vertex addresses folded to
`sp`. Writing the exhausted-loop exit as a `return 1` from *inside* the loop
reproduces the un-hoisted body instruction for instruction — gcc will not move a
MEM out of a loop whose body is reachable only past a conditional exit — but it
costs the post-loop `if (j == 8)` shape the target has, and the two cannot be
had together. Refuted: the natural `for`/`break` form (31/72, every plane load
hoisted out); the `v[j]` subscript form (49/71); `m_planes[i]` subscripts
(35/66, six induction pointers); a source-level peel of the first iteration
(37/73); `while (j < 8 && ...)` (33/66); and **thirteen `-fno-` flags**
(`move-all-movables`, `strength-reduce`, `thread-jumps`, `gcse`,
`cse-follow-jumps`, `rerun-cse-after-loop`, `expensive-optimizations`,
`schedule-insns`, `schedule-insns2`, `reduce-all-givs`, `force-mem`,
`peephole`), none of which produces the target's shape.

## Harness: a positional sweep can INVERT the ranking, not just flatten it

Phase 13 recorded that `eqsweep.py`'s positional count reads as "everything
wrong" once a candidate's length drifts, so it cannot discriminate. This
function is a sharper case: every variant is the target's own length, and the
sweep still gets the ORDER wrong — it ranks `and-while` (42) above
`return-1-inside` (46) where `eqcheck.py`'s diff-aligned count reads 33 against
19. The divergence starts at instruction 20 and every later word shifts, so the
positional count measures how EARLY a variant diverges rather than how much of
it is wrong. `sweeps/vifrustum_cull_bbox.py` carries the caveat in its header;
the rule is the README's — `--keep DIR` and `eqasm.py`, because the listing
decides.

**`VIColorBuffer::Color(VIColor32 &)` — not attempted.** 292 instructions of GS
display-list building against all eight statics; it is the whole build path and
deserves its own pass.
