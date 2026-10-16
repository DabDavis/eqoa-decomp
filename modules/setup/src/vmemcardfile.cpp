// vmemcardfile.cpp (name ours) -- SETUP 0x00589828 .. 0x00589B50: a file on the
// memory card, each sceMc call followed by its sceMcSync.
#include "patcher.h"

// 0x00589828.
VMemCardFile::VMemCardFile(const char *name, int mode)
{
    m_name = 0;
    m_fd = -1;
    m_mode = mode;
    if (name)
        m_name = strdup(name);
}

// 0x00589870.
VMemCardFile::~VMemCardFile()
{
    if (m_name)
        free(m_name);
}

// 0x005898C0.  Every directory on the path but the last component, from the
// card's root.
int VMemCardFile::Mkdirhier(const char *path)
{
    char cwd[1024];
    char buf[2048];
    int result = 0;
    sprintf(cwd, "/");
    sceMcChdir(0, 0, "/", cwd);
    while (!sceMcSync(1, 0, &result))
        ;
    sprintf(buf, path);
    char *tok = strtok(buf, "/");
    while (tok)
    {
        char *dir = tok;
        tok = strtok(0, "/");
        if (tok)
        {
            sceMcMkdir(0, 0, dir);
            while (!sceMcSync(1, 0, &result))
                ;
        }
    }
    return 0;
}

// 0x005899D0.
int VMemCardFile::Open()
{
    InitMemCard();
    Mkdirhier(m_name);
    sceMcOpen(0, 0, m_name, m_mode);
    sceMcSync(0, 0, &m_fd);
    return 1;
}

// 0x00589A28.
int VMemCardFile::Close()
{
    if (m_fd >= 0)
    {
        int result = 0;
        sceMcClose(m_fd);
        sceMcSync(0, 0, &result);
    }
    return 1;
}

// 0x00589A68.
int VMemCardFile::InitMemCard()
{
    VIMemCardPrepare(0);
    return 0;
}

// 0x00589A88.
int VMemCardFile::Write(char *buf, int size)
{
    if (m_fd < 0)
        return -1;
    int result = 0;
    sceMcWrite(m_fd, buf, size);
    sceMcSync(0, 0, &result);
    return result;
}

// 0x00589AC8.
int VMemCardFile::Read(char *buf, int size)
{
    if (m_fd < 0)
        return -1;
    int result = 0;
    sceMcRead(m_fd, buf, size);
    sceMcSync(0, 0, &result);
    return result;
}

// 0x00589B08.  The size is where a seek to the end lands.
int VMemCardFile::GetSize()
{
    if (m_fd < 0)
        return -1;
    int result = 0;
    sceMcSeek(m_fd, 0, 2);
    sceMcSync(0, 0, &result);
    return result;
}
