// VILoader -- SUPPORT 0x0044FA78 .. 0x004508D8, its own TU.  The asynchronous
// resource loader.  A transaction is a byte range of one open resource file;
// Process keeps exactly one read in flight, hands the buffer to VIESFParse the
// frame after it lands, and walks on.  Everything the streaming zone loader
// pulls in goes through here.
#include "vi.h"

VILoader::VILoader()
{
    m_cur = -1;
    m_numFiles = 0;
    m_files = 0;
    m_buffer = 0;
    m_bufferBase = 0;
    m_bufSize = 0;
    m_progress = 0;
}

VILoader::~VILoader()
{
    Clear();
}

int VILoader::Init(int bufSize, int numFiles)
{
    if (Clear() < 0) return -1;
    if (m_trans.Init(100) < 0) return -1;
    m_cur = m_trans.End();
    m_numFiles = numFiles;
    m_bufSize = bufSize;
    if (Allocate() < 0) return -1;
    m_esf.SetPermResource(0);
    return 0;
}

int VILoader::Clear()
{
    for (int i = 0; i < m_numFiles; i++)
        if (m_files[i].m_file.Close() < 0) return -1;
    if (m_files)
    {
        delete [] m_files;
        m_numFiles = 0;
        m_files = 0;
    }
    if (m_trans.Clear() < 0) return -1;
    if (m_map.Clear() < 0) return -1;
    if (m_bufferBase)
    {
        delete [] m_bufferBase;
        m_buffer = 0;
        m_bufferBase = 0;
    }
    m_bufSize = 0;
    m_cur = -1;
    m_progress = 0;
    return 0;
}

int VILoader::Open(int i, const char *path)
{
    if (m_files[i].m_file.Open(path, 97) < 0) return -1;
    return 0;
}

int VILoader::OpenResourceFile(int i, const char *path, VIScene &sc)
{
    VIESFParse p;

    if (p.ParseResourceFile(path, 1, sc, m_files[i].m_dir) < 0) return -1;
    if (m_files[i].m_file.Open(path, 97) < 0) return -1;
    return 0;
}

int VILoader::Close(int i)
{
    if (m_files[i].m_file.Close() < 0) return -1;
    return 0;
}

int VILoader::Process(VIScene &sc)
{
    int f;

    if (m_cur == m_trans.End()) return 0;

    if (m_trans.Value(m_cur).m_state == 1)
    {
        f = m_trans.Value(m_cur).m_file;
        if (m_files[f].m_file.IsAsynchComplete())
        {
            m_trans.Value(m_cur).m_state = 2;
            if (Complete(m_cur, sc) < 0) return -1;
            m_cur = m_trans.Next(m_cur);
            if (m_cur == m_trans.End()) return 0;
        }
    }

    if (m_trans.Value(m_cur).m_state != 0) return 0;

    f = m_trans.Value(m_cur).m_file;
    if (m_files[f].m_file.IsAsynchComplete())
    {
        if (m_files[f].m_file.ReadAsynch(m_trans.Value(m_cur).m_position,
                                         m_buffer + m_trans.Value(m_cur).m_readOffset,
                                         m_trans.Value(m_cur).m_readSize) < 0) return -1;
        m_trans.Value(m_cur).m_state = 1;
    }
    return 0;
}

int VILoader::CompleteAll(VIScene &sc)
{
    while (m_cur != m_trans.End())
    {
        if (Process(sc) < 0) return -1;
        if (m_progress) m_progress(m_progressArg);
    }
    return 0;
}

int VILoader::AllComplete()
{
    return m_cur == m_trans.End();
}

int VILoader::ReleaseAllComplete(VIScene &sc)
{
    int i = m_trans.Begin();
    while (i != m_trans.End())
    {
        int n = m_trans.Next(i);
        if (m_trans.Value(i).m_state == 3)
            if (Release(i, sc) < 0) return -1;
        i = n;
    }
    return 0;
}

int VILoader::IsResourceAvailable(int i, unsigned int id)
{
    unsigned long off;
    unsigned int size;

    return Find(i, id, off, size) != 0;
}

int VILoader::Load(int i, unsigned int id)
{
    unsigned long off;
    unsigned int size;

    if (Find(i, id, off, size) == 0) return -1;
    return Load(i, id, off, size);
}

int VILoader::Load(int i, unsigned int id, unsigned long off, unsigned int size)
{
    return Load(i, id, off, size, -1);
}

int VILoader::Load(int i, unsigned long off, unsigned int size, int zone)
{
    return Load(i, 0, off, size, zone);
}

int VILoader::Load(int i, unsigned int id, unsigned long off, unsigned int size, int zone)
{
    VILoaderTrans t;
    int r;

    if (i < 0 || i >= m_numFiles) return -1;
    if (id)
    {
        int k = m_map.Find(id);
        if (k != m_map.End())
        {
            r = m_map.Value(k);
            m_trans.Value(r).m_refCount++;
            return r;
        }
    }
    if (size > m_bufSize + m_files[i].m_file.AsynchAlignment() * 2) return -1;

    t.m_state = 0;
    t.m_refCount = 1;
    t.m_id = id;
    t.m_file = i;
    t.m_position = off;
    t.m_size = size;
    t.m_offset = 0;
    t.m_zone = zone;
    Align(t);
    r = m_trans.Add(t);
    if (r < 0) return -1;
    if (m_cur == m_trans.End()) m_cur = r;
    if (id)
        if (m_map.Insert(id, r) < 0) return -1;
    return r;
}

int VILoader::IsComplete(int i)
{
    return m_trans.Value(i).m_state == 3;
}

int VILoader::Release(int i, VIScene &sc)
{
    m_trans.Value(i).m_refCount--;
    if (m_trans.Value(i).m_refCount == 0)
    {
        if (m_trans.Value(i).m_id)
        {
            int k = m_map.Find(m_trans.Value(i).m_id);
            if (k != m_map.End())
                if (m_map.Erase(k) < 0) return -1;
        }
        if (m_trans.Value(i).m_state == 3 && m_trans.Value(i).m_id)
            if (sc.ReleaseResource(m_trans.Value(i).m_id) < 0) return -1;
        if (m_cur == i) m_cur = m_trans.Next(i);
        if (m_trans.Erase(i) < 0) return -1;
    }
    return 0;
}

void VILoader::SetProgressCallback(void (*fn)(void *), void *arg)
{
    m_esf.SetProgressCallback(fn, arg);
    m_progress = fn;
    m_progressArg = arg;
}

int VILoader::Find(int i, unsigned int id, unsigned long &off, unsigned int &size)
{
    VIResourceDir &d = m_files[i].m_dir;
    int lo = 0;
    int hi = d.m_size - 1;
    int mid;

    while (lo <= hi)
    {
        mid = (lo + hi) >> 1;
        if (d.m_data[mid].m_id == id) break;
        if (id < d.m_data[mid].m_id) hi = mid - 1;
        else lo = mid + 1;
    }
    if (lo <= hi)
    {
        off = d.m_data[mid].m_offset;
        size = d.m_data[mid].m_size;
        return 1;
    }
    return 0;
}

int VILoader::Complete(int i, VIScene &sc)
{
    if (m_trans.Value(i).m_state != 2) return -1;
    if (m_objFile.OpenObject(m_buffer + m_trans.Value(i).m_offset,
                             m_trans.Value(i).m_size) < 0) return -1;
    m_objFile.FileType(0xAB4F);
    if (m_trans.Value(i).m_zone != -1)
    {
        if (m_esf.ParseZoneResource(m_objFile, m_trans.Value(i).m_zone, sc) < 0) return -1;
    }
    else
    {
        if (m_esf.Parse(m_objFile, sc) < 0) return -1;
    }
    m_trans.Value(i).m_state = 3;
    if (m_trans.Value(i).m_id)
        if (sc.ShareResource(m_trans.Value(i).m_id) < 0) return -1;
    if (m_objFile.Close() < 0) return -1;
    return 0;
}

int VILoader::Allocate()
{
    m_bufferBase = new char[m_bufSize + 64];
    if (m_bufferBase == 0) return -1;
    m_buffer = (char *)(((unsigned int)m_bufferBase + 64) & ~63u);
    m_files = new VIResourceFile[m_numFiles];
    if (m_files == 0) return -1;
    return 0;
}

int VILoader::Align(VILoaderTrans &t)
{
    t.m_readSize = t.m_size;
    t.m_readOffset = t.m_offset;

    int a = m_files[t.m_file].m_file.AsynchAlignment();
    unsigned int r = t.m_position % a;
    t.m_position -= r;
    t.m_offset += r;
    t.m_readSize += r;
    unsigned int m = t.m_readSize % a;
    if (m) m = a - m;
    t.m_readSize += m;
    return 0;
}
