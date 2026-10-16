// PatcherAPI_Impl.cpp (name ours) -- SETUP 0x00582F40 .. 0x00586900: the patcher
// proper.  It fetches the manifest, builds the distribution tree from it, and
// runs every operation through Patcher_thread -- synchronously: the PS2 port
// calls the thread function instead of starting a thread.
#include "patcher.h"

int gPatcherCancel;

// The bytes-downloaded tick, inline wherever the count moves.
static inline void Tick(PatcherAPI_Impl *p)
{
    if (p->m_bytesTotal > 100)
        p->m_api->OnTick(p->m_bytesDone / (p->m_bytesTotal / 100));
    else if (p->m_bytesTotal)
        p->m_api->OnTick(p->m_bytesDone * 100 / p->m_bytesTotal);
}

// 0x00582F40.
PatcherAPI_Impl::PatcherAPI_Impl()
{
    gPatcherCancel = 0;
    m_threadInUse = 0;
    m_needsExit = 0;
    m_downloading = 0;
    m_initializing = 0;
    m_threadData = new PatcherThreadData;
    m_checkOnInitialize = 1;
    m_x3E0 = 0;
    m_x3D8 = 0;
    m_productName = 0;
    m_server = 0;
    m_port = 0;
    m_file = 0;
    m_doc = 0;
    m_user = 0;
    m_password = 0;
    m_api = 0;
    m_distList = 0;
    m_changes = 0;
    m_patchingPatcher = 0;
    m_restartNameSet = 0;
    m_version = 0;
    m_estimatesFull = 0;
    m_commandLine[0] = 0;
    m_restartName[0] = 0;
    m_x5EC[0] = 0;
    m_cancel = 0;
    m_startTime = 0;
    m_bytesTotal = 0;
    m_bytesDone = 0;
    m_bytesDownloaded = 0;
    m_timeDownloaded = 0;
    m_unzipDone = 0;
    m_unzipTotal = 0;
    m_bytesUnzipped = 0;
    m_timeUnzipped = 0;
    m_now = 0;
    m_lastEstimate = 0;
    m_quickScan = 0;
    m_useCRC = 0;
    CommandLineOperations();
    m_scanCount = 0;
    m_scanTotal = 0;
    m_estimateIndex = 0;
    for (int i = 49; i >= 0; i--)
        m_estimates[i] = 0;
}

// 0x00583058.
PatcherAPI_Impl::~PatcherAPI_Impl()
{
    if (m_doc)
    {
        xmlFreeDoc(m_doc);
        m_doc = 0;
    }
    if (m_productName)
    {
        free(m_productName);
        m_productName = 0;
    }
    if (m_server)
    {
        free(m_server);
        m_server = 0;
    }
    if (m_file)
    {
        free(m_file);
        m_file = 0;
    }
    if (m_user)
    {
        free(m_user);
        m_user = 0;
    }
    if (m_password)
    {
        free(m_password);
        m_password = 0;
    }
    if (m_distList)
    {
        delete m_distList;
        m_distList = 0;
    }
    if (m_changes)
    {
        delete m_changes;
        m_changes = 0;
    }
    if (m_threadData)
    {
        delete m_threadData;
        m_threadData = 0;
    }
    if (m_x3E0)
        m_x3E0 = 0;
}

// 0x005831A0.  "Hello" marks a buffer nothing was read into.
char *PatcherAPI_Impl::DownloadMemoryFile(int size, char *server, int port, char *path, int compressed)
{
    if (!size || !server || !path)
        return 0;
    char *buf = (char *)memalign(128, size);
    memset(buf, 0, size);
    sprintf(buf, "Hello");
    if (!DownloadToMemory(buf, size, server, port, path, compressed))
        return buf;
    free(buf);
    return 0;
}

// 0x00583278.  The server can answer "come back later" while it rebuilds the
// manifest; that page is re-fetched until it goes away.
int PatcherAPI_Impl::Initialize(const char *server, int port, const char *file, PatcherAPI *api, int flags)
{
    if (server)
    {
        if (m_server)
        {
            free(m_server);
            m_server = 0;
        }
        m_server = strdup(server);
    }
    m_port = port;
    if (file)
    {
        if (m_file)
        {
            free(m_file);
            m_file = 0;
        }
        m_file = strdup(file);
    }
    m_api = api;
    if (m_doc)
    {
        xmlFreeDoc(m_doc);
        m_doc = 0;
    }
    int ok = 1;
    char *mem = DownloadMemoryFile(0x19000, m_server, m_port, m_file, 1);
    if (mem)
    {
        int done = 0;
        int told = 0;
        while (!done)
        {
            if (strstr(mem, "<TITLE>_rerun_xml_script_</TITLE>"))
            {
                if (!told)
                {
                    m_api->OnStatusInfo("Updating server files... Please be patient...");
                    told = 1;
                }
                else
                    m_api->OnStatusInfo(".");
                free(mem);
                mem = DownloadMemoryFile(0x19000, m_server, m_port, m_file, 1);
                if (mem)
                {
                    m_api->OnError(43, "Error opening remote xml file.\nCould not access patcher update file.  Please verify your network connection.  If the problem persists, please contact station.sony.com.\n");
                    ok = 0;
                }
            }
            else
                done = 1;
        }
        if (ok)
        {
            m_doc = xmlParseMemory(mem, strlen(mem));
            if (m_doc)
            {
                xmlNodePtr root = xmlDocGetRootElement(m_doc);
                if (!root || strcasecmp((char *)root->name, "VerantPatcher"))
                {
                    xmlFreeDoc(m_doc);
                    m_doc = 0;
                }
            }
        }
        free(mem);
    }
    else
        return 0;
    return ContinueInitialization(flags);
}

// 0x005834C8.
int PatcherAPI_Impl::Initialize(const char *server, int port, const char *file, PatcherAPI *api,
                                const char *user, const char *password, int flags)
{
    m_api = api;
    if (m_user)
    {
        free(m_user);
        m_user = 0;
    }
    if (user)
        m_user = strdup(user);
    if (m_password)
    {
        free(m_password);
        m_password = 0;
    }
    if (password)
        m_password = strdup(password);
    if (m_api->ValidUser(m_user, m_password))
        return Initialize(server, port, file, api, flags);
    api->OnError(52, "User / password combination invalid.  Access Denied.\n");
    return 0;
}

// 0x00583608.
int PatcherAPI_Impl::InitializeFromLocalFile(const char *file, PatcherAPI *api, int flags)
{
    char msg[512];
    m_api = api;
    m_doc = xmlParseFile(file);
    if (!m_doc)
    {
        m_api->OnError(53, FormatString(msg, "Error initializing patcher.\n"));
        return 0;
    }
    return ContinueInitialization(flags);
}

// 0x00583698.
int PatcherAPI_Impl::ContinueInitialization(int flags)
{
    xmlNodePtr root = xmlDocGetRootElement(m_doc);
    if (!root || strcasecmp((char *)root->name, "VerantPatcher"))
    {
        m_api->OnError(41, "Document of the wrong type, VerantPatcher expected\n");
        xmlFreeDoc(m_doc);
        m_doc = 0;
        return 0;
    }
    m_version = PullXMLAttributeLong(root, "Version");
    xmlNodePtr n;
    for (n = root->children; n && n->type != XML_ELEMENT_NODE; n = n->next)
        ;
    if (!n)
    {
        m_api->OnError(41, "No Product element found\n");
        xmlFreeDoc(m_doc);
        return 0;
    }
    if (strcasecmp((char *)n->name, "Product"))
    {
        m_api->OnError(41, "Outer node of the wrong type, Product expected\n");
        xmlFreeDoc(m_doc);
        m_doc = 0;
        return 0;
    }
    m_productName = PullXMLAttribute(n, "Name");
    m_api->OnProduct(m_productName);
    char *rootPath = PullXMLAttribute(n, "Root");
    FindRoots(rootPath);
    SetRoot();
    if (m_root && strlen(m_root))
        chdir(m_root);
    if (rootPath)
        free(rootPath);
    InitializeChanges(n);
    StartInitializeThread(n);
    return 1;
}

// 0x00583888.
void PatcherAPI_Impl::StartInitializeThread(_xmlNode *node)
{
    if (m_initializing)
        return;
    m_initializing = 1;
    if (m_x3E0)
        m_x3E0 = 0;
    if (m_distList)
        delete m_distList;
    m_distList = new PatcherDistributionInfo;
    m_x3D8 = 0;
    m_threadData->m_op = 0;
    m_threadData->m_node = node;
    m_threadData->m_api = m_api;
    m_threadData->m_impl = this;
    m_threadData->m_dist = m_distList;
    Patcher_thread(m_threadData);
    if (!m_x3E0)
        m_initializing = 0;
    else
        m_threadInUse = 1;
}

// 0x00583958.
void PatcherAPI_Impl::SetInitializing(bool on)
{
    m_initializing = on;
    if (!on)
        SetLocal();
}

// 0x00583980.
void PatcherAPI_Impl::CommandLineOperations()
{
}

// 0x00583988.  What is left of handing the patcher's own update to the game.
void PatcherAPI_Impl::PatchPatcher(const char *a, const char *b)
{
    if (!m_patchingPatcher && a && b)
        strcmp(a, b);
}

// 0x005839C8.
void PatcherAPI_Impl::SetRestartName(const char *name, const char *arg)
{
    if (name && !m_restartNameSet)
    {
        sprintf(m_restartName, name);
        AddCommandLineArg("PatchPatcher");
        AddCommandLineArg(arg);
        m_restartNameSet = 1;
    }
}

// 0x00583A30.
void PatcherAPI_Impl::AddCommandLineArg(const char *arg)
{
    if (!arg)
        return;
    if (!m_commandLine[0])
        sprintf(m_commandLine, "%s", arg);
    else
        sprintf(m_commandLine, "%s %s", m_commandLine, arg);
}

// 0x00583A80.
int PatcherAPI_Impl::InitializeChanges(_xmlNode *node)
{
    for (xmlNodePtr c = node->children; c; c = c->next)
        if (!strcasecmp((char *)c->name, "Changes"))
            SetChangesFile(c);
    return 1;
}

// 0x00583AF0.  DistributionDownloader::SetChangesFile again, for the product.
int PatcherAPI_Impl::SetChangesFile(_xmlNode *node)
{
    char *server = PullXMLAttribute(node, "Server");
    int port = PullXMLAttributeLong(node, "Port");
    char *remote = PullXMLAttribute(node, "RemotePath");
    int compressed = PullXMLAttributeBool(node, "Compressed");
    short version = PullXMLAttributeLong(node, "Version");
    int size = PullXMLAttributeLong(node, "DownloadSize");
    if (!server && m_server)
        server = strdup(m_server);
    if (!port)
        port = m_port;
    if (!strlen(remote))
        return 0;
    if (!strlen(server))
        return 0;
    char *html = DownloadMemoryFile(0x1F400, server, port, remote, compressed != 0);
    if (html)
    {
        PatcherChanges *changes = GetProductChanges();
        if (!changes)
        {
            m_changes = new PatcherChanges;
            GetProductChanges()->Initialize(version, html, 0, 0);
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
                        m_changes = nc;
                    done = 1;
                }
            }
            if (!done)
            {
                changes = GetProductChanges();
                while (changes->GetNext())
                    changes = changes->GetNext();
                changes->SetNext(nc);
                nc->Initialize(version, html, changes, 0);
            }
        }
        free(html);
    }
    else
    {
        if (server)
            free(server);
        if (remote)
            free(remote);
        return 0;
    }
    if (server)
        free(server);
    if (remote)
        free(remote);
    return 1;
}

// 0x00583DE0.  A root with no drive borrows the current directory's.
int PatcherAPI_Impl::FindRoots(const char *root)
{
    getcwd(m_cwd, 255);
    if (!root || strlen(root) < 2)
    {
        m_root[0] = 0;
        return 1;
    }
    if (root[1] != ':' && root[0] == '\\')
    {
        sprintf(m_root, "%c:%s", m_cwd[0], root);
        return 1;
    }
    sprintf(m_root, root);
    return 1;
}

// 0x00583E88.
int PatcherAPI_Impl::SetRoot()
{
    char msg[512];
    if (!m_root || strlen(m_root) < 2 || PatcherAPI::GenerateLocalPath(m_root))
        return 1;
    m_api->OnError(50, FormatString(msg, "Could not generate path %s\n", m_root));
    return 0;
}

// 0x00583F20.  PatcherDownloader::GenerateLocalSubpath, owning `done'.
int PatcherAPI_Impl::GenerateLocalSubPath(const char *path, char *done, const char *cwd)
{
    if (done && strlen(done) == strlen(path))
    {
        free(done);
        return 1;
    }
    int i = strlen(done) + 1;
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
    if (done)
        free(done);
    done = strdup(path);
    done[i] = 0;
    if ((int)strlen(done) >= 2 && done[1] != ':')
        chdir(cwd);
    if (!chdir(done) || (!mkdir(done, 0) && !chdir(done)))
    {
        GenerateLocalSubPath(path, done, cwd);
        return 1;
    }
    return 0;
}

// 0x005840A8.
int PatcherAPI_Impl::SetDistributionRoot(const char *names, const char *root)
{
    if (!names || !strlen(names))
    {
        if (!root)
            m_root[0] = 0;
        else
            sprintf(m_root, root);
        return 1;
    }
    char *copy = strdup(names);
    char sep[] = ",";
    for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
    {
        PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), t);
        if (d)
            d->SetDistributionRoot(root);
    }
    free(copy);
    if (m_distList)
        m_distList->CalculateHierarchicalProperties();
    return 1;
}

// 0x005841B0.
int PatcherAPI_Impl::RescanFiles(const char *names, int flags)
{
    while (m_threadInUse)
        Sleep(20);
    gPatcherCancel = 0;
    if (GetDistributionList())
        GetDistributionList()->ResetScanFlag(0);
    if (m_x3E0)
        m_x3E0 = 0;
    m_x3D8 = 0;
    m_threadData->m_api = m_api;
    m_threadData->m_impl = this;
    m_threadData->m_op = 3;
    m_threadData->m_flags = flags;
    if (!names || !strlen(names))
        m_threadData->m_name[0] = 0;
    else
        sprintf(m_threadData->m_name, names);
    Patcher_thread(m_threadData);
    m_threadInUse = 1;
    m_api->OnStatusInfo("Rescanning Fileset...\n");
    return 1;
}

// 0x005842D0.
bool PatcherAPI_Impl::RescanRoutine(const char *names, int flags)
{
    char cwd[128];
    getcwd(cwd, 128);
    if (m_root)
        chdir(m_root);
    m_scanTotal = GetFileCount(names, flags);
    m_scanCount = 0;
    m_api->OnSetScanTickCount(100);
    if (!names || !strlen(names))
    {
        for (PatcherDistributionInfo *d = GetDistributionList(); d; d = d->GetNext())
        {
            d->RescanFiles(flags);
            d->CalculateHierarchicalProperties();
        }
    }
    else
    {
        char *copy = strdup(names);
        char sep[] = ",";
        for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
        {
            PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), t);
            if (d)
            {
                bool was = d->Select(1);
                d->RescanFiles(flags);
                d->CalculateHierarchicalProperties();
                d->Select(was);
            }
        }
        free(copy);
    }
    if (!m_root)
        return 1;
    chdir(cwd);
    return 1;
}

// 0x00584498.
bool PatcherAPI_Impl::UseCRCRoutine(bool on)
{
    char cwd[128];
    getcwd(cwd, 128);
    gPatcherCancel = 0;
    if (m_root)
        chdir(m_root);
    m_scanTotal = GetFileCount(0, 0x10);
    m_scanCount = 0;
    m_api->OnSetScanTickCount(100);
    for (PatcherDistributionInfo *d = GetDistributionList(); d; d = d->GetNext())
    {
        d->UseCRC(on);
        d->CalculateHierarchicalProperties();
    }
    if (m_root)
        chdir(cwd);
    return 1;
}

// 0x00584588.
int PatcherAPI_Impl::SetLocal()
{
    return !chdir(m_cwd);
}

// 0x005845A8.
const char *PatcherAPI_Impl::GetRoot(const char *name)
{
    if (name)
    {
        PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), name);
        if (d)
            return d->GetDistributionRoot();
    }
    return m_root;
}

// 0x00584608.
char *PatcherAPI_Impl::GetLocal()
{
    return m_cwd;
}

// 0x00584610.
int PatcherAPI_Impl::UseCRC(bool on, int flags)
{
    while (m_threadInUse)
        Sleep(20);
    m_useCRC = on;
    if (m_x3E0)
        m_x3E0 = 0;
    m_unzipTotal = 0;
    m_bytesTotal = 0;
    m_x3D8 = 0;
    m_threadData->m_api = m_api;
    m_threadData->m_impl = this;
    m_threadData->m_op = 2;
    m_threadData->m_useCRC = on;
    Patcher_thread(m_threadData);
    m_threadInUse = 1;
    return 1;
}

// 0x005846B8.
int PatcherAPI_Impl::InitialScan(const char *names)
{
    while (m_threadInUse)
        Sleep(20);
    if (m_x3E0)
        m_x3E0 = 0;
    m_x3D8 = 0;
    m_threadData->m_api = m_api;
    m_threadData->m_impl = this;
    m_threadData->m_op = 4;
    if (GetDistributionList())
        GetDistributionList()->ResetScanFlag(0);
    if (!names || !strlen(names))
        m_threadData->m_name[0] = 0;
    else
        sprintf(m_threadData->m_name, names);
    Patcher_thread(m_threadData);
    m_threadInUse = 1;
    return 1;
}

// 0x005847A0.
int PatcherAPI_Impl::UsingCRC()
{
    if (m_useCRC == 1)
        return 1;
    if (m_distList)
        return m_distList->UsingCRC();
    return 0;
}

// 0x005847E0.
int PatcherAPI_Impl::NeedDownload(int flags)
{
    if (m_distList)
        return m_distList->NeedDownload(flags);
    return 0;
}

// 0x00584808.
int PatcherAPI_Impl::DeleteOldFiles()
{
    if (m_distList)
        return m_distList->DeleteOld();
    return 0;
}

// 0x00584830.
int PatcherAPI_Impl::Download(const char *names, int flags)
{
    gPatcherCancel = 0;
    m_cancel = 0;
    if (m_downloading)
        return 0;
    m_downloading = 1;
    if (m_x3E0)
        m_x3E0 = 0;
    m_x3D8 = 0;
    m_threadData->m_api = m_api;
    m_threadData->m_impl = this;
    m_threadData->m_op = 1;
    m_threadData->m_flags = flags;
    if (!names || !strlen(names))
        m_threadData->m_name[0] = 0;
    else
    {
        Select(names, 1);
        sprintf(m_threadData->m_name, names);
    }
    m_api->OnStart();
    bool ok = Patcher_thread(m_threadData);
    printf("!!Success = %i\n", ok);
    return ok;
}

// 0x00584930.
int PatcherAPI_Impl::Uninstall(const char *names)
{
    char sep[] = ",";
    int done = 0;
    if (!names || !strlen(names))
    {
        for (PatcherDistributionInfo *d = GetDistributionList(); d; d = d->GetNext())
            d->Uninstall();
        done = 1;
    }
    else
    {
        char *copy = strdup(names);
        for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
        {
            PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), t);
            if (d)
            {
                d->Uninstall();
                done = 1;
            }
        }
        free(copy);
    }
    return done;
}

// 0x00584A40.
int PatcherAPI_Impl::GetFileCount(const char *names, int flags)
{
    char sep[] = ",";
    int count = 0;
    PatcherDistributionInfo *list = GetDistributionList();
    if (list)
        list->ResetScanFlag(1);
    if (!names || !strlen(names))
    {
        for (; list; list = list->GetNext())
            count += list->GetFileCount(flags);
        return count;
    }
    char *copy = strdup(names);
    for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
    {
        PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(list, t);
        if (d)
        {
            bool was = d->IsSelected();
            d->Select(1);
            count += d->GetFileCount(flags);
            d->Select(was);
        }
    }
    free(copy);
    return count;
}

// 0x00584B98.
bool PatcherAPI_Impl::InitialScanRoutine(const char *names)
{
    char sep[] = ",";
    m_scanTotal = GetFileCount(names, 0x60);
    m_scanCount = 0;
    m_api->OnSetScanTickCount(100);
    PatcherDistributionInfo *list = GetDistributionList();
    if (!names || !strlen(names))
    {
        for (; list; list = list->GetNext())
        {
            list->InitialScan();
            list->CalculateHierarchicalProperties();
        }
    }
    else
    {
        char *copy = strdup(names);
        for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
        {
            PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(list, t);
            if (d)
            {
                bool was = d->IsSelected();
                d->Select(1);
                d->InitialScan();
                d->CalculateHierarchicalProperties();
                d->Select(was);
            }
        }
        free(copy);
    }
    return 1;
}

// 0x00584D00.
bool PatcherAPI_Impl::DownloadRoutine(const char *names, int flags)
{
    gPatcherCancel = 0;
    m_cancel = 0;
    m_bytesDone = 0;
    m_bytesTotal = 0;
    m_bytesDownloaded = 0;
    m_timeDownloaded = 0;
    m_unzipDone = 0;
    m_unzipTotal = 0;
    m_bytesUnzipped = 0;
    m_timeUnzipped = 0;
    m_unzipTotal = GetTotalSize(names, flags);
    m_bytesTotal = GetDownloadSize(names, flags);
    if (m_bytesTotal)
        m_api->OnSetTickCount(100);
    char sep[] = ",";
    bool ok = 0;
    if (!names || !strlen(names))
    {
        ok = 1;
        for (int i = 0; i < 50; i++)
            GetEstimate();
        if (!GetDistributionList()->Download(flags))
        {
            printf("PatcherAPI_Impl::DownloadRoutine successFalse 1\n");
            ok = 0;
        }
    }
    else
    {
        char *copy = strdup(names);
        for (int i = 0; i < 50; i++)
            GetEstimate();
        for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
        {
            PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), t);
            if (d)
            {
                d->Download(flags);
                ok = 1;
            }
        }
        free(copy);
    }
    if (!GetDownloadSize(names, 0))
        m_api->OnTick(100);
    if (!m_cancel)
        m_api->OnFinish();
    m_downloading = 0;
    printf("PatcherAPI_Impl::DownloadRoutine success = %i\n", ok);
    return ok;
}

// 0x00584F58.
int PatcherAPI_Impl::ThreadBusy()
{
    return m_threadInUse || m_downloading;
}

// 0x00584F80.
int PatcherAPI_Impl::Select(const char *names, bool on)
{
    char sep[] = ",";
    int done = 0;
    if (!names || !strlen(names))
    {
        for (PatcherDistributionInfo *d = GetDistributionList(); d; d = d->GetNext())
        {
            d->Select(on);
            d->SelectChildren(on);
        }
        done = 1;
    }
    else
    {
        char *copy = strdup(names);
        for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
        {
            PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), t);
            if (d)
            {
                d->Select(on);
                done = 1;
                d->SelectChildren(on);
            }
        }
        free(copy);
    }
    return done;
}

// 0x005850B8.
int PatcherAPI_Impl::Cancel()
{
    m_cancel = 1;
    gPatcherCancel = 1;
    return 1;
}

// 0x005850D0.
int PatcherAPI_Impl::Continue()
{
    gPatcherCancel = 0;
    m_cancel = 0;
    return Download(0, 0);
}

// 0x00585100.
char *PatcherAPI_Impl::GetProductName()
{
    return m_productName;
}

// 0x00585108.
short PatcherAPI_Impl::GetVersion()
{
    return m_version;
}

// 0x00585110.
PatcherChanges *PatcherAPI_Impl::GetProductChanges()
{
    return m_changes;
}

// 0x00585118.
PatcherDistributionInfo *PatcherAPI_Impl::GetDistributionList()
{
    return m_distList;
}

// 0x00585120.  When the download will finish: the bytes left over the rate so
// far, averaged over the last 50 estimates.
long PatcherAPI_Impl::GetEstimate()
{
    long start;
    start = time(&start);
    double unzipRate;
    if (m_timeUnzipped > 2000)
        unzipRate = (float)m_bytesUnzipped / (float)m_timeUnzipped;
    else
        unzipRate = 2393.0;
    double downloadRate;
    if (m_timeDownloaded > 2000)
        downloadRate = (float)m_bytesDownloaded / (float)m_timeDownloaded;
    else
        downloadRate = 1868.0;
    long unzipLeft = m_unzipTotal - m_unzipDone;
    long downloadLeft = m_bytesTotal - m_bytesDone;
    if (!(unzipRate > 0))
        unzipRate = 1.0;
    if (!(downloadRate > 0))
        downloadRate = 1.0;
    long left = (long)(unzipLeft / unzipRate + downloadLeft / downloadRate);
    long now;
    now = time(&now);
    long finish = now + left / 1000 - m_startTime;
    long sum = finish;
    long count;
    int i;
    if (m_estimatesFull)
    {
        for (i = 0; i < 50; i++)
            sum += m_estimates[i];
        count = 51;
    }
    else
    {
        for (i = 0; i < m_estimateIndex; i++)
            sum += m_estimates[i];
        count = m_estimateIndex + 1;
    }
    long average = sum / count;
    m_estimates[m_estimateIndex] = finish;
    if (++m_estimateIndex == 50)
    {
        m_estimateIndex = 0;
        m_estimatesFull = 1;
    }
    long estimate = average + m_startTime;
    return now < estimate ? estimate : now;
}

// 0x00585420.
long PatcherAPI_Impl::GetEstimateRemaining()
{
    long now;
    now = time(&now);
    GetEstimate();
    return GetEstimate() - now;
}

// 0x00585468.
long PatcherAPI_Impl::GetTotalSize(const char *names, int flags)
{
    PatcherDistributionInfo *list = GetDistributionList();
    if (list)
        list->ResetScanFlag(3);
    if (!names || !strlen(names))
    {
        if (m_distList)
            return m_distList->GetTotalSize(flags);
        return 0;
    }
    char sep[] = ",";
    long total = 0;
    char *copy = strdup(names);
    if (copy)
    {
        for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
        {
            PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), t);
            if (d)
            {
                if (!d->IsSelected())
                {
                    d->Select(1);
                    total += d->GetTotalSize(flags);
                    d->Select(0);
                }
                else
                    total += d->GetTotalSize(flags);
            }
        }
        free(copy);
    }
    return total;
}

// 0x005855C0.
long PatcherAPI_Impl::GetDownloadSize(const char *names, int flags)
{
    PatcherDistributionInfo *list = GetDistributionList();
    if (list)
        list->ResetScanFlag(2);
    if (!names || !strlen(names))
    {
        if (m_distList)
            return m_distList->GetDownloadSize(flags);
        return 0;
    }
    char sep[] = ",";
    long total = 0;
    char *copy = strdup(names);
    if (copy)
    {
        for (char *t = strtok(copy, sep); t; t = strtok(0, sep))
        {
            PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), t);
            if (d)
            {
                if (!d->IsSelected())
                {
                    d->Select(1);
                    total += d->GetDownloadSize(flags);
                    d->Select(0);
                }
                else
                    total += d->GetDownloadSize(flags);
            }
        }
        free(copy);
    }
    return total;
}

// 0x00585718.  Http_get's status: 4 reading, 5 failed, 6 done, 8 aborted.
int PatcherAPI_Impl::DownloadToMemory(char *buf, int size, const char *server, int port, char *path,
                                      int compressed)
{
    VIPatchImage *images = m_api->FindPatchImage(0);
    if (!images)
        return -1;
    Http_get http(images);
    char url[256];
    char msg[512];
    unsigned int status = 0;
    unsigned int bytes = 0;
    http.Initialize(m_x5EC);
    http.SetUrlListener(this);
    if (m_cancel)
        return 100;
    if (!path)
    {
        m_api->OnError(60, "Null path requested for download.\n");
        return 0;
    }
    if (path[0] == '\\' || path[0] == '/')
    {
        path[0] = '/';
        sprintf(url, "http://%s:%i%s", server, port, path);
    }
    else
        sprintf(url, "http://%s:%i/%s", server, port, path);
    int len = strlen(url);
    for (int i = 0; i < len; i++)
        if (url[i] == '\\')
            url[i] = '/';
    m_api->OnFile(buf);
    RotateThreadReadyQueue(100);
    http.MemoryGet(buf, size, url, m_user, m_password, compressed);
    for (;;)
    {
        RotateThreadReadyQueue(100);
        if (status != http.GetStatus() || bytes != http.Get_bytes_received())
        {
            status = http.GetStatus();
            switch (status)
            {
            case 1:
            case 2:
            case 3:
            case 4:
            case 7:
                break;
            case 5:
            {
                int type = http.GetErrorType();
                char *text = http.GetErrorString();
                if (text)
                    FormatString(msg, "Error downloading into memory: %s\n", text);
                else
                    FormatString(msg, "Error downloading into memory\n");
                http.Close();
                return type;
            }
            case 6:
                m_api->OnFileTick(100);
                m_api->OnFileFinished(buf);
                http.Close();
                return 0;
            case 8:
                m_api->OnError(http.GetErrorType(), http.GetErrorString());
                return 100;
            }
        }
        if (m_cancel && http.GetBytesDownloaded() > 100)
            return 100;
        RotateThreadReadyQueue(100);
    }
}

// 0x00585A88.
void PatcherAPI_Impl::OnBytesDownloaded(const char *url, const char *file, int got, int total)
{
    if (!m_bytesTotal)
        return;
    m_bytesDone += got;
    Tick(this);
}

// The port argument is never read: the URL takes m_port.  Its address is still
// taken -- the slot is allocated after msg[], so ADDRESSOF put it in the stack,
// and only the entry store survives once the null test folds away.
static inline int NotNull(void *p)
{
    return p != 0;
}

// 0x00585B30.  One file into its patch image, through Http_get::Get, with the
// tick, file-tick and estimate callbacks as it goes.  The four summed counters
// are re-based on the values saved before the fetch.
int PatcherAPI_Impl::DownloadFile(char *server, int port, char *path, char *file, int compressed,
                                  long downloadSize, long totalSize)
{
    VIPatchImage *images = m_api->FindPatchImage(0);
    if (!images)
        return -1;
    Http_get http(images);
    char url[256];
    char msg[512];
    int size = downloadSize + totalSize;
    int started = 0;
    int unzipping = 0;
    http.Initialize(m_x5EC);
    http.SetUrlListener(this);
    if (m_cancel)
        return 100;
    if (!path || !file)
    {
        m_api->OnError(60, "Null path requested for download.\n");
        return 0;
    }
    if (!NotNull(&port))
        return 0;
    if (path[0] == '\\' || path[0] == '/')
    {
        path[0] = '/';
        sprintf(url, "http://%s:%i%s", server, m_port, path);
    }
    else
        sprintf(url, "http://%s:%i/%s", server, m_port, path);
    int i;
    for (i = 0; i < (int)strlen(url); i++)
        if (url[i] == '\\')
            url[i] = '/';
    for (i = 0; i < (int)strlen(file); i++)
        if (file[i] == '\\')
            file[i] = '/';
    if (size)
    {
        started = 1;
        m_api->OnFile(file);
        m_api->OnSetFileTickCount(100);
    }
    unsigned int lastBytes;
    unsigned int total;
    unsigned int savedDone;
    unsigned int savedUnzipDone;
    unsigned int savedTimeDownloaded;
    unsigned int savedTimeUnzipped;
    unsigned int savedBytesDownloaded;
    unsigned int savedBytesUnzipped;
    int lastStatus;
    do
    {
        lastBytes = 0;
        lastStatus = 0;
        total = m_bytesTotal + m_unzipTotal;
        savedTimeDownloaded = m_timeDownloaded;
        savedTimeUnzipped = m_timeUnzipped;
        savedBytesDownloaded = m_bytesDownloaded;
        savedBytesUnzipped = m_bytesUnzipped;
        savedUnzipDone = m_unzipDone;
        savedDone = m_bytesDone;
    } while (http.Get(url, file, m_user, m_password, compressed));
    for (;;)
    {
        if (lastStatus != http.GetStatus() || lastBytes != http.Get_bytes_received())
        {
            lastStatus = http.GetStatus();
            time(&m_now);
            switch (lastStatus)
            {
            case 1:
            case 2:
            case 3:
                break;
            case 5:
            {
                char *text = http.GetErrorString();
                int type = http.GetErrorType();
                if (text)
                    FormatString(msg, "Error downloading %s: %s\n", file, text);
                else
                    FormatString(msg, "Error downloading %s\n", file);
                m_api->OnError(type, msg);
                if (type == 32)
                    m_api->Cancel(0);
                http.Close();
                m_bytesDone = savedDone;
                return type;
            }
            case 4:
                if (downloadSize <= 0)
                    break;
                if (lastBytes == http.GetBytesDownloaded())
                    break;
                lastBytes = http.GetBytesDownloaded();
                if (!started)
                    break;
                if (m_cancel)
                    break;
                m_timeDownloaded = savedTimeDownloaded + http.GetTimeDownloaded();
                m_timeUnzipped = savedTimeUnzipped + http.GetTimeUnzipped();
                m_bytesDownloaded = savedBytesDownloaded + http.GetBytesDownloaded();
                m_bytesUnzipped = savedBytesUnzipped + http.GetBytesUnzipped();
                m_bytesDone = savedDone + lastBytes;
                Tick(this);
                if (downloadSize > 100)
                    m_api->OnFileTick(lastBytes / (downloadSize / 100));
                else if (downloadSize > 0)
                    m_api->OnFileTick(lastBytes * 100 / downloadSize);
                if (m_now < m_lastEstimate + 1)
                    break;
                if (m_startTime + 6 < m_now)
                {
                    if (total)
                        m_api->OnEstimate(GetEstimate(), GetEstimateRemaining());
                    m_lastEstimate = m_now;
                }
                else
                    GetEstimate();
                break;
            case 7:
                if (!started)
                    break;
                if (!unzipping)
                    lastBytes = 0;
                if (lastBytes == http.GetBytesUnzipped())
                    break;
                lastBytes = http.GetBytesUnzipped();
                if (m_cancel)
                    break;
                m_timeDownloaded = savedTimeDownloaded + http.GetTimeDownloaded();
                m_timeUnzipped = savedTimeUnzipped + http.GetTimeUnzipped();
                m_bytesDownloaded = savedBytesDownloaded + http.GetBytesDownloaded();
                m_bytesUnzipped = savedBytesUnzipped + http.GetBytesUnzipped();
                m_unzipDone = savedUnzipDone + lastBytes;
                if (m_now == m_lastEstimate)
                    break;
                if (m_startTime + 5 < m_now)
                {
                    if (total)
                        m_api->OnEstimate(GetEstimate(), GetEstimateRemaining());
                    m_lastEstimate = m_now;
                }
                else
                    GetEstimate();
                break;
            case 6:
                if (started)
                {
                    m_bytesDone = savedDone + http.GetBytesDownloaded();
                    m_unzipDone = savedUnzipDone + http.GetBytesUnzipped();
                    if (total < m_bytesDone + m_unzipDone)
                        m_api->OnTick(100);
                    else
                        Tick(this);
                    m_api->OnFileTick(100);
                    m_api->OnFileFinished(file);
                    m_timeDownloaded = savedTimeDownloaded + http.GetTimeDownloaded();
                    m_timeUnzipped = savedTimeUnzipped + http.GetTimeUnzipped();
                    m_bytesDownloaded = savedBytesDownloaded + http.GetBytesDownloaded();
                    m_bytesUnzipped = savedBytesUnzipped + http.GetBytesUnzipped();
                }
                http.Close();
                return 0;
            case 8:
                if (started)
                {
                    m_api->OnTick(0);
                    m_api->OnFileTick(0);
                }
                m_api->OnError(http.GetErrorType(), http.GetErrorString());
                return 100;
            }
        }
        else
            Sleep(10);
        if (m_cancel && http.GetBytesDownloaded() > 100)
        {
            m_api->OnStatusInfo("Send aborted.\n");
            http.Close();
            while (http.Running())
                ;
            m_bytesDone = savedDone;
            m_api->OnTick(0);
            return 100;
        }
    }
}

// 0x005864F8.
int PatcherAPI_Impl::GetFile(const char *dist, const char *file, int x)
{
    PatcherDistributionInfo *d = PatcherDistributionInfo::GetDistributionByName(GetDistributionList(), dist);
    if (!d)
        d = GetDistributionList();
    PatcherDownloader *dl = d->m_impl->GetDownloaderByFile(file);
    if (dl)
    {
        dl->ScanFiles(0);
        dl->Download(0);
    }
    return 1;
}

// 0x00586598.
void PatcherAPI_Impl::Restart(bool now)
{
    Cancel();
}

// 0x005865B8.  A Win32 thread function's DWORD, which is 64 bits here.
unsigned long Patcher_thread(void *data)
{
    PatcherThreadData *td = (PatcherThreadData *)data;
    unsigned long result;
    td->m_impl->SetThreadInUse(1, 0);
    switch (td->m_op)
    {
    case 0:
        result = td->m_dist->Initialize(td->m_node, td->m_api);
        td->m_dist->CalculateHierarchicalProperties();
        td->m_impl->SetInitializing(0);
        break;
    case 4:
        result = td->m_impl->InitialScanRoutine(td->m_name);
        break;
    case 1:
        result = td->m_impl->DownloadRoutine(td->m_name, td->m_flags);
        break;
    case 3:
        result = td->m_impl->RescanRoutine(td->m_name, td->m_flags);
        break;
    case 2:
        result = td->m_impl->UseCRCRoutine(td->m_useCRC);
        break;
    }
    td->m_impl->SetThreadInUse(0, td->m_op);
    return result;
}

// 0x005866B8.
void PatcherAPI_Impl::SetThreadInUse(bool on, int op)
{
    m_threadInUse = on;
    if (!on)
    {
        switch (op)
        {
        case 0:
            m_api->OnStatusInfo("\n");
            break;
        case 1:
        case 2:
            break;
        case 3:
            m_api->OnStatusInfo("File Rescan Complete.\n");
            break;
        case 4:
            break;
        }
    }
    if (NeedsExit())
        exit(0);
}

// 0x00586760.
int PatcherAPI_Impl::NeedsExit()
{
    return m_needsExit;
}

// 0x00586768.
int PatcherAPI_Impl::SetCheckOnInitialize(bool on)
{
    m_checkOnInitialize = on;
    return 1;
}

// 0x00586778.
int PatcherAPI_Impl::CheckOnInitialize()
{
    return m_checkOnInitialize;
}

// 0x00586780.
int PatcherAPI_Impl::SetQuickScan(bool on)
{
    m_quickScan = on;
    return 1;
}

// 0x00586790.
int PatcherAPI_Impl::QuickScan()
{
    return m_quickScan;
}

// 0x00586798.
int PatcherAPI_Impl::IncrementScanCount()
{
    if (m_scanTotal > 0)
    {
        m_scanCount++;
        m_api->OnScanTick(m_scanCount * 100 / m_scanTotal);
        return 1;
    }
    return 0;
}

// 0x005867F8.
void PatcherAPI_Impl::SetPatchImage(VIPatchImage *images)
{
    if (images)
        m_images = images;
}

// 0x00586808.
VIPatchImage *PatcherAPI_Impl::FindPatchImage(const char *name)
{
    if (!name)
        return m_images;
    for (VIPatchImage *i = m_images; i; i = i->m_next)
        if (strstr(name, i->m_name))
            return i;
    return 0;
}
