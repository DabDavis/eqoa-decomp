// cl_wg.h -- windows: VIWndGame.
// CLIENT 0x00C4E5F8..0x00C58B78.
// TUs (one window class each, ending with its __tf):
//   VIWndGame                  0x00C4E5F8..0x00C58B78
#ifndef CL_WG_H
#define CL_WG_H

// The HUD / start-menu records VIWndGame fills (0x00CD02A0.. 0x00CD0310, their
// per-slot sub-menus and item tables).  They are another lane's data (the TU whose static
// initialiser 0x00C0B240 builds 0x00CD02A0 and 0x00CD02F0): provisional types and names,
// the owner's win at merge.
extern VIMenuDef     gHudMenu0, gHudMenu1, gHudMenu2;   // 0x00CD02A0 is cl-uitext's gStartMenu
extern VIMenuDef     gInvMenu, gItemMenu3, gItemMenu4;   // 0x00CD02F0 is gEquipMenu
extern VIMenuEntry gInvSubItems[][3];       // the inventory menu's per-slot sub-menu items
extern VIMenuDef     gInvSubMenus[];          // ... and the sub-menus
extern VIMenuEntry gEquipSubItems[][3];       // the equipment menu's
extern VIMenuDef     gEquipSubMenus[];
extern VIMenuEntry gItemMenu3SubItems[][5];       // 0x00CD0300's
extern VIMenuDef     gItemMenu3SubMenus[];
extern VIMenuEntry gItemMenu4SubItems[][5];       // 0x00CD0310's
extern VIMenuDef     gItemMenu4SubMenus[];
// The start menu's four item tables and counts OpenStartMenu picks between (the bank's
// flag x the expansion mask's bit 1).
extern VIMenuEntry gStartItemsX[], gStartItemsBankX[], gStartItems[];
extern int           gStartCountX, gStartCountBankX, gStartCount, gStartCountBank;
extern int           gGuildCost;            // OnMessage: the guild's price, 5000
void Command(char *obj, VIClient *app, const unsigned short *text);  // VIClient.cpp: on VIClient+0x4F098

// The game window, VIClient+0x2BB78 (`new`, 1824 bytes, in InitBody).  VIWndGame.cpp: its
// methods run 0x00C4E640 .. 0x00C57D40, then the TU's template copies, its static
// initialiser (0x00C58A80: the five colours at 0x00CF5AB0), __tf9VIWndGame (0x00C58B28), the
// class's five inline members (0x00C58B78 .. 0x00C58BB0) and _GLOBAL_$I$ (0x00C58BB8).  One
// assert, VIWndGame.cpp:4007, in OnMessage.  It owns every in-game panel (+0x680 ..
// +0x71C, `new`ed by Init in the order Init lists them, deleted by Free) and switches
// between them through m_mode (UpdateMode's switch).  Method names are provisional;
// the ones VIClient calls kept the names game.h gave them.
class VIWndGame : public VIWnd
{
public:
    VIWndGame();                                                // 0x00C4E640
    virtual ~VIWndGame();                                       // 0x00C4E760: Free()
    int  Init(VIScene *scene);                                  // 0x00C4E7D8: news every panel
    int  Free();                                                // 0x00C4F648: deletes them
    int  WorldUnloaded();                                       // 0x00C4FF00, name provisional (UnloadWorld): releases m_mat0/m_mat1
    void SetMode(int keepHud);                                  // 0x00C4FF80, name provisional: hides every panel, back to mode 0 (13 stays: the start menu)
    void ChatRefresh();                                         // 0x00C50200, name provisional (Msg000D)
    void UpdateMode();                                          // 0x00C50220: shows and activates m_mode's panel
    virtual void OnDrawSelf();                                  // 0x00C50B58
    void UpdateGlow();                                          // 0x00C51170
    void DrawGroup(VIVect2I at);                                // 0x00C512C8
    void DrawPet(VIVect2I at);                                  // 0x00C516F0
    void DrawStatus(VIVect2I at);                               // 0x00C51A38
    void DrawTarget(VIVect2I at);                               // 0x00C51B50
    void DrawTargetInfo();                                      // 0x00C51F10
    void UpdateQuickBars();                                     // 0x00C522A0 (VIClient: per frame)
    void BuildInvMenu();                                        // 0x00C522F8
    int  SetInvMenuItem(int inv, int slot);                     // 0x00C52498
    void BuildEquipMenu();                                      // 0x00C52748
    int  SetEquipMenuItem(int inv, int slot);                   // 0x00C528E8
    void BuildMenu0300();                                       // 0x00C52B28
    int  SetMenu0300Item(int inv, int slot);                    // 0x00C52CC8
    void BuildMenu0310();                                       // 0x00C53080
    int  SetMenu0310Item(int inv, int slot);                    // 0x00C53220
    void BuildAbilityMenu0();                                   // 0x00C534F8
    int  SetAbilityMenu0Item(int ability, int slot);            // 0x00C53718
    void BuildAbilityMenu1();                                   // 0x00C53888
    int  SetAbilityMenu1Item(int ability, int slot);            // 0x00C53A48
    void BuildQuickMenu();                                      // 0x00C53BB8
    int  SetQuickMenuItem(int inv, int slot);                   // 0x00C53CB8
    virtual void OnKeyDown(int key);                            // 0x00C53DF8
    virtual void OnKeyUp(int key);                              // 0x00C54250
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);  // 0x00C543F8: VIWndGame.cpp:4007
    void LootOpen();                                            // 0x00C556D8, name provisional
    void LootClose();                                           // 0x00C55710, name provisional
    void MerchantOpen();                                        // 0x00C55740, name provisional
    void MerchantClose();                                       // 0x00C557A0, name provisional
    void ShowDialogBox(VIDialogBox *d);                         // 0x00C557C8, name provisional (Msg0034, loot window up)
    void OpenDialog(const unsigned short *text, int b);         // 0x00C55820, name provisional (ShowDialog)
    void OpenBank();                                           // 0x00C55888, name provisional
    void CloseBank();                                       // 0x00C558F0: mode 0, hides +0x6AC
    void OpenTrainer();                                         // 0x00C55918
    void CloseTrainer();                                       // 0x00C55950: mode 0 and a call on +0x688 first
    void Msg001D();                                             // 0x00C55990, name provisional
    void OpenClassMastery();                                         // 0x00C559B0: also a call on its +0x6C0 first
    void CloseClassMastery();                                       // 0x00C559F8
    void OpenStartMenu(int a);                                  // 0x00C55A20
    void CloseStartMenu(int mode);                              // 0x00C55BC8
    void PromptWord(int id, int type, int p1, int p2, int show);  // 0x00C55C30: the message box with word `id`
    void Prompt(const char *text, int type, int p1, int p2);    // 0x00C55CE8
    void Prompt(const unsigned short *text, int a, int b, int c);  // 0x00C55D88, name provisional: a mode-14 prompt box
    void Trade02();                                             // 0x00C55E28, name provisional: mode 15
    void Trade16();                                             // 0x00C55E60, name provisional: closes the trade (mode 15/16)
    void TradeOffer(unsigned int id);                           // 0x00C55EC0, name provisional (Msg0057): mode 16
    void TradeClose();                                          // 0x00C55EF8
    void Trade07();                                             // 0x00C55F38, name provisional
    void Trade06();                                             // 0x00C55F58, name provisional
    void Msg0061();                                             // 0x00C55F78, name provisional: the blacksmith, mode 19
    void Msg0063();                                             // 0x00C55FB8, name provisional: closes the blacksmith
    void Msg0060();                                             // 0x00C55FF8, name provisional
    void ShowBuddyRequest();                                    // 0x00C56018, name provisional (Msg076F): mode 17
    void HideBuddyRequest();                                    // 0x00C56050, name provisional (AnswerBuddyRequest)
    void OpenBuddyList();                                         // 0x00C56078: the buddy list, mode 18
    void CloseBuddyList();                                       // 0x00C560B0: the buddy list
    void OpenGuild();                                           // 0x00C560F0: mode 20
    void CloseGuild();                                          // 0x00C56128
    void OpenGuildMenu();                                       // 0x00C56150: mode 21
    void CloseGuildMenu();                                      // 0x00C56188
    void OpenQuestLog();                                         // 0x00C561C8: the quest log, mode 22
    void CloseQuestLog();                                       // 0x00C56200
    void OpenGramInbox();                                         // 0x00C56240: the gram inbox, mode 24
    void CloseGramInbox();                                       // 0x00C56278
    void OpenComposeGram();                                         // 0x00C562B8: compose a gram, mode 23
    void CloseComposeGram();                                       // 0x00C562F0
    void OpenIgnoreList();                                      // 0x00C56330: mode 26
    void CloseIgnoreList();                                     // 0x00C56368
    void ShowInvite(const char *name);                          // 0x00C563A8, name provisional (Msg0773)
    void OpenBuffs();                                         // 0x00C56418: the buffs, mode 27
    void CloseBuffs();                                       // 0x00C56450
    void OpenOptions();                                         // 0x00C56478: the options, mode 34
    void CloseOptions();                                        // 0x00C564B0
    int  IsOptionsOpen();                                      // 0x00C564D8
    void OpenTell();                                         // 0x00C564E8: a tell, mode 28
    void CloseTell();                                       // 0x00C56520
    void OpenReply();                                         // 0x00C56560: a reply, mode 29
    void CloseReply();                                       // 0x00C56598
    void OpenCSRequest();                                       // 0x00C565D8: mode 31
    void CloseCSRequest();                                       // 0x00C565F8
    void OpenCSInbox();                                         // 0x00C56638: mode 32
    void CloseCSInbox();                                       // 0x00C56658
    void OpenWhoList();                                          // 0x00C56698: the who list, mode 35
    void CloseWhoList();                                       // 0x00C566B8
    void CloseRecipe();                                       // 0x00C56718
    void OpenExperiment();                                      // 0x00C56740: mode 37
    void CloseExperiment();                                       // 0x00C56760
    void OpenAuctionSearch(unsigned int kind);                  // 0x00C56788: mode 39
    void CloseAuctionSearch();                                       // 0x00C56848
    void OpenFaction();                                         // 0x00C568B0: mode 41
    void CloseFaction();                                        // 0x00C56918
    void OpenFishing();                                         // 0x00C56970: mode 36
    void CloseFishing();                                        // 0x00C569E0
    void ManualCameraBegun();                                   // 0x00C56A18, name provisional (BeginManualCamera): mode 38
    int  IsModal();                                             // 0x00C56A38, name provisional: mode 38
    void Refresh();                                             // 0x00C56A48, name provisional (ResetView): mode 0
    void FishingA();                                            // 0x00C56A68
    void FishingB();                                            // 0x00C56A88
    void FishingC(const unsigned short *text);                  // 0x00C56AA8: VIClientPart3FC10 passes the text on
    void FishingD(const char *text);                            // 0x00C56AC8: passes the text to VIWndFishing::SetText(const char *)
    void ShowName(int a, int b, int max, const unsigned short *name); // 0x00C56AE8, name provisional
    void AddText(const char *s);                                // 0x00C56BA0, name provisional
    void AddText(const unsigned short *s);                      // 0x00C56BC0, name provisional
    void AddText(const unsigned short *s, VIColor32F c);        // 0x00C56BE0, name provisional: c by value (it copies it)
    void LootOption(int id);                                    // 0x00C56C20, name provisional
    void AddWord(int id, VIColor32F c);                         // 0x00C56C40, name provisional: c by value
    void ShowNetStats(long age, unsigned long history, long x, unsigned short number, int ping); // 0x00C56C88, name provisional
    int  IsLootable();                                          // 0x00C56CA8, name provisional: mode 0
    int  DialogAllowed();                                       // 0x00C56CB8, name provisional (Msg0034): mode 9 or 14
    int  ModeHasChat();                                         // 0x00C56CE0: a table of m_mode < 34
    int  IsOptionsMode();                                       // 0x00C56D18: mode 34
    int  IsExperimentMode();                                    // 0x00C56D28: mode 37
    int  IsRecipeMode();                                        // 0x00C56D38: mode 6
    int  IsAuctionMode();                                       // 0x00C56D48: mode 39
    void OpenGoto();                                            // 0x00C56D58: mode 4
    void DrawGauge(int bar, int frame, VIVect2I at, int lit, float v); // 0x00C56D78
    void DrawCompass(float yaw);                                // 0x00C56F10
    void DrawPatch();                                           // 0x00C57400
    void ToggleHud();                                           // 0x00C57AB0
    void ShowNetStat(int on);                                   // 0x00C57B28
    VIChatText *GetChatLog();                                       // 0x00C57B40: the chat window's +0x1A8
    void SetEditTitle(int on);                                  // 0x00C57B60
    void Interact();                                            // 0x00C57CA0
    void Attack();                                              // 0x00C57D40

    // The class's inline members: emitted after __tf9VIWndGame in this order.
    int  HudOn() { return m_p05C; }                             // 0x00C58B78
    int  IsHud0(VIWndHUDMenu *w) { return w == m_hud[0]; }           // 0x00C58B80
    int  IsHud1(VIWndHUDMenu *w) { return w == m_hud[1]; }           // 0x00C58B90
    int  IsHud2(VIWndHUDMenu *w) { return w == m_hud[2]; }           // 0x00C58BA0
    void SetP678(int v) { m_p678 = v; }                         // 0x00C58BB0

    int               m_mat0;              // +0x024: a material, -1 none (WorldUnloaded releases it)
    int               m_mat1;              // +0x028
    int               m_glow0On;           // +0x02C: > 0 brightens m_glow0, else it dims to 0.3
    int               m_glow1On;           // +0x030
    int               m_glow2On;           // +0x034
    float             m_glow0;             // +0x038
    int               m_p03C;              // +0x03C: -1 none
    long              m_p040;              // +0x040: a scene time
    float             m_glow1;             // +0x048
    float             m_glow2;             // +0x04C
    int               m_mode;              // +0x050: UpdateMode's switch
    int               m_hudSel;            // +0x054: which of the three HUD menus
    int               m_p058;              // +0x058
    int               m_p05C;              // +0x05C: the HUD is up (Init sets 1)
    VIUIWindow        m_window;            // +0x060
    VIUIWindow        m_window2;           // +0x1E0
    VIUIWindow        m_window3;           // +0x360
    VIUITextScroll    m_scroll;            // +0x4E0
    int               m_p678;              // +0x678: InteractWithTarget sets it for a player target
    int               m_p67C;              // +0x67C
    VIWndLoot        *m_loot;              // +0x680
    VIWndMenu4       *m_menu4;             // +0x684
    VIWndChat        *m_chat;              // +0x688
    VIWndNetStat     *m_netStat;           // +0x68C
    VIWndEdit        *m_edit;              // +0x690
    VIWndGoto        *m_goto;              // +0x694
    VIWndSpellBook   *m_spellBook;         // +0x698
    VIWndInfluence   *m_influence;         // +0x69C
    VIWndResponse    *m_response;          // +0x6A0
    VIWndPrivChat    *m_privChat;          // +0x6A4
    VIWndMerchant    *m_merchant;          // +0x6A8
    VIWndBank        *m_bank;              // +0x6AC
    VIWndHUDMenu     *m_hud[3];            // +0x6B0
    VIWndTrainer     *m_trainer;           // +0x6BC
    VIWndClassMastery *m_classMastery;     // +0x6C0
    VIWndStartMenu   *m_startMenu;         // +0x6C4
    VIWndMBox        *m_mbox;              // +0x6C8
    VIWndTrade       *m_trade;             // +0x6CC
    VIWndBuddy       *m_buddy;             // +0x6D0
    VIWndBlacksmith  *m_blacksmith;        // +0x6D4
    VIWndGuild       *m_guild;             // +0x6D8
    VIWndQuestLog    *m_questLog;          // +0x6DC
    VIWndComposeGram *m_composeGram;       // +0x6E0
    VIWndGramInbox   *m_gramInbox;         // +0x6E4
    VIWndIgnoreList  *m_ignoreList;        // +0x6E8
    VIWndBuffPC      *m_buffPC;            // +0x6EC
    VIWndOptions     *m_options;           // +0x6F0
    VIWndTell        *m_tell;              // +0x6F4
    VIWndReply       *m_reply;             // +0x6F8
    VIWndCSRequest   *m_csRequest;         // +0x6FC
    VIWndCSInbox     *m_csInbox;           // +0x700
    VIWndWhoList     *m_whoList;           // +0x704
    VIWndRecipe      *m_recipe;            // +0x708
    VIWndExperiment  *m_experiment;        // +0x70C
    VIWndFishing     *m_fishing;           // +0x710
    int               m_p714;              // +0x714
    VIWndAuctionSearch *m_auctionSearch;   // +0x718
    VIWndFaction     *m_faction;           // +0x71C
    // ---- Other lanes add VIWndGame declarations ONLY between their own marker lines.
    void OpenRecipe(int skill);    // 0x00C566E0: the recipe panel, mode 6; `skill` goes on to VIWndRecipe 0x00C8D430 in a1
    // cl-wg's inlines (0x00C58B80/90/A0, their names); m_hud[3] is cl-wg's member at +0x6B0.
};

CHK(c_VIWndGame, sizeof(VIWndGame) == 0x720)
#endif
