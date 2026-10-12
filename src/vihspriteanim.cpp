// VIHSpriteAnim -- SUPPORT 0x0044D258 .. 0x0044D828.
//
// One skeletal animation clip.  Structurally it is the third of the engine's
// carve-one-block containers, after VICollBuffer (Phase 13) and VISurface
// (Phase 16): CalcDataSize writes byte OFFSETS into the three section
// pointers, Allocate takes one `new char[]`, and Init rebases all three onto
// m_data in place.
//
// The frame side is the fourth class whose semantics were settled from the
// console before the decomp -- `go-eqoa-vi/vianim` reads both frame formats
// bit-exact out of a live game -- so a miss here is codegen.
#include "vi.h"

VIHSpriteAnim::VIHSpriteAnim()
{
    m_data = 0;
    Clear();
}

VIHSpriteAnim::~VIHSpriteAnim()
{
    Clear();
}

int VIHSpriteAnim::Init(VIHSpriteAnimFormat format, int numNodes, int numFrames,
                        int numTriggers, float playRate, float playSpeed,
                        VIPlaybackType type)
{
    m_format = format;
    m_numNodes = numNodes;
    m_numFrames = numFrames;
    m_numTriggers = numTriggers;
    m_playRate = playRate;
    m_playSpeed = playSpeed;
    m_playbackType = type;
    if (CalcDataSize() < 0)
        return -1;
    if (Allocate() < 0)
        return -1;
    m_nodes = (VIHSpriteAnimNode *)((int)m_data + (int)m_nodes);
    m_frames = (char *)((int)m_data + (int)m_frames);
    m_triggers = (VIHSpriteKeyframe *)((int)m_data + (int)m_triggers);
    int f = 0;
    for (int i = 0; i < m_numNodes; i++)
    {
        VIHSpriteAnimNode *n =
            (VIHSpriteAnimNode *)(i * 8 + (unsigned int)m_nodes);
        n->m_firstFrame = f;
        f += m_numFrames;
    }
    return 0;
}

int VIHSpriteAnim::Clear()
{
    if (m_data)
    {
        delete [] m_data;
        m_data = 0;
    }
    // Phase 16's hoist: the first store emitted after the join is the source's
    // LAST store of that constant, so m_dataSize goes at the bottom.
    m_playRate = 10.0f;
    m_playSpeed = 1.0f;
    m_format = VIHSpriteAnimFormatFrame;
    m_numNodes = 0;
    m_numFrames = 0;
    m_playbackType = VIPlaybackTypeLoop;
    m_nodes = 0;
    m_frames = 0;
    m_triggers = 0;
    m_dataSize = 0;
    return 0;
}

int VIHSpriteAnim::GetNumFramesPerNode() const
{
    return m_numFrames;
}

float VIHSpriteAnim::GetPlayRate() const
{
    return m_playRate;
}

int VIHSpriteAnim::SetPlayRate(float r)
{
    m_playRate = r;
    return 0;
}

// A shipped bug: it returns the play RATE.  m_playSpeed is at +0x1C and this
// reads +0x18.
float VIHSpriteAnim::GetPlaySpeed() const
{
    return m_playRate;
}

int VIHSpriteAnim::SetPlaySpeed(float s)
{
    m_playSpeed = s;
    return 0;
}

VIPlaybackType VIHSpriteAnim::GetPlayMode() const
{
    return m_playbackType;
}

int VIHSpriteAnim::SetPlaybackType(VIPlaybackType t)
{
    m_playbackType = t;
    return 0;
}

int VIHSpriteAnim::GetNodeRefID(int node)
{
    return ((VIHSpriteAnimNode *)(node * 8 + (unsigned int)m_nodes))->m_refID;
}

int VIHSpriteAnim::SetNodeRefID(int node, int id)
{
    ((VIHSpriteAnimNode *)(node * 8 + (unsigned int)m_nodes))->m_refID = id;
    return 0;
}

int VIHSpriteAnim::SetNodeFrame(int node, int frame, VIHSpriteFrame &f)
{
    // Two levers, worth 7 words each.  `m_frames` is read BEFORE the guard --
    // its load sits in the first basic block, ahead of the branch, where no
    // scheduler could have put it -- and the frame index is accumulated INTO
    // the parameter, which is Phase 9's assign-then-modify-in-place: a fresh
    // temp costs the whole index chain its register assignment.
    char *fb = m_frames;
    if (m_format != VIHSpriteAnimFormatFrame)
        return -1;
    VIHSpriteAnimNode *n =
        (VIHSpriteAnimNode *)(node * 8 + (unsigned int)m_nodes);
    frame += n->m_firstFrame;
    VIHSpriteFrame *d =
        (VIHSpriteFrame *)(frame * 32 + (unsigned int)fb);
    // Written out, not `*d = f`: VIHSpriteFrame's operator= copies the
    // rotation component by component and the target block-moves it here.
    d->m_rot = f.m_rot;
    d->m_scale = f.m_scale;
    d->m_trans = f.m_trans;
    return 0;
}

int VIHSpriteAnim::SetNodeFrame(int node, int frame, VIHSpritePackFrame &f)
{
    char *fb = m_frames;
    if (m_format != VIHSpriteAnimFormatPack)
        return -1;
    VIHSpriteAnimNode *n =
        (VIHSpriteAnimNode *)(node * 8 + (unsigned int)m_nodes);
    frame += n->m_firstFrame;
    VIHSpritePackFrame *d =
        (VIHSpritePackFrame *)(frame * 16 + (unsigned int)fb);
    *d = f;
    return 0;
}

int VIHSpriteAnim::GetNodeFrame(int node, int frame, VIHSpriteFrame &f)
{
    VIHSpriteAnimNode *n =
        (VIHSpriteAnimNode *)(node * 8 + (unsigned int)m_nodes);
    int i = frame + n->m_firstFrame;
    switch (m_format)
    {
    case VIHSpriteAnimFormatFrame:
    {
        VIHSpriteFrame *s =
            (VIHSpriteFrame *)(i * 32 + (unsigned int)m_frames);
        f.m_rot = s->m_rot;
        f.m_scale = s->m_scale;
        f.m_trans = s->m_trans;
        break;
    }
    case VIHSpriteAnimFormatPack:
    {
        VIHSpritePackFrame *s =
            (VIHSpritePackFrame *)(i * 16 + (unsigned int)m_frames);
        // The scale and the three translation components share ONE
        // dequantisation scale, and the author walks them as one four-element
        // run off a pointer -- which is Phase 9's lever and is what puts the
        // first of the four back on the outer base (16(a3) / 8(v1)) while the
        // other three come off `d` and `p`.  Written as four member
        // assignments the run collapses onto the base and the two `addiu`s
        // that materialise the pointers disappear (10 words wrong).
        // The unpacked frame keeps the scale and the translation apart and
        // the packed one does not, so the four values that share the 1/512
        // scale are walked as a RUN on both sides.  That run is Phase 9's
        // lever: the first of the four folds back onto the outer base
        // (16(a3) / 8(v1)) and the other three come off the pointer.  Written
        // as member assignments both pointers vanish -- 10 words wrong.
        float *d = &f.m_scale;
        short *p = &s->m_scaleTrans.x;
        f.m_rot.x = s->m_rot.x * (1.0f / 32767.0f);
        f.m_rot.y = s->m_rot.y * (1.0f / 32767.0f);
        f.m_rot.z = s->m_rot.z * (1.0f / 32767.0f);
        f.m_rot.w = s->m_rot.w * (1.0f / 32767.0f);
        d[0] = p[0] * (1.0f / 512.0f);
        d[1] = p[1] * (1.0f / 512.0f);
        d[2] = p[2] * (1.0f / 512.0f);
        d[3] = p[3] * (1.0f / 512.0f);
        break;
    }
    default:
        return -1;
    }
    return 0;
}

int VIHSpriteAnim::SetKeyframe(int i, int id, float frame)
{
    ((VIHSpriteKeyframe *)(i * 8 + (unsigned int)m_triggers))->m_frame = frame;
    ((VIHSpriteKeyframe *)(i * 8 + (unsigned int)m_triggers))->m_id = id;
    return 0;
}

int VIHSpriteAnim::CalcDataSize()
{
    // Phase 14's lever twice over.  The three sections are sized and stored
    // INSIDE each case -- written once after the switch, the two arms cannot
    // hold their own `sll` and the whole tail changes shape -- and the running
    // offset is a local declared inside the case, not one shared by both:
    // sharing it costs the function its entire register assignment (22 of 35
    // words, with an instruction stream that is otherwise identical).
    switch (m_format)
    {
    case VIHSpriteAnimFormatFrame:
    {
        int off = m_numNodes * 8;
        m_frames = (char *)off;
        m_nodes = 0;
        off += m_numNodes * m_numFrames * 32;
        m_triggers = (VIHSpriteKeyframe *)off;
        m_dataSize = off + m_numTriggers * 8;
        break;
    }
    case VIHSpriteAnimFormatPack:
    {
        int off = m_numNodes * 8;
        m_frames = (char *)off;
        m_nodes = 0;
        off += m_numNodes * m_numFrames * 16;
        m_triggers = (VIHSpriteKeyframe *)off;
        m_dataSize = off + m_numTriggers * 8;
        break;
    }
    default:
        return -1;
    }
    return 0;
}

int VIHSpriteAnim::Allocate()
{
    if (m_dataSize)
    {
        m_data = new char[m_dataSize];
        if (m_data == 0)
            return -1;
    }
    return 0;
}
