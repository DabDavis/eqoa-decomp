# Phase 43 — the DRDP network library: the wire buffer, the containers and the tree

Six whole translation units, all of them from the part of SUPPORT no phase had
touched — **1,941/2,156 -> 2,078/2,305**: **144 functions written, 137
byte-exact**, **98 of them on the FIRST build**.

| TU | written | exact | first build |
|---|---:|---:|---:|
| `buffer.cpp`  | 80 | 78 | 66 |
| `vtree.cpp`   | 32 | 30 | 19 |
| `vlist.cpp`   | 16 | 14 | 7 |
| `vset.cpp`    |  7 |  7 | 4 |
| `ll.cpp`      |  7 |  7 | 2 |
| `rle.cpp`     |  2 |  1 | 0 |

## Why this region

`SUPPORT` 0x004acb50–0x004c1ca8 is **586 symbols and 86 KB of C compiled as
C++** — `unet`, `usocket`, `bandwidth`, `close_set`, `connection`, `endpoint`,
`route`, `message`, `drdp` and the containers under them. It is the biggest
unmatched body left in the module and it is the layer the whole game protocol
rides on: `buffer_read_packed_uns64` **is** the 7-bit varint
`protocol-reference.md` calls `Write7BitEncodedInt64`, and
`run_length_encode`/`decode` **are** the state channel's RLE.

## The source tree is NAMED, by its own asserts

`_uassert`'s second argument is `__FILE__`, a relocation site the differ masks —
and `eqcalls.py` reads string literals out of the relocated dump, so it comes
back readable. Nine files are named this way:

```
C:/code-frontiers-gm/drdp/{usocket,bandwidth,connection,drdp,endpoint,
                           message,route,route_table,vtree}.cpp
```

So the library is `drdp/`, the files are `.cpp`, and `vtree.cpp` is the name the
tree's TU actually had. The five TUs with no assert in them (`buffer`, `vlist`,
`vset`, `ll`, `rle`) are named here by the same convention.

**The asserts also carry `__LINE__` as an immediate** — phase 27's lever, and
`vtree.cpp` needed it four times: 215, 283, 331, 365. The source is padded to
that layout, and every edit that changes a line count above an assert has to be
paid back.

## The wire format, settled

`buffer_t` is `{uns8 *data; int size; int position; int error; int read_only;
int swap;}` and every scalar accessor is one line over ONE pair of helpers that
either `memcpy` or copy the bytes BACKWARDS:

- **`buffer_init`'s flag word**: bit 0 swaps when the host is BIG-endian and bit
  1 when it is LITTLE-endian, so bit 0 means "this buffer is little-endian data"
  and bit 1 "big-endian data". The decision is made once, at init, by calling
  `little_endian()` — which is `lui`/`lb` on a static holding 1, i.e. **measured
  at run time, not compiled in**.
- **`buffer_append_packed_uns64` is the varint**: seven bits a byte, low group
  first, 0x80 as the continuation bit. **`buffer_read_packed_uns64`'s shift is
  bounded at 64** as well as by the continuation bit, so a stream with that bit
  set for ever still terminates.
- **The packed signed codecs are zigzag** — `(v << 1) | sign` with `v = ~v` when
  negative — and **the 16-bit one sign-extends**: the zigzagged value is an
  `int16` handed to a `uns64` parameter, so a result that overflows 16 bits goes
  out as a TEN-byte varint. `buffer_append_packed_uns16`/`uns32` zero-extend and
  cannot.
- **The offset codecs add the raw byte UNSIGNED even where the type is signed** —
  `lbu`/`lhu`, never `lb`/`lh`.
- **The RLE is alternating runs**: zeros (up to 255) then non-zeros (up to 127),
  one header byte `nonzeros << 4 | zeros` when both are small and two —
  `0x80 | nonzeros`, then `zeros` — when they are not, followed by the non-zero
  bytes themselves. A zero header byte ends the stream, which is why only the
  short form can terminate it.

## Shipped, and worth knowing before writing a server against it

- **`buffer_appendm_*` ignores `read_only`** and **neither `m` helper sets the
  error flag on overflow** — the single-value pair does both. An array append
  into a read-only buffer silently writes.
- **`buffer_copy`'s first parameter is the SOURCE**: the memcpy destination is
  parameter 2's data. And when the DESTINATION cannot take the copy it still
  advances the SOURCE's position by the full size before returning.  The
  direction is confirmed by a caller: `run_length_decode` passes the compressed
  input first and its output buffer second.
- **`run_length_encode` reads the start position and throws it away**, then
  returns the END position — so its return value is not the byte count it looks
  like.
- **`vtree_remove` calls `vtree_iterate` TWICE** with the same arguments, once
  to test the successor and once to use it.
- **`vtree_get_bh` computes the right subtree's black height and never compares
  it** — the two results go into a two-element array and only `bh[0]` is read.
- **`vlist_remove_by_key` always returns 0**, whatever `vlist_remove` answered.
- **`vlist_insert` dereferences `p->prev` before null-testing `p`.**

## Layouts

```
buffer_t   24    data / size / position / error / read_only / swap
vnode_t    16    the TREE's node, named by the mangling of
                 vtree_validate_node(vtree_t *, vnode_t *):
                 +0 colour  +4 parent  +8/+0xC child[2], an ARRAY by direction
vlist_t    24    head / tail / size / vnode_offset / cmp / key_offset
vtree_t    36    root / size / vnode_offset / cmp / key_offset / nil(16)
vset_t     36    a vtree_t, and `vnode_offset` doubles as the ELEMENT SIZE
llist      12    Amiga Exec's, a second time -- phase 11 found VILList the same
```

**The list's node is modelled here as the same 16-byte type with the first two
words unioned** — `{next, prev}` at +0/+4 — because no `vlist` function names a
node in its mangled signature, so nothing in this TU can settle it.  The rest of
the library hints the original had two: `close_set`, `connection`, `route` and
`rce` each expose BOTH a `get_vnode_offset` and a `get_vlink_offset`, which
reads as a tree node and a list link being separate members of one record.  The
choice does not reach the code generator for anything matched here.

Both containers store the key offset **relative to the node**, not to the object:
`init` computes `key_offset - vnode_offset` so a key is reached from a node
without going back through the object. A `vset` OWNS its elements — `malloc(size
+ 16)`, `memcpy`, and the node lives in the sixteen bytes of slack, which is
what makes the element size and the node offset the same number.

**The tree carries its NIL sentinel INSIDE itself**, at +0x14, with all three of
its own links pointing at it: no walk needs a null test, `root->parent == &nil`
is the empty-tree test, and the sentinel is black for ever. **The children are an
ARRAY indexed by direction**, so insert, remove, rotate and both fixups are each
written ONCE and reached with `child[dir]` / `child[dir ^ 1]`.

## New levers

**`&tree->nil` written OUT at each use, not held in a local.** A named local is
one pseudo that lives across the whole function; the target rematerialises the
address two and three times. Written out, `vtree_peek_end` went 8 wrong to 0 and
`vtree_lookup` 6 to 0 — and `vtree_peek_end_in_range` needed it too. This is the
mirror of phase 31's "an address NAMED before a call cannot be coalesced".

**A comparison against 0 that emits `slt` is a comparison against a VARIABLE.**
Every spelling of `>= 0` — `> -1`, `!(x < 0)`, a ternary, a named `int r` first,
64-bit — gives gcc's `nor`+`srl` pair (measured, five spellings). Only
`x > less` with `int less = -1;` gives `slt reg,reg`; constant propagation then
turns the load into `li -1`, so the object code shows a folded constant that no
constant will produce. `vtree_insert_raw` 16 wrong to 8 on that line.

**A `break` keeps the block; a `return` moves it out of line.** The last four
words of `vtree_insert_raw` were the loop's exit direction: with `return` inside
the `if`, gcc makes the store the branch TARGET; with `break` it stays the
fall-through, which is what the target has.

**A guarded do/while needs the guard to read a DIFFERENT variable.** Phase 10's
lever, sharpened: `buffer_append_string`'s entry test is `*string` through the
PARAMETER register while its loop walks a callee-saved copy declared inside the
guard. Written over one variable — as a `while`, as a do/while, or as a
`for(;;)` with the exits at the bottom — gcc threads the guard onto the loop's
own test and jumps INTO the loop, three spellings out of three.

**The `m` variants call the single helper per element.** `buffer_readm_*` is not
a bulk memcpy when the buffer swaps: it calls `buffer_read_bytes` once per
element, and gcc reverses that `for` into a countdown guarded by `blez`.

**Phase 7's if-direction, eleven more times** — `buffer_skip`,
`buffer_set_position`, `buffer_copy`'s two error arms, `vset_add`/`remove`/
`remove_first`, `vlist_replace`'s two, `vtree_get_bh` and `vtree_peek_end`. The
rule that keeps paying: the block that lands LAST is the one the source's `if`
holds.

**Two statements, not a chain**, in `buffer_copy`: `from->error = to->error = 1`
stores `to` first, and the second arm's lone store only cross-jumps onto this
block's tail when the tail is the one it duplicates.

**A store-to-load forward is not a re-read** (phase 31) in
`vlist_remove_head`: `list->head = node->next; if (list->head == 0)` reuses the
stored register, where `if (node->next == 0)` re-loads and costs a
branch-likely.

**The limit test in a run counter is an EQUALITY test**: `run_length_encode`
stops with `zeros != 255`, not `zeros < 255` — `bnel` against a register, not
`sltiu`.

**A recomputation moved to the loop's TAIL shares the load at its head.**
`vtree_peek_end_in_range` recomputes the node from the object each iteration;
written at the top of the body it loads `vnode_offset` twice, written after
`object = next` it shares the one the object conversion already made — 16 wrong
words to 0.

## Harness

Three new `SPAN_CAP` entries, all the same cause — a TU's statics are unnamed in
the map, so the named symbol in front of them swallows the gap:

- `uns64_range_t_cmp` **44** (buffer.cpp opens with the read and append helpers)
- `buffer_init_read` **44** (the two `m` helpers follow it)
- `vtree_contains` **112** (the 400-byte delete fixup follows it)

## Open

- `vtree_iterate` 10/66 — the descend loop's guard. The target peels the first
  test and compares against a COPY of the sentinel in a second register; five
  spellings swept (while, do/while, guarded do/while with and without a named
  copy, both if-directions).
- `vtree_vnode_range_cmp` 3/29 — `if (r >= 0) return 0; return -1;` if-converts
  to `movz` where the target branches. Four spellings.
- `run_length_encode` 36/93 — gcc CSEs the two 127s (the loop's limit and the
  entry guard's copy of it) into one pseudo that takes a seventh callee-saved
  register, and every later register shifts. The whole tail of the function is
  instruction-for-instruction identical. Eight spellings swept.
- `buffer_read_offset_int8` / `int16` 2 words each — the `addu`'s destination.
  Their UNSIGNED twins are byte-exact from the same macro; twelve spellings.
- `vlist_remove_head` / `remove_tail` 4 words each — the result variable takes
  the return register where the target keeps it in `v1` and copies.

## Not attempted

The other 444 symbols of the library: `connection` (101), `drdp` (105),
`endpoint` (34), `route` (40), `message` (32), `usocket` (14), `bandwidth` (12),
`close_set` (10), `unet` (8). `connection_parse_segment_body` alone is 3,064
bytes.
