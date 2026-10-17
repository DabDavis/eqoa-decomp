#line 1 "C:/code-frontiers-gm/game/VIWndCharCreateReview.cpp"
// VIWndCharCreateReview -- CLIENT 0x00C2D8D8 .. 0x00C2F5E8: the last page of character
// creation (cl_wc.h).
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C2D8D8.
VIWndCharCreateReview::VIWndCharCreateReview()
{
    m_actor = -1;
    m_char = 0;
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_idle = -1;
    m_time = -1;
}

// 0x00C2D930.
VIWndCharCreateReview::~VIWndCharCreateReview()
{
    Free();
}

// 0x00C2D978.
int VIWndCharCreateReview::Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam)
{
    m_camera = &camera;
    m_walkCam = &walkCam;
    m_scene = &scene;
    return 0;
}

// 0x00C2D990.
int VIWndCharCreateReview::Free()
{
    m_actor = -1;
    m_char = 0;
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_idle = -1;
    m_time = -1;
    return 0;
}

// 0x00C2D9C0.
int VIWndCharCreateReview::Reset()
{
    m_idle = 0;
    m_time = 0;
    if (m_actor != -1 && m_scene)
    {
        m_scene->ReleaseActor(m_actor);
        m_actor = -1;
    }
    return 0;
}

// 0x00CE4260: the seven stat labels (STR STA AGI DEX WIS INT CHA).
static const unsigned short sStatWords[7] = { 248, 249, 250, 251, 252, 253, 254 };

// 0x00CE4270: each race's home city word, the humans' by city.
static const VIRec16_C2D7C0 sHomes[11] =
{
    { 6, 0, 364, -1 }, { 8, 0, 366, -1 }, { 2, 0, 360, -1 }, { 4, 0, 608, -1 },
    { 1, 0, 1125, -1 }, { 3, 0, 361, -1 }, { 7, 0, 365, -1 }, { 0, 1, 358, -1 },
    { 0, 2, 357, -1 }, { 5, 0, 363, -1 }, { 9, 0, 1187, -1 },
};

// One WORDS.TXT entry drawn at (x, y).
static inline void DrawWord(VIUI *ui, VIFont *font, unsigned short id, int x, int y)
{
    font->DrawString(ui->m_raster, ui->m_dict, Word(ui, id), x, y);
}

// A wide string drawn at (x, y).
static inline void DrawText(VIUI *ui, VIFont *font, const unsigned short *s, int x, int y)
{
    font->DrawString(ui->m_raster, ui->m_dict, s, x, y);
}

// One WORDS.TXT entry centred at (x, y).
static inline void DrawCentered(VIUI *ui, VIFont *font, unsigned short id, int x, int y)
{
    font->DrawCenteredString(ui->m_raster, ui->m_dict, Word(ui, id), x, y, 1);
}

// 0x00C2DA18.  The two panels, the name, race, class and the seven stats; the model, idling
// every 10000 ticks; and the home city under it.
void VIWndCharCreateReview::OnDrawSelf()
{
    VIRaster *raster = m_scene->m_raster;
    VIClient *app = (VIClient *)GetAppPtr();
    const VIRaceDef *race = GetRace(m_char->m_race);
    VIUI *ui = &app->m_ui;
    const VIClassDef *cls = GetClass(m_char->m_class);
    int i;
    unsigned short num[8];
    VIVect2I vpos;
    VIVect2I tl;
    VIVect2I size;
    VIVect2I inner;
    VIVect2I at;
    VIVect2I tp(60, 96);
    ui->Begin2D();
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    const VIUITexEntry *c0 = GetUITexture((VIUITexId)44);
    const VIUITexEntry *c1 = GetUITexture((VIUITexId)50);
    const VIUITexEntry *c2 = GetUITexture((VIUITexId)36);
    const VIUITexEntry *c3 = GetUITexture((VIUITexId)39);
    if (!c0 || !c1 || !c2 || !c3)
        return;
    size.Set(284, 377);
    inner.Set(size.x - c0->m_uv1.x - c1->m_uv1.x - c2->m_uv1.x - c3->m_uv1.x, size.y - c0->m_uv1.y - c1->m_uv1.y - c2->m_uv1.y - c3->m_uv1.y);
    tl.Set(1, 37);
    at.Set(c2->m_uv1.x + 1, c2->m_uv1.y + 37);
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
    fp.m_bottom.m_id = (VIUITexId)40;
    fp.m_bl.m_id = (VIUITexId)39;
    fp.m_br.m_flags = 1;
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
    font->SetTextColor(black);
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 261)) + 64;
    int ty = 8;
    ui->BlitBar((unsigned short)261, white, VIVect2I((640 - w) / 2, ty), (VIUITexId)33, (VIUITexId)34,
                w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    at.Set(50, 408);
    w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217))
      + 110 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 218));
    ui->BlitBar("", black, at, (VIUITexId)25, (VIUITexId)26, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    at += VIVect2I(20, 4);
    ui->Blit((VIUITexId)65, at);
    at.x += 25;
    DrawCentered(ui, font, 217, at.x, at.y + 5);
    at.x += 20 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217));
    ui->Blit((VIUITexId)66, at);
    at.x += 25;
    DrawCentered(ui, font, 218, at.x, at.y + 5);
    font->DrawString(ui->m_raster, ui->m_dict, m_char->m_name, tp.x, tp.y);
    tp.y += 19;
    DrawWord(ui, font, GetRace(m_char->m_race)->m_nameStrID, tp.x, tp.y);
    tp.y += 19;
    DrawWord(ui, font, GetClass(m_char->m_class)->m_nameStrID, tp.x, tp.y);
    tp.y += 38;
    for (i = 0; i < 7; i++)
    {
        DrawWord(ui, &ui->m_font, sStatWords[i], tp.x, tp.y);
        vpos.Set(tp.x + 140, tp.y);
        VIString::snprintf16(num, sizeof(num), "%u",
                             m_char->m_stats[i] + race->m_bonus[i] + cls->m_bonus[i] + 60);
        DrawText(ui, &ui->m_font, num, vpos.x, vpos.y);
        tp.y += 19;
    }
    ui->End2D();
    if (m_actor == -1)
    {
        const unsigned int *models = GetRace(m_char->m_race)->m_model;
        CreateModel(models[m_char->m_sex]);
    }
    if (m_time == -1)
        m_time = app->m_p07568;
    m_idle -= app->m_p07568 - m_time;
    if (m_actor != -1)
    {
        VICSprite *spr = (VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor));
        if (m_idle <= 0 || m_idle > 10000)
        {
            spr->SetAction((VICSpriteAnimID)15, *m_scene);
            m_idle = 10000;
        }
        else
            spr->SetAction((VICSpriteAnimID)0, *m_scene);
    }
    UpdateCamera();
    m_scene->SetPersonalLightMode((VIPersonalLightMode)2);
    raster->Viewport(358, 48, 620, 396);
    m_scene->SetFarPlane(*m_camera);
    m_scene->Render(*m_camera);
    raster->Viewport(0, 0, 639, 447);
    m_time = app->m_p07568;
    ui->Begin2D();
    unsigned short buf[32];
    VIString::strcpy16(buf, 32, Word(ui, 406));
    VIString::strcat16(buf, 32, ": ");
    ui->DrawCenteredStringEx(buf, 490, 350, white, 0, 1);
    const VIRec16_C2D7C0 *h = sHomes;
    do
    {
        if (h->m_race == m_char->m_race && h->m_city == m_char->m_p38)
        {
            if (h->m_race == 0 && (m_char->m_class == 1 || m_char->m_class == 7))
                ui->DrawCenteredStringEx((unsigned short)602, 490,
                                         ui->m_font.GetHeight(ui->m_raster, ui->m_dict) + 345, white, 0, 1);
            else
                ui->DrawCenteredStringEx(Word(ui, h->m_cityWord), 490,
                                         ui->m_font.GetHeight(ui->m_raster, ui->m_dict) + 345, white, 0, 1);
            break;
        }
    } while (++h < sHomes + 11);
    ui->End2D();
}

// 0x00C2E5E8.  Enter creates (267), back is 260.
void VIWndCharCreateReview::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 13:
    case 134:
        app->PlaySound(13);
        if (m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
        GetParent()->OnMessage(this, 267, 0, 0);
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        if (m_actor != -1)
        {
            m_scene->ReleaseActor(m_actor);
            m_actor = -1;
        }
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    }
}

// 0x00C2E6F8.
void VIWndCharCreateReview::OnKeyUp(int key)
{
    GetAppPtr();
    VIWnd::OnKeyUp(key);
}

// 0x00C2E738.
void VIWndCharCreateReview::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    GetAppPtr();
    VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C2E7A8.  The model in the form's look, at the walk camera's committed location; a
// model the dictionary lacks falls back to HumanMale.
void VIWndCharCreateReview::CreateModel(unsigned int model)
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
    spr->SetHair(m_char->m_hairColor, m_char->m_hairLength, m_char->m_hairStyle, *m_scene);
    spr->SetFace(m_char->m_face, *m_scene);
    m_walkCam->m_parm64 = m_actor;
    m_walkCam->SetCameraMode((VIWalkCameraMode)1);
}

// 0x00C2E8C0.  The race's review camera, or the default one.
void VIWndCharCreateReview::UpdateCamera()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIRaceCamera *rc = GetRaceCamera(m_char->m_race, app->ExpansionFlags());
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
