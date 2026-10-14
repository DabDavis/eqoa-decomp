# The decomp phase log — one paragraph per phase

This is the condensed per-phase roll-up that used to live inline in the root
`CLAUDE.md`, moved out on 2026-08-27 when that file passed its size budget.
Nothing here is unique to it: the long form of every phase is
[`decomp/README.md`](../decomp/README.md), and each phase also has its own note
in this directory (`decomp-phaseNN-*.md`) annotated in
[`claude-notes/README.md`](README.md).

Keep it for what it is good at — **reading the whole campaign end to end**, or
grepping for a lever ("the if-direction is per function", "a jump table is
data") without opening a 4,500-line README. Phases 20, 29 and 37 have no `##`
section of their own in `decomp/README.md`, so their roll-up here and their
note file are the two places to look.

The heading below is the entry as it stood in `CLAUDE.md`, verbatim.

---

## [decomp/README.md](../decomp/README.md) — matching decomp (**2,793 of 3,177 byte-exact**)
Offline tooling to extract any function out of a decrypted module image and diff it against a
candidate build — **no compiler needed for the harness itself**, and useful for ordinary RE
regardless. `eqdecomp.py` / `eqdiff.py` / `eqd.py` (`list` `show` `extract` `census` `diff`) ·
`eqfingerprint.py` (fingerprints, to narrow compiler+flags *before* matching) ·
**`eqcalls.py`** (**the un-relocated image cannot name a callee, and a resident SUPPORT
can**: an in-world EE memory dump is the SAME BUILD relocated — 209,610/227,290 words
identical and **every one of the 17,680 differences is a relocation site** — so
`calls <symbol>` reads each `jal`'s target and looks it up in the same symbol map;
it corrected `VIScene::GetSprite`→`Sprite` and `VIDictionary::Release`→`Remove`,
both guessed wrong since Phase 10) · **`eqrtl.py`**
(which PASS made a difference, out of gcc's own `-da` dumps — **no compiler source needed**) · `eqsweep.py`
(sources x toolchains x flags, ranked) · **`eqcheck.py`** (builds `src/` and scores every
function; `MATCHED.txt` is a regression baseline) · **`eqasm.py`** (target | candidate side by
side — `eqd.py diff` hides "same instructions, different order") · `test_eqdecomp.py` (the
canary — 26 tests, 161 assertions, every check paired with a negative control). Oracle is
`SUPPORT_decompressed_SNR1.bin` indexed by `VA - 0x003E3D00`; the image is **un-relocated**,
so an unlinked `.o` matches its shape and no normalisation is needed — and where it does not,
the differ masks **exactly the fields the candidate object's own relocation table names**,
counted and printed (`readelf -r` offsets are PER SECTION; ignoring that masked 14 real
instruction words). `toolchains/` holds 15 prebuilt EE compilers and four SN assemblers — **all 14 scored on the
full tree**: SN `2.95.3-107`/`-114` 141, `2.95.2-273a`/`-274` 137, `-136` 40, every SCE/Cygnus
build 6-25, and **SCE never shipped a 2.95.x**, so the fork question is closed. The 114
`ps2eeas` is **1.9.6.516 = ProDG 2.2**, and the build that would close the 23 is v1.15-v1.35
(ProDG 2.3-2.9), which **is not publicly preserved**; **SN's gcc source was mail-order only**,
so a from-source rebuild cannot meet its own control. `toolchains/src/` holds the one source drop that does
exist (`2.9-ee-990721`) and **builds it as a rig** (`build.sh`/`exp.sh`, control 33/36 against
the shipped binary): the `sched`/`sched2` divergence is **the R5900 `load` ready-delay** — 2
gives ours, **1 gives exactly the target's order**, surgically (10/12 corpus unchanged);
`ADJUST_COST`, `ISSUE_RATE` and the ALU multiplicity are all refuted. It does NOT show SN's
v1.2x used 1, and closes none of the 23;
`tools/` a venv with asm-differ and m2c. **The build is `ee-gcc 2.95.x -O2 -G0 -fno-exceptions` compiled with
`-S` and assembled with SN's `ps2eeas` from the `2.95.3-114` package** — a DIFFERENT package
from the compiler, and a different VERSION from the one that pads a post-branch `div`. Two
nops settle it: gcc emits the R5900 `mtc1`→COP1 hazard nop as the comment `#nop` and every GNU
`as` drops it. **The layout is settled too: `VIMatrix44::m` is `VIVect4 m[4]`** — not
`float m[4][4]` — in an `aligned(16)` class, with a zeroing `VIVect4()` and copies through a
member taking a `const VIVect4 &` (a `Set`, not an `operator=`); that made `SwapRows` match
from natural source after ~30 spellings had failed, and all 28 earlier matches survived the
migration. Style facts that unlock groups of functions: **row pointers are variables**, loops
**hoist `float (*d)[4] = m`**, inner loops are **unrolled in the source**, four routines are
**Graphics Gems verbatim** with a load-bearing temp order, and **a MEM address comes out
index-first where an address materialised into a variable comes out base-first**.

**Phase 58 — the ESF PARSER.**  `viesfparse.cpp`, from a standing start: **73 of
`VIESFParse`'s 117 functions written and 59 byte-exact**, and the tree 2808/3198 ->
2852/3250.  The largest class in the corpus that had never been scored — 55,984 bytes, and
the one piece of code that turns an `.ESF`/`.CSF` object tree into live engine objects.
**`eqcalls.py` is what made it cheap**: every function is a straight-line run of calls into
`VIObjFile` and back into itself, so naming the callees off the resident SUPPORT turns each
disassembly into a transcript — `ReadBegin`, a tag compare, N `Read`s at known
displacements, `ReadEnd`.  **Three shapes repeat all the way down**: the WRAPPER, the WALKER
(`NumSubObjects()` re-read every iteration) and the ARRAY WALKER (the count hoisted, because
the array being filled was sized from it); a wrapper handing back an INDEX ends `movn`, one
handing back a STATUS ends `movz`.  Four field-map corrections — **`VIZone` +0x08 is an
eight-byte `long`** (`m_baseOffset`, handed to the `ReadBegin(offset, …)` overload),
**`VIZoneRoom::m_a3C` is the ACTOR array and `m_a44` the STATIC LIGHTING array** one entry
per actor, **`VIZoneStaticTableElem` is two `(unsigned long, unsigned int)` pairs** and not
`int[8]`, and **`VICSprite::m_body` and `m_node` are ONE array of ten**.  Shipped:
`ParseWorldBase` ignores its own `VIWorld` argument, `ParseActorObj` reads a `VIColor32` it
never uses, `ParseParticleDefinition` does not fail on a foreign tag, and
`ParseCSpriteNodeIDList` forces every entry of a version-0 file into slot 3.  New levers:
**an index-returning wrapper is TWO RETURNS, not a clamp** (the ternary weakens the test to
`i < -1`, the clamp reuses the -1 as the comparand); **a reference into a container is
declared BEFORE the call whose argument setup it shares** (55/94 and 63/95 wrong to
byte-exact); **a scratch buffer used in one branch belongs in that branch's block scope**
(21/117 to 0, where permuting the declarations does nothing); **ONE shared loop counter, not
two**; and a switch whose default block ends the function is `default: goto dflt;` with the
block after the `fail:` label.  Open: the three dispatchers' default arm, where two exits
both produce -1 and every spelling folds the branch away — substituting -2 brings it back,
which is the control that says the fold is value-driven.
Finding: `decomp-phase58-the-esf-parser-20260829.md`.

**Phase 59 — the SPRITE LEAVES.**  The twelve `Parse*SpriteObj` leaves — the
dispatcher's whole right-hand side, the code that turns an ESF tag into a live scene
object — plus six more of the zone parser: **18 written, 16 byte-exact**, and the tree
2852/3250 -> 2868/3268.  Eleven of the twelve leaves matched on the first or second build,
because every leaf is the same six steps (read the id and the bounds, ask the dictionary
whether that id is already live, `CreateSprite`, `Add`, initialise, walk the sub-objects)
and only the `VISpriteType` and the `VIResourceType` differ.  New levers: **the order of
`m_bsphere.r = radius` and `m_bsphere.c.Zero()` is decided by whether the radius is a
CONSTANT or a VARIABLE** — a loaded radius wants the store FIRST so the scheduler can fill
the load's shadow with the first zero, a literal wants it second (`VIPointSprite::Init` is
the other answer), and both need phase 9's `VISphere *b` for the compound operators;
**`VIObjFile` has no `Read(bool &)`**, so a `bool` member is read through an `(int &)` cast
and `sizeof(bool)` being four makes it identical; **hand the CALL the member, not the local
it was just stored from** — an address-taken out-param may alias the store through the
object pointer, so the local reloads where the member re-reads the register cse already
has (23/156 and 29/169 wrong to byte-exact on one edit, and the same shape puts
`p->m_sound = ParseSound()` in the branch's delay slot); **a store between two calls is a
source fact**, gcc will not move one across a `jal`; **two nested objects want two (type,
version) pairs** (59/135 to 10, then phase 58's shared loop counter to 0); and **a
dispatcher that returns a result VARIABLE needs `break` to one shared `return i`** — with
`return i` inside each case gcc proves the value on every path and deletes the variable
(169/215 wrong).  Field map corrected four times: **`VIStreamAudioSprite::m_looping` is at
+0x40 and `m_priority` at +0x44**, off `IsLooping`/`GetPriority` being a bare `lw`;
**`VIRadialFloraDist` is not `int m_a[7]`** but a DictID, an index and three floats;
**`VIStaticLighting::ShareResources(VIRaster &)` exists**; and **`VIRaster::MaterialPal`
hands the pointer back as an `int`**.  `ParseZoneRoom` confirmed phase 55's dead-local rule
a second time — its prologue zeroes a four-word object nothing ever reads.  Open:
`ParseGroupSpriteMembers` (46/114 — the target keeps NINE induction variables for eight
member addresses, a DEST_REG giv and a DEST_ADDR giv on the same word, and no spelling
separates them) and `ParseZoneResource` (50/197 — three identical `jal ReadEnd` blocks the
target does not cross-jump and we do).
Finding: `decomp-phase59-the-sprite-leaves-20260829.md`.

**Phase 60 — the ASSET PARSERS.**  The other half of `VIESFParse`: the code that turns
the file's bytes into a texture, a font, an XM module, an ADPCM wave, a skeleton, a
material or a particle definition, plus the five pixel converters underneath all of it.
**16 written, 23 byte-exact**, and the tree 2868/3268 -> 2891/3284 — fifteen of the
sixteen new functions exact, and eight drafts phases 58/59 left open closed as well, four
of them on ONE edit.  New technique: **a jump table's case order can be READ, not
guessed.**  The image is un-relocated, so a switch's table is zero words and the
case→body mapping has to be inferred from body order, which is SOURCE order and not value
order; the relocated copy in `memory-dumps/inworld-eqoa.live.eememory` has the real
targets, and `eqcalls.py` already prints the table's symbol (`addr _vt$8VIEnvRay+0x48`),
so five words out of the dump settle it — which is how `ParseSurfaceObj`, `ParseFontObj`
and `Convert24` were all fixed at once.  New levers: **a converter's cases `break` to ONE
shared `return 0` with `default: return -1`**, so reorg duplicates the `move v0,zero` into
each arm's delay slot (the last five words of `Convert8`, and the other four converters
were right first build); **local copies of the walked pointers** and **`for (; n > 0; n--)`
on the count parameter itself**, where a separate counter gives `bnez` instead of `bgtz`;
**a named local for the first component orders a read-modify-write's two loads**, because
swapping the operands of a commutative `|` changes nothing; **`0x8000u`**, since the signed
spelling hoists a register out of the loop where the unsigned one stays an `ori`
immediate; **a copy of the index into a second local forces its load BEFORE the next
call** — `int j = slot;` between two `Read`s, which is what puts it in a callee-saved
register and closed all four `ParseCSprite*List` walkers; **the MEMBER spelling beats a
named `VIArray &` here**, the counterpoint to phase 52, so both have to be tried; **a
chained assignment `a = b = 1.0f` stops the constant reaching `$f20`** and the 16 bytes of
frame that come with it; **a dispatcher's shared `return i` must be ONE statement** —
phase 59's `break` rule extended to the DEFAULT arm with a `goto out;` past the `if (i < 0)`
check, which closed four more; **an if/else whose trivial arm comes FIRST keeps the
object's own register** for the call in the other arm; **three named `VIVect3 &` row
references** make a 3x3's row bases loop-invariant pseudos; **`if (!(x & 1))` is not
`if ((x & 1) == 0)`** (`xori`+`andi`+`beqz` versus `andi`+`bnez`); and **a second local for
a stack-resident field** keeps it in a callee-saved register.  Harness trap: **`mips` is a
predefined macro** in ee-gcc 2.95, so `int mips;` is a parse error four lines from the
name.  Field map corrected: `VICSpriteAnimEntry::m_particle` and `m_trigger` are FLOATS;
`ParseParticleDefinition` has a second overload taking `(VIObjFile &, VIParticleDefinition &)`;
the surface and font parsers' depth code is a 0..4 index over 4/8/16/24/32 bpp and NOT
`VIBitDepth`'s numbering; and `VISurface`/`VIRFont` are handed to `VIDictionary::Add` as a
`VIResource &` although they derive from `VIBuffer`.  Open: `ParseAdpcmObj` 140/245, one
callee-saved register short so the whole function shifts — and, worth its own phase, **the
`return -1` block's POSITION**, which is now the only thing between FOUR functions and
byte-exact (`ParseObject` 7/230, both `Parse*LODSpriteLevels` 12/72,
`ParseHSpriteAttachments` 18/81); every combination of `goto fail` and inline `return -1`
over their failure sites was swept and none moves it.
Finding: `decomp-phase60-the-asset-parsers-20260829.md`.

**Phase 61 — FINISHING SUPPORT (in progress).**  "Keep going until SUPPORT is finished",
worked by up to ten agents in isolated LANES (`_lane.py`: own `src/`+`build/`, a `.base/`
snapshot, three-way `git merge-file` back into main). **Every named SUPPORT function is
written** (the last "unwritten" one, `VIRStrip6`, is a VU1 label equal to `VIEEUnloadIrx`'s
entry -- `SPAN_ALIAS`), and **2,891 -> 3,660 of 3,751 byte-exact** so far. The phase's real
finding is that **byte-exact is not correct**: the scorer masks every relocated field, so
`eqcallcheck.py` (jal targets against the relocated dump) found 84 "matched" functions calling
the WRONG function -- every `DestroyPool` walked with `Erase` instead of `Next`, VIXm's spu/xm
fields were swapped, a static sprite's extra argument is its actor's baked colour buffer --
and `eqrefcheck.py` (%hi/%lo pairs) found 261 data-reference findings: the original strings,
tables, `C:/code-frontiers-gm/` `__FILE__` paths and the DRDP identifiers the asserts reveal;
both now gate the score. `eqlocal.py` scores the code the map does not name (125 KB: 107 KB
per-TU template copies, the rest our statics) -- **all 65 unnamed functions are exact and no
unique code is unmatched**. **Every compiler-knob hypothesis was refuted on a patched copy of
the real cc1plus** (four latencies, five scheduler tie-breaks, the cross-jump USE barrier:
each closes 0 opens and breaks 51..2,217 matches), so every open function is SOURCE SHAPE --
and the large late gains came from deriving gcc's own rules from its 990721 source: inline
reference parameters lose the POINTER flag (alias.c), loop.c's hoisting COUNT, gcse's
computable hash order, cse's skip-blocks and struct-member invalidation, strict aliasing on at
-O2, cross-jumping after sched2, the Insert family's copy-preference rule, plus simulators of
global and local allocation (`decomp/regalloc/`). Findings:
`decomp-phase61-finishing-support-20261013.md`, `decomp-alias-rule-inline-ref-params-20261014.md`.

**Phase 57 — the RASTER'S DMA HALF.**  `viraster.cpp` again: 24 more of `VIRaster`'s
167 functions written and 16 byte-exact, taking the file to 128 written.  Phase 56 wrote
the object; this takes the half that talks to the hardware — the three microcode uploads,
the per-material uniform block, the strip and billboard closers, the blit brackets, the GS
register packets, the skinning palette, the deferred free lists, the four remaining light
setters and the DMAC handler.  **The packet vocabulary**: `m_id = 16` is a DMAtag CNT and
`80` a REF; `0x50000002`/`0x50000004` VIFcode DIRECT; `0x10000000` FLUSHE, `0x13000000`
MSCNT, `0x14000000 | RStripOffset` MSCAL, `0x15000000 | ((VIBbRender - VIBbInit) >> 3)`
MSCALF — **only the label DIFFERENCE is used** — and `0x6C..` UNPACK V4-32 with NUM in bits
16-23; `0x1000000000008001` is a GIF tag and `14` its A+D REGS, so `SetFogRegisters` writes
FOGCOL, `SetGSDithering` DTHE and `InitGSRegisters` COLCLAMP/TEST_1/DIMX.  `SetObjectBlends`
ships the skinning palette in blocks of **32 matrices** — 128 quadwords, what one UNPACK's
NUM field carries.  Corrects phase 56 three times: **`VIRasterPointLight` is
`VIVect3` + `float` + `VIColor32F`**, not a `VIVect4` (the `Mul` it calls takes a
`VIVect3`); **the dir-light array at 0x700001BC OVERLAPS the point-light array at
0x700001C0**, so a frame uses one kind or the other; and **`VISurface` HAS a destructor**
(`delete s` calls `_$_9VISurface(s, 3)`, not `__builtin_delete`).  Six more statics named,
including **`DBuffer`, the double-buffered GS register block** whose +480 is the ZBUF_1
value `GetZBufferReg` returns.  Levers: **a `volatile` pointer local materialises a hardware
address into a register**; **a whole-object assignment written INLINE keeps its own address
pseudo** (the only way to keep `SetDirLight`'s two `mult i,28`); **every SPR field is its own
absolute constant**, so a per-field cast beats the `VIRASTER_SPR->` macro, which lets cse
hoist one base; **`VIVect3::Set()` materialises a local's address** and `SetDefaultLights`'
second vector is a **dead local**; **a named `end` pointer decides which side of a pointer
subtraction carries the `+1`**; **a named byte count keeps `(n << 4) + 16` together**;
**`((long *)base)[k]`, not a named `long *`**, for `sd` into a DMA packet; **the `lui`s come
out in SOURCE order**, which settles two VIFcode stores; and **a constant-size `memcpy` is a
CALL only when an argument is a `void *` VARIABLE** — every other spelling expands into a
block-move loop, or a *pair* of loops behind a runtime `andi 7`.

**Phase 56 — the RASTER.**  `viraster.cpp`: 104 of `VIRaster`'s 167 functions written and
98 byte-exact, 2,679/3,049 -> 2,777/3,153.  `VIRaster` is SUPPORT 0x003FF2E8 .. 0x0040C758,
its own TU, and was the largest class in the corpus never touched — 167 functions and
12,384 instructions against `VIScene`'s 140.  It is the renderer: five resource pools, the
material list and its LRU, the DMA double buffer every draw call appends to, and the
immediate-mode vertex path (four quadwords per `Vertex()` — position, uv, normal, colour).
**The per-frame uniform state is NOT in the object**: `GetModelView` is `jr ra` +
`lui v0,0x7000`, and the matrices, the sun, the ambient, the ONE point light
(`MaxPointLights()` returns 1, `MaxDirLights()` returns 0), the fog and the enable bytes all
live in SPR at 0x70000000.  Corrects `raster-system.md` three times: **+0x3298 is not the
surface cache** (that is a file-scope object; +0x40..+0x4488 is the 17,480-byte
tessellator), **+0x4488 is a FLOAT** (the Z bias), and **+0x0C..+0x2F are the immediate-mode
vertex attributes**.  Levers: **a field's absolute address is its own constant, not a struct
base** (two uses of one SPR field give it a `lui`+`ori` pointer of its own); **an SPR array
subscript keeps the member offset in the DISPLACEMENT**; **a mem-initialiser list runs in
DECLARATION order where a ctor BODY runs after every member ctor**; **five of these return
VOID** among a class of ints; **nest the body so the two `return 0`s cross-jump**; **the
element POINTER, not the subscript, when a call sits between**.  Harness: the `VIRaster`
class body had to MOVE to the bottom of `vi.h` (its members are `VIPool`/`VIList`
instantiations, declared 1,000 lines below where the shape assertion lived), and **three
stale objects in `build/` were shadowing real symbols** in any hand-rolled walk of
`build/*.o`.

**Phase 55 — the SKY and the WEATHER.**  `viatmosphere.cpp`: 30 of `VIAtmosphere`'s 32
functions written and 26 byte-exact, plus `WeatherFilter::FilterActor`, 2,653/3,020 ->
2,679/3,049.  `VIAtmosphere` is SUPPORT 0x00416880 .. 0x0041C7F8, its own TU, embedded in
`VIScene` at +0x640: the sky dome and its three rotation matrices, 128 stars and a shooting
star, the lightning flash and its 32 patches, the rain/snow field with its splash pool, and
the Lavastorm fireballs, in 4,528 bytes.  **The constructor is a member roll-call and the
destructor is that roll-call in reverse**, and both matched on the FIRST build, which is what
proves the layout: `VILList` at +0x14/+0x24/+0x1160/+0x1170, `VIMatrix44` at
+0x500/+0x540/+0x580, `VIVect3[2]` at +0x50 and +0x68, `VIVect3[64]` at +0xA8,
`VIVect4[128]` at +0x620 and `VIVect4[32]` at +0xF60, `aligned(16)` so 0x11AC rounds to
0x11B0.  Three nested classes fall out: `VIWeatherParticle` (36 bytes, a `VIColor32F` at
+0x0C, and NOT a `VILNode` — one `new char[36*n]` with no array-new cookie), `VISplash`, and
`VIFireball` (a `VILNode`, 48 bytes, then TWO `VIParticleEmitter`s at +0x40 and +0x420, which
also fixes `sizeof(VIParticleEmitter)` at 992).  It corrects
`claude-notes/weather-system-decompiled.md` in four places, including that `ResetLightning`
CLAMPS to [0.1, 0.8] where the note read a 10% skip.  Levers: **a function-scope local and a
block-scope local get different FP registers**, and BOTH directions are load-bearing in this
one phase (`RenderWeather` needs function scope, `RenderStars` needs per-block); a
`VIMIN(VIMAX(k, x), K)` threads the constant arm into three compares and the argument ORDER
reads back off the branch directions; **name the clamp result and multiply in a SECOND
statement**, or gcc distributes the multiply into all three arms; a `VIVect4` four-float
constructor hoists its constants where four member assignments rebuild them every iteration;
a dead local is a source fact (`RenderWeather` builds a `VIBBox` it never reads); and a
subscript with a ternary index scales the TERNARY, not the index.  Harness: **grep for the
class before declaring it**, a third time — `WeatherFilter` was already in `vi.h` AND defined
in `viactorfilter.cpp`, and it belongs to THIS TU (`FilterActor` is the word before the
constructor, `__tf13WeatherFilter` the word after `ProcessLavastorm`).  Open: `Update` 4/128,
`RenderStars` 15/464, `ProcessWeather` 278/626 as a draft, and **`Init`, `ProcessTransitions`
and `ProcessLavastorm` NOT WRITTEN**.

**Phase 54 — the SCENE.**  `viscene.cpp`: 125 of `VIScene`'s 140 functions written,
114 byte-exact, 2,538/2,895 -> 2,653/3,020.  `VIScene` is the object the whole engine hangs
off — it EMBEDS `VIWorld`, `VIClipStack`, `VIParticleSystem`, `VIAtmosphere`,
`VIQueryTriList`, `VISceneSoundTrack`, `VISpellEffectSystem` and `VIRadialFloraSystem` by
value, 19,372 bytes — and an "actor" everywhere in it is a `VIActor *` carried in an `int`
and dereferenced with no bounds check.  The phase opened by REPAIRING the tree: a previous
sitting had correctly retyped `m_world` to an embedded `VIWorld` and `CreateSprite`'s first
parameter to `VISpriteType` and left thirteen TUs not compiling, which reads as a 131-symbol
regression.  Structures settled: **`VISceneIntersectVars` was wrong in five of its eight
fields** (`QueryIntersection` builds one, so every offset is visible), **`VISceneInsertVars`'s
+0x70 is a `VISphere`** — four words zeroed twice is that class's constructor —
**`VISceneStreamVars::m_p14` is a `VICollide *`**, **`VISceneStreamRadialVars::m_p2C` and
`m_p30` are FLOATS**, **`VIActor::m_p70` is a `VIColor32F`** (the baked static-lighting
colour), **`VIScene`'s two streaming-radius arrays are PAIRS** copied inner-before-outer, and
**`VIZone::Load` takes `(id, zone, VILoader &, file)`**.  Levers: **a guard that `return 0`s
lets gcc if-convert the last test where one that `goto done`s does not** (three functions on
one edit); two `return -1`s cross-jump only when they are one statement; **a subscript at
each use gives TWO induction pointers where a named reference gives one and a counter**;
**`a[k]` and `a.m_data[k]` are not the same to the strength reducer** (43 wrong words to 1);
a scratch local keeps its constructor only if declared with the others; **name the clamp or
gcc distributes the following `+` into all three arms**; two identical tails beat one shared
tail; **an early `return -1` costs the entry-block constant**; `(int)` on a `long` difference
is `dsll32`/`dsra32`/`cvt.s.w`; a store pair is emitted in reverse source order.  Shipped:
**`ReleaseStatic` leaks a room's material palette when there is no raster** — the release and
the flag that records it are both inside `if (m_raster)`, and that was the one word between
the function and byte-exact — and `CreateActor`/`StreamInStaticActor` write the SOURCE
sprite's index into the COPY's `VISprite::m_actor`.  Not attempted: the time-of-day group
(`InitTimeOfDay` is 1,165 instructions), the two `SetSky`s, `StreamOutStaticActor`,
`RasterActorBBox`, `RenderShadow`, the sound-track pair, `DebugResources`, `Render` and
`RenderDeferredList` — fifteen in all.  Long form:
`claude-notes/decomp-phase54-the-scene-20260828.md`.

**Phase 53 — the CHARACTER SPRITE.**  `vicsprite.cpp`: 44 of `VICSprite`'s 50 functions
written, 30 byte-exact, 2,508/2,851 -> 2,538/2,895.  Every player and every NPC in EQOA is
one of these — a `VIHSprite` with the customisation layer on top — and at **4,624 bytes it
is the biggest object in the corpus**.  The class is the finding, read off the constructor
and `Clear` and checked against a `this`-offset census over all fifty methods: sixty-six
animation slots, nineteen texture slots, six attach points, a capsule, a weapon trail, one
sound and an array of node-bound particle emitters.  **Its own data starts at +0x190, not
+0x188** — `VIHSprite` carries a `VIMatrix44`, so it is `aligned(16)` and its SIZE rounds
up, and the base's eight bytes of tail padding are the first in the corpus to be visible.
**The constructor is a member-constructor roll-call**: gcc runs every member's constructor,
in declaration order, before a line of the body, so the run between the vptr store and the
last `jal` is a LIST of which field groups are members with constructors of their own.
Levers: **the priority table is written LAYER BY LAYER, not entry by entry** (119/122 ->
70/122 -> byte-exact); a two-case if-chain on a sprite type is a SWITCH; `~4u` not `~4`;
**a hoisted `this` for a call argument costs a callee-saved register**; a local's
declaration order decides the prologue.  Shipped: `SetHelm` tints palette entry 0 N times,
and `AttachItem` does nothing for a player.  Harness: `decomp/_perm.py`, a line-permutation
sweep that loads the image once.  Long form:
`claude-notes/decomp-phase53-the-character-sprite-20260828.md`.

**Phase 52 added the ZONE TREE — `vizone.cpp`, one whole TU: 50 of `VIZone`'s 67
functions written, 24 byte-exact; 2,484/2,798 -> 2,508/2,851.** The sibling of phase 51, one level
down: a `VIWorld` is a BSP over ZONES, a `VIZone` a BSP over ROOMS, and the room is where the
scene's actual work happens. It is also the phase that MEASURES phase 51's prediction —
disassemble `Collide__7VIWorldiR15VISceneCollVars` and `Collide__6VIZoneiR15VISceneCollVars`
and diff them by mnemonic and **they agree instruction for instruction**, bar the register
that holds `node`, `m_tree`'s offset and the leaf arm; the same holds for `Pick`,
`QueryProximal`, `QueryEmitters`, `Stream` and `QueryVolume`. So the descent half of the file
is phase 51's source with one line changed and the three biggest (212/204/196 instructions)
matched on the first build. The LEAF end is the new content, four facts about `VIZoneRoom`:
**`m_p20` is the DESCENT stamp** (against the world tree's own `m_state`, threaded through the
Vars), **`m_p24` is the SCENE stamp** at `VIScene + 0x544` on a different clock, **`m_p1C` is
the room's own static sprite** drawn through the static `VIZone::IdentityMatrix` (a zone's
geometry is already in world space), and **`m_p68` is the head of the room's ACTOR list — a
pointer kept in an `int` and terminated by `-1`, not by null**. **`VIZone::Find` is a BINARY
SEARCH** over the resource directory by DictID, so a zone resolves one in log time with no
dictionary. **The room streamer is a six-way decision on ONE distance**, two independent
in/halt/out triples with different radii, so statics have their own hysteresis inside the band
where the room is already resident. Levers: **a NAMED REFERENCE to a member array is what
materialises its address** — `m_pretrans.m_data`, `&m_pretrans[0]` and an inline `Data()` all
fold to one `lw` where `VIArray<T> &pt = m_pretrans;` does not, three wrappers byte-exact on
that line, which corrects phase 51's subscript rule to "only with a VARIABLE index"; **phase
51's cross-jump lever run BACKWARDS** — the world's wrappers have ONE `jal` and the zone's have
TWO, and `int hit; if/else; return hit;` keeps them apart where two `return`s merge them, so
the number of `jal`s has to be read off the target per file; and **a `-1` sentinel in a POINTER
is `lui`+`ori` where an `int` is one `li`**, three room walkers from 30/65 wrong to byte-exact.
It also corrects **eight fields across seven of phase 51's Vars structs** (`VISceneInsertVars`
`+0x04` is the ACTOR and `+0x0C` the scene, `VIScenePickVars` `+0x10` is the RASTER,
`VISceneRendVars` `+0x34` is a float view distance, and the four query Vars carry a
`VISphere`/`VIBBox` plus a query object), every one offset-for-offset, with `viworld.cpp`
migrated and re-scored to prove nothing regressed. Open: almost every miss is ONE phenomenon
and it is phase 51's — `if (f() < 0) return -1; return 0;` if-converts in our build where the
target branches, nine spellings swept and every one if-converts.
**Phase 51 added the WORLD TREE — `viworld.cpp`, one whole TU: 37 of `VIWorld`'s
44 functions written, 27 byte-exact; 2,457/2,761 -> 2,484/2,798.** The top of the
spatial hierarchy — a terrain grid, an array of zones, and a BSP over the zones —
and the tree `go-eqoa-vi`'s `vicoll/scene.go` already walked against the console,
now with the source that produced it. `VIWorld` is **976 bytes** (`SetProfiles`'
last store is `+964`, inside the 22nd and last 40-byte terrain profile).
**`VIWorldTree::m_state` IS the pass stamp** — every traversal wrapper copies it
into the Vars, walks, and increments it. **A `VIWorldRegion` is ONE BYTE** and the
region array is the terrain-profile grid, sized `rows*cols*subdiv*subdiv`, which is
why `GetTerrainProfile` ends in an `lbu`. **A child index is TAGGED**: any bit of
`0xF0000000` means "not an ordinary interior index", `0x40000000` on top means LEAF,
and **any other tag is stripped and used as an interior index anyway**. There are
**three descent shapes** — segment, sphere and eight-corner volume — and the three
segment walks differ only in what the slack `r` is (`m_radius + 0.03125`, the bare
`0.03125`, `0.0`). Levers: **a constant-selecting wrapper is TWO CALLS** with the
result test inside each arm — gcc cross-jumps them into one `jal` and the branch
survives, where a select if-converts to `movz` (eleven of twelve wrappers exact on
one edit); **`array[i]` through an inline `operator[]` is an ADDRESS PSEUDO**
(`addiu t,this,52` + `lw d,4(t)`) where `array.m_data[i]` folds to `lw d,56(this)`,
and that pseudo is what the descents keep callee-saved across their recursion (58
wrong words -> 0); **two distances computed before the first test are LOCALS
REASSIGNED after every call**, not an expression spelled out at each use (207 ->
100); **`int r = -1; if (0 <= X()) r = 0;`** gives `slti`+`movz` where the ternary
gives `slt` against a materialised -1 and `movn`; and **a clamp on an `int &` is one
read-modify-write** on a local, the lower clamp a ternary and the upper an `if`.
Also: the clamp ORDER is a register-allocation fact (`GetTerrainProfile` clamps COL
then ROW, and every other order costs eleven words), and a local whose constructor
emits code belongs in the scope that uses it. Finding:
`decomp-phase51-the-world-tree-20260828.md`.

**Phase 50 added the DRDP CORE — `drdp.cpp`, the top of the stack, and with it
the DRDP network library is COMPLETE: all twenty-one translation units. 112
functions written, 100 byte-exact, 72 on the FIRST build; 2,357/2,649 ->
2,457/2,761.** A `drdp_t` is one PROCESS on the network — the sockets it listens
on, every endpoint, route and connection it knows about, the address<->route
cache and the leaky bucket that paces the whole machine — 544 bytes off `malloc`
and tagged `"DRDP"`. Three ROLES share the file (MASTER 0, LEADER 1, CLIENT 2):
a master owns the whole software endpoint id space and hands it out a block at a
time, a leader is given a block and asks for another when it runs dry, a client
has neither; the system messages that arbitrate all that are a 48-entry jump
table in `drdp_process_system_message`, the largest function in the library at
524 instructions. **An endpoint id IS an address** — `drdp_address_to_endpoint`
packs four octets, the UDP port and the drdp port big-end first into 64 bits, so
routing to a hardware endpoint needs no lookup at all; a local hardware endpoint
is `0xffffffffffff0000 | drdpPort` and a SOFTWARE endpoint is a plain number in
`[2048, 0xffffffff]`, the two bounds `route_table_init` is handed verbatim.
**The system endpoint's drdp port is the low half of the CLOCK**, stepped off
the three reserved values (`0xffff` -> 0, `0xfffe` -> 1, `0xfffd` -> 2), and
that is the whole of the local port allocation — two processes started in the
same millisecond collide. **The CRC carries the protocol version**: every packet
ends in the CRC-32 of everything before it XOR'd with
`drdp_crc_table[version]`, and `drdp_consume_crc_get_protocol_version` recovers
the version by looking the difference up in the same table, searched outwards
from where version 2 sits — so a packet from an incompatible build reports its
version rather than looking like corruption, and the three answers (bad crc,
wrong protocol, unlistened port) are distinct replies. **A packet's segments are
walked TWICE** — once to check every header parses and names two real endpoints,
and only then again to hand each to its endpoint, so a bad packet is reported to
every connection on the route as corrupt rather than half-applied. And **the
route table goes out delta-coded**: the leader ids first, then every range as
two packed deltas against the previous range's end, with the owner an INDEX into
that leader array (`n` the master, `n+1` nobody). Levers: **an inline helper's
key is a CONST REFERENCE** — the debug-checked binary search that turns an owner
id into a leader index re-reads its key out of memory on every iteration, which
a by-value `uns64` never does, and `const uns64 &` took
`drdp_master_send_route_table` from 219 wrong words to 53; **an assert whose
expression CALLS the helper keeps the result in `v0`**, where an assignment
followed by a VASSERT parks it in `s0` and shifts every register below it (53 ->
23); **a line number inside a multi-line macro invocation is the CLOSING line**,
so every assert in a padded file has to be one line; **a default-callback
if/else puts the DEFAULT on the fall-through** — `if (pfn == 0) X = default;
else X = pfn;`, 60 wrong words to 5; **where one store sits among a run of zero
stores decides the schedule**, and `drdp_create_addr`'s last five closed on
moving `drdp->role = role;` up ONE statement; **a buffer rebase must not zero
the position first**, or gcc folds the zero and deletes both adds; **an aligned
six-byte struct copy is a `memcpy`, not an assignment** — a struct assignment
uses `TYPE_ALIGN` (2 here) and gives `lwl`/`lwr` where `memcpy(&to,&from,4)`
plus a field copy gives `lw`/`sw` + `lhu`/`sh`; **splitting `x = c | (x << 16)`
into two statements** turns a shift into a temporary into an in-place one;
**`int t = field;` gives `slti` where the field compared in place gives
`sltiu`**; **`if (bits) r = 1;` and `r = bits ? 1 : 0;` are not
interchangeable** — the first gives `li 1` + `movz`, the second `sltu` plus a
`dsll32`/`dsra32` pair, because the DImode comparison has to be truncated to
`int`; **a three-arm chain whose middle two arms share a value is one `||`**;
and **a walk that re-PEEKS is not a get_next walk**, because the body takes the
element off the tree. Shipped:
`drdp_handle_segment_endpoint_not_local` calls `buffer_skip` on BOTH arms of a
test whose sides are otherwise identical — the forward that used to sit on one
of them is gone, and nothing in the recovered library calls `route_forward_segment`; seven
functions evaluate a `drdp_format_endpoint*`, a `message_get_size` or a
`connection_get_dst` and throw the answer away, the same trace residue phase 49
found in `route_flush`; and `find_leader_index`'s two asserts quote a DIFFERENT
source file from every other assert in the object, which is the proof it is an
inline function in an include. Harness: the working loop for a padded file is
**collapse -> edit -> repad**, and a failed pattern in a multi-edit script
throws away the edits that already succeeded — one edit per script. Open:
`drdp_process_system_message` 370/529 and `drdp_process_udp` 84/223 are one
phenomenon and it is stack LAYOUT — the original gives every local a
16-byte-aligned slot in declaration order, so a two-byte `uns16` sits between two
aggregates, where this build assigns all the BLKmode locals first and then PACKS
the address-taken scalars after them; nothing about the code shape differs.
(`claude-notes/decomp-phase50-the-drdp-core-20260828.md`)

**Phase 49 added the ROUTE LAYER — three whole translation units, and with it
the DRDP library is complete except for `drdp.cpp` itself. `route.cpp` 31/32,
`route_cache.cpp` 9/10, `route_table.cpp` 7/9: 51 written, 47 byte-exact, 39 on
the FIRST build; 2,310/2,598 -> 2,357/2,649.** A route is one UDP peer — the
socket address every connection to that machine shares, the leaky bucket that
paces it, and a tree of connections sorted by when each wants to go out next.
**`route_get_key_offset` is the OPPOSITE way round from the connection's**: key 0
is the desired flush time and key 1 the id, where `connection_get_key_offset`
puts the far id first. **`route_flush` is the only place in the whole library
that builds a packet** — the local and far ports, then the queued resets (a
header with NO body, flag `0x10000` for a connection and `0x20000` for an
endpoint, the instance numbers carried in the crc and flush-number slots), then
the forwarded bytes, then as many connection segments as fit, walked in
flush-time order so the one that has waited longest goes first; the flush number
is bumped at the end and **skipped when it would become -1**, because -1 is the
"never flushed" value. `route_cache.cpp` is the NAT table: address to route and
back, five minutes renewed on every use, one record on THREE containers (a tree
per direction plus a list by expiry, the shape `close_t` has), and a miss answers
with the address unchanged. `route_table.cpp` is a tree of RANGES whose
comparator returns 0 for any two that OVERLAP, so a degenerate-range lookup finds
the range containing an id; `init` seeds it with one entry covering everything,
and the two files use different sentinels — -1 means "delete" in `update_range`
and ZERO means "no route" in `find`. Levers: **`vset_remove_first` returns 0 on
SUCCESS**, so both flush-reset loops test `== 0` and a plain truth test inverts
the branch; **a runtime-sized stack buffer is `alloca`, and the give-away is the
frame pointer** — a fixed `addiu sp,sp,-N` plus `move s8,sp` plus a SECOND
`addiu sp,sp,-M`, which took `route_forward_segment` from 27/55 wrong to
byte-exact on one `__builtin_alloca`; **a 40-byte local is placed before a 2-byte
one whatever the declaration order**, so the only spelling that gives the target's
frame is declaring the `segment_header_t` inside the block that uses it (18
words); **a comparison tree carrying a `slti` for "below the range" is an
if-CHAIN, not a switch**, phase 47's rule run backwards; **a three-way
comparator's last `return 1` goes out of line** — `if (a > b) return 1; ...` gets
cross-jumped into the shared block and `if (!(a > b)) return a < b ? -1 : 0;
return 1;` does not (three comparators, 9 words); and **`route_create` writes
`m_uiId` AFTER the clock read**, 39 wrong words to 0 on moving one line past a
call. Shipped: `route_flush`'s forwarded-message loop evaluates
`message_get_size` an extra time and throws it away, the argument of a trace
whose call is gone. Harness: phase 48's trap in a form it does not name — **two
`eqcheck.py` invocations in ONE shell pipeline** collide in `decomp/build`
exactly the way a backgrounded one does. Open: the largest function in each file
(`route_flush` 59/234, `route_cache_touch` 82/166, `route_table_update_range`
97/201) plus `route_table_find` 11/30, all register allocation or branch layout
rather than shape.
(`claude-notes/decomp-phase49-the-route-layer-20260828.md`)

**Phase 48 added the CONNECTION — `connection.cpp`, the layer phases 43/44/45
were holding up and the one `endpoint.cpp` said in its own header comment it was
blocked on. 105 functions written, 74 of the TU's 116 byte-exact; 2,236/2,493 ->
2,310/2,598.** `connection_t` is 4,576 bytes, the largest struct in the library
by a factor of seven, and it is on THREE containers at once
(`connection_get_vnode_offset(i)` is `i*16+4`): the endpoint's tree keyed by the
far id, the drdp's, and the ROUTE's tree keyed by the desired flush time — which
is the one `route_flush` walks in order, so the route always services the
connection that wants to go out soonest. **`+0x120` is `uns32
m_uiFlushTime[1024]`, not a byte buffer**: a ring of send timestamps indexed by
`flush & 1023`, and the round trip is the difference between the ring entry and
now. **The timeout is the LARGEST of the sixteen samples and the reported ping is
the SMALLEST** — two loops over the same array with opposite comparisons, four
hundred bytes apart. 248 state channels of 60 bytes are malloc'd in ONE block on
first use, so a connection that never sends state never pays for them; a state
channel is a SLOT, and `send_state` sends nothing when the bytes have not changed
and replaces the pending message *wherever it is sitting* when they have
(`m_ucWhere`: 0 nowhere, 1 send queue, 2 outstanding). A guaranteed send is
FRAGMENTED at 1,300 bytes into type-250 messages with their own sequence numbers,
up to 0x514000, built into a LOCAL list first and only queued once every piece
succeeded. **The terminate notification is built in `connection_create`** and kept
for the life of the connection, so a terminate can be delivered when there is no
memory left to allocate one. Levers: **a range check folds to ONE unsigned
compare** (`sltiu v0,a0,248` is `0 <= t && t < 248`, not `t < 248`, which gives
`slti`); **a stack object declared FIRST lands at offset 0 and its address is then
free** — the five `send_uns8_*` helpers went from 20/23/30/33 wrong to byte-exact
on swapping `buffer_t buffer;` ahead of `char data[64];`; **a result variable that
shares a register with a parameter must be UNINITIALISED**, since `int iResult =
0;` at the top of `send_internal` costs a whole callee-saved register and 16 bytes
of frame; **a member compared against a CALL's result wants a local** hoisted above
the `if` (phase 47's operand-order lever with the other fix); **which of two
constants ends up in the destination is decided by the INITIALISER**, not by
if-versus-ternary, so put the value you want in the destination register first and
the `movn`/`movz` follows; and **three stores to adjacent fields are a permutation
sweep** — only one of six orders matches `connection_set_failure`, and it is not
the one the delay-slot rule predicts. Shipped: **`VASSERT(!"this makes no sense")`
can never fire**, because gcc 2.95 does not fold a string literal's address to a
constant truth and what ships is a runtime `beqz` on a link-time address that
always passes — every such assert in this library is dead the same way; and
**filling the 4,096-message guaranteed window KILLS the connection**, since
`send_guaranteed` returns -1 and `send_internal` turns any non-zero result into a
terminate. Harness: two new `SPAN_CAP`s, and **a backgrounded `eqcheck.py -f
<regex>` is still `eqcheck.py`** — phase 41's rule names `_q.py`, but the hazard is
the shared `decomp/build`, and a filtered run left running reported 17 phantom
regressions in a file the phase never touched. Not attempted: `parse_segment_body`
(3,064 bytes), `flush`, `create_buffer`, `destroy_buffer` and the serialisers —
the save-and-restore half, which needs `segment_body_t`'s parse order first.
(`claude-notes/decomp-phase48-the-connection-20260828.md`)

**Phase 47 added the HIERARCHICAL SPRITE — `VIHSprite`, the class every animated
character in the engine is: a node tree, a palette of blend matrices, a trigger
table and three pools. Phases 19/20 had nineteen of the twenty-six readers it
already had; this phase wrote the other fifty-five symbols of the TU — constructor, play-list
management, pose evaluator, centre-transform cache and all eight virtuals.
2,194/2,438 -> 2,236/2,493.** As in Phases 11/13/18/22/46 the SEMANTICS were
settled from the console FIRST (`go-eqoa-vi/vianim` is a bit-exact port of the
pose path), so every mismatch was codegen — and, as in Phase 18, the decomp paid
the port back. **`VIHSpriteNode`'s two frames were the wrong way round in
`vi.h`**: `SetNodeBindPose`/`GetNodeBindPose` write and read +0x7C/+0x8C/+0x90,
so +0x10 is the ANIMATED transform and +0x7C the bind pose — which is what
`vianim`'s console-read map said — and **+0xA0 is the INVERSE bind-world
matrix**, not a second world matrix (`SetBlendMatrices` hands
`blend[i] = invBindWorld[i] * world[i]` to `VIRaster::SetObjectBlends`).
Six levers: **a comparison's operand order decides whether the guard's load
survives a call** — `m_blendMS == 0 || sc.GetTime() - m_blendStart > m_blendMS`
re-reads the member after the call where `… || m_blendMS < sc.GetTime() - …`
parks it in a callee-saved register (13 wrong words to 0, and the same edit
closed two more functions); **a lone `sw zero` between two member constructors
is a MEMBER's constructor**, and `Copy`'s two eight-byte block moves say it is a
class with an array member — the LOD table at +0xA8; **naming a pool pointer can
COST registers** when its only uses are the walk calls (`Release` 43 of 79 wrong
to byte-exact on dropping it, and two virtuals with it) — the pointer-variable
lever run backwards; **a container pointer declared INSIDE the loop** is hoisted
to the preheader so its `addiu` lands after the guard, where a function-scope
declaration computes it before (`SetBindPose`, 8 words); **the outer clamp
macro's argument ORDER**, a second sighting of Phase 39's rule —
`VIMIN(1.0f, VIMAX(0.0f, t))` and not `VIMIN(VIMAX(t, 0.0f), 1.0f)`, the last
eleven words of a 209-instruction function; and **a two-case if/else-if on a
small int is a SWITCH** (`ProcessPlayTiming` 137 of 332 wrong to 26). Shipped:
`CalcNodeTransform` constructs and destroys a **second `VIMatrix44` it never
uses**, on the path every attachment and every helm goes through. Harness: a new
`SPAN_CAP` — `CalcLODLevelIndex(VIPickSegment &, VIRaster &)` takes the span of
the TU's trailing unnamed `VIMap<int,int>` statics (2,784 -> 268). Open:
`ProcessPlayList` (532 words), where the target spills `this`, the LOD level,
the scene and the play index to the stack at entry and the candidate keeps three
of them in registers.
**Phase 46 added the COLLISION CORE — the twenty functions Phase 22 left in
`VICollide`: the six buffer walkers, the three query walkers, the per-primitive
tests (triangle, edge, vertex, sphere, capsule, cylinder), the point-in-triangle
test and the capsule entry point. Twenty written, eight byte-exact, plus
Phase 22's `Cull(VICollRay &, VIBBox &)` closed on the way past — nine in all.** As in Phases
11/13/18/22 the SEMANTICS were settled from the console first —
`go-eqoa-vi/vicoll` is a bit-exact port of every one of these, checked against a
live SUPPORT — so every mismatch is codegen and every finding is a codegen
finding. Six levers: **a chain of six rejections is one `||`, not six `if`s**
(six `if`s leave the FIRST `return 1` in line; the `||` puts the single one after
all twelve compares — that closed both `Cull(..., VIBBox &)` overloads, one of
them open since Phase 22); **`n = numVerts; n = n - 2;` is not
`n = numVerts - 2`** (split, the `lh` goes straight into the loop counter — 12 of
`QueryBufferV`'s 16 words, and 65 -> 14 on `CollideBufferPackV`); **a skip's
statement order decides which of its two loads is issued first** (`prim +=`
written before `vert +=` gives the target's numVerts-first load pair, and the
two ADDs stay vert-then-prim either way); **reload assigns stack slots in
DECLARATION order, not first-assignment order** (`char *vp;` declared before the
accumulator and assigned after the early returns — same code, declaration moved,
byte-exact); **an early exit written `goto done` keeps the accumulator where
`return hit` folds it to `move v0,zero`** (the shared exit has two reaching
definitions and the private one has one); and **a vector copy followed by
in-place component updates keeps the first two stores and loses the third**, the
2/2/1 pattern at three separate sites. Source facts: **`VICapsule`/`VICylinder`
have a zeroing constructor** whose two vectors come out zeroed TWICE and two
floats once — the `VIBBox` signature from Phase 28 — and **two of these
functions carry a `VIPlane` local that does not look like a plane**: the
normalised cross product is `pl.n`, the negated dot with the base is `pl.d`, and
the slab distance is `pl.Distance(start)`, which is why the target NEGATES and
ADDS where a subtract would be shorter (`VIPlane::Distance` is inline, and reads
the POINT first). What is open is **two** phenomena, not one: five walkers
differ by ONE spilled pseudo — the byte-exact `CollideBufferPackV` is the same
body with the fetch substituted, so it cannot be the shape — and the three
point/sphere collides differ in the ENTRY BLOCK's schedule, the target hoisting
all six loads before the first subtract, which is the shape
`decomp-scheduler-rig` attributes to the R5900 `load` ready-delay. Finding:
`decomp-phase46-the-collision-core-20260827.md`.
**Phase 45 added the SOCKET LAYER — the floor under phases 43 and 44,
`SUPPORT 0x004acc90..0x004ae088`: five more TUs of the same
C-compiled-as-C++ plus one function added to a sixth, 33 named functions
written and 32 byte-exact.** `uassert.cpp` 3/3, `uerror.cpp` 4/4,
`ipaddress.cpp` 4/4, `unet.cpp` 8/8, `unetutil.cpp` +1, `usocket.cpp`
13/14. The one open function is `usocket_sendto`, **two words of PROLOGUE
SCHEDULE** — nine spellings all score exactly 2/33, which is what settles
it as the standing `sched2` family rather than a source question. Five
file statics were written and are not scored (`NO-TGT` rows, as this
tree's other sixteen unnamed statics have been since phase 39).
**`ipaddress_t` is SIX bytes** with three encodings around it: SCE's
`sceInetAddress` carries the IPv4 host-ordered at +4 so the converters
count DOWN through it, and the WIRE form is the four octets then the port
**little-endian**, which is not network order — and those six bytes are
what `usocket_sendto`/`usocket_recvfrom` prepend to every datagram.
**`unet.cpp` is the only PS2-specific file in the library**: one EE thread
doing `sceLibnetInitialize` over a `memalign(64, 0x10200)` and a blocking
`sceLibnetWaitGetAddress`, with the local address cached as a **STRING**
(`g_szHostName[128]`), which is why `unet_get_local_address` resolves its
own hostname. Every failure path prints and HANGS, and so do both asserts
(`for (;;) ;`, not `abort`) — which is why a tripped `VASSERT` on a console
is a freeze. `usocket_t` is 200 bytes and this file owns eleven; the rest
is `VIInitUSocketEx`'s two EE threads, so for datagrams nothing calls
sceInet from the caller's thread at all. **`forward` is a lag-and-loss
simulator, shipped in the retail binary**: `usocket_sendto_addr` drops the
packet when `rand() * 100 / 2^31 < iDrop`, and when the delay is non-zero
it does not send to the destination at all — it `alloca`s `iSize + 8`,
appends the real destination's four octets, its port in NETWORK order, the
delay in 32 ms units clamped to 255, and a zero, and sends the whole thing
to a global relay address; `usocket_recvfrom_addr` strips the same trailer
off anything arriving FROM that address. **The in-world dump still holds
64.37.152.90:10164 in that global.** `usocket_select` returns 1
unconditionally; `usocket_accept` asserts `!"usocket_accept: not
implemented"` and then creates a socket anyway, behind a hang; and
`uerror.cpp` names the entire SCE INET error enum (-500..-514) as strings,
maps exactly TWO of them, and its default arm still runs the lookup and
throws the answer away. Harness: **a function that never RETURNS lost its
last instruction** — `trim_padding` anchors on the last `jr ra` and
`_uassert` has none, so the trailing-zero rule ate the delay slot of the
`b` closing its `for (;;)`; `eqdecomp.is_branch` and a new paired canary
fix it. Three new `SPAN_CAP`s (`usocket.cpp` has five statics and the map
names none). New levers: **a switch's source order is the OPPOSITE of its
test order**, the first function where phase 27's two halves point apart;
**the shift align-up wants an unsigned divide AND a fresh local**, or gcc
folds the `+ 15` into the relocation addend and a whole instruction
disappears; **a constant serving two purposes is materialised once**
(`unet_startup_async_status` returns the same `1` it stores, and `return
iStatus` costs two moves); **five struct-field assignments are a
permutation with exactly one answer** — 120 orders, one scores 0 and the
next scores 2, so sweep it rather than reason about it; **a two-byte copy
is a `memcpy`, not two `char` assignments** (`lb`,`lb`,`sb`,`sb`, both
loads then both stores, is the inline block move; the written-out form
gives `lbu` and reverses the pair); **the byte swap needs its mask to
order the shifts** (`((x << 8) & 0xff00) | (x >> 8)`, not `(x << 8) | (x >>
8)`); **a copy gcc can coalesce away has to be made to CONFLICT** —
swapping two statements so the send pointer takes its new value while
`pData` is still live forces the extra `move` AND the socket-pointer spill
that goes with it, 31 wrong words to 1; **a drop test returns its own
comparison**, because `return 1` costs a `li` the target does not have;
and **`goto done` is a shape, not a smell** (four forward gotos into one
cleanup, with the first guard written `>= 4` so the error is inline — 41
wrong words at once). `usocket.cpp`'s five asserts land on lines 553, 729,
945, 1116 and 1208, some 1,300 lines for nineteen functions, which is far
more than they fill: the original very probably carried the Windows and the
PS2 back ends side by side under an `#ifdef`, and that is also the only
thing that explains a `select` that returns 1 and an `accept` that is a
hang. Not attempted: `viusocket.cpp`, plus `connection` (101), `drdp`
(105), `route` (40) and phase 44's twelve `endpoint` leftovers.
**Phase 44 added the DRDP TRANSPORT LAYER — five more TUs, 74 written
and 74 byte-exact, 69 on the FIRST build, and the first phase in the
campaign where EVERYTHING WRITTEN MATCHED.** `message.cpp` 27/27,
`endpoint.cpp` 23/23 (partial), `bandwidth.cpp` 12/12, `close_set.cpp`
10/10, `unetutil.cpp` 2/2 — the layer directly above phase 43's
foundation: what a message IS, who owns one, and the two bookkeeping
structures the connection layer sits on. **The fields NAME THEMSELVES,
three ways**: `bandwidth_process`'s four asserts quote
`pBandwidth->m_iBytesSentSinceLastSample`, `TimeSinceLastSample` and
`DRDP_INT64_MAX` verbatim, so the `m_` prefix, the Hungarian and the
unprefixed CamelCase locals are read off the binary rather than
invented; `message_get_key_offset`'s assert is `!"unknown key index"` at
**line 397**, which dates the file; and every `*_get_vnode_offset` /
`*_get_key_offset` / `*_get_vlink_offset` is a literal table of its own
struct's offsets, compiled in. Layouts: `bandwidth_t` 48, `close_key_t`
16, `close_t` 56, `close_set_t` 60, **`message_t` 72 with its payload in
the malloc slack behind it** (`message_get_data` is `this + 1`; a
trailing `m_data[1]` would have made `sizeof` 80 on this ABI, and the
target's 72 says it is not there), `endpoint_t` 176. What the layer
does: **`bandwidth_t` is a leaky bucket and a one-second sample window
in one struct** — `get_flush_time` divides the overflow over the cap by
the limit, `get_process_interval_max` returns SECONDS, and the
elapsed-time subtract is UNSIGNED so a wrapped millisecond clock still
gives the right interval; **the close set holds a closed connection's
(id, instance) for exactly 120000 ms**, on two containers at once, and
DROPS a record whose two minutes are already up at insert; **an
endpoint's "in sync" is a LATCH** (bit 1 of `m_iFlags`, set once and
never rechecked, so it answers "has it ever caught up");
**`endpoint_find_connection` refuses a pair of software endpoints** even
though the tree found the connection; **`endpoint_get_instance` is a
post-increment on a counter seeded from the clock**, so instance numbers
are unique across a restart with no state kept. Shipped: **`message_create`
leaves `m_ucRefnumOffset` and `m_usFlush` UNINITIALISED** and both go
out on the wire; **`message_duplicate` copies the source's endpoint and
connection pointers with it**; **the tag test reads freed memory on
purpose** — `destroy_buffer` writes `"NULL"` over the tag BEFORE the
`free`, and `endpoint_is_valid` then asks the drdp whether it still owns
the pointer, because a freed block keeps its tag until the allocator
reuses it. New levers: **a char ARRAY, not a string literal, is what
makes a four-byte `memcpy` unaligned** — `static const char
MESSAGE_TAG[] = "MESSAGE";` gives the target's `lwl`/`lwr` into
`swl`/`swr` where a literal, which this compiler word-aligns, gives
`lw`/`sw`; four instructions in each of three functions, and the only
evidence in either file for how the constants were declared. **The
allocation check has no house style: it is per FUNCTION** — four
`malloc` tests, all laid out identically in the target, three of them
from `if (!p) return -2;` and `message_duplicate` from `if (p) { ...;
return 0; } return -2;`, which is phase 7's if-direction rule reaching
the error path. Also **ONE variable for a difference and its result**
(`uns32_cmp_wrap`, 7 wrong to 0), **a guard around the whole body beats
an early return** when a later `return -2` would otherwise move out of
line (`close_set_insert`, 15 to 0), **two pointer stores before the
payload read rather than after** (`message_create_buffer` — a statement
order visible only as a register swap, 12 words), **a comparison's
operand order survives to the `xor`**, **a validity check reuses the
first test's result as its return value**, and **a walk that caches the
next node BEFORE the body**, which a `for` increment cannot produce
because calls do not reorder. No new `SPAN_CAP`s and nothing left open.
Not attempted: twelve `endpoint` functions that read `connection_t` and
`segment_header_t` fields directly, plus `connection` (101), `drdp`
(105), `route` (40), `usocket` (14) and `unet` (8).
**Phase 43 added the DRDP NETWORK LIBRARY — six whole TUs, 144
written and 137 byte-exact, 98 on the FIRST build.** `SUPPORT`
0x004acb50-0x004c1ca8 is 586 symbols and 86 KB of **C compiled as C++**
— `unet`, `usocket`, `bandwidth`, `connection`, `endpoint`, `route`,
`drdp` and the containers under them — the biggest unmatched body left
in the module, and the layer the whole game protocol rides on.
`buffer.cpp` 78/80, `vtree.cpp` 30/32, `vlist.cpp` 14/16, `vset.cpp`
7/7, `ll.cpp` 7/7, `rle.cpp` 1/2. **The library NAMES ITS OWN SOURCE
TREE**: `_uassert`'s second argument is `__FILE__` and `eqcalls.py`
reads it out of the relocated dump — `C:/code-frontiers-gm/drdp/`, with
`vtree.cpp`, `connection.cpp`, `drdp.cpp`, `endpoint.cpp`, `route.cpp`,
`route_table.cpp`, `message.cpp`, `usocket.cpp` and `bandwidth.cpp`
named outright; its THIRD argument is `__LINE__`, so `vtree.cpp` is
padded to 215/283/331/365 (phase 27's lever). **The wire format is
settled**: `buffer_append_packed_uns64` IS the 7-bit varint
`protocol-reference.md` calls `Write7BitEncodedInt64`, the signed
packed codecs ARE zigzag — and **the 16-bit one SIGN-EXTENDS**, so a
zigzag result overflowing 16 bits goes out as a TEN-byte varint where
the `uns16` twin cannot; `buffer_init`'s bit 0 means "little-endian
data" and bit 1 "big-endian", decided ONCE against a `little_endian()`
that reads byte 0 of a static holding 1; and the RLE is alternating
runs (zeros to 255, non-zeros to 127, one header byte
`nonzeros << 4 | zeros` when both are small). Layouts: `buffer_t` 24,
**`vnode_t` 16, the TREE's node** (+0 colour, +4 parent, +8/+0xC a
`child[2]` ARRAY indexed by direction; the list's node is modelled as
the same type unioned, unproven — no `vlist` signature names it), `vlist_t` 24, `vtree_t` 36
**with its NIL sentinel inside itself at +0x14**, `vset_t` a `vtree_t`
whose `vnode_offset` doubles as the ELEMENT SIZE. Levers: **`&tree->nil`
written OUT at each use, not held in a local** (a named local is one
pseudo where the target rematerialises the address three times — 8
wrong to 0, twice); **a comparison against 0 that emits `slt` is a
comparison against a VARIABLE** (five spellings of `>= 0` all give
`nor`+`srl`; only `x > less` with `int less = -1` gives `slt`, and
cprop then makes it `li -1`); **a `break` keeps the block where a
`return` moves it out of line**; **a guarded do/while needs the guard
to read a DIFFERENT variable**; **a recomputation moved to the loop's
TAIL shares the load at its head**; phase 7's if-direction ELEVEN more
times. Shipped: **`buffer_appendm_*` ignores `read_only` and neither
`m` helper sets the error flag**; **`buffer_copy`'s FIRST parameter is
the source**, and a destination overflow still advances it;
`run_length_encode` returns the END position after reading and
discarding the start; `vtree_remove` calls `vtree_iterate` twice;
`vtree_get_bh` computes a black height it never compares;
`vlist_remove_by_key` always returns 0. Three new `SPAN_CAP`s, all
unnamed statics. Open: five, none structural.
**Phase 42 added the 2D UI LAYER — three TUs, `VIUI` 22 of 33 written,
`VIUIWindow` 26 of 28 and `VIUITextScroll` 7 of 7, 71 written and 55
byte-exact, 38 on the FIRST build.** The layer every menu, panel and HUD
element is drawn through. **Every atlas entry is named by a ROW of the
texture table and a COLUMN that is the current language**; the two static
tables are unnamed file statics whose shape is read off their two free
accessors — **`GetUITexture`'s bound is 252 and its stride 36, `GetUISound`'s
are 17 and 8** — and **both return entry 0 out of range rather than a null**,
so the `if (e == 0) return -1;` in all fourteen callers is dead code that
still has to be written. `VIUIWindow` is 0x17C bytes with a four-entry
palette `VIUICOLOR` indexes (`sll 4; addiu 304; addu this`, three times) and
an open/close animation whose two floats are the whole state machine.
Levers: **a by-value class parameter that is MODIFIED is a separate local in
the source** — gcc gives an invisible-reference parameter no home slot, so
the modification writes through the CALLER's object (65 wrong words to 0);
**a named local for the size in one arm, an inline temporary in the other**,
counted by how many slots a constructed value is written to (58 -> 0);
**`VIVect2I` needs a zeroing default ctor AND a user copy ctor**, correcting
phase 9, with no regression in the three older TUs that use it; **the
statement order inside a loop tail** (four orderings scored 20, 39, 26 and
0 — the induction step FIRST); **the ternary inside the call, not a named
float**; and **phase 16's hoist gets its missing half: a ZERO store needs no
register, so it is never hoisted**. Shipped: **`VIUIWindow::Text` computes
two values and throws both away** — it row-centres the text and then passes
the original `x`, and fades the colour's alpha and then passes the CALLER's
colour; the four-vector `Init` drops `GetHeight`'s answer; `VIUI::Init`
returns `VIStringTable::Load`'s value rather than 0/-1; `SoundPlay(const
char *, float)` returns 0 on both failure paths. Open: the six
`DrawStringEx`/`DrawCenteredStringEx` overloads are 14 words each and ONE
instruction apart — **`cse` merges the address of the saved font colour,
formed by `move_by_pieces`' `copy_addr_to_reg`, with the same address at the
final `SetTextColor`**, and **neither `-fno-gcse` nor `-fno-cse-follow-jumps`
changes it**. Not attempted: the four frame builders (**`VIUIFrameParam` is
64 bytes, eight `{VIUITexId id; unsigned int flags}` pairs**), the six
`BlitBar` overloads, `ReloadWords`, and `VIUIWindow`'s
`Draw`/`TextBox`/`DrawTextInternal` family.
**Phase 41 added the ASYNCHRONOUS LOADER — three whole TUs, `VILoader` 21
of 23, `VIActor` 3 of 3 and `VICSFFile` 5 of 7, 33 written and 29 byte-exact,
20 on the FIRST build.** The loader is a queue of transactions over ONE bounce
buffer with ONE `VIFile::ReadAsynch` in flight; `Process` advances at most one
per call, `Align` rounds the read out to the file's own asynchronous alignment
at both ends and carries the slack in `m_offset`, and a DictID map dedups so a
second `Load` of the same resource just bumps a refcount. **A `.CSF` is a
40-byte header (`'CESF'`) plus N zlib streams of at most 256 KB**, and
`Decompress` opens the inflated SET as one `VIFile` scatter/gather memory file
— which is why `VIObjFile` reads a `.CSF` and an `.ESF` identically.
**`VIESFParse` is 1064 bytes, not 1072** (phase 36 measured a stack local's
frame; a MEMBER settles it), and **`VIResourceDir` is a real class that emits
NOTHING** — it derives from `VIArray<VIResourceElem>` and adds no member, so
its synthesized ctor and dtor inline to the base's and only a mangled parameter
proves it exists. **`new T[n]` for an element WITH a destructor carries a
16-byte cookie** (phase 27's `VIQueryTri` is the control) and `delete []`
destroys BACKWARDS off the count at `m_files[-16]`. Levers: **ONE shared local,
not two** — phase 40's rule run backwards, twice; **two guards are one `||`**,
or the first `return -1` will not cross-jump with a later one (a LENGTH
difference); **nested guards share ONE return at the bottom**, fixing an early
return and an if-conversion at once; **the success path in the `if`** twice,
once so gcc reuses a `v0` that already holds zero; **a declaration order
decides which store fills a ctor call's delay slot**; **a remainder written
back into ITSELF** is what makes an align-up a `movz`; **`& ~63u` is
`lui`+`ori` where `& -64` is one `li`**; and **`sw` on a zlib length says
`uLongf` is FOUR bytes here**. `VIActor` is 160 bytes with the ROTATION at
+0x50 and the translation at +0x44 — only `CalcObjectToWorld`'s argument
registers say which. Shipped: **`VILoader::Load`'s capacity check is
inverted** (`size > bufSize + 2*align`, where it should be
`size + 2*align > bufSize`), `Complete` discards `VIObjFile::FileType`'s
answer, `Compress` closes its input twice, `Decompress` leaks on every failure
path, and `VIActor` never initialises `m_scale`. Two new `SPAN_CAP`s. **Grep for an existing STUB before adding
a class to `vi.h`** — `viarray.cpp` had modelled `VIResourceElem` as four bare
`int`s and the collision failed the whole TU, taking 126 `VIArray` symbols out
of the score. **Do NOT run `_q.py` while `eqcheck.py` is running** — they share `decomp/build` and a
parallel run reports phantom regressions everywhere. Open: `Find` 17/49 and
`Align` 17/48 are instruction-for-instruction identical and differ only by
register; `Decompress` 6/155; `Compress` 26/286.
**Phase 40 added the GS VRAM ALLOCATOR — `VIEESurfCache` in one TU, 15 of
its 21 byte-exact, 12 on the FIRST build**, plus
`VIMultiMap<unsigned,int>::Key`/`LowerBound` and two never-instantiated
destructors. `VIRaster + 0x3298`: one span of video memory, three containers
over it and an LRU. **The free map's key is the design** — a bucket carries
the GS PSM it is formatted for, or **255 for virgin memory**, and the key is
`(psm << 24) | size`, so a `LowerBound` finds the smallest run already in the
right format and a second one on `(255, size)` falls back to memory that may
be reformatted; a GS page is 8192 bytes and its layout depends on the psm,
which is why every split and merge is page-aware. Corrects three docs:
**`VIMaterialLayer` is really `VIMaterialPass`** (`Set`'s third parameter
mangles `R14VIMaterialPass`) and its +0x48 is the **CACHE STAMP**, not a pass
index; **`VISurface + 0x18` is the cache index and +0x14 the surface's own
pool slot**, which is the info map's key; and **`VIPixelStore` 3 and 4 are
both right at once** — 3 is 24-bit colour in a 32-bit slot (PSMCT24) and 4 is
the packed form, which `CalcCacheInfo` rejects, so phase 30's table and the
enum before it were each half the answer. Levers: **which of two adjacent
stores ends the block decides whether cross-jumping eats it**; **the align-up
idiom is an if/ELSE with a fresh intermediate** (27 wrong words to 6); **a
SECOND LOCAL for the same index** three times (23 to 0 in `Decache`); **two
assignments per arm is what blocks if-conversion**, which is why `Set` loads
the same `-127.0f` twice. Layouts: `VIEESurfAlloc` **12**,
`VIEESurfCacheInfo` **84**, `VIEEBucket`'s four fields named. Shipped:
**`FindLRU` returns 1 on success**, **`Cache` returns 1 only when it actually
uploaded**, **`DebugVerify` is `return 0;`**, **`Set` writes MIPTBP2 from
levels 1..3 for every non-indexed texture** and puts TEX1's LOD constant in
bits 0..7 on top of MMAG/MMIN, and `CalcCacheInfo` ignores `this`. Two new
`SPAN_CAP`s. Open: six, five of them one instruction — and one standing
question, `int f = 1; if (c == 0) f = 0;`, which nine spellings all collapse
to `sltu` where the target keeps the 1 in a register.
**Phase 39 added the SOUND DEVICE — `VISoundDevice` in one TU, 50 of its 60
symbols byte-exact, 40 on the FIRST build.** The EE half of the sound system
and it makes **no `sceSpu` call at all**: every operation is appended to a
packet by a file static and shipped over SIF RPC, so the class is ordinary
C++ over `VILList`. Those five statics needed a new `SPAN_CAP`, and **the
canary picked the number** — 308, not 312, because a cap must land one word
past the `jr ra`, taking the delay slot and not the padding nop.
**`BgmBufferSize` is NOT static and `SetBgmVersion` IS**, and only the CALL
says so (both read the same file-scope static and mangle without a `this`);
the wrong choice made `Init` come out one instruction SHORT with every other
word identical. **A float stored into an `int` is `cvt.w.s`+`mfc1`; into an
`unsigned int` it is a CALL to `fptoui`** — every volume word is unsigned and
every pan word signed, eight sites. **The stereo gate is an `if` on the
LOCAL** and both directions ship: `if (m_stereo == 0) pan = 0;` gives `movz`,
`int p = 0; if (m_stereo) p = pan;` gives `movn`, and two stores or a ternary
give a branch — one edit each closed `SetVolume` and `SetPan`. **The clamp is
`VIMIN(VIMAX(lo, v), hi)`**, whose only trace is the operand order of the
first `c.lt.s`. **The `-1` sentinel is a signed `int`** (`li -1` against
`lui 0xffff`+`ori` for an `unsigned`). **The same expression is 32-bit in
`SetPitch` and 64-bit in `Play`.** **The type test is two `if`s in four
functions and an `if`/`else if` in three**, and in `SetPitch` it decides the
frame size. **A shared address base is what puts a search on one walking
pointer**, and the loop must be a `do`/`while` or gcc peels its first
iteration onto constant displacements. **`Reset` is not `Init` with a
prefix** — three subsets of one field block, read back off Phase 16's hoist.
Layouts: `VIPlayback` **56** on a `VILNode`, `VISpuBlock` **24**,
`VISoundDevice` >= **0x117C** with four 256-byte BGM name buffers, and
**`sizeof(VIWave)` 92 / `sizeof(VIXm)` 64** straight off `CreateSound`'s two
`__builtin_new` arguments — the first independent confirmation of phase 17's
layouts. **The playback handle is packed exactly as `VISpellEffectSystem`'s,
serial seeded at 0x16062**, and `Playback` needs `(handle & 0xff) % 46` so the
remainder comes out `div`. Shipped: **`EnableBgmFading` is EMPTY**,
**`SetBgmVolume` and `IsBgmPlaying` both IGNORE their slot** (so
`SetSoundClassVolume` ends with two self-assignments), **`Stop` returns
nothing**, the XM allocator **floors every request at 0xC000** when
`sBgmVersion == 1` and `FreeXmMem` applies the same floor, and **`Process`
starts ONE transfer per frame** while restarting its 128-entry scan from zero.
Open: ten, none structural.
**Phase 38 added the SPELL EVENT DRIVER — `VISpellEventDriver` in one TU,
28 of its 35 byte-exact, 20 on the FIRST build.** **The destructor says
what a pointer POINTS AT**: the deletes at +0x28/+0x2C are
`_$_11VILightning`, so `spell-event-driver.md`'s "reserved" pair is the
bolt pair, and +0x30, which IS a `VISpellActiveEvent`, is never freed.
**A bit test that becomes a 0/1 VALUE is `int x = 0; if (bits & m) x = 1;`**
— every predicate spelling (`!= 0`, `!!`, a ternary, a `bool`, an inline
accessor) gives `sra`+`andi 1` and never materialises the mask, where the
target has `lui`+`and`+`movz reg,zero`. **ONE variable, not a separate
`t`**: reusing the result variable for a division puts the quotient in the
RESULT's register, which closed both interpolators two words each after six
other spellings had not. **The two guards of a function need not be the
same shape** (one early return, one `if` running to the bottom). **A switch
whose cases all retire the same object shares nothing after it** — the
out-of-range `sltiu` targets the bare `return 0` the retire falls THROUGH
into, so a tail after the switch would sit on the default's path (5 wrong
against 69). **The completion check is NESTED**, not a flat chain of
`if (x) return 1;`, whose last test if-converts to `sltu` (47 -> 0).
**Reading a field TWICE is the else arm**; **a `bool` argument is compared
against 1** (`active == true`); **the jump table gives the case ORDER as
well as the labels**, twice (the dispatch switch runs 1, 2, 3, 6, 5, 4, 7,
8, 11, 12, 9, 10, 13..17); and a two-field float zero is a CHAIN whose
stores come out LEFT to right where phase 7's came out right to left.
Layouts: the driver is 100 bytes, **`m_anim` is an ARRAY of four**
(`ProcessWaitEvent` indexes it), **`VISpellActiveEvent + 0xB0` is the
64-bit start time** that forces its alignment, and **`VIHSprite` reaches at
least +0x188**. Shipped: `ProcessCreateSound` builds a `VIMatrix33` nothing
reads, `NewCreateSprite` drops a `GetActorSpritePtr` result,
`NewPlayTargetLightning` declares three zeroed `VIVect3` it never uses,
`NewSetAction`/`NewPlayAttack` return **-1** for a non-`VICSprite` actor,
and `NewSetEmitterMotif` takes an in-flight blend OVER rather than
restarting it. Open: four, none structural — the two `ProcessCreate*` twins
rematerialise `-1` three times where ours keeps it in a register, and
`ProcessWaitEvent` is four words of `a2`-vs-`a0`. Not attempted:
`ProcessProjectileEvent` (1187), `NewPlayTargetProjectile` (609),
`ProcessCreateEmitter` (420), `ProcessLightningEvent` (399).
**Phase 37 added the SPELL EFFECT SYSTEM and a sweep of scene leaves — 66
written, 65 exact, 35 of them on the FIRST build — and closed a function
open since phase 31.** `VISpellEventList`+`VISpellEffect`+
`VISpellActiveEvent` 11/11, `VISpellEffectSystem` 24/24, `VIList<T>`'s
missing `Size`/`Begin`/`Next`/destructor over five instantiations 14/14,
the four remaining filter overrides 8/8 with their `__tf`, and
`VIEmitterQuery` / `VISceneSoundTrack` / `VISceneDisplayElem::Compare`
7/7. **The nested class is `VISpellEffectPbi` — SIXTEEN characters — and
the two functions taking it have no second parameter**; the obvious
`VISpellEffectPb` + trailing `int` split is one character short and the
code touches only `a0`/`a1`. **A playback HANDLE is not a slot index**:
`Play` packs the slot into the low byte and a counter seeded at `0x16062`
above it, `Playback` checks both halves, and the remainder is a real
`div` — which needs `(handle & 0xff) % 24`, because a cast to
`unsigned char` lets gcc prove non-negativity and emit `divu`. **Three
guards are TWO statements** (one `||` scores 3, three `if`s score 13,
either two-way split is byte-exact) with `handle` declared BEFORE them so
its `-1` goes callee-saved. **Phase 16's hoist read backwards**: the LAST
constant store is the hoisted one, so `m_inited = 1` closes `Init`.
**Elapsed time is an `int`** — `Process` sign-extends the 64-bit
difference before comparing. **A `__tf` that will not match is a TU
question, not a class question** (a derived `__tf` inlines the base's only
when they share a TU). **The accepting direction is the one that
branches** — `VIWalkCameraFilter::FilterActor` is one `||` returning 1,
and both `&&` spellings cost exactly two words. Shipped:
`VISpellEventList::Destroy` is the only owner of an event's particle
definition or sprite, `ReleaseAllSpellEffects` re-reads `Begin()` every
iteration and never calls `Next()`, `SetVibrateCallback` returns the
previous callback, and `Play` hands `Trigger` 10000 while timing the
playback out at 60000. Harness: **`eqcheck.py` retries the assembler
once** — a spurious wine failure had produced a whole phantom regression.
Open: two, both `PointInSolid`.
**Phase 36 added the DEVICE CLASSES — `VIPad` 13/14, `VIKeyboard` 8/8 and
`VIBattleMusic` 7/8, 28 of the 30 written, no re-tuning.** **An `ld` is NOT
proof of a 64-bit member**: three `VIPad` methods load eight bytes at +0x20 and
compare against 2 while `Read` reads the same offset with `lw` and `Init`
zeroes +0x20 AND +0x24 with two `sw` — **gcc 2.95 merges two adjacent `int`
compares into ONE 64-bit load** (`m_state == 2 && m_actAligned == 0` IS "the
eight bytes equal 2" on little-endian), and four spellings give identical
output, so an access WIDER than its field means a merge, not a wide field.
**An inline predicate returning `int` MATERIALISES the comparison**
(`sltiu`+`dsll32`/`dsra32`) where the test written out branches directly.
**Phase 7's if-direction five more times**, and in `VIPad::Read` it decides the
whole 290-instruction layout (the ready path is the `if` BODY and the teardown
the function's TAIL — 86 of 290 words). Plus **phase 23's `||`** (two
`if (X) return 0;` cost a `b`), **a nested ternary rather than two returns**
(`Preload` closes on `move v0,a1`), **the guard BEFORE the assignment** (gcc
steals it into the delay slot), and phase 35's count-read-once and phase 16's
hoist again. **`VIPad` is 96 bytes and closes exactly** on `scePadRead`'s
32-byte buffer at +0x3C; **`VIConsoleKey` is TWO bytes, not sixteen** (phase 27
guessed 16); **`VIScene+0x4BA4` is the `VISoundDevice *`**. Shipped:
**`VIPad::Read` initialises a 32-byte local it never reads** (the alignment it
sends is a file static, and gcc does not delete a dead initialised array), every
button is the COMPLEMENT of one bit with `(b ^ 0x80) >> 7` for the top bit only,
the stick dead zone is 88..167, and **`VIBattleMusic` has two back ends behind
feature bit 0x100** where **track 9 means "pick one of eight at random"**. Open:
two, both allocation (`Read` 6/290, `Free` 2/17).
**Phase 35 added the LAST SPRITE LEAF and four more classes — 45 of the 47
written, 33 exact on the FIRST build — and CLOSED TWO FUNCTIONS OPEN SINCE
PHASE 20.** `VIPointLight` 11/11, `VICSpriteCust` 9/9, `VISpellEffectDriver`
14/14, `VIRadialFloraSystem` 7/7 and `VIIconDefinition` 4/6. **Phase 7's
if-direction, fifteen phases late**: `VIPointSprite::Raster` and
`VIStreamAudioSprite::Raster` were six words wrong since phase 20 and phase 32
filed it under the register-allocation phenomenon — it is the guard, and the
nested `!= -1` form is byte-exact (a NEIGHBOUR matching with one direction is a
reason to try the other). **`VIPointLight` IS `VIPointSprite`** — five of its
functions are byte-identical IN THE IMAGE, and only the relocated dump can say
they call `GetLightUISprite` rather than `GetPointUISprite`; its vtable is 96
bytes (phase 19 a ninth time) and it overrides only ONE `Collide`. **The radius
is read THROUGH the sub-object pointer** the `VIVect3` copy already made
(phase 9, 14 of 46). **A magic word is an IMMEDIATE, not a `memcpy`** —
`'PS2D'` as `lui`+`ori`+`sw` took `VIIconDefinition`'s 133-instruction ctor to
byte-exact on one line. **An uninitialised local is a source fact**:
`Ascii2Sjis`'s `run` is left uninitialised so the digits arm's own `run = 0`
survives (30 wrong words against 13). **Phase 16's hoist reads a statement
order out of the object code and TWICE here it is not the field order** —
`VISpellEffectDriver::Destroy` (all 720 orderings swept, one exact) and
`VIRadialFloraSystem`'s ctor. Corrects `vicspritecust-tables.md` twice:
**`$Tints` is FIFTEEN entries** and **`$Race`'s rows are `[4][2]`**. Shipped:
**`GetArmorSetTexture` ignores its race** (`li a0,20` in the guard's delay
slot), `VIRadialFloraSystem::Reset` returns **void** among `int`s,
`VISpellEffectDriver::Trigger` refuses while its driver list is non-empty, and
**two of the three lights in a card icon are BLACK**. Open: two, both the
standing allocation phenomenon.
**Phase 34 added the EFFECT VOLUME, the BOID, the ZONE ROOM and the last three
VALUE POOLS — 74 of the 79 written, 53 exact on the FIRST build.**
`vieffectvolume.cpp` is 28 of 29 (`VIEffectVolume` 24 written, `VIBoid` 5 of 5),
the three new `VIPool<T>` instantiations 35 of 38, `VIZoneRoom` 10 of 11, and
`VIEffectVolumeSprite::Copy` — open since phase 32 — closed by a LAYOUT
correction, not by codegen. **A non-annulled branch's delay slot reads out which
statements are OUTSIDE the `if`**: `SetType`'s `bc1f` carries the store of
`m_type`, so that store runs on both paths and `m_type = t;` sits ABOVE the
guard; written inside it costs 9 of 24 words **and swaps `this` and `t` between
`s0` and `s1`**, because the right spelling kills `t` at the call and its
priority (refs / live-length) overtakes `this`. `VIBoid::Free` and
`VIEffectVolume::Process` are the same shape. **The mangled name is the argument
list** — `Init__6VIBoid` takes NOTHING, and inventing two arguments for it left
17 of 43 words wrong in `AllocFlock`, all of them that same register swap.
**The jump table decides the case labels** (phase 30's rule, forward): the
five-entry table at `0x004EABF0` says `AttachSurface`'s cases 1, 3 and 4 SHARE a
body, and three distinct bodies written as an if/else chain emit no table at
all. **`VIPool<T>::Insert` spells the sentinel OUT** — `lui 0x7fff`/`ori 0xffff`
with no call, where every other use of `End()` in the file is a `jal` and `Prev`
four functions down still calls it. **`effect-volume-system.md`'s field table is
off by one slot**: `Density()` is `lw v0,44(a0)`, so `m_density` is +0x2C, and
renaming the four fields `Copy` touches took it to byte-exact; **`VIScene+0x4B9C`
is the `VIDictionary *`**, in front of the raster pointer at +0x4BA0. Layouts:
`VIEffectVolume` **112**, `VIBoid` **176**, `VIZoneRoom` **120** — exactly the
stub size phase 24 measured off the `VIArray<VIZoneRoom>` node stride, so the
stub is gone. A zone room is `VICollBuffer`'s shape a third time (one
`new char[]`; the section pointers are byte OFFSETS until `Init` rebases them,
a portal is 32 bytes opening with a `VIPlane`), and `CalcPortalPlane` is
**Newell's method with the centroid accumulated in the same loop**, its
`Normalize` result compared against `0.0f` through a `cvt.s.w`. The new pool
elements: `VIEEBucket` and `VIRasterMaterialLRU` 16 bytes and POD,
`VILoaderTrans` **48 with alignment 8**; none takes phase 25's
`VIPOOL_VALUE_COMMON` macro, and `MoveToLast` (the LRU touch) matched byte-exact
first build at 55 instructions. Shipped: **`VIBoid::SeekGoal` is EMPTY**
(`jr ra; nop`), `VIBoid::Apply` and `VIEffectVolume::Process` both ignore their
`dt` (Process takes the time from `VITimer::GetTime()`, skips below 1/60 s and
clamps at 0.1 s), `AllocParticles` calls `FreeParticles()` and then calls it
AGAIN in its type-0 arm, `AttachSprite(-1)` returns -1 without detaching, and a
boid seeds every field from **two `rand()` draws differenced and scaled by
2^-31** — sixteen calls in one `Init`. Four new `SPAN_CAP`s. Open: five —
`AllocParticles` 5/103 (the `movn`/`movz` clamp direction; twelve spellings
swept, and phase 27's ternary rule does not reach it because gcc always
coalesces the result with the value), `CalcPortalPlane` 37/145 (its loop body is
exact once Newell's three terms precede the centroid accumulation) and the three
pool `Allocate`s, which are phase 25's standing miss.
**Phase 33 added TWO WHOLE TRANSLATION UNITS — the motion trail and the
lightning bolt — 33 of 35 symbols, 25 exact on the FIRST build.**
`vitrailfx.cpp` is 16 of 16 (the `_GLOBAL_$I$`/`$D$` pair came free);
`vilightning.cpp` is 17 of 19. `VILightning` is **8832 bytes**, exactly the
allocation `spell-effect-system.md` measured: a bolt's shape is the sum of
**three harmonics** of interpolated 17-sample noise (0.57 at 0.64 Hz, 0.15 at
10.24, 0.08 at 44), subdivided along the segment in three passes, with branches
forking off **point 3*i of BOLT 0** and a **three-frame after-image ring** that
Render draws oldest-faintest. **A pointer copy of the array decides which of
three loop shapes gcc emits** — the plain subscript is REVERSED into a countdown
on `bgez`, an explicit end pointer gives `sltu` plus an entry guard, and
`VIVect3 *s = m_samples[which]; … s[i]` gives the target's forward walk on a
SIGNED `slt` with no guard. **A returned temporary's stack slot is reused by the
next declaration**: `r.Color(VIColor32F(…));` frees `sp+0`, so a `VIVect3`
declared AFTER the call lands on it — 127 wrong words to 0 — and the same
sharing is why a `VIVect2` handed to `UV` and a `VIVect3` handed to `Vertex`
both live at `sp+32`. **Where a value is computed relative to a CALL decides its
register class**: two integer assignments moved above two calls go callee-saved
and push the `VIMatrix44 &` parameter out to a stack slot, 76 wrong to 35.
**`VIMIN(3, x)` gives `slti x,4`+`movn` where `VIMIN(x, 3)` gives
`slti x,3`+`movz`**, and **`VIMAX(x, 70)` emits `li 69`** — gcc canonicalises
`x > 70` to `69 < x`, so a constant in the object code is not the source's.
**Statement order inside an `if` body decided a branch-likely** (27 wrong to 1).
Shipped: **`VITrailFx::ClearSegmentsForward` retires at most TWO segments** — it
re-reads `m_next` through its ARGUMENT rather than the cursor, and once the
first node is on the free list that read returns the list's tail sentinel;
`VIBoltHarmonic::Create`'s `count` argument is stored and never read; and
`VILightning`'s fourth ribbon is the negation of the third, which only works
because `off` is not re-zeroed per iteration. Harness: **`eqcalls.py` now reads
STRING LITERALS out of the relocated dump** (`addr  "lightning.tga"` where
`name_at` could only say `_vt$9VIHSprite+0x90`) plus one new `SPAN_CAP`. Open:
two, both the standing register-allocation phenomenon.
**Phase 32 added the REMAINING SPRITE LEAVES — `VINameSprite` 16 of 17,
`VISoundSprite` 13 of 15, `VIStreamAudioSprite` 14 of 16 and
`VIEffectVolumeSprite` 10 of 12, 53 of 60 with 41 exact on the FIRST
build.** The vtable is **96 bytes in all four**, so phase 19's rule holds a
fifth through eighth time and no leaf in the engine adds a virtual.
**`int k = 32; while (k--) *d++ = *s++;` is not any `for` loop** — gcc
reverses a forward loop into a countdown exiting on `bgez`, and only a
source already counting down through zero gives the `bne` against -1 (92
wrong to 0, with the record copied through POINTERS not subscripts).
**An inline member's `this` is the address pseudo a named field is not**:
`at.Set(0,0.6f,0)` parks `&at` in a callee-saved register across the two
intervening calls where `at.x = ...` keeps every store on `sp` — phase 9's
lever through an inline member, the inverse of phase 31's. **`float d[3]`
is not `VIVect3 d`** (an array has no ctor, and gcc will not delete a
ctor's zero stores across a call). **A `void` return frees `v0`**, a new
lever. **`_GLOBAL_$I$` is named after the first global OBJECT**, so a bare
`int C::N;` goes to `.comm` and hands the name away; `= 0` matches two more
functions. Shipped: **a name plate is a TWO-PHASE sprite** — `::Raster`
only enqueues into a 24-entry table and a STATIC `::RasterDeferred` sorts
by depth, drops every plate whose screen rect overlaps one already kept and
draws the rest; its off-distance test sums the homogeneous `w²`, so 22.0f
is not a distance; `VISoundSprite::Release` calls `IsPlaying`/`Stop`
BEFORE null-testing the device; `VIStreamAudioSprite::QueryEmitters`
RETURNS -1 for a priority outside 0..2 and evicts `Begin()`, the SMALLEST
key (which puts `VISprite::QueryEmitters`' guessed `Last()` in doubt); and
**`VIEffectVolumeSprite` draws AND picks with the STREAM AUDIO debug
marker**, copies its volume's density twice, and steps the effect on a
hardcoded 1/30 s. Open: seven, two of them named phenomena — **both**
`QueryEmitters` in the corpus fail on phase 19's `gcse` merge, and
`VIStreamAudioSprite::Raster` is wrong in exactly the same six words as
`VIPointSprite::Raster` has been since phase 20.
**Phase 31 added the CAMERA GROUP — `VICamera` 8 of 10, `VIFlyCamera` 10 of
the 13 written, `VIWalkCamera` 19 of the 20 written and `VIWalkCameraFilter`
2/2, on the settled toolchain with no re-tuning.** `VIWalkCamera` DERIVES from
`VIWalkControl`, whose semantics `go-eqoa-vi/viwalk` had already checked against
the console, so its character half was settled before a line was written.
**The orbit body is a real nested TYPE carried TWICE** — `InitCameraIntermediate`
copies +0x1A0..+0x1F0 onto +0x1F8..+0x248 through a POINTER to each half, so the
+0 field folds back onto `this` and every other one comes off the pointer, which
a flat member list cannot produce. **An address NAMED before a call cannot be
coalesced into the argument register**: a local reference declared at the TOP of
`VICamera::Update` parks `&m_view` in a callee-saved register (its live range
crosses the intervening call) while the member name keeps every field store on a
displacement — 161 wrong words to 0, and writing the stores through the
reference too is Phase 9's lever and is WRONG here. **An inline helper's
temporary is a NEW temp each expansion** — same stack slot, distinct address
pseudo — so `CalcFrustum`'s eight NDC corners must be eight braced blocks at
function scope for gcc to merge their sret address into one callee-saved
register; Phase 19's `gcse` finding with the roles reversed. **A store-to-load
forward is not a re-read of the source**: the view matrix's three translations
read the COLUMN back out of `m_view`, where a second read of `right.x` must be
re-loaded because the stores above it may alias the caller's vector. **`VIMIN`/
`VIMAX` are MACROS**, so `VIMIN(1, VIMAX(-1, d.Magnitude()))` emits THREE calls
plus a cross-jumped copy of the next one; bound to a local it is one — and the
clamped value is then dropped on the floor, in both `Forward` and `Hover`.
**A pointer variable must be declared where it is USED** (one held from the top
of `VIFlyCamera::UpdateCameraPhysics` is live across `fmodf` and goes
callee-saved), and that function uses **ONE** damping local for both `powf`
results, which is what makes both definitions a `mov.s`. Phase 9's
assign-then-modify-in-place took `SetOrbitAngle` from 27 wrong to 0. **The flag
words are touched 32 BITS AT A TIME and UNSIGNED** — `lw`/`ori`/`sw` on a field
Phase 18 proved is a `long`, with `lui 0xffff`+`ori` as ONE mask shared by both
words. **Phase 16's store-hoist rule and Phase 21's `lui`-order rule DISAGREE**
on `VIWalkCamera::Init` and cannot both be source order; the `lui` order is kept
(29 wrong against 40) and the function is open. Layouts: `VICamera` is 0x1F0
with four matrices and a `VIFrustum` at +0x130, and its `Init` is the only caller
of `VIRaster::Frustum`; `VIFlyCamera` is `VIWalkControl`'s underived little
brother carrying its six-vector body twice 0x60 apart. Shipped: **the orbit
PITCH velocity is multiplied by ZERO every step** (the damping is a `VIVect3`
whose x alone is written), `OrbitIn` declares two stack vectors it never reads,
`VIFlyCamera::Init` leaves the committed velocity and force uninitialised,
`SetOrbitAngle` calls `fmodf` twice on one argument, the two zoom-snap arms
disagree about clamping, and **`VIWalkCameraFilter` rejects every `VICSprite`**
so the third-person camera swings through players and NPCs. Two new `SPAN_CAP`s:
without them `FindBestOrbitAngle` reads as 804 instructions instead of 188.
**`FindOrbitAngle` and `FindBestOrbitAngle` are byte-exact** — the eight-angle
camera sweep, at the caller's angle and +-45/90/135/180 off it, the first taking
the first clear angle and the second the one whose sweep got furthest; the
second matched on the FIRST build. Not attempted: `CalcPick`,
`CalcPickFrustum`, `MoveCamera`.
**Phase 30 added the FONT LAYER — `VIRFont` 24 of 27 and `VIFont` 13 of 13, 30
of the 40 exact on the first build — and CORRECTED `VIPixelStore`, wrong since
Phase 16.** That enum was read off the ORDER of `CalcMipLevelStride`'s case
BODIES, not off the jump table; the table (0x004EA560) says **RGB24 is 4, not
3**, that 3 is a second four-byte store rather than the default, and VIRFont's
two switches add a **seventh** value, 6 (`sltiu 7`). It survived byte-exact for
fourteen phases because **the table is data and the differ compares
instructions** — a wrong label set with the right block count, order and bound
emits identical code. `VIRFont` is the decoded font (one `new unsigned char[]`
block whose four sections are byte OFFSETS until `Init` rebases them) AND an
LRU cache of the glyphs resident in a 256x256 `VISurface`; `VIRFontChar` is 12
bytes, `VIRFontSlot` 56 with `VIVect2 m_uv[4]` at +0x18. Levers: **`out[n] = c`
is not `*p++ = c`** — a hand-written walking pointer coalesces with the
parameter, where the subscript lets gcc's `loop` pass build the induction
variable out of a COPY of it, and one line took both 257-instruction `Cache`
twins from 32 wrong to byte-exact; **a slot and its store are separately
controllable** (Phase 13's rule, both halves at once — `int n; int locked = 0;
… n = 0;`); **the I4 rounding is a BRANCH**, not an if-converted add; and **a
binary search computes its exit test TWICE**, so a folded `while` condition
CSEs them where an explicit `break` does not. `VIFont` is nothing but the text
colour, initialised in the **mem-initialiser** (a body assignment would leave
`VIColor32F`'s zeroing ctor in front); every lookup re-hashes **`"ArialUI"`**
on every call into a static of its own; `SjisToJis` is the corpus's first
end-to-end 64-bit function (`unsigned long` is 8 bytes) and its constant is
**0x2021, not 0x2121**, because gcc 2.95 will not distribute a constant add
across a left shift. Shipped: `Lock` ignores its raster, `Cache` reports its
overflow only after evicting the glyphs it just cached, and `m_maxSlots` /
`m_numSlots` are computed by different formulas. Open: `BeginChar` 7/14
(registers), `InitSlots` 27/132 (one `addiu`; eight spellings swept and gcc
folds every one) and `CreateResources` 25/127, where **the count is a shift and
the spelling that scores 13 is the wrong one** — it reorders the blocks.

**Phase 29 added the ESF/CSF CONTAINER — `VIObjFile` 59 of 62, plus
`VIFile::~VIFile`, which was missing from the tree entirely.** The layer every
`.ESF`/`.CSF` is read through, so its semantics were never in doubt and every
miss was codegen; 21 exact on the first build, same toolchain, no re-tuning.
**`VIObjFile` HAS a `VIFile`, it does not DERIVE from one** — the destructor's
`__in_chrg` argument is the only witness (a BASE gets it forwarded; a MEMBER is
called with 2 and the class does its own `__builtin_delete`), measured on the
compiler both ways, and the layout is identical either way. **A chain of guards
that all return the same value is ONE `&&`/`||`, not N `if`s** — written as a
list, gcc emits a trampoline for the first and **if-converts the last test to
`slt`+`movn`** where the target branches (14 spellings swept); written as one
chain, `ReadDirBegin` 8/26, `ReadBegin(unsigned long,…)` 26/38 and both `Copy`
overloads (38/166, 34/155) all go byte-exact on that one line — and `&&` and
`||` are NOT interchangeable. **Where a global is READ is a source fact**: a
global load cannot move across a call, so `int endian = VIEndian;` must sit
exactly where the `lw` is — one line earlier took `WriteEnd` 32/109 to
byte-exact and gave back a whole callee-saved register. Phase 20's per-function
if-direction in bulk (ONE flip took all twelve `Write` overloads, a DIFFERENT
one all twelve `Read`s), Phase 14's fresh local for the intermediate in
`unsigned short` arithmetic (`ReadStringBody` 28 → 0), and **Phase 16's
constant hoist used FORWARD for the first time** — `OpenObject`'s thirteen
stores and five constants written straight off the rule, exact on the first
build. Layout **592 bytes** (off `Append`'s frame, which holds one as a local):
`VIFile` +0, `m_mode` +0xC0, a 32-byte header +0xC8 with two eight-byte
`long`s, `m_level` +0xE8, `Dir m_dirs[10]` +0xF0 (12-byte wire record, `long`
offset at +16, `int` remaining at +24), eight `int` flags at +0x230; magic
`'OBJF'`, open flags 33/50/42 for Read/Update/Create. Shipped: **`Copy` leaks
its 4 KB bounce buffer** on the read-failure path, **the four-argument `Copy`
never writes its `size` out-parameter** (the fifth argument arrives in `t0` and
is never touched), `WriteString` swaps the length in place and then uses the
swapped value as the byte count, and `ReadBegin(offset,…)` resets `m_level` to
-1 before seeking. Open: `WriteBegin` 25/145 (the target rematerialises the
constant 1 into a second register mid-function), `ReadBegin(4-arg)` 23/131 (two
instructions, the `Eof()` guard's delay slot) and `PostOpen` 3/78 (one branch).
**Phase 28 added the PARTICLE SYSTEM — five classes, 77 of the 84 functions
written, 60 byte-exact on the FIRST build**: `VIParticleMotif` +
`VIParticleDefinitionEx` 24/24, `VIParticleDefinition` 10/10,
`VIParticleSystem` 10/12, `VIParticleEmitter` 23/25, `VIParticleSprite` 10/13.
Everything here stands on classes earlier phases settled, so almost every miss
was codegen. **`sizeof(bool)` IS FOUR on this ABI** — a `bool` member stores
with `sw` and passing one to a `bool` parameter emits nothing, where an `int`
emits `sltu`; invisible for 27 phases because no earlier class had a `bool`
member. **Phase 27's inline `Value(i)` fixed all four pool walks in one edit**,
where Phase 22's pointer variable could not (twelve spellings swept).
**Phase 16's constant hoist is completed: the hoisted stores keep their SOURCE
order**, which reads a constructor's statement order straight off the object
code. **`VIBBox` has a user COPY CONSTRUCTOR** — every by-value return ZEROES
the caller's slot before copying, which a synthesized one never would.
**A `VIResource` is a MEMBER of `VIParticleDefinition` at +0x2FC, not a base**
(`VIDictionary::Remove` is handed `d + 764`), and its chained `m_id = m_link =
0` is the pair that opens the class's own constructor. **`NextMotif` does not
return early** — found-and-last keeps walking, worth 17 of 56 words.
**`SetDirection(VIVect3)` uses a by-VALUE `Cross`** whose temporary is written
three times and never read (its address is taken by `operator=`'s reference, so
the dead stores survive) — 120 instructions, first build. Layouts:
`VIParticleEmitter` **992** (confirming Phase 24's measurement off
`VICSpritePartEmitter`, whose stub is now gone), `VIParticleSprite` **1076**
with the emitter EMBEDDED at +0x50, **`VIParticleSystem` is EMBEDDED IN
`VIScene` at +0x548**, a particle is **104 bytes on a `VILNode`**, and
**Phase 12's unnamed float at `VIParticleAttributes::+0x2C8` is the MOTIF
WEIGHT**. Shipped oddity: `VIParticleSprite::Copy` calls
`SetLocation(src.Location())` TWICE, so the copy loses the source's last
location. Open: `Init` 33/84 and `ReleaseAllParticleDefinitions` 15/36; the six
large functions (`Update` 696, `Release` 660, `RecomputeStaticBBox` 510,
`Raster` 394, `Render` 332, `Pick` 174) are not attempted.
**Phase 27 is a BREADTH sweep — eighteen small classes, 114 of 116, 96 of them
exact on the FIRST build**, on the settled toolchain with no re-tuning:
`VICameraEffect` 13/14, `VITimer`, `VILanguage`, `VIActorFilter`,
`VISceneFilter`, `VISceneQuery`, `VIMotifBlender`, `VIEnvRay`+`VITransEnvRay`,
`VIWorldTree`, `VIZoneTree`, `VIGameOptions`, `VIConsole`, `VIWnd` 13/13,
`VIClipper`, `VIQueryTriList`, `VIStaticLighting`, `VIStringTable` and
`VITimeOfDay` 4/5. **An assert's LINE NUMBER is an immediate** — `_VIAssert`'s
two string arguments are relocation sites the differ masks, but `__LINE__` is
`li a2,59` and has to match, so `vilanguage.cpp` is PADDED to the original's
line layout. **A ternary VALUE gives `slt`+`movn` where every `if` spelling
gives `slti`+`movz`** (the exact dual of Phase 12's `||`). **The element
pointer's index goes in its INITIALISER** — `int *p = m_data + i; *p` is
base-first where `p[i]` is offset-first — and `VIStaticLighting` needs the ARRAY
pointer declared INSIDE the loop as well. **A container's inline `Value(i)` is
what keeps `this` on the CONTAINER's register** (`VISceneQuery`, `VIWnd`).
**A store through a REGISTER base invalidates gcc 2.95 cse's WHOLE memory
table** — which is the only thing that stops it forwarding six vector stores
into the six loads that follow, and it is why `VICameraEffect::Apply` needs
Phase 9's pointer variable for one field it never addresses. **A switch's data
table is readable out of the RELOCATED dump** — `GetDayOfWeek`'s month offsets
`{6,2,2,5,0,3,5,1,4,6,2,4}` came off the jump table at 0x004E9CE0, including
which case has offset 0 (no block at all) and which two share one.
**`VIActorFilter`/`VISceneFilter`/`VISceneQuery` each have ONE virtual and NO
base** (all three `__tf`s take two arguments; `VISceneQuery` is *not* a
`VISceneFilter`), **`VIWnd` has SIX** — the first class whose declaration order
was DERIVED from Phase 17's `8N` arithmetic rather than checked against it, and
all thirteen matched first try. **`VIQueryTri` is the corpus's only real
`new T[n]`**, the control for Phase 24. Shipped oddities: `VIStringTable::Unload`
returns **void** among `int`s, `GetString` takes a **one-based** row, and
`VIGameOptions::Load` calls `SetDefaults` when the first word is **non**-zero.
Open: `Apply` 56/312 and `GetLocalTime` 9/80, both register allocation.
**Phase 26 added the RED-BLACK TREE — one CLRS tree written out THREE times
(`VISet<T>` 12/16, `VIMultiMap<K,V>` 34/46 over three instantiations,
`VIMap<K,V>` + `VIDictionary` 58/74 over four) plus `VIList<T>` at **30/30** over
five, 134 of 166 on the settled toolchain with no re-tuning** — eleven of
VISet's sixteen byte-exact on the FIRST build, the 238-word `EraseFixup` among
them. The tree lies over VIPool's index-addressed free list and **slot 0 is the
NIL sentinel** (AddNode's first growth step is TWO, Allocate zeroes
`m_nodes[0]`, and `End()` is 0 where VIPool's is 0x7FFFFFFF). **The fixup
loop's parent is a LOCAL and `EraseFixup`'s is not** — written out, each colour
store forces a re-load because it may alias the word it came from, and the two
uncle-is-red arms stop cross-joining: 20 words. **A pseudo-destructor call
(`NODE(i)->m_value.~V()`) is invisible for every POD instantiation** and is what
`VIMultiMap<float,VIEmitterEntry>` needs; **`DestroyPool` runs it a SECOND time**,
after Erase has already freed the node, indexing by a COPY of the loop variable.
**`Size()` discounts the sentinel**; **`Key` returns a REFERENCE**.
**`VIList`'s iterator is the node ADDRESS carried in an `int`**, its `End()` is
0x7FFFFFFF, and **`new Node` is not null-checked by gcc 2.95**, so the element's
initialiser lands AHEAD of the source's own `if (n == 0)`. Three class sizes
came off node strides: **`VIActor` is 160 bytes align 16**, **`VIMaterial` is
`aligned(16)`** (an attribute that moves no member, which is why phase 15 could
not see it), and **`VIEmitterEntry` is EIGHTY bytes with a `VIMatrix44` at
+16** — not the two words `vi.h` had guessed since phase 19 — which in turn says
**`VIMatrix44` has a user-declared `operator=`**. Harness: **`SPAN_IGNORE`**, for
four map entries (`VIRStrip`..`VIRStrip4`) whose address lands in the MIDDLE of a
function body and truncated their host by exactly their own length
(`EraseFixup` read as 178 words instead of 238). Open: `Insert` everywhere (the
parent local lands in `a2` where the target has `a1`), `Erase`'s `y` selection
(six spellings, every one if-converts to a `movz` where the target BRANCHES),
and `Allocate` by four.
**Phase 25 added the ANIMATION CLIP, the THIRD TEMPLATE and the VALUE POOLS —
`VIHSpriteAnim` 19/19, `VIVector<T>` 27/28 over four instantiations, `VIHSprite`'s
readers 25/26, and the three value `VIPool<T>`s — and ONE WORD IN IT CLOSED PHASE
12's `Add` IN ALL THIRTEEN POINTER INSTANTIATIONS**: the node address is an
INTEGER add with the offset on the left, phase 24's `VIArray` lever again.
The fourth group whose semantics came from the console first (`go-eqoa-vi/vianim`).
**`m_frames` is read BEFORE the guard** — its `lw` is in the FIRST basic block,
ahead of the branch, where no scheduler could put it — and **the frame index
accumulates INTO the parameter** (7 words each). **A local declared INSIDE each
case is 22 of 35** in `CalcDataSize`; shared at function scope the instructions
are identical and every register differs. **The two frame formats are NOT the
same shape**: the unpacked keeps scale and translation apart, the packed is two
`VIVect4S16` whose second four are (scale, x, y, z), and `GetNodeFrame` walks
those four as ONE run off a pointer on each side. **`m_data = 0` is OUTSIDE the
`if` in `VIVector::Clear`**, said by two branch-likely delay slots holding a
stolen copy of it. **`m_trans.Zero()`, not three stores** — 59 of 129 words
where `VIHSpriteNode`'s ctor inlines, and that ctor IS the whole 256-byte
layout. **Every indexed `VIHSprite` reader names the ELEMENT through a
POINTER** where `VIHSpriteAnim` and `VIVector` need the INTEGER form for the
same shape of access — both spellings in one phase. **Phase 10's guarded
do/while** in `Last` (24 of 37). **`VIHSpritePlay`'s three 64-bit clock fields
are load-bearing at COMPILE time**: they give alignment 8, and without them gcc
guards the 104-byte node copy with a runtime `(src|dst)&7` test (48 of 100) —
`vianim`'s console-read field map confirmed by a route the console could not
take. **`VIHSpriteFrame` has a user `operator=`**; **`VIPool::Init(const
VIPool&)` uses ONE index for both walks**. Four shipped bugs:
`VIHSpriteAnim::GetPlaySpeed` returns the play RATE, its `Clear` never zeroes
`m_numTriggers`, `VIHSprite::Parent` is INVERTED (it answers only for node 0)
and `VIHSprite::Prev` walks node i's OWN child chain. Fourteen new `SPAN_CAP`s.
Open: the `Allocate` copy loops (two induction variables in the opposite order —
not the source, six spellings tried) and gcc's own
`__10VIMatrix44RC10VIMatrix44` at 19/21.
**Phase 24 added the SECOND TEMPLATE — `VIArray<T>` 131 of 132 across TWENTY-TWO
instantiations, plus `VIClipStack` 7 of the 10 written, same toolchain, no re-tuning.**
Two words — `{int m_size; T *m_data;}` — and `radial-flora-system.md` had
that shape years ago off the CALLERS. **The allocation is a RAW char block and the
elements are PLACED into it**: `new T[n]` would carry gcc's own vec-init loop, which the
target has for none of the twenty-two, and `delete [] (char *)m_data` is the same decision
back — the typed spelling drags the array-cookie destroy loop into `Clear` for any element
with a destructor. **The five nops in an empty loop are `ps2eeas`**, padding a short
branch-likely loop for the R5900 erratum, byte-identical in our object; and the empty loop
survives at all only because the address computation is a USE of the index — a bare empty
`for` is deleted outright. **The element address is `i * sizeof(T) + (unsigned int)m_data`,
OFFSET FIRST** — every pointer spelling canonicalises base-first and an integer add does
not (five instantiations fixed in one edit; the one open function is where `sizeof(T) == 1`
degenerates it). **`DestroyPool`'s index is a COPY of the loop variable and `Next()` runs
BEFORE the body**, which stops gcc strength-reducing the multiply out of the loop; `Next`
itself needs Phase 7's if-direction. Every element class is modelled from the two loops
alone — size from the address arithmetic, the constructor from `Allocate`'s body, the
destructor from `DestroyPool`'s `jal` — including **`VIParticleEmitter` as a MEMBER of
`VICSpritePartEmitter` at +16** (a base would sit at 0), six of six on the first build, and
`VIRadialFloraInst`'s three-deep double-zero. **`long` is eight bytes, said a third way.**
`VIClipStack` is three counts each one word in front of the array it counts: **`Push*`
returns the index it pushed** (`return m_numEntries++;` is the wrong spelling, worth 111
words and one instruction of length), and **`PushTranslatedVolume` never reads its
`numPlanes` parameter** — `this` goes into that register at instruction 0. Eighteen new
`SPAN_CAP`s: every VIArray TU emits its own static twins of the pieces gcc did not make
weak. Open: one `addu`, one `.p2align` pad, and `PushBBox`, which differs from the target
by ONE adjacent-pair swap once the register names are normalised.
**Phase 23 added the FILE LAYER — `VIFile` 29/36 and `VIWadFile` 9/9, same
toolchain, no re-tuning, 19 of the first 19 on the first build.** `VIFile` is
**four back ends behind one interface with no virtuals at all** (buffered
CD/host file, flat memory, a scatter/gather segment list with a binary search,
nothing) and every entry point re-spells the ladder by hand. **`long` is EIGHT
bytes on this ABI and `long long` is SIXTEEN** — Phase 16 saw that in one
`sd`-zeroed slot; here it decides every 64-bit member, and getting it wrong does
not mis-size the class, it **fails to compile** (`unsupported wide integer
operation`, `long long` being TImode). Four levers: **nested guards with ONE
`return 0` at the bottom** (two `if (X) return 0;` statements, or an `||`, put
the second branch the wrong way round — `BufferFlush` 11 wrong to 0); **`VIMIN`/
`VIMAX` with the expression WRITTEN OUT beats a named local**, the macro's
re-evaluation giving `movn` with the second value as the register default where
a local gives `movz` with the first (three functions, one line each); **a
walking pointer's local is declared at FUNCTION scope** and the 6 words it costs
are entirely in the PROLOGUE, in the register-save order; and **a retry loop's
CONDITION carries the calls while its BODY is only the wait**
(`while ((r = sceCdSearchFile(f, name)) == 0 && sceCdGetError())`, 25 wrong to
9). In all three `sceCd*`/`VICdStream*` twins **the disc path is the `if` and
the IOP path the tail**. New `SPAN_CAP` entry: `CDRead`'s derived span swallows
an unnamed **static interrupt handler**. Shipped bugs: **`VIFile::Close` tests
the FILE DESCRIPTOR for bit 6, not `m_flags`** (+0x00 vs +0x04), so its
asynchronous drain never runs; **`VIWadFile::Load` returns 0, not -1, when the
caller's buffer is too small**; `VIWadFile::Free` returns void; wad lookup is a
**LINEAR** walk over a `hash*131 + c` hash, so a collision silently returns the
wrong file; `MemorySeek` rejects a seek to exactly EOF; and `BufferedWrite`'s
post-flush salvage keeps `VIMIN(0, m_bufLength)` bytes and copies all of them to
`m_bufferAligned[0]`. Open: `BufferedRead` 47/233, `BufferedWrite` 23/126 (two
`memcpy` sites the target cross-joins), `Open(const char *, int)` 35/93 (three
separate `andi 1/2/4` where every spelling folds to `andi 7`, plus a dead
`m_flags & 8`), and three delay-slot choices.
**Phase 22 added VICollide — 18 of the 22 written, same toolchain, no re-tuning — and
one finding in it made EIGHT already-open functions byte-exact in four OTHER
classes.** The fourth class whose semantics came from the console first
(`go-eqoa-vi/vicoll`), so every miss is codegen. **`VIMatrix44`'s default
constructor ZEROES**: invisible for twenty-one phases because every VIMatrix44 in
the corpus was a LOCAL, where `VIMatrix44 m; m.Zero();` emits the same words — a
MEMBER is what separates them, and the target's `VICollide` ctor interleaves the
array ctor and the VU0 asm PER MEMBER, which no ctor body can do. **Phase 20
refuted the wrong spelling**: `VIMatrix44() { Zero(); }` emits a `jal` (gcc 2.95
will not re-inline an inline member called from another inline member), while the
asm written straight into the ctor body inlines — and that took `VIGroupSprite`
from 6 to 12 of 13, plus `VIPointSprite::Pick` and `VIQuat::ConvertTo` (13/13).
Layout: `m_count` +0 with 12 bytes of padding, `m_world` +0x10, `m_invWorld`
+0x50 (DERIVED, no setter), `VIVect3 m_preTranslations[32]` +0x90, the
`VIPool<VICollBuffer *>` +0x210 — and **a collision buffer's refcount is
`VIBuffer::m_userData`**, with `sizeof(VICollBuffer)` = 92. Levers: **a loop whose
test consumes a CALL must call it in the INIT and the INCREMENT** (`Clear`, 13
wrong to 0, where the condition form cannot); Phase 9's pointer variable four
more times and it is **per site** (naming the pool fixes `ReleaseCollBuffer` and
costs `DebugResources` 25 words, because it takes `this` out of `s2`);
**`VIVect4::Set` is how a returned TEMPORARY is copied** (`wn.Set(nrm * m_world)`
— plain assignment block-moves the sret slot); two accumulators come out in the
REVERSE of source order; the three packing dispatchers are a `switch`, Phase 16's
rule again. **Three more guessed callee names corrected by `eqcalls.py`**:
`SetTransform` is `SetWorld`, `CopyObj` is `ShareCollBuffer`, `ReleaseObj` is
`ReleaseCollBuffer`. Open: three `Cull`s (30/68, 39/74, 38/76 — the standing
phenomenon, and `Cull(ray, VIBBox)` has **no stack frame at all**, so the source
is not the variable) and `DebugResources` by ONE word.
**Phase 21 added the VIEW FRUSTUM and the BAKED-LIGHTING BUFFER — `VIFrustum` 8/9 and
`VIColorBuffer` 9/9 of the nine written, same toolchain, no re-tuning, 12 of 18 first
try.** `VIFrustum` is `VIVect3 m_verts[8]` + `VIPlane m_planes[6]` (0xC0), read off its
two accessors; both setters are **whole-object assignments** and return **void**.
`CalcPlanes` went **209 wrong to 0** on one question: **a `VIVect3` copy is written out
component by component** — `v[0] = m_verts[3]` passes a reference, which is Phase 9's
pointer variable and parks six `&m_verts[k]` in `s0`-`s5`; the field form is twelve plain
displacement loads. The locals are an ARRAY (the zeroing LOOP proves it, against Phase 20's
one-at-a-time rule) and **the fourth corner of each quad face is copied and never passed**,
a dead store surviving because the array's address is taken. New lever: **the `lui`s come
out in SOURCE order while the stores are rescheduled**, so a block of writes to file-scope
statics is read off the `lui` sequence — four source orderings gave byte-identical output,
and reading the `lui`s put `Lock` and `Unlock` exact at once. `CalcDataSize` needed a
**third** answer to the if-direction question (`if (c) { … } else return -1; return 0;` is
the only one of three that orders the two epilogues right), **both stores of an aligned
value unconditional**, Phase 14's fresh intermediate in integer code (`a = a + 16 - (a&15)`
reassociates to `addiu -16`), and a tag count as an `if`/`else` with the whole expression
in each arm. Open: `Cull(VIBBox&, VIMatrix44&)` 19/66 — the target does not hoist the plane
loads out of the inner loop and enters it by jumping into the MIDDLE of its body; a
`return 1` from inside reproduces the body and loses the post-loop `if (j == 8)`, and
thirteen `-fno-` flags produce neither. `VIColorBuffer::Color` not attempted.
**Phase 20 added the SPRITE LEAVES — `VISkinSprite` 12/12, `VIFloraSprite` 13/13,
`VILODSprite` 13/14, `VIPointSprite` 9/11, `VIGroupSprite` 8/15, same toolchain, no
re-tuning.** **The sprite vtable is 96 bytes ALL THE WAY DOWN** — the map gives the
leaves' `_vt$` symbols 128/136/144, but that is the gap swallowing the RTTI name
string, which begins at byte 96 exactly in all six: ten virtuals, and **not one leaf
adds a virtual**. **A count is read twice or not at all** — `VIGroupSprite` has FIVE
members, said independently by the ctor's array loop (`li v1,4`) and by `Copy`'s
block-move end (`this+228` = `0x44 + 5*32`); a first draft of four cost 19 words.
A member array is copied by **`memcpy`**, and 40 bytes goes by pieces while 160
becomes gcc's runtime-alignment-checked loop (the crossover is `2*MAX_MOVE_BYTES`
= 64). **Matrix locals are declared and zeroed ONE AT A TIME.** `VIVect3` needs
`operator-=(float)`/`operator+=(float)`. And **Phase 7's if-direction is per
function**: `VILODSprite::Raster` is byte-exact with the CALL in the `if` and
`::Pick` with the `return 0` in it, each 6-12 words worse the other way —
`VIPointSprite::Pick` went 41 wrong to 8 on that question alone. Shipped facts:
`VISkinSprite`/`VIFloraSprite` have **no collision object at all** (both `Collide`
overloads are `return 0` stubs), and `VILODSprite::Raster` calls
**`VIScene::GetNextColorBuffer` for every level it SKIPS** whose sprite is a
`VISimpleSprite` — a per-frame cursor that must be consumed even when nothing is
drawn. Open: the target materialises a fresh register for `VIMatrix44::Zero()`'s
asm operand where ours uses the object's existing base (5 words x 3 matrices x 6
functions); four operand spellings, `"+r"`, a `volatile` local and moving `Zero()`
into the constructor are all refuted.
**Phase 4 added `VIMatrix33` — 20 of its 24, on the SAME toolchain with no re-tuning**, which
is what makes the compiler identity a finding rather than a fit: `VIVect3 m[3]` stride 12, and
**`VIVect3` HAS a user `operator=` *and* copy ctor where `VIVect4` must have neither** (nine
element copies vs a 64-byte block move) — adding both regressed nothing.
**`VIMatrix33::Translate` is a shipped BUG**: it stores to offsets 36/40, one row past a
three-row matrix (`VIMatrix44`'s translation row copied verbatim), where `Transform` five
functions later uses row 2 — and it matches byte for byte written that way. **`Mul(m,m)`
accumulates into `this` with NO temp**, so `go-eqoa-vi/vimath`'s `Matrix33.Mul` disagrees with
the console when the destination is also an operand; the product's scalar is a **fresh local
per term** (one reused variable gives right instructions, wrong FPRs).
**Phase 9 added the SHAPE classes and the first INTEGER code — `VIBBox` 16/20, `VISphere`
2/3, `VIBRect` 12/13, `VIRect` 9/13, no re-tuning**: `VIRect` is `slt`/`movz`/`movn`/the
R5900 three-operand `mult`, a different instruction-set corner of the same build. New
lever: **naming a sub-object through a POINTER VARIABLE** puts its +4/+8 members on that
register and folds +0 back into the outer base (`VIVect3 *mx = &max;` — 18 spellings of
`max.y` reach `16(a0)` instead). The author writes **assign-then-modify-in-place**, one
operation per statement; `if (!(cond)) return 0; return 1;` is its own shape; a multi-term
rejection is one `||`; a **fresh local per squared distance**. `VIVect2` needs the zeroing
ctor + two-float ctor + `operator=`, **`VIVect2I` needs `operator=` but NO zeroing ctor**.
**Two more shipped bugs**: every `Scale` pivots on **`min + max`, not the centre** (no
`0.5f` exists in any of them, so an off-origin box is MOVED), and `VIRect::Store` /
`VIBRect::Store` / `VIBRect::Transform` build a **degenerate rectangle** — `min.y` for all
four corners, `+12` never read. `eqsweep.py --keep DIR` writes each variant's object out,
because the sweep scores POSITIONALLY and a right-layout/wrong-length variant reads as
"everything wrong".
**Phase 7 added `VIPlane`/`VIPlaneD` and CRACKED `InvertAffine`** with a new tool,
`re-tools/dblflow.py`: the image is un-relocated so every `jal` target is zero and the callee
cannot be read, but a double function is nothing but libgcc calls and **the OPERANDS identify
the algorithm** — it prints `cN = f(a.m[1].y, a.m[2].z)` per call, and 315 instructions came
out readable in one pass (source scored 2/315 wrong first build). `InvertAffine`'s
determinant is **the six triple products left-associated, NOT `r0 · (r1 × r2)` over
cofactors** — the float VU0 sibling does use the cross product, so `invert.go`'s note is a
DIFFERENT computation; **a zero determinant writes nothing at all**; every negation is
`__subdf3(0.0, x)`; and its last two words were a **chained assignment**
(`m[0].w = m[1].w = m[2].w = 0.0` stores 2,1,0). `VIPlane`'s normal is the FIRST member.
Two general rules: **the success path goes in the `if`** (gcc 2.95 moves an `if`-body ending
in `return` out of line, so the source's `if` holds the block that lands LAST — inverting the
guard fixed three functions), and **a reference out-parameter read three times needs a local
copy**. And the standing phenomenon got sharper: **an `addu`'s operand order is decided by
the ALLOCATED REGISTER NUMBERS, not the source** — one function lost its match when an
unrelated function in the same TU was rewritten.
**Phase 6 added the DOUBLE SIBLINGS — `VIMatrix44D` 41, `VIQuatD` 13 and their two free
`operator*`, 50 of 56 on the same toolchain with no re-tuning.** Nothing in a `…D` class is
VU0 (the vector unit is single-precision only), so every operator is a libgcc call and gcc
2.95 does not CSE across one: **the double twin is an ORACLE for the FLOAT source**, and it
changed six already-matched float functions (all still matching). `VIQuat::RotateX/Y/Z` has
**no `h = a*0.5f`** — the half-angle is computed twice; `UnitRotate` and both quaternion
`Transform`s have **no `x2/y2/z2/w2`** (nine products, each `(q.c+q.c)*q.d`, in wx/wy/wz/xx…
order); `RotateHPR` and both euler `Transform`s have **no `shsj`/`chsj`**. `VIVect3D` and
`VIVect4D` have **zeroing default ctors** — and therefore their `Distance`/`DistanceSquared`
locals are NOT struct locals, because gcc does not delete those stores even when every member
is overwritten first. **`VIMatrix44D` has a destructor that is not in the symbol map at all**
(only the `a1 = 2` `__in_chrg` call proves it), and its `ConvertTo` has no `m.Zero()` where
the float twin needs one. The class also **shows what VU0 hides**: `Mul(m,m)` is
`VIMatrix33::Mul`'s no-temp accumulation at 4x4, `BlendAccum` multiplies component-by-weight
ONCE per row and is not `SubMatrixBlendAccum`'s shape, and **`InvertAffine` expands the
determinant as six triple products, not `r0 · (r1 × r2)` over cofactors** — so
`go-eqoa-vi/vimath/invert.go`'s comment on the float path needs re-checking.
**Phase 5 added `VIQuat` (12 of 13) and the whole `VIVect` family (26 of 32, software doubles
included), 21 of the 35 first try — and ONE new flag, `-fno-exceptions`.**
`VIQuat::ConvertTo(VIVect3&)` is the first function in the tree with a local whose class has a
destructor, and with exceptions on it grows three `_$_10VIMatrix44(this, 2)` cleanup calls;
**every other function is byte-identical either way**, so the flag was invisible for four
phases. RTTI stays on. **`operator*` returns a TEMPORARY** (`return VIQuat(a,b,c,d);` is
elided into the caller's slot; a named local costs 10 words of `ldl/ldr/sdl/sdr`), so the
class has a 4-float ctor as well as the zeroing one. **Field order decides the schedule**
(`RotateX/Y/Z` are one source shape in x,y,z,w order, not three). **`Normalize` is ONE shape
across all six widths** — `if (d != 0) {...; return 1;} return 0;` — settled by the
double version, where the compare is a libgcc call. **`Slerp` returns void**;
**`VIMatrix44::Zero()` is defined in the class body**; a `jal` with a small constant in `a1`
on a class object is the **GNU 2.x destructor's `__in_chrg`**; and the **double siblings are
all libgcc** (no `sqrt.s`, no NaN retest) yet match from the same source text.
The **twenty-three** open functions were read as **one phenomenon** — the target neither merges
two identical memory references nor keeps a value live where ours does — but that is a
DESCRIPTION, not a cause: **`eqrtl.py` names three passes** (`sched`/`sched2` for the
call-result pair, `lreg` for the double `Distance` frames — 128-byte frame saving s0-s4 where
the target's is 64 saving s0 alone — and `loop` for the induction pointers), all three cost
models in `config/mips/`, and the scheduler must NOT be turned off (`-fno-schedule-insns2`
costs 141 -> 15). It is **neither a
flag** (29 swept now, `-fvolatile` and six more scheduling switches included) **nor any
compiler on disk** (all that build agree with ours) **nor, for the Phase 5 members, the
source**: six spellings of `VIVect3::Distance` give byte-identical output. Phase 6 gave it
its four smallest examples (6–20 words): **the target keeps the instruction that consumes a
libcall's result glued to the call and loads the next operand after it.**
**Phase 19 added the SPRITE HIERARCHY — `VISprite` 15/16 and `VISimpleSprite`
13/13, same toolchain, no re-tuning, 27 of 28 first try.** The lever is one
measured fact: **a gcc 2.x vtable ends with a NULL entry, so B bytes hold
`B/8 - 2` virtuals** (`class B { int a,b; virtual ~B(); virtual int f1();
virtual int f2(); }` emits `.size _vt$1B,40`). `_vt$8VISprite` is 96 bytes, so
**ten** — and **the console agrees independently**: `go-eqoa-vi/vicoll`
dispatches the swept sphere through entry 4 of the live table at `sprite+0x3C`
and the ray through entry 5, exactly where this declaration order puts them.
**It also corrects Phase 17** — all three sound vtables are 64 bytes, i.e. SIX
virtuals, so `VISound::ChannelCount` is not one (every `VISoundDevice` dispatch
site reaches slots 1..6 and no further; the error was invisible because the
slot is LAST). Layout: 8-byte `VIResource` base, `VIBBox` +0x08, `VISphere`
+0x20, `m_offDist` 500.0f +0x30, `m_type` +0x34 (go-eqoa-vi's VISpriteType),
an actor word -1 at +0x38 that **`Copy` does NOT carry over**, vptr +0x3C,
size 0x40; `VISimpleSprite` adds prim buffer / collide-object index / material
palette at +0x40/+0x44/+0x48 and **no virtuals**. **`VISphere` needs a zeroing
default ctor** (Phase 13's `VIBBox` double-zero). Three levers: **a derived
class's `__tf` inlines the base's when they share a TU** (34 instructions
against 20 — the cheapest test for a TU boundary in a hierarchy);
**`SetMaterialPal`'s `VIDictionary &` is DISCARDED**, and the tell is the
SECOND call, where a 3-argument source must move the argument back into `a2`
and the target does not; and **a local whose address is taken by a `const &`
parameter is stored exactly ONCE**, so a sort key splits into a
non-address-taken intermediate (which lives in `$f20`) and the addressed value.
Two more `SPAN_CAP` entries: the sprite TUs carry their own `VIMultiMap`
red-black tree, so a two-instruction `Release__8VISprite` had a 3512-byte span.
Open: `QueryEmitters` 32/112, the standing phenomenon with its pass NAMED —
**`gcse` merges the four `addiu a0,sp,32`** into one pseudo needing a
callee-saved register where the target rematerialises; `-fno-gcse` fixes it and
costs the tree 16 functions, while declaring the local FIRST puts it at sp+0
(a bare `move a0,sp`, nothing to merge) and reproduces the target's allocation
instruction for instruction at its exact length, every local at the wrong offset.
**Phase 17 added the SOUND RESOURCES and the FIRST VTABLE — `VISound` 6/6,
`VIWave` 20/20, `VIXm` 12/12, same toolchain, no re-tuning, 29 of 38 first try.**
Everything before this either had no vtable or inherited without one, so this
settles the polymorphic ABI — the gate on `VISprite`, `VIRaster`, `VIScene` and
nearly every class left. **The vptr goes at the END of the class's own data**
(measured: `class N { virtual ~N(); int a,b,c,m; }` puts the vptr at 16), and a
derived class adds none — so `VISound`'s vptr at +0x0C means an 8-byte
non-polymorphic `VIResource` plus `m_type` ahead of it. **A vtable entry is 8
bytes** (`{short delta; short index; pfn}`), no thunks, entry 0 the header
holding `__tf<class>`, so **virtual N sits at byte 8N** (its pfn at 8N+4); all
three vtables are 64 bytes, which proves **`VIXm::SetRepeat` is NOT virtual**,
and the slots come off call sites (`lw rB,12(rA)` then `lh rt,imm(rB)`).
**Phase 19 corrects the COUNT**: a gcc 2.x vtable ends with a NULL entry, so
64 bytes is SIX virtuals and `VISound::ChannelCount` is not one. **The RTTI
name strings ARE the hierarchy** — `6VIWave`/`7VISound`/`10VIResource`
consecutive in `.rodata` — and `__tf`'s argument count separates single
inheritance (3 args, 16-byte `__ti`) from no base (2, 8). **Every TU emits its
OWN `__tf`**, three copies each, so three more `SPAN_CAP` entries. **There is
no `__7VISound`**: the base has no user-declared ctor and each derived class
stamps `m_type` LAST — read off a cheaper rule than the Phase 13/16 hoist,
**the vptr store lands after the block's FIRST member store and the rest keeps
source order**, the R5900 scheduler filling the `lui`+`addiu` gap (and the
store pulled up is the FIRST of a repeated value, not the last). Shipped bug:
**`VIWave::Deallocate` returns -1 on every path**, so `Destroy()` always
reports failure. Both halves of Phase 13's if-conversion lever sit side by
side: `VIWave::IsCached` is one `sltu`, `VIXm::IsCached` BRANCHES and only
`if (a) { if (b) return 1; } return 0;` produces it.
**Phase 16 added the TEXTURE SURFACE — `VISurface` 11/12, same toolchain, no re-tuning,
six first try.** `VIMaterial`'s other half, and the first class that builds **GS
display-list headers**: 64 bytes of VIF/GIF tags in front of every mip level and the CLUT
(VIF DIRECT, GIFtag A+D `NLOOP=1`, `TRXDIR`=host->local, GIFtag `IMAGE` EOP). `VIMipLevel`
is 24 bytes at +0x44 x10 and +0x134 closes it exactly; `m_pixels`/`m_header` are byte
OFFSETS and only `m_header` is rebased onto `m_data`, in place. **The `VIPixelStore` enum
comes off `CalcMipLevelStride`'s jump table** (0 I4, 1 I8, 2 RGBA32, 3 RGB24, 5 RGBA16 —
**4 has no case at all**), and `long long` is 16 bytes here but **`long` is 8**, which is
what makes those `sd` constants `unsigned long`. Four levers: **the LAST store of each
distinct CONSTANT is hoisted to the front** (Phase 13 saw only the tail of this, its block
having one constant) — read that way `Deallocate` is field order with the last two
swapped, byte-exact, where the emitted order taken for source order is nine words wrong;
**`Init`'s `VIRaster &` is DISCARDED and the first call is `Deallocate()`** (Phase 14's
precedent, measurable here: any call keeping `r` live allocates s1/s2 the other way round);
**a walking pointer keeps its `addiu` only when its origin is opaque** — compute the
pointer and store it BACK to the member, never the reverse, and declare the walker at
FUNCTION scope so both writers share it (57 wrong words -> 12); and **a switch orders
comparisons by VALUE and bodies by SOURCE**, with three `return 0`s written as one `break`
to the bottom so reorg can steal `move v0,zero` into the first `bne`'s delay slot. Open:
`CalcSurfaceOffsets` 87/143 — the standing phenomenon in three forms, **not a flag** (ten
swept).
**Phase 15 added the RENDER STATE — `VIMaterial` 18/31, same toolchain, no re-tuning, six
first try.** The second property bag after `VIParticleAttributes`, deriving from **`VIBuffer`**
(the ctor's first two stores are +0x04 then +0x00), with a 144-byte layer capped at three.
**The lever is `this + i * 144`, NOT `&m_layers[i]`**: the target folds the array base into
the DISPLACEMENT (48/52/56/60/64/100/104) and a subscript cannot — gcc folds it onto `this`
and leaves the access at `0(reg)` — but only where the address is used TWICE, which is why
single-read accessors match a subscript and read-modify-writes do not. Twelve accessors went
byte-exact in one edit. **The flag words are `unsigned`** (`sltiu`, and `lui 0xffff`+`ori`
masks) — five more at once; **`VIColor32` has an opaque-black default ctor** and **`Set` is an
inline member**. A shipped bug: **`SetLayerUVSet` ORs the COMPLEMENT**, so asking for UV set 1
turns on every other layer flag — matched as written. Harness fact: **`long long` is not an
8-byte member on this ABI** (it moved every later field by 16 and broke thirteen matched
accessors; `double` is right). Open: five, none structural. Not started: `SetNumLayers`,
`SetLayerTexture`, `CopyLayer`, `Process` and the four `Construct*Pass` DMA builders.
**Phase 14 added the GS DISPLAY-LIST BUILDER — `VIPrimBuffer` 29/31, same toolchain, no
re-tuning, fourteen first try.** VICollBuffer's render-side sibling, except every vertex is
one or more QUADWORDS carrying a VIF tag, so the vertex section is `<< 4` and `CalcDataSize`
counts quadwords. **`VIVect4` is `aligned(16)`** — the four packing scales are one member at
+0x30, four bytes past the `VIBBox` above them, and the attribute fixed NINE functions at
once with no regression (it is also why `VIMatrix44` comes out 16-aligned). The type enum is
read off the DISPATCHERS: `VIRaster::PickPrimBuffer` tests 0, 2, 4, 5 and
`DrawPrimBuffer(…float…)` tests 8, so **VUNC=0, PackVUNC=2, PackVUNCG=4, PackVUNBW=5,
PackVU=8**. **The tags are VIFcodes** and `End`'s last word is `X | 0x14000000` = **MSCAL**,
so `X` is a VU1 microprogram offset out of `_8VIRaster$RStrip*Offset` — **which is why
`End`'s four packed cases look byte-identical in the un-relocated image and still do not
cross-jump.** Six levers: **cross-jumping does not come back for a second pass** (the
closing statements written INTO EACH CASE keep three byte-identical bodies apart, where
written after the switch all three collapse — 87 instructions against 153); **a fresh local
for the intermediate, in INTEGER code** (`t = n + 16; r = t - m;` is byte-exact at all ten
alignment sites, `r = n + 16; r = r - m;` collapses the diamond); **the ORDER of a case's
locals decides where the split lands** (`End`'s VU1 offset must be read LAST); **a chained
assignment is what makes a float zero a REGISTER** (`Clear` stores eleven statics with
`swc1` off a `lwc1` of `m_scale.y`; separate statements give `sw $0` eleven times); **a
local declared INSIDE the case** puts its zeroing ctor below the switch test; and **which of
two identical stores ends the block** decides whether cross-jumping eats it. Also
**`PrimVertexGroup` is `unsigned short`** and `Vertex(VIVect3S16&)` wants **one pointer base
per byte group**, read off the target's `addiu`s. `Begin(int)` and `Init`'s `VIRaster &` are
both DISCARDED. Harness: a fifth `SPAN_CAP` and **`_q.py`** (one TU, 4.5 s, which is what
makes a 32-variant sweep affordable). Open: two, both register allocation.
**Phase 13 added the GEOMETRY BUFFER, and two of its levers reached BACKWARDS —
`VICollBuffer` 18/19, same toolchain, no re-tuning.** The third class whose semantics
were settled from the console first (`go-eqoa-vi/vicoll/collbuffer.go` ports the reader),
so every miss was codegen. **The block's LAST store is hoisted to the FRONT** — the first
store emitted after a join names the source's *last* statement, whichever it is, which is
how `m_data = 0` was placed in `Clear()`. **`if (x) { … } return 0;` defeats the
if-conversion** that turns a flat `if (p==0) return -1; return 0;` into `movn` — five
spellings of the TEST are byte-identical, so the test is not where it lives — and applying
the same nesting to `VIPool<T>::Allocate`, Phase 12's unexplained miss, took it from 9
wrong words to 4 **in all thirteen instantiations**. **A shared tail written into EACH case
cross-jumps** and leaves its constant duplicated in the predecessors; shared, the `li 28`
stays in the merged tail and every register in the function moves by one. Also **`VIBBox`
has a zeroing ctor calling `VIVect3::Zero()`** on min and max (six stores on `this` share
ONE base register; the target gives each vector its own), **`VIVect3S16` needs a user
`operator=`**, and a **`VIBuffer` base carries +0x00/+0x04** — all three buffer classes zero
them, +0x04 FIRST (a chained assignment) and before the member sub-objects, and `Clear()`
never touches them. `CalcBVolumes` is 27 words short: its `int` parameter is **unused**,
`p` is VICollide's vertex-group table, stack slots go in DECLARATION order while the
initialising stores go in STATEMENT order, and the packed cases read a SUBSCRIPT
(index-first `addu`) where the V case materialises a variable (base-first) — Phase 3's rule,
both halves in one function. Harness: **the wrong-count was lying** — positional counting
makes one extra instruction read as "everything wrong" AND scores every source variant the
same, so a sweep cannot discriminate (four shapes all scored 180/258 while the aligned count
ran 64 -> 27); `eqcheck.py` now counts by diff alignment on a length drift and marks the row
`~`, which took `Skitter` from 171/201 to 91 and two `Scale`s off "100% wrong".
`VIPrimBuffer` was measured here and became Phase 14 above.
**Phase 12 added the PROPERTY BAG and the first TEMPLATE — `VIParticleAttributes` 55/55
and `VIPool<T *>` 133/159 across THIRTEEN instantiations, same toolchain, no re-tuning.**
`VIParticleAttributes`'s whole layout reads off its own accessors: every setter stamps a
version counter at `+0x2CC` (whose LOAD is the setter's first instruction — the scheduler,
not the source) and the **implicit** `operator=` settles every field's type by copying
member by member, so `+0x1C` is a `VIVect3`, the gradient is `VIColor32F[32]` and **`+0x2C8`
is a FLOAT** between two ints. Size `0x2D4` = 724, exactly what the ctor hands `memset`. Two
findings: **the five vector setters are an explicit `if`/`else`, not a `||` VALUE** (the
disjunction materialises 0 first where the target materialises 1 in the first delay slot —
Phase 9's `||` rule is about conditions, not values), and **an argument set-up instruction
belongs to the `jal` ABOVE it as often as the one below** (reading them the other way merged
two ctor calls and lost `SetStartSizeVar`, the whole of a 65-of-175 miss). `VIPool` is the
engine's intrusive free-list pool — `Node {T; int next; int prev;}`, `-2` in `prev` means
free, **`End()` is 0x7FFFFFFF** as `go-eqoa-vi` found from the console, and `Erase` returns
the successor and matched byte-exact first try at 55 instructions. New levers: **a branch to
a shared epilogue carrying that epilogue's own first instruction in its delay slot is ONE
return path, not two** (the delay-slot filler, and it only happens when the success path is
in the `if`); **a loop variable that is not the parameter is two spare `move`s** (`int j =
i;` took `LinkFreeChain` from 39 wrong to 0); **`if`/`else` with ONE return at the bottom is
what cross-jumps**. Two sites **test a node address for null before writing through it** and
nothing in the compiler produces that — it is written, and it is not a reference conversion.
**The instantiations are declared MEMBER BY MEMBER**: not one of the thirteen carries the
whole class, so `template class VIPool<X *>;` would emit ten functions the module never had.
The 26 misses are two functions ×13 — `Add` is ONE word of operand order, `Allocate` nine,
whose closing `if (x < 0) return -1; return 0;` if-converts where the target branches (the
control is `Init` in the same class, where the identical idiom if-converts in the TARGET too
and the function is byte-exact). Harness: **`eqdecomp.SPAN_CAP`**, a hand-audited table of
four symbols whose gap-derived span swallowed whole neighbouring functions, held to
"shrink only, land one word past a `jr ra`, leave another `jr ra` behind" by
`test_span_caps_only_ever_shrink`.
**Phase 11 added the first CONTAINER and the first INHERITANCE — `VILNode`/`VILList`
19/20, `VICollRay` 7/8, `VICollSphere` 4/4, `VIPickSegment` 11/11, same toolchain, no
re-tuning.** `VILList` is **Amiga Exec's list** (`Init` is `NewList()`, so both ends of a
walk hit a null the header provides). `VICollRay` is the first class whose SEMANTICS were
settled before the decomp — `go-eqoa-vi/vicoll/ray.go` is a console-checked port of exactly
these functions — so every mismatch was codegen. `VICollSphere` inherits with **no vtable**
and **repeats** the base's `Init`/`Precalc` body rather than calling it. New levers: **a
forwarded field write is not a reload** (`n->m_next->m_prev = n` vs `m_head->m_prev = n`);
**an accumulator beats an early return**; **compound operators are how a sub-object gets a
base register** (`m_delta -= m_start` folds +0 and puts `&m_delta` in a register — writing
the components by hand is 62/62 wrong); **the zero-length block is the ELSE**; **a dead
store to an address-taken local survives across a CALL and not otherwise** (so the three
matrix-transform `Init`s reuse ONE `VIVect4`); and Phase 9's **branching** `if (cond) return
-1; return 0;` is what if-converts to `slti`+`movz` where every ternary gives `slt`+`movn`.
Shipped facts: the sphere's radius is scaled by **inline `sqrtf` with the NaN retest**, and
`VIColor32F` copies as an **unaligned `ldl`/`ldr` block move**. The two open functions are
the standing phenomenon — `Node` is **`greg`** (caching the next pointer moves `p` into
`v0`, the return register, splitting three cross-jumped exits) and `Skitter` is the
CSE-vs-store-alias half.
**Phase 10 added the FIRST NON-MATHEMATICAL class — `VIString`, 46 of 47, same
toolchain, no re-tuning**: `new[]`/`delete[]`, char and UCS-2 pointer walks, C varargs,
`alloca`, and members returning a class BY VALUE (`operator=` included — a0 is the sret
slot). `m_len` +0, `m_size` +4 (BYTES), `m_str` +8. New lever: **a loop whose condition is
also read in the body is a GUARDED DO/WHILE** — `if (*a) { do { … } while (*a); }` — a
plain `while` leaves the test at the top where gcc reuses its load (`strcmp16` 14/14 wrong
through six `while` spellings, byte-exact first try guarded). Second lever: **a copy loop
WALKS THE PARAMETER**, base in a local — five functions were right-instructions/wrong-
registers without it. **`delete []`, not `free`**: `Allocate` tests `m_str` twice (source +
delete's own null check), `Deallocate` once. **`__builtin_next_arg` is NOT `va_start`
here** — the EABI register save block sits BELOW `virtual_incoming_args`, so it lands 40
bytes high; `- (8 - __builtin_args_info(2)) * 8` fixes it and no flag does. Another shipped
bug: **`strcat16(…, const char *)` sign-extends its bytes** where `str8to16` converts them.
Findings: `decomp-phase11-containers-and-collision-20260823.md`,
`decomp-phase10-vistring-20260823.md`,
`decomp-phase7-viplane-and-invertaffine-20260822.md`,
`decomp-phase6-double-siblings-20260822.md`,
`decomp-phase5-viquat-and-vivect-20260822.md`,
`decomp-phase4-vimatrix33-20260822.md`, `decomp-phase3-vimatrix44-20260822.md`,
`decomp-feasibility-20260822.md`.

