// vset.cpp -- SUPPORT 0x004be920..0x004beb10, the DRDP set.
//
// Seven one-line wrappers over vtree, and the one thing they add is OWNERSHIP:
// an element is malloc'd with sixteen bytes of slack, copied in, and the tree
// node lives in that slack -- which is why `vnode_offset` is the element size
// and the key offset is zero.
#include "drdp.h"

void vset_init(vset_t *set, vcmp_t cmp, int size)
{
    vtree_init(&set->tree, size, cmp, 0);
}

int vset_get_size(vset_t *set)
{
    return vtree_get_size(&set->tree);
}

int vset_contains(vset_t *set, const void *element)
{
    return vtree_lookup(&set->tree, element) != 0;
}

int vset_add(vset_t *set, const void *element)
{
    if (vset_contains(set, element))
        return 0;

    void *copy = malloc(set->tree.vnode_offset + 16);

    if (copy)
    {
        memcpy(copy, element, set->tree.vnode_offset);
        vtree_insert(&set->tree, copy);
        return 0;
    }
    return -2;
}

int vset_remove(vset_t *set, const void *element)
{
    void *object = vtree_lookup(&set->tree, element);

    if (object)
    {
        vtree_remove(&set->tree, object);
        free(object);
        return 0;
    }
    return -1;
}

void vset_remove_all(vset_t *set)
{
    void *object;

    while ((object = vtree_remove_first(&set->tree)) != 0)
        free(object);
}

int vset_remove_first(vset_t *set, void *element)
{
    void *object = vtree_remove_first(&set->tree);

    if (object)
    {
        memcpy(element, object, set->tree.vnode_offset);
        free(object);
        return 0;
    }
    return -1;
}
