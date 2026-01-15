/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Header providing constants for SmartChip pinctrl bindings.
 *
 * Copyright (c) 2022 SmartChip
 */

#include <common.h>
#include <log.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <errno.h>
#include <reset.h>
#include <linux/bitops.h>

#define GPIO_DAT_PORTA      0xc4
#define GPIO_DAT_PORTB      0xd0
#define GPIO_DAT_PORTC      0xdc
#define GPIO_DAT_PORTD      0xe8
#define GPIO_DAT_PORTE      0xf4
#define GPIO_DAT_PORTF      0x100
#define GPIO_DAT_PORTG      0x10c

#define GPIO_SET_PORTA      0xbc
#define GPIO_SET_PORTB      0xc8

#define GPIO_DIR_PORTA      0xc0
#define GPIO_DIR_PORTB      0xcc

#define GPIO_INTR_MUX0      0x110
#define GPIO_INTR_MUX1      0x114
#define GPIO_INTR_ENABLE    0x118
#define GPIO_INTR_POLARITY  0x11c
#define GPIO_INTR_TYPE      0x120
#define GPIO_INTR_MASK      0x124
#define GPIO_INTR_EOI       0x12c
#define GPIO_INTR_RAWSTATUS 0x130

#define GPIO_DEBOUNCE       0x128

#define SMARTCHIP_MAX_PORTS   7
#define GPIO_DAT_PORT_SIZE  (GPIO_DAT_PORTB - GPIO_DAT_PORTA)
#define GPIO_SET_PORT_SIZE  (GPIO_SET_PORTB - GPIO_SET_PORTA)
#define GPIO_DIR_PORT_SIZE  (GPIO_DIR_PORTB - GPIO_DIR_PORTA)

#define GPIO_SWPORT_DR(p)       (GPIO_SET_PORTA + (p) * 0xc)
#define GPIO_SWPORT_DDR(p)      (GPIO_DIR_PORTA + (p) * 0xc)
#define GPIO_INTEN              0x30
#define GPIO_INTMASK            0x34
#define GPIO_INTTYPE_LEVEL      0x38
#define GPIO_INT_POLARITY       0x3c
#define GPIO_INTSTATUS          0x40
#define GPIO_PORTA_DEBOUNCE     0x48
#define GPIO_PORTA_EOI          0x4c
#define GPIO_EXT_PORT(p)        (GPIO_DAT_PORTA + (p) * 0xc)

#define GPIO_ADDRESS_OFFSET (0xbc)
#define SMARTCHIP_GPIO_CLOCK  (150 * 1000 * 1000)

struct gpio_smartchip_priv {
	struct reset_ctl_bulk   resets;
};

struct gpio_smartchip_platdata {
	const char  *name;
	int     bank;
	int     pins;
	fdt_addr_t  base;
};

static int smartchip_gpio_direction_input(struct udevice *dev, unsigned pin)
{
	struct gpio_smartchip_platdata *plat = dev_get_platdata(dev);

	setbits_le32(plat->base + GPIO_SWPORT_DDR(plat->bank), 1 << pin);
	return 0;
}

static int smartchip_gpio_direction_output(struct udevice *dev, unsigned pin,
    int val)
{
	struct gpio_smartchip_platdata *plat = dev_get_platdata(dev);

	clrbits_le32(plat->base + GPIO_SWPORT_DDR(plat->bank), 1 << pin);

	if (val)
		setbits_le32(plat->base + GPIO_SWPORT_DR(plat->bank), 1 << pin);
	else
		clrbits_le32(plat->base + GPIO_SWPORT_DR(plat->bank), 1 << pin);

	return 0;
}

static int smartchip_gpio_get_value(struct udevice *dev, unsigned pin)
{
	struct gpio_smartchip_platdata *plat = dev_get_platdata(dev);
	return !!(readl(plat->base + GPIO_EXT_PORT(plat->bank)) & (1 << pin));
}

static int smartchip_gpio_set_value(struct udevice *dev, unsigned pin, int val)
{
	struct gpio_smartchip_platdata *plat = dev_get_platdata(dev);

	if (val)
		setbits_le32(plat->base + GPIO_SWPORT_DR(plat->bank), 1 << pin);
	else
		clrbits_le32(plat->base + GPIO_SWPORT_DR(plat->bank), 1 << pin);

	return 0;
}

static int smartchip_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct gpio_smartchip_platdata *plat = dev_get_platdata(dev);
	u32 gpio;

	gpio = readl(plat->base + GPIO_SWPORT_DDR(plat->bank));
	if (gpio & BIT(offset))
		return GPIOF_INPUT;
	else
		return GPIOF_OUTPUT;
}

static const struct dm_gpio_ops gpio_smartchip_ops = {
	.direction_input    = smartchip_gpio_direction_input,
	.direction_output   = smartchip_gpio_direction_output,
	.get_value      = smartchip_gpio_get_value,
	.set_value      = smartchip_gpio_set_value,
	.get_function       = smartchip_gpio_get_function,
};

static int gpio_smartchip_reset(struct udevice *dev)
{
	int ret;
	struct gpio_smartchip_priv *priv = dev_get_priv(dev);

	ret = reset_get_bulk(dev, &priv->resets);
	if (ret) {
		/* Return 0 if error due to !CONFIG_DM_RESET and reset
		 * DT property is not present.
		 */
		if (ret == -ENOENT || ret == -ENOTSUPP)
			return 0;

		dev_warn(dev, "Can't get reset: %d\n", ret);
		return ret;
	}

	ret = reset_deassert_bulk(&priv->resets);
	if (ret) {
		reset_release_bulk(&priv->resets);
		dev_err(dev, "Failed to reset: %d\n", ret);
		return ret;
	}

	return 0;
}

static int gpio_smartchip_probe(struct udevice *dev)
{
	struct gpio_dev_priv *priv = dev_get_uclass_priv(dev);
	struct gpio_smartchip_platdata *plat = dev->platdata;

	if (!plat) {
		/* Reset on parent device only */
		return gpio_smartchip_reset(dev);
	}

	priv->gpio_count = plat->pins;
	priv->bank_name = plat->name;

	return 0;
}

static int gpio_smartchip_bind(struct udevice *dev)
{
	struct gpio_smartchip_platdata *plat = dev_get_platdata(dev);
	struct udevice *subdev;
	fdt_addr_t base;
	int ret, bank = 0;
	ofnode node;

	/* If this is a child device, there is nothing to do here */
	if (plat)
		return 0;

	base = dev_read_addr(dev);
	if (base == FDT_ADDR_T_NONE) {
		debug("Can't get the GPIO register base address\n");
		return -ENXIO;
	}

	for (node = dev_read_first_subnode(dev); ofnode_valid(node);
	    node = dev_read_next_subnode(node)) {
		if (!ofnode_read_bool(node, "gpio-controller"))
			continue;

		plat = devm_kcalloc(dev, 1, sizeof(*plat), GFP_KERNEL);
		if (!plat)
			return -ENOMEM;

		plat->base = base - GPIO_ADDRESS_OFFSET;
		plat->bank = bank;
		plat->pins = ofnode_read_u32_default(node, "smart,nr-gpios", 0);

		if (ofnode_read_string_index(node, "bank-name", 0,
		        &plat->name)) {
			/*
			 * Fall back to node name. This means accessing pins
			 * via bank name won't work.
			 */
			plat->name = ofnode_get_name(node);
		}

		ret = device_bind_ofnode(dev, dev->driver, plat->name,
		        plat, node, &subdev);
		if (ret)
			return ret;

		bank++;
	}

	return 0;
}

static int gpio_smartchip_remove(struct udevice *dev)
{
	struct gpio_smartchip_platdata *plat = dev_get_platdata(dev);
	struct gpio_smartchip_priv *priv = dev_get_priv(dev);

	if (!plat && priv)
		return reset_release_bulk(&priv->resets);

	return 0;
}

static const struct udevice_id gpio_smartchip_ids[] = {
	{ .compatible = "smartchip,gpio" },
	{ }
};

U_BOOT_DRIVER(gpio_smartchip) = {
	.name       = "gpio-smartchip",
	.id     = UCLASS_GPIO,
	.of_match   = gpio_smartchip_ids,
	.ops        = &gpio_smartchip_ops,
	.bind       = gpio_smartchip_bind,
	.probe      = gpio_smartchip_probe,
	.remove     = gpio_smartchip_remove,
	.priv_auto_alloc_size   = sizeof(struct gpio_smartchip_priv),
};

