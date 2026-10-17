// vixmlparser.cpp (name ours) -- STATION 0x002F4EF0 .. 0x002F54F0, just in front
// of libxml: a SAX-like walk over a libxml tree that the PS2GUIPage parser
// drives, then this TU's copies of VIList<VIXMLCallback>.
#include "station.h"
#include "vilist_t.h"

// 0x002F4EF0.
VIXMLEvent::VIXMLEvent(VIXMLParser *parser, int type, void *data, xmlNode *node,
                       const char *text)
{
    m_parser = parser;
    SetType(type);
    m_data = data;
    m_node = node;
    m_text = text;
}

// 0x002F4F50.
VIXMLParser *VIXMLEvent::GetParser()
{
    return m_parser;
}

// 0x002F4F58.
void VIXMLEvent::SetType(int type)
{
    m_type = type;
}

// 0x002F4F60.
int VIXMLEvent::GetType()
{
    return m_type;
}

// 0x002F4F68.
void *VIXMLEvent::GetData()
{
    return m_data;
}

// 0x002F4F70.
const char *VIXMLEvent::GetName()
{
    if (!m_node)
        return 0;
    return (const char *)m_node->name;
}

// 0x002F4F90.
void VIXMLEvent::SetText(const char *s)
{
    m_text = s;
}

// 0x002F4F98.
const char *VIXMLEvent::GetText()
{
    return m_text;
}

// 0x002F4FA0.  The caller frees the copy.
char *VIXMLEvent::GetProp(const char *name)
{
    if (!m_node || !name)
        return 0;
    return (char *)xmlGetProp(m_node, (const xmlChar *)name);
}

// 0x002F4FD0.
int VIXMLEvent::GetPropInt(const char *name, int def)
{
    char *s = GetProp(name);
    if (s)
    {
        def = atoi(s);
        free(s);
    }
    return def;
}

// 0x002F5020.
VIXMLParser::VIXMLParser()
{
    m_data = 0;
    m_callback = 0;
}

// 0x002F5050.  Depth first: the start event, the children, the end event.
void VIXMLParser::Walk(xmlNode *node)
{
    if (!m_callback)
        return;
    xmlNode *child = node->children;
    VIXMLEvent ev(this, 0, m_data, node, 0);
    m_callback(ev);
    for (; child; child = child->next)
    {
        if (child->name)
        {
            if (strcmp((const char *)child->name, "text"))
                Walk(child);
            else
                ev.SetText((const char *)child->content);
        }
    }
    ev.SetType(1);
    m_callback(ev);
}

// 0x002F5118.
int VIXMLParser::Parse(char *buf, int len)
{
    xmlDoc *doc = xmlParseMemory(buf, len);
    if (!doc)
        return 1;
    xmlNode *root = xmlDocGetRootElement(doc);
    if (root)
        Walk(root);
    xmlFreeDoc(doc);
    return 0;
}

// 0x002F5180.
void VIXMLParser::Push(VIXMLCallback cb)
{
    if (cb)
    {
        m_stack.Insert(cb, m_stack.Begin());
        m_callback = cb;
    }
}

// 0x002F51C8.
void VIXMLParser::Pop()
{
    if (m_stack.Size())
    {
        int i = m_stack.Erase(m_stack.Begin());
        if (i == m_stack.End())
            m_callback = 0;
        else
            m_callback = ((VIList<VIXMLCallback>::Node *)i)->m_data;
    }
}

// 0x002F5230.
void VIXMLParser::SetData(void *data)
{
    m_data = data;
}

// 0x002F5238.
void *VIXMLParser::GetData()
{
    return m_data;
}

// VIList<T>::Insert is declared in vi.h and defined in src/vilist_t.h: v
// goes in front of node i, or at the end when i is End().




















// 0x002F5240 ..  This TU's copies (the first Clear and the destructor's callee
// are the page parser's, 0x002C7328).
template VIList<VIXMLCallback>::VIList();
template int VIList<VIXMLCallback>::Clear();
template VIList<VIXMLCallback>::~VIList();
template int VIList<VIXMLCallback>::End() const;
template int VIList<VIXMLCallback>::Begin() const;
template int VIList<VIXMLCallback>::Add(const VIXMLCallback &);
template int VIList<VIXMLCallback>::Insert(const VIXMLCallback &, int);
template int VIList<VIXMLCallback>::Size() const;
template int VIList<VIXMLCallback>::Erase(int);
