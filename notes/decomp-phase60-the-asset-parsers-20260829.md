# Phase 60 — the asset parsers, and the pixel converters

**2026-08-29.  16 functions written, 23 byte-exact.  2,868/3,268 → 2,891/3,284,
and `VIESFParse` itself 78 → 101 of its 120 functions.**

Phases 58 and 59 opened `viesfparse.cpp` and closed 78 of `VIESFParse`'s 120
functions.  This phase takes the ASSET half — the code that turns the file's
bytes into a texture, a font, an XM module, an ADPCM wave, a skeleton, a
material or a particle definition — plus the five pixel converters underneath
all of it, and eight of the drafts phases 58/59 left open.

Sixteen functions were written from nothing and fifteen of those are byte-exact;
the sixteenth (`ParseAdpcmObj`) is one callee-saved register short.  Eight
already-written drafts closed on levers this phase found, four of them on ONE
edit.

## Byte-exact

| function | insns | builds | note |
|---|---|---|---|
| `Convert8` | 51 | 7 | the first converter; every lever below came out of it |
| `Convert4` | 101 | 1 | |
| `Convert16` | 96 | 1 | |
| `Convert24` | 103 | 4 | the one converter with a jump table |
| `Convert32` | 94 | 1 | |
| `ParseCSpritePartEmitters` | 123 | 5 | |
| `ParseCSpritePlayList` | 171 | 5 | |
| `ParseParticleDefinitionObj` | 118 | 1 | |
| `ParseParticleDefinition(VIObjFile&, VIParticleDefinition&)` | 285 | 3 | 13 float setters, 32 gradient colours |
| `ParseXmObj` | 166 | 1 | |
| `ParseHSpriteHierarchy` | 169 | 4 | |
| `ParseHSpriteObj` | 197 | 1 | |
| `ParseSurfaceObj` | 242 | 3 | |
| `ParseMaterial(VIMaterial *)` | 236 | 3 | |
| `ParseFontObj` | 271 | 3 | |
| `ParseCSpriteASlotList` | 56 | — | draft closed |
| `ParseCSpriteNodeIDList` | 58 | — | draft closed |
| `ParseCSpriteSkinList` | 65 | — | draft closed |
| `ParseCSpriteTSlotList` | 65 | — | draft closed |
| `ParseSound` | 38 | — | draft closed |
| `ParseSubSprite` | 54 | — | draft closed |
| `ParseSprite` | 118 | — | draft closed |
| `ParseParticleDefinition()` | 30 | — | draft closed |

## The levers

**A jump table's case order can be READ, not guessed.**  The image is
un-relocated, so a switch's table is five zero words and the case→body mapping
has to be inferred from the body order — which is source order, not value order,
and the two disagree.  The RELOCATED copy in `memory-dumps/inworld-eqoa.live.eememory`
has the real targets:

    _vt$8VIEnvRay+0x48 -> 0x4350CC 0x4350F4 0x43511C 0x435144 0x43516C

which says `ParseSurfaceObj`'s depth codes 0..4 select Convert4, Convert8,
Convert16, Convert24, Convert32 in that order.  `eqcalls.py` already prints the
table's SYMBOL for the `lui`/`addiu` pair (`addr _vt$8VIEnvRay+0x48`); reading
five words out of the dump at that address is the rest of it.  The same trick
confirmed `Convert24`'s own table and `ParseFontObj`'s.

**A converter's cases `break` to ONE shared `return 0`, with `default: return -1`.**
Written the obvious way — `case A: ...; return 0;` in each arm — the tail is one
`move v0,zero` and the whole function is 5 words wrong.  Written with `break` and
a single `return 0` after the switch, reorg fills each arm's branch delay slot
with a COPY of that `move v0,zero`, which is where the target's duplicated
returns come from.  This was the last 5 words of `Convert8` and it made the
other four converters right first build.

**Local copies of the walked pointers.**  A converter's loop walks `src` and
`dst`; spelled `*src++` / `dst += 4` the parameters ARE the induction variables,
and the target keeps both — `move v1,a2` in the entry block and `move t0,v1`
again in the case body.  `unsigned char *s = src, *d = dst;` inside the case is
the whole difference (32 wrong words to 11).

**`for (; n > 0; n--)` on the count parameter itself**, not `for (int i = 0; i < n; i++)`:
the target's bottom test is `bgtz` on the decremented count, where a separate
counter gives `bnez`.  11 wrong words to 5.

**A named local for the first component makes its load precede the read-modify-write's.**
`Convert24`'s RGBA16 arm is three `*d = (*d & mask) | value;` statements.  In the
first one the target loads `src[0]` BEFORE `*d`; swapping the operands of `|`
changes nothing, because gcc canonicalises a commutative operator.  Hoisting the
component into its own local —

    unsigned int r = src[0] >> 3;
    *d = (*d & 0xFFE0) | r;

— is what orders the two loads.  5 wrong words to 0.

**`0x8000u`, not `0x8000`.**  As a signed `int` the constant is loop-invariant
and gcc hoists `li a2,-32768` out of the loop and ORs the register; with the
unsigned suffix it stays an `ori` immediate, which is what the target has.

**A copy of the index into a second local forces its load BEFORE the next call.**
The four `ParseCSprite*List` walkers all read a slot index and then read a value,
and the target loads the index into a CALLEE-SAVED register in the second Read's
delay slot — i.e. before the call, which a value used only after it would never
need.  `int j = slot;` between the two reads is the whole of it:

    m_file->Read(slot);
    int j = slot;
    m_file->Read(node);
    c.m_items[j].m_node = node;

27/56, 36/58, 35/65 and 38/65 wrong → all four byte-exact.  It is the same shape
as phase 45's "a copy must be made to CONFLICT", reached from the other side.

**The MEMBER spelling beats a named array reference here.**  Phase 52's lever
says `VIArray<T> &a = m_x;` materialises the address; `ParseCSpritePartEmitters`
wants the opposite — `c.m_emitters.Init(n)` and `c.m_emitters[i].m_field` written
out at every use, with gcc hoisting `&c.m_emitters` itself.  A named reference
costs a register and swaps `this`/`c`; 36 wrong words to 0.  The rule is not
"always name it": it is that the target's register count decides, and both
spellings have to be tried.

**A chained assignment stops an FP constant being hoisted.**  `ParseCSpritePlayList`
re-materialises 1.0f and 0.0f at the top of every iteration.  Written as four
separate assignments gcc hoists 1.0f into `$f20` — a CALLEE-SAVED float register,
which also grows the frame by 16 bytes.  `vol[0] = vol[1] = 1.0f;` keeps it in
`$f1` inside the loop, which is what the target does.  158 wrong words to 4.

**A dispatcher's shared `return i` must be ONE statement.**  Phase 59 found that a
result variable needs `break` rather than `return i` in each case.  The same
functions need the DEFAULT arm to reach that one `return i` too:

    default:
        i = -1;
        if (m_file->ReadEnd() < 0) return -1;
        goto out;
    }
    if (i < 0) goto fail;
    out:
    return i;

With `return i;` written twice gcc proves `i == -1` on the default path and
folds the whole arm to `li v0,-1`; with one `return i` it keeps the variable and
emits `li s0,-1` in the ReadEnd's delay slot plus `move v0,s0`.  `ParseSound`,
`ParseSubSprite`, `ParseSprite` and `ParseParticleDefinition` all closed on it.

**An if/else whose trivial arm comes FIRST keeps the object's own register.**
`ParseParticleDefinition(VIObjFile&, VIParticleDefinition&)` picks the definition
or a fresh motif each iteration.  Written `a = &d; if (i) { m = d.CreateMotif(); ... }`
cse hands `CreateMotif` the pointer it just stored into `a` (`move a0,s1`); the
target uses `&d`'s own register (`move a0,s5`).  Splitting it —

    if (i == 0) a = &d;
    else { VIParticleMotif *m = d.CreateMotif(); ...; a = m; }

— leaves `a` undefined on the call's path, so the call gets `s5`.  1 wrong word
to 0, and the `move s1,s5` still lands in the branch's delay slot.

**Three named `VIVect3 &` row references.**  `ParseMaterial` reads a 3x3 UV
transform.  `m.m[0].x` … `m.m[2].z` fold to nine sp-relative addresses; the
target keeps the three ROW bases in callee-saved registers (`addiu s7,sp,28`,
`addiu s6,sp,40`) and only the +4/+8 inline.  Declaring `VIVect3 &r0 = m.m[0];`
and friends is what creates the three loop-invariant pseudos.  112 wrong words
to 0 together with the `!` below.

**`if (!(flags & 1))` is not `if ((flags & 1) == 0)`.**  The first gives
`xori`+`andi`+`beqz` — the truth value materialised and then tested — where the
second gives `andi`+`bnez`.  Both are one word different and both appear in
`ParseMaterial`, one per bit.

**A second local for a field read once and used many times.**  `ParseFontObj`'s
depth code is a `Read(int &)` target, so it lives on the stack; the target loads
it ONCE into `s5` and never reads the slot again.  `int d = depth;` right after
the read is what puts it in a callee-saved register, and the frame grows by the
16 bytes for `s5`.  198 wrong words to 0.

**`sltiu` against 2 is a RANGE check** (phase 48), and it is how both
`ParseSurfaceObj` and `ParseFontObj` gate their palette: `if (depth >= 0 && depth < 2)`,
not `if (depth < 2)`.  Two words each.

## Harness trap

**`mips` is a predefined macro.**  ee-gcc 2.95 defines `mips` to `1` for this
target, so `int mips;` is `int 1;` — "parse error before `1'", four lines below
where the name actually is.  `ParseSurfaceObj`'s mip-level count had to be
renamed.

## Field-map corrections

* **`VICSpriteAnimEntry::m_particle` and `m_trigger` are FLOATS**, not ints.
  `ParseCSpritePlayList` reads both with `Read(float &)` and stores them with
  `swc1` at +0x28 and +0x2C.
* **`VIESFParse::ParseParticleDefinition` has a SECOND overload**,
  `(VIObjFile &, VIParticleDefinition &)`, which is where all 13 float setters,
  the two colour variances and the 32-entry gradient are read.
* **The surface and font parsers' depth code is not `VIBitDepth`'s numbering.**
  0..4 select Convert4/8/16/24/32, i.e. 4, 8, 16, 24 and 32 bits per pixel, and
  `< 2` is exactly "paletted".  `vi.h`'s `VIBitDepth` (None, 32, 16, 8, 4) is a
  different encoding and the parser casts into it.
* **`VISurface` and `VIRFont` are handed to `VIDictionary::Add` as `VIResource &`**
  even though they derive from `VIBuffer` — the two classes have the same two-word
  shape and the parser casts.
* **`VIParticleDefinition::m_resource` is at +764** (0x2FC), confirmed by
  `ParseParticleDefinitionObj`'s `addiu a1,s2,764`.

## Open

**`ParseAdpcmObj` — 140 of 245.**  The structure is right end to end: the two
memcpy-free format arms, the mono decode with its `n - ((loop+1) << 4)` size, the
stereo decode into a scratch buffer and the de-interleave loop back into the
wave.  What is left is ONE callee-saved register: the target uses s0..s7 and this
candidate s0..s6, so every register from `wave` onwards is one lower and the
positional count reads as a whole-function miss.  Three sweeps over the
arithmetic spellings and the local scopes moved it by one word, which is the
signature of a register-count difference rather than a codegen one.

**`ParseHSpriteAttachments` — 18 of 81** (22 at the start of the phase).  Two
edits helped — one shared `if (a < 0)` test after the AttachSkin/Attach if/else
(phase 52's lever run backwards), and `goto fail` throughout — and what is left
is the block-placement problem below.

**The `return -1` block's POSITION is still unsolved, and it now blocks four
functions.**  `ParseObject` (7/230), both `Parse*LODSpriteLevels` (12/72 each) and
`ParseHSpriteAttachments` (18/81) all differ only in WHERE gcc put the one
`b <epilogue>; li v0,-1` block that every failing test branches to.  The target
puts it at the LAST inline `return -1` in `ParseHSpriteAttachments`, at the
SECOND in `ParseCSpritePartEmitters` (which matched), and at a mid-function one
in `ParseObject`; this candidate puts it at the first, or at the bottom when a
`fail:` label exists.  Every combination of `goto fail` / inline `return -1` over
the four sites was swept in `ParseObject` and `ParseHSpriteAttachments` and none
moves it.  It is worth a phase of its own: the lever is likely in how gcc 2.95's
`jump.c` chooses which duplicate to keep, and the answer would close four
functions at once.

## Not written

`ParsePrimBuffer` (774), `ParseSkinPrimBuffer` (722), `ParseFloraPrimBuffer`
(356), `ParsePrimBufferObjV0` (178), `ParseSkinPrimBufferObjV0` (218),
`ParseCollBuffer` (440), `ParseCSpriteObj` (354), `ParseHSpriteAnimObj` (450),
`ParseSpellEffectObj` (276) and `AdpcmToPcm` (758).  `ParseSpellEffectObj` is
fully decoded in this session's working — three `VISpellEventList`s at +12/+24/+36
selected by a three-way switch, a 44-byte event record, and two dictionary
lookups keyed on the event type ({3,8} particle definitions, {9,11} sprites) —
and only needs writing.  `AdpcmToPcm` is 758 instructions of 64-bit fixed-point
with no calls and is the single largest thing left in the class.
