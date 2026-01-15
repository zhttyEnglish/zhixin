/*
 * (C) Copyright 2015 Hans de Goede <hdegoede@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _SCA200V100_CPU_H
#define _SCA200V100_CPU_H

/********** TZC register **************************/
//base addr
#define TZC_4F_BASE     0x01058000  //4-filter
#define TZC_2F_BASE     0x64539000  //2-filter
#define TZC_GLB_REG_BASE    0x6453A000  //tzc400 global register

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
#define RTC_RF_BASE         (0x01076040)
#define RTC_CON1            (0x04)
#define SYS_POWER_ON_CTRL       (RTC_RF_BASE + RTC_CON1)
#define GLOBAL_TIMER_GATE_CTL       (0x02106000)

/********** GIC register **************************/
#define GIC_BASE            0x01000000

/********** CCI register **************************/
#define SMARTCHIP_CCI_BASE    0x0d090000
#define CCI_SECURE_ACCESS_REG  (0x8)
#define CCI_PMCR        (0x100)

#define CCI_S0_BASE (0x1000)
#define CCI_S1_BASE (0x2000)
#define CCI_S2_BASE (0x3000)
#define CCI_S3_BASE (0x4000)
#define CCI_S4_BASE (0x5000)

#define CCI_S0_SNOOP_CTRL_REG (CCI_S0_BASE + 0)
#define CCI_S1_SNOOP_CTRL_REG (CCI_S1_BASE + 0)
#define CCI_S2_SNOOP_CTRL_REG (CCI_S2_BASE + 0)
#define CCI_S3_SNOOP_CTRL_REG (CCI_S3_BASE + 0)
#define CCI_S4_SNOOP_CTRL_REG (CCI_S4_BASE + 0)

/********** QSPI register **************************/
#define SMARTCHIP_QSPI_FLASH_BASE   0x10000000

/********** EFUSE register ************************/
#define EFUSE_BASE_ADDRESS              0x01060000
#define POLESTAR_EFUSE_SECURE_CONFIG    0x01060000
#define SECURE_BOOT_SHIFT               0x15

/********* ABB DDRPLL register **************************/
#define DDRPLL_DIG_REG0_ADDR    0x01072a00
#define DDRPLL_DIG_REG1_ADDR    0x01072a04
#define DDRPLL_DIG_REG_ADDR     0x01072a0c
/* bit area */
#define   DDRPLL_DIG_REG_PLL_LOCK_MASK		(0x1 << 0)

/********* ABB ADC register *************************/
#define ABB_CORE_REG0_ADDR  0x01072000
#define ABB_CORE_REG6_ADDR  0x01072018
#define ABB_CORE_REG9_ADDR  0x01072024
#define SAR10_WORK_MODE     0x01072800
#define SAR10_CHAN_SEL      0x01072804
#define SAR10_DATA          0x01072810

/******** Pinshare register *************************/
#define SMARTCHIP_PINSHARE_BASE     0x01073000
#define SMARTCHIP_PINSELECT_BASE    0x01073130

/******** Global register *************************/
#define BOOT_DEVICE_STATUS_BASE_REG 0x01074100
#define UPGRADE_BOOT_UBOOT_ADDR_REG 0x0107413c

/********* PMU register **************************/
#define PMU_BASE            0x01075000
#define PMU_CTRL_OFFSET     0x00000000
#define IP_PMU_CTRL_OFFSET  0x00000004

/********** AHB burst define************************/
/********** GMAC register ************************/
#define SMARTCHIP_GMAC_BASE         0x01130000

#define SMARTCHIP_GMAC_MAC_0   (SMARTCHIP_GMAC_BASE + 0x40)
#define SMARTCHIP_GMAC_MAC_1   (SMARTCHIP_GMAC_BASE + 0x44)

/******** GPIO define ***************************/
#define SMARTCHIP_GPIO_BASE     0x08400000

/******** SRAM define ***************************/
#define SMARTCHIP_SRAM_BASE     0x017c0000
#define SHARED_RAM_BASE         0x017c0000
#define SHARED_RAM_SIZE         0x00001000 //4KB res, 512B for spl header.

/******** CEVA define ***************************/
#define CEVA_PWR_CTRL           0x010c3060
#define CEVA_PWR_STAT           0x010c3058
#define CEVA_CORE_0_RGU         0x010c1004
#define CEVA_CORE_0_REG         0x010c3000
#define CEVA_SRAM_ADDR          0x01800000
#define SMARTCHIP_CEVA_RAM_BASE 0x01800000

/******** DRAM define ***************************/
#define SMARTCHIP_DRAM_BASE     0x20000000
#define DRAM_START_ADDR         0x20000000

#endif /* _SCA200V100_CPU_H */
