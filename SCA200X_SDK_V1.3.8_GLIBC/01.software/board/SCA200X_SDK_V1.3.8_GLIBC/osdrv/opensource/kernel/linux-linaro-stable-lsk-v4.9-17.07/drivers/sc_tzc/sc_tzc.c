/*
 * smartchip linux tzc-400 driver
 * export tzc400_init_4f and tzc400_rgn_config
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/crc32.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>

#include <linux/sc_tzc.h>

struct tzc_private {
	void *tzc_base_2f;
	void *tzc_base_4f;
	u32    irq_num;
	struct device *dev;
};

static struct tzc_private *tzc = NULL;

#define tzc_writel(tzc_id, value, offset) \
do { \
    writel(value, ((tzc_id == 0) ? tzc->tzc_base_2f : tzc->tzc_base_4f) + offset); \
} \
while(0);

#define tzc_readl(tzc_id, offset, value) \
do { \
    value = readl(((tzc_id == 0) ? tzc->tzc_base_2f : tzc->tzc_base_4f) + offset); \
} \
while(0);

static void tzc400_init(u32 tzc_idx,  /*0: trust_2f, 1:trust_4f */
    u32 action,       //Control the interrupt and bus response signal hehavior
    u32 gate_keeper,  //Control gate keeper closed or open
    u32 spec_ctrl     //Spectulation access control
)
{
	tzc_writel(tzc_idx, action, TZ_ACTION          );
	tzc_writel(tzc_idx, gate_keeper, TZ_GATE_KEEPER     );
	tzc_writel(tzc_idx, spec_ctrl, TZ_SPECULATION_CTRL);
}

EXPORT_SYMBOL(tzc400_init);

void tzc400_rgn_config(
    u32 tzc_idx,  /*0: trust_2f, 1:trust_4f */
    u32 region_num,
    u32 region_base_low,
    u32 region_base_high,
    u32 region_top_low,
    u32 region_top_high,
    u32 region_attribute,
    u32 region_id_access
)
{
	tzc_writel(tzc_idx, region_base_low, RGN_BASE_LOW + 0x20 * region_num);
	tzc_writel(tzc_idx, region_base_high, RGN_BASE_HIGH + 0x20 * region_num);
	tzc_writel(tzc_idx, region_top_low, RGN_TOP_LOW   + 0x20 * region_num);
	tzc_writel(tzc_idx, region_top_high, RGN_TOP_HIGH  + 0x20 * region_num);
	tzc_writel(tzc_idx, region_attribute, RGN_ATTRIBUTE + 0x20 * region_num);
	tzc_writel(tzc_idx, region_id_access, RGN_ID_ACCESS + 0x20 * region_num);
}

/*
 * disable filter region in tzc-400
*/
void tzc400_disable_filter_region(u32 tzc_idx, int filter, int region_num)
{
	int value = 0;
	tzc_readl(tzc_idx, RGN_ATTRIBUTE + 0x20 * region_num, value);
	value &= (~(0x01 << filter));

	tzc_writel(tzc_idx, value, RGN_ATTRIBUTE + 0x20 * region_num);
}
EXPORT_SYMBOL(tzc400_disable_filter_region);

static u32 tzc400_print_intr_reason(u32 tzc_idx, int filter)
{
	u32 int_status;
	u32 fail_addr_l;
	u32 fail_addr_h;
	u32 fail_control;
	u32 fail_id;

	tzc_readl(tzc_idx, TZ_INT_STATUS, int_status);
	tzc_readl(tzc_idx, FAIL_ADDR_LOW  + 0x10 * filter, fail_addr_l);
	tzc_readl(tzc_idx, FAIL_ADDR_HIGH + 0x10 * filter, fail_addr_h);
	tzc_readl(tzc_idx, FAIL_CONTROL   + 0x10 * filter, fail_control);
	tzc_readl(tzc_idx, FAIL_ID        + 0x10 * filter, fail_id);

	dev_err(tzc->dev, "\nint status: 0x%x\n" \
	    "fail_addr_low_2: 0x%x\n" \
	    "fail_addr_high_2: 0x%x\n" \
	    "fail_control_2: 0x%x\n" \
	    "fail_id_2: 0x%x\n",
	    int_status, fail_addr_l, fail_addr_h, fail_control, fail_id);
	return 0;
}

static irqreturn_t tzc_irq_handler(int irq, void *arg)
{
	u32 int_status  = 0;

	tzc400_print_intr_reason(SC_TZC_400_4F, GMAC_FILTER_IDX );

	tzc_readl(SC_TZC_400_4F, TZ_INT_STATUS, int_status);
	tzc_writel(SC_TZC_400_4F, int_status, TZ_INT_CLEAR);

	return IRQ_HANDLED;
}

static int smartchip_tzc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *tzc_regs_2f = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct resource *tzc_regs_4f = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	if (!tzc_regs_2f || !tzc_regs_4f) {
		dev_emerg(&pdev->dev, "get resources 2f 4f: %p %p\n", tzc_regs_2f, tzc_regs_4f);
		goto out;
	}

	tzc = devm_kmalloc(&pdev->dev, sizeof(*tzc), GFP_KERNEL);
	if (!tzc)
		goto out;

	tzc->tzc_base_2f = devm_ioremap_nocache(&pdev->dev, tzc_regs_2f->start, resource_size(tzc_regs_2f));
	tzc->tzc_base_4f = devm_ioremap_nocache(&pdev->dev, tzc_regs_4f->start, resource_size(tzc_regs_4f));
	if (!tzc->tzc_base_2f || !tzc->tzc_base_4f) {
		dev_emerg(&pdev->dev, "remap resources 2f 4f: %p %p\n", tzc->tzc_base_2f, tzc->tzc_base_4f);
		goto out;
	}

	platform_set_drvdata(pdev, tzc);

	tzc->irq_num = platform_get_irq(pdev, 0);
	if (tzc->irq_num <= 0) {
		dev_emerg(&pdev->dev, "remap resources 2f 4f: %p %p\n", tzc->tzc_base_2f, tzc->tzc_base_4f);
		goto out;
	}

	ret = devm_request_irq(&pdev->dev, tzc->irq_num, tzc_irq_handler,
	        IRQF_SHARED, "tzc_irq", (void *)&pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to request AR eth_tzc_irq \n");
		goto out;
	}

	dev_info(&pdev->dev, "\nioremap resource 2f: 0x%x 0x%x to %p"\
	    "\nioremap resource 4f: 0x%x 0x%x to %p"\
	    "\ntzc_irq = %d \n",
	    tzc_regs_2f->start, resource_size(tzc_regs_2f), tzc->tzc_base_2f,
	    tzc_regs_4f->start, resource_size(tzc_regs_4f), tzc->tzc_base_4f,
	    tzc->irq_num);

	tzc->dev = &pdev->dev;

	return 0;

out:
	return -1;
}

static const struct of_device_id smartchip_tzc_match_table[] = {
	{.compatible = "smartchip,tzc-400"},
	{},
};

static int smartchip_tzc_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver smartchip_tzc_driver = {
	.probe = smartchip_tzc_probe,
	.remove = smartchip_tzc_remove,
	.driver = {
		.name = "tzc-smartchip",
		.of_match_table = smartchip_tzc_match_table,
	},
};

static int __init smartchip_tzc_init(void)
{
	return platform_driver_register(&smartchip_tzc_driver);
}

static void __exit smartchip_tzc_exit(void)
{
	platform_driver_unregister(&smartchip_tzc_driver);
}

MODULE_AUTHOR("smartchip");
MODULE_DESCRIPTION("smartchip tzc common driver");
MODULE_LICENSE("GPL");

subsys_initcall(smartchip_tzc_init);
module_exit(smartchip_tzc_exit);

