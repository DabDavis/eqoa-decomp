// VIStringTable -- SUPPORT 0x004A4050 .. 0x004A4940.
#include "vi.h"

extern "C" void *memcpy(void *d, const void *s, unsigned int n);

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
