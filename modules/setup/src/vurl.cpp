// vurl.cpp (name ours) -- SETUP 0x005DC568 .. 0x005DD9C0: the HTTP/1.x client over libinet
// that every download ends in, and STATION's too.  Blocking, except GetPoll; the
// listener hears each chunk as it lands.
#include "vurl.h"

// 0x005DC568.
VUrl::VUrl(const char *url)
{
    Initialize();
    ParseURL(url);
}

// 0x005DC5A8.
VUrl::~VUrl()
{
    if (m_server)
        free(m_server);
    if (m_path)
        free(m_path);
    if (m_netBuf)
    {
        free(m_netBuf);
        sceLibnetTerminate(&m_client);
    }
}

// 0x005DC628.
VUrl::VUrl(int protocol, const char *server, const char *path)
{
    Initialize();
    SetProtocol(protocol);
    SetServer(server);
    SetRelativePath(path);
}

// 0x005DC698.
void VUrl::Initialize()
{
    m_listener = 0;
    m_server = 0;
    m_path = 0;
    m_error = 0;
    m_bytes = 0;
    m_protocol = -1;
    m_port = 80;
    sprintf(m_errorText, "No error");
    SetThreadPriority(100);
    m_netBuf = (unsigned int *)memalign(64, 0x10200);
    sceLibnetInitialize(&m_client, 2048, 8192, 32);
}

// 0x005DC720.
void VUrl::SetUrlListener(VIUrlListener *l)
{
    m_listener = l;
}

// 0x005DC728.
void VUrl::SetPort(int port)
{
    m_port = port;
}

// 0x005DC730.
int VUrl::GetError()
{
    return m_error;
}

// 0x005DC738.
char *VUrl::GetErrorString()
{
    return m_errorText;
}

// 0x005DC740.
void VUrl::SetThreadPriority(int p)
{
    m_priority = p;
}

// 0x005DC748.  http only.
void VUrl::SetProtocol(int p)
{
    if (p)
    {
        m_error = 1;
        sprintf(m_errorText, "Unsupported protocol %i", p);
        m_protocol = -1;
        PrintError();
    }
    else
        m_protocol = 0;
}

// 0x005DC7A8.
void VUrl::SetServer(const char *server)
{
    if (!server)
    {
        m_error = 3;
        sprintf(m_errorText, "Cannot set server to NULL");
        PrintError();
        return;
    }
    if (m_server)
        free(m_server);
    m_server = strdup(server);
}

// 0x005DC820.
void VUrl::SetRelativePath(const char *path)
{
    if (!path)
    {
        m_error = 4;
        sprintf(m_errorText, "Cannot set path to NULL");
        PrintError();
        return;
    }
    if (m_path)
        free(m_path);
    m_path = strdup(path);
}

// 0x005DC898.  "http://server[:port]/path[#...]", the path's blanks as %20.
void VUrl::ParseURL(const char *url)
{
    char *copy = strdup(url);
    const char *sep = "/";
    char *tok = strtok(copy, sep);
    if (tok && !strcmp(tok, "http:"))
        SetProtocol(0);
    else
    {
        SetProtocol(-1);
        free(copy);
        return;
    }
    tok = strtok(0, sep);
    if (tok)
    {
        char *colon = strstr(tok, ":");
        if (!colon)
            SetServer(tok);
        else
        {
            *colon = 0;
            SetServer(tok);
            SetPort(atoi(colon + 1));
        }
    }
    else
    {
        SetServer(0);
        free(copy);
        return;
    }
    tok = strtok(0, "#");
    if (tok)
    {
        int size = strlen(tok) * 3;
        char *path = (char *)malloc(size);
        memset(path, 0, size);
        int j = 0;
        for (int i = 0; tok[i]; i++, j++)
        {
            if (tok[i] != ' ')
                path[j] = tok[i];
            else
            {
                path[j++] = '%';
                path[j++] = '2';
                path[j] = '0';
            }
        }
        printf("%s\n", path);
        SetRelativePath(path);
        free(path);
    }
    else
        SetRelativePath("/");
    free(copy);
}

// 0x005DCAA0.
int VUrl::OpenSocket()
{
    sceInetParam param;
    memset(&param, 0, sizeof(param));
    param.type = 1;
    param.local_port = -1;
    if (m_server)
    {
        if (!sceInetName2Address(&m_client, m_netBuf, 0, &param.remote_addr, m_server, -1, -1))
        {
            param.remote_port = m_port;
            m_cid = sceInetCreate(&m_client, m_netBuf, &param);
            if (sceInetOpen(&m_client, m_netBuf, m_cid, 10000))
            {
                m_error = 5;
                sprintf(m_errorText, "Could not connect to %s", m_server);
                PrintError();
                return 0;
            }
            return 1;
        }
        else
        {
            m_error = 3;
            sprintf(m_errorText, "Cannot resolve server %s", m_server);
            PrintError();
            return 0;
        }
    }
    else
    {
        m_error = 3;
        sprintf(m_errorText, "Cannot connect to NULL server");
        PrintError();
        return 0;
    }
}

// 0x005DCBB8.  The whole body into buf; 1 only when Content-Length was given
// and met.
int VUrl::Get(char *buf, int size)
{
    char header[1024];
    int sendFlags;
    int recvFlags;
    char *req = (char *)malloc(1024);
    sendFlags = 0;
    recvFlags = 0;
    if (!m_path)
    {
        m_error = 4;
        sprintf(m_errorText, "Cannot get NULL path");
        PrintError();
        free(req);
        return 0;
    }
    if (!OpenSocket())
    {
        free(req);
        return 0;
    }
    memset(buf, 0, size);
    int total = 0;
    memset(req, 0, 1024);
    int headerDone = 0;
    sprintf(req, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", m_path, m_server);
    int lastChar = 0;
    int parsed = 0;
    int headerLen = 0;
    int contentLength = -1;
    sceInetSend(&m_client, m_netBuf, m_cid, req, strlen(req), &sendFlags, -1);
    free(req);
    char *data = (char *)malloc(1460);
    memset(data, 0, 1460);
    memset(header, 0, 1024);
    while (!(recvFlags & 4))
    {
        int i = 0;
        int n = sceInetRecv(&m_client, m_netBuf, m_cid, data, 1460, &recvFlags, 100);
        if (n > 0)
        {
            char *dest = buf + total;
            if (!headerDone)
            {
                for (; i < n; i++)
                {
                    if (data[i] == '\r' && lastChar == '\n')
                    {
                        i++;
                        headerDone = 1;
                        break;
                    }
                    lastChar = data[i];
                    header[headerLen++] = data[i];
                }
            }
            if (headerDone && !parsed)
            {
                parsed = 1;
                for (char *line = strtok(header, "\r\n"); line; line = strtok(0, "\r\n"))
                    if (strstr(line, "Content-Length"))
                        contentLength = atoi(strstr(line, ":") + 2);
            }
            int start = 0;
            if (i)
            {
                start = i + 1;
                n -= start;
            }
            if (m_listener)
                m_listener->OnBytesDownloaded(m_server, m_path, n, total + n);
            memcpy(dest, data + start, n);
            total += n;
            if (contentLength != -1 && total >= contentLength)
                break;
        }
        else
        {
            if (n < 0)
            {
                printf("Error, code = %i\n", n);
                free(data);
                return 0;
            }
            if (m_listener)
                m_listener->OnBytesDownloaded(m_server, m_path, n, total + n);
        }
    }
    printf("!!DONE DOWNLOADING\n");
    m_bytes = total;
    sceInetClose(&m_client, m_netBuf, m_cid, -1);
    free(data);
    if (contentLength == -1 || total < contentLength)
        return 0;
    return 1;
}

// GetPoll's state, kept between calls: 1 start, 2 receiving, 3 done, 4 failed.
static int sState = 3;
static int sPos = 0;
static int sTotal = 0;
static int sHeaderDone = 0;
static char sLastChar = 0;
static int sN = 0;
static int sParsed = 0;
static int sHeaderLen = 0;
static int sContentLength = -1;
static int sUnused = 0;
static char sHeader[1024];
static char sData[1460];

// 0x005DCFC0 (name ours; not in the 2002 betas).  Get, one receive per call:
// called again until it returns 3 or 4.
int VUrl::GetPoll(char *buf, int size)
{
    int sendFlags;
    int recvFlags;
    if (sState == 3 || sState == 4)
        sState = 1;
    sendFlags = 0;
    recvFlags = 0;
    if (sState == 1)
    {
        char *req = (char *)malloc(1024);
        if (!m_path)
        {
            m_error = 4;
            sprintf(m_errorText, "Cannot get NULL path");
            PrintError();
            free(req);
            return 4;
        }
        if (!OpenSocket())
        {
            free(req);
            return 4;
        }
        memset(buf, 0, size);
        memset(req, 0, 1024);
        sprintf(req, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", m_path, m_server);
        int len = strlen(req);
        sceInetSend(&m_client, m_netBuf, m_cid, req, len, &sendFlags, -1);
        free(req);
        sState = 2;
        memset(sHeader, 0, 1024);
        memset(sData, 0, 1460);
        sPos = 0;
        sTotal = 0;
        sHeaderDone = 0;
        sLastChar = 0;
        sN = 0;
        sParsed = 0;
        sHeaderLen = 0;
        sContentLength = -1;
        sUnused = 0;
    }
    sN = sceInetRecv(&m_client, m_netBuf, m_cid, sData, 1460, &recvFlags, 10);
    sPos = 0;
    if (sN > 0)
    {
        if (!sHeaderDone)
        {
            for (; sPos < sN; sPos++)
            {
                if (sData[sPos] == '\r' && sLastChar == '\n')
                {
                    sPos++;
                    sHeaderDone = 1;
                    break;
                }
                sLastChar = sData[sPos];
                sHeader[sHeaderLen] = sData[sPos];
                sHeaderLen++;
            }
        }
        if (sHeaderDone && !sParsed)
        {
            sParsed = 1;
            for (char *line = strtok(sHeader, "\r\n"); line; line = strtok(0, "\r\n"))
                if (strstr(line, "Content-Length"))
                    sContentLength = atoi(strstr(line, ":") + 2);
        }
        int start = 0;
        if (sPos)
        {
            start = sPos + 1;
            sN -= start;
        }
        if (m_listener)
            m_listener->OnBytesDownloaded(m_server, m_path, sN, sTotal + sN);
        memcpy(buf + sTotal, sData + start, sN);
        sTotal += sN;
        printf("total = %i; contentSize = %i\n", sTotal, sContentLength);
        if (sContentLength != -1 && sTotal >= sContentLength)
            sState = 3;
    }
    else
    {
        if (sN < 0)
        {
            printf("Error, code = %i\n", sN);
            return 4;
        }
        if (m_listener)
            m_listener->OnBytesDownloaded(m_server, m_path, sN, sTotal + sN);
    }
    if (recvFlags & 4)
        sState = 3;
    if (sState == 3)
    {
        printf("!!DONE DOWNLOADING\n");
        m_bytes = sTotal;
        sceInetClose(&m_client, m_netBuf, m_cid, -1);
        if (sContentLength == -1 || sTotal < sContentLength)
            return 4;
    }
    return sState;
}

// 0x005DD498.  Every directory on the path but the last, from the card's root.
int VUrl::Mkdirhier(const char *path)
{
    char buf[1024];
    int result = 0;
    sceMcChdir(0, 0, "/", 0);
    sceMcSync(0, 0, &result);
    sprintf(buf, path);
    char *tok = strtok(buf, "/");
    while (tok)
    {
        char *dir = tok;
        tok = strtok(0, "/");
        if (tok)
        {
                sceMcSync(0, 0, &result);
                sceMcMkdir(0, 0, dir);
                sceMcSync(0, 0, &result);
                sceMcChdir(0, 0, dir, 0);
        }
    }
    return 1;
}

// 0x005DD590.
int VUrl::OpenMemCardFile(const char *path)
{
    printf("!!Opening mem card file %s\n", path);
    InitMemCard();
    Mkdirhier(path);
    sceMcOpen(0, 0, path, 0x202);
    sceMcSync(0, 0, &m_fd);
    if (m_fd < 0)
        return -1;
    return 1;
}

// 0x005DD618.
int VUrl::CloseMemCardFile()
{
    if (m_fd >= 0)
        sceMcClose(m_fd);
    return 1;
}

// 0x005DD648.  A PS2 card, formatted if it is not.  The free-space test reads
// the C library's free, not the count sceMcGetInfo wrote.
int VUrl::InitMemCard()
{
    int type;
    int freeSpace;
    int format;
    int result = sceMcInit();
    if (result)
        return -1;
    result = sceMcGetInfo(0, 0, &type, &freeSpace, &format);
    sceMcSync(0, 0, &result);
    if (result < -9 || type != 2 || !free)
        return -1;
    if (!format)
        sceMcFormat(0, 0);
    sceMcSync(0, 0, &result);
    return 1;
}

// 0x005DD700.
int VUrl::WriteToMemCard(char *buf, int size)
{
    if (m_fd < 0)
        return -1;
    sceMcWrite(m_fd, buf, size);
    return 1;
}

// 0x005DD730.  Straight to the card, header skipped.
int VUrl::GetToMemoryCard(char *path)
{
    char req[1024];
    char data[1460];
    int sendFlags;
    int recvFlags;
    sendFlags = 0;
    recvFlags = 0;
    if (!m_path)
    {
        m_error = 4;
        sprintf(m_errorText, "Cannot get NULL path");
        PrintError();
        return 0;
    }
    int r = OpenMemCardFile(path);
    if (r < 0)
        return r;
    if (!OpenSocket())
        return 0;
    memset(req, 0, 1024);
    int skip = 0;
    strcat(req, "GET /");
    int headerDone = 0;
    strcat(req, m_path);
    char lastChar = 0;
    strcat(req, " HTTP/1.0\r\n\r\n");
    sceInetSend(&m_client, m_netBuf, m_cid, req, strlen(req), &sendFlags, -1);
    memset(data, 0, 1460);
    int total = sceInetRecv(&m_client, m_netBuf, m_cid, data, 1460, &recvFlags, 0);
    while (!(recvFlags & 4))
    {
        skip = 0;
        int n = sceInetRecv(&m_client, m_netBuf, m_cid, data + total, 1460, &recvFlags, 100);
        if (!headerDone && n > 0)
        {
            for (; skip < n; skip++)
            {
                if (data[skip] == '\r' && lastChar == '\n')
                {
                    skip++;
                    headerDone = 1;
                    break;
                }
                lastChar = data[skip];
            }
        }
        if (n)
            WriteToMemCard(data + skip, n - skip);
        total += n;
        RotateThreadReadyQueue(100);
    }
    m_bytes = total - skip;
    sceInetClose(&m_client, m_netBuf, m_cid, -1);
    CloseMemCardFile();
    return 1;
}

// 0x005DD988.
void VUrl::PrintError()
{
    printf("Error %i: %s\n", m_error, m_errorText);
}

// 0x005DD9B8.
long VUrl::GetBytes()
{
    return m_bytes;
}
