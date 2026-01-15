/*
 * sdhci-dove.c Support for SDHCI on Marvell's Dove SoC
 *
 * Author: Saeed Bishara <saeed@marvell.com>
 *     Mike Rapoport <mike@compulab.co.il>
 * Based on sdhci-cns3xxx.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mmc/host.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/delay.h>

#include "sdhci-pltfm.h"

#define MAX_PHASES              8

#define RGU_RESET0_N_CTRL           (0x0A102000)
#define RGU_RESET0_MASK             (0x0A102008)
#define  RGU_EMMC_CORE_RSTN         (0x1 << 28)
#define RGU_RESET1_N_CTRL           (0x0A102010)
#define RGU_RESET1_MASK             (0x0A102018)
#define  RGU_EMMC_HRSTN             (0x1)

#define SDHCI_PROXIMA_RESET_AXI         (0x1004)
#define SDHCI_PROXIMA_RESET_BASE        (0x1008)
#define SDHCI_PROXIMA_RESET_TX          (0x100c)
#define SDHCI_PROXIMA_RESET_RX          (0x1010)
#define SDHCI_PROXIMA_RESET_TIMER       (0x1014)

#define ABB_REG32_SDCPLL            (0x0A108080)
#define  ABB_EMMC_CLK_DRV_PHASE_MASK        (0x07)
#define  ABB_EMMC_CLK_DRV_PHASE_ENABLE      (0x08)
#define  ABB_EMMC_CLK_FREQ_25M          (0x00)
#define  ABB_EMMC_CLK_FREQ_50M          (0x40)
#define  ABB_EMMC_CLK_FREQ_100M         (0x80)
#define  ABB_EMMC_CLK_FREQ_200M         (0xC0)
#define  ABB_EMMC_CLK_FREQ_MASK         (0xC0)

#define ABB_REG33_SDCPLL            (0x0A108084)
#define  ABB_EMMC_CLK_SAMPLE_PHASE_MASK     (0x0F)

#define CGU_CLK_CTRL11              (0x0A10402C)
#define  CGU_EMMC_CLK_FIX_DIV_MASK      (0x3FF << 16)
#define  CGU_EMMC_CLK_FIX_ENABLE        (0x1 << 28)
#define  CGU_EMMC_CLK_FIX_SEL_MASK      (0x7 << 29)

#define CGU_CLK_CTRL12              (0x0A104030)
#define  CGU_EMMC_CLK_SAMPLE_DIV_MASK       (0x3FF << 0)
#define  CGU_EMMC_CLK_SAMPLE_DRV_2FIX_MUX   (0x1 << 10)
#define  CGU_EMMC_CLK_SAMPLE_ENABLE     (0x1 << 12)
#define  CGU_EMMC_CLK_SAMPLE_SEL_MASK       (0x7 << 13)
#define  CGU_EMMC_CLK_DRV_DIV_MASK      (0x3FF << 16)
#define  CGU_EMMC_CLK_DRV_INV           (0x1 << 27)
#define  CGU_EMMC_CLK_DRV_ENABLE        (0x1 << 28)
#define  CGU_EMMC_CLK_DRV_SEL_MASK      (0x7 << 29)

#define PINMUX_GPIO_MSC             (0x0A10A000)
#define  PINMUX_GPIO_MODE_1V8           (0x1 << 5)

struct sdhci_proxima_dev {
	void __iomem *rgu_reset0_n_ctrl;
	void __iomem *rgu_reset0_mask;
	void __iomem *rgu_reset1_n_ctrl;
	void __iomem *rgu_reset1_mask;
	void __iomem *abb_reg32_sdcpll;
	void __iomem *abb_reg33_sdcpll;
	void __iomem *cgu_clk_ctrl11;
	void __iomem *cgu_clk_ctrl12;
	void __iomem *pinmux_gpio_msc;
};

static void sdhci_proxima_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct sdhci_proxima_dev *proxima_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;
	u32 reg_val = 0;
	u32 base_clock = 0;
	u32 div = 0;

	pltfm_host = sdhci_priv(host);
	proxima_dev_priv = sdhci_pltfm_priv(pltfm_host);
	sdhci_set_clock(host, clock);

	reg_val = readl(proxima_dev_priv->cgu_clk_ctrl12);
	reg_val &= ~(CGU_EMMC_CLK_DRV_ENABLE);
	writel(reg_val, proxima_dev_priv->cgu_clk_ctrl12);
	if (clock == 0) {
		mdelay(2);
		return;
	}

	div = 0;
	if (clock < 50000000) {
		base_clock = ABB_EMMC_CLK_FREQ_25M;
		if (clock < 25000000) {
			for (div = 0; div < 0x3ff; div++) {
				if ((25000000 / (div + 1)) <= clock)
					break;
			}
		}
	} else if (clock < 100000000) {
		base_clock = ABB_EMMC_CLK_FREQ_50M;
	} else if (clock < 200000000) {
		base_clock = ABB_EMMC_CLK_FREQ_100M;
	} else {
		base_clock = ABB_EMMC_CLK_FREQ_200M;
	}

	/* Set frequence of base clock */
	reg_val = readl(proxima_dev_priv->abb_reg32_sdcpll);
	reg_val &= ~(ABB_EMMC_CLK_FREQ_MASK);
	reg_val |= base_clock;
	writel(reg_val, proxima_dev_priv->abb_reg32_sdcpll);

	/* Set frequence of fix clock */
	reg_val = readl(proxima_dev_priv->cgu_clk_ctrl11);
	reg_val &= ~(CGU_EMMC_CLK_FIX_DIV_MASK | CGU_EMMC_CLK_FIX_ENABLE |
	        CGU_EMMC_CLK_FIX_SEL_MASK);
	reg_val |= div << 16 | CGU_EMMC_CLK_FIX_ENABLE;
	writel(reg_val, proxima_dev_priv->cgu_clk_ctrl11);

	/* Set frequence of drv and sample clock
	 * Invert phase of drv clock
	 */
	reg_val = readl(proxima_dev_priv->cgu_clk_ctrl12);
	reg_val &= ~(CGU_EMMC_CLK_SAMPLE_DIV_MASK | CGU_EMMC_CLK_SAMPLE_DRV_2FIX_MUX |
	        CGU_EMMC_CLK_SAMPLE_ENABLE | CGU_EMMC_CLK_SAMPLE_SEL_MASK |
	        CGU_EMMC_CLK_DRV_DIV_MASK | CGU_EMMC_CLK_DRV_INV |
	        CGU_EMMC_CLK_DRV_ENABLE | CGU_EMMC_CLK_DRV_SEL_MASK);
	reg_val |= div | (div ? CGU_EMMC_CLK_SAMPLE_DRV_2FIX_MUX : 0) |
	    CGU_EMMC_CLK_SAMPLE_ENABLE | (div << 16) |
	    CGU_EMMC_CLK_DRV_INV | CGU_EMMC_CLK_DRV_ENABLE;
	writel(reg_val, proxima_dev_priv->cgu_clk_ctrl12);
	mdelay(2);
}

/*
 * Find out the greatest range of consecuitive selected
 * DLL clock output phases that can be used as sampling
 * setting for SD3.0 UHS-I card read operation (in SDR104
 * timing mode) or for eMMC4.5 card read operation (in HS200
 * timing mode).
 * Select the 3/4 of the range and configure the DLL with the
 * selected DLL clock output phase.
 */
static int sdhci_proxima_find_appropriate_phase(struct sdhci_host *host, u8 *phase_table,
    u8 total_phases)
{
	struct mmc_host *mmc = host->mmc;
	u8 ranges[MAX_PHASES][MAX_PHASES] = { {0}, {0} };
	u8 phases_per_row[MAX_PHASES] = { 0 };
	int row_index = 0, col_index = 0, selected_row_index = 0, curr_max = 0;
	int i, cnt, phase_zero_raw_index = 0, phase_max_raw_index = 0;
	bool phase_zero_found = false, phase_max_found = false;
	int ret;

	if (!total_phases || (total_phases > MAX_PHASES)) {
		dev_err(mmc_dev(mmc), "%s: Invalid argument: total_phases %d\n",
		    mmc_hostname(mmc), total_phases);
		return -EINVAL;
	}

	for (cnt = 0; cnt < total_phases; cnt++) {
		ranges[row_index][col_index] = phase_table[cnt];
		phases_per_row[row_index] += 1;
		col_index++;

		/* check if next phase in phase_table is consecutive or not */
		if ((phase_table[cnt] + 1) != phase_table[cnt + 1]) {
			row_index++;
			col_index = 0;
		}
	}

	if (row_index >= MAX_PHASES)
		return -EINVAL;

	/* Check if phase-0 is present in first valid window? */
	if (!ranges[0][0]) {
		phase_zero_found = true;
		phase_zero_raw_index = 0;
		/* Check if cycle exist between 2 valid windows */
		for (cnt = 1; cnt <= row_index; cnt++) {
			if (phases_per_row[cnt]) {
				for (i = 0; i < phases_per_row[cnt]; i++) {
					if (ranges[cnt][i] == MAX_PHASES) {
						phase_max_found = true;
						phase_max_raw_index = cnt;
						break;
					}
				}
			}
		}
	}

	/* If 2 valid windows form cycle then merge them as single window */
	if (phase_zero_found && phase_max_found) {
		/* number of phases in raw where phase 0 is present */
		u8 phases_zero = phases_per_row[phase_zero_raw_index];
		/* number of phases in raw where phase 15 is present */
		u8 phases_max = phases_per_row[phase_max_raw_index];

		if (phases_zero + phases_max >= MAX_PHASES)
			/*
			 * If there are more than 1 phase windows then total
			 * number of phases in both the windows should not be
			 * more than or equal to MAX_PHASES.
			 */
			return -EINVAL;

		/* Merge 2 cyclic windows */
		i = phases_max;
		for (cnt = 0; cnt < phases_zero; cnt++) {
			ranges[phase_max_raw_index][i] =
			    ranges[phase_zero_raw_index][cnt];
			if (++i >= MAX_PHASES)
				break;
		}

		phases_per_row[phase_zero_raw_index] = 0;
		phases_per_row[phase_max_raw_index] = phases_max + phases_zero;
	}

	for (cnt = 0; cnt <= row_index; cnt++) {
		if (phases_per_row[cnt] > curr_max) {
			curr_max = phases_per_row[cnt];
			selected_row_index = cnt;
		}
	}

	i = curr_max / 2;
	if (i)
		i--;

	ret = ranges[selected_row_index][i];

	if (ret >= MAX_PHASES) {
		ret = -EINVAL;
		dev_err(mmc_dev(mmc), "%s: Invalid phase selected=%d\n",
		    mmc_hostname(mmc), ret);
	}

	return ret;
}

static int sdhci_proxima_set_phase(struct sdhci_host *host, u8 phase)
{
	struct sdhci_proxima_dev *proxima_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;
	u32 reg_val = 0;

	pltfm_host = sdhci_priv(host);
	proxima_dev_priv = sdhci_pltfm_priv(pltfm_host);

	/* Close fix clock, drv clock and sample clock */
	reg_val = readl(proxima_dev_priv->cgu_clk_ctrl11);
	reg_val &= ~(CGU_EMMC_CLK_FIX_ENABLE);
	writel(reg_val, proxima_dev_priv->cgu_clk_ctrl11);
	reg_val = readl(proxima_dev_priv->cgu_clk_ctrl12);
	reg_val &= ~(CGU_EMMC_CLK_SAMPLE_ENABLE | CGU_EMMC_CLK_DRV_ENABLE);
	writel(reg_val, proxima_dev_priv->cgu_clk_ctrl12);
	mdelay(1);

	/*config phase*/
	reg_val = readl(proxima_dev_priv->abb_reg33_sdcpll);
	reg_val &= ~ABB_EMMC_CLK_SAMPLE_PHASE_MASK;
	reg_val |= phase;
	writel(reg_val, proxima_dev_priv->abb_reg33_sdcpll);

	/* Open fix clock, drv clock and sample clock */
	reg_val = readl(proxima_dev_priv->cgu_clk_ctrl11);
	reg_val |= CGU_EMMC_CLK_FIX_ENABLE;
	writel(reg_val, proxima_dev_priv->cgu_clk_ctrl11);
	reg_val = readl(proxima_dev_priv->cgu_clk_ctrl12);
	reg_val |= (CGU_EMMC_CLK_SAMPLE_ENABLE | CGU_EMMC_CLK_DRV_ENABLE);
	writel(reg_val, proxima_dev_priv->cgu_clk_ctrl12);
	mdelay(1);

	return 0;
}

static int sdhci_proxima_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	struct mmc_host *mmc = host->mmc;
	u8 phase, tuned_phases[MAX_PHASES], tuned_phase_cnt = 0;
	int tuning_seq_cnt = 3;
	int ret;

retry:
	phase = 0;
	do {
		ret = sdhci_proxima_set_phase(host, phase);
		if (ret)
			return ret;
		ret = mmc_send_tuning(mmc, opcode, NULL);
		if (!ret) {
			/* Tuning is successful at this tuning phase */
			tuned_phases[tuned_phase_cnt++] = phase;
			dev_info(mmc_dev(mmc), "%s: Found good phase = %d\n",
			    mmc_hostname(mmc), phase);
		}
	} while (++phase < MAX_PHASES);

	if (tuned_phase_cnt) {
		ret = sdhci_proxima_find_appropriate_phase(host, tuned_phases,
		        tuned_phase_cnt);
		if (ret < 0)
			return ret;
		else
			phase = ret;

		/*
		 * Finally set the selected phase
		 */
		ret = sdhci_proxima_set_phase(host, phase);
		if (ret)
			return ret;
		dev_info(mmc_dev(mmc), "%s: Setting the tuning phase to %d\n",
		    mmc_hostname(mmc), phase);
	} else {
		if (--tuning_seq_cnt)
			goto retry;
		/* Tuning failed */
		dev_dbg(mmc_dev(mmc), "%s: No tuning point found\n",
		    mmc_hostname(mmc));
		ret = -EIO;
	}

	return ret;
}

void sdhci_proxima_voltage_switch(struct sdhci_host *host)
{
	struct sdhci_proxima_dev *proxima_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;
	int pin_mod;
	u16 ctrl;

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	pltfm_host = sdhci_priv(host);
	proxima_dev_priv = sdhci_pltfm_priv(pltfm_host);

	pin_mod = readl(proxima_dev_priv->pinmux_gpio_msc);

	if (!(ctrl & SDHCI_CTRL_VDD_180))
		pin_mod &= ~(PINMUX_GPIO_MODE_1V8);
	else if (ctrl & SDHCI_CTRL_VDD_180)
		pin_mod |= PINMUX_GPIO_MODE_1V8;

	writel(pin_mod, proxima_dev_priv->pinmux_gpio_msc);
}

static const struct sdhci_ops sdhci_proxima_ops = {
	.platform_execute_tuning = sdhci_proxima_execute_tuning,
	.reset = sdhci_reset,
	.set_clock = sdhci_proxima_set_clock,
	.set_bus_width = sdhci_set_bus_width,
	.set_uhs_signaling = sdhci_set_uhs_signaling,
	.voltage_switch = sdhci_proxima_voltage_switch,
};

static const struct sdhci_pltfm_data sdhci_proxima_pdata = {
	.ops    = &sdhci_proxima_ops,
	.quirks = SDHCI_QUIRK_32BIT_ADMA_SIZE | SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC |
	SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12,
	.quirks2 = SDHCI_QUIRK2_BROKEN_DDR50
};

static void sdhci_proxima_global_rst(struct sdhci_host *host)
{
	struct sdhci_proxima_dev *proxima_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;
	u32 val;

	pltfm_host = sdhci_priv(host);
	proxima_dev_priv = sdhci_pltfm_priv(pltfm_host);

	writel(~(RGU_EMMC_CORE_RSTN), proxima_dev_priv->rgu_reset0_mask);
	val = readl(proxima_dev_priv->rgu_reset0_n_ctrl);
	val |= RGU_EMMC_CORE_RSTN;
	writel(val, proxima_dev_priv->rgu_reset0_n_ctrl);
	writel(~(RGU_EMMC_HRSTN), proxima_dev_priv->rgu_reset1_mask);
	val = readl(proxima_dev_priv->rgu_reset1_n_ctrl);
	val |= RGU_EMMC_HRSTN;
	writel(val, proxima_dev_priv->rgu_reset1_n_ctrl);

	sdhci_writeb(host, 0x00, SDHCI_PROXIMA_RESET_AXI);
	sdhci_writeb(host, 0x00, SDHCI_PROXIMA_RESET_BASE);
	sdhci_writeb(host, 0x00, SDHCI_PROXIMA_RESET_TX);
	sdhci_writeb(host, 0x00, SDHCI_PROXIMA_RESET_RX);
	sdhci_writeb(host, 0x00, SDHCI_PROXIMA_RESET_TIMER);
	udelay(10);
	sdhci_writeb(host, 0x01, SDHCI_PROXIMA_RESET_AXI);
	sdhci_writeb(host, 0x01, SDHCI_PROXIMA_RESET_BASE);
	sdhci_writeb(host, 0x01, SDHCI_PROXIMA_RESET_TX);
	sdhci_writeb(host, 0x01, SDHCI_PROXIMA_RESET_RX);
	sdhci_writeb(host, 0x01, SDHCI_PROXIMA_RESET_TIMER);
}

static int sdhci_proxima_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_proxima_dev *proxima_dev_priv = NULL;
	//  u16 ctrl = 0;
	int ret = 0;

	host = sdhci_pltfm_init(pdev, &sdhci_proxima_pdata, sizeof(struct sdhci_proxima_dev));
	if (IS_ERR(host))
		return PTR_ERR(host);

	pltfm_host = sdhci_priv(host);
	proxima_dev_priv = sdhci_pltfm_priv(pltfm_host);

	proxima_dev_priv->rgu_reset0_n_ctrl = devm_ioremap(&pdev->dev, RGU_RESET0_N_CTRL, 0x4);
	if (!proxima_dev_priv->rgu_reset0_n_ctrl) {
		dev_err(&pdev->dev, "ioremap failed for resource RGU_RESET0_N_CTRL\n");
		ret = -ENOMEM;
	}

	proxima_dev_priv->rgu_reset0_mask = devm_ioremap(&pdev->dev, RGU_RESET0_MASK, 0x4);
	if (!proxima_dev_priv->rgu_reset0_mask) {
		dev_err(&pdev->dev, "ioremap failed for resource RGU_RESET0_MASK\n");
		ret = -ENOMEM;
	}

	proxima_dev_priv->rgu_reset1_n_ctrl = devm_ioremap(&pdev->dev, RGU_RESET1_N_CTRL, 0x4);
	if (!proxima_dev_priv->rgu_reset1_n_ctrl) {
		dev_err(&pdev->dev, "ioremap failed for resource RGU_RESET1_N_CTRL\n");
		ret = -ENOMEM;
	}

	proxima_dev_priv->rgu_reset1_mask = devm_ioremap(&pdev->dev, RGU_RESET1_MASK, 0x4);
	if (!proxima_dev_priv->rgu_reset1_mask) {
		dev_err(&pdev->dev, "ioremap failed for resource RGU_RESET1_MASK\n");
		ret = -ENOMEM;
	}

	sdhci_proxima_global_rst(host);

	proxima_dev_priv->abb_reg32_sdcpll = devm_ioremap(&pdev->dev, ABB_REG32_SDCPLL, 0x4);
	if (!proxima_dev_priv->abb_reg32_sdcpll) {
		dev_err(&pdev->dev, "ioremap failed for resource ABB_REG32_SDCPLL\n");
		ret = -ENOMEM;
	}

	proxima_dev_priv->abb_reg33_sdcpll = devm_ioremap(&pdev->dev, ABB_REG33_SDCPLL, 0x4);
	if (!proxima_dev_priv->abb_reg32_sdcpll) {
		dev_err(&pdev->dev, "ioremap failed for resource ABB_REG33_SDCPLL\n");
		ret = -ENOMEM;

	}

	proxima_dev_priv->cgu_clk_ctrl11 = devm_ioremap(&pdev->dev, CGU_CLK_CTRL11, 0x4);
	if (!proxima_dev_priv->cgu_clk_ctrl11) {
		dev_err(&pdev->dev, "ioremap failed for resource CGU_CLK_CTRL11\n");
		ret = -ENOMEM;
	}

	proxima_dev_priv->cgu_clk_ctrl12 = devm_ioremap(&pdev->dev, CGU_CLK_CTRL12, 0x4);
	if (!proxima_dev_priv->cgu_clk_ctrl12) {
		dev_err(&pdev->dev, "ioremap failed for resource CGU_CLK_CTRL12\n");
		ret = -ENOMEM;
	}

	proxima_dev_priv->pinmux_gpio_msc = devm_ioremap(&pdev->dev, PINMUX_GPIO_MSC, 0x4);
	if (!proxima_dev_priv->pinmux_gpio_msc) {
		dev_err(&pdev->dev, "ioremap failed for resource PINMUX_GPIO_MSC\n");
		ret = -ENOMEM;
	}
	if (ret)
		goto err_sdhci_add;

	pltfm_host->clk = devm_clk_get(&pdev->dev, NULL);
	if (!IS_ERR(pltfm_host->clk))
		clk_prepare_enable(pltfm_host->clk);

	ret = mmc_of_parse(host->mmc);
	if (ret)
		goto err_sdhci_add;

	sdhci_get_of_property(pdev);
	mmc_of_parse_voltage(pdev->dev.of_node, &host->ocr_mask);

	ret = sdhci_add_host(host);
	if (ret)
		goto err_sdhci_add;
#if 0
	ctrl = readl(proxima_dev_priv->emmc_clk_ctrl);
	writel(0, proxima_dev_priv->emmc_clk_ctrl);
	usleep_range(1000, 1100);
	writel(0, proxima_dev_priv->emmc_pll);
	writel(ctrl, proxima_dev_priv->emmc_clk_ctrl);

	usleep_range(1000, 1100);

	if (!(host->mmc->caps2 & MMC_CAP2_NO_MMC)) {

		ctrl = sdhci_readw(host, SDHCI_EMMC_CTRL_R);
		ctrl |= SDHCI_EMMC_CARD << SDHCI_EMMC_CARD;
		sdhci_writew(host, ctrl, SDHCI_EMMC_CTRL_R);

		ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		ctrl |= SDHCI_CTRL_HOST_VER4_ENABLE;
		ctrl &= (~SDHCI_CTRL_UHS_IF_ENABLE);
		sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
	}
#endif
	return 0;

err_sdhci_add:
	clk_disable_unprepare(pltfm_host->clk);
	sdhci_pltfm_free(pdev);

	return ret;
}

static const struct of_device_id sdhci_proxima_of_match_table[] = {
	{ .compatible = "smartchip,proxima-sdhc", },
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_proxima_of_match_table);

static struct platform_driver sdhci_proxima_driver = {
	.driver     = {
		.name   = "sdhci-proxima",
		.pm = &sdhci_pltfm_pmops,
		.of_match_table = sdhci_proxima_of_match_table,
	},
	.probe      = sdhci_proxima_probe,
	.remove     = sdhci_pltfm_unregister,
};

module_platform_driver(sdhci_proxima_driver);

MODULE_DESCRIPTION("SDHCI driver for proxima");
MODULE_AUTHOR("dongyangmeng <dongyang.meng@smartchip.cn>");
MODULE_LICENSE("GPL v2");
