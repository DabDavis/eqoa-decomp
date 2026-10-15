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

#include "vilist_t.h"


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
