# Decomp phase 26 — the RED-BLACK TREE, four templates of it, and the list

*2026-08-24*

Phase 25 closed the animation clip and the value pools at **1,004 of 1,098**.
Phase 26 adds the engine's ordered containers: one red-black tree written four
times over (`VISet<T>`, `VIMultiMap<K,V>`, `VIMap<K,V>`), the heap-allocated
list (`VIList<T>`), and `VIDictionary`, which is nothing but a name for one of
the maps.

**1004/1098 -> 1138/1268.**  134 of the 166 symbols in this phase, on the
settled toolchain with no re-tuning:

| file | matched |
|---|---|
| `viset.cpp` — `VISet<VIWnd *>` | 12/16 |
| `vimultimap.cpp` — three instantiations | 34/46 |
| `vimap.cpp` — four instantiations + `VIDictionary` | 58/74 |
| `vilist.cpp` — five instantiations | **30/30** |

Eleven of `VISet`'s sixteen were byte-exact on the FIRST build, the 238-word
`EraseFixup` and both rotates among them.

## What the tree is

CLRS, verbatim, laid over VIPool's index-addressed free list rather than over
pointers.  Every link is an `int` index into one `new char[]` block, so a node
keeps its identity across a reallocation.

```
    +0x00 m_size      live count, the SENTINEL included
    +0x04 m_capacity  node slots allocated
    +0x08 m_nodes     Node[]
    +0x0C m_root      0 when the tree is empty
    +0x10 m_free      head of the free chain, -1 when exhausted

    Node { int m_color; int m_parent; int m_left; int m_right; <payload>; }
```

**Slot 0 is the NIL sentinel**, and three separate measurements say so:
`AddNode`'s first growth step is to **TWO** slots and not one; `Allocate`
stamps `m_nodes[0]`'s four link words to zero on every path; and `End()`
returns **0** where VIPool's returns `0x7FFFFFFF`.  The sentinel index IS the
past-the-end iterator, so `Begin()` on an empty tree and `End()` agree by
construction.

A free node re-uses `m_parent` as the chain link and carries **-2** in
`m_left`, exactly as VIPool marks `m_prev`.  `m_color` is 1 for RED, 0 for
BLACK, and `AddNode` stamps 1.

The payload is where the four templates differ, and nothing else does:

| template | Node | payload |
|---|---|---|
| `VISet<T>` | 20 (T = pointer) | `T m_data` at +16 |
| `VIMultiMap<K,V>` | 24 / 28 / 112 | `K` at +16, `V` after it |
| `VIMap<K,V>` | 24 / 28 / 104 | same, plus `Find` and `Key` |

`VIMap` is `VIMultiMap` with a `Find` and a `Key`; `VISet` is the same tree
with one payload instead of two.  **The three were written out separately in
the engine, and they are NOT one source with different arguments** — see the
divergences below.

## The levers

### The fixup loop's parent is a LOCAL, and EraseFixup's is not

`Insert`'s red-black fixup reads `m_nodes[x].m_parent` into a local once per
iteration:

```c
int q = NODE(x)->m_parent;
if (q == NODE(NODE(q)->m_parent)->m_left) { ... }
```

Written out as `NODE(NODE(x)->m_parent)` at every use — the way `EraseFixup`
IS written — each colour store forces the parent index to be re-loaded, because
the store may alias the very word it came from.  The target keeps `q *
sizeof(Node)` in one register across the whole body, and with it **the two
uncle-is-red arms CROSS-JOIN into a single tail**.  Without it they do not, and
the function is **20 words longer**: 190 against the target's 170.

`EraseFixup` matched byte-exact at 238 words on the first build with the
written-out form, in all five instantiations.  The two functions do the same
kind of walk and disagree about this; it is per function, like phase 20's
if-direction.

### Placement new is what null-tests the payload address

`Insert` stores the value through a pointer it has just null-tested:

```c
T *d = &NODE(n)->m_data;
if (d)
    new (d) T(v);
```

The tested pointer is **node+16**, not the node — VIPool::Add tests the node
itself only because its payload is at offset 0.

### The pseudo-destructor is invisible until it isn't

`FreeNode` destroys the payload and `DestroyPool` destroys it AGAIN:

```c
NODE(i)->m_value.~V();
```

For every POD instantiation that call vanishes and the function is the same 16
words it was without it.  For `VIMultiMap<float,VIEmitterEntry>`, whose value
carries a `VIMatrix44`, it is a `jal` on node+48 and `FreeNode` is 33 words.
`DestroyPool`'s copy runs **after** `Erase` has already freed the node — a
double destruction, harmless here — and it indexes by a COPY of the loop
variable taken before the erase:

```c
for (int i = Begin(); i != End(); )
{
    int j = i;
    i = Erase(i);
    NODE(j)->m_value.~V();
}
```

21 words for a POD, 34 for the entry.  Both byte-exact.

### `Size()` discounts the sentinel

`VIMap<K,V>::Size` is not `return m_size;`.  It is

```c
int n = m_size - 1;
if (n < 0)
    n = 0;
return n;
```

six words, `slt v1,-1,v0` + `movz`.

### `Key` returns a REFERENCE

`VIMap<int,VIEESurfCacheInfo>::Key`'s last instruction is `addiu v0,v0,16` —
an address, not a load.

### The three-armed `y` in Erase does not if-convert, and every spelling of it does

CLRS's `y = (left == nil || right == nil) ? z : successor(z)` compiles in the
target to two branches with the two assignments in their delay slots.  Six
spellings were measured — the conjunction, the nested if/else, the
initialise-then-overwrite, the three-armed chain — and **every one of them
if-converts to a single `movz`/`movn`**.  The best is 34 wrong words out of
117, and the residue is entirely the register renaming that follows.  Left
open in all five instantiations.

## VIList<T> — the SIXTH template, and 25 of 25

The one container in the engine that is not laid over a block of slots: every
node is its own `new`, and **the iterator is the node ADDRESS carried in an
`int`**, so the interface still looks like VIPool's.

```
    +0x00 m_size   +0x04 m_head   +0x08 m_tail
    Node { T m_data; Node *m_next; Node *m_prev; }
```

`End()` is **0x7FFFFFFF** here, not the tree's 0 — a null pointer is not
available as a sentinel when the iterator is a pointer.

**`new Node` is not null-checked by gcc 2.95** (that is `-fcheck-new`), so the
element's initialiser — the copy for a pointer element, the constructor call
for a value one — lands AHEAD of the source's own `if (n == 0) return -1;`.
Read literally off the object code, and it is what the source must say.

`Clear`'s three closing stores are ordered by phase 16's rule: the block's LAST
store of a given constant is the one hoisted to the front, and here that is
`m_tail`.  Source order `m_size = 0; m_head = 0; m_tail = 0;` puts `m_tail` in
the guard's delay slot and the other two after it — 4 wrong words to 0, in all
five instantiations at once.

Five instantiations, and the two VALUE ones fixed two class SIZES by
measurement:

* **`VIActor` is 160 bytes with alignment 16.**  Its node is 176 with the links
  at +160/+164; 160 + 4 + 4 = 168, so the node is rounded UP, which only
  happens if the element's alignment is 16.
* **`VIMaterial` is `aligned(16)` too.**  Its node is 496 with the links at
  +480/+484 — 488 rounded up.  The attribute moves no member (480 is already a
  multiple of 16), which is exactly why it was invisible in phase 15.
* `VIScene::VISceneOccup` is a 24-byte POD: its node is 32, and its `Clear` is
  the same 24 instructions as the two pointer instantiations.

## VIEmitterEntry was two words and is eighty

`vi.h` had carried a guess since phase 19: `class VIEmitterEntry { int m_a;
int m_b; };`.  `VIMultiMap<float,VIEmitterEntry>` refutes it four ways at once.
The node stride is **112**; the key sits at +16 and the value at **+32**;
`Insert` copies the value as two `sw`s plus a `jal` on `(dst+16, src+16)`
relative to the value; and `FreeNode` runs a destructor on node+**48**.  All
four are satisfied by

```c
class VIEmitterEntry
{
public:
    int        m_a;
    int        m_b;
    VIMatrix44 m_matrix;   // +16 by alignment
};
```

— 80 bytes, alignment 16, eight bytes of padding at +8 that **the implicit
`operator=` does not copy**, which is how they are known to be padding rather
than members.

That in turn says **`VIMatrix44` has a user-declared `operator=`**: the
memberwise copy in `Allocate`'s reallocation loop reaches the matrix as a
CALL, where a synthesised trivial assignment would be a `lq`/`sq` block move.

## VIDictionary is a name

Every member forwards to `VIMap<unsigned, VIDictEntry>`, which is its FIRST and
only member, so `a0` goes straight through at every call.  `VIDictEntry` is
`{ VIResourceType m_type; int m_index; }` at node +20/+24.  `Add` stamps the id
into the resource's own `m_id` before inserting; `Remove` is
`Find`/`End`/`Erase`; `FindTyped` names the entry through a POINTER and puts
the success path in the `if` (phase 7), which is worth 8 of its 35 words.

## The harness: SPAN_IGNORE

Four map entries — `VIRStrip`, `VIRStrip2`, `VIRStrip3`, `VIRStrip4` — have
addresses that are **not function starts**.  They land in the MIDDLE of a
function body: `VIRStrip` is the last six instructions of
`VIMap<unsigned,VIDictEntry>::Clear`, `VIRStrip2` is `AddNode`'s `jr ra` and
its delay slot.  Left in the address list they truncate their host by exactly
their own length, and the host reads as "6 words short" forever — `Clear` came
back as 22 words instead of 28, `EraseFixup` as 178 instead of 238.

`eqdecomp.SPAN_IGNORE` drops them from the gap derivation and gives them span
0, the treatment `foreign` already gets.  `test_span_ignore_names_land_inside_
a_function` holds each one to the claim: none of the three words before it may
be a `jr ra`, and two known function starts are checked to FAIL that same test
so it cannot pass vacuously.

Three new `SPAN_CAP` entries as well — every `VIMultiMap` instantiation shares
its TU with a `VIPool` of its own whose walkers gcc did not make weak.

## Open

* `Insert` in all five instantiations, 95-105 wrong words of ~180.  The shape
  is right to the instruction; what differs is which register the parent local
  lands in (`a1` in the target, `a2` in ours), which costs two `move a1,sN`
  before the rotate calls.  Six source permutations were measured and none
  moves it.  The standing phenomenon.
* `Erase`, 34 of 117, five instantiations — the `y` if-conversion above.
* `Allocate`, 4 of ~86 everywhere: two strength-reduced pointers whose
  `addiu`s come out in the opposite order.  Seven spellings measured, all
  identical.
* `Next`, 6 of 45 — the target routes both loop exits through one `move a1,a0`
  and ours short-circuits one of them.
* `VIMap<int,VIEESurfCacheInfo>::Allocate` (60/115) and
  `VIMultiMap<float,VIEmitterEntry>::Allocate` (67/102) — the two whose element
  copy is not a block move.
* `VIDictionary::Find`, 3 words of `addu` operand order.

## Facts corrected elsewhere

* `VIEmitterEntry` — see above; `vi.h`'s two-word guess is gone.
* `VIMaterial` is `aligned(16)`; `VIActor` is 160 bytes, alignment 16.
* `VIMatrix44` has a user `operator=`.
* `VIList::End()` is `0x7FFFFFFF`; the TREES' `End()` is 0.
