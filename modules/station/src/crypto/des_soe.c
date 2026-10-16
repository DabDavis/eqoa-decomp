/* des_soe.c (name ours) -- STATION 0x002D21F0 .. 0x002D2320: SOE's three calls
 * around libdes.  They link between libdes's cbc_cksm.c and ecb_enc.c, so this
 * file sat in libdes's own folder and its name sorts between theirs.
 *
 * The Station protocol hashes a fixed string ("LPSS-V1.0$@Z>.ADJ", VIStationAPI)
 * into one DES key with des_string_to_key, then CBC-decrypts or -encrypts a
 * buffer under it, the key doubling as the IV.  All three names are ours; the
 * wrappers are C, like the library they wrap (nothing in the image says
 * otherwise). */
#include "des_locl.h"

static des_cblock sKey;   /* 0x003735C0 */

/* 0x002D21F0 */
void des_soe_set_key(char *str)
{
	des_string_to_key(str, &sKey);
}

/* 0x002D2210 */
void des_soe_decrypt(des_cblock *in, des_cblock *out, long length)
{
	des_key_schedule ks;
	des_cblock iv;

	memcpy(iv, sKey, sizeof(des_cblock));
	des_set_key(&iv, ks);
	des_ncbc_encrypt(in, out, length, ks, &iv, DES_DECRYPT);
}

/* 0x002D2298 */
void des_soe_encrypt(des_cblock *in, des_cblock *out, long length)
{
	des_key_schedule ks;
	des_cblock iv;

	memcpy(iv, sKey, sizeof(des_cblock));
	des_set_key(&iv, ks);
	des_ncbc_encrypt(in, out, length, ks, &iv, DES_ENCRYPT);
}
