// Http_get.cpp (name ours) -- SETUP 0x005816A8 .. 0x005821D0: one HTTP fetch into
// memory, gunzipped if asked, and copied into the patch image list.  The gzip
// header reader and the inflate call are zlib 1.1.4's gzio.c check_header and
// uncompress, cut down: a raw deflate stream is selected by poking the inflate
// state's mode to BLOCKS after inflateInit.
#include "patcher.h"

// 0x005816A8.
Http_get::Http_get(VIPatchImage *images)
{
    m_compressed = 1;
    m_status = (unsigned int)-1;
    m_magic[0] = 0x1F;
    m_magic[1] = 0x8B;
    m_images = images;
    m_x1374 = 0;
    m_url = 0;
    m_x137C = 0;
    m_x1380 = 0;
    m_listener = 0;
    m_x13A0 = 0;
    m_closed = 0;
    m_running = 0;
    m_bytesDownloaded = 0;
    m_size = 0;
    m_mem = 0;
    m_bytesUnzipped = 0;
    m_timeDownloaded = 0;
    m_timeUnzipped = 0;
    m_readIntoMemory = 0;
    m_temp = 0;
    m_memSize = 0;
    m_errorString = 0;
    m_x13A8 = 0;
    m_errorType = 0;
}

// 0x00581738.
Http_get::~Http_get()
{
    if (m_temp)
    {
        free(m_temp);
        m_temp = 0;
    }
    if (m_x13A8)
        m_x13A8 = 0;
    if (m_errorString)
    {
        free(m_errorString);
        m_errorString = 0;
    }
}

// 0x005817C0.
int Http_get::Initialize(char *x)
{
    m_x1334 = x;
    return 1;
}

// 0x005817D0.
long Get_thread(void *http)
{
    return ((Http_get *)http)->Get_thread();
}

// 0x005817F0.
int Http_get::Get(char *url, char *file, char *c, char *d, int compressed)
{
    if (m_mem)
        free(m_mem);
    for (VIPatchImage *image = m_images; image; image = image->m_next)
    {
        if (strstr(file, image->m_name))
        {
            if (image->m_data)
            {
                free(image->m_data);
                image->m_data = 0;
            }
            break;
        }
    }
    // The members in declaration order: that order reaches sched1, and with it
    // the live lengths that give `url` s3 and `c` s4.
    char *mem = (char *)malloc(0x200000);
    m_mem = mem;
    m_url = url;
    m_x1374 = 0;
    m_dest = mem;
    m_x137C = c;
    m_x1380 = d;
    m_compressed = compressed;
    MemoryGet(mem, 0x200000, url, c, d, compressed);
    WriteFile(file);
    FreeMem();
    return 0;
}

// 0x005818F8.  Into the image of the same name, or a new one at the end.
int Http_get::WriteFile(char *file)
{
    if (!file)
        return 0;
    VIPatchImage *image = 0;
    char *path = strdup(file);
    printf("!!!Writing file %s\n", path);
    VIPatchImage *last = m_images;
    for (VIPatchImage *i = last; i; i = i->m_next)
    {
        if (strstr(path, i->m_name))
        {
            image = i;
            break;
        }
        last = i;
    }
    if (image)
    {
        if (image->m_data)
            free(image->m_data);
        image->m_data = 0;
        image->m_size = 0;
    }
    else
    {
        image = new VIPatchImage;
        last->m_next = image;
    }
    char *name = 0;
    for (char *t = strtok(path, "/\\"); t; t = strtok(0, "/\\"))
        name = t;
    if (name)
    {
        sprintf(image->m_name, name);
        image->m_size = m_size;
        image->m_data = (char *)memalign(128, m_size);
        memset(image->m_data, 0, m_size);
        memcpy(image->m_data, m_mem, m_size);
        image->m_crc = crc32(0, (Bytef *)image->m_data, image->m_size);
        image->m_dirty = 1;
    }
    free(path);
    return 0;
}

// 0x00581A90.
void Http_get::FreeMem()
{
    if (m_mem)
        free(m_mem);
    m_mem = 0;
    m_size = 0;
}

// 0x00581AD0.
int Http_get::MemoryGet(char *buf, int size, char *url, char *c, char *d, int compressed)
{
    m_url = url;
    m_dest = buf;
    m_destSize = size;
    m_x137C = c;
    m_x1380 = d;
    m_compressed = compressed;
    m_x1374 = 0;
    return Get_thread();
}

// 0x00581B10.  gzio.c's get_byte, over a buffer.
int Http_get::http_get_byte(z_stream *s)
{
    if (s->avail_in == 0)
    {
        errno = 0;
        return EOF;
    }
    s->avail_in--;
    return *(s->next_in)++;
}

// 0x00581B58.  gzio.c's check_header, with nowhere to put z_err.
void Http_get::http_check_header(z_stream *s)
{
    int method;
    int flags;
    uInt len;
    int c;

    for (len = 0; len < 2; len++)
    {
        c = http_get_byte(s);
        if (c != m_magic[len])
        {
            if (len != 0)
                s->avail_in++, s->next_in--;
            if (c != EOF)
                s->avail_in++, s->next_in--;
            return;
        }
    }
    method = http_get_byte(s);
    flags = http_get_byte(s);
    if (method != Z_DEFLATED)
        return;

    for (len = 0; len < 6; len++)
        (void)http_get_byte(s);

    if ((flags & 0x04) != 0)
    {
        len = (uInt)http_get_byte(s);
        len += ((uInt)http_get_byte(s)) << 8;
        while (len-- != 0 && http_get_byte(s) != EOF)
            ;
    }
    if ((flags & 0x08) != 0)
    {
        while ((c = http_get_byte(s)) != 0 && c != EOF)
            ;
    }
    if ((flags & 0x10) != 0)
    {
        while ((c = http_get_byte(s)) != 0 && c != EOF)
            ;
    }
    if ((flags & 0x02) != 0)
    {
        for (len = 0; len < 2; len++)
            (void)http_get_byte(s);
    }
}

// 0x00581D60.  uncompress(), over a gzip stream.
int Http_get::http_uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
    z_stream stream;
    int err;

    stream.next_in = (Bytef *)source;
    stream.avail_in = (uInt)sourceLen;
    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

    err = inflateInit(&stream);
    *(int *)stream.state = 7;
    if (err != Z_OK)
        return err;

    http_check_header(&stream);
    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END)
    {
        inflateEnd(&stream);
        *destLen = stream.total_out;
        return err == Z_OK ? Z_BUF_ERROR : err;
    }
    *destLen = stream.total_out;
    err = inflateEnd(&stream);
    return err;
}

// 0x00581E30.  The PC timed the download and the unzip; `t' is what is left
// of the clock, and is never set.
long Http_get::Get_thread()
{
    char msg[512];
    char zero[4096];
    long length = 0;
    long t;
    m_running = 1;
    memset(zero, 0, 4096);
    if (m_dest)
    {
        int bytes = 0;
        m_temp = (char *)malloc(m_destSize);
        sprintf(m_temp, "blah");
        m_x300[0] = 0;
        *m_dest = 0;
        VUrl *url = new VUrl(m_url);
        url->SetUrlListener(m_listener);
        if (!url->Get(m_temp, m_destSize))
        {
            SetError(url->GetErrorString(), url->GetError());
            if (url)
                delete url;
            m_running = 0;
            m_status = 8;
            return 0;
        }
        if (!m_closed)
        {
            long n = url->GetBytes();
            m_status = 4;
            bytes = n;
            m_bytesDownloaded = n;
        }
        if (url)
            delete url;
        long size = m_destSize;
        sprintf(m_dest, "Hello");
        if (m_compressed == 1)
        {
            http_uncompress((Bytef *)m_dest, (uLongf *)&size, (Bytef *)m_temp, bytes);
            length = size;
        }
        if (!strcasecmp(m_dest, "Hello"))
        {
            if (!strncpy(m_dest, m_temp, bytes + 1))
            {
                FormatString(msg, "Could not copy %s to memory\n", m_url);
                SetError(msg, 31);
                free(m_temp);
                m_temp = 0;
                return m_status;
            }
        }
        else
            m_dest[size] = 0;
        m_timeDownloaded = t;
        free(m_temp);
        m_temp = 0;
    }
    m_timeUnzipped = t;
    if (m_closed)
        m_status = 8;
    else
        m_status = 6;
    m_running = 0;
    if (m_x13A8)
        m_x13A8 = 0;
    m_size = length;
    return length;
}

// 0x00582090.
void Http_get::SetError(const char *msg, int type)
{
    m_errorType = type;
    if (m_errorString)
    {
        free(m_errorString);
        m_errorString = 0;
    }
    if (msg)
        m_errorString = strdup(msg);
    m_status = 5;
    m_closed = 1;
    m_running = 0;
}

// 0x00582100.
int Http_get::GetErrorType()
{
    return m_errorType;
}

// 0x00582108.
char *Http_get::GetErrorString()
{
    return m_errorString;
}

// 0x00582110.
void Http_get::SetStatus(int status)
{
    m_status = status;
}

// 0x00582118.
int Http_get::GetStatus()
{
    return m_status;
}

// 0x00582120.
void Http_get::Close()
{
    m_closed = 1;
}

// 0x00582130.
int Http_get::Running()
{
    return m_running;
}

// 0x00582138.
unsigned long Http_get::Get_bytes_received()
{
    return m_bytesDownloaded + m_bytesUnzipped;
}

// 0x00582148.
unsigned long Http_get::GetBytesDownloaded()
{
    return m_bytesDownloaded;
}

// 0x00582150.
unsigned long Http_get::GetTimeDownloaded()
{
    return m_timeDownloaded;
}

// 0x00582158.
unsigned long Http_get::GetBytesUnzipped()
{
    return m_bytesUnzipped;
}

// 0x00582160.
unsigned long Http_get::GetTimeUnzipped()
{
    return m_timeUnzipped;
}

// 0x00582168.
int Http_get::SetReadIntoMemory(int size)
{
    if (size >= 0)
    {
        m_memSize = size;
        m_readIntoMemory = 1;
        return 1;
    }
    return 0;
}
