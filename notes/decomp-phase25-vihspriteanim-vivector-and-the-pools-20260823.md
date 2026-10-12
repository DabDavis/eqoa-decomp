# Decomp phase 25 — the animation clip, `VIVector<T>`, and the value pools

**883/985 → 1004/1098.**  `VIHSpriteAnim` 19 of 19, `VIVector<T>` 27 of 28
across four instantiations, `VIHSprite`'s twenty-six readers 25 of 26, and the
three VALUE `VIPool<T>` instantiations `VIHSprite` owns — on the settled
toolchain (`ee-gcc 2.95.3-107 -O2 -G0 -fno-exceptions`, assembled with SN
`ps2eeas` out of the `2.95.3-114` package) with no re-tuning.

One finding in the pools reached BACKWARDS into phase 12: `VIPool<T>::Add`'s
node address is an INTEGER add with the offset on the left, and that one word
was the whole of the `Add is one word of operand order` miss recorded in phase
12 for all thirteen POINTER instantiations.  They are byte-exact now.

This is the fourth class group whose semantics were settled from the console
first — `go-eqoa-vi/vianim` reads both animation frame formats and the whole
playback pool out of a live game — so every miss here was codegen, and twice
the Go port and the decomp checked each other.

## `VIHSpriteAnim` — one animation clip

SUPPORT 0x0044D258..0x0044D828.  The third of the engine's carve-one-block
containers after `VICollBuffer` (phase 13) and `VISurface` (phase 16):
`CalcDataSize` writes byte OFFSETS into the three section pointers, `Allocate`
takes one `new char[]`, and `Init` rebases all three onto `m_data` in place.

```
  +0x00 m_id  +0x04 m_userData      (VIBuffer)
  +0x08 m_format      +0x0C m_numNodes
  +0x10 m_numFrames   +0x14 m_numTriggers
  +0x18 m_playRate    +0x1C m_playSpeed
  +0x20 m_playbackType
  +0x24 m_nodes       +0x28 m_frames      +0x2C m_triggers
  +0x30 m_dataSize    +0x34 m_data
```

Two shipped bugs, both matched as written:

* **`GetPlaySpeed` returns the play RATE.**  `m_playSpeed` is at +0x1C, which
  `SetPlaySpeed` writes; the getter reads +0x18.
* **`Clear` never zeroes `m_numTriggers`.**  Ten fields are reset and +0x14 is
  not one of them, so a cleared clip keeps the previous trigger count.

### `m_frames` is read BEFORE the guard, and the frame index accumulates INTO the parameter

`SetNodeFrame` is fourteen instructions of addressing in front of a copy, and
two source facts are worth seven words each:

```c++
    char *fb = m_frames;                       /* BEFORE the format test */
    if (m_format != VIHSpriteAnimFormatFrame)
        return -1;
    VIHSpriteAnimNode *n =
        (VIHSpriteAnimNode *)(node * 8 + (unsigned int)m_nodes);
    frame += n->m_firstFrame;                  /* into the PARAMETER */
    VIHSpriteFrame *d =
        (VIHSpriteFrame *)(frame * 32 + (unsigned int)fb);
```

The `lw` of `m_frames` sits in the FIRST basic block, ahead of the branch,
where no scheduler could have put it — gcc 2.95 does not speculate loads across
a conditional.  And the index accumulation is phase 9's
assign-then-modify-in-place: a fresh temp costs the whole index chain its
register assignment.  Together they took the function from 13 wrong words to 0.

### A local declared INSIDE each case — 22 of 35 words

`CalcDataSize` is a two-arm switch over the frame format whose arms differ only
in the element size.  Both the sizing statements AND the running offset go
inside each case:

```c++
    switch (m_format)
    {
    case VIHSpriteAnimFormatFrame:
    {
        int off = m_numNodes * 8;
        m_frames = (char *)off;
        m_nodes = 0;
        off += m_numNodes * m_numFrames * 32;
        m_triggers = (VIHSpriteKeyframe *)off;
        m_dataSize = off + m_numTriggers * 8;
        break;
    }
    ...
```

Written with the tail after the switch, the two arms cannot hold their own
`sll` and the whole tail changes shape (22 wrong).  Written with the tail
inside but `off` SHARED at function scope, the instruction stream is identical
to the target and every register is different (also 22 wrong).  Only the
per-case local produces the target's allocation — phase 14's "a local declared
INSIDE the case", now with the register allocator as the thing it decides.

### The two frame formats are NOT the same shape

```c++
class VIHSpriteFrame     { VIQuat m_rot; float m_scale; VIVect3 m_trans; };   // 32
class VIHSpritePackFrame { VIVect4S16 m_rot; VIVect4S16 m_scaleTrans; };      // 16
```

The unpacked frame keeps the scale and the translation apart — `SetNodeFrame`'s
copy proves it, splitting at +0x14 with a three-wide run — and the packed one
does not: its second four shorts are (scale, x, y, z), which is exactly what
`go-eqoa-vi/vianim` reads off the console.  Both are quantised by the same
1/512, and `GetNodeFrame` walks those four as ONE run on both sides:

```c++
        float *d = &f.m_scale;
        short *p = &s->m_scaleTrans.x;
        ...
        d[0] = p[0] * (1.0f / 512.0f);
```

That run is phase 9's pointer lever: the first of the four folds back onto the
outer base (`16(a3)` / `8(v1)`) and the other three come off the pointer.
Written as four member assignments both pointers disappear — ten words wrong.

## `VIVector<T>` — the third template, and the only one that grows

Four instantiations (`VIHSpriteNode`, `VIMatrix44`, `VISceneDisplayElem`,
`int`), 28 symbols, one source.

```c++
template <class T> class VIVector
{
public:
    int m_size;         // +0x00   live elements
    int m_capacity;     // +0x04   allocated elements
    T  *m_data;         // +0x08
};
```

`Allocate()` is the growth step and is written for a vector that already holds
elements: a SECOND raw block, copy-construct the live ones across, destroy and
free the first, install the second.  Both `Add`s return the INDEX they added at
— phase 24's `VIClipStack::Push*` idiom, spelled the same way.

### `m_data = 0` is OUTSIDE the `if`, and two delay slots say so

`Clear()`'s two branch-likely delay slots each hold `sw zero,8(s0)` — a copy of
the join block's first instruction, stolen by reorg, which it can only do when
the join really is on both paths.  So the store is not inside the guarded
block, and phase 13's hoist then puts it first, which makes it the source's
LAST statement.

### One `return 0` at the bottom of `Allocate`

With a `return 0` written into the first arm, gcc if-converts that arm's null
test to `movn` and the two exits stop cross-jumping; the cost is four words at
the top and three more at the bottom, where the `DestroyPool() < 0` test stops
reaching the shared epilogue.  Phase 12/13's lever, a third time.

### `m_trans.Zero()`, not three component stores — 59 of 129 words

`VIHSpriteNode`'s constructor is inlined into
`VIVector<VIHSpriteNode>::Add()`, and its three `VIHSpriteFrame` members are
each constructed twice: the member zeroing constructors, then the identity
body.  In that body the translation is `m_trans.Zero()` — a member call, which
materialises `&m_trans` and folds its +0 back onto the frame's base.  Written
as three component stores all three land on the ROTATION's base register.

`VIHSpriteNode` itself is 256 bytes and every field of it is read off that one
inlined constructor:

```
  +0x000 m_parent -1  +0x004 m_child -1  +0x008 m_sibling -1  +0x00C 0
  +0x010 VIHSpriteFrame  +0x030 VIMatrix44
  +0x070 VIVect3         +0x07C VIHSpriteFrame
  +0x0A0 VIMatrix44      +0x0E0 VIHSpriteFrame
```

The two matrices are double-constructed the way phase 24's `VIRadialFloraInst`
is (VIVect4[4]'s constructors, then `VIMatrix44`'s own VU0 `vsub` + four
`sqc2`), and the four ints are written LAST — read off phase 16's hoist, which
puts the last store of each distinct constant at the head of the block.

## `VIHSprite`'s readers — and the OTHER address spelling

Twenty-six functions: the node-tree walk, the trigger table, the attachment
list and the play accessors.  Two shipped bugs here as well:

* **`Parent(i)` is inverted.**  It answers `m_nodes.m_data[i].m_parent` only
  for node 0 — whose parent is nothing — and returns `End()` for every other
  node.
* **`Prev(i)` walks node i's OWN child chain**, where the previous sibling of
  `i` lives under `i`'s parent.

### Every indexed reader names the ELEMENT through a pointer

```c++
    VIHSpriteNode *n = &m_nodes.m_data[i];
    if (n->m_child != -1)
        return n->m_child;
```

`m_data[i].field` puts the address sum in the INDEX's register
(`addu a1,a1,base`); the target puts it in the base's (`addu v1,v1,a1`).  Three
words of five, in every one of them.

This is the interesting half of the phase: `VIHSpriteAnim` and `VIVector` need
phase 24's INTEGER form (`i * sizeof(T) + (unsigned int)p`, offset first) for
exactly the same shape of access, and `VIHSprite` needs the pointer form.  The
two spellings sit side by side in one phase and the object code separates them
every time.

`FindTrigger` shows the lever one level up: the CONTAINER is named through a
pointer, so `m_size` folds back onto `this` (`88(t0)`) while `m_data` comes off
the pointer (`4(t1)`).

### Phase 10's guarded do/while, in `Last`

Written as a `while`, the sibling is read TWICE per turn, gcc keeps both loads
and the loop comes out as three copies of its body — 24 of 37 words.  The
guarded form reads it once:

```c++
    int k = n->m_child;
    if (k != -1)
    {
        do
        {
            j = k;
            VIHSpriteNode *m = &m_nodes.m_data[j];
            k = m->m_sibling;
        }
        while (k != -1);
    }
```

## The VALUE pools, and the word that reached back to phase 12

`VIPool<T>` was matched in phase 12 for thirteen POINTER instantiations.
`VIHSprite` owns three VALUE ones — `VIHSpriteAttachment` (8 bytes),
`VIHSpritePlay` (96) and `VIHSpritePlayNode` (44) — which need three members
the pointer ones never ask for: `Prev(int) const`, `Add()` (the
default-construct form), and `Init(const VIPool<T> &)`.

**`Add`'s node address is an integer add with the offset on the left.**
`m_nodes + m_free` canonicalises base-first however it is written; the cast to
`unsigned int` does not.  That is phase 24's `VIArray` lever again, and it
closed the one `addu` phase 12 left open in ALL THIRTEEN pointer
instantiations at the same time.

**The allocation is a RAW char block here too.**  Invisible for every pointer
instantiation, where the Node has no constructor and both spellings emit the
same words, and worth 53 of 125 in `VIPool<VIHSpritePlayNode>`, whose element
carries a `VIHSpriteFrame`: `new Node[n]` drags gcc's own vec-init loop in
front of the copy.

**`VIHSpritePlay`'s three 64-bit clock fields are load-bearing at COMPILE
time.**  They give the class alignment 8, and without them gcc guards the
104-byte node copy in `Allocate` with a runtime `(src|dst)&7` test that the
target does not have — 48 of 100 words.  The field map is `vianim`'s, read off
a live console, and this is the decomp confirming it by a route the console
could not: an alignment.

**`VIHSpriteFrame` has a user `operator=`.**  The implicit one copies `m_rot`
with a 16-byte `ldl`/`ldr` block move; the target copies the rotation and the
scale as five floats off one pointer and the translation as three off another.
`VIHSpriteAnim::SetNodeFrame` still block-moves the rotation, so it is written
out by hand there (`d->m_rot = f.m_rot;`) rather than as `*d = f`.

**`Init(const VIPool<T> &)` uses ONE index for both walks**, declared at
function scope: the free-chain loop then keeps it in the same callee-saved
register the live walk used.  Five words in the smallest instantiation, and the
difference between three matches and none.

## Harness

Fourteen new `SPAN_CAP` entries — `VIHSpriteAnim::Allocate` (72 bytes derived
2584, the TU closing with an unnamed `VIMap<int,int>` instantiation), six for
`VIVector` and seven for the value pools.  Same static-twin picture phase 24
described.

## Open

`Allocate` in `VIVector<VISceneDisplayElem>` (4 of 60),
`VIPool<VIHSpritePlay>` (16 of 75) and `VIPool<VIHSpritePlayNode>` (18 of 89),
plus the thirteen pointer `VIPool::Allocate`s at 4 of 66 — all one phenomenon,
and it is not the source: the loop's two induction variables are initialised
and incremented in the opposite order, and six spellings of the loop body leave
it unchanged.  `VIHSprite::GetNextAttach` (6 of 25) is prologue scheduling.
`__10VIMatrix44RC10VIMatrix44` — gcc's own implicit copy constructor, now
visible because something finally uses it — is 19 of 21, right instructions
and wrong registers: the target keeps `this` in `v0` and walks a copy in `v1`
where ours walks `v0` and restores it.
