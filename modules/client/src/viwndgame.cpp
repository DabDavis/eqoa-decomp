#line 1 "C:/code-frontiers-gm/game/VIWndGame.cpp"
// viwndgame.cpp (lane cl-wg) -- VIWndGame, the game window: CLIENT 0x00C4E640 .. 0x00C58BD8.
// The HUD (status bars, group, pet, target, the three HUD menus) and every in-game panel,
// one at a time through m_mode.  OnMessage's assert must stay on VIWndGame.cpp:4007.
#include "game.h"
#include "viarray_t.h"
#include "vimap_t.h"

// The panels' sizes Init `new`s them with (other lanes' classes; this lane padded its copies).
CHK(c_wg_VIWndSpellBook, sizeof(VIWndSpellBook) == 2016)
CHK(c_wg_VIWndLoot, sizeof(VIWndLoot) == 1464)
CHK(c_wg_VIWndMerchant, sizeof(VIWndMerchant) == 2480)
CHK(c_wg_VIWndMenu4, sizeof(VIWndMenu4) == 76)
CHK(c_wg_VIWndChat, sizeof(VIWndChat) == 5592)
CHK(c_wg_VIWndNetStat, sizeof(VIWndNetStat) == 44)
CHK(c_wg_VIWndEdit, sizeof(VIWndEdit) == 448)
CHK(c_wg_VIWndGoto, sizeof(VIWndGoto) == 60)
CHK(c_wg_VIWndResponse, sizeof(VIWndResponse) == 2512)
CHK(c_wg_VIWndPrivChat, sizeof(VIWndPrivChat) == 1456)
CHK(c_wg_VIWndBank, sizeof(VIWndBank) == 52)
CHK(c_wg_VIWndTrainer, sizeof(VIWndTrainer) == 968)
CHK(c_wg_VIWndClassMastery, sizeof(VIWndClassMastery) == 584)
CHK(c_wg_VIWndStartMenu, sizeof(VIWndStartMenu) == 44)
CHK(c_wg_VIWndMBox, sizeof(VIWndMBox) == 1504)
CHK(c_wg_VIWndTrade, sizeof(VIWndTrade) == 872)
CHK(c_wg_VIWndBuddy, sizeof(VIWndBuddy) == 504)
CHK(c_wg_VIWndBlacksmith, sizeof(VIWndBlacksmith) == 2328)
CHK(c_wg_VIWndGuild, sizeof(VIWndGuild) == 520)
CHK(c_wg_VIWndQuestLog, sizeof(VIWndQuestLog) == 496)
CHK(c_wg_VIWndGramInbox, sizeof(VIWndGramInbox) == 880)
CHK(c_wg_VIWndIgnoreList, sizeof(VIWndIgnoreList) == 512)
CHK(c_wg_VIWndBuffPC, sizeof(VIWndBuffPC) == 88)
CHK(c_wg_VIWndOptions, sizeof(VIWndOptions) == 2432)
CHK(c_wg_VIWndTell, sizeof(VIWndTell) == 1768)
CHK(c_wg_VIWndReply, sizeof(VIWndReply) == 608)
CHK(c_wg_VIWndCSInbox, sizeof(VIWndCSInbox) == 856)
CHK(c_wg_VIWndWhoList, sizeof(VIWndWhoList) == 888)
CHK(c_wg_VIWndRecipe, sizeof(VIWndRecipe) == 1064)
CHK(c_wg_VIWndExperiment, sizeof(VIWndExperiment) == 6216)
CHK(c_wg_VIWndFishing, sizeof(VIWndFishing) == 2064)
CHK(c_wg_VIWndAuctionSearch, sizeof(VIWndAuctionSearch) == 5928)
CHK(c_wg_VIWndFaction, sizeof(VIWndFaction) == 1256)
CHK(c_wg_VIWndInfluence, sizeof(VIWndInfluence) == 44)
CHK(c_wg_VIWndHUDMenu, sizeof(VIWndHUDMenu) == 504)
CHK(c_wg_VIWndComposeGram, sizeof(VIWndComposeGram) == 3176)
CHK(c_wg_VIWndCSRequest, sizeof(VIWndCSRequest) == 2560)


// 0x00CF5AB0: five colours, built by the TU's static initialiser (0x00C58A80) and never read.
static VIColor32 sColors[5] =
{
    VIColor32(255, 132, 83, 255),
    VIColor32(231, 194, 44, 255),
    VIColor32(255, 255, 255, 255),
    VIColor32(57, 191, 219, 255),
    VIColor32(128, 128, 128, 255),
};

// 0x00C4E640.
VIWndGame::VIWndGame()
{
    m_loot = 0;
    m_menu4 = 0;
    m_chat = 0;
    m_netStat = 0;
    m_edit = 0;
    m_goto = 0;
    m_spellBook = 0;
    m_influence = 0;
    m_response = 0;
    m_privChat = 0;
    m_merchant = 0;
    m_bank = 0;
    m_hud[0] = 0;
    m_hud[1] = 0;
    m_hud[2] = 0;
    m_trainer = 0;
    m_classMastery = 0;
    m_startMenu = 0;
    m_mbox = 0;
    m_trade = 0;
    m_buddy = 0;
    m_blacksmith = 0;
    m_guild = 0;
    m_questLog = 0;
    m_composeGram = 0;
    m_gramInbox = 0;
    m_ignoreList = 0;
    m_tell = 0;
    m_reply = 0;
    m_csRequest = 0;
    m_csInbox = 0;
    m_whoList = 0;
    m_recipe = 0;
    m_experiment = 0;
    m_fishing = 0;
    m_auctionSearch = 0;
    m_faction = 0;
    m_mat0 = -1;
    m_mat1 = -1;
    m_glow0 = 0.3f;
    m_glow0On = -1;
    m_glow1 = 0.3f;
    m_glow1On = -1;
    m_glow2 = 0.3f;
    m_glow2On = -1;
    m_p678 = 0;
}

// 0x00C4E760.
VIWndGame::~VIWndGame()
{
    Free();
}

// 0x00C4E7D8: every panel, `new`ed, hidden, owned by the client and parented here, in
// this order (Free deletes them in another).  Most Inits returning non-zero fail the
// whole thing with -1; the first seven hand their own code back.
int VIWndGame::Init(VIScene *scene)
{
    int r;

    m_influence = new VIWndInfluence;
    if (!m_influence) { r = -1; goto done; }
    m_influence->m_visible = 0;
    m_influence->SetAppPtr(GetAppPtr());
    m_influence->SetPos(VIVect2I(143, 15));
    AddChild(m_influence);

    m_spellBook = new VIWndSpellBook;
    if (!m_spellBook) { r = -1; goto done; }
    m_spellBook->m_visible = 0;
    m_spellBook->SetAppPtr(GetAppPtr());
    m_spellBook->m_pos = VIVect2I(0, 0);
    AddChild(m_spellBook);
    if ((r = m_spellBook->Init()) != 0) goto done;

    m_loot = new VIWndLoot;
    if (!m_loot) { r = -1; goto done; }
    m_loot->m_visible = 0;
    m_loot->SetAppPtr(GetAppPtr());
    AddChild(m_loot);
    if ((r = m_loot->Init()) != 0) goto done;

    m_merchant = new VIWndMerchant;
    if (!m_merchant) { r = -1; goto done; }
    m_merchant->m_visible = 0;
    m_merchant->SetAppPtr(GetAppPtr());
    AddChild(m_merchant);
    if ((r = m_merchant->Init()) != 0) goto done;

    m_menu4 = new VIWndMenu4;
    if (!m_menu4) { r = -1; goto done; }
    m_menu4->m_visible = 0;
    m_menu4->SetAppPtr(GetAppPtr());
    m_menu4->m_pos = VIVect2I(175, 18);
    AddChild(m_menu4);

    m_chat = new VIWndChat;
    if (!m_chat) { r = -1; goto done; }
    m_chat->m_visible = 0;
    m_chat->SetAppPtr(GetAppPtr());
    AddChild(m_chat);
    if ((r = m_chat->Init()) != 0) goto done;

    m_netStat = new VIWndNetStat;
    if (!m_netStat) { r = -1; goto done; }
    m_netStat->m_visible = 0;
    m_netStat->SetAppPtr(GetAppPtr());
    AddChild(m_netStat);

    m_edit = new VIWndEdit;
    if (!m_edit) { r = -1; goto done; }
    m_edit->m_visible = 0;
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);

    m_goto = new VIWndGoto;
    if (!m_goto) { r = -1; goto done; }
    m_goto->m_visible = 0;
    m_goto->SetAppPtr(GetAppPtr());
    m_goto->Init(scene);
    AddChild(m_goto);

    m_response = new VIWndResponse;
    if (!m_response) { r = -1; goto done; }
    m_response->m_visible = 0;
    m_response->SetAppPtr(GetAppPtr());
    m_response->SetPos(VIVect2I(143, 35));
    AddChild(m_response);

    m_privChat = new VIWndPrivChat;
    if (!m_privChat) { r = -1; goto done; }
    m_privChat->m_visible = 0;
    m_privChat->SetAppPtr(GetAppPtr());
    m_privChat->SetPos(VIVect2I(143, 120));
    AddChild(m_privChat);

    m_bank = new VIWndBank;
    if (!m_bank) { r = -1; goto done; }
    m_bank->m_visible = 0;
    m_bank->SetAppPtr(GetAppPtr());
    AddChild(m_bank);
    if ((r = m_bank->Init()) != 0) goto done;

    m_hud[0] = new VIWndHUDMenu;
    if (!m_hud[0]) { r = -1; goto done; }
    m_hud[0]->m_visible = 0;
    m_hud[0]->SetAppPtr(GetAppPtr());
    AddChild(m_hud[0]);
    if ((r = m_hud[0]->Init()) != 0) goto done;
    m_hud[0]->m_pos = VIVect2I(430, 25);
    m_hud[0]->m_index = 0;
    m_hud[0]->SetMenu(&gHudMenu0);

    m_hud[1] = new VIWndHUDMenu;
    if (!m_hud[1]) { r = -1; goto done; }
    m_hud[1]->m_visible = 0;
    m_hud[1]->SetAppPtr(GetAppPtr());
    AddChild(m_hud[1]);
    if ((r = m_hud[1]->Init()) != 0) goto done;
    m_hud[1]->m_pos = VIVect2I(430, 25);
    m_hud[1]->m_index = 1;
    m_hud[1]->SetMenu(&gHudMenu1);

    m_hud[2] = new VIWndHUDMenu;
    if (!m_hud[2]) { r = -1; goto done; }
    m_hud[2]->m_visible = 0;
    m_hud[2]->SetAppPtr(GetAppPtr());
    AddChild(m_hud[2]);
    if (m_hud[2]->Init()) { r = -1; goto done; }
    m_hud[2]->m_pos = VIVect2I(430, 25);
    m_hud[2]->m_index = 2;
    m_hud[2]->SetMenu(&gHudMenu2);

    m_trainer = new VIWndTrainer;
    if (!m_trainer) { r = -1; goto done; }
    m_trainer->m_visible = 0;
    m_trainer->SetAppPtr(GetAppPtr());
    AddChild(m_trainer);
    if (m_trainer->Init()) { r = -1; goto done; }
    m_trainer->m_pos = VIVect2I(143, 10);

    m_classMastery = new VIWndClassMastery;
    if (!m_classMastery) { r = -1; goto done; }
    m_classMastery->m_visible = 0;
    m_classMastery->SetAppPtr(GetAppPtr());
    AddChild(m_classMastery);
    if (m_classMastery->Init()) { r = -1; goto done; }
    m_classMastery->SetPos(VIVect2I(143, 10));

    m_startMenu = new VIWndStartMenu;
    if (!m_startMenu) { r = -1; goto done; }
    m_startMenu->m_visible = 0;
    m_startMenu->SetAppPtr(GetAppPtr());
    AddChild(m_startMenu);
    if (m_startMenu->Init()) { r = -1; goto done; }
    m_startMenu->SetMainX24(&gStartMenu);
    m_startMenu->Items34(&gEquipMenu);
    m_startMenu->Items38(&gItemMenu3);
    m_startMenu->Items3C(&gItemMenu4);
    m_startMenu->Items40(&gInvMenu);

    m_mbox = new VIWndMBox;
    if (!m_mbox) { r = -1; goto done; }
    m_mbox->m_visible = 0;
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    if (m_mbox->Init(0, 1)) { r = -1; goto done; }
    m_mbox->m_pos = VIVect2I(80, 115);

    m_trade = new VIWndTrade;
    if (!m_trade) { r = -1; goto done; }
    m_trade->m_visible = 0;
    m_trade->SetAppPtr(GetAppPtr());
    AddChild(m_trade);
    if (m_trade->Init()) { r = -1; goto done; }

    m_buddy = new VIWndBuddy;
    if (!m_buddy) { r = -1; goto done; }
    m_buddy->m_visible = 0;
    m_buddy->SetAppPtr(GetAppPtr());
    AddChild(m_buddy);
    if (m_buddy->Init()) { r = -1; goto done; }

    m_blacksmith = new VIWndBlacksmith;
    if (!m_blacksmith) { r = -1; goto done; }
    m_blacksmith->m_visible = 0;
    m_blacksmith->SetAppPtr(GetAppPtr());
    AddChild(m_blacksmith);
    if (m_blacksmith->Init()) { r = -1; goto done; }

    m_guild = new VIWndGuild;
    if (!m_guild) { r = -1; goto done; }
    m_guild->m_visible = 0;
    m_guild->SetAppPtr(GetAppPtr());
    AddChild(m_guild);
    if (m_guild->Init()) { r = -1; goto done; }

    m_questLog = new VIWndQuestLog;
    if (!m_questLog) { r = -1; goto done; }
    m_questLog->m_visible = 0;
    m_questLog->SetAppPtr(GetAppPtr());
    AddChild(m_questLog);
    if (m_questLog->Init()) { r = -1; goto done; }

    m_composeGram = new VIWndComposeGram;
    if (!m_composeGram) { r = -1; goto done; }
    m_composeGram->m_visible = 0;
    m_composeGram->SetAppPtr(GetAppPtr());
    AddChild(m_composeGram);
    if (m_composeGram->Init(0)) { r = -1; goto done; }

    m_gramInbox = new VIWndGramInbox;
    if (!m_gramInbox) { r = -1; goto done; }
    m_gramInbox->m_visible = 0;
    m_gramInbox->SetAppPtr(GetAppPtr());
    AddChild(m_gramInbox);
    if (m_gramInbox->Init()) { r = -1; goto done; }

    m_ignoreList = new VIWndIgnoreList;
    if (!m_ignoreList) { r = -1; goto done; }
    m_ignoreList->m_visible = 0;
    m_ignoreList->SetAppPtr(GetAppPtr());
    AddChild(m_ignoreList);
    if (m_ignoreList->Init()) { r = -1; goto done; }

    m_buffPC = new VIWndBuffPC;
    if (!m_buffPC) { r = -1; goto done; }
    m_buffPC->m_visible = 0;
    m_buffPC->SetAppPtr(GetAppPtr());
    AddChild(m_buffPC);
    if (m_buffPC->Init()) { r = -1; goto done; }

    m_options = new VIWndOptions;
    if (!m_options) { r = -1; goto done; }
    m_options->m_visible = 0;
    m_options->SetAppPtr(GetAppPtr());
    AddChild(m_options);
    if (m_options->Init(scene)) { r = -1; goto done; }

    m_tell = new VIWndTell;
    if (!m_tell) { r = -1; goto done; }
    m_tell->m_visible = 0;
    m_tell->SetAppPtr(GetAppPtr());
    AddChild(m_tell);
    if (m_tell->Init()) { r = -1; goto done; }

    m_reply = new VIWndReply;
    if (!m_reply) { r = -1; goto done; }
    m_reply->m_visible = 0;
    m_reply->SetAppPtr(GetAppPtr());
    AddChild(m_reply);
    if (m_reply->Init()) { r = -1; goto done; }

    m_csRequest = new VIWndCSRequest;
    if (!m_csRequest) { r = -1; goto done; }
    m_csRequest->m_visible = 0;
    m_csRequest->SetAppPtr(GetAppPtr());
    AddChild(m_csRequest);
    if (m_csRequest->Init(0)) { r = -1; goto done; }

    m_csInbox = new VIWndCSInbox;
    if (!m_csInbox) { r = -1; goto done; }
    m_csInbox->m_visible = 0;
    m_csInbox->SetAppPtr(GetAppPtr());
    AddChild(m_csInbox);
    if (m_csInbox->Init()) { r = -1; goto done; }

    m_whoList = new VIWndWhoList;
    if (!m_whoList) { r = -1; goto done; }
    m_whoList->m_visible = 0;
    m_whoList->SetAppPtr(GetAppPtr());
    m_whoList->Init();
    AddChild(m_whoList);

    m_recipe = new VIWndRecipe;
    if (!m_recipe) { r = -1; goto done; }
    m_recipe->m_visible = 0;
    m_recipe->SetAppPtr(GetAppPtr());
    if (m_recipe->Init()) { r = -1; goto done; }
    AddChild(m_recipe);
    ((VIClient *)GetAppPtr())->GetP3834C()->m_wndRecipe = m_recipe;

    m_experiment = new VIWndExperiment;
    if (!m_experiment) { r = -1; goto done; }
    m_experiment->m_visible = 0;
    m_experiment->SetAppPtr(GetAppPtr());
    if (m_experiment->Init()) { r = -1; goto done; }
    AddChild(m_experiment);
    ((VIClient *)GetAppPtr())->GetP3834C()->m_wndExperiment = m_experiment;

    m_fishing = new VIWndFishing;
    if (!m_fishing) { r = -1; goto done; }
    m_fishing->m_visible = 0;
    m_fishing->SetAppPtr(GetAppPtr());
    if (m_fishing->Init(scene)) { r = -1; goto done; }
    m_fishing->SetChat(m_chat);
    AddChild(m_fishing);

    m_auctionSearch = new VIWndAuctionSearch;
    if (!m_auctionSearch) { r = -1; goto done; }
    m_auctionSearch->m_visible = 0;
    m_auctionSearch->SetAppPtr(GetAppPtr());
    if (m_auctionSearch->Init()) { r = -1; goto done; }
    AddChild(m_auctionSearch);

    m_faction = new VIWndFaction;
    if (!m_faction) { r = -1; goto done; }
    m_faction->m_visible = 0;
    m_faction->SetAppPtr(GetAppPtr());
    if ((r = m_faction->Init()) != 0) { r = -1; goto done; }
    AddChild(m_faction);
    m_window.Init(mUI, VIVect2I(0, 0), VIVect2I(530, 235), 195, 1, 1);
    m_window.Close(1);
    m_hudSel = 0;
    m_p678 = 0;
    m_mode = 0;
    m_glow0 = 0.3f;
    m_glow0On = -1;
    m_p03C = -1;
    m_glow1 = 0.3f;
    m_glow1On = -1;
    m_glow2On = -1;
    m_glow2 = 0.3f;
    m_p058 = -1;
    m_p05C = 1;
    UpdateMode();
done:
    return r;
}

// 0x00C4F648: every panel removed and deleted, the options saved first (Free).
int VIWndGame::Free()
{
    if (m_influence) { RemoveChild(m_influence); delete m_influence; m_influence = 0; }
    if (m_loot) { RemoveChild(m_loot); delete m_loot; m_loot = 0; }
    if (m_menu4) { RemoveChild(m_menu4); delete m_menu4; m_menu4 = 0; }
    if (m_chat) { RemoveChild(m_chat); delete m_chat; m_chat = 0; }
    if (m_netStat) { RemoveChild(m_netStat); delete m_netStat; m_netStat = 0; }
    if (m_edit) { RemoveChild(m_edit); delete m_edit; m_edit = 0; }
    if (m_goto) { RemoveChild(m_goto); delete m_goto; m_goto = 0; }
    if (m_spellBook) { RemoveChild(m_spellBook); delete m_spellBook; m_spellBook = 0; }
    if (m_response) { RemoveChild(m_response); delete m_response; m_response = 0; }
    if (m_privChat) { RemoveChild(m_privChat); delete m_privChat; m_privChat = 0; }
    if (m_merchant) { RemoveChild(m_merchant); delete m_merchant; m_merchant = 0; }
    if (m_bank) { RemoveChild(m_bank); delete m_bank; m_bank = 0; }
    if (m_hud[0]) { RemoveChild(m_hud[0]); delete m_hud[0]; m_hud[0] = 0; }
    if (m_hud[1]) { RemoveChild(m_hud[1]); delete m_hud[1]; m_hud[1] = 0; }
    if (m_hud[2]) { RemoveChild(m_hud[2]); delete m_hud[2]; m_hud[2] = 0; }
    if (m_classMastery) { RemoveChild(m_classMastery); delete m_classMastery; m_classMastery = 0; }
    if (m_trainer) { RemoveChild(m_trainer); delete m_trainer; m_trainer = 0; }
    if (m_startMenu) { RemoveChild(m_startMenu); delete m_startMenu; m_startMenu = 0; }
    if (m_mbox) { RemoveChild(m_mbox); delete m_mbox; m_mbox = 0; }
    if (m_trade) { RemoveChild(m_trade); delete m_trade; m_trade = 0; }
    if (m_buddy) { RemoveChild(m_buddy); delete m_buddy; m_buddy = 0; }
    if (m_blacksmith) { RemoveChild(m_blacksmith); delete m_blacksmith; m_blacksmith = 0; }
    if (m_guild) { RemoveChild(m_guild); delete m_guild; m_guild = 0; }
    if (m_questLog) { RemoveChild(m_questLog); delete m_questLog; m_questLog = 0; }
    if (m_composeGram) { RemoveChild(m_composeGram); delete m_composeGram; m_composeGram = 0; }
    if (m_gramInbox) { RemoveChild(m_gramInbox); delete m_gramInbox; m_gramInbox = 0; }
    if (m_ignoreList) { RemoveChild(m_ignoreList); delete m_ignoreList; m_ignoreList = 0; }
    if (m_buffPC) { RemoveChild(m_buffPC); delete m_buffPC; m_buffPC = 0; }
    if (m_options) { RemoveChild(m_options); m_options->Free(); delete m_options; m_options = 0; }
    if (m_tell) { RemoveChild(m_tell); delete m_tell; m_tell = 0; }
    if (m_reply) { RemoveChild(m_reply); delete m_reply; m_reply = 0; }
    if (m_csRequest) { RemoveChild(m_csRequest); delete m_csRequest; m_csRequest = 0; }
    if (m_csInbox) { RemoveChild(m_csInbox); delete m_csInbox; m_csInbox = 0; }
    if (m_whoList) { RemoveChild(m_whoList); delete m_whoList; m_whoList = 0; }
    if (m_recipe) { RemoveChild(m_recipe); delete m_recipe; m_recipe = 0; }
    if (m_experiment) { RemoveChild(m_experiment); delete m_experiment; m_experiment = 0; }
    if (m_fishing) { RemoveChild(m_fishing); delete m_fishing; m_fishing = 0; }
    if (m_auctionSearch) { RemoveChild(m_auctionSearch); delete m_auctionSearch; m_auctionSearch = 0; }
    if (m_faction) { RemoveChild(m_faction); delete m_faction; m_faction = 0; }
    return 0;
}

// 0x00C4FF00: the world is gone; its two materials and the text sets with it.
int VIWndGame::WorldUnloaded()
{
    VIScene &scene = ((VIClient *)GetAppPtr())->m_scene;
    if (m_mat0 >= 0)
    {
        scene.m_raster->ReleaseMaterial(m_mat0, *scene.m_dict);
        m_mat0 = -1;
    }
    if (m_mat1 >= 0)
    {
        scene.m_raster->ReleaseMaterial(m_mat1, *scene.m_dict);
        m_mat1 = -1;
    }
    VITextSet::FreeAll();
    VITextSet::Reset();
    return 0;
}

// 0x00C4FF80: every panel hidden, the HUD back (if it is up), and back to mode 0 -- or,
// in the start menu (mode 13), out of it.  keepHud keeps the HUD menu selection.
void VIWndGame::SetMode(int keepHud)
{
    m_loot->m_visible = 0;
    m_menu4->m_visible = 0;
    m_edit->m_visible = 0;
    m_goto->m_visible = 0;
    m_spellBook->m_visible = 0;
    m_response->m_visible = 0;
    m_privChat->m_visible = 0;
    m_merchant->m_visible = 0;
    m_bank->m_visible = 0;
    m_trainer->m_visible = 0;
    m_classMastery->m_visible = 0;
    m_startMenu->m_visible = 0;
    m_mbox->m_visible = 0;
    m_trade->m_visible = 0;
    m_buddy->m_visible = 0;
    m_blacksmith->m_visible = 0;
    m_guild->m_visible = 0;
    m_questLog->m_visible = 0;
    m_composeGram->m_visible = 0;
    m_gramInbox->m_visible = 0;
    m_ignoreList->m_visible = 0;
    m_chat->SetMode(0);
    m_tell->m_visible = 0;
    m_reply->m_visible = 0;
    m_csRequest->m_visible = 0;
    m_csInbox->m_visible = 0;
    m_whoList->m_visible = 0;
    m_options->m_visible = 0;
    m_recipe->m_visible = 0;
    m_experiment->m_visible = 0;
    m_fishing->m_visible = 0;
    m_auctionSearch->m_visible = 0;
    m_faction->m_visible = 0;
    if (m_p05C)
    {
        m_hud[0]->m_visible = 1;
        m_hud[1]->m_visible = 1;
        m_hud[2]->m_visible = 1;
        m_influence->m_visible = 1;
        m_chat->m_visible = 1;
    }
    else
    {
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_influence->m_visible = 0;
        m_chat->m_visible = 0;
    }
    if (((VIClient *)GetAppPtr())->m_p50304 & 2)
    {
        m_hud[0]->SetTitle(m_window.T("(1)"));
        m_hud[1]->SetTitle(m_window.T("(2)"));
    }
    else
    {
        m_hud[0]->SetTitle(m_window.T(""));
        m_hud[0]->SetTitle(m_window.T(""));
    }
    m_p678 = 0;
    if (keepHud)
        m_hudSel = 0;
    if (m_mode == 13)
        CloseStartMenu(0);
    else
    {
        m_mode = 0;
        UpdateMode();
    }
}

// 0x00C50200 (Msg000D).
void VIWndGame::ChatRefresh()
{
    m_chat->Reset();
}

// 0x00C50220: m_mode's panel shown (most hide the HUD menus and turn the chat's
// mode on), the rest of the HUD shown or not, and the active window set.
void VIWndGame::UpdateMode()
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->MaybeStopAutoAttack();
    app->SetP06DC0(0, 0);
    if (!m_p05C && m_mode)
    {
        m_mode = 0;
        app->SetP06DC0(1, 1);
        app->SetActiveWindow(this);
        return;
    }
    m_influence->m_visible = 0;
    switch (m_mode)
    {
    case 0:
        app->SetP06DC0(1, 1);
    case 38:
        if (!m_p05C)
        {
            app->SetActiveWindow(this);
            break;
        }
        m_influence->m_visible = 1;
        m_chat->m_visible = 1;
        switch (m_hudSel)
        {
        case 0:
            m_hud[2]->m_visible = 0;
            m_hud[0]->m_visible = 1;
            m_hud[1]->m_visible = 0;
            app->SetActiveWindow(m_hud[0]);
            break;
        case 1:
            m_hud[2]->m_visible = 0;
            m_hud[0]->m_visible = 0;
            m_hud[1]->m_visible = 1;
            app->SetActiveWindow(m_hud[1]);
            break;
        case 2:
            m_hud[0]->m_visible = 0;
            m_hud[1]->m_visible = 0;
            m_hud[2]->m_visible = 1;
            app->SetActiveWindow(m_hud[2]);
            break;
        }
        break;
    case 1:
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_menu4->m_visible = 0;
        m_loot->m_visible = 1;
        break;
    case 10:
        m_menu4->m_visible = 0;
        m_merchant->m_visible = 1;
        break;
    case 7:
        m_influence->m_visible = 1;
        m_response->m_visible = 1;
        app->SetActiveWindow(m_response);
        break;
    case 9:
        m_influence->m_visible = 1;
        m_privChat->m_visible = 1;
        app->SetActiveWindow(m_privChat);
        break;
    case 2:
        app->SetP06DC0(1, 1);
        m_influence->m_visible = 0;
        m_menu4->m_visible = 0;
        m_edit->m_visible = 1;
        app->SetActiveWindow(m_edit);
        break;
    case 4:
        m_influence->m_visible = 1;
        m_menu4->m_visible = 0;
        m_goto->m_visible = 1;
        app->SetActiveWindow(m_goto);
        break;
    case 5:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_spellBook->Open();
        break;
    case 11:
        m_menu4->m_visible = 0;
        m_bank->m_visible = 1;
        break;
    case 12:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_trainer->m_visible = 1;
        app->SetActiveWindow(m_trainer);
        break;
    case 40:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_classMastery->m_visible = 1;
        app->SetActiveWindow(m_classMastery);
        break;
    case 14:
        m_influence->m_visible = 1;
        m_mbox->m_visible = 1;
        app->SetActiveWindow(m_mbox);
        break;
    case 15:
        m_chat->SetMode(1);
        m_influence->m_visible = 0;
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_trade->m_visible = 1;
        m_trade->Start();
        break;
    case 16:
        m_influence->m_visible = 1;
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_trade->m_visible = 1;
        m_trade->Ask();
        break;
    case 17:
        m_influence->m_visible = 1;
        m_buddy->m_visible = 1;
        m_buddy->ShowRequest();
        break;
    case 18:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_buddy->m_visible = 1;
        m_buddy->Close();
        break;
    case 19:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_blacksmith->m_visible = 1;
        m_blacksmith->Open();
        break;
    case 20:
        m_influence->m_visible = 1;
        m_guild->m_visible = 1;
        m_guild->ShowInvite();
        break;
    case 21:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_guild->m_visible = 1;
        m_guild->Close();
        break;
    case 22:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_questLog->m_visible = 1;
        m_questLog->Open();
        break;
    case 23:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_composeGram->m_visible = 1;
        m_composeGram->Refresh();
        app->SetActiveWindow(m_composeGram);
        break;
    case 24:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_gramInbox->m_visible = 1;
        m_gramInbox->Refresh();
        app->SetActiveWindow(m_gramInbox);
        break;
    case 26:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_ignoreList->m_visible = 1;
        m_ignoreList->Close();
        break;
    case 27:
        m_buffPC->m_visible = 1;
        app->SetActiveWindow(m_buffPC);
        break;
    case 34:
        m_options->m_visible = 1;
        app->SetActiveWindow(m_options);
        break;
    case 28:
    {
        char name[32];
        unsigned short wide[24];
        wide[0] = 0;
        m_tell->m_visible = 1;
        if (app->GetTargetPlayerName(name) >= 0)
            VIString::str8to16(wide, 48, name);
        app->SetP06DC0(1, 1);
        m_tell->Open(wide);
        app->SetActiveWindow(m_tell);
        break;
    }
    case 29:
        app->SetP06DC0(1, 1);
        m_reply->m_visible = 1;
        m_reply->Start(app->m_p4F0B8);
        app->SetActiveWindow(m_reply);
        break;
    case 31:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_csRequest->m_visible = 1;
        m_csRequest->Refresh();
        app->SetActiveWindow(m_csRequest);
        break;
    case 32:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_csInbox->m_visible = 1;
        m_csInbox->Refresh();
        app->SetActiveWindow(m_csInbox);
        break;
    case 33:
        m_influence->m_visible = 1;
        m_chat->SetScroll(1);
        app->SetActiveWindow(m_chat);
        break;
    case 35:
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_chat->SetMode(1);
        m_whoList->m_visible = 1;
        app->SetActiveWindow(m_whoList);
        break;
    case 6:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_recipe->m_visible = 1;
        app->SetActiveWindow(m_recipe);
        break;
    case 37:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_experiment->m_visible = 1;
        m_experiment->Open();
        break;
    case 36:
        m_influence->m_visible = 1;
        m_fishing->m_visible = 1;
        app->SetActiveWindow(m_fishing);
        app->SetP06DC0(0, 1);
        break;
    case 39:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_auctionSearch->m_visible = 1;
        if (!m_auctionSearch->m_mode)
            app->SetActiveWindow(m_auctionSearch);
        break;
    case 41:
        m_chat->SetMode(1);
        m_hud[0]->m_visible = 0;
        m_hud[1]->m_visible = 0;
        m_hud[2]->m_visible = 0;
        m_faction->m_visible = 1;
        app->SetActiveWindow(m_faction);
        break;
    }
}

// 0x00C50B58: the HUD -- the player's bars, the group, the pet, the status line, the
// target -- and the manual camera's idle timeout.
void VIWndGame::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    app->UpdateGame();
    if (!m_p05C)
        return;
    UpdateGlow();
    ui.Begin2D();
    VIVect2I a;
    VIVect2I b;
    VIVect2I c;
    VIVect2I p(10, 15);
    b.Set(0, 0);
    ui.m_font.GetHeight(ui.m_raster, ui.m_dict);
    ui.Blit((VIUITexId)6, p);
    if (app->FindEntity(app->m_selfId))
    {
        VIVect3 hpr;
        app->m_camera.m_dir.HPR(&hpr.x, &hpr.y, &hpr.z);
        DrawCompass(hpr.x);
        DrawPatch();
    }
    p += VIVect2I(59, 21);
    float v = app->GetSelfP19() / 255.0f;
    DrawGauge(7, 136, p, 1, v);
    p.y += 10;
    if (app->GetMaxPower() == 0)
        v = 0.0f;
    else
        v = (float)app->GetPower() / (float)app->GetMaxPower();
    DrawGauge(9, 136, p, 1, v);
    float part = 0.0f;
    p.y += 10;
    int level = app->GetLevel();
    int exp = app->GetExp();
    unsigned int need = ExpForLevel(level + 1);
    v = exp / (float)need;
    int fifth = need / 5;
    if (exp <= 0)
        part = 0.0f;
    else
    {
        int i = 1;
        if (exp < fifth)
            part = (float)exp / (float)fifth;
        else
        {
        next:
            if (++i < 6)
            {
                if (exp >= i * fifth)
                    goto next;
                part = (float)(exp - (i - 1) * fifth) / (float)fifth;
            }
        }
    }
    part = VIMIN(1.0f, VIMAX(0.0f, part));
    DrawGauge(152, 151, p + VIVect2I(0, 10), 1, part);
    DrawGauge(8, 136, p, 1, v);
    p.Set(20, 93);
    DrawGroup(p);
    p.y += VIMAX(0, (app->GetP2BE4C() - 1) * 48);
    DrawPet(p);
    p.Set(430, 50);
    DrawStatus(p);
    p.Set(-3, 331);
    if (m_chat->m_mode)
        ui.BlitRect((VIUITexId)32, VIVect2I(p.x + 2, p.y), VIVect2I(203, 125), (VIAlphaBlendMode)0, 1.0f);
    p.Set(20, 348);
    DrawTarget(p);
    if (m_p678)
        m_window.Open(0);
    else
        m_window.Close(0);
    if (m_window.IsOpen() || m_window.IsOpening() || m_window.IsClosing())
        DrawTargetInfo();
    ui.End2D();
    if (m_p03C != -1 && (int)(app->m_p07568 - m_p040) > 300 && !m_mode)
    {
        app->BeginManualCamera();
        m_p03C = -1;
    }
    if (m_mode)
    {
        m_p040 = app->m_p07568;
        m_p03C = -1;
    }
}

// 0x00C51170: the three glows, brightening while their flag is up and fading to 0.3.
void VIWndGame::UpdateGlow()
{
    if (m_glow0On > 0)
        m_glow0 = VIMIN(m_glow0 + 0.32f, 1.0f);
    else
        m_glow0 = VIMAX(0.3f, m_glow0 - 0.105f);
    if (m_glow1On > 0)
        m_glow1 = VIMIN(m_glow1 + 0.32f, 1.0f);
    else
        m_glow1 = VIMAX(0.3f, m_glow1 - 0.105f);
    if (m_glow2On > 0)
        m_glow2 = VIMIN(m_glow2 + 0.32f, 1.0f);
    else
        m_glow2 = VIMAX(0.3f, m_glow2 - 0.105f);
}

// 0x00C512C8: the group, one bar per member who is not the player: a frame, the name,
// the health gauge and two status icons; the target's frame glows.  Not in the start
// menu (mode 13).
void VIWndGame::DrawGroup(VIVect2I at)
{
    VIVect2I q;
    VIVect2I p = at;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    if (m_mode == 13)
        return;
    int glowing = 0;
    for (int i = 0; i < app->GetP2BE4C(); i++)
    {
        if (app->GetGroupMemberId(i) == app->m_selfId)
            continue;
        if (!app->GetGroupMemberActive(i))
            continue;
        VIVect2I start = p;
        float glow = 0.3f;
        q = p + VIVect2I(87, 12);
        int isTarget = app->GetGroupMemberId(i) == app->m_target;
        if (isTarget)
            glow = m_glow1;
        ui.BlitBar("", VIColor32F(), p, (VIUITexId)25, (VIUITexId)26, 174,
                   (VIAlphaBlendMode)3, glow, 0, 1);
        if (isTarget)
            glowing = 1;
        ui.m_font.SetTextColor(VIColor32F(0, 0, 0, glow));
        m_window2.Init(UI(), VIVect2I(), q, 160, 1, 0);
        m_window2.Open(1);
        m_window2.TextBox(m_window2.T(app->GetGroupMemberName(i)), 0, 0, 160, 1,
                          VIColor32F(0, 0, 0, glow), 10);
        if (app->GetGroupMemberHPKnown(i))
        {
            p += VIVect2I(10, 25);
            float hp = app->GetGroupMemberHP(i) / 255.0f;
            DrawGauge(7, 136, p, isTarget, hp);
            p += VIVect2I(65, 2);
            if (app->GroupMemberFlag4(i))
            {
                p.y -= 2;
                ui.BlitEx((VIUITexId)118, p, (VIAlphaBlendMode)3, glow, 0);
                p.x += 25;
                p.y += 2;
            }
            if (app->GroupMemberFlag8(i))
            {
                ui.BlitEx((VIUITexId)119, p, (VIAlphaBlendMode)3, glow, 0);
                p.x += 25;
            }
        }
        p = start + VIVect2I(0, 48);
    }
    m_glow1On = glowing ? 1 : -1;
}

// 0x00C516F0: the pet's bar, under the group: its frame (glowing while it is the target),
// its name, and for a creature pet its health and two status icons.
void VIWndGame::DrawPet(VIVect2I at)
{
    VIVect2I q;
    VIVect2I p = at;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    if (m_mode == 13)
        return;
    if (app->HasPet())
    {
        int one = 1;            // one register for all three 1s (the movz's and BlitBar's)
        int isTarget = 0;
        if (app->m_target == app->m_petId) isTarget = one;
        m_glow2On = isTarget ? one : -1;
        ui.BlitBar("", VIColor32F(), p, (VIUITexId)25, (VIUITexId)26, 174,
                   (VIAlphaBlendMode)3, m_glow2, 0, 1);
        const char *name = app->GetPetName();
        if (name)
        {
            q = p + VIVect2I(87, 12);
            m_window3.Init(UI(), VIVect2I(), q, 160, 1, 0);
            m_window3.Open(1);
            m_window3.TextBox(m_window3.T(name), 0, 0, 160, 1, VIColor32F(0, 0, 0, m_glow2), 10);
        }
        if (!app->PetIsCreature())
            return;
        p += VIVect2I(10, 25);
        float hp;
        app->GetPetHealth(&hp);
        DrawGauge(7, 136, p, isTarget, hp);
        p += VIVect2I(65, 2);
        if (app->PetFlag4())
        {
            p.y -= 2;
            ui.BlitEx((VIUITexId)118, p, (VIAlphaBlendMode)3, m_glow2, 0);
            p.x += 25;
            p.y += 2;
        }
        if (app->PetFlag8())
            ui.BlitEx((VIUITexId)119, p, (VIAlphaBlendMode)3, m_glow2, 0);
    }
    else
    {
        m_glow2On = -1;
        m_glow2 = 0.3f;
    }
}

// 0x00C51A38: the breath bar, under water while nothing else is open (or fishing, or
// the manual camera).
void VIWndGame::DrawStatus(VIVect2I at)
{
    VIVect2I q;
    VIVect2I p = at;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    if (!app->m_walkCam.IsUnderLiquid())
        return;
    if (IsLootable() || m_mode == 36 || m_mode == 38)
    {
        ui.DrawStringEx((unsigned short)911, p.x, p.y, VIColor32F(1, 1, 1, 1), 1);
        p.y += 19;
        DrawGauge(9, 136, p, 1, app->GetPlayerP30() / 255.0f);
    }
}

// 0x00C51B50: the target's bar: its name (scrolling), health, disposition and the
// player's own status icon when the player is targeted.  The name glows while there
// is a target.
void VIWndGame::DrawTarget(VIVect2I at)
{
    VIVect2I p = at;
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    char name[32];
    unsigned short wide[24];
    int r = app->GetTargetName(name);
    m_glow0On = r == 0 ? 1 : -1;
    VIString::str8to16(wide, 48, name);
    ui.DrawStringEx((unsigned short)408, p.x, p.y, VIColor32F(1, 1, 1, m_glow0), 1);
    p.y += 25;
    ui.BlitBar("", VIColor32F(), p, (VIUITexId)25, (VIUITexId)26, 174,
               (VIAlphaBlendMode)3, m_glow0, 0, 1);
    ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, name);
    m_scroll.Init(UI(), p + VIVect2I(87, 12), 160, 0, (VIScrollStyle)1);
    m_scroll.Open(1);
    if (!VIString::strlen16(wide))
        m_scroll.Reset();
    m_scroll.TextScroll(wide, VIColor32F(0, 0, 0, m_glow0), 8);
    p += VIVect2I(10, 25);
    float hp = app->GetTargetP19() / 255.0f;
    DrawGauge(7, 136, p, r == 0, hp);
    p += VIVect2I(65, 2);
    if (app->m_targetInfoValid && !r)
    {
        int icon;
        switch (app->m_targetDisposition)
        {
        case 0:
        case 1:
            icon = 115;
            break;
        case 4:
        case 5:
        case 6:
            icon = 117;
            break;
        case 2:
        case 3:
        default:
            icon = 116;
            break;
        }
        ui.BlitEx((VIUITexId)icon, p, (VIAlphaBlendMode)3, m_glow0, 0);
        p.x += 25;
    }
    if (app->m_selfId == app->m_target && app->GetPlayer()->m_p2C > 0)
    {
        ui.BlitEx((VIUITexId)211, p, (VIAlphaBlendMode)3, m_glow0, 0);
        p.x += 25;
    }
}

// 0x00C51F10: the target's details box (m_window): name, guild, race, class or title,
// level -- a player's (m_p256 2) from the target info when the server sent it, a
// creature's (3) just name and level.
void VIWndGame::DrawTargetInfo()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    unsigned short text[512];
    const VIRaceDef *race;
    const VIClassDef *cls;
    VICreatureEntity *ent = (VICreatureEntity *)app->FindEntity(app->m_target);
    text[0] = 0;
    if (!ent)
        goto close;
    {
        race = GetRace(ent->m_race);
        cls = GetClass(ent->m_class);
        if (!race || !cls)
            return;
        if (ent->m_p256 == 2)
        {
            if (app->m_targetInfoValid)
            {
                unsigned short *guild = app->m_p4FC28;
                int hasGuild = VIString::strlen16(guild) != 0;
                unsigned short *title = app->m_p4FCA8;
                int hasTitle = VIString::strlen16(title) != 0;
                if (hasGuild)
                {
                    if (hasTitle)
                        VIString::snprintf16(text, 1024, "%s\n%S\n%S\n%S\n%S %d", ent->m_name, guild,
                                             ui.m_words.GetString(race->m_nameStrID, ui.m_language), title,
                                             ui.m_words.GetString(503, ui.m_language), ent->m_level);
                    else
                        VIString::snprintf16(text, 1024, "%s\n%S\n%S\n%S\n%S %d", ent->m_name, guild,
                                             ui.m_words.GetString(race->m_nameStrID, ui.m_language),
                                             ui.m_words.GetString(cls->m_nameStrID, ui.m_language),
                                             ui.m_words.GetString(503, ui.m_language), ent->m_level);
                    goto draw;
                }
                if (hasTitle)
                {
                    VIString::snprintf16(text, 1024, "%s\n%S\n%S\n%S %d", ent->m_name,
                                         ui.m_words.GetString(race->m_nameStrID, ui.m_language), title,
                                         ui.m_words.GetString(503, ui.m_language), ent->m_level);
                    goto draw;
                }
            }
            VIString::snprintf16(text, 1024, "%s\n%S\n%S\n%S %d", ent->m_name,
                                 ui.m_words.GetString(race->m_nameStrID, ui.m_language),
                                 ui.m_words.GetString(cls->m_nameStrID, ui.m_language),
                                 ui.m_words.GetString(503, ui.m_language), ent->m_level);
        }
        else if (ent->m_p256 == 3)
            VIString::snprintf16(text, 1024, "%s\n%S %d", ent->m_name,
                                 ui.m_words.GetString(503, ui.m_language), ent->m_level);
        else
        {
        close:
            m_window.Close(1);
            return;
        }
    draw:
        VIUIWindow &w = m_window;
        int lines = w.CountLines(text, 155);
        w.Init(UI(), VIVect2I(0, 0), VIVect2I(500, 235), 195, lines, 1);
        if (!VIString::strlen16(text))
            w.Close(1);
        w.Draw();
        w.TextBox(text, 20, 0, w.m_size.x - 40, lines, VIColor32F(1, 1, 1, w.m_open), 3);
    }
}

// 0x00C522A0 (VIClient, every frame): the start menu's item menus and the HUD's ability
// and quick-slot menus, rebuilt from the inventory and the ability list.
void VIWndGame::UpdateQuickBars()
{
    BuildInvMenu();
    BuildEquipMenu();
    BuildMenu0300();
    BuildMenu0310();
    BuildAbilityMenu0();
    BuildAbilityMenu1();
    BuildQuickMenu();
}

// 0x00C522F8: the inventory menu (0x00CD02E0): every item the info text accepts, then the
// rest of the slots emptied.
void VIWndGame::BuildInvMenu()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIInfoText info;
    info.SetClient(app);
    for (; i < gInvMenu.m_count; i++)
    {
        gInvMenu.m_entries[i].m_icon = -1;
        gInvMenu.m_entries[i].m_icon2 = -1;
        gInvMenu.m_entries[i].m_x08 = -1;
        gInvMenu.m_entries[i].m_number = -1;
        gInvMenu.m_entries[i].m_word = -1;
        gInvMenu.m_entries[i].m_sub = 0;
        gInvMenu.m_entries[i].m_msg = 0;
        gInvMenu.m_entries[i].m_id = -1;
    }
    VIItemList &inv = app->m_inventory;
    int k;
    for (k = 0, i = 0; k < app->GetInvCount(); k++)
        if (info.IsCharged(inv.Item(k)))
        {
            SetInvMenuItem(k, i);
            i++;
        }
    for (; i < gInvMenu.m_count; i++)
        SetInvMenuItem(-1, i);
}

// 0x00C52498: one inventory menu slot -- the item's icon and count, flagged when its
// charges are under a quarter, and a three-entry sub-menu (words 122, 200, 281).
int VIWndGame::SetInvMenuItem(int inv, int slot)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (inv < 0)
    {
        gInvMenu.m_entries[slot].m_icon = -1;
        gInvMenu.m_entries[slot].m_icon2 = -1;
        gInvMenu.m_entries[slot].m_x08 = -1;
        gInvMenu.m_entries[slot].m_number = -1;
        gInvMenu.m_entries[slot].m_word = -1;
        gInvMenu.m_entries[slot].m_sub = 0;
        gInvMenu.m_entries[slot].m_msg = 0;
        gInvMenu.m_entries[slot].m_id = -1;
    }
    else
    {
    gInvMenu.m_entries[slot].m_icon = app->GetInvItemIcon(inv);
    gInvMenu.m_entries[slot].m_number = app->GetInvItemAmount(inv);
    gInvMenu.m_entries[slot].m_x08 = -1;
    gInvMenu.m_entries[slot].m_word = -1;
    gInvMenu.m_entries[slot].m_sub = &gInvSubMenus[slot];
    gInvMenu.m_entries[slot].m_msg = 0;
    gInvMenu.m_entries[slot].m_id = inv;
    if (app->GetInvItemMaxHp(inv) > 0)
    {
        float f = app->GetInvItemCurHp(inv);
        f /= app->GetInvItemMaxHp(inv);
        if (f < 0.25f)
            gInvMenu.m_entries[slot].m_x08 = 1;
    }
    gInvSubMenus[slot].m_x00 = -1;
    gInvSubMenus[slot].m_x04 = -1;
    gInvSubMenus[slot].m_count = 3;
    gInvSubMenus[slot].m_entries = gInvSubItems[slot];
    gInvSubMenus[slot].m_entries[0].m_word = 122;
    gInvSubMenus[slot].m_entries[0].m_msg = 0x2020000;
    gInvSubMenus[slot].m_entries[1].m_word = 200;
    gInvSubMenus[slot].m_entries[1].m_msg = 0x2050000;
    gInvSubMenus[slot].m_entries[2].m_word = 281;
    gInvSubMenus[slot].m_entries[2].m_msg = 0x2060000;
    for (int j = 0; j < 3; j++)
    {
        gInvSubMenus[slot].m_entries[j].m_sub = 0;
        gInvSubMenus[slot].m_entries[j].m_icon = -1;
        gInvSubMenus[slot].m_entries[j].m_number = -1;
        gInvSubMenus[slot].m_entries[j].m_id = inv;
    }
    }
    return 0;
}

// 0x00C52748: the equipment menu (00CD02F0): every item the info text accepts, then the rest of the
// slots emptied.
void VIWndGame::BuildEquipMenu()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIInfoText info;
    info.SetClient(app);
    for (; i < gEquipMenu.m_count; i++)
    {
        gEquipMenu.m_entries[i].m_icon = -1;
        gEquipMenu.m_entries[i].m_icon2 = -1;
        gEquipMenu.m_entries[i].m_x08 = -1;
        gEquipMenu.m_entries[i].m_number = -1;
        gEquipMenu.m_entries[i].m_word = -1;
        gEquipMenu.m_entries[i].m_sub = 0;
        gEquipMenu.m_entries[i].m_msg = 0;
        gEquipMenu.m_entries[i].m_id = -1;
    }
    VIItemList &inv = app->m_inventory;
    int k;
    for (k = 0, i = 0; k < app->GetInvCount(); k++)
        if (info.IsMisc(inv.Item(k)))
        {
            SetEquipMenuItem(k, i);
            i++;
        }
    for (; i < gEquipMenu.m_count; i++)
        SetEquipMenuItem(-1, i);
}

// 0x00C528E8: one equipment menu slot -- the item's icon and count and a three-entry
// sub-menu (words 122, 200, 281).  Its equip and wear slots are read and not used.
int VIWndGame::SetEquipMenuItem(int inv, int slot)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (inv < 0)
    {
        gEquipMenu.m_entries[slot].m_icon = -1;
        gEquipMenu.m_entries[slot].m_icon2 = -1;
        gEquipMenu.m_entries[slot].m_x08 = -1;
        gEquipMenu.m_entries[slot].m_number = -1;
        gEquipMenu.m_entries[slot].m_word = -1;
        gEquipMenu.m_entries[slot].m_sub = 0;
        gEquipMenu.m_entries[slot].m_msg = 0;
        gEquipMenu.m_entries[slot].m_id = -1;
    }
    else
    {
        gEquipMenu.m_entries[slot].m_icon = app->GetInvItemIcon(inv);
        gEquipMenu.m_entries[slot].m_number = app->GetInvItemAmount(inv);
        gEquipMenu.m_entries[slot].m_x08 = -1;
        gEquipMenu.m_entries[slot].m_word = -1;
        gEquipMenu.m_entries[slot].m_sub = &gEquipSubMenus[slot];
        gEquipMenu.m_entries[slot].m_msg = 0;
        gEquipMenu.m_entries[slot].m_id = inv;
        gEquipSubMenus[slot].m_x00 = -1;
        gEquipSubMenus[slot].m_x04 = -1;
        gEquipSubMenus[slot].m_count = 3;
        gEquipSubMenus[slot].m_entries = gEquipSubItems[slot];
        int equip = app->GetInvItemEquipSlot(inv);
        int wear = app->GetInvItemWearSlot(inv);
        gEquipSubMenus[slot].m_entries[0].m_word = 122;
        gEquipSubMenus[slot].m_entries[0].m_msg = 0x2020000;
        gEquipSubMenus[slot].m_entries[1].m_word = 200;
        gEquipSubMenus[slot].m_entries[1].m_msg = 0x2050000;
        gEquipSubMenus[slot].m_entries[2].m_word = 281;
        gEquipSubMenus[slot].m_entries[2].m_msg = 0x2060000;
        for (int j = 0; j < 3; j++)
        {
            gEquipSubMenus[slot].m_entries[j].m_sub = 0;
            gEquipSubMenus[slot].m_entries[j].m_icon = -1;
            gEquipSubMenus[slot].m_entries[j].m_number = -1;
            gEquipSubMenus[slot].m_entries[j].m_id = inv;
        }
    }
    return 0;
}

// 0x00C52B28: the start menu's third item menu (00CD0300): every item the info text accepts, then the rest of the
// slots emptied.
void VIWndGame::BuildMenu0300()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIInfoText info;
    info.SetClient(app);
    for (; i < gItemMenu3.m_count; i++)
    {
        gItemMenu3.m_entries[i].m_icon = -1;
        gItemMenu3.m_entries[i].m_icon2 = -1;
        gItemMenu3.m_entries[i].m_x08 = -1;
        gItemMenu3.m_entries[i].m_number = -1;
        gItemMenu3.m_entries[i].m_word = -1;
        gItemMenu3.m_entries[i].m_sub = 0;
        gItemMenu3.m_entries[i].m_msg = 0;
        gItemMenu3.m_entries[i].m_id = -1;
    }
    VIItemList &inv = app->m_inventory;
    int k;
    for (k = 0, i = 0; k < app->GetInvCount(); k++)
        if (info.IsWearable(inv.Item(k)))
        {
            SetMenu0300Item(k, i);
            i++;
        }
    for (; i < gItemMenu3.m_count; i++)
        SetMenu0300Item(-1, i);
}

// 0x00C52CC8: one slot of 0x00CD0300's menu -- flagged when the item has no charges or is
// under a quarter, badged 138 when it is worn and 140 when it cannot be used, and a
// five-entry sub-menu whose first entry is word 121 or (worn) 154.
int VIWndGame::SetMenu0300Item(int inv, int slot)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (inv < 0)
    {
        gItemMenu3.m_entries[slot].m_icon = -1;
        gItemMenu3.m_entries[slot].m_icon2 = -1;
        gItemMenu3.m_entries[slot].m_x08 = -1;
        gItemMenu3.m_entries[slot].m_number = -1;
        gItemMenu3.m_entries[slot].m_word = -1;
        gItemMenu3.m_entries[slot].m_sub = 0;
        gItemMenu3.m_entries[slot].m_msg = 0;
        gItemMenu3.m_entries[slot].m_id = -1;
    }
    else
    {
        gItemMenu3.m_entries[slot].m_icon = app->GetInvItemIcon(inv);
        gItemMenu3.m_entries[slot].m_number = app->GetInvItemAmount(inv);
        gItemMenu3.m_entries[slot].m_word = -1;
        gItemMenu3.m_entries[slot].m_sub = &gItemMenu3SubMenus[slot];
        gItemMenu3.m_entries[slot].m_id = inv;
        if (app->GetInvItemMaxHp(inv) > 0)
        {
            float f = app->GetInvItemCurHp(inv);
            f /= app->GetInvItemMaxHp(inv);
            if (f < 0.25f)
                gItemMenu3.m_entries[slot].m_x08 = 1;
        }
        else
            gItemMenu3.m_entries[slot].m_x08 = 1;
        gItemMenu3SubMenus[slot].m_x00 = -1;
        gItemMenu3SubMenus[slot].m_x04 = -1;
        gItemMenu3SubMenus[slot].m_count = 5;
        gItemMenu3SubMenus[slot].m_entries = gItemMenu3SubItems[slot];
        if (app->GetInvItemEquipSlot(inv) == -1)
        {
            gItemMenu3.m_entries[slot].m_icon2 = -1;
            gItemMenu3SubMenus[slot].m_entries[0].m_word = 121;
        }
        else
        {
            gItemMenu3.m_entries[slot].m_icon2 = 138;
            gItemMenu3SubMenus[slot].m_entries[0].m_word = 154;
        }
        if (!app->CanUseListItem(inv, 0))
        {
            gItemMenu3.m_entries[slot].m_icon2 = 140;
            gItemMenu3SubMenus[slot].m_entries[0].m_word = 121;
        }
        gItemMenu3SubMenus[slot].m_entries[0].m_msg = 0x2030000;
        gItemMenu3SubMenus[slot].m_entries[1].m_word = 122;
        gItemMenu3SubMenus[slot].m_entries[1].m_msg = 0x2020000;
        gItemMenu3SubMenus[slot].m_entries[2].m_word = 433;
        gItemMenu3SubMenus[slot].m_entries[2].m_msg = 0x2070000;
        gItemMenu3SubMenus[slot].m_entries[3].m_word = 200;
        gItemMenu3SubMenus[slot].m_entries[3].m_msg = 0x2050000;
        gItemMenu3SubMenus[slot].m_entries[4].m_word = 281;
        gItemMenu3SubMenus[slot].m_entries[4].m_msg = 0x2060000;
        for (int j = 0; j < gItemMenu3SubMenus[slot].m_count; j++)
        {
            gItemMenu3SubMenus[slot].m_entries[j].m_sub = 0;
            gItemMenu3SubMenus[slot].m_entries[j].m_icon = -1;
            gItemMenu3SubMenus[slot].m_entries[j].m_icon2 = -1;
            gItemMenu3SubMenus[slot].m_entries[j].m_number = -1;
            gItemMenu3SubMenus[slot].m_entries[j].m_id = inv;
        }
    }
    return 0;
}

// 0x00C53080: the start menu's fourth item menu (00CD0310): every item the info text accepts, then the rest of the
// slots emptied.
void VIWndGame::BuildMenu0310()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIInfoText info;
    info.SetClient(app);
    for (; i < gItemMenu4.m_count; i++)
    {
        gItemMenu4.m_entries[i].m_icon = -1;
        gItemMenu4.m_entries[i].m_icon2 = -1;
        gItemMenu4.m_entries[i].m_x08 = -1;
        gItemMenu4.m_entries[i].m_number = -1;
        gItemMenu4.m_entries[i].m_word = -1;
        gItemMenu4.m_entries[i].m_sub = 0;
        gItemMenu4.m_entries[i].m_msg = 0;
        gItemMenu4.m_entries[i].m_id = -1;
    }
    VIItemList &inv = app->m_inventory;
    int k;
    for (k = 0, i = 0; k < app->GetInvCount(); k++)
        if (info.IsUsable(inv.Item(k)))
        {
            SetMenu0310Item(k, i);
            i++;
        }
    for (; i < gItemMenu4.m_count; i++)
        SetMenu0310Item(-1, i);
}

// 0x00C53220: one slot of 0x00CD0310's menu -- a sub-menu of four entries when the
// item's +0x4C is 5 and of five otherwise (433 is the one the four leave out).
int VIWndGame::SetMenu0310Item(int inv, int slot)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (inv < 0)
    {
        gItemMenu4.m_entries[slot].m_icon = -1;
        gItemMenu4.m_entries[slot].m_icon2 = -1;
        gItemMenu4.m_entries[slot].m_x08 = -1;
        gItemMenu4.m_entries[slot].m_number = -1;
        gItemMenu4.m_entries[slot].m_word = -1;
        gItemMenu4.m_entries[slot].m_sub = 0;
        gItemMenu4.m_entries[slot].m_msg = 0;
        gItemMenu4.m_entries[slot].m_id = -1;
    }
    else
    {
        gItemMenu4.m_entries[slot].m_icon = app->GetInvItemIcon(inv);
        gItemMenu4.m_entries[slot].m_number = app->GetInvItemAmount(inv);
        gItemMenu4.m_entries[slot].m_x08 = -1;
        gItemMenu4.m_entries[slot].m_word = -1;
        gItemMenu4.m_entries[slot].m_sub = &gItemMenu4SubMenus[slot];
        gItemMenu4.m_entries[slot].m_id = inv;
        int five = app->GetInvItemP4C(inv) == 5;
        gItemMenu4SubMenus[slot].m_x00 = -1;
        gItemMenu4SubMenus[slot].m_x04 = -1;
        gItemMenu4SubMenus[slot].m_count = five ? 4 : 5;
        gItemMenu4SubMenus[slot].m_entries = gItemMenu4SubItems[slot];
        gItemMenu4SubMenus[slot].m_entries[0].m_word = 120;
        gItemMenu4SubMenus[slot].m_entries[0].m_msg = 0x2030000;
        gItemMenu4SubMenus[slot].m_entries[1].m_word = 122;
        gItemMenu4SubMenus[slot].m_entries[1].m_msg = 0x2020000;
        if (five)
        {
            gItemMenu4SubMenus[slot].m_entries[2].m_word = 200;
            gItemMenu4SubMenus[slot].m_entries[2].m_msg = 0x2050000;
            gItemMenu4SubMenus[slot].m_entries[3].m_word = 281;
            gItemMenu4SubMenus[slot].m_entries[3].m_msg = 0x2060000;
            gItemMenu4SubMenus[slot].m_entries[4].m_word = -1;
            gItemMenu4SubMenus[slot].m_entries[4].m_msg = 0;
        }
        else
        {
            gItemMenu4SubMenus[slot].m_entries[2].m_word = 433;
            gItemMenu4SubMenus[slot].m_entries[2].m_msg = 0x2070000;
            gItemMenu4SubMenus[slot].m_entries[3].m_word = 200;
            gItemMenu4SubMenus[slot].m_entries[3].m_msg = 0x2050000;
            gItemMenu4SubMenus[slot].m_entries[4].m_word = 281;
            gItemMenu4SubMenus[slot].m_entries[4].m_msg = 0x2060000;
        }
        for (int j = 0; j < gItemMenu4SubMenus[slot].m_count; j++)
        {
            gItemMenu4SubMenus[slot].m_entries[j].m_sub = 0;
            gItemMenu4SubMenus[slot].m_entries[j].m_icon = -1;
            gItemMenu4SubMenus[slot].m_entries[j].m_icon2 = -1;
            gItemMenu4SubMenus[slot].m_entries[j].m_number = -1;
            gItemMenu4SubMenus[slot].m_entries[j].m_id = inv;
        }
    }
    return 0;
}

// 0x00C534F8: the first HUD menu (0x00CD02B0, nine slots): the abilities in slots 0..4
// where they sit, the ones in 252..255 packed from slot 5 on, the empty slots cleared.
void VIWndGame::BuildAbilityMenu0()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIArray<int> list;
    app->GetAbilityList(list);
    for (; i < 9; i++)
    {
        gHudMenu0.m_entries[i].m_icon = -1;
        gHudMenu0.m_entries[i].m_icon2 = -1;
        gHudMenu0.m_entries[i].m_x08 = -1;
        gHudMenu0.m_entries[i].m_msg = 0;
        gHudMenu0.m_entries[i].m_id = 0;
    }
    int n2 = 0;
    int n1 = 0;
    for (i = 0; i < list.m_size; i++)
    {
        if (app->GetAbilityValid(list[i]))
        {
            int slot = app->GetAbilitySlot(list[i]);
            if (slot >= 9)
            {
                if (slot >= 256)
                    continue;
                if (slot < 252)
                    continue;
                slot = n2 + 5;
                n2++;
            }
            else
            {
                if (slot >= 5)
                    continue;
                n1++;
            }
            SetAbilityMenu0Item(list[i], slot);
        }
    }
    for (i = 0; i < 9; i++)
        if (gHudMenu0.m_entries[i].m_icon2 == -1)
            SetAbilityMenu0Item(-1, i);
    n1 = VIMIN(5, VIMAX(0, n1));
    if (n2 > 0)
        gHudMenu0.m_count = VIMIN(9, n2 + 5);
    else
        gHudMenu0.m_count = VIMAX(5, n1);
}

// 0x00C53718: one slot of the first HUD menu -- the ability's icon, badged 61 while it is
// locked.
int VIWndGame::SetAbilityMenu0Item(int ability, int slot)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (slot < 0 || slot >= 9)
        return -1;
    if (ability == -1)
    {
        gHudMenu0.m_entries[slot].m_icon = -1;
        gHudMenu0.m_entries[slot].m_icon2 = -1;
        gHudMenu0.m_entries[slot].m_x08 = -1;
        gHudMenu0.m_entries[slot].m_number = -1;
        gHudMenu0.m_entries[slot].m_word = -1;
        gHudMenu0.m_entries[slot].m_sub = 0;
        gHudMenu0.m_entries[slot].m_msg = 0;
        gHudMenu0.m_entries[slot].m_id = 0;
    }
    else
    {
        gHudMenu0.m_entries[slot].m_icon = -1;
        gHudMenu0.m_entries[slot].m_icon2 = app->GetAbilityIconFront(ability);
        gHudMenu0.m_entries[slot].m_x08 = app->IsAbilityLocked(ability) > 0 ? 61 : -1;
        gHudMenu0.m_entries[slot].m_number = -1;
        gHudMenu0.m_entries[slot].m_word = -1;
        gHudMenu0.m_entries[slot].m_sub = 0;
        gHudMenu0.m_entries[slot].m_msg = 0x8060000;
        gHudMenu0.m_entries[slot].m_id = ability;
    }
    return 0;
}

// 0x00C53888: the second HUD menu (0x00CD02C0, five slots): the abilities in slots 5..9.
void VIWndGame::BuildAbilityMenu1()
{
    int i = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    VIArray<int> list;
    app->GetAbilityList(list);
    for (; i < 5; i++)
    {
        gHudMenu1.m_entries[i].m_icon = -1;
        gHudMenu1.m_entries[i].m_icon2 = -1;
        gHudMenu1.m_entries[i].m_x08 = -1;
        gHudMenu1.m_entries[i].m_msg = 0;
        gHudMenu1.m_entries[i].m_id = 0;
    }
    int n = 0;
    for (i = 0; i < list.m_size; i++)
    {
        if (app->GetAbilityValid(list[i]))
        {
            int slot = app->GetAbilitySlot(list[i]);
            if (slot >= 5 && slot < 10)
            {
                n++;
                SetAbilityMenu1Item(list[i], slot - 5);
            }
        }
    }
    for (i = 0; i < 5; i++)
        if (gHudMenu1.m_entries[i].m_icon2 == -1)
            SetAbilityMenu1Item(-1, i);
    n = VIMIN(5, VIMAX(0, n));
    gHudMenu1.m_count = VIMAX(5, n);
}

// 0x00C53A48: one slot of the second HUD menu.
int VIWndGame::SetAbilityMenu1Item(int ability, int slot)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (slot < 0 || slot >= 5)
        return -1;
    if (ability == -1)
    {
        gHudMenu1.m_entries[slot].m_icon = -1;
        gHudMenu1.m_entries[slot].m_icon2 = -1;
        gHudMenu1.m_entries[slot].m_x08 = -1;
        gHudMenu1.m_entries[slot].m_number = -1;
        gHudMenu1.m_entries[slot].m_word = -1;
        gHudMenu1.m_entries[slot].m_sub = 0;
        gHudMenu1.m_entries[slot].m_msg = 0;
        gHudMenu1.m_entries[slot].m_id = 0;
    }
    else
    {
        int icon = app->GetAbilityIconFront(ability);
        int locked = app->IsAbilityLocked(ability);
        gHudMenu1.m_entries[slot].m_icon = -1;
        gHudMenu1.m_entries[slot].m_icon2 = icon;
        gHudMenu1.m_entries[slot].m_x08 = locked > 0 ? 61 : -1;
        gHudMenu1.m_entries[slot].m_number = -1;
        gHudMenu1.m_entries[slot].m_word = -1;
        gHudMenu1.m_entries[slot].m_sub = 0;
        gHudMenu1.m_entries[slot].m_msg = 0x8060000;
        gHudMenu1.m_entries[slot].m_id = ability;
    }
    return 0;
}

// 0x00C53BB8: the third HUD menu (0x00CD02D0, four slots): the items on a quick slot.
void VIWndGame::BuildQuickMenu()
{
    VIClient *app = (VIClient *)GetAppPtr();
    for (int i = 0; i < 4; i++)
    {
        gHudMenu2.m_entries[i].m_icon = -1;
        gHudMenu2.m_entries[i].m_icon2 = -1;
        gHudMenu2.m_entries[i].m_x08 = -1;
        gHudMenu2.m_entries[i].m_number = -1;
        gHudMenu2.m_entries[i].m_word = -1;
        gHudMenu2.m_entries[i].m_sub = 0;
        gHudMenu2.m_entries[i].m_msg = 0;
        gHudMenu2.m_entries[i].m_id = 0;
    }
    for (int k = 0; k < app->GetInvCount(); k++)
    {
        int q = app->GetInvItemQuickSlot(k);
        if (q >= 0)
            SetQuickMenuItem(k, q);
    }
}

// 0x00C53CB8: one quick slot -- the item's count.
int VIWndGame::SetQuickMenuItem(int inv, int slot)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (slot >= 4)
        return -1;
    if (inv < 0)
    {
        gHudMenu2.m_entries[slot].m_icon = -1;
        gHudMenu2.m_entries[slot].m_icon2 = -1;
        gHudMenu2.m_entries[slot].m_x08 = -1;
        gHudMenu2.m_entries[slot].m_number = -1;
        gHudMenu2.m_entries[slot].m_word = -1;
        gHudMenu2.m_entries[slot].m_sub = 0;
        gHudMenu2.m_entries[slot].m_msg = 0;
        gHudMenu2.m_entries[slot].m_id = -1;
    }
    else
    {
        gHudMenu2.m_entries[slot].m_icon = -1;
        gHudMenu2.m_entries[slot].m_x08 = -1;
        gHudMenu2.m_entries[slot].m_number = app->GetInvItemAmount(inv);
        gHudMenu2.m_entries[slot].m_word = -1;
        gHudMenu2.m_entries[slot].m_sub = 0;
        gHudMenu2.m_entries[slot].m_msg = 0x2030000;
        gHudMenu2.m_entries[slot].m_id = inv;
    }
    return 0;
}

// 0x00C53DF8: the game's keys.  With the HUD down only the four pad keys 134, 140, 142 and
// 144 get through.
void VIWndGame::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (!m_p05C && key != 144 && key != 134 && key != 140 && key != 142)
        return;
    switch (key)
    {
    case 27:
    case 144:
        if (m_mode == 30 && key == 27)
        {
            m_menu4->SetVisible(false);
            m_mode = 0;
            UpdateMode();
        }
        else if (m_mode == 0)
        {
            if (m_startMenu->GetVisible())
                CloseStartMenu(0);
            else
                OpenStartMenu(1);
        }
        break;
    case 80:
    case 112:
    case 145:
        if (m_mode == 0)
            app->InitCameraStyle();
        break;
    case 32:
    case 134:
        if (m_mode == 0)
            app->ToggleAutoAttack();
        break;
    case 81:
    case 113:
    case 133:
        if (m_mode == 0)
            app->InteractWithTarget();
        break;
    case 132:
        if (m_mode == 30)
        {
            m_menu4->SetVisible(false);
            m_mode = 0;
            UpdateMode();
        }
        else if (m_mode == 0)
            app->ClearTarget();
        break;
    case 96:
    case 126:
    case 140:
        if (m_mode == 0)
        {
            app->CycleTarget(1);
            m_p03C = 1;
            m_p040 = app->m_p07568;
        }
        break;
    case 9:
    case 142:
        if (m_mode == 0)
        {
            app->CycleTarget(0);
            m_p03C = 0;
            m_p040 = app->m_p07568;
        }
        break;
    case 68:
    case 100:
    case 137:
        Interact();
        break;
    case 65:
    case 97:
    case 136:
        Attack();
        break;
    case 47:
        if (m_mode == 38)
            app->ResetView();
        m_edit->Clear();
        m_edit->SetMaxLength(128);
        SetEditTitle(1);
        m_edit->SetText("/");
        m_edit->m_x24 = 0xB0F0000;
        m_mode = 2;
        UpdateMode();
        break;
    case 13:
    case 147:
        if (m_mode == 38)
            app->ResetView();
        m_edit->Clear();
        m_edit->SetMaxLength(128);
        SetEditTitle(1);
        m_edit->m_x24 = 0xB0F0000;
        m_mode = 2;
        UpdateMode();
        break;
    case 88:
    case 120:
        if (m_mode == 0)
            app->ToggleP74();
        break;
    case 146:
        if (m_mode == 0 || (m_mode != 30 && ModeHasChat()))
            app->ToggleP74();
        break;
    case 90:
    case 122:
    case 141:
        if (m_mode == 0)
        {
            if (m_menu4->GetVisible())
                break;
            m_influence->SetVisible(false);
            m_menu4->SetSet(VITextSet::GetRoot());
            m_menu4->SetVisible(true);
            m_mode = 30;
            app->SetActiveWindow(m_menu4);
        }
        else if (m_mode == 30)
        {
            m_menu4->SetVisible(false);
            m_mode = 0;
            UpdateMode();
        }
        break;
    case 67:
    case 99:
    case 143:
        if (m_mode == 0)
        {
            m_mode = 33;
            UpdateMode();
            app->SetP06DC0(1, 1);
        }
        break;
    case 82:
    case 114:
    {
        if (!VIString::strlen16(app->m_p4F0B8))
            break;
        if (m_mode == 38)
            app->ResetView();
        unsigned short buf[16];
        VIString::snprintf16(buf, sizeof(buf), "/reply ");
        m_edit->Clear();
        m_edit->SetMaxLength(128);
        SetEditTitle(0);
        m_edit->SetText(buf);
        m_edit->m_x24 = 0xB0F0000;
        m_mode = 2;
        UpdateMode();
        break;
    }
    case 84:
    case 116:
    {
        if (m_mode == 38)
            app->ResetView();
        unsigned short buf[16];
        VIString::snprintf16(buf, sizeof(buf), "/tell ");
        m_edit->Clear();
        m_edit->SetMaxLength(128);
        SetEditTitle(0);
        m_edit->SetText(buf);
        m_edit->m_x24 = 0xB0F0000;
        m_mode = 2;
        UpdateMode();
        break;
    }
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
        app->SelectTarget(key - 48);
        break;
    default:
        VIWnd::OnKeyDown(key);
        break;
    }
}

// 0x00C54250
void VIWndGame::OnKeyUp(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 96:
    case 126:
    case 140:
        if (m_mode == 38)
            app->ResetView();
        else if (m_mode == 0)
            m_p03C = -1;
        break;
    case 9:
    case 142:
        if (m_mode == 38)
            app->ResetView();
        else if (m_mode == 0)
            m_p03C = -1;
        break;
    case 32:
    case 134:
        app->MaybeStopAutoAttack();
        break;
    case 81:
    case 113:
    case 133:
        m_p678 = 0;
        break;
    case 67:
    case 99:
    case 143:
        if (m_mode == 33)
        {
            m_chat->SetScroll(0);
            m_mode = 0;
            UpdateMode();
        }
        break;
    default:
        VIWnd::OnKeyUp(key);
        break;
    }
}




































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































// The words table lookup the guild answers make (an inline: its `this` is a fresh copy
// of &app->m_ui, not OnMessage's own reference).
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C543F8: the game window's messages -- the panels' answers (258..305) and, for 261,
// the start menu's commands.  The guild's errors assert (VIWndGame.cpp:3924..4007).
void VIWndGame::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    unsigned short buf[64];
    char name[24];
    unsigned short text1[32];
    unsigned short text2[256];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    VIClientGuild *guild = app->GetGuild();
    int keep = 0;
    VIClientPart2EF24 *p2ef24 = app->GetP2EF24();
    VIClientPart38300 *p38300 = app->GetP38300();
    VIPlayer *player = app->GetPlayer();
    VIClientPart382E8 *p382e8 = app->GetP382E8();
    switch (msg)
    {
    case 304:
        CloseBuddyList();
        VIString::snprintf16(buf, 128, "/t %s ", app->GetBuddyName(p1));
        m_edit->SetMaxLength(128);
        m_edit->SetText(buf);
        m_edit->m_x24 = 0xB0F0000;
        SetEditTitle(0);
        CloseStartMenu(2);
        break;
    case 305:
        CloseGuildMenu();
        VIString::snprintf16(buf, 128, "/t %s ", app->GetGuild()->GetMemberName(p1));
        m_edit->SetMaxLength(128);
        m_edit->SetText(buf);
        m_edit->m_x24 = 0xB0F0000;
        SetEditTitle(0);
        CloseStartMenu(2);
        break;
    case 276:
        m_mode = 0;
        m_response->SetVisible(false);
        UpdateMode();
        break;
    case 279:
        m_mode = 0;
        m_privChat->SetVisible(false);
        UpdateMode();
        break;
    case 258:
        if (m_mode == 30)
        {
            m_menu4->SetVisible(false);
            m_mode = 0;
            if (p1 != -1)
                app->DoAction(p1, p2);
            UpdateMode();
        }
        break;
    case 260:
        if (w == m_mbox)
        {
            switch (p1)
            {
            case 0:
                switch (p2)
                {
                case 0x50E0000:
                    break;
                case 0x7020000:
                    app->DeclineInvite();
                    break;
                }
                break;
            case 2:
                app->SendD4Off();
                break;
            case 3:
                app->Send0701b();
                break;
            }
            w->SetVisible(false);
            m_chat->SetMode(0);
            m_mode = 0;
            UpdateMode();
        }
        else if (w == m_spellBook || w == m_trainer)
        {
            OpenStartMenu(0);
        }
        else if (w == m_startMenu)
        {
            CloseStartMenu(0);
        }
        else if (w == m_trade)
        {
            Trade16();
        }
        else if (w == m_blacksmith)
        {
            app->Send0063();
            Msg0063();
        }
        else
        {
            w->SetVisible(false);
            m_chat->SetMode(0);
            m_mode = 0;
            UpdateMode();
        }
        break;
    case 259:
        if (VIString::strlen16(m_edit->m_text))
        {
            if (app->GetP382E8()->IsCommand(p1))
            {
                app->GetP382E8()->DoText(p1, m_edit->m_text);
                m_edit->SetTitle("");
            }
            else
            {
                switch (p1)
                {
                case 0xB051009:
                    p2ef24->Create(m_edit->m_text);
                    break;
                case 0xB05100B:
                    p2ef24->SetMotd(m_edit->m_text);
                    break;
                case 0xB051006:
                    p2ef24->Say(m_edit->m_text);
                    break;
                case 0xB051000:
                    m_edit->SetTitle("");
                    VIString::str16to8(name, 24, m_edit->m_text);
                    p2ef24->Invite(name);
                    break;
                case 0xB040000:
                    m_edit->SetTitle("");
                    VIString::str16to8(name, 24, m_edit->m_text);
                    app->AddBuddy(name);
                    break;
                case 0xB053001:
                    m_edit->SetTitle("");
                    VIString::str16to8(name, 24, m_edit->m_text);
                    p38300->Add(name);
                    break;
                case 0xB053002:
                    m_edit->SetTitle("");
                    VIString::str16to8(name, 24, m_edit->m_text);
                    p38300->Remove(name);
                    break;
                case 0xB0F0000:
                    app->ChatLine(m_edit->m_text);
                    break;
                case 0xB0E000A:
                    app->m_alarm.Command(app, m_edit->m_text);
                    break;
                case 0xB0B1002:
                    PromptWord(1472, 1, 0, 0xB0B1002, 1);
                    keep = 1;
                    break;
                }
            }
        }
        if (!keep)
        {
            m_edit->SetVisible(false);
            m_edit->ClearX24();
            m_mode = 0;
            UpdateMode();
        }
        break;
    case 288:
        m_chat->SetMode(p1);
        break;
    case 268:
        switch (p1)
        {
        case 0:
            switch (p2)
            {
            case 0x3010000:
                break;
            case 0xB051007:
                p2ef24->Disband();
                break;
            case 0xB051008:
                p2ef24->Quit();
                break;
            case 0x7020000:
                app->AcceptInvite();
                break;
            case 0x50E0000:
                app->Send0041();
                break;
            case 0xA010000:
                app->DoAction(0xA010000, 0);
                break;
            case 0xB0B1002:
                m_edit->SetTitle("");
                VIString::str16to8(name, 24, m_edit->m_text);
                app->SendReport(name);
                break;
            }
            break;
        case 2:
            app->SendD4On();
            break;
        case 3:
            app->Send0701a();
            break;
        default:
            return;
        }
        m_mbox->SetVisible(false);
        m_mode = 0;
        UpdateMode();
        break;
    case 272:
        LootOption(912);
        app->SetActiveWindow(m_spellBook);
        break;
    case 273:
        app->Send0033(p1, p2);
        break;
    case 261:
        if (m_mode == 13)
        {
            CloseStartMenu(0);
            if (app->GetP382E8()->IsCommand(p1))
                app->GetP382E8()->Show(p1, p2);
            else
            {
                switch (p1)
                {
                case 0xA010000:
                    PromptWord(722, 1, 0, 0xA010000, 1);
                    break;
                case 0xB053001:
                    app->GetTargetPlayerName(name);
                    m_edit->Clear();
                    m_edit->SetMaxLength(24);
                    m_edit->SetText(name);
                    m_edit->SetTitle((const unsigned short *)ui.m_words.GetString(67, ui.m_language));
                    m_edit->m_x24 = p1;
                    m_mode = 2;
                    break;
                case 0xB053002:
                    app->GetTargetPlayerName(name);
                    m_edit->Clear();
                    m_edit->SetMaxLength(24);
                    m_edit->SetText(name);
                    m_edit->SetTitle((const unsigned short *)ui.m_words.GetString(613, ui.m_language));
                    m_edit->m_x24 = p1;
                    m_mode = 2;
                    break;
                case 0x8010000:
                    m_mode = 5;
                    break;
                case 0x3010000:
                    m_edit->Clear();
                    m_edit->SetMaxLength(128);
                    m_edit->m_x24 = 0xB0F0000;
                    SetEditTitle(1);
                    m_mode = 2;
                    break;
                case 0xB040000:
                {
                    name[0] = 0;
                    VIEntity *ent = app->FindEntity(app->m_target);
                    if (ent && ent->IsType(130))
                        app->GetTargetPlayerName(name);
                    m_edit->Clear();
                    m_edit->SetMaxLength(24);
                    m_edit->SetText(name);
                    m_edit->SetTitle((const unsigned short *)ui.m_words.GetString(86, ui.m_language));
                    m_edit->m_x24 = 0xB040000;
                    m_mode = 2;
                    break;
                }
                case 0xB051000:
                    switch (p2ef24->CanInvite())
                    {
                    case 0:
                    {
                        name[0] = 0;
                        VIEntity *ent = app->FindEntity(app->m_target);
                        if (ent && ent->IsType(130))
                            app->GetTargetPlayerName(name);
                        m_edit->Clear();
                        m_edit->SetMaxLength(24);
                        m_edit->SetText(name);
                        m_edit->SetTitle((const unsigned short *)ui.m_words.GetString(86, ui.m_language));
                        m_edit->m_x24 = 0xB051000;
                        m_mode = 2;
                        break;
                    }
                    case 3:
                        AddText((const unsigned short *)ui.m_words.GetString(570, ui.m_language));
                        break;
                    case 11:
                        AddText((const unsigned short *)ui.m_words.GetString(584, ui.m_language));
                        break;
                    case 5:
                        AddText((const unsigned short *)ui.m_words.GetString(546, ui.m_language));
                        break;
                    }
                    break;
                case 0xB051006: {
                    int r = p2ef24->CanChat();
                    switch (r)
                    {
                    case 0:
                        m_edit->Clear();
                        SetEditTitle(0);
                        m_edit->m_x24 = p1;
                        m_edit->SetMaxLength(128);
                        m_mode = 2;
                        break;
                    case 3:
                        app->ShowMsg(570);
                        m_mode = 0;
                        break;
                    default:
                        VIString::snprintf16(text1, 64, "%S: %u", ui.m_words.GetString(835, ui.m_language), r);
                        app->ShowText(text1);
                        m_mode = 0;
                        break;
                    }
                    break;
                }
                case 0xB051009: {
                    int r = p2ef24->CanCreate();
                    switch (r)
                    {
                    case 0:
                        m_edit->Clear();
                        SetEditTitle(0);
                        m_edit->SetTitle(Word(&app->m_ui, 1027));
                        m_edit->m_x24 = p1;
                        m_edit->SetMaxLength(64);
                        m_mode = 2;
                        break;
                    case 4:
                        app->ShowText(Word(&app->m_ui, 569));
                        m_mode = 0;
                        break;
                    case 6:
                        VIString::snprintf16(text2, 512, Word(&app->m_ui, 567), gGuildCost);
                        app->ShowText(text2);
                        m_mode = 0;
                        break;
                    case 7:
                        app->ShowText(Word(&app->m_ui, 568));
                        m_mode = 0;
                        break;
                    default:
                        VIASSERTF(0, "unexpected guilderror 0x%02x", r);
                        break;
                    }
                    break;
                }

                case 0xB051007: {
                    int r = p2ef24->CanDisband();
                    switch (r)
                    {
                    case 0:
                        PromptWord(543, 1, 0, 0xB051007, 1);
                        break;
                    case 3:
                        app->ShowText(Word(&app->m_ui, 570));
                        m_mode = 0;
                        break;
                    case 5:
                        app->ShowText(Word(&app->m_ui, 546));
                        m_mode = 0;
                        break;
                    default:
                        VIASSERTF(0, "unexpected guilderror 0x%02x", r);
                        break;
                    }
                    break;
                }
                case 0xB051008: {

                    int r = p2ef24->CanQuit();
                    switch (r)
                    {
                    case 0:
                        PromptWord(542, 1, 0, 0xB051008, 1);
                        break;
                    case 3:
                        app->ShowText(Word(&app->m_ui, 570));
                        m_mode = 0;
                        break;
                    default:
                        VIASSERTF(0, "unexpected guilderror 0x%02x", r);
                        break;
                    }
                    break;
                }
                case 0xB05100A: {
                    int r = p2ef24->CanList();
                    switch (r)
                    {
                    case 0:
                        Prompt(guild->GetMotd(), 0, 0, 0);
                        break;
                    case 3:
                        app->ShowText(Word(&app->m_ui, 570));
                        m_mode = 0;
                        break;
                    default:
                        VIASSERTF(0, "unexpected guilderror 0x%02x", r);
                        break;
                    }
                    break;
                }
                case 0xB05100B: {
                    int r = p2ef24->CanSetMotd();
                    switch (r)
                    {
                    case 0:
                        m_edit->Clear();
                        SetEditTitle(0);
                        m_edit->SetTitle(Word(&app->m_ui, 1028));
                        m_edit->m_x24 = p1;
                        m_edit->SetMaxLength(64);
                        m_mode = 2;
                        break;
                    case 3:
                        app->ShowText(Word(&app->m_ui, 570));
                        m_mode = 0;
                        break;
                    case 5:
                        app->ShowText(Word(&app->m_ui, 546));
                        m_mode = 0;
                        break;
                    default:
                        VIASSERTF(0, "unexpected guilderror 0x%02x", r);
                        break;
                    }
                    break;
                }
                case 0xB05100C:
                    app->GetP2EF24()->SetChat(p2 != 0);
                    m_mode = 0;
                    break;
                case 0xB0A0000:
                    m_mode = 22;
                    break;
                case 0xB0B0000:
                    m_mode = 23;
                    break;
                case 0xB0B0002:
                    m_mode = 24;
                    break;
                case 0xB0D0000:
                    m_mode = 28;
                    break;
                case 0xB0D0001:
                    m_mode = 29;
                    break;
                case 0xB0B1000:
                    m_mode = 31;
                    break;
                case 0xB0B1001:
                    m_mode = 32;
                    break;
                case 0xB0B1002:
                {
                    name[0] = 0;
                    VIEntity *ent = app->FindEntity(app->m_target);
                    if (ent && ent->IsType(130))
                        app->GetTargetPlayerName(name);
                    m_edit->Clear();
                    m_edit->SetMaxLength(24);
                    m_edit->SetText(name);
                    m_edit->SetTitle((const unsigned short *)ui.m_words.GetString(1470, ui.m_language));
                    m_edit->m_x24 = 0xB0B1002;
                    m_mode = 2;
                    break;
                }
                default:
                    m_mode = 0;
                    app->DoAction(p1, p2);
                    break;
                }
            }
            UpdateMode();
        }
        else if (m_mode == 0)
        {
            UpdateMode();
            switch (p1)
            {
            case 0x8060000:
                app->Send0032(p2);
                break;
            case 0x2030000:
                app->UseItem(p2, 1);
                break;
            case 0x3010000:
                m_edit->Clear();
                SetEditTitle(1);
                m_edit->SetMaxLength(128);
                m_edit->m_x24 = 0xB0F0000;
                m_mode = 2;
                break;
            case 0xB053001:
                app->GetTargetName(name);
                m_edit->Clear();
                m_edit->SetMaxLength(24);
                m_edit->SetText(name);
                m_edit->SetTitle((const unsigned short *)ui.m_words.GetString(67, ui.m_language));
                m_edit->m_x24 = p1;
                m_mode = 2;
                break;
            case 0xB053002:
                app->GetTargetName(name);
                m_edit->Clear();
                m_edit->SetMaxLength(24);
                m_edit->SetText(name);
                m_edit->SetTitle((const unsigned short *)ui.m_words.GetString(613, ui.m_language));
                m_edit->m_x24 = p1;
                m_mode = 2;
                break;
            case 0xB0D0000:
                m_mode = 28;
                break;
            case 0xB0D0001:
                m_mode = 29;
                break;
            default:
                app->DoAction(p1, p2);
                break;
            }
        }
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C556D8
void VIWndGame::LootOpen()
{
    m_loot->Open();
    m_mode = 1;
    UpdateMode();
}

// 0x00C55710
void VIWndGame::LootClose()
{
    if (m_mode == 1)
        m_mode = 0;
    m_loot->SetVisible(false);
    UpdateMode();
}

// 0x00C55740
void VIWndGame::MerchantOpen()
{
    ((VIClient *)GetAppPtr())->PlaySound(15);
    m_hud[0]->SetVisible(false);
    m_hud[1]->SetVisible(false);
    m_hud[2]->SetVisible(false);
    m_merchant->Open();
    m_mode = 10;
    UpdateMode();
}

// 0x00C557A0
void VIWndGame::MerchantClose()
{
    m_mode = 0;
    m_merchant->SetVisible(false);
    UpdateMode();
}

// 0x00C557C8
void VIWndGame::ShowDialogBox(VIDialogBox *d)
{
    ((VIClient *)GetAppPtr())->PlaySound(15);
    m_response->SetDialog(*d);
    m_mode = 7;
    UpdateMode();
}

// 0x00C55820
void VIWndGame::OpenDialog(const unsigned short *text, int b)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (b)
        app->PlaySound(15);
    m_privChat->SetText(text);
    m_mode = 9;
    UpdateMode();
}

// 0x00C55888
void VIWndGame::OpenBank()
{
    ((VIClient *)GetAppPtr())->PlaySound(15);
    m_hud[0]->SetVisible(false);
    m_hud[1]->SetVisible(false);
    m_hud[2]->SetVisible(false);
    m_bank->ShowMenu(1);
    m_mode = 11;
    UpdateMode();
}

// 0x00C558F0
void VIWndGame::CloseBank()
{
    m_mode = 0;
    m_bank->SetVisible(false);
    UpdateMode();
}

// 0x00C55918
void VIWndGame::OpenTrainer()
{
    m_trainer->Open();
    m_mode = 12;
    UpdateMode();
}

// 0x00C55950
void VIWndGame::CloseTrainer()
{
    m_mode = 0;
    m_chat->SetMode(0);
    m_trainer->SetVisible(false);
    UpdateMode();
}

// 0x00C55990
void VIWndGame::Msg001D()
{
    m_trainer->Refresh();
}

// 0x00C559B0
void VIWndGame::OpenClassMastery()
{
    m_classMastery->Refresh();
    SetMode(0);
    m_mode = 40;
    UpdateMode();
}

// 0x00C559F8
void VIWndGame::CloseClassMastery()
{
    m_classMastery->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C55A20: the start menu (mode 13).  Its item table depends on the bank (a banker
// in range) and on the expansion mask's bit 1; the first item drops out unless the zone
// part says otherwise.
void VIWndGame::OpenStartMenu(int a)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (app->GetBank()->GetP78CC())
    {
        if (app->m_p50304 & 2)
        {
            gStartMenu.m_entries = gStartItemsBankX;
            gStartMenu.m_count = gStartCountBankX;
        }
        else
        {
            gStartMenu.m_entries = gStartItemsBank;
            gStartMenu.m_count = gStartCountBank;
        }
    }
    else
    {
        if (app->m_p50304 & 2)
        {
            gStartMenu.m_entries = gStartItemsX;
            gStartMenu.m_count = gStartCountX;
        }
        else
        {
            gStartMenu.m_entries = gStartItems;
            gStartMenu.m_count = gStartCount;
        }
    }
    if (!app->GetP382E8()->IsEnabled())
    {
        gStartMenu.m_entries++;
        gStartMenu.m_count--;
    }
    SetMode(0);
    m_hud[2]->SetVisible(false);
    m_hud[0]->SetVisible(false);
    m_hud[1]->SetVisible(false);
    m_influence->SetVisible(false);
    m_chat->SetMode(1);
    m_startMenu->SetVisible(true);
    m_mode = 13;
    app->StopAutoAttack();
    m_startMenu->ShowMain(a);
    app->ApplyRaceCamera();
}

// 0x00C55BC8
void VIWndGame::CloseStartMenu(int mode)
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->ApplyDefaultCamera();
    m_mode = mode;
    m_chat->SetMode(0);
    m_startMenu->SetVisible(false);
    m_influence->SetVisible(true);
    UpdateMode();
}

// 0x00C55C30
void VIWndGame::PromptWord(int id, int type, int p1, int p2, int show)
{
    VIClient *app = (VIClient *)GetAppPtr();
    SetMode(0);
    m_mbox->Init(type, 1);
    m_mbox->SetText(id);
    m_mbox->m_p1 = p1;
    m_mbox->m_p2 = p2;
    m_mode = 14;
    UpdateMode();
    m_mbox->SetVisible(show);
}

// 0x00C55CE8
void VIWndGame::Prompt(const char *text, int type, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    SetMode(0);
    m_mbox->Init(type, 1);
    m_mbox->SetText(text);
    m_mbox->m_p1 = p1;
    m_mbox->m_p2 = p2;
    m_mode = 14;
    UpdateMode();
}

// 0x00C55D88
void VIWndGame::Prompt(const unsigned short *text, int a, int b, int c)
{
    VIClient *app = (VIClient *)GetAppPtr();
    SetMode(0);
    m_mbox->Init(a, 1);
    m_mbox->SetText(text);
    m_mbox->m_p1 = b;
    m_mbox->m_p2 = c;
    m_mode = 14;
    UpdateMode();
}

// 0x00C55E28
void VIWndGame::Trade02()
{
    SetMode(0);
    m_mode = 15;
    UpdateMode();
}

// 0x00C55E60
void VIWndGame::Trade16()
{
    if (m_mode == 15 || m_mode == 16)
    {
        m_trade->Hide();
        m_trade->SetVisible(false);
        m_chat->SetMode(0);
        m_mode = 0;
        UpdateMode();
    }
}

// 0x00C55EC0
void VIWndGame::TradeOffer(unsigned int id)
{
    SetMode(0);
    m_mode = 16;
    UpdateMode();
}

// 0x00C55EF8
void VIWndGame::TradeClose()
{
    m_trade->SetVisible(false);
    m_chat->SetMode(0);
    m_mode = 0;
    UpdateMode();
}

// 0x00C55F38
void VIWndGame::Trade07()
{
    m_trade->ClearAccepted();
}

// 0x00C55F58
void VIWndGame::Trade06()
{
    m_trade->TheyAccepted();
}

// 0x00C55F78
void VIWndGame::Msg0061()
{
    ((VIClient *)GetAppPtr())->PlaySound(15);
    m_mode = 19;
    UpdateMode();
}

// 0x00C55FB8
void VIWndGame::Msg0063()
{
    m_mode = 0;
    m_chat->SetMode(0);
    m_blacksmith->SetVisible(false);
    UpdateMode();
}

// 0x00C55FF8
void VIWndGame::Msg0060()
{
    m_blacksmith->Refresh();
}

// 0x00C56018
void VIWndGame::ShowBuddyRequest()
{
    SetMode(0);
    m_mode = 17;
    UpdateMode();
}

// 0x00C56050
void VIWndGame::HideBuddyRequest()
{
    m_buddy->SetVisible(false);
    m_mode = 0;
    UpdateMode();
}

// 0x00C56078
void VIWndGame::OpenBuddyList()
{
    SetMode(0);
    m_mode = 18;
    UpdateMode();
}

// 0x00C560B0
void VIWndGame::CloseBuddyList()
{
    m_buddy->HideChildren();
    m_buddy->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C560F0
void VIWndGame::OpenGuild()
{
    SetMode(0);
    m_mode = 20;
    UpdateMode();
}

// 0x00C56128
void VIWndGame::CloseGuild()
{
    m_guild->SetVisible(false);
    m_mode = 0;
    UpdateMode();
}

// 0x00C56150
void VIWndGame::OpenGuildMenu()
{
    SetMode(0);
    m_mode = 21;
    UpdateMode();
}

// 0x00C56188
void VIWndGame::CloseGuildMenu()
{
    m_guild->Reset();
    m_guild->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C561C8
void VIWndGame::OpenQuestLog()
{
    SetMode(0);
    m_mode = 22;
    UpdateMode();
}

// 0x00C56200
void VIWndGame::CloseQuestLog()
{
    m_questLog->HideBox();
    m_questLog->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C56240
void VIWndGame::OpenGramInbox()
{
    SetMode(0);
    m_mode = 24;
    UpdateMode();
}

// 0x00C56278
void VIWndGame::CloseGramInbox()
{
    m_gramInbox->End();
    m_gramInbox->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C562B8
void VIWndGame::OpenComposeGram()
{
    SetMode(0);
    m_mode = 23;
    UpdateMode();
}

// 0x00C562F0
void VIWndGame::CloseComposeGram()
{
    m_composeGram->End();
    m_composeGram->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C56330
void VIWndGame::OpenIgnoreList()
{
    SetMode(0);
    m_mode = 26;
    UpdateMode();
}

// 0x00C56368
void VIWndGame::CloseIgnoreList()
{
    m_ignoreList->Reset();
    m_ignoreList->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C563A8: "<word 636 with name>" in a yes/no box whose answer is 0x07020000.
void VIWndGame::ShowInvite(const char *name)
{
    unsigned short text[128];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    VIString::snprintf16(text, 256, (const unsigned short *)ui.m_words.GetString(636, ui.m_language), name);
    Prompt(text, 1, 0, 0x7020000);
}

// 0x00C56418
void VIWndGame::OpenBuffs()
{
    SetMode(0);
    m_mode = 27;
    UpdateMode();
}

// 0x00C56450
void VIWndGame::CloseBuffs()
{
    m_buffPC->SetVisible(false);
    m_mode = 0;
    UpdateMode();
}

// 0x00C56478
void VIWndGame::OpenOptions()
{
    SetMode(0);
    m_mode = 34;
    UpdateMode();
}

// 0x00C564B0
void VIWndGame::CloseOptions()
{
    m_options->SetVisible(false);
    m_mode = 0;
    UpdateMode();
}

// 0x00C564D8
int VIWndGame::IsOptionsOpen()
{
    return m_options->GetVisible() != 0;
}

// 0x00C564E8
void VIWndGame::OpenTell()
{
    SetMode(0);
    m_mode = 28;
    UpdateMode();
}

// 0x00C56520
void VIWndGame::CloseTell()
{
    m_tell->Close();
    m_tell->SetVisible(false);
    m_mode = 0;
    UpdateMode();
}

// 0x00C56560
void VIWndGame::OpenReply()
{
    SetMode(0);
    m_mode = 29;
    UpdateMode();
}

// 0x00C56598
void VIWndGame::CloseReply()
{
    m_reply->Stub8EA00();
    m_reply->SetVisible(false);
    m_mode = 0;
    UpdateMode();
}

// 0x00C565D8
void VIWndGame::OpenCSRequest()
{
    m_mode = 31;
    UpdateMode();
}

// 0x00C565F8
void VIWndGame::CloseCSRequest()
{
    m_csRequest->End();
    m_csRequest->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C56638
void VIWndGame::OpenCSInbox()
{
    m_mode = 32;
    UpdateMode();
}

// 0x00C56658
void VIWndGame::CloseCSInbox()
{
    m_csInbox->End();
    m_csInbox->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C56698
void VIWndGame::OpenWhoList()
{
    m_mode = 35;
    UpdateMode();
}

// 0x00C566B8
void VIWndGame::CloseWhoList()
{
    m_whoList->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C566E0
void VIWndGame::OpenRecipe(int skill)
{
    m_recipe->SetSkill(skill);
    m_mode = 6;
    UpdateMode();
}

// 0x00C56718
void VIWndGame::CloseRecipe()
{
    m_recipe->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C56740
void VIWndGame::OpenExperiment()
{
    m_mode = 37;
    UpdateMode();
}

// 0x00C56760
void VIWndGame::CloseExperiment()
{
    m_experiment->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C56788: the auction search (mode 39); kinds 0xA8020000 and 0xA8030000 open the
// bank's matching list first, anything else closes both.
void VIWndGame::OpenAuctionSearch(unsigned int kind)
{
    VIBankControl *bank = ((VIClient *)GetAppPtr())->GetBank();
    m_mode = 39;
    UpdateMode();
    m_auctionSearch->SetMode(kind);
    switch (kind)
    {
    case 0xA8020000:
        bank->RequestLots();
        m_auctionSearch->Open();
        break;
    case 0xA8030000:
        bank->RequestBids();
        m_auctionSearch->Open();
        break;
    default:
        bank->ClearResults();
        bank->ClearSaved();
        break;
    }
}

// 0x00C56848
void VIWndGame::CloseAuctionSearch()
{
    VIBankControl *bank = ((VIClient *)GetAppPtr())->GetBank();
    bank->ClearResults();
    bank->ClearSaved();
    m_auctionSearch->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C568B0
void VIWndGame::OpenFaction()
{
    VIClientPart4F078 *part = ((VIClient *)GetAppPtr())->GetP4F078();
    part->Reset();
    part->Request(0);
    m_mode = 41;
    UpdateMode();
}

// 0x00C56918
void VIWndGame::CloseFaction()
{
    ((VIClient *)GetAppPtr())->GetP4F078()->Reset();
    m_faction->SetVisible(false);
    OpenStartMenu(0);
}

// 0x00C56970
void VIWndGame::OpenFishing()
{
    VIClientPart3FC10 *part = ((VIClient *)GetAppPtr())->GetP3FC10();
    if (IsLootable())
    {
        m_mode = 36;
        UpdateMode();
    }
    else
        part->Cancel();
}

// 0x00C569E0
void VIWndGame::CloseFishing()
{
    if (m_mode == 36)
    {
        m_fishing->SetVisible(false);
        m_mode = 0;
        UpdateMode();
    }
}

// 0x00C56A18
void VIWndGame::ManualCameraBegun()
{
    m_mode = 38;
    UpdateMode();
}

// 0x00C56A38
int VIWndGame::IsModal()
{
    return m_mode == 38;
}

// 0x00C56A48
void VIWndGame::Refresh()
{
    m_mode = 0;
    UpdateMode();
}

// 0x00C56A68
void VIWndGame::FishingA()
{
    m_fishing->OpenBait();
}

// 0x00C56A88
void VIWndGame::FishingB()
{
    m_fishing->CloseBait();
}

// 0x00C56AA8
void VIWndGame::FishingC(const unsigned short *text)
{
    m_fishing->SetText(text);
}

// 0x00C56AC8
void VIWndGame::FishingD(const char *text)
{
    m_fishing->SetText(text);
}

// 0x00C56AE8: the edit box, titled with word `b`, for a reply that comes back as `a`.
void VIWndGame::ShowName(int a, int b, int max, const unsigned short *name)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    m_edit->Clear();
    m_edit->SetMaxLength(max);
    if (name)
        m_edit->SetText(name);
    m_edit->m_x24 = a;
    m_edit->SetTitle((const unsigned short *)ui.m_words.GetString(b, ui.m_language));
    m_mode = 2;
}

// 0x00C56BA0
void VIWndGame::AddText(const char *s)
{
    m_chat->AddText(s);
}

// 0x00C56BC0
void VIWndGame::AddText(const unsigned short *s)
{
    m_chat->AddText(s);
}

// 0x00C56BE0
void VIWndGame::AddText(const unsigned short *s, VIColor32F c)
{
    m_chat->AddText(s, c);
}

// 0x00C56C20
void VIWndGame::LootOption(int id)
{
    m_chat->AddWord(id);
}

// 0x00C56C40
void VIWndGame::AddWord(int id, VIColor32F c)
{
    m_chat->AddWord(id, c);
}

// 0x00C56C88
void VIWndGame::ShowNetStats(long age, unsigned long history, long x, unsigned short number, int ping)
{
    m_netStat->SetData(age, history, x, number, ping);
}

// 0x00C56CA8
int VIWndGame::IsLootable()
{
    return m_mode == 0;
}

// 0x00C56CB8
int VIWndGame::DialogAllowed()
{
    return m_mode == 9 || m_mode == 14;
}

// 0x00C56CE0: the modes the chat line stays up in.
int VIWndGame::ModeHasChat()
{
    switch (m_mode)
    {
    case 2:
    case 28:
    case 29:
    case 30:
    case 33:
        return 1;
    case 0:
    default:
        return 0;
    }
}

// 0x00C56D18
int VIWndGame::IsOptionsMode()
{
    return m_mode == 34;
}

// 0x00C56D28
int VIWndGame::IsExperimentMode()
{
    return m_mode == 37;
}

// 0x00C56D38
int VIWndGame::IsRecipeMode()
{
    return m_mode == 6;
}

// 0x00C56D48
int VIWndGame::IsAuctionMode()
{
    return m_mode == 39;
}

// 0x00C56D58
void VIWndGame::OpenGoto()
{
    m_mode = 4;
    UpdateMode();
}

// 0x00C56D78: a gauge -- frame texture `frame`, then texture `bar` cut to v of its width,
// inset 3,1 for the experience bar (152) and 2,2 otherwise; dimmed unless `lit`.
void VIWndGame::DrawGauge(int bar, int frame, VIVect2I at, int lit, float v)
{
    VIVect2I size(0, -1);
    const VIUITexEntry *fill = GetUITexture((VIUITexId)bar);
    const VIUITexEntry *box = GetUITexture((VIUITexId)frame);
    if (!fill || !box)
        return;
    VIUI &ui = ((VIClient *)GetAppPtr())->m_ui;
    ui.BlitEx((VIUITexId)box->m_x00, at, (VIAlphaBlendMode)3, lit ? 1.0f : 0.3f, 0);
    at += bar == 152 ? VIVect2I(3, 1) : VIVect2I(2, 2);
    size.x = (int)(fill->m_uv1.x * v);
    ui.BlitEx((VIUITexId)fill->m_x00, at, size, (VIAlphaBlendMode)3, lit ? 1.0f : 0.3f, 0);
}

// 0x00C56F10: the compass, a 44-pixel quad of "sheet1" turned by the heading at 39,54.
// The rose's UV window depends on the region (the game options' +0x96).
void VIWndGame::DrawCompass(float yaw)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIScene *scene;
    if (m_mat0 == -1)
    {
        VIScene &s = app->m_scene;
        VIResourceType type;
        int index;
        VIDictionary *dict = s.m_dict;
        unsigned int id = UIImageId("sheet1");
        scene = &s;
        if (!dict->Find(id, type, index))
            return;
        m_mat0 = scene->m_raster->CreateMaterial(0);
        if (m_mat0 < 0)
            return;
        VIMaterial *mat = scene->m_raster->Material(m_mat0);
        mat->Init(1);
        mat->SetLayerFillType(0, (VIMaterialFillType)0);
        mat->SetLayerModulate(0, 1);
        mat->SetLayerBlendMode(0, (VIAlphaBlendMode)0);
        mat->SetLayerWrapMode(0, (VIMaterialWrapMode)3);
        mat->SetZWrite(0);
        mat->SetZTest(0);
        mat->SetLayerTexture(0, index, *scene->m_raster, *scene->m_dict);
    }
    scene = &app->m_scene;
    VIVect3 corner[4];
    VIVect3 out[4];
    corner[0].Set(-22.0f, 22.0f, 0.0f);
    corner[1].Set(-22.0f, -22.0f, 0.0f);
    corner[2].Set(22.0f, 22.0f, 0.0f);
    corner[3].Set(22.0f, -22.0f, 0.0f);
    VIMatrix44 m;
    m.RotateZ(yaw + 3.1415927f);
    m.m[3].x = 39.0f;
    m.m[3].y = 54.0f;
    m.Mul(corner[0], out[0]);
    m.Mul(corner[1], out[1]);
    m.Mul(corner[2], out[2]);
    m.Mul(corner[3], out[3]);
    VIRaster *r = scene->m_raster;
    r->SetMaterial(m_mat0);
    r->Color(VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
    r->BeginTriStrip();
    VIVect2 uv[2];
    switch ((signed char)VIGetGameOptions()->m_region)
    {
    case 0:
    case 1:
        uv[0].x = 0.046875f;
        uv[0].y = 0.474609375f;
        uv[1].x = 0.130859375f;
        uv[1].y = 0.55859375f;
        break;
    case 2:
        uv[0].x = 0.775390625f;
        uv[0].y = 0.861328125f;
        uv[1].x = 0.859375f;
        uv[1].y = 0.9453125f;
        break;
    case 3:
        uv[0].x = 0.86328125f;
        uv[0].y = 0.861328125f;
        uv[1].x = 0.947265625f;
        uv[1].y = 0.9453125f;
        break;
    }
    r->UV(VIVect2(uv[0].x, uv[1].y));
    r->Vertex(VIVect2(out[0].x, out[0].y));
    r->UV(VIVect2(uv[0].x, uv[0].y));
    r->Vertex(VIVect2(out[1].x, out[1].y));
    r->UV(VIVect2(uv[1].x, uv[1].y));
    r->Vertex(VIVect2(out[2].x, out[2].y));
    r->UV(VIVect2(uv[1].x, uv[0].y));
    r->Vertex(VIVect2(out[3].x, out[3].y));
    r->EndTriStrip();
}

// VIClient's inline target accessor (its out-of-line copy is GetTargetId, 0x00BE39E0).
static inline unsigned int TargetId(VIClient *app)
{
    return app->m_target;
}

// 0x00C57400: the group's patches on the compass -- each member within 2000 units, at
// its bearing 27 pixels out from 39,55, in its slot's colour.  The target's own patch
// blinks off for the last 150 ms of every 1.5 s.
void VIWndGame::DrawPatch()
{
    static long start;
    VIClient *app = (VIClient *)GetAppPtr();
    float radius = 27.0f;
    VIVect3 hpr;
    VIVect3 me;
    VIVect3 d;
    VIVect2 center(39.0f, 55.0f);
    VIVect2 p;
    VIVect2 v0;
    VIVect2 v1;
    VIVect2 v2;
    VIVect2 v3;
    VIVect2I uv(3, 3);
    VIVect2I size(10, 10);
    float texSize = 16.0f;
    VIScene &scene = app->m_scene;
    VIRaster *r = scene.m_raster;
    if (m_mat1 == -1)
    {
        int index = -1;
        VIDictionary *dict = scene.m_dict;
        if (!dict->FindTyped(UIImageId("Patch_Art"), (VIResourceType)1, index))
            return;
        m_mat1 = r->CreateMaterial(0);
        if (m_mat1 < 0)
            return;
        VIMaterial *mat = r->Material(m_mat1);
        mat->Init(1);
        mat->SetLayerFillType(0, (VIMaterialFillType)0);
        mat->SetLayerModulate(0, 1);
        mat->SetLayerBlendMode(0, (VIAlphaBlendMode)0);
        mat->SetLayerWrapMode(0, (VIMaterialWrapMode)3);
        mat->SetZWrite(0);
        mat->SetZTest(0);
        mat->SetLayerTexture(0, index, *r, *scene.m_dict);
        start = scene.GetTime();
    }
    app->m_camera.m_dir.HPR(&me.x, &me.y, &me.z);
    long phase = (scene.GetTime() - start) % 1500;
    for (int i = 0; i < app->GetP2BE4C(); i++)
    {
        if (app->GetGroupMemberId(i) == app->m_selfId)
            continue;
        if (phase > 1350 && TargetId(app) == app->GetGroupMemberId(i))
            continue;
        VIEntity *ent = app->FindEntity(app->GetGroupMemberId(i));
        if (ent && ent->m_actor != -1)
            d = ent->GetActorPos();
        else
        {
            if (app->GetGroupMember(i)->m_p0C != app->m_worldId)
                continue;
            d = app->GetGroupMember(i)->m_pos;
        }
        d -= app->m_camera.m_position;
        if (d.Magnitude() > 2000.0f)
            continue;
        d.y = 0.0f;
        d.HPR(&hpr.x, &hpr.y, &hpr.z);
        float h = hpr.x;
        float yaw = me.x;
        if (h - yaw > 3.1415927f)
            hpr.x -= 6.2831855f;
        if (hpr.x - yaw < -3.1415927f)
            hpr.x += 6.2831855f;
        hpr.x -= yaw;
        p = center;
        p.x -= sinf(hpr.x) * radius;
        p.y -= cosf(hpr.x) * radius;
        float hx = size.x >> 1;
        float hy = size.y >> 1;
        v0.x = p.x - hx;
        v0.y = p.y - hy;
        v1.x = p.x + hx;
        v1.y = p.y - hy;
        v2.x = p.x + hx;
        v2.y = p.y + hy;
        v3.x = p.x - hx;
        v3.y = p.y + hy;
        r->SetMaterial(m_mat1);
        r->BeginTriFan();
        switch (i)
        {
        case 1:
            r->Color(VIColor32F(0.0f, 1.0f, 0.0f, 1.0f));
            break;
        case 2:
            r->Color(VIColor32F(0.0f, 0.0f, 1.0f, 1.0f));
            break;
        case 3:
            r->Color(VIColor32F(0.0f, 1.0f, 1.0f, 1.0f));
            break;
        case 4:
            r->Color(VIColor32F(1.0f, 1.0f, 0.0f, 1.0f));
            break;
        default:
            r->Color(VIColor32F(1.0f, 1.0f, 0.0f, 1.0f));
            break;
        }
        r->UV(VIVect2(uv.x / texSize, uv.y / texSize));
        r->Vertex(v0);
        r->UV(VIVect2((uv.x + size.x) / texSize, uv.y / texSize));
        r->Vertex(v1);
        r->UV(VIVect2((uv.x + size.x) / texSize, (uv.y + size.y) / texSize));
        r->Vertex(v2);
        r->UV(VIVect2(uv.x / texSize, (uv.y + size.y) / texSize));
        r->Vertex(v3);
        r->EndTriFan();
    }
}

// 0x00C57AB0
void VIWndGame::ToggleHud()
{
    m_p05C = (m_p05C + 1) % 2;
    if (!m_p05C)
    {
        SetMode(0);
        return;
    }
    m_influence->SetVisible(true);
    m_chat->SetVisible(true);
    m_hud[0]->SetVisible(true);
    m_hud[1]->SetVisible(true);
    m_hud[2]->SetVisible(true);
}

// 0x00C57B28
void VIWndGame::ShowNetStat(int on)
{
    if (m_netStat)
        m_netStat->SetVisible(on);
}

// 0x00C57B40
VIChatText *VIWndGame::GetChatLog()
{
    if (m_chat)
        return &m_chat->m_text;
    return 0;
}

// 0x00C57B60: the chat line's title -- the channel's name (words 106..109, 512 by the
// group state) and word 522 after it, or nothing.
void VIWndGame::SetEditTitle(int on)
{
    unsigned short title[256];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    if (on)
    {
        switch (app->m_p382E4)
        {
        case 0:
            VIString::strcpy16(title, 512, (const unsigned short *)ui.m_words.GetString(106, ui.m_language));
            break;
        case 1:
            VIString::strcpy16(title, 512, (const unsigned short *)ui.m_words.GetString(109, ui.m_language));
            break;
        case 2:
            VIString::strcpy16(title, 512, (const unsigned short *)ui.m_words.GetString(107, ui.m_language));
            break;
        case 3:
            VIString::strcpy16(title, 512, (const unsigned short *)ui.m_words.GetString(108, ui.m_language));
            break;
        case 4:
            VIString::strcpy16(title, 512, (const unsigned short *)ui.m_words.GetString(512, ui.m_language));
            break;
        default:
            title[0] = 0;
            break;
        }
        if (title[0])
        {
            VIString::strcat16(title, 512, " ");
            VIString::strcat16(title, 512, (const unsigned short *)ui.m_words.GetString(522, ui.m_language));
        }
    }
    else
        title[0] = 0;
    m_edit->SetTitle(title);
}

// 0x00C57CA0: the interact key steps +0x54 0 -> 1 (2 without the expansion) -> 2 -> 0.
void VIWndGame::Interact()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int x = app->m_p50304 >> 1;
    x &= 1;
    if (!IsLootable())
        return;
    switch (m_hudSel)
    {
    case 0:
        m_hudSel = x ? 1 : 2;
        break;
    case 1:
        m_hudSel = 2;
        break;
    case 2:
        m_hudSel = 0;
        break;
    }
    UpdateMode();
}

// 0x00C57D40: the attack key steps it the other way round.
void VIWndGame::Attack()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int x = app->m_p50304 >> 1;
    x &= 1;
    if (!IsLootable())
        return;
    switch (m_hudSel)
    {
    case 0:
        m_hudSel = 2;
        break;
    case 1:
        m_hudSel = 0;
        break;
    case 2:
    {
        int v = 0;
        if (x)
            v = 1;
        m_hudSel = v;
        break;
    }
    }

    UpdateMode();
}
