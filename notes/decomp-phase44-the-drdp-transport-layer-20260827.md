# Phase 44 — the DRDP transport layer: rate limiting, the close set, the message and the endpoint

Five more translation units out of the DRDP library phase 43 opened —
**2,078/2,305 -> 2,152/2,379**: **74 functions written, 74 byte-exact**,
**69 of them on the FIRST build**.

| TU | written | exact | first build |
|---|---:|---:|---:|
| `message.cpp`   | 27 | 27 | 22 |
| `endpoint.cpp`  | 23 | 23 | 22 |
| `bandwidth.cpp` | 12 | 12 | 10 |
| `close_set.cpp` | 10 | 10 |  8 |
| `unetutil.cpp`  |  2 |  2 |  0 |

This is the first phase in the campaign where **everything written matched**.
Two of the five TUs are complete; `endpoint.cpp` is deliberately partial and
`unetutil.cpp` is two functions out of a region whose other TUs are not started.

## Why these, and what is still open

Phase 43 settled the library's foundation — the wire buffer, the list, the set,
the red-black tree and the RLE — and left "the other 444 symbols". These five
files are the layer directly above it: what a message IS, who owns one, and the
two bookkeeping structures the connection layer sits on.

Still not attempted: `connection` (101), `drdp` (105), `route` (40), `usocket`
(14), `unet` (8), and the twelve `endpoint` functions named below.
`connection_parse_segment_body` alone is still 3,064 bytes.

## The layouts

```
bandwidth_t   48   m_fBpsLimit +0 / m_fQueueSize +4 / m_fQueueSizeMax +8 /
                   m_uiLastProcessTime +C / m_iBpsSent +10 / m_iBpsRecv +14 /
                   m_uiLastSampleTime +18 / m_iBytesSentSinceLastSample +20 /
                   m_iBytesRecvSinceLastSample +28
close_key_t   16   id (uns64) +0 / instance (uns32) +8
close_t       56   vnode +0 / vlink +10 / key +18 / time +28 / file +2C / line +30
close_set_t   60   vtree_t +0 / vlist_t +24
message_t     72   "MESS" +0 / m_link[2] +4 / m_pEndpoint +14 / m_pConnection +18 /
                   m_uiSrc +20 / m_uiDst +28 / m_uiSize +30 / m_usSeqnum +34 /
                   m_ucType +36 / m_ucRefnumOffset +37 / m_usFlush +38 /
                   m_uiCreated +3C / m_uiTime +40
endpoint_t   176   "ENDP" +0 / m_vnode +4 / m_pDrdp +14 / m_pAppPtr +18 /
                   m_uiId +20 / m_uiInstance +28 / m_iFlags +2C /
                   m_connections (vtree) +30 / m_arrivals (vlist) +54 /
                   m_closeSet +6C / +A8 / +AC
```

**The fields NAME themselves, three different ways.** `bandwidth_process`'s
four asserts quote `pBandwidth->m_iBytesSentSinceLastSample`,
`m_iBytesRecvSinceLastSample`, `TimeSinceLastSample` and `DRDP_INT64_MAX`
verbatim — so the Hungarian on this struct, the `m_` prefix, and the fact that
locals are unprefixed CamelCase are all read off the binary rather than
invented. `message_get_key_offset`'s assert is `!"unknown key index"` at line
**397** of `message.cpp`, which dates the file. And every `*_get_vnode_offset`
/ `*_get_key_offset` / `*_get_vlink_offset` function is a literal table of the
struct's own offsets, compiled in.

## What the transport actually does

- **`bandwidth_t` is a leaky bucket plus a sample window, in one struct.**
  `m_fQueueSize` is bytes owed to the wire; `process` drains it at
  `m_fBpsLimit` bytes a second and clamps at zero; `get_flush_time` answers
  "when may I send again" by dividing the OVERFLOW over the cap by the limit.
  Separately, once a second, the two byte counters become the bps figures
  `route_get_bps_sent`/`recv` report. **The elapsed-time subtraction is
  unsigned**, so a wrapped millisecond clock still gives the right interval,
  and the conversion to float is gcc's unsigned expansion (halve, round,
  double), not a `cvt.s.w`.
- **`bandwidth_get_process_interval_max` returns SECONDS, and it is
  `m_fQueueSizeMax / m_fBpsLimit`** — how long a full queue takes to drain, and
  therefore the longest a caller may go between two `process` calls without the
  bucket saturating.
- **The close set holds a closed connection's (id, instance) pair for exactly
  120000 ms.** Every record is on TWO containers at once — a tree keyed by the
  pair for the lookup, a list keyed by the expiry for the sweep — which is why
  `close_get_vnode_offset` and `close_get_vlink_offset` are separate functions.
  A record whose two minutes are already up when it is inserted is **dropped**,
  not inserted. The record also carries the FILE and LINE that closed it and
  nothing ever reads them back: they are for a debugger.
- **A message is a 72-byte header with its payload in the malloc slack behind
  it.** `message_get_data` is `this + 1`; every allocation is
  `m_uiSize + sizeof(message_t)`. There is no flexible array and no
  `m_data[1]` — that would have made `sizeof` 80 on this ABI.
- **An endpoint's "in sync" is a LATCH.** `endpoint_is_in_sync` walks the
  connections only until they are all in sync once, then sets bit 1 of
  `m_iFlags` and never walks again. It answers "has it ever caught up", not "is
  it caught up now".
- **`endpoint_find_connection` refuses a pair of software endpoints.** The tree
  finds the connection and the function returns 0 anyway when
  `drdp_is_software_endpoint` is true of both the local id and the far one.
- **`endpoint_get_instance` is a post-increment**, and `m_uiInstance` is seeded
  from `drdp_get_time` at create — so instance numbers are unique across a
  restart without any state being kept.

## Shipped, and worth knowing before writing a server against it

- **`message_create` leaves `m_ucRefnumOffset` and `m_usFlush`
  uninitialised.** The allocation is a plain `malloc` and neither field is
  written; both go out on the wire in `destroy_buffer`. The ten stores in
  `create` are the complete list and +0x37 and +0x38 are not among them.
- **`message_duplicate` copies the source's `m_pEndpoint` and
  `m_pConnection` too** — two pointers into structures the copy is not
  registered with. The caller is expected to `message_set_connection`
  afterwards, and that function re-derives the endpoint from the connection
  rather than taking it as a second argument, so the two cannot be left
  disagreeing.
- **The tag test reads freed memory on purpose.** `message_is_valid` memcmps
  four bytes, and `destroy_buffer` writes `"NULL"` over the tag BEFORE the
  `free` so the poison lands while the block is still ours. `endpoint_is_valid`
  does the same and then asks the drdp whether it still owns the pointer,
  because a freed block may keep its tag until the allocator reuses it.
- **The two timestamps in a serialised message are OFFSETS from the endpoint's
  clock**, read once per record. Phase 43 measured `buffer_append_offset_uns32`
  adding the raw byte UNSIGNED, so a stamp more than 255 ms either side of the
  endpoint's clock does not round-trip. Nothing checks that.
- **`message_create_buffer` reads the header into a stack LOCAL first**,
  because the payload's length is not known until the header is read; the
  72-byte struct assignment into the malloc'd record is the block copy the
  target unrolls four doublewords at a time.

## New levers

**A char ARRAY, not a string literal, is what makes a 4-byte `memcpy`
unaligned.** `message_create`, `message_destroy_buffer` and `endpoint_create`
all copy a four-byte tag, and the target does it with `lwl`/`lwr` into
`swl`/`swr` — alignment 1. From a string literal used in place, this compiler
aligns the constant to a word and emits a plain `lw`/`sw` pair. Declaring
`static const char MESSAGE_TAG[] = "MESSAGE";` at file scope is the difference,
and it is worth four instructions in each of three functions. This is the only
evidence in either file for how the constants were declared.

**A `slt` against zero is a comparison against a VARIABLE — the sequel.** Phase
43 found the rule; `uns32_cmp_wrap` needed its dual: `int diff = *a - *b;` then
`if (diff > 0) diff = 1; else if (diff < 0) diff = -1; return diff;` — **ONE
variable, the difference and the result sharing a register**. With a separate
`result` gcc allocates a second pseudo and every later word shifts. 7 wrong
words to 0.

**The allocation check has no house style: it is per FUNCTION.** Four functions
in this phase test a `malloc` and the target lays all four out the same way —
the failure block inline at the branch, with the `-2` in a delay slot — but
they do not get there from the same source.

- `message_create`, `message_create_buffer`, `close_set_create_buffer`:
  `if (!p) return -2;`
- `message_duplicate`: `if (p) { ...; return 0; } return -2;`

Written the other way round, each of them moves the `-2` block to the bottom of
the function and every later word shifts. Two spellings, four sites, measured
one at a time; nothing observed here predicts which a given function wants.
This is phase 7's if-direction rule reaching the error path.

**A guard that reads `>= 0` around the whole body, not an early return.**
`close_set_insert` was 15 words wrong through both spellings of
`if (expired) return 0;` and byte-exact as
`if ((int)(uiExpire - uiNow) >= 0) { ... }` with one `return 0` at the bottom —
because the early return moves the LATER `return -2` out of line.

**A three-way comparator's tail if-converts, and the polarity decides
`movz`/`movn`.** `result = -1; if (!(a < b)) result = 0;` is the `movz` every
comparator here ends on. Spelled as a ternary it comes out `movn` with the two
constants in the other registers — phase 27's rule, and `close_get_vlink_offset`
needed the ternary written the awkward way round (`iWhich == 0 ? 16 : -1`) to
get `movn` where `if (iWhich) iOffset = -1;` gave `movz`.

**Two pointer stores before the payload read, not after.**
`message_create_buffer` writes `m_pEndpoint` and `m_pConnection` BEFORE the
`buffer_readm_uns8` that fills the payload. Written after — the order the
struct suggests — the two callee-saved registers holding `ppMessage` and
`pEndpoint` swap and 12 words shift. A statement order, visible only as a
register allocation.

**A comparison's operand order survives to the `xor`.**
`endpoint_has_connection` returns `pConnection == vtree_lookup(...)`, not
`vtree_lookup(...) == pConnection`: the target's `xor v0,s1,v0` names which
operand is first. Two words.

**A validity check reuses the first test's result as the return value.**
`endpoint_is_valid` and `endpoint_has_connection` are both
`int r = <first test>; if (r) r = <second test>; return r;` — an early
`return 0` costs a separate `move v0,zero` block that the target does not have.

**A store-order pair in a zeroing constructor is a source order.**
`bandwidth_init` and the tail of `bandwidth_process` were each two words wrong
purely on which of two adjacent zero stores came first. Neither is a scheduling
artefact that can be reasoned about; both were fixed by permuting the source
and rebuilding.

**A walk that caches the next node before the body.**
`endpoint_connection_get_arrival_count` calls `vlist_get_next` BEFORE
`message_get_connection`, which a `for` loop's increment cannot produce because
calls do not reorder. Written as `pNext = get_next(...); if (...) count++;
p = pNext;` it is byte-exact.

## Harness

No new `SPAN_CAP` entries were needed. Two long symbol names in this region are
truncated at 56 characters by `eqd.py list`'s display — `message_create` and
`message_create_buffer` — but the symbol MAP carries them in full, so
`eqd.py show` and `eqcheck.py` find them; only the listing is short.

`message.cpp` is padded to put its single assert on line 397. The padding is a
documented block: the wire record field by field, the container wiring, the two
shipped hazards above, and why the tag copy is unaligned.

## Open

Nothing. Every function written in this phase is byte-exact.

## Not attempted

- `endpoint.cpp`, twelve functions: `connection_create`, `get_connection`,
  `send`, `send_reset_connection`, `handle_segment`, `connection_free_arrivals`,
  `_connection_destroy`, `destroy_buffer`, `reconcile_connections`,
  `create_buffer`, `transfer`, `close`. All of them read or write
  `connection_t` and `segment_header_t` fields directly, and those two layouts
  are settled in `connection.cpp` and `drdp.cpp`.
- `usocket.cpp` (14) and `unet.cpp` (8) are the PS2 port layer — `sceInet*`,
  `sceLibnet*`, `VIUSocketEx*` and a thread. Tractable, but every callee needs
  a prototype first, and `usocket_sendto_addr` carries a simulated-loss path
  that calls `rand` and the double libcalls.
- `route.cpp` (40), `connection.cpp` (101), `drdp.cpp` (105).
