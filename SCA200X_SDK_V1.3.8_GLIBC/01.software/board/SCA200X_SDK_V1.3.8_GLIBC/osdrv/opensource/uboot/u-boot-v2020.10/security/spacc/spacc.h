#ifndef _SPACC_H_
#define _SPACC_H_

//CBC CS mode setting
#define CBC_CS_SEL_NULL           0
#define CBC_CS_SEL_1              1
#define CBC_CS_SEL_2              2
#define CBC_CS_SEL_3              3

//Cipher algorithm setting
#define CIPH_ALG_NULL             0
#define CIPH_ALG_DES              1
#define CIPH_ALG_AES              2
#define CIPH_ALG_RC4              3
#define CIPH_ALG_MULTI2           4
#define CIPH_ALG_KASUMI           5
#define CIPH_ALG_SNOW             6
#define CIPH_ALG_ZUC              7
#define CIPH_ALG_EE3              128

//Cipher mode setting
#define CIPH_MODE_ECB             0
#define CIPH_MODE_CBC             1
#define CIPH_MODE_CTR             2
#define CIPH_MODE_CCM             3
#define CIPH_MODE_GCM             5
#define CIPH_MODE_OFB             7
#define CIPH_MODE_CFB             8
#define CIPH_MODE_F8              9
#define CIPH_MODE_XTS             10

//Hash algorithm setting
#define HASH_ALG_NULL             0
#define HASH_ALG_MD5              1
#define HASH_ALG_SHA1             2
#define HASH_ALG_SHA224           3
#define HASH_ALG_SHA256           4
#define HASH_ALG_SHA384           5
#define HASH_ALG_SHA512           6
#define HASH_ALG_AES_XCBC_MAC     7
#define HASH_ALG_AES_CMAC         8
#define HASH_ALG_KASUMI_F9        9
#define HASH_ALG_SNOW             10
#define HASH_ALG_CRC32            11
#define HASH_ALG_ZUC              12
#define HASH_ALG_SHA512_224       13
#define HASH_ALG_SHA512_256       14
#define HASH_ALG_MICHAEL          15
#define HASH_ALG_SHA3_224         16
#define HASH_ALG_SHA3_256         17
#define HASH_ALG_SHA3_384         18
#define HASH_ALG_SHA3_512         19

//Hash mode settings
#define HASH_MODE_RAW             0
#define HASH_MODE_SSLMAC          1
#define HASH_MODE_HMAC            2

//Error defination
#define SPACC_ERR_OK              0
#define SPACC_ERR_ICV_FAIL        1
#define SPACC_ERR_MEMORY_ERROR    2
#define SPACC_ERR_BLOCK_ERROR     3
#define SPACC_ERR_SECURITY_ERROR  4

unsigned int spacc_cipher(unsigned int algo, unsigned int mode, unsigned int encrypt,
    unsigned char *src, unsigned int src_len,
    unsigned char *dst, unsigned int dst_len,
    unsigned char *key, unsigned int key_len,
    unsigned int otpidx,
    unsigned char *iv, unsigned int iv_len);

unsigned int spacc_hash(unsigned int algo, unsigned int mode,
    unsigned char *src, unsigned int src_len,
    unsigned char *dst, unsigned int dst_len,
    unsigned char *key, unsigned int key_len);
#endif
