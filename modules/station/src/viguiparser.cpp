// viguiparser.cpp (name ours) -- STATION 0x002C45F8 .. 0x002C73D0.  The
// PS2GUIPage parser: STATION.PAK's page files are XML, and each element type
// pushes a handler on the XML reader (VIXMLParser, stwnd.h) that reads that
// element's children and pops itself at the element's close.
//
//   <PS2GUIPage gridWidth gridHeight>      PageHandler
//     <Text|Toggle|ComboBox|Button|Object|Selector name val ...>   ElementHandler
//       <Position x y/> <Image .../> <TextColor r g b/> ...   the object's handler
#include "station.h"
#include "vilist_t.h"

extern "C" unsigned int VIHashResourceID(const char *s);

// 0x00350128: what a ComboBox's namedRange="states" fills it with.
static const char *sStates[] =
{
    "AL - Alabama", "AK - Alaska", "AZ - Arizona", "AR - Arkansas",
    "AA - Armed Forces South America", "AE - Armed Forces Europe", "AP - Armed Forces Pacific",
    "CA - California", "CO - Colorado", "CT - Connecticut", "DE - Delaware",
    "DC - District of Columbia", "FL - Florida", "GA - Georgia", "HI - Hawaii", "ID - Idaho",
    "IL - Illinois", "IN - Indiana", "IA - Iowa", "KS - Kansas", "KY - Kentucky",
    "LA - Louisiana", "ME - Maine", "MD - Maryland", "MA - Massachusetts", "MI - Michigan",
    "MN - Minnesota", "MS - Mississippi", "MO - Missouri", "MT - Montana", "NE - Nebraska",
    "NV - Nevada", "NH - New Hampshire", "NJ - New Jersey", "NM - New Mexico", "NY - New York",
    "NC - North Carolina", "ND - North Dakota", "OH - Ohio", "OK - Oklahoma", "OR - Oregon",
    "OT - Other (Non USA)", "PA - Pennsylvania", "RI - Rhode Island", "SC - South Carolina",
    "SD - South Dakota", "TN - Tennessee", "TX - Texas ", "UT - Utah", "VT - Vermont",
    "VA - Virginia", "WA - Washington", "WV - West Virginia", "WI - Wisconsin", "WY - Wyoming",
    0,
};

// 0x002C45F8.
VIGUIParser::VIGUIParser(VIWndGenericRenderer *owner)
{
    m_page = 0;
    m_object = 0;
    m_owner = owner;
}

// 0x002C4610.  "DarkTile" and "BlueTile" are compared case-sensitively, and
// "DarkTile" and "LoginBarTop" are in the list twice.  One result and an
// else-if chain: a `return` per case lays the last one out the other way.
int VIGUITextureId(const char *name)
{
    int id;
    if (!name)
        return 0;
    if (!strcmp(name, "DarkTile"))
        id = 91;
    else if (!strcmp(name, "BlueTile"))
        id = 32;
    else if (!strcasecmp(name, "GrayTile"))
        id = 51;
    else if (!strcasecmp(name, "MetalTopTile"))
        id = 37;
    else if (!strcasecmp(name, "TitleTile"))
        id = 34;
    else if (!strcasecmp(name, "MetalLeftTile"))
        id = 42;
    else if (!strcasecmp(name, "Scrollwork"))
        id = 4;
    else if (!strcasecmp(name, "BluePointerR"))
        id = 89;
    else if (!strcasecmp(name, "BluePointerL"))
        id = 88;
    else if (!strcasecmp(name, "BluePointerU"))
        id = 90;
    else if (!strcasecmp(name, "BluePointerD"))
        id = 87;
    else if (!strcasecmp(name, "LoginBarTop"))
        id = 163;
    else if (!strcasecmp(name, "LoginBarVert"))
        id = 161;
    else if (!strcasecmp(name, "LoginBarBott"))
        id = 162;
    else if (!strcasecmp(name, "LoginBarTop"))
        id = 163;
    else if (!strcasecmp(name, "TopBlueGem"))
        id = 145;
    else if (!strcasecmp(name, "TopRedGem"))
        id = 144;
    else if (!strcasecmp(name, "BottomBlueGem"))
        id = 147;
    else if (!strcasecmp(name, "BottomRedGem"))
        id = 146;
    else if (!strcasecmp(name, "LoginParchScroll"))
        id = 143;
    else if (!strcasecmp(name, "LoginPlaqueLeft"))
        id = 148;
    else if (!strcasecmp(name, "LoginPlaqueTile"))
        id = 167;
    else if (!strcasecmp(name, "ParchIndentCap"))
        id = 141;
    else if (!strcasecmp(name, "ParchIndentTile"))
        id = 150;
    else if (!strcasecmp(name, "DarkTile"))
        id = 91;
    else if (!strcasecmp(name, "TargetCap"))
        id = 25;
    else if (!strcasecmp(name, "TargetTile"))
        id = 26;
    else if (!strcasecmp(name, "StationX"))
        id = 173;
    else if (!strcasecmp(name, "StationSqr"))
        id = 174;
    else if (!strcasecmp(name, "StationTri"))
        id = 175;
    else if (!strcasecmp(name, "StationCir"))
        id = 176;
    else if (!strcasecmp(name, "TextCapL"))
        id = 177;
    else if (!strcasecmp(name, "TextCapR"))
        id = 178;
    else if (!strcasecmp(name, "TextTile"))
        id = 179;
    else if (!strcasecmp(name, "CBox"))
        id = 180;
    else if (!strcasecmp(name, "CBoxCheck"))
        id = 181;
    else if (!strcasecmp(name, "ButtonCapL"))
        id = 182;
    else if (!strcasecmp(name, "ButtonCapR"))
        id = 183;
    else if (!strcasecmp(name, "ButtonTile"))
        id = 184;
    else
    {
        printf("!!Warning: using numeric text id %s\n", name);
        id = atoi(name);
    }
    return id;
}

// 0x002C4A98.  Any object's children; "Object" and "Selector" close here.
static void ObjectHandler(VIXMLEvent e)
{
    VIGUIParser *p = (VIGUIParser *)e.GetData();
    if (!p)
        return;
    VIXMLParser *reader = e.GetParser();
    if (!reader)
        return;
    const char *name = e.GetName();
    if (!name)
        return;
    VIGUIObject *o = p->GetObject();
    if (!o)
        return;
    if (e.GetType() == 0)
    {
        if (!strcasecmp(name, "Position"))
        {
            int x = e.GetPropInt("x", 0);
            int y = e.GetPropInt("y", 0);
            o->SetPos(x, y);
        }
        else if (!strcasecmp(name, "SelectorPos"))
        {
            int x = e.GetPropInt("x", -1);
            int y = e.GetPropInt("y", -1);
            VIGUIPage *page = p->GetPage();
            if (page)
            {
                page->SetSelectorX(x);
                page->SetSelectorY(y);
            }
        }
        else if (!strcasecmp(name, "SelectEffect"))
        {
            char *type = e.GetProp("type");
            if (type)
            {
                if (!strcasecmp(type, "scrollleft"))
                    o->SetSelectEffect(1);
                else if (!strcasecmp(type, "fadealpha"))
                    o->SetSelectEffect(2);
                free(type);
            }
            o->SetXTarget(e.GetPropInt("xtarget", 0));
            o->SetAlphaTarget(e.GetPropInt("alphatarget", 255) / 255.0f);
        }
        else if (!strcasecmp(name, "ButtonPos"))
        {
            int x = e.GetPropInt("x", 0);
            int y = e.GetPropInt("y", 0);
            o->SetButtonPos(x, y);
        }
        else if (!strcasecmp(name, "ImageOffset"))
        {
            int x = e.GetPropInt("x", 0);
            int y = e.GetPropInt("y", 0);
            o->SetImageOffset(x, y);
        }
        else if (!strcasecmp(name, "BGColor"))
        {
            float r = e.GetPropInt("r", 0) / 255.0f;
            float g = e.GetPropInt("g", 0) / 255.0f;
            float b = e.GetPropInt("b", 0) / 255.0f;
            float a = e.GetPropInt("a", 0) / 255.0f;
            o->SetBGColor(VIColor32F(r, g, b, a));
        }
        else if (!strcasecmp(name, "Image"))
        {
            char *image = e.GetProp("imageName");
            char *type = e.GetProp("imageType");
            char *tile = e.GetProp("tile");
            char *cap = e.GetProp("capTexture");
            o->SetAlpha(e.GetPropInt("alpha", 255) / 255.0f);
            if (image)
            {
                unsigned int id = VIHashResourceID(image);
                o->SetImage(id);
                free(image);
            }
            else if (type)
            {
                int id = VIGUITextureId(type);
                o->SetTexture(id);
                free(type);
            }
            if (tile)
            {
                if (!strcasecmp(tile, "horiz"))
                    o->SetTileMode(1);
                else if (!strcasecmp(tile, "vert"))
                    o->SetTileMode(2);
                else if (!strcasecmp(tile, "both"))
                    o->SetTileMode(3);
                free(tile);
            }
            if (cap)
            {
                int id = VIGUITextureId(cap);
                o->SetCapTexture(id);
                free(cap);
            }
        }
        else if (!strcasecmp(name, "Size"))
        {
            int w = e.GetPropInt("width", -1);
            int h = e.GetPropInt("height", -1);
            if (w != -1 && h != -1)
                o->SetSize(w, h);
        }
        else if (!strcasecmp(name, "Tileset"))
        {
            char *type = e.GetProp("type");
            if (type)
            {
                if (!strcasecmp(type, "Parchment"))
                    o->SetTileset(0);
                free(type);
            }
        }
        else if (!strcasecmp(name, "Depth"))
            o->SetDepth(e.GetPropInt("index", 0));
    }
    else if (e.GetType() == 1)
    {
        if (!strcasecmp(name, "Object"))
        {
            reader->Pop();
            VIGUIPage *page = p->GetPage();
            if (page)
                page->AddObject(o);
        }
        else if (!strcasecmp(name, "Selector"))
        {
            reader->Pop();
            VIGUIPage *page = p->GetPage();
            if (page)
                page->SetSelector(o);
        }
    }
}

// 0x002C5210.  A Text, a Button's or a ComboBox's text; the rest goes to
// ObjectHandler.
static void TextHandler(VIXMLEvent e)
{
    VIGUIParser *p = (VIGUIParser *)e.GetData();
    if (!p)
        return;
    VIXMLParser *reader = e.GetParser();
    if (!reader)
        return;
    const char *name = e.GetName();
    if (!name)
        return;
    VIGUIText *o = (VIGUIText *)p->GetObject();
    if (!o)
        return;
    int t = o->GetType();
    if (t != 1 && t != 4 && t != 5)
        return;
    if (e.GetType() == 0)
    {
        if (!strcasecmp(name, "TextColor"))
        {
            float r = e.GetPropInt("r", 0) / 255.0f;
            float g = e.GetPropInt("g", 0) / 255.0f;
            float b = e.GetPropInt("b", 0) / 255.0f;
            o->SetTextColor(VIColor32F(r, g, b, 1.0f));
            return;
        }
        if (!strcasecmp(name, "RollColor"))
        {
            float r = e.GetPropInt("r", 0) / 255.0f;
            float g = e.GetPropInt("g", 0) / 255.0f;
            float b = e.GetPropInt("b", 0) / 255.0f;
            o->SetRollColor(VIColor32F(r, g, b, 1.0f));
            return;
        }
        if (!strcasecmp(name, "EditLabel"))
        {
            char *title = e.GetProp("title");
            if (title)
            {
                o->SetEditLabel(title);
                free(title);
            }
        }
        else if (!strcasecmp(name, "MaxChars"))
        {
            int n = e.GetPropInt("size", -1);
            if (n != -1)
                o->SetMaxChars(n);
        }
        else if (!strcasecmp(name, "Shadow"))
        {
            char *active = e.GetProp("active");
            if (active)
            {
                if (!strcasecmp(active, "false"))
                    o->SetShadow(0);
                else
                    o->SetShadow(1);
                free(active);
            }
        }
        else if (!strcasecmp(name, "Wrap"))
        {
            char *mode = e.GetProp("mode");
            if (mode)
            {
                if (!strcasecmp(mode, "true"))
                    o->SetWrap(1);
                else
                    o->SetWrap(0);
                free(mode);
            }
        }
        else if (!strcasecmp(name, "Password"))
        {
            char *mode = e.GetProp("mode");
            if (mode)
            {
                if (!strcasecmp(mode, "true"))
                    o->SetPassword(1);
                else
                    o->SetPassword(0);
                free(mode);
            }
        }
    }
    else if (e.GetType() == 1)
    {
        if (!strcasecmp(name, "Text"))
        {
            reader->Pop();
            VIGUIPage *page = p->GetPage();
            if (!page)
                return;
            int col = o->GetButtonX();
            int row = o->GetButtonY();
            if (col == -1 || row == -1)
                page->AddObject(o);
            else
                page->AddObject(o, col, row);
        }
    }
    ObjectHandler(e);
}

// 0x002C56C8.
static void ButtonHandler(VIXMLEvent e)
{
    VIGUIParser *p = (VIGUIParser *)e.GetData();
    if (!p)
        return;
    VIXMLParser *reader = e.GetParser();
    if (!reader)
        return;
    const char *name = e.GetName();
    if (!name)
        return;
    VIGUIButton *o = (VIGUIButton *)p->GetObject();
    if (!o)
        return;
    if (o->GetType() != 4)
        return;
    if (e.GetType() == 0)
        TextHandler(e);
    else if (e.GetType() == 1)
    {
        if (strcasecmp(name, "Button"))
            return;
        reader->Pop();
        VIGUIPage *page = p->GetPage();
        if (!page)
            return;
        int col = o->GetButtonX();
        int row = o->GetButtonY();
        if (col == -1 || row == -1)
            page->AddObject(o);
        else
            page->AddObject(o, col, row);
    }
}

// 0x002C5838.  A ComboBox's options: each <Option> names one, or a numeric
// range, or namedRange="states"; namedRange is never freed.
static void OptionHandler(VIXMLEvent e)
{
    VIGUIParser *p = (VIGUIParser *)e.GetData();
    if (!p)
        return;
    VIXMLParser *reader = e.GetParser();
    if (!reader)
        return;
    const char *name = e.GetName();
    if (!name)
        return;
    VIGUIComboBox *o = (VIGUIComboBox *)p->GetObject();
    if (!o)
        return;
    if (o->GetType() != 5)
        return;
    if (e.GetType() == 0)
    {
        if (!strcasecmp(name, "Option"))
        {
            char *n = e.GetProp("name");
            int lo = e.GetPropInt("minRange", 0);
            int hi = e.GetPropInt("maxRange", 0);
            char *named = e.GetProp("namedRange");
            if (n)
            {
                o->AddItem(n);
                free(n);
            }
            if (hi != lo)
            {
                char buf[16];
                for (int i = lo; i <= hi; i++)
                {
                    sprintf(buf, "%i", i);
                    o->AddItem(buf);
                }
            }
            if (named && !strcasecmp(named, "states"))
            {
                for (int i = 0; sStates[i]; i++)
                    o->AddItem(sStates[i]);
            }
        }
        else if (!strcasecmp(name, "PopupSize"))
        {
            int w = e.GetPropInt("width", -1);
            int h = e.GetPropInt("height", 75);
            o->SetPopupSize(w, h);
        }
    }
    else if (e.GetType() == 1)
    {
        if (!strcasecmp(name, "ComboBox"))
        {
            reader->Pop();
            VIGUIPage *page = p->GetPage();
            if (!page)
                return;
            int col = o->GetButtonX();
            int row = o->GetButtonY();
            if (col == -1 || row == -1)
                page->AddObject(o);
            else
                page->AddObject(o, col, row);
            return;
        }
    }
    TextHandler(e);
}

// 0x002C5B38.
static void ToggleHandler(VIXMLEvent e)
{
    VIGUIParser *p = (VIGUIParser *)e.GetData();
    if (!p)
        return;
    VIXMLParser *reader = e.GetParser();
    if (!reader)
        return;
    const char *name = e.GetName();
    if (!name)
        return;
    VIGUIToggleButton *o = (VIGUIToggleButton *)p->GetObject();
    if (!o)
        return;
    if (o->GetType() != 2)
        return;
    if (e.GetType() == 0)
    {
        if (!strcasecmp(name, "ToggleImage"))
        {
            char *type = e.GetProp("imageType");
            if (!type)
                return;
            int id = VIGUITextureId(type);
            o->SetToggleImage(id);
            free(type);
            return;
        }
    }
    else if (e.GetType() == 1)
    {
        if (!strcasecmp(name, "Toggle"))
        {
            reader->Pop();
            VIGUIPage *page = p->GetPage();
            if (!page)
                return;
            int col = o->GetButtonX();
            int row = o->GetButtonY();
            if (col == -1 || row == -1)
                page->AddObject(o);
            else
                page->AddObject(o, col, row);
        }
    }
    ObjectHandler(e);
}

// 0x002C5D08.  "|<n>" in a page's text becomes WORDS.TXT entry n.  The
// character after the number is copied twice (once as `c`, once by the loop).
// OPEN, 27 of 92 words: the target's number loop is not rotated and its first
// digit is peeled (decomp-station-20261016.md); everything else is exact.
static const char *ExpandText(VIUI *ui, const char *s)
{
    static char sBuf[512];
    char tmp[512];
    char *p = tmp;
    char *d = sBuf;
    if (!s)
    {
        strcpy(sBuf, "(null)");
        return sBuf;
    }
    strcpy(tmp, s);
    while (*p)
    {
        char c = *p++;
        if (c == '|')
        {
            int id = 0;
            while (*p && *p >= '0' && *p <= '9')
                id = id * 10 + *p++ - '0';
            c = *p;
            if (id)
            {
                const unsigned short *w =
                    (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
                if (w)
                {
                    VIString::str16to8(d, 512 - (d - sBuf), w);
                    d += VIString::strlen16(w);
                }
            }
        }
        if (!c)
            break;
        *d++ = c;
    }
    *d = 0;
    return sBuf;
}

// 0x002C5E78.  The page's elements: each news its object and pushes the
// handler for its children.
static void ElementHandler(VIXMLEvent e)
{
    VIGUIParser *p = (VIGUIParser *)e.GetData();
    if (!p)
        return;
    VIWndGenericRenderer *owner = p->GetOwner();
    VIUI *ui = owner->UI();
    VIXMLParser *reader = e.GetParser();
    if (!reader)
        return;
    const char *name = e.GetName();
    if (!name)
        return;
    if (e.GetType() == 0)
    {
        if (!strcasecmp(name, "Text"))
        {
            char *n = e.GetProp("name");
            char *val = e.GetProp("val");
            char *align = e.GetProp("alignment");
            VIGUIText *t = new VIGUIText(n, p->GetOwner(), reader);
            if (n)
                free(n);
            if (val)
            {
                t->GetModel()->SetString(ExpandText(ui, val));
                free(val);
            }
            if (align)
            {
                if (!strcasecmp(align, "center"))
                    t->SetCenter(1);
                free(align);
            }
            p->SetObject(t);
            reader->Push(TextHandler);
        }
        else if (!strcasecmp(name, "Toggle"))
        {
            char *n = e.GetProp("name");
            VIGUIToggleButton *t = new VIGUIToggleButton(n, p->GetOwner(), reader);
            if (n)
                free(n);
            p->SetObject(t);
            reader->Push(ToggleHandler);
        }
        else if (!strcasecmp(name, "ComboBox"))
        {
            char *n = e.GetProp("name");
            char *val = e.GetProp("val");
            char *align = e.GetProp("alignment");
            VIGUIComboBox *c = new VIGUIComboBox(n, p->GetOwner(), reader);
            if (n)
                free(n);
            if (val)
            {
                c->GetModel()->SetString(ExpandText(ui, val));
                free(val);
            }
            if (align)
            {
                if (!strcasecmp(align, "center"))
                    c->SetCenter(1);
                free(align);
            }
            p->SetObject(c);
            reader->Push(OptionHandler);
        }
        else if (!strcasecmp(name, "Button"))
        {
            char *n = e.GetProp("name");
            char *val = e.GetProp("val");
            char *next = e.GetProp("nextPage");
            char *action = e.GetProp("action");
            char *align = e.GetProp("alignment");
            VIGUIButton *b = new VIGUIButton(n, p->GetOwner(), reader);
            if (n)
                free(n);
            if (val)
            {
                b->GetModel()->SetString(ExpandText(ui, val));
                free(val);
            }
            if (next)
            {
                b->SetNextPage(next);
                free(next);
            }
            if (action)
            {
                b->SetAction(action);
                free(action);
            }
            if (align)
            {
                if (!strcasecmp(align, "center"))
                    b->SetCenter(1);
                free(align);
            }
            p->SetObject(b);
            reader->Push(ButtonHandler);
        }
        else if (!strcasecmp(name, "Object") || !strcasecmp(name, "Selector"))
        {
            char *n = e.GetProp("name");
            VIGUIObject *o = new VIGUIObject(n, 3, p->GetOwner(), reader);
            if (n)
                free(n);
            p->SetObject(o);
            reader->Push(ObjectHandler);
        }
    }
    else if (e.GetType() == 1)
    {
        if (!strcasecmp(name, "Text"))
            reader->Pop();
    }
}

// 0x002C6438.  The root: news the page and pushes ElementHandler.
static void PageHandler(VIXMLEvent e)
{
    VIGUIParser *p = (VIGUIParser *)e.GetData();
    if (!p)
        return;
    VIXMLParser *reader = e.GetParser();
    if (!reader)
        return;
    const char *name = e.GetName();
    if (!name)
        return;
    if (e.GetType() == 0)
    {
        if (strcasecmp(name, "PS2GUIPage"))
            return;
        VIGUIPage *page = new VIGUIPage;
        p->SetPage(page);
        int w = e.GetPropInt("gridWidth", 0);
        int h = e.GetPropInt("gridHeight", 0);
        page->Init(p->GetOwner(), w, h);
        reader->Push(ElementHandler);
    }
    else if (e.GetType() == 1)
    {
    }
}

// 0x002C6588.  The whole file, then LoadMemory; the page is named after the path.
VIGUIPage *VIGUIParser::LoadFile(const char *path)
{
    VIFile f;
    if (!path)
        return 0;
    if (f.Open(path, 33))
        return 0;
    f.Seek(0, VIFile::Seek_End);
    int len = f.Tell();
    if (len <= 0)
        return 0;
    char *buf = (char *)malloc(len + 1);
    memset(buf, 0, len + 1);
    f.Seek(0, VIFile::Seek_Set);
    f.Read(buf, len);
    f.Close();
    VIGUIPage *page = LoadMemory(buf, len);
    free(buf);
    if (!page)
        return 0;
    page->SetName(path);
    return page;
}

// 0x002C66A8.
VIGUIPage *VIGUIParser::LoadMemory(const char *buf, int len)
{
    if (!buf || len <= 0)
        return 0;
    VIXMLParser reader;
    reader.SetData(this);
    reader.Push(PageHandler);
    reader.Parse((char *)buf, len);
    return m_page;
}

// 0x002C6740 .. 0x002C6760: defined after every use, so they are calls.
void VIGUIParser::SetPage(VIGUIPage *p)
{
    m_page = p;
}

VIGUIPage *VIGUIParser::GetPage()
{
    return m_page;
}

VIWndGenericRenderer *VIGUIParser::GetOwner()
{
    return m_owner;
}

void VIGUIParser::SetObject(VIGUIObject *o)
{
    m_object = o;
}

VIGUIObject *VIGUIParser::GetObject()
{
    return m_object;
}
