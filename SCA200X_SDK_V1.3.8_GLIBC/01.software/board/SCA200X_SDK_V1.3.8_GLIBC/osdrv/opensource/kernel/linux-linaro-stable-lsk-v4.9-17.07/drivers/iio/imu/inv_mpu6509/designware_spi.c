// SPDX-License-Identifier: GPL-2.0
/*
 * Designware master SPI core controller driver
 *
 * Copyright (C) 2014 Stefan Roese <sr@denx.de>
 *
 * Very loosely based on the Linux driver:
 * drivers/spi/spi-dw.c, which is:
 * Copyright (c) 2009, Intel Corporation.
 *
 * Copyright (C) 2022 SmartChipc, Inc.
 */

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/iopoll.h>
#include "designware_spi.h"

/* Register offsets */
#define DW_SPI_CTRL0            0x00
#define DW_SPI_CTRL1            0x04
#define DW_SPI_SSIENR           0x08
#define DW_SPI_MWCR         0x0c
#define DW_SPI_SER          0x10
#define DW_SPI_BAUDR            0x14
#define DW_SPI_TXFLTR           0x18
#define DW_SPI_RXFLTR           0x1c
#define DW_SPI_TXFLR            0x20
#define DW_SPI_RXFLR            0x24
#define DW_SPI_SR           0x28
#define DW_SPI_IMR          0x2c
#define DW_SPI_ISR          0x30
#define DW_SPI_RISR         0x34
#define DW_SPI_TXOICR           0x38
#define DW_SPI_RXOICR           0x3c
#define DW_SPI_RXUICR           0x40
#define DW_SPI_MSTICR           0x44
#define DW_SPI_ICR          0x48
#define DW_SPI_DMACR            0x4c
#define DW_SPI_DMATDLR          0x50
#define DW_SPI_DMARDLR          0x54
#define DW_SPI_IDR          0x58
#define DW_SPI_VERSION          0x5c
#define DW_SPI_DR           0x60

/* Bit fields in CTRLR0 */
#define SPI_DFS_OFFSET          0

#define SPI_FRF_OFFSET          4
#define SPI_FRF_SPI         0x0
#define SPI_FRF_SSP         0x1
#define SPI_FRF_MICROWIRE       0x2
#define SPI_FRF_RESV            0x3

#define SPI_MODE_OFFSET         6
#define SPI_SCPH_OFFSET         6
#define SPI_SCOL_OFFSET         7

#define SPI_TMOD_OFFSET         8
#define SPI_TMOD_MASK           (0x3 << SPI_TMOD_OFFSET)
#define SPI_TMOD_TR         0x0     /* xmit & recv */
#define SPI_TMOD_TO         0x1     /* xmit only */
#define SPI_TMOD_RO         0x2     /* recv only */
#define SPI_TMOD_EPROMREAD      0x3     /* eeprom read mode */

#define SPI_SLVOE_OFFSET        10
#define SPI_SRL_OFFSET          11
#define SPI_CFS_OFFSET          12

/* Bit fields in SR, 7 bits */
#define SR_MASK             GENMASK(6, 0)   /* cover 7 bits */
#define SR_BUSY             BIT(0)
#define SR_TF_NOT_FULL          BIT(1)
#define SR_TF_EMPT          BIT(2)
#define SR_RF_NOT_EMPT          BIT(3)
#define SR_RF_FULL          BIT(4)
#define SR_TX_ERR           BIT(5)
#define SR_DCOL             BIT(6)

#define RX_TIMEOUT          1000        /* timeout in ms */

/* Bit fields in ISR, IMR, RISR, 7 bits */
#define SPI_INT_TXEI            (1 << 0)
#define SPI_INT_TXOI            (1 << 1)
#define SPI_INT_RXUI            (1 << 2)
#define SPI_INT_RXOI            (1 << 3)
#define SPI_INT_RXFI            (1 << 4)
#define SPI_INT_MSTI            (1 << 5)

struct dw_spi_priv {
	void __iomem *regs;
	unsigned int mode;      /* compatible with kernel */

	int cs_gpio;    /* External chip-select gpio */

	int bits_per_word;
	u8 cs;          /* chip select pin */
	u8 tmode;       /* TR/TO/RO/EEPROM */
	u8 type;        /* SPI/SSP/MicroWire */
	int len;

	u32 fifo_len;       /* depth of the FIFO buffer */
	void *tx;
	void *tx_end;
	void *rx;
	void *rx_end;
};

static inline u32 dw_readl(struct dw_spi_priv *priv, u32 offset)
{
	return readl(priv->regs + offset);
}

static inline void dw_writel(struct dw_spi_priv *priv, u32 offset, u32 val)
{
	writel(val, priv->regs + offset);
}

static inline void spi_enable_chip(struct dw_spi_priv *priv, int enable)
{
	dw_writel(priv, DW_SPI_SSIENR, (enable ? 1 : 0));
}

/* Return the max entries we can fill into tx fifo */
static inline u32 tx_max(struct dw_spi_priv *priv)
{
	u32 tx_left, tx_room, rxtx_gap;

	tx_left = (priv->tx_end - priv->tx) / (priv->bits_per_word >> 3);
	tx_room = priv->fifo_len - dw_readl(priv, DW_SPI_TXFLR);

	/*
	 * Another concern is about the tx/rx mismatch, we
	 * thought about using (priv->fifo_len - rxflr - txflr) as
	 * one maximum value for tx, but it doesn't cover the
	 * data which is out of tx/rx fifo and inside the
	 * shift registers. So a control from sw point of
	 * view is taken.
	 */
	rxtx_gap = ((priv->rx_end - priv->rx) - (priv->tx_end - priv->tx)) /
	    (priv->bits_per_word >> 3);

	return min3(tx_left, tx_room, (u32)(priv->fifo_len - rxtx_gap));
}

/* Return the max entries we should read out of rx fifo */
static inline u32 rx_max(struct dw_spi_priv *priv)
{
	u32 rx_left = (priv->rx_end - priv->rx) / (priv->bits_per_word >> 3);
	u32 rxflr = dw_readl(priv, DW_SPI_RXFLR);

	return min_t(u32, rx_left, rxflr);
}

static void dw_writer(struct dw_spi_priv *priv)
{
	u32 max = tx_max(priv);
	u16 txw = 0;

	while (max--) {
		/* Set the tx word if the transfer's original "tx" is not null */
		if (priv->tx_end - priv->len) {
			if (priv->bits_per_word == 8)
				txw = *(u8 *)(priv->tx);
			else
				txw = *(u16 *)(priv->tx);
		}
		dw_writel(priv, DW_SPI_DR, txw);
		priv->tx += priv->bits_per_word >> 3;
	}
}

static void dw_reader(struct dw_spi_priv *priv)
{
	u32 max = rx_max(priv);
	u16 rxw;

	while (max--) {
		rxw = dw_readl(priv, DW_SPI_DR);

		/* Care about rx if the transfer's original "rx" is not null */
		if (priv->rx_end - priv->len) {
			if (priv->bits_per_word == 8)
				*(u8 *)(priv->rx) = rxw;
			else
				*(u16 *)(priv->rx) = rxw;
		}
		priv->rx += priv->bits_per_word >> 3;
	}
}

/*
 * We define external_cs_manage function as 'weak' as some targets
 * (like MSCC Ocelot) don't control the external CS pin using a GPIO
 * controller. These SoCs use specific registers to control by
 * software the SPI pins (and especially the CS).
 */
void external_cs_manage(struct dw_spi_priv *priv, bool on)
{
	if(priv->mode & SPI_CS_HIGH)
		on = !on;

	if (!gpio_is_valid(priv->cs_gpio))
		return;

	gpio_set_value(priv->cs_gpio, on ? 1 : 0);
}

static int dw_spi_poll_transfer(struct dw_spi_priv *priv)
{
	do {
		dw_writer(priv);
		dw_reader(priv);
	} while (priv->rx_end > priv->rx);

	return 0;
}

static int dw_spi_xfer(struct dw_spi_priv *priv, unsigned int bitlen,
    const void *dout, void *din, unsigned long flags)
{
	const u8 *tx = dout;
	u8 *rx = din;
	int ret = 0;
	u32 cr0 = 0;

	/* spi core configured to do 8 bit transfers */
	if (bitlen % 8) {
		pr_debug("Non byte aligned SPI transfer.\n");
		return -1;
	}

	/* Start the transaction if necessary. */
	external_cs_manage(priv, false);

	cr0 = (priv->bits_per_word - 1) | (priv->type << SPI_FRF_OFFSET) |
	    ((priv->mode & 0x3) << SPI_MODE_OFFSET) |
	    (priv->tmode << SPI_TMOD_OFFSET);

	if (rx && tx)
		priv->tmode = SPI_TMOD_TR;
	else if (rx)
		priv->tmode = SPI_TMOD_RO;
	else
		/*
		 * In transmit only mode (SPI_TMOD_TO) input FIFO never gets
		 * any data which breaks our logic in poll_transfer() above.
		 */
		priv->tmode = SPI_TMOD_TR;

	cr0 &= ~SPI_TMOD_MASK;
	cr0 |= (priv->tmode << SPI_TMOD_OFFSET);

	priv->len = bitlen >> 3;
	pr_debug("%s: rx=%p tx=%p len=%d [bytes]\n", __func__, rx, tx, priv->len);

	priv->tx = (void *)tx;
	priv->tx_end = priv->tx;
	if(tx) {
		priv->tx_end += priv->len;
	}
	priv->rx = (void *)rx;
	priv->rx_end = priv->rx;
	if(rx) {
		priv->rx_end += priv->len;
	}

	/* Disable controller before writing control registers */
	spi_enable_chip(priv, 0);

	pr_debug("%s: cr0=%08x\n", __func__, cr0);
	/* Reprogram cr0 only if changed */
	if (dw_readl(priv, DW_SPI_CTRL0) != cr0)
		dw_writel(priv, DW_SPI_CTRL0, cr0);

	dw_writel(priv, DW_SPI_CTRL1, 0);
	/*
	 * Configure the desired SS (slave select 0...3) in the controller
	 * The DW SPI controller will activate and deactivate this CS
	 * automatically. So no cs_activate() etc is needed in this driver.
	 */
	dw_writel(priv, DW_SPI_SER, 1 << priv->cs);

	/* Enable controller after writing control registers */
	spi_enable_chip(priv, 1);

	ret = dw_spi_poll_transfer(priv);

	/* Stop the transaction if necessary */
	dw_writel(priv, DW_SPI_SER, 0);
	external_cs_manage(priv, true);

	return ret;
}

static struct dw_spi_priv spi_priv;
#define SPI1_REG_BASE      0x08140000
#define SPI1_REG_MAP_LEN    0x40000

int dw_spi_reg_read(unsigned int reg, unsigned int *val)
{
	int ret;
	u8 input[2] = {(u8)reg | 0x80, 0x80};
	u8 output[2] = {0, 0};

	ret = dw_spi_xfer(&spi_priv, 16, input, output, 0);
	*val = output[1];

	return ret;
}
EXPORT_SYMBOL_GPL(dw_spi_reg_read);

int dw_spi_init(int gpio, int cs)
{
	spi_priv.regs = ioremap(SPI1_REG_BASE, SPI1_REG_MAP_LEN);
	pr_debug("i2c2_reg_base = %p", spi_priv.regs);

	/* Currently only bits_per_word == 8 supported */
	spi_priv.bits_per_word = 8;

	spi_priv.mode = 0;
	spi_priv.type = 0;
	spi_priv.tmode = 0; /* Tx & Rx */
	spi_priv.cs_gpio = gpio;
	spi_priv.cs = cs;
	spi_priv.fifo_len = 16;

	return 0;
}
EXPORT_SYMBOL_GPL(dw_spi_init);

int dw_spi_remove(void)
{
	iounmap(spi_priv.regs);
	return 0;
}
EXPORT_SYMBOL_GPL(dw_spi_remove);

