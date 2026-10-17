#line 1 "C:/code-frontiers-gm/game/VIWndFishing.cpp"
// VIWndFishing.cpp -- CLIENT 0x00C4BB28 .. 0x00C4E5F8: the fishing window (cl_we.h)
// and the cast's splash effect, a file-scope emitter that LoadTargetEffect /
// ReleaseTargetEffect set up and tear down with the world.  Names ours.
#include "game.h"

int gTargetEffectDef = -1;       // 0x00CD75A0: the particle definition, -1 none
int gTargetEffectTex = -1;       // 0x00CD75A4: its texture
static VIParticleEmitter sTargetEmitter;   // 0x00CD75B0

// 0x00C4BB28.
VIWndFishing::VIWndFishing()
{
    m_list = 0;
    m_edit = 0;
    m_chat = 0;
    m_desc[0] = 0;
    m_text[0] = 0;
    m_textEnd = 0;
    m_scene = 0;
}

// 0x00C4BBA8.
VIWndFishing::~VIWndFishing()
{
    Free();
}

// 0x00C4BC20.
int VIWndFishing::Init(VIScene *scene)
{
    m_list = new VIWndList;
    if (!m_list)
        return -1;
    m_list->SetAppPtr(GetAppPtr());
    m_list->SetVisible(false);
    m_list->Init(42);
    AddChild(m_list);
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->SetVisible(false);
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    m_info.SetClient((VIClient *)GetAppPtr());
    m_desc[0] = 0;
    m_text[0] = 0;
    m_textEnd = 0;
    ResetCast();
    m_scene = scene;
    return 0;
}

// 0x00C4BD18.
int VIWndFishing::Free()
{
    if (m_list)
    {
        RemoveChild(m_list);
        delete m_list;
        m_list = 0;
    }
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    m_scene = 0;
    return 0;
}

// 0x00C4BDB0.  The bait list, the cast, the legend, the message box, the bait's
// description and the power meter.
void VIWndFishing::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI &ui = app->m_ui;
    VIClientPart3FC10 *part = app->GetP3FC10();
    m_list->SetPos(VIVect2I(10, 58));
    FillBait();
    if (m_casting)
    {
        Tick();
        if (!m_casting)
        {
            if (m_hit)
                part->Cast(m_p7E0);
            else
                SetText((const unsigned short *)ui.m_words.GetString(1421, ui.m_language));
        }
    }
    else if (part->m_state == 1 && app->m_p07190.m_keys[3] > 0.25f)
        Cast();
    unsigned short back[32];
    unsigned short cast[32];
    VIString::snprintf16(back, sizeof(back), "|t%S", ui.m_words.GetString(161, ui.m_language));
    if (m_casting || part->m_state == 2)
        VIString::snprintf16(cast, sizeof(cast), "|-|x%S", ui.m_words.GetString(1438, ui.m_language));
    else
        VIString::snprintf16(cast, sizeof(cast), "|x%S", ui.m_words.GetString(1438, ui.m_language));
    ui.Begin2D();
    m_window.Draw();
    m_window.TextBox(cast, 20, 9, m_window.Width() - 40, 1, (VIUICOLOR)0, 0);
    m_window.TextBox(back, 20, 10, m_window.Width() - 40, 1, (VIUICOLOR)0, 0);
    if (app->m_p07568 < m_textEnd)
    {
        m_win3.SetTitle(m_text);
        m_win3.Open(false);
    }
    else
    {
        m_textEnd = 0;
        m_text[0] = 0;
        m_win3.Close(false);
    }
    if (m_win3.IsOpen() || m_win3.IsOpening() || m_win3.IsClosing())
        m_win3.Draw();
    if (VIString::strlen16(m_desc) == 0)
        m_win2.Close(true);
    else
    {
        VIUIWindow &win = m_win2;
        int lines = win.CountLines(m_desc, win.Width() - 40);
        win.Init(UI(), VIVect2I(0, 0), VIVect2I(340, 185), 340, lines, 1);
        win.Open(false);
        win.Draw();
        win.TextBox(m_desc, 20, 0, win.Width() - 40, lines,
                    VIColor32F(1.0f, 1.0f, 1.0f, win.m_open), 3);
    }
    {
        VIRaster &r = app->m_raster;
        r.SetSolidFillUIMaterial();
        r.BeginTriFan();
        VIColor32F c(0.0f, 0.0f, 0.0f, 1.0f);
        r.Color(c);
        r.Vertex(VIVect2I(64, 144));
        r.Vertex(VIVect2I(85, 144));
        r.Vertex(VIVect2I(85, 265));
        r.Vertex(VIVect2I(64, 265));
        r.EndTriFan();
        r.BeginTriFan();
        c.Set(0.0f, 1.0f, 0.0f, 1.0f);
        r.Color(c);
        r.Vertex(VIVect2I(65, 145));
        r.Vertex(VIVect2I(84, 145));
        r.Vertex(VIVect2I(84, 264));
        r.Vertex(VIVect2I(65, 264));
        r.EndTriFan();
        r.BeginTriFan();
        c.Set(1.0f, 1.0f, 0.0f, 1.0f);
        r.Color(c);
        r.Vertex(VIVect2I(65, 185));
        r.Vertex(VIVect2I(84, 185));
        r.Vertex(VIVect2I(84, 224));
        r.Vertex(VIVect2I(65, 224));
        r.EndTriFan();
        r.BeginTriFan();
        c.Set(1.0f, 0.0f, 0.0f, 1.0f);
        r.Color(c);
        r.Vertex(VIVect2I(65, 200));
        r.Vertex(VIVect2I(84, 200));
        r.Vertex(VIVect2I(84, 209));
        r.Vertex(VIVect2I(65, 209));
        r.EndTriFan();
        if (m_power)
        {
            r.SetSolidFillUIAVGMaterial();
            r.BeginTriFan();
            c.Set(0.0f, 0.0f, 1.0f, 0.7f);
            r.Color(c);
            r.Vertex(VIVect2I(65, 264 - m_power));
            r.Vertex(VIVect2I(84, 264 - m_power));
            c.Set(1.0f, 1.0f, 1.0f, 0.7f);
            r.Color(c);
            r.Vertex(VIVect2I(84, 264));
            r.Vertex(VIVect2I(65, 264));
            r.EndTriFan();
        }
    }
    ui.End2D();
}

// 0x00C4C4C0.
void VIWndFishing::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3FC10 *part = app->GetP3FC10();
    switch (key)
    {
    case 32:
    case 134:
        if (part->m_state == 1)
        {
            if (!m_casting)
            {
                app->PlaySound(11);
                Cast();
            }
            else
                m_release = 1;
        }
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        if (part->m_state == 3)
            part->Pull();
        else
            part->Cancel();
        break;
    case 13:
    case 147:
        if (part->m_state != 3)
            if (part->m_state != 4)
                Rename();
        break;
    case 67:
    case 99:
    case 143:
        if (part->m_state != 3)
            if (part->m_state != 4)
                ChatOn();
        break;
    case 19:
    case 20:
    case 138:
    case 139:
        if (m_chat && m_chatOn)
        {
            m_chat->OnKeyDown(key);
            m_chatKey = key;
        }
        break;
    }
}

// 0x00C4C6C8.
void VIWndFishing::OnKeyUp(int key)
{
    switch (key)
    {
    case 67:
    case 99:
    case 143:
        ChatOff();
        break;
    case 19:
    case 20:
    case 138:
    case 139:
        if (m_chat && m_chatOn)
        {
            m_chat->OnKeyUp(key);
            m_chatKey = 0;
        }
        break;
    }
}

// 0x00C4C788.
void VIWndFishing::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3FC10 *part = app->GetP3FC10();
    VIItemList &bait = part->m_catch;
    int count = bait.ItemCount;
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p1 != this && (VIWnd *)p2 == this && (VIWnd *)p1 != m_list && (VIWnd *)p1 != m_edit)
        {
            app->PlaySound(15);
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(75, 224), 125, 11, 0);
            m_window.SetTitle(m_window.T(1205));
            m_window.Close(true);
            m_window.Open(false);
            m_win3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, m_window.TopLeft().y + 22), VIVect2I(-1, -1), 1);
            m_win3.Close(true);
            m_desc[0] = 0;
            m_text[0] = 0;
            m_textEnd = 0;
            m_list->SetVisible(false);
            m_edit->SetVisible(false);
            ResetCast();
            m_chatOn = 0;
            m_chatKey = 0;
        }
        else if ((VIWnd *)p1 == this && (VIWnd *)p2 != this && (VIWnd *)p2 != m_list && (VIWnd *)p2 != m_edit)
        {
            m_chatOn = 0;
            m_chatKey = 0;
        }
        break;
    case 256:
        if (w == m_list)
        {
            app->PlaySound(13);
            if (p1 == 0)
                part->Reel();
            else if (p1 < count + 1)
            {
                int slot = p1 - 1;
                if (slot < count)
                    part->Keep(slot, bait.Item(slot)->Quantity);
            }
            else if (p1 == count + 1)
                part->Pull();
        }
        break;
    case 260:
        if (w == m_edit)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 259:
        Renamed();
        break;
    }
}

// 0x00C4CA90.
void VIWndFishing::ResetCast()
{
    m_casting = 0;
    m_power = 0;
    m_ready = 1;
    m_release = 0;
    m_splashing = 0;
}

// 0x00C4CAB0.
void VIWndFishing::Cast()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3FC10 *part = app->GetP3FC10();
    m_casting = 1;
    m_castTime = app->m_p07568;
    m_power = 120;
    m_ready = 0;
    m_release = 0;
}

// 0x00C4CB18.  The power meter falls for a second, then rises; a release (or the
// stick pushed) throws.  The splash plays for half a second after a second.
void VIWndFishing::Tick()
{
    VIClient *app = (VIClient *)GetAppPtr();
    long dt = app->m_p07568 - m_castTime;
    VIClientPart07190 &pad = app->m_p07190;
    if (dt > 500)
    {
        float t;
        if (dt >= 501 && dt < 1501)
        {
            t = 1.0f - (float)(dt - 500) / 1000.0f;
            m_power = (int)(t * 120.0f);
        }
        else if (dt >= 1501 && dt < 2500)
        {
            if (!m_ready)
            {
                t = (float)(dt - 1500) / 1000.0f;
                m_power = (int)(t * 120.0f);
                if (m_release || pad.m_keys[3] < -0.25f)
                {
                    m_ready = 1;
                    Splash();
                }
            }
        }
        else if (!m_ready)
        {
            m_power = 120;
            m_ready = 1;
            Splash();
        }
    }
    if (m_splashing)
    {
        long d = app->m_p07568 - m_splashTime;
        if (d > 1000)
        {
            if (d >= 1001 && d < 1501)
            {
                if (m_onWater)
                    sTargetEmitter.Update();
            }
            else if (!(d >= 1501 && d < 2501))
            {
                m_splashing = 0;
                m_casting = 0;
            }
        }
    }
    m_release = 0;
}

// 0x00C4CCB8.
void VIWndFishing::Splash()
{
    int actor = -1;
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_power - 56 < 8u)
        m_p7E0 = 0;
    else if (m_power - 41 < 38u)
        m_p7E0 = 5;
    else
        m_p7E0 = 15;
    app->PlayAction(65);
    if (app->m_selfId != 0xFFFFFFFF)
    {
        VIEntity *e = app->FindEntity(app->m_selfId);
        if (e->m_actor != -1)
            actor = e->m_actor;
    }
    VIVect3 pos = app->m_walkCam.m_committed.GetSmoothLocation();
    m_hit = FindWater(actor, pos, app->m_walkCam.m_committed.m_angles, m_scene, m_target);
    m_onWater = m_hit;
    if (gTargetEffectDef == -1)
        m_onWater = 0;
    if (m_onWater)
    {
        VIMatrix33 m;
        m.Identity();
        sTargetEmitter.Purge();
        sTargetEmitter.SetLocation(m_target);
        sTargetEmitter.SetLocation(m_target);
        sTargetEmitter.SetDirection(m);
    }
    m_splashing = 1;
    m_splashTime = app->m_p07568;
}

// 0x00C4CE20.  The bait list: a heading, the baits, a last row, then empty rows.
void VIWndFishing::FillBait()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart3FC10 *part = app->GetP3FC10();
    int n = part->m_catch.ItemCount;
    if (n > 0)
        m_list->SetItem(0, 2, -1, -1, 118, 0, -1);
    else
        m_list->SetItem(0, 2, -1, -1, 641, 0, -1);
    int i;
    for (i = 0; i < n; i++)
    {
        VINetItem *it = part->m_catch.Item(i);
        int type = 1;
        if (it->Quantity >= 2)
            type = 5;
        m_list->SetItem(i + 1, type, app->ItemIcon(it), -1, -1, it->Quantity, -1);
    }
    if (n > 0)
        m_list->SetItem(n + 1, 2, -1, -1, 119, 0, -1);
    else
        m_list->SetItem(n + 1, 0, -1, -1, -1, 0, -1);
    for (i = n + 2; i < 42; i++)
        m_list->SetItem(i, 0, -1, -1, -1, 0, -1);
    Describe();
}

// 0x00C4D010.
void VIWndFishing::OpenBait()
{
    VIClient *app = (VIClient *)GetAppPtr();
    ChatOff();
    m_edit->SetVisible(false);
    m_list->ResetScroll();
    m_list->Open();
    m_list->SetVisible(true);
    m_desc[0] = 0;
    m_win2.Init(UI(), VIVect2I(0, 0), VIVect2I(460, 152), 340, 10, 1);
    if (m_win2.IsOpen())
        m_win2.Close(true);
    m_win2.Open(false);
    app->PlaySound(15);
    app->SetActiveWindow(m_list);
}

// 0x00C4D100.
void VIWndFishing::CloseBait()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_list->ResetScroll();
    m_list->SetVisible(false);
    app->SetActiveWindow(this);
    m_desc[0] = 0;
}

// 0x00C4D148.
void VIWndFishing::Describe()
{
    VIItemList &items = ((VIClient *)GetAppPtr())->GetP3FC10()->m_catch;
    int count = items.ItemCount;
    m_desc[0] = 0;
    if (count > 0)
    {
        int slot = m_list->GetSel() - 1;
        if (slot >= 0 && slot < count)
        {
            VINetItem *it = items.Item(slot);
            m_info.DescribeItem(m_desc, sizeof(m_desc), it);
        }
    }
}

// 0x00C4D1C8.
void VIWndFishing::Rename()
{
    VIClient *app = (VIClient *)GetAppPtr();
    ChatOff();
    m_edit->SetRows(1);
    m_edit->SetMultiLine(0);
    m_edit->SetCenter(VIVect2I(320, 224));
    m_edit->SetMaxLength(128);
    m_edit->SetVisible(true);
    app->SetActiveWindow(m_edit);
}

// 0x00C4D258.
void VIWndFishing::Renamed()
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->ChatLine(m_edit->m_text);
    m_edit->SetVisible(false);
    app->SetActiveWindow(this);
}

// 0x00C4D2B0.
void VIWndFishing::SetText(const char *s)
{
    m_textEnd = ((VIClient *)GetAppPtr())->m_p07568 + 5000;
    VIString::str8to16(m_text, sizeof(m_text), s);
}

// 0x00C4D300.
void VIWndFishing::SetText(const unsigned short *s)
{
    m_textEnd = ((VIClient *)GetAppPtr())->m_p07568 + 5000;
    VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x00C4D350.  The splash: a particle definition on texture 0x2B6BF329, a short
// fountain fading from pale blue to white.
int LoadTargetEffect(VIScene &sc)
{
    VIParticleSystem &ps = sc.m_particleSystem;
    VIDictionary *d = sc.m_dict;
    VIRaster *r = sc.m_raster;
    VIColor32F from;
    VIColor32F to;
    VIColor32F c;
    if (!d->FindTyped(0x2B6BF329, (VIResourceType)1, gTargetEffectTex))
    {
        gTargetEffectTex = -1;
        gTargetEffectDef = -1;
        return -1;
    }
    gTargetEffectDef = ps.CreateParticleDefinition(0);
    if (gTargetEffectDef < 0)
        return -1;
    VIParticleDefinition *def = ps.ParticleDefinition(gTargetEffectDef);
    def->SetTexture(gTargetEffectTex, *r, *d);
    def->SetBlendMode((VIParticleDefinitionEx::BLENDMODE)1);
    VIVect3 v(0.0f, 1.0f, 0.0f);
    def->SetNozzleAxis(v);
    v.Set(0.39f, 0.39f, 0.39f);
    def->SetNozzleHprVar(v);
    def->SetBirthrate(100.0f);
    def->SetLifespan(0.8f);
    def->SetStartSize(0.2f);
    def->SetEndSize(0.2f);
    def->SetGravityOn(1);
    from.Set(0.8f, 0.8f, 1.0f, 1.0f);
    to.Set(1.0f, 1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 32; i++)
    {
        c = to;
        float t = (float)i * 0.03125f;
        c.r = (c.r - from.r) * t + from.r;
        c.g = (c.g - from.g) * t + from.g;
        c.b = (c.b - from.b) * t + from.b;
        c.a = (c.a - from.a) * t + from.a;
        def->SetGradientColor(i, c);
    }
    sTargetEmitter.Create(&ps, def);
    return 0;
}

// 0x00C4D610.
int ReleaseTargetEffect(VIScene &sc)
{
    VIParticleSystem &ps = sc.m_particleSystem;
    VIDictionary *d = sc.m_dict;
    if (gTargetEffectDef != -1)
    {
        sTargetEmitter.Destroy();
        ps.ReleaseParticleDefinition(gTargetEffectDef, *d);
        gTargetEffectDef = -1;
        gTargetEffectTex = -1;
    }
    return 0;
}

// 0x00C4D688.  Where a line cast ten units ahead lands: nothing in the way, and
// water (an environment hit) below the end before any solid.
int VIWndFishing::FindWater(int actor, const VIVect3 &pos, const VIVect3 &hpr, VIScene *sc, VIVect3 &out)
{
    VIVect3 dir;
    VIVect3 a;
    VIVect3 b;
    VIVect3 start;
    VITransEnvRay env;
    VICollRay ray;
    VISceneFilter filter;
    start = pos;
    start.y += 1.0f;
    out = pos;
    dir.HPR(hpr.x, 0.0f, 0.0f);
    out.x += dir.x * 10.0f;
    out.y += dir.y * 10.0f;
    out.z += dir.z * 10.0f;
    a.Set(out.x, out.y + 1.0f, out.z);
    b.Set(out.x, out.y - 10.0f, out.z);
    env.Init(a, b, 3);
    filter.Init(actor);
    ray.Init(start, a);
    if (sc->Collide(ray, filter))
        return 0;
    if (!sc->Collide(env))
        return 0;
    float t = env.GetHitTime();
    out = env.GetHitPoint();
    ray.Init(a, b);
    if (!sc->Collide(ray, filter))
        return 1;
    if (ray.GetHitTime() < t)
        return 0;
    return 1;
}

// 0x00C4D8E0.
void VIWndFishing::ChatOn()
{
    if (m_chat)
        m_chat->SetScroll(1);
    m_chatKey = 0;
    m_chatOn = 1;
}

// 0x00C4D920.
void VIWndFishing::ChatOff()
{
    if (m_chat && m_chatOn)
    {
        m_chat->SetScroll(0);
        if (m_chatKey)
            m_chat->OnKeyUp(m_chatKey);
    }
    m_chatOn = 0;
    m_chatKey = 0;
}
