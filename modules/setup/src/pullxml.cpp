// pullxml.cpp (name ours) -- SETUP 0x00589B50 .. 0x00589F50: attribute readers
// for the patch manifest.  xmlGetProp hands back a copy, freed here.
#include "patcher.h"

// 0x00589B50.
char *PullXMLAttribute(_xmlNode *node, const char *name)
{
    return (char *)xmlGetProp(node, (const xmlChar *)name);
}

// 0x00589B70.
int PullXMLAttributeLong(_xmlNode *node, const char *name)
{
    char *s = (char *)xmlGetProp(node, (const xmlChar *)name);
    if (!s)
        return 0;
    int v = atoi(s);
    free(s);
    return v;
}

// 0x00589BC0.
float PullXMLAttributeFloat(_xmlNode *node, const char *name)
{
    char *s = (char *)xmlGetProp(node, (const xmlChar *)name);
    if (!s)
        return 0;
    float v = atof(s);
    free(s);
    return v;
}

// 0x00589C20.
bool PullXMLAttributeBool(_xmlNode *node, const char *name)
{
    bool v = false;
    char *s = (char *)xmlGetProp(node, (const xmlChar *)name);
    if (s)
    {
        if (!strcasecmp(s, "y") || !strcasecmp(s, "yes") || !strcasecmp(s, "t") ||
            !strcasecmp(s, "true"))
            v = true;
        else
            v = !strcasecmp(s, "1");
        free(s);
    }
    return v;
}

// 0x00589CD0.  Anything not recognised keeps the default.
bool PullXMLAttributeBool(_xmlNode *node, const char *name, bool def)
{
    bool v = def;
    char *s = (char *)xmlGetProp(node, (const xmlChar *)name);
    if (s)
    {
        if (!strcasecmp(s, "y") || !strcasecmp(s, "yes") || !strcasecmp(s, "t") ||
            !strcasecmp(s, "true") || !strcasecmp(s, "on") || !strcasecmp(s, "1"))
            v = true;
        else if (!strcasecmp(s, "n") || !strcasecmp(s, "no") || !strcasecmp(s, "f") ||
                 !strcasecmp(s, "false") || !strcasecmp(s, "off") || !strcasecmp(s, "0"))
            v = false;
        free(s);
    }
    return v;
}

// 0x00589E10.  2 when the attribute is absent or says neither.
int PullDefinedXMLAttBool(_xmlNode *node, const char *name)
{
    int v = 2;
    char *s = (char *)xmlGetProp(node, (const xmlChar *)name);
    if (s)
    {
        if (!strcasecmp(s, "y") || !strcasecmp(s, "yes") || !strcasecmp(s, "t") ||
            !strcasecmp(s, "true") || !strcasecmp(s, "on") || !strcasecmp(s, "1"))
            v = 1;
        else if (!strcasecmp(s, "n") || !strcasecmp(s, "no") || !strcasecmp(s, "f") ||
                 !strcasecmp(s, "false") || !strcasecmp(s, "off") || !strcasecmp(s, "0"))
            v = 0;
        free(s);
    }
    return v;
}
