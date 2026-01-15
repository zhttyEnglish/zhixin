/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Header providing constants for SmartChip power domain bindings.
 *
 * Copyright (c) 2021 SmartChip
 */

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <power-domain-uclass.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/iopoll.h>

#include <dt-bindings/power/sca200v100-power.h>

/* PMU */
#define PWR_CTRL_IP_PMU_CTRL_VENC_M                 0
#define PWR_CTRL_IP_PMU_CTRL_VENC_D                 1
#define PWR_CTRL_IP_PMU_CTRL_VENC_ISO               2
#define PWR_CTRL_IP_PMU_CTRL_ISP_M                  3
#define PWR_CTRL_IP_PMU_CTRL_ISP_D                  4
#define PWR_CTRL_IP_PMU_CTRL_ISP_ISO                5
#define PWR_CTRL_IP_PMU_CTRL_DLA_M                  6
#define PWR_CTRL_IP_PMU_CTRL_DLA_D                  7
#define PWR_CTRL_IP_PMU_CTRL_DLA_ISO                8

#define PWR_CTRL_IP_PMU_ACK_VENC_M                  0
#define PWR_CTRL_IP_PMU_ACK_VENC_D                  1
#define PWR_CTRL_IP_PMU_ACK_ISP_M                   2
#define PWR_CTRL_IP_PMU_ACK_ISP_D                   3
#define PWR_CTRL_IP_PMU_ACK_DLA_M                   4
#define PWR_CTRL_IP_PMU_ACK_DLA_D                   5

#define PWR_CTRL_PMU_CTRL_ADDR_OFFSET               0x0
#define PWR_CTRL_IP_PMU_CTRL_ADDR_OFFSET            0x4
#define PWR_CTRL_IP_PMU_ACK_ADDR_OFFSET             0x8

#define PWR_CTRL_PMU_KEY_EN                         1
#define PWR_CTRL_PMU_KEY                            0xACCE55

/* A53 */
#define PWR_CTRL_A53_CTRL_CORE1_M                   0
#define PWR_CTRL_A53_CTRL_CORE2_M                   1
#define PWR_CTRL_A53_CTRL_CORE3_M                   2
#define PWR_CTRL_A53_CTRL_CORE1_ISO                 3
#define PWR_CTRL_A53_CTRL_CORE2_ISO                 4
#define PWR_CTRL_A53_CTRL_CORE3_ISO                 5
#define PWR_CTRL_A53_CTRL_CORE1_D                   7
#define PWR_CTRL_A53_CTRL_CORE2_D                   8
#define PWR_CTRL_A53_CTRL_CORE3_D                   9

#define PWR_CTRL_A53_ACK_CORE1_M                    14
#define PWR_CTRL_A53_ACK_CORE2_M                    15
#define PWR_CTRL_A53_ACK_CORE3_M                    16
#define PWR_CTRL_A53_ACK_CORE1_D                    18
#define PWR_CTRL_A53_ACK_CORE2_D                    19
#define PWR_CTRL_A53_ACK_CORE3_D                    20

#define PWR_CTRL_A53_CTRL_ADDR_OFFSET               0x30
#define PWR_CTRL_A53_ACK_ADDR_OFFSET                0x20

/* CEVA */
#define PWR_CTRL_CEVA_CTRL_CORE0_M                  1
#define PWR_CTRL_CEVA_CTRL_CORE0_D                  2
#define PWR_CTRL_CEVA_CTRL_CORE0_ISO                3
#define PWR_CTRL_CEVA_ACK_CORE0_M                   4
#define PWR_CTRL_CEVA_ACK_CORE0_D                   5

#define PWR_CTRL_CEVA_KEY_ADDR_OFFSET               0x60
#define PWR_CTRL_CEVA_CTRL_ADDR_OFFSET              0x58
#define PWR_CTRL_CEVA_ACK_ADDR_OFFSET               0x58

#define PWR_CTRL_CEVA_CORE0_KEY_EN                  1
#define PWR_CTRL_CEVA_CORE0_KEY                     0x4C43544D

/* general */
#define PWR_CTRL_POLL_TIMEOUT_INTERVAL              100
#define PWR_CTRL_POWEROFF_ACKED(_val, _bit)         ((_val & BIT(_bit)) == 0)
#define PWR_CTRL_POWERON_ACKED(_val, _bit)          ((_val & BIT(_bit)) == BIT(_bit))

#define PWR_CTRL_ID_MASK                            GENMASK(SCA200V100_PWR_CTRL_GATING_SHIFT-1, 0)
#define PWR_CTRL_STATUS_MASK                        GENMASK(31, SCA200V100_PWR_CTRL_GATING_SHIFT)

enum pwr_ctrl_reg_type {
	PWR_CTRL_REG_TYPE_PMU,
	PWR_CTRL_REG_TYPE_A53_CGU,
	PWR_CTRL_REG_TYPE_CEVA_CORE0,
	PWR_CTRL_REG_TYPE_MAX
};

enum pwr_ctrl_reg_offset_type {
	PWR_CTRL_REG_OFFSET_TYPE_KEY,
	PWR_CTRL_REG_OFFSET_TYPE_CTRL,
	PWR_CTRL_REG_OFFSET_TYPE_ACK,
	PWR_CTRL_REG_OFFSET_TYPE_MAX
};

enum pwr_ctrl_bit_offset_type {
	PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO,
	PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M,
	PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D,
	PWR_CTRL_BIT_OFFSET_TYPE_ACK_M,
	PWR_CTRL_BIT_OFFSET_TYPE_ACK_D,
	PWR_CTRL_BIT_OFFSET_TYPE_MAX
};

struct pwr_ctrl_plat {
	struct regmap *map;
};

struct pwr_ctrl_tab {
	u32 reg_type;
	u32 reg_offset[PWR_CTRL_REG_OFFSET_TYPE_MAX];
	u32 bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_MAX];
	u32 key_en;
	u32 key;
};

struct pwr_ctrl {
	void __iomem *base[PWR_CTRL_REG_TYPE_MAX];
	struct pwr_ctrl_tab *tab;
};

static struct pwr_ctrl_tab sc_pwr_ctrl_tab[SCA200V100_PWR_CTRL_MAX] = {
	[SCA200V100_PWR_CTRL_SOC_VENC] = {
		.reg_type = PWR_CTRL_REG_TYPE_PMU,
		.reg_offset = {
			[PWR_CTRL_REG_OFFSET_TYPE_KEY]  = PWR_CTRL_PMU_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_CTRL] = PWR_CTRL_IP_PMU_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_ACK]  = PWR_CTRL_IP_PMU_ACK_ADDR_OFFSET
		},
		.bit_offset = {
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]  = PWR_CTRL_IP_PMU_CTRL_VENC_ISO,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]    = PWR_CTRL_IP_PMU_CTRL_VENC_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]    = PWR_CTRL_IP_PMU_CTRL_VENC_D,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]    = PWR_CTRL_IP_PMU_ACK_VENC_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]    = PWR_CTRL_IP_PMU_ACK_VENC_D
		},
		.key_en = PWR_CTRL_PMU_KEY_EN,
		.key = PWR_CTRL_PMU_KEY
	},
	[SCA200V100_PWR_CTRL_SOC_ISP] = {
		.reg_type = PWR_CTRL_REG_TYPE_PMU,
		.reg_offset = {
			[PWR_CTRL_REG_OFFSET_TYPE_KEY] = PWR_CTRL_PMU_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_CTRL] = PWR_CTRL_IP_PMU_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_ACK] = PWR_CTRL_IP_PMU_ACK_ADDR_OFFSET
		},
		.bit_offset = {
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]  = PWR_CTRL_IP_PMU_CTRL_ISP_ISO,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]    = PWR_CTRL_IP_PMU_CTRL_ISP_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]    = PWR_CTRL_IP_PMU_CTRL_ISP_D,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]    = PWR_CTRL_IP_PMU_ACK_ISP_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]    = PWR_CTRL_IP_PMU_ACK_ISP_D
		},
		.key_en = PWR_CTRL_PMU_KEY_EN,
		.key = PWR_CTRL_PMU_KEY
	},
	[SCA200V100_PWR_CTRL_SOC_DLA] = {
		.reg_type = PWR_CTRL_REG_TYPE_PMU,
		.reg_offset = {
			[PWR_CTRL_REG_OFFSET_TYPE_KEY] = PWR_CTRL_PMU_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_CTRL] = PWR_CTRL_IP_PMU_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_ACK] = PWR_CTRL_IP_PMU_ACK_ADDR_OFFSET
		},
		.bit_offset = {
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]  = PWR_CTRL_IP_PMU_CTRL_DLA_ISO,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]    = PWR_CTRL_IP_PMU_CTRL_DLA_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]    = PWR_CTRL_IP_PMU_CTRL_DLA_D,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]    = PWR_CTRL_IP_PMU_ACK_DLA_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]    = PWR_CTRL_IP_PMU_ACK_DLA_D
		},
		.key_en = PWR_CTRL_PMU_KEY_EN,
		.key = PWR_CTRL_PMU_KEY
	},
	[SCA200V100_PWR_CTRL_A53_CORE1] = {
		.reg_type = PWR_CTRL_REG_TYPE_A53_CGU,
		.reg_offset = {
			[PWR_CTRL_REG_OFFSET_TYPE_CTRL] = PWR_CTRL_A53_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_ACK] = PWR_CTRL_A53_ACK_ADDR_OFFSET
		},
		.bit_offset = {
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]  = PWR_CTRL_A53_CTRL_CORE1_ISO,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]    = PWR_CTRL_A53_CTRL_CORE1_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]    = PWR_CTRL_A53_CTRL_CORE1_D,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]    = PWR_CTRL_A53_ACK_CORE1_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]    = PWR_CTRL_A53_ACK_CORE1_D
		},
		.key_en = 0,
		.key = 0
	},
	[SCA200V100_PWR_CTRL_A53_CORE2] = {
		.reg_type = PWR_CTRL_REG_TYPE_A53_CGU,
		.reg_offset = {
			[PWR_CTRL_REG_OFFSET_TYPE_CTRL] = PWR_CTRL_A53_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_ACK] = PWR_CTRL_A53_ACK_ADDR_OFFSET
		},
		.bit_offset = {
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]  = PWR_CTRL_A53_CTRL_CORE2_ISO,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]    = PWR_CTRL_A53_CTRL_CORE2_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]    = PWR_CTRL_A53_CTRL_CORE2_D,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]    = PWR_CTRL_A53_ACK_CORE2_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]    = PWR_CTRL_A53_ACK_CORE2_D
		},
		.key_en = 0,
		.key = 0
	},
	[SCA200V100_PWR_CTRL_A53_CORE3] = {
		.reg_type = PWR_CTRL_REG_TYPE_A53_CGU,
		.reg_offset = {
			[PWR_CTRL_REG_OFFSET_TYPE_CTRL] = PWR_CTRL_A53_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_ACK] = PWR_CTRL_A53_ACK_ADDR_OFFSET
		},
		.bit_offset = {
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]  = PWR_CTRL_A53_CTRL_CORE3_ISO,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]    = PWR_CTRL_A53_CTRL_CORE3_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]    = PWR_CTRL_A53_CTRL_CORE3_D,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]    = PWR_CTRL_A53_ACK_CORE3_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]    = PWR_CTRL_A53_ACK_CORE3_D
		},
		.key_en = 0,
		.key = 0
	},
	[SCA200V100_PWR_CTRL_CEVA_CORE0] = {
		.reg_type = PWR_CTRL_REG_TYPE_CEVA_CORE0,
		.reg_offset = {
			[PWR_CTRL_REG_OFFSET_TYPE_KEY] = PWR_CTRL_CEVA_KEY_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_CTRL] = PWR_CTRL_CEVA_CTRL_ADDR_OFFSET,
			[PWR_CTRL_REG_OFFSET_TYPE_ACK] = PWR_CTRL_CEVA_ACK_ADDR_OFFSET
		},
		.bit_offset = {
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]  = PWR_CTRL_CEVA_CTRL_CORE0_ISO,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]    = PWR_CTRL_CEVA_CTRL_CORE0_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]    = PWR_CTRL_CEVA_CTRL_CORE0_D,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]    = PWR_CTRL_CEVA_ACK_CORE0_M,
			[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]    = PWR_CTRL_CEVA_ACK_CORE0_D
		},
		.key_en = PWR_CTRL_CEVA_CORE0_KEY_EN,
		.key = PWR_CTRL_CEVA_CORE0_KEY
	}
};

static int pwr_ctrl_power_free(struct power_domain *power_domain)
{
	return 0;
}

static int pwr_ctrl_power_off(struct power_domain *power_domain)
{
	void __iomem *base_addr = NULL;
	void __iomem *ctrl_addr = NULL;
	void __iomem *ack_addr = NULL;
	u32 ret = 0;
	u32 val = 0;
	u32 tmp = (u32)(-1);
	struct pwr_ctrl *pcd = dev_get_priv(power_domain->dev);
	struct pwr_ctrl_tab *tab = NULL;

	switch(power_domain->id) {
	default:
		log_err("%s: invalid power domain id %lu\n", __func__, power_domain->id);
		return -EINVAL;

	case SCA200V100_PWR_CTRL_SOC_VENC:
	case SCA200V100_PWR_CTRL_SOC_ISP:
	case SCA200V100_PWR_CTRL_SOC_DLA:
	case SCA200V100_PWR_CTRL_A53_CORE1:
	case SCA200V100_PWR_CTRL_A53_CORE2:
	case SCA200V100_PWR_CTRL_A53_CORE3:
	case SCA200V100_PWR_CTRL_CEVA_CORE0:
		tab = &pcd->tab[power_domain->id];
		base_addr = pcd->base[tab->reg_type];
		log_debug("%s %lu base_addr=%p\n", __func__, power_domain->id, base_addr);
		if(tab->key_en == true) {
			log_debug("key_addr=%p, key=%x\n", base_addr + tab->reg_offset[PWR_CTRL_REG_OFFSET_TYPE_KEY], tab->key);
			val = readl(base_addr + tab->reg_offset[PWR_CTRL_REG_OFFSET_TYPE_KEY]);
			if (val != tab->key) {
				log_err("%s: %lu read invalid power domain key %d\n", __func__, power_domain->id, val);
				return -EPERM;
			}
		}

		ctrl_addr = base_addr + tab->reg_offset[PWR_CTRL_REG_OFFSET_TYPE_CTRL];
		ack_addr = base_addr + tab->reg_offset[PWR_CTRL_REG_OFFSET_TYPE_ACK];
		log_debug("%s %lu ctrl_addr=%p ack_addr=%p\n", __func__, power_domain->id, ctrl_addr, ack_addr);

		val = readl(ctrl_addr);
		val &= ~BIT(tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]);
		writel(val, ctrl_addr);

		val &= ~BIT(tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]);
		writel(val, ctrl_addr);

		ret = readl_poll_timeout(ack_addr, tmp, PWR_CTRL_POWEROFF_ACKED(tmp, tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]),
		        PWR_CTRL_POLL_TIMEOUT_INTERVAL);
		if (ret < 0) {
			log_err("%s: %lu mother not acked, ret=%x\n", __func__, power_domain->id, ret);
			return ret;
		}

		val &= ~BIT(tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]);
		writel(val, ctrl_addr);

		ret = readl_poll_timeout(ack_addr, tmp, PWR_CTRL_POWEROFF_ACKED(tmp, tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]),
		        PWR_CTRL_POLL_TIMEOUT_INTERVAL);
		if (ret < 0) {
			log_err("%s: %lu daughter not acked, ret=%x\n", __func__, power_domain->id, ret);
			return ret;
		}
		return 0;
	}
}

static int pwr_ctrl_power_on(struct power_domain *power_domain)
{
	void __iomem *base_addr = NULL;
	void __iomem *ctrl_addr = NULL;
	void __iomem *ack_addr = NULL;
	u32 ret = 0;
	u32 val = 0;
	u32 tmp = 0;
	struct pwr_ctrl *pcd = dev_get_priv(power_domain->dev);
	struct pwr_ctrl_tab *tab = NULL;

	switch(power_domain->id) {
	default:
		log_err("%s: invalid power domain id %lu\n", __func__, power_domain->id);
		return -EINVAL;

	case SCA200V100_PWR_CTRL_SOC_VENC:
	case SCA200V100_PWR_CTRL_SOC_ISP:
	case SCA200V100_PWR_CTRL_SOC_DLA:
	case SCA200V100_PWR_CTRL_A53_CORE1:
	case SCA200V100_PWR_CTRL_A53_CORE2:
	case SCA200V100_PWR_CTRL_A53_CORE3:
	case SCA200V100_PWR_CTRL_CEVA_CORE0:
		tab = &pcd->tab[power_domain->id];
		base_addr = pcd->base[tab->reg_type];
		log_debug("%s %lu base_addr=%p\n", __func__, power_domain->id, base_addr);
		if(tab->key_en == true) {
			log_debug("key_addr=%p, key=%x\n", base_addr + tab->reg_offset[PWR_CTRL_REG_OFFSET_TYPE_KEY], tab->key);
			val = readl(base_addr + tab->reg_offset[PWR_CTRL_REG_OFFSET_TYPE_KEY]);
			if (val != tab->key) {
				log_err("%s: %lu read invalid pmu key %d\n", __func__, power_domain->id, val);
				return -EPERM;
			}
		}

		ctrl_addr = base_addr + tab->reg_offset[PWR_CTRL_REG_OFFSET_TYPE_CTRL];
		ack_addr = base_addr + tab->reg_offset[PWR_CTRL_REG_OFFSET_TYPE_ACK];
		log_debug("%s %lu ctrl_addr=%p ack_addr=%p\n", __func__, power_domain->id, ctrl_addr, ack_addr);

		val = readl(ctrl_addr);
		val |= BIT(tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_M]);
		writel(val, ctrl_addr);

		ret = readl_poll_timeout(ack_addr, tmp, PWR_CTRL_POWERON_ACKED(tmp, tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_ACK_M]),
		        PWR_CTRL_POLL_TIMEOUT_INTERVAL);
		if (ret < 0) {
			log_err("%s: %lu mother not acked, ret=%x\n", __func__, power_domain->id, ret);
			return ret;
		}

		val |= BIT(tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_D]);
		writel(val, ctrl_addr);

		ret = readl_poll_timeout(ack_addr, tmp, PWR_CTRL_POWERON_ACKED(tmp, tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_ACK_D]),
		        PWR_CTRL_POLL_TIMEOUT_INTERVAL);
		if (ret < 0) {
			log_err("%s: %lu daughter not acked, ret=%x\n", __func__, power_domain->id, ret);
			return ret;
		}

		val |= BIT(tab->bit_offset[PWR_CTRL_BIT_OFFSET_TYPE_CTRL_ISO]);
		writel(val, ctrl_addr);

		return 0;
	}
}

static int pwr_ctrl_power_request(struct power_domain *power_domain)
{
	return 0;
}

static int sc_pwr_ctrl_init(struct udevice *dev)
{
	u32 array[SCA200V100_PWR_CTRL_MAX] = {0};
	u32 size = 0;
	u32 i = 0;
	int ret = 0;
	u32 id = 0;
	u32 status = 0;
	struct power_domain pd = {0};

	size = dev_read_size(dev, "pwr-default-gating");
	if (size < 0) {
		log_err("%s: read pwr-default-gating size %d error\n", __func__, size);
		return -EINVAL;
	}

	ret = dev_read_u32_array(dev, "pwr-default-gating", (u32 *)array, size / sizeof(u32));
	if (ret) {
		log_err("%s: read pwr-default-gating failed %d\n", __func__, ret);
		return -EINVAL;
	}

	for (i = 0; i < size / sizeof(u32); i++) {
		id = array[i] & PWR_CTRL_ID_MASK;
		status = array[i] & PWR_CTRL_STATUS_MASK;

		pd.dev = dev;
		pd.id = id;
		pd.priv = dev_get_priv(dev);

		switch (status) {
		default:
			log_err("%s: pwr-default-gating id %d status %d error\n", __func__, id, status);
			ret = -EINVAL;
			break;

		case SCA200V100_PWR_CTRL_OFF:
			ret = power_domain_off(&pd);
			break;
		}
	}

	log_debug("%s ret=%d\n", __func__, ret);

	return ret;
}

static int sc_pwr_ctrl_probe(struct udevice *dev)
{
	struct pwr_ctrl *pcd = dev_get_priv(dev);

	log_debug("%s pcd=%p\n", __func__, pcd);

	return 0;
}

static int sc_pwr_ctrl_remove(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	struct pwr_ctrl_plat *plat = dev_get_platdata(dev);

	regmap_uninit(plat->map);
#endif

	log_debug("%s", __func__);
	return 0;
}

static int sc_pwr_ctrl_ofdata_to_platdata(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	struct pwr_ctrl_plat *plat = dev_get_platdata(dev);
	struct pwr_ctrl *pcd = dev_get_priv(dev);
	int ret = 0;
	int reg_type = 0;

	ret = regmap_init_mem(dev_ofnode(dev), &plat->map);
	if (ret) {
		log_err("%s: regmap failed %d\n", __func__, ret);
		return ret;
	}

	for(reg_type = 0; reg_type < PWR_CTRL_REG_TYPE_MAX; reg_type++) {
		pcd->base[reg_type] = regmap_get_range(plat->map, reg_type);
	}
	pcd->tab = sc_pwr_ctrl_tab;

	sc_pwr_ctrl_init(dev);
#endif
	return 0;
}

static const struct udevice_id sc_pwr_ctrl_ids[] = {
	{
		.compatible = "smartchip,sca200v100-pwr_ctrl",
	},
	{ /* sentinel */ }
};

struct power_domain_ops sc_pwr_ctrl_ops = {
	.rfree = pwr_ctrl_power_free,
	.off = pwr_ctrl_power_off,
	.on = pwr_ctrl_power_on,
	.request = pwr_ctrl_power_request,
};

U_BOOT_DRIVER(sc_pwr_ctrl) = {
	.name = "sc_pwr_ctrl",
	.id = UCLASS_POWER_DOMAIN,
	.ops = &sc_pwr_ctrl_ops,
	.probe = sc_pwr_ctrl_probe,
	.remove = sc_pwr_ctrl_remove,
	.of_match = sc_pwr_ctrl_ids,
	.priv_auto_alloc_size = sizeof(struct pwr_ctrl),
	.ofdata_to_platdata = sc_pwr_ctrl_ofdata_to_platdata,
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	.platdata_auto_alloc_size = sizeof(struct pwr_ctrl_plat),
#endif
};

