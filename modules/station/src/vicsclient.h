// vicsclient.h -- the customer-service client and its records.  Two source files,
// linked into two modules, byte-identical in both: vicsclient.cpp (STATION
// 0x002DDC48 .. 0x002DF918, CLIENT 0x00BE7120 .. 0x00BE8DF0) and vicsrecords.cpp
// (STATION 0x002EA680 .. 0x002EADA0, CLIENT 0x00BFC890 .. 0x00BFCFB0).  Written by
// STATION's lane cs; CLIENT's src/vicsclient.{h,cpp} and vicsrecords.cpp are
// symlinks to STATION's.  CLIENT's VIClient embeds one VICSClient at +0x33950
// (its old placeholder VIClientPart33950); STATION's VICSInterface embeds one.
//
// VIClientI is each module's own (viclienti.h, NOT shared): STATION's build of it
// has only slot 1, GetConn, which is all this code calls; CLIENT's has 17 more
// after it.  No C library declarations here: CLIENT's game.h declares memset
// with an int size, which <string.h>'s cannot sit beside.
#ifndef VICSCLIENT_H
#define VICSCLIENT_H

#include "vidatetime.h"         // VITime, VIDate
#include "vinet.h"              // VIClientConn, VINetSend, and drdpapi.h's DRDP declarations
#include "vimemstreamvector.h"  // VIMemStreamVector.h's templates

class VIClientI;

// A wide string in a chain of writes: the same WriteString, but inside the one
// full expression, so the chain's temporaries live until the send (STATION
// 0x002DE220, 0x002DE340).  Not in CLIENT's vimemstream.h (no CLIENT function
// written so far needed it); declared here as a stand-in for the original's.
inline VIMemStream &operator<<(VIMemStream &s, const unsigned short *text)
{
    s.WriteString(text);
    return s;
}

// A raw word READ in a chain (the CS headers): explicit, like CLIENT's write-side
// VIMemStreamRaw, so an int still reads zigzag through VIMemStreamInt.  Four bytes,
// so it travels in a register and leaves no store.  Name ours.
struct VIMemStreamRawRef
{
    explicit VIMemStreamRawRef(int &v) : m_v(&v) {}
    int *m_v;
};
inline VIMemStream &operator>>(VIMemStream &s, VIMemStreamRawRef r)
{
    s.Read(r.m_v, 4, 1);
    return s;
}

// A flag WRITTEN as one byte, in a chain: the bool is held whole (a word load) and
// stored as a byte, where CLIENT's VIMemStreamByte narrows the load to an lbu.
// The byte goes through WriteByte, an inline INSIDE this inline, so this one's
// frame is 16 bytes (WriteByte's temp), not 1: SendLogin's layout needs that, and
// the record writers match either way.  Explicit, so an int still writes zigzag.
// Name ours.
struct VIMemStreamBool
{
    explicit VIMemStreamBool(bool v) : m_v(v) {}
    bool m_v;
};
inline VIMemStream &operator<<(VIMemStream &s, VIMemStreamBool v)
{
    s.WriteByte(v.m_v);
    return s;
}

// A narrow string in a chain (SendLogin): WriteString, chainable, like the wide
// one above.  Stand-in, as above.
inline VIMemStream &operator<<(VIMemStream &s, const char *text)
{
    s.WriteString(text);
    return s;
}

// A 16-bit word written in a chain (SendLogin's opcode).  Taken by CONST
// REFERENCE, so the temporary lives to the end of the full expression; and
// BLKmode (four bytes, two-byte aligned), so once freed it merges with the flag
// slot after it and takes GetConn's 8-byte result.  SendLogin needs both
// properties; the shape is measured, the name and the second half are ours.
struct VIMemStreamU16
{
    explicit VIMemStreamU16(unsigned short v) : m_v(v) {}
    unsigned short m_v;
    unsigned short m_unused;
};
inline VIMemStream &operator<<(VIMemStream &s, const VIMemStreamU16 &v)
{
    s.Write(&v.m_v, 2, 1);
    return s;
}

// ----------------------------------------------------------- the records --
// The names are lane cswnd's, from how the CS windows use them: an inbox row
// and the message it opens, and the request form's categories, each with its
// subcategories.

// An inbox row: 24 bytes, copied whole.  operator<< 0x002EA680, >> 0x002EA738.
struct VICSMessageHeader
{
    int    m_id;            // +0x00
    VIDate m_date;          // +0x04
    int    m_category;      // +0x0C
    int    m_subcategory;   // +0x10
    bool   m_unread;        // +0x14  one byte on the wire; cleared when the text arrives
};
VIMemStream &operator<<(VIMemStream &s, const VICSMessageHeader &h);
VIMemStream &operator>>(VIMemStream &s, VICSMessageHeader &h);

// The message opened: its row, then its text (8,192 characters).
struct VICSMessage : public VICSMessageHeader
{
    VICSMessage &operator=(const VICSMessageHeader &h);   // 0x002DE4A8: the row, an empty text

    unsigned short m_text[8192];   // +0x18
};

// A subcategory: 136 bytes, copied whole.  operator<< 0x002EA8B8, >> 0x002EA918.
struct VICSSubcategory
{
    int            m_id;         // +0x00  zigzag on the wire
    bool           m_x04;        // +0x04  one byte on the wire
    unsigned short m_name[64];   // +0x08
};
VIMemStream &operator<<(VIMemStream &s, const VICSSubcategory &c);
VIMemStream &operator>>(VIMemStream &s, VICSSubcategory &c);

// A category: a subcategory's three fields and its subcategories.  148 bytes;
// its copy and its destruction go through the vector.  operator<< 0x002EA7D8,
// >> 0x002EA848.
struct VICSCategory
{
    int                       m_id;         // +0x00
    bool                      m_x04;        // +0x04
    unsigned short            m_name[64];   // +0x08
    VIVector<VICSSubcategory> m_subs;       // +0x88
};
VIMemStream &operator<<(VIMemStream &s, const VICSCategory &c);
VIMemStream &operator>>(VIMemStream &s, VICSCategory &c);

// ---------------------------------------------------------- the client --
// The customer-service protocol, on message 0x0AA1 out and 0x07B0 in: a sub-op
// byte, then its fields.  Each request sets m_state to what it waits for, and
// each reply handler only acts in that state.  0x4994 bytes.  CLIENT's
// VIClient embeds one at +0x33950 (its VIClientPart33950); STATION's
// VICSInterface creates one.
class VICSClient
{
public:
    VICSClient(VIClientI *owner);                                   // 0x002DDC48
    ~VICSClient();                                                  // 0x002DDC88
    void Recv07B0(VIMemStream &s);                                  // 0x002DDCD8, CLIENT's name
    void OnConnect();                                               // 0x002DDDC0, CLIENT's name
    void Reset();                                                   // 0x002DDE40
    void RequestInbox();                                            // 0x002DDE50: sub-op 2
    void RequestCategories();                                       // 0x002DDF28: sub-op 6
    void RequestMessage(int id);                                    // 0x002DE000: sub-op 3
    void DeleteMessage(int id);                                     // 0x002DE110: sub-op 4
    void SubmitRequest(int category, int subcategory,
                       const unsigned short *text);                 // 0x002DE220: sub-op 0
    void SubmitReply(int id, const unsigned short *text);           // 0x002DE340: sub-op 1
    int  GetX08();                                                  // 0x002DE468
    int  GetCount();                                                // 0x002DE470
    VICSMessageHeader *GetHeader(int i);                            // 0x002DE478
    int  GetState();                                                // 0x002DE490
    VICSMessage *GetMessage();                                      // 0x002DE498
    VIVector<VICSCategory> *GetCategories();                        // 0x002DE4A0
    int  StatusToError(unsigned char status);                       // 0x002DE4E8
    void RecvInbox(VIMemStream &s);                                 // 0x002DE6C0
    void RecvCategories(VIMemStream &s);                            // 0x002DE7E0
    void RecvMessage(VIMemStream &s);                               // 0x002DE850
    void RecvSubmitRequest(VIMemStream &s);                         // 0x002DE8E0
    void RecvSubmitReply(VIMemStream &s);                           // 0x002DE930
    void RecvDelete(VIMemStream &s);                                // 0x002DE990
    void RemoveEntry(int id);                                       // 0x002DEA08
    int  Find(int id);                                              // 0x002DEAA0

    VIClientI              *m_owner;        // +0x0000
    int                     m_state;        // +0x0004  the reply waited for; <0 an error
    int                     m_x08;          // +0x0008  set by sub-op 5, cleared by an inbox
    int                     m_count;        // +0x000C  -1 until the first inbox
    VICSMessageHeader       m_headers[100]; // +0x0010
    VICSMessage             m_message;      // +0x0970
    VIVector<VICSCategory>  m_categories;   // +0x4988
};
typedef char c_VICSClient_size[(sizeof(VICSClient) == 0x4994) ? 1 : -1];

#endif
