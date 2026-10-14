// VILNode / VILList -- SUPPORT 0x003E9818 .. 0x003E9C80.
//
// The engine's intrusive list, and the first CONTAINER in this tree.  It is
// Amiga Exec's list transliterated into C++: Init() is NewList(), the header's
// three pointers are lh_Head / lh_Tail / lh_TailPred, and every walk
// terminates on a null that the header itself provides.
#include "vi.h"

extern "C" unsigned int VIHashRefID(const char *);

VILNode::VILNode()
{
    m_next = 0;
    m_prev = 0;
}

void VILNode::Remove()
{
    if (m_next && m_prev)
    {
        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
        m_next = 0;
        m_prev = 0;
    }
}

void VILNode::InsertAfter(VILNode *n)
{
    if (n->m_next == 0)
        n = n->m_prev;
    VILNode *nx = n->m_next;
    m_prev = n;
    m_next = nx;
    nx->m_prev = this;
    m_prev->m_next = this;
}

int VILNode::Index() const
{
    int i = 0;
    const VILNode *p = this;
    if (p->m_prev->m_prev)
    {
        do { p = p->m_prev; i++; } while (p->m_prev->m_prev);
    }
    return i;
}

VILList *VILNode::List() const
{
    const VILNode *p = this;
    if (p->m_prev)
    {
        do { p = p->m_prev; } while (p->m_prev);
    }
    return (VILList *)p;
}

void VILNode::SetName(const char *name)
{
    m_id = VIHashRefID(name);
}

VILList::VILList()
{
    Init();
}

VILList::~VILList()
{
}

void VILList::Init()
{
    m_head = (VILNode *)&m_tail;
    m_tail = 0;
    m_tailPred = (VILNode *)&m_head;
}

void VILList::AddHead(VILNode *n)
{
    if (n)
    {
        n->m_prev = (VILNode *)&m_head;
        n->m_next = m_head;
        n->m_next->m_prev = n;
        m_head = n;
    }
}

void VILList::AddTail(VILNode *n)
{
    if (n)
    {
        VILNode *t = m_tailPred;
        n->m_next = (VILNode *)&m_tail;
        n->m_prev = t;
        t->m_next = n;
        m_tailPred = n;
    }
}

VILNode *VILList::RemHead()
{
    VILNode *r = 0;
    VILNode *n = m_head;
    if (n->m_next)
    {
        r = n;
        n->Remove();
    }
    return r;
}

VILNode *VILList::RemTail()
{
    VILNode *r = 0;
    VILNode *n = m_tailPred;
    if (n->m_prev)
    {
        r = n;
        n->Remove();
    }
    return r;
}

int VILList::Length() const
{
    int i = 0;
    VILNode *p = m_head;
    if (p->m_next)
    {
        do { p = p->m_next; i++; } while (p->m_next);
    }
    return i;
}

int VILList::Index(VILNode *n) const
{
    int i = 0;
    for (VILNode *p = m_head; p->m_next; p = p->m_next)
    {
        if (p == n)
            return i;
        i++;
    }
    return -1;
}

// The next node is CACHED in `n` and re-read through a temporary the loop
// tests: `lw v0,0(v1); move v1,v0; bnez v0` needs `t` live past the copy, so
// it cannot be coalesced with `n`.  Walking `p = p->m_next` reloads the link at
// the step (this gcse never PREs a load through a pseudo), and `n = n->m_next`
// tested as `n` loads straight into n's register.  The read comes BEFORE
// `p = n`, else cse bases it on p.  `break`, not `return 0`, shares the tail.
VILNode *VILList::Node(int index) const
{
    int i = 0;
    VILNode *p = m_head;
    VILNode *n, *t;
    if (index >= 0 && (n = p->m_next) != 0)
    {
        do {
            if (i == index)
                return p;
            i++;
            if (index < i)
                break;
            t = n->m_next;
            p = n;
            n = t;
        } while (t);
    }
    return 0;
}

VILNode *VILList::Find(const char *name) const
{
    return Find(VIHashRefID(name));
}

VILNode *VILList::Find(unsigned int id) const
{
    for (VILNode *p = m_head; p->m_next; p = p->m_next)
    {
        if (p->m_id == id)
            return p;
    }
    return 0;
}

void VILList::Reverse()
{
    if (Length() < 2)
        return;
    VILNode *p = m_head->m_next;
    while (p->m_next)
    {
        VILNode *n = p->m_next;
        p->Remove();
        AddHead(p);
        p = n;
    }
}

void VILList::PriorityInsert(VILNode *n, int dir)
{
    VILNode *p = m_head;
    while (p->m_next)
    {
        // Both comparisons put p on the LEFT: the target loads p->m_pri first
        // in BOTH cases, which is what decides the two loads' order.  dir 0 is
        // ascending, dir 1 descending; any other value never inserts, and the
        // node goes on the tail.
        switch (dir)
        {
        case 0:
            if (p->m_pri >= n->m_pri) { n->InsertAfter(p->m_prev); return; }
            break;
        case 1:
            if (p->m_pri <= n->m_pri) { n->InsertAfter(p->m_prev); return; }
            break;
        }
        p = p->m_next;
    }
    AddTail(n);
}
