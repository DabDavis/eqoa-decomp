// DirectoryDownloader.cpp (name ours) -- SETUP 0x0057D938 .. 0x0057E308: a
// <Directory> of the manifest, whose children are directories and files.
#include "patcher.h"

// 0x0057D938.
DirectoryDownloader::DirectoryDownloader()
{
    m_isDistribution = 0;
    m_localName = 0;
}

// 0x0057D978.
DirectoryDownloader::~DirectoryDownloader()
{
    if (m_localName)
    {
        free(m_localName);
        m_localName = 0;
    }
}

// 0x0057D9D0.  The XML gives the paths relative to the parent; they are made
// whole here.
int DirectoryDownloader::Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
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
        if (m_name)
        {
            if (m_localName)
                free(m_localName);
            m_localName = strdup(m_name);
        }
    }
    m_remotePath = GetFullPath(m_name, remote, path);
    if (local)
        free(local);
    if (remote)
        free(remote);
    InitializeChildren(m_localPath, m_remotePath, node);
    return 1;
}

// 0x0057DAF0.
int DirectoryDownloader::InitializeChildren(char *local, char *remote, _xmlNode *node)
{
    PatcherDownloader *prev = 0;
    for (_xmlNode *n = node->children; n; n = n->next)
    {
        if (gPatcherCancel)
            return 0;
        PatcherDownloader *d = ScanForDownloaderType((char *)n->name);
        if (d)
        {
            d->Initialize(n, this, prev, m_localPath, m_remotePath, m_info);
            if (!GetChild())
                SetChild(d);
            if (prev)
                prev->SetNext(d);
            prev = d;
        }
    }
    return 1;
}

// 0x0057DBD0.
PatcherDownloader *DirectoryDownloader::ScanForDownloaderType(char *type)
{
    PatcherDownloader *d = 0;
    if (!strcasecmp(type, "Directory"))
        d = new DirectoryDownloader;
    else if (!strcasecmp(type, "File"))
        d = new FileDownloader;
    return d;
}

// 0x0057DC58.
int DirectoryDownloader::SetXMLInfo(_xmlNode *node)
{
    m_localName = PullXMLAttribute(node, "LocalName");
    return 1;
}

// 0x0057DC90.
long DirectoryDownloader::GetTotalSize(int flags)
{
    long total = 0;
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        total += c->GetTotalSize(flags);
    return total;
}

// 0x0057DD08.
long DirectoryDownloader::GetDownloadSize(int flags)
{
    long total = 0;
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        total += c->GetDownloadSize(flags);
    return total;
}

// 0x0057DD80.
int DirectoryDownloader::GetFileCount(int flags)
{
    int count = 0;
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        count += c->GetFileCount(flags);
    return count;
}

// 0x0057DDF8.  The special directories are the PC's; none exists here.
int DirectoryDownloader::Download(int flags)
{
    printf("!!DirectoryDownloader::Download\n");
    if (!NeedDownload(flags))
        return 0;
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
    int result = 0;
    PatcherDownloader *c = GetChild();
    if (!NeedDelete() && NeedDownload(flags))
    {
        MakeDirectory();
        for (; c && result != 100; c = c->GetNext())
        {
            int r = c->Download(flags);
            if (r && result != 100)
                result = r;
        }
        if (!result)
            FlagDownloaded();
    }
    printf("!!!DirectoryDownloader returning %i\n", result);
    return result;
}

// 0x0057DFA8.
int DirectoryDownloader::Delete()
{
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        c->Delete();
    if (NeedDelete() && DeleteDirectory())
        FlagDeleted();
    return 1;
}

// 0x0057E038.
int DirectoryDownloader::UseCRC(bool on)
{
    int ok = 1;
    if (gPatcherCancel)
        return 0;
    if (!NeedDelete())
        MakeDirectory();
    for (PatcherDownloader *c = GetChild(); c && !gPatcherCancel; c = c->GetNext())
        if (!c->UseCRC(on))
            ok = 0;
    return ok;
}

// 0x0057E0F8.
int DirectoryDownloader::MakeDirectory()
{
    if (m_localPath)
        mkdir(m_localPath, 0);
    return 1;
}

// 0x0057E128.
int DirectoryDownloader::DeleteDirectory()
{
    if (m_localPath)
        rmdir(m_localPath);
    return 1;
}

// 0x0057E158.
char *DirectoryDownloader::GetFullPath(const char *name, const char *path, const char *root)
{
    bool haveName = name != 0;
    bool haveRoot = root != 0;
    char *full = 0;
    if (path)
    {
        if (!strcasecmp(path, "::CurrentDirectory::"))
            return strdup(root);
        if (!strcasecmp(path, "::HomeDirectory::"))
            return 0;
        if (!strcasecmp(path, "::WindowsHome::"))
            return strdup(getenv("windir"));
        if (!strcasecmp(path, "::Desktop::"))
            ;
        else if (!strcasecmp(path, "::StartMenu::"))
            ;
        return strdup(path);
    }
    if (haveRoot && haveName)
    {
        full = (char *)malloc(strlen(root) + strlen(name) + 2);
        sprintf(full, "%s/%s", root, name);
    }
    else if (haveRoot)
        full = strdup(root);
    else if (haveName)
        full = strdup(name);
    return full;
}
