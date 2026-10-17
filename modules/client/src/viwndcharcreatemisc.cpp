#line 1 "C:/code-frontiers-gm/game/VIWndCharCreateMisc.cpp"
// VIWndCharCreateMisc -- CLIENT 0x00C26E50 .. 0x00C29968: the appearance page of character
// creation (cl_wc.h).  Lane cl-wc.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// One WORDS.TXT entry centred at (x, y).
static inline void DrawWord(VIUI *ui, VIFont *font, int id, int x, int y, unsigned int f)
{
    font->DrawCenteredString(ui->m_raster, ui->m_dict, Word(ui, id), x, y, f);
}

// 0x00C26E50.
VIWndCharCreateMisc::VIWndCharCreateMisc()
{
    m_actor = -1;
    m_glow = 1.0f;
    m_pulse = 1;
    m_char = 0;
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_sel = 0;
    m_face = 0;
    m_hairStyle = 0;
    m_hairLength = 0;
    m_hairColor = 0;
    m_time = 0;
}

// 0x00C26ED0.
VIWndCharCreateMisc::~VIWndCharCreateMisc()
{
    Free();
}

// 0x00C26F28.
int VIWndCharCreateMisc::Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam)
{
    m_camera = &camera;
    m_walkCam = &walkCam;
    m_scene = &scene;
    m_face = 3;
    m_hairStyle = 3;
    m_hairLength = 3;
    m_hairColor = 0;
    m_pulse = 1;
    m_glow = 1.0f;
    m_time = 0;
    return 0;
}

// 0x00C26F68.
int VIWndCharCreateMisc::Free()
{
    m_glow = 1.0f;
    m_actor = -1;
    m_pulse = 1;
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_sel = 0;
    m_face = 0;
    m_hairStyle = 0;
    m_hairLength = 0;
    m_hairColor = 0;
    m_char = 0;
    return 0;
}

// 0x00C26FB0.
int VIWndCharCreateMisc::Reset()
{
    m_pulse = 1;
    m_glow = 1.0f;
    m_sel = 0;
    m_face = 0;
    m_hairStyle = 0;
    m_hairLength = 0;
    m_hairColor = 0;
    if (m_actor != -1 && m_scene)
    {
        m_scene->ReleaseActor(m_actor);
        m_actor = -1;
    }
    return 0;
}

// 0x00C27028.
void VIWndCharCreateMisc::OnDrawSelf()
{
    VIRaster *raster = m_scene->m_raster;
    VIClient *app = (VIClient *)GetAppPtr();
    long now = app->m_p07568;
    VIUI *ui = &app->m_ui;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    ui->Begin2D();
    VIVect2I tl;
    VIVect2I size;
    VIVect2I inner;
    VIVect2I at;
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    const VIUITexEntry *c0 = GetUITexture((VIUITexId)44);
    const VIUITexEntry *c1 = GetUITexture((VIUITexId)50);
    const VIUITexEntry *c2 = GetUITexture((VIUITexId)36);
    const VIUITexEntry *c3 = GetUITexture((VIUITexId)39);
    if (!c0 || !c1 || !c2 || !c3)
        return;
    size.Set(284, 377);
    inner.Set(size.x - c0->m_uv1.x - c1->m_uv1.x - c2->m_uv1.x - c3->m_uv1.x,
              size.y - c0->m_uv1.y - c1->m_uv1.y - c2->m_uv1.y - c3->m_uv1.y);
    tl.Set(1, 37);
    at.Set(tl.x + c2->m_uv1.x, tl.y + c2->m_uv1.y);
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 450), (VIAlphaBlendMode)0, 1.0f);
    const VIUITexEntry *gem = GetUITexture((VIUITexId)59);
    if (!gem)
        return;
    int x = 285;
    int y = 37;
    ui->BlitEx((VIUITexId)59, VIVect2I(x, y), (VIAlphaBlendMode)0, 1.0f, 0);
    VIFont *font = &app->m_ui.m_font;
    ui->BlitEx((VIUITexId)59, VIVect2I(x + gem->m_uv1.x, y), (VIAlphaBlendMode)0, 1.0f, 1);
    ui->BlitEx((VIUITexId)59, VIVect2I(x + gem->m_uv1.x, y + gem->m_uv1.y), (VIAlphaBlendMode)0, 1.0f, 3);
    ui->BlitEx((VIUITexId)59, VIVect2I(x, y + gem->m_uv1.y), (VIAlphaBlendMode)0, 1.0f, 2);
    y = 405;
    ui->BlitHorz(VIVect2I(x, y), (VIUITexId)60, 62, (VIAlphaBlendMode)0, 1.0f, 0);
    VIUIFrameParam fp;
    VIUIFrameParam studs;
    memset(&fp, 0, sizeof(fp));
    fp.m_bottom.m_id = (VIUITexId)49;
    fp.m_left.m_id = (VIUITexId)47;
    fp.m_bl.m_id = (VIUITexId)48;
    fp.m_right.m_id = (VIUITexId)52;
    fp.m_br.m_id = (VIUITexId)50;
    fp.m_top.m_id = (VIUITexId)45;
    fp.m_tl.m_id = (VIUITexId)44;
    fp.m_tr.m_id = (VIUITexId)46;
    inner.Set(size.x - c2->m_uv1.x - c3->m_uv1.x, size.y - c2->m_uv1.y - c3->m_uv1.y);
    ui->BlitFillFrame(at, inner, fp, (VIUITexId)35, (VIAlphaBlendMode)0, 1.0f, 1);
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
    ui->BlitStuddedFrame(tl, size, fp, studs, 2, 3, (VIAlphaBlendMode)0, 1.0f, 1);
    ui->BlitStuddedFrame(tl + VIVect2I(346, 0), size, fp, studs, 2, 3, (VIAlphaBlendMode)0, 1.0f, 1);
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 259)) + 64;
    int ty = 8;
    ui->BlitBar((unsigned short)259, white, VIVect2I((640 - w) / 2, ty), (VIUITexId)33, (VIUITexId)34,
                w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    font->SetTextColor(black);
    at.Set(50, 408);
    w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217))
      + 110 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 216));
    ui->BlitBar("", black, at, (VIUITexId)25, (VIUITexId)26, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    at += VIVect2I(20, 4);
    ui->Blit((VIUITexId)65, at);
    at.x += 25;
    DrawWord(ui, font, 217, at.x, at.y + 5, 1);
    at.x += 20 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217));
    ui->Blit((VIUITexId)66, at);
    at.x += 25;
    DrawWord(ui, font, 216, at.x, at.y + 5, 1);

    tl.Set(27, 92);
    at = tl;
    ui->BlitEx((VIUITexId)135, at, (VIAlphaBlendMode)0, 1.0f, 0);
    at.x += 120;
    ui->BlitEx((VIUITexId)135, at, (VIAlphaBlendMode)0, 1.0f, 1);
    at = tl + VIVect2I(95, -40);
    ui->BlitEx((VIUITexId)131, at, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel == 0)
        ui->BlitEx((VIUITexId)206, at + VIVect2I(9, 15), (VIAlphaBlendMode)3, m_glow, 0);
    at += VIVect2I(25, 60);
    DrawWord(ui, font, 604, at.x, at.y, 0);

    tl.y += 84;
    at = tl;
    ui->BlitEx((VIUITexId)135, at, (VIAlphaBlendMode)0, 1.0f, 0);
    at.x += 120;
    ui->BlitEx((VIUITexId)135, at, (VIAlphaBlendMode)0, 1.0f, 1);
    at = tl + VIVect2I(95, -40);
    ui->BlitEx((VIUITexId)132, at, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel == 1)
        ui->BlitEx((VIUITexId)207, at + VIVect2I(23, 25), (VIAlphaBlendMode)3, m_glow, 0);
    at += VIVect2I(25, 60);
    DrawWord(ui, font, 605, at.x, at.y, 0);

    tl.y += 84;
    at = tl;
    ui->BlitEx((VIUITexId)135, at, (VIAlphaBlendMode)0, 1.0f, 0);
    at.x += 120;
    ui->BlitEx((VIUITexId)135, at, (VIAlphaBlendMode)0, 1.0f, 1);
    at = tl + VIVect2I(95, -40);
    ui->BlitEx((VIUITexId)133, at, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel == 2)
        ui->BlitEx((VIUITexId)208, at + VIVect2I(1, 2), (VIAlphaBlendMode)3, m_glow, 0);
    at += VIVect2I(25, 60);
    DrawWord(ui, font, 606, at.x, at.y, 0);

    tl.y += 84;
    at = tl;
    ui->BlitEx((VIUITexId)135, at, (VIAlphaBlendMode)0, 1.0f, 0);
    at.x += 120;
    ui->BlitEx((VIUITexId)135, at, (VIAlphaBlendMode)0, 1.0f, 1);
    at = tl + VIVect2I(95, -35);
    ui->BlitEx((VIUITexId)134, at, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel == 3)
        ui->BlitEx((VIUITexId)209, at + VIVect2I(5, 6), (VIAlphaBlendMode)3, m_glow, 0);
    at += VIVect2I(25, 56);
    DrawWord(ui, font, 607, at.x, at.y, 0);

    at = VIVect2I(73, m_sel * 83 + 75);
    ui->BlitEx((VIUITexId)85, at, (VIAlphaBlendMode)0, 1.0f, 0);
    at.x += 125;
    ui->BlitEx((VIUITexId)85, at, (VIAlphaBlendMode)0, 1.0f, 1);
    int cx = 142, cy = 240;
    m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(cx, cy), 280, 16, 1);
    m_window.DrawHScroll(30, 1, m_window.Width() - 60, (float)m_face / 3.0f, 0);
    m_window.DrawHScroll(30, 6, m_window.Width() - 60, (float)m_hairStyle / 3.0f, 0);
    m_window.DrawHScroll(30, 11, m_window.Width() - 60, (float)m_hairLength / 3.0f, 0);
    m_window.DrawHScroll(30, 16, m_window.Width() - 60, (float)m_hairColor / 7.0f, 0);
    font->SetTextColor(white);
    ui->End2D();
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
    if (m_actor == -1)
    {
        const unsigned int *models = GetRace(m_char->m_race)->m_model;
        CreateModel(models[m_char->m_sex]);
    }
    if (m_actor != -1)
        ((VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor)))->SetAction((VICSpriteAnimID)0, *m_scene);
    UpdateCamera();
    if (m_actor != -1)
    {
        VIColor32F light(0.75f, 0.69f, 0.36f, 1.0f);
        VIVect3 eye = m_walkCam->m_last.m_cameraPos;
        m_scene->SetPersonalLightMode((VIPersonalLightMode)1);
        m_scene->SetPersonalLight(eye, 5.0f, light);
    }
    raster->Viewport(358, 48, 620, 396);
    m_scene->SetFarPlane(*m_camera);
    m_scene->Render(*m_camera);
    raster->Viewport(0, 0, 639, 447);
}

// 0x00C280D8.  Enter hands the parent the four values (264), back is 260; up/down pick the
// row, left/right change it.
void VIWndCharCreateMisc::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
    {
        app->PlaySound(13);
        VINewChar c;
        c.m_face = m_face;
        c.m_hairStyle = m_hairStyle;
        c.m_hairLength = m_hairLength;
        c.m_hairColor = m_hairColor;
        if (m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
        GetParent()->OnMessage(this, 264, (int)&c, 0);
        m_sel = 0;
        m_face = 3;
        m_hairStyle = 3;
        m_hairLength = 3;
        m_hairColor = 0;
        break;
    }
    case 27:
    case 132:
        app->PlaySound(8);
        m_sel = 0;
        m_face = 3;
        m_hairStyle = 3;
        m_hairLength = 3;
        m_hairColor = 0;
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
        Adjust(-1);
        break;
    case 18:
    case 137:
        Adjust(1);
        break;
    }
}

// 0x00C282F0.
void VIWndCharCreateMisc::OnKeyUp(int key)
{
    GetAppPtr();
    VIWnd::OnKeyUp(key);
}

// 0x00C28330.  The hair rows turn the model a twelfth of a turn.
void VIWndCharCreateMisc::MoveSel(int d)
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
    else if (m_sel + d >= 4)
    {
        app->PlaySound(9);
        m_sel = 3;
    }
    else
    {
        app->PlaySound(11);
        m_sel += d;
    }
    if (m_sel == 2 || m_sel == 3)
        m_walkCam->SetOrbitAngle(0.5235988f);
    else
        m_walkCam->SetOrbitAngle(0.0f);
}

// 0x00C28408.  The camera on the race's spot in the select scene.
void VIWndCharCreateMisc::Activate()
{
    const VIRaceDef *race = GetRace(m_char->m_race);
    VIVect3 pos;
    pos.x = race->m_selectX;
    pos.y = 0;
    pos.z = race->m_selectZ;
    VIVect3 hpr;
    m_walkCam->Init(pos, hpr, *m_camera, *m_scene);
    SetSceneCameraLimits(m_walkCam, 0.5f, 25.0f, 3.5f);
    m_walkCam->SetCameraMode((VIWalkCameraMode)1);
}

// 0x00C284B0.
void VIWndCharCreateMisc::Adjust(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_pulse = 1;
    m_glow = 1.0f;
    m_time = app->m_p07568;
    switch (m_sel)
    {
    case 0:
        AdjustFace(d);
        break;
    case 1:
        AdjustHairStyle(d);
        break;
    case 2:
        AdjustHairLength(d);
        break;
    case 3:
        AdjustHairColor(d);
        break;
    }
}

// 0x00C28580.
void VIWndCharCreateMisc::AdjustFace(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int v = m_face + d;
    if (v < 0)
        m_face = 3;
    else if (v >= 4)
        m_face = 0;
    else
        m_face = v;
    app->PlaySound(11);
    ((VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor)))->SetFace(m_face, *m_scene);
}

// 0x00C28610.
void VIWndCharCreateMisc::AdjustHairStyle(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int v = m_hairStyle + d;
    if (v < 0)
        m_hairStyle = 3;
    else if (v >= 4)
        m_hairStyle = 0;
    else
        m_hairStyle = v;
    app->PlaySound(11);
    ((VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor)))->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
}

// 0x00C286A8.
void VIWndCharCreateMisc::AdjustHairLength(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int v = m_hairLength + d;
    if (v < 0)
        m_hairLength = 3;
    else if (v >= 4)
        m_hairLength = 0;
    else
        m_hairLength = v;
    app->PlaySound(11);
    ((VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor)))->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
}

// 0x00C28740.
void VIWndCharCreateMisc::AdjustHairColor(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int v = m_hairColor + d;
    if (v < 0)
        m_hairColor = 7;
    else if (v >= 8)
        m_hairColor = 0;
    else
        m_hairColor = v;
    app->PlaySound(11);
    ((VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor)))->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
}

// 0x00C287D8.  Gaining the focus seeds the four values from the form, and where the form has
// none (-1), from the race's defaults: the Qeynos humans of the expansion and of the original
// have their own.
void VIWndCharCreateMisc::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int exp = app->ExpansionFlags();
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_time = app->m_p07568;
            VIWndCharCreateBlank *form = (VIWndCharCreateBlank *)GetParent();
            GetRace(form->m_char.m_race);
            VIRaceInfo *def = GetRaceInfo(form->m_char.m_race, exp);
            if (form->m_char.m_p38 == 1)
            {
                if (exp & 2)
                {
                    if (form->m_char.m_sex == 0)
                    {
                        m_face = form->m_char.m_face != -1 ? form->m_char.m_face : 2;
                        m_hairStyle = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : 2;
                        m_hairLength = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : 2;
                        m_hairColor = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : 1;
                    }
                    else
                    {
                        m_face = form->m_char.m_face != -1 ? form->m_char.m_face : 0;
                        m_hairStyle = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : 0;
                        m_hairLength = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : 3;
                        m_hairColor = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : 1;
                    }
                }
                else
                {
                    m_face = form->m_char.m_face != -1 ? form->m_char.m_face : 2;
                    m_hairStyle = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : 0;
                    m_hairLength = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : 2;
                    m_hairColor = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : 2;
                }
            }
            else if (form->m_char.m_sex == 0)
            {
                m_face = form->m_char.m_face != -1 ? form->m_char.m_face : def->m_male[0];
                m_hairStyle = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : def->m_male[1];
                m_hairLength = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : def->m_male[2];
                m_hairColor = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : def->m_male[3];
            }
            else
            {
                m_face = form->m_char.m_face != -1 ? form->m_char.m_face : def->m_female[0];
                m_hairStyle = form->m_char.m_hairStyle != -1 ? form->m_char.m_hairStyle : def->m_female[1];
                m_hairLength = form->m_char.m_hairLength != -1 ? form->m_char.m_hairLength : def->m_female[2];
                m_hairColor = form->m_char.m_hairColor != -1 ? form->m_char.m_hairColor : def->m_female[3];
            }
            m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(0, 0), 275, 13, 1);
            m_window.Open(true);
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C28A68.  The model in the page's look, turned a little off the camera.
void VIWndCharCreateMisc::CreateModel(unsigned int model)
{
    VIDictionary *dict = m_scene->m_dict;
    GetAppPtr();
    VIResourceType type;
    int idx;
    if (!dict->Find(model, type, idx) && !dict->Find(VIHashResourceID("HumanMale"), type, idx))
        return;
    m_actor = m_scene->CreateActor(m_walkCam->m_committed.m_position,
                                   m_walkCam->m_committed.m_angles, 1.0f, idx);
    VICSprite *spr = (VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor));
    spr->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
    spr->SetFace(m_face, *m_scene);
    m_walkCam->m_parm64 = m_actor;
    m_walkCam->SetCameraMode((VIWalkCameraMode)1);
    const VIVect3 &hpr = m_scene->GetActorHPR(m_actor);
    m_scene->SetActorHPR(m_actor, VIVect3(hpr.x + 5.934119f, hpr.y, hpr.z));
}

// 0x00C28BC8.  The race's appearance camera; the original human male has his own.
void VIWndCharCreateMisc::UpdateCamera()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIRaceCamera *rc = GetRaceCamera(m_char->m_race, app->ExpansionFlags());
    if (rc)
    {
        if (m_char->m_race == 0 && m_char->m_sex == 0 && !(app->ExpansionFlags() & 2))
        {
            m_walkCam->m_targetHeight = 0.9f;
            m_walkCam->m_lookAtHeightAbove = 1.48f;
            m_walkCam->SetDefOrbitPitchAngle(0.567232f);
            m_walkCam->SetOrbitDistance(1.25f);
        }
        else
        {
            m_walkCam->m_targetHeight = rc->m_custHeight;
            m_walkCam->m_lookAtHeightAbove = rc->m_custLookAt;
            m_walkCam->SetDefOrbitPitchAngle(rc->m_custPitch);
            m_walkCam->SetOrbitDistance(rc->m_custDistance);
        }
    }
    else
    {
        m_walkCam->m_targetHeight = 1.1f;
        m_walkCam->m_lookAtHeightAbove = 1.7f;
        m_walkCam->SetDefOrbitPitchAngle(0.69813174f);
        m_walkCam->SetOrbitDistance(0.75f);
    }
    m_walkCam->Process(*m_camera, *m_scene, 0);
}

