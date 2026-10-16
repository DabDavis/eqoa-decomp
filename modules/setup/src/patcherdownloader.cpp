// PatcherDownloader.cpp (name ours) -- SETUP 0x00588270 .. 0x00589078: the base
// of the downloader tree.  Each node owns its next sibling and its first child.
#include "patcher.h"

// 0x00588270.
PatcherDownloader::PatcherDownloader()
{
    m_info = 0;
    m_isDistribution = 0;
    m_server = 0;
    m_port = 0;
    m_name = 0;
    m_localPath = 0;
    m_remotePath = 0;
    m_defaultRoot = 0;
    m_root = 0;
    m_xtraFile = 0;
    m_delete = 0;
    m_needDownload = 0;
    for (int i = 1; i >= 0; i--)
        m_special[i] = 0;
    m_useCRC = 2;
    m_ignoreTimeStamp = 1;
    m_api = 0;
    m_parent = 0;
    m_child = 0;
    m_prev = 0;
    m_next = 0;
    m_installOnly = 0;
    m_dirAvailable = 0;
    m_topLevel = 0;
    m_scanAsTest = 0;
}

// 0x00588310.
PatcherDownloader::~PatcherDownloader()
{
    if (m_server)
    {
        free(m_server);
        m_server = 0;
    }
    if (m_name)
    {
        free(m_name);
        m_name = 0;
    }
    if (m_localPath)
    {
        free(m_localPath);
        m_localPath = 0;
    }
    if (m_remotePath)
    {
        free(m_remotePath);
        m_remotePath = 0;
    }
    if (m_defaultRoot)
    {
        free(m_defaultRoot);
        m_defaultRoot = 0;
    }
    if (m_root)
    {
        free(m_root);
        m_root = 0;
    }
    if (m_next)
    {
        delete m_next;
        m_next = 0;
    }
    if (m_child)
    {
        delete m_child;
        m_child = 0;
    }
}

// 0x00588430.  A node inherits Delete, IgnoreTimeStamp and InstallOnly from its
// parent; a test, beta or xtra file is always checked by CRC.
int PatcherDownloader::Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                                  const char *root, const char *path)
{
    SetParent(parent);
    SetPrevious(prev);
    if (m_name)
    {
        free(m_name);
        m_name = 0;
    }
    if (m_localPath)
    {
        free(m_localPath);
        m_localPath = 0;
    }
    if (m_remotePath)
    {
        free(m_remotePath);
        m_remotePath = 0;
    }
    if (m_server)
    {
        free(m_server);
        m_server = 0;
    }
    m_name = PullXMLAttribute(node, "Name");
    m_localPath = PullXMLAttribute(node, "LocalPath");
    m_remotePath = PullXMLAttribute(node, "RemotePath");
    m_server = PullXMLAttribute(node, "Server");
    m_port = PullXMLAttributeLong(node, "Port");
    m_delete = PullXMLAttributeBool(node, "Delete");
    m_ignoreTimeStamp = PullXMLAttributeBool(node, "IgnoreTimeStamp", 0);
    m_installOnly = PullXMLAttributeBool(node, "InstallOnly", 0);
    m_special[0] = PullXMLAttributeBool(node, "TestFile", 0);
    m_special[1] = PullXMLAttributeBool(node, "BetaFile", 0);
    m_xtraFile = PullXMLAttributeBool(node, "XtraFile", 0);
    if (m_special[0] || m_special[1] || m_xtraFile)
        m_useCRC = 1;
    else
        m_useCRC = PullDefinedXMLAttBool(node, "UseCRC");
    if (GetParent())
    {
        if (GetParent()->NeedDelete())
            m_delete = 1;
        if (GetParent()->IgnoringTimeStamp())
            m_ignoreTimeStamp = 1;
        if (GetParent()->IsInstallOnly())
            m_installOnly = 1;
    }
    if (m_delete)
        m_needDownload = 0;
    return 1;
}

// 0x00588678.
int PatcherDownloader::Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                                  const char *root, const char *path, PatcherDistributionInfo *info)
{
    m_info = info;
    if (info)
        m_api = info->GetPatcher();
    return Initialize(node, parent, prev, root, path);
}

// 0x00588718.
int PatcherDownloader::UseCRC(bool on)
{
    if (on)
        m_useCRC = 1;
    else
        m_useCRC = 0;
    int ok = 1;
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        if (!c->UseCRC(on))
            ok = 0;
    return ok;
}

// 0x005887A8.
int PatcherDownloader::IgnoreTimeStamp(bool on)
{
    m_ignoreTimeStamp = on;
    return 1;
}

// 0x005887B8.
int PatcherDownloader::IgnoringTimeStamp()
{
    return m_ignoreTimeStamp;
}

// 0x005887C0.
int PatcherDownloader::IsInstallOnly()
{
    return m_installOnly;
}

// 0x005887C8.  A quick scan never reads a CRC.
int PatcherDownloader::UsingCRC()
{
    if (m_api->QuickScan())
        return 0;
    if (m_useCRC == 1)
        return 1;
    if (m_useCRC == 2 && GetParent() && GetParent()->UsingCRC())
        return 1;
    return 0;
}

// 0x00588850.
int PatcherDownloader::CalculateHierarchicalProperties()
{
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
    {
        c->CalculateHierarchicalProperties();
        if (c->NeedDownload(0x10))
            m_needDownload = 1;
        if (c->ScanAsTest())
            m_scanAsTest = 1;
    }
    if (NeedDelete() && IsDistributionDownloader())
        m_needDownload = 0;
    return 1;
}

// 0x00588908.
int PatcherDownloader::FlagDownloaded()
{
    m_needDownload = 0;
    return 1;
}

// 0x00588918.
int PatcherDownloader::FlagDeleted()
{
    m_delete = 0;
    return 1;
}

// 0x00588928.  0x10 counts everything; 4 skips test files, 8 beta files.
int PatcherDownloader::NeedDownload(int flags)
{
    if (!(flags & 0x10))
    {
        if ((flags & 4) && m_special[0])
            return 0;
        if ((flags & 8) && m_special[1])
            return 0;
    }
    return m_needDownload;
}

// 0x00588970.
int PatcherDownloader::ScanAsTest()
{
    return m_scanAsTest;
}

// 0x00588978.
int PatcherDownloader::NeedDelete()
{
    return m_delete;
}

// 0x00588980.  Tail recursion, which gcc turns into a loop.
char *PatcherDownloader::GetServer()
{
    if (!m_server && m_parent)
        return m_parent->GetServer();
    return m_server;
}

// 0x005889A8.
int PatcherDownloader::GetPort()
{
    if (!m_port && m_parent)
        return m_parent->GetPort();
    return m_port;
}

// 0x005889D0.
int PatcherDownloader::IsDistributionDownloader()
{
    return m_isDistribution;
}

// 0x005889D8.
PatcherDistributionInfo *PatcherDownloader::GetDistributionInfo()
{
    return m_info;
}

// 0x005889E0.
int PatcherDownloader::SetDistributionInfo(PatcherDistributionInfo *info)
{
    m_info = info;
    if (info)
        m_api = info->GetPatcher();
    return 1;
}

// 0x00588A20.
int PatcherDownloader::SetDistributionRoot(const char *root)
{
    if (m_root)
    {
        free(m_root);
        m_root = 0;
    }
    if (root)
        m_root = strdup(root);
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        c->SetDistributionRoot(root);
    return 0;
}

// 0x00588AB8.  0x80 rescans only what is flagged as a test.
int PatcherDownloader::ScanFiles(int flags)
{
    PatcherDownloader *c = GetChild();
    if ((flags & 0x80) && !ScanAsTest())
        return 1;
    for (; c; c = c->GetNext())
        c->ScanFiles(flags);
    return 1;
}

// 0x00588B58.
const char *PatcherDownloader::GetDistributionRoot()
{
    return m_root ? m_root : m_defaultRoot;
}

// 0x00588B70.
void PatcherDownloader::ResetScanFlag(int flags)
{
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        c->ResetScanFlag(flags);
}

// 0x00588BD8.
PatcherDownloader *PatcherDownloader::GetPrevious()
{
    return m_prev;
}

// 0x00588BE0.
PatcherDownloader *PatcherDownloader::GetNext()
{
    return m_next;
}

// 0x00588BE8.
PatcherDownloader *PatcherDownloader::GetParent()
{
    return m_parent;
}

// 0x00588BF0.
PatcherDownloader *PatcherDownloader::GetChild()
{
    return m_child;
}

// 0x00588BF8.
int PatcherDownloader::SetParent(PatcherDownloader *p)
{
    m_parent = p;
    return 1;
}

// 0x00588C08.
int PatcherDownloader::SetChild(PatcherDownloader *p)
{
    m_child = p;
    return 1;
}

// 0x00588C18.
int PatcherDownloader::SetPrevious(PatcherDownloader *p)
{
    m_prev = p;
    return 1;
}

// 0x00588C28.
int PatcherDownloader::SetNext(PatcherDownloader *p)
{
    m_next = p;
    return 1;
}

// 0x00588C38.
int PatcherDownloader::PrepForUninstall()
{
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        c->PrepForUninstall();
    m_delete = 1;
    return 1;
}

// 0x00588CA8.
int PatcherDownloader::GenerateLocalPath(const char *path)
{
    char cwd[256];
    getcwd(cwd, 255);
    int r = 0;
    if (path && (int)strlen(path) >= 2)
    {
        if (path[1] == ':')
        {
            char drive[4];
            drive[0] = path[0];
            drive[1] = ':';
            drive[2] = '\\';
            drive[3] = 0;
            chdir(drive);
            return GenerateLocalSubpath(drive, path, cwd);
        }
        r = GenerateLocalSubpath(0, path, cwd);
    }
    return r;
}

// 0x00588D60.  Make each directory of `path' past `done', one per call; a name
// with a '.' in it is the file, and ends the walk.
int PatcherDownloader::GenerateLocalSubpath(const char *done, const char *path, const char *cwd)
{
    char buf[128];
    if (done && strlen(done) == strlen(path))
        return 1;
    int i;
    if (!done)
        i = 0;
    else
        i = strlen(done) + 1;
    int found = 0;
    while (!found && i < (int)strlen(path))
    {
        if (path[i] == '\\' || path[i] == '/')
            found = 1;
        else
        {
            if (path[i] == '.')
                return 1;
            i++;
        }
    }
    sprintf(buf, path);
    buf[i] = 0;
    if ((int)strlen(buf) >= 2 && buf[1] != ':')
        chdir(cwd);
    if (!chdir(buf) || (!mkdir(buf, 0) && !chdir(buf)))
    {
        GenerateLocalSubpath(buf, path, cwd);
        return 1;
    }
    return 0;
}

// 0x00588ED0.
int PatcherDownloader::DirectoryAvailable()
{
    return m_dirAvailable;
}

// 0x00588ED8.  A distribution with a root of its own keeps it.
int PatcherDownloader::SetDirectoryAvailable(bool on)
{
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        if (!c->IsDistributionDownloader() || !c->GetDistributionRoot())
            c->SetDirectoryAvailable(on);
    return 1;
}

// 0x00588F68.
int PatcherDownloader::SetTopLevel(bool on)
{
    m_topLevel = on;
    return 1;
}

// 0x00588F78.
int PatcherDownloader::IsTopLevel()
{
    return m_topLevel;
}

// 0x00588F80.
PatcherDownloader *PatcherDownloader::GetDownloaderByFile(const char *file)
{
    if (!file)
        return 0;
    if (!IsDistributionDownloader() && m_localPath && !strcasecmp(file, m_localPath))
        return this;
    PatcherDownloader *found = 0;
    PatcherDownloader *next = GetNext();
    if (next)
        found = next->GetDownloaderByFile(file);
    if (found)
        return found;
    PatcherDownloader *child = GetChild();
    if (child)
        found = child->GetDownloaderByFile(file);
    return found;
}
