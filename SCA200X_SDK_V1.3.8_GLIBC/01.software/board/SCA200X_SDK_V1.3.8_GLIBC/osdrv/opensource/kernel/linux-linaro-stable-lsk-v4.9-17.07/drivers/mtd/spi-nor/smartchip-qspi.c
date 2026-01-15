/*
 * SmartChip qspi nor driver
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/spi-nor.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/smartchip-pinshare.h>
#include <linux/gpio/consumer.h>
#include <linux/dma-mapping.h>
#include "smartchip-qspi.h"
#include <linux/smartchip-mcp.h>
#include "sc_nor_dma.h"

static int dma_initial_flag = 0;

#define NOR_FLASH_32M  0x2000000 //unit is byte

/* Linux do not support cmd 0xeb, while all flash support it  */
#define SMARTCHIP_QSPI_CMD_FAST        0xeb

/* Read/Write regs, for Spansion 4k disable */
#define SMARTCHIP_CMD_READ_ANY_REG_SPANSION    (0x65)
#define SMARTCHIP_CMD_WRITE_ANY_REG_SPANSION   (0x71)

//SmartChip qspi controller registers
#define QSPI_CMD_REG(x)          ((x) * 4)
#define QSPI_CMD_INIT_REG_L(x)   (((x) * 2 + 64) * 4)
#define QSPI_CMD_INIT_REG_H(x)   (QSPI_CMD_INIT_REG_L(x) + 4)
#define QSPI_CMD_ADDR_REG(x)     (((x)+32) * 4)
#define QSPI_RD_HW_REG           (130 * 4)
#define QSPI_RD_LW_REG           (131 * 4)
#define QSPI_WR_HW_REG           (132 * 4)
#define QSPI_WR_LW_REG           (133 * 4)
#define QSPI_UPDATE_RD_REG       (134 * 4)
#define QSPI_UPDATE_WR_REG       (135 * 4)

#define QSPI_CFG_REG             (128 * 4)
#define QSPI_INIT_REG            (129 * 4)
#define QSPI_ACCESS_EN_REG       (136 * 4)

#define CLOCK_SPEED_MASK            (0xC)
#define CLOCK_SPEED_25M             (0<<2)
#define CLOCK_SPEED_50M             (1<<2)
#define CLOCK_SPEED_100M            (2<<2)

#define QSPI_PHASE_REVERSE (1 << 5)

/*Instruction bit map*/
#define HW_BUS_WIDTH_8_BIT          (0<<17)
#define HW_BUS_WIDTH_16_BIT         (1<<17)
#define HW_BUS_WIDTH_32_BIT         (2<<17)
#define HW_BUS_WIDTH_RESERVED       (3<<17)

#define HW_MODE_BIT_WIDTH_1_BIT     (0<<15)
#define HW_MODE_BIT_WIDTH_2_BIT     (1<<15)
#define HW_MODE_BIT_WIDTH_4_BIT     (2<<15)
#define HW_MODE_BIT_WIDTH_RESERVED  (3<<15)

#define HW_TAG_APB                  (0<<14)
#define HW_TAG_AHB                  (1<<14)

#define HW_DIR_READ                 (0<<13)
#define HW_DIR_WRITE                (1<<13)

#define HW_CMD_BIT_WIDTH_1_BIT      (0<<11)
#define HW_CMD_BIT_WIDTH_2_BIT      (1<<11)
#define HW_CMD_BIT_WIDTH_4_BIT      (2<<11)
#define HW_CMD_BIT_WIDTH_RESERVED   (3<<11)

#define HW_CMD_SHIFT    3
#define HW_CMD_MASK     8

#define HW_MODE_CYCLE_SHIFT 0
#define HW_MODE_CYCLE_MASK  3

#define LW_MODE_SHIFT     24
#define LW_MODE_MASK      8

#define LW_DUMMY_CYCLE_SHIFT 19
#define LW_DUMMY_CYCLE_MASK  5

#define LW_DATA_BIT_WIDTH_1_BIT     (0<<17)
#define LW_DATA_BIT_WIDTH_2_BIT     (1<<17)
#define LW_DATA_BIT_WIDTH_4_BIT     (2<<17)
#define LW_DATA_BIT_WIDTH_RESERVED  (3<<17)

#define LW_DATA_CYCLE_SHIFT   8
#define LW_DATA_CYCLE_MASK    9

#define LW_ADDRESS_BIT_WIDTH_1_BIT    (0<<6)
#define LW_ADDRESS_BIT_WIDTH_2_BIT    (1<<6)
#define LW_ADDRESS_BIT_WIDTH_4_BIT    (2<<6)
#define LW_ADDRESS_BIT_WIDTH_RESERVED (3<<6)

#define LW_ADDRESS_CNT_0             (0<<4)
#define LW_ADDRESS_CNT_3B            (1<<4)
#define LW_ADDRESS_CNT_4B            (2<<4)
#define LW_ADDRESS_CNT_RESERVED      (3<<4)

#define LW_CMD_QUAD_MODE_SHIFT      3
#define LW_CMD_QUAD_MODE_MASK       1
#define LW_ADDR_QUAD_MODE_SHIFT     2
#define LW_ADDR_QUAD_MODE_MASK      1
#define LW_MODE_QUAD_MODE_SHIFT     1
#define LW_MODE_QUAD_MODE_MASK      1
#define LW_DATA_QUAD_MODE_SHIFT     0
#define LW_DATA_QUAD_MODE_MASK      1

#define SMARTCHIP_QSPI_ERR(fmt, ...) \
    printk(KERN_ERR"[SMARTCHIP QSPI][err] %s %d, " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define SMARTCHIP_QSPI_DBG(fmt, ...) \
    printk(KERN_DEBUG"[SMARTCHIP QSPI][dbg] %s %d, " fmt, __func__, __LINE__, ##__VA_ARGS__)

void sc_nor_dma_write(struct sc_nor_host *host, unsigned int value, unsigned int reg)
{
	writel(value, (void __iomem *)((unsigned int)host->nor_dma_base + reg));
}
unsigned int sc_nor_dma_read(struct sc_nor_host *host, unsigned int reg)
{
	return readl((void __iomem *)((unsigned int)host->nor_dma_base + reg));
}
void sc_nor_dma_lowbit_write(struct sc_nor_host *host, unsigned int value)
{
	writel(value, (void __iomem *)((unsigned int)host->nor_dma_base_low));
}
unsigned int sc_nor_dma_lowbit_read(struct sc_nor_host *host)
{
	return readl((void __iomem *)((unsigned int)host->nor_dma_base_low));
}

static inline uint32_t smartchip_qspi_readl(struct smartchip_qspi *aq, uint32_t reg)
{
	return readl(aq->regs + reg);
}

static inline void smartchip_qspi_writel(struct smartchip_qspi *aq, uint32_t reg, uint32_t val)
{
	writel(val, aq->regs + reg);
}

static inline uint16_t smartchip_qspi_readw(struct smartchip_qspi *aq, uint32_t reg)
{
	return readw(aq->regs + reg);
}

static inline void smartchip_qspi_writew(struct smartchip_qspi *aq, uint32_t reg, uint16_t val)
{
	writew(val, aq->regs + reg);
}

static inline uint8_t smartchip_qspi_readb(struct smartchip_qspi *aq, uint32_t reg)
{
	return readb(aq->regs + reg);
}

static inline void smartchip_qspi_writeb(struct smartchip_qspi *aq, uint32_t reg, uint8_t val)
{
	writeb(val, aq->regs + reg);
}

/*----------------------------------------------------------------------------------------------*/
static void check_busy(struct spi_nor *nor)
{
	struct smartchip_qspi *aq = nor->priv;
	unsigned int tmp, busy;
	//reconstruct read status register command - 05h
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(12), 0x710);
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(12), 0x0028);
	smartchip_qspi_writel(aq, QSPI_CMD_ADDR_REG(12), (0xC0 << 16) | 0xFFFF);

	do {
		tmp = smartchip_qspi_readl(aq, QSPI_CMD_REG(12)); //active 05h
		busy = tmp & 0x1;
	} while(busy);
}

static void nor_select_chip(struct spi_nor *nor, int chip)
{
	//reconstruction DIE select command - C2h
	struct smartchip_qspi *aq = nor->priv;
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(31), 0x700);
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(31), 0x2610);

	switch (chip) {
	case 0:
		/* TODO: choose the NOR Flash(DIE-0) */
		smartchip_qspi_writel(aq, QSPI_CMD_REG(31), 0x0);
		check_busy(nor);
		break;
	case 1:
		/* TODO: choose the NOR Flash(DIE-1) */
		smartchip_qspi_writel(aq, QSPI_CMD_REG(31), 0x1);
		check_busy(nor);
		break;
	default:
		break;
	}
}

static int smartchip_qspi_read_reg(struct spi_nor *nor, u8 opcode,
    u8 *buf, int len)
{
	struct smartchip_qspi *aq = nor->priv;
	uint32_t instr_lw, instr_hw;
	int i;
	uint32_t data_cycle;
	int old_flag = 0;
	unsigned long long tmp = 0xff;

	if(len > 8)
		BUG();

	if(len)
		data_cycle = len * 8 - 1;
	else
		data_cycle = 0;

	switch(opcode) {
	case SPINOR_OP_RDCR:
	case SPINOR_OP_RDSR:
		instr_hw = HW_BUS_WIDTH_8_BIT | HW_TAG_APB | HW_DIR_READ | HW_CMD_BIT_WIDTH_1_BIT | opcode << HW_CMD_SHIFT;
		instr_lw = LW_DATA_BIT_WIDTH_1_BIT | data_cycle << LW_DATA_CYCLE_SHIFT | LW_ADDRESS_BIT_WIDTH_1_BIT | LW_ADDRESS_CNT_0;
		break;
	case SPINOR_OP_RDID:
		data_cycle = 63;
		instr_hw = HW_BUS_WIDTH_RESERVED | HW_TAG_APB | HW_DIR_READ | HW_CMD_BIT_WIDTH_1_BIT | opcode << HW_CMD_SHIFT;
		instr_lw = LW_DATA_BIT_WIDTH_1_BIT | data_cycle << LW_DATA_CYCLE_SHIFT | LW_ADDRESS_BIT_WIDTH_1_BIT | LW_ADDRESS_CNT_0;
		break;
	default:
		SMARTCHIP_QSPI_ERR("Unsupported cmd 0x%x\n", opcode);
		BUG();
	}

	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(0), instr_lw);
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(0), instr_hw);

	if(len == 1) {
		*(uint8_t *)&tmp = smartchip_qspi_readb(aq, QSPI_CMD_REG(0));
	} else if(len <= 2) {
		*(uint16_t *)&tmp = smartchip_qspi_readw(aq, QSPI_CMD_REG(0));
	} else if(len <= 4) {
		*(uint32_t *)&tmp = smartchip_qspi_readl(aq, QSPI_CMD_REG(0));
	} else if(len <= 8) {
		*(unsigned long long *)&tmp = *(unsigned long long *)(aq->regs + QSPI_CMD_REG(0));
	}

	memcpy(buf, &tmp, len);

	return 0;
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
static int smartchip_qspi_write_reg(struct spi_nor *nor, u8 opcode,
    u8 *buf, int len)
{
	struct smartchip_qspi *aq = nor->priv;
	uint32_t instr_lw, instr_hw;
	uint8_t status;
	int i;
	uint32_t data_cycle;
	unsigned long long tmp = 0;
	int old_flag = 0;

	/* 4 byte aligned for APB bus write*/
	if(len > 8)
		BUG();

	if(len)
		data_cycle = len * 8 - 1;
	else
		data_cycle = 0;

	memcpy(&tmp, buf, len);

	switch(opcode) {
	case SPINOR_OP_WRSR:
	case SPINOR_OP_WRDI:
	case SPINOR_OP_WREN:
	case SPINOR_OP_EN4B:
	case SPINOR_OP_EX4B:
		instr_hw = HW_BUS_WIDTH_RESERVED | HW_TAG_APB | HW_DIR_WRITE | HW_CMD_BIT_WIDTH_1_BIT | opcode << HW_CMD_SHIFT;
		instr_lw = LW_DATA_BIT_WIDTH_1_BIT | data_cycle << LW_DATA_CYCLE_SHIFT | LW_ADDRESS_BIT_WIDTH_1_BIT | LW_ADDRESS_CNT_0;
		break;
	default:
		SMARTCHIP_QSPI_ERR("Unsupported cmd 0x%x\n", opcode);
		BUG();
	}

	/* Configure controller */
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(0), instr_hw);
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(0), instr_lw);

	/* No data, write 0x1 to trigger command */
	if(!buf) {
		smartchip_qspi_writel(aq, QSPI_CMD_REG(0), 0x1);
	} else {
		if(len <= 4)
			smartchip_qspi_writel(aq, QSPI_CMD_REG(0), tmp);
		else
			*(unsigned long long *)(aq->regs +  QSPI_CMD_REG(0)) = tmp;

		/* Read status */
		while(1) {
			smartchip_qspi_read_reg(nor, SPINOR_OP_RDSR, &status, 1);
			status &= SR_WIP;
			if(status == 0)
				break;
			udelay(100);
		}
	}

	return 0;
}
#pragma GCC pop_options

static ssize_t _smartchip_qspi_write_3B(struct spi_nor *nor, loff_t to, size_t len,
    const u_char *write_buf)
{
	struct smartchip_qspi *aq = nor->priv;
	ssize_t ret;
	int i, tmplen1, tmplen2;
	int pinfunc;
	uint8_t status;
	unsigned int instr_hw, instr_lw;
	struct gpio_desc *desc;

	/* Get pin original func */
	pinfunc = smartchip_get_pinshare(PIN_NO(aq->pin_no));

	tmplen1 = len;

	while (tmplen1) {
		tmplen2 = tmplen1 < 256 ? tmplen1 : 256;

		/* Get GPIO & disable cs signal */
		desc = devm_gpiod_get(nor->dev, "cs", GPIOD_OUT_HIGH);
		if (IS_ERR(desc)) {
			dev_err(nor->dev, "Error getting GPIO\n");
			printk(KERN_EMERG"%s %d, error %d\n", __func__, __LINE__, PTR_ERR(desc));
			return PTR_ERR(desc);
		}

		/* Write enable */
		smartchip_qspi_write_reg(nor, SPINOR_OP_WREN, NULL, 0);

		udelay(1);

		/* Config qspi cs as gpio */
		smartchip_set_pinshare(PIN_NO(aq->pin_no), aq->pin_func);

		/* Enable CS signal */
		gpiod_set_value(desc, 0);

		/* Write first 32bits */
		*(uint8_t *)(aq->mem + to) = *((uint8_t *)write_buf);
		tmplen1 -= 1;
		tmplen2 -= 1;
		write_buf += 1;

		/* Write left data */
		for(i = 0; i < tmplen2; i++) {
			instr_hw = HW_BUS_WIDTH_RESERVED |
			    HW_TAG_APB |
			    HW_DIR_WRITE |
			    HW_CMD_BIT_WIDTH_1_BIT |
			    *(unsigned char *)write_buf << HW_CMD_SHIFT |
			    0 << HW_MODE_CYCLE_SHIFT;

			instr_lw = LW_DATA_BIT_WIDTH_1_BIT |
			    LW_ADDRESS_BIT_WIDTH_1_BIT |
			    LW_ADDRESS_CNT_0;

			/* Send one byte */
			smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(31), instr_lw);
			smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(31), instr_hw);
			smartchip_qspi_writel(aq, QSPI_CMD_REG(31), 0x1);
			write_buf++;
		}

		udelay(1);

		/* Disable CS signal */
		gpiod_set_value(desc, 1);

		devm_gpiod_put(nor->dev, desc);

		/* Restore pinshare */
		smartchip_set_pinshare(PIN_NO(aq->pin_no), pinfunc);

		/* check status */
		while(1) {
			smartchip_qspi_read_reg(nor, SPINOR_OP_RDSR, &status, 1);
			status &= SR_WIP;
			if(status == 0)
				break;

			udelay(100);
		};

		tmplen1 -= tmplen2;
	}

	return len;
}

static int smartchip_qspi_write_3B(struct spi_nor *nor, loff_t to, size_t len,
    const u_char *write_buf)
{
	unsigned int ret;
	unsigned int ret1 = 0, ret2_1 = 0, ret2_2 = 0, ret3 = 0;
	unsigned int to2 = 0, len1 = 0, len2 = 0;
	int old_flag = 0;
	struct smartchip_qspi *aq = nor->priv;

	if ((to + len) <= NOR_FLASH_32M) {
		ret1 = _smartchip_qspi_write_3B(nor, to, len, write_buf);
		ret = ret1;
	} else if ((to < NOR_FLASH_32M) && ((to + len) > NOR_FLASH_32M)) {
		len1 = NOR_FLASH_32M - to;
		ret2_1 = _smartchip_qspi_write_3B(nor, to, len1, write_buf);
		write_buf += len1;

		to2 = NOR_FLASH_32M;
		len2 = len - len1;
		nor_select_chip(nor, 1);
		ret2_2 = _smartchip_qspi_write_3B(nor, to2, len2, write_buf);
		nor_select_chip(nor, 0);
		ret = (ret2_1 + ret2_2);
	} else if(to >= NOR_FLASH_32M) {
		to = to - NOR_FLASH_32M;
		nor_select_chip(nor, 1);
		ret3 = _smartchip_qspi_write_3B(nor, to, len, write_buf);
		nor_select_chip(nor, 0);
		ret = ret3;
	}

	return ret;
}

static ssize_t _smartchip_qspi_write_4B(struct spi_nor *nor, loff_t to, size_t len,
    const u_char *write_buf)
{
	ssize_t ret;
	int i, tmplen1, tmplen2;
	int pinfunc;
	uint8_t status;
	unsigned int instr_hw, instr_lw;
	struct gpio_desc *desc;
	struct smartchip_qspi *aq = nor->priv;

	/* Get pin original func */
	pinfunc = smartchip_get_pinshare(PIN_NO(aq->pin_no));

	tmplen1 = len;

	while(tmplen1) {
		tmplen2 = tmplen1 < 256 ? tmplen1 : 256;

		/* Get GPIO & disable cs signal */
		desc = devm_gpiod_get(nor->dev, "cs", GPIOD_OUT_HIGH);
		if (IS_ERR(desc)) {
			dev_err(nor->dev, "Error getting GPIO\n");
			printk(KERN_EMERG"%s %d, error %d\n", __func__, __LINE__, PTR_ERR(desc));
			return PTR_ERR(desc);
		}

		/* Write enable */
		smartchip_qspi_write_reg(nor, SPINOR_OP_WREN, NULL, 0);

		udelay(1);

		/* Config qspi cs as gpio */
		smartchip_set_pinshare(PIN_NO(aq->pin_no), aq->pin_func);

		/* Enable CS signal */
		gpiod_set_value(desc, 0);

		/* Send write cmd */
		instr_hw = HW_BUS_WIDTH_RESERVED |
		    HW_TAG_APB |
		    HW_DIR_WRITE |
		    HW_CMD_BIT_WIDTH_1_BIT |
		    nor->program_opcode << HW_CMD_SHIFT |
		    0 << HW_MODE_CYCLE_SHIFT;

		instr_lw = LW_DATA_BIT_WIDTH_1_BIT |
		    LW_ADDRESS_BIT_WIDTH_1_BIT |
		    LW_ADDRESS_CNT_0;

		smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(31), instr_lw);
		smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(31), instr_hw);
		smartchip_qspi_writel(aq, QSPI_CMD_REG(31), 0x1);

		/* Send 4b address  */
		for(i = 0; i < 4; i++) {
			instr_hw = HW_BUS_WIDTH_RESERVED |
			    HW_TAG_APB |
			    HW_DIR_WRITE |
			    HW_CMD_BIT_WIDTH_1_BIT |
			    ((to >> ((3 - i) * 8)) & 0xff) << HW_CMD_SHIFT |
			    0 << HW_MODE_CYCLE_SHIFT;

			instr_lw = LW_DATA_BIT_WIDTH_1_BIT |
			    LW_ADDRESS_BIT_WIDTH_1_BIT |
			    LW_ADDRESS_CNT_0;

			smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(31), instr_lw);
			smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(31), instr_hw);
			smartchip_qspi_writel(aq, QSPI_CMD_REG(31), 0x1);
		}

		/* Write data */
		for (i = 0; i < tmplen2; i++) {
			instr_hw = HW_BUS_WIDTH_RESERVED |
			    HW_TAG_APB |
			    HW_DIR_WRITE |
			    HW_CMD_BIT_WIDTH_1_BIT |
			    *(unsigned char *)write_buf << HW_CMD_SHIFT |
			    0 << HW_MODE_CYCLE_SHIFT;

			instr_lw = LW_DATA_BIT_WIDTH_1_BIT |
			    LW_ADDRESS_BIT_WIDTH_1_BIT |
			    LW_ADDRESS_CNT_0;

			/* Send one byte */
			smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(31), instr_lw);
			smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(31), instr_hw);
			smartchip_qspi_writel(aq, QSPI_CMD_REG(31), 0x1);
			write_buf++;
		}

		udelay(1);

		/* Disable CS signal */
		gpiod_set_value(desc, 1);

		devm_gpiod_put(nor->dev, desc);

		/* Restore pinshare */
		smartchip_set_pinshare(PIN_NO(aq->pin_no), pinfunc);

		/* check status */
		while(1) {
			smartchip_qspi_read_reg(nor, SPINOR_OP_RDSR, &status, 1);
			status &= SR_WIP;
			if(status == 0)
				break;
			cond_resched();
			udelay(100);
		};

		to += tmplen2;
		tmplen1 -= tmplen2;
	}

	return len;
}

static int smartchip_qspi_write_4B(struct spi_nor *nor, loff_t to, size_t len,
    const u_char *write_buf)
{
	unsigned int ret;
	unsigned int ret1 = 0, ret2_1 = 0, ret2_2 = 0, ret3 = 0;
	unsigned int to2 = 0, len1 = 0, len2 = 0;
	struct smartchip_qspi *aq = nor->priv;
	int old_flag = 0;

	if ((to + len) <= NOR_FLASH_32M) {
		ret1 = _smartchip_qspi_write_4B(nor, to, len, write_buf);
		ret = ret1;
	} else if ((to < NOR_FLASH_32M) && ((to + len) > NOR_FLASH_32M)) {
		len1 = NOR_FLASH_32M - to;
		ret2_1 = _smartchip_qspi_write_4B(nor, to, len1, write_buf);
		write_buf += len1;

		to2 = NOR_FLASH_32M;
		len2 = len - len1;
		nor_select_chip(nor, 1);
		ret2_2 = _smartchip_qspi_write_4B(nor, to2, len2, write_buf);
		nor_select_chip(nor, 0);
		ret = (ret2_1 + ret2_2);
	} else if(to >= NOR_FLASH_32M) {
		to = to - NOR_FLASH_32M;
		nor_select_chip(nor, 1);
		ret3 = _smartchip_qspi_write_4B(nor, to, len, write_buf);
		nor_select_chip(nor, 0);
		ret = ret3;
	}

	return ret;
}

static ssize_t _smartchip_qspi_write(struct spi_nor *nor, loff_t to, size_t len,
    const u_char *write_buf)
{
	struct smartchip_qspi *aq = nor->priv;
	ssize_t ret;
	int i;
	uint8_t status;

	for(i = 0; i < len; i++) {
		/* Write enable */
		smartchip_qspi_write_reg(nor, SPINOR_OP_WREN, NULL, 0);

		*((u_char *)aq->mem + to + i) = *write_buf++;
		/* check status */
		while(1) {
			smartchip_qspi_read_reg(nor, SPINOR_OP_RDSR, &status, 1);
			status &= SR_WIP;
			if(status == 0)
				break;
			udelay(100);
		};
	}
	return len;
}

static int smartchip_qspi_write(struct spi_nor *nor, loff_t to, size_t len,
    const u_char *write_buf)
{
	unsigned int ret1 = 0, ret2_1 = 0, ret2_2 = 0, ret3 = 0;
	unsigned int to2 = 0, len1 = 0, len2 = 0;
	unsigned int ret;

	struct smartchip_qspi *aq = nor->priv;

	if ((to + len) <= NOR_FLASH_32M) {
		ret1 = _smartchip_qspi_write(nor, to, len, write_buf);
		ret = ret1;
	} else if ((to < NOR_FLASH_32M) && ((to + len) > NOR_FLASH_32M)) {
		len1 = NOR_FLASH_32M - to;
		ret2_1 = _smartchip_qspi_write(nor, to, len1, write_buf);
		write_buf += len1;

		to2 = NOR_FLASH_32M;
		len2 = len - len1;
		nor_select_chip(nor, 1);
		ret2_2 = _smartchip_qspi_write(nor, to2, len2, write_buf);
		nor_select_chip(nor, 0);
		ret = (ret2_1 + ret2_2);
	} else if(to >= NOR_FLASH_32M) {
		to = to - NOR_FLASH_32M;
		nor_select_chip(nor, 1);
		ret3 = _smartchip_qspi_write(nor, to, len, write_buf);
		nor_select_chip(nor, 0);
		ret = ret3;
	}

	return ret;
}

static ssize_t smartchip_qspi_dma_read(struct spi_nor *nor, loff_t from, size_t len, u_char *read_buf)
{
	struct smartchip_qspi *aq = nor->priv;
	struct sc_nor_host *host = aq->host;
	int ret, i = 0;
	size_t trans_size, ret_len = 0;
	unsigned int check_buf = 0;

	if(!len)
		return 0;

	if (dma_initial_flag == 0) {
		sc_ahb_dma_init(host);
		dma_initial_flag++;
	}

	while(len) {
		trans_size = len > host->temp_buf_len ? host->temp_buf_len : len;

		check_buf = *(unsigned int *)((u_char *)aq->mem + from + trans_size - 4);
		*(unsigned int *)((u_char *)host->temp_buf + trans_size - 4) = ~check_buf;

		ret = sc_ahb_dma_m2m(host, (u_char *)aq->map_phys + from,
		        nor->phys, trans_size, read_buf, check_buf);
		if(ret) {
			return 0;
		}
		ret_len  += trans_size;
		from     += trans_size;
		read_buf += trans_size;
		len      -= trans_size;
	}
	return ret_len;
}

static ssize_t smartchip_qspi_read(struct spi_nor *nor, loff_t from, size_t len,
    u_char *read_buf)
{
	struct smartchip_qspi *aq = nor->priv;
	int i;

	for(i = 0; i < len; i++)
		*read_buf++ = *((u_char *)aq->mem + from + i);

	return len;
}

static ssize_t _smartchip_qspi_dma_cpu_read(struct spi_nor *nor, loff_t from, size_t len,
    u_char *read_buf)
{
	struct smartchip_qspi *aq = nor->priv;
	size_t len1 = len;
	size_t left = 0;

#if 0
	while(len > 0) {
		if((from % 4) == 0)
			break;
		*read_buf++ = *((u_char *)aq->mem + from);
		from++;
		len--;
		if(len == 0)
			break;
	}
	if(len) {
		left = len % 4;
		len -= left;
		smartchip_qspi_dma_read(nor, from, len, read_buf);
		from += len;
		read_buf += len;
		while(left-- > 0) {
			*read_buf++ = *((u_char *)aq->mem + from);
			from++;
		}
	}
#else
	memcpy(read_buf, (u_char *)aq->mem + from, len);
#endif
	return len1;
}

static ssize_t smartchip_qspi_dma_cpu_read(struct spi_nor *nor, loff_t from, size_t len, u_char *read_buf)
{
	unsigned int len1 = 0, from2 = 0, len2 = 0;
	unsigned int ret1 = 0, ret2_1 = 0, ret2_2 = 0, ret3 = 0;
	unsigned int ret;

	int old_flag = 0;
	struct smartchip_qspi *aq = nor->priv;

	if ((from + len) <= NOR_FLASH_32M) {
		ret1 = _smartchip_qspi_dma_cpu_read(nor, from, len, read_buf);
		ret = ret1;
	}

	if ((from < NOR_FLASH_32M) && ((from + len) > NOR_FLASH_32M)) {
		len1 = NOR_FLASH_32M - from;
		ret2_1 = _smartchip_qspi_dma_cpu_read(nor, from, len1, read_buf);
		read_buf += len1;

		from2 = NOR_FLASH_32M;
		len2 = len - len1;
		nor_select_chip(nor, 1);
		ret2_2 = _smartchip_qspi_dma_cpu_read(nor, from2, len2, read_buf);
		nor_select_chip(nor, 0);
		ret = (ret2_1 + ret2_2);
	}

	if(from >= NOR_FLASH_32M) {
		from = from - NOR_FLASH_32M;
		nor_select_chip(nor, 1);
		ret3 = _smartchip_qspi_dma_cpu_read(nor, from, len, read_buf);
		nor_select_chip(nor, 0);
		ret = ret3;
	}

	return ret;
}

/* Single mode only */
static int _smartchip_qspi_erase(struct spi_nor *nor, loff_t offs)
{
	struct smartchip_qspi *aq = nor->priv;
	uint32_t instr_lw, instr_hw;
	uint8_t status;
	uint32_t add_cnt;

	add_cnt = nor->addr_width == 3 ? LW_ADDRESS_CNT_3B : LW_ADDRESS_CNT_4B;

	/* Write enable */
	smartchip_qspi_write_reg(nor, SPINOR_OP_WREN, NULL, 0);

	instr_hw = HW_BUS_WIDTH_RESERVED |
	    HW_MODE_BIT_WIDTH_1_BIT |
	    HW_TAG_APB | HW_DIR_WRITE |
	    HW_CMD_BIT_WIDTH_1_BIT |
	    nor->erase_opcode << HW_CMD_SHIFT;

	/* No mode */
	instr_lw = LW_DATA_BIT_WIDTH_1_BIT |
	    0 << LW_DATA_CYCLE_SHIFT |
	    LW_ADDRESS_BIT_WIDTH_1_BIT |
	    add_cnt;

	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(0), instr_hw);
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(0), instr_lw);
	smartchip_qspi_writel(aq, QSPI_CMD_ADDR_REG(0), offs);
	smartchip_qspi_writel(aq, QSPI_CMD_REG(0), 0x1);

	/* check status */
	while(1) {
		smartchip_qspi_read_reg(nor, SPINOR_OP_RDSR, &status, 1);
		status &= SR_WIP;
		if(status == 0)
			break;
		cond_resched();
		udelay(100);
	}
	return 0;
}

static int smartchip_qspi_erase(struct spi_nor *nor, loff_t offs)
{
	unsigned int ret1 = 0, ret2 = 0;
	unsigned int offs2 = 0;
	unsigned int ret;

	struct smartchip_qspi *aq = nor->priv;

	if (offs < NOR_FLASH_32M) {
		ret1 = _smartchip_qspi_erase(nor, offs);
		ret = ret1;
	}

	if(offs >= NOR_FLASH_32M) {
		offs = offs - NOR_FLASH_32M;
		nor_select_chip(nor, 1);
		ret2 = _smartchip_qspi_erase(nor, offs);
		nor_select_chip(nor, 0);
		ret = ret2;
	}

	return ret;
}

static int smartchip_qspi_set_clk(struct smartchip_qspi *aq, int clock)
{
	unsigned int val, setting;

	/* set clock, support 25M/50M/100M */
	switch(clock) {
	case 100000000:
		setting = CLOCK_SPEED_100M;
		break;
	case 50000000:
		setting = CLOCK_SPEED_50M;
		break;
	case 25000000:
		setting = CLOCK_SPEED_25M;
		break;
	default:
		return -EINVAL;
	}

	val = smartchip_qspi_readl(aq, QSPI_CFG_REG);
	val = (val & ~CLOCK_SPEED_MASK) | setting;
	smartchip_qspi_writel(aq, QSPI_CFG_REG, val);
	return 0;
}

static int smartchip_qspi_init(struct smartchip_qspi *aq)
{
	unsigned int val = 0;
	unsigned char clock = 0;

	SMARTCHIP_QSPI_DBG("qspi ctrl init: registers=%x,mem=%x,clock %d\n",
	    aq->regs, aq->mem, aq->clk_rate);

	smartchip_set_pinshare(PIN_NO(76), PIN_FUNC_QSPI_SCK);
	smartchip_set_pinshare(PIN_NO(77), PIN_FUNC_QSPI_CS);
	smartchip_set_pinshare(PIN_NO(80), PIN_FUNC_QSPI_DATA_0);
	smartchip_set_pinshare(PIN_NO(81), PIN_FUNC_QSPI_DATA_1);
	smartchip_set_pinshare(PIN_NO(82), PIN_FUNC_QSPI_DATA_2);
	smartchip_set_pinshare(PIN_NO(83), PIN_FUNC_QSPI_DATA_3);

	smartchip_qspi_writel(aq, QSPI_INIT_REG, 0x1);
	smartchip_qspi_writel(aq, QSPI_ACCESS_EN_REG, 0xacce55);

	/* Use 25M as default clk to read flash id */
	smartchip_qspi_set_clk(aq, 25000000);

	/* Reverse phase */
	val = smartchip_qspi_readl(aq, QSPI_CFG_REG);
	val |= QSPI_PHASE_REVERSE;
	smartchip_qspi_writel(aq, QSPI_CFG_REG, val);
	return 0;
}

static void smartchip_qspi_set_4b(struct spi_nor *nor, int enable)
{
	struct smartchip_qspi *aq = nor->priv;

	if(enable)
		smartchip_qspi_write_reg(nor, SPINOR_OP_EN4B, NULL, 0);
	else
		smartchip_qspi_write_reg(nor, SPINOR_OP_EX4B, NULL, 0);
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
static void smartchip_qspi_disable_4k(struct spi_nor *nor)
{
	struct smartchip_qspi *aq = nor->priv;
	uint8_t val = 0x08, status;
	uint32_t instr_hw, instr_lw;
	uint32_t addr_cnt = nor->addr_width == 3 ? LW_ADDRESS_CNT_3B : LW_ADDRESS_CNT_4B;

	/* Write enable */
	smartchip_qspi_write_reg(nor, SPINOR_OP_WREN, NULL, 0);

	instr_hw = HW_BUS_WIDTH_RESERVED |
	    HW_TAG_APB |
	    HW_DIR_WRITE |
	    HW_CMD_BIT_WIDTH_1_BIT |
	    SMARTCHIP_CMD_WRITE_ANY_REG_SPANSION << HW_CMD_SHIFT;

	instr_lw = LW_DATA_BIT_WIDTH_1_BIT |
	    7 << LW_DATA_CYCLE_SHIFT |
	    LW_ADDRESS_BIT_WIDTH_1_BIT |
	    addr_cnt;

	/* Write 0x8 to register CR3NV */
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(0), instr_hw);
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(0), instr_lw);
	smartchip_qspi_writel(aq, QSPI_CMD_ADDR_REG(0), 0x4);
	smartchip_qspi_writel(aq, QSPI_CMD_REG(0), val);

	/* check status */
	while(1) {
		smartchip_qspi_read_reg(nor, SPINOR_OP_RDSR, &status, 1);
		status &= SR_WIP;
		if(status == 0)
			break;
		udelay(100);
	}
}

static void smartchip_qspi_drive_enhance_winbond(struct spi_nor *nor)
{
#define WINB_CMD_REG_ACCESS 0x50
#define WINB_CMD_RD_REG3 0x15
#define WINB_CMD_WR_REG3 0x11
	struct smartchip_qspi *aq = nor->priv;
	unsigned int instr_hw, instr_lw;

	instr_hw = HW_BUS_WIDTH_8_BIT |
	    HW_TAG_APB |
	    HW_DIR_WRITE |
	    HW_CMD_BIT_WIDTH_1_BIT |
	    WINB_CMD_REG_ACCESS << HW_CMD_SHIFT;

	/* Enable access register */
	instr_lw = 0;
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(31), instr_lw); //CMD 01h
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(31), instr_hw); //CMD 01h
	smartchip_qspi_writel(aq, QSPI_CMD_REG(31), 0x1);

	instr_hw = HW_BUS_WIDTH_8_BIT |
	    HW_TAG_APB |
	    HW_DIR_WRITE |
	    HW_CMD_BIT_WIDTH_1_BIT |
	    WINB_CMD_WR_REG3 << HW_CMD_SHIFT;

	instr_lw = LW_DATA_BIT_WIDTH_1_BIT |
	    7 << LW_DATA_CYCLE_SHIFT |
	    LW_ADDRESS_BIT_WIDTH_1_BIT;

	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_L(31), instr_lw); //CMD 01h
	smartchip_qspi_writel(aq, QSPI_CMD_INIT_REG_H(31), instr_hw); //CMD 01h
	smartchip_qspi_writel(aq, QSPI_CMD_REG(31), 0x0);
}

static int smartchip_qspi_prepare(struct spi_nor *nor, enum spi_nor_ops ops)
{
	mcp_mutex_get();
	return 0;
}

static void smartchip_qspi_unprepare(struct spi_nor *nor, enum spi_nor_ops ops)
{
	mcp_mutex_release();
}
#pragma GCC pop_options

static int smartchip_qspi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *node;
	struct resource *res;
	struct spi_nor *nor;
	struct mtd_info *mtd;
	struct smartchip_qspi *aq;
	struct sc_nor_host *host;
	int ret = 0;
	int err = 0;
	int addr_cnt;
	int part_cnt;
	uint32_t instr_lw, instr_hw;
	uint32_t clock_frequency;
	uint32_t part_size, part_offs = 0;
	int old_flag;
	const char *part_name;
#ifdef CONFIG_SMARTCHIP_QSPI_ENHANCE_PATCH
	int gpio, pinfunc, pinno;
	enum of_gpio_flags flags;
#endif

	aq = devm_kzalloc(&pdev->dev, sizeof(*aq), GFP_KERNEL);
	if (!aq) {
		return -ENOMEM;
	}

	/* Map the registers */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "qspi_base");
	aq->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(aq->regs)) {
		dev_err(&pdev->dev, "missing registers\n");
		err = PTR_ERR(aq->regs);
		goto exit;
	}

	/* Map the AHB memory */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "qspi_mmap");
	aq->mem = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(aq->mem)) {
		dev_err(&pdev->dev, "missing AHB memory\n");
		err = PTR_ERR(aq->mem);
		goto exit;
	}

	aq->map_phys = res->start;

	if (!of_property_read_u32(np, "clock-frequency", &clock_frequency))
		aq->clk_rate = clock_frequency;
	else
		return -EINVAL;

	/* Setup the spi-nor */
	nor = &aq->nor;
	mtd = &nor->mtd;

	nor->dev = &pdev->dev;
	spi_nor_set_flash_node(nor, np);

	host = devm_kzalloc(dev, sizeof(*host), GFP_KERNEL);
	if (!host) {
		dev_err(dev, "devm_kzalloc fail\n");
		return -ENOMEM;
	}
	host->dev = dev;
	platform_set_drvdata(pdev, host);

#if(NOR_DMA==1)
	host->temp_buf_len = PAGE_SIZE;

	if (!host->temp_buf)
		host->temp_buf = dmam_alloc_coherent(nor->dev,
		        host->temp_buf_len, &(nor->phys), GFP_KERNEL);
	if(!host->temp_buf)
		return -ENOMEM;

	if(!host->dma_llp)
		host->dma_llp = dmam_alloc_coherent(nor->dev, sizeof(struct ahb_dma_llp) * AHB_DMA_MAX_LLP_ND, &host->dma_buffer,
		        GFP_KERNEL);
	if(!host->dma_llp)
		return -ENOMEM;

	host->nor_dma_base = devm_ioremap_nocache(dev, BASE_ADDR_DMA, 0x500);
	if (IS_ERR(host->nor_dma_base)) {
		ret = PTR_ERR(host->nor_dma_base);
		dev_err(dev, "aq->nor_dma_base ioremap fail\n");
	} else {
		dev_info(dev, "aq->nor_dma_base = %p\n", host->nor_dma_base);
	}

	host->nor_dma_base_low = devm_ioremap_nocache(dev, BASE_ADDR_DMA_LOW_BIT, 0x4);
	if (IS_ERR(host->nor_dma_base_low)) {
		ret = PTR_ERR(host->nor_dma_base_low);
		dev_err(dev, "host->nor_dma_base_low ioremap fail\n");
	} else {
		dev_info(dev, "host->nor_dma_base_low = %p\n", host->nor_dma_base_low);
	}

	aq->host = host;
#endif

	nor->priv = aq;
	mtd->priv = nor;
	nor->read_reg  = smartchip_qspi_read_reg;
	nor->write_reg = smartchip_qspi_write_reg;
	nor->write     = smartchip_qspi_write;
	nor->erase     = smartchip_qspi_erase;
	nor->read      = smartchip_qspi_dma_cpu_read;
	nor->prepare = smartchip_qspi_prepare;
	nor->unprepare = smartchip_qspi_unprepare;

	mcp_mutex_get();
	err = smartchip_qspi_init(aq);
	if (err)
		goto exit;

	for(err = 0; err < 30; err++)
		SMARTCHIP_QSPI_DBG("%d %x %x\n", err,
		    smartchip_qspi_readl(aq, QSPI_CMD_INIT_REG_H(err)),
		    smartchip_qspi_readl(aq, QSPI_CMD_INIT_REG_L(err)));

	/* Read id */
	smartchip_qspi_read_reg(nor, SPINOR_OP_RDID, aq->id, 8);

	/* Scan nor flash */
	err = spi_nor_scan(nor, NULL, SPI_NOR_QUAD);
	if (err)
		goto exit;

	addr_cnt = nor->addr_width == 3 ? LW_ADDRESS_CNT_3B : LW_ADDRESS_CNT_4B;

#ifdef CONFIG_SMARTCHIP_QSPI_ENHANCE_PATCH
	if(addr_cnt == LW_ADDRESS_CNT_3B)
		nor->write = smartchip_qspi_write_3B;
	else
		nor->write = smartchip_qspi_write_4B;
#endif

	/* Configure read registers, read 1 byte for each cycle */
	if(nor->flash_read == SPI_NOR_QUAD) {
		nor->read_opcode = SMARTCHIP_QSPI_CMD_FAST;

		switch(aq->id[0]) {
		case SNOR_MFR_SPANSION:
			aq->read_dummy_cycle = 7;
			break;
		case SNOR_MFR_WINBOND:
			aq->read_dummy_cycle = 3;
			break;
		default:
			aq->read_dummy_cycle = 15;
			break;
		}

		/* Set contoller quad/4B for read, one byte for each read */
		instr_hw = HW_BUS_WIDTH_RESERVED |
		    HW_TAG_AHB |
		    HW_DIR_READ |
		    HW_CMD_BIT_WIDTH_1_BIT |
		    nor->read_opcode << HW_CMD_SHIFT |
		    7 << HW_MODE_CYCLE_SHIFT;

		instr_lw = (aq->read_dummy_cycle) << LW_DUMMY_CYCLE_SHIFT |
		    LW_DATA_BIT_WIDTH_4_BIT |
		    7 << LW_DATA_CYCLE_SHIFT |
		    LW_ADDRESS_BIT_WIDTH_4_BIT |
		    addr_cnt |
		    1 << LW_ADDR_QUAD_MODE_SHIFT |
		    1 << LW_DATA_QUAD_MODE_SHIFT |
		    1 << LW_MODE_QUAD_MODE_SHIFT;

		smartchip_qspi_writel(aq, QSPI_RD_LW_REG, instr_lw);
		smartchip_qspi_writel(aq, QSPI_RD_HW_REG, instr_hw);
		smartchip_qspi_writel(aq, QSPI_UPDATE_RD_REG, 0x1);
	} else {
		nor->read_opcode = SPINOR_OP_READ_FAST;

		switch(aq->id[0]) {
		default:
			aq->read_dummy_cycle = 7;
			break;
		}

		/* Set contoller single/4B for read */
		instr_hw = HW_BUS_WIDTH_RESERVED |
		    HW_TAG_AHB | HW_DIR_READ |
		    HW_CMD_BIT_WIDTH_1_BIT |
		    nor->read_opcode << HW_CMD_SHIFT |
		    0 << HW_MODE_CYCLE_SHIFT;

		instr_lw = (aq->read_dummy_cycle) << LW_DUMMY_CYCLE_SHIFT |
		    LW_DATA_BIT_WIDTH_1_BIT |
		    7 << LW_DATA_CYCLE_SHIFT |
		    LW_ADDRESS_BIT_WIDTH_1_BIT |
		    addr_cnt |
		    0 << LW_ADDR_QUAD_MODE_SHIFT |
		    0 << LW_DATA_QUAD_MODE_SHIFT |
		    0 << LW_MODE_QUAD_MODE_SHIFT |
		    0 << LW_CMD_QUAD_MODE_SHIFT;

		smartchip_qspi_writel(aq, QSPI_RD_LW_REG, instr_lw);
		smartchip_qspi_writel(aq, QSPI_RD_HW_REG, instr_hw);
		smartchip_qspi_writel(aq, QSPI_UPDATE_RD_REG, 0x1);
	}

	if(nor->program_opcode == SPINOR_OP_BP || SPINOR_OP_PP_4B) {
		instr_hw = HW_BUS_WIDTH_RESERVED |
		    HW_TAG_AHB |
		    HW_DIR_WRITE |
		    HW_CMD_BIT_WIDTH_1_BIT |
		    nor->program_opcode << HW_CMD_SHIFT |
		    0 << HW_MODE_CYCLE_SHIFT;

		instr_lw = 0 << LW_DUMMY_CYCLE_SHIFT |
		    LW_DATA_BIT_WIDTH_1_BIT |
		    7 << LW_DATA_CYCLE_SHIFT |
		    LW_ADDRESS_BIT_WIDTH_1_BIT |
		    addr_cnt |
		    0 << LW_ADDR_QUAD_MODE_SHIFT |
		    0 << LW_DATA_QUAD_MODE_SHIFT |
		    0 << LW_MODE_QUAD_MODE_SHIFT |
		    0 << LW_CMD_QUAD_MODE_SHIFT;

		smartchip_qspi_writel(aq, QSPI_WR_LW_REG, instr_lw);
		smartchip_qspi_writel(aq, QSPI_WR_HW_REG, instr_hw);
		smartchip_qspi_writel(aq, QSPI_UPDATE_WR_REG, 0x1);
	} else {
		BUG();
	}

	/* Enable 4B in case that spi-nor driver did not enable it */
	if(nor->addr_width == 4) {
		smartchip_qspi_set_4b(nor, 1);
	} else {
		smartchip_qspi_set_4b(nor, 0);
	}

	/* Disable 4k sector for Spansion */
	if(aq->id[0] == SNOR_MFR_SPANSION) {
		smartchip_qspi_disable_4k(nor);
	}

	if (aq->id[0] == SNOR_MFR_WINBOND) {
		smartchip_qspi_drive_enhance_winbond(nor);
	}

	platform_set_drvdata(pdev, nor);

#ifdef CONFIG_SMARTCHIP_QSPI_ENHANCE_PATCH
	if (of_property_read_u32(np, "pinno", &pinno)) {
		SMARTCHIP_QSPI_ERR("No pin no in dts\n");

		mcp_mutex_release();
		return -EINVAL;
	}

	aq->pin_no = pinno;

	SMARTCHIP_QSPI_ERR("qspi pinshare pinno = %d\n", pinno);

	if (of_property_read_u32(np, "pinfunc", &pinfunc)) {
		SMARTCHIP_QSPI_ERR("No pin func in dts\n");
		mcp_mutex_release();
		return -EINVAL;
	}

	aq->pin_func = pinfunc;

	SMARTCHIP_QSPI_ERR("qspi pinshare pinfunc = %d\n", pinfunc);

	gpio = of_get_named_gpio_flags(np, "cs-gpios", 0, &flags);

	SMARTCHIP_QSPI_ERR("qspi gpio = %d, %d\n", gpio, flags);

	if (gpio_is_valid(gpio)) {
		aq->gpio = gpio;
	}

	aq->gpio = gpio;
#endif

	/* Change clk from dts setting */
	smartchip_qspi_set_clk(aq, aq->clk_rate);

	mcp_mutex_release();

	/* Register mtd device */
	err = mtd_device_register(mtd, NULL, 0);
	if (err)
		goto exit;

	part_cnt = of_get_available_child_count(np);
	if(!part_cnt) {
		return 0;
	}

	/* Add mtd partitions */
	for_each_available_child_of_node(np, node) {
		if (of_property_read_string(node, "part-name", &part_name)) {
			SMARTCHIP_QSPI_DBG("No mtd partition name\n");
			part_name = NULL;
		}

		if (of_property_read_u32(node, "part-size", &part_size)) {
			if(part_cnt != 1) {
				BUG();
			} else {
				part_size = mtd->size - part_offs;
			}
		}

		if(part_offs + part_size > mtd->size) {
			SMARTCHIP_QSPI_ERR("Mtd part %s exceed flash size\n");
			return err;
		}

		err = mtd_add_partition(mtd, part_name, part_offs, part_size);
		if(err < 0) {
			SMARTCHIP_QSPI_ERR("Add mtd part %s fail\n");
			goto exit;
		}
		part_offs += part_size;
		part_cnt--;
	}

	return 0;

exit:
	mcp_mutex_release();
	devm_kfree(&pdev->dev, aq);
	return err;
}

static int smartchip_qspi_remove(struct platform_device *pdev)
{
	struct spi_nor *nor = platform_get_drvdata(pdev);
	struct smartchip_qspi *aq = nor->priv;
	struct sc_nor_host *host = aq->host;

	nor->dev = &pdev->dev;
	dmam_free_coherent(nor->dev, PAGE_SIZE, host->temp_buf, nor->phys);
	dmam_free_coherent(nor->dev, sizeof(struct ahb_dma_llp) * AHB_DMA_MAX_LLP_ND, host->dma_llp, host->dma_buffer);

	return 0;
}

static const struct of_device_id smartchip_qspi_dt_ids[] = {
	{ .compatible = "smartchip,smartx-qspi" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, smartchip_qspi_dt_ids);

static struct platform_driver smartchip_qspi_driver = {
	.driver = {
		.name   = "smartchip,smartx-qspi",
		.of_match_table = smartchip_qspi_dt_ids,
	},
	.probe      = smartchip_qspi_probe,
	.remove     = smartchip_qspi_remove,
};

module_platform_driver(smartchip_qspi_driver);

MODULE_AUTHOR("SmartChip");
MODULE_DESCRIPTION("SmartChip QSPI Controller driver");
MODULE_LICENSE("GPL v2");

