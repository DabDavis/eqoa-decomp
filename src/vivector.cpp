// VIVector<T> -- the engine's THIRD template, and the only container in it
// that grows.
//
// SUPPORT scatters the four instantiations across five address ranges
// (0x00426960, 0x0043F318, 0x0044AFC0, 0x0044CF38, 0x0046D8A0, 0x004704F0),
// which is the same picture VIArray gave in Phase 24: every TU that holds a
// VIVector emits its own static copies of the pieces gcc chose not to make
// weak, so a named symbol's derived span swallows an unnamed twin.
#include "vi.h"

// Placement new, DECLARED `throw()` as in vipool.cpp: the allocation is a RAW
// char block and the elements are constructed into it one at a time, and the
// null test on each element is gcc 2.95's own -- it null-checks the result of
// a nothrow operator new.  Written out by hand as `if (d) *d = ...` instead,
// Allocate's two induction pointers come out in the other order.
inline void *operator new(unsigned int, void *p) throw()
{
    return p;
}

template <class T> VIVector<T>::VIVector()
{
    m_size = 0;
    m_capacity = 0;
    m_data = 0;
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


// ------------------------------------------------------------ instantiations

template VIVector<VIHSpriteNode>::VIVector();
template VIVector<VIHSpriteNode>::~VIVector();
template int VIVector<VIHSpriteNode>::DestroyPool();
template int VIVector<VIHSpriteNode>::Clear();
template int VIVector<VIHSpriteNode>::Allocate();
template int VIVector<VIHSpriteNode>::Init(int);
template int VIVector<VIHSpriteNode>::Init(const VIVector<VIHSpriteNode> &);
template int VIVector<VIHSpriteNode>::Add();

template VIVector<VIMatrix44>::VIVector();
template VIVector<VIMatrix44>::~VIVector();
template int VIVector<VIMatrix44>::DestroyPool();
template int VIVector<VIMatrix44>::Clear();
template int VIVector<VIMatrix44>::Allocate();
template int VIVector<VIMatrix44>::Init(int);
template int VIVector<VIMatrix44>::Init(const VIVector<VIMatrix44> &);
template int VIVector<VIMatrix44>::Add();

template VIVector<VISceneDisplayElem>::VIVector();
template VIVector<VISceneDisplayElem>::~VIVector();
template int VIVector<VISceneDisplayElem>::DestroyPool();
template int VIVector<VISceneDisplayElem>::Clear();
template int VIVector<VISceneDisplayElem>::Allocate();
template int VIVector<VISceneDisplayElem>::Init(int);
template int VIVector<VISceneDisplayElem>::Add(const VISceneDisplayElem &);

template int VIVector<int>::DestroyPool();
template int VIVector<int>::Clear();
template int VIVector<int>::Allocate();
template int VIVector<int>::Init(int);
template int VIVector<int>::Add(const int &);
