#line 1 "C:/code-frontiers-gm/game/VIWndCharacterSelect.cpp"
// viwndcharacterselect.cpp -- CLIENT 0x00C1FBB8 .. 0x00C22C18: the character select
// screen.  Eight slots and a delete slot; the highlighted character stands in the
// scene as a preview (ShowCharacter, from VIClient's character list); Enter plays it,
// offers play/customise first with Frontiers, deletes it after the delete slot, or
// creates one in an empty slot.
#include "game.h"

// One WORDS.TXT entry centred at (x, y).
static inline void DrawCentered(VIUI *ui, VIFont *font, int id, int x, int y)
{
    font->DrawCenteredString(ui->m_raster, ui->m_dict,
                             (const unsigned short *)ui->m_words.GetString(id, ui->m_language), x, y, 1);
}

// 0x00C1FBB8.
VIWndCharacterSelect::VIWndCharacterSelect()
{
    m_showTime = -1;
    m_text[0] = 0;
    m_deleting = 0;
    m_sel = 0;
    m_mbox = 0;
    m_raster = 0;
    m_p44C = 0;
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_dict = 0;
    m_collide = 0;
    m_cust = 0;
    m_actor = -1;
    m_attackSlot = -1;
    m_busy = 0;
    m_p5EC = 0;
    m_choosing = 0;
    m_choice = 0;
    m_lastRace = -1;
}

// 0x00C1FC50.
VIWndCharacterSelect::~VIWndCharacterSelect()
{
    Free();
}

// 0x00C1FCA8.
int VIWndCharacterSelect::Init(VIScene &scene, VIDictionary &dict, VIRaster &raster,
                               VICamera &camera, VIWalkCamera &walkCam, VICollide &collide)
{
    int r;
    m_camera = &camera;
    m_walkCam = &walkCam;
    m_scene = &scene;
    m_raster = &raster;
    m_dict = &dict;
    m_collide = &collide;
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        goto fail;
    m_mbox->SetVisible(false);
    m_mbox->m_pos = VIVect2I(80, 115);
    m_mbox->SetAppPtr(GetAppPtr());
    if ((r = m_mbox->Init(1, 1)) != 0)
        goto done;
    AddChild(m_mbox);
    m_cust = new VIWndCharCustFrontiers;
    if (!m_cust)
    {
fail:
        r = -1;
    }
    else
    {
        m_cust->SetVisible(false);
        m_cust->SetAppPtr(GetAppPtr());
        if ((r = m_cust->Init(scene, camera, walkCam)) != 0)
            goto done;
        AddChild(m_cust);
        m_showTime = -1;
        m_text[0] = 0;
        m_deleting = 0;
        m_sel = 0;
        m_attackSlot = -1;
        m_busy = 0;
        m_p5EC = 0;
        m_choice = 0;
        m_lastRace = -1;
        m_choosing = 0;
    }
done:
    return r;
}

// 0x00C1FE20.
int VIWndCharacterSelect::Free()
{
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    if (m_cust)
    {
        RemoveChild(m_cust);
        delete m_cust;
        m_cust = 0;
    }
    m_showTime = -1;
    m_scene = 0;
    m_dict = 0;
    m_raster = 0;
    m_p44C = 0;
    m_camera = 0;
    m_walkCam = 0;
    m_collide = 0;
    m_actor = -1;
    m_attackSlot = -1;
    m_text[0] = 0;
    m_deleting = 0;
    m_sel = 0;
    m_busy = 0;
    m_p5EC = 0;
    m_choice = 0;
    m_lastRace = -1;
    m_choosing = 0;
    return 0;
}

// 0x00C1FF08.
int VIWndCharacterSelect::Connecting()
{
    m_showTime = -1;
    m_attackSlot = -1;
    m_lastRace = -1;
    m_text[0] = 0;
    m_deleting = 0;
    m_sel = 0;
    m_busy = 0;
    m_p5EC = 0;
    m_choice = 0;
    m_choosing = 0;
    if (m_actor != -1 && m_scene)
    {
        m_scene->ReleaseActor(m_actor);
        m_actor = -1;
    }
    return 0;
}

// The "..." after a pending line: one more dot every 15 frames, three at most.
static char sDots;    // 0x00CD7524
static char sTicks;   // 0x00CD7525

// 0x00C1FF88.  The title bar, the slots (the characters, the free slots, the delete
// slot), the button hints, the preview in its viewport, the slot's race and class,
// and the play/customise box or the delete box.
void VIWndCharacterSelect::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    unsigned short text[128];
    char d;
    VIUI *ui = &app->m_ui;
    if (--sTicks <= 0)
    {
        sTicks = 15;
        if (++sDots == 3)
            sDots = 0;
    }
    ui->Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    float alpha = 1.0f;
    VIColor32F black(0.0f, 0.0f, 0.0f, alpha);
    VIColor32F grey(0.8f, 0.8f, 0.8f, 1.0f);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, 1.0f);
    int slot = 0;
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 450), (VIAlphaBlendMode)0, 1.0f);
    VIFont *font = &ui->m_font;
    int title = m_deleting ? 262 : 263;
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict,
                                 (const unsigned short *)ui->m_words.GetString(title, ui->m_language)) + 64;
    ui->BlitBar((unsigned short)title, white, VIVect2I((640 - w) / 2, 8), (VIUITexId)33, (VIUITexId)34, w,
                (VIAlphaBlendMode)0, 1.0f, 0, 1);
    ui->BlitRect((VIUITexId)51, VIVect2I(-1, 48), VIVect2I(348, 355), (VIAlphaBlendMode)0, 1.0f);
    ui->BlitStuddedHorz(VIVect2I(-1, 37), (VIUITexId)37, (VIUITexId)38, 348, 3, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(-1, 396), (VIUITexId)40, (VIUITexId)41, 348, 3, (VIAlphaBlendMode)0, 1.0f, 0);
    VIUIFrameParam frame;
    memset(&frame, 0, sizeof(frame));
    VIUIFrameParam studs;
    memset(&studs, 0, sizeof(studs));
    frame.m_left.m_id = (VIUITexId)42;
    frame.m_right.m_id = (VIUITexId)42;
    frame.m_bl.m_id = (VIUITexId)39;
    frame.m_br.m_flags = 1;
    frame.m_bottom.m_id = (VIUITexId)40;
    frame.m_right.m_flags = 1;
    frame.m_br.m_id = (VIUITexId)39;
    frame.m_top.m_id = (VIUITexId)37;
    frame.m_tl.m_id = (VIUITexId)36;
    frame.m_tr.m_flags = 1;
    studs.m_left.m_id = (VIUITexId)43;
    frame.m_tr.m_id = (VIUITexId)36;
    studs.m_right.m_id = (VIUITexId)43;
    studs.m_right.m_flags = 1;
    studs.m_bottom.m_id = (VIUITexId)41;
    studs.m_top.m_id = (VIUITexId)38;
    ui->BlitStuddedFrame(VIVect2I(347, 37), VIVect2I(284, 377), frame, studs, 2, 3, (VIAlphaBlendMode)0, 1.0f, 1);
    VIVect2I pen(15, 60);
    VIColor32F bar;
    m_p5EC = 0;
    for (; slot < app->GetCharCount(); slot++)
    {
        VIUITexId l, m, r;
        if (slot == m_sel)
        {
            l = (VIUITexId)56;
            m = (VIUITexId)58;
            bar = yellow;
            r = (VIUITexId)57;
        }
        else
        {
            l = (VIUITexId)53;
            bar = white;
            m = (VIUITexId)55;
            r = (VIUITexId)54;
        }
        VICharRecord *c = app->GetChar(slot);
        if (c->m_serverId == -1)
        {
            m_p5EC = 1;
            VIString::strcpy16(text, sizeof(text),
                               (const unsigned short *)ui->m_words.GetString(878, ui->m_language));
            for (d = 0; d < sDots + 1; d++)
                VIString::strcat16(text, sizeof(text), ".");
            ui->BlitBar(text, bar, pen, l, m, r, 320, (VIAlphaBlendMode)0, 1.0f, 1, 1);
        }
        else
            ui->BlitBar(c->m_name, bar, pen, l, m, r, 320, (VIAlphaBlendMode)0, 1.0f, 1, 1);
        pen.y += 30;
    }
    if (!m_deleting)
    {
        for (; slot < 8; slot++)
        {
            VIUITexId l, m, r;
            if (slot == m_sel)
            {
                l = (VIUITexId)56;
                m = (VIUITexId)58;
                bar = yellow;
                r = (VIUITexId)57;
            }
            else
            {
                l = (VIUITexId)53;
                bar = grey;
                m = (VIUITexId)55;
                r = (VIUITexId)54;
            }
            ui->BlitBar((unsigned short)219, bar, pen, l, m, r, 320, (VIAlphaBlendMode)0, 1.0f, 1, 1);
            pen.y += 30;
        }
        VIUITexId l, m, r;
        int hi;
        if (m_sel == 8)
        {
            l = (VIUITexId)56;
            m = (VIUITexId)58;
            bar = yellow;
            r = (VIUITexId)57;
            hi = 1;
        }
        else
        {
            bar = black;
            l = (VIUITexId)53;
            m = (VIUITexId)55;
            r = (VIUITexId)54;
            hi = 0;
        }
        ui->BlitBar((unsigned short)220, bar, pen, l, m, r, 320, (VIAlphaBlendMode)0, 1.0f, 1, hi);
    }
    pen.y += 35;
    memset(&frame, 0, sizeof(frame));
    frame.m_bottom.m_id = (VIUITexId)49;
    frame.m_left.m_id = (VIUITexId)47;
    frame.m_bl.m_id = (VIUITexId)48;
    frame.m_br.m_id = (VIUITexId)50;
    frame.m_right.m_id = (VIUITexId)52;
    frame.m_top.m_id = (VIUITexId)45;
    frame.m_tl.m_id = (VIUITexId)44;
    frame.m_tr.m_id = (VIUITexId)46;
    ui->BlitFillFrame(VIVect2I(15, 358), VIVect2I(320, 35), frame, (VIUITexId)35, (VIAlphaBlendMode)0, 1.0f, 1);
    font->SetTextColor(black);
    const VIUITexEntry *tex = GetUITexture((VIUITexId)65);
    VIVect2I p(45, 368);
    VIVect2I at(70, tex->m_uv1.y / 2 + 365);
    ui->BlitEx((VIUITexId)66, p, (VIAlphaBlendMode)0, 1.0f, 0);
    DrawCentered(ui, font, 212, at.x, at.y);
    p.x += font->GetStringWidth(ui->m_raster, ui->m_dict,
                                (const unsigned short *)ui->m_words.GetString(212, ui->m_language)) + tex->m_uv1.x + 30;
    at.x += font->GetStringWidth(ui->m_raster, ui->m_dict,
                                 (const unsigned short *)ui->m_words.GetString(212, ui->m_language)) + tex->m_uv1.x + 30;
    ui->BlitEx((VIUITexId)65, p, (VIAlphaBlendMode)0, 1.0f, 0);
    DrawCentered(ui, font, 161, at.x, at.y);
    font->SetTextColor(white);
    ui->End2D();
    VIWnd *active = app->m_p2BD40;
    if (m_actor == -1 && m_sel < app->GetCharCount() && active != m_cust)
    {
        ShowCharacter(GetSel()->m_modelId, GetSel()->m_race, GetSel()->m_class, GetSel()->m_hairColor, GetSel()->m_hairLength,
                      GetSel()->m_hairStyle, GetSel()->m_face, GetSel()->m_robe, GetSel()->m_p40,
                      GetSel()->m_level, GetSel()->m_weapons, GetSel()->m_p50, GetSel()->m_armor,
                      GetSel()->m_p59);
    }
    long now = app->m_p07568;
    if (m_actor != -1)
    {
        VICSprite *spr = (VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor));
        if (m_showTime < now)
        {
            if (m_attackSlot != -1)
                spr->SetAttackAction((VICSpriteAttachSlot)m_attackSlot, *m_scene);
            else
                spr->SetAction((VICSpriteAnimID)23, *m_scene);
            m_showTime = now + 5000;
        }
        else
            spr->SetAction((VICSpriteAnimID)0, *m_scene);
    }
    int visible = m_actor != -1 && active != m_cust;
    if (visible)
    {
        ApplyCamera();
        m_scene->SetPersonalLightMode((VIPersonalLightMode)2);
        m_raster->Viewport(358, 48, 620, 396);
        m_scene->SetFarPlane(*m_camera);
        m_scene->Render(*m_camera);
        m_raster->Viewport(0, 0, 639, 447);
    }
    ui->Begin2D();
    if (m_sel < app->GetCharCount())
    {
        VICharRecord *c = GetSel();
        VIString::snprintf16(text, sizeof(text), "%S %u",
                             (const unsigned short *)ui->m_words.GetString(503, ui->m_language), c->m_level);
        if (c->m_level > 0)
        {
            ui->DrawCenteredStringEx(text, 501, 350, white, 0, 1);
            ui->DrawCenteredStringEx((unsigned short)GetClass(c->m_class)->m_nameStrID, 501,
                                     font->GetHeight(ui->m_raster, ui->m_dict) + 346, white, 0, 1);
        }
    }
    else
    {
        ui->BlitRect((VIUITexId)91, VIVect2I(320, 47), VIVect2I(264, 349), (VIAlphaBlendMode)0, 1.0f);
        if (VIGetFeatureFlags() & 0x100)
            m_scene->m_sound->StopBgm(0);
        else
            app->ClearMusic();
        m_lastRace = -1;
    }
    VIVect2I center(320, 224);
    if (m_busy)
    {
        m_window.Init(UI(), VIVect2I(0, 0), center, 330, 3, 0);
        m_window.SetTitle(m_window.T((unsigned short)271));
        m_window.Open(false);
        m_window.Draw();
        VIString::strcpy16(text, sizeof(text), (const unsigned short *)ui->m_words.GetString(879, ui->m_language));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(text, sizeof(text), ".");
        m_window.TextBox(m_window.T(text), 40, 1, m_window.m_size.x, 1, (VIUICOLOR)0, 0);
    }
    else if (m_choosing)
    {
        m_window.Init(UI(), VIVect2I(0, 0), center, 330, 5, 0);
        m_window.SetTitle(m_window.T((unsigned short)422));
        m_window.Open(false);
        m_window.Draw();
        m_window.TextBox(m_window.T((unsigned short)1778), 40, 1, m_window.m_size.x - 60, 1, (VIUICOLOR)(m_choice == 0), 0);
        if (m_choice == 0)
            m_window.TickR(40, 1, true);
        m_window.TextBox(m_window.T((unsigned short)259), 40, 2, m_window.m_size.x - 60, 1, (VIUICOLOR)(m_choice == 1), 0);
        if (m_choice == 1)
            m_window.TickR(40, 2, true);
        VIString::snprintf16(text, sizeof(text), "|x%S |t%S",
                             (const unsigned short *)ui->m_words.GetString(212, ui->m_language),
                             (const unsigned short *)ui->m_words.GetString(161, ui->m_language));
        m_window.TextBox(text, 20, 4, m_window.m_size.x - 40, 1, (VIUICOLOR)0, 0);
    }
    else
        m_window.Close(true);
    ui->End2D();
}

// 0x00C20F28.
void VIWndCharacterSelect::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VINewChar c;
    memset(&c, 0, sizeof(c));
    long t = app->m_p07568;
    char s[2];
    s[0] = 'a' + (char)(t % 26);
    s[1] = 'a' + (char)(t / 26 % 26);
    VIString::snprintf(c.m_name, sizeof(c.m_name), "Default%c%c", s[0], s[1]);
    c.m_p18 = 1;
    m_text[0] = 0;
    switch (key)
    {
    case 13:
    case 134:
        Option(0);
        break;
    case 27:
    case 132:
        if (m_busy || m_p5EC)
            app->PlaySound(10);
        else
        {
            app->PlaySound(8);
            if (m_deleting)
                m_deleting = 0;
            else if (m_choosing)
                m_choosing = 0;
            else
            {
                m_sel = 0;
                if (VIGetFeatureFlags() & 0x100)
                    m_scene->m_sound->StopBgm(0);
                else
                    app->ClearMusic();
                app->Msg0777();
            }
        }
        break;
    case 19:
    case 138:
        if (m_choosing)
        {
            m_window.ResetTickPulse();
            if (m_choice > 0)
            {
                m_choice = m_choice - 1;
                app->PlaySound(11);
            }
            else
            {
                m_choice = 0;
                app->PlaySound(9);
            }
        }
        else
            MoveSel(-1);
        break;
    case 20:
    case 139:
        if (m_choosing)
        {
            m_window.ResetTickPulse();
            if (m_choice > 0)
            {
                m_choice = 1;
                app->PlaySound(9);
            }
            else
            {
                m_choice = m_choice + 1;
                app->PlaySound(11);
            }
        }
        else
            MoveSel(1);
        break;
    }
}

// 0x00C211A8.
void VIWndCharacterSelect::OnKeyUp(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIWnd::OnKeyUp(key);
}

// 0x00C211E8.  Slot 8, the delete slot, is reachable only with no delete pending.
void VIWndCharacterSelect::MoveSel(int delta)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_busy || m_p5EC)
    {
        app->PlaySound(10);
        return;
    }
    m_sel = m_sel + delta;
    if (m_sel < 0)
    {
        app->PlaySound(9);
        m_sel = 0;
    }
    else if (m_deleting && m_sel >= app->GetCharCount())
    {
        app->PlaySound(9);
        m_sel = app->GetCharCount() - 1;
    }
    else if (!m_deleting && m_sel >= 8)
    {
        if (m_sel > 8)
            app->PlaySound(9);
        else
            app->PlaySound(11);
        m_sel = 8;
    }
    else
    {
        app->PlaySound(11);
        app->m_scene.m_particleSystem.Purge();
        if (m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
    }
}

// 0x00C21320.
void VIWndCharacterSelect::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
            m_deleting = 0;
        }
        else if (w == m_cust)
        {
            m_choice = 0;
            m_choosing = 1;
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 268:   // the message box: yes
        m_mbox->SetVisible(false);
        app->SetActiveWindow(this);
        if (m_deleting && p1)
        {
            app->DeleteCharacter(m_sel);
            m_deleting = 0;
            if (m_actor != -1)
            {
                m_scene->ReleaseActor(m_actor);
                m_actor = -1;
            }
        }
        break;
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_mbox && (VIWnd *)p1 != m_cust &&
            (VIWnd *)p1 != this)
        {
            m_showTime = app->m_p07568 + 5000;
            if (m_actor != -1)
            {
                m_scene->ReleaseActor(m_actor);
                m_actor = -1;
                m_lastRace = -1;
            }
            m_window.Close(true);
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 330, 3, 0);
            m_window.SetTitle(m_window.T((unsigned short)271));
            m_choosing = 0;
            m_choice = 0;
        }
        else if ((VIWnd *)p1 == this && (VIWnd *)p2 != this && (VIWnd *)p2 != m_cust &&
            (VIWnd *)p2 != m_mbox && m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
        if ((VIWnd *)p2 == this)
        {
            m_mbox->SetVisible(false);
            m_cust->SetVisible(false);
        }
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C21588.
void VIWndCharacterSelect::ShowText(const char *s)
{
    m_busy = 0;
    m_p5EC = 0;
    m_choosing = 0;
    VIString::str8to16(m_text, sizeof(m_text), s);
    if (VIString::strlen16(m_text))
        ShowMessage(m_text, 0, 0, 0);
}

// 0x00C215F8.
void VIWndCharacterSelect::ShowText(const unsigned short *s)
{
    m_busy = 0;
    m_p5EC = 0;
    m_choosing = 0;
    VIString::strcpy16(m_text, sizeof(m_text), s);
    if (VIString::strlen16(m_text))
        ShowMessage(m_text, 0, 0, 0);
}

// 0x00C21668.
void VIWndCharacterSelect::ShowWord(unsigned short id)
{
    ShowText((const unsigned short *)UI()->m_words.GetString(id, UI()->m_language));
}

// 0x00C216A8.  The character in a slot, posed in the scene: the race's select spot
// (or a fixed one), its armour, hair, face, robe and weapons; the race's music.
void VIWndCharacterSelect::ShowCharacter(unsigned int model, int race, int cls, int hairColor,
                                         int hairLength, int hairStyle, int face, int robe,
                                         unsigned char robeTint, int pos, int *weapons,
                                         unsigned char *attack, unsigned char *armor,
                                         unsigned char *tints)
{
    VIClient *app = (VIClient *)GetAppPtr();
    const VIRaceDef *rd = GetRace(race);
    VIVect3 at;
    VIVect3 hpr;
    VIResourceType type;
    int index;
    int i;
    if (!m_dict->Find(model, type, index) &&
        !m_dict->Find(VIHashResourceID("HumanMale"), type, index))
        return;
    if (pos >= app->GetFrameRate())
    {
        switch (race)
        {
        case 2:
        case 5:
        case 9:
            at.x = 13.0f;
            break;
        default:
            switch (cls)
            {
            case 3:
            case 6:
            case 11:
                at.x = 13.0f;
                break;
            default:
                at.x = -13.0f;
                break;
            }
            break;
        }
        at.z = -26.0f;
        at.y = 0.0f;
    }
    else
    {
        at.x = rd->m_selectX;
        at.y = 0.0f;
        at.z = rd->m_selectZ;
    }
    hpr = m_walkCam->m_committed.m_angles;
    m_actor = m_scene->CreateActor(at, hpr, 1.0f, index);
    m_showTime = app->m_p07568 + 5000;
    if (m_actor != -1)
    {
        VICSprite *spr = (VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor));
        if (spr)
        {
            if (armor && tints)
                for (i = 0; i < 6; i++)
                    spr->SetArmorSlot((VICSpriteArmorSlot)i, (VICSpriteArmorSet)armor[i],
                                      (VICSpriteTint)tints[i], *m_scene);
            spr->SetHair(hairColor, hairLength, hairStyle, *m_scene);
            spr->SetFace(face, *m_scene);
            if (robe != -1)
                spr->SetRobe(1, robe, (VICSpriteTint)robeTint, *m_scene);
            VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
            spr->SetWeaponTrail(0, white);
        }
        m_attackSlot = -1;
        if (weapons && attack && spr)
            for (i = 0; i < 3; i++)
            {
                if (!weapons[i])
                    continue;
                VIResourceType itype;
                int item;
                if (!m_dict->Find(weapons[i], itype, item) &&
                    !m_dict->Find(VIHashResourceID("!Item"), itype, item))
                    continue;
                spr->AttachItem(item, (VICSpriteAttachSlot)i, (VICSpriteAttackType)attack[i], -1,
                                *m_scene);
                if ((unsigned)i < 2 && attack[i])
                {
                    spr->SetAttackAction((VICSpriteAttachSlot)i, *m_scene);
                    m_attackSlot = i;
                }
            }
    }
    m_walkCam->Init(at, hpr, *m_camera, *m_scene);
    m_walkCam->m_parm64 = m_actor;
    m_walkCam->SetCameraMode((VIWalkCameraMode)1);
    if (race != m_lastRace)
    {
        if (VIGetFeatureFlags() & 0x100)
            m_scene->m_sound->PlayBgm(0, rd->m_bgm);
        else
            app->SetMusic(race == 9 ? GetRace(5)->m_bgm : rd->m_bgm);
        m_lastRace = race;
    }
}

// 0x00C21B08.
void VIWndCharacterSelect::ApplyCamera()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel < app->GetCharCount())
    {
        VIRaceCamera *rc = GetRaceCamera(GetSel()->m_race, app->ExpansionFlags());
        if (rc)
        {
            m_walkCam->m_targetHeight = rc->m_reviewHeight;
            m_walkCam->m_lookAtHeightAbove = rc->m_reviewLookAt;
            m_walkCam->SetDefOrbitPitchAngle(rc->m_reviewPitch);
            m_walkCam->SetOrbitDistance(rc->m_reviewDistance);
        }
        else
        {
            m_walkCam->m_targetHeight = 1.1f;
            m_walkCam->m_lookAtHeightAbove = 1.7f;
            m_walkCam->SetDefOrbitPitchAngle(0.69813174f);
            m_walkCam->SetOrbitDistance(0.75f);
        }
    }
    m_walkCam->OrbitRight(0.3f);
    m_walkCam->Process(*m_camera, *m_scene, 0);
}

// 0x00C21C28.
void VIWndCharacterSelect::Option(int on)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    if (m_busy || m_p5EC)
    {
        app->PlaySound(10);
        return;
    }
    app->PlaySound(13);
    if (m_deleting)
    {
        if (GetSel()->m_serverId == -1)
            return;
        unsigned short buf[128];
        VIString::snprintf16(buf, sizeof(buf),
                             (const unsigned short *)ui->m_words.GetString(880, ui->m_language),
                             GetSel());
        ShowMessage(buf, 1, m_deleting, 0);
    }
    else if (m_choosing)
    {
        if (m_choice == 0)
        {
            m_window.Close(true);
            m_choosing = 0;
            m_busy = 1;
            app->SelectCharacter(m_sel);
        }
        else if (m_choice == 1)
        {
            VIVect3 &pos = m_walkCam->m_committed.m_position;
            VIVect3 &hpr = m_walkCam->m_committed.m_angles;
            m_cust->SetVisible(true);
            m_cust->SetChar(m_sel, pos, hpr);
            if (m_actor != -1)
            {
                m_scene->ReleaseActor(m_actor);
                m_actor = -1;
            }
            app->SetActiveWindow(m_cust);
        }
    }
    else if (m_sel == 8)
    {
        if (app->GetCharCount() > 0)
        {
            m_deleting = 1;
            m_sel = 0;
            if (m_actor != -1)
            {
                m_scene->ReleaseActor(m_actor);
                m_actor = -1;
            }
        }
    }
    else if (m_sel >= app->GetCharCount() && m_sel < 8)
    {
        app->SelectCharacter(m_sel);
        if (m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
    }
    else
    {
        if (GetSel()->m_serverId == -1)
            return;
        if (!on && (app->ExpansionFlags() & 2))
        {
            m_choice = 0;
            m_choosing = 1;
        }
        else
        {
            m_busy = 1;
            app->SelectCharacter(m_sel);
        }
    }
}

// 0x00C21E68.
void VIWndCharacterSelect::ShowMessage(const unsigned short *s, int type, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_mbox->Init(type, 1);
    m_mbox->SetText(s);
    m_mbox->m_p1 = p1;
    m_mbox->m_p2 = p2;
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}

// 0x00C21F10.
void VIWndCharacterSelect::ShowMessage(const char *s, int type, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_mbox->Init(type, 1);
    m_mbox->SetText(s);
    m_mbox->m_p1 = p1;
    m_mbox->m_p2 = p2;
    m_mbox->SetVisible(true);
    app->SetActiveWindow(m_mbox);
}

// 0x00C21FB8.
VICharRecord *VIWndCharacterSelect::GetSel()
{
    VIClient *app = (VIClient *)GetAppPtr();
    return app->GetChar(m_sel);
}
