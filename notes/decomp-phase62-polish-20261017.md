# Phase 62 — the CLIENT polish pass (2026-10-17): 3,761 -> 3,849 of 3,934 byte-exact

The 18 lanes (`decomp-phase62-lanes-20261017.md`) left CLIENT written but untidy. Main stood
at 3,761 of 4,008 scored rows byte-exact, with 153 rows written but open and 9 functions
unwritten. Merge hacks, lane scaffolding and a scratch TU were still in the tree. The
polish pass did five things:
- fixed the bookkeeping;
- recovered what the merge had broken;
- closed opens with ten more lanes;
- folded the scratch TU back into VIClient.cpp;
- removed the scaffolding.

**Result:** CLIENT is **3,849 of 3,934 byte-exact**, plus 1,921 verified duplicates and 112
functions too small to attribute.
- **Opens:** 85 written but open, down from 144 at the start of the lane work, and 4,862
  wrong words, down from about 11,700. That is 117 KB of code.
- **Unwritten:** 5 functions (1,120 bytes), all dead per-TU copies of VIItemList.h members.
- **Bytes:** exact, duplicate and tiny code together is 1,008,788 of CLIENT's 1,126,740
  bytes.
- **Checks:** eqcallcheck finds 0 wrong callees in 16,676 call sites, and eqrefcheck 0
  data-reference findings.
- **Other modules:** STATION gained one function from ports (1,984 -> 1,985). SUPPORT 3,665,
  SETUP 1,661 and MCERR 44/44 are unchanged.

## 1. Bookkeeping first: 3,761 -> 3,786 before any open function was worked

Half the gap between "written" and "exact" was not code at all.
- **94 NO-TGT rows** were symbols the build emits that the map does not have.
  - Stale placeholder instantiations: `client_vivector.cpp` still instantiated
    `VIVector<VIElem_CAA028>` .. `<VIRec12_CB30D0>` after cl-char had named those types (the
    `VIDbChar*` rows), and vicharacter.cpp emits the real copies.
  - Fourteen `VIArray`/`VIPool` constructors (`move v0,a0; sw zero,0(a0); jr ra; sw
    zero,4(a0)`) were still `func_` in the map. The callers name them: where the target
    calls a placeholder and our build calls a real name at the same offset, that name is the
    row's. This is now a tool, `decomp/eqcallnames.py`. The same pass named the
    `VIArray<VIItemStat>` copy constructor and libgcc's `__rtti_si`/`__rtti_user`.
  - Two signatures were wrong. `VIWndList::Nothing` takes its point BY VALUE. The empty
    inline `VIWndPrivChat::SetPos(const VIVect2I &)` is emitted after its `__tf`
    (0x00C84878).
- **7 CALLEE rows** were byte-exact but called the wrong name.
  - Lane stand-ins for `VITextSet`'s statics were free functions (`GetRoot()`, `MakeId()`,
    `Remove(VITextSet *)`).
  - cl-tail's `VIRec792_BBC168` was cl-pre's `VIAuctionItem`.
- **4 merge regressions.** In each, a lane's header version overwrote the spelling another
  lane's function needed. The lanes' own trees were rescored and compared per function
  against main.
  - DoAction went 25 -> 507. `VIBankControl::Do` and the faction part's `Command` had been
    re-declared with ONE parameter. DoAction hands every subsystem `(id, arg)`, and the
    target loads `a2` before each call.
  - The VIClient constructor went 5 -> 56. `VIPlayer`'s +0x4C/+0x58 were `float[3]`; they are
    `VIVect3`s the constructor's roll-call zeroes.
  - VIWndCharCreateName::OnDrawSelf went 0 -> 101. `VIWndEdit::GetEnd()` must be
    `m_pos + m_size`, not `VIVect2I(m_pos.x + m_size.x, ...)`.
  - VIWndGoto::OnDrawSelf went 0 -> 5. `ZoneName` returns `unsigned short`: the target hands
    it to `DrawStringEx(unsigned short)` with no `andi`.
- **Merge hacks removed.**
  - cl-uitext's `VIRecipe` and cl-parts2's `VICraftDetail` were one 0xB4-byte record. It is
    now one class with the semantic field names, and its lists are
    `VIArray<VIRecipeIngredient>` and `VIArray<VIRecipeTool>`.
  - cl-wq's `VIMiniMenuDef` was cl-wl's `VIMenuDef` with four `VIMenuEntry`s after it.
  - The buddy menu was `char[]` behind a cast; it is now an `extern VIMenuDef`.
  - The by-address `D_<ADDR>` data and the `gMenuDef02A0/02F0` placeholders are named after
    their roles: `gStartMenu`, `gEquipMenu`, `gHudMenu0..2`, `gInvMenu`, the start-menu
    item tables and so on.
  - 70 lane marker lines and 169 lane tags are gone, verified neutral.
- **`visort.h` was rewritten.** cl-char had transcribed it from Dinkumware's `<algorithm>`
  (VC++ 5/6, all rights reserved). It is now our own text with the same template shape: a
  quicksort loop that recurses into the smaller half down to 16 elements, a median of three
  BY VALUE, a null `T *` value tag, and an insertion sort to finish. vicharacter.cpp stays
  560/560 and its callee check is clean. The out-of-line loops are `SortLoop<...>`
  (0x00CB60C8, 0x00CB6358).
- **Four of the nine unwritten functions were written.**
  - `FindWeaponTypeWord` (0x00C0D000) is a one-function TU between VITextList's and
    VIWndAuction.cpp. Its `.rodata` is game.h's table-name strings and then an 8-row
    `{word, type}` table: 1HS, 2HS, 1HB, 2HB, 1HP, 2HP, RANGED, SHIELD.
  - `VIClientPart4EC58::Use` and `VIWndTell::Close` are empty bodies named off their only
    callers.
  - `VIWndMenu4Edit::SetDepth` is the class's one inline, emitted after its `__tf`
    (0x00C77748).
  - The VIMap<int,VIAbility> destructor came with the fold (section 3).

## 2. Ten polish lanes: 57 functions closed

Each lane owned a disjoint set of TUs, from a brief plus a manifest seeded with the old lanes'
diagnoses. Wave 2 (`b`) re-took the leftovers of two wave-1 lanes. "Closed" means
byte-exact.

| Lane | Closed | The functions, and the source fact that closed them |
|---|---|---|
| pl-ui1 | 17 / 26 | **NetStat::UpdateTexture** 108 (`int res = -1; if (m_mat == res) return res;`, a pointer copy for the first loop, `white` built field by field); **CreateTexture** 45 (a goto chain to one exit: the label stops jump.c's conditional move); **DrawCompass** 106; **Trainer::DrawItems** 95 (a second `VIUI &u` used only for the colour); **DrawTarget**/**DrawGroup** (`Init(UI(), ...)` not `Init(mUI, ...)`); **DrawPet**; **BankItem** (the dead frame-param stores are a 3-cycle permutation); **FillBait**, **Fishing::Tick** (one `float t` set in two blocks is GLOBAL to local-alloc); Trainer BaseStat/CurStat/ItemUp/Adjust; Chat::SetMode (stores in each arm, merged back by jump2); Attack; **Guild::OnDrawSelf** (`GetMemberOnline` returns int; `SetTextColor` through a static inline) |
| pl-ui4 + pl-ui4b | 12 / 29 | **SpellBook::OnDrawSelf** 494 (a `lit` flag written for a conditional move, the literal 39, a reused `i`, a NAMED `off` operand reversing which value takes the freed slot, `p.x += 57 - w / 2`); **Faction::OnDrawSelf** 298 (temporaries for pos/size, `unsigned short *title = buf`, `VIUIWindow &win = m_win3`, `m_win2.Init(VIWnd::mUI, ...)` beside `m_win3.Init(UI(), ...)`); **Menu4::DrawChoice** 54 (the permuter: the colour as a TEMPORARY in `TextBox`); Menu4::OnKeyUp, Menu4::OnMessage (`case 295:` first); Loot::Init and Merchant::Init (`goto fail`, with Merchant's unused `VIVect2I` in a block closing before `fail:`); Menu::OnDrawSelf; Menu4Edit::DrawText; SpellBook::Up; Faction::Down; MercMenu::Refresh |
| pl-vic | 8 / 26 | **Tick** 242 (the journal's buffer read through an INLINE `VIJournal::Read(T *, int)`: `a1 = sp+16` goes straight into the call, so gcse's LCM has no address pseudo to hoist across the assert); **EnterGameLoad** 187; **BeginManualCamera** 124 (an inline forwarder for the two `BeginManual` calls); **Msg00B4** 84 (the permuter: `VIUI &ui = m_ui;` after the second loop makes loop.c treat that loop as "phony"); IsAbilitySlotSpell; VIAbility::Read (single exit); Msg001A, Msg0040 (`m_inventory.Count()` instead of a list reference). **CreateCharacter** was closed by the coordinator off pl-vic's diagnosis: `VIRaceDef::m_model[2]` in vi.h (an ARRAY_REF indexes base first; SUPPORT never names the fields) |
| pl-misc | 7 / 21 | **GetSwing** 49 (if/else chains of returns; this also removed the module's one eqrefcheck finding); VIJournal::Eof; InviteDeclined (`case 1:` and `default:` as two identical bodies); ParseToken; ApplyLook; VITextList::Draw (`if (...) break;` instead of a `for` condition); SetVisible |
| pl-vsb | 4 / 9 | **LoadWorld** 285 (`VIDictionary *dict = scene.m_dict;` before the hash call: one more early pseudo moves every global up a register and the frame to 1,376); the **VIClient constructor** (in a run of stores off one base, the store the target emits first is written LAST); **DoAction** (each `%05.2f` arm ends in its own `AddText`); **PlayLoop** (the Find/Play block written once per branch) |
| pl-ui2 | 4 / 15 | **ClassMastery::OnDrawSelf** 211 (`VICMSkill *&s = sAvail[GetSel()]`, `msg = text` after the dots loop, an inline `Points()` accessor); **ComposeGram::OnDrawSelf** (one shared `char d`); **MBox::DrawIcon** (a switch returning through a result variable); **VIWndEdit::OnKeyUp** (a store written in both arms becomes a NON-annulled delay slot) |
| pl-ui3 + pl-ui3b | 3 / 14 | **Buddy::OnDrawSelf** 443 (a TU-local inline `DrawWord`, temporaries, `Width()`, `break` not `continue`); CharCreateBlank::Init (nested if/else and one `return r`); CharacterSelect::OnMessage. Wave 2 closed none but cut 787 words and derived the temp-slot rule (section 4): CharCreateClass 801 -> 351 and VIWndEdit::OnDrawSelf 484 -> 344 now have the target's exact frames |
| pl-vsa | 2 / 4 | **SendPosition** 355 (`VIBBox old = range;` copy-init instead of `old(range)`, 296 -> 4 alone; a local view struct copies `VIWalkPhysics` in field order where g++ copies an anonymous union last); **UpdateControls** 281 (the target's dead `1.0 < fabsf(height)` compare kept as `if (...) fwd = 0.0f;`; clamps as `VIMIN(VIMAX(...))`) |

## 3. VIClient.cpp is one TU again

cl-vic had developed 15 VIClient.cpp functions in a scratch TU, `vicscratch.cpp`. It builds
in 0.4 s against viclient.cpp's 30 s. Two of those functions carried `#line` directives to
pin their asserts. The fold put every one of them back:
- each goes after the viclient.cpp function with the nearest LOWER address, inside its
  assert region;
- the helpers (two inline functions, a macro, an enum, a view struct, two statics) move to
  the top of the file;
- every asserting function is re-padded onto its line: Connect 1705, ConnectShard 1790,
  WriteQuantized 2207, SendPosition 2277, Msg000D 9800..9821, ProcessMessage 10306 and
  Tick 11682..11727;
- the `#line 1 "VIClient-tail.cpp"` hack became a re-sync on VIClient.cpp's own physical
  line;
- vicscratch's duplicate VIAlarm constructor was dropped, because vialarm.cpp owns it.

**The fold broke Tick (0 -> 237), and the reason is a gcc fact.** InitBody calls
`VIJournal::Read<unsigned char>(T *, int)` BEFORE the template's inline definition, which
sits just above Tick. In one TU, a specialization that has been CALLED before its inline
definition is never integrated afterwards: Tick then called it too. The target's InitBody
reloads its byte with `lbu`, so the variable is unsigned. The fix keeps it unsigned and
instantiates a DIFFERENT 1-byte specialization, `j.Read((char *)&c, 1)`. The out-of-line
copy (0x00BE3680) is then `Read<char>`, and Tick integrates `Read<unsigned char>` again.

Everything else kept its score. The fold also emitted the VIMap<int,VIAbility> destructor
(0x00BDF070), one of the unwritten nine, and it is exact.

**Code earlier in a TU moves later functions.** gcc 2.95 hashes some RTL by pointer (cse's
SYMBOL_REFs; eqperm's prefix rule exists for this reason). pl-vic measured the same effect on
the dispatcher: its last 17 words are gcse hash-bucket order, and the TU's instruction count
decides it. Treat any TU restructuring as a code change and score it whole.

## 4. Levers worth keeping

The working memory holds them as one file (`feedback_client_polish_levers.md`); the lanes'
`NOTES.md` have the evidence.
- **The temp-slot rule, exact** (pl-ui3b, from `function.c`, reproduced in a standalone
  test).
  - `assign_stack_temp` takes an EXACT-size free slot first, else the smallest larger one.
  - It splits a larger slot only if `size - roundup16(request) >= 16`; otherwise the
    request takes the whole slot.
  - A new slot records the REQUESTED size (8 for a `VIVect2I`), and merged free slots ADD
    their sizes.
  - So two or three freed `VIVect2I` temps form a region that an 8-byte request swallows
    whole. A frame that is "too big" is a source shape, not missing reuse.
- **A colour's `this` survives a non-zero float literal** (pl-ui3b). An inlined
  `VIColor32F(r, g, b, a)` folds `this` into a store only when the stored value is a valid
  operand. A non-zero float literal is not, so gcse copies the address and CSE2 keeps it
  live. `black(0, 0, 0, alpha)` with `float alpha = 1.0f` removes it.
- **The dots loop's `slt` destination says whether `d` is shared.** Per-loop
  `for (char d = 0; ...)` stays block-local and local-alloc ties the exit `slt` to it
  (`slt v0,v0,v1`). One function-scope `char d;` is global and ties to the limit
  (`slt v1,v0,v1`). This closed GramInbox and ComposeGram, and helped three CS windows in
  CLIENT and in STATION.
- **A switch returning through a result variable keeps an early `return -1` inline.** A
  store written in BOTH switch arms lands in a non-annulled delay slot (reorg's
  try_merge_delay_insns).
- **An inline accessor call as an argument** (`Init(UI(), ...)`, `GetP4EC58()->Points()`)
  expands into a pseudo before the next argument's call. A plain member is left as a MEM
  and loaded late.
- **The global-CSE pass numbers its pointers in hash-bucket order, and the table size
  follows the function's instruction count.** So spill-slot order tells you how many
  instructions the original had at that stage (pl-ui4b). After global allocation, a
  frequently used value can take a hard register back from block-local ones; constants
  lose first.

## 5. STATION, from the same source

The CS windows and VIWndEdit are STATION's source too. Each CLIENT edit was ported by a
three-way merge of CLIENT's before->after diff into STATION's file:
- VIWndEdit::OnKeyUp closed in STATION (1,984 -> 1,985);
- STATION's VIWndEdit::OnDrawSelf went 827 -> 364, with the length right;
- WrapLines went 32 -> 8;
- CSInbox, CSRequest and ReadCS OnDrawSelf each lost 12 words.

STATION's `GetEnd()` takes CLIENT's `m_pos + m_size` spelling, which is neutral there.

## 6. What is left

**85 open functions, 4,862 wrong words.** Each lane's `NOTES.md` has the diagnosis per function.
- **Big draw functions, frame right, allocation wrong** (7 over 150 words):
  - CharacterSelect 456 (a colour constructor's `this`);
  - CharCreateClass 351 and VIWndEdit::OnDrawSelf 344 (both now have the target's frame);
  - Recipe 324 (the target spills one more pseudo);
  - CharCreateRace 253;
  - Trainer::DrawStats 202 (a whole-function register permutation);
  - VIClient::Process 175 (a temp address at sp+208 keeps a register).
- **Mid-size** (19, 41..150): CS windows, TrackTarget, CycleTarget, VIWndList, TradeList,
  MainMenu, BuffPC, Blacksmith, Experiment and others, mostly register-priority ties that
  wave 2 sized exactly.
- **Ties under 40 words** (59): every one diagnosed. Most are a priority tie of two pseudos,
  a sched2 rank order, or a gcse hash-bucket order. A 10-minute permuter run per function
  found nothing for most of them before the pass, and a final unattended batch reran 38
  (section 7).
- **Unwritten, 5:** 0x00BDFFE0/0x00BE0048 (VIClient.cpp), 0x00BED650/0x00BED810
  (VIClientPart3FC10's TU) and 0x00C01C88 (3834C's TU).
  - They are each TU's own dead copy of VIItemList.h's `GetSlotId` (:315) and `Took`
    (:326).
  - Each references its own TU's copy of the assert strings, so the scorer cannot credit
    them as duplicates.
  - In the original, VIItemList's out-of-line members were emitted in EVERY TU that used
    them, which means a template or an equivalent. Scoring them needs VIItemList as a
    template, or per-TU attribution of global symbols in eqcheck.
- **Not done, recorded for later:**
  - VIClient's `SendOp*` senders (0x00BE4B78..0x00BE6470, their own TU after VIAlarm.cpp)
    still sit at the end of viclient.cpp.
  - VIItemList.h's members are still defined there under `#line` rather than in a header.
  - viwndmessage.cpp and viwndoptionlist.cpp are byte-identical to STATION's apart from
    the include, and could be shared by symlink.

## 7. The final permuter batch

eqperm ran unattended over the 38 smallest opens: 4 minutes each, `-j 6`, seed 1017
(`decomp-perm/batch-20261017-1206.md`). Fifteen VIClient.cpp entries could not set up,
because the file defines a stringising macro (`VI_TRACK_RANGE`). eqperm now refuses only a
function that USES one, and a second batch re-ran those fifteen
(`decomp-perm/batch-20261017-1343.md`).

**Batch 1: 21 searched, 9 reduced finds, 2 matches.** Both matches went through spellings the
README's fakematch rule rejects (a `decltype` temporary, a one-use reference), so neither was
applied as found. Each one pointed at a source fact:
- **ApplyItems 1 -> 0, closed.** The find took a one-use reference to `type`. What that
  changed was the inline `SecondModel`'s parameter: the target compares a COPY of `type`
  with no extension.
  - An `unsigned char` parameter ands it with 0xff.
  - An `int` parameter shares the register.
  - A `short` parameter gives exactly the copy: the byte loads already fit, so combine drops
    the sign extension.
- **SelectCategory: `sOwned` and `sAvail` are ONE array.** The find (19 -> 0) needed a bool
  temporary and a one-use reference to `sCount`. The real lead is in the image.
  - SNR1's relocation table gives each site its full target. The instruction keeps only the
    original OBJECT's addend.
  - A static of its own reads 0 there: viplayerrecord's statics at +0x10, +0x20 and +0x40,
    and vitextset's `sPending` at +0x400.
  - Every reference to the available list reads 800 (0x320). The original addressed it as an
    offset into one object: `static VICMSkill *sSkills[2][200]`. Its
    `memset(sOwned, 0, sizeof(sOwned) + sizeof(sAvail))` had already hinted at that.
  - With the one array, the swapped spill slots and the s0/s1 swap close with no
    temporaries.
  - One real difference is left. In the last loop, expand adds `sSkills` and 800 as two
    insns, and CSE keeps `r + 800` beside a REG_EQUAL. loop.c strength-reduces from that, so
    the base takes two `addiu`s where the target loads `%lo(sSkills+800)` once.
  - `*(sSkills[1] + i)` folds it but flips the spill slots back. A one-use inline in the
    known-skill branch on top of that matches. Neither is natural, so the function stays
    open.
  - The count went 19 -> 33. An open function's count includes the relocated immediates
    (they differ until everything else matches) and a two-word shift. The real difference
    is one instruction and a nop.
- **Sync 18 -> 14.** The store of `p.m_flags` goes last, after `m_targetHeading`. A statement
  order is a source fact, so it was applied.
- **Rejected:**
  - WrapLines 8 -> 4: a one-use reference to `m_break`. A block reference to the whole line
    scores 16 to 57.
  - Menu4::OnDrawSelf 23 -> 16: a condition temporary, `int t = m_hold >= 0`.
  - Four finds whose gain was in the allocation and length weights, not in wrong words:
    Disconnect::OnDrawSelf 9 -> 10, Faction::DrawStanding 9 -> 88, VITextList::AddText
    12 -> 19 and BankControl::Move 13 -> 13.
- **Not searched:** `VIItem::SetFrom`, whose function does not build alone.
  `VIChatText::AddText` is overloaded and needed `--line`; it ran on its own (below).

**The residue check, module-wide.** The same comparison over every matched function, at
every relocation against a named symbol:
- CLIENT: 0 of 1,182 sites differ. SelectCategory's array was the only object split in two.
- STATION 0 of 2,566, SETUP 0 of 2,958, MCERR 0 of 106.
- SUPPORT: 24 of 928 differ, all at named statics where the target's residue is a large
  section offset: the DRDP library's tags (`NULL_TAG`, `DRDP_TAG`, `CONNECTION_TAG`, ...),
  libgcc2's `p`, `OptionsFile` and `VIWadFile::WadFileID`. The SN assembler keeps a
  relocation against a named local symbol by name, as our objects show. So the original
  either referenced an anonymous literal there, or the object was assembled by a GNU `as`,
  which rewrites local symbols as section offsets. This is not resolved. It changes no
  matched byte (the field is masked).

**Batch 2: the fifteen VIClient.cpp entries, nothing.** Every entry set up and searched, with
376 to 3,394 builds each: about two a second, because every build compiles all of
VIClient.cpp. No entry improved in wrong words. The one reduced find, GetInvItemSellPrice
with its multiply's operands swapped, stays at 13.

**VIChatText::AddText 6 -> 3.** It ran alone with `--line`, because the method is overloaded.
- The find was a one-use temporary for `pos + 1`. `pos -= n - space - 1;` gives the same 3
  with no temporary, and so does `pos += 1 - (n - space)`.
- The last three words are the address of `m_lines[m_tail].m_text`: the target adds `this`
  before the 16-byte member offset, ours after. No spelling of the argument moves it.

**Net of the batches:**
- ApplyItems closed, 3,848 -> 3,849.
- Sync -4 and AddText -3.
- SelectCategory +14 by the count, which is one real instruction.

The 4,859 wrong words first quoted for this pass came from the run before CreateCharacter
closed. The 86-open state was 4,856.

## Files

- `decomp/modules/client/src/viclient.cpp` is the whole of VIClient.cpp now. It is
  line-padded up to Tick: see section 3 for the anchors.
- `visort.h` is rewritten.
- `viweapontype.cpp` is new.
- `decomp/eqcallnames.py` is new.
- The final batch changed `vicreatureentity.cpp` (SecondModel's `short`, Sync's store
  order), `viwndclassmastery.cpp` (`sSkills[2][200]`) and `vichattext.cpp` (AddText). In
  `viwndmenu4.cpp` it fixed one line's indentation only.
- The polish scripts lived outside the tree and are described here rather than kept:
  `fold.py` (the fold and re-padder), `tidy_markers.py`, `apply_data_names.py`, a
  per-function lane-vs-main comparison, and the residue scan. The scan compares the
  instruction's residue with our object's LO16 addend at every relocation against a named
  symbol, using `E.module_relocations` for the target side.
