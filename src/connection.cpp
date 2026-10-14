#line 1 "C:/code-frontiers-gm/drdp/connection.cpp"
// connection.cpp -- SUPPORT 0x004af418..0x004b4aa8, ONE conversation.
//
// The original is `C:/code-frontiers-gm/drdp/connection.cpp` -- its own asserts
// say so -- and it is over four thousand lines, which is why the source below
// is padded: an assert's `__LINE__` is an immediate that has to match, so the
// line each one lands on is as load-bearing as the code around it (phase 27).
//
// A connection carries four message containers and 248 state channels.  Three
// of the containers are lists of `message_t` and the fourth is the pre-built
// terminate notification; the state channels are malloc'd on first use, 60
// bytes each, because most connections never send state at all.
#include "drdp.h"

static const char CONNECTION_TAG[] = "CONNECTION";
static const char NULL_TAG[] = "NULL";

// Phase 10's va_start: the EABI's register save block sits BELOW
// virtual_incoming_args, so bare __builtin_next_arg lands past the whole area.
typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)

extern "C" void vpf(const char *pFormat, char *ap);
extern "C" unsigned int strlen(const char *);
extern "C" int vsprintf(char *, const char *, va_list);

// The message types.  Anything below 248 is a state channel INDEX; the four
// above are the transports, and `connection_get_max_message_size` is where
// they are told apart by size.
enum
{
    CONNECTION_STATE_CHANNEL_COUNT = 248,
    CONNECTION_MESSAGE_TYPE_SYSTEM = 249,
    CONNECTION_MESSAGE_TYPE_BIG    = 250,
    CONNECTION_MESSAGE_TYPE_GUARANTEED = 251,
    CONNECTION_MESSAGE_TYPE_BEST_EFFORT = 252,
    CONNECTION_MESSAGE_TYPE_TERMINATE = 253
};

enum
{
    CONNECTION_KEY_COUNT = 2
};

// A macro, so each argument is evaluated twice -- the call that wins runs again.
#define DRDP_MIN(a, b) ((a) < (b) ? (a) : (b))
#define DRDP_MAX(a, b) ((a) < (b) ? (b) : (a))

// The wrap-safe clock comparisons.  INLINE FUNCTIONS, not macros: the sum
// handed in as an argument is computed first, where a macro lets fold
// reassociate `(t + k) - now` into `t - (now - k)`.
inline int uns32_gt(uns32 a, uns32 b)
{
    return (int)(a - b) > 0;
}

inline int uns32_ge(uns32 a, uns32 b)
{
    return (int)(a - b) >= 0;
}

inline int uns16_gt(uns16 a, uns16 b)
{
    return (int16)(a - b) > 0;
}

inline int uns16_lt(uns16 a, uns16 b)
{
    return (int16)(a - b) < 0;
}

inline int uns16_le(uns16 a, uns16 b)
{
    return (int16)(a - b) <= 0;
}

inline int uns16_ge(uns16 a, uns16 b)
{
    return (int16)(a - b) >= 0;
}

inline int uns16_diff(uns16 a, uns16 b)
{
    return (int16)(a - b);
}

// A message list starts empty and unsorted.
inline void message_list_init(message_list_t *pList)
{
    vlist_init(&pList->list, message_get_vlink_offset(0), 0, 0);
}

// The rest of the message-list API is inline too.  connection_send_guaranteed
// goes through it for EVERY list operation: each call's parameter is then a
// pseudo set inside a loop, loop.c hoists `&messages` into a callee-saved
// register, and the target's `move a0,s4` comes out where `&messages.list`
// spelt at the call rematerialises `addiu a0,sp,16` (18 wrong words to 0).
inline void message_list_add_tail(message_list_t *pList, message_t *pMessage)
{
    vlist_add_tail(&pList->list, pMessage);
}

inline message_t *message_list_peek_head(message_list_t *pList)
{
    return (message_t *)vlist_peek_head(&pList->list);
}

inline message_t *message_list_remove_head(message_list_t *pList)
{
    return (message_t *)vlist_remove_head(&pList->list);
}

inline int message_list_get_size(message_list_t *pList)
{
    return vlist_get_size(&pList->list);
}

// The by-seqnum list has the same front end; connection_move_delayed_to_arrivals
// peeks through it (3 wrong words to 0: the test is on v0, not on a copy).
inline message_t *message_seqnum_peek_head(message_seqnum_t *pSeqnum)
{
    return (message_t *)vlist_peek_head(&pSeqnum->list);
}

// The idle period a software pair waits before it resets.
#define CONNECTION_IDLE_RESET 120000

// The largest state message, and so the most a state delta decodes to.
#define DRDP_SEND_SIZE_MAX_STATE 1300

// A state channel starts with its send sequence number at 1 and its pending
// message nowhere.  Both lists sort by the message's own sequence number, so
// `uns16_cmp_wrap` orders them across the wrap.
void state_channel_init(state_channel_t *pChannel)
{
    pChannel->m_pPending = 0;
    pChannel->rx = 0;
    pChannel->m_usTxAck = 0;
    pChannel->m_ucRxFlag = 0;
    pChannel->m_ucWhere = 0;
    pChannel->m_usTx = 1;
    vlist_init(&pChannel->m_txList.list, message_get_vlink_offset(0),
               (vcmp_t)uns16_cmp_wrap, message_get_key_offset(0));
    vlist_init(&pChannel->m_rxList.list, message_get_vlink_offset(0),
               (vcmp_t)uns16_cmp_wrap, message_get_key_offset(0));
}

// The connection's own zeroing.  It is a static -- the symbol map has no name
// for it, and `state_channel_init` above acquires its span.
static void connection_init(connection_t *pConnection)
{
    memcpy(pConnection->m_szTag, CONNECTION_TAG, 4);
    pConnection->endpoint = 0;
    pConnection->m_pRoute = 0;
    pConnection->m_pAppPtr = 0;
    pConnection->app_info = 0;
    pConnection->local_id = 0;
    pConnection->remote_id = 0;
    vlist_init(&pConnection->m_sendQueue.list, message_get_vlink_offset(0), 0, 0);
    vlist_init(&pConnection->m_outstanding.list, message_get_vlink_offset(0), 0, 0);
    vlist_init(&pConnection->m_arrivals.list, message_get_vlink_offset(0),
               (vcmp_t)uns16_cmp_wrap, message_get_key_offset(0));
    vlist_init(&pConnection->m_delayed.list, message_get_vlink_offset(0),
               (vcmp_t)uns16_cmp_wrap, message_get_key_offset(0));
    pConnection->m_pTerminate = 0;
    pConnection->m_uiTimeInstanceSent = 0;
    pConnection->time_last_flush = 0;
    pConnection->m_uiInstanceRemote = 0;
    pConnection->m_usFlags = 0;
    pConnection->m_ucRttIndex = 0;
    pConnection->m_ucRxFlag = 0;
    memset(pConnection->m_usRtt, 0, sizeof(pConnection->m_usRtt));
    pConnection->m_usTimeout = 1000;
    memset(pConnection->m_uiFlushTime, 0, sizeof(pConnection->m_uiFlushTime));
    pConnection->m_usFlush = 1;
    pConnection->m_usArrivalNumber = 0;
    pConnection->m_usFlushAck = 0;
    pConnection->m_uiRxMask = 0;
    memset(&pConnection->m_uiArrivalHistory, 255, sizeof(pConnection->m_uiArrivalHistory));
    pConnection->m_usTxSeqnum = 1;
    pConnection->m_usRxSeqnum = 0;
    pConnection->m_usTxAck = 0;
    pConnection->m_uiTxMask = 0;
    pConnection->m_iRetryTime = 5000;
    pConnection->m_iWarnTime = 30000;
    pConnection->m_iFailTime = 60000;
    memset(pConnection->m_uiCstat, 0, sizeof(pConnection->m_uiCstat));
    pConnection->m_pLogger = 0;
    pConnection->m_pChannels = 0;
    pConnection->m_iLastRouteFlushNumber = -1;
}

// The tag has to match AND the endpoint has to still own it, so a connection
// destroyed and freed under a pointer somebody kept fails here.
int connection_is_valid(const connection_t *pConnection)
{
    int iResult = memcmp(pConnection->m_szTag, CONNECTION_TAG, 4) == 0;

    if (iResult)
        iResult = endpoint_has_connection(pConnection->endpoint, pConnection) != 0;
    return iResult;
}

// 248 channels of 60 bytes is 14,880, allocated the first time the application
// sends state and never again.
int connection_allocate_state_channels(connection_t *pConnection)
{
    int i;

    if (!pConnection->m_pChannels)
    {
        pConnection->m_pChannels =
            (state_channel_t *)malloc(sizeof(state_channel_t) * CONNECTION_STATE_CHANNEL_COUNT);
        if (!pConnection->m_pChannels)
            return -2;
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
            state_channel_init(&pConnection->m_pChannels[i]);
    }
    return 0;
}

int connection_get_vnode_offset(int iIndex)
{
    return iIndex * 16 + 4;
}














































































































int connection_get_key_offset(int index)
{
    int iOffset = -1;

    VASSERT(0 <= index && index < CONNECTION_KEY_COUNT);   // 340
    switch (index)
    {
    case 0:
        iOffset = 80;
        break;
    case 1:
        iOffset = 180;
        break;
    }
    return iOffset;
}














































































































































// The failure path is endpoint_create's: the local is NULLED on success, so
// the destroy at the bottom runs only if the initialisation fell out early.
//
// The terminate notification is built HERE, once, and kept until the
// connection actually goes away -- so a terminate can be delivered when there
// is no memory left to allocate one.  Its source and destination are the far
// end's and ours the other way round, because it is delivered as an ARRIVAL.
int connection_create(connection_t **ppConnection, endpoint_t *pEndpoint, uns64 uiDst,
                      uns32 uiInstance, int iFlags)
{
    int iResult;
    connection_t *pConnection = (connection_t *)malloc(sizeof(connection_t));

    if (!pConnection)
    {
        iResult = -2;
        goto out;
    }
    connection_init(pConnection);
    pConnection->endpoint = pEndpoint;
    pConnection->local_id = endpoint_get_id(pEndpoint);
    pConnection->remote_id = uiDst;
    pConnection->m_uiDesiredFlushTime = endpoint_get_time(pEndpoint);
    pConnection->time_last_arrival = endpoint_get_time(pEndpoint);
    pConnection->m_uiTimeLastInstanceArrival = endpoint_get_time(pEndpoint);
    pConnection->m_uiTimeSend = endpoint_get_time(pEndpoint);
    pConnection->m_uiInstanceLocal = uiInstance;
    drdp_format_endpoint(pConnection->m_szName, pConnection->remote_id);
    if (iFlags & 1)
    {
        pConnection->m_uiInstanceRemote = pConnection->m_uiInstanceLocal;
        pConnection->m_usFlags |= 0xa0;
    }
    if (iFlags & 2)
        pConnection->m_usFlags |= 0x40;
    if (!connection_is_sw_sw(pConnection))
    {
        iResult = message_create(&pConnection->m_pTerminate, pEndpoint, pConnection,
                                 pConnection->remote_id, pConnection->local_id, 0, 256,
                                 CONNECTION_MESSAGE_TYPE_TERMINATE, 0xffff,
                                 endpoint_get_time(pEndpoint));
        if (iResult)
            goto out;
    }
    *ppConnection = pConnection;
    iResult = 0;
    pConnection = 0;
out:
    if (pConnection)
        connection_destroy(pConnection);
    return iResult;
}

// Everything the connection still owns that is a message: the two plain lists,
// the two sorted ones, and both lists of every state channel.
void connection_free_outstanding(connection_t *pConnection)
{
    message_t *pMessage;
    int i;

    while ((pMessage = (message_t *)vlist_remove_head(&pConnection->m_sendQueue.list)) != 0)
        message_destroy(pMessage);
    while ((pMessage = (message_t *)vlist_remove_head(&pConnection->m_outstanding.list)) != 0)
        message_destroy(pMessage);
    while ((pMessage = (message_t *)vlist_remove_head(&pConnection->m_delayed.list)) != 0)
        message_destroy(pMessage);
    while ((pMessage = (message_t *)vlist_remove_head(&pConnection->m_arrivals.list)) != 0)
        message_destroy(pMessage);
    if (pConnection->m_pChannels)
    {
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
        {
            state_channel_t *pChannel = &pConnection->m_pChannels[i];

            while ((pMessage = (message_t *)vlist_remove_head(&pChannel->m_txList.list)) != 0)
                message_destroy(pMessage);
            while ((pMessage = (message_t *)vlist_remove_head(&pChannel->m_rxList.list)) != 0)
                message_destroy(pMessage);
        }
    }
}

// The two serialisers.  A null buffer means "just free them"; a real one gets
// the count first and then each message, which is how a connection survives a
// save and restore.
void message_seqnum_destroy(message_seqnum_t *pSeqnum, buffer_t *pBuffer)
{
    message_t *pMessage;

    if (pBuffer)
        buffer_append_packed_uns64(pBuffer, vlist_get_size(&pSeqnum->list));
    while ((pMessage = (message_t *)vlist_remove_head(&pSeqnum->list)) != 0)
        message_destroy_buffer(pMessage, pBuffer);
}

void message_list_destroy(message_list_t *pList, buffer_t *pBuffer)
{
    message_t *pMessage;

    if (pBuffer)
        buffer_append_packed_uns64(pBuffer, vlist_get_size(&pList->list));
    while ((pMessage = (message_t *)vlist_remove_head(&pList->list)) != 0)
        message_destroy_buffer(pMessage, pBuffer);
}

// The flush-time ring on the wire: a packed count of zero entries, then one
// non-zero entry as a plain uns32, repeated until all 1,024 are accounted
// for, then 0xbaadf00d.  Most of a quiet connection's ring is zeroes.  The
// zero test reads entry 1024 before the bound stops it.
void compress_flush_time_history(buffer_t *pBuffer, uns32 *puiHistory)
{
    int i = 0;

    while (i < 1024)
    {
        uns64 uiZeros = 0;

        while (puiHistory[i] == 0 && i < 1024)
        {
            uiZeros++;
            i++;
        }
        buffer_append_packed_uns64(pBuffer, uiZeros);
        if (i == 1024)
            break;
        buffer_append_uns32(pBuffer, puiHistory[i]);
        i++;
    }
    buffer_append_uns32(pBuffer, 0xbaadf00d);
}

void decompress_flush_time_history(buffer_t *pBuffer, uns32 *puiHistory)
{
    int i = 0;
    uns64 uiZeros;
    uns32 uiMagic;

    while (i < 1024)
    {
        uiZeros = 0;
        buffer_read_packed_uns64(pBuffer, &uiZeros);
        while (uiZeros--)
            puiHistory[i++] = 0;
        if (i == 1024)
            break;
        buffer_read_uns32(pBuffer, &puiHistory[i++]);
    }
    buffer_read_uns32(pBuffer, &uiMagic);
}

// Compress, decompress into a scratch ring, and compress again for real.
// Whatever compared the two rings is gone; the round trip is still made.
void compress_flush_time_history_verified(buffer_t *pBuffer, uns32 *puiHistory)
{
    uns32 uiCheck[1024];
    buffer_t buffer;
    buffer_t check;
    uns8 data[8192];

    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    compress_flush_time_history(&buffer, puiHistory);
    buffer_init_read(&check, buffer.data, buffer.position, BUFFER_LITTLE_ENDIAN);
    decompress_flush_time_history(&check, uiCheck);
    compress_flush_time_history(pBuffer, puiHistory);
}

// Serialise and free.  Times go out as offsets from now, so a connection
// restored later comes back with its deadlines intact relative to the clock.
// The last byte says which of the two optional parts follow: bit 0 the
// terminate notification, bit 1 the state channels.
void connection_destroy_buffer(connection_t *pConnection, buffer_t *pBuffer)
{
    uns32 uiNow = endpoint_get_time(pConnection->endpoint);
    int i;

    if (pBuffer)
    {
        uns8 ucFlags = 0;

        vlist_get_size(&pConnection->m_sendQueue.list);
        vlist_get_size(&pConnection->m_outstanding.list);
        vlist_get_size(&pConnection->m_arrivals.list);
        vlist_get_size(&pConnection->m_delayed.list);
        buffer_appendm_uns8(pBuffer, (const uns8 *)pConnection->m_szTag, 4);
        if (!pConnection->app_info)
            buffer_append_packed_int32(pBuffer, -1);
        else
        {
            int iLength = strlen(pConnection->app_info);

            buffer_append_packed_int32(pBuffer, iLength);
            buffer_appendm_uns8(pBuffer, (const uns8 *)pConnection->app_info, iLength);
        }
        buffer_append_packed_uns64(pBuffer, pConnection->remote_id);
        buffer_append_uns32(pBuffer, pConnection->m_uiInstanceLocal);
        buffer_append_uns32(pBuffer, pConnection->m_uiInstanceRemote);
        buffer_append_offset_uns32(pBuffer, pConnection->m_uiTimeInstanceSent, uiNow);
        buffer_append_offset_uns32(pBuffer, pConnection->time_last_flush, uiNow);
        buffer_append_offset_uns32(pBuffer, pConnection->time_last_arrival, uiNow);
        buffer_append_offset_uns32(pBuffer, pConnection->m_uiTimeLastInstanceArrival, uiNow);
        buffer_append_offset_uns32(pBuffer, pConnection->m_uiTimeSend, uiNow);
        buffer_append_offset_uns32(pBuffer, pConnection->m_uiDesiredFlushTime, uiNow);
        buffer_append_offset_uns32(pBuffer, pConnection->m_uiFlushBy, uiNow);
        compress_flush_time_history_verified(pBuffer, pConnection->m_uiFlushTime);
        buffer_append_uns16(pBuffer, pConnection->m_usFlags);
        buffer_append_uns8(pBuffer, pConnection->m_ucRttIndex);
        buffer_append_uns8(pBuffer, pConnection->m_ucRxFlag);
        buffer_append_uns16(pBuffer, pConnection->m_usTimeout);
        buffer_appendm_uns16(pBuffer, pConnection->m_usRtt, 16);
        buffer_append_uns16(pBuffer, pConnection->m_usFlush);
        buffer_append_uns16(pBuffer, pConnection->m_usArrivalNumber);
        buffer_append_uns16(pBuffer, pConnection->m_usFlushAck);
        buffer_append_uns64(pBuffer, pConnection->m_uiRxMask);
        buffer_append_uns64(pBuffer, pConnection->m_uiArrivalHistory);
        buffer_append_uns16(pBuffer, pConnection->m_usTxSeqnum);
        buffer_append_uns16(pBuffer, pConnection->m_usRxSeqnum);
        buffer_append_uns16(pBuffer, pConnection->m_usTxAck);
        buffer_append_uns64(pBuffer, pConnection->m_uiTxMask);
        buffer_append_uns32(pBuffer, pConnection->m_iRetryTime);
        buffer_append_uns32(pBuffer, pConnection->m_iWarnTime);
        buffer_append_uns32(pBuffer, pConnection->m_iFailTime);
        buffer_appendm_packed(pBuffer, pConnection->m_uiCstat, 11);
        if (pConnection->m_pTerminate)
            ucFlags |= 1;
        if (pConnection->m_pChannels)
            ucFlags |= 2;
        buffer_append_uns8(pBuffer, ucFlags);
    }
    message_list_destroy(&pConnection->m_sendQueue, pBuffer);
    message_list_destroy(&pConnection->m_outstanding, pBuffer);
    message_seqnum_destroy(&pConnection->m_arrivals, pBuffer);
    message_seqnum_destroy(&pConnection->m_delayed, pBuffer);
    if (pConnection->m_pChannels)
    {
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
        {
            if (pBuffer)
            {
                buffer_append_uns16(pBuffer, pConnection->m_pChannels[i].m_usTx);
                buffer_append_uns16(pBuffer, pConnection->m_pChannels[i].m_usTxAck);
                buffer_append_uns16(pBuffer, pConnection->m_pChannels[i].rx);
                buffer_append_uns8(pBuffer, pConnection->m_pChannels[i].m_ucRxFlag);
                buffer_append_uns8(pBuffer, pConnection->m_pChannels[i].m_ucWhere);
            }
            message_seqnum_destroy(&pConnection->m_pChannels[i].m_txList, pBuffer);
            message_seqnum_destroy(&pConnection->m_pChannels[i].m_rxList, pBuffer);
        }
        free(pConnection->m_pChannels);
        pConnection->m_pChannels = 0;
    }
    if (pConnection->m_pTerminate)
    {
        message_destroy_buffer(pConnection->m_pTerminate, pBuffer);
        pConnection->m_pTerminate = 0;
    }
    memcpy(pConnection->m_szTag, NULL_TAG, 4);
    free(pConnection);
}

int message_seqnum_load(message_seqnum_t *pSeqnum, endpoint_t *pEndpoint, buffer_t *pBuffer)
{
    uns64 uiCount;
    message_t *pMessage;
    int i;
    int iResult;

    buffer_read_packed_uns64(pBuffer, &uiCount);
    for (i = 0; i < uiCount; i++)
    {
        iResult = message_create_buffer(&pMessage, pEndpoint, 0, pBuffer);
        if (iResult)
            goto done;
        vlist_add_tail(&pSeqnum->list, pMessage);
    }
    iResult = 0;
done:
    return iResult;
}

int message_list_load(message_list_t *pList, endpoint_t *pEndpoint, buffer_t *pBuffer)
{
    uns64 uiCount;
    message_t *pMessage;
    int i;
    int iResult;

    buffer_read_packed_uns64(pBuffer, &uiCount);
    for (i = 0; i < uiCount; i++)
    {
        iResult = message_create_buffer(&pMessage, pEndpoint, 0, pBuffer);
        if (iResult)
            goto done;
        vlist_add_tail(&pList->list, pMessage);
    }
    iResult = 0;
done:
    return iResult;
}

// A restored list holds new copies of the messages, so every state channel's
// pending pointer is re-aimed at the copy that is on it.
void conection_state_channel_fixup(connection_t *pConnection, message_list_t *pList)
{
    message_t *pMessage;

    for (pMessage = (message_t *)vlist_peek_head(&pList->list);
         pMessage;
         pMessage = (message_t *)vlist_get_next(&pList->list, pMessage))
    {
        int iType = message_get_type(pMessage);

        if (0 <= iType && iType < CONNECTION_STATE_CHANNEL_COUNT)
        {
            state_channel_t *pChannel = &pConnection->m_pChannels[iType];

            pChannel->m_pPending = pMessage;
        }
    }
}

// The inverse of `destroy_buffer`.  It returns 0 on EVERY path -- a failed
// load destroys the half-built connection and still reports success, with
// `*ppConnection` never written, so the caller inserts whatever was in it.
int connection_create_buffer(connection_t **ppConnection, endpoint_t *pEndpoint,
                             buffer_t *pBuffer)
{
    uns8 ucFlags = 0;
    int iLength;
    uns32 uiNow = endpoint_get_time(pEndpoint);
    connection_t *connection = (connection_t *)malloc(sizeof(connection_t));
    int i;

    if (!connection)
        goto done;
    connection->endpoint = pEndpoint;
    connection->local_id = endpoint_get_id(pEndpoint);
    connection->m_pAppPtr = 0;
    connection->m_pRoute = 0;
    vlist_init(&connection->m_sendQueue.list, message_get_vlink_offset(0), 0, 0);
    vlist_init(&connection->m_outstanding.list, message_get_vlink_offset(0), 0, 0);
    vlist_init(&connection->m_arrivals.list, message_get_vlink_offset(0),
               (vcmp_t)uns16_cmp_wrap, message_get_key_offset(0));
    vlist_init(&connection->m_delayed.list, message_get_vlink_offset(0),
               (vcmp_t)uns16_cmp_wrap, message_get_key_offset(0));
    connection->m_pTerminate = 0;
    connection->m_pChannels = 0;
    buffer_readm_uns8(pBuffer, (uns8 *)connection->m_szTag, 4);
    buffer_read_packed_int32(pBuffer, &iLength);
    connection->app_info = 0;
    if (iLength >= 0)
    {
        connection->app_info = (char *)malloc(iLength + 1);
        VASSERT(connection->app_info != 0);   // 846
        buffer_readm_uns8(pBuffer, (uns8 *)connection->app_info, iLength);
        connection->app_info[iLength] = 0;
    }
    buffer_read_packed_uns64(pBuffer, &connection->remote_id);
    drdp_format_endpoint(connection->m_szName, connection->remote_id);
    buffer_read_uns32(pBuffer, &connection->m_uiInstanceLocal);
    buffer_read_uns32(pBuffer, &connection->m_uiInstanceRemote);
    buffer_read_offset_uns32(pBuffer, &connection->m_uiTimeInstanceSent, uiNow);
    buffer_read_offset_uns32(pBuffer, &connection->time_last_flush, uiNow);
    buffer_read_offset_uns32(pBuffer, &connection->time_last_arrival, uiNow);
    buffer_read_offset_uns32(pBuffer, &connection->m_uiTimeLastInstanceArrival, uiNow);
    buffer_read_offset_uns32(pBuffer, &connection->m_uiTimeSend, uiNow);
    buffer_read_offset_uns32(pBuffer, &connection->m_uiDesiredFlushTime, uiNow);
    buffer_read_offset_uns32(pBuffer, &connection->m_uiFlushBy, uiNow);
    decompress_flush_time_history(pBuffer, connection->m_uiFlushTime);
    buffer_read_uns16(pBuffer, &connection->m_usFlags);
    buffer_read_uns8(pBuffer, &connection->m_ucRttIndex);
    buffer_read_uns8(pBuffer, &connection->m_ucRxFlag);
    buffer_read_uns16(pBuffer, &connection->m_usTimeout);
    buffer_readm_uns16(pBuffer, connection->m_usRtt, 16);
    buffer_read_uns16(pBuffer, &connection->m_usFlush);
    buffer_read_uns16(pBuffer, &connection->m_usArrivalNumber);
    buffer_read_uns16(pBuffer, &connection->m_usFlushAck);
    buffer_read_uns64(pBuffer, &connection->m_uiRxMask);
    buffer_read_uns64(pBuffer, &connection->m_uiArrivalHistory);
    buffer_read_uns16(pBuffer, &connection->m_usTxSeqnum);
    buffer_read_uns16(pBuffer, &connection->m_usRxSeqnum);
    buffer_read_uns16(pBuffer, &connection->m_usTxAck);
    buffer_read_uns64(pBuffer, &connection->m_uiTxMask);
    buffer_read_uns32(pBuffer, (uns32 *)&connection->m_iRetryTime);
    buffer_read_uns32(pBuffer, (uns32 *)&connection->m_iWarnTime);
    buffer_read_uns32(pBuffer, (uns32 *)&connection->m_iFailTime);
    buffer_readm_packed(pBuffer, connection->m_uiCstat, 11);
    buffer_read_uns8(pBuffer, &ucFlags);
    if (message_list_load(&connection->m_sendQueue, pEndpoint, pBuffer))
        goto done;
    if (message_list_load(&connection->m_outstanding, pEndpoint, pBuffer))
        goto done;
    if (message_seqnum_load(&connection->m_arrivals, pEndpoint, pBuffer))
        goto done;
    if (message_seqnum_load(&connection->m_delayed, pEndpoint, pBuffer))
        goto done;
    if (ucFlags & 2)
    {
        if (connection_allocate_state_channels(connection))
            goto done;
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
        {
            if (pBuffer)
            {
                buffer_read_uns16(pBuffer, &connection->m_pChannels[i].m_usTx);
                buffer_read_uns16(pBuffer, &connection->m_pChannels[i].m_usTxAck);
                buffer_read_uns16(pBuffer, &connection->m_pChannels[i].rx);
                buffer_read_uns8(pBuffer, &connection->m_pChannels[i].m_ucRxFlag);
                buffer_read_uns8(pBuffer, &connection->m_pChannels[i].m_ucWhere);
            }
            if (message_seqnum_load(&connection->m_pChannels[i].m_txList, pEndpoint, pBuffer))
                goto done;
            if (message_seqnum_load(&connection->m_pChannels[i].m_rxList, pEndpoint, pBuffer))
                goto done;
        }
        conection_state_channel_fixup(connection, &connection->m_sendQueue);
        conection_state_channel_fixup(connection, &connection->m_outstanding);
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
            ;
    }
    if (ucFlags & 1)
    {
        if (message_create_buffer(&connection->m_pTerminate, pEndpoint, connection, pBuffer))
            goto done;
    }
    *ppConnection = connection;
    connection = 0;
done:
    if (connection)
        connection_destroy(connection);
    return 0;
}

void connection_destroy(connection_t *pConnection)
{
    connection_destroy_buffer(pConnection, 0);
}

// One best-effort message, straight onto the send queue.  Its sequence number
// is 0xffff -- best effort is never acknowledged, so it never needs one.
int connection_send_best_effort(connection_t *pConnection, const uns8 *pData, int iSize)
{
    message_t *pMessage = 0;
    int iResult = message_create(&pMessage, pConnection->endpoint, pConnection,
                                 pConnection->local_id, pConnection->remote_id, pData,
                                 iSize, CONNECTION_MESSAGE_TYPE_BEST_EFFORT, 0xffff,
                                 endpoint_get_time(pConnection->endpoint));

    if (iResult == 0)
    {
        vlist_add_tail(&pConnection->m_sendQueue.list, pMessage);
        pMessage = 0;
    }
    if (pMessage)
        message_destroy(pMessage);
    return iResult;
}

































































































// A guaranteed message is FRAGMENTED at 1300 bytes and every fragment gets its
// own sequence number, so a five-megabyte send is four thousand messages.  All
// of them are built into a local list first and only moved onto the send queue
// when the whole set succeeded -- a half-sent message is worse than none.
int connection_send_guaranteed(connection_t *connection, const uns8 *pData, int iSize,
                               int iType)
{
    message_list_t messages;
    message_t *pMessage = 0;
    int iResult;
    uns32 uiOffset;
    uns32 uiChunk;
    uns8 ucType;
    message_list_init(&messages);
    connection->m_uiTimeSend = endpoint_get_time(connection->endpoint);
    for (uiOffset = 0; uiOffset == 0 || uiOffset < (uns32)iSize; uiOffset += 1300)
    {
        uiChunk = iSize - uiOffset;
        if (uiChunk > 1300)
            uiChunk = 1300;
        ucType = (uns8)iType;
        if (uiOffset + uiChunk < (uns32)iSize)
            ucType = CONNECTION_MESSAGE_TYPE_BIG;
        if (uns16_gt(connection->m_usTxSeqnum, connection->m_usTxAck + 4096))
        {
            VASSERT(!connection_is_sw_sw( connection ));   // 1072
            iResult = -1;
            goto out;
        }
        iResult = message_create(&pMessage, connection->endpoint, connection,
                                 connection->local_id, connection->remote_id,
                                 pData + uiOffset, uiChunk, ucType, connection->m_usTxSeqnum++,
                                 endpoint_get_time(connection->endpoint));
        if (iResult != 0)
            goto out;
        message_list_add_tail(&messages, pMessage);
    }
    message_list_get_size(&messages);
    while (message_list_peek_head(&messages))
    {
        message_get_seqnum(message_list_peek_head(&messages));
        message_get_size(message_list_peek_head(&messages));
        vlist_add_tail(&connection->m_sendQueue.list, message_list_remove_head(&messages));
    }
    iResult = 0;
out:
    while (message_list_peek_head(&messages))
        message_destroy(message_list_remove_head(&messages));
    return iResult;
}
















































































// State is a SLOT, not a stream: if the bytes have not changed since the
// message already pending on this channel there is nothing to send, and if
// they have the pending one is replaced wherever it is sitting.
int connection_send_state(connection_t *pConnection, const uns8 *pData, int iSize, int iChannel)
{
    message_t *pMessage = 0;
    state_channel_t *pChannel;
    int iSame;
    int iResult;
    if (iChannel >= CONNECTION_STATE_CHANNEL_COUNT)
        return -1;
    iResult = connection_allocate_state_channels(pConnection);
    if (iResult)
        return iResult;
    pChannel = &pConnection->m_pChannels[iChannel];
    if (pChannel->m_pPending)
        pMessage = pChannel->m_pPending;
    else
        pMessage = (message_t *)vlist_peek_tail(&pChannel->m_txList.list);
    if (!pMessage)
        iSame = iSize == 0;
    else
    {
        iSame = 0;
        if (message_get_size(pMessage) == (uns32)iSize)
            iSame = memcmp(pData, message_get_data(pMessage), iSize) == 0;
    }
    if (iSame)
        return 0;
    iResult = message_create(&pMessage, pConnection->endpoint, pConnection,
                             pConnection->local_id, pConnection->remote_id, pData, iSize,
                             (uns8)iChannel, 0xffff, endpoint_get_time(pConnection->endpoint));
    if (iResult)
        return iResult;
    switch (pChannel->m_ucWhere)
    {
    case 0:
        vlist_add_tail(&pConnection->m_sendQueue.list, pMessage);
        break;
    case 1:
        vlist_replace(&pConnection->m_sendQueue.list, pChannel->m_pPending, pMessage);
        break;
    case 2:
        vlist_add_tail(&pConnection->m_sendQueue.list, pMessage);
        vlist_remove(&pConnection->m_outstanding.list, pChannel->m_pPending);
        break;
    }
    if (pChannel->m_pPending)
        message_destroy(pChannel->m_pPending);
    pChannel->m_pPending = pMessage;
    pChannel->m_ucWhere = 1;
    return 0;
}

int connection_is_closed(connection_t *pConnection)
{
    return (pConnection->m_usFlags >> 11) & 1;
}

// The one place the four transports are told apart.  A terminated connection
// silently drops everything, which is how the application can keep writing to
// one that has already gone away.
int connection_send_internal(connection_t *connection, const uns8 *pData, int iSize, int iType)
{
    int iResult;
    VASSERT(!connection_is_closed( connection ));   // 1242
    VASSERT(!endpoint_is_closed( connection->endpoint ));   // 1243
    if (connection_is_terminated(connection))
        iResult = 0;
    else
    {
        int iError;

        switch (iType)
        {
        case CONNECTION_MESSAGE_TYPE_BEST_EFFORT:
            iError = connection_send_best_effort(connection, pData, iSize);
            break;
        case CONNECTION_MESSAGE_TYPE_GUARANTEED:
        case CONNECTION_MESSAGE_TYPE_SYSTEM:
            iError = connection_send_guaranteed(connection, pData, iSize, iType);
            break;
        default:
            iError = connection_send_state(connection, pData, iSize, iType);
            break;
        }
        if (iError == 0)
        {
            iResult = 0;
            route_resort(connection);
        }
        else
        {
            connection_terminate_reason(connection, 1, 7);
            iResult = iError;
        }
    }
    return iResult;
}






















void connection_send(connection_t *pConnection, const void *pData, int size, int message_type)
{
    VASSERT(size <= connection_get_max_message_size( message_type ));   // 1300
    connection_send_internal(pConnection, (const uns8 *)pData, size, message_type);
}



















































int connection_get_max_message_size(int iType)
{
    int iSize = -1;

    if ((0 <= iType && iType < CONNECTION_STATE_CHANNEL_COUNT)
        || iType == CONNECTION_MESSAGE_TYPE_BEST_EFFORT)
        iSize = 1300;
    else if (iType == CONNECTION_MESSAGE_TYPE_GUARANTEED || iType == CONNECTION_MESSAGE_TYPE_SYSTEM)
        iSize = 0x514000;
    return iSize;
}

route_t *connection_get_route(connection_t *pConnection)
{
    return pConnection->m_pRoute;
}

void connection_set_route(connection_t *pConnection, route_t *pRoute)
{
    pConnection->m_pRoute = pRoute;
}

// A size is one byte when it fits in one and a 0xff escape plus a word when it
// does not, so the common short state channel costs one byte on the wire.
void buffer_append_size(buffer_t *pBuffer, uns16 usSize)
{
    if (usSize < 255)
    {
        buffer_append_uns8(pBuffer, (uns8)usSize);
    }
    else
    {
        buffer_append_uns8(pBuffer, 255);
        buffer_append_uns16(pBuffer, usSize);
    }
}

void buffer_read_size(buffer_t *pBuffer, uns16 *pusSize)
{
    uns8 ucSize;

    buffer_read_uns8(pBuffer, &ucSize);
    if (ucSize == 255)
        buffer_read_uns16(pBuffer, pusSize);
    else
        *pusSize = ucSize;
}

// The delayed list holds guaranteed messages that arrived out of order, so the
// mask is one bit per sequence number past the next one expected.  Sixty-four
// bits, which is why the outstanding window is capped at 4,096 and not more.
void connection_get_ack_mask(connection_t *pConnection, uns64 *puiMask)
{
    message_t *pMessage;
    uns64 uiMask = 0;

    for (pMessage = (message_t *)vlist_peek_head(&pConnection->m_delayed.list); pMessage;
         pMessage = (message_t *)vlist_get_next(&pConnection->m_delayed.list, pMessage))
    {
        int iBit = (int16)(message_get_seqnum(pMessage) - pConnection->m_usRxSeqnum);

        iBit -= 2;
        if (iBit < 64)
            uiMask |= (uns64)1 << iBit;
        else
            break;
    }
    *puiMask = uiMask;
}

// The state channel delta.  Both are statics; the map has no name for either,
// so connection_get_ack_mask above acquires their span.
static void xor_in_place(uns8 *pTo, const uns8 *pFrom, int iSize)
{
    int i;

    for (i = 0; i < iSize; i++)
        pTo[i] ^= pFrom[i];
}

static void xor_into(uns8 *pTo, const uns8 *pA, const uns8 *pB, int iSize)
{
    int i;

    for (i = 0; i < iSize; i++)
        pTo[i] = pA[i] ^ pB[i];
}

int connection_is_sw_sw(connection_t *pConnection)
{
    int iResult = 0;

    if (drdp_is_software_endpoint(pConnection->local_id))
        iResult = drdp_is_software_endpoint(pConnection->remote_id) != 0;
    return iResult;
}

// The receive half of a reset: forget what the far end told us, and throw away
// everything that has arrived out of order on every state channel.
void connection_rx_reset(connection_t *pConnection)
{
    message_t *pMessage;
    int i;

    pConnection->m_uiInstanceRemote = 0;
    pConnection->m_usFlags &= 0xffdf;
    pConnection->m_ucRxFlag = 0;
    pConnection->m_usArrivalNumber = 0;
    pConnection->m_usRxSeqnum = 0;
    drdp_format_endpoint1(pConnection->local_id);
    drdp_format_endpoint2(pConnection->remote_id);
    if (pConnection->m_pChannels)
    {
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
        {
            state_channel_t *pChannel = &pConnection->m_pChannels[i];

            pChannel->rx = 0;
            pChannel->m_ucRxFlag = 0;
            while ((pMessage = (message_t *)vlist_remove_head(&pChannel->m_rxList.list)) != 0)
                message_destroy(pMessage);
        }
    }
}

// And the send half: a NEW local instance, so anything still in flight from
// the old one is recognised as stale when it comes back.
void connection_tx_reset(connection_t *pConnection)
{
    message_t *pMessage;
    int i;

    drdp_format_endpoint1(pConnection->local_id);
    drdp_format_endpoint2(pConnection->remote_id);
    pConnection->m_uiInstanceLocal = endpoint_get_instance(pConnection->endpoint);
    pConnection->m_usFlags &= 0xfdbf;
    pConnection->m_uiTimeInstanceSent = 0;
    pConnection->time_last_flush = 0;
    pConnection->m_usFlush = 1;
    pConnection->m_usTxSeqnum = 1;
    pConnection->m_usFlushAck = 0;
    pConnection->m_uiRxMask = 0;
    pConnection->m_usTxAck = 0;
    pConnection->m_uiTxMask = 0;
    if (pConnection->m_pChannels)
    {
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
        {
            state_channel_t *pChannel = &pConnection->m_pChannels[i];

            pChannel->m_usTx = 1;
            pChannel->m_usTxAck = 0;
            while ((pMessage = (message_t *)vlist_remove_head(&pChannel->m_txList.list)) != 0)
                message_destroy(pMessage);
        }
    }
}

// A message is appended to the wire buffer only if it FITS; the caller stops
// on the first refusal.  A guaranteed message carries its sequence number, a
// best-effort one does not, and only the guaranteed one is kept for a retry.
int connection_append_message(connection_t *pConnection, buffer_t *pBuffer,
                              message_t *pMessage, message_list_t *pList, uns16 usFlush)
{
    uns8 ucType = message_get_type(pMessage);
    uns32 uiSize = message_get_size(pMessage);
    uns16 usSeqnum = message_get_seqnum(pMessage);
    int iReliable = (uns8)(ucType + 7) < 3;
    int iCost = uiSize + 4;

    if ((uns8)(ucType + 7) <= 2)
        iCost = uiSize + 6;
    if (pBuffer->size < pBuffer->position + iCost)
        return -1;
    pMessage = (message_t *)vlist_remove_head(&pList->list);
    buffer_append_uns8(pBuffer, ucType);
    buffer_append_size(pBuffer, (uns16)uiSize);
    if (iReliable)
        buffer_append_uns16(pBuffer, usSeqnum);
    buffer_appendm_uns8(pBuffer, (const uns8 *)message_get_data(pMessage), uiSize);
    if (iReliable)
    {
        message_set_time(pMessage, endpoint_get_time(pConnection->endpoint));
        message_set_flush(pMessage, usFlush);
        vlist_add_tail(&pConnection->m_outstanding.list, pMessage);
        message_get_type(pMessage);
    }
    else
    {
        message_destroy(pMessage);
    }
    return 0;
}

// A state message goes out as a DELTA: XOR'd against the last copy the far end
// acknowledged (the head of the channel's tx list, when its sequence number is
// the acked one) and run-length encoded, so an unchanged byte costs nothing.
// The delta byte says how far back that reference is.  A channel 4,096 ahead
// of its acknowledgements is terminated rather than allowed to wrap.
int connection_append_state(connection_t *pConnection, buffer_t *buffer, message_t *pMessage,
                            message_list_t *pList, uns16 usFlush)
{
    uns8 data[1300];
    message_t *pCopy;
    int iDelta = 0;
    uns8 ucChannel = message_get_type(pMessage);
    uns16 usSize = message_get_size(pMessage);
    state_channel_t *pChannel = &pConnection->m_pChannels[ucChannel];
    uns16 usSeqnum;
    uns16 usNext;
    message_t *pReference;
    message_t *pSent;
    int result;

    if (buffer->position > 300)
        return -1;
    if (buffer->size < usSize + buffer->position + 27)
        return -1;
    usNext = pChannel->m_usTx + 1;
    if ((int16)(usNext - pChannel->m_usTxAck) == 4096)
    {
        connection_terminate_reason(pConnection, 1, 8);
        return -1;
    }
    if (pChannel->m_ucWhere == 1)
    {
        message_set_seqnum(pMessage, pChannel->m_usTx++);
        result = message_create(&pCopy, pConnection->endpoint, pConnection,
                                 pConnection->local_id, pConnection->remote_id,
                                 message_get_data(pMessage), message_get_size(pMessage),
                                 message_get_type(pMessage), message_get_seqnum(pMessage),
                                 endpoint_get_time(pConnection->endpoint));
        VASSERT(result == 0);   // 1586
        vlist_add_tail(&pChannel->m_txList.list, pCopy);
    }
    usSeqnum = message_get_seqnum(pMessage);
    message_seqnum_remove_thru(&pChannel->m_txList, usSeqnum - 32);
    pReference = (message_t *)vlist_peek_head(&pChannel->m_txList.list);
    if (pReference)
    {
        if (message_get_seqnum(pReference) == pChannel->m_usTxAck)
        {
            uns32 uiCommon;

            iDelta = (uns8)(usSeqnum - message_get_seqnum(pReference));
            uiCommon = DRDP_MIN(message_get_size(pMessage), message_get_size(pReference));
            xor_into(data, (const uns8 *)message_get_data(pMessage),
                     (const uns8 *)message_get_data(pReference), uiCommon);
            memcpy(data + uiCommon, (uns8 *)message_get_data(pMessage) + uiCommon,
                   message_get_size(pMessage) - uiCommon);
        }
        else
            memcpy(data, message_get_data(pMessage), message_get_size(pMessage));
    }
    pSent = (message_t *)vlist_remove_head(&pList->list);
    buffer_append_uns8(buffer, ucChannel);
    buffer_append_size(buffer, usSize);
    buffer_append_uns16(buffer, usSeqnum);
    buffer_append_uns8(buffer, iDelta);
    run_length_encode(data, usSize, buffer);

























    VASSERT(!buffer->overflow);   // 1639
    pChannel->m_ucWhere = 2;
    message_set_time(pSent, endpoint_get_time(pConnection->endpoint));
    message_set_flush(pSent, usFlush);
    vlist_add_tail(&pConnection->m_outstanding.list, pSent);
    return 0;
}

int connection_append(connection_t *pConnection, buffer_t *pBuffer, message_list_t *pList,
                      uns16 usFlush)
{
    message_t *pMessage = (message_t *)vlist_peek_head(&pList->list);
    int iType = message_get_type(pMessage);

    if (iType < CONNECTION_MESSAGE_TYPE_TERMINATE)
        if (iType >= CONNECTION_MESSAGE_TYPE_SYSTEM)
            return connection_append_message(pConnection, pBuffer, pMessage, pList, usFlush);
    return connection_append_state(pConnection, pBuffer, pMessage, pList, usFlush);
}

// When to put this message on the wire: the round trip plus a hundred
// milliseconds, unless the far end is behind on flush numbers, in which case
// it has already waited long enough.
uns32 connection_message_get_desired_flush_time(connection_t *pConnection, message_t *pMessage)
{
    uns32 uiTime = message_get_time(pMessage) + pConnection->m_usTimeout + 100;

    if (pConnection->m_usFlags & 0x10)
    {
        uns16 usAck = pConnection->m_usFlushAck;

        if ((int16)(usAck - message_get_flush(pMessage)) > 0)
            uiTime = message_get_time(pMessage);
    }
    return uiTime;
}

// Which of the two queues wants the earlier flush.  `iHave` is zero on the
// first call, so the send queue's own time is taken as the starting point
// rather than compared against a garbage one.
void connection_get_next_message(connection_t *pConnection, message_list_t **ppList,
                                 uns32 *puiTime, int iHave)
{
    message_list_t *pList;
    message_t *pMessage;
    *ppList = 0;
    pList = &pConnection->m_sendQueue;
    pMessage = (message_t *)vlist_peek_head(&pList->list);
    if (pMessage)
    {
        if (iHave)
        {
            if ((int)(message_get_time(pMessage) - *puiTime) < 0)
            {
                *puiTime = message_get_time(pMessage);
                *ppList = pList;
            }
        }
        else
        {
            iHave = 1;
            *puiTime = message_get_time(pMessage);
            *ppList = pList;
        }
    }
    pList = &pConnection->m_outstanding;
    pMessage = (message_t *)vlist_peek_head(&pList->list);
    if (pMessage)
    {
        uns32 uiTime = connection_message_get_desired_flush_time(pConnection, pMessage);
        if (iHave)
        {
            if ((int)(uiTime - *puiTime) < 0)
            {
                *puiTime = uiTime;
                *ppList = pList;
            }
        }
        else
        {
            *puiTime = uiTime;
            *ppList = pList;
        }
    }
}











































































































































































// Two software endpoints never exchange a segment, so nothing else would ever
// notice one of them going away.  This is the sweep that does: two minutes
// with nothing queued resets the send half, six with no arrival destroys it.
int connection_idle_check(connection_t *pConnection)
{
    int iResult = 0;

    if (connection_is_sw_sw(pConnection))
    {
        if ((pConnection->m_usFlags & 0x200) && (pConnection->m_usFlags & 0x40)
            && !vlist_get_size(&pConnection->m_sendQueue.list)
            && !vlist_get_size(&pConnection->m_outstanding.list)
            && !(pConnection->m_usFlags & 0xf)
            && uns32_ge(endpoint_get_time(pConnection->endpoint),
                        pConnection->time_last_flush + CONNECTION_IDLE_RESET))
        {
            connection_tx_reset(pConnection);
            iResult = 1;
        }
        if ((~pConnection->m_usFlags & 0x200) && (pConnection->m_usFlags & 0x20)
            && !vlist_get_size(&pConnection->m_delayed.list)
            && !vlist_get_size(&pConnection->m_arrivals.list)
            && uns32_ge(endpoint_get_time(pConnection->endpoint),
                        pConnection->time_last_arrival + 360000))
        {
            iResult |= 2;
            drdp_format_endpoint1(pConnection->local_id);
            drdp_format_endpoint2(pConnection->remote_id);
            _endpoint_connection_destroy(pConnection, __FILE__, __LINE__);   // 1923
        }
    }
    return iResult;
}




































// One segment out of this connection into the route's packet.  The header byte
// says which acknowledgements follow -- bit 0 arrivals (and bit 2 their mask),
// bit 1 guaranteed (and bit 3 its mask), bit 4 state channels, bit 5 "my rx
// flag is low", bit 6 the far instance -- and then as many messages as fit,
// in desired-flush-time order.  A reliable message unacknowledged for longer
// than the fail time terminates the connection from here.
int connection_flush(connection_t *connection, buffer_t *pBuffer, int iFlushNumber)
{
    uns16 usFlush = connection->m_usFlush;
    uns8 ucFlags = 0;
    uns64 uiAckMask;
    message_list_t *pList;
    uns32 uiTime;
    int iResult;
    int i;

    drdp_format_endpoint1(connection->local_id);
    drdp_format_endpoint2(connection->remote_id);
    endpoint_get_time(connection->endpoint);
    connection->m_iLastRouteFlushNumber = iFlushNumber;
    if (endpoint_is_closed(connection->endpoint) && endpoint_is_in_sync(connection->endpoint))
    {
        drdp_format_endpoint1(connection->local_id);
        drdp_endpoint_destroy(connection->endpoint);
        iResult = -3;
        goto done;
    }
    if (connection_is_closed(connection) && connection_is_in_sync(connection))
    {
        drdp_format_endpoint1(connection->local_id);
        drdp_format_endpoint2(connection->remote_id);
        connection_terminate_reason(connection, 1, 10);
        _endpoint_connection_destroy(connection, __FILE__, __LINE__);   // 1996
        iResult = -3;
        goto done;
    }
    iResult = connection_idle_check(connection);
    if (iResult & 2)
    {
        iResult = -3;
        goto done;
    }
    if (iResult & 1)
    {
        iResult = -1;
        goto done;
    }
    if (!connection_is_sw_sw(connection) && !connection_is_closed(connection)
        && !endpoint_is_closed(connection->endpoint)
        && uns32_ge(endpoint_get_time(connection->endpoint),
                    connection->m_uiTimeSend + 30000))
    {
        uns8 ucKeepAlive = 18;

        drdp_format_endpoint1(connection->local_id);
        drdp_format_endpoint2(connection->remote_id);
        connection_send_internal(connection, &ucKeepAlive, 1, CONNECTION_MESSAGE_TYPE_SYSTEM);
        if (connection->m_usFlags & 0x100)
        {
            iResult = -1;
            goto done;
        }
    }
    uiAckMask = 0;
    if ((connection->m_usFlags & 1) || (connection->m_usFlags & 2))
    {
        connection_get_ack_mask(connection, &uiAckMask);
        ucFlags |= 2;
        if (uiAckMask)
            ucFlags |= 8;
    }
    if (connection->m_usFlags & 4)
        ucFlags |= 0x10;
    if (connection->m_usFlags & 1)
    {
        ucFlags |= 1;
        if (connection->m_uiRxMask)
            ucFlags |= 4;
    }
    if (connection->m_usFlags & 8)
        ucFlags |= 0x40;
    if (connection->m_ucRxFlag < 17)
        ucFlags |= 0x20;
    buffer_append_uns8(pBuffer, ucFlags);
    if (ucFlags & 0x40)
        buffer_append_uns32(pBuffer, connection->m_uiInstanceRemote);
    buffer_append_uns16(pBuffer, usFlush);
    if (ucFlags & 1)
    {
        buffer_append_uns16(pBuffer, connection->m_usArrivalNumber);
        if (ucFlags & 4)
            buffer_append_packed_uns64(pBuffer, connection->m_uiRxMask);
    }
    if (ucFlags & 2)
    {
        buffer_append_uns16(pBuffer, connection->m_usRxSeqnum);
        if (ucFlags & 8)
            buffer_append_packed_uns64(pBuffer, uiAckMask);
    }
    if (ucFlags & 0x10)
    {
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
        {
            if (connection->m_pChannels[i].m_ucRxFlag)
            {
                buffer_append_uns8(pBuffer, i);
                buffer_append_uns16(pBuffer, connection->m_pChannels[i].rx);
                connection->m_pChannels[i].m_ucRxFlag--;
            }
        }
        buffer_append_uns8(pBuffer, CONNECTION_STATE_CHANNEL_COUNT);
    }
    if (pBuffer->overflow)
    {
        iResult = -2;
        goto done;
    }
    for (;;)
    {
        pList = 0;
        connection_get_next_message(connection, &pList, &uiTime, 0);
        if (!pList)
            break;
        if (drdp_is_hardware_endpoint(connection->local_id)
            || drdp_is_hardware_endpoint(connection->remote_id))
        {
            message_t *pHead = (message_t *)vlist_peek_head(&pList->list);
            uns32 uiAge = endpoint_get_time(connection->endpoint) - message_get_created(pHead);
            uns32 uiSilence = endpoint_get_time(connection->endpoint)
                              - connection->time_last_arrival;
            int iTimeout;

            if (connection->m_usFlags & 0x40)
                iTimeout = route_is_terminated(connection->m_pRoute) ? connection->m_iRetryTime
                                                                      : connection->m_iWarnTime;
            else
                iTimeout = connection->m_iRetryTime;
            if ((uns32)connection->m_iFailTime < uiAge)
            {
                connection_terminate_reason(connection, 1, 3);
                iResult = -1;
                goto done;
            }
            if (pList == &connection->m_outstanding && iTimeout < (int)uiAge
                && iTimeout < (int)uiSilence)
            {
                connection_terminate_reason(connection, 1, 2);
                iResult = -1;
                goto done;
            }
        }
        if (endpoint_get_time(connection->endpoint) < uiTime)
            break;
        if (connection_append(connection, pBuffer, pList, usFlush))
            break;
    }
    if (~connection->m_usFlags & 0x40)
        connection->m_usTimeout += 1000;
    if (connection->m_usFlags & 0x200)
    {




















































































































        VASSERT(uns32_gt( connection->time_last_flush + 2 * CONNECTION_IDLE_RESET, endpoint_get_time( connection->endpoint ) ));   // 2240
    }
    else
    {
        connection->m_uiTimeInstanceSent = endpoint_get_time(connection->endpoint);
        connection->time_last_arrival = endpoint_get_time(connection->endpoint);
        connection->m_usFlags |= 0x200;
    }
    connection->m_usFlags &= 0xfff0;
    connection->m_usFlush++;
    connection->m_uiCstat[6]++;
    connection->m_uiFlushTime[usFlush & 1023] = endpoint_get_time(connection->endpoint);
    connection->time_last_flush = endpoint_get_time(connection->endpoint);
    iResult = 0;
done:
    return iResult;
}


uns32 connection_get_desired_flush_time(connection_t *pConnection)
{
    return pConnection->m_uiDesiredFlushTime;
}

// The round trip history is sixteen deep and the TIMEOUT is the largest of
// them, where `get_ping_time` below reports the smallest.  The index advances
// as `++` then `%= 16` on the MEMBER: flow deletes the first store, and the
// pair leaves `andi 0xff` + `andi 0xf` that no one-expression spelling gives.
void connection_add_rtt_sample(connection_t *pConnection, uns16 usSample)
{
    int i;
    pConnection->m_usRtt[pConnection->m_ucRttIndex] = usSample;
    pConnection->m_usTimeout = pConnection->m_usRtt[0];
    for (i = 1; i < 16; i++)
        pConnection->m_usTimeout = DRDP_MAX(pConnection->m_usRtt[i], pConnection->m_usTimeout);
    pConnection->m_ucRttIndex++;
    pConnection->m_ucRttIndex %= 16;
}







// A software-to-software connection never really talks to anything, so its
// flush time is pushed a long way out; a real one wakes up thirty seconds
// after its last send, or a whole day once it is closed.
void connection_recalc_desired_flush_time(connection_t *connection)
{
    message_list_t *pList;

    if (connection_is_sw_sw(connection))
    {
        if (connection->m_usFlags & 0x200)
            connection->m_uiDesiredFlushTime = connection->time_last_flush + 120000;
        else if (connection->m_usFlags & 0x20)
            connection->m_uiDesiredFlushTime = connection->time_last_arrival + 360000;
        else
            connection->m_uiDesiredFlushTime = connection->time_last_arrival + 86400000;
    }
    else if (connection_is_closed(connection) || connection_is_terminated(connection))
    {
        VASSERT(endpoint_get_time( connection->endpoint )
                - connection->time_last_arrival < ONE_DAY/2);   // 2304
        connection->m_uiDesiredFlushTime =
            endpoint_get_time(connection->endpoint) + 86400000;
    }
    else
    {
        connection->m_uiDesiredFlushTime = connection->m_uiTimeSend + 30000;
    }
    connection_get_next_message(connection, &pList, &connection->m_uiDesiredFlushTime, 1);
    if (connection->m_usFlags & 0xf)
        if ((int)(connection->m_uiDesiredFlushTime - connection->m_uiFlushBy) >= 0)
            connection->m_uiDesiredFlushTime = connection->m_uiFlushBy;
    if (connection_is_closed(connection) && connection_is_in_sync(connection))
    {
        drdp_format_endpoint1(connection_get_src(connection));
        drdp_format_endpoint2(connection_get_dst(connection));
        connection->m_uiDesiredFlushTime = connection->time_last_arrival;
    }
    if (endpoint_is_closed(connection->endpoint)
        && endpoint_is_in_sync(connection->endpoint))
    {
        drdp_format_endpoint1(connection_get_src(connection));
        drdp_format_endpoint2(connection_get_dst(connection));
        connection->m_uiDesiredFlushTime = connection->time_last_arrival;
    }
}

// Everything up to and including `usSeqnum` is acknowledged, and the mask
// covers the sixty-four after that -- so one word retires up to sixty-five
// messages.  Only the three reliable types are on this list; a state channel
// message on it is left alone.
void connection_record_guaranteed_ack(connection_t *pConnection, uns16 usSeqnum, uns64 uiMask)
{
    message_t *pMessage;
    message_t *pNext;
    for (pMessage = (message_t *)vlist_peek_head(&pConnection->m_outstanding.list); pMessage;
         pMessage = pNext)
    {
        pNext = (message_t *)vlist_get_next(&pConnection->m_outstanding.list, pMessage);
        if (message_get_type(pMessage) == CONNECTION_MESSAGE_TYPE_GUARANTEED
            || message_get_type(pMessage) == CONNECTION_MESSAGE_TYPE_BIG
            || message_get_type(pMessage) == CONNECTION_MESSAGE_TYPE_SYSTEM)
        {
            int iAhead = (int16)(message_get_seqnum(pMessage) - usSeqnum);
            if (iAhead <= 0)
            {
                vlist_remove(&pConnection->m_outstanding.list, pMessage);
                message_destroy(pMessage);
            }
            else if (iAhead != 1)
            {
                uns32 uiBit = iAhead - 2;
                if (uiBit < 64)
                {
                    uns64 uiFlag = 1;
                    uiFlag <<= uiBit;
                    if (uiFlag & uiMask)
                {
                        vlist_remove(&pConnection->m_outstanding.list, pMessage);
                        message_destroy(pMessage);
                    }
                }
            }
        }
    }
    pConnection->m_usTxAck = usSeqnum;
    pConnection->m_uiTxMask = uiMask;
}

// One acknowledgement per state channel, the array ending at channel 248.
void connection_record_state_acks(connection_t *pConnection, const state_ack_t *pAcks)
{
    int i;
    int iChannel;
    uns16 usSeqnum;
    for (i = 0; (iChannel = pAcks[i].m_ucChannel, usSeqnum = pAcks[i].m_usSeqnum,
            iChannel != CONNECTION_STATE_CHANNEL_COUNT); i++)
    {
        state_channel_t *pChannel = &pConnection->m_pChannels[iChannel];
        if ((int16)(usSeqnum - pChannel->m_usTxAck) > 0)
        {
            if (pChannel->m_ucWhere == 2
                && message_get_seqnum(pChannel->m_pPending) == usSeqnum)
            {
                pChannel->m_ucWhere = 0;
                vlist_remove(&pConnection->m_outstanding.list, pChannel->m_pPending);
                message_destroy(pChannel->m_pPending);
                pChannel->m_pPending = 0;
            }
            pChannel->m_usTxAck = usSeqnum;
            message_seqnum_remove_thru(&pChannel->m_txList, pChannel->m_usTxAck);
        }
    }
}

// The far end says which flush number it saw; the time that one went out is
// still in the ring, so the difference IS the round trip.
void connection_record_flush_ack(connection_t *pConnection, uns16 usFlush)
{
    uns32 *puiTime;
    uns32 *puiRing;
    if (uns16_lt(usFlush, pConnection->m_usFlush - 1024))
        return;
    puiRing = pConnection->m_uiFlushTime;
    puiTime = &puiRing[usFlush & 1023];
    if (*puiTime)
    {
        uns32 uiRtt = endpoint_get_time(pConnection->endpoint) - *puiTime;

        *puiTime = 0;
        connection_add_rtt_sample(pConnection, (uns16)(uiRtt > 0xffff ? 0xffff : uiRtt));
    }
}

void connection_record_acks(connection_t *pConnection, segment_body_t *pBody)
{
    if (pBody->m_ucFlags & 0x40)
        pConnection->m_usFlags |= 0x40;
    if (pBody->m_ucFlags & 1)
    {
        connection_record_flush_ack(pConnection, pBody->m_usFlush);
        if (pBody->m_ucFlags & 4)
        {
            uns64 uiMask = pBody->m_uiFlushMask;
            uns16 usFlush = pBody->m_usFlush;

            while (uiMask)
            {
                usFlush = usFlush - 1;
                if (uiMask & 1)
                    connection_record_flush_ack(pConnection, usFlush);
                uiMask >>= 1;
            }
        }
        if ((int16)(pBody->m_usFlush - pConnection->m_usFlushAck) > 0)
            pConnection->m_usFlushAck = pBody->m_usFlush;
    }
    if (pBody->m_iGuaranteedAck)
        connection_record_guaranteed_ack(pConnection, pBody->m_usSeqnum, pBody->m_uiMask);
    if (pBody->m_ucFlags & 0x10)
        connection_record_state_acks(pConnection, pBody->m_acks);
}
















































































































































































// A guaranteed message that arrived in order, or one that completes a run of
// them: a FRAGMENT is held until the last piece lands, and then all of them
// are copied into one message and that is what the application sees.
void connection_move_delayed_to_arrivals(connection_t *pConnection)
{
    message_t *pMessage;
    while ((pMessage = message_seqnum_peek_head(&pConnection->m_delayed)) != 0
           && message_get_seqnum(pMessage) == (uns16)(pConnection->m_usRxSeqnum + 1))
    {
        pConnection->m_usRxSeqnum = pConnection->m_usRxSeqnum + 1;
        vlist_remove_head(&pConnection->m_delayed.list);
        if (message_get_type(pMessage) == CONNECTION_MESSAGE_TYPE_BIG)
        {
            vlist_add_tail(&pConnection->m_arrivals.list, pMessage);
        }
        else if (vlist_get_size(&pConnection->m_arrivals.list) == 0)
        {
            endpoint_arrival_enqueue(pConnection->endpoint, pMessage);
        }
        else
        {
            message_t *pPiece;
            uns32 total_size = 0;
            uns32 uiOffset;
            int result;
            vlist_add_tail(&pConnection->m_arrivals.list, pMessage);
            for (pPiece = (message_t *)vlist_peek_head(&pConnection->m_arrivals.list); pPiece;
                 pPiece = (message_t *)vlist_get_next(&pConnection->m_arrivals.list, pPiece))
                total_size += message_get_size(pPiece);
            VASSERT(total_size <= DRDP_SEND_SIZE_MAX_GUARANTEED);   // 2651
            result = message_create(&pMessage, pConnection->endpoint, pConnection,
                                     pConnection->remote_id, pConnection->local_id, 0,
                                     total_size, message_get_type(pMessage),
                                     message_get_seqnum((message_t *)vlist_peek_head(
                                         &pConnection->m_arrivals.list)),
                                     endpoint_get_time(pConnection->endpoint));






            VASSERT(result == 0);   // 2664
            uiOffset = 0;
            while ((pPiece = (message_t *)vlist_remove_head(&pConnection->m_arrivals.list)) != 0)
            {
                memcpy((uns8 *)message_get_data(pMessage) + uiOffset, message_get_data(pPiece),
                       message_get_size(pPiece));
                uiOffset += message_get_size(pPiece);
                message_destroy(pPiece);
            }
            endpoint_arrival_enqueue(pConnection->endpoint, pMessage);
        }
    }
}

// One state channel message that arrived.  Its own sequence number retires
// everything older on the channel; the refnum offset says how many further
// back the delta it carries was computed against.
void connection_record_state_message(connection_t *pConnection, message_t *pMessage,
                                     message_t *pWith)
{
    uns16 usSeqnum = message_get_seqnum(pMessage);
    state_channel_t *pChannel = &pConnection->m_pChannels[message_get_type(pMessage)];
    uns8 ucRefnum;

    message_get_type(pMessage);
    pChannel->rx = usSeqnum;
    pChannel->m_ucRxFlag = 2;
    ucRefnum = message_get_refnum_offset(pMessage);
    if (ucRefnum == 0)
        message_seqnum_remove_thru(&pChannel->m_rxList, usSeqnum - 32);
    else
        message_seqnum_remove_thru(&pChannel->m_rxList, usSeqnum - ucRefnum);
    if (pWith)
        vlist_add_tail(&pChannel->m_rxList.list, pWith);
    endpoint_arrival_enqueue(pConnection->endpoint, pMessage);
}

void connection_record_message(connection_t *pConnection, message_t *pMessage,
                               message_list_t *pList)
{
    switch (message_get_type(pMessage))
    {
    case CONNECTION_MESSAGE_TYPE_BEST_EFFORT:
    case 254:
    case 255:
        endpoint_arrival_enqueue(pConnection->endpoint, pMessage);
        break;
    case CONNECTION_MESSAGE_TYPE_SYSTEM:
    case CONNECTION_MESSAGE_TYPE_BIG:
    case CONNECTION_MESSAGE_TYPE_GUARANTEED:
        vlist_insert(&pConnection->m_delayed.list, pMessage);
        connection_move_delayed_to_arrivals(pConnection);
        break;
    default:
        connection_record_state_message(pConnection, pMessage,
                                        (message_t *)vlist_remove_head(&pList->list));
        break;
    }
}

void connection_record_segment_messages(connection_t *pConnection, segment_body_t *pBody)
{
    message_t *pMessage;

    if (pBody->m_usConnFlags)
    {
        int i;

        if (!(pConnection->m_usFlags & 0xf))
            pConnection->m_uiFlushBy = endpoint_get_time(pConnection->endpoint) + 100;
        pConnection->m_usFlags |= pBody->m_usConnFlags;
        for (i = 0; i < pBody->m_iChannelCount; i++)
            pConnection->m_pChannels[pBody->m_ucChannels[i]].m_ucRxFlag = 2;
    }
    while ((pMessage = (message_t *)vlist_remove_head(&pBody->m_messages.list)) != 0)
        connection_record_message(pConnection, pMessage, &pBody->m_messages);
}

// The arrival history is a sliding bitmap of the last sixty-four segments, so
// the far end can be told which ones went missing without keeping any of them.
void connection_record_segment(connection_t *pConnection, const segment_header_t *pHeader,
                               segment_body_t *pBody)
{
    pConnection->time_last_arrival = endpoint_get_time(pConnection->endpoint);
    if (pHeader->flags & 0x2000)
    {
        pConnection->m_uiTimeLastInstanceArrival = endpoint_get_time(pConnection->endpoint);
        if (~pConnection->m_usFlags & 0x20)
        {
            pConnection->m_usFlags |= 0x20;
            pConnection->m_uiInstanceRemote = pHeader->m_uiCrc;
        }
        if (pHeader->flags & 0x80000)
        {
            if (!(pConnection->m_usFlags & 0xf))
                pConnection->m_uiFlushBy = endpoint_get_time(pConnection->endpoint) + 100;
            pConnection->m_usFlags |= 9;
        }
    }
    if (pBody->m_iArrival)
    {
        if (pConnection->m_ucRxFlag < 255)
            pConnection->m_ucRxFlag = pConnection->m_ucRxFlag + 1;
        if (pBody->m_ucFlags & 0x20)
            pConnection->m_usFlags &= 0xffef;
        else
            pConnection->m_usFlags |= 0x10;
    }
    else
        pConnection->m_ucRxFlag = 0;
    if (pBody->m_iArrival)
    {
        pConnection->m_uiArrivalHistory = (pConnection->m_uiArrivalHistory << 1) | 1;
        pConnection->m_usArrivalNumber = pConnection->m_usArrivalNumber + 1;
        if (uns16_gt(pBody->m_usArrivalNumber, pConnection->m_usArrivalNumber))
            pConnection->m_uiArrivalHistory <<=
                (int16)(pBody->m_usArrivalNumber - pConnection->m_usArrivalNumber);
        pConnection->m_usArrivalNumber = pBody->m_usArrivalNumber;
    }
    else
    {
        pConnection->m_uiArrivalHistory |=
            (uns64)1 << ((int16)(pConnection->m_usArrivalNumber - pBody->m_usArrivalNumber) - 1);
    }
    if (pBody->m_iAcks)
        connection_record_acks(pConnection, pBody);
    connection_record_segment_messages(pConnection, pBody);
    route_resort(pConnection);
}

// Have we already seen this guaranteed sequence number?  Either it is behind
// the window or it is sitting on one of the two sorted lists.
int connection_has_reliable(connection_t *pConnection, uns16 usSeqnum)
{
    int iHave = 0;

    if (uns16_le(usSeqnum, pConnection->m_usRxSeqnum)
        || vlist_lookup(&pConnection->m_arrivals.list, &usSeqnum)
        || vlist_lookup(&pConnection->m_delayed.list, &usSeqnum))
        iHave = 1;
    return iHave;
}

// A segment that would not parse is reported to the application's logger as
// a possible attack, with the connection's name, rather than printed.
void connection_log_parse_failure(connection_t *pConnection, const char *pFormat, ...)
{
    char szText[4096];
    va_list args;

    if (pConnection->m_pLogger)
    {
        va_start(args, pFormat);
        vsprintf(szText, pFormat, args);
        pConnection->m_pLogger->hack(pConnection->m_szName, szText);
    }
}



















































































































































































































































































































































// Parse one segment's body.  Nothing is applied here: the acknowledgements go
// into the body and every message is built and queued on `pBody->m_messages`,
// and only `connection_record_segment` touches the connection.  Every check
// that fails is logged to the connection's logger as a possible attack.  With
// no connection (or no body) the same walk runs as a pure validation.
//
// The result starts at 0x80000000 and most failures leave it there; only some
// set -1.  Either way it is non-zero, which is all the caller tests.
int connection_parse_segment_body(connection_t *pConnection, const segment_header_t *pHeader,
                                  segment_body_t *pBody, buffer_t *pBuffer)
{
    segment_body_t body;
    uns8 ucType;
    uns16 size;
    uns16 usSeqnum;
    uns8 ucRefnumOffset;
    message_t *pReset;
    message_t *pMessage;
    message_t *pCopy;
    int iResult;
    if (pConnection)
        pConnection->m_uiCstat[5]++;
    iResult = 0x80000000;
    if (!pBody)
    {
        memset(&body, 0, sizeof(body));
        pBody = &body;
    }
    if ((pHeader->flags & 0x2000) && pConnection)
    {
        if (pConnection->m_usFlags & 0x20)
        {
            if (pConnection->m_uiInstanceRemote != pHeader->m_uiCrc)
            {
                iResult = -1;
                pConnection->m_uiCstat[0]++;
                goto out;
            }
        }
        else if (~pHeader->flags & 0x80000)
        {
            iResult = -1;
            connection_log_parse_failure(pConnection, "connection has NOT received an instance the remote instance MUST be asking for an instance ack (routers w/o UDP state?)\n");
            pConnection->m_uiCstat[2]++;
            goto out;
        }
    }
    buffer_read_uns8(pBuffer, &pBody->m_ucFlags);
    if (pBody->m_ucFlags & 0x40)
        buffer_read_uns32(pBuffer, &pBody->m_uiInstance);
    pBody->m_iAcks = 1;
    if (pConnection && (~pConnection->m_usFlags & 0x40))
    {
        if (~pBody->m_ucFlags & 0x40)
            pBody->m_iAcks = 0;
        else if (pBody->m_uiInstance != pConnection->m_uiInstanceLocal)
            pBody->m_iAcks = 0;
    }
    pBody->m_iArrival = 1;
    buffer_read_uns16(pBuffer, &pBody->m_usArrivalNumber);
    if (pConnection)
    {
        int iIncrement = (int16)(pBody->m_usArrivalNumber - pConnection->m_usArrivalNumber);
        if (iIncrement <= 0)
        {
            int iBack = ~iIncrement;
            uns64 uiBit = 1;
            uiBit <<= iBack;
            if (iIncrement == 0 || (uiBit & pConnection->m_uiArrivalHistory))
            {
                iResult = -1;
                goto out;
            }
            if (iBack >= 64)
            {
                iResult = -1;
                pConnection->m_uiCstat[0]++;
                goto out;
            }
            pBody->m_iArrival = 0;
        }
        else if (iIncrement > 4096)
        {
            connection_log_parse_failure(pConnection, "increment (%u) > CONNECTION_FLUSH_INCREMENT_MAX\n", iIncrement);
            iResult = -1;
            pConnection->m_uiCstat[2]++;
            goto out;
        }
        else if (iIncrement > 1 && connection_get_leader(pConnection))
        {
            drdp_format_endpoint1(pConnection->remote_id);
            drdp_format_endpoint2(pConnection->local_id);
        }
    }
    if (pBody->m_ucFlags & 1)
    {
        buffer_read_uns16(pBuffer, &pBody->m_usFlush);
        if (pBody->m_ucFlags & 4)
            buffer_read_packed_uns64(pBuffer, &pBody->m_uiFlushMask);
        if (pConnection && pBody->m_iAcks
            && uns16_ge(pBody->m_usFlush, pConnection->m_usFlush))
        {
            iResult = -1;
            connection_log_parse_failure(pConnection, "segment is acking a segment that hasn't been sent yet segment_body->flush_ack (%u) >= connection->flush_tx (%u)\n",
                                         pBody->m_usFlush, pConnection->m_usFlush);
            pConnection->m_uiCstat[2]++;
            goto out;
        }
    }
    pBody->m_iGuaranteedAck = 0;
    if (pBody->m_ucFlags & 2)
    {
        pBody->m_iGuaranteedAck = pBody->m_iAcks;
        buffer_read_uns16(pBuffer, &pBody->m_usSeqnum);
        if (pBody->m_ucFlags & 8)
            buffer_read_packed_uns64(pBuffer, &pBody->m_uiMask);
        if (pConnection && pBody->m_iAcks)
        {
            int iAcked;
            uns64 uiMask;
            if (uns16_ge(pBody->m_usSeqnum, pConnection->m_usTxSeqnum))
            {
                iResult = -1;
                connection_log_parse_failure(pConnection, "segment_body->guaranteed_ack (%u) >= connection->guaranteed_tx (%u)\n",
                                             pBody->m_usSeqnum, pConnection->m_usTxSeqnum);
                pConnection->m_uiCstat[2]++;
                goto out;
            }
            if (uns16_lt(pBody->m_usSeqnum, pConnection->m_usTxAck))
                pBody->m_iGuaranteedAck = 0;
            iAcked = uns16_diff(pBody->m_usSeqnum, pConnection->m_usTxAck);
            if (iAcked > 4096)
            {
                connection_log_parse_failure(pConnection, "messages_acked (%u - %u = %u) > CONNECTION_MAX_OUTSTANDING\n",
                                             pBody->m_usSeqnum, pConnection->m_usTxAck, iAcked);
                iResult = -1;
                pConnection->m_uiCstat[2]++;
                goto out;
            }
            if (iAcked >= 64)
                uiMask = 0;
            else
                uiMask = pConnection->m_uiTxMask >> iAcked;
            if ((pBody->m_uiMask ^ uiMask) & uiMask)
                pBody->m_iGuaranteedAck = 0;
            if (pBody->m_iArrival && !pBody->m_iGuaranteedAck)
            {
                iResult = -1;
                connection_log_parse_failure(pConnection, "segment_body->guaranteed_ack_mask: some of the previously acked packets have become \"un-acked\"\n");
                pConnection->m_uiCstat[2]++;
                goto out;
            }
        }
    }
    if (pBody->m_ucFlags & 0x10)
    {
        int i;
        for (i = 0; ; i++)
        {
            buffer_read_uns8(pBuffer, &pBody->m_acks[i].m_ucChannel);
            if (pBody->m_acks[i].m_ucChannel == CONNECTION_STATE_CHANNEL_COUNT)
                break;
            if (pBody->m_acks[i].m_ucChannel > CONNECTION_STATE_CHANNEL_COUNT)
            {
                connection_log_parse_failure(pConnection, "segment_body->state_ack[i].channel > STATE_CHANNEL_COUNT\n");
                if (pConnection)
                    pConnection->m_uiCstat[2]++;
                iResult = -1;
                goto out;
            }
            if (i > 0 && pBody->m_acks[i - 1].m_ucChannel >= pBody->m_acks[i].m_ucChannel)
            {
                connection_log_parse_failure(pConnection, "state acks must come in increasing order\n");
                if (pConnection)
                    pConnection->m_uiCstat[2]++;
                iResult = -1;
                goto out;
            }
            if (i == CONNECTION_STATE_CHANNEL_COUNT)
            {
                connection_log_parse_failure(pConnection, "this should've been the terminator\n");
                if (pConnection)
                    pConnection->m_uiCstat[2]++;
                iResult = -1;
                goto out;
            }
            buffer_read_uns16(pBuffer, &pBody->m_acks[i].m_usSeqnum);
        }
        if (pConnection && pBody->m_iAcks)
        {
            int j;
            if (!pConnection->m_pChannels)
            {
                iResult = -1;
                connection_log_parse_failure(pConnection, "there are no state channels on this connection, how could we be getting state acks\n");
                pConnection->m_uiCstat[2]++;
                goto out;
            }
            int iChannel;
            uns16 usAck;
            for (j = 0; (iChannel = pBody->m_acks[j].m_ucChannel, usAck = pBody->m_acks[j].m_usSeqnum,
                    iChannel != CONNECTION_STATE_CHANNEL_COUNT); j++)
            {
                state_channel_t *pChannel = &pConnection->m_pChannels[iChannel];
                if (uns16_ge(usAck, pChannel->m_usTx))
                {
                    iResult = -1;
                    connection_log_parse_failure(pConnection, "can't ack more than we've sent, yo: seqnum (%u) >= connection->state_channel[channel].tx (%u)\n",
                                                 usAck, pChannel->m_usTx);
                    pConnection->m_uiCstat[2]++;
                    goto out;
                }
                if (pBody->m_iArrival && uns16_lt(usAck, pChannel->m_usTxAck))
                {
                    iResult = -1;
                    connection_log_parse_failure(pConnection, "can't ack less than we've already acked: seqnum (%u) < connection->state_channel[channel].ack (%u)\n",
                                                 usAck, pChannel->m_usTxAck);
                    pConnection->m_uiCstat[2]++;
                    goto out;
                }
            }
        }
    }
    if (pConnection
        && (drdp_is_hardware_endpoint(pHeader->src) || drdp_is_hardware_endpoint(pHeader->dst))
        && (~pConnection->m_usFlags & 0x20))
    {
        iResult = message_create(&pReset, pConnection->endpoint, pConnection, pHeader->src,
                                 pHeader->dst, 0, 0, 255, 0xffff,
                                 endpoint_get_time(pConnection->endpoint));
        if (iResult)
        {
            iResult = -1;
            pConnection->m_uiCstat[1]++;
            goto out;
        }
        vlist_add_tail(&pBody->m_messages.list, pReset);
    }
    while (buffer_bytes_remaining(pBuffer))
    {
        buffer_read_uns8(pBuffer, &ucType);
        buffer_read_size(pBuffer, &size);
        if (size >= 2047)
        {
            connection_log_parse_failure(pConnection, "size (%u) >= SEGMENT_HEADER_SEGMENT_BODY_SIZE (%u)\n",
                                         size, 2047);
            goto out;
        }
        if ((uns8)(ucType + 7) < 3)
        {
            buffer_read_uns16(pBuffer, &usSeqnum);
            pBody->m_usConnFlags |= 3;
            if (pConnection)
            {
                if (uns16_gt(usSeqnum, pConnection->m_usRxSeqnum + 4096))
                {
                    connection_log_parse_failure(pConnection, "seqnum (%u) > (uns16)( connection->guaranteed_rx + CONNECTION_MAX_OUTSTANDING ) (%u)\n",
                                                 usSeqnum, (uns16)(pConnection->m_usRxSeqnum + 4096));
                    pConnection->m_uiCstat[2]++;
                    goto out;
                }
                if (connection_has_reliable(pConnection, usSeqnum))
                {
                    buffer_skip(pBuffer, size);
                    pConnection->m_uiCstat[7]++;
                    continue;
                }
            }
            if (message_list_contains_type_seqnum(&pBody->m_messages, ucType, usSeqnum))
            {
                connection_log_parse_failure(pConnection, "this message appears twice in the same segment (%u)\n", usSeqnum);
                if (pConnection)
                    pConnection->m_uiCstat[2]++;
                goto out;
            }
            if (message_create(&pMessage, pConnection ? pConnection->endpoint : 0, pConnection,
                               pHeader->src, pHeader->dst, 0, size, ucType, usSeqnum,
                               pConnection ? endpoint_get_time(pConnection->endpoint) : 0))
            {
                connection_log_parse_failure(pConnection, "message_create failed\n");
                if (pConnection)
                    pConnection->m_uiCstat[1]++;
                goto out;
            }
            if (pConnection)
                pConnection->m_uiCstat[8]++;
            buffer_readm_uns8(pBuffer, (uns8 *)message_get_data(pMessage), size);
            vlist_add_tail(&pBody->m_messages.list, pMessage);
        }
        else if (ucType == CONNECTION_MESSAGE_TYPE_BEST_EFFORT)
        {
            usSeqnum = 0;
            if (pBody->m_iArrival)
            {
                if (message_create(&pMessage, pConnection ? pConnection->endpoint : 0, pConnection,
                                   pHeader->src, pHeader->dst, 0, size, ucType, 0xffff,
                                   pConnection ? endpoint_get_time(pConnection->endpoint) : 0))
                {
                    connection_log_parse_failure(pConnection, "message_create failed\n");
                    if (pConnection)
                        pConnection->m_uiCstat[1]++;
                    goto out;
                }
                buffer_readm_uns8(pBuffer, (uns8 *)message_get_data(pMessage), size);
                vlist_add_tail(&pBody->m_messages.list, pMessage);
            }
            else
                buffer_skip(pBuffer, size);
        }
        else if (ucType < CONNECTION_STATE_CHANNEL_COUNT)
        {
            state_channel_t *pChannel;
            pBody->m_usConnFlags |= 5;
            if (ucType >= CONNECTION_STATE_CHANNEL_COUNT)
            {
                connection_log_parse_failure(pConnection, "type (%u) >= STATE_CHANNEL_COUNT (%u)\n",
                                             ucType, CONNECTION_STATE_CHANNEL_COUNT);
                if (pConnection)
                    pConnection->m_uiCstat[2]++;
                goto out;
            }
            pChannel = 0;
            if (pConnection)
            {
                if (connection_allocate_state_channels(pConnection))
                {
                    connection_log_parse_failure(pConnection, "connection_allocate_state_channels failed\n");
                    pConnection->m_uiCstat[2]++;
                    goto out;
                }
                pChannel = &pConnection->m_pChannels[ucType];
            }
            buffer_read_uns16(pBuffer, &usSeqnum);
            buffer_read_uns8(pBuffer, &ucRefnumOffset);
            if (ucRefnumOffset > 32)
            {
                connection_log_parse_failure(pConnection, "refnum_offset (%u) > REFNUM_OFFSET_MAX (%u)\n",
                                             ucRefnumOffset, 32);
                if (pConnection)
                    pConnection->m_uiCstat[2]++;
                goto out;
            }
            if (!pConnection)
            {
                uns8 data[1300];
                VASSERT(size <= DRDP_SEND_SIZE_MAX_STATE);   // 3505
                run_length_decode(pBuffer, data, size);
                pBody->m_ucChannels[pBody->m_iChannelCount++] = ucType;
                continue;
            }
            if (uns16_le(usSeqnum, pChannel->rx))
            {
                uns8 data[1300];
                pConnection->m_uiCstat[9]++;



                VASSERT(size <= DRDP_SEND_SIZE_MAX_STATE);   // 3517
                run_length_decode(pBuffer, data, size);
                pBody->m_ucChannels[pBody->m_iChannelCount++] = ucType;
                continue;
            }
            if ((int16)(usSeqnum - pChannel->rx) > 4096)
            {
                pConnection->m_uiCstat[2]++;
                goto out;
            }
            if (message_create(&pMessage, pConnection->endpoint, pConnection, pHeader->src,
                               pHeader->dst, 0, size, ucType, usSeqnum,
                               endpoint_get_time(pConnection->endpoint)))
            {
                connection_log_parse_failure(pConnection, "message_create failed\n");
                pConnection->m_uiCstat[1]++;
                goto out;
            }
            message_set_refnum_offset(pMessage, ucRefnumOffset);
            vlist_add_tail(&pBody->m_messages.list, pMessage);
            run_length_decode(pBuffer, (uns8 *)message_get_data(pMessage), size);
            if (ucRefnumOffset)
            {
                message_t *pReference;
                uns16 usRefnum;
                usRefnum = usSeqnum - ucRefnumOffset;
                pReference = (message_t *)vlist_lookup(&pChannel->m_rxList.list, &usRefnum);
                if (!pReference)
                {
                    connection_log_parse_failure(pConnection, "refnum does not exist\n");
                    pConnection->m_uiCstat[2]++;
                    goto out;
                }
                xor_in_place((uns8 *)message_get_data(pMessage),
                             (const uns8 *)message_get_data(pReference),
                             DRDP_MIN(message_get_size(pMessage), message_get_size(pReference)));
            }
            if (message_duplicate(&pCopy, pMessage))
            {
                connection_log_parse_failure(pConnection, "message_duplicate failed\n");
                pConnection->m_uiCstat[1]++;
                goto out;
            }
            pConnection->m_uiCstat[10]++;
            vlist_add_tail(&pBody->m_messages.list, pCopy);
        }
        else
        {
            connection_log_parse_failure(pConnection, "invalid message type\n");
            goto out;
        }
    }
    if (pConnection && !pBody->m_iArrival)
        pConnection->m_uiCstat[4]++;
    iResult = 0;
out:
    return iResult;
}

// One segment for this connection: parse it whole into a body on the stack,
// and only then apply it, so a malformed segment changes nothing.  A client
// that hears from a software endpoint on a new route follows it there.
int connection_handle_segment(connection_t *pConnection, const segment_header_t *pHeader,
                              buffer_t *pBuffer)
{
    segment_body_t body;
    int iResult;
    int iOffset;
    memset(&body, 0, sizeof(body));
    iOffset = message_get_vlink_offset(0);
    vlist_init(&body.m_messages.list, iOffset, 0, 0);
    drdp_format_endpoint1(pConnection->local_id);
    drdp_format_endpoint2(pConnection->remote_id);
    endpoint_get_time(pConnection->endpoint);
    iResult = connection_parse_segment_body(pConnection, pHeader, &body, pBuffer);
    if (iResult == 0)
    {
        if (body.m_iArrival && drdp_is_client(endpoint_get_drdp(pConnection->endpoint))
            && drdp_is_software_endpoint(pHeader->src))
            drdp_re_route_connections_to_endpoint(endpoint_get_drdp(pConnection->endpoint),
                                                  pHeader->src, pHeader->m_uiRoute);
        connection_record_segment(pConnection, pHeader, &body);
        iResult = 0;
    }
    while (message_list_peek_head(&body.m_messages))
        message_destroy(message_list_remove_head(&body.m_messages));
    return iResult;
}

uns64 connection_get_src(const connection_t *pConnection)
{
    return pConnection->local_id;
}

uns64 connection_get_dst(const connection_t *pConnection)
{
    return pConnection->remote_id;
}

void *connection_get_app_ptr(const connection_t *pConnection)
{
    return pConnection->m_pAppPtr;
}

void connection_set_app_ptr(connection_t *pConnection, void *pAppPtr)
{
    pConnection->m_pAppPtr = pAppPtr;
}

char *connection_get_app_info(const connection_t *pConnection)
{
    return pConnection->app_info;
}

void connection_set_app_info(connection_t *pConnection, char *pAppInfo)
{
    if (pConnection->app_info)
    {
        free(pConnection->app_info);
        pConnection->app_info = 0;
    }
    if (pAppInfo)
        pConnection->app_info = strdup(pAppInfo);
}

int connection_has_instance_ack(connection_t *pConnection)
{
    return pConnection->m_usFlags & 0x40;
}

int connection_has_instance_remote(connection_t *pConnection)
{
    return pConnection->m_usFlags & 0x20;
}

uns32 connection_get_instance_remote(connection_t *pConnection)
{
    return pConnection->m_uiInstanceRemote;
}

// The local instance is pushed until it is acknowledged, and then again every
// two minutes so a far end that lost it gets it back.
int connection_send_instance_local(connection_t *pConnection)
{
    int iSend = 0;
    uns32 uiWhen = pConnection->m_uiTimeInstanceSent + 120000;

    if (connection_is_sw_sw(pConnection) || !(pConnection->m_usFlags & 0x40)
        || uns32_gt(uiWhen, endpoint_get_time(pConnection->endpoint)))
        iSend = 1;
    return iSend;
}

uns32 connection_get_instance_local(connection_t *pConnection)
{
    return pConnection->m_uiInstanceLocal;
}





































































































// A reset is a segment with no body: the far end is told which instance pair it
// was, so a stale one cannot be mistaken for a live one.
void connection_send_reset(connection_t *connection, void *pData, int iSize)
{
    int iReason = 1;

    VASSERT(connection_is_terminated( connection ));   // 3781
    if (connection->m_usFlags & 0x20)
        iReason = 3;
    route_send_reset_connection(connection->m_pRoute, connection->local_id,
                                connection->remote_id, iReason,
                                connection->m_uiInstanceLocal,
                                connection->m_uiInstanceRemote);
}












// The terminate notification built in `connection_create` is delivered HERE and
// everything still queued is freed.  It happens once: the flag latches.
void connection_terminate(connection_t *connection, int iSendReset, void *pData, int iSize)
{
    if (~connection->m_usFlags & 0x100)
    {
        drdp_connection_terminate(endpoint_get_drdp(connection->endpoint),
                                  connection->remote_id, pData, iSize);
        VASSERT(drdp_is_hardware_endpoint( connection->remote_id )
                || drdp_is_hardware_endpoint( connection->local_id ));   // 3810
        message_set_time(connection->m_pTerminate,
                         endpoint_get_time(connection->endpoint));
        memcpy(message_get_data(connection->m_pTerminate), pData,
               (uns32)iSize < message_get_size(connection->m_pTerminate)
                   ? (uns32)iSize : message_get_size(connection->m_pTerminate));
        endpoint_arrival_enqueue(connection->endpoint, connection->m_pTerminate);
        connection->m_pTerminate = 0;
        connection_free_outstanding(connection);
        connection->m_usFlags |= 0x100;
        if (iSendReset & 1)
            connection_send_reset(connection, pData, iSize);
    }
}

void connection_terminate_file_line(connection_t *pConnection, int iSendReset, char *pFile,
                                    int iLine)
{
    if (~pConnection->m_usFlags & 0x100)
    {
        char data[256];
        buffer_t buffer;

        buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
        buffer_append_uns8(&buffer, 16);
        buffer_append_string(&buffer, pFile);
        buffer_append_uns32(&buffer, iLine);
        connection_terminate(pConnection, iSendReset, buffer.data, buffer.position);
    }
}

void connection_terminate_reason(connection_t *pConnection, int iSendReset, uns8 ucReason)
{
    connection_terminate(pConnection, iSendReset, &ucReason, 1);
}

int connection_get_last_route_flush_number(connection_t *pConnection)
{
    return pConnection->m_iLastRouteFlushNumber;
}
endpoint_t *connection_get_endpoint(connection_t *pConnection)
{
    return pConnection->endpoint;
}
void connection_set_bps_limit(connection_t *pConnection, int iLimit)
{
    route_set_bps_limit(pConnection->m_pRoute, iLimit);
}
int connection_get_bps_limit(connection_t *pConnection)
{
    return route_get_bps_limit(pConnection->m_pRoute);
}
int connection_get_bps_sent(connection_t *pConnection)
{
    return route_get_bps_sent(pConnection->m_pRoute);
}
int connection_get_bps_recv(connection_t *pConnection)
{
    return route_get_bps_recv(pConnection->m_pRoute);
}
int connection_is_terminated(connection_t *pConnection)
{
    return (pConnection->m_usFlags >> 8) & 1;
}
// Closing is not destroying: it stays alive until everything it owes is
// acknowledged, which is what `connection_is_in_sync` decides.
void connection_close_internal(connection_t *connection, int iNow)
{
    VASSERT(connection_is_valid( connection ));   // 3878


    VASSERT(drdp_is_local_hardware_endpoint( connection->local_id )
            || drdp_is_remote_hardware_endpoint( connection->remote_id ));   // 3882
    VASSERT(!connection_is_closed( connection ));   // 3883
    endpoint_connection_free_arrivals(connection);
    connection->m_usFlags |= 0x800;

    // Everything the application had waiting is dropped: it asked to close, so
    // it is not going to read any of it.  What is still OWED to the far end is
    // not dropped -- the connection stays on the endpoint until the far end
    // has acknowledged all of it, and only `close_now` skips that wait.
    //
    // The reason byte is 10.  It reaches the far end as a one-byte terminate
    // payload, and the application on that side sees it as an arrival on the
    // pre-built notification message rather than as a socket error.
    if (iNow || connection_is_in_sync(connection))
    {
        connection_terminate_reason(connection, 1, 10);
        _endpoint_connection_destroy(connection, __FILE__, __LINE__);   // 3898
    }
}












void connection_close(connection_t *pConnection)
{
    connection_close_internal(pConnection, 0);
}

void connection_close_now(connection_t *pConnection)
{
    connection_close_internal(pConnection, 1);
}

int connection_open(connection_t **ppConnection, endpoint_t *pEndpoint, uns64 dst)
{
    VASSERT(drdp_is_endpoint( dst ));   // 3925
    return endpoint_connection_create(pEndpoint, ppConnection, dst,
                                      drdp_get_route_endpoint(endpoint_get_drdp(pEndpoint),
                                                              dst),
                                      endpoint_get_instance(pEndpoint), 1);
}











































































// Everything on the list is delivered as if it had arrived off the wire.  It
// is how a short-circuited pair of software endpoints talks: the sender's own
// queues are handed straight to the receiver.
void connection_receive_list(connection_t *connection, message_list_t *pList)
{
    message_t *pMessage;

    while ((pMessage = (message_t *)vlist_remove_head(&pList->list)) != 0)
    {
        message_set_connection(pMessage, connection);
        switch (message_get_type(pMessage))
        {
        case 254:
        case 255:
            VASSERT(!"this makes no sense");   // 4020
            break;
        case CONNECTION_MESSAGE_TYPE_BEST_EFFORT:
            endpoint_arrival_enqueue(connection->endpoint, pMessage);
            break;
        case CONNECTION_MESSAGE_TYPE_SYSTEM:
        case CONNECTION_MESSAGE_TYPE_BIG:
        case CONNECTION_MESSAGE_TYPE_GUARANTEED:
            if (!connection_has_reliable(connection, message_get_seqnum(pMessage)))
            {
                vlist_insert(&connection->m_delayed.list, pMessage);
                connection_move_delayed_to_arrivals(connection);
            }
            else
            {
                message_destroy(pMessage);
            }
            break;
        default:
            {
                state_channel_t *state_channel;
                uns16 seqnum;
                int result;










                result = connection_allocate_state_channels( connection );
                VASSERT(result == 0);   // 4054
                state_channel = &connection->m_pChannels[message_get_type(pMessage)];
                seqnum = message_get_seqnum(pMessage);
                if ((int16)(seqnum - state_channel->rx) <= 0)
                {
                    message_destroy(pMessage);
                }
                else
                {


                    VASSERT(uns16_sub( seqnum, state_channel->rx ) < CONNECTION_MAX_OUTSTANDING_STATE);   // 4065
                    connection_record_state_message(connection, pMessage, 0);
                }
            }
            break;
        }
    }

}

// The two halves of a software-to-software pair.  The far end's outstanding
// and queued messages become this end's arrivals with no wire in between.
void connection_short_circuit(connection_t *pConnection, connection_t *pWith)
{
    if ((pWith->m_usFlags & 0x20)
        && pWith->m_uiInstanceRemote != pConnection->m_uiInstanceLocal)
        connection_rx_reset(pWith);
    pWith->m_usFlags |= 0x20;
    pConnection->m_usFlags |= 0x40;
    pWith->m_uiInstanceRemote = pConnection->m_uiInstanceLocal;
    connection_receive_list(pWith, &pConnection->m_outstanding);
    connection_receive_list(pWith, &pConnection->m_sendQueue);
}

int connection_send_instance_push(const connection_t *pConnection)
{
    return pConnection->m_usFlags & 0x80;
}

// A reset is only believed when BOTH instance numbers in it match the ones
// this connection knows -- otherwise it is a reset for a connection that has
// already been replaced, and acting on it would kill the live one.
int connection_handle_reset(connection_t *pConnection, const segment_header_t *pHeader)
{
    if (pHeader->flags & 0x2000)
        if (connection_has_instance_remote(pConnection))
            if (connection_get_instance_remote(pConnection) != pHeader->m_uiCrc)
                return -1;
    if (pHeader->flags & 0x40000)
        if (connection_get_instance_local(pConnection) != pHeader->m_uiFlushNumber)
            return -1;
    if (!(pHeader->flags & 0x2000) && !(pHeader->flags & 0x40000))
        if (connection_send_instance_local(pConnection))
            return -1;
    drdp_format_endpoint1(pConnection->remote_id);
    drdp_format_endpoint2(pConnection->local_id);
    connection_terminate_reason(pConnection, 0, 6);
    return 0;
}

uns32 connection_get_time_last_instance_arrival(const connection_t *pConnection)
{
    return pConnection->m_uiTimeLastInstanceArrival;
}

void connection_get_arrival_history(connection_t *pConnection, uns64 *puiHistory)
{
    *puiHistory = pConnection->m_uiArrivalHistory;
}

void connection_get_arrival_number(connection_t *pConnection, uns16 *pusNumber)
{
    *pusNumber = pConnection->m_usArrivalNumber;
}

void connection_get_timeout(connection_t *pConnection, int *piTimeout)
{
    *piTimeout = pConnection->m_usTimeout;
}

void connection_get_ping_time(connection_t *pConnection, int *piPing)
{
    int i;

    *piPing = pConnection->m_usRtt[0];
    for (i = 1; i < 16; i++)
        *piPing = *piPing < pConnection->m_usRtt[i] ? *piPing : pConnection->m_usRtt[i];
}

void connection_set_simulated(connection_t *pConnection, int iDrop, int iDelay, int iJitter)
{
    route_set_simulated(pConnection->m_pRoute, iDrop, iDelay, iJitter);
}

// Only the uns64 instantiation of each template is ever asked for.
template void buffer_appendm_packed(buffer_t *, const uns64 *, int);
template void buffer_readm_packed(buffer_t *, uns64 *, int);

void connection_set_debug(connection_t *pConnection, int iDebug)
{
    if (iDebug)
        pConnection->m_usFlags |= 0x400;
    else
        pConnection->m_usFlags &= 0xfbff;
}

int connection_get_debug(connection_t *pConnection)
{
    return (pConnection->m_usFlags >> 10) & 1;
}

int connection_get_leader(connection_t *pConnection)
{
    return drdp_is_leader_endpoint(endpoint_get_drdp(pConnection->endpoint),
                                   pConnection->remote_id);
}

void connection_set_failure(connection_t *pConnection, int iRetry, int iWarn, int iFail)
{
    pConnection->m_iRetryTime = iRetry;
    pConnection->m_iWarnTime = iWarn;
    pConnection->m_iFailTime = iFail;
}

uns32 connection_get_time_last_arrival(const connection_t *pConnection)
{
    return pConnection->time_last_arrival;
}

// The trace hook.  A null connection always prints; a real one prints only
// when it has been marked for debugging.
void connection_pf(connection_t *pConnection, const char *pFormat, ...)
{
    va_list args;

    if (!pConnection || (pConnection->m_usFlags & 0x400))
    {
        va_start(args, pFormat);
        vpf(pFormat, args);
    }
}

// In sync means nothing RELIABLE is still owed -- best effort and state do not
// count, because neither is ever retried.
int message_list_is_in_sync(message_list_t *pList)
{
    message_t *pMessage;

    for (pMessage = (message_t *)vlist_peek_head(&pList->list); pMessage;
         pMessage = (message_t *)vlist_get_next(&pList->list, pMessage))
    {
        if (message_get_type(pMessage) == CONNECTION_MESSAGE_TYPE_GUARANTEED)
            return 0;
        if (message_get_type(pMessage) == CONNECTION_MESSAGE_TYPE_BIG)
            return 0;
        if (message_get_type(pMessage) == CONNECTION_MESSAGE_TYPE_SYSTEM)
            return 0;
    }
    return 1;
}

// The answer LATCHES: once a connection has been in sync it stays in sync, so
// a message sent after the close does not keep it alive for ever.
int connection_is_in_sync(connection_t *pConnection)
{
    int iInSync = 0;

    if (connection_is_terminated(pConnection) || (pConnection->m_usFlags & 0x1000))
        iInSync = 1;
    if (!iInSync)
        if (message_list_is_in_sync(&pConnection->m_sendQueue))
            if (message_list_is_in_sync(&pConnection->m_outstanding))
            {
                pConnection->m_usFlags |= 0x1000;
                iInSync = 1;
            }
    return iInSync;
}

void connection_set_remote_id(connection_t *pConnection, uns64 uiRemoteId)
{
    pConnection->remote_id = uiRemoteId;
    drdp_format_endpoint(pConnection->m_szName, uiRemoteId);
}

// The soonest of the two, so a debug dump never reports a flush in the past.
uns32 connection_get_debug_time(connection_t *pConnection)
{
    uns32 uiTime = pConnection->time_last_flush;

    if ((int)(uiTime - pConnection->m_uiDesiredFlushTime) <= 0)
        uiTime = pConnection->m_uiDesiredFlushTime;
    return uiTime;
}

void connection_set_logger(connection_t *pConnection, drdp::logger_t *pLogger)
{
    pConnection->m_pLogger = pLogger;
}

void connection_handle_bad_crc(connection_t *pConnection)
{
    pConnection->m_uiCstat[5]++;
    pConnection->m_uiCstat[3]++;
}

void connection_handle_corrupt(connection_t *pConnection)
{
    pConnection->m_uiCstat[5]++;
    pConnection->m_uiCstat[2]++;
}

int connection_get_cstat_max()
{
    return 11;
}

uns64 connection_get_cstat(const connection_t *pConnection, int iIndex)
{
    return pConnection->m_uiCstat[iIndex];
}

// The eleven counters, in the order `connection_get_cstat` indexes them.  The
// two after the null are the message type names, in another table the linker
// happened to put next to this one.
static const char *cstat_text[] =
{
    "discarded (old)",
    "discarded (no memory was available to process message)",
    "discarded (corrupt)",
    "discarded (crc)",
    "out of sequence",
    "recv",
    "sent",
    "guaranteed recv (duplicates)",
    "guaranteed recv",
    "state recv (duplicates)",
    "state recv"
};

const char *connection_get_cstat_label(int iIndex)
{
    return cstat_text[iIndex];
}

// The five system-message helpers.  Each builds its payload in a 64-byte stack
// buffer and sends it as one MESSAGE_TYPE_SYSTEM.
void connection_send_uns8(connection_t *pConnection, uns8 ucType)
{
    uns8 data = ucType;

    connection_send(pConnection, &data, 1, CONNECTION_MESSAGE_TYPE_SYSTEM);
}

void connection_send_uns8_uns64(connection_t *pConnection, uns8 ucType, uns64 uiA)
{
    buffer_t buffer;
    char data[64];

    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    buffer_append_uns8(&buffer, ucType);
    buffer_append_uns64(&buffer, uiA);
    connection_send(pConnection, buffer.data, buffer.position, CONNECTION_MESSAGE_TYPE_SYSTEM);
}

void connection_send_uns8_uns64_uns64(connection_t *pConnection, uns8 ucType, uns64 uiA,
                                      uns64 uiB)
{
    buffer_t buffer;
    char data[64];

    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    buffer_append_uns8(&buffer, ucType);
    buffer_append_uns64(&buffer, uiA);
    buffer_append_uns64(&buffer, uiB);
    connection_send(pConnection, buffer.data, buffer.position, CONNECTION_MESSAGE_TYPE_SYSTEM);
}

void connection_send_uns8_uns64_uns64_uns64(connection_t *pConnection, uns8 ucType, uns64 uiA,
                                            uns64 uiB, uns64 uiC)
{
    buffer_t buffer;
    char data[64];

    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    buffer_append_uns8(&buffer, ucType);
    buffer_append_uns64(&buffer, uiA);
    buffer_append_uns64(&buffer, uiB);
    buffer_append_uns64(&buffer, uiC);
    connection_send(pConnection, buffer.data, buffer.position, CONNECTION_MESSAGE_TYPE_SYSTEM);
}

void connection_send_uns8_uns64_uns64_uns64_uns64(connection_t *pConnection, uns8 ucType,
                                                  uns64 uiA, uns64 uiB, uns64 uiC, uns64 uiD)
{
    buffer_t buffer;
    char data[64];

    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    buffer_append_uns8(&buffer, ucType);
    buffer_append_uns64(&buffer, uiA);
    buffer_append_uns64(&buffer, uiB);
    buffer_append_uns64(&buffer, uiC);
    buffer_append_uns64(&buffer, uiD);
    connection_send(pConnection, buffer.data, buffer.position, CONNECTION_MESSAGE_TYPE_SYSTEM);
}
