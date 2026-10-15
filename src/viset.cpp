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

#include "viset_t.h"


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
