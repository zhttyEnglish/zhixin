/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2012
 * Texas Instruments, <www.ti.com>
 */
#ifndef	_ASM_SPL_H_
#define	_ASM_SPL_H_

enum {
	BOOT_DEVICE_RAM,
	BOOT_DEVICE_MMC1,
	BOOT_DEVICE_MMC2,
	BOOT_DEVICE_MMC2_2,
	BOOT_DEVICE_NAND,
	BOOT_DEVICE_ONENAND,
	BOOT_DEVICE_NOR,
	BOOT_DEVICE_UART,
	BOOT_DEVICE_SPI,
	BOOT_DEVICE_USB,
	BOOT_DEVICE_SATA,
	BOOT_DEVICE_I2C,
	BOOT_DEVICE_BOARD,
	BOOT_DEVICE_DFU,
	BOOT_DEVICE_XIP,
	BOOT_DEVICE_BOOTROM,
	BOOT_DEVICE_SPINAND,
	BOOT_DEVICE_NONE
};

int secure_boot_enabled(void);
int get_boot_device(void);
void set_slave_cpu_info(unsigned long sprs_el3, unsigned long scr_el3, unsigned long ep);

int spl_verify(unsigned char *hash, unsigned long hash_len,
		unsigned char *sig, unsigned long sig_len,
		unsigned char *img, unsigned long img_len);

int spl_verify_sw(unsigned char *hash, unsigned long hash_len,
		unsigned char *sig, unsigned long sig_len,
		unsigned char *img, unsigned long img_len);

/* Linker symbols. */
extern char __bss_start[], __bss_end[];

#ifndef CONFIG_DM
extern gd_t gdata;
#endif

#endif
