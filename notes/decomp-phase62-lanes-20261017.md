# Phase 62 — the 18 CLIENT lanes (2026-10-16/17): 1,208 -> 3,761 byte-exact

The record of the parallel lanes that wrote nearly all of CLIENT, condensed from their
eighteen `NOTES.md` files (each lane was a `_lane.py` tree, `decomp-cl-<lane>/`). The lanes took
CLIENT from 1,208 to 3,761 byte-exact. After the merge there were 1,936 verified duplicates,
153 written-but-open functions and 9 unwritten ones. The polish pass that followed is
`decomp-phase62-polish-20261017.md`. The phase's main note is
`decomp-phase62-client-20261015.md`.

Key: "W/E" is the lane's own written/exact count for its range. "Lane a/b" is the
whole-module eqcheck in the lane's tree, which started at 1,208/1,254. "(recorded)" means the
lever was saved to the working memory during the run. "(maybe known)" means an overlapping
lever already existed. Lane-built tools: store-order permuters and hill-climbers (wb
`rperm.py`, wc `permtry.py`/`permhill.py`, we `permlines.py`, wm `permfn.py`/`permhill.py`, ws
`hill.py`), vic `linemove.py`, wh `pad.py` (assert-line padding), wg `addfor.py`, pre
`tuorder.py`/`tryv.py`. They lived in the lane trees and are not part of `decomp/`.

## 1. Per lane

**cl-vic** (0x00BBF0D8..0x00BE3C28) holds VIClient.cpp's leftovers. VIClient's vtable is 0x00CDB4C0 (19 entries). Its functions:
ctor 0x00BBF698 (474 words), dtor, InitBody, Process, LoadWorld ("PreGame Load"), UpdateGame (one in-game frame), UpdateControls,
TrackTarget (selection rings), SendPosition (the 0x40), DoAction (a 61-case action dispatcher), ChatLine (the /command parser),
SendReport (0x00F0), DefaultTextSets (the quick-chat tree, generated from the disassembly) and Msg000D (the MemoryDump, 435
words, exact). All 33 written, about 13 exact; the 13 large ones are open, and the ctor is 5 words off. Of the 32 inherited opens,
6 closed: JournalTick, JournalNetwork, JournalConsole and Init through cl-tail's void VIJournal; ShowA110 through cl-pre's
signed Show; CharacterList 79->0. ConnectShard, Connect and Msg076F went from 30/36/9 to 8/12/5 wrong words. Lane 1,229/1,282.
Its scratch TU `vicscratch.cpp` builds in 0.4 s against viclient.cpp's 28 s.

**cl-pre** (0x00BB4A80..0x00BBF0D8): four TUs, 170 rows written, 167 exact. Lane 1,345/1,393. `viadmin.cpp` (name ours, 59/59)
holds VIClientPart382E8 (VIClient+0x382E8, 24 B), the GM/debug command channel, plus the 192-byte VIAuctionQuery `<<`/`>>`.
`VIBankControl.cpp` (assert :593, 90/92) holds VIBankControl (0x78D0 B), the bank and auction control, with VIAuctionItem :
VIItem (792 B); VIItemList.h's live Read/Get are emitted there. `unknown_bbdc80.cpp` (11/11) is a dead class plus its VIMap.
`VIClientText.cpp` (provisional name; VIClient+0x4FC24, the `|t` token expander) is 7/8.

**cl-parts1** (0x00BE3C28..0x00BF5628): NINE original TUs, each closed by its own template-copy run. About 183 written, 181
exact. Lane 1,349/1,393.
- VIAlarm (VIClient+0x4F098, 24 B; its ctor sat in cl-vic's range): 5/5.
- Seven new VIClient senders, plus SendOpByteByte fixed: all 28 senders exact.
- 4FD28, damage numbers (0x5AC B): 8/9.
- 4F078, factions (32 B), with the new `vimemstreamarray.h`: 13/13.
- 3FC10, fishing (0x7774 B): 20/20.
- 2EFC8 (4 B): 4/4.
- 2EF24, guild commands (0xA4 B): 38/39.
- VIClientGuild (0x2A0 B, rewritten in address order): 42/42.
- 06D50, the icon cache (0x40 B): 11 functions plus 31 container members, all exact.

**cl-parts2** (0x00BF5628..0x00C04D10): eight TUs, 215 written, 214 exact. Lane 1,394/1,434.
- VIClientPart2EFD0, the gram (mail) client: 18/18.
- 4EC58, class mastery (0x420 B): 42/42.
- `viclientgrid` (VIMaskIndex, dead code): 4/4.
- 38300, the ignore list: 17/17.
- VIShardInfo.cpp (shard list and race tables): 22/22.
- 3834C, crafting (0x78C4 B): 75/75.
- 38334, the who list (24 B): 18/18.
- `vichattext` (word lookups, plus VIChatText, 5,136 B, which is VIWndChat's +0x1A8): 18/19.
- It replaced six placeholder pools with their real types.

**cl-uitext** (0x00C04D10..0x00C0D000): nine TUs, found from the data (each TU's .rodata ends in one `bad_alloc`). 125 written,
template copies included, 123 exact. Lane 1,325/1,373. The TUs: `vifont` (CLIENT's own VIFont.cpp: SUPPORT's text plus
GetSexWord; 15/15), `viinfotext` (VIInfoText, 4 B: 27 item-description builders plus kind predicates and helpers; 52/52),
`vikeyflash` (VIKeyFlash, 52 B, the pad auto-repeat tracker; 7/7), `vitextset` (the quick-chat tree; 12/12), `vistringtable`
(SUPPORT's file byte for byte; 10/10) and `vitextlist` (VITextList and VITextLine, 36 B each; 23/25), plus the small
`viwearslot`, `vimenudefs` and `vitargetword`.

**cl-char** (0x00CA4658..0x00CB6020; the TU really ends at 0x00CB6B08): ONE TU, `VICharacter.cpp`, 557/557. It was merged first.
Lane 1,614/1,657. It is the SERVER's database layer, compiled in and never called. 18 tables are VIDb<T>Key/VIDb<T> pairs, each
with `<`, `==`, KeyName over the image's column names, `<<` and `>>`. Six more tables are row-only. VICharacter (296 B) holds the
CHARACTERS columns and 16 row arrays, with Diff (17 KeyDiffs) and FlattenChatMenus (6,424 B). CapitalizeName is here too. Headers:
`vikeydiff.h`, `vimemstreamarray.h` (17 instantiations) and `visort.h`, a transcription of Dinkumware's sort (rewritten in the polish pass in our own words, same shape). Ten empty `jr ra` stubs
are left unwritten.

**cl-tail** (0x00CB6020..end): 16 TUs, most names ours. 248 written, 238 exact by its final count. Lane 1,431/1,485.
- `virandom` (VIRandomMT, STATION's file): 7/7.
- `vifilestream` (was "VIJournalFile"): 12/12.
- `vijournal`: 10/11.
- `viplayerrecord`: 36/36.
- `vinetitem` (VINetItem.cpp): 20/21.
- `vicommandsender` (VIClientPart2EFCC, opcode 0x0C01): 5/5.
- `viaiextrapolate` (vt 0x00CF31F8): 3/4.
- `vientity` (VIEntity.cpp, vt 0x00CF3DD8): 41/41.
- `viswing` (VIClientPart06D00, the character-resource manager): 56/60.
- `vicreatureentity` (vt 0x00CF3600): 24/29.
- `viclientutil`: 8/9.
- `viexperience`: 14/14.
- `viargs` (a command-line parser): 2/2.
- Small ones, all exact: `vinetcharacter`, `vishardentry`, `visynctime`.

One `ren.py` run renamed 54 rows in MAIN's map; they were restored.

**cl-wa** (0x00C0D000..0x00C1A510): six window TUs, 91 of 92 written (0x00C0D000 is cl-uitext's), 90 exact. Lane 1,298/1,359.
VIWndAuction (848 B, vt 0x00CE0D58) 23/23; VIWndAuctionSearch (5,928, 0x00CE1328) 32/32; VIWndBank (52, 0x00CE1730) 7/7;
VIWndBankItem (1,464, 0x00CE1AC8) 11/12; VIWndBankMenu (432, 0x00CE1DD0) 10/10; VIWndBankMoney (44, 0x00CE20C0) 7/7.

**cl-wb** (0x00C1A510..0x00C26E50): six window TUs, 108 written, 99 exact. Lane 1,308/1,363. VIWndBlacksmith (2,328 B, vt
0x00CE23F0) 12/14; VIWndBuddy (504, 0x00CE27D8) 15/16; VIWndBuffPC (88, 0x00CE2AD8) 14/15; VIWndCharacterSelect (1,528,
0x00CE2E10) 18/21; VIWndCharCreateBlank (156, 0x00CE3130) 20/21; VIWndCharCreateClass (1,560, 0x00CE3438) 20/21. cl-wb posted
the form's layout early for cl-wc's derived Race page. Five OnDrawSelf are first drafts.

**cl-wc** (0x00C26E50..0x00C338F8): six window TUs, 104 written, 99 exact. Lane 1,307/1,358. VIWndCharCreateMisc (480 B, vt
0x00CE3810) 19/20; VIWndCharCreateName (72, 0x00CE3B08) 14/14; VIWndCharCreateRace (608, 0x00CE3E40; derives from
VIWndCharCreateBlank) 14/15; VIWndCharCreateReview (72, 0x00CE4208) 12/13; VIWndCharCustFrontiers (496, 0x00CE45E8) 18/18;
VIWndChat (5,592, 0x00CE48E8) 22/24.

**cl-wd** (0x00C338F8..0x00C41A48): seven window TUs, 136 written, 128 exact. Lane 1,315/1,367. VIWndClassMastery (584 B, vt
0x00CE4ED8; it emits its own VIList<VICMSkill *>) plus its Inspect (1,712, 0x00CE5430), Points (480, 0x00CE57D0) and Require
(560, 0x00CE5C18) pages; VIWndComposeGram (3,176, 0x00CE6048); VIWndCSInbox (856, 0x00CE65C0); VIWndCSRequest (2,560,
0x00CE6920). The CS windows are STATION's source with CLIENT substitutions. cl-parts2 must merge first.

**cl-we** (0x00C41A48..0x00C4E5F8): six window TUs, 127 written, 117 exact after a polish pass. Lane 1,325/1,381. VIWndDeposit
(528 B, vt 0x00CE6C38) 18/18; VIWndDisconnect (0x1A8, embedded at VIClient+0x2BB98, vt 0x00CE6F30) 7/8; VIWndEdit (448,
0x00CE7510; STATION's names, 28 of 31 byte-identical to STATION's) 38/41; VIWndExperiment (6,216, 0x00CE7820) 15/16;
VIWndFaction (1,256, 0x00CE7DD0) 11/14; VIWndFishing (2,064, 0x00CE80D8) 28/30.

**cl-wg** (0x00C4E5F8..0x00C58B78): ONE TU, `VIWndGame.cpp` (assert :4007); VIWndGame is 1,824 B, vt 0x00CE8918. 144 written,
135 exact. Lane 1,341/1,396. VIWndGame owns 37 panels at +0x680..+0x71C, and `m_mode` (+0x50, 0..41) picks one. The lane
renamed about 30 neutral panel names (BankClose -> OpenBank, ...). Its tail (five inlines plus `_GLOBAL_$I$`) lies in cl-wh's
range. One `ren.py` run hit MAIN's map (29 rows); it was restored.

**cl-wh** (0x00C58B78..0x00C685F8): ten window TUs, 147 written, 144 exact. Lane 1,352/1,401. VIWndGoto (60 B, vt 0x00CE8E30)
13/13; VIWndGramInbox (880, 0x00CE9168) 14/14; VIWndGuild (520, 0x00CE94E0; asserts 447/477/503) 15/16; VIWndHUDMenu (504,
0x00CE9910; three instances) 16/17; VIWndIdle (36, 0x00CE9C10) 7/7; VIWndIgnoreList (512, 0x00CE9F10) 15/15; VIWndInfluence
(44, 0x00CEA208) 8/8; VIWndInitFromMc (432, 0x00CEA5F8) 10/11; VIWndInspect (2,552, 0x00CEA908; new'd by four windows) 13/13;
VIWndItemMenu (1,648, 0x00CEACB8) 33/33.

**cl-wl** (0x00C685F8..0x00C77748): eight window TUs, 144 written, 125 exact. Lane 1,333/1,398. VIWndList (592 B, vt 0x00CEAFD0;
the base of Menu, MiniMenu and TradeList, adding virtual slots 7..10, of which 7..9 are inline) 28/29; VIWndLogin (880,
0x00CEB2F0) 10/11; VIWndLoot (1,464, 0x00CEB5E0) 8/9; VIWndMainMenu (1,072, 0x00CEB928) 19/23; VIWndMBox (0x5E0, 0x00CEBCA0;
STATION's class plus CLIENT's icon) 22/24; VIWndMenu (600, 0x00CEBF90) 9/10; VIWndMenu4 (76, 0x00CEC418) 9/15; VIWndMenu4Edit
(1,592, 0x00CEC728) 20/23. Eight other lanes copied its by-value layouts verbatim.

**cl-wm** (0x00C77748..0x00C860C8): ten window TUs, 132 written, 122 exact. Lane 1,330/1,386. VIWndMerchant (2,480 B, vt
0x00CECB40) 19/21; VIWndMercMenu (944, 0x00CECE38) 9/11; VIWndMessage (1,152, 0x00CECEB0; STATION's file verbatim) 13/13;
VIWndMiniMenu (864, 0x00CED1A0; derives from VIWndList) 14/16; VIWndMoneyMenu (424, 0x00CED4C0) 10/10; VIWndNetStat (44,
0x00CED7C0) 4/7; VIWndOptionList (520, 0x00CED838; STATION's verbatim) 10/11; VIWndOptions (2,432, 0x00CEDBF8; SETUP's file
plus three changes) 20/20; VIWndPrivChat (1,456, 0x00CEDEF8) 9/9; VIWndQuantity (504, 0x00CEE200) 14/14.

**cl-wq** (0x00C860C8..0x00C93680): seven window TUs, 140 written, 137 exact. Lane 1,346/1,394. VIWndQuestLog (496 B, vt
0x00CEE500) 18/18; VIWndReadCS (0xB760, 0x00CEE928; STATION's) 22/24; VIWndReadGram (8,672, 0x00CEED38; embeds
VIWndComposeGram) 21/21; VIWndRecipe (1,064, 0x00CEF0C8) 29/30; VIWndReply (608, 0x00CEF488) 18/18; VIWndResponse (2,512,
0x00CEF7A8) 13/13; VIWndServerSelect (1,904, 0x00CEFC20) 16/16. It made `VIUI::End2D` and `VIUIWindow::TickR` void.

**cl-ws** (0x00C93680..0x00CA4658): seven window TUs, 167 written, 156 exact. Lane 1,364/1,421. VIWndSpellBook (2,016 B, vt
0x00CEFF60) 41/44; VIWndStartMenu (44, 0x00CF02D0) 15/15; VIWndTell (1,768, 0x00CF06C8) 17/17; VIWndTrade (872, 0x00CF0D90)
26/26; VIWndTradeList (616, 0x00CF1080; derives from VIWndList) 12/13; VIWndTrainer (968, 0x00CF1708) 28/35; VIWndWhoList
(888, 0x00CF1BE0) 17/17.

## 2. New codegen levers (gcc 2.95 EE)

### Stream writers and temp slots
- **The opcode's spelling decides the next byte's slot** (recorded). `out.WriteU16(op)` is an inline temp freed at the statement's end, so the next byte reuses its slot; a named `unsigned short o; out.Write(&o,2,1)` keeps its own. ConnectShard 30->8, Connect 36->12, Msg076F 9->5. [vic; also pre, tail]
- **Two writes in ONE statement get two 16-byte slots** (`out.WriteU16(op), out.WriteByte(b);`); in two statements they share one. SendOpByteByte 14->0, Invite 4->0, InviteReceived 15->0. [parts1] (maybe known)
- **A chain containing an out-of-line call keeps every wrapper's slot only if the wrappers are taken by CONST REF.** `VIMemStreamIntC` matched all five paged auction requests. A const-ref vimemstream.h operator is neutral on CLIENT (1,323/1,384); STATION is unmeasured. [pre]
- **A named byte bound to an inline REFERENCE helper is placed at the frame's END at that moment** (`unsigned char sub = N; WriteRef(s, sub);`), and its address goes straight into the call. A reference helper for the opcode recomputes `addiu a1,sp,32` per call. Evidence: the gram senders. [parts2]
- **A bool written as a byte from a `lw` comes through a by-value bool parameter or wrapper** (`VIMemStreamFlag`, inline `WriteBool`); `WriteByte(field)` narrows the load to `lbu`. [pre, tail]
- **Destructor order tells the scopes.** The envelope stream's dtor runs before `m_state = 2` and the inner stream's after it, so only the envelope is in a `{ }` block. [parts2]

### Switches
- **The jump-table threshold is 5 case LABELS, and empty labels count.** A table with 4 live cases has an empty `case X: break;`: Recv0579 needed `case 6:` (38->3); Response::OnKeyDown needed `case 0:`, and that table also blocks loop.c strength reduction; the -1..3 focus switch needed `case -1:`. 0..3 gives a tree, 0..5 a table. [parts1, wq, ws, wd]
- **A table that starts below its smallest case is `case 0:` stacked on `default:`** (Expand) [pre]. States sent to the failure body are stacked with `default:` [wq]. `case 2: case 3: default:` puts the bodies after the tests (SetMode, DoAction) [wa, vic].
- **Non-consecutive case values never form a range.** Identical bodies that pass the switch operand itself (no `lui` of the case value) are STACKED labels. [pre]
- **Signedness.** 0xA1xx0000-style ids compared with `slt` are a signed int switch [pre, wq]. `if (d >= 0 && d < 4)` before a switch gives `sltiu` plus the signed tree, where an unsigned index gives a linear chain (Recipe::Difficulty) [wq].

### Loops and counters
- **A loop entered past its first test drops out of loop.c.** `int n = 0;` before `stats->Begin()`'s conditional makes an `if (all) goto body` into the loop, which loop.c rejects as "multiple entry points", so the high parts are never hoisted. `for (i = stats->Begin(), n = 0; ...)` closed AddStats. [uitext]
- **A table search compiled as a do-while with `slt` on a POINTER was an INDEX loop.** loop.c turns `i < 21` into a signed pointer compare; a pointer loop gives `sltu` plus a pretest. [parts2]
- **`slt p,end` with a counter that survives** is an index that loop.c eliminates plus a body counter: `for (int i = 0; m_option[i].m_text[0] && i < 6; i++) n++;` (Response::Move). [wq]
- **A `for` whose INIT and INCREMENT are the same call** shows as argument moves before the loop and a `b` landing ON the jal: jump2 cross-jumped the init's call. Chat::OnDrawSelf 70->44 words. [wc]
- **A down-counting exponent beside an up-going index** (`bgez` exit) is `for (i = 1; i >= 0; i--) ... a[1-i] ... pow(10, i)`, which loop.c does not reverse. Seen in BuffPC::Value and VIWndDeposit's `m_digit[8-i]`. [wb, we]
- **A dots loop wants ONE function-scope `char d` shared by every switch arm.** A per-arm `for (char d...)` ties the exit `slt` to the counter (`slt v0`), where the target ties it to the limit (`slt v1`). GramInbox::OnDrawSelf 22->0. The same residue is open in ComposeGram, CSInbox and STATION. [wh]
- **Two counters where one would do** (`shown++`, `row++`): with a single `i`, IgnoreList::OnDrawSelf is 59 words off. [wh]
- **Nine consecutive zero/ReadSigned stores are nine FIELDS**: -O2 does not unroll. [parts2]
- **Duplicating a call or tail into each arm raises its pseudo's allocation priority**, because cross-jumping merges the arms only AFTER reload. wa OnMessage (`w` 5000 vs `p1` 5185, read off .lreg); IgnoreList 268 16->0; ClassMastery::OnKeyDown; VIWndGame::OnMessage. [wa, wh, wd, wg] (maybe known)

### Init and return shapes (the base goto-done form was already known)
- **A shared `li sN,-1` with `move v0,sN` at the end is a result variable set to -1 at every failure**, with the `fail:` label INSIDE the last check's `if` (`if (!m_last) { fail: r = -1; } else {...} done: return r;`). Seen in the Init of Blank, BankItem, Trade, StartMenu and ItemMenu; wq saw the same exit. A named VIVect2I in scope makes the gotos illegal. [wb, wa, ws, wh, wq]
- **The -1 block stays in place because it is a goto target.** jump.c's "if (foo) bar; else break;" swap (~line 1900) only moves a block whose condjump goes to the NEXT label, and the `fail:` label is that label. [ws]
- **Cross-jumping keeps the LAST `{ r = -1; goto done; }` copy.** Each copy gives cse a skip-block whose PATHLENGTH (10) breaks constant sharing exactly where the target does. VIWndGame::Init 444->268->9. [wg]
- `return m_x->Init() ? -1 : 0;` for the last child's Init puts the -1 block before it. [wa]

### Addresses and operand order
- **Post-increment as its own statement.** `out[k] = v; k++;` gives the base-first addu (Select 23->0); so does `SetXItem(k, i); i++;`. Writing `SetRow(i, row); row++;` puts the increment in the jal's delay slot. [parts2, wg, we, ws]
- **A store through a named element reference** (`int &d = a.m_data[k++]; d = i;`) puts the base first (GetIndices). [parts2]
- **Test through the index, then name the slot** (`if (m_slots[i].m_key == key) { Slot *s = &m_slots[i]; ...`): the copy survives the join (VIKeyFlash::Pulse). [uitext]
- **A pointer to the element, with each use spelled `l->m_text[col]`,** gives `((data+line*36)+20)+col*4`. A reference folds the +20 into the column offset (SetText). [uitext]
- **A pointer difference used twice as an EXPRESSION, not a variable.** The memcpy size keeps `(d>>1)<<1`, while `buf[&text[i] - start] = 0` folds EXACT_DIV*2 back to the byte difference (VITextList::AddText). [uitext]
- **An angle wrap through an `operator[]` that returns a REFERENCE** (`while (m_hpr[i] >= PI) ...`) rebuilds `this + i*4` on every use. A `float &a` hoists the address; `(&m_hpr.x)[i]` puts the index first. Needs a new vi.h `VIVect3::operator[]` (WriteUpdate). [tail]
- `gWorldInfo[w].m_f` per field is an ARRAY_REF (base + index); a `VIWorldInfo &` gives index + base. [tail]
- **A reference to a pool element, not a pointer local** (`VICMSkill &s = skills.Value(i); list.Add(&s);`): SelectCategory 166->33. Two statics `sOwned`/`sAvail` instead of one array: 33->19. [wd]
- **A VIArray bounds test through an inline reference accessor** (`static inline int Size(const VIArray<T> &)`): its `this` is cse'd with the next subscript, giving `addiu t,this,124; lw d,4(t)`. Six SpellBook functions closed. Unify with cl-char's vi.h `Size()`. [ws]
- **A record spelled through the record at every use** (`D_sub[slot].m_items = ...; D_sub[slot].m_items[0].m_word = 122;`): cse forwards the stored pointer where no call intervenes and reloads it where one does. An `items` local is 27 words off. [wg]

### Constants, conditionals and fold
- **`if (!c) x = K; else x = v;` makes K its own pseudo**: the if-converter forces it into a register and loop.c hoists it. `c ? v : K` stores K directly (VITextList::Draw). [uitext]
- **`a ? 0 : (b == c)`** gives `beql` plus `xor/sltiu`; `a != -1 && b == c` gives `movn`. [uitext]
- **A float-compare select never if-converts**: in `mode = 3; if (alpha == 1.0f) mode = 0;` the default store goes first. [ws]
- **An int FLAGS parameter tested by bit keeps its `andi` in an s-reg** (`!(flags & 1)` becomes `xori/andi`); a bool parameter gives neither (DescribeInvItem). [uitext]
- **One variable assigned by if/else-if/else, with one return,** written `(a<<2) | 2 | (b<<4)`. A return per case swaps the last two blocks, and any other term order reassociates (VITextSet::MakeId). [uitext]
- **`m_z = y * 2` after `m_x = y` reloads y**; write `m_z = m_x * 2`, since cse knows the stored member. [wa]
- **`bool on = k.m_x04; return k.m_x05 || on;` keeps two byte loads plus `sltu`.** `k.m_x05 || k.m_x04` merges them into one `lhu`, the converse of phase 36's `ld` merge. [parts2]
- **A sign-extended byte built in compound statements** (`v = (int8)V[0]; v <<= 8; v |= V[1];`) keeps `lbu` plus `sll 24`; written as one expression it becomes `lb`. [tail]
- `s[0] = 'a' + (char)(t % 26)` masks the 64-bit remainder (`andi 0xff`) BEFORE the add. [wb]
- `char c = (char)*wide; (unsigned char)(c - 'A') < 26` loads with `lbu` and keeps `andi 0xff`. An unsigned char drops the andi; the two-compare form on a signed char loads with `lb`. [vic]
- **An enum field's range assert is two tests** (`bltz` + `slti`; recorded). g++ makes a non-negative enum unsigned, and fold_range_test never merges the bounds (SendPosition's `WalkPhysics.State`). [vic]
- **A key `operator<` uses the if/return form** `if (a < b || (a == b && ...)) return 1; return 0;`. With strcmp, `return strcmp() < 0` expands to `srl 31`, while if/return gives `slti v0,v0,0`. [char]
- **`x += 4 + strlen(s)` loads x before the call; `x = x + 4 + strlen(s)` calls first** (VIJournal::WriteString). [tail]
- **A local for a libcall product** (`long r = m_Blend * m / BLEND; return m_remote + dt + r;`) puts the member load after the calls (VISyncTime::GetTime). [tail]
- **fold reassociates constant arithmetic, so write the OTHER spelling** (maybe known). `W - 30 - cols[0]` gives the target's `W - (cols[0] + 30)` (DrawSell); `A + 155 + B + C` gives `(A + (B + 155)) + C`; `m_sel - (m_rows - 1)` and `(m_sel + 1) - m_rows` swap; also `(m_page + (m_pageCount - 1)) % m_pageCount`. A named `int w = width - 170;` keeps `addiu; subu`, and `float n = -dp;` keeps the negation. [wa, wc, wq, wd, wh, vic]
- **fold distributes into `?:` arms** (maybe known). `(c ? q1 : q2) * h` puts h in a SAVE_EXPR (DrawStanding). `VIMIN(LAST, x+1)` with LAST a `?:` gives per-arm constants. `x == (c ? 3 : 2)` fixed an allocation that `c ? x==3 : x==2` did not. [we, wl]
- **A constant reused from a register that a branch proved equal means the statement is written in BOTH arms.** cse follows the jump into each arm and jump2 merges the copies (Misc::OnMessage). [wc]
- A constant-folded division keeps its zero trap: `* 65 / 100` leaves both 319 and `beqzl; break 7`. [wq] (maybe known)

### Calls, inline helpers and the frame
- **A derived dtor deletes `this` itself only when its first base needs no destructor**; otherwise it passes `__in_chrg` to the base (build_delete). This is why VIClientI is a one-slot base with no dtor; ~VIClient is exact (440 words). [vic]
- **A foreign class `new`ed in a TU needs its ctor DECLARED.** Otherwise g++ synthesizes an inline one and inlines VIWnd's ctor plus the vptr store (VIWndFishing::Init). [we]
- **A call's delay slot can be filled from AFTER the call.** An insn touching only callee-saved registers is stolen from the fall-through: `font = &app->m_ui.m_font` written after `Begin2D()`. [wc] (maybe known)
- **A call in a `?:` RHS runs AFTER the lhs address is loaded** (`m_flag = Locked() > 0 ? 61 : -1`); a plain call RHS runs before it (expand_assignment's call-first case). [wg]
- **Object vs argument order.** In `m_info.M(buf, n, menu->GetId(...))` the object's address is computed BEFORE the inner calls. If the target computes it after, the call was its own statement (wh 9->0). An object loaded before its argument's call is a pointer local re-read for each call (`VISoundDevice *snd`). [we, wh] (maybe known)
- **A by-value VIVect2I temp whose y store goes through the temp's register means a VARIABLE argument.** The integrator folds the ctor's `this` only when no argument is a constant. `int x = 285, y = 37;` closed DrawRows and DrawValues. [wc]
- **Values computed BEFORE a word lookup are an inline helper's arguments** (`DrawWord(ui, font, id, x, y)`: DrawValues 343->6, Guild::OnDrawSelf 156->14). A fresh `addiu a0,app,4` as `this` is a file-static `Word(&app->m_ui, id)` (VIWndGame::OnMessage 715->0). [wc, wh, wg] (maybe known)
- **snprintf16 with GetString calls nested as its arguments** loads the format into a callee-saved register BEFORE the calls; string locals move that load to the call. VIWndGoto::OnDrawSelf 64->0; also DoAction. [wh, vic]
- **An inline of the CALLEE's class sets the store order inside its callers.** VIWndList's `{ m_p68 = 0; m_p6C = 0; }` stores +0x6C first. VIWndBuddy's own ResetSel was the only route to exact Init/Free. An inline after `__tf` is emitted `sw 48; jr; sw 44`. [we, wb, wh]
- **An inline method that names `this` once** (STATION's `ClearX24()`) avoids the reload that `m_edit->m_x24 = m_x28 = 0` causes. [wg]
- **Declare, then assign**: `VIVect2 p; p = pos - VIVect2(...);` places p's slot before the call temps, and the zeroing stores die (Draw 84->0). [parts1] (maybe known)
- **Two VIVect2I declared before either is set** (`VIVect2I pos, lift; pos.Set(110,20); lift = pos;`) keep one zero store and reload pos.y (ItemMenu::OnDrawSelf 34->0). [wh]
- **VIUIFrameParam fields as two plain named locals** declared after the first BlitRect are passed by address without a copy; an array element is copied (Merchant). [wm]
- **An `extern "C" pow` clobbers memory**, so a loop over it reads a LOCAL copy of the member (`int n = m_max;`, held in s0; Quantity::Open). [wm]
- **Dead code survives at -O2** (maybe known). An unused `char fmt[] = "..."` is still copied to the stack. An unused `char buf[512]; VIMemStream s(buf,512);` shows as eight stores plus a free() tail. Two dead `sw` of a VIVect2I constant are a discarded `VIVect2I(180, 90);`, or an empty inline taking a VIVect2I by value. [tail, wa, wh, ws]
- **Static menu tables are filled FIELD-major** (every icon, then icon2, number, id, sub); the -1/0 stores follow with the last store of each constant hoisted (ShowMemberMenu 46->0). [wh]
- **An 8-byte key copied field by field (`lw/sw` x2), where elsewhere it is `ldl/ldr`,** means the source assigns the two columns. [char]
- **`lb lb sb sb` on adjacent bytes is a BLKmode copy of a 2-byte struct** (movqi always loads `lbu`): `T p; p = T(x, y);` with a `T(const unsigned char &, const unsigned char &)` ctor (ServerSelect::Compare). [wq]
- **Two FLAT `if (p2 == this && ...)` blocks, not nested,** leave the second test un-threaded, so it reloads the member (ReadCS, ReadGram). [wq]
- **The Dinkumware sort's fingerprint** is a median of copies plus a 64-bit register round trip of the pivot (`_Median` takes and returns VALUES). `_Sort` is out of line only if `sort -> _Sort_0 -> _Sort` reaches it first. [char]
- **A flag set unconditionally at a block's end is dead inside the block**, so its register is free there (ChatLine's `handled = 1`; the frame lost a saved register). [vic]
- **A member reached through a block reference at the ctor's end** (`VIScene &scene = m_scene; scene.x = ...`) keeps `this+0x7590` live in s5: ctor 220->12. [vic] (maybe known)
- **Small facts.** `snprintf16`'s size is in BYTES. VIUIWindow is 0x180 B and 8-aligned. A base's inline getters hide a free function of the same name in a derived class (`::GetRace`). [we, wc]
- **Recorded in memory from cl-parts2 (1).** A loop pretest that compares the Init RESULT register means the result variable reaches a shared `return r` at the end. VICMNode::Read 98->0; a VOID Recv08 closed only via an inline `int ReadList()`. [parts2]
- **Recorded in memory from cl-parts2 (2).** An inline accessor in a CONDITION is not cosmetic: `j < m_items.Count()` made loop.c hoist `this+112` (TookItems 42->0), and in an `||` guard it moved dbr's fill (AddToCombine 3->0). [parts2]
- **Non-POD pool elements.** VIPool<VINetItem> needs DestroyPool/Erase that DESTROY the element and an Add that COPY-constructs it, through VIItem's copy ctor. This may be the template's real form; cl-pre changed vipool_t.h/viarray_t.h that way, and SUPPORT stays neutral. [parts2, pre]

### Known levers re-derived (already in memory before the lanes)
- **Order of stores, cases and copies:** phase-16 constant-store order (ctor zeros not hoisted, wb; ctor winners put the first field last, we); case bodies in source order; cross-jumping keeps the last copy (VIWndGame::OnMessage kept the FIRST); a store pair in reverse order.
- **Loops and addresses:** return vs break in search loops; the count-up `break` loop; shared vs per-loop counters (try both); the inline subscript as an address pseudo; `VIUIWindow &win` and inline `Width()`.
- **Frame:** a temp's slot reused by the next declaration; counting slots (named vs temporary VIVect2I); spill slots in declaration order; gcse PRE's hash-bucket slot order (wd re-proved it).
- **Conditionals:** VIMIN/VIMAX's triple compare; clamp a local; movn/movz follows the initialiser; two-sided ranges fold to `sltiu`; empty cases shape the tree.
- **Other:** `addiu` vs `ori` for `s >> b`; `while ((i = Begin()) != End())`; one full-expression for stream temps; nesting so the `return 0`s cross-jump.

## 3. Semantics recovered

### Protocol
- **0x0040** is `VIClient::SendPosition` 0x00BC2F30. It quantises against six VIBBox ranges (`gBoundsPos/Vel/Accel/Hpr/AVel/AAccel`, 0x00CD7A58.., built by static init 0x00CC6558) with a 24-bit quantizer at 0x00CC49C8, and asserts `0 <= WalkPhysics.State < 256`. [vic, tail]
- **0x00F0** is SendReport: "Text=" plus the text ring. [vic]
- **0x0700/0x0701** are the GM/debug channel. VIClientPart382E8 holds loss (u8), latency (u16), jitter (u16) and enabled. `ApplySimulation` feeds them to `connection_set_simulated`, a lag/loss simulator. Ids are 0xA1xx0000/0xA2xx0000, and `/create`, `/mastery` and `/tradelvl` go to it while it is enabled. [pre]
- **Auction and bank.** Requests are 0x1205: a sub-op byte, then zigzag ints. Sell sends index, price, buyout, duration. Replies are 0x1206, into VIBankControl::Recv1206. Bank money is 0x1255 (which, amount). [pre, wa, we]
- **Auction records.** VIAuctionQuery is 192 B. VIAuctionItem is a VIItem plus lot +0x2E4, qty +0x2E8, curHp +0x2EC, charges +0x2F0, seller +0x300. [pre, uitext]
- **Factions.** Request 0x0AC4 (page); the reply 0x07E2 lists {wide name[64], int value}. The window clamps standing to ±2000. [parts1, we]
- **Fishing.** 0x0F02 sub-ops 1..10 are Start, Stop, Ready, Caught(s), Waiting, Bite, Loot(s), Missed, Lost, Damaged. Client actions: Cast(quality), Cancel, Reel, Pull, Keep(slot, n). Window text lasts 5 s, and Tick acts in a 501..1500 window. [parts1, we]
- **Guild.** 0x0468 has sub-ops 1..16 (roster, MOTD, ranks, online). Commands go out as 0x0578 (Create, Disband, Quit, Promote, Demote, Kick, SetMotd, Say, Decline, Accept, Invite, SetChat). Invitations are 0x0579/0x057A, shown in a 45 s box. Can* checks return reason codes. A guild costs **5000** (`gGuildCost`, 0x00CD7A50). [parts1, wh]
- **Gram (mail).** Replies are 0x0D41 (inbox, message, send, delete). Requests go in an envelope: the opcode plus a sub-op byte around an inner stream. There are 100 headers. [parts2]
- **Gram records.** VIGramHeader is 176 B {id +4, from[24] +0xC, subject[64] +0x24, VIDate +0xA4, read +0xAC}; VIGramMessage is 1,200 B, adding text[512]. The original's message READER calls WriteString, a shipped bug that was kept. [parts2, char]
- **Class mastery.** 0x1401 requests and 0x1402 replies, sub-ops 01..09. Select(percent) is sub-op 1, clamped 0..100. Send02(id) buys; Send03(id) acts on an owned skill; sub-op 9 opens a node. VICMSkill is 152 B; VICMNode is 852 B (minimum STR..CHA at +0x1C..+0x34, points spent at +0x18). [parts2, wd]
- **Ignore list:** opcodes 0x1000..0x1006 (Send1001(int)); rows are {char name[32]; int id}. [parts2]
- **Crafting.** Requests are 0x1101: a sub-op byte, the server's token (+0x78C0), then args. Replies are 0x1102. Six slots, a VINetItem combine pool, and 144-byte recipe rows {id +8, level +0xC, difficulty +0xF (0..3), name[64] +0x10}. [parts2, wq]
- **Who list.** Reply 0x0E02 echoes the request's serial, and a reply with another serial is DROPPED. Rows are 32 B {race, class, level bytes, name[24] +3, id +0x1C}, the same layout as game.h's VIInviteRec. [parts2, ws]
- **0x0B01** is VIClientPart2EFC8::Send(int). **0x0C01** is VIClientPart2EFCC::SendText(wide) and SendFlag(int); I checked the image, and both functions load 3073. [parts1, tail]
- **0x000D** is Msg000D (VIClient.cpp:9800-9821) plus VIPlayer::Read 0x00CBA850 for the player block. **0x001D** refreshes the trainer. Six of the seven new VIClient senders, and VIMaskIndex, have no caller. [vic, tail, ws, parts1, parts2]

### Server tables (VICharacter.cpp, compiled in, never called) [char]
- **Key classes and their key columns** (each key also includes CHARACTER_ID, except User's): User; CharItem (INVENTORY_SLOT_ID); CharBankItem (BANK_SLOT_ID); CharQuestToken (TOKEN_SLOT_ID); CharQuestHistory (QUEST_HISTORY_TOKEN_ID); CharFaction (FACTION_ID); CharAbility (ABILITY_ID); CharAttribute (ATTRIBUTE_ID); CharInfluence (SORT); CharChatMenu (MENU_ID); CharChatMenuItem (MENU_ID, POSITION); CharAbilityEffect (SORT); CharIgnore (SEQUENCER); CharAuction and CharBid (AUCTION_ID); CharSkill (SKILL_TYPE); CharRecipe (RECIPE_ID); CharMastery (MASTERY_ID).
- **Row-only tables:** BuddyLink, GuildMember, Guild, Cluster, RemoteCharacter, AuctionListing. Table names start at 0x00CF1C40 ("EQA.USERS", ...).
- **VICharacter** (296 B) is the CHARACTERS row plus 16 row arrays; `Diff(New, Old, s)` writes 17 key-diffs, which is the save delta. Row `==` skips some columns (CHARACTER_ITEMS +0x20, CHAR_ABILITY_EFFECTS +0x18). CHARACTER_INFLUENCES +0x18 is compared but never serialised.

### Client behaviour
- **Window messages to the parent:** 256 list Enter; 258 Menu4 command; 260 back/cancel; 261 menu entry (msg, id); 262..267 char-create pages (race/sex/city, class, misc, stats, name, create); 268 box accepted; 273 spell slotted; 277 swap items; 279 private-chat Enter; 280/281 sell/buy; 288 panel shown (1) or back to menu (0); 289/290/310 bank items/money/auction; 291/292 money menu; 293 quantity (value, id); 294 amount; 295 focus; 296 list moved (old, new); 297 inspect back; 305 guild member info; 4352..4358 server select. [wa, wb, wh, wl, wm, ws, we, wq]
- **Character creation.** The pages go race -> class -> name -> misc -> review, over a VINewChar form (88 B, city at +0x38). GetRaceInfo(race, expansion) has 36-byte rows (per-sex default face and hair) plus expansion tables gRaceInfoEx/gRaceCamerasEx. There is a home-city table per race at 0x00CE4270. VIRaceCamera +0x04 and +0x14 are the review and appearance cameras. Names go through CapitalizeName (strupr, then strlwr from the second letter). [wb, wc, parts2, char]
- **VIClientPart06D00 is the character-resource manager**: SetModel/Item/Face/Hair/Robe/ArmorSlot, and Update(camera), which ranks, loads and releases VICharRes. GetSwing indexes SwingEffect, SwingDelay and SwingDelay2. [tail]
- **VICreatureEntity** has ten virtuals (slot 10 is WriteIdentity), item/armor/hair/face/robe fields, and level/race/class at +0x70/+0x258/+0x259. GetType is an inline returning 129 for VIEntity and 130 for VICreatureEntity. `gEntityUpdateRate` is 100. VIAIExtrapolateFilter extrapolates NPCs along the ground, and VISyncTime blends the remote and local clocks. [tail, wg]
- **ZoneName(world, row, col)** returns the WORDS.TXT id of a grid square (641 outside the grid). VIWndGoto is a world/row/column picker clamped to gWorldInfo, and MainMenu::UpdateLocation displays the name. [tail, wh, wl]
- **VIKeyFlash** auto-repeats a held pad key: it fires at once, then after 15 frames, then after 8, 7, ..., 1. [uitext]
- **Quick chat** is a VITextSet tree (a 256-entry table) shown by VIWndMenu4 (directions 0 up, 1 left, 2 right, 3 down). `%W<n>` expands to a WORDS.TXT entry, and `|<n>` sends command message 258. Menu4Edit edits the tree (34 command words); DefaultTextSets builds the default one. [wl, uitext, vic]
- **Ids.** Item-menu entries: 0x02020000 inspect, ..030000 use, ..040000 none, ..050000 pick up to move, ..060000 use a count, ..070000 quick-slot. Ignore-list Remove is 0x0B053004. Auction search modes are 0xA8010000..0xA8040000, and its states 0..8 are form, results, lot, rankings, bid, races, classes, wear slots, attack types. [wh, wa]
- **Trade** focus values: -1 closed, 0 inventory, 1 my offer, 2 my money, 3 their offer. The prompt is "<name> wants to trade" (Yes/No), with trade chat. Money goes through VIWndDeposit (modes 1 deposit, 2 withdraw, 3 trade; 9 digits). The bank deposit limit is VIPlayer +0x34. [ws, we, wa]
- **Trainer** modes: 0 stats, 1/2 buttons, 3 derived, 4 trained. It shows 7 trainable stats and 9 derived ones, each {stat, word, desc}. The spell book has tabs and 10 slots. [ws]
- **Server select.** Shard rows are 88 B {wide name[32], long id +0x48, region +0x50}, filtered by region and sorted with qsort. It checks the memory-card session (McErr, VIIsMcSameSession). [parts2, wq]
- **Small windows and parts.** The Response window is an NPC dialog with up to 6 options (130 B each). NetStat::SetData(age, sent, recv, number, ping) is fed by drdp_get_bps_sent/recv. Damage numbers are floating glyphs drawn each frame. VIAlarm is a game-time alarm set from chat. [wq, wm, vic, parts1]
- **Quirks.** ReplaceChar skips the first character. CLIENT's VIWndOptions calls `app->ApplyOptions()`. The CS windows re-fetch the CS client through VIClient's virtual slot 8 every time, where STATION caches it. [tail, wm, wd]

## 4. Corrections to older documentation
- **Memory `project_decomp_phase62_client.md`:** "VIClientI declares all 18 slots" is WRONG. VIClientI is ONE slot (GetConn), as in STATION; ~VIClient and the 16 getters are VIClient's own virtuals, slots 2..18. [vic]
- **Same file:** "VIRec792_BBC168 = VINetItem+28" is WRONG. VIAuctionItem derives from VIItem, and its tail differs. [pre, uitext, wa]
- **Same file:** the "Journal template residue, one cause (ADDRESSOF)" diagnosis is wrong. The cause was VIJournal::Write/Read returning int; made void, JournalTick, JournalNetwork, JournalConsole and Init match, and Tick goes 273->242. [tail]
- **game.h and map:** VIPlayer does not derive from VIPlayerRecord, and OpenResources is LoadModels (Msg000D asserts `LoadModels() >= 0`) [vic]. VIClientPart382E8::Show's id is signed (`...Uii` -> `...ii`), and VIClientText::SetClient returns int [pre].
- **game.h and map, cl-parts2:** 0x00BF7F80 is GetBonus(int), not "Count". VIClientPart38300::Recv's opcode is unsigned short. GetP44Total takes an int. cl-wg's `void OpenRecipe()` takes an int. VIChatText::AddText(wide) returns int. [parts2]
- **game.h and map, cl-tail:** VIClientPart06D00's SetTint/SetTint2/SetPart are SetHair/SetRobe/SetArmorSlot. GetSwing returns int, and VISwing is gone. VIRandom is VIRandomMT (Next -> Random, Range -> RandomRange), and "VIJournalFile" is VIFileStream. CreateEntity's type is unsigned char. [tail]
- **game.h, VIClient:** `char m_p4F098[24]` is VIAlarm, and VIClientPart4FD28::AddDamage returns int [parts1]. VIWndCharCreateBlank's "ShowList" is Reset; VIWndCharacterSelect's m_p428 is m_sel, ShowCharacter takes 14 parameters, and Connecting returns int [wb].
- **Window signatures:** VIWndList's M_00C69508 takes ONE int (the a2 = 1 was a store's constant); its Init returns int and SetItem's flags is `int`. VIInfoText 0x00C05CC8 returns int. 0x00C04F48 is VIFont::DrawCenteredString. NetStat::SetData's third parameter is unsigned long. The VIMiniMenuDef and `const void *` placeholders are VIMenuDef. [we, wl, wm]
- **More signatures:** VIWndGame::AddText/AddWord take VIColor32F BY VALUE (`G10VIColor32F`), and PromptWord, Prompt and DrawGauge are fixed [wg]. VIClientGuild::GetMemberOnline returns int, not bool; measured, not applied [wh]. VIKeyFlash is an auto-repeat tracker, and its slot field m_p1 is m_wait [uitext].
- **Wrong placeholder types:** VIPool<VIPtr_BF45A8 *> was the wrong SIZE, which broke its Next/Allocate. Each resolves as follows: [parts1, parts2, wd, tail, uitext, char, wg]
  - VIPool<VIPtr_BF45A8 *> is VIPool<VIIconSlot>, and VIMap<unsigned, VIRec4_BF41A8> is VIMap<unsigned, int>.
  - VIPool<VIPtr_BF9718 *> is VIPool<VICMSkill>; <VIPtr_C00F58 *> is VIPool<VINetItem>; <VIPtr_C01508 *> is VIPool<VIRec144_C017C0>.
  - VIList<VIPtr_C35C38/C35E00 *> is VIList<VICMSkill *>.
  - VIPool<VIRec524_CC0F38> is VIPool<VICharRes>, and VIMap<int, VIRec8_CC0C88> is VIMultiMap<VICharResKey, int>.
  - cl-we's VIRec764_C00F98 is VINetItem; VIElem_CBB288/340 are VIItemText/VIItemMod.
  - VIRec36_C0CDC0 is VITextLine, and VIRec1_C0C2D0_A1 is char.
  - 51 VICharacter blocks are VIDb* rows, and 0x00BBBC40 is `VIArray<int>::VIArray()`.
- **vi.h and shared headers:**
  - VIUI::End2D() and VIUIWindow::TickR() are VOID [wq, ws, wh], and VIString::strupr is VOID [char].
  - VIWalkPhysics +0x58 (`int m_locoState`) is an enum `State`; not applied [vic].
  - vipool_t.h/viarray_t.h assumed POD elements, and vimemstream.h's `operator<<(VIMemStreamInt)` is probably const-ref [pre].
  - STATION's VIWndEdit needs `GetEnd() { return m_pos + m_size; }`. STATION's MBox is 0x5D8 B, CLIENT's 0x5E0 [wc, wl].
- **Plans and other lanes' readings:** LANES.md's split was off at most window boundaries (section 5). cl-tail read 0x00CB60C8/6358 as SGI-STL `__quick_sort_loop`, but they are Dinkumware `_Sort` [char]. VIWndTell::OnMessage matched with `case 294:` where the jump table says 295 [ws]. Before the lanes, the coordinator fixed game.h's window names: "VIWndLogin" was VIWndCharacterSelect, "VIWnd2BB7C" was VIWndLogin and "VIWnd2BB88" was VIWndCharCreateBlank.

## 5. Structure of the original source tree
- **Root** `C:/code-frontiers-gm/game/`. Assert-attested files:
  - VIClient.cpp (Msg000D 9800-9821; after the last assert, viclient.cpp switches to `#line 1 "VIClient-tail.cpp"`, our name).
  - VIBankControl.cpp:593, VIShardInfo.cpp:166, VIStringTable.cpp:245.
  - VICharacter.cpp:2309/2310/2315, VIKeyDiff.h:118.
  - VICreatureEntity.cpp:364, VIEntity.cpp:298/681, VIFileStream.cpp:57/196/219, VINetItem.cpp:58.
  - VIWndGame.cpp:4007, VIWndGuild.cpp:447/477/503.
  - VIItemList.h:315/326, VIMemStreamArray.h:71, VIMemStreamVector.h:71.
- **Our names, not the original's:** viadmin, unknown_bbdc80, viclienttext, the nine cl-uitext TUs, vinetcharacter, vishardentry, vicommandsender, viaiextrapolate, viswing, viclientutil, viexperience, viargs.
- **Window TU shape** (all window lanes): first the methods in address order; then the TU's own template copies (no COMDAT) and synthesized members; then `__tf<Class>`; then the class's inlines, emitted out of line; finally `_GLOBAL_$I$/$D$` and `__static_initialization_and_destruction_0` if the TU has statics. An out-of-line inline belongs to the TU of its class's vtable.
- **TU edges:** in cl-uitext's range each .rodata ends in one `bad_alloc` string [uitext]. cl-parts1's range is nine TUs, each closed by its template-copy run [parts1].
- **Tails that crossed lane lines:**
  - 0x00C338F8..0x00C33908: VIWndChat (a white static at 0x00CF5458).
  - 0x00C41A48/50: VIWndCSRequest.
  - 0x00C4E568..0x00C4E620: VIWndFishing (static `sTargetEmitter` at 0x00CD75B0).
  - 0x00C58B78..0x00C58BB8: VIWndGame (5-colour table at 0x00CF5AB0).
  - 0x00C71960..90: VIWndMBox. 0x00C9EA18..38: VIWndTradeList.
  - 0x00CB6020/60C8/6358: VICharacter.cpp.
  - 0x00C0D000: the end of the VITextList TU. 0x00BE3C10: VIAlarm's ctor. 0x00C77748: a Menu4Edit inline.
- **TU-local data:**
  - gGuildCost lives in viclientpart2ef24.cpp, and gEntityUpdateRate in viclientutil.cpp.
  - 0x00CC6558 initialises six VIBBox globals.
  - Class-icon tables sit at 0x00CEB978 and 0x00CEFFB8; sCategoryWords is at 0x00CD7528 {1653, 1654, 1655, 511, 1656}.
  - Each {word, id} lookup is its own TU with its own table; `i < sizeof(t)/sizeof(t[0])` gives `sltiu`.

  [parts1, tail, wl, ws, wd, uitext]
- **VIItemList.h carries code** (Get/Read/Took/GetSlotId). The live copies are in VIBankControl.cpp (0x00BBC8D0, 0x00BBD398, 0x00BBD430). Every TU that uses them keeps a dead copy: VIClient.cpp 0x00BDF938/0x00BE1ED8/0x00BDFFE0/0x00BE0048, fishing 0x00BED350..0x00BED810, crafting 0x00C01C88. It also has an inline `VIItemList() { Clear(); }`. [vic, pre, parts1, parts2]
- **Other headers with code:**
  - VIKeyDiff.h is used only by VICharacter.cpp.
  - VIMemStreamArray.h has 17 instantiations in VICharacter.cpp, plus 0x00BBD9E8/DA90, 0x00BEB738, 0x00BFC7E8 and 0x00CBB590, all through one shared `vimemstreamarray.h`.
  - VIMemStreamVector.h's reader 0x00C03770 is in the who-list TU.
  - visort.h was a Dinkumware VC++ 5/6 sort transcription, used only by VICharacter.cpp. The polish pass rewrote it in our own words with the same template shape (vicharacter.cpp stays 560/560).

  [char, pre, parts1, parts2, tail]
- **VIClient::ExpandText** (0x00BE3AC0) is an inline member with no caller, so MercMenu::Refresh only matches when it calls it inline. viclientaccess.cpp's 25 accessors are probably the same family. [wm]
- **Shared with STATION:**
  - viwndmessage.cpp and viwndoptionlist.cpp are verbatim, so they can be symlinks.
  - viwndedit.cpp differs only in OnMessage/OnDrawSelf (`app->m_p07568` vs `VITimer::GetTime()`), so one #ifdef covers it.
  - The CS windows are STATION's source plus `(VIClient *)GetAppPtr()`, ClosePanel700/6FC, ShowMsg(864) and a dimming blit.
  - VIWndMBox is STATION's class plus an icon, and virandom.cpp is STATION's file.

  [wm, we, wd, wl, tail]
- **Shared with SETUP and SUPPORT:** viwndoptions.cpp is SETUP's file plus three changes. vistringtable.cpp is SUPPORT's byte for byte, so it can be a symlink. vifont.cpp is CLIENT's own: SUPPORT's text plus GetSexWord and its own bad_alloc. [wm, uitext]
- **Class layout:**
  - VIClient (vt 0x00CDB4C0, 19 entries) sits over a one-slot VIClientI.
  - VIEntity's vtable is 0x00CF3DD8: 1 dtor, 2 GetType, 3 IsType, 4 WriteUpdate, 5 ReadUpdate, 6 Write, 7 Read, 8 IsSelectable, 9 Refresh. VICreatureEntity's is 0x00CF3600 (11 entries).
  - Menu, MiniMenu and TradeList derive from VIWndList; CharCreateRace derives from CharCreateBlank. VIWndDisconnect is embedded in VIClient.
- **vi.h additions (all additive; SUPPORT stays 3,665):**
  - explicit `VIBBox(const VIVect3 &)`;
  - VIArray `operator()` and `Size()`;
  - `VIVect3::operator[]`;
  - VIList `Prev()` and `Value()`;
  - `VIVect2I::operator-`, which now exists in three copies (vi.h, cl_wm.h, viwndmenu4edit.cpp).

  vimemstream.h gained WriteBool and WriteF32.

## 6. Opens left by the lanes (the polish pass now owns 144 of the 153)
Each figure is wrong/total words.
- **Cross-cutting, shared with STATION** (one fix closes both modules): VIWndEdit (OnDrawSelf 836/1450, WrapLines 32/181, OnKeyUp 6/20); CSRequest (OnDrawSelf 132/944, OnMessage 70/190); CSInbox::OnDrawSelf 99/704; ReadCS (OnDrawSelf 151/578, Wrap 8/143); OptionList::OnDrawSelf 5.
- **Cross-cutting, GetString -> snprintf16 argument order:** Login::OnDrawSelf 6, OptionList 5, Msg00D3 9, DoAction 25.
- **Cross-cutting, the VIMIN/VIMAX clamp's registers:** ExtrapolateGround 22/408, Walk 23/95, QuantizeVect 28/46.
- **cl-vic, newly written:**
  - ctor 5/474: four last-block stores scheduled apart.
  - PlayLoop 21/63: the target duplicates hash+Find per branch.
  - Tick 242/369: gcse LCM hoists sp+16 across an `||` assert.
  - InitBody 38/422 and LoadWorld 285/404: allocation and spill slots.
  - Process 303/649: `this` in s6 vs s5.
  - SendReport 27/177 and StatWithItem 30/342: register pairs.
  - ChatLine 327/419: the text and the handled flag swap.
  - UpdateGame 291/743: frame 320 vs 304.
  - SendPosition 355/749: the WalkPhysics copy.
  - TrackTarget 214/688: spill order.
  - UpdateControls 281/1035: FP registers, and a DEAD `c.lt.s` the target keeps.
  - DoAction 25/1143.
  - ConnectShard/Connect/Msg076F 8/12/5: temp slots.
- **cl-vic, inherited opens tried:**
  - GetAbilityList 1 and GetAbilityRecords 6: `addu` operand order.
  - CanUse 2: movn vs movz.
  - Msg0040 25: a PRE copy.
  - Msg001A 11.
  - VIAbility::Read 12: both returns go via v1.
  - Msg00B4 84: the target's second loop gets no loop.c treatment.
- **cl-vic, not attempted:** Channel40 5, WriteQuantized 6, ChannelEntity 7, LeaderByte263 7, DrawNetStats 10, GetInvItemSellPrice 13, IsAbilitySlotSpell 16, ProcessMessage 17, CreateCharacter 20, VIMap Node copy 37, DrawPosition 38, BeginManualCamera 124, EnterGameLoad 187, CycleTarget 241.
- **cl-pre:** Move 13/88: the post-call copy comes late. RecvResults 46/182: the target strength-reduces the compare loop; about 10 spellings failed. ParseToken 11/44: the target re-zero-extends the HImode char once.
- **cl-parts1:** 2EF24::InviteDeclined 7/60 is a `this`/&ui global-alloc tie. 4FD28::Process 65/325 comes down to one LCM decision: the target's PRE inserts `R = sp+16` at the end of the ctor block, while ours is hoisted by loop.c. The VIItemList.h dead copies can't be scored; let eqcheck score `<tu>:<name>` rows.
- **cl-parts2:** VIChatText::AddText(wide) 73/173: the target keeps `this+16` in s2 and adds it LAST. 0x00C01C88 (a dead Took copy) is unwritten.
- **cl-uitext:** VITextList::AddText 12/124 is a global-alloc tie: `i` (22/154) and the byte count (6/21) tie at 4/7, and `i` wins on regno. VITextList::Draw 28/181 loads m_size before m_top (gcse's reaching register).
- **cl-tail:** VIJournal::Eof 5/17 (v0 vs a1); VIItem::SetFrom 13/195 (constant registers); GetSwing 49/114 (cross-jumped case tails; the eqrefcheck finding); VIMultiMap Insert 43/208; SetVisible 14/62 (a shift); Create 4/57; Sync 18/147 (store order); ApplyLook 18/79 (a slot reload); ApplyItems 12/63; VIPingRing::Add 8/16 (an `andi 0xffff`).
- **cl-wa:** BankItem::OnDrawSelf 3/341: the order of three stores into an unused second VIUIFrameParam.
- **cl-wb:**
  - CharCreateBlank::Init 53/139: s0/s1, because `r` outranks `this`.
  - Five OnDrawSelf: Class 839/1042 and CharacterSelect 654/999 are drafts; Blacksmith 188/435, BuffPC 247/415, Buddy 443/617. Buddy's frame is right; the target reaches the window and font through the MEMBER.
  - ShowStats 18/156: giv allocation.
  - CharacterSelect::OnMessage 32/153.
  - ShowCharacter 130/280: the 13.0 switch cross-jumps only in ours.
- **cl-wc:** Misc::OnDrawSelf 4/1068: the white is passed through the colour's `this`. Race::OnDrawSelf 277/817: a constant 48 gets CSE'd, plus the sex-icon layout. Review::OnDrawSelf 27/755: temp rotation. Chat::SetMode 14 and Chat::OnDrawSelf 17/325 are allocation ties.
- **cl-wd:**
  - Require::ShowPage 2/288: sched1 takes the lo_sum first because it KILLS the high-part pseudo.
  - ClassMastery::SelectCategory: PRE's hash buckets (S = 171); eight empty asm statements flip them.
  - ClassMastery::OnDrawSelf 211/526: high(mUI) in s8.
  - ComposeGram::OnDrawSelf 26/559: only the dots loops (cl-wh's lever).
  - ComposeGram::OnMessage 20/154: a p1/p2/w tie.
- **cl-we:** Disconnect::OnDrawSelf 9/106. Experiment::OnDrawSelf 138/368 and Faction::OnDrawSelf 298/529: the target spills `this`. DrawStanding 27/141: fold distributes into the `?:`. Down 12/40. FillBait 2/124: a PRE-hoisted n+1. Fishing::Tick 7/104: float constant registers.
- **cl-wg:** OnDrawSelf 66/389 (the XP-bar loop keeps its loop notes); DrawCompass 106/317 (which pointers spill); DrawPet 51/209 (sltiu vs movz); DrawTargetInfo 35/228 (s1/s2); DrawTarget 12/239; OpenAuctionSearch 11/49 (an empty `beq` slot); Attack 10/40 (movz off a shared `li 1`); DrawGroup 8/265 (the constant 25).
- **cl-wh:** Guild::OnDrawSelf 14: the Text call moves &black first, GetMemberOnline should be int, and End2D void. HUDMenu::DrawStack 9: the load order into BlitEx's temp. InitFromMc::OnDrawSelf 4: End2D void, applied later.
- **cl-wl** (mostly undiagnosed):
  - VIWndList::OnDrawSelf 198/342.
  - MBox: OnDrawSelf 64/589, DrawIcon 22/144.
  - Menu::OnDrawSelf 6/82.
  - Login::OnDrawSelf 6/147.
  - Loot::Init 31/70: placement of the -1 block.
  - MainMenu: UpdateLocation 63/177, OnDrawSelf 131/433, Wrap 143/155, DrawStats 196/373.
  - Menu4: Expand 2/89, ParseEscape 48/87, OnDrawSelf 107/436, DrawChoice 118/165, OnKeyUp 165/291, OnMessage 309/373.
  - Menu4Edit: DrawCommands 14/168, DrawShadowText 25/70, DrawText 2/37.
- **cl-wm:**
  - Merchant::Init 60: returns through s2.
  - MercMenu::Refresh 6: needs ExpandText inline, which costs ExpandText's out-of-line copy (cl-vic's call).
  - MercMenu::OnDrawSelf 31.
  - NetStat UpdateTexture 108 and CreateTexture 45: our cse reuses the -1 register for the 255s.
  - NetStat::SetData 14.
  - MiniMenu DrawPlain 135 and DrawTitled 159: font vs &m_size in s8.
  - Merchant::UpdateInfo 2: two givs swap.
- **cl-wq:** Recipe::OnDrawSelf 466/811, a first cut (frame 752 vs 704). ReadCS's two opens are STATION's, listed above.
- **cl-ws:**
  - Trainer: DrawStats 202 (we hoist &ui->m_font), DrawItems 95 (the target holds ui twice), DrawDetails 38, Adjust 10, BaseStat and CurStat 5 (return through a0), ItemUp 3.
  - SpellBook: OnDrawSelf 494/668 (39 held in s5 from the prologue), AddRecords 23, Up 3 (slti/movn).
  - TradeList::OnDrawSelf 184.
- **Merge level:**
  - Nine functions are unwritten (1,340 B): five dead VIItemList.h copies, `_$_t5VIMap2ZiZ9VIAbility`, 0x00C0D000 and 0x00C77748.
  - Cleanups:
    - The VICraftDetail -> VIRecipe cast in viinfotext.cpp.
    - VIPlayer's float[3] casts.
    - wq's VIMiniMenuDef cast.
    - Three copies of VIVect2I `operator-`.
    - ws's static `Size` vs vi.h's.
    - Symlink the shareable files.
