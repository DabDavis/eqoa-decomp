// VISet<T> -- SUPPORT 0x004ABBA0 .. 0x004ACAD4 for VISet<VIWnd *>.
//
// The engine's FOURTH template and its first ordered container: textbook CLRS
// red-black insertion and deletion, written over VIPool's index-addressed
// free list rather than over pointers.  Every link is an `int` index into one
// `new char[]` block, so a node keeps its identity across a reallocation --
// and slot 0 is the NIL sentinel, which is what makes End() zero and what
// makes AddNode's first growth step TWO slots.
//
//     +0x00 m_size      +0x04 m_capacity   +0x08 m_nodes
//     +0x0C m_root      +0x10 m_free
//
//     Node { int m_color; int m_parent; int m_left; int m_right; T m_data; }
//
// Read straight off FreeNode (stride 20, m_nodes at +8, -2 into m_left, the
// chain link in m_parent) and AddNode (colour 1 written into +0 on insert,
// both children zeroed).
//
// Phase 24's lever is everywhere in this class: the node address is an
// INTEGER add with the offset on the left, and m_nodes is re-read at every
// use because a store through one node may alias the pointer.  NODE() is a
// macro and not an inline member for exactly that reason -- it has to expand
// the member load fresh each time.
#include "vi.h"

class VIWnd;

#define NODE(i) ((Node *)((i) * sizeof(Node) + (unsigned int)m_nodes))

// Placement new, as in vipool.cpp.
inline void *operator new(unsigned int, void *p) throw()
{
    return p;
}

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
        i = Erase(i);
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

    if (m_root != 0)
    {
        p = m_root;
        for (;;)
        {
            int c;
            if (v < NODE(p)->m_data)
            {
                side = 1;
                c = NODE(p)->m_left;
            }
            else if (NODE(p)->m_data < v)
            {
                side = 0;
                c = NODE(p)->m_right;
            }
            else
            {
                return p;
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

    // The address is null-tested before it is written through, as in
    // VIPool::Add -- but the tested pointer here is node+16, not the node.
    T *d = &NODE(n)->m_data;
    if (d)
        new (d) T(v);

    // The parent is a LOCAL here and is NOT one in EraseFixup: with it, the
    // target keeps `q * sizeof(Node)` in one register across the whole body
    // and the two uncle-is-red arms CROSS-JOIN into a single tail.  Written
    // out as NODE(NODE(x)->m_parent) every time, each colour store forces the
    // index to be re-loaded -- the store may alias the very word it came
    // from -- and the arms stop being identical, which costs 20 words.
    while (x != m_root && NODE(NODE(x)->m_parent)->m_color != 0)
    {
        int q = NODE(x)->m_parent;
        if (q == NODE(NODE(q)->m_parent)->m_left)
        {
            int y = NODE(NODE(q)->m_parent)->m_right;
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
                q = NODE(x)->m_parent;
                NODE(q)->m_color = 0;
                NODE(NODE(q)->m_parent)->m_color = 1;
                RightRotate(NODE(q)->m_parent);
            }
        }
        else
        {
            int y = NODE(NODE(q)->m_parent)->m_left;
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

    // Not a conjunction: written `if (left && right) y = n; else y = z;` the
    // whole choice if-converts to one `movz`, where the target BRANCHES and
    // fills both delay slots with the two assignments.
    // CLRS's `y = (left == nil || right == nil) ? z : successor(z)`, and the
    // target BRANCHES where every conjunction and every two-armed spelling
    // if-converts to one `movz`/`movn`.  Left open at 34 words.
    int y;
    if (NODE(z)->m_left == 0)
        y = z;
    else if (NODE(z)->m_right != 0)
        y = n;
    else
        y = z;

    int x = NODE(y)->m_left;
    if (x == 0)
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


// VISet<VIWnd *>
template VISet<VIWnd *>::VISet();
template VISet<VIWnd *>::~VISet();
template int VISet<VIWnd *>::End() const;
template int VISet<VIWnd *>::Begin() const;
template int VISet<VIWnd *>::Next(int) const;
template int VISet<VIWnd *>::Find(VIWnd *const &) const;
template int VISet<VIWnd *>::DestroyPool();
template int VISet<VIWnd *>::LinkFreeChain(int);
template int VISet<VIWnd *>::Allocate();
template int VISet<VIWnd *>::FreeNode(int);
template int VISet<VIWnd *>::AddNode(int, int);
template int VISet<VIWnd *>::LeftRotate(int);
template int VISet<VIWnd *>::RightRotate(int);
template int VISet<VIWnd *>::Insert(VIWnd *const &);
template int VISet<VIWnd *>::EraseFixup(int);
template int VISet<VIWnd *>::Erase(int);
