// buffer.cpp -- SUPPORT 0x004bcee8..0x004be120, the DRDP wire buffer.
//
// 85 named functions and FOUR statics the symbol map does not name: the map's
// previous entry, uns64_range_t_cmp, and buffer_init_read both acquire a span
// that swallows two of them, so both are capped.
//
// The byte order is the design.  A buffer carries a `swap` flag decided ONCE at
// init from the host's endianness and the flags, and every scalar read or
// append goes through one pair of helpers that either memcpy or copy the bytes
// BACKWARDS.  Everything else in the file is one line over that pair.
#include "drdp.h"

// The overflow guard is one `||` in both directions: two `if`s would give two
// stores of `error = 1`, and the target branches to a single one.
static void buffer_read_bytes(buffer_t *buffer, void *value, int size)
{
    if (buffer->position + size > buffer->size || buffer->overflow)
    {
        buffer->overflow = 1;
        return;
    }
    if (buffer->swap)
    {
        uns8 *p = (uns8 *)value + size - 1;
        for (int i = 0; i < size; i++)
            *p-- = buffer->data[buffer->position + i];
    }
    else
    {
        memcpy(value, buffer->data + buffer->position, size);
    }
    buffer->position += size;
}

// The append side tests read_only and the read side does not -- and neither
// `m` helper below tests anything at all.
static void buffer_append_bytes(buffer_t *buffer, const void *value, int size)
{
    if (buffer->read_only)
        return;
    if (buffer->position + size > buffer->size || buffer->overflow)
    {
        buffer->overflow = 1;
        return;
    }
    if (buffer->swap)
    {
        const uns8 *p = (const uns8 *)value + size - 1;
        for (int i = 0; i < size; i++)
            buffer->data[buffer->position + i] = *p--;
    }
    else
    {
        memcpy(buffer->data + buffer->position, value, size);
    }
    buffer->position += size;
}

int buffer_bytes_remaining(buffer_t *buffer)
{
    if (buffer->overflow)
        return -1;
    return buffer->size - buffer->position;
}

// `lui`/`lb` on a static: the low byte of a word holding 1, so the answer is
// measured at run time and not compiled in.
static int endian_probe = 1;

int little_endian()
{
    return *(int8 *)&endian_probe;
}

void buffer_init(buffer_t *buffer, void *data, int size, int flags)
{
    buffer->size = size;
    buffer->data = (uns8 *)data;
    buffer->position = 0;
    buffer->overflow = 0;
    // ONE `||`, not two `if`s: when the left half is true the target never
    // calls little_endian() a second time, and only short-circuiting can skip
    // a call gcc has to assume has side effects.
    buffer->swap = ((flags & BUFFER_LITTLE_ENDIAN) && !little_endian()) ||
                   ((flags & BUFFER_BIG_ENDIAN) && little_endian());
    buffer->read_only = 0;
}

void buffer_init_read(buffer_t *buffer, const void *data, int size, int flags)
{
    buffer_init(buffer, (void *)data, size, flags);
    buffer->read_only = 1;
}

static void buffer_readm_bytes(buffer_t *buffer, void *value, int size, int count)
{
    int total = size * count;
    if (buffer->position + total > buffer->size)
        return;
    if (buffer->overflow)
        return;
    if (buffer->swap)
    {
        uns8 *p = (uns8 *)value;
        for (int i = 0; i < count; i++)
        {
            buffer_read_bytes(buffer, p, size);
            p += size;
        }
    }
    else
    {
        memcpy(value, buffer->data + buffer->position, total);
        buffer->position += total;
    }
}

static void buffer_appendm_bytes(buffer_t *buffer, const void *value, int size, int count)
{
    int total = size * count;
    if (buffer->position + total > buffer->size)
        return;
    if (buffer->overflow)
        return;
    if (buffer->swap)
    {
        const uns8 *p = (const uns8 *)value;
        for (int i = 0; i < count; i++)
        {
            buffer_append_bytes(buffer, p, size);
            p += size;
        }
    }
    else
    {
        memcpy(buffer->data + buffer->position, value, total);
        buffer->position += total;
    }
}

// Phase 7's if-direction: the ERROR store is the block that lands LAST, so it
// is the `else` and the success path falls through.
void buffer_skip(buffer_t *buffer, int size)
{
    int position = buffer->position + size;

    if (position <= buffer->size && buffer->overflow == 0)
        buffer->position = position;
    else
        buffer->overflow = 1;
}

int buffer_get_position(buffer_t *buffer)
{
    return buffer->position;
}

void buffer_set_position(buffer_t *buffer, int position)
{
    if (position >= 0 && position < buffer->size)
        buffer->position = position;
    else
        buffer->overflow = 1;
}

void buffer_swap(buffer_t *buffer, int flags)
{
    buffer->swap = ((flags & BUFFER_LITTLE_ENDIAN) && !little_endian()) ||
                   ((flags & BUFFER_BIG_ENDIAN) && little_endian());
}

// The DIRECTION is read out of the memcpy: the destination is the SECOND
// parameter's data, so this copies parameter 1 into parameter 2 -- and the
// overflow check that sets BOTH error flags is the one on parameter 1.
// Phase 7's if-direction again: both error blocks land after the success path,
// so the success path is what the `if` holds.
void buffer_copy(buffer_t *from, buffer_t *to, int size)
{
    if (from->position + size <= from->size)
    {
        if (to->position + size <= to->size)
        {
            memcpy(to->data + to->position, from->data + from->position, size);
            from->position += size;
            to->position += size;
        }
        else
        {
            // the branch-likely's delay slot carries this store, so it really
            // is on the failing path: the source is consumed even when the
            // destination cannot take it.
            from->position += size;
            to->overflow = 1;
        }
    }
    else
    {
        // two statements, not a chain: the chain stores `to` first, and the
        // second arm's lone store only cross-jumps onto this block's tail if
        // the tail is the one it duplicates.
        from->overflow = 1;
        to->overflow = 1;
    }
}

void *buffer_get_position_ptr(buffer_t *buffer)
{
    return buffer->data + buffer->position;
}

#define BUFFER_TYPE(NAME, TYPE)                                               \
    void buffer_read_##NAME(buffer_t *buffer, TYPE *value)                    \
    {                                                                         \
        buffer_read_bytes(buffer, value, sizeof(TYPE));                       \
    }                                                                         \
    void buffer_readm_##NAME(buffer_t *buffer, TYPE *value, int count)        \
    {                                                                         \
        buffer_readm_bytes(buffer, value, sizeof(TYPE), count);               \
    }                                                                         \
    void buffer_append_##NAME(buffer_t *buffer, TYPE value)                   \
    {                                                                         \
        buffer_append_bytes(buffer, &value, sizeof(TYPE));                    \
    }                                                                         \
    void buffer_appendm_##NAME(buffer_t *buffer, const TYPE *value, int count) \
    {                                                                         \
        buffer_appendm_bytes(buffer, value, sizeof(TYPE), count);             \
    }

BUFFER_TYPE(int8,   int8)
BUFFER_TYPE(uns8,   uns8)
BUFFER_TYPE(int16,  int16)
BUFFER_TYPE(uns16,  uns16)
BUFFER_TYPE(int32,  int32)
BUFFER_TYPE(uns32,  uns32)
BUFFER_TYPE(int64,  int64)
BUFFER_TYPE(uns64,  uns64)
BUFFER_TYPE(float,  float)
BUFFER_TYPE(double, double)

// The 7-bit varint every packed codec below funnels through.
void buffer_append_packed_uns64(buffer_t *buffer, uns64 value)
{
    uns8 c;
    uns8 b;

    do
    {
        c = (uns8)value;
        value >>= 7;
        b = (uns8)(c & 0x7f);
        if (value)
            b |= 0x80;
        buffer_append_uns8(buffer, b);
    }
    while (value);
}

void buffer_read_packed_uns64(buffer_t *buffer, uns64 *value)
{
    uns64 v = 0;
    uns32 shift = 0;

    while (shift < 64)
    {
        uns8 c;
        buffer_read_uns8(buffer, &c);
        v |= (uns64)(c & 0x7f) << shift;
        if (!(c & 0x80))
            break;
        shift += 7;
    }
    *value = v;
}

// Zigzag, and the sign extension is real: the packed value is an int16 handed
// to a uns64 parameter, so a zigzag result that overflows 16 bits is sent as a
// ten-byte varint.
// Zigzag, written as ONE value modified in place with a separate sign flag:
// an if/else over two shifted results gives two `sll`s and a branch where the
// target shifts once, in the argument.  The int16 form truncates BEFORE the
// or -- `sll 17`/`sra 16` -- and the argument conversion sign-extends from bit
// 15, so a zigzag result that overflows 16 bits is sent as a ten-byte varint.
void buffer_append_packed_int16(buffer_t *buffer, int16 value)
{
    int sign = 0;

    if (value < 0)
    {
        value = ~value;
        sign = 1;
    }
    value <<= 1;
    value |= sign;
    buffer_append_packed_uns64(buffer, value);
}

void buffer_read_packed_int16(buffer_t *buffer, int16 *value)
{
    uns64 v;

    buffer_read_packed_uns64(buffer, &v);
    int16 r = (int16)(v >> 1);
    if (v & 1)
        r = (int16)~r;
    *value = r;
}

void buffer_append_packed_int32(buffer_t *buffer, int32 value)
{
    int sign = 0;

    if (value < 0)
    {
        value = ~value;
        sign = 1;
    }
    value <<= 1;
    value |= sign;
    buffer_append_packed_uns64(buffer, value);
}

void buffer_read_packed_int32(buffer_t *buffer, int32 *value)
{
    uns64 v;

    buffer_read_packed_uns64(buffer, &v);
    int32 r = (int32)(v >> 1);
    if (v & 1)
        r = ~r;
    *value = r;
}

void buffer_append_packed_int64(buffer_t *buffer, int64 value)
{
    int sign = 0;

    if (value < 0)
    {
        value = ~value;
        sign = 1;
    }
    buffer_append_packed_uns64(buffer, (value << 1) | sign);
}

void buffer_read_packed_int64(buffer_t *buffer, int64 *value)
{
    uns64 v;

    buffer_read_packed_uns64(buffer, &v);
    int64 r = (int64)(v >> 1);
    if (v & 1)
        r = ~r;
    *value = r;
}

void buffer_append_packed_uns16(buffer_t *buffer, uns16 value)
{
    buffer_append_packed_uns64(buffer, value);
}

void buffer_read_packed_uns16(buffer_t *buffer, uns16 *value)
{
    uns64 v;

    buffer_read_packed_uns64(buffer, &v);
    *value = (uns16)v;
}

void buffer_append_packed_uns32(buffer_t *buffer, uns32 value)
{
    buffer_append_packed_uns64(buffer, value);
}

void buffer_read_packed_uns32(buffer_t *buffer, uns32 *value)
{
    uns64 v;

    buffer_read_packed_uns64(buffer, &v);
    *value = (uns32)v;
}

// The delta codecs.  The VALUE is the left operand of the add: written the
// other way round all four of the 32- and 64-bit readers compute into the
// offset's register instead.  The add is in the declared TYPE, with no cast to
// the unsigned twin: `(UTYPE)v + offset` leaves the 8- and 16-bit SIGNED pair
// computing into the value's register (2 wrong words each), and moving the
// operands instead breaks the four wide readers.
#define BUFFER_OFFSET_TYPE(NAME, TYPE, UTYPE)                                 \
    void buffer_read_offset_##NAME(buffer_t *buffer, TYPE *value, TYPE offset) \
    {                                                                         \
        TYPE v;                                                               \
        buffer_read_##NAME(buffer, &v);                                       \
        *value = (TYPE)(v + offset);                                          \
    }                                                                         \
    void buffer_append_offset_##NAME(buffer_t *buffer, TYPE value, TYPE offset) \
    {                                                                         \
        buffer_append_##NAME(buffer, (TYPE)(value - offset));                 \
    }

BUFFER_OFFSET_TYPE(int8,  int8,  uns8)
BUFFER_OFFSET_TYPE(uns8,  uns8,  uns8)
BUFFER_OFFSET_TYPE(int16, int16, uns16)
BUFFER_OFFSET_TYPE(uns16, uns16, uns16)
BUFFER_OFFSET_TYPE(int32, int32, uns32)
BUFFER_OFFSET_TYPE(uns32, uns32, uns32)
BUFFER_OFFSET_TYPE(int64, int64, uns64)
BUFFER_OFFSET_TYPE(uns64, uns64, uns64)

// Phase 10's guarded do/while -- and the walking pointer is a SECOND variable,
// declared inside the guard.  The guard reads `*string` through the PARAMETER
// register and the loop reads `*p` through a callee-saved one, which is the
// only thing that stops gcc threading the entry test onto the loop's own and
// jumping straight into it.
void buffer_append_string(buffer_t *buffer, const char *string)
{
    if (*string != 0 && buffer->overflow == 0)
    {
        const char *p = string;

        do
        {
            buffer_append_int8(buffer, *p);
            p++;
        }
        while (*p != 0 && buffer->overflow == 0);
    }
    buffer_append_int8(buffer, 0);
}

// Two counters, not one: `len` stops at the caller's limit while `n` keeps
// counting, so the whole string is consumed off the wire either way.
void buffer_read_string(buffer_t *buffer, char *string, int size)
{
    int len = 0;
    int n = 0;

    for (;;)
    {
        int8 c;
        buffer_read_int8(buffer, &c);
        if (c == 0)
            break;
        if (buffer->overflow)
            break;
        if (n < size - 1)
        {
            string[len] = c;
            len++;
        }
        n++;
    }
    string[len] = 0;
}
