// main.cpp (name ours) -- STATION 0x002B6B80 .. 0x002B7978: the module's entry
// point, then the VIMap<int, int> and VIArray<VIItemStat> copies the rest of the
// module links against.
#include "station.h"
#include "vimap_t.h"
#include "viarray_t.h"

// 0x00353B10: the AES-256 key the password goes back under.
static const unsigned char sKey[32] =
{
    0xEC, 0xCA, 0xBF, 0x71, 0xF4, 0xBF, 0x32, 0x6B, 0x57, 0x29, 0x72, 0x4D, 0xC1, 0xD7, 0xAA, 0xAC,
    0x53, 0x84, 0x37, 0xDA, 0x68, 0xE4, 0x3B, 0xB4, 0xA5, 0xE4, 0xA4, 0xE9, 0xD4, 0x89, 0x6D, 0x44,
};

// 0x002B6B80.  SLUS hands in two 256-byte buffers and gets the login back in the
// first and the password, AES-encrypted under a fixed key, in the second.  It
// returns 1 when the player asked for the network to come down (Process 2).
int station_main(char *login, char *password)
{
    switch (VIHasDiscIdChanged())
    {
    case -1:
        for (;;)
            ;
    case 1:
        VIDisplayScreen16("ERROR", 4, true);
        for (;;)
            ;
    }
    malloc_stats();
    printf("did malloc stats\n");
    sceGsSyncV(0);
    sceGsSyncV(0);
    printf("creating VIStation...\n");
    sceGsSyncV(0);
    sceGsSyncV(0);
    VIStation *station = new VIStation;
    printf("created VIStation\n");
    sceGsSyncV(0);
    sceGsSyncV(0);
    if (station->Init(login, password) < 0)
        return -1;
    int result;
    while (!(result = station->Process()))
        ;
    strcpy(login, station->m_login);
    strcpy(password, station->m_password);
    printf("Login = %s\n", login);
    symmetric_key skey;
    if (rijndael_setup(sKey, 32, 0, &skey) != CRYPT_ERROR)
    {
        unsigned char buf[32];
        memset(buf, 0, 32);
        strcpy((char *)buf, password);
        aes_ecb_encrypt_buffer(buf, 32, (unsigned char *)password, 32, &skey);
    }
    station->Close();
    delete station;
    printf("Done deleting station\n");
    malloc_stats();
    printf("!!Exiting\n");
    if (result == 2)
    {
        ezNetCtlDownInterface(0);
        return 1;
    }
    return 0;
}

// 0x002B6DB8 .. 0x002B7928.  Every TU that includes the item header carries its
// own copy of both groups (no COMDAT); this first one is the copy the linker
// resolved every call to.
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

template VIArray<VIItemStat>::VIArray();
template int VIArray<VIItemStat>::DestroyPool();
template VIArray<VIItemStat>::~VIArray();
template int VIArray<VIItemStat>::Clear();
template int VIArray<VIItemStat>::Allocate();
template int VIArray<VIItemStat>::Init(int);
