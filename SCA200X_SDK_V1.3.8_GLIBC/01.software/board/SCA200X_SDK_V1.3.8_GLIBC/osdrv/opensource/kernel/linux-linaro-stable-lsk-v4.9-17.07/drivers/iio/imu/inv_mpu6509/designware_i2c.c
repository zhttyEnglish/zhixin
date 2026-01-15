// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2009
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
 *
 * Copyright (C) 2022 SmartChipc, Inc.
 */

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include "designware_i2c.h"

/*
 * This assigned unique hex value is constant and is derived from the two ASCII
 * letters 'DW' followed by a 16-bit unsigned number
 */
#define DW_I2C_COMP_TYPE    0x44570140

static int dw_i2c_enable(struct i2c_regs *i2c_base, bool enable)
{
	u32 ena = enable ? IC_ENABLE_0B : 0;
	int timeout = 100;

	do {
		writel(ena, &i2c_base->ic_enable);
		if ((readl(&i2c_base->ic_enable_status) & IC_ENABLE_0B) == ena)
			return 0;
		/*
		 * Wait 10 times the signaling period of the highest I2C
		 * transfer supported by the driver (for 400KHz this is
		 * 25us) as described in the DesignWare I2C databook.
		 */
		udelay(25);
	} while (timeout--);
	printk("timeout in %sabling I2C adapter\n", enable ? "en" : "dis");

	return -ETIMEDOUT;
}

/* High and low times in different speed modes (in ns) */
enum {
	/* SDA Hold Time */
	DEFAULT_SDA_HOLD_TIME       = 300,
};

/**
 * calc_counts() - Convert a period to a number of IC clk cycles
 *
 * @ic_clk: Input clock in Hz
 * @period_ns: Period to represent, in ns
 * @return calculated count
 */
static uint calc_counts(uint ic_clk, uint period_ns)
{
	return DIV_ROUND_UP(ic_clk / 1000 * period_ns, NANO_TO_KILO);
}

/**
 * struct i2c_mode_info - Information about an I2C speed mode
 *
 * Each speed mode has its own characteristics. This struct holds these to aid
 * calculations in dw_i2c_calc_timing().
 *
 * @speed: Speed in Hz
 * @min_scl_lowtime_ns: Minimum value for SCL low period in ns
 * @min_scl_hightime_ns: Minimum value for SCL high period in ns
 * @def_rise_time_ns: Default rise time in ns
 * @def_fall_time_ns: Default fall time in ns
 */
struct i2c_mode_info {
	int speed;
	int min_scl_hightime_ns;
	int min_scl_lowtime_ns;
	int def_rise_time_ns;
	int def_fall_time_ns;
};

static const struct i2c_mode_info info_for_mode[] = {
	[IC_SPEED_MODE_STANDARD] = {
		I2C_SPEED_STANDARD_RATE,
		MIN_SS_SCL_HIGHTIME,
		MIN_SS_SCL_LOWTIME,
		1000,
		300,
	},
	[IC_SPEED_MODE_FAST] = {
		I2C_SPEED_FAST_RATE,
		MIN_FS_SCL_HIGHTIME,
		MIN_FS_SCL_LOWTIME,
		300,
		300,
	},
	[IC_SPEED_MODE_FAST_PLUS] = {
		I2C_SPEED_FAST_PLUS_RATE,
		MIN_FP_SCL_HIGHTIME,
		MIN_FP_SCL_LOWTIME,
		260,
		500,
	},
	[IC_SPEED_MODE_HIGH] = {
		I2C_SPEED_HIGH_RATE,
		MIN_HS_SCL_HIGHTIME,
		MIN_HS_SCL_LOWTIME,
		120,
		120,
	},
};

static struct __iomem i2c_regs *i2c2_reg_base;

/* Returns time in milliseconds */
static u64 get_timer(u64 base)
{
	return ktime_get_raw_ns() / 1000000 - base;
}

void dw_i2c_dump_regs(struct i2c_regs *i2c_base)
{
	u32 regval;
	u32  i, j;
	u32 *addr = (u32 *)i2c_base;
	char str[48] = {0};
	char str2[16] = {0};

	printk("%s", __func__);

	for(j = 0; j < 16; j++) {
		for(i = 0; i < 4; i++) {
			regval = readl(addr);
			sprintf(str2, "%8x ", regval);
			strcat(str, str2);
			addr++;
		}
		printk("%s\n", str);
		str[0] = 0;
	}
}

/**
 * dw_i2c_calc_timing() - Calculate the timings to use for a bus
 *
 * @priv: Bus private information (NULL if not using driver model)
 * @mode: Speed mode to use
 * @ic_clk: IC clock speed in Hz
 * @spk_cnt: Spike-suppression count
 * @config: Returns value to use
 * @return 0 if OK, -EINVAL if the calculation failed due to invalid data
 */
static int dw_i2c_calc_timing(struct dw_i2c *priv, enum i2c_speed_mode mode,
    int ic_clk, int spk_cnt,
    struct dw_i2c_speed_config *config)
{
	int fall_cnt, rise_cnt, min_tlow_cnt, min_thigh_cnt;
	int hcnt, lcnt, period_cnt, diff, tot;
	int sda_hold_time_ns, scl_rise_time_ns, scl_fall_time_ns;
	const struct i2c_mode_info *info;

	/*
	 * Find the period, rise, fall, min tlow, and min thigh in terms of
	 * counts of the IC clock
	 */
	info = &info_for_mode[mode];
	period_cnt = ic_clk / info->speed;
	scl_rise_time_ns = priv && priv->scl_rise_time_ns ?
	    priv->scl_rise_time_ns : info->def_rise_time_ns;
	scl_fall_time_ns = priv && priv->scl_fall_time_ns ?
	    priv->scl_fall_time_ns : info->def_fall_time_ns;
	rise_cnt = calc_counts(ic_clk, scl_rise_time_ns);
	fall_cnt = calc_counts(ic_clk, scl_fall_time_ns);
	min_tlow_cnt = calc_counts(ic_clk, info->min_scl_lowtime_ns);
	min_thigh_cnt = calc_counts(ic_clk, info->min_scl_hightime_ns);

	pr_debug("dw_i2c: mode %d, ic_clk %d, speed %d, period %d rise %d fall %d tlow %d thigh %d spk %d\n",
	    mode, ic_clk, info->speed, period_cnt, rise_cnt, fall_cnt,
	    min_tlow_cnt, min_thigh_cnt, spk_cnt);

	/*
	 * Back-solve for hcnt and lcnt according to the following equations:
	 * SCL_High_time = [(HCNT + IC_*_SPKLEN + 7) * ic_clk] + SCL_Fall_time
	 * SCL_Low_time = [(LCNT + 1) * ic_clk] - SCL_Fall_time + SCL_Rise_time
	 */
	hcnt = min_thigh_cnt - fall_cnt - 7 - spk_cnt;
	lcnt = min_tlow_cnt - rise_cnt + fall_cnt - 1;

	if (hcnt < 0 || lcnt < 0) {
		pr_debug("dw_i2c: bad counts. hcnt = %d lcnt = %d\n", hcnt, lcnt);
		return -EINVAL;
	}

	/*
	 * Now add things back up to ensure the period is hit. If it is off,
	 * split the difference and bias to lcnt for remainder
	 */
	tot = hcnt + lcnt + 7 + spk_cnt + rise_cnt + 1;

	if (tot < period_cnt) {
		diff = (period_cnt - tot) / 2;
		hcnt += diff;
		lcnt += diff;
		tot = hcnt + lcnt + 7 + spk_cnt + rise_cnt + 1;
		lcnt += period_cnt - tot;
	}

	config->scl_lcnt = lcnt;
	config->scl_hcnt = hcnt;

	/* Use internal default unless other value is specified */
	sda_hold_time_ns = priv && priv->sda_hold_time_ns ?
	    priv->sda_hold_time_ns : DEFAULT_SDA_HOLD_TIME;
	config->sda_hold = calc_counts(ic_clk, sda_hold_time_ns);

	pr_debug("dw_i2c: hcnt = %d lcnt = %d sda hold = %d\n", hcnt, lcnt,
	    config->sda_hold);

	return 0;
}

/**
 * calc_bus_speed() - Calculate the config to use for a particular i2c speed
 *
 * @priv: Private information for the driver (NULL if not using driver model)
 * @i2c_base: Registers for the I2C controller
 * @speed: Required i2c speed in Hz
 * @bus_clk: Input clock to the I2C controller in Hz (e.g. IC_CLK)
 * @config: Returns the config to use for this speed
 * @return 0 if OK, -ve on error
 */
static int calc_bus_speed(struct dw_i2c *priv, struct i2c_regs *regs, int speed,
    ulong bus_clk, struct dw_i2c_speed_config *config)
{
	const struct dw_scl_sda_cfg *scl_sda_cfg = NULL;
	enum i2c_speed_mode i2c_spd;
	int spk_cnt;
	int ret;

	if (priv)
		scl_sda_cfg = priv->scl_sda_cfg;
	/* Allow high speed if there is no config, or the config allows it */
	if (speed >= I2C_SPEED_HIGH_RATE)
		i2c_spd = IC_SPEED_MODE_HIGH;
	else if (speed >= I2C_SPEED_FAST_PLUS_RATE)
		i2c_spd = IC_SPEED_MODE_FAST_PLUS;
	else if (speed >= I2C_SPEED_FAST_RATE)
		i2c_spd = IC_SPEED_MODE_FAST;
	else
		i2c_spd = IC_SPEED_MODE_STANDARD;

	/* Check is high speed possible and fall back to fast mode if not */
	if (i2c_spd == IC_SPEED_MODE_HIGH) {
		u32 comp_param1;

		comp_param1 = readl(&regs->comp_param1);
		if ((comp_param1 & DW_IC_COMP_PARAM_1_SPEED_MODE_MASK)
		    != DW_IC_COMP_PARAM_1_SPEED_MODE_HIGH)
			i2c_spd = IC_SPEED_MODE_FAST;
	}

	/* Get the proper spike-suppression count based on target speed */
	if (!priv || !priv->has_spk_cnt)
		spk_cnt = 0;
	else if (i2c_spd >= IC_SPEED_MODE_HIGH)
		spk_cnt = readl(&regs->hs_spklen);
	else
		spk_cnt = readl(&regs->fs_spklen);
	if (scl_sda_cfg) {
		config->sda_hold = scl_sda_cfg->sda_hold;
		if (i2c_spd == IC_SPEED_MODE_STANDARD) {
			config->scl_hcnt = scl_sda_cfg->ss_hcnt;
			config->scl_lcnt = scl_sda_cfg->ss_lcnt;
		} else if (i2c_spd == IC_SPEED_MODE_HIGH) {
			config->scl_hcnt = scl_sda_cfg->hs_hcnt;
			config->scl_lcnt = scl_sda_cfg->hs_lcnt;
		} else {
			config->scl_hcnt = scl_sda_cfg->fs_hcnt;
			config->scl_lcnt = scl_sda_cfg->fs_lcnt;
		}
	} else {
		ret = dw_i2c_calc_timing(priv, i2c_spd, bus_clk, spk_cnt,
		        config);
		if (ret)
			return ret;
	}
	config->speed_mode = i2c_spd;

	return 0;
}

/**
 * _dw_i2c_set_bus_speed() - Set the i2c speed
 *
 * @priv: Private information for the driver (NULL if not using driver model)
 * @i2c_base: Registers for the I2C controller
 * @speed: Required i2c speed in Hz
 * @bus_clk: Input clock to the I2C controller in Hz (e.g. IC_CLK)
 * @return 0 if OK, -ve on error
 */
int _dw_i2c_set_bus_speed(struct dw_i2c *priv, struct i2c_regs *i2c_base,
    unsigned int speed, unsigned int bus_clk)
{
	struct dw_i2c_speed_config config;
	unsigned int cntl;
	unsigned int ena;
	int ret;

	ret = calc_bus_speed(priv, i2c_base, speed, bus_clk, &config);
	if (ret)
		return ret;

	/* Get enable setting for restore later */
	ena = readl(&i2c_base->ic_enable) & IC_ENABLE_0B;

	/* to set speed cltr must be disabled */
	dw_i2c_enable(i2c_base, false);

	cntl = (readl(&i2c_base->ic_con) & (~IC_CON_SPD_MSK));

	switch (config.speed_mode) {
	case IC_SPEED_MODE_HIGH:
		cntl |= IC_CON_SPD_HS;
		writel(config.scl_hcnt, &i2c_base->ic_hs_scl_hcnt);
		writel(config.scl_lcnt, &i2c_base->ic_hs_scl_lcnt);
		break;
	case IC_SPEED_MODE_STANDARD:
		cntl |= IC_CON_SPD_SS;
		writel(config.scl_hcnt, &i2c_base->ic_ss_scl_hcnt);
		writel(config.scl_lcnt, &i2c_base->ic_ss_scl_lcnt);
		break;
	case IC_SPEED_MODE_FAST_PLUS:
	case IC_SPEED_MODE_FAST:
	default:
		cntl |= IC_CON_SPD_FS;
		writel(config.scl_hcnt, &i2c_base->ic_fs_scl_hcnt);
		writel(config.scl_lcnt, &i2c_base->ic_fs_scl_lcnt);
		break;
	}

	writel(cntl, &i2c_base->ic_con);

	/* Configure SDA Hold Time if required */
	if (config.sda_hold)
		writel(config.sda_hold, &i2c_base->ic_sda_hold);

	/* Restore back i2c now speed set */
	if (ena == IC_ENABLE_0B)
		dw_i2c_enable(i2c_base, true);
	if (priv)
		priv->config = config;

	return 0;
}

int dw_i2c_gen_speed_config(struct dw_i2c *priv, int speed_hz,
    struct dw_i2c_speed_config *config)
{
	ulong rate;
	int ret;

#if CONFIG_COMMON_CLK
	rate = clk_get_rate(priv->clk);
	if (IS_ERR_VALUE(rate))
		return -EINVAL;
#else
	rate = IC_CLK;
#endif

	ret = calc_bus_speed(priv, priv->regs, speed_hz, rate, config);
	if (ret)
		printk("%s: ret=%d\n", __func__, ret);
	if (ret)
		return ret;

	return 0;
}

/*
 * i2c_setaddress - Sets the target slave address
 * @i2c_addr:   target i2c address
 *
 * Sets the target slave address.
 */
static void i2c_setaddress(struct i2c_regs *i2c_base, unsigned int i2c_addr)
{
	/* Disable i2c */
	dw_i2c_enable(i2c_base, false);

	writel(i2c_addr, &i2c_base->ic_tar);

	/* Enable i2c */
	dw_i2c_enable(i2c_base, true);
}

/*
 * i2c_flush_rxfifo - Flushes the i2c RX FIFO
 *
 * Flushes the i2c RX FIFO
 */
static void i2c_flush_rxfifo(struct i2c_regs *i2c_base)
{
	while (readl(&i2c_base->ic_status) & IC_STATUS_RFNE)
		readl(&i2c_base->ic_cmd_data);
}

/*
 * i2c_wait_for_bb - Waits for bus busy
 *
 * Waits for bus busy
 */
static int i2c_wait_for_bb(struct i2c_regs *i2c_base)
{
	u64 start_time_bb = get_timer(0);

	while ((readl(&i2c_base->ic_status) & IC_STATUS_MA) ||
	    !(readl(&i2c_base->ic_status) & IC_STATUS_TFE)) {
		/* Evaluate timeout */
		if (get_timer(start_time_bb) > (u64)(I2C_BYTE_TO_BB))
			return 1;
	}

	return 0;
}

static int i2c_xfer_init(struct i2c_regs *i2c_base, u8 chip, uint addr,
    int alen)
{
	if (i2c_wait_for_bb(i2c_base))
		return 1;

	i2c_setaddress(i2c_base, chip);
	while (alen) {
		alen--;
		/* high byte address going out first */
		writel((addr >> (alen * 8)) & 0xff,
		    &i2c_base->ic_cmd_data);
	}
	return 0;
}

static int i2c_xfer_finish(struct i2c_regs *i2c_base)
{
	u64 start_stop_det = get_timer(0);

	while (1) {
		if ((readl(&i2c_base->ic_raw_intr_stat) & IC_STOP_DET)) {
			readl(&i2c_base->ic_clr_stop_det);
			break;
		} else if (get_timer(start_stop_det) > I2C_STOPDET_TO) {
			break;
		}
	}

	if (i2c_wait_for_bb(i2c_base)) {
		printk("Timed out waiting for bus\n");
		return 1;
	}

	i2c_flush_rxfifo(i2c_base);

	return 0;
}

/*
 * i2c_read - Read from i2c memory
 * @chip:   target i2c address
 * @addr:   address to read from
 * @alen:
 * @buffer: buffer for read data
 * @len:    no of bytes to be read
 *
 * Read from i2c memory.
 */
static int __dw_i2c_read(struct i2c_regs *i2c_base, u8 dev, uint addr,
    int alen, u8 *buffer, int len)
{
	u64 start_time_rx;
	unsigned int active = 0;

	if (i2c_xfer_init(i2c_base, dev, addr, alen))
		return 1;

	start_time_rx = get_timer(0);
	while (len) {
		if (!active) {
			/*
			 * Avoid writing to ic_cmd_data multiple times
			 * in case this loop spins too quickly and the
			 * ic_status RFNE bit isn't set after the first
			 * write. Subsequent writes to ic_cmd_data can
			 * trigger spurious i2c transfer.
			 */
			if (len == 1)
				writel(IC_CMD | IC_STOP, &i2c_base->ic_cmd_data);
			else
				writel(IC_CMD, &i2c_base->ic_cmd_data);
			active = 1;
		}

		if (readl(&i2c_base->ic_status) & IC_STATUS_RFNE) {
			*buffer++ = (u8)readl(&i2c_base->ic_cmd_data);
			len--;
			start_time_rx = get_timer(0);
			active = 0;
		} else if (get_timer(start_time_rx) > I2C_BYTE_TO) {
			return 1;
		}
	}

	return i2c_xfer_finish(i2c_base);
}

/*
 * i2c_write - Write to i2c memory
 * @chip:   target i2c address
 * @addr:   address to read from
 * @alen:
 * @buffer: buffer for read data
 * @len:    no of bytes to be read
 *
 * Write to i2c memory.
 */
static int __dw_i2c_write(struct i2c_regs *i2c_base, u8 dev, uint addr,
    int alen, u8 *buffer, int len)
{
	int nb = len;
	u64 start_time_tx;

	if (i2c_xfer_init(i2c_base, dev, addr, alen))
		return 1;

	start_time_tx = get_timer(0);
	while (len) {
		if (readl(&i2c_base->ic_status) & IC_STATUS_TFNF) {
			if (--len == 0) {
				writel(*buffer | IC_STOP,
				    &i2c_base->ic_cmd_data);
			} else {
				writel(*buffer, &i2c_base->ic_cmd_data);
			}
			buffer++;
			start_time_tx = get_timer(0);

		} else if (get_timer(start_time_tx) > (nb * I2C_BYTE_TO)) {
			printk("Timed out. i2c write Failed\n");
			return 1;
		}
	}

	return i2c_xfer_finish(i2c_base);
}

/*
 * __dw_i2c_init - Init function
 * @speed:  required i2c speed
 * @slaveaddr:  slave address for the device
 *
 * Initialization function.
 */
int __dw_i2c_init(struct i2c_regs *i2c_base, int speed, int slaveaddr)
{
	int ret;

	/* Disable i2c */
	ret = dw_i2c_enable(i2c_base, false);
	if (ret)
		return ret;

	writel(IC_CON_SD | IC_CON_RE | IC_CON_SPD_FS | IC_CON_MM,
	    &i2c_base->ic_con);
	writel(IC_RX_TL, &i2c_base->ic_rx_tl);
	writel(IC_TX_TL, &i2c_base->ic_tx_tl);
	writel(IC_STOP_DET, &i2c_base->ic_intr_mask);

	/* Enable i2c */
	ret = dw_i2c_enable(i2c_base, true);
	if (ret)
		return ret;

	return 0;
}

#define I2C_MAX_OFFSET_LEN  4

#define I2C2_REG_BASE      0x08240000
#define I2C2_REG_MAP_LEN    0x20000

static int designware_i2c_xfer(struct i2c_msg *msg, int nmsgs)
{
	int ret;

	pr_debug("i2c_xfer: %d messages\n", nmsgs);
	for (; nmsgs > 0; nmsgs--, msg++) {
		pr_debug("i2c_xfer: chip=0x%x, len=0x%x\n", msg->addr, msg->len);
		if (msg->flags & I2C_M_RD) {
			ret = __dw_i2c_read(i2c2_reg_base, msg->addr, 0, 0,
			        msg->buf, msg->len);
		} else {
			ret = __dw_i2c_write(i2c2_reg_base, msg->addr, 0, 0,
			        msg->buf, msg->len);
		}
		if (ret) {
			pr_debug("i2c_write: error sending\n");
			return -EREMOTEIO;
		}
	}

	return 0;
}

static int dw_i2c_setup_offset(uint offset,
    uint8_t offset_buf[], struct i2c_msg *msg)
{
	msg->addr = 0x68;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = offset_buf;

	*offset_buf++ = (uint8_t)(offset & 0xFF);

	return 0;
}

int dw_i2c_read(uint reg, u8 *buffer, int len)
{
	struct i2c_msg msg[2], *ptr;
	uint8_t offset_buf[I2C_MAX_OFFSET_LEN];
	int msg_count;

	ptr = msg;
	if (!dw_i2c_setup_offset(reg, offset_buf, ptr))
		ptr++;

	if (len) {
		ptr->addr = msg->addr;
		ptr->flags = I2C_M_RD;
		ptr->len = len;
		ptr->buf = buffer;
		ptr++;
	}
	msg_count = ptr - msg;

	return designware_i2c_xfer(msg, msg_count);
}

int dw_i2c_reg_read(unsigned int reg, unsigned int *val)
{
	return dw_i2c_read(reg, (u8 *)val, 1);
}
EXPORT_SYMBOL_GPL(dw_i2c_reg_read);

int dw_i2c_init(void)
{
	i2c2_reg_base = ioremap(I2C2_REG_BASE, I2C2_REG_MAP_LEN);
	pr_debug("i2c2_reg_base = %p", i2c2_reg_base);
	return 0;
}
EXPORT_SYMBOL_GPL(dw_i2c_init);

int dw_i2c_remove(void)
{
	iounmap(i2c2_reg_base);
	return 0;
}
EXPORT_SYMBOL_GPL(dw_i2c_remove);

