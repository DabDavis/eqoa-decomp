// stcrypto.h -- OWNER: lane crypto.  What STATION's C++ calls in the C libraries it
// links (0x002D1F68 .. 0x002DC7F0, all in src/crypto/: libdes and LibTomCrypt are
// fetched and patched by modules/station/crypto/fetch.py; des_soe.c and aes_soe.c are
// ours):
//   0x002D1F68 .. 0x002D49C0  libdes 3.24, SSLeay 0.6.3's DES (des_options() is the fixed
//                             string "des(idx,long)"), plus SOE's des_soe_* wrappers
//   0x002D49C0 .. 0x002D80C8  LibTomCrypt's rijndael (a pre-0.67 release: string errors,
//                             32-bit key and table words), plus SOE's aes_ecb_*_buffer
//   0x002D80C8 .. 0x002DC7F0  Michael Fromberger's MPI, as that LibTomCrypt carried it
//                             (linked whole; nothing in STATION calls it)
// The C side declares these itself (des_locl.h, mycrypt.h); this header is the C++ view.
#ifndef STCRYPTO_H
#define STCRYPTO_H

// LibTomCrypt.  The key-schedule union is 4,176 bytes on station_main's stack:
// Blowfish's 32-bit schedule (4,168 bytes), rounded to 16.
#define CRYPT_OK    0
#define CRYPT_ERROR 1
union symmetric_key
{
    unsigned char m_x[4176];
};
extern "C" int  rijndael_setup(const unsigned char *key, int keylen, int rounds,
                               symmetric_key *skey);
// 0x002D7FB8 / 0x002D8040: rijndael ECB over len / 16 blocks; the out length is
// never read.  Names ours.
extern "C" void aes_ecb_encrypt_buffer(const unsigned char *in, int len,
                                       unsigned char *out, int outlen,
                                       symmetric_key *skey);
extern "C" void aes_ecb_decrypt_buffer(const unsigned char *in, int len,
                                       unsigned char *out, int outlen,
                                       symmetric_key *skey);

// libdes.  SOE's three wrappers (0x002D21F0 .. 0x002D2298, names ours): hash a string
// into one module key with des_string_to_key, then CBC-decrypt / -encrypt a buffer
// under it, the key doubling as the IV.  VIStationAPI keys them with
// "LPSS-V1.0$@Z>.ADJ".
typedef unsigned char des_cblock[8];
extern "C" void des_soe_set_key(char *str);
extern "C" void des_soe_decrypt(des_cblock *in, des_cblock *out, long length);
extern "C" void des_soe_encrypt(des_cblock *in, des_cblock *out, long length);

#endif
