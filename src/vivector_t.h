// vivector_t.h -- VIVector's template definitions, moved VERBATIM out of vivector.cpp so that
// CLIENT's own copies of the template (modules/client/src) compile from the same source.
// vivector.cpp includes it where the block stood, so the token stream it compiles is
// unchanged; the placement new is guarded because a CLIENT TU includes several of these.
#ifndef VI_VIVECTOR_T_H
#define VI_VIVECTOR_T_H

// Placement new, DECLARED `throw()` as in vipool.cpp: the allocation is a RAW
// char block and the elements are constructed into it one at a time, and the
// null test on each element is gcc 2.95's own -- it null-checks the result of
// a nothrow operator new.  Written out by hand as `if (d) *d = ...` instead,
// Allocate's two induction pointers come out in the other order.
#ifndef VI_PLACEMENT_NEW
#define VI_PLACEMENT_NEW
inline void *operator new(VI_SIZE_T, void *p) throw()
{
    return p;
}
#endif

template <class T> VIVector<T>::VIVector()
{
    m_size = 0;
    m_capacity = 0;
    m_data = 0;
}

template <class T> VIVector<T>::VIVector(const VIVector<T> &a)
{
    m_size = 0;
    m_capacity = 0;
    m_data = 0;
    Init(a);
}

template <class T> VIVector<T>::~VIVector()
{
    if (m_data)
    {
        DestroyPool();
        delete [] (char *)m_data;
    }
}

template <class T> int VIVector<T>::DestroyPool()
{
    int i = Begin();
    while (i != End())
    {
        int j = i;
        i = Next(i);
        m_data[j].~T();
    }
    return 0;
}

template <class T> int VIVector<T>::Clear()
{
    if (m_data)
    {
        if (DestroyPool() < 0)
            return -1;
        delete [] (char *)m_data;
    }
    // m_data = 0 is OUTSIDE the `if`, and the two branch-likely delay slots
    // say so: each holds a copy of it stolen from the head of the join block,
    // which reorg can only do when the join really is on both paths.  Phase
    // 13's hoist then puts it first, so it is the source's LAST statement.
    m_size = 0;
    m_capacity = 0;
    m_data = 0;
    return 0;
}

// The growth step.  Called with no block it just takes one; called with a
// block it takes a second, copy-constructs the live elements across, destroys
// and frees the first, and installs the second.
template <class T> int VIVector<T>::Allocate()
{
    if (m_capacity == 0)
        return 0;
    // ONE `return 0` at the bottom, shared by both arms -- Phase 12/13's
    // lever.  With a `return 0` written into the first arm gcc if-converts its
    // null test to `movn` and the two exits stop cross-jumping, which costs
    // this function four words at the top and three more at the bottom, where
    // the `DestroyPool() < 0` test stops reaching the shared epilogue.
    if (m_data == 0)
    {
        m_data = (T *)new char[m_capacity * sizeof(T)];
        if (m_data == 0)
            return -1;
    }
    else
    {
        T *p = (T *)new char[m_capacity * sizeof(T)];
        if (p == 0)
            return -1;
        for (int i = 0; i < m_size; i++)
            new (&p[i]) T(m_data[i]);
        if (DestroyPool() < 0)
            return -1;
        delete [] (char *)m_data;
        m_data = p;
    }
    return 0;
}

template <class T> int VIVector<T>::Init(int n)
{
    if (Clear() < 0)
        return -1;
    m_capacity = n;
    if (Allocate() < 0)
        return -1;
    return 0;
}

template <class T> int VIVector<T>::Init(const VIVector<T> &a)
{
    if (Clear() < 0)
        return -1;
    m_capacity = a.m_capacity;
    if (Allocate() < 0)
        return -1;
    for (int i = 0; i < a.m_size; i++)
    {
        T *p = (T *)(i * sizeof(T) + (unsigned int)m_data);
        new (p) T(a.m_data[i]);
    }
    m_size = a.m_size;
    return 0;
}

// Both Adds return the INDEX they added at -- Phase 24's VIClipStack::Push*
// idiom, and the same spelling: the old count is read into a local, the member
// is stepped, and the local is returned.
template <class T> int VIVector<T>::Add()
{
    if (m_size == m_capacity)
    {
        if (m_size == 0)
            m_capacity = 1;
        else
            m_capacity = m_size * 2;
        if (Allocate() < 0)
            return -1;
    }
    T *p = (T *)(m_size * sizeof(T) + (unsigned int)m_data);
    new (p) T();
    int n = m_size;
    m_size = n + 1;
    return n;
}

template <class T> int VIVector<T>::Add(const T &v)
{
    if (m_size == m_capacity)
    {
        if (m_size == 0)
            m_capacity = 1;
        else
            m_capacity = m_size * 2;
        if (Allocate() < 0)
            return -1;
    }
    T *p = (T *)(m_size * sizeof(T) + (unsigned int)m_data);
    new (p) T(v);
    int n = m_size;
    m_size = n + 1;
    return n;
}

#endif // VI_VIVECTOR_T_H
