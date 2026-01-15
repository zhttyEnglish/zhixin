/*
 * SmartChip program for generating derived key
 */

#include <common.h>
#include "spacc/spacc.h"

int gen_derived_key(unsigned char *str1, unsigned char *str2,
    unsigned char *iv, unsigned char iv_len,
    unsigned char *dst, unsigned int key_len,
    int key_idx)
{
	int i, ret;
	unsigned char tmp_iv[16];

	if(key_idx > 1) {
		return -1;
	}

	if(key_len != 32 || iv_len != 16) {
		return -1;
	}

	/* encrypt string 1 */
	ret = spacc_cipher(CIPH_ALG_AES, CIPH_MODE_CBC, 1,
	        str1, 32, dst, 32, NULL, 32, key_idx, iv, 16);
	if(ret)
		return -1;

	/* update iv */
	for(i = 0; i < 16; i++) {
		tmp_iv[i] = dst[i] ^ dst[i + 16];
	}

	/* encrypt string 2 */
	return spacc_cipher(CIPH_ALG_AES, CIPH_MODE_CBC, 1,
	        str2, 32, dst, 32, NULL, 32, key_idx, tmp_iv, 16);
}
