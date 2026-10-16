// PatcherDistributionInfo.cpp (name ours) -- SETUP 0x00586D00 .. 0x005873F0: the
// handle on one distribution of the manifest.
#include "patcher.h"

// 0x00586D00.
PatcherDistributionInfo::PatcherDistributionInfo()
{
    m_impl = new PatcherDistributionInfo_Impl;
}

// 0x00586D48.
PatcherDistributionInfo::~PatcherDistributionInfo()
{
    if (m_impl)
    {
        delete m_impl;
        m_impl = 0;
    }
}

// 0x00586DB8.
bool PatcherDistributionInfo::Initialize(_xmlNode *node, PatcherAPI *api)
{
    m_impl->SetDistributionInfo(this);
    return m_impl->Initialize(node, api);
}

// 0x00586e10.
int PatcherDistributionInfo::NeedDownload(int a)
{
    return m_impl->NeedDownload(a);
}

// 0x00586e30.
PatcherDistributionInfo * PatcherDistributionInfo::GetParent()
{
    return m_impl->GetParent();
}

// 0x00586e50.
PatcherDistributionInfo * PatcherDistributionInfo::GetChild()
{
    return m_impl->GetChild();
}

// 0x00586e70.
PatcherDistributionInfo * PatcherDistributionInfo::GetPrevious()
{
    return m_impl->GetPrevious();
}

// 0x00586e90.
PatcherDistributionInfo * PatcherDistributionInfo::GetNext()
{
    return m_impl->GetNext();
}

// 0x00586eb0.
PatcherAPI * PatcherDistributionInfo::GetPatcher()
{
    return m_impl->GetPatcher();
}

// 0x00586ed0.
char * PatcherDistributionInfo::GetName()
{
    return m_impl->GetName();
}

// 0x00586ef0.
char * PatcherDistributionInfo::GetTitle()
{
    return m_impl->GetTitle();
}

// 0x00586f10.
char * PatcherDistributionInfo::GetPath()
{
    return m_impl->GetPath();
}

// 0x00586f30.
char * PatcherDistributionInfo::GetServer()
{
    return m_impl->GetServer();
}

// 0x00586f50.
int PatcherDistributionInfo::GetPort()
{
    return m_impl->GetPort();
}

// 0x00586f70.
bool PatcherDistributionInfo::IsOptional()
{
    return m_impl->IsOptional();
}

// 0x00586f90.
bool PatcherDistributionInfo::IsSelected()
{
    return m_impl->IsSelected();
}

// 0x00586fb0.
bool PatcherDistributionInfo::Select(bool a)
{
    return m_impl->Select(a);
}

// 0x00586fd0.
int PatcherDistributionInfo::SelectChildren(bool a)
{
    return m_impl->SelectChildren(a);
}

// 0x00586ff0.
int PatcherDistributionInfo::IsRestricted()
{
    return m_impl->IsRestricted();
}

// 0x00587010.
char * PatcherDistributionInfo::GetRestrictedFilename()
{
    return m_impl->GetRestrictedFilename();
}

// 0x00587030.
int PatcherDistributionInfo::RestrictedFileExists()
{
    return m_impl->RestrictedFileExists();
}

// 0x00587050.
long PatcherDistributionInfo::GetTotalSize(int a)
{
    return m_impl->GetTotalSize(a);
}

// 0x00587070.
long PatcherDistributionInfo::GetDownloadSize(int a)
{
    return m_impl->GetDownloadSize(a);
}

// 0x00587090.
int PatcherDistributionInfo::GetFileCount(int a)
{
    return m_impl->GetFileCount(a);
}

// 0x005870b0.
int PatcherDistributionInfo::InitialScan()
{
    return m_impl->InitialScan();
}

// 0x005870d0.
int PatcherDistributionInfo::SetDistributionRoot(const char * a)
{
    return m_impl->SetDistributionRoot(a);
}

// 0x005870f0.
const char * PatcherDistributionInfo::GetDistributionRoot()
{
    return m_impl->GetDistributionRoot();
}

// 0x00587110.
int PatcherDistributionInfo::RescanFiles(int a)
{
    return m_impl->RescanFiles(a);
}

// 0x00587130.
int PatcherDistributionInfo::Download(int a)
{
    return m_impl->Download(a);
}

// 0x00587150.
int PatcherDistributionInfo::Continue()
{
    return m_impl->Continue();
}

// 0x00587170.
int PatcherDistributionInfo::DeleteOld()
{
    return m_impl->DeleteOld();
}

// 0x00587190.
int PatcherDistributionInfo::Uninstall()
{
    return m_impl->Uninstall();
}

// 0x005871b0.
int PatcherDistributionInfo::UseCRC(bool a)
{
    return m_impl->UseCRC(a);
}

// 0x005871d0.
int PatcherDistributionInfo::UsingCRC()
{
    return m_impl->UsingCRC();
}

// 0x005871f0.
int PatcherDistributionInfo::SetChanges(PatcherChanges * a)
{
    return m_impl->SetChanges(a);
}

// 0x00587210.
PatcherChanges * PatcherDistributionInfo::GetChanges()
{
    return m_impl->GetChanges();
}

// 0x00587230.
short PatcherDistributionInfo::GetVersion()
{
    return m_impl->GetVersion();
}

// 0x00587250.  Depth first: this node, its later siblings, then its children.
PatcherDistributionInfo *PatcherDistributionInfo::GetDistributionByName(PatcherDistributionInfo *info,
                                                                        const char *name)
{
    if (name && info)
    {
        if (!info->GetName())
            return 0;
        if (!strcasecmp(info->GetName(), name))
            return info;
        PatcherDistributionInfo *found;
        if (info->GetNext() && (found = GetDistributionByName(info->GetNext(), name)))
            return found;
        if (info->GetChild() && (found = GetDistributionByName(info->GetChild(), name)))
            return found;
    }
    return 0;
}

// 0x00587318.
void PatcherDistributionInfo::SetXtraDistribution(int a, bool b)
{
    if (m_impl)
        m_impl->SetXtraDistribution(a, b);
}

// 0x00587340.
int PatcherDistributionInfo::IsXtraDistribution(int a)
{
    if (m_impl)
        return m_impl->IsXtraDistribution(a);
    return 0;
}

// 0x00587368.
int PatcherDistributionInfo::CalculateHierarchicalProperties()
{
    return m_impl->CalculateHierarchicalProperties();
}

// 0x00587388.
void PatcherDistributionInfo::ResetScanFlag(int a)
{
    if (m_impl)
        m_impl->ResetScanFlag(a);
}

