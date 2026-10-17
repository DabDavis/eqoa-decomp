// A command-line option parser: CLIENT 0x00CB6B08 .. 0x00CB6DA8, one small TU between
// VICharacter.cpp and VIDateTime.cpp (file name ours). Nothing calls
// ParseArgs; client_main hands VIClient::Init its strings directly.
#include "game.h"

extern "C" int atoi(const char *s);
extern "C" double atof(const char *s);

struct VIArgOption                                   // 12 bytes
{
    const char *m_name;
    int         m_type;                              // 0 flag, 1 int, 2 string, 3 string with '|' as ' ',
                                                     // 4 three strings, 5 four floats
    void       *m_value;
};

// 0x00CB6B08: every `from` to `to` -- after the first character (as shipped).
void ReplaceChar(char *s, char from, char to)
{
    while (*s++)
        if (*s == from)
            *s = to;
}

// 0x00CB6B50
int ParseArgs(int argc, char **argv, VIArgOption *opts, int n)
{
    for (int i = 1; i < argc; i++)
    {
        int j;
        for (j = 0; j < n; j++)
            if (!VIString::strcasecmp(opts[j].m_name, argv[i]))
                break;
        if (j == n)
            return -1;
        int k;
        switch (opts[j].m_type)
        {
        case 0:
            *(int *)opts[j].m_value = 1;
            break;
        case 1:
            if (++i >= argc)
                return -1;
            *(int *)opts[j].m_value = atoi(argv[i]);
            break;
        case 2:
        case 3:
            if (++i >= argc)
                return -1;
            *(char **)opts[j].m_value = argv[i];
            if (opts[j].m_type == 3)
                ReplaceChar(argv[i], '|', ' ');
            break;
        case 4:
            for (k = 0; k < 3; k++)
            {
                if (++i >= argc)
                    return -1;
                ((char **)opts[j].m_value)[k] = argv[i];
            }
            break;
        case 5:
            for (k = 0; k < 4; k++)
            {
                if (++i >= argc)
                    return -1;
                ((float *)opts[j].m_value)[k] = atof(argv[i]);
            }
            break;
        }
    }
    return 0;
}
