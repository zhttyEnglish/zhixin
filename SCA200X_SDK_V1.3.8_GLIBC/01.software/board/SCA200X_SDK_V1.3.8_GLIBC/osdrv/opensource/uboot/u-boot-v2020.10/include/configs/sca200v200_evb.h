#ifndef __EVB_SCA200V200_CONFIG_H
#define __EVB_SCA200V200_CONFIG_H

/*
 * Include common smartx configuration where most the settings are
 */
#include "sca200v200_common.h"

/* 0: no smartx_image_header */
#define CONFIGSC_SEC_IMGHD 0

/*
 * Proxima specific configuration
 */

#define PROXIMA_SRAM_START (0x00100000)
#define PROXIMA_SRAM_END   (0x0013FFFF)     // 256KB

#ifdef CONFIG_SPL_BUILD
	#define CONFIG_SYS_INIT_RAM_ADDR        0x00101000  //reserved first 4KB
	#define CONFIG_SYS_INIT_RAM_SIZE        0xF000      /* 60KB */
#else
	#define CONFIG_SYS_INIT_RAM_ADDR        0x30000000
	#define CONFIG_SYS_INIT_RAM_SIZE        0x2000000 /* 32MB */
#endif

#ifdef CONFIG_USB_EHCI
	#define CONFIG_USB_EHCI_SMARTCHIP
	#define CONFIG_USB_MAX_CONTROLLER_COUNT 2
#endif

/* #ifdef CONFIG_USB_DWC3 */
//#define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS  2
#define CONFIG_USB_FUNCTION_MASS_STORAGE    1
/* #endif */

#endif /* __EVB_SCA200V200_CONFIG_H */
