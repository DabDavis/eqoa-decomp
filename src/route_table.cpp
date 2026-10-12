// route_table.cpp -- SUPPORT 0x004bc9d0..0x004bcf00.
//
// Which endpoint ids live behind which route, held as a tree of RANGES.  The
// comparator returns 0 for any two ranges that OVERLAP, so a lookup by a
// degenerate range (lo == hi == the id) finds the range that contains it, and
// the tree is a sorted partition of the whole id space rather than a set of
// points.
//
// `update_range` is the whole file: it takes a sub-range of an existing entry
// and gives it a different route, which means splitting the entry it landed in
// and then merging with either neighbour that now carries the same route.
#include "drdp.h"

int rte_get_vnode_offset(int iIndex)
{
    return iIndex == 0 ? 0 : -1;
}

int rte_get_key_offset(int iIndex)
{
    return iIndex == 0 ? 16 : -1;
}











// The range handed in is always contained in exactly one existing entry --
// `init` seeds the tree with one covering everything and nothing ever leaves a
// hole -- which is what the two asserts say.
void route_table_update_range(route_table_t *pTable, uns64_range_t *pRange, uns64 uiRoute)
{
    route_table_entry_t *pRte =
        (route_table_entry_t *)vtree_lookup(&pTable->m_tree, pRange);
    route_table_entry_t *pPrev;
    route_table_entry_t *pNext;
    route_table_entry_t *pNew;

    VASSERT(pRte != 0);

    VASSERT(pRte->m_range.lo <= pRange->lo && pRange->hi <= pRte->m_range.hi);
    if (pRte->m_uiRoute == uiRoute)
        return;
    if (pRte->m_range.lo == pRange->lo && pRte->m_range.hi == pRange->hi)
    {
        if (uiRoute == (uns64)-1)
        {
            vtree_remove(&pTable->m_tree, pRte);
            free(pRte);
            return;
        }
        pPrev = (route_table_entry_t *)vtree_get_prev(&pTable->m_tree, pRte);
        pNext = (route_table_entry_t *)vtree_get_next(&pTable->m_tree, pRte);
        pRte->m_uiRoute = uiRoute;
        if (pPrev && pPrev->m_range.hi + 1 == pRange->lo && pPrev->m_uiRoute == uiRoute)
        {
            vtree_remove(&pTable->m_tree, pPrev);
            pRte->m_range.lo = pPrev->m_range.lo;
            free(pPrev);
        }
        if (!pNext)
            return;
        if (pRange->hi + 1 != pNext->m_range.lo)
            return;
        if (uiRoute != pNext->m_uiRoute)
            return;
        vtree_remove(&pTable->m_tree, pNext);
        pRte->m_range.hi = pNext->m_range.hi;
        free(pNext);
        return;
    }
    if (pRte->m_range.lo == pRange->lo)
    {
        pRte->m_range.lo = pRange->hi + 1;
        if (uiRoute == (uns64)-1)
            return;
        pPrev = (route_table_entry_t *)vtree_get_prev(&pTable->m_tree, pRte);
        if (pPrev && pPrev->m_range.hi + 1 == pRange->lo && pPrev->m_uiRoute == uiRoute)
        {
            pPrev->m_range.hi = pRange->hi;
            return;
        }
    }
    else if (pRte->m_range.hi == pRange->hi)
    {
        pRte->m_range.hi = pRange->lo - 1;
        if (uiRoute == (uns64)-1)
            return;
        pNext = (route_table_entry_t *)vtree_get_next(&pTable->m_tree, pRte);
        if (pNext && pRange->hi + 1 == pNext->m_range.lo && uiRoute == pNext->m_uiRoute)
        {
            pNext->m_range.lo = pRange->lo;
            return;
        }
    }
    else
    {
        pNew = (route_table_entry_t *)malloc(sizeof(*pNew));
        pNew->m_range.lo = pRange->hi + 1;
        pNew->m_range.hi = pRte->m_range.hi;
        pNew->m_uiRoute = pRte->m_uiRoute;
        pRte->m_range.hi = pRange->lo - 1;
        vtree_insert(&pTable->m_tree, pNew);
        if (uiRoute == (uns64)-1)
            return;
    }
    pRte = (route_table_entry_t *)malloc(sizeof(*pRte));
    pRte->m_range.lo = pRange->lo;
    pRte->m_range.hi = pRange->hi;
    pRte->m_uiRoute = uiRoute;
    vtree_insert(&pTable->m_tree, pRte);
}

void route_table_update(route_table_t *pTable, uns64 uiId, uns64 uiRoute)
{
    uns64_range_t range;

    range.lo = uiId;
    range.hi = uiId;
    route_table_update_range(pTable, &range, uiRoute);
}

// A block is 2,048 consecutive ids -- the granularity the master hands them
// out in, so one message can move a whole endpoint block to a new route.
void route_table_update_block(route_table_t *pTable, uns64 uiId, uns64 uiRoute)
{
    uns64_range_t range;

    range.lo = uiId;
    range.hi = uiId + 2047;
    route_table_update_range(pTable, &range, uiRoute);
}

// The tree starts as ONE entry covering the whole space with route zero, which
// is the value `find` reports as "no route".
void route_table_init(route_table_t *pTable, uns64 uiLo, uns64 uiHi)
{
    route_table_entry_t *pRte;

    vtree_init(&pTable->m_tree, rte_get_vnode_offset(0), (vcmp_t)uns64_range_t_cmp,
               rte_get_key_offset(0));
    pTable->m_uiLo = uiLo;
    pTable->m_uiHi = uiHi;
    pRte = (route_table_entry_t *)malloc(sizeof(*pRte));
    pRte->m_range.lo = pTable->m_uiLo;
    pRte->m_range.hi = pTable->m_uiHi;
    pRte->m_uiRoute = 0;
    vtree_insert(&pTable->m_tree, pRte);
}

void route_table_destroy(route_table_t *pTable)
{
    vtree_free(&pTable->m_tree);
}

// An id outside the table's own bounds fails; an id inside it that lands on an
// entry with route zero fails too, and one that lands on no entry at all
// answers -1 and SUCCEEDS -- which is the case `init` makes impossible.
int route_table_find(route_table_t *pTable, uns64 uiId, uns64 *puiRoute)
{
    uns64_range_t range;
    route_table_entry_t *pRte;
    uns64 uiRoute;

    range.lo = uiId;
    range.hi = uiId;
    if (uiId < pTable->m_uiLo)
        return -1;
    if (uiId > pTable->m_uiHi)
        return -1;
    pRte = (route_table_entry_t *)vtree_lookup(&pTable->m_tree, &range);
    uiRoute = pRte ? pRte->m_uiRoute : (uns64)-1;
    if (uiRoute == 0)
        return -1;
    *puiRoute = uiRoute;
    return 0;
}

// Zero means the two ranges OVERLAP, which is what makes a point lookup find
// its containing range.
int uns64_range_t_cmp(const uns64_range_t *pA, const uns64_range_t *pB)
{
    if (pA->hi < pB->lo)
        return -1;
    return pB->hi < pA->lo;
}
