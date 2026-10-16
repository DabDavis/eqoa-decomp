// viwndgenericrenderer.cpp (name ours) -- STATION 0x002C98B8 .. 0x002D1F68: the
// window that runs the Station pages.  It loads STATION.PAK (off the memory card
// when one is there, else the disc's /station.pak), keeps a stack of GUI pages
// with one dialog over them, turns the pages' actions into account-server
// requests (OnAction), and answers the server through VIStationListener.
#include "station.h"
#include "vilist_t.h"

void McErr(VIWnd *w, const VIVect2I &pos, int x);
extern "C" unsigned int VIHashResourceID(const char *s);
extern "C" float sinf(float);

// One WORDS.TXT entry in the current language, as the other windows.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00350208 ..  The module's own state across renderers (names ours).
static int sDisconnected = 0;                       // a "DONE" with code 3 arrived
static const char *sCountries[] =                   // the Country list's choices
{
    "US - USA",
    "CA - Canada",
    "MX - Mexico",
};
static int sCardError = 0;                          // the card could not be read
static int sFirstLoad = 1;                          // the card check runs once
static int sFirstDisconnect = 1;
static int sAccountSaved = 0;
static int sStatusFrames0 = 0;
static int sPingDelay = 0;
static int sStatusFrames = 0;

// 0x00373440: drawn over the page while the options save (the "saving" box).
static VIUIWindow sSaveWindow;

// 0x002C98B8.
VIWndGenericRenderer::VIWndGenericRenderer()
    : m_cs(this)
{
    m_editor = 0;
    m_page = 0;
    m_x40 = 0;
    m_x538 = 0;
    m_dialog = 0;
    m_loggedIn = 0;
    m_result = 0;
    m_uid = -1;
    m_paymentType = -1;
    m_subStatus = -1;
    m_xC74 = -1;
    m_loginStatus = -1;
    m_loaded = 0;
    m_done = 0;
    m_saveAccount = 0;
    m_xA4C = 0;
    m_removeChild = 0;
    m_xE64 = 0;
    m_newsActive = 0;
    m_newsWait = 0;
    m_newsPoll = 0;
    m_newsUrl = 0;
    m_checkNet = 0;
    m_saveDelay = 0;
    memset(m_news, 0, sizeof(m_news));
    memset(m_savedLogin, 0, 64);
    memset(m_savedPassword, 0, 64);
    memset(m_login, 0, 64);
    memset(m_password, 0, 64);
    memset(&m_billing, 0, sizeof(m_billing));
    memset(m_sku, 0, 32);
    memset(m_gameCard, 0, 64);
    memset(m_action, 0, sizeof(m_action));
    memset(&m_account, 0, sizeof(m_account));
    memset(m_newsURL, 0, sizeof(m_newsURL));
}

// 0x002C9A78.
VIWndGenericRenderer::~VIWndGenericRenderer()
{
    if (m_editor)
    {
        RemoveChild(m_editor);
        delete m_editor;
        m_editor = 0;
    }
    int i = m_pages.Begin();
    while (i != m_pages.End())
    {
        VIGUIPage *p = ((VIList<VIGUIPage *>::Node *)i)->m_data;
        if (p)
            delete p;
        int next = m_pages.Next(i);
        m_pages.Erase(i);
        i = next;
    }
    m_page = 0;
    if (m_dialog)
    {
        delete m_dialog;
        m_dialog = 0;
    }
}

// 0x002C9C28.
void VIWndGenericRenderer::SetLogin(const char *s)
{
    if (s)
        strncpy(m_login, s, 63);
}

// 0x002C9C50.
void VIWndGenericRenderer::SetPassword(const char *s)
{
    if (s)
        strncpy(m_password, s, 63);
}

// 0x002C9C78.
int VIWndGenericRenderer::IsLoaded()
{
    if (!m_loaded)
        Load();
    return m_loaded;
}

// 0x002C9CB0.  "name:value" lines; '#' starts a comment.
void VIWndGenericRenderer::ReadSavedAccount(const char *text)
{
    if (!text)
        return;
    char *copy = strdup(text);
    if (!copy)
        return;
    for (char *line = strtok(copy, "\r\n"); line; line = strtok(0, "\r\n"))
    {
        if (*line == '#')
            continue;
        char *sep = strstr(line, ":");
        char *value = sep + 1;
        *sep = 0;
        if (!line || !value)
            continue;
        if (!strcasecmp(line, "username"))
        {
            strncpy(m_savedLogin, value, 64);
            strncpy(m_login, value, 64);
        }
        else if (!strcasecmp(line, "password"))
        {
            strncpy(m_savedPassword, value, 64);
            strncpy(m_password, value, 64);
        }
        else if (!strcasecmp(line, "saveaccount"))
            m_saveAccount = atoi(value) != 0;
    }
    free(copy);
}

// 0x002C9E28.
void VIWndGenericRenderer::Load()
{
    GetAppPtr();
    m_child = 0;
    m_comboParent = 0;
    m_editParent = 0;
    m_spinStart = VITimer::GetTime();
    if (!sDisconnected)
    {
        if (sCardError)
            return;
        if (sFirstLoad)
        {
            sFirstLoad = 0;
            VIVect2I pos(320, 224);
            McErr(this, pos, 4);
            sCardError = 1;
            return;
        }
        char *buf = (char *)malloc(0x32000);
        memset(buf, 0, 0x32000);
        VIGetGameOptions()->Load();
        strncpy(m_savedLogin, VIGetGameOptions()->Username(), 64);
        strncpy(m_login, VIGetGameOptions()->Username(), 64);
        strncpy(m_savedPassword, VIGetGameOptions()->Password(), 64);
        strncpy(m_password, VIGetGameOptions()->Password(), 64);
        m_saveAccount = VIGetGameOptions()->m_x90 != 0;
        int n = VIMemCardReadFile("station.pak", buf, 0x32000);
        if (n > 0)
        {
            printf("!!Using mem card\n");
            m_pak.Load(buf, n);
            free(buf);
        }
        else if (n == -1)
        {
            VIVect2I pos(320, 224);
            McErr(this, pos, 4);
            sCardError = 1;
            free(buf);
            return;
        }
        else
        {
            VIFile f;
            printf("!!Not Using mem card\n");
            free(buf);
            if (!f.Open("/station.pak", 33))
            {
                f.Seek(0, VIFile::Seek_End);
                int size = f.Tell();
                if (size <= 0)
                    return;
                char *data = (char *)memalign(64, size + 1);
                memset(data, 0, size + 1);
                f.Seek(0, VIFile::Seek_Set);
                f.Read(data, size);
                f.Close();
                m_pak.Load(data, size);
                free(data);
            }
        }
        LoadPage("data/ui/login.xml");
    }
    m_dialog = new VIGUIDialog;
    m_dialog->Init(this, 0, 0);
    if (sDisconnected)
    {
        m_dialog->Open(Word(UI(), 767), "DisconnectConfirmed");
        m_dialog->SetText(Word(UI(), 1341));
    }
    else
    {
        if (!m_editor)
            m_editor = new VIWndEdit;
        m_editor->m_visible = 0;
        m_editor->SetPos(VIVect2I(32, 190));
        m_editor->SetAppPtr(GetAppPtr());
        m_editor->Clear();
        AddChild(m_editor);
        LoadServers();
    }
    m_loaded = 1;
}

// 0x002CA258.
int VIWndGenericRenderer::Start()
{
    return 0;
}

// 0x002CA260.
void VIWndGenericRenderer::AddServers(VIFlatFile *f)
{
    if (f)
    {
        char *data = (char *)f->GetData();
        int size = f->GetSize();
        if (data && size > 0)
            AddServers(data);
    }
}

// 0x002CA2C8.  "host:port" lines; '#' starts a comment.
void VIWndGenericRenderer::AddServers(const char *text)
{
    if (!text)
        return;
    char *copy = strdup(text);
    if (!copy)
        return;
    for (char *line = strtok(copy, "\r\n"); line; line = strtok(0, "\r\n"))
    {
        if (*line == '#')
            continue;
        char *sep = strstr(line, ":");
        if (!sep)
            break;
        *sep = 0;
        int port = atoi(sep + 1);
        if (port > 0)
        {
            printf("Adding %s:%i\n", line, port);
            m_api.AddServer(line, port);
        }
    }
    free(copy);
}

// 0x002CA3B8.
void VIWndGenericRenderer::ReadConfig(VIFlatFile *f)
{
    if (f)
    {
        char *data = (char *)f->GetData();
        int size = f->GetSize();
        if (data && size > 0)
            ReadConfig(data);
    }
}

// 0x002CA420.  "name:value" lines; NewsURL is the only name.
void VIWndGenericRenderer::ReadConfig(const char *text)
{
    if (!text)
        return;
    char *copy = strdup(text);
    if (!copy)
        return;
    for (char *line = strtok(copy, "\r\n"); line; line = strtok(0, "\r\n"))
    {
        if (*line == '#')
            continue;
        char *sep = strstr(line, ":");
        char *value = sep + 1;
        if (!sep)
            break;
        *sep = 0;
        if (value && !strcmp(line, "NewsURL"))
        {
            strcpy(m_newsURL, value);
            printf("Assigning news URL from config file: %s\n", m_newsURL);
        }
    }
    free(copy);
}

// 0x002CA520.
void VIWndGenericRenderer::LoadServers()
{
    m_api.SetListener(this);
    VIFlatFile *f = m_pak.Find("/stationdata/servers/stationAltHosts.txt");
    if (f || (f = m_pak.Find("/stationdata/servers/stationLiveHosts.txt")))
        AddServers(f);
    else
        AddServers("64.37.156.200:9735");
    f = m_pak.Find("/stationdata/config/options.cfg");
    if (f)
        ReadConfig(f);
    else
    {
        printf("No config file found in the station pak\n");
        ReadConfig("NewsURL:http://everquestonlineadventures.station.sony.com/privatebeta/news_blank.jsp");
    }
    m_api.Initialize();
}

// 0x002CA5F0.  Keeps what the account pages hold, then drops the top page.
void VIWndGenericRenderer::PageBack()
{
    if (m_pages.Size() < 2)
        return;
    if (!m_page)
        return;
    if (strstr(m_page->GetName(), "IDCreat2") || strstr(m_page->GetName(), "IDCREAT2"))
    {
        const char *country = m_page->GetText("Country");
        const char *postal = m_page->GetText("PostalCode");
        const char *day = m_page->GetText("BirthDay");
        const char *month = m_page->GetText("BirthMonth");
        const char *year = m_page->GetText("BirthYear");
        const char *gender = m_page->GetText("Gender");
        const char *email = m_page->GetText("EmailAddress");
        if (country && postal && day && month && year && gender)
        {
            strncpy(m_account.m_x0E0, country, 32);
            strncpy(m_account.m_x100, postal, 32);
            strncpy(m_account.m_x120, day, 16);
            strncpy(m_account.m_x130, month, 16);
            strncpy(m_account.m_x140, year, 16);
            strncpy(m_account.m_x150, gender, 16);
            if (email)
                strncpy(m_account.m_x160, email, 128);
        }
    }
    else if (strstr(m_page->GetName(), "IDCreate") || strstr(m_page->GetName(), "IDCREATE"))
    {
        const char *name = m_page->GetText("StationName");
        const char *password = m_page->GetText("StationPassword");
        const char *password2 = m_page->GetText("StationPassword2");
        const char *first = m_page->GetText("FirstName");
        const char *middle = m_page->GetText("MiddleName");
        const char *last = m_page->GetText("LastName");
        if (name && password && password2 && first && middle && last)
        {
            strncpy(m_account.m_name, name, 32);
            strncpy(m_account.m_password, password, 32);
            strncpy(m_account.m_x040, first, 64);
            strncpy(m_account.m_x080, middle, 32);
            strncpy(m_account.m_x0A0, last, 64);
        }
    }
    int i = m_pages.Begin();
    if (i != m_pages.End())
    {
        i = m_pages.Erase(i);
        if (m_page)
            delete m_page;
        if (i == m_pages.End())
            m_page = 0;
        else
            m_page = ((VIList<VIGUIPage *>::Node *)i)->m_data;
    }
}

// 0x002CA970.  Fills a page from what the earlier ones held, then makes it the
// top of the stack.
void VIWndGenericRenderer::PushPage(VIGUIPage *page)
{
    if (!page)
        return;
    if (strstr(page->GetName(), "UpAcct") || strstr(page->GetName(), "UPACCT"))
    {
        page->ClearOptions("Country");
        const char **c = sCountries;
        for (int i = 2; i >= 0; i--)
            page->AddOption("Country", *c++);
        if (!m_account.m_x0E0[0])
        {
            switch ((signed char)VIGetGameOptions()->m_region)
            {
            case 1:
                page->SetText("Country", "GB - United Kingdom");
                break;
            case 2:
                page->SetText("Country", "FR - France");
                break;
            case 3:
                page->SetText("Country", "DE - Germany");
                break;
            default:
                page->SetText("Country", "US - USA");
                break;
            }
        }
    }
    else if (strstr(page->GetName(), "IDCreat2") || strstr(page->GetName(), "IDCREAT2"))
    {
        const char *country = page->GetText("Country");
        const char *postal = page->GetText("PostalCode");
        const char *day = page->GetText("BirthDay");
        const char *month = page->GetText("BirthMonth");
        const char *year = page->GetText("BirthYear");
        const char *gender = page->GetText("Gender");
        const char *email = page->GetText("EmailAddress");
        page->ClearOptions("Country");
        const char **c = sCountries;
        for (int i = 2; i >= 0; i--)
            page->AddOption("Country", *c++);
        if (!m_account.m_x0E0[0])
        {
            switch ((signed char)VIGetGameOptions()->m_region)
            {
            case 0:
                strncpy(m_account.m_x0E0, "US - USA", 32);
                break;
            case 1:
                strncpy(m_account.m_x0E0, "GB - United Kingdom", 32);
                break;
            case 2:
                strncpy(m_account.m_x0E0, "FR - France", 32);
                break;
            case 3:
                strncpy(m_account.m_x0E0, "DE - Germany", 32);
                break;
            }
        }
        if (country && postal && day && month && year && gender)
        {
            if (*country && !m_account.m_x0E0[0])
                strncpy(m_account.m_x0E0, country, 32);
            if (*postal && !m_account.m_x100[0])
                strncpy(m_account.m_x100, postal, 32);
            if (*day && !m_account.m_x120[0])
                strncpy(m_account.m_x120, day, 16);
            if (*month && !m_account.m_x130[0])
                strncpy(m_account.m_x130, month, 16);
            if (*year && !m_account.m_x140[0])
                strncpy(m_account.m_x140, year, 16);
            if (*gender && !m_account.m_x150[0])
                strncpy(m_account.m_x150, gender, 16);
            if (email && *email && !m_account.m_x160[0])
                strncpy(m_account.m_x160, email, 128);
        }
    }
    else if (strstr(page->GetName(), "IDCreate") || strstr(page->GetName(), "IDCREATE"))
    {
        const char *name = page->GetText("StationName");
        const char *password = page->GetText("StationPassword");
        const char *password2 = page->GetText("StationPassword2");
        const char *first = page->GetText("FirstName");
        const char *middle = page->GetText("MiddleName");
        const char *last = page->GetText("LastName");
        if (name && password && password2 && first && middle && last)
        {
            if (*name && !m_account.m_name[0])
                strncpy(m_account.m_name, name, 32);
            if (*password && !m_account.m_password[0])
                strncpy(m_account.m_password, password, 32);
            if (*first && !m_account.m_x040[0])
                strncpy(m_account.m_x040, first, 64);
            if (*middle && !m_account.m_x080[0])
                strncpy(m_account.m_x080, middle, 32);
            if (*last && !m_account.m_x0A0[0])
                strncpy(m_account.m_x0A0, last, 64);
        }
    }
    else if (strstr(page->GetName(), "BillPlan") || strstr(page->GetName(), "BILLPLAN"))
    {
        page->ClearOptions("BillingPlan");
        for (int i = m_plans.Begin(); i != m_plans.End(); i = m_plans.Next(i))
        {
            char text[1024];
            VISubscriptionPlan plan = ((VIList<VISubscriptionPlan>::Node *)i)->m_data;
            printf("My SKU = %s\n", m_sku);
            sprintf(text, "%s ($%s)", plan.m_shortDesc, plan.m_price);
            page->AddOption("BillingPlan", text);
            if (i == m_plans.Begin())
                page->SetText("BillingPlan", text);
            if (m_sku[0] && !strcmp(m_sku, plan.m_sku))
                page->SetText("CurrentPlanName", text);
            printf("\t%s\n", plan.m_sku);
        }
    }
    m_pages.Insert(page, m_pages.Begin());
    m_page = page;
    if (strstr(page->GetName(), "login"))
    {
        if (m_saveAccount)
        {
            page->SetText("StationName", m_login);
            page->SetText("StationPassword", m_password);
            page->SetToggle("SaveAccountToggle", 1);
        }
        const char *name = page->GetText("StationName");
        const char *password = page->GetText("StationPassword");
        if (name && password && *name && *password)
            page->Select("LogInButton");
    }
    else if (strstr(page->GetName(), "IDCreate") || strstr(page->GetName(), "IDCREATE"))
    {
        if (m_dialog)
        {
            m_dialog->Open(Word(UI(), 1241), 0);
            m_dialog->SetText(Word(UI(), 1376));
        }
        page->SetText("StationName", m_account.m_name);
        page->SetText("StationPassword", m_account.m_password);
        page->SetText("StationPassword2", m_account.m_password);
        page->SetText("FirstName", m_account.m_x040);
        page->SetText("MiddleName", m_account.m_x080);
        page->SetText("LastName", m_account.m_x0A0);
    }
    else if (strstr(page->GetName(), "IDCreat2") || strstr(page->GetName(), "IDCREAT2"))
    {
        page->SetText("Country", m_account.m_x0E0);
        page->SetText("PostalCode", m_account.m_x100);
        page->SetText("BirthDay", m_account.m_x120);
        page->SetText("BirthMonth", m_account.m_x130);
        page->SetText("BirthYear", m_account.m_x140);
        page->SetText("Gender", m_account.m_x150);
        page->SetText("EmailAddress", m_account.m_x160);
    }
}

// 0x002CB4D0.  The pak's copy of a page if it has one, else the disc's.
void VIWndGenericRenderer::LoadPage(const char *name)
{
    if (!name)
        return;
    VIGUIParser parser(this);
    VIGUIPage *page;
    VIFlatFile *f = m_pak.Find(name);
    if (f)
    {
        printf("!!!USING THE FLAT FILE VERSION OF %s\n", name);
        page = parser.LoadMemory((const char *)f->GetData(), f->GetSize());
        page->SetName(name);
    }
    else
        page = parser.LoadFile(name);
    if (page)
        PushPage(page);
}

// 0x002CB6F8: slot 1.  On success the new account logs in.
void VIWndGenericRenderer::OnReply41(bool success, int code, const char *msg)
{
    if (!m_dialog || !m_dialog->IsOpen())
        return;
    m_dialog->ClearConfirm();
    printf("!!!!!success = %i\n", success);
    if (success)
    {
        strcpy(m_login, m_account.m_name);
        strcpy(m_password, m_account.m_password);
        m_dialog->SetConfirm();
        m_saveAccount = 1;
        m_dialog->SetText(Word(UI(), 1384));
        m_api.ValidateLogin(m_login, m_password);
        return;
    }
    char text[1024];
    sprintf(text, "%s", msg);
    if (strstr(text, "exists"))
    {
        char fmt[1024];
        VIString::str16to8(fmt, 1024, Word(UI(), 1371));
        sprintf(text, fmt, m_account.m_name, m_account.m_name);
        m_dialog->SetText(text);
        PageBack();
        return;
    }
    m_dialog->SetText(text);
}

// 0x002CB5A0: slot 11.
void VIWndGenericRenderer::OnReply65(int success)
{
    if (!m_dialog || !m_dialog->IsOpen())
        return;
    m_dialog->ClearConfirm();
    if (success)
    {
        m_dialog->SetText(Word(UI(), 1345));
        LoadPage("data/ui/Conn.xml");
    }
    else
    {
        m_dialog->SetText(Word(UI(), 1254));
        LoadPage("data/ui/Conn.xml");
    }
}

// 0x002CB658: slot 12.
void VIWndGenericRenderer::OnReply67(int success)
{
    if (!m_dialog || !m_dialog->IsOpen())
        return;
    m_dialog->ClearConfirm();
    if (success)
    {
        m_dialog->SetText(Word(UI(), 1396));
        PageBack();
    }
    else
        m_dialog->SetText(Word(UI(), 1353));
}

// 0x002CB858: slot 4.
void VIWndGenericRenderer::OnReply45(int result, int code)
{
    if (!m_dialog)
        return;
    if (!result)
    {
        m_dialog->SetText(Word(UI(), 1373));
        LoadPage("data/ui/BillMeth.xml");
        HideDialog();
    }
    else
        m_dialog->SetText(Word(UI(), 1289));
}

// 0x002CB8E8: slot 9.
void VIWndGenericRenderer::OnReply55(int result, int code, const char *s16, const char *sku, int c, int d)
{
    if (!m_dialog)
        return;
    if (!result)
    {
        if (sku)
            strncpy(m_sku, sku, 31);
        LoadPage("data/ui/GCCons.xml");
        HideDialog();
        return;
    }
    switch (code)
    {
    case 51:
        m_dialog->SetText(Word(UI(), 1358));
        break;
    case 52:
        m_dialog->SetText(Word(UI(), 1283));
        break;
    case 53:
        m_dialog->SetText(Word(UI(), 1270));
        break;
    default:
        m_dialog->SetText(Word(UI(), 1287));
        break;
    }
}

// 0x002CB9D8: slot 6.
void VIWndGenericRenderer::OnReply47(int result, int code)
{
    if (!m_dialog)
        return;
    if (!result)
    {
        m_loggedIn = 1;
        m_dialog->SetText(Word(UI(), 1343));
        LoadPage("data/ui/conn.xml");
        HideDialog();
        return;
    }
    m_sku[0] = 0;
    switch (code)
    {
    case 1:
        m_dialog->SetText(Word(UI(), 1309));
        break;
    case 4:
        m_dialog->SetText(Word(UI(), 1399));
        break;
    case 51:
        m_dialog->SetText(Word(UI(), 1358));
        break;
    case 52:
        m_dialog->SetText(Word(UI(), 1283));
        break;
    case 53:
        m_dialog->SetText(Word(UI(), 1270));
        break;
    case 10:
        m_dialog->SetText(Word(UI(), 1357));
        break;
    case 11:
        m_dialog->SetText(Word(UI(), 1349));
        break;
    case 12:
        m_dialog->SetText(Word(UI(), 1389));
        break;
    case 16:
        m_dialog->SetText(Word(UI(), 1348));
        break;
    case 17:
        m_dialog->SetText(Word(UI(), 1347));
        break;
    case 19:
        m_dialog->SetText(Word(UI(), 1352));
        break;
    case 20:
        m_dialog->SetText(Word(UI(), 1351));
        break;
    case 21:
        m_dialog->SetText(Word(UI(), 1350));
        break;
    default:
        m_dialog->SetText(Word(UI(), 1355));
        break;
    }
}

// 0x002CBB58: slot 5.
void VIWndGenericRenderer::OnConsumeAccountKey(int result, int code)
{
    if (!m_dialog || !m_dialog->IsOpen())
        return;
    m_dialog->ClearConfirm();
    printf("OnConsumeAccountKey( %d, %d )\n", result, code);
    if (!result)
    {
        m_dialog->SetText(Word(UI(), 1374));
        m_dialog->SetConfirm();
        printf("calling ValidateLogin()\n");
        m_api.ValidateLogin(m_login, m_password);
        m_saveAccount = 1;
        return;
    }
    switch (code)
    {
    case 43:
        m_dialog->SetText(Word(UI(), 1295));
        break;
    case 41:
        m_dialog->SetText(Word(UI(), 1296));
        break;
    case 40:
    {
        unsigned short text[512];
        const unsigned short *a = Word(UI(), 1360);
        VIString::snprintf16(text, 1024, a, Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
        m_dialog->SetText(text);
        return;
    }
    case 44:
        m_dialog->SetText(Word(UI(), 1359));
        break;
    default:
        if (result == 1)
            m_dialog->SetText(Word(UI(), 1809));
        else
            m_dialog->SetText(Word(UI(), 1290));
        break;
    }
}

// 0x002CBD30: slot 3.
void VIWndGenericRenderer::OnChangePassword(bool success, const char *msg)
{
    if (!m_dialog || !m_dialog->IsOpen())
        return;
    m_dialog->ClearConfirm();
    if (success)
    {
        m_dialog->SetText(Word(UI(), 1344));
        const char *p = m_page->GetText("NewPassword");
        if (p)
        {
            strcpy(m_password, p);
            PageBack();
        }
    }
    else
        m_dialog->SetText(msg);
}

// 0x002CBDE8: slot 8.
void VIWndGenericRenderer::OnCancelAccount(int result, int code)
{
    printf("OnCancelAccount (%i, %i)\n", result, code);
    if (!m_dialog)
    {
        printf("dialog is NULL\n");
        return;
    }
    if (!result)
    {
        m_dialog->Open(Word(UI(), 1220), 0);
        m_dialog->SetText(Word(UI(), 1208));
        LoadPage("data/ui/login.xml");
        m_dialog->ClearConfirm();
        return;
    }
    switch (code)
    {
    case 1:
        m_dialog->SetText(Word(UI(), 1379));
        break;
    case 2:
    case 3:
        m_dialog->SetText(Word(UI(), 1356));
        break;
    default:
        m_dialog->SetText(Word(UI(), 1261));
        break;
    }
}

// 0x002CBF18: slot 19.
void VIWndGenericRenderer::OnError(int code, const char *msg)
{
    if (!m_dialog || !msg)
        return;
    VIStation *station = (VIStation *)GetAppPtr();
    m_editor->m_visible = 0;
    station->m_xB40C = (int)this;
    station->m_wm.SetFocus(this, true);
    m_dialog->Open("", 0);
    m_dialog->SetText(msg);
}

// 0x002CBFA8: slot 20.
void VIWndGenericRenderer::OnStatus(const char *msg)
{
    VINetStatus status;
    status.Init();
    if (!status.GetState())
    {
        if (m_dialog && msg && !sFirstDisconnect)
        {
            m_dialog->Open(Word(UI(), 1235), 0);
            m_dialog->SetText(msg);
        }
        m_checkNet = 1;
        sFirstDisconnect = 0;
    }
}

// 0x002CC060: slot 22.
void VIWndGenericRenderer::OnConnected()
{
    if (m_dialog)
        m_dialog->Close();
    m_checkNet = 0;
}

// 0x002CC098: slot 7.
void VIWndGenericRenderer::OnGetSubscriptionInfo(int result, int code, int status,
                                                 int paymentType, const char *sku)
{
    if (!m_dialog || !m_dialog->IsOpen())
        return;
    if (sku)
        printf("OnGetSubscriptionInfo: sku = %s\n", sku);
    m_dialog->ClearConfirm();
    if (result)
    {
        switch (code)
        {
        case 1:
            m_dialog->SetText(Word(UI(), 1378));
            LoadPage("data/ui/EnterKey.xml");
            break;
        case 2:
        case 3:
            m_dialog->SetText(Word(UI(), 1316));
            LoadPage("data/ui/BillMeth.xml");
            break;
        default:
            m_dialog->SetText(Word(UI(), 1263));
            break;
        }
        return;
    }
    m_subStatus = status;
    switch (status)
    {
    case 0:
        m_dialog->SetText(Word(UI(), 1398));
        LoadPage("data/ui/BillMeth.xml");
        break;
    case 3:
        m_dialog->SetText(Word(UI(), 1383));
        LoadPage("data/ui/BillMeth.xml");
        break;
    case 1:
    case 2:
    case 4:
    case 5:
    case 7:
        HideDialog();
        m_paymentType = paymentType;
        printf("!!!!!m_paymentType = %i\n", paymentType);
        strncpy(m_sku, sku, 32);
        OnAction(m_action);
        m_action[0] = 0;
        break;
    case 6:
        m_dialog->SetText(Word(UI(), 1401));
        LoadPage("data/ui/BillMeth.xml");
        break;
    default:
        m_dialog->SetText(Word(UI(), 1239));
        break;
    }
}

// 0x002CC288.  Writes the login to the options when the toggle (or a change)
// asks for it; the save itself happens a few frames on.
void VIWndGenericRenderer::SaveAccount()
{
    char text[10240];
    if (!m_page)
        return;
    int save = 0;
    if (!sAccountSaved)
    {
        if (m_page->GetName())
        {
            if (strstr(m_page->GetName(), "login") || strstr(m_page->GetName(), "LOGIN"))
                m_saveAccount = m_page->GetToggle("SaveAccountToggle");
        }
        if (!m_saveAccount)
        {
            sprintf(text, "");
            save = 1;
            m_savedLogin[0] = ""[0];
            m_savedPassword[0] = ""[0];
        }
        else if (strcmp(m_savedLogin, m_login) || strcmp(m_savedPassword, m_password))
        {
            sprintf(text, "%s:%s\n%s:%s\n%s:%i\n", "username", m_login, "password", m_password,
                    "saveaccount", 1);
            save = 1;
            strcpy(m_savedLogin, m_login);
            strcpy(m_savedPassword, m_password);
        }
    }
    else
        save = m_saveAccount;
    if (!save)
        return;
    VIGetGameOptions()->SetUsername(m_saveAccount ? m_login : "");
    VIGetGameOptions()->SetPassword(m_saveAccount ? m_password : "");
    VIGetGameOptions()->m_x90 = m_saveAccount;
    sAccountSaved = 1;
    m_saveDelay = 3;
}

// 0x002CC498: slot 2.
void VIWndGenericRenderer::OnValidateLogin(int result, int uid, int accstat, int subtime,
                                           int partime, int subfeatures, int gamefeatures,
                                           const char *s92, const char *s12)
{
    char text[1024];
    char fmt[1024];
    unsigned short wtext[512];
    int status = m_xE64;
    int expansion = 0;
    printf("uid: %d / result: %d / accstat: %d / subtime: %d / partime: %d / subfeatures: %d / gamefeatures: %d\n",
           uid, result, accstat, subtime, partime, subfeatures, gamefeatures);
    m_loginStatus = -1;
    m_xE64 = 0;
    m_uid = -1;
    m_paymentType = -1;
    m_subStatus = -1;
    if (!m_dialog || !m_dialog->IsOpen())
        return;
    m_dialog->ClearConfirm();
    printf("!!!!m_loginStatus = %i\n", accstat);
    if (result)
    {
        m_dialog->SetText(Word(UI(), 1394));
        return;
    }
    if (accstat == 3 || accstat == 1 || accstat == 5 || accstat == 9 || accstat == 7)
    {
        m_loginStatus = accstat;
        SaveAccount();
        if (accstat == 1)
        {
            if ((unsigned int)(subtime - 1) <= 0xD2EFE)
            {
                int days = subtime / 86400;
                if (days > 0)
                {
                    VIString::str16to8(fmt, 1024, Word(UI(), 1381));
                    sprintf(text, fmt, days);
                }
                else
                    VIString::str16to8(text, 1024, Word(UI(), 1783));
                m_dialog->SetText(text);
            }
        }
        else if (status != 1)
        {
            if ((VIGetExpansionFlags() & 2) && !(gamefeatures & 2))
            {
                expansion = 1;
                m_dialog->SetText(Word(UI(), 1660));
            }
            else if (!(VIGetExpansionFlags() & 2) && (gamefeatures & 2))
                m_dialog->SetText(Word(UI(), 1661));
            else
                m_dialog->SetText(Word(UI(), 1301));
        }
        const char *name = m_page->GetText("StationName");
        const char *password = m_page->GetText("StationPassword");
        if (name && password)
        {
            strcpy(m_login, name);
            strcpy(m_password, password);
        }
        SaveAccount();
        m_uid = uid;
        m_loggedIn = 1;
        if (expansion)
        {
            LoadPage("data/ui/EnterKey.xml");
            return;
        }
        LoadPage("data/ui/conn.xml");
        return;
    }
    m_loginStatus = accstat;
    const char *name = m_page->GetText("StationName");
    const char *password = m_page->GetText("StationPassword");
    if (name && password)
    {
        strcpy(m_login, name);
        strcpy(m_password, password);
    }
    else if (!m_login[0])
    {
        m_dialog->SetText(Word(UI(), 1363));
        return;
    }
    SaveAccount();
    m_uid = uid;
    m_loggedIn = 1;
    switch (accstat)
    {
    case 2:
        if (status != 1)
            HideDialog();
        LoadPage("data/ui/EnterKey.xml");
        break;
    case 10:
        m_dialog->SetText(Word(UI(), 1397));
        LoadPage("data/ui/EnterKey.xml");
        break;
    case 6:
    {
        const unsigned short *a = Word(UI(), 1400);
        VIString::snprintf16(wtext, 1024, a, Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
        m_dialog->SetText(wtext);
        LoadPage("data/ui/BillMeth.xml");
        break;
    }
    case 0:
        if ((VIGetExpansionFlags() & 2) && !(gamefeatures & 2))
        {
            m_dialog->SetText(Word(UI(), 1660));
            LoadPage("data/ui/EnterKey.xml");
            break;
        }
        {
            const unsigned short *a = Word(UI(), 1380);
            VIString::snprintf16(wtext, 1024, a, Word(UI(), (VIGetExpansionFlags() & 2) ? 1803 : 1802));
            m_dialog->SetText(wtext);
            LoadPage("data/ui/BillMeth.xml");
        }
        break;
    case 8:
        m_dialog->SetText(Word(UI(), 1385));
        break;
    default:
        m_dialog->SetText(Word(UI(), 1364));
        break;
    }
}

// 0x002CC950: slot 10.
void VIWndGenericRenderer::OnGetSubscriptionList(int result, int code, int count,
                                                 const char *list)
{
    const VISubscriptionPlan *plans = (const VISubscriptionPlan *)list;
    printf("OnGetSubscriptionList(%i, %i)\n", result, code);
    m_dialog->ClearConfirm();
    HideDialog();
    m_plans.Clear();
    for (int i = 0; i < count; i++)
    {
        printf("Plan %i:\n", i);
        printf("\tduration = %s\n", plans[i].m_duration);
        printf("\tlong_desc = %s\n", plans[i].m_longDesc);
        printf("\tshort_desc = %s\n", plans[i].m_shortDesc);
        printf("\tprice = %s\n", plans[i].m_price);
        printf("\tsku = %s\n", plans[i].m_sku);
        m_plans.Add(plans[i]);
    }
    if (!result && !code)
    {
        LoadPage("data/ui/BillPlan.xml");
        return;
    }
    m_dialog->Open(Word(UI(), 767), 0);
    m_dialog->SetText(Word(UI(), 1262));
}

// 0x002CCB10 .. 0x002CCB38: slots 13 .. 18, empty.
void VIWndGenericRenderer::OnReply69(int a, const unsigned short *s12, const unsigned short *s528)
{
}

void VIWndGenericRenderer::OnReply71(int a, const unsigned short *s12, int count,
                                     const VIStationRecord132 *recs)
{
}

void VIWndGenericRenderer::OnReply73(int a, const unsigned short *s12, int count,
                                     const VIStationRecord142 *recs)
{
}

void VIWndGenericRenderer::OnReply75(int a, const unsigned short *s12, int count,
                                     const VIStationRecord132 *recs)
{
}

void VIWndGenericRenderer::OnGetGCPaymentMethodFields(int a, const unsigned short *s12,
                                                      const unsigned short *text, int count,
                                                      const VIStationRecord204 *recs)
{
}

void VIWndGenericRenderer::OnReply84(int a, const unsigned short *s12, const char *s524,
                                     const unsigned short *s588, const unsigned short *s720)
{
}

// 0x002CCB40: slot 21.
void VIWndGenericRenderer::OnIdle()
{
    VIStation *station = (VIStation *)GetAppPtr();
    if (station)
        station->Process();
}

// 0x002CCB70.
void VIWndGenericRenderer::HideDialog()
{
    if (m_dialog)
    {
        printf("HIDE\n");
        m_dialog->Close();
    }
}

// 0x002CCBB0 .. 0x002CD018: the form checks (names ours).
static int IsAlpha(char c)
{
    if ((unsigned int)(c - 'a') < 26 || (unsigned int)(c - 'A') < 26)
        return 1;
    return 0;
}

static int IsDigit(char c)
{
    return (unsigned int)(c - '0') < 10;
}

// 0x002CCC00.  A name needs a letter or a digit; the label goes red when it
// has neither.
static int CheckName(VIWndGenericRenderer *r, VIGUIPage *page, const char *s,
                     const char *label, VIColor32F red)
{
    if (!page || !s)
        return 0;
    int letters = 0;
    int digits = 0;
    for (int i = 0; s[i]; i++)
    {
        if (IsAlpha(s[i]))
            letters++;
        else if (IsDigit(s[i]))
            digits++;
    }
    if (letters)
        return 1;
    if (digits)
        return 1;
    if (label)
        page->SetTextColor(label, red);
    return 0;
}

static char *ToUpper(char *s)
{
    if (!s)
        return 0;
    for (char *p = s; *p; p++)
        if (*p >= 'a' && *p <= 'z')
            *p -= 32;
    return s;
}

static int IsNumeral(char c)
{
    return (unsigned int)(c - '0') < 10;
}

static int IsNameChar(char c)
{
    if ((unsigned int)(c - 'a') < 26 || (unsigned int)(c - 'A') < 26 ||
        (unsigned int)(c - '0') < 10 || c == '_')
        return 1;
    return 0;
}

static int IsValidName(const char *s)
{
    for (; *s; s++)
        if (!IsNameChar(*s))
            return 0;
    return 1;
}

static int IsSafeChar(char c)
{
    if (c == '$' || c == '"' || c == '<' || c == '>' || c == '(' || c == ')' ||
        c == ';' || c == '-')
        return 0;
    return 1;
}

// 0x002CCE58: 0 good, 2 no '@', 3 an empty part, 4 ends in '.'.
static int CheckEmail(const char *s)
{
    if (!*s)
        return 0;
    int at = 0;
    int dot = 0;
    for (; *s; s++)
    {
        if (!IsNameChar(*s) && *s != '.' && *s != '-' && *s != '+' && *s != '@')
            return 1;
        if (*s == '@')
            at = 1;
        if (*s == '.')
        {
            dot = 1;
            if (!s[1])
                return 4;
        }
    }
    if (at)
        return dot ? 0 : 3;
    return 2;
}

static int IsZip(const char *s)
{
    if (!s)
        return 0;
    if (strlen(s) != 5)
        return 0;
    for (; *s; s++)
        if (!IsNumeral(*s))
            return 0;
    return 1;
}

static int IsNumber(const char *s)
{
    if (!s)
        return 0;
    for (; *s; s++)
        if (!IsNumeral(*s))
            return 0;
    return 1;
}

// 0x002CD018.  A page's action by name: the form checks, then the request.
// Every `m_dialog->SetText(Word(UI(), n))` tail is cross-jumped into one copy
// at 0x002CFA04, the Disconnect arm's.
void VIWndGenericRenderer::OnAction(const char *action)
{
    printf("ACTION OCCURED: %s\n", action);
    if (!m_page && !strcmp(action, "DisconnectConfirmed"))
    {
        VIStation *station = (VIStation *)GetAppPtr();
        if (!station)
            return;
        printf("DISCONNECTING\n");
        station->FadeOut();
        m_result = 2;
    }
    if (!action)
        return;
    if (!m_page)
        return;

    if (!strcmp(action, "SubmitStationLogin"))
    {
        m_dialog->Open(Word(UI(), 1300), 0);
        if (!m_dialog)
            return;
        m_dialog->SetText(Word(UI(), 758));
        const char *name = m_page->GetText("StationName");
        const char *password = m_page->GetText("StationPassword");
        if (name && password)
        {
            m_dialog->SetConfirm();
            m_api.ValidateLogin(name, password);
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "ContinueAcctCreate"))
    {
        if (!m_dialog || !m_page)
            return;
        const char *name = m_page->GetText("StationName");
        const char *password = m_page->GetText("StationPassword");
        const char *password2 = m_page->GetText("StationPassword2");
        const char *first = m_page->GetText("FirstName");
        const char *middle = m_page->GetText("MiddleName");
        const char *last = m_page->GetText("LastName");
        if (name && password && password2 && first && middle && last)
        {
            VIColor32F red(0.90980393f, 0.54901963f, 0.22352941f, 1.0f);
            bool ok = CheckName(this, m_page, name, "StationNameLabel", red);
            bool valid;
            valid = CheckName(this, m_page, password, "Password1Label", red);
            ok &= valid;
            valid = CheckName(this, m_page, password2, "Password2Label", red);
            ok &= valid;
            valid = CheckName(this, m_page, first, "FirstNameLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, last, "LastNameLabel", red);
            ok &= valid;
            if (!IsValidName(name))
            {
                m_dialog->Open(Word(UI(), 1294), 0);
                m_dialog->SetText(Word(UI(), 1312));
            }
            else if (!IsValidName(password))
            {
                m_dialog->Open(Word(UI(), 1292), 0);
                m_dialog->SetText(Word(UI(), 1311));
            }
            else if (!ok)
            {
                m_dialog->Open(Word(UI(), 1288), 0);
                m_dialog->SetText(Word(UI(), 1310));
            }
            else if (strcmp(password, password2))
            {
                m_dialog->Open(Word(UI(), 1288), 0);
                m_dialog->SetText(Word(UI(), 1314));
            }
            else
            {
                strncpy(m_account.m_name, name, 32);
                strncpy(m_account.m_password, password, 32);
                strncpy(m_account.m_x040, first, 64);
                strncpy(m_account.m_x080, middle, 32);
                strncpy(m_account.m_x0A0, last, 64);
                LoadPage("data/ui/IDCreat2.xml");
            }
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "SubmitAcctCreate"))
    {
        if (!m_dialog || !m_page)
            return;
        const char *country = m_page->GetText("Country");
        const char *postal = m_page->GetText("PostalCode");
        const char *day = m_page->GetText("BirthDay");
        const char *month = m_page->GetText("BirthMonth");
        const char *year = m_page->GetText("BirthYear");
        const char *gender = m_page->GetText("Gender");
        const char *email = m_page->GetText("EmailAddress");
        if (country && postal && day && month && year && gender)
        {
            VIColor32F red(0.90980393f, 0.54901963f, 0.22352941f, 1.0f);
            bool ok = CheckName(this, m_page, country, "CountryLabel", red);
            bool valid;
            valid = CheckName(this, m_page, postal, "PostalCodeLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, day, "BirthDayLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, month, "BirthMonthLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, year, "BirthYearLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, gender, "GenderLabel", red);
            ok &= valid;
            switch (CheckEmail(email))
            {
            case 1:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1390));
                return;
            case 2:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1392));
                return;
            case 3:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1391));
                return;
            case 4:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1393));
                return;
            }
            if (!strncmp(country, "US -", 4) && !IsZip(postal))
            {
                m_dialog->Open(Word(UI(), 1293), 0);
                m_dialog->SetText(Word(UI(), 1354));
            }
            else if (!ok)
            {
                m_dialog->Open(Word(UI(), 1288), 0);
                m_dialog->SetText(Word(UI(), 1310));
            }
            else
            {
                strncpy(m_account.m_x0E0, country, 32);
                strncpy(m_account.m_x100, postal, 32);
                strncpy(m_account.m_x120, day, 16);
                strncpy(m_account.m_x130, month, 16);
                strncpy(m_account.m_x140, year, 16);
                strncpy(m_account.m_x150, gender, 16);
                if (email)
                    strncpy(m_account.m_x160, email, 128);
                m_dialog->Open(Word(UI(), 1241), 0);
                m_dialog->SetText(Word(UI(), 758));
                m_dialog->SetConfirm();
                m_xE64 = 1;
                m_api.CreateAccount(&m_account);
            }
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "SubmitForgotPassword"))
    {
        if (!m_dialog || !m_page)
            return;
        const char *user = m_page->GetText("Username");
        const char *day = m_page->GetText("BirthDay");
        const char *month = m_page->GetText("BirthMonth");
        const char *year = m_page->GetText("BirthYear");
        const char *email = m_page->GetText("EmailAddress");
        if (user && day && month && year && email)
        {
            VIColor32F red(0.90980393f, 0.54901963f, 0.22352941f, 1.0f);
            bool ok = true;
            bool valid;
            valid = CheckName(this, m_page, user, "UsernameLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, day, "BirthDayLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, month, "BirthMonthLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, year, "BirthYearLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, email, "EmailAddressLabel", red);
            ok &= valid;
            switch (CheckEmail(email))
            {
            case 1:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1390));
                return;
            case 2:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1392));
                return;
            case 3:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1391));
                return;
            case 4:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1393));
                return;
            }
            if (!ok)
            {
                m_dialog->Open(Word(UI(), 1288), 0);
                m_dialog->SetText(Word(UI(), 1310));
            }
            else
            {
                m_dialog->Open(Word(UI(), 1268), 0);
                m_dialog->SetText(Word(UI(), 758));
                m_dialog->SetConfirm();
                m_api.Request66(user, month, day, year, email);
            }
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "SubmitAcctUpdate"))
    {
        if (!m_dialog || !m_page)
            return;
        const char *country = m_page->GetText("Country");
        const char *postal = m_page->GetText("PostalCode");
        const char *email = m_page->GetText("EmailAddress");
        if (country && postal)
        {
            VIColor32F red(0.90980393f, 0.54901963f, 0.22352941f, 1.0f);
            bool ok = true;
            bool valid;
            valid = CheckName(this, m_page, country, "CountryLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, postal, "PostalCodeLabel", red);
            ok &= valid;
            switch (CheckEmail(email))
            {
            case 1:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1390));
                return;
            case 2:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1392));
                return;
            case 3:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1391));
                return;
            case 4:
                m_dialog->Open(Word(UI(), 1286), 0);
                m_dialog->SetText(Word(UI(), 1393));
                return;
            }
            if (!strncmp(country, "US -", 4) && !IsZip(postal))
            {
                m_dialog->Open(Word(UI(), 1293), 0);
                m_dialog->SetText(Word(UI(), 1354));
            }
            else if (!ok)
            {
                m_dialog->Open(Word(UI(), 1288), 0);
                m_dialog->SetText(Word(UI(), 1310));
            }
            else
            {
                m_dialog->Open(Word(UI(), 1369), 0);
                m_dialog->SetText(Word(UI(), 758));
                m_dialog->SetConfirm();
                m_api.Request64(m_uid, country, postal, email);
            }
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "SubmitChangePassword"))
    {
        m_dialog->Open(Word(UI(), 1229), 0);
        if (!m_dialog)
            return;
        m_dialog->SetText(Word(UI(), 758));
        const char *current = m_page->GetText("CurrentPassword");
        const char *password = m_page->GetText("NewPassword");
        const char *password2 = m_page->GetText("NewPassword2");
        if (current && password && password2)
        {
            if (!*current || !*password || !*password2)
                m_dialog->SetText(Word(UI(), 1305));
            else if (strcmp(current, m_password))
                m_dialog->SetText(Word(UI(), 1245));
            else if (strcmp(password, password2))
            {
                m_dialog->Open(Word(UI(), 1225), 0);
                m_dialog->SetText(Word(UI(), 1308));
            }
            else
            {
                m_dialog->SetConfirm();
                m_api.ChangePassword(m_uid, current, password);
            }
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "SubmitCheckAccountKey"))
    {
        if (!m_dialog)
            return;
        m_dialog->Open(Word(UI(), 1231), 0);
        m_dialog->SetText(Word(UI(), 758));
        char *key = (char *)m_page->GetText("AccountKey");
        if (key)
        {
            m_dialog->SetConfirm();
            const char *game = VIGetGameCode();
            m_api.Request48(m_uid, game, ToUpper(key));
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "SubmitCheckGameCard"))
    {
        if (!m_dialog)
            return;
        m_dialog->Open(Word(UI(), 1230), 0);
        m_dialog->SetText(Word(UI(), 758));
        char *card = (char *)m_page->GetText("GameCardNo");
        if (card)
        {
            strncpy(m_gameCard, ToUpper(card), 63);
            m_api.Request54(m_uid, VIGetGameCode(), card);
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "SubmitConsumeGameCard"))
    {
        if (!m_dialog)
            return;
        m_dialog->Open(Word(UI(), 1230), 0);
        m_dialog->SetText(Word(UI(), 758));
        strncpy(m_billing.m_x1E0, "gamecard", 32);
        strncpy(m_billing.m_x1A0, m_gameCard, 64);
        m_api.Request46(m_uid, m_billing, m_sku);
    }
    else if (!strcmp(action, "SubmitRental"))
    {
        if (!m_dialog)
            return;
        const char *rental = m_page->GetText("RentalNumber");
        if (rental)
        {
            m_dialog->Open(Word(UI(), 1232), 0);
            m_dialog->SetText(Word(UI(), 758));
            strncpy(m_billing.m_x1E0, "rental", 32);
            strncpy(m_billing.m_x1A0, rental, 64);
            m_api.Request46(m_uid, m_billing, m_sku);
        }
        else
        {
            m_dialog->Open(Word(UI(), 767), 0);
            m_dialog->SetText(Word(UI(), 1291));
        }
    }
    else if (!strcmp(action, "SubmitPurchaseSubscription"))
    {
        if (!m_dialog)
            return;
        m_dialog->Open(Word(UI(), 1329), 0);
        m_dialog->SetText(Word(UI(), 758));
        if (m_page->GetText("BillingPlan"))
        {
            const char *selected = m_page->GetText("BillingPlan");
            for (int i = m_plans.Begin(); i != m_plans.End(); i = m_plans.Next(i))
            {
                char text[1024];
                VISubscriptionPlan plan = ((VIList<VISubscriptionPlan>::Node *)i)->m_data;
                sprintf(text, "%s ($%s)", plan.m_shortDesc, plan.m_price);
                if (!strcmp(text, selected))
                {
                    printf("Selected plan = %s\n", text);
                    sprintf(m_sku, plan.m_sku);
                }
            }
            printf("!!!sku = %s\n", m_sku);
            m_api.Request46(m_uid, m_billing, m_sku);
        }
        else
            m_dialog->SetText(Word(UI(), 1264));
    }
    else if (!strcmp(action, "ContinueUpdateCCInfo"))
    {
        const char *first = m_page->GetText("FirstName");
        const char *last = m_page->GetText("LastName");
        const char *address = m_page->GetText("Address");
        const char *city = m_page->GetText("City");
        const char *state = m_page->GetText("State");
        const char *postal = m_page->GetText("PostalCode");
        const char *country = m_page->GetText("Country");
        if (first && last && address && city && state && postal && country)
        {
            VIColor32F red(0.90980393f, 0.54901963f, 0.22352941f, 1.0f);
            if (!m_dialog)
                return;
            bool ok = CheckName(this, m_page, first, "FirstNameLabel", red);
            bool valid;
            valid = CheckName(this, m_page, last, "LastNameLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, address, "AddressLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, city, "CityLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, state, "StateLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, postal, "PostalCodeLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, country, "CountryLabel", red);
            ok &= valid;
            if (!strncmp(country, "US -", 4) && !IsZip(postal))
            {
                m_dialog->Open(Word(UI(), 1293), 0);
                m_dialog->SetText(Word(UI(), 1354));
            }
            else if (!ok)
            {
                m_dialog->Open(Word(UI(), 1284), 0);
                m_dialog->SetText(Word(UI(), 1310));
            }
            else
            {
                strncpy(m_billing.m_x000, address, 127);
                strncpy(m_billing.m_x080, city, 63);
                strncpy(m_billing.m_x0C0, country, 31);
                strncpy(m_billing.m_x0E0, state, 63);
                strncpy(m_billing.m_x120, first, 31);
                strncpy(m_billing.m_x140, last, 31);
                strncpy(m_billing.m_x160, postal, 31);
                LoadPage("data/ui/ChangeCC2.xml");
            }
        }
        else
        {
            m_dialog->Open(Word(UI(), 1213), 0);
            m_dialog->SetText(Word(UI(), 1264));
        }
    }
    else if (!strcmp(action, "SubmitUpdateCCInfo"))
    {
        const char *type = m_page->GetText("CardType");
        const char *number = m_page->GetText("CardNo");
        const char *month = m_page->GetText("ExpireMonth");
        const char *year = m_page->GetText("ExpireYear");
        if (type && number && month && year)
        {
            VIColor32F red(0.90980393f, 0.54901963f, 0.22352941f, 1.0f);
            if (!m_dialog)
                return;
            m_dialog->Open(Word(UI(), 1329), 0);
            m_dialog->SetText(Word(UI(), 758));
            bool ok = CheckName(this, m_page, type, "CardTypeLabel", red);
            bool valid;
            valid = CheckName(this, m_page, number, "CardNoLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, month, "ExpireMonthLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, year, "ExpireYearLabel", red);
            ok &= valid;
            if (!IsNumber(number))
            {
                m_dialog->Open(Word(UI(), 1285), 0);
                m_dialog->SetText(Word(UI(), 1326));
            }
            else if (!ok)
            {
                m_dialog->Open(Word(UI(), 1284), 0);
                m_dialog->SetText(Word(UI(), 1310));
            }
            else
            {
                strncpy(m_billing.m_x180, year, 15);
                strncpy(m_billing.m_x190, month, 15);
                strncpy(m_billing.m_x1A0, number, 63);
                strncpy(m_billing.m_x1E0, type, 15);
                m_api.Request46(m_uid, m_billing, m_sku);
            }
        }
        else
        {
            m_dialog->Open(Word(UI(), 1213), 0);
            m_dialog->SetText(Word(UI(), 1264));
        }
    }
    else if (!strcmp(action, "CancelAccount"))
    {
        if (!m_dialog)
            return;
        m_dialog->Open(Word(UI(), 1220), 0);
        m_dialog->SetText(Word(UI(), 758));
        m_api.Request52(m_uid, VIGetGameCode());
    }
    else if (!strcmp(action, "UpdateBilling"))
    {
        if (m_paymentType == -1 || m_subStatus == -1)
        {
            if (m_loginStatus == 9)
            {
                m_dialog->Open(Word(UI(), 1331), 0);
                m_dialog->SetText(Word(UI(), 1375));
            }
            else
            {
                m_dialog->Open(Word(UI(), 1274), 0);
                m_dialog->SetText(Word(UI(), 1337));
                sprintf(m_action, action);
                m_dialog->SetConfirm();
                m_api.Request50(m_uid, VIGetGameCode());
            }
        }
        else if (m_subStatus == 5)
        {
            m_dialog->Open(Word(UI(), 1214), 0);
            m_dialog->SetText(Word(UI(), 1388));
        }
        else if (m_loginStatus == 9)
        {
            m_dialog->Open(Word(UI(), 1214), 0);
            m_dialog->SetText(Word(UI(), 1377));
        }
        else if (m_paymentType == 2)
            LoadPage("data/ui/UpGlobalCollect.xml");
        else if (m_paymentType == 0)
            LoadPage("data/ui/UpCC.xml");
        else
            LoadPage("data/ui/UpGC.xml");
    }
    else if (!strcmp(action, "CancelAccountLink"))
    {
        if (m_subStatus == -1)
        {
            m_dialog->Open(Word(UI(), 1273), 0);
            m_dialog->SetText(Word(UI(), 1336));
            sprintf(m_action, action);
            m_dialog->SetConfirm();
            m_api.Request50(m_uid, VIGetGameCode());
        }
        else if (m_subStatus == 5)
        {
            m_dialog->Open(Word(UI(), 1220), 0);
            m_dialog->SetText(Word(UI(), 1387));
        }
        else if (m_subStatus == 0 || m_subStatus == 1)
        {
            m_dialog->Open(Word(UI(), 1220), 0);
            m_dialog->SetText(Word(UI(), 1382));
        }
        else
            LoadPage("data/ui/Cancel.xml");
    }
    else if (!strcmp(action, "ForgotPassword"))
    {
        if (!m_dialog)
            return;
        m_dialog->Open(Word(UI(), 1313), 0);
        m_dialog->SetText(Word(UI(), 1248));
    }
    else if (!strcmp(action, "CustomerService"))
    {
        if (!m_dialog)
            return;
        LoadPage("data/ui/CSPage.xml");
    }
    else if (!strcmp(action, "NewCSTicket"))
        m_cs.OpenRequest(m_login, m_password);
    else if (!strcmp(action, "CSTicketBrowser"))
        m_cs.OpenInbox(m_login, m_password);
    else if (!strcmp(action, "RentEQOA"))
    {
        if (!m_dialog)
            return;
        m_dialog->Open(Word(UI(), 1332), 0);
        m_dialog->SetText(Word(UI(), 1334));
    }
    else if (!strcmp(action, "ContinueCCInfo"))
    {
        const char *first = m_page->GetText("FirstName");
        const char *last = m_page->GetText("LastName");
        const char *address = m_page->GetText("Address");
        const char *city = m_page->GetText("City");
        const char *state = m_page->GetText("State");
        const char *postal = m_page->GetText("PostalCode");
        const char *country = m_page->GetText("Country");
        if (first && last && address && city && state && postal && country)
        {
            VIColor32F red(0.90980393f, 0.54901963f, 0.22352941f, 1.0f);
            if (!m_dialog)
                return;
            bool ok = CheckName(this, m_page, first, "FirstNameLabel", red);
            bool valid;
            valid = CheckName(this, m_page, last, "LastNameLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, address, "AddressLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, city, "CityLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, state, "StateLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, postal, "PostalCodeLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, country, "CountryLabel", red);
            ok &= valid;
            if (!strncmp(country, "US -", 4) && !IsZip(postal))
            {
                m_dialog->Open(Word(UI(), 1293), 0);
                m_dialog->SetText(Word(UI(), 1354));
            }
            else if (!ok)
            {
                m_dialog->Open(Word(UI(), 1284), 0);
                m_dialog->SetText(Word(UI(), 1310));
            }
            else
            {
                strncpy(m_billing.m_x000, address, 127);
                strncpy(m_billing.m_x080, city, 63);
                strncpy(m_billing.m_x0C0, country, 31);
                strncpy(m_billing.m_x0E0, state, 63);
                strncpy(m_billing.m_x120, first, 31);
                strncpy(m_billing.m_x140, last, 31);
                strncpy(m_billing.m_x160, postal, 31);
                LoadPage("data/ui/CCSetup2.xml");
            }
        }
        else
        {
            m_dialog->Open(Word(UI(), 1213), 0);
            m_dialog->SetText(Word(UI(), 1264));
        }
    }
    else if (!strcmp(action, "SubmitCCInfo"))
    {
        const char *type = m_page->GetText("CardType");
        const char *number = m_page->GetText("CardNo");
        const char *month = m_page->GetText("ExpireMonth");
        const char *year = m_page->GetText("ExpireYear");
        if (type && number && month && year)
        {
            VIColor32F red(0.90980393f, 0.54901963f, 0.22352941f, 1.0f);
            if (!m_dialog)
                return;
            bool ok = CheckName(this, m_page, type, "CardTypeLabel", red);
            bool valid;
            valid = CheckName(this, m_page, number, "CardNoLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, month, "ExpireMonthLabel", red);
            ok &= valid;
            valid = CheckName(this, m_page, year, "ExpireYearLabel", red);
            ok &= valid;
            if (!IsNumber(number))
            {
                m_dialog->Open(Word(UI(), 1285), 0);
                m_dialog->SetText(Word(UI(), 1326));
            }
            else if (!ok)
            {
                m_dialog->Open(Word(UI(), 1284), 0);
                m_dialog->SetText(Word(UI(), 1310));
            }
            else
            {
                strncpy(m_billing.m_x180, year, 15);
                strncpy(m_billing.m_x190, month, 15);
                strncpy(m_billing.m_x1A0, number, 63);
                strncpy(m_billing.m_x1E0, type, 15);
                m_dialog->SetConfirm();
                m_dialog->Open(Word(UI(), 758), 0);
                m_dialog->SetText(Word(UI(), 1252));
                m_api.Request62(m_uid, VIGetGameCode());
            }
        }
        else
        {
            m_dialog->Open(Word(UI(), 1213), 0);
            m_dialog->SetText(Word(UI(), 1264));
        }
    }
    else if (!strcmp(action, "PopDialog"))
        HideDialog();
    else if (!strcmp(action, "PlayEQOA"))
    {
        if (m_loginStatus == 7)
        {
            m_dialog->Open(Word(UI(), 1346), 0);
            m_dialog->SetText(Word(UI(), 1386));
        }
        else if (m_loggedIn && GetAppPtr())
        {
            m_dialog->SetConfirm();
            m_dialog->Open(Word(UI(), 1275), "REALLYPlayEQOA");
            m_dialog->SetText(Word(UI(), 1810));
            m_newsActive = 1;
            m_newsWait = 4;
            m_newsPoll = 4;
            m_dialog->SetPrompt(0);
        }
    }
    else if (!strcmp(action, "REALLYPlayEQOA"))
    {
        if (!m_loggedIn)
            return;
        VIStation *station = (VIStation *)GetAppPtr();
        if (!station)
            return;
        station->FadeOut();
        m_result = 1;
    }
    else if (!strcmp(action, "Disconnect"))
    {
        if (!GetAppPtr())
            return;
        if (!m_dialog)
            return;
        m_dialog->SetConfirm();
        m_dialog->Open(Word(UI(), 1234), "DisconnectConfirmed");
        m_dialog->SetText(Word(UI(), 1212));
    }
    else if (!strcmp(action, "DisconnectConfirmed"))
    {
        VIStation *station = (VIStation *)GetAppPtr();
        if (!station)
            return;
        printf("DISCONNECTING\n");
        station->FadeOut();
        m_result = 2;
    }
}

// 0x002CFA98.
void VIWndGenericRenderer::OnDrawSelf()
{
    VIStation *station = (VIStation *)GetAppPtr();
    if (m_removeChild)
    {
        printf("!!!Removing the child\n");
        RemoveChild(m_removeChild);
        m_removeChild = 0;
        return;
    }
    if (m_checkNet)
    {
        if (m_child)
            m_child->OnDraw();
        else
        {
            m_page->Draw();
            if (m_dialog && m_dialog->IsOpen())
                m_dialog->Draw();
        }
        if (++sStatusFrames0 % 300 == 0)
        {
            VINetStatus status;
            status.Init();
            printf("Checking status (0)...\n");
            if (status.GetState() && m_result != 2)
            {
                m_editor->m_visible = 0;
                station->m_xB40C = (int)this;
                station->m_wm.SetFocus(this, true);
                printf("Network disconnected (0)\n");
                m_dialog->ClearConfirm();
                m_dialog->SetText(Word(UI(), 1395));
                m_dialog->Open(Word(UI(), 1306), "DisconnectConfirmed");
                m_checkNet = 0;
            }
        }
        return;
    }
    if (!IsLoaded())
        return;
    if (sDisconnected == 1)
    {
        OnAction("DisconnectConfirmed");
        sDisconnected = 0;
    }
    if (!m_page)
        return;
    if (sPingDelay >= 6)
    {
        m_api.Process();
        if (m_cs.IsActive())
            m_cs.Update();
    }
    if (sPingDelay < 1000)
        sPingDelay++;
    if (m_child)
        m_child->OnDraw();
    else
    {
        m_page->Draw();
        if (m_dialog && m_dialog->IsOpen())
            m_dialog->Draw();
    }
    if (++sStatusFrames % 300 == 0)
    {
        VINetStatus status;
        status.Init();
        printf("Checking status...\n");
        if (status.GetState() && m_result != 2)
        {
            m_editor->m_visible = 0;
            station->m_xB40C = (int)this;
            station->m_wm.SetFocus(this, true);
            printf("Network disconnected\n");
            m_dialog->ClearConfirm();
            m_dialog->SetText(Word(UI(), 1395));
            m_dialog->Open(Word(UI(), 1306), "DisconnectConfirmed");
        }
    }
    if (m_saveDelay && m_result != 2)
    {
        m_saveDelay--;
        UI()->Begin2D();
        VIVect2I pos(0, 0);
        VIVect2I size(320, 230);
        sSaveWindow.Init(UI(), pos, size, 460, 5, 0);
        int rows = sSaveWindow.CountLines(Word(UI(), 1765), 380);
        sSaveWindow.Init(UI(), pos, size, 460, rows, 0);
        sSaveWindow.Open(true);
        sSaveWindow.Draw();
        sSaveWindow.TextBox(Word(UI(), 1765), 40, 0, sSaveWindow.m_size.x - 80, rows,
                            (VIUICOLOR)0, 8);
        UI()->End2D();
        if (!m_saveDelay && VIGetGameOptions()->Save() < 0)
        {
            sAccountSaved = 1;
            McErr(this, VIVect2I(320, 224), 4);
        }
    }
    if (!m_newsActive)
        return;
    if (m_newsWait)
    {
        m_dialog->SetPrompt(0);
        m_newsWait--;
        return;
    }
    if (!m_newsUrl)
    {
        m_newsUrl = new VUrl(m_newsURL);
        if (!m_newsUrl)
        {
            printf("!!SAY WHAT????\n");
            return;
        }
    }
    printf("doing passive get\n");
    int r = m_newsUrl->GetPoll(m_news, sizeof(m_news));
    if (m_newsPoll && r != 4)
    {
        m_dialog->SetPrompt(1);
        m_newsPoll--;
        return;
    }
    printf("done\n");
    m_dialog->SetPrompt(1);
    m_newsPoll = 0;
    if (r == 3)
    {
        int i = 0;
        while ((unsigned char)(m_news[i] - 9) < 2)
        {
            printf("Ascii: %i\n", m_news[i]);
            i++;
        }
        if (VIGetExpansionFlags() & 2)
            m_dialog->Open(Word(UI(), 1804), "REALLYPlayEQOA");
        else
            m_dialog->Open(Word(UI(), 1260), "REALLYPlayEQOA");
        m_dialog->SetText(m_news + i);
        m_newsActive = 0;
        m_dialog->ClearConfirm();
        if (m_newsUrl)
            delete m_newsUrl;
        m_newsUrl = 0;
    }
    else if (r == 4)
    {
        m_dialog->SetText(Word(UI(), 1238));
        printf("%s\n", "Could not download news, please continue.");
        m_newsActive = 0;
        m_dialog->ClearConfirm();
        if (m_newsUrl)
            delete m_newsUrl;
        m_newsUrl = 0;
    }
}

// 0x002D0220.
void VIWndGenericRenderer::OnKeyUp(int key)
{
    if (m_result)
        return;
    if (!m_page)
        return;
    if (m_dialog && m_dialog->IsOpen())
    {
        m_dialog->OnKeyUp(key);
        return;
    }
    if (m_comboParent)
    {
        m_combo.OnKeyUp(key);
        return;
    }
    if (m_editParent)
    {
        m_edit.OnKeyUp(key);
        return;
    }
    if (m_child)
    {
        m_child->OnKeyUp(key);
        return;
    }
    m_page->OnKeyUp(key);
}

// 0x002D0320.  Escape is Triangle; the arrow keys and Enter are the pad's.
void VIWndGenericRenderer::OnKeyDown(int key)
{
    if (m_result)
        return;
    if (key == 27)
        key = 132;
    if (key == 19)
        key = 138;
    else if (key == 20)
        key = 139;
    else if (key == 18)
        key = 137;
    else if (key == 17)
        key = 136;
    else if (key == 13 && !m_editParent)
        key = 134;
    if (m_dialog && m_dialog->IsOpen())
    {
        m_dialog->OnKeyDown(key);
        return;
    }
    if (m_comboParent)
    {
        m_combo.OnKeyDown(key);
        return;
    }
    if (m_editParent)
    {
        m_edit.OnKeyDown(key);
        return;
    }
    if (m_child)
    {
        m_child->OnKeyDown(key);
        return;
    }
    if (m_pages.Size() == 1 && key == 132 && !m_dialog->IsOpen())
    {
        OnAction("Disconnect");
        return;
    }
    if (m_page)
        m_page->OnKeyDown(key);
}

// 0x002D04D8.
void VIWndGenericRenderer::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    printf("!!Entered OnMessage, id == %i\n", msg);
    switch (msg)
    {
    case 307:
        SetFocus(this, true);
        break;
    case 308:
        CloseCombo();
        break;
    case 4356:
        SetFocus(w, true);
        break;
    case 4352:
        AddChild(w);
        break;
    case 4354:
        printf("!!!Remove Child.....\n");
        m_removeChild = w;
        printf("!!!removing child, param = %i\n", p1);
        break;
    case 4358:
        if (p1 == 3)
            sDisconnected = 1;
        printf("DONE!  code = %i\n", p1);
        sCardError = 0;
        SetFocus(this, true);
        m_done++;
        if (sAccountSaved)
            SaveAccount();
        break;
    default:
        printf("!!Yo, its the default!\n");
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
    if (m_page)
    {
        if (m_dialog && m_dialog->IsOpen())
            m_dialog->OnMessage(w, msg, p1, p2);
        else
            m_page->OnMessage(w, msg, p1, p2);
    }
}

// 0x002D0710.  The two background halves, and a framed text box over them.
void VIWndGenericRenderer::DrawBackground(VIUI *ui, int alt, const unsigned short *title,
                                          const unsigned short *text)
{
    VIVect2I pos(0, 0);
    ui->Blit(VIHashResourceID(alt ? "P:\\studio\\eqps2\\Game_Assets\\UI\\2stbk00.tga"
                                  : "P:\\studio\\eqps2\\Game_Assets\\UI\\stbk00.tga"),
             pos, (VIAlphaBlendMode)0, 1.0f, 0);
    pos.x = 320;
    pos.y = 0;
    ui->Blit(VIHashResourceID(alt ? "P:\\studio\\eqps2\\Game_Assets\\UI\\2stbk10.tga"
                                  : "P:\\studio\\eqps2\\Game_Assets\\UI\\stbk10.tga"),
             pos, (VIAlphaBlendMode)0, 1.0f, 0);
    VIUIWindow w;
    VIVect2I size(520, 210);
    pos.x = 0;
    pos.y = 0;
    w.Init(ui, pos, size, 200, 14, 0);
    w.Close(true);
    if (title && text)
    {
        VIVect2I tl = w.TopLeft();
        VIVect2I br = w.BottomRight();
        ui->StretchBlitEx((VIUITexId)61, w.TopLeft() + VIVect2I(0, 12),
                          VIVect2I(br.x - tl.x, br.y - tl.y) + VIVect2I(0, 9),
                          (VIAlphaBlendMode)3, 0.5f);
        w.TextBox(title, 10, 0, 180, 1, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 8);
        w.TextBox(text, 10, 2, 180, 14, VIColor32F(1.0f, 1.0f, 1.0f, 1.0f), 0);
    }
}

// 0x002D09C0.  A pulse on the spinner sprite, one turn a 2*pi.
void VIWndGenericRenderer::DrawSpinner(VIUI *ui, const VIVect2I &pos)
{
    float t = (VITimer::GetTime() - m_spinStart) * 0.01f;
    if (t > 6.2831855f)
        t -= 6.2831855f;
    ui->BlitEx((VIUITexId)89, pos, (VIAlphaBlendMode)3, sinf(t + 1.5707964f) * 0.5f + 0.5f, 0);
}

// 0x002D0A90.
void VIWndGenericRenderer::ResetSpinner()
{
    m_spinStart = VITimer::GetTime();
}

// 0x002D0AC8.
int VIWndGenericRenderer::OpenCombo(VIWnd *parent, VIOptionItem *item)
{
    if (m_comboParent)
        return -1;
    m_comboParent = parent;
    parent->AddChild(&m_combo);
    m_combo.SetItem(item);
    m_combo.SetVisible(true);
    return 0;
}

// 0x002D0B48.
VIWndCombo *VIWndGenericRenderer::GetCombo()
{
    return &m_combo;
}

// 0x002D0B58.
int VIWndGenericRenderer::ComboOpen()
{
    return m_comboParent != 0;
}

// 0x002D0B70.
void VIWndGenericRenderer::CloseCombo()
{
    if (m_comboParent)
    {
        m_combo.SetVisible(false);
        m_comboParent->RemoveChild(&m_combo);
        m_comboParent = 0;
    }
}

// 0x002D0BC0.
int VIWndGenericRenderer::BeginEdit(VIWnd *parent, const unsigned short *title,
                                    const unsigned short *text, int maxLen, int password)
{
    if (m_editParent)
        return -1;
    m_editParent = parent;
    parent->AddChild(&m_edit);
    m_edit.SetCenter(VIVect2I(320, 224));
    m_edit.SetMultiLine(0);
    m_edit.SetMaxLength(maxLen);
    m_edit.SetTitle(title);
    m_edit.SetRows(1);
    m_edit.SetText(text);
    m_edit.SetX18C(password != 0);
    m_edit.SetVisible(true);
    return 0;
}

// 0x002D0CB8.
VIWndEdit *VIWndGenericRenderer::GetEdit()
{
    return &m_edit;
}

// 0x002D0CC8.
int VIWndGenericRenderer::Editing()
{
    return m_editParent != 0;
}

// 0x002D0CE0.  Off the stack when there was no edit (a shipped bug).
unsigned short *VIWndGenericRenderer::EndEdit()
{
    unsigned short none[1];
    unsigned short *text;
    if (m_editParent)
    {
        m_edit.SetVisible(false);
        m_editParent->RemoveChild(&m_edit);
        m_editParent = 0;
        text = m_edit.GetText();
    }
    else
    {
        none[0] = 0;
        printf("EndEdit() : not in edit mode!\n");
        text = none;
    }
    return text;
}

// VIList::Insert is declared in vi.h and defined nowhere else in the tree (lane
// misc's vixmlparser.cpp carries the same text): v goes in front of node i, or
// at the end when i is End().
// (VIList<T>::Insert is in src/vilist_t.h.)


















