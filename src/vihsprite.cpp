// VIHSprite -- SUPPORT 0x00445AF0 ..  The hierarchical sprite: a node tree,
// blend matrices, triggers, and three pools.
//
// Only the readers are attempted here.  Every one of them indexes a container
// with a plain SUBSCRIPT -- `addu base, offset` -- where VIHSpriteAnim and
// VIVector both need Phase 24's integer form; the two spellings sit side by
// side in this phase and the object code separates them every time.
#include "vi.h"

int VIHSprite::Init()
{
    if (Clear() < 0)
        return -1;
    return 0;
}

int VIHSprite::Size()
{
    return m_nodes.m_size;
}

int VIHSprite::End()
{
    return 0x7FFFFFFF;
}

int VIHSprite::Root()
{
    if (m_nodes.m_size)
        return 0;
    return End();
}

int VIHSprite::First(int i)
{
    // The element is named through a POINTER -- Phase 9's lever, and it is
    // what every indexed reader in this class needs: `m_data[i].field` puts
    // the sum in the INDEX's register (`addu a1,a1,base`), where the target
    // puts it in the base's (`addu v1,v1,a1`).  Three words of five, every
    // time.
    VIHSpriteNode *n = &m_nodes.m_data[i];
    if (n->m_child != -1)
        return n->m_child;
    return End();
}

int VIHSprite::Next(int i)
{
    VIHSpriteNode *n = &m_nodes.m_data[i];
    if (n->m_sibling != -1)
        return n->m_sibling;
    return End();
}

// A shipped bug, and the inverse of what the name says: the parent is only
// answered for node 0 -- whose parent is nothing -- and every other node gets
// End().
int VIHSprite::Parent(int i)
{
    // Phase 7's if-direction, per function: the End() path goes in the `if`
    // here and the body lands last, where First and Next want the opposite.
    if (i)
        return End();
    VIHSpriteNode *n = &m_nodes.m_data[i];
    return n->m_parent;
}

int VIHSprite::IsLeaf(int i)
{
    VIHSpriteNode *n = &m_nodes.m_data[i];
    return n->m_child == -1;
}

int VIHSprite::Last(int i)
{
    // Phase 10's guarded do/while: written as a `while` the sibling is read
    // TWICE per turn and gcc keeps both loads, which unrolls the loop into
    // three copies of its body (24 of 37 words wrong).
    int j = End();
    VIHSpriteNode *n = &m_nodes.m_data[i];
    int k = n->m_child;
    if (k != -1)
    {
        do
        {
            j = k;
            VIHSpriteNode *m = &m_nodes.m_data[j];
            k = m->m_sibling;
        }
        while (k != -1);
    }
    return j;
}

// The second half of the same bug: the walk starts at node i's OWN first
// child, where the previous sibling of i lives under i's parent.
int VIHSprite::Prev(int i)
{
    int p = End();
    if (i == 0)
        return p;
    VIHSpriteNode *n = &m_nodes.m_data[i];
    int j = n->m_child;
    while (j != i)
    {
        p = j;
        VIHSpriteNode *m = &m_nodes.m_data[j];
        j = m->m_sibling;
    }
    if (j < 0)
        return -1;
    return p;
}

int VIHSprite::FindTrigger(int id, int &out)
{
    // The CONTAINER is named through a pointer here, not just the element:
    // `m_size` folds back onto `this` (88(t0)) while `m_data` comes off the
    // pointer (4(t1)).  That is Phase 9's lever one level up, and it is the
    // only reader in the class that shows it.
    for (int i = 0; i < m_triggers.m_size; i++)
    {
        VIArray<VIHSpriteTrigger> *a = &m_triggers;
        VIHSpriteTrigger *t = &a->m_data[i];
        if (t->m_a == id)
        {
            out = i;
            return 1;
        }
    }
    return 0;
}

int VIHSprite::HasTripped(int i)
{
    VIHSpriteTrigger *t = &m_triggers.m_data[i];
    return t->m_b;
}

int VIHSprite::ResetTrigger(int i)
{
    VIHSpriteTrigger *t = &m_triggers.m_data[i];
    t->m_b = 0;
    return 0;
}

int VIHSprite::GetNumAttach() const
{
    return m_attach.Size();
}

int VIHSprite::GetFirstAttach() const
{
    if (m_attach.Size() == 0)
        return -1;
    return m_attach.Begin();
}

int VIHSprite::GetLastAttach() const
{
    if (m_attach.Size() == 0)
        return -1;
    return m_attach.Prev(m_attach.End());
}

int VIHSprite::GetPrevAttach(int i) const
{
    if (i == m_attach.Begin())
        return -1;
    return m_attach.Prev(i);
}

int VIHSprite::GetAttachSprite(int i) const
{
    VIHSpriteAttachment *a = &m_attach.m_nodes[i].m_data;
    return a->m_sprite;
}

int VIHSprite::GetAttachNode(int i) const
{
    VIHSpriteAttachment *a = &m_attach.m_nodes[i].m_data;
    return a->m_node;
}

int VIHSprite::GetPlayAnimation(int i)
{
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    return p->m_animID;
}

int VIHSprite::SetPlaySpeed(int i, float s)
{
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    p->m_speed = s;
    if (PrecalcPlayback(i) < 0)
        return -1;
    return 0;
}

float VIHSprite::GetPlaySpeed(int i)
{
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    return p->m_speed;
}

int VIHSprite::IsPlaybackStopped(int i)
{
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    return p->m_playbackType == VIPlaybackTypeStopEnd;
}

int VIHSprite::SetPlayRate(int i, float r)
{
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    p->m_rate = r;
    if (PrecalcPlayback(i) < 0)
        return -1;
    return 0;
}

float VIHSprite::GetPlayRate(int i)
{
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    return p->m_rate;
}

// ---------------------------------------------------------------------------
// Phase 47 -- the rest of the class.
// ---------------------------------------------------------------------------

// VIHSpriteFrame::Pack -- 0x00444D98, ahead of the class it serves.  Rotation
// in 1/32767, scale and translation in 1/512, each rounded half away from zero
// and clamped to a short.  Both the rounding and the clamp are MACROS, which
// is why the product is recomputed at every test.
#define VIROUND(x) ((x) < 0.0f ? (int)((x) - 0.5f) : (int)((x) + 0.5f))

static inline void PackQuat(VIVect4S16 &out, const VIQuat &q, float k)
{
    out.x = VIMIN(32767, VIMAX(-32768, VIROUND(q.x * k)));
    out.y = VIMIN(32767, VIMAX(-32768, VIROUND(q.y * k)));
    out.z = VIMIN(32767, VIMAX(-32768, VIROUND(q.z * k)));
    out.w = VIMIN(32767, VIMAX(-32768, VIROUND(q.w * k)));
}

void VIHSpriteFrame::Pack(VIHSpritePackFrame &p)
{
    PackQuat(p.m_rot, m_rot, 32767.0f);
    VIQuat st(m_scale, m_trans.x, m_trans.y, m_trans.z);
    PackQuat(p.m_scaleTrans, st, 512.0f);
}

VIHSprite::VIHSprite()
{
    m_type = 4;
    m_defaultScale = 1.0f;
    m_refMap = -1;
    m_defaultAnim = -1;
    m_center[1].Zero();

    VIHSpriteFrame *f0 = &m_transform[0];
    VIHSpriteFrame *f1 = &m_transform[1];

    f1->m_rot.x = 0.0f;
    f1->m_rot.y = 0.0f;
    f1->m_rot.z = 0.0f;
    f1->m_rot.w = 1.0f;
    m_transform[1].m_scale = m_defaultScale;
    m_transform[1].m_trans.Zero();
    m_center[0] = m_center[1];
    m_transform[0].m_rot = m_transform[1].m_rot;
    f0->m_scale = f1->m_scale;
    m_transform[0].m_trans = m_transform[1].m_trans;
    m_blendMS = 0;
    m_blendStart = 0;
    m_playTime = -1;
    m_centerTime = -1;
    m_poseTime = -1;
}

VIHSprite::~VIHSprite()
{
    Clear();
}

int VIHSprite::Clear()
{
    if (m_nodes.Clear() < 0)
        return -1;
    if (m_blend.Clear() < 0)
        return -1;
    if (m_triggers.Clear() < 0)
        return -1;
    if (m_attach.Clear() < 0)
        return -1;
    if (m_plays.Clear() < 0)
        return -1;
    if (m_playNodes.Clear() < 0)
        return -1;
    m_refMap = -1;
    m_defaultScale = 1.0f;
    m_defaultAnim = -1;
    return 0;
}

int VIHSprite::GetDefaultAnim()
{
    return m_defaultAnim;
}

float VIHSprite::GetDefaultScale() const
{
    return m_defaultScale;
}

int VIHSprite::GetNextAttach(int i) const
{
    if (m_attach.Next(i) == m_attach.End())
        return -1;
    return m_attach.Next(i);
}

int VIHSprite::SetNodeFixScale(int i, const VIVect3 &v)
{
    VIHSpriteNode *n = &m_nodes.m_data[i];
    n->m_fixScale = v;
    return 0;
}

int VIHSprite::GetNodeFixScale(int i, VIVect3 &v)
{
    VIHSpriteNode *n = &m_nodes.m_data[i];
    v = n->m_fixScale;
    return 0;
}

// The world bind transform goes into the node's own frame and its inverse into
// +0xA0; the REST pose is that transform taken back into the parent's space --
// or, for a root, the bind pose itself.
int VIHSprite::SetNodeBindPose(int i, const VIQuat &q, float s, const VIVect3 &t)
{
    VIMatrix44 m, r;
    VIVector<VIHSpriteNode> *a = &m_nodes;

    // A fresh element pointer per statement: every store re-derives the
    // element address, base first.
    VIHSpriteNode *n0 = &a->m_data[i];
    n0->m_bind.m_rot = q;
    VIHSpriteNode *n1 = &a->m_data[i];
    n1->m_bind.m_scale = s;
    VIHSpriteNode *n2 = &a->m_data[i];
    n2->m_bind.m_trans = t;
    m.Transform(q, s, t);
    VIHSpriteNode *n3 = &a->m_data[i];
    n3->m_invBindWorld.Invert(m);

    VIHSpriteNode *n4 = &a->m_data[i];
    if (n4->m_parent != -1)
    {
        r.Mul(m, a->m_data[n4->m_parent].m_invBindWorld);
        VIHSpriteNode *n5 = &a->m_data[i];
        r.ConvertTo(n5->m_rest.m_rot, n5->m_rest.m_scale, n5->m_rest.m_trans);
    }
    else
    {
        n4->m_rest.m_rot = q;
        VIHSpriteNode *n6 = &a->m_data[i];
        n6->m_rest.m_scale = s;
        VIHSpriteNode *n7 = &a->m_data[i];
        n7->m_rest.m_trans = t;
    }
    return 0;
}

int VIHSprite::GetNodeBindPose(int i, VIQuat &q, float &s, VIVect3 &t)
{
    // SetNodeBindPose's fresh pointer per statement: one pointer reassigned
    // three times leaves the first in the wrong register (8 wrong words).
    VIVector<VIHSpriteNode> *a = &m_nodes;
    VIHSpriteNode *n0 = &a->m_data[i];
    q = n0->m_bind.m_rot;
    VIHSpriteNode *n1 = &a->m_data[i];
    s = n1->m_bind.m_scale;
    VIHSpriteNode *n2 = &a->m_data[i];
    t = n2->m_bind.m_trans;
    return 0;
}

// The named array pointer, declared before the test, is what puts its
// `addiu a0,s0,64` in the `bnez`'s delay slot (6 wrong words without it).
int VIHSprite::AddRoot()
{
    VIVector<VIHSpriteNode> *a = &m_nodes;
    if (m_nodes.m_size == 0)
    {
        int i = a->Add();
        if (i < 0)
            return -1;
        if (m_blend.Add() < 0)
            return -1;
        return i;
    }
    return -1;
}

int VIHSprite::Add(int parent)
{
    VIVector<VIHSpriteNode> *a = &m_nodes;
    if (m_nodes.m_size == 0)
        return -1;
    int i = a->Add();
    if (i < 0)
        return -1;
    if (m_blend.Add() < 0)
        return -1;
    VIHSpriteNode *n = &a->m_data[i];
    n->m_parent = parent;
    VIHSpriteNode *p = &a->m_data[parent];
    if (p->m_child == -1)
    {
        p->m_child = i;
    }
    else
    {
        VIHSpriteNode *l = &a->m_data[Last(parent)];
        l->m_sibling = i;
    }
    return i;
}

int VIHSprite::AttachSkin(int sprite, VIScene &sc)
{
    if (sc.ShareSprite(sprite) < 0)
        return -1;
    VIHSpriteAttachment a;
    a.m_sprite = sprite;
    a.m_node = -1;
    return m_attach.Add(a);
}

int VIHSprite::Attach(int sprite, int node, VIScene &sc)
{
    if (sc.ShareSprite(sprite) < 0)
        return -1;
    VIHSpriteAttachment a;
    a.m_node = node;
    a.m_sprite = sprite;
    return m_attach.Add(a);
}

int VIHSprite::Detach(int i, VIScene &sc)
{
    VIPool<VIHSpriteAttachment> *p = &m_attach;
    if (sc.ReleaseSprite(p->Value(i).m_sprite) < 0)
        return -1;
    return p->Erase(i);
}

int VIHSprite::AddPlay(int anim, VIScene &sc)
{
    VIHSpriteAnim *a = sc.Animation(anim);
    return AddPlay(anim, a->m_playbackType, a->m_playSpeed, sc);
}

int VIHSprite::AddPlay(int anim, VIPlaybackType type, VIScene &sc)
{
    VIHSpriteAnim *a = sc.Animation(anim);
    return AddPlay(anim, type, a->m_playSpeed, sc);
}

// The play record, then one play node per animation node whose ref id the
// sprite's ref map can resolve -- a node the map does not know is skipped,
// not an error.
int VIHSprite::AddPlay(int anim, VIPlaybackType type, float speed, VIScene &sc)
{
    VIPool<VIHSpritePlay> *pool = &m_plays;
    VIRefMap *map = sc.RefMap(m_refMap);
    VIHSpriteAnim *a = sc.Animation(anim);
    int i = pool->Add();

    if (i < 0)
        return -1;
    if (sc.ShareAnimation(anim) < 0)
        return -1;

    pool->Value(i).m_animID = anim;
    pool->Value(i).m_playbackType = type;
    pool->Value(i).m_interpolate = 1;
    pool->Value(i).m_blendMode = 0;
    pool->Value(i).m_blendStart = sc.GetTime();
    pool->Value(i).m_blendDuration = 0;
    pool->Value(i).m_weight = 0.0f;
    pool->Value(i).m_rate = a->m_playRate;
    pool->Value(i).m_speed = speed;
    pool->Value(i).m_frameA = 0;
    pool->Value(i).m_frameB = 0;
    pool->Value(i).m_timeAccum = 0.0f;
    pool->Value(i).m_frameT = 0.0f;
    pool->Value(i).m_lastTime = sc.GetTime();
    pool->Value(i).m_frameRate = 0.0f;
    pool->Value(i).m_numNodes = 0;
    pool->Value(i).m_firstNode = -1;
    pool->Value(i).m_pad58 = -1;

    for (int k = 0; k < a->m_numNodes; k++)
    {
        int key = a->m_nodes[k].m_refID;
        int f = map->m_map.Find(key);
        int node;

        if (f != map->m_map.End())
            node = map->m_map.Value(f);
        else
            node = -1;

        if (node != -1)
        {
            int n = m_playNodes.Add();

            if (n < 0)
                return -1;
            if (m_plays.Value(i).m_numNodes == 0)
                m_plays.Value(i).m_firstNode = n;
            m_plays.Value(i).m_pad58 = n;
            m_playNodes.Value(n).m_a = 0;
            m_playNodes.Value(n).m_b = node;
            m_playNodes.Value(n).m_c = a->m_nodes[k].m_firstFrame;
            m_plays.Value(i).m_numNodes++;
        }
    }

    if (PrecalcPlayback(i) < 0)
        return -1;
    return i;
}

int VIHSprite::AddPlayWithSync(int anim, int sync, VIScene &sc)
{
    VIHSpritePlay *p = &m_plays.m_nodes[sync].m_data;
    int i = AddPlay(anim, (VIPlaybackType)p->m_playbackType, p->m_speed, sc);
    if (i < 0)
        return -1;
    if (SyncTiming(i, sync, sc) < 0)
        return -1;
    return i;
}

int VIHSprite::AddInterpolatedPlay(int anim, VIPlaybackType type, float speed,
                                   int ms, VIScene &sc)
{
    int i = AddPlay(anim, type, speed, sc);
    if (i < 0)
        return -1;
    VIPool<VIHSpritePlay> *pool = &m_plays;
    pool->Value(i).m_blendMode = 1;
    pool->Value(i).m_blendDuration = ms;
    return i;
}

int VIHSprite::AddInterpolatedPlayWithSync(int anim, int sync, int ms, VIScene &sc)
{
    VIPool<VIHSpritePlay> *pool = &m_plays;
    int i = AddPlay(anim, (VIPlaybackType)pool->Value(sync).m_playbackType,
                    pool->Value(sync).m_speed, sc);
    if (i < 0)
        return -1;
    pool->Value(i).m_blendMode = 1;
    pool->Value(i).m_blendDuration = ms;
    if (SyncTiming(i, sync, sc) < 0)
        return -1;
    return i;
}

int VIHSprite::ErasePlay(int i, VIScene &sc)
{
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    int n = p->m_numNodes;
    int j = p->m_firstNode;
    if (n)
    {
        do
        {
            int k = m_playNodes.Next(j);
            if (m_playNodes.Erase(j) < 0)
                return -1;
            n--;
            j = k;
        }
        while (n);
    }
    VIPool<VIHSpritePlay> *pool = &m_plays;
    if (sc.ReleaseAnimation(pool->Value(i).m_animID) < 0)
        return -1;
    if (pool->Erase(i) < 0)
        return -1;
    return 0;
}

int VIHSprite::EraseAllPlay(VIScene &sc)
{
    int i = m_plays.Begin();
    while (i != m_plays.End())
    {
        int n = m_plays.Next(i);
        ErasePlay(i, sc);
        i = n;
    }
    return 0;
}

int VIHSprite::StopPlayback(int i, VIScene &sc)
{
    if (ProcessPlayTiming(i, sc) < 0)
        return -1;
    VIPool<VIHSpritePlay> *pool = &m_plays;
    pool->Value(i).m_playbackType = VIPlaybackTypeStopEnd;
    pool->Value(i).m_blendMode = 0;
    return 0;
}

int VIHSprite::StopPlaybackAtCycle(int i, VIScene &sc)
{
    if (ProcessPlayTiming(i, sc) < 0)
        return -1;
    VIPool<VIHSpritePlay> *pool = &m_plays;
    pool->Value(i).m_playbackType = VIPlaybackTypeOnce;
    pool->Value(i).m_blendMode = 0;
    return 0;
}

int VIHSprite::FreezePlayback(int i, VIScene &sc)
{
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    int n = p->m_numNodes;
    int j = p->m_firstNode;
    if (n > 0)
    {
        do
        {
            m_playNodes.Value(j).m_a = 1;
            n--;
            j = m_playNodes.Next(j);
        }
        while (n > 0);
    }
    if (ProcessPlayList(-1, sc) < 0)
        return -1;
    VIPool<VIHSpritePlay> *pool = &m_plays;
    pool->Value(i).m_playbackType = VIPlaybackTypeStopEnd;
    pool->Value(i).m_blendMode = 0;
    return 0;
}

int VIHSprite::StopAllPlayback(VIScene &sc)
{
    int i = m_plays.Begin();
    while (i != m_plays.End())
    {
        int n = m_plays.Next(i);
        StopPlayback(i, sc);
        i = n;
    }
    return 0;
}

int VIHSprite::IsBlendComplete(int i, VIScene &sc)
{
    long t = sc.GetTime();
    VIHSpritePlay *p = &m_plays.m_nodes[i].m_data;
    return p->m_blendDuration < t - p->m_blendStart;
}

int VIHSprite::PrecalcPlayback(int i)
{
    VIPool<VIHSpritePlay> *pool = &m_plays;
    VIHSpritePlay *p = &pool->m_nodes[i].m_data;
    float s = p->m_speed;
    if (s != 0.0f)
    {
        if (0.0f < s)
        {
            p->m_frameRate = p->m_rate * s / 1000.0f;
            pool->Value(i).m_frameDuration = 1.0f / pool->Value(i).m_frameRate;
        }
        else
        {
            p->m_frameRate = p->m_rate * -s / 1000.0f;
            pool->Value(i).m_frameDuration = 1.0f / pool->Value(i).m_frameRate;
        }
    }
    else
    {
        p->m_frameRate = 0.0f;
        pool->Value(i).m_frameDuration = 0.0f;
    }
    return 0;
}

int VIHSprite::SyncTiming(int a, int b, VIScene &sc)
{
    VIPool<VIHSpritePlay> *pool = &m_plays;
    VIHSpritePlay *pa = &pool->m_nodes[a].m_data;
    VIHSpritePlay *pb = &pool->m_nodes[b].m_data;
    VIHSpriteAnim *aa = sc.Animation(pa->m_animID);
    VIHSpriteAnim *ab = sc.Animation(pool->Value(b).m_animID);
    if (aa->m_numFrames != ab->m_numFrames)
        return -1;
    pa->m_frameA = pb->m_frameA;
    pa->m_frameB = pb->m_frameB;
    pa->m_timeAccum = pb->m_timeAccum;
    pa->m_frameT = pb->m_frameT;
    pa->m_lastTime = pb->m_lastTime;
    pa->m_frameRate = pb->m_frameRate;
    return 0;
}

int VIHSprite::IsCenterTransformBlending(VIScene &sc)
{
    return m_blendMS != 0;
}

int VIHSprite::IsCenterTransformBlendComplete(VIScene &sc)
{
    if (m_blendMS == 0 || sc.GetTime() - m_blendStart > m_blendMS)
        return 1;
    return 0;
}

int VIHSprite::StopCenterTransformBlend(VIScene &sc)
{
    VIHSpriteFrame f;
    VIVect3 v;
    if (InterpTransAboutCenter(sc, v, f) < 0)
        return -1;
    m_center[1] = v;
    m_transform[1].m_rot = f.m_rot;
    m_transform[1].m_scale = f.m_scale;
    VIVect3 &t = f.m_trans;
    m_transform[1].m_trans = t;
    m_blendMS = 0;
    return 0;
}

int VIHSprite::SetPlayFrame(int i, float frame, VIScene &sc)
{
    VIPool<VIHSpritePlay> *pool = &m_plays;
    VIHSpriteAnim *a = sc.Animation(pool->Value(i).m_animID);
    int fi = (int)frame;
    VIHSpritePlay *p = &pool->m_nodes[i].m_data;
    p->m_frameA = fi;
    p->m_timeAccum = (frame - (float)fi) * p->m_frameDuration;
    if (p->m_playbackType == VIPlaybackTypeOnce)
    {
        if (a->m_numFrames - 1 < fi)
            p->m_frameA = a->m_numFrames - 1;
        p->m_frameB = p->m_frameA + 1;
        if (a->m_numFrames - 1 < p->m_frameB)
        {
            p->m_frameB = p->m_frameA;
            p->m_timeAccum = 0.0f;
        }
    }
    else
    {
        p->m_frameA = fi % a->m_numFrames;
        p->m_frameB = p->m_frameA + 1;
        if (a->m_numFrames - 1 < p->m_frameB)
            p->m_frameB = 0;
    }
    p->m_frameT = p->m_timeAccum * p->m_frameRate;
    p->m_lastTime = sc.GetTime();
    return 0;
}

int VIHSprite::SetCenterTransform(VIVect3 &center, const VIVect3 &hpr, float s,
                                  const VIVect3 &t, VIScene &sc)
{
    if (m_blendMS == 0 || sc.GetTime() - m_blendStart > m_blendMS)
    {
        m_center[1] = center;
        m_transform[1].m_rot.ConvertFrom(hpr);
        m_transform[1].m_scale = s;
        m_transform[1].m_trans = t;
    }
    return 0;
}

int VIHSprite::BlendToCenterTransform(VIVect3 &center, const VIVect3 &hpr, float s,
                                      const VIVect3 &t, int ms, VIScene &sc)
{
    if (m_blendMS == 0 || sc.GetTime() - m_blendStart > m_blendMS)
    {
        VIHSpriteFrame *f1 = &m_transform[1];

        m_center[0] = m_center[1];
        m_transform[0].m_rot = m_transform[1].m_rot;
        m_transform[0].m_scale = f1->m_scale;
        m_transform[0].m_trans = m_transform[1].m_trans;
        m_center[1] = center;
        f1->m_rot.ConvertFrom(hpr);
        m_transform[1].m_scale = s;
        m_transform[1].m_trans = t;
        m_blendMS = ms;
        m_blendStart = sc.GetTime();
    }
    return 0;
}

int VIHSprite::Process(int lod, VIScene &sc)
{
    if (sc.GetTime() != m_poseTime)
    {
        if (m_plays.Size() > 0)
        {
            if (ProcessPlayList(lod, sc) < 0)
                return -1;
            if (ProcessHierarchy() < 0)
                return -1;
        }
        else
        {
            if (SetBindPose() < 0)
                return -1;
        }
    }
    m_poseTime = sc.GetTime();
    return 0;
}

int VIHSprite::SetBindPose()
{
    for (int i = 0; i < m_nodes.m_size; i++)
    {
        VIVector<VIHSpriteNode> *a = &m_nodes;
        VIHSpriteNode *n = &a->m_data[i];
        n->m_world.Transform(n->m_bind.m_rot, n->m_bind.m_scale, n->m_bind.m_trans);
    }
    return 0;
}

int VIHSprite::ProcessHierarchy()
{
    VIMatrix44 m;
    m.Identity();
    if (ProcessHierarchy(0, m) < 0)
        return -1;
    return 0;
}

// The element through an inline helper handed the MEMBER m_nodes: its `a` is
// an unflagged pseudo, so the m_data load at +8 may alias the stack and stays
// behind v's zeroing stores, as in the guest.  A named `VIVector *` lets sched
// hoist it above them: 13/94 (claude-notes/decomp-alias-rule-inline-ref-
// params-20261014.md).
static inline VIHSpriteNode &NodeAt(VIVector<VIHSpriteNode> &a, int i)
{
    return a.m_data[i];
}

int VIHSprite::ProcessHierarchy(int i, VIMatrix44 &m)
{
    VIMatrix44 t;
    VIVect3 v;
    VIHSpriteNode *n = &NodeAt(m_nodes, i);
    v = n->m_fixScale;
    v *= n->m_frame.m_scale;
    t.Transform(n->m_frame.m_rot, v, n->m_frame.m_trans);
    n->m_world.Mul(t, m);
    // A fresh element pointer for the child, and another (off a second
    // array pointer) for each sibling: 23 wrong words as subscripts.
    VIHSpriteNode *n2 = &NodeAt(m_nodes, i);
    int c = n2->m_child;
    while (c != -1)
    {
        if (ProcessHierarchy(c, n->m_world) < 0)
            return -1;
        VIVector<VIHSpriteNode> *b = &m_nodes;
        VIHSpriteNode *s = &b->m_data[c];
        c = s->m_sibling;
    }
    return 0;
}

int VIHSprite::ProcessUpHierarchy(int i)
{
    VIMatrix44 t;
    VIVect3 v;
    VIHSpriteNode *nodes = m_nodes.m_data;
    VIHSpriteNode *n = &nodes[i];
    if (n->m_parent == -1)
    {
        v = n->m_fixScale;
        v *= n->m_frame.m_scale;
        n->m_world.Transform(n->m_frame.m_rot, v, n->m_frame.m_trans);
    }
    else
    {
        VIHSpriteNode *p = &nodes[n->m_parent];
        ProcessUpHierarchy(n->m_parent);
        v = n->m_fixScale;
        v *= n->m_frame.m_scale;
        t.Transform(n->m_frame.m_rot, v, n->m_frame.m_trans);
        n->m_world.Mul(t, p->m_world);
    }
    return 0;
}

int VIHSprite::SetBlendMatrices(VIRaster &r)
{
    VIMatrix44 t;
    int found = 0;
    int i;

    for (i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (m_attach.Value(i).m_node == -1)
        {
            found = 1;
            break;
        }
    }
    if (found)
    {
        for (i = 0; i < m_nodes.m_size; i++)
        {
            VIVector<VIMatrix44> *b = &m_blend;
            VIVector<VIHSpriteNode> *a = &m_nodes;
            b->m_data[i].Mul(a->m_data[i].m_invBindWorld, a->m_data[i].m_world);
        }
        VIVector<VIMatrix44> *b = &m_blend;
        r.SetObjectBlends(m_nodes.m_size, b->m_data);
    }
    return 0;
}

int VIHSprite::CalcTransAboutCenter(const VIMatrix44 &m, VIScene &sc, VIMatrix44 &out)
{
    if (sc.GetTime() != m_centerTime)
        UpdateCenterTransform(sc);
    out.Mul(m_centerMatrix, m);
    return 0;
}

int VIHSprite::CalcNodeTransform(int i, VIMatrix44 &m, VIScene &sc, VIMatrix44 &out)
{
    VIMatrix44 t;
    VIMatrix44 u;
    CalcTransAboutCenter(m, sc, t);
    if (sc.GetTime() != m_poseTime)
    {
        if (m_plays.Size() > 0)
        {
            if (ProcessPlayList(CalcLODLevelIndex(m), sc) < 0)
                return -1;
            if (ProcessUpHierarchy(i) < 0)
                return -1;
        }
        else
        {
            if (SetBindPose() < 0)
                return -1;
        }
    }
    out.Mul(m_nodes.m_data[i].m_world, t);
    return 0;
}

// The two SetModelView calls are ONE jal in the target: jump2 cross-jumps them
// only because each arm carries the whole tail.  With the tail after the if/else
// the fall-through arm ends in the call, flow puts a (use 0) after it, and
// find_cross_jump stops on the CALL_INSN vs INSN mismatch.
int VIHSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    int hit = 0;
    VIMatrix44 m;
    VIMatrix44 t;
    if (Process(CalcLODLevelIndex(*r.GetModelView()), sc) < 0)
        return -1;
    CalcTransAboutCenter(*r.GetModelView(), sc, m);
    if (SetBlendMatrices(r) < 0)
        return -1;
    for (int i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (m_attach.Value(i).m_node != -1)
        {
            t.Mul(m_nodes.m_data[m_attach.Value(i).m_node].m_world, m);
            r.SetModelView(t);
            if (sc.RasterSprite(cam, m_attach.Value(i).m_sprite, flags))
                hit = 1;
        }
        else
        {
            r.SetModelView(m);
            if (sc.RasterSprite(cam, m_attach.Value(i).m_sprite, flags))
                hit = 1;
        }
    }
    return hit;
}

int VIHSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    VIMatrix44 m;
    VIMatrix44 t;
    int hit = 0;
    if (Process(-1, sc) < 0)
        return -1;
    CalcTransAboutCenter(*c.GetWorld(), sc, m);
    for (int i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (m_attach.Value(i).m_node != -1)
        {
            t.Mul(m_nodes.m_data[m_attach.Value(i).m_node].m_world, m);
            c.SetWorld(t);
            if (sc.Sprite(m_attach.Value(i).m_sprite)->Collide(s, f, sc, c))
            {
                f.m_hitAttachment = i;
                hit = 1;
            }
        }
    }
    return hit;
}

// The sphere overload above with the ray handed down instead -- slot 5 of the
// attachment's vtable rather than slot 4.
int VIHSprite::Collide(VICollRay &ray, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    VIMatrix44 m;
    VIMatrix44 t;
    int hit = 0;
    if (Process(-1, sc) < 0)
        return -1;
    CalcTransAboutCenter(*c.GetWorld(), sc, m);
    for (int i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (m_attach.Value(i).m_node != -1)
        {
            t.Mul(m_nodes.m_data[m_attach.Value(i).m_node].m_world, m);
            c.SetWorld(t);
            if (sc.Sprite(m_attach.Value(i).m_sprite)->Collide(ray, f, sc, c))
            {
                f.m_hitAttachment = i;
                hit = 1;
            }
        }
    }
    return hit;
}

// Raster's duplicated tail, plus ONE function-scope `s` set in both arms: set
// twice it is a global pseudo with no v0 suggestion, so the object goes to t1
// and the function pointer takes v0 (per-arm locals tie the object to v0).
int VIHSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    VIMatrix44 m;
    VIMatrix44 t;
    int hit = 0;
    VISprite *s;
    if (Process(CalcLODLevelIndex(seg, r), sc) < 0)
        return -1;
    CalcTransAboutCenter(*r.GetModelView(), sc, m);
    if (SetBlendMatrices(r) < 0)
        return -1;
    for (int i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (m_attach.Value(i).m_node != -1)
        {
            t.Mul(m_nodes.m_data[m_attach.Value(i).m_node].m_world, m);
            r.SetModelView(t);
            s = sc.Sprite(m_attach.Value(i).m_sprite);
            if (s->Pick(seg, f, sc, r))
            {
                f.m_hitAttachment = i;
                hit = 1;
            }
        }
        else
        {
            r.SetModelView(m);
            s = sc.Sprite(m_attach.Value(i).m_sprite);
            if (s->Pick(seg, f, sc, r))
            {
                f.m_hitAttachment = i;
                hit = 1;
            }
        }
    }
    return hit;
}

int VIHSprite::QueryEmitters(VIScene &sc, const VIMatrix44 &m, VIEmitterQuery &q)
{
    VIMatrix44 t;
    VIMatrix44 u;
    if (Process(CalcLODLevelIndex(m), sc) < 0)
        return -1;
    CalcTransAboutCenter(m, sc, u);
    for (int i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (m_attach.Value(i).m_node != -1)
        {
            t.Mul(m_nodes.m_data[m_attach.Value(i).m_node].m_world, u);
            int spr = m_attach.Value(i).m_sprite;
            q.m_b = spr;
            if (sc.Sprite(spr)->QueryEmitters(sc, t, q) < 0)
                return -1;
        }
    }
    return 0;
}

int VIHSprite::QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                 const VIMatrix44 &m, VIScene &sc)
{
    VIMatrix44 t;
    VIMatrix44 u;
    if (Process(-1, sc) < 0)
        return -1;
    CalcTransAboutCenter(m, sc, u);
    for (int i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (m_attach.Value(i).m_node != -1)
        {
            t.Mul(m_nodes.m_data[m_attach.Value(i).m_node].m_world, u);
            if (sc.Sprite(m_attach.Value(i).m_sprite)->QueryIntersection(b, l, t, sc) < 0)
                return -1;
        }
    }
    return 0;
}

int VIHSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int idx = sc.CreateSprite((VISpriteType)m_type, n);
    if (idx < 0)
        return -1;
    VIHSprite *d = (VIHSprite *)sc.Sprite(idx);
    if (d->m_nodes.Init(m_nodes) < 0)
        return -1;
    if (d->m_blend.Init(m_blend) < 0)
        return -1;
    if (d->m_triggers.Init(m_triggers) < 0)
        return -1;
    if (d->m_attach.Init(m_attach) < 0)
        return -1;
    if (d->m_plays.Init(m_plays) < 0)
        return -1;
    if (d->m_playNodes.Init(m_playNodes) < 0)
        return -1;
    d->m_bbox = m_bbox;
    d->m_bsphere = m_bsphere;
    d->m_type = m_type;
    d->m_offDist = m_offDist;
    d->m_lod = m_lod;
    d->m_defaultScale = m_defaultScale;
    d->m_center[0] = m_center[0];
    d->m_center[1] = m_center[1];
    // Member by member, NOT through VIHSpriteFrame::operator=: the target
    // block-moves each rotation (the implicit VIQuat assignment, ldl/ldr),
    // where the user operator= copies it as four floats -- and the extra
    // pointers that costs pushed the whole function onto s6..s8 (183 words).
    d->m_transform[0].m_rot = m_transform[0].m_rot;
    d->m_transform[0].m_scale = m_transform[0].m_scale;
    d->m_transform[0].m_trans = m_transform[0].m_trans;
    d->m_transform[1].m_rot = m_transform[1].m_rot;
    d->m_transform[1].m_scale = m_transform[1].m_scale;
    d->m_transform[1].m_trans = m_transform[1].m_trans;
    d->m_refMap = m_refMap;
    d->m_defaultAnim = m_defaultAnim;
    for (int i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (sc.ShareSprite(m_attach.Value(i).m_sprite) < 0)
            return -1;
    }
    for (int j = m_plays.Begin(); j != m_plays.End(); j = m_plays.Next(j))
    {
        if (sc.ShareAnimation(m_plays.Value(j).m_animID) < 0)
            return -1;
    }
    if (sc.ShareAnimation(m_defaultAnim) < 0)
        return -1;
    if (sc.ShareRefMap(m_refMap) < 0)
        return -1;
    return idx;
}

int VIHSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                       VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    for (int i = m_attach.Begin(); i != m_attach.End(); i = m_attach.Next(i))
    {
        if (sc.ReleaseSprite(m_attach.Value(i).m_sprite) < 0)
            return -1;
    }
    for (int j = m_plays.Begin(); j != m_plays.End(); j = m_plays.Next(j))
    {
        if (sc.ReleaseAnimation(m_plays.Value(j).m_animID) < 0)
            return -1;
    }
    if (m_defaultAnim != -1)
    {
        if (sc.ReleaseAnimation(m_defaultAnim) < 0)
            return -1;
    }
    if (sc.ReleaseRefMap(m_refMap) < 0)
        return -1;
    return 0;
}

int VIHSprite::UpdateCenterTransform(VIScene &sc)
{
    VIVect3 center;
    VIMatrix44 a;
    VIMatrix44 b;
    VIHSpriteFrame f;
    if (InterpTransAboutCenter(sc, center, f) < 0)
        return -1;
    m_centerMatrix.Scale(m_defaultScale);
    a.Translate(VIVect3(-center.x, -center.y, -center.z));
    b.Mul(m_centerMatrix, a);
    m_centerMatrix.Transform(f.m_rot, f.m_scale, f.m_trans);
    a.Mul(b, m_centerMatrix);
    b.Translate(center);
    m_centerMatrix.Mul(a, b);
    m_centerTime = sc.GetTime();
    return 0;
}

int VIHSprite::InterpTransAboutCenter(VIScene &sc, VIVect3 &center, VIHSpriteFrame &f)
{
    if (m_blendMS == 0 || sc.GetTime() - m_blendStart > m_blendMS)
    {
        center = m_center[1];
        f.m_rot = m_transform[1].m_rot;
        f.m_scale = m_transform[1].m_scale;
        f.m_trans = m_transform[1].m_trans;
        m_blendMS = 0;
        return 0;
    }
    float t;
    if (m_blendMS == 0)
    {
        t = 1.0f;
    }
    else
    {
        t = (float)(int)(sc.GetTime() - m_blendStart) / (float)m_blendMS;
        t = VIMIN(1.0f, VIMAX(0.0f, t));
    }
    if (m_blendMS == 0)
    {
        center = m_center[1];
    }
    else
    {
        center = m_center[1];
        center -= m_center[0];
        center *= t;
        center += m_center[0];
    }
    if (m_blendMS == 0)
    {
        f.m_rot = m_transform[1].m_rot;
        f.m_scale = m_transform[1].m_scale;
        f.m_trans = m_transform[1].m_trans;
    }
    else
    {
        f.m_rot.Slerp(m_transform[0].m_rot, m_transform[1].m_rot, t);
        f.m_scale = m_transform[0].m_scale
                  + (m_transform[1].m_scale - m_transform[0].m_scale) * t;
        f.m_trans = m_transform[1].m_trans;
        f.m_trans -= m_transform[0].m_trans;
        f.m_trans *= t;
        f.m_trans += m_transform[0].m_trans;
    }
    return 0;
}

int VIHSprite::CalcLODLevelIndex(const VIMatrix44 &m)
{
    VIVect3 d(m.m[3].x, m.m[3].y, m.m[3].z);
    float dist = d.x * d.x + d.y * d.y + d.z * d.z;
    int lvl = -1;
    for (int i = 0; i < m_lod.m_num; i++)
    {
        if (dist < m_lod.m_dist[i] * m_lod.m_dist[i])
            break;
        lvl++;
    }
    return lvl;
}

int VIHSprite::CalcLODLevelIndex(VIPickSegment &seg, VIRaster &r)
{
    VIVect3 d;
    d.x = r.GetModelView()->m[3].x - seg.m_start.x;
    d.y = r.GetModelView()->m[3].y - seg.m_start.y;
    d.z = r.GetModelView()->m[3].z - seg.m_start.z;
    float dist = d.x * d.x + d.y * d.y + d.z * d.z;
    int lvl = -1;
    for (int i = 0; i < m_lod.m_num; i++)
    {
        if (dist < m_lod.m_dist[i] * m_lod.m_dist[i])
            break;
        lvl++;
    }
    return lvl;
}

int VIHSprite::ProcessPlayTiming(int i, VIScene &sc)
{
    VIPool<VIHSpritePlay> *pool = &m_plays;
    VIHSpriteAnim *a = sc.Animation(pool->Value(i).m_animID);
    VIHSpritePlay *p = &pool->Value(i);
    // ONE `return 0` at the bottom, the stopped case falling through to it:
    // with an early return the return label splits the tail from the epilogue
    // and sched2 cannot slide `move v0,zero` behind `lq ra`.
    if (p->m_playbackType != VIPlaybackTypeStopped &&
        p->m_playbackType != VIPlaybackTypeStopEnd)
    {
        long since = sc.GetTime() - p->m_lastTime;
        float dt = (float)(int)since;
        float from;
        if (0.0f < p->m_speed)
            from = (float)p->m_frameA + p->m_frameT;
        else
            from = (float)p->m_frameA - p->m_frameT;
        if (p->m_frameRate != 0.0f)
        {
            float acc = p->m_timeAccum + dt;
            int n = (int)(acc * p->m_frameRate);
            p->m_timeAccum = acc - (float)n * p->m_frameDuration;
            if (0.0f < p->m_speed)
            {
                p->m_frameA = p->m_frameA + n;
                if (p->m_playbackType == VIPlaybackTypeOnce)
                {
                    if (a->m_numFrames - 1 < p->m_frameA)
                        p->m_frameA = a->m_numFrames - 1;
                    p->m_frameB = p->m_frameA + 1;
                    if (a->m_numFrames - 1 < p->m_frameB)
                    {
                        p->m_frameB = p->m_frameA;
                        p->m_timeAccum = 0.0f;
                        p->m_playbackType = VIPlaybackTypeStopEnd;
                    }
                }
                else
                {
                    p->m_frameA = p->m_frameA % a->m_numFrames;
                    p->m_frameB = p->m_frameA + 1;
                    if (a->m_numFrames - 1 < p->m_frameB)
                        p->m_frameB = 0;
                }
            }
            else
            {
                p->m_frameA = p->m_frameA - n;
                if (p->m_playbackType == VIPlaybackTypeOnce)
                {
                    if (p->m_frameA < 0)
                        p->m_frameA = 0;
                    p->m_frameB = p->m_frameA - 1;
                    if (p->m_frameB < 0)
                    {
                        p->m_frameB = p->m_frameA;
                        p->m_timeAccum = 0.0f;
                        p->m_playbackType = VIPlaybackTypeStopEnd;
                    }
                }
                else
                {
                    p->m_frameA = p->m_frameA % a->m_numFrames;
                    if (p->m_frameA < 0)
                        p->m_frameA = p->m_frameA + a->m_numFrames;
                    p->m_frameB = p->m_frameA - 1;
                    if (p->m_frameB < 0)
                        p->m_frameB = a->m_numFrames - 1;
                }
            }
            p->m_frameT = p->m_timeAccum * p->m_frameRate;
            if (a->m_numTriggers)
            {
                float to;
                if (0.0f < p->m_speed)
                {
                    to = (float)p->m_frameA + p->m_frameT;
                    if ((float)a->m_numFrames < to)
                        to = fmodf(to, (float)a->m_numFrames) + (float)a->m_numFrames;
                }
                else
                {
                    to = (float)p->m_frameA - p->m_frameT;
                    if (to < -(float)a->m_numFrames)
                        to = fmodf(to, (float)a->m_numFrames) - (float)a->m_numFrames;
                }
                for (int k = 0; k < a->m_numTriggers; k++)
                {
                    VIArray<VIHSpriteTrigger> *ta = &m_triggers;
                    int idx;
                    if (FindTrigger(a->m_triggers[k].m_id, idx))
                    {
                        float tf = a->m_triggers[k].m_frame;
                        if (0.0f < p->m_speed)
                        {
                            if (from < tf && tf <= to)
                            {
                                VIHSpriteTrigger *tr = &ta->m_data[idx];
                                tr->m_c = sc.GetTime()
                                    - (long)((to - tf) * p->m_frameDuration);
                                VIHSpriteTrigger *tb = &ta->m_data[idx];
                                tb->m_b = 1;
                            }
                            else
                            {
                                tf = tf + (float)a->m_numFrames;
                                if (from < tf && tf <= to)
                                {
                                    VIHSpriteTrigger *tr = &ta->m_data[idx];
                                tr->m_c = sc.GetTime()
                                    - (long)((to - tf) * p->m_frameDuration);
                                VIHSpriteTrigger *tb = &ta->m_data[idx];
                                tb->m_b = 1;
                                }
                            }
                        }
                        else
                        {
                            if (tf < from && to <= tf)
                            {
                                VIHSpriteTrigger *tr = &ta->m_data[idx];
                                tr->m_c = sc.GetTime()
                                    - (long)((tf - to) * p->m_frameDuration);
                                VIHSpriteTrigger *tb = &ta->m_data[idx];
                                tb->m_b = 1;
                            }
                            else
                            {
                                tf = tf - (float)a->m_numFrames;
                                if (tf < from && to <= tf)
                                {
                                    VIHSpriteTrigger *tr = &ta->m_data[idx];
                                tr->m_c = sc.GetTime()
                                    - (long)((tf - to) * p->m_frameDuration);
                                VIHSpriteTrigger *tb = &ta->m_data[idx];
                                tb->m_b = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        switch (p->m_blendMode)
        {
        case 0:
            p->m_weight = 0.0f;
            break;
        case 1:
            {
                // `since` and `dt` REUSED: both stay in the registers the
                // first pair had (v1, $f4); fresh locals take others.
                since = sc.GetTime() - p->m_blendStart;
                dt = (float)(int)since;
                if (p->m_blendDuration < since)
                    p->m_blendMode = 0;
                else
                    p->m_weight = dt / (float)(int)p->m_blendDuration;
            }
            break;
        }
    }
    p->m_lastTime = sc.GetTime();
    return 0;
}

// A frame copied member by member with the rotation BLOCK-moved (VIQuat's
// implicit assignment, ldl/ldr), not through VIHSpriteFrame's user operator=.
static inline void CopyFrame(VIHSpriteFrame &d, const VIHSpriteFrame &f)
{
    d.m_rot = f.m_rot;
    d.m_scale = f.m_scale;
    d.m_trans = f.m_trans;
}

// Phase 61 (306 -> 130 wrong): m_plays named at every use (loop.c hoists
// the invariant this+120 into its own register, a copy of the Begin
// argument); `&a.m_scale` a float pointer for the packed scale/translation
// (the extra callee-saved pseudo is what pushes `this` and `p` to the stack);
// e->m_c read before the lod test; the play node re-derived for the m_a == 1
// update; frames copied with the rotation block-moved.  OPEN: register
// naming in the copy and blend blocks.
int VIHSprite::ProcessPlayList(int lod, VIScene &sc)
{
    VIHSpriteFrame a;
    VIHSpriteFrame b;
    VIHSpriteFrame c;
    if (sc.GetTime() == m_playTime)
        return 0;
    m_playTime = sc.GetTime();
    for (int p = m_plays.Begin(); p != m_plays.End(); p = m_plays.Next(p))
    {
        VIHSpriteAnim *an = sc.Animation(m_plays.Value(p).m_animID);
        if (ProcessPlayTiming(p, sc) < 0)
            return -1;
        float weight = m_plays.Value(p).m_weight;
        int frameA = m_plays.Value(p).m_frameA;
        int frameB = m_plays.Value(p).m_frameB;
        float t = m_plays.Value(p).m_frameT;
        int count = m_plays.Value(p).m_numNodes;
        int pn = m_plays.Value(p).m_firstNode;
        if (count > 0)
        {
            do
            {
                VIHSpritePlayNode *e = &m_playNodes.Value(pn);
                VIHSpriteNode *n = &m_nodes.m_data[e->m_b];
                int fc = e->m_c;
                if (lod < n->m_lod)
                {
                    if (e->m_a != 2)
                    {
                        if (an->m_format == VIHSpriteAnimFormatFrame)
                        {
                            // fa/fb/pa/pb: the target's sum has the SCALED INDEX
                            // first and the base (a MEM operand, `an->m_frames`)
                            // last; `&base[i]` gives (plus base idx), and that
                            // order flows through sched and the allocator:
                            // 130 -> 17.  (-fno-force-mem on the array spelling
                            // had scored 36: it flips these same four RTL
                            // operand orders, plus 15 the target keeps.)
                            VIHSpriteFrame *fa = (VIHSpriteFrame *)(
                                (fc + frameA) * sizeof(VIHSpriteFrame) + (int)an->m_frames);
                            VIHSpriteFrame *fb = (VIHSpriteFrame *)(
                                (fc + frameB) * sizeof(VIHSpriteFrame) + (int)an->m_frames);
                            CopyFrame(a, *fa);
                            CopyFrame(b, *fb);
                        }
                        else
                        {
                            VIHSpritePackFrame *pa = (VIHSpritePackFrame *)(
                                (fc + frameA) * sizeof(VIHSpritePackFrame) + (int)an->m_frames);
                            VIHSpritePackFrame *pb = (VIHSpritePackFrame *)(
                                (fc + frameB) * sizeof(VIHSpritePackFrame) + (int)an->m_frames);
                            short *sa = &pa->m_scaleTrans.x;
                            short *sb = &pb->m_scaleTrans.x;
                            float *st = &a.m_scale;
                            a.m_rot.x = (float)pa->m_rot.x * (1.0f / 32767.0f);
                            a.m_rot.y = (float)pa->m_rot.y * (1.0f / 32767.0f);
                            a.m_rot.z = (float)pa->m_rot.z * (1.0f / 32767.0f);
                            a.m_rot.w = (float)pa->m_rot.w * (1.0f / 32767.0f);
                            st[0] = (float)sa[0] * (1.0f / 512.0f);
                            st[1] = (float)sa[1] * (1.0f / 512.0f);
                            st[2] = (float)sa[2] * (1.0f / 512.0f);
                            st[3] = (float)sa[3] * (1.0f / 512.0f);
                            b.m_rot.x = (float)pb->m_rot.x * (1.0f / 32767.0f);
                            b.m_rot.y = (float)pb->m_rot.y * (1.0f / 32767.0f);
                            b.m_rot.z = (float)pb->m_rot.z * (1.0f / 32767.0f);
                            b.m_rot.w = (float)pb->m_rot.w * (1.0f / 32767.0f);
                            b.m_scale = (float)sb[0] * (1.0f / 512.0f);
                            b.m_trans.x = (float)sb[1] * (1.0f / 512.0f);
                            b.m_trans.y = (float)sb[2] * (1.0f / 512.0f);
                            b.m_trans.z = (float)sb[3] * (1.0f / 512.0f);
                        }
                        if (m_playNodes.Value(pn).m_a == 1)
                        {
                            m_playNodes.Value(pn).m_a = 2;
                            CopyFrame(m_playNodes.Value(pn).m_frame, a);
                        }
                    }
                    else
                    {
                        CopyFrame(a, e->m_frame);
                        CopyFrame(b, m_playNodes.Value(pn).m_frame);
                    }
                    if (!m_plays.Value(p).m_interpolate)
                    {
                        c.m_rot = a.m_rot;
                        c.m_scale = a.m_scale;
                        c.m_trans.x = a.m_trans.x;
                        c.m_trans.y = a.m_trans.y;
                        c.m_trans.z = a.m_trans.z;
                    }
                    else
                    {
                        c.m_rot.Slerp(a.m_rot, b.m_rot, t);
                        c.m_scale = a.m_scale + (b.m_scale - a.m_scale) * t;
                        c.m_trans.x = (b.m_trans.x - a.m_trans.x) * t + a.m_trans.x;
                        c.m_trans.y = (b.m_trans.y - a.m_trans.y) * t + a.m_trans.y;
                        c.m_trans.z = (b.m_trans.z - a.m_trans.z) * t + a.m_trans.z;
                    }
                    switch (m_plays.Value(p).m_blendMode)
                    {
                    case 0:
                        CopyFrame(n->m_frame, c);
                        break;
                    case 1:
                        {
                        c.m_rot.Slerp(n->m_frame.m_rot, c.m_rot, weight);
                        c.m_scale = n->m_frame.m_scale
                                  + (c.m_scale - n->m_frame.m_scale) * weight;
                        c.m_trans -= n->m_frame.m_trans;
                        c.m_trans *= weight;
                        c.m_trans += n->m_frame.m_trans;
                        CopyFrame(n->m_frame, c);
                        }
                        break;
                    }
                }
                else
                {
                    CopyFrame(n->m_frame, n->m_rest);
                }
                pn = m_playNodes.Next(pn);
                count--;
            }
            while (count > 0);
        }
    }
    return 0;
}
