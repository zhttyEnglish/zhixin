#ifndef __LINUX_RTP5905_MFD_H_
#define __LINUX_RTP5905_MFD_H_

#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/regmap.h>

/* rtp chip id list */
#define RTP5905_ID          0

#define RTP5905_ADDRESS         0x30

/* gpio */
#define RTP5905_GPIO_NUM        6
#define RTP5905_GPIO_BASE       0//RTP5905_GPIO_EXPANDER_BASE   /*not fix */

/* irq */
#ifdef CONFIG_ARCH_ROCKCHIP
	#define RTP5905_IRQ         0//PMU_INT_PIN          /* not fix */
#else
	#define RTP5905_IRQ         0//not fix
#endif

#define RTP5905_REG_NONE_WORTH      (0x8F)

/* dvs */
#define RTP5905_DVS_STEP_US     25000

/* rtp register define */
/* rtp system register */
#define RTP5905_REG_SYS_UVOV_EN     (0x01)
#define RTP5905_REG_SYS_UVOV_SET    (0x02)
#define RTP5905_REG_SYS_UVOV_STATUS (0x03)
#define RTP5905_REG_POW_INIT_ON     (0x04)
#define RTP5905_REG_PWRHOLD         (0x05)
#define RTP5905_REG_SYS_PAD_CTL     (0x0E)

/* rtp power config register */
#define RTP5905_REG_POWER_EN        (0x10)
#define RTP5905_REG_POWER_STB_EN    (0x12)
#define RTP5905_REG_POWER_CFG       (0x14)
#define RTP5905_REG_POWER_STATUS    (0x15)
#define RTP5905_REG_PMU_STATE_CTL   (0x17)
#define RTP5905_REG_STB_DC1VOUT     (0x18)
#define RTP5905_REG_STB_DC2VOUT     (0x19)
#define RTP5905_REG_STB_DC3VOUT     (0x1A)
#define RTP5905_REG_STB_DC4VOUT     (0x1B)
#define RTP5905_REG_STB_LDO1VOUT    (0x1D)
#define RTP5905_REG_STB_LDO2VOUT    (0x1F)

/* rtp dcdc config register */
#define RTP5905_REG_DC1VOUT_SET     (0x20)
#define RTP5905_REG_DC2VOUT_SET     (0x21)
#define RTP5905_REG_DC3VOUT_SET     (0x22)
#define RTP5905_REG_DC4VOUT_SET     (0x23)
#define RTP5905_REG_DVS_STEP_CTRL   (0x27)
#define RTP5905_REG_DVS_CTRL        (0x28)

/* rtp ldo config register */
#define RTP5905_REG_LDO1VOUT_SET    (0x51)
#define RTP5905_REG_LDO2VOUT_SET    (0x53)
#define RTP5905_REG_LDO1_REF_SEL    (0xF5)

/* rtp ldo config register */
#define RTP5905_REG_ACTIVE_CNTROL   (0x67)
/* SYS/GPIO control register */
#define RTP5905_REG_GPIO0_CTRL      (0x80)
#define RTP5905_REG_GPIO1_CTRL      (0x81)
#define RTP5905_REG_GPIO2_CTRL      (0x83)
#define RTP5905_REG_GPIO_IN_STAT    (0x84)
#define RTP5905_REG_PWREN_CTRL      (0x85)
#define RTP5905_REG_PWRGD_CTRL      (0x86)
#define RTP5905_REG_PONKEY_SETTING  (0x87)
#define RTP5905_REG_PWR_OFF_CFG     (0x88)

/* interrupt register */
#define RTP5905_REG_AVCCIN_IRQ_EN   (0x90)
#define RTP5905_REG_UVOV_IRQ_EN     (0x91)
#define RTP5905_REG_GPIO_IRQ_EN     (0x92)
#define RTP5905_REG_LDO_OCP_IRQ_EN  (0x93)
#define RTP5905_REG_PONKEY_IRQ_EN   (0x94)
#define RTP5905_REG_DC_OCOT_IRQ_EN  (0x95)
#define RTP5905_REG_AVCC_IRQ_EN2    (0x96)
#define RTP5905_REG_AVCC_IRQ_STS2   (0x97)
#define RTP5905_REG_ALDOIN_IRQ_STS  (0x98)
#define RTP5905_REG_PONKEY_IRQ_STS  (0x99)
#define RTP5905_REG_GPIO_IRQ_STS    (0x9A)
#define RTP5905_REG_LDO_OCP_IRQ_STS (0x9B)
#define RTP5905_REG_DC_UVOV_IRQ_STS (0x9C)
#define RTP5905_REG_DC_OCOT_IRQ_STS (0x9D)

#define RTP5905_REG_MAX_REGISTER    (0xFF)

/* rtp interrupter */
#define RTP5905_IRQ_ALDOIN_OFF      (((uint64_t) 1) << 2)
#define RTP5905_IRQ_ALDOIN_ON       (((uint64_t) 1) << 3)
#define RTP5905_IRQ_TIMEOUT     (((uint64_t) 1) << 4)
#define RTP5905_IRQ_GLOBAL      (((uint64_t) 1) << 7)

#define RTP5905_IRQ_DC1_OVP         (((uint64_t) 1) << 8)
#define RTP5905_IRQ_DC2_OVP         (((uint64_t) 1) << 9)
#define RTP5905_IRQ_DC3_OVP         (((uint64_t) 1) << 10)
#define RTP5905_IRQ_DC4_OVP         (((uint64_t) 1) << 11)
#define RTP5905_IRQ_DCDC1_UV        (((uint64_t) 1) << 12)
#define RTP5905_IRQ_DCDC2_UV        (((uint64_t) 1) << 13)
#define RTP5905_IRQ_DCDC3_UV        (((uint64_t) 1) << 14)
#define RTP5905_IRQ_DCDC4_UV        (((uint64_t) 1) << 15)

#define RTP5905_IRQ_GPIO0_F         (((uint64_t) 1) << 16)
#define RTP5905_IRQ_GPIO1_F         (((uint64_t) 1) << 17)
#define RTP5905_IRQ_GPIO2_F         (((uint64_t) 1) << 18)
#define RTP5905_IRQ_GPIO3_F         (((uint64_t) 1) << 19)
#define RTP5905_IRQ_GPIO0_R         (((uint64_t) 1) << 20)
#define RTP5905_IRQ_GPIO1_R         (((uint64_t) 1) << 21)
#define RTP5905_IRQ_GPIO2_R         (((uint64_t) 1) << 22)
#define RTP5905_IRQ_GPIO3_R         (((uint64_t) 1) << 23)

#define RTP5905_IRQ_LDO2_OCP        (((uint64_t) 1) << 28)
#define RTP5905_IRQ_LDO3_OCP        (((uint64_t) 1) << 29)
#define RTP5905_IRQ_LDO4_OCP        (((uint64_t) 1) << 30)
#define RTP5905_IRQ_LDO5_OCP        (((uint64_t) 1) << 31)

#define RTP5905_IRQ_ALARM       (((uint64_t) 1) << 34)
#define RTP5905_IRQ_PONKEY_SHORT    (((uint64_t) 1) << 35)
#define RTP5905_IRQ_PONKEY_F        (((uint64_t) 1) << 36)
#define RTP5905_IRQ_PONKEY_R        (((uint64_t) 1) << 37)
#define RTP5905_IRQ_PONKEY_OFF      (((uint64_t) 1) << 38)
#define RTP5905_IRQ_PONKEY_LONG     (((uint64_t) 1) << 39)

#define RTP5905_IRQ_DC1_OTP         (((uint64_t) 1) << 40)
#define RTP5905_IRQ_DC2_OTP         (((uint64_t) 1) << 41)
#define RTP5905_IRQ_DC3_OTP         (((uint64_t) 1) << 42)
#define RTP5905_IRQ_DC4_OTP         (((uint64_t) 1) << 43)
#define RTP5905_IRQ_DC1_OCP         (((uint64_t) 1) << 44)
#define RTP5905_IRQ_DC2_OCP         (((uint64_t) 1) << 45)
#define RTP5905_IRQ_DC3_OCP         (((uint64_t) 1) << 46)
#define RTP5905_IRQ_DC4_OCP         (((uint64_t) 1) << 47)

#define RTP5905_IRQ_AVCC_UV1        (((uint64_t) 1) << 48)
#define RTP5905_IRQ_AVCC_UV2        (((uint64_t) 1) << 49)
#define RTP5905_IRQ_AVCC_UV_OFF     (((uint64_t) 1) << 50)
#define RTP5905_IRQ_AVCC_OV1        (((uint64_t) 1) << 52)
#define RTP5905_IRQ_AVCC_OV2        (((uint64_t) 1) << 53)

#define RTP_RTC_NOTIFIER_ON     RTP5905_IRQ_ALARM

/* irq index */
#define RTP5905_IRQ_NUM_ALDOIN_OFF  (2)
#define RTP5905_IRQ_NUM_ALDOIN_ON   (3)
#define RTP5905_IRQ_NUM_TIMEOUT     (4)
#define RTP5905_IRQ_NUM_GLOBAL      (7)

#define RTP5905_IRQ_NUM_DC1_OVP     (8)
#define RTP5905_IRQ_NUM_DC2_OVP     (9)
#define RTP5905_IRQ_NUM_DC3_OVP     (10)
#define RTP5905_IRQ_NUM_DC4_OVP     (11)
#define RTP5905_IRQ_NUM_DCDC1_UV    (12)
#define RTP5905_IRQ_NUM_DCDC2_UV    (13)
#define RTP5905_IRQ_NUM_DCDC3_UV    (14)
#define RTP5905_IRQ_NUM_DCDC4_UV    (15)

#define RTP5905_IRQ_NUM_GPIO0_F     (16)
#define RTP5905_IRQ_NUM_GPIO1_F     (17)
#define RTP5905_IRQ_NUM_GPIO2_F     (18)
#define RTP5905_IRQ_NUM_GPIO3_F     (19)
#define RTP5905_IRQ_NUM_GPIO0_R     (20)
#define RTP5905_IRQ_NUM_GPIO1_R     (21)
#define RTP5905_IRQ_NUM_GPIO2_R     (22)
#define RTP5905_IRQ_NUM_GPIO3_R     (23)

#define RTP5905_IRQ_NUM_LDO2_OCP    (28)
#define RTP5905_IRQ_NUM_LDO3_OCP    (29)
#define RTP5905_IRQ_NUM_LDO4_OCP    (30)
#define RTP5905_IRQ_NUM_LDO5_OCP    (31)

#define RTP5905_IRQ_NUM_ALARM       (34)
#define RTP5905_IRQ_NUM_PONKEY_SHORT    (35)
#define RTP5905_IRQ_NUM_PONKEY_F    (36)
#define RTP5905_IRQ_NUM_PONKEY_R    (37)
#define RTP5905_IRQ_NUM_PONKEY_OFF      (38)
#define RTP5905_IRQ_NUM_PONKEY_LONG     (39)

#define RTP5905_IRQ_NUM_DC1_OTP     (40)
#define RTP5905_IRQ_NUM_DC2_OTP     (41)
#define RTP5905_IRQ_NUM_DC3_OTP     (42)
#define RTP5905_IRQ_NUM_DC4_OTP     (43)
#define RTP5905_IRQ_NUM_DC1_OCP     (44)
#define RTP5905_IRQ_NUM_DC2_OCP     (45)
#define RTP5905_IRQ_NUM_DC3_OCP     (46)
#define RTP5905_IRQ_NUM_DC4_OCP     (47)

#define RTP5905_IRQ_NUM_AVCC_UV1    (48)
#define RTP5905_IRQ_NUM_AVCC_UV2    (49)
#define RTP5905_IRQ_NUM_AVCC_UV_OFF     (50)
#define RTP5905_IRQ_NUM_AVCC_OV1    (52)
#define RTP5905_IRQ_NUM_AVCC_OV2    (53)

#define RTP_MFD_ATTR(_name)             \
{                       \
    .attr = {.name = #_name, .mode = 0644}, \
    .show =  _name##_show,          \
    .store = _name##_store,         \
}

/*regulator*/
#define RTP5905_REGULATOR_NUM   8
#define RTP5905_LDO1_VOL        1300

#define RTP5905_LDO(_pmic, _id, min, max, step, vreg, shift, nbits, ereg, ebit, svreg, sereg, voffset)  \
{                                               \
    .desc = {                                       \
        .name       = (_pmic),                          \
        .type       = REGULATOR_VOLTAGE,                        \
        .id         = (_id),                            \
        .n_voltages     = (step) ? ((max - min) / step + 1) : 1,            \
        .owner      = THIS_MODULE,                          \
    },                                          \
    .min_uV         = (min) * 1000,                         \
    .max_uV         = (max) * 1000,                         \
    .step_uV        = (step) * 1000,                        \
    .vol_reg        = (vreg),                           \
    .vol_shift      = (shift),                          \
    .vol_nbits      = (nbits),                          \
    .enable_reg         = (ereg),                           \
    .enable_bit         = (ebit),                           \
    .stb_vol_reg        = (svreg),                          \
    .stb_enable_reg     = (sereg),                          \
    .vol_offset     = (voffset),                            \
}

#define RTP5905_BUCK(_pmic, _id, min, max, step, vreg, shift, nbits, ereg, ebit, svreg, sereg, voffset) \
{                                               \
    .desc   = {                                     \
        .name       = (_pmic),                          \
        .type       = REGULATOR_VOLTAGE,                        \
        .id     = (_id),                            \
        .n_voltages     = (step) ? ((max - min) / step + 1) : 1,            \
        .owner      = THIS_MODULE,                          \
    },                                          \
    .min_uV         = (min) * 1000,                         \
    .max_uV         = (max) * 1000,                         \
    .step_uV        = (step) * 1000,                        \
    .vol_reg        = (vreg),                           \
    .vol_shift      = (shift),                          \
    .vol_nbits      = (nbits),                          \
    .enable_reg     = (ereg),                           \
    .enable_bit     = (ebit),                           \
    .stb_vol_reg        = (svreg),                          \
    .stb_enable_reg     = (sereg),                          \
    .vol_offset     = (voffset),                            \
}

#define RTP5905_REGU_ATTR(_name)                            \
{                                       \
    .attr = {.name = #_name, .mode = 0644},                 \
    .show = _name##_show,                           \
    .store = _name##_store,                         \
}
/********************************************************************************/

enum {
	RTP5905_ID_LDO1,
	RTP5905_ID_LDO2,

	RTP5905_ID_BUCK1,
	RTP5905_ID_BUCK2,
	RTP5905_ID_BUCK3,
	RTP5905_ID_BUCK4,

	RTP5905_ID_REGULATORS_NUM,
};

enum {
	RTP5905_DEV_PMIC,
	RTP5905_DEV_RTC,
	RTP5905_DEV_GPIO,
};

enum {
	RTP5905_IC_TYPE_A,
	RTP5905_IC_TYPE_B,
	RTP5905_IC_TYPE_D,
	RTP5905_IC_TYPE_G,
};

struct rtp5905_mfd_chip {
	struct device *dev;
	struct i2c_client *i2c_client;
	struct regmap *regmap;
	struct mutex io_mutex;
	//struct wake_lock irq_wake;
	unsigned long id;
	int (*read)(struct rtp5905_mfd_chip *chip, uint8_t reg, int size, uint8_t *dest);
	int (*write)(struct rtp5905_mfd_chip *chip, uint8_t reg, int size, uint8_t *src);

	/* GPIO Handling */
	struct gpio_chip gpio;

	/* IRQ Handling */
	struct mutex irq_lock;
	int chip_irq;
	uint64_t irq_enable;
	struct work_struct irq_work;
	struct blocking_notifier_head notifier_list;

	int (*init_irqs)(struct rtp5905_mfd_chip *chip);
	int (*update_irqs_en)(struct rtp5905_mfd_chip *chip);
	int (*read_irqs)(struct rtp5905_mfd_chip *chip, uint64_t *irqs);
	int (*write_irqs)(struct rtp5905_mfd_chip *chip, uint64_t irqs);
};

struct rtp5905_board {
	int gpio_base;
	int irq;
	int pmic_sleep_gpio;
	int pmic_hold_gpio;
	bool pmic_sleep;
	bool pm_off;
	unsigned long regulator_ext_sleep_control[RTP5905_ID_REGULATORS_NUM];
	struct regulator_init_data *rtp5905_pmic_init_data[RTP5905_ID_REGULATORS_NUM];

	/** Called before subdevices are set up */
	int (*pre_init)(struct rtp5905_mfd_chip *chip);
	/** Called after subdevices are set up */
	int (*post_init)(struct rtp5905_mfd_chip *chip);
	/** Called before ic is power down */
	int (*late_exit)(struct rtp5905_mfd_chip *chip);
};

struct rtp5905_pmu_info {
	char *regulator_name;
	int regulator_id;
	int regulator_vol;
	int regulator_stb_vol;
	int regulator_stb_en;
};

int rtp5905_set_bits(struct rtp5905_mfd_chip *chip, int reg, uint8_t bit_mask);
int rtp5905_clr_bits(struct rtp5905_mfd_chip *chip, int reg, uint8_t bit_mask);
int rtp5905_update_bits(struct rtp5905_mfd_chip *chip, int reg, uint8_t reg_val, uint8_t mask);
int rtp5905_reg_read(struct rtp5905_mfd_chip *chip, uint8_t reg, uint8_t *val);
int rtp5905_reg_write(struct rtp5905_mfd_chip *chip, uint8_t reg, uint8_t *val);
int rtp5905_bulk_read(struct rtp5905_mfd_chip *chip, uint8_t reg, int count, uint8_t *val);
int rtp5905_bulk_write(struct rtp5905_mfd_chip *chip, uint8_t reg, int count, uint8_t *val);

int rtp5905_register_notifier(struct rtp5905_mfd_chip *chip, struct notifier_block *nb, uint64_t irqs);
int rtp5905_unregister_notifier(struct rtp5905_mfd_chip *chip, struct notifier_block *nb, uint64_t irqs);

int rtp5905_irq_init(struct rtp5905_mfd_chip *chip, int irq);
int rtp5905_irq_exit(struct rtp5905_mfd_chip *chip);

void rtp5905_power_off(void);

extern int rtp5905_ic_type;
extern uint8_t rtp5905_regu_init_vol[RTP5905_ID_REGULATORS_NUM];

static inline int rtp5905_chip_id(struct rtp5905_mfd_chip *chip)
{
	return chip->id;
}
#if 0
static inline int rtp5905_read_reg(struct regmap *map, u8 reg, u8 *value)
{
	return regmap_read(map, reg, value);
}

static inline int rtp5905_bulk_read(struct regmap *map, u8 reg, u8 *buf,    int count)
{
	return regmap_bulk_read(map, reg, buf, count);
}

static inline int rtp5905_write_reg(struct regmap *map, u8 reg, u8 value)
{
	return regmap_write(map, reg, value);
}

static inline int rtp5905_bulk_write(struct regmap *map, u8 reg, u8 *buf, int count)
{
	return regmap_bulk_write(map, reg, buf, count);
}

static inline int rtp5905_update_bit(struct regmap *map, u8 reg, u8 mask, u8 val)
{
	return regmap_update_bits(map, reg, mask, val);
}

static inline int rtp5905_set_bit(struct regmap *map, int reg,  uint8_t bit_num)
{

	return regmap_update_bits(map, reg, BIT(bit_num), ~0u);
}

static inline int rtp5905_clr_bit(struct regmap *map, int reg,  uint8_t bit_num)
{

	return regmap_update_bits(map, reg, BIT(bit_num), 0u);
}
#endif
struct rtp5905_board *rtp5905_get_board(void);

#endif /* __LINUX_RTP5905_MFD_H_ */
