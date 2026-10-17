// vikeydiff.h -- VIKeyDiff.h: the difference between two
// sorted VIArrays of database rows -- the rows of New that are new or changed, and the
// keys of Old that New no longer has -- and its stream writer.  VICharacter.cpp alone
// uses it (17 instantiations, CLIENT 0x00CAA1C8 .. 0x00CB3600), so only that TU includes
// it.  The asserts are on lines 118 and 122 ("Old(k-1) < Old(k)", "New(l-1) < New(l)"),
// which the #line below puts there.  Names ours except Old, New, k, l and the file.
#ifndef VIKEYDIFF_H
#define VIKEYDIFF_H

#include "vimemstreamvector.h"

// Not inline: every instantiation is an out-of-line copy (0x00CAA540 ..: `!(a == b)`).
template <class T> bool operator!=(const T &a, const T &b)
{
    return !(a == b);
}

// The result: rows to (re)write, then keys to delete (24 bytes; the entry builds one on
// its stack, Changed at +0 and Deleted at +12).
template <class T, class K> class VIKeyDiff
{
public:
    VIVector<T> Changed;
    VIVector<K> Deleted;
};

#line 114 "C:/code-frontiers-gm/game/VIKeyDiff.h"
template <class T, class K>
void KeyDiff(const VIArray<T> &New, const VIArray<T> &Old, VIKeyDiff<T, K> &Diff)
{
    for (int k = 1; k < Old.Size(); k++)
        VIASSERT(Old(k-1) < Old(k));


    for (int l = 1; l < New.Size(); l++)
        VIASSERT(New(l-1) < New(l));
    int k = 0;
    int l = 0;
    while (k < Old.Size() || l < New.Size())
    {
        if (k < Old.Size() && (l >= New.Size() || (const K &)Old(k) < (const K &)New(l)))
        {
            Diff.Deleted.Add(Old(k));
            k++;
        }
        else if (k >= Old.Size() || (const K &)New(l) < (const K &)Old(k))
        {
            Diff.Changed.Add(New(l));
            l++;
        }
        else
        {
            if (New(l) != Old(k))
                Diff.Changed.Add(New(l));
            k++;
            l++;
        }
    }
}

template <class T, class K>
VIMemStream &operator<<(VIMemStream &s, const VIKeyDiff<T, K> &Diff)
{
    s << Diff.Changed;
    s << Diff.Deleted;
    return s;
}

template <class T, class K>
void KeyDiff(const VIArray<T> &New, const VIArray<T> &Old, VIMemStream &s)
{
    VIKeyDiff<T, K> Diff;
    KeyDiff(New, Old, Diff);
    s << Diff;
}

#endif
