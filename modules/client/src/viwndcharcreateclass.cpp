#line 1 "C:/code-frontiers-gm/game/VIWndCharCreateClass.cpp"
// viwndcharcreateclass.cpp -- CLIENT 0x00C240F0 .. 0x00C26E50: the class page of
// character creation.  Page 0 lists the classes the race, the city and the owned
// expansions allow (sClasses, filtered by expansion in BuildList, by race and city
// as the cursor moves); page 1 spends 20 points over the seven stats on top of the
// race's and the class's bonuses.  The parent (VIWndCharCreateBlank) gets 263 with the
// class and 265 with the stats, or 260 to go back.
#include "game.h"

// 0x00CE3490: the classes in the order the page lists them, each with the WORDS.TXT
// id of its description.
static VIRec8_C26D38 sClasses[15] =
{
    { 14, 394 }, {  5, 385 }, {  9, 389 }, {  7, 387 }, { 12, 392 },
    { 10, 390 }, {  4, 384 }, { 11, 391 }, {  2, 382 }, {  1, 381 },
    {  6, 386 }, {  3, 383 }, {  8, 388 }, {  0, 380 }, { 13, 393 },
};

// 0x00CE3508: the seven stats, each with the WORDS.TXT ids of its name and its
// description.
struct VIStatRow
{
    int m_stat;
    int m_name;
    int m_desc;
};
static VIStatRow sStats[7] =
{
    { 0, 248, 202 }, { 1, 249, 203 }, { 2, 250, 204 }, { 3, 251, 205 },
    { 4, 252, 206 }, { 5, 253, 207 }, { 6, 254, 208 },
};

// 0x00C240F0.
VIWndCharCreateClass::VIWndCharCreateClass()
{
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_sel = 0;
    m_race = 0;
    m_city = 0;
    m_points = 0;
    m_page = 0;
    m_time = 0;
    m_count = 0;
    m_class = -1;
    m_p64 = 1;
    m_p6C = 1.0f;
    memset(m_stats, 0, sizeof(m_stats));
}

// 0x00C241A0.
VIWndCharCreateClass::~VIWndCharCreateClass()
{
    Free();
}

// 0x00C24200.
int VIWndCharCreateClass::Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam)
{
    m_walkCam = &walkCam;
    m_camera = &camera;
    m_scene = &scene;
    ClearText();
    m_page = 0;
    memset(m_stats, 0, sizeof(m_stats));
    m_p64 = 1;
    m_p68 = 1;
    m_p6C = 1.0f;
    m_points = 20;
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_time = 0;
    m_list.Init(0);
    m_count = 0;
    return 0;
}

// 0x00C24290.
int VIWndCharCreateClass::Free()
{
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    m_sel = 0;
    m_race = 0;
    m_city = 0;
    m_class = -1;
    m_points = 0;
    m_page = 0;
    m_p64 = 1;
    m_p68 = 1;
    m_p6C = 1.0f;
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    return 0;
}

// 0x00C242F0.
int VIWndCharCreateClass::Reset()
{
    m_sel = 0;
    m_race = 0;
    m_city = 0;
    m_class = -1;
    m_points = 20;
    m_page = 0;
    m_p64 = 1;
    m_p68 = 1;
    m_p6C = 1.0f;
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    ClearText();
    return 0;
}

static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

static inline void DrawCentered(VIUI *ui, VIFont *font, unsigned short id, int x, int y)
{
    font->DrawCenteredString(ui->m_raster, ui->m_dict, Word(ui, id), x, y, 1);
}

// 0x00C24360.  The frame and the page title, the class list (the cursor row blue, the
// classes the race and city cannot be grey), the points left, the seven stat bars (a
// stat the class favours in the pulse colour, the arrows on the cursor row), the
// description box; then the pulse fades toward (10, 50, 70) and back.
void VIWndCharCreateClass::OnDrawSelf()
{
    char buf[8];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    long now = app->m_p07568;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    ui->Begin2D();
    VIVect2I num;
    VIVect2I tl;
    VIVect2I size;
    VIVect2I inner;
    VIVect2I at;
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, 1.0f);
    VIColor32F grey(0.35f, 0.35f, 0.35f, 1.0f);
    VIColor32F blue(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 450), (VIAlphaBlendMode)0, 1.0f);
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
    VIUIFrameParam fp;
    VIUIFrameParam frame;
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
    memset(&frame, 0, sizeof(frame));
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
    ui->BlitStuddedFrame(tl, size, frame, studs, 2, 3, (VIAlphaBlendMode)0, 1.0f, 1);
    at.Set(tl.x + size.x, tl.y + c2->m_uv1.y);
    inner.Set(340, 348);
    ui->BlitRect((VIUITexId)51, at, inner, (VIAlphaBlendMode)0, 1.0f);
    at.Set(tl.x + size.x, tl.y);
    inner.Set(inner.x + c2->m_uv1.x, inner.y + c2->m_uv1.y + c3->m_uv1.y);
    ui->BlitStuddedFrame(at, inner, frame, studs, 3, 2, (VIAlphaBlendMode)0, 1.0f, 0);
    at += VIVect2I(10, 225);
    inner.Set(320, 135);
    ui->BlitFillFrame(at, inner, fp, (VIUITexId)35, (VIAlphaBlendMode)0, 1.0f, 1);
    int word = m_page ? 603 : 258;
    VIFont *font = &ui->m_font;
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, word)) + 64;
    ui->BlitBar((unsigned short)word, white, VIVect2I((640 - w) / 2, 8), (VIUITexId)33, (VIUITexId)34, w,
                (VIAlphaBlendMode)0, 1.0f, 0, 1);
    font->SetTextColor(black);
    at.Set(50, 408);
    w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217))
      + 110 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 212));
    ui->BlitBar("", black, at, (VIUITexId)25, (VIUITexId)26, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    at += VIVect2I(20, 4);
    ui->Blit((VIUITexId)65, at);
    at.x += 25;
    DrawCentered(ui, font, 217, at.x, at.y + 5);
    at.x += 20 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217));
    ui->Blit((VIUITexId)66, at);
    at.x += 25;
    DrawCentered(ui, font, m_page == 0 ? 212 : 216, at.x, at.y + 5);
    at.Set(60, 87);
    for (int i = 0; i < m_count; i++)
    {
        ui->m_font.SetTextColor(black);
        if ((i == m_sel && m_page == 0) || (m_list[i].m_class == m_class && m_page == 1))
        {
            const VIUITexEntry *arrow = GetUITexture((VIUITexId)85);
            ui->BlitEx((VIUITexId)arrow->m_x00, at - VIVect2I(32, arrow->m_uv1.y / 2),
                       (VIAlphaBlendMode)3, m_page ? 1.0f : m_p6C, 0);
            ui->m_font.SetTextColor(yellow);
        }
        if (!IsValidRaceClassCityComboExpansion(m_race, m_list[i].m_class, m_city, app->ExpansionFlags()))
            ui->m_font.SetTextColor(grey);
        DrawCentered(ui, &ui->m_font, GetClass(m_list[i].m_class)->m_nameStrID, at.x, at.y);
        at.y += 19;
    }
    int barH = GetUITexture((VIUITexId)54)->m_uv1.y - 3;
    ui->m_font.SetTextColor(white);
    at.Set(295, 65);
    num.Set(530, 65);
    ui->BlitBar((unsigned short)255, m_color, VIVect2I(295, 65), (VIUITexId)53, (VIUITexId)55, (VIUITexId)54, 235,
                (VIAlphaBlendMode)0, 1.0f, 1, 0);
    VIString::snprintf(buf, sizeof(buf), "%u", m_points);
    ui->BlitBar(buf, m_color, num, (VIUITexId)53, (VIUITexId)55, (VIUITexId)54, 85, (VIAlphaBlendMode)0, 1.0f, 0, 0);
    at.y += barH;
    num.y += barH;
    for (unsigned int s = 0; s < 7; s++)
    {
        VIColor32F c;
        VIVect2I mark;
        int hi = 1;
        const VIUITexEntry *arrows = GetUITexture((VIUITexId)126);
        VIUITexId l, m, r;
        if ((int)s == m_sel && m_page == 1)
        {
            l = (VIUITexId)56;
            m = (VIUITexId)58;
            mark = num;
            c = yellow;
            r = (VIUITexId)57;
        }
        else
        {
            int cls = m_page == 0 ? m_list[m_sel].m_class : m_class;
            l = (VIUITexId)53;
            m = (VIUITexId)55;
            r = (VIUITexId)54;
            if (IsStatHighlighted(cls, sStats[s].m_stat))
            {
                hi = 0;
                c = m_color;
            }
            else
            {
                c = white;
                hi = 1;
            }
        }
        ui->BlitBar((unsigned short)sStats[s].m_name, c, at, l, m, r, 235, (VIAlphaBlendMode)0, 1.0f, 1, hi);
        VIString::snprintf(buf, sizeof(buf), "%u", m_stats[sStats[s].m_stat] + 60);
        ui->BlitBar(buf, c, num, l, m, r, 85, (VIAlphaBlendMode)0, 1.0f, 0, hi);
        if (mark.x > 0 && mark.y > 0)
        {
            mark.x += 1;
            mark.y += arrows->m_uv1.y / 2 - 3;
            ui->BlitEx((VIUITexId)arrows->m_x00, mark, (VIAlphaBlendMode)3, m_p6C, 0);
            mark.x = mark.x - 4 + (85 - arrows->m_uv1.x);
            ui->BlitEx((VIUITexId)arrows->m_x00, mark, (VIAlphaBlendMode)3, m_p6C, 1);
        }
        at.y += barH;
        num.y += barH;
    }
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(455, 328), 340, 7, 1);
    m_window.TextBox(m_text, 20, 0, m_window.Width() - 40, 7, black, 2);
    ui->End2D();
    if (m_p64 == -1)
    {
        m_p6C -= dt * 2.0f;
        m_p6C = m_p6C < 0.0f ? 0.0f : m_p6C;
        if (m_p6C == 0.0f)
            m_p64 = 1;
    }
    else
    {
        m_p6C += dt * 3.0f;
        m_p6C = VIMIN(m_p6C, 1.0f);
        if (m_p6C == 1.0f)
            m_p64 = -1;
    }
    if (m_p68 == -1)
    {
        m_color.r -= dt;
        m_color.r = m_color.r < blue.r ? blue.r : m_color.r;
        m_color.g -= dt;
        m_color.g = m_color.g < blue.g ? blue.g : m_color.g;
        m_color.b -= dt;
        m_color.b = m_color.b < blue.b ? blue.b : m_color.b;
        if (m_color.r == blue.r && m_color.g == blue.g && m_color.b == blue.b)
            m_p68 = 1;
    }
    else
    {
        m_color.r += dt * 2.0f;
        m_color.r = VIMIN(m_color.r, 1.0f);
        m_color.g += dt * 2.0f;
        m_color.g = VIMIN(m_color.g, 1.0f);
        m_color.b += dt * 2.0f;
        m_color.b = VIMIN(m_color.b, 1.0f);
        if (m_color.r == 1.0f && m_color.g == 1.0f && m_color.b == 1.0f)
            m_p68 = -1;
    }
}

// 0x00C253A8.
void VIWndCharCreateClass::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int total = 0;
    switch (key)
    {
    case 13:
    case 134:   // pick
        app->PlaySound(13);
        if (m_page == 0)
        {
            m_class = m_list[m_sel].m_class;
            GetParent()->OnMessage(this, 263, m_list[m_sel].m_class, 0);
            Pulse();
            m_sel = 0;
            m_points = 20;
            m_page = 1;
            UpdateText();
        }
        else if (m_page == 1)
        {
            const VIRaceDef *race = GetRace(m_race);
            const VIClassDef *cls = GetClass(m_class);
            VINewChar c;
            for (int i = 0; i < 7; i++)
            {
                int s = sStats[i].m_stat;
                c.m_stats[s] = m_stats[s] - race->m_bonus[s] - cls->m_bonus[s];
                total += c.m_stats[s];
            }
            if (total <= 20)
            {
                m_sel = 0;
                m_page = 0;
                m_points = 20;
                Move(-1, 0);
                UpdateText();
                GetParent()->OnMessage(this, 265, (int)&c, 0);
            }
        }
        break;
    case 27:
    case 132:   // back
        app->PlaySound(8);
        if (m_page == 0)
        {
            m_sel = 0;
            m_points = 20;
            UpdateText();
            memset(m_stats, 0, sizeof(m_stats));
            GetParent()->OnMessage(this, 260, 0, 0);
        }
        else if (m_page == 1)
        {
            m_page = 0;
            Pulse();
            m_sel = 0;
            m_points = 20;
            UpdateText();
            for (int i = 0; i < m_count; i++)
                if (m_list[i].m_class == m_class)
                {
                    m_sel = i;
                    UpdateText();
                    break;
                }
        }
        break;
    case 19:
    case 138:   // up
        Move(-1, 1);
        break;
    case 20:
    case 139:   // down
        Move(1, 1);
        break;
    case 17:
    case 136:   // left
        if (m_page == 1)
            AdjustStat(-1);
        break;
    case 18:
    case 137:   // right
        if (m_page == 1)
            AdjustStat(1);
        break;
    }
}

// 0x00C25718.
void VIWndCharCreateClass::OnKeyUp(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIWnd::OnKeyUp(key);
}

// 0x00C25758.  Page 0 skips the classes the race and city cannot be.
void VIWndCharCreateClass::Move(int delta, int sound)
{
    VIClient *app = (VIClient *)GetAppPtr();
    Pulse();
    if (m_page == 0)
    {
        if (m_sel + delta < 0)
        {
            if (sound)
                app->PlaySound(9);
            m_sel = 0;
        }
        else if (m_sel + delta >= m_count)
        {
            if (sound)
                app->PlaySound(9);
            m_sel = m_count - 1;
        }
        else
        {
            if (sound)
                app->PlaySound(11);
            m_sel = m_sel + delta;
        }
        while (!IsValidRaceClassCityComboExpansion(m_race, m_list[m_sel].m_class, m_city,
                                                   app->ExpansionFlags()))
        {
            if (m_sel == 0)
                delta = 1;
            else if (m_sel == m_count - 1)
                delta = -1;
            m_sel = m_sel + delta;
        }
    }
    else if (m_page == 1)
    {
        if (m_sel + delta < 0)
        {
            if (sound)
                app->PlaySound(9);
            m_sel = 0;
        }
        else if (m_sel + delta >= 7)
        {
            if (sound)
                app->PlaySound(9);
            m_sel = 6;
        }
        else
        {
            if (sound)
                app->PlaySound(11);
            m_sel = m_sel + delta;
        }
    }
    UpdateText();
}

// 0x00C25910.  295: this page was opened.
void VIWndCharCreateClass::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (msg == 295)
    {
        if ((VIWndCharCreateClass *)p2 == this)
        {
            m_p64 = 1;
            m_p6C = 1.0f;
            m_time = app->m_p07568;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(455, 330), 340, 6, 1);
            m_window.Open(true);
            BuildList();
            Refresh();
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00C25A18.
void VIWndCharCreateClass::SetRace(int race, int city)
{
    m_page = 0;
    m_race = race;
    m_city = city;
    m_class = -1;
    BuildList();
    Refresh();
}

// 0x00C25A58.  The cursor onto the form's class, or the first one allowed.
void VIWndCharCreateClass::Refresh()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_race < 0 || m_city < 0)
        return;
    VIWndCharCreateBlank *form = (VIWndCharCreateBlank *)GetParent();
    m_sel = 0;
    for (int i = 0; i < m_count; i++)
        if (form->GetClass() == m_list[i].m_class)
        {
            m_sel = i;
            break;
        }
    if (!IsValidRaceClassCityComboExpansion(m_race, m_list[m_sel].m_class, m_city,
                                            app->ExpansionFlags()))
        Move(1, 0);
    UpdateText();
}

// 0x00C25B48.  The race's creation camera, or a default.
void VIWndCharCreateClass::ApplyCamera()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIRaceCamera *rc = GetRaceCamera(m_race, app->ExpansionFlags());
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

// 0x00C25C38.
void VIWndCharCreateClass::ClearText()
{
    m_text[0] = 0;
}

// 0x00C25C40.
void VIWndCharCreateClass::UpdateText()
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    ClearText();
    if (m_page == 0)
    {
        VIString::strcpy16(m_text, sizeof(m_text),
                           (const unsigned short *)ui->m_words.GetString(m_list[m_sel].m_word, ui->m_language));
        SetBaseStats(m_list[m_sel].m_class);
    }
    else if (m_page == 1)
        VIString::strcpy16(m_text, sizeof(m_text),
                           (const unsigned short *)ui->m_words.GetString(sStats[m_sel].m_desc, ui->m_language));
}

// 0x00C25D30.
void VIWndCharCreateClass::SetPage(int page)
{
    m_page = page;
}

// 0x00C25D38.
void VIWndCharCreateClass::SetBaseStats(int cls)
{
    const VIRaceDef *race = GetRace(m_race);
    const VIClassDef *c = GetClass(cls);
    for (int i = 0; i < 7; i++)
    {
        int s = sStats[i].m_stat;
        m_stats[s] = race->m_bonus[s] + c->m_bonus[s];
    }
}

// 0x00C25DC8.  The stat row moves by one point; never below the bonuses.
void VIWndCharCreateClass::AdjustStat(int delta)
{
    VIClient *app = (VIClient *)GetAppPtr();
    const VIRaceDef *race = GetRace(m_race);
    const VIClassDef *cls = GetClass(m_class);
    Pulse();
    if (m_points <= 0 && delta > 0)
    {
        app->PlaySound(9);
        return;
    }
    if (m_points - delta > 20)
    {
        app->PlaySound(9);
        return;
    }
    int s = sStats[m_sel].m_stat;
    int base = race->m_bonus[s] + cls->m_bonus[s];
    if (delta < 0 && m_stats[s] == base)
        return;
    if (m_stats[s] + delta < base)
        return;
    app->PlaySound(11);
    m_points = m_points - delta;
    m_stats[sStats[m_sel].m_stat] = m_stats[sStats[m_sel].m_stat] + delta;
}

// 0x00C25F28.
void VIWndCharCreateClass::Pulse()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_p64 = 1;
    m_p6C = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C25F68.  The classes an owned expansion allows.
void VIWndCharCreateClass::BuildList()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_list.Init(15);
    m_count = 0;
    for (int i = 0; i < 15; i++)
    {
        const VIClassDef *c = GetClass(sClasses[i].m_class);
        if ((app->ExpansionFlags() & c->m_expansion) == c->m_expansion)
        {
            m_list[m_count] = sClasses[i];
            m_count++;
        }
    }
}
