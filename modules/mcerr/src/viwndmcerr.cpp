// VIWndMcErr.cpp (name ours: the image has no asserts) -- MCERR 0x002A8280 ..
// 0x002AB638, the whole of the module's own code.  One window class and the
// session key it keeps on the memory card.  The functions are in address order,
// which is source order.
#include "mcerr.h"

// The window every entry point drives.  Its constructor and destructor are the
// module's static initialisation (`_GLOBAL_$I$__10VIWndMcErr`).
static VIWndMcErr sMcErrWnd;

// 0x002A8280.  The VIVect2I default constructor is what zeroes m_center.
VIWndMcErr::VIWndMcErr()
{
}

// 0x002A82C8.
VIWndMcErr::~VIWndMcErr()
{
}

// 0x002A8318.
int VIWndMcErr::Init()
{
    m_sel = 0;
    return 0;
}

// 0x002A8328.
void VIWndMcErr::Free()
{
}

// 0x002A8330.
void VIWndMcErr::OnDrawSelf()
{
    ProcessScreen(0, 0, 0);
}

// 0x002A8358.
void VIWndMcErr::OnKeyDown(int key)
{
    ProcessScreen(3, key, 0);
}

// 0x002A8380.
void VIWndMcErr::OnKeyUp(int key)
{
    ProcessScreen(4, key, 0);
}

// 0x002A83A8.  295 is VIWnd::SetFocus's notice (p1 the old focus, p2 the new
// one): gaining the focus re-opens the window at a fixed size.
void VIWndMcErr::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295 && p2 == (int)this)
    {
        m_window.Close(true);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 380, 7, 0);
        m_window.Open(false);
    }
}

// 0x002A8430.
void VIWndMcErr::GotoScreen(int screen)
{
    m_screen = screen;
    m_count = 0;
    m_sel = 0;
}

// 0x002A8440.  A child window tells its owner; a synchronous one leaves the
// result for McErrSyncIdle to return.
void VIWndMcErr::ExitMcErr(int result)
{
    if (m_owner)
    {
        m_owner->OnMessage(this, 0x1102, 0, 0);
        m_owner->OnMessage(this, 0x1106, result, 0);
    }
    else
        m_result = result;
}

// 0x002A84D0.  Screen 25 still has a body (0x002A9A00) but no case.
int VIWndMcErr::ProcessScreen(int msg, int key, int p)
{
    switch (m_screen)
    {
    case 4:  ProcessScreen04(msg, key, p); break;
    case 10: ProcessScreen10(msg, key, p); break;
    case 11: ProcessScreen11(msg, key, p); break;
    case 12: ProcessScreen12(msg, key, p); break;
    case 15: ProcessScreen15(msg, key, p); break;
    case 16: ProcessScreen16(msg, key, p); break;
    case 17: ProcessScreen17(msg, key, p); break;
    case 20: ProcessScreen20(msg, key, p); break;
    case 21: ProcessScreen21(msg, key, p); break;
    case 22: ProcessScreen22(msg, key, p); break;
    case 30: ProcessScreen30(msg, key, p); break;
    case 50: ProcessScreen50(msg, key, p); break;
    case 52: ProcessScreen52(msg, key, p); break;
    case 54: ProcessScreen54(msg, key, p); break;
    case 60: ProcessScreen60(msg, key, p); break;
    case 61: ProcessScreen61(msg, key, p); break;
    case 62: ProcessScreen62(msg, key, p); break;
    case 63: ProcessScreen63(msg, key, p); break;
    case 64: ProcessScreen64(msg, key, p); break;
    }
    return 0;
}

// 0x002A8640.  Checking the card: forty frames, then a verdict.
int VIWndMcErr::ProcessScreen04(int msg, int key, int p)
{
    VIUI *ui = UI();
    if (msg == 0)
    {
        int rows = m_window.CountLines(Word(UI(), 951), 460);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 500, rows, 0);
        m_window.Open(true);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(Word(UI(), 951), 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        if (++m_count == 40)
        {
            switch (VIMemCardStatus(0))
            {
            case 0:
            case 3:
                if (VIIsMcSameSession() == 1)
                {
                    printf("SAME SESSION\n");
                    ExitMcErr(1);
                }
                else
                {
                    printf("DIFFERENT SESSION\n");
                    if (m_mode == 1)
                        GotoScreen(17);
                    else
                        GotoScreen(11);
                }
                break;
            case 1:
                if (m_mode == 1)
                    GotoScreen(12);
                else
                    GotoScreen(10);
                break;
            case 2:
            case 4:
                if (m_mode == 1)
                    GotoScreen(17);
                else
                    GotoScreen(11);
                break;
            case 5:
                if (m_mode == 1)
                    GotoScreen(12);
                else
                    GotoScreen(10);
                break;
            }
        }
        ui->End2D();
    }
    return 0;
}

// 0x002A8850.
int VIWndMcErr::ProcessScreen10(int msg, int key, int p)
{
    VIUI *ui = UI();
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(Word(UI(), 962), 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        m_window.Open(true);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(Word(UI(), 962), 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), 0);
        ui->End2D();
        break;
    }
    case 3:
    {
        if (key == 13 || key == 134)
        {
            switch (VIMemCardStatus(0))
            {
            case 1:
            case 5:
                GotoScreen(15);
                break;
            default:
                GotoScreen(4);
                break;
            }
        }
        break;
    }
    }
    return 0;
}

// 0x002A89E8.
int VIWndMcErr::ProcessScreen11(int msg, int key, int p)
{
    VIUI *ui = UI();
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(Word(UI(), 963), 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        m_window.Open(true);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(Word(UI(), 963), 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), 0);
        ui->End2D();
        break;
    }
    case 3:
    {
        if (key == 13 || key == 134)
        {
            switch (VIMemCardStatus(0))
            {
            case 0:
            case 3:
                if (VIIsMcSameSession() == 1)
                    ExitMcErr(1);
                else
                    GotoScreen(16);
                break;
            case 2:
            case 4:
                GotoScreen(16);
                break;
            default:
                GotoScreen(4);
                break;
            }
        }
        break;
    }
    }
    return 0;
}

// 0x002A8BD0.  A yes/no prompt; the first frame lights "no".
int VIWndMcErr::ProcessScreen12(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[1024];
    VIString::snprintf16(buf, sizeof(buf), "%S %S", Word(ui, 1611), Word(ui, 1613));
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(buf, 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        m_window.Open(true);
        ui->Begin2D();
        if (++m_count == 1)
            m_sel = 1;
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 21), Word(UI(), 22));
        ui->End2D();
        break;
    }
    case 3:
    {
        switch (key)
        {
        case 13:
        case 134:
            if (m_sel == 0)
                ExitMcErr(0);
            else
                GotoScreen(4);
            break;
        case 17:
        case 136:
            m_sel = 0;
            break;
        case 18:
        case 137:
            m_sel = 1;
            break;
        }
        break;
    }
    }
    return 0;
}

// 0x002A8E18.
int VIWndMcErr::ProcessScreen15(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[512];
    switch (msg)
    {
    case 0:
    {
        if (VIGetExpansionFlags() & 2)
            VIString::snprintf16(buf, sizeof(buf), Word(UI(), 955), Word(UI(), 1803));
        else
            VIString::snprintf16(buf, sizeof(buf), Word(UI(), 955), Word(UI(), 1802));
        int rows = m_window.CountLines(buf, 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        m_window.Open(true);
        ui->Begin2D();
        if (++m_count == 1)
            m_sel = 1;
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 969), Word(UI(), 896));
        ui->End2D();
        break;
    }
    case 3:
    {
        switch (key)
        {
        case 13:
        case 134:
            if (m_sel == 0)
                ExitMcErr(3);
            else
                GotoScreen(4);
            break;
        case 17:
        case 136:
            m_sel = 0;
            break;
        case 18:
        case 137:
            m_sel = 1;
            break;
        }
        break;
    }
    }
    return 0;
}

// 0x002A9098.
int VIWndMcErr::ProcessScreen16(int msg, int key, int p)
{
    VIUI *ui = UI();
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(Word(UI(), 970), 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        m_window.Open(true);
        ui->Begin2D();
        if (++m_count == 1)
            m_sel = 1;
        m_window.Draw();
        m_window.TextBox(Word(UI(), 970), 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 969), Word(UI(), 896));
        ui->End2D();
        break;
    }
    case 3:
    {
        switch (key)
        {
        case 13:
        case 134:
            if (m_sel == 0)
                ExitMcErr(3);
            else
                GotoScreen(4);
            break;
        case 17:
        case 136:
            m_sel = 0;
            break;
        case 18:
        case 137:
            m_sel = 1;
            break;
        }
        break;
    }
    }
    return 0;
}

// 0x002A92B0.  Screen 12 with a different first word.
int VIWndMcErr::ProcessScreen17(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[1024];
    VIString::snprintf16(buf, sizeof(buf), "%S %S", Word(ui, 1612), Word(ui, 1613));
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(buf, 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        m_window.Open(true);
        ui->Begin2D();
        if (++m_count == 1)
            m_sel = 1;
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 21), Word(UI(), 22));
        ui->End2D();
        break;
    }
    case 3:
    {
        switch (key)
        {
        case 13:
        case 134:
            if (m_sel == 0)
                ExitMcErr(0);
            else
                GotoScreen(4);
            break;
        case 17:
        case 136:
            m_sel = 0;
            break;
        case 18:
        case 137:
            m_sel = 1;
            break;
        }
        break;
    }
    }
    return 0;
}

// 0x002A94F8.  1802/1803 is the game's name, picked by the expansion flag.
int VIWndMcErr::ProcessScreen20(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[1024];
    unsigned short line[512];
    VIString::snprintf16(line, sizeof(line), Word(UI(), 957),
                         Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
    VIString::snprintf16(buf, sizeof(buf), "%S\n\n%S", Word(ui, 1626), line);
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(buf, 430);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 470, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), 0);
        ui->End2D();
        break;
    }
    case 3:
    {
        if (key == 13 || key == 134)
            GotoScreen(30);
        break;
    }
    }
    return 0;
}

// 0x002A96D0.
int VIWndMcErr::ProcessScreen21(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[1024];
    VIString::snprintf16(buf, sizeof(buf), Word(UI(), 1670),
                         Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(buf, 430);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 470, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), 0);
        ui->End2D();
        break;
    }
    case 3:
    {
        if (key == 13 || key == 134)
            GotoScreen(30);
        break;
    }
    }
    return 0;
}

// 0x002A9868.
int VIWndMcErr::ProcessScreen22(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[1024];
    VIString::snprintf16(buf, sizeof(buf), Word(UI(), 1801),
                         Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(buf, 430);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 470, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), 0);
        ui->End2D();
        break;
    }
    case 3:
    {
        if (key == 13 || key == 134)
            GotoScreen(30);
        break;
    }
    }
    return 0;
}

// 0x002A9A00.  No case in ProcessScreen reaches it.
int VIWndMcErr::ProcessScreen25(int msg, int key, int p)
{
    VIUI *ui = UI();
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(Word(UI(), 961), 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(Word(UI(), 961), 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), 0);
        ui->End2D();
        break;
    }
    case 3:
    {
        if (key == 13 || key == 134)
            GotoScreen(30);
        break;
    }
    }
    return 0;
}

// 0x002A9B60.  Looking at the card again, after 75 frames.
int VIWndMcErr::ProcessScreen30(int msg, int key, int p)
{
    VIUI *ui = UI();
    if (msg == 0)
    {
        int rows = m_window.CountLines(Word(UI(), 951), 460);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 500, rows, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(Word(UI(), 951), 20, 0, m_window.Width() - 40, 256, VIUICOLOR_0, 8);
        if (++m_count == 75)
        {
            int free;
            switch (VIMemCardStatus(&free))
            {
            case 0:
            case 3:
                if (VIMemCardIsPrepared() == 1)
                    GotoScreen(63);
                else if (free < 3000)
                    GotoScreen(60);
                else
                    GotoScreen(61);
                break;
            case 1:
                GotoScreen(20);
                break;
            case 2:
            case 4:
                printf("NOT FORMATTED\n");
                GotoScreen(50);
                break;
            case 5:
                GotoScreen(20);
                break;
            }
        }
        ui->End2D();
    }
    return 0;
}

// 0x002A9D20.  Format?  Keys count only once the screen has drawn.
int VIWndMcErr::ProcessScreen50(int msg, int key, int p)
{
    VIUI *ui = UI();
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(Word(UI(), 964), 310);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 350, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(Word(UI(), 964), 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        if (m_count == 0)
            m_sel = 1;
        m_count++;
        DrawPrompt(rows + 1, Word(UI(), 21), Word(UI(), 22));
        ui->End2D();
        break;
    }
    case 3:
    {
        if (m_count > 0)
        {
            switch (key)
            {
            case 13:
            case 134:
                if (m_sel == 0)
                {
                    if (VIMemCardStatus(0) == 2)
                        GotoScreen(52);
                    else
                        GotoScreen(54);
                }
                else
                    GotoScreen(21);
                break;
            case 17:
            case 136:
                m_sel = 0;
                break;
            case 18:
            case 137:
                m_sel = 1;
                break;
            }
        }
        break;
    }
    }
    return 0;
}

// 0x002A9F58.  Formatting, ten frames in.
int VIWndMcErr::ProcessScreen52(int msg, int key, int p)
{
    VIUI *ui = UI();
    if (msg == 0)
    {
        int rows = m_window.CountLines(Word(UI(), 959), 540);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 580, rows, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(Word(UI(), 959), 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        if (++m_count == 10)
        {
            if (VIMemCardFormat() < 0)
                GotoScreen(54);
            else
                GotoScreen(30);
        }
        ui->End2D();
    }
    return 0;
}

// 0x002AA088.
int VIWndMcErr::ProcessScreen54(int msg, int key, int p)
{
    VIUI *ui = UI();
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(Word(UI(), 958), 310);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 350, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(Word(UI(), 958), 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), 0);
        ui->End2D();
        break;
    }
    case 3:
    {
        if (key == 13 || key == 134)
        {
            if (VIMemCardStatus(0) == 1)
                GotoScreen(20);
            else
                GotoScreen(30);
        }
        break;
    }
    }
    return 0;
}

// 0x002AA208.  Not enough room on the card.
int VIWndMcErr::ProcessScreen60(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[512];
    VIString::snprintf16(buf, sizeof(buf), Word(UI(), 960),
                         Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(buf, 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), Word(UI(), 753));
        ui->End2D();
        break;
    }
    case 3:
    {
        switch (key)
        {
        case 13:
        case 134:
            if (m_sel == 1)
                GotoScreen(21);
            else if (VIMemCardStatus(0) == 1)
                GotoScreen(20);
            else
                GotoScreen(30);
            break;
        case 17:
        case 136:
            m_sel = 0;
            break;
        case 18:
        case 137:
            m_sel = 1;
            break;
        }
        break;
    }
    }
    return 0;
}

// 0x002AA458.  Create the save?
int VIWndMcErr::ProcessScreen61(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[1024];
    unsigned short line1[512];
    unsigned short line2[512];
    VIString::snprintf16(line1, sizeof(line1), Word(UI(), 974),
                         Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
    VIString::snprintf16(line2, sizeof(line2), Word(ui, 956),
                         Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
    VIString::snprintf16(buf, sizeof(buf), "%S\n\n%S\n\n%S", line1, line2, Word(ui, 967));
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(buf, 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 21), Word(UI(), 22));
        ui->End2D();
        break;
    }
    case 3:
    {
        switch (key)
        {
        case 13:
        case 134:
            if (m_sel == 1)
                GotoScreen(22);
            else if (VIMemCardStatus(0) != 0)
                GotoScreen(30);
            else
                GotoScreen(62);
            break;
        case 17:
        case 136:
            m_sel = 0;
            break;
        case 18:
        case 137:
            m_sel = 1;
            break;
        }
        break;
    }
    }
    return 0;
}

#define MCERR_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MCERR_MAX(a, b) ((a) < (b) ? (b) : (a))

// 0x002AA750.  Creating the save: pad the card asynchronously, then write the
// product file and the options.
//
// Three facts in the image fix the shape.  Every failure on frame 5 jumps
// straight to the drawing, past the `m_count >= 6` test that the success path
// falls into -- hence the gotos.  Every GotoScreen(64) is ONE block, the one in
// Save's failure arm: jump2 cross-jumps identical `GotoScreen(64); b draw` tails
// into the last copy, so case -1 is written ahead of case 1.  And case 0 keeps
// a division by 20 whose result is dead: the div goes, its zero trap stays.
int VIWndMcErr::ProcessScreen62(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[512];
    VIString::snprintf16(buf, sizeof(buf), Word(UI(), 968),
                         Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
    if (msg == 0)
    {
        int rows = m_window.CountLines(buf, 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 1, 0);
        ui->Begin2D();
        m_window.Draw();
        if (++m_count == 5)
        {
            int free;
            if (VIMemCardStatus(0) != 0)
            {
                GotoScreen(64);
                goto draw;
            }
            VIMemCardStatus(&free);
            if (free < 3000)
            {
                GotoScreen(60);
                goto draw;
            }
            if (VIMemCardPrepare(-1) < 0)
            {
                GotoScreen(64);
                goto draw;
            }
            if (VIMemCardPadAsync(2996) < 0)
            {
                GotoScreen(64);
                goto draw;
            }
        }
        if (m_count >= 6)
        {
            switch (VIMemCardAsyncIdle())
            {
            case 0:
                {
                    int pct = m_count / 20;
                }
                break;
            case -1:
                GotoScreen(64);
                break;
            case 1:
                {
                    printf("FINISHED: %d\n", m_count);
                    const char *data = "12345678";
                    if (VIMemCardWriteFile(VIMemCardProductId(), data, 8) < 0)
                        GotoScreen(64);
                    else
                        GotoScreen(63);
                    if (VIGetGameOptions()->Save() < 0)
                        GotoScreen(64);
                    else
                        GotoScreen(63);
                }
                break;
            }
        }
    draw:
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        float t = m_count / 640.0f;
        t = MCERR_MIN(MCERR_MAX(t, 0.0f), 1.0f);
        m_window.DrawProgress(40, rows, m_window.Width() - 80, m_count < 6 ? 0.0f : t * 0.9f + 0.1f);
        ui->End2D();
    }
    return 0;
}

// 0x002AAA68.  Done: write the session key.
int VIWndMcErr::ProcessScreen63(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[1024];
    unsigned short line[512];
    VIString::snprintf16(line, sizeof(line), Word(UI(), 973),
                         Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
    VIString::snprintf16(buf, sizeof(buf), "%S\n\n%S", line, Word(ui, 965));
    switch (msg)
    {
    case 0:
    {
        int rows = m_window.CountLines(buf, 510);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 550, rows + 2, 0);
        ui->Begin2D();
        if (++m_count == 1)
        {
            int free;
            if (VIMemCardStatus(&free) != 0)
                GotoScreen(64);
            else
            {
                unsigned long id = VITimer::GetTime() + free;
                VISetMcSessionId(id);
                printf("Writing session key (%ld) to memory card.\n", id);
                if (VIMemCardWriteFile("session.id", &id, 8) < 0)
                    GotoScreen(64);
            }
        }
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 786), 0);
        ui->End2D();
        break;
    }
    case 3:
    {
        if (m_count >= 2)
        {
            if (key == 13 || key == 134)
                ExitMcErr(1);
        }
        break;
    }
    }
    return 0;
}

// 0x002AACE8.  The card failed.
int VIWndMcErr::ProcessScreen64(int msg, int key, int p)
{
    VIUI *ui = UI();
    unsigned short buf[512];
    switch (msg)
    {
    case 0:
    {
        if (VIGetExpansionFlags() & 2)
            VIString::snprintf16(buf, sizeof(buf), Word(UI(), 949), Word(UI(), 1803));
        else
            VIString::snprintf16(buf, sizeof(buf), Word(UI(), 949), Word(UI(), 1802));
        int rows = m_window.CountLines(buf, 450);
        m_window.Init(UI(), VIVect2I(0, 0), m_center, 490, rows + 2, 0);
        ui->Begin2D();
        m_window.Draw();
        m_window.TextBox(buf, 20, 0, m_window.Width() - 40, rows, VIUICOLOR_0, 8);
        DrawPrompt(rows + 1, Word(UI(), 896), Word(UI(), 753));
        ui->End2D();
        break;
    }
    case 3:
    {
        switch (key)
        {
        case 13:
        case 134:
            if (m_sel == 0)
            {
                switch (VIMemCardStatus(0))
                {
                case 1:
                    GotoScreen(20);
                    break;
                case 0:
                    GotoScreen(62);
                    break;
                case 2:
                case 3:
                case 4:
                case 5:
                    GotoScreen(30);
                    break;
                }
            }
            else
                GotoScreen(30);
            break;
        case 17:
        case 136:
            m_sel = 0;
            break;
        case 18:
        case 137:
            m_sel = 1;
            break;
        }
        break;
    }
    }
    return 0;
}

// 0x002AAF98.  One centred prompt, or two at the thirds; `|q`..`|w` lights the
// selected one.
void VIWndMcErr::DrawPrompt(int row, const unsigned short *a, const unsigned short *b)
{
    VIUIWindow &w = m_window;
    int third = m_window.Width() / 3;
    int half = m_window.Width() >> 1;
    unsigned short buf[256];
    if (b)
    {
        if (m_sel == 0)
        {
            VIString::str8to16(buf, sizeof(buf), "|q");
            VIString::strcat16(buf, sizeof(buf), a);
            VIString::strcat16(buf, sizeof(buf), "|w");
        }
        else
            VIString::strcpy16(buf, sizeof(buf), a);
        m_window.TextBox(buf, third - 200, row, 400, 1, (VIUICOLOR)(m_sel == 0), 8);
        if (m_sel == 1)
        {
            VIString::str8to16(buf, sizeof(buf), "|q");
            VIString::strcat16(buf, sizeof(buf), b);
            VIString::strcat16(buf, sizeof(buf), "|w");
        }
        else
            VIString::strcpy16(buf, sizeof(buf), b);
        m_window.TextBox(buf, third * 2 - 200, row, 400, 1, (VIUICOLOR)(m_sel == 1), 8);
    }
    else
    {
        VIString::str8to16(buf, sizeof(buf), "|q");
        VIString::strcat16(buf, sizeof(buf), a);
        VIString::strcat16(buf, sizeof(buf), "|w");
        m_window.TextBox(buf, half - 200, row, 400, 1, (VIUICOLOR)1, 8);
    }
}

// 0x002AB190.
VIWndMcErr *McErrWnd()
{
    return &sMcErrWnd;
}

// 0x002AB1A0.  Open the window as a child of `owner`, which is told twice
// (0x1100, then 0x1104) with the window as p1; ExitMcErr's 0x1102/0x1106 undo it.
int McErr(VIWnd *owner, const VIVect2I &center, int screen)
{
    sMcErrWnd.m_screen = screen;
    sMcErrWnd.m_center = center;
    sMcErrWnd.m_owner = owner;
    sMcErrWnd.m_mode = 0;
    owner->OnMessage(&sMcErrWnd, 0x1100, (int)&sMcErrWnd, 0);
    owner->OnMessage(&sMcErrWnd, 0x1104, (int)&sMcErrWnd, 0);
    sMcErrWnd.GotoScreen(screen);
    return 0;
}

// 0x002AB258.
void McErrSetMode(int mode)
{
    sMcErrWnd.m_mode = mode;
}

// 0x002AB268.  Drive the window from the caller's own loop.
int McErrSyncSet(const VIVect2I &center, int screen)
{
    sMcErrWnd.m_center = center;
    sMcErrWnd.m_owner = 0;
    sMcErrWnd.m_result = 0;
    sMcErrWnd.m_mode = 0;
    sMcErrWnd.GotoScreen(screen);
    memset(&sMcErrWnd.m_pad, 0, sizeof(sMcErrWnd.m_pad));
    return 0;
}

// 0x002AB2D0.  One frame: draw, then every key and every pad button that
// changed since the last frame (a button is key 128 + its index).
int McErrSyncIdle(VIPad *pad, VIKeyboard *kb)
{
    VIConsolePad p;
    VIConsoleKey k;
    sMcErrWnd.OnDrawSelf();
    if (kb && kb->Read(k) == 1)
    {
        if (k.m_b)
            sMcErrWnd.OnKeyDown(k.m_a);
        else
            sMcErrWnd.OnKeyUp(k.m_a);
    }
    if (pad && pad->Read(p))
    {
        for (int i = 0; i < 20; i++)
        {
            // The image keeps &sMcErrWnd itself in a register and makes the
            // pointer `+ 460` from it; a direct `&sMcErrWnd.m_pad` folds the
            // offset into the address and the calls' `this` is derived instead.
            VIWndMcErr *w = &sMcErrWnd;
            float *was = &w->m_pad.m_rx;
            float *now = &p.m_rx;
            if ((was[i] != 0.0f) != (now[i] != 0.0f))
            {
                if (now[i] != 0.0f)
                    sMcErrWnd.OnKeyDown(i + 128);
                else
                    sMcErrWnd.OnKeyUp(i + 128);
            }
        }
        sMcErrWnd.m_pad = p;
    }
    return sMcErrWnd.m_result;
}

static unsigned long sSessionId;

// 0x002AB4E0.
void VISetMcSessionId(unsigned long id)
{
    sSessionId = id;
}

// 0x002AB4F0.
unsigned long VIGetMcSessionId()
{
    return sSessionId;
}

// 0x002AB500.  -1 when there is no readable key on the card.
int VIIsMcSameSession()
{
    unsigned long id;
    if (VIMemCardStatus(0) != 0)
        return -1;
    if (VIMemCardReadFile("session.id", &id, 8) < 8)
        return -1;
    if (id != VIGetMcSessionId())
        return 0;
    return 1;
}
