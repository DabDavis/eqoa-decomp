// patcher.h -- Verant's patcher, as SETUP links it (0x0057CDD8 .. 0x00589F50,
// plus VUrl at 0x005DC568).  The class and method names are the 2002 betas'
// own exports; the layouts are what the written functions have measured.
// The C++ is SETUP's alone: STATION shares libxml and VUrl (vurl.h), not the
// patcher.
#ifndef PATCHER_H
#define PATCHER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <malloc.h>
#include <errno.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "zlib.h"
#include "vurl.h"

extern "C" int SleepThread(void);
extern "C" int RotateThreadReadyQueue(int priority);

// PS2 stand-ins for the Win32 calls the PC patcher made.
char *_strlwr(char *s);
int   Sleep(int ms);
const char *GetCommandLine();
extern "C" int link(const char *from, const char *to);
int   rmdir(char *path);
int   chdir(const char *path);
int   mkdir(char *path);
int   mkdir(char *path, int mode);
char *getcwd(char *buf, int size);

// A file the patcher keeps in memory, for the game to put on the card.
struct VIPatchImage
{
    VIPatchImage()
    {
        m_size = 0;
        m_data = 0;
        m_dirty = 0;
        m_deleted = 0;
        m_next = 0;
        m_crc = 0;
        m_name[0] = 0;
    }

    // VISetup::DeletePatchImage's `delete` runs this inline (its body sits
    // inside the delete's null check).
    ~VIPatchImage()
    {
        if (m_data)
            free(m_data);
        m_data = 0;
    }

    char  m_name[64];            // +0x00
    int   m_size;                // +0x40
    char *m_data;                // +0x44
    int   m_dirty;               // +0x48
    int   m_deleted;             // +0x4C
    unsigned int m_crc;          // +0x50
    VIPatchImage *m_next;        // +0x54
};
class PatcherAPI;
class PatcherAPI_Impl;
class PatcherChanges;
class PatcherDistributionInfo;

// What Patcher_thread is handed: one operation and its arguments.  284 bytes,
// allocated once by PatcherAPI_Impl and reused.
struct PatcherThreadData
{
    PatcherAPI      *m_api;      // +0x000
    PatcherAPI_Impl *m_impl;     // +0x004
    int   m_op;                  // +0x008 0 initialize, 1 download, 2 use CRC,
                                 //        3 rescan, 4 initial scan
    char  m_name[256];           // +0x00C the distributions, comma-separated
    _xmlNode *m_node;            // +0x10C
    PatcherDistributionInfo *m_dist;   // +0x110
    bool  m_useCRC;              // +0x114
    int   m_flags;               // +0x118
};

unsigned long Patcher_thread(void *data);

// The patcher proper.  1,888 bytes; VIUrlListener's vptr at +0.  No thread on
// the PS2: every operation runs Patcher_thread to completion.
class PatcherAPI_Impl : public VIUrlListener
{
public:
    PatcherAPI_Impl();
    virtual ~PatcherAPI_Impl();
    virtual int Initialize(const char *server, int port, const char *file, PatcherAPI *api, int flags);
    virtual int Initialize(const char *server, int port, const char *file, PatcherAPI *api,
                           const char *user, const char *password, int flags);
    virtual int InitializeFromLocalFile(const char *file, PatcherAPI *api, int flags);
    virtual void OnBytesDownloaded(const char *url, const char *file, int got, int total);

    char *DownloadMemoryFile(int size, char *server, int port, char *path, int compressed);
    int  ContinueInitialization(int flags);
    void StartInitializeThread(_xmlNode *node);
    void SetInitializing(bool on);
    void CommandLineOperations();
    void PatchPatcher(const char *a, const char *b);
    void SetRestartName(const char *name, const char *arg);
    void AddCommandLineArg(const char *arg);
    int  InitializeChanges(_xmlNode *node);
    int  SetChangesFile(_xmlNode *node);
    int  FindRoots(const char *root);
    int  SetRoot();
    static int GenerateLocalSubPath(const char *path, char *done, const char *cwd);
    int  SetDistributionRoot(const char *names, const char *root);
    int  RescanFiles(const char *names, int flags);
    bool RescanRoutine(const char *names, int flags);
    bool UseCRCRoutine(bool on);
    int  SetLocal();
    const char *GetRoot(const char *name);
    char *GetLocal();
    int  UseCRC(bool on, int flags);
    int  InitialScan(const char *names);
    int  UsingCRC();
    int  NeedDownload(int flags);
    int  DeleteOldFiles();
    int  Download(const char *names, int flags);
    int  Uninstall(const char *names);
    int  GetFileCount(const char *names, int flags);
    bool InitialScanRoutine(const char *names);
    bool DownloadRoutine(const char *names, int flags);
    int  ThreadBusy();
    int  Select(const char *names, bool on);
    int  Cancel();
    int  Continue();
    char *GetProductName();
    short GetVersion();
    PatcherChanges *GetProductChanges();
    PatcherDistributionInfo *GetDistributionList();
    long GetEstimate();
    long GetEstimateRemaining();
    long GetTotalSize(const char *names, int flags);
    long GetDownloadSize(const char *names, int flags);
    int  DownloadToMemory(char *buf, int size, const char *server, int port, char *path, int compressed);
    int  DownloadFile(char *server, int port, char *path, char *file, int compressed, long a, long b);
    int  GetFile(const char *dist, const char *file, int x);
    void Restart(bool now);
    void SetThreadInUse(bool on, int op);
    int  NeedsExit();
    int  SetCheckOnInitialize(bool on);
    int  CheckOnInitialize();
    int  SetQuickScan(bool on);
    int  QuickScan();
    int  IncrementScanCount();
    void SetPatchImage(VIPatchImage *images);
    VIPatchImage *FindPatchImage(const char *name);

    PatcherAPI *m_api;           // +0x004
    short m_version;             // +0x008
    char *m_productName;         // +0x00C
    char *m_server;              // +0x010
    int   m_port;                // +0x014
    char *m_file;                // +0x018
    char *m_user;                // +0x01C
    char *m_password;            // +0x020
    char  m_root[255];           // +0x024
    char  m_cwd[256];            // +0x123
    long  m_estimates[50];       // +0x228 the last 50 finish times
    int   m_estimateIndex;       // +0x3B8
    int   m_estimatesFull;       // +0x3BC
    xmlDocPtr m_doc;             // +0x3C0
    PatcherChanges *m_changes;   // +0x3C4
    PatcherDistributionInfo *m_distList;   // +0x3C8
    PatcherThreadData *m_threadData;       // +0x3CC
    int   m_downloading;         // +0x3D0
    int   m_initializing;        // +0x3D4
    long  m_x3D8;                // +0x3D8
    int   m_x3E0;                // +0x3E0
    int   m_threadInUse;         // +0x3E4
    char  m_commandLine[256];    // +0x3E8
    char  m_restartName[256];    // +0x4E8
    int   m_patchingPatcher;     // +0x5E8
    char  m_x5EC[260];           // +0x5EC handed to each Http_get
    long  m_startTime;           // +0x6F0
    int   m_cancel;              // +0x6F8
    unsigned int m_bytesDone;    // +0x6FC
    unsigned int m_bytesTotal;   // +0x700
    unsigned long m_bytesDownloaded;   // +0x708 summed over the files so far
    unsigned long m_timeDownloaded;    // +0x710
    unsigned int m_unzipDone;    // +0x718
    unsigned int m_unzipTotal;   // +0x71C
    unsigned long m_bytesUnzipped;     // +0x720
    unsigned long m_timeUnzipped;      // +0x728
    long  m_now;                 // +0x730
    long  m_lastEstimate;        // +0x738
    int   m_restartNameSet;      // +0x740
    int   m_useCRC;              // +0x744
    int   m_needsExit;           // +0x748
    int   m_checkOnInitialize;   // +0x74C
    int   m_quickScan;           // +0x750
    int   m_scanCount;           // +0x754
    int   m_scanTotal;           // +0x758
    VIPatchImage *m_images;      // +0x75C
};

// One HTTP download into memory, optionally gzip, and from there into the
// patch image list.  5,048 bytes, the vptr last.
class Http_get
{
public:
    Http_get(VIPatchImage *images);
    virtual ~Http_get();
    int   Initialize(char *x);
    int   Get(char *url, char *file, char *c, char *d, int compressed);
    int   WriteFile(char *file);
    void  FreeMem();
    int   MemoryGet(char *buf, int size, char *url, char *c, char *d, int compressed);
    int   http_get_byte(z_stream *s);
    void  http_check_header(z_stream *s);
    int   http_uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
    long  Get_thread();
    void  SetError(const char *msg, int type);
    int   GetErrorType();
    char *GetErrorString();
    void  SetStatus(int status);
    int   GetStatus();
    void  Close();
    int   Running();
    unsigned long Get_bytes_received();
    unsigned long GetBytesDownloaded();
    unsigned long GetTimeDownloaded();
    unsigned long GetBytesUnzipped();
    unsigned long GetTimeUnzipped();
    int   SetReadIntoMemory(int size);
    // Inline: its one copy comes after __tf8Http_get, out of line with the vtable.
    void  SetUrlListener(VIUrlListener *l) { m_listener = l; }

    char  m_x000[768];           // +0x000
    char  m_x300[4892 - 768];    // +0x300, its first byte cleared per Get
    VIUrlListener *m_listener;   // +0x131C
    char  m_x1320[16];           // +0x1320
    VIPatchImage *m_images;      // +0x1330
    char *m_x1334;               // +0x1334 Initialize
    char *m_errorString;         // +0x1338
    int   m_errorType;           // +0x133C
    unsigned int m_status;       // +0x1340
    int   m_size;                // +0x1344
    char *m_mem;                 // +0x1348
    unsigned long m_bytesDownloaded;   // +0x1350
    unsigned long m_bytesUnzipped;     // +0x1358
    unsigned long m_timeDownloaded;    // +0x1360
    unsigned long m_timeUnzipped;      // +0x1368
    char *m_url;                 // +0x1370
    int   m_x1374;               // +0x1374
    char *m_dest;                // +0x1378
    char *m_x137C;               // +0x137C
    char *m_x1380;               // +0x1380
    int   m_compressed;          // +0x1384
    int   m_destSize;            // +0x1388
    int   m_closed;              // +0x138C
    int   m_running;             // +0x1390
    int   m_readIntoMemory;      // +0x1394
    char *m_temp;                // +0x1398
    int   m_memSize;             // +0x139C
    long  m_x13A0;               // +0x13A0
    int   m_x13A8;               // +0x13A8
    int   m_magic[2];            // +0x13AC the gzip magic, 0x1F 0x8B
};                               // vptr +0x13B4

long Get_thread(void *http);

// A log that writes nowhere.
class Log
{
public:
    Log(char *name, int level);
    ~Log();
    void level(int l);
    void write(int l, char *fmt, ...);
    void close();

    int m_x0;                    // +0x00
    int m_level;                 // +0x04
};

// What the game sees: a handle (m_impl, then the vptr), forwarding every call;
// the virtual On* callbacks are the client's (VISetup's) to fill in.
class PatcherAPI
{
public:
    PatcherAPI();
    virtual ~PatcherAPI();
    virtual int Initialize(const char *url, int port, const char *file, int flags);
    virtual int Initialize(const char *url, int port, const char *file, const char *a,
                           const char *b, int flags);
    virtual int InitializeFromLocalFile(const char *file, int flags);
    virtual int ValidUser(const char *user, const char *password);
    virtual int OnStart() = 0;
    virtual int OnProduct(const char *name) = 0;
    virtual int OnFile(const char *name) = 0;
    virtual int OnUnzip(const char *name) = 0;
    virtual int OnFileFinished(const char *name) = 0;
    virtual int OnFinish() = 0;
    virtual int OnCancel() = 0;
    virtual int OnForceRestart(const char *name, bool now);
    virtual int OnForceComputerRestart() = 0;
    virtual int OnSetTickCount(int n) = 0;
    virtual int OnSetFileTickCount(int n) = 0;
    virtual int OnSetScanTickCount(int n);
    virtual int OnTick(int n) = 0;
    virtual int OnFileTick(int n) = 0;
    virtual int OnScanTick(int n);
    virtual int OnStatusInfo(const char *text) = 0;
    virtual int OnEstimate(long total, long left) = 0;
    virtual int OnError(int code, const char *text) = 0;
    virtual int OnFatalError(int code, const char *text) = 0;

    int NeedDownload(int);
    int DownloadAll(int);
    int DeleteOldFiles();
    int Download(const char *, int);
    int Uninstall(const char *);
    int Select(const char *, bool);
    PatcherDistributionInfo *GetDistributionList();
    int Cancel(int flags);
    int Continue();
    const char *GetProductName();
    short GetVersion();
    PatcherChanges *GetProductChanges();
    int SetRoot();
    int SetDistributionRoot(const char *, const char *);
    int RescanFiles(const char *, int);
    int RescanTestFiles(const char *, int);
    int SetLocal();
    const char *GetRoot(const char *);
    char *GetLocal();
    int UseCRC(bool, int);
    int UsingCRC();
    long GetEstimate();
    long GetEstimateRemaining();
    int GetFileCount(const char *, int);
    long GetTotalSize(const char *, int);
    long GetDownloadSize(const char *, int);
    int DownloadFile(char *, int, char *, char *, int, long, long);
    int DownloadToMemory(char *, int, char *, int, char *, int);
    int GetFile(const char *, const char *, int);
    char *DownloadMemoryFile(int size, char *server, int port, char *path, int compressed);
    void SetRestartName(const char *, const char *);
    void Restart(bool);
    void PatchPatcher(const char *, const char *);
    int IsBusy();
    static int GenerateLocalPath(const char *path);
    int SetCheckOnInitialize(bool);
    int CheckOnInitialize();
    int SetQuickScan(bool);
    int QuickScan();
    int IncrementScanCount();
    int InitialScan(const char *);
    void SetPatchImage(VIPatchImage *);
    VIPatchImage *FindPatchImage(const char *);

    PatcherAPI_Impl *m_impl;     // +0x00
};

class PatcherDownloader;
class DistributionDownloader;
class PatcherChanges_Impl;
class PatcherDistributionInfo_Impl;

// One product's release notes, a doubly linked list of versions (24 bytes).
class PatcherChanges_Impl
{
public:
    PatcherChanges_Impl();
    virtual ~PatcherChanges_Impl();
    int   Initialize(int version, char *html, PatcherChanges *prev, PatcherChanges *next);
    short GetVersion();
    char *GetTextChanges();
    char *GetHtmlChanges();
    int   ExtractTextChangesFromHTMLChanges();
    PatcherChanges *GetPrevious();
    PatcherChanges *GetNext();
    int   SetPrevious(PatcherChanges *p);
    int   SetNext(PatcherChanges *p);

    short           m_version;   // +0x00
    char           *m_text;      // +0x04
    char           *m_html;      // +0x08
    PatcherChanges *m_prev;      // +0x0C
    PatcherChanges *m_next;      // +0x10
};                               // vptr +0x14

class PatcherChanges
{
public:
    PatcherChanges();
    virtual ~PatcherChanges();
    virtual int Initialize(int version, char *html, PatcherChanges *prev, PatcherChanges *next);
    PatcherChanges *GetPrevious();
    PatcherChanges *GetNext();
    int   SetPrevious(PatcherChanges *p);
    int   SetNext(PatcherChanges *p);
    short GetVersion();
    char *GetTextChanges();
    char *GetHtmlChanges();

    PatcherChanges_Impl *m_impl; // +0x00
};

// The patcher's tree of downloaders: a directory, a distribution (a product or
// a component), a file.  104 bytes, the vptr last.
class PatcherDownloader
{
public:
    PatcherDownloader();
    virtual ~PatcherDownloader();
    virtual int  ScanAsTest();
    virtual int  Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                            const char *root, const char *path);
    virtual int  Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                            const char *root, const char *path, PatcherDistributionInfo *info);
    virtual int  UseCRC(bool on);
    virtual int  UsingCRC();
    virtual int  IgnoreTimeStamp(bool on);
    virtual int  SetDistributionRoot(const char *root);
    virtual int  ScanFiles(int flags);
    virtual const char *GetDistributionRoot();
    virtual int  SetXMLInfo(_xmlNode *node) = 0;
    virtual long GetTotalSize(int flags) = 0;
    virtual long GetDownloadSize(int flags) = 0;
    virtual int  Download(int flags) = 0;
    virtual int  Delete() = 0;
    virtual int  GetFileCount(int flags) = 0;
    virtual int  PrepForUninstall();
    virtual PatcherDistributionInfo *GetDistributionInfo();
    virtual int  SetDistributionInfo(PatcherDistributionInfo *info);
    virtual int  DirectoryAvailable();
    virtual int  SetDirectoryAvailable(bool on);
    virtual void ResetScanFlag(int flags);

    int  IgnoringTimeStamp();
    int  IsInstallOnly();
    int  CalculateHierarchicalProperties();
    int  FlagDownloaded();
    int  FlagDeleted();
    int  NeedDownload(int flags);
    int  NeedDelete();
    char *GetServer();
    int  GetPort();
    int  IsDistributionDownloader();
    PatcherDownloader *GetPrevious();
    PatcherDownloader *GetNext();
    PatcherDownloader *GetParent();
    PatcherDownloader *GetChild();
    int  SetParent(PatcherDownloader *p);
    int  SetChild(PatcherDownloader *p);
    int  SetPrevious(PatcherDownloader *p);
    int  SetNext(PatcherDownloader *p);
    static int GenerateLocalPath(const char *path);
    static int GenerateLocalSubpath(const char *done, const char *path, const char *cwd);
    int  SetTopLevel(bool on);
    int  IsTopLevel();
    PatcherDownloader *GetDownloaderByFile(const char *file);

    char       *m_name;          // +0x00 "Name"
    char       *m_server;        // +0x04 "Server", else the parent's
    int         m_port;          // +0x08 "Port", else the parent's
    char       *m_remotePath;    // +0x0C "RemotePath"
    char       *m_localPath;     // +0x10 "LocalPath"
    char       *m_defaultRoot;   // +0x14
    char       *m_root;          // +0x18
    int         m_useCRC;        // +0x1C 0 no, 1 yes, 2 as the parent does
    int         m_ignoreTimeStamp;  // +0x20 "IgnoreTimeStamp"
    int         m_installOnly;   // +0x24 "InstallOnly"
    int         m_scanAsTest;    // +0x28 a child is a test file
    int         m_special[2];    // +0x2C "TestFile", +0x30 "BetaFile"
    int         m_xtraFile;      // +0x34 "XtraFile"
    int         m_needDownload;  // +0x38
    int         m_delete;        // +0x3C "Delete"
    int         m_isDistribution;   // +0x40
    PatcherDownloader *m_next;   // +0x44, owned
    PatcherDownloader *m_prev;   // +0x48
    PatcherDownloader *m_child;  // +0x4C, owned
    PatcherDownloader *m_parent; // +0x50
    PatcherDistributionInfo *m_info;   // +0x54
    PatcherAPI *m_api;           // +0x58
    int         m_dirAvailable;  // +0x5C
    int         m_topLevel;      // +0x60
};                               // vptr +0x64

// The cancel flag: PatcherAPI_Impl::Cancel sets it and every downloader loop
// polls it.
extern int gPatcherCancel;

class DirectoryDownloader : public PatcherDownloader
{
public:
    DirectoryDownloader();
    virtual ~DirectoryDownloader();
    virtual int  Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                            const char *root, const char *path);
    virtual int  UseCRC(bool on);
    virtual int  SetXMLInfo(_xmlNode *node);
    virtual long GetTotalSize(int flags);
    virtual long GetDownloadSize(int flags);
    virtual int  Download(int flags);
    virtual int  Delete();
    virtual int  GetFileCount(int flags);
    int  InitializeChildren(char *local, char *remote, _xmlNode *node);
    PatcherDownloader *ScanForDownloaderType(char *type);
    int  MakeDirectory();
    int  DeleteDirectory();
    static char *GetFullPath(const char *name, const char *path, const char *root);

    char *m_localName;           // +0x68 "LocalName"
};

// One delta patch a <File> offers: the bytes that turn the copy whose CRC is
// m_crc into the current one.  36 bytes, no vtable; each owns the next.
class FileDeltaDownloader
{
public:
    FileDeltaDownloader();
    ~FileDeltaDownloader();
    int  Initialize(const char *server, int port, const char *remotePath, const char *deltaPath,
                    const char *localPath, int downloadSize, int totalSize, int crc);
    int  Download(char *buf, int size, PatcherAPI *api);
    int  SetNext(FileDeltaDownloader *next);
    FileDeltaDownloader *GetNext();
    int  Matches(unsigned int crc);
    char *GetDeltaFilePath();
    int  GetDownloadSize();
    int  GetTotalSize();

    char *m_server;              // +0x00
    int   m_port;                // +0x04
    char *m_remotePath;          // +0x08
    char *m_deltaPath;           // +0x0C
    char *m_localPath;           // +0x10
    int   m_downloadSize;        // +0x14
    int   m_totalSize;           // +0x18
    unsigned int m_crc;          // +0x1C
    FileDeltaDownloader *m_next; // +0x20, owned
};

// A <File>.  184 bytes.
class FileDownloader : public PatcherDownloader
{
public:
    FileDownloader();
    virtual ~FileDownloader();
    virtual int  ScanAsTest();
    virtual int  Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                            const char *root, const char *path);
    virtual int  UseCRC(bool on);
    virtual int  UsingCRC();
    virtual int  SetDistributionRoot(const char *root);
    virtual int  ScanFiles(int flags);
    virtual int  SetXMLInfo(_xmlNode *node);
    virtual long GetTotalSize(int flags);
    virtual long GetDownloadSize(int flags);
    virtual int  Download(int flags);
    virtual int  Delete();
    virtual int  GetFileCount(int flags);
    virtual int  AddDeltaFile(_xmlNode *node, const char *deltaName, const char *deltaLocal,
                              const char *filePath, const char *remotePath);
    virtual int  CheckForNeedDownload();
    virtual int  InitializeFilePath();
    int  FileDiscrepancy();
    int  PartialPatchCheck();
    int  NeedsPartialPatch();
    int  PartialPatchDownload(bool self);
    int  SizeDiscrepancy(void *);
    long TimeDiscrepancy(void *);
    unsigned int Get_crc(char *path);
    int  CRCDiscrepancy();
    long ParseTime(char *text);
    char *GetFullPath(const char *name, const char *path, const char *root);

    int   m_discrepancy;         // +0x68
    unsigned int m_localCRC;     // +0x6C
    FileDeltaDownloader *m_deltas;  // +0x70, owned
    FileDeltaDownloader *m_delta;   // +0x74, the one that matches m_localCRC
    char *m_localName;           // +0x78 "LocalName"
    char *m_tempName;            // +0x7C "TempFilename"
    int   m_partialPatch;        // +0x80
    long  m_totalSize;           // +0x88 "TotalSize"
    long  m_downloadSize;        // +0x90 "DownloadSize"
    long  m_time;                // +0x98 "TimeStamp"
    unsigned int m_crc;          // +0xA0 "CRC"
    int   m_compressed;          // +0xA4 "Compressed"
    int   m_restart;             // +0xA8 "Restart"
    int   m_keepBackup;          // +0xAC "Keep_Backup"
    int   m_unchecked;           // +0xB0
};

// A <Distribution>: a product or a component, which may live in an XML file of
// its own on the server.  140 bytes.
class DistributionDownloader : public PatcherDownloader
{
public:
    DistributionDownloader();
    virtual ~DistributionDownloader();
    virtual int  Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                            const char *root, const char *path);
    virtual int  UseCRC(bool on);
    virtual int  ScanFiles(int flags);
    virtual int  SetXMLInfo(_xmlNode *node);
    virtual long GetTotalSize(int flags);
    virtual long GetDownloadSize(int flags);
    virtual int  Download(int flags);
    virtual int  Delete();
    virtual int  GetFileCount(int flags);
    virtual int  PrepForUninstall();
    virtual void ResetScanFlag(int flags);
    int  DistributionIsExternal(_xmlNode *node);
    int  GetExternalTree(_xmlNode *node);
    int  ExternalInitialize(_xmlNode *node);
    int  TestAndFinalizeDistribution(_xmlNode *node);
    int  InitializeChildren(_xmlNode *node);
    static PatcherDownloader *ScanForDownloaderType(char *type);
    int  SetChangesFile(_xmlNode *node);
    PatcherDistributionInfo *GetXtraDistribution(int which);
    void SetXtraDistribution(int which, bool on);
    int  IsXtraDistribution(int which);

    char *m_xtraName[2];         // +0x68 "TestDistribution", "BetaDistribution"
    int   m_isXtra[2];           // +0x70
    int   m_scanFlag[5];         // +0x78 ScanFiles, GetFileCount, GetDownloadSize,
                                 //       GetTotalSize, and "checked on initialize"
};

// One distribution's description, out of the XML manifest (88 bytes).
class PatcherDistributionInfo_Impl
{
public:
    PatcherDistributionInfo_Impl();
    virtual ~PatcherDistributionInfo_Impl();
    bool Initialize(_xmlNode *node, PatcherAPI *api);
    PatcherAPI *FindPatcherAPI();
    int  CheckPatcherAPI();
    int  NeedDownload(int flags);
    int  NeedDelete();
    PatcherDistributionInfo *GetParent();
    PatcherDistributionInfo *GetChild();
    PatcherDistributionInfo *GetPrevious();
    PatcherDistributionInfo *GetNext();
    PatcherAPI *GetPatcher();
    char *GetName();
    char *GetTitle();
    char *GetPath();
    char *GetServer();
    int  GetPort();
    bool IsOptional();
    bool IsSelected();
    bool Select(bool on);
    int  SelectChildren(bool on);
    int  IsRestricted();
    char *GetRestrictedFilename();
    int  RestrictedFileExists();
    long GetTotalSize(int flags);
    long GetDownloadSize(int flags);
    int  GetFileCount(int flags);
    int  InitialScan();
    int  SetDistributionRoot(const char *root);
    const char *GetDistributionRoot();
    int  RescanFiles(int flags);
    int  Download(int flags);
    int  Continue();
    int  DeleteOld();
    int  Uninstall();
    int  UseCRC(bool on);
    int  UsingCRC();
    int  SetChanges(PatcherChanges *changes);
    PatcherChanges *GetChanges();
    DistributionDownloader *GetDistributionDownloader();
    int  SetDistributionDownloader(DistributionDownloader *d);
    int  SetDistributionInfo(PatcherDistributionInfo *info);
    short GetVersion();
    int  SetXMLInfo(_xmlNode *node);
    PatcherDistributionInfo *GetDistributionInfo();
    void SetXtraDistribution(int which, bool on);
    int  IsXtraDistribution(int which);
    int  CalculateHierarchicalProperties();
    PatcherDownloader *GetDownloaderByFile(const char *file);
    void ResetScanFlag(int flags);

    // The callbacks, guarded.  Inline: the copies the map names are the ones
    // emitted after __tf (0x00588130 ..).
    int IncrementScanCount()
    {
        if (m_api)
            return m_api->IncrementScanCount();
        return 0;
    }
    int CallFinish()
    {
        if (m_api)
            return m_api->OnFinish();
        return 0;
    }
    int CallForceRestart(char *name, bool now)
    {
        if (m_api)
            return m_api->OnForceRestart(name, now);
        return 0;
    }
    int CallForceComputerRestart()
    {
        if (m_api)
            return m_api->OnForceComputerRestart();
        return 0;
    }
    int CallError(int code, char *msg)
    {
        if (m_api)
            return m_api->OnError(code, msg);
        return 0;
    }
    int CallFatalError(int code, char *msg)
    {
        if (m_api)
            return m_api->OnFatalError(code, msg);
        return 0;
    }

    char       *m_name;          // +0x00 "Name"
    char       *m_title;         // +0x04 "Title"
    char       *m_path;          // +0x08 "RemotePath"
    char       *m_server;        // +0x0C "Server"
    int         m_port;          // +0x10 "Port"
    int         m_version;       // +0x14, read back as a short
    bool        m_optional;      // +0x18 "Optional"
    int         m_restricted;    // +0x1C "Restricted"
    char       *m_restrictedFile;   // +0x20 "RestrictedFile"
    int         m_restrictedFileExists; // +0x24
    bool        m_selected;      // +0x28
    long        m_x30;           // +0x30
    long        m_x38;           // +0x38
    PatcherAPI *m_api;           // +0x40
    int         m_x44;           // +0x44
    PatcherDistributionInfo *m_info;     // +0x48
    PatcherChanges          *m_changes;  // +0x4C
    DistributionDownloader  *m_downloader; // +0x50
};                               // vptr +0x54

class PatcherDistributionInfo
{
public:
    PatcherDistributionInfo();
    virtual ~PatcherDistributionInfo();
    bool Initialize(_xmlNode *node, PatcherAPI *api);
    int  NeedDownload(int flags);
    PatcherDistributionInfo *GetParent();
    PatcherDistributionInfo *GetChild();
    PatcherDistributionInfo *GetPrevious();
    PatcherDistributionInfo *GetNext();
    PatcherAPI *GetPatcher();
    char *GetName();
    char *GetTitle();
    char *GetPath();
    char *GetServer();
    int  GetPort();
    bool IsOptional();
    bool IsSelected();
    bool Select(bool on);
    int  SelectChildren(bool on);
    int  IsRestricted();
    char *GetRestrictedFilename();
    int  RestrictedFileExists();
    long GetTotalSize(int flags);
    long GetDownloadSize(int flags);
    int  GetFileCount(int flags);
    int  InitialScan();
    int  SetDistributionRoot(const char *root);
    const char *GetDistributionRoot();
    int  RescanFiles(int flags);
    int  Download(int flags);
    int  Continue();
    int  DeleteOld();
    int  Uninstall();
    int  UseCRC(bool on);
    int  UsingCRC();
    int  SetChanges(PatcherChanges *changes);
    PatcherChanges *GetChanges();
    short GetVersion();
    static PatcherDistributionInfo *GetDistributionByName(PatcherDistributionInfo *info,
                                                          const char *name);
    void SetXtraDistribution(int which, bool on);
    int  IsXtraDistribution(int which);
    int  CalculateHierarchicalProperties();
    void ResetScanFlag(int flags);

    PatcherDistributionInfo_Impl *m_impl;   // +0x00
};

// The delta coder (0x0057CEB0 ..).  A delta is the big-endian int 1, the new
// size, then runs: a negative length -n and n literal bytes, or a length n and
// the offset in the old data of n bytes to copy.
char *MakeDeltaData(char *from, int fromSize, char *to, int toSize, int *outSize, int step);
char *UnmakeDeltaData(char *data, int size, char *delta, int deltaSize, int *outSize);
int   UnmakeDeltaDataFile(char *file, char *delta, int deltaSize);
void *DeltaIBSearch(void *key, void *base, int count, int size);

// The patcher's utilities (0x00589078 ..).
char *FormatSeconds(long secs);
char *FormatSecondsToMinutes(long secs);
char *FormatTime(long t);
char *FormatTimeMinutes(long t);
char *FormatString(char *buf, char *fmt, ...);
char *GetTempName(char *ext);
int   GetCurrentExecutableName(char *buf);
void  defMain(void *arg);

// The kernel's thread record, as CreateThread reads it.
struct ThreadParam
{
    int   status;           // +0x00
    void (*entry)(void *);  // +0x04
    void *stack;            // +0x08
    int   stackSize;        // +0x0C
    void *gpReg;            // +0x10
    int   initPriority;     // +0x14
    int   currentPriority;  // +0x18
    unsigned int attr;      // +0x1C
    unsigned int option;    // +0x20
    int   waitType;         // +0x24
    int   waitId;           // +0x28
    int   wakeupCount;      // +0x2C
};

extern "C" int CreateThread(ThreadParam *param);
extern "C" int StartThread(int id, void *arg);
extern "C" int TerminateThread(int id);
extern "C" int DeleteThread(int id);
extern "C" char _gp[];

// uthread: a Win32-shaped thread handle over the EE kernel (56 bytes).
struct uthread
{
    int         id;         // +0x00, -1 until created
    ThreadParam param;      // +0x04
    void       *stack;      // +0x34, kept to free
};

extern "C" void uthread_init(uthread *t);
extern "C" int  uthread_create(uthread *t, void (*func)(void *), void *arg);
extern "C" void uthread_destroy(uthread *t);

int VIMemCardPrepare(int port);

// A file on the memory card (12 bytes): the name, the open descriptor, the
// sceMcOpen mode.
class VMemCardFile
{
public:
    char *m_name;   // +0x00, strdup'd
    int   m_fd;     // +0x04, -1 while closed
    int   m_mode;   // +0x08

    VMemCardFile(const char *name, int mode);
    ~VMemCardFile();
    int Mkdirhier(const char *path);
    int Open();
    int Close();
    int InitMemCard();
    int Write(char *buf, int size);
    int Read(char *buf, int size);
    int GetSize();
};

// PullXMLAttribute*: the manifest readers (0x00589B50 ..).
char *PullXMLAttribute(_xmlNode *node, const char *name);
int   PullXMLAttributeLong(_xmlNode *node, const char *name);   // atoi: the callers truncate nothing
float PullXMLAttributeFloat(_xmlNode *node, const char *name);
bool  PullXMLAttributeBool(_xmlNode *node, const char *name);
bool  PullXMLAttributeBool(_xmlNode *node, const char *name, bool def);
int   PullDefinedXMLAttBool(_xmlNode *node, const char *name);

#endif
