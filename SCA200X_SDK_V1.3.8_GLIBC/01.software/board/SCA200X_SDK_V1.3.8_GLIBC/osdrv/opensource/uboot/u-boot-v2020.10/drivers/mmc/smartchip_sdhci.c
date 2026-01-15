// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2021 SmartChip Electronics Co., Ltd
 *
 * SmartChip SD Host Controller Interface
 */

#include <common.h>
#include <dm.h>
#include <dt-structs.h>
#include <linux/err.h>
#include <linux/libfdt.h>
#include <malloc.h>
#include <mapmem.h>
#include <sdhci.h>
#include <clk.h>
#include <linux/delay.h>

#ifdef CONFIG_SMARTCHIP_SCA200V100

	#define EMMC_GLOBAL_REGISTER_BASE   0x01021000
	#define EMMC_CLK_CTRL_ADDR      0x01070034
	#define EMMC_RESET_CTRL_ADDR        0x01071034
	/* EMMC_CLK_CTRL REGISTER DEFINE */
	#define CLK_CTRL_CFG_ON         0x1
	#define CLK_CTRL_CRADCLK_ON     0x2
	#define CLK_CTRL_CLK2CARD_ON        0x4
	#define CLK_CTRL_CLK_SEL        0x8
	#define CLK_CTRL_FREQ_SEL       0x3ff0
	#define CLK_EMMC_PAD_2_SEL      0x4000

#elif defined(CONFIG_SMARTCHIP_SCA200V200)

	#define EMMC_GLOBAL_REGISTER_BASE   0x08051000
	#define CGU_EMMC_FIX_CLK_ADDR       0x0a10402c
	#define CGU_EMMC_SAMPLE_CLK     0x0a104030
	#define CGU_EMMC_DRV_CLK        0x0a104030
	#define RGU_EMMC_CORE_RSTN_ADDR     0x0a102000
	#define RGU_EMMC_CORE_RSTN_MASK_ADDR    0x0a102008
	#define RGU_EMMC_HRSTN_ADDR     0x0a102010
	#define RGU_EMMC_HRSTN_MASK_ADDR    0x0a102018

#endif

/* GLOBAL REGISTER DEFINE */
#define G_PWR_CTRL          0x0
#define G_AXI_RESETN_CTRL       0x4
#define G_BASE_RESETN_CTRL      0x8
#define G_TX_RESETN_CTRL        0xC
#define G_RX_RESETN_CTRL        0x10
#define G_TIMER_RESETN_CTRL     0x14
#define G_MISC_CTRL         0x18
#define G_SEC_CTRL          0x1C

/* 400KHz is max freq for card ID etc. Use that as min */
#define EMMC_MIN_FREQ           400000

#define SDHCI_P_VENDOR_SPECIFIC_AREA    0x500
#define SDHCI_MSHC_CTRL_R       SDHCI_P_VENDOR_SPECIFIC_AREA + 0x8
#define SDHCI_SW_CG_DIS         0x10

//#define FPGA_EMMC
struct smartchip_sdhc_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	unsigned int out_phase;
};

struct smartchip_sdhc {
	struct sdhci_host host;
	void *base;
};

#ifdef CONFIG_SMARTCHIP_SCA200V100
static void sca200v100_rgu_reset_post(void)
{
#ifdef CONFIG_SPL_BUILD
	unsigned int val;
#endif

	writel(0x0003ff3f, 0x01050074);
#ifdef CONFIG_SPL_BUILD
	val = readl(0x01073094);
	val |= 0x18;
	writel(val, 0x01073094);
#endif
}

#elif defined(CONFIG_SMARTCHIP_SCA200V200)
static void sca200v200_rgu_reset_pre(void)
{
	unsigned int val;

	writel(~(1 << 28), RGU_EMMC_CORE_RSTN_MASK_ADDR);
	val = readl(RGU_EMMC_CORE_RSTN_ADDR);
	val |= (1 << 28);
	writel(val, RGU_EMMC_CORE_RSTN_ADDR);
	writel(~(1 << 0), RGU_EMMC_HRSTN_MASK_ADDR);
	val = readl(RGU_EMMC_HRSTN_ADDR);
	val |= (1 << 0);
	writel(val, RGU_EMMC_HRSTN_ADDR);
}

static void sca200v200_rgu_reset_post(void)
{
	writel(0x0003ff3f, 0x08420000);
}
#endif

static void snps_sdhci_global_reset(void)
{
#ifdef CONFIG_SMARTCHIP_SCA200V100
	writel(0x00000002, EMMC_RESET_CTRL_ADDR);
	writel(0x00000003, EMMC_RESET_CTRL_ADDR);
#elif defined(CONFIG_SMARTCHIP_SCA200V200)
	sca200v200_rgu_reset_pre();
#endif

	writel(0x00000000, EMMC_GLOBAL_REGISTER_BASE + G_AXI_RESETN_CTRL);
	writel(0x00000000, EMMC_GLOBAL_REGISTER_BASE + G_BASE_RESETN_CTRL);
	writel(0x00000000, EMMC_GLOBAL_REGISTER_BASE + G_TX_RESETN_CTRL);
	writel(0x00000000, EMMC_GLOBAL_REGISTER_BASE + G_RX_RESETN_CTRL);
	writel(0x00000000, EMMC_GLOBAL_REGISTER_BASE + G_TIMER_RESETN_CTRL);

	writel(0x00000001, EMMC_GLOBAL_REGISTER_BASE + G_AXI_RESETN_CTRL);
	writel(0x00000001, EMMC_GLOBAL_REGISTER_BASE + G_BASE_RESETN_CTRL);
	writel(0x00000001, EMMC_GLOBAL_REGISTER_BASE + G_TX_RESETN_CTRL);
	writel(0x00000001, EMMC_GLOBAL_REGISTER_BASE + G_RX_RESETN_CTRL);
	writel(0x00000001, EMMC_GLOBAL_REGISTER_BASE + G_TIMER_RESETN_CTRL);

#ifdef CONFIG_SMARTCHIP_SCA200V100
	sca200v100_rgu_reset_post();
#elif defined(CONFIG_SMARTCHIP_SCA200V200)
	sca200v200_rgu_reset_post();
#endif

}

static void snps_sdhci_set_control_reg(struct sdhci_host *host)
{
#ifdef FPGA_EMMC
	struct mmc *mmc = (struct mmc *)host->mmc;
	unsigned int reg;

	if (!IS_MMC(mmc))
		return;

	/* Force to 1.8V for FPGA to bypass levelshift
	 * or use
	 * if ((mmc->signal_voltage == MMC_SIGNAL_VOLTAGE_180)) {
	 *  ...
	 * }
	 */
	reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	reg |= SDHCI_CTRL_VDD_180;
	sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);

	sdhci_set_uhs_timing(host);
#endif
}

static void snps_sdhci_set_clock(struct sdhci_host *host, u32 div)
{
	/* FPGA clock setting */
	unsigned int reg_val;

#ifdef CONFIG_SMARTCHIP_SCA200V100
	struct smartchip_sdhc_plat *plat = dev_get_platdata(host->mmc->dev);

	reg_val = readl(0x01021018);
	if (div > 1)
		reg_val &= ~(plat->out_phase << 13);
	else
		reg_val |= (1 << 13);
	writel(reg_val, 0x01021018);

	reg_val = readl(EMMC_CLK_CTRL_ADDR);
	reg_val &= ~(CLK_CTRL_FREQ_SEL | CLK_CTRL_CLK_SEL);
	reg_val |= CLK_CTRL_CFG_ON | CLK_CTRL_CRADCLK_ON | CLK_CTRL_CLK2CARD_ON
	    | (div ? CLK_CTRL_CLK_SEL : 0) | (div << 4)
	    | CLK_EMMC_PAD_2_SEL;
	writel(reg_val, EMMC_CLK_CTRL_ADDR);
#elif defined(CONFIG_SMARTCHIP_SCA200V200)
#ifdef FPGA_EMMC
	if (div < 4)
		div = 50;
#endif
	reg_val = readl(CGU_EMMC_FIX_CLK_ADDR);
	reg_val &= ~(0x1 << 28 | 0x3ff << 16);
	reg_val |= 0x1 << 28 | div << 16;
	writel(reg_val, CGU_EMMC_FIX_CLK_ADDR);
	reg_val = readl(CGU_EMMC_SAMPLE_CLK);
	reg_val &= ~(0x1 << 12 | 0x3ff);
	reg_val |= 0x1 << 12 | div;
	writel(reg_val, CGU_EMMC_SAMPLE_CLK);
	reg_val = readl(CGU_EMMC_DRV_CLK);
	reg_val &= ~(0x1 << 28 | 0x3ff << 16);
	reg_val |= 0x1 << 28 | (div << 16) | (0x1 << 27);
	writel(reg_val, CGU_EMMC_DRV_CLK);
#endif
	mdelay(2);

}

static int snps_sdhci_send_cmd12(struct sdhci_host *host)
{
	unsigned int stat = 0;
	unsigned int time = 0;
	ulong start = get_timer(0);
	u32 mask, flags;

	mask = SDHCI_CMD_INHIBIT;
	while (sdhci_readl(host, SDHCI_PRESENT_STATE) & mask) {
		if (time >= 1600) {
			printf("%s: MMC: busy detect timeout", __func__);
			return 0;
		}
		time++;
		udelay(1000);
	}

	flags = SDHCI_CMD_RESP_SHORT_BUSY | SDHCI_CMD_CRC | SDHCI_CMD_INDEX;
	sdhci_writel(host, SDHCI_INT_ALL_MASK, SDHCI_INT_STATUS);
	sdhci_writeb(host, 0xe, SDHCI_TIMEOUT_CONTROL);
	sdhci_writel(host, 0, SDHCI_ARGUMENT);
	sdhci_writew(host, SDHCI_MAKE_CMD(MMC_CMD_STOP_TRANSMISSION, flags), SDHCI_COMMAND);

	mask = SDHCI_INT_RESPONSE;
	start = get_timer(0);
	do {
		stat = sdhci_readl(host, SDHCI_INT_STATUS);
		if (stat & SDHCI_INT_ERROR) {
			printf("%s: Send CMD12 error, stat 0x%x\n",
			    __func__, stat);
			break;
		}

		if (get_timer(start) >= 1000) {
			printf("%s: Timeout for status update!\n",
			    __func__);
			return 0;
		}
	} while ((stat & mask) != mask);

	sdhci_readl(host, SDHCI_RESPONSE);
	sdhci_writel(host, mask, SDHCI_INT_STATUS);

	if (host->quirks & SDHCI_QUIRK_WAIT_SEND_CMD)
		udelay(1000);

	sdhci_writel(host, SDHCI_INT_ALL_MASK, SDHCI_INT_STATUS);

	printf("%s: Send CMD12\n", __func__);

	return 0;
}

static int snps_sdhci_cmd_post_retry(struct sdhci_host *host, struct mmc_cmd *cmd)
{
	if (cmd->cmdidx == MMC_CMD_READ_SINGLE_BLOCK ||
	    cmd->cmdidx == MMC_CMD_READ_MULTIPLE_BLOCK)
		snps_sdhci_send_cmd12(host);

	return 0;
}

static int snps_sdhci_cmd_pre_retry(struct sdhci_host *host, struct mmc_cmd *cmd)
{
#ifdef CONFIG_SMARTCHIP_SCA200V100
	unsigned int reg_val;
#endif

	if (cmd->cmdidx == SD_CMD_SEND_IF_COND ||
	    cmd->cmdidx == MMC_CMD_APP_CMD)
		return 0;

	printf("%s: Reverse phase\n", __func__);
#ifdef CONFIG_SMARTCHIP_SCA200V100
	reg_val = readl(0x01021018);
	reg_val ^= 1 << 13;
	writel(reg_val, 0x01021018);
#endif
	if (cmd->cmdidx == MMC_CMD_READ_SINGLE_BLOCK ||
	    cmd->cmdidx == MMC_CMD_READ_MULTIPLE_BLOCK)
		snps_sdhci_send_cmd12(host);

	return 1;
}

static const struct sdhci_ops snps_sdhci_ops = {
	.set_control_reg = snps_sdhci_set_control_reg,
	.set_clock = snps_sdhci_set_clock,
	.cmd_pre_retry = snps_sdhci_cmd_pre_retry,
	.cmd_post_retry = snps_sdhci_cmd_post_retry,
};

static int snps_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct smartchip_sdhc_plat *plat = dev_get_platdata(dev);
	struct smartchip_sdhc *prv = dev_get_priv(dev);
	struct sdhci_host *host = &prv->host;
	int max_frequency, len, ret;
	struct clk clk;

	debug("%s\n", __func__);
	snps_sdhci_global_reset();

	max_frequency = dev_read_u32_default(dev, "max-frequency", 0);
	ret = clk_get_by_index(dev, 0, &clk);

	if (!ret) {
		ret = clk_set_rate(&clk, max_frequency);
		if (IS_ERR_VALUE(ret))
			printf("%s clk set rate fail!\n", __func__);
	} else {
		debug("%s fail to get clk\n", __func__);
	}

	if (dev_read_prop(dev, "out-phase", &len)) {
		debug("Config emmc to out-phase.\n");
		plat->out_phase = 0;
	} else {
		debug("Config emmc to same-phase.\n");
		plat->out_phase = 1;
	}

	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD |
	    SDHCI_QUIRK_HOST_CMD_RETRY;
	host->max_clk = max_frequency;
	host->ops = &snps_sdhci_ops;
	/*
	 * The sdhci-driver only supports 4bit and 8bit, as sdhci_setup_cfg
	 * doesn't allow us to clear MMC_MODE_4BIT.  Consequently, we don't
	 * check for other bus-width values.
	 */
	if (host->bus_width == 8)
		host->host_caps |= MMC_MODE_8BIT;

	host->mmc = &plat->mmc;
	host->mmc->priv = &prv->host;
	host->mmc->dev = dev;
	upriv->mmc = host->mmc;

	ret = sdhci_setup_cfg(&plat->cfg, host, 0, EMMC_MIN_FREQ);
	if (ret)
		return ret;

	ret = sdhci_probe(dev);
	if (ret)
		return ret;

	sdhci_writeb(host, SDHCI_SW_CG_DIS, SDHCI_MSHC_CTRL_R);

	return 0;
}

static int snps_sdhci_ofdata_to_platdata(struct udevice *dev)
{
	struct sdhci_host *host = dev_get_priv(dev);

	debug("%s\n", __func__);
	host->name = dev->name;
	host->ioaddr = dev_read_addr_ptr(dev);
	host->bus_width = dev_read_u32_default(dev, "bus-width", 4);

	return 0;
}

static int smartchip_sdhci_bind(struct udevice *dev)
{
	struct smartchip_sdhc_plat *plat = dev_get_platdata(dev);

	debug("%s\n", __func__);
	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id snps_sdhci_ids[] = {
	{ .compatible = "snps,sdhci" },
	{ }
};

U_BOOT_DRIVER(snps_sdhci_drv) = {
	.name       = "smartchip_sdhci",
	.id     = UCLASS_MMC,
	.of_match   = snps_sdhci_ids,
	.ofdata_to_platdata = snps_sdhci_ofdata_to_platdata,
	.ops        = &sdhci_ops,
	.bind       = smartchip_sdhci_bind,
	.probe      = snps_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct smartchip_sdhc),
	.platdata_auto_alloc_size = sizeof(struct smartchip_sdhc_plat),
};
