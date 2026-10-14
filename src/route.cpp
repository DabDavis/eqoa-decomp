#line 1 "C:/code-frontiers-gm/drdp/route.cpp"
// route.cpp -- SUPPORT 0x004bb370..0x004bc300, one UDP peer.
//
// A route is the socket address every connection to one machine shares. It
// owns the leaky bucket that paces that address, a tree of connections sorted
// by when each one wants to go out next, two sets of pending resets, and a
// list of raw bytes somebody asked to be forwarded verbatim.
//
// `route_flush` is the only place in the whole library that builds a packet:
// it opens a 1,500-byte buffer, writes the two port numbers, drains the two
// reset sets and the pending list into it, and then walks the connection tree
// in flush-time order asking each one for a segment until the buffer is full.
//
// The original is `C:/code-frontiers-gm/drdp/route.cpp` -- its asserts say so.
#include "drdp.h"

static const char ROUTE_TAG[] = "ROUTE";
static const char NULL_TAG[] = "NULL";

enum
{
    ROUTE_NODE_MAX = 2,
    ROUTE_KEY_MAX  = 2
};

// ONE_DAY, and the two flush windows the route pushes itself out to when it
// has nothing to do.
#define ONE_DAY 86400000






































































































int route_get_vnode_offset(int index)
{
    VASSERT(0 <= index && index < ROUTE_NODE_MAX);
    return index * 16 + 4;
}




int route_get_key_offset(int index)
{
    int iOffset = -1;

    VASSERT(0 <= index && index < ROUTE_KEY_MAX);
    switch (index)
    {
    case 0:
        iOffset = 48;
        break;
    case 1:
        iOffset = 40;
        break;
    }
    return iOffset;
}

// 3,000 bytes per second and a queue of 300 is the default pacing; the
// application raises it through `route_set_bps_limit`, which also resizes the
// queue to whatever one flush interval of that rate is worth.
int route_create(route_t **ppRoute, drdp_t *pDrdp, uns64 uiId)
{
    int iResult;
    route_t *pRoute = (route_t *)malloc(sizeof(route_t));

    if (!pRoute)
        iResult = -2;
    else
    {
        memcpy(pRoute->m_szTag, ROUTE_TAG, 4);
        pRoute->m_pDrdp = pDrdp;
        iResult = 0;
        pRoute->m_uiDesiredFlushTime = drdp_get_time(pDrdp);
        pRoute->m_uiId = uiId;
        vlist_init(&pRoute->m_pending, message_get_vlink_offset(0), 0, 0);
        vtree_init(&pRoute->m_connections.tree, connection_get_vnode_offset(2),
                   (vcmp_t)uns32_cmp_wrap, connection_get_key_offset(1));
        bandwidth_init(&pRoute->m_bandwidth, 3000, 300);
        vset_init(&pRoute->m_resetConnections, (vcmp_t)reset_connection_cmp,
                  sizeof(reset_connection_t));
        vset_init(&pRoute->m_resetEndpoints, (vcmp_t)reset_endpoint_cmp,
                  sizeof(reset_endpoint_t));
        pRoute->m_iDelay = 0;
        pRoute->m_iDrop = 0;
        pRoute->m_iJitter = 0;
        pRoute->m_iTerminated = 0;
        pRoute->m_iFlushNumber = 0;
        memset(pRoute->m_uiStat, 0, sizeof(pRoute->m_uiStat));
        *ppRoute = pRoute;
        pRoute = 0;
    }
    if (pRoute)
        route_destroy(pRoute);
    return iResult;
}

int route_is_valid(route_t *pRoute)
{
    return memcmp(pRoute->m_szTag, ROUTE_TAG, 4) == 0;
}
















void route_destroy(route_t *route)
{
    message_t *pMessage;

    VASSERT(route_is_valid( route ));
    memcpy(route->m_szTag, NULL_TAG, 4);
    while ((pMessage = (message_t *)vlist_remove_head(&route->m_pending)) != 0)
        message_destroy(pMessage);
    vset_remove_all(&route->m_resetConnections);
    vset_remove_all(&route->m_resetEndpoints);
    free(route);
}

uns32 route_get_desired_flush_time(route_t *pRoute)
{
    return pRoute->m_uiDesiredFlushTime;
}

// A route goes away when nothing is left on any of its four containers.
int route_destroy_check(route_t *pRoute)
{
    if (vtree_get_size(&pRoute->m_connections.tree))
        return 0;
    if (vset_get_size(&pRoute->m_resetConnections))
        return 0;
    if (vset_get_size(&pRoute->m_resetEndpoints))
        return 0;
    return vlist_get_size(&pRoute->m_pending) == 0;
}

void route_unlink_connection(connection_t *pConnection)
{
    route_t *pRoute = connection_get_route(pConnection);

    if (pRoute)
    {
        vtree_remove(&pRoute->m_connections.tree, pConnection);
        connection_set_route(pConnection, 0);
    }
}

void route_link_connection(route_t *pRoute, connection_t *pConnection)
{
    route_unlink_connection(pConnection);
    vtree_insert(&pRoute->m_connections.tree, pConnection);
    connection_set_route(pConnection, pRoute);
}

// Only a connection with a HARDWARE endpoint at one end is terminated: a pair
// of software endpoints does not use the route at all, so losing it means
// nothing to them.
void route_terminate(route_t *pRoute, uns8 ucReason)
{
    connection_t *pConnection;

    for (pConnection = (connection_t *)vtree_peek_first(&pRoute->m_connections.tree);
         pConnection;
         pConnection = (connection_t *)vtree_get_next(&pRoute->m_connections.tree, pConnection))
    {
        if (drdp_is_hardware_endpoint(connection_get_src(pConnection))
            || drdp_is_hardware_endpoint(connection_get_dst(pConnection)))
            connection_terminate_reason(pConnection, 0, ucReason);
    }
    pRoute->m_iTerminated = 1;
}

// The tree is keyed by a value the connection computes for itself, so moving
// one means taking it out, recomputing, and putting it back.
void connections_by_desired_flush_time_resort(connections_by_desired_flush_time_t *pTree,
                                              connection_t *pConnection)
{
    vtree_remove(&pTree->tree, pConnection);
    connection_recalc_desired_flush_time(pConnection);
    vtree_insert(&pTree->tree, pConnection);
}

void route_resort(connection_t *pConnection)
{
    route_t *pRoute = connection_get_route(pConnection);

    connections_by_desired_flush_time_resort(&pRoute->m_connections, pConnection);
    drdp_resort(pRoute);
}

// Each queued connection reset becomes a header-only segment: no body, and the
// two instance numbers carried in the crc and flush-number slots so the far
// end can tell a live connection from one that has already been replaced.
void route_flush_reset_connection(route_t *pRoute, buffer_t *pBuffer)
{
    reset_connection_t reset;
    segment_header_t header;

    while (buffer_bytes_remaining(pBuffer) >= 64
           && vset_remove_first(&pRoute->m_resetConnections, &reset) == 0)
    {
        memset(&header, 0, sizeof(header));
        header.flags = 0x10000;
        header.src = reset.m_uiSrc;
        header.dst = reset.m_uiDst;
        drdp_format_endpoint1(header.src);
        drdp_format_endpoint2(header.dst);
        if (reset.m_iFlags & 1)
        {
            header.flags |= 0x2000;
            header.m_uiCrc = reset.m_uiInstanceLocal;
        }
        if (reset.m_iFlags & 2)
        {
            header.flags |= 0x40000;
            header.m_uiFlushNumber = reset.m_uiInstanceRemote;
        }
        buffer_append_segment_header(pBuffer, &header);
    }
}

void route_flush_reset_endpoint(route_t *pRoute, buffer_t *pBuffer)
{
    segment_header_t header;
    reset_endpoint_t reset;

    while (buffer_bytes_remaining(pBuffer) >= 64
           && vset_remove_first(&pRoute->m_resetEndpoints, &reset) == 0)
    {
        memset(&header, 0, sizeof(header));
        header.flags = 0x20000;
        header.src = reset.m_uiId;
        drdp_format_endpoint1(reset.m_uiId);
        buffer_append_segment_header(pBuffer, &header);
    }
}

// The only packet builder in the library.  Two ports, then the resets, then
// the forwarded bytes, then as many connection segments as fit -- and the
// tree's ORDER is what makes the one that has waited longest go first.
//
// `connection_flush` answers -2 for "stop", -1 for "nothing to send, but I
// moved" and 0 for "here is a segment"; anything else is ignored.
void route_flush(route_t *pRoute)
{
    char data[1500];
    char cdata[1504];
    buffer_t buffer;
    buffer_t cbuffer;
    drdp_addr_t address;
    drdp_addr_t route;
    uns16 usPort;
    connection_t *pConnection;
    message_t *pMessage;
    int iStart;
    int iDebug = 0;

    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    drdp_endpoint_to_address(pRoute->m_uiId, address.m_ucIp, &address.m_usPort, &usPort);
    drdp_remap_address_to_route(pRoute->m_pDrdp, &address, &route);
    buffer_append_uns16(&buffer, drdp_get_port_drdp(pRoute->m_pDrdp));
    buffer_append_uns16(&buffer, usPort);
    iStart = buffer.position;
    drdp_format_endpoint1(pRoute->m_uiId);
    drdp_get_time(pRoute->m_pDrdp);
    route_flush_reset_connection(pRoute, &buffer);
    route_flush_reset_endpoint(pRoute, &buffer);
    while ((pMessage = (message_t *)vlist_peek_head(&pRoute->m_pending)) != 0)
    {
        if (message_get_size(pMessage) >= (uns32)buffer_bytes_remaining(&buffer))
            break;
        message_get_size(pMessage);
        buffer_appendm_uns8(&buffer, (const uns8 *)message_get_data(pMessage),
                            message_get_size(pMessage));
        vlist_remove_head(&pRoute->m_pending);
        message_destroy(pMessage);
    }
    while ((pConnection = (connection_t *)vtree_peek_first(&pRoute->m_connections.tree)) != 0
           && (int)(connection_get_desired_flush_time(pConnection)
                    - drdp_get_time(pRoute->m_pDrdp)) <= 0
           && buffer_bytes_remaining(&buffer) >= 128
           && connection_get_last_route_flush_number(pConnection) != pRoute->m_iFlushNumber)
    {
        int iResult;

        buffer_init(&cbuffer, cdata, buffer_bytes_remaining(&buffer) - 64,
                    BUFFER_LITTLE_ENDIAN);
        iResult = connection_flush(pConnection, &cbuffer, pRoute->m_iFlushNumber);
        if (iResult == -2)
            goto done;
        if (iResult < -1)
            continue;
        switch (iResult)
        {
        case 0:
        {
            segment_header_t header;

            drdp_format_endpoint1(connection_get_src(pConnection));
            drdp_format_endpoint2(connection_get_dst(pConnection));
            memset(&header, 0, sizeof(header));
            header.flags = 0;
            header.m_uiSize = cbuffer.position;
            header.src = connection_get_src(pConnection);
            header.dst = connection_get_dst(pConnection);
            if (connection_send_instance_local(pConnection))
            {
                header.flags |= 0x2000;
                if (!connection_has_instance_ack(pConnection))
                    header.flags |= 0x80000;
                header.m_uiCrc = connection_get_instance_local(pConnection);
                if (connection_send_instance_push(pConnection))
                    header.flags |= 0x4000;
            }
            buffer_append_segment_header(&buffer, &header);
            buffer_appendm_uns8(&buffer, cbuffer.data, cbuffer.position);
            iDebug |= connection_get_debug(pConnection);
            connections_by_desired_flush_time_resort(&pRoute->m_connections, pConnection);
            break;
        }
        case -1:
            connections_by_desired_flush_time_resort(&pRoute->m_connections, pConnection);
            break;
        }
    }
done:
    if (iStart < buffer.position)
    {
        drdp_crc_send(pRoute->m_pDrdp, &route, &buffer, pRoute->m_iDrop, pRoute->m_iDelay,
                      pRoute->m_iJitter);
        if (iDebug)
        {
            drdp_get_time(pRoute->m_pDrdp);
            drdp_format_addr1(&address);
            drdp_format_addr2(&route);
        }
        bandwidth_send(&pRoute->m_bandwidth, drdp_get_time(pRoute->m_pDrdp),
                       buffer.position + 28);
    }
    pRoute->m_iFlushNumber++;
    if (pRoute->m_iFlushNumber == -1)
        pRoute->m_iFlushNumber = 0;
}

// A terminated route, or one with nothing on it, sleeps for a day.  Otherwise
// the answer is the soonest of: the first connection's own time, now if a
// reset is queued, and whatever the leaky bucket says the wire is free.
void route_recalc_desired_flush_time(route_t *pRoute)
{
    connection_t *pConnection =
        (connection_t *)vtree_peek_first(&pRoute->m_connections.tree);
    uns32 uiTime;

    if (pRoute->m_iTerminated || !pConnection)
        pRoute->m_uiDesiredFlushTime = drdp_get_time(pRoute->m_pDrdp) + ONE_DAY;
    else
        pRoute->m_uiDesiredFlushTime = connection_get_desired_flush_time(pConnection);
    if (vset_get_size(&pRoute->m_resetConnections)
        || vset_get_size(&pRoute->m_resetEndpoints))
        if ((int)(pRoute->m_uiDesiredFlushTime - drdp_get_time(pRoute->m_pDrdp)) >= 0)
            pRoute->m_uiDesiredFlushTime = drdp_get_time(pRoute->m_pDrdp);
    uiTime = bandwidth_get_flush_time(&pRoute->m_bandwidth, drdp_get_time(pRoute->m_pDrdp));
    if ((int)(uiTime - pRoute->m_uiDesiredFlushTime) > 0)
        pRoute->m_uiDesiredFlushTime = uiTime;
}

void segment_header_print(segment_header_t *pHeader)
{
    drdp_format_endpoint1(pHeader->src);
    drdp_format_endpoint2(pHeader->dst);
    drdp_format_endpoint4(pHeader->m_uiRoute);
}

// A segment for somebody else: re-emit it into a private buffer with the route
// bit forced on and hang the bytes off the pending list, so the next flush
// carries them out verbatim.
void route_forward_segment(route_t *pRoute, segment_header_t *pHeader, buffer_t *pBuffer)
{
    buffer_t buffer;
    message_t *pMessage;
    char *data = (char *)__builtin_alloca(2047);

    buffer_init(&buffer, data, 2047, BUFFER_LITTLE_ENDIAN);
    pHeader->flags |= 0x8000;
    buffer_append_segment_header(&buffer, pHeader);
    buffer_copy(pBuffer, &buffer, pHeader->m_uiSize);
    if (message_create(&pMessage, 0, 0, 0, 0, buffer.data, buffer.position, 0, 0,
                       drdp_get_time(pRoute->m_pDrdp)) == 0)
    {
        vlist_add_tail(&pRoute->m_pending, pMessage);
        drdp_get_time(pRoute->m_pDrdp);
        drdp_format_endpoint3(pRoute->m_uiId);
    }
}

void route_set_simulated(route_t *pRoute, int iDrop, int iDelay, int iJitter)
{
    pRoute->m_iDrop = iDrop;
    pRoute->m_iDelay = iDelay;
    pRoute->m_iJitter = iJitter;
}

void route_set_bps_limit(route_t *pRoute, int iLimit)
{
    bandwidth_set_bps_limit(&pRoute->m_bandwidth, drdp_get_time(pRoute->m_pDrdp), iLimit);
    bandwidth_set_queue_size_max(&pRoute->m_bandwidth,
                                 (int)(iLimit * drdp_get_flush_interval(pRoute->m_pDrdp)));
}

int route_get_bps_limit(route_t *pRoute)
{
    return bandwidth_get_bps_limit(&pRoute->m_bandwidth);
}

int route_get_bps_sent(route_t *pRoute)
{
    return bandwidth_get_bps_sent(&pRoute->m_bandwidth);
}

int route_get_bps_recv(route_t *pRoute)
{
    return bandwidth_get_bps_recv(&pRoute->m_bandwidth);
}

int route_is_terminated(route_t *pRoute)
{
    return pRoute->m_iTerminated;
}

// Both comparators are the vset's deduplicator, so they have to walk every
// field: two resets that differ only in an instance number are two records.
int reset_connection_cmp(const reset_connection_t *pA, const reset_connection_t *pB)
{
    if (pA->m_uiSrc > pB->m_uiSrc)
        return 1;
    if (pA->m_uiSrc < pB->m_uiSrc)
        return -1;
    if (pA->m_uiDst > pB->m_uiDst)
        return 1;
    if (pA->m_uiDst < pB->m_uiDst)
        return -1;
    if (pA->m_iFlags > pB->m_iFlags)
        return 1;
    if (pA->m_iFlags < pB->m_iFlags)
        return -1;
    if (pA->m_uiInstanceLocal > pB->m_uiInstanceLocal)
        return 1;
    if (pA->m_uiInstanceLocal < pB->m_uiInstanceLocal)
        return -1;
    if (!(pA->m_uiInstanceRemote > pB->m_uiInstanceRemote))
        return pA->m_uiInstanceRemote < pB->m_uiInstanceRemote ? -1 : 0;
    return 1;
}

int reset_endpoint_cmp(const reset_endpoint_t *pA, const reset_endpoint_t *pB)
{
    if (!(pA->m_uiId > pB->m_uiId))
        return pA->m_uiId < pB->m_uiId ? -1 : 0;
    return 1;
}

void route_send_reset_connection(route_t *pRoute, uns64 uiSrc, uns64 uiDst, int iFlags,
                                 uns32 uiInstanceLocal, uns32 uiInstanceRemote)
{
    reset_connection_t reset;

    memset(&reset, 0, sizeof(reset));
    reset.m_uiSrc = uiSrc;
    reset.m_uiDst = uiDst;
    reset.m_iFlags = iFlags;
    if (iFlags & 1)
        reset.m_uiInstanceLocal = uiInstanceLocal;
    if (iFlags & 2)
        reset.m_uiInstanceRemote = uiInstanceRemote;
    vset_add(&pRoute->m_resetConnections, &reset);
    drdp_resort(pRoute);
}

void route_send_reset_endpoint(route_t *pRoute, uns64 uiId)
{
    reset_endpoint_t reset;

    reset.m_uiId = uiId;
    vset_add(&pRoute->m_resetEndpoints, &reset);
    drdp_resort(pRoute);
}

drdp_t *route_get_drdp(route_t *pRoute)
{
    return pRoute->m_pDrdp;
}

connection_t *route_get_first_connection(route_t *pRoute)
{
    return (connection_t *)vtree_peek_first(&pRoute->m_connections.tree);
}

connection_t *route_get_next_connection(route_t *pRoute, connection_t *pConnection)
{
    return (connection_t *)vtree_get_next(&pRoute->m_connections.tree, pConnection);
}

// Twenty-eight bytes of IP and UDP header are charged to the bucket on top of
// what actually arrived, the same way `route_flush` charges what it sends.
void route_record_arrival(route_t *pRoute, int iSize)
{
    bandwidth_recv(&pRoute->m_bandwidth, drdp_get_time(pRoute->m_pDrdp), iSize + 28);
}
