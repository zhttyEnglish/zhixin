#ifndef _PKA_H_
#define _PKA_H_

enum {
	RSA_256 = 2,
	RSA_512 = 3,
	RSA_1024 = 4,
	RSA_2048 = 5,
	RSA_4096 = 6,
};

enum {
	RSA_ENC = 0,
	RSA_DEC = 1,
	RSA_SIG = 2,
	RSA_VER = 3,
};

int pka_rsa_encrypt(unsigned char *src, unsigned long src_len,
    unsigned char *dst, unsigned long *dst_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *e, unsigned long e_length,
    unsigned long radix);

int pka_rsa_decrypt(unsigned char *src, unsigned long src_len,
    unsigned char *dst, unsigned long *dst_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *d, unsigned long d_length,
    unsigned long radix);

int pka_rsa_sign(unsigned char *src, unsigned long src_len,
    unsigned char *dst, unsigned long *dst_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *d, unsigned long d_length,
    unsigned int radix);

int pka_rsa_verify(unsigned char *sig, unsigned long sig_len,
    unsigned char *dst, unsigned long dst_len,
    unsigned char *img, unsigned long img_len,
    unsigned char *n, unsigned long n_length,
    unsigned char *e, unsigned long e_length,
    unsigned int radix);
#endif
