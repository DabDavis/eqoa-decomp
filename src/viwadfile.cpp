// VIWadFile -- SUPPORT 0x003EDE70 .. 0x003EE220.
//
// A flat archive over a VIFile.  The header is three words (magic, version,
// entry count), the directory is `count` 16-byte records, and the data starts
// at a fixed 256 bytes in.  Lookup is a LINEAR walk over a case-folded string
// hash: there is no ordering, no bucket table, and a collision is a wrong file.
#include "vi.h"

extern "C" char *strcpy(char *, const char *);

unsigned int VIWadFile::WadFileID;

VIWadFile::VIWadFile()
{
    m_numEntries = 0;
    m_entries = 0;
}

// Empty: the whole body is m_file's destructor and the delete.
VIWadFile::~VIWadFile()
{
}

int VIWadFile::Init(const char *path)
{
    if (m_file.Open(path, 33) < 0)
        return -1;
    return SharedInit();
}

int VIWadFile::Init(void *p, int size)
{
    if (m_file.Open(p, size, 33, size) < 0)
        return -1;
    return SharedInit();
}

void VIWadFile::Free()
{
    m_file.Close();
    if (m_entries)
        delete [] m_entries;
    m_entries = 0;
}

int VIWadFile::GetSize(const char *name)
{
    WadEntry *e = LookupEntry(name);
    if (e)
        return e->size;
    return -1;
}

int VIWadFile::Load(const char *name, void *dst, int size)
{
    WadEntry *e = LookupEntry(name);
    if (e == 0)
        return -1;
    if (size < e->size)
        return 0;
    if (m_file.Seek(e->offset + 256, VIFile::Seek_Set) < 0)
        return -1;
    if (m_file.Read(dst, e->size) < 0)
        return -1;
    return e->size;
}

int VIWadFile::SharedInit()
{
    int id;
    int version;
    int count;

    if (m_file.Read(&id, 4) < 0)
        return -1;
    if (id != WadFileID)
        return -1;
    if (m_file.Read(&version, 4) < 0)
        return -1;
    if (version >= 2)
        return -1;
    if (m_file.Read(&count, 4) < 0)
        return -1;
    m_numEntries = count;
    m_entries = new WadEntry[count];
    if (m_file.Read(m_entries, m_numEntries * sizeof(WadEntry)) < 0)
        return -1;
    return 0;
}

VIWadFile::WadEntry *VIWadFile::LookupEntry(const char *name)
{
    unsigned int h = HashStringLowercase(name);
    WadEntry *e = 0;
    for (int i = 0; i < m_numEntries; i++)
    {
        if (m_entries[i].hash == h)
        {
            e = &m_entries[i];
            break;
        }
    }
    return e;
}

unsigned int VIWadFile::HashStringLowercase(const char *name)
{
    char buf[256];

    char *p = buf;

    strcpy(buf, name);
    unsigned int h = 0;
    if (*p)
    {
        do
        {
            unsigned char c = *p;
            if (c >= 'A' && c <= 'Z')
                *p = c + 32;
            h = h * 131 + *p;
            p++;
        }
        while (*p);
    }
    return h;
}
