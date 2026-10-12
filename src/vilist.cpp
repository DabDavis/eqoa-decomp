// VIList<T> -- SUPPORT 0x00409888.. for VIList<VISurface *>.
//
// The engine's SIXTH template, and the only container in it that is not laid
// over a block of slots: every node is its own `new`, the list keeps a head
// and a tail, and the iterator is the node ADDRESS carried in an `int` so the
// interface still looks like VIPool's.
//
//     +0x00 m_size      +0x04 m_head      +0x08 m_tail
//     Node { T m_data; Node *m_next; Node *m_prev; }
//
// Read off Add: `operator new` is asked for 12 bytes for a pointer element
// and 496 for VIMaterial, and the two link words land at 480/484 there, so
// the payload comes FIRST and sizeof(VIMaterial) is 480.
//
// `new Node` is NOT null-checked by gcc 2.95 (that is -fcheck-new), so the
// element's initialiser -- the copy for a pointer, the constructor call for a
// value -- lands ahead of the source's own `if (n == 0) return -1;`.
#include "vi.h"

class VISurface;
class VIPrimBuffer;

template <class T> VIList<T>::VIList()
{
    m_size = 0;
    m_head = 0;
    m_tail = 0;
}

template <class T> VIList<T>::~VIList()
{
    Clear();
}

template <class T> int VIList<T>::Size() const
{
    return m_size;
}

// VIPool's sentinel, NOT VISet's: the iterator here is a node address, so 0
// is not available and End() is 0x7FFFFFFF again.
template <class T> int VIList<T>::End() const
{
    return 0x7FFFFFFF;
}

// The head, or End() when there is none -- and the empty answer goes through
// End() rather than spelling 0x7FFFFFFF, which is why both are `jal`s.
template <class T> int VIList<T>::Begin() const
{
    if (m_head)
        return (int)m_head;
    return End();
}

template <class T> int VIList<T>::Next(int i) const
{
    Node *n = (Node *)i;

    if (n->m_next)
        return (int)n->m_next;
    return End();
}

template <class T> int VIList<T>::Clear()
{
    while (m_head)
    {
        Node *n = m_head->m_next;
        delete m_head;
        m_head = n;
    }
    // Phase 16's rule decides the order: the block's LAST store of a given
    // constant is the one hoisted to the front, and here that is m_tail.
    m_size = 0;
    m_head = 0;
    m_tail = 0;
    return 0;
}

template <class T> int VIList<T>::Init()
{
    if (Clear() < 0)
        return -1;
    return 0;
}

template <class T> int VIList<T>::Add()
{
    Node *n = new Node;
    if (n == 0)
        return -1;

    n->m_next = 0;
    n->m_prev = m_tail;
    if (m_tail)
        m_tail->m_next = n;
    m_tail = n;
    if (m_head == 0)
        m_head = n;

    m_size++;
    return (int)m_tail;
}

template <class T> int VIList<T>::Add(const T &v)
{
    Node *n = new Node;
    n->m_data = v;
    if (n == 0)
        return -1;

    n->m_next = 0;
    n->m_prev = m_tail;
    if (m_tail)
        m_tail->m_next = n;
    m_tail = n;
    if (m_head == 0)
        m_head = n;

    m_size++;
    return (int)m_tail;
}

template <class T> int VIList<T>::Erase(int i)
{
    Node *p = (Node *)i;
    Node *n = p->m_next;

    if (n)
        n->m_prev = p->m_prev;
    else
        m_tail = p->m_prev;

    if (p->m_prev)
        p->m_prev->m_next = p->m_next;
    else
        m_head = p->m_next;

    delete p;
    m_size--;

    if (n == 0)
        return End();
    return (int)n;
}


#define VILIST_POINTER(T)                          \
    template VIList<T>::VIList();                  \
    template VIList<T>::~VIList();                 \
    template int VIList<T>::Begin() const;         \
    template int VIList<T>::Next(int) const;       \
    template int VIList<T>::Clear();               \
    template int VIList<T>::Init();                \
    template int VIList<T>::Add(T const &);        \
    template int VIList<T>::Erase(int);

VILIST_POINTER(VISurface *)
VILIST_POINTER(VIPrimBuffer *)

#define VILIST_VALUE(T)                            \
    template VIList<T>::VIList();                  \
    template VIList<T>::~VIList();                 \
    template int VIList<T>::Size() const;          \
    template int VIList<T>::Clear();               \
    template int VIList<T>::Init();                \
    template int VIList<T>::Add();                 \
    template int VIList<T>::Erase(int);

VILIST_VALUE(VIMaterial)
VILIST_VALUE(VIActor)
template int VIList<VIActor>::Begin() const;
template int VIList<VIActor>::Next(int) const;

template VIList<VIScene::VISceneOccup>::VIList();
template VIList<VIScene::VISceneOccup>::~VIList();
template int VIList<VIScene::VISceneOccup>::Size() const;
template int VIList<VIScene::VISceneOccup>::Clear();
template int VIList<VIScene::VISceneOccup>::Init();
template int VIList<VIScene::VISceneOccup>::Add();
template int VIList<VIScene::VISceneOccup>::Erase(int);
