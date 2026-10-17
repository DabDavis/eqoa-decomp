#line 1 "C:/code-frontiers-gm/game/VIWndTrainer.cpp"
// VIWndTrainer -- CLIENT 0x00C9EA88 .. 0x00CA1AB0 (the TU runs to its __tf at 0x00CA2688,
// then SetPos).  The stat trainer: seven trainable stats with points to spend, nine derived
// stats, and under each the equipped items that give the selected one.
#include "game.h"

// One WORDS.TXT entry in the current language.
// The font's colour through a reference parameter: the argument is a fresh `addiu sp,N`,
// not the register the caller holds the colour's address in.
static inline void TextColor(VIUI *ui, const VIColor32F &c)
{
    ui->m_font.SetTextColor(c);
}

static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// A row of either page: the stat, its caption, and (trainable stats) a description.
struct VITrainerRow
{
    int m_stat;
    int m_word;
    int m_desc;
};

static VITrainerRow sStats[7] = {       // 0x00CF1758
    { 0, 248, 202 }, { 1, 249, 203 }, { 2, 250, 204 }, { 3, 251, 205 },
    { 4, 252, 206 }, { 5, 253, 207 }, { 6, 254, 208 },
};

static VITrainerRow sDetails[9] = {     // 0x00CF17B0
    { 14, 242, -1 }, { 7, 233, -1 }, { 9, 466, -1 }, { 24, 48, -1 }, { 25, 483, -1 },
    { 26, 484, -1 }, { 27, 485, -1 }, { 22, 486, -1 }, { 23, 487, -1 },
};

// 0x00C9EA88.
VIWndTrainer::VIWndTrainer()
{
    m_sel = 0;
    m_points = 0;
    m_pos.Set(0, 0);
    m_base = 0;
    m_cur = 0;
    m_mode = 0;
    m_fade = 1.0f;
    m_fadeDir = 1;
    m_colorDir = 1;
    m_color.Set(0.0f, 0.0f, 0.0f, 1.0f);
    m_time = 0;
    m_last = 0;
}

// 0x00C9EB50.
VIWndTrainer::~VIWndTrainer()
{
    Free();
}

// 0x00C9EBC0.
int VIWndTrainer::Init()
{
    memset(m_add, 0, sizeof(m_add));
    m_mode = 0;
    m_fade = 1.0f;
    m_fadeDir = 1;
    m_colorDir = 1;
    m_item = 0;
    m_color.Set(0.0f, 0.0f, 0.0f, 1.0f);
    m_time = 0;
    m_last = 0;
    return 0;
}

// 0x00C9EC30.
int VIWndTrainer::Free()
{
    m_base = 0;
    m_cur = 0;
    memset(m_add, 0, sizeof(m_add));
    m_mode = 0;
    m_fade = 1.0f;
    m_fadeDir = 1;
    m_colorDir = 1;
    m_item = 0;
    m_color.Set(0.0f, 0.0f, 0.0f, 1.0f);
    m_last = 0;
    return 0;
}

// 0x00C9ECA8.
void VIWndTrainer::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    long now = app->m_p07568;
    VIUI *ui = &app->m_ui;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 162), 640, 17, 0);
    m_window.Open(true);
    m_window.Draw();
    unsigned short buf[128];
    switch (m_mode)
    {
    case 0:
    case 4:
        VIString::snprintf16(buf, sizeof(buf), "|-|x%S   |+|t%S", Word(ui, 212), Word(ui, 161));
        break;
    case 1:
    case 2:
        VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 161));
        break;
    case 3:
        VIString::snprintf16(buf, sizeof(buf), "|-|x%S   |+|t%S", Word(ui, 212), Word(ui, 217));
        break;
    default:
        VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 161));
        break;
    }
    m_window.Text(buf, 340, 16, (VIUICOLOR)0, 0);
    if (m_mode != 3)
        DrawStats();
    else
        DrawDetails();
    VIVect2I pos;
    VIVect2I bar(25, 250);
    VIColor32F hi(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIColor32F c;
    const VIUITexEntry *arrow = GetUITexture((VIUITexId)85);
    const VIUITexEntry *slot = GetUITexture((VIUITexId)150);
    if (m_mode != 3)
    {
        c = m_mode == 1 ? hi : black;
        ui->BlitBar((unsigned short)482, c, bar, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 200,
                    (VIAlphaBlendMode)0, 1.0f, 1, 0);
    }
    if (m_mode == 1)
    {
        pos.x = bar.x;
        pos.y = bar.y + slot->m_uv1.y / 2 - arrow->m_uv1.y / 2 - 1;
    }
    if (m_mode != 3)
    {
        bar.y += 25;
        c = m_mode == 2 ? hi : black;
        ui->BlitBar((unsigned short)77, c, bar, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 200,
                    (VIAlphaBlendMode)0, 1.0f, 1, 0);
    }
    if (m_mode == 2)
    {
        pos.x = bar.x;
        pos.y = bar.y + slot->m_uv1.y / 2 - arrow->m_uv1.y / 2 - 1;
    }
    if (m_mode == 1 || m_mode == 2)
        ui->BlitEx((VIUITexId)arrow->m_x00, pos, (VIAlphaBlendMode)3, m_fade, 0);
    ui->End2D();
    if (m_fadeDir == -1)
    {
        m_fade -= dt * 2.0f;
        m_fade = m_fade < 0.0f ? 0.0f : m_fade;
        if (m_fade == 0.0f)
            m_fadeDir = 1;
    }
    else
    {
        m_fade += dt * 3.0f;
        m_fade = m_fade < 1.0f ? m_fade : 1.0f;
        if (m_fade == 1.0f)
            m_fadeDir = -1;
    }
    if (m_colorDir == -1)
    {
        m_color.r -= dt;
        m_color.r = m_color.r < 0.0f ? 0.0f : m_color.r;
        m_color.g -= dt;
        m_color.g = m_color.g < 0.0f ? 0.0f : m_color.g;
        m_color.b -= dt;
        m_color.b = m_color.b < 0.0f ? 0.0f : m_color.b;
        if (m_color.r == 0.0f && m_color.g == 0.0f && m_color.b == 0.0f)
            m_colorDir = 1;
    }
    else
    {
        m_color.r += dt * 2.0f;
        m_color.r = m_color.r < 0.8f ? m_color.r : 0.8f;
        m_color.g += dt * 2.0f;
        m_color.g = m_color.g < 0.8f ? m_color.g : 0.8f;
        m_color.b += dt * 2.0f;
        m_color.b = m_color.b < 0.8f ? m_color.b : 0.8f;
        if (m_color.r == 0.8f && m_color.g == 0.8f && m_color.b == 0.8f)
            m_colorDir = -1;
    }
    if (m_mode == 0)
    {
        if (m_flash.Pulse(136))
            Adjust(-1);
        if (m_flash.Pulse(137))
            Adjust(1);
    }
}

// 0x00C9F3B8.
void VIWndTrainer::DrawStats()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIClientPart4EC58 *part = app->GetP4EC58();
    char num[16];
    unsigned short text[64];
    VIVect2I p0, p1, p2, p3, p4, p5;
    VIColor32F hi(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    p0.Set(25, 15);
    p1 = p0 + VIVect2I(0, 25);
    p2 = p1 + VIVect2I(300, 0);
    p3 = p1 + VIVect2I(200, 0);
    p4 = p2 + VIVect2I(220, 0);
    DrawHeader(p0);
    if (m_mode == 4)
    {
        ui->BlitBar((unsigned short)665, black, p1, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 200,
                    (VIAlphaBlendMode)0, 1.0f, 1, 0);
        ui->BlitBar("", black, p3, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 75,
                    (VIAlphaBlendMode)0, 1.0f, 0, 0);
    }
    else
    {
        ui->BlitBar((unsigned short)489, m_points < 1 ? black : m_color, p1, (VIUITexId)141,
                    (VIUITexId)142, (VIUITexId)150, 200, (VIAlphaBlendMode)0, 1.0f, 1, 0);
        VIString::snprintf(num, 16, "%d", m_points);
        ui->BlitBar(num, m_points < 1 ? black : m_color, p3, (VIUITexId)141, (VIUITexId)142,
                    (VIUITexId)150, 75, (VIAlphaBlendMode)0, 1.0f, 0, 0);
    }
    p1.y += 25;
    p3.y += 25;
    int stat;
    for (int i = 0; i < 7; i++)
    {
        stat = sStats[i].m_stat;
        VIColor32F c;
        if (i == m_sel && m_mode == 0)
            c = hi;
        else if (IsStatHighlighted(app->GetClass(), stat))
            c = m_color;
        else
            c = black;
        ui->BlitBar((unsigned short)sStats[i].m_word, c, p1, (VIUITexId)141, (VIUITexId)142,
                    (VIUITexId)150, 200, (VIAlphaBlendMode)0, 1.0f, 1, 0);
        VIString::snprintf(num, 16, "%d", m_cur[stat] + m_add[stat]);
        ui->BlitBar(num, c, p3, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 75,
                    (VIAlphaBlendMode)0, 1.0f, 0, 0);
        if (i == m_sel && m_mode == 0)
        {
            const VIUITexEntry *arrow = GetUITexture((VIUITexId)85);
            const VIUITexEntry *slot = GetUITexture((VIUITexId)150);
            p5.y = p1.y + slot->m_uv1.y / 2 - arrow->m_uv1.y / 2 - 1;
            p5.x = p1.x;
            ui->BlitEx((VIUITexId)arrow->m_x00, p5, (VIAlphaBlendMode)3, m_fade, 0);
            p5.x = p3.x + 2;
            ui->BlitEx((VIUITexId)128, p5, (VIAlphaBlendMode)3, m_fade, 1);
            p5.x += 60;
            ui->BlitEx((VIUITexId)128, p5, (VIAlphaBlendMode)3, m_fade, 0);
        }
        p1.y += 25;
        p3.y += 25;
        ui->m_font.SetTextColor(black);
    }
    stat = sStats[m_sel].m_stat;
    VIString::strcpy16(text, sizeof(text), Word(ui, 480));
    VIString::strcat16(text, sizeof(text), " ");
    VIString::strcat16(text, sizeof(text), Word(ui, sStats[m_sel].m_word));
    VIString::snprintf(num, 16, "%d", m_base[stat] + m_add[stat] + part->GetStat(stat));
    ui->BlitBar(text, black, p2, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 220,
                (VIAlphaBlendMode)0, 1.0f, 1, 0);
    ui->BlitBar(num, black, p4, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 55,
                (VIAlphaBlendMode)0, 1.0f, 0, 0);
    p2.y += 25;
    p4.y += 25;
    VIString::strcpy16(text, sizeof(text), Word(ui, 481));
    VIString::strcat16(text, sizeof(text), " ");
    VIString::strcat16(text, sizeof(text), Word(ui, sStats[m_sel].m_word));
    VIString::snprintf(num, 16, "%d", VIMIN(m_cur[stat] + m_add[stat], app->GetP44Total(stat)));
    ui->BlitBar(text, black, p2, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 220,
                (VIAlphaBlendMode)0, 1.0f, 1, 0);
    ui->BlitBar(num, black, p4, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 55,
                (VIAlphaBlendMode)0, 1.0f, 0, 0);
    p2.y += 35;
    DrawItems(p2, stat);
}

// 0x00C9FCD0.
void VIWndTrainer::DrawDetails()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIClientPart4EC58 *part = app->GetP4EC58();
    char num[16];
    unsigned short text[64];
    VIVect2I p0, p1, p2, p3, p4, p5;
    VIColor32F hi(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    p0.Set(25, 15);
    p1 = p0 + VIVect2I(0, 25);
    p2 = p1 + VIVect2I(300, 0);
    p3 = p1 + VIVect2I(200, 0);
    p4 = p2 + VIVect2I(200, 0);
    DrawHeader(p0);
    int stat;
    for (int i = 0; i < 9; i++)
    {
        VIColor32F c;
        c = i == m_sel && m_mode == 3 ? hi : black;
        stat = sDetails[i].m_stat;
        ui->BlitBar((unsigned short)sDetails[i].m_word, c, p1, (VIUITexId)141, (VIUITexId)142,
                    (VIUITexId)150, 200, (VIAlphaBlendMode)0, 1.0f, 1, 0);
        if (i == m_sel && m_mode == 3)
        {
            const VIUITexEntry *arrow = GetUITexture((VIUITexId)85);
            const VIUITexEntry *slot = GetUITexture((VIUITexId)150);
            p5.y = p1.y + slot->m_uv1.y / 2 - arrow->m_uv1.y / 2 - 1;
            p5.x = p1.x;
            ui->BlitEx((VIUITexId)arrow->m_x00, p5, (VIAlphaBlendMode)3, m_fade, 0);
        }
        VIString::snprintf(num, 16, "%d", CurStat(stat));
        ui->BlitBar(num, c, p3, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 75,
                    (VIAlphaBlendMode)0, 1.0f, 0, 0);
        p1.y += 25;
        p3.y += 25;
        TextColor(ui, black);
    }
    stat = sDetails[m_sel].m_stat;

    VIString::strcpy16(text, sizeof(text), Word(ui, 480));
    VIString::strcat16(text, sizeof(text), " ");
    VIString::strcat16(text, sizeof(text), Word(ui, sDetails[m_sel].m_word));
    if (stat == 7)
        VIString::snprintf(num, 16, "%d", m_base[8] + part->GetStat(8));
    else if (stat == 9)
        VIString::snprintf(num, 16, "%d", m_base[10] + part->GetStat(10));
    else
        VIString::snprintf(num, 16, "%d", BaseStat(stat) + part->GetStat(stat));
    ui->BlitBar(text, black, p2, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 200,
                (VIAlphaBlendMode)0, 1.0f, 1, 0);
    ui->BlitBar(num, black, p4, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 75,
                (VIAlphaBlendMode)0, 1.0f, 0, 0);
    p2.y += 25;
    p4.y += 25;
    VIString::strcpy16(text, sizeof(text), Word(ui, 481));
    VIString::strcat16(text, sizeof(text), " ");
    VIString::strcat16(text, sizeof(text), Word(ui, sDetails[m_sel].m_word));
    VIString::snprintf(num, 16, "%d", CurStat(stat));
    ui->BlitBar(text, black, p2, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 200,
                (VIAlphaBlendMode)0, 1.0f, 1, 0);
    ui->BlitBar(num, black, p4, (VIUITexId)141, (VIUITexId)142, (VIUITexId)150, 75,
                (VIAlphaBlendMode)0, 1.0f, 0, 0);
    p2.y += 35;
    if (stat == 7)
        DrawItems(p2, 8);
    else if (stat == 9)
        DrawItems(p2, 10);
    else
        DrawItems(p2, stat);
}

// 0x00CA03D8.
void VIWndTrainer::DrawHeader(VIVect2I pos)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    const VIRaceDef *race = GetRace(app->GetRace());
    const VIClassDef *cls = GetClass(app->GetClass());
    unsigned short buf[128];
    char name[24];
    app->GetPlayerName(name, sizeof(name));
    VIString::snprintf16(buf, sizeof(buf), "%s: %S %d %S %S", name, Word(ui, 299), app->GetLevel(),
                         Word(ui, race->m_nameStrID), Word(ui, cls->m_nameStrID));
    ui->m_font.SetTextColor(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
    ui->m_font.DrawString(ui->m_raster, ui->m_dict, buf, pos.x, pos.y);
}

// 0x00CA0540.
void VIWndTrainer::DrawItems(VIVect2I pos, int stat)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIUI &u = app->m_ui;
    VIVect2I at = pos;
    unsigned short buf[64];
    VIColor32F black;
    VIColor32F blue;
    VIColor32F red;
    blue.Set(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    red.Set(95.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f, 1.0f);
    black.a = 1.0f;
    black.r = 0.0f;
    black.g = 0.0f;
    black.b = 0.0f;
    m_list.Init(UI(), VIVect2I(0, 0), VIVect2I(462, 183), 295, 10, 1);
    m_list.Open(true);
    int count;
    switch (m_mode)
    {
    case 0:
        count = CountItems(sStats[m_sel].m_stat);
        break;
    case 3:
        count = CountItems(sDetails[m_sel].m_stat);
        break;
    case 1:
    case 2:
    case 4:
        return;
    }
    int n = 0;
    int i;
    for (i = 0; i < m_item && i < app->GetInvCount(); i++)
        if (FindStat(i, stat))
            n++;
    float t;
    if (count > 10)
        t = (float)n / (float)(count - 10);
    else
        t = 0.0f;
    t = VIMIN(1.0f, VIMAX(0.0f, t));
    m_list.DrawFrameVScroll(15, 0, m_list.Width() - 40, 10, t);
    int lines = 0;
    for (i = m_item; i < app->GetInvCount(); i++)
    {
        int slot = app->GetInvItemEquipSlot(i);
        VIWordId *slotWord = FindEquipSlotWord(slot);
        if (!slotWord)
            continue;
        VIItemStat *s = FindStat(i, stat);
        if (!s)
            continue;
        buf[0] = 0;
        const char *fmt = s->m_value >= 0 ? "%d: +%d %d" : "%d: %d %d";
        ui->m_font.SetTextColor(black);
        VIString::snprintf16(buf, sizeof(buf), fmt, slot, s->m_value, s->m_id);
        VIWordId *statWord = FindStatWord(s->m_id);
        if (statWord)
        {
            fmt = s->m_value >= 0 ? "%S: +%d %S" : "%S: %d %S";
            ui->m_font.SetTextColor(s->m_value >= 0 ? blue : red);
            VIString::snprintf16(buf, sizeof(buf), fmt, Word(ui, slotWord->m_word), s->m_value,
                                 Word(ui, statWord->m_word));
        }
        int rows = m_list.CountLines(buf, m_list.Width() - 50);
        if (lines < 10)
        {
            m_last = i;
            if (10 - lines < rows)
            {
                rows = 10 - lines;
                m_last = i - 1;
            }
            m_list.TextBox(buf, 20, lines, m_list.Width() - 50, rows, u.m_font.GetTextColor(), 2);
        }
        lines += rows;
    }
}

// 0x00CA09E8.
void VIWndTrainer::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 19:
    case 138:
        Up(key);
        break;
    case 20:
    case 139:
        Down(key);
        break;
    case 17:
    case 136:
        Left(key);
        break;
    case 18:
    case 137:
        Right(key);
        break;
    case 13:
    case 134:
        Select(key);
        break;
    case 27:
    case 132:
        Cancel(key);
        break;
    case 69:
    case 101:
    case 142:
        PageUp(key);
        break;
    case 67:
    case 99:
    case 143:
        PageDown(key);
        break;
    }
}

// 0x00CA0AD8.
void VIWndTrainer::Up(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    switch (m_mode)
    {
    case 1:
        app->PlaySound(11);
        m_mode = 0;
        Move(0);
        break;
    case 2:
        app->PlaySound(11);
        m_mode = 1;
        break;
    case 0:
    case 3:
        Move(-1);
        break;
    case 4:
        app->PlaySound(9);
        break;
    }
}

// 0x00CA0B98.
void VIWndTrainer::Down(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    switch (m_mode)
    {
    case 0:
        if (m_sel < 6u)
            Move(1);
        else
        {
            app->PlaySound(11);
            m_mode = 1;
        }
        break;
    case 1:
        app->PlaySound(11);
        m_mode = 2;
        break;
    case 2:
        app->PlaySound(9);
        break;
    case 3:
        Move(1);
        break;
    case 4:
        app->PlaySound(9);
        break;
    }
}

// 0x00CA0C70.
void VIWndTrainer::Left(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    switch (m_mode)
    {
    case 0:
        m_flash.Press(136);
        break;
    case 1:
    case 2:
    case 3:
        break;
    case 4:
        app->PlaySound(9);
        break;
    }
}

// 0x00CA0D00.
void VIWndTrainer::Right(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    switch (m_mode)
    {
    case 0:
        m_flash.Press(137);
        break;
    case 1:
    case 2:
    case 3:
        break;
    case 4:
        app->PlaySound(9);
        break;
    }
}

// 0x00CA0D90.
void VIWndTrainer::Select(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    switch (m_mode)
    {
    case 0:
        break;
    case 1:
    {
        int sum = 0;
        for (int i = 0; i < 7; i++)
            sum += m_add[i];
        if (sum == 0)
            app->PlaySound(10);
        else
        {
            app->PlaySound(13);
            app->TrainStats(m_add);
            m_mode = 4;
        }
        break;
    }
    case 2:
        app->PlaySound(13);
        m_sel = 0;
        m_item = 0;
        m_mode = 3;
        Move(0);
        break;
    case 3:
        break;
    case 4:
        app->PlaySound(9);
        break;
    }
}

// 0x00CA0E98.
void VIWndTrainer::Cancel(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    switch (m_mode)
    {
    case 0:
    case 1:
    case 2:
    case 4:
        app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    case 3:
        app->PlaySound(8);
        m_mode = 0;
        m_sel = 0;
        Move(0);
        break;
    }
}

// 0x00CA0F60.
void VIWndTrainer::PageUp(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    switch (m_mode)
    {
    case 0:
        ItemUp(sStats[m_sel].m_stat);
        break;
    case 1:
    case 2:
        break;
    case 3:
        ItemUp(sDetails[m_sel].m_stat);
        break;
    case 4:
        app->PlaySound(9);
        break;
    }
}

// 0x00CA1018.
void VIWndTrainer::PageDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    ResetFade();
    switch (m_mode)
    {
    case 4:
        app->PlaySound(9);
        break;
    case 0:
        ItemDown(sStats[m_sel].m_stat);
        break;
    case 1:
    case 2:
        break;
    case 3:
        ItemDown(sDetails[m_sel].m_stat);
        break;
    }
}

// 0x00CA10E0.
void VIWndTrainer::OnKeyUp(int key)
{
    switch (key)
    {
    case 17:
    case 136:
        LeftReleased();
        break;
    case 18:
    case 137:
        RightReleased();
        break;
    }
}

// 0x00CA1150.
void VIWndTrainer::LeftReleased()
{
    if (m_mode == 0)
        m_flash.Release(136);
}

// 0x00CA1180.
void VIWndTrainer::RightReleased()
{
    if (m_mode == 0)
        m_flash.Release(137);
}

// 0x00CA11B0.
void VIWndTrainer::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if ((VIWnd *)p2 == this)
        {
            m_list.Init(UI(), VIVect2I(0, 0), VIVect2I(400, 220), 150, 9, 0);
            m_list.Open(true);
            m_last = 0;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 162), 640, 17, 0);
            m_window.Open(true);
            m_flash.Reset();
        }
        if ((VIWnd *)p1 == this)
            m_flash.Reset();
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00CA12E0.
void VIWndTrainer::Move(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int count;
    if (m_mode == 0)
        count = 7;
    else if (m_mode == 3)
        count = 9;
    else
    {
        m_sel = 0;
        m_item = 0;
        return;
    }
    if (m_sel + d < 0)
    {
        app->PlaySound(9);
        m_sel = 0;
    }
    else if (m_sel + d >= count)
    {
        app->PlaySound(9);
        m_sel = count - 1;
    }
    else
    {
        app->PlaySound(11);
        m_sel += d;
    }
    int stat;
    if (m_mode == 0)
        stat = sStats[m_sel].m_stat;
    else
        stat = sDetails[m_sel].m_stat;
    m_item = 0;
    if (m_mode == 0 || m_mode == 3)
    {
        for (int i = 0; i < app->GetInvCount(); i++)
            if (FindStat(i, stat))
            {
                m_item = i;
                break;
            }
    }
}

// 0x00CA1420.
void VIWndTrainer::Adjust(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int stat = sStats[m_sel].m_stat;
    if ((m_points <= 0 && d > 0) || (m_points == app->GetPlayerP40() && d < 0))
    {
        app->PlaySound(9);
        return;
    }
    int v = m_base[stat] + m_add[stat];
    if (app->GetP44Total(-1) < v + d)
    {
        app->PlaySound(9);
        return;
    }
    int base = m_base[stat];
    if (d < 0 && base == v)
    {
        app->PlaySound(9);
        return;
    }
    app->PlaySound(11);
    m_points -= d;
    m_add[stat] += d;
}

// 0x00CA1550.
void VIWndTrainer::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_sel = 0;
    m_item = 0;
    m_mode = 0;
    m_points = app->GetPlayerP40();
    m_base = app->GetPlayerPD8();
    m_cur = app->GetPlayerStats();
    Refresh();
}

// 0x00CA15B8.
void VIWndTrainer::Refresh()
{
    VIClient *app = (VIClient *)GetAppPtr();
    for (int i = 0; i < 7; i++)
        m_add[i] = 0;
    m_points = app->GetPlayerP40();
    if (m_mode == 4)
    {
        m_mode = 0;
        m_sel = 0;
        m_item = 0;
    }
}

// 0x00CA1628.
int VIWndTrainer::CountItems(int stat)
{
    int n = 0;
    VIClient *app = (VIClient *)GetAppPtr();
    for (int i = 0; i < app->GetInvCount(); i++)
        if (FindStat(i, stat))
            n++;
    return n;
}

// 0x00CA16B0.
int VIWndTrainer::BaseStat(int stat)
{
    if (stat < 0 || stat >= 29)
        return 0;
    int v;
    if (stat >= 16 && stat <= 27)
    {
        int d = m_base[4] / 7;
        int r = m_base[stat] + 40;
        v = d + r;
        v = VIMIN(v, 315);
    }
    else
        v = m_base[stat];
    return v;
}

// 0x00CA1728.
int VIWndTrainer::CurStat(int stat)
{
    if (stat < 0 || stat >= 29)
        return 0;
    int v;
    if (stat >= 16 && stat <= 27)
    {
        int d = m_cur[4] / 7;
        int r = m_cur[stat] + 40;
        v = d + r;
        v = VIMIN(v, 315);
    }
    else
        v = m_cur[stat];
    return v;
}

// 0x00CA17A0.
void VIWndTrainer::ItemDown(int stat)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int last;
    for (int i = app->GetInvCount() - 1; i >= m_item; i--)
        if (FindStat(i, stat))
        {
            last = i;
            break;
        }
    if (m_last == last)
    {
        app->PlaySound(9);
        return;
    }
    for (int j = m_item + 1; j < app->GetInvCount(); j++)
        if (FindStat(j, stat))
        {
            m_item = j;
            app->PlaySound(11);
            return;
        }
    app->PlaySound(9);
}

// 0x00CA1898.
void VIWndTrainer::ItemUp(int stat)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int first;
    for (int i = 0; i < app->GetInvCount(); i++)
        if (FindStat(i, stat))
        {
            first = i;
            break;
        }
    int j = m_item - 1;
    if (m_item == first)
    {
        app->PlaySound(9);
        return;
    }
    for (; j >= 0; j--)
        if (FindStat(j, stat))
        {
            m_item = j;
            app->PlaySound(11);
            return;
        }
    app->PlaySound(9);
}

// 0x00CA1988.
VIItemStat *VIWndTrainer::FindStat(int i, int stat)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (i >= app->GetInvCount())
        return 0;
    if (app->GetInvItemEquipSlot(i) == -1)
        return 0;
    VIArray<VIItemStat> *stats = app->GetInvItemStats(i);
    for (int k = stats->Begin(); k != stats->End(); k = stats->Next(k))
        if ((*stats)[k].m_id == stat)
            return &(*stats)[k];
    return 0;
}

// 0x00CA1A70.
void VIWndTrainer::ResetFade()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_fadeDir = 1;
    m_fade = 1.0f;
    m_time = app->m_p07568;
}
