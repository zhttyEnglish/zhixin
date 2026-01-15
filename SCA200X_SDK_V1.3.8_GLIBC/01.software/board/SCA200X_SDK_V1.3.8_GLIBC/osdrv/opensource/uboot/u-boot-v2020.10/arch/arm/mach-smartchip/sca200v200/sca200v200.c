#include <common.h>
#include <fdt_support.h>
#include <init.h>
#include <log.h>
#include <spl.h>
#include <spl_gpio.h>
#include <syscon.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <asm/arch/boot.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region sca200v200_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x10000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
		PTE_BLOCK_NON_SHARE |
		PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x20000000UL,
		.phys = 0x20000000UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
		PTE_BLOCK_INNER_SHARE
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = sca200v200_mem_map;

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
}
#endif

/* get boot device from boot config pin */
int get_boot_device(void)
{
	int val = readl(SMARTCHIP_BOOT_MODE_VALUE_REG) & 0x07;

	return val;
}

int get_boot_init_device(void)
{
	int boot_mode;
	int boot_device;

	boot_mode = get_boot_device();

	switch (boot_mode) {
	case BOOT_VALUE_ROM_QSPI_NOR_1V8:
    case BOOT_VALUE_ROM_QSPI_NOR_3V3:
		boot_device = BOOT_DEVICE_SPI;
		break;
	case BOOT_VALUE_ROM_QSPI_NAND_1V8:
    case BOOT_VALUE_ROM_QSPI_NAND_3V3:
		boot_device = BOOT_DEVICE_SPINAND;
		break;
	case BOOT_VALUE_ROM_EMMC:
		boot_device = BOOT_DEVICE_MMC1;
		break;
	default:
		printf("Boot mode is not supported now!\n");
		boot_device = BOOT_DEVICE_MMC1;
		break;
	}

	return boot_device;
}

int secure_boot_enabled(void)
{
	//save the secure mode to register so u-boot/kernel/.. can get it from normal world.
	/*if(((readl(POLESTAR_EFUSE_SECURE_CONFIG) & BIT_32(SECURE_BOOT_SHIFT)) != 0)) {
	    writel(readl(BOOT_DEVICE_STATUS_BASE_REG) | SEC_BOOT_EN, BOOT_DEVICE_STATUS_BASE_REG);
	    return 1;
	}*/

	return 0;
}

