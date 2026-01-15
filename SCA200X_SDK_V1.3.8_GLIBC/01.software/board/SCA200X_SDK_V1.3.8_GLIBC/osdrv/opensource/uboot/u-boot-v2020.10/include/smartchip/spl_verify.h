#ifndef __SMARTCHIP_SPL_VERITY_H
#define __SMARTCHIP_SPL_VERITY_H

extern int spl_verify_sw(unsigned char *hash, unsigned long hash_len,
    unsigned char *sig, unsigned long sig_len,
    unsigned char *img, unsigned long img_len);

#endif /* __SMARTCHIP_SPL_VERITY_H */
