// viflatfile.cpp (name ours) -- STATION 0x002E2258 .. 0x002E2E00: STATION.PAK
// held in memory as a tree of flat files, and the loader that builds it.  The
// pak is a run of big-endian records: a name length, the name, a data length,
// the data.  A record's path names its directories ("a/b/c"), which are made
// as they are first met.  The VIList<VIFlatFile *> copies follow, then the two
// type_info functions (the directory's inlines its base's).
#include "station.h"
#include "vilist_t.h"

// 0x002E2258
VIFlatFile::VIFlatFile(int type)
{
    m_name = 0;
    m_data = 0;
    m_size = -1;
    m_type = type;
}

// 0x002E2280
VIFlatFile::~VIFlatFile()
{
    if (m_name)
    {
        free(m_name);
        m_name = 0;
    }
    if (m_data)
    {
        free(m_data);
        m_data = 0;
    }
    m_size = -1;
    m_type = 0;
}

// 0x002E2300: a copy of both; nothing without a name, data and a size.
void VIFlatFile::Set(const char *name, const void *data, int size)
{
    if (!name || !data || size <= 0)
        return;
    if (m_name)
        free(m_name);
    m_name = strdup(name);
    if (m_data)
        free(m_data);
    m_data = malloc(size);
    memcpy(m_data, data, size);
    m_size = size;
    printf("Adding flat file %s\n", name);
}

// 0x002E23B8
void *VIFlatFile::GetData()
{
    return m_data;
}

// 0x002E23C0
int VIFlatFile::GetSize()
{
    return m_size;
}

// 0x002E23C8
int VIFlatFile::GetType()
{
    return m_type;
}

// 0x002E23D0
char *VIFlatFile::GetName()
{
    return m_name;
}

// 0x002E23D8
VIFlatFileDirectory::VIFlatFileDirectory()
    : VIFlatFile(2)
{
    m_parent = 0;
}

// 0x002E2420: every child goes with it.
VIFlatFileDirectory::~VIFlatFileDirectory()
{
    int i = m_files.Begin();
    while (i != m_files.End())
    {
        VIFlatFile *f = ((VIList<VIFlatFile *>::Node *)i)->m_data;
        int next = m_files.Erase(i);
        delete f;
        i = next;
    }
}

// 0x002E24F0
void VIFlatFileDirectory::Init(VIFlatFileDirectory *parent, const char *name)
{
    if (!name)
        return;
    m_parent = parent;
    if (m_name)
        free(m_name);
    m_name = strdup(name);
}

// 0x002E2540
void VIFlatFileDirectory::Add(VIFlatFile *f)
{
    if (f)
        m_files.Add(f);
}

// 0x002E2568: the first path element names a file here, or a directory to
// look on in.
VIFlatFile *VIFlatFileDirectory::Find(const char *path)
{
    if (!path)
        return 0;
    VIFlatFile *found = 0;
    char *copy = strdup(path);
    char *tok = strtok(copy, "/");
    if (tok)
    {
        for (int i = m_files.Begin(); i != m_files.End(); i = m_files.Next(i))
        {
            VIFlatFile *f = ((VIList<VIFlatFile *>::Node *)i)->m_data;
            if (!f)
                continue;
            char *name = f->GetName();
            if (!name)
                continue;
            if (strcasecmp(tok, name))
                continue;
            if (f->GetType() == 1)
            {
                found = f;
                break;
            }
            if (f->GetType() == 2)
            {
                found = ((VIFlatFileDirectory *)f)->Find(tok + strlen(tok) + 1);
                break;
            }
        }
    }
    free(copy);
    return found;
}

// 0x002E26A0: Find, and what is not there is made.  A path with a "/" in it
// makes a directory for its first element and goes on in it.
VIFlatFile *VIFlatFileDirectory::AddFile(const char *path, const void *data, int size)
{
    if (!path)
        return 0;
    VIFlatFile *found = 0;
    char *copy = strdup(path);
    char *tok = strtok(copy, "/");
    if (tok)
    {
        for (int i = m_files.Begin(); i != m_files.End(); i = m_files.Next(i))
        {
            VIFlatFile *f = ((VIList<VIFlatFile *>::Node *)i)->m_data;
            if (!f)
                continue;
            char *name = f->GetName();
            if (!name)
                continue;
            if (strcasecmp(tok, name))
                continue;
            if (f->GetType() == 1)
            {
                found = f;
                break;
            }
            if (f->GetType() == 2)
            {
                found = ((VIFlatFileDirectory *)f)->AddFile(tok + strlen(tok) + 1, data, size);
                break;
            }
        }
        if (!found)
        {
            if (!strstr(path, "/"))
            {
                VIFlatFile *f = new VIFlatFile(1);
                f->Set(tok, data, size);
                Add(f);
                found = f;
            }
            else
            {
                VIFlatFileDirectory *d = new VIFlatFileDirectory;
                d->Init(this, tok);
                Add(d);
                found = d->AddFile(tok + strlen(tok) + 1, data, size);
            }
        }
    }
    free(copy);
    return found;
}

// 0x002E28B8
VIFlatFileSystem::VIFlatFileSystem()
{
    m_root.Init(0, "/");
}

// 0x002E28F8
VIFlatFileSystem::~VIFlatFileSystem()
{
}

#define SWAP32(x) ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) >> 8) & 0xFF00) | \
                   (((x) >> 24) & 0xFF))

// 0x002E2940: the pak's records, each added to the tree; a record that runs
// past the end stops the load.
void VIFlatFileSystem::Load(const char *data, int size)
{
    if (!data || size <= 0)
        return;
    int pos = 0;
    while (pos < size)
    {
        char name[1024];
        unsigned int nameLen = 0;
        memcpy(&nameLen, data + pos, 4);
        pos += 4;
        nameLen = SWAP32(nameLen);
        if (pos > size)
            return;
        memset(name, 0, 1024);
        memcpy(name, data + pos, nameLen);
        pos += nameLen;
        if (pos > size)
            return;
        unsigned int dataLen = 0;
        memcpy(&dataLen, data + pos, 4);
        pos += 4;
        dataLen = SWAP32(dataLen);
        if (pos > size)
            return;
        char *buf = (char *)malloc(dataLen);
        if (!buf)
            return;
        memset(buf, 0, dataLen);
        memcpy(buf, data + pos, dataLen);
        pos += dataLen;
        if (pos > size)
        {
            free(buf);
            return;
        }
        m_root.AddFile(name, buf, dataLen);
        free(buf);
    }
}

// 0x002E2AE0: a leading "/" is the root itself.
VIFlatFile *VIFlatFileSystem::Find(const char *path)
{
    if (!path)
        return 0;
    if (path[0] == '/')
        path++;
    return m_root.Find(path);
}
