/*
 * PWM driver for SmartChip SoCs
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/time.h>

#define SMARTCHIP_PWM_NUM       (0x08)

#define REG_PWM_LOAD_COUNT  (0x00)
#define REG_PWM_LOAD_COUNT2     (0xb0)
#define REG_PWM_CTRL        (0x08)

#define SMART_PWM_SIZE      (0x14)
#define SMART_PWM_SIZE2          (0x04)

#define TIME_ENABLE     (0x01)
#define TIME_PWM_ENABLE     (0x01 << 3)
#define USER_DEFINED        (0x01 << 1)
#define PWM_ENABLE      (TIME_ENABLE | TIME_PWM_ENABLE | USER_DEFINED)

struct smartchip_pwm_regs {
	unsigned long on_count;
	unsigned long off_count;
	unsigned long ctrl;
};

struct smartchip_pwm_data {
	unsigned long       default_clk_rate;
	bool            supports_polarity;
	struct smartchip_pwm_regs regs;
	unsigned int        npwm;
};

struct smartchip_pwm_chip {
	struct pwm_chip     chip;
	struct clk      *clk;
	void __iomem        *base;
	const struct smartchip_pwm_data *data;
};

static inline struct smartchip_pwm_chip *to_smartchip_pwm_chip(struct pwm_chip *c)
{
	return container_of(c, struct smartchip_pwm_chip, chip);
}

static inline unsigned long smartchip_pwm_read_count(const struct pwm_device *pwm)
{
	struct smartchip_pwm_chip *smart_chip = to_smartchip_pwm_chip(pwm->chip);

	return readl(smart_chip->base + pwm->hwpwm * SMART_PWM_SIZE + REG_PWM_LOAD_COUNT);
}

static inline unsigned long smartchip_pwm_read_count2(const struct pwm_device *pwm)
{
	struct smartchip_pwm_chip *smart_chip = to_smartchip_pwm_chip(pwm->chip);

	return readl(smart_chip->base + pwm->hwpwm * SMART_PWM_SIZE2 + REG_PWM_LOAD_COUNT2);
}

static inline unsigned long smartchip_pwm_read_ctrl(const struct pwm_device *pwm)
{
	struct smartchip_pwm_chip *smart_chip = to_smartchip_pwm_chip(pwm->chip);

	return readl(smart_chip->base + pwm->hwpwm * SMART_PWM_SIZE + REG_PWM_CTRL);
}

static inline void smartchip_pwm_wrtie_count(const struct pwm_device *pwm, unsigned int value)
{
	struct smartchip_pwm_chip *smart_chip = to_smartchip_pwm_chip(pwm->chip);

	return writel(value, smart_chip->base + pwm->hwpwm * SMART_PWM_SIZE + REG_PWM_LOAD_COUNT);
}

static inline void smartchip_pwm_wrtie_count2(const struct pwm_device *pwm, unsigned int value)
{
	struct smartchip_pwm_chip *smart_chip = to_smartchip_pwm_chip(pwm->chip);

	return writel(value, smart_chip->base + pwm->hwpwm * SMART_PWM_SIZE2 + REG_PWM_LOAD_COUNT2);
}

static inline void smartchip_pwm_wrtie_ctrl(const struct pwm_device *pwm, unsigned int value)
{
	struct smartchip_pwm_chip *smart_chip = to_smartchip_pwm_chip(pwm->chip);

	return writel(value, smart_chip->base + pwm->hwpwm * SMART_PWM_SIZE + REG_PWM_CTRL);
}

static void smartchip_pwm_get_state(struct pwm_chip *chip,
    struct pwm_device *pwm,
    struct pwm_state *state)
{
	unsigned long clk_rate;
	struct smartchip_pwm_chip *smart_chip;
	u64 tmp;
	u32 ctrl;
	int ret;

	smart_chip = to_smartchip_pwm_chip(chip);
	if(NULL == smart_chip)
		return;

	ret = clk_enable(smart_chip->clk);
	if (ret)
		return;

	clk_rate = clk_get_rate(smart_chip->clk);
	tmp = smartchip_pwm_read_count(pwm) + smartchip_pwm_read_count2(pwm);
	tmp *= NSEC_PER_SEC;

	state->period = DIV_ROUND_CLOSEST_ULL(tmp, clk_rate);

	tmp = smartchip_pwm_read_count2(pwm);
	tmp *= NSEC_PER_SEC;
	state->duty_cycle = DIV_ROUND_CLOSEST_ULL(tmp, clk_rate);

	ctrl = smartchip_pwm_read_ctrl(pwm);
	ctrl &= PWM_ENABLE;

	if(ctrl) {
		state->enabled = 1;
	} else {
		state->enabled = 0;
	}
#if 1
	if(!smart_chip->data->supports_polarity) {
		state->polarity = PWM_POLARITY_NORMAL;
	}
#endif
	clk_disable(smart_chip->clk);
}

static int smartchip_pwm_config(struct pwm_chip *chip,
    struct pwm_device *pwm,
    struct pwm_state *state)
{
	struct smartchip_pwm_chip *smart_chip;
	unsigned long count1, count2;
	u64 div, clk_rate;

	smart_chip = to_smartchip_pwm_chip(chip);

	clk_rate = clk_get_rate(smart_chip->clk);

	div = clk_rate * (state->period - state->duty_cycle);
	count1 = DIV_ROUND_CLOSEST_ULL(div, NSEC_PER_SEC);

	div = clk_rate * state->duty_cycle;
	count2 = DIV_ROUND_CLOSEST_ULL(div, NSEC_PER_SEC);

	smartchip_pwm_wrtie_count(pwm, count1);
	smartchip_pwm_wrtie_count2(pwm, count2);

	return 0;
}

static int smartchip_pwm_apply(struct pwm_chip *chip,
    struct pwm_device *pwm,
    struct pwm_state *state)
{
	//unsigned long clk_rate;
	struct smartchip_pwm_chip *smart_chip;
	struct pwm_state curstate;
	bool enabled;
	int ret;

	smart_chip = to_smartchip_pwm_chip(chip);
	if(NULL == smart_chip)
		return PTR_ERR(smart_chip);

	ret = clk_enable(smart_chip->clk);
	if (ret)
		return ret;

	pwm_get_state(pwm, &curstate);
	enabled = curstate.enabled;

	smartchip_pwm_config(chip, pwm, state);
	if(state->enabled != enabled) {
		unsigned long ctrl;
		ctrl = smartchip_pwm_read_ctrl(pwm);

		if(state->enabled) {
			ctrl |= PWM_ENABLE;
		} else {
			ctrl &= (~PWM_ENABLE);
		}

		smartchip_pwm_wrtie_ctrl(pwm, ctrl);
	}

	smartchip_pwm_get_state(chip, pwm, state);

	clk_disable(smart_chip->clk);

	return 0;
}

static const struct pwm_ops smartchip_pwm_ops = {
	.get_state  = smartchip_pwm_get_state,
	.apply      = smartchip_pwm_apply,
	.owner      = THIS_MODULE,
};

static const struct smartchip_pwm_data sca200v100_pwm_data = {
	.default_clk_rate   = 150000000, //150M
	.supports_polarity  = 0,
	.npwm           = SMARTCHIP_PWM_NUM,
};

static const struct of_device_id smartchip_pwm_dt_ids[] = {
	{ .compatible = "smartchip,sca200v100-pwm", .data = &sca200v100_pwm_data},
	{},
};

static int smartchip_pwm_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	struct smartchip_pwm_chip *smart_chip;
	struct resource *r;
	int ret;

	//printk(KERN_ERR "pwm probe start!\n");

	id = of_match_device(smartchip_pwm_dt_ids, &pdev->dev);
	if (!id) {
		printk(KERN_ERR "pwm NO match device!\n");
		return -EINVAL;
	}

	smart_chip = devm_kzalloc(&pdev->dev, sizeof(*smart_chip), GFP_KERNEL);
	if (smart_chip == NULL)
		return -ENOMEM;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	smart_chip->base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(smart_chip->base))
		return PTR_ERR(smart_chip->base);

	smart_chip->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(smart_chip->clk))
		return PTR_ERR(smart_chip->clk);

	ret = clk_prepare_enable(smart_chip->clk);
	if(ret)
		return ret;

	platform_set_drvdata(pdev, smart_chip);

	smart_chip->data = id->data;
	smart_chip->chip.dev = &pdev->dev;
	smart_chip->chip.ops = &smartchip_pwm_ops;
	smart_chip->chip.base = -1;
	smart_chip->chip.npwm = smart_chip->data->npwm;

	ret = pwmchip_add(&smart_chip->chip);
	if (ret < 0) {
		clk_unprepare(smart_chip->clk);
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
	}

	printk(KERN_ERR "pwm probe success!\n");

	return ret;
}

static int smartchip_pwm_remove(struct platform_device *pdev)
{
	struct smartchip_pwm_chip *smart_chip;

	smart_chip = platform_get_drvdata(pdev);
	if(NULL == smart_chip)
		return -EINVAL;

	return pwmchip_remove(&smart_chip->chip);
}

static struct platform_driver smartchip_pwm_driver = {
	.driver = {
		.name = "smartchip-pwm",
		.of_match_table = smartchip_pwm_dt_ids,
	},
	.probe = smartchip_pwm_probe,
	.remove = smartchip_pwm_remove,
};
module_platform_driver(smartchip_pwm_driver);

MODULE_AUTHOR("kaiwang <kai.wang@smartchip.cn>");
MODULE_DESCRIPTION("smartchip SoC PWM driver");
MODULE_LICENSE("GPL v2");

