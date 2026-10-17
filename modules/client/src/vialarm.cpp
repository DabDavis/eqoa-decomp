#line 1 "C:/code-frontiers-gm/game/VIAlarm.cpp"
// VIAlarm (file and class names ours) -- CLIENT 0x00BE3C10 .. 0x00BE3F98: the game alarm
// at VIClient+0x4F098, then this TU's template copies (0x00BE3FA0 .. 0x00BE4B60, verified
// duplicates).  It follows VIClient.cpp's __tf9VIClientI; the senders' TU follows it.
#include "game.h"


// 0x00BE3C10
VIAlarm::VIAlarm()
{
    m_on = 0;
}

// 0x00BE3C20
int VIAlarm::IsOn()
{
    return m_on;
}

// 0x00BE3C28: "Off" (word 552) turns it off; otherwise digits and ':' only, "H", "H:MM"
// or "H.MM" (word 1402, "Invalid alarm time.", for anything else).
void VIAlarm::Command(VIClient *c, const unsigned short *arg)
{
    VIUI &ui = c->m_ui;
    if (!VIString::strcasecmp16((const unsigned short *)ui.m_words.GetString(552, ui.m_language), arg))
    {
        Set(c, 0, 0, 0);
        return;
    }
    for (const unsigned short *p = arg; *p; p++)
        if ((unsigned short)(*p - '0') > 10)
        {
            c->ShowMsg(1402);
            return;
        }
    char buf[6];
    VIString::str16to8(buf, 6, arg);
    int i;
    for (i = 0; i < 6; i++)
        if (buf[i] == ':' || buf[i] == '.' || !buf[i])
            break;
    int hours, minutes;
    if (buf[i])
    {
        if (sscanf(buf, "%d", &hours) != 1)
            hours = 0;
        if (sscanf(&buf[i + 1], "%d", &minutes) != 1)
            minutes = 0;
    }
    else
    {
        hours = 0;
        if (sscanf(buf, "%d", &minutes) != 1)
            minutes = 0;
    }
    Set(c, 1, hours, minutes);
}

// 0x00BE3DF0
void VIAlarm::Set(VIClient *c, int on, int hours, int minutes)
{
    if (on && minutes <= 0 && hours <= 0)
    {
        c->ShowMsg(1402);
        return;
    }
    if (!on && m_on)
        c->ShowMsg(1202);
    m_on = on;
    m_hours = hours;
    m_minutes = minutes;
    m_when = c->m_p07568 + (long)hours * 3600000 + (long)minutes * 60000;
    if (on)
    {
        unsigned short buf[128];
        VIUI &ui = c->m_ui;
        VIString::snprintf16(buf, 128, (const unsigned short *)ui.m_words.GetString(1200, ui.m_language),
                             hours, minutes);
        c->ShowText(buf);
    }
}

// 0x00BE3F18
int VIAlarm::Tick(VIClient *c)
{
    if (m_on && c->m_p07568 > m_when)
    {
        unsigned short buf[256];
        VIUI &ui = c->m_ui;
        VIString::snprintf16(buf, 256, (const unsigned short *)ui.m_words.GetString(1201, ui.m_language),
                             m_hours, m_minutes);
        c->PromptText(buf, 0);
        m_on = 0;
    }
    return 0;
}
