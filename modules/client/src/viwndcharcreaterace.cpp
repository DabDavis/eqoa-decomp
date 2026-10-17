#line 1 "C:/code-frontiers-gm/game/VIWndCharCreateRace.cpp"
// VIWndCharCreateRace -- CLIENT 0x00C2B058 .. 0x00C2D8D8: the race page of character creation
// (cl_wc.h).
#include "game.h"

// 0x00CE3E98: the races in display order, with the humans split by city.
static VIRec16_C2D7C0 sRaces[11] =
{
    { 6, 0,  -1, 373 },
    { 2, 0,  -1, 369 },
    { 4, 0,  -1, 371 },
    { 1, 0,  -1, 368 },
    { 8, 0,  -1, 375 },
    { 3, 0,  -1, 370 },
    { 7, 0,  -1, 374 },
    { 0, 1, 459, 455 },
    { 0, 2, 460, 454 },
    { 9, 0,  -1, 376 },
    { 5, 0,  -1, 372 },
};

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// One WORDS.TXT entry centred at (x, y).
static inline void DrawCentered(VIUI *ui, VIFont *font, int id, int x, int y)
{
    font->DrawCenteredString(ui->m_raster, ui->m_dict, Word(ui, id), x, y, 1);
}

// 0x00C2B058.
VIWndCharCreateRace::VIWndCharCreateRace()
{
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_time = 0;
    m_sel = 0;
    m_sex = 0;
    m_actor = -1;
    m_list.Init(0);
    m_count = 0;
}

// 0x00C2B0D8.
VIWndCharCreateRace::~VIWndCharCreateRace()
{
    Free();
}

// 0x00C2B138.
int VIWndCharCreateRace::Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam)
{
    m_scene = &scene;
    m_time = 0;
    m_camera = &camera;
    m_walkCam = &walkCam;
    m_glow = 1.0f;
    m_pulse = 1;
    m_music = 1;
    m_list.Init(0);
    m_count = 0;
    m_top = 0;
    return 0;
}

// 0x00C2B198.
int VIWndCharCreateRace::Free()
{
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_sel = 0;
    m_sex = 0;
    m_glow = 1.0f;
    m_pulse = 1;
    m_actor = -1;
    m_list.Init(0);
    m_count = 0;
    m_top = 0;
    return 0;
}

// 0x00C2B200.
int VIWndCharCreateRace::Reset()
{
    m_glow = 1.0f;
    m_pulse = 1;
    m_sel = 0;
    m_sex = 0;
    m_top = 0;
    if (m_actor != -1 && m_scene)
    {
        m_scene->ReleaseActor(m_actor);
        m_actor = -1;
    }
    return 0;
}

// 0x00C2B270.
void VIWndCharCreateRace::OnDrawSelf()
{
    int y = 8;
    VIRaster *raster = m_scene->m_raster;
    VIClient *app = (VIClient *)GetAppPtr();
    long now = app->m_p07568;
    VIUI *ui = &app->m_ui;
    VIFont *font = &app->m_ui.m_font;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    ui->Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIColor32F grey(0.8f, 0.8f, 0.8f, 1.0f);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, 1.0f);
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 450), (VIAlphaBlendMode)0, 1.0f);
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 257)) + 64;
    ui->BlitBar((unsigned short)257, white, VIVect2I((640 - w) / 2, y), (VIUITexId)33, (VIUITexId)34,
                w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    ui->BlitRect((VIUITexId)51, VIVect2I(-1, y + 40), VIVect2I(348, 355), (VIAlphaBlendMode)0, 1.0f);
    ui->BlitStuddedHorz(VIVect2I(-1, 37), (VIUITexId)37, (VIUITexId)38, 348, 3, (VIAlphaBlendMode)0, 1.0f, 0);
    ui->BlitStuddedHorz(VIVect2I(-1, 396), (VIUITexId)40, (VIUITexId)41, 348, 3, (VIAlphaBlendMode)0, 1.0f, 0);
    VIUIFrameParam fp;
    VIUIFrameParam studs;
    memset(&fp, 0, sizeof(fp));
    memset(&studs, 0, sizeof(studs));
    fp.m_left.m_id = (VIUITexId)42;
    fp.m_right.m_id = (VIUITexId)42;
    fp.m_bl.m_id = (VIUITexId)39;
    fp.m_br.m_flags = 1;
    fp.m_bottom.m_id = (VIUITexId)40;
    fp.m_right.m_flags = 1;
    fp.m_br.m_id = (VIUITexId)39;
    fp.m_top.m_id = (VIUITexId)37;
    fp.m_tl.m_id = (VIUITexId)36;
    fp.m_tr.m_flags = 1;
    studs.m_left.m_id = (VIUITexId)43;
    fp.m_tr.m_id = (VIUITexId)36;
    studs.m_right.m_id = (VIUITexId)43;
    studs.m_right.m_flags = 1;
    studs.m_bottom.m_id = (VIUITexId)41;
    studs.m_top.m_id = (VIUITexId)38;
    ui->BlitStuddedFrame(VIVect2I(347, 37), VIVect2I(284, 377), fp, studs, 2, 3, (VIAlphaBlendMode)0, 1.0f, 1);
    VIColor32F c;
    VIVect2I pos;
    VIVect2I at;
    const VIUITexEntry *arrow = GetUITexture((VIUITexId)54);
    int bw;
    pos.Set(15, 48);
    if (m_count < 11)
    {
        bw = 210;
    }
    else
    {
        float t = (float)m_top / (float)(m_count - 10);
        m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(0, 0), 345, 5, 1);
        m_window.DrawVScroll(VIVect2I(322, pos.y), (arrow->m_uv1.y - 3) * 10, t, 0);
        bw = 190;
    }
    for (int i = 0; i < m_count && i < 10; i++)
    {
        int row = i + m_top;
        VIUITexId left = (VIUITexId)53, right, mid;
        if (row == m_sel)
        {
            mid = (VIUITexId)57;
            left = (VIUITexId)56;
            c = yellow;
            right = (VIUITexId)58;
        }
        else
        {
            c = white;
            mid = (VIUITexId)54;
            right = (VIUITexId)55;
        }
        if (m_list[row].m_race == 0)
        {
            unsigned short name[32];
            VIString::strcpy16(name, sizeof(name), Word(ui, ::GetRace(0)->m_nameStrID));
            VIString::strcat16(name, sizeof(name), "-");
            VIString::strcat16(name, sizeof(name), Word(ui, m_list[row].m_cityWord));
            ui->BlitBar(name, c, pos, left, right, mid, bw, (VIAlphaBlendMode)0, 1.0f, 1, 1);
        }
        else
            ui->BlitBar((unsigned short)::GetRace(m_list[row].m_race)->m_nameStrID, c, pos, left, right, mid,
                        bw, (VIAlphaBlendMode)0, 1.0f, 1, 1);
        at = pos + VIVect2I(bw, 0);
        if (row == m_sel)
        {
            VIUITexId male = (VIUITexId)128, female;
            if (m_sex == 0)
                female = (VIUITexId)127;
            else if (m_sex == 1)
            {
                male = (VIUITexId)126;
                female = (VIUITexId)128;
            }
            else
                female = (VIUITexId)128;
            ui->BlitBar((unsigned short)*GetSexWord(m_sex), c, at, left, right, mid, 117,
                        (VIAlphaBlendMode)0, 1.0f, 0, 1);
            ui->BlitEx(male, at + VIVect2I(1, 5), (VIAlphaBlendMode)3, male == 128 ? 1.0f : m_glow, male == 128);
            ui->BlitEx(female, at + VIVect2I(104, 5), (VIAlphaBlendMode)3, female == 128 ? 1.0f : m_glow, 0);
        }
        else
            ui->BlitBar("", c, at, left, right, mid, 117, (VIAlphaBlendMode)0, 1.0f, 0, 1);
        pos.y += arrow->m_uv1.y - 3;
    }
    memset(&fp, 0, sizeof(fp));
    pos.y += 5;
    fp.m_bottom.m_id = (VIUITexId)49;
    fp.m_left.m_id = (VIUITexId)47;
    fp.m_bl.m_id = (VIUITexId)48;
    fp.m_right.m_id = (VIUITexId)52;
    fp.m_br.m_id = (VIUITexId)50;
    fp.m_top.m_id = (VIUITexId)45;
    fp.m_tl.m_id = (VIUITexId)44;
    fp.m_tr.m_id = (VIUITexId)46;
    ui->BlitFillFrame(pos, VIVect2I(325, 100), fp, (VIUITexId)35, (VIAlphaBlendMode)0, 1.0f, 1);
    pos += VIVect2I(163, 50);
    m_window.Init(VIWnd::UI(), VIVect2I(0, 0), pos, 345, 5, 1);
    m_window.TextBox(Word(ui, m_list[m_sel].m_descWord), 20, 0, m_window.Width() - 40, 5, black, 2);
    if (m_pulse == -1)
    {
        m_glow -= dt + dt;
        m_glow = m_glow < 0.0f ? 0.0f : m_glow;
        if (m_glow == 0.0f)
            m_pulse = 1;
    }
    else
    {
        m_glow += dt * 3.0f;
        m_glow = m_glow < 1.0f ? m_glow : 1.0f;
        if (m_glow == 1.0f)
            m_pulse = -1;
    }
    font->SetTextColor(black);
    VIVect2I key(50, 408);
    w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217))
      + 110 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 212));
    ui->BlitBar("", black, key, (VIUITexId)25, (VIUITexId)26, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    key += VIVect2I(20, 4);
    ui->Blit((VIUITexId)65, key);
    key.x += 25;
    DrawCentered(ui, font, 217, key.x, key.y + 5);
    key.x += 20 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217));
    ui->Blit((VIUITexId)66, key);
    key.x += 25;
    DrawCentered(ui, font, 212, key.x, key.y + 5);
    font->SetTextColor(white);
    ui->End2D();
    if (m_actor == -1)
    {
        const unsigned int *models = ::GetRace(m_list[m_sel].m_race)->m_model;
        CreateModel(models[m_sex]);
    }
    if (m_actor != -1)
        ((VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor)))->SetAction((VICSpriteAnimID)0, *m_scene);
    UpdateCamera();
    m_scene->SetPersonalLightMode((VIPersonalLightMode)2);
    if (~m_actor)
    {
        raster->Viewport(358, 48, 620, 396);
        m_scene->SetFarPlane(*m_camera);
        m_scene->Render(*m_camera);
        raster->Viewport(0, 0, 639, 447);
    }
    ui->End2D();
}

// 0x00C2BF38.  Enter hands the parent the race, sex and city (262), back is 260; up/down pick
// the race, left/right the sex.
void VIWndCharCreateRace::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
    {
        app->PlaySound(13);
        VINewChar c;
        c.m_race = m_list[m_sel].m_race;
        c.m_sex = m_sex;
        c.m_p38 = m_list[m_sel].m_city;
        if (m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
        GetParent()->OnMessage(this, 262, (int)&c, 0);
        m_sel = 0;
        m_top = 0;
        m_sex = 0;
        break;
    }
    case 27:
    case 132:
        app->PlaySound(8);
        m_sel = 0;
        m_top = 0;
        m_sex = 0;
        if (m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 19:
    case 138:
        MoveSel(-1);
        break;
    case 20:
    case 139:
        MoveSel(1);
        break;
    case 17:
    case 136:
        SetSex(-1);
        break;
    case 18:
    case 137:
        SetSex(1);
        break;
    }
}

// 0x00C2C150.
void VIWndCharCreateRace::OnKeyUp(int key)
{
    GetAppPtr();
    VIWnd::OnKeyUp(key);
}

// 0x00C2C190.  Ten rows show at a time.
void VIWndCharCreateRace::MoveSel(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_glow = 1.0f;
    m_pulse = 1;
    m_time = app->m_p07568;
    if (m_sel + d < 0)
    {
        app->PlaySound(9);
        m_sel = 0;
    }
    else if (m_sel + d >= m_count)
    {
        app->PlaySound(9);
        m_sel = m_count - 1;
    }
    else
    {
        app->PlaySound(11);
        m_sel += d;
        if (m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
    }
    if (m_top + 9 < m_sel)
        m_top = m_sel - 9;
    if (m_sel < m_top)
        m_top = m_sel;
}

// 0x00C2C288.
void VIWndCharCreateRace::SetSex(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_pulse = 1;
    m_glow = 1.0f;
    m_time = app->m_p07568;
    if (m_sex + d < 0)
    {
        app->PlaySound(9);
        m_sex = 0;
    }
    else if (m_sex + d >= 2)
    {
        app->PlaySound(9);
        m_sex = 1;
    }
    else
    {
        app->PlaySound(11);
        m_sex += d;
        if (m_actor != -1)
        {
            m_music = 0;
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
    }
}

// 0x00C2C348.  Gaining the focus opens the list on the form's race and city, if it has one.
void VIWndCharCreateRace::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_pulse = 1;
            m_glow = 1.0f;
            m_time = app->m_p07568;
            m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(175, 343), 340, 4, 1);
            m_window.Open(true);
            VIWndCharCreateBlank *form = (VIWndCharCreateBlank *)GetParent();
            BuildList();
            m_sel = 0;
            m_top = 0;
            for (int i = 0; i < m_count; i++)
            {
                if (form->m_char.m_race == m_list[i].m_race && form->m_char.m_p38 == m_list[i].m_city)
                {
                    m_sel = i;
                    if (m_count > 10 && i > 9)
                    {
                        int top = i - 9;
                        if (top < 0)
                            top = 0;
                        if (top > m_count - 10)
                            top = m_count - 10;
                        m_top = top;
                    }
                    break;
                }
            }
            m_sex = form->m_char.m_race != -1 ? form->m_char.m_sex : 0;
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C2C4F0.  The model of the selected race and sex on the race's spot, in the form's look
// where it has one and the race's default where not; the first model after SetSex keeps the
// music, every other one plays the race's (the humans' by city).
void VIWndCharCreateRace::CreateModel(unsigned int model)
{
    VIDictionary *dict = m_scene->m_dict;
    VIClient *app = (VIClient *)GetAppPtr();
    int exp = app->ExpansionFlags();
    const VIRaceDef *race = ::GetRace(m_list[m_sel].m_race);
    VIRaceInfo *def = GetRaceInfo(m_list[m_sel].m_race, exp);
    VIVect3 pos;
    VIVect3 hpr;
    VIResourceType type;
    int idx;
    if (!dict->Find(model, type, idx) && !dict->Find(VIHashResourceID("HumanMale"), type, idx))
        return;
    pos.x = race->m_selectX;
    pos.y = 0;
    pos.z = race->m_selectZ;
    hpr = m_walkCam->m_committed.m_angles;
    m_actor = m_scene->CreateActor(pos, hpr, 1.0f, idx);
    m_walkCam->Init(pos, hpr, *m_camera, *m_scene);
    m_walkCam->m_parm64 = m_actor;
    m_walkCam->SetCameraMode((VIWalkCameraMode)1);
    VIWndCharCreateBlank *form = (VIWndCharCreateBlank *)GetParent();
    int face, style, length, color;
    if (m_list[m_sel].m_city == 1)
    {
        if (exp & 2)
        {
            if (m_sex == 0)
            {
                face = form->m_char.m_face != -1 ? form->m_char.m_face : 2;
                style = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : 2;
                length = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : 2;
                color = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : 1;
            }
            else
            {
                face = form->m_char.m_face != -1 ? form->m_char.m_face : 0;
                style = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : 0;
                length = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : 3;
                color = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : 1;
            }
        }
        else
        {
            face = form->m_char.m_face != -1 ? form->m_char.m_face : 2;
            style = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : 0;
            length = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : 2;
            color = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : 2;
        }
    }
    else if (m_sex == 0)
    {
        face = form->m_char.m_face != -1 ? form->m_char.m_face : def->m_male[0];
        style = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : def->m_male[1];
        length = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : def->m_male[2];
        color = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : def->m_male[3];
    }
    else
    {
        face = form->m_char.m_face != -1 ? form->m_char.m_face : def->m_female[0];
        style = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : def->m_female[1];
        length = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : def->m_female[2];
        color = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : def->m_female[3];
    }
    VICSprite *spr = (VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor));
    spr->SetHair(color, length, style, *m_scene);
    spr->SetFace(face, *m_scene);
    if (m_music > 0)
    {
        const char *bgm;
        if (m_list[m_sel].m_city == 2)
            bgm = "bgm\\qeynos.bgm";
        else if (m_list[m_sel].m_city == 1)
            bgm = "bgm\\freeport.bgm";
        else
            bgm = race->m_bgm;
        if (VIGetFeatureFlags() & 0x100)
            m_scene->m_sound->PlayBgm(0, bgm);
        else
            app->SetMusic(bgm);
    }
    else
        m_music = 1;
}

// 0x00C2C8D8.  The selected race's review camera, or the default one.
void VIWndCharCreateRace::UpdateCamera()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIRaceCamera *rc = GetRaceCamera(m_list[m_sel].m_race, app->ExpansionFlags());
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
    m_walkCam->OrbitRight(0.3f);
    m_walkCam->Process(*m_camera, *m_scene, 0);
}

// 0x00C2C9D8.  The races the owned expansions allow (a race of no expansion always).
void VIWndCharCreateRace::BuildList()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_list.Init(11);
    m_count = 0;
    for (int i = 0; i < 11; i++)
    {
        const VIRaceDef *race = ::GetRace(sRaces[i].m_race);
        if ((app->ExpansionFlags() & race->m_expansion) || !race->m_expansion)
        {
            m_list[m_count] = sRaces[i];
            m_count++;
        }
    }
}
