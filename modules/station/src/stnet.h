// stnet.h -- OWNER: lane net.  CLIENT's VIMemStream containers as STATION
// links them (VIMemStreamArray.cpp, VIMemStreamSet.cpp, VIMemStreamVector.cpp,
// VIMemStreamVector.h: 0x002E36E0 .. 0x002E4E18), the flat-file reader
// (0x002E2258 .. 0x002E2E00), the time formatters (0x002E1FF8 .. 0x002E2258) and
// the send helpers (0x002E4E18 .. 0x002E53F0).  Most of this is CLIENT's code
// too: prefer a shared source (a symlink, as vimemstream.cpp is) when the TU is
// identical in both modules.
#ifndef STNET_H
#define STNET_H

#include "vidatetime.h"   // CLIENT-identical: VITime, VIDate
#include "vimemstreamx.h"  // CLIENT-identical: the container stream operators
#include "vinet.h"         // CLIENT-identical: VINet.cpp

// ---- viflatfile.cpp -- 0x002E2258 .. 0x002E2E00 ----------------------------
// STATION.PAK in memory: a tree of named byte blocks.  A file (type 1) owns a
// copy of its name and data; a directory (type 2) is a file with a list of
// files under it.

// 0x14 bytes, vptr at +0x10.  ctor 0x002E2258, virtual dtor 0x002E2280, __tf
// 0x002E2D48.  The accessors are out of line (0x002E23B8 ..), so the walks
// below CALL them.
class VIFlatFile
{
public:
    VIFlatFile(int type);
    virtual ~VIFlatFile();

    void  Set(const char *name, const void *data, int size);   // 0x002E2300
    void *GetData();                                          // 0x002E23B8
    int   GetSize();                                          // 0x002E23C0
    int   GetType();                                          // 0x002E23C8
    char *GetName();                                          // 0x002E23D0

    char *m_name;   // +0x00  strdup'd
    void *m_data;   // +0x04  malloc'd copy
    int   m_size;   // +0x08  -1 while empty
    int   m_type;   // +0x0C  1 a file, 2 a directory
};
CHK(c_VIFlatFile, sizeof(VIFlatFile) == 0x14)

// 0x24 bytes.  ctor 0x002E23D8, virtual dtor 0x002E2420 (deletes every child),
// __tf 0x002E2D88.
class VIFlatFileDirectory : public VIFlatFile
{
public:
    VIFlatFileDirectory();
    virtual ~VIFlatFileDirectory();

    void        Init(VIFlatFileDirectory *parent, const char *name);    // 0x002E24F0
    void        Add(VIFlatFile *f);                                     // 0x002E2540
    VIFlatFile *Find(const char *path);                                 // 0x002E2568
    VIFlatFile *AddFile(const char *path, const void *data, int size);  // 0x002E26A0

    VIFlatFileDirectory  *m_parent;   // +0x14
    VIList<VIFlatFile *>  m_files;    // +0x18
};
CHK(c_VIFlatFileDirectory, sizeof(VIFlatFileDirectory) == 0x24)

// The root, named "/", and the pak loader.  No vtable: its destructor is not
// virtual.  VIWndGenericRenderer's constructor builds one in place.  Name ours.
class VIFlatFileSystem
{
public:
    VIFlatFileSystem();                              // 0x002E28B8
    ~VIFlatFileSystem();                             // 0x002E28F8
    void        Load(const char *data, int size);    // 0x002E2940
    VIFlatFile *Find(const char *path);              // 0x002E2AE0

    VIFlatFileDirectory m_root;   // +0x00
};
CHK(c_VIFlatFileSystem, sizeof(VIFlatFileSystem) == 0x24)

#endif
