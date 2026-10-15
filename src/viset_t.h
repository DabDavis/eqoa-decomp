// viset_t.h -- VISet's template definitions, moved VERBATIM out of viset.cpp so that
// CLIENT's own copies of the template (modules/client/src) compile from the same source.
// viset.cpp includes it where the block stood, so the token stream it compiles is
// unchanged; the placement new is guarded because a CLIENT TU includes several of these.
#ifndef VI_VISET_T_H
#define VI_VISET_T_H

#define NODE(i) ((Node *)((i) * sizeof(Node) + (unsigned int)m_nodes))

// Placement new, as in vipool.cpp.
#ifndef VI_PLACEMENT_NEW
#define VI_PLACEMENT_NEW
inline void *operator new(unsigned int, void *p) throw()
{
    return p;
}
#endif

template <class T> VISet<T>::VISet()
{
    m_size = 0;
    m_capacity = 0;
    m_nodes = 0;
    m_root = 0;
    m_free = -1;
}

template <class T> VISet<T>::~VISet()
{
    if (m_nodes)
    {
        DestroyPool();
        delete [] (char *)m_nodes;
    }
}

template <class T> int VISet<T>::Size() const
{
    return m_size;
}

// The sentinel index IS the past-the-end iterator.
template <class T> int VISet<T>::End() const
{
    return 0;
}

template <class T> int VISet<T>::Begin() const
{
    int i = m_root;
    if (i != 0)
    {
        while (NODE(i)->m_left != 0)
            i = NODE(i)->m_left;
    }
    return i;
}

// ONE return at the bottom (phase 61): with the descent returning on its
// own, the ascent's `i = p` and the return collapse to one move.
template <class T> int VISet<T>::Next(int i) const
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

// Not found is End(), and it is a CALL: the `jal` at the bottom of the target
// takes no argument but `this`.
template <class T> int VISet<T>::Find(const T &v) const
{
    int i = m_root;
    while (i != 0)
    {
        if (v < NODE(i)->m_data)
            i = NODE(i)->m_left;
        else if (NODE(i)->m_data < v)
            i = NODE(i)->m_right;
        else
            return i;
    }
    return End();
}

template <class T> int VISet<T>::DestroyPool()
{
    for (int i = Begin(); i != End(); )
    {
        int j = i;
        i = Next(i);
        NODE(j)->m_data.~T();
    }
    return 0;
}

template <class T> int VISet<T>::LinkFreeChain(int i)
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

template <class T> int VISet<T>::Allocate()
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
            for (int i = 1; i < m_size; i++)
            {
                new (&n[i]) Node(m_nodes[i]);
            }
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

template <class T> int VISet<T>::Clear()
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

template <class T> int VISet<T>::Init(int n)
{
    if (Clear() < 0)
        return -1;
    m_capacity = n + 1;
    if (Allocate() < 0)
        return -1;
    return 0;
}

// The payload is DESTROYED here: for a POD the pseudo-destructor call
// vanishes, and it is what VIMultiMap<float,VIEmitterEntry> needs.
template <class T> int VISet<T>::FreeNode(int i)
{
    NODE(i)->m_data.~T();

    NODE(i)->m_parent = m_free;
    NODE(i)->m_left = -2;
    m_free = i;
    m_size--;
    return 0;
}

template <class T> int VISet<T>::AddNode(int p, int side)
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

template <class T> int VISet<T>::LeftRotate(int x)
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

template <class T> int VISet<T>::RightRotate(int x)
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

template <class T> int VISet<T>::Insert(const T &v)
{
    int side = 0;
    int p = 0;
    int c = m_root;

    if (c != 0)
    {
        Node *nodes = m_nodes;
        T key = v;
        // `p = c` at the TOP of the walk (see VIMap::Insert).
        do
        {
            p = c;
            if (key < nodes[p].m_data)
            {
                side = 1;
                c = nodes[p].m_left;
            }
            else if (nodes[p].m_data < key)
            {
                side = 0;
                c = nodes[p].m_right;
            }
            else
            {
                return p;
            }
        } while (c != 0);
    }

    int x = AddNode(p, side);
    if (x < 0)
        return -1;

    int n = x;

    // The null test on node+16 is gcc's own: placement new is declared
    // `throw()`, and gcc 2.95 null-checks a nothrow operator new's result.
    new (&NODE(n)->m_data) T(v);

    // The fix-up REUSES the walk's `p` (parent) and `c` (uncle): see
    // VIMap::Insert in vimap.cpp for the global-alloc reasoning.
    while (x != m_root && NODE(NODE(x)->m_parent)->m_color != 0)
    {
        p = NODE(x)->m_parent;
        if (p == NODE(NODE(p)->m_parent)->m_left)
        {
            c = NODE(NODE(p)->m_parent)->m_right;
            if (NODE(c)->m_color != 0)
            {
                NODE(p)->m_color = 0;
                NODE(c)->m_color = 0;
                NODE(NODE(p)->m_parent)->m_color = 1;
                x = NODE(p)->m_parent;
            }
            else
            {
                if (x == NODE(p)->m_right)
                {
                    x = p;
                    LeftRotate(x);
                }
                p = NODE(x)->m_parent;
                NODE(p)->m_color = 0;
                NODE(NODE(p)->m_parent)->m_color = 1;
                RightRotate(NODE(p)->m_parent);
            }
        }
        else
        {
            c = NODE(NODE(p)->m_parent)->m_left;
            if (NODE(c)->m_color != 0)
            {
                NODE(p)->m_color = 0;
                NODE(c)->m_color = 0;
                NODE(NODE(p)->m_parent)->m_color = 1;
                x = NODE(p)->m_parent;
            }
            else
            {
                if (x == NODE(p)->m_left)
                {
                    x = p;
                    RightRotate(x);
                }
                p = NODE(x)->m_parent;
                NODE(p)->m_color = 0;
                NODE(NODE(p)->m_parent)->m_color = 1;
                LeftRotate(NODE(p)->m_parent);
            }
        }
    }
    NODE(m_root)->m_color = 0;
    return n;
}

template <class T> int VISet<T>::EraseFixup(int x)
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
template <class T> int VISet<T>::Erase(int z)
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

#endif // VI_VISET_T_H
