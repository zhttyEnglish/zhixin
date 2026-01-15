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
#include <dt-bindings/clock/sca200v200-cgu.h>

struct sca200v200_sys_ctrl {
	u32 sram_top_ctrl;
	u32 boot_rom_ctrl;
	u32 venc_ctrl0;
	u32 venc_ctrl1;
	u32 rsz_ctrl;
	u32 jpeg_ctrl;
	u32 disp_eng_ctrl;
	u32 topdma_ctrl;
	u32 bus_ctrl0;
	u32 bus_ctrl1;
	u32 bus_status;
	u32 cfg_pin;
	u32 chip_id;
	u32 bus_ctrl2;
	u32 bus_ctrl3;
	u32 bus_ctrl4;
	u32 reserved0[48];
	u32 top_reserved_0;
	u32 top_reserved_1;
	u32 top_reserved_2;
	u32 top_reserved_3;
	u32 top_reserved_4;
	u32 top_reserved_5;
	u32 top_reserved_6;
	u32 top_reserved_7;
	u32 top_reserved_8;
	u32 top_reserved_9;
	u32 top_reserved_a;
	u32 top_reserved_b;
	u32 top_reserved_c;
	u32 top_reserved_d;
	u32 top_reserved_e;
	u32 top_reserved_f;
	u32 reserved1[48];
	u32 pmu_ctrl;
	u32 ip_pmu_ctrl;
	u32 ip_pmu_ack;
	u32 reserved2[61];
	u32 outstanding_dma;
	u32 outstanding_isp0;
	u32 outstanding_isp1;
	u32 outstanding_scaler;
	u32 outstanding_de;
	u32 outstanding_jpeg;
	u32 outstanding_dla;
	u32 outstanding_venc;
	u32 outstanding_npu;
	u32 reserved3[55];
	u32 debug_0;
	u32 debug_1;
	u32 debug_2;
	u32 debug_3;
	u32 reserved4[60];
	u32 noc_main_pm;
	u32 noc_vision_pm;
	u32 ddr_ctl_pm;
};

struct sca200v200_abb {
	u32 abb_reg00;
	u32 abb_reg01;
	u32 abb_reg02;
	u32 abb_reg03;
	u32 abb_reg04;
	u32 abb_reg05;
	u32 abb_reg06;
	u32 abb_reg07;
	u32 abb_reg08;
	u32 abb_reg09;
	u32 abb_reg10;
	u32 abb_reg11;
	u32 abb_reg12;
	u32 abb_reg13;
	u32 abb_reg14;
	u32 abb_reg15;
	u32 abb_reg16;
	u32 abb_reg17;
	u32 abb_reg18;
	u32 abb_reg19;
	u32 abb_reg20;
	u32 abb_reg21;
	u32 abb_reg22;
	u32 abb_reg23;
	u32 abb_reg24;
	u32 abb_reg25;
	u32 abb_reg26;
	u32 abb_reg27;
	u32 abb_reg28;
	u32 abb_reg29;
	u32 abb_reg30;
	u32 abb_reg31;
	u32 abb_reg32;
	u32 abb_reg33;
	u32 abb_reg34;
	u32 abb_reg35;
	u32 abb_reg36;
	u32 abb_reg37;
	u32 abb_reg38;
	u32 abb_reg39;
	u32 abb_reg40;
	u32 abb_reg41;
	u32 abb_reg42;
	u32 abb_reg43;
	u32 abb_reg44;
	u32 abb_reg45;
	u32 abb_reg46;
	u32 abb_reg47;
	u32 abb_reg48;
	u32 abb_reg49;
	u32 abb_reg50;
	u32 abb_reg51;
	u32 abb_reg52;
	u32 abb_reg53;
	u32 abb_reg54;
	u32 reverse0[9];
	u32 pll_lock_reg;
	u32 top_reg;
	u32 aupll_reg;
	u32 pll_bypass_reg;
	u32 reverse1[60];
	u32 sar10_work_mode;
	u32 sar10_chan_sel;
	u32 sar10_ready_th;
	u32 sar10_interval;
	u32 sar10_data;
	u32 sar10_int_status;
	u32 sar10_int_clr;
	u32 reverse2[57];
	u32 ts_work_mode;
	u32 ts_chan_sel;
	u32 ts_ready_th;
	u32 ts_interval;
	u32 ts_data;
	u32 ts_int_status;
	u32 ts_int_clr;
	u32 reverse3[57];
	u32 ddrpll_dig_reg0;
	u32 ddrpll_dig_reg1;
	u32 ddrpll_dig_reg2;
	u32 reverse4;
	u32 pixpll_dig_reg0;
	u32 pixpll_dig_reg1;
	u32 pixpll_dig_reg2;
	u32 reverse5;
	u32 aupll_dig_reg0;
	u32 aupll_dig_reg1;
	u32 aupll_dig_reg2;
};

struct sca200v200_rgu {
	u32 reset0_n_ctrl;
	u32 reset0_level;
	u32 reset0_mask;
	u32 hold_cycle_sel;
	u32 reset1_n_ctrl;
	u32 reset1_level;
	u32 reset1_mask;
	u32 reverse0;
	u32 reset2_n_ctrl;
	u32 reset2_level;
	u32 reset2_mask;
	u32 reverse1;
	u32 wdt_rstn_ctrl;
};

struct sca200v200_cgu {
	u32 cgu_clk_ctrl00;
	u32 cgu_clk_ctrl01;
	u32 cgu_clk_ctrl02;
	u32 cgu_clk_ctrl03;
	u32 cgu_clk_ctrl04;
	u32 cgu_clk_ctrl05;
	u32 cgu_clk_ctrl06;
	u32 cgu_clk_ctrl07;
	u32 cgu_clk_ctrl08;
	u32 cgu_clk_ctrl09;
	u32 cgu_clk_ctrl10;
	u32 cgu_clk_ctrl11;
	u32 cgu_clk_ctrl12;
	u32 cgu_clk_ctrl13;
	u32 cgu_clk_ctrl14;
	u32 cgu_clk_ctrl15;
	u32 cgu_clk_ctrl16;
	u32 cgu_clk_ctrl17;
	u32 cgu_clk_ctrl18;
	u32 cgu_clk_ctrl19;
	u32 cgu_clk_ctrl20;
	u32 cgu_clk_ctrl21;
	u32 cgu_clk_ctrl22;
	u32 cgu_clk_ctrl23;
	u32 reverse0[40];
	u32 cgu_clk_en_00;
	u32 cgu_clk_en_01;
	u32 cgu_clk_en_02;
	u32 cgu_clk_en_03;
	u32 cgu_clk_en_04;
	u32 reverse1[59];
	u32 cgu_gen_clk_ctrl;
	u32 cgu_i2s_clk_ctrl;
	u32 cgu_scgmac_clk_ctrl;
};

struct sca200v200_a53 {
	u32 a53_config_reg0;
	u32 a53_config_reg1;
	u32 a53_config_reg2;
	u32 a53_config_reg3;
	u32 a53_config_reg4;
	u32 a53_config_reg5;
	u32 a53_config_reg6;
	u32 a53_config_reg7;
	u32 a53_status_reg0;
	u32 a53_status_reg1;
	u32 a53_status_reg2;
	u32 a53_status_reg3;
	u32 a53_status_reg4;
};

struct sca200v200_abb_freqs {
	u32 abb_pixel_freq;
	u32 abb_pixel1_freq;
	u32 abb_pixel2_freq;
	u32 abb_pixel3_freq;
	u32 abb_pixel4_freq;
	u32 abb_audio_freq;
};

struct sca200v200_clock {
	void __iomem *a53;
	void __iomem *rgu;
	void __iomem *cgu;
	void __iomem *sys_ctrl;
	void __iomem *abb;
	struct sca200v200_abb_freqs abb_freqs;
};

struct sca200v200_clock_plat {
	struct regmap *map;
};

static void sca200v200_abb_set_pixel(struct sca200v200_clock *priv, u32 freq)
{
	struct sca200v200_abb *abb = priv->abb;
	struct sca200v200_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 loop_div, post_div;

	if (freq > 800000000 || freq < 6250000) {
		log_err("Unsupport abb pix freq %d\n", freq);
		return;
	}

	abb->abb_reg38 &= ~0x3f;

	if (freq > 400000000) {
		loop_div = 4;
		post_div = 1;
	} else if (freq > 200000000) {
		loop_div = 4;
		post_div = 2;
	} else if (freq > 100000000) {
		loop_div = 4;
		post_div = 3;
	} else if (freq > 50000000) {
		loop_div = 4;
		post_div = 4;
	} else if (freq > 25000000) {
		loop_div = 4;
		post_div = 5;
	} else if (freq > 12500000) {
		loop_div = 4;
		post_div = 6;
	} else if (freq >= 6250000) {
		loop_div = 4;
		post_div = 7;
	}
	abb->abb_reg38 |= (loop_div | (post_div << 3));
	abb->pixpll_dig_reg0 = (u32)(((u64)400000000) *
	        ((u64)(1 << (24 + loop_div - post_div))) /
	        (u64)freq);

	abb_freqs->abb_pixel_freq = freq;
}

static void sca200v200_abb_set_pixel1(struct sca200v200_clock *priv, u32 freq)
{
	struct sca200v200_abb *abb = priv->abb;
	struct sca200v200_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 pixel_post_div;
	u32 div;

	pixel_post_div = (abb->abb_reg38 >> 3) & 0x7;
	div = abb_freqs->abb_pixel_freq * (1 << pixel_post_div) / freq;
	if (div > 32) {
		log_err("Unsupport pixel1 freq %d div from pixel freq %d\n",
		    freq, abb_freqs->abb_pixel_freq);
	}

	abb->abb_reg23 = div;
	abb_freqs->abb_pixel1_freq = freq;
}

static void sca200v200_abb_set_pixel2(struct sca200v200_clock *priv, u32 freq)
{
	struct sca200v200_abb *abb = priv->abb;
	struct sca200v200_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 pixel_post_div;
	u32 div;

	pixel_post_div = (abb->abb_reg38 >> 3) & 0x7;
	div = abb_freqs->abb_pixel_freq * (1 << pixel_post_div) / freq;
	if (div > 32) {
		log_err("Unsupport pixel2 freq %d div from pixel freq %d\n",
		    freq, abb_freqs->abb_pixel_freq);
	}

	abb->abb_reg24 = div;
	abb_freqs->abb_pixel2_freq = freq;
}

static void sca200v200_abb_set_pixel3(struct sca200v200_clock *priv, u32 freq)
{
	struct sca200v200_abb *abb = priv->abb;
	struct sca200v200_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 pixel_post_div;
	u32 div;

	pixel_post_div = (abb->abb_reg38 >> 3) & 0x7;
	div = abb_freqs->abb_pixel_freq * (1 << pixel_post_div) / freq;
	if (div > 32) {
		log_err("Unsupport pixel3 freq %d div from pixel freq %d\n",
		    freq, abb_freqs->abb_pixel_freq);
	}

	abb->abb_reg25 = div;
	abb_freqs->abb_pixel3_freq = freq;
}

static void sca200v200_abb_set_pixel4(struct sca200v200_clock *priv, u32 freq)
{
	struct sca200v200_abb *abb = priv->abb;
	struct sca200v200_abb_freqs *abb_freqs = &priv->abb_freqs;
	u32 pixel_post_div;
	u32 div;

	pixel_post_div = (abb->abb_reg38 >> 3) & 0x7;
	div = abb_freqs->abb_pixel_freq * (1 << pixel_post_div) / freq;
	if (div > 32) {
		log_err("Unsupport pixel4 freq %d div from pixel freq %d\n",
		    freq, abb_freqs->abb_pixel_freq);
	}

	abb->abb_reg26 = div;
	abb_freqs->abb_pixel4_freq = freq;
}

static void sca200v200_abb_set_audio(struct sca200v200_clock *priv, u32 freq)
{
	struct sca200v200_abb *abb = priv->abb;
	struct sca200v200_abb_freqs *abb_freqs = &priv->abb_freqs;
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

static ulong sca200v200_clk_get_rate(struct clk *clk)
{
	return 0;
}

static void sca200v200_clk_set_no_switch_h(u32 *reg, u32 sel, u32 div)
{
	*reg &= ~(1 << 28);
	*reg &= ~(0x7 << 24);
	*reg |= (sel << 24);
	*reg &= ~(0x7f << 16);
	*reg |= (div << 16);
	*reg |= (1 << 28);
}

static void sca200v200_clk_set_no_switch_l(u32 *reg, u32 sel, u32 div)
{
	*reg &= ~(1 << 12);
	*reg &= ~(0x7 << 8);
	*reg |= (sel << 8);
	*reg &= ~(0x7f << 0);
	*reg |= (div << 0);
	*reg |= (1 << 12);
}

static void sca200v200_clk_set_with_switch(u32 *reg, u32 sel, u32 div)
{
	if (*reg & (1 << 31)) {
		sca200v200_clk_set_no_switch_l(reg, sel, div);
		*reg &= ~(1 << 31);
	} else {
		sca200v200_clk_set_no_switch_h(reg, sel, div);
		*reg |= (1 << 31);
	}
}

static inline void sca200v200_clk_enable_no_switch_h(u32 *reg)
{
	*reg |= (1 << 28);
}

static inline void sca200v200_clk_enable_no_switch_l(u32 *reg)
{
	*reg |= (1 << 12);
}

static inline void sca200v200_clk_disable_no_switch_h(u32 *reg)
{
	*reg &= ~(1 << 28);
}

static inline void sca200v200_clk_disable_no_switch_l(u32 *reg)
{
	*reg &= ~(1 << 12);
}

static void sca200v200_clk_enable_with_switch(u32 *reg)
{
	if (*reg & (1 << 31)) {
		sca200v200_clk_disable_no_switch_l(reg);
		sca200v200_clk_enable_no_switch_h(reg);
	} else {
		sca200v200_clk_disable_no_switch_h(reg);
		sca200v200_clk_enable_no_switch_l(reg);
	}
}

static void sca200v200_clk_disable_with_switch(u32 *reg)
{
	sca200v200_clk_disable_no_switch_l(reg);
	sca200v200_clk_disable_no_switch_h(reg);
}

static ulong sca200v200_clk_set_sel_div(struct sca200v200_clock *sc_clk, u32 clk_id, u32 sel, u32 div)
{
	struct sca200v200_cgu *cgu = sc_clk->cgu;
	struct sca200v200_a53 *a53 = sc_clk->a53;

	switch (clk_id) {
	case CGU_AXI_CLK:
		sca200v200_clk_set_with_switch(&cgu->cgu_clk_ctrl00, sel, div);
		break;
	case CGU_SYS_CLK:
		sca200v200_clk_set_with_switch(&cgu->cgu_clk_ctrl01, sel, div);
		break;
	case CGU_ISP_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl03, sel, div);
		break;
	case CGU_ISP_HDR_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl03, sel, div);
		break;
	case CGU_VIF_AXI_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl04, sel, div);
		break;
	case CGU_DLA_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl04, sel, div);
		break;
	case CGU_RSZ_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl05, sel, div);
		break;
	case CGU_DE_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl05, sel, div);
		break;
	case CGU_VENC_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl06, sel, div);
		break;
	case CGU_JPEG_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl06, sel, div);
		break;
	case CGU_MIPI_CSI_0_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl07, sel, div);
		break;
	case CGU_MIPI_CSI_1_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl07, sel, div);
		break;
	case CGU_MIPI_PCS_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl08, sel, div);
		break;
	case CGU_SD0_FIX_CLK:
		break;
	case CGU_SD0_SAMPLE_CLK:
		break;
	case CGU_SD0_DRV_CLK:
		break;
	case CGU_SD1_FIX_CLK:
		break;
	case CGU_SD1_SAMPLE_CLK:
		break;
	case CGU_SD1_DRV_CLK:
		break;
	case CGU_EMMC_FIX_CLK:
		break;
	case CGU_EMMC_SAMPLE_CLK:
		break;
	case CGU_EMMC_DRV_CLK:
		break;
	case CGU_QSPI_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl13, sel, div);
		break;
	case CGU_I2S_MCLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl13, sel, div);
		break;
	case CGU_I2S_MST0_SCLK:
		cgu->cgu_i2s_clk_ctrl &= ~(1 << 15);
		cgu->cgu_i2s_clk_ctrl &= ~(0x7fff);
		cgu->cgu_i2s_clk_ctrl |= div;
		cgu->cgu_i2s_clk_ctrl |= (1 << 15);
		break;
	case CGU_I2S_MST1_SCLK:
		cgu->cgu_i2s_clk_ctrl &= ~(1 << 31);
		cgu->cgu_i2s_clk_ctrl &= ~(0x7fff << 16);
		cgu->cgu_i2s_clk_ctrl |= (div << 16);
		cgu->cgu_i2s_clk_ctrl |= (1 << 31);
		break;
	case CGU_USB_PHY0_CLK:
		break;
	case CGU_AUDIO_300M:
		break;
	case CGU_AUDIO_ADC_CLK:
		break;
	case CGU_GMAC_CORE_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl15, sel, div);
		break;
	case CGU_GMAC_PHY_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl15, sel, div);
		break;
	case CGU_HDECOMP_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl16, sel, div);
		break;
	case CGU_EFUSE_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl16, sel, div);
		break;
	case CGU_SENSOR_MCLK0:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl17, sel, div);
		break;
	case CGU_SENSOR_MCLK1:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl17, sel, div);
		break;
	case CGU_SENSOR_MCLK2:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl18, sel, div);
		break;
	case CGU_DVP_PATTERN_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl18, sel, div);
		break;
	case CGU_DVP_SUB_1_2X_PIX_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl19, sel, div);
		break;
	case CGU_NUC_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl19, sel, div);
		break;
	case CGU_SCGMAC_PTP_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl20, sel, div);
		break;
	case CGU_SCGMAC_RGMIITX_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl20, sel, div);
		break;
	case CGU_SCGMAC_MDC_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl21, sel, div);
		break;
	case CGU_NPU_CLK:
		sca200v200_clk_set_with_switch(&cgu->cgu_clk_ctrl02, sel, div);
		break;
	case CGU_NPU_ACLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl22, sel, div);
		break;
	case CGU_IFC_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl22, sel, div);
		break;
	case CGU_EIS_CLK:
		sca200v200_clk_set_no_switch_l(&cgu->cgu_clk_ctrl23, sel, div);
		break;
	case CGU_SCALER_CLK:
		sca200v200_clk_set_no_switch_h(&cgu->cgu_clk_ctrl23, sel, div);
		break;
	case CGU_GEN_CLK:
		cgu->cgu_gen_clk_ctrl &= ~(1 << 12);
		cgu->cgu_gen_clk_ctrl &= ~(0x7f);
		cgu->cgu_gen_clk_ctrl |= div;
		cgu->cgu_gen_clk_ctrl &= ~(0x7 << 8);
		cgu->cgu_gen_clk_ctrl |= (sel << 8);
		cgu->cgu_gen_clk_ctrl |= (1 << 12);
		break;
	case CGU_CPU_CLK:
		/* a53_config_reg2: for CGU_CPU_CLK_PRE */
		sca200v200_clk_set_with_switch(&a53->a53_config_reg2, sel, div);
		/* bit 30: CGU_CPU_CLK clock source
		 *  0: CGU_OSCIN_CLK
		 *  1: CGU_CPU_CLK_PRE
		 */
		a53->a53_config_reg2 |= (1 << 30);
		break;
	case CGU_CS_DBG_CLK:
		sca200v200_clk_set_no_switch_l(&a53->a53_config_reg4, sel, div);
		break;
	default:
		log_debug("Unknown clock %d\n", clk_id);
		return -ENOENT;
	}

	return 0;
}

static ulong sca200v200_clk_set_rate(struct clk *clk, ulong rate)
{
#ifndef CONFIG_SPL_BUILD
	//  struct sca200v200_clock *priv = dev_get_priv(clk->dev);
	//  struct sca200v200_cgu *cgu = priv->cgu;
	//  struct sca200v200_abb_freqs *abb_freqs = &priv->abb_freqs;
	//  uint32_t raw;
	//  ulong div;
#endif
	ulong ret = 0;

#ifndef CONFIG_SPL_BUILD
	switch (clk->id) {
	case CGU_AXI_CLK:
	case CGU_SYS_CLK:
	case CGU_ISP_CLK:
	case CGU_ISP_HDR_CLK:
	case CGU_VIF_AXI_CLK:
	case CGU_DLA_CLK:
	case CGU_RSZ_CLK:
	case CGU_DE_CLK:
	case CGU_VENC_CLK:
	case CGU_JPEG_CLK:
	case CGU_MIPI_CSI_0_CLK:
	case CGU_MIPI_CSI_1_CLK:
	case CGU_MIPI_PCS_CLK:
	case CGU_SD0_FIX_CLK:
	case CGU_SD0_SAMPLE_CLK:
	case CGU_SD0_DRV_CLK:
	case CGU_SD1_FIX_CLK:
	case CGU_SD1_SAMPLE_CLK:
	case CGU_SD1_DRV_CLK:
	case CGU_EMMC_FIX_CLK:
	case CGU_EMMC_SAMPLE_CLK:
	case CGU_EMMC_DRV_CLK:
	case CGU_QSPI_CLK:
	case CGU_I2S_MCLK:
	case CGU_I2S_MST0_SCLK:
	case CGU_I2S_MST1_SCLK:
	case CGU_USB_PHY0_CLK:
	case CGU_AUDIO_300M:
	case CGU_AUDIO_ADC_CLK:
	case CGU_GMAC_CORE_CLK:
	case CGU_GMAC_PHY_CLK:
	case CGU_HDECOMP_CLK:
	case CGU_EFUSE_CLK:
	case CGU_SENSOR_MCLK0:
	case CGU_SENSOR_MCLK1:
	case CGU_SENSOR_MCLK2:
	case CGU_DVP_PATTERN_CLK:
	case CGU_DVP_SUB_1_2X_PIX_CLK:
	case CGU_NUC_CLK:
	case CGU_SCGMAC_PTP_CLK:
	case CGU_SCGMAC_RGMIITX_CLK:
	case CGU_SCGMAC_MDC_CLK:
	case CGU_NPU_CLK:
	case CGU_NPU_ACLK:
	case CGU_IFC_CLK:
	case CGU_EIS_CLK:
	case CGU_SCALER_CLK:
	case CGU_GEN_CLK:
	case CGU_SCAN_CLK_60M:
	case CGU_SCAN_CLK_100M:
	case CGU_SCAN_CLK_150M:
	case CGU_SCAN_CLK_300M:
	case CGU_SCAN_CLK_480M:
	case CGU_SCAN_CLK_2400M:
	case CGU_SCAN_CLK_I2S:
	case MIPI_TX_PLL_ATE_OUT_CLK:
	case AUDIO_PLL_ATE_OUT_CLK:
	case PIXEL_PLL_ATE_OUT_CLK:
	case FIX_PLL_ATE_OUT_CLK:
	case ADC_PLL_ATE_OUT_CLK:
	case CGU_CPU_CLK:
	case CGU_CS_DBG_CLK:
	default:
		break;
	}
#endif
	return ret;
}

static int __maybe_unused sca200v200_clk_set_parent(struct clk *clk,
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

static int sca200v200_clk_enable_byid(struct sca200v200_clock *sc_clk, u32 clk_id)
{
	struct sca200v200_cgu *cgu = sc_clk->cgu;
	struct sca200v200_a53 *a53 = sc_clk->a53;

	switch (clk_id) {
	case CGU_AXI_CLK:
		sca200v200_clk_enable_with_switch(&cgu->cgu_clk_ctrl00);
		break;
	case CGU_SYS_CLK:
		sca200v200_clk_enable_with_switch(&cgu->cgu_clk_ctrl01);
		break;
	case CGU_ISP_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl03);
		break;
	case CGU_ISP_HDR_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl03);
		break;
	case CGU_VIF_AXI_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl04);
		break;
	case CGU_DLA_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl04);
		break;
	case CGU_RSZ_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl05);
		break;
	case CGU_DE_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl05);
		break;
	case CGU_VENC_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl06);
		break;
	case CGU_JPEG_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl06);
		break;
	case CGU_MIPI_CSI_0_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl07);
		break;
	case CGU_MIPI_CSI_1_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl07);
		break;
	case CGU_MIPI_PCS_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl08);
		break;
	case CGU_SD0_FIX_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl08);
		break;
	case CGU_SD0_SAMPLE_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl09);
		break;
	case CGU_SD0_DRV_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl09);
		break;
	case CGU_SD1_FIX_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl10);
		break;
	case CGU_SD1_SAMPLE_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl10);
		break;
	case CGU_SD1_DRV_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl11);
		break;
	case CGU_EMMC_FIX_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl11);
		break;
	case CGU_EMMC_SAMPLE_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl12);
		break;
	case CGU_EMMC_DRV_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl12);
		break;
	case CGU_QSPI_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl13);
		break;
	case CGU_I2S_MCLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl13);
		break;
	case CGU_I2S_MST0_SCLK:
		cgu->cgu_i2s_clk_ctrl |= (1 << 15);
		break;
	case CGU_I2S_MST1_SCLK:
		cgu->cgu_i2s_clk_ctrl |= (1 << 31);
		break;
	case CGU_USB_PHY0_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl14);
		break;
	case CGU_AUDIO_300M:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl14);
		break;
	case CGU_AUDIO_ADC_CLK:
		break;
	case CGU_GMAC_CORE_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl15);
		break;
	case CGU_GMAC_PHY_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl15);
		break;
	case CGU_HDECOMP_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl16);
		break;
	case CGU_EFUSE_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl16);
		break;
	case CGU_SENSOR_MCLK0:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl17);
		break;
	case CGU_SENSOR_MCLK1:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl17);
		break;
	case CGU_SENSOR_MCLK2:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl18);
		break;
	case CGU_DVP_PATTERN_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl18);
		break;
	case CGU_DVP_SUB_1_2X_PIX_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl19);
		break;
	case CGU_NUC_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl19);
		break;
	case CGU_SCGMAC_PTP_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl20);
		break;
	case CGU_SCGMAC_RGMIITX_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl20);
		break;
	case CGU_SCGMAC_MDC_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl21);
		break;
	case CGU_NPU_CLK:
		sca200v200_clk_enable_with_switch(&cgu->cgu_clk_ctrl02);
		break;
	case CGU_NPU_ACLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl22);
		break;
	case CGU_IFC_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl22);
		break;
	case CGU_EIS_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_clk_ctrl23);
		break;
	case CGU_SCALER_CLK:
		sca200v200_clk_enable_no_switch_h(&cgu->cgu_clk_ctrl23);
		break;
	case CGU_GEN_CLK:
		sca200v200_clk_enable_no_switch_l(&cgu->cgu_gen_clk_ctrl);
		break;
	case CGU_CPU_CLK:
		break;
	case CGU_CS_DBG_CLK:
		sca200v200_clk_enable_no_switch_l(&a53->a53_config_reg4);
		break;
	default:
		debug("%s: unsupported clk %d\n", __func__, clk_id);
		return -ENOENT;
	}

	return 0;
}

static int sca200v200_clk_enable(struct clk *clk)
{
	struct sca200v200_clock *priv = dev_get_priv(clk->dev);

	return sca200v200_clk_enable_byid(priv, clk->id);
}

static int sca200v200_clk_disable_byid(struct sca200v200_clock *sc_clk, u32 clk_id)
{
	struct sca200v200_cgu *cgu = sc_clk->cgu;
	struct sca200v200_a53 *a53 = sc_clk->a53;

	switch (clk_id) {
	case CGU_AXI_CLK:
		sca200v200_clk_disable_with_switch(&cgu->cgu_clk_ctrl00);
		break;
	case CGU_SYS_CLK:
		sca200v200_clk_disable_with_switch(&cgu->cgu_clk_ctrl01);
		break;
	case CGU_ISP_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl03);
		break;
	case CGU_ISP_HDR_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl03);
		break;
	case CGU_VIF_AXI_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl04);
		break;
	case CGU_DLA_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl04);
		break;
	case CGU_RSZ_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl05);
		break;
	case CGU_DE_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl05);
		break;
	case CGU_VENC_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl06);
		break;
	case CGU_JPEG_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl06);
		break;
	case CGU_MIPI_CSI_0_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl07);
		break;
	case CGU_MIPI_CSI_1_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl07);
		break;
	case CGU_MIPI_PCS_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl08);
		break;
	case CGU_SD0_FIX_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl08);
		break;
	case CGU_SD0_SAMPLE_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl09);
		break;
	case CGU_SD0_DRV_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl09);
		break;
	case CGU_SD1_FIX_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl10);
		break;
	case CGU_SD1_SAMPLE_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl10);
		break;
	case CGU_SD1_DRV_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl11);
		break;
	case CGU_EMMC_FIX_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl11);
		break;
	case CGU_EMMC_SAMPLE_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl12);
		break;
	case CGU_EMMC_DRV_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl12);
		break;
	case CGU_QSPI_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl13);
		break;
	case CGU_I2S_MCLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl13);
		break;
	case CGU_I2S_MST0_SCLK:
		cgu->cgu_i2s_clk_ctrl |= (1 << 15);
		break;
	case CGU_I2S_MST1_SCLK:
		cgu->cgu_i2s_clk_ctrl |= (1 << 31);
		break;
	case CGU_USB_PHY0_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl14);
		break;
	case CGU_AUDIO_300M:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl14);
		break;
	case CGU_AUDIO_ADC_CLK:
		break;
	case CGU_GMAC_CORE_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl15);
		break;
	case CGU_GMAC_PHY_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl15);
		break;
	case CGU_HDECOMP_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl16);
		break;
	case CGU_EFUSE_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl16);
		break;
	case CGU_SENSOR_MCLK0:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl17);
		break;
	case CGU_SENSOR_MCLK1:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl17);
		break;
	case CGU_SENSOR_MCLK2:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl18);
		break;
	case CGU_DVP_PATTERN_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl18);
		break;
	case CGU_DVP_SUB_1_2X_PIX_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl19);
		break;
	case CGU_NUC_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl19);
		break;
	case CGU_SCGMAC_PTP_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl20);
		break;
	case CGU_SCGMAC_RGMIITX_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl20);
		break;
	case CGU_SCGMAC_MDC_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl21);
		break;
	case CGU_NPU_CLK:
		sca200v200_clk_disable_with_switch(&cgu->cgu_clk_ctrl02);
		break;
	case CGU_NPU_ACLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl22);
		break;
	case CGU_IFC_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl22);
		break;
	case CGU_EIS_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_clk_ctrl23);
		break;
	case CGU_SCALER_CLK:
		sca200v200_clk_disable_no_switch_h(&cgu->cgu_clk_ctrl23);
		break;
	case CGU_GEN_CLK:
		sca200v200_clk_disable_no_switch_l(&cgu->cgu_gen_clk_ctrl);
		break;
	case CGU_CPU_CLK:
		break;
	case CGU_CS_DBG_CLK:
		sca200v200_clk_disable_no_switch_l(&a53->a53_config_reg4);
		break;
	default:
		debug("%s: unsupported clk %d\n", __func__, clk_id);
		return -ENOENT;
	}

	return 0;
}

static int sca200v200_clk_disable(struct clk *clk)
{
	struct sca200v200_clock *priv = dev_get_priv(clk->dev);

	return sca200v200_clk_disable_byid(priv, clk->id);
}

static struct clk_ops sca200v200_clk_ops = {
	.get_rate = sca200v200_clk_get_rate,
	.set_rate = sca200v200_clk_set_rate,
#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
	.set_parent = sca200v200_clk_set_parent,
#endif
	.enable = sca200v200_clk_enable,
	.disable = sca200v200_clk_disable,
};

static int sca200v200_abb_init(struct udevice *dev)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	int ret;

	ret = dev_read_u32_array(dev, "abb-freqs",
	        (u32 *)&priv->abb_freqs,
	        sizeof(struct sca200v200_abb_freqs) / sizeof(u32));
	if (ret) {
		log_err("%s: read abb-freqs failed %d\n", __func__, ret);
		return -EINVAL;
	}
	sca200v200_abb_set_pixel(priv, priv->abb_freqs.abb_pixel_freq);
	sca200v200_abb_set_pixel1(priv, priv->abb_freqs.abb_pixel1_freq);
	sca200v200_abb_set_pixel2(priv, priv->abb_freqs.abb_pixel2_freq);
	sca200v200_abb_set_pixel3(priv, priv->abb_freqs.abb_pixel3_freq);
	sca200v200_abb_set_pixel4(priv, priv->abb_freqs.abb_pixel4_freq);
	sca200v200_abb_set_audio(priv, priv->abb_freqs.abb_audio_freq);

	return 0;
}

static int sca200v200_cgu_init(struct udevice *dev)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	u32 array[CGU_CLK_NUM], size;
	u32 i;
	u32 clk_id, enable, sel, div;
	int ret;

	size = dev_read_size(dev, "clk-default-config");
	if (size < 0) {
		log_err("%s: sca200v200 clk-default-config size %d error\n",
		    __func__, size);
		return -EINVAL;
	}

	ret = dev_read_u32_array(dev, "clk-default-config", (u32 *)array,
	        size / sizeof(u32));
	if (ret) {
		log_err("%s: read clk-default-config failed %d\n", __func__,
		    ret);
		return -EINVAL;
	}

	for (i = 0; i < size / sizeof(u32); i++) {
		clk_id = (array[i] & CLK_ID_MASK);
		enable = (array[i] & CLK_GATE_MASK) >> CLK_GATE_SHIFT;
		sel = (array[i] & CLK_SEL_MASK) >> CLK_SEL_SHIFT;
		div = (array[i] & CLK_DIV_MASK) >> CLK_DIV_SHIFT;

		sca200v200_clk_set_sel_div(priv, clk_id, sel, div);
		if (enable)
			sca200v200_clk_enable_byid(priv, clk_id);
		else
			sca200v200_clk_disable_byid(priv, clk_id);
	}

	return 0;
}

static void sca200v200_pwrctrl_a53(struct udevice *dev, u32 on)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	struct sca200v200_sys_ctrl *sys_ctrl = priv->sys_ctrl;
	struct sca200v200_rgu *rgu = priv->rgu;
	struct sca200v200_a53 *a53 = priv->a53;

	sys_ctrl->pmu_ctrl = 0xacce55;

	// core 1 low power begin
	if (on) {
		//memset((void *)(long)SMARTCHIP_SRAM_BASE, 0, 64);
		/* release cpu1 reset */
		a53->a53_config_reg3 |= (1 << 1 | 1 << 5);

		// power up
		sys_ctrl->ip_pmu_ctrl |= (1 << 9);      // power on pwr_on 2
		sys_ctrl->ip_pmu_ctrl |= (1 << 1);      // power on pwr_on 1a
		udelay(20);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 17);        // calmp on iso_en = 0

		rgu->reset2_level |= (1 << 1 | 1 << 5);     // rst high
	} else {
		// power down
		rgu->reset2_level &= ~(1 << 1 | 1 << 5);    // rst low

		sys_ctrl->ip_pmu_ctrl |= (1 << 17);     // clamp off iso = 1
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 1);     // power down pwr_on_1
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 9);     // power down pwr_on 2
	}
}

static void sca200v200_pwrctrl_dla(struct udevice *dev, u32 on)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	struct sca200v200_sys_ctrl *sys_ctrl = priv->sys_ctrl;
	struct sca200v200_rgu *rgu = priv->rgu;

	sys_ctrl->pmu_ctrl = 0xacce55;

	if (on) {
		// powen up
		sys_ctrl->ip_pmu_ctrl |= (1 << 13);
		sys_ctrl->ip_pmu_ctrl |= (1 << 5);
		udelay(20);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 21);

		rgu->reset1_level |= (1 << 13);
		rgu->reset0_level |= (1 << 27);

		// clock cgu_dla_clk on
		sca200v200_clk_enable_byid(priv, CGU_DLA_CLK);
	} else {
		// power down
		rgu->reset0_level &= ~(1 << 27);
		rgu->reset1_level &= ~(1 << 13);

		sys_ctrl->ip_pmu_ctrl |= (1 << 21);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 5);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 13);

		// clock cgu_dla_clk off
		sca200v200_clk_disable_byid(priv, CGU_DLA_CLK);
	}
}

static void sca200v200_pwrctrl_npu(struct udevice *dev, u32 on)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	struct sca200v200_sys_ctrl *sys_ctrl = priv->sys_ctrl;
	struct sca200v200_rgu *rgu = priv->rgu;

	sys_ctrl->pmu_ctrl = 0xacce55;

	if (on) {
		// powen up
		sys_ctrl->ip_pmu_ctrl |= (1 << 12);
		sys_ctrl->ip_pmu_ctrl |= (1 << 4);
		udelay(20);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 20);

		// powen up
		sys_ctrl->ip_pmu_ctrl |= (1 << 15);
		sys_ctrl->ip_pmu_ctrl |= (1 << 7);
		udelay(20);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 23);

		rgu->reset1_level |= (1 << 27 | 1 << 28);

		// cgu_npu_aclk on
		sca200v200_clk_enable_byid(priv, CGU_NPU_ACLK);

		rgu->reset1_level |= (1 << 27 | 1 << 28 | 1 << 29);
	} else {
		// power down
		rgu->reset1_level &= ~(1 << 27 | 1 << 28);

		sys_ctrl->ip_pmu_ctrl |= (1 << 23);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 7);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 15);

		// cgu_npu_aclk off
		sca200v200_clk_disable_byid(priv, CGU_NPU_ACLK);

		rgu->reset1_level &= ~(1 << 27 | 1 << 28 | 1 << 29);

		sys_ctrl->ip_pmu_ctrl |= (1 << 20);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 4);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 12);
	}
}

static void sca200v200_pwrctrl_rne(struct udevice *dev, u32 on)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	struct sca200v200_sys_ctrl *sys_ctrl = priv->sys_ctrl;
	struct sca200v200_rgu *rgu = priv->rgu;
	struct sca200v200_cgu *cgu = priv->cgu;

	sys_ctrl->pmu_ctrl = 0xacce55;

	if (on) {
		// powen up
		sys_ctrl->ip_pmu_ctrl |= (1 << 15);
		sys_ctrl->ip_pmu_ctrl |= (1 << 7);
		udelay(20);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 23);

		rgu->reset1_level |= (1 << 27 | 1 << 28);

		// npu_hclk off
		cgu->cgu_clk_en_00 |= (1 << 26);
		// cgu_npu_aclk on
		sca200v200_clk_enable_byid(priv, CGU_NPU_ACLK);
	} else {
		// power down
		rgu->reset1_level &= ~(1 << 27 | 1 << 28);

		sys_ctrl->ip_pmu_ctrl |= (1 << 23);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 7);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 15);

		// cgu_npu_aclk off
		sca200v200_clk_disable_byid(priv, CGU_NPU_ACLK);
		// npu_hclk off
		cgu->cgu_clk_en_00 &= ~(1 << 26);
	}

}

static void sca200v200_pwrctrl_venc(struct udevice *dev, u32 on)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	struct sca200v200_sys_ctrl *sys_ctrl = priv->sys_ctrl;
	struct sca200v200_rgu *rgu = priv->rgu;

	sys_ctrl->pmu_ctrl = 0xacce55;

	if (on) {
		// powen up
		sys_ctrl->ip_pmu_ctrl |= (1 << 14);
		sys_ctrl->ip_pmu_ctrl |= (1 << 6);
		udelay(20);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 22);

		rgu->reset0_level |= (1 << 2);
		rgu->reset1_level |= (1 << 26);
		// cgu_venc_clk on
		sca200v200_clk_enable_byid(priv, CGU_VENC_CLK);
	} else {
		// power down
		rgu->reset0_level &= ~(1 << 26);
		rgu->reset1_level &= ~(1 << 2);

		sys_ctrl->ip_pmu_ctrl |= (1 << 22);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 6);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 14);

		// cgu_venc_clk off
		sca200v200_clk_disable_byid(priv, CGU_VENC_CLK);
	}
}

static void sca200v200_pwrctrl_vision(struct udevice *dev, u32 on)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	struct sca200v200_sys_ctrl *sys_ctrl = priv->sys_ctrl;
	struct sca200v200_rgu *rgu = priv->rgu;
	struct sca200v200_cgu *cgu = priv->cgu;

	sys_ctrl->pmu_ctrl = 0xacce55;

	if (on) {
		// powen up
		sys_ctrl->ip_pmu_ctrl |= (1 << 11);
		sys_ctrl->ip_pmu_ctrl |= (1 << 3);
		udelay(20);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 19);

		rgu->reset0_level |= (1 << 13 | 1 << 14 | 1 << 15 | 1 << 16 | 1 << 17 | 1 << 18 | 1 << 19 | 1 << 30);
		rgu->reset1_level |= (1 << 4 | 1 << 6 | 1 << 17 | 1 << 18 | 1 << 20 |
		        1 << 21 | 1 << 22 | 1 << 23 | 1 << 24 | 1 << 30 | 1 << 31);

		sys_ctrl->rsz_ctrl |= 0xf00;
		// open clock
		sca200v200_clk_enable_byid(priv, CGU_MIPI_CSI_0_CLK);
		sca200v200_clk_enable_byid(priv, CGU_MIPI_CSI_1_CLK);
		sca200v200_clk_enable_byid(priv, CGU_MIPI_PCS_CLK);
		sca200v200_clk_enable_byid(priv, CGU_VIF_AXI_CLK);
		sca200v200_clk_enable_byid(priv, CGU_ISP_CLK);
		sca200v200_clk_enable_byid(priv, CGU_ISP_HDR_CLK);
		sca200v200_clk_enable_byid(priv, CGU_DVP_PATTERN_CLK);
		sca200v200_clk_enable_byid(priv, CGU_DVP_SUB_1_2X_PIX_CLK);
		sca200v200_clk_enable_byid(priv, CGU_NUC_CLK);
		sca200v200_clk_enable_byid(priv, CGU_JPEG_CLK);
		sca200v200_clk_enable_byid(priv, CGU_RSZ_CLK);
		sca200v200_clk_enable_byid(priv, CGU_IFC_CLK);
		sca200v200_clk_enable_byid(priv, CGU_EIS_CLK);
		sca200v200_clk_enable_byid(priv, CGU_SCALER_CLK);
		sca200v200_clk_enable_byid(priv, CGU_DE_CLK);
		cgu->cgu_clk_en_00 |= (1 << 1);
		cgu->cgu_clk_en_03 |= (1 << 1 | 1 << 5);

		rgu->reset1_level |= (1 << 14 | 1 << 19);
	} else {
		// power down
		rgu->reset0_level &= ~(1 << 13 | 1 << 14 | 1 << 15 | 1 << 16 | 1 << 17 | 1 << 18 | 1 << 19 | 1 << 30);
		rgu->reset1_level &= ~(1 << 4 | 1 << 6 | 1 << 14 | 1 << 17 | 1 << 18 | 1 << 19 | 1 << 20 |
		        1 << 21 | 1 << 22 | 1 << 23 | 1 << 24 | 1 << 30 | 1 << 31);

		sys_ctrl->ip_pmu_ctrl |= (1 << 19);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 3);
		sys_ctrl->ip_pmu_ctrl &= ~(1 << 11);

		// close clock
		sca200v200_clk_disable_byid(priv, CGU_MIPI_CSI_0_CLK);
		sca200v200_clk_disable_byid(priv, CGU_MIPI_CSI_1_CLK);
		sca200v200_clk_disable_byid(priv, CGU_MIPI_PCS_CLK);
		sca200v200_clk_disable_byid(priv, CGU_VIF_AXI_CLK);
		sca200v200_clk_disable_byid(priv, CGU_ISP_CLK);
		sca200v200_clk_disable_byid(priv, CGU_ISP_HDR_CLK);
		sca200v200_clk_disable_byid(priv, CGU_DVP_PATTERN_CLK);
		sca200v200_clk_disable_byid(priv, CGU_DVP_SUB_1_2X_PIX_CLK);
		sca200v200_clk_disable_byid(priv, CGU_NUC_CLK);
		sca200v200_clk_disable_byid(priv, CGU_JPEG_CLK);
		sca200v200_clk_disable_byid(priv, CGU_RSZ_CLK);
		sca200v200_clk_disable_byid(priv, CGU_IFC_CLK);
		sca200v200_clk_disable_byid(priv, CGU_EIS_CLK);
		sca200v200_clk_disable_byid(priv, CGU_SCALER_CLK);
		sca200v200_clk_disable_byid(priv, CGU_DE_CLK);
		cgu->cgu_clk_en_03 &= ~(1 << 1 | 1 << 5);
		cgu->cgu_clk_en_00 &= ~(1 << 1);
	}
}

static void sca200v200_power_on(struct udevice *dev)
{
	struct sca200v200_clock *priv = dev_get_priv(dev);
	struct sca200v200_cgu *cgu = priv->cgu;
	u32 array[PWR_NUM], size;
	u32 i;
	u32 pwr_id, on;
	int ret;

	size = dev_read_size(dev, "power-default-config");
	if (size < 0) {
		log_err("%s: sca200v200 pwr-default-config size %d error\n",
		    __func__, size);
		return;
	}

	ret = dev_read_u32_array(dev, "power-default-config", (u32 *)array,
	        size / sizeof(u32));
	if (ret) {
		log_err("%s: read clk-default-config failed %d\n", __func__,
		    ret);
		return;
	}

	for (i = 0; i < size / sizeof(u32); i++) {
		pwr_id = (array[i] & PWR_ID_MASK);
		on = (array[i] & PWR_GATE_MASK);
		switch (pwr_id) {
		case PWR_CORE1:
			sca200v200_pwrctrl_a53(dev, 0);
			sca200v200_pwrctrl_a53(dev, on);
			break;
		case PWR_VISION:
			sca200v200_pwrctrl_vision(dev, 0);
			sca200v200_pwrctrl_vision(dev, on);
			break;
		case PWR_NPU_CORE:
			sca200v200_pwrctrl_rne(dev, 0);
			sca200v200_pwrctrl_rne(dev, on);
			break;
		case PWR_NPU_TOP:
			sca200v200_pwrctrl_npu(dev, 0);
			sca200v200_pwrctrl_npu(dev, on);
			break;
		case PWR_DLA:
			sca200v200_pwrctrl_dla(dev, 0);
			sca200v200_pwrctrl_dla(dev, on);
			break;
		case PWR_VENC:
			sca200v200_pwrctrl_venc(dev, 0);
			sca200v200_pwrctrl_venc(dev, on);
			break;
		default:
			break;
		}
	}

	/* power off gmacsc */
	cgu->cgu_clk_ctrl20 &= ~(1 << 12 | 1 << 28);
	cgu->cgu_clk_ctrl21 &= ~(1 << 12);
	cgu->cgu_clk_en_00 &= ~(1 << 25);
}

static int sca200v200_clk_probe(struct udevice *dev)
{
	return 0;
}

static int sca200v200_clk_ofdata_to_platdata(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	struct sca200v200_clock_plat *plat = dev_get_platdata(dev);
	struct sca200v200_clock *priv = dev_get_priv(dev);
	int ret;

	ret = regmap_init_mem(dev_ofnode(dev), &plat->map);
	if (ret)
		log_err("%s: regmap failed %d\n", __func__, ret);

	priv->a53 = regmap_get_range(plat->map, 0);
	priv->rgu = regmap_get_range(plat->map, 1);
	priv->cgu = regmap_get_range(plat->map, 2);
	priv->sys_ctrl = regmap_get_range(plat->map, 3);
	priv->abb = regmap_get_range(plat->map, 4);

	sca200v200_power_on(dev);
	sca200v200_abb_init(dev);
	sca200v200_cgu_init(dev);
#endif
	return 0;
}

static const struct udevice_id sca200v200_cgu_ids[] = {
	{ .compatible = "smartchip,sca200v200-cgu" },
	{ }
};

U_BOOT_DRIVER(clk_sca200v200) = {
	.name       = "sca200v200_cgu",
	.id     = UCLASS_CLK,
	.of_match   = sca200v200_cgu_ids,
	.priv_auto_alloc_size = sizeof(struct sca200v200_clock),
	.ofdata_to_platdata = sca200v200_clk_ofdata_to_platdata,
	.ops        = &sca200v200_clk_ops,
	.probe      = sca200v200_clk_probe,
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	.platdata_auto_alloc_size = sizeof(struct sca200v200_clock_plat),
#endif
};
