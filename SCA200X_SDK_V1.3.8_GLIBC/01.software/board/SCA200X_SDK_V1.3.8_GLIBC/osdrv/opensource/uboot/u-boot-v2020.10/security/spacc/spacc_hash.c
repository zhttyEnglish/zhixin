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
#include <watchdog.h>

/*
 * Buffer data descriptor table, 8 bytes aligned
 * addr: buffer address
 * len:   buffer length
 */
struct spacc_ddt {
	unsigned int addr;
	unsigned int len;
} __attribute__((aligned(8)));

struct spacc_hash_job {
	struct spacc_ddt src_ddt;
	struct spacc_ddt dst_ddt;

	unsigned long algo;
	unsigned long mode;

	unsigned long proc_len;

	unsigned long key_sz;

	unsigned int key_ctx;
};

#if 0
static void spacc_driver_init(void)
{
	//Disable interrupt
	spacc_write_reg32(SPACC_IRQ_EN, 0x0);
}
#endif

static void spacc_hash_set_algo(struct spacc_hash_job *job, unsigned int algo, unsigned int mode)
{
	job->algo = algo;
	job->mode = mode;
}

static int spacc_hash_set_key(struct spacc_hash_job *job, unsigned char *key, unsigned int length)
{
	unsigned int value = 0;
	int i;
	if(key) {
		job->key_sz = length;
		job->key_ctx = SPACC_KEY_CONTEXT_IDX_0;

		//Set key size register
		value = job->key_sz;
		value = spacc_set_bits(value, KEY_SIZE_CTX_IDX_SHIFT, job->key_ctx);
		spacc_write_reg32(SPACC_KEY_SZ, value);

		//Set key
		memcpy((unsigned char *)SPACC_HASH_CTX_0, key, length);
	} else {
		job->key_sz = 16;
		job->key_ctx = SPACC_KEY_CONTEXT_IDX_0;
		spacc_write_reg32(SPACC_KEY_SZ, 0x0);
		//Set key
		for(i = 0; i < 4; i++)
			spacc_write_reg32(SPACC_HASH_CTX_0 + 4 * i, 0x0);

	}
	return 0;
}

static void spacc_hash_set_input(struct spacc_hash_job *job, unsigned char *buffer, unsigned int length)
{
	job->src_ddt.addr = (unsigned long)buffer;
	job->src_ddt.len = length;
	job->proc_len = length;
}

static void spacc_hash_set_output(struct spacc_hash_job *job, unsigned char *buffer, unsigned int length)
{
	job->dst_ddt.addr = (unsigned long)buffer;
	job->dst_ddt.len = length;
}

static void spacc_start_hash_operation(struct spacc_hash_job *job)
{
	unsigned int value = 0;

	//Set src & dst register
	spacc_write_reg32(SPACC_SRC_PTR, (unsigned int)(ulong)(&job->src_ddt));
	spacc_write_reg32(SPACC_DST_PTR, (unsigned int)(ulong)(&job->dst_ddt));

	//Set 0 to below registers
	spacc_write_reg32(SPACC_AAD_OFFSET, 0x0);
	spacc_write_reg32(SPACC_PRE_AAD_LEN, job->proc_len);
	spacc_write_reg32(SPACC_POST_AAD_LEN, 0x0);

	//Set PROC len register
	spacc_write_reg32(SPACC_PROC_LEN, job->proc_len);

	spacc_write_reg32(SPACC_ICV_LEN, 0x0);
	spacc_write_reg32(SPACC_ICV_OFFSET, 0x0);

	//Set SW register
	spacc_write_reg32(SPACC_SW_CTRL, 0x5);

	//Set AUX register
	spacc_write_reg32(SPACC_AUX_INFO, 0x0);

	//Set IV register
	spacc_write_reg32(SPACC_IV_OFFSET, 0x0);

	//Set ctrl register
	value = spacc_set_bits(value, HASH_ALG_SHIFT, job->algo);
	value = spacc_set_bits(value, HASH_MODE_SHIFT, job->mode);
	value = spacc_set_bits(value, KEY_SIZE_CTX_IDX_SHIFT, job->key_ctx);
	value = spacc_set_bit(value, MSG_BEGIN_BIT);
	value = spacc_set_bit(value, MSG_END_BIT);
	value = spacc_set_bit(value, ENCRYPT_BIT);

	//Set key expand bit
	//value = spacc_set_bit(value, KEY_EXP_BIT);

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
		WATCHDOG_RESET();
	}

	value = spacc_read_reg32(SPACC_STATUS);
	value = spacc_get_bits(value, STATUS_RET_CODE_SHIFT, STATUS_RET_CODE_LEN);

	return value;
}

unsigned int spacc_hash(unsigned int algo, unsigned int mode,
    unsigned char *src, unsigned int src_len,
    unsigned char *dst, unsigned int dst_len,
    unsigned char *key, unsigned int key_len)
{
	int ret;
	struct spacc_hash_job job;

	memset(&job, 0, sizeof(struct spacc_hash_job));

	spacc_hash_set_algo(&job, algo, mode);
	spacc_hash_set_input(&job, src, src_len);
	spacc_hash_set_output(&job, dst, dst_len);
	spacc_hash_set_key(&job, key, key_len);

	spacc_start_hash_operation(&job);

	ret = spacc_polling_for_finish();

	spacc_pop();

	return ret;
}

#ifdef TEST_SPACC
static unsigned char *src = 0x80000000;
static unsigned char *dst1 = 0x30001f00;
static unsigned char *dst2 = 0x30002000;
static unsigned char *dst3 = 0x30003000;
static unsigned char *dst4 = 0x30004000;
static unsigned char *dst5 = 0x80005000;

static unsigned char *key = 0x30006000;
static unsigned char *iv = 0x30007000;

void spacc_hash_test_sha256(void)
{
	int i, ret, outlen = ~0;
	struct spacc_hash_job job = {0};

	for(i = 0; i < 128; i++) {
		src[i] = 'A';
		//dst3[i] = 0;
		dst4[i] = 0;
		dst5[i] = 0;
	}

	//------------------------------test1-------------------------------
	serial_puts(1, "\nBefore Hash\n");
	for(i = 0; i < 2; i++) {
		print_str(1, *(unsigned int *)(dst4 + i * 16 + 0x0));
		print_str(1, *(unsigned int *)(dst4 + i * 16 + 0x4));
		print_str(1, *(unsigned int *)(dst4 + i * 16 + 0x8));
		print_str(1, *(unsigned int *)(dst4 + i * 16 + 0xc));
		serial_puts(1, "\n");
	}

	ret = spacc_hash(HASH_ALG_SHA256, HASH_MODE_RAW,
	        src, 128,
	        dst4, 128,
	        NULL, 0);

	serial_puts(1, "\nAfter Hash\n");
	for(i = 0; i < 2; i++) {
		print_str(1, *(unsigned int *)(dst4 + i * 16 + 0x0));
		print_str(1, *(unsigned int *)(dst4 + i * 16 + 0x4));
		print_str(1, *(unsigned int *)(dst4 + i * 16 + 0x8));
		print_str(1, *(unsigned int *)(dst4 + i * 16 + 0xc));
		serial_puts(1, "\n");
	}

	//------------------------------test2-------------------------------
	serial_puts(1, "\nBefore Hash\n");
	for(i = 0; i < 2; i++) {
		print_str(1, *(unsigned int *)(dst5 + i * 16 + 0x0));
		print_str(1, *(unsigned int *)(dst5 + i * 16 + 0x4));
		print_str(1, *(unsigned int *)(dst5 + i * 16 + 0x8));
		print_str(1, *(unsigned int *)(dst5 + i * 16 + 0xc));
		serial_puts(1, "\n");
	}

	ret = spacc_hash(HASH_ALG_SHA256, HASH_MODE_RAW,
	        src, 128,
	        dst5, 128,
	        NULL, 0);

	serial_puts(1, "\nAfter Hash\n");
	for(i = 0; i < 2; i++) {
		print_str(1, *(unsigned int *)(dst5 + i * 16 + 0x0));
		print_str(1, *(unsigned int *)(dst5 + i * 16 + 0x4));
		print_str(1, *(unsigned int *)(dst5 + i * 16 + 0x8));
		print_str(1, *(unsigned int *)(dst5 + i * 16 + 0xc));
		serial_puts(1, "\n");
	}
}

void spacc_hash_test_hmac_sha256(void)
{
	int i, ret, outlen = 128;

	serial_puts(1, "\nxiaohui Hash hmac\n");

	for(i = 0; i < 128; i++) {
		src[i] = 'C';
		dst5[i] = '\0';
	}
	for(i = 0; i < 16; i++)
		key[i] = 'D';

	serial_puts(1, "\nBefore Hash\n");
	print_str(1, *(unsigned int *)(dst5 + 0x0));
	print_str(1, *(unsigned int *)(dst5 + 0x4));
	print_str(1, *(unsigned int *)(dst5 + 0x8));
	print_str(1, *(unsigned int *)(dst5 + 0xc));
	print_str(1, *(unsigned int *)(dst5 + 0x10));
	print_str(1, *(unsigned int *)(dst5 + 0x14));
	print_str(1, *(unsigned int *)(dst5 + 0x18));
	print_str(1, *(unsigned int *)(dst5 + 0x1c));
	print_str(1, *(unsigned int *)(dst5 + 0x20));
	serial_puts(1, "\n");

	ret = spacc_hash(HASH_ALG_SHA256, HASH_MODE_HMAC,
	        src, 128,
	        dst5, &outlen,
	        key, 16);

	serial_puts(1, "\nAfter Hash\n");
	print_str(1, *(unsigned int *)(dst5 + 0x0));
	print_str(1, *(unsigned int *)(dst5 + 0x4));
	print_str(1, *(unsigned int *)(dst5 + 0x8));
	print_str(1, *(unsigned int *)(dst5 + 0xc));
	print_str(1, *(unsigned int *)(dst5 + 0x10));
	print_str(1, *(unsigned int *)(dst5 + 0x14));
	print_str(1, *(unsigned int *)(dst5 + 0x18));
	print_str(1, *(unsigned int *)(dst5 + 0x1c));
	print_str(1, *(unsigned int *)(dst5 + 0x20));
	serial_puts(1, "\n");
}
#endif

