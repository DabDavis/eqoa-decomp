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

// Phase 10's va_start: the EABI's register save block sits BELOW
// virtual_incoming_args, so bare __builtin_next_arg lands past the whole area.
typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)

extern "C" void vpf(const char *pFormat, char *ap);

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

// A state channel starts with its send sequence number at 1 and its pending
// message nowhere.  Both lists sort by the message's own sequence number, so
// `uns16_cmp_wrap` orders them across the wrap.
void state_channel_init(state_channel_t *pChannel)
{
    pChannel->m_pPending = 0;
    pChannel->m_usRx = 0;
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
    pConnection->m_pEndpoint = 0;
    pConnection->m_pRoute = 0;
    pConnection->m_pAppPtr = 0;
    pConnection->m_pAppInfo = 0;
    pConnection->m_uiLocalId = 0;
    pConnection->m_uiRemoteId = 0;
    vlist_init(&pConnection->m_sendQueue.list, message_get_vlink_offset(0), 0, 0);
    vlist_init(&pConnection->m_outstanding.list, message_get_vlink_offset(0), 0, 0);
    vlist_init(&pConnection->m_arrivals.list, message_get_vlink_offset(0),
               (vcmp_t)uns16_cmp_wrap, message_get_key_offset(0));
    vlist_init(&pConnection->m_delayed.list, message_get_vlink_offset(0),
               (vcmp_t)uns16_cmp_wrap, message_get_key_offset(0));
    pConnection->m_pTerminate = 0;
    pConnection->m_uiTimeInstanceSent = 0;
    pConnection->m_uiTimeTx = 0;
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
    pConnection->m_iWarnTime = 30000;
    pConnection->m_iFailTime = 60000;
    pConnection->m_iRetryTime = 5000;
    memset(pConnection->m_uiCstat, 0, sizeof(pConnection->m_uiCstat));
    pConnection->m_pLogger = 0;
    pConnection->m_iLastRouteFlushNumber = -1;
    pConnection->m_pChannels = 0;
}

// The tag has to match AND the endpoint has to still own it, so a connection
// destroyed and freed under a pointer somebody kept fails here.
int connection_is_valid(const connection_t *pConnection)
{
    int iResult = memcmp(pConnection->m_szTag, CONNECTION_TAG, 4) == 0;

    if (iResult)
        iResult = endpoint_has_connection(pConnection->m_pEndpoint, pConnection) != 0;
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







































































































































































































int connection_get_key_offset(int iIndex)
{
    int iOffset = -1;

    VASSERT(0 <= iIndex && iIndex < CONNECTION_KEY_COUNT);
    switch (iIndex)
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
        iResult = -2;
    else
    {
        connection_init(pConnection);
        pConnection->m_pEndpoint = pEndpoint;
        pConnection->m_uiLocalId = endpoint_get_id(pEndpoint);
        pConnection->m_uiRemoteId = uiDst;
        pConnection->m_uiDesiredFlushTime = endpoint_get_time(pEndpoint);
        pConnection->m_uiTimeLastArrival = endpoint_get_time(pEndpoint);
        pConnection->m_uiTimeLastInstanceArrival = endpoint_get_time(pEndpoint);
        pConnection->m_uiInstanceLocal = uiInstance;
        pConnection->m_uiTimeSend = endpoint_get_time(pEndpoint);
        drdp_format_endpoint(pConnection->m_szName, pConnection->m_uiRemoteId);
        if (iFlags & 1)
        {
            pConnection->m_uiInstanceRemote = pConnection->m_uiInstanceLocal;
            pConnection->m_usFlags |= 0xa0;
        }
        if (iFlags & 2)
            pConnection->m_usFlags |= 0x40;
        if (connection_is_sw_sw(pConnection))
            iResult = 0;
        else
            iResult = message_create(&pConnection->m_pTerminate, pEndpoint, pConnection,
                                     pConnection->m_uiRemoteId, pConnection->m_uiLocalId, 0, 256,
                                     CONNECTION_MESSAGE_TYPE_TERMINATE, 0xffff,
                                     endpoint_get_time(pEndpoint));
        if (iResult == 0)
        {
            *ppConnection = pConnection;
            iResult = 0;
            pConnection = 0;
        }
    }
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

void connection_destroy(connection_t *pConnection)
{
    connection_destroy_buffer(pConnection, 0);
}

// One best-effort message, straight onto the send queue.  Its sequence number
// is 0xffff -- best effort is never acknowledged, so it never needs one.
int connection_send_best_effort(connection_t *pConnection, const uns8 *pData, int iSize)
{
    message_t *pMessage = 0;
    int iResult = message_create(&pMessage, pConnection->m_pEndpoint, pConnection,
                                 pConnection->m_uiLocalId, pConnection->m_uiRemoteId, pData,
                                 iSize, CONNECTION_MESSAGE_TYPE_BEST_EFFORT, 0xffff,
                                 endpoint_get_time(pConnection->m_pEndpoint));

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
int connection_send_guaranteed(connection_t *pConnection, const uns8 *pData, int iSize,
                               int iType)
{
    message_list_t messages;
    message_t *pMessage = 0;
    int iResult;
    int iOffset = 0;
    int iChunk;
    int ucType;

    vlist_init(&messages.list, message_get_vlink_offset(0), 0, 0);
    pConnection->m_uiTimeSend = endpoint_get_time(pConnection->m_pEndpoint);
    do
    {
        iChunk = iSize - iOffset;
        if (iChunk > 1300)
            iChunk = 1300;
        ucType = (uns8)iType;
        if (iOffset + iChunk < iSize)
            ucType = CONNECTION_MESSAGE_TYPE_BIG;
        if ((int16)(pConnection->m_usTxSeqnum - (uns16)(pConnection->m_usTxAck + 4096)) > 0)
        {
            VASSERT(!connection_is_sw_sw(pConnection));
            iResult = -1;
            goto out;
        }
        iResult = message_create(&pMessage, pConnection->m_pEndpoint, pConnection,
                                 pConnection->m_uiLocalId, pConnection->m_uiRemoteId,
                                 pData + iOffset, iChunk, ucType, pConnection->m_usTxSeqnum++,
                                 endpoint_get_time(pConnection->m_pEndpoint));
        if (iResult != 0)
            goto out;
        vlist_add_tail(&messages.list, pMessage);
        iOffset += 1300;
    }
    while (iOffset < iSize);
    vlist_get_size(&messages.list);
    while (vlist_peek_head(&messages.list))
    {
        message_get_seqnum((message_t *)vlist_peek_head(&messages.list));
        message_get_size((message_t *)vlist_peek_head(&messages.list));
        vlist_add_tail(&pConnection->m_sendQueue.list, vlist_remove_head(&messages.list));
    }
    iResult = 0;
out:
    while ((pMessage = (message_t *)vlist_remove_head(&messages.list)) != 0)
        message_destroy(pMessage);
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

    if (iChannel >= CONNECTION_STATE_CHANNEL_COUNT)
        return -1;
    if (connection_allocate_state_channels(pConnection))
        return -2;
    pChannel = &pConnection->m_pChannels[iChannel];
    pMessage = pChannel->m_pPending;
    if (!pMessage)
        pMessage = (message_t *)vlist_peek_tail(&pChannel->m_txList.list);
    iSame = 0;
    if (pMessage)
        iSame = message_get_size(pMessage) == (uns32)iSize
                && memcmp(pData, message_get_data(pMessage), iSize) == 0;
    if (iSame)
        return 0;
    if (message_create(&pMessage, pConnection->m_pEndpoint, pConnection,
                       pConnection->m_uiLocalId, pConnection->m_uiRemoteId, pData, iSize,
                       (uns8)iChannel, 0xffff,
                       endpoint_get_time(pConnection->m_pEndpoint)) != 0)
        return -2;
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
    pChannel->m_ucWhere = 1;
    pChannel->m_pPending = pMessage;
    return 0;
}

int connection_is_closed(connection_t *pConnection)
{
    return (pConnection->m_usFlags >> 11) & 1;
}

// The one place the four transports are told apart.  A terminated connection
// silently drops everything, which is how the application can keep writing to
// one that has already gone away.
int connection_send_internal(connection_t *pConnection, const uns8 *pData, int iSize, int iType)
{
    int iResult;

    VASSERT(!connection_is_closed(pConnection));
    VASSERT(!endpoint_is_closed(pConnection->m_pEndpoint));
    if (connection_is_terminated(pConnection))
        iResult = 0;
    else
    {
        switch (iType)
        {
        case CONNECTION_MESSAGE_TYPE_BEST_EFFORT:
            iResult = connection_send_best_effort(pConnection, pData, iSize);
            break;
        case CONNECTION_MESSAGE_TYPE_GUARANTEED:
        case CONNECTION_MESSAGE_TYPE_SYSTEM:
            iResult = connection_send_guaranteed(pConnection, pData, iSize, iType);
            break;
        default:
            iResult = connection_send_state(pConnection, pData, iSize, iType);
            break;
        }
        if (iResult != 0)
            connection_terminate_reason(pConnection, 1, 7);
        else
        {
            route_resort(pConnection);
            iResult = 0;
        }
    }
    return iResult;
}



























int connection_send(connection_t *pConnection, const void *pData, int iSize, int iType)
{
    VASSERT(iSize <= connection_get_max_message_size(iType));
    return connection_send_internal(pConnection, (const uns8 *)pData, iSize, iType);
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
        int iBit = (int16)(message_get_seqnum(pMessage) - pConnection->m_usRxSeqnum) - 2;

        if (iBit >= 64)
            break;
        uiMask |= (uns64)1 << iBit;
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

    if (drdp_is_software_endpoint(pConnection->m_uiLocalId))
        iResult = drdp_is_software_endpoint(pConnection->m_uiRemoteId) != 0;
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
    drdp_format_endpoint1(pConnection->m_uiLocalId);
    drdp_format_endpoint2(pConnection->m_uiRemoteId);
    if (pConnection->m_pChannels)
    {
        for (i = 0; i < CONNECTION_STATE_CHANNEL_COUNT; i++)
        {
            state_channel_t *pChannel = &pConnection->m_pChannels[i];

            pChannel->m_usRx = 0;
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

    drdp_format_endpoint1(pConnection->m_uiLocalId);
    drdp_format_endpoint2(pConnection->m_uiRemoteId);
    pConnection->m_uiInstanceLocal = endpoint_get_instance(pConnection->m_pEndpoint);
    pConnection->m_usFlags &= 0xfdbf;
    pConnection->m_usTxSeqnum = 1;
    pConnection->m_uiTimeInstanceSent = 0;
    pConnection->m_uiTimeTx = 0;
    pConnection->m_usFlush = 1;
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
    int iType = message_get_type(pMessage);
    uns32 uiSize = message_get_size(pMessage);
    uns16 usSeqnum = message_get_seqnum(pMessage);
    int iReliable = (uns8)(iType + 7) < 3;
    int iCost = (uns8)(iType + 7) > 2 ? uiSize + 4 : uiSize + 6;

    if (pBuffer->size < pBuffer->position + iCost)
        return -1;
    pMessage = (message_t *)vlist_remove_head(&pList->list);
    buffer_append_uns8(pBuffer, (uns8)iType);
    buffer_append_size(pBuffer, (uns16)uiSize);
    if (iReliable)
        buffer_append_uns16(pBuffer, usSeqnum);
    buffer_appendm_uns8(pBuffer, (const uns8 *)message_get_data(pMessage), uiSize);
    if (iReliable)
    {
        message_set_time(pMessage, endpoint_get_time(pConnection->m_pEndpoint));
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

int connection_append(connection_t *pConnection, buffer_t *pBuffer, message_list_t *pList,
                      uns16 usFlush)
{
    message_t *pMessage = (message_t *)vlist_peek_head(&pList->list);
    int iType = message_get_type(pMessage);

    if (iType < CONNECTION_MESSAGE_TYPE_TERMINATE && iType >= CONNECTION_MESSAGE_TYPE_SYSTEM)
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
    message_t *pMessage;

    *ppList = 0;
    pMessage = (message_t *)vlist_peek_head(&pConnection->m_sendQueue.list);
    if (pMessage)
    {
        if (!iHave || (int)(message_get_time(pMessage) - *puiTime) < 0)
        {
            iHave = 1;
            *puiTime = message_get_time(pMessage);
            *ppList = &pConnection->m_sendQueue;
        }
    }
    pMessage = (message_t *)vlist_peek_head(&pConnection->m_outstanding.list);
    if (pMessage)
    {
        uns32 uiTime = connection_message_get_desired_flush_time(pConnection, pMessage);

        if (!iHave || (int)(uiTime - *puiTime) < 0)
        {
            *puiTime = uiTime;
            *ppList = &pConnection->m_outstanding;
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
        if ((pConnection->m_usFlags & 0x240) == 0x240
            && !vlist_get_size(&pConnection->m_sendQueue.list)
            && !vlist_get_size(&pConnection->m_outstanding.list)
            && !(pConnection->m_usFlags & 0xf)
            && (int)(endpoint_get_time(pConnection->m_pEndpoint)
                     - (pConnection->m_uiTimeTx + 120000)) >= 0)
        {
            connection_tx_reset(pConnection);
            iResult = 1;
        }
        if (!(pConnection->m_usFlags & 0x200) && (pConnection->m_usFlags & 0x20)
            && !vlist_get_size(&pConnection->m_delayed.list)
            && !vlist_get_size(&pConnection->m_arrivals.list)
            && (int)(endpoint_get_time(pConnection->m_pEndpoint)
                     - (pConnection->m_uiTimeLastArrival + 360000)) >= 0)
        {
            iResult |= 2;
            drdp_format_endpoint1(pConnection->m_uiLocalId);
            drdp_format_endpoint2(pConnection->m_uiRemoteId);
            _endpoint_connection_destroy(pConnection, __FILE__, __LINE__);   // 1923
        }
    }
    return iResult;
}












































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































uns32 connection_get_desired_flush_time(connection_t *pConnection)
{
    return pConnection->m_uiDesiredFlushTime;
}

// The round trip history is sixteen deep and the TIMEOUT is the largest of
// them, where `get_ping_time` below reports the smallest.
void connection_add_rtt_sample(connection_t *pConnection, uns16 usSample)
{
    uns8 ucIndex;
    int i;

    pConnection->m_usRtt[pConnection->m_ucRttIndex] = usSample;
    pConnection->m_usTimeout = pConnection->m_usRtt[0];
    for (i = 1; i < 16; i++)
    {
        uns16 usRtt = pConnection->m_usRtt[i];

        if (usRtt < pConnection->m_usTimeout)
            usRtt = pConnection->m_usTimeout;
        pConnection->m_usTimeout = usRtt;
    }
    ucIndex = (uns8)(pConnection->m_ucRttIndex + 1);
    pConnection->m_ucRttIndex = (uns8)(ucIndex & 15);
}

// A software-to-software connection never really talks to anything, so its
// flush time is pushed a long way out; a real one wakes up thirty seconds
// after its last send, or a whole day once it is closed.
void connection_recalc_desired_flush_time(connection_t *pConnection)
{
    message_list_t *pList;

    if (connection_is_sw_sw(pConnection))
    {
        if (pConnection->m_usFlags & 0x200)
            pConnection->m_uiDesiredFlushTime = pConnection->m_uiTimeTx + 120000;
        else if (pConnection->m_usFlags & 0x20)
            pConnection->m_uiDesiredFlushTime = pConnection->m_uiTimeLastArrival + 360000;
        else
            pConnection->m_uiDesiredFlushTime = pConnection->m_uiTimeLastArrival + 86400000;
    }
    else if (connection_is_closed(pConnection) || connection_is_terminated(pConnection))
    {
        VASSERT(endpoint_get_time(pConnection->m_pEndpoint)
                - pConnection->m_uiTimeLastArrival < 43200000);
        pConnection->m_uiDesiredFlushTime =
            endpoint_get_time(pConnection->m_pEndpoint) + 86400000;
    }
    else
    {
        pConnection->m_uiDesiredFlushTime = pConnection->m_uiTimeSend + 30000;
    }
    connection_get_next_message(pConnection, &pList, &pConnection->m_uiDesiredFlushTime, 1);
    if (pConnection->m_usFlags & 0xf)
        if ((int)(pConnection->m_uiDesiredFlushTime - pConnection->m_uiFlushBy) >= 0)
            pConnection->m_uiDesiredFlushTime = pConnection->m_uiFlushBy;
    if (connection_is_closed(pConnection) && connection_is_in_sync(pConnection))
    {
        drdp_format_endpoint1(connection_get_src(pConnection));
        drdp_format_endpoint2(connection_get_dst(pConnection));
        pConnection->m_uiDesiredFlushTime = pConnection->m_uiTimeLastArrival;
    }
    if (endpoint_is_closed(pConnection->m_pEndpoint)
        && endpoint_is_in_sync(pConnection->m_pEndpoint))
    {
        drdp_format_endpoint1(connection_get_src(pConnection));
        drdp_format_endpoint2(connection_get_dst(pConnection));
        pConnection->m_uiDesiredFlushTime = pConnection->m_uiTimeLastArrival;
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
        int iAhead;

        pNext = (message_t *)vlist_get_next(&pConnection->m_outstanding.list, pMessage);
        if (message_get_type(pMessage) != CONNECTION_MESSAGE_TYPE_GUARANTEED
            && message_get_type(pMessage) != CONNECTION_MESSAGE_TYPE_BIG
            && message_get_type(pMessage) != CONNECTION_MESSAGE_TYPE_SYSTEM)
            continue;
        iAhead = (int16)(message_get_seqnum(pMessage) - usSeqnum);
        if (iAhead <= 0)
        {
            vlist_remove(&pConnection->m_outstanding.list, pMessage);
            message_destroy(pMessage);
        }
        else if (iAhead != 1 && (uns32)(iAhead - 2) < 64
                 && (((uns64)1 << (iAhead - 2)) & uiMask))
        {
            vlist_remove(&pConnection->m_outstanding.list, pMessage);
            message_destroy(pMessage);
        }
    }
    pConnection->m_uiTxMask = uiMask;
    pConnection->m_usTxAck = usSeqnum;
}

// One acknowledgement per state channel, the array ending at channel 248.
void connection_record_state_acks(connection_t *pConnection, const state_ack_t *pAcks)
{
    while (pAcks->m_ucChannel != CONNECTION_STATE_CHANNEL_COUNT)
    {
        state_channel_t *pChannel = &pConnection->m_pChannels[pAcks->m_ucChannel];
        uns16 usSeqnum = pAcks->m_usSeqnum;

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
            message_seqnum_remove_thru(&pChannel->m_txList, usSeqnum);
        }
        pAcks++;
    }
}

// The far end says which flush number it saw; the time that one went out is
// still in the ring, so the difference IS the round trip.
void connection_record_flush_ack(connection_t *pConnection, uns16 usFlush)
{
    uns32 *puiTime;

    if ((int16)(usFlush - (uns16)(pConnection->m_usFlush - 1024)) < 0)
        return;
    puiTime = &pConnection->m_uiFlushTime[usFlush & 1023];
    if (*puiTime)
    {
        uns32 uiRtt = endpoint_get_time(pConnection->m_pEndpoint) - *puiTime;

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

    while ((pMessage = (message_t *)vlist_peek_head(&pConnection->m_delayed.list)) != 0
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
            endpoint_arrival_enqueue(pConnection->m_pEndpoint, pMessage);
        }
        else
        {
            message_t *pPiece;
            message_t *pWhole;
            uns32 uiTotal = 0;
            uns32 uiOffset;

            vlist_add_tail(&pConnection->m_arrivals.list, pMessage);
            for (pPiece = (message_t *)vlist_peek_head(&pConnection->m_arrivals.list); pPiece;
                 pPiece = (message_t *)vlist_get_next(&pConnection->m_arrivals.list, pPiece))
                uiTotal += message_get_size(pPiece);
            VASSERT(uiTotal <= 0x514000);
            if (message_create(&pWhole, pConnection->m_pEndpoint, pConnection,
                               pConnection->m_uiRemoteId, pConnection->m_uiLocalId, 0, uiTotal,
                               message_get_type(pMessage),
                               message_get_seqnum((message_t *)vlist_peek_head(
                                   &pConnection->m_arrivals.list)),
                               endpoint_get_time(pConnection->m_pEndpoint)) != 0)
                VASSERT(0);
            uiOffset = 0;
            while ((pPiece = (message_t *)vlist_remove_head(&pConnection->m_arrivals.list)) != 0)
            {
                memcpy((uns8 *)message_get_data(pWhole) + uiOffset, message_get_data(pPiece),
                       message_get_size(pPiece));
                uiOffset += message_get_size(pPiece);
                message_destroy(pPiece);
            }
            endpoint_arrival_enqueue(pConnection->m_pEndpoint, pWhole);
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

    pChannel->m_usRx = usSeqnum;
    pChannel->m_ucRxFlag = 2;
    if (message_get_refnum_offset(pMessage))
        message_seqnum_remove_thru(&pChannel->m_rxList,
                                   (uns16)(usSeqnum - message_get_refnum_offset(pMessage)));
    else
        message_seqnum_remove_thru(&pChannel->m_rxList, (uns16)(usSeqnum - 32));
    if (pWith)
        vlist_add_tail(&pChannel->m_rxList.list, pWith);
    endpoint_arrival_enqueue(pConnection->m_pEndpoint, pMessage);
}

void connection_record_message(connection_t *pConnection, message_t *pMessage,
                               message_list_t *pList)
{
    switch (message_get_type(pMessage))
    {
    case CONNECTION_MESSAGE_TYPE_BEST_EFFORT:
        endpoint_arrival_enqueue(pConnection->m_pEndpoint, pMessage);
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
            pConnection->m_uiFlushBy = endpoint_get_time(pConnection->m_pEndpoint) + 100;
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
    pConnection->m_uiTimeLastArrival = endpoint_get_time(pConnection->m_pEndpoint);
    if (pHeader->m_uiFlags & 0x2000)
    {
        pConnection->m_uiTimeLastInstanceArrival = endpoint_get_time(pConnection->m_pEndpoint);
        if (!(pConnection->m_usFlags & 0x20))
        {
            pConnection->m_usFlags |= 0x20;
            pConnection->m_uiInstanceRemote = pHeader->m_uiCrc;
        }
        if (pHeader->m_uiFlags & 0x80000)
        {
            if (!(pConnection->m_usFlags & 0xf))
                pConnection->m_uiFlushBy = endpoint_get_time(pConnection->m_pEndpoint) + 100;
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
        pConnection->m_uiArrivalHistory = (pConnection->m_uiArrivalHistory << 1) | 1;
        pConnection->m_usArrivalNumber = pConnection->m_usArrivalNumber + 1;
        if ((int16)(pBody->m_usArrivalNumber - pConnection->m_usArrivalNumber) > 0)
            pConnection->m_uiArrivalHistory <<=
                (int16)(pBody->m_usArrivalNumber - pConnection->m_usArrivalNumber);
        pConnection->m_usArrivalNumber = pBody->m_usArrivalNumber;
    }
    else
    {
        pConnection->m_ucRxFlag = 0;
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

    if ((int16)(usSeqnum - pConnection->m_usRxSeqnum) <= 0)
        iHave = 1;
    else if (vlist_lookup(&pConnection->m_arrivals.list, &usSeqnum))
        iHave = 1;
    else if (vlist_lookup(&pConnection->m_delayed.list, &usSeqnum))
        iHave = 1;
    return iHave;
}

uns64 connection_get_src(const connection_t *pConnection)
{
    return pConnection->m_uiLocalId;
}

uns64 connection_get_dst(const connection_t *pConnection)
{
    return pConnection->m_uiRemoteId;
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
    return pConnection->m_pAppInfo;
}

void connection_set_app_info(connection_t *pConnection, char *pAppInfo)
{
    if (pConnection->m_pAppInfo)
    {
        free(pConnection->m_pAppInfo);
        pConnection->m_pAppInfo = 0;
    }
    if (pAppInfo)
        pConnection->m_pAppInfo = strdup(pAppInfo);
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

    if (connection_is_sw_sw(pConnection))
        iSend = 1;
    else if (!(pConnection->m_usFlags & 0x40))
        iSend = 1;
    else if ((int)(uiWhen - endpoint_get_time(pConnection->m_pEndpoint)) > 0)
        iSend = 1;
    return iSend;
}

uns32 connection_get_instance_local(connection_t *pConnection)
{
    return pConnection->m_uiInstanceLocal;
}

// A reset is a segment with no body: the far end is told which instance pair it
// was, so a stale one cannot be mistaken for a live one.
void connection_send_reset(connection_t *pConnection, void *pData, int iSize)
{
    int iReason = 1;

    VASSERT(connection_is_terminated(pConnection));
    if (pConnection->m_usFlags & 0x20)
        iReason = 3;
    route_send_reset_connection(pConnection->m_pRoute, pConnection->m_uiLocalId,
                                pConnection->m_uiRemoteId, iReason,
                                pConnection->m_uiInstanceLocal,
                                pConnection->m_uiInstanceRemote);
}












// The terminate notification built in `connection_create` is delivered HERE and
// everything still queued is freed.  It happens once: the flag latches.
void connection_terminate(connection_t *pConnection, int iSendReset, void *pData, int iSize)
{
    if (!connection_is_terminated(pConnection))
    {
        drdp_connection_terminate(endpoint_get_drdp(pConnection->m_pEndpoint),
                                  pConnection->m_uiRemoteId, pData, iSize);
        VASSERT(drdp_is_hardware_endpoint(pConnection->m_uiRemoteId)
                || drdp_is_hardware_endpoint(pConnection->m_uiLocalId));   // 3810
        message_set_time(pConnection->m_pTerminate,
                         endpoint_get_time(pConnection->m_pEndpoint));
        memcpy(message_get_data(pConnection->m_pTerminate), pData,
               (uns32)iSize < message_get_size(pConnection->m_pTerminate)
                   ? (uns32)iSize : message_get_size(pConnection->m_pTerminate));
        endpoint_arrival_enqueue(pConnection->m_pEndpoint, pConnection->m_pTerminate);
        pConnection->m_pTerminate = 0;
        connection_free_outstanding(pConnection);
        pConnection->m_usFlags |= 0x100;
        if (iSendReset & 1)
            connection_send_reset(pConnection, pData, iSize);
    }
}

void connection_terminate_file_line(connection_t *pConnection, int iSendReset, char *pFile,
                                    int iLine)
{
    if (!connection_is_terminated(pConnection))
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
    return pConnection->m_pEndpoint;
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
void connection_close_internal(connection_t *pConnection, int iNow)
{
    VASSERT(connection_is_valid(pConnection));
    VASSERT(drdp_is_local_hardware_endpoint(pConnection->m_uiLocalId)
            || drdp_is_remote_hardware_endpoint(pConnection->m_uiRemoteId));   // 3882
    VASSERT(!connection_is_closed(pConnection));   // 3883
    endpoint_connection_free_arrivals(pConnection);
    pConnection->m_usFlags |= 0x800;

    // Everything the application had waiting is dropped: it asked to close, so
    // it is not going to read any of it.  What is still OWED to the far end is
    // not dropped -- the connection stays on the endpoint until the far end
    // has acknowledged all of it, and only `close_now` skips that wait.  A
    // connection closed twice would assert above rather than double-destroy,
    // and the flag is what makes that assert fire rather than the free list.
    //
    // The reason byte is 10.  It reaches the far end as a one-byte terminate
    // payload, and the application on that side sees it as an arrival on the
    // pre-built notification message rather than as a socket error.
    if (iNow || connection_is_in_sync(pConnection))
    {
        connection_terminate_reason(pConnection, 1, 10);
        _endpoint_connection_destroy(pConnection, __FILE__, __LINE__);   // 3898
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

int connection_open(connection_t **ppConnection, endpoint_t *pEndpoint, uns64 uiDst)
{
    VASSERT(drdp_is_endpoint(uiDst));
    return endpoint_connection_create(pEndpoint, ppConnection, uiDst,
                                      drdp_get_route_endpoint(endpoint_get_drdp(pEndpoint),
                                                              uiDst),
                                      endpoint_get_instance(pEndpoint), 1);
}



























































// Everything on the list is delivered as if it had arrived off the wire.  It
// is how a short-circuited pair of software endpoints talks: the sender's own
// queues are handed straight to the receiver.
int connection_receive_list(connection_t *pConnection, message_list_t *pList)
{
    message_t *pMessage;

    while ((pMessage = (message_t *)vlist_remove_head(&pList->list)) != 0)
    {
        message_set_connection(pMessage, pConnection);
        switch (message_get_type(pMessage))
        {
        case CONNECTION_MESSAGE_TYPE_BEST_EFFORT:
            endpoint_arrival_enqueue(pConnection->m_pEndpoint, pMessage);
            break;
        case CONNECTION_MESSAGE_TYPE_SYSTEM:
        case CONNECTION_MESSAGE_TYPE_BIG:
        case CONNECTION_MESSAGE_TYPE_GUARANTEED:
            if (!connection_has_reliable(pConnection, message_get_seqnum(pMessage)))
            {
                vlist_insert(&pConnection->m_delayed.list, pMessage);
                connection_move_delayed_to_arrivals(pConnection);
            }
            else
            {
                message_destroy(pMessage);
            }
            break;
        case 254:
        case 255:
            VASSERT(!"this makes no sense");   // 4020
            break;
        default:
            {
                state_channel_t *pChannel;
                uns16 usSeqnum;




























                VASSERT(connection_allocate_state_channels(pConnection) == 0);   // 4054
                pChannel = &pConnection->m_pChannels[message_get_type(pMessage)];
                usSeqnum = message_get_seqnum(pMessage);
                if ((int16)(usSeqnum - pChannel->m_usRx) <= 0)
                {
                    message_destroy(pMessage);
                }
                else
                {


                    VASSERT((int16)(usSeqnum - pChannel->m_usRx) < 4096);   // 4065
                    connection_record_state_message(pConnection, pMessage, 0);
                }
            }
            break;
        }
    }
    return 0;
}

// The two halves of a software-to-software pair.  The far end's outstanding
// and queued messages become this end's arrivals with no wire in between.
void connection_short_circuit(connection_t *pConnection, connection_t *pWith)
{
    if (connection_has_instance_remote(pWith)
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
    if (pHeader->m_uiFlags & 0x2000)
        if (connection_has_instance_remote(pConnection))
            if (connection_get_instance_remote(pConnection) != pHeader->m_uiCrc)
                return -1;
    if (pHeader->m_uiFlags & 0x40000)
        if (connection_get_instance_local(pConnection) != pHeader->m_uiFlushNumber)
            return -1;
    if (!(pHeader->m_uiFlags & 0x42000))
        if (connection_send_instance_local(pConnection))
            return -1;
    drdp_format_endpoint1(pConnection->m_uiRemoteId);
    drdp_format_endpoint2(pConnection->m_uiLocalId);
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
    return drdp_is_leader_endpoint(endpoint_get_drdp(pConnection->m_pEndpoint),
                                   pConnection->m_uiRemoteId);
}

void connection_set_failure(connection_t *pConnection, int iRetry, int iWarn, int iFail)
{
    pConnection->m_iRetryTime = iRetry;
    pConnection->m_iWarnTime = iWarn;
    pConnection->m_iFailTime = iFail;
}

uns32 connection_get_time_last_arrival(const connection_t *pConnection)
{
    return pConnection->m_uiTimeLastArrival;
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

    if (connection_is_terminated(pConnection))
        iInSync = 1;
    else if (pConnection->m_usFlags & 0x1000)
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
    pConnection->m_uiRemoteId = uiRemoteId;
    drdp_format_endpoint(pConnection->m_szName, uiRemoteId);
}

// The soonest of the two, so a debug dump never reports a flush in the past.
uns32 connection_get_debug_time(connection_t *pConnection)
{
    uns32 uiTime = pConnection->m_uiTimeTx;

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
    "discarded (no memory was available to process it)",
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
