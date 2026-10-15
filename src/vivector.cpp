// VIVector<T> -- the engine's THIRD template, and the only container in it
// that grows.
//
// SUPPORT scatters the four instantiations across five address ranges
// (0x00426960, 0x0043F318, 0x0044AFC0, 0x0044CF38, 0x0046D8A0, 0x004704F0),
// which is the same picture VIArray gave in Phase 24: every TU that holds a
// VIVector emits its own static copies of the pieces gcc chose not to make
// weak, so a named symbol's derived span swallows an unnamed twin.
#include "vi.h"

#include "vivector_t.h"


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
