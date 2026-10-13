// vlist.cpp -- SUPPORT 0x004be460..0x004be920, the DRDP sorted list.
//
// An intrusive doubly-linked list over objects that are NOT the nodes: the
// caller hands over an object pointer and the list adds `vnode_offset` to find
// the link words, so one object can sit in several containers at once.  The
// key offset is stored relative to the NODE, which is why init subtracts.
#include "drdp.h"

#define VLIST_NODE(list, object) ((vnode_t *)((char *)(object) + (list)->vnode_offset))
#define VLIST_KEY(list, node)    ((const void *)((char *)(node) + (list)->key_offset))

// Every accessor that hands a node back to the caller goes through this, and
// its null test is what `vlist_get_next` if-converts into a `movz`.
inline void *vlist_object(const vlist_t *list, vnode_t *node)
{
    if (node)
        return (void *)((char *)node - list->vnode_offset);
    return 0;
}

void vlist_init(vlist_t *list, int vnode_offset, vcmp_t cmp, int key_offset)
{
    list->cmp = cmp;
    list->head = 0;
    list->tail = 0;
    list->size = 0;
    list->vnode_offset = vnode_offset;
    list->key_offset = key_offset - vnode_offset;
}

void vlist_add_head(vlist_t *list, void *object)
{
    vnode_t *node = VLIST_NODE(list, object);

    node->next = list->head;
    node->prev = 0;
    list->head = node;
    if (list->tail == 0)
        list->tail = node;
    if (node->next)
        node->next->prev = node;
    if (node->prev)
        node->prev->next = node;
    list->size++;
}

void vlist_add_tail(vlist_t *list, void *object)
{
    vnode_t *node = VLIST_NODE(list, object);

    node->next = 0;
    node->prev = list->tail;
    list->tail = node;
    if (list->head == 0)
        list->head = node;
    if (node->next)
        node->next->prev = node;
    if (node->prev)
        node->prev->next = node;
    list->size++;
}

void vlist_replace(vlist_t *list, void *object, void *with)
{
    vnode_t *node = VLIST_NODE(list, object);
    vnode_t *n = VLIST_NODE(list, with);

    n->next = node->next;
    n->prev = node->prev;
    if (n->next == 0)
        list->tail = n;
    else
        n->next->prev = n;
    if (n->prev == 0)
        list->head = n;
    else
        n->prev->next = n;
}

void *vlist_remove_head(vlist_t *list)
{
    vnode_t *node = list->head;
    void *result;

    if (node)
    {
        list->head = node->next;
        if (list->head == 0)
            list->tail = 0;
        if (node->prev)
            node->prev->next = node->next;
        if (node->next)
            node->next->prev = node->prev;
        list->size--;
        result = vlist_object(list, node);
    }
    else
        result = 0;
    return result;
}

void *vlist_remove_tail(vlist_t *list)
{
    vnode_t *node = list->tail;
    void *result;

    if (node)
    {
        list->tail = node->prev;
        if (list->tail == 0)
            list->head = 0;
        if (node->prev)
            node->prev->next = node->next;
        if (node->next)
            node->next->prev = node->prev;
        list->size--;
        result = vlist_object(list, node);
    }
    else
        result = 0;
    return result;
}

void vlist_insert(vlist_t *list, void *object)
{
    vnode_t *node = VLIST_NODE(list, object);
    const void *key = VLIST_KEY(list, node);

    if (list->head == 0 || list->cmp(key, VLIST_KEY(list, list->head)) < 0)
    {
        vlist_add_head(list, object);
        return;
    }
    if (list->cmp(VLIST_KEY(list, list->tail), key) <= 0)
    {
        vlist_add_tail(list, object);
        return;
    }

    vnode_t *p = list->head->next;

    while (list->cmp(key, VLIST_KEY(list, p)) >= 0)
        p = p->next;
    node->next = p;
    node->prev = p->prev;
    if (p)
        p->prev = node;
    if (node->prev)
        node->prev->next = node;
    list->size++;
}

int vlist_contains(vlist_t *list, void *object)
{
    vnode_t *node = VLIST_NODE(list, object);
    vnode_t *p = list->head;

    while (p)
    {
        if (p == node)
            return 1;
        p = p->next;
    }
    return 0;
}

void *vlist_remove(vlist_t *list, void *object)
{
    vnode_t *node = VLIST_NODE(list, object);

    if (node == list->head)
        return vlist_remove_head(list);
    if (node == list->tail)
        return vlist_remove_tail(list);
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    list->size--;
    return vlist_object(list, node);
}

void *vlist_remove_by_key(vlist_t *list, void *key)
{
    void *object = vlist_lookup(list, key);

    if (object)
        vlist_remove(list, object);
    return 0;
}

void *vlist_lookup(const vlist_t *list, void *key)
{
    vnode_t *node = list->head;

    while (node)
    {
        int r = list->cmp(key, VLIST_KEY(list, node));

        if (r < 0)
        {
            node = 0;
            break;
        }
        if (r == 0)
            break;
        node = node->next;
    }
    return vlist_object(list, node);
}

int vlist_get_size(vlist_t *list)
{
    return list->size;
}

void *vlist_peek_head(vlist_t *list)
{
    return vlist_object(list, list->head);
}

void *vlist_peek_tail(vlist_t *list)
{
    return vlist_object(list, list->tail);
}

void *vlist_get_next(vlist_t *list, void *object)
{
    return vlist_object(list, VLIST_NODE(list, object)->next);
}

void *vlist_get_prev(vlist_t *list, void *object)
{
    return vlist_object(list, VLIST_NODE(list, object)->prev);
}
