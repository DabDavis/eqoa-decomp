# Phase 45 — the socket layer

*2026-08-27.  `decomp/src/{uassert,uerror,ipaddress,unet,usocket}.cpp` plus one
function added to `unetutil.cpp`.  33 named functions written, 32 byte-exact.
Five file statics were written as well; they show as `NO-TGT` rows, the way this
tree's other sixteen unnamed statics have since phase 39, so they count in the
denominator and can never count in the numerator.*

Phases 43 and 44 took the DRDP library from its containers up through the
message and the endpoint.  This phase goes the other way — **down**, to the floor
the whole stack stands on: `SUPPORT 0x004acc90..0x004ae088`, five more
translation units of the same C-compiled-as-C++, holding the assert every
`VASSERT` in the tree lands in, the OS error remap, the address codec, the PS2
network bring-up, and the socket itself.

| TU | span | written | exact | first build |
|----|------|---------|-------|-------------|
| `uassert.cpp`  | `0x004acc90..0x004acdb8` | 3  | 3  | 2 |
| `uerror.cpp`   | `0x004acdb8..0x004ace90` | 4  | 4  | 3 |
| `ipaddress.cpp`| `0x004ace90..0x004acf58` | 4  | 4  | 4 |
| `unet.cpp`     | `0x004acf58..0x004ad3f0` | 8  | 8  | 3 |
| `unetutil.cpp` | `0x004ad3f0` (one added) | 1  | 1  | 1 |
| `usocket.cpp`  | `0x004ad480..0x004ae088` | 13 of 14 | 13 | 5 |

The one open function is `usocket_sendto`, and it is two words of PROLOGUE
SCHEDULE — `move s2,a2` and `sq ra,112(sp)` in the other order, with every other
instruction identical.  Nine spellings (a named result, a named length, a named
pointer, the pointer arithmetic hoisted, the parameters reassigned, the call
result compared) all score exactly 2/33, which is what settles it as the
standing `sched2` family rather than a source question.

---

## What the layer does

**`ipaddress_t` is SIX bytes** — four octets and a `uns16` port — and it has
three different encodings around it, which is the whole of `ipaddress.cpp`:

* SCE's generic `sceInetAddress` carries the IPv4 as a host-order word at +4, so
  `sceaddr_to_ipaddress` and `ipaddress_to_sceaddr` count DOWN through it: octet
  0 is byte 7.  Neither of them touches the port.
* the WIRE form is the four octets in order followed by the port **little-endian
  first**, which is *not* network order.  `ipaddress_write` splits the port with
  a `lbu` and a `lhu`+`srl`; `ipaddress_read` reassembles it with two stores to
  the same halfword, the first of them dead — through a pointer, so it survives.

That six-byte form is what `usocket_sendto` and `usocket_recvfrom` prepend to
every datagram: they are the entry points the DRDP route layer calls, and every
size on that path moves by six.

**`unet.cpp` is the only PS2-specific file in the library.**  The whole of
"start the network" is one EE thread — `sceSifMInitRpc`, `sceLibnetInitialize`
over a `memalign(64, 0x10200)`, a handler registration, and a blocking
`sceLibnetWaitGetAddress` — and the local address it comes back with is cached
as a **STRING**, `g_szHostName[128]`.  Nothing keeps an `ipaddress_t`, which is
why `unet_get_local_address` asks `unet_gethostname` for the text and then
resolves its own name through `sceInetName2Address`.  The three failure paths
print and **hang**; so do both asserts.  The dump this was read out of still had
`192.0.2.100` sitting in that buffer.

**`usocket_t` is 200 bytes and this file owns eleven of them** — the four-byte
tag, the sceInet descriptor, the type, a listen flag and one byte of readiness
bits (bit 0 readable, bit 1 writable, each cleared by the operation that fails).
The other 189 belong to `VIInitUSocketEx`: the port does not talk to sceInet from
the caller's thread for datagrams at all, it hands the work to a pair of EE
threads living in the back of the same allocation.  `usocket_recv`, the stream
path, is the one exception.

### `forward` — a lag-and-loss simulator, shipped

The third global in this region is an `ipaddress_t` called `forward`, and it is
what makes `usocket_sendto_addr` a 110-instruction function.

* the first thing it does is `rand() * 100.0 / 2147483648.0` and compare that to
  the caller's `iDrop`.  Over, and the packet is silently dropped.
* then `iDelay + (int)((float)iJitter * (float)rand() / 2147483648.0)`.  If that
  is non-zero it does **not** send the packet to its address.  It `alloca`s
  `iSize + 8`, copies the payload in, and appends an eight-byte trailer — the
  real destination's four octets, its port in NETWORK order, the delay in 32 ms
  units clamped to 255, and a zero — and sends the whole thing to `forward`.
* `usocket_recvfrom_addr` is the other end: anything that arrives FROM `forward`
  (four octets and the port both compared) has the same trailer stripped off its
  tail, `*piRead` walked back twice by four, and the real source address put back
  in the caller's `ipaddress_t`.

So a relay somewhere re-sent the packet after the requested delay.  The in-world
dump still holds **64.37.152.90:10164** in that global.

`usocket_select` returns 1 unconditionally.  `usocket_accept` asserts
`!"usocket_accept: not implemented"` — and then goes on to create a socket
anyway, which nothing on a console ever reaches, because the assert hangs first.

### The error table names an enum nobody mapped

`uerror.cpp` carries the whole SCE INET error enum as strings —
`sceINETE_TIMEOUT` through `sceINETE_NO_ROUTE`, -500 to -514, fifteen entries —
and `uerror_remap_os_error_code` maps exactly **two** of them: a refused
connection to -6 and a timeout to -4.  Everything else is -1, and the default arm
still runs `get_errno_constant` and **throws the answer away**.  Whatever printed
it is gone; the call for it was left behind.

---

## Harness: a function that never returns lost its last instruction

`trim_padding` finds the last `jr ra` and keeps one word after it, because the
delay slot is part of the function.  `_uassert` and `_uassertf` end in
`for (;;) ;` and `_uassert` **has no `jr ra` at all**, so the fallback rule —
strip trailing zero words — ate the delay slot of the `b` that closes the loop
and reported a byte-exact function as one instruction short.

The fix is the same argument the `jr ra` case already makes: a terminating branch
owns the word after it.  `eqdecomp.is_branch` is new and narrow (the rule only
runs when there is no `jr ra` in the span at all), and
`test_trim_keeps_a_terminating_branchs_delay_slot` pairs it with the control that
the ordinary case still loses every trailing zero.

Three new `SPAN_CAP` entries, all the usual cause — `usocket.cpp` has five file
statics and the symbol map names none of them, so `usocket_is_valid` (three
statics, 536 bytes derived), `usocket_destroy` (one, 296) and `usocket_sendto`
(one, 504) each acquire a span reaching into the next.

---

## New levers

**A switch's source order is the OPPOSITE of its test order, and both halves
matter at once.**  `uerror_remap_os_error_code` tests -511 first and lays the
-500 body out first.  Phase 27 had the rule in two pieces — comparisons ordered
by VALUE, bodies by SOURCE — and this is the first function where they point in
opposite directions, so the source has `case -500:` above `case -511:`.  Written
as an if/else chain it is 6 or 7 words wrong in *either* direction; written as a
switch the other way round, 4.

**The align-up needs an unsigned divide AND a fresh local.**  `(x + 15) / 16 *
16` on a signed `int` brings gcc's round-toward-zero correction (`slt`, `movn`,
`sra`) — three extra instructions.  Cast to `unsigned` and it is `srl`+`sll`, as
the target has.  But `((unsigned)g_aStack + 15) / 16 * 16` written in place lets
gcc fold the `+ 15` into the symbol's relocation ADDEND, and the whole
`addiu v0,v0,15` disappears; assigning the address to an `unsigned` local first
is what keeps it.  (Phase 40's align-up lever is the `andi` idiom; this is the
shift one, and it wants the same treatment for the same reason.)

**A constant that serves two purposes is materialised once.**
`unet_startup_async_status` returns **1** when the startup thread is not yet
dormant — and it is the same `1` that then goes into `g_iStarted`, emitted into
the branch's delay slot and reused.  Reading the function as
`iStatus = ReferThreadStatus(...); if (iStatus != 16) return iStatus;` costs two
`move`s and does not match, even though it is the more natural code.

**Five struct-field assignments are a permutation, and there is exactly one.**
`unet_startup_async` fills five of `ee_thread_t`'s nine fields and the stores
come out in the order 16, 12, 4, 8, 20 — thoroughly rescheduled.  All 120 orders
score: one is 0, the next is 2, and the source order is
`stack_size, gp_reg, func, stack, initial_priority`.  Worth doing mechanically
rather than reasoning about; the whole sweep is under four minutes.

**A two-byte copy is a `memcpy`, not two `char` assignments.**  Both ends of the
`forward` trailer move the port through a `uns16` local a byte at a time, and the
target's shape is `lb`, `lb`, `sb`, `sb` — both loads, then both stores, in
offset order.  That is gcc's inline block move.  Writing it out as
`((char *)&usPort)[0] = p[i];` gives `lbu`, reverses the pair, and takes a
different register class with it; `memcpy(&usPort, p + i, 2)` is byte-exact and
fixed 15 wrong words in one edit.

**The byte swap needs its mask to order the shifts.**  `(x << 8) | (x >> 8)`
emits `sll` before `srl` no matter which way round the `|` is written; the target
emits `srl` first.  `((x << 8) & 0xff00) | (x >> 8)` does it, and so does
`(x * 256) | (x / 256)`; five other spellings do not.  The masked form is the
one that reads as a hand-written `htons`, and it is the same macro at both ends
of the wrap.

**A copy gcc can coalesce away has to be made to CONFLICT.**  This is the lever
that took `usocket_sendto_addr` from 31 wrong words to 1.  The function needs a
send pointer that starts as `pData` and becomes the `alloca`'d buffer, and
written the obvious way —

```c
const void *pSend = pData;
...
memcpy(pBuffer, pData, iSize);
pSend = pBuffer;
```

— gcc puts both in ONE register: on the wrap path the initial copy is dead, and
on the other path the two are the same value, so `regmove` ties them.  The target
keeps them apart, in `s4` and `s7`, and pays for it by **spilling the socket
pointer to the stack** (`sw a0,4(s8)` … `lw a0,4(s8)`), which is nine live values
into eight callee-saved registers.  Swapping two statements —

```c
pSend = pBuffer;
memcpy(pBuffer, pData, iSize);
```

— makes `pSend` take its new value while `pData` is still live, the ranges
conflict, the copy survives, and the spill follows.  Every register in the
function falls into place with it.

**A drop test returns its own comparison.**  `if (rand_pct() < iDrop) return 1;`
makes gcc materialise the 1 in the branch's delay slot.  The target's delay slot
is a `nop`, because the value it returns *is* the `slt` result:

```c
iLoss = (int)(rand() * 100.0 / 2147483648.0) < iDrop;
if (iLoss)
    return iLoss;
```

**`goto done` is a shape, not a smell.**  `usocket_create_internal` has four
exits into one cleanup and every one of them is a forward `goto`; the first guard
is `if ((unsigned)iType >= 4) { iResult = -1; goto done; }`, which puts the error
INLINE and the body out of line.  The `if (iType < 4) { … } else iResult = -1;`
form branches the other way, adds a jump over the else, and swaps which of
`pSocket`/`iResult` gets `s0`.  All 41 wrong words went at once.

---

## Padding, again

`usocket.cpp`'s five asserts land on lines **553, 729, 945, 1116 and 1208** — an
immediate each, so the file has to be padded to the layout it shipped with.  That
is roughly 1,300 lines for nineteen functions.  Phase 43 found `vtree.cpp` at
215/283/331/365 and phase 27 established the rule; what is new here is the
*ratio*.  Nineteen small functions do not fill 1,300 lines, so the original
carried a great deal more than this — most likely the Windows and the PS2 back
ends side by side under an `#ifdef`, which is also the only thing that explains a
`usocket_select` that returns 1 and an `accept` that is a hang.

## What is left

Not attempted: the `viusocket.cpp` TU immediately after this one
(`VIUSocketRecvThread`, `VIUSocketSendThread`, `VIUSocketRecv`, `VIUSocketSend`,
`VIInitUSocketExApi`, `VIFreeUSocketExApi`), and the same three bodies phase 44
left — `connection` (101 functions), `drdp` (105), `route` (40) — plus the twelve
`endpoint` functions that read `connection_t` and `segment_header_t` directly.
`pf`/`vpf`/`dpf_set_print_mask` (`0x004be120..0x004be1f0`), the library's own
printf, is a small TU on its own and is called from `unet.cpp` for two of its five
messages where libc `printf` serves the other three.
