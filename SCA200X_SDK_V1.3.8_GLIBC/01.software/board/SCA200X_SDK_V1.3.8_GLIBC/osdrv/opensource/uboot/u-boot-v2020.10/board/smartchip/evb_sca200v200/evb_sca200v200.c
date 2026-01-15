#include <common.h>
#include <phy_interface.h>
#include <asm/io.h>
#include <dm.h>
#include <smartchip/sc_common.h>

extern int smartx_eth_initialize(ulong base_addr, unsigned int interface, unsigned char *enetaddr);

#ifdef CONFIG_SPL_BUILD
DECLARE_GLOBAL_DATA_PTR;

void spl_board_init(void)
{
#if defined(CONFIG_SPI_FLASH) || defined(CONFIG_SPL_SPI_LOAD)
	smartchip_qspi_nor_init();
#endif

	return;
}
#endif

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */
int board_eth_init(void)
{
#ifdef CONFIG_SMARTCHIP_GMAC
	//Set pin to eth
	/* Set pin share in uinform interface */
	//  smartx_eth_gpio_set();

	unsigned char enetaddr[6] = {0x00, 0x50, 0xc2, 0x13, 0x6f, 0x00};

	/*hbbai: set AHB burst mode*/
	//  writel(0, SMARTCHIP_GMAC_AHB_CFG);
	//  writel(0x00000001, SMARTCHIP_GMAC_AHB_TX_CFG);
	//  writel(0x00000001, SMARTCHIP_GMAC_AHB_RX_CFG);

	int ret = smartx_eth_initialize(SMARTCHIP_GMAC_BASE, PHY_INTERFACE_MODE_RMII, enetaddr);
	if (ret < 0) {
		printf("Failed to initialize gmac\n");
		return ret;
	}

	writel((enetaddr[0] << 24) | (enetaddr[1] << 16) | (enetaddr[2] << 8) | (enetaddr[3]), SMARTCHIP_GMAC_MAC_0);
	writel((enetaddr[4] << 8) | (enetaddr[5]), SMARTCHIP_GMAC_MAC_1);

#endif
	return 0;
}

int board_init(void)
{

#if defined(CONFIG_SPI_FLASH) || defined(CONFIG_SPL_SPI_LOAD)
	smartchip_qspi_nor_init();
#endif

	return 0;
}

int board_early_init_f(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device(UCLASS_CLK, 0, &dev);
	if (ret) {
		debug("%s: Cannot find clock device\n", __func__);
	}

	return 0;
}
