// vipool_t.h -- VIPool's template definitions, moved VERBATIM out of vipool.cpp so that
// CLIENT's own copies of the template (modules/client/src) compile from the same source.
// vipool.cpp includes it where the block stood, so the token stream it compiles is
// unchanged; the placement new is guarded because a CLIENT TU includes several of these.
#ifndef VI_VIPOOL_T_H
#define VI_VIPOOL_T_H

// Placement new -- but unlike viarray.cpp and vivector.cpp, declared throw():
// gcc 2.95 null-checks the result of a NOTHROW operator new, and Allocate's
// copy loop is `new (&n[i]) Node(m_nodes[i])` WITH that check.  Written out
// by hand as `if (d) *d = m_nodes[i]` the two induction pointers come out in
// the other order (4 words in fifteen instantiations); declared throw() in
// the other template TUs it regresses them.
#ifndef VI_PLACEMENT_NEW
#define VI_PLACEMENT_NEW
inline void *operator new(unsigned int, void *p) throw()
{
    return p;
}
#endif

template <class T> VIPool<T>::VIPool()
{
    m_size = 0;
    m_capacity = 0;
    m_nodes = 0;
    m_free = -1;
    m_first = -1;
    m_last = -1;
}

template <class T> VIPool<T>::~VIPool()
{
    if (m_nodes)
    {
        DestroyPool();
        delete [] (char *)m_nodes;
    }
}

template <class T> int VIPool<T>::Size() const
{
    return m_size;
}

template <class T> int VIPool<T>::End() const
{
    return 0x7FFFFFFF;
}

template <class T> int VIPool<T>::Begin() const
{
    if (m_first == -1)
        return End();
    return m_first;
}

template <class T> int VIPool<T>::Next(int i) const
{
    if (m_nodes[i].m_next != -1)
        return m_nodes[i].m_next;
    return End();
}

template <class T> int VIPool<T>::Clear()
{
    if (m_nodes)
    {
        DestroyPool();
        delete [] (char *)m_nodes;
    }
    m_size = 0;
    m_capacity = 0;
    m_nodes = 0;
    m_free = -1;
    m_first = -1;
    m_last = -1;
    return 0;
}

template <class T> int VIPool<T>::DestroyPool()
{
    for (int i = Begin(); i != End(); i = Next(i))
        ;
    return 0;
}

template <class T> int VIPool<T>::LinkFreeChain(int i)
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
            m_nodes[j].m_next = j + 1;
            m_nodes[j].m_prev = -2;
            j++;
        }
        m_nodes[j].m_next = -1;
        m_nodes[j].m_prev = -2;
    }
    return 0;
}

template <class T> int VIPool<T>::Allocate()
{
    // A RAW char block, as in VIArray and VIVector -- invisible for every
    // POINTER instantiation, where the Node has no constructor and the two
    // spellings emit the same words, and worth 53 of 125 in
    // VIPool<VIHSpritePlayNode>, whose element carries a VIHSpriteFrame:
    // `new Node[n]` drags gcc's own vec-init loop in front of the copy.
    // VIPool<VIHSpritePlayNode> is the one instantiation the placement copy
    // does not close (44/89): its frame is copied with VIHSpriteFrame's
    // operator= shape, which the implicit copy constructor is not.
    // The whole body is NESTED under the capacity test, with one `return 0` at
    // the bottom.  Written flat -- `if (m_capacity == 0) return 0; ...` with
    // the closing test its own statement -- gcc if-converts that test to
    // `slti`+`movz` where the target branches on a copy of the call result.
    if (m_capacity)
    {
        if (m_nodes == 0)
        {
            m_nodes = (Node *)new char[m_capacity * sizeof(Node)];
            if (m_nodes == 0)
                return -1;
        }
        else
        {
            Node *n = (Node *)new char[m_capacity * sizeof(Node)];
            if (n == 0)
                return -1;
            for (int i = 0; i < m_size; i++)
                new (&n[i]) Node(m_nodes[i]);
            if (DestroyPool() < 0)
                return -1;
            delete [] (char *)m_nodes;
            m_nodes = n;
        }
        if (LinkFreeChain(m_size) < 0)
            return -1;
    }
    return 0;
}

template <class T> int VIPool<T>::Add(const T &v)
{
    if (m_free == -1)
    {
        if (m_capacity)
            m_capacity = m_capacity * 2;
        else
            m_capacity = 1;
        if (Allocate() < 0)
            return -1;
    }

    Node *n = (Node *)(m_free * sizeof(Node) + (unsigned int)m_nodes);
    if (n)
        n->m_data = v;

    m_nodes[m_free].m_prev = m_last;
    if (m_last != -1)
    {
        m_nodes[m_last].m_next = m_free;
    }
    else
    {
        m_first = m_free;
        m_last = m_free;
    }
    m_last = m_free;
    m_free = m_nodes[m_free].m_next;
    m_nodes[m_last].m_next = -1;
    m_size++;
    return m_last;
}

// Only the value instantiations have it: the pointer ones never ask for the
// element BEFORE a given one.
template <class T> int VIPool<T>::Prev(int i) const
{
    if (i != End())
        return m_nodes[i].m_prev;
    return m_last;
}

// The default-construct form of Add.  Same shape as Add(const T &) with the
// copy replaced by a placement new -- which is empty for a T that has no
// constructor, and is VIHSpriteFrame's identity pass for VIHSpritePlayNode.
template <class T> int VIPool<T>::Add()
{
    if (m_free == -1)
    {
        if (m_capacity)
            m_capacity = m_capacity * 2;
        else
            m_capacity = 1;
        if (Allocate() < 0)
            return -1;
    }

    // Phase 24's lever, and it reaches BACK: the node address is an integer
    // add with the offset on the left.  `m_nodes + m_free` canonicalises
    // base-first and left ONE `addu` wrong in all thirteen pointer
    // instantiations from phase 12 onwards -- the miss that phase recorded as
    // "Add is one word of operand order".
    Node *n = (Node *)(m_free * sizeof(Node) + (unsigned int)m_nodes);
    if (n)
        new (&n->m_data) T();

    m_nodes[m_free].m_prev = m_last;
    if (m_last != -1)
    {
        m_nodes[m_last].m_next = m_free;
    }
    else
    {
        m_first = m_free;
        m_last = m_free;
    }
    m_last = m_free;
    m_free = m_nodes[m_free].m_next;
    m_nodes[m_last].m_next = -1;
    m_size++;
    return m_last;
}

// Splice a new element in FRONT of i.  Only VIPool<VIEEBucket> has one, and
// the End() case is the whole of Add: an empty pool inserts at the tail.
template <class T> int VIPool<T>::Insert(const T &v, int i)
{
    // The sentinel spelled OUT, not End(): the target compares i against
    // `lui 0x7fff`/`ori 0xffff` with no call, where every other use of End()
    // in this file is a `jal` -- and Prev, four functions down, still calls it.
    if (i == 0x7FFFFFFF)
        return Add(v);

    if (m_free == -1)
    {
        if (m_capacity)
            m_capacity = m_capacity * 2;
        else
            m_capacity = 1;
        if (Allocate() < 0)
            return -1;
    }

    Node *n = (Node *)(m_free * sizeof(Node) + (unsigned int)m_nodes);
    if (n)
        n->m_data = v;

    int f = m_free;
    m_free = m_nodes[f].m_next;
    m_nodes[f].m_prev = m_nodes[i].m_prev;
    m_nodes[f].m_next = i;
    if (i != m_first)
        m_nodes[m_nodes[i].m_prev].m_next = f;
    else
        m_first = f;
    m_nodes[i].m_prev = f;
    m_size++;
    return f;
}

// Unlink i and re-append it at the tail -- the LRU touch, and the only reason
// VIPool<VIRasterMaterialLRU> exists.
template <class T> int VIPool<T>::MoveToLast(int i)
{
    if (m_nodes[i].m_next != -1)
        m_nodes[m_nodes[i].m_next].m_prev = m_nodes[i].m_prev;
    else
        m_last = m_nodes[i].m_prev;

    if (m_nodes[i].m_prev != -1)
        m_nodes[m_nodes[i].m_prev].m_next = m_nodes[i].m_next;
    else
        m_first = m_nodes[i].m_next;

    m_nodes[i].m_prev = m_last;
    if (m_last != -1)
    {
        m_nodes[m_last].m_next = i;
    }
    else
    {
        m_first = i;
        m_last = i;
    }
    m_last = i;
    m_nodes[i].m_next = -1;
    return i;
}

template <class T> int VIPool<T>::Erase(int i)
{
    int n = m_nodes[i].m_next;

    if (n != -1)
        m_nodes[n].m_prev = m_nodes[i].m_prev;
    else
        m_last = m_nodes[i].m_prev;

    if (m_nodes[i].m_prev != -1)
        m_nodes[m_nodes[i].m_prev].m_next = m_nodes[i].m_next;
    else
        m_first = m_nodes[i].m_next;

    m_nodes[i].m_next = m_free;
    m_nodes[i].m_prev = -2;
    m_free = i;
    m_size--;

    if (n == -1)
        return End();
    return n;
}

template <class T> int VIPool<T>::Init(int n)
{
    if (Clear() < 0)
        return -1;
    m_capacity = n;
    if (Allocate() < 0)
        return -1;
    return 0;
}

// The copy form.  The live chain is walked with the SOURCE pool's own
// Begin/Next/End and each node copied whole; the free chain is then rebuilt
// from the source's, stamping -2 into every m_prev as LinkFreeChain does.
template <class T> int VIPool<T>::Init(const VIPool<T> &a)
{
    if (Clear() < 0)
        return -1;
    m_capacity = a.m_capacity;
    if (Allocate() < 0)
        return -1;
    // ONE index for both walks, declared at function scope: the free-chain
    // loop keeps it in the same callee-saved register the live walk used.
    int i;
    for (i = a.Begin(); i != a.End(); i = a.Next(i))
    {
        Node *d = (Node *)(i * sizeof(Node) + (unsigned int)m_nodes);
        if (d)
            *d = *(Node *)(i * sizeof(Node) + (unsigned int)a.m_nodes);
    }
    m_size = a.m_size;
    m_free = a.m_free;
    m_first = a.m_first;
    m_last = a.m_last;
    for (i = a.m_free; i != -1; i = m_nodes[i].m_next)
    {
        m_nodes[i].m_next = a.m_nodes[i].m_next;
        m_nodes[i].m_prev = -2;
    }
    return 0;
}

#endif // VI_VIPOOL_T_H
