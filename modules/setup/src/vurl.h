// vurl.h -- the HTTP/1.x client over libinet, and what it calls.  One source
// file, linked into two modules: SETUP (0x005DC568, behind libxml) and STATION
// (0x002DC7F0, among its own TUs), byte-identical in both.  STATION's
// src/vurl.cpp and src/vurl.h are symlinks to SETUP's.
#ifndef VURL_H
#define VURL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

extern "C" int RotateThreadReadyQueue(int priority);

// libnet/libinet, as VUrl calls them.  The client record is opaque here.
struct sceSifMClientData
{
    char m_data[60];
};
struct sceInetAddress
{
    int  reserved;
    char data[12];
};
struct sceInetParam
{
    int type;                    // 1 = connect
    int local_port;              // -1 = any
    sceInetAddress remote_addr;  // +0x08
    int remote_port;             // +0x18
    int reserved[9];
};
extern "C" int sceLibnetInitialize(sceSifMClientData *cd, unsigned int bufsize,
                                   unsigned int stacksize, int priority);
extern "C" int sceLibnetTerminate(sceSifMClientData *cd);
extern "C" int sceInetName2Address(sceSifMClientData *cd, unsigned int *buf, int flags,
                                   sceInetAddress *addr, const char *name, int ms, int retry);
extern "C" int sceInetCreate(sceSifMClientData *cd, unsigned int *buf, sceInetParam *param);
extern "C" int sceInetOpen(sceSifMClientData *cd, unsigned int *buf, int cid, int ms);
extern "C" int sceInetClose(sceSifMClientData *cd, unsigned int *buf, int cid, int ms);
extern "C" int sceInetSend(sceSifMClientData *cd, unsigned int *buf, int cid, const void *ptr,
                           int count, int *flags, int ms);
extern "C" int sceInetRecv(sceSifMClientData *cd, unsigned int *buf, int cid, void *ptr,
                           int count, int *flags, int ms);
extern "C" int sceMcInit(void);
extern "C" int sceMcGetInfo(int port, int slot, int *type, int *free, int *format);
extern "C" int sceMcFormat(int port, int slot);

// The download thread's listener.  No virtual destructor: OnBytesDownloaded is
// slot 1 and PatcherAPI_Impl's own destructor comes after it.
class VIUrlListener
{
public:
    virtual void OnBytesDownloaded(const char *url, const char *file, int got, int total) = 0;
};

// The HTTP client (0x005DC568), shared with STATION.  1,128 bytes;
// the destructor is not virtual.
class VUrl
{
public:
    VUrl(const char *url);
    VUrl(int protocol, const char *server, const char *path);
    ~VUrl();
    void Initialize();
    void SetUrlListener(VIUrlListener *l);
    void SetPort(int port);
    int  GetError();
    char *GetErrorString();
    void SetThreadPriority(int p);
    void SetProtocol(int p);
    void SetServer(const char *server);
    void SetRelativePath(const char *path);
    void ParseURL(const char *url);
    int  OpenSocket();
    int  Get(char *buf, int size);
    int  GetPoll(char *buf, int size);
    int  Mkdirhier(const char *path);
    int  OpenMemCardFile(const char *path);
    int  CloseMemCardFile();
    int  InitMemCard();
    int  WriteToMemCard(char *buf, int size);
    int  GetToMemoryCard(char *path);
    void PrintError();
    long GetBytes();

    int   m_protocol;            // +0x000 0 http, -1 none
    char *m_server;              // +0x004
    char *m_path;                // +0x008
    int   m_port;                // +0x00C 80
    int   m_cid;                 // +0x010 the libinet connection
    int   m_error;               // +0x014
    char  m_errorText[1024];     // +0x018
    sceSifMClientData m_client;  // +0x418
    unsigned int *m_netBuf;      // +0x454 0x10200 bytes, 64-aligned
    int   m_priority;            // +0x458
    int   m_bytes;               // +0x45C after the last Get
    int   m_fd;                  // +0x460 the memory card file
    VIUrlListener *m_listener;   // +0x464
};

// The memory card calls VUrl and the patcher's VMemCardFile make.
extern "C" int sceMcChdir(int port, int slot, const char *newDir, char *curDir);
extern "C" int sceMcMkdir(int port, int slot, const char *name);
extern "C" int sceMcOpen(int port, int slot, const char *name, int mode);
extern "C" int sceMcClose(int fd);
extern "C" int sceMcRead(int fd, void *buf, int size);
extern "C" int sceMcWrite(int fd, const void *buf, int size);
extern "C" int sceMcSeek(int fd, int offset, int mode);
extern "C" int sceMcSync(int mode, int *cmd, int *result);

#endif
