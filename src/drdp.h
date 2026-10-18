// The DRDP network library, recovered by matching.  SUPPORT 0x004acb50..0x004c1ca8
// is one C library compiled as C++ -- every name mangles (`__FP8buffer_t...`),
// nothing has a vtable, and the only classes are `struct`s of plain fields.
//
// This is the layer the whole game protocol rides on: `buffer_t` is the wire
// buffer every segment is read out of and appended into, so its packed and
// offset codecs ARE the encodings `claude-notes/protocol-reference.md` calls
// zigzag and 7-bit.
#ifndef DRDP_H
#define DRDP_H

#include "viport.h"   // VI_PORT: the native build's switches

// The mangled names settle every one of these: buffer_read_int8 takes `PSc`
// (signed char *) where buffer_read_string takes `Pc` (plain char *), so the
// three char types are distinct and int8 is `signed char`, not `char`.
// `Ul` is `unsigned long`, and phase 23 measured `long` at EIGHT bytes on this
// ABI, so int64/uns64 are long -- not long long, which is SIXTEEN.
typedef signed char    int8;
typedef unsigned char  uns8;
typedef short          int16;
typedef unsigned short uns16;
typedef int            int32;
typedef unsigned int   uns32;
typedef long           int64;
typedef unsigned long  uns64;

extern "C" void *memcpy(void *, const void *, unsigned int);
extern "C" void *malloc(unsigned int);
extern "C" void free(void *);
// The library's own assert.  Its THIRD argument is __LINE__, an immediate that
// has to match, so every file carrying one is padded to the original's line
// layout -- phase 27's lever.  The two strings are relocation sites the differ
// masks, and `eqcalls.py` reads them back out of the relocated dump: they say
// the original tree was `C:/code-frontiers-gm/drdp/`.
extern "C" void _uassert(const char *expr, const char *file, int line);

#define VASSERT(x) if (!(x)) _uassert(#x, __FILE__, __LINE__)

// Amiga Exec's list, a second time -- phase 11 found VILList to be the same
// shape.  The header's three words overlap two nodes, so `lh_head` and
// `lh_tailpred` both point at real memory and a walk in either direction ends
// on a null the header itself provides: ll_init writes `&lh_tail` into
// lh_head, `list` into lh_tailpred, and zero into lh_tail.
struct lnode
{
    lnode *ln_succ;
    lnode *ln_pred;
};

struct llist
{
    lnode *lh_head;
    lnode *lh_tail;
    lnode *lh_tailpred;
};

void   ll_init(llist *list);
void   ll_add_head(llist *list, lnode *node);
void   ll_add_tail(llist *list, lnode *node);
void   ll_insert_after(lnode *node, lnode *after);
void   ll_remove(lnode *node);
lnode *ll_rem_head(llist *list);
lnode *ll_rem_tail(llist *list);

// +0x00 data  +0x04 max_size  +0x08 size  +0x0C overflow  +0x10 read_only
// +0x14 swap.  Read off buffer_init (which stores all six) and confirmed by
// buffer_bytes_remaining, which returns -1 when +0x0C is set.  +0x08 -- the
// write cursor, what buffer_get_position returns -- is `size` in the original:
// its own assert reads `buffer.size <= connection_get_max_message_size(...)`
// and compiles to a load of +0x08.  `max_size` for the capacity at +0x04 is
// OURS; no string in the binary names it.
struct buffer_t
{
    uns8 *data;
    int   max_size;
    int   size;
    int   overflow;
    int   read_only;
    int   swap;
};

// buffer_init's flag word: bit 0 swaps when the HOST is big-endian and bit 1
// swaps when it is little-endian, so bit 0 means "this buffer is little-endian
// data" and bit 1 "big-endian data".  The EE is little-endian, so a game
// buffer opened with bit 1 byte-swaps every read.
enum
{
    BUFFER_LITTLE_ENDIAN = 1,
    BUFFER_BIG_ENDIAN    = 2
};

int  little_endian();

void buffer_init(buffer_t *buffer, void *data, int size, int flags);
void buffer_init_read(buffer_t *buffer, const void *data, int size, int flags);
int  buffer_bytes_remaining(buffer_t *buffer);
void buffer_skip(buffer_t *buffer, int size);
int  buffer_get_position(buffer_t *buffer);
void buffer_set_position(buffer_t *buffer, int position);
void buffer_swap(buffer_t *buffer, int flags);
void buffer_copy(buffer_t *to, buffer_t *from, int size);
void *buffer_get_position_ptr(buffer_t *buffer);

#define BUFFER_DECL(NAME, TYPE)                                              \
    void buffer_read_##NAME(buffer_t *buffer, TYPE *value);                  \
    void buffer_readm_##NAME(buffer_t *buffer, TYPE *value, int count);      \
    void buffer_append_##NAME(buffer_t *buffer, TYPE value);                 \
    void buffer_appendm_##NAME(buffer_t *buffer, const TYPE *value, int count);

BUFFER_DECL(int8,   int8)
BUFFER_DECL(uns8,   uns8)
BUFFER_DECL(int16,  int16)
BUFFER_DECL(uns16,  uns16)
BUFFER_DECL(int32,  int32)
BUFFER_DECL(uns32,  uns32)
BUFFER_DECL(int64,  int64)
BUFFER_DECL(uns64,  uns64)
BUFFER_DECL(float,  float)
BUFFER_DECL(double, double)

void buffer_append_packed_uns64(buffer_t *buffer, uns64 value);
void buffer_read_packed_uns64(buffer_t *buffer, uns64 *value);
void buffer_append_packed_int16(buffer_t *buffer, int16 value);
void buffer_read_packed_int16(buffer_t *buffer, int16 *value);
void buffer_append_packed_int32(buffer_t *buffer, int32 value);
void buffer_read_packed_int32(buffer_t *buffer, int32 *value);
void buffer_append_packed_int64(buffer_t *buffer, int64 value);
void buffer_read_packed_int64(buffer_t *buffer, int64 *value);
void buffer_append_packed_uns16(buffer_t *buffer, uns16 value);
void buffer_read_packed_uns16(buffer_t *buffer, uns16 *value);
void buffer_append_packed_uns32(buffer_t *buffer, uns32 value);
void buffer_read_packed_uns32(buffer_t *buffer, uns32 *value);

#define BUFFER_OFFSET_DECL(NAME, TYPE)                                       \
    void buffer_read_offset_##NAME(buffer_t *buffer, TYPE *value, TYPE offset); \
    void buffer_append_offset_##NAME(buffer_t *buffer, TYPE value, TYPE offset);

BUFFER_OFFSET_DECL(int8,  int8)
BUFFER_OFFSET_DECL(uns8,  uns8)
BUFFER_OFFSET_DECL(int16, int16)
BUFFER_OFFSET_DECL(uns16, uns16)
BUFFER_OFFSET_DECL(int32, int32)
BUFFER_OFFSET_DECL(uns32, uns32)
BUFFER_OFFSET_DECL(int64, int64)
BUFFER_OFFSET_DECL(uns64, uns64)

// The generic containers.  A vlist/vset/vtree holds no memory of its own: the
// caller's object carries the link node, and the container is told at init
// where that node sits inside it and where the sort key does.  `vlist_init`
// stores the KEY offset as a delta from the node's, so a key is reached from a
// node without going back through the object.
// The TREE's node, and the mangling of vtree_validate_node(vtree_t *, vnode_t *)
// is what names it.  The children are an ARRAY indexed by direction, which is
// why every walk, rotate and fixup in vtree.cpp is written once and reached
// with `child[dir]` / `child[dir ^ 1]`.
//
// The LIST's node is modelled as the same type with the first two words
// unioned.  Nothing here settles that: no vlist function names a node in its
// mangled signature, and the rest of the library -- close_set, connection,
// route and rce each expose BOTH a `get_vnode_offset` and a
// `get_vlink_offset` -- hints the original had two types.  The choice does not
// reach the code generator for anything matched in this phase.
struct vnode_t
{
    union
    {
        vnode_t *next;
        int      colour;
    };
    union
    {
        vnode_t *prev;
        vnode_t *parent;
    };
    vnode_t *child[2];
};

typedef int (*vcmp_t)(const void *, const void *);

struct vlist_t
{
    vnode_t *head;
    vnode_t *tail;
    int      size;
    int      vnode_offset;
    vcmp_t   cmp;
    int      key_offset;
};

void  vlist_init(vlist_t *list, int vnode_offset, vcmp_t cmp, int key_offset);
void  vlist_add_head(vlist_t *list, void *object);
void  vlist_add_tail(vlist_t *list, void *object);
void  vlist_replace(vlist_t *list, void *object, void *with);
void *vlist_remove_head(vlist_t *list);
void *vlist_remove_tail(vlist_t *list);
void  vlist_insert(vlist_t *list, void *object);
int   vlist_contains(vlist_t *list, void *object);
void *vlist_remove(vlist_t *list, void *object);
void *vlist_remove_by_key(vlist_t *list, void *key);
void *vlist_lookup(const vlist_t *list, void *key);
int   vlist_get_size(vlist_t *list);
void *vlist_peek_head(vlist_t *list);
void *vlist_peek_tail(vlist_t *list);
void *vlist_get_next(vlist_t *list, void *object);
void *vlist_get_prev(vlist_t *list, void *object);

// The red-black tree the DRDP library sorts everything with, and the SET that
// copies its elements into the heap on top of it.  A tree node is sixteen
// bytes and the tree carries its own NIL sentinel INSIDE itself, at +0x14:
// vtree_init points root, and the sentinel's own three links, at that address.
struct vtree_t
{
    vnode_t *root;
    int      size;
    int      vnode_offset;
    vcmp_t   cmp;
    int      key_offset;
    vnode_t  nil;
};

void  vtree_init(vtree_t *tree, int vnode_offset, vcmp_t cmp, int key_offset);
int   vtree_get_size(vtree_t *tree);
int   vtree_validate_node(vtree_t *tree, vnode_t *node);
int   vtree_get_bh(vtree_t *tree, vnode_t *node);
int   vtree_validate(vtree_t *tree);
int   vtree_validate_bh(vtree_t *tree);
void  vtree_insert_raw(vtree_t *tree, void *object);
void  vtree_replace_x_with_y(vtree_t *tree, vnode_t *x, vnode_t *y);
void  vtree_rotate(vtree_t *tree, vnode_t *x, int dir);
void  vtree_insert(vtree_t *tree, void *object);
int   vtree_contains(vtree_t *tree, void *object);
void *vtree_iterate(vtree_t *tree, void *object, int dir);
void *vtree_remove(vtree_t *tree, void *object);
void *vtree_remove_by_key(vtree_t *tree, void *key);
void *vtree_lookup(const vtree_t *tree, const void *key);
void *vtree_peek_end(vtree_t *tree, int dir);
void *vtree_peek_first(vtree_t *tree);
void *vtree_peek_last(vtree_t *tree);
void *vtree_remove_end(vtree_t *tree, int dir);
void *vtree_remove_first(vtree_t *tree);
void *vtree_remove_last(vtree_t *tree);
void *vtree_get_prev(vtree_t *tree, void *object);
void *vtree_get_next(vtree_t *tree, void *object);
int   vtree_vnode_range_cmp(vtree_t *tree, void *lo, void *hi, vnode_t *node);
void *vtree_iterate_in_range(vtree_t *tree, void *lo, void *hi, void *object, int dir);
void *vtree_peek_end_in_range(vtree_t *tree, void *lo, void *hi, int dir);
void *vtree_peek_first_in_range(vtree_t *tree, void *lo, void *hi);
void *vtree_peek_last_in_range(vtree_t *tree, void *lo, void *hi);
void *vtree_get_prev_in_range(vtree_t *tree, void *lo, void *hi, void *object);
void *vtree_get_next_in_range(vtree_t *tree, void *lo, void *hi, void *object);
void  vtree_vnode_free(vtree_t *tree, vnode_t *node);
void  vtree_free(vtree_t *tree);

// A set OWNS its elements: `vnode_offset` doubles as the element size, so the
// allocation is size + 16 and the node lands just past the copy.
struct vset_t
{
    vtree_t tree;
};

void vset_init(vset_t *set, vcmp_t cmp, int size);
int  vset_get_size(vset_t *set);
int  vset_contains(vset_t *set, const void *element);
int  vset_add(vset_t *set, const void *element);
int  vset_remove(vset_t *set, const void *element);
void vset_remove_all(vset_t *set);
int  vset_remove_first(vset_t *set, void *element);

// The state channel's compressor: alternating runs of zero and non-zero bytes,
// one byte per pair when both are small (`nonzeros << 4 | zeros`) and two when
// they are not (`0x80 | nonzeros`, then `zeros`).  A zero header byte ends the
// stream, which is why the short form is the one that can terminate.
void run_length_encode(const uns8 *data, uns32 size, buffer_t *buffer);
void run_length_decode(buffer_t *buffer, uns8 *data, uns32 size);

void buffer_append_string(buffer_t *buffer, const char *string);
void buffer_read_string(buffer_t *buffer, char *string, int size);

// ---------------------------------------------------------------------------
// bandwidth.cpp -- SUPPORT 0x004aeb80..0x004aef90.  A leaky bucket over the
// send queue plus a one-second sample window for the two bps counters.  Its
// asserts NAME six of the fields (`pBandwidth->m_iBytesSentSinceLastSample`),
// which is where the Hungarian on this struct comes from.
#define DRDP_INT64_MAX 0x7fffffffffffffff
#define DRDP_INT_MAX   0x7fffffff

struct bandwidth_t
{
    float m_fBpsLimit;              // +0x00
    float m_fQueueSize;             // +0x04  bytes still owed to the wire
    float m_fQueueSizeMax;          // +0x08
    uns32 m_uiLastProcessTime;      // +0x0C
    int   m_iBpsSent;               // +0x10
    int   m_iBpsRecv;               // +0x14
    uns32 m_uiLastSampleTime;       // +0x18
    int64 m_iBytesSentSinceLastSample;  // +0x20
    int64 m_iBytesRecvSinceLastSample;  // +0x28
};

void  bandwidth_init(bandwidth_t *pBandwidth, int iBpsLimit, int iQueueSizeMax);
void  bandwidth_process(bandwidth_t *pBandwidth, uns32 uiTime);
void  bandwidth_send(bandwidth_t *pBandwidth, uns32 uiTime, int iSize);
void  bandwidth_recv(bandwidth_t *pBandwidth, uns32 uiTime, int iSize);
uns32 bandwidth_get_flush_time(bandwidth_t *pBandwidth, uns32 uiTime);
void  bandwidth_set_queue_size_max(bandwidth_t *pBandwidth, int iQueueSizeMax);
int   bandwidth_get_queue_size_max(bandwidth_t *pBandwidth);
float bandwidth_get_process_interval_max(bandwidth_t *pBandwidth);
void  bandwidth_set_bps_limit(bandwidth_t *pBandwidth, uns32 uiTime, int iBpsLimit);
int   bandwidth_get_bps_limit(bandwidth_t *pBandwidth);
int   bandwidth_get_bps_sent(bandwidth_t *pBandwidth);
int   bandwidth_get_bps_recv(bandwidth_t *pBandwidth);

// ---------------------------------------------------------------------------
// The two generic comparators the library sorts times and endpoint ids with.
// `uns32_cmp_wrap` subtracts and looks at the SIGN, so it orders a wrapping
// millisecond clock correctly where a plain `<` would not.
int uns32_cmp_wrap(const uns32 *a, const uns32 *b);
int uns64_cmp(const uns64 *a, const uns64 *b);

// ---------------------------------------------------------------------------
// close_set.cpp -- SUPPORT 0x004aef90..0x004af420.  The set of recently closed
// connections, so a late segment for one of them is answered with a reset
// rather than re-opening it.  Every record is on BOTH containers at once: a
// tree keyed by (endpoint id, instance) for the lookup, and a list keyed by the
// expiry time for the sweep -- which is why `close_get_vnode_offset` and
// `close_get_vlink_offset` are two different functions.
struct close_key_t
{
    uns64 id;
    uns32 instance;
};

struct close_t
{
    vnode_t     vnode;      // +0x00
    lnode       vlink;      // +0x10
    close_key_t key;        // +0x18
    uns32       time;       // +0x28  when this record expires
    const char *file;       // +0x2C  who closed it
    int         line;       // +0x30
};

struct close_set_t
{
    vtree_t tree;           // +0x00
    vlist_t list;           // +0x24
};

int  close_key_t_cmp(const close_key_t *key1, const close_key_t *key2);
int  close_get_vnode_offset(int iWhich);
int  close_get_vlink_offset(int iWhich);
int  close_get_key_offset(int iWhich);
void close_set_init(close_set_t *pSet);
int  close_set_insert(close_set_t *pSet, uns64 uiId, uns32 uiInstance, uns32 uiTime,
                      uns32 uiNow, const char *pFile, int iLine);
void close_set_release_expired(close_set_t *pSet, uns32 uiNow);
int  close_set_lookup(close_set_t *pSet, uns64 uiId, uns32 uiInstance);
void close_set_destroy_buffer(close_set_t *pSet, buffer_t *pBuffer, uns32 uiNow);
int  close_set_create_buffer(close_set_t *pSet, buffer_t *pBuffer, uns32 uiNow);

// The formatter the close set and the route both call for their trace lines:
// it writes into one of four static rings and returns it.
const char *drdp_format_endpoint1(uns64 uiId);

// ---------------------------------------------------------------------------
// message.cpp -- SUPPORT 0x004bad40..0x004bb370.  One outgoing or arrived
// application message.  The header is 72 bytes and the payload is the malloc
// slack straight after it, which is all `message_get_data` returns: `this + 1`.
// The record opens with a four-byte tag, `"MESS"` -- `message_is_valid` memcmps
// it, and `destroy` overwrites it with `"NULL"` before the free, so a message
// used after being destroyed fails its own validity test rather than passing.
struct connection_t;
struct drdp_t;
struct route_t;

// ---------------------------------------------------------------------------
// endpoint.cpp -- SUPPORT 0x004b9218..0x004bab40.  One addressable party: an
// id, the connections open to it, and the messages that have arrived for them.
// 176 bytes, straight off `malloc` in `endpoint_create`, and tagged "ENDP" the
// same way a message is.
struct endpoint_t
{
    char        m_szTag[4];     // +0x00  "ENDP"
    vnode_t     m_vnode;        // +0x04  the drdp's tree of endpoints
    drdp_t     *m_pDrdp;        // +0x14
    void       *m_pAppPtr;      // +0x18  the application's, never read here
    uns64       m_uiId;         // +0x20  the tree's key
    uns32       m_uiInstance;   // +0x28  seeded from the clock, then handed
                                //        out one at a time by `get_instance`
    int         m_iFlags;       // +0x2C  bit 0 closed, bit 1 in sync (STICKY)
    vtree_t     m_connections;  // +0x30
    vlist_t     m_arrivals;     // +0x54
    close_set_t m_closeSet;     // +0x6C
    uns32       m_uiA8;         // +0xA8
    uns32       m_uiAC;         // +0xAC  a second clock reading from create
};                              //  0xB0

struct message_t
{
    char          m_szTag[4];       // +0x00  "MESS", then "NULL" once freed
    lnode         m_link[2];        // +0x04  it is on TWO lists at once
    endpoint_t   *m_pEndpoint;      // +0x14
    connection_t *m_pConnection;    // +0x18
    uns64         m_uiSrc;          // +0x20
    uns64         m_uiDst;          // +0x28
    uns32         m_uiSize;         // +0x30  of the payload, not of this
    uns16         m_usSeqnum;       // +0x34
    uns8          m_ucType;         // +0x36
    uns8          m_ucRefnumOffset; // +0x37
    uns16         m_usFlush;        // +0x38
    uns32         m_uiCreated;      // +0x3C
    uns32         m_uiTime;         // +0x40
};                                  //  0x48

// The two container heads a message hangs off.  Only their offset-zero vlist is
// reached from this TU; the rest of each is settled in connection.cpp.
struct message_list_t
{
    vlist_t list;
};

struct message_seqnum_t
{
    vlist_t list;
};

int   message_create(message_t **ppMessage, endpoint_t *pEndpoint, connection_t *pConnection,
                     uns64 uiSrc, uns64 uiDst, const void *pData, uns32 uiSize,
                     uns8 ucType, uns16 usSeqnum, uns32 uiTime);
int   message_duplicate(message_t **ppMessage, message_t *pMessage);
void  message_destroy_buffer(message_t *pMessage, buffer_t *pBuffer);
int   message_create_buffer(message_t **ppMessage, endpoint_t *pEndpoint,
                            connection_t *pConnection, buffer_t *pBuffer);
void  message_destroy(message_t *pMessage);
uns8  message_get_type(const message_t *pMessage);
void *message_get_data(message_t *pMessage);
uns32 message_get_size(const message_t *pMessage);
uns16 message_get_seqnum(const message_t *pMessage);
void  message_set_seqnum(message_t *pMessage, uns16 usSeqnum);
uns32 message_get_time(const message_t *pMessage);
uns32 message_get_created(const message_t *pMessage);
void  message_set_time(message_t *pMessage, uns32 uiTime);
uns8  message_get_refnum_offset(const message_t *pMessage);
void  message_set_refnum_offset(message_t *pMessage, uns8 ucOffset);
uns16 message_get_flush(const message_t *pMessage);
void  message_set_flush(message_t *pMessage, uns16 usFlush);
endpoint_t   *message_get_endpoint(message_t *pMessage);
connection_t *message_get_connection(message_t *pMessage);
void  message_set_connection(message_t *pMessage, connection_t *pConnection);
uns64 message_get_src(const message_t *pMessage);
uns64 message_get_dst(const message_t *pMessage);
int   message_is_valid(const message_t *pMessage);
int   message_get_vlink_offset(int iWhich);
int   message_get_key_offset(int iWhich);
int   message_list_contains_type_seqnum(message_list_t *pList, uns8 ucType, uns16 usSeqnum);
void  message_seqnum_remove_thru(message_seqnum_t *pSeqnum, uns16 usSeqnum);

// ---------------------------------------------------------------------------
// connection.cpp -- SUPPORT 0x004af418..0x004b4aa8.  ONE conversation between
// two endpoints, and the biggest struct in the library at 4,576 bytes.  The
// asserts name six of its fields outright (`connection->local_id`,
// `->remote_id`, `->endpoint`, `->app_info`, `->time_last_arrival`, and
// `state_channel->rx`), which is where the naming below comes from.
//
// A connection is on THREE containers at once, so it carries three vnodes and
// `connection_get_vnode_offset` is `index * 16 + 4`: the endpoint's tree of
// connections keyed by the far id (key 0, +80), the drdp's, and the ROUTE's
// tree keyed by the desired flush time (key 1, +180) -- which is the one
// `route_flush` walks in order.

// One of 248 state channels.  A state channel is a slot the application
// overwrites: `connection_send_state` compares the new bytes against the
// message already pending and sends nothing when they are equal, and when they
// are not it REPLACES the pending one wherever it is -- `m_ucWhere` says which
// container that is.
struct state_channel_t
{
    message_t       *m_pPending;    // +0x00  the message not yet on the wire
    uns16            m_usTx;        // +0x04  next sequence number to send
    uns16            m_usTxAck;     // +0x06  the far end has this one
    uns16            rx;            // +0x08  next sequence number expected
    uns8             m_ucRxFlag;    // +0x0A
    uns8             m_ucWhere;     // +0x0B  0 nowhere, 1 send queue, 2 outstanding
    message_seqnum_t m_txList;      // +0x0C  sent, by sequence number
    message_seqnum_t m_rxList;      // +0x24  arrived, by sequence number
};                              //  0x3C

void state_channel_init(state_channel_t *pChannel);

// One acknowledgement of one state channel.  The array is terminated by a
// channel index of 248, which is why there are 249 slots for 248 channels.
namespace drdp { struct logger_t; }

struct state_ack_t
{
    uns8  m_ucChannel;      // +0x00  248 ends the array
    uns8  m_ucPad;          // +0x01
    uns16 m_usSeqnum;       // +0x02
};

// The parsed segment.  It is 1,340 bytes and `connection_handle_segment` puts
// one on the STACK, which is why that function's frame is the largest in the
// library.
struct segment_body_t
{
    uns8    m_ucFlags;              // +0x000
    uns8    m_ucPad;                // +0x001
    uns16   m_usConnFlags;          // +0x002  OR'd straight into the connection
    uns8    m_ucChannels[248];      // +0x004  which channels arrived
    int     m_iChannelCount;        // +0x0FC
    uns32   m_uiInstance;           // +0x100  the sender's idea of OUR instance
    int     m_iArrival;             // +0x104
    int     m_iGuaranteedAck;       // +0x108
    int     m_iAcks;                // +0x10C
    uns16   m_usArrivalNumber;      // +0x110
    uns16   m_usFlush;              // +0x112
    uns64   m_uiFlushMask;          // +0x118
    uns16   m_usSeqnum;             // +0x120
    uns64   m_uiMask;               // +0x128
    state_ack_t m_acks[249];        // +0x130
    message_list_t m_messages;      // +0x514
};

// The wire header every segment opens with.  Only the first word is always
// present: it carries the payload size in its low eleven bits and a bit per
// optional field, so a segment between two known endpoints on a known route is
// one packed word and nothing else.
struct segment_header_t
{
    uns32 flags;            // +0x00
    uns32 m_uiSize;         // +0x04  flags & 0x7ff
    uns32 m_uiCrc;          // +0x08  flags & 0x2000
    uns32 m_uiFlushNumber;  // +0x0C  flags & 0x40000
    uns64 m_uiRoute;        // +0x10  flags & 0x8000, else the caller's default
    uns64 src;              // +0x18  flags & 0x1000 means "same as the route"
    uns64 dst;              // +0x20  flags & 0x800, else the caller's default
};                          //  0x28

struct connection_t
{
    char        m_szTag[4];         // +0x0000  "CONN"
    vnode_t     m_vnode[3];         // +0x0004  endpoint, drdp, route
    endpoint_t *endpoint;           // +0x0034
    route_t    *m_pRoute;           // +0x0038
    void       *m_pAppPtr;          // +0x003C
    char       *app_info;           // +0x0040  strdup'd
    uns64       local_id;           // +0x0048
    uns64       remote_id;          // +0x0050  key 0
    char        m_szName[64];       // +0x0058  drdp_format_endpoint's
    uns32       m_uiInstanceLocal;  // +0x0098
    uns32       m_uiInstanceRemote; // +0x009C
    uns32       m_uiTimeInstanceSent;   // +0x00A0
    uns32       time_last_flush;         // +0x00A4
    uns32       time_last_arrival;      // +0x00A8
    uns32       m_uiTimeLastInstanceArrival;  // +0x00AC
    uns32       m_uiTimeSend;       // +0x00B0
    uns32       m_uiDesiredFlushTime;   // +0x00B4  key 1
    uns32       m_uiFlushBy;        // +0x00B8  the record deadline
    message_list_t   m_sendQueue;   // +0x00BC  waiting for a flush
    message_list_t   m_outstanding; // +0x00D4  on the wire, unacknowledged
    message_seqnum_t m_arrivals;    // +0x00EC  by sequence number
    message_seqnum_t m_delayed;     // +0x0104  by sequence number
    message_t  *m_pTerminate;       // +0x011C  pre-allocated, delivered once
    uns32       m_uiFlushTime[1024];// +0x0120  when flush number N went out
    uns16       m_usFlags;          // +0x1120
    uns8        m_ucRttIndex;       // +0x1122
    uns8        m_ucRxFlag;         // +0x1123
    uns16       m_usTimeout;        // +0x1124  the LARGEST of the sixteen
    uns16       m_usRtt[16];        // +0x1126
    uns16       m_usFlush;          // +0x1146  the next flush number
    uns16       m_usArrivalNumber;  // +0x1148
    uns16       m_usFlushAck;       // +0x114A  the highest one acknowledged
    uns64       m_uiRxMask;         // +0x1150
    uns64       m_uiArrivalHistory; // +0x1158  all ones, so nothing looks late
    uns16       m_usTxSeqnum;       // +0x1160
    uns16       m_usRxSeqnum;       // +0x1162  the next guaranteed one expected
    uns16       m_usTxAck;          // +0x1164
    uns64       m_uiTxMask;         // +0x1168
    int         m_iRetryTime;       // +0x1170  5,000
    int         m_iWarnTime;        // +0x1174  30,000
    int         m_iFailTime;        // +0x1178  60,000
    int         m_iLastRouteFlushNumber;  // +0x117C  -1
    uns64       m_uiCstat[11];      // +0x1180
    state_channel_t *m_pChannels;   // +0x11D8  248 of them, lazily malloc'd
    drdp::logger_t *m_pLogger;      // +0x11DC
};                                  //  0x11E0

int   endpoint_get_vnode_offset(int iWhich);
int   endpoint_get_key_offset(int iWhich);
int   endpoint_create(endpoint_t **ppEndpoint, drdp_t *pDrdp, uns64 uiId);
void  endpoint_arrival_enqueue(endpoint_t *pEndpoint, message_t *pMessage);
void  endpoint_arrival_remove(endpoint_t *pEndpoint, message_t *pMessage);
void  endpoint_connection_insert(endpoint_t *pEndpoint, connection_t *pConnection);
void  endpoint_connection_remove(connection_t *pConnection);
uns32 endpoint_get_instance(endpoint_t *pEndpoint);
uns32 endpoint_get_time(endpoint_t *pEndpoint);
uns64 endpoint_get_id(const endpoint_t *pEndpoint);
int   endpoint_connection_get_arrival_count(connection_t *pConnection);
int   endpoint_is_valid(const endpoint_t *pEndpoint);
int   endpoint_has_connection(const endpoint_t *pEndpoint, const connection_t *pConnection);
drdp_t *endpoint_get_drdp(endpoint_t *pEndpoint);
void  endpoint_free_arrivals(endpoint_t *pEndpoint, buffer_t *pBuffer);
void  endpoint_destroy_buffer(endpoint_t *pEndpoint, buffer_t *pBuffer);
void  endpoint_destroy(endpoint_t *pEndpoint);
void *endpoint_get_app_ptr(const endpoint_t *pEndpoint);
void  endpoint_set_app_ptr(endpoint_t *pEndpoint, void *pAppPtr);
connection_t *endpoint_find_connection(endpoint_t *pEndpoint, uns64 uiDst);
int   endpoint_open(endpoint_t **ppEndpoint, drdp_t *pDrdp);
int   endpoint_is_closed(endpoint_t *pEndpoint);
int   endpoint_is_in_sync_internal(endpoint_t *pEndpoint);
int   endpoint_is_in_sync(endpoint_t *pEndpoint);

int   connection_get_vnode_offset(int iIndex);
int   connection_get_key_offset(int iIndex);
int   connection_is_valid(const connection_t *pConnection);
int   connection_allocate_state_channels(connection_t *pConnection);
int   connection_create(connection_t **ppConnection, endpoint_t *pEndpoint, uns64 uiDst,
                        uns32 uiInstance, int iFlags);
void  connection_free_outstanding(connection_t *pConnection);
void  connection_destroy(connection_t *pConnection);
void  connection_destroy_buffer(connection_t *pConnection, buffer_t *pBuffer);
int   connection_create_buffer(connection_t **ppConnection, endpoint_t *pEndpoint,
                               buffer_t *pBuffer);
int   connection_send_best_effort(connection_t *pConnection, const uns8 *pData, int iSize);
int   connection_send_guaranteed(connection_t *pConnection, const uns8 *pData, int iSize,
                                 int iType);
int   connection_send_state(connection_t *pConnection, const uns8 *pData, int iSize,
                            int iChannel);
int   connection_send_internal(connection_t *pConnection, const uns8 *pData, int iSize,
                               int iType);
void  connection_send(connection_t *pConnection, const void *pData, int iSize, int iType);
int   connection_get_max_message_size(int iType);
route_t *connection_get_route(connection_t *pConnection);
void  connection_set_route(connection_t *pConnection, route_t *pRoute);
void  connection_get_ack_mask(connection_t *pConnection, uns64 *puiMask);
int   connection_append_message(connection_t *pConnection, buffer_t *pBuffer,
                                message_t *pMessage, message_list_t *pList, uns16 usFlush);
int   connection_append_state(connection_t *pConnection, buffer_t *pBuffer,
                              message_t *pMessage, message_list_t *pList, uns16 usFlush);
int   connection_append(connection_t *pConnection, buffer_t *pBuffer,
                        message_list_t *pList, uns16 usFlush);
uns32 connection_message_get_desired_flush_time(connection_t *pConnection, message_t *pMessage);
void  connection_get_next_message(connection_t *pConnection, message_list_t **ppList,
                                  uns32 *puiTime, int iHave);
int   connection_is_sw_sw(connection_t *pConnection);
void  connection_rx_reset(connection_t *pConnection);
void  connection_tx_reset(connection_t *pConnection);
int   connection_idle_check(connection_t *pConnection);
int   connection_flush(connection_t *pConnection, buffer_t *pBuffer, int iFlushNumber);
void  connection_recalc_desired_flush_time(connection_t *pConnection);
uns32 connection_get_desired_flush_time(connection_t *pConnection);
void  connection_add_rtt_sample(connection_t *pConnection, uns16 usSample);
void  connection_record_guaranteed_ack(connection_t *pConnection, uns16 usSeqnum, uns64 uiMask);
void  connection_record_state_acks(connection_t *pConnection, const state_ack_t *pAcks);
void  connection_record_flush_ack(connection_t *pConnection, uns16 usFlush);
void  connection_record_acks(connection_t *pConnection, segment_body_t *pBody);
void  connection_move_delayed_to_arrivals(connection_t *pConnection);
void  connection_record_state_message(connection_t *pConnection, message_t *pMessage,
                                      message_t *pWith);
void  connection_record_message(connection_t *pConnection, message_t *pMessage,
                                message_list_t *pList);
void  connection_record_segment_messages(connection_t *pConnection, segment_body_t *pBody);
void  connection_record_segment(connection_t *pConnection, const segment_header_t *pHeader,
                                segment_body_t *pBody);
int   connection_has_reliable(connection_t *pConnection, uns16 usSeqnum);
void  connection_log_parse_failure(connection_t *pConnection, const char *pFormat, ...);
int   connection_parse_segment_body(connection_t *pConnection, const segment_header_t *pHeader,
                                    segment_body_t *pBody, buffer_t *pBuffer);
int   connection_handle_segment(connection_t *pConnection, const segment_header_t *pHeader,
                                buffer_t *pBuffer);
uns64 connection_get_src(const connection_t *pConnection);
void *connection_get_app_ptr(const connection_t *pConnection);
void  connection_set_app_ptr(connection_t *pConnection, void *pAppPtr);
char *connection_get_app_info(const connection_t *pConnection);
void  connection_set_app_info(connection_t *pConnection, char *pAppInfo);
int   connection_has_instance_ack(connection_t *pConnection);
int   connection_has_instance_remote(connection_t *pConnection);
uns32 connection_get_instance_remote(connection_t *pConnection);
int   connection_send_instance_local(connection_t *pConnection);
uns32 connection_get_instance_local(connection_t *pConnection);
void  connection_send_reset(connection_t *pConnection, void *pData, int iSize);
void  connection_terminate(connection_t *pConnection, int iSendReset, void *pData, int iSize);
void  connection_terminate_file_line(connection_t *pConnection, int iSendReset, char *pFile,
                                     int iLine);
void  connection_terminate_reason(connection_t *pConnection, int iSendReset, uns8 ucReason);
int   connection_is_terminated(connection_t *pConnection);
void  connection_close_internal(connection_t *pConnection, int iNow);
void  connection_close(connection_t *pConnection);
void  connection_close_now(connection_t *pConnection);
int   connection_open(connection_t **ppConnection, endpoint_t *pEndpoint, uns64 uiDst);
void  connection_set_simulated(connection_t *pConnection, int iDrop, int iDelay, int iJitter);
int   connection_get_last_route_flush_number(connection_t *pConnection);
void  connection_set_bps_limit(connection_t *pConnection, int iLimit);
int   connection_get_bps_limit(connection_t *pConnection);
int   connection_get_bps_sent(connection_t *pConnection);
int   connection_get_bps_recv(connection_t *pConnection);
void  connection_receive_list(connection_t *pConnection, message_list_t *pList);
void  connection_short_circuit(connection_t *pConnection, connection_t *pWith);
int   connection_send_instance_push(const connection_t *pConnection);
int   connection_handle_reset(connection_t *pConnection, const segment_header_t *pHeader);
uns32 connection_get_time_last_instance_arrival(const connection_t *pConnection);
void  connection_get_arrival_history(connection_t *pConnection, uns64 *puiHistory);
void  connection_get_arrival_number(connection_t *pConnection, uns16 *pusNumber);
void  connection_get_timeout(connection_t *pConnection, int *piTimeout);
void  connection_get_ping_time(connection_t *pConnection, int *piPing);
void  connection_set_debug(connection_t *pConnection, int iDebug);
int   connection_get_debug(connection_t *pConnection);
int   connection_get_leader(connection_t *pConnection);
int   drdp_is_leader_endpoint(drdp_t *pDrdp, uns64 uiId);
void  connection_set_failure(connection_t *pConnection, int iRetry, int iWarn, int iFail);
uns32 connection_get_time_last_arrival(const connection_t *pConnection);
void  connection_pf(connection_t *pConnection, const char *pFormat, ...);
int   message_list_is_in_sync(message_list_t *pList);
void  connection_set_remote_id(connection_t *pConnection, uns64 uiRemoteId);
uns32 connection_get_debug_time(connection_t *pConnection);
void  connection_set_logger(connection_t *pConnection, drdp::logger_t *pLogger);
void  connection_handle_bad_crc(connection_t *pConnection);
void  connection_handle_corrupt(connection_t *pConnection);
int   connection_get_cstat_max();
uns64 connection_get_cstat(const connection_t *pConnection, int iIndex);
const char *connection_get_cstat_label(int iIndex);
void  connection_send_uns8(connection_t *pConnection, uns8 ucType);
void  connection_send_uns8_uns64(connection_t *pConnection, uns8 ucType, uns64 uiA);
void  connection_send_uns8_uns64_uns64(connection_t *pConnection, uns8 ucType, uns64 uiA, uns64 uiB);
void  connection_send_uns8_uns64_uns64_uns64(connection_t *pConnection, uns8 ucType, uns64 uiA,
                                             uns64 uiB, uns64 uiC);
void  connection_send_uns8_uns64_uns64_uns64_uns64(connection_t *pConnection, uns8 ucType,
                                                   uns64 uiA, uns64 uiB, uns64 uiC, uns64 uiD);
void  message_seqnum_destroy(message_seqnum_t *pSeqnum, buffer_t *pBuffer);
void  message_list_destroy(message_list_t *pList, buffer_t *pBuffer);
int   message_seqnum_load(message_seqnum_t *pSeqnum, endpoint_t *pEndpoint, buffer_t *pBuffer);
int   message_list_load(message_list_t *pList, endpoint_t *pEndpoint, buffer_t *pBuffer);
void  conection_state_channel_fixup(connection_t *pConnection, message_list_t *pList);
// The two packed-array templates.  Only the uns64 instantiation is emitted.
template <class T>
void buffer_appendm_packed(buffer_t *pBuffer, const T *pValue, int iCount)
{
    int i;

    for (i = 0; i < iCount; i++)
        buffer_append_packed_uns64(pBuffer, pValue[i]);
}

template <class T>
void buffer_readm_packed(buffer_t *pBuffer, T *pValue, int iCount)
{
    uns64 uiValue;
    int i;

    for (i = 0; i < iCount; i++)
    {
        buffer_read_packed_uns64(pBuffer, &uiValue);
        pValue[i] = uiValue;
    }
}

void  buffer_append_size(buffer_t *pBuffer, uns16 usSize);
void  buffer_read_size(buffer_t *pBuffer, uns16 *pusSize);
void  compress_flush_time_history(buffer_t *pBuffer, uns32 *puiHistory);
void  decompress_flush_time_history(buffer_t *pBuffer, uns32 *puiHistory);
void  compress_flush_time_history_verified(buffer_t *pBuffer, uns32 *puiHistory);

// The rest of the library, reached from connection.cpp.
extern "C" char *strdup(const char *);
extern "C" void *memset(void *, int, unsigned int);
extern "C" int   memcmp(const void *, const void *, unsigned int);

const char *drdp_format_endpoint2(uns64 uiId);
const char *drdp_format_endpoint4(uns64 uiId);
int   drdp_is_endpoint(uns64 uiId);
int   drdp_is_hardware_endpoint(uns64 uiId);
int   drdp_is_local_hardware_endpoint(uns64 uiId);
int   drdp_is_remote_hardware_endpoint(uns64 uiId);
void  drdp_connection_terminate(drdp_t *pDrdp, uns64 uiId, const void *pData, int iSize);
uns64 drdp_get_route_endpoint(drdp_t *pDrdp, uns64 uiId);
uns32 endpoint_get_instance(endpoint_t *pEndpoint);
int   endpoint_connection_create(endpoint_t *pEndpoint, connection_t **ppConnection, uns64 uiDst,
                                 uns64 uiRoute, uns32 uiInstance, int iFlags);
void  endpoint_connection_free_arrivals(connection_t *pConnection);
void  _endpoint_connection_destroy(connection_t *pConnection, const char *pFile, int iLine);
void  route_send_reset_connection(route_t *pRoute, uns64 uiSrc, uns64 uiDst, int iFlags,
                                  uns32 uiInstanceLocal, uns32 uiInstanceRemote);
void  route_send_reset_endpoint(route_t *pRoute, uns64 uiId);
void  route_set_simulated(route_t *pRoute, int iDrop, int iDelay, int iJitter);
void  route_set_bps_limit(route_t *pRoute, int iLimit);
int   route_get_bps_limit(route_t *pRoute);
int   route_get_bps_sent(route_t *pRoute);
int   route_get_bps_recv(route_t *pRoute);
void  route_resort(connection_t *pConnection);
#define uns16_sub(a, b) ((int16)((a) - (b)))   // inline in the original: no call

// Reached from endpoint.cpp, defined in connection.cpp and drdp.cpp.
endpoint_t *connection_get_endpoint(connection_t *pConnection);
uns64 connection_get_dst(const connection_t *pConnection);
int   connection_is_closed(connection_t *pConnection);
int   connection_is_in_sync(connection_t *pConnection);
int   connection_get_vnode_offset(int iWhich);
int   connection_get_key_offset(int iWhich);
uns32 drdp_get_time(const drdp_t *pDrdp);
int   drdp_has_endpoint(const drdp_t *pDrdp, const endpoint_t *pEndpoint);
void  drdp_arrival_enqueue(drdp_t *pDrdp, message_t *pMessage);
void  drdp_arrival_remove(drdp_t *pDrdp, message_t *pMessage);
void  drdp_connection_insert(drdp_t *pDrdp, connection_t *pConnection);
void  drdp_connection_remove(connection_t *pConnection);
int   drdp_endpoint_create(drdp_t *pDrdp, endpoint_t **ppEndpoint);
int   drdp_is_software_endpoint(uns64 uiId);

// ---------------------------------------------------------------------------
// route.cpp / route_cache.cpp / route_table.cpp -- SUPPORT 0x004bb370..0x004bcf00.
// A ROUTE is one UDP peer: the socket address every connection to that machine
// shares, the leaky bucket that paces it, and the tree of connections sorted by
// when each one wants to go out next.  `route_flush` is the only place in the
// library that builds a packet.

// Six bytes, the same shape as `ipaddress_t` -- four octets and a port.  The
// comparator memcmps the octets and then compares the ports unsigned, so the
// ordering is by address first.
struct drdp_addr_t
{
    uns8  m_ucIp[4];
    uns16 m_usPort;
};

int drdp_addr_t_cmp(const drdp_addr_t *pA, const drdp_addr_t *pB);

// A reset queued for the next flush.  It is a vset ELEMENT, so the set owns the
// copy and `vset_add` deduplicates through the comparator -- which is why the
// comparator walks all five fields.
struct reset_connection_t
{
    uns64 m_uiSrc;              // +0x00
    uns64 m_uiDst;              // +0x08
    int   m_iFlags;             // +0x10  bit 0 local instance, bit 1 remote
    uns32 m_uiInstanceLocal;    // +0x14
    uns32 m_uiInstanceRemote;   // +0x18
};                              //  0x20

struct reset_endpoint_t
{
    uns64 m_uiId;
};

int reset_connection_cmp(const reset_connection_t *pA, const reset_connection_t *pB);
int reset_endpoint_cmp(const reset_endpoint_t *pA, const reset_endpoint_t *pB);

// The route's tree of connections, keyed by desired flush time.  Its own type
// so `resort` can take it by name.
struct connections_by_desired_flush_time_t
{
    vtree_t tree;
};

void connections_by_desired_flush_time_resort(connections_by_desired_flush_time_t *pTree,
                                              connection_t *pConnection);

struct route_t
{
    char        m_szTag[4];         // +0x000  "ROUT"
    vnode_t     m_vnode[2];         // +0x004
    drdp_t     *m_pDrdp;            // +0x024
    uns64       m_uiId;             // +0x028  key 1
    uns32       m_uiDesiredFlushTime;   // +0x030  key 0
    vlist_t     m_pending;          // +0x034  raw bytes waiting for a packet
    connections_by_desired_flush_time_t m_connections;  // +0x04C
    vset_t      m_resetConnections; // +0x070
    vset_t      m_resetEndpoints;   // +0x094
    bandwidth_t m_bandwidth;        // +0x0B8
    int         m_iTerminated;      // +0x0E8
    int         m_iDrop;            // +0x0EC  the lag-and-loss simulator's
    int         m_iDelay;           // +0x0F0
    int         m_iJitter;          // +0x0F4
    int         m_iFlushNumber;     // +0x0F8  wraps at -1, never IS -1
    uns64       m_uiStat[13];       // +0x100
};                                  //  0x168

int   route_get_vnode_offset(int iIndex);
int   route_get_key_offset(int iIndex);
int   route_create(route_t **ppRoute, drdp_t *pDrdp, uns64 uiId);
int   route_is_valid(route_t *pRoute);
void  route_destroy(route_t *pRoute);
uns32 route_get_desired_flush_time(route_t *pRoute);
int   route_destroy_check(route_t *pRoute);
void  route_unlink_connection(connection_t *pConnection);
void  route_link_connection(route_t *pRoute, connection_t *pConnection);
void  route_terminate(route_t *pRoute, uns8 ucReason);
void  route_flush_reset_connection(route_t *pRoute, buffer_t *pBuffer);
void  route_flush_reset_endpoint(route_t *pRoute, buffer_t *pBuffer);
void  route_flush(route_t *pRoute);
void  route_recalc_desired_flush_time(route_t *pRoute);
void  segment_header_print(segment_header_t *pHeader);
void  route_forward_segment(route_t *pRoute, segment_header_t *pHeader, buffer_t *pBuffer);
int   route_is_terminated(route_t *pRoute);
drdp_t *route_get_drdp(route_t *pRoute);
connection_t *route_get_first_connection(route_t *pRoute);
connection_t *route_get_next_connection(route_t *pRoute, connection_t *pConnection);
void  route_record_arrival(route_t *pRoute, int iSize);

// route_cache.cpp -- the address <-> route translation, both directions, with a
// five-minute expiry.  One record is on THREE containers: a tree per direction
// and a list by expiry, which is why `rce` has two vnodes AND a vlink.
struct route_cache_entry_t
{
    vnode_t     m_vnode[2];     // +0x00
    lnode       m_vlink;        // +0x20
    drdp_addr_t m_address;      // +0x28  key 0
    drdp_addr_t m_route;        // +0x2E  key 1
    uns32       m_uiExpire;     // +0x34  key 2
};                              //  0x38

struct route_cache_t
{
    vtree_t m_byAddress;        // +0x00
    vtree_t m_byRoute;          // +0x24
    vlist_t m_byExpire;         // +0x48
};

int  rce_get_vnode_offset(int iIndex);
int  rce_get_vlink_offset(int iIndex);
int  rce_get_key_offset(int iIndex);
void route_cache_init(route_cache_t *pCache);
void route_cache_destroy(route_cache_t *pCache);
void route_cache_touch(route_cache_t *pCache, const drdp_addr_t *pAddress,
                       const drdp_addr_t *pRoute, uns32 uiNow);
void route_cache_release_expired(route_cache_t *pCache, uns32 uiNow);
void route_cache_address_to_route(route_cache_t *pCache, const drdp_addr_t *pAddress,
                                  drdp_addr_t *pRoute, uns32 uiNow);
void route_cache_route_to_address(route_cache_t *pCache, const drdp_addr_t *pRoute,
                                  drdp_addr_t *pAddress, uns32 uiNow);

// route_table.cpp -- which endpoint ids live behind which route, as a tree of
// RANGES.  The comparator returns 0 for any two that OVERLAP, so a lookup by a
// degenerate range finds the range that contains it.
struct uns64_range_t
{
    uns64 lo;
    uns64 hi;
};

struct route_table_entry_t
{
    vnode_t      m_vnode;       // +0x00
    uns64_range_t range;        // +0x10  key 0
    uns64        owner;         // +0x20
};                              //  0x28

struct route_table_t
{
    uns64   m_uiLo;             // +0x00
    uns64   m_uiHi;             // +0x08
    vtree_t entries;             // +0x10
};

int  uns64_range_t_cmp(const uns64_range_t *pA, const uns64_range_t *pB);
int  rte_get_vnode_offset(int iIndex);
int  rte_get_key_offset(int iIndex);
void route_table_update_range(route_table_t *pTable, uns64_range_t *pRange, uns64 uiRoute);
void route_table_update(route_table_t *pTable, uns64 uiId, uns64 uiRoute);
void route_table_update_block(route_table_t *pTable, uns64 uiId, uns64 uiRoute);
void route_table_init(route_table_t *pTable, uns64 uiLo, uns64 uiHi);
void route_table_destroy(route_table_t *pTable);
int  route_table_find(route_table_t *pTable, uns64 uiId, uns64 *puiRoute);

// The rest of drdp.cpp, reached from the route layer.
uns16 drdp_get_port_drdp(const drdp_t *pDrdp);
float drdp_get_flush_interval(drdp_t *pDrdp);
void  drdp_resort(route_t *pRoute);
void  drdp_crc_send(drdp_t *pDrdp, const drdp_addr_t *pAddress, buffer_t *pBuffer,
                    int iDrop, int iDelay, int iJitter);
void  drdp_endpoint_to_address(uns64 uiId, uns8 *pIp, uns16 *pusPort, uns16 *pusDrdpPort);
void  drdp_remap_address_to_route(drdp_t *pDrdp, const drdp_addr_t *pFrom, drdp_addr_t *pTo);
const char *drdp_format_addr1(const drdp_addr_t *pAddress);
const char *drdp_format_addr2(const drdp_addr_t *pAddress);
const char *drdp_format_addr3(const drdp_addr_t *pAddress);
const char *drdp_format_endpoint3(uns64 uiId);
const char *drdp_format_endpoint4(uns64 uiId);
int   connection_flush(connection_t *pConnection, buffer_t *pBuffer, int iFlushNumber);
void  connection_recalc_desired_flush_time(connection_t *pConnection);
void  buffer_append_segment_header(buffer_t *pBuffer, segment_header_t *pHeader);
void  buffer_read_segment_header(buffer_t *pBuffer, segment_header_t *pHeader, uns64 uiRoute,
                                 uns64 uiDst);

// ---------------------------------------------------------------------------
// Phase 45 -- the SOCKET LAYER, the floor the whole DRDP stack stands on.
// SUPPORT 0x004acc90..0x004ae088 is five more TUs of the same C-compiled-as-C++
// library: the assert every VASSERT lands in, the errno remap, the address
// codec, the PS2 net bring-up and the socket itself.
//
// uassert.cpp -- 0x004acc90..0x004acdb8.  Three functions and two file statics
// nothing else in the library writes: `_uassert_set_location` parks a file and
// a line so `_uassertf`, which takes a printf format instead of an expression
// string, can quote them.  Both report to stderr and then HANG -- `for (;;) ;`,
// not `abort` -- which is why a failed assert on a console is a freeze.
extern "C" void _uassert_set_location(const char *pFile, int iLine);
extern "C" void _uassertf(int iExpr, const char *pFormat, ...);

// uerror.cpp -- 0x004acdb8..0x004ace90.  The OS error remap.  Its table names
// the SCE INET error enum in full (`sceINETE_CONNECTION_REFUSED` and fourteen
// siblings, -500 through -514) but only two of the fifteen are MAPPED: a
// refused connection and a timeout.  Everything else is -1, and the lookup runs
// anyway with its result thrown away -- whatever trace consumed it is gone and
// the call for it is still there.
const char *get_errno_constant(int iError);
int uerror_remap_os_error_code(int iError);
int uerror_get_error();
int uerror_get_net_error(int iResult);

// ipaddress.cpp -- 0x004ace90..0x004acf58.  Six bytes: four address octets and
// a port, and the four codecs between that, SCE's 16-byte generic address and
// the wire.  `sceInetAddress` carries the address as a host-order word at +4,
// which is why the two converters count DOWN through it; the wire form is the
// octets in order and the port LITTLE-endian, which is not network order.
struct ipaddress_t
{
    uns8  m_ucIp[4];
    uns16 m_usPort;
};

struct sceInetAddress
{
    uns8 m_ucData[16];
};

void sceaddr_to_ipaddress(const sceInetAddress *pFrom, ipaddress_t *pTo);
void ipaddress_to_sceaddr(const ipaddress_t *pFrom, sceInetAddress *pTo);
void ipaddress_write(const ipaddress_t *pAddress, void *pData);
void ipaddress_read(ipaddress_t *pAddress, const void *pData);

// unet.cpp -- 0x004acf58..0x004ad3f0.  The bring-up, and the one place in the
// library that is PS2-specific: `sceLibnetInitialize` over a memalign'd 64 KB,
// a handler registration, `sceLibnetWaitGetAddress`, and the local address
// cached as a STRING so `unet_gethostname` can hand it back.  It runs on its
// own EE thread so the boot does not block on DHCP; `unet_startup` runs the
// same body inline when nobody asked for the async form first.
void unet_startup();
void unet_startup_async();
int  unet_startup_async_status();
void unet_shutdown();
int  unet_namelookup(const char *pName, ipaddress_t *pAddress);
int  unet_gethostname(char *pName, int iSize);
int  unet_get_local_address(ipaddress_t *pAddress);

int uns16_cmp_wrap(const uns16 *a, const uns16 *b);

// viusocket.cpp -- 0x004ae088..0x004aeb70.  The PS2 port's datagram path: two
// EE threads per socket, each with its own libnet client and work buffer, and
// two queues of 2,084-byte packets drawn from one global pool of sixteen.
// Read off VIInitUSocketEx's stores and VIFreeUSocketEx's `memset(.., 188)`.
struct sceSifMClientData
{
    uns32 m_uiData[15];
};

struct usocket_ex_t
{
    sceSifMClientData m_sendCd;     // +0x00  the send thread's libnet client
    sceSifMClientData m_recvCd;     // +0x3C
    uns32  *m_pSendBuf;             // +0x78  memalign(64, 2112)
    uns32  *m_pRecvBuf;             // +0x7C
    llist   m_sendList;             // +0x80  queued by VIUSocketSend
    llist   m_recvList;             // +0x8C  filled by the receive thread
    void   *m_pSendStack;           // +0x98  memalign(16, 8192)
    void   *m_pRecvStack;           // +0x9C
    int     m_iSendThread;          // +0xA0
    int     m_iRecvThread;          // +0xA4
    int     m_iSendError;           // +0xA8  sticky: the last send's failure
    int     m_iRecvError;           // +0xAC
    int     m_iSendSema;            // +0xB0
    int     m_iRecvSema;            // +0xB4
    int     m_iQuit;                // +0xB8
};                                  //  0xBC

// usocket.cpp -- 0x004ad480..0x004ae088.  200 bytes, of which this file names
// eleven: the tag, the SCE descriptor, the type, the listen flag and one byte
// of readiness bits.  The rest is the VIUSocketEx thread state hung off the
// back of the same allocation.
struct usocket_t
{
    char  m_szTag[4];       // +0x00  "SOCK", then "NULL" once destroyed
    int   m_iFd;            // +0x04  the sceInet descriptor, -1 when closed
    uns8  type;             // +0x08
    uns8  m_ucListen;       // +0x09
    uns8  m_ucFlags;        // +0x0A  bit 0 readable, bit 1 writable
    uns8  m_ucPad;          // +0x0B
    usocket_ex_t m_ex;      // +0x0C  VIInitUSocketEx's, never touched here
};                          //  0xC8

enum
{
    USOCKET_TYPE_UDP         = 0,
    USOCKET_TYPE_ICMP        = 1,
    USOCKET_TYPE_TCP_CONNECT = 2,
    USOCKET_TYPE_TCP_LISTEN  = 3
};

int  usocket_is_valid(usocket_t *pSocket);
int  usocket_create_internal(usocket_t **ppSocket, int iType, int iFd);
int  usocket_create(usocket_t **ppSocket, int iType, ipaddress_t *pLocal,
                    const ipaddress_t *pRemote);
void usocket_destroy(usocket_t *pSocket);
int  usocket_sendto_addr(usocket_t *pSocket, const ipaddress_t *pAddress,
                         const void *pData, int iSize, int iDrop, int iDelay,
                         int iJitter);
int  usocket_sendto(usocket_t *pSocket, const void *pData, int iSize, int iDrop,
                    int iDelay, int iJitter);
int  usocket_recvfrom_addr(usocket_t *pSocket, ipaddress_t *pAddress, void *pData,
                           int iSize, int *piRead);
int  usocket_recvfrom(usocket_t *pSocket, void *pData, int iSize, int *piRead);
int  usocket_select(usocket_t **ppSockets, int iCount, int iTimeout);
int  usocket_accept(usocket_t *pSocket, usocket_t **ppAccepted);
int  usocket_read_ready(usocket_t *pSocket);
int  usocket_mode_listen(usocket_t *pSocket);
int  usocket_get_fd(usocket_t *pSocket);
int  usocket_recv(usocket_t *pSocket, void *pData, int iSize, int *piRead);

// The VI glue, in its own TU just past this one: two EE threads and the two
// entry points usocket calls into.  Not attempted this phase.
int VIInitUSocketEx(usocket_t *pSocket);
int VIFreeUSocketEx(usocket_t *pSocket);
int VIUSocketRecv(usocket_t *pSocket, void *pData, int iSize, int *piFlags,
                  sceInetAddress *pFrom, int *piPort);
int VIUSocketSend(usocket_t *pSocket, const void *pData, int iSize, int iFlags,
                  sceInetAddress *pTo, int iPort);

// ---------------------------------------------------------------------------
// drdp.cpp -- SUPPORT 0x004b4aa8..0x004b9218, the top of the library.  A
// `drdp_t` is one PROCESS on the network: the sockets it listens on, every
// endpoint, route and connection it knows about, the NAT cache and the leaky
// bucket that paces the whole machine.  544 bytes off `malloc`, tagged "DRDP"
// the way a message is tagged "MESS".
//
// The asserts name six of its fields outright -- `drdp->role`,
// `drdp->system_endpoint`, `drdp->route_table.entries` and
// `drdp->endpoint_block[i].next` / `.remaining` -- which is where the naming
// below comes from.  The original is `C:/code-frontiers-gm/drdp/drdp.cpp`.

// One half-open run of software endpoint ids this process may hand out.  The
// master owns block 1 and gives block 0 away a chunk at a time; a leader is
// handed one and asks for another when it runs dry.
struct endpoint_block_t
{
    uns64 next;
    uns64 remaining;
};

// One leader the master is talking to: its endpoint id and the connection it
// arrived on.  `leader_get_key_offset(0)` is 16, so the id is the key.
struct leader_t
{
    vnode_t       m_vnode;          // +0x00
    uns64         m_uiId;           // +0x10  key 0
    connection_t *m_pConnection;    // +0x18
};                                  //  0x20

// The drdp's tree of routes sorted by when each wants to flush.  Its own type
// so `drdp_resort` can take it by name, exactly as the route's tree of
// connections is its own type in route.cpp.
struct routes_by_desired_flush_time_t
{
    vtree_t tree;
};

void routes_by_desired_flush_time_resort(routes_by_desired_flush_time_t *pTree,
                                         route_t *pRoute);

enum
{
    DRDP_ROLE_MASTER = 0,
    DRDP_ROLE_LEADER = 1,
    DRDP_ROLE_CLIENT = 2
};

// The three reserved drdp port numbers.  A local process picks its own from
// the low half of the clock and steps off any of these three.
enum
{
    DRDP_PORT_NONE        = 0xffff,
    DRDP_PORT_LEADER      = 0xfffe,
    DRDP_PORT_UNREACHABLE = 0xfffd
};

// The software endpoint id space, and the size of one block out of it.
#define DRDP_SW_ENDPOINT_MIN  2048
#define DRDP_SW_ENDPOINT_MAX  0xffffffffu
#define ENDPOINT_BLOCK_SIZE   2048

typedef void (*drdp_leader_callback_t)(drdp_t *pDrdp, connection_t *pConnection);
typedef void (*drdp_endpoint_callback_t)(drdp_t *pDrdp, uns64 uiId);

struct drdp_t
{
    char        m_szTag[4];             // +0x000  "DRDP"
    usocket_t  *m_socket[2];            // +0x004  udp and icmp
    int         m_iNumSocket;           // +0x00C
    int         m_iSocket;              // +0x010  round robin over the two
    vtree_t     m_endpoints;            // +0x014  by id
    routes_by_desired_flush_time_t m_routes;  // +0x038
    vtree_t     m_routesById;           // +0x05C
    vlist_t     m_arrivals;             // +0x080
    vtree_t     m_leaders;              // +0x098
    vtree_t     m_connections;          // +0x0BC
    route_table_t route_table;          // +0x0E0
    route_cache_t m_routeCache;         // +0x118
    bandwidth_t m_bandwidth;            // +0x178
    uns32       m_uiTime;               // +0x1A8
    int         role;                   // +0x1AC
    int         m_iBytesRecv;           // +0x1B0
    int         m_iBytesSent;           // +0x1B4
    int         m_iBytesDropped;        // +0x1B8
    uns32       m_uiTimeBps;            // +0x1BC  reset every second
    uns32       m_uiTimeRecv;           // +0x1C0
    uns32       m_uiTimeFlush;          // +0x1C4
    ipaddress_t m_bindAddress;          // +0x1C8
    uns16       m_usProtocolVersion;    // +0x1CE
    endpoint_block_t endpoint_block[2]; // +0x1D0
    endpoint_t *system_endpoint;        // +0x1F0
    void       *m_pAppPtr;              // +0x1F4
    drdp_leader_callback_t   m_pfnLeaderAdd;        // +0x1F8
    drdp_leader_callback_t   m_pfnLeaderRemove;     // +0x1FC
    drdp_endpoint_callback_t m_pfnEndpointDestroyed;// +0x200
    connection_t *m_pMasterConnection;  // +0x204
    void       *m_pData;                // +0x208  the master's app payload
    int         m_iDataSize;            // +0x20C
    int         m_i210;                 // +0x210
    bool        m_bNoSend;              // +0x214  the wire is suppressed
    drdp::logger_t *m_pLogger;          // +0x218
    int         m_i21C;                 // +0x21C
};                                      //  0x220

// The two entry points a message-level trace lands on.  Only the second is
// reached from this file, through a vtable slot -- so the class needs a first
// virtual for the offset to come out at 12.
namespace drdp
{
    struct logger_t
    {
        virtual void hack(const char *pWho, const char *pWhat);
        virtual void log(const char *pText);
    };
}

// The ICMP unreachable the process listens for on its second socket.
struct icmp_info_t
{
    uns8        m_ucType;   // +0x00
    uns8        m_ucCode;   // +0x01
    ipaddress_t m_local;    // +0x02
    ipaddress_t m_remote;   // +0x08
};                          //  0x0E

int icmp_parse(const void *pData, int iSize, icmp_info_t *pInfo);

void  drdp_addr_to_ipaddress(const drdp_addr_t *pFrom, ipaddress_t *pTo);
void  ipaddress_to_drdp_addr(const ipaddress_t *pFrom, drdp_addr_t *pTo);
void  drdp_assign_endpoint_block_master(drdp_t *pDrdp);
void  drdp_assign_endpoint_block(drdp_t *pDrdp, connection_t *pConnection);
int   drdp_create_addr(drdp_t **ppDrdp, const drdp_addr_t *pAddress, int iRole,
                       uns32 uiTime, bool bNoSend, drdp::logger_t *pLogger, void *pAppPtr,
                       drdp_leader_callback_t pfnLeaderAdd,
                       drdp_leader_callback_t pfnLeaderRemove,
                       drdp_endpoint_callback_t pfnEndpointDestroyed,
                       int iBpsLimit, int iQueueSizeMax);
int   drdp_create_client(drdp_t **ppDrdp, const drdp_addr_t *pAddress, uns32 uiTime,
                         bool bNoSend, drdp::logger_t *pLogger);
int   drdp_create_master(drdp_t **ppDrdp, const drdp_addr_t *pAddress, uns32 uiTime,
                         bool bNoSend, drdp::logger_t *pLogger, void *pAppPtr,
                         drdp_leader_callback_t pfnLeaderAdd,
                         drdp_leader_callback_t pfnLeaderRemove,
                         drdp_endpoint_callback_t pfnEndpointDestroyed,
                         const void *pData, int iSize);
int   drdp_create_leader(drdp_t **ppDrdp, const drdp_addr_t *pAddress,
                         const drdp_addr_t *pMaster, uns32 uiTime, bool bNoSend,
                         drdp::logger_t *pLogger, void *pAppPtr,
                         drdp_leader_callback_t pfnLeaderAdd,
                         drdp_leader_callback_t pfnLeaderRemove,
                         drdp_endpoint_callback_t pfnEndpointDestroyed,
                         const void *pData, int iSize);
int   drdp_is_valid(const drdp_t *pDrdp);
void  drdp_endpoint_destroy(endpoint_t *pEndpoint);
void  drdp_route_destroy(route_t *pRoute);
void  drdp_destroy(drdp_t *pDrdp);
int   drdp_namelookup(const char *pName, drdp_addr_t *pAddress);
int   drdp_gethostname(char *pName, int iSize);
void  drdp_route_terminate(drdp_t *pDrdp, drdp_addr_t *pAddress, uns8 ucReason);
void  drdp_re_route_connections_to_endpoint(drdp_t *pDrdp, uns64 uiId, uns64 uiRoute);
void  drdp_terminate_connections_to_endpoint(drdp_t *pDrdp, uns64 uiId);
void  drdp_handle_segment_endpoint_not_local(drdp_t *pDrdp, segment_header_t *pHeader,
                                             buffer_t *pBuffer);
int   drdp_handle_segment(drdp_t *pDrdp, buffer_t *pBuffer, uns64 uiRoute);
void  drdp_send_drdp_port_unreachable(drdp_t *pDrdp, const drdp_addr_t *pAddress, uns16 usPort);
void  drdp_send_drdp_proto_unreachable(drdp_t *pDrdp, const drdp_addr_t *pAddress);
int   drdp_process_connectionless(drdp_t *pDrdp, drdp_addr_t *pAddress, buffer_t *pBuffer);
int   drdp_consume_crc_get_protocol_version(buffer_t *pBuffer);
int   drdp_process_udp(drdp_t *pDrdp, const void *pData, int iSize);
int   drdp_process_packet(drdp_t *pDrdp, const void *pData, int iSize);
int   drdp_receive_packet(drdp_t *pDrdp, void *pData, int iSize, int *piRead, int iTimeout);
void  drdp_set_time(drdp_t *pDrdp, uns32 uiTime);
void  drdp_flush(drdp_t *pDrdp);
int   drdp_get_sw_endpoint_status(drdp_t *pDrdp, uns64 uiId);
int   drdp_endpoint_is_local(const drdp_t *pDrdp, uns64 uiId);
endpoint_t *drdp_find_endpoint(drdp_t *pDrdp, uns64 uiId);
void  drdp_leader_set_connection(drdp_t *pDrdp, uns64 uiId, connection_t *pConnection);
void  drdp_leader_remove_internal(drdp_t *pDrdp, uns64 uiId);
void  drdp_close_connections_to_endpoint(drdp_t *pDrdp, uns64 uiId);
void  drdp_reconcile_local_endpoints(drdp_t *pDrdp, endpoint_t *pEndpoint);
void  drdp_invalidate_range(drdp_t *pDrdp, uns64_range_t *pRange);
void  drdp_invalidate_all_endpoints_owned_by_leader(drdp_t *pDrdp, uns64 uiId);
void  drdp_master_send_route_table(drdp_t *pDrdp, connection_t *pConnection);
void  drdp_slave_read_route_table(buffer_t *pBuffer, route_table_t *pTable, uns64 uiOwner);
void  drdp_leader_remove(drdp_t *pDrdp, uns64 uiId);
int   drdp_leader_add_internal(drdp_t *pDrdp, uns64 uiId, connection_t *pConnection);
void  drdp_handle_block_transfer(drdp_t *pDrdp, uns64 uiNext);
void  drdp_handle_block_received(drdp_t *pDrdp, uns64 uiNext, uns64 uiId);
void  drdp_handle_endpoint_destroyed(drdp_t *pDrdp, uns64 uiId, uns64 uiFrom);
int   drdp_leader_add(drdp_t *pDrdp, connection_t *pConnection);
void  drdp_handle_endpoint_info(drdp_t *pDrdp, uns64 uiId, uns64 uiRoute);
void  drdp_process_system_message(drdp_t *pDrdp, message_t *pMessage);
message_t *drdp_receive(drdp_t *pDrdp);
int   drdp_get_route(drdp_t *pDrdp, uns64 uiId, route_t **ppRoute);
void  drdp_address_to_endpoint(const uns8 *pIp, uns16 usPort, uns16 usDrdpPort, uns64 *puiId);
char *drdp_format_addr(char *pName, const drdp_addr_t *pAddress);
char *drdp_format_endpoint(char *pName, uns64 uiId);
void  drdp_send_reset_endpoint(drdp_t *pDrdp, const segment_header_t *pHeader);
int   leader_get_vnode_offset(int iIndex);
int   leader_get_key_offset(int iIndex);
void  default_callback_leader_add(drdp_t *pDrdp, connection_t *pConnection);
void  default_callback_leader_remove(drdp_t *pDrdp, connection_t *pConnection);
extern uns32 drdp_crc_table[256];
int   drdp_get_leader_connections(drdp_t *pDrdp, connection_t **ppConnections, int iMax);
void  drdp_set_default_logger(drdp_t *pDrdp, drdp::logger_t *pLogger);
drdp::logger_t *drdp_get_default_logger(drdp_t *pDrdp);
int   drdp_is_software_endpoint(uns64 uiId);
uns16 drdp_get_port_udp(const drdp_t *pDrdp);
void  drdp_set_port_udp(drdp_t *pDrdp, uns16 usPort);
connection_t *drdp_get_master_connection(drdp_t *pDrdp);
int   drdp_get_bps_sent(drdp_t *pDrdp);
int   drdp_get_bps_recv(drdp_t *pDrdp);
void  drdp_endpoint_transfer(drdp_t *pDrdp, endpoint_t *pEndpoint, uns64 uiTo,
                             const void *pData, int iSize);
void  drdp_route_override(drdp_t *pDrdp, uns64 uiFrom, uns64 uiTo);
void  drdp_send_addr(drdp_t *pDrdp, const drdp_addr_t *pAddress, int iCount,
                     const void *pData, int iSize);
endpoint_t *drdp_get_system_endpoint(drdp_t *pDrdp);
int   drdp_is_client(drdp_t *pDrdp);
void  drdp_send_reset_connection(drdp_t *pDrdp, const segment_header_t *pHeader);
int   drdp_get_local_address(uns8 *pIp);
void  drdp_set_app_ptr(drdp_t *pDrdp, void *pAppPtr);
void *drdp_get_app_ptr(const drdp_t *pDrdp);
ipaddress_t *drdp_get_bind_address(const drdp_t *pDrdp);
void  drdp_remap_route_to_address(drdp_t *pDrdp, const drdp_addr_t *pFrom, drdp_addr_t *pTo);
void  drdp_log_hack_attempt(drdp_t *pDrdp, const char *pWho, const char *pWhat);
uns32 drdp_crc(uns32 uiCrc, const char *pData, uns32 uiSize);
uns32 drdp_crc_table_get(int iIndex);
int   drdp_crc_table_lookup(uns32 uiValue, int iIndex);
int   drdp_process_icmp(drdp_t *pDrdp, const void *pData, int iSize);

endpoint_t *connection_get_endpoint(connection_t *pConnection);
int   endpoint_handle_segment(endpoint_t *pEndpoint, segment_header_t *pHeader,
                              buffer_t *pBuffer);
void  endpoint_reconcile_connections(endpoint_t *pEndpoint, connection_t *pConnection,
                                     endpoint_t *pWith);
void  endpoint_transfer(endpoint_t *pEndpoint, uns64 uiTo, const void *pData, int iSize);
int   endpoint_create_buffer(endpoint_t **ppEndpoint, drdp_t *pDrdp, buffer_t *pBuffer);
void  endpoint_destroy(endpoint_t *pEndpoint);
int   route_is_valid(route_t *pRoute);
extern "C" int sprintf(char *, const char *, ...);

// Names the original's own asserts quote (eqrefcheck reads them out of the
// relocated dump); the values are the immediates the matched code already used.
#define SEGMENT_HEADER_SRC_ADDR           0x8000
#define SEGMENT_HEADER_RESET_ENDPOINT     0x20000
#define MESSAGE_TYPE_CONNECT              255
#define MESSAGE_TYPE_DISCONNECT           253
// The transports' own names, from the assert in drdp_master_send_route_table
// and from a { 0, "MESSAGE_TYPE_SYSTEM", "MESSAGE_TYPE_FRAGMENT",
// "MESSAGE_TYPE_GUARANTEED", 0 } name table that sits, unreferenced, after
// connection.cpp's cstat_text: 249..251 in order.  252 (best effort) and 248
// (the state channel count) have no name in the binary.
#define MESSAGE_TYPE_SYSTEM               249
#define MESSAGE_TYPE_FRAGMENT             250
#define MESSAGE_TYPE_GUARANTEED           251
#define DRDP_SEND_SIZE_MAX_GUARANTEED     0x514000
#define ONE_DAY 86400000
#define CONNECTION_MAX_OUTSTANDING_STATE  4096
#define rt_peek_first(t) ((route_table_entry_t *)vtree_peek_first(t))
#define rt_peek_last(t)  ((route_table_entry_t *)vtree_peek_last(t))

#endif
