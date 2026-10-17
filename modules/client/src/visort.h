// visort.h (name ours) -- the sort VICharacter.cpp's FlattenChatMenus inlines twice.
//
// Written for this decomp from the object code; the names and the text are ours.  Its
// SHAPE is the STL sort of the late 1990s as Microsoft's C++ library shipped it (HP's 1994
// STL by way of Dinkumware): a quicksort loop that recurses into the smaller half and
// stops at 16 elements, a median of three taken BY VALUE, and an insertion sort to finish.
// The shape is load-bearing, not the names:
//   - every helper is an inline template, and gcc 2.95 inlines each one where it is called;
//   - the by-value median is what leaves the pivot's three stack copies and its round trip
//     through a register;
//   - the value type travels as a null `T *` tag, which is how the loop and the swap
//     declare their temporaries;
//   - the loop calls itself, so its recursive calls go to its out-of-line copy (CLIENT
//     0x00CB60C8 for the chat-menu keys, 0x00CB6358 for the items).
#ifndef VISORT_H
#define VISORT_H

const int kSortCutoff = 16;

template <class T> inline T *SortTag(const T *)
{
    return (T *)0;
}

template <class A, class B, class T> inline void SortSwapTagged(A a, B b, T *)
{
    T held = *a;
    *a = *b, *b = held;
}

template <class A, class B> inline void SortSwap(A a, B b)
{
    SortSwapTagged(a, b, SortTag(a));
}

template <class In, class Out> inline Out SortCopyBack(In from, In to, Out end)
{
    while (from != to)
        *--end = *--to;
    return end;
}

template <class T> inline T SortMedian(T a, T b, T c)
{
    if (a < b)
        return (b < c ? b : a < c ? c : a);
    else
        return (a < c ? a : b < c ? c : b);
}

template <class It, class T> inline It SortPartition(It lo, It hi, T pivot)
{
    for (;; ++lo)
    {
        for (; *lo < pivot; ++lo)
            ;
        for (; pivot < *--hi;)
            ;
        if (hi <= lo)
            return lo;
        SortSwap(lo, hi);
    }
}

template <class It, class T> inline void SortLoop(It lo, It hi, T *)
{
    for (; kSortCutoff < hi - lo;)
    {
        It cut = SortPartition(lo, hi, SortMedian(T(*lo), T(*(lo + (hi - lo) / 2)), T(*(hi - 1))));
        if (hi - cut <= cut - lo)
            SortLoop(cut, hi, SortTag(lo)), hi = cut;
        else
            SortLoop(lo, cut, SortTag(lo)), lo = cut;
    }
}

template <class It, class T> inline void SortShiftIn(It at, T value)
{
    for (It prev = at; value < *--prev; at = prev)
        *at = *prev;
    *at = value;
}

template <class It, class T> inline void SortInsertionTagged(It lo, It hi, T *)
{
    if (lo != hi)
        for (It next = lo; ++next != hi;)
        {
            T value = *next;
            if (!(value < *lo))
                SortShiftIn(next, value);
            else
            {
                SortCopyBack(lo, next, next + 1);
                *lo = value;
            }
        }
}

template <class It> inline void SortInsertion(It lo, It hi)
{
    SortInsertionTagged(lo, hi, SortTag(lo));
}

template <class It, class T> inline void SortTagged(It lo, It hi, T *)
{
    if (hi - lo <= kSortCutoff)
        SortInsertion(lo, hi);
    else
    {
        SortLoop(lo, hi, (T *)0);
        SortInsertion(lo, lo + kSortCutoff);
        for (lo += kSortCutoff; lo != hi; ++lo)
            SortShiftIn(lo, T(*lo));
    }
}

template <class It> inline void sort(It lo, It hi)
{
    SortTagged(lo, hi, SortTag(lo));
}

#endif
