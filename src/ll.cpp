// ll.cpp -- SUPPORT 0x004acb50..0x004acc90, the DRDP intrusive list.
//
// Amiga Exec's list verbatim, down to the header that overlaps two nodes so
// that neither end of a walk needs a special case.  Phase 11 found the engine's
// own VILList to be the same design; this is the network library's copy.
#include "drdp.h"

// The map orders this file remove / insert_after / init / add_head / add_tail /
// rem_head / rem_tail, so that is the source order.
void ll_remove(lnode *node)
{
    if (node->ln_succ && node->ln_pred)
    {
        node->ln_pred->ln_succ = node->ln_succ;
        node->ln_succ->ln_pred = node->ln_pred;
        node->ln_succ = 0;
        node->ln_pred = 0;
    }
}

// A node whose successor is null IS the tail sentinel, so inserting after it
// means inserting after its predecessor.
void ll_insert_after(lnode *node, lnode *after)
{
    if (after->ln_succ == 0)
        after = after->ln_pred;
    lnode *next = after->ln_succ;

    node->ln_pred = after;
    node->ln_succ = next;
    next->ln_pred = node;
    node->ln_pred->ln_succ = node;
}

void ll_init(llist *list)
{
    list->lh_head = (lnode *)&list->lh_tail;
    list->lh_tail = 0;
    list->lh_tailpred = (lnode *)list;
}

void ll_add_head(llist *list, lnode *node)
{
    if (node)
    {
        node->ln_pred = (lnode *)list;
        node->ln_succ = list->lh_head;
        node->ln_succ->ln_pred = node;
        list->lh_head = node;
    }
}

void ll_add_tail(llist *list, lnode *node)
{
    if (node)
    {
        lnode *tailpred = list->lh_tailpred;

        node->ln_succ = (lnode *)&list->lh_tail;
        node->ln_pred = tailpred;
        node->ln_pred->ln_succ = node;
        list->lh_tailpred = node;
    }
}

lnode *ll_rem_head(llist *list)
{
    lnode *node = list->lh_head;
    lnode *result = 0;

    if (node->ln_succ)
    {
        result = node;
        ll_remove(node);
    }
    return result;
}

lnode *ll_rem_tail(llist *list)
{
    lnode *node = list->lh_tailpred;
    lnode *result = 0;

    if (node->ln_pred)
    {
        result = node;
        ll_remove(node);
    }
    return result;
}
