#ifndef _SCA200V100_COMMON_CONFIG_H
#define _SCA200V100_COMMON_CONFIG_H

#include <linux/stringify.h>
#include "smartchip-common.h"

#define SMARTCHIP_EMULATION   /*For emulation use only*/

#define CONFIG_SYS_MAX_NAND_DEVICE 1
#define CONFIG_SYS_NAND_U_BOOT_OFFS 0x260000

#define CONFIG_ARMV8_SET_SMPEN
#define CONFIG_ARMV8_SWITCH_TO_EL1

//#define DEBUG 1
//#define FPGA_SIMULATION

#include <asm/arch/cpu.h>   /* get chip and board defs */

#define CONFIG_SPL_BSS_START_ADDR   0x017d0000
#define CONFIG_SPL_BSS_MAX_SIZE     0x8000

/*#define CONFIG_SYS_PROMPT "smartchip#" */

/* Serial & console */
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_COM1  SMARTCHIP_UART0_BASE

/* NS16550 reg in the low bits of cpu reg, FPGA clk 60M */
#ifdef FPGA_SIMULATION
	#define CONFIG_SYS_NS16550_CLK      40000000
#else
	#define CONFIG_SYS_NS16550_CLK      150000000
	#define CONFIG_DW_WDT_CLOCK_KHZ     150000
#endif

#define CONFIG_DW_WDT_BASE      0x08600000

#if !defined(CONFIG_SPL_BUILD)
	#define CONFIG_GICV2
	#define GICD_BASE           (0x01001000)
	#define GICC_BASE           (0x01002000)
	#define CONFIG_ARMV8_MULTIENTRY
	#define CONFIG_ARMV8_SPIN_TABLE
#endif

/* CPU */
#ifdef FPGA_SIMULATION
	#define CONFIG_SYS_HZ_CLOCK             (40000000)
	#define COUNTER_FREQUENCY               (40000000)
#else
	#define CONFIG_SYS_HZ_CLOCK             (250000000)
	#define COUNTER_FREQUENCY               (250000000)
#endif
#define CONFIG_SYS_CACHELINE_SIZE       64

/* spl malloc */
#if 1 /*we use gd->malloc_base(CONFIG_SYS_MALLOC_F_ADDR) and CONFIG_SYS_MALLOC_F_LEN instead*/
	//#define CONFIG_MALLOC_F_ADDR          0x017d0000 /* troot firmware address */
	//#define CONFIG_SYS_SPL_MALLOC_SIZE        0x3000
	//#define SPL_MALLOC_SIZE       0x3000 /* 12 KiB */
	#define SPL_HEAP_STACK_GAP_SIZE         0x0
	/*
	* 0x17e9c00 - 0x017ff000  text + data (85KB)
	* 0x17ff000 - 0x01800000  spl dtb (4KB)
	*/
	#define CONFIG_SPL_MAX_SIZE         0x15400
	//#define CONFIG_SYS_SPL_MALLOC_START     ((CONFIG_SPL_TEXT_BASE)+CONFIG_SPL_MAX_SIZE+SPL_HEAP_STACK_GAP_SIZE)
#endif

/* spl stack, troot space */
#define CONFIG_SPL_STACK            0x017e8bff /* 0x17e9c00 - 1 */

/* ============================================== uboot memory ============================================== */
#define CONFIG_SYS_SDRAM_BASE       0x20000000
#define CONFIG_SYS_LOAD_ADDR        0x200A0000 /* default load address */
/* uboot stack */
/* end of 32 KiB in sram */
#define LOW_LEVEL_SRAM_STACK        CONFIG_SYS_INIT_SP_ADDR

/* ==============================================       ============================================== */

#define CONFIG_SYS_INIT_SP_OFFSET \
    (CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
    (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define SDRAM_MAX_SIZE          0x80000000

#define CONFIG_NR_DRAM_BANKS        1
#define PHYS_SDRAM_0            CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE       0x8000000 /* 128M now, or 0x40x for 1 GiB */
/*#define CONFIG_SYS_DRAM_TEST          */
/*#define CONFIG_SYS_MALLOC_SIMPLE*/
#define CONFIG_SYS_MEMTEST_START    CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END      (CONFIG_SYS_SDRAM_BASE+PHYS_SDRAM_0_SIZE)
#define CONFIG_SYS_ALT_MEMTEST      1

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
/*#define CONFIG_SERIAL_TAG */

/* mmc config */
#define CONFIG_SYS_MMC_ENV_DEV      0   /* first detected MMC controller */

/* 4MB of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN       (CONFIG_ENV_SIZE + (16 << 20))
#define CONFIG_ENV_OVERWRITE        /* Serial change Ok */

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_CBSIZE   1024    /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE   1024    /* Print Buffer Size */
#define CONFIG_SYS_MAXARGS  16  /* max number of command args */
//#define CONFIG_SYS_GENERIC_BOARD

/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE     CONFIG_SYS_CBSIZE

/* standalone support */
#define CONFIG_STANDALONE_LOAD_ADDR CONFIG_SYS_LOAD_ADDR

/* FLASH and environment organization */

#define CONFIG_SYS_MONITOR_LEN      (1024 << 10)    /* 1024 KiB */

#define CONFIG_SYS_MAX_FLASH_BANKS 1

#define CONFIG_SPL_PAD_TO           CONFIG_SPL_MAX_SIZE /* 64K decimal for 'dd' */

/* I2C */
//#define CONFIG_SYS_I2C
//#define CONFIG_SYS_I2C_BUS_MAX        4

//#define CONFIG_SYS_I2C_BASE         0x40200000
//#define CONFIG_SYS_I2C_BASE1        0x40220000
//#define CONFIG_SYS_I2C_BASE2        0x40240000
//#define CONFIG_SYS_I2C_BASE3        0x40260000

//#define CONFIG_SYS_I2C_SPEED      100000
//#define CONFIG_SYS_I2C_SPEED1     100000
//#define CONFIG_SYS_I2C_SPEED2     100000
//#define CONFIG_SYS_I2C_SPEED3     100000

//#define CONFIG_SYS_I2C_SLAVE      0x7f
//#define CONFIG_SYS_I2C_SLAVE1     0x7f
//#define CONFIG_SYS_I2C_SLAVE2     0x7f
//#define CONFIG_SYS_I2C_SLAVE3     0x7f

//#define CONFIG_CONS_INDEX              1       /* UART0 */

//#if CONFIG_CONS_INDEX == 1
//#define OF_STDOUT_PATH        "/serial@08500000:115200"
//#else
//#error Unsupported console port nr. Please fix stdout-path in smartchip-common.h.
//#endif

/* GPIO */

/* Ethernet support */
#ifdef CONFIG_SMARTCHIP_GMAC
	/*#define CONFIG_DW_AUTONEG*/
	#define CONFIG_PHY_GIGE         /* GMAC can use gigabit PHY */
	#define CONFIG_PHY_ADDR             1
	#define CONFIG_MII          /* MII PHY management       */
	#define CONFIG_SYS_RX_ETH_BUFFER      64
#endif

#ifdef CONFIG_USB_EHCI
	#define CONFIG_USB_OHCI_NEW
	//#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS 1
	//#define CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS 1
	//#define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS 1
#endif

#ifdef CONFIG_USB_MUSB_SMARTCHIP
	#define CONFIG_MUSB_HOST
	#define CONFIG_MUSB_PIO_ONLY
#endif

#if defined CONFIG_USB_EHCI || defined CONFIG_USB_MUSB_SMARTCHIP
	/* #define CONFIG_CMD_USB */
	#define CONFIG_USB_STORAGE
#endif

#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#ifndef CONFIG_SPL_BUILD
#include <config_distro_bootcmd.h>

#ifdef CONFIG_CMD_PXE
	#undef CONFIG_CMD_PXE
#endif
#ifdef CONFIG_SYS_HUSH_PARSER
	#undef CONFIG_SYS_HUSH_PARSER
#endif

#define CONFIG_SYS_BOOTM_LEN  (64*1024*1024)
#define SMARTX_ETH_MAC_ADDR  "ethaddr=00:00:01:02:03:04\0"
#define CONFIG_ETHPRIME "smartx-gmac"
#define CONFIG_IPADDR  192.168.3.138
#define CONFIG_SERVERIP 192.168.3.5
#define CONFIG_NETMASK  255.255.255.0

//#define CONFIG_BOOTARGS "console=ttyS0,115200,earlyprintk"

#ifdef CONFIG_MMC
	#define BOOT_TARGET_DEVICES_MMC(func) func(MMC, mmc, 0)
#else
	#define BOOT_TARGET_DEVICES_MMC(func)
#endif

#ifdef CONFIG_AHCI
	#define BOOT_TARGET_DEVICES_SCSI(func) func(SCSI, scsi, 0)
#else
	#define BOOT_TARGET_DEVICES_SCSI(func)
#endif

#ifdef CONFIG_USB_EHCI
	#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
	#define BOOT_TARGET_DEVICES_USB(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
    BOOT_TARGET_DEVICES_MMC(func) \
    BOOT_TARGET_DEVICES_SCSI(func) \
    BOOT_TARGET_DEVICES_USB(func)

#include <config_distro_bootcmd.h>

/*#define CONFIG_BOOTDELAY 10 */
#define CONFIG_LZO
#define CONSOLE_STDOUT_SETTINGS \
    "stdout=serial\0" \
    "stderr=serial\0"

#define CONSOLE_STDIN_SETTINGS \
    "stdin=serial\0"

#define CONSOLE_ENV_SETTINGS \
    CONSOLE_STDIN_SETTINGS \
    CONSOLE_STDOUT_SETTINGS

#define CONFIG_EXTRA_ENV_SETTINGS \
    CONSOLE_ENV_SETTINGS \
    "fdt_high=0xffffffffffffffff\0" \
    SMARTX_ETH_MAC_ADDR

#else /* ifndef CONFIG_SPL_BUILD */
#define CONFIG_EXTRA_ENV_SETTINGS
#endif

#endif /* _SMARTCHIP_COMMON_CONFIG_H */

