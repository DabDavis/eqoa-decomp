// vilist_t.h -- VIList's template definitions, moved VERBATIM out of vilist.cpp so that
// CLIENT's own copies of the template (modules/client/src) compile from the same source.
// vilist.cpp includes it where the block stood, so the token stream it compiles is
// unchanged; the placement new is guarded because a CLIENT TU includes several of these.
#ifndef VI_VILIST_T_H
#define VI_VILIST_T_H

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

// Before node i (End() = 0x7FFFFFFF appends).  No SUPPORT TU emits it; STATION's
// page and XML-parser TUs do (VIList<VIGUIObject *>, VIList<VIXMLCallback>).
template <class T> int VIList<T>::Insert(const T &v, int i)
{
    if (i == 0x7FFFFFFF)
        return Add(v);
    Node *p = (Node *)i;
    Node *n = new Node;
    n->m_data = v;
    if (n == 0)
        return -1;
    n->m_prev = p->m_prev;
    n->m_next = p;
    if (p != m_head)
        p->m_prev->m_next = n;
    else
        m_head = n;
    p->m_prev = n;
    m_size++;
    return 0;
}

#endif // VI_VILIST_T_H
