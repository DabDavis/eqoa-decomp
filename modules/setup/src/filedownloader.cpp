// FileDownloader.cpp (name ours) -- SETUP 0x005800A8 .. 0x005816A8: a <File> of
// the manifest.  On the PS2 the "disc" a file is checked against is the
// patcher's in-memory image (VIPatchImage), which the game writes to the card.
#include "patcher.h"

// Inline, and not members: an `||' of the bare fields is folded into one 64-bit
// load of +0x30, and an inline MEMBER would have been emitted out of line with
// PatcherDownloader's vtable, where SETUP has no copy.
static inline int IsTestFile(PatcherDownloader *d) { return d->m_special[0]; }
static inline int IsBetaFile(PatcherDownloader *d) { return d->m_special[1]; }
static inline int IsXtraFile(PatcherDownloader *d) { return d->m_xtraFile; }

// 0x005800A8.
FileDownloader::FileDownloader()
{
    m_downloadSize = 0;
    m_totalSize = 0;
    m_time = 0;
    m_crc = 0;
    m_keepBackup = 0;
    m_restart = 0;
    m_partialPatch = 0;
    m_localName = 0;
    m_tempName = 0;
    m_unchecked = 1;
    m_discrepancy = 0;
    m_localCRC = 0;
    m_delta = 0;
    m_deltas = 0;
    m_compressed = 1;
}

// 0x00580120.
FileDownloader::~FileDownloader()
{
    if (m_localName)
    {
        free(m_localName);
        m_localName = 0;
    }
    if (m_tempName)
    {
        free(m_tempName);
        m_tempName = 0;
    }
    if (m_deltas)
    {
        delete m_deltas;
        m_deltas = 0;
        m_delta = 0;
    }
}

// 0x005801B0.
int FileDownloader::Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                               const char *root, const char *path)
{
    PatcherDownloader::Initialize(node, parent, prev, root, path);
    SetXMLInfo(node);
    char *local = m_localPath;
    char *remote = m_remotePath;
    m_localPath = 0;
    m_remotePath = 0;
    if (m_localName)
        m_localPath = GetFullPath(m_localName, local, root);
    else
    {
        m_localPath = GetFullPath(m_name, local, root);
        m_localName = strdup(m_name);
    }
    m_remotePath = GetFullPath(m_name, remote, path);
    _xmlNode *n = node->children;
    char *deltaName = (char *)malloc(strlen(m_localPath) + 7);
    sprintf(deltaName, "%s_delta", m_localName);
    char *deltaLocal = GetFullPath(deltaName, local, root);
    for (; n; n = n->next)
    {
        if (!strcasecmp((char *)n->name, "DeltaFile"))
        {
            char *file = PullXMLAttribute(n, "FilePath");
            char *full = GetFullPath(file, remote, path);
            AddDeltaFile(n, deltaName, deltaLocal, file, full);
            free(full);
            free(file);
        }
    }
    free(deltaName);
    free(deltaLocal);
    if (local)
        free(local);
    if (remote)
        free(remote);
    if (!m_localPath)
        m_needDownload = 0;
    if (!m_api || m_api->CheckOnInitialize())
        CheckForNeedDownload();
    return 1;
}

// 0x005803E8.
int FileDownloader::InitializeFilePath()
{
    if (m_localPath)
    {
        char cwd[256];
        getcwd(cwd, 255);
        if (strcasecmp(m_localPath, "::HomeDirectory::") && strcasecmp(m_localPath, "::CurrentDirectory::")
            && strcasecmp(m_localPath, "::Desktop::") && strcasecmp(m_localPath, "::StartMenu::")
            && strcasecmp(m_localPath, "::WindowsHome::"))
            GenerateLocalPath(m_localPath);
        chdir(cwd);
    }
    return 1;
}

// 0x00580498.  A file that is the running patcher itself is patched by the
// game, not here.
int FileDownloader::CheckForNeedDownload()
{
    m_unchecked = 0;
    m_info->m_impl->IncrementScanCount();
    if (GetParent()->NeedDelete())
        return 0;
    char cwd[256];
    getcwd(cwd, 254);
    char exe[256];
    GetCurrentExecutableName(exe);
    if (m_localName)
    {
        if (!m_tempName)
        {
            m_tempName = strdup(m_localName);
            if (m_tempName)
                m_tempName[0] = '_';
        }
        int self = 0;
        if (GetCommandLine() && m_tempName)
        {
            char *cmd = strdup(GetCommandLine());
            char *temp = strdup(m_tempName);
            self = strstr(_strlwr(cmd), _strlwr(temp)) != 0;
            free(cmd);
            free(temp);
        }
        if (self)
        {
            m_needDownload = 0;
            m_delete = 0;
            m_api->PatchPatcher(m_tempName, m_localName);
            return 1;
        }
        strcasecmp(exe, m_localName);
    }
    if (NeedDelete())
        return 1;
    m_needDownload = FileDiscrepancy();
    if (m_needDownload == 1)
        m_partialPatch = PartialPatchCheck();
    return 1;
}

// 0x00580630.
int FileDownloader::SetXMLInfo(_xmlNode *node)
{
    if (m_localName)
    {
        free(m_localName);
        m_localName = 0;
    }
    if (m_tempName)
    {
        free(m_tempName);
        m_tempName = 0;
    }
    m_localName = PullXMLAttribute(node, "LocalName");
    m_totalSize = PullXMLAttributeLong(node, "TotalSize");
    m_downloadSize = PullXMLAttributeLong(node, "DownloadSize");
    char *stamp = PullXMLAttribute(node, "TimeStamp");
    m_time = ParseTime(stamp);
    if (stamp)
        free(stamp);
    m_restart = PullXMLAttributeBool(node, "Restart");
    m_keepBackup = PullXMLAttributeBool(node, "Keep_Backup");
    m_tempName = PullXMLAttribute(node, "TempFilename");
    m_crc = PullXMLAttributeLong(node, "CRC");
    if (PullXMLAttributeBool(node, "Compressed", 1))
        m_compressed = 1;
    else
        m_compressed = 0;
    if (UsingCRC() && !m_crc)
        m_useCRC = 0;
    return 1;
}

// 0x005807A0.
long FileDownloader::GetTotalSize(int flags)
{
    if (NeedDownload(flags))
    {
        if (m_delta)
            return m_delta->GetTotalSize();
        return m_totalSize;
    }
    return 0;
}

// 0x005807E0.
int FileDownloader::GetFileCount(int flags)
{
    if (!(flags & 0x10))
    {
        if ((flags & 4) && m_special[0])
            return 0;
        if ((flags & 8) && m_special[1])
            return 0;
    }
    if ((flags & 0x80) && !ScanAsTest())
        return 0;
    return 1;
}

// 0x00580860.
long FileDownloader::GetDownloadSize(int flags)
{
    if (NeedDownload(flags))
    {
        if (m_delta)
            return m_delta->GetDownloadSize();
        return m_downloadSize;
    }
    return 0;
}

// 0x005808A0.  The CRC setting holds only for this one check.
int FileDownloader::UseCRC(bool on)
{
    int saved = m_useCRC;
    if (!gPatcherCancel)
    {
        if (on)
        {
            m_useCRC = 1;
            m_needDownload = CRCDiscrepancy();
        }
        else
        {
            m_useCRC = 0;
            CheckForNeedDownload();
        }
        m_info->m_impl->IncrementScanCount();
    }
    m_useCRC = saved;
    return 1;
}

// 0x00580930.  A file that restarts the patcher is always checked by CRC.
int FileDownloader::UsingCRC()
{
    if (m_restart)
        return 1;
    return PatcherDownloader::UsingCRC();
}

// 0x00580960.  20 is a failed download, 100 a bad CRC; the file that is the
// running patcher goes down under its temporary name, and either way a
// Restart file hands the patcher a restart.
int FileDownloader::Download(int flags)
{
    char exe[256];
    char msg[512];
    printf("!!FileDownloader::Download\n");
    GetCurrentExecutableName(exe);
    if (NeedDelete())
        return 0;
    if (!NeedDownload(flags))
        return 0;
    int self = 0;
    if (m_localName)
        self = !strcasecmp(exe, m_localName);
    if (NeedsPartialPatch())
    {
        if (PartialPatchDownload(self))
            FlagDownloaded();
    }
    else
    {
        char *target = self ? m_tempName : m_localPath;
        int result = m_api->DownloadFile(GetServer(), GetPort(), m_remotePath, target, m_compressed,
                                         m_downloadSize, m_totalSize);
        printf("!!Looking for %s\n", m_localPath);
        VIPatchImage *image = m_api->FindPatchImage(m_localPath);
        if (image)
        {
            unsigned int crc = crc32(0, (unsigned char *)image->m_data, image->m_size);
            printf("Expected CRC = %i, downloaded CRC = %i\n", m_crc, crc);
            if (crc != m_crc)
                result = 100;
        }
        else
            result = 100;
        if (!result)
            FlagDownloaded();
        else
        {
            if (result != 100)
                result = 20;
            PatcherDistributionInfo *info = m_info;
            info->m_impl->CallError(result, FormatString(msg, "Download error for %s\n", target));
            return result;
        }
    }
    if (m_restart)
    {
        if (!strcasecmp(exe, m_localName))
        {
            m_info->m_impl->CallFinish();
            m_info->m_impl->CallForceRestart(m_tempName, 1);
        }
        else
        {
            m_info->m_impl->CallFinish();
            m_info->m_impl->CallForceRestart(m_localName, 0);
        }
    }
    return 0;
}

// 0x00580BF0.
int FileDownloader::Delete()
{
    char cwd[256];
    getcwd(cwd, 256);
    if (NeedDelete())
    {
        VIPatchImage *image = m_api->FindPatchImage(m_localPath);
        if (image)
            image->m_deleted = 1;
        FlagDeleted();
    }
    return 1;
}

// 0x00580C58.
int FileDownloader::FileDiscrepancy()
{
    if (UsingCRC())
        return CRCDiscrepancy();
    if (SizeDiscrepancy(0))
    {
        printf("Size discrepency on %i\n", m_localPath);
        m_discrepancy = 1;
        if (m_deltas)
            m_localCRC = Get_crc(m_localPath);
        return 1;
    }
    return 0;
}

// 0x00580CF0.  Is there a delta from the copy we have?
int FileDownloader::PartialPatchCheck()
{
    if (!m_discrepancy)
        return 0;
    if (!m_localCRC)
        return 0;
    for (FileDeltaDownloader *d = m_deltas; d; d = d->GetNext())
    {
        if (d->Matches(m_localCRC))
        {
            m_delta = d;
            return 1;
        }
    }
    return 0;
}

// 0x00580D78.
int FileDownloader::NeedsPartialPatch()
{
    return 0;
}

// 0x00580D80.  Apply the matching delta to the image in memory.
int FileDownloader::PartialPatchDownload(bool self)
{
    if (!m_delta)
        return 0;
    int size = m_delta->GetTotalSize();
    int download = m_delta->GetDownloadSize();
    if (size < download)
        size = download + 1;
    char *buf = (char *)malloc(size + 1);
    if (m_delta->Download(buf, size + 1, m_api))
    {
        buf[size] = 0;
        VIPatchImage *image = m_api->FindPatchImage(m_localPath);
        if (image)
        {
            int oldSize = image->m_size;
            char *old = image->m_data;
            int length = 0;
            char *data = UnmakeDeltaData(old, oldSize, buf, m_delta->GetTotalSize(), &length);
            free(buf);
            free(image->m_data);
            image->m_size = 0;
            image->m_data = (char *)memalign(128, length + 1);
            memset(image->m_data, 0, length + 1);
            image->m_size = length;
            memcpy(image->m_data, data, length);
            image->m_dirty = 1;
            free(data);
            return 1;
        }
    }
    free(buf);
    return 0;
}

// 0x00580ED8.
int FileDownloader::SizeDiscrepancy(void *)
{
    VIPatchImage *image = m_api->FindPatchImage(m_localPath);
    if (!image)
        return 1;
    int size = image->m_size;
    if (!m_totalSize)
        return 0;
    printf("Compare (%s) %i to %i\n", m_localPath, size, m_totalSize);
    if (size != m_totalSize)
        return 1;
    return 0;
}

// 0x00580F50.  Always 0: the PC compared the file's time with m_time, and all
// that is left of it is a read the compiler had to keep -- a VOLATILE one, put
// in the annulled delay slot of the branch and then overwritten by the 0.
long FileDownloader::TimeDiscrepancy(void *)
{
    if (UsingCRC())
        return 0;
    *(volatile long *)&m_time;
    return 0;
}

// 0x00580F90.  The 64 KB buffer is the PC's file reader, never used here.
unsigned int FileDownloader::Get_crc(char *path)
{
    char buf[0x10000];
    VIPatchImage *image = m_api->FindPatchImage(m_localPath);
    if (!image)
        return 0;
    return image->m_crc;
}

// 0x00580FD8.
int FileDownloader::CRCDiscrepancy()
{
    m_discrepancy = 1;
    m_localCRC = Get_crc(m_localPath);
    if (!m_crc || m_crc != m_localCRC)
        return 1;
    return 0;
}

// 0x00581020.  "year:month:day:hour:minute:second".
long FileDownloader::ParseTime(char *text)
{
    if (!text)
        return 0;
    struct tm t;
    char buf[16];
    int n = 0;
    t.tm_isdst = -1;
    t.tm_year = -1;
    t.tm_mon = -1;
    t.tm_mday = -1;
    t.tm_hour = -1;
    t.tm_min = -1;
    t.tm_sec = -1;
    int i;
    char *p;
    for (i = 0, p = text; i < (int)strlen(text); p++, i++)
    {
        switch (*p)
        {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            buf[n++] = *p;
            break;
        case ':':
            {
                buf[n] = 0;
                if (t.tm_year == -1)
                    t.tm_year = atoi(buf) - 1900;
                else if (t.tm_mon == -1)
                    t.tm_mon = atoi(buf) - 1;
                else if (t.tm_mday == -1)
                    t.tm_mday = atoi(buf);
                else if (t.tm_hour == -1)
                    t.tm_hour = atoi(buf);
                else if (t.tm_min == -1)
                    t.tm_min = atoi(buf);
                else if (t.tm_sec == -1)
                    t.tm_sec = atoi(buf);
                n = 0;
            }
            break;
        }
    }
    if (t.tm_sec == -1 && n > 0)
    {
        buf[n] = 0;
        t.tm_sec = atoi(buf);
    }
    return mktime(&t);
}

// 0x005811C0.
char *FileDownloader::GetFullPath(const char *name, const char *path, const char *root)
{
    char *full = 0;
    char *special = 0;
    bool haveName = name != 0;
    bool haveRoot = root != 0;
    bool havePath = path != 0;
    if (havePath)
    {
        if (!strcasecmp(path, "::HomeDirectory::"))
        {
            havePath = false;
            haveRoot = false;
        }
        else if (!strcasecmp(path, "::WindowsHome::"))
        {
            havePath = false;
            haveRoot = false;
            special = strdup(getenv("windir"));
        }
        else if (!strcasecmp(path, "::Desktop::"))
        {
            havePath = false;
            haveRoot = false;
        }
        else if (!strcasecmp(path, "::StartMenu::"))
        {
            havePath = false;
            haveRoot = false;
        }
    }
    if (havePath)
    {
        if (haveName)
        {
            full = (char *)malloc(strlen(path) + strlen(name) + 2);
            sprintf(full, "%s/%s", path, name);
        }
        else
            full = strdup(path);
    }
    else if (special)
    {
        if (!haveName)
            return 0;
        special = (char *)malloc(strlen(special) + strlen(name) + 2);
        sprintf(full, "%s/%s", special, name);
        free(special);
        special = 0;
    }
    else if (haveRoot)
    {
        if (haveName)
        {
            full = (char *)malloc(strlen(root) + strlen(name) + 2);
            sprintf(full, "%s/%s", root, name);
        }
        else
            full = strdup(root);
    }
    else if (haveName)
        full = strdup(name);
    if (special)
        free(special);
    return full;
}

// 0x005813D8.
int FileDownloader::SetDistributionRoot(const char *root)
{
    PatcherDownloader::SetDistributionRoot(root);
    CheckForNeedDownload();
    return 1;
}

// 0x00581418.
int FileDownloader::ScanFiles(int flags)
{
    if (!gPatcherCancel)
    {
        if (flags & 0x80)
        {
            if (!ScanAsTest())
                return 1;
        }
        CheckForNeedDownload();
    }
    return 1;
}

// 0x00581480.
int FileDownloader::AddDeltaFile(_xmlNode *node, const char *deltaName, const char *deltaLocal,
                                 const char *filePath, const char *remotePath)
{
    FileDeltaDownloader *d = new FileDeltaDownloader;
    char *server = PullXMLAttribute(node, "Server");
    if (!server)
    {
        char *s = GetServer();
        if (s)
            server = strdup(s);
    }
    int port = PullXMLAttributeLong(node, "Port");
    if (!port)
        port = GetPort();
    if (server)
    {
        int download = PullXMLAttributeLong(node, "DownloadSize");
        int total = PullXMLAttributeLong(node, "TotalSize");
        int crc = PullXMLAttributeLong(node, "CRC");
        d->Initialize(server, port, remotePath, deltaLocal, deltaName, download, total, crc);
        if (!m_deltas)
            m_deltas = d;
        else
        {
            FileDeltaDownloader *last = m_deltas;
            while (last->GetNext())
                last = last->GetNext();
            last->SetNext(d);
        }
        free(server);
        return 1;
    }
    return 0;
}

// 0x00581618.
int FileDownloader::ScanAsTest()
{
    if (IsTestFile(this) || IsBetaFile(this) || IsXtraFile(this))
        return 1;
    return PatcherDownloader::ScanAsTest();
}
