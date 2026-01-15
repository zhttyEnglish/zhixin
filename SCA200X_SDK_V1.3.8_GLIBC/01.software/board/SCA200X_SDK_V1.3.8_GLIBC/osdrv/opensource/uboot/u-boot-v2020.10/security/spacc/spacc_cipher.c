/*
 * SmartChipc spacc driver for bootloader & uboot
 *
 * Support Algorithms:
 * AES CBC
 * SHA256, HMAC-SHA256
 */

#include <linux/string.h>
#include "spacc.h"
#include "spacc_hal.h"

/* 512k at most for each operation */
#define SPACC_MAX_LENGTH  (512 * 1024)

/*
 * Buffer data descriptor table, 8 bytes aligned
 * addr: buffer address
 * len:   buffer length
 */
struct spacc_ddt {
	unsigned int addr;
	unsigned int len;
} __attribute__((aligned(8)));

struct spacc_cipher_job {
	struct spacc_ddt src_ddt;
	struct spacc_ddt dst_ddt;

	unsigned long algo;
	unsigned long mode;                 // Encription Algorith mode
	unsigned long enc;

	unsigned long iv_len;
	unsigned long iv_offset;

	unsigned long proc_len;

	unsigned long key_sz;

	unsigned int key_ctx;
	unsigned int use_otp;
	unsigned char key[64];
};

static struct spacc_cipher_job job;

void spacc_driver_init(void)
{
	//Disable interrupt
	spacc_write_reg32(SPACC_IRQ_EN, 0x0);
}

static void spacc_cipher_set_algorithm(unsigned int algo, unsigned int mode, int encrypt)
{
	job.algo = algo;
	job.mode = mode;
	job.enc = encrypt;
}

static void spacc_cipher_set_input(unsigned char *src, unsigned int length)
{
	job.src_ddt.addr = (unsigned int)(ulong)src;
	job.src_ddt.len = length;
	job.proc_len = length;
}

static void spacc_cipher_set_output(unsigned char *dst, unsigned int length)
{
	job.dst_ddt.addr = (unsigned int)(ulong)dst;
	job.dst_ddt.len = length;
}

static int spacc_cipher_set_key(unsigned char *key, unsigned int length, unsigned int ctx)
{
	unsigned int value = 0;
	int i = 0;

	//Only support AES
	if(job.algo != CIPH_ALG_AES)
		return 1;

	job.key_sz = length;
	job.key_ctx = SPACC_KEY_CONTEXT_IDX_0 + ctx;

	//If use OTP root, set cipher context to zero
	//Or move key to cipher context
	if(!key) {
		job.use_otp = 1;
	}

	//Set key size register
	value = job.key_sz;
	value = spacc_set_bit(value, KEY_SIZE_CIPHER_BIT);
	value = spacc_set_bits(value, KEY_SIZE_CTX_IDX_SHIFT, job.key_ctx);
	spacc_write_reg32(SPACC_KEY_SZ, value);

	if(key) {
		memcpy((unsigned char *)SPACC_CIPH_CTX_0 + SPACC_CIPH_CTX_SZ * job.key_ctx, key, length);
		for(i = 0; i < length; i++)
			job.key[i] = key[i];
	} else {
		memset((unsigned char *)SPACC_CIPH_CTX_0 + SPACC_CIPH_CTX_SZ * job.key_ctx, 0, length);
	}

	return 0;
}

static int spacc_cipher_set_iv(unsigned char *iv, unsigned long length, unsigned long offs)
{
	//Only support AES
	if(job.algo != CIPH_ALG_AES)
		return 1;

	memcpy((void *)(ulong)(SPACC_CIPH_CTX_0 + SPACC_CIPH_CTX_SZ * job.key_ctx + AES_IV_OFFSET), iv, length);

	job.iv_len = length;
	job.iv_offset = offs;
	return 0;
}

static void spacc_cipher_get_iv(unsigned char *iv, unsigned long length, unsigned long offs)
{
	memcpy(iv, (void *)(ulong)(SPACC_CIPH_CTX_0 + SPACC_CIPH_CTX_SZ * job.key_ctx + AES_IV_OFFSET), length);
}

static void spacc_start_cipher_operation(void)
{
	unsigned int value;

	//Set src & dst register
	spacc_write_reg32(SPACC_SRC_PTR, (unsigned int)(ulong)(&job.src_ddt));
	spacc_write_reg32(SPACC_DST_PTR, (unsigned int)(ulong)(&job.dst_ddt));

	//Set 0 to below registers
	spacc_write_reg32(SPACC_AAD_OFFSET, 0x0);
	spacc_write_reg32(SPACC_PRE_AAD_LEN, 0x0);
	spacc_write_reg32(SPACC_POST_AAD_LEN, 0x0);

	//Set PROC len register
	spacc_write_reg32(SPACC_PROC_LEN, job.proc_len);

	spacc_write_reg32(SPACC_ICV_LEN, 0x0);
	spacc_write_reg32(SPACC_ICV_OFFSET, 0x0);

	//Set IV register
	value = job.iv_offset;
	value = spacc_clear_bit(value, IV_OFFSET_ENABLE_BIT);
	spacc_write_reg32(SPACC_IV_OFFSET, value);

	//Set SW register
	if(job.enc)
		spacc_write_reg32(SPACC_SW_CTRL, 0x1);
	else
		spacc_write_reg32(SPACC_SW_CTRL, 0x2);

	spacc_write_reg32(SPACC_AUX_INFO, 0x0);

	//Set ctrl register
	value = 0;
	if(job.enc)
		value = spacc_set_bit(value, ENCRYPT_BIT);
	value = spacc_set_bits(value, CIPH_ALG_SHIFT, job.algo);
	value = spacc_set_bits(value, CIPH_MODE_SHIFT, job.mode);
	if(job.use_otp) {
		value = spacc_set_bit(value, SEC_KEY_BIT);
	}
	value = spacc_set_bits(value, CTX_IDX_SHIFT, job.key_ctx);

	value = spacc_set_bit(value, MSG_BEGIN_BIT);
	value = spacc_set_bit(value, MSG_END_BIT);

	//Set key expand bit
	value = spacc_set_bit(value, KEY_EXP_BIT);

	spacc_write_reg32(SPACC_CTRL, value);
}

static void spacc_pop(void)
{
	spacc_write_reg32(SPACC_STAT_POP, POP_VALUE);
}

static unsigned int spacc_polling_for_finish(void)
{
	unsigned int fifo_stat = 0x0;
	unsigned int value = 0;

	while(fifo_stat == 0x0) {
		fifo_stat = spacc_read_reg32(SPACC_FIFO_STAT);
		fifo_stat = spacc_get_bits(fifo_stat, FIFO_STAT_CNT_SHIFT, FIFO_STAT_CNT_LEN);
	}

	value = spacc_read_reg32(SPACC_STATUS);
	value = spacc_get_bits(value, STATUS_RET_CODE_SHIFT, STATUS_RET_CODE_LEN);

	return value;
}

unsigned int __spacc_cipher(unsigned int algo, unsigned int mode, unsigned int encrypt,
    unsigned char *src, unsigned int src_len,
    unsigned char *dst, unsigned int dst_len,
    unsigned char *key, unsigned int key_len,
    unsigned int ctx,
    unsigned char *iv, unsigned int iv_len)
{
	memset(&job, 0, sizeof(struct spacc_cipher_job));

	spacc_cipher_set_algorithm(algo, mode, encrypt);
	spacc_cipher_set_input(src, src_len);
	spacc_cipher_set_output(dst, dst_len);
	spacc_cipher_set_key(key, key_len, ctx);
	spacc_cipher_set_iv(iv, iv_len, 0x20);

	spacc_start_cipher_operation();

	spacc_polling_for_finish();

	spacc_pop();

	spacc_cipher_get_iv(iv, iv_len, 0x20);

	return 0;
}

unsigned int spacc_cipher(unsigned int algo, unsigned int mode, unsigned int encrypt,
    unsigned char *src, unsigned int src_len,
    unsigned char *dst, unsigned int dst_len,
    unsigned char *key, unsigned int key_len,
    unsigned int ctx,
    unsigned char *iv, unsigned int iv_len)
{
	int ret, tmp_len, tmp_len1, tmp_len2;
	unsigned char iv_tmp[32];

	memcpy(iv_tmp, iv, iv_len);

	while(src_len) {
		tmp_len1 = SPACC_MAX_LENGTH - ((unsigned long)src & (SPACC_MAX_LENGTH - 1));
		tmp_len2 = SPACC_MAX_LENGTH - ((unsigned long)dst & (SPACC_MAX_LENGTH - 1));

		tmp_len = tmp_len1 < tmp_len2 ? tmp_len1 : tmp_len2;
		if(tmp_len > src_len)
			tmp_len = src_len;

		ret = __spacc_cipher(algo, mode, encrypt,
		        src, tmp_len,
		        dst, tmp_len,
		        key, key_len,
		        ctx,
		        iv_tmp, iv_len);
		if(ret < 0)
			break;

		src += tmp_len;
		dst += tmp_len;
		src_len -= tmp_len;
	}

	return ret;
}

#ifdef SPACC_TEST
static unsigned char *src = 0x80000000;
static unsigned char *dst1 = 0x80001000;
static unsigned char *dst2 = 0x80002000;
static unsigned char *dst3 = 0x80003000;
static unsigned char *dst4 = 0x80004000;
static unsigned char *dst5 = 0x80005000;

static unsigned char *key1 = 0x80006000;
static unsigned char *key2 = 0x80007000;
static unsigned char *iv = 0x80008000;

void spacc_cipher_test256(void)
{
	int i, outlen = ~0, ret = 0;
	struct spacc_cipher_job job = {0};

	serial_puts(0, "\nTest SPAcc Enc\n");

	for(i = 0; i < 128; i++) {
		src[i] = 'A' + i / 16;
		dst1[i] = '\0';
		dst2[i] = '\0';
		dst3[i] = '\0';
		dst4[i] = '\0';
	}

	for(i = 0; i < 32; i++)
		key1[i] = 0x20 + i;
	for(i = 0; i < 32; i++)
		key2[i] = 0x00 + i;

	for(i = 0; i < 16; i++)
		iv[i] = 'D';

	spacc_driver_init();

	//---------------------------test 1-------------------------------
	//Encrypt
	spacc_cipher(CIPH_ALG_AES, CIPH_MODE_CBC, 1,
	    src, 128,
	    dst1, 128,
	    key1, 32,
	    0,
	    iv, 16);
	serial_puts(0, "test1, after enc\n");
	for(i = 0; i < 8; i++) {
		print_str(0, *(unsigned int *)(dst1 + i * 16 + 0x0));
		print_str(0, *(unsigned int *)(dst1 + i * 16 + 0x4));
		print_str(0, *(unsigned int *)(dst1 + i * 16 + 0x8));
		print_str(0, *(unsigned int *)(dst1 + i * 16 + 0xc));
		serial_puts(0, "\n");
	}
	//Decrypt
	spacc_cipher(CIPH_ALG_AES, CIPH_MODE_CBC, 0,
	    dst1, 128,
	    dst2, 128,
	    NULL, 32,
	    0,
	    iv, 16);
	serial_puts(0, "test1, after dec\n");
	for(i = 0; i < 8; i++) {
		print_str(0, *(unsigned int *)(dst2 + i * 16 + 0x0));
		print_str(0, *(unsigned int *)(dst2 + i * 16 + 0x4));
		print_str(0, *(unsigned int *)(dst2 + i * 16 + 0x8));
		print_str(0, *(unsigned int *)(dst2 + i * 16 + 0xc));
		serial_puts(0, "\n");
	}

	//---------------------------test 2-------------------------------
	//Encrypt
	spacc_cipher(CIPH_ALG_AES, CIPH_MODE_CBC, 1,
	    src, 128,
	    dst1, 128,
	    key2, 32,
	    1,
	    iv, 16);
	serial_puts(0, "test2, after enc\n");
	for(i = 0; i < 8; i++) {
		print_str(0, *(unsigned int *)(dst1 + i * 16 + 0x0));
		print_str(0, *(unsigned int *)(dst1 + i * 16 + 0x4));
		print_str(0, *(unsigned int *)(dst1 + i * 16 + 0x8));
		print_str(0, *(unsigned int *)(dst1 + i * 16 + 0xc));
		serial_puts(0, "\n");
	}

	//Decrypt
	spacc_cipher(CIPH_ALG_AES, CIPH_MODE_CBC, 0,
	    dst1, 128,
	    dst2, 128,
	    NULL, 32,
	    1,
	    iv, 16);
	serial_puts(0, "test2, after dec\n");
	for(i = 0; i < 8; i++) {
		print_str(0, *(unsigned int *)(dst2 + i * 16 + 0x0));
		print_str(0, *(unsigned int *)(dst2 + i * 16 + 0x4));
		print_str(0, *(unsigned int *)(dst2 + i * 16 + 0x8));
		print_str(0, *(unsigned int *)(dst2 + i * 16 + 0xc));
		serial_puts(0, "\n");
	}

	return 0;
}
#endif

