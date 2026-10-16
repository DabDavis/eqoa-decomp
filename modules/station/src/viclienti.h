// viclienti.h -- STATION's VIClientI (NOT shared: CLIENT's src/viclienti.h is its
// own, game.h's 18-slot class).  vicsclient.cpp and vicsrecords.cpp include it
// for the one virtual they call.
#ifndef VICLIENTI_H
#define VICLIENTI_H

#include "vinet.h"   // VIClientConn

// No data: its vptr is at +0.  __tf 0x002E1FB8.  CLIENT's VIClientI has the same
// slot 1 and 17 more; STATION's build of it has only this one.
class VIClientI
{
public:
    virtual VIClientConn GetConn(int type) = 0;   // slot 1
};


#endif
