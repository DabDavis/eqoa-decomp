// drdpapi.h (name ours) -- SUPPORT's DRDP library as CLIENT and STATION call it,
// declared rather than by including SUPPORT's drdp.h beside vi.h (no SUPPORT TU
// includes both).  Same C++ linkage, so the same names.  This is CLIENT's
// game.h block verbatim (game.h lines 40-72 and 965-979 when written), plus
// endpoint_send, which only STATION's VINet.cpp copy needs a declaration for;
// when the file is shared, game.h includes this instead of carrying its own.
#ifndef DRDPAPI_H
#define DRDPAPI_H

struct drdp_t;
struct connection_t;
void connection_close(connection_t *pConnection);
void drdp_flush(drdp_t *pDrdp);
struct endpoint_t;
int  connection_open(connection_t **ppConnection, endpoint_t *pEndpoint, unsigned long uiDst);
void connection_set_failure(connection_t *pConnection, int iRetry, int iWarn, int iFail);
endpoint_t *drdp_get_system_endpoint(drdp_t *pDrdp);
int  drdp_receive_packet(drdp_t *pDrdp, void *pData, int iSize, int *piRead, int iTimeout);
void drdp_set_time(drdp_t *pDrdp, unsigned int uiTime);
void drdp_destroy(drdp_t *pDrdp);
void connection_send(connection_t *pConnection, const void *pData, int size, int message_type);
void connection_get_timeout(connection_t *pConnection, int *piTimeout);
struct drdp_addr_t
{
    unsigned char  m_ucIp[4];
    unsigned short m_usPort;
};
void drdp_endpoint_to_address(unsigned long uiId, unsigned char *pIp, unsigned short *pusPort, unsigned short *pusDrdpPort);
namespace drdp { struct logger_t; }
int  drdp_namelookup(const char *pName, drdp_addr_t *pAddress);
int  drdp_create_client(drdp_t **ppDrdp, const drdp_addr_t *pAddress, unsigned int uiTime, bool bNoSend, drdp::logger_t *pLogger);
unsigned short drdp_get_port_udp(const drdp_t *pDrdp);
void drdp_set_port_udp(drdp_t *pDrdp, unsigned short usPort);
void drdp_address_to_endpoint(const unsigned char *pIp, unsigned short usPort, unsigned short usDrdpPort, unsigned long *puiId);
void connection_set_simulated(connection_t *pConnection, int iDrop, int iDelay, int iJitter);
void drdp_send_addr(drdp_t *pDrdp, const drdp_addr_t *pAddress, int iCount, const void *pData, int iSize);
int connection_get_cstat_max();
unsigned long connection_get_cstat(const connection_t *pConnection, int iIndex);
const char *connection_get_cstat_label(int iIndex);
int drdp_process_packet(drdp_t *pDrdp, const void *pData, int iSize);
unsigned int drdp_get_time(const drdp_t *pDrdp);
struct message_t;
message_t *drdp_receive(drdp_t *pDrdp);
void message_destroy(message_t *pMessage);
unsigned long message_get_src(const message_t *pMessage);
unsigned char message_get_type(const message_t *pMessage);
void *message_get_data(message_t *pMessage);
unsigned int message_get_size(const message_t *pMessage);
connection_t *message_get_connection(message_t *pMessage);
void connection_get_arrival_history(connection_t *pConnection, unsigned long *puiHistory);
void connection_get_arrival_number(connection_t *pConnection, unsigned short *pusNumber);
void connection_get_ping_time(connection_t *pConnection, int *piPing);
unsigned int connection_get_time_last_arrival(const connection_t *pConnection);
void endpoint_send(endpoint_t *pEndpoint, unsigned long uiDst, const void *pData, int size, int message_type);

#endif
