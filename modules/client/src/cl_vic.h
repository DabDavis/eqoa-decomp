// cl_vic.h -- VIClient.cpp's leftovers (ctor, dtor, InitBody, Process, ...) and the 43 open functions; OWNER of game.h's VIClient/VIClientI.
// CLIENT 0x00BBF0D8..0x00BE3C28.
// cl-vic owns game.h itself (VIClient, VIClientI and everything above them that no
// other lane claims); this header is for any NEW class it needs.
#ifndef CL_VIC_H
#define CL_VIC_H

// PLACEHOLDER for cl-parts2's class (its code is 0x00C03B18.. 0x00C04130):
// a ring of 32 wide lines of 80, head and tail first; VIWndGame hands one out
// (GetChatLog) and SendReport copies it whole (5136 bytes, by ld/sd: 8-aligned).
// (VITextRing was cl-parts2's VIChatText)

// SUPPORT's DRDP rate counters (UpdateGame's "s%04ubps r%04ubps"); they belong
// in the shared drdpapi.h, which a lane does not edit.
int drdp_get_bps_sent(drdp_t *pDrdp);
int drdp_get_bps_recv(drdp_t *pDrdp);

// The 0x40's quantisation boxes: globals another TU's static initialiser fills
// (0x00CC6558, cl-tail's range), by address until named.  Position, velocity, force,
// angles, angular velocity, torque.
void WrapAngle(float &a);                          // cl-tail's: wraps one angle (WrapAngles' single-float twin)

#endif
