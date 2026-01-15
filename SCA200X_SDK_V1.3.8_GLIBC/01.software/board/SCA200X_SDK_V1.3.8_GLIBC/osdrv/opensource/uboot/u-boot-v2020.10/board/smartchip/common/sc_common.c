#include <asm/io.h>
#include <asm/spl.h>
#include <dm.h>
#include <mmc.h>
#include <mtd.h>
#include <spi_flash.h>
#include <asm/arch/boot.h>
#include <uboot_upgrade.h>
#include <smartchip/sc_common.h>
#include <part_efi.h>
#include <memalign.h>
#include <part.h>
#include <common.h>

struct mtd_info *nand_mtd;
struct udevice *nor_dev;
struct blk_desc *block_dev;

/* the code in the boot and ota is the same */
void print_buf(unsigned char *buf, int len)
{
	int i = 0;

	for(i = 0; i < len; i++) {
		printf("%02x ", buf[i]);
		if((i + 1) % 16 == 0 || i == len - 1)
			printf("\n");
	}
}

#ifdef CONFIG_MMC
struct mmc *smartchip_emmc_init(void)
{
	struct mmc *mmcp = NULL;
	int ret = 0;

	ret = mmc_init_device(0);
	if (ret) {
		printf("spl: could not initialize mmc. ret: %d\n", ret);
		return NULL;
	}
	mmcp = find_mmc_device(0);
	ret = mmcp ? 0 : -ENODEV;
	if (ret) {
		printf("spl: could not find mmc dev 0. ret: %d\n", ret);
		return NULL;
	}

	ret = mmc_init(mmcp);
	if (ret) {
		mmcp = NULL;
		printf("spl: mmc init failed with ret %d!\n", ret);
		return NULL;
	}

	return mmcp;
}
#endif

#if defined(CONFIG_SPI_FLASH) || defined(CONFIG_SPL_SPI_LOAD)
struct udevice *smartchip_qspi_nor_init(void)
{
	struct udevice *dev = NULL;

	if ((CONFIG_IS_ENABLED(MTD) && CONFIG_IS_ENABLED(SPI_FLASH))
	    || CONFIG_IS_ENABLED(DM_SPI_FLASH)) {
		if (uclass_get_device(UCLASS_SPI_FLASH, 0, &dev)) {
			pr_err("spi flash probe failed\n");
			return NULL;
		}
	}

	return dev;
}
#endif

#ifdef CONFIG_MTD_SPI_NAND
struct mtd_info *smartchip_qspi_nand_init(void)
{
	struct mtd_info *mtd = NULL;

	/* Ensure all devices (and their partitions) are probed */
	mtd_probe_devices();

	mtd_for_each_device(mtd) {
		if (!mtd_is_partition(mtd) && mtd->type == MTD_NANDFLASH) {
			break;
		}
	}

	if (mtd == NULL) {
		printf("qspi nand init failed\n");
	}

	return mtd;
}
#endif

/* init qspi nor/qspi nand/emmc */
int smartchip_device_init(void)
{
	static int initd = 0;
	int boot_mode;

	if (initd)
		return 0;

	boot_mode = get_boot_init_device();
#ifdef CONFIG_SPI_FLASH
	if (BOOT_DEVICE_SPI == boot_mode
		||BOOT_DEVICE_BOARD == boot_mode) {
		nor_dev = smartchip_qspi_nor_init();
		if (!nor_dev)
			return -ENODEV;
		goto out;
	}
#endif

#ifdef CONFIG_MTD_SPI_NAND
	if (BOOT_DEVICE_SPINAND == boot_mode
		||BOOT_DEVICE_BOARD == boot_mode) {
		nand_mtd = smartchip_qspi_nand_init();
		if (!nand_mtd)
			return -ENODEV;
		goto out;
	}
#endif

#ifdef CONFIG_MMC
	if (BOOT_DEVICE_MMC1 == boot_mode
		||BOOT_DEVICE_BOARD == boot_mode) {
		struct mmc *mmcp;
		mmcp = smartchip_emmc_init();
		if (mmcp)
			block_dev = mmc_get_blk_desc(mmcp);
		else
			return -ENODEV;
		goto out;
	}
#endif
out:
	initd = 1;

	return 0;
}

#ifdef CONFIG_ENV_IS_IN_MMC
int mmc_get_env_addr(struct mmc *mmc, int copy, u32 *env_addr)
{
	/* unit is byte, not blk */
	*env_addr = CONFIG_ENV_OFFSET;
	return 0;
}
#endif

