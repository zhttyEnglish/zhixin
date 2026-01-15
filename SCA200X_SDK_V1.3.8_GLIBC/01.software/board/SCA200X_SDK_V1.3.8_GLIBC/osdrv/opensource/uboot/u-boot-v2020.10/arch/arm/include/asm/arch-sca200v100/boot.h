#ifndef __SCA200V100_BOOT_H
#define __SCA200V100_BOOT_H

#include <asm/arch/cpu.h>

#define SPL_RUN_DONE                (1UL << 31)//BIT(31)
#define SEC_BOOT_EN                 (1UL << 30)//BIT(30)

#define POLESTAR_BOOT_MODE_VALUE_REG    0x01073120
#define BOOT_VALUE_ROM_QSPI_NOR     0x0000
#define BOOT_VALUE_ROM_QSPI_NAND    0x0001
#define BOOT_VALUE_ROM_SPI_NOR      0x0002
#define BOOT_VALUE_QSPI_NOR         0x0003
#define BOOT_VALUE_ROM_EMMC         0x0004
#define BOOT_VALUE_ROM_SD           0x0005
#define BOOT_VALUE_ROM_AUTO         0x0006
#define BOOT_VALUE_ROM_AUTO_BYPASS_PLL      0x0007

#define BIT_32(nr)          (1UL << (nr))

#define UPGRADE_BOOT_UBOOT_MAX_SIZE (2 * 512 * 1024)

/*
 * @for spl head: BOOTROM_DEVICE_QSPI_NOR
 *    bootrom set.
 * @for cfg pin : BOOT_VALUE_ROM_QSPI_NOR
 *    cfg reg val.
 */
#define BOOTROM_DEVICE_QSPI_NOR         0x0001
#define BOOTROM_DEVICE_QSPI_NAND        0x0002
#define BOOTROM_DEVICE_SPI_NOR          0x0004
#define BOOTROM_DEVICE_EMMC             0x0008
#define BOOTROM_DEVICE_SD               0x0010
#define BOOTROM_DEVICE_QSPI_NOR_XIP     0x0020
#define BOOTROM_DEVICE_ALL             (BOOTROM_DEVICE_QSPI_NOR|BOOTROM_DEVICE_QSPI_NAND|BOOTROM_DEVICE_SPI_NOR|BOOTROM_DEVICE_EMMC|BOOTROM_DEVICE_SD)
#define BOOTROM_BYPASS_PLL_ALL          0x0040
#define BOOTROM_DEVICE_USB              0x0080
#define BOOTROM_DEVICE_UART             0x0100

#define BL_RAM_BASE         (SMARTCHIP_SRAM_BASE + SHARED_RAM_SIZE)  //0x017c1000 offset at 4KB

#define BOOT_INFO_STRICT_BASE_ADDR SMARTCHIP_SRAM_BASE
#define SMP_INFO_SIZE              (0x40)

#define BOOT_HOLD_ENTRY_SHIFT   (3)
#define BOOT_SPSR_EL3_ADDR      (BOOT_INFO_STRICT_BASE_ADDR + 24)
#define BOOT_SCR_EL3_ADDR       (BOOT_INFO_STRICT_BASE_ADDR + 32)

/* bootrom 将spl 固定加载到如下地址 */
#define SPL_HEADER_BUF_BASE (SMARTCHIP_SRAM_BASE + SMP_INFO_SIZE)
#define SPL_BACK_ADDR       (SMARTCHIP_CEVA_RAM_BASE) /* temp spl saved address */
#define BOOT_BACK_INFO      (SMARTCHIP_DRAM_BASE + 0x01000000) /* temp spl save boot.img's info */
#define BOOT_BACK_ADDR      (SMARTCHIP_DRAM_BASE + 0x02000000) /* temp spl save boot.img */
#define BOOT_RECV_COMP      (SMARTCHIP_DRAM_BASE + 0x03000000) /* temp spl recv boot.img.zip */

#ifndef __ASSEMBLY__

int get_boot_device(void);
int get_boot_init_device(void);

int secure_boot_enabled(void);

#endif

#endif /* __SCA200V100_BOOT_H */
