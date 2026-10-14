// VIMap<K, V> -- SUPPORT 0x003E4498.. for VIMap<unsigned,VIDictionary::VIDictEntry>.
//
// VIMultiMap with a Find, and VIDictionary's base class.  Four instantiations,
// the same source as viset.cpp and vimultimap.cpp with no re-tuning.
#include "vi.h"

#define NODE(i) ((Node *)((i) * sizeof(Node) + (unsigned int)m_nodes))

// Placement new, as in vipool.cpp.
inline void *operator new(unsigned int, void *p) throw()
{
    return p;
}

template <class K, class V> VIMap<K, V>::VIMap()
{
    m_size = 0;
    m_capacity = 0;
    m_nodes = 0;
    m_root = 0;
    m_free = -1;
}

template <class K, class V> VIMap<K, V>::~VIMap()
{
    if (m_nodes)
    {
        DestroyPool();
        delete [] (char *)m_nodes;
    }
}

// NOT `return m_size`: the sentinel is counted in m_size, so Size() discounts
// it and clamps -- an empty tree that has never allocated has m_size 0.
template <class K, class V> int VIMap<K, V>::Size() const
{
    int n = m_size - 1;
    if (n < 0)
        n = 0;
    return n;
}

// The sentinel index IS the past-the-end iterator.
template <class K, class V> int VIMap<K, V>::End() const
{
    return 0;
}

template <class K, class V> int VIMap<K, V>::Begin() const
{
    int i = m_root;
    if (i != 0)
    {
        while (NODE(i)->m_left != 0)
            i = NODE(i)->m_left;
    }
    return i;
}

// ONE return at the bottom, as VIMultiMap's (phase 61).
template <class K, class V> int VIMap<K, V>::Next(int i) const
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

// Two readers VIMultiMap has no symbols for.
// Returns a REFERENCE: the target's last instruction is `addiu v0,v0,16`, an
// address, not a load.
template <class K, class V> const K &VIMap<K, V>::Key(int i) const
{
    return m_nodes[i].m_key;
}

// Not found is End(), and it is a CALL: the `jal` at the bottom of the target
// takes no argument but `this`.
template <class K, class V> int VIMap<K, V>::Find(const K &k) const
{
    int i = m_root;
    while (i != 0)
    {
        if (k < NODE(i)->m_key)
            i = NODE(i)->m_left;
        else if (NODE(i)->m_key < k)
            i = NODE(i)->m_right;
        else
            return i;
    }
    return End();
}

template <class K, class V> int VIMap<K, V>::DestroyPool()
{
    for (int i = Begin(); i != End(); )
    {
        int j = i;
        i = Next(i);
        NODE(j)->m_value.~V();
    }
    return 0;
}

template <class K, class V> int VIMap<K, V>::LinkFreeChain(int i)
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

template <class K, class V> int VIMap<K, V>::Allocate()
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

template <class K, class V> int VIMap<K, V>::Clear()
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

template <class K, class V> int VIMap<K, V>::Init(int n)
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
template <class K, class V> int VIMap<K, V>::FreeNode(int i)
{
    NODE(i)->m_value.~V();

    NODE(i)->m_parent = m_free;
    NODE(i)->m_left = -2;
    m_free = i;
    m_size--;
    return 0;
}

template <class K, class V> int VIMap<K, V>::AddNode(int p, int side)
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

template <class K, class V> int VIMap<K, V>::LeftRotate(int x)
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

template <class K, class V> int VIMap<K, V>::RightRotate(int x)
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

template <class K, class V> int VIMap<K, V>::Insert(const K &k, const V &v)
{
    int p = 0;
    int side = 0;
    int c = m_root;

    if (c != 0)
    {
        // `p = c` at the TOP of the walk: reorg copies it into the latch's
        // bnezl, and sched1 sees only nodes/24/key in the preheader (a
        // latch `p = c` puts the copy in the preheader, where its zero
        // register weight schedules it ahead of the `li 24`).
        // The node array and the key are LOCALS, read once before the walk:
        // spelled as m_nodes / the reference inside the loop, gcse's PRE
        // hoists both loads to just after NOTE_INSN_LOOP_BEG, the loop starts
        // with an insn rather than its entry jump, loop.c calls it "phony",
        // and the `li 20` stays inside.  With nothing for PRE to move, loop.c
        // takes the loop and hoists the constant as the target does.
        Node *nodes = m_nodes;
        K key = k;
        do
        {
            p = c;
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
                return p;
            }
        } while (c != 0);
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

    // The fix-up REUSES the walk's variables: `p` is the parent and `c` the
    // uncle (the original's two function-scope ints), and that is
    // global-alloc's input, not style.  `p` carries AddNode's argument copy,
    // a COPY preference for a1 that beats the plain a2 preference the
    // post-rotate `p * sizeof(Node)` gives it, and its long summed life puts
    // it below NODE(p) and the uncle's scale, so both of those avoid a1/a2
    // (regs_someone_prefers) and q*24 lands in a2 -- a fresh `int q` per
    // pass takes a2 itself (73 words).  `c` makes the uncle a GLOBAL pseudo:
    // a block-local uncle is local-alloc'd to v0 first, and the
    // INSN_SCHEDULING fake lifetime then pushes `&NODE(c)` to v1 (2 words).
    // The parent stays a variable rather than NODE(NODE(x)->m_parent): each
    // colour store would force the index to be re-loaded.
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

template <class K, class V> int VIMap<K, V>::EraseFixup(int x)
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
template <class K, class V> int VIMap<K, V>::Erase(int z)
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




#define VIMAP_COMMON(K, V, KV)                              \
    template int VIMap<K, V >::FreeNode(int);               \
    template int VIMap<K, V >::DestroyPool();               \
    template int VIMap<K, V >::LinkFreeChain(int);          \
    template int VIMap<K, V >::LeftRotate(int);             \
    template int VIMap<K, V >::RightRotate(int);            \
    template int VIMap<K, V >::AddNode(int, int);           \
    template int VIMap<K, V >::Clear();                     \
    template int VIMap<K, V >::Allocate();                  \
    template int VIMap<K, V >::Erase(int);                  \
    template int VIMap<K, V >::EraseFixup(int);             \
    template int VIMap<K, V >::Begin() const;               \
    template int VIMap<K, V >::End() const;                 \
    template int VIMap<K, V >::Next(int) const;             \
    template int VIMap<K, V >::Insert(const K &, const V &);

VIMAP_COMMON(unsigned int, VIDictionary::VIDictEntry, x)
template VIMap<unsigned int, VIDictionary::VIDictEntry>::VIMap();
template int VIMap<unsigned int, VIDictionary::VIDictEntry>::Init(int);
template int VIMap<unsigned int, VIDictionary::VIDictEntry>::Size() const;
template int VIMap<unsigned int, VIDictionary::VIDictEntry>::Find(const unsigned int &) const;

VIMAP_COMMON(unsigned int, int, x)
template VIMap<unsigned int, int>::VIMap();
template int VIMap<unsigned int, int>::Find(const unsigned int &) const;
template VIMap<unsigned int, int>::~VIMap();

VIMAP_COMMON(int, VIEESurfCacheInfo, x)
template VIMap<int, VIEESurfCacheInfo>::VIMap();
template int VIMap<int, VIEESurfCacheInfo>::Init(int);
template int VIMap<int, VIEESurfCacheInfo>::Size() const;
template const int &VIMap<int, VIEESurfCacheInfo>::Key(int) const;
template VIMap<int, VIEESurfCacheInfo>::~VIMap();

template VIMap<int, int>::VIMap();
template int VIMap<int, int>::LinkFreeChain(int);
template int VIMap<int, int>::LeftRotate(int);
template int VIMap<int, int>::RightRotate(int);
template int VIMap<int, int>::AddNode(int, int);
template int VIMap<int, int>::Clear();
template int VIMap<int, int>::Allocate();
template int VIMap<int, int>::DestroyPool();
template int VIMap<int, int>::Begin() const;
template int VIMap<int, int>::End() const;
template int VIMap<int, int>::Next(int) const;
template int VIMap<int, int>::Init(int);
template int VIMap<int, int>::Insert(const int &, const int &);
template int VIMap<int, int>::Find(const int &) const;
template VIMap<int, int>::~VIMap();


// ------------------------------------------------------------ VIDictionary --
// Nothing but a name for VIMap<unsigned, VIDictEntry>: every member forwards,
// and the two Finds are the only ones that read the node.
VIDictionary::VIDictionary()
{
}

// m_map's own destructor runs after this (`li a1,2`); the Clear is the body.
VIDictionary::~VIDictionary()
{
    Clear();
}

int VIDictionary::Init(int n)
{
    if (m_map.Init(n) < 0)
        return -1;
    return 0;
}

int VIDictionary::Size()
{
    return m_map.Size();
}

int VIDictionary::Clear()
{
    return m_map.Clear();
}

int VIDictionary::Add(VIResource &r, unsigned int id, VIResourceType type, int i)
{
    r.m_id = id;

    VIDictEntry e;
    e.m_index = i;
    e.m_type = type;

    if (m_map.Insert(id, e) < 0)
        return -1;
    return 0;
}

int VIDictionary::Remove(VIResource &r)
{
    int i = m_map.Find(r.m_id);
    if (i == m_map.End())
        return -1;
    if (m_map.Erase(i) < 0)
        return -1;
    return 0;
}

int VIDictionary::Find(unsigned int id, VIResourceType &type, int &i)
{
    int n = m_map.Find(id);
    if (n == m_map.End())
        return 0;
    // Through the inline Value() accessor: it hands back `m_nodes + n` as
    // (plus nodes offset), where `m_nodes[n]` written out folds the other way
    // round and reuses the offset register for the second address.
    type = m_map.Value(n).m_type;
    i = m_map.Value(n).m_index;
    return 1;
}

int VIDictionary::FindTyped(unsigned int id, VIResourceType type, int &i)
{
    int n = m_map.Find(id);
    if (n == m_map.End())
        return 0;

    // Phase 7's if-direction, and the entry is named through a POINTER: the
    // target reads the index as `4(a0)` off &m_value, not as `24(v1)` off the
    // node.
    VIDictEntry *e = &m_map.m_nodes[n].m_value;
    if (type == e->m_type)
    {
        i = e->m_index;
        return 1;
    }
    return 0;
}
