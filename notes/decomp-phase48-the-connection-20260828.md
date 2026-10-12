# Phase 48 — the CONNECTION, and the 4,576-byte struct the whole protocol hangs off

**2,236/2,493 -> 2,310/2,598.  One translation unit, `connection.cpp`:
105 functions written, 74 of the TU's 116 symbols byte-exact.**

Phases 43, 44 and 45 took the DRDP library from the bottom — the socket, the
buffer, the message, the endpoint, the transport. This is the layer they were
all holding up, and the one `endpoint.cpp` said in its own header comment it was
blocked on: **`connection_t`, the record for ONE conversation between two
endpoints.**  It is 4,576 bytes, the largest struct in the library by a factor
of seven, and eleven of `endpoint.cpp`'s open functions read its fields
directly.

The original is `C:/code-frontiers-gm/drdp/connection.cpp` — its own asserts say
so — and it is over 4,065 lines, which is what the source here is padded to.

## The layout

`connection_get_vnode_offset(i)` is `i * 16 + 4` and `connection_get_key_offset`
answers for exactly two, so the class carries **three** vnodes and is on three
containers at once: the endpoint's tree of connections keyed by the far id (key
0, `+0x50`), the drdp's, and the **route's** tree keyed by the desired flush
time (key 1, `+0xB4`) — which is the one `route_flush` walks in order, so the
route always services the connection that wants to go out soonest.

```
+0x0000  char        m_szTag[4]          "CONN"
+0x0004  vnode_t     m_vnode[3]          endpoint, drdp, route
+0x0034  endpoint_t *m_pEndpoint         "connection->endpoint"
+0x0038  route_t    *m_pRoute
+0x003C  void       *m_pAppPtr
+0x0040  char       *m_pAppInfo          strdup'd; "connection->app_info"
+0x0048  uns64       m_uiLocalId         "connection->local_id"
+0x0050  uns64       m_uiRemoteId        "connection->remote_id", key 0
+0x0058  char        m_szName[64]        drdp_format_endpoint's, for traces
+0x0098  uns32       m_uiInstanceLocal   a NEW one on every tx reset
+0x009C  uns32       m_uiInstanceRemote
+0x00A0  uns32       m_uiTimeInstanceSent
+0x00A4  uns32       m_uiTimeTx
+0x00A8  uns32       m_uiTimeLastArrival "connection->time_last_arrival"
+0x00AC  uns32       m_uiTimeLastInstanceArrival
+0x00B0  uns32       m_uiTimeSend
+0x00B4  uns32       m_uiDesiredFlushTime  key 1
+0x00B8  uns32       m_uiFlushBy         a 100 ms deadline, armed by an arrival
+0x00BC  message_list_t   m_sendQueue    waiting for a flush
+0x00D4  message_list_t   m_outstanding  on the wire, unacknowledged
+0x00EC  message_seqnum_t m_arrivals     by sequence number
+0x0104  message_seqnum_t m_delayed      by sequence number
+0x011C  message_t  *m_pTerminate        pre-built, delivered once
+0x0120  uns32       m_uiFlushTime[1024] when flush number N went out
+0x1120  uns16       m_usFlags
+0x1122  uns8        m_ucRttIndex
+0x1123  uns8        m_ucRxFlag
+0x1124  uns16       m_usTimeout         the LARGEST of the sixteen
+0x1126  uns16       m_usRtt[16]
+0x1146  uns16       m_usFlush           the next flush number
+0x1148  uns16       m_usArrivalNumber
+0x114A  uns16       m_usFlushAck
+0x1150  uns64       m_uiRxMask
+0x1158  uns64       m_uiArrivalHistory  all ones, so nothing looks late at boot
+0x1160  uns16       m_usTxSeqnum
+0x1162  uns16       m_usRxSeqnum
+0x1164  uns16       m_usTxAck
+0x1168  uns64       m_uiTxMask
+0x1170  int         m_iRetryTime        5,000
+0x1174  int         m_iWarnTime         30,000
+0x1178  int         m_iFailTime         60,000
+0x117C  int         m_iLastRouteFlushNumber  -1
+0x1180  uns64       m_uiCstat[11]
+0x11D8  state_channel_t *m_pChannels    248, lazily malloc'd
+0x11DC  drdp::logger_t  *m_pLogger
         0x11E0
```

**`+0x120` is not a byte buffer, it is `uns32 m_uiFlushTime[1024]`.**
`connection_record_flush_ack` indexes it with `usFlush & 1023` and reads a
32-bit time out of it, so the four kilobytes `connection_init` memsets are a
**ring of send timestamps, one per flush number**.  That is where the round trip
comes from: the far end says which flush number it saw and the difference
against the ring entry IS the sample, clamped to 0xffff.

**The timeout is the LARGEST of the sixteen samples and the reported ping is the
SMALLEST.**  `connection_add_rtt_sample` walks the ring taking the max into
`m_usTimeout`; `connection_get_ping_time` walks the same ring taking the min.
Two loops over the same array with opposite comparisons, four hundred bytes
apart.

## The state channels

`connection_allocate_state_channels` mallocs **248 channels of 60 bytes,
14,880 in one block, on first use and never again** — a connection that only
sends guaranteed traffic never pays for them at all.

A state channel is a SLOT, not a stream.  `connection_send_state` compares the
new bytes against the message already pending on that channel and **sends
nothing when they are equal**; when they are not it replaces the pending one
*wherever it is sitting*, and `m_ucWhere` is what says where that is — 0
nowhere, 1 on the send queue (so `vlist_replace`), 2 already on the outstanding
list (so append the new one and `vlist_remove` the old).  The two lists on the
channel itself, at `+0x0C` and `+0x24`, are both `message_seqnum_t` sorted by
`uns16_cmp_wrap`, so they order correctly across the sequence-number wrap.

## What the message types are, in the binary's own words

The linker put `cstat_text` next to a second string table, and `eqcalls.py`
reads both out of the relocated dump:

```
 0 discarded (old)                    6 sent
 1 discarded (no memory ...)          7 guaranteed recv (duplicates)
 2 discarded (corrupt)                8 guaranteed recv
 3 discarded (crc)                    9 state recv (duplicates)
 4 out of sequence                   10 state recv
 5 recv                                 MESSAGE_TYPE_SYSTEM      249
                                        MESSAGE_TYPE_FRAGMENT    250
                                        MESSAGE_TYPE_GUARANTEED  251
```

252 is best effort and 253 is the terminate notification; 0..247 are state
channel indices.  `connection_get_max_message_size` is where they are told
apart: **1,300 bytes for a state channel or a best-effort message, and
0x514000 — 5,324,800 — for a guaranteed one**, because a guaranteed send is
FRAGMENTED at 1,300 bytes into a run of type-250 messages each with its own
sequence number.  Five megabytes is four thousand messages.

`connection_send_guaranteed` builds the whole run into a LOCAL list first and
only moves it onto the send queue once every piece succeeded — a half-sent
message is worse than none — and `connection_move_delayed_to_arrivals`
reassembles it at the far end, holding fragments on `m_arrivals` until the last
piece lands and then copying all of them into one message.

## Three things the code does that the field map did not predict

**The terminate notification is built in `connection_create`.**  One
256-byte message of type 253, source and destination the other way round
because it is delivered as an ARRIVAL, kept for the life of the connection and
handed to `endpoint_arrival_enqueue` by `connection_terminate` with the
caller's reason memcpy'd into it.  It exists so that **a terminate can be
delivered when there is no memory left to allocate one** — which is exactly the
moment a connection is most likely to be terminated.  A software-to-software
connection skips it: `connection_is_sw_sw` is checked first and the message is
never made.

**`connection_is_valid` needs the tag AND the endpoint.**  `memcmp` against
`"CONNECTION"`'s first four bytes is only half of it; the other half is
`endpoint_has_connection`, so a connection that was destroyed and freed under a
pointer somebody kept fails here rather than passing on a stale tag.

**The typo is in the original.**  `conection_state_channel_fixup` — one `n` —
is the exported symbol name, not a transcription slip.

## Six levers

**A range check folds to ONE unsigned compare.**  `sltiu v0,a0,248` is not
`iType < 248`, which gives `slti`; it is `0 <= iType && iType < 248`, which gcc
canonicalises to `(unsigned)iType < 248`.  Both `connection_get_key_offset` and
`connection_get_max_message_size` open with one, and writing the plain form
costs exactly one wrong word in each.  (The traps file's "`sltiu` means
unsigned" is the observation; this is the source spelling that produces it.)

**A stack object declared FIRST lands at offset 0, and its address is then
free.**  The five `connection_send_uns8_*` helpers each build a payload in a
64-byte buffer.  Written `char data[64]; buffer_t buffer;` the buffer lands at
sp+64, gcc hoists `&buffer` into a callee-saved register and re-uses it across
the four appends; written `buffer_t buffer; char data[64];` the buffer is at
sp+0, `&buffer` is `sp`, and every call rematerialises it — which is what the
target does.  **Four functions went from 20, 23, 30 and 33 wrong to byte-exact
on that one swap**, and the frame shrank by 16 bytes in each.

**A result variable that SHARES a register with a parameter must be
uninitialised.**  `connection_send_internal` keeps `iType` and `iResult` in the
same callee-saved register, because `iResult` is first assigned after `iType`'s
last read.  Declaring it `int iResult = 0;` at the top makes the two ranges
overlap, costs a whole extra callee-saved register and grows the frame from 80
to 96 bytes: 41 wrong words instead of 33.  The initialisation belongs in the
first arm (`if (terminated) iResult = 0;`), not in the declaration.

**A member compared against a CALL's result wants a local.**  Phase 47 found
that the operand ORDER decides which side is loaded first; this is the same
lever with the other fix.  `if ((int16)(m_usFlushAck - message_get_flush(m)) > 0)`
evaluates the call first and re-reads the member after it; hoisting the member
into `uns16 usAck = ...;` above the `if` parks it in a callee-saved register the
way the target does.  `connection_message_get_desired_flush_time` 6/32 -> 0.

**Which of two constants ends up in the destination is decided by the
INITIALISER, not by if-versus-ternary.**  `connection_send_reset` picks 3 or 1.
`(flags & 0x20) ? 3 : 1` materialises 3 into the destination and 1 into the
scratch and selects with `movz`; `int r = 1; if (flags & 0x20) r = 3;`
materialises 1 into the destination and 3 into the scratch and selects with
`movn`, which is the target.  Phase 27's "a ternary gives `movn`, an `if` gives
`movz`" is the same observation seen from the other side — the opcode follows
from which value was written into the destination first, so the rule to apply
is *put the value you want in the destination register in the initialiser.*

**Three stores to adjacent fields are a permutation sweep.**  `connection_set_failure`
writes three ints and is four instructions long.  Only one of the six orders
matches — `iRetry`, `iWarn`, `iFail` — and the naive reading ("the delay slot
steals the LAST source store") predicts the wrong one, because the middle two
also reorder.  Phase 45's struct-field sweep, applied to statements: six builds,
twenty seconds.

And a seventh, small: `connection_send_internal`'s switch emits its three
bodies in SOURCE order (best effort, guaranteed, state) while its comparison
tree tests 251 first — phase 45's "a switch's source order is the OPPOSITE of
its test order", second sighting, and it is worth eight words.

## Shipped

**`VASSERT(!"this makes no sense")` can never fire.**  `connection_receive_list`
guards message types 254 and 255 with it, and gcc 2.95 does not fold the address
of a string literal to a constant truth: what ships is `lui`/`addiu` of the
literal's address followed by `beqz`, so the branch always takes the
assert-passed path and the message is dropped in silence.  Every
`VASSERT(!"...")` in this library is dead the same way.

**Filling the guaranteed window KILLS the connection.**  `connection_send_guaranteed`
returns -1 when `m_usTxSeqnum` is more than 4,096 ahead of `m_usTxAck`, and
`connection_send_internal` turns any non-zero result into
`connection_terminate_reason(connection, 1, 7)` — so an application that
outruns the far end's acknowledgements is disconnected rather than blocked.

**`connection_terminate` calls `message_get_size` twice** to compute one
`memcpy` length, and `connection_recalc_desired_flush_time` calls
`endpoint_get_time` twice on the closed path.  Both are visible in the object
code, which is why the source has to have them.

## Harness

**Two new `SPAN_CAP` entries.**  `state_channel_init` (640 -> 180) is followed by
the TU-local static that zeroes a `connection_t`, and `connection_get_ack_mask`
(304 -> 172) is followed by the two state-channel XOR helpers — one in place, one
into a destination.  Both caps land one word past a `jr ra` and
`test_span_caps_only_ever_shrink` checks it.

**A backgrounded `eqcheck.py -f <regex>` is still `eqcheck.py`.**  Phase 41's
rule names `_q.py`, but the hazard is the shared `decomp/build` directory and
ANY second process in it will do.  A filtered run left in the background
reported 17 phantom regressions in `vilightning.cpp` — a file this phase never
touched — and dropped the total from 2,598 to 2,568 across two consecutive
runs.  Check `ps` for a live `eqcheck` before trusting a score.

## Open

`connection_parse_segment_body` (3,064 bytes), `connection_flush` (1,392),
`connection_create_buffer` (1,304) and `connection_destroy_buffer` (928) are
**not attempted** — with the three flush-time-history codecs, the two
`message_*_load` serialisers and `conection_state_channel_fixup` they are the
save-and-restore half of the file, and they need `segment_body_t`'s parse order
settled first.

Of the forty-two written and not matching, the largest are
`connection_send_guaranteed` 63/130 (the fragment loop's rotation),
`connection_move_delayed_to_arrivals` 44/144, `connection_send_state` 42/111 and
`connection_record_guaranteed_ack` 33/84.  **Six of them are one register-class
decision each** — `connection_send_internal` (33/79), `connection_get_next_message`
(30/59), `connection_is_in_sync` (10/33) and `connection_send_instance_local`
(17/31) all have the target's exact block structure and differ only in which
callee-saved register holds `this`, or in gcc folding the last arm of an
if-chain to `sltu` where the target branches.  The fold is the standing
phenomenon: `combine` turns `if (c) x = 1;` into `x = (c != 0)` whenever `x`'s
only other reaching definition is a literal 0, and nothing tried this phase
(an explicit `else`, an uninitialised declaration, a different second test)
stops it.
