// The XM player: SNDDRV 0x402C .. 0x8C80.
//
// A module is a FastTracker 2 song re-packed by the EE tools: a 28-byte
// header, the order table, 12-byte pattern descriptors, 224-byte instruments
// and 24-byte sample records, with the patterns' rows kept in XM's own packed
// form and the samples already in SPU RAM as ADPCM (XmRelocate fixes the
// pointers once both have arrived).  A player is stepped once per tracker tick
// by the driver's timer thread; each tick it reads a row or runs the effects,
// then writes what changed to its voices, and the key-ons and key-offs it
// collected go to libsd in one switch write per core.
#include "xm.h"

// ---- the voices ---------------------------------------------------------------

int XmVoices::Init(u32 lo, u32 sampleBase, u32 hi)
{
    int i;
    int n;
    u32 mask0;
    u32 mask1;

    m_sampleBase = sampleBase;
    for (i = 0; i < 32; i++)
    {
        m_ch[i].m_04 = 0;
        m_ch[i].m_08 = 0;
        m_ch[i].m_0c = 0;
        m_ch[i].m_10 = 0;
        m_ch[i].m_14 = 0;
        m_ch[i].m_18 = 0;
        m_ch[i].m_addr = 0;
    }
    n = 0;
    mask0 = lo;
    mask1 = hi;
    for (i = 0; i < 24; i++)
    {
        if ((1 << i) & mask0)
            m_ch[n++].m_voice = i;
    }
    for (i = 0; i < 24; i++)
    {
        if ((1 << i) & mask1)
            m_ch[n++].m_voice = i + 24;
    }
    m_keyOn[0] = 0;
    m_keyOn[1] = 0;
    m_keyOff[0] = 0;
    m_keyOff[1] = 0;
    return 0;
}

void XmVoices::Reset()
{
}

int XmVoices::Wait()
{
    return -1;
}

int XmVoices::Busy()
{
    return 0;
}

int XmVoices::Wait2()
{
    return -1;
}

int XmVoices::Elapsed()
{
    return 0;
}

int XmVoices::Flush(int ticks)
{
    if (m_keyOn[0])
        sceSdSetSwitch(0x1500, m_keyOn[0]);
    if (m_keyOn[1])
        sceSdSetSwitch(0x1501, m_keyOn[1]);
    if (m_keyOff[0])
        sceSdSetSwitch(0x1600, m_keyOff[0]);
    if (m_keyOff[1])
        sceSdSetSwitch(0x1601, m_keyOff[1]);
    m_keyOn[0] = 0;
    m_keyOn[1] = 0;
    m_keyOff[0] = 0;
    m_keyOff[1] = 0;
    return 0;
}

int XmVoices::KeyOn(int ch, u32 addr, int length, u32 loopType, int loopStart, u32 rate,
                    int volL, int volR)
{
    XmVoice *v = &m_ch[ch];
    int core;
    int voice;

    v->m_addr = addr;
    if (addr == 0)
        return -1;
    if (v->m_voice < 24)
    {
        m_keyOn[0] |= 1 << v->m_voice;
        core = 0;
        voice = v->m_voice;
    }
    else
    {
        m_keyOn[1] |= 1 << (v->m_voice - 24);
        core = 1;
        voice = v->m_voice - 24;
    }
    sceSdSetParam(core | (voice << 1), volL << 6);
    sceSdSetParam(core | (voice << 1) | 0x100, volR << 6);
    sceSdSetParam(core | (voice << 1) | 0x200, (int)(rate * 4096) / 48000);
    sceSdSetAddr(core | (voice << 1) | 0x2040, addr);
    v->m_loopStart = loopStart;
    v->m_loopType = loopType;
    return 0;
}

int XmVoices::KeyOff(int ch)
{
    XmVoice *v = &m_ch[ch];

    if (v->m_voice < 24)
        m_keyOff[0] |= 1 << v->m_voice;
    else
        m_keyOff[1] |= 1 << (v->m_voice - 24);
    return 0;
}

int XmVoices::SetPitch(int ch, u32 rate)
{
    XmVoice *v = &m_ch[ch];
    int core;
    int voice;

    if (v->m_addr)
    {
        if (v->m_voice < 24)
        {
            core = 0;
            voice = v->m_voice;
        }
        else
        {
            core = 1;
            voice = v->m_voice - 24;
        }
        sceSdSetParam(core | (voice << 1) | 0x200, (int)(rate * 4096) / 48000);
    }
    return 0;
}

int XmVoices::Nop(int ch, int x)
{
    return -1;
}

int XmVoices::SetVolume(int ch, int volL, int volR)
{
    XmVoice *v = &m_ch[ch];
    int core;
    int voice;

    if (v->m_addr)
    {
        if (v->m_voice < 24)
        {
            core = 0;
            voice = v->m_voice;
        }
        else
        {
            core = 1;
            voice = v->m_voice - 24;
        }
        sceSdSetParam(core | (voice << 1), volL << 6);
        sceSdSetParam(core | (voice << 1) | 0x100, volR << 6);
    }
    return 0;
}

// A sample offset in samples becomes ADPCM bytes: 28 samples to a 16-byte
// block, rounded down to two blocks.
int XmVoices::SetOffset(int ch, u32 samples)
{
    XmVoice *v = &m_ch[ch];
    int core;
    int voice;
    u32 off;

    if (v->m_addr)
    {
        if (v->m_voice < 24)
        {
            core = 0;
            voice = v->m_voice;
        }
        else
        {
            core = 1;
            voice = v->m_voice - 24;
        }
        off = samples * 2;
        off /= 7;
        off *= 2;
        off &= ~0x1f;
        sceSdSetAddr(core | (voice << 1) | 0x2040, v->m_addr + off);
        if (v->m_loopType)
        {
            off = v->m_loopType;
            off *= 2;
            off /= 7;
            off &= ~0x1f;
            sceSdSetAddr(core | (voice << 1) | 0x2140, v->m_addr + off);
        }
    }
    return 0;
}

// ---- the player -----------------------------------------------------------------
// A constructor is the first function in the TU to return `this`, and from it
// on every function ends in one `j` to its epilogue instead of the three
// before it: the compiler's own state, not the source.

XmPlayer::XmPlayer()
{
    m_lo = 0;
    m_module = 0;
    m_voices = 0;
}

XmPlayer::~XmPlayer()
{
    Stop();
}

// Always answers -1, started or not; the driver ignores it.
int XmPlayer::Play(u32 lo, XmModule *module, u32 hi)
{
    m_module = module;
    m_lo = lo;
    m_voices = new XmVoices;
    if (m_voices->Init(lo, module->Channels(), hi) < 0)
        return -1;
    m_repeat = 0;
    Restart();
    m_done = 0;
    m_volume = 255;
    m_pan = 128;
    m_pitch = 4096;
    return -1;
}

void XmPlayer::Stop()
{
    if (m_voices)
    {
        delete m_voices;
        m_voices = 0;
    }
}

void XmPlayer::Restart()
{
    int i;

    m_speed = m_module->m_speed;
    m_tempo = m_module->m_tempo;
    m_tick = m_speed;
    m_row = -1;
    m_order = 0;
    m_seek = 1;
    m_pattern = m_module->m_order[m_order];
    m_rowData = m_module->m_pattern[m_pattern].m_data;
    m_rowPos = 0;
    m_rows = m_module->m_pattern[m_pattern].m_size;
    m_breakRow = -1;
    m_jumpOrder = -1;
    m_globalVol = 128;
    for (i = 0; i < 32; i++)
        memset(&m_ch[i], 0, 0x68);
}

// One tracker tick.  0 once the song is over (or never started), else 1.
// The arm that would have run several ticks to catch up on lost time is
// compiled but dead: `if (1)` at -O0 still expands its else.
int XmPlayer::Process()
{
    int ticks;

    if (m_done == 1)
        return 0;
    if (m_voices == 0)
        return 0;
    if (1)
    {
        Tick();
        m_voices->Flush(0);
    }
    else
    {
        ticks = 0x5964baabU / (m_tempo << 1);
        while (m_voices->Elapsed() > ticks)
        {
            Tick();
            m_voices->Flush(ticks);
        }
    }
    if (m_voices->Busy() == 0)
        m_voices->Wait();
    return 1;
}

void XmPlayer::Tick()
{
    int p;
    int ch;

    m_tick++;
    if (m_tick >= m_speed)
    {
        m_tick = 0;
        m_row++;
        p = m_module->m_order[m_order];
        m_seek = 0;
        if (m_row == m_module->m_pattern[p].m_rows || m_breakRow != -1 || m_jumpOrder != -1)
        {
            m_order++;
            if (m_order >= m_module->m_length)
            {
                if (m_repeat)
                    Restart();
                else
                    m_done = 1;
                return;
            }
            p = m_module->m_order[m_order];
            m_pattern = p;
            if (m_breakRow != -1)
            {
                m_row = m_breakRow;
                if (m_row)
                    m_seek = 1;
            }
            else
                m_row = 0;
            if (m_jumpOrder != -1)
            {
                m_row = 0;
                if (m_jumpOrder < m_module->m_length)
                    m_order = m_jumpOrder;
            }
            m_tick = 0;
            m_rowData = m_module->m_pattern[m_pattern].m_data;
            m_rowPos = 0;
            m_rows = m_module->m_pattern[m_pattern].m_size;
            m_breakRow = -1;
            m_jumpOrder = -1;
        }
        if (m_rows)
        {
            // a pattern break into the middle of a pattern skips to its row --
            // but it is handed m_rowPos, which was just set to 0
            if (m_seek)
                SkipRows(m_rowPos);
            for (ch = 0; ch < m_module->m_channels; ch++)
                ReadRow(ch);
        }
    }
    else
        TickEffects();
    Update();
}

// Step the packed pointer over `rows` rows.  An unpacked note is skipped as
// SIX bytes, its five plus the flag byte counted twice.
void XmPlayer::SkipRows(int rows)
{
    int r;
    int c;
    u8 flags;
    u8 *p = m_rowData;

    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < m_module->m_channels; c++)
        {
            flags = *p++;
            if (flags == 0x80)
                continue;
            if (flags & 0x80)
            {
                if (flags & 1)
                    p++;
                if (flags & 2)
                    p++;
                if (flags & 4)
                    p++;
                if (flags & 8)
                    p++;
                if (flags & 0x10)
                    p++;
            }
            else
                p += 5;
        }
    }
    m_rowPos = p - m_rowData;
}

// One channel's slot of the row: XM's packing (bit 7 set: bits 0..4 say which
// of note, instrument, volume, effect and parameter follow), a new note and
// instrument looked up, then the effects run for tick 0.
void XmPlayer::ReadRow(int chan)
{
    XmChannel *c;
    u8 *p;
    u8 flags;

    c = &m_ch[chan];
    p = m_rowData + m_rowPos;
    flags = *p++;
    c->m_note = 0;
    c->m_ins = 0;
    c->m_vol = 0;
    c->m_fx = 0;
    c->m_param = 0;
    if (flags == 0x80)
        ;
    else
    {
        if (flags & 0x80)
        {
            if (flags & 1)
                c->m_note = *p++;
            if (flags & 2)
                c->m_ins = *p++;
            if (flags & 4)
                c->m_vol = *p++;
            if (flags & 8)
                c->m_fx = *p++;
            if (flags & 0x10)
                c->m_param = *p++;
        }
        else
        {
            c->m_note = flags;
            c->m_ins = *p++;
            c->m_vol = *p++;
            c->m_fx = *p++;
            c->m_param = *p++;
        }
        if (c->m_note == 97)
            KeyOff(c);
        else
        {
            if (c->m_note)
            {
                c->m_fadeVol = 0x7fff;
                c->m_key = c->m_note - 1;
                c->SetKey(1);
            }
            if (c->m_ins)
            {
                c->m_sample = 0;
                if (c->m_ins <= m_module->m_instruments)
                {
                    c->m_inst = &m_module->m_instrument[c->m_ins - 1];
                    if (c->m_inst->m_samples)
                    {
                        c->m_sample = &m_module->m_sample[c->m_inst->m_firstSample
                                                          + c->m_inst->m_map[c->m_key]];
                        if (c->m_sample)
                        {
                            c->m_pan = c->m_sample->m_pan;
                            c->m_volume = c->m_sample->m_volume;
                            c->m_relNote = c->m_sample->m_relNote;
                            c->m_finetune = c->m_sample->m_finetune;
                            c->Reset();
                        }
                    }
                }
            }
            if (c->m_note)
            {
                c->m_target = Period((u8)(c->m_key + c->m_relNote), c->m_finetune);
                c->m_period = c->m_target;
                if (c->m_inst)
                {
                    if ((u8)(c->m_inst->m_volEnv.m_flags % 2))
                        c->m_volEnv = &c->m_inst->m_volEnv;
                    else
                        c->m_volEnv = 0;
                    if ((u8)(c->m_inst->m_panEnv.m_flags % 2))
                        c->m_panEnv = &c->m_inst->m_panEnv;
                    else
                        c->m_panEnv = 0;
                    c->m_fadeout = c->m_inst->m_fadeout;
                }
            }
        }
        Effects(c);
    }
    m_rowPos = p - m_rowData;
}

// The ticks between rows run only the effects.
void XmPlayer::TickEffects()
{
    int i;

    for (i = 0; i < m_module->m_channels; i++)
        Effects(&m_ch[i]);
}

// ---- the effects -------------------------------------------------------------
// FastTracker 2's tables, in .data: the linear-frequency table (one octave in
// 768 steps), the Amiga period table (12 notes x 8 finetunes, plus one octave
// row of guard) and the vibrato/tremolo sine.
int gXmLinearTable[768] = {
    535232, 534749, 534266, 533784, 533303, 532822, 532341, 531861,
    531381, 530902, 530423, 529944, 529466, 528988, 528511, 528034,
    527558, 527082, 526607, 526131, 525657, 525183, 524709, 524236,
    523763, 523290, 522818, 522346, 521875, 521404, 520934, 520464,
    519994, 519525, 519057, 518588, 518121, 517653, 517186, 516720,
    516253, 515788, 515322, 514858, 514393, 513929, 513465, 513002,
    512539, 512077, 511615, 511154, 510692, 510232, 509771, 509312,
    508852, 508393, 507934, 507476, 507018, 506561, 506104, 505647,
    505191, 504735, 504280, 503825, 503371, 502917, 502463, 502010,
    501557, 501104, 500652, 500201, 499749, 499298, 498848, 498398,
    497948, 497499, 497050, 496602, 496154, 495706, 495259, 494812,
    494366, 493920, 493474, 493029, 492585, 492140, 491696, 491253,
    490809, 490367, 489924, 489482, 489041, 488600, 488159, 487718,
    487278, 486839, 486400, 485961, 485522, 485084, 484647, 484210,
    483773, 483336, 482900, 482465, 482029, 481595, 481160, 480726,
    480292, 479859, 479426, 478994, 478562, 478130, 477699, 477268,
    476837, 476407, 475977, 475548, 475119, 474690, 474262, 473834,
    473407, 472979, 472553, 472126, 471701, 471275, 470850, 470425,
    470001, 469577, 469153, 468730, 468307, 467884, 467462, 467041,
    466619, 466198, 465778, 465358, 464938, 464518, 464099, 463681,
    463262, 462844, 462427, 462010, 461593, 461177, 460760, 460345,
    459930, 459515, 459100, 458686, 458272, 457859, 457446, 457033,
    456621, 456209, 455797, 455386, 454975, 454565, 454155, 453745,
    453336, 452927, 452518, 452110, 451702, 451294, 450887, 450481,
    450074, 449668, 449262, 448857, 448452, 448048, 447644, 447240,
    446836, 446433, 446030, 445628, 445226, 444824, 444423, 444022,
    443622, 443221, 442821, 442422, 442023, 441624, 441226, 440828,
    440430, 440033, 439636, 439239, 438843, 438447, 438051, 437656,
    437261, 436867, 436473, 436079, 435686, 435293, 434900, 434508,
    434116, 433724, 433333, 432942, 432551, 432161, 431771, 431382,
    430992, 430604, 430215, 429827, 429439, 429052, 428665, 428278,
    427892, 427506, 427120, 426735, 426350, 425965, 425581, 425197,
    424813, 424430, 424047, 423665, 423283, 422901, 422519, 422138,
    421757, 421377, 420997, 420617, 420237, 419858, 419479, 419101,
    418723, 418345, 417968, 417591, 417214, 416838, 416462, 416086,
    415711, 415336, 414961, 414586, 414212, 413839, 413465, 413092,
    412720, 412347, 411975, 411604, 411232, 410862, 410491, 410121,
    409751, 409381, 409012, 408643, 408274, 407906, 407538, 407170,
    406803, 406436, 406069, 405703, 405337, 404971, 404606, 404241,
    403876, 403512, 403148, 402784, 402421, 402058, 401695, 401333,
    400970, 400609, 400247, 399886, 399525, 399165, 398805, 398445,
    398086, 397727, 397368, 397009, 396651, 396293, 395936, 395579,
    395222, 394865, 394509, 394153, 393798, 393442, 393087, 392733,
    392378, 392024, 391671, 391317, 390964, 390612, 390259, 389907,
    389556, 389204, 388853, 388502, 388152, 387802, 387452, 387102,
    386753, 386404, 386056, 385707, 385359, 385012, 384664, 384317,
    383971, 383624, 383278, 382932, 382587, 382242, 381897, 381552,
    381208, 380864, 380521, 380177, 379834, 379492, 379149, 378807,
    378466, 378124, 377783, 377442, 377102, 376762, 376422, 376082,
    375743, 375404, 375065, 374727, 374389, 374051, 373714, 373377,
    373040, 372703, 372367, 372031, 371695, 371360, 371025, 370690,
    370356, 370022, 369688, 369355, 369021, 368688, 368356, 368023,
    367691, 367360, 367028, 366697, 366366, 366036, 365706, 365376,
    365046, 364717, 364388, 364059, 363731, 363403, 363075, 362747,
    362420, 362093, 361766, 361440, 361114, 360788, 360463, 360137,
    359813, 359488, 359164, 358840, 358516, 358193, 357869, 357547,
    357224, 356902, 356580, 356258, 355937, 355616, 355295, 354974,
    354654, 354334, 354014, 353695, 353376, 353057, 352739, 352420,
    352103, 351785, 351468, 351150, 350834, 350517, 350201, 349885,
    349569, 349254, 348939, 348624, 348310, 347995, 347682, 347368,
    347055, 346741, 346429, 346116, 345804, 345492, 345180, 344869,
    344558, 344247, 343936, 343626, 343316, 343006, 342697, 342388,
    342079, 341770, 341462, 341154, 340846, 340539, 340231, 339924,
    339618, 339311, 339005, 338700, 338394, 338089, 337784, 337479,
    337175, 336870, 336566, 336263, 335959, 335656, 335354, 335051,
    334749, 334447, 334145, 333844, 333542, 333242, 332941, 332641,
    332341, 332041, 331741, 331442, 331143, 330844, 330546, 330247,
    329950, 329652, 329355, 329057, 328761, 328464, 328168, 327872,
    327576, 327280, 326985, 326690, 326395, 326101, 325807, 325513,
    325219, 324926, 324633, 324340, 324047, 323755, 323463, 323171,
    322879, 322588, 322297, 322006, 321716, 321426, 321136, 320846,
    320557, 320267, 319978, 319690, 319401, 319113, 318825, 318538,
    318250, 317963, 317676, 317390, 317103, 316817, 316532, 316246,
    315961, 315676, 315391, 315106, 314822, 314538, 314254, 313971,
    313688, 313405, 313122, 312839, 312557, 312275, 311994, 311712,
    311431, 311150, 310869, 310589, 310309, 310029, 309749, 309470,
    309190, 308911, 308633, 308354, 308076, 307798, 307521, 307243,
    306966, 306689, 306412, 306136, 305860, 305584, 305308, 305033,
    304758, 304483, 304208, 303934, 303659, 303385, 303112, 302838,
    302565, 302292, 302019, 301747, 301475, 301203, 300931, 300660,
    300388, 300117, 299847, 299576, 299306, 299036, 298766, 298497,
    298227, 297958, 297689, 297421, 297153, 296884, 296617, 296349,
    296082, 295815, 295548, 295281, 295015, 294749, 294483, 294217,
    293952, 293686, 293421, 293157, 292892, 292628, 292364, 292100,
    291837, 291574, 291311, 291048, 290785, 290523, 290261, 289999,
    289737, 289476, 289215, 288954, 288693, 288433, 288173, 287913,
    287653, 287393, 287134, 286875, 286616, 286358, 286099, 285841,
    285583, 285326, 285068, 284811, 284554, 284298, 284041, 283785,
    283529, 283273, 283017, 282762, 282507, 282252, 281998, 281743,
    281489, 281235, 280981, 280728, 280475, 280222, 279969, 279716,
    279464, 279212, 278960, 278708, 278457, 278206, 277955, 277704,
    277453, 277203, 276953, 276703, 276453, 276204, 275955, 275706,
    275457, 275209, 274960, 274712, 274465, 274217, 273970, 273722,
    273476, 273229, 272982, 272736, 272490, 272244, 271999, 271753,
    271508, 271263, 271018, 270774, 270530, 270286, 270042, 269798,
    269555, 269312, 269069, 268826, 268583, 268341, 268099, 267857,
};

u16 gXmPeriodTable[104] = {
    29024, 28800, 28608, 28384, 28192, 28000, 27776, 27584,
    27392, 27200, 27008, 26816, 26624, 26432, 26240, 26048,
    25856, 25664, 25472, 25312, 25120, 24928, 24768, 24576,
    24384, 24224, 24064, 23872, 23712, 23552, 23360, 23200,
    23040, 22880, 22688, 22528, 22368, 22208, 22048, 21888,
    21696, 21600, 21440, 21280, 21120, 20960, 20832, 20672,
    20480, 20352, 20224, 20096, 19936, 19808, 19648, 19520,
    19328, 19232, 19104, 18944, 18816, 18688, 18560, 18400,
    18240, 18144, 18016, 17888, 17760, 17632, 17504, 17376,
    17216, 17120, 17024, 16896, 16768, 16640, 16512, 16416,
    16256, 16160, 16064, 15936, 15808, 15712, 15584, 15488,
    15360, 15264, 15168, 15040, 14944, 14816, 14720, 14624,
    14496, 14400, 14304, 14176, 14080, 13984, 13888, 13792,
};

u8 gXmSine[32] = {
      0,  24,  49,  74,  97, 120, 141, 161,
    180, 197, 212, 224, 235, 244, 250, 253,
    255, 253, 250, 244, 235, 224, 212, 197,
    180, 161, 141, 120,  97,  74,  49,  24,
};

#define XMMIN(a, b) ((a) < (b) ? (a) : (b))
#define XMMAX(a, b) ((a) > (b) ? (a) : (b))

// Tick 0 runs a note's effects with m_tick == 0; slides act on later ticks.
// The volume column is decoded here too: 0x10..0x50 set the volume, 0x60.. are
// the column's own slides, vibrato, panning and portamento.
void XmPlayer::Effects(XmChannel *c)
{
    u8 vol;
    u8 fx;
    u8 param;

    c->m_periodSet = 0;
    c->m_volumeSet = 0;
    vol = c->m_vol;
    fx = c->m_fx;
    param = c->m_param;
    switch (vol >> 4)
    {
    case 6:
        if (vol & 0xf)
            c->m_volColSlide = vol & 0xf;
        if (m_tick)
        {
            c->m_volume = c->m_volume - c->m_volColSlide;
            c->m_volume = XMMAX(c->m_volume, 0);
        }
        break;
    case 7:
        if (vol & 0xf)
            c->m_volColSlide = vol & 0xf;
        if (m_tick)
        {
            c->m_volume = c->m_volume + c->m_volColSlide;
            c->m_volume = XMMIN(c->m_volume, 64);
        }
        break;
    case 8:
        if (vol & 0xf)
            c->m_volColSlide = vol & 0xf;
        if (m_tick == 0)
        {
            c->m_volume = c->m_volume - c->m_volColSlide;
            c->m_volume = XMMAX(c->m_volume, 0);
        }
        break;
    case 9:
        if (vol & 0xf)
            c->m_volColSlide = vol & 0xf;
        if (m_tick == 0)
        {
            c->m_volume = c->m_volume + c->m_volColSlide;
            c->m_volume = XMMIN(c->m_volume, 64);
        }
        break;
    case 10:
        Fx(c, 4, vol << 4);
        break;
    case 11:
        Fx(c, 4, vol & 0xf);
        break;
    case 12:
        c->m_pan = (vol & 0xf) << 4;
        break;
    case 13:
        if (vol & 0xf)
            c->m_panColSlide = vol & 0xf;
        if (m_tick)
        {
            c->m_pan = c->m_pan - (c->m_panColSlide << 2);
            c->m_pan = XMMAX(c->m_pan, 0);
        }
        break;
    case 14:
        if (vol & 0xf)
            c->m_panColSlide = vol & 0xf;
        if (m_tick)
        {
            c->m_pan = c->m_pan + (c->m_panColSlide << 2);
            c->m_pan = XMMIN(c->m_pan, 255);
        }
        break;
    case 15:
        Fx(c, 3, vol << 4);
        break;
    default:
        if (vol >= 0x10 && vol <= 0x50)
            Fx(c, 12, vol - 0x10);
        break;
    }
    switch (fx)
    {
    case 0x10:                          // G: global volume
        m_globalVol = param << 1;
        break;
    case 0x11:                          // H: global volume slide
        break;
    case 0x14:                          // K: key off
        KeyOff(c);
        break;
    case 0x15:                          // L: envelope position
        break;
    case 0x19:                          // P: panning slide
        c->PanSlide(m_tick, param);
        break;
    case 0x1b:                          // R: retrigger
        break;
    case 0x1d:                          // T: tremor
        break;
    case 0x21:                          // X: extra fine portamento
        break;
    default:
        if (fx < 0x10)
            Fx(c, fx, param);
        break;
    }
    if (!c->m_periodSet)
        c->m_outPeriod = c->m_period;
    if (!c->m_volumeSet)
        c->m_outVolume = c->m_volume;
}

// Effects 0..F.
void XmPlayer::Fx(XmChannel *chan, u8 fx, u8 param)
{
    XmChannel *c;
    u8 cmd;
    u8 arg;
    u8 note;

    c = chan;
    cmd = fx;
    arg = param;
    switch (cmd)
    {
    case 0:                             // arpeggio
        if (arg)
        {
            note = c->m_key;
            switch (m_tick % 3)
            {
            case 1:
                note += arg >> 4;
                break;
            case 2:
                note += arg & 0xf;
                break;
            }
            c->m_outPeriod = Period((u8)(note + c->m_relNote), c->m_finetune);
            c->m_periodSet = 1;
        }
        break;
    case 1:                             // portamento up
        if (arg)
            c->m_slide = arg << 2;
        if (m_tick)
            c->m_period = c->m_period - c->m_slide;
        break;
    case 2:                             // portamento down
        if (arg)
            c->m_slide = arg << 2;
        if (m_tick)
            c->m_period = c->m_period + c->m_slide;
        break;
    case 3:                             // tone portamento
        c->m_porta = 1;
        if (arg)
        {
            c->m_portaSpeed = arg;
            c->m_portaSpeed = c->m_portaSpeed << 2;
        }
        c->Porta();
        c->m_periodSet = 1;
        break;
    case 4:                             // vibrato
        if (arg & 0xf)
            c->m_vibDepth = arg & 0xf;
        if (arg & 0xf0)
            c->m_vibSpeed = (arg & 0xf0) >> 2;
        c->Vibrato(m_tick);
        c->m_periodSet = 1;
        break;
    case 5:                             // tone portamento + volume slide
        c->m_porta = 1;
        c->Porta();
        if (arg)
            c->m_volSlide = arg;
        c->VolSlide(m_tick);
        c->m_periodSet = 1;
        break;
    case 6:                             // vibrato + volume slide
        c->Vibrato(m_tick);
        if (arg)
            c->m_volSlide = arg;
        c->VolSlide(m_tick);
        c->m_periodSet = 1;
        break;
    case 7:                             // tremolo
        if (arg & 0xf)
            c->m_tremDepth = arg & 0xf;
        if (arg & 0xf0)
            c->m_tremSpeed = (arg & 0xf0) >> 2;
        c->Tremolo(m_tick);
        c->m_volumeSet = 1;
        break;
    case 8:                             // panning
        c->m_pan = arg;
        break;
    case 9:                             // sample offset
        if (m_tick)
            break;
        if (arg)
            c->m_offset = arg * 256;
        else
            c->m_offset = c->m_lastOffset;
        break;
    case 10:                            // volume slide
        if (arg)
            c->m_volSlide = arg;
        c->VolSlide(m_tick);
        break;
    case 11:                            // position jump
        m_jumpOrder = arg;
        break;
    case 12:                            // set volume
        if (m_tick)
            break;
        c->m_volume = arg <= 64 ? arg : 64;
        break;
    case 13:                            // pattern break, the row in BCD
        m_breakRow = (arg >> 4) * 10 + (arg & 0xf);
        break;
    case 14:
        ExtFx(c, arg);
        break;
    case 15:                            // speed below 32, else BPM
        if (m_tick)
            break;
        if (arg < 32)
        {
            m_speed = arg;
            m_tick = 0;
        }
        else
        {
            m_tempo = arg;
            m_tick = 0;
        }
        break;
    }
}

// Effects E0..EF.  Fine portamento runs the other way from XM's: E1 adds.
void XmPlayer::ExtFx(XmChannel *c, u8 param)
{
    u8 lo;

    lo = param & 0xf;
    switch (param >> 4)
    {
    case 0:
        break;
    case 1:                             // fine portamento
        if (m_tick == 0)
        {
            if (lo)
                c->m_finePorta = lo;
            c->m_period = c->m_period + (c->m_finePorta << 2);
        }
        break;
    case 2:
        if (m_tick == 0)
        {
            if (lo)
                c->m_finePorta = lo;
            c->m_period = c->m_period - (c->m_finePorta << 2);
        }
        break;
    case 3:                             // glissando
        c->m_glissando = lo;
        break;
    case 4:                             // vibrato waveform
        c->m_waves &= 0xf0;
        c->m_waves |= lo;
        break;
    case 5:                             // finetune
        break;
    case 6:                             // pattern loop
        break;
    case 7:                             // tremolo waveform
        c->m_waves &= 0xf;
        c->m_waves |= lo << 4;
        break;
    case 8:
        break;
    case 9:                             // retrigger
        break;
    case 10:                            // fine volume slide
        if (m_tick == 0)
        {
            if (lo)
                c->m_volColSlide = lo;
            c->m_volume = c->m_volume + c->m_volColSlide;
            c->m_volume = XMMIN(c->m_volume, 64);
        }
        break;
    case 11:
        if (m_tick == 0)
        {
            if (lo)
                c->m_volColSlide = lo;
            c->m_volume = c->m_volume - c->m_volColSlide;
            c->m_volume = XMMAX(c->m_volume, 0);
        }
        break;
    case 12:                            // note cut
        if (m_tick >= lo)
            c->m_volume = 0;
        break;
    case 13:                            // note delay
        break;
    case 14:                            // pattern delay
        break;
    case 15:
        break;
    }
}

// The tick's result to the voices: volume through the envelope, fadeout,
// global and the EE's volume; pan through the EE's pan and the pan envelope.
// A fresh key-on retriggers the sample, anything else only updates it.
void XmPlayer::Update()
{
    int i;
    XmChannel *c;
    int l;
    int r;
    int vol;
    int pan;

    for (i = 0; i < m_module->m_channels; i++)
    {
        c = &m_ch[i];
        c->Envelopes();
        if (!c->m_keyOn && c->m_volEnv)
        {
            c->m_fadeVol = c->m_fadeVol - c->m_fadeout;
            if (c->m_fadeVol < 0)
                c->m_fadeVol = 0;
        }
        pan = c->m_pan;
        pan = XMMIN(XMMAX(pan, 0), 255);
        pan = pan + (m_pan - 128) * (128 - abs(pan - 128)) / 128;
        pan = pan + (c->m_envPan - 128) * (128 - abs(pan - 128)) / 128;
        vol = c->m_outVolume * 255 / 64;
        vol = c->m_envVol * vol / 64;
        vol = vol * c->m_fadeVol / 0x7fff;
        l = (255 - pan) * vol >> 8;
        r = vol * pan >> 8;
        l = l * m_globalVol >> 7;
        r = r * m_globalVol >> 7;
        l = l * m_volume >> 8;
        r = r * m_volume >> 8;
        l = l >> 2;
        r = r >> 2;
        if (c->m_keyOn && !c->m_keyed && !c->m_porta)
        {
            if (c->m_sample && c->m_sample->m_length > 0)
            {
                if (c->m_offset >= c->m_sample->m_length)
                    c->m_offset = c->m_sample->m_length - 1;
                m_voices->KeyOn(c - m_ch, c->m_sample->m_spuAddr, c->m_sample->m_length,
                                c->m_sample->m_flags & 3, c->m_sample->m_loopStart,
                                Rate(c->m_outPeriod) * m_pitch >> 12, l, r);
                m_voices->SetOffset(c - m_ch, c->m_offset);
            }
        }
        else
        {
            m_voices->SetVolume(c - m_ch, l, r);
            m_voices->SetPitch(c - m_ch, Rate(c->m_outPeriod) * m_pitch >> 12);
        }
        c->m_keyed = c->m_keyOn;
        c->m_offset = 0;
        c->m_porta = 0;
    }
}

static short XmInterp(short x, short x0, short x1, short y0, short y1);

// Amiga periods: the table's octave 0, finetune interpolated between two of its
// columns, shifted down by the octave.  A negative result (finetune off the
// table's top) is tried again with finetune 0.
int XmPlayer::AmigaPeriod(u8 note, short finetune)
{
    u8 n;
    u8 oct;
    int lo;
    int hi;
    int idx;
    int period;

again:
    n = note % 12;
    oct = note / 12;
    idx = (short)(finetune >> 4);
    idx = idx + (n << 3);
    lo = gXmPeriodTable[idx];
    hi = gXmPeriodTable[idx + 1];
    period = XmInterp(finetune / 16, 0, 15, lo, hi) >> oct;
    if (period < 0 && finetune)
    {
        finetune = 0;
        goto again;
    }
    return period;
}

// Linear periods: 10 octaves of 768.
int XmPlayer::LinearPeriod(u8 note, short finetune)
{
    return 7680 - note * 64 - (short)(finetune / 2);
}

int XmPlayer::Period(u8 note, short finetune)
{
    if ((u8)(m_module->m_flags % 2))
        return LinearPeriod(note, finetune);
    return AmigaPeriod(note, finetune);
}

// A period as an SPU rate, 4096 = 48 kHz before the player's pitch.
int XmPlayer::Rate(int period)
{
    int rate;

    if ((u8)(m_module->m_flags % 2))
    {
        rate = gXmLinearTable[period % 768] >> (period / 768);
    }
    else
    {
        if (period == 0)
            rate = 0;
        else
            rate = 0xa3ba68 / period >> 3;
        rate = rate * 48000 / 4096;
    }
    return rate;
}

void XmPlayer::KeyOff(XmChannel *c)
{
    c->SetKey(0);
    if (c->m_volEnv == 0)
        c->m_volume = 0;
}

void XmChannel::Reset()
{
    m_volEnvPos = 0;
    m_volEnvA = 0;
    m_volEnvB = 1;
    m_panEnvPos = 0;
    m_panEnvA = 0;
    m_panEnvB = 1;
}

void XmChannel::Envelopes()
{
    if (m_volEnv)
        m_envVol = EnvStep(m_volEnv, &m_volEnvPos, &m_volEnvA, &m_volEnvB);
    else
        m_envVol = 64;
    if (m_panEnv)
        m_envPan = EnvStep(m_panEnv, &m_panEnvPos, &m_panEnvA, &m_panEnvB) << 2;
    else
        m_envPan = 128;
}

// One tick of an envelope: the value between its two current points, then the
// position moves on unless a held key is sitting on the sustain point.
short XmChannel::EnvStep(XmEnvelope *e, u16 *ppos, u8 *pa, u8 *pb)
{
    u8 a;
    u8 b;
    u16 pos;
    short v;

    a = *pa;
    b = *pb;
    pos = *ppos;
    a = XMMIN(a, e->m_points - 1);
    b = XMMIN(b, e->m_points - 1);
    v = XmInterp(pos, e->m_pt[a].m_x, e->m_pt[b].m_x, e->m_pt[a].m_y, e->m_pt[b].m_y);
    if ((e->m_flags & 2) && m_keyOn && a == e->m_sustain && pos == e->m_pt[a].m_x)
        ;
    else
    {
        pos++;
        if (pos >= e->m_pt[b].m_x)
        {
            a = b;
            b = b + 1;
            if (e->m_flags & 4)
            {
                if (b > e->m_loopEnd)
                {
                    a = e->m_loopStart;
                    b = a + 1;
                    pos = e->m_pt[a].m_x;
                }
            }
            else if (b >= e->m_points)
            {
                b = b - 1;
                pos = pos - 1;
            }
        }
    }
    *pa = a;
    *pb = b;
    *ppos = pos;
    return v;
}

void XmChannel::Porta()
{
    int d;

    d = m_outPeriod - m_target;
    if (d == 0 || m_portaSpeed > abs(d))
        m_outPeriod = m_target;
    else if (d > 0)
        m_outPeriod = m_outPeriod - m_portaSpeed;
    else
        m_outPeriod = m_outPeriod + m_portaSpeed;
    m_period = m_outPeriod;
}

void XmChannel::Vibrato(int tick)
{
    u8 pos;
    u16 v;

    v = 0;
    pos = ((u8)m_vibPos >> 2) & 0x1f;
    switch (m_waves & 3)
    {
    case 0:
        v = gXmSine[pos];
        break;
    case 1:
        pos <<= 3;
        if (m_vibPos < 0)
            pos = 255 - pos;
        v = pos;
        break;
    case 2:
        v = 255;
        break;
    }
    v *= m_vibDepth;
    v >>= 7;
    v <<= 2;
    if (m_vibPos >= 0)
        m_outPeriod = m_period + v;
    else
        m_outPeriod = m_period - v;
    if (tick)
        m_vibPos = m_vibPos + m_vibSpeed;
}

void XmChannel::Tremolo(int tick)
{
    u8 pos;
    u16 v;

    v = 0;
    pos = ((u8)m_tremPos >> 2) & 0x1f;
    switch ((m_waves >> 4) & 3)
    {
    case 0:
        v = gXmSine[pos];
        break;
    case 1:
        pos <<= 3;
        if (m_tremPos < 0)
            pos = 255 - pos;
        v = pos;
        break;
    case 2:
        v = 255;
        break;
    }
    v *= m_tremDepth;
    v >>= 6;
    if (m_tremPos >= 0)
    {
        m_outVolume = m_volume + v;
        if (m_outVolume > 64)
            m_outVolume = 64;
    }
    else
    {
        m_outVolume = m_volume - v;
        if (m_outVolume < 0)
            m_outVolume = 0;
    }
    if (tick)
        m_tremPos = m_tremPos + m_tremSpeed;
}

void XmChannel::VolSlide(int tick)
{
    if (tick)
    {
        m_volume = m_volume + (m_volSlide >> 4);
        if (m_volume > 64)
            m_volume = 64;
        m_volume = m_volume - (m_volSlide & 0xf);
        if (m_volume < 0)
            m_volume = 0;
    }
}

void XmChannel::PanSlide(int tick, u8 param)
{
    u8 lo;
    u8 hi;

    if (param)
        m_panSlide = param;
    if (tick)
    {
        lo = m_panSlide & 0xf;
        hi = m_panSlide >> 4;
        if (hi)
            lo = 0;
        m_pan = m_pan - lo;
        m_pan = m_pan + hi;
        m_pan = XMMIN(XMMAX(0, m_pan), 255);
    }
}

// Linear interpolation of y between (x0, y0) and (x1, y1).
static short XmInterp(short x, short x0, short x1, short y0, short y1)
{
    short dx;
    short dy;
    short dp;

    if (x0 == x1)
        return y0;
    dy = y1 - y0;
    dx = x1 - x0;
    dp = x - x0;
    return y0 + dp * dy / dx;
}
