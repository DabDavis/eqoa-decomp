// rle.cpp -- SUPPORT 0x004be220..0x004be460, the DRDP run-length codec.
//
// This is the compression `claude-notes/state-channel-formats.md` sees on the
// wire: a state channel is mostly unchanged bytes, so the stream is a sequence
// of (zero-run, non-zero-run) pairs and only the non-zero bytes are carried.
#include "drdp.h"

// OPEN, by one register: gcc CSEs the two 127s -- the loop's limit and the
// entry guard's copy of it -- into one pseudo whose live range spans the outer
// loop, so it takes a seventh callee-saved register and every later one shifts.
// The target rematerialises the constant in both places.  Eight spellings swept
// (`<` vs `!=`, hex, an explicit guarded do/while, a pointer walker, the
// counters at function scope, a named `start`); the whole tail of the function
// is otherwise instruction-for-instruction identical.
void run_length_encode(const uns8 *data, uns32 size, buffer_t *buffer)
{
    const uns8 *end = data + size;

    // the start position is read and thrown away, and so is the end one: the
    // function is VOID.  v0 holds the end position at the `jr ra` either way,
    // but declared `int` its one caller, connection_append_state, goes from
    // byte-exact to 6 wrong words -- an ignored call_value is not a call.
    buffer_get_position(buffer);
    for (;;)
    {
        uns32 zeros = 0;
        uns32 nonzeros = 0;

        while (data + zeros < end && data[zeros] == 0 && zeros != 255)
            zeros++;
        data += zeros;
        while (data + nonzeros < end && data[nonzeros] != 0 && nonzeros != 127)
            nonzeros++;
        if (nonzeros >= 8 || zeros >= 16)
        {
            buffer_append_uns8(buffer, (uns8)(nonzeros | 0x80));
            buffer_append_uns8(buffer, (uns8)zeros);
        }
        else
        {
            uns8 c = (uns8)((nonzeros << 4) | zeros);

            buffer_append_uns8(buffer, c);
            if (c == 0)
                break;
        }
        buffer_appendm_uns8(buffer, data, nonzeros);
        data += nonzeros;
    }
    buffer_get_position(buffer);
}

void run_length_decode(buffer_t *buffer, uns8 *data, uns32 size)
{
    buffer_t out;
    uns8 c;

    buffer_init(&out, data, size, BUFFER_LITTLE_ENDIAN);
    for (;;)
    {
        uns32 zeros;
        uns32 nonzeros;

        buffer_read_uns8(buffer, &c);
        if (c == 0)
            break;
        if (c & 0x80)
        {
            nonzeros = c & 0x7f;
            buffer_read_uns8(buffer, &c);
            zeros = c;
        }
        else
        {
            nonzeros = c >> 4;
            zeros = c & 0xf;
        }
        for (uns32 i = 0; i < zeros; i++)
            buffer_append_uns8(&out, 0);
        buffer_copy(buffer, &out, nonzeros);
    }
}
