#include <linux/acpi.h>
#include <linux/gpio/driver.h>
/* FIXME: for gpio_get_value(), replace this with direct register read */
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/spinlock.h>
#include <linux/platform_data/gpio-smartchip.h>
#include <linux/slab.h>

#include "gpiolib.h"

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

#define SMARTCHIP_GPIO_CLOCK  (150 * 1000 * 1000)

struct smartchip_gpio;

struct smartchip_gpio_port {
	struct gpio_chip    gc;
	bool            is_registered;
	struct smartchip_gpio *gpio;
	unsigned int        idx;
};

struct smartchip_gpio {
	struct  device      *dev;
	void __iomem        *regs;
	struct smartchip_gpio_port    *ports;
	unsigned int        nr_ports;
	unsigned int        n_totalgpio;
	struct irq_domain   **domain;
	struct irq_chip     ic;
	int                 irq_used[SMARTCHIP_MAX_INTERRUPTS];
	struct smartchip_platform_data *pdata;

	spinlock_t          irq_lock;
	spinlock_t          lvl_lock;
};

static struct lock_class_key gpio_lock_class;

static inline u32 smartchip_read(struct smartchip_gpio *gpio, unsigned int offset)
{
	struct gpio_chip *gc    = &gpio->ports[0].gc;
	void __iomem *reg_base  = gpio->regs;

	return gc->read_reg(reg_base + offset);
}

static inline void smartchip_write(struct smartchip_gpio *gpio, unsigned int offset,
    u32 val)
{
	struct gpio_chip *gc    = &gpio->ports[0].gc;
	void __iomem *reg_base  = gpio->regs;

	gc->write_reg(reg_base + offset, val);
}

static int smartchip_gpio_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct smartchip_gpio_port *port = gpiochip_get_data(gc);
	struct smartchip_gpio *gpio = port->gpio;
	/*int i;

	for(i = 0; i < port->idx; ++i)
	{
	   offset += gpio->ports[i].gc.ngpio;
	}*/

	return irq_find_mapping(gpio->domain[port->idx], offset);
}

static int smartchip_irq_data_to_real_hwirq(struct smartchip_gpio *gpio, struct irq_data *d)
{
	int i = 0, real_offset = 0;

	for(i = 0; i < gpio->nr_ports; ++i) {
		if(d->domain == gpio->domain[i])
			break;
		real_offset += gpio->ports[i].gc.ngpio;
	}
	return real_offset + d->hwirq;
}

static int smartchip_gpio_set_debounce(struct gpio_chip *gc,
    unsigned offset, unsigned debounce)
{
	struct smartchip_gpio_port *port = gpiochip_get_data(gc);
	struct smartchip_gpio *gpio = port->gpio;
	unsigned long flags, val_deb;
	//unsigned long mask = gc->pin2mask(gc, offset);

	spin_lock_irqsave(&gc->bgpio_lock, flags);

	debounce *= (SMARTCHIP_GPIO_CLOCK / 1000);
	debounce = (debounce > 0x7) ? 0x7 : debounce;

	//set maxmium debounce val for gpio
	val_deb = smartchip_read(gpio, GPIO_DEBOUNCE);
	if(val_deb < debounce) {
		//smartchip gpio max debounce is 7 * 1 /150000000 s
		//unsigned debounce unit is ms, so we always set max.
		smartchip_write(gpio, GPIO_DEBOUNCE, debounce);
	}

	spin_unlock_irqrestore(&gc->bgpio_lock, debounce);

	return 0;
}

static int smartchip_gpio_add_port(struct smartchip_gpio *gpio,
    struct smartchip_port_property *pp,
    unsigned int offs)
{
	struct smartchip_gpio_port *port;
	void __iomem *dat, *set, *dirin;
	int err;

	port = &gpio->ports[offs];
	port->gpio = gpio;
	port->idx = pp->idx;

	dat = gpio->regs + GPIO_DAT_PORTA + (pp->idx * GPIO_DAT_PORT_SIZE);
	set = gpio->regs + GPIO_SET_PORTA + (pp->idx * GPIO_SET_PORT_SIZE);
	dirin = gpio->regs + GPIO_DIR_PORTA +
	    (pp->idx * GPIO_DIR_PORT_SIZE);

	err = bgpio_init(&port->gc, gpio->dev, 4, dat, set, NULL, NULL,
	        dirin, false);
	if (err) {
		dev_err(gpio->dev, "failed to init gpio chip for port%d\n",
		    port->idx);
		return err;
	}

#ifdef CONFIG_OF_GPIO
	port->gc.of_node = to_of_node(pp->fwnode);
#endif
	port->gc.ngpio = pp->ngpio;
	port->gc.base = pp->gpio_base;

	port->gc.set_debounce = smartchip_gpio_set_debounce;

	err = gpiochip_add_data(&port->gc, port);
	if (err)
		dev_err(gpio->dev, "failed to register gpiochip for port%d\n",
		    port->idx);
	else
		port->is_registered = true;

	gpio->n_totalgpio += pp->ngpio;

	//smartchip_configure_irqs(gpio, port, pp);

	/* Add GPIO-signaled ACPI event support */
	/*if (pp->irq)
	    acpi_gpiochip_request_interrupts(&port->gc);*/

	return err;
}

static void smartchip_gpio_unregister(struct smartchip_gpio *gpio)
{
	unsigned int m;

	for (m = 0; m < gpio->nr_ports; ++m)
		if (gpio->ports[m].is_registered)
			gpiochip_remove(&gpio->ports[m].gc);
}

static struct smartchip_platform_data *
smartchip_gpio_get_pdata(struct device *dev)
{
	struct fwnode_handle *fwnode;
	struct smartchip_platform_data *pdata;
	struct smartchip_port_property *pp;
	int nports;
	int i;
	static int gpio_base = 0;

	nports = device_get_child_node_count(dev);
	if (nports == 0)
		return ERR_PTR(-ENODEV);

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->properties = devm_kcalloc(dev, nports, sizeof(*pp), GFP_KERNEL);
	if (!pdata->properties)
		return ERR_PTR(-ENOMEM);

	pdata->nports = nports;
	//pdata->fwnode = dev->fwnode;

	pdata->nr_irqs = 0;
	if (fwnode_property_read_bool(dev->fwnode,
	        "interrupt-controller")) {
		for(i = 0; i < SMARTCHIP_MAX_INTERRUPTS; ++i) {
			pdata->irq[i] = irq_of_parse_and_map(dev->of_node, i);
			if (!pdata->irq)
				break;
			pdata->nr_irqs++;
		}
		dev_info(dev, "get %d irqs for all ports\n", i);
	}

	i = 0;
	device_for_each_child_node(dev, fwnode)  {
		pp = &pdata->properties[i++];
		pp->fwnode = fwnode;

		if (fwnode_property_read_u32(fwnode, "reg", &pp->idx) ||
		    pp->idx >= SMARTCHIP_MAX_PORTS) {
			dev_err(dev,
			    "missing/invalid port index for port%d\n", i);
			fwnode_handle_put(fwnode);
			return ERR_PTR(-EINVAL);
		}

		if (fwnode_property_read_u32(fwnode, "smart,nr-gpios",
		        &pp->ngpio)) {
			dev_info(dev,
			    "failed to get number of gpios for port%d\n",
			    i);
			pp->ngpio = 32;
		}

		pp->gpio_base = gpio_base;
		gpio_base += pp->ngpio;
	}

	return pdata;
}

/*static int smartchip_hwirq_to_gc(struct smartchip_gpio *gpio, unsigned hwirq, struct gpio_chip **gc)
{
    int i = 0, real_offset = hwirq;

    for(i = 0; i < SMARTCHIP_MAX_PORTS; ++i)
    {
        if(real_offset - gpio->ports[i].gc.ngpio < 0)
            break;
        real_offset -= gpio->ports[i].gc.ngpio;
    }
    *gc = &gpio->ports[i].gc;
    return real_offset;
}*/

static void smartchip_toggle_trigger(struct smartchip_gpio *gpio, unsigned int offs, unsigned channel)
{
	u32 v = smartchip_read(gpio, GPIO_INTR_POLARITY);
	int i = 0, real_offset = offs;

	/*for(i = 0; i < SMARTCHIP_MAX_PORTS; ++i)
	{
	    if(real_offset - gpio->ports[i].gc.ngpio < 0)
	        break;
	    real_offset -= gpio->ports[i].gc.ngpio;
	}*/

	printk(KERN_INFO "%s %d %d %d\n", __func__, __LINE__, offs, gpio->ports[i].gc.get(&gpio->ports[i].gc, real_offset));

	if (gpio->ports[i].gc.get(&gpio->ports[i].gc, real_offset))
		v &= ~BIT(channel);
	else
		v |= BIT(channel);

	smartchip_write(gpio, GPIO_INTR_POLARITY, v);
}

static irqreturn_t smartchip_irq_handler(int irq, void *dev_id)//(struct irq_desc *desc)
{
	struct smartchip_gpio *gpio = (struct smartchip_gpio *)dev_id;//irq_desc_get_handler_data(desc);
	struct smartchip_platform_data *pdata = gpio->pdata;//dev_get_platdata(gpio->dev);
	u32 irq_status = readl_relaxed(gpio->regs + GPIO_INTR_RAWSTATUS);
	//u32 irq_mask = readl_relaxed(gpio->regs + GPIO_INTR_MASK);
	u32 irq_channel = irq - pdata->irq[0];//irq_desc_get_irq(desc) - pdata->irq[0];
	//struct irq_chip *chip = irq_desc_get_chip(desc);
	u32 i = 0;

	if(irq_status & (1 << irq_channel)) {
		int hwirq = readl_relaxed(gpio->regs + GPIO_INTR_MUX0);
		int gpio_irq = 0;
		if(irq_channel >= 4) {
			hwirq = readl_relaxed(gpio->regs + GPIO_INTR_MUX1);
			hwirq = ((hwirq >> ((irq_channel - 4) * 8)) & 0xff);
		} else {
			hwirq = ((hwirq >> ((irq_channel - 0) * 8)) & 0xff);
		}

		for(i = 0; i < SMARTCHIP_MAX_PORTS; ++i) {
			if(hwirq - gpio->ports[i].gc.ngpio < 0)
				break;
			hwirq -= gpio->ports[i].gc.ngpio;
		}

		gpio_irq = irq_find_mapping(gpio->domain[i], hwirq);

		generic_handle_irq(gpio_irq);

		if ((irq_get_trigger_type(gpio_irq) & IRQ_TYPE_SENSE_MASK)
		    == IRQ_TYPE_EDGE_BOTH)
			smartchip_toggle_trigger(gpio, hwirq, irq_channel);

		irq_status &= ~BIT(irq_channel);
		writel_relaxed(irq_status, gpio->regs + GPIO_INTR_EOI);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static void smartchip_gpio_irq_ack(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct smartchip_gpio *gpio = igc->private;
	int hwirq = 0;//d->hwirq;
	int i = 0;

	hwirq = smartchip_irq_data_to_real_hwirq(gpio, d);

	for(i = 0; i < SMARTCHIP_MAX_INTERRUPTS; ++i) {
		if(hwirq == gpio->irq_used[i])
			break;
	}

	if(i >= SMARTCHIP_MAX_INTERRUPTS) {
		return;
	}

	smartchip_write(gpio, GPIO_INTR_EOI, (1 << i));
}

static void smartchip_gpio_irq_mask(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct smartchip_gpio *gpio = igc->private;
	int hwirq = 0;//d->hwirq;
	int i = 0, mask = 0;
	unsigned long flags;

	hwirq = smartchip_irq_data_to_real_hwirq(gpio, d);

	printk(KERN_INFO "%s %d %d\n", __func__, __LINE__, hwirq);

	spin_lock_irqsave(&gpio->irq_lock, flags);
	for(i = 0; i < SMARTCHIP_MAX_INTERRUPTS; ++i) {
		if(hwirq == gpio->irq_used[i])
			break;
	}

	if(i >= SMARTCHIP_MAX_INTERRUPTS) {
		spin_unlock_irqrestore(&gpio->irq_lock, flags);
		return;
	}

	mask = smartchip_read(gpio, GPIO_INTR_MASK);
	mask |= (1 << i);
	smartchip_write(gpio, GPIO_INTR_MASK, mask);

	spin_unlock_irqrestore(&gpio->irq_lock, flags);
}

static void smartchip_gpio_irq_unmask(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct smartchip_gpio *gpio = igc->private;
	int hwirq = 0;//d->hwirq;
	int i = 0, mask = 0;
	unsigned long flags;

	printk(KERN_INFO "%s %d %d\n", __func__, __LINE__, hwirq);

	hwirq = smartchip_irq_data_to_real_hwirq(gpio, d);

	spin_lock_irqsave(&gpio->irq_lock, flags);
	for(i = 0; i < SMARTCHIP_MAX_INTERRUPTS; ++i) {
		if(hwirq == gpio->irq_used[i])
			break;
	}

	if(i >= SMARTCHIP_MAX_INTERRUPTS) {
		spin_unlock_irqrestore(&gpio->irq_lock, flags);
		return;
	}

	mask = smartchip_read(gpio, GPIO_INTR_MASK);
	mask &= (~(1 << i));
	smartchip_write(gpio, GPIO_INTR_MASK, mask);

	spin_unlock_irqrestore(&gpio->irq_lock, flags);
}

static int smartchip_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct smartchip_gpio *gpio = igc->private;
	int bit = 0;
	unsigned long level, polarity, flags;
	int i = 0;

	if (type & ~(IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING |
	        IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW))
		return -EINVAL;

	spin_lock_irqsave(&gpio->irq_lock, flags);
	for(i = 0; i < SMARTCHIP_MAX_INTERRUPTS; ++i) {
		if(-1 == gpio->irq_used[i])
			break;

		printk(KERN_INFO "%s %d %d\n", __func__, __LINE__, gpio->irq_used[i]);
	}

	if(i >= SMARTCHIP_MAX_INTERRUPTS) {
		spin_unlock_irqrestore(&gpio->irq_lock, flags);
		printk(KERN_ERR "irq request more than %d\n", SMARTCHIP_MAX_INTERRUPTS);
		return -EINVAL;
	}
	bit = i;
	gpio->irq_used[i] = smartchip_irq_data_to_real_hwirq(gpio, d);
	spin_unlock_irqrestore(&gpio->irq_lock, flags);

	spin_lock_irqsave(&gpio->lvl_lock, flags);
	level = smartchip_read(gpio, GPIO_INTR_TYPE);
	polarity = smartchip_read(gpio, GPIO_INTR_POLARITY);

	switch (type) {
	case IRQ_TYPE_EDGE_BOTH:
		level |= BIT(bit);
		smartchip_toggle_trigger(gpio, d->hwirq, bit);
		break;
	case IRQ_TYPE_EDGE_RISING:
		level |= BIT(bit);
		polarity |= BIT(bit);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		level |= BIT(bit);
		polarity &= ~BIT(bit);
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		level &= ~BIT(bit);
		polarity |= BIT(bit);
		break;
	case IRQ_TYPE_LEVEL_LOW:
		level &= ~BIT(bit);
		polarity &= ~BIT(bit);
		break;
	}
	irq_setup_alt_chip(d, type);

	smartchip_write(gpio, GPIO_INTR_TYPE, level);
	smartchip_write(gpio, GPIO_INTR_POLARITY, polarity);
	spin_unlock_irqrestore(&gpio->lvl_lock, flags);

	return 0;
}

static void smartchip_gpio_irq_enable(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct smartchip_gpio *gpio = igc->private;
	int hwirq = 0;//d->hwirq;
	//struct gpio_chip *gc = NULL;
	unsigned long flags;
	int i = 0, val = 0;

	hwirq = smartchip_irq_data_to_real_hwirq(gpio, d);

	spin_lock_irqsave(&gpio->irq_lock, flags);

	for(i = 0; i < SMARTCHIP_MAX_INTERRUPTS; ++i) {
		if(hwirq == gpio->irq_used[i])
			break;

		printk(KERN_INFO "%s %d %d\n", __func__, __LINE__, gpio->irq_used[i]);
	}

	printk(KERN_INFO "%s %d %d %d\n", __func__, __LINE__, i, hwirq);

	if(i >= SMARTCHIP_MAX_INTERRUPTS) {
		spin_unlock_irqrestore(&gpio->irq_lock, flags);
		return;
	}

	printk(KERN_INFO "%s %d %d\n", __func__, __LINE__, gpio->irq_used[i]);

	if(i >= 4) {
		val = smartchip_read(gpio, GPIO_INTR_MUX1);
		smartchip_write(gpio, GPIO_INTR_MUX1, val | ((gpio->irq_used[i] & 0x7f) << ((i - 4) * 8)));
	} else {
		val = smartchip_read(gpio, GPIO_INTR_MUX0);
		smartchip_write(gpio, GPIO_INTR_MUX0, val | ((gpio->irq_used[i] & 0x7f) << (i * 8)));
	}

	val = smartchip_read(gpio, GPIO_INTR_ENABLE);
	smartchip_write(gpio, GPIO_INTR_ENABLE, val | (1 << i));

	spin_unlock_irqrestore(&gpio->irq_lock, flags);
}

static void smartchip_gpio_irq_disable(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct smartchip_gpio *gpio = igc->private;
	int hwirq = 0;//d->hwirq;
	unsigned long flags;
	int i = 0, val = 0;

	hwirq = smartchip_irq_data_to_real_hwirq(gpio, d);

	spin_lock_irqsave(&gpio->irq_lock, flags);

	for(i = 0; i < SMARTCHIP_MAX_INTERRUPTS; ++i) {
		if(hwirq == gpio->irq_used[i])
			break;
	}

	if(i >= SMARTCHIP_MAX_INTERRUPTS) {
		spin_unlock_irqrestore(&gpio->irq_lock, flags);
		return;
	}

	gpio->irq_used[i] = -1;

	val = smartchip_read(gpio, GPIO_INTR_ENABLE);
	smartchip_write(gpio, GPIO_INTR_ENABLE, val & (~(1 << i)));

	spin_unlock_irqrestore(&gpio->irq_lock, flags);
}

static void smartchip_configure_irqs(struct smartchip_gpio *gpio,
    struct smartchip_platform_data *pdata)
{
	struct irq_chip_generic *irq_gc = NULL;
	unsigned int hwirq;
	struct irq_chip_type *ct;
	int err, i, j;

	for(i = 0; i < pdata->nr_irqs; ++i) {
		gpio->irq_used[i] = -1;
		//printk(KERN_INFO "%s %d %d\n", __func__, __LINE__, gpio->irq_used[i]);
	}

	spin_lock_init(&gpio->irq_lock);
	spin_lock_init(&gpio->lvl_lock);

	gpio->domain = devm_kcalloc(gpio->dev, gpio->nr_ports, sizeof(struct irq_domain *),
	        GFP_KERNEL);
	if (!gpio->domain)
		return;

	for(i = 0; i < gpio->nr_ports; ++i) {
		struct smartchip_port_property *pp = &pdata->properties[i];
		struct smartchip_gpio_port *port = &gpio->ports[i];
		struct fwnode_handle  *fwnode = pp->fwnode;
		struct gpio_chip *gc = &port->gc;
		unsigned int ngpio = gc->ngpio;
		gpio->domain[i] = irq_domain_create_linear(fwnode, ngpio,
		        &irq_generic_chip_ops, gpio);
		if (!gpio->domain[i])
			goto err;

		err = irq_alloc_domain_generic_chips(gpio->domain[i], ngpio, 2,
		        "gpio-smartchip", handle_level_irq,
		        IRQ_NOREQUEST, 0,
		        IRQ_GC_INIT_NESTED_LOCK);
		if (err) {
			dev_info(gpio->dev, "irq_alloc_domain_generic_chips failed\n");
			irq_domain_remove(gpio->domain[i]);
			gpio->domain[i] = NULL;
			goto err;
		}

		irq_gc = irq_get_domain_generic_chip(gpio->domain[i], 0);
		if (!irq_gc) {
			irq_domain_remove(gpio->domain[i]);
			gpio->domain[i] = NULL;
			goto err;
		}

		irq_gc->reg_base = gpio->regs;
		irq_gc->private = gpio;
		port->gc.to_irq = smartchip_gpio_to_irq;

		for (j = 0; j < 2; j++) {
			ct = &irq_gc->chip_types[j];
			ct->chip.irq_ack = smartchip_gpio_irq_ack;
			ct->chip.irq_mask = smartchip_gpio_irq_mask;
			ct->chip.irq_unmask = smartchip_gpio_irq_unmask;
			ct->chip.irq_set_type = smartchip_gpio_irq_set_type;
			ct->chip.irq_enable = smartchip_gpio_irq_enable;
			ct->chip.irq_disable = smartchip_gpio_irq_disable;
			//ct->regs.ack = GPIO_PORTA_EOI;
			//ct->regs.mask = GPIO_INTMASK;
			ct->type = IRQ_TYPE_LEVEL_MASK;
		}

		irq_gc->chip_types[0].type = IRQ_TYPE_LEVEL_MASK;
		irq_gc->chip_types[1].type = IRQ_TYPE_EDGE_BOTH;
		irq_gc->chip_types[1].handler = handle_edge_irq;

		for (hwirq = 0; hwirq < ngpio; hwirq++) {
			irq_create_mapping(gpio->domain[i], hwirq);
		}
	}

	for(i = 0; i < pdata->nr_irqs; ++i) {
		/*
		 * Request a shared IRQ since where MFD would have devices
		 * using the same irq pin
		 */
		err = devm_request_irq(gpio->dev, pdata->irq[i],
		        smartchip_irq_handler,
		        IRQF_SHARED, "gpio-smartchip", gpio);
		if (err) {
			dev_err(gpio->dev, "error requesting IRQ\n");
			goto err;
		}
	}

	/* Unmask interrupts */
	smartchip_write(gpio, GPIO_INTR_MASK, 0x0);
	return;

err:
	for(i = 0; i < gpio->nr_ports; ++i) {
		if(NULL != gpio->domain[i])
			continue;

		for (hwirq = 0 ; hwirq < gpio->ports[i].gc.ngpio ; hwirq++)
			irq_dispose_mapping(irq_find_mapping(gpio->domain[i], hwirq));

		irq_domain_remove(gpio->domain[i]);
		gpio->domain[i] = NULL;
	}

	devm_kfree(gpio->dev, gpio->domain);
}

static void smartchip_irq_teardown(struct smartchip_gpio *gpio)
{
	//unsigned int n_totalgpio = gpio->n_totalgpio;
	irq_hw_number_t hwirq;
	int i = 0;

	if (!gpio->domain)
		return;

	for(i = 0; i < gpio->nr_ports; ++i) {
		if(NULL != gpio->domain[i])
			continue;

		for (hwirq = 0 ; hwirq < gpio->ports[i].gc.ngpio ; hwirq++)
			irq_dispose_mapping(irq_find_mapping(gpio->domain[i], hwirq));

		irq_domain_remove(gpio->domain[i]);
		gpio->domain[i] = NULL;
	}

	/* Mask out interrupts */
	smartchip_write(gpio, GPIO_INTR_MASK, 0xffffffff);

	devm_kfree(gpio->dev, gpio->domain);
}

static int smartchip_gpio_probe(struct platform_device *pdev)
{
	unsigned int i;
	struct resource *res;
	struct smartchip_gpio *gpio;
	int err;
	struct device *dev = &pdev->dev;
	struct smartchip_platform_data *pdata = dev_get_platdata(dev);

	if (!pdata) {
		pdata = smartchip_gpio_get_pdata(dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	}

	if (!pdata->nports)
		return -ENODEV;

	gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	gpio->dev = &pdev->dev;
	gpio->nr_ports = pdata->nports;
	gpio->pdata = pdata;

	gpio->ports = devm_kcalloc(&pdev->dev, gpio->nr_ports,
	        sizeof(*gpio->ports), GFP_KERNEL);
	if (!gpio->ports)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpio->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(gpio->regs))
		return PTR_ERR(gpio->regs);

	for (i = 0; i < gpio->nr_ports; i++) {
		err = smartchip_gpio_add_port(gpio, &pdata->properties[i], i);
		if (err)
			goto out_unregister;
	}

	smartchip_configure_irqs(gpio, pdata);

	platform_set_drvdata(pdev, gpio);

	return 0;

out_unregister:
	smartchip_gpio_unregister(gpio);
	smartchip_irq_teardown(gpio);

	return err;
}

static int smartchip_gpio_remove(struct platform_device *pdev)
{
	struct smartchip_gpio *gpio = platform_get_drvdata(pdev);

	smartchip_gpio_unregister(gpio);
	smartchip_irq_teardown(gpio);

	return 0;
}

static const struct of_device_id smartchip_of_match[] = {
	{ .compatible = "smartchip,gpio" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, smartchip_of_match);

static struct platform_driver smartchip_gpio_driver = {
	.driver     = {
		.name   = "gpio-smartchip",
		.of_match_table = of_match_ptr(smartchip_of_match),
	},
	.probe      = smartchip_gpio_probe,
	.remove     = smartchip_gpio_remove,
};

module_platform_driver(smartchip_gpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kwang");
MODULE_DESCRIPTION("SmartChip GPIO driver");

