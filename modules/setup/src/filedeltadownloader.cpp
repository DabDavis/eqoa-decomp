// FileDeltaDownloader.cpp (name ours) -- SETUP 0x0057FE40 .. 0x005800A8: one
// <DeltaFile> of a <File>.
#include "patcher.h"

// 0x0057FE40.
FileDeltaDownloader::FileDeltaDownloader()
{
    m_server = 0;
    m_port = 0;
    m_remotePath = 0;
    m_deltaPath = 0;
    m_localPath = 0;
    m_downloadSize = 0;
    m_totalSize = 0;
    m_crc = 0;
    m_next = 0;
}

// 0x0057FE70.
FileDeltaDownloader::~FileDeltaDownloader()
{
    if (m_next)
    {
        delete m_next;
        m_next = 0;
    }
    if (m_server)
    {
        free(m_server);
        m_server = 0;
    }
    if (m_remotePath)
    {
        free(m_remotePath);
        m_remotePath = 0;
    }
    if (m_deltaPath)
    {
        free(m_deltaPath);
        m_deltaPath = 0;
    }
    if (m_localPath)
    {
        free(m_localPath);
        m_localPath = 0;
    }
}

// 0x0057FF28.
int FileDeltaDownloader::Initialize(const char *server, int port, const char *remotePath,
                                    const char *deltaPath, const char *localPath, int downloadSize,
                                    int totalSize, int crc)
{
    m_port = port;
    if (server)
        m_server = strdup(server);
    if (remotePath)
        m_remotePath = strdup(remotePath);
    if (deltaPath)
        m_deltaPath = strdup(deltaPath);
    if (localPath)
        m_localPath = strdup(localPath);
    m_downloadSize = downloadSize;
    m_totalSize = totalSize;
    m_crc = crc;
    return 1;
}

// 0x0057FFF0.
int FileDeltaDownloader::Download(char *buf, int size, PatcherAPI *api)
{
    api->DownloadToMemory(buf, size, m_server, m_port, m_remotePath, 1);
    return 1;
}

// 0x00580028.
int FileDeltaDownloader::SetNext(FileDeltaDownloader *next)
{
    if (m_next)
    {
        delete m_next;
        m_next = 0;
    }
    if (next)
        m_next = next;
    return 1;
}

// 0x00580078.
FileDeltaDownloader *FileDeltaDownloader::GetNext()
{
    return m_next;
}

// 0x00580080.
int FileDeltaDownloader::Matches(unsigned int crc)
{
    return m_crc == crc;
}

// 0x00580090.
char *FileDeltaDownloader::GetDeltaFilePath()
{
    return m_deltaPath;
}

// 0x00580098.
int FileDeltaDownloader::GetDownloadSize()
{
    return m_downloadSize;
}

// 0x005800A0.
int FileDeltaDownloader::GetTotalSize()
{
    return m_totalSize;
}
