#ifndef __SCA200V200_BOOT_H
#define __SCA200V200_BOOT_H

#include <asm/arch/cpu.h>

#define SPL_RUN_DONE                (1UL << 31)//BIT(31)
#define SEC_BOOT_EN                 (1UL << 30)//BIT(30)

#define SMARTCHIP_BOOT_MODE_VALUE_REG     0x0A10602C

#define BOOT_VALUE_ROM_QSPI_NOR_1V8     0x0000
#define BOOT_VALUE_ROM_QSPI_NAND_1V8    0x0001
#define BOOT_VALUE_ROM_QSPI_NOR_3V3     0x0002
#define BOOT_VALUE_ROM_QSPI_NAND_3V3    0x0003
#define BOOT_VALUE_ROM_EMMC             0x0004
#define BOOT_VALUE_QSPI_NOR             0x0005
#define BOOT_VALUE_ROM_USB              0x0006
#define BOOT_VALUE_ROM_UART_TIMEOUT     0x0007

#define BOOTROM_DEVICE_QSPI_NOR_1V8                0x0001
#define BOOTROM_DEVICE_QSPI_NAND_1V8               0x0002
#define BOOTROM_DEVICE_QSPI_NOR_3V3                0x0004
#define BOOTROM_DEVICE_QSPI_NAND_3V3               0x0008
#define BOOTROM_DEVICE_EMMC                                0x0010
#define BOOTROM_DEVICE_QSPI_NOR_XIP                0x0020
#define BOOTROM_DEVICE_USB                                 0x0040
#define BOOTROM_DEVICE_UART                                0x0080

#define SMP_INFO_SIZE           (0x40)
/* bootrom 将spl 固定加载到如下地址 */
#define SPL_HEADER_BUF_BASE (SMARTCHIP_SRAM_BASE + SMP_INFO_SIZE)
#define SPL_BACK_ADDR       (SMARTCHIP_SRAM_BASE + 0x00080000) /* temp spl saved address */
#define BOOT_BACK_INFO      (SMARTCHIP_DRAM_BASE + 0x01000000) /* temp spl save boot.img's info */
#define BOOT_BACK_ADDR      (SMARTCHIP_DRAM_BASE + 0x02000000) /* temp spl save boot.img */
#define BOOT_RECV_COMP      (SMARTCHIP_DRAM_BASE + 0x03000000) /* temp spl recv boot.img.zip */

int get_boot_device(void);
int get_boot_init_device(void);

int secure_boot_enabled(void);

#endif /* __SCA200V200_BOOT_H */
