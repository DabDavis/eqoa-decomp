#line 1 "C:/code-frontiers-gm/drdp/message.cpp"
// message.cpp -- SUPPORT 0x004bad40..0x004bb370, one application message.
//
// A message is a 72-byte header with its payload in the malloc slack straight
// behind it, which is why `message_get_data` is `this + 1` and every size in
// this file is `m_uiSize + sizeof(message_t)`.  It is the unit the DRDP layer
// queues, retransmits and acknowledges: the connection keeps three of these
// lists (arrival, unacked, unsent), and `m_link[0]`/`m_link[1]` are what let
// one message be on two of them at once.
//
// The four-byte tag at the front is a liveness check with teeth.  `create`
// writes "MESS", `is_valid` memcmps it, and `destroy_buffer` overwrites it
// with "NULL" *before* the free -- so a pointer used after the message is gone
// reports invalid instead of reading whatever the allocator left there.
#include "drdp.h"

static const char MESSAGE_TAG[] = "MESSAGE";
static const char NULL_TAG[] = "NULL";

// The header fields go out in the order the struct declares them, but two of
// them do not go out at all: `m_pEndpoint` and `m_pConnection` are pointers
// into this process and are supplied again by whoever restores the stream.
int message_create(message_t **ppMessage, endpoint_t *pEndpoint, connection_t *pConnection,
                   uns64 uiSrc, uns64 uiDst, const void *pData, uns32 uiSize,
                   uns8 ucType, uns16 usSeqnum, uns32 uiTime)
{
    *ppMessage = 0;

    message_t *pMessage = (message_t *)malloc(uiSize + sizeof(message_t));

    if (!pMessage)
        return -2;

    memcpy(pMessage->m_szTag, MESSAGE_TAG, 4);
    pMessage->m_pEndpoint = pEndpoint;
    pMessage->m_pConnection = pConnection;
    pMessage->m_uiSrc = uiSrc;
    pMessage->m_uiDst = uiDst;
    pMessage->m_ucType = ucType;
    pMessage->m_usSeqnum = usSeqnum;
    pMessage->m_uiSize = uiSize;
    pMessage->m_uiCreated = uiTime;
    pMessage->m_uiTime = uiTime;
    if (pData)
        memcpy(message_get_data(pMessage), pData, uiSize);
    *ppMessage = pMessage;
    return 0;
}

// Header and payload in one copy: the source's own size is already in it.
int message_duplicate(message_t **ppMessage, message_t *pMessage)
{
    *ppMessage = 0;

    message_t *pCopy = (message_t *)malloc(pMessage->m_uiSize + sizeof(message_t));

    if (pCopy)
    {
        memcpy(pCopy, pMessage, pMessage->m_uiSize + sizeof(message_t));
        *ppMessage = pCopy;
        return 0;
    }
    return -2;
}

// Serialise and free in one pass; a null buffer makes this the destructor,
// which is what `message_destroy` is.  The two timestamps go out as OFFSETS
// from the endpoint's clock, so the stream survives a restore on a machine
// whose millisecond counter is somewhere else entirely.
void message_destroy_buffer(message_t *pMessage, buffer_t *pBuffer)
{
    if (pBuffer)
    {
        uns32 uiTime = endpoint_get_time(pMessage->m_pEndpoint);

        buffer_appendm_uns8(pBuffer, (const uns8 *)pMessage->m_szTag, 4);
        buffer_append_packed_uns64(pBuffer, pMessage->m_uiSrc);
        buffer_append_packed_uns64(pBuffer, pMessage->m_uiDst);
        buffer_append_uns32(pBuffer, pMessage->m_uiSize);
        buffer_append_uns16(pBuffer, pMessage->m_usSeqnum);
        buffer_append_uns8(pBuffer, pMessage->m_ucType);
        buffer_append_uns8(pBuffer, pMessage->m_ucRefnumOffset);
        buffer_append_uns16(pBuffer, pMessage->m_usFlush);
        buffer_append_offset_uns32(pBuffer, pMessage->m_uiCreated, uiTime);
        buffer_append_offset_uns32(pBuffer, pMessage->m_uiTime, uiTime);
        buffer_appendm_uns8(pBuffer, (const uns8 *)(pMessage + 1), pMessage->m_uiSize);
    }
    memcpy(pMessage->m_szTag, NULL_TAG, 4);
    free(pMessage);
}

// The mirror image, and the payload's length is not known until the header has
// been read -- so the header is read into a LOCAL, and only then is the real
// record allocated and the local assigned into it.  That assignment is the
// 72-byte block copy the target unrolls four doublewords at a time.
int message_create_buffer(message_t **ppMessage, endpoint_t *pEndpoint,
                          connection_t *pConnection, buffer_t *pBuffer)
{
    message_t message;

    *ppMessage = 0;

    uns32 uiTime = endpoint_get_time(pEndpoint);

    buffer_readm_uns8(pBuffer, (uns8 *)message.m_szTag, 4);
    buffer_read_packed_uns64(pBuffer, &message.m_uiSrc);
    buffer_read_packed_uns64(pBuffer, &message.m_uiDst);
    buffer_read_uns32(pBuffer, &message.m_uiSize);
    buffer_read_uns16(pBuffer, &message.m_usSeqnum);
    buffer_read_uns8(pBuffer, &message.m_ucType);
    buffer_read_uns8(pBuffer, &message.m_ucRefnumOffset);
    buffer_read_uns16(pBuffer, &message.m_usFlush);
    buffer_read_offset_uns32(pBuffer, &message.m_uiCreated, uiTime);
    buffer_read_offset_uns32(pBuffer, &message.m_uiTime, uiTime);

    message_t *pMessage = (message_t *)malloc(message.m_uiSize + sizeof(message_t));

    if (!pMessage)
        return -2;

    *pMessage = message;
    pMessage->m_pEndpoint = pEndpoint;
    pMessage->m_pConnection = pConnection;
    buffer_readm_uns8(pBuffer, (uns8 *)(pMessage + 1), message.m_uiSize);
    *ppMessage = pMessage;
    return 0;
}

void message_destroy(message_t *pMessage)
{
    message_destroy_buffer(pMessage, 0);
}

// ---------------------------------------------------------------------------
//
// The accessor wall.  Nothing outside this file touches a `message_t` field
// directly -- the library is C compiled as C++, so the discipline is the only
// encapsulation there is, and every one of these is a load in a delay slot.
//
// The wire record, for anyone writing the other end of this protocol.  What
// `message_destroy_buffer` appends and `message_create_buffer` reads back is
// NOT the in-memory struct: the two pointers are dropped, the two timestamps
// are re-based, and two of the scalars are varints.  In order:
//
//     4 bytes   the tag, "MESS" -- appendm_uns8, so it is not length-prefixed
//     varint    m_uiSrc          packed uns64, 7 bits a byte, low group first
//     varint    m_uiDst          packed uns64
//     4 bytes   m_uiSize         plain uns32, the payload length
//     2 bytes   m_usSeqnum
//     1 byte    m_ucType
//     1 byte    m_ucRefnumOffset
//     2 bytes   m_usFlush
//     4 bytes   m_uiCreated      as an OFFSET from the endpoint's clock
//     4 bytes   m_uiTime         as an OFFSET from the endpoint's clock
//     m_uiSize  the payload, verbatim
//
// The two offset codecs are the reason a serialised message survives being
// restored on a machine whose millisecond counter is somewhere else: the
// endpoint's `get_time` is read once, both stamps go out as differences from
// it, and the reader adds its own clock back on.  `buffer_append_offset_uns32`
// is the codec phase 43 measured -- it adds the raw byte UNSIGNED even where
// the type is signed, which is why a stamp more than 255 ms either side of the
// endpoint's clock does not round-trip.  Nothing here checks that.
//
// `m_pEndpoint` and `m_pConnection` are supplied again by the caller of
// `message_create_buffer`, which is why that function takes them as arguments
// and writes them over the restored copy before the payload is read in.
//
// ---------------------------------------------------------------------------
// The container wiring.
//
// A message is on up to two lists at once, and that is what `m_link[2]` is
// for.  `message_get_vlink_offset(i)` answers `4 + i * 8` -- the offset of
// link `i` inside the record -- and a `vlist_t` is handed that number at
// `vlist_init` so it can convert between an object and its node without the
// object knowing which list it is on.  `message_get_key_offset(0)` answers 52,
// the offset of `m_usSeqnum`, so a list initialised with it is sorted by
// sequence number and `vlist_lookup` finds a message by one.
//
// The two heads a message hangs off are `message_list_t` and
// `message_seqnum_t`.  Both open with a `vlist_t`, which is why every call in
// this file passes `&pList->list` and the target passes the head pointer
// unchanged.  What else is in them is settled in `connection.cpp`, which owns
// `message_list_load`, `message_list_destroy`, `message_list_is_in_sync`,
// `message_seqnum_load` and `message_seqnum_destroy` -- five functions that
// live nowhere near these, at 0x004af.. and 0x004b0.., inside the
// connection's own span rather than at the end of the module with the rest of
// the message code.
//
// ---------------------------------------------------------------------------
// Two things this file does that are worth knowing before trusting it.
//
// `message_create` leaves `m_ucRefnumOffset` and `m_usFlush` UNINITIALISED.
// The allocation is a plain `malloc` and neither field is written, so a fresh
// message carries whatever the allocator left there until somebody calls the
// setter.  Both fields go out on the wire in `destroy_buffer`.  This is not a
// misreading of the code: the ten stores in `create` are the complete list,
// and +0x37 and +0x38 are not among them.
//
// `message_duplicate` copies `m_uiSize + sizeof(message_t)` bytes out of the
// source, which means the copy inherits the source's `m_pEndpoint` and
// `m_pConnection` as well -- two pointers into structures the copy does not
// own and is not registered with.  Nothing in this file fixes that up; the
// caller is expected to `message_set_connection` afterwards, and
// `message_set_connection` is written the way it is (re-deriving the endpoint
// from the connection rather than taking it as a second argument) precisely so
// that the two can never be left disagreeing.
//
// ---------------------------------------------------------------------------
// On the tag, and why the copy is unaligned.
//
// `MESSAGE_TAG` and `NULL_TAG` are file-static `char` ARRAYS, not string
// literals used in place.  It shows: an array of char has alignment 1, so
// gcc's inline 4-byte `memcpy` comes out as `lwl`/`lwr` into `swl`/`swr`,
// where the same copy from a string literal -- which this compiler aligns to a
// word -- is a plain `lw`/`sw` pair.  Four instructions against two, in two
// functions, and it is the only evidence in the file for how the constants
// were declared.  Written as literals, `create` and `destroy_buffer` are both
// four words wrong and nothing else in either is.
//
// Only the first four bytes are ever compared or copied, so the trailing
// "AGE" of "MESSAGE" is never read.  It is in the binary because the constant
// is the whole word: the author wrote the tag out in full and copied a prefix.
//
// `message_is_valid` compares FOUR bytes, not seven, and it is called on
// pointers that may already have been freed -- so it reads memory the
// allocator owns.  That is why `destroy_buffer` writes "NULL" over the tag
// BEFORE the `free` rather than after: the poison has to land while the block
// is still ours.
//
// ---------------------------------------------------------------------------
// A note on the size arithmetic.
//
// `sizeof(message_t)` is 72, and every allocation in this file is
// `payload + 72`.  That number is settled twice over: `message_get_data`
// returns `pMessage + 1`, which the target computes as `addiu v0,a0,72`, and
// the block copy in `message_create_buffer` moves exactly 72 bytes -- two
// unrolled iterations of four doublewords, then one more doubleword.  The
// struct closes on 72 only because of the two `uns64`s: they force 8-byte
// alignment, which pads +0x1C and +0x44 and rounds the tail up from 68.
//
// The payload is not a member.  There is no flexible array here and no
// `m_data[1]`: the space is the malloc slack, and the only way to it is the
// pointer arithmetic `message_get_data` does.  A trailing `uns8 m_data[1]`
// would have made `sizeof` 80 on this ABI and every allocation eight bytes
// too big, and the target's 72 says it is not there.
//
// ---------------------------------------------------------------------------
// Where the rest of the protocol is.
//
// `m_ucType` is the segment type byte the DRDP dispatcher switches on, and the
// set of values it takes is enumerated in `drdp.cpp`, not here -- this file
// carries the byte and never looks at it.  `m_usFlush` is a deadline in
// milliseconds that `route_flush` reads when it decides which connection to
// service next; `m_ucRefnumOffset` is the back-reference the acknowledgement
// path uses to name a message without repeating its sequence number.
//
// `message_list_contains_type_seqnum` is the only place in this file that
// interprets any of that, and it is a duplicate check: the acknowledgement
// path calls it before queueing, so the same (type, seqnum) pair is not sent
// twice when two segments arrive out of order and both ask for one.
//
// `message_seqnum_remove_thru` is the retirement sweep, and its comparison is
// the detail that matters: it subtracts and looks at BIT 15 of the result, so
// it orders sequence numbers across the wrap at 65535 the way
// `uns32_cmp_wrap` orders a wrapping millisecond clock.  Written as a plain
// `<` it would stop retiring anything the moment the counter rolled over, and
// the unacked list would grow without bound for the rest of the session.
//
// ---------------------------------------------------------------------------
// Ordering, for anyone re-deriving this file.
//
// The map's order is the source's order, and it is not alphabetical: create,
// duplicate, destroy_buffer, create_buffer, destroy, then the accessors in
// struct order, then is_valid, then the two offset functions, then the two
// list helpers.  `destroy` sits AFTER `create_buffer` even though it is a
// one-liner, because it is the last of the five lifetime functions and the
// accessors start below it.
//
// The four lifetime functions do not share an if-direction, and this is the
// clearest example of that rule in the corpus.  `create` and `create_buffer`
// spell the allocation check `if (!p) return -2;` and the failure block lands
// inline, at the branch, with the `-2` in its delay slot.  `duplicate` spells
// it `if (p) { ...; return 0; } return -2;` -- the success path in the `if` --
// and gets the same layout by the opposite route.  Written the other way
// round, each of the three moves the `-2` block to the bottom of the function
// and every later word shifts.  Three functions, two spellings, measured one
// at a time; there is no rule here that predicts which.
//
// ---------------------------------------------------------------------------

uns8 message_get_type(const message_t *pMessage)
{
    return pMessage->m_ucType;
}

void *message_get_data(message_t *pMessage)
{
    return pMessage + 1;
}

uns32 message_get_size(const message_t *pMessage)
{
    return pMessage->m_uiSize;
}

uns16 message_get_seqnum(const message_t *pMessage)
{
    return pMessage->m_usSeqnum;
}

void message_set_seqnum(message_t *pMessage, uns16 usSeqnum)
{
    pMessage->m_usSeqnum = usSeqnum;
}

uns32 message_get_time(const message_t *pMessage)
{
    return pMessage->m_uiTime;
}

uns32 message_get_created(const message_t *pMessage)
{
    return pMessage->m_uiCreated;
}

void message_set_time(message_t *pMessage, uns32 uiTime)
{
    pMessage->m_uiTime = uiTime;
}

uns8 message_get_refnum_offset(const message_t *pMessage)
{
    return pMessage->m_ucRefnumOffset;
}

void message_set_refnum_offset(message_t *pMessage, uns8 ucOffset)
{
    pMessage->m_ucRefnumOffset = ucOffset;
}

uns16 message_get_flush(const message_t *pMessage)
{
    return pMessage->m_usFlush;
}

void message_set_flush(message_t *pMessage, uns16 usFlush)
{
    pMessage->m_usFlush = usFlush;
}

endpoint_t *message_get_endpoint(message_t *pMessage)
{
    return pMessage->m_pEndpoint;
}

connection_t *message_get_connection(message_t *pMessage)
{
    return pMessage->m_pConnection;
}

// Setting the connection re-derives the endpoint from it: the two are never
// allowed to disagree, and a null connection leaves the old endpoint alone.
void message_set_connection(message_t *pMessage, connection_t *pConnection)
{
    pMessage->m_pConnection = pConnection;
    if (pConnection)
        pMessage->m_pEndpoint = connection_get_endpoint(pConnection);
}

uns64 message_get_src(const message_t *pMessage)
{
    return pMessage->m_uiSrc;
}

uns64 message_get_dst(const message_t *pMessage)
{
    return pMessage->m_uiDst;
}

int message_is_valid(const message_t *pMessage)
{
    return memcmp(pMessage, "MESSAGE", 4) == 0;
}

int message_get_vlink_offset(int iWhich)
{
    return 4 + iWhich * 8;
}

int message_get_key_offset(int iWhich)
{
    int iOffset = -1;

    if (iWhich == 0)
        iOffset = 52;
    else
        VASSERT(!"unknown key index");
    return iOffset;
}

// A duplicate check for a type/seqnum pair that is still on a list -- the
// caller uses it to keep from queueing the same acknowledgement twice.
int message_list_contains_type_seqnum(message_list_t *pList, uns8 ucType, uns16 usSeqnum)
{
    message_t *pMessage;

    for (pMessage = (message_t *)vlist_peek_head(&pList->list);
         pMessage;
         pMessage = (message_t *)vlist_get_next(&pList->list, pMessage))
    {
        if (message_get_type(pMessage) == ucType && message_get_seqnum(pMessage) == usSeqnum)
            return 1;
    }
    return 0;
}

// Retire everything below a sequence number.  The comparison is on the SIGNED
// 16-bit difference, so it keeps working across the wrap at 65535.
void message_seqnum_remove_thru(message_seqnum_t *pSeqnum, uns16 usSeqnum)
{
    message_t *pMessage;

    while ((pMessage = (message_t *)vlist_peek_head(&pSeqnum->list)) != 0
           && (int16)(message_get_seqnum(pMessage) - usSeqnum) < 0)
    {
        message_destroy((message_t *)vlist_remove_head(&pSeqnum->list));
    }
}
