/* ~.~ *-c-*
 *
 * Copyright (c) 2022, John Lee <furious_tauren@163.com>
 * Wed Aug 24 11:02:24 AM CST 2022
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>

#include <dm.h>

#include <led.h>
#include <mmc.h>

#include <image-sparse.h>
#include <image.h>
#include <fs.h>

#include <linux/delay.h>

struct led_upgrade
{
	const char     *label;
	struct udevice *dev;
};

static int led_init(struct led_upgrade *pled, int cnt)
{
	int i;
	int ret;

	for(i = 0; i < cnt; i++) {
		ret = led_get_by_label(pled[i].label, &pled[i].dev);
		if (ret) {
			printf("LED '%s' not found (err=%d)\n", pled[i].label, ret);
			return -1;
		}

		ret = led_set_state(pled[i].dev, LEDST_ON);
		if (ret < 0) {
			printf("LED '%s' operation failed (err=%d)\n", pled[i].label, ret);
			return -1;
		}
	}

	return 0;
}

static int led_toggle(const struct led_upgrade *pled, int cnt)
{
	int i;

	for(i = 0; i < cnt; i++) {
		if(!pled[i].dev)
			continue;

		led_set_state(pled[i].dev, LEDST_TOGGLE);
	}

	return 0;
}

static int led_off(const struct led_upgrade *pled, int cnt)
{
	int i;

	for(i = 0; i < cnt; i++) {
		if(!pled[i].dev)
			continue;

		led_set_state(pled[i].dev, LEDST_OFF);
	}

	return 0;
}

static int led_on(const struct led_upgrade *pled, int cnt)
{
	int i;

	for(i = 0; i < cnt; i++) {
		if(!pled[i].dev)
			continue;

		led_set_state(pled[i].dev, LEDST_ON);
	}

	return 0;
}

static struct led_upgrade g_leds[] = {
	{
		.label = "upgrade",
		.dev = NULL,
	},
	{
		.label = "upgrade_bd2",
		.dev = NULL,
	},
};

static void update_led_int(void)
{
	int ret;

	ret = led_init(g_leds, ARRAY_SIZE(g_leds));
	if (ret) {
		printf("ERROR: led_init\n");
	}

}

/* -------------------------------------------------------------- */
static struct mmc *init_mmc_device(int dev, int part)
{
	struct mmc *mmc;
	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("no mmc device at slot %x\n", dev);
		return NULL;
	}

#if 0
	if (!mmc_getcd(mmc))
		mmc->has_init = 0;
#endif

	mmc->has_init = 0;
	if (mmc_init(mmc))
		return NULL;

#ifdef CONFIG_BLOCK_CACHE
	struct blk_desc *bd = mmc_get_blk_desc(mmc);
	blkcache_invalidate(bd->if_type, bd->devnum);
#endif

	if (blk_select_hwpart_devnum(IF_TYPE_MMC, dev, part))
		return NULL;

	return mmc;
}


static int do_mmc_write(int dev, int part, void *addr, u32 blk, u32 size)
{
	struct mmc *mmc;
	u32 n, cnt;

	mmc = init_mmc_device(dev, part);
	if (!mmc)
		return -1;

	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return -1;
	}

	cnt = size / mmc->write_bl_len;
	if (size % mmc->write_bl_len)
		cnt++;

	printf("MMC write: dev # %d, part # %d, block # %d, count %d ... ",
	       dev, part, blk, cnt);

	n = blk_dwrite(mmc_get_blk_desc(mmc), blk, cnt, addr);
	printf("%d blocks written: %s\n", n, (n == cnt) ? "OK" : "ERROR");

	return (n == cnt) ? 0 : -1;
}


static lbaint_t mmc_sparse_write(struct sparse_storage *info, lbaint_t blk,
				 lbaint_t blkcnt, const void *buffer)
{
	struct blk_desc *dev_desc = info->priv;

	return blk_dwrite(dev_desc, blk, blkcnt, buffer);
}

static lbaint_t mmc_sparse_reserve(struct sparse_storage *info,
				   lbaint_t blk, lbaint_t blkcnt)
{
	return blkcnt;
}

static int do_mmc_sparse_write(int dev, int part, void *addr, u32 blk)
{
	struct sparse_storage sparse;
	struct blk_desc *dev_desc;
	struct mmc *mmc;
	char dest[11];

	if (!is_sparse_image(addr)) {
		printf("Not a sparse image\n");
		return -1;
	}

	mmc = init_mmc_device(dev, part);
	if (!mmc)
		return -1;

	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return -1;
	}

	dev_desc = mmc_get_blk_desc(mmc);
	sparse.priv = dev_desc;
	sparse.blksz = 512;
	sparse.start = blk;
	sparse.size = dev_desc->lba - blk;
	sparse.write = mmc_sparse_write;
	sparse.reserve = mmc_sparse_reserve;
	sparse.mssg = NULL;
	sprintf(dest, "0x" LBAF, sparse.start * sparse.blksz);

	if (write_sparse_image(&sparse, dest, addr, NULL))
		return -1;

	return 0;
}

static int fw_check(void *fit)
{
	int noffset, ndepth = 0;

	noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
	noffset = fdt_next_node(fit, noffset, &ndepth);
	while (noffset >= 0 && ndepth > 0) {
		if (ndepth != 1)
			goto next_node;

		if (!fit_image_verify(fit, noffset))
			return -1;

next_node:
		noffset = fdt_next_node(fit, noffset, &ndepth);
	}

	return 0;
}

static int scan_mmc_devices(void)
{
	struct udevice *dev;
	int count = 0;

	for (uclass_first_device(UCLASS_MMC, &dev);
	     dev; uclass_next_device(&dev)) {
		count++;
	}

	return count > 1;
}

static int fw_upgrade(const char * mem_addr)
{
	int noffset, ndepth = 0;
	void *fit;
	char *ep;
	int ret = CMD_RET_FAILURE;

	led_off(g_leds, ARRAY_SIZE(g_leds));
	printf("############################# checking ...\n");

	fit = (void *)simple_strtoul(mem_addr, &ep, 16);
	if (!fit_check_format(fit)) {
		printf("Bad FIT format, aborting auto-upgrade\n");
		ret = CMD_RET_FAILURE;
		goto out;
	}

	if (fw_check(fit)) {
		printf("FIT verify faield, aborting auto-upgrade\n");
		ret = CMD_RET_FAILURE;
		goto out;
	}
	printf("\n");

	mdelay(200);
	led_on(g_leds, ARRAY_SIZE(g_leds));
	printf("############################# updating ...\n");

	noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
	noffset = fdt_next_node(fit, noffset, &ndepth);
	while (noffset >= 0 && ndepth > 0) {
		uint8_t type;
		const void *data;
		size_t size;
		ulong fladdr;
		ulong part;

		if (ndepth != 1)
			goto next_node;

		printf("\nUpdating '%s':\n", fit_get_name(fit, noffset, NULL));

		if (fit_image_get_data(fit, noffset, &data, &size)) {
			printf("Can't get data section, goto next node\n");
			goto next_node;
		}

		if (fit_image_get_load(fit, noffset, &fladdr)) {
			printf("Can't get load info, goto next node\n");
			goto next_node;
		}

		if (fit_image_get_entry(fit, noffset, &part)) {
			printf("Can't get node entry, goto next node\n");
			goto next_node;

		}

		if (fit_image_get_type(fit, noffset, &type)) {
			printf("Can't get node type, goto next node\n");
			goto next_node;
		}

		if (type == IH_TYPE_RAMDISK || type == IH_TYPE_FILESYSTEM)
			do_mmc_sparse_write(0, part, (void *)data, fladdr);
		else
			do_mmc_write(0, part, (void *)data, fladdr, size);

next_node:
		noffset = fdt_next_node(fit, noffset, &ndepth);
	}

	printf("SUCCESS: Update OK. Please power off your board\n");
	ret = CMD_RET_SUCCESS;

out:
	if (CMD_RET_SUCCESS != ret) {
		printf("ERROR: Update Err. Please check your board\n");
		led_off(g_leds, ARRAY_SIZE(g_leds));
	}

	while (1) {
		if(CMD_RET_SUCCESS == ret)
			led_toggle(g_leds, ARRAY_SIZE(g_leds));
		mdelay(50);
	}

	return ret;

}

static int do_firmware_upgrade(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	int ret = CMD_RET_SUCCESS;

	if(0 == strcmp("dram", argv[1]) && (3 == argc))
		return fw_upgrade(argv[2]);

	if(argc < 5) {
		ret = CMD_RET_USAGE;
		goto EXIT;
	}

	if (!scan_mmc_devices()) {
		printf("ERROR: BD2FT no device\n");
		goto EXIT;
	}

	/* fwup mmc 1 28000000 smartchip_fw.bin */
	if (file_exists(argv[1], argv[2], "BD2FT_NOUP1.bin", FS_TYPE_FAT)) {
		printf("DBG: BD2FT NOUP\n");
		goto EXIT;
	}

	if (! file_exists(argv[1], argv[2], argv[4], FS_TYPE_FAT)) {
		printf("DBG: BD2FT %s is not exists\n", argv[4]);
		goto EXIT;
	}

	update_led_int();
	printf("############################# reading %s ...\n", argv[1]);
	if (do_load(cmdtp, flag, argc, argv, FS_TYPE_FAT)) {
		printf("ERROR: BD2FT load file\n");
		goto EXIT_HANG;
	}

	ret = fw_upgrade(argv[3]);

EXIT:
	return ret;

EXIT_HANG:
	led_off(g_leds, ARRAY_SIZE(g_leds));
	while(1) {
		mdelay(50);
	};
	return ret;
}

U_BOOT_CMD(
	fwup,	5,	0,	do_firmware_upgrade,
	"upgrade firmware from a mmc/usb device",
	"<interface> <dev[:part]> <addr> <filename>\n"
	"fwup dram <addr>\n"
	"example:\n"
	"	fwup mmc 1 28000000 smartchip_fw.bin\n"
	"	fwup dram 28000000\n"
);



