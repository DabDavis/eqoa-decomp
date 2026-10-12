// close_set.cpp -- SUPPORT 0x004aef90..0x004af420, the recently-closed set.
//
// When a connection goes away its (endpoint id, instance) pair stays here for
// two minutes, so a segment that arrives late for it is answered with a reset
// instead of being taken for the opening of a new one.  Every record is on TWO
// containers at once -- a tree keyed by the pair for the lookup and a list
// keyed by the expiry time for the sweep -- which is what makes
// `close_get_vnode_offset` and `close_get_vlink_offset` two functions.
//
// The record also carries the FILE and LINE that closed it: `close_set_insert`
// takes both, and nothing here ever reads them back.  They are for a debugger.
#include "drdp.h"

// Both comparators are the same three-way shape, and the tail if-converts:
// `result = -1; if (!(a < b)) result = 0;` is the `movz` the target ends on.
int close_key_t_cmp(const close_key_t *key1, const close_key_t *key2)
{
    int result;

    if (key1->id > key2->id)
        result = 1;
    else if (key1->id < key2->id)
        result = -1;
    else if (key1->instance > key2->instance)
        result = 1;
    else if (key1->instance < key2->instance)
        result = -1;
    else
        result = 0;
    return result;
}

void close_set_init(close_set_t *pSet)
{
    vtree_init(&pSet->tree, close_get_vnode_offset(0),
               (vcmp_t)close_key_t_cmp, close_get_key_offset(0));
    vlist_init(&pSet->list, close_get_vlink_offset(0),
               (vcmp_t)uns32_cmp_wrap, close_get_key_offset(1));
}

// The expiry is computed from the caller's timestamp, not from now: a record
// whose two minutes are already up is dropped rather than inserted -- and the
// guard is a NESTED if with one `return 0` at the bottom, not an early return.
// Written as `if (...) return 0;` gcc moves the malloc failure's `return -2`
// out of line to the epilogue, where the target keeps it inline as the
// fall-through of a branch-likely.
int close_set_insert(close_set_t *pSet, uns64 uiId, uns32 uiInstance, uns32 uiTime,
                     uns32 uiNow, const char *pFile, int iLine)
{
    uns32 uiExpire = uiTime + 120000;

    close_set_release_expired(pSet, uiNow);
    if ((int)(uiExpire - uiNow) >= 0)
    {
        close_t *pClose = (close_t *)malloc(sizeof(close_t));

        if (!pClose)
            return -2;

        pClose->key.id = uiId;
        pClose->key.instance = uiInstance;
        pClose->time = uiExpire;
        pClose->file = pFile;
        pClose->line = iLine;
        vlist_insert(&pSet->list, pClose);
        vtree_insert(&pSet->tree, pClose);
        drdp_format_endpoint1(pClose->key.id);
    }
    return 0;
}

// The list is sorted by expiry, so the sweep stops at the first record that is
// still alive.  The formatter's return value is thrown away here and in
// `insert`: it writes its static ring, and a trace macro read it.
void close_set_release_expired(close_set_t *pSet, uns32 uiNow)
{
    close_t *pClose;

    while ((pClose = (close_t *)vlist_peek_head(&pSet->list)) != 0
           && (int)(pClose->time - uiNow) <= 0)
    {
        drdp_format_endpoint1(pClose->key.id);
        vtree_remove(&pSet->tree, pClose);
        vlist_remove_head(&pSet->list);
        free(pClose);
    }
}

int close_set_lookup(close_set_t *pSet, uns64 uiId, uns32 uiInstance)
{
    close_key_t key;

    key.id = uiId;
    key.instance = uiInstance;
    return vtree_lookup(&pSet->tree, &key) != 0;
}

// Serialise and tear down in one pass -- the buffer is optional, and with a
// null one this is just the destructor.  The times go out as an OFFSET from
// `uiNow`, so the stream survives being restored on a machine whose clock is
// somewhere else entirely.
void close_set_destroy_buffer(close_set_t *pSet, buffer_t *pBuffer, uns32 uiNow)
{
    close_t *pClose;

    if (pBuffer)
        buffer_append_uns64(pBuffer, vlist_get_size(&pSet->list));

    while ((pClose = (close_t *)vlist_remove_head(&pSet->list)) != 0)
    {
        if (pBuffer)
        {
            buffer_append_uns64(pBuffer, pClose->key.id);
            buffer_append_uns32(pBuffer, pClose->key.instance);
            buffer_append_offset_uns32(pBuffer, pClose->time, uiNow);
        }
        free(pClose);
    }
    vtree_init(&pSet->tree, close_get_vnode_offset(0),
               (vcmp_t)close_key_t_cmp, close_get_key_offset(0));
}

int close_set_create_buffer(close_set_t *pSet, buffer_t *pBuffer, uns32 uiNow)
{
    uns64 uiCount;
    uns64 i;

    buffer_read_uns64(pBuffer, &uiCount);
    for (i = 0; i < uiCount; i++)
    {
        close_t *pClose = (close_t *)malloc(sizeof(close_t));

        if (!pClose)
            return -2;

        buffer_read_uns64(pBuffer, &pClose->key.id);
        buffer_read_uns32(pBuffer, &pClose->key.instance);
        buffer_read_offset_uns32(pBuffer, &pClose->time, uiNow);
        vlist_insert(&pSet->list, pClose);
        vtree_insert(&pSet->tree, pClose);
    }
    return 0;
}

// The three offsets the two containers are wired up with.  `iWhich` is an
// index the library never passes anything but 0 or 1 -- the tree's node and
// key, then the list's link and key.
int close_get_vnode_offset(int iWhich)
{
    int iOffset = -1;

    if (iWhich == 0)
        iOffset = 0;
    return iOffset;
}

int close_get_vlink_offset(int iWhich)
{
    return iWhich == 0 ? 16 : -1;
}

int close_get_key_offset(int iWhich)
{
    int iOffset = -1;

    switch (iWhich)
    {
    case 0: iOffset = 24; break;
    case 1: iOffset = 40; break;
    }
    return iOffset;
}
