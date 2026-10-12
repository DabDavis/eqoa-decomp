# Phase 39 — the SOUND DEVICE (`VISoundDevice`), 1841 of 2029

`visounddevice.cpp` is one translation unit, SUPPORT `0x0049A3E0` .. `0x0049D288`,
sixty symbols: the constructor, the destructor and fifty-eight methods.  **Fifty
are byte-exact, forty of them on the first build**, on the settled toolchain
(`ee-gcc2.95.3-107 -O2 -G0 -fno-exceptions`, `ps2eeas` from the `2.95.3-114`
package) with no re-tuning.  Tree total 1792/1965 -> **1841/2029**.

The class is the EE half of the sound system: 46 playback records, 128 loaded
`VISound`s, two first-fit allocators (SPU RAM and the IOP's XM region), one SIF
DMA in flight, and the BGM cross-fade.  It issues **no** `sceSpu` call of its
own — every operation is appended to a packet by a file static and shipped over
SIF RPC — which is why the whole class is ordinary C++ over `VILList`.

## The five statics, and a new `SPAN_CAP`

`DebugResources__13VISoundDevice` is the last named symbol in the file, and its
gap-derived span is 936 bytes.  The real function is **308**; the 628 bytes after
it are five unnamed file statics — bind, call, flush, the packet append and the
packet search.  `eqdecomp.SPAN_CAP` now caps it — and the canary insisted on 308 rather than
312: a cap has to land exactly one word past a `jr ra`, i.e. take the delay
slot and not the assembler's padding nop.

The append helper is what every method actually calls:

    static int   gSndArgs[16];      /* one command's arguments   */
    static int   gSndPacket[128];   /* [0] = count, then {id, argc, args...} */
    static int   gSndWords;
    static void  SoundCmd(int id, int argc);
    static int  *SoundFindCmd(int id);   /* merge into a queued command */

`StopChannels` is the only caller that searches: a stop already queued this
frame is OR'd into rather than re-sent.

## Levers

**`BgmBufferSize` is NOT static and `SetBgmVersion` is.**  Both read the same
file-scope `VISoundDevice::sBgmVersion` and both mangle without a `this`
(`BgmBufferSize__13VISoundDevice`), so the declaration cannot be read off the
name.  The CALL says it: `Init` and `Reset` do `jal BgmBufferSize` with
`move a0,s1` in the delay slot, and `SetBgmVersion` stores **a0**, its first
argument.  Declaring `BgmBufferSize` static costs one instruction at each of
four call sites and, in `Init`, shifts a branch offset — which is how it was
found: `Init` came out one instruction SHORT with everything else identical.

**A float stored into an `int` is `cvt.w.s` + `mfc1`; into an `unsigned int` it
is a CALL to `fptoui`.**  Every volume word this class sends is unsigned and
every pan word is signed, in all four of `Play`, `SetPitch`, `SetVolume`,
`SetPan`.  Getting one wrong is a whole instruction plus a register class.

**The stereo gate is an `if` on the LOCAL, and the two directions are different
functions' shapes.**  `SetVolume`/`SetPan`/`Play`'s XM arm want

    if (m_stereo == 0)
        pan = 0;
    gSndArgs[3] = pan;          /* movz */

and `Play`'s WAVE arm wants

    int p = 0;
    if (m_stereo)
        p = pan;
    gSndArgs[6] = p;            /* movn */

An `if`/`else` with two STORES gives a branch-likely and two stores; a ternary
gives the other conditional move.  This is Phase 27's rule applied to the
DESTINATION rather than the test, and it took `SetVolume` and `SetPan` from 53
and 57 wrong words to byte-exact in one edit each.

**The clamp is `VIMIN(VIMAX(lo, v), hi)`, not `VIMIN(hi, VIMAX(lo, v))`.**  Both
spellings emit the same six instructions in the same order; only the operand
order of the first `c.lt.s` differs (`v < 1.0` against `1.0 < v`).  Phase 23
found that VIMIN's argument order decides the `slt`; here it decides a `c.lt.s`
inside a nested macro, and the outer macro's FIRST argument is the inner one.

**The `-1` sentinel is a signed `int`.**  `m_xferSpuAddr`/`m_xferXmAddr` are `int`
and `= -1` is one `li`; declared `unsigned int` the same statement is
`lui 0xffff` + `ori 0xffff`, because gcc materialises a 32-bit unsigned constant
as a zero-extended 64-bit value on this ABI.  Two words, and it decided the
whole schedule of both `Begin*Transfer` leaves.

**The same integer expression is 32-bit in one function and 64-bit in another.**
`SetPitch` computes `(w->SampleRate() << 12) / 48000` with `sll` + a real `div`;
`Play` computes the identical thing with `dsll` + `__divdi3`.  So one is written
on an `int` and the other on a `long` — measured, not chosen.

**Phase 7's if-direction, five more times.**  `Process` and `Flush` both want
`if (m_xferIdle == 0) IdleTransfer(); else { scan }` — the idle path in the
`else` — and inverting it costs 30 and 62 words.  `ReleaseSound` wants
`if (--s->m_link <= 0) { … }` with the body nested rather than
`if (… > 0) return 0;`, which is Phase 13's nesting: the early return then
shares the function's own `return 0` instead of branching to a private copy.

**The type test is TWO `if`s in four functions and an `if`/`else if` in three.**
`Process`'s playback sweep, `SetPitch`, `SetVolume` and `SetPan` all call
`Type()` a SECOND time after the WAVE arm has run; `Stop`, `Play` and the upload
scans do not.  In `SetPitch` it decides the frame size: written as `else if`,
the `pitch` parameter is dead in the WAVE arm and gcc drops a callee-saved
register (80-byte frame against the target's 112).

**A shared address base is what puts a search on one walking pointer.**  The XM
sweep in `Process` reads two words 4 bytes apart, eight times, stride 8.  Two
independent subscripts (`gSndXm[8 + j*2]`, `gSndXm[9 + j*2]`) give gcc two
induction variables and an `sll`+`addu` per access; one base
(`int *p = &gSndXm.m_ch[0][1]; … p[-1], p[0]; p += 2;`) gives the target's
`addiu v1,s4,36` / `lw v0,-4(v1)` / `addiu v1,v1,8`.  And the loop must be a
`do`/`while`: a `for` over a constant bound peels its first iteration and reads
the first pair off constant displacements.

**`Reset` is not `Init` with a prefix.**  The same eighteen-statement field block
appears in the constructor, `Init` and `Reset` with three different subsets:
the constructor has no `m_stereo`, no BGM names and no class-volume loop; `Reset`
has everything except `m_handleSeed = 0x16062` and the trailing `m_xferFree = 0`
(it already cleared the transfer at the top).  Each omission is one instruction,
and Phase 16's hoist is what reads the subset back: the constant `1` that gets
hoisted is `m_stereo`'s in `Init` and `m_xferIdle`'s in the constructor, so the
constructor's block cannot contain an `m_stereo` store.

**`m_xferSpuAddr += m_xferLast` precedes its `SoundCmd`.**  Written after the
call — which is what the emitted order looks like — gcc cannot hoist the two
loads above the `jal`, and the add comes out three instructions later.  Written
before, the store falls into the call's delay slot exactly as the target has it.

## Layout

    class VIPlayback : public VILNode      /* 56 bytes */
        unsigned long m_channels;   +0x10
        int  m_priority, m_sound, m_handle, m_pitch;   +0x18..+0x24
        float m_volume, m_pan;      +0x28 +0x2C
        int  m_frame;               +0x30
        VISOUNDCLASS m_class;       +0x34

    class VISpuBlock : public VILNode      /* 24 bytes */
        unsigned int m_addr;  int m_size;              +0x10 +0x14

`VISoundDevice` is at least `0x117C` bytes: `unsigned long m_channels` at +0,
`VIPlayback m_playbacks[46]` at +8, `VISound *m_sounds[128]` at +0xA18, the frame
counter at +0xC18, `VIPlayback *m_playbackSlots[46]` at +0xC1C, six `VILList`s
(+0xCD4 active, +0xCE0 free, +0xCF0/+0xCFC the SPU memory and node lists,
+0xD0C/+0xD18 the XM pair) with the two `VISpuBlock[128]` arrays at +0xD08 and
+0xD24, the transfer state +0xD34..+0xD5C, `float m_classVolume[2]` at +0xD60,
`m_stereo` at +0xD68, **four 256-byte name buffers** at +0xD6C (two request
slots), +0xF6C (playing) and +0x106C (pending), and the BGM fade at +0x116C.

`sizeof(VIWave)` is **92** and `sizeof(VIXm)` **64**, read straight off
`CreateSound`'s two `__builtin_new` arguments — the first independent
confirmation of Phase 17's two layouts.  `VIXm`'s +28/+32 are the module and
sample cached flags (`vi.h`'s `m_p28`/`m_p32`, now named), +20 is the sample's
SPU address, +24 the module's, +40 the module size, +44 the sample pointer and
+48 the sample size, all read off `Process`'s two `Begin*Transfer` calls.

**The playback handle is packed exactly as `VISpellEffectSystem`'s is** — the
slot in the low byte, a serial above it, the serial seeded at **0x16062** and
masked with `0x7FFFFF00` — and `Playback()` needs the same `(handle & 0xff) % 46`
rather than a cast, so the remainder comes out `div` and not `divu`.  Two
unrelated classes, the same idiom and the same magic number.

## Shipped

- **`EnableBgmFading` is empty** (`jr ra; nop`) — the fade is unconditional.
- **`SetBgmVolume` ignores its slot**: it writes the one master volume whatever
  the index, so `SetSoundClassVolume`'s closing
  `SetBgmVolume(0, m_bgmVolume); SetBgmVolume(1, m_bgmVolume);` is a pair of
  self-assignments.
- **`IsBgmPlaying` ignores its slot** too, answering for the one running track.
- **`Stop` returns nothing** — no path writes `v0`.
- `CloseXmMem` nulls `m_xmBlocks`; `CloseSpuMem` does not null `m_spuBlocks`.
- The XM allocator **floors every request at 0xC000 when `sBgmVersion == 1`**,
  and `FreeXmMem` applies the identical floor, so the two agree by construction.
- `AllocXmMem` prints on entry, prints the remaining free total on success and
  prints on failure; `AllocSpuMem` prints only on failure.
- `Process` starts **one** transfer per frame and restarts the 128-entry scan
  from zero every frame, so a sound low in the table waits for every sound above
  it.  `Flush` is the same loop run to completion.
- `Reset` stops every XM playback by name before `StopAllBgm`, then deletes all
  128 sounds without clearing the slots — the later re-init loop does that.

## Open

Ten, none structural.  `Process` 65/190 and `Play` 33/250 are the standing
register-allocation phenomenon plus two `gcse` constant hoists (mine parks
`0xffffff` and `1` in callee-saved registers where the target rematerialises);
`StreamBgm` 28/118 is one layout question — the target places BOTH fade arms
after the three-test dispatch and every if-chain spelling tried puts the first
arm inline, and a `switch` gives gcc's balanced tree (`beqz`, `bgtz`) instead of
the target's linear `beqz`, `beq -1`, `beq 1`.  `SetPitch` 22/89 and
`AllocateChannels` 17/103 are two-register swaps; `DebugResources` 15/77,
`AllocSpuMem` 13/47, `IdleTransfer` 7/117 and the two `Begin*Transfer` leaves
(5/13, 12/13) are instruction-order noise around one constant materialisation.
