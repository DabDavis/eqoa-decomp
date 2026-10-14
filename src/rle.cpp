// rle.cpp -- SUPPORT 0x004be220..0x004be460, the DRDP run-length codec.
//
// This is the compression `claude-notes/state-channel-formats.md` sees on the
// wire: a state channel is mostly unchanged bytes, so the stream is a sequence
// of (zero-run, non-zero-run) pairs and only the non-zero bytes are carried.
#include "drdp.h"

// The run tests read `end` through a CONST REFERENCE.  That makes `end`
// addressable, so until the addressof pass each read is a load, cse keeps it as
// its own pseudo, and the purge turns it into a copy `p = end` that loop.c then
// hoists into each inner loop's preheader -- the target's two `move a0,s5`.
// Those two insns are what keep the 127 in the outer loop: loop.c moves an
// invariant only while threshold * savings * lifetime >= the loop's insn count,
// and with them the outer loop is 65 insns to a threshold of 64 (by value it is
// 63, the 127 is hoisted into a seventh callee-saved register, every later
// register shifts).
static inline int rle_before(const uns8 *p, const uns8 *const &end)
{
    return p < end;
}

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

        while (rle_before(data + zeros, end) && data[zeros] == 0 && zeros != 255)
            zeros++;
        data += zeros;
        while (rle_before(data + nonzeros, end) && data[nonzeros] != 0 && nonzeros != 127)
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
