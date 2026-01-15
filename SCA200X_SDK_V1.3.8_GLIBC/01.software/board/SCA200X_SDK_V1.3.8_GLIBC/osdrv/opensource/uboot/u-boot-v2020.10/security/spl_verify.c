/*
 * SmartChip image verify program
 *
 * 1. verify image header
 * 2. verify hash and signature
 * 3. decrypt encrypted image
 */

#include <asm/byteorder.h>
#include <common.h>
#include "pka/pka.h"
#include "spacc/spacc.h"
#include <u-boot/sha256.h>
#include <asm/arch/image.h>

#include <fs.h>

#include <smartchip/spl_verify.h>

extern int gen_derived_key(unsigned char *str1, unsigned char *str2,
    unsigned char *iv, unsigned char iv_len,
    unsigned char *dst, unsigned int key_len,
    int key_idx);

static unsigned char calc_hash[32] __attribute__((aligned(4)))   = {0};
static unsigned char calc_sign[256] __attribute__((aligned(4)))  = {0};

static const unsigned char decrypt_iv[17] __attribute__((aligned(4)))  = "SC_SIGN_IV_0";

static unsigned char derive_key[32] __attribute__((aligned(4)))  = {0};
static const unsigned char derive_iv[17] __attribute__((aligned(4)))   = "SC_DERIVE_IV";
static const unsigned char derive_str1[33] __attribute__((aligned(4))) = "SC_DERIVE_STRING1_0123456789";
static const unsigned char derive_str2[33] __attribute__((aligned(4))) = "SC_DERIVE_STRING2_0123456789";

static int key_derived = 0;

extern unsigned char external_rsa_n[256];
extern unsigned char external_rsa_d[256];
extern unsigned char external_rsa_e[4];

static void reverse_buffer(unsigned char *buf, int len)
{
	int start = 0, end = len - 1;
	unsigned char tmp;

	while(start < end) {
		tmp = buf[start];
		buf[start] = buf[end];
		buf[end] = tmp;
		start++;
		end--;
	}
}

/*void print_buf(unsigned char *buf, int len)
{
    int i = 0;

    for(i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if((i + 1) % 16 == 0 || i == len - 1)
            printf("\n");
    }
}*/

int spl_verify_sw(unsigned char *hash, unsigned long hash_len,
    unsigned char *sig, unsigned long sig_len,
    unsigned char *img, unsigned long img_len)
{
	//int ret = 0;
	int i = 0;
	//int out_len = 32;
	//int div, mod;
	//unsigned long long img_reallen;
	//struct upgrade_hdr *hdr;
	char sig_buf[256], hash_buf[32];

	if(hash_len != 32 || sig_len != 256 ) {
		printf("Bad size\n");
		return -1;
	}

	memcpy(sig_buf, sig, 256);
	memcpy(hash_buf, hash, 32);

	sha256_context ctx;
	sha256_starts(&ctx);
	//if (single_upgrade)
	sha256_update(&ctx, img, img_len);
#if 0
	else {
		loff_t act_read = 0;
		int n = sizeof(struct upgrade_hdr) + 32 + 256;
		hdr = (struct upgrade_hdr *)IMAGE_LOAD_ADDR;
		img_reallen = __le64_to_cpu(hdr->img_size);

		/*
		 * read UPGRADE_UNIT already, but sha256 don't include upgrade_hdr,
		 * sha256, signature
		 */
		sha256_update(&ctx, img, UPGRADE_UNIT - n);

		div = (img_reallen - UPGRADE_UNIT + n) / UPGRADE_UNIT;
		mod = (img_reallen - UPGRADE_UNIT + n) % UPGRADE_UNIT;
		for (i = 0; i < div; i++) {
			ret = fs_set_usb_dev();
			if (ret)
				return ret;

			ret = fs_read(upgradefilename, IMAGE_LOAD_ADDR + UPGRADE_UNIT, UPGRADE_UNIT * (i + 1), UPGRADE_UNIT, &act_read);
			if (ret) {
				printf("boot_upgrade: read file fail %s", upgradefilename);
				return -1;
			} else {
				printf("boot_upgrade: act_read=0x%llx \n", act_read);
				sha256_update(&ctx, (const uint8_t *)IMAGE_LOAD_ADDR + UPGRADE_UNIT, UPGRADE_UNIT);
			}
		}

		if (mod) {
			ret = fs_set_usb_dev();
			if (ret)
				return ret;

			ret = fs_read(upgradefilename, IMAGE_LOAD_ADDR + UPGRADE_UNIT, UPGRADE_UNIT * (i + 1), mod, &act_read);
			if (ret) {
				printf("boot_upgrade: read file fail %s", upgradefilename);
				return -1;
			} else {
				printf("boot_upgrade: act_read=0x%llx \n", act_read);
				sha256_update(&ctx, (const uint8_t *)IMAGE_LOAD_ADDR + UPGRADE_UNIT, mod);
			}
		}
	}
#endif
	sha256_finish(&ctx, calc_hash);

	/*printf("Original hash:\n");
	for(i = 0; i < 32; i++) {
	    printf("%02x ", hash_buf[i]);
	    if((i + 1) % 16 == 0)
	        printf("\n");
	}

	printf("Calculated hash\n");
	for(i = 0; i < 32; i++) {
	    printf("%02x ", calc_hash[i]);
	    if((i+1) % 16 == 0)
	        printf("\n");
	}*/

	i = 0;

	//Compare hash
	while(i < hash_len) {
		if(calc_hash[i] != hash_buf[i]) {
			printf("Hash verify Fail\n");
			return -1;
		}
		i++;
	}

	printf("Hash verify success\n");

	reverse_buffer((unsigned char *)sig_buf, sig_len);
	reverse_buffer((unsigned char *)hash_buf, hash_len);

	return pka_rsa_verify((unsigned char *)sig_buf, sig_len,
	        (unsigned char *)calc_sign, 256,
	        (unsigned char *)hash_buf, hash_len,
	        (unsigned char *)&external_rsa_n, 256,
	        (unsigned char *)&external_rsa_e, 4,
	        RSA_2048);
}

int spl_verify(unsigned char *hash, unsigned long hash_len,
    unsigned char *sig, unsigned long sig_len,
    unsigned char *img, unsigned long img_len)
{
	int ret = 0, i = 0;
	int out_len = 32;

	if(hash_len != 32 || sig_len != 256 || img_len % 32 != 0) {
		printf("Bad size\n");
		return -1;
	}

	//Calculate Hash of the image
	ret = spacc_hash(HASH_ALG_SHA256, HASH_MODE_RAW,
	        img, img_len,
	        calc_hash, out_len,
	        NULL, 0);
	if(ret) {
		printf("Calculate Hash fail 0x%x\n", ret);
		return -1;
	}

	/*printf("Original hash:\n");
	for(i = 0; i < 32; i++) {
	    printf("%02x ", hash[i]);
	    if((i + 1) % 16 == 0)
	        printf("\n");
	}

	printf("Calculated hash\n");

	for(i = 0; i < 32; i++) {
	    printf("%02x ", calc_hash[i]);
	    if((i+1) % 16 == 0)
	        printf("\n");
	}*/

	i = 0;

	//Compare hash
	while(i < hash_len) {
		if(calc_hash[i] != hash[i]) {
			printf("Hash verify Fail\n");
			return 1;
		}
		i++;
	}

	printf("Hash verify success\n");

	reverse_buffer((unsigned char *)sig, sig_len);
	reverse_buffer((unsigned char *)hash, hash_len);

	return pka_rsa_verify((unsigned char *)sig, sig_len,
	        (unsigned char *)calc_sign, 256,
	        (unsigned char *)hash, hash_len,
	        (unsigned char *)&external_rsa_n, 256,
	        (unsigned char *)&external_rsa_e, 4,
	        RSA_2048);
}

/*
 * Decrypt image
 * src: address of encrypted image
 * dst: address of decrypted image
 * len: image size
 */
int spl_decrypt(unsigned char *src, unsigned char *dst, int len)
{
	int ret, outlen = ~0;
	//int i;

	/* generate derived key */
	if(key_derived == 0) {
		ret = gen_derived_key((unsigned char *)derive_str1, (unsigned char *)derive_str2,
		        (unsigned char *)derive_iv, 16, derive_key, 32, 1);
		if(ret) {
			printf("Generate derived key fail\n");
			return ret;
		}
		key_derived = 1;
	}

	return spacc_cipher(CIPH_ALG_AES, CIPH_MODE_CBC, 0,
	        (unsigned char *)src, len,
	        (unsigned char *)dst, outlen,
	        derive_key, 32,
	        0,
	        (unsigned char *)decrypt_iv, 16);
}

