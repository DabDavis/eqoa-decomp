// VIMovie -- SUPPORT 0x004C1840 .. 0x004C1AD8.  The engine's front end to the
// SCE PSS player (sce_playpss.cpp / sce_strfile.cpp / sce_audiodec.cpp):
// allocate every buffer the player needs once, then play a movie off the disc
// by name, with a callback the player polls to abort.
#include "vi.h"

extern "C" void *memset(void *, int, unsigned int);
extern "C" int   sprintf(char *, const char *, ...);
extern "C" void *memalign(int, int);
extern "C" void  free(void *);
extern "C" void *sceSifAllocIopHeap(int size);
extern "C" int   sceSifFreeIopHeap(void *p);
extern "C" int   sceMpegInit();
extern "C" int   strFileOpen(char *filename);
extern "C" int   strFileClose();
extern "C" int   strFileRead(unsigned char *buff, int size);

// What playPss is handed: the MPEG work area, the 640x448 RGB32 frame, two
// sets of upload tags, the video ring, the EE and IOP audio rings, and the
// zero block audiodec silences the SPU with.  48 bytes, cleared first.
struct VIMovieBuffers
{
    void *m_mpegWork;      // +0x00
    int   m_mpegWorkSize;  // +0x04
    void *m_image;         // +0x08
    void *m_tags;          // +0x0C
    void *m_ring;          // +0x10
    int   m_ringSize;      // +0x14
    void *m_audio;         // +0x18
    int   m_audioSize;     // +0x1C
    void *m_iopAudio;      // +0x20
    int   m_iopAudioSize;  // +0x24
    void *m_zero;          // +0x28
    void *m_iopZero;       // +0x2C
};

extern "C" void playPss(int (*read)(unsigned char *, int), VIMovieBuffers *p, int (*abort)());

static VIMovieBuffers sMovie;

void VIMovieShutdown();

int VIMovieStartup(int, int)
{
    memset(&sMovie, 0, sizeof(sMovie));
    sMovie.m_mpegWork = memalign(64, 0x13C768);
    sMovie.m_mpegWorkSize = 0x13C768;
    sMovie.m_image = memalign(64, 0x118000);
    sMovie.m_tags = memalign(64, 0x34A00);
    sMovie.m_ring = memalign(64, 0x30000);
    sMovie.m_ringSize = 0x30000;
    sMovie.m_audio = memalign(64, 0xC000);
    sMovie.m_audioSize = 0xC000;
    sMovie.m_iopAudio = sceSifAllocIopHeap(0x6000);
    sMovie.m_iopAudioSize = 0x6000;
    sMovie.m_zero = memalign(64, 2048);
    sMovie.m_iopZero = sceSifAllocIopHeap(2048);

    if (!sMovie.m_mpegWork || !sMovie.m_image || !sMovie.m_tags || !sMovie.m_ring
        || !sMovie.m_audio || !sMovie.m_iopAudio || !sMovie.m_zero || !sMovie.m_iopZero)
    {
        VIMovieShutdown();
        return -1;
    }
    sceMpegInit();
    return 0;
}

void VIMovieShutdown()
{
    if (sMovie.m_mpegWork)
        free(sMovie.m_mpegWork);
    if (sMovie.m_image)
        free(sMovie.m_image);
    if (sMovie.m_tags)
        free(sMovie.m_tags);
    if (sMovie.m_ring)
        free(sMovie.m_ring);
    if (sMovie.m_audio)
        free(sMovie.m_audio);
    if (sMovie.m_iopAudio)
        sceSifFreeIopHeap(sMovie.m_iopAudio);
    if (sMovie.m_zero)
        free(sMovie.m_zero);
    if (sMovie.m_iopZero)
        sceSifFreeIopHeap(sMovie.m_iopZero);
}

int VIMoviePlay(const char *name, int (*abort)())
{
    char path[256];
    sprintf(path, "cdrom0:\\%s;1", name);
    if (!strFileOpen(path))
        return -1;
    playPss(strFileRead, &sMovie, abort);
    strFileClose();
    return 0;
}
