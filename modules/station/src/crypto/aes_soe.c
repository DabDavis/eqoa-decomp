/* aes_soe.c (name ours) -- STATION 0x002D7FB8 .. 0x002D80C8: SOE's two buffer
 * calls over LibTomCrypt's rijndael.  They link between aes.c and mpi.c, so this
 * file sat in LibTomCrypt's folder and its name sorts between theirs.
 *
 * ECB over len / 16 whole blocks, in place or not; the output length is never
 * read.  station_main encrypts the password under a fixed key with the first.
 * Both names are ours. */
#include "mycrypt.h"

/* 0x002D7FB8 */
void aes_ecb_encrypt_buffer(const unsigned char *in, int len, unsigned char *out,
                            int outlen, symmetric_key *skey)
{
   int i;

   for (i = 0; i < len / 16; i++)
      rijndael_ecb_encrypt(in + i * 16, out + i * 16, skey);
}

/* 0x002D8040 */
void aes_ecb_decrypt_buffer(const unsigned char *in, int len, unsigned char *out,
                            int outlen, symmetric_key *skey)
{
   int i;

   for (i = 0; i < len / 16; i++)
      rijndael_ecb_decrypt(in + i * 16, out + i * 16, skey);
}
