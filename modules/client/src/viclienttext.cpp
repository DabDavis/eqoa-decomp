#line 1 "C:/code-frontiers-gm/game/VIClientText.cpp"
#include "game.h"

// VIClient+0x4FC24's text expander (file and class names provisional): '%' escapes in a
// server string.  With `flag` set, a letter (or & * ( or a digit 7..9) becomes a two-
// character '|x' colour/format code and %p the player's name; with it clear, %1..%4 are
// the player's name, level, class name and the target's name.  %% is a '%', anything
// else "(?)".  Returns -1 for a null source, else 0; stops where the next piece no
// longer fits.

VIClientText::VIClientText()
{
    m_client = 0;
}

VIClientText::~VIClientText()
{
    m_client = 0;
}

int VIClientText::SetClient(VIClient *client)
{
    m_client = client;
    return 0;
}

int VIClientText::Expand(unsigned short *dst, unsigned short size, const unsigned short *src, int flag)
{
    char name[128];
    unsigned short buf[128];
    VIUI &ui = m_client->m_ui;
    unsigned short *d = dst;
    int full = 0;
    if (!src)
    {
        *dst = 0;
        return -1;
    }
    while (*src)
    {
        unsigned short c = *src++;
        if (c == '%')
        {
            int tok;
            const unsigned short *s;
            src = ParseToken(src, &tok);
            if (flag)
            {
                switch (tok)
                {
                case 't': VIString::str8to16(buf, sizeof(buf), "|t"); s = buf; break;
                case 'c': VIString::str8to16(buf, sizeof(buf), "|c"); s = buf; break;
                case 'x': VIString::str8to16(buf, sizeof(buf), "|x"); s = buf; break;
                case 's': VIString::str8to16(buf, sizeof(buf), "|s"); s = buf; break;
                case 'p':
                    m_client->GetPlayerName(name, sizeof(name));
                    VIString::str8to16(buf, sizeof(buf), name);
                    s = buf;
                    break;
                case 7:   VIString::str8to16(buf, sizeof(buf), "|7"); s = buf; break;
                case '&': VIString::str8to16(buf, sizeof(buf), "|&"); s = buf; break;
                case 8:   VIString::str8to16(buf, sizeof(buf), "|8"); s = buf; break;
                case '*': VIString::str8to16(buf, sizeof(buf), "|*"); s = buf; break;
                case 9:   VIString::str8to16(buf, sizeof(buf), "|9"); s = buf; break;
                case '(': VIString::str8to16(buf, sizeof(buf), "|("); s = buf; break;
                case 'l': VIString::str8to16(buf, sizeof(buf), "|v"); s = buf; break;
                case 'r': VIString::str8to16(buf, sizeof(buf), "|V"); s = buf; break;
                case 'u': VIString::str8to16(buf, sizeof(buf), "|z"); s = buf; break;
                case 'd': VIString::str8to16(buf, sizeof(buf), "|Z"); s = buf; break;
                case 'a': VIString::str8to16(buf, sizeof(buf), "|a"); s = buf; break;
                case 'A': VIString::str8to16(buf, sizeof(buf), "|A"); s = buf; break;
                case '%': VIString::str8to16(buf, sizeof(buf), "%"); s = buf; break;
                case 0:
                default:  VIString::str8to16(buf, sizeof(buf), "(?)"); s = buf; break;
                }
            }
            else
            {
                switch (tok)
                {
                case 1:
                    m_client->GetPlayerName(name, sizeof(name));
                    VIString::str8to16(buf, sizeof(buf), name);
                    s = buf;
                    break;
                case 2:
                    sprintf(name, "%d", m_client->GetLevel());
                    VIString::str8to16(buf, sizeof(buf), name);
                    s = buf;
                    break;
                case 3:
                {
                    const unsigned short *cls;
                    switch (m_client->GetClass())
                    {
                    case 0:  cls = (const unsigned short *)ui.m_words.GetString(165, ui.m_language); break;
                    case 1:  cls = (const unsigned short *)ui.m_words.GetString(166, ui.m_language); break;
                    case 2:  cls = (const unsigned short *)ui.m_words.GetString(167, ui.m_language); break;
                    case 3:  cls = (const unsigned short *)ui.m_words.GetString(168, ui.m_language); break;
                    case 4:  cls = (const unsigned short *)ui.m_words.GetString(169, ui.m_language); break;
                    case 5:  cls = (const unsigned short *)ui.m_words.GetString(170, ui.m_language); break;
                    case 6:  cls = (const unsigned short *)ui.m_words.GetString(171, ui.m_language); break;
                    case 7:  cls = (const unsigned short *)ui.m_words.GetString(172, ui.m_language); break;
                    case 8:  cls = (const unsigned short *)ui.m_words.GetString(173, ui.m_language); break;
                    case 9:  cls = (const unsigned short *)ui.m_words.GetString(174, ui.m_language); break;
                    case 10: cls = (const unsigned short *)ui.m_words.GetString(175, ui.m_language); break;
                    case 11: cls = (const unsigned short *)ui.m_words.GetString(176, ui.m_language); break;
                    case 12: cls = (const unsigned short *)ui.m_words.GetString(177, ui.m_language); break;
                    case 13: cls = (const unsigned short *)ui.m_words.GetString(178, ui.m_language); break;
                    case 14: cls = (const unsigned short *)ui.m_words.GetString(179, ui.m_language); break;
                    default: cls = 0; break;
                    }
                    if (cls)
                    {
                        s = buf;
                        VIString::strcpy16(buf, sizeof(buf), cls);
                    }
                    else
                    {
                        buf[0] = 0;
                        s = buf;
                    }
                    break;
                }
                case 4:
                    m_client->GetTargetName(name);
                    VIString::str8to16(buf, sizeof(buf), name);
                    s = buf;
                    break;
                case '%': VIString::str8to16(buf, sizeof(buf), "%"); s = buf; break;
                default:  s = buf; VIString::str8to16(buf, sizeof(buf), "(?)"); break;
                }
            }
            unsigned int len = VIString::strlen16(s);
            if ((d - dst) + len < size)
            {
                VIString::strcpy16(d, size - (d - dst), s);
                d += VIString::strlen16(s);
            }
            else
                full = 1;
        }
        else if ((d - dst) + 1 < size)
            *d++ = c;
        else
            full = 1;
        if (full)
            break;
    }
    *d = 0;
    return 0;
}

const unsigned short *VIClientText::ParseToken(const unsigned short *p, int *tok)
{
    int n = 0;
    int c;
    for (; (c = *p) != 0; p++)
    {
        if (c >= '0' && c <= '9')
            n = n * 10 + (*p - '0');
        else
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (unsigned short)c == '&' || (unsigned short)c == '*' || (unsigned short)c == '(' || (unsigned short)c == '%')
            {
                n = *p;
                p++;
            }
            break;
        }
    }
    *tok = n;
    return p;
}
