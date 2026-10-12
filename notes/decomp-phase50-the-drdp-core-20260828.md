# Phase 50 — `drdp.cpp`, the top of the library (and the last of it)

**2,357/2,649 -> 2,457/2,761.  One translation unit, `drdp.cpp`: 112 functions
written, 100 byte-exact, 72 of them on the FIRST build.**  With this the DRDP
network library is COMPLETE — all twenty-one of its translation units are in
`decomp/src/`.

`drdp.cpp` is SUPPORT `0x004b4aa8..0x004b9218`.  A `drdp_t` is one PROCESS on
the network: the sockets it listens on, every endpoint, route and connection it
knows about, the address<->route cache, and the leaky bucket that paces the
whole machine.  544 bytes off `malloc`, tagged `"DRDP"` the way a message is
tagged `"MESS"`.  Everything below it in the library exists to serve four entry
points here — `drdp_flush`, `drdp_process_packet`, `drdp_receive` and
`drdp_set_time`.

## What the file is

**Three ROLES share it**, and the asserts name them: `DRDP_ROLE_MASTER` 0,
`DRDP_ROLE_LEADER` 1, `DRDP_ROLE_CLIENT` 2.  A master owns the whole software
endpoint id space and hands it out a block at a time; a leader is given a block
and asks for another when it runs dry; a client has neither and talks to one
leader.  The system messages that arbitrate all of that are the switch in
`drdp_process_system_message` — 524 instructions, a 48-entry jump table, and
the largest function in the library.

**An endpoint id is an ADDRESS.**  `drdp_address_to_endpoint` packs four octets,
the UDP port and the drdp port big-end first into 64 bits, so a hardware
endpoint id IS the socket it lives on and no lookup is needed to route to it.
A LOCAL hardware endpoint is `0xffffffffffff0000 | drdpPort`; a SOFTWARE
endpoint is a plain number in `[2048, 0xffffffff]` — `DRDP_SW_ENDPOINT_MIN` and
`DRDP_SW_ENDPOINT_MAX`, which `route_table_init` is handed verbatim.

**The system endpoint's drdp port is the low half of the CLOCK.**
`drdp_create_addr` takes `(uns16)uiTime` and steps it off the three reserved
values — `0xffff` becomes 0, `0xfffe` becomes 1, `0xfffd` becomes 2 — and that
is the whole of the local port allocation.  Two processes started in the same
millisecond collide.

**The CRC carries the protocol version.**  Every packet ends in four bytes that
are the CRC-32 of everything before them XOR'd with `drdp_crc_table[version]`;
`drdp_consume_crc_get_protocol_version` recovers the version by XORing the two
and looking the difference up in the table, searched outwards from where
version 2 sits.  A packet from an incompatible build therefore reports its
version rather than looking like corruption, and the three answers are
distinct: `-1` is a bad CRC (every connection on the route is told), a version
that is neither 2 nor -1 gets a protocol-unreachable, and a drdp port nobody is
listening on gets a port-unreachable.

**A packet's segments are walked TWICE.**  `drdp_process_udp` reads every
segment header once to check it parses and names two real endpoints, and only
then rewinds and hands each to its endpoint.  A packet that fails the first
walk is reported to every connection on the route as corrupt rather than being
half-applied.

**The route table is sent delta-coded.**  `drdp_master_send_route_table` writes
the leader ids first, then every range as two packed deltas against the
previous range's end, and the range's OWNER as an INDEX into the leader array —
with `n` meaning the master itself and `n + 1` meaning nobody.  An 8-byte owner
id costs a byte or two on the wire.  `drdp_slave_read_route_table` is the other
end and REPLACES the table rather than merging into it.

## Levers

**An inline helper's key is a CONST REFERENCE.**  The debug-checked binary
search that turns an owner id into a leader index is inlined twice, and both
copies re-read the key out of memory on every iteration — the first from a
stack temporary at `sp+32`, the second straight from `rte->owner`.  A by-value
`uns64` parameter is a register and never reloads.  Changing the parameter to
`const uns64 &` took `drdp_master_send_route_table` from 219 wrong words to 53,
and it is also what puts the caller's temporary on the stack in the first
place.

**An assert whose expression CALLS the helper keeps the result in `v0`.**  The
first of the two searches is written `VASSERT( leader_count == find_leader_index(
... ) );` — the value dies inside the assert, so it never reaches a callee-saved
register.  Written as an assignment followed by a VASSERT it lands in `s0` and
every register below it shifts.  53 wrong words to 23.

**`__LINE__` inside a multi-line macro invocation is the CLOSING line.**  That
same assert, split over two source lines to fit, reported 2195 where the target
says 2194.  Every VASSERT in a padded file has to be one line.

**A default-callback if/else puts the DEFAULT on the fall-through.**
`drdp_create_addr` substitutes `default_callback_leader_add` when the caller
passes none, and the shape is `if (pfn == 0) X = default; else X = pfn;` — not
the other way round.  60 wrong words to 5.

**Where one store sits among a run of zero stores decides the schedule.**  The
last five were closed by moving `drdp->role = role;` up ONE statement, ahead of
`drdp->master_connection = 0;`.  The load of `role` off the incoming stack slot
is hoisted two slots by the scheduler, and which store it displaces is the
whole difference.

**A buffer rebase must not zero the position first.**  `buffer.position = 0;`
written before `buffer.data += buffer.position;` deletes both adds, because gcc
constant-folds the zero.  The three statements go data, size, position.

**An aligned six-byte struct copy is a `memcpy`, not an assignment.**  A struct
assignment uses `TYPE_ALIGN` — 2 for a `drdp_addr_t` — and emits `lwl`/`lwr`;
`memcpy(&to, &from, 4)` followed by `to.port = from.port` emits `lw`/`sw` plus
`lhu`/`sh`, which is what the target has.  The unsigned halfword is the tell
that the port is a FIELD copy and not part of the block move.

**Splitting `x = c | (x << 16)` into two statements** turns a shift into a
temporary into an in-place one, and closed `drdp_address_to_endpoint`.  The
same split in its loop body was worth two more words.

**`int iType = field;` gives `slti` where the field compared in place gives
`sltiu`.**  `drdp_process_icmp` compares a `uns8` against 3 and 5; read through
an `int` local the comparison is signed, and gcc's range knowledge no longer
turns it unsigned.

**`if (bits) r = 1;` and `r = bits ? 1 : 0;` are not interchangeable.**  The
first gives `li 1` + `movz` — which is what `drdp_is_hardware_endpoint` and
`drdp_is_remote_hardware_endpoint` have.  The second gives `sltu` followed by a
`dsll32`/`dsra32` pair, because the comparison is done in DImode and truncating
it to `int` costs a sign-extension.  Phase 27's rule with the DImode half
spelled out.

**A three-arm chain whose middle two arms share a value is one `||`.**  The
block-choosing chain in `drdp_endpoint_create` is
`if (b1 == 0) i = 0; else if (b0 == 0 || b1 < b0) i = 1; else i = 0;` — with
three separate `else if`s the first test comes out the wrong way round.

**A walk that re-PEEKS is not a get_next walk.**
`drdp_reconcile_local_endpoints` calls `vtree_peek_first_in_range` on every
iteration and never calls `get_next`, because the body hands the connection to
another endpoint and takes it off the tree.  Two other walks here
(`drdp_close_connections_to_endpoint`, `drdp_invalidate_range`) cache the next
element BEFORE the body for the same reason — phase 44's rule, twice more.

## Shipped

`drdp_handle_segment_endpoint_not_local` calls `buffer_skip` on BOTH arms of a
test whose two sides are otherwise identical: the forward that used to sit on
one of them is gone and the branch is now pure cost.  `route_forward_segment`
exists in `route.cpp` and nothing in this file calls it.

Seven functions evaluate a `drdp_format_endpoint*`, a `message_get_size`, a
`connection_get_dst` or a `message_get_dst` and throw the answer away — the
arguments of traces whose calls are gone, the same residue phase 49 found in
`route_flush`.  `drdp_process_udp` calls `drdp_get_time` and discards it;
`drdp_flush` peeks the first route after the loop and asks it for its first
connection for nobody.

`drdp_create_addr` tolerates the SECOND socket failing — the ICMP socket is
optional and `num_socket` simply stays at one — but a failure of the first is
fatal, and the code that picks a socket to receive on round-robins over
`num_socket` either way.

`find_leader_index`'s two asserts quote a DIFFERENT source file from every
other assert in the object, and their line numbers (2157, 2165) are therefore
constants of a header rather than of `drdp.cpp` — which is the proof that it is
an inline function in an include and not a static in this file.  WHICH header
is not recovered: the string is a relocation site the differ masks, so only the
two immediates are load-bearing and the path in `src/drdp.cpp` is a placeholder
marked as one.

## Open

`drdp_process_system_message` 370/529 and `drdp_process_udp` 84/223 are one
phenomenon and it is stack LAYOUT.  The original gives every local a
16-byte-aligned slot in declaration order, so a two-byte `uns16` sits between
two aggregates; this build assigns all the BLKmode locals first, in declaration
order from offset 0, and then PACKS the address-taken scalars after them.  Every
`sp+N` in both functions is therefore off, and nothing about the code shape
differs.  Declaring the scalars as one-element arrays does not move them.

`drdp_get_leader_connections` 25/38 computes `ppConnections[n]` with a walking
pointer where the target recomputes `n * 4 + base` (offset first) each time.
`drdp_format_endpoint` 27/62 is three instructions short — the four `return
pName;` tails cross-jump in ours and not in the target.  `drdp_endpoint_create`
17/96, `drdp_receive_packet` 19/82 and `drdp_get_route` 9/38 are register
allocation.  `drdp_is_endpoint` 14/20, `drdp_is_local_hardware_endpoint` 4/12,
`drdp_has_endpoint` 3/24, `drdp_handle_segment` 3/63 and `drdp_create_leader`
2/87 are each one or two words of if-conversion the source spelling has not yet
reached.

## Harness

The file is 3,721 lines because the asserts say so.  The working loop for a
padded file is **collapse -> edit -> repad**: `/tmp/collapse.py` squeezes every
run of blank lines to one so a pattern match sees the real text,
`/tmp/repad.py` re-inserts exactly the blanks each `__LINE__` needs and refuses
to run if a marker would have to move BACKWARDS.  Editing a padded file
directly fails on patterns that span a padded gap, and — worse — a failed
`assert` in a multi-edit script throws away the edits that already succeeded.
Apply one edit per script.
