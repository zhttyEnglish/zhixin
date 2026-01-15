// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 2021 SmartChip, Inc
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <dt-structs.h>
#include <errno.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <regmap.h>
#include <syscon.h>
#include <bitfield.h>
#include <asm/io.h>
#include <dm/lists.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <dt-bindings/clock/sca200v100-cgu.h>

struct sca200v100_abb {
	u32 abb_core_reg0;
	u32 abb_core_reg1;
	u32 abb_core_reg2;
	u32 abb_core_reg3;
	u32 abb_core_reg4;
	u32 abb_core_reg5;
	u32 abb_core_reg6;
	u32 abb_core_reg7;
	u32 abb_core_reg8;
	u32 abb_core_reg9;
	u32 abb_core_reg10;
	u32 abb_core_reg11;
	u32 abb_core_reg12;
	u32 abb_core_reg13;
	u32 abb_core_reg14;
	u32 abb_core_reg15;
	u32 abb_core_reg16;
	u32 abb_core_reg17;
	u32 abb_core_reg18;
	u32 abb_core_reg19;
	u32 abb_core_reg20;
	u32 abb_core_reg21;
	u32 abb_core_reg22;
	u32 abb_core_reg23;
	u32 abb_core_reg24;
	u32 abb_core_reg25;
	u32 abb_core_reg26;
	u32 abb_core_reg27;
	u32 abb_core_reg28;
	u32 abb_core_reg29;
	u32 abb_core_reg30;
	u32 abb_core_reg31;
	u32 abb_core_reg32;
	u32 abb_core_reg33;
	u32 abb_core_reg34;
	u32 abb_core_reg35;
	u32 abb_core_reg36;
	u32 abb_core_reg37;
	u32 abb_core_reg38;
	u32 abb_core_reg39;
	u32 abb_core_reg40;
	u32 abb_core_reg41;
	u32 abb_core_reg42;
	u32 abb_core_reg43;
	u32 abb_core_reg44;
	u32 abb_core_reg45;
	u32 abb_core_reg46;
	u32 abb_core_reg47;
	u32 abb_core_reg48;
	u32 abb_core_reg49;
	u32 abb_core_reg50;
	u32 abb_core_reg51;
	u32 abb_core_reg52;
	u32 abb_core_reg53;
	u32 abb_core_reg54;
	u32 backup_pll_reg1;
	u32 backup_pll_reg2;
	u32 backup_pll_reg3;
	u32 backup_pll_reg4;
	u32 backup_pll_reg5;
	u32 backup_pll_reg6;
	u32 backup_pll_reg7;

	u32 reserved_1[2];

	u32 aupll1_reg_b40;
	u32 aupll1_reg_b41;
	u32 aupll1_reg_b42;
	u32 aupll1_reg_b43;
	u32 aupll1_reg_b44;
	u32 aupll1_reg_b45;

	u32 reserved_2[2];

	u32 aupll2_reg_b40;
	u32 aupll2_reg_b41;
	u32 aupll2_reg_b42;
	u32 aupll2_reg_b43;
	u32 aupll2_reg_b44;
	u32 aupll2_reg_b45;

	u32 reserved_3[50];

	u32 dsppll_0_reg;
	u32 dsppll_1_reg;
	u32 dsppll_2_reg;
	u32 addapll_reg;

	u32 reserved_4;

	u32 top_reg;
	u32 ddrpll_reg;
	u32 aupll_reg;

	u32 reserved_5[376];

	u32 sar10_work_mode;
	u32 sar10_chan_sel;
	u32 sar10_ready_th;
	u32 sar10_interval;
	u32 sar10_data;

	u32 reserved_6[3];

	u32 sar10_int_status;
	u32 sar10_int_clr;

	u32 reserved_7[54];

	u32 ts_work_mode;
	u32 ts_chan_sel;
	u32 ts_ready_th;
	u32 ts_interval;
	u32 ts_data;

	u32 reserved_8[3];

	u32 ts_int_status;
	u32 ts_int_clr;

	u32 reserved_9[54];

	u32 ddrpll_dig_reg0;
	u32 ddrpll_dig_reg1;
	u32 ddrpll_dig_reg2;
	u32 ddrpll_dig_reg3;
	u32 pixpll_dig_reg0;
	u32 pixpll_dig_reg1;
	u32 pixpll_dig_reg2;
	u32 pixpll_dig_reg3;
	u32 sdcpll_dig_reg0;
	u32 sdcpll_dig_reg1;
	u32 sdcpll_dig_reg2;
	u32 sdcpll_dig_reg3;
	u32 aupll_dig_reg0;
	u32 aupll_dig_reg1;
	u32 aupll_dig_reg2;
	u32 aupll_dig_reg3;
	u32 aupll_dig_reg4;
	u32 aupll1_dig_reg0;
	u32 aupll1_dig_reg1;
	u32 aupll1_dig_reg2;
	u32 aupll1_dig_reg3;
	u32 aupll1_dig_reg4;
	u32 aupll2_dig_reg0;
	u32 aupll2_dig_reg1;
	u32 aupll2_dig_reg2;
	u32 aupll2_dig_reg3;
	u32 aupll2_dig_reg4;

	u32 reserved_10[37];

	u32 pvt_ctr_reg;
	u32 pvt_out_reg;
};

struct sca200v100_cgu {
	u32 video_clk_ctrl;
	u32 codec_clk_ctrl;
	u32 bb_clk_ctrl;
	u32 hdmi_clk_ctrl;
	u32 mipi_clk_ctrl;
	u32 typec_clk_ctrl;
	u32 usb20_clk_ctrl;
	u32 pcie_clk_ctrl;
	u32 perip_clk_ctrl;
	u32 sec_clk_ctrl;
	u32 noc_clk_ctrl;
	u32 ddr_clk_ctrl;
	u32 gmac_clk_ctrl;
	u32 emmc_clk_ctrl;
	u32 dmac_clk_ctrl;
	u32 video_if_clk_ctrl;
	u32 sram_clk_ctrl;
	u32 m7_clk_ctrl_thr;
	u32 ceva_clk_ctrl;
	u32 dla_clk_ctrl;
	u32 a53_clk_ctrl;
	u32 rom_clk_ctrl;
	u32 hdecomp_clk_ctrl;
	u32 audio_clk_ctrl;
	u32 sensor_clk_ctrl;
};

struct sca200v100_a53_cgu {
	u32 cgu_pll_config;
	u32 cgu_pll_ctrl_1;
	u32 cgu_pll_ctrl_2;
	u32 cgu_trt_config;
	u32 cgu_trt_ctrl;
	u32 cgu_pvt_data;
	u32 cgu_pvt_ctrl;
	u32 cgu_ls_ctrl;
	u32 standby_status;
	u32 rm_ctrl;
	u32 cgu_reserved1;
	u32 cgu_reserved2;
	u32 global_0;
	u32 global_1;
	u32 global_2;
	u32 global_3;
};

struct sca200v100_ceva_cgu {
	u32 cgu_pll_cfg;
	u32 cgu_pll_ctrl1;
	u32 cgu_pll_ctrl2;
	u32 cgu_trt0_cfg;
	u32 cgu_trt0_ctrl;
	u32 cgu_pvt_data;
	u32 cgu_pvt_ctrl;
	u32 cgu_reserve;
};

struct sca200v100_top_regs {
	u32 sram_top_ctrl;
	u32 boot_rom_ctrl;
	u32 venc0_ctrl;
	u32 venc1_ctrl;
	u32 rsz_ctrl;
	u32 jpeg_ctrl;
	u32 disp_eng_ctrl;
	u32 smmu_ctrl;
	u32 dla_pll_ctrl0;
	u32 dla_pll_ctrl1;
	u32 topdma_ctrl;
	u32 hde_cs_ctrl;
	u32 wdt_rstn_ctrl;
	u32 perip_eh2h_ctrl;
};

struct sca200v100_abb_freqs {
	u32 abb_emmc_freq;
	u32 abb_sdc0_freq;
	u32 abb_sdc1_freq;
	u32 abb_pixel_freq;
	u32 abb_audio_freq;
	u32 abb_audio1_freq;
	u32 abb_audio2_freq;
	u32 abb_backup_freq;
	u32 abb_a53_freq;
	u32 abb_ceva_freq;
	u32 abb_dla_freq;
	u32 abb_noc_freq;
};

struct sca200v100_clock {
	void __iomem *abb;
	void __iomem *cgu;
	void __iomem *top;
	void __iomem *a53_cgu;
	void __iomem *ceva_cgu;
	struct sca200v100_abb_freqs abb_freqs;
};

struct sca200v100_clock_plat {
	struct regmap *map;
};

enum {
	/* CGU VIDEO_CLK_CTRL */
	CLK_ISP_SEL_SHIFT       = 0,
	CLK_ISP_SEL_MASK        = 0xf << CLK_ISP_SEL_SHIFT,
	CLK_ISP_ON_SHIFT        = 4,
	CLK_ISP_ON_MASK         = 1 << CLK_ISP_ON_SHIFT,
	CLK_CFG_ISP_ON_SHIFT        = 5,
	CLK_CFG_ISP_ON_MASK     = 1 << CLK_CFG_ISP_ON_SHIFT,
	CLK_CORE_DISP_SEL_SHIFT     = 8,
	CLK_CORE_DISP_SEL_MASK      = 0xf << CLK_CORE_DISP_SEL_SHIFT,
	CLK_CORE_DISP_ON_SHIFT      = 12,
	CLK_CORE_DISP_ON_MASK       = 1 << CLK_CORE_DISP_ON_SHIFT,
	CLK_CFG_DISP_ON_SHIFT       = 13,
	CLK_CFG_DISP_ON_MASK        = 1 << CLK_CFG_DISP_ON_SHIFT,
	CLK_PIXEL_DISP_ON_SHIFT     = 14,
	CLK_PIXEL_DISP_ON_MASK      = 1 << CLK_PIXEL_DISP_ON_SHIFT,
	CLK_PIXEL_DISP_SEL_SHIFT    = 15,
	CLK_PIXEL_DISP_SEL_MASK     = 1 << CLK_PIXEL_DISP_SEL_SHIFT,
	CLK_DVP_PIX_ON_SHIFT        = 17,
	CLK_DVP_PIX_ON_MASK     = 1 << CLK_DVP_PIX_ON_SHIFT,
	CLK_SUB_1_2X_PIX_ON_SHIFT   = 18,
	CLK_SUB_1_2X_PIX_ON_MASK    = 1 << CLK_SUB_1_2X_PIX_ON_SHIFT,
	CLK_SUB_1_1X_PIX_ON_SHIFT   = 19,
	CLK_SUB_1_1X_PIX_ON_MASK    = 1 << CLK_SUB_1_1X_PIX_ON_SHIFT,
	CLK_DSI_PIX_ON_SHIFT        = 20,
	CLK_DSI_PIX_ON_MASK     = 1 << CLK_DSI_PIX_ON_SHIFT,
	CLK_DSI_PIX_SEL_SHIFT       = 21,
	CLK_DSI_PIX_SEL_MASK        = 1 << CLK_DSI_PIX_SEL_SHIFT,
	CLK_DVP_PIX_HLY_ON_SHIFT    = 22,
	CLK_DVP_PIX_HLY_ON_MASK     = 1 << CLK_DVP_PIX_HLY_ON_SHIFT,
	CLK_PATTERN_GEN_ON_SHIFT    = 23,
	CLK_PATTERN_GEN_ON_MASK     = 1 << CLK_PATTERN_GEN_ON_SHIFT,
	CLK_CFG_DVPCTL_ON_SHIFT     = 24,
	CLK_CFG_DVPCTL_ON_MASK      = 1 << CLK_CFG_DVPCTL_ON_SHIFT,

	/* CGU CODEC_CLK_CTRL */
	CLK_AXI_VENC_SEL_SHIFT      = 0,
	CLK_AXI_VENC_SEL_MASK       = 0xf << CLK_AXI_VENC_SEL_SHIFT,
	CLK_AXI_VENC_ON_SHIFT       = 4,
	CLK_AXI_VENC_ON_MASK        = 0x1 << CLK_AXI_VENC_ON_SHIFT,
	CLK_CFG_VENC_ON_SHIFT       = 5,
	CLK_CFG_VENC_ON_MASK        = 0x1 << CLK_CFG_VENC_ON_SHIFT,
	CLK_BPU_VENC_SEL_SHIFT      = 8,
	CLK_BPU_VENC_SEL_MASK       = 0xF << CLK_BPU_VENC_SEL_SHIFT,
	CLK_BPU_VENC_ON_SHIFT       = 12,
	CLK_BPU_VENC_ON_MASK        = 0x1 << CLK_BPU_VENC_ON_SHIFT,
	CLK_CORE_JPEG_SEL_SHIFT     = 16,
	CLK_CORE_JPEG_SEL_MASK      = 0xf << CLK_CORE_JPEG_SEL_SHIFT,
	CLK_CORE_JPEG_ON_SHIFT      = 20,
	CLK_CORE_JPEG_ON_MASK       = 0x1 << CLK_CORE_JPEG_ON_SHIFT,
	CLK_CFG_JPEG_ON_SHIFT       = 21,
	CLK_CFG_JPEG_ON_MASK        = 0x1 << CLK_CFG_JPEG_ON_SHIFT,

	/* CGU MIPI_CLK_CTRL */
	CLK_VIDEO_MIPI0_SEL_SHIFT   = 0,
	CLK_VIDEO_MIPI0_SEL_MASK    = 0xf << CLK_VIDEO_MIPI0_SEL_SHIFT,
	CLK_VIDEO_MIPI0_ON_SHIFT    = 4,
	CLK_VIDEO_MIPI0_ON_MASK     = 0x1 << CLK_VIDEO_MIPI0_ON_SHIFT,
	CLK_CFG_MIPI_ON_SHIFT       = 5,
	CLK_CFG_MIPI_ON_MASK        = 0x1 << CLK_CFG_MIPI_ON_SHIFT,
	CLK_PCS_MIPI_SEL_SHIFT      = 6,
	CLK_PCS_MIPI_SEL_MASK       = 0x1 << CLK_PCS_MIPI_SEL_SHIFT,
	CLK_PCS_MIPI_ON_SHIFT       = 7,
	CLK_PCS_MIPI_ON_MASK        = 0x1 << CLK_PCS_MIPI_ON_SHIFT,
	CLK_VIDEO_MIPI1_SEL_SHIFT   = 8,
	CLK_VIDEO_MIPI1_SEL_MASK    = 0xf << CLK_VIDEO_MIPI1_SEL_SHIFT,
	CLK_VIDEO_MIPI1_ON_SHIFT    = 12,
	CLK_VIDEO_MIPI1_ON_MASK     = 0x1 << CLK_VIDEO_MIPI1_ON_SHIFT,
	CLK_CFG_DSI_ON_SHIFT        = 7,
	CLK_CFG_DSI_ON_MASK     = 0x1 << CLK_CFG_DSI_ON_SHIFT,
	CLK_VIDEO_MIPI2_SEL_SHIFT   = 16,
	CLK_VIDEO_MIPI2_SEL_MASK    = 0xf << CLK_VIDEO_MIPI2_SEL_SHIFT,
	CLK_VIDEO_MIPI2_ON_SHIFT    = 20,
	CLK_VIDEO_MIPI2_ON_MASK     = 0x1 << CLK_VIDEO_MIPI2_ON_SHIFT,
	CLK_VIDEO_MIPI3_SEL_SHIFT   = 24,
	CLK_VIDEO_MIPI3_SEL_MASK    = 0xf << CLK_VIDEO_MIPI3_SEL_SHIFT,
	CLK_VIDEO_MIPI3_ON_SHIFT    = 28,
	CLK_VIDEO_MIPI3_ON_MASK     = 0x1 << CLK_VIDEO_MIPI3_ON_SHIFT,

	/* CGU USB20_CLK_CTRL */
	CLK_CFG_USB2OTG0_ON_SHIFT   = 0,
	CLK_CFG_USB2OTG0_ON_MASK    = 0x1 << CLK_CFG_USB2OTG0_ON_SHIFT,
	CLK_PHY_USB2PHY0_ON_SHIFT   = 1,
	CLK_PHY_USB2PHY0_ON_MASK    = 0x1 << CLK_PHY_USB2PHY0_ON_SHIFT,
	CLK_CFG_USB2OTG1_ON_SHIFT   = 2,
	CLK_CFG_USB2OTG1_ON_MASK    = 0x1 << CLK_CFG_USB2OTG1_ON_SHIFT,
	CLK_PHY_USB2PHY1_ON_SHIFT   = 3,
	CLK_PHY_USB2PHY1_ON_MASK    = 0x1 << CLK_PHY_USB2PHY1_ON_SHIFT,
	CLK_CFG_USB2OTG2_ON_SHIFT   = 4,
	CLK_CFG_USB2OTG2_ON_MASK    = 0x1 << CLK_CFG_USB2OTG2_ON_SHIFT,
	CLK_PHY_USB2PHY2_ON_SHIFT   = 5,
	CLK_PHY_USB2PHY2_ON_MASK    = 0x1 << CLK_PHY_USB2PHY2_ON_SHIFT,

	/* CGU PERIP_CLK_CTRL */
	CLK_CORE_PERIP_ON_SHIFT     = 0,
	CLK_CORE_PERIP_ON_MASK      = 0x1 << CLK_CORE_PERIP_ON_SHIFT,
	CLK_CORE_QSPI_SEL_SHIFT     = 1,
	CLK_CORE_QSPI_SEL_MASK      = 0x1 << CLK_CORE_QSPI_SEL_SHIFT,
	CLK_CORE_QSPI_ON_SHIFT      = 3,
	CLK_CORE_QSPI_ON_MASK       = 0x1 << CLK_CORE_QSPI_ON_SHIFT,
	CLK_PERIP_I2SM_ON_SHIFT     = 4,
	CLK_PERIP_I2SM_ON_MASK      = 0x1 << CLK_PERIP_I2SM_ON_SHIFT,
	CLK_CFG_CS_ON_SHIFT     = 5,
	CLK_CFG_CS_ON_MASK      = 0x1 << CLK_CFG_CS_ON_SHIFT,
	CLK_CORE_CS_ON_SHIFT        = 6,
	CLK_CORE_CS_ON_MASK     = 0x1 << CLK_CORE_CS_ON_SHIFT,
	CLK_CFG_SPIDBG_ON_SHIFT     = 7,
	CLK_CFG_SPIDBG_ON_MASK      = 0x1 << CLK_CFG_SPIDBG_ON_SHIFT,
	CLK_SDC_FIXED_ON_SHIFT      = 8,
	CLK_SDC_FIXED_ON_MASK       = 0x1 << CLK_SDC_FIXED_ON_SHIFT,
	CLK_SDC_SAMPLE_ON_SHIFT     = 9,
	CLK_SDC_SAMPLE_ON_MASK      = 0x1 << CLK_SDC_SAMPLE_ON_SHIFT,
	CLK_SDC_DRV_SEL_SHIFT       = 10,
	CLK_SDC_DRV_SEL_MASK        = 0x1 << CLK_SDC_DRV_SEL_SHIFT,
	CLK_SDC_DRV_ON_SHIFT        = 11,
	CLK_SDC_DRV_ON_MASK     = 0x1 << CLK_SDC_DRV_ON_SHIFT,
	CLK_AUDIO_SEL_SHIFT     = 12,
	CLK_AUDIO_SEL_MASK      = 0x3 << CLK_AUDIO_SEL_SHIFT,

	/* CGU SEC_CLK_CTRL */
	CLK_CFG_SEC_ON_SHIFT        = 0,
	CLK_CFG_SEC_ON_MASK     = 0x1 << CLK_CFG_SEC_ON_SHIFT,
	CLK_CFG_EFUSE_ON_SHIFT      = 1,
	CLK_CFG_EFUSE_ON_MASK       = 0x1 << CLK_CFG_EFUSE_ON_SHIFT,

	/* CGU NOC_CLK_CTRL */
	CLK_CCI_ON_SHIFT        = 0,
	CLK_CCI_ON_MASK         = 0x1 << CLK_CCI_ON_SHIFT,
	CLK_100_NOC_ON_SHIFT        = 1,
	CLK_100_NOC_ON_MASK     = 0x1 << CLK_100_NOC_ON_SHIFT,
	CLK_150_NOC_G0_ON_SHIFT     = 2,
	CLK_150_NOC_G0_ON_MASK      = 0x1 << CLK_150_NOC_G0_ON_SHIFT,
	CLK_150_NOC_G1_ON_SHIFT     = 3,
	CLK_150_NOC_G1_ON_MASK      = 0x1 << CLK_150_NOC_G1_ON_SHIFT,
	CLK_150_NOC_G2_ON_SHIFT     = 4,
	CLK_150_NOC_G2_ON_MASK      = 0x1 << CLK_150_NOC_G2_ON_SHIFT,
	CLK_150_NOC_G3_ON_SHIFT     = 5,
	CLK_150_NOC_G3_ON_MASK      = 0x1 << CLK_150_NOC_G3_ON_SHIFT,
	CLK_150_NOC_G4_ON_SHIFT     = 6,
	CLK_150_NOC_G4_ON_MASK      = 0x1 << CLK_150_NOC_G4_ON_SHIFT,
	CLK_150_NOC_G5_ON_SHIFT     = 7,
	CLK_150_NOC_G5_ON_MASK      = 0x1 << CLK_150_NOC_G5_ON_SHIFT,
	CLK_600_NOC_ON_SHIFT        = 8,
	CLK_600_NOC_ON_MASK     = 0x1 << CLK_600_NOC_ON_SHIFT,

	/* CGU DDR_CLK_CTRL */
	CLK_CFG_DDR_ON_SHIFT        = 0,
	CLK_CFG_DDR_ON_MASK     = 0x1 << CLK_CFG_DDR_ON_SHIFT,
	CLK_DDR_ON_SHIFT        = 1,
	CLK_DDR_ON_MASK         = 0x1 << CLK_DDR_ON_SHIFT,

	/* CGU GMAC_CLK_CTRL */
	CLK_CFG_GMAC_SEL_SHIFT      = 0,
	CLK_CFG_GMAC_SEL_MASK       = 0x3 << CLK_CFG_GMAC_SEL_SHIFT,
	CLK_CFG_GMAC_ON_SHIFT       = 3,
	CLK_CFG_GMAC_ON_MASK        = 0x1 << CLK_CFG_GMAC_ON_SHIFT,
	CLK_CORE_GMAC_ON_SHIFT      = 4,
	CLK_CORE_GMAC_ON_MASK       = 0x1 << CLK_CORE_GMAC_ON_SHIFT,
	CLK_PHY_GMAC_SEL_SHIFT      = 5,
	CLK_PHY_GMAC_SEL_MASK       = 0x1 << CLK_PHY_GMAC_SEL_SHIFT,
	CLK_PHY_GMAC_ON_SHIFT       = 6,
	CLK_PHY_GMAC_ON_MASK        = 0x1 << CLK_PHY_GMAC_ON_SHIFT,

	/* CGU EMMC_CLK_CTRL */
	CLK_CFG_EMMC_ON_SHIFT       = 0,
	CLK_CFG_EMMC_ON_MASK        = 0x1 << CLK_CFG_EMMC_ON_SHIFT,
	CLK_EMMC_CARDCLK_ON_SHIFT   = 1,
	CLK_EMMC_CARDCLK_ON_MASK    = 0x1 << CLK_EMMC_CARDCLK_ON_SHIFT,
	CLK_EMMC_CLK_SEL_SHIFT      = 3,
	CLK_EMMC_CLK_SEL_MASK       = 0x1 << CLK_EMMC_CLK_SEL_SHIFT,
	CLK_EMMC_FREQ_SEL_SHIFT     = 4,
	CLK_EMMC_FREQ_SEL_MASK      = 0x3ff << CLK_EMMC_FREQ_SEL_SHIFT,
	CLK_EMMC_DRV_SEL_SHIFT      = 14,
	CLK_EMMC_DRV_SEL_MASK       = 0x1 << CLK_EMMC_DRV_SEL_SHIFT,

	/* CGU DMAC_CLK_CTRL */
	CLK_CORE_TOPDMAC_SEL_SHIFT  = 0,
	CLK_CORE_TOPDMAC_SEL_MASK   = 0x7 << CLK_CORE_TOPDMAC_SEL_SHIFT,
	CLK_CORE_TOPDMAC_ON_SHIFT   = 3,
	CLK_CORE_TOPDMAC_ON_MASK    = 0x1 << CLK_CORE_TOPDMAC_ON_SHIFT,
	CLK_CFG_TOPDMAC_ON_SHIFT    = 4,
	CLK_CFG_TOPDMAC_ON_MASK     = 0x1 << CLK_CFG_TOPDMAC_ON_SHIFT,
	CLK_AXI_TOPDMAC_ON_SHIFT    = 5,
	CLK_AXI_TOPDMAC_ON_MASK     = 0x1 << CLK_AXI_TOPDMAC_ON_SHIFT,

	/* CGU VIDEO_IF_CLK_CTRL*/
	CLK_AXI_VIF_SEL_SHIFT       = 0,
	CLK_AXI_VIF_SEL_MASK        = 0xf << CLK_AXI_VIF_SEL_SHIFT,
	CLK_AXI_VIF_ON_SHIFT        = 4,
	CLK_AXI_VIF_ON_MASK     = 0x1 << CLK_AXI_VIF_ON_SHIFT,
	CLK_CFG_VIF_ON_SHIFT        = 5,
	CLK_CFG_VIF_ON_MASK     = 0x1 << CLK_CFG_VIF_ON_SHIFT,
	CLK_HDR_SEL_SHIFT       = 8,
	CLK_HDR_SEL_MASK        = 0xf << CLK_HDR_SEL_SHIFT,
	CLK_HDR_ON_SHIFT        = 12,
	CLK_HDR_ON_MASK         = 0x1 << CLK_HDR_ON_SHIFT,
	CLK_SCALER_SEL_SHIFT        = 16,
	CLK_SCALER_SEL_MASK     = 0xf << CLK_SCALER_SEL_SHIFT,
	CLK_SCALER_ON_SHIFT     = 20,
	CLK_SCALER_ON_MASK      = 0x1 << CLK_SCALER_ON_SHIFT,
	CLK_CFG_SCALER_ON_SHIFT     = 21,
	CLK_CFG_SCALER_ON_MASK      = 0x1 << CLK_CFG_SCALER_ON_SHIFT,

	/* CGU SRAM_CLK_CTRL */
	CLK_SRAM_ON_SHIFT       = 0,
	CLK_SRAM_ON_MASK        = 0x1 << CLK_SRAM_ON_SHIFT,

	/* CGU CEVA_CLK_CTRL */
	CLK_CFG_CEVA_ON_SHIFT       = 0,
	CLK_CFG_CEVA_ON_MASK        = 0x1 << CLK_CFG_CEVA_ON_SHIFT,

	/* CGU DLA_CLK_CTRL */
	CLK_CFG_DLA_ON_SHIFT        = 0,
	CLK_CFG_DLA_ON_MASK     = 0x1 << CLK_CFG_DLA_ON_SHIFT,

	/* CGU ROM_CLK_CTRL */
	CLK_CFG_ROM_ON_SHIFT        = 0,
	CLK_CFG_ROM_ON_MASK     = 0x1 << CLK_CFG_ROM_ON_SHIFT,

	/* CGU HDECOMP_CLK_CTRL */
	CLK_CORE_HDECOMP_SEL_SHIFT  = 0,
	CLK_CORE_HDECOMP_SEL_MASK   = 0x7 << CLK_CORE_HDECOMP_SEL_SHIFT,
	CLK_CORE_HDECOMP_ON_SHIFT   = 3,
	CLK_CORE_HDECOMP_ON_MASK    = 0x1 << CLK_CORE_HDECOMP_ON_SHIFT,
	CLK_CFG_HDECOMP_ON_SHIFT    = 4,
	CLK_CFG_HDECOMP_ON_MASK     = 0x1 << CLK_CFG_HDECOMP_ON_SHIFT,

	/* CGU AUDIO_CLK_CTRL */
	CLK_AUDIO_300M_ON_SHIFT     = 0,
	CLK_AUDIO_300M_ON_MASK      = 0x1 << CLK_AUDIO_300M_ON_SHIFT,
	CLK_AUDIO_ADC_ON_SHIFT      = 1,
	CLK_AUDIO_ADC_ON_MASK       = 0x1 << CLK_AUDIO_ADC_ON_SHIFT,

	/* CGU SENSOR_CLK_CTRL */
	CLK_SENSOR0_SEL_SHIFT       = 0,
	CLK_SENSOR0_SEL_MASK        = 0x3 << CLK_SENSOR0_SEL_SHIFT,
	CLK_SENSOR0_ON_SHIFT        = 2,
	CLK_SENSOR0_ON_MASK     = 0x1 << CLK_SENSOR0_ON_SHIFT,
	CLK_SENSOR1_SEL_SHIFT       = 4,
	CLK_SENSOR1_SEL_MASK        = 0x3 << CLK_SENSOR1_SEL_SHIFT,
	CLK_SENSOR1_ON_SHIFT        = 6,
	CLK_SENSOR1_ON_MASK     = 0x1 << CLK_SENSOR1_ON_SHIFT,
	CLK_SENSOR2_SEL_SHIFT       = 8,
	CLK_SENSOR2_SEL_MASK        = 0x3 << CLK_SENSOR2_SEL_SHIFT,
	CLK_SENSOR2_ON_SHIFT        = 10,
	CLK_SENSOR2_ON_MASK     = 0x1 << CLK_SENSOR2_ON_SHIFT,
	CLK_SENSOR3_SEL_SHIFT       = 12,
	CLK_SENSOR3_SEL_MASK        = 0x3 << CLK_SENSOR3_SEL_SHIFT,
	CLK_SENSOR3_ON_SHIFT        = 14,
	CLK_SENSOR3_ON_MASK     = 0x1 << CLK_SENSOR3_ON_SHIFT,
};

static void sca200v100_abb_set_emmc(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;

	switch (freq) {
	case 50000000:
		abb->abb_core_reg32 &= ~0xc0;
		break;
	case 100000000:
		abb->abb_core_reg32 &= ~0xc0;
		abb->abb_core_reg32 |= 0x40;
		break;
	case 200000000:
		abb->abb_core_reg32 &= ~0xc0;
		abb->abb_core_reg32 |= 0xc0;
		break;
	default:
		log_err("Unsupport abb emmc freq %d\n", freq);
		return;
	}
	abb_freqs->abb_emmc_freq = freq;
}

static void sca200v100_abb_set_sdc0(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;

	switch (freq) {
	case 50000000:
		abb->abb_core_reg34 &= ~0xc0;
		break;
	case 100000000:
		abb->abb_core_reg34 &= ~0xc0;
		abb->abb_core_reg34 |= 0x40;
		break;
	case 200000000:
		abb->abb_core_reg34 &= ~0xc0;
		abb->abb_core_reg34 |= 0xc0;
		break;
	default:
		log_err("Unsupport abb sdc0 freq %d\n", freq);
		return;
	}
	abb_freqs->abb_sdc0_freq = freq;
}

static void sca200v100_abb_set_sdc1(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;

	switch (freq) {
	case 50000000:
		abb->abb_core_reg48 &= ~0xc0;
		break;
	case 100000000:
		abb->abb_core_reg48 &= ~0xc0;
		abb->abb_core_reg48 |= 0x40;
		break;
	case 200000000:
		abb->abb_core_reg48 &= ~0xc0;
		abb->abb_core_reg48 |= 0xc0;
		break;
	default:
		log_err("Unsupport abb sdc1 freq %d\n", freq);
		return;
	}
	abb_freqs->abb_sdc1_freq = freq;
}

static void sca200v100_abb_set_pixel(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 loop_div, post_div;

	if (freq > 600000000 || freq < 6250000) {
		log_err("Unsupport abb pix freq %d\n", freq);
		return;
	}

	abb->abb_core_reg38 &= ~0x3f;

	if (freq > 400000000) {
		loop_div = 5;
		post_div = 1;
	} else if (freq > 200000000) {
		loop_div = 5;
		post_div = 2;
	} else if (freq > 100000000) {
		loop_div = 5;
		post_div = 3;
	} else if (freq > 50000000) {
		loop_div = 5;
		post_div = 4;
	} else if (freq > 25000000) {
		loop_div = 5;
		post_div = 5;
	} else if (freq > 12500000) {
		loop_div = 5;
		post_div = 6;
	} else if (freq >= 6250000) {
		loop_div = 4;
		post_div = 7;
	}
	abb->abb_core_reg38 |= (loop_div | (post_div << 3));
	abb->pixpll_dig_reg0 = (u32)(((u64)400000000) *
	        ((u64)(1 << (24 + loop_div - post_div))) /
	        (u64)freq);

	abb_freqs->abb_pixel_freq = freq;
}

static void sca200v100_abb_set_audio(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 intg;
	float dcm;

	if (freq < 500000 || freq > 2000000000) {
		log_err("Unsupport abb audio freq %d\n", freq);
	}

	intg = freq / 40000000;
	dcm = ((float)(freq % 40000000)) * 0.4194304;

	abb->aupll_dig_reg0 = intg << 24 | (u32)dcm;

	abb_freqs->abb_audio_freq = freq;
}

static void sca200v100_abb_set_audio1(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 intg;
	float dcm;

	if (freq < 500000 || freq > 2000000000) {
		log_err("Unsupport abb audio freq %d\n", freq);
	}

	intg = freq / 40000000;
	dcm = ((float)(freq % 40000000)) * 0.4194304;

	abb->aupll1_dig_reg0 = intg << 24 | (u32)dcm;

	abb->aupll2_reg_b40 = 1;
	udelay(5);
	abb->aupll2_reg_b40 = 0;

	abb_freqs->abb_audio1_freq = freq;
}

static void sca200v100_abb_set_audio2(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 intg;
	float dcm;

	if (freq < 500000 || freq > 2000000000) {
		log_err("Unsupport abb audio freq %d\n", freq);
	}

	intg = freq / 40000000;
	dcm = ((float)(freq % 40000000)) * 0.4194304;

	abb->aupll2_dig_reg0 = intg << 24 | (u32)dcm;

	abb_freqs->abb_audio2_freq = freq;

	abb->aupll2_reg_b45 = 0x20;
}

static void sca200v100_abb_set_backup(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;

	if (freq < 40000000 || freq > 2550000000) {
		log_err("Unsupport abb backup freq %d\n", freq);
	}
	abb->backup_pll_reg2 = freq / 10000000;

	abb_freqs->abb_backup_freq = freq;
}

static void sca200v100_abb_set_a53(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_a53_cgu *a53_cgu = priv->a53_cgu;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;
	//unsigned int val;
	//  volatile unsigned int i, j;

	if (freq < 40000000 || freq > 2550000000) {
		log_err("Unsupport abb a53 freq %d\n", freq);
	}
#if 0
	val = a53_cgu->cgu_pll_config;
	val = (val >> 8) & 0xff;

	for (i = 7; i >= 2; i--) {
		if (val & (1 << i))
			break;
	}
	i = i - 2;
	i = 5 - i;
	val = 0;
	if (i == 0) {
		val = 0;
	} else {
		for (j = 0; j < i; j++)
			val |= 1 << j;
	}

	a53_cgu->cgu_reserved1 &= ~0x1f;
	a53_cgu->cgu_reserved1 |= val;

	a53_cgu->cgu_pll_config |= 0x3 << 26;
	a53_cgu->cgu_pll_config |= 0x1 << 24;

	a53_cgu->cgu_pll_config &= 0xffff00ff;
	a53_cgu->cgu_pll_config |= (freq / 10000000) << 8;

	for (i = 0; i < 1000; i++);

	a53_cgu->cgu_pll_config &= ~(0x1 << 24);
	a53_cgu->cgu_pll_config &= ~(0x1 << 26);
#elif 1
	a53_cgu->cgu_pll_config &= ~0x1;
	a53_cgu->cgu_pll_config |= 0x1;
	udelay(5);
	a53_cgu->cgu_pll_config &= ~(0x1 << 26);
	a53_cgu->cgu_pll_config |= 0x1 << 26;
	a53_cgu->cgu_pll_config &= ~(0x1 << 24);
	a53_cgu->cgu_pll_config |= 0x1 << 24;
	udelay(5);

	a53_cgu->cgu_pll_config &= 0xffff00ff;
	a53_cgu->cgu_pll_config |= (freq / 10000000) << 8;

	a53_cgu->cgu_pll_config &= ~0x1;
	udelay(5);
	a53_cgu->cgu_pll_config &= ~(0x1 << 24);
	a53_cgu->cgu_pll_config &= ~(0x1 << 26);
	udelay(5);
#else
	a53_cgu->cgu_pll_config |= 0x2000000 ;
	val = a53_cgu->cgu_pll_config;
	val &= 0xffff00ff;
	val |= (freq / 10000000) << 8;
	a53_cgu->cgu_pll_config = val;
#endif
	abb_freqs->abb_a53_freq = freq;
}

static void sca200v100_abb_set_ceva(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_ceva_cgu *ceva_cgu = priv->ceva_cgu;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;
	struct sca200v100_cgu *cgu = priv->cgu;
	unsigned int val;
	//  volatile unsigned int i, j;

	if( !(cgu->ceva_clk_ctrl & CLK_CFG_CEVA_ON_MASK) ) {
		log_debug("cgu ceva clk off\n");
		return;
	}
	if (freq < 40000000 || freq > 2550000000) {
		log_err("Unsupport abb ceva freq %d\n", freq);
		return;
	}
#if 0
	val = ceva_cgu->cgu_pll_cfg;
	val = (val >> 8) & 0xff;

	for (i = 7; i >= 2; i--) {
		if (val & (1 << i))
			break;
	}
	i = i - 2;
	i = 5 - i;
	val = 0;
	if (i == 0) {
		val = 0;
	} else {
		for (j = 0; j < i; j++)
			val |= 1 << j;
	}

	ceva_cgu->cgu_pll_ctrl2 &= ~0x1f;
	ceva_cgu->cgu_pll_ctrl2 |= val;

	ceva_cgu->cgu_pll_cfg |= 0x3 << 26;
	ceva_cgu->cgu_pll_cfg |= 0x1 << 24;

	ceva_cgu->cgu_pll_cfg &= 0xffff00ff;
	ceva_cgu->cgu_pll_cfg |= (freq / 10000000) << 8;

	for (i = 0; i < 1000; i++);

	ceva_cgu->cgu_pll_cfg &= ~(0x1 << 24);
	ceva_cgu->cgu_pll_cfg &= ~(0x1 << 26);
#elif 0
	ceva_cgu->cgu_pll_cfg &= ~0x1;
	ceva_cgu->cgu_pll_cfg |= 0x1;
	udelay(5);
	ceva_cgu->cgu_pll_cfg &= ~(0x1 << 26);
	ceva_cgu->cgu_pll_cfg |= 0x1 << 26;
	ceva_cgu->cgu_pll_cfg &= ~(0x1 << 24);
	ceva_cgu->cgu_pll_cfg |= 0x1 << 24;
	udelay(5);

	ceva_cgu->cgu_pll_cfg &= 0xffff00ff;
	ceva_cgu->cgu_pll_cfg |= (freq / 10000000) << 8;

	ceva_cgu->cgu_pll_cfg &= ~0x1;
	udelay(5);
	ceva_cgu->cgu_pll_cfg &= ~(0x1 << 24);
	ceva_cgu->cgu_pll_cfg &= ~(0x1 << 26);
	udelay(5);
#else
	ceva_cgu->cgu_pll_cfg |= 0x2000000;
	val = ceva_cgu->cgu_pll_cfg;
	val &= 0xffff00ff;
	val |= (freq / 10000000) << 8;
	ceva_cgu->cgu_pll_cfg = val;
#endif
	abb_freqs->abb_ceva_freq = freq;
}

static void sca200v100_abb_set_dla(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_top_regs *top = priv->top;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;

	if (freq < 40000000 || freq > 2550000000) {
		log_err("Unsupport abb dla freq %d\n", freq);
	}
	top->dla_pll_ctrl0 &= 0xffff00ff;
	top->dla_pll_ctrl0 |= (freq / 10000000) << 8;

	abb_freqs->abb_dla_freq = freq;
}

static void sca200v100_abb_set_noc(struct sca200v100_clock *priv, u32 freq)
{
	struct sca200v100_abb *abb = priv->abb;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;

	if (freq < 3333333 || freq > 850000000) {
		log_err("Unsupport abb noc freq %d\n", freq);
	}

	abb->abb_core_reg24 &= 0xffffff00;
	abb->abb_core_reg24 |= (freq / 10000000 * 3);

	abb_freqs->abb_noc_freq = freq;
}

static void sca200v100_abb_set_quirks(struct sca200v100_clock *priv)
{
	struct sca200v100_abb *abb = priv->abb;

	abb->abb_core_reg0 |= 0x80;
}

static ulong sca200v100_clk_get_rate(struct clk *clk)
{
#if 0
	struct sca200v100_clock *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	switch (clk->id) {
	// hdecomp
	case CLK_CORE_HDECOMP:
	// topdmac
	case CLK_CORE_TOPDMAC:
	// emmc
	case CLK_EMMC_FREQ:
	case CLK_EMMC_CLK:
	case CLK_EMMC_DRV:
	// isp
	case CLK_ISP:
	case CLK_AXI_VIF:
	case CLK_HDR:
	// rsz ifc eis
	case CLK_SCALER:
	// jpeg
	case CLK_CORE_JPEG:
	// disp
	case CLK_CORE_DISP:
	// venc
	case CLK_AXI_VENC:
	case CLK_BPU_VENC:
	// pixel disp
	case CLK_PIXEL_DISP:
	// dsi pix
	case CLK_DSI_PIX:
	// mipi_csi
	case CLK_VIDEO_MIPI0:
	case CLK_VIDEO_MIPI1:
	case CLK_VIDEO_MIPI2:
	case CLK_VIDEO_MIPI3:
	case CLK_PCS_MIPI:
	// gmac
	case CLK_CFG_GMAC:
	case CLK_PHY_GMAC:
	// sensor
	case CLK_SENSOR0:
	case CLK_SENSOR1:
	case CLK_SENSOR2:
	case CLK_SENSOR3:
	// sdcard
	case CLK_SDC_DRV:
	// qspi
	case CLK_CORE_QSPI:
		// i2sm
#if 0
	case CLK_AUDIO:
	case I2S_MST0_DIV:
	case I2S_MST1_DIV:
	case I2S_MST2_DIV:
#endif
	case CLK_PERIP_I2SM:
	default:
		log_debug("Unknown clock %lu\n", clk->id);
		return -ENOENT;
	}

	return rate;
#endif
	return 0;
}

static ulong sca200v100_clk_set_sel(struct sca200v100_cgu *cgu, u32 clk_id, u32 sel)
{
	switch (clk_id) {
	// hdecomp
	case CLK_CORE_HDECOMP:
		cgu->hdecomp_clk_ctrl &= ~CLK_CORE_HDECOMP_SEL_MASK;
		cgu->hdecomp_clk_ctrl |= sel << CLK_CORE_HDECOMP_SEL_SHIFT;
		break;
	// topdmac
	case CLK_CORE_TOPDMAC:
		cgu->dmac_clk_ctrl &= ~CLK_CORE_TOPDMAC_SEL_MASK;
		cgu->dmac_clk_ctrl |= sel << CLK_CORE_TOPDMAC_SEL_SHIFT;
		break;
	// emmc
	case CLK_EMMC_CARDCLK:
		//case CLK_EMMC_FREQ:
		//case CLK_EMMC_CLK:
		break;
	//case CLK_EMMC_DRV:
	//  if ((rate != 0) && (rate != 1)) {
	//      log_err("CLK_EMMC_DRV not support rate %ld\n", rate);
	//      return -EINVAL;
	//  }
	//  cgu->emmc_clk_ctrl &= ~CLK_EMMC_DRV_SEL_MASK;
	//  cgu->emmc_clk_ctrl |= rate << CLK_EMMC_DRV_SEL_SHIFT;
	//  break;
	// isp vif
	case CLK_ISP:
		cgu->video_clk_ctrl &= ~CLK_ISP_SEL_MASK;
		cgu->video_clk_ctrl |= sel << CLK_ISP_SEL_SHIFT;
		break;
	case CLK_AXI_VIF:
		cgu->video_if_clk_ctrl &= ~CLK_AXI_VIF_SEL_MASK;
		cgu->video_if_clk_ctrl |= sel << CLK_AXI_VIF_SEL_SHIFT;
		break;
	case CLK_HDR:
		cgu->video_if_clk_ctrl &= ~CLK_HDR_SEL_MASK;
		cgu->video_if_clk_ctrl |= sel << CLK_HDR_SEL_SHIFT;
		break;
	// rsz ifc eis
	case CLK_SCALER:
		cgu->video_if_clk_ctrl &= ~CLK_SCALER_SEL_MASK;
		cgu->video_if_clk_ctrl |= sel << CLK_SCALER_SEL_SHIFT;
		break;
	// jpeg
	case CLK_CORE_JPEG:
		cgu->codec_clk_ctrl &= ~CLK_CORE_JPEG_SEL_MASK;
		cgu->codec_clk_ctrl |= sel << CLK_CORE_JPEG_SEL_SHIFT;
		break;
	// disp
	case CLK_CORE_DISP:
		cgu->video_clk_ctrl &= ~CLK_CORE_DISP_SEL_MASK;
		cgu->video_clk_ctrl |= sel << CLK_CORE_DISP_SEL_SHIFT;
		break;
	// venc
	case CLK_AXI_VENC:
		cgu->codec_clk_ctrl &= ~CLK_AXI_VENC_SEL_MASK;
		cgu->codec_clk_ctrl |= sel << CLK_AXI_VENC_SEL_SHIFT;
		break;
	case CLK_BPU_VENC:
		cgu->codec_clk_ctrl &= ~CLK_BPU_VENC_SEL_MASK;
		cgu->codec_clk_ctrl |= sel << CLK_BPU_VENC_SEL_SHIFT;
		break;
	// pixel disp
	case CLK_PIXEL_DISP:
		cgu->video_clk_ctrl &= ~CLK_PIXEL_DISP_SEL_MASK;
		cgu->video_clk_ctrl |= sel << CLK_PIXEL_DISP_SEL_SHIFT;
		break;
	// dsi pix
	case CLK_DSI_PIX:
		cgu->video_clk_ctrl &= ~CLK_DSI_PIX_SEL_MASK;
		cgu->video_clk_ctrl |= sel << CLK_DSI_PIX_SEL_SHIFT;
		break;
	// mipi_csi
	case CLK_VIDEO_MIPI0:
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI0_SEL_MASK;
		cgu->mipi_clk_ctrl |= sel << CLK_VIDEO_MIPI0_SEL_SHIFT;
		break;
	case CLK_VIDEO_MIPI1:
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI1_SEL_MASK;
		cgu->mipi_clk_ctrl |= sel << CLK_VIDEO_MIPI1_SEL_SHIFT;
		break;
	case CLK_VIDEO_MIPI2:
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI2_SEL_MASK;
		cgu->mipi_clk_ctrl |= sel << CLK_VIDEO_MIPI2_SEL_SHIFT;
		break;
	case CLK_VIDEO_MIPI3:
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI3_SEL_MASK;
		cgu->mipi_clk_ctrl |= sel << CLK_VIDEO_MIPI3_SEL_SHIFT;
		break;
	case CLK_PCS_MIPI:
		cgu->mipi_clk_ctrl &= ~CLK_PCS_MIPI_SEL_MASK;
		cgu->mipi_clk_ctrl |= sel << CLK_PCS_MIPI_SEL_SHIFT;
		break;
	// gmac
	case CLK_CFG_GMAC:
		cgu->gmac_clk_ctrl &= ~CLK_CFG_GMAC_SEL_MASK;
		cgu->gmac_clk_ctrl |= sel << CLK_CFG_GMAC_SEL_SHIFT;
		break;
	case CLK_PHY_GMAC:
		cgu->gmac_clk_ctrl &= ~CLK_PHY_GMAC_SEL_MASK;
		cgu->gmac_clk_ctrl |= sel << CLK_PHY_GMAC_SEL_SHIFT;
		break;
	// sensor
	case CLK_SENSOR0:
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR0_SEL_MASK;
		cgu->sensor_clk_ctrl |= sel << CLK_SENSOR0_SEL_SHIFT;
		break;
	case CLK_SENSOR1:
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR1_SEL_MASK;
		cgu->sensor_clk_ctrl |= sel << CLK_SENSOR1_SEL_SHIFT;
		break;
	case CLK_SENSOR2:
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR2_SEL_MASK;
		cgu->sensor_clk_ctrl |= sel << CLK_SENSOR2_SEL_SHIFT;
		break;
	case CLK_SENSOR3:
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR3_SEL_MASK;
		cgu->sensor_clk_ctrl |= sel << CLK_SENSOR3_SEL_SHIFT;
		break;
	// sdcard
	case CLK_SDC_DRV:
		cgu->perip_clk_ctrl &= ~CLK_SDC_DRV_SEL_MASK;
		cgu->perip_clk_ctrl |= sel << CLK_SDC_DRV_SEL_SHIFT;
		break;
	// qspi
	case CLK_CORE_QSPI:
		cgu->perip_clk_ctrl &= ~CLK_CORE_QSPI_SEL_MASK;
		cgu->perip_clk_ctrl |= sel << CLK_CORE_QSPI_SEL_SHIFT;
		break;
	// i2sm
	case CLK_AUDIO:
		cgu->perip_clk_ctrl &= ~CLK_AUDIO_SEL_MASK;
		cgu->perip_clk_ctrl |= sel << CLK_AUDIO_SEL_SHIFT;
		break;
#if 0
	case I2S_MST0_DIV:
	case I2S_MST1_DIV:
	case I2S_MST2_DIV:
#endif
	//case CLK_PERIP_I2SM:
	//  cgu->perip_clk_ctrl &= ~CLK_AUDIO_SEL_MASK;
	//  sca200v100_abb_set_audio(priv, rate);
	//  break;
	default:
		log_debug("Unknown clock %d\n", clk_id);
		return -ENOENT;
	}

	return 0;
}

static ulong sca200v100_clk_set_rate(struct clk *clk, ulong rate)
{
#ifndef CONFIG_SPL_BUILD
	struct sca200v100_clock *priv = dev_get_priv(clk->dev);
	struct sca200v100_cgu *cgu = priv->cgu;
	struct sca200v100_abb_freqs *abb_freqs = &priv->abb_freqs;
	uint32_t raw;
	ulong div;
#endif
	ulong ret = 0;

#ifndef CONFIG_SPL_BUILD
	switch (clk->id) {
	// hdecomp
	case CLK_CORE_HDECOMP:
		raw = cgu->hdecomp_clk_ctrl;
		cgu->hdecomp_clk_ctrl &= ~CLK_CORE_HDECOMP_SEL_MASK;
		if (rate == 666000000) {
			break;
		} else if (rate == 600000000) {
			cgu->hdecomp_clk_ctrl |= 1 << CLK_CORE_HDECOMP_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->hdecomp_clk_ctrl |= 2 << CLK_CORE_HDECOMP_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->hdecomp_clk_ctrl |= 3 << CLK_CORE_HDECOMP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->hdecomp_clk_ctrl |= 4 << CLK_CORE_HDECOMP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->hdecomp_clk_ctrl |= 5 << CLK_CORE_HDECOMP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->hdecomp_clk_ctrl |= 6 << CLK_CORE_HDECOMP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->hdecomp_clk_ctrl |= 7 << CLK_CORE_HDECOMP_SEL_SHIFT;
		} else {
			cgu->hdecomp_clk_ctrl = raw;
			log_debug("CLK_CORE_HDECOMP not support rate %ld\n",
			    rate);
			return -EINVAL;
		}
		break;
	// topdmac
	case CLK_CORE_TOPDMAC:
		raw = cgu->dmac_clk_ctrl;
		cgu->dmac_clk_ctrl &= ~CLK_CORE_TOPDMAC_SEL_MASK;
		if (rate == 600000000) {
			break;
		} else if (rate == 500000000) {
			cgu->dmac_clk_ctrl |= 1 << CLK_CORE_TOPDMAC_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->dmac_clk_ctrl |= 2 << CLK_CORE_TOPDMAC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->dmac_clk_ctrl |= 4 << CLK_CORE_TOPDMAC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->dmac_clk_ctrl |= 5 << CLK_CORE_TOPDMAC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->dmac_clk_ctrl |= 6 << CLK_CORE_TOPDMAC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->dmac_clk_ctrl |= 7 << CLK_CORE_TOPDMAC_SEL_SHIFT;
		} else {
			cgu->dmac_clk_ctrl = raw;
			log_debug("CLK_CORE_TOPDMAC not support rate %ld\n",
			    rate);
			return -EINVAL;
		}
		break;
	// emmc
	case CLK_EMMC_CARDCLK:
		//case CLK_EMMC_FREQ:
		//case CLK_EMMC_CLK:
		if (rate < 20000 || rate > 200000000) {
			log_debug("CLK_EMMC not support rate %ld\n", rate);
			return -EINVAL;
		}
		cgu->emmc_clk_ctrl &= ~CLK_EMMC_FREQ_SEL_MASK;
		cgu->emmc_clk_ctrl &= ~CLK_EMMC_CLK_SEL_MASK;
		if (rate <= 50000000) {
			sca200v100_abb_set_emmc(priv, 50000000);
			div = 50000000 / rate / 2;
			cgu->emmc_clk_ctrl |= div << CLK_EMMC_FREQ_SEL_SHIFT;
			cgu->emmc_clk_ctrl |= 1 << CLK_EMMC_CLK_SEL_SHIFT;
		} else if (rate <= 100000000) {
			sca200v100_abb_set_emmc(priv, 100000000);
		} else {
			sca200v100_abb_set_emmc(priv, 200000000);
		}
		break;
	//case CLK_EMMC_DRV:
	//  if ((rate != 0) && (rate != 1)) {
	//      log_err("CLK_EMMC_DRV not support rate %ld\n", rate);
	//      return -EINVAL;
	//  }
	//  cgu->emmc_clk_ctrl &= ~CLK_EMMC_DRV_SEL_MASK;
	//  cgu->emmc_clk_ctrl |= rate << CLK_EMMC_DRV_SEL_SHIFT;
	//  break;
	// isp vif
	case CLK_ISP:
		raw = cgu->video_clk_ctrl;
		cgu->video_clk_ctrl &= ~CLK_ISP_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 666000000) {
			cgu->video_clk_ctrl |= 1 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 600000000) {
			cgu->video_clk_ctrl |= 2 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->video_clk_ctrl |= 3 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 450000000) {
			cgu->video_clk_ctrl |= 4 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->video_clk_ctrl |= 5 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 360000000) {
			cgu->video_clk_ctrl |= 6 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->video_clk_ctrl |= 7 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 250000000) {
			cgu->video_clk_ctrl |= 8 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->video_clk_ctrl |= 9 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->video_clk_ctrl |= 10 << CLK_ISP_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->video_clk_ctrl |= 11 << CLK_ISP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->video_clk_ctrl |= 12 << CLK_ISP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->video_clk_ctrl |= 13 << CLK_ISP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->video_clk_ctrl |= 14 << CLK_ISP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->video_clk_ctrl |= 15 << CLK_ISP_SEL_SHIFT;
		} else {
			cgu->video_clk_ctrl = raw;
			log_debug("CLK_ISP not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	case CLK_AXI_VIF:
		raw = cgu->video_if_clk_ctrl;
		cgu->video_if_clk_ctrl &= ~CLK_AXI_VIF_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 666000000) {
			cgu->video_if_clk_ctrl |= 1 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 600000000) {
			cgu->video_if_clk_ctrl |= 2 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->video_if_clk_ctrl |= 3 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 450000000) {
			cgu->video_if_clk_ctrl |= 4 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->video_if_clk_ctrl |= 5 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 360000000) {
			cgu->video_if_clk_ctrl |= 6 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->video_if_clk_ctrl |= 7 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 250000000) {
			cgu->video_if_clk_ctrl |= 8 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->video_if_clk_ctrl |= 9 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->video_if_clk_ctrl |= 10 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->video_if_clk_ctrl |= 11 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->video_if_clk_ctrl |= 12 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->video_if_clk_ctrl |= 13 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->video_if_clk_ctrl |= 14 << CLK_AXI_VIF_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->video_if_clk_ctrl |= 15 << CLK_AXI_VIF_SEL_SHIFT;
		} else {
			cgu->video_if_clk_ctrl = raw;
			log_debug("CLK_AXI_VIF not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	case CLK_HDR:
		raw = cgu->video_if_clk_ctrl;
		cgu->video_if_clk_ctrl &= ~CLK_HDR_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 666000000) {
			cgu->video_if_clk_ctrl |= 1 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 600000000) {
			cgu->video_if_clk_ctrl |= 2 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->video_if_clk_ctrl |= 3 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 450000000) {
			cgu->video_if_clk_ctrl |= 4 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->video_if_clk_ctrl |= 5 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 360000000) {
			cgu->video_if_clk_ctrl |= 6 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->video_if_clk_ctrl |= 7 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 250000000) {
			cgu->video_if_clk_ctrl |= 8 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->video_if_clk_ctrl |= 9 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->video_if_clk_ctrl |= 10 << CLK_HDR_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->video_if_clk_ctrl |= 11 << CLK_HDR_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->video_if_clk_ctrl |= 12 << CLK_HDR_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->video_if_clk_ctrl |= 13 << CLK_HDR_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->video_if_clk_ctrl |= 14 << CLK_HDR_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->video_if_clk_ctrl |= 15 << CLK_HDR_SEL_SHIFT;
		} else {
			cgu->video_if_clk_ctrl = raw;
			log_debug("CLK_HDR not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	// rsz ifc eis
	case CLK_SCALER:
		raw = cgu->video_if_clk_ctrl;
		cgu->video_if_clk_ctrl &= ~CLK_SCALER_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 666000000) {
			cgu->video_if_clk_ctrl |= 1 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 600000000) {
			cgu->video_if_clk_ctrl |= 2 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->video_if_clk_ctrl |= 3 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 450000000) {
			cgu->video_if_clk_ctrl |= 4 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->video_if_clk_ctrl |= 5 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 360000000) {
			cgu->video_if_clk_ctrl |= 6 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->video_if_clk_ctrl |= 7 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 250000000) {
			cgu->video_if_clk_ctrl |= 8 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->video_if_clk_ctrl |= 9 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->video_if_clk_ctrl |= 10 << CLK_SCALER_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->video_if_clk_ctrl |= 11 << CLK_SCALER_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->video_if_clk_ctrl |= 12 << CLK_SCALER_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->video_if_clk_ctrl |= 13 << CLK_SCALER_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->video_if_clk_ctrl |= 14 << CLK_SCALER_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->video_if_clk_ctrl |= 15 << CLK_SCALER_SEL_SHIFT;
		} else {
			cgu->video_if_clk_ctrl = raw;
			log_debug("CLK_SCALER not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	// jpeg
	case CLK_CORE_JPEG:
		raw = cgu->codec_clk_ctrl;
		cgu->codec_clk_ctrl &= ~CLK_CORE_JPEG_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 666000000) {
			cgu->codec_clk_ctrl |= 1 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 600000000) {
			cgu->codec_clk_ctrl |= 2 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->codec_clk_ctrl |= 3 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 450000000) {
			cgu->codec_clk_ctrl |= 4 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->codec_clk_ctrl |= 5 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 360000000) {
			cgu->codec_clk_ctrl |= 6 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->codec_clk_ctrl |= 7 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 250000000) {
			cgu->codec_clk_ctrl |= 8 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->codec_clk_ctrl |= 9 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->codec_clk_ctrl |= 10 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->codec_clk_ctrl |= 11 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->codec_clk_ctrl |= 12 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->codec_clk_ctrl |= 13 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->codec_clk_ctrl |= 14 << CLK_CORE_JPEG_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->codec_clk_ctrl |= 15 << CLK_CORE_JPEG_SEL_SHIFT;
		} else {
			cgu->codec_clk_ctrl = raw;
			log_debug("CLK_CORE_JPEG not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	// disp
	case CLK_CORE_DISP:
		raw = cgu->video_clk_ctrl;
		cgu->video_clk_ctrl &= ~CLK_CORE_DISP_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 666000000) {
			cgu->video_clk_ctrl |= 1 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 600000000) {
			cgu->video_clk_ctrl |= 2 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->video_clk_ctrl |= 3 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 450000000) {
			cgu->video_clk_ctrl |= 4 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->video_clk_ctrl |= 5 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 360000000) {
			cgu->video_clk_ctrl |= 6 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->video_clk_ctrl |= 7 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 250000000) {
			cgu->video_clk_ctrl |= 8 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->video_clk_ctrl |= 9 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->video_clk_ctrl |= 10 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->video_clk_ctrl |= 11 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->video_clk_ctrl |= 12 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->video_clk_ctrl |= 13 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->video_clk_ctrl |= 14 << CLK_CORE_DISP_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->video_clk_ctrl |= 15 << CLK_CORE_DISP_SEL_SHIFT;
		} else {
			cgu->video_clk_ctrl = raw;
			log_debug("CLK_CORE_DISP not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	// venc
	case CLK_AXI_VENC:
		raw = cgu->codec_clk_ctrl;
		cgu->codec_clk_ctrl &= ~CLK_AXI_VENC_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 666000000) {
			cgu->codec_clk_ctrl |= 1 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 600000000) {
			cgu->codec_clk_ctrl |= 2 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->codec_clk_ctrl |= 3 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 450000000) {
			cgu->codec_clk_ctrl |= 4 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->codec_clk_ctrl |= 5 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 360000000) {
			cgu->codec_clk_ctrl |= 6 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->codec_clk_ctrl |= 7 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 250000000) {
			cgu->codec_clk_ctrl |= 8 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->codec_clk_ctrl |= 9 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->codec_clk_ctrl |= 10 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->codec_clk_ctrl |= 11 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->codec_clk_ctrl |= 12 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->codec_clk_ctrl |= 13 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->codec_clk_ctrl |= 14 << CLK_AXI_VENC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->codec_clk_ctrl |= 15 << CLK_AXI_VENC_SEL_SHIFT;
		} else {
			cgu->codec_clk_ctrl = raw;
			log_debug("CLK_AXI_VENC not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	case CLK_BPU_VENC:
		raw = cgu->codec_clk_ctrl;
		cgu->codec_clk_ctrl &= ~CLK_BPU_VENC_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 666000000) {
			cgu->codec_clk_ctrl |= 1 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 600000000) {
			cgu->codec_clk_ctrl |= 2 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->codec_clk_ctrl |= 3 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 450000000) {
			cgu->codec_clk_ctrl |= 4 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->codec_clk_ctrl |= 5 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 360000000) {
			cgu->codec_clk_ctrl |= 6 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->codec_clk_ctrl |= 7 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 250000000) {
			cgu->codec_clk_ctrl |= 8 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->codec_clk_ctrl |= 9 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->codec_clk_ctrl |= 10 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->codec_clk_ctrl |= 11 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->codec_clk_ctrl |= 12 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->codec_clk_ctrl |= 13 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->codec_clk_ctrl |= 14 << CLK_BPU_VENC_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->codec_clk_ctrl |= 15 << CLK_BPU_VENC_SEL_SHIFT;
		} else {
			cgu->codec_clk_ctrl = raw;
			log_debug("CLK_BPU_VENC not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	// pixel disp
	case CLK_PIXEL_DISP:
		raw = cgu->video_clk_ctrl;
		cgu->video_clk_ctrl &= ~CLK_PIXEL_DISP_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (abb_freqs->abb_pixel_freq / 2 == rate) {
			cgu->video_clk_ctrl |= CLK_PIXEL_DISP_SEL_MASK;
		} else {
			cgu->video_clk_ctrl = raw;
			log_debug("CLK_PIXEL_DISP not support rate %ld\n",
			    rate);
			return -EINVAL;
		}
		break;
	// dsi pix
	case CLK_DSI_PIX:
		if (abb_freqs->abb_pixel_freq == rate)
			cgu->video_clk_ctrl &= ~CLK_DSI_PIX_SEL_MASK;
		else if (abb_freqs->abb_pixel_freq / 2 == rate)
			cgu->video_clk_ctrl |= CLK_DSI_PIX_SEL_MASK;
		else {
			log_debug("CLK_DSI_PIX not support rate %ld", rate);
			return -EINVAL;
		}
		break;
	// mipi_csi
	case CLK_VIDEO_MIPI0:
		raw = cgu->mipi_clk_ctrl;
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI0_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 600000000) {
			cgu->mipi_clk_ctrl |= 1 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->mipi_clk_ctrl |= 2 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->mipi_clk_ctrl |= 3 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->mipi_clk_ctrl |= 4 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->mipi_clk_ctrl |= 5 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->mipi_clk_ctrl |= 6 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 125000000) {
			cgu->mipi_clk_ctrl |= 7 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 100000000) {
			cgu->mipi_clk_ctrl |= 8 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->mipi_clk_ctrl |= 9 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 50000000) {
			cgu->mipi_clk_ctrl |= 10 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (rate == 25000000) {
			cgu->mipi_clk_ctrl |= 11 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->mipi_clk_ctrl |= 12 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->mipi_clk_ctrl |= 13 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->mipi_clk_ctrl |= 14 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->mipi_clk_ctrl |= 15 << CLK_VIDEO_MIPI0_SEL_SHIFT;
		} else {
			cgu->mipi_clk_ctrl = raw;
			log_debug("CLK_VIDEO_MIPI0 not support rate %ld\n",
			    rate);
			return -EINVAL;
		}
		break;
	case CLK_VIDEO_MIPI1:
		raw = cgu->mipi_clk_ctrl;
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI1_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 600000000) {
			cgu->mipi_clk_ctrl |= 1 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->mipi_clk_ctrl |= 2 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->mipi_clk_ctrl |= 3 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->mipi_clk_ctrl |= 4 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->mipi_clk_ctrl |= 5 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->mipi_clk_ctrl |= 6 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 125000000) {
			cgu->mipi_clk_ctrl |= 7 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 100000000) {
			cgu->mipi_clk_ctrl |= 8 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->mipi_clk_ctrl |= 9 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 50000000) {
			cgu->mipi_clk_ctrl |= 10 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (rate == 25000000) {
			cgu->mipi_clk_ctrl |= 11 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->mipi_clk_ctrl |= 12 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->mipi_clk_ctrl |= 13 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->mipi_clk_ctrl |= 14 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->mipi_clk_ctrl |= 15 << CLK_VIDEO_MIPI1_SEL_SHIFT;
		} else {
			cgu->mipi_clk_ctrl = raw;
			log_debug("CLK_VIDEO_MIPI1 not support rate %ld\n",
			    rate);
			return -EINVAL;
		}
		break;
	case CLK_VIDEO_MIPI2:
		raw = cgu->mipi_clk_ctrl;
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI2_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 600000000) {
			cgu->mipi_clk_ctrl |= 1 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->mipi_clk_ctrl |= 2 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->mipi_clk_ctrl |= 3 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->mipi_clk_ctrl |= 4 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->mipi_clk_ctrl |= 5 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->mipi_clk_ctrl |= 6 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 125000000) {
			cgu->mipi_clk_ctrl |= 7 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 100000000) {
			cgu->mipi_clk_ctrl |= 8 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->mipi_clk_ctrl |= 9 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 50000000) {
			cgu->mipi_clk_ctrl |= 10 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (rate == 25000000) {
			cgu->mipi_clk_ctrl |= 11 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->mipi_clk_ctrl |= 12 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->mipi_clk_ctrl |= 13 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->mipi_clk_ctrl |= 14 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->mipi_clk_ctrl |= 15 << CLK_VIDEO_MIPI2_SEL_SHIFT;
		} else {
			cgu->mipi_clk_ctrl = raw;
			log_debug("CLK_VIDEO_MIPI2 not support rate %ld\n",
			    rate);
			return -EINVAL;
		}
		break;
	case CLK_VIDEO_MIPI3:
		raw = cgu->mipi_clk_ctrl;
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI3_SEL_MASK;
		if (abb_freqs->abb_pixel_freq == rate) {
			break;
		} else if (rate == 600000000) {
			cgu->mipi_clk_ctrl |= 1 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 500000000) {
			cgu->mipi_clk_ctrl |= 2 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 400000000) {
			cgu->mipi_clk_ctrl |= 3 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 300000000) {
			cgu->mipi_clk_ctrl |= 4 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 200000000) {
			cgu->mipi_clk_ctrl |= 5 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 150000000) {
			cgu->mipi_clk_ctrl |= 6 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 125000000) {
			cgu->mipi_clk_ctrl |= 7 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 100000000) {
			cgu->mipi_clk_ctrl |= 8 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 75000000) {
			cgu->mipi_clk_ctrl |= 9 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 50000000) {
			cgu->mipi_clk_ctrl |= 10 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (rate == 25000000) {
			cgu->mipi_clk_ctrl |= 11 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq == rate) {
			cgu->mipi_clk_ctrl |= 12 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 2 == rate) {
			cgu->mipi_clk_ctrl |= 13 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 4 == rate) {
			cgu->mipi_clk_ctrl |= 14 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else if (abb_freqs->abb_backup_freq / 8 == rate) {
			cgu->mipi_clk_ctrl |= 15 << CLK_VIDEO_MIPI3_SEL_SHIFT;
		} else {
			cgu->mipi_clk_ctrl = raw;
			log_debug("CLK_VIDEO_MIPI3 not support rate %ld\n",
			    rate);
			return -EINVAL;
		}
		break;
	case CLK_PCS_MIPI:
		raw = cgu->mipi_clk_ctrl;
		cgu->mipi_clk_ctrl &= ~CLK_PCS_MIPI_SEL_MASK;
		if (rate == 200000000) {
			break;
		} else if (rate == 333000000) {
			cgu->mipi_clk_ctrl |= 1 << CLK_PCS_MIPI_SEL_SHIFT;
		} else {
			cgu->mipi_clk_ctrl = raw;
			log_debug("CLK_PCS_MIPI not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	// gmac
	case CLK_CFG_GMAC:
		raw = cgu->gmac_clk_ctrl;
		cgu->gmac_clk_ctrl &= ~CLK_CFG_GMAC_SEL_MASK;
		if (rate == 150000000) {
			break;
		} else if (rate == 75000000) {
			cgu->gmac_clk_ctrl |= 1 << CLK_CFG_GMAC_SEL_SHIFT;
		} else if (rate == 37500000) {
			cgu->gmac_clk_ctrl |= 2 << CLK_CFG_GMAC_SEL_SHIFT;
		} else if (rate == 18750000) {
			cgu->gmac_clk_ctrl |= 3 << CLK_CFG_GMAC_SEL_SHIFT;
		} else {
			cgu->gmac_clk_ctrl = raw;
			log_debug("CLK_CFG_GMAC not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	case CLK_PHY_GMAC:
		raw = cgu->gmac_clk_ctrl;
		cgu->mipi_clk_ctrl &= ~CLK_PHY_GMAC_SEL_MASK;
		if (rate == 50000000) {
			break;
		} else if (rate == 25000000) {
			cgu->gmac_clk_ctrl |= 1 << CLK_PHY_GMAC_SEL_SHIFT;
		} else {
			cgu->gmac_clk_ctrl = raw;
			log_debug("CLK_PHY_GMAC not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	// sensor
	case CLK_SENSOR0:
		raw = cgu->sensor_clk_ctrl;
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR0_SEL_MASK;
		if (rate == 74250000) {
			break;
		} else if (rate == 37125000) {
			cgu->sensor_clk_ctrl |= 1 << CLK_SENSOR0_SEL_SHIFT;
		} else if (rate == 27000000) {
			cgu->sensor_clk_ctrl |= 2 << CLK_SENSOR0_SEL_SHIFT;
		} else if (rate == 24000000) {
			cgu->sensor_clk_ctrl |= 3 << CLK_SENSOR0_SEL_SHIFT;
		} else {
			cgu->sensor_clk_ctrl = raw;
			log_debug("CLK_SENSOR0 not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	case CLK_SENSOR1:
		raw = cgu->sensor_clk_ctrl;
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR1_SEL_MASK;
		if (rate == 74250000) {
			break;
		} else if (rate == 37125000) {
			cgu->sensor_clk_ctrl |= 1 << CLK_SENSOR1_SEL_SHIFT;
		} else if (rate == 27000000) {
			cgu->sensor_clk_ctrl |= 2 << CLK_SENSOR1_SEL_SHIFT;
		} else if (rate == 24000000) {
			cgu->sensor_clk_ctrl |= 3 << CLK_SENSOR1_SEL_SHIFT;
		} else {
			cgu->sensor_clk_ctrl = raw;
			log_debug("CLK_SENSOR1 not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	case CLK_SENSOR2:
		raw = cgu->sensor_clk_ctrl;
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR2_SEL_MASK;
		if (rate == 74250000) {
			break;
		} else if (rate == 37125000) {
			cgu->sensor_clk_ctrl |= 1 << CLK_SENSOR2_SEL_SHIFT;
		} else if (rate == 27000000) {
			cgu->sensor_clk_ctrl |= 2 << CLK_SENSOR2_SEL_SHIFT;
		} else if (rate == 24000000) {
			cgu->sensor_clk_ctrl |= 3 << CLK_SENSOR2_SEL_SHIFT;
		} else {
			cgu->sensor_clk_ctrl = raw;
			log_debug("CLK_SENSOR2 not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
	case CLK_SENSOR3:
		raw = cgu->sensor_clk_ctrl;
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR3_SEL_MASK;
		if (rate == 74250000) {
			break;
		} else if (rate == 37125000) {
			cgu->sensor_clk_ctrl |= 1 << CLK_SENSOR3_SEL_SHIFT;
		} else if (rate == 27000000) {
			cgu->sensor_clk_ctrl |= 2 << CLK_SENSOR3_SEL_SHIFT;
		} else if (rate == 24000000) {
			cgu->sensor_clk_ctrl |= 3 << CLK_SENSOR3_SEL_SHIFT;
		} else {
			cgu->sensor_clk_ctrl = raw;
			log_debug("CLK_SENSOR3_SEL not support rate %ld\n",
			    rate);
			return -EINVAL;
		}
		break;
	// sdcard
	case CLK_SDC_DRV:
		raw = cgu->perip_clk_ctrl;
		cgu->perip_clk_ctrl &= ~CLK_SDC_DRV_SEL_MASK;
		if ((rate != 0) && (rate != 1)) {
			cgu->perip_clk_ctrl = raw;
			log_debug("CLK_SDC_DRV not support rate %ld\n", rate);
			return -EINVAL;
		}
		cgu->perip_clk_ctrl |= rate << CLK_SDC_DRV_SEL_SHIFT;
		break;
	// qspi
	case CLK_CORE_QSPI:
		raw = cgu->perip_clk_ctrl;
		cgu->perip_clk_ctrl &= ~CLK_CORE_QSPI_SEL_MASK;
		if (rate == 600000000) {
			break;
		} else if (400000000) {
			cgu->perip_clk_ctrl |= 1 << CLK_CORE_QSPI_SEL_SHIFT;
		} else {
			cgu->perip_clk_ctrl = raw;
			log_debug("CLK_CORE_QSPI not support rate %ld\n", rate);
			return -EINVAL;
		}
		break;
		// i2sm
#if 0
	case CLK_AUDIO:
	case I2S_MST0_DIV:
	case I2S_MST1_DIV:
	case I2S_MST2_DIV:
		break;
#endif
	case CLK_PERIP_I2SM:
		cgu->perip_clk_ctrl &= ~CLK_AUDIO_SEL_MASK;
		if (rate < 500000)
			rate *= 4;
		cgu->perip_clk_ctrl |= 3 << CLK_AUDIO_SEL_SHIFT;
		sca200v100_abb_set_audio(priv, rate);
		break;
	case CLK_PLL_PIXEL:
		sca200v100_abb_set_pixel(priv, rate);
		break;
	default:
		log_debug("Unknown clock %lu\n", clk->id);
		return -ENOENT;
	}
#endif
	return ret;
}

static int __maybe_unused sca200v100_clk_set_parent(struct clk *clk,
    struct clk *parent)
{
	//  struct clk *c, *cp;
	int ret = 0;

	debug("%s(#%lu), parent: %lu\n", __func__, clk->id, parent->id);
#if 0
	ret = clk_get_by_id(clk->id, &c);
	if (ret)
		return ret;

	ret = clk_get_by_id(parent->id, &cp);
	if (ret)
		return ret;

	c->dev->parent = cp->dev;
#endif
	clk->dev->parent = parent->dev;

	return ret;
}

static int sca200v100_clk_enable_byid(struct sca200v100_cgu *cgu, u32 clk_id)
{
	switch (clk_id) {
	// coresight
	case CLK_CFG_CS:
		cgu->perip_clk_ctrl |= CLK_CFG_CS_ON_MASK;
		break;
	case CLK_CORE_CS:
		cgu->perip_clk_ctrl |= CLK_CORE_CS_ON_MASK;
		break;
	// hdecomp
	case CLK_CFG_HDECOMP:
		cgu->hdecomp_clk_ctrl |= CLK_CFG_HDECOMP_ON_MASK;
		break;
	case CLK_CORE_HDECOMP:
		cgu->hdecomp_clk_ctrl |= CLK_CORE_HDECOMP_ON_MASK;
		break;
	// topdmac
	case CLK_CFG_TOPDMAC:
		cgu->dmac_clk_ctrl |= CLK_CFG_TOPDMAC_ON_MASK;
		break;
	case CLK_AXI_TOPDMAC:
		cgu->dmac_clk_ctrl |= CLK_AXI_TOPDMAC_ON_MASK;
		break;
	case CLK_CORE_TOPDMAC:
		cgu->dmac_clk_ctrl |= CLK_CORE_TOPDMAC_ON_MASK;
		break;
	// emmc
	case CLK_CFG_EMMC:
		cgu->emmc_clk_ctrl |= CLK_CFG_EMMC_ON_MASK;
		break;
	case CLK_EMMC_CARDCLK:
		cgu->emmc_clk_ctrl |= CLK_EMMC_CARDCLK_ON_MASK;
		break;
	// security
	case CLK_CFG_SEC:
		cgu->sec_clk_ctrl |= CLK_CFG_SEC_ON_MASK;
		break;
	case CLK_CFG_EFUSE:
		cgu->sec_clk_ctrl |= CLK_CFG_EFUSE_ON_MASK;
		break;
	// rom
	case CLK_CFG_ROM:
		cgu->rom_clk_ctrl |= CLK_CFG_ROM_ON_MASK;
		break;
	// spi debug
	case CLK_CFG_SPIDBG:
		cgu->perip_clk_ctrl |= CLK_CFG_SPIDBG_ON_MASK;
		break;
	// unknown
	case CLK_150_NOC_G0:
		cgu->noc_clk_ctrl |= CLK_150_NOC_G0_ON_MASK;
		break;
	// isp rsz ifc eis
	case CLK_CFG_ISP:
		cgu->video_clk_ctrl |= CLK_CFG_ISP_ON_MASK;
		break;
	case CLK_ISP:
		cgu->video_clk_ctrl |= CLK_ISP_ON_MASK;
		break;
	case CLK_AXI_VIF:
		cgu->video_if_clk_ctrl |= CLK_AXI_VIF_ON_MASK;
		break;
	case CLK_HDR:
		cgu->video_if_clk_ctrl |= CLK_HDR_ON_MASK;
		break;
	case CLK_CFG_SCALER:
		cgu->video_if_clk_ctrl |= CLK_CFG_SCALER_ON_MASK;
		break;
	case CLK_SCALER:
		cgu->video_if_clk_ctrl |= CLK_SCALER_ON_MASK;
		break;
	// jpeg
	case CLK_CFG_JPEG:
		cgu->codec_clk_ctrl |= CLK_CFG_JPEG_ON_MASK;
		break;
	case CLK_CORE_JPEG:
		cgu->codec_clk_ctrl |= CLK_CORE_JPEG_ON_MASK;
		break;
	// disp
	case CLK_CFG_DISP:
		cgu->video_clk_ctrl |= CLK_CFG_DISP_ON_MASK;
		break;
	case CLK_CORE_DISP:
		cgu->video_clk_ctrl |= CLK_CORE_DISP_ON_MASK;
		break;
	// Vision Network
	case CLK_150_NOC_G1:
		cgu->noc_clk_ctrl |= CLK_150_NOC_G1_ON_MASK;
		break;
	// ceva top
	case CLK_CFG_CEVA:
		cgu->ceva_clk_ctrl |= CLK_CFG_CEVA_ON_MASK;
		break;
	case CLK_150_NOC_G2:
		cgu->noc_clk_ctrl |= CLK_150_NOC_G2_ON_MASK;
		break;
	// dla_top
	case CLK_CFG_DLA:
		cgu->dla_clk_ctrl |= CLK_CFG_DLA_ON_MASK;
		break;
	// venc
	case CLK_CFG_VENC:
		cgu->codec_clk_ctrl |= CLK_CFG_VENC_ON_MASK;
		break;
	case CLK_AXI_VENC:
		cgu->codec_clk_ctrl |= CLK_AXI_VENC_ON_MASK;
		break;
	case CLK_BPU_VENC:
		cgu->codec_clk_ctrl |= CLK_BPU_VENC_ON_MASK;
		break;
	// unknown
	case CLK_150_NOC_G3:
		cgu->noc_clk_ctrl |= CLK_150_NOC_G3_ON_MASK;
		break;
	// unknown
	case CLK_150_NOC_G4:
		cgu->noc_clk_ctrl |= CLK_150_NOC_G4_ON_MASK;
		break;
	// disp
	case CLK_PIXEL_DISP:
		cgu->video_clk_ctrl |= CLK_PIXEL_DISP_ON_MASK;
		break;
	// dvpctl
	case CLK_PATTERN_GEN:
		cgu->video_clk_ctrl |= CLK_PATTERN_GEN_ON_MASK;
		break;
	// dvp_pin_topology
	case CLK_SUB_1_1X_PIX:
		cgu->video_clk_ctrl |= CLK_SUB_1_1X_PIX_ON_MASK;
		break;
	// disp vif
	case CLK_DVP_PIX_HLY:
		cgu->video_clk_ctrl |= CLK_DVP_PIX_HLY_ON_MASK;
		break;
	// vif dvp_pin_topology
	case CLK_SUB_1_2X_PIX:
		cgu->video_clk_ctrl |= CLK_SUB_1_2X_PIX_ON_MASK;
		break;
	// vif
	case CLK_DVP_PIX:
		cgu->video_clk_ctrl |= CLK_DVP_PIX_ON_MASK;
		break;
	// mipi_dsi  disp
	case CLK_DSI_PIX:
		cgu->video_clk_ctrl |= CLK_DSI_PIX_ON_MASK;
		break;
	// ddr
	case CLK_CFG_DDR:
		cgu->ddr_clk_ctrl |= CLK_CFG_DDR_ON_MASK;
		break;
	case CLK_DDR:
		cgu->ddr_clk_ctrl |= CLK_DDR_ON_MASK;
		break;
	// mipi_csi
	case CLK_CFG_MIPI:
		cgu->mipi_clk_ctrl |= CLK_CFG_MIPI_ON_MASK;
		break;
	case CLK_VIDEO_MIPI0:
		cgu->mipi_clk_ctrl |= CLK_VIDEO_MIPI0_ON_MASK;
		break;
	case CLK_VIDEO_MIPI1:
		cgu->mipi_clk_ctrl |= CLK_VIDEO_MIPI1_ON_MASK;
		break;
	case CLK_VIDEO_MIPI2:
		cgu->mipi_clk_ctrl |= CLK_VIDEO_MIPI2_ON_MASK;
		break;
	case CLK_VIDEO_MIPI3:
		cgu->mipi_clk_ctrl |= CLK_VIDEO_MIPI3_ON_MASK;
		break;
	case CLK_PCS_MIPI:
		cgu->mipi_clk_ctrl |= CLK_PCS_MIPI_ON_MASK;
		break;
	// mipi_dsi
	case CLK_CFG_DSI:
		cgu->mipi_clk_ctrl |= CLK_CFG_DSI_ON_MASK;
		break;
	// vif
	case CLK_CFG_VIF:
		cgu->video_if_clk_ctrl |= CLK_CFG_VIF_ON_MASK;
		break;
	// dvpctl
	case CLK_CFG_DVPCTL:
		cgu->video_clk_ctrl |= CLK_CFG_DVPCTL_ON_MASK;
		break;
	// usb20_0
	case CLK_CFG_USB2OTG0:
		cgu->usb20_clk_ctrl |= CLK_CFG_USB2OTG0_ON_MASK;
		break;
	case CLK_PHY_USB2PHY0:
		cgu->usb20_clk_ctrl |= CLK_PHY_USB2PHY0_ON_MASK;
		break;
	// usb20_1
	case CLK_CFG_USB2OTG1:
		cgu->usb20_clk_ctrl |= CLK_CFG_USB2OTG1_ON_MASK;
		break;
	case CLK_PHY_USB2PHY1:
		cgu->usb20_clk_ctrl |= CLK_PHY_USB2PHY1_ON_MASK;
		break;
	// USB20_2
	case CLK_CFG_USB2OTG2:
		cgu->usb20_clk_ctrl |= CLK_CFG_USB2OTG2_ON_MASK;
		break;
	case CLK_PHY_USB2PHY2:
		cgu->usb20_clk_ctrl |= CLK_PHY_USB2PHY2_ON_MASK;
		break;
	// unknown
	case CLK_150_NOC_G5:
		cgu->noc_clk_ctrl |= CLK_150_NOC_G5_ON_MASK;
		break;
	// gmac
	case CLK_CFG_GMAC:
		cgu->gmac_clk_ctrl |= CLK_CFG_GMAC_ON_MASK;
		break;
	case CLK_CORE_GMAC:
		cgu->gmac_clk_ctrl |= CLK_CORE_GMAC_ON_MASK;
		break;
	case CLK_PHY_GMAC:
		cgu->gmac_clk_ctrl |= CLK_PHY_GMAC_ON_MASK;
		break;
	// cci
	case CLK_CCI:
		cgu->noc_clk_ctrl |= CLK_CCI_ON_MASK;
		break;
	// sram
	case CLK_SRAM:
		cgu->sram_clk_ctrl |= CLK_SRAM_ON_MASK;
		break;
	// unknow
	case CLK_600_NOC:
		cgu->noc_clk_ctrl |= CLK_600_NOC_ON_MASK;
		break;
	// clk sensor
	case CLK_SENSOR0:
		cgu->sensor_clk_ctrl |= CLK_SENSOR0_ON_MASK;
		break;
	case CLK_SENSOR1:
		cgu->sensor_clk_ctrl |= CLK_SENSOR1_ON_MASK;
		break;
	case CLK_SENSOR2:
		cgu->sensor_clk_ctrl |= CLK_SENSOR2_ON_MASK;
		break;
	case CLK_SENSOR3:
		cgu->sensor_clk_ctrl |= CLK_SENSOR3_ON_MASK;
		break;
	// peripheral
	case CLK_CORE_PERIP:
		cgu->perip_clk_ctrl |= CLK_CORE_PERIP_ON_MASK;
		break;
	// audio_codec
	case CLK_AUDIO_300M:
		cgu->audio_clk_ctrl |= CLK_AUDIO_300M_ON_MASK;
		break;
	case CLK_AUDIO_ADC:
		cgu->audio_clk_ctrl |= CLK_AUDIO_ADC_ON_MASK;
		break;
	// i2s_mst
	case CLK_PERIP_I2SM:
		cgu->perip_clk_ctrl |= CLK_PERIP_I2SM_ON_MASK;
		break;
	// sdcard
	case CLK_SDC_FIXED:
		cgu->perip_clk_ctrl |= CLK_SDC_FIXED_ON_MASK;
		break;
	case CLK_SDC_SAMPLE:
		cgu->perip_clk_ctrl |= CLK_SDC_SAMPLE_ON_MASK;
		break;
	case CLK_SDC_DRV:
		cgu->perip_clk_ctrl |= CLK_SDC_DRV_ON_MASK;
		break;
	// qspi
	case CLK_CORE_QSPI:
		cgu->perip_clk_ctrl |= CLK_CORE_QSPI_ON_MASK;
		break;

	default:
		debug("%s: unsupported clk %d\n", __func__, clk_id);
		return -ENOENT;
	}

	return 0;
}

static int sca200v100_clk_enable(struct clk *clk)
{
	struct sca200v100_clock *priv = dev_get_priv(clk->dev);
	struct sca200v100_cgu *cgu = priv->cgu;

	return sca200v100_clk_enable_byid(cgu, clk->id);
}

static int sca200v100_clk_disable_byid(struct sca200v100_cgu *cgu, u32 clk_id)
{
	switch (clk_id) {
	// coresight
	case CLK_CFG_CS:
		cgu->perip_clk_ctrl &= ~CLK_CFG_CS_ON_MASK;
		break;
	case CLK_CORE_CS:
		cgu->perip_clk_ctrl &= ~CLK_CORE_CS_ON_MASK;
		break;
	// hdecomp
	case CLK_CFG_HDECOMP:
		cgu->hdecomp_clk_ctrl &= ~CLK_CFG_HDECOMP_ON_MASK;
		break;
	case CLK_CORE_HDECOMP:
		cgu->hdecomp_clk_ctrl &= ~CLK_CORE_HDECOMP_ON_MASK;
		break;
	// topdmac
	case CLK_CFG_TOPDMAC:
		cgu->dmac_clk_ctrl &= ~CLK_CFG_TOPDMAC_ON_MASK;
		break;
	case CLK_AXI_TOPDMAC:
		cgu->dmac_clk_ctrl &= ~CLK_AXI_TOPDMAC_ON_MASK;
		break;
	case CLK_CORE_TOPDMAC:
		cgu->dmac_clk_ctrl &= ~CLK_CORE_TOPDMAC_ON_MASK;
		break;
	// emmc
	case CLK_CFG_EMMC:
		cgu->emmc_clk_ctrl &= ~CLK_CFG_EMMC_ON_MASK;
		break;
	case CLK_EMMC_CARDCLK:
		cgu->emmc_clk_ctrl &= ~CLK_EMMC_CARDCLK_ON_MASK;
		break;
	// security
	case CLK_CFG_SEC:
		cgu->sec_clk_ctrl &= ~CLK_CFG_SEC_ON_MASK;
		break;
	case CLK_CFG_EFUSE:
		cgu->sec_clk_ctrl &= ~CLK_CFG_EFUSE_ON_MASK;
		break;
	// rom
	case CLK_CFG_ROM:
		cgu->rom_clk_ctrl &= ~CLK_CFG_ROM_ON_MASK;
		break;
	// spi debug
	case CLK_CFG_SPIDBG:
		cgu->perip_clk_ctrl &= ~CLK_CFG_SPIDBG_ON_MASK;
		break;
	// unknown
	case CLK_150_NOC_G0:
		cgu->noc_clk_ctrl &= ~CLK_150_NOC_G0_ON_MASK;
		break;
	// isp rsz ifc eis
	case CLK_CFG_ISP:
		cgu->video_clk_ctrl &= ~CLK_CFG_ISP_ON_MASK;
		break;
	case CLK_ISP:
		cgu->video_clk_ctrl &= ~CLK_ISP_ON_MASK;
		break;
	case CLK_AXI_VIF:
		cgu->video_if_clk_ctrl &= ~CLK_AXI_VIF_ON_MASK;
		break;
	case CLK_HDR:
		cgu->video_if_clk_ctrl &= ~CLK_HDR_ON_MASK;
		break;
	case CLK_CFG_SCALER:
		cgu->video_if_clk_ctrl &= ~CLK_CFG_SCALER_ON_MASK;
		break;
	case CLK_SCALER:
		cgu->video_if_clk_ctrl &= ~CLK_SCALER_ON_MASK;
		break;
	// jpeg
	case CLK_CFG_JPEG:
		cgu->codec_clk_ctrl &= ~CLK_CFG_JPEG_ON_MASK;
		break;
	case CLK_CORE_JPEG:
		cgu->codec_clk_ctrl &= ~CLK_CORE_JPEG_ON_MASK;
		break;
	// disp
	case CLK_CFG_DISP:
		cgu->video_clk_ctrl &= ~CLK_CFG_DISP_ON_MASK;
		break;
	case CLK_CORE_DISP:
		cgu->video_clk_ctrl &= ~CLK_CORE_DISP_ON_MASK;
		break;
	// Vision Network
	case CLK_150_NOC_G1:
		cgu->noc_clk_ctrl &= ~CLK_150_NOC_G1_ON_MASK;
		break;
	// ceva top
	case CLK_CFG_CEVA:
		cgu->ceva_clk_ctrl &= ~CLK_CFG_CEVA_ON_MASK;
		break;
	case CLK_150_NOC_G2:
		cgu->noc_clk_ctrl &= ~CLK_150_NOC_G2_ON_MASK;
		break;
	// dla_top
	case CLK_CFG_DLA:
		cgu->dla_clk_ctrl &= ~CLK_CFG_DLA_ON_MASK;
		break;
	// venc
	case CLK_CFG_VENC:
		cgu->codec_clk_ctrl &= ~CLK_CFG_VENC_ON_MASK;
		break;
	case CLK_AXI_VENC:
		cgu->codec_clk_ctrl &= ~CLK_AXI_VENC_ON_MASK;
		break;
	case CLK_BPU_VENC:
		cgu->codec_clk_ctrl &= ~CLK_BPU_VENC_ON_MASK;
		break;
	// unknown
	case CLK_150_NOC_G3:
		cgu->noc_clk_ctrl &= ~CLK_150_NOC_G3_ON_MASK;
		break;
	// unknown
	case CLK_150_NOC_G4:
		cgu->noc_clk_ctrl &= ~CLK_150_NOC_G4_ON_MASK;
		break;
	// disp
	case CLK_PIXEL_DISP:
		cgu->video_clk_ctrl &= ~CLK_PIXEL_DISP_ON_MASK;
		break;
	// dvpctl
	case CLK_PATTERN_GEN:
		cgu->video_clk_ctrl &= ~CLK_PATTERN_GEN_ON_MASK;
		break;
	// dvp_pin_topology
	case CLK_SUB_1_1X_PIX:
		cgu->video_clk_ctrl &= ~CLK_SUB_1_1X_PIX_ON_MASK;
		break;
	// disp vif
	case CLK_DVP_PIX_HLY:
		cgu->video_clk_ctrl &= ~CLK_DVP_PIX_HLY_ON_MASK;
		break;
	// vif dvp_pin_topology
	case CLK_SUB_1_2X_PIX:
		cgu->video_clk_ctrl &= ~CLK_SUB_1_2X_PIX_ON_MASK;
		break;
	// vif
	case CLK_DVP_PIX:
		cgu->video_clk_ctrl &= ~CLK_DVP_PIX_ON_MASK;
		break;
	// mipi_dsi  disp
	case CLK_DSI_PIX:
		cgu->video_clk_ctrl &= ~CLK_DSI_PIX_ON_MASK;
		break;
	// ddr
	case CLK_CFG_DDR:
		cgu->ddr_clk_ctrl &= ~CLK_CFG_DDR_ON_MASK;
		break;
	case CLK_DDR:
		cgu->ddr_clk_ctrl &= ~CLK_DDR_ON_MASK;
		break;
	// mipi_csi
	case CLK_CFG_MIPI:
		cgu->mipi_clk_ctrl &= ~CLK_CFG_MIPI_ON_MASK;
		break;
	case CLK_VIDEO_MIPI0:
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI0_ON_MASK;
		break;
	case CLK_VIDEO_MIPI1:
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI1_ON_MASK;
		break;
	case CLK_VIDEO_MIPI2:
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI2_ON_MASK;
		break;
	case CLK_VIDEO_MIPI3:
		cgu->mipi_clk_ctrl &= ~CLK_VIDEO_MIPI3_ON_MASK;
		break;
	case CLK_PCS_MIPI:
		cgu->mipi_clk_ctrl &= ~CLK_PCS_MIPI_ON_MASK;
		break;
	// mipi_dsi
	case CLK_CFG_DSI:
		cgu->mipi_clk_ctrl &= ~CLK_CFG_DSI_ON_MASK;
		break;
	// vif
	case CLK_CFG_VIF:
		cgu->video_if_clk_ctrl &= ~CLK_CFG_VIF_ON_MASK;
		break;
	// dvpctl
	case CLK_CFG_DVPCTL:
		cgu->video_clk_ctrl &= ~CLK_CFG_DVPCTL_ON_MASK;
		break;
	// usb20_0
	case CLK_CFG_USB2OTG0:
		cgu->usb20_clk_ctrl &= ~CLK_CFG_USB2OTG0_ON_MASK;
		break;
	case CLK_PHY_USB2PHY0:
		cgu->usb20_clk_ctrl &= ~CLK_PHY_USB2PHY0_ON_MASK;
		break;
	// usb20_1
	case CLK_CFG_USB2OTG1:
		cgu->usb20_clk_ctrl &= ~CLK_CFG_USB2OTG1_ON_MASK;
		break;
	case CLK_PHY_USB2PHY1:
		cgu->usb20_clk_ctrl &= ~CLK_PHY_USB2PHY1_ON_MASK;
		break;
	// USB20_2
	case CLK_CFG_USB2OTG2:
		cgu->usb20_clk_ctrl &= ~CLK_CFG_USB2OTG2_ON_MASK;
		break;
	case CLK_PHY_USB2PHY2:
		cgu->usb20_clk_ctrl &= ~CLK_PHY_USB2PHY2_ON_MASK;
		break;
	// unknown
	case CLK_150_NOC_G5:
		cgu->noc_clk_ctrl &= ~CLK_150_NOC_G5_ON_MASK;
		break;
	// gmac
	case CLK_CFG_GMAC:
		cgu->gmac_clk_ctrl &= ~CLK_CFG_GMAC_ON_MASK;
		break;
	case CLK_CORE_GMAC:
		cgu->gmac_clk_ctrl &= ~CLK_CORE_GMAC_ON_MASK;
		break;
	case CLK_PHY_GMAC:
		cgu->gmac_clk_ctrl &= ~CLK_PHY_GMAC_ON_MASK;
		break;
	// cci
	case CLK_CCI:
		cgu->noc_clk_ctrl &= ~CLK_CCI_ON_MASK;
		break;
	// sram
	case CLK_SRAM:
		cgu->sram_clk_ctrl &= ~CLK_SRAM_ON_MASK;
		break;
	// unknow
	case CLK_600_NOC:
		cgu->noc_clk_ctrl &= ~CLK_600_NOC_ON_MASK;
		break;
	// clk sensor
	case CLK_SENSOR0:
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR0_ON_MASK;
		break;
	case CLK_SENSOR1:
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR1_ON_MASK;
		break;
	case CLK_SENSOR2:
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR2_ON_MASK;
		break;
	case CLK_SENSOR3:
		cgu->sensor_clk_ctrl &= ~CLK_SENSOR3_ON_MASK;
		break;
	// peripheral
	case CLK_CORE_PERIP:
		cgu->perip_clk_ctrl &= ~CLK_CORE_PERIP_ON_MASK;
		break;
	// audio_codec
	case CLK_AUDIO_300M:
		cgu->audio_clk_ctrl &= ~CLK_AUDIO_300M_ON_MASK;
		break;
	case CLK_AUDIO_ADC:
		cgu->audio_clk_ctrl &= ~CLK_AUDIO_ADC_ON_MASK;
		break;
	// i2s_mst
	case CLK_PERIP_I2SM:
		cgu->perip_clk_ctrl &= ~CLK_PERIP_I2SM_ON_MASK;
		break;
	// sdcard
	case CLK_SDC_FIXED:
		cgu->perip_clk_ctrl &= ~CLK_SDC_FIXED_ON_MASK;
		break;
	case CLK_SDC_SAMPLE:
		cgu->perip_clk_ctrl &= ~CLK_SDC_SAMPLE_ON_MASK;
		break;
	case CLK_SDC_DRV:
		cgu->perip_clk_ctrl &= ~CLK_SDC_DRV_ON_MASK;
		break;
	// qspi
	case CLK_CORE_QSPI:
		cgu->perip_clk_ctrl &= ~CLK_CORE_QSPI_ON_MASK;
		break;

	default:
		debug("%s: unsupported clk %d\n", __func__, clk_id);
		return -ENOENT;
	}

	return 0;
}

static int sca200v100_clk_disable(struct clk *clk)
{
	struct sca200v100_clock *priv = dev_get_priv(clk->dev);
	struct sca200v100_cgu *cgu = priv->cgu;

	return sca200v100_clk_disable_byid(cgu, clk->id);
}

static struct clk_ops sca200v100_clk_ops = {
	.get_rate = sca200v100_clk_get_rate,
	.set_rate = sca200v100_clk_set_rate,
#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
	.set_parent = sca200v100_clk_set_parent,
#endif
	.enable = sca200v100_clk_enable,
	.disable = sca200v100_clk_disable,
};

static int sca200v100_abb_init(struct udevice *dev)
{
	struct sca200v100_clock *priv = dev_get_priv(dev);
	int ret;

	ret = dev_read_u32_array(dev, "abb-freqs",
	        (u32 *)&priv->abb_freqs,
	        sizeof(struct sca200v100_abb_freqs) / sizeof(u32));
	if (ret) {
		log_err("%s: read abb-freqs failed %d\n", __func__, ret);
		return -EINVAL;
	}
	sca200v100_abb_set_emmc(priv, priv->abb_freqs.abb_emmc_freq);
	sca200v100_abb_set_sdc0(priv, priv->abb_freqs.abb_sdc0_freq);
	sca200v100_abb_set_sdc1(priv, priv->abb_freqs.abb_sdc1_freq);
	sca200v100_abb_set_pixel(priv, priv->abb_freqs.abb_pixel_freq);
	sca200v100_abb_set_audio(priv, priv->abb_freqs.abb_audio_freq);
	sca200v100_abb_set_audio1(priv, priv->abb_freqs.abb_audio1_freq);
	sca200v100_abb_set_audio2(priv, priv->abb_freqs.abb_audio2_freq);
	sca200v100_abb_set_backup(priv, priv->abb_freqs.abb_backup_freq);
	sca200v100_abb_set_a53(priv, priv->abb_freqs.abb_a53_freq);
	sca200v100_abb_set_dla(priv, priv->abb_freqs.abb_dla_freq);
	sca200v100_abb_set_noc(priv, priv->abb_freqs.abb_noc_freq);
	sca200v100_abb_set_quirks(priv);

	return 0;
}

static int sca200v100_cgu_init(struct udevice *dev)
{
	struct sca200v100_clock *priv = dev_get_priv(dev);
	u32 array[CLK_MAX_GATING], size;
	u32 i;
	int ret;

	size = dev_read_size(dev, "clk-default-gating");
	if (size < 0) {
		log_err("%s: sca200v100 clk-default-gating size %d error\n",
		    __func__, size);
		return -EINVAL;
	}

	ret = dev_read_u32_array(dev, "clk-default-gating", (u32 *)array,
	        size / sizeof(u32));
	if (ret) {
		log_err("%s: read clk-default-gating failed %d\n", __func__,
		    ret);
		return -EINVAL;
	}
	for (i = 0; i < size / sizeof(u32); i++) {
		if (array[i] & CLK_INFO_MASK)
			sca200v100_clk_enable_byid(priv->cgu,
			    array[i] & CLK_ID_MASK);
		else
			sca200v100_clk_disable_byid(priv->cgu,
			    array[i] & CLK_ID_MASK);
	}

	size = dev_read_size(dev, "clk-default-sel");
	if (size < 0) {
		log_err("%s: sca200v100 clk-default-sel size %d error\n", __func__,
		    size);
		return -EINVAL;
	}
	ret = dev_read_u32_array(dev, "clk-default-sel", (u32 *)array,
	        size / sizeof(u32));
	for (i = 0; i < size / sizeof(u32); i++) {
		sca200v100_clk_set_sel(priv->cgu, array[i] & CLK_ID_MASK,
		    (array[i] & CLK_INFO_MASK) >> CLK_SELECT_SHIFT);
	}

	return 0;
}

static int sca200v100_clk_probe(struct udevice *dev)
{
#ifdef CONFIG_SPL_BUILD
	//  struct sca200v100_clock_plat *plat = dev_get_platdata(dev);
	//  struct sca200v100_clock *priv = dev_get_priv(dev);

#endif
	return 0;
}

static int sca200v100_clk_ofdata_to_platdata(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	//struct sca200v100_clock_plat *plat = dev_get_platdata(dev);
	struct sca200v100_clock *priv = dev_get_priv(dev);
	int ret;

#define CGU_BASE_NUM    10
	unsigned int tmp[CGU_BASE_NUM];

	ret = dev_read_u32_array(dev, "reg", tmp, CGU_BASE_NUM);
	if (ret) {
		pr_err("%s: read cgu base address failed %d\n", __func__, ret);
		return -EINVAL;
	}
	priv->cgu = (void *)(ulong)tmp[0];
	priv->abb = (void *)(ulong)tmp[2];
	priv->top = (void *)(ulong)tmp[4];
	priv->ceva_cgu = (void *)(ulong)tmp[6];
	priv->a53_cgu = (void *)(ulong)tmp[8];

	sca200v100_abb_init(dev);
	sca200v100_cgu_init(dev);
	sca200v100_abb_set_ceva(priv, priv->abb_freqs.abb_ceva_freq);
#endif
	return 0;
}

static const struct udevice_id sca200v100_cgu_ids[] = {
	{ .compatible = "smartchip,sca200v100-cgu" },
	{ }
};

U_BOOT_DRIVER(clk_sca200v100) = {
	.name       = "sca200v100_cgu",
	.id     = UCLASS_CLK,
	.of_match   = sca200v100_cgu_ids,
	.priv_auto_alloc_size = sizeof(struct sca200v100_clock),
	.ofdata_to_platdata = sca200v100_clk_ofdata_to_platdata,
	.ops        = &sca200v100_clk_ops,
	.probe      = sca200v100_clk_probe,
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	.platdata_auto_alloc_size = sizeof(struct sca200v100_clock_plat),
#endif
};
