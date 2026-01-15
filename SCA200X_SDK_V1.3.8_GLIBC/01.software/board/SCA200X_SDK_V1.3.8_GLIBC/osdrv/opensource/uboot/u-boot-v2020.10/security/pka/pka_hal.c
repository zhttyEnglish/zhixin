/*
 *SmartChip pka driver
 *Support RSA operations
 */
#include <common.h>
#include <watchdog.h>

#include "pka_hal.h"
#include "pka.h"

struct pka_job {
	unsigned char *n, *e, *d;
	unsigned char *src, *dst;
	unsigned long n_len;
	unsigned long e_len;
	unsigned long d_len;
	unsigned long src_len;
	unsigned long dst_len;
	unsigned long type;
	unsigned long radix;
};

static void pka_driver_init(void)
{
	unsigned value = 0;

	//Enable interrupt
	value = pka_set_bit(value, IRQ_EN_IE_BIT);
	pka_write_reg32(PKA_IRQ_EN, value);

	//Set build config
	value = 0x80049801;//Don't modify it
	pka_write_reg32(PKA_BUILD_CONFIG, value);
}

static int pka_polling_for_finish(void)
{
	unsigned int value;

	value = pka_read_reg32(PKA_RTN_CODE);

	while((pka_get_bit(value, RTN_CODE_IRO_BIT)) == 0) {
		value = pka_read_reg32(PKA_RTN_CODE);
		WATCHDOG_RESET();
	}

	value = pka_get_bits(value, RTN_CODE_STOP_REASON_SHIFT, RTN_CODE_STOP_REASON_LEN);

	pka_write_reg32(PKA_STAT, 1 << STAT_DONE_BIT);

	return value;
}

#ifdef DMA
static void pka_move_data(unsigned long dst, unsigned long src, unsigned long len)
{
}
#else
static void pka_move_data(unsigned long dst, unsigned long src, unsigned long len)
{
	int i;
	unsigned int *p1 = (unsigned int *)dst;
	unsigned int *p2 = (unsigned int *)src;

	for(i = 0; i < len / 4; i++)
		*p1++ = *p2++;
}
#endif

//Calc r^-1, and result is store in register C0
static int pka_calc_r_inv(struct pka_job *job)
{
	unsigned int value = 0;

	//Move modulus to region D
	pka_move_data((unsigned long)pka_region_D_BaseAddress, (unsigned long)job->n, job->n_len);

	printf("pka caculate r_inv:\n");

	//Write stack pointer register
	pka_write_reg32(PKA_STACK_PNTR, 0x0);
	//Write flag register
	pka_write_reg32(PKA_FLAGS, 0x0);
	//Write entry register, enter calc_r_inv
	pka_write_reg32(PKA_ENTRY_PNT, ENTRY_CALC_R_INV);

	//Set RSA radix bits, and start operation
	value = pka_set_bit(value, CTRL_GO_BIT);
	value = pka_set_bits(value, CTRL_BASE_RADIX_SHIFT, job->radix);
	pka_write_reg32(PKA_CTRL, value);

	// wait for finish
	return pka_polling_for_finish();
}

//Calc m', and result is store in register D1
static int pka_calc_mp(struct pka_job *job)
{
	unsigned int value = 0;

	printf("pka caculate mp:\n");

	pka_write_reg32(PKA_FLAGS, 0x0);
	pka_write_reg32(PKA_ENTRY_PNT, ENTRY_CALC_MP);  // entry of calc_mp

	//Set RSA radix bits, and start operation
	value = pka_set_bit(value, CTRL_GO_BIT);
	value = pka_set_bits(value, CTRL_BASE_RADIX_SHIFT, job->radix);
	pka_write_reg32(PKA_CTRL, value);

	// wait for finish
	return pka_polling_for_finish();
}

//Calc R^2, and result is store in register D3
static int pka_calc_r_square(struct pka_job *job)
{
	unsigned int value = 0;

	printf("pka caculate r_squar:\n");

	pka_write_reg32(PKA_FLAGS, 0x00000000);
	pka_write_reg32(PKA_ENTRY_PNT, 0x00000012);  // entry of calc_r_sqr
	pka_write_reg32(PKA_IRQ_EN, 0x40000000);

	//Set RSA radix bits, and start operation
	value = pka_set_bit(value, CTRL_GO_BIT);
	value = pka_set_bits(value, CTRL_BASE_RADIX_SHIFT, job->radix);
	pka_write_reg32(PKA_CTRL, value);

	// wait for finish
	return pka_polling_for_finish();
}

/*
 *n: modulus
 *e: exponent
 *radix: rsa radix, for example, radix=2048, RSA2048 operation
 */
int pka_rsa_expmod(unsigned char *src, unsigned long src_len,
    unsigned char *dst, unsigned long *dst_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *e, unsigned long e_length,
    unsigned long radix)
{
	struct pka_job job;
	unsigned int value = 0;
	int ret;
	int keysize;// key size in byte

	switch(radix) {
	case RSA_256:
		keysize = 256;
		break;
	case RSA_512:
		keysize = 512;
		break;
	case RSA_1024:
		keysize = 1024;
		break;
	case RSA_2048:
		keysize = 2048;
		break;
	default:
		break;
	};

	keysize = keysize / 8;

	memset(&job, 0, sizeof(struct pka_job));
	job.n = n;
	job.e = e;
	job.src = src;
	job.dst = dst;
	job.n_len = n_length;
	job.e_len = e_length;
	job.src_len = src_len;
	job.dst_len = *dst_len;
	job.type = RSA_ENC;
	job.radix = radix;

	ret = pka_calc_r_inv(&job);
	if(ret)
		return ret;

	ret = pka_calc_mp(&job);
	if(ret)
		return ret;

	ret = pka_calc_r_square(&job);
	if(ret)
		return ret;

	//Clear D2
	memset((void *)(ulong)(pka_region_D_BaseAddress + 2 * keysize), 0, keysize);

	//Move public exponent to D2
	pka_move_data(pka_region_D_BaseAddress + 2 * keysize, (unsigned long)(job.e), job.e_len);

	//Move data before decrypt to A0
	pka_move_data(pka_region_A_BaseAddress, (unsigned long)job.src, job.src_len);

	//do RSA encryption
	pka_write_reg32(PKA_FLAGS, 0x0);
	pka_write_reg32(PKA_ENTRY_PNT, ENTRY_MODEXP);
	pka_write_reg32(PKA_JMP_PROB, 0x0);

	//Set RSA radix bits, and start operation
	value = 0;
	value = pka_set_bit(value, CTRL_GO_BIT);
	value = pka_set_bits(value, CTRL_BASE_RADIX_SHIFT, job.radix);
	pka_write_reg32(PKA_CTRL, value);

	// wait for finish
	ret = pka_polling_for_finish();
	if(ret)
		return ret;

	//Move data after encrypt from A0 to dst buffer
	pka_move_data((ulong)job.dst, pka_region_A_BaseAddress, job.dst_len);

	return 0;
}

/*
 *n: modulus
 *e: public exponent
 *radix: rsa radix, for example, radix=2048, RSA2048 operation
 */
int pka_rsa_encrypt(unsigned char *src, unsigned long src_len,
    unsigned char *dst, unsigned long *dst_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *e, unsigned long e_length,
    unsigned long radix)
{
	return pka_rsa_expmod(src, src_len, dst, dst_len,
	        n, n_length, e, e_length, radix);
}

/*
 *n: modulus
 *d: private exponent
 */
int pka_rsa_decrypt(unsigned char *src, unsigned long src_len,
    unsigned char *dst, unsigned long *dst_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *d, unsigned long d_length,
    unsigned long radix)
{
	return pka_rsa_expmod(src, src_len, dst, dst_len,
	        n, n_length, d, d_length, radix);

}

/*
 *use private key to sign data
 *n: modulus
 *d: private exponent
 */
int pka_rsa_sign(unsigned char *src, unsigned long src_len,
    unsigned char *dst, unsigned long *dst_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *d, unsigned long d_length,
    unsigned int radix)
{
	return pka_rsa_expmod(src, src_len, dst, dst_len,
	        n, n_length, d, d_length, radix);
}

/*
 *use public key to verify data
 *n: modulus
 *d: private exponent
 */
int pka_rsa_verify(unsigned char *sig, unsigned long sig_len,
    unsigned char *dst, unsigned long dst_len,
    unsigned char *img, unsigned long img_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *e, unsigned long e_length,
    unsigned int radix)
{
	int ret, i;

	pka_driver_init();

	//decrypt data with public key
	ret = pka_rsa_expmod(sig, sig_len,
	        dst, &dst_len,
	        n, n_length,
	        e, e_length,
	        radix);
	//decrypt fail
	if(ret)
		return ret;

	//Compare original data and decryted data
	for(i = 0; i < img_len; i++) {
		if(img[i] != dst[i])
			return 1;
	}

	//Verify success
	return 0;
}

//#define PKA_TEST
#ifdef PKA_TEST
unsigned char *src  = 0x30000000;
unsigned char *dst1 = 0x30010000;
unsigned char *dst2 = 0x30020000;

extern unsigned char external_rsa_n[256];
extern unsigned char external_rsa_d[256];
extern unsigned char external_rsa_e[4];

void pka_test_rsa2048(void)
{
	int i, outlen = 256;
	int ret;

	serial_puts(1, "\nxhzhu rsa\n");

	pka_driver_init();

#if 1
	for(i = 0; i < 256; i++) {
		src[i] = 'B';
		dst1[i] = '\0';
		dst2[i] = '\0';
	}

	for(i = 0; i < 240; i++) {
		src[i] = 'A';
	}
#endif

#if 0
	pka_rsa_encrypt(src, 256,
	    dst1, &outlen,
	    &external_rsa_n, 256,
	    &external_rsa_e, 4,
	    RSA_2048);

	serial_puts(1, "\nafter enc\n");
	print_str(1, *(int *)(dst1 + 0x0));
	print_str(1, *(int *)(dst1 + 0x4));
	print_str(1, *(int *)(dst1 + 0x8));
	print_str(1, *(int *)(dst1 + 0xc));
	print_str(1, *(int *)(dst1 + 256 - 0x10));
	print_str(1, *(int *)(dst1 + 256 - 0xc));
	print_str(1, *(int *)(dst1 + 256 - 0x8));
	print_str(1, *(int *)(dst1 + 256 - 0x4));

	outlen = 256;

	pka_rsa_decrypt(dst1, 256,
	    dst2, &outlen,
	    &external_rsa_n, 256,
	    &external_rsa_d, 256,
	    RSA_2048);

	serial_puts(1, "\nafter dec\n");
	print_str(1, *(int *)(dst2 + 0x0));
	print_str(1, *(int *)(dst2 + 0x4));
	print_str(1, *(int *)(dst2 + 0x8));
	print_str(1, *(int *)(dst2 + 0xc));
	print_str(1, *(int *)(dst2 + 256 - 0x10));
	print_str(1, *(int *)(dst2 + 256 - 0xc));
	print_str(1, *(int *)(dst2 + 256 - 0x8));
	print_str(1, *(int *)(dst2 + 256 - 0x4));
#endif

#if 1
	pka_rsa_sign(src, 256,
	    dst1, &outlen,
	    &external_rsa_n, 256,
	    &external_rsa_d, 256,
	    RSA_2048);

	serial_puts(1, "\nafter sign\n");
	print_str(1, *(int *)(dst1 + 0x0));
	print_str(1, *(int *)(dst1 + 0x4));
	print_str(1, *(int *)(dst1 + 0x8));
	print_str(1, *(int *)(dst1 + 0xc));
	print_str(1, *(int *)(dst1 + 256 - 0x10));
	print_str(1, *(int *)(dst1 + 256 - 0xc));
	print_str(1, *(int *)(dst1 + 256 - 0x8));
	print_str(1, *(int *)(dst1 + 256 - 0x4));

	outlen = 256;

	ret = pka_rsa_verify(dst1, 256,
	        dst2, outlen,
	        src, 256,
	        &external_rsa_n, 256,
	        &external_rsa_e, 4,
	        RSA_2048);

	serial_puts(1, "\nafter verify, ret=\n");
	print_str(1, ret);
	serial_puts(1, "\n");
	print_str(1, *(int *)(dst2 + 0x0));
	print_str(1, *(int *)(dst2 + 0x4));
	print_str(1, *(int *)(dst2 + 0x8));
	print_str(1, *(int *)(dst2 + 0xc));
	print_str(1, *(int *)(dst2 + 256 - 0x10));
	print_str(1, *(int *)(dst2 + 256 - 0xc));
	print_str(1, *(int *)(dst2 + 256 - 0x8));
	print_str(1, *(int *)(dst2 + 256 - 0x4));
#endif

}
#endif
