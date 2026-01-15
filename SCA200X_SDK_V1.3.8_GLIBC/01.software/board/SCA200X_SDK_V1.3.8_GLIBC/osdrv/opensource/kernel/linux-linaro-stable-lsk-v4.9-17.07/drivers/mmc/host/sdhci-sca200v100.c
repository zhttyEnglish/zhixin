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

#define CLK_CTRL_CFG_ON         0x1
#define CLK_CTRL_CRADCLK_ON     0x2
#define CLK_CTRL_CLK2CARD_ON        0x4
#define CLK_CTRL_CLK_SEL        0x8
#define CLK_CTRL_FREQ_SEL       0x3ff0
#define CLK_EMMC_PAD_2_SEL      0x4000

#define MAX_PHASES          16

#define POLESTAR_EMMC_CLK_CTRL      (0x01070034)
#define POLESTAR_EMMC_PLL       (0x01072080)
#define POLESTAR_EMMC_PLL_PHASE     (0x01072084)
#define POLESTAR_EMMC_PB_MSG_REG00  (0x01073104)

#define POLESTAR_EMMC_RESET_CTRL    (0x01071034)

#define POLESTAR_MISC_CTRL      (0x01021018)

#define CLK_SEL_SDC_50M         0x00
#define CLK_SEL_SDC_100M        0x40
#define CLK_SEL_SDC_200M        0x80
#define CLK_SEL_SDC_FREQ_MASK       0xc0
#define POLESTAR_EMMC_AXI_RESET_CTL (0x1004)
#define POLESTAR_EMMC_BASE_RESET_CTL    (0x1008)
#define POLESTAR_EMMC_TX_RESET_CTL  (0x100c)
#define POLESTAR_EMMC_RX_RESET_CTL  (0x1010)
#define POLESTAR_EMMC_TIMER_RESET_CTL   (0x1014)

struct sdhci_smartchip_dev {
	struct mutex    write_lock; /* protect back to back writes */
	void __iomem *emmc_pll_phase;
	void __iomem *emmc_pll;
	void __iomem *emmc_clk_ctrl;
	void __iomem *pb_msd_reg00;
	void __iomem *misc_ctrl;
	void __iomem *emmc_reset_ctrl;
	void __iomem *emmc_secure;
	u32 out_phase;
};

static void sdhci_smartx_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct sdhci_smartchip_dev *smartchip_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;
	u32 div = 0;
	u32 reg_val = 0;
	u32 base_clock = 0;

	pltfm_host = sdhci_priv(host);
	smartchip_dev_priv = sdhci_pltfm_priv(pltfm_host);

	sdhci_set_clock(host, clock);

	reg_val = readl(smartchip_dev_priv->emmc_clk_ctrl);
	reg_val &= ~CLK_CTRL_CLK2CARD_ON;
	writel(reg_val, smartchip_dev_priv->emmc_clk_ctrl);

	if (clock == 0) {
		mdelay(2);
		return;
	}

	/*
	 * Config emmc sample clock and drv clock to out-phase
	 * or in-phase for clock below 25MHz
	 */
	reg_val = readl(smartchip_dev_priv->misc_ctrl);
	if (clock < 25000000 && smartchip_dev_priv->out_phase)
		reg_val &= ~(1 << 13);
	else
		reg_val |= 1 << 13;
	writel(reg_val, smartchip_dev_priv->misc_ctrl);

	if (clock < 100000000) {
		base_clock = CLK_SEL_SDC_50M;
		if (clock >= 50000000) {
			div = 0;
		} else {
			for (div = 1; div < 0x3ff; div++) {
				if ((50000000 / (div * 2)) <= clock)
					break;
			}
		}
	} else if (clock < 200000000) {
		base_clock = CLK_SEL_SDC_100M;
		div = 0;
	} else {
		base_clock = CLK_SEL_SDC_200M;
		div = 0;
	}

	reg_val = readl(smartchip_dev_priv->emmc_pll);
	reg_val &= ~(CLK_SEL_SDC_FREQ_MASK);
	reg_val |= base_clock;
	writel(reg_val, smartchip_dev_priv->emmc_pll);

	reg_val = readl(smartchip_dev_priv->emmc_clk_ctrl);
	reg_val &= ~(CLK_CTRL_FREQ_SEL | CLK_CTRL_CLK_SEL);
	reg_val |= CLK_CTRL_CFG_ON | CLK_CTRL_CRADCLK_ON | CLK_CTRL_CLK2CARD_ON
	    | (div ? CLK_CTRL_CLK_SEL : 0) | (div << 4)
	    | CLK_EMMC_PAD_2_SEL;
	writel(reg_val, smartchip_dev_priv->emmc_clk_ctrl);
	mdelay(2);
	/*
	pr_err("clock=%d emmc_pll=0x%x emmc_clk_ctrl=0x%x\n",
	        clock, readl(smartchip_dev_priv->emmc_pll), readl(smartchip_dev_priv->emmc_clk_ctrl));
	        */
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

static int sc_find_most_appropriate_phase(struct sdhci_host *host,
    u8 *phase_table, u8 total_phases)
{
	int ret;
	u8 ranges[MAX_PHASES][MAX_PHASES] = { {0}, {0} };
	u8 phases_per_row[MAX_PHASES] = { 0 };
	int row_index = 0, col_index = 0, selected_row_index = 0, curr_max = 0;
	int i, cnt, phase_0_raw_index = 0, phase_15_raw_index = 0;
	bool phase_0_found = false, phase_15_found = false;
	struct mmc_host *mmc = host->mmc;

	if (!total_phases || (total_phases > MAX_PHASES)) {
		dev_err(mmc_dev(mmc), "%s: Invalid argument: total_phases=%d\n",
		    mmc_hostname(mmc), total_phases);
		return -EINVAL;
	}

	for (cnt = 0; cnt < total_phases; cnt++) {
		ranges[row_index][col_index] = phase_table[cnt];
		phases_per_row[row_index] += 1;
		col_index++;

		if ((cnt + 1) == total_phases) {
			continue;
			/* check if next phase in phase_table is consecutive or not */
		} else if ((phase_table[cnt] + 1) != phase_table[cnt + 1]) {
			row_index++;
			col_index = 0;
		}
	}

	if (row_index >= MAX_PHASES)
		return -EINVAL;

	/* Check if phase-0 is present in first valid window? */
	if (!ranges[0][0]) {
		phase_0_found = true;
		phase_0_raw_index = 0;
		/* Check if cycle exist between 2 valid windows */
		for (cnt = 1; cnt <= row_index; cnt++) {
			if (phases_per_row[cnt]) {
				for (i = 0; i < phases_per_row[cnt]; i++) {
					if (ranges[cnt][i] == 15) {
						phase_15_found = true;
						phase_15_raw_index = cnt;
						break;
					}
				}
			}
		}
	}

	/* If 2 valid windows form cycle then merge them as single window */
	if (phase_0_found && phase_15_found) {
		/* number of phases in raw where phase 0 is present */
		u8 phases_0 = phases_per_row[phase_0_raw_index];
		/* number of phases in raw where phase 15 is present */
		u8 phases_15 = phases_per_row[phase_15_raw_index];

		if (phases_0 + phases_15 >= MAX_PHASES)
			/*
			 * If there are more than 1 phase windows then total
			 * number of phases in both the windows should not be
			 * more than or equal to MAX_PHASES.
			 */
			return -EINVAL;

		/* Merge 2 cyclic windows */
		i = phases_15;
		for (cnt = 0; cnt < phases_0; cnt++) {
			ranges[phase_15_raw_index][i] =
			    ranges[phase_0_raw_index][cnt];
			if (++i >= MAX_PHASES)
				break;
		}

		phases_per_row[phase_0_raw_index] = 0;
		phases_per_row[phase_15_raw_index] = phases_15 + phases_0;
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

static int sc_config_cm_dll_phase(struct sdhci_host *host, u8 phase)
{
	struct sdhci_smartchip_dev *smartchip_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;

	u32 ctl = 0;
	u32 pll_val = 0;

	static const u8 grey_coded_phase_table[] = {
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
		0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
	};

	pltfm_host = sdhci_priv(host);
	smartchip_dev_priv = sdhci_pltfm_priv(pltfm_host);

	ctl = readl(smartchip_dev_priv->emmc_clk_ctrl);
	writel(ctl & ~(0x4), smartchip_dev_priv->emmc_clk_ctrl);

	/*config phase*/
	pll_val = readl(smartchip_dev_priv->emmc_pll_phase);
	pll_val &= ~(0x1f);
	pll_val |= grey_coded_phase_table[phase];
	writel(pll_val, smartchip_dev_priv->emmc_pll_phase);

	/*open clk*/
	writel(ctl, smartchip_dev_priv->emmc_clk_ctrl);
	mdelay(1);

	return 0;
}

static int sdhci_sc_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	int tuning_seq_cnt = 3;
	u8 phase, tuned_phases[MAX_PHASES], tuned_phase_cnt = 0;
	int rc;
	struct mmc_host *mmc = host->mmc;
	//  struct mmc_ios ios = host->mmc->ios;

#if 0
	/*
	 * Tuning is required for SDR104, HS200 and HS400 cards and
	 * if clock frequency is greater than 100MHz in these modes.
	 */
	if (host->clock <= 100 * 1000 * 1000 ||
	    !((ios.timing == MMC_TIMING_MMC_HS200) ||
	        (ios.timing == MMC_TIMING_UHS_SDR104)))
		return 0;
#endif

retry:
	phase = 0;
	do {
		/* Set the phase in delay line hw block */
		rc = sc_config_cm_dll_phase(host, phase);
		if (rc)
			return rc;

		rc = mmc_send_tuning(mmc, opcode, NULL);
		if (!rc) {
			/* Tuning is successful at this tuning point */
			tuned_phases[tuned_phase_cnt++] = phase;
			dev_info(mmc_dev(mmc), "%s: Found good phase = %d\n",
			    mmc_hostname(mmc), phase);
		}
	} while (++phase < ARRAY_SIZE(tuned_phases));

	if (tuned_phase_cnt) {
		rc = sc_find_most_appropriate_phase(host, tuned_phases,
		        tuned_phase_cnt);
		if (rc < 0)
			return rc;
		else
			phase = rc;

		/*
		 * Finally set the selected phase in delay
		 * line hw block.
		 */
		rc = sc_config_cm_dll_phase(host, phase);
		if (rc)
			return rc;
		dev_info(mmc_dev(mmc), "%s: Setting the tuning phase to %d\n",
		    mmc_hostname(mmc), phase);
	} else {
		if (--tuning_seq_cnt)
			goto retry;
		/* Tuning failed */
		dev_dbg(mmc_dev(mmc), "%s: No tuning point found\n",
		    mmc_hostname(mmc));
		rc = -EIO;
	}

	return rc;
}

static void sdhci_smartchip_sw_rst(struct sdhci_host *host)
{
	struct sdhci_smartchip_dev *smartchip_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;

	pltfm_host = sdhci_priv(host);
	smartchip_dev_priv = sdhci_pltfm_priv(pltfm_host);

	writel(0x0, smartchip_dev_priv->emmc_reset_ctrl);
	writel(0x3, smartchip_dev_priv->emmc_reset_ctrl);

	sdhci_writeb(host, 0x00, POLESTAR_EMMC_AXI_RESET_CTL);
	sdhci_writeb(host, 0x01, POLESTAR_EMMC_AXI_RESET_CTL);
	sdhci_writeb(host, 0x00, POLESTAR_EMMC_BASE_RESET_CTL);
	sdhci_writeb(host, 0x01, POLESTAR_EMMC_BASE_RESET_CTL);
	sdhci_writeb(host, 0x00, POLESTAR_EMMC_TX_RESET_CTL);
	sdhci_writeb(host, 0x01, POLESTAR_EMMC_TX_RESET_CTL);
	sdhci_writeb(host, 0x00, POLESTAR_EMMC_RX_RESET_CTL);
	sdhci_writeb(host, 0x01, POLESTAR_EMMC_RX_RESET_CTL);
	sdhci_writeb(host, 0x00, POLESTAR_EMMC_TIMER_RESET_CTL);
	sdhci_writeb(host, 0x01, POLESTAR_EMMC_TIMER_RESET_CTL);

	/* Delay 1ms for stable */
	udelay(1000);
}

void sdhci_smartchip_voltage_switch(struct sdhci_host *host)
{
	struct sdhci_smartchip_dev *smartchip_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;
	int pb_vol;
	u16 ctrl;

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	pltfm_host = sdhci_priv(host);
	smartchip_dev_priv = sdhci_pltfm_priv(pltfm_host);

	pb_vol = readl(smartchip_dev_priv->pb_msd_reg00);

	if (!(ctrl & SDHCI_CTRL_VDD_180))
		pb_vol &= ~(0x18);
	else if (ctrl & SDHCI_CTRL_VDD_180)
		pb_vol |= 0x18;

	writel(pb_vol, smartchip_dev_priv->pb_msd_reg00);
}

void sdhci_smartchip_reset(struct sdhci_host *host, u8 mask)
{
	struct sdhci_smartchip_dev *smartchip_dev_priv = NULL;
	struct sdhci_pltfm_host *pltfm_host;

	pltfm_host = sdhci_priv(host);
	smartchip_dev_priv = sdhci_pltfm_priv(pltfm_host);

	/* Global reset for the quirks of getting OCR error */
	if (mask == SDHCI_RESET_ALL)
		sdhci_smartchip_sw_rst(host);
	sdhci_reset(host, mask);
}

static const struct sdhci_ops sdhci_smartchip_ops = {
	.set_clock = sdhci_smartx_set_clock,
	//.set_clock = sdhci_set_clock,
	.voltage_switch = sdhci_smartchip_voltage_switch,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_smartchip_reset,
	.set_uhs_signaling = sdhci_set_uhs_signaling,
	.platform_execute_tuning = sdhci_sc_execute_tuning,
};

// static const struct sdhci_pltfm_data sdhci_smartchip_pdata = {
//     .ops    = &sdhci_smartchip_ops,
//     .quirks = SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER |
//           SDHCI_QUIRK_NO_BUSY_IRQ |
//           SDHCI_QUIRK_BROKEN_TIMEOUT_VAL |
//           SDHCI_QUIRK_FORCE_DMA |
//           SDHCI_QUIRK_NO_HISPD_BIT,
// };
static const struct sdhci_pltfm_data sdhci_smartchip_pdata = {
	.ops    = &sdhci_smartchip_ops,
	.quirks = SDHCI_QUIRK_32BIT_ADMA_SIZE | SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC |
	SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12,
	.quirks2 = SDHCI_QUIRK2_BROKEN_DDR50

};

static int sdhci_smartchip_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_smartchip_dev *smartchip_dev_priv = NULL;

	int ret = 0;

	host = sdhci_pltfm_init(pdev, &sdhci_smartchip_pdata, sizeof(struct sdhci_smartchip_dev));
	if (IS_ERR(host))
		return PTR_ERR(host);

	pltfm_host = sdhci_priv(host);
	smartchip_dev_priv = sdhci_pltfm_priv(pltfm_host);

	mutex_init(&smartchip_dev_priv->write_lock);
	smartchip_dev_priv->emmc_pll = devm_ioremap(&pdev->dev, POLESTAR_EMMC_PLL, 0x4);
	if (!smartchip_dev_priv->emmc_pll) {
		dev_err(&pdev->dev, "ioremap failed for resource POLESTAR_EMMC_PLL\n");
		ret = -ENOMEM;
	}
	smartchip_dev_priv->emmc_clk_ctrl = devm_ioremap(&pdev->dev, POLESTAR_EMMC_CLK_CTRL, 0x4);
	if (!smartchip_dev_priv->emmc_clk_ctrl) {
		dev_err(&pdev->dev, "ioremap failed for resource POLESTAR_EMMC_CLK_CTRL\n");
		ret = -ENOMEM;

	}
	smartchip_dev_priv->emmc_pll_phase = devm_ioremap(&pdev->dev, POLESTAR_EMMC_PLL_PHASE, 0x4);
	if (!smartchip_dev_priv->emmc_pll_phase) {
		dev_err(&pdev->dev, "ioremap failed for resource POLESTAR_EMMC_PLL_PHASE\n");
		ret = -ENOMEM;
	}
	if (ret)
		goto err_sdhci_add;

	smartchip_dev_priv->pb_msd_reg00 = devm_ioremap(&pdev->dev, POLESTAR_EMMC_PB_MSG_REG00, 0x4);
	if (!smartchip_dev_priv->pb_msd_reg00) {
		dev_err(&pdev->dev, "ioremap failed for resource POLESTAR_EMMC_PB_MSG_REG00\n");
		ret = -ENOMEM;
	}
	if (ret)
		goto err_sdhci_add;

	smartchip_dev_priv->misc_ctrl = devm_ioremap(&pdev->dev, POLESTAR_MISC_CTRL, 0x4);
	if (!smartchip_dev_priv->pb_msd_reg00) {
		dev_err(&pdev->dev, "ioremap failed for resource POLESTAR_MISC_CTRL\n");
		ret = -ENOMEM;
	}
	if (ret)
		goto err_sdhci_add;

	smartchip_dev_priv->emmc_reset_ctrl = devm_ioremap(&pdev->dev, POLESTAR_EMMC_RESET_CTRL, 0x4);
	if (!smartchip_dev_priv->emmc_reset_ctrl) {
		dev_err(&pdev->dev, "ioremap failed for resource POLESTAR_EMMC_RESET_CTRL\n");
		ret = -ENOMEM;
	}
	if (ret)
		goto err_sdhci_add;

	sdhci_smartx_set_clock(host, 400000);
	sdhci_smartchip_sw_rst(host);

	/*
	 * Config emmc sample clock and drv clock to out-phase
	 * or in-phase below 25MHz
	 */
	if (of_find_property(pdev->dev.of_node, "out-phase", NULL))
		smartchip_dev_priv->out_phase = 1;
	else
		smartchip_dev_priv->out_phase = 0;

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

	return 0;

err_sdhci_add:
	clk_disable_unprepare(pltfm_host->clk);
	sdhci_pltfm_free(pdev);

	return ret;
}

static const struct of_device_id sdhci_smartchip_of_match_table[] = {
	{ .compatible = "smartchip,smartx-sdhc", },
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_smartchip_of_match_table);

static struct platform_driver sdhci_smartchip_driver = {
	.driver     = {
		.name   = "sdhci-smartchip",
		.pm = &sdhci_pltfm_pmops,
		.of_match_table = sdhci_smartchip_of_match_table,
	},
	.probe      = sdhci_smartchip_probe,
	.remove     = sdhci_pltfm_unregister,
};

module_platform_driver(sdhci_smartchip_driver);

MODULE_DESCRIPTION("SDHCI driver for smartchip");
MODULE_AUTHOR("minzhao <min.zhao@smartchip.cn>");
MODULE_LICENSE("GPL v2");
