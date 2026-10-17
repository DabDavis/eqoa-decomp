#line 1 "C:/code-frontiers-gm/game/VIWndCharCustFrontiers.cpp"
// VIWndCharCustFrontiers -- CLIENT 0x00C2F5E8 .. 0x00C31F00: re-styling an existing character
// from character select (cl_wc.h).
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// One word centred at (x, y).
static inline void DrawWord(VIUI *ui, VIFont *font, int id, int x, int y)
{
    font->DrawCenteredString(ui->m_raster, ui->m_dict, Word(ui, id), x, y, 0);
}

// 0x00C2F5E8.
VIWndCharCustFrontiers::VIWndCharCustFrontiers()
{
    m_camera = 0;
    m_scene = 0;
    m_walkCam = 0;
    m_app = 0;
    m_sel = 0;
    m_actor = -1;
    m_charIndex = -1;
}

// 0x00C2F668.
int VIWndCharCustFrontiers::Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam)
{
    m_app = (VIClient *)GetAppPtr();
    m_scene = &scene;
    m_camera = &camera;
    m_walkCam = &walkCam;
    return 0;
}

// 0x00C2F6C8.  The character in slot i, at the select scene's spot for it.
int VIWndCharCustFrontiers::SetChar(int i, const VIVect3 &pos, const VIVect3 &hpr)
{
    m_charIndex = i;
    m_pos = pos;
    m_hpr = hpr;
    VICharRecord *c = m_app->GetChar(m_charIndex);
    m_face = c->m_face;
    m_hairStyle = c->m_hairStyle;
    m_hairLength = c->m_hairLength;
    m_hairColor = c->m_hairColor;
    m_walkCam->Init(pos, VIVect3(), *m_camera, *m_scene);
    SetSceneCameraLimits(m_walkCam, 0.5f, 25.0f, 3.5f);
    m_walkCam->SetCameraMode((VIWalkCameraMode)1);
    return 0;
}

// 0x00C2F7B8.  The title bar and the two columns over the select scene, then the model in its
// own viewport under a warm personal light.
void VIWndCharCustFrontiers::OnDrawSelf()
{
    VIClient *app = m_app;
    int y = 8;
    VIUI *ui = &app->m_ui;
    ui->Begin2D();
    VIFont *font = &app->m_ui.m_font;
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 450), (VIAlphaBlendMode)0, 1.0f);
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 259)) + 64;
    ui->BlitBar((unsigned short)259, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), VIVect2I((640 - w) / 2, y),
                (VIUITexId)33, (VIUITexId)34, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    DrawRows();
    DrawValues();
    ui->End2D();
    if (m_actor == -1)
    {
        CreateModel();
        if (m_actor == -1)
            return;
    }
    ((VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor)))->SetAction((VICSpriteAnimID)0, *m_scene);
    UpdateCamera();
    VIColor32F light(0.75f, 0.69f, 0.36f, 1.0f);
    VIVect3 at = m_walkCam->m_last.m_cameraPos;
    m_scene->SetPersonalLightMode((VIPersonalLightMode)1);
    m_scene->SetPersonalLight(at, 5.0f, light);
    VIRaster *r = &m_app->m_raster;
    r->Viewport(358, 48, 620, 396);
    m_scene->SetFarPlane(*m_camera);
    m_scene->Render(*m_camera);
    r->Viewport(0, 0, 639, 447);
}

// 0x00C2FA28.  Enter keeps the new look, back drops it; up/down pick the row, left/right
// change it.
void VIWndCharCustFrontiers::OnKeyDown(int key)
{
    switch (key)
    {
    case 27:
    case 132:
        m_app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 13:
    case 134:
        Accept();
        break;
    case 17:
    case 136:
        Prev();
        break;
    case 18:
    case 137:
        Next();
        break;
    case 19:
    case 138:
        Up();
        break;
    case 20:
    case 139:
        Down();
        break;
    }
}

// 0x00C2FB78.  295 moves the focus from p1 to p2: gaining it opens the window, losing it
// drops the model.
void VIWndCharCustFrontiers::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if (p2 == (int)this && p1 != (int)this)
        {
            m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(224, 320), 500, 1, 1);
            m_window.Open(true);
            m_sel = 0;
            m_p1BC = 0;
            m_p1BD = 15;
            Release();
        }
        if (p1 == (int)this && p2 != (int)this)
            Release();
    }
}

// 0x00C2FC50.
void VIWndCharCustFrontiers::UpdateCamera()
{
    int exp = m_app->ExpansionFlags();
    VIRaceCamera *rc = GetRaceCamera(m_app->GetChar(m_charIndex)->m_race, exp);
    if (rc)
    {
        m_walkCam->m_targetHeight = rc->m_custHeight;
        m_walkCam->m_lookAtHeightAbove = rc->m_custLookAt;
        m_walkCam->SetDefOrbitPitchAngle(rc->m_custPitch);
        m_walkCam->SetOrbitDistance(rc->m_custDistance);
    }
    m_walkCam->Process(*m_camera, *m_scene, 0);
}

// 0x00C2FCF0.  The hair rows turn the model a twelfth of a turn.
void VIWndCharCustFrontiers::Down()
{
    if (m_sel >= 3)
    {
        m_sel = 3;
        m_app->PlaySound(9);
    }
    else
    {
        m_sel = m_sel + 1;
        m_app->PlaySound(11);
    }
    m_window.ResetTickPulse();
    if (m_sel == 2 || m_sel == 3)
        m_walkCam->SetOrbitAngle(0.5235988f);
    else
        m_walkCam->SetOrbitAngle(0.0f);
}

// 0x00C2FD90.
void VIWndCharCustFrontiers::Up()
{
    if (m_sel <= 0)
    {
        m_sel = 0;
        m_app->PlaySound(9);
    }
    else
    {
        m_sel = m_sel - 1;
        m_app->PlaySound(11);
    }
    m_window.ResetTickPulse();
    if (m_sel == 2 || m_sel == 3)
        m_walkCam->SetOrbitAngle(0.5235988f);
    else
        m_walkCam->SetOrbitAngle(0.0f);
}

// 0x00C2FE28.  The row's previous value, wrapping.
void VIWndCharCustFrontiers::Prev()
{
    VICSprite *spr = (VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor));
    m_app->PlaySound(11);
    switch (m_sel)
    {
    case 0:
        m_face = m_face ? m_face - 1 : 3;
        spr->SetFace(m_face, *m_scene);
        break;
    case 1:
        m_hairStyle = m_hairStyle ? m_hairStyle - 1 : 3;
        spr->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
        break;
    case 2:
        m_hairLength = m_hairLength ? m_hairLength - 1 : 3;
        spr->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
        break;
    case 3:
        m_hairColor = m_hairColor ? m_hairColor - 1 : 7;
        spr->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
        break;
    }
    m_window.ResetTickPulse();
}

// 0x00C2FF78.  The row's next value, wrapping.
void VIWndCharCustFrontiers::Next()
{
    VICSprite *spr = (VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor));
    m_app->PlaySound(11);
    switch (m_sel)
    {
    case 0:
        m_face = m_face == 3 ? 0 : m_face + 1;
        spr->SetFace(m_face, *m_scene);
        break;
    case 1:
        m_hairStyle = m_hairStyle == 3 ? 0 : m_hairStyle + 1;
        spr->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
        break;
    case 2:
        m_hairLength = m_hairLength == 3 ? 0 : m_hairLength + 1;
        spr->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
        break;
    case 3:
        m_hairColor = m_hairColor == 7 ? 0 : m_hairColor + 1;
        spr->SetHair(m_hairColor, m_hairLength, m_hairStyle, *m_scene);
        break;
    }
    m_window.ResetTickPulse();
}

// 0x00C300C8.
void VIWndCharCustFrontiers::Accept()
{
    m_app->PlaySound(13);
    m_app->SetCharLook(m_charIndex, m_face, m_hairStyle, m_hairLength, m_hairColor);
    GetParent()->OnMessage(this, 260, 0, 0);
}

// 0x00C30138.  The character's model as character select draws it: armour, hair, face, robe
// and the three held items, turned a little off the camera.
void VIWndCharCustFrontiers::CreateModel()
{
    Release();
    if (m_charIndex < 0 || m_charIndex >= m_app->GetCharCount())
        return;
    VICharRecord *c = m_app->GetChar(m_charIndex);
    VIDictionary *dict = &m_app->m_dict;
    VIResourceType type;
    int idx;
    if (!dict->Find(c->m_modelId, type, idx) && !dict->Find(VIHashResourceID("HumanMale"), type, idx))
        return;
    m_actor = m_scene->CreateActor(m_pos, m_hpr, 1.0f, idx);
    if (m_actor != -1)
    {
        VICSprite *spr = (VICSprite *)m_scene->Sprite(m_scene->GetActorSprite(m_actor));
        if (spr)
        {
            spr->SetAction((VICSpriteAnimID)0, *m_scene);
            int i;
            for (i = 0; i < 6; i++)
                if (i != 5)
                    spr->SetArmorSlot((VICSpriteArmorSlot)i, (VICSpriteArmorSet)c->m_armor[i],
                                      (VICSpriteTint)c->m_p59[i], *m_scene);
            spr->SetHair(c->m_hairColor, c->m_hairLength, c->m_hairStyle, *m_scene);
            spr->SetFace(c->m_face, *m_scene);
            if (c->m_robe != -1)
                spr->SetRobe(1, c->m_robe, (VICSpriteTint)c->m_p40, *m_scene);
            VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
            spr->SetWeaponTrail(0, white);
            for (i = 0; i < 3; i++)
            {
                if (c->m_weapons[i] && (dict->Find(c->m_weapons[i], type, idx) ||
                                        dict->Find(VIHashResourceID("!Item"), type, idx)))
                    spr->AttachItem(idx, (VICSpriteAttachSlot)i, (VICSpriteAttackType)c->m_p50[i], -1, *m_scene);
            }
        }
    }
    m_walkCam->m_parm64 = m_actor;
    m_walkCam->SetCameraMode((VIWalkCameraMode)1);
    const VIVect3 &hpr = m_scene->GetActorHPR(m_actor);
    m_scene->SetActorHPR(m_actor, VIVect3(hpr.x + 5.934119f, hpr.y, hpr.z));
}

// 0x00C30410.
void VIWndCharCustFrontiers::Release()
{
    if (m_actor != -1)
    {
        m_scene->ReleaseActor(m_actor);
        m_actor = -1;
    }
}

// 0x00C30458.  The panel the rows sit in -- a filled frame of the four corner pieces with two
// studded frames over it -- the gem corners of the model's box, and the legend bar.
void VIWndCharCustFrontiers::DrawRows()
{
    VIVect2I tl;
    VIVect2I size;
    VIVect2I inner;
    VIVect2I at;
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIClient *app = m_app;
    const VIUITexEntry *c0 = GetUITexture((VIUITexId)44);
    VIUI *ui = &app->m_ui;
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
    const VIUITexEntry *gem = GetUITexture((VIUITexId)59);
    if (!gem)
        return;
    int x = 285;
    int y = 37;
    ui->BlitEx((VIUITexId)59, VIVect2I(x, y), (VIAlphaBlendMode)0, 1.0f, 0);
    VIFont *font = &app->m_ui.m_font;
    VIUIWindow *win = &m_window;
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
    fp.m_br.m_flags = 1;
    fp.m_right.m_flags = 1;
    fp.m_bottom.m_id = (VIUITexId)40;
    fp.m_bl.m_id = (VIUITexId)39;
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
    at.Set(50, 408);
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217))
          + 110 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 218));
    ui->BlitBar("", black, at, (VIUITexId)25, (VIUITexId)26, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    win->Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(at.x + (w >> 1), at.y + 12), w, 1, 0);
    win->Open(true);
    unsigned short text[64];
    VIString::snprintf16(text, sizeof(text), "|x%S   |t%S", Word(ui, 1779), Word(ui, 217));
    win->TextBox(text, 20, 0, win->Width() - 40, 1, (VIUICOLOR)0, 0);
}

// 0x00C30A78.  The four rows -- each a bracket pair, its icon, the selection's pulse and its
// word -- the selection bar, and the four values as sliders.
void VIWndCharCustFrontiers::DrawValues()
{
    VIClient *app = m_app;
    VIFont *font = &app->m_ui.m_font;
    VIUI *ui = &app->m_ui;
    font->SetTextColor(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
    VIVect2I base(27, 92);
    VIVect2I pos = base;
    ui->BlitEx((VIUITexId)135, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    pos.x += 120;
    ui->BlitEx((VIUITexId)135, pos, (VIAlphaBlendMode)0, 1.0f, 1);
    pos = base + VIVect2I(95, -40);
    ui->BlitEx((VIUITexId)131, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel == 0)
        ui->BlitEx((VIUITexId)206, pos + VIVect2I(9, 15), (VIAlphaBlendMode)3, m_window.TickPulse(), 0);
    pos += VIVect2I(25, 60);
    DrawWord(ui, font, 604, pos.x, pos.y);

    base.y += 84;
    pos = base;
    ui->BlitEx((VIUITexId)135, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    pos.x += 120;
    ui->BlitEx((VIUITexId)135, pos, (VIAlphaBlendMode)0, 1.0f, 1);
    pos = base + VIVect2I(95, -40);
    ui->BlitEx((VIUITexId)132, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel == 1)
        ui->BlitEx((VIUITexId)207, pos + VIVect2I(23, 25), (VIAlphaBlendMode)3, m_window.TickPulse(), 0);
    pos += VIVect2I(25, 60);
    DrawWord(ui, font, 605, pos.x, pos.y);

    base.y += 84;
    pos = base;
    ui->BlitEx((VIUITexId)135, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    pos.x += 120;
    ui->BlitEx((VIUITexId)135, pos, (VIAlphaBlendMode)0, 1.0f, 1);
    pos = base + VIVect2I(95, -40);
    ui->BlitEx((VIUITexId)133, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel == 2)
        ui->BlitEx((VIUITexId)208, pos + VIVect2I(1, 2), (VIAlphaBlendMode)3, m_window.TickPulse(), 0);
    pos += VIVect2I(25, 60);
    DrawWord(ui, font, 606, pos.x, pos.y);

    base.y += 84;
    pos = base;
    ui->BlitEx((VIUITexId)135, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    pos.x += 120;
    ui->BlitEx((VIUITexId)135, pos, (VIAlphaBlendMode)0, 1.0f, 1);
    pos = base + VIVect2I(95, -35);
    ui->BlitEx((VIUITexId)134, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    if (m_sel == 3)
        ui->BlitEx((VIUITexId)209, pos + VIVect2I(5, 6), (VIAlphaBlendMode)3, m_window.TickPulse(), 0);
    pos += VIVect2I(25, 56);
    DrawWord(ui, font, 607, pos.x, pos.y);

    pos = VIVect2I(73, m_sel * 83 + 75);
    ui->BlitEx((VIUITexId)85, pos, (VIAlphaBlendMode)0, 1.0f, 0);
    pos.x += 125;
    ui->BlitEx((VIUITexId)85, pos, (VIAlphaBlendMode)0, 1.0f, 1);
    int cx = 142, cy = 240;
    m_window.Init(VIWnd::UI(), VIVect2I(0, 0), VIVect2I(cx, cy), 280, 16, 1);
    m_window.DrawHScroll(30, 1, m_window.Width() - 60, (float)m_face / 3.0f, 0);
    m_window.DrawHScroll(30, 6, m_window.Width() - 60, (float)m_hairStyle / 3.0f, 0);
    m_window.DrawHScroll(30, 11, m_window.Width() - 60, (float)m_hairLength / 3.0f, 0);
    m_window.DrawHScroll(30, 16, m_window.Width() - 60, (float)m_hairColor / 7.0f, 0);
}
