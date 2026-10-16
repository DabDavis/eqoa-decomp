// DistributionDownloader.cpp (name ours) -- SETUP 0x0057E308 .. 0x0057FE40: a
// <Distribution> of the manifest.  It may live in an XML file of its own on the
// server; it may name a test and a beta distribution to fold into its sizes and
// its download; and each walk over it (scan, count, size) runs once until the
// matching ResetScanFlag.
#include "patcher.h"

// 0x0057E308.
DistributionDownloader::DistributionDownloader()
{
    m_isDistribution = 1;
    for (int i = 0; i < 2; i++)
    {
        m_xtraName[i] = 0;
        m_isXtra[i] = 0;
    }
    for (int j = 4; j >= 0; j--)
        m_scanFlag[j] = 0;
}

// 0x0057E390.  Only the top level owns its distribution info.
DistributionDownloader::~DistributionDownloader()
{
    for (int i = 0; i < 2; i++)
    {
        if (m_xtraName[i])
        {
            free(m_xtraName[i]);
            m_xtraName[i] = 0;
        }
    }
    if (m_info && !m_topLevel)
    {
        delete m_info;
        m_info = 0;
    }
}

// 0x0057E440.
int DistributionDownloader::Initialize(_xmlNode *node, PatcherDownloader *parent, PatcherDownloader *prev,
                                       const char *root, const char *path)
{
    PatcherDownloader::Initialize(node, parent, prev, root, path);
    if (!GetDistributionInfo())
    {
        SetDistributionInfo(new PatcherDistributionInfo);
        GetDistributionInfo()->m_impl->SetDistributionDownloader(this);
    }
    if (!GetDistributionInfo()->m_impl->CheckPatcherAPI())
        return 0;
    m_api = m_info->GetPatcher();
    return TestAndFinalizeDistribution(node);
}

// 0x0057E530.  A distribution with no children and a RemotePath is a file of
// its own.
int DistributionDownloader::DistributionIsExternal(_xmlNode *node)
{
    if (!node->children && m_remotePath)
        return 1;
    return 0;
}

// 0x0057E558.  Into memory first; failing that, through a temporary file.
int DistributionDownloader::GetExternalTree(_xmlNode *node)
{
    char msg[512];
    xmlDocPtr doc = 0;
    int ok = 1;
    char *mem = m_api->DownloadMemoryFile(0x19000, GetServer(), GetPort(), m_remotePath, 1);
    if (mem)
    {
        doc = xmlParseMemory(mem, strlen(mem));
        if (!doc)
        {
            FormatString(msg, "Error parsing xml file: %s.\n", m_remotePath);
            m_info->m_impl->CallError(40, msg);
            ok = 0;
        }
        free(mem);
    }
    else
        ok = 0;
    if (!ok)
    {
        char *tmp = GetTempName(".xml");
        if (!m_api->DownloadFile(GetServer(), GetPort(), m_remotePath, tmp, 1, 0, 0))
        {
            doc = xmlParseFile(tmp);
            if (!doc)
            {
                PatcherDistributionInfo *info = m_info;
                info->m_impl->CallError(40, FormatString(msg, "Error parsing xml file: %s.\n", m_remotePath));
                remove(tmp);
                return 0;
            }
            remove(tmp);
        }
    }
    if (doc)
    {
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (strcasecmp((char *)root->name, "VerantPatcher"))
        {
            FormatString(msg, "Document %s of the wrong type, VerantPatcher expected.\n", m_remotePath);
            m_info->m_impl->CallError(41, msg);
            xmlFreeDoc(doc);
            return 0;
        }
        int done = 1;
        for (xmlNodePtr n = root->children; n; n = n->next)
            if (!ExternalInitialize(n))
                done = 0;
        xmlFreeDoc(doc);
        return done;
    }
    FormatString(msg, "Error accessing external xml file: %s:%d/%s.\n", GetServer(), GetPort(), m_remotePath);
    m_info->m_impl->CallError(44, msg);
    return 0;
}

// 0x0057E810.  PatcherDownloader::Initialize again, for a node out of the
// external file -- the first free is m_server's where the base frees m_name.
int DistributionDownloader::ExternalInitialize(_xmlNode *node)
{
    if (m_server)
    {
        free(m_server);
        m_server = 0;
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
    m_useCRC = PullXMLAttributeBool(node, "UseCRC");
    m_delete = PullXMLAttributeBool(node, "Delete");
    if (GetParent())
    {
        if (GetParent()->NeedDelete())
            m_delete = 1;
        if (GetParent()->UsingCRC())
            m_useCRC = 1;
    }
    return TestAndFinalizeDistribution(node);
}

// 0x0057E988.
int DistributionDownloader::TestAndFinalizeDistribution(_xmlNode *node)
{
    PatcherDistributionInfo *info = GetDistributionInfo();
    info->m_impl->SetXMLInfo(node);
    SetXMLInfo(node);
    if (DistributionIsExternal(node))
        return GetExternalTree(node) != 0;
    if (!info->IsRestricted() || info->RestrictedFileExists())
    {
        if (!InitializeChildren(node))
            m_info->m_impl->CallError(40, "Error: Could not parse xml tree.");
        if (m_api->CheckOnInitialize())
            m_scanFlag[4] = 1;
    }
    return 1;
}

// 0x0057EA90.  A child distribution finds its own info; the rest share ours.
int DistributionDownloader::InitializeChildren(_xmlNode *node)
{
    PatcherDownloader *prev = 0;
    for (_xmlNode *n = node->children; n; n = n->next)
    {
        PatcherDownloader *d = ScanForDownloaderType((char *)n->name);
        if (!d)
        {
            if (!strcasecmp((char *)n->name, "Changes"))
                SetChangesFile(n);
            continue;
        }
        if (d->IsDistributionDownloader())
            d->Initialize(n, this, prev, 0, 0);
        else
            d->Initialize(n, this, prev, 0, 0, m_info);
        if (!GetChild())
            SetChild(d);
        if (prev)
            prev->SetNext(d);
        prev = d;
    }
    return 1;
}

// 0x0057EBC0.
PatcherDownloader *DistributionDownloader::ScanForDownloaderType(char *type)
{
    PatcherDownloader *d = 0;
    if (!strcasecmp(type, "Distribution"))
        d = new DistributionDownloader;
    else if (!strcasecmp(type, "Directory"))
        d = new DirectoryDownloader;
    else if (!strcasecmp(type, "File"))
        d = new FileDownloader;
    return d;
}

// 0x0057EC70.
int DistributionDownloader::SetXMLInfo(_xmlNode *node)
{
    m_defaultRoot = PullXMLAttribute(node, "Root");
    m_xtraName[0] = PullXMLAttribute(node, "TestDistribution");
    m_xtraName[1] = PullXMLAttribute(node, "BetaDistribution");
    return 1;
}

// 0x0057ECE0.  The release notes, an HTML file on the server, threaded into the
// distribution's version-ordered list.
int DistributionDownloader::SetChangesFile(_xmlNode *node)
{
    char *server = PullXMLAttribute(node, "Server");
    int port = PullXMLAttributeLong(node, "Port");
    char *remote = PullXMLAttribute(node, "RemotePath");
    int compressed = PullXMLAttributeBool(node, "Compressed");
    short version = PullXMLAttributeLong(node, "Version");
    int size = PullXMLAttributeLong(node, "DownloadSize");
    if (!server)
    {
        char *s = GetServer();
        if (s)
            server = strdup(s);
    }
    if (!port)
        port = GetPort();
    if (remote && server)
    {
        char *html = m_api->DownloadMemoryFile(0x1F400, server, port, remote, compressed != 0);
        if (html)
        {
            PatcherChanges *changes = GetDistributionInfo()->GetChanges();
            if (!changes)
            {
                PatcherDistributionInfo *info = GetDistributionInfo();
                info->SetChanges(new PatcherChanges);
                GetDistributionInfo()->GetChanges()->Initialize(version, html, 0, 0);
            }
            else
            {
                int done = 0;
                PatcherChanges *nc = new PatcherChanges;
                for (; changes; changes = changes->GetNext())
                {
                    if (version < changes->GetVersion())
                    {
                        PatcherChanges *prev = changes->GetPrevious();
                        PatcherChanges *next = changes->GetNext();
                        changes->SetPrevious(nc);
                        nc->Initialize(version, html, prev, changes);
                        if (!nc->GetPrevious())
                            GetDistributionInfo()->SetChanges(nc);
                        done = 1;
                    }
                }
                if (!done)
                {
                    changes = GetDistributionInfo()->GetChanges();
                    while (changes->GetNext())
                        changes = changes->GetNext();
                    changes->SetNext(nc);
                    nc->Initialize(version, html, changes, 0);
                }
            }
            if (html)
                free(html);
            if (server)
                free(server);
            if (remote)
                free(remote);
            return 1;
        }
    }
    return 0;
}

// 0x0057F028.  4 adds the test distribution, 8 the beta.
long DistributionDownloader::GetTotalSize(int flags)
{
    if (m_scanFlag[3])
        return 0;
    m_scanFlag[3] = 1;
    long total = 0;
    PatcherDownloader *c = GetChild();
    while (c)
    {
        if (!c->IsDistributionDownloader())
        {
            if (GetDistributionInfo()->IsSelected())
            {
                for (PatcherDownloader *d = c; d; d = d->GetNext())
                    total += d->GetTotalSize(flags);
                c = 0;
            }
            else
                c = c->GetNext();
        }
        else
        {
            total += c->GetTotalSize(flags);
            c = c->GetNext();
        }
    }
    if (flags & 4)
    {
        PatcherDistributionInfo *x = GetXtraDistribution(0);
        if (x)
        {
            x->Select(1);
            total += x->GetTotalSize(flags);
            x->Select(0);
        }
    }
    if (flags & 8)
    {
        PatcherDistributionInfo *x = GetXtraDistribution(1);
        if (x)
        {
            x->Select(1);
            total += x->GetTotalSize(flags);
            x->Select(0);
        }
    }
    return total;
}

// 0x0057F1D8.
long DistributionDownloader::GetDownloadSize(int flags)
{
    if (m_scanFlag[2])
        return 0;
    m_scanFlag[2] = 1;
    long total = 0;
    PatcherDownloader *c = GetChild();
    while (c)
    {
        if (!c->IsDistributionDownloader())
        {
            if (GetDistributionInfo()->IsSelected())
            {
                for (PatcherDownloader *d = c; d; d = d->GetNext())
                    total += d->GetDownloadSize(flags);
                c = 0;
            }
            else
                c = c->GetNext();
        }
        else
        {
            total += c->GetDownloadSize(flags);
            c = c->GetNext();
        }
    }
    if (flags & 0x84)
    {
        PatcherDistributionInfo *x = GetXtraDistribution(0);
        if (x)
        {
            x->Select(1);
            total += x->GetDownloadSize(flags);
            x->Select(0);
        }
    }
    if (flags & 0x88)
    {
        PatcherDistributionInfo *x = GetXtraDistribution(1);
        if (x)
        {
            x->Select(1);
            total += x->GetDownloadSize(flags);
            x->Select(0);
        }
    }
    return total;
}

// 0x0057F388.
void DistributionDownloader::ResetScanFlag(int flags)
{
    PatcherDownloader::ResetScanFlag(flags);
    if (flags < 5)
        m_scanFlag[flags] = 0;
}

// 0x0057F3D0.  0x40 skips the loose files of a distribution already checked
// on initialize, unless 0x10 asks for everything.
int DistributionDownloader::GetFileCount(int flags)
{
    if (m_scanFlag[1])
        return 0;
    m_scanFlag[1] = 1;
    int count = 0;
    int files = 1;
    if (!(flags & 0x10))
    {
        if ((flags & 0x40) && m_scanFlag[4])
            files = 0;
    }
    PatcherDownloader *c = GetChild();
    while (c)
    {
        if (c->IsDistributionDownloader())
        {
            count += c->GetFileCount(flags);
            c = c->GetNext();
        }
        else if (files && GetDistributionInfo()->IsSelected())
        {
            for (PatcherDownloader *d = c; d; d = d->GetNext())
                count += d->GetFileCount(flags);
            c = 0;
        }
        else
            c = c->GetNext();
    }
    if (flags & 0x24)
    {
        PatcherDistributionInfo *x = GetXtraDistribution(0);
        if (x)
        {
            x->Select(1);
            count += x->GetFileCount(flags);
            x->Select(0);
        }
    }
    if (flags & 0x28)
    {
        PatcherDistributionInfo *x = GetXtraDistribution(1);
        if (x)
        {
            x->Select(1);
            count += x->GetFileCount(flags);
            x->Select(0);
        }
    }
    return count;
}

// 0x0057F5B8.  100 from a child stops the walk.
int DistributionDownloader::Download(int flags)
{
    int result = 0;
    PatcherDownloader *c = GetChild();
    char cwd[256];
    getcwd(cwd, 255);
    const char *root = GetDistributionRoot();
    if (root)
    {
        PatcherAPI::GenerateLocalPath(root);
        chdir(root);
    }
    for (; c && result != 100; c = c->GetNext())
    {
        if (!c->IsDistributionDownloader())
        {
            if (GetDistributionInfo()->IsSelected())
            {
                for (PatcherDownloader *d = c; d && result != 100; d = d->GetNext())
                {
                    int r = d->Download(flags);
                    if (r && result != 100)
                        result = r;
                }
                if (!result)
                    FlagDownloaded();
            }
        }
        else
        {
            int r = c->Download(flags);
            if (r && result != 100)
                result = r;
        }
    }
    chdir(cwd);
    if (flags & 4)
    {
        PatcherDistributionInfo *x = GetXtraDistribution(0);
        if (x)
        {
            x->Select(1);
            result = x->Download(0) && result;
            x->Select(0);
        }
    }
    if (flags & 8)
    {
        PatcherDistributionInfo *x = GetXtraDistribution(1);
        if (x)
        {
            x->Select(1);
            result = x->Download(0) && result;
            x->Select(0);
        }
    }
    return result;
}

// 0x0057F808.
int DistributionDownloader::PrepForUninstall()
{
    PatcherDownloader *c = GetChild();
    int selected = GetDistributionInfo()->IsSelected();
    for (; c; c = c->GetNext())
        if (c->IsDistributionDownloader() || selected)
            c->PrepForUninstall();
    m_delete = 1;
    return 1;
}

// 0x0057F8B8.  The children see `on'; this node keeps its own setting.
int DistributionDownloader::UseCRC(bool on)
{
    int ok = 1;
    char cwd[128];
    getcwd(cwd, 128);
    if (GetDistributionRoot())
        chdir(GetDistributionRoot());
    int saved = m_useCRC;
    if (GetDistributionInfo()->IsSelected())
    {
        m_useCRC = on;
        for (PatcherDownloader *c = GetChild(); c && !gPatcherCancel; c = c->GetNext())
            if (!c->UseCRC(on))
                ok = 0;
    }
    m_useCRC = saved;
    if (GetDistributionRoot())
        chdir(cwd);
    return ok;
}

// 0x0057FA00.
int DistributionDownloader::Delete()
{
    char cwd[128];
    getcwd(cwd, 128);
    if (GetDistributionRoot())
        chdir(GetDistributionRoot());
    for (PatcherDownloader *c = GetChild(); c; c = c->GetNext())
        c->Delete();
    chdir(cwd);
    return 1;
}

// 0x0057FAB0.  0x80 rescans only what is flagged as a test.
int DistributionDownloader::ScanFiles(int flags)
{
    if (m_scanFlag[0])
        return 1;
    m_scanFlag[0] = 1;
    int scan = 1;
    if (((flags & 0x40) && m_scanFlag[4]) || ((flags & 0x80) && !ScanAsTest()))
        scan = 0;
    PatcherDownloader *c = GetChild();
    char cwd[128];
    getcwd(cwd, 128);
    if (GetDistributionRoot())
        chdir(GetDistributionRoot());
    while (c)
    {
        if (c->IsDistributionDownloader())
            c->ScanFiles(flags);
        else if (scan && GetDistributionInfo()->IsSelected())
        {
            for (PatcherDownloader *d = c; d; d = d->GetNext())
                d->ScanFiles(flags);
            c = 0;
            if (!gPatcherCancel && !(flags & 0x80))
                m_scanFlag[4] = 1;
            continue;
        }
        c = c->GetNext();
    }
    if (scan)
    {
        if (flags & 0xA4)
        {
            PatcherDistributionInfo *x = GetXtraDistribution(0);
            if (x)
            {
                x->Select(1);
                x->m_impl->GetDistributionDownloader()->ScanFiles(flags);
                x->Select(0);
            }
        }
        if (flags & 0xA8)
        {
            PatcherDistributionInfo *x = GetXtraDistribution(1);
            if (x)
            {
                x->Select(1);
                x->m_impl->GetDistributionDownloader()->ScanFiles(flags);
                x->Select(0);
            }
        }
    }
    chdir(cwd);
    return 1;
}

// 0x0057FD38.
PatcherDistributionInfo *DistributionDownloader::GetXtraDistribution(int which)
{
    PatcherDistributionInfo *x = 0;
    PatcherDistributionInfo *list = m_api->GetDistributionList();
    if (list && m_xtraName[which])
        x = PatcherDistributionInfo::GetDistributionByName(list, m_xtraName[which]);
    if (x)
        x->SetXtraDistribution(which, 1);
    return x;
}

// 0x0057FDB8.  Only the beta (1) is ever flagged.
void DistributionDownloader::SetXtraDistribution(int which, bool on)
{
    if (which == 1)
        m_isXtra[which] = on;
}

// 0x0057FDD0.
int DistributionDownloader::IsXtraDistribution(int which)
{
    if (which == 1)
        return m_isXtra[which];
    return 0;
}
