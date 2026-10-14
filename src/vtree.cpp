#line 1 "C:/code-frontiers-gm/drdp/vtree.cpp"
// vtree.cpp -- SUPPORT 0x004beb10..0x004bf970.
//
// The name is not a guess: `_uassert`'s second argument in this file is the
// string "C:/code-frontiers-gm/drdp/vtree.cpp", which `eqcalls.py` reads out of
// the relocated dump.  Four asserts carry a LINE NUMBER as an immediate, so the
// source below is padded to the original's line layout -- 215, 283, 331, 365.
//
// A CLRS red-black tree, intrusive the same way vlist is, with two design
// decisions that shape every function: the NIL sentinel is a real node living
// INSIDE the tree object (at +0x14), so no walk needs a null test; and the
// children are an ARRAY indexed by direction, so insert, remove, rotate and
// both fixups are each written ONCE and reached with `child[dir]`.
#include "drdp.h"

#define VTREE_KEY(tree, node)  ((const void *)((char *)(node) + (tree)->key_offset))
// The unchecked form.  vtree_node() below tests for a null object because the
// caller's may be one; where the pointer has just come back from another vtree
// call it cannot be, and the target has no test there.
#define VTREE_NODE(tree, object) ((vnode_t *)((char *)(object) + (tree)->vnode_offset))

// A null object means the sentinel, which is what makes `vtree_insert_raw`'s
// first two instructions a branch rather than an add.
inline vnode_t *vtree_node(vtree_t *tree, void *object)
{
    if (object == 0)
        return &tree->nil;
    return (vnode_t *)((char *)object + tree->vnode_offset);
}

inline void *vtree_object(const vtree_t *tree, vnode_t *node)
{
    if (node != &tree->nil)
        return (void *)((char *)node - tree->vnode_offset);
    return 0;
}

void vtree_init(vtree_t *tree, int vnode_offset, vcmp_t cmp, int key_offset)
{
    tree->root = &tree->nil;
    tree->size = 0;
    tree->vnode_offset = vnode_offset;
    tree->cmp = cmp;
    tree->key_offset = key_offset - vnode_offset;
    tree->nil.colour = 0;
    tree->nil.parent = &tree->nil;
    tree->nil.child[0] = &tree->nil;
    tree->nil.child[1] = &tree->nil;
}

int vtree_get_size(vtree_t *tree)
{
    return tree->size;
}

int vtree_validate_node(vtree_t *tree, vnode_t *node)
{
    if (node->child[0] != &tree->nil)
    {
        if (node->child[0]->parent != node)
            return 0;
        if (!vtree_validate_node(tree, node->child[0]))
            return 0;
    }
    if (node->child[1] != &tree->nil)
    {
        if (node->child[1]->parent != node)
            return 0;
        if (!vtree_validate_node(tree, node->child[1]))
            return 0;
    }
    return 1;
}

// The right subtree's black height is computed and never compared: an array of
// two, so both stores survive.
int vtree_get_bh(vtree_t *tree, vnode_t *node)
{
    if (node != &tree->nil)
    {
        int bh[2];

        bh[0] = vtree_get_bh(tree, node->child[0]);
        bh[1] = vtree_get_bh(tree, node->child[1]);
        return bh[0] + (node->child[0]->colour == 0);
    }
    return 0;
}

int vtree_validate(vtree_t *tree)
{
    if (tree->root->parent != &tree->nil)
        return 0;
    return vtree_validate_node(tree, tree->root) != 0;
}

int vtree_validate_bh(vtree_t *tree)
{
    if (tree->root->parent != &tree->nil)
        return 0;
    if (!vtree_validate_node(tree, tree->root))
        return 0;
    vtree_get_bh(tree, tree->root);
    return 1;
}

void vtree_insert_raw(vtree_t *tree, void *object)
{
    vnode_t *node = vtree_node(tree, object);
    vnode_t *nil = &tree->nil;

    node->parent = nil;
    node->colour = 1;
    node->child[0] = nil;
    node->child[1] = nil;

    if (tree->root == nil)
    {
        tree->root = node;
        return;
    }

    vnode_t *p = tree->root;
    // A VARIABLE, not the constant: every spelling of `>= 0` -- including
    // `> -1` -- gives gcc's `nor`+`srl` pair, and only a comparison against a
    // register gives the target's `slt`.  Constant propagation then turns the
    // load into `li -1`, which is why the object code looks like a folded
    // constant that will not fold.
    int less = -1;

    for (;;)
    {
        int dir = tree->cmp(VTREE_KEY(tree, node), VTREE_KEY(tree, p)) > less;

        if (p->child[dir] == nil)
        {
            p->child[dir] = node;
            node->parent = p;
            break;
        }
        p = p->child[dir];
    }
}

void vtree_replace_x_with_y(vtree_t *tree, vnode_t *x, vnode_t *y)
{
    y->parent = x->parent;
    if (x->parent == &tree->nil)
        tree->root = y;
    else if (x == x->parent->child[0])
        x->parent->child[0] = y;
    else
        x->parent->child[1] = y;
}

void vtree_rotate(vtree_t *tree, vnode_t *x, int dir)
{
    vnode_t *y = x->child[dir ^ 1];
    vnode_t *c = y->child[dir];

    x->child[dir ^ 1] = c;
    if (c != &tree->nil)
        c->parent = x;
    vtree_replace_x_with_y(tree, x, y);
    y->child[dir] = x;
    x->parent = y;
}

void vtree_insert(vtree_t *tree, void *element)
{
    vnode_t *node = vtree_node(tree, element);
// -----------------------------------------------------------------------
// The layout, measured off vtree_init:
//   vtree_t   +0x00 root        +0x04 size      +0x08 vnode_offset
//             +0x0C cmp         +0x10 key_offset (RELATIVE to the node)
//             +0x14 nil         -- a whole vnode_t, sixteen bytes
//   vnode_t   +0x00 colour      +0x04 parent
//             +0x08 child[0]    +0x0C child[1]
// vtree_init points root, and all three of the sentinel's own links, at
// the sentinel -- so `root->parent == &nil` is the empty-tree test and
// every leaf's children are the sentinel rather than null.  The sentinel
// is BLACK (colour 0) and stays black; a new node is RED (colour 1).
// The key offset is stored as a DELTA from the node, exactly as vlist
// stores it, so a key is reached from a node without going back through
// the object: init computes `key_offset - vnode_offset` and every
// comparison adds the result to a node address.
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Insertion is the textbook two-step: an ordinary binary-search-tree
// insert that paints the new node red, then a walk back up repairing the
// red-red violations that creates.
// vtree_insert_raw is the first half and is a global symbol of its own,
// so a caller that has already placed a node can repair the tree without
// searching for it again.
// The comparison is `>= 0`, which gcc canonicalises to `-1 < r` and emits
// as `li -1` plus `slt` -- so the -1 in the object code is the
// compiler's, not the source's.  The result is a 0/1 value used directly
// as the child index, which is what collapses the two mirror descents
// into one loop.
// A duplicate key is inserted, not rejected: `>= 0` sends an equal key
// down the right spine.
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// The rotation pair.  vtree_replace_x_with_y is the half that re-parents,
// and it re-reads x->parent after writing y->parent -- the two may alias,
// so the second read cannot be forwarded.
// vtree_rotate names the child it is pivoting `y` and reaches it with
// `child[dir ^ 1]`, so `dir` is the direction the tree ROTATES, not the
// side the pivot comes from.
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Insertion, with the fixup written out.  The assert on the line below is
// the first of four in this file, and its line number is an immediate the
// differ compares, so the padding above is load-bearing.
// -----------------------------------------------------------------------
    VASSERT(element != 0);
    vtree_insert_raw(tree, element);
    while (tree->root != node && node->parent->colour != 0)
    {
        vnode_t *parent = node->parent;
        int d = (parent != parent->parent->child[0]);
        vnode_t *uncle = parent->parent->child[d ^ 1];

        if (uncle->colour != 0)
        {
            parent->colour = 0;
            uncle->colour = 0;
            node->parent->parent->colour = 1;
            node = node->parent->parent;
        }
        else
        {
            if (node == parent->child[d ^ 1])
            {
                node = parent;
                vtree_rotate(tree, node, d);
            }
            node->parent->colour = 0;
            node->parent->parent->colour = 1;
            vtree_rotate(tree, node->parent->parent, d ^ 1);
        }
    }
    tree->root->colour = 0;
    tree->size++;
}

int vtree_contains(vtree_t *tree, void *element)
{
    vnode_t *node = vtree_node(tree, element);
    // `d` is 0 when the parent is its own parent's LEFT child, and the
    // uncle is always the other side -- which is why one body serves both
    // mirror images of every case below.
        // Case 1: the uncle is red.  Repaint the parent and the uncle
        // black and the grandparent red, and carry the violation two
        // levels up.  Nothing rotates.
        //
        // Case 2 and 3: the uncle is black.  If this node is the INNER
        // grandchild, rotate the parent to make it the outer one (case
        // 2), then repaint and rotate the grandparent the other way
        // (case 3).  Case 3 always ends the loop.
    // The root is repainted black unconditionally -- it is the cheapest
    // way to close case 1 having walked all the way up.
//
// A membership test, and the only function in the file that reaches its
// own key through the caller's OBJECT: everything else is handed a key
// directly.  It answers through vtree_lookup rather than walking, so a
// tree whose comparator disagrees with pointer identity answers by the
// comparator.
//
// The delete fixup.  CLRS's four cases, again written once with `d`
// selecting the mirror, and again reaching the sibling through the
// direction array rather than naming left and right.
//
// It is a static: the symbol map has no name for it, and the gap it
// leaves is why vtree_contains needs a SPAN_CAP.
//
// The loop ends either by walking the double-black up to the root or by
// case 4, which fixes the violation outright and sets `node` to the root
// so the test fails.
//
// Either way the node it stops on is painted black.
//

    VASSERT(element != 0);
    return vtree_lookup(tree, VTREE_KEY(tree, node)) != 0;
}

static void vtree_remove_fixup(vtree_t *tree, vnode_t *node)
{
    while (node != tree->root && node->colour == 0)
    {
        int d = (node == node->parent->child[1]);
        vnode_t *sibling = node->parent->child[d ^ 1];

        if (sibling->colour != 0)
        {
            sibling->colour = 0;
            node->parent->colour = 1;
            vtree_rotate(tree, node->parent, d);
            sibling = node->parent->child[d ^ 1];
        }
        if (sibling->child[d]->colour == 0 && sibling->child[d ^ 1]->colour == 0)
        {
            sibling->colour = 1;
            node = node->parent;
        }
        else
        {
            if (sibling->child[d ^ 1]->colour == 0)
            {
                sibling->child[d]->colour = 0;
                sibling->colour = 1;
                vtree_rotate(tree, sibling, d ^ 1);
                sibling = node->parent->child[d ^ 1];
            }
            sibling->colour = node->parent->colour;
            node->parent->colour = 0;
            sibling->child[d ^ 1]->colour = 0;
            vtree_rotate(tree, node->parent, d);
            node = tree->root;
        }
    }
    node->colour = 0;
}

void *vtree_iterate(vtree_t *tree, void *element, int dir)
{
    vnode_t *node = vtree_node(tree, element);
    vnode_t *nil = &tree->nil;
// Successor/predecessor, both directions; the child walk re-spells `&tree->nil` (loop.c hoists it: `move a1,s1`).

    VASSERT(element != 0);
    if (node->child[dir] == nil)
    {
        vnode_t *p = node->parent;
        while (p != nil && p->child[dir] == node)
        {
            node = p;
            p = p->parent;
        }
        node = p;
    }
    else
    {
        node = node->child[dir];
        while (node->child[dir ^ 1] != &tree->nil)
            node = node->child[dir ^ 1];
    }
    return vtree_object(tree, node);
}
void *vtree_remove(vtree_t *tree, void *element)
{
    vnode_t *node = vtree_node(tree, element);
    vnode_t *nil = &tree->nil;
    vnode_t *y;
// CLRS delete: `y` is the node actually unlinked -- the target itself when
// it has at most one child, and its successor when it has two.  The
// successor is fetched by calling vtree_iterate TWICE with the same
// arguments, once to test it and once to use it.
// The fixup runs only when the unlinked node was BLACK, which is the one
// The successor is the leftmost node of the right subtree, and vtree_iterate
// finds it -- so remove needs no descent of its own.
//
// `x` is the child that takes the unlinked node's place.
// case that shortens a path.
    VASSERT(element != 0);
    if (node->child[0] == nil || node->child[1] == nil)
        y = node;
    else if (vtree_iterate(tree, vtree_object(tree, node), 1) == 0)
        y = nil;
    else
        y = VTREE_NODE(tree, vtree_iterate(tree, vtree_object(tree, node), 1));

    vnode_t *x = (y->child[0] != nil) ? y->child[0] : y->child[1];

    vtree_replace_x_with_y(tree, y, x);
    if (y->colour == 0)
        vtree_remove_fixup(tree, x);
    if (y != node)
    {
        y->child[0] = node->child[0];
        y->child[1] = node->child[1];
        y->colour = node->colour;
        vtree_replace_x_with_y(tree, node, y);
        node->child[0]->parent = y;
        node->child[1]->parent = y;
    }
    tree->size--;
    return element;
}

void *vtree_remove_by_key(vtree_t *tree, void *key)
{
    void *object = vtree_lookup(tree, key);

    if (object)
        vtree_remove(tree, object);
    return object;
}

void *vtree_lookup(const vtree_t *tree, const void *key)
{
    vnode_t *node = tree->root;

    while (node != (vnode_t *)&tree->nil)
    {
        int r = tree->cmp(key, VTREE_KEY(tree, node));

        if (r < 0)
            node = node->child[0];
        else if (r > 0)
            node = node->child[1];
        else
            break;
    }
    return vtree_object(tree, node);
}

// `&tree->nil` written OUT at every use, not held in a local: a named local is
// one pseudo that lives across the loop, where the target rematerialises the
// address three times.  The same edit closed vtree_lookup.
void *vtree_peek_end(vtree_t *tree, int dir)
{
    vnode_t *node = tree->root;

    if (node != &tree->nil)
    {
        while (node->child[dir] != &tree->nil)
            node = node->child[dir];
        return vtree_object(tree, node);
    }
    return 0;
}

void *vtree_peek_first(vtree_t *tree)
{
    return vtree_peek_end(tree, 0);
}

void *vtree_peek_last(vtree_t *tree)
{
    return vtree_peek_end(tree, 1);
}

void *vtree_remove_end(vtree_t *tree, int dir)
{
    void *object = vtree_peek_end(tree, dir);

    if (object)
        vtree_remove(tree, object);
    return object;
}

void *vtree_remove_first(vtree_t *tree)
{
    return vtree_remove_end(tree, 0);
}

void *vtree_remove_last(vtree_t *tree)
{
    return vtree_remove_end(tree, 1);
}

void *vtree_get_prev(vtree_t *tree, void *object)
{
    return vtree_iterate(tree, object, 0);
}

void *vtree_get_next(vtree_t *tree, void *object)
{
    return vtree_iterate(tree, object, 1);
}

int vtree_vnode_range_cmp(vtree_t *tree, void *lo, void *hi, vnode_t *node)
{
    int r = tree->cmp(lo, VTREE_KEY(tree, node));

    if (r > 0)
        return r;
    r = tree->cmp(hi, VTREE_KEY(tree, node));
    if (r < 0)
        r = -1;
    else
        r = 0;
    return r;
}

void *vtree_iterate_in_range(vtree_t *tree, void *lo, void *hi, void *object, int dir)
{
    void *next = vtree_iterate(tree, object, dir);

    if (next && vtree_vnode_range_cmp(tree, lo, hi, VTREE_NODE(tree, next)) != 0)
        next = 0;
    return next;
}

void *vtree_peek_end_in_range(vtree_t *tree, void *lo, void *hi, int dir)
{
    vnode_t *node = tree->root;

    while (node != &tree->nil)
    {
        int r = vtree_vnode_range_cmp(tree, lo, hi, node);

        if (r < 0)
        {
            node = node->child[0];
            continue;
        }
        if (r > 0)
        {
            node = node->child[1];
            continue;
        }

        void *object = vtree_object(tree, node);

        for (;;)
        {
            void *next = vtree_iterate_in_range(tree, lo, hi, object, dir);

            if (next == 0)
                break;
            object = next;
            node = VTREE_NODE(tree, object);
        }
        break;
    }
    return vtree_object(tree, node);
}

void *vtree_peek_first_in_range(vtree_t *tree, void *lo, void *hi)
{
    return vtree_peek_end_in_range(tree, lo, hi, 0);
}

void *vtree_peek_last_in_range(vtree_t *tree, void *lo, void *hi)
{
    return vtree_peek_end_in_range(tree, lo, hi, 1);
}

void *vtree_get_prev_in_range(vtree_t *tree, void *lo, void *hi, void *object)
{
    return vtree_iterate_in_range(tree, lo, hi, object, 0);
}

void *vtree_get_next_in_range(vtree_t *tree, void *lo, void *hi, void *object)
{
    return vtree_iterate_in_range(tree, lo, hi, object, 1);
}

void vtree_vnode_free(vtree_t *tree, vnode_t *node)
{
    if (node != &tree->nil)
    {
        vtree_vnode_free(tree, node->child[0]);
        vtree_vnode_free(tree, node->child[1]);
        free((char *)node - tree->vnode_offset);
    }
}

void vtree_free(vtree_t *tree)
{
    vtree_vnode_free(tree, tree->root);
}
