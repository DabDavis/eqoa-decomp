// VIMultiMap<K, V> -- SUPPORT 0x0046FAF8.. (float,int), 0x0047E198..
// (float,VIEmitterEntry) and the <unsigned,int> copy in the scene TU.
//
// VISet's tree with the key split out of the payload: the Node carries K then
// V after the four link words, and Insert's two placement news are what
// separate them.  Same source, same toolchain, no re-tuning -- see
// viset.cpp for the layout and for every lever used here.
#include "vi.h"

#define NODE(i) ((Node *)((i) * sizeof(Node) + (unsigned int)m_nodes))

// Placement new, as in vipool.cpp.
inline void *operator new(unsigned int, void *p) throw()
{
    return p;
}

// The largest key: the rightmost node.
template <class K, class V> int VIMultiMap<K, V>::Last() const
{
    int i = m_root;
    if (i != 0)
    {
        while (NODE(i)->m_right != 0)
            i = NODE(i)->m_right;
    }
    return i;
}

template <class K, class V> VIMultiMap<K, V>::VIMultiMap()
{
    m_size = 0;
    m_capacity = 0;
    m_nodes = 0;
    m_root = 0;
    m_free = -1;
}

template <class K, class V> VIMultiMap<K, V>::~VIMultiMap()
{
    if (m_nodes)
    {
        DestroyPool();
        delete [] (char *)m_nodes;
    }
}

// VIMap's Size, word for word: the sentinel is counted in m_size.
template <class K, class V> int VIMultiMap<K, V>::Size() const
{
    int n = m_size - 1;
    if (n < 0)
        n = 0;
    return n;
}

// The sentinel index IS the past-the-end iterator.
template <class K, class V> int VIMultiMap<K, V>::End() const
{
    return 0;
}

template <class K, class V> int VIMultiMap<K, V>::Begin() const
{
    int i = m_root;
    if (i != 0)
    {
        while (NODE(i)->m_left != 0)
            i = NODE(i)->m_left;
    }
    return i;
}

// ONE return at the bottom: with the descent returning on its own, the
// ascent's `i = p; return i;` collapses to a single move and the result
// leaves through the other register.
template <class K, class V> int VIMultiMap<K, V>::Next(int i) const
{
    if (NODE(i)->m_right != 0)
    {
        i = NODE(i)->m_right;
        while (NODE(i)->m_left != 0)
            i = NODE(i)->m_left;
    }
    else
    {
        int p = NODE(i)->m_parent;
        while (p != 0 && NODE(p)->m_right == i)
        {
            i = p;
            p = NODE(p)->m_parent;
        }
        i = p;
    }
    return i;
}

// Next mirrored, with End() as the way in from the back: Prev(End()) is the
// largest key, which is how VIScene drops the farthest entry.
template <class K, class V> int VIMultiMap<K, V>::Prev(int i) const
{
    if (i == End())
        return Last();

    if (NODE(i)->m_left != 0)
    {
        i = NODE(i)->m_left;
        while (NODE(i)->m_right != 0)
            i = NODE(i)->m_right;
    }
    else
    {
        // The step reads its node through a POINTER: as an rvalue the address
        // is expanded in order (li 24, mult, then m_nodes), where `NODE(p)->`
        // as a memory operand is an EXPAND_SUM address that loads m_nodes
        // FIRST -- and loop.c hoists the two invariants in that order.
        int p = NODE(i)->m_parent;
        while (p != 0 && NODE(p)->m_left == i)
        {
            i = p;
            Node *n = NODE(p);
            p = n->m_parent;
        }
        i = p;
    }
    return i;
}

// The value is destroyed a SECOND time here -- Erase has already run FreeNode
// on the same node -- and the index it uses is a COPY taken before the erase.
// Invisible for a POD; two calls and a stack frame for VIEmitterEntry.
template <class K, class V> int VIMultiMap<K, V>::DestroyPool()
{
    for (int i = Begin(); i != End(); )
    {
        int j = i;
        i = Next(i);
        NODE(j)->m_value.~V();
    }
    return 0;
}

template <class K, class V> int VIMultiMap<K, V>::LinkFreeChain(int i)
{
    m_free = i;
    if (i >= m_capacity)
    {
        m_free = -1;
    }
    else
    {
        int j = i;
        while (j < m_capacity - 1)
        {
            m_nodes[j].m_parent = j + 1;
            m_nodes[j].m_left = -2;
            j++;
        }
        m_nodes[j].m_parent = -1;
        m_nodes[j].m_left = -2;
    }
    return 0;
}

template <class K, class V> int VIMultiMap<K, V>::Allocate()
{
    if (m_capacity)
    {
        if (m_nodes == 0)
        {
            m_nodes = (Node *)new char[m_capacity * sizeof(Node)];
            if (m_nodes == 0)
                return -1;
            m_size++;
        }
        else
        {
            Node *n = (Node *)new char[m_capacity * sizeof(Node)];
            if (n == 0)
                return -1;
            // From ZERO here, where VISet's identical loop starts at 1: the
            // guard is `blez m_size` against VISet's `slt 1, m_size`, and the
            // walking pointer starts at n rather than at n + 1.
            for (int i = 0; i < m_size; i++)
                new (&n[i]) Node(m_nodes[i]);
            if (DestroyPool() < 0)
                return -1;
            delete [] (char *)m_nodes;
            m_nodes = n;
        }
        m_nodes[0].m_color = 0;
        m_nodes[0].m_left = 0;
        m_nodes[0].m_right = 0;
        m_nodes[0].m_parent = 0;
        if (LinkFreeChain(m_size > 0 ? m_size : 1) < 0)
            return -1;
    }
    return 0;
}

template <class K, class V> int VIMultiMap<K, V>::Clear()
{
    if (m_nodes)
    {
        if (DestroyPool() < 0)
            return -1;
        delete [] (char *)m_nodes;
    }
    m_size = 0;
    m_capacity = 0;
    m_nodes = 0;
    m_root = 0;
    m_free = -1;
    return 0;
}

template <class K, class V> int VIMultiMap<K, V>::Init(int n)
{
    if (Clear() < 0)
        return -1;
    m_capacity = n + 1;
    if (Allocate() < 0)
        return -1;
    return 0;
}

// The value is DESTROYED here: for a POD the pseudo-destructor call vanishes,
// and for VIEmitterEntry it is VIMatrix44's, inlined, on node+48.
template <class K, class V> int VIMultiMap<K, V>::FreeNode(int i)
{
    NODE(i)->m_value.~V();

    NODE(i)->m_parent = m_free;
    NODE(i)->m_left = -2;
    m_free = i;
    m_size--;
    return 0;
}

template <class K, class V> int VIMultiMap<K, V>::AddNode(int p, int side)
{
    if (m_free == -1)
    {
        if (m_capacity)
            m_capacity = m_capacity * 2;
        else
            m_capacity = 2;
        if (Allocate() < 0)
            return -1;
    }

    int i = m_free;
    m_free = NODE(i)->m_parent;
    NODE(i)->m_color = 1;
    NODE(i)->m_left = 0;
    NODE(i)->m_right = 0;
    NODE(i)->m_parent = p;

    if (p != 0)
    {
        if (side)
            NODE(p)->m_left = i;
        else
            NODE(p)->m_right = i;
    }
    else
    {
        m_root = i;
    }

    m_size++;
    return i;
}

template <class K, class V> int VIMultiMap<K, V>::LeftRotate(int x)
{
    int y = NODE(x)->m_right;

    NODE(x)->m_right = NODE(y)->m_left;
    if (NODE(y)->m_left != 0)
        NODE(NODE(y)->m_left)->m_parent = x;

    NODE(y)->m_parent = NODE(x)->m_parent;
    if (NODE(x)->m_parent == 0)
        m_root = y;
    else if (x == NODE(NODE(x)->m_parent)->m_left)
        NODE(NODE(x)->m_parent)->m_left = y;
    else
        NODE(NODE(x)->m_parent)->m_right = y;

    NODE(y)->m_left = x;
    NODE(x)->m_parent = y;
    return 0;
}

template <class K, class V> int VIMultiMap<K, V>::RightRotate(int x)
{
    int y = NODE(x)->m_left;

    NODE(x)->m_left = NODE(y)->m_right;
    if (NODE(y)->m_right != 0)
        NODE(NODE(y)->m_right)->m_parent = x;

    NODE(y)->m_parent = NODE(x)->m_parent;
    if (NODE(x)->m_parent == 0)
        m_root = y;
    else if (x == NODE(NODE(x)->m_parent)->m_left)
        NODE(NODE(x)->m_parent)->m_left = y;
    else
        NODE(NODE(x)->m_parent)->m_right = y;

    NODE(y)->m_right = x;
    NODE(x)->m_parent = y;
    return 0;
}

template <class K, class V> int VIMultiMap<K, V>::Insert(const K &k, const V &v)
{
    int p = 0;
    int side = 0;

    if (m_root != 0)
    {
        // The node array and the key are LOCALS, read once before the walk:
        // spelled as m_nodes / the reference inside the loop, gcse's PRE
        // hoists both loads to just after NOTE_INSN_LOOP_BEG, the loop starts
        // with an insn rather than its entry jump, loop.c calls it "phony",
        // and the `li 20` stays inside.  With nothing for PRE to move, loop.c
        // takes the loop and hoists the constant as the target does.
        Node *nodes = m_nodes;
        K key = k;
        p = m_root;
        for (;;)
        {
            int c;
            if (key < nodes[p].m_key)
            {
                side = 1;
                c = nodes[p].m_left;
            }
            else if (nodes[p].m_key < key)
            {
                side = 0;
                c = nodes[p].m_right;
            }
            else
            {
                // A MULTIMAP: an equal key goes right like a greater one, no
                // early return.  The second compare is still emitted (sltu /
                // c.lt.s with nothing reading it) because the two arms are
                // only merged after the compare has been expanded.
                side = 0;
                c = nodes[p].m_right;
            }
            if (c == 0)
                break;
            p = c;
        }
    }

    int x = AddNode(p, side);
    if (x < 0)
        return -1;

    int n = x;

    // Placement new DECLARED `throw()`: gcc 2.95 null-tests a nothrow
    // operator new's result, so each element is null-tested before the copy
    // is written through it -- node+16, then node+20, never the node.
    new (&NODE(n)->m_key) K(k);
    new (&NODE(n)->m_value) V(v);

    // The parent is a LOCAL here and is NOT one in EraseFixup: with it, the
    // target keeps `q * sizeof(Node)` in one register across the whole body
    // and the two uncle-is-red arms CROSS-JOIN into a single tail.  Written
    // out as NODE(NODE(x)->m_parent) every time, each colour store forces the
    // index to be re-loaded -- the store may alias the very word it came
    // from -- and the arms stop being identical, which costs 20 words.
    while (x != m_root && NODE(NODE(x)->m_parent)->m_color != 0)
    {
        int q = NODE(x)->m_parent;
        // STEPPING STONE, not believed to be the original spelling: the
        // uncle `y` is one variable for the loop body and is REUSED below for
        // the first half's post-rotate parent.  That asymmetry is what moves
        // global-alloc's priorities so q lands in a1 (q stays the post-rotate
        // parent in the second half, which is now byte-exact).  Spelled
        // symmetrically -- q, or a fresh local, in both halves -- the arm is
        // 73 words wrong instead of 24, with q in a2 by its preference for
        // the post-rotate `q * sizeof(Node)` pseudo.  What is left is
        // register choice at the loop top and in the first half (nodes,
        // q*20, NODE(q), the uncle's scale) plus the preheader's li/move
        // pair, which sched1 orders by register weight.
        int y;
        if (q == NODE(NODE(q)->m_parent)->m_left)
        {
            y = NODE(NODE(q)->m_parent)->m_right;
            if (NODE(y)->m_color != 0)
            {
                NODE(q)->m_color = 0;
                NODE(y)->m_color = 0;
                NODE(NODE(q)->m_parent)->m_color = 1;
                x = NODE(q)->m_parent;
            }
            else
            {
                if (x == NODE(q)->m_right)
                {
                    x = q;
                    LeftRotate(x);
                }
                y = NODE(x)->m_parent;
                NODE(y)->m_color = 0;
                NODE(NODE(y)->m_parent)->m_color = 1;
                RightRotate(NODE(y)->m_parent);
            }
        }
        else
        {
            y = NODE(NODE(q)->m_parent)->m_left;
            if (NODE(y)->m_color != 0)
            {
                NODE(q)->m_color = 0;
                NODE(y)->m_color = 0;
                NODE(NODE(q)->m_parent)->m_color = 1;
                x = NODE(q)->m_parent;
            }
            else
            {
                if (x == NODE(q)->m_left)
                {
                    x = q;
                    RightRotate(x);
                }
                q = NODE(x)->m_parent;
                NODE(q)->m_color = 0;
                NODE(NODE(q)->m_parent)->m_color = 1;
                LeftRotate(NODE(q)->m_parent);
            }
        }
    }
    NODE(m_root)->m_color = 0;
    return n;
}

template <class K, class V> int VIMultiMap<K, V>::EraseFixup(int x)
{
    while (x != m_root && NODE(x)->m_color == 0)
    {
        if (x == NODE(NODE(x)->m_parent)->m_left)
        {
            int w = NODE(NODE(x)->m_parent)->m_right;
            if (NODE(w)->m_color != 0)
            {
                NODE(w)->m_color = 0;
                NODE(NODE(x)->m_parent)->m_color = 1;
                LeftRotate(NODE(x)->m_parent);
                w = NODE(NODE(x)->m_parent)->m_right;
            }
            if (NODE(NODE(w)->m_left)->m_color == 0 && NODE(NODE(w)->m_right)->m_color == 0)
            {
                NODE(w)->m_color = 1;
                x = NODE(x)->m_parent;
            }
            else
            {
                if (NODE(NODE(w)->m_right)->m_color == 0)
                {
                    NODE(NODE(w)->m_left)->m_color = 0;
                    NODE(w)->m_color = 1;
                    RightRotate(w);
                    w = NODE(NODE(x)->m_parent)->m_right;
                }
                NODE(w)->m_color = NODE(NODE(x)->m_parent)->m_color;
                NODE(NODE(x)->m_parent)->m_color = 0;
                NODE(NODE(w)->m_right)->m_color = 0;
                LeftRotate(NODE(x)->m_parent);
                x = m_root;
            }
        }
        else
        {
            int w = NODE(NODE(x)->m_parent)->m_left;
            if (NODE(w)->m_color != 0)
            {
                NODE(w)->m_color = 0;
                NODE(NODE(x)->m_parent)->m_color = 1;
                RightRotate(NODE(x)->m_parent);
                w = NODE(NODE(x)->m_parent)->m_left;
            }
            if (NODE(NODE(w)->m_right)->m_color == 0 && NODE(NODE(w)->m_left)->m_color == 0)
            {
                NODE(w)->m_color = 1;
                x = NODE(x)->m_parent;
            }
            else
            {
                if (NODE(NODE(w)->m_left)->m_color == 0)
                {
                    NODE(NODE(w)->m_right)->m_color = 0;
                    NODE(w)->m_color = 1;
                    LeftRotate(w);
                    w = NODE(NODE(x)->m_parent)->m_left;
                }
                NODE(w)->m_color = NODE(NODE(x)->m_parent)->m_color;
                NODE(NODE(x)->m_parent)->m_color = 0;
                NODE(NODE(w)->m_left)->m_color = 0;
                RightRotate(NODE(x)->m_parent);
                x = m_root;
            }
        }
    }
    NODE(x)->m_color = 0;
    return 0;
}

// The splice form of RB-DELETE: y's LINKS are moved onto z's position rather
// than z's payload being overwritten, so the caller's index stays valid.
template <class K, class V> int VIMultiMap<K, V>::Erase(int z)
{
    int n = Next(z);

    // CLRS's `y = (left == nil || right == nil) ? z : successor(z)`, spelled
    // as the DISJUNCTION with `y = z` in the then-arm.  `||` expands to
    // "if (left == 0) goto Lthen; if (right != 0) goto Lelse; Lthen: y = z",
    // and that label between the second jump and `y = z` is what keeps
    // jump.c from if-converting the choice: every conjunction, the else-if
    // chain and every `y = z` default all fold to `movz`/`movn`.
    int y;
    if (NODE(z)->m_left == 0 || NODE(z)->m_right == 0)
        y = z;
    else
        y = n;

    // CLRS again, both arms written out: `x = left; if (x == 0) x = right;`
    // lets cse reuse the 20 the NODE(y) index was scaled by, where the target
    // loads a fresh one for NODE(x).
    int x;
    if (NODE(y)->m_left != 0)
        x = NODE(y)->m_left;
    else
        x = NODE(y)->m_right;

    NODE(x)->m_parent = NODE(y)->m_parent;
    if (NODE(y)->m_parent == 0)
        m_root = x;
    else if (y == NODE(NODE(y)->m_parent)->m_left)
        NODE(NODE(y)->m_parent)->m_left = x;
    else
        NODE(NODE(y)->m_parent)->m_right = x;

    if (NODE(y)->m_color == 0)
        EraseFixup(x);

    if (y != z)
    {
        NODE(y)->m_parent = NODE(z)->m_parent;
        NODE(y)->m_left = NODE(z)->m_left;
        NODE(y)->m_right = NODE(z)->m_right;
        NODE(y)->m_color = NODE(z)->m_color;
        if (NODE(z)->m_parent == 0)
            m_root = y;
        else if (NODE(NODE(z)->m_parent)->m_left == z)
            NODE(NODE(z)->m_parent)->m_left = y;
        else
            NODE(NODE(z)->m_parent)->m_right = y;
        NODE(NODE(z)->m_left)->m_parent = y;
        NODE(NODE(z)->m_right)->m_parent = y;
    }

    FreeNode(z);
    return n;
}

// Phase 40, for VIEESurfCache::FindFree.  A REFERENCE, as VIMap's is.
template <class K, class V> const K &VIMultiMap<K, V>::Key(int i) const
{
    return m_nodes[i].m_key;
}

// The leftmost node whose key is >= k, or End() when there is none.  The walk
// keeps the LAST node it stood on rather than the last non-null child, so it
// ends on the leaf nearest k from either side and one comparison decides
// which; an equal key takes the LEFT child, which is what makes it the LOWER
// bound of a run of equal keys rather than any of them.
template <class K, class V> int VIMultiMap<K, V>::LowerBound(const K &k) const
{
    int i = 0;
    int j = m_root;
    while (j != 0)
    {
        i = j;
        if (k < NODE(i)->m_key)
            j = NODE(i)->m_left;
        else if (NODE(i)->m_key < k)
            j = NODE(i)->m_right;
        else
            j = NODE(i)->m_left;
    }
    if (i != 0)
        if (NODE(i)->m_key < k)
            return Next(i);
    return i;
}


// VIMultiMap<unsigned int, int>
template VIMultiMap<unsigned int, int>::VIMultiMap();
template int VIMultiMap<unsigned int, int>::FreeNode(int);
template int VIMultiMap<unsigned int, int>::DestroyPool();
template int VIMultiMap<unsigned int, int>::LinkFreeChain(int);
template int VIMultiMap<unsigned int, int>::LeftRotate(int);
template int VIMultiMap<unsigned int, int>::RightRotate(int);
template int VIMultiMap<unsigned int, int>::AddNode(int, int);
template int VIMultiMap<unsigned int, int>::Clear();
template int VIMultiMap<unsigned int, int>::Allocate();
template int VIMultiMap<unsigned int, int>::Erase(int);
template int VIMultiMap<unsigned int, int>::Insert(const unsigned int &, const int &);
template int VIMultiMap<unsigned int, int>::EraseFixup(int);
template const unsigned int &VIMultiMap<unsigned int, int>::Key(int) const;
template int VIMultiMap<unsigned int, int>::LowerBound(const unsigned int &) const;
template VIMultiMap<unsigned int, int>::~VIMultiMap();

// VIMultiMap<float, int>
template int VIMultiMap<float, int>::FreeNode(int);
template int VIMultiMap<float, int>::DestroyPool();
template int VIMultiMap<float, int>::LinkFreeChain(int);
template int VIMultiMap<float, int>::LeftRotate(int);
template int VIMultiMap<float, int>::RightRotate(int);
template int VIMultiMap<float, int>::AddNode(int, int);
template int VIMultiMap<float, int>::Init(int);
template int VIMultiMap<float, int>::Clear();
template int VIMultiMap<float, int>::Allocate();
template int VIMultiMap<float, int>::Erase(int);
template int VIMultiMap<float, int>::Insert(const float &, const int &);
template int VIMultiMap<float, int>::EraseFixup(int);
template int VIMultiMap<float, int>::Size() const;
template int VIMultiMap<float, int>::Last() const;
template int VIMultiMap<float, int>::Prev(int) const;

// VIMultiMap<float, VIEmitterEntry>
template VIMultiMap<float, VIEmitterEntry>::VIMultiMap();
template int VIMultiMap<float, VIEmitterEntry>::FreeNode(int);
template int VIMultiMap<float, VIEmitterEntry>::DestroyPool();
template int VIMultiMap<float, VIEmitterEntry>::LinkFreeChain(int);
template int VIMultiMap<float, VIEmitterEntry>::LeftRotate(int);
template int VIMultiMap<float, VIEmitterEntry>::RightRotate(int);
template int VIMultiMap<float, VIEmitterEntry>::AddNode(int, int);
template int VIMultiMap<float, VIEmitterEntry>::Init(int);
template int VIMultiMap<float, VIEmitterEntry>::Clear();
template int VIMultiMap<float, VIEmitterEntry>::Allocate();
template int VIMultiMap<float, VIEmitterEntry>::Erase(int);
template int VIMultiMap<float, VIEmitterEntry>::Insert(const float &, const VIEmitterEntry &);
template int VIMultiMap<float, VIEmitterEntry>::EraseFixup(int);
template int VIMultiMap<float, VIEmitterEntry>::Size() const;
template VIMultiMap<float, VIEmitterEntry>::~VIMultiMap();
