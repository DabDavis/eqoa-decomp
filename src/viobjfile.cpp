// VIObjFile -- SUPPORT 0x003E9C80 .. 0x003EC1D8.
//
// The ESF/CSF container layer, on top of VIFile.  A 32-byte file header at
// offset 0, then a tree of objects, each introduced by a 12-byte record
// (type, version, size, sub-object count) and closed by patching that record's
// size and count back in place.  m_dirs is the open-object stack -- ten deep,
// which every entry point tests as `m_level == 9`.
//
// Everything on the wire goes through VIBufferSwap when the global VIEndian is
// not 1, and every function that swaps reads that global into a LOCAL: the
// target keeps it in a callee-saved register across the VIFile calls between
// the two swap blocks, which a re-read of the global could not do.
#include "vi.h"

extern int VIEndian;
void VIBufferSwap(int n, unsigned char *p);

extern "C" int strlen(const char *);

unsigned int VIObjFile::ObjFileID;

VIObjFile::VIObjFile()
{
    ResetState();
}

VIObjFile::~VIObjFile()
{
    if (m_open)
        Close();
}

int VIObjFile::ResetState()
{
    m_open = 0;
    m_write = 0;
    m_read = 0;
    m_writeOpen = 0;
    m_readOpen = 0;
    m_canWrite = 0;
    m_canRead = 0;
    m_dirty = 0;
    m_level = -1;
    return 0;
}

int VIObjFile::Open(const char *path, FileMode mode)
{
    if (m_open)
        Close();
    int flags;
    if (mode == Mode_Read)
        flags = 33;
    else
        flags = (mode == Mode_Update) ? 50 : 42;
    if (m_file.Open(path, flags) < 0)
        return -1;
    m_mode = mode;
    m_open = 1;
    return PostOpen();
}

int VIObjFile::Open(void *p, long size, long len, FileMode mode)
{
    if (m_open)
        Close();
    int flags;
    if (mode == Mode_Read)
        flags = 33;
    else
        flags = (mode == Mode_Update) ? 50 : 42;
    if (m_file.Open(p, size, flags, len) < 0)
        return -1;
    m_mode = mode;
    m_open = 1;
    return PostOpen();
}

int VIObjFile::Open(int nSegms, VIFile::MemSegm *segms, long len, FileMode mode)
{
    if (m_open)
        Close();
    int flags;
    if (mode == Mode_Read)
        flags = 33;
    else
        flags = (mode == Mode_Update) ? 50 : 42;
    if (m_file.Open(nSegms, segms, flags, len) < 0)
        return -1;
    m_mode = mode;
    m_open = 1;
    return PostOpen();
}

int VIObjFile::PostOpen()
{
    if (m_mode == Mode_Create)
    {
        m_hdr.m_magic = 0x4F424A46;
        m_hdr.m_numObjects = 0;
        m_hdr.m_fileType = 0;
        m_hdr.m_pad = 0;
        m_hdr.m_headerSize = 32;
        m_hdr.m_dirOffset = -1;
        WriteFileHeader(0);
    }
    else
    {
        m_file.Seek(0, VIFile::Seek_End);
        if (m_file.Tell() >= 32)
        {
            ReadFileHeader(0);
            if (m_hdr.m_magic == 0x4F424A46)
            {
                if (m_mode != Mode_Update)
                {
                    if (m_file.Tell() != m_hdr.m_headerSize)
                        m_file.Seek(m_hdr.m_headerSize, VIFile::Seek_Set);
                }
                else
                    m_file.Seek(0, VIFile::Seek_End);
            }
            else
            {
                AbortOpen();
                return -2;
            }
        }
        else
        {
            AbortOpen();
            return -1;
        }
    }
    if (m_mode == Mode_Read)
    {
        m_write = 0;
        m_read = 1;
    }
    else
    {
        m_read = 0;
        m_write = 1;
    }
    m_canWrite = 0;
    m_dirty = 0;
    m_canRead = 0;
    return 0;
}

int VIObjFile::OpenObject(const void *p, unsigned int size)
{
    if (m_open)
        Close();
    if (m_file.Open(p, size, 33, size) < 0)
        return -1;
    m_hdr.m_magic = 0x4F424A46;
    m_hdr.m_headerSize = 32;
    m_hdr.m_dirOffset = -1;
    m_open = 1;
    m_mode = Mode_Read;
    m_hdr.m_numObjects = 1;
    m_hdr.m_fileType = 0;
    m_hdr.m_pad = 0;
    m_write = 0;
    m_read = 1;
    m_canRead = 0;
    m_canWrite = 0;
    m_dirty = 0;
    return 0;
}

int VIObjFile::AbortOpen()
{
    m_open = 0;
    m_hdr.m_magic = 0;
    m_hdr.m_numObjects = 0;
    m_hdr.m_fileType = 0;
    m_hdr.m_headerSize = 0;
    m_hdr.m_dirOffset = -1;
    m_file.Close();
    return 0;
}

int VIObjFile::Close()
{
    if (m_open)
    {
        if (m_writeOpen)
        {
            while (m_level != -1)
                WriteEnd();
        }
        Flush();
        m_file.Close();
        ResetState();
        return 0;
    }
    return -1;
}

int VIObjFile::Flush()
{
    if (!m_open)
        return -1;
    if (m_writeOpen)
        return -1;
    if (m_dirty)
        WriteFileHeader(0);
    m_file.Flush();
    return 0;
}

int VIObjFile::NumObjects()
{
    return m_hdr.m_numObjects;
}

unsigned int VIObjFile::FileType()
{
    return m_hdr.m_fileType;
}

int VIObjFile::FileType(unsigned int t)
{
    m_hdr.m_fileType = t;
    return 0;
}

int VIObjFile::Append(const char *path)
{
    VIObjFile f;
    unsigned long offset;
    unsigned int size;

    if (!m_open)
        return -1;
    if (m_mode == Mode_Read)
        return -1;
    if (f.Open(path, Mode_Read) < 0)
        return -1;
    for (int i = 0; i < f.NumObjects(); i++)
        if (Copy(f, offset, size) < 0)
            return -1;
    if (f.Close() < 0)
        return -1;
    return 0;
}

int VIObjFile::NumSubObjects()
{
    if (m_level < 0)
        return -1;
    return m_dirs[m_level].m_obj.m_numSubObjects;
}

unsigned int VIObjFile::ObjectVersion()
{
    if (m_level < 0)
        return 0;
    return m_dirs[m_level].m_obj.m_version;
}

unsigned int VIObjFile::ObjectType()
{
    if (m_level < 0)
        return 0;
    return m_dirs[m_level].m_obj.m_type;
}

unsigned int VIObjFile::ObjectSize()
{
    if (m_level < 0)
        return 0;
    return m_dirs[m_level].m_obj.m_size;
}

int VIObjFile::WriteFileHeader(int restore)
{
    long pos = m_file.Tell();
    if (pos)
        m_file.Seek(0, VIFile::Seek_Set);
    int endian = VIEndian;
    if (endian != 1)
    {
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_magic);
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_numObjects);
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_fileType);
        VIBufferSwap(8, (unsigned char *)&m_hdr.m_headerSize);
        VIBufferSwap(8, (unsigned char *)&m_hdr.m_dirOffset);
    }
    m_file.Write(&m_hdr, 32);
    if (endian != 1)
    {
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_magic);
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_numObjects);
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_fileType);
        VIBufferSwap(8, (unsigned char *)&m_hdr.m_headerSize);
        VIBufferSwap(8, (unsigned char *)&m_hdr.m_dirOffset);
    }
    if (restore)
        m_file.Seek(pos, VIFile::Seek_Set);
    return 0;
}

int VIObjFile::ReadFileHeader(int restore)
{
    long pos = m_file.Tell();
    if (pos)
        m_file.Seek(0, VIFile::Seek_Set);
    m_file.Read(&m_hdr, 32);
    if (VIEndian != 1)
    {
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_magic);
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_numObjects);
        VIBufferSwap(4, (unsigned char *)&m_hdr.m_fileType);
        VIBufferSwap(8, (unsigned char *)&m_hdr.m_headerSize);
        VIBufferSwap(8, (unsigned char *)&m_hdr.m_dirOffset);
    }
    if (restore)
        m_file.Seek(pos, VIFile::Seek_Set);
    return 0;
}

int VIObjFile::WriteBegin(unsigned short type, unsigned short version)
{
    unsigned long offset;

    return WriteBegin(type, version, offset) > -1 ? 0 : -1;
}

int VIObjFile::WriteBegin(unsigned short type, unsigned short version, unsigned long &offset)
{
    if (m_write && !m_readOpen && m_level != 9)
    {
        if (m_level != -1 && m_dirs[m_level].m_obj.m_size != 0
            && m_dirs[m_level].m_obj.m_numSubObjects == 0)
            return -1;
        if (m_mode != Mode_Update && m_mode != Mode_Create)
            m_file.Seek(0, VIFile::Seek_End);
        m_level++;
        m_dirs[m_level].m_obj.m_type = type;
        m_dirs[m_level].m_obj.m_version = version;
        m_dirs[m_level].m_obj.m_size = 0;
        m_dirs[m_level].m_obj.m_numSubObjects = 0;
        m_dirs[m_level].m_offset = m_file.Tell();
        int endian = VIEndian;
        offset = m_dirs[m_level].m_offset;
        if (endian != 1)
        {
            VIBufferSwap(2, (unsigned char *)&m_dirs[m_level].m_obj.m_type);
            VIBufferSwap(2, (unsigned char *)&m_dirs[m_level].m_obj.m_version);
            VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_size);
            VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_numSubObjects);
        }
        m_file.Write(&m_dirs[m_level].m_obj, 12);
        if (endian != 1)
        {
            VIBufferSwap(2, (unsigned char *)&m_dirs[m_level].m_obj.m_type);
            VIBufferSwap(2, (unsigned char *)&m_dirs[m_level].m_obj.m_version);
            VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_size);
            VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_numSubObjects);
        }
        m_canWrite = 1;
        if (m_level == 0)
            m_dirty = 1;
        m_writeOpen = 1;
        return 0;
    }
    return -1;
}

int VIObjFile::WriteDirBegin(unsigned short type, unsigned short version, unsigned long &offset)
{
    if (WriteBegin(type, version, offset) >= 0)
    {
        m_hdr.m_dirOffset = offset;
        m_dirty = 1;
        return 0;
    }
    return -1;
}

int VIObjFile::WriteEnd()
{
    unsigned int size;

    return WriteEnd(size);
}

int VIObjFile::WriteEnd(unsigned int &size)
{
    if (m_writeOpen)
    {
        long pos = m_file.Tell();
        m_file.Seek(m_dirs[m_level].m_offset + 4, VIFile::Seek_Set);
        int endian = VIEndian;
        size = m_dirs[m_level].m_obj.m_size + 12;
        if (endian != 1)
        {
            VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_size);
            VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_numSubObjects);
        }
        m_file.Write(&m_dirs[m_level].m_obj.m_size, 8);
        if (endian != 1)
        {
            VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_size);
            VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_numSubObjects);
        }
        m_file.Seek(pos, VIFile::Seek_Set);
        m_level--;
        if (m_level == -1)
        {
            m_writeOpen = 0;
            m_hdr.m_numObjects++;
        }
        else
        {
            m_dirs[m_level].m_obj.m_size += m_dirs[m_level + 1].m_obj.m_size;
            m_dirs[m_level].m_obj.m_size += 12;
            m_dirs[m_level].m_obj.m_numSubObjects++;
        }
        m_canWrite = 0;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(char v)
{
    if (m_canWrite)
    {
        m_file.Write(&v, 1);
        m_dirs[m_level].m_obj.m_size += 1;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(signed char v)
{
    if (m_canWrite)
    {
        m_file.Write(&v, 1);
        m_dirs[m_level].m_obj.m_size += 1;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(unsigned char v)
{
    if (m_canWrite)
    {
        m_file.Write(&v, 1);
        m_dirs[m_level].m_obj.m_size += 1;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(short v)
{
    if (m_canWrite)
    {
        if (VIEndian != 1)
            VIBufferSwap(2, (unsigned char *)&v);
        m_file.Write(&v, 2);
        m_dirs[m_level].m_obj.m_size += 2;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(unsigned short v)
{
    if (m_canWrite)
    {
        if (VIEndian != 1)
            VIBufferSwap(2, (unsigned char *)&v);
        m_file.Write(&v, 2);
        m_dirs[m_level].m_obj.m_size += 2;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(int v)
{
    if (m_canWrite)
    {
        if (VIEndian != 1)
            VIBufferSwap(4, (unsigned char *)&v);
        m_file.Write(&v, 4);
        m_dirs[m_level].m_obj.m_size += 4;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(unsigned int v)
{
    if (m_canWrite)
    {
        if (VIEndian != 1)
            VIBufferSwap(4, (unsigned char *)&v);
        m_file.Write(&v, 4);
        m_dirs[m_level].m_obj.m_size += 4;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(long v)
{
    if (m_canWrite)
    {
        if (VIEndian != 1)
            VIBufferSwap(8, (unsigned char *)&v);
        m_file.Write(&v, 8);
        m_dirs[m_level].m_obj.m_size += 8;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(unsigned long v)
{
    if (m_canWrite)
    {
        if (VIEndian != 1)
            VIBufferSwap(8, (unsigned char *)&v);
        m_file.Write(&v, 8);
        m_dirs[m_level].m_obj.m_size += 8;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(float v)
{
    if (m_canWrite)
    {
        if (VIEndian != 1)
            VIBufferSwap(4, (unsigned char *)&v);
        m_file.Write(&v, 4);
        m_dirs[m_level].m_obj.m_size += 4;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(double v)
{
    if (m_canWrite)
    {
        if (VIEndian != 1)
            VIBufferSwap(8, (unsigned char *)&v);
        m_file.Write(&v, 8);
        m_dirs[m_level].m_obj.m_size += 8;
        return 0;
    }
    return -1;
}

int VIObjFile::Write(const unsigned char *p, int n)
{
    if (m_canWrite)
    {
        m_file.Write(p, n);
        m_dirs[m_level].m_obj.m_size += n;
        return 0;
    }
    return -1;
}

int VIObjFile::WriteString(const char *s)
{
    if (m_canWrite)
    {
        unsigned short len = strlen(s);
        if (VIEndian != 1)
            VIBufferSwap(2, (unsigned char *)&len);
        m_file.Write(&len, 2);
        m_dirs[m_level].m_obj.m_size += 2;
        m_file.Write(s, len);
        m_dirs[m_level].m_obj.m_size += len;
        return 0;
    }
    return -1;
}

int VIObjFile::ReadBegin(unsigned short &type, unsigned short &version)
{
    unsigned long offset;
    unsigned int size;

    return ReadBegin(type, version, offset, size);
}

int VIObjFile::ReadBegin(unsigned short &type, unsigned short &version,
                         unsigned long &offset, unsigned int &size)
{
    if (!m_read)
        return -1;
    if (m_writeOpen)
        return -1;
    if (m_level == 9)
        return -1;
    if (m_file.Eof())
        return -1;
    if (m_level >= 0)
    {
        if (m_dirs[m_level].m_obj.m_numSubObjects == 0)
            return -1;
        if (m_dirs[m_level].m_remain == 0)
            return -1;
    }
    m_dirs[m_level + 1].m_offset = m_file.Tell();
    offset = m_dirs[m_level + 1].m_offset;
    if (m_file.Read(&m_dirs[m_level + 1].m_obj, 12) < 0)
        return -1;
    m_level++;
    if (VIEndian != 1)
    {
        VIBufferSwap(2, (unsigned char *)&m_dirs[m_level].m_obj.m_type);
        VIBufferSwap(2, (unsigned char *)&m_dirs[m_level].m_obj.m_version);
        VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_size);
        VIBufferSwap(4, (unsigned char *)&m_dirs[m_level].m_obj.m_numSubObjects);
    }
    size = m_dirs[m_level].m_obj.m_size + 12;
    m_dirs[m_level].m_remain = m_dirs[m_level].m_obj.m_size;
    if (m_file.Eof())
        return -1;
    m_readOpen = 1;
    type = m_dirs[m_level].m_obj.m_type;
    version = m_dirs[m_level].m_obj.m_version;
    if (m_dirs[m_level].m_obj.m_numSubObjects != 0)
        m_canRead = 0;
    else
        m_canRead = 1;
    return 0;
}

int VIObjFile::ReadBegin(unsigned long offset, unsigned short &type, unsigned short &version)
{
    if (m_read && !m_writeOpen && !m_readOpen)
    {
        m_level = -1;
        m_file.Seek(offset, VIFile::Seek_Set);
        if (ReadBegin(type, version) >= 0)
            return 0;
    }
    return -1;
}

int VIObjFile::ReadDirBegin(unsigned short &type, unsigned short &version)
{
    if (m_read && !m_writeOpen && !m_readOpen && m_hdr.m_dirOffset != -1)
    {
        if (ReadBegin(m_hdr.m_dirOffset, type, version) >= 0)
            return 0;
    }
    return -1;
}

int VIObjFile::ReadEnd()
{
    if (m_readOpen)
    {
        if (m_dirs[m_level].m_remain > 0)
        {
            m_file.Seek(m_dirs[m_level].m_remain, VIFile::Seek_Cur);
            m_dirs[m_level].m_remain = 0;
        }
        m_level--;
        if (m_level < 0)
            m_readOpen = 0;
        else
        {
            m_dirs[m_level].m_remain -= m_dirs[m_level + 1].m_obj.m_size;
            m_dirs[m_level].m_remain -= 12;
        }
        m_canRead = 0;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(char &v)
{
    if (m_canRead && m_dirs[m_level].m_remain > 0)
    {
        m_file.Read(&v, 1);
        m_dirs[m_level].m_remain -= 1;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(signed char &v)
{
    if (m_canRead && m_dirs[m_level].m_remain > 0)
    {
        m_file.Read(&v, 1);
        m_dirs[m_level].m_remain -= 1;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(unsigned char &v)
{
    if (m_canRead && m_dirs[m_level].m_remain > 0)
    {
        m_file.Read(&v, 1);
        m_dirs[m_level].m_remain -= 1;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(short &v)
{
    if (m_canRead && m_dirs[m_level].m_remain >= 2)
    {
        m_file.Read(&v, 2);
        if (VIEndian != 1)
            VIBufferSwap(2, (unsigned char *)&v);
        m_dirs[m_level].m_remain -= 2;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(unsigned short &v)
{
    if (m_canRead && m_dirs[m_level].m_remain >= 2)
    {
        m_file.Read(&v, 2);
        if (VIEndian != 1)
            VIBufferSwap(2, (unsigned char *)&v);
        m_dirs[m_level].m_remain -= 2;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(int &v)
{
    if (m_canRead && m_dirs[m_level].m_remain >= 4)
    {
        m_file.Read(&v, 4);
        if (VIEndian != 1)
            VIBufferSwap(4, (unsigned char *)&v);
        m_dirs[m_level].m_remain -= 4;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(unsigned int &v)
{
    if (m_canRead && m_dirs[m_level].m_remain >= 4)
    {
        m_file.Read(&v, 4);
        if (VIEndian != 1)
            VIBufferSwap(4, (unsigned char *)&v);
        m_dirs[m_level].m_remain -= 4;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(long &v)
{
    if (m_canRead && m_dirs[m_level].m_remain >= 8)
    {
        m_file.Read(&v, 8);
        if (VIEndian != 1)
            VIBufferSwap(8, (unsigned char *)&v);
        m_dirs[m_level].m_remain -= 8;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(unsigned long &v)
{
    if (m_canRead && m_dirs[m_level].m_remain >= 8)
    {
        m_file.Read(&v, 8);
        if (VIEndian != 1)
            VIBufferSwap(8, (unsigned char *)&v);
        m_dirs[m_level].m_remain -= 8;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(float &v)
{
    if (m_canRead && m_dirs[m_level].m_remain >= 4)
    {
        m_file.Read(&v, 4);
        if (VIEndian != 1)
            VIBufferSwap(4, (unsigned char *)&v);
        m_dirs[m_level].m_remain -= 4;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(double &v)
{
    if (m_canRead && m_dirs[m_level].m_remain >= 8)
    {
        m_file.Read(&v, 8);
        if (VIEndian != 1)
            VIBufferSwap(8, (unsigned char *)&v);
        m_dirs[m_level].m_remain -= 8;
        return 0;
    }
    return -1;
}

int VIObjFile::Read(unsigned char *p, int n)
{
    if (m_canRead && m_dirs[m_level].m_remain >= n)
    {
        m_file.Read(p, n);
        m_dirs[m_level].m_remain -= n;
        return 0;
    }
    return -1;
}

int VIObjFile::ReadString(char *dst, unsigned short max)
{
    unsigned short len;

    if (ReadStringLength(len) < 0)
        return -1;
    if (ReadStringBody(dst, len, max) < 0)
        return -1;
    return 0;
}

int VIObjFile::ReadStringLength(unsigned short &len)
{
    return Read(len);
}

int VIObjFile::ReadStringBody(char *dst, unsigned short len)
{
    if (m_canRead && m_dirs[m_level].m_remain >= len)
    {
        m_file.Read(dst, len);
        dst[len] = 0;
        m_dirs[m_level].m_remain -= len;
        return 0;
    }
    return -1;
}

int VIObjFile::ReadStringBody(char *dst, unsigned short len, unsigned short max)
{
    if (m_canRead && m_dirs[m_level].m_remain >= len)
    {
        if (len > max - 1)
        {
            m_file.Read(dst, max - 1);
            int skip = len + 1;
            m_file.Seek(skip - max, VIFile::Seek_Cur);
            dst[max - 1] = 0;
        }
        else
        {
            m_file.Read(dst, len);
            dst[len] = 0;
        }
        m_dirs[m_level].m_remain -= len;
        return 0;
    }
    return -1;
}

int VIObjFile::Copy(VIObjFile &src, unsigned long &offset, unsigned int &size)
{
    Obj obj;

    if (!m_write || m_readOpen || !src.m_read || src.m_writeOpen)
        return -1;
    if (m_level != -1 && m_dirs[m_level].m_obj.m_size != 0
        && m_dirs[m_level].m_obj.m_numSubObjects == 0)
        return -1;
    char *buf = new char[4096];
    if (!buf)
        return -1;
    if (m_mode != Mode_Update && m_mode != Mode_Create)
        m_file.Seek(0, VIFile::Seek_End);
    offset = m_file.Tell();
    if (src.m_file.Read(&obj, 12) < 0)
        return -1;
    m_file.Write(&obj, 12);
    if (m_level == -1)
        m_dirty = 1;
    if (VIEndian != 1)
    {
        VIBufferSwap(2, (unsigned char *)&obj.m_type);
        VIBufferSwap(2, (unsigned char *)&obj.m_version);
        VIBufferSwap(4, (unsigned char *)&obj.m_size);
        VIBufferSwap(4, (unsigned char *)&obj.m_numSubObjects);
    }
    unsigned long n = obj.m_size;
    size = obj.m_size + 12;
    while (n >= 4096)
    {
        src.m_file.Read(buf, 4096);
        m_file.Write(buf, 4096);
        n -= 4096;
    }
    if (n)
    {
        src.m_file.Read(buf, n);
        m_file.Write(buf, n);
    }
    if (src.m_level >= 0)
    {
        src.m_dirs[src.m_level].m_remain -= obj.m_size;
        src.m_dirs[src.m_level].m_remain -= 12;
    }
    if (m_level == -1)
        m_hdr.m_numObjects++;
    else
    {
        m_dirs[m_level].m_obj.m_size += obj.m_size;
        m_dirs[m_level].m_obj.m_size += 12;
        m_dirs[m_level].m_obj.m_numSubObjects++;
    }
    if (buf)
        delete [] (char *)buf;
    return 0;
}

int VIObjFile::Copy(VIObjFile &src, unsigned long srcOffset, unsigned long &offset,
                    unsigned int &size)
{
    Obj obj;

    if (!m_write || m_readOpen || !src.m_read || src.m_writeOpen)
        return -1;
    if (m_level != -1 && m_dirs[m_level].m_obj.m_size != 0
        && m_dirs[m_level].m_obj.m_numSubObjects == 0)
        return -1;
    char *buf = new char[4096];
    if (!buf)
        return -1;
    src.m_level = -1;
    if (src.m_file.Seek(srcOffset, VIFile::Seek_Set) < 0)
        return -1;
    if (m_mode != Mode_Update && m_mode != Mode_Create)
        m_file.Seek(0, VIFile::Seek_End);
    offset = m_file.Tell();
    if (src.m_file.Read(&obj, 12) < 0)
        return -1;
    m_file.Write(&obj, 12);
    if (m_level == -1)
        m_dirty = 1;
    if (VIEndian != 1)
    {
        VIBufferSwap(2, (unsigned char *)&obj.m_type);
        VIBufferSwap(2, (unsigned char *)&obj.m_version);
        VIBufferSwap(4, (unsigned char *)&obj.m_size);
        VIBufferSwap(4, (unsigned char *)&obj.m_numSubObjects);
    }
    unsigned long n = obj.m_size;
    while (n >= 4096)
    {
        src.m_file.Read(buf, 4096);
        m_file.Write(buf, 4096);
        n -= 4096;
    }
    if (n)
    {
        src.m_file.Read(buf, n);
        m_file.Write(buf, n);
    }
    if (m_level == -1)
        m_hdr.m_numObjects++;
    else
    {
        m_dirs[m_level].m_obj.m_size += obj.m_size;
        m_dirs[m_level].m_obj.m_size += 12;
        m_dirs[m_level].m_obj.m_numSubObjects++;
    }
    if (buf)
        delete [] (char *)buf;
    return 0;
}
