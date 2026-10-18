// The XM player: SNDDRV 0x402C .. 0x8C80 (xm.cpp), driven by the commands in
// the driver TU and stepped by its timer thread.
//
// The driver TU (the one that defines gXmSlot[8]) also emits, at its end, the
// out-of-line copies of everything here that is defined in-class: XmSlot's
// synthesized constructor and destructor (0x3C3C, 0x3C8C), the static
// initialiser that runs them over the array (0x3D00), XmPlayer's accessors
// (0x3E3C .. 0x3F74) and the two _GLOBAL_ functions (0x3FBC, 0x3FF4).  In g++
// 2.8 at -O0 an inline member is never inlined; each TU that uses one emits its
// own copy after its own functions, and `this` (and the parameters of an inline
// member) stay in registers rather than going to the stack.
#ifndef XM_H
#define XM_H
#include "snddrv.h"

// One SPU voice as the player drives it: 56 bytes.
struct XmVoice
{
    int m_voice;            // +0x00  0..47: core = voice >= 24
    int m_04;
    int m_08;
    int m_0c;
    int m_10;
    int m_14;
    int m_18;
    int m_loopStart;        // +0x1C  the sample's, from KeyOn (unused after)
    int m_20;
    int m_24;
    u32 m_loopType;         // +0x28  the sample's flags & 3; SetOffset uses it as the
                            //        loop OFFSET, a shipped bug (loops start at 0..3)
    int m_2c;
    int m_30;
    u32 m_addr;             // +0x34  the sample's SPU address, 0 = silent
};

// The voices a player owns -- one per channel of its slot's mask -- and the
// key-on/key-off masks it collects during a tick and sends at the end of it.
// 0x718 bytes, from `new`, with no constructor.
class XmVoices
{
public:
    u32     m_keyOn[2];     // +0x00  core 0, core 1
    u32     m_keyOff[2];    // +0x08
    int     m_10;
    u32     m_sampleBase;   // +0x14  the module's samples in SPU RAM
    XmVoice m_ch[32];       // +0x18

    int       Init(u32 lo, u32 sampleBase, u32 hi);
    void      Reset();
    int       Wait();
    int       Busy();
    int       Wait2();
    int       Elapsed();
    int       Flush(int ticks);
    int       KeyOn(int ch, u32 addr, int length, u32 loopType, int loopStart, u32 rate,
                    int volL, int volR);
    int       KeyOff(int ch);
    int       SetPitch(int ch, u32 rate);
    int       Nop(int ch, int x);
    int       SetVolume(int ch, int volL, int volR);
    int       SetOffset(int ch, u32 samples);
};

// The module as XmModule::Relocate leaves it in IOP memory.
struct XmPattern                // 12 bytes
{
    int   m_00;
    short m_rows;               // +0x04
    short m_size;               // +0x06  packed bytes, 0 = empty
    u8   *m_data;               // +0x08  made absolute by Relocate
};

struct XmSample                 // 24 bytes
{
    int m_length;               // +0x00  in blocks of 28 samples, then bytes
    int m_loopStart;            // +0x04  -1 = none
    int m_loopEnd;              // +0x08
    int m_flags;                // +0x0C  & 3 = loop type
    u8  m_volume;               // +0x10  0..64
    u8  m_pan;                  // +0x11
    s8  m_finetune;             // +0x12
    u8  m_relNote;              // +0x13
    u32 m_spuAddr;              // +0x14  relative, then absolute
};

struct XmEnvPoint
{
    short m_x;                  // ticks
    short m_y;                  // 0..64
};

struct XmEnvelope               // 56 bytes
{
    u8         m_points;
    u8         m_sustain;       // +0x01  the point a held key stops at
    u8         m_loopStart;     // +0x02
    u8         m_loopEnd;       // +0x03
    u32        m_flags;         // +0x04  1 on, 2 sustain, 4 loop
    XmEnvPoint m_pt[12];        // +0x08
};

struct XmInstrument             // 224 bytes
{
    u8         m_samples;       // +0x00  how many
    u8         m_01[3];
    u8         m_map[96];       // +0x04  note -> sample
    XmEnvelope m_volEnv;        // +0x64
    XmEnvelope m_panEnv;        // +0x9C
    short      m_d4[2];
    short      m_fadeout;       // +0xD8
    short      m_firstSample;   // +0xDA  -1 = none
    XmSample *m_sample;         // +0xDC  made absolute by Relocate
};

class XmModule
{
public:
    int          m_00;
    short        m_length;      // +0x04  orders in the song
    short        m_06;
    short        m_channels;    // +0x08
    short        m_patterns;    // +0x0A
    short        m_instruments; // +0x0C
    u16          m_flags;       // +0x0E  1 = linear frequencies, tested as (u8)(f % 2)
    short        m_speed;       // +0x10
    short        m_tempo;       // +0x12
    int          m_14;
    int          m_18;          // +0x18  zeroed by the constructor, else unused
    u8           m_order[256];  // +0x1C
    XmPattern    m_pattern[256];        // +0x11C
    XmInstrument m_instrument[128];     // +0xD1C
    XmSample     m_sample[128];         // +0x7D1C
                                        // +0x891C  the packed rows

    XmModule();
    ~XmModule();
    int  Relocate(u32 spuAddr, int flags);
    void Nop();
    int  Channels();
};

// One tracker channel: 0x68 bytes.
struct XmChannel
{
    u8            m_note;       // +0x00  the row's five fields
    u8            m_ins;
    u8            m_vol;
    u8            m_fx;
    u8            m_param;
    u8            m_key;        // +0x05  note - 1
    short         m_outVolume;  // +0x06  this tick's, after tremolo
    short         m_volume;     // +0x08  0..64
    short         m_pan;        // +0x0A  0..255
    XmInstrument *m_inst;       // +0x0C
    XmEnvelope   *m_volEnv;     // +0x10  0 when the instrument has none
    XmEnvelope   *m_panEnv;     // +0x14
    XmSample     *m_sample;     // +0x18
    u8            m_relNote;    // +0x1C
    u8            m_1d;
    short         m_finetune;   // +0x1E
    int           m_outPeriod;  // +0x20  this tick's, after vibrato/arpeggio
    int           m_period;     // +0x24
    int           m_target;     // +0x28  the note's own period: portamento's goal
    int           m_fadeVol;    // +0x2C  32767 at key-on, falls by m_fadeout
    int           m_fadeout;    // +0x30
    char          m_porta;      // +0x34  a tone portamento this row: no retrigger
    char          m_keyOn;      // +0x35
    char          m_keyed;      // +0x36  the key-on has gone to the voice
    u8            m_37;
    short         m_envVol;     // +0x38  0..64
    u16           m_volEnvPos;  // +0x3A
    u8            m_volEnvA;    // +0x3C  the segment's two points
    u8            m_volEnvB;    // +0x3D
    short         m_envPan;     // +0x3E  0..255
    u16           m_panEnvPos;  // +0x40
    u8            m_panEnvA;    // +0x42
    u8            m_panEnvB;    // +0x43
    u16           m_slide;      // +0x44  portamento up/down, x4
    u16           m_portaSpeed; // +0x46  x4
    u16           m_vibDepth;   // +0x48
    u16           m_vibSpeed;   // +0x4A
    s8            m_vibPos;     // +0x4C
    u8            m_4d;
    u16           m_tremDepth;  // +0x4E
    u16           m_tremSpeed;  // +0x50
    s8            m_tremPos;    // +0x52
    u8            m_53;
    int           m_offset;     // +0x54  sample offset for the next key-on
    int           m_lastOffset; // +0x58
    u16           m_waves;      // +0x5C  vibrato waveform low nibble, tremolo high
    u8            m_volSlide;   // +0x5E  A/5/6's parameter
    u8            m_panSlide;   // +0x5F  P's parameter
    u8            m_volColSlide;// +0x60
    u8            m_panColSlide;// +0x61
    u8            m_finePorta;  // +0x62
    u8            m_glissando;  // +0x63
    u8            m_periodSet;  // +0x64  an effect set m_outPeriod this tick
    u8            m_volumeSet;  // +0x65  an effect set m_outVolume this tick
    u8            m_66[2];

    void  SetKey(int on)
    {
        if (on)
        {
            m_keyOn = 1;
            m_keyed = 0;
        }
        else
        {
            m_keyOn = 0;
            m_keyed = 1;
        }
    }
    void  Reset();
    void  Envelopes();
    short EnvStep(XmEnvelope *e, u16 *pos, u8 *a, u8 *b);
    void  Porta();
    void  Vibrato(int tick);
    void  Tremolo(int tick);
    void  VolSlide(int tick);
    void  PanSlide(int tick, u8 param);
};

// One player's whole state, embedded in its slot at +0x0C.  0xD50 bytes.
class XmPlayer
{
public:
    int       m_done;       // +0x00  1 once the song has ended (no repeat)
    u32       m_lo;         // +0x04  the slot's core 0 voice mask
    XmModule *m_module;     // +0x08  the module in IOP memory, relocated
    XmVoices *m_voices;     // +0x0C
    int       m_tick;       // +0x10  tick within the row
    int       m_row;        // +0x14
    int       m_pattern;    // +0x18
    int       m_order;      // +0x1C  position in the order table
    u8       *m_rowData;    // +0x20  the pattern's packed rows
    int       m_rowPos;     // +0x24  read offset into them
    int       m_rows;       // +0x28  rows in the pattern
    int       m_globalVol;  // +0x2C  0..128
    int       m_breakRow;   // +0x30  -1, or the row a pattern break goes to
    int       m_jumpOrder;  // +0x34  -1, or the order a position jump goes to
    int       m_seek;       // +0x38  rows must be skipped to reach m_row
    short     m_speed;      // +0x3C  ticks per row
    short     m_tempo;      // +0x3E  BPM
    int       m_repeat;     // +0x40
    int       m_volume;     // +0x44  0..255, from the EE
    int       m_pan;        // +0x48  0..255, 128 centre
    int       m_pitch;      // +0x4C  4096 = 1.0
    XmChannel m_ch[32];     // +0x50

    XmPlayer();
    ~XmPlayer();

    void SetRepeat(int r)   { m_repeat = r; }
    int  Tempo()            { return m_tempo; }
    void SetVolume(int v)   { m_volume = v; }
    void SetPan(int p)      { m_pan = p; }
    void SetPitch(int p)    { m_pitch = p; }
    int  Pitch()            { return m_pitch; }

    int  Play(u32 lo, XmModule *module, u32 hi);
    void Stop();
    void Restart();
    int  Process();
    void Tick();
    void SkipRows(int rows);
    void ReadRow(int ch);
    void TickEffects();
    void Effects(XmChannel *c);
    void Fx(XmChannel *chan, u8 fx, u8 param);
    void ExtFx(XmChannel *c, u8 param);
    void Update();
    int  AmigaPeriod(u8 note, short finetune);
    int  LinearPeriod(u8 note, short finetune);
    int  Period(u8 note, short finetune);
    int  Rate(int period);
    void KeyOff(XmChannel *c);
};

// gXmSlot[8] at .data 0x9634, stride 0xD60.  The status reply copies each
// slot's channel mask into words 8.. as {lo, hi} pairs (visounddevice.cpp's
// gSndXm), and the timer thread steps every active slot.
struct XmSlot
{
    u32      m_lo;          // +0x000  voices 0..23 (core 0)
    u32      m_hi;          // +0x004  voices 24..47 (core 1)
    int      m_active;      // +0x008  the module's IOP address while playing
    XmPlayer m_player;      // +0x00C
    int      m_tick;        // +0xD5C  += tempo * pitch >> 12, a tick per 250
};


#endif
