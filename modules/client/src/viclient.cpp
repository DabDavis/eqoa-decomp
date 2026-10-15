#line 1 "C:/code-frontiers-gm/game/VIClient.cpp"
// VIClient -- the game object.  CLIENT 0x00BBF0D8 .. 0x00BE3C0F, about 150 KB:
// the extent of this file's .rodata block (0x00CDA318 .. 0x00CDB7F8), from the
// phase 62 survey.  The constructor is at 0x00BBF698; the opcode dispatcher at
// 0x00BD0DC8; the VIMap/VIItemList copies it instantiates close the file.
//
// The original asserts, so __LINE__ is an immediate in those functions: 14
// asserts, lines 1705 (0x00BC18C0) .. 11727, increasing with address, so the
// functions below are in the original's order and an asserting function has
// to sit on its original line.  Functions without an assert can go anywhere.
#include "game.h"

// The layout game.h declares, checked here so a wrong offset fails the build.
#define CHK(n, c)  typedef char n[(c) ? 1 : -1];
#define OFF(m)     ((int)&((VIClient *)0)->m)
CHK(c_ui,       OFF(m_ui)       == 0x4)
CHK(c_language, OFF(m_ui.m_language) == 0x34)
CHK(c_quit,     OFF(m_quit)     == 0x5C)
CHK(c_sound,    OFF(m_sound)    == 0x4FE0)
CHK(c_console,  OFF(m_console)  == 0x6390)
CHK(c_loader,   OFF(m_loader)   == 0x6638)
CHK(c_entities, OFF(m_entities) == 0x7500)
CHK(c_drdp,     OFF(m_drdp)     == 0x7560)
CHK(c_scene,    OFF(m_scene)    == 0x7590)
CHK(c_wndGame,  OFF(m_wndGame)  == 0x2BB78)
CHK(c_2BD40,    OFF(m_p2BD40)   == 0x2BD40)
CHK(c_2EFD0,    OFF(m_p2EFD0)   == 0x2EFD0)
CHK(c_4F0E8,    OFF(m_p4F0E8)   == 0x4F0E8)
CHK(c_exit,     OFF(m_exitCode) == 0x4F12C)
CHK(c_size,     sizeof(VIClient) > 0x50320 && sizeof(VIClient) <= 0x50330)
CHK(c_entityId, ((int)&((VIEntity *)0)->m_id) == 0x0C)
CHK(c_entityActor, ((int)&((VIEntity *)0)->m_actor) == 0x24)
CHK(c_selfId,   OFF(m_selfId)   == 0x71F8)
CHK(c_level,    OFF(m_player.m_level) == 0xC300)
CHK(c_corpse,   OFF(HaveCorpse) == 0x13B94)
CHK(c_merchant, OFF(m_merchantName) == 0x22A78)
CHK(c_buddies,  OFF(m_buddies)  == 0x2EC70)
CHK(c_guild,    OFF(m_guild)    == 0x2EC84)
CHK(c_bank,     OFF(m_bank)     == 0x47388)
CHK(c_50304,    OFF(m_p50304)   == 0x50304)
CHK(c_buddy,    sizeof(VIBuddy) == 172)
CHK(c_netitem,  sizeof(VINetItem) == 0x2FC)

// 0x00BC16D0
void VIClient::Shutdown()
{
    m_p4F0EC = -1;
    Disconnect(0, 1);
    FreeBuffer();
    m_console.Close();
}

// 0x00BC1720
void VIClient::Disconnect(int reason, int quiet)
{
    if (m_sound.IsBgmPlaying(0) || m_sound.IsBgmPlaying(1))
    {
        if (!(VIGetFeatureFlags() & 0x100))
            StopMusic();
        m_sound.StopAllBgm();
    }
    m_scene.StopSoundTrack(m_loader, 1);

    m_p00058 = -1;
    m_p00080 = 0;
    m_p4F0E8 = reason;
    if (!quiet && m_p4F130 && m_p2BD40 != &m_p2BB98 && reason != 21)
        Notify();

    for (int i = 0; i < 24; i++)
        if (m_entities[i])
        {
            delete m_entities[i];
            m_entities[i] = 0;
        }

    if (m_conn)
    {
        connection_close(m_conn);
        m_conn = 0;
    }
    if (m_drdp)
    {
        drdp_flush(m_drdp);
        drdp_destroy(m_drdp);
        m_drdp = 0;
    }

    m_p2EFD0.Reset();
    // In address order: of the six orders of these three stores, only this one
    // gives the target's (sw +0x2EBFC first, the +0x2E7F1 byte in the delay slot).
    m_p2E7F0 = 0;
    m_p2E7F1 = 0;
    m_p2EBFC = 0;
    Reset();
}

// 0x00BC2D18
bool VIClient::IsRunning()
{
    return !m_quit;
}

// 0x00BC9870: every slot is visited, so a duplicate id answers with the LAST
// match (the test if-converts to a movz).
VIEntity *VIClient::FindEntity(int id)
{
    VIEntity *found = 0;
    for (int i = 0; i < 24; i++)
        if (m_entities[i] && m_entities[i]->m_id == id)
            found = m_entities[i];
    return found;
}


















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































// 0x00BD0DC8: the opcode dispatcher for DRDP message type 0xFB (the receive
// loop, 0x00BD4C10, routes the other types elsewhere).  One switch on a u16,
// compiled as a balanced compare tree; the case bodies below are in the
// target's body order, which is source order.  Each case's scalars are block
// locals whose address goes to the stream, so their stack slots read back as
// this declaration list: bytes at sp+2112 in body order, ints at sp+2144 in
// first-use order, buffers in one reused region at sp+48.  Shared tails are
// written out in full in every case; gcc cross-jumps them.
void VIClient::ProcessMessage(long src, const void *data, int size)
{
    int iSrc = src;
    VIMemStream s(data, size);
    unsigned short opcode;
    s.Read(&opcode, 2, 1);

    switch (opcode)
    {
    case 0x000D:
        Msg000D(iSrc, s);
        break;

    case 0x0A7B:
        Msg0A7B(s);
        break;

    case 0x0A7A:
        Msg0A7A(s);
        break;

    case 0x069E:
    {
        int unused;
        unsigned short buf[1024];
        s.Read(&unused, 4, 1);
        s.ReadString(buf, 2048);
        m_wndGame->AddText(buf, VIColor32F(0.5f, 0.5f, 1.0f, 1.0f));
        break;
    }

    case 0x00CE:
    {
        unsigned short buf[1024];
        s.ReadString(buf, 2048);
        m_wndGame->AddText(buf, VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
        break;
    }

    case 0x069C:
    {
        unsigned char sub;
        s.Read(&sub, 1, 1);
        switch (sub)
        {
        case 3:
            ShowMsg(627);
            break;
        case 1:
        {
            unsigned short buf[128];
            char name[24];
            s.ReadString(name, 24);
            VIUI &ui = m_ui;
            VIString::snprintf16(buf, 256, (const unsigned short *)ui.m_words.GetString(558, ui.m_language), name);
            ShowText(buf);
            break;
        }
        case 2:
        {
            unsigned short buf[128];
            char name[24];
            s.ReadString(name, 24);
            VIUI &ui = m_ui;
            VIString::snprintf16(buf, 256, (const unsigned short *)ui.m_words.GetString(628, ui.m_language), name);
            ShowText(buf);
            break;
        }
        case 4:
        {
            unsigned short buf[128];
            char name[24];
            s.ReadString(name, 24);
            VIUI &ui = m_ui;
            VIString::snprintf16(buf, 256, (const unsigned short *)ui.m_words.GetString(642, ui.m_language), name);
            ShowText(buf);
            break;
        }
        case 5:
        {
            unsigned short buf[128];
            char name[24];
            s.ReadString(name, 24);
            VIUI &ui = m_ui;
            VIString::snprintf16(buf, 256, (const unsigned short *)ui.m_words.GetString(1022, ui.m_language), name);
            ShowText(buf);
            break;
        }
        }
        break;
    }

    case 0x0772:
        Msg0772(s);
        break;

    case 0x0773:
        Msg0773(s);
        break;

    case 0x0774:
        Msg0774(s);
        break;

    case 0x069A:
    {
        if (!InGroup())
            break;
        unsigned char sub;
        s.Read(&sub, 1, 1);
        switch (sub)
        {
        case 1:
            ShowMsg(630);
            break;
        case 2:
            ShowMsg(631);
            break;
        case 3:
            ShowMsg(632);
            break;
        case 4:
            ShowMsg(633);
            break;
        case 5:
        {
            unsigned short buf[128];
            VIUI &ui = m_ui;
            VIString::snprintf16(buf, 256, "%S  %S", ui.m_words.GetString(629, ui.m_language),
                                 ui.m_words.GetString(827, ui.m_language));
            break;
        }
        case 7:
            ShowMsg(631);
            break;
        case 8:
            break;
        }
        m_p2BD70 = -1;
        m_p2BE4C = m_p2BD78 = m_inGroup = 0;
        if (m_p382E4 == 2)
            GroupChanged(0);
        break;
    }

    case 0x062F:
        if (!InGroup())
            break;
        ShowMsg(634);
        m_p2BD70 = -1;
        m_p2BE4C = m_p2BD78 = m_inGroup = 0;
        if (m_p382E4 == 2)
            GroupChanged(0);
        break;

    case 0x0775:
    {
        int id;
        unsigned char action;
        s.Read(&id, 4, 1);
        s.Read(&action, 1, 1);
        VIEntity *e = FindEntity(id);
        if (!e)
            break;
        if (id == m_selfId && action != 34)
            break;
        if (m_scene.GetActorSpriteType(e->m_actor) != 5)
            break;
        ((VICSprite *)m_scene.Sprite(m_scene.GetActorSprite(e->m_actor)))->SetAction((VICSpriteAnimID)action, m_scene);
        break;
    }

    case 0x0020:
    {
        int old = m_player.m_level;
        s.ReadSigned(m_player.m_level);
        s.ReadSigned(m_player.m_exp);
        if (old < m_player.m_level)
            PlaySound(16);
        break;
    }

    case 0x001C:
        s.Read(&m_player.m_p44, 4, 1);
        break;

    case 0x0034:
        Msg0034(s);
        break;

    case 0x0046:
    {
        int id;
        unsigned short buf[512];
        s.Read(&id, 4, 1);
        s.ReadString(buf, 1024);
        ShowDialog(id, buf, 1, 1);
        break;
    }

    case 0x0758:
    {
        char buf[512];
        s.ReadString(buf, 512);
        m_wndGame->AddText(buf);
        break;
    }

    case 0x0000:
    {
        int version;
        s.Read(&version, 4, 1);
        if (version != 18)
        {
            printf("Disconnect-??\n");
            Disconnect(16, 0);
        }
        break;
    }

    case 0x0729:
        printf("Disconnect-5\n");
        Disconnect(3, 0);
        break;

    case 0x072B:
        printf("Disconnect-6\n");
        Disconnect(4, 0);
        break;

    case 0x0728:
        printf("Disconnect-7\n");
        Disconnect(6, 0);
        break;

    case 0x072A:
        printf("Disconnect-8\n");
        Disconnect(7, 0);
        break;

    case 0x002E:
        m_wndLogin->ShowWord(269);
        m_charCount--;
        break;

    case 0x0065:
    {
        VIUI &ui = m_ui;
        m_wndLogin->ShowText((const unsigned short *)ui.m_words.GetString(828, ui.m_language));
        m_charCount--;
        break;
    }

    case 0x002F:
    {
        VIUI &ui = m_ui;
        m_wndLogin->ShowText((const unsigned short *)ui.m_words.GetString(829, ui.m_language));
        m_charCount--;
        break;
    }

    case 0x002C:
        if (m_p00058 == 2 || m_p00058 == 3)
        {
            m_p00058 = 3;
            m_p4F0E8 = 21;
            CharacterList(s);
        }
        break;

    case 0x0777:
        Msg0777();
        break;

    case 0x003B:
    {
        VINetItem item;
        ReadNetItem(s, item);
        AddInvItem(item);
        break;
    }

    case 0x003C:
    {
        unsigned char a, b;
        int c;
        s.Read(&a, 1, 1);
        s.Read(&b, 1, 1);
        s.ReadSigned(c);
        Msg003C(a, c, b);
        break;
    }

    case 0x003D:
    {
        unsigned char a, b;
        s.Read(&a, 1, 1);
        s.Read(&b, 1, 1);
        Msg003D(a, b);
        break;
    }

    case 0x003E:
    {
        unsigned char a, b;
        s.Read(&a, 1, 1);
        s.Read(&b, 1, 1);
        Msg003E(a, b);
        break;
    }

    case 0x003F:
    {
        unsigned char a, b;
        s.Read(&a, 1, 1);
        s.Read(&b, 1, 1);
        Msg003F(a, b);
        break;
    }

    case 0x0040:
    {
        unsigned char a;
        s.Read(&a, 1, 1);
        Msg0040(a);
        break;
    }

    case 0x0019:
    {
        unsigned char a;
        int b;
        s.Read(&a, 1, 1);
        s.ReadSigned(b);
        Msg0019(a, b);
        break;
    }

    case 0x001A:
        Msg001A();
        break;

    case 0x0018:
        VIASSERT(!HaveCorpse);
        m_corpse.Read(s);
        HaveCorpse = 1;
        if (m_wndGame->IsLootable())
            m_wndGame->LootOpen();
        else
            LootNoWindow();
        break;

    case 0x0016:
        m_wndGame->LootClose();
        HaveCorpse = 0;
        m_corpse.Clear();
        break;

    case 0x0085:
    {
        unsigned char opt;
        s.Read(&opt, 1, 1);
        switch (opt)
        {
        case 1:
            m_wndGame->LootOption(673);
            break;
        case 2:
            m_wndGame->LootOption(674);
            break;
        case 3:
            m_wndGame->LootOption(675);
            break;
        }
        break;
    }

    case 0x00B1:
    {
        int a, b, c, d;
        unsigned char n;
        int targets[256];
        s.Read(&a, 4, 1);
        s.Read(&b, 4, 1);
        s.Read(&n, 1, 1);
        for (int i = 0; i < n; i++)
            s.Read(&targets[i], 4, 1);
        s.Read(&c, 4, 1);
        s.Read(&d, 4, 1);
        CastingCombat(a, b, n, targets, c, d);
        break;
    }

    case 0x00B3:
        Msg00B3(s);
        break;

    case 0x00B4:
    {
        unsigned char a, b;
        s.Read(&m_p2BB60, 4, 1);
        s.Read(&a, 1, 1);
        s.Read(&b, 1, 1);
        Msg00B4(a, b);
        break;
    }

    case 0x00FF:
        Msg00FF(s);
        break;

    case 0x00B5:
    {
        unsigned char a, b;
        s.Read(&m_p2BB60, 4, 1);
        s.Read(&a, 1, 1);
        s.Read(&b, 1, 1);
        Msg00B5(a, b);
        break;
    }

    case 0x00B6:
    {
        unsigned char a, b;
        s.Read(&m_p2BB60, 4, 1);
        s.Read(&a, 1, 1);
        s.Read(&b, 1, 1);
        Msg00B6(a, b);
        break;
    }

    case 0x00FD:
        Msg00FD(s);
        break;

    case 0x00B8:
    {
        int z;
        s.Read(&m_p2BB60, 4, 1);
        s.ReadSigned(z);
        Msg00B8(z);
        break;
    }

    case 0x00CD:
    {
        unsigned char a;
        int z;
        s.Read(&a, 1, 1);
        s.Read(&m_p2BB60, 4, 1);
        s.ReadSigned(z);
        Msg00CD(a, z);
        break;
    }

    case 0x00B2:
        Msg00B2(s);
        break;

    case 0x00FC:
    {
        unsigned char v;
        s.Read(&v, 1, 1);
        m_p06DC8 = v;
        break;
    }

    case 0x00B7:
        memset(m_merchantName, 0, 256);
        s.Read(&m_p1B300, 4, 1);
        s.ReadSigned(m_p1B308);
        s.ReadSigned(m_p1B30C);
        m_merchant.Read(s);
        s.ReadString(m_merchantName, 256);
        MerchantOpen();
        break;

    case 0x03D5:
    {
        VIBuddy r;
        s >> r;
        r.m_online = s.ReadBool();
        s.ReadString(r.m_wname, 128);
        int i = m_buddies.Find(r.m_id);
        if (i == m_buddies.End())
            m_buddies.Insert(r.m_id, r);
        else
            m_buddies.Value(i) = r;
        break;
    }

    case 0x03D6:
    {
        int id;
        s >> id;
        m_buddies.Erase(m_buddies.Find(id));
        break;
    }

    case 0x03D7:
        ShowMsg(590);
        break;

    case 0x03D8:
        ShowMsg(591);
        break;

    case 0x076F:
        Msg076F(s);
        break;

    case 0x0770:
    {
        char name[24];
        s.ReadString(name, 24);
        Msg0770(name);
        break;
    }

    case 0x0771:
    {
        unsigned char a;
        char name[24];
        s.Read(&a, 1, 1);
        s.ReadString(name, 24);
        Msg0771(a, name);
        break;
    }

    case 0x0468:
        m_guild.Recv0468(s);
        break;

    case 0x0700:
        m_p382E8.Recv0700(s);
        break;

    case 0x07E2:
        m_p4F078.Recv07E2(s);
        break;

    case 0x07E0:
        Msg07E0(s);
        break;

    case 0x057A:
        m_p2EF24.Recv057A(s);
        break;

    case 0x0579:
        m_p2EF24.Recv0579(s);
        break;

    case 0x0D41:
        m_p2EFD0.Recv0D41(s);
        break;

    case 0x0E02:
        m_p38334.Recv0E02(s);
        break;

    case 0x0F02:
        m_p3FC10.Recv0F02(s);
        break;

    case 0x1102:
        m_p3834C.Recv1102(s);
        break;

    case 0x1402:
        m_p4EC58.Recv1402(s);
        break;

    case 0x07F2:
    case 0x07F4:
        CharSelectMsg(1662);
        break;

    case 0x07F0:
        CharSelectMsg(676);
        break;

    case 0x07F1:
        CharSelectMsg(676);
        break;

    case 0x07F3:
        CharSelectMsg(676);
        break;

    case 0x07A1:
        Msg07A1(s);
        break;

    case 0x07A2:
        Msg07A2(s);
        break;

    case 0x07C0:
        Disconnect(17, 0);
        break;

    case 0x07C1:
        Disconnect(18, 0);
        break;

    case 0x0052:
        s.ReadSigned(m_player.m_p34);
        break;

    case 0x124D:
        if (m_wndGame->IsLootable())
        {
            m_wndGame->BankClose();
            m_bank.Close();
        }
        break;

    case 0x1250:
    case 0x1251:
    case 0x1253:
        m_bank.Recv(opcode, s);
        break;

    case 0x1206:
        m_bank.Recv1206(s);
        break;

    case 0x0036:
    {
        unsigned char a;
        int old = m_player.m_p40;
        s.Read(&a, 1, 1);
        s.ReadSigned(m_player.m_p40);
        if (m_player.m_p40 != old)
            QuestStage(a, m_player.m_p40 - old);
        break;
    }

    case 0x001D:
    {
        int z;
        s.ReadSigned(z);
        Msg001D(z);
        break;
    }

    case 0x000A:
    {
        unsigned char a, b;
        s.Read(&a, 1, 1);
        s.Read(&b, 1, 1);
        Msg000A(a, b);
        break;
    }

    case 0x0010:
        Msg0010(s);
        break;

    case 0x00EC:
    case 0x07A4:
        printf("Disconnect-9\n");
        Disconnect(8, 0);
        break;

    case 0x0776:
    {
        int a, b, c;
        s.Read(&a, 4, 1);
        s.Read(&b, 4, 1);
        s.Read(&c, 4, 1);
        Msg0776(a, b, c);
        break;
    }

    case 0x077A:
        m_wndGame->AddWord(1196, VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
        StopAutoAttack();
        Msg077A(1915);
        break;

    case 0x0057:
    {
        int a, b;
        s.Read(&a, 4, 1);
        s.Read(&b, 4, 1);
        Msg0057(a, b);
        break;
    }

    case 0x0056:
    {
        unsigned char sub;
        int id;
        s.Read(&sub, 1, 1);
        s.Read(&id, 4, 1);
        if (id != m_tradePartner && sub != 2 && sub != 20 && sub != 1)
            break;
        switch (sub)
        {
        case 2:
            Trade02(id);
            break;
        case 1:
            Trade01();
            break;
        case 20:
            Trade20();
            break;
        case 16:
            Trade16();
            break;
        case 17:
            Trade17();
            break;
        case 21:
            Trade21();
            break;
        case 22:
            Trade22();
            break;
        case 7:
            Trade07();
            break;
        case 6:
            Trade06();
            break;
        default:
            if (m_p2BB04)
                TradeA(sub, s);
            else
                TradeB(sub, s);
            break;
        }
        break;
    }

    case 0x005E:
        Msg005E();
        break;

    case 0x0060:
    {
        int id, z;
        s.Read(&id, 4, 1);
        s.ReadSigned(z);
        Msg0060(m_inventory.Find(id), z);
        break;
    }

    case 0x0061:
    {
        int a;
        s.Read(&a, 4, 1);
        Msg0061(a);
        break;
    }

    case 0x0063:
        Msg0063();
        break;

    case 0x0012:
    {
        char name[24];
        s.ReadString(name, 24);
        m_wnd2BB88->SetName(name);
        break;
    }

    case 0x0013:
    {
        long a, b;
        s.Read(&a, 8, 1);
        s.Read(&b, 8, 1);
        SetTime(a, b);
        break;
    }

    case 0x0045:
    {
        int z;
        s.ReadSigned(z);
        Msg0045(z);
        break;
    }

    case 0x006E:
        Msg006E();
        break;

    case 0x007B:
    {
        int a;
        s.Read(&a, 4, 1);
        Msg007B(a);
        break;
    }

    case 0x007C:
    {
        unsigned short buf[128];
        s.Read(&m_p2BB0C, 4, 1);
        s.ReadString(buf, 256);
        Msg007C(buf);
        break;
    }

    case 0x007D:
    {
        int z;
        s.Read(&m_p2BB0C, 4, 1);
        s.ReadSigned(z);
        Msg007D(z);
        break;
    }

    case 0x00F8:
        s.Read(&m_speed, 4, 1);
        SetSpeed(m_speed);
        break;

    case 0x007E:
        s.Read(&m_p2BE50, 4, 1);
        break;

    case 0x007F:
        m_p2BE50 = -1;
        m_p2BE54 = 0;
        break;

    case 0x00D3:
    {
        char name[64];
        unsigned char c;
        s.ReadString(name, 64);
        s.Read(&c, 1, 1);
        Msg00D3(name, c);
        break;
    }

    case 0x1003:
    case 0x1004:
    case 0x1005:
        m_p38300.Recv(opcode, s);
        Msg1003Done();
        break;

    case 0x0790:
        Msg0790(s);
        break;

    case 0x00C5:
    {
        unsigned char w;
        s.Read(&w, 1, 1);
        SetWeather(w, 0);
        break;
    }

    case 0x00C7:
    {
        unsigned char w;
        s.Read(&w, 1, 1);
        SetWeather(w, 1);
        break;
    }

    case 0x00D7:
        Msg00D7();
        break;

    case 0x07A3:
    {
        char name[24];
        s.ReadString(name, 24);
        VIString::str8to16(m_p4F0B8, 48, name);
        break;
    }

    case 0x00D9:
        s.Read(&m_player.m_p2C, 4, 1);
        break;

    case 0x00DB:
    {
        int attacker, amount, defender;
        s.Read(&attacker, 4, 1);
        s.ReadSigned(amount);
        s.Read(&defender, 4, 1);
        if (attacker == m_selfId)
            DamageFromSelf(defender, amount);
        else
            DamageFromOther(attacker, amount);
        break;
    }

    case 0x07B0:
        m_p33950.Recv07B0(s);
        break;

    case 0x07B2:
        s.Read(&m_p07578, 4, 1);
        break;

    case 0x07D1:
        s.Read(&m_p50304, 4, 1);
        break;

    case 0x07D2:
        Disconnect(19, 0);
        break;

    // Two opcodes are handled by doing nothing.  They leave no code of their
    // own -- jump optimisation deletes a test whose both arms reach the exit --
    // but they are nodes of the compare tree, and its shape needs one in
    // 0x00ED..0x00F7 and one in 0x0778..0x0779 (a skeleton of this switch
    // reproduces the target's 102 tests only then).  Which value in each gap
    // is not recoverable; the gap's first is used.
    case 0x00ED:
        break;

    case 0x0778:
        break;
    }
}

// 0x00BD90F8: words are UTF-16 behind GetString's char * (SUPPORT casts it the
// same way, VIUIWindow::T).  The named reference is the source fact: through
// `m_ui.` directly, m_language loads off `this` and 4 words differ.
void VIClient::ShowMsg(unsigned short id)
{
    VIUI &ui = m_ui;
    ShowText((const unsigned short *)ui.m_words.GetString(id, ui.m_language));
}

// 0x00BD9138
void VIClient::ShowText(const unsigned short *s)
{
    m_wndGame->AddText(s);
}

// Layout checks added after the dispatcher was written: the CHKs at the top
// cannot grow without moving the padded assert at line 10307.
CHK(c_autoAttack, OFF(m_autoAttack) == 0x71E4)
CHK(c_target,     OFF(m_target)     == 0xC140)
CHK(c_1B300,      OFF(m_p1B300)     == 0x1B300)
CHK(c_2BB10,      OFF(m_p2BB10)     == 0x2BB10)

// VIClient.cpp's own copy of VIMap<int, VIBuddy>.  gcc 2.95 emitted a template
// instantiation in EVERY TU that used it (there is no COMDAT here), so CLIENT holds
// its own copies of SUPPORT's containers; these are the members this one has.  The
// definitions are SUPPORT's own (src/vimap_t.h), unchanged.
#include "vimap_t.h"
template int VIMap<int, VIBuddy>::Begin() const;  // 0x00BDF330
template int VIMap<int, VIBuddy>::End() const;  // 0x00BDF388
template int VIMap<int, VIBuddy>::Next(int) const;  // 0x00BDF390
template int VIMap<int, VIBuddy>::DestroyPool();  // 0x00BDF448
template VIMap<int, VIBuddy>::~VIMap();  // 0x00BDF4A0
template int VIMap<int, VIBuddy>::Clear();  // 0x00BE1A88
template int VIMap<int, VIBuddy>::Find(const int &) const;  // 0x00BE1F68
template int VIMap<int, VIBuddy>::LinkFreeChain(int);  // 0x00BE1FD0
template int VIMap<int, VIBuddy>::Allocate();  // 0x00BE2078
template int VIMap<int, VIBuddy>::AddNode(int, int);  // 0x00BE2240
template int VIMap<int, VIBuddy>::LeftRotate(int);  // 0x00BE2338
template int VIMap<int, VIBuddy>::RightRotate(int);  // 0x00BE23F0
template int VIMap<int, VIBuddy>::Insert(const int &, const VIBuddy &);  // 0x00BE24A8
template int VIMap<int, VIBuddy>::EraseFixup(int);  // 0x00BE2818
template int VIMap<int, VIBuddy>::FreeNode(int);  // 0x00BE2BD0
template int VIMap<int, VIBuddy>::Erase(int);  // 0x00BE2C10
CHK(c_abilities,  OFF(m_abilities)  == 0x2BB2C)
CHK(c_ability,    sizeof(VIAbility) == 452)

// VIMap<int, VIAbility>, VIClient's ability map (+0x2BB2C): the members a plain
// VIAbility reproduces.  Allocate/Insert/AddNode copy through the real class's
// member-wise copy constructor and DestroyPool destroys through its destructor; those
// are named in the map and still open.
template int VIMap<int, VIAbility>::Begin() const;  // 0x00BDEED0
template int VIMap<int, VIAbility>::End() const;  // 0x00BDEF28
template int VIMap<int, VIAbility>::Next(int) const;  // 0x00BDEF30
template int VIMap<int, VIAbility>::LinkFreeChain(int);  // 0x00BE0C00
template int VIMap<int, VIAbility>::LeftRotate(int);  // 0x00BE0EE8
template int VIMap<int, VIAbility>::RightRotate(int);  // 0x00BE0FA0
template int VIMap<int, VIAbility>::EraseFixup(int);  // 0x00BE1408
template int VIMap<int, VIAbility>::Erase(int);  // 0x00BE1848
template int VIMap<int, VIAbility>::Find(const int &) const;  // 0x00BE1A20
template int VIMap<int, VIAbility>::Clear();  // 0x00BE3258
template int VIMap<int, VIAbility>::Init(int);  // 0x00BE32C8
CHK(c_player,     OFF(m_player)     == 0xC2DC)
CHK(c_playerSize, sizeof(VIPlayer)  == 0x14C)
CHK(c_raster,     OFF(m_raster)     == 0x380)
CHK(c_dict,       OFF(m_dict)       == 0x6618)
CHK(c_walkCam,    OFF(m_walkCam)    == 0x6DD0)
CHK(c_p07568,     OFF(m_p07568)     == 0x7568)
CHK(c_group,      OFF(m_group)      == 0x2BD7C)
CHK(c_effects,    OFF(m_effectCount) == 0x2C170)
CHK(c_quests,     OFF(m_questCount) == 0x2CEE0)
CHK(c_entity19,   ((int)&((VIEntity *)0)->m_p19) == 0x19)

// ---------------------------------------------------------------------- accessors --
// A run of one-line accessors, 0x00BD7708..0x00BD7A50.  Names provisional; the meanings
// are FrontiersForge's (claude-notes/frontiersforge-viclient-xref-20261015.md).
void VIClient::SetP06DC0(int a, int b)
{
    m_p06DC0 = a;
    m_p06DC4 = b;
}

void VIClient::SetP06DC0(int a)
{
    m_p06DC0 = a;
}

void VIClient::SetP06DC4(int b)
{
    m_p06DC4 = b;
}

int VIClient::GetClass() { return m_player.m_class; }
int VIClient::GetRace() { return m_player.m_race; }
int VIClient::GetLevel() { return m_player.m_level; }

int VIClient::GetSelfP19()
{
    VIEntity *e = FindEntity(m_selfId);
    if (e)
        return e->m_p19;
    return 0;
}

int VIClient::GetHP() { return m_player.m_hp; }
int VIClient::GetMaxHP() { return m_player.m_maxHp; }
int VIClient::GetPlayerP30() { return m_player.m_p30; }
int VIClient::GetPower() { return m_player.m_power; }
int VIClient::GetMaxPower() { return m_player.m_maxPower; }
int VIClient::GetExp() { return m_player.m_exp; }

unsigned int VIClient::GetGroupMemberId(int i) { return m_group[i].m_id; }
int VIClient::GetGroupMemberHP(int i) { return m_group[i].m_hp; }
int VIClient::GetGroupMemberHPKnown(int i) { return m_group[i].m_hpKnown; }
int VIClient::GetGroupMemberActive(int i) { return m_group[i].m_active; }
char *VIClient::GetGroupMemberName(int i) { return m_group[i].m_name; }

int VIClient::GetTargetP19()
{
    VIEntity *e = FindEntity(m_target);
    if (e)
        return e->m_p19;
    return 0;
}

int VIClient::IsGroupLeader() { return m_p2BD78; }

// ------------------------------------------------------------- small handlers --
// The smallest of the dispatcher's handlers and helpers (names provisional).
void VIClient::StopAutoAttack()
{
    m_autoAttack = 0;
}

void VIClient::Msg006E()
{
}

int VIClient::InGroup()
{
    return m_inGroup;
}

void VIClient::LootNoWindow()
{
    Msg077A(22);
}

void VIClient::Msg003D(int a, int b)
{
    m_inventory.Op003D(a, b);
}

void VIClient::Msg07A2(VIMemStream &s)
{
    Disconnect(14, 0);
}

int VIClient::PlaySound(int id)
{
    return m_ui.SoundPlay((VIUISoundId)id);
}

void VIClient::StopMusic()
{
    m_pC188 = 0;
    m_pC18C = -1;
    m_pC190 = 0;
}

void VIClient::Trade01()
{
    Trade16();
}

void VIClient::Trade07()
{
    m_wndGame->Trade07();
}

void VIClient::Trade06()
{
    m_wndGame->Trade06();
}

void VIClient::Trade16()
{
    m_wndGame->Trade16();
    TradeReset();
}

void VIClient::Trade02(int partner)
{
    m_tradePartner = partner;
    TradeSetup();
    m_wndGame->Trade02();
}

void VIClient::Msg0063()
{
    m_p22B78 = -1;
    m_wndGame->Msg0063();
}

void VIClient::Msg001D(int z)
{
    m_player.m_p40 -= z;
    m_wndGame->Msg001D();
}

void VIClient::Msg07A1(VIMemStream &s)
{
    char buf[8];
    s.Read(buf, 8, 1);
    Disconnect(13, 0);
}

void VIClient::FreeBuffer()
{
    if (m_p4F0B0)
        delete [] (char *)m_p4F0B0;
    m_p4F0B0 = 0;
    m_p4F0B4 = 0;
}

void VIClient::Msg0060(VIInvItem *item, int z)
{
    m_inventory.Op0060(item, z);
    PlaySound(1);
    m_wndGame->Msg0060();
}

void VIClient::Msg00D7()
{
    m_p1B304 = 0;
    m_bank.Close();
    m_p13B90 = 0;
}

void VIClient::SetTime(long a, long b)
{
    m_clockA.Set(a, m_p07568);
    m_clockB.Set(b, m_p07568);
}

void VIClient::AddInvItem(VINetItem &item)
{
    m_inventory.AddItem(item, item.m_p38, 40);
    if (HasInvSpace())
        PlaySound(3);
    m_p1B304 = 0;
}

void VIClient::MerchantOpen()
{
    if (m_wndGame->IsLootable())
    {
        m_wndGame->MerchantOpen();
        m_p1B304 = 0;
    }
}

void VIClient::Trade17()
{
    m_wndGame->AddWord(733, VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    m_wndGame->Trade07();
}

void VIClient::Trade21()
{
    m_wndGame->AddWord(734, VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    m_wndGame->Trade07();
}

void VIClient::Trade22()
{
    m_wndGame->AddWord(735, VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    m_wndGame->Trade07();
}

void VIClient::Msg0061(int a)
{
    if (m_wndGame->IsLootable())
    {
        m_p22B78 = a;
        m_wndGame->Msg0061();
    }
}

void VIClient::Notify()
{
    m_pC1B0 = 0;
    m_p2BB98.SetAppPtr(this);
    m_p2BD44 = &m_p2BB98;
    SetActiveWindow(&m_p2BB98);
}

void VIClient::Msg0A7A(VIMemStream &s)
{
    unsigned short text[1024];
    s.ReadString(text, sizeof(text));
    m_wndGame->AddText(text, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
}

void VIClient::Msg0770(const char *name)
{
    unsigned short text[256];
    VIUI &ui = m_ui;
    VIString::snprintf16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(557, ui.m_language), name);
    m_wndGame->AddText(text);
}

void VIClient::Msg007B(unsigned int id)
{
    if (id == 0xFFFFFFFF)
        m_wndGame->LootOption(838);
    else
    {
        m_target = id;
        TargetInfo(16, m_p2BB68, id);
    }
}

void VIClient::Msg007C(const unsigned short *text)
{
    if (m_questCount < 8)
    {
        VIString::strcpy16(m_questTitle[m_questCount], 256, text);
        m_questCount++;
    }
}

void VIClient::DamageFromSelf(int defender, int amount)
{
    if (!VIGetGameOptions()->m_x92)
        m_p4FD28.AddDamage(defender, amount, 0);
}

void VIClient::CharSelectMsg(int id)
{
    m_p00058 = 3;
    m_wnd2BB88->m_visible = 0;
    m_wndLogin->m_visible = 1;
    m_p2BD44 = m_wndLogin;
    SetActiveWindow(m_wndLogin);
    m_wndLogin->ShowWord(id);
}

void VIClient::Msg0777()
{
    m_wndLogin->m_visible = 0;
    printf("Disconnect-12\n");
    m_p4F0EC = -1;
    Disconnect(0, 1);
    Msg0777Done(2);
}

void VIClient::SetSpeed(float v)
{
    VIWalkCamera &cam = m_walkCam;
    m_speed = v;
    cam.m_runSpeed = v;
    cam.InitIntermediate();
    float w = v == 0.0f ? 0.0f : 3.0f;
    cam.m_walkSpeed = w;
    cam.InitIntermediate();
}

void VIClient::DamageFromOther(int attacker, int amount)
{
    if (m_walkCam.m_cameraMode && !m_wndGame->IsModal() && !VIGetGameOptions()->m_x92)
        m_p4FD28.AddDamage(m_selfId, amount, 1);
}

void VIClient::Msg00B8(int index)
{
    for (int i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
        if (m_abilities.Value(i).m_displayIndex == index)
        {
            m_abilities.Erase(i);
            break;
        }
}

void VIClient::Reset()
{
    for (int i = m_p2BD58.Begin(); i != m_p2BD58.End(); i = m_p2BD58.Erase(i))
        SendMsg(1572, m_p2BD58.Value(i).m_data[0], 2);
}

void VIClient::Trade20()
{
    if (m_p2BB04 && m_p2BB00 != 0xFFFFFFFF)
    {
        char name[32];
        unsigned short text[128];
        TradeName(name);
        VIUI &ui = m_ui;
        VIString::snprintf16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(651, ui.m_language), name);
        ShowText(text);
    }
    TradeReset();
}

void VIClient::QuestStage(unsigned char flag, int z)
{
    unsigned short text[64];
    if (flag)
        PlaySound(7);
    if (m_wndGame->IsModal())
        QuestHook();
    VIUI &ui = m_ui;
    VIString::snprintf16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(625, ui.m_language));
    ShowDialog(0, text, 0, 0);
    m_wndGame->Msg001D();
}

void VIClient::Msg0790(VIMemStream &s)
{
    s.Read(&m_p4F0F0, 8, 1);
    s.Read(&m_p4F0F8, 4, 1);
    s.Read(&m_p4F100, 8, 1);
    m_p4F108 = -1;
    Msg0790Done();
}

void VIClient::Msg1003Done()
{
    for (int i = 0; i < 24; i++)
    {
        VIEntity *e = m_entities[i];
        if (e)
        {
            e->Virtual9();
            if (e->Virtual3(130) && e->m_id != m_selfId)
                ((VICreatureEntity *)e)->m_p2B8 = 1;
        }
    }
}

void VIClient::Msg00CD(int index, int lockout)
{
    for (int i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
    {
        VIAbility &a = m_abilities.Value(i);
        if (a.m_displayIndex == index)
        {
            a.m_lockout = lockout;
            return;
        }
    }
}

void VIClient::Msg0019(int slot, int b)
{
    VIItemList &corpse = m_corpse;
    VIInvList &inv = m_inventory;
    VINetItem item;
    corpse.Get(slot, item);
    item.m_p2F0 = -1;
    corpse.Took(slot, inv.AddItem(item, item.m_p38, 40));
}

void VIClient::Msg0776(int caster, int target, int effect)
{
    if (caster == m_selfId)
        return;
    VIEntity *c = FindEntity(caster);
    VIEntity *t = FindEntity(target);
    if (!c || !t)
        return;
    int ca = c->m_actor;
    int ta = t->m_actor;
    int i;
    if (m_dict.FindTyped(effect, (VIResourceType)21, i))
        m_scene.m_spellEffects.Play(i, 0, ca, ta, 0);
}
