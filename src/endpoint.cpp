#line 1 "C:/code-frontiers-gm/drdp/endpoint.cpp"
// endpoint.cpp -- SUPPORT 0x004b9218..0x004baa20, one addressable party.
//
// An endpoint owns a tree of connections keyed by the far id, one list of
// messages that have arrived for any of them, and a close set remembering the
// connections that have gone away.  It does NOT own the socket: the route
// does.  What it owns is identity -- `m_uiId` is the tree key the drdp keeps
// it under, and `m_uiInstance` is a counter seeded from the clock and handed
// out one at a time, so two connections made to the same far id at different
// moments are told apart even after both are closed.
//
// The original is `C:/code-frontiers-gm/drdp/endpoint.cpp`, a little over
// 1,200 lines -- its asserts say so -- and the source below is padded to that
// layout: an assert's `__LINE__` is an immediate, and so is the line the
// `endpoint_connection_destroy` macro hands `_endpoint_connection_destroy`.
#include "drdp.h"

// A char ARRAY, not a literal: alignment 1, so the inline four-byte copy in
// `endpoint_create` comes out `lwl`/`lwr` into `swl`/`swr` (message.cpp).
static const char ENDPOINT_TAG[] = "ENDPOINT";
static const char NULL_TAG[] = "NULL";

// The asserting form that takes a printf format: the location is parked first
// and the format quoted with it (uassert.cpp).
#define VASSERTF(x, f) _uassert_set_location(__FILE__, __LINE__), _uassertf((x), (f))

// Every destroy says where it came from; the close set records it.
#define endpoint_connection_destroy(c) _endpoint_connection_destroy((c), __FILE__, __LINE__)

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

// A connection is made once per far id: a second one, or one to an endpoint
// with no route, is refused.  Bit 0 of the flags says "we opened it", bit 1
// "the far end's instance is already known".
int endpoint_connection_create(endpoint_t *pEndpoint, connection_t **ppConnection, uns64 uiDst,
                               uns64 uiRoute, uns32 uiInstance, int iFlags)
{
    int iResult;
    route_t *pRoute;
    connection_t *pConnection = 0;

    // Two failure arms, not one `||`: the second `iResult = -1` is what
    // ranks iResult below uiInstance in global-alloc (s4/s3, as the target).
    if (uiRoute == (uns64)-1)
        iResult = -1;
    else if (vtree_lookup(&pEndpoint->m_connections, &uiDst))
        iResult = -1;
    else
    {
        iResult = drdp_get_route(pEndpoint->m_pDrdp, uiRoute, &pRoute);
        if (iResult == 0)
        {
            int iConnectionFlags = iFlags & 1;

            if (iFlags & 2)
                iConnectionFlags |= 2;
            iResult = connection_create(&pConnection, pEndpoint, uiDst, uiInstance,
                                        iConnectionFlags);
            if (iResult == 0)
            {
                endpoint_connection_insert(pEndpoint, pConnection);
                route_link_connection(pRoute, pConnection);
                drdp_format_endpoint1(connection_get_src(pConnection));
                drdp_format_endpoint2(connection_get_dst(pConnection));
                connection_set_logger(pConnection,
                                      drdp_get_default_logger(pEndpoint->m_pDrdp));
                *ppConnection = pConnection;
                pConnection = 0;
            }
        }
    }
    if (pConnection)
        connection_destroy(pConnection);
    return iResult;
}

// Find the connection to a far id, or make one -- but only between two
// SOFTWARE endpoints, which is the one pairing `find_connection` refuses.
int endpoint_get_connection(endpoint_t *pEndpoint, uns64 uiDst, connection_t **ppConnection)
{
    int iResult;
    connection_t *pConnection = 0;

    pConnection = (connection_t *)vtree_lookup(&pEndpoint->m_connections, &uiDst);
    if (!pConnection)
    {
        if (!drdp_is_software_endpoint(uiDst)
            || !drdp_is_software_endpoint(endpoint_get_id(pEndpoint)))
        {
            iResult = -1;
            goto done;
        }
        iResult = endpoint_connection_create(pEndpoint, &pConnection, uiDst,
                                             drdp_get_route_endpoint(pEndpoint->m_pDrdp, uiDst),
                                             endpoint_get_instance(pEndpoint), 0);
        if (iResult)
            goto done;
    }
    *ppConnection = pConnection;
    pConnection = 0;
    iResult = 0;
done:
    if (pConnection)
        connection_destroy(pConnection);
    return iResult;
}





















































































































// Three ways out.  A far id the drdp has no route for comes straight back as
// a one-byte system message, type 22 "unreachable", queued as if it had
// arrived from the far end.  A far id that lives in this same process is
// delivered by building the message and enqueuing it on that endpoint --
// no connection, no wire.  Everything else goes through a connection.
void endpoint_send(endpoint_t *endpoint, uns64 uiDst, const void *pData, int size, int message_type)
{
    message_t *pMessage = 0;
    endpoint_t *pTo;
    int result;

    VASSERT( !endpoint_is_closed( endpoint ) );   // 315
    VASSERT( size <= connection_get_max_message_size( message_type ) );   // 316
    if (!drdp_is_client(endpoint->m_pDrdp)
        && drdp_get_route_endpoint(endpoint->m_pDrdp, uiDst) == (uns64)-1)
    {
        buffer_t buffer;
        uns8 data[16];
        message_t *pUnreachable;

        buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
        buffer_append_uns8(&buffer, 22);
        if (message_create(&pUnreachable, endpoint, 0, uiDst, endpoint_get_id(endpoint),
                           buffer.data, buffer.position, 249, 0,
                           drdp_get_time(endpoint->m_pDrdp)) == 0)
            endpoint_arrival_enqueue(endpoint, pUnreachable);
        return;
    }
    pTo = drdp_find_endpoint(endpoint->m_pDrdp, uiDst);
    if (!pTo)
    {
        connection_t *pConnection = 0;

        result = endpoint_get_connection(endpoint, uiDst, &pConnection);




























        VASSERT( result == 0 );   // 366
        result = connection_send_internal(pConnection, (const uns8 *)pData, size, message_type);


        VASSERT( result == 0 );   // 370
    }
    else
    {
        result = message_create(&pMessage, pTo, 0, endpoint_get_id(endpoint), uiDst, pData,
                                 size, message_type, 0, drdp_get_time(endpoint->m_pDrdp));












        VASSERT( result == 0 );   // 388
        endpoint_arrival_enqueue(pTo, pMessage);
    }
}

uns32 endpoint_get_time(endpoint_t *pEndpoint)
{
    return drdp_get_time(pEndpoint->m_pDrdp);
}

uns64 endpoint_get_id(const endpoint_t *pEndpoint)
{
    return pEndpoint->m_uiId;
}


















// A hardware pair is reset over the wire.  A software pair is answered by
// the drdp's knowledge of the source: status 2 means the source lives behind
// a route the header names, and nothing is sent at all.
void endpoint_send_reset_connection(endpoint_t *pEndpoint, const segment_header_t *segment_header)
{
    int iStatus;

    if (drdp_is_hardware_endpoint(segment_header->src) || drdp_is_hardware_endpoint(segment_header->dst))
    {
        drdp_send_reset_connection(pEndpoint->m_pDrdp, segment_header);
        return;
    }
    iStatus = drdp_get_sw_endpoint_status(pEndpoint->m_pDrdp, segment_header->src);
    VASSERT( drdp_is_software_endpoint( segment_header->src ) && drdp_is_software_endpoint( segment_header->dst ) );   // 433
    switch (iStatus)
    {
    case 0:
    case 1:
        break;
    case 2:







        VASSERT( segment_header->flags & SEGMENT_HEADER_SRC_ADDR );   // 447
        break;
    }
}









// One segment for this endpoint.  A segment from a connection this endpoint
// closed in the last two minutes is answered with a reset.  An unknown
// sender that carries an instance number opens a connection; one that does
// not is reset.  A remote hardware endpoint that has moved to a new drdp port
// is found under its LEADER port (0xfffe) and re-keyed in place.
int endpoint_handle_segment(endpoint_t *pEndpoint, segment_header_t *segment_header, buffer_t *pBuffer)
{
    int iResult;
    int iCreated = 0;
    connection_t *pConnection = 0;

    if (segment_header->flags & 0x2000)
    {
        if (close_set_lookup(&pEndpoint->m_closeSet, segment_header->src, segment_header->m_uiCrc))
        {
            if (segment_header->flags & 0x2000)
            {
                drdp_format_endpoint1(segment_header->src);
                drdp_format_endpoint2(segment_header->dst);
            }
            endpoint_send_reset_connection(pEndpoint, segment_header);
            goto skip;
        }
    }
    close_set_release_expired(&pEndpoint->m_closeSet, drdp_get_time(pEndpoint->m_pDrdp));
    VASSERT( ( segment_header->flags & SEGMENT_HEADER_RESET_ENDPOINT ) == 0 );   // 485
    pConnection = (connection_t *)vtree_lookup(&pEndpoint->m_connections, &segment_header->src);
    if (!pConnection)
    {
        if (drdp_is_remote_hardware_endpoint(segment_header->src))
        {
            drdp_addr_t addr;
            uns16 usDrdpPort;
            uns64 uiLeader;

            drdp_endpoint_to_address(segment_header->src, addr.m_ucIp, &addr.m_usPort, &usDrdpPort);
            drdp_address_to_endpoint(addr.m_ucIp, addr.m_usPort, DRDP_PORT_LEADER, &uiLeader);
            pConnection = (connection_t *)vtree_lookup(&pEndpoint->m_connections, &uiLeader);
            if (pConnection)
            {
                route_t *pRoute;
                int iLimit;

                iResult = drdp_get_route(pEndpoint->m_pDrdp,
                                         drdp_get_route_endpoint(pEndpoint->m_pDrdp,
                                                                 segment_header->src),
                                         &pRoute);
                if (iResult)
                    goto out;
                iLimit = connection_get_bps_limit(pConnection);
                endpoint_connection_remove(pConnection);
                route_unlink_connection(pConnection);
                connection_set_remote_id(pConnection, segment_header->src);
                route_link_connection(pRoute, pConnection);
                endpoint_connection_insert(pEndpoint, pConnection);
                connection_set_bps_limit(pConnection, iLimit);
            }
        }
    }
    if (pConnection)
    {
        if (segment_header->flags & 0x10000)
        {
            iResult = connection_handle_reset(pConnection, segment_header);
            goto out;
        }
        if ((segment_header->flags & 0x2000) && connection_has_instance_remote(pConnection)
            && segment_header->m_uiCrc != connection_get_instance_remote(pConnection)
            && segment_header->m_uiCrc != connection_get_instance_remote(pConnection))
        {
            if (drdp_is_hardware_endpoint(segment_header->src)
                || drdp_is_hardware_endpoint(segment_header->dst))
            {
                if (segment_header->flags & 0x4000)
                    connection_terminate_reason(pConnection, 1, 11);
            }
            else
                connection_rx_reset(pConnection);
        }
    }
    if (!pConnection)
    {
        if ((segment_header->flags & 0x2000) && !(segment_header->flags & 0x10000))
        {
            uns64 uiRoute;
            uns32 uiInstance;
            int iFlags;

            if (drdp_is_client(pEndpoint->m_pDrdp))
                uiRoute = segment_header->m_uiRoute;
            else
                uiRoute = drdp_get_route_endpoint(pEndpoint->m_pDrdp, segment_header->src);
            if (segment_header->flags & 0x4000)
            {
                uiInstance = segment_header->m_uiCrc;
                iFlags = 2;
            }
            else
            {
                uiInstance = endpoint_get_instance(pEndpoint);
                iFlags = 0;
            }
            iCreated = 1;
            iResult = endpoint_connection_create(pEndpoint, &pConnection, segment_header->src, uiRoute,
                                                 uiInstance, iFlags);
            if (iResult)
                goto out;
        }
        else
            endpoint_send_reset_connection(pEndpoint, segment_header);
    }
    if (pConnection)
    {
        if (connection_is_terminated(pConnection))
        {
            uns8 ucReason = 4;

            connection_send_reset(pConnection, &ucReason, 1);
        }
        else
        {
            buffer_t buffer;

            buffer_init_read(&buffer, buffer_get_position_ptr(pBuffer), segment_header->m_uiSize,
                             BUFFER_LITTLE_ENDIAN);
            iResult = connection_handle_segment(pConnection, segment_header, &buffer);
            if (iResult)
            {
                if (iCreated)

































































                    endpoint_connection_destroy(pConnection);   // 654
                goto out;
            }
        }
    }
skip:
    buffer_skip(pBuffer, segment_header->m_uiSize);
    iResult = 0;
out:
    return iResult;
}

// A connection going away takes its arrivals with it -- except between two
// software endpoints, where the message outlives the connection and is only
// cut loose from it.
void endpoint_connection_free_arrivals(connection_t *pConnection)
{
    endpoint_t *pEndpoint = connection_get_endpoint(pConnection);
    message_t *pMessage = (message_t *)vlist_peek_head(&pEndpoint->m_arrivals);

    while (pMessage)
    {
        message_t *pNext = (message_t *)vlist_get_next(&pEndpoint->m_arrivals, pMessage);

        if (message_get_connection(pMessage) == pConnection)
        {
            if (drdp_is_software_endpoint(message_get_src(pMessage))
                && drdp_is_software_endpoint(message_get_dst(pMessage)))
                message_set_connection(pMessage, 0);
            else
            {
                endpoint_arrival_remove(pEndpoint, pMessage);
                message_destroy(pMessage);
            }
        }
        pMessage = pNext;
    }
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





































// Reached through the `endpoint_connection_destroy` macro, which hands it the
// caller's file and line -- the close set keeps them and nothing reads them.
// A connection whose far instance is known goes into the close set, so a late
// segment from it is reset rather than re-opening it.
void _endpoint_connection_destroy(connection_t *pConnection, const char *pFile, int iLine)
{
    endpoint_t *pEndpoint = connection_get_endpoint(pConnection);
    int result;

    endpoint_connection_remove(pConnection);
    route_unlink_connection(pConnection);
    if (!connection_is_closed(pConnection))
        endpoint_connection_free_arrivals(pConnection);
    drdp_format_endpoint1(connection_get_src(pConnection));
    drdp_format_endpoint2(connection_get_dst(pConnection));
    if (connection_has_instance_remote(pConnection))
    {
        result = close_set_insert(&pEndpoint->m_closeSet, connection_get_dst(pConnection),
                                   connection_get_instance_remote(pConnection),
                                   connection_get_time_last_instance_arrival(pConnection),
                                   drdp_get_time(pEndpoint->m_pDrdp), pFile, iLine);
        VASSERT( result == 0 );   // 770
    }
    connection_destroy(pConnection);
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

// Serialise and free.  The record is the tag, the id, the instance counter,
// the flags, the two counts and +0xA8; the close set and every connection
// follow it, then the arrivals.  With no buffer it is a plain destroy.
void endpoint_destroy_buffer(endpoint_t *pEndpoint, buffer_t *pBuffer)
{
    connection_t *pConnection;

    vtree_get_size(&pEndpoint->m_connections);
    vlist_get_size(&pEndpoint->m_arrivals);
    drdp_format_endpoint1(pEndpoint->m_uiId);
    if (pBuffer)
    {
        int iCount = vtree_get_size(&pEndpoint->m_connections);

        buffer_appendm_uns8(pBuffer, (const uns8 *)pEndpoint->m_szTag, 4);
        buffer_append_packed_uns64(pBuffer, pEndpoint->m_uiId);
        buffer_append_uns32(pBuffer, pEndpoint->m_uiInstance);
        buffer_append_uns32(pBuffer, pEndpoint->m_iFlags);
        buffer_append_packed_uns64(pBuffer, iCount);
        buffer_append_packed_uns64(pBuffer, vlist_get_size(&pEndpoint->m_arrivals));
        buffer_append_packed_uns32(pBuffer, pEndpoint->m_uiA8);
    }
    close_set_destroy_buffer(&pEndpoint->m_closeSet, pBuffer, drdp_get_time(pEndpoint->m_pDrdp));
    while ((pConnection = (connection_t *)vtree_peek_first(&pEndpoint->m_connections)) != 0)
    {
        endpoint_connection_remove(pConnection);
        route_unlink_connection(pConnection);
        connection_destroy_buffer(pConnection, pBuffer);
    }
    if (!endpoint_is_closed(pEndpoint))
        endpoint_free_arrivals(pEndpoint, pBuffer);
    memcpy(pEndpoint->m_szTag, NULL_TAG, 4);
    free(pEndpoint);
}

void endpoint_destroy(endpoint_t *pEndpoint)
{
    endpoint_destroy_buffer(pEndpoint, 0);
}














































































// Two endpoints in this process each hold a connection to the other: tie the
// pair together so each side delivers straight into the other, then drop
// both connections.
void endpoint_reconcile_connections(endpoint_t *pEndpoint, connection_t *pConnection,
                                    endpoint_t *pWith)
{
    connection_t *pOther = 0;
    int result;
    uns64 uiId = endpoint_get_id(pEndpoint);
    uns64 uiWith = endpoint_get_id(pWith);

    drdp_format_endpoint1(uiId);
    drdp_format_endpoint2(uiWith);
    result = endpoint_get_connection(pWith, endpoint_get_id(pEndpoint), &pOther);
    VASSERT( result == 0 );   // 944
    connection_short_circuit(pConnection, pOther);
    connection_short_circuit(pOther, pConnection);

    endpoint_connection_destroy(pConnection);   // 948
    endpoint_connection_destroy(pOther);   // 949
}
















// The inverse of `destroy_buffer`.  Each restored connection is re-linked: to
// the local endpoint it names if there is one, else onto its route, and one
// whose far end has no route any more is dropped.
int endpoint_create_buffer(endpoint_t **ppEndpoint, drdp_t *pDrdp, buffer_t *pBuffer)
{
    int iResult;
    uns64 uiConnections;
    uns64 uiArrivals;
    connection_t *pConnection;
    route_t *pRoute;
    message_t *pMessage;
    int i;
    endpoint_t *pEndpoint = (endpoint_t *)malloc(sizeof(endpoint_t));

    if (!pEndpoint)
    {
        iResult = -2;
        goto done;
    }
    pEndpoint->m_pDrdp = pDrdp;
    pEndpoint->m_pAppPtr = 0;
    pEndpoint->m_uiAC = drdp_get_time(pDrdp);
    vtree_init(&pEndpoint->m_connections, connection_get_vnode_offset(0),
               (vcmp_t)uns64_cmp, connection_get_key_offset(0));
    vlist_init(&pEndpoint->m_arrivals, message_get_vlink_offset(0), 0, 0);
    close_set_init(&pEndpoint->m_closeSet);
    buffer_readm_uns8(pBuffer, (uns8 *)pEndpoint->m_szTag, 4);
    buffer_read_packed_uns64(pBuffer, &pEndpoint->m_uiId);
    buffer_read_uns32(pBuffer, &pEndpoint->m_uiInstance);
    buffer_read_uns32(pBuffer, (uns32 *)&pEndpoint->m_iFlags);
    buffer_read_packed_uns64(pBuffer, &uiConnections);
    buffer_read_packed_uns64(pBuffer, &uiArrivals);
    buffer_read_packed_uns32(pBuffer, &pEndpoint->m_uiA8);
    pEndpoint->m_uiA8++;
    iResult = close_set_create_buffer(&pEndpoint->m_closeSet, pBuffer,
                                      drdp_get_time(pEndpoint->m_pDrdp));
    if (iResult)
        goto done;
    for (i = 0; i < uiConnections; i++)
    {
        endpoint_t *pWith;

        iResult = connection_create_buffer(&pConnection, pEndpoint, pBuffer);
        if (iResult)
            goto done;
        endpoint_connection_insert(pEndpoint, pConnection);
        pWith = drdp_find_endpoint(pEndpoint->m_pDrdp, connection_get_dst(pConnection));
        if (!pWith)
        {
            uns64 uiRoute = drdp_get_route_endpoint(pEndpoint->m_pDrdp,
                                                    connection_get_dst(pConnection));

            if (uiRoute == (uns64)-1)
                endpoint_connection_destroy(pConnection);   // 1020
            else
            {
                iResult = drdp_get_route(pEndpoint->m_pDrdp, uiRoute, &pRoute);
                if (iResult)
                    goto done;
                route_link_connection(pRoute, pConnection);
            }
        }
        else
            endpoint_reconcile_connections(pEndpoint, pConnection, pWith);
    }
    drdp_reconcile_local_endpoints(pDrdp, pEndpoint);
    for (i = 0; i < uiArrivals; i++)
    {
        uns64 uiSrc;
        connection_t *pOwner;

        iResult = message_create_buffer(&pMessage, pEndpoint, 0, pBuffer);
        if (iResult)
            goto done;
        uiSrc = message_get_src(pMessage);
        pOwner = (connection_t *)vtree_lookup(&pEndpoint->m_connections, &uiSrc);
        message_set_connection(pMessage, pOwner);
        endpoint_arrival_enqueue(pEndpoint, pMessage);
    }
    *ppEndpoint = pEndpoint;
    iResult = 0;
    pEndpoint = 0;
done:
    if (pEndpoint)
        endpoint_destroy(pEndpoint);
    return iResult;
}

void *endpoint_get_app_ptr(const endpoint_t *pEndpoint)
{
    return pEndpoint->m_pAppPtr;
}

void endpoint_set_app_ptr(endpoint_t *pEndpoint, void *pAppPtr)
{
    pEndpoint->m_pAppPtr = pAppPtr;
}








































// Hand an endpoint to another process: serialise the whole thing into one
// system message (type 17) to the master, which forwards it, and destroy the
// local copy.  The buffer is half a megabyte ON THE STACK.
void endpoint_transfer(endpoint_t *pEndpoint, uns64 uiTo, const void *pData, int iSize)
{
    connection_t *pMaster = drdp_get_master_connection(pEndpoint->m_pDrdp);
    buffer_t buffer;
    uns8 data[0x80000];

    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    buffer_append_uns8(&buffer, 17);
    buffer_append_uns64(&buffer, endpoint_get_id(pEndpoint));
    buffer_append_uns64(&buffer, uiTo);
    buffer_append_uns64(&buffer, iSize);
    buffer_appendm_uns8(&buffer, (const uns8 *)pData, iSize);
    endpoint_destroy_buffer(pEndpoint, &buffer);
    VASSERTF( !buffer.overflow, "endpoint_transfer: buffer overflow" );   // 1120
    connection_send(pMaster, buffer.data, buffer.position, 249);
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
















































// A closed endpoint drops its arrivals at once, but is only destroyed when
// every connection is in sync -- until then it waits for the far ends.
void endpoint_close(endpoint_t *endpoint)
{
    VASSERT( !endpoint_is_closed( endpoint ) );   // 1224
    endpoint_free_arrivals(endpoint, 0);
    endpoint->m_iFlags |= 1;
    if (endpoint_is_in_sync(endpoint))
        drdp_endpoint_destroy(endpoint);
}

