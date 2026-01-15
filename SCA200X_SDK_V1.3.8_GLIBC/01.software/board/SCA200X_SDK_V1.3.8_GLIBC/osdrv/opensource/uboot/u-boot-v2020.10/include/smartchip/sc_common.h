#ifndef __SMARTCHIP_COMMON_H
#define __SMARTCHIP_COMMON_H

#include <part_efi.h>
#include <part.h>
#include <u-boot/crc.h>
#include <uboot_upgrade.h>
#include <linux/mtd/mtd.h>
#include <asm/arch/image.h>

/*
 * uboot ddr layout:
 * | ddr base       | 0x20000000
 * | kernel         | 0x200a0000
 * | kernel dtb     | 0x22080000
 * | logo           | 0x22400000
 * | upgrade decomp | 0x23400000  the same with compressed kernel load address
 * | upgrade image  | 0x23900000
 * | gap            |
 * | uboot heap     | about ddr top - 17M
 * | uboot relocate | about ddr top - 1M
 * */

/* ahb dma cannot access secure sram, so relocate some address to ddr */
#define SPL_GPT0_ADDR           (IMAGE_DECOMPRESS_ADDR)
#define SPL_GPT1_ADDR           (IMAGE_DECOMPRESS_ADDR + 0x10000)
#define SPL_NAND_DATABUF_ADDR   (IMAGE_DECOMPRESS_ADDR + 0x10000)

#define KERNEL_COMP_SIZE        0x800000
#define KERNEL_LOAD_ADDR        IMAGE_DECOMPRESS_ADDR
#define KERNEL_DTB_LOAD_ADDR    0x22080000

#ifdef CONFIG_MMC
#define SPL_SIZE        0x400000 /* emmc */
struct mmc *smartchip_emmc_init(void);
int spl_mmc_read_from_hw_partition(int hwpart, lbaint_t offset,
    lbaint_t blkcnt, void *buffer);
int spl_mmc_read_from_boot_partition(lbaint_t offset,
    lbaint_t blkcnt, void *buffer);
#endif

static inline u32 efi_crc32(const void *buf, u32 len)
{
	return crc32(0, buf, len);
}

extern struct mtd_info *nand_mtd;
extern struct udevice *nor_dev;
extern struct blk_desc *block_dev;

void print_buf(unsigned char *buf, int len);
int smartchip_device_init(void);

#endif /* __SMARTCHIP_COMMON_H */
