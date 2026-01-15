#ifndef __SCA200V100_IMAGE_H
#define __SCA200V100_IMAGE_H
#include <linux/types.h>
#include <image-sparse.h>
#include <asm/arch/boot.h>

#define UART_BOOT_OK   "CCCCCCCC"

/* sign header for secure
 * data struct:
 *   header[]   @64byte secure header
 *   hase_buf[] @header.hase_size
 *   sig_buf[]  @header.sig_len
 *   img[]      @header.img_len
 *
 * 64byte
 */
struct smartx_image_header {
	unsigned int magic;
	unsigned int version;
	unsigned int img_type; /* 0: uboot, 0x100000 optee */
	unsigned int img_load_addr; //actual entry_point after decryption
	unsigned int img_len;
	unsigned int algo;
	unsigned int hash_size;
	unsigned int sig_len;
	unsigned int checksum;
	unsigned int reserved[7];
};

/* @boot_info: [BOOTROM_DEVICE_QSPI_NOR ...]
 *    rom code use this field to stroe boot device and partition.
 */
typedef struct spl_header_info {
	uint32_t res[4];
	uint32_t baurdrate;
	uint32_t boot_info;

	uint32_t res1[6];
	uint32_t spl_dtb_offset;
} spl_header_info;

/* @return value: BOOTROM_DEVICE_QSPI_NOR ... */
static inline unsigned int get_spl_boot_dev(void)
{
	spl_header_info *pHeader = (spl_header_info *)SPL_HEADER_BUF_BASE;
	unsigned int boot_info = (pHeader->boot_info >> 16) & 0xffff;

	return boot_info;
}

/* return: bool */
static inline unsigned int spl_is_uart_boot(void)
{
	return (BOOTROM_DEVICE_UART == get_spl_boot_dev()) ? 1 : 0;
}

static inline unsigned int get_spl_boot_brate(void)
{
	spl_header_info *pHeader = (spl_header_info *)SPL_HEADER_BUF_BASE;

	return pHeader->baurdrate;
}

typedef struct {
	uint32_t size;
} boot_img_info;

static inline void *get_boot_image(void)
{
	return (void *)(long)BOOT_BACK_ADDR;
}

static inline uint32_t get_boot_image_size(void)
{
	boot_img_info *img_info = (boot_img_info *)(long)BOOT_BACK_INFO;

	return img_info->size;
}
#endif /* __SCA200V100_IMAGE_H */
