// VIWnd -- SUPPORT 0x004AB818 .. 0x004ABBA0.
#include "vi.h"

VIWnd *VIWnd::mFocus;

VIWnd::VIWnd()
{
    m_parent = 0;
    m_appPtr = 0;
    m_visible = 1;
}

VIWnd::~VIWnd()
{
    int i = m_children.Begin();

    while (i != m_children.End())
    {
        int next = m_children.Next(i);
        VIWnd *w = m_children.Value(i);

        RemoveChild(w);
        delete w;

        i = next;
    }
}

void VIWnd::AddChild(VIWnd *w)
{
    m_children.Insert(w);

    w->m_parent = this;
}

void VIWnd::RemoveChild(VIWnd *w)
{
    m_children.Erase(m_children.Find(w));

    w->m_parent = 0;
}

void VIWnd::SetAppPtr(void *p)
{
    m_appPtr = p;
}

void *VIWnd::GetAppPtr()
{
    return m_appPtr;
}

void VIWnd::OnDraw()
{
    OnDrawSelf();

    for (int i = m_children.Begin(); i != m_children.End(); i = m_children.Next(i))
    {
        VIWnd *w = m_children.Value(i);

        if (w->m_visible)
            w->OnDraw();
    }
}

void VIWnd::OnDrawSelf()
{
}

void VIWnd::OnKeyUp(int key)
{
    if (m_parent)
        m_parent->OnKeyUp(key);
}

void VIWnd::OnKeyDown(int key)
{
    if (m_parent)
        m_parent->OnKeyDown(key);
}

void VIWnd::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (m_parent)
        m_parent->OnMessage(w, msg, p1, p2);
}

void VIWnd::SetFocus(VIWnd *w, bool notify)
{
    if (notify)
    {
        if (mFocus)
            mFocus->OnMessage(mFocus, 295, (int)mFocus, (int)w);

        if (w)
            w->OnMessage(mFocus, 295, (int)mFocus, (int)w);
    }

    mFocus = w;
}

// Phase 61.  The five accessors that follow __tf5VIWnd in the image are vi.h's
// INLINE members.  gcc 2.95 emits a class's inline members out of line in the
// TU that emits its vtable, at the end; nothing in SUPPORT calls these copies.
