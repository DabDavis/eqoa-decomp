// main.cpp (name ours) -- SETUP 0x00566C00 .. 0x00567838: the module's entry
// point, then the VIMap<int, int> copy the rest of the module links against.
#include "setup.h"
#include "vimap_t.h"

extern "C" int initEzNetCnf();
extern "C" int exitEzNetCnf();
extern "C" int ezNetCtlDownInterface(int);
extern "C" int sceDevctl(const char *name, int cmd, const void *arg, unsigned int arglen,
                         void *buf, unsigned int buflen);
extern "C" int sceSifMExitRpc();
extern "C" int sceSifStopModule(int id, int argLen, const char *args, int *result);
extern "C" int sceSifUnloadModule(int id);
int  VIEELoadIrx(const char *name, int argLen, const char *args);
int  VIInitDiscId();
void VIDisplayScreen16(const char *s, int x, bool b);
void preparePowerOff();
void unet_shutdown();

// The IRX modules this module loaded, unloaded again on the way out when the
// network was brought up.
static int sIrx[16];

// 0x00566C00.  What it returns tells SLUS which scene to launch next: 2 and 5
// out of the setup's own exit paths, 0 when the network never came up.
int setup_main()
{
    VISetup *setup = new VISetup;
    VIInitDiscId();
    for (int i = 15; i >= 0; i--)
        sIrx[i] = -1;
    VIEELoadIrx("EZNETCNF.IRX", 0, 0);
    initEzNetCnf();
    sIrx[0] = VIEELoadIrx("MSIFRPC.IRX", 0, 0);
    sIrx[1] = VIEELoadIrx("LIBNET.IRX", 0, 0);
    if (VIGetFeatureFlags() & 0x800)
    {
        LoadIrxRetry("DEV9.IRX", 0, 0, 25);
        if (sceDevctl("dev9x:", 0x4401, 0, 0, 0, 0) < 0)
        {
            VIDisplayScreen16("ERROR", 3, true);
            for (;;)
                ;
        }
        preparePowerOff();
    }
    if (setup->Init(1) < 0)
        return -1;
    int r;
restart:
    while (!(r = setup->Process()))
        ;
    if (r == 3 && setup->RunPatcher() == 4)
    {
        setup->RestartConnect();
        goto restart;
    }
    int net = setup->RestartRequired();
    setup->Free();
    if (net)
        ezNetCtlDownInterface(0);
    exitEzNetCnf();
    printf("deleting setup\n");
    delete setup;
    if (r == 2)
        return 2;
    if (net)
    {
        unet_shutdown();
        sceSifMExitRpc();
        for (int i = 15; i >= 0; i--)
        {
            if (sIrx[i] >= 0)
            {
                int result;
                sceSifStopModule(sIrx[i], 0, 0, &result);
                sceSifUnloadModule(sIrx[i]);
                sIrx[i] = -1;
            }
        }
    }
    return net ? 5 : 0;
}

// 0x00566E68 .. 0x00567838.  Every TU that includes the header that uses it
// carries a copy of VIMap<int, int> (ten in all, no COMDAT); this first one is
// the copy the linker resolved every call to.  Nothing in the module calls
// Init, Insert or Find.
template int VIMap<int, int>::Begin() const;
template int VIMap<int, int>::End() const;
template int VIMap<int, int>::Next(int) const;
template int VIMap<int, int>::DestroyPool();
template int VIMap<int, int>::Clear();
template int VIMap<int, int>::LinkFreeChain(int);
template int VIMap<int, int>::Allocate();
template int VIMap<int, int>::Init(int);
template int VIMap<int, int>::AddNode(int, int);
template int VIMap<int, int>::LeftRotate(int);
template int VIMap<int, int>::RightRotate(int);
template int VIMap<int, int>::Insert(const int &, const int &);
template int VIMap<int, int>::Find(const int &) const;
