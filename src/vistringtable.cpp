// VIStringTable -- SUPPORT 0x004A4050 .. 0x004A4940.  Line-padded (phase 61):
// LoadRawFile's two asserts carry lines 245 and 253 of
// game/VIStringTable.cpp as immediates.
#include "vi.h"

extern "C" void *memcpy(void *d, const void *s, unsigned int n);
extern "C" void *memset(void *d, int c, unsigned int n);
extern "C" int   memcmp(const void *a, const void *b, unsigned int n);
extern "C" void  _VIAssert(const char *expr, const char *file, int line);
extern "C" void  _VIAssertf(int cond, const char *fmt, ...);
extern "C" const char *__VIAssertf_file;
extern "C" int   __VIAssertf_line;

extern int VIMemCardReadFile(const char *name, void *buf, int size);
int UTF8_strlen(const char *s, int max);
int UTF8_convertToUTF16(const char *s, unsigned short *out, int max);

#define VIASSERT(x)  if (!(x)) _VIAssert(#x, __FILE__, __LINE__);
#define VIASSERTF(x, fmt, args...) \
    { __VIAssertf_file = __FILE__; __VIAssertf_line = __LINE__; _VIAssertf(x, fmt, ## args); }

static const char NullString[] = "";

VIStringTable::VIStringTable()
{
    m_numRows = 0;
    m_numCols = 0;
    m_text = 0;
    m_strings = 0;
}

VIStringTable::~VIStringTable()
{
    Unload();
}

void VIStringTable::Unload()
{
    delete [] m_text;
    delete [] m_strings;

    m_numRows = 0;
    m_numCols = 0;
    m_text = 0;
    m_strings = 0;
}

const char *VIStringTable::GetString(int row, int col)
{
    const char *s = NullString;

    row--;

    if (row >= 0 && row < m_numRows && col >= 0 && col < m_numCols)
    {
        const char *t = m_strings[row * m_numCols + col];

        if (t)
            s = t;
    }

    return s;
}

// Phase 61.  WORDS.TXT off the card: read into a 128 KB scratch array, then
// copied into one of exactly the size read, so the table keeps no slack.  The
// second Init's result is its OWN local: sharing `r` swaps the two
// callee-saved registers and merges the inline cleanup into the tail.
int VIStringTable::LoadFromMemCard(const char *name)
{
    VIArray<char> tmp;
    int r = tmp.Init(0x20000);

    if (r)
        return r;

    int n = VIMemCardReadFile(name, tmp.m_data, tmp.m_size);

    if (n < 0)
        return -1;

    VIArray<char> buf;

    if (n)
    {
        int e = buf.Init(n);

        if (e)
            return e;
        memcpy(buf.m_data, tmp.m_data, n);
        tmp.Clear();
    }

    r = ProcessRawFile(buf);
    if (r == 0)
    {
        r = FixupTextBuffer();
        if (r == 0)
            return 0;
    }
    return r;
}

int VIStringTable::LoadFromMemory(const void *data, int size)
{
    VIArray<char> buf;

    int r = buf.Init(size);

    if (r == 0)
    {
        memcpy(buf.m_data, data, size);

        r = ProcessRawFile(buf);

        if (r == 0)
        {
            r = FixupTextBuffer();

            if (r == 0)
                return 0;
        }
    }

    return r;
}

// Phase 61.  The text is UCS-2 by now.  Pass one sizes the table -- a TAB
// starts a column and a NEWLINE a row, and the widest row sets the column
// count; pass two cuts the text in place, NUL over every TAB, CR and NEWLINE,
// and points each cell at the character after its separator.  The two passes
// keep their column in two DIFFERENT locals -- one variable reused costs the
// register assignment of both loops.
int VIStringTable::FixupTextBuffer()
{
    int len = VIString::strlen16((unsigned short *)m_text) + 1;
    int i;

    m_numRows = 1;
    m_numCols = 1;
    int col = 1;

    for (i = 0; i < len; i++)
    {
        unsigned short c = ((unsigned short *)m_text)[i];

        if (c == '\t')
            col++;
        else if (c == '\n')
        {
            m_numRows++;
            m_numCols = VIMAX(m_numCols, col);
            col = 1;
        }
    }

    int r;

    m_strings = new char *[m_numRows * m_numCols];
    if (m_strings == 0)
        r = -1;
    else
    {
        memset(m_strings, 0, m_numRows * m_numCols * sizeof(char *));
        m_strings[0] = m_text;

        int row = 0;
        int cell = 0;
        for (i = 0; i < len; i++)
        {
            if (((unsigned short *)m_text)[i] == '\t')
            {
                ((unsigned short *)m_text)[i] = 0;
                cell++;
                m_strings[row * m_numCols + cell] = (char *)&((unsigned short *)m_text)[i + 1];
            }
            else if (((unsigned short *)m_text)[i] == '\r')
                ((unsigned short *)m_text)[i] = 0;
            else if (((unsigned short *)m_text)[i] == '\n')
            {
                ((unsigned short *)m_text)[i] = 0;
                row++;
                cell = 0;
                m_strings[row * m_numCols + cell] = (char *)&((unsigned short *)m_text)[i + 1];
            }
        }
        r = 0;
    }

    if (r)
        Unload();
    return r;
}















































int VIStringTable::LoadRawFile(const char *name, VIArray<char> &buf)
{
    VIFile File;

    VIASSERTF(File.Open(name, 33) == 0, "file open failed: %s", name);

    int r = File.Seek(0, VIFile::Seek_End);



    if (r == 0)
    {
        VIASSERT(File.Tell() <= 0xFFFFFFFF);
        r = File.Tell();     // the size, in the SAME local as every result

        int e = buf.Init(r);

        if (e)
            return e;
        if (r == 0)
            return 0;
        e = File.Seek(0, VIFile::Seek_Set);
        if (e)
            return e;
        r = File.Read(buf.m_data, r);
        if (r == 0)
            return 0;
    }
    return r;
}

// The big-endian BOM is a function-local static OBJECT with a constructor: the
// image tests a guard word on every call and stores 0xFFFE the first time,
// with the address formed BEFORE the value -- an inlined constructor's `this`.
// A plain `unsigned short` with an inline-function initialiser forms the value
// first (three words).
static inline unsigned short Swap16(unsigned short c)
{
    return (c >> 8) | (c << 8);
}

// Phase 61.  Four encodings by their byte-order mark: UTF-8 (converted), UTF-16
// little-endian (copied), UTF-16 big-endian (swapped), and anything else as
// 8-bit text widened through VIString::str8to16.  m_text ends up UCS-2 either
// way.  The three BOMs are file statics; their SIZEOF is what makes the
// length tests unsigned (`sltiu`).
class Utf16Char
{
public:
    unsigned short c;
    Utf16Char(unsigned short v) : c(v) {}
};

static const unsigned char Utf8Bom[3] = { 0xEF, 0xBB, 0xBF };
static const unsigned char Utf16Bom[2] = { 0xFF, 0xFE };

int VIStringTable::ProcessRawFile(const VIArray<char> &buf)
{
    static Utf16Char BigEndianBom(0xFFFE);
    int n;

    if (buf.m_size >= sizeof(Utf8Bom) && memcmp(Utf8Bom, buf.m_data, sizeof(Utf8Bom)) == 0)
    {
        const char *p = buf.m_data + sizeof(Utf8Bom);

        n = UTF8_strlen(p, buf.m_size - sizeof(Utf8Bom));
        m_text = (char *)new unsigned short[n + 1];
        if (m_text == 0)
            return -1;
        // The mismatch test is materialised (`sltu`) and RETURNED: a UTF-8
        // file that does not convert cleanly answers 1, not -1.
        int r = n != UTF8_convertToUTF16(p, (unsigned short *)m_text, n);
        if (r)
            return r;
    }
    else if (buf.m_size >= sizeof(Utf16Bom) && memcmp(Utf16Bom, buf.m_data, sizeof(Utf16Bom)) == 0)
    {
        const char *p = buf.m_data + sizeof(Utf16Bom);

        n = buf.m_size / 2 - 1;
        m_text = (char *)new unsigned short[n + 1];
        if (m_text == 0)
            return -1;
        memcpy(m_text, p, n * 2);
        ((unsigned short *)m_text)[n] = 0;
    }
    else if (buf.m_size >= sizeof(BigEndianBom) && memcmp(&BigEndianBom, buf.m_data, sizeof(BigEndianBom)) == 0)
    {
        const unsigned short *p = (const unsigned short *)(buf.m_data + sizeof(BigEndianBom));

        n = buf.m_size / 2 - 1;
        m_text = (char *)new unsigned short[n + 1];
        if (m_text == 0)
            return -1;
        for (int i = 0; i < n; i++)
            ((unsigned short *)m_text)[i] = Swap16(p[i]);
        ((unsigned short *)m_text)[n] = 0;
    }
    else
    {
        const char *p = buf.m_data;

        n = buf.m_size;
        m_text = (char *)new unsigned short[n + 1];
        if (m_text == 0)
            return -1;
        VIString::str8to16((unsigned short *)m_text, (n + 1) * 2, p);
    }
    return 0;
}

int VIStringTable::Load(const char *name)
{
    VIArray<char> buf;

    int r = -1;

    if (LoadRawFile(name, buf) >= 0)
    {
        if (ProcessRawFile(buf) >= 0)
        {
            // A ternary VALUE and not an `if`: the value form materialises -1
            // and selects with `movn`, where every `if` spelling folds the
            // test to `slti ...,0` and selects with `movz` (six measured).
            r = (FixupTextBuffer() > -1) ? 0 : -1;
        }
    }

    if (r)
        Unload();

    return r;
}
