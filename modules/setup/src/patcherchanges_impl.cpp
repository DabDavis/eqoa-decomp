// PatcherChanges_Impl.cpp (name ours) -- SETUP 0x00586B10 .. 0x00586D00.
#include "patcher.h"

// 0x00586B10.
PatcherChanges_Impl::PatcherChanges_Impl()
{
    m_next = 0;
    m_prev = 0;
    m_text = 0;
    m_html = 0;
    m_version = 0;
}

// 0x00586B38.  The list owns what follows it.
PatcherChanges_Impl::~PatcherChanges_Impl()
{
    if (m_text)
    {
        free(m_text);
        m_text = 0;
    }
    if (m_html)
    {
        free(m_html);
        m_html = 0;
    }
    if (m_next)
    {
        delete m_next;
        m_next = 0;
    }
}

// 0x00586BD8.
int PatcherChanges_Impl::Initialize(int version, char *html, PatcherChanges *prev,
                                    PatcherChanges *next)
{
    m_version = version;
    m_html = strdup(html);
    ExtractTextChangesFromHTMLChanges();
    SetPrevious(prev);
    SetNext(next);
    return 1;
}

// 0x00586C48.
short PatcherChanges_Impl::GetVersion()
{
    return m_version;
}

// 0x00586C50.
char *PatcherChanges_Impl::GetTextChanges()
{
    return m_text;
}

// 0x00586C58.
char *PatcherChanges_Impl::GetHtmlChanges()
{
    return m_html;
}

// 0x00586C60.  The PS2 keeps the HTML as it is.
int PatcherChanges_Impl::ExtractTextChangesFromHTMLChanges()
{
    m_text = strdup(m_html);
    return 1;
}

// 0x00586C90.
PatcherChanges *PatcherChanges_Impl::GetPrevious()
{
    return m_prev;
}

// 0x00586C98.
PatcherChanges *PatcherChanges_Impl::GetNext()
{
    return m_next;
}

// 0x00586CA0.
int PatcherChanges_Impl::SetPrevious(PatcherChanges *p)
{
    m_prev = p;
    return 1;
}

// 0x00586CB0.
int PatcherChanges_Impl::SetNext(PatcherChanges *p)
{
    m_next = p;
    return 1;
}
