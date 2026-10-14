// drdp.cpp -- SUPPORT 0x004b4aa8..0x004b9218, the top of the DRDP library.
//
// A `drdp_t` is one PROCESS on the network.  It owns the sockets, every
// endpoint, route and connection it knows about, the address<->route cache and
// the leaky bucket that paces the whole machine.  Everything below it in this
// library -- the buffer, the message, the connection, the route -- exists to
// serve the four entry points here: `drdp_flush`, `drdp_process_packet`,
// `drdp_receive` and `drdp_set_time`.
//
// Three ROLES share the file.  A MASTER owns the whole software endpoint id
// space and hands it out a block at a time; a LEADER is given a block and asks
// for another when it runs dry; a CLIENT has neither and talks to one leader.
// The system messages that arbitrate all that are the switch in
// `drdp_process_system_message`, the largest function in the library.
//
// The original is `C:/code-frontiers-gm/drdp/drdp.cpp` -- its asserts say so,
// and they name six of the struct's fields outright.  It is a little over three
// thousand lines, which is why the source below is padded: the line number an
// assert reports is an immediate that has to match, so where each one lands is
// as load-bearing as the code around it (phase 27).  An assert split over two
// source lines reports the SECOND one, so every one of them is a single line.
#include "drdp.h"

static const char DRDP_TAG[] = "DRDP";
static const char NULL_TAG[] = "NULL";
static const char INVALID_NAME[] = "(invalid)";

uns32 drdp_crc_table[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

// The two callbacks a caller may leave out.  They are real functions, not
// null checks at the call site: `drdp_create_addr` substitutes them and every
// use is an unconditional indirect call.
void default_callback_leader_add(drdp_t *pDrdp, connection_t *pConnection)
{
}

void default_callback_leader_remove(drdp_t *pDrdp, connection_t *pConnection)
{
}

// The two names for the same six bytes.  A `drdp_addr_t` is what the library
// talks in and an `ipaddress_t` is what the socket layer takes, and they are
// the same shape -- so both directions are one block move of the octets and
// one halfword of the port.
void drdp_addr_to_ipaddress(const drdp_addr_t *pFrom, ipaddress_t *pTo)
{
    memcpy(pTo->m_ucIp, pFrom->m_ucIp, 4);
    pTo->m_usPort = pFrom->m_usPort;
}

void ipaddress_to_drdp_addr(const ipaddress_t *pFrom, drdp_addr_t *pTo)
{
    memcpy(pTo->m_ucIp, pFrom->m_ucIp, 4);
    pTo->m_usPort = pFrom->m_usPort;
}

// The master takes a block for ITSELF out of the pool it owns, and tells every
// leader where the pool now starts so their own route tables agree.
void drdp_assign_endpoint_block_master(drdp_t *pDrdp)
{
    leader_t *pLeader;















































    VASSERT( pDrdp->m_endpointBlock[1].remaining >= ENDPOINT_BLOCK_SIZE );   // 175

    pDrdp->m_endpointBlock[0].next      = pDrdp->m_endpointBlock[1].next;
    pDrdp->m_endpointBlock[0].remaining = ENDPOINT_BLOCK_SIZE;

    route_table_update_block(&pDrdp->m_routeTable, pDrdp->m_endpointBlock[1].next,
                             endpoint_get_id(pDrdp->m_pSystemEndpoint));

    for (pLeader = (leader_t *)vtree_peek_first(&pDrdp->m_leaders); pLeader;
         pLeader = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pLeader))
    {
        connection_send_uns8_uns64(pLeader->m_pConnection, 36,
                                   pDrdp->m_endpointBlock[1].next);
    }

    pDrdp->m_endpointBlock[1].next      += ENDPOINT_BLOCK_SIZE;
    pDrdp->m_endpointBlock[1].remaining -= ENDPOINT_BLOCK_SIZE;
}

// The same, for a leader on the far end of `pConnection`.
void drdp_assign_endpoint_block(drdp_t *pDrdp, connection_t *pConnection)
{

























    VASSERT( ( pDrdp->m_endpointBlock[1].next & ( ENDPOINT_BLOCK_SIZE-1 ) ) == 0 );   // 222
    VASSERT( pDrdp->m_endpointBlock[1].remaining >= ENDPOINT_BLOCK_SIZE );   // 223

    route_table_update_block(&pDrdp->m_routeTable, pDrdp->m_endpointBlock[1].next,
                             connection_get_dst(pConnection));
    connection_send_uns8_uns64(pConnection, 33, pDrdp->m_endpointBlock[1].next);

    pDrdp->m_endpointBlock[1].next      += ENDPOINT_BLOCK_SIZE;
    pDrdp->m_endpointBlock[1].remaining -= ENDPOINT_BLOCK_SIZE;
}

// The constructor every role goes through.  544 bytes off malloc, six
// containers, two sockets and a system endpoint whose drdp port is the low
// half of the clock -- stepped off the three reserved values.
int drdp_create_addr(drdp_t **ppDrdp, const drdp_addr_t *pAddress, int iRole, uns32 uiTime,
                     bool bNoSend, drdp::logger_t *pLogger, void *pAppPtr,
                     drdp_leader_callback_t pfnLeaderAdd,
                     drdp_leader_callback_t pfnLeaderRemove,
                     drdp_endpoint_callback_t pfnEndpointDestroyed,
                     int iBpsLimit, int iQueueSizeMax)
{
    ipaddress_t address;
    drdp_t *pDrdp;
    uns16 usPort;
    int iResult;
    int i;

    pDrdp = (drdp_t *)malloc(sizeof(drdp_t));
    if (!pDrdp)
    {
        iResult = -2;
        goto done;
    }

    memset(pDrdp, 0, sizeof(drdp_t));
    memcpy(pDrdp->m_szTag, DRDP_TAG, 4);

    pDrdp->m_bNoSend = bNoSend;
    pDrdp->m_pLogger = pLogger;
    pDrdp->m_iNumSocket = 0;
    pDrdp->m_iSocket = 0;
    pDrdp->m_uiTime = uiTime;
    pDrdp->m_uiTimeRecv = uiTime;
    pDrdp->m_uiTimeFlush = uiTime;
    pDrdp->m_iBytesSent = 0;
    pDrdp->m_iBytesRecv = 0;
    pDrdp->m_iBytesDropped = 0;
    pDrdp->m_uiTimeBps = uiTime;
    pDrdp->m_iRole = iRole;
    pDrdp->m_pMasterConnection = 0;
    pDrdp->m_pData = 0;
    pDrdp->m_iDataSize = 0;
    pDrdp->m_pSystemEndpoint = 0;
    pDrdp->m_i210 = 0;

    memset(pDrdp->m_endpointBlock, 0, sizeof(pDrdp->m_endpointBlock));

    bandwidth_init(&pDrdp->m_bandwidth, iBpsLimit, iQueueSizeMax);
    route_table_init(&pDrdp->m_routeTable, DRDP_SW_ENDPOINT_MIN, DRDP_SW_ENDPOINT_MAX);
    route_cache_init(&pDrdp->m_routeCache);

    pDrdp->m_pAppPtr = pAppPtr;
    if (pfnLeaderAdd == 0)
        pDrdp->m_pfnLeaderAdd = default_callback_leader_add;
    else
        pDrdp->m_pfnLeaderAdd = pfnLeaderAdd;
    if (pfnLeaderRemove == 0)
        pDrdp->m_pfnLeaderRemove = default_callback_leader_remove;
    else
        pDrdp->m_pfnLeaderRemove = pfnLeaderRemove;
    pDrdp->m_pfnEndpointDestroyed = pfnEndpointDestroyed;
    pDrdp->m_usProtocolVersion = 1;

    vtree_init(&pDrdp->m_endpoints, endpoint_get_vnode_offset(0), (vcmp_t)uns64_cmp,
               endpoint_get_key_offset(0));
    vtree_init(&pDrdp->m_routes.tree, route_get_vnode_offset(0), (vcmp_t)uns32_cmp_wrap,
               route_get_key_offset(0));
    vtree_init(&pDrdp->m_routesById, route_get_vnode_offset(1), (vcmp_t)uns64_cmp,
               route_get_key_offset(1));
    vtree_init(&pDrdp->m_leaders, leader_get_vnode_offset(0), (vcmp_t)uns64_cmp,
               leader_get_key_offset(0));
    vlist_init(&pDrdp->m_arrivals, message_get_vlink_offset(1), 0, 0);
    vtree_init(&pDrdp->m_connections, connection_get_vnode_offset(1), (vcmp_t)uns64_cmp,
               connection_get_key_offset(0));

    for (i = 0; i < 2; i++)
        pDrdp->m_socket[i] = 0;

    drdp_addr_to_ipaddress(pAddress, &pDrdp->m_bindAddress);

    iResult = usocket_create(&pDrdp->m_socket[pDrdp->m_iNumSocket], USOCKET_TYPE_UDP,
                             &pDrdp->m_bindAddress, 0);
    if (iResult)
        goto done;
    pDrdp->m_iNumSocket++;

    memcpy(address.m_ucIp, pAddress->m_ucIp, 4);
    address.m_usPort = 0;
    if (usocket_create(&pDrdp->m_socket[pDrdp->m_iNumSocket], USOCKET_TYPE_ICMP,
                       &address, 0) == 0)
    {
        pDrdp->m_iNumSocket++;
    }

    usPort = (uns16)uiTime;
    if (usPort == DRDP_PORT_NONE)
        usPort = 0;
    if (usPort == DRDP_PORT_LEADER)
        usPort = 1;
    if (usPort == DRDP_PORT_UNREACHABLE)
        usPort = 2;

    iResult = endpoint_create(&pDrdp->m_pSystemEndpoint, pDrdp,
                              usPort | 0xffffffffffff0000);
    if (iResult)
        goto done;
    vtree_insert(&pDrdp->m_endpoints, pDrdp->m_pSystemEndpoint);

    if (iRole == DRDP_ROLE_MASTER)
    {
        pDrdp->m_endpointBlock[0].next      = 0;
        pDrdp->m_endpointBlock[0].remaining = 0;
        pDrdp->m_endpointBlock[1].next      = DRDP_SW_ENDPOINT_MIN;
        pDrdp->m_endpointBlock[1].remaining = DRDP_SW_ENDPOINT_MAX - DRDP_SW_ENDPOINT_MIN + 1;
        drdp_assign_endpoint_block_master(pDrdp);
    }

    *ppDrdp = pDrdp;
    pDrdp = 0;

done:
    if (pDrdp)
        drdp_destroy(pDrdp);
    return iResult;
}

// A client: no app payload, no callbacks, and the smallest of the three
// bandwidth budgets.
int drdp_create_client(drdp_t **ppDrdp, const drdp_addr_t *pAddress, uns32 uiTime,
                       bool bNoSend, drdp::logger_t *pLogger)
{
    return drdp_create_addr(ppDrdp, pAddress, DRDP_ROLE_CLIENT, uiTime, bNoSend, pLogger,
                            0, 0, 0, 0, 3000, 300);
}

// A master: it keeps a copy of the application's handshake payload so it can
// compare a leader's against it.
int drdp_create_master(drdp_t **ppDrdp, const drdp_addr_t *pAddress, uns32 uiTime,
                       bool bNoSend, drdp::logger_t *pLogger, void *pAppPtr,
                       drdp_leader_callback_t pfnLeaderAdd,
                       drdp_leader_callback_t pfnLeaderRemove,
                       drdp_endpoint_callback_t pfnEndpointDestroyed,
                       const void *pData, int iSize)
{
    drdp_t *pDrdp = 0;
    int iResult;

    iResult = drdp_create_addr(&pDrdp, pAddress, DRDP_ROLE_MASTER, uiTime, bNoSend, pLogger,
                               pAppPtr, pfnLeaderAdd, pfnLeaderRemove, pfnEndpointDestroyed,
                               2097152, 65000);
    if (iResult)
        goto done;

    if (iSize > 0)
    {
        pDrdp->m_pData = malloc(iSize);
        if (!pDrdp->m_pData)
        {
            iResult = -1;
            goto done;
        }
        pDrdp->m_iDataSize = iSize;
        memcpy(pDrdp->m_pData, pData, iSize);
    }

    iResult = 0;
    *ppDrdp = pDrdp;
    pDrdp = 0;

done:
    if (pDrdp)
        drdp_destroy(pDrdp);
    return iResult;
}

// A leader: the same, plus the connection to the master and the hello that
// carries the payload the master will check.
int drdp_create_leader(drdp_t **ppDrdp, const drdp_addr_t *pAddress, const drdp_addr_t *pMaster,
                       uns32 uiTime, bool bNoSend, drdp::logger_t *pLogger, void *pAppPtr,
                       drdp_leader_callback_t pfnLeaderAdd,
                       drdp_leader_callback_t pfnLeaderRemove,
                       drdp_endpoint_callback_t pfnEndpointDestroyed,
                       const void *pData, int iSize)
{
    drdp_t *pDrdp = 0;
    uns64 uiMaster;
    buffer_t buffer;
    int iResult;

    if (!pMaster->m_ucIp[0])
    {
        iResult = -8;
        goto done;
    }

    iResult = drdp_create_addr(&pDrdp, pAddress, DRDP_ROLE_LEADER, uiTime, bNoSend, pLogger,
                               pAppPtr, pfnLeaderAdd, pfnLeaderRemove, pfnEndpointDestroyed,
                               2097152, 65000);
    if (iResult)
        goto done;

    drdp_address_to_endpoint(pMaster->m_ucIp, pMaster->m_usPort, DRDP_PORT_LEADER, &uiMaster);
    iResult = connection_open(&pDrdp->m_pMasterConnection, pDrdp->m_pSystemEndpoint, uiMaster);
    if (iResult)
        goto done;
    connection_set_bps_limit(pDrdp->m_pMasterConnection, 2097152);

    {
        char *pHello = (char *)__builtin_alloca(iSize + 1);

        buffer_init(&buffer, pHello, iSize + 1, BUFFER_LITTLE_ENDIAN);
        buffer_append_uns8(&buffer, 7);
        buffer_appendm_uns8(&buffer, (const uns8 *)pData, iSize);
        connection_send(pDrdp->m_pMasterConnection, buffer.data, buffer.position, 249);
    }

    *ppDrdp = pDrdp;
    pDrdp = 0;

done:
    if (pDrdp)
        drdp_destroy(pDrdp);
    return iResult;
}





















































































































































// Hand out ONE software endpoint id.  The two blocks are drained smallest
// first, and when the one in use runs dry the master takes another out of the
// pool it owns while a leader asks the master for one.
int drdp_endpoint_create(drdp_t *pDrdp, endpoint_t **ppEndpoint)
{
    endpoint_t *pEndpoint = 0;
    int iResult;
    int i;
    VASSERT( pDrdp->m_endpointBlock[0].remaining > 0 || pDrdp->m_endpointBlock[1].remaining > 0 );   // 613
    if (pDrdp->m_endpointBlock[0].remaining == 0
        && pDrdp->m_endpointBlock[1].remaining == 0)
    {
        iResult = -1;
        goto done;
    }
    if (pDrdp->m_endpointBlock[1].remaining == 0
        || (pDrdp->m_endpointBlock[0].remaining != 0 && pDrdp->m_endpointBlock[1].remaining >= pDrdp->m_endpointBlock[0].remaining))
        i = 0;
    else
        i = 1;
    iResult = endpoint_create(&pEndpoint, pDrdp, pDrdp->m_endpointBlock[i].next);
    if (iResult)
        goto done;
    vtree_insert(&pDrdp->m_endpoints, pEndpoint);
    pDrdp->m_endpointBlock[i].next++;
    pDrdp->m_endpointBlock[i].remaining--;
    if (pDrdp->m_endpointBlock[i].remaining == 0)
    {
        if (pDrdp->m_iRole == DRDP_ROLE_MASTER)
        {
            drdp_assign_endpoint_block_master(pDrdp);
        }
        else
        {




















            VASSERT( pDrdp->m_iRole == DRDP_ROLE_LEADER );   // 659
            connection_send_uns8(pDrdp->m_pMasterConnection, 32);
        }
    }
    *ppEndpoint = pEndpoint;
    pEndpoint = 0;
    iResult = 0;
done:
    if (pEndpoint)
        endpoint_destroy(pEndpoint);
    return iResult;
}

int drdp_is_valid(const drdp_t *pDrdp)
{
    return memcmp(pDrdp->m_szTag, DRDP_TAG, 4) == 0;
}

// Tearing an endpoint down is a broadcast: the route table forgets it and
// every leader is told, so nothing keeps routing to a dead id.
void drdp_endpoint_destroy(endpoint_t *pEndpoint)
{
    drdp_t *pDrdp = endpoint_get_drdp(pEndpoint);
    leader_t *pLeader;

    route_table_update(&pDrdp->m_routeTable, endpoint_get_id(pEndpoint), (uns64)-1);

    if (pDrdp->m_pfnEndpointDestroyed)
        pDrdp->m_pfnEndpointDestroyed(pDrdp, endpoint_get_id(pEndpoint));

    if (pDrdp->m_iRole == DRDP_ROLE_MASTER)
    {
        for (pLeader = (leader_t *)vtree_peek_first(&pDrdp->m_leaders); pLeader;
             pLeader = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pLeader))
        {
            connection_send_uns8_uns64_uns64(pLeader->m_pConnection, 19,
                                             endpoint_get_id(pEndpoint), (uns64)-1);
        }
    }
    else
    {
        connection_send_uns8_uns64(pDrdp->m_pMasterConnection, 21,
                                   endpoint_get_id(pEndpoint));
    }

    vtree_remove(&pDrdp->m_endpoints, pEndpoint);
    endpoint_destroy(pEndpoint);
}

void drdp_route_destroy(route_t *pRoute)
{
    drdp_t *pDrdp = route_get_drdp(pRoute);

    vtree_remove(&pDrdp->m_routesById, pRoute);
    vtree_remove(&pDrdp->m_routes.tree, pRoute);
    route_destroy(pRoute);
}



void drdp_destroy(drdp_t *pDrdp)
{
    route_t *pRoute;
    int i;
























    VASSERT( drdp_is_valid( pDrdp ) );   // 747

    memcpy(pDrdp->m_szTag, NULL_TAG, 4);

    for (i = 0; i < 2; i++)
    {
        if (pDrdp->m_socket[i])
        {
            usocket_destroy(pDrdp->m_socket[i]);
            pDrdp->m_socket[i] = 0;
        }
    }

    if (pDrdp->m_pSystemEndpoint)
    {
        vtree_remove(&pDrdp->m_endpoints, pDrdp->m_pSystemEndpoint);
        endpoint_destroy(pDrdp->m_pSystemEndpoint);
        pDrdp->m_pSystemEndpoint = 0;
    }

    route_table_destroy(&pDrdp->m_routeTable);
    route_cache_destroy(&pDrdp->m_routeCache);

    while ((pRoute = (route_t *)vtree_peek_first(&pDrdp->m_routes.tree)) != 0)
        drdp_route_destroy(pRoute);

    if (pDrdp->m_pData)
        free(pDrdp->m_pData);

    free(pDrdp);
}

int drdp_namelookup(const char *pName, drdp_addr_t *pAddress)
{
    ipaddress_t address;
    int iResult;

    iResult = unet_namelookup(pName, &address);

    if (!iResult)
        ipaddress_to_drdp_addr(&address, pAddress);

    return iResult;
}

int drdp_gethostname(char *pName, int iSize)
{
    return unet_gethostname(pName, iSize);
}

// Every packet that leaves this process goes out here: the crc of the whole
// buffer, XOR'd with the table entry for the protocol version, is appended and
// the bandwidth meter is charged the wire size -- 28 bytes of IP and UDP
// header on top of the payload.
void drdp_crc_send(drdp_t *pDrdp, const drdp_addr_t *pAddress, buffer_t *pBuffer,
                   int iDrop, int iDelay, int iJitter)
{
    ipaddress_t address;
    uns32 uiCrc;

    uiCrc = drdp_crc_table_get(2);
    buffer_append_uns32(pBuffer, uiCrc ^ drdp_crc(0, (const char *)pBuffer->data,
                                                  pBuffer->position));
    drdp_addr_to_ipaddress(pAddress, &address);

    pDrdp->m_iBytesSent += pBuffer->position;

    if (!pDrdp->m_bNoSend)
    {
        usocket_sendto_addr(pDrdp->m_socket[0], &address, pBuffer->data, pBuffer->position,
                            iDrop, iDelay, iJitter);
    }

    bandwidth_send(&pDrdp->m_bandwidth, pDrdp->m_uiTime, pBuffer->position + 28);
}

// An address is not a route: one machine can carry many drdp ports, so a
// terminate aimed at an address hits every route whose id is that address with
// any port at all -- which is a RANGE lookup between the two extreme ports.
void drdp_route_terminate(drdp_t *pDrdp, drdp_addr_t *pAddress, uns8 ucReason)
{
    uns64 uiLo;
    uns64 uiHi;
    route_t *pRoute;

    drdp_address_to_endpoint(pAddress->m_ucIp, pAddress->m_usPort, 0, &uiLo);
    drdp_address_to_endpoint(pAddress->m_ucIp, pAddress->m_usPort, DRDP_PORT_NONE, &uiHi);

    for (pRoute = (route_t *)vtree_peek_first_in_range(&pDrdp->m_routesById, &uiLo, &uiHi);
         pRoute;
         pRoute = (route_t *)vtree_get_next_in_range(&pDrdp->m_routesById, &uiLo, &uiHi, pRoute))
    {
        route_terminate(pRoute, ucReason);
    }
}

// The segment header: one packed word carrying the payload size in eleven bits
// and a bit for every field that follows.  A segment between two known
// endpoints on a known route is that word and nothing else.
void buffer_read_segment_header(buffer_t *pBuffer, segment_header_t *pHeader, uns64 uiRoute,
                                uns64 uiDst)
{
    buffer_read_packed_uns32(pBuffer, &pHeader->m_uiFlags);
    pHeader->m_uiSize = pHeader->m_uiFlags & 0x7ff;
    pHeader->m_uiFlags = pHeader->m_uiFlags & 0xfffff800;

    if (pHeader->m_uiFlags & 0x2000)
        buffer_read_uns32(pBuffer, &pHeader->m_uiCrc);
    if (pHeader->m_uiFlags & 0x40000)
        buffer_read_uns32(pBuffer, &pHeader->m_uiFlushNumber);

    if (pHeader->m_uiFlags & 0x8000)
        buffer_read_uns64(pBuffer, &pHeader->m_uiRoute);
    else
        pHeader->m_uiRoute = uiRoute;

    if (pHeader->m_uiFlags & 0x1000)
        pHeader->m_uiSrc = pHeader->m_uiRoute;
    else
        buffer_read_packed_uns64(pBuffer, &pHeader->m_uiSrc);

    if (pHeader->m_uiFlags & 0x800)
        buffer_read_packed_uns64(pBuffer, &pHeader->m_uiDst);
    else
        pHeader->m_uiDst = uiDst;
}

void buffer_append_segment_header(buffer_t *pBuffer, segment_header_t *pHeader)
{
    if (drdp_is_local_hardware_endpoint(pHeader->m_uiSrc))
        pHeader->m_uiFlags |= 0x1000;
    if (drdp_is_software_endpoint(pHeader->m_uiDst))
        pHeader->m_uiFlags |= 0x800;

    buffer_append_packed_uns32(pBuffer, pHeader->m_uiFlags | pHeader->m_uiSize);

    if (pHeader->m_uiFlags & 0x2000)
        buffer_append_uns32(pBuffer, pHeader->m_uiCrc);
    if (pHeader->m_uiFlags & 0x40000)
        buffer_append_uns32(pBuffer, pHeader->m_uiFlushNumber);
    if (pHeader->m_uiFlags & 0x8000)
        buffer_append_uns64(pBuffer, pHeader->m_uiRoute);
    if (~pHeader->m_uiFlags & 0x1000)
        buffer_append_packed_uns64(pBuffer, pHeader->m_uiSrc);
    if (pHeader->m_uiFlags & 0x800)
        buffer_append_packed_uns64(pBuffer, pHeader->m_uiDst);
}

// An endpoint has moved: every connection whose far id is that endpoint is
// re-linked onto the route it lives behind now.
void drdp_re_route_connections_to_endpoint(drdp_t *pDrdp, uns64 uiId, uns64 uiRoute)
{
    connection_t *pConnection;
    route_t *pRoute = 0;
    int iResult;

    pConnection = (connection_t *)vtree_peek_first_in_range(&pDrdp->m_connections,
                                                            &uiId, &uiId);
    if (!pConnection)
        return;

    iResult = drdp_get_route(pDrdp, uiRoute, &pRoute);



























































































































    VASSERT( iResult == 0 );   // 1032

    while (pConnection)
    {
        if (connection_get_route(pConnection) != pRoute)
            route_link_connection(pRoute, pConnection);
        pConnection = (connection_t *)vtree_get_next_in_range(&pDrdp->m_connections,
                                                              &uiId, &uiId, pConnection);
    }
}

// The far end has gone: only a connection with a HARDWARE end is torn down,
// because a software-to-software connection is re-routed instead.
void drdp_terminate_connections_to_endpoint(drdp_t *pDrdp, uns64 uiId)
{
    connection_t *pConnection;

    drdp_format_endpoint1(uiId);

    for (pConnection = (connection_t *)vtree_peek_first_in_range(&pDrdp->m_connections,
                                                                 &uiId, &uiId);
         pConnection;
         pConnection = (connection_t *)vtree_get_next_in_range(&pDrdp->m_connections,
                                                               &uiId, &uiId, pConnection))
    {
        if (drdp_is_hardware_endpoint(connection_get_src(pConnection))
            || drdp_is_hardware_endpoint(connection_get_dst(pConnection)))
        {
            connection_terminate_reason(pConnection, 0, 5);
        }
    }
}

// A segment for an endpoint this process does not own.  If the id is behind a
// known route it is forwarded; if it is not, a reset goes back -- but only to
// a hardware peer, because a leader is expected to be out of date for a while.
void drdp_handle_segment_endpoint_not_local(drdp_t *pDrdp, segment_header_t *pHeader,
                                            buffer_t *pBuffer)
{
    route_t *pRoute;
    uns64 uiRoute;

    if (drdp_is_local_hardware_endpoint(pHeader->m_uiDst))
    {

































        VASSERT( !"not tested, shouldn't happen unless I start using drdp ports again" );   // 1109
        drdp_send_reset_endpoint(pDrdp, pHeader);
        return;
    }

    VASSERT( drdp_is_software_endpoint( pHeader->m_uiDst ) );   // 1114

    uiRoute = drdp_get_route_endpoint(pDrdp, pHeader->m_uiDst);
    if (uiRoute == (uns64)-1)
    {
        buffer_skip(pBuffer, pHeader->m_uiSize);
        if (drdp_is_hardware_endpoint(pHeader->m_uiSrc)
            && !drdp_is_leader_endpoint(pDrdp, pHeader->m_uiSrc))
        {
            drdp_send_reset_endpoint(pDrdp, pHeader);
        }
        return;
    }

    if (drdp_get_route(pDrdp, uiRoute, &pRoute))
    {
        buffer_skip(pBuffer, pHeader->m_uiSize);
        return;
    }
    buffer_skip(pBuffer, pHeader->m_uiSize);
}

// One segment out of a packet: the header, then the endpoint it is for.
int drdp_handle_segment(drdp_t *pDrdp, buffer_t *pBuffer, uns64 uiRoute)
{
    segment_header_t header;
    endpoint_t *pEndpoint;
    int iResult;
    buffer_read_segment_header(pBuffer, &header, uiRoute,
                               endpoint_get_id(pDrdp->m_pSystemEndpoint));
    if (header.m_uiFlags & 0x20000)
    {
        drdp_format_endpoint1(header.m_uiSrc);
        drdp_terminate_connections_to_endpoint(pDrdp, header.m_uiSrc);
        goto done;
    }
    pEndpoint = (endpoint_t *)vtree_lookup(&pDrdp->m_endpoints, &header.m_uiDst);
    if (!pEndpoint)
    {
        drdp_handle_segment_endpoint_not_local(pDrdp, &header, pBuffer);
        goto done;
    }
    if (header.m_uiFlags & 0x8000)
    {
        drdp_format_endpoint1(header.m_uiSrc);
        drdp_format_endpoint2(header.m_uiDst);
        drdp_format_endpoint3(uiRoute);
        drdp_format_endpoint4(header.m_uiRoute);
    }
    iResult = endpoint_handle_segment(pEndpoint, &header, pBuffer);
    if (iResult)
        goto out;
done:
    iResult = 0;
out:
    return iResult;
}

// The two answers to a packet that arrived for a drdp port nobody is
// listening on.  Both are two bytes and a crc, which is what makes the
// "nothing left after the crc" case in `drdp_process_udp` a terminate.
void drdp_send_drdp_port_unreachable(drdp_t *pDrdp, const drdp_addr_t *pAddress, uns16 usPort)
{
    buffer_t buffer;
    uns8 data[32];
    drdp_addr_t route;

    drdp_remap_address_to_route(pDrdp, pAddress, &route);
    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    buffer_append_uns16(&buffer, usPort);
    drdp_crc_send(pDrdp, &route, &buffer, 0, 0, 0);
}

void drdp_send_drdp_proto_unreachable(drdp_t *pDrdp, const drdp_addr_t *pAddress)
{
    buffer_t buffer;
    uns8 data[32];
    drdp_addr_t route;

    drdp_remap_address_to_route(pDrdp, pAddress, &route);
    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    buffer_append_uns16(&buffer, DRDP_PORT_UNREACHABLE);
    drdp_crc_send(pDrdp, &route, &buffer, 0, 0, 0);
}

// A packet whose drdp port is the wildcard belongs to no connection at all:
// it becomes one type-248 message on the arrival list, addressed from the
// sender's machine and to nobody.
int drdp_process_connectionless(drdp_t *pDrdp, drdp_addr_t *pAddress, buffer_t *pBuffer)
{
    message_t *pMessage;
    uns64 uiSrc;
    int iResult;

    drdp_address_to_endpoint(pAddress->m_ucIp, pAddress->m_usPort, DRDP_PORT_NONE, &uiSrc);

    iResult = message_create(&pMessage, 0, 0, uiSrc, (uns64)-1,
                             buffer_get_position_ptr(pBuffer),
                             buffer_bytes_remaining(pBuffer),
                             248, 0, drdp_get_time(pDrdp));
    if (iResult)
        return iResult;

    drdp_arrival_enqueue(pDrdp, pMessage);
    return 0;
}

// The crc is the LAST four bytes, and the sender xors the crc table entry for
// its protocol version into it -- so recovering the version is a lookup of the
// difference, searched outwards from where version 2 sits.
int drdp_consume_crc_get_protocol_version(buffer_t *pBuffer)
{
    uns32 uiCrc;
    int iPosition;

    if (pBuffer->size < 4)
        return -1;

    iPosition = pBuffer->position;
    pBuffer->position = pBuffer->size - 4;
    buffer_read_uns32(pBuffer, &uiCrc);
    pBuffer->position = iPosition;
    pBuffer->size -= 4;

    return drdp_crc_table_lookup(uiCrc ^ drdp_crc(0, (const char *)pBuffer->data,
                                                 pBuffer->size), 2);
}

// One UDP datagram.  The socket layer wrote the sender's address into the
// first six bytes, so the buffer is opened over the whole thing and then
// rebased past them; what follows is the destination drdp port, the segments,
// and the crc at the tail.
//
// The segments are walked TWICE: once to check every header parses and names
// two real endpoints, and only then again to hand each to its endpoint.  A
// packet that fails the first walk is reported to every connection on the
// route rather than half-applied.
int drdp_process_udp(drdp_t *pDrdp, const void *pData, int iSize)
{
    buffer_t buffer;
    drdp_addr_t address;
    uns16 usDrdpPort;
    uns16 usPort;
    uns64 uiRoute;
    route_t *pRoute;
    connection_t *pConnection;
    int iVersion;
    int iPosition;
    int iResult;

    bandwidth_recv(&pDrdp->m_bandwidth, pDrdp->m_uiTime, iSize + 22);

    buffer_init_read(&buffer, pData, iSize, BUFFER_LITTLE_ENDIAN);
    buffer_readm_uns8(&buffer, address.m_ucIp, 4);
    buffer_read_uns16(&buffer, &address.m_usPort);

    buffer.data = buffer.data + buffer.position;
    buffer.size = buffer.size - buffer.position;
    buffer.position = 0;

    buffer_read_uns16(&buffer, &usDrdpPort);

    if (usDrdpPort == DRDP_PORT_NONE)
    {
        drdp_addr_t connectionless;

        memcpy(&connectionless, &address, 4);
        connectionless.m_usPort = address.m_usPort;

        return drdp_process_connectionless(pDrdp, &connectionless, &buffer);
    }

    drdp_remap_route_to_address(pDrdp, &address, &address);
    drdp_address_to_endpoint(address.m_ucIp, address.m_usPort, usDrdpPort, &uiRoute);

    iVersion = drdp_consume_crc_get_protocol_version(&buffer);

    if (usDrdpPort == DRDP_PORT_UNREACHABLE)
    {
        if (iVersion == -1)
            return 0;
        if (iVersion == 2)
            return 0;
        drdp_route_terminate(pDrdp, &address, 14);
        return 0;
    }

    if (iVersion == -1)
    {
        pRoute = (route_t *)vtree_lookup(&pDrdp->m_routesById, &uiRoute);
        if (!pRoute)
            return -1;
        for (pConnection = route_get_first_connection(pRoute); pConnection;
             pConnection = route_get_next_connection(pRoute, pConnection))
        {
            connection_handle_bad_crc(pConnection);
        }
        return -1;
    }

    if (iVersion != 2)
    {
        drdp_send_drdp_proto_unreachable(pDrdp, &address);
        return -1;
    }

    if (buffer_bytes_remaining(&buffer) == 0)
    {
        route_t *pTerminated = (route_t *)vtree_lookup(&pDrdp->m_routesById, &uiRoute);

        if (pTerminated)
            route_terminate(pTerminated, 15);
        return 0;
    }

    buffer_read_uns16(&buffer, &usPort);
    if (usPort == DRDP_PORT_LEADER)
        usPort = drdp_get_port_drdp(pDrdp);
    if (usPort != drdp_get_port_drdp(pDrdp))
    {
        drdp_send_drdp_port_unreachable(pDrdp, &address, usPort);
        return -1;
    }

    iPosition = buffer.position;

    while (buffer_bytes_remaining(&buffer))
    {
        segment_header_t header;

        buffer_read_segment_header(&buffer, &header, uiRoute,
                                   endpoint_get_id(pDrdp->m_pSystemEndpoint));
        buffer_skip(&buffer, header.m_uiSize);

        if (!buffer.error && drdp_is_endpoint(header.m_uiDst)
            && drdp_is_endpoint(header.m_uiSrc))
        {
            continue;
        }

        pRoute = (route_t *)vtree_lookup(&pDrdp->m_routesById, &uiRoute);
        if (!pRoute)
            return -1;
        for (pConnection = route_get_first_connection(pRoute); pConnection;
             pConnection = route_get_next_connection(pRoute, pConnection))
        {
            connection_log_parse_failure(pConnection, "basic packet structure is bad");
            connection_handle_corrupt(pConnection);
        }
        return -1;
    }

    drdp_format_endpoint1(uiRoute);
    drdp_get_time(pDrdp);

    buffer.position = iPosition;

    while (buffer_bytes_remaining(&buffer))
    {
        iResult = drdp_handle_segment(pDrdp, &buffer, uiRoute);
        if (iResult)
            return iResult;
    }

    {
        route_t *pArrived = (route_t *)vtree_lookup(&pDrdp->m_routesById, &uiRoute);

        if (pArrived)
            route_record_arrival(pArrived, iSize - 6);
    }
    return 0;
}




// The one entry point a datagram arrives through.  A port of zero in the
// six-byte prefix is the ICMP socket, not the UDP one.
int drdp_process_packet(drdp_t *pDrdp, const void *pData, int iSize)
{
    buffer_t buffer;
    uns16 usPort;





































































































































    VASSERT( drdp_is_valid( pDrdp ) );   // 1529

    buffer_init_read(&buffer, pData, iSize, BUFFER_LITTLE_ENDIAN);
    buffer_skip(&buffer, 4);
    buffer_read_uns16(&buffer, &usPort);

    if (usPort)
        return drdp_process_udp(pDrdp, pData, iSize);

    return drdp_process_icmp(pDrdp, pData, iSize);
}


























































// Read ONE datagram off whichever socket has one.  The starting socket
// rotates, so a busy UDP socket cannot starve the ICMP one.
int drdp_receive_packet(drdp_t *pDrdp, void *pData, int iSize, int *piRead, int iTimeout)
{
    int iResult;
    int i;
    VASSERT( drdp_is_valid( pDrdp ) );   // 1604
    pDrdp->m_uiTimeRecv = pDrdp->m_uiTime;
    if (iTimeout)
    {
        iResult = usocket_select(&pDrdp->m_socket[0], pDrdp->m_iNumSocket, iTimeout);
        if (iResult)
            goto out;
    }
    for (i = 0; i < pDrdp->m_iNumSocket; i++)
    {
        do
        {
            iResult = usocket_recvfrom(pDrdp->m_socket[pDrdp->m_iSocket], pData, iSize, piRead);
        }
        while (iResult == -6);
        if (!iResult)
        {
            pDrdp->m_iBytesRecv += *piRead;
            break;
        }
        pDrdp->m_iSocket++;
        pDrdp->m_iSocket %= pDrdp->m_iNumSocket;
    }
    iResult = -1;
    if (i != pDrdp->m_iNumSocket)
        iResult = 0;
out:
    return iResult;
}

// The clock the whole library reads.  The three byte counters are the bps
// meters, and they reset once a second.
void drdp_set_time(drdp_t *pDrdp, uns32 uiTime)
{
    if ((int)(uiTime - pDrdp->m_uiTimeBps) > 1000)
    {
        pDrdp->m_iBytesSent = 0;
        pDrdp->m_iBytesRecv = 0;
        pDrdp->m_iBytesDropped = 0;
        pDrdp->m_uiTimeBps = uiTime;
    }
    pDrdp->m_uiTime = uiTime;
}

uns32 drdp_get_time(const drdp_t *pDrdp)
{
    return pDrdp->m_uiTime;
}

void routes_by_desired_flush_time_resort(routes_by_desired_flush_time_t *pTree, route_t *pRoute)
{
    vtree_remove(&pTree->tree, pRoute);
    route_recalc_desired_flush_time(pRoute);
    vtree_insert(&pTree->tree, pRoute);
}

// Send whatever is due.  The route that wants to go out soonest is at the
// front of the tree, so the loop stops the moment the head is not due yet --
// and the bandwidth meter can stop it sooner than that.
void drdp_flush(drdp_t *pDrdp)
{
    route_t *pRoute;

    route_cache_release_expired(&pDrdp->m_routeCache, pDrdp->m_uiTime);

    while ((int)(bandwidth_get_flush_time(&pDrdp->m_bandwidth, pDrdp->m_uiTime)
                 - pDrdp->m_uiTime) <= 0)
    {
        pRoute = (route_t *)vtree_peek_first(&pDrdp->m_routes.tree);
        if (!pRoute)
            return;

        if (pDrdp->m_uiTime < route_get_desired_flush_time(pRoute))
            goto done;

        route_flush(pRoute);

        if (route_destroy_check(pRoute))
            drdp_route_destroy(pRoute);
        else
            routes_by_desired_flush_time_resort(&pDrdp->m_routes, pRoute);
    }

    pRoute = (route_t *)vtree_peek_first(&pDrdp->m_routes.tree);
    if (pRoute)
        route_get_first_connection(pRoute);

done:
    pDrdp->m_uiTimeFlush = pDrdp->m_uiTime;
}

// 1 unknown, 0 dead, 2 live.
int drdp_get_sw_endpoint_status(drdp_t *pDrdp, uns64 uiId)
{
    uns64 uiRoute;

    if (route_table_find(&pDrdp->m_routeTable, uiId, &uiRoute) == -1)
        return 1;
    if (uiRoute == (uns64)-1)
        return 0;
    return 2;
}






// Which machine an endpoint id lives behind.  A hardware id IS its own route;
// a software id is looked up, and a leader that does not know falls back on
// the master.
uns64 drdp_get_route_endpoint(drdp_t *pDrdp, uns64 uiId)
{
    uns64 uiRoute = (uns64)-1;

    if (drdp_is_hardware_endpoint(uiId))
    {
        uiRoute = uiId;
    }
    else
    {


































































        VASSERT( pDrdp->m_iRole != DRDP_ROLE_CLIENT );   // 1791

        if (route_table_find(&pDrdp->m_routeTable, uiId, &uiRoute) == -1)
        {
            if (pDrdp->m_iRole == DRDP_ROLE_MASTER)
                uiRoute = (uns64)-1;
            else
                uiRoute = connection_get_dst(pDrdp->m_pMasterConnection);
        }
    }

    if (endpoint_get_id(pDrdp->m_pSystemEndpoint) == uiRoute)
        uiRoute = connection_get_dst(pDrdp->m_pMasterConnection);

    return uiRoute;
}

int drdp_endpoint_is_local(const drdp_t *pDrdp, uns64 uiId)
{
    return vtree_lookup(&pDrdp->m_endpoints, &uiId) != 0;
}

endpoint_t *drdp_find_endpoint(drdp_t *pDrdp, uns64 uiId)
{
    return (endpoint_t *)vtree_lookup(&pDrdp->m_endpoints, &uiId);
}

void drdp_arrival_enqueue(drdp_t *pDrdp, message_t *pMessage)
{
    vlist_add_tail(&pDrdp->m_arrivals, pMessage);
}

void drdp_arrival_remove(drdp_t *pDrdp, message_t *pMessage)
{
    vlist_remove(&pDrdp->m_arrivals, pMessage);
}

// A leader's connection came or went.  The application is told either way, and
// the record keeps whichever it is now -- including none.
void drdp_leader_set_connection(drdp_t *pDrdp, uns64 uiId, connection_t *pConnection)
{
    leader_t *pLeader = (leader_t *)vtree_lookup(&pDrdp->m_leaders, &uiId);

    if (pLeader)
    {
        if (pConnection == 0)
            pDrdp->m_pfnLeaderRemove(pDrdp, pLeader->m_pConnection);
        else
            pDrdp->m_pfnLeaderAdd(pDrdp, pConnection);
        pLeader->m_pConnection = pConnection;
    }
}

void drdp_leader_remove_internal(drdp_t *pDrdp, uns64 uiId)
{
    leader_t *pLeader = (leader_t *)vtree_remove_by_key(&pDrdp->m_leaders, &uiId);

    if (pLeader->m_pConnection)
        pDrdp->m_pfnLeaderRemove(pDrdp, pLeader->m_pConnection);

    free(pLeader);
}

// Destroying a connection unlinks it from the tree being walked, so the walk
// has to hold the next one before the body runs.
void drdp_close_connections_to_endpoint(drdp_t *pDrdp, uns64 uiId)
{
    connection_t *pConnection;
    connection_t *pNext;

    drdp_format_endpoint1(uiId);

    pConnection = (connection_t *)vtree_peek_first_in_range(&pDrdp->m_connections,
                                                            &uiId, &uiId);
    while (pConnection)
    {
        pNext = (connection_t *)vtree_get_next_in_range(&pDrdp->m_connections,
                                                        &uiId, &uiId, pConnection);





















































































        _endpoint_connection_destroy(pConnection, __FILE__, __LINE__);   // 1954
        pConnection = pNext;
    }
}

// A local endpoint was re-created: every connection that already pointed at
// its id is handed to the endpoint that owns it now.
void drdp_reconcile_local_endpoints(drdp_t *pDrdp, endpoint_t *pEndpoint)
{
    connection_t *pConnection;
    uns64 uiId;

    uiId = endpoint_get_id(pEndpoint);

    while ((pConnection = (connection_t *)vtree_peek_first_in_range(&pDrdp->m_connections,
                                                                    &uiId, &uiId)) != 0)
    {
        endpoint_reconcile_connections(connection_get_endpoint(pConnection), pConnection,
                                       pEndpoint);
    }
}

// A whole block of software endpoint ids has gone away.  Every connection into
// the range dies, and the application hears about every id in it -- one call
// per id, so a block of 2,048 is 2,048 callbacks.
void drdp_invalidate_range(drdp_t *pDrdp, uns64_range_t *pRange)
{
    connection_t *pConnection;
    connection_t *pNext;
    uns64 uiId;

    pConnection = (connection_t *)vtree_peek_first_in_range(&pDrdp->m_connections,
                                                            &pRange->lo, &pRange->hi);
    while (pConnection)
    {
        connection_get_dst(pConnection);
        pNext = (connection_t *)vtree_get_next_in_range(&pDrdp->m_connections,
                                                        &pRange->lo, &pRange->hi, pConnection);










































        _endpoint_connection_destroy(pConnection, __FILE__, __LINE__);   // 2034
        pConnection = pNext;
    }

    if (pDrdp->m_pfnEndpointDestroyed)
    {
        for (uiId = pRange->lo; uiId <= pRange->hi; uiId++)
            pDrdp->m_pfnEndpointDestroyed(pDrdp, uiId);
    }
}

void drdp_invalidate_all_endpoints_owned_by_leader(drdp_t *pDrdp, uns64 uiId)
{
    route_table_entry_t *pEntry;
    route_table_entry_t *pNext;

    pEntry = (route_table_entry_t *)vtree_peek_first(&pDrdp->m_routeTable.m_tree);
    while (pEntry)
    {
        pNext = (route_table_entry_t *)vtree_get_next(&pDrdp->m_routeTable.m_tree, pEntry);

        if (pEntry->m_uiRoute == uiId)
        {
            drdp_invalidate_range(pDrdp, &pEntry->m_range);
            vtree_remove(&pDrdp->m_routeTable.m_tree, pEntry);
            free(pEntry);
        }

        pEntry = pNext;
    }
}

// The leader array is sorted, so which leader owns a route is a binary
// search -- and the debug build checks the answer: a hit re-tests the element
// and a miss walks the whole array asserting the key really is not there.
//
// It is INLINE and it lives somewhere else.  Its two asserts point at a string
// no other assert in the object uses and quote lines 2157 and 2165, which are
// nowhere near this file's -- so it is an inline function in an include, and
// the two line numbers are constants of that header rather than of drdp.cpp.
// Which header is not recovered; the path below is a placeholder, and only the
// two immediates are load-bearing.
//
// The key is a CONST REFERENCE, not a value: both inlined copies re-read it out
// of memory on every iteration, which is also why the first caller has to
// materialise a stack temporary for `endpoint_get_id`'s result to bind to.
#define UTIL_FILE  "C:/code-frontiers-gm/drdp/<inline>.h"
#define UASSERT_AT(x, line)  if (!(x)) _uassert(#x, UTIL_FILE, line)

static inline int find_leader_index(const uns64 *p, int n, const uns64 &key)
{
    int lo = 0;
    int hi = n;
    int check;
    int i;

    while (lo < hi)
    {
        check = (lo + hi) >> 1;
        if (key < p[check])
        {
            hi = check;
        }
        else if (p[check] < key)
        {
            lo = check + 1;
        }
        else
        {
            UASSERT_AT( p[check] == key, 2157 );
            return check;
        }
    }

    for (i = 0; i < n; i++)
        UASSERT_AT( p[i] != key, 2165 );

    return n;
}

// The whole route table, sent to a leader that has just joined.  Every range
// is delta-coded against the previous one and its owner is an INDEX into the
// leader array that precedes it, so an id that is 8 bytes on the wire costs a
// byte or two here.
void drdp_master_send_route_table(drdp_t *pDrdp, connection_t *pConnection)
{
    buffer_t buffer;
    char *pData;
    uns64 *p_leader_endpoints;
    route_table_entry_t *rte;
    uns64 uiLast;
    int iSize;
    int iLeaderCount;
    int leader_index;
    int i;

    iSize = vtree_get_size(&pDrdp->m_routeTable.m_tree) * 5 + 65536;
    pData = new char[iSize];
    buffer_init(&buffer, pData, iSize, BUFFER_LITTLE_ENDIAN);
    buffer_append_uns8(&buffer, 48);

    iLeaderCount = vtree_get_size(&pDrdp->m_leaders);
    p_leader_endpoints = new uns64[iLeaderCount];

    {
        leader_t *pLeader = (leader_t *)vtree_peek_first(&pDrdp->m_leaders);

        for (i = 0; i < iLeaderCount; i++)
        {
            p_leader_endpoints[i] = pLeader->m_uiId;
            pLeader = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pLeader);
        }
    }















































    VASSERT( iLeaderCount == find_leader_index( p_leader_endpoints, iLeaderCount, endpoint_get_id( pDrdp->m_pSystemEndpoint ) ) );   // 2194

    buffer_append_packed_int32(&buffer, iLeaderCount);
    buffer_appendm_uns64(&buffer, p_leader_endpoints, iLeaderCount);
    buffer_append_uns64(&buffer, pDrdp->m_routeTable.m_uiLo);
    buffer_append_uns64(&buffer, pDrdp->m_routeTable.m_uiHi);
    buffer_append_packed_uns64(&buffer, vtree_get_size(&pDrdp->m_routeTable.m_tree));

    uiLast = 0;

    for (rte = (route_table_entry_t *)vtree_peek_first(&pDrdp->m_routeTable.m_tree); rte;
         rte = (route_table_entry_t *)vtree_get_next(&pDrdp->m_routeTable.m_tree, rte))
    {
        buffer_append_packed_uns64(&buffer, rte->m_range.lo - uiLast);
        uiLast = rte->m_range.lo;
        buffer_append_packed_uns64(&buffer, rte->m_range.hi - uiLast);
        uiLast = rte->m_range.hi;

        leader_index = find_leader_index(p_leader_endpoints, iLeaderCount, rte->m_uiRoute);

        if (leader_index == iLeaderCount)
        {
            if (rte->m_uiRoute == 0)
            {
                leader_index++;
            }
            else




                VASSERT( endpoint_get_id( pDrdp->m_pSystemEndpoint ) == rte->m_uiRoute );   // 2225
        }
        else



            VASSERT( p_leader_endpoints[leader_index] == rte->m_uiRoute );   // 2231

        buffer_append_packed_int32(&buffer, leader_index);
    }



    VASSERT( !buffer.error );   // 2238
    VASSERT( DRDP_SW_ENDPOINT_MIN == ((route_table_entry_t *)vtree_peek_first(&pDrdp->m_routeTable.m_tree))->m_range.lo );   // 2239
    VASSERT( ((route_table_entry_t *)vtree_peek_last(&pDrdp->m_routeTable.m_tree))->m_range.hi == DRDP_SW_ENDPOINT_MAX );   // 2240

    VASSERT( buffer.position <= connection_get_max_message_size( 249 ) );   // 2242

    connection_send(pConnection, buffer.data, buffer.position, 249);

    delete [] pData;
    delete [] p_leader_endpoints;
}

// The other end of the same wire format.  The table is REPLACED, not merged:
// the tree is freed and re-initialised before the first entry is read.
void drdp_slave_read_route_table(buffer_t *pBuffer, route_table_t *pTable, uns64 uiOwner)
{
    route_table_entry_t *rte;
    uns64 *p_leader_endpoints;
    uns64 uiCount;
    uns64 uiLast;
    uns64 i;
    int iLeaderCount;
    int leader_index;

    uiLast = 0;
    i = 0;

    buffer_read_packed_int32(pBuffer, &iLeaderCount);
    p_leader_endpoints = new uns64[iLeaderCount];
    buffer_readm_uns64(pBuffer, p_leader_endpoints, iLeaderCount);

    buffer_read_uns64(pBuffer, &pTable->m_uiLo);
    buffer_read_uns64(pBuffer, &pTable->m_uiHi);

    vtree_free(&pTable->m_tree);
    vtree_init(&pTable->m_tree, rte_get_vnode_offset(0), (vcmp_t)uns64_range_t_cmp,
               rte_get_key_offset(0));

    buffer_read_packed_uns64(pBuffer, &uiCount);

    for (i = 0; i < uiCount; i++)
    {
        rte = (route_table_entry_t *)malloc(sizeof(route_table_entry_t));

        buffer_read_packed_uns64(pBuffer, &rte->m_range.lo);
        rte->m_range.lo += uiLast;
        uiLast = rte->m_range.lo;

        buffer_read_packed_uns64(pBuffer, &rte->m_range.hi);
        rte->m_range.hi += uiLast;
        uiLast = rte->m_range.hi;

        buffer_read_packed_int32(pBuffer, &leader_index);

        if (leader_index == iLeaderCount)
            rte->m_uiRoute = uiOwner;
        else if (leader_index == iLeaderCount + 1)
            rte->m_uiRoute = 0;
        else
            rte->m_uiRoute = p_leader_endpoints[leader_index];

        vtree_insert(&pTable->m_tree, rte);
    }











    VASSERT( !pBuffer->error );   // 2312
    VASSERT( buffer_bytes_remaining( pBuffer ) == 0 );   // 2313
    VASSERT( DRDP_SW_ENDPOINT_MIN == ((route_table_entry_t *)vtree_peek_first(&pTable->m_tree))->m_range.lo );   // 2314
    VASSERT( ((route_table_entry_t *)vtree_peek_last(&pTable->m_tree))->m_range.hi == DRDP_SW_ENDPOINT_MAX );   // 2315

    delete [] p_leader_endpoints;
}

// A leader has gone.  Every OTHER leader is told, the record goes, and every
// endpoint that leader owned is invalidated.
void drdp_leader_remove(drdp_t *pDrdp, uns64 uiId)
{
    leader_t *pLeader;

    if (!vtree_lookup(&pDrdp->m_leaders, &uiId))
        return;

    for (pLeader = (leader_t *)vtree_peek_first(&pDrdp->m_leaders); pLeader;
         pLeader = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pLeader))
    {
        if (pLeader->m_uiId != uiId && pLeader->m_pConnection)
            connection_send_uns8_uns64(pLeader->m_pConnection, 3, uiId);
    }

    drdp_leader_remove_internal(pDrdp, uiId);
    drdp_invalidate_all_endpoints_owned_by_leader(pDrdp, uiId);
}

// A leader tells the master its connection is gone; the master does the
// removal itself.
void drdp_connection_terminate(drdp_t *pDrdp, uns64 uiId, const void *pData, int iSize)
{
    buffer_t buffer;
    uns8 data[64];

    if (!vtree_lookup(&pDrdp->m_leaders, &uiId))
        return;

    drdp_leader_set_connection(pDrdp, uiId, 0);

    if (pDrdp->m_iRole == DRDP_ROLE_LEADER)
    {
        buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
        buffer_append_uns8(&buffer, 6);
        buffer_append_uns64(&buffer, uiId);
        buffer_appendm_uns8(&buffer, (const uns8 *)pData, iSize);
        connection_send(pDrdp->m_pMasterConnection, buffer.data, buffer.position, 249);
        return;
    }





























































    VASSERT( pDrdp->m_iRole == DRDP_ROLE_MASTER );   // 2422
    drdp_leader_remove(pDrdp, uiId);
}

int drdp_leader_add_internal(drdp_t *pDrdp, uns64 uiId, connection_t *pConnection)
{
    leader_t *pLeader = (leader_t *)malloc(sizeof(leader_t));

    if (!pLeader)
        return -2;

    pLeader->m_uiId = uiId;
    pLeader->m_pConnection = pConnection;

    if (pConnection)
        pDrdp->m_pfnLeaderAdd(pDrdp, pConnection);

    vtree_insert(&pDrdp->m_leaders, pLeader);
    return 0;
}

// The master has handed this process a block of ids.  It goes into whichever
// of the two slots has less left, and the master is told it arrived.
void drdp_handle_block_transfer(drdp_t *pDrdp, uns64 uiNext)
{
    int i;

    if (pDrdp->m_endpointBlock[0].remaining != 0
        && pDrdp->m_endpointBlock[1].remaining != 0)
    {






































        VASSERT( pDrdp->m_endpointBlock[0].remaining == 0 || pDrdp->m_endpointBlock[1].remaining == 0 );   // 2490
    }

    i = pDrdp->m_endpointBlock[1].remaining < pDrdp->m_endpointBlock[0].remaining;

    pDrdp->m_endpointBlock[i].next = uiNext;
    pDrdp->m_endpointBlock[i].remaining = ENDPOINT_BLOCK_SIZE;

    route_table_update_block(&pDrdp->m_routeTable, uiNext,
                             endpoint_get_id(pDrdp->m_pSystemEndpoint));
    connection_send_uns8_uns64(pDrdp->m_pMasterConnection, 34, uiNext);
}

void drdp_handle_block_received(drdp_t *pDrdp, uns64 uiNext, uns64 uiId)
{
    leader_t *pLeader;

    for (pLeader = (leader_t *)vtree_peek_first(&pDrdp->m_leaders); pLeader;
         pLeader = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pLeader))
    {
        if (uiId != pLeader->m_uiId)
            connection_send_uns8_uns64_uns64(pLeader->m_pConnection, 35, uiNext, uiId);
    }
}

void drdp_handle_endpoint_destroyed(drdp_t *pDrdp, uns64 uiId, uns64 uiFrom)
{
    leader_t *pLeader;

    for (pLeader = (leader_t *)vtree_peek_first(&pDrdp->m_leaders); pLeader;
         pLeader = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pLeader))
    {
        if (uiFrom != pLeader->m_uiId)
            connection_send_uns8_uns64_uns64(pLeader->m_pConnection, 19, uiId, (uns64)-1);
    }

    route_table_update(&pDrdp->m_routeTable, uiId, (uns64)-1);
    drdp_close_connections_to_endpoint(pDrdp, uiId);

    if (pDrdp->m_pfnEndpointDestroyed)
        pDrdp->m_pfnEndpointDestroyed(pDrdp, uiId);
}

// A leader has arrived: it gets the route table, TWO blocks of ids and the
// list of every other leader, in that order.
int drdp_leader_add(drdp_t *pDrdp, connection_t *pConnection)
{
    buffer_t buffer;
    uns8 data[1024];
    uns64 uiId;
    leader_t *pLeader;
    int iResult;

    uiId = connection_get_dst(pConnection);
    if (vtree_lookup(&pDrdp->m_leaders, &uiId))
        return -1;

    iResult = drdp_leader_add_internal(pDrdp, connection_get_dst(pConnection), pConnection);
    if (iResult)
        return iResult;

    drdp_master_send_route_table(pDrdp, pConnection);





















































































    VASSERT( pDrdp->m_endpointBlock[1].remaining >= 2 * ENDPOINT_BLOCK_SIZE );   // 2637

    drdp_assign_endpoint_block(pDrdp, pConnection);
    drdp_assign_endpoint_block(pDrdp, pConnection);
    connection_send_uns8(pConnection, 4);

    buffer_init(&buffer, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
    buffer_append_uns8(&buffer, 1);

    for (pLeader = (leader_t *)vtree_peek_first(&pDrdp->m_leaders); pLeader;
         pLeader = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pLeader))
    {
        if (pLeader->m_pConnection != pConnection && pLeader->m_pConnection)
            buffer_append_uns64(&buffer, pLeader->m_uiId);
    }












    VASSERT( !buffer.error );   // 2664

    connection_send(pConnection, buffer.data, buffer.position, 249);
    return 0;
}

void drdp_handle_endpoint_info(drdp_t *pDrdp, uns64 uiId, uns64 uiRoute)
{
    route_table_update(&pDrdp->m_routeTable, uiId, uiRoute);

    if (uiRoute == (uns64)-1)
    {
        drdp_close_connections_to_endpoint(pDrdp, uiId);
        if (pDrdp->m_pfnEndpointDestroyed)
            pDrdp->m_pfnEndpointDestroyed(pDrdp, uiId);
    }
    else
    {
        drdp_re_route_connections_to_endpoint(pDrdp, uiId, uiRoute);
    }
}











































// Every type-249 message in the library lands here.  The switch is the
// protocol between the three roles, and the jump table says the type is 1..48.
void drdp_process_system_message(drdp_t *pDrdp, message_t *pMessage)
{
    buffer_t buffer;
    connection_t *pConnection;
    connection_t *pOpened;
    leader_t *pLeader;
    leader_t *pTo;
    leader_t *pFrom;
    uns64 uiSrc;
    uns64 uiDst;
    uns64 uiId;
    uns64 uiOther;
    uns64 uiA;
    uns64 uiB;
    void *pData;
    uns32 uiSize;
    uns8 ucType;
    int iResult;
    uiSrc = message_get_src(pMessage);
    uiDst = message_get_dst(pMessage);
    pData = message_get_data(pMessage);
    uiSize = message_get_size(pMessage);
    pConnection = message_get_connection(pMessage);
    buffer_init_read(&buffer, pData, uiSize, BUFFER_LITTLE_ENDIAN);
    buffer_read_uns8(&buffer, &ucType);
    switch (ucType)
    {
    case 18:
        drdp_format_endpoint2(connection_get_dst(pConnection));
        drdp_format_endpoint1(connection_get_src(pConnection));
        break;
    case 7:
        if (pDrdp->m_iRole != DRDP_ROLE_MASTER)
        {
            connection_terminate_file_line(pConnection, 1, __FILE__, __LINE__);   // 2764
            break;
        }
        if (pDrdp->m_iDataSize + 1 == (int)message_get_size(pMessage)
            && memcmp(pDrdp->m_pData, (uns8 *)message_get_data(pMessage) + 1,
                      pDrdp->m_iDataSize) == 0)
        {
            connection_set_bps_limit(pConnection, 2097152);
            if (drdp_leader_add(pDrdp, pConnection) == 0)
                break;





            connection_terminate_file_line(pConnection, 1, __FILE__, __LINE__);   // 2779
            break;
        }





        connection_terminate_file_line(pConnection, 1, __FILE__, __LINE__);   // 2787
        break;
    case 1:
        if (pDrdp->m_iRole == DRDP_ROLE_LEADER)
        {
            while (buffer_bytes_remaining(&buffer) > 0)
            {
                buffer_read_uns64(&buffer, &uiId);
                drdp_leader_add_internal(pDrdp, uiId, 0);
            }
            connection_send(pDrdp->m_pMasterConnection, pData, uiSize, 249);
            break;
        }










        VASSERT( pDrdp->m_iRole == DRDP_ROLE_MASTER );   // 2810
        while (buffer_bytes_remaining(&buffer) > 0)
        {
            leader_t *pFound;
            buffer_read_uns64(&buffer, &uiId);
            pFound = (leader_t *)vtree_lookup(&pDrdp->m_leaders, &uiId);
            if (pFound)
                connection_send_uns8_uns64(pFound->m_pConnection, 2, uiSrc);
        }
        break;
    case 2:
        buffer_read_uns64(&buffer, &uiOther);
        iResult = connection_open(&pOpened, pDrdp->m_pSystemEndpoint, uiOther);











        VASSERT( iResult == 0 );   // 2834
        connection_set_bps_limit(pOpened, 2097152);
        connection_send_uns8(pOpened, 5);
        drdp_leader_add_internal(pDrdp, uiOther, pOpened);
        break;
    case 5:
        if (!vtree_lookup(&pDrdp->m_leaders, &uiSrc))
        {











            connection_terminate_file_line(pConnection, 1, __FILE__, __LINE__);   // 2853
            break;
        }

        VASSERT( pConnection != 0 );   // 2857
        connection_set_bps_limit(pConnection, 2097152);
        drdp_leader_set_connection(pDrdp, uiSrc, pConnection);
        break;
    case 6:
    {
        uns8 data[128];
        buffer_t reply;



        VASSERT( pDrdp->m_iRole == DRDP_ROLE_MASTER );   // 2868
        if (!vtree_lookup(&pDrdp->m_leaders, &uiSrc))
            break;
        buffer_read_uns64(&buffer, &uiId);
        pLeader = (leader_t *)vtree_lookup(&pDrdp->m_leaders, &uiId);
        if (!pLeader)
            break;
        buffer_init(&reply, data, sizeof(data), BUFFER_LITTLE_ENDIAN);
        buffer_append_uns8(&reply, 12);
        buffer_append_uns64(&reply, uiSrc);
        buffer_append_uns64(&reply, uiId);
        buffer_copy(&buffer, &reply, buffer_bytes_remaining(&buffer));
        connection_terminate(pLeader->m_pConnection, 1, reply.data, reply.position);
        break;
    }
    case 3:
    {
        leader_t *pGone;


















        VASSERT( pDrdp->m_iRole == DRDP_ROLE_LEADER );   // 2904
        buffer_read_uns64(&buffer, &uiId);
        pGone = (leader_t *)vtree_lookup(&pDrdp->m_leaders, &uiId);
        if (!pGone)
            break;
        if (pGone->m_pConnection)
            connection_terminate_reason(pGone->m_pConnection, 1, 13);
        drdp_leader_remove_internal(pDrdp, uiId);
        drdp_invalidate_all_endpoints_owned_by_leader(pDrdp, uiId);
        break;
    }
    case 32:
        drdp_assign_endpoint_block(pDrdp, pConnection);
        break;
    case 33:
        buffer_read_uns64(&buffer, &uiA);
        drdp_handle_block_transfer(pDrdp, uiA);
        break;
    case 34:
        buffer_read_uns64(&buffer, &uiA);
        drdp_handle_block_received(pDrdp, uiA, uiSrc);
        break;
    case 48:
        drdp_slave_read_route_table(&buffer, &pDrdp->m_routeTable,
                                    connection_get_dst(pDrdp->m_pMasterConnection));
        break;
    case 35:
        buffer_read_uns64(&buffer, &uiA);
        buffer_read_uns64(&buffer, &uiB);
        route_table_update_block(&pDrdp->m_routeTable, uiA, uiB);
        break;
    case 36:
        if (uiSrc == connection_get_dst(pDrdp->m_pMasterConnection))
        {
            buffer_read_uns64(&buffer, &uiA);
            route_table_update_block(&pDrdp->m_routeTable, uiA, uiSrc);
        }
        else





























            connection_terminate_file_line(pConnection, 1, __FILE__, __LINE__);   // 2971
        break;
    case 17:
        if (pDrdp->m_iRole == DRDP_ROLE_MASTER)
        {
            uns64 uiEndpoint, uiTo, uiFrom;
            leader_t *pWalk, *pNext;
            buffer_read_uns64(&buffer, &uiEndpoint);
            buffer_read_uns64(&buffer, &uiTo);
            pFrom = (leader_t *)vtree_lookup(&pDrdp->m_leaders, &uiSrc);
            if (!pFrom)
            {
                drdp_log_hack_attempt(pDrdp, drdp_format_endpoint1(uiSrc),
                                      "HACK DRDP_SYSTEM_ENDPOINT_TRANSFER receieved from non-leader");




                connection_terminate_file_line(pConnection, 1, __FILE__, __LINE__);   // 2989
                break;
            }
            pTo = (leader_t *)vtree_lookup(&pDrdp->m_leaders, &uiTo);
            if (!pTo)
                pTo = pFrom;
            drdp_format_endpoint1(uiEndpoint);
            drdp_format_endpoint2(message_get_src(pMessage));
            drdp_format_endpoint3(connection_get_dst(pTo->m_pConnection));
            message_get_size(pMessage);
            uiFrom = pTo->m_uiId;
            connection_send(pTo->m_pConnection, message_get_data(pMessage),
                            message_get_size(pMessage), 249);
            if (pFrom->m_uiId == uiFrom)
                break;
            route_table_update(&pDrdp->m_routeTable, uiEndpoint, uiFrom);
            drdp_re_route_connections_to_endpoint(pDrdp, uiEndpoint, uiFrom);
            pWalk = (leader_t *)vtree_peek_first(&pDrdp->m_leaders);
            while (pWalk)
            {
                pNext = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pWalk);
                if (uiFrom != pWalk->m_uiId)
                {
                    connection_send_uns8_uns64_uns64(pWalk->m_pConnection, 19,
                                                     uiEndpoint, uiTo);
                }
                pWalk = pNext;
            }
        }
        else
        {
            uns64 uiEndpoint;
            uns64 uiTo;
            uns64 uiSkip;
            endpoint_t *pEndpoint;
            buffer_read_uns64(&buffer, &uiEndpoint);
            buffer_read_uns64(&buffer, &uiTo);
            buffer_read_uns64(&buffer, &uiSkip);
            buffer_skip(&buffer, (int)uiSkip);
            iResult = endpoint_create_buffer(&pEndpoint, pDrdp, &buffer);








































            VASSERT( iResult == 0 );   // 3069
            vtree_insert(&pDrdp->m_endpoints, pEndpoint);
            drdp_format_endpoint1(uiEndpoint);
            route_table_update(&pDrdp->m_routeTable, endpoint_get_id(pEndpoint),
                               endpoint_get_id(pDrdp->m_pSystemEndpoint));
        }
        break;
    case 21:
        buffer_read_uns64(&buffer, &uiId);
        drdp_handle_endpoint_destroyed(pDrdp, uiId, uiSrc);
        break;
    case 19:
        buffer_read_uns64(&buffer, &uiId);
        buffer_read_uns64(&buffer, &uiB);
        drdp_handle_endpoint_info(pDrdp, uiId, uiB);
        break;
    case 20:
        if (uiSrc == connection_get_dst(pDrdp->m_pMasterConnection))
        {
            buffer_read_uns64(&buffer, &uiId);
            drdp_handle_endpoint_info(pDrdp, uiId, uiSrc);
        }
        else












            connection_terminate_file_line(pConnection, 1, __FILE__, __LINE__);   // 3104
        break;
    }
}





message_t *drdp_receive(drdp_t *pDrdp)
{
    message_t *pMessage;
    endpoint_t *pEndpoint;
    VASSERT( drdp_is_valid( pDrdp ) );   // 3117

    pMessage = (message_t *)vlist_peek_head(&pDrdp->m_arrivals);

    if (pMessage)
    {
        pEndpoint = message_get_endpoint(pMessage);
        if (!pEndpoint)
            drdp_arrival_remove(pDrdp, pMessage);
        else
            endpoint_arrival_remove(pEndpoint, pMessage);

        if (message_get_type(pMessage) == 249)
            drdp_process_system_message(pDrdp, pMessage);

        if (drdp_is_software_endpoint(message_get_src(pMessage))
            && drdp_is_software_endpoint(message_get_dst(pMessage)))
        {















            VASSERT( message_get_type( pMessage ) != 255 && message_get_type( pMessage ) != 253 );   // 3150
            message_set_connection(pMessage, 0);
        }
    }

    if (pMessage)
    {
        message_get_connection(pMessage);
        message_get_endpoint(pMessage);
    }

    return pMessage;
}

int drdp_get_route(drdp_t *pDrdp, uns64 uiId, route_t **ppRoute)
{
    int iResult;
    route_t *pRoute;

    *ppRoute = 0;

    pRoute = (route_t *)vtree_lookup(&pDrdp->m_routesById, &uiId);
    if (!pRoute)
    {
        iResult = route_create(&pRoute, pDrdp, uiId);
        if (iResult)
            goto done;
        vtree_insert(&pDrdp->m_routesById, pRoute);
        vtree_insert(&pDrdp->m_routes.tree, pRoute);
    }
    iResult = 0;
    *ppRoute = pRoute;
done:
    return iResult;
}

// A hardware endpoint id IS its address: four octets, the UDP port and the
// drdp port, packed big-end first into 64 bits.
void drdp_address_to_endpoint(const uns8 *pIp, uns16 usPort, uns16 usDrdpPort, uns64 *puiId)
{
    uns64 uiId = 0;
    int i;

    for (i = 0; i < 4; i++)
    {
        uiId = uiId << 8;
        uiId = uiId | pIp[i];
    }

    uiId = uiId << 16;
    uiId = usPort | uiId;
    uiId = uiId << 16;
    uiId = usDrdpPort | uiId;

    *puiId = uiId;
}

void drdp_endpoint_to_address(uns64 uiId, uns8 *pIp, uns16 *pusPort, uns16 *pusDrdpPort)
{
    int i;

    *pusDrdpPort = (uns16)uiId;
    uiId >>= 16;
    *pusPort = (uns16)uiId;
    uiId >>= 16;

    for (i = 0; i < 4; i++)
    {
        pIp[3 - i] = (uns8)uiId;
        uiId >>= 8;
    }
}

// Four formatters into four static rings, so a trace line can carry four
// different addresses without the caller owning a buffer.
char *drdp_format_addr(char *pName, const drdp_addr_t *pAddress)
{
    sprintf(pName, "%u.%u.%u.%u:%u", pAddress->m_ucIp[0], pAddress->m_ucIp[1],
            pAddress->m_ucIp[2], pAddress->m_ucIp[3], pAddress->m_usPort);
    return pName;
}

const char *drdp_format_addr1(const drdp_addr_t *pAddress)
{
    static char szName[32];

    return drdp_format_addr(szName, pAddress);
}

const char *drdp_format_addr2(const drdp_addr_t *pAddress)
{
    static char szName[32];

    return drdp_format_addr(szName, pAddress);
}

const char *drdp_format_addr3(const drdp_addr_t *pAddress)
{
    static char szName[32];

    return drdp_format_addr(szName, pAddress);
}

char *drdp_format_endpoint(char *pName, uns64 uiId)
{
    uns8 ucIp[4];
    uns16 usPort;
    uns16 usDrdpPort;
    if (uiId == (uns64)-1)
        memcpy(pName, INVALID_NAME, sizeof(INVALID_NAME));
    else if (drdp_is_hardware_endpoint(uiId))
    {
        drdp_endpoint_to_address(uiId, ucIp, &usPort, &usDrdpPort);
        if (drdp_is_local_hardware_endpoint(uiId))
            sprintf(pName, "local:%u", usDrdpPort);
        else
            sprintf(pName, "%u.%u.%u.%u:%u:%u", ucIp[0], ucIp[1], ucIp[2], ucIp[3],
                    usPort, usDrdpPort);
    }
    else
        sprintf(pName, "0x%x", (int)uiId);
    return pName;
}

const char *drdp_format_endpoint1(uns64 uiId)
{
    static char szName[32];

    return drdp_format_endpoint(szName, uiId);
}

const char *drdp_format_endpoint2(uns64 uiId)
{
    static char szName[32];

    return drdp_format_endpoint(szName, uiId);
}

const char *drdp_format_endpoint3(uns64 uiId)
{
    static char szName[32];

    return drdp_format_endpoint(szName, uiId);
}

const char *drdp_format_endpoint4(uns64 uiId)
{
    static char szName[32];

    return drdp_format_endpoint(szName, uiId);
}

void drdp_resort(route_t *pRoute)
{
    routes_by_desired_flush_time_resort(&route_get_drdp(pRoute)->m_routes, pRoute);
}

int leader_get_vnode_offset(int iIndex)
{
    if (iIndex == 0)
        return 0;
    return -1;
}

int leader_get_key_offset(int iIndex)
{
    return iIndex == 0 ? 16 : -1;
}

int drdp_get_leader_connections(drdp_t *pDrdp, connection_t **ppConnections, int iMax)
{
    leader_t *pLeader;
    int n = 0;

    for (pLeader = (leader_t *)vtree_peek_first(&pDrdp->m_leaders); pLeader;
         pLeader = (leader_t *)vtree_get_next(&pDrdp->m_leaders, pLeader))
    {
        if (pLeader->m_pConnection)
        {
            if (n == iMax)
            {
                n = -1;
                goto done;
            }
            ppConnections[n++] = pLeader->m_pConnection;
        }
    }
done:
    return n;
}

void drdp_set_default_logger(drdp_t *pDrdp, drdp::logger_t *pLogger)
{
    pDrdp->m_pLogger = pLogger;
}

drdp::logger_t *drdp_get_default_logger(drdp_t *pDrdp)
{
    return pDrdp->m_pLogger;
}

// The id space, tested three ways.  A hardware id is an address, so it has a
// non-zero port half and nothing in the top byte; a software id is a plain
// number between the two block bounds.
int drdp_is_hardware_endpoint(uns64 uiId)
{
    int iResult = 0;

    if (uiId != (uns64)-1 && ( uiId >> 56 ) != 0)
    {
        if (uiId & 0xffff0000)
            iResult = 1;
    }

    return iResult;
}

int drdp_is_software_endpoint(uns64 uiId)
{
    return uiId - DRDP_SW_ENDPOINT_MIN <= DRDP_SW_ENDPOINT_MAX - DRDP_SW_ENDPOINT_MIN;
}

int drdp_is_endpoint(uns64 uiId)
{
    int iResult = 0;

    if (drdp_is_hardware_endpoint(uiId) || drdp_is_software_endpoint(uiId))
        iResult = 1;


    return iResult;
}

int drdp_is_local_hardware_endpoint(uns64 uiId)
{
    int iResult;

    if (uiId == (uns64)-1)
        iResult = 0;
    else
        iResult = ( uiId & 0xffffffffffff0000 ) == 0xffffffffffff0000;
    return iResult;
}

int drdp_is_remote_hardware_endpoint(uns64 uiId)
{
    int iResult = 0;

    if (uiId != (uns64)-1 && ( uiId >> 56 ) != 0)
    {
        if (( uiId & 0xffffffffffff0000 ) != 0xffffffffffff0000)
            iResult = 1;
    }

    return iResult;
}

uns16 drdp_get_port_udp(const drdp_t *pDrdp)
{
    return pDrdp->m_bindAddress.m_usPort;
}

uns16 drdp_get_port_drdp(const drdp_t *pDrdp)
{
    return (uns16)endpoint_get_id(pDrdp->m_pSystemEndpoint);
}

void drdp_set_port_udp(drdp_t *pDrdp, uns16 usPort)
{
    pDrdp->m_bindAddress.m_usPort = usPort;
}

connection_t *drdp_get_master_connection(drdp_t *pDrdp)
{
    return pDrdp->m_pMasterConnection;
}

int drdp_has_endpoint(const drdp_t *pDrdp, const endpoint_t *pEndpoint)
{
    uns64 uiId = endpoint_get_id(pEndpoint);
    int iResult = drdp_is_valid(pDrdp);

    if (iResult)
        iResult = pEndpoint == (endpoint_t *)vtree_lookup(&pDrdp->m_endpoints, &uiId);
    return iResult;
}

void drdp_connection_insert(drdp_t *pDrdp, connection_t *pConnection)
{
    vtree_insert(&pDrdp->m_connections, pConnection);
}

void drdp_connection_remove(connection_t *pConnection)
{
    vtree_remove(&endpoint_get_drdp(connection_get_endpoint(pConnection))->m_connections,
                 pConnection);
}

int drdp_get_bps_sent(drdp_t *pDrdp)
{
    return bandwidth_get_bps_sent(&pDrdp->m_bandwidth);
}

int drdp_get_bps_recv(drdp_t *pDrdp)
{
    return bandwidth_get_bps_recv(&pDrdp->m_bandwidth);
}

int drdp_is_leader_endpoint(drdp_t *pDrdp, uns64 uiId)
{
    leader_t *pLeader = (leader_t *)vtree_lookup(&pDrdp->m_leaders, &uiId);
    int iResult = 0;

    if (pLeader)
        iResult = pLeader->m_pConnection != 0;

    return iResult;
}

void drdp_endpoint_transfer(drdp_t *pDrdp, endpoint_t *pEndpoint, uns64 uiTo,
                            const void *pData, int iSize)
{
    vtree_remove(&pDrdp->m_endpoints, pEndpoint);
    drdp_format_endpoint1(endpoint_get_id(pEndpoint));
    drdp_format_endpoint2(uiTo);
    endpoint_transfer(pEndpoint, uiTo, pData, iSize);
}

void drdp_route_override(drdp_t *pDrdp, uns64 uiFrom, uns64 uiTo)
{
    drdp_addr_t from;
    drdp_addr_t to;
    uns16 usDrdpPort;

    drdp_endpoint_to_address(uiFrom, from.m_ucIp, &from.m_usPort, &usDrdpPort);
    drdp_endpoint_to_address(uiTo, to.m_ucIp, &to.m_usPort, &usDrdpPort);

    route_cache_touch(&pDrdp->m_routeCache, &from, &to, pDrdp->m_uiTime);
}

// A raw datagram to a LIST of addresses, prefixed with the wildcard drdp port
// so the far end takes it as connectionless.  The packet is built once on the
// stack and sent once per address.
void drdp_send_addr(drdp_t *pDrdp, const drdp_addr_t *pAddress, int iCount,
                    const void *pData, int iSize)
{
    ipaddress_t address;
    uns8 *pPacket;
    int iTotal;
    int i;

    pPacket = (uns8 *)__builtin_alloca(iSize + 2);
    memset(pPacket, 255, 2);
    iTotal = iSize + 2;
    memcpy(pPacket + 2, pData, iSize);

    for (i = 0; i < iCount; i++)
    {
        drdp_addr_to_ipaddress(pAddress, &address);

        pDrdp->m_iBytesSent += iSize;

        if (!pDrdp->m_bNoSend)
        {
            usocket_sendto_addr(pDrdp->m_socket[0], &address, pPacket, iTotal, 0, 0, 0);
        }

        bandwidth_send(&pDrdp->m_bandwidth, pDrdp->m_uiTime, iTotal + 28);
        pAddress++;
    }
}

endpoint_t *drdp_get_system_endpoint(drdp_t *pDrdp)
{
    return pDrdp->m_pSystemEndpoint;
}

int drdp_is_client(drdp_t *pDrdp)
{
    return pDrdp->m_iRole == DRDP_ROLE_CLIENT;
}

// A reset goes back the way the segment came: src and dst swapped, and the
// arriving crc carried as the far end's instance number.
void drdp_send_reset_connection(drdp_t *pDrdp, const segment_header_t *pHeader)
{
    route_t *pRoute = 0;

    if (pHeader->m_uiFlags & 0x30000)
        return;

    if (drdp_get_route(pDrdp, pHeader->m_uiRoute, &pRoute))
        return;

    route_send_reset_connection(pRoute, pHeader->m_uiDst, pHeader->m_uiSrc,
                                (pHeader->m_uiFlags & 0x2000) ? 2 : 0,
                                0, pHeader->m_uiCrc);
}

void drdp_send_reset_endpoint(drdp_t *pDrdp, const segment_header_t *pHeader)
{
    route_t *pRoute = 0;

    if (pHeader->m_uiFlags & 0x30000)
        return;

    if (drdp_get_route(pDrdp, pHeader->m_uiRoute, &pRoute))
        return;

    route_send_reset_endpoint(pRoute, pHeader->m_uiDst);
}

int drdp_get_local_address(uns8 *pIp)
{
    ipaddress_t address;
    int iResult;

    iResult = unet_get_local_address(&address);
    if (!iResult)
        memcpy(pIp, address.m_ucIp, 4);

    return iResult;
}

void drdp_set_app_ptr(drdp_t *pDrdp, void *pAppPtr)
{
    pDrdp->m_pAppPtr = pAppPtr;
}

void *drdp_get_app_ptr(const drdp_t *pDrdp)
{
    return pDrdp->m_pAppPtr;
}

ipaddress_t *drdp_get_bind_address(const drdp_t *pDrdp)
{
    return (ipaddress_t *)&pDrdp->m_bindAddress;
}

void drdp_remap_address_to_route(drdp_t *pDrdp, const drdp_addr_t *pFrom, drdp_addr_t *pTo)
{
    route_cache_address_to_route(&pDrdp->m_routeCache, pFrom, pTo, pDrdp->m_uiTime);
}

void drdp_remap_route_to_address(drdp_t *pDrdp, const drdp_addr_t *pFrom, drdp_addr_t *pTo)
{
    route_cache_route_to_address(&pDrdp->m_routeCache, pFrom, pTo, pDrdp->m_uiTime);
}

void drdp_log_hack_attempt(drdp_t *pDrdp, const char *pWho, const char *pWhat)
{
    if (pDrdp->m_pLogger)
        pDrdp->m_pLogger->hack(pWho, pWhat);
}

float drdp_get_flush_interval(drdp_t *pDrdp)
{
    return bandwidth_get_process_interval_max(&pDrdp->m_bandwidth);
}

// The ordinary CRC-32, unrolled eight ways by hand.
#define DRDP_CRC_STEP   uiCrc = drdp_crc_table[( uiCrc ^ *pData++ ) & 0xff] ^ ( uiCrc >> 8 )

uns32 drdp_crc(uns32 uiCrc, const char *pData, uns32 uiSize)
{
    uiCrc = ~uiCrc;

    while (uiSize >= 8)
    {
        DRDP_CRC_STEP;
        DRDP_CRC_STEP;
        DRDP_CRC_STEP;
        DRDP_CRC_STEP;
        DRDP_CRC_STEP;
        DRDP_CRC_STEP;
        DRDP_CRC_STEP;
        DRDP_CRC_STEP;
        uiSize -= 8;
    }

    while (uiSize)
    {
        DRDP_CRC_STEP;
        uiSize--;
    }

    return ~uiCrc;
}

uns32 drdp_crc_table_get(int iIndex)
{
    return drdp_crc_table[iIndex];
}

// Which table entry a value is, searched outwards from where the caller
// expects it -- the protocol version is recovered this way.
int drdp_crc_table_lookup(uns32 uiValue, int iIndex)
{
    int lo = iIndex - 4;
    int hi = iIndex + 4;
    int i;

    if (lo < 0)
        lo = 0;
    if (hi >= 256)
        hi = 255;

    if (drdp_crc_table[iIndex] == uiValue)
        return iIndex;

    for (i = lo; i < hi; i++)
    {
        if (drdp_crc_table[i] == uiValue)
            return i;
    }

    return -1;
}

// The second socket.  A destination or port unreachable naming this process's
// own bind address terminates every route to the machine that answered.
int drdp_process_icmp(drdp_t *pDrdp, const void *pData, int iSize)
{
    icmp_info_t info;
    drdp_addr_t address;
    ipaddress_t *pBind;
    int iType;

    pBind = drdp_get_bind_address(pDrdp);

    if (iSize < 6)
        return -1;

    if (icmp_parse((const char *)pData + 6, iSize - 6, &info))
        return -1;

    iType = info.m_ucType;
    if (iType < 3)
        return 0;
    if (iType >= 5 && iType != 11)
        return 0;

    if (pBind->m_ucIp[0])
    {
        if (memcmp(info.m_local.m_ucIp, pBind->m_ucIp, 4))
            return -1;
    }

    if (info.m_local.m_usPort != pBind->m_usPort)
        return -1;

    if (info.m_ucType != 3)
        return 0;

    if (info.m_ucCode != 3 && info.m_ucCode != 1)
        return 0;

    ipaddress_to_drdp_addr(&info.m_remote, &address);
    drdp_route_terminate(pDrdp, &address, 1);
    return 0;
}
