// route_cache.cpp -- SUPPORT 0x004bc300..0x004bc9d0.
//
// The translation between the address a packet really came from and the
// address the protocol thinks of as the route, in BOTH directions, with a
// five-minute expiry.  It exists because a NAT rewrites the source port: the
// far end says "I am 1.2.3.4:5000" and the packet arrives from 1.2.3.4:41235,
// and every later segment for that endpoint has to go back to the port the NAT
// actually opened.
//
// One record is on THREE containers -- a tree per direction and a list by
// expiry -- which is why `rce` exposes two vnode offsets AND a vlink offset.
#include "drdp.h"

// Five minutes.  It is added to `now` at every touch, and `release_expired`
// walks the list from the front until it finds one that has not run out.
#define ROUTE_CACHE_EXPIRE 300000

int rce_get_vnode_offset(int iIndex)
{
    return iIndex * 16;
}

int rce_get_vlink_offset(int iIndex)
{
    return iIndex == 0 ? 32 : -1;
}

int rce_get_key_offset(int iIndex)
{
    int iOffset = -1;

    switch (iIndex)
    {
    case 0:
        iOffset = 40;
        break;
    case 1:
        iOffset = 46;
        break;
    case 2:
        iOffset = 52;
        break;
    }
    return iOffset;
}

void route_cache_init(route_cache_t *pCache)
{
    vtree_init(&pCache->m_byAddress, rce_get_vnode_offset(0), (vcmp_t)drdp_addr_t_cmp,
               rce_get_key_offset(0));
    vtree_init(&pCache->m_byRoute, rce_get_vnode_offset(1), (vcmp_t)drdp_addr_t_cmp,
               rce_get_key_offset(1));
    vlist_init(&pCache->m_byExpire, rce_get_vlink_offset(0), (vcmp_t)uns32_cmp_wrap,
               rce_get_key_offset(2));
}

void route_cache_destroy(route_cache_t *pCache)
{
    route_cache_entry_t *pEntry;

    while ((pEntry = (route_cache_entry_t *)vlist_remove_head(&pCache->m_byExpire)) != 0)
    {
        vtree_remove(&pCache->m_byAddress, pEntry);
        vtree_remove(&pCache->m_byRoute, pEntry);
        free(pEntry);
    }
}

// Both directions are looked up first.  If neither is known this is a new
// pair; if both are known and they are DIFFERENT records the older mapping has
// been superseded, so it is thrown away and the survivor is re-keyed.
void route_cache_touch(route_cache_t *pCache, const drdp_addr_t *pAddress,
                       const drdp_addr_t *pRoute, uns32 uiNow)
{
    route_cache_entry_t *pByAddress;
    route_cache_entry_t *pByRoute;

    route_cache_release_expired(pCache, uiNow);
    pByAddress = (route_cache_entry_t *)vtree_lookup(&pCache->m_byAddress, pAddress);
    pByRoute = (route_cache_entry_t *)vtree_lookup(&pCache->m_byRoute, pRoute);
    if (!pByAddress && !pByRoute)
    {
        route_cache_entry_t *pEntry = (route_cache_entry_t *)malloc(sizeof(*pEntry));

        if (!pEntry)
            return;
        pEntry->m_address = *pAddress;
        pEntry->m_route = *pRoute;
        pEntry->m_uiExpire = uiNow + ROUTE_CACHE_EXPIRE;
        vlist_insert(&pCache->m_byExpire, pEntry);
        vtree_insert(&pCache->m_byAddress, pEntry);
        vtree_insert(&pCache->m_byRoute, pEntry);
        drdp_format_addr1(&pEntry->m_address);
        drdp_format_addr2(&pEntry->m_route);
        return;
    }
    if (pByAddress && pByRoute && pByAddress != pByRoute)
    {
        vtree_remove(&pCache->m_byAddress, pByRoute);
        vtree_remove(&pCache->m_byRoute, pByRoute);
        vlist_remove(&pCache->m_byExpire, pByRoute);
        drdp_format_addr1(&pByRoute->m_address);
        drdp_format_addr2(&pByRoute->m_route);
        drdp_format_addr1(pAddress);
        drdp_format_addr2(pRoute);
        free(pByRoute);
        pByRoute = 0;
    }
    if (!pByAddress)
        pByAddress = pByRoute;
    if (drdp_addr_t_cmp(&pByAddress->m_address, pAddress))
    {
        vtree_remove(&pCache->m_byAddress, pByAddress);
        pByAddress->m_address = *pAddress;
        vtree_insert(&pCache->m_byAddress, pByAddress);
    }
    else if (drdp_addr_t_cmp(&pByAddress->m_route, pRoute))
    {
        vtree_remove(&pCache->m_byRoute, pByAddress);
        pByAddress->m_route = *pRoute;
        vtree_insert(&pCache->m_byRoute, pByAddress);
    }
    drdp_format_addr1(&pByAddress->m_address);
    drdp_format_addr2(&pByAddress->m_route);
    drdp_format_addr1(pAddress);
    drdp_format_addr2(pRoute);
    if (pByAddress->m_uiExpire != uiNow + ROUTE_CACHE_EXPIRE)
    {
        vlist_remove(&pCache->m_byExpire, pByAddress);
        pByAddress->m_uiExpire = uiNow + ROUTE_CACHE_EXPIRE;
        vlist_insert(&pCache->m_byExpire, pByAddress);
    }
    drdp_format_addr1(&pByAddress->m_address);
    drdp_format_addr2(&pByAddress->m_route);
}

// The list is sorted by expiry, so the sweep stops at the first live record --
// close_set_release_expired's shape, a second time.
void route_cache_release_expired(route_cache_t *pCache, uns32 uiNow)
{
    route_cache_entry_t *pEntry;

    while ((pEntry = (route_cache_entry_t *)vlist_peek_head(&pCache->m_byExpire)) != 0
           && (int)(pEntry->m_uiExpire - uiNow) <= 0)
    {
        drdp_format_addr1(&pEntry->m_address);
        drdp_format_addr2(&pEntry->m_route);
        vtree_remove(&pCache->m_byAddress, pEntry);
        vtree_remove(&pCache->m_byRoute, pEntry);
        vlist_remove_head(&pCache->m_byExpire);
        free(pEntry);
    }
}

// A miss answers with the address unchanged, so a machine nobody has heard
// from is reached at the address it claims.  A hit also RENEWS the record --
// the cache is keyed by use, not by when the mapping was learned.
void route_cache_address_to_route(route_cache_t *pCache, const drdp_addr_t *pAddress,
                                  drdp_addr_t *pRoute, uns32 uiNow)
{
    route_cache_entry_t *pEntry =
        (route_cache_entry_t *)vtree_lookup(&pCache->m_byAddress, pAddress);

    if (!pEntry)
    {
        *pRoute = *pAddress;
        return;
    }
    *pRoute = pEntry->m_route;
    if (pEntry->m_uiExpire != uiNow + ROUTE_CACHE_EXPIRE)
    {
        vlist_remove(&pCache->m_byExpire, pEntry);
        pEntry->m_uiExpire = uiNow + ROUTE_CACHE_EXPIRE;
        vlist_insert(&pCache->m_byExpire, pEntry);
    }
}

void route_cache_route_to_address(route_cache_t *pCache, const drdp_addr_t *pRoute,
                                  drdp_addr_t *pAddress, uns32 uiNow)
{
    route_cache_entry_t *pEntry =
        (route_cache_entry_t *)vtree_lookup(&pCache->m_byRoute, pRoute);

    if (!pEntry)
    {
        *pAddress = *pRoute;
        return;
    }
    *pAddress = pEntry->m_address;
    if (pEntry->m_uiExpire != uiNow + ROUTE_CACHE_EXPIRE)
    {
        vlist_remove(&pCache->m_byExpire, pEntry);
        pEntry->m_uiExpire = uiNow + ROUTE_CACHE_EXPIRE;
        vlist_insert(&pCache->m_byExpire, pEntry);
    }
}

// Address first, then port, both unsigned -- so the ordering is by machine and
// then by the port the NAT happened to pick.
int drdp_addr_t_cmp(const drdp_addr_t *pA, const drdp_addr_t *pB)
{
    int iResult = memcmp(pA->m_ucIp, pB->m_ucIp, 4);

    if (iResult > 0)
        return 1;
    if (iResult < 0)
        return -1;
    if (!(pA->m_usPort > pB->m_usPort))
        return pA->m_usPort < pB->m_usPort ? -1 : 0;
    return 1;
}
