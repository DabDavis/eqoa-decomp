// PatcherDistributionInfo_Impl.cpp (name ours) -- SETUP 0x005873F0 .. 0x005880F0.
// A distribution's attributes out of the manifest, and the DistributionDownloader
// that does its work.
#include "patcher.h"

// 0x005873F0.
PatcherDistributionInfo_Impl::PatcherDistributionInfo_Impl()
{
    m_server = 0;
    m_path = 0;
    m_downloader = 0;
    m_info = 0;
    m_name = 0;
    m_title = 0;
    m_optional = 0;
    m_restricted = 0;
    m_restrictedFile = 0;
    m_restrictedFileExists = 0;
    m_selected = 0;
    m_x38 = 0;
    m_x30 = 0;
    m_version = 0;
    m_api = 0;
    m_changes = 0;
}

// 0x00587448.
PatcherDistributionInfo_Impl::~PatcherDistributionInfo_Impl()
{
    if (m_server)
    {
        free(m_server);
        m_server = 0;
    }
    if (m_path)
    {
        free(m_path);
        m_path = 0;
    }
    if (m_name)
    {
        free(m_name);
        m_name = 0;
    }
    if (m_title)
    {
        free(m_title);
        m_title = 0;
    }
    if (m_restrictedFile)
    {
        free(m_restrictedFile);
        m_restrictedFile = 0;
    }
    if (m_changes)
    {
        delete m_changes;
        m_changes = 0;
    }
    if (m_downloader)
    {
        if (m_downloader->IsTopLevel())
        {
            delete m_downloader;
            m_downloader = 0;
        }
        else
            m_downloader = 0;
    }
}

// 0x00587570.
bool PatcherDistributionInfo_Impl::Initialize(_xmlNode *node, PatcherAPI *api)
{
    m_api = api;
    m_downloader = new DistributionDownloader;
    m_downloader->SetDistributionInfo(GetDistributionInfo());
    SetXMLInfo(node);
    if (node)
    {
        m_downloader->Initialize(node, 0, 0, 0, 0);
        m_downloader->SetTopLevel(1);
    }
    return 1;
}

// 0x00587640.  The api lives on the first distribution of the top level.  The
// second call of each pair is a tail call, which gcc turns into a jump.
PatcherAPI *PatcherDistributionInfo_Impl::FindPatcherAPI()
{
    if (m_api)
        return m_api;
    PatcherDistributionInfo *prev = GetPrevious();
    PatcherDistributionInfo *parent = GetParent();
    if (prev)
    {
        if (prev->m_impl->FindPatcherAPI())
            return prev->m_impl->FindPatcherAPI();
    }
    else if (parent)
    {
        if (parent->m_impl->FindPatcherAPI())
            return parent->m_impl->FindPatcherAPI();
    }
    return 0;
}

// 0x005876C8.
int PatcherDistributionInfo_Impl::CheckPatcherAPI()
{
    m_api = FindPatcherAPI();
    return m_api != 0;
}

// 0x005876F8.
int PatcherDistributionInfo_Impl::NeedDownload(int flags)
{
    DistributionDownloader *d = GetDistributionDownloader();
    if (d)
        return d->NeedDownload(flags);
    return 0;
}

// 0x00587738.
int PatcherDistributionInfo_Impl::NeedDelete()
{
    DistributionDownloader *d = GetDistributionDownloader();
    if (d)
        return d->NeedDelete();
    return 0;
}

// 0x00587770.
PatcherDistributionInfo *PatcherDistributionInfo_Impl::GetParent()
{
    PatcherDownloader *d = GetDistributionDownloader()->GetParent();
    if (d && d->IsDistributionDownloader())
        return d->GetDistributionInfo();
    return 0;
}

// 0x005877D8.
PatcherDistributionInfo *PatcherDistributionInfo_Impl::GetChild()
{
    PatcherDownloader *d = GetDistributionDownloader()->GetChild();
    if (d && d->IsDistributionDownloader())
        return d->GetDistributionInfo();
    return 0;
}

// 0x00587840.
PatcherDistributionInfo *PatcherDistributionInfo_Impl::GetPrevious()
{
    PatcherDownloader *d = GetDistributionDownloader()->GetPrevious();
    if (d && d->IsDistributionDownloader())
        return d->GetDistributionInfo();
    return 0;
}

// 0x005878A8.
PatcherDistributionInfo *PatcherDistributionInfo_Impl::GetNext()
{
    PatcherDownloader *d = GetDistributionDownloader()->GetNext();
    if (d && d->IsDistributionDownloader())
        return d->GetDistributionInfo();
    return 0;
}

// 0x00587910.
PatcherAPI *PatcherDistributionInfo_Impl::GetPatcher()
{
    return m_api;
}

// 0x00587918.
char *PatcherDistributionInfo_Impl::GetName()
{
    return m_name;
}

// 0x00587920.
char *PatcherDistributionInfo_Impl::GetTitle()
{
    return m_title;
}

// 0x00587928.
char *PatcherDistributionInfo_Impl::GetPath()
{
    return m_path;
}

// 0x00587930.
char *PatcherDistributionInfo_Impl::GetServer()
{
    return m_server;
}

// 0x00587938.
int PatcherDistributionInfo_Impl::GetPort()
{
    return m_port;
}

// 0x00587940.
bool PatcherDistributionInfo_Impl::IsOptional()
{
    return m_optional;
}

// 0x00587948.
bool PatcherDistributionInfo_Impl::IsSelected()
{
    return m_selected;
}

// 0x00587950.
bool PatcherDistributionInfo_Impl::Select(bool on)
{
    m_selected = on;
    return 1;
}

// 0x00587960.
int PatcherDistributionInfo_Impl::SelectChildren(bool on)
{
    for (PatcherDistributionInfo *c = GetChild(); c; c = c->GetNext())
    {
        c->Select(on);
        c->SelectChildren(on);
    }
    return 1;
}

// 0x005879C8.
int PatcherDistributionInfo_Impl::IsRestricted()
{
    return m_restricted;
}

// 0x005879D0.
char *PatcherDistributionInfo_Impl::GetRestrictedFilename()
{
    return m_restrictedFile;
}

// 0x005879D8.
int PatcherDistributionInfo_Impl::RestrictedFileExists()
{
    return m_restrictedFileExists;
}

// 0x005879E0.
long PatcherDistributionInfo_Impl::GetTotalSize(int flags)
{
    long r = 0;
    if (m_downloader)
        r = m_downloader->GetTotalSize(flags);
    return r;
}

// 0x00587A18.
long PatcherDistributionInfo_Impl::GetDownloadSize(int flags)
{
    long r = 0;
    if (m_downloader)
        r = m_downloader->GetDownloadSize(flags);
    return r;
}

// 0x00587A50.
int PatcherDistributionInfo_Impl::GetFileCount(int flags)
{
    int r = 0;
    if (m_downloader)
        r = m_downloader->GetFileCount(flags);
    return r;
}

// 0x00587A88.
int PatcherDistributionInfo_Impl::InitialScan()
{
    if (m_downloader)
        m_downloader->ScanFiles(0x60);
    return 1;
}

// 0x00587AC0.
int PatcherDistributionInfo_Impl::SetDistributionRoot(const char *root)
{
    char cwd[256];
    getcwd(cwd, 255);
    if (root && m_downloader)
    {
        PatcherAPI::GenerateLocalPath(root);
        m_downloader->SetDistributionRoot(root);
        m_downloader->CalculateHierarchicalProperties();
        chdir(cwd);
    }
    return 1;
}

// 0x00587B48.
const char *PatcherDistributionInfo_Impl::GetDistributionRoot()
{
    if (m_downloader)
        return m_downloader->GetDistributionRoot();
    return 0;
}

// 0x00587B80.
int PatcherDistributionInfo_Impl::RescanFiles(int flags)
{
    if (m_downloader)
    {
        m_downloader->ScanFiles(flags);
        m_downloader->CalculateHierarchicalProperties();
    }
    return 1;
}

// 0x00587BD0.
int PatcherDistributionInfo_Impl::Download(int flags)
{
    char cwd[256];
    getcwd(cwd, 255);
    if (m_api)
        m_api->SetRoot();
    int failed = !GetDistributionDownloader()->Download(flags);
    chdir(cwd);
    return failed;
}

// 0x00587C50.
int PatcherDistributionInfo_Impl::Continue()
{
    if (m_downloader && !m_downloader->Download(0))
        return 1;
    return 0;
}

// 0x00587C98.
int PatcherDistributionInfo_Impl::DeleteOld()
{
    char cwd[256];
    getcwd(cwd, 255);
    if (m_api)
        m_api->SetRoot();
    if (m_downloader)
        m_downloader->Delete();
    chdir(cwd);
    return 1;
}

// 0x00587D08.
int PatcherDistributionInfo_Impl::Uninstall()
{
    char cwd[256];
    getcwd(cwd, 255);
    if (m_api)
        m_api->SetRoot();
    if (m_downloader)
    {
        m_downloader->PrepForUninstall();
        m_downloader->Delete();
    }
    chdir(cwd);
    return 1;
}

// 0x00587D90.
int PatcherDistributionInfo_Impl::UseCRC(bool on)
{
    if (m_downloader)
        return m_downloader->UseCRC(on);
    return 0;
}

// 0x00587DC8.
int PatcherDistributionInfo_Impl::UsingCRC()
{
    if (m_downloader)
        return m_downloader->UsingCRC();
    return 0;
}

// 0x00587E00.
int PatcherDistributionInfo_Impl::SetChanges(PatcherChanges *changes)
{
    m_changes = changes;
    return 1;
}

// 0x00587E10.
PatcherChanges *PatcherDistributionInfo_Impl::GetChanges()
{
    return m_changes;
}

// 0x00587E18.
DistributionDownloader *PatcherDistributionInfo_Impl::GetDistributionDownloader()
{
    return m_downloader;
}

// 0x00587E20.
int PatcherDistributionInfo_Impl::SetDistributionDownloader(DistributionDownloader *d)
{
    m_downloader = d;
    return 1;
}

// 0x00587E30.
int PatcherDistributionInfo_Impl::SetDistributionInfo(PatcherDistributionInfo *info)
{
    m_info = info;
    return 1;
}

// 0x00587E40.
short PatcherDistributionInfo_Impl::GetVersion()
{
    return m_version;
}

// 0x00587E48.  An attribute already set is kept; a child inherits Restricted and
// Optional from its parent when it does not set them.
int PatcherDistributionInfo_Impl::SetXMLInfo(_xmlNode *node)
{
    if (!m_name)
        m_name = PullXMLAttribute(node, "Name");
    if (!m_title)
        m_title = PullXMLAttribute(node, "Title");
    if (!m_path)
        m_path = PullXMLAttribute(node, "RemotePath");
    if (!m_server)
        m_server = PullXMLAttribute(node, "Server");
    if (!m_port)
        m_port = PullXMLAttributeLong(node, "Port");
    m_optional = PullXMLAttributeBool(node, "Optional");
    m_restricted = PullXMLAttributeBool(node, "Restricted");
    if (!m_restrictedFile)
        m_restrictedFile = PullXMLAttribute(node, "RestrictedFile");
    m_restrictedFileExists = 1;
    m_selected = !m_optional;
    if (m_restricted)
    {
        FILE *f;
        if (!strlen(m_restrictedFile) || !(f = fopen(m_restrictedFile, "r")))
        {
            m_restrictedFileExists = 0;
            m_selected = 0;
        }
        else
            fclose(f);
    }
    if (GetParent())
    {
        if (!m_restricted)
        {
            m_restricted = GetParent()->IsRestricted();
            m_restrictedFileExists = GetParent()->RestrictedFileExists();
        }
        if (!m_optional)
            m_optional = GetParent()->IsOptional();
    }
    return 1;
}

// 0x00588010.
PatcherDistributionInfo *PatcherDistributionInfo_Impl::GetDistributionInfo()
{
    return m_info;
}

// 0x00588018.
void PatcherDistributionInfo_Impl::SetXtraDistribution(int which, bool on)
{
    if (m_downloader)
        m_downloader->SetXtraDistribution(which, on);
}

// 0x00588040.
int PatcherDistributionInfo_Impl::IsXtraDistribution(int which)
{
    if (m_downloader)
        return m_downloader->IsXtraDistribution(which);
    return 0;
}

// 0x00588068.
int PatcherDistributionInfo_Impl::CalculateHierarchicalProperties()
{
    if (m_downloader)
        return m_downloader->CalculateHierarchicalProperties();
    return 0;
}

// 0x00588090.
PatcherDownloader *PatcherDistributionInfo_Impl::GetDownloaderByFile(const char *file)
{
    if (m_downloader)
        return m_downloader->GetDownloaderByFile(file);
    return 0;
}

// 0x005880B8.
void PatcherDistributionInfo_Impl::ResetScanFlag(int flags)
{
    if (m_downloader)
        m_downloader->ResetScanFlag(flags);
}
