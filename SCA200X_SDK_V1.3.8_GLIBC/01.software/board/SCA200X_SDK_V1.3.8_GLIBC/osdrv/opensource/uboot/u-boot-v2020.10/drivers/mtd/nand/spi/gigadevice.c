// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Stefan Roese <sr@denx.de>
 *
 * Derived from drivers/mtd/nand/spi/micron.c
 *   Copyright (c) 2016-2017 Micron Technology, Inc.
 */

#ifndef __UBOOT__
#include <malloc.h>
#include <linux/device.h>
#include <linux/kernel.h>
#endif
#include <linux/mtd/spinand.h>

#define SPINAND_MFR_GIGADEVICE			0xC8
#define GD5FXGQ4XA_STATUS_ECC_1_7_BITFLIPS	(1 << 4)
#define GD5FXGQ4XA_STATUS_ECC_8_BITFLIPS	(3 << 4)

#define GD5FXGQ4XEXXG_REG_STATUS2		0xf0

#define GD5F4GM5RFxxG_STATUS_ECC_MASK 	GENMASK(6, 4)
#define GD5F4GM5RFxxG_STATUS_ECC_1_3_BITFLIPS	(1 << 4)
#define GD5F4GM5RFxxG_STATUS_ECC_4_BITFLIPS		(2 << 4)
#define GD5F4GM5RFxxG_STATUS_ECC_5_BITFLIPS		(3 << 4)
#define GD5F4GM5RFxxG_STATUS_ECC_6_BITFLIPS		(4 << 4)
#define GD5F4GM5RFxxG_STATUS_ECC_7_BITFLIPS		(5 << 4)
#define GD5F4GM5RFxxG_STATUS_ECC_8_BITFLIPS		(6 << 4)
#define GD5F4GM5RFxxG_STATUS_ECC_UNCOR_ERROR	(7 << 4)

static SPINAND_OP_VARIANTS(read_cache_variants,
		SPINAND_PAGE_READ_FROM_CACHE_QUADIO_OP(0, 2, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_DUALIO_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

static SPINAND_OP_VARIANTS(read_cache_variants_f1,
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

static SPINAND_OP_VARIANTS(read_cache_variants_f2,
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP_3A(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP_3A(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP_3A(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP_3A(false, 0, 0, NULL, 0));

static SPINAND_OP_VARIANTS(write_cache_variants,
		SPINAND_PROG_LOAD_X4(true, 0, NULL, 0),
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_variants,
		SPINAND_PROG_LOAD_X4(false, 0, NULL, 0),
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

static int gd5fxgq4xexxg_ooblayout_ecc(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	region->offset = 64;
	region->length = 64;

	return 0;
}

static int gd5fxgq4xexxg_ooblayout_free(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	/* Reserve 1 bytes for the BBM. */
	region->offset = 1;
	region->length = 63;

	return 0;
}

static int gd5fxgq4xexxg_ecc_get_status(struct spinand_device *spinand,
					u8 status)
{
	u8 status2;
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(GD5FXGQ4XEXXG_REG_STATUS2,
						      &status2);
	int ret;

	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case GD5FXGQ4XA_STATUS_ECC_1_7_BITFLIPS:
		/*
		 * Read status2 register to determine a more fine grained
		 * bit error status
		 */
		ret = spi_mem_exec_op(spinand->slave, &op);
		if (ret)
			return ret;

		/*
		 * 4 ... 7 bits are flipped (1..4 can't be detected, so
		 * report the maximum of 4 in this case
		 */
		/* bits sorted this way (3...0): ECCS1,ECCS0,ECCSE1,ECCSE0 */
		return ((status & STATUS_ECC_MASK) >> 2) |
			((status2 & STATUS_ECC_MASK) >> 4);

	case GD5FXGQ4XA_STATUS_ECC_8_BITFLIPS:
		return 8;

	case STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	default:
		break;
	}

	return -EINVAL;
}

static const struct mtd_ooblayout_ops gd5fxgq4xexxg_ooblayout = {
	.ecc = gd5fxgq4xexxg_ooblayout_ecc,
	.rfree = gd5fxgq4xexxg_ooblayout_free,
};

static int gd5fxgm5xfxxg_ooblayout_ecc(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	region->offset = 128;
	region->length = 128;

	return 0;
}

static int gd5fxgm5xfxxg_ooblayout_free(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	/* Reserve 1 bytes for the BBM. */
	region->offset = 1;
	region->length = 127;

	return 0;
}

static int gd5fxgm5xfxxg_ecc_get_status(struct spinand_device *spinand,
					u8 status)
{
	switch (status & GD5F4GM5RFxxG_STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case GD5F4GM5RFxxG_STATUS_ECC_1_3_BITFLIPS:
		return 3;

	case GD5F4GM5RFxxG_STATUS_ECC_4_BITFLIPS:
		return 4;

	case GD5F4GM5RFxxG_STATUS_ECC_5_BITFLIPS:
		return 5;

	case GD5F4GM5RFxxG_STATUS_ECC_6_BITFLIPS:
		return 6;

	case GD5F4GM5RFxxG_STATUS_ECC_7_BITFLIPS:
		return 7;

	case GD5F4GM5RFxxG_STATUS_ECC_8_BITFLIPS:
		return 8;

	case GD5F4GM5RFxxG_STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	default:
		break;
	}

	return -EINVAL;
}

static const struct mtd_ooblayout_ops gd5fxgm5xfxxg_ooblayout = {
	.ecc = gd5fxgm5xfxxg_ooblayout_ecc,
	.rfree = gd5fxgm5xfxxg_ooblayout_free,
};

static const struct spinand_info gigadevice_spinand_table[] = {
	SPINAND_INFO("GD5F1GQ4UExxG", 0xd1,
		     NAND_MEMORG(1, 2048, 128, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&gd5fxgq4xexxg_ooblayout,
				     gd5fxgq4xexxg_ecc_get_status)),
	SPINAND_INFO("GD5F1GQ5RExxG", 0x41,
		     NAND_MEMORG(1, 2048, 128, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants_f1,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&gd5fxgq4xexxg_ooblayout,
				     gd5fxgq4xexxg_ecc_get_status)),
	SPINAND_INFO("GD5F4GM5RFxxG", 0xA4,
		     NAND_MEMORG(1, 4096, 256, 64, 2048, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants_f2,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&gd5fxgm5xfxxg_ooblayout,
				     gd5fxgm5xfxxg_ecc_get_status)),
	SPINAND_INFO("GD5F4GM8RExxG", 0x85,
		     NAND_MEMORG(1, 2048, 128, 64, 4096, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants_f1,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&gd5fxgq4xexxg_ooblayout,
				     gd5fxgq4xexxg_ecc_get_status)),
	SPINAND_INFO("GD5F4GM8UExxG", 0x95,
		     NAND_MEMORG(1, 2048, 128, 64, 4096, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants_f1,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&gd5fxgq4xexxg_ooblayout,
				     gd5fxgq4xexxg_ecc_get_status)),
};

static int gigadevice_spinand_detect(struct spinand_device *spinand)
{
	u8 *id = spinand->id.data;
	u8 did;
	int ret;

	/*
	 * Earlier GDF5-series devices (A,E) return [0][MID][DID]
	 * Later (F) devices return [MID][DID1][DID2]
	 */

	if (id[0] == SPINAND_MFR_GIGADEVICE)
		did = id[1];
	else if (id[0] == 0 && id[1] == SPINAND_MFR_GIGADEVICE)
		did = id[2];
	else if (id[0] == 0xff && id[1] == SPINAND_MFR_GIGADEVICE)
		did = id[2];
	else
		return 0;

	ret = spinand_match_and_init(spinand, gigadevice_spinand_table,
				     ARRAY_SIZE(gigadevice_spinand_table),
				     did);
	if (ret)
		return ret;

	return 1;
}

static const struct spinand_manufacturer_ops gigadevice_spinand_manuf_ops = {
	.detect = gigadevice_spinand_detect,
};

const struct spinand_manufacturer gigadevice_spinand_manufacturer = {
	.id = SPINAND_MFR_GIGADEVICE,
	.name = "GigaDevice",
	.ops = &gigadevice_spinand_manuf_ops,
};
