// station.h -- STATION's own code: the SOE Station account screens, the GUI page
// system they are written in (STATION.PAK's PS2GUIPage XML), the account server
// client and customer service, on top of SUPPORT's vi.h.  STATION exports one
// name (station_main), so every class name here is its RTTI string, and every
// method name is a vtable slot of a known base, a SETUP or CLIENT twin, or ours.
//
// What the module links, in address order (0x002B6B80 ..):
//   main, vistation, viwindowmanager, vifader (SETUP's), viwndedit (CLIENT has
//   it too), viwndstationlogin, the VIGUI* classes, viwndgenericrenderer,
//   Eric Young's libdes, LibTomCrypt's rijndael, Fromberger's MPI, VUrl
//   (SETUP's), the customer-service client, VIMemStream* (CLIENT's), the
//   flat-file reader, the station API and its TCP connection, the CS windows,
//   libxml2 2.3.6 (SETUP's build minus testXPath.c), and VIWndCombo.
//
// ONE HEADER PER AREA, each declaring what that area's TUs define (see
// the comment at the top of each).  Included below in dependency order: a class
// that embeds another by value needs the complete type first.
#ifndef STATION_H
#define STATION_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vi.h"
#include "vifader.h"       // SETUP's, shared
#include "vurl.h"          // SETUP's, shared
#include "vimemstream.h"   // CLIENT's, shared

#define CHK(n, c)  typedef char n[(c) || VI_PORT ? 1 : -1];

// The libgraph call station_main makes four times (a LIB function, 0x0034FE28).
extern "C" int sceGsSyncV(int mode);
extern "C" void malloc_stats(void);
extern "C" int ezNetCtlDownInterface(int);
int  VIHasDiscIdChanged();
void VIDisplayScreen16(const char *s, int x, bool b);
VIGameOptions *VIGetGameOptions();

// What every TU that includes CLIENT's item header carries a copy of (see
// SETUP's viwndbeta.cpp): an 8-byte POD element with no constructor.
struct VIItemStat
{
    int m_id;
    int m_value;
};

// Every class, so any header can name any other by pointer.
class VIStation;
class VIWindowManager;
class VIWndStationLogin;
class VIWndEdit;
class VIGUIDataModel;
class VIGUIStringDataModel;
class VIGUIToggleDataModel;
class VIGUIObject;
class VIGUIText;
class VIGUIButton;
class VIGUIToggleButton;
class VIGUIComboBox;
class VIGUIPage;
class VIGUIDialog;
class VIGUISelectionGrid;
class VIWndGenericRenderer;
class VIClientI;
class VICSInterface;
class VIFlatFile;
class VIFlatFileDirectory;
class VIStationListener;
class VIConnectionListener;
class VIStationAPI;
class VIWndCSInbox;
class VIWndCSRequest;
class VIWndReadCS;
class VIWndMBox;
class VIWndMessage;
class VIWndOptionList;
class VIWndCombo;

#include "stcrypto.h"        // libdes, rijndael, MPI (C)
#include "stnet.h"           // VIMemStream containers, VIFlatFile, time, vinet
#include "stapi.h"           // VINetBuffer, VITCPConnection, VIStationAPI, listeners
#include "stcs.h"            // the CS client, VIClientI, VICSInterface, CS records
#include "stedit.h"          // VIWndEdit (CLIENT has it too)
#include "stgui.h"           // VIGUIDataModel*, VIGUIObject, Text, Button, Toggle, ComboBox
#include "stpage.h"          // VIGUIPage, VIGUIDialog, VIGUISelectionGrid, the page parser
#include "stwndcs.h"         // VIWndCSInbox, VIWndCSRequest, VIWndReadCS
#include "stwnd.h"           // VIWndMBox, VIWndMessage, VIWndOptionList, VIWndCombo, XML helpers
#include "strenderer.h"      // VIWndGenericRenderer
#include "stcore.h"          // VIStation, VIWindowManager, VIWndStationLogin

#endif
