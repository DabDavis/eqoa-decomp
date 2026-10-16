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
    m_dialogHead = 0;
    m_dialogTail = 0;
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










































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































// 0x00BC18C0: to a server by name.  A name that does not resolve ends the session (2);
// otherwise a DRDP client on a random local port (another while that one is taken, 64
// tries; 5 when none works), the UDP port through the journal, a connection to the
// server's system endpoint, a hello (0, 18) and the login (0x0904: a zero byte, the
// expansion flags, the game code, the account and the password -- or, when it is empty,
// the 32 bytes at +0x502DC).  Then no player, no target, and the clocks and keys reset.
void VIClient::Connect(const char *host, unsigned short port)
{
    drdp_addr_t addr;
    if (drdp_namelookup(host, &addr) != 0 || addr.m_ucIp[0] == 0)
    {
        printf("Disconnect-1\n");
        Disconnect(2, 0);
        return;
    }
    addr.m_usPort = port;
    int r;
    m_drdp = 0;
    m_conn = 0;
    for (int i = 0; i < 64; i++)
    {
        drdp_addr_t a;
        drdp_addr_t local;
        memset(&local, 0, sizeof(local));
        local.m_usPort = m_random.Range(1025, 32767); a = local;
        r = drdp_create_client(&m_drdp, &a, m_p07568, m_journal.Playback(), 0);
        if (r != -3)
            break;
    }
    if (r != 0)
    {
        printf("Disconnect-2\n");
        Disconnect(5, 0);
        return;
    }
    unsigned short p;
    if (m_journal.m_mode != 2)
    {
        m_journal.Write(drdp_get_port_udp(m_drdp));

    }
    m_journal.Read(&p, 2, 1);
    drdp_set_port_udp(m_drdp, p);
    unsigned long dst;
    drdp_address_to_endpoint(addr.m_ucIp, addr.m_usPort, 0xFFFE, &dst);
    int result = connection_open(&m_conn, drdp_get_system_endpoint(m_drdp), dst);
    VIASSERT(result == 0);
    connection_set_failure(m_conn, 1000, 30000, 60000);
    if (m_p4FC20)
        connection_set_failure(m_conn, 300000, 300000, 300000);
    connection_set_simulated(m_conn, 0, 0, 0);
    {
        VIMemStream out;
        m_p00058 = 0;
        m_p4F0E8 = 20;
        out.WriteU16(0);
        out << VIMemStreamRaw(18);
        VINetSend(out, GetConn(251));
    }
    VIMemStream out;
    int noPass = strlen(m_wnd2BB7C->m_pass) == 0;
    out.WriteU16(0x0904);
    out.WriteByte(0);
    out << VIMemStreamRaw(VIGetExpansionFlags());
    out.WriteString(VIGetGameCode());
    out.WriteString(m_wnd2BB7C->m_user);
    out.WriteByte(noPass);
    if (noPass)
        out.Write(m_p502DC, 1, 32);
    else
        out.WriteString(m_wnd2BB7C->m_pass);
    VINetSend(out, GetConn(251));
    m_p07570 = -1;
    m_selfId = 0xFFFFFFFF;
    m_target = 0xFFFFFFFF;
    m_autoAttack = 0;
    m_p07580 = m_p07568;
    m_p07588 = m_p07568;
    m_p2BB58 = -1;
    m_p4F0F0 = -1;
    m_p4F108 = -1;
    memset(m_keys, 0, sizeof(m_keys));
    memset(&m_p07190, 0, sizeof(m_p07190));
    m_inGroup = 0;
    m_p2BD78 = 0;
    m_p2BE4C = 0;
    m_p382E4 = 0;
    m_clockA.Reset();
    m_clockB.Reset();
    m_effectCount = 0;
}




























// 0x00BC1DD0: to the shard at `addr` -- the old connection closed, a new one opened on the
// system endpoint, then a hello (0, 18) and the login (1): the flag byte, the expansion
// flags, the game code, the account name, and the password -- or, when it is empty, the 32
// bytes at +0x502DC.
void VIClient::ConnectShard(unsigned long addr)
{
    m_p00058 = 2;
    m_wndLogin->Connecting();
    m_p4F0E8 = 20;
    connection_close(m_conn);
    m_conn = 0;
    int result = connection_open(&m_conn, drdp_get_system_endpoint(m_drdp), addr);
    VIASSERT(result == 0);
    if (m_p4FC20)
        connection_set_failure(m_conn, 300000, 300000, 300000);
    {
        VIMemStream out;
        out.WriteU16(0);
        out << VIMemStreamRaw(18);
        VINetSend(out, GetConn(251));
    }
    {
        VIMemStream out;
        int noPass = strlen(m_wnd2BB7C->m_pass) == 0;
        out.WriteU16(1);
        out.WriteByte(m_p4FC20);
        out << VIMemStreamRaw(VIGetExpansionFlags());
        out.WriteString(VIGetGameCode());
        out.WriteString(m_wnd2BB7C->m_user);
        out.WriteByte(noPass);
        if (noPass)
            out.Write(m_p502DC, 1, 32);
        else
            out.WriteString(m_wnd2BB7C->m_pass);
        VINetSend(out, GetConn(251));
    }
}



































































































































































































































































































































































































// 0x00BC2D28, name provisional: one coordinate, quantised over [lo, hi] into
// iBytesPerDimension bytes, written high byte first.
void WriteQuantized(VIMemStream &s, int iBytesPerDimension, float v, float lo, float hi)
{
    unsigned int SCALE = 1 << (iBytesPerDimension * 8);
    VIASSERT(iBytesPerDimension < sizeof(SCALE));
    float range = hi - lo;
    if (range == 0.0f)
        return;
    float t = v - lo; t = VIMIN(range, VIMAX(0.0f, t));
    unsigned int q = (unsigned int)(t * SCALE / range);
    if (q > SCALE - 1)
        q = SCALE - 1;
    for (int i = 0; i < iBytesPerDimension; i++)
    {
        unsigned char b = q >> ((iBytesPerDimension - i - 1) * 8);
        s.Write(&b, 1, 1);
    }
}

// 0x00BC2EB0, name provisional: a point, each axis over the box's extent.
void WriteQuantized(VIMemStream &s, const VIVect3 &v, int iBytesPerDimension, const VIBBox &box)
{
    for (int i = 0; i < 3; i++)
        WriteQuantized(s, iBytesPerDimension, (&v.x)[i], (&box.min.x)[i], (&box.max.x)[i]);
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
        SendOpcode(1915);
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
        s.Read(&m_petId, 4, 1);
        break;

    case 0x007F:
        m_petId = -1;
        m_petName[0] = 0;
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

// VIMap<int, VIAbility>, VIClient's ability map (+0x2BB2C).  VIAbility ends in a VIArray,
// so Allocate copies nodes through an out-of-line Node copy constructor (0x00BE0AB8) and
// DestroyPool/FreeNode destroy through its destructor.
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
template int VIMap<int, VIAbility>::AddNode(int, int);  // 0x00BE0DF0
template int VIMap<int, VIAbility>::Insert(const int &, const VIAbility &);  // 0x00BE1058
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

int VIClient::GetHP() { return m_player.m_stat[7]; }
int VIClient::GetMaxHP() { return m_player.m_stat[8]; }
int VIClient::GetPlayerP30() { return m_player.m_p30; }
int VIClient::GetPower() { return m_player.m_stat[9]; }
int VIClient::GetMaxPower() { return m_player.m_stat[10]; }
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
    SendOpcode(22);
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
    m_musicFade = 0;
    m_pC18C = -1;
    m_musicName[0] = 0;
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

void VIClient::Msg0060(int slot, int hp)
{
    m_inventory.Op0060(slot, hp);
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
    m_inventory.AddItem(item, item.m_maxStack, 40);
    if (IsMerchantOpen())
    {
        PlaySound(3);
        m_p1B304 = 0;
    }
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
        SendOpIntInt(16, m_p2BB68, id);
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
        SendMsg(1572, m_p2BD58.Value(i).m_id, 2);
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
        ResetView();
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
    VIItemList &inv = m_inventory;
    VINetItem item;
    corpse.Get(slot, item);
    item.m_equipSlot = -1;
    corpse.Took(slot, inv.AddItem(item, item.m_maxStack, 40));
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
    {
        VIScene &scene = m_scene;
        scene.m_spellEffects.Play(i, 0, ca, ta, 0);
    }
}

// 0x00BCC368
int VIClient::IsMerchantOpen()
{
    return m_p1B300 != 0xFFFFFFFF;
}

// 0x00BCCCB8: the trade partner's name, or "Unknown".
void VIClient::TradeName(char *buf)
{
    VIEntity *e;
    if (m_p2BB00 == 0xFFFFFFFF || !(e = FindEntity(m_p2BB00)) || !e->Virtual3(130))
        strcpy(buf, "Unknown");
    else
        strcpy(buf, ((VICreatureEntity *)e)->m_name);
}

// 0x00BCCD58
void VIClient::TradeSetup()
{
    m_tradeInv.CopyFrom(m_inventory);
    m_tradeBCount = 0;
    m_tradeACount = 0;
    m_p2BAEC = m_player.m_p34;
    m_p2BAF0 = 0;
    m_p2BAF4 = 0;
    m_p2BAF8 = 0;
    m_p2BAFC = 0;
}

// 0x00BDA168
void VIClient::TradeReset()
{
    m_tradeACount = 0;
    m_p2BAEC = 0;
    m_p2BAF0 = 0;
    m_p2BAF4 = 0;
    m_p2BAF8 = 0;
    m_p2BAFC = 0;
    m_p2BB00 = 0xFFFFFFFF;
    m_p2BB04 = 0;
    m_tradePartner = 0xFFFFFFFF;
    m_tradeBCount = 0;
}

// 0x00BDF870: emitted at the end of the file with the template copies, so an
// inline the TU could not expand where TradeSetup calls it.
void VIItemList::CopyFrom(VIItemList &src)
{
    ItemCount = src.ItemCount;
    for (int i = 0; i < ItemCount; i++)
    {
        ItemInternal[i] = *src.Item(i);
        m_p04++;
    }
}

// ------------------------------------------------------------- item getters --
// The getters the UI windows call (0x00BC98A8..0x00BCD568), in address order.  Each is
// one line; the names are the list's and FrontiersForge's for the field.
// 0x00BC98A8
int VIClient::GetInvCount()
{
    return m_inventory.Count();
}

// 0x00BC98B8
int VIClient::GetInvItemIcon(int i)
{
    return ItemIcon(m_inventory.Item(i));
}

// 0x00BC9918
int VIClient::GetInvItemName(int i, unsigned short *buf, int size)
{
    VIString::strcpy16(buf, size, m_inventory.Item(i)->m_name);
    return 0;
}

// 0x00BC9A10
int VIClient::GetInvItemDesc(int i, unsigned short *buf, int size)
{
    VIString::strcpy16(buf, size, m_inventory.Item(i)->m_desc);
    return 0;
}

// 0x00BC9DD0
int VIClient::ItemIcon(VINetItem *item)
{
    if (VIGetExpansionFlags() & 2 && item->m_icon2)
        return item->m_icon2;
    return item->m_icon;
}

// 0x00BDC438: side 0 is the array at +0x2AEE8.
VITradeItem *VIClient::TradeItem(int side, int i)
{
    if (side)
        return &m_tradeB[i];
    return &m_tradeA[i];
}

// 0x00BC98F0
int VIClient::GetInvItemAmount(int i) { return m_inventory.Item(i)->Quantity; }
// 0x00BC9A60
int VIClient::GetInvItemEquipSlot(int i) { return m_inventory.Item(i)->m_equipSlot; }
// 0x00BC9A88
int VIClient::GetInvItemWearSlot(int i) { return m_inventory.Item(i)->m_wearSlot; }
// 0x00BC9AB0
int VIClient::GetInvItemAttackType(int i) { return m_inventory.Item(i)->m_attackType; }
// 0x00BC9AD8
int VIClient::GetInvItemLevelReq(int i) { return m_inventory.Item(i)->m_levelReq; }
// 0x00BC9B00
int VIClient::GetInvItemP24(int i) { return m_inventory.Item(i)->m_p24; }
// 0x00BC9B28
int VIClient::GetInvItemP08(int i) { return m_inventory.Item(i)->m_p08; }
// 0x00BC9B50
int VIClient::GetInvItemClassMask(int i) { return m_inventory.Item(i)->m_classMask; }
// 0x00BC9B78
int VIClient::GetInvItemRaceMask(int i) { return m_inventory.Item(i)->m_raceMask; }
// 0x00BC9BA0
int VIClient::GetInvItemCharges(int i) { return m_inventory.Item(i)->m_charges; }
// 0x00BC9BC8
int VIClient::GetInvItemP4C(int i) { return m_inventory.Item(i)->m_p4C; }
// 0x00BC9BF0
int VIClient::GetInvItemDamage(int i) { return m_inventory.Item(i)->m_damage; }
// 0x00BC9C18
int VIClient::GetInvItemCurHp(int i) { return m_inventory.Item(i)->m_curHp; }
// 0x00BC9C40
int VIClient::GetInvItemMaxHp(int i) { return m_inventory.Item(i)->m_maxHp; }
// 0x00BC9C68
int VIClient::GetInvItemRange(int i) { return m_inventory.Item(i)->m_range; }
// 0x00BC9C90
int VIClient::GetInvItemMaxStack(int i) { return m_inventory.Item(i)->m_maxStack; }
// 0x00BC9CB8
int VIClient::GetInvItemP58(int i) { return m_inventory.Item(i)->m_p58; }
// 0x00BC9CE0
int VIClient::GetInvItemDurability(int i) { return m_inventory.Item(i)->m_durability; }
// 0x00BC9D08
int VIClient::GetInvItemP18(int i) { return m_inventory.Item(i)->m_p18; }
// 0x00BC9D30
int VIClient::GetInvItemNoTrade(int i) { return m_inventory.Item(i)->m_noTrade; }
// 0x00BC9D58
int VIClient::GetInvItemRent(int i) { return m_inventory.Item(i)->m_rent; }
// 0x00BC9D80
int VIClient::GetInvItemLore(int i) { return m_inventory.Item(i)->m_lore; }
// 0x00BC9DA8
int VIClient::GetInvItemP54(int i) { return m_inventory.Item(i)->m_p54; }
// 0x00BC9F60
int VIClient::GetInvItemQuickSlot(int i) { return m_inventory.Item(i)->m_quickSlot; }
// 0x00BCAE60
int VIClient::GetLootItemAmount(int i) { return m_corpse.Item(i)->Quantity; }
// 0x00BCAED8
int VIClient::GetLootItemLevelReq(int i) { return m_corpse.Item(i)->m_levelReq; }
// 0x00BCAF00
int VIClient::GetLootItemClassMask(int i) { return m_corpse.Item(i)->m_classMask; }
// 0x00BCAF28
int VIClient::GetLootItemRaceMask(int i) { return m_corpse.Item(i)->m_raceMask; }
// 0x00BCAF50
int VIClient::GetLootItemNoTrade(int i) { return m_corpse.Item(i)->m_noTrade; }
// 0x00BCAF78
int VIClient::GetLootItemRent(int i) { return m_corpse.Item(i)->m_rent; }
// 0x00BCAFA0
int VIClient::GetLootItemLore(int i) { return m_corpse.Item(i)->m_lore; }
// 0x00BCAFC8
int VIClient::GetLootItemP4C(int i) { return m_corpse.Item(i)->m_p4C; }
// 0x00BCAFF0
int VIClient::GetLootItemWearSlot(int i) { return m_corpse.Item(i)->m_wearSlot; }
// 0x00BCBA80
int VIClient::GetAbilityIconBack(int i) { return m_abilities.Value(i).m_iconBack; }
// 0x00BCBAA8
int VIClient::GetAbilityIconFront(int i) { return m_abilities.Value(i).m_iconFront; }
// 0x00BCBAD0
int VIClient::GetAbilitySlot(int i) { return m_abilities.Value(i).m_slot; }
// 0x00BCBAF8
int VIClient::GetAbilityDisplayIndex(int i) { return m_abilities.Value(i).m_displayIndex; }
// 0x00BCBB20
int VIClient::GetAbilityLevel(int i) { return m_abilities.Value(i).m_level; }
// 0x00BCBB48
int VIClient::GetAbilityCastTime(int i) { return m_abilities.Value(i).m_castTime; }
// 0x00BCBB70
int VIClient::GetAbilityPowerCost(int i) { return m_abilities.Value(i).m_powerCost; }
// 0x00BCBBC0
int VIClient::GetAbilityCooldown(int i) { return m_abilities.Value(i).m_cooldown; }
// 0x00BCBBE8
int VIClient::GetAbilityScope(int i) { return m_abilities.Value(i).m_scope; }
// 0x00BCBC10
int VIClient::GetAbilityEquipReq(int i) { return m_abilities.Value(i).m_equipReq; }
// 0x00BCBDF0
int VIClient::GetAbilityValid(int i) { return m_abilities.Value(i).m_valid; }
// 0x00BCBE18
int VIClient::GetAbilityP14(int i) { return m_abilities.Value(i).m_p14; }
// 0x00BCC4D0
int VIClient::GetMerchantItemP08(int i) { return m_merchant.Item(i)->m_p08; }
// 0x00BCC4F8
int VIClient::GetMerchantItemWearSlot(int i) { return m_merchant.Item(i)->m_wearSlot; }
// 0x00BCC520
int VIClient::GetMerchantItemP18(int i) { return m_merchant.Item(i)->m_p18; }
// 0x00BCC548
int VIClient::GetMerchantItemP24(int i) { return m_merchant.Item(i)->m_p24; }
// 0x00BCC570
int VIClient::GetMerchantItemAttackType(int i) { return m_merchant.Item(i)->m_attackType; }
// 0x00BCC598
int VIClient::GetMerchantItemDamage(int i) { return m_merchant.Item(i)->m_damage; }
// 0x00BCC5C0
int VIClient::GetMerchantItemRange(int i) { return m_merchant.Item(i)->m_range; }
// 0x00BCC5E8
int VIClient::GetMerchantItemLevelReq(int i) { return m_merchant.Item(i)->m_levelReq; }
// 0x00BCC610
int VIClient::GetMerchantItemMaxHp(int i) { return m_merchant.Item(i)->m_maxHp; }
// 0x00BCC638
int VIClient::GetMerchantItemDurability(int i) { return m_merchant.Item(i)->m_durability; }
// 0x00BCC660
int VIClient::GetMerchantItemClassMask(int i) { return m_merchant.Item(i)->m_classMask; }
// 0x00BCC688
int VIClient::GetMerchantItemRaceMask(int i) { return m_merchant.Item(i)->m_raceMask; }
// 0x00BCC6B0
int VIClient::GetMerchantItemP58(int i) { return m_merchant.Item(i)->m_p58; }
// 0x00BCC6D8
int VIClient::GetMerchantItemMaxStack(int i) { return m_merchant.Item(i)->m_maxStack; }
// 0x00BCC700
int VIClient::GetMerchantItemCharges(int i) { return m_merchant.Item(i)->m_charges; }
// 0x00BCC728
int VIClient::GetMerchantItemCurHp(int i) { return m_merchant.Item(i)->m_curHp; }
// 0x00BCC750
int VIClient::GetMerchantItemNoTrade(int i) { return m_merchant.Item(i)->m_noTrade; }
// 0x00BCC778
int VIClient::GetMerchantItemRent(int i) { return m_merchant.Item(i)->m_rent; }
// 0x00BCC7A0
int VIClient::GetMerchantItemLore(int i) { return m_merchant.Item(i)->m_lore; }
// 0x00BCC7C8
int VIClient::GetMerchantItemP4C(int i) { return m_merchant.Item(i)->m_p4C; }
// 0x00BCCF00
int VIClient::GetTradeInvItemWearSlot(int i) { return m_tradeInv.Item(i)->m_wearSlot; }
// 0x00BCCF28
int VIClient::GetTradeInvItemEquipSlot(int i) { return m_tradeInv.Item(i)->m_equipSlot; }
// 0x00BCCF50
int VIClient::GetTradeInvItemP18(int i) { return m_tradeInv.Item(i)->m_p18; }
// 0x00BCCF78
int VIClient::GetTradeInvItemAttackType(int i) { return m_tradeInv.Item(i)->m_attackType; }
// 0x00BCCFA0
int VIClient::GetTradeInvItemDamage(int i) { return m_tradeInv.Item(i)->m_damage; }
// 0x00BCCFC8
int VIClient::GetTradeInvItemRange(int i) { return m_tradeInv.Item(i)->m_range; }
// 0x00BCCFF0
int VIClient::GetTradeInvItemLevelReq(int i) { return m_tradeInv.Item(i)->m_levelReq; }
// 0x00BCD018
int VIClient::GetTradeInvItemMaxHp(int i) { return m_tradeInv.Item(i)->m_maxHp; }
// 0x00BCD040
int VIClient::GetTradeInvItemDurability(int i) { return m_tradeInv.Item(i)->m_durability; }
// 0x00BCD068
int VIClient::GetTradeInvItemClassMask(int i) { return m_tradeInv.Item(i)->m_classMask; }
// 0x00BCD090
int VIClient::GetTradeInvItemRaceMask(int i) { return m_tradeInv.Item(i)->m_raceMask; }
// 0x00BCD0B8
int VIClient::GetTradeInvItemP58(int i) { return m_tradeInv.Item(i)->m_p58; }
// 0x00BCD0E0
int VIClient::GetTradeInvItemMaxStack(int i) { return m_tradeInv.Item(i)->m_maxStack; }
// 0x00BCD108
int VIClient::GetTradeInvItemAmount(int i) { return m_tradeInv.Item(i)->Quantity; }
// 0x00BCD130
int VIClient::GetTradeInvItemCurHp(int i) { return m_tradeInv.Item(i)->m_curHp; }
// 0x00BCD158
int VIClient::GetTradeInvItemCharges(int i) { return m_tradeInv.Item(i)->m_charges; }
// 0x00BCD180
int VIClient::GetTradeInvItemNoTrade(int i) { return m_tradeInv.Item(i)->m_noTrade; }
// 0x00BCD1A8
int VIClient::GetTradeInvItemRent(int i) { return m_tradeInv.Item(i)->m_rent; }
// 0x00BCD1D0
int VIClient::GetTradeInvItemLore(int i) { return m_tradeInv.Item(i)->m_lore; }
// 0x00BCD1F8
int VIClient::GetTradeInvItemP4C(int i) { return m_tradeInv.Item(i)->m_p4C; }
// 0x00BCD308
int VIClient::GetTradeOfferWearSlot(int side, int i) { return TradeItem(side, i)->m_item.m_wearSlot; }
// 0x00BCD328
int VIClient::GetTradeOfferEquipSlot(int side, int i) { return TradeItem(side, i)->m_item.m_equipSlot; }
// 0x00BCD348
int VIClient::GetTradeOfferP18(int side, int i) { return TradeItem(side, i)->m_item.m_p18; }




// 0x00BD4630: one tick played back out of the journal -- JournalTick's records read back
// and replayed (2 a packet, 3 a key, 4 the pointer, 5 the pad, 6 a quit) up to the record
// that ends the tick (255).  A journal that runs out ends the program.
void VIClient::Tick(int net)
{
    VIJournal &Journal = m_journal;
    int done = 0;
    do
    {
        unsigned char type;
        long time;
        JournalTick(net);
        Journal.Read(type);
        Journal.Read(time);
        if (Journal.GetError())
            exit(0);
        m_p07568 = time;
        if (m_drdp)
            drdp_set_time(m_drdp, m_p07568);
        m_scene.SetTime(m_p07568);
        switch (type)
        {
        case 2: {
            unsigned short size;
            unsigned char buf[0x10000];
            Journal.Read(size);
            Journal.Read(buf, 1, size);
            VIASSERT(!Journal.GetError());
            ProcessPacket(buf, size);
            break; }




        case 3: {
            unsigned char key, down;
            Journal.Read(key);
            Journal.Read(down);
            VIASSERT(!Journal.GetError());
            SetKey(key, down);
            break; }




        case 4: {
            int x, y;
            Journal.Read(x);
            Journal.Read(y);
            VIASSERT(!Journal.GetError());
            Pointer(x, y);
            break; }



        case 5: {
            VIClientPart07190 pad;
            Journal.Read((unsigned char *)&pad, 1, 1);
            Journal.Read((unsigned char *)&pad + 1, 1, 1);
            Journal.Read(pad.m_keys, 4, 20);
            VIASSERT(!Journal.GetError());
            PadKeys(&pad);
            break; }

        case 6:
            m_quit = 1;
            break;

        case 255:
            done = 1;
            break;
        default:
            VIASSERTF(0, "unknown pJournal type %u", type);
            break;
        }
    } while (!done);
    TickHookB();
    ShowNetStats();
}

// 0x00BCD368
int VIClient::GetTradeOfferAttackType(int side, int i) { return TradeItem(side, i)->m_item.m_attackType; }
// 0x00BCD388
int VIClient::GetTradeOfferDamage(int side, int i) { return TradeItem(side, i)->m_item.m_damage; }
// 0x00BCD3A8
int VIClient::GetTradeOfferRange(int side, int i) { return TradeItem(side, i)->m_item.m_range; }
// 0x00BCD3C8
int VIClient::GetTradeOfferLevelReq(int side, int i) { return TradeItem(side, i)->m_item.m_levelReq; }
// 0x00BCD3E8
int VIClient::GetTradeOfferMaxHp(int side, int i) { return TradeItem(side, i)->m_item.m_maxHp; }
// 0x00BCD408
int VIClient::GetTradeOfferDurability(int side, int i) { return TradeItem(side, i)->m_item.m_durability; }
// 0x00BCD428
int VIClient::GetTradeOfferClassMask(int side, int i) { return TradeItem(side, i)->m_item.m_classMask; }
// 0x00BCD448
int VIClient::GetTradeOfferRaceMask(int side, int i) { return TradeItem(side, i)->m_item.m_raceMask; }
// 0x00BCD468
int VIClient::GetTradeOfferP58(int side, int i) { return TradeItem(side, i)->m_item.m_p58; }
// 0x00BCD488
int VIClient::GetTradeOfferMaxStack(int side, int i) { return TradeItem(side, i)->m_item.m_maxStack; }
// 0x00BCD4A8
int VIClient::GetTradeOfferAmount(int side, int i) { return TradeItem(side, i)->m_item.Quantity; }
// 0x00BCD4C8
int VIClient::GetTradeOfferCurHp(int side, int i) { return TradeItem(side, i)->m_item.m_curHp; }
// 0x00BCD4E8
int VIClient::GetTradeOfferCharges(int side, int i) { return TradeItem(side, i)->m_item.m_charges; }
// 0x00BCD508
int VIClient::GetTradeOfferNoTrade(int side, int i) { return TradeItem(side, i)->m_item.m_noTrade; }
// 0x00BCD528
int VIClient::GetTradeOfferRent(int side, int i) { return TradeItem(side, i)->m_item.m_rent; }
// 0x00BCD548
int VIClient::GetTradeOfferLore(int side, int i) { return TradeItem(side, i)->m_item.m_lore; }
// 0x00BCD568
int VIClient::GetTradeOfferP4C(int side, int i) { return TradeItem(side, i)->m_item.m_p4C; }

// --------------------------------------------------------------------- vtable --
// VIClientI's slots 1 and 3-18 (slot 2 is the destructor, slot 17 GetPlayer).
// 0x00BD5EE8
VIClientConn VIClient::GetConn(int type)
{
    return VIClientConn(m_conn, type);
}

VIBankControl *VIClient::GetBank() { return &m_bank; }                  // 0x00BCCB58
VIClientGuild *VIClient::GetGuild() { return &m_guild; }                // 0x00BCDE38
VIClientPart2EF24 *VIClient::GetP2EF24() { return &m_p2EF24; }          // 0x00BCDE48
VIClientPart2EFCC *VIClient::GetP2EFCC() { return &m_p2EFCC; }          // 0x00BCDE58
VIClientPart2EFD0 *VIClient::GetP2EFD0() { return &m_p2EFD0; }          // 0x00BCDE68
VIClientPart33950 *VIClient::GetP33950() { return &m_p33950; }          // 0x00BCDE78
VIClientPart38300 *VIClient::GetP38300() { return &m_p38300; }          // 0x00BCE1A0
VIClientPart382E8 *VIClient::GetP382E8() { return &m_p382E8; }          // 0x00BCE1B0
VIClientPart38334 *VIClient::GetP38334() { return &m_p38334; }          // 0x00BCE1C0
VIClientPart3834C *VIClient::GetP3834C() { return &m_p3834C; }          // 0x00BCE1D0
VIClientPart3FC10 *VIClient::GetP3FC10() { return &m_p3FC10; }          // 0x00BCE1E0
VIClientPart4EC58 *VIClient::GetP4EC58() { return &m_p4EC58; }          // 0x00BCE1F0
VIClientPart4F078 *VIClient::GetP4F078() { return &m_p4F078; }          // 0x00BCE200
VIRandom *VIClient::GetP4F234() { return &m_random; }                   // 0x00BCE7A0
VIClientPart2EFC8 *VIClient::GetP2EFC8() { return &m_p2EFC8; }          // 0x00BD7BD8

// ---------------------------------------------------------- more accessors --
// 0x00BE3880
VIClientPart07190 *VIClient::GetP07190() { return &m_p07190; }
// 0x00BC2118
int VIClient::IsP00058Unset() { return m_p00058 == -1; }
// 0x00BC6108
void VIClient::ClearPC1B0() { m_pC1B0 = 0; }
// 0x00BC70E0
VIColor32F *VIClient::GetPC178() { return &m_fadeColor; }
// 0x00BD6208
int *VIClient::GetPlayerStats() { return m_player.m_stat; }
// 0x00BD6218
int *VIClient::GetPlayerPD8() { return m_player.m_baseStat; }
// 0x00BD83E0
int VIClient::IsConnected() { return m_conn != 0; }
// 0x00BD90E8
int VIClient::IsNotP0662C() { return !m_p0662C; }
// 0x00BD9510
char *VIClient::GetPetName() { return m_petName; }
// 0x00BE3888
int VIClient::GetP13B90() { return m_p13B90; }
// 0x00BE3898
void VIClient::SetP13B90(int v) { m_p13B90 = v; }
// 0x00BE38F0
unsigned short *VIClient::GetMerchantName() { return m_merchantName; }
// 0x00BE3AA0
unsigned short *VIClient::GetP4FC28() { return m_p4FC28; }
// 0x00BE3AB0
unsigned short *VIClient::GetP4FCA8() { return m_p4FCA8; }
// 0x00BE3B28
int VIClient::GetP50304() { return m_p50304; }
// 0x00BCAE08
int VIClient::GetLootCount() { return m_corpse.Count(); }
// 0x00BCC3D8
int VIClient::GetMerchantCount() { return m_merchant.Count(); }
// 0x00BCCE08
int VIClient::GetTradeInvCount() { return m_tradeInv.Count(); }
// 0x00BD5F00
VIClientDrdpRef VIClient::GetDrdpRef(long addr) { return VIClientDrdpRef(m_drdp, addr); }
// 0x00BDAEA0
void VIClient::SetP2BAF4(int v) { m_p2BAF4 = v; }
// 0x00BE3868
void VIClient::SetP4F0EC(int v) { m_p4F0EC = v; }
// 0x00BE38D8
int VIClient::GetP1B304() { return m_p1B304; }
// 0x00BE3B10
int VIClient::GetP382E4() { return m_p382E4; }
// 0x00BCCB68
int VIClient::GetPlayerP38() { return m_player.m_p38; }
// 0x00BD60B0
int VIClient::GetPlayerP40() { return m_player.m_p40; }
// 0x00BE38C8
int VIClient::GetPlayerP34() { return m_player.m_p34; }
// 0x00BCEFA8
int VIClient::GetQuestCount() { return m_questCount; }
// 0x00BD83F0
int VIClient::GetCharCount() { return m_charCount; }
// 0x00BE3A88
int VIClient::GetP2BB74() { return m_p2BB74; }
// 0x00BE3838
int VIClient::GetExitCode() { return m_exitCode; }
// 0x00BE3850
int VIClient::GetDisconnectReason() { return m_p4F0E8; }
// 0x00BE38A8
VIItemList *VIClient::GetInventory() { return &m_inventory; }
// 0x00BE38B8
VIItemList *VIClient::GetInventory2() { return &m_inventory; }
// 0x00BDB788
VIBuddy *VIClient::GetBuddy(int n)
{
    int i = m_buddies.Begin();
    for (int k = 0; k < n; k++)
        i = m_buddies.Next(i);
    return &m_buddies.Value(i);
}

// 0x00BCDE10
int VIClient::GetBuddyCount() { return m_buddies.Size(); }
// 0x00BCDD50
char *VIClient::GetBuddyName(int n) { return GetBuddy(n)->m_name; }
// 0x00BCDD70
unsigned short *VIClient::GetBuddyWName(int n) { return GetBuddy(n)->m_wname; }
// 0x00BCDD90
int VIClient::GetBuddyRace(int n) { return GetBuddy(n)->m_race; }
// 0x00BCDDB0
int VIClient::GetBuddyClass(int n) { return GetBuddy(n)->m_class; }
// 0x00BCDDD0
int VIClient::GetBuddyLevel(int n) { return GetBuddy(n)->m_level; }
// 0x00BCDDF0
int VIClient::GetBuddyOnline(int n) { return GetBuddy(n)->m_online; }
// 0x00BCD220
int VIClient::GetTradeCount(int side)
{
    if (side)
        return m_tradeBCount;
    return m_tradeACount;
}

// 0x00BCD588
int VIClient::GetTradeP2BAF0(int side)
{
    if (side)
        return m_p2BAF0;
    return m_p2BAF4;
}

// 0x00BCBB98
int VIClient::IsAbilityLocked(int i) { return m_abilities.Value(i).m_lockout > 0; }
// 0x00BCBC38
float VIClient::GetAbilityRange(int i) { return m_abilities.Value(i).m_range; }
// 0x00BCC7F0
VIArray<VIItemStat> *VIClient::GetMerchantItemStats(int i) { return &m_merchant.Item(i)->m_stats; }

// 0x00BE3660..0x00BE3678: VIMemStream's, emitted here.
int VIMemStream::GetPos() { return m_rpos; }
int VIMemStream::Remaining() { return m_size - m_rpos; }
const char *VIMemStream::GetBuffer() { return m_rbuf; }

// 0x00BE05B8, 0x00BE31F0: VIItemList's, emitted here.
void VIItemList::SetEquipSlot(int i, int v) { ItemInternal[i].m_equipSlot = v; }
void VIItemList::SetQuickSlot(int i, int v) { ItemInternal[i].m_quickSlot = v; }

// 0x00BB8970 (VIBankControl.cpp's region; 0x00BDEDD0 is this file's dead copy).
VINetItem *VIItemList::Item(int i) { return &ItemInternal[i]; }

template int VIMap<int, VIAbility>::Size() const;  // 0x00BE05F8
template int VIMap<int, VIBuddy>::Size() const;  // 0x00BE0788

// --------------------------------------------------------------------- senders --
// 0x00BDC308: 0x07E0 asks for the DRDP statistics; the answer is 0x07E1 with them as
// text, followed by whatever 0x07E0 carried.
void VIClient::Msg07E0(VIMemStream &s)
{
    unsigned short text[512];
    VINetStats(text, sizeof(text), m_conn);
    VIMemStream out;
    unsigned short op = 0x07E1;
    out.Write(&op, 2, 1);
    out.WriteString(text);
    out.Write(s.GetBuffer() + s.GetPos(), 1, s.Remaining());
    s.Skip(s.Remaining());
    VINetSend(out, GetConn(251));
}

// 0x00BCDA18
void VIClient::RequestBuddy(int n)
{
    const VIBuddy *b = GetBuddy(n);
    VIMemStream out;
    unsigned short op = 873;
    out.Write(&op, 2, 1);
    VINetSend(out << b->m_id, GetConn(251));
}

// 0x00BE4B78
void VIClient::SendOpcode(int op)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out, GetConn(251));
}

// 0x00BD3548
void VIClient::Msg0774(VIMemStream &s)
{
    int id;
    s.Read(&id, 4, 1);
    for (int i = m_p2BD58.Begin(); i != m_p2BD58.End(); i = m_p2BD58.Next(i))
        if (m_p2BD58.Value(i).m_id == id)
        {
            m_p2BD58.Erase(i);
            ShowMsg(635);
            break;
        }
}

// 0x00BCB228
void VIClient::Msg003E(int slot, int charge)
{
    VINetItem item;
    m_inventory.Get(slot, item);
    if (charge)
        m_inventory.UseCharge(slot);
    else
        m_inventory.Remove(slot);
    if (item.m_wearSlot != -1)
    {
        if (item.m_attackType)
            PlaySound(5);
        else
            PlaySound(1);
    }
}

// 0x00BE0478, 0x00BE04C0: VIItemList's, emitted here.
void VIItemList::UseCharge(int slot)
{
    if (--ItemInternal[slot].m_charges == 0)
        Took(slot, 1);
}

void VIItemList::Remove(int slot)
{
    Took(slot, 1);
}

// 0x00BDCBE0
int VIAbility::Read(VIMemStream &s)
{
    s.ReadSigned(m_p00);
    s.ReadSigned(m_displayIndex);
    s.ReadSigned(m_valid);
    s.ReadSigned(m_slot);
    s.ReadSigned(m_lockout);
    s.ReadSigned(m_p14);
    s.ReadSigned(m_level);
    s.Read(&m_range, 4, 1);
    s.ReadSigned(m_castTime);
    s.ReadSigned(m_powerCost);
    s.ReadSigned((int &)m_iconBack);
    s.ReadSigned((int &)m_iconFront);
    s.ReadSigned(m_scope);
    s.ReadSigned(m_cooldown);
    s.ReadSigned((int &)m_equipReq);
    s.ReadString(m_name, sizeof(m_name));
    s.ReadString(m_desc, sizeof(m_desc));
    int n;
    s.ReadSigned(n);
    int r = m_p1BC.Init(n);
    if (r)
        return r;
    for (int i = 0; i < m_p1BC.m_size; i++)
    {
        s.ReadSigned(m_p1BC[i].m_p00);
        s.ReadSigned(m_p1BC[i].m_p04);
        s.ReadSigned(m_p1BC[i].m_p08);
        s.ReadSigned(m_p1BC[i].m_p0C);
        s.ReadString(m_p1BC[i].m_name, sizeof(m_p1BC[i].m_name));
    }
    return 0;
}

// 0x00BCF3A0: one ability, added to the map; the flag shows word 721.
void VIClient::Msg00B3(VIMemStream &s)
{
    VIAbility a;
    int flag;
    s.Read(&m_p2BB60, 4, 1);
    s.Read(&flag, 4, 1);
    a.Read(s);
    m_abilities.Insert(a.m_p00, a);
    if (flag)
    {
        VIUI &ui = m_ui;
        ShowText((const unsigned short *)ui.m_words.GetString(721, ui.m_language));
    }
}

// 0x00BE5748
void VIClient::SendOpcodeByte(int op, unsigned char b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteByte(b);
    VINetSend(out, GetConn(251));
}

// 0x00BD9B48
void VIClient::GroupChanged(unsigned char state)
{
    m_p382E4 = state;
    SendOpcodeByte(14, state);
    switch (m_p382E4)
    {
    case 0:
        ShowMsg(595);
        break;
    case 1:
        ShowMsg(596);
        break;
    case 2:
        ShowMsg(597);
        break;
    case 3:
        ShowMsg(598);
        break;
    case 4:
        ShowMsg(599);
        break;
    }
}

// 0x00BCB138: the copy of the item is taken and never used.
void VIClient::Msg003C(int slot, int n, unsigned char sound)
{
    VINetItem item;
    m_inventory.Get(slot, item);
    m_inventory.Took(slot, n);
    if (IsMerchantOpen() && sound)
    {
        PlaySound(3);
        m_p1B304 = 0;
    }
    else
        m_p13B90 = 0;
}

// 0x00BDB820
void VIClient::Msg0045(int on)
{
    unsigned short text[128];
    VIUI &ui = m_ui;
    VIString::snprintf16(text, sizeof(text), "%S: ", ui.m_words.GetString(550, ui.m_language));
    if (on)
        VIString::strcat16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(551, ui.m_language));
    else
        VIString::strcat16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(552, ui.m_language));
    m_wndGame->AddText(text);
}

// 0x00BCF2D0
void VIClient::Msg00FF(VIMemStream &s)
{
    s.Read(&m_p2BB60, 4, 1);
    for (int i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
        if (m_abilities.Value(i).m_displayIndex < 252)
        {
            m_abilities.Value(i).m_valid = 0;
            m_abilities.Value(i).m_slot = -1;
        }
}

// 0x00BDB6B8
void VIClient::Msg0771(unsigned char type, const char *name)
{
    unsigned short text[256];
    int id;
    switch (type)
    {
    case 2:
        id = 558;
        break;
    case 1:
        id = 559;
        break;
    case 5:
        id = 560;
        break;
    case 6:
        id = 587;
        break;
    case 7:
        id = 589;
        break;
    case 8:
        id = 586;
        break;
    case 9:
        id = 592;
        break;
    case 10:
        id = 642;
        break;
    }
    VIUI &ui = m_ui;
    VIString::snprintf16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(id, ui.m_language), name);
    m_wndGame->AddText(text);
}

// 0x00BD3600
void VIClient::Msg0A7B(VIMemStream &s)
{
    unsigned short text[1024];
    VIRGB rgb;
    s.ReadString(text, sizeof(text));
    s >> rgb;
    VIColor32F c;
    c.Set(rgb.r * (1.0f / 255.0f), rgb.g * (1.0f / 255.0f), rgb.b * (1.0f / 255.0f), 1.0f);
    m_wndGame->AddText(text, c);
}

// 0x00BDBB98
void VIClient::Msg007D(int i)
{
    if (i < 8)
    {
        for (int k = i; k < m_questCount - 1; k++)
            VIString::strcpy16(m_questTitle[k], 256, m_questTitle[k + 1]);
        m_questCount--;
    }
}

// 0x00BCF8B0
void VIClient::Msg00FD(VIMemStream &s)
{
    int v[2];
    s.Read(&m_p2BB60, 4, 1);
    s.Read(&v[0], 4, 1);
    s.Read(&v[1], 4, 1);
    int i = m_abilities.Find(v[0]);
    if (i != m_abilities.End())
    {
        m_abilities.Value(i).m_p14 = v[1];
        if (v[1] == 0)
        {
            m_abilities.Value(i).m_valid = 0;
            m_abilities.Value(i).m_slot = -1;
        }
    }
}

// 0x00BD9F00: the wire's weather byte onto the atmosphere's enum; anything else is clear.
void VIClient::SetWeather(unsigned char w, bool instant)
{
    switch (w)
    {
    case 0:
        m_scene.m_atmosphere.SetWeather(VIAtmosphere::WXCLEAR, instant);
        break;
    case 1:
        m_scene.m_atmosphere.SetWeather(VIAtmosphere::WXRAIN, instant);
        break;
    case 2:
        m_scene.m_atmosphere.SetWeather(VIAtmosphere::WXSNOW, instant);
        break;
    case 3:
        m_scene.m_atmosphere.SetWeather(VIAtmosphere::WXSLEET, instant);
        break;
    case 4:
        m_scene.m_atmosphere.SetWeather(VIAtmosphere::WXSANDSTORM, instant);
        break;
    case 5:
        m_scene.m_atmosphere.SetWeather(VIAtmosphere::WXLAVASTORM, instant);
        break;
    case 6:
        m_scene.m_atmosphere.SetWeather(VIAtmosphere::WXTHUNDERSTORM, instant);
        break;
    default:
        m_scene.m_atmosphere.SetWeather(VIAtmosphere::WXCLEAR, instant);
        break;
    }
}

// 0x00BCB8A0
void VIClient::Msg0040(int slot)
{
    VINetItem item;
    VIItemList &inv = m_inventory;
    int i;
    for (i = 0; i < inv.ItemCount; i++)
        if (inv.Item(i)->m_equipSlot == slot)
            break;
    if (i < inv.ItemCount)
    {
        inv.Get(i, item);
        if (item.m_attackType == 0)
            PlaySound(5);
        else
            PlaySound(1);
    }
    m_inventory.Unequip(slot);
}

// 0x00BDBC70
void VIClient::ShowDialog(int id, const unsigned short *text, int a, int b)
{
    unsigned short buf[218];
    m_text.Expand(buf, 218, text, 1);
    if (m_wndGame->IsLootable())
        m_wndGame->OpenDialog(buf, b);
    else if (!(m_dialogHead == 0 && m_dialogTail == 11) && m_dialogTail != m_dialogHead - 1)
    {
        VIString::strcpy16(m_dialogs[m_dialogTail].m_text, 436, buf);
        if (++m_dialogTail == 12)
            m_dialogTail = 0;
    }
}

// 0x00BE04E0..0x00BE0568: VIItemList's, emitted here.
int VIItemList::IsEquipped(int equip)
{
    int found = 0;
    for (int i = 0; i < ItemCount; i++)
        if (ItemInternal[i].m_equipSlot == equip)
        {
            found = 1;
            break;
        }
    return found;
}

void VIItemList::ReplaceEquipped(int from, int to)
{
    for (int i = 0; i < ItemCount; i++)
        if (ItemInternal[i].m_equipSlot == from)
            ItemInternal[i].m_equipSlot = to;
}

int VIItemList::Unequip(int equip)
{
    if (IsEquipped(equip))
    {
        ReplaceEquipped(equip, -1);
        return 0;
    }
    return -1;
}

// 0x00BDB0E8: the separator buffer holds one character, so only its terminator fits and
// the " " never arrives -- the name runs straight into the word.
void VIClient::Msg00D3(const char *name, unsigned char c)
{
    unsigned short text[128];
    unsigned short space[1];
    VIString::str8to16(space, sizeof(space), " ");
    VIString::str8to16(text, sizeof(text), name);
    VIString::strcat16(text, sizeof(text), space);
    VIUI &ui = m_ui;
    if (c == 0)
        VIString::strcat16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(610, ui.m_language));
    else
        VIString::strcat16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(611, ui.m_language));
    m_wndGame->Prompt(text, 1, 2, 0);
}

// 0x00BE4CF0
void VIClient::SendMsg(int op, int v, unsigned char b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteU32(v);
    out.WriteByte(b);
    VINetSend(out, GetConn(251));
}

// 0x00BD9D60: FrontiersForge's target info (disposition 0..6).
void VIClient::Msg0010(VIMemStream &s)
{
    unsigned char disp, b, c;
    int id;
    s >> disp >> b >> c;
    s.Read(m_p4FCA8, 2, 64);
    s.Read(m_p4FC28, 2, 64);
    s.Read(&id, 4, 1);
    if (id == m_p2BB68)
    {
        m_targetInfoValid = 1;
        m_targetDisposition = disp;
        m_p2BB70 = b;
        m_p2BB74 = c;
    }
}

// 0x00BD98B0: a slot whose stack did not all fit stays, so the index moves on only then.
void VIClient::Msg001A()
{
    VINetItem item;
    VIItemList &corpse = m_corpse;
    int i = 0;
    while (i < corpse.ItemCount)
    {
        corpse.Get(i, item);
        item.m_equipSlot = -1;
        int n = m_inventory.AddItem(item, item.m_maxStack, 40);
        corpse.Took(i, n);
        if (n != item.Quantity)
            i++;
    }
}

// 0x00BD3458: already grouped, decline at once; otherwise queue it and show it.
void VIClient::Msg0773(VIMemStream &s)
{
    int id, x;
    char name[24];
    s.Read(&id, 4, 1);
    s.Read(&x, 4, 1);
    s.ReadString(name, 24);
    if (InGroup())
        SendMsg(1572, id, 2);
    else
    {
        VIRec32_BE2DE8 r;
        r.m_id = id;
        r.m_p04 = x;
        VIString::strcpy8(r.m_name, 24, name);
        m_p2BD58.Add(r);
        if (m_wndGame->IsLootable())
            m_wndGame->ShowInvite(name);
    }
}

// 0x00BE4FC0
void VIClient::SendOpIntInt(int op, int a, unsigned int b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteU32(a);
    out.WriteU32(b);
    VINetSend(out, GetConn(251));
}

// 0x00BE5C28
void VIClient::SendOpByteInt(int op, unsigned char b, int v)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteByte(b);
    out.WriteU32(v);
    VINetSend(out, GetConn(251));
}

// 0x00BC7098
void VIClient::SetFade(float to, float time)
{
    if (time == 0.0f)
    {
        m_fadeTo = to;
        m_fadeCur = to;
        m_fadeRate = 0.0f;
    }
    else
    {
        m_fadeTo = to;
        m_fadeRate = (to - m_fadeCur) / time;
    }
}

// 0x00BD7618: both windows hear message 295 with (old, new); the loader shows its
// progress only while m_p2BB8C is the active window.
void VIClient::SetActiveWindow(VIWnd *w)
{
    SetFade(0.0f, 0.0f);
    if (m_p2BD40)
        m_p2BD40->OnMessage(m_p2BD40, 295, (int)m_p2BD40, (int)w);
    if (w)
        w->OnMessage(m_p2BD40, 295, (int)m_p2BD40, (int)w);
    m_p2BD40 = w;
    w->SetFocus(w, false);
    if (m_p2BD40 == m_p2BB8C)
        m_loader.SetProgressCallback(LoadingProgress, this);
    else
        m_loader.SetProgressCallback(0, 0);
}

// 0x00BC2528
int VIClient::Msg0777Done(int mode)
{
    printf("CHANGING RETURN FLOW %d\n", mode);
    switch (mode)
    {
    case 1:
        break;
    case 3:
        m_quit = 1;
        m_exitCode = 0;
        SetActiveWindow(0);
        break;
    case 2:
        if (m_worldLoaded)
            UnloadWorld();
        if (!m_p4F110 && LoadWorld() < 0)
        {
            m_quit = 1;
            m_exitCode = -1;
        }
        ShowLoginWindow();
        break;
    case 4:
        m_quit = 1;
        m_exitCode = -1;
        ezNetCtlDownInterface(0);
        break;
    }
    return 0;
}

// 0x00BDA1A8
void VIClient::Msg0057(int partner, unsigned int id)
{
    m_tradePartner = partner;
    m_p2BB00 = id;
    if (m_wndGame->IsLootable())
    {
        m_p2BB04 = 0;
        m_wndGame->TradeOffer(id);
    }
    else
    {
        char name[32];
        unsigned short text[128];
        VIEntity *e = FindEntity(id);
        if (e && e->Virtual3(130))
            strcpy(name, ((VICreatureEntity *)e)->m_name);
        else
            strcpy(name, "Someone");
        VIUI &ui = m_ui;
        VIString::snprintf16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(626, ui.m_language), name);
        ShowText(text);
        SendOpByteInt(86, 20, partner);
        TradeReset();
    }
}

// 0x00BDC470
int VIClient::FitsSlot3(VINetItem *it)
{
    switch (it->m_wearSlot)
    {
    case -1: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
    case 10: case 11: case 13: case 19: case 21: case 22: case 23: case 24: case 25:
    case 26: case 27:
        return 0;
    case 12: case 14: case 15: case 16: case 18: case 20:
        return 1;
    }
    return 0;
}

// 0x00BD9C18: the pos == 255 test can never pass after pos < 4.
void VIClient::Msg000A(unsigned char slot, unsigned char pos)
{
    if (pos < 4)
    {
        int prev = -1;
        if (pos == 255)
        {
            m_inventory.SetQuickSlot(slot, 255);
            return;
        }
        for (int i = 0; i < GetInvCount(); i++)
            if (m_inventory.Item(i)->m_quickSlot == pos)
            {
                prev = i;
                break;
            }
        VINetItem *it = m_inventory.Item(slot);
        if ((it->m_p4C == 2 && pos < 2) || (it->m_p4C == 7 && pos == 2) || (FitsSlot3(it) && pos == 3))
        {
            if (prev != -1)
                m_inventory.SetQuickSlot(prev, -1);
            m_inventory.SetQuickSlot(slot, pos);
        }
    }
}

// 0x00BDE050
void VIClient::Msg0790Done()
{
    if (m_conn)
    {
        int timeout;
        connection_get_timeout(m_conn, &timeout);
        timeout = timeout > 1000 ? 1000 : timeout;
        if (m_p4F0F0 != -1 && m_p07568 >= m_p4F108 + timeout)
        {
            {
                VIMemStream out;
                unsigned short op = 2450;
                out.Write(&op, 2, 1);
                out.WriteU32(m_p4F0F8);
                out.WriteU64(m_p4F100);
                VINetSendTo(out, GetDrdpRef(m_p4F0F0));
            }
            m_p4F108 = m_p07568;
        }
    }
}

// 0x00BDA528: the trade sub-opcodes while +0x2BB04 is set (6 and 7 are the window's).
void VIClient::TradeA(unsigned char sub, VIMemStream &s)
{
    VINetItem item;
    int a, b, c;
    switch (sub)
    {
    case 3:
        s.Read(&a, 4, 1);
        s.ReadSigned(b);
        TradeAddSlot(a, b);
        break;
    case 4:
        s.Read(&a, 4, 1);
        TradeRemoveSlot(a);
        break;
    case 5:
        s.ReadSigned(c);
        TradeSetMoney(c);
        break;
    case 8:
        ReadNetItem(s, item);
        TradeAddItem(item);
        break;
    case 9:
        s.Read(&a, 4, 1);
        TradeRemoveItem(a);
        break;
    case 10:
        s.ReadSigned(c);
        SetP2BAF4(c);
        break;
    }
}

// 0x00BDA688: the same handlers, numbered the other way round.
void VIClient::TradeB(unsigned char sub, VIMemStream &s)
{
    VINetItem item;
    int a, b, c;
    switch (sub)
    {
    case 3:
        ReadNetItem(s, item);
        TradeAddItem(item);
        break;
    case 4:
        s.Read(&a, 4, 1);
        TradeRemoveItem(a);
        break;
    case 5:
        s.ReadSigned(b);
        SetP2BAF4(b);
        break;
    case 8:
        s.Read(&a, 4, 1);
        s.ReadSigned(c);
        TradeAddSlot(a, c);
        break;
    case 9:
        s.Read(&a, 4, 1);
        TradeRemoveSlot(a);
        break;
    case 10:
        s.ReadSigned(b);
        TradeSetMoney(b);
        break;
    }
}

// 0x00BDAE78
void VIClient::TradeSetMoney(int money)
{
    m_p2BAF0 = money;
    m_p2BAEC = m_player.m_p34 - money;
}

// 0x00BCF5A8
void VIClient::Msg00B5(int from, int to)
{
    int found = -1;
    int i;
    for (i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
    {
        VIAbility &a = m_abilities.Value(i);
        if (a.m_valid)
        {
            if (a.m_slot == from)
                found = i;
            if (from < a.m_slot)
                a.m_slot--;
        }
    }
    if (from < to)
        to--;
    for (i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
    {
        VIAbility &a = m_abilities.Value(i);
        if (a.m_valid && a.m_slot >= to)
            a.m_slot++;
    }
    if (found != -1)
        m_abilities.Value(found).m_slot = to;
}

// 0x00BCF730
void VIClient::Msg00B6(int from, int to)
{
    int found = -1;
    int i;
    for (i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
    {
        VIAbility &a = m_abilities.Value(i);
        if (a.m_displayIndex == from)
            found = i;
        if (from < a.m_displayIndex && a.m_displayIndex < 252)
            a.m_displayIndex--;
    }
    for (i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
    {
        VIAbility &a = m_abilities.Value(i);
        if (a.m_displayIndex >= to && a.m_displayIndex < 252)
            a.m_displayIndex++;
    }
    if (found != -1)
        m_abilities.Value(found).m_displayIndex = to;
}

// 0x00BD8E98
int VIClient::PlayCue(const char *name, float vol)
{
    return m_ui.SoundPlay(name, vol);
}

// 0x00BCD980
void VIClient::TradeCancel()
{
    SendOpByteInt(86, 1, m_tradePartner);
    Trade16();
}

// 0x00BCC388
int VIClient::MerchantClose()
{
    m_wndGame->MerchantClose();
    m_p1B300 = 0xFFFFFFFF;
    return 0;
}

// 0x00BE52C8
void VIClient::SendOpSigned(int op, int v)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << v, GetConn(251));
}

// 0x00BDAEB8: the death pose, the spells it cast stopped, the time stamped; the cue,
// every pending invite declined (3), the camera reset.
void VIClient::Msg005E()
{
    VIEntity *e = FindEntity(m_selfId);
    if (e)
    {
        VIScene &scene = m_scene;
        if (scene.GetActorSpriteType(e->m_actor) == 5)
        {
            ((VICSprite *)scene.Sprite(scene.GetActorSprite(e->m_actor)))->SetDeath(1, 0, scene);
            if (e->Virtual3(130))
            {
                scene.m_spellEffects.StopIfThisWasCaster(e->m_actor);
                ((VICreatureEntity *)e)->m_deathTime = scene.GetTime();
            }
        }
    }
    m_wndGame->SetMode(0);
    PlayCue("death", 0.4f);
    m_p00074 = 0;
    m_p00078 = 0;
    StopAutoAttack();
    ResetBattleMusic();
    for (int i = m_p2BD58.Begin(); i != m_p2BD58.End(); i = m_p2BD58.Erase(i))
        SendMsg(1572, m_p2BD58.Value(i).m_id, 3);
    ResetCamera();
    m_p2BB40 = 0;
    m_p2BB48 = 0;
}

// 0x00BD38C8
void VIClient::Msg00B2(VIMemStream &s)
{
    VIVect3 pos, dir;
    char world;
    int x;
    m_walkCam.m_effect.Reset();
    m_walkCam.m_effect.EndManual(0.0f);
    s.Read(&world, 1, 1);
    s >> pos;
    s >> dir;
    s.Read(&x, 4, 1);
    if (m_selfId != 0xFFFFFFFF)
    {
        m_wndGame->SetMode(0);
        m_p1B304 = 0;
        m_bank.Close();
        m_p13B90 = 0;
        Respawn(world, pos, dir);
        if (m_p2BB00 != 0xFFFFFFFF)
            TradeCancel();
        if (HaveCorpse)
            LootNoWindow();
        if (m_p1B300 != 0xFFFFFFFF)
            MerchantClose();
        if (m_p3FC10.IsActive())
            m_p3FC10.Cancel();
    }
    SendOpSigned(73, x);
}

// 0x00BD3738: the caster's, the target's and every listed target's scene actor (the
// list ends in -1); a cast of my own stamps the cast window; then the effect.
void VIClient::CastingCombat(unsigned int caster, unsigned int target, unsigned char n, int *targets, int time, int effect)
{
    int actors[8];
    int i;
    int casterActor, targetActor;
    if (caster == 0xFFFFFFFF)
        casterActor = -1;
    else
    {
        VIEntity *e = FindEntity(caster);
        casterActor = e ? e->m_actor : -1;
    }
    if (target == 0xFFFFFFFF)
        targetActor = -1;
    else
    {
        VIEntity *e = FindEntity(target);
        targetActor = e ? e->m_actor : -1;
    }
    int count = 0;
    for (int k = 0; k < n; k++)
    {
        VIEntity *e = FindEntity(targets[k]);
        if (e)
            actors[count++] = e->m_actor;
    }
    actors[count] = -1;
    if (caster == m_selfId)
    {
        long now = m_p07568;
        m_p2BB40 = now;
        long end = time + now;
        if (end < m_p2BB48)
            end = m_p2BB48;
        m_p2BB48 = end;
    }
    if (casterActor != -1 && targetActor != -1 && m_dict.FindTyped(effect, (VIResourceType)21, i))
    {
        VIScene &scene = m_scene;
        scene.m_spellEffects.Play(i, time, casterActor, targetActor, actors);
    }
}

// 0x00BCE5D8: a copy of the committed body takes the view's heading and is set back
// as the walker's location; an orbiting camera swings round behind (PI from it).
void VIClient::ResetView()
{
    VIWalkPhysics p;
    VIEntity *e = FindEntity(m_selfId);
    if (e)
    {
        VIWalkCamera &cam = m_walkCam;
        if (cam.m_cameraMode != 3)
        {
            p = m_walkCam.m_committed;
            p.m_angles.x = m_walkCam.m_effect.ManualHpr().x;
            cam.VIWalkControl::SetLocation(p.m_position, p.m_angles, m_scene);
            e->SetHpr(p.m_angles);
            if (cam.m_cameraMode)
            {
                cam.ChooseOrbitAngle(m_walkCam.m_committed.m_angles.x + 3.14159274f, m_scene);
                m_walkCam.m_effect.EndManual(0.2f);
            }
            else
                m_walkCam.m_effect.EndManual(0.0f);
        }
        else
        {
            cam.ChooseOrbitAngle(m_walkCam.m_effect.ManualHpr().x + 3.14159274f, m_scene);
            m_walkCam.m_effect.EndManual(0.2f);
        }
        m_wndGame->Refresh();
    }
}

// 0x00BDB3E8: open -- the second reply's stack temporaries sit in the text buffer's
// freed space in the target; and the snprintf16 argument order (see Msg0770).
void VIClient::Msg076F(VIMemStream &s)
{
    s.Read(&m_p2BB10, 4, 1);
    s.ReadString(m_buddyRequestName, 24);
    if (!m_wndGame->IsLootable())
    {
        unsigned short text[128];
        VIUI &ui = m_ui;
        VIString::snprintf16(text, sizeof(text), (const unsigned short *)ui.m_words.GetString(649, ui.m_language), m_buddyRequestName);
        ShowText(text);
        {
            VIMemStream out;
            unsigned short op = 872;
            out.Write(&op, 2, 1);
            out.WriteU32(m_p2BB10);
            out.WriteByte(1);
            VINetSend(out, GetConn(251));
        }
        m_p2BB10 = 0xFFFFFFFF;
    }
    else if (GetBuddyCount() >= 32)
    {
        {
            VIMemStream out;
            unsigned short op = 872;
            out.Write(&op, 2, 1);
            out.WriteU32(m_p2BB10);
            out.WriteByte(8);
            VINetSend(out, GetConn(251));
        }
        m_p2BB10 = 0xFFFFFFFF;
    }
    else
        m_wndGame->ShowBuddyRequest();
}

// 0x00BD2E80: open -- the target recomputes this + i*96 + K for each raw read instead of
// reusing the record base the zigzag reads go through.
void VIClient::CharacterList(VIMemStream &s)
{
    s.ReadSigned(m_charCount);
    m_charCount = m_charCount > 8 ? 8 : m_charCount;
    for (int i = 0; i < m_charCount; i++)
    {
        VICharRecord &c = m_chars[i];
        s.ReadString(c.m_name, 24);
        s.ReadSigned(c.m_serverId);
        s.ReadSigned(c.m_modelId);
        s.ReadSigned(c.m_class);
        s.ReadSigned(c.m_race);
        s.ReadSigned(c.m_level);
        s.ReadSigned(c.m_hairColor);
        s.ReadSigned(c.m_hairLength);
        s.ReadSigned(c.m_hairStyle);
        s.ReadSigned(c.m_face);
        s.Read(&m_chars[i].m_robe, 4, 1);
        s.Read(&m_chars[i].m_p40, 1, 1);
        s.Read(m_chars[i].m_weapons, 4, 3);
        s.Read(m_chars[i].m_p50, 1, 3);
        s.Read(m_chars[i].m_armor, 1, 6);
        s.Read(m_chars[i].m_p59, 1, 6);
    }
    m_wnd2BB88->ShowList();
    m_wnd2BB88->m_visible = 0;
    m_p4F130 = 1;
    m_wndLogin->m_visible = 1;
    m_p2BD44 = m_wndLogin;
    SetActiveWindow(m_wndLogin);
    if (m_p50300)
        m_wndLogin->Option(1);
}

// ------------------------------------------------------------ small wrappers --
// 0x00BE4C28
void VIClient::SendOpInt(int op, int v)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteU32(v);
    VINetSend(out, GetConn(251));
}

// 0x00BCB0B8
void VIClient::RequestLootAll() { SendOpcode(26); }
// 0x00BC2248
void VIClient::Quit()
{
    m_quit = 1;
    m_exitCode = -1;
}

// 0x00BC8B08
void VIClient::SendOp000F(int v) { SendOpInt(15, v); }
// 0x00BCB018
void VIClient::SendOp0015(int v) { SendOpInt(21, v); }
// 0x00BD8348
void VIClient::SendOp0626(int v) { SendOpInt(1574, v); }
// 0x00BD8408
VICharRecord *VIClient::GetChar(int i) { return &m_chars[i]; }
// 0x00BD7B10
VIPartyMember *VIClient::GetGroupMember(int i) { return &m_group[i]; }
// 0x00BD50C8
void VIClient::SetP50308(int a, int b, int c)
{
    m_p50308 = 1;
    m_p5030C = a;
    m_p50310 = b;
    m_p50314 = c;
}

// 0x00BD5F18
void VIClient::SendRaw(const void *data, int size) { connection_send(m_conn, data, size, 251); }

// ------------------------------------------------------- panel forwarders --
void VIClient::ClosePanel6FC() { m_wndGame->ClosePanel6FC(); }              // 0x00BCE150
void VIClient::ClosePanel700() { m_wndGame->ClosePanel700(); }              // 0x00BCE178
void VIClient::OpenPanel27() { m_wndGame->OpenPanel27(); }                  // 0x00BCE7B0
void VIClient::ClosePanel6EC() { m_wndGame->ClosePanel6EC(); }              // 0x00BCE7D8
void VIClient::OpenPanel22() { m_wndGame->OpenPanel22(); }                  // 0x00BCEC80
void VIClient::ClosePanel6DC() { m_wndGame->ClosePanel6DC(); }              // 0x00BCECA8
void VIClient::OpenPanel24() { m_wndGame->OpenPanel24(); }                  // 0x00BCED60
void VIClient::ClosePanel6E4() { m_wndGame->ClosePanel6E4(); }              // 0x00BCED88
void VIClient::OpenPanel23() { m_wndGame->OpenPanel23(); }                  // 0x00BCEDB0
void VIClient::ClosePanel6E0() { m_wndGame->ClosePanel6E0(); }              // 0x00BCEDD8
void VIClient::OpenPanel28() { m_wndGame->OpenPanel28(); }                  // 0x00BCEE00
void VIClient::ClosePanel6F4() { m_wndGame->ClosePanel6F4(); }              // 0x00BCEE28
void VIClient::OpenPanel29() { m_wndGame->OpenPanel29(); }                  // 0x00BCEE50
void VIClient::ClosePanel6F8() { m_wndGame->ClosePanel6F8(); }              // 0x00BCEE78

// 0x00BCAB78
VIArray<VIItemStat> *VIClient::GetInvItemStats(int i) { return &m_inventory.Item(i)->m_stats; }
void VIClient::ClosePanel708() { m_wndGame->ClosePanel708(); }              // 0x00BCEF30
void VIClient::ClosePanel70C() { m_wndGame->ClosePanel70C(); }              // 0x00BCEF58
void VIClient::OpenPanel18() { m_wndGame->OpenPanel18(); }                  // 0x00BCDCE8
// 0x00BD5F38
void VIClient::ClearTarget()
{
    m_targetInfoValid = 0;
    m_target = 0xFFFFFFFF;
}

// 0x00BD6088
void VIClient::SendOp0035(int v, unsigned char b) { SendMsg(53, v, b); }
// 0x00BD91B0
void VIClient::ShowText(const char *s) { m_wndGame->AddText(s); }
// 0x00BE3AC0
int VIClient::ExpandText(unsigned short *dst, unsigned short size, const unsigned short *src, int flag)
{
    return m_text.Expand(dst, size, src, flag);
}

// 0x00BE3AE8
int VIClient::IsTargetPet() { return m_target == m_petId; }
// 0x00BE3B58
VINetItem *VIClient::GetLootItem(int i) { return m_corpse.Item(i); }
// 0x00BE3B80
VINetItem *VIClient::GetMerchantItem(int i) { return m_merchant.Item(i); }
// 0x00BE3BA8
VINetItem *VIClient::GetTradeInvItem(int i) { return m_tradeInv.Item(i); }

// 0x00BBFE00
void ReleaseMat(VIRaster &r, VIDictionary &d, int mat)
{
    if (mat != -1)
        r.ReleaseMaterial(mat, d);
}

// 0x00BC70F0
void VIClient::SetPC178(const VIColor32F &v) { m_fadeColor = v; }

// 0x00BC8AD8
int VIClient::ProcessPacket(const void *data, int size)
{
    m_p2BB50 = m_p07568;
    return drdp_process_packet(m_drdp, data, size);
}

// 0x00BCCB88
void VIClient::Send0063()
{
    m_p22B78 = 0xFFFFFFFF;
    SendOpcode(99);
}

// 0x00BCD248
int VIClient::GetTradeOfferIcon(int side, int i) { return ItemIcon(&TradeItem(side, i)->m_item); }

// 0x00BCD9C8
void VIClient::GetBuddyRequestName(char *buf) { VIString::strcpy8(buf, 24, m_buddyRequestName); }

// 0x00BD5B30
void VIClient::MaybeStopAutoAttack()
{
    if (!VIGetGameOptions()->m_x95)
        m_autoAttack = 0;
}

// 0x00BD91D8
void VIClient::PromptText(const unsigned short *text, int a) { m_wndGame->Prompt(text, a, 0, 0); }

// 0x00BC8B28: a sort by the second float (func_00BC8B98 takes its address).
static int CompareY(const void *a, const void *b)
{
    float d = ((const float *)a)[1] - ((const float *)b)[1];
    int r;
    if (d > 0.0f)
        r = 1;
    else if (d < 0.0f)
        r = -1;
    else
        r = 0;
    return r;
}

// 0x00BD8FB8
int VIClient::SetMusic(const char *name)
{
    m_pC18C = -1;
    strcpy(m_musicName, name);
    return 0;
}

// 0x00BDD4E8
int VIClient::ExpansionFlags() { return VIGetExpansionFlags() & m_p50304; }

int VIClient::GetLootItemIcon(int i) { return ItemIcon(m_corpse.Item(i)); }          // 0x00BCAE20
int VIClient::GetMerchantItemIcon(int i) { return ItemIcon(m_merchant.Item(i)); }    // 0x00BCC3F0
int VIClient::GetTradeInvItemIcon(int i) { return ItemIcon(m_tradeInv.Item(i)); }    // 0x00BCCE20

// 0x00BCBD70, 0x00BCBDB0
void VIClient::GetAbilityName(int i, int unused, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, m_abilities.Value(i).m_name);
}

void VIClient::GetAbilityDesc(int i, int unused, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, m_abilities.Value(i).m_desc);
}

// 0x00BD7AD0
int VIClient::GetP2BE4C()
{
    if (InGroup())
        return m_p2BE4C;
    return 0;
}

// 0x00BE5478
void VIClient::SendOpSigned2Int(int op, int a, int b, unsigned int c)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << a << b << VIMemStreamRaw(c), GetConn(251));
}

// 0x00BCCA38
void VIClient::MerchantRequest(int a, int b)
{
    SendOpSigned2Int(74, a, b, m_p1B300);
    m_p1B304 = 1;
}

// 0x00BC85F8
void VIClient::TickLocal() { Tick(0); }

// 0x00BC8618
void VIClient::TickNet()
{
    Tick(1);
    if (m_drdp)
    {
        Msg0790Done();
        drdp_flush(m_drdp);
    }
}

// 0x00BCCBB8
int VIClient::IsInvItemUsable(int i)
{
    VINetItem *it = m_inventory.Item(i);
    if (it->m_wearSlot == -1 && !it->m_p54)
        return 0;
    return 1;
}

// 0x00BCD278
int VIClient::GetTradeOfferName(int side, int i, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, TradeItem(side, i)->m_item.m_name);
    return 0;
}

// 0x00BCD2C0
int VIClient::GetTradeOfferDesc(int side, int i, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, TradeItem(side, i)->m_item.m_desc);
    return 0;
}

// 0x00BCAE88
int VIClient::GetLootItemName(int i, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, m_corpse.Item(i)->m_name);
    return 0;
}

// 0x00BCC430
int VIClient::GetMerchantItemName(int i, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, m_merchant.Item(i)->m_name);
    return 0;
}

// 0x00BCC480
int VIClient::GetMerchantItemDesc(int i, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, m_merchant.Item(i)->m_desc);
    return 0;
}

// 0x00BCCE60
int VIClient::GetTradeInvItemName(int i, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, m_tradeInv.Item(i)->m_name);
    return 0;
}

// 0x00BCCEB0
int VIClient::GetTradeInvItemDesc(int i, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, m_tradeInv.Item(i)->m_desc);
    return 0;
}

// 0x00BCBD20
void VIClient::GetAbilityRecordName(int i, int j, unsigned short *buf, unsigned int size)
{
    VIString::strcpy16(buf, size, m_abilities.Value(i).m_p1BC[j].m_name);
}

// 0x00BCD5B0
void VIClient::TradeSend2()
{
    if (m_p2BB00 == 0xFFFFFFFF)
        Trade16();
    else
        SendOpByteInt(86, 2, m_tradePartner);
}

// 0x00BCD8E0
void VIClient::TradeSend6()
{
    if (m_p2BB00 == 0xFFFFFFFF)
        Trade16();
    else
        SendOpByteInt(86, 6, m_tradePartner);
}

// 0x00BCD930
void VIClient::TradeSend7()
{
    if (m_p2BB00 == 0xFFFFFFFF)
        Trade16();
    else
        SendOpByteInt(86, 7, m_tradePartner);
}

// 0x00BBBDD0, VIItemList.h:315 -- every TU carries a copy (0x00BDFFE0 is this file's
// dead one); the assert's text gives the original member names.
#line 313 "C:/code-frontiers-gm/game/VIItemList.h"
int VIItemList::GetSlotId(int SlotIndex)
{
    VIASSERT(SlotIndex < ItemCount);
    return ItemInternal[SlotIndex].m_invIndex;
}
#line 324 "C:/code-frontiers-gm/game/VIItemList.h"
void VIItemList::Took(int Slot, int Quantity)
{
    VIASSERT(Slot < ItemCount && ItemInternal[ Slot ].Quantity >= Quantity);
    ItemInternal[Slot].Quantity -= Quantity;
    if (ItemInternal[Slot].Quantity == 0)
    {
        for (; Slot + 1 < ItemCount; Slot++)
            ItemInternal[Slot] = ItemInternal[Slot + 1];
        ItemCount--;
    }
}
#line 1 "C:/code-frontiers-gm/game/VIClient-tail.cpp"

// 0x00BE50B0
void VIClient::SendOpIntIntByte(int op, int a, unsigned int b, unsigned char c)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteU32(a);
    out.WriteU32(b);
    out.WriteByte(c);
    VINetSend(out, GetConn(251));
}

// 0x00BE4DE0
void VIClient::SendItemPair(int op, int a, int b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteU32(a);
    out.WriteU32(b);
    VINetSend(out, GetConn(251));
}

// 0x00BE4ED0
void VIClient::SendLoot(int op, int slot, int x)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteU32(slot);
    out.WriteU32(x);
    VINetSend(out, GetConn(251));
}

// 0x00BE51C0
void VIClient::SendOpIntSignedInt(int op, int a, int b, unsigned int c)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamRaw(a) << b << VIMemStreamRaw(c), GetConn(251));
}

// 0x00BCADB8
void VIClient::RequestQuickSlot(int i, int pos) { SendMsg(10, m_inventory.GetSlotId(i), pos); }

// 0x00BCCC70
void VIClient::Send0062(int i) { SendOpIntInt(98, m_inventory.GetSlotId(i), m_p22B78); }

// 0x00BCABA0
void VIClient::UseItem(int i, unsigned char b)
{
    if (i < m_inventory.Count())
        SendOpIntIntByte(62, m_inventory.GetSlotId(i), m_target, b);
}

// 0x00BCAC08
void VIClient::SwapItems(int i, int j)
{
    VIItemList &inv = m_inventory;
    if (i < inv.ItemCount && j < inv.ItemCount)
        SendItemPair(61, inv.GetSlotId(i), inv.GetSlotId(j));
}

// 0x00BCB038
void VIClient::LootOne(int i, int x)
{
    if (i < m_corpse.Count())
        SendLoot(25, m_corpse.GetSlotId(i), x);
}

// 0x00BCCA80
void VIClient::SellItem(int i, int n)
{
    VIItemList &inv = m_inventory;
    if (i < inv.ItemCount && inv.Item(i)->Quantity >= n)
    {
        SendOpIntSignedInt(75, inv.GetSlotId(i), n, m_p1B300);
        m_p1B304 = 1;
    }
}

// 0x00BCA8F0
int VIPlayer::CanUse(VINetItem *it)
{
    int one = 1;
    int classBit = one << m_class;
    int raceBit = one << m_race;
    if ((it->m_classMask & classBit) == 0)
        return 0;
    if ((it->m_raceMask & raceBit) == 0)
        return 0;
    if (m_level < it->m_levelReq)
        one = 0;
    return one;
}

// 0x00BC24C8
int VIClient::DisconnectKind()
{
    switch (m_p4F0E8)
    {
    case 3: case 4: case 8: case 9:
        return 3;
    case 0: case 1: case 2: case 6: case 7: case 10: case 13: case 15: case 17: case 19:
        return 2;
    case 5: case 11: case 12: case 14: case 16: case 18:
        return 4;
    case 20: case 21:
        return 0;
    }
    return 0;
}

// 0x00BCA950
int VIClient::CanUseItem(VINetItem *it) { return m_player.CanUse(it); }

// 0x00BCAB18
int VIClient::SplitDigits(int v, int *thousands, int *hundreds, int *tens, int *ones)
{
    *thousands = v / 1000;
    v %= 1000;
    *hundreds = v / 100;
    v %= 100;
    *tens = v / 10;
    *ones = v % 10;
    return 0;
}

// 0x00BCE210
void VIClient::ShowNotice(const unsigned short *text, int sound)
{
    if (sound)
        PlaySound(sound);
    ShowDialog(0, text, 0, sound == 0);
}

// 0x00BCEFC0
int VIClient::GetQuestTitle(int i, unsigned short *buf)
{
    if (i >= m_questCount)
        return -1;
    VIString::strcpy16(buf, 256, m_questTitle[i]);
    return 0;
}

// 0x00BD8990
void VIClient::BackToLogin()
{
    m_wnd2BB88->m_visible = 0;
    m_wnd2BB88->ShowList();
    m_wndLogin->m_visible = 1;
    m_p2BD44 = m_wndLogin;
    SetActiveWindow(m_wndLogin);
}

// 0x00BDD488
int VIClient::IsTimedOut()
{
    if (m_drdp && m_conn)
    {
        unsigned int now = drdp_get_time(m_drdp);
        if (now - connection_get_time_last_arrival(m_conn) < 8000)
            return 0;
    }
    return 1;
}

// 0x00BDDFF0
unsigned short *VIClient::NewWide(const char *s)
{
    unsigned short *w = new unsigned short[(strlen(s) + 1) * 2];
    VIString::str8to16(w, (strlen(s) + 1) * 2, s);
    return w;
}

// 0x00BE5B20
void VIClient::SendOpByteIntSigned(int op, unsigned char b, int c, int d)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamByte(b) << VIMemStreamRaw(c) << d, GetConn(251));
}

// 0x00BCD888
void VIClient::TradeSend5(int x)
{
    if (m_p2BB00 == 0xFFFFFFFF)
        Trade16();
    else
        SendOpByteIntSigned(86, 5, m_tradePartner, x);
}

// 0x00BD0D00
void VIClient::ProcessMessageF8(long src, VIMemStream &s)
{
    unsigned short op;
    s.Read(&op, 2, 1);
    if (op == 0x993)
        Msg0993(s);
}

// 0x00BDC120
void VIClient::Msg0993(VIMemStream &s)
{
    int token;
    s.Read(&token, 4, 1);
    if (token == m_p4F0F8)
    {
        VIMemStream out;
        unsigned short o = 0x791;
        out.Write(&o, 2, 1);
        out.WriteU32(token);
        VINetSend(out, GetConn(251));
    }
}

// 0x00BDCB30: the stat channel, 233 bytes.  Both loops exit by `break`: a `for (i...; i < 29; i++)`
// is reversed into a countdown by the second loop pass, and the target counts up.
void VIClient::ReadStats(VIMemStream &s, VIPlayer &p)
{
    int i = 0;
    for (;;)
    {
        s.Read(&p.m_stat[i], 4, 1);
        if (++i >= 29)
            break;
    }
    i = 0;
    for (;;)
    {
        s.Read(&p.m_baseStat[i], 4, 1);
        if (++i >= 29)
            break;
    }
    s.Read(&p.m_p30, 1, 1);
}

// 0x00BD3CB0
void VIClient::Channel42(const void *data, int size)
{
    VIMemStream s(data, size);
    ReadStats(s, m_player);
}

// 0x00BD0D60
void VIClient::ProcessMessageF9(const void *data, int size)
{
    VIMemStream s(data, size);
    unsigned char b;
    s.Read(&b, 1, 1);
}

// 0x00BD3D18: the active effects (state channel 0x43), 132-byte records after the count
void VIClient::Channel43(const void *data, int size)
{
    VIMemStream s(data, size);
    s.Read(&m_effectCount, 4, 1);
    m_effectCount = m_effectCount < 9 ? m_effectCount : 8;
    for (int i = 0; i < m_effectCount; i++)
    {
        s.Read(&m_effectIcon[i], 4, 1);
        s.ReadString(m_effectName[i], 64);
        s.m_rpos = 4 + (i + 1) * 132;
    }
}

// 0x00BD4138
void VIClient::UpdateGroupMember(unsigned int id, unsigned char known, unsigned char hp, int x, const VIVect3 &pos)
{
    for (int i = 0; i < m_p2BE4C; i++)
    {
        VIPartyMember &m = m_group[i];
        if (m.m_id == id)
        {
            m.m_active = 1;
            m.m_hpKnown = known;
            m.m_hp = hp;
            m.m_p0C = x;
            m.m_pos = pos;
            return;
        }
    }
}

// 0x00BD3FA0
void VIClient::Channel40(const void *data, int size)
{
    for (int i = 0; i < m_p2BE4C; i++)
        m_group[i].m_active = 0;
    VIMemStream s(data, size);
    while (!s.m_error && s.m_size != s.m_rpos)
    {
        short xyz[3];
        VIVect3 pos;
        unsigned int id;
        unsigned char known, hp;
        signed char x;
        s.Read(&id, 4, 1);
        s.Read(&known, 1, 1);
        s.Read(&hp, 1, 1);
        s.Read(&x, 1, 1);
        s.Read(xyz, 2, 3);
        float *f = &pos.x;
        for (int k = 0; k < 3; k++)
            f[k] = xyz[k];
        UpdateGroupMember(id, known, hp, x, pos);
    }
}

// 0x00BD3E40
void VIClient::Channel44(int type, const void *data, int size)
{
    int k = type - 0x44;
    m_channels[k].m_size = size;
    if (size)
    {
        VIMemStream s(data, size);
        s.Read(m_channels[k].m_id);
        s.Read(m_channels[k].m_p08);
        s.Read(m_channels[k].m_p0C);
        s.Read(m_channels[k].m_p10);
        s.Read(m_channels[k].m_p14);
        s.Read(m_channels[k].m_p18);
        s.Read(m_channels[k].m_ping.m_head);
        s.Read(m_channels[k].m_ping.m_samples);
    }
}

// 0x00BD3C18: the DRDP state channels (message types 0x00..0x6B), by type
void VIClient::ProcessStateChannel(int type, const void *data, int size)
{
    if (type == 0x40)
        Channel40(data, size);
    else if (type == 0x42)
        Channel42(data, size);
    else if (type == 0x43)
        Channel43(data, size);
    else if (type >= 0x44 && type < 0x6C)
        Channel44(type, data, size);
    else if (type < 24)
        ChannelEntity(type, data, size);
}

// 0x00BD3AF8: DRDP message type 0xFC; 0x07B3 is the shard list
void VIClient::ProcessMessageFC(const void *data, int size)
{
    VIMemStream s(data, size);
    unsigned short op;
    s.Read(&op, 2, 1);
    if (op == 0x7B3)
    {
        if (m_p00058 == 0 || m_p00058 == 1)
        {
            m_shardInfo.Read(s);
            if (m_p00058 == 0)
            {
                m_p00058 = 1;
                m_p4F0E8 = 21;
                if (m_p50300 && m_shardInfo.GetCount() && m_shardInfo.IsValid(0))
                    ConnectShard(m_shardInfo.GetId(0));
            }
        }
    }
}

// 0x00BD0560
void VIClient::ResetWorld()
{
    for (int i = 0; i < 24; i++)
        if (m_entities[i])
        {
            delete m_entities[i];
            m_entities[i] = 0;
        }
    memset(m_channels, 0, sizeof(m_channels));
    m_p382E8.Reset();
    m_selfId = 0xFFFFFFFF;
    m_guild.Reset();
    m_buddies.Clear();
    m_p38300.Reset();
}

// 0x00BD4C10
void VIClient::PumpMessages()
{
    message_t *msg;
    while (m_drdp && (msg = drdp_receive(m_drdp)) != 0)
    {
        long src = message_get_src(msg);
        int type = message_get_type(msg);
        const void *data = message_get_data(msg);
        int size = message_get_size(msg);
        VIMemStream s(data, size);
        switch (type)
        {
        case 0xFF:
            connection_close(m_conn);
            ResetWorld();
            m_conn = message_get_connection(msg);
            m_p2EFD0.OnConnect();
            m_p33950.OnConnect();
            break;
        case 0xF8:
            ProcessMessageF8(src, s);
            break;
        case 0xFD:
        {
            int reason;
            m_p4F0EC = *(unsigned char *)message_get_data(msg);
            switch (*(unsigned char *)message_get_data(msg))
            {
            case 1:
                reason = 1;
                break;
            case 14:
                reason = 16;
                break;
            case 15:
                reason = 15;
                break;
            default:
                reason = 10;
                break;
            }
            printf("Disconnect-11\n");
            Disconnect(reason, 0);
            break;
        }
        case 0xFC:
            ProcessMessageFC(data, size);
            break;
        case 0xFB:
            ProcessMessage(src, data, size);
            break;
        case 0xF9:
            ProcessMessageF9(data, size);
            break;
        case 0xFE:
            break;
        default:
            ProcessStateChannel(type, data, size);
            break;
        }
        message_destroy(msg);
    }
}

// 0x00BCDD10
void VIClient::ClosePanel6D0()
{
    m_wndGame->ClosePanel6D0();
    m_p2BB10 = 0xFFFFFFFF;
}

// 0x00BCE800
void VIClient::OpenPanel34()
{
    m_battleMusic.Unload();
    m_wndGame->OpenPanel34();
}

// 0x00BCECD0
void VIClient::OpenPanel40()
{
    m_p4EC58.Open();
    m_wndGame->OpenPanel40();
}

// 0x00BCED18
void VIClient::ClosePanel6C0()
{
    m_p4EC58.Close();
    m_wndGame->ClosePanel6C0();
}

// 0x00BCEEA0
void VIClient::OpenPanel35()
{
    m_p38334.Open();
    m_wndGame->SetPanel35();
}

// 0x00BCEEE8
void VIClient::ClosePanel704()
{
    m_p38334.Close();
    m_wndGame->ClosePanel704();
}

// 0x00BD60C0
int VIClient::GetP44Total()
{
    int n = m_player.m_p44;
    return n + m_p4EC58.Count();
}

// 0x00BC2128
void VIClient::ReturnToSelect()
{
    m_p2BD44 = m_wnd2BB7C;
    SetActiveWindow(m_wnd2BB7C);
    if (m_p50300)
        ShowLoginWindow();
}

// 0x00BD9160
void VIClient::AddText(const unsigned short *text, VIColor32F color)
{
    m_wndGame->AddText(text, color);
}

// 0x00BD7A80
int VIClient::IsGroupFull()
{
    return InGroup() && m_p2BE4C == 4;
}

// 0x00BD5018
void VIClient::MoveToOrigin()
{
    VIVect3 v;
    m_scene.GetWorldGridCenter(0, 0, v);
    MoveTo(0, v);
}

// 0x00BD5068
void VIClient::MoveToGrid(int a, int col, int row)
{
    VIVect3 v;
    m_scene.GetWorldGridCenter(col + 1, row + 1, v);
    MoveTo(a, v);
}

// 0x00BC20A8
int VIClient::ConnectRandomShard()
{
    char host[256];
    unsigned short port;
    printf("Getting random shard list server\n");
    if (GetShardServer(host, &port) < 0)
        return -1;
    printf("Connecting to %s\n", host);
    Connect(host, port);
    return 0;
}

// 0x00BD93F0
int VIClient::HasPet() { return m_petId != 0xFFFFFFFF; }

// 0x00BD95C8
int VIClient::PetIsCreature()
{
    VIEntity *e;
    if (HasPet() && (e = FindEntity(m_petId)) != 0 && e->Virtual3(130))
        return 1;
    return 0;
}

// 0x00BD7728
int VIClient::GetPlayerName(char *buf, int size)
{
    VIEntity *e = FindEntity(m_selfId);
    if (e)
        VIString::snprintf(buf, size, "%s", ((VICreatureEntity *)e)->m_name);
    else
        VIString::snprintf(buf, size, "");
    return 0;
}

// 0x00BCC950
int VIClient::GetMerchantPrice(int i)
{
    int v = m_merchant.Item(i)->m_value * m_p1B30C;
    int r = v / 100;
    if (v % 100 >= 50)
        r++;
    return r;
}

// 0x00BCC9C0
float VIClient::GetInvItemCondition(int i)
{
    int hp = GetInvItemCurHp(i);
    int max = GetInvItemMaxHp(i);
    if (max == 0)
        return 1.0f;
    return (float)hp / (float)max;
}

// 0x00BCCC00
int VIClient::GetInvItemRepairCost(int i)
{
    VINetItem *it = m_inventory.Item(i);
    float c = GetInvItemCondition(i);
    return (int)((float)it->m_value * (1.0f - c));
}

// 0x00BD5978
void VIClient::ApplyCameraPreset(const VICameraPreset &p)
{
    VIWalkCamera &cam = m_walkCam;
    cam.SetCameraMode(p.m_mode);
    cam.m_targetHeight = p.m_targetHeight;
    cam.m_lookAtHeightAbove = p.m_lookAtHeightAbove;
    cam.SetOrbitPitchAngle(p.m_pitch);
    cam.SetOrbitDistance(p.m_distance);
    cam.SetOrbitAngle(p.m_angle);
}

// 0x00BC7B38
void VIClient::ApplyDefaultCamera()
{
    ApplyCameraPreset(m_camPreset);
    m_p06DC0 = 1;
    m_p06DC4 = 1;
}

// 0x00BD5940
void VIClient::SaveCameraPreset(VICameraPreset &p)
{
    VIWalkCamera &cam = m_walkCam;
    p.m_mode = (VIWalkCameraMode)cam.m_cameraMode;
    p.m_targetHeight = cam.m_targetHeight;
    p.m_lookAtHeightAbove = cam.m_lookAtHeightAbove;
    p.m_pitch = m_walkCam.m_last.m_orbit.y;
    p.m_distance = cam.m_last.m_dist;
    p.m_angle = m_walkCam.m_last.m_orbit.x;
}

// 0x00BD58C8
void VIClient::ToggleCameraStyle()
{
    if (VIGetGameOptions()->m_x06 == 1)
        SetCameraStyle(2);
    else
        SetCameraStyle(3);
    m_walkCam.SetOrbitAngle(m_walkCam.m_committed.m_angles.x + 3.14159265f);
    SaveCameraPreset(m_camSaved);
}

// 0x00BD4FA0
int VIClient::LoadInvIcons()
{
    for (int i = 0; i < GetInvCount(); i++)
        if (GetInvItemIcon(i))
            m_p06D50.Preload(GetInvItemIcon(i));
    return 0;
}

// 0x00BD7E00
void VIClient::LeaveGroup()
{
    SendOpcode(0x623);
    m_p2BD70 = -1;
    m_inGroup = m_p2BE4C = m_p2BD78 = 0;
    if (m_p382E4 == 2)
        GroupChanged(0);
}

// 0x00BD8368
void VIClient::DisbandGroup()
{
    SendOpcode(0x627);
    m_p2BD70 = -1;
    m_p2BD78 = m_p2BE4C = m_inGroup = 0;
    if (m_p382E4 == 2)
        GroupChanged(0);
}

// 0x00BC9EE0
int VIClient::FindInvByQuickSlot(int slot)
{
    int r = -1;
    for (int i = 0; i < GetInvCount(); i++)
        if (m_inventory.Item(i)->m_quickSlot == slot)
        {
            r = i;
            break;
        }
    return r;
}

// 0x00BD9410
int VIClient::PetFlag4()
{
    if (m_petId == 0xFFFFFFFF)
        return -1;
    VIEntity *e = FindEntity(m_petId);
    if (e && e->Virtual3(130))
        return ((VICreatureEntity *)e)->Flag4() != 0;
    return 0;
}

// 0x00BD9490
int VIClient::PetFlag8()
{
    if (m_petId == 0xFFFFFFFF)
        return -1;
    VIEntity *e = FindEntity(m_petId);
    if (e && e->Virtual3(130))
        return ((VICreatureEntity *)e)->Flag8() != 0;
    return 0;
}

// 0x00BC7B70
void VIClient::UpdateTimeOfDay()
{
    if (m_p00064)
    {
        m_timeOfDay = (float)(m_clockB.GetTime(m_p07568) % 3600000) / 3600000.0f * 24.0f;
        m_scene.SetTimeOfDay(m_timeOfDay);
    }
}

// 0x00BD4B90
void VIClient::ShowNetStats()
{
    if (m_conn)
    {
        unsigned short number;
        unsigned long history;
        int ping;
        connection_get_arrival_history(m_conn, &history);
        connection_get_arrival_number(m_conn, &number);
        connection_get_ping_time(m_conn, &ping);
        m_wndGame->ShowNetStats(m_p07568 - m_p2BB50, history, m_p2BB58, number, ping);
    }
}

// 0x00BD7920
int VIClient::GetTargetName(char *buf)
{
    VIEntity *e = FindEntity(m_target);
    if (e && e->Virtual3(130))
    {
        VIString::strcpy8(buf, 24, ((VICreatureEntity *)e)->m_name);
        return 0;
    }
    VIString::strcpy8(buf, 24, "");
    return -1;
}

// 0x00BD79B0
int VIClient::GetTargetPlayerName(char *buf)
{
    VIEntity *e = FindEntity(m_target);
    if (e && e->Virtual3(130) && ((VICreatureEntity *)e)->m_p256 == 2)
    {
        VIString::strcpy8(buf, 24, ((VICreatureEntity *)e)->m_name);
        return 0;
    }
    VIString::strcpy8(buf, 24, "");
    return -1;
}

// 0x00BDD408
void VIClient::ShowTargetPlayerName(int a, int b)
{
    char name[32];
    unsigned short wide[24];
    GetTargetPlayerName(name);
    VIString::str8to16(wide, sizeof(wide), name);
    m_wndGame->ShowName(a, b, 24, wide);
}

// 0x00BDD230
int VIClient::SetNameEntry(int i, const char *name, unsigned short v)
{
    if (!m_p4F0B0 || i < 0 || i >= m_p4F0B4)
        return -1;
    strcpy(m_p4F0B0[i].m_name, name);
    m_p4F0B0[i].m_p20 = v;
    return 0;
}

// 0x00BCC8C0
float VIClient::GetInvItemWear(int i)
{
    float c = GetInvItemCondition(i);
    if (c == 0.0f)
        return 0.5f;
    if (c == 1.0)
        return 0.0f;
    return (1.0f - c) * 0.5f;
}

// 0x00BDD0B0
int VIClient::FindEquipped(VIItemList *list, int slot, int *index)
{
    int n = list->ItemCount;
    for (int i = 0; i < n; i++)
        if (list->Item(i)->m_equipSlot == slot)
        {
            if (index)
                *index = i;
            return 1;
        }
    return 0;
}

// 0x00BC8660
void VIClient::JournalTick(int flags)
{
    if (flags & 2 && JournalConsole())
        return;
    if (flags & 1 && JournalNetwork())
        return;
    unsigned char tag = 255;
    m_journal.Write(tag);
    m_journal.Write(VITimer::GetTime());
}

// 0x00BCE848
void VIClient::ApplyOptions()
{
    VIGameOptions *o = VIGetGameOptions();
    m_wndGame->HidePanel6F0();
    m_battleMusic.Preload(o->m_language);
    if ((m_walkCam.m_cameraMode == 2 && o->m_x06 == 0) || (m_walkCam.m_cameraMode == 3 && o->m_x06 == 1))
        ToggleCameraStyle();
}

// 0x00BD9208
int VIClient::GroupMemberFlag4(int i)
{
    if (!m_inGroup)
        return -1;
    if (i >= m_p2BE4C)
        return -1;
    VIEntity *e = FindEntity(m_group[i].m_id);
    if (e && e->Virtual3(130))
        return ((VICreatureEntity *)e)->Flag4() != 0;
    return 0;
}

// 0x00BD92A8
int VIClient::GroupMemberFlag8(int i)
{
    if (!m_inGroup)
        return -1;
    if (i >= m_p2BE4C)
        return -1;
    VIEntity *e = FindEntity(m_group[i].m_id);
    if (e && e->Virtual3(130))
        return ((VICreatureEntity *)e)->Flag8() != 0;
    return 0;
}

// 0x00BD9350
int VIClient::LeaderByte263()
{
    if (!m_inGroup)
    {
        VIEntity *e = FindEntity(m_selfId);
        if (e && ((VICreatureEntity *)e)->Byte263())
            return 1;
        return 0;
    }
    if (m_p2BE4C > 0)
    {
        VIEntity *e = FindEntity(m_group[0].m_id);
        if (e && e->Virtual3(130) && ((VICreatureEntity *)e)->Byte263())
            return 1;
    }
    return 0;
}

// 0x00BC9968
int VIClient::GetObjectName(unsigned int type, int i, unsigned short *buf, unsigned int size)
{
    switch (type)
    {
    case 0x02030000:
        VIString::strcpy16(buf, size, m_inventory.Item(i)->m_name);
        break;
    case 0x08060000:
        VIString::strcpy16(buf, size, m_abilities.Value(i).m_name);
        break;
    }
    return 0;
}

// 0x00BCB9D8
void VIClient::GetAbilityList(VIArray<VIRec4_BBC218> &out)
{
    out.Init(m_abilities.Size());
    int n = 0;
    for (int i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
        out.m_data[n++].m_data[0] = i;
}

// 0x00BCC818
int VIClient::GetInvItemSellPrice(int i)
{
    float w = GetInvItemWear(i);
    int v = m_inventory.Item(i)->m_value * m_p1B308;
    int r = v / 100;
    if (v % 100 >= 50)
        r++;
    return r - (int)(r * w);
}

// 0x00BD7B30
void VIClient::DropInvites(int id)
{
    int i = m_p2BD58.Begin();
    while (i != m_p2BD58.End())
        if (m_p2BD58.Value(i).m_id == id)
            i = m_p2BD58.Erase(i);
        else
            i = m_p2BD58.Next(i);
}

// 0x00BCBE40: no caller in the image.  It walks the map by NODE index, 0..Size()-1.
int VIClient::IsAbilitySlotSpell(int slot)
{
    for (int i = 0; i < m_abilities.Size(); i++)
    {
        VIAbility &a = m_abilities.Value(i);
        if (a.m_slot == slot)
        {
            int r = 0;
            if (a.m_displayIndex < 0xFC)
                r = 1;
            return r;
        }
    }
    return 1;
}

// 0x00BD9520
int VIClient::GetPetHealth(float *hp)
{
    if (m_petId != 0xFFFFFFFF)
    {
        VIEntity *e = FindEntity(m_petId);
        if (e && e->Virtual3(130))
        {
            *hp = e->m_p19 / 255.0f;
            return 0;
        }
    }
    *hp = 0;
    return -1;
}

// 0x00BD9658
void VIClient::SendD4Off()
{
    VIMemStream out;
    out.WriteU16(0xD4);
    out.WriteByte(0);
    SendRaw(out.m_rbuf, out.m_size);
}

// 0x00BDC850
int VIClient::CheckPower()
{
    if (m_player.m_stat[9] >= (int)(m_player.m_stat[10] / 10.0f))
        return 1;
    VIUI &ui = m_ui;
    m_wndGame->AddText((const unsigned short *)ui.m_words.GetString(731, ui.m_language), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    return 0;
}

// 0x00BDD140
int VIClient::AllocNames(int n)
{
    FreeBuffer();
    m_p4F0B0 = new VINameEntry[n];
    if (!m_p4F0B0)
        return -1;
    m_p4F0B4 = n;
    return 0;
}

// 0x00BCEAF0
void VIClient::Send006B(int x)
{
    char buf[32];
    VIMemStream out(buf, sizeof(buf));
    unsigned short o = 0x6B;
    out.Write(&o, 2, 1);
    out.WriteSigned(x);
    SendRaw(buf, out.m_size);
}

// 0x00BCFDC8
void VIClient::PlayAction(int action)
{
    if (m_selfId != 0xFFFFFFFF)
        SendOpcodeByte(71, action);
    VIEntity *e = FindEntity(m_selfId);
    if (e && e->m_actor != -1)
    {
        VIScene &scene = m_scene;
        if (scene.GetActorSpriteType(e->m_actor) == 5)
            ((VICSprite *)scene.Sprite(scene.GetActorSprite(e->m_actor)))->SetAction((VICSpriteAnimID)action, scene);
    }
}

// 0x00BD5E38
void VIClient::InitCameraStyle()
{
    VIGameOptions *o = VIGetGameOptions();
    // Cases 2 and 3 are two identical bodies, not stacked labels: stacked, they make one
    // range node [2,3] in the compare tree, and the target tests 2 and 3 apart (jump2
    // cross-jumps the copies afterwards).
    switch (m_walkCam.m_cameraMode)
    {
    case 2:
        SetCameraStyle(0);
        break;
    case 3:
        SetCameraStyle(0);
        break;
    case 0:
        if (o->m_x06 == 1)
            SetCameraStyle(2);
        else
            SetCameraStyle(3);
        break;
    default:
        SetCameraStyle(0);
        break;
    }
    SaveCameraPreset(m_camSaved);
}

// 0x00BD9700
void VIClient::SendD4On()
{
    VIMemStream out;
    out.WriteU16(0xD4);
    out.WriteByte(1);
    SendRaw(out.m_rbuf, out.m_size);
}

// 0x00BD97B0
void VIClient::PromptWithName(const unsigned short *name)
{
    unsigned short text[128];
    unsigned short space[2];
    VIUI &ui = m_ui;
    VIString::strcpy16(text, 256, (const unsigned short *)ui.m_words.GetString(1189, ui.m_language));
    VIString::str8to16(space, 4, " ");
    VIString::strcat16(text, 256, space);
    VIString::strcat16(text, 256, name);
    m_wndGame->Prompt(text, 1, 3, 0);
}

// 0x00BC7A80
void VIClient::ApplyRaceCamera()
{
    VIWalkCamera &cam = m_walkCam;
    VIRaceCamera *rc = GetRaceCamera(m_player.m_race, ExpansionFlags());
    if (rc)
    {
        SaveCameraPreset(m_camPreset);
        cam.m_targetHeight = rc->m_targetHeight;
        cam.m_lookAtHeightAbove = rc->m_lookAtHeightAbove;
        cam.SetOrbitPitchAngle(rc->m_pitch);
        cam.SetOrbitDistance(rc->m_distance);
        cam.SetCameraMode((VIWalkCameraMode)1);
        cam.ChooseOrbitAngle(m_walkCam.m_committed.m_angles.x, m_scene);
        cam.Halt();
        m_p06DC0 = 0;
        m_p06DC4 = 0;
    }
}

// 0x00BD9010
void VIClient::UpdateMusic()
{
    if (m_musicFade <= 0.0f && strlen(m_musicName))
    {
        m_sound.PlayBgm(0, m_musicName);
        m_pC18C = 1;
        m_sound.SetBgmVolume(0, 1.0f);
    }
    else if (!m_musicName[0])
        m_sound.StopBgm(0);
}

// 0x00BCBC60
void VIClient::GetAbilityRecords(int i, VIArray<VIRec4_BBC218> &out)
{
    VIArray<VIRec144_BE08C8> &recs = m_abilities.Value(i).m_p1BC;
    out.Init(recs.m_size);
    int n = 0;
    for (int k = recs.Begin(); k != recs.End(); k = recs.Next(k))
        out.m_data[n++].m_data[0] = k;
}

// 0x00BCF9C0
void VIClient::SetKey(int key, int down)
{
    if (m_pC1B0)
        return;
    if (VITimer::GetTime() < m_p50320)
        return;
    VIWnd *w = m_p2BD40;
    if (!w)
        return;
    if (down)
        w->OnKeyDown(key);
    else
        w->OnKeyUp(key);
    m_keys[key] = down;
}

// 0x00BD5D70
void VIClient::ToggleFollow()
{
    int found = 0;
    m_p00078 ^= 1;
    if (m_p00078)
    {
        if (m_target != m_selfId)
            for (int i = 0; i < GetP2BE4C(); i++)
                if (GetGroupMemberId(i) == m_target)
                {
                    m_p0007C = m_target;
                    found = 1;
                    break;
                }
        if (!found)
        {
            m_wndGame->LootOption(1195);
            m_p00078 = 0;
        }
    }
}

// 0x00BCC118
void VIClient::Send00FE()
{
    VIMemStream out;
    unsigned short o = 0xFE;
    out.Write(&o, 2, 1);
    out.WriteU32(m_p2BB60);
    VINetSend(out, GetConn(251));
}

// 0x00BCF020
void VIClient::Send007D(int x)
{
    char buf[32];
    VIMemStream out(buf, sizeof(buf));
    out.WriteU16(0x7D);
    out.WriteU32(m_p2BB0C);
    out.WriteSigned(x);
    SendRaw(buf, out.m_size);
}

// 0x00BC9F88
int VIClient::GetStatTotal(int stat)
{
    int total = m_player.m_baseStat[stat];
    for (int i = 0; i < GetInvCount(); i++)
    {
        VINetItem *it = m_inventory.Item(i);
        for (int k = 0; k < it->m_stats.m_size; k++)
            if (it->m_stats.m_data[k].m_id == stat && it->m_equipSlot != -1)
                total += it->m_stats.m_data[k].m_value;
    }
    return total;
}

// 0x00BC9E10
int VIClient::IsRangedItem(int i, int list)
{
    VINetItem *it;
    switch (list)
    {
    case 0:
        it = m_inventory.Item(i);
        break;
    case 1:
        it = m_merchant.Item(i);
        break;
    case 2:
        it = m_bank.Item(i);
        break;
    case 3:
        it = m_corpse.Item(i);
        break;
    case 4:
        it = m_tradeInv.Item(i);
        break;
    case 5:
        it = &TradeItem(1, i)->m_item;
        break;
    case 6:
        it = &TradeItem(0, i)->m_item;
        break;
    default:
        return 0;
    }
    int t = it->m_attackType;
    if (t < 11)
    {
        if (t >= 7)
            return 1;
    }
    return 0;
}

// 0x00BE5390
void VIClient::SendOpSignedSigned(int op, int a, int b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << a << b, GetConn(251));
}

// 0x00BE5578
void VIClient::SendOpSignedByte(int op, int a, unsigned char b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << a << VIMemStreamByte(b), GetConn(251));
}

// 0x00BE5660
void VIClient::SendOpSignedRaw(int op, int a, int b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << a << VIMemStreamRaw(b), GetConn(251));
}

// 0x00BE5D18
void VIClient::SendOpByteSigned(int op, unsigned char a, int b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamByte(a) << b, GetConn(251));
}

// 0x00BE5810
void VIClient::SendOpByteByte(int op, unsigned char a, unsigned char b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamByte(a) << VIMemStreamByte(b), GetConn(251));
}

// 0x00BE5F10
void VIClient::SendOpByteRaw(int op, unsigned char a, int b)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamByte(a) << VIMemStreamRaw(b), GetConn(251));
}

// 0x00BD50E8
void VIClient::LoadWorld(int world)
{
    const char *path;
    if (!m_p0662C)
        path = gWorldInfo[world].m_path;
    else
        path = m_worldPath;
    m_worldId = world;
    VIScene &scene = m_scene;
    scene.m_p4BAC = world;
    VIESFParse parser;
    parser.ParseWorld(path, IsNotP0662C(), scene);
    if (IsNotP0662C())
        m_loader.Open(5, path);
    for (int i = 0; i < 24; i++)
        if (m_entities[i])
            m_entities[i]->OnWorldLoaded();
}

// 0x00BDCDC0
int VIClient::ReadAbilities(VIMemStream &s, VIMap<int, VIAbility> &map)
{
    VIAbility a;
    int n;
    s.ReadSigned(n);
    int r = map.Init(n);
    if (r)
        goto fail;
    for (int i = 0; i < n; i++)
    {
        r = a.Read(s);
        if (r)
            goto fail;
        map.Insert(a.m_p00, a);
    }
    r = 0;
    goto done;
fail:
    map.Init(0);
done:
    return r;
}

// 0x00BCEBA0
void VIClient::Send006C(int a, const VIRec388 *r)
{
    char buf[512];
    VIMemStream out(buf, sizeof(buf));
    unsigned short o = 0x6C;
    out.Write(&o, 2, 1);
    out.WriteSigned(a);
    out.WriteSigned(r->m_p00);
    out.WriteString(r->m_p04);
    out.WriteString(r->m_p104);
    SendRaw(buf, out.m_size);
}

// 0x00BCDBF8
void VIClient::AnswerBuddyRequest()
{
    {
        VIMemStream out;
        unsigned short o = 0x367;
        out.Write(&o, 2, 1);
        out.WriteU32(m_p2BB10);
        VINetSend(out, GetConn(251));
    }
    m_wndGame->HideBuddyRequest();
}

// 0x00BCDAE8
void VIClient::EmptyBCDAE8() {}

// 0x00BCFA80
void VIClient::Pointer(int x, int y) {}

// 0x00BD4B88
void VIClient::TickHookB() {}

// 0x00BD9348
int VIClient::ReturnZero() { return 0; }

// 0x00BCCB78
void VIClient::SetPlayerP38(int v) { m_player.m_p38 = v; }

// 0x00BCD9F8
void VIClient::DeclineBuddyRequest() { ClosePanel6D0(); }

// 0x00BD8FF0
int VIClient::ClearMusic()
{
    m_pC18C = -1;
    m_musicName[0] = 0;
    return 0;
}

// 0x00BD9638
void VIClient::Send0041() { SendOpcodeByte(65, 5); }

// 0x00BDD520
int VIClient::GetFrameRate()
{
    if (m_p50304 & 2)
        return 60;
    return 50;
}

// 0x00BCEF80
void VIClient::ClosePanel718() { m_wndGame->ClosePanel718(); }

// 0x00BD6228
void VIClient::ClosePanel6BC() { m_wndGame->ClosePanel6BC(); }

// 0x00BD9860
void VIClient::Send0701a() { m_p382E8.Send0701a(); }

// 0x00BD9888
void VIClient::Send0701b() { m_p382E8.Send0701b(); }

// 0x00BC8B68
void VIClient::TargetNearest(int group)
{
    m_target = -1;
    CycleTarget(group);
}

// 0x00BCCB28
int VIClient::ClosePanel6AC()
{
    m_wndGame->ClosePanel6AC();
    return 0;
}

// 0x00BCE990
void VIClient::ShowA110(int x) { m_p382E8.Show(0xA1100000, x); }

// 0x00BD8A60
void VIClient::Send0012(int a, int b)
{
    if (m_p00058 == 3)
        SendOpSignedSigned(18, a, b);
}

// 0x00BDD3D8
void VIClient::ShowNameClear(int a, int b) { m_wndGame->ShowName(a, b, 256, 0); }

// 0x00BCF988: no caller in the image; non-static, or it would not have been emitted.
int strlen16(const unsigned short *s)
{
    int n = 0;
    while (*s)
    {
        s++;
        n++;
    }
    return n;
}

// 0x00BD5C70
void VIClient::ToggleP74()
{
    unsigned short text[128];
    m_p00074 = 1 - m_p00074;
    if (m_p00074)
    {
        VIUI &ui = m_ui;
        VIString::snprintf16(text, 128, "%S: %S", ui.m_words.GetString(720, ui.m_language), ui.m_words.GetString(551, ui.m_language));
    }
    else
    {
        VIUI &ui = m_ui;
        VIString::snprintf16(text, 128, "%S: %S", ui.m_words.GetString(720, ui.m_language), ui.m_words.GetString(552, ui.m_language));
    }
    m_wndGame->AddText(text);
}

// 0x00BD7D00
void VIClient::DeclineInvite()
{
    if (m_p2BD58.Begin() == m_p2BD58.End())
    {
        ShowMsg(624);
        return;
    }
    unsigned short text[128];
    int id = m_p2BD58.Value(m_p2BD58.Begin()).m_id;
    VIUI &ui = m_ui;
    VIString::snprintf16(text, 256, (const unsigned short *)ui.m_words.GetString(623, ui.m_language), m_p2BD58.Value(m_p2BD58.Begin()).m_name);
    ShowText(text);
    DropInvites(id);
    SendMsg(1572, id, 1);
}

// 0x00BC7120
void VIClient::RefreshCollision()
{
    for (int i = 0; i < 24; i++)
    {
        VIEntity *e = m_entities[i];
        if (!e)
            continue;
        e->Virtual9();
        if (!e->Virtual3(130))
            continue;
        if (e->m_id == m_selfId)
            continue;
        VICreatureEntity *c = (VICreatureEntity *)e;
        if (!c->m_p2B8)
            continue;
        if (e->m_actor == -1)
            continue;
        m_scene.SetActorCollideable(e->m_actor, m_p38300.Find(c->m_name) == -1);
        c->m_p2B8 = 0;
    }
}

// 0x00BCDAF0
void VIClient::RejectBuddyRequest()
{
    {
        VIMemStream out;
        unsigned short o = 0x368;
        out.Write(&o, 2, 1);
        out.WriteU32(m_p2BB10);
        out.WriteByte(2);
        VINetSend(out, GetConn(251));
    }
    m_wndGame->HideBuddyRequest();
}

// 0x00BCC010
void VIClient::Send0033(int a, int b)
{
    char buf[32];
    VIMemStream out(buf, sizeof(buf));
    out.WriteU16(0x33);
    out.WriteU32(m_p2BB60);
    out.WriteByte(a);
    out.WriteByte(b);
    SendRaw(buf, out.m_size);
}

// 0x00BE63A8
void VIClient::SendOpU32(int op, unsigned int v)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteU32(v);
    VINetSend(out, GetConn(251));
}

// 0x00BD5B68
void VIClient::InteractWithTarget()
{
    VIEntity *e = FindEntity(m_target);
    if (!e || !e->Virtual3(130))
        return;
    VICreatureEntity *c = (VICreatureEntity *)e;
    unsigned short f = c->m_npcFlags;
    if (f & 1)
        SendOpU32(76, m_target);
    else if (f & 2)
        SendOpU32(4685, m_target);
    else if (f & 8)
        SendOpU32(97, m_target);
    else if (c->m_p256 == 2 || c->m_p256 == 3)
        m_wndGame->m_p678 = 1;
    else
        SendOpU32(4, m_target);
}

// 0x00BD6100
void VIClient::TrainStats(int *points)
{
    int i;
    int total = 0;
    for (i = 0; i < 7; i++)
        total += points[i];
    if (!total || m_player.m_p40 < total)
        return;
    VIMemStream out;
    unsigned short o = 0x1D;
    out.Write(&o, 2, 1);
    for (i = 0; i < 7; i++)
        out.WriteSigned(points[i]);
    SendRaw(out.m_rbuf, out.m_size);
}

// 0x00BD7BE8
void VIClient::AcceptInvite()
{
    if (m_p2BD58.Begin() == m_p2BD58.End())
    {
        ShowMsg(624);
        return;
    }
    unsigned short text[128];
    int id = m_p2BD58.Value(m_p2BD58.Begin()).m_id;
    VIUI &ui = m_ui;
    VIString::snprintf16(text, 256, (const unsigned short *)ui.m_words.GetString(622, ui.m_language), m_p2BD58.Value(m_p2BD58.Begin()).m_name);
    ShowText(text);
    DropInvites(id);
    SendOpInt(1570, id);
    m_inGroup = 1;
    m_p2BD78 = 0;
    Reset();
}

// 0x00BCAC90
void VIClient::UseItemOn(int i, int x)
{
    if (m_p13B90)
        return;
    if (i >= m_inventory.Count())
        return;
    m_p13B90 = 1;
    int slot = m_inventory.GetSlotId(i);
    VIMemStream out;
    unsigned short o = 0x3C;
    out.Write(&o, 2, 1);
    out.WriteU32(slot);
    out.WriteU32(x);
    VINetSend(out, GetConn(251));
}

// 0x00BCBEE8
void VIClient::Send0032(int i)
{
    char buf[32];
    VIMemStream out(buf, sizeof(buf));
    out.WriteU16(0x32);
    out.WriteU32(m_p2BB60);
    out.WriteByte(m_abilities.Value(i).m_slot);
    out.WriteU32(m_target);
    SendRaw(buf, out.m_size);
}

// 0x00BD5F60
void VIClient::PrintPosition()
{
    char text[128];
    VIVect3 hpr;
    VIVect3 pos;
    pos = m_walkCam.m_committed.m_position;
    hpr = m_walkCam.m_committed.m_angles;
    VIString::snprintf(text, 128, "%.2f,%.2f,%.2f %.2f,%.2f,%.2f", pos.x, pos.y, pos.z, hpr.x, hpr.y, hpr.z);
    m_wndGame->AddText(text);
}

// 0x00BCE9C0
void VIClient::Send006A(const VIRec6A *r)
{
    char buf[2048];
    VIMemStream out(buf, sizeof(buf));
    unsigned short o = 0x6A;
    out.Write(&o, 2, 1);
    out.WriteSigned(r->m_p00);
    for (int i = 0; i < 4; i++)
    {
        out.WriteSigned(r->m_p04[i].m_p00);
        out.WriteString(r->m_p04[i].m_p04);
        out.WriteString(r->m_p04[i].m_p104);
    }
    SendRaw(buf, out.m_size);
}

// 0x00BCD750
void VIClient::TradeSend4(int i)
{
    if (m_p2BB00 == 0xFFFFFFFF)
    {
        Trade16();
        return;
    }
    int id = m_tradeB[i].m_id;
    char buf[256];
    VIMemStream out(buf, sizeof(buf));
    out.WriteU16(0x56);
    out.WriteByte(4);
    out.WriteU32(m_tradePartner);
    out.WriteU32(id);
    SendRaw(buf, out.m_size);
}

// 0x00BC9730
void VIClient::SelectTarget(int which)
{
    int n = GetP2BE4C();
    switch (which)
    {
    case 1:
        m_target = m_selfId;
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    {
        if (which > n)
        {
            m_target = 0xFFFFFFFF;
            break;
        }
        m_target = m_group[which - 2].m_id == m_selfId ? m_group[which - 1].m_id : m_group[which - 2].m_id;
        break;
    }
    case 6:
        m_target = m_petId;
        break;
    default:
        m_target = 0xFFFFFFFF;
        break;
    }
    m_autoAttack = 0;
    m_targetInfoValid = 0;
    SendOpIntInt(16, m_p2BB68, m_target);
}

// 0x00BC95F0
static float s_reticleAlpha = 0.5f;

void VIClient::DrawReticle(int flash)
{
    VIVect2I center(320, 224);
    VIResourceType type;
    if (m_reticleTex == -1)
    {
        if (!m_dict.Find(VIHashResourceID("P:\\studio\\eqps2\\Game_Assets\\UI\\AT_Reticle.tga"), type, m_reticleTex))
            return;
    }
    if (flash)
        s_reticleAlpha = 1.0f;
    m_raster.Begin2D();
    m_raster.Blit(m_reticleTex, 4, center + VIVect2I(-16, -16), (VIAlphaBlendMode)3, s_reticleAlpha);
    m_raster.End2D();
    s_reticleAlpha = s_reticleAlpha - 0.05f < 0.5f ? 0.5f : s_reticleAlpha - 0.05f;
}

// 0x00BD4E60
int VIClient::OpenResources()
{
    VILoader &ld = m_loader;
    if (ld.Init(0x110000, IsNotP0662C() ? 6 : 5) < 0)
        return -1;
    ld.SetProgressCallback(LoadingProgress, this);
    if (ld.OpenResourceFile(0, "data\\char.esf", m_scene) < 0)
        return -1;
    if (ld.OpenResourceFile(1, "data\\ambtrack.esf", m_scene) < 0)
        return -1;
    if (ld.OpenResourceFile(2, "data\\item.esf", m_scene) < 0)
        return -1;
    if (ld.OpenResourceFile(3, "data\\itemicon.esf", m_scene) < 0)
        return -1;
    if (ld.OpenResourceFile(4, "data2\\charface.esf", m_scene) < 0)
        return -1;
    if (m_p06D00.Init(m_scene, ld) < 0)
        return -1;
    if (m_p06D50.Init(48, m_scene, ld) < 0)
        return -1;
    ld.SetProgressCallback(0, 0);
    return 0;
}

// 0x00BD8630
void VIClient::DeleteCharacter(int i)
{
    SendOpSigned(45, m_chars[i].m_serverId);
    m_charCount--;
    for (int k = i; k < m_charCount; k++)
        m_chars[k] = m_chars[k + 1];
}

// 0x00BDA028
void VIClient::RequestTrade()
{
    VIEntity *e = FindEntity(m_target);
    if (!e || !e->Virtual3(130))
        return;
    if (((VICreatureEntity *)e)->m_p256 != 2)
        return;
    if (m_target == m_selfId)
        return;
    if (m_p2BB00 != 0xFFFFFFFF)
        TradeCancel();
    unsigned short text[128];
    VIUI &ui = m_ui;
    VIString::snprintf16(text, 256, (const unsigned short *)ui.m_words.GetString(837, ui.m_language), ((VICreatureEntity *)e)->m_name);
    m_wndGame->AddText(text, VIColor32F(0.5f, 0.2f, 0.78f, 1.0f));
    m_p2BB04 = 1;
    m_p2BB00 = m_target;
    SendOpU32(87, m_target);
}

// 0x00BD59E8
void VIClient::ToggleAutoAttack()
{
    VIEntity *e = FindEntity(m_target);
    VIGameOptions *o = VIGetGameOptions();
    if (!e)
        return;
    if (e->m_p18 & 2)
    {
        if (m_wndGame->m_p05C)
            SendOp0015(m_target);
        return;
    }
    // `c` is not initialised when Virtual3 says no: the target's `movn` keeps whatever
    // the register held.
    VICreatureEntity *c;
    if (e->Virtual3(130))
        c = (VICreatureEntity *)e;
    if (!c)
        return;
    if (c->m_npcFlags & 0x80)
        return;
    if (!o->m_x95 || !m_autoAttack)
    {
        if (m_p071E8 <= m_p07568 || !m_p071E8)
        {
            m_p071E8 = m_p07568;
            if (m_target != 0xFFFFFFFF)
                SendOp000F(m_target);
        }
    }
    if (!o->m_x95)
        m_autoAttack = 1;
    else
        m_autoAttack = 1 - m_autoAttack;
}

// 0x00BCD600
void VIClient::TradeSend3(int i, int n)
{
    if (m_p2BB00 == 0xFFFFFFFF)
    {
        Trade16();
        return;
    }
    int idx = m_tradeInv.Item(i)->m_invIndex;
    char buf[256];
    VIMemStream out(buf, sizeof(buf));
    out.WriteU16(0x56);
    out.WriteByte(3);
    out.WriteU32(m_tradePartner);
    out.WriteU32(idx);
    out.WriteSigned(n);
    SendRaw(buf, out.m_size);
}

// 0x00BCC1F0
void VIClient::Send003A(int slot, int index)
{
    int i;
    for (i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
        if (m_abilities.Value(i).m_displayIndex == index)
            break;
    if (i == m_abilities.End())
        return;
    char buf[32];
    VIMemStream out(buf, sizeof(buf));
    out.WriteU16(0x3A);
    out.WriteU32(m_p2BB60);
    out.WriteByte(slot);
    out.WriteByte(index);
    SendRaw(buf, out.m_size);
}

// 0x00BCFC38
void VIClient::PlayEmote(int which)
{
    if (m_selfId == 0xFFFFFFFF)
        return;
    switch (which)
    {
    case 0:
        SendOpcode(30);
        break;
    case 1:
        SendOpcode(66);
        break;
    case 2:
        SendOpcode(67);
        break;
    case 3:
        SendOpcode(68);
        break;
    }
    VIEntity *e = FindEntity(m_selfId);
    if (!e)
        return;
    VIScene &scene = m_scene;
    if (scene.GetActorSpriteType(e->m_actor) != 5)
        return;
    VICSprite *sp = (VICSprite *)scene.Sprite(scene.GetActorSprite(e->m_actor));
    switch (which)
    {
    case 0:
        sp->SetAction((VICSpriteAnimID)56, scene);
        break;
    case 1:
        sp->SetAction((VICSpriteAnimID)57, scene);
        break;
    case 2:
        sp->SetAction((VICSpriteAnimID)60, scene);
        break;
    case 3:
        sp->SetAction((VICSpriteAnimID)61, scene);
        break;
    }
}

// 0x00BCA970
int VIClient::CanUseListItem(int i, int list)
{
    VINetItem *it;
    switch (list)
    {
    case 0:
        if (i < 0)
            goto none;
        it = m_inventory.SafeItem(i);
        break;
    case 1:
        if (i < 0)
            goto none;
        it = m_merchant.SafeItem(i);
        break;
    case 2:
        if (i < 0 || i >= m_bank.Count())
            goto none;
        it = m_bank.Item(i);
        break;
    case 3:
        if (i < 0)
            goto none;
        it = m_corpse.SafeItem(i);
        break;
    case 4:
        if (i < 0)
            goto none;
        it = m_tradeInv.SafeItem(i);
        break;
    case 5:
        if (i < 0 || i >= m_tradeBCount)
            goto none;
        it = &TradeItem(1, i)->m_item;
        break;
    case 6:
        if (i < 0 || i >= m_tradeACount)
            goto none;
        it = &TradeItem(0, i)->m_item;
        break;
    default:
        return 0;
    none:
        it = 0;
        break;
    }
    if (!it)
        return 0;
    return m_player.CanUse(it);
}

// 0x00BCFA88
void VIClient::PadKeys(const VIClientPart07190 *pad)
{
    if (m_pC1B0)
        return;
    for (int i = 0; i < 20; i++)
        if ((m_p07190.m_keys[i] != 0.0f) != (pad->m_keys[i] != 0.0f))
            SetKey(i + 128, pad->m_keys[i] != 0.0f);
    m_p07190 = *pad;
}

// 0x00BE02F0: moves item a to slot b, sliding the ones between along by one.
void VIItemList::Op003D(int a, int b)
{
    VINetItem item = ItemInternal[a];
    int step = a < b ? 1 : -1;
    for (int i = a; i != b; i += step)
        ItemInternal[i] = ItemInternal[i + step];
    ItemInternal[b] = item;
}

// 0x00BDFFA8
int VIItemList::Find(int id)
{
    for (int i = 0; i < ItemCount; i++)
        if (ItemInternal[i].m_invIndex == id)
            return i;
    return -1;
}

// 0x00BE05D0
void VIItemList::Op0060(int slot, int hp)
{
    ItemInternal[slot].m_curHp = hp;
    if (hp < 0)
        ItemInternal[slot].m_curHp = 0;
}

// 0x00BBF5F8, name provisional: motor 0 is the high-frequency one, 1 the low with a level.
void Rumble(unsigned char motor, unsigned char level, int ms, VIPad *pad)
{
    if (!(VIGetFeatureFlags() & 0x200))
        return;
    if (!VIGetGameOptions()->m_x97)
        return;
    switch (motor)
    {
    case 0:
        pad->VibrateHf(ms);
        break;
    case 1:
        pad->VibrateLf(level, ms);
        break;
    }
}

// 0x00BC2180
void VIClient::ShowLoginWindow()
{
    m_p4F130 = 0;
    m_shardInfo.Clear();
    m_p2BD44 = m_wnd2BB80;
    SetActiveWindow(m_wnd2BB80);
}

// 0x00BCE8E0: ApplyOptions' body, run only when the options panel is up.
void VIClient::ResetBattleMusic()
{
    VIGameOptions *o = VIGetGameOptions();
    if (!m_wndGame->IsPanel6F0Open())
        return;
    m_wndGame->HidePanel6F0();
    m_battleMusic.Preload(o->m_language);
    if ((m_walkCam.m_cameraMode == 2 && o->m_x06 == 0) || (m_walkCam.m_cameraMode == 3 && o->m_x06 == 1))
        ToggleCameraStyle();
}

// 0x00BD8A90, name provisional: a character-select entry's look, from the customise window.
void VIClient::SetCharLook(int i, int face, int hairStyle, int hairLength, int hairColor)
{
    if (!(ExpansionFlags() & 2))
        return;
    if (i < m_charCount && m_charCount < 9)
    {
        m_chars[i].m_face = face;
        m_chars[i].m_hairStyle = hairStyle;
        m_chars[i].m_hairLength = hairLength;
        m_chars[i].m_hairColor = hairColor;
    }
}

// 0x00BDB8F8, name provisional: each id's four pairs out of the 0x000D dump, into
// VITextSet's table; an empty string, or a lone space, is left unset.
void VIClient::LoadTextSets(VIArray<VIRec6A> &sets)
{
    VITextSet::Reset();
    for (int i = 0; i < sets.m_size; i++)
    {
        VITextSet *t = VITextSet::Get(sets[i].m_p00);
        for (int j = 0; j < sets[i].m_p04.m_size; j++)
        {
            VIRec388 &r = sets[i].m_p04[j];
            if ((unsigned int)r.m_p00 >= 4)
                continue;
            if (r.m_p104[0] && (r.m_p104[0] != ' ' || r.m_p104[1]))
            {
                int n = VIString::strlen16(r.m_p104) + 1;
                t->m_pairs[r.m_p00].m_p00 = new unsigned short[n];
                VIString::strcpy16(t->m_pairs[r.m_p00].m_p00, n * 2, r.m_p104);
            }
            if (r.m_p04[0] && (r.m_p04[0] != ' ' || r.m_p04[1]))
            {
                int n = VIString::strlen16(r.m_p04) + 1;
                t->m_pairs[r.m_p00].m_p04 = new unsigned short[n];
                VIString::strcpy16(t->m_pairs[r.m_p00].m_p04, n * 2, r.m_p04);
            }
        }
    }
}

// 0x00BD51C0, name provisional: MoveTo, Respawn and the 0x000D dump all land here.  A
// changed disc is fatal: -1 hangs, 1 says so first.
void VIClient::EnterWorld(int world, const VIVect3 &pos)
{
    if (!IsNotP0662C())
    {
        m_worldId = world;
        return;
    }
    if (m_worldId != world)
    {
        m_scene.StreamClearCache(m_loader, 5);
        m_loader.Close(5);
        if (VIGetFeatureFlags() & 0x100)
            VICdStreamCancel();
        LoadWorld(world);
    }
    VIMatrix44 m;
    m.Identity();
    m_raster.SetDefaultLights(m);
    m_loader.SetProgressCallback(LoadingProgress, this);
    m_scene.StreamFillCache(pos, m_loader, 5);
    m_loader.SetProgressCallback(0, 0);
    m_scene.ResetRadialFlora();
    switch (VIHasDiscIdChanged())
    {
    case -1:
        for (;;)
            ;
    case 1:
        VIDisplayScreen16("ERROR", 4, true);
        for (;;)
            ;
    }
    m_p50320 = VITimer::GetTime() + 4000;
}

// 0x00BD7430, name provisional: the pet menu's commands, sent as 0x0041 and one byte.
void VIClient::PetCommand(int cmd)
{
    if (m_selfId == 0xFFFFFFFF)
        return;
    if (HasPet())
    {
        switch (cmd)
        {
        case 0x05030000:
            SendOpcodeByte(65, 0);
            break;
        case 0x050A0000:
            SendOpcodeByte(65, 1);
            break;
        case 0x050B0000:
            SendOpcodeByte(65, 2);
            break;
        case 0x050C0000:
            SendOpcodeByte(65, 3);
            break;
        case 0x050D0000:
            SendOpcodeByte(65, 4);
            break;
        case 0x050E0000:
        {
            unsigned short text[128];
            VIUI &ui = m_ui;
            VIString::snprintf16(text, 256, "%S?", ui.m_words.GetString(830, ui.m_language));
            m_wndGame->Prompt(text, 1, 0, 0x050E0000);
            break;
        }
        case 0x050F0000:
            SendOpcodeByte(65, 7);
            break;
        case 0x050F1000:
            SendOpByteRaw(65, 8, m_target);
            break;
        }
    }
    else
        ShowMsg(1021);
}

// 0x00BDC8F8, name provisional: a ranged weapon's ammunition.  Attack type 7 wants an item
// of kind 1 in the pack and not worn, 9 one of kind 2, 10 nothing; otherwise word 724 or
// 725, in red.
int VIClient::HasAmmoFor(VINetItem *weapon)
{
    switch (weapon->m_attackType)
    {
    case 7:
        for (int i = 0; i < GetInvCount(); i++)
            if (m_inventory.Item(i)->m_p18 == 1 && m_inventory.Item(i)->m_wearSlot == -1)
                return 1;
        break;
    case 9:
        for (int i = 0; i < GetInvCount(); i++)
            if (m_inventory.Item(i)->m_p18 == 2 && m_inventory.Item(i)->m_wearSlot == -1)
                return 1;
        break;
    case 10:
        return 1;
    }
    if (weapon->m_attackType == 7)
    {
        VIUI &ui = m_ui;
        m_wndGame->AddText((const unsigned short *)ui.m_words.GetString(724, ui.m_language), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    }
    else
    {
        VIUI &ui = m_ui;
        m_wndGame->AddText((const unsigned short *)ui.m_words.GetString(725, ui.m_language), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    }
    return 0;
}

// 0x00BDF9D0, name provisional: `item` into the list, onto a stack of the same id up to
// `limit`, else into a new slot while there are fewer than `capacity`; a limit under 2
// does not stack and puts in one.  Returns how many went in.
int VIItemList::Merge(VINetItem &item, int limit, int capacity)
{
    int at = -1;
    if (limit >= 2)
    {
        int n = item.Quantity;
        for (int i = 0; i < ItemCount; i++)
            if (ItemInternal[i].m_id == item.m_id)
            {
                at = i;
                break;
            }
        if (at != -1)
        {
            int have = ItemInternal[at].Quantity;
            if (have + n > limit)
            {
                n = limit - have;
                ItemInternal[at].Quantity = limit;
            }
            else
                ItemInternal[at].Quantity = have + n;
            return n;
        }
        if (ItemCount >= capacity)
            return 0;
        ItemInternal[ItemCount] = item;
        if (item.Quantity > limit)
        {
            n = limit;
            ItemInternal[ItemCount].Quantity = n;
        }
        else
            n = item.Quantity;
        ItemCount++;
        return n;
    }
    if (ItemCount < capacity)
    {
        ItemInternal[ItemCount] = item;
        ItemInternal[ItemCount].Quantity = 1;
        ItemCount++;
        return 1;
    }
    return 0;
}

// 0x00BDAA10: onto the stack already offered from the same inventory slot (the last one
// found), else into a new entry with the next running id while there are fewer than four.
void VIClient::TradeAddItem(VINetItem &item)
{
    int at = -1;
    for (int i = 0; i < m_tradeACount; i++)
        if (m_tradeA[i].m_item.m_invIndex == item.m_invIndex)
            at = i;
    if (at == -1)
    {
        if (m_tradeACount == 4)
            return;
        m_tradeA[m_tradeACount].m_item = item;
        m_tradeA[m_tradeACount].m_id = m_p2BAFC;
        m_p2BAFC++;
        m_tradeACount++;
    }
    else
        m_tradeA[at].m_item.Quantity += item.Quantity;
}

// 0x00BDAD08: the entry with running id `id` out, the rest closed up behind it.
void VIClient::TradeRemoveItem(int id)
{
    int at = -1;
    for (int i = 0; i < m_tradeACount; i++)
        if (m_tradeA[i].m_id == id)
        {
            at = i;
            break;
        }
    if (at == -1)
        return;
    for (; at <= m_tradeACount - 2; at++)
        m_tradeA[at] = m_tradeA[at + 1];
    m_tradeACount--;
}

// 0x00BDAB68: the other side's entry with running id `id` back into the trade copy of the
// inventory (stacked as the item allows, forty slots), and the rest closed up.
void VIClient::TradeRemoveSlot(int id)
{
    int at = -1;
    for (int i = 0; i < m_tradeBCount; i++)
        if (m_tradeB[i].m_id == id)
        {
            at = i;
            break;
        }
    if (at == -1)
        return;
    m_tradeInv.Merge(m_tradeB[at].m_item, m_tradeB[at].m_item.m_maxStack, 40);
    for (; at <= m_tradeBCount - 2; at++)
        m_tradeB[at] = m_tradeB[at + 1];
    m_tradeBCount--;
}

// 0x00BDA7E8: `n` of the trade copy's item `id` onto this side of the trade, onto the entry
// already holding that inventory slot or into a new one while there are fewer than four.
// The local item is constructed and never used.
void VIClient::TradeAddSlot(int id, int n)
{
    VINetItem item;
    int at = -1;
    int slot = m_tradeInv.Find(id);
    if (slot == -1)
        return;
    VINetItem *it = m_tradeInv.Item(slot);
    if (it->Quantity < n)
        return;
    for (int i = 0; i < m_tradeBCount; i++)
        if (m_tradeB[i].m_item.m_invIndex == it->m_invIndex)
        {
            at = i;
            break;
        }
    if (at == -1)
    {
        if (m_tradeBCount == 4)
            return;
        m_tradeB[m_tradeBCount].m_item = *it;
        m_tradeB[m_tradeBCount].m_item.Quantity = n;
        m_tradeB[m_tradeBCount].m_id = m_p2BAF8;
        m_p2BAF8++;
        m_tradeBCount++;
    }
    else
        m_tradeB[at].m_item.Quantity += n;
    m_tradeInv.Took(slot, n);
}

// 0x00BD8428, name provisional: the login window's choice.  A slot past the list, while
// there are fewer than eight, opens character creation; a character is played: 0x002A with
// its server id and look.
int VIClient::SelectCharacter(int i)
{
    if (i >= m_charCount && m_charCount < 8)
    {
        m_wndLogin->ShowWord(413);
        m_wndLogin->m_visible = 0;
        m_wnd2BB88->m_visible = 1;
        m_p2BD44 = m_wnd2BB88;
        SetActiveWindow(m_wnd2BB88->Focus());
    }
    else if (m_chars[i].m_serverId != -1)
    {
        m_p00058 = 4;
        ClearMusic();
        VICharRecord &c = m_chars[i];
        {
            VIMemStream out;
            unsigned short op = 42;
            out.Write(&op, 2, 1);
            out << VIMemStreamRaw(c.m_serverId) << VIMemStreamRaw(c.m_face) << VIMemStreamRaw(c.m_hairStyle)
                << VIMemStreamRaw(c.m_hairLength) << VIMemStreamRaw(c.m_hairColor);
            VINetSend(out, GetConn(251));
        }
        m_p502D4 = 1;
        m_p502D8 = 1;
    }
    return 0;
}

// 0x00BDCEA0, name provisional: the equipment slot an item goes to, by its wear slot.  A
// pair (ears, wrists, fingers, hands) keeps the one it is in, else takes the second when
// the first is taken; the second hand only for a WAR, RAN, MNK, BRD or RGE of 20 and up.
int VIClient::EquipSlotFor(VIItemList *list, VINetItem *item)
{
    int cls = m_player.m_class;
    int level = m_player.m_level;
    switch (item->m_wearSlot)
    {
    case -1:
        break;
    case 1:
        return 0;
    case 3:
        if (item->m_equipSlot == 2 || item->m_equipSlot == 3)
            return item->m_equipSlot;
        if (FindEquipped(list, 2, 0))
            return 3;
        return 2;
    case 4:
        return 4;
    case 5:
        return 5;
    case 6:
        if (item->m_equipSlot == 6 || item->m_equipSlot == 7)
            return item->m_equipSlot;
        if (FindEquipped(list, 6, 0))
            return 7;
        return 6;
    case 7:
        return 8;
    case 19:
        return 1;
    case 8:
        if (item->m_equipSlot == 9 || item->m_equipSlot == 10)
            return item->m_equipSlot;
        if (FindEquipped(list, 9, 0))
            return 10;
        return 9;
    case 2:
        return 21;
    case 9:
        return 11;
    case 10:
        return 12;
    case 11:
        return 13;
    case 13:
        return 17;
    case 14:
        if (item->m_equipSlot == 14 || item->m_equipSlot == 15)
            return item->m_equipSlot;
        if (FindEquipped(list, 14, 0) && level >= 20
            && (cls == 0 || cls == 6 || cls == 4 || cls == 1 || cls == 5))
            return 15;
        return 14;
    case 15:
        return 16;
    case 16:
        return 18;
    case 18:
        if (item->m_equipSlot == 19 || item->m_equipSlot == 20)
            return item->m_equipSlot;
        if (FindEquipped(list, 19, 0))
            return 20;
        return 19;
    case 12:
    case 20:
        return 14;
    case 21:
        return 15;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
        return 19;
    }
    return -1;
}

// 0x00BDB1D0, name provisional: 0x0366 asks for `name` on the buddy list -- not oneself
// (word 679) and not someone already on it (592); either is said in red instead.
void VIClient::AddBuddy(const char *name)
{
    unsigned short text[128];
    char self[24];
    GetPlayerName(self, 24);
    int word = 0;
    if (!VIString::strcasecmp(self, name))
    {
        VIUI &ui = m_ui;
        word = 679;
        VIString::snprintf16(text, 256, (const unsigned short *)ui.m_words.GetString(679, ui.m_language));
    }
    else
    {
        for (int i = m_buddies.Begin(); i != m_buddies.End(); i = m_buddies.Next(i))
        {
            const char *n = m_buddies.Value(i).m_name;
            if (!VIString::strcasecmp(n, name))
            {
                VIUI &ui = m_ui;
                word = 592;
                VIString::snprintf16(text, 256, (const unsigned short *)ui.m_words.GetString(592, ui.m_language), n);
                break;
            }
        }
    }
    if (word)
        m_wndGame->AddText(text, VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    else
    {
        VIMemStream out;
        out.WriteU16(870);
        out.WriteString(name);
        VINetSend(out, GetConn(251));
    }
}

// 0x00BD8128, name provisional: 0x0625 invites `who` to the group.  Not oneself (word 679),
// only as the leader of a group (640), not someone in it already (1023); a first invite
// marks the group as forming.
void VIClient::InviteToGroup(VIInviteRec *who)
{
    if (who->m_id == m_selfId)
    {
        ShowMsg(679);
        return;
    }
    if (InGroup() && !IsGroupLeader())
    {
        ShowMsg(640);
        return;
    }
    unsigned short text[256];
    unsigned short text2[256];
    for (int i = 0; i < GetP2BE4C(); i++)
        if (GetGroupMember(i)->m_id == who->m_id)
        {
            VIUI &ui = m_ui;
            VIString::snprintf16(text, 512, (const unsigned short *)ui.m_words.GetString(1023, ui.m_language), who->m_name);
            ShowText(text);
            return;
        }
    if (!InGroup())
    {
        m_p2BD78 = 1;
        m_inGroup = 1;
    }
    VIUI &ui = m_ui;
    VIString::snprintf16(text2, 512, (const unsigned short *)ui.m_words.GetString(637, ui.m_language), who->m_name);
    ShowText(text2);
    {
        VIMemStream out;
        out.WriteU16(0x0625);
        out << VIMemStreamRaw(who->m_id);
        out.WriteString(who->m_name);
        VINetSend(out, GetConn(251));
    }
    Reset();
}

// 0x00BE6268, name provisional: a character-creation form as `op` (0x002B).
void VIClient::SendNewChar(int op, const VINewChar *c)
{
    VIMemStream out;
    unsigned short o = op;
    out.Write(&o, 2, 1);
    out.WriteString(c->m_name);
    out.WriteSigned(c->m_p18);
    out.WriteSigned(c->m_race);
    out.WriteSigned(c->m_class);
    out.WriteSigned(c->m_sex);
    out.WriteSigned(c->m_hairColor);
    out.WriteSigned(c->m_hairLength);
    out.WriteSigned(c->m_hairStyle);
    out.WriteSigned(c->m_face);
    out.WriteSigned(c->m_p38);
    out.Write(c->m_stats, 4, 7);
    VINetSend(out, GetConn(251));
}

// 0x00BD8770, name provisional: the creation form becomes the next character-select
// entry (no server id yet) and is sent as 0x002B.
void VIClient::CreateCharacter(VINewChar *c)
{
    const VIRaceDef *race = ::GetRace(c->m_race);
    const unsigned int *models = &race->m_maleModel;            // male, female
    memset(&m_chars[m_charCount], 0, sizeof(VICharRecord));
    m_chars[m_charCount].m_name[0] = 0;
    m_chars[m_charCount].m_race = c->m_race;
    m_chars[m_charCount].m_serverId = -1;
    m_chars[m_charCount].m_modelId = models[c->m_sex];
    m_chars[m_charCount].m_class = c->m_class;
    m_chars[m_charCount].m_hairColor = c->m_hairColor;
    m_chars[m_charCount].m_hairLength = c->m_hairLength;
    m_chars[m_charCount].m_hairStyle = c->m_hairStyle;
    m_chars[m_charCount].m_face = c->m_face;
    m_chars[m_charCount].m_robe = -1;
    m_chars[m_charCount].m_level = 1;
    m_wnd2BB88->m_visible = 0;
    m_wndLogin->m_visible = 1;
    m_p2BD44 = m_wndLogin;
    m_wndLogin->m_p428 = m_charCount;
    SetActiveWindow(m_p2BD44);
    m_wndLogin->ShowCharacter(models[c->m_sex], c->m_race, c->m_class, c->m_hairColor,
                              c->m_hairLength, c->m_hairStyle, c->m_face, -1, 0, 0, 0, 0, 0, 0);
    m_charCount++;
    m_wndLogin->ShowWord(413);
    SendNewChar(43, c);
}

// The vector sum the engine's TUs each define for themselves (viatmosphere.cpp,
// vilightning.cpp, viraster.cpp ...): three adds into the caller's slot.
static inline VIVect3 operator+(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x + b.x, a.y + b.y, a.z + b.z);
}

// 0x00BC7478, name provisional: dead reckoning for every other entity -- its position and
// heading moved on by its last velocities, scaled by the time to its next update; a
// creature flagged at +0x255 keeps pitch and roll level.
void VIClient::Extrapolate()
{
    for (int i = 0; i < 24; i++)
    {
        VIEntity *e = m_entities[i];
        if (!e || e->m_id == m_selfId)
            continue;
        VIVect3 vel = m_entVel[i];
        VIVect3 rot = m_entRot[i];
        float t = (float)(m_entStamp[i] - m_p07568) / (float)(gEntityUpdateRate * 2);
        if (!m_p2BD4C || t < 0.0f)
            t = 0.0f;
        vel *= t;
        rot *= t;
        VIVect3 hpr;
        e->SetActorPos(*e->GetPos() + vel);
        hpr = *e->GetHpr() + rot;
        if (e->Virtual3(130) && ((VICreatureEntity *)e)->m_p255)
        {
            hpr.y = 0.0f;
            hpr.z = 0.0f;
        }
        e->SetActorHpr(hpr);
    }
}

// 0x00CF4D80: ConnectStatusString's buffer, for the two messages that carry a code.
static unsigned short sConnectStatus[256];

// 0x00BC2268: the text for the last disconnect reason (+0x4F0E8), two of them with the
// server's code (+0x4F0EC) when there is one.  The name is the function's own printf.
const unsigned short *VIClient::ConnectStatusString()
{
    switch (m_p4F0E8)
    {
    case 0:
        if (m_p4F0EC != -1)
        {
            VIUI &ui = m_ui;
            VIString::snprintf16(sConnectStatus, 512, "%S (%S: %d)", ui.m_words.GetString(272, ui.m_language),
                                 ui.m_words.GetString(819, ui.m_language), m_p4F0EC);
            return sConnectStatus;
        }
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(272, ui.m_language);
        }
    case 1:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(666, ui.m_language);
        }
    case 2:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(431, ui.m_language);
        }
    case 3:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(270, ui.m_language);
        }
    case 4:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(593, ui.m_language);
        }
    case 5:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(594, ui.m_language);
        }
    case 6:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(620, ui.m_language);
        }
    case 7:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(621, ui.m_language);
        }
    case 8:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(619, ui.m_language);
        }
    case 9:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(820, ui.m_language);
        }
    case 10:
        if (m_p4F0EC == -1)
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(667, ui.m_language);
        }
        {
            VIUI &ui = m_ui;
            VIString::snprintf16(sConnectStatus, 512, "%S (%i)", ui.m_words.GetString(667, ui.m_language), m_p4F0EC);
        }
        return sConnectStatus;
    case 11:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(1017, ui.m_language);
        }
    case 12:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(821, ui.m_language);
        }
    case 20:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(758, ui.m_language);
        }
    case 21:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(822, ui.m_language);
        }
    case 13:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(677, ui.m_language);
        }
    case 14:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(716, ui.m_language);
        }
    case 17:
    case 18:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(1024, ui.m_language);
        }
    case 16:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(718, ui.m_language);
        }
    case 15:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(719, ui.m_language);
        }
    case 19:
        {
            VIUI &ui = m_ui;
            return (const unsigned short *)ui.m_words.GetString(1420, ui.m_language);
        }
    default:
        printf("ConnectStatusString() : error\n");
        sConnectStatus[0] = 0;
        return sConnectStatus;
    }
}

// 0x00BD53A0: a teleport to `pos` in `world`.  Vibration and music stop, the world is
// entered, and the walker is set down on the ground a sphere finds below: cast from 300
// above to 300 below, then again from 20 above the first hit.
void VIClient::MoveTo(int world, const VIVect3 &pos)
{
    VIVect3 dir(0.0f, 0.0f, 1.0f);
    VIVect3 p;
    VIVect3 top;
    VIVect3 bottom;
    VISceneFilter filter;
    VICollSphere sphere;
    VIEntity *e = FindEntity(m_selfId);
    if (VIGetGameOptions()->m_x97)
        m_console.m_pad.StopVibration();
    VIScene &scene = m_scene;
    VIWalkCamera &cam = m_walkCam;
    m_battleMusic.StopNow();
    scene.StopSoundTrack(m_loader, 1);
    p = pos;
    EnterWorld(world, pos);
    top.Set(p.x, p.y + 300.0f, p.z);
    bottom.Set(p.x, p.y - 300.0f, p.z);
    if (e)
    {
        filter.Init(e->m_actor);
        cam.m_parm64 = e->m_actor;
    }
    sphere.Init(top, bottom, 0.1f);
    scene.Collide(sphere, filter);
    {
        VIVect3 hit = sphere.GetHitPoint();
        p.Set(hit.x + 1.0f, hit.y + 1.0f, hit.z + 1.0f);
    }
    top.Set(p.x, p.y + 10.0f + 10.0f, p.z);
    bottom = p;
    scene.Collide(sphere, filter);
    {
        VIVect3 hit = sphere.GetHitPoint();
        p.Set(hit.x, hit.y - 0.2f, hit.z);
    }
    cam.SetLocation(p, dir, m_camera, scene);
    cam.SetOrbitAngleBehind();
    m_p00070 = 0;
    m_p00074 = 0;
    m_p00078 = 0;
    scene.SetSky(m_camera, 1);
}

// 0x00BD5628: MoveTo's ground-finding for 0x00B2's respawn -- cast from 3 above to 3 below,
// then from 20 above the first hit -- with the heading the server gave.  The VIESFParse is
// constructed and never used.
void VIClient::Respawn(int world, VIVect3 &pos, VIVect3 &dir)
{
    VIVect3 p;
    VIVect3 top;
    VIVect3 bottom;
    VISceneFilter filter;
    VICollSphere sphere;
    VIEntity *e = FindEntity(m_selfId);
    VIESFParse parse;
    if (VIGetGameOptions()->m_x97)
        m_console.m_pad.StopVibration();
    VIScene &scene = m_scene;
    VIWalkCamera &cam = m_walkCam;
    m_battleMusic.StopNow();
    scene.StopSoundTrack(m_loader, 1);
    p = pos;
    EnterWorld(world, p);
    top.Set(p.x, p.y + 3.0f, p.z);
    bottom.Set(p.x, p.y - 3.0f, p.z);
    if (e)
    {
        filter.Init(e->m_actor);
        cam.m_parm64 = e->m_actor;
    }
    sphere.Init(top, bottom, 0.1f);
    scene.Collide(sphere, filter);
    {
        VIVect3 hit = sphere.GetHitPoint();
        p.Set(hit.x + 1.0f, hit.y + 1.0f, hit.z + 1.0f);
    }
    top.Set(p.x, p.y + 10.0f + 10.0f, p.z);
    bottom = p;
    scene.Collide(sphere, filter);
    {
        VIVect3 hit = sphere.GetHitPoint();
        p.Set(hit.x, hit.y - 0.2f, hit.z);
    }
    cam.SetLocation(p, dir, m_camera, scene);
    cam.SetOrbitAngleBehind();
    m_p00070 = 0;
    m_p00074 = 0;
    m_p00078 = 0;
    scene.SetSky(m_camera, 1);
}

// 0x00BC55D8, name provisional: the screen fade -- m_fadeCur moved toward m_fadeTo at
// m_fadeRate (a frame over a tenth of a second counts as a thirtieth), then drawn over the
// whole screen in m_fadeColor at that alpha, or in half-alpha black when forced.
void VIClient::DrawFade(int force)
{
    float dt = m_scene.m_particleSystem.DeltaTime();
    if (dt > 0.1f)
        dt = 1.0f / 30.0f;
    m_fadeCur += m_fadeRate * dt;
    if (m_fadeRate > 0.0f)
        m_fadeCur = VIMIN(m_fadeCur, m_fadeTo);
    else
        m_fadeCur = VIMAX(m_fadeTo, m_fadeCur);
    if (force || m_fadeCur > 0.0f)
    {
        VIRaster &r = m_raster;
        if (m_fadeMat < 0)
        {
            if ((m_fadeMat = r.CreateMaterial(1)) < 0)
                return;
            VIMaterial *m = r.Material(m_fadeMat);
            m->InitSolidFill();
            m->SetLayerBlendMode(0, (VIAlphaBlendMode)3);
        }
        r.SetMaterial(m_fadeMat);
        r.Begin2D();
        r.BeginTriStrip();
        VIColor32F c = m_fadeColor;
        c.a *= m_fadeCur;
        if (force)
            c = VIColor32F(0.0f, 0.0f, 0.0f, 0.5f);
        r.Color(c);
        VIVect2I v;
        v.Set(-1, r.m_screenH + 1);
        r.Vertex(v);
        v.Set(-1, -1);
        r.Vertex(v);
        v.Set(r.m_screenW + 1, r.m_screenH + 1);
        r.Vertex(v);
        v.Set(r.m_screenW + 1, -1);
        r.Vertex(v);
        r.EndTriStrip();
        r.End2D();
    }
}

// A colour from its bytes, the way VIFont::SetTextColor(const VIColor32 &) does it; inline,
// so a constant colour folds to four float constants.
static inline VIColor32F ColorF(const VIColor32 &c)
{
    return VIColor32F(c.r * (1.0f / 255.0f), c.g * (1.0f / 255.0f), c.b * (1.0f / 255.0f), c.a * (1.0f / 255.0f));
}

// 0x00BC5840, name provisional: the debug location readout.  In a world of one grid cell,
// the walker's position; otherwise the world's name and the cell, then the position
// inside the cell.
void VIClient::DrawPosition()
{
    unsigned short name[64];
    unsigned short text[128];
    VIVect3 pos;
    VIVect3 off;
    VIColor32F col = ColorF(VIColor32(185, 80, 250, 255));
    VIUI &ui = m_ui;
    VIFont &font = ui.m_font;
    m_raster.Begin2D();
    pos = m_walkCam.m_committed.m_position;
    font.SetTextColor(col);
    if (m_scene.GetWorldGridNumCols() == 1 && m_scene.GetWorldGridNumRows() == 1)
        VIString::snprintf16(text, 256, "%.2f,%.2f,%.2f", pos.x, pos.y, pos.z);
    else
    {
        int x, z;
        m_scene.CalcWorldGridOffset(pos, x, z, off);
        VIUI &ui = m_ui;
        name[0] = 0;
        if ((unsigned int)m_worldId < 6)
            VIString::strcpy16(name, 128, (const unsigned short *)ui.m_words.GetString(gWorldInfo[m_worldId].m_nameWord, ui.m_language));
        VIString::snprintf16(text, 256, "%S (%02d%02d)", name, x - 1, z - 1);
        VIFont &font = ui.m_font;
        font.DrawString(ui.m_raster, ui.m_dict, text, 130, 24);
        VIString::snprintf16(text, 256, "%.2f,%.2f,%.2f", off.x, off.y, off.z);
    }
    font.DrawString(ui.m_raster, ui.m_dict, text, 130, 43);
    font.SetTextColor(VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
    m_raster.End2D();
}

// 0x00BCA060, name provisional (no direct callers): stats 12 and 13 with gear.  The best
// single bonus from a worn, unbroken item counts in full and every other one at a tenth,
// rounded, on top of the base value.
int VIClient::GearStat(VIItemList *list, int stat)
{
    if ((unsigned int)(stat - 12) < 2)
    {
        int base = m_player.m_baseStat[stat];
        int bestItem = -1;
        int bestStat = -1;
        int best = 0;
        int i;
        for (i = 0; i < list->ItemCount; i++)
        {
            VINetItem *it = list->Item(i);
            if (it->m_equipSlot == -1)
                continue;
            if (!it->m_curHp && it->m_maxHp)
                continue;
            for (int j = 0; j < it->m_stats.m_size; j++)
                if (it->m_stats[j].m_id == stat && it->m_stats[j].m_value > 0
                    && (bestItem == -1 || best < it->m_stats[j].m_value))
                {
                    best = it->m_stats[j].m_value;
                    bestItem = i;
                    bestStat = j;
                }
        }
        if (bestItem != -1)
        {
            float rest = 0.0f;
            for (i = 0; i < list->ItemCount; i++)
            {
                VINetItem *it = list->Item(i);
                if (it->m_equipSlot == -1)
                    continue;
                if (!it->m_curHp && it->m_maxHp)
                    continue;
                for (int j = 0; j < it->m_stats.m_size; j++)
                {
                    if (i == bestItem && j == bestStat)
                        continue;
                    if (it->m_stats[j].m_id == stat && it->m_stats[j].m_value > 0)
                        rest += it->m_stats[j].m_value * 0.1f;
                }
            }
            float r = rest < 0.0f ? rest - 0.5f : rest + 0.5f;
            return base + (int)r + best;
        }
    }
    return 0;
}

// 0x00BCB610, name provisional: item i into equipment slot `slot`, if it can go there,
// with whatever it displaces taken off first (a two-hander both hands, a robe the body
// pieces, and so on).
void VIClient::EquipItem(VIItemList *list, int i, int slot)
{
    if (!CanWearIn(list->Item(i)->m_wearSlot, slot))
        return;
    list->Unequip(slot);
    switch (list->Item(i)->m_wearSlot)
    {
    case 6:
        list->Unequip(8);
        break;
    case 7:
        list->Unequip(7);
        list->Unequip(6);
        break;
    case 12:
    case 20:
        list->Unequip(16);
        list->Unequip(18);
        list->Unequip(19);
        break;
    case 13:
        list->Unequip(15);
        list->Unequip(16);
        list->Unequip(18);
        list->Unequip(20);
        break;
    case 14:
        list->Unequip(16);
        list->Unequip(18);
        if (slot == 15)
        {
            list->Unequip(17);
            list->Unequip(20);
        }
        else
            list->Unequip(19);
        break;
    case 18:
        list->Unequip(16);
        list->Unequip(18);
        if (slot == 20)
        {
            list->Unequip(17);
            list->Unequip(15);
        }
        else
            list->Unequip(14);
        break;
    case 15:
        list->Unequip(14);
        list->Unequip(15);
        list->Unequip(17);
        list->Unequip(18);
        list->Unequip(19);
        list->Unequip(20);
        break;
    case 16:
        list->Unequip(14);
        list->Unequip(15);
        list->Unequip(17);
        list->Unequip(16);
        list->Unequip(19);
        list->Unequip(20);
        break;
    case 21:
        list->Unequip(16);
        list->Unequip(18);
        if (slot == 15)
        {
            list->Unequip(17);
            list->Unequip(20);
        }
        else
            list->Unequip(19);
        break;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
        list->Unequip(16);
        list->Unequip(18);
        list->Unequip(14);
        break;
    }
    list->SetEquipSlot(i, slot);
}

// 0x00BBD130 (VIClient.cpp's own dead copy is 0x00BDFD40), name provisional: Merge, with the
// list's running index (m_p04) stamped on a new entry and handed back in item.m_invIndex.
int VIItemList::AddItem(VINetItem &item, int limit, int capacity)
{
    int at = -1;
    if (limit >= 2)
    {
        int n = item.Quantity;
        for (int i = 0; i < ItemCount; i++)
            if (ItemInternal[i].m_id == item.m_id)
            {
                at = i;
                break;
            }
        if (at != -1)
        {
            int have = ItemInternal[at].Quantity;
            if (have + n > limit)
            {
                n = limit - have;
                ItemInternal[at].Quantity = limit;
            }
            else
                ItemInternal[at].Quantity = have + n;
            item.m_invIndex = ItemInternal[at].m_invIndex;
            return n;
        }
        if (ItemCount >= capacity)
            return 0;
        ItemInternal[ItemCount] = item;
        ItemInternal[ItemCount].m_invIndex = m_p04++;
        item.m_invIndex = ItemInternal[ItemCount].m_invIndex;
        if (item.Quantity > limit)
        {
            n = limit;
            ItemInternal[ItemCount].Quantity = n;
        }
        else
            n = item.Quantity;
        ItemCount++;
        return n;
    }
    if (ItemCount < capacity)
    {
        ItemInternal[ItemCount] = item;
        ItemInternal[ItemCount].Quantity = 1;
        ItemInternal[ItemCount].m_invIndex = m_p04++;
        item.m_invIndex = ItemInternal[ItemCount].m_invIndex;
        ItemCount++;
        return 1;
    }
    return 0;
}

// 0x00BD7E78, name provisional: InviteToGroup for an entity -- it must exist (word 678), be a
// creature (588), a player (1015) and have +0x264 set (1014) -- then the same checks and the
// same 0x0625.
void VIClient::InviteEntity(int id)
{
    unsigned short text[256];
    VIEntity *e = FindEntity(id);
    if (!e)
    {
        VIUI &ui = m_ui;
        ShowText((const unsigned short *)ui.m_words.GetString(678, ui.m_language));
        return;
    }
    if (!e->Virtual3(130))
    {
        VIUI &ui = m_ui;
        ShowText((const unsigned short *)ui.m_words.GetString(588, ui.m_language));
        return;
    }
    VICreatureEntity *c = (VICreatureEntity *)e;
    if (c->m_p256 != 2)
    {
        ShowMsg(1015);
        return;
    }
    if (!c->m_p264)
    {
        ShowMsg(1014);
        return;
    }
    if (id == m_selfId)
    {
        ShowMsg(679);
        return;
    }
    if (InGroup() && !IsGroupLeader())
    {
        ShowMsg(640);
        return;
    }
    for (int i = 0; i < GetP2BE4C(); i++)
        if (GetGroupMember(i)->m_id == id)
        {
            VIUI &ui = m_ui;
            VIString::snprintf16(text, 512, (const unsigned short *)ui.m_words.GetString(1023, ui.m_language), c->m_name);
            ShowText(text);
            return;
        }
    if (!InGroup())
    {
        m_p2BD78 = 1;
        m_inGroup = 1;
    }
    VIUI &ui = m_ui;
    VIString::snprintf16(text, 512, (const unsigned short *)ui.m_words.GetString(637, ui.m_language), c->m_name);
    ShowText(text);
    {
        VIMemStream out;
        out.WriteU16(0x0625);
        out << VIMemStreamRaw(id);
        out.WriteString(c->m_name);
        VINetSend(out, GetConn(251));
    }
    Reset();
}

// 0x00BD8B40: the camera styles.  0 is first person at the race's eye height, 2 the fixed
// follow, 1 and 3 the two orbits (3 at the race's height, both swung round behind the
// walker); anything else is first person without the eye height.
int VIClient::SetCameraStyle(int style)
{
    switch (style)
    {
    case 0:
    {
        VIWalkCamera &cam = m_walkCam;
        cam.SetPitchRange(-1.5707964f, 1.5707964f);
        cam.m_turnRate = 1.5707964f;
        cam.InitIntermediate();
        cam.SetCameraMode((VIWalkCameraMode)0);
        cam.m_lookAtHeight = GetRaceCamera(GetRace(), ExpansionFlags())->m_p34;
        break;
    }
    case 2:
    {
        VIWalkCamera &cam = m_walkCam;
        cam.m_targetHeight = 1.68f;
        cam.m_minOrbitDistance = 1.5f;
        cam.m_maxOrbitDistance = 25.0f;
        cam.m_minCollisionDist = 3.5f;
        cam.SetOrbitDistance(3.38f);
        cam.SetPitchRange(-0.7853982f, 1.5707964f);
        cam.m_turnRate = 1.5707964f;
        cam.InitIntermediate();
        cam.m_lookAtHeightAbove = 1.68f;
        cam.SetCameraMode((VIWalkCameraMode)2);
        break;
    }
    case 3:
    {
        VIWalkCamera &cam = m_walkCam;
        cam.m_targetHeight = 1.68f;
        cam.m_minOrbitDistance = 1.5f;
        cam.m_maxOrbitDistance = 8.0f;
        cam.m_minCollisionDist = 3.5f;
        cam.SetOrbitDistance(3.38f);
        cam.SetPitchRange(-0.7853982f, 1.5707964f);
        cam.SetDefOrbitPitchAngle(0.2617994f);
        cam.m_turnRate = 6.2831855f;
        cam.InitIntermediate();
        cam.m_lookAtHeightAbove = GetRaceCamera(GetRace(), ExpansionFlags())->m_p34;
        cam.SetCameraMode((VIWalkCameraMode)3);
        cam.ChooseOrbitAngle(m_walkCam.m_committed.m_angles.x + 3.1415927f, m_scene);
        break;
    }
    case 1:
    {
        VIWalkCamera &cam = m_walkCam;
        cam.m_targetHeight = 1.1f;
        cam.m_minOrbitDistance = 1.5f;
        cam.m_maxOrbitDistance = 25.0f;
        cam.m_minCollisionDist = 3.5f;
        cam.SetPitchRange(-1.5707964f, 1.5707964f);
        cam.SetDefOrbitPitchAngle(0.43633232f);
        cam.m_turnRate = 1.5707964f;
        cam.InitIntermediate();
        cam.SetCameraMode((VIWalkCameraMode)1);
        cam.ChooseOrbitAngle(m_walkCam.m_committed.m_angles.x + 3.1415927f, m_scene);
        break;
    }
    default:
    {
        VIWalkCamera &cam = m_walkCam;
        cam.SetPitchRange(-1.5707964f, 1.5707964f);
        cam.m_turnRate = 1.5707964f;
        cam.InitIntermediate();
        cam.SetCameraMode((VIWalkCameraMode)0);
        break;
    }
    }
    return 0;
}

// 0x00BC8700: one console input event into the journal, tagged by kind (3 a key, 4 the
// pointer, 5 the pad, 6 a quit) and stamped with the time; 0 when there was none.
int VIClient::JournalConsole()
{
    unsigned char data[0x10000];
    int type;
    if (m_console.GetInput(&type, data, 0x10000))
        return 0;
    switch (type)
    {
    case 2:
        m_journal.Write((unsigned char)3);
        m_journal.Write(VITimer::GetTime());
        m_journal.Write(data[0]);
        m_journal.Write(data[1]);
        break;
    case 3:
        m_journal.Write((unsigned char)4);
        m_journal.Write(VITimer::GetTime());
        m_journal.Write(((int *)data)[0]);
        m_journal.Write(((int *)data)[1]);
        break;
    case 4:
        m_journal.Write((unsigned char)5);
        m_journal.Write(VITimer::GetTime());
        m_journal.Write(data[0]);
        m_journal.Write(data[1]);
        m_journal.Write(data + 4, 4, 20);
        break;
    case 1:
        m_journal.Write((unsigned char)6);
        m_journal.Write(VITimer::GetTime());
        break;
    }
    return 1;
}

// 0x00BC89D0: one received DRDP packet into the journal -- tag 2, the time, its size as a
// halfword, then its bytes; 0 when there was none.
int VIClient::JournalNetwork()
{
    unsigned char data[0x10000];
    int size;
    if (!m_drdp)
        return 0;
    if (drdp_receive_packet(m_drdp, data, 0x10000, &size, 0))
        return 0;
    m_journal.Write((unsigned char)2);
    m_journal.Write(VITimer::GetTime());
    m_journal.Write((unsigned short)size);
    m_journal.Write(data, 1, size);
    return 1;
}

// 0x00BD31C0: the group roster -- count, member ids and names -- with each old member's last
// known state replayed onto it, and the group marked as formed when we are member 0.
void VIClient::Msg0772(VIMemStream &s)
{
    if (!InGroup())
        return;
    int x;
    s.Read(&x, 4, 1);
    VIPartyMember old[4];
    memcpy(old, m_group, sizeof(old));
    int n = m_p2BE4C;
    s.Read(&m_p2BE4C, 4, 1);
    int i;
    for (i = 0; i < m_p2BE4C; i++)
    {
        s.Read(&m_group[i].m_id, 4, 1);
        m_group[i].m_active = 0;
        m_group[i].m_hpKnown = 0;
        m_group[i].m_hp = 0;
    }
    for (i = 0; i < m_p2BE4C; i++)
        s.ReadString(m_group[i].m_name, 24);
    for (i = 0; i < n; i++)
        if (old[i].m_active)
            UpdateGroupMember(old[i].m_id, old[i].m_hpKnown, old[i].m_hp, old[i].m_p0C, old[i].m_pos);
    if (m_group[0].m_id == m_selfId)
        m_p2BD78 = 1;
}

// 0x00BCB2F0: 0x003F -- EquipItem on the inventory itself, then the equip sound (1 for a
// weapon, 5 for anything else).
void VIClient::Msg003F(int i, int slot)
{
    if (!CanWearIn(m_inventory.Item(i)->m_wearSlot, slot))
        return;
    m_inventory.Unequip(slot);
    switch (m_inventory.Item(i)->m_wearSlot)
    {
    case 6:
        m_inventory.Unequip(8);
        break;
    case 7:
        m_inventory.Unequip(7);
        m_inventory.Unequip(6);
        break;
    case 12:
    case 20:
        m_inventory.Unequip(16);
        m_inventory.Unequip(18);
        m_inventory.Unequip(19);
        break;
    case 13:
        m_inventory.Unequip(15);
        m_inventory.Unequip(16);
        m_inventory.Unequip(18);
        m_inventory.Unequip(20);
        break;
    case 14:
        m_inventory.Unequip(16);
        m_inventory.Unequip(18);
        if (slot == 15)
        {
            m_inventory.Unequip(17);
            m_inventory.Unequip(20);
        }
        else
            m_inventory.Unequip(19);
        break;
    case 18:
        m_inventory.Unequip(16);
        m_inventory.Unequip(18);
        if (slot == 20)
        {
            m_inventory.Unequip(17);
            m_inventory.Unequip(15);
        }
        else
            m_inventory.Unequip(14);
        break;
    case 15:
        m_inventory.Unequip(14);
        m_inventory.Unequip(15);
        m_inventory.Unequip(17);
        m_inventory.Unequip(18);
        m_inventory.Unequip(19);
        m_inventory.Unequip(20);
        break;
    case 16:
        m_inventory.Unequip(14);
        m_inventory.Unequip(15);
        m_inventory.Unequip(17);
        m_inventory.Unequip(16);
        m_inventory.Unequip(19);
        m_inventory.Unequip(20);
        break;
    case 21:
        m_inventory.Unequip(16);
        m_inventory.Unequip(18);
        if (slot == 15)
        {
            m_inventory.Unequip(17);
            m_inventory.Unequip(20);
        }
        else
            m_inventory.Unequip(19);
        break;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
        m_inventory.Unequip(16);
        m_inventory.Unequip(18);
        m_inventory.Unequip(14);
        break;
    }
    m_inventory.SetEquipSlot(i, slot);
    if (!m_inventory.Item(i)->m_attackType)
        PlaySound(5);
    else
        PlaySound(1);
}

// 0x00BDBD58: an NPC dialog -- its id, text and up to six options, each expanded.  With the
// loot window up the window takes it; otherwise it waits here for an answer, unless one is
// already waiting or the game window refuses, when it is declined at once (0x0035, 255).
void VIClient::Msg0034(VIMemStream &s)
{
    VIDialogBox d;
    unsigned short raw[128];
    s.Read(&d.m_id, 4, 1);
    s.ReadString(raw, 256);
    m_text.Expand(d.m_text, 128, raw, 1);
    s >> d.m_count;
    int i;
    for (i = 0; i < d.m_count; i++)
    {
        unsigned short opt[64];
        memset(opt, 0, sizeof(opt));
        s.ReadString(opt, 128);
        m_text.Expand(d.m_options[i], 64, opt, 1);
    }
    if (m_wndGame->IsLootable())
    {
        m_wndGame->ShowDialogBox(&d);
        return;
    }
    if (m_wndGame->DialogAllowed() && !m_p2EBFC)
    {
        m_dialog.m_id = d.m_id;
        memcpy(m_dialog.m_text, d.m_text, sizeof(d.m_text));
        m_dialog.m_count = d.m_count;
        for (i = 0; i < d.m_count; i++)
            memcpy(m_dialog.m_options[i], d.m_options[i], sizeof(d.m_options[i]));
        m_p2EBFC = 1;
    }
    else
        SendMsg(53, d.m_id, 255);
}

// 0x00BDDCF0, name provisional: one id's four (label, text) pairs from narrow strings, each
// one optional -- sent as 0x006A (for id 0, as four 0x006C entries instead) and kept in
// VITextSet's table.
int VIClient::SetTextSet(int id, const char *l0, const char *t0, const char *l1, const char *t1,
                         const char *l2, const char *t2, const char *l3, const char *t3)
{
    VIRec6A r;
    r.m_p00 = id;
    r.m_p04.Init(4);
    memset(&r.m_p04[0], 0, sizeof(VIRec388));
    memset(&r.m_p04[1], 0, sizeof(VIRec388));
    memset(&r.m_p04[2], 0, sizeof(VIRec388));
    memset(&r.m_p04[3], 0, sizeof(VIRec388));
    r.m_p04[0].m_p00 = 0;
    if (l0)
        VIString::str8to16(r.m_p04[0].m_p104, sizeof(r.m_p04[0].m_p104), l0);
    if (t0)
        VIString::str8to16(r.m_p04[0].m_p04, sizeof(r.m_p04[0].m_p04), t0);
    r.m_p04[1].m_p00 = 1;
    if (l1)
        VIString::str8to16(r.m_p04[1].m_p104, sizeof(r.m_p04[1].m_p104), l1);
    if (t1)
        VIString::str8to16(r.m_p04[1].m_p04, sizeof(r.m_p04[1].m_p04), t1);
    r.m_p04[2].m_p00 = 2;
    if (l2)
        VIString::str8to16(r.m_p04[2].m_p104, sizeof(r.m_p04[2].m_p104), l2);
    if (t2)
        VIString::str8to16(r.m_p04[2].m_p04, sizeof(r.m_p04[2].m_p04), t2);
    r.m_p04[3].m_p00 = 3;
    if (l3)
        VIString::str8to16(r.m_p04[3].m_p104, sizeof(r.m_p04[3].m_p104), l3);
    if (t3)
        VIString::str8to16(r.m_p04[3].m_p04, sizeof(r.m_p04[3].m_p04), t3);
    if (id == 0)
    {
        Send006C(0, &r.m_p04[0]);
        Send006C(0, &r.m_p04[1]);
        Send006C(0, &r.m_p04[2]);
        Send006C(0, &r.m_p04[3]);
    }
    else
        Send006A(&r);
    VITextSet *set = VITextSet::Get(id);
    if (l0)
        set->m_pairs[0].m_p00 = NewWide(l0);
    if (t0)
        set->m_pairs[0].m_p04 = NewWide(t0);
    if (l1)
        set->m_pairs[1].m_p00 = NewWide(l1);
    if (t1)
        set->m_pairs[1].m_p04 = NewWide(t1);
    if (l2)
        set->m_pairs[2].m_p00 = NewWide(l2);
    if (t2)
        set->m_pairs[2].m_p04 = NewWide(t2);
    if (l3)
        set->m_pairs[3].m_p00 = NewWide(l3);
    if (t3)
        set->m_pairs[3].m_p04 = NewWide(t3);
    return 0;
}

// 0x00BC92B0, name provisional: target the entity nearest the middle of the screen -- any
// visible one but the player and the pet (and a creature whose +0x256 is 3), measured at
// node 4 when its sprite is a character's.
void VIClient::TargetCentre()
{
    VIEntity *best = 0;
    const VIMatrix44 &vp = m_camera.m_viewProj;
    unsigned int old = m_target;
    float bestDist = 999999.0f;
    for (int i = 0; i < 24; i++)
    {
        VIVect3 pos;
        VIEntity *e = m_entities[i];
        if (e == 0 || e->m_actor == -1)
            continue;
        pos = e->GetActorPos();
        if (e->m_id == m_selfId || e->m_id == m_petId)
            continue;
        if (e->Virtual3(130) && ((VICreatureEntity *)e)->m_p256 == 3)
            continue;
        VISprite *s = m_scene.GetActorSpritePtr(e->m_actor);
        if (s == 0)
            continue;
        if (s->m_type == 5)
        {
            VIMatrix44 m;
            ((VICSprite *)s)->CalcSlotWorldTransform((VICSpriteNodeID)4, e->m_actor, m_scene, m);
            pos.x = m.m[3].x;
            pos.y = m.m[3].y;
            pos.z = m.m[3].z;
        }
        if (e->m_p18 & 1)
        {
            if (m_camera.m_frustum.Cull(pos, 0.0f))
                continue;
            VIVect4 h;
            VIVect3 scr;
            h.Set(VIVect4(pos.x, pos.y, pos.z, 1.0f) * vp);
            float inv = 1.0f / h.w;
            scr.x = h.x * inv;
            scr.y = h.y * inv;
            scr.z = 0.0f;
            float d = scr.Magnitude();
            if (d < bestDist)
            {
                bestDist = d;
                best = e;
            }
        }
    }
    if (best)
        m_target = best->m_id;
    else
        m_target = 0xFFFFFFFF;
    if (old != m_target)
    {
        m_autoAttack = 0;
        m_targetInfoValid = 0;
        SendOpIntInt(16, m_p2BB68, m_target);
        DrawReticle(1);
    }
    else
        DrawReticle(0);
}

extern "C" void qsort(void *base, unsigned int n, unsigned int size,
                      int (*cmp)(const void *, const void *));

// DrawNetStats' line for one state channel.  Names provisional.
struct VINetStat
{
    unsigned int   m_p00;                           // channel +0x08
    unsigned int   m_p04;                           // +0x0C: the second sort key
    unsigned int   m_p08;                           // +0x10
    unsigned int   m_send;                          // +0x14: bits/s
    unsigned int   m_recv;                          // +0x18: bits/s
    unsigned short m_last;                          // the newest ping
    unsigned short m_max;                           // the worst ping: the first sort key
    int            m_channel;                       // picks the colour
};

// 0x00BC5BE8, name provisional: DrawNetStats' qsort order, the worst ping first.
static int NetStatSort(const void *a, const void *b)
{
    const VINetStat *x = (const VINetStat *)a;
    const VINetStat *y = (const VINetStat *)b;
    if (x->m_max < y->m_max)
        return 1;
    if (x->m_max > y->m_max)
        return -1;
    if (x->m_p04 < y->m_p04)
        return 1;
    if (x->m_p04 > y->m_p04)
        return -1;
    return 0;
}

// 0x00BC5C38, name provisional: a line per live state channel, the worst ping first, each
// in its channel's colour.
void VIClient::DrawNetStats()
{
    VIColor32F colors[8] = {
        VIColor32F(1.0f, 1.0f, 0.0f, 1.0f),
        VIColor32F(1.0f, 0.0f, 1.0f, 1.0f),
        VIColor32F(0.0f, 1.0f, 1.0f, 1.0f),
        VIColor32F(0.7f, 0.7f, 0.7f, 1.0f),
        VIColor32F(0.7f, 0.7f, 1.0f, 1.0f),
        VIColor32F(0.7f, 1.0f, 0.7f, 1.0f),
        VIColor32F(1.0f, 0.7f, 0.7f, 1.0f),
        VIColor32F(1.0f, 1.0f, 1.0f, 1.0f),
    };
    VINetStat stats[40];
    int n = 0;
    for (int i = 0; i < 40; i++)
    {
        if (m_channels[i].m_size == 0)
            continue;
        stats[n].m_p00 = m_channels[i].m_p08;
        stats[n].m_p04 = m_channels[i].m_p0C;
        stats[n].m_p08 = m_channels[i].m_p10;
        stats[n].m_send = m_channels[i].m_p14;
        stats[n].m_recv = m_channels[i].m_p18;
        stats[n].m_last = m_channels[i].m_ping.Last();
        stats[n].m_max = m_channels[i].m_ping.Max();
        stats[n].m_channel = i;
        n++;
    }
    qsort(stats, n, sizeof(VINetStat), NetStatSort);
    VIUI &ui = m_ui;
    VIVect2I p;
    p.x = m_ui.m_font.GetHeight(ui.m_raster, ui.m_dict) + 189;
    p.y = m_ui.m_font.GetHeight(ui.m_raster, ui.m_dict) * 2;
    for (int j = 0; j < n; j++)
    {
        char buf[64];
        VIString::snprintf(buf, sizeof(buf), "%02u: %05ue %03up %04ums s%04ukbps r%04ukbps",
                           stats[j].m_p00, stats[j].m_p04, stats[j].m_p08, stats[j].m_last,
                           stats[j].m_send / 1000, stats[j].m_recv / 1000);
        m_ui.m_font.SetTextColor(colors[stats[j].m_channel % 8]);
        m_ui.m_font.DrawString(ui.m_raster, ui.m_dict, buf, p.x, p.y);
        p.y += m_ui.m_font.GetHeight(ui.m_raster, ui.m_dict);
    }
    m_ui.m_font.SetTextColor(VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
}

static inline VIVect3 Cross(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.y * b.z - a.z * b.y,
                   -a.x * b.z + a.z * b.x,
                   a.x * b.y - a.y * b.x);
}

// 0x00BCE270, name provisional: hold the camera at the player's eye -- the race camera's
// height above the actor -- facing the way the walk camera does (first person and fixed:
// its own heading; third person: along the camera), the pitch kept short of vertical.
void VIClient::BeginManualCamera()
{
    VIEntity *e = FindEntity(m_selfId);
    if (e == 0)
        return;
    VIWalkCamera &cam = m_walkCam;
    VIVect3 pos = e->GetActorPos();
    pos.y += GetRaceCamera(GetRace(), ExpansionFlags())->m_p34;
    VIVect3 right, up, dir, hpr;
    VIMatrix44 m;
    m.Identity();
    switch (cam.m_cameraMode)
    {
    case 0:
    case 2:
        hpr = m_walkCam.m_committed.m_angles;
        break;
    case 3:
        {
            const VIVect3 &d = m_camera.m_dir;
            dir.Set(d.x, d.y, d.z);
        }
        up.Set(0.0f, 1.0f, 0.0f);
        right = Cross(up, dir);
        up = Cross(dir, right);
        right.Normalize();
        up.Normalize();
        m.m[0].x = right.x;
        m.m[0].y = right.y;
        m.m[0].z = right.z;
        m.m[1].x = up.x;
        m.m[1].y = up.y;
        m.m[1].z = up.z;
        m.m[2].x = dir.x;
        m.m[2].y = dir.y;
        m.m[2].z = dir.z;
        m.ConvertToEulerZXY(hpr);
        break;
    default:
        return;
    }
    hpr.y = VIMIN(VIMAX(-1.5607964f, hpr.y), 1.5607964f);
    if (cam.m_cameraMode)
        m_walkCam.m_effect.BeginManual(pos, hpr, 0.2f);
    else
        m_walkCam.m_effect.BeginManual(pos, hpr, 0.0f);
    m_wndGame->ManualCameraBegun();
}

// 0x00BDC4B8, name provisional: is the target within reach of the item in `slot` (-1: bare
// hands) -- 4.5 for a melee weapon, the weapon's own range for a bow, a thrown weapon or
// a wand (which also need power and ammunition), 3 or 5 more for a large or huge target.
// Neither side may be a player when the player is.  Out of range with a ranged weapon,
// word 723, in red.
int VIClient::TargetInRange(int slot)
{
    VINetItem *item = m_inventory.Item(slot);
    VIVect3 a, b;
    float extra = a.x;
    float maxR, minR;
    if (m_target == 0xFFFFFFFF)
        return 0;
    VIEntity *t = FindEntity(m_target);
    if (t == 0 || t->m_p19 == 0)
        return 0;
    VIEntity *me = FindEntity(m_selfId);
    if (me == 0)
        return 0;
    if (!t->Virtual3(130))
        return 0;
    if (!me->Virtual3(130))
        return 0;
    if (((VICreatureEntity *)me)->m_p256 == 2
        && (((VICreatureEntity *)t)->m_p256 == 2 || ((VICreatureEntity *)t)->m_p256 == 3))
        return 0;
    if (m_target == m_selfId)
        return 0;
    if (((VICreatureEntity *)t)->m_npcFlags & 0x400)
        extra += 3.0f;
    else if (((VICreatureEntity *)t)->m_npcFlags & 0x800)
        extra += 5.0f;
    if (slot == -1)
    {
        maxR = (extra + 4.5f) * (extra + 4.5f);
        minR = 0.0f;
        a = *me->GetPos();
        b = *t->GetPos();
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        float d = dx * dx + dy * dy + dz * dz;
        if (minR <= d && d <= maxR)
            return 1;
        return 0;
    }
    switch (item->m_attackType)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        maxR = (extra + 4.5f) * (extra + 4.5f);
        minR = 0.0f;
        break;
    case 7:
    case 9:
    case 10:
        maxR = (item->m_range + extra) * (item->m_range + extra);
        minR = 0.0f;
        if (!CheckPower())
            return 0;
        if (!HasAmmoFor(item))
            return 0;
        break;
    case 0:
    default:
        return 0;
    }
    a = *me->GetPos();
    b = *t->GetPos();
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    float d = dx * dx + dy * dy + dz * dz;
    if (minR <= d && d <= maxR)
        return 1;
    if (item->m_attackType == 7 || item->m_attackType == 9 || item->m_attackType == 10)
    {
        VIUI &ui = m_ui;
        m_wndGame->AddText((const unsigned short *)ui.m_words.GetString(723, ui.m_language), VIColor32F(1.0f, 0.25f, 0.25f, 1.0f));
    }
    return 0;
}

// 0x00BC76A0, name provisional: one auto-attack swing, once it is due.  What is worn in
// slots 14..20 picks a swing; with the target in range and the swing naming an effect,
// the effect plays here, and otherwise the character swings, 0x00D8 goes out and the next
// swing waits two seconds.
void VIClient::AutoAttackTick(VIEntity *me)
{
    FindEntity(m_target);
    if (!m_autoAttack)
        return;
    if (m_p07568 < m_p071E8)
        return;
    if (m_scene.GetActorSpriteType(me->m_actor) != 5)
        return;
    int b16 = 0, b14 = 0, b15 = 0, b20 = 0, b18 = 0;
    int slot = -1;
    int b17 = 0;
    int anim = 23;
    int b19 = 0;
    int delay = 3500;
    int noEffect = 0;
    for (int i = 0; i < m_inventory.Count(); i++)
    {
        if (m_inventory.Item(i)->m_equipSlot == 14)
        {
            b14 = 1;
            anim = AttackAnim(m_inventory.Item(i)->m_attackType, 0);
            if (slot == -1)
                slot = i;
        }
        else if (m_inventory.Item(i)->m_equipSlot == 15)
            b15 = 1;
        else if (m_inventory.Item(i)->m_equipSlot == 17)
            b17 = 1;
        else if (m_inventory.Item(i)->m_equipSlot == 16)
        {
            b16 = 1;
            anim = AttackAnim(m_inventory.Item(i)->m_attackType, 0);
            if (slot == -1)
                slot = i;
        }
        else if (m_inventory.Item(i)->m_equipSlot == 18)
        {
            b18 = 1;
            anim = AttackAnim(m_inventory.Item(i)->m_attackType, 0);
            if (slot == -1)
                slot = i;
        }
        else if (m_inventory.Item(i)->m_equipSlot == 19)
        {
            noEffect = 1;
            b19 = 1;
        }
        else if (m_inventory.Item(i)->m_equipSlot == 20)
            b20 = 1;
    }
    VISwing *swing = GetSwing(m_player.m_class, m_player.m_level, b14, b15, b16, b18, b17, b19, b20);
    unsigned int effect = swing->Effect();
    if (effect == 0)
        noEffect = 1;
    else
        delay = swing->Delay();
    if (TargetInRange(slot) && !noEffect)
    {
        int idx;
        if (m_dict.FindTyped(effect, (VIResourceType)21, idx))
        {
            VIEntity *self = FindEntity(m_selfId);
            VIEntity *t = FindEntity(m_target);
            if (self && t && t->m_actor != -1)
            {
                VIScene &scene = m_scene;
                scene.m_spellEffects.Play(idx, 0, self->m_actor, t->m_actor, 0);
                m_p071F0 = m_p07568;
            }
        }
    }
    else
    {
        if (m_inventory.Item(slot)->m_equipSlot != 18 && !noEffect)
        {
            VIEntity *self = FindEntity(m_selfId);
            if (self && m_scene.GetActorSpriteType(self->m_actor) == 5)
                ((VICSprite *)m_scene.Sprite(m_scene.GetActorSprite(self->m_actor)))->SetAction((VICSpriteAnimID)anim, m_scene);
        }
        SendOpcode(216);
        delay = 2000;
    }
    m_p071E8 += delay;
}

// 0x00BC5B80, name provisional, no caller in the image: point i of a ping graph, three
// pixels a sample, the newest last, `scale` pixels a millisecond up from 448.
void PingPoint(int i, VIPingRing *r, VIVect2I *p, float scale)
{
    int k = (r->m_head + i) % 64;
    int v = r->m_samples[k];
    p->x = i * 3;
    p->y = 448 - (int)(scale * v);
}

// 0x00BC5AC8, name provisional, no caller in the image: a UI image as a bar 16 high, h*54+5
// wide.
void VIClient::DrawPingBar(const char *image, const VIVect2I &at, float h)
{
    VIVect2I a;
    VIVect2I s(64, 16);
    s.x = (int)(h * 54.0f + 5.0f);
    m_ui.Blit(UIImageId(image), at, s, a, (VIAlphaBlendMode)0, 1.0f, 0);
}

// 0x00BCA840, name provisional: a full stack of `item` in `list` -- its index into *slot.
int VIClient::FindFullStack(VIItemList &list, VINetItem *item, int *slot)
{
    for (int i = 0; i < list.Count(); i++)
    {
        VINetItem *it = list.Item(i);
        if (item->m_id == it->m_id && item->m_maxStack == it->Quantity && item->m_maxStack > 1)
        {
            *slot = it->m_invIndex;
            return 1;
        }
    }
    return 0;
}

// 0x00BDD2B8: where to find a shard -- the override's host and port (10070 unless it names
// one), else a random entry of the shard list; -1 with no list.
int VIClient::GetShardServer(char *host, unsigned short *port)
{
    m_random.Seed(m_p07568);
    if (m_shardOverride[0])
    {
        char buf[256];
        int p = 10070;
        strcpy(buf, m_shardOverride);
        char *c = strchr(buf, ':');
        if (c)
        {
            *c = 0;
            sscanf(c + 1, "%d", &p);
        }
        strcpy(host, buf);
        *port = p;
    }
    else
    {
        if (m_p4F0B0 == 0)
            return -1;
        int k = m_random.Range(0, m_p4F0B4);
        strcpy(host, m_p4F0B0[k].m_name);
        *port = m_p4F0B0[k].m_p20;
    }
    return 0;
}

// 0x00BC5F78, name provisional (Msg005E): stop any camera effect, put the walk camera
// back on (a fixed camera first becomes style 2), and keep where it is looking from --
// the position, the direction, the player's level distance from it and its yaw.
void VIClient::ResetCamera()
{
    VIWalkCamera &cam = m_walkCam;
    m_pC1B0 = 1;
    cam.m_effect.Reset();
    cam.m_effect.EndManual(0.0f);
    if (cam.m_cameraMode == 0)
    {
        SetCameraStyle(2);
        cam.Process(m_camera, m_scene, 1);
    }
    m_camPos = m_camera.m_position;
    m_camDir = m_camera.m_dir;
    VIEntity *e = FindEntity(m_selfId);
    if (e == 0 || e->m_actor < 0)
        return;
    VIVect3 d = m_scene.GetActorLocation(e->m_actor);
    d -= m_camPos;
    d.y = 0.0f;
    m_camDist = d.Magnitude();
    m_pC1D0 = 0.0f;
    m_camYaw = -(float)atan2(m_camDir.z, m_camDir.x);
}

// 0x00BC72C0, name provisional: every creature but the player, once a frame -- its own
// update, its walk while +0x2BD48 is set, and its character sprite: the death animation
// once in state 14, else the walk's or the state's locomotion, then processed where the
// actor stands.
void VIClient::UpdateCreatures()
{
    for (int i = 0; i < 24; i++)
    {
        VIEntity *e = m_entities[i];
        if (e == 0)
            continue;
        e->Virtual9();
        if (!e->Virtual3(130))
            continue;
        if (e->m_id == m_selfId)
            continue;
        VICreatureEntity *c = (VICreatureEntity *)e;
        if (m_p2BD48)
            c->Walk();
        if (e->m_actor == -1)
            continue;
        if (m_scene.GetActorSpriteType(e->m_actor) != 5)
            continue;
        VICSprite *s = (VICSprite *)m_scene.Sprite(m_scene.GetActorSprite(e->m_actor));
        if (c->m_state == 14)
        {
            if (!s->m_dead)
            {
                s->SetDeath(1, c->m_p2A8, m_scene);
                {
                    VIScene &scene = m_scene;
                    scene.m_spellEffects.StopIfThisWasCaster(e->m_actor);
                }
                c->m_deathTime = m_scene.GetTime();
            }
        }
        else if (c->m_p255)
            c->m_walk.UpdateSprite(*s, m_scene);
        else
            s->SetLocomotion((VICSpriteAnimID)c->m_state, m_scene);
        s->Process(m_camera, m_scene.GetActorLocation(e->m_actor), m_scene);
        c->m_p2A8 = 0;
    }
}

// 0x00BCF0F8: 0x00B4, an ability into a slot -- the one at display index `index` turns
// over and takes `slot`; any other in that slot goes off and gives it up.  Word 824 (on)
// or 825 (off) with its name, and sound 6.
void VIClient::Msg00B4(int index, int slot)
{
    int found;
    int i;
    for (i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
        if (m_abilities.Value(i).m_displayIndex == index)
        {
            found = i;
            break;
        }
    if (i == m_abilities.End())
        return;
    VIUI &ui = m_ui;
    for (i = m_abilities.Begin(); i != m_abilities.End(); i = m_abilities.Next(i))
        if (m_abilities.Value(i).m_slot == slot && m_abilities.Value(i).m_displayIndex != index)
        {
            m_abilities.Value(i).m_valid = 0;
            m_abilities.Value(i).m_slot = -1;
        }
    m_abilities.Value(found).m_valid = !m_abilities.Value(found).m_valid;
    m_abilities.Value(found).m_slot = slot;
    unsigned short text[256];
    VIString::snprintf16(text, sizeof(text),
                         (const unsigned short *)ui.m_words.GetString(m_abilities.Value(found).m_valid ? 824 : 825, ui.m_language),
                         m_abilities.Value(found).m_name);
    m_wndGame->AddText(text);
    PlaySound(6);
}

// 0x00BBFE80: client_main's one call.  A changed disc stops here for good, with "ERROR" on
// screen when it is a different disc.  The journal opens -- recording to the first free
// dem\clientNNN.dem when `record` is set -- and takes the start-up record: the time,
// three strings, 32 bytes and three flags.  Then the rest of the start-up and the shard
// list's region.
int VIClient::Init(const char *cmd, char *a, const char *c, char *b, int record, int f6, int f7, int f8)
{
    switch (VIHasDiscIdChanged())
    {
    case -1:
        for (;;)
            ;
    case 1:
        VIDisplayScreen16("ERROR", 4, true);
        for (;;)
            ;
    }
    char empty[32];
    memset(empty, 0, 4);
    if (!b)
        b = empty;
    if (!c)
        c = "";
    char path[64];
    int mode;
    char *p;
    if (record)
    {
        VIJournal::FreeName(path, "dem\\client%03u.dem");
        mode = 1;
        p = path;
    }
    else
    {
        mode = 0;
        p = 0;
    }
    if (m_journal.Open(mode, p, 2048) < 0)
        return -1;
    long t = VITimer::GetTime();
    m_journal.Write(&t, 8, 1);
    m_journal.WriteString(cmd);
    m_journal.WriteString(a);
    m_journal.WriteString(c);
    m_journal.Write(b, 1, 32);
    m_journal.Write((char)f6);
    m_journal.Write((char)f7);
    m_journal.Write((char)f8);
    int r = InitBody();
    if (r < 0)
        return r;
    m_shardInfo.SetRegion((char)VIGetGameOptions()->m_region);
    return 0;
}

// 0x00BC1498, name provisional (Msg0777Done, the destructor): drop the world and come back
// up empty -- the world's resources, the loader, the scene, the dictionary, the sound, the
// collision and the raster cleared, then the dictionary and the scene made new, the spell
// effects' rumble rewired and the camera effect stopped.  The first failure is returned.
int VIClient::UnloadWorld()
{
    int r;
    m_worldLoaded = 0;
    ReleaseTargetEffect(m_scene);
    m_p4FD28.Release();
    m_fadeMat = -1;
    m_battleMusic.Free();
    if ((r = m_p06D50.Clear()) != 0)
        goto done;
    if ((r = m_p06D00.Clear()) != 0)
        goto done;
    if ((r = m_loader.Clear()) != 0)
        goto done;
    m_wndGame->WorldUnloaded();
    if ((r = m_scene.Clear()) != 0)
        goto done;
    if ((r = m_dict.Clear()) != 0)
        goto done;
    if ((r = m_sound.Reset()) != 0)
        goto done;
    m_sound.SetSoundClassVolume((VISOUNDCLASS)0, VIGetGameOptions()->m_musicVolume);
    m_sound.SetSoundClassVolume((VISOUNDCLASS)1, VIGetGameOptions()->m_effectVolume);
    if ((r = m_collide.Reset()) != 0)
        goto done;
    if ((r = m_raster.Reset()) != 0)
        goto done;
    if ((r = m_dict.Init(0)) != 0)
        goto done;
    if ((r = m_scene.Init(m_p07568, m_dict, &m_raster, &m_sound, m_collide)) != 0)
        goto done;
    {
        VIScene &scene = m_scene;
        scene.m_spellEffects.SetVibrateCallback((VIVibrateFn)Rumble, &m_console);
    }
    m_p2BD50 = 0;
    m_p1B300 = 0xFFFFFFFF;
    m_p1B304 = 0;
    m_p00080 = 0;
    m_pC1B0 = 0;
    m_walkCam.m_effect.Reset();
    m_walkCam.m_effect.EndManual(0.0f);
done:
    return r;
}

// 0x00BC6118, name provisional: the camera while the player lies dead (ResetCamera keeps
// where it starts).  It backs away from the body along the kept bearing, rising and
// swinging round a little more each frame, and for four seconds turns from the kept
// direction down onto the body -- easing in and out, the level part of the direction
// rescaled to keep it a unit vector; at four seconds the screen fades.
void VIClient::DeathCamera()
{
    if (!m_pC1B0)
        return;
    VIEntity *me = FindEntity(m_selfId);
    VIScene &scene = m_scene;
    float dt = m_scene.m_particleSystem.DeltaTime();
    if (dt > 0.1f)
        dt = 1.0f / 30.0f;
    if (me == 0 || me->m_actor < 0)
        return;
    VIVect3 pos = scene.GetActorLocation(me->m_actor);
    VIVect3 eye;
    eye.x = pos.x - cosf(m_camYaw) * (m_camDist + m_pC1D0 * 0.15f);
    eye.y = m_camPos.y;
    eye.z = pos.z + sinf(m_camYaw) * (m_camDist + m_pC1D0 * 0.15f);
    m_camPos.y += VIMIN(m_pC1D0 * 0.25f, 1.0f) * 0.019f;
    m_camYaw += VIMIN(m_pC1D0 * 0.25f, 1.0f) * 0.014f;
    VIVect3 dir;
    if (m_pC1D0 < 4.0f)
    {
        float k = cosf(m_pC1D0 * 0.25f * 3.1415927f + 3.1415927f) * 0.5f + 0.5f;
        dir = pos;
        dir -= eye;
        dir.Normalize();
        dir.y = m_camDir.y + (dir.y - m_camDir.y) * k;
        float x2 = dir.x * dir.x;
        float z2 = dir.z * dir.z;
        float s = (1.0f - dir.y * dir.y) / (x2 + z2);
        x2 *= s;
        z2 *= s;
        float sx = sqrtf(x2);
        dir.x = dir.x < 0.0f ? -sx : sx;
        float sz = sqrtf(z2);
        dir.z = dir.z < 0.0f ? -sz : sz;
    }
    else
    {
        dir = pos;
        dir -= eye;
        dir.Normalize();
    }
    m_camera(eye, dir);
    float t = m_pC1D0;
    m_pC1D0 = t + dt;
    if (t <= 4.0f && m_pC1D0 > 4.0f)
        SetFade(1.0f, 4.0f);
}

static inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline int operator==(const VIVect3 &a, const VIVect3 &b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

// 0x00BD41B8: a creature's state channel (0x00..0x17 is its slot).  Under five bytes the
// slot empties.  Otherwise a kind byte, the id and the type: another entity in the slot
// goes (and with kind 0 nothing replaces it), the right one is made when missing, and it
// reads the rest.  The extrapolation gets its velocity and turn -- none for a new entity,
// one not in the scene, or a creature that has not moved -- and a time stamp; the
// player's own entity goes back to where the walk camera has it.
void VIClient::ChannelEntity(int ch, const void *data, int size)
{
    VIMemStream s(data, size);
    if (m_selfId == 0xFFFFFFFF)
        return;
    m_p2BB58 = 0x0123456789ABCDEFL;
    int created = 0;
    VIVect3 old;
    if ((unsigned int)size < 5)
    {
        delete m_entities[ch];
        m_entities[ch] = 0;
    }
    else
    {
        unsigned char kind, type;
        unsigned int id;
        s.Read(&kind, 1, 1);
        s.Read(&id, 4, 1);
        s.Read(&type, 1, 1);
        if (m_entities[ch])
        {
            if (m_entities[ch]->Virtual2() != type || m_entities[ch]->m_id != id || !kind)
            {
                delete m_entities[ch];
                m_entities[ch] = 0;
                if (!kind)
                    return;
            }
        }
        if (!m_entities[ch])
        {
            m_entities[ch] = CreateEntity(type, id, m_scene, m_p06D00);
            created = 1;
        }
        VIEntity *e = m_entities[ch];
        e->m_id = id;
        if (e->Virtual3(130))
            old = ((VICreatureEntity *)e)->m_p218;
        e->Virtual5(s);
        if (created || e->m_actor == -1)
        {
            m_entVel[ch].Set(0.0f, 0.0f, 0.0f);
            m_entRot[ch].Set(0.0f, 0.0f, 0.0f);
        }
        else
        {
            m_entVel[ch] = e->GetActorPos() - *e->GetPos();
            m_entRot[ch] = e->GetActorHpr() - *e->GetHpr();
            WrapAngles(m_entRot[ch]);
        }
        m_entStamp[ch] = m_p07568 + gEntityUpdateRate * 2;
        if (e->Virtual3(130))
        {
            if (((VICreatureEntity *)e)->m_p218 == old)
            {
                m_entVel[ch].Set(0.0f, 0.0f, 0.0f);
            }
        }
        if (e->m_id == m_selfId)
        {
            e->SetPos(m_walkCam.m_committed.m_position);
            e->SetHpr(m_walkCam.m_committed.m_angles);
        }
    }
}

// 0x00BBF0D8: VILoader's progress callback.  At most every 30 ms the client ticks and the
// loading screen draws itself -- the frame out of "sheet1" and the bar, one layout for the
// original game and one for Frontiers (expansion flag 2), forty pixels lower -- then the
// loading sprite, mirrored, in a 642x450 window at (500, 350).
void VIClient::LoadingProgress(void *client)
{
    VIClient *c = (VIClient *)client;
    VIScene &scene = c->m_scene;
    VIUI &ui = c->m_ui;
    static long last;
    VIRaster *r = scene.m_raster;
    c->TickLocal();
    long now = c->m_p07568;
    if ((int)(now - last) < 30)
        return;
    c->TickNet();
    last = now;
    r->BeginScene();
    ui.Begin2D();
    if (!(VIGetExpansionFlags() & 2))
    {
        ui.Blit(UIImageId("sheet1"), VIVect2I(82, 120), VIVect2I(475, 107), VIVect2I(37, 0), (VIAlphaBlendMode)0, 1.0f, 0);
        ui.Blit(UIImageId("sheet1"), VIVect2I(177, 237), VIVect2I(288, 33), VIVect2I(31, 113), (VIAlphaBlendMode)0, 1.0f, 0);
        ui.BlitEx((VIUITexId)33, VIVect2I(190, 285), (VIAlphaBlendMode)0, 1.0f, 0);
        ui.BlitHorz(VIVect2I(206, 285), (VIUITexId)34, 228, (VIAlphaBlendMode)0, 1.0f, 0);
        ui.BlitEx((VIUITexId)33, VIVect2I(434, 285), (VIAlphaBlendMode)0, 1.0f, 1);
        ui.BlitEx((VIUITexId)203, VIVect2I(228, 292), (VIAlphaBlendMode)0, 1.0f, 0);
    }
    else
    {
        ui.Blit(UIImageId("sheet1"), VIVect2I(82, 80), VIVect2I(475, 107), VIVect2I(37, 0), (VIAlphaBlendMode)0, 1.0f, 0);
        ui.Blit(UIImageId("sheet1"), VIVect2I(177, 197), VIVect2I(288, 33), VIVect2I(31, 113), (VIAlphaBlendMode)0, 1.0f, 0);
        ui.BlitEx((VIUITexId)243, VIVect2I(143, 247), (VIAlphaBlendMode)0, 1.0f, 0);
        ui.BlitEx((VIUITexId)33, VIVect2I(190, 325), (VIAlphaBlendMode)0, 1.0f, 0);
        ui.BlitHorz(VIVect2I(206, 325), (VIUITexId)34, 228, (VIAlphaBlendMode)0, 1.0f, 0);
        ui.BlitEx((VIUITexId)33, VIVect2I(434, 325), (VIAlphaBlendMode)0, 1.0f, 1);
        ui.BlitEx((VIUITexId)203, VIVect2I(228, 332), (VIAlphaBlendMode)0, 1.0f, 0);
    }
    ui.End2D();
    VIMatrix44 m;
    m.Identity();
    m.m[0].x = -m.m[0].x;
    m.m[2].z = -m.m[2].z;
    m.m[3].z = 0.7f;
    scene.SetTime(now);
    r->SetModelView(m);
    VICamera &cam = c->m_camera;
    r->SetProjection(cam.m_proj);
    r->Viewport(500, 350, 642, 450);
    scene.Sprite(c->m_loadSprite)->Raster(cam, scene, *r, 3);
    r->Viewport(0, 0, 639, 447);
    r->EndScene();
}

// 0x00BC0F28, name provisional (Msg000D): the global resources and a fresh session.  A
// changed disc stops here for good.  The camera; ui.csf (with the selection rings and the
// loading gears it must hold), the fonts, spell effects, effects, lightning, sky,
// character customisation, debug art and the extra art (off the memory card when it is
// there); the battle music; nine o'clock; and every session field back to its start.
// Prints the time it took.
int VIClient::EnterGameLoad()
{
    VIESFParse parse;
    VIArray<VIElem_BDF7A0> arr;
    VICSFFile csf;
    m_worldLoaded = 0;
    switch (VIHasDiscIdChanged())
    {
    case -1:
        for (;;)
            ;
    case 1:
        VIDisplayScreen16("ERROR", 4, true);
        for (;;)
            ;
    }
    VIVect3 pos;
    VIVect3 dir(0.0f, 0.0f, 1.0f);
    if (m_camera.Init(m_raster, pos, dir, 55.0f, 10.0f / 7.0f, 0.1f, 900.0f) < 0)
        return -1;
    long t = -VITimer::GetTime();
    VIScene &scene = m_scene;
    int r;
    char buf[16384];
    VIResourceType type;
    int n;
    if ((r = csf.Parse("data2\\ui.csf", parse, scene)) != 0)
        goto done;
    m_pC144 = -1;
    if (!scene.m_dict->Find(VIHashResourceID("Selection_Rings"), type, m_ringsIndex)
        || !scene.m_dict->Find(VIHashResourceID("Loading_Gears"), type, m_loadSprite))
    {
        r = -1;
        goto done;
    }
    parse.SetProgressCallback(LoadingProgress, this);
    csf.SetProgressCallback(LoadingProgress, this);
    if ((r = csf.Parse("data2\\Arial.csf", parse, scene)) != 0)
        goto done;
    if ((r = csf.Parse("data2\\ArialUI.csf", parse, scene)) != 0)
        goto done;
    if ((r = csf.Parse("data2\\spellFX.csf", parse, scene)) != 0)
        goto done;
    if ((r = csf.Parse("data2\\fx.csf", parse, scene)) != 0)
        goto done;
    if ((r = VILightningFxInit(*scene.m_raster, *scene.m_dict)) != 0)
        goto done;
    if ((r = csf.Parse("data2\\sky.csf", parse, scene)) != 0)
        goto done;
    if ((r = scene.SetSky()) != 0)
        goto done;
    if ((r = csf.Parse("data2\\charcust.csf", parse, scene)) != 0)
        goto done;
    VICSpriteCust::SetResources(m_raster, m_dict);
    if ((r = csf.Parse("data2\\debug.csf", parse, scene)) != 0)
        goto done;
    n = VIMemCardReadFile("addart.csf", buf, sizeof(buf));
    if (n >= 0)
        csf.Parse(buf, n, parse, scene);
    else
        csf.Parse("data2\\addart.csf", parse, scene);
    m_battleMusic.Init(&scene);
    if (!(VIGetFeatureFlags() & 0x100))
        m_battleMusic.Preload(VIGetGameOptions()->m_language);
    t += VITimer::GetTime();
    printf("EnterGame Load: %d ms\n", (int)t);
    m_p00064 = 1;
    m_timeOfDay = 9.0f;
    scene.SetTimeOfDay(9.0f);
    m_p00068 = 0;
    scene.SetPersonalLightMode((VIPersonalLightMode)0);
    m_p06DC0 = 1;
    m_p06DC4 = 1;
    m_p06DC8 = 0;
    m_p00074 = 0;
    m_p00078 = 0;
    {
        long now = VITimer::GetTime();
        m_dialogHead = 0;
        m_dialogTail = 0;
        m_p2EBFC = 0;
        m_p2BB60 = 0;
        m_targetInfoValid = 0;
        m_p2BB68 = 0;
        m_targetDisposition = 0;
        m_p2BB74 = 1;
        m_p2C598 = now;
        m_p2BB70 = 2;
    }
    TradeReset();
    m_p3834C.ResetP78C0();
    m_p4EC58.ResetP41C();
    m_p2BB10 = 0xFFFFFFFF;
    m_p2EF24.Reset();
    m_p2BB48 = 0;
    m_p2BB40 = 0;
    m_p00080 = 0;
    m_p4F0B8[0] = 0;
    if ((r = m_p4FD28.Init(scene, m_entities, 24)) < 0)
        goto done;
    LoadTargetEffect(scene);
    m_reticleTex = -1;
    m_p50308 = 0;
    m_worldLoaded = 1;
done:
    return r;
}

// 0x00BC8B98, name provisional: the next target.  With `group` set the candidates are the
// player, the group and the pet, in that order; otherwise every visible entity that is
// not one of them, scored by how far it sits from the middle of the screen and how far
// away it is (a corpse scores two worse), and sorted.  A new mode starts again at the top
// (the group mode at its last entry); otherwise the target moves one past the current one.
static inline float DistSq(const VIVect3 &a, const VIVect3 &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

static inline VIVect3 Flat(const VIVect3 &v)
{
    return VIVect3(v.x, 0.0f, v.z);
}

void VIClient::CycleTarget(int group)
{
    static int lastGroup;
    const VIMatrix44 &vp = m_camera.m_viewProj;
    VIVect4 v[2];
    struct
    {
        unsigned int key;
        float        score;
    } cand[24];
    int n = 0;
    if (group)
    {
        if (GetP2BE4C() == 0)
        {
            n = 1;
            cand[0].score = 0.0f;
            cand[0].key = m_selfId;
        }
        else
        {
            n = 1;
            cand[0].score = 0.0f;
            cand[0].key = m_selfId;
            for (int i = 0; i < GetP2BE4C(); i++)
                if (GetGroupMemberId(i) != m_selfId)
                {
                    cand[n].score = 0.0f;
                    cand[n].key = GetGroupMemberId(i);
                    n++;
                }
        }
        if (m_petId != 0xFFFFFFFF)
        {
            cand[n].score = 0.0f;
            cand[n].key = m_petId;
            n++;
        }
    }
    else
    {
        VIMatrix44 m;
        for (int i = 0; i < 24; i++)
        {
            VIVect3 pos;
            VIEntity *e = m_entities[i];
            if (e == 0 || e->m_actor == -1)
                continue;
            int dead = 0;
            pos = e->GetActorPos();
            int inGroup = 0;
            int isPet = e->m_id == m_petId;
            int isSelf = e->m_id == m_selfId;
            for (int j = 0; j < GetP2BE4C(); j++)
                if (e->m_id == GetGroupMemberId(j))
                {
                    inGroup = 1;
                    break;
                }
            if (inGroup || isSelf || isPet)
                continue;
            if (e->Virtual3(130))
            {
                if (((VICreatureEntity *)e)->m_p256 == 3)
                    continue;
                dead = ((VICreatureEntity *)e)->m_state == 14;
            }
            int near = 0;
            if (DistSq(pos, m_camera.m_position) < 15.0f)
            {
                VIVect3 a(pos.x - m_camera.m_position.x, 0.0f, pos.z - m_camera.m_position.z);
                VIVect3 b = Flat(m_camera.m_dir);
                a.Normalize();
                b.Normalize();
                if (a.x * b.x + a.y * b.y + a.z * b.z > 0.65f)
                    near = 1;
            }
            if (!near && m_camera.m_frustum.Cull(pos, 0.0f))
                continue;
            if (!(e->m_p18 & 1))
                continue;
            VIVect3 t[2];
            VIVect4 h;
            t[0] = pos;
            h = VIVect4(pos.x, pos.y, pos.z, 1.0f) * vp;
            float d2 = DistSq(pos, m_camera.m_position);
            v[0] = h;
            float inv = 1.0f / h.w;
            t[0].x = h.x * inv;
            t[0].y = h.y * inv;
            t[0].z = 0.0f;
            if (d2 < 40000.0f)
            {
                float s;
                if (near)
                    s = t[0].z;
                else if (d2 < 25.0f)
                    s = sqrtf(d2) / 50.0f * 0.35f;
                else
                    s = sqrtf(d2) / 50.0f * 0.35f + t[0].x * t[0].x;
                cand[n].score = s + (dead ? 2.0f : 0.0f);
                cand[n].key = (unsigned int)e;
                n++;
            }
        }
    }
    int k;
    if (!group)
        qsort(cand, n, sizeof(cand[0]), CompareY);
    else if (lastGroup != group)
    {
        k = n - 1;
        goto pick;
    }
    if (lastGroup != group)
        k = n;
    else
        for (k = 0; k < n; k++)
            if (group ? cand[k].key == m_target : ((VIEntity *)cand[k].key)->m_id == m_target)
                break;
pick:
    m_autoAttack = 0;
    if (n == 0)
        m_target = 0xFFFFFFFF;
    else if (k == n)
        m_target = group ? cand[0].key : ((VIEntity *)cand[0].key)->m_id;
    else
    {
        k++;
        if (k < 0)
            k += n;
        if (k >= n)
            k -= n;
        m_target = group ? cand[k].key : ((VIEntity *)cand[k].key)->m_id;
    }
    m_targetInfoValid = 0;
    SendOpIntInt(16, m_p2BB68, m_target);
    lastGroup = group;
}
