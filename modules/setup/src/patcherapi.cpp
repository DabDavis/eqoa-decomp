// PatcherAPI.cpp (name ours) -- SETUP 0x005824D0 .. 0x00582F40: the handle the
// game holds.  Every call checks for the impl and forwards to it.
#include "patcher.h"

// 0x005824D0.
PatcherAPI::PatcherAPI()
{
    m_impl = new PatcherAPI_Impl;
}

// 0x00582518.  On the PC this removed the temporary files; here only the
// names are still built.
PatcherAPI::~PatcherAPI()
{
    char name[32];
    printf("patcherapi dest\n");
    if (m_impl)
    {
        while (m_impl->ThreadBusy())
            Sleep(10);
        if (m_impl)
            delete m_impl;
        m_impl = 0;
    }
    printf("patcherapi dest 2\n");
    for (int i = 0; i < 15; i++)
    {
        sprintf(name, "~VPatchTemp%d.xml", i);
        sprintf(name, "~VPatchTemp%d.xml.gz", i);
        sprintf(name, "~VPatchTemp%d.txt", i);
        sprintf(name, "~VPatchTemp%d.txt.gz", i);
    }
    printf("patcherapi dest 3\n");
}

// 0x00582670.
int PatcherAPI::InitializeFromLocalFile(const char *file, int flags)
{
    if (m_impl)
        return m_impl->InitializeFromLocalFile(file, this, flags);
    return 0;
}

// 0x005826B0.
int PatcherAPI::Initialize(const char *url, int port, const char *file, int flags)
{
    if (m_impl)
        return m_impl->Initialize(url, port, file, this, flags);
    return 0;
}

// 0x005826F8.
int PatcherAPI::Initialize(const char *url, int port, const char *file, const char *a,
                           const char *b, int flags)
{
    if (m_impl)
        return m_impl->Initialize(url, port, file, this, a, b, flags);
    return 0;
}

int PatcherAPI::NeedDownload(int a)
{
    if (m_impl)
        return m_impl->NeedDownload(a);
    return 0;
}

int PatcherAPI::DownloadAll(int flags)
{
    if (m_impl)
        return Download(0, flags);
    return 0;
}

int PatcherAPI::DeleteOldFiles()
{
    if (m_impl)
        return m_impl->DeleteOldFiles();
    return 0;
}

int PatcherAPI::Download(const char *a, int b)
{
    if (m_impl)
        return m_impl->Download(a, b);
    return 0;
}

int PatcherAPI::Uninstall(const char *a)
{
    if (m_impl)
        return m_impl->Uninstall(a);
    return 0;
}

int PatcherAPI::Select(const char *a, bool b)
{
    if (m_impl)
        return m_impl->Select(a, b);
    return 0;
}

PatcherDistributionInfo *PatcherAPI::GetDistributionList()
{
    if (m_impl)
        return m_impl->GetDistributionList();
    return 0;
}

// Stop the impl, tell the client, and with bit 1 wait for the thread to end.
int PatcherAPI::Cancel(int flags)
{
    if (m_impl)
        m_impl->Cancel();
    OnCancel();
    if (flags & 2)
        while (m_impl && m_impl->ThreadBusy())
            ;
    return 1;
}

int PatcherAPI::Continue()
{
    if (m_impl)
        return m_impl->Continue();
    return 0;
}

const char *PatcherAPI::GetProductName()
{
    if (m_impl)
        return m_impl->GetProductName();
    return 0;
}

short PatcherAPI::GetVersion()
{
    if (m_impl)
        return m_impl->GetVersion();
    return 0;
}

PatcherChanges *PatcherAPI::GetProductChanges()
{
    if (m_impl)
        return m_impl->GetProductChanges();
    return 0;
}

int PatcherAPI::SetRoot()
{
    if (m_impl)
        return m_impl->SetRoot();
    return 0;
}

int PatcherAPI::SetDistributionRoot(const char *a, const char *b)
{
    if (m_impl)
        return m_impl->SetDistributionRoot(a, b);
    return 0;
}

int PatcherAPI::RescanFiles(const char *a, int b)
{
    if (m_impl)
        return m_impl->RescanFiles(a, b);
    return 0;
}

int PatcherAPI::RescanTestFiles(const char *a, int b)
{
    if (m_impl)
        return m_impl->RescanFiles(a, b | 0x80);
    return 0;
}

int PatcherAPI::SetLocal()
{
    if (m_impl)
        return m_impl->SetLocal();
    return 0;
}

const char *PatcherAPI::GetRoot(const char *a)
{
    if (m_impl)
        return m_impl->GetRoot(a);
    return 0;
}

char *PatcherAPI::GetLocal()
{
    if (m_impl)
        return m_impl->GetLocal();
    return 0;
}

int PatcherAPI::UseCRC(bool a, int b)
{
    if (m_impl)
        return m_impl->UseCRC(a, b);
    return 0;
}

int PatcherAPI::UsingCRC()
{
    if (m_impl)
        return m_impl->UsingCRC();
    return 0;
}

long PatcherAPI::GetEstimate()
{
    if (m_impl)
        return m_impl->GetEstimate();
    return 0;
}

long PatcherAPI::GetEstimateRemaining()
{
    if (m_impl)
        return m_impl->GetEstimateRemaining();
    return 0;
}

int PatcherAPI::GetFileCount(const char *a, int b)
{
    if (m_impl)
        return m_impl->GetFileCount(a, b);
    return 0;
}

long PatcherAPI::GetTotalSize(const char *a, int b)
{
    if (m_impl)
        return m_impl->GetTotalSize(a, b);
    return 0;
}

long PatcherAPI::GetDownloadSize(const char *a, int b)
{
    if (m_impl)
        return m_impl->GetDownloadSize(a, b);
    return 0;
}

int PatcherAPI::OnForceRestart(const char *name, bool now)
{
    Restart(true);
    return 1;
}

int PatcherAPI::DownloadFile(char *a, int b, char *c, char *d, int e, long f, long g)
{
    if (m_impl)
        return m_impl->DownloadFile(a, b, c, d, e, f, g);
    return -1;
}

int PatcherAPI::DownloadToMemory(char *a, int b, char *c, int d, char *e, int f)
{
    if (m_impl)
        return m_impl->DownloadToMemory(a, b, c, d, e, f);
    return -1;
}

int PatcherAPI::GetFile(const char *a, const char *b, int c)
{
    if (m_impl)
        return m_impl->GetFile(a, b, c);
    return 0;
}

char *PatcherAPI::DownloadMemoryFile(int a, char *b, int c, char *d, int e)
{
    if (m_impl)
        return m_impl->DownloadMemoryFile(a, b, c, d, e);
    return 0;
}

void PatcherAPI::SetRestartName(const char *a, const char *b)
{
    if (m_impl)
        m_impl->SetRestartName(a, b);
}

void PatcherAPI::Restart(bool a)
{
    if (m_impl)
        m_impl->Restart(a);
}

void PatcherAPI::PatchPatcher(const char *a, const char *b)
{
    if (m_impl)
        m_impl->PatchPatcher(a, b);
}

int PatcherAPI::ValidUser(const char *user, const char *password)
{
    return 1;
}

int PatcherAPI::IsBusy()
{
    if (m_impl)
        return m_impl->ThreadBusy();
    return 0;
}

// A drive letter ("C:") is split off into its own small (and never freed) string.
int PatcherAPI::GenerateLocalPath(const char *path)
{
    char cwd[255];
    getcwd(cwd, 255);
    char *drive = (char *)malloc(4);
    drive[0] = 0;
    if (path[1] == ':')
    {
        drive[0] = path[0];
        drive[1] = ':';
        drive[2] = '\\';
        drive[3] = 0;
        chdir(drive);
    }
    return PatcherAPI_Impl::GenerateLocalSubPath(path, drive, cwd);
}

int PatcherAPI::SetCheckOnInitialize(bool a)
{
    if (m_impl)
        return m_impl->SetCheckOnInitialize(a);
    return 0;
}

int PatcherAPI::CheckOnInitialize()
{
    if (m_impl)
        return m_impl->CheckOnInitialize();
    return 0;
}

int PatcherAPI::SetQuickScan(bool a)
{
    if (m_impl)
        return m_impl->SetQuickScan(a);
    return 0;
}

int PatcherAPI::QuickScan()
{
    if (m_impl)
        return m_impl->QuickScan();
    return 0;
}

int PatcherAPI::IncrementScanCount()
{
    if (m_impl)
        return m_impl->IncrementScanCount();
    return 0;
}

int PatcherAPI::InitialScan(const char *a)
{
    if (m_impl)
        return m_impl->InitialScan(a);
    return 0;
}

int PatcherAPI::OnSetScanTickCount(int n)
{
    return 1;
}

int PatcherAPI::OnScanTick(int n)
{
    return 1;
}

void PatcherAPI::SetPatchImage(VIPatchImage *a)
{
    if (m_impl)
        m_impl->SetPatchImage(a);
}

// Falls off the end when there is no impl: v0 is whatever it was.
VIPatchImage *PatcherAPI::FindPatchImage(const char *a)
{
    if (m_impl)
        return m_impl->FindPatchImage(a);
}
