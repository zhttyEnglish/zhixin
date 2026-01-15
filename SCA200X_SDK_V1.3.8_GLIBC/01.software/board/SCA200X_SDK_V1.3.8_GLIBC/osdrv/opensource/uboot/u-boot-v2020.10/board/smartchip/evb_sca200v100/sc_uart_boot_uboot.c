#include <uboot_upgrade.h>
#include <smartchip/sc_common.h>

#include "sc_uart_boot.c"

enum {
	ERR_SUCCESS    = 0,
	ERR_VERIFY     = -1,
	ERR_PROGRAM    = -2,
	ERR_ERASE      = -3,
	ERR_FLASH_TYPE = -4,
	ERR_IMG_SIZE   = -5,
	ERR_OTHER      = -6,
	ERR_DDR        = -7,
};

#ifdef CONFIG_MMC
#include <mmc.h>
static int sc_mmc_write_boot(unsigned char *bootloader, size_t size)
{
	int i, ret;
	int part;
	unsigned int blk_size;
	struct mmc *mmc_dev;
	struct blk_desc *block_dev;

	mmc_dev = smartchip_emmc_init();
	if (!mmc_dev) {
		printf("error: smartchip_emmc_init\n");
		return ERR_OTHER;
	}
	block_dev = mmc_get_blk_desc(mmc_dev);
	if (!block_dev) {
		printf("error: mmc_get_blk_desc\n");
		return ERR_OTHER;
	}

	blk_size = ALIGN(size / block_dev->blksz, mmc_dev->erase_grp_size);
	for (i = 0; i < 2; i++) {
		part = i == 0 ? MMC_PART_BOOT_AERA_1 : MMC_PART_BOOT_AERA_2;
		ret = blk_select_hwpart_devnum(IF_TYPE_MMC, 0, part);
		if (ret) {
			printf("fail select BOOT_AERA_1 \n");
			return ERR_OTHER;
		}

		debug("Erase mmc bootloader%d...\n", i);
		ret = blk_derase(block_dev, 0, blk_size);
		if (ret < 0) {
			printf("error %d, %d\n", __LINE__, ret);
			return ERR_ERASE;
		}

		debug("Write mmc bootloader%d...\n", i);
		ret = blk_dwrite(block_dev, 0, blk_size, bootloader);
		if (ret < 0) {
			printf("error %d, %d\n", __LINE__, ret);
			return ERR_PROGRAM;
		}
	}

	ret = blk_select_hwpart_devnum(IF_TYPE_MMC, 0, MMC_PART_UDA);
	if (ret) {
		printf("fail select BOOT_AERA_1 \n");
		return ERR_OTHER;
	}

	return ERR_SUCCESS;
}
#endif

static int uboot_upgrage_recv_cmd(void)
{
	struct sc_uart_cmd_header hd;

	printf("start recv cmd\n");
	printf(UART_BOOT_OK);

	/* 1. head */
	sc_uart_get_header((char *)hd.magic_header);
	sc_uart_readn((void *)&hd + 2, sizeof(hd) - 2);
	sc_dbg("head.addr :0x%x\n", hd.dst_addr_l);
	sc_dbg("head.len  :0x%x\n", hd.len);
	serial_putc(SC_UART_ACK_CHAR);

	if(hd.len) {
		serial_putc(SC_UART_NAK_CHAR);
		return SC_UART_CMD_BURN_DDR;
	}

	return hd.cmd;

}

static void sc_burn_uboot(void)
{
	void *load_addr;
	ulong total_len;

	load_addr = get_boot_image();
	total_len = get_boot_image_size();
	printf("write boot.img. addr: %p, len: 0x%lx\n", load_addr, total_len);

	smartchip_device_init();

#if CONFIG_MMC
	sc_mmc_write_boot(load_addr, total_len);

#elif defined(CONFIG_SPI_FLASH)
	//todo

#elif defined(CONFIG_MTD_SPI_NAND)
	//todo

#endif
}

int uboot_upgrade(void)
{
	int cmd;
	unsigned int boot_info;

	boot_info = get_spl_boot_dev();
	if(BOOTROM_DEVICE_USB == boot_info) {
		// return
		return 0;
	} else if(BOOTROM_DEVICE_UART == boot_info) {
		//do next
	} else {
		// return
		return 0;
	}

	cmd = uboot_upgrage_recv_cmd();
	switch(cmd) {
	case SC_UART_CMD_BURN_DDR:
		break;
	case SC_UART_CMD_BURN_EMMC:
		sc_burn_uboot();
		break;
	default:
		break;
	}

	return 0;
}
