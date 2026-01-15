/*
 * (C) Copyright 2015 Hans de Goede <hdegoede@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _SCA200V200_CPU_H
#define _SCA200V200_CPU_H

/*FIXME: These addresses need to be fixed according to SmartChip register map*/

#define BOOT_DEVICE_STATUS_BASE_REG (0x0A106100)
#define UPGRADE_BOOT_UBOOT_ADDR_REG (0x0A10613c)
#define UPGRADE_BOOT_UBOOT_MAX_SIZE (2 * 512 * 1024)

#ifdef CONFIG_SPL_BUILD
	#ifdef VELOCE_DDR
		#define DDR_PHY_BASE        0x0c000000
		#define DDR_CTR_BASE        0x0e000000
		#define DDR_REG_BASE        0x0e010000
		#define DDR_MON_BASE        0x0e020000
		#define AXI_MON0_BASE       0x0e030000
		#define AXI_MON1_BASE       0x0e040000
		#define AXI_MON2_BASE       0x0e050000
		#define CASE_PASS_ADDR      0x5ffffff0
		#define CASE_FAIL_ADDR      0x5ffffff4
	#endif
#endif

#define SMARTCHIP_UART0_BASE  0x01500000

/********** TZC register **************************/
//base addr
#define TZC_4F_BASE      0x08430000  //4-filter

// TZC400 register offset
#define BUILD_CONFIG        0x000
#ifdef __UBOOT__
	#define ACTION          0x004
#endif
#define GATE_KEEPER     0x008
#define SPECULATION_CTRL    0x00C
#define INT_STATUS      0x010
#define INT_CLEAR       0x014

//Filter0~3: 0x10*x x=0~3
#define FAIL_ADDR_LOW       0x020 + (0x10 * 0)
#define FAIL_ADDR_HIGH      0x024 + (0x10 * 0)
#define FAIL_CONTROL        0x028 + (0x10 * 0)
#define FAIL_ID             0x02C + (0x10 * 0)

//Region0~8: 0x20*n n=1~8
#define RGN_BASE_LOW        0x100 + (0x20 * 0)
#define RGN_BASE_HIGH       0x104 + (0x20 * 0)
#define RGN_TOP_LOW         0x108 + (0x20 * 0)
#define RGN_TOP_HIGH        0x10C + (0x20 * 0)
#define RGN_ATTRIBUTE       0x110 + (0x20 * 0)
#define RGN_ID_ACCESS       0x114 + (0x20 * 0)

/********** RTC register **************************/
// power-on pull up register
#define RTC_RF_BASE         (0x0a10c000)
#define RTC_CON1            (0x04)
#define SYS_POWER_ON_CTRL       (RTC_RF_BASE + RTC_CON1)
#define GLOBAL_TIMER_GATE_CTL       (0x09006000)

#define CPU_CONFIG_BASE         (0x0a100000)
#define CPU_RESET_CTRL          (CPU_CONFIG_BASE + 0xc)
#define CPU_POR_RESET_1         (1 << 1)
#define CPU_CORE_RESET_1        (1 << 5)

/********** GIC register **************************/
#define GIC_BASE            (0x00200000)

#if 0
	#define SMARTCHIP_PINSHARE_BASE       (0x0a10a000)
	#define SMARTCHIP_PINSELECT_BASE      (0x01073130)

	#define SMART_USB_BOOT_REG       7
	#define SMART_USB_BOOT_SEL_PAD       2
	#define SMART_USB_BOOT_DEFAULT_SEL_PAD   0
	#define SMART_USB_BOOT_BIT       3

	#define SMART_UART_BOOT_REG      4
	#define SMART_UART_BOOT_SEL_PAD      1
	#define SMART_UART_BOOT_DEFAULT_SEL_PAD  0
	#define SMART_UART_BOOT_BIT      24

	#define SMART_SPIM1_BOOT_REG     19
	#define SMART_SPIM1_BOOT_SEL_PAD     3
	#define SMART_SPIM1_BOOT_BIT     0
#endif

/*********** PMU register *******************************/
#define PMC_BASE        0x0a10c100
#define PMC_MSC         0x28


/********** AHB burst define************************/
/********** GMAC register ************************/
#define SMARTCHIP_GMAC_BASE       0x08040000
#define SMARTCHIP_GMAC_AHB_CFG        (SMARTCHIP_GMAC_BASE + 0x1000)
#define SMARTCHIP_GMAC_AHB_TX_CFG     (SMARTCHIP_GMAC_BASE + 0x100C)
#define SMARTCHIP_GMAC_AHB_RX_CFG     (SMARTCHIP_GMAC_BASE + 0x1010)

#define SMARTCHIP_GMAC_MAC_0   (SMARTCHIP_GMAC_BASE + 0x40)
#define SMARTCHIP_GMAC_MAC_1   (SMARTCHIP_GMAC_BASE + 0x44)

/*********** ABB DDRPLL *******************************/
#define DDRPLL_DIG_REG0_ADDR    0x0a108400
#define DDRPLL_DIG_REG1_ADDR    0x0a108404
#define DDRPLL_DIG_REG_ADDR     0x0a108100
#define DDRPLL_ANA_REG_ADDR     0x0e010004
/* bit area */
#define   DDRPLL_DIG_REG_PLL_LOCK_MASK        (0x1 << 4)

/********** ABB ADC define************************/
#define ABB_CORE_REG0_ADDR  0x0a108000
#define ABB_CORE_REG6_ADDR  0x0a108018
#define ABB_CORE_REG9_ADDR  0x0a108024
#define SAR10_WORK_MODE     0x0a108200
#define SAR10_CHAN_SEL      0x0a108204
#define SAR10_DATA          0x0a108210

/******** SRAM define ***************************/
#define SMARTCHIP_SRAM_BASE     0x00100000
#define SHARED_RAM_BASE         0x00100000

/******** DRAM define ***************************/
#define SMARTCHIP_DRAM_BASE     0x20000000
#define DRAM_START_ADDR         0x20000000

#endif /* _SCA200V200_CPU_H */
