// cl_vic.h -- what VIClient.cpp (CLIENT 0x00BBF0D8..0x00BE3C28) declares beyond game.h,
// which holds VIClient and VIClientI themselves.
#ifndef CL_VIC_H
#define CL_VIC_H

// SUPPORT's DRDP rate counters (UpdateGame's "s%04ubps r%04ubps"); they belong
// in SUPPORT's drdpapi.h.
int drdp_get_bps_sent(drdp_t *pDrdp);
int drdp_get_bps_recv(drdp_t *pDrdp);

void WrapAngle(float &a);                          // viexperience.cpp: wraps one angle (WrapAngles' single-float twin)

#endif
