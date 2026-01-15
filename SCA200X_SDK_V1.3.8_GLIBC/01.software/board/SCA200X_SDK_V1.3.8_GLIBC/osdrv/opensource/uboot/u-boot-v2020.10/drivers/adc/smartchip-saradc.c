// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <adc.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/err.h>

#define ABB_SAR10_CTL  0x18
#define ABB_SAR10_CHAN_SEL  0x24

#define SARADC_TIMEOUT			(100 * 1000)

struct smartchip_saradc_regs {
	unsigned int work_mode;
	unsigned int chan_sel;
	unsigned int ready_th;
	unsigned int interval;
	unsigned int data;
};

struct smartchip_saradc_data {
	int				num_bits;
	int				num_channels;
	unsigned long			clk_rate;
};

struct smartchip_saradc_priv {
	struct smartchip_saradc_regs		*regs;
	void __iomem *abb_base;

	int					active_channel;
	const struct smartchip_saradc_data	*data;
};

int smartchip_saradc_channel_data(struct udevice *dev, int channel,
				 unsigned int *data)
{
	struct smartchip_saradc_priv *priv = dev_get_priv(dev);
	struct adc_uclass_platdata *uc_pdata = dev_get_uclass_platdata(dev);

	if (channel != priv->active_channel) {
		pr_err("Requested channel is not active!");
		return -EINVAL;
	}

	if (0)
		return -EBUSY;

	/* Read value */
	*data = readl(&priv->regs->data);
	*data &= uc_pdata->data_mask;

	/* Power down adc */
	writel(1, priv->abb_base + ABB_SAR10_CTL);

	return 0;
}

int smartchip_saradc_start_channel(struct udevice *dev, int channel)
{
	struct smartchip_saradc_priv *priv = dev_get_priv(dev);

	if (channel < 0 || channel >= priv->data->num_channels) {
		pr_err("Requested channel is invalid!");
		return -EINVAL;
	}

	/* Select the channel to be used and trigger conversion */
	writel(0, priv->abb_base + ABB_SAR10_CTL);
	writel(channel, priv->abb_base + ABB_SAR10_CHAN_SEL);

	writel(0x0, &priv->regs->work_mode);
	readl(&priv->regs->data);

	priv->active_channel = channel;

	return 0;
}

int smartchip_saradc_stop(struct udevice *dev)
{
	struct smartchip_saradc_priv *priv = dev_get_priv(dev);

	/* Power down adc */
	writel(1, priv->abb_base + ABB_SAR10_CTL);

	priv->active_channel = -1;

	return 0;
}

int smartchip_saradc_probe(struct udevice *dev)
{
	struct smartchip_saradc_priv *priv = dev_get_priv(dev);

	priv->active_channel = -1;

	return 0;
}

int smartchip_saradc_ofdata_to_platdata(struct udevice *dev)
{
	struct adc_uclass_platdata *uc_pdata = dev_get_uclass_platdata(dev);
	struct smartchip_saradc_priv *priv = dev_get_priv(dev);
	struct smartchip_saradc_data *data;

	data = (struct smartchip_saradc_data *)dev_get_driver_data(dev);
	priv->regs = (struct smartchip_saradc_regs *)dev_read_addr(dev);
	if (priv->regs == (struct smartchip_saradc_regs *)FDT_ADDR_T_NONE) {
		pr_err("Dev: %s - can't get address!", dev->name);
		return -ENODATA;
	}
	priv->abb_base = (void *)dev_read_addr_index(dev, 1);
	if (priv->abb_base == (void *)FDT_ADDR_T_NONE) {
		pr_err("Dev: %s - can't get address!", dev->name);
		return -ENODATA;
	}

	priv->data = data;
	uc_pdata->data_mask = (1 << priv->data->num_bits) - 1;;
	uc_pdata->data_format = ADC_DATA_FORMAT_BIN;
	uc_pdata->data_timeout_us = SARADC_TIMEOUT / 5;
	uc_pdata->channel_mask = (1 << priv->data->num_channels) - 1;

	return 0;
}

static const struct adc_ops smartchip_saradc_ops = {
	.start_channel = smartchip_saradc_start_channel,
	.channel_data = smartchip_saradc_channel_data,
	.stop = smartchip_saradc_stop,
};

static const struct smartchip_saradc_data saradc_data = {
	.num_bits = 10,
	.num_channels = 8,
	.clk_rate = 1000000,
};

static const struct udevice_id smartchip_saradc_ids[] = {
	{ .compatible = "smartchip,saradc",
	  .data = (ulong)&saradc_data },
};

U_BOOT_DRIVER(smartchip_saradc) = {
	.name		= "smartchip_saradc",
	.id		= UCLASS_ADC,
	.of_match	= smartchip_saradc_ids,
	.ops		= &smartchip_saradc_ops,
	.probe		= smartchip_saradc_probe,
	.ofdata_to_platdata = smartchip_saradc_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct smartchip_saradc_priv),
};
