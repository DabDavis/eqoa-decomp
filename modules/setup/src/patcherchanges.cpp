// PatcherChanges.cpp (name ours) -- SETUP 0x00586900 .. 0x00586B10: the release
// notes handle.  Unlike PatcherAPI it does not check its impl.
#include "patcher.h"

// 0x00586900.
PatcherChanges::PatcherChanges()
{
    m_impl = new PatcherChanges_Impl;
    SetPrevious(0);
    SetNext(0);
}

// 0x00586960.
PatcherChanges::~PatcherChanges()
{
    if (m_impl)
    {
        delete m_impl;
        m_impl = 0;
    }
}

// 0x005869d0.
int PatcherChanges::Initialize(int a, char *b, PatcherChanges *c, PatcherChanges *d)
{
    return m_impl->Initialize(a, b, c, d);
}

// 0x005869f0.
PatcherChanges * PatcherChanges::GetPrevious()
{
    return m_impl->GetPrevious();
}

// 0x00586a10.
PatcherChanges * PatcherChanges::GetNext()
{
    return m_impl->GetNext();
}

// 0x00586a30.
int PatcherChanges::SetPrevious(PatcherChanges * a)
{
    return m_impl->SetPrevious(a);
}

// 0x00586a50.
int PatcherChanges::SetNext(PatcherChanges * a)
{
    return m_impl->SetNext(a);
}

// 0x00586a70.
short PatcherChanges::GetVersion()
{
    return m_impl->GetVersion();
}

// 0x00586a90.
char * PatcherChanges::GetTextChanges()
{
    return m_impl->GetTextChanges();
}

// 0x00586ab0.
char * PatcherChanges::GetHtmlChanges()
{
    return m_impl->GetHtmlChanges();
}

