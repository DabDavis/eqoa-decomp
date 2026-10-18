// VIKeyboard -- SUPPORT 0x004A0E60 .. 0x004A1AC8, its own TU.
//
// The USB keyboard behind the same GetInput() as the pad.  Events arrive from
// the libkbd driver, get translated and dead-key-combined, and land in a
// 128-entry queue that Dequeue drains from the FRONT -- every read shuffles
// the rest down by one.
#include "vi.h"

extern "C" int  printf(const char *fmt, ...);
extern "C" void *memset(void *d, int c, unsigned int n);
extern "C" int  lgKbmInit(int a, int b, int c);
extern "C" void lgKbMPrepareForRebootIOP();
extern "C" int  lgKbdClose(int fd);
extern "C" int  lgKbdEnumerate(int port, void *info);
extern "C" int  lgKbdOpen(int port, int *fd);
extern "C" int  lgKbdSetRepeatRate(int a, int b);
extern "C" int  lgKbdEnumHint(int *hint);
extern "C" int  lgKbdRead(int fd, void *buf);

VIGameOptions *VIGetGameOptions();

// Phase 61.  One layout row: a USB usage code and the character it gives
// plain, shifted and with right-alt.  Four layouts, indexed by the options'
// REGION byte: US and UK are const, French and German are not (they sit in
// .data, between this TU's dead-key lists).
struct VIKeyMap
{
    unsigned short m_code;
    unsigned short m_plain;
    unsigned short m_shift;
    unsigned short m_alt;
};

static const VIKeyMap sKeysUS[74] =
{
    {  53, '`', '~', 0 }, {  30, '1', '!', 0 }, {  31, '2', '@', 0 },
    {  32, '3', '#', 0 }, {  33, '4', '$', 0 }, {  34, '5', '%', 0 },
    {  35, '6', '^', 0 }, {  36, '7', '&', 0 }, {  37, '8', '*', 0 },
    {  38, '9', '(', 0 }, {  39, '0', ')', 0 }, {  45, '-', '_', 0 },
    {  46, '=', '+', 0 }, { 137, 0, 0, 0 }, {  42, 0x08, 0x08, 0 },
    {  43, 0x09, 0x09, 0 }, {  20, 'q', 'Q', 0 }, {  26, 'w', 'W', 0 },
    {   8, 'e', 'E', 0 }, {  21, 'r', 'R', 0 }, {  23, 't', 'T', 0 },
    {  28, 'y', 'Y', 0 }, {  24, 'u', 'U', 0 }, {  12, 'i', 'I', 0 },
    {  18, 'o', 'O', 0 }, {  19, 'p', 'P', 0 }, {  47, '[', '{', 0 },
    {  48, ']', '}', 0 }, {  49, '\\', '|', 0 }, {   4, 'a', 'A', 0 },
    {  22, 's', 'S', 0 }, {   7, 'd', 'D', 0 }, {   9, 'f', 'F', 0 },
    {  10, 'g', 'G', 0 }, {  11, 'h', 'H', 0 }, {  13, 'j', 'J', 0 },
    {  14, 'k', 'K', 0 }, {  15, 'l', 'L', 0 }, {  51, ';', ':', 0 },
    {  52, '\'', '"', 0 }, {  50, '#', '~', 0 }, {  40, 0x0A, 0x0A, 0 },
    {  29, 'z', 'Z', 0 }, {  27, 'x', 'X', 0 }, {   6, 'c', 'C', 0 },
    {  25, 'v', 'V', 0 }, {   5, 'b', 'B', 0 }, {  17, 'n', 'N', 0 },
    {  16, 'm', 'M', 0 }, {  54, ',', '<', 0 }, {  55, '.', '>', 0 },
    {  56, '/', '?', 0 }, { 135, 0, 0, 0 }, { 139, 0, 0, 0 },
    {  44, ' ', ' ', 0 }, { 138, 0, 0, 0 }, { 136, 0, 0, 0 },
    {  41, 0x1B, 0x1B, 0 }, {  84, '/', '/', 0 }, {  85, '*', '*', 0 },
    {  86, '-', '-', 0 }, {  87, '+', '+', 0 }, {  88, 0x0A, 0x0A, 0 },
    {  89, '1', '1', 0 }, {  90, '2', '2', 0 }, {  91, '3', '3', 0 },
    {  92, '4', '4', 0 }, {  93, '5', '5', 0 }, {  94, '6', '6', 0 },
    {  95, '7', '7', 0 }, {  96, '8', '8', 0 }, {  97, '9', '9', 0 },
    {  98, '0', '0', 0 }, {  99, '.', '.', 0 },
};

static const VIKeyMap sKeysUK[74] =
{
    {  53, '`', 0xAC, 0 }, {  30, '1', '!', 0 }, {  31, '2', '"', 0 },
    {  32, '3', 0xA3, 0 }, {  33, '4', '$', 0 }, {  34, '5', '%', 0 },
    {  35, '6', '^', 0 }, {  36, '7', '&', 0 }, {  37, '8', '*', 0 },
    {  38, '9', '(', 0 }, {  39, '0', ')', 0 }, {  45, '-', '_', 0 },
    {  46, '=', '+', 0 }, { 137, 0, 0, 0 }, {  42, 0x08, 0x08, 0 },
    {  43, 0x09, 0x09, 0 }, {  20, 'q', 'Q', 0 }, {  26, 'w', 'W', 0 },
    {   8, 'e', 'E', 0 }, {  21, 'r', 'R', 0 }, {  23, 't', 'T', 0 },
    {  28, 'y', 'Y', 0 }, {  24, 'u', 'U', 0 }, {  12, 'i', 'I', 0 },
    {  18, 'o', 'O', 0 }, {  19, 'p', 'P', 0 }, {  47, '[', '{', 0 },
    {  48, ']', '}', 0 }, {   4, 'a', 'A', 0 }, {  22, 's', 'S', 0 },
    {   7, 'd', 'D', 0 }, {   9, 'f', 'F', 0 }, {  10, 'g', 'G', 0 },
    {  11, 'h', 'H', 0 }, {  13, 'j', 'J', 0 }, {  14, 'k', 'K', 0 },
    {  15, 'l', 'L', 0 }, {  51, ';', ':', 0 }, {  52, '\'', '@', 0 },
    {  50, '#', '~', 0 }, {  40, 0x0A, 0x0A, 0 }, { 100, '\\', 0, 0 },
    {  29, 'z', 'Z', 0 }, {  27, 'x', 'X', 0 }, {   6, 'c', 'C', 0 },
    {  25, 'v', 'V', 0 }, {   5, 'b', 'B', 0 }, {  17, 'n', 'N', 0 },
    {  16, 'm', 'M', 0 }, {  54, ',', '<', 0 }, {  55, '.', '>', 0 },
    {  56, '/', '?', 0 }, { 135, 0, 0, 0 }, { 139, 0, 0, 0 },
    {  44, ' ', ' ', 0 }, { 138, 0, 0, 0 }, { 136, 0, 0, 0 },
    {  41, 0x1B, 0x1B, 0 }, {  84, '/', '/', 0 }, {  85, '*', '*', 0 },
    {  86, '-', '-', 0 }, {  87, '+', '+', 0 }, {  88, 0x0A, 0x0A, 0 },
    {  89, '1', '1', 0 }, {  90, '2', '2', 0 }, {  91, '3', '3', 0 },
    {  92, '4', '4', 0 }, {  93, '5', '5', 0 }, {  94, '6', '6', 0 },
    {  95, '7', '7', 0 }, {  96, '8', '8', 0 }, {  97, '9', '9', 0 },
    {  98, '0', '0', 0 }, {  99, '.', '.', 0 },
};

static VIKeyMap sKeysFR[75] =
{
    {  53, 0xB2, 0, 0 }, {  30, '&', '1', 0 }, {  31, 0xE9, '2', '~' },
    {  32, '"', '3', '#' }, {  33, '\'', '4', '{' }, {  34, '(', '5', '[' },
    {  35, '-', '6', '|' }, {  36, 0xE8, '7', '`' }, {  37, '_', '8', '\\' },
    {  38, 0xE7, '9', '^' }, {  39, 0xE0, '0', '@' }, {  45, ')', 0xB0, ']' },
    {  46, '=', '+', '}' }, { 137, 0, 0, 0 }, {  42, 0x08, 0x08, 0 },
    {  43, 0x09, 0x09, 0 }, {  20, 'a', 'A', 0 }, {  26, 'z', 'Z', 0 },
    {   8, 'e', 'E', 0x80 }, {  21, 'r', 'R', 0 }, {  23, 't', 'T', 0 },
    {  28, 'y', 'Y', 0 }, {  24, 'u', 'U', 0 }, {  12, 'i', 'I', 0 },
    {  18, 'o', 'O', 0 }, {  19, 'p', 'P', 0 }, {  47, '^', 0xA8, 0 },
    {  48, '$', 0xA3, 0xA4 }, {  49, '\\', '|', 0 }, {   4, 'q', 'Q', 0 },
    {  22, 's', 'S', 0 }, {   7, 'd', 'D', 0 }, {   9, 'f', 'F', 0 },
    {  10, 'g', 'G', 0 }, {  11, 'h', 'H', 0 }, {  13, 'j', 'J', 0 },
    {  14, 'k', 'K', 0 }, {  15, 'l', 'L', 0 }, {  51, 'm', 'M', 0 },
    {  52, 0xF9, '%', 0 }, {  50, '*', 0xB5, 0 }, {  40, 0x0A, 0x0A, 0 },
    { 100, '<', '>', 0 }, {  29, 'w', 'W', 0 }, {  27, 'x', 'X', 0 },
    {   6, 'c', 'C', 0 }, {  25, 'v', 'V', 0 }, {   5, 'b', 'B', 0 },
    {  17, 'n', 'N', 0 }, {  16, ',', '?', 0 }, {  54, ';', '.', 0 },
    {  55, ':', '/', 0 }, {  56, '!', 0xA7, 0 }, { 135, 0, 0, 0 },
    { 139, 0, 0, 0 }, {  44, ' ', ' ', 0 }, { 138, 0, 0, 0 },
    { 136, 0, 0, 0 }, {  41, 0x1B, 0x1B, 0 }, {  84, '/', '/', 0 },
    {  85, '*', '*', 0 }, {  86, '-', '-', 0 }, {  87, '+', '+', 0 },
    {  88, 0x0A, 0x0A, 0 }, {  89, '1', '1', 0 }, {  90, '2', '2', 0 },
    {  91, '3', '3', 0 }, {  92, '4', '4', 0 }, {  93, '5', '5', 0 },
    {  94, '6', '6', 0 }, {  95, '7', '7', 0 }, {  96, '8', '8', 0 },
    {  97, '9', '9', 0 }, {  98, '0', '0', 0 }, {  99, '.', '.', 0 },
};

static VIKeyMap sKeysDE[74] =
{
    {  53, '^', 0xB0, 0 }, {  30, '1', '!', 0 }, {  31, '2', '"', 0xB2 },
    {  32, '3', 0xA7, 0xB3 }, {  33, '4', '$', 0 }, {  34, '5', '%', 0 },
    {  35, '6', '&', 0 }, {  36, '7', '/', '{' }, {  37, '8', '(', '[' },
    {  38, '9', ')', ']' }, {  39, '0', '=', '}' }, {  45, 0xDF, '?', '\\' },
    {  46, 0xB4, '`', 0 }, { 137, 0, 0, 0 }, {  42, 0x08, 0x08, 0 },
    {  43, 0x09, 0x09, 0 }, {  20, 'q', 'Q', '@' }, {  26, 'w', 'W', 0 },
    {   8, 'e', 'E', 0x80 }, {  21, 'r', 'R', 0 }, {  23, 't', 'T', 0 },
    {  28, 'z', 'Z', 0 }, {  24, 'u', 'U', 0 }, {  12, 'i', 'I', 0 },
    {  18, 'o', 'O', 0 }, {  19, 'p', 'P', 0 }, {  47, 0xFC, 0xDC, 0 },
    {  48, '+', '*', '~' }, {   4, 'a', 'a', 0 }, {  22, 's', 's', 0 },
    {   7, 'd', 'd', 0 }, {   9, 'f', 'f', 0 }, {  10, 'g', 'g', 0 },
    {  11, 'h', 'h', 0 }, {  13, 'j', 'j', 0 }, {  14, 'k', 'k', 0 },
    {  15, 'l', 'l', 0 }, {  51, 0xF6, 0xD6, 0 }, {  52, 0xE4, 0xC4, 0 },
    {  50, '#', '\'', '~' }, {  40, 0x0A, 0x0A, 0 }, { 100, '<', '>', '|' },
    {  29, 'y', 'Y', 0 }, {  27, 'x', 'X', 0 }, {   6, 'c', 'C', 0 },
    {  25, 'v', 'V', 0 }, {   5, 'b', 'B', 0 }, {  17, 'n', 'N', 0 },
    {  16, 'm', 'M', 0xB5 }, {  54, ',', ';', 0 }, {  55, '.', ':', 0 },
    {  56, '-', '_', 0 }, { 135, 0, 0, 0 }, { 139, 0, 0, 0 },
    {  44, ' ', ' ', 0 }, { 138, 0, 0, 0 }, { 136, 0, 0, 0 },
    {  41, 0x1B, 0x1B, 0 }, {  84, '/', '/', 0 }, {  85, '*', '*', 0 },
    {  86, '-', '-', 0 }, {  87, '+', '+', 0 }, {  88, 0x0A, 0x0A, 0 },
    {  89, '1', '1', 0 }, {  90, '2', '2', 0 }, {  91, '3', '3', 0 },
    {  92, '4', '4', 0 }, {  93, '5', '5', 0 }, {  94, '6', '6', 0 },
    {  95, '7', '7', 0 }, {  96, '8', '8', 0 }, {  97, '9', '9', 0 },
    {  98, '0', '0', 0 }, {  99, ',', ',', 0 },
};

// The dead keys of each layout and their counts; US and UK have none, and
// their zero-length lists share an address with their counts.
static int sDeadUS[] = {};
static int sDeadUSCount = 0;
static int sDeadUK[] = {};
static int sDeadUKCount = 0;
static int sDeadFR[] = { '^', '`', '~', 0xA8 };
static int sDeadFRCount = 4;
static int sDeadDE[] = { '^', 0xB4, '`' };
static int sDeadDECount = 3;

int gKeyboardAttached;

static VIConsoleKey sKeyQueue[128];

// libkbd's read buffer: a count and up to twelve events.
struct _lgKbdEventBuf
{
    unsigned short m_x0;
    unsigned short m_count;     // +0x02
    _lgKbdEvent    m_events[12];// +0x04
};

int VIStartKeyboardDriver()
{
    if (lgKbmInit(1, 0, 1))
        return -1;
    return 0;
}

int VIStopKeyboardDriver()
{
    lgKbMPrepareForRebootIOP(); VI_HOST_ONLY(return 0;)
}

VIKeyboard::VIKeyboard()
{
    m_fd = -1;
}

VIKeyboard::~VIKeyboard()
{
}

int VIKeyboard::Init()
{
    gKeyboardAttached = 0;
    printf("======= INIT KEYBOARD =======\n");
    m_fd = -1;
    m_queue = sKeyQueue;
    m_numQueued = 0;
    m_x0 = 0;
    // Phase 16's hoist: the store emitted FIRST after the join is the source's
    // LAST store of that constant, so +0x210 closes the block.
    m_x210 = 0;
    return 0;
}

void VIKeyboard::Free()
{
    printf("======= FREE KEYBOARD =======\n");
    if (m_fd != -1)
    {
        lgKbdClose(m_fd);
        m_fd = -1;
    }
}

// Phase 61.  Opens the keyboard on the first call and polls it on every
// later one, then hands back one queued key.  Keypad keys with num lock OFF
// and the cursor block become the control codes 17..26 (17 left, 18 right,
// 19 up, 20 down, 21 page up, 22 page down, 23 end, 24 home, 25 insert,
// 26 delete); either Enter is 13, or 4 with shift.  Anything else goes through
// TranslateEvent on the way DOWN and is remembered per usage code, so the key
// comes back UP as the same character.
int VIKeyboard::Read(VIConsoleKey &k)
{
    char info[16];
    _lgKbdEventBuf buf;
    VIConsoleKey key;
    int hint;

    switch (m_x0)
    {
    case 0:
        gKeyboardAttached = 0;
        m_x210 = 0;
        if (lgKbdEnumerate(0, info) == 0)
        {
            if (lgKbdOpen(0, &m_fd) == 0)
            {
                lgKbdSetRepeatRate(0, 0);
                memset(m_keys, 0, sizeof(m_keys));
                m_numQueued = 0;
                m_x0 = 1;
            }
            else
                m_fd = -1;
        }
        break;

    case 1:
        gKeyboardAttached = 1;
        if (lgKbdEnumHint(&hint) != 0 || hint == 1)
        {
            lgKbdClose(m_fd);
            m_x0 = 0;
            break;
        }
        switch (lgKbdRead(m_fd, &buf))
        {
        // The sixth label is load-bearing: the target's tree checks
        // `r > 0x8000000D` before `r == 0`, which gcc emits only when the
        // node under 0x8000000D has a child.  Any of 0x80000007..0x8000000C
        // gives the same object code; the value is not recoverable.
        case (int)0x80000008:
            break;
        case (int)0x8000000D:
            printf("Keyboard Buffer Overflow? Resetting...\n");
        case (int)0x80000000:
        case (int)0x80000004:
        case (int)0x80000006:
            lgKbdClose(m_fd);
            m_x0 = 0;
            break;

        case 0:
            for (int i = 0; i < buf.m_count; i++)
            {
                unsigned short code = buf.m_events[i].m_code;
                int down;

                down = buf.m_events[i].m_state == 1;
                key.m_a = 0;

                if (code >= 83 && code <= 103)
                {
                    if (!(buf.m_events[i].m_locks & 1))
                    {
                        switch (code)
                        {
                        case 89: key.m_a = 23;  break;
                        case 90: key.m_a = 20;  break;
                        case 91: key.m_a = 22;  break;
                        case 92: key.m_a = 17;  break;
                        case 93: key.m_a = 255; break;
                        case 94: key.m_a = 18;  break;
                        case 95: key.m_a = 24;  break;
                        case 96: key.m_a = 19;  break;
                        case 97: key.m_a = 21;  break;
                        case 88: key.m_a = (buf.m_events[i].m_mods & 0x22) ? 4 : 13; break;
                        case 98:
                        case 99:
                            break;
                        }
                    }

                    if (key.m_a == 255)
                        key.m_a = 0;
                    else if (key.m_a == 0)
                    {
                        if (code == 88)
                            key.m_a = (buf.m_events[i].m_mods & 0x22) ? 4 : 13;
                        else
                        {
                            if (down)
                            {
                                key.m_a = TranslateEvent(&buf.m_events[i]);
                                m_keys[buf.m_events[i].m_code].m_a = key.m_a;
                            }
                            else
                            {
                                key.m_a = m_keys[buf.m_events[i].m_code].m_a;
                                m_keys[buf.m_events[i].m_code].m_a = 0;
                            }
                            if (key.m_a)
                                m_keys[buf.m_events[i].m_code].m_b = down;
                        }
                    }
                }
                else
                {
                    switch (buf.m_events[i].m_code)
                    {
                    case 80: key.m_a = 17; break;
                    case 82: key.m_a = 19; break;
                    case 79: key.m_a = 18; break;
                    case 81: key.m_a = 20; break;
                    case 75: key.m_a = 21; break;
                    case 78: key.m_a = 22; break;
                    case 77: key.m_a = 23; break;
                    case 74: key.m_a = 24; break;
                    case 73: key.m_a = 25; break;
                    case 76: key.m_a = 26; break;
                    case 40: key.m_a = (buf.m_events[i].m_mods & 0x22) ? 4 : 13; break;
                    default:
                        if (down)
                        {
                            key.m_a = TranslateEvent(&buf.m_events[i]);
                            m_keys[buf.m_events[i].m_code].m_a = key.m_a;
                        }
                        else
                        {
                            key.m_a = m_keys[buf.m_events[i].m_code].m_a;
                            m_keys[buf.m_events[i].m_code].m_a = 0;
                        }
                        m_keys[buf.m_events[i].m_code].m_b = down;
                        break;
                    }
                }

                key.m_b = down;
                if (key.m_a)
                    PrepareQueueKeyEvent(&key);
            }
            break;
        }
        break;
    }

    return DequeueKeyEvent(&k);
}

// Phase 61.  One event to a character through the region's layout: shift
// picks the second column, right-alt the third, and caps lock upper-cases
// a..z only.  0 when the usage code is not in the layout.
unsigned char VIKeyboard::TranslateEvent(_lgKbdEvent *e)
{
    int n = 0;
    unsigned char c = 0;
    const VIKeyMap *map;

    VIGetGameOptions();
    switch ((char)VIGetGameOptions()->m_region)
    {
    case 0: n = 74; map = sKeysUS; break;
    case 1: n = 74; map = sKeysUK; break;
    case 2: n = 75; map = sKeysFR; break;
    case 3: n = 74; map = sKeysDE; break;
    }

    for (int i = 0; i < n; i++)
    {
        if (e->m_code == (unsigned char)map[i].m_code)
        {
            if (e->m_mods & 0x22)
                c = map[i].m_shift;
            else if (e->m_mods & 0x40)
                c = map[i].m_alt;
            else
                c = map[i].m_plain;

            if ((e->m_locks & 2) && c >= 'a' && c <= 'z')
                c -= 32;
            break;
        }
    }
    return c;
}

// Phase 61.  A dead key is held back until the next press: combined with it
// when CombineDeadkey knows the pair, or else sent as itself followed by the
// key.  Every character goes out as a press AND a release.
void VIKeyboard::PrepareQueueKeyEvent(VIConsoleKey *k)
{
    int *dead = 0;
    int n = 0;
    int i;
    VIConsoleKey q;

    VIGetGameOptions();
    switch ((char)VIGetGameOptions()->m_region)
    {
    case 0: dead = sDeadUS; n = sDeadUSCount; break;
    case 1: dead = sDeadUK; n = sDeadUKCount; break;
    case 2: dead = sDeadFR; n = sDeadFRCount; break;
    case 3: dead = sDeadDE; n = sDeadDECount; break;
    }

    if (k->m_b)
    {
        if (m_x210)
        {
            m_x214 = k->m_a;

            unsigned char c = CombineDeadkey(k->m_a);

            if (c == 0)
            {
                q.m_a = m_x210;
                q.m_b = 1;
                QueueKeyEvent(&q);
                q.m_b = 0;
                QueueKeyEvent(&q);
                q.m_a = k->m_a;
                q.m_b = 1;
                QueueKeyEvent(&q);
                q.m_b = 0;
                QueueKeyEvent(&q);
            }
            else
            {
                q.m_a = c;
                q.m_b = 1;
                QueueKeyEvent(&q);
                q.m_b = 0;
                QueueKeyEvent(&q);
            }
            m_x210 = 0;
        }
        else
        {
            for (i = 0; i < n; i++)
                if (k->m_a == dead[i])
                {
                    m_x210 = k->m_a;
                    break;
                }
            if (m_x210 == 0)
                QueueKeyEvent(k);
        }
    }
    else
    {
        int found = 0;

        for (i = 0; i < n; i++)
            if (k->m_a == dead[i])
            {
                found = k->m_a;
                break;
            }
        if (!found && m_x214 != k->m_a)
            QueueKeyEvent(k);
        if (m_x214 == k->m_a)
            m_x214 = 0;
    }
}

// Phase 61.  The Latin-1 letter for a dead key and a base letter, 0 for a
// pair it does not know.  Shipped: diaeresis + 'Y' gives 0xDF, which is
// German sharp s, not Y-diaeresis (Latin-1 has none).
unsigned char VIKeyboard::CombineDeadkey(unsigned char c)
{
    unsigned char r = 0;

    switch (m_x210)
    {
    case '^':
        switch (c)
        {
        case 'a': r = 0xE2; break;
        case 'A': r = 0xC2; break;
        case 'e': r = 0xEA; break;
        case 'E': r = 0xCA; break;
        case 'i': r = 0xEE; break;
        case 'I': r = 0xCE; break;
        case 'o': r = 0xF4; break;
        case 'O': r = 0xD4; break;
        case 'u': r = 0xFB; break;
        case 'U': r = 0xDB; break;
        }
        break;

    case 0xB4:
        switch (c)
        {
        case 'a': r = 0xE1; break;
        case 'A': r = 0xC1; break;
        case 'e': r = 0xE9; break;
        case 'E': r = 0xC9; break;
        case 'i': r = 0xED; break;
        case 'I': r = 0xCD; break;
        case 'o': r = 0xF3; break;
        case 'O': r = 0xD3; break;
        case 'u': r = 0xFA; break;
        case 'U': r = 0xDA; break;
        case 'y': r = 0xFD; break;
        case 'Y': r = 0xDD; break;
        }
        break;

    case '`':
        switch (c)
        {
        case 'a': r = 0xE0; break;
        case 'A': r = 0xC0; break;
        case 'e': r = 0xE8; break;
        case 'E': r = 0xC8; break;
        case 'i': r = 0xEC; break;
        case 'I': r = 0xCC; break;
        case 'o': r = 0xF2; break;
        case 'O': r = 0xD2; break;
        case 'u': r = 0xF9; break;
        case 'U': r = 0xD9; break;
        }
        break;

    case 0xA8:
        switch (c)
        {
        case 'a': r = 0xE4; break;
        case 'A': r = 0xC4; break;
        case 'e': r = 0xEB; break;
        case 'E': r = 0xCB; break;
        case 'i': r = 0xEF; break;
        case 'I': r = 0xCF; break;
        case 'o': r = 0xF6; break;
        case 'O': r = 0xD6; break;
        case 'u': r = 0xFC; break;
        case 'U': r = 0xDC; break;
        case 'y': r = 0xFF; break;
        case 'Y': r = 0xDF; break;
        }
        break;

    case '~':
        switch (c)
        {
        case 'a': r = 0xE3; break;
        case 'A': r = 0xC3; break;
        case 'n': r = 0xF1; break;
        case 'N': r = 0xD1; break;
        case 'o': r = 0xF5; break;
        case 'O': r = 0xD5; break;
        }
        break;
    }
    return r;
}

void VIKeyboard::QueueKeyEvent(VIConsoleKey *k)
{
    // The count is read ONCE -- the same lever as VISpellEffectDriver's
    // AddSecondaryTarget in phase 35: re-reading the member for the increment
    // costs 13 of 15 words and moves `this` onto its own register.
    int n = m_numQueued;

    if (n < 128)
    {
        m_queue[n] = *k;
        m_numQueued = n + 1;
    }
}

int VIKeyboard::DequeueKeyEvent(VIConsoleKey *k)
{
    int i;

    // Phase 7's if-direction: the DRAIN is the `if` body and the empty case is
    // the tail, which is what puts `return 0` at the very end of the function.
    // Written as an early `== 0` return gcc keeps it inline behind a
    // branch-likely and 27 of 29 words move.
    if (m_numQueued != 0)
    {
        *k = m_queue[0];
        m_numQueued--;

        for (i = 0; i < m_numQueued; i++)
            m_queue[i] = m_queue[i + 1];

        return 1;
    }
    return 0;
}
