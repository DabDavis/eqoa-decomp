// viwndbeta.cpp (name ours) -- SETUP 0x0056D508 .. 0x0056E3F0.  The public beta's
// welcome notice.  Still linked in Frontiers, but VISetup no longer embeds one.
#include "setup.h"
#include "viarray_t.h"

// 0x0056D508.
VIWndBeta::VIWndBeta()
{
}

// 0x0056D548.
VIWndBeta::~VIWndBeta()
{
}

// 0x0056D598.
int VIWndBeta::Init()
{
    return 0;
}

// 0x0056D5A0.
void VIWndBeta::Free()
{
}

// 0x0056D5A8.  The app pointer is fetched and never used, here and below.
void VIWndBeta::OnDrawSelf()
{
    VIUI *ui = UI();
    VISetup *setup = (VISetup *)GetAppPtr();
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 449), (VIAlphaBlendMode)3, 1.0f);
    m_window.Draw();
    m_window.TextBox(m_window.T("Welcome to the EverQuest Online Adventures public beta test. "
                                "Thank you for your participation. Some of the features in this "
                                "version of the game are incomplete and do not reflect the quality "
                                "of the final product.  We appreciate your patience while we work "
                                "to improve the game."),
                     40, 0, m_window.Width() - 80, 6, VIUICOLOR_0, 8);
    m_window.Text(m_window.T("|x Continue"), 40, 7, VIUICOLOR_0, 0);
    ui->End2D();
}

// 0x0056D6A0.
void VIWndBeta::OnKeyUp(int key)
{
}

// 0x0056D6A8.  X dismisses it.
void VIWndBeta::OnKeyDown(int key)
{
    VISetup *setup = (VISetup *)GetAppPtr();
    if (key == 134)
    {
        SetFocus(0, true);
        UI()->SoundPlay((VIUISoundId)11);
    }
}

// 0x0056D708.
void VIWndBeta::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VISetup *setup = (VISetup *)GetAppPtr();
    if (msg == 295 && p2 == (int)this)
    {
        m_window.Close(true);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 550, 8, 0);
        m_window.SetTitle("Welcome");
        m_window.Open(false);
    }
}

// ---------------------------------------------------------------------------
// The template copies at the end of this TU.  A network header that Beta,
// Controller, Eula and Language include brings in the item types, and each of
// those TUs emits its own copy of the group below (no COMDAT); this one, the
// first linked, is the one every call resolves to.  The types are modelled
// only as far as the copies measure them -- CLIENT's game.h has the classes:
//   * VIArray<VIItemStat>: an 8-byte POD element with no constructor (the
//     Allocate loop is empty).  It is VIItem::m_stats: the betas' VINetItem is
//     756 bytes (their GetItem multiplies by 756) and Frontiers' is 764, the
//     eight bytes of this array.
//   * VIItemList<VINetItem>::GetItem, the betas' name (`GetItem__Ct10VIItemList
//     1Z9VINetItemi`): the 764-byte items start at +8.
// ---------------------------------------------------------------------------
struct VIItemStat
{
    int m_id;
    int m_value;
};

class VIItem
{
public:
    int                 m_x00[23];     // +0x000
    unsigned short      m_name[64];    // +0x05C
    unsigned short      m_desc[256];   // +0x0DC
    VIArray<VIItemStat> m_stats;       // +0x2DC
};

class VINetItem : public VIItem
{
public:
    int m_x2E4[6];                     // +0x2E4
};

template <class T> class VIItemList
{
public:
    T *GetItem(int i) const;

    int m_count;                       // +0x00
    int m_x04;                         // +0x04
    T   m_items[40];                   // +0x08
};

template <class T> T *VIItemList<T>::GetItem(int i) const
{
    return (T *)&m_items[i];
}

template VIArray<VIItemStat>::VIArray();                       // 0x0056E198
template int VIArray<VIItemStat>::DestroyPool();                // 0x0056E1A8
template VIArray<VIItemStat>::~VIArray();                      // 0x0056E1F8
template int VIArray<VIItemStat>::Clear();                      // 0x0056E260
template int VIArray<VIItemStat>::Allocate();                   // 0x0056E2C0
template int VIArray<VIItemStat>::Init(int);                    // 0x0056E338
template VINetItem *VIItemList<VINetItem>::GetItem(int) const;  // 0x0056E388
