// VIPool<T> -- SUPPORT 0x0048DE08 .. 0x0048E380 for VIPool<VICollBuffer *>.
//
// The engine's intrusive free-list pool, and the first TEMPLATE in this tree:
// nineteen instantiations exist in the module and every one of them is the
// same code, so a single source has to answer for 236 symbols.
//
// The layout is readable straight off the small members: Size() returns +0,
// Begin() reads +16, Next() indexes a 12-byte node and reads its +4, and the
// constructor writes -1 to +12/+16/+20 and zero to +0/+4/+8.
//
//     +0x00 m_size       live element count
//     +0x04 m_capacity   node slots allocated
//     +0x08 m_nodes      Node[]
//     +0x0C m_free       head of the free chain, -1 when exhausted
//     +0x10 m_first      head of the live chain
//     +0x14 m_last       tail of the live chain
//
//     Node { T m_data; int m_next; int m_prev; }   // 12 bytes for T = a pointer
//
// End() is 0x7FFFFFFF and not -1 -- go-eqoa-vi found that from the console
// side, and here it is in the object code as `lui 0x7fff` / `ori 0xffff`.
// -2 in m_prev marks a node that is on the free chain.
#include "vi.h"

class VICollBuffer;
class VIPrimBuffer;
class VIColorBuffer;
class VIHSpriteAnim;
class VIMaterialPal;
class VISpellEffect;
class VIParticleMotif;
class VIStaticLighting;
class VIParticleDefinition;
class VIRFont;
class VIRefMap;
class VISprite;
class VISurface;


#include "vipool_t.h"

// ---------------------------------------------------------------------------
// The instantiations.  Thirteen pointer element types, and NOT one of them
// carries the whole class: the module holds only the members the game reached,
// so each is instantiated MEMBER BY MEMBER to mirror exactly what is there --
// `template class VIPool<X *>;` would emit six Inits, three Sizes and a Clear
// that no instantiation in the image has, and each would read as a function
// this decomp cannot find rather than as one the compiler never emitted.
// ---------------------------------------------------------------------------
// VIPool<VICollBuffer *>
template VIPool<VICollBuffer *>::VIPool();
template VIPool<VICollBuffer *>::~VIPool();
template int VIPool<VICollBuffer *>::Size() const;
template int VIPool<VICollBuffer *>::End() const;
template int VIPool<VICollBuffer *>::Begin() const;
template int VIPool<VICollBuffer *>::Next(int) const;
template int VIPool<VICollBuffer *>::Clear();
template int VIPool<VICollBuffer *>::DestroyPool();
template int VIPool<VICollBuffer *>::LinkFreeChain(int);
template int VIPool<VICollBuffer *>::Allocate();
template int VIPool<VICollBuffer *>::Add(VICollBuffer *const &);
template int VIPool<VICollBuffer *>::Erase(int);

// VIPool<VIPrimBuffer *>
template VIPool<VIPrimBuffer *>::VIPool();
template VIPool<VIPrimBuffer *>::~VIPool();
template int VIPool<VIPrimBuffer *>::Size() const;
template int VIPool<VIPrimBuffer *>::End() const;
template int VIPool<VIPrimBuffer *>::Begin() const;
template int VIPool<VIPrimBuffer *>::Next(int) const;
template int VIPool<VIPrimBuffer *>::Clear();
template int VIPool<VIPrimBuffer *>::DestroyPool();
template int VIPool<VIPrimBuffer *>::LinkFreeChain(int);
template int VIPool<VIPrimBuffer *>::Allocate();
template int VIPool<VIPrimBuffer *>::Add(VIPrimBuffer *const &);
template int VIPool<VIPrimBuffer *>::Erase(int);
template int VIPool<VIPrimBuffer *>::Init(int);

// VIPool<VIColorBuffer *>
template VIPool<VIColorBuffer *>::VIPool();
template VIPool<VIColorBuffer *>::~VIPool();
template int VIPool<VIColorBuffer *>::Size() const;
template int VIPool<VIColorBuffer *>::End() const;
template int VIPool<VIColorBuffer *>::Begin() const;
template int VIPool<VIColorBuffer *>::Next(int) const;
template int VIPool<VIColorBuffer *>::Clear();
template int VIPool<VIColorBuffer *>::DestroyPool();
template int VIPool<VIColorBuffer *>::LinkFreeChain(int);
template int VIPool<VIColorBuffer *>::Allocate();
template int VIPool<VIColorBuffer *>::Add(VIColorBuffer *const &);
template int VIPool<VIColorBuffer *>::Erase(int);

// VIPool<VIHSpriteAnim *>
template VIPool<VIHSpriteAnim *>::VIPool();
template VIPool<VIHSpriteAnim *>::~VIPool();
template int VIPool<VIHSpriteAnim *>::Size() const;
template int VIPool<VIHSpriteAnim *>::End() const;
template int VIPool<VIHSpriteAnim *>::Begin() const;
template int VIPool<VIHSpriteAnim *>::Next(int) const;
template int VIPool<VIHSpriteAnim *>::Clear();
template int VIPool<VIHSpriteAnim *>::DestroyPool();
template int VIPool<VIHSpriteAnim *>::LinkFreeChain(int);
template int VIPool<VIHSpriteAnim *>::Allocate();
template int VIPool<VIHSpriteAnim *>::Add(VIHSpriteAnim *const &);
template int VIPool<VIHSpriteAnim *>::Erase(int);
template int VIPool<VIHSpriteAnim *>::Init(int);

// VIPool<VIMaterialPal *>
template VIPool<VIMaterialPal *>::VIPool();
template VIPool<VIMaterialPal *>::~VIPool();
template int VIPool<VIMaterialPal *>::Size() const;
template int VIPool<VIMaterialPal *>::End() const;
template int VIPool<VIMaterialPal *>::Begin() const;
template int VIPool<VIMaterialPal *>::Next(int) const;
template int VIPool<VIMaterialPal *>::Clear();
template int VIPool<VIMaterialPal *>::DestroyPool();
template int VIPool<VIMaterialPal *>::LinkFreeChain(int);
template int VIPool<VIMaterialPal *>::Allocate();
template int VIPool<VIMaterialPal *>::Add(VIMaterialPal *const &);
template int VIPool<VIMaterialPal *>::Erase(int);
template int VIPool<VIMaterialPal *>::Init(int);

// VIPool<VISpellEffect *>
template VIPool<VISpellEffect *>::VIPool();
template VIPool<VISpellEffect *>::~VIPool();
template int VIPool<VISpellEffect *>::End() const;
template int VIPool<VISpellEffect *>::Begin() const;
template int VIPool<VISpellEffect *>::Next(int) const;
template int VIPool<VISpellEffect *>::Clear();
template int VIPool<VISpellEffect *>::DestroyPool();
template int VIPool<VISpellEffect *>::LinkFreeChain(int);
template int VIPool<VISpellEffect *>::Allocate();
template int VIPool<VISpellEffect *>::Add(VISpellEffect *const &);
template int VIPool<VISpellEffect *>::Erase(int);

// VIPool<VIParticleMotif *>
template VIPool<VIParticleMotif *>::VIPool();
template VIPool<VIParticleMotif *>::~VIPool();
template int VIPool<VIParticleMotif *>::Size() const;
template int VIPool<VIParticleMotif *>::End() const;
template int VIPool<VIParticleMotif *>::Begin() const;
template int VIPool<VIParticleMotif *>::Next(int) const;
template int VIPool<VIParticleMotif *>::DestroyPool();
template int VIPool<VIParticleMotif *>::LinkFreeChain(int);
template int VIPool<VIParticleMotif *>::Allocate();
template int VIPool<VIParticleMotif *>::Add(VIParticleMotif *const &);
template int VIPool<VIParticleMotif *>::Erase(int);

// VIPool<VIStaticLighting *>
template VIPool<VIStaticLighting *>::VIPool();
template VIPool<VIStaticLighting *>::~VIPool();
template int VIPool<VIStaticLighting *>::Size() const;
template int VIPool<VIStaticLighting *>::End() const;
template int VIPool<VIStaticLighting *>::Begin() const;
template int VIPool<VIStaticLighting *>::Next(int) const;
template int VIPool<VIStaticLighting *>::Clear();
template int VIPool<VIStaticLighting *>::DestroyPool();
template int VIPool<VIStaticLighting *>::LinkFreeChain(int);
template int VIPool<VIStaticLighting *>::Allocate();
template int VIPool<VIStaticLighting *>::Add(VIStaticLighting *const &);
template int VIPool<VIStaticLighting *>::Erase(int);
template int VIPool<VIStaticLighting *>::Init(int);

// VIPool<VIParticleDefinition *>
template VIPool<VIParticleDefinition *>::VIPool();
template VIPool<VIParticleDefinition *>::~VIPool();
template int VIPool<VIParticleDefinition *>::End() const;
template int VIPool<VIParticleDefinition *>::Begin() const;
template int VIPool<VIParticleDefinition *>::Next(int) const;
template int VIPool<VIParticleDefinition *>::Clear();
template int VIPool<VIParticleDefinition *>::DestroyPool();
template int VIPool<VIParticleDefinition *>::LinkFreeChain(int);
template int VIPool<VIParticleDefinition *>::Allocate();
template int VIPool<VIParticleDefinition *>::Add(VIParticleDefinition *const &);
template int VIPool<VIParticleDefinition *>::Erase(int);

// VIPool<VIRFont *>
template VIPool<VIRFont *>::VIPool();
template VIPool<VIRFont *>::~VIPool();
template int VIPool<VIRFont *>::Size() const;
template int VIPool<VIRFont *>::End() const;
template int VIPool<VIRFont *>::Begin() const;
template int VIPool<VIRFont *>::Next(int) const;
template int VIPool<VIRFont *>::Clear();
template int VIPool<VIRFont *>::DestroyPool();
template int VIPool<VIRFont *>::LinkFreeChain(int);
template int VIPool<VIRFont *>::Allocate();
template int VIPool<VIRFont *>::Add(VIRFont *const &);
template int VIPool<VIRFont *>::Erase(int);

// VIPool<VIRefMap *>
template VIPool<VIRefMap *>::VIPool();
template VIPool<VIRefMap *>::~VIPool();
template int VIPool<VIRefMap *>::End() const;
template int VIPool<VIRefMap *>::Begin() const;
template int VIPool<VIRefMap *>::Next(int) const;
template int VIPool<VIRefMap *>::Clear();
template int VIPool<VIRefMap *>::DestroyPool();
template int VIPool<VIRefMap *>::LinkFreeChain(int);
template int VIPool<VIRefMap *>::Allocate();
template int VIPool<VIRefMap *>::Add(VIRefMap *const &);
template int VIPool<VIRefMap *>::Erase(int);
template int VIPool<VIRefMap *>::Init(int);

// VIPool<VISprite *>
template VIPool<VISprite *>::VIPool();
template VIPool<VISprite *>::~VIPool();
template int VIPool<VISprite *>::Size() const;
template int VIPool<VISprite *>::End() const;
template int VIPool<VISprite *>::Begin() const;
template int VIPool<VISprite *>::Next(int) const;
template int VIPool<VISprite *>::Clear();
template int VIPool<VISprite *>::DestroyPool();
template int VIPool<VISprite *>::LinkFreeChain(int);
template int VIPool<VISprite *>::Allocate();
template int VIPool<VISprite *>::Add(VISprite *const &);
template int VIPool<VISprite *>::Erase(int);
template int VIPool<VISprite *>::Init(int);

// VIPool<VISurface *>
template VIPool<VISurface *>::VIPool();
template VIPool<VISurface *>::~VIPool();
template int VIPool<VISurface *>::Size() const;
template int VIPool<VISurface *>::End() const;
template int VIPool<VISurface *>::Begin() const;
template int VIPool<VISurface *>::Next(int) const;
template int VIPool<VISurface *>::Clear();
template int VIPool<VISurface *>::DestroyPool();
template int VIPool<VISurface *>::LinkFreeChain(int);
template int VIPool<VISurface *>::Allocate();
template int VIPool<VISurface *>::Add(VISurface *const &);
template int VIPool<VISurface *>::Erase(int);
template int VIPool<VISurface *>::Init(int);


// The three VALUE instantiations VIHSprite owns.  Same source, no re-tuning:
// what changes is the element size, whether T constructs, and which of the two
// Adds and which of the two Inits each one asks for.
#define VIPOOL_VALUE_COMMON(T)                          \
    template VIPool<T>::VIPool();                       \
    template VIPool<T>::~VIPool();                      \
    template int VIPool<T>::End() const;                \
    template int VIPool<T>::Begin() const;              \
    template int VIPool<T>::Next(int) const;            \
    template int VIPool<T>::Clear();                    \
    template int VIPool<T>::DestroyPool();              \
    template int VIPool<T>::LinkFreeChain(int);         \
    template int VIPool<T>::Allocate();                 \
    template int VIPool<T>::Erase(int);                 \
    template int VIPool<T>::Init(const VIPool<T> &);

VIPOOL_VALUE_COMMON(VIHSpriteAttachment)
template int VIPool<VIHSpriteAttachment>::Size() const;
template int VIPool<VIHSpriteAttachment>::Prev(int) const;
template int VIPool<VIHSpriteAttachment>::Add(const VIHSpriteAttachment &);

VIPOOL_VALUE_COMMON(VIHSpritePlay)
template int VIPool<VIHSpritePlay>::Size() const;
template int VIPool<VIHSpritePlay>::Add();

VIPOOL_VALUE_COMMON(VIHSpritePlayNode)
template int VIPool<VIHSpritePlayNode>::Add();


// The three VALUE instantiations phase 34 added.  VIEEBucket is VIEESurfCache's
// bucket chain, VILoaderTrans VILoader's in-flight transfer table and
// VIRasterMaterialLRU the raster's material cache.  None of the three takes
// VIPOOL_VALUE_COMMON: each asks for a DIFFERENT subset, and the macro's
// Erase and Init(const VIPool &) are in only one of them between them.
#define VIPOOL_PLUMBING(T)                              \
    template VIPool<T>::VIPool();                       \
    template VIPool<T>::~VIPool();                      \
    template int VIPool<T>::End() const;                \
    template int VIPool<T>::Begin() const;              \
    template int VIPool<T>::Next(int) const;            \
    template int VIPool<T>::Clear();                    \
    template int VIPool<T>::DestroyPool();              \
    template int VIPool<T>::LinkFreeChain(int);         \
    template int VIPool<T>::Allocate();                 \
    template int VIPool<T>::Add(const T &);

VIPOOL_PLUMBING(VIEEBucket)
template int VIPool<VIEEBucket>::Prev(int) const;
template int VIPool<VIEEBucket>::Erase(int);
template int VIPool<VIEEBucket>::Insert(const VIEEBucket &, int);

VIPOOL_PLUMBING(VILoaderTrans)
template int VIPool<VILoaderTrans>::Erase(int);
template int VIPool<VILoaderTrans>::Init(int);

VIPOOL_PLUMBING(VIRasterMaterialLRU)
template int VIPool<VIRasterMaterialLRU>::Size() const;
template int VIPool<VIRasterMaterialLRU>::Init(int);
template int VIPool<VIRasterMaterialLRU>::MoveToLast(int);
