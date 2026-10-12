# Phase 49 — the ROUTE layer, and the only packet builder in the library

**2,310/2,598 -> 2,357/2,649.  Three whole translation units:
`route.cpp` 31/32, `route_cache.cpp` 9/10, `route_table.cpp` 7/9 — 51 written,
47 byte-exact, 39 of them on the FIRST build.**

Phase 48 took the connection; this is the layer under it.  A **route** is one
UDP peer: the socket address every connection to that machine shares, the leaky
bucket that paces it, and a tree of connections sorted by when each one wants to
go out next.  With this the DRDP library is complete except for `drdp.cpp`
itself.

The originals name themselves in their asserts: `C:/code-frontiers-gm/drdp/route.cpp`
and `.../route_table.cpp`.  `route_cache.cpp` has no assert at all and is named
only by its symbols.

## route.cpp — 360 bytes, and the one function that makes a packet

```
+0x000  char m_szTag[4]        "ROUT"
+0x004  vnode_t m_vnode[2]     the drdp's two trees
+0x024  drdp_t *m_pDrdp
+0x028  uns64 m_uiId           key 1
+0x030  uns32 m_uiDesiredFlushTime   key 0
+0x034  vlist_t m_pending      raw bytes to forward verbatim
+0x04C  connections_by_desired_flush_time_t m_connections
+0x070  vset_t m_resetConnections    element 32 bytes
+0x094  vset_t m_resetEndpoints      element 8 bytes
+0x0B8  bandwidth_t m_bandwidth      3,000 bps and a queue of 300 by default
+0x0E8  int m_iTerminated
+0x0EC  int m_iDrop / m_iDelay / m_iJitter
+0x0F8  int m_iFlushNumber     wraps at -1, and never IS -1
+0x100  uns64 m_uiStat[13]
```

**`route_get_key_offset` is the OPPOSITE way round from the connection's.**
Key 0 is the desired flush time at +0x30 and key 1 is the id at +0x28, where
`connection_get_key_offset` puts the far id first and the flush time second.
The two are not interchangeable and the tree that uses each is different.

`route_flush` is **the only place in the whole library that builds a packet**.
It opens a 1,500-byte buffer, writes the local DRDP port and the far port,
drains the two reset sets and the forward list into it, and then walks the
connection tree *in flush-time order* asking each connection for a segment until
the buffer is full — so the connection that has waited longest goes out first,
and that ordering is the tree's, not a scan.  Each connection is offered
`remaining - 64` bytes and answers -2 for "stop the packet", -1 for "nothing to
send but I have moved" and 0 for "here is a segment"; only a 0 gets a segment
header written in front of it.  At the end the flush number is bumped, and
**skipped when it would become -1** because -1 is the "never flushed" value
`connection_get_last_route_flush_number` starts at.

A queued reset is a header with **no body at all**: `route_flush_reset_connection`
sets flag `0x10000` and carries the two instance numbers in the crc and
flush-number slots, `route_flush_reset_endpoint` sets `0x20000` and carries only
the id.  Both are vset ELEMENTS, so the set owns the copy and deduplicates
through the comparator — which is why `reset_connection_cmp` walks all five
fields rather than just the address pair.

## route_cache.cpp — the NAT table

The translation between the address a packet really came from and the address
the protocol calls the route, **in both directions**, with a five-minute expiry
renewed on every use.  It exists because a NAT rewrites the source port: the far
end says "I am 1.2.3.4:5000", the packet arrives from 1.2.3.4:41235, and every
later segment has to go back to the port the NAT actually opened.

One record is on **three** containers — a tree per direction and a list by
expiry — which is why `rce` exposes two vnode offsets AND a vlink offset, the
same shape `close_t` has.  A miss answers with the address unchanged, so a
machine nobody has heard from is reached at the address it claims.

## route_table.cpp — a tree of RANGES

Which endpoint ids live behind which route.  **`uns64_range_t_cmp` returns 0 for
any two ranges that OVERLAP**, so a lookup by a degenerate range (`lo == hi ==
the id`) finds the range that contains it and the tree is a sorted partition of
the whole id space rather than a set of points.  `route_table_init` seeds it with
ONE entry covering everything, route zero; `update_range` splits the entry a
sub-range lands in and then merges with either neighbour that now carries the
same route.  A block is 2,048 consecutive ids, the granularity the master hands
them out in.

**The two files use different sentinels.**  In `update_range` a route of -1
means "delete this range"; in `find` a route of **zero** means "no route" and
answers -1, while an id that lands on no entry at all answers -1 through the
out-parameter and *succeeds*.  `init` is what makes that last case impossible.

## Five levers

**`vset_remove_first` returns 0 on SUCCESS**, so both flush-reset loops are
`while (remaining >= 64 && vset_remove_first(...) == 0)`.  Written as a plain
truth test the branch inverts and the loop runs exactly when it should not — two
functions, one word each, and the only reason it was caught is that the
condition sense shows up as `beqz` where the candidate has `bnez`.

**A runtime-sized stack buffer is `alloca`, and the give-away is the frame
pointer.**  `route_forward_segment` opens with `addiu sp,sp,-144`, `move s8,sp`,
then a SECOND `addiu sp,sp,-2048` — a fixed frame plus a variable one, which is
what gcc emits for `alloca` and never for an array.  Written `char data[2047];`
the function is 27 of 55 wrong with a 2,176-byte frame; written
`char *data = (char *)__builtin_alloca(2047);` it is byte-exact.  Phase 10 found
`alloca` in `VIString`; this is the shape to recognise it by.

**A 40-byte local is placed before a 2-byte one whatever the declaration order.**
`route_flush`'s frame has `usPort` at +3104 and the segment header at +3120, and
no ordering of `uns16 usPort;` and `segment_header_t header;` at function scope
produces it — gcc assigns the larger alignment first.  Declaring the header
inside the `case` block that uses it puts it *after* every function-scope local,
which is the only spelling that gives the target's layout.  18 words.

**A comparison tree over three case values with a hole is an if-CHAIN, not a
switch.**  `route_flush` dispatches on `connection_flush`'s -2/-1/0.  A `switch`
gives one tree; the target has `beq -2`, `slti -1`, `beq -1`, `bnez`, which is
exactly what four consecutive `if`s produce — including the `slti` test for
"below the range", a comparison no switch over {-2,-1,0} needs.  Phase 47's
"a two-case if-chain is a SWITCH" run backwards: the `slti` is the tell.

**A three-way comparator's last `return 1` goes out of line.**  Every `*_cmp` in
this layer walks its fields with `if (a > b) return 1; if (a < b) return -1;` and
ends on a `movz`.  Written that way gcc cross-jumps the final `return 1` into the
shared block the earlier fields use; written `if (!(a > b)) return a < b ? -1 : 0;
return 1;` the last one is placed after the `movz` block, which is what the target
has.  `reset_endpoint_cmp` 5 of 10 wrong to 0, `reset_connection_cmp` 3 to 0,
`drdp_addr_t_cmp` 1 to 0.

And a sixth, small: **`route_create` writes `m_uiId` AFTER the clock read**, not
with the other two initialisers — 39 wrong words to 0 on moving one line past a
call.

## Shipped

**`route_flush`'s forwarded-message loop evaluates `message_get_size` an extra
time and throws it away** — the argument of a trace whose call is gone, the same
pattern as the `drdp_format_*` results the whole library discards.  Without that
stray call the function does not match.

## Harness

**I hit phase 48's own trap again, with a variant it does not name.**  Running
two `eqcheck.py` invocations *in one shell pipeline* is the same collision as
leaving one in the background: the total fell from 2,649 to a lower number and
the second run reported regressions in files this phase never touched.  The rule
is one scorer at a time, and `ps aux | grep eqcheck` before trusting a number —
the pipeline form is easy to write by accident.

## Open

Four functions, and all four are the biggest in their file:
`route_flush` 59/234 (register allocation around the connection loop, plus the
two `resort` calls the target does not cross-jump), `route_cache_touch` 82/166
(which callee-saved register holds each of the two lookup results),
`route_table_update_range` 97/201 (the split-and-merge shape is right; the
branch layout is not) and `route_table_find` 11/30 — two branch-likely
conversions and the ternary landing in `v1` where the target reuses the
pointer's own `v0`.

With `drdp.cpp` — 77 symbols, and the last file in the library — the DRDP stack
would be complete.
