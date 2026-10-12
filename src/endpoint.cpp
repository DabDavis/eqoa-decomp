// endpoint.cpp -- SUPPORT 0x004b9218..0x004bab40, one addressable party.
//
// An endpoint owns a tree of connections keyed by the far id, one list of
// messages that have arrived for any of them, and a close set remembering the
// connections that have gone away.  It does NOT own the socket: the route
// does.  What it owns is identity -- `m_uiId` is the tree key the drdp keeps
// it under, and `m_uiInstance` is a counter seeded from the clock and handed
// out one at a time, so two connections made to the same far id at different
// moments are told apart even after both are closed.
//
// PARTIAL.  The lifetime, segment-handling and serialisation half of this file
// -- `connection_create`, `get_connection`, `send`, `send_reset_connection`,
// `handle_segment`, `connection_free_arrivals`, `_connection_destroy`,
// `destroy_buffer`, `reconcile_connections`, `create_buffer`, `transfer` and
// `close` -- reads and writes `connection_t` and `segment_header_t` fields
// directly, and those two layouts are settled in connection.cpp and drdp.cpp,
// neither of which is written yet.  What is here is everything that reaches
// the other structures only through their own accessors.
#include "drdp.h"

// A char ARRAY, not a literal: alignment 1, so the inline four-byte copy in
// `endpoint_create` comes out `lwl`/`lwr` into `swl`/`swr` (message.cpp).
static const char ENDPOINT_TAG[] = "ENDPOINT";

int endpoint_get_vnode_offset(int iWhich)
{
    return iWhich == 0 ? 4 : -1;
}

int endpoint_get_key_offset(int iWhich)
{
    return iWhich == 0 ? 32 : -1;
}

// The failure path is a goto-free cleanup: the local is NULLED on success, so
// the destroy at the bottom runs only if the initialisation fell out early.
int endpoint_create(endpoint_t **ppEndpoint, drdp_t *pDrdp, uns64 uiId)
{
    int iResult;
    endpoint_t *pEndpoint = (endpoint_t *)malloc(sizeof(endpoint_t));

    if (!pEndpoint)
        iResult = -2;
    else
    {
        memcpy(pEndpoint->m_szTag, ENDPOINT_TAG, 4);
        pEndpoint->m_uiId = uiId;
        pEndpoint->m_pDrdp = pDrdp;
        pEndpoint->m_pAppPtr = 0;
        iResult = 0;
        pEndpoint->m_uiInstance = drdp_get_time(pDrdp);
        pEndpoint->m_iFlags = 0;
        pEndpoint->m_uiA8 = 0;
        pEndpoint->m_uiAC = drdp_get_time(pEndpoint->m_pDrdp);
        vtree_init(&pEndpoint->m_connections, connection_get_vnode_offset(0),
                   (vcmp_t)uns64_cmp, connection_get_key_offset(0));
        vlist_init(&pEndpoint->m_arrivals, message_get_vlink_offset(0), 0, 0);
        close_set_init(&pEndpoint->m_closeSet);
        *ppEndpoint = pEndpoint;
        pEndpoint = 0;
    }
    if (pEndpoint)
        endpoint_destroy(pEndpoint);
    return iResult;
}

// A message for a closed endpoint, or for a closed connection, is dropped
// rather than queued -- the application is never handed one it cannot answer.
void endpoint_arrival_enqueue(endpoint_t *pEndpoint, message_t *pMessage)
{
    connection_t *pConnection = message_get_connection(pMessage);

    if (endpoint_is_closed(pEndpoint) || (pConnection && connection_is_closed(pConnection)))
        message_destroy(pMessage);
    else
    {
        vlist_add_tail(&pEndpoint->m_arrivals, pMessage);
        drdp_arrival_enqueue(pEndpoint->m_pDrdp, pMessage);
    }
}

// Every arrival is on TWO lists: this endpoint's and the drdp's, which is the
// one the application drains.  Both sides of every enqueue/remove pair here.
void endpoint_arrival_remove(endpoint_t *pEndpoint, message_t *pMessage)
{
    drdp_arrival_remove(pEndpoint->m_pDrdp, pMessage);
    vlist_remove(&pEndpoint->m_arrivals, pMessage);
}

void endpoint_connection_insert(endpoint_t *pEndpoint, connection_t *pConnection)
{
    vtree_insert(&pEndpoint->m_connections, pConnection);
    drdp_connection_insert(pEndpoint->m_pDrdp, pConnection);
}

// The remove takes no endpoint: the connection knows its own.
void endpoint_connection_remove(connection_t *pConnection)
{
    vtree_remove(&connection_get_endpoint(pConnection)->m_connections, pConnection);
    drdp_connection_remove(pConnection);
}

uns32 endpoint_get_instance(endpoint_t *pEndpoint)
{
    return pEndpoint->m_uiInstance++;
}

uns32 endpoint_get_time(endpoint_t *pEndpoint)
{
    return drdp_get_time(pEndpoint->m_pDrdp);
}

uns64 endpoint_get_id(const endpoint_t *pEndpoint)
{
    return pEndpoint->m_uiId;
}

// How many of the endpoint's queued arrivals belong to one connection.  The
// walk caches the next node BEFORE the body, which is what puts the
// `vlist_get_next` call ahead of `message_get_connection` in the target.
int endpoint_connection_get_arrival_count(connection_t *pConnection)
{
    endpoint_t *pEndpoint = connection_get_endpoint(pConnection);
    message_t *pMessage = (message_t *)vlist_peek_head(&pEndpoint->m_arrivals);
    int iCount = 0;

    while (pMessage)
    {
        message_t *pNext = (message_t *)vlist_get_next(&pEndpoint->m_arrivals, pMessage);

        if (message_get_connection(pMessage) == pConnection)
            iCount++;
        pMessage = pNext;
    }
    return iCount;
}

// Two questions, and the second is the one that matters: a freed endpoint may
// still carry its tag if the allocator has not reused the block, so the drdp
// is asked whether it still owns this pointer.
int endpoint_is_valid(const endpoint_t *pEndpoint)
{
    int iValid = memcmp(pEndpoint, ENDPOINT_TAG, 4) == 0;

    if (iValid)
        iValid = drdp_has_endpoint(pEndpoint->m_pDrdp, pEndpoint) != 0;
    return iValid;
}

int endpoint_has_connection(const endpoint_t *pEndpoint, const connection_t *pConnection)
{
    uns64 uiDst = connection_get_dst(pConnection);
    int iResult = endpoint_is_valid(pEndpoint);

    if (iResult)
        iResult = pConnection == vtree_lookup(&pEndpoint->m_connections, &uiDst);
    return iResult;
}

drdp_t *endpoint_get_drdp(endpoint_t *pEndpoint)
{
    return pEndpoint->m_pDrdp;
}

void endpoint_free_arrivals(endpoint_t *pEndpoint, buffer_t *pBuffer)
{
    message_t *pMessage;

    while ((pMessage = (message_t *)vlist_peek_head(&pEndpoint->m_arrivals)) != 0)
    {
        endpoint_arrival_remove(pEndpoint, pMessage);
        message_destroy_buffer(pMessage, pBuffer);
    }
}

void endpoint_destroy(endpoint_t *pEndpoint)
{
    endpoint_destroy_buffer(pEndpoint, 0);
}

void *endpoint_get_app_ptr(const endpoint_t *pEndpoint)
{
    return pEndpoint->m_pAppPtr;
}

void endpoint_set_app_ptr(endpoint_t *pEndpoint, void *pAppPtr)
{
    pEndpoint->m_pAppPtr = pAppPtr;
}

// Two software endpoints are never allowed to talk to each other directly --
// the pair is rejected here even though the tree found the connection.
connection_t *endpoint_find_connection(endpoint_t *pEndpoint, uns64 uiDst)
{
    connection_t *pConnection = (connection_t *)vtree_lookup(&pEndpoint->m_connections, &uiDst);

    if (!pConnection)
        return 0;
    if (drdp_is_software_endpoint(pEndpoint->m_uiId) && drdp_is_software_endpoint(uiDst))
        return 0;
    if (connection_is_closed(pConnection))
        pConnection = 0;
    return pConnection;
}

int endpoint_open(endpoint_t **ppEndpoint, drdp_t *pDrdp)
{
    return drdp_endpoint_create(pDrdp, ppEndpoint);
}

int endpoint_is_closed(endpoint_t *pEndpoint)
{
    return pEndpoint->m_iFlags & 1;
}

int endpoint_is_in_sync_internal(endpoint_t *pEndpoint)
{
    connection_t *pConnection;

    for (pConnection = (connection_t *)vtree_peek_first(&pEndpoint->m_connections);
         pConnection;
         pConnection = (connection_t *)vtree_get_next(&pEndpoint->m_connections, pConnection))
    {
        if (!connection_is_in_sync(pConnection))
            return 0;
    }
    return 1;
}

// Bit 1 is a LATCH, not a state: once every connection has been in sync at the
// same moment the endpoint reports in sync for ever, and the walk is never run
// again.  So this answers "has it ever caught up", not "is it caught up now".
int endpoint_is_in_sync(endpoint_t *pEndpoint)
{
    if (!(pEndpoint->m_iFlags & 2) && endpoint_is_in_sync_internal(pEndpoint))
        pEndpoint->m_iFlags |= 2;
    return (pEndpoint->m_iFlags >> 1) & 1;
}
