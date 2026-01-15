// SPDX-License-Identifier: GPL-2.0-only
/*
 * Special handling for DW DMA core
 *
 * Copyright (c) 2009, 2014 Intel Corporation.
 */

#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/irqreturn.h>
#include <linux/jiffies.h>
#include <linux/pci.h>
#include <linux/platform_data/dma-dw.h>
#include <linux/spi/spi.h>
#include <linux/types.h>

#include "spi-dw.h"

#define RX_BUSY     0
#define RX_BURST_LEVEL  16
#define TX_BUSY     1
#define TX_BURST_LEVEL  16

static bool dw_spi_dma_chan_filter(struct dma_chan *chan, void *param)
{
	struct dw_dma_slave *s = param;

	if (s->dma_dev != chan->device->dev)
		return false;

	chan->private = s;
	return true;
}

static void dw_spi_dma_maxburst_init(struct dw_spi *dws)
{
	struct dma_slave_caps caps;
	u32 max_burst, def_burst;
	int ret;

	def_burst = dws->fifo_len / 2;

	ret = dma_get_slave_caps(dws->rxchan, &caps);
	if (!ret && caps.max_burst)
		max_burst = caps.max_burst;
	else
		max_burst = RX_BURST_LEVEL;

	dws->rxburst = min(max_burst, def_burst);
	dw_writel(dws, DW_SPI_DMARDLR, dws->rxburst - 1);

	ret = dma_get_slave_caps(dws->txchan, &caps);
	if (!ret && caps.max_burst)
		max_burst = caps.max_burst;
	else
		max_burst = TX_BURST_LEVEL;

	/*
	 * Having a Rx DMA channel serviced with higher priority than a Tx DMA
	 * channel might not be enough to provide a well balanced DMA-based
	 * SPI transfer interface. There might still be moments when the Tx DMA
	 * channel is occasionally handled faster than the Rx DMA channel.
	 * That in its turn will eventually cause the SPI Rx FIFO overflow if
	 * SPI bus speed is high enough to fill the SPI Rx FIFO in before it's
	 * cleared by the Rx DMA channel. In order to fix the problem the Tx
	 * DMA activity is intentionally slowed down by limiting the SPI Tx
	 * FIFO depth with a value twice bigger than the Tx burst length.
	 */
	dws->txburst = min(max_burst, def_burst);
	dw_writel(dws, DW_SPI_DMATDLR, dws->txburst);
}

/*static void dw_spi_dma_sg_burst_init(struct dw_spi *dws)
{
    struct dma_slave_caps tx = {0}, rx = {0};

    dma_get_slave_caps(dws->txchan, &tx);
    dma_get_slave_caps(dws->rxchan, &rx);

    if (tx.max_sg_burst > 0 && rx.max_sg_burst > 0)
        dws->dma_sg_burst = min(tx.max_sg_burst, rx.max_sg_burst);
    else if (tx.max_sg_burst > 0)
        dws->dma_sg_burst = tx.max_sg_burst;
    else if (rx.max_sg_burst > 0)
        dws->dma_sg_burst = rx.max_sg_burst;
    else
        dws->dma_sg_burst = 0;
}*/

static int dw_spi_dma_init_generic(struct device *dev, struct dw_spi *dws)
{
	dws->rxchan = dma_request_slave_channel(dev, "rx");
	if (!dws->rxchan)
		return -ENODEV;

	dws->txchan = dma_request_slave_channel(dev, "tx");
	if (!dws->txchan) {
		dma_release_channel(dws->rxchan);
		dws->rxchan = NULL;
		return -ENODEV;
	}

	dws->master->dma_rx = dws->rxchan;
	dws->master->dma_tx = dws->txchan;

	init_completion(&dws->dma_completion);

	dw_spi_dma_maxburst_init(dws);

	//dw_spi_dma_sg_burst_init(dws);

	return 0;
}

static void dw_spi_dma_exit(struct dw_spi *dws)
{
	if (dws->txchan) {
		dmaengine_terminate_sync(dws->txchan);
		dma_release_channel(dws->txchan);
	}

	if (dws->rxchan) {
		dmaengine_terminate_sync(dws->rxchan);
		dma_release_channel(dws->rxchan);
	}
}

static irqreturn_t dw_spi_dma_transfer_handler(struct dw_spi *dws)
{
	dw_spi_check_status(dws, false);

	complete(&dws->dma_completion);

	return IRQ_HANDLED;
}

static bool dw_spi_can_dma(struct spi_master *master,
    struct spi_device *spi, struct spi_transfer *xfer)
{
	struct dw_spi *dws = spi_master_get_devdata(master);

	return xfer->len > dws->fifo_len;
}

static enum dma_slave_buswidth dw_spi_dma_convert_width(u8 n_bytes)
{
	if (n_bytes == 1)
		return DMA_SLAVE_BUSWIDTH_1_BYTE;
	else if (n_bytes == 2)
		return DMA_SLAVE_BUSWIDTH_2_BYTES;

	return DMA_SLAVE_BUSWIDTH_UNDEFINED;
}

static int dw_spi_dma_wait(struct dw_spi *dws, unsigned int len, u32 speed)
{
	unsigned long long ms;

	ms = len * MSEC_PER_SEC * BITS_PER_BYTE;
	do_div(ms, speed);
	ms += ms + 200;

	if (ms > UINT_MAX)
		ms = UINT_MAX;

	ms = wait_for_completion_timeout(&dws->dma_completion,
	        msecs_to_jiffies(ms));

	if (ms == 0) {
		dev_err(&dws->master->cur_msg->spi->dev,
		    "DMA transaction timed out\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static inline bool dw_spi_dma_tx_busy(struct dw_spi *dws)
{
	return !(dw_readl(dws, DW_SPI_SR) & SR_TF_EMPT);
}

static int dw_spi_dma_wait_tx_done(struct dw_spi *dws,
    struct spi_transfer *xfer)
{
	int retry = SPI_WAIT_RETRIES;
	struct spi_delay delay;
	u32 nents;

	nents = dw_readl(dws, DW_SPI_TXFLR);
	delay.unit = SPI_DELAY_UNIT_SCK;
	delay.value = nents * dws->n_bytes * BITS_PER_BYTE;

	while (dw_spi_dma_tx_busy(dws) && retry--)
		spi_delay_exec(&delay, xfer);

	if (retry < 0) {
		dev_err(&dws->master->dev, "Tx hanged up\n");
		return -EIO;
	}

	return 0;
}

/*
 * dws->dma_chan_busy is set before the dma transfer starts, callback for tx
 * channel will clear a corresponding bit.
 */
static void dw_spi_dma_tx_done(void *arg)
{
	struct dw_spi *dws = arg;

	clear_bit(TX_BUSY, &dws->dma_chan_busy);
	if (test_bit(RX_BUSY, &dws->dma_chan_busy))
		return;

	complete(&dws->dma_completion);
}

static int dw_spi_dma_config_tx(struct dw_spi *dws)
{
	struct dma_slave_config txconf;

	memset(&txconf, 0, sizeof(txconf));
	txconf.direction = DMA_MEM_TO_DEV;
	txconf.dst_addr = dws->dma_addr;
	txconf.dst_maxburst = dws->txburst;
	txconf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	txconf.dst_addr_width = dw_spi_dma_convert_width(dws->n_bytes);
	txconf.device_fc = false;

	return dmaengine_slave_config(dws->txchan, &txconf);
}

static int dw_spi_dma_submit_tx(struct dw_spi *dws, struct scatterlist *sgl,
    unsigned int nents)
{
	struct dma_async_tx_descriptor *txdesc;
	dma_cookie_t cookie;
	int ret;

	txdesc = dmaengine_prep_slave_sg(dws->txchan, sgl, nents,
	        DMA_MEM_TO_DEV,
	        DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!txdesc)
		return -ENOMEM;

	txdesc->callback = dw_spi_dma_tx_done;
	txdesc->callback_param = dws;

	cookie = dmaengine_submit(txdesc);
	ret = dma_submit_error(cookie);
	if (ret) {
		dmaengine_terminate_sync(dws->txchan);
		return ret;
	}

	set_bit(TX_BUSY, &dws->dma_chan_busy);

	return 0;
}

static inline bool dw_spi_dma_rx_busy(struct dw_spi *dws)
{
	return !!(dw_readl(dws, DW_SPI_SR) & SR_RF_NOT_EMPT);
}

static int dw_spi_dma_wait_rx_done(struct dw_spi *dws)
{
	int retry = SPI_WAIT_RETRIES;
	struct spi_delay delay;
	unsigned long ns, us;
	u32 nents;

	/*
	 * It's unlikely that DMA engine is still doing the data fetching, but
	 * if it's let's give it some reasonable time. The timeout calculation
	 * is based on the synchronous APB/SSI reference clock rate, on a
	 * number of data entries left in the Rx FIFO, times a number of clock
	 * periods normally needed for a single APB read/write transaction
	 * without PREADY signal utilized (which is true for the DW APB SSI
	 * controller).
	 */
	nents = dw_readl(dws, DW_SPI_RXFLR);
	ns = 4U * NSEC_PER_SEC / dws->max_freq * nents;
	if (ns <= NSEC_PER_USEC) {
		delay.unit = SPI_DELAY_UNIT_NSECS;
		delay.value = ns;
	} else {
		us = DIV_ROUND_UP(ns, NSEC_PER_USEC);
		delay.unit = SPI_DELAY_UNIT_USECS;
		delay.value = clamp_val(us, 0, USHRT_MAX);
	}

	while (dw_spi_dma_rx_busy(dws) && retry--)
		spi_delay_exec(&delay, NULL);

	if (retry < 0) {
		dev_err(&dws->master->dev, "Rx hanged up\n");
		return -EIO;
	}

	return 0;
}

/*
 * dws->dma_chan_busy is set before the dma transfer starts, callback for rx
 * channel will clear a corresponding bit.
 */
static void dw_spi_dma_rx_done(void *arg)
{
	struct dw_spi *dws = arg;

	clear_bit(RX_BUSY, &dws->dma_chan_busy);
	if (test_bit(TX_BUSY, &dws->dma_chan_busy))
		return;

	complete(&dws->dma_completion);
}

static int dw_spi_dma_config_rx(struct dw_spi *dws)
{
	struct dma_slave_config rxconf;

	memset(&rxconf, 0, sizeof(rxconf));
	rxconf.direction = DMA_DEV_TO_MEM;
	rxconf.src_addr = dws->dma_addr;
	rxconf.src_maxburst = dws->rxburst;
	rxconf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	rxconf.src_addr_width = dw_spi_dma_convert_width(dws->n_bytes);
	rxconf.device_fc = false;

	return dmaengine_slave_config(dws->rxchan, &rxconf);
}

static int dw_spi_dma_submit_rx(struct dw_spi *dws, struct scatterlist *sgl,
    unsigned int nents)
{
	struct dma_async_tx_descriptor *rxdesc;
	dma_cookie_t cookie;
	int ret;

	rxdesc = dmaengine_prep_slave_sg(dws->rxchan, sgl, nents,
	        DMA_DEV_TO_MEM,
	        DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!rxdesc)
		return -ENOMEM;

	rxdesc->callback = dw_spi_dma_rx_done;
	rxdesc->callback_param = dws;

	cookie = dmaengine_submit(rxdesc);
	ret = dma_submit_error(cookie);
	if (ret) {
		dmaengine_terminate_sync(dws->rxchan);
		return ret;
	}

	set_bit(RX_BUSY, &dws->dma_chan_busy);

	return 0;
}

static int dw_spi_dma_setup(struct dw_spi *dws, struct spi_transfer *xfer)
{
	u16 imr, dma_ctrl;
	int ret;

	if (!xfer->tx_buf)
		return -EINVAL;

	/* Setup DMA channels */
	ret = dw_spi_dma_config_tx(dws);
	if (ret)
		return ret;

	if (xfer->rx_buf) {
		ret = dw_spi_dma_config_rx(dws);
		if (ret)
			return ret;
	}

	/* Set the DMA handshaking interface */
	dma_ctrl = SPI_DMA_TDMAE;
	if (xfer->rx_buf)
		dma_ctrl |= SPI_DMA_RDMAE;
	dw_writel(dws, DW_SPI_DMACR, dma_ctrl);

	/* Set the interrupt mask */
	imr = SPI_INT_TXOI;
	if (xfer->rx_buf)
		imr |= SPI_INT_RXUI | SPI_INT_RXOI;
	spi_umask_intr(dws, imr);

	reinit_completion(&dws->dma_completion);

	dws->transfer_handler = dw_spi_dma_transfer_handler;

	return 0;
}

static int dw_spi_dma_transfer_all(struct dw_spi *dws,
    struct spi_transfer *xfer)
{
	int ret;

	/* Submit the DMA Tx transfer */
	ret = dw_spi_dma_submit_tx(dws, xfer->tx_sg.sgl, xfer->tx_sg.nents);
	if (ret)
		goto err_clear_dmac;

	/* Submit the DMA Rx transfer if required */
	if (xfer->rx_buf) {
		ret = dw_spi_dma_submit_rx(dws, xfer->rx_sg.sgl,
		        xfer->rx_sg.nents);
		if (ret)
			goto err_clear_dmac;

		/* rx must be started before tx due to spi instinct */
		dma_async_issue_pending(dws->rxchan);
	}

	dma_async_issue_pending(dws->txchan);

	ret = dw_spi_dma_wait(dws, xfer->len, xfer->effective_speed_hz);

err_clear_dmac:
	dw_writel(dws, DW_SPI_DMACR, 0);

	return ret;
}

/*
 * In case if at least one of the requested DMA channels doesn't support the
 * hardware accelerated SG list entries traverse, the DMA driver will most
 * likely work that around by performing the IRQ-based SG list entries
 * resubmission. That might and will cause a problem if the DMA Tx channel is
 * recharged and re-executed before the Rx DMA channel. Due to
 * non-deterministic IRQ-handler execution latency the DMA Tx channel will
 * start pushing data to the SPI bus before the Rx DMA channel is even
 * reinitialized with the next inbound SG list entry. By doing so the DMA Tx
 * channel will implicitly start filling the DW APB SSI Rx FIFO up, which while
 * the DMA Rx channel being recharged and re-executed will eventually be
 * overflown.
 *
 * In order to solve the problem we have to feed the DMA engine with SG list
 * entries one-by-one. It shall keep the DW APB SSI Tx and Rx FIFOs
 * synchronized and prevent the Rx FIFO overflow. Since in general the tx_sg
 * and rx_sg lists may have different number of entries of different lengths
 * (though total length should match) let's virtually split the SG-lists to the
 * set of DMA transfers, which length is a minimum of the ordered SG-entries
 * lengths. An ASCII-sketch of the implemented algo is following:
 *                  xfer->len
 *                |___________|
 * tx_sg list:    |___|____|__|
 * rx_sg list:    |_|____|____|
 * DMA transfers: |_|_|__|_|__|
 *
 * Note in order to have this workaround solving the denoted problem the DMA
 * engine driver should properly initialize the max_sg_burst capability and set
 * the DMA device max segment size parameter with maximum data block size the
 * DMA engine supports.
 */

static int dw_spi_dma_transfer_one(struct dw_spi *dws,
    struct spi_transfer *xfer)
{
	struct scatterlist *tx_sg = NULL, *rx_sg = NULL, tx_tmp, rx_tmp;
	unsigned int tx_len = 0, rx_len = 0;
	unsigned int base, len;
	int ret;

	sg_init_table(&tx_tmp, 1);
	sg_init_table(&rx_tmp, 1);

	for (base = 0, len = 0; base < xfer->len; base += len) {
		/* Fetch next Tx DMA data chunk */
		if (!tx_len) {
			tx_sg = !tx_sg ? &xfer->tx_sg.sgl[0] : sg_next(tx_sg);
			sg_dma_address(&tx_tmp) = sg_dma_address(tx_sg);
			tx_len = sg_dma_len(tx_sg);
		}

		/* Fetch next Rx DMA data chunk */
		if (!rx_len) {
			rx_sg = !rx_sg ? &xfer->rx_sg.sgl[0] : sg_next(rx_sg);
			sg_dma_address(&rx_tmp) = sg_dma_address(rx_sg);
			rx_len = sg_dma_len(rx_sg);
		}

		len = min(tx_len, rx_len);

		sg_dma_len(&tx_tmp) = len;
		sg_dma_len(&rx_tmp) = len;

		/* Submit DMA Tx transfer */
		ret = dw_spi_dma_submit_tx(dws, &tx_tmp, 1);
		if (ret)
			break;

		/* Submit DMA Rx transfer */
		ret = dw_spi_dma_submit_rx(dws, &rx_tmp, 1);
		if (ret)
			break;

		/* Rx must be started before Tx due to SPI instinct */
		dma_async_issue_pending(dws->rxchan);

		dma_async_issue_pending(dws->txchan);

		/*
		 * Here we only need to wait for the DMA transfer to be
		 * finished since SPI controller is kept enabled during the
		 * procedure this loop implements and there is no risk to lose
		 * data left in the Tx/Rx FIFOs.
		 */
		ret = dw_spi_dma_wait(dws, len, xfer->effective_speed_hz);
		if (ret)
			break;

		reinit_completion(&dws->dma_completion);

		sg_dma_address(&tx_tmp) += len;
		sg_dma_address(&rx_tmp) += len;
		tx_len -= len;
		rx_len -= len;
	}

	dw_writel(dws, DW_SPI_DMACR, 0);

	return ret;
}

static int dw_spi_dma_transfer(struct dw_spi *dws, struct spi_transfer *xfer)
{
	unsigned int nents;
	int ret;

	nents = max(xfer->tx_sg.nents, xfer->rx_sg.nents);

	/*
	 * Execute normal DMA-based transfer (which submits the Rx and Tx SG
	 * lists directly to the DMA engine at once) if either full hardware
	 * accelerated SG list traverse is supported by both channels, or the
	 * Tx-only SPI transfer is requested, or the DMA engine is capable to
	 * handle both SG lists on hardware accelerated basis.
	 */
	if (!dws->dma_sg_burst || !xfer->rx_buf || nents <= dws->dma_sg_burst)
		ret = dw_spi_dma_transfer_all(dws, xfer);
	else
		ret = dw_spi_dma_transfer_one(dws, xfer);
	if (ret)
		return ret;

	if (dws->master->cur_msg->status == -EINPROGRESS) {
		ret = dw_spi_dma_wait_tx_done(dws, xfer);
		if (ret)
			return ret;
	}

	if (xfer->rx_buf && dws->master->cur_msg->status == -EINPROGRESS)
		ret = dw_spi_dma_wait_rx_done(dws);

	return ret;
}

static void dw_spi_dma_stop(struct dw_spi *dws)
{
	if (test_bit(TX_BUSY, &dws->dma_chan_busy)) {
		dmaengine_terminate_sync(dws->txchan);
		clear_bit(TX_BUSY, &dws->dma_chan_busy);
	}
	if (test_bit(RX_BUSY, &dws->dma_chan_busy)) {
		dmaengine_terminate_sync(dws->rxchan);
		clear_bit(RX_BUSY, &dws->dma_chan_busy);
	}
}

static const struct dw_spi_dma_ops dw_spi_dma_generic_ops = {
	.dma_init   = dw_spi_dma_init_generic,
	.dma_exit   = dw_spi_dma_exit,
	.dma_setup  = dw_spi_dma_setup,
	.can_dma    = dw_spi_can_dma,
	.dma_transfer   = dw_spi_dma_transfer,
	.dma_stop   = dw_spi_dma_stop,
};

void dw_spi_dma_setup_generic(struct dw_spi *dws)
{
	dws->dma_ops = &dw_spi_dma_generic_ops;
}
EXPORT_SYMBOL_GPL(dw_spi_dma_setup_generic);

#if 0
/*
 * Special handling for DW core on Intel MID platform
 *
 * Copyright (c) 2009, 2014 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>

#include "spi-dw.h"

#include <linux/platform_data/dma-dw.h>

#define RX_BUSY     0
#define TX_BUSY     1

static struct dw_dma_slave dma_tx = { .dst_id = 0 };
static struct dw_dma_slave dma_rx = { .src_id = 0 };

static unsigned int m2p_dma_hs[] = {4, 15, 6, 15, 14, 9};
static unsigned int p2m_dma_hs[] = {5, 14, 7, 14, 15, 8};

#define SPI_DMA_CHANNEL_USED_MAP 0xf3 //chan 2-3 is allowed in linux
unsigned char chan_bitmap = SPI_DMA_CHANNEL_USED_MAP;

static bool spi_dma_chan_filter(struct dma_chan *chan, void *param)
{
	unsigned char *chan_bitmap = (unsigned char *)param;
	if ((1 << chan->chan_id) & (*chan_bitmap)) {
		return false;
	}

	(*chan_bitmap) |= (1 << chan->chan_id);
	return true;
}

static int spi_dma_init(struct dw_spi *dws)
{
	dma_cap_mask_t mask;

	/*
	 * Get pci device for DMA controller, currently it could only
	 * be the DMA controller of Medfield
	 */

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	/* 1. Init rx channel */
	dws->rxchan = dma_request_channel(mask, spi_dma_chan_filter, &chan_bitmap);
	if (!dws->rxchan)
		goto err_exit;
	dws->master->dma_rx = dws->rxchan;

	/* 2. Init tx channel */
	dws->txchan = dma_request_channel(mask, spi_dma_chan_filter, &chan_bitmap);
	if (!dws->txchan)
		goto free_rxchan;
	dws->master->dma_tx = dws->txchan;

	dws->dma_inited = 1;
	return 0;

free_rxchan:
	dma_release_channel(dws->rxchan);
err_exit:
	return -EBUSY;
}

static void spi_dma_exit(struct dw_spi *dws)
{
	if (!dws->dma_inited)
		return;

	dmaengine_terminate_sync(dws->txchan);
	dma_release_channel(dws->txchan);

	dmaengine_terminate_sync(dws->rxchan);
	dma_release_channel(dws->rxchan);
}

static irqreturn_t dma_transfer(struct dw_spi *dws)
{
	u16 irq_status = dw_readl(dws, DW_SPI_ISR);

	if (!irq_status)
		return IRQ_NONE;

	dw_readl(dws, DW_SPI_ICR);
	spi_reset_chip(dws);

	printk(KERN_EMERG
	    "%s: FIFO overrun/underrun, irq_status %d, txlen %d, rxlen %d, tx[0] %d, tx[1] %d, tx[2] %d, tx[3] %d\n"
	    , __func__, irq_status, dws->tx_len, dws->rx_len, *(u8 *)(dws->tx), *(u8 *)(dws->tx + 1), *(u8 *)(dws->tx + 2),
	    *(u8 *)(dws->tx + 3));
	//dev_err(&dws->master->dev, "%s: FIFO overrun/underrun, irq_status %d, txlen %d, rxlen %d, tx[0] %d, tx[1] %d, tx[2] %d, tx[3] %d\n"
	//  , __func__, irq_status, dws->tx_len, dws->rx_len, *(u8*)(dws->tx), *(u8*)(dws->tx + 1), *(u8*)(dws->tx + 2), *(u8*)(dws->tx + 3));
	dws->master->cur_msg->status = -EIO;
	spi_finalize_current_transfer(dws->master);
	return IRQ_HANDLED;
}

static bool spi_can_dma(struct spi_master *master, struct spi_device *spi,
    struct spi_transfer *xfer)
{
	struct dw_spi *dws = spi_master_get_devdata(master);

	if (!dws->dma_inited)
		return false;

	return xfer->len > dws->fifo_len || xfer->tx_len > dws->fifo_len || xfer->rx_len > dws->fifo_len;
}

static enum dma_slave_buswidth convert_dma_width(u32 dma_width)
{
	if (dma_width == 1)
		return DMA_SLAVE_BUSWIDTH_1_BYTE;
	else if (dma_width == 2)
		return DMA_SLAVE_BUSWIDTH_2_BYTES;

	return DMA_SLAVE_BUSWIDTH_UNDEFINED;
}

/*
 * dws->dma_chan_busy is set before the dma transfer starts, callback for tx
 * channel will clear a corresponding bit.
 */
static void dw_spi_dma_tx_done(void *arg)
{
	struct dw_spi *dws = arg;
	volatile u32 middle = 1;

	clear_bit(TX_BUSY, &dws->dma_chan_busy);
	if (test_bit(RX_BUSY, &dws->dma_chan_busy))
		return;
	//printk(KERN_EMERG "dw_spi_dma_tx_done\n");

	middle = dw_readl(dws, DW_SPI_TXFLR);

	/* wait for tx fifo empty */
	while(middle) {
		cpu_relax();
		//usleep_range(middle, middle);
		middle = dw_readl(dws, DW_SPI_TXFLR);
	}
	spi_finalize_current_transfer(dws->master);
}

static struct dma_async_tx_descriptor *dw_spi_dma_prepare_tx(struct dw_spi *dws,
    struct spi_transfer *xfer)
{
	struct dma_slave_config txconf;
	struct dma_async_tx_descriptor *txdesc;

	if (!xfer->tx_buf || (xfer->tx_len < dws->fifo_len
	        && xfer->len < dws->fifo_len))//txbuf null or eeprom mode never use tx dma
		return NULL;
	//printk(KERN_DEBUG "dw_spi_dma_prepare_tx\n");

	txconf.direction = DMA_MEM_TO_DEV;
	txconf.dst_addr = dws->dma_addr;
	txconf.dst_maxburst = 0x8;
	txconf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	//txconf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	txconf.dst_addr_width = convert_dma_width(dws->dma_width);
	txconf.device_fc = false;
#ifdef SCA200V100
	txconf.hs_src = dws->txchan->chan_id * 2;
	txconf.hs_dst = txconf.hs_src + 1;
#else
	txconf.hs_dst =  m2p_dma_hs[dws->component];
#endif
	txconf.bit_time_ns = (1000000000ul) / dws->current_freq;

#ifdef SCA200V100
	if (1 == dws->component) {
		dw_writew_hs(dws, txconf.hs_src, HS_NONE | (HS_SPI_MST1_TX << 8));
	}
	if (4 == dws->component) {
		dw_writew_hs(dws, txconf.hs_src, HS_NONE | (HS_SPI_SLV0_TX << 8));
	}
	if (5 == dws->component) {
		dw_writew_hs(dws, txconf.hs_src, HS_NONE | (HS_SPI_SLV1_TX << 8));
	}
#else
	if (1 == dws->component) {
		dw_writel_hs(dws, DMA_HS_SEL_OFFSET, (dw_readl_hs(dws, DMA_HS_SEL_OFFSET) & ~ 0xf0) | (((uint32_t)0x3) << 4));
	}
	if (3 == dws->component) {
		dw_writel_hs(dws, DMA_HS_SEL_OFFSET, (dw_readl_hs(dws, DMA_HS_SEL_OFFSET) & ~ 0xf0) | (((uint32_t)0x4) << 4));
	}
	if (4 == dws->component) {
		dw_writel_hs(dws, DMA_HS_SEL_OFFSET, (dw_readl_hs(dws, DMA_HS_SEL_OFFSET) & ~ 0xf) | 0x0);
	}
	if (5 == dws->component) {
		dw_writel_hs(dws, DMA_HS_SEL_OFFSET, (dw_readl_hs(dws, DMA_HS_SEL_OFFSET) & ~ (0x01 << 11)) | (0x01 << 11));
	}
#endif
	dmaengine_slave_config(dws->txchan, &txconf);

	txdesc = dmaengine_prep_slave_sg(dws->txchan,
	        xfer->tx_sg.sgl,
	        xfer->tx_sg.nents,
	        DMA_MEM_TO_DEV,
	        DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!txdesc)
		return NULL;

	txdesc->callback = dw_spi_dma_tx_done;
	txdesc->callback_param = dws;

	return txdesc;
}

/*
 * dws->dma_chan_busy is set before the dma transfer starts, callback for rx
 * channel will clear a corresponding bit.
 */
static void dw_spi_dma_rx_done(void *arg)
{
	struct dw_spi *dws = arg;

	clear_bit(RX_BUSY, &dws->dma_chan_busy);
	if (test_bit(TX_BUSY, &dws->dma_chan_busy))
		return;

	//printk(KERN_EMERG "dw_spi_dma_rx_done\n");
	spi_finalize_current_transfer(dws->master);
}

static struct dma_async_tx_descriptor *dw_spi_dma_prepare_rx(struct dw_spi *dws,
    struct spi_transfer *xfer)
{
	struct dma_slave_config rxconf;
	struct dma_async_tx_descriptor *rxdesc;
	if (!xfer->rx_buf || (!xfer->rx_len && !xfer->len))
		return NULL;
	//printk(KERN_EMERG "dw_spi_dma_prepare_rx\n");

	rxconf.direction = DMA_DEV_TO_MEM;
	rxconf.src_addr = dws->dma_addr;
	rxconf.src_maxburst = 0x8;
	rxconf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	//rxconf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rxconf.src_addr_width = convert_dma_width(dws->dma_width);
	rxconf.device_fc = false;
#ifdef SCA200V100
	rxconf.hs_src = dws->rxchan->chan_id * 2;
	rxconf.hs_dst = rxconf.hs_src + 1;
#else
	rxconf.hs_src = p2m_dma_hs[dws->component];
#endif
	rxconf.bit_time_ns = (1000000000ul) / dws->current_freq;

#ifdef SCA200V100
	if (1 == dws->component) {
		dw_writew_hs(dws, rxconf.hs_src, HS_SPI_MST1_RX | (HS_NONE << 8));
	}
	if (4 == dws->component) {
		dw_writew_hs(dws, rxconf.hs_src, HS_SPI_SLV0_RX | (HS_NONE << 8));
	}
	if (5 == dws->component) {
		dw_writew_hs(dws, rxconf.hs_src, HS_SPI_SLV1_RX | (HS_NONE << 8));
	}
#else
	if (1 == dws->component) {
		dw_writel_hs(dws, DMA_HS_SEL_OFFSET, (dw_readl_hs(dws, DMA_HS_SEL_OFFSET) & ~ 0xf) | 0x3);
	}
	if (3 == dws->component) {
		dw_writel_hs(dws, DMA_HS_SEL_OFFSET, (dw_readl_hs(dws, DMA_HS_SEL_OFFSET) & ~ 0xf) | 0x4);
	}
	if (4 == dws->component) {
		dw_writel_hs(dws, DMA_HS_SEL_OFFSET, (dw_readl_hs(dws, DMA_HS_SEL_OFFSET) & ~ 0xf0) | 0x0);
	}
	if (5 == dws->component) {
		dw_writel_hs(dws, DMA_HS_SEL_OFFSET, (dw_readl_hs(dws, DMA_HS_SEL_OFFSET) & ~ (0x1 << 10)) | (0x01 << 10));
	}
#endif

	dmaengine_slave_config(dws->rxchan, &rxconf);

	rxdesc = dmaengine_prep_slave_sg(dws->rxchan,
	        xfer->rx_sg.sgl,
	        xfer->rx_sg.nents,
	        DMA_DEV_TO_MEM,
	        DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!rxdesc)
		return NULL;

	rxdesc->callback = dw_spi_dma_rx_done;
	rxdesc->callback_param = dws;

	return rxdesc;
}

static int spi_dma_setup(struct dw_spi *dws, struct spi_transfer *xfer)
{
	u16 dma_ctrl = 0;

	dw_writel(dws, DW_SPI_DMARDLR, 0x7);
	dw_writel(dws, DW_SPI_DMATDLR, 0x8);

	if (xfer->rx_buf)
		dma_ctrl |= SPI_DMA_RDMAE;

	if(xfer->tx_buf && (xfer->tx_len > dws->fifo_len || xfer->len > dws->fifo_len))//eeprom mode, tx never use dma
		dma_ctrl |= SPI_DMA_TDMAE;

	dw_writel(dws, DW_SPI_DMACR, dma_ctrl);

	//printk(KERN_EMERG "dmacr 0x%08x\n", dw_readl(dws, DW_SPI_DMACR));

	/* Set the interrupt mask */
	spi_umask_intr(dws, SPI_INT_TXOI | SPI_INT_RXUI | SPI_INT_RXOI);

	dws->transfer_handler = dma_transfer;

	return 0;
}

static int spi_dma_transfer(struct dw_spi *dws, struct spi_transfer *xfer)
{
	struct dma_async_tx_descriptor *txdesc, *rxdesc;

	/* Prepare the TX dma transfer */
	txdesc = dw_spi_dma_prepare_tx(dws, xfer);

	/* Prepare the RX dma transfer */
	rxdesc = dw_spi_dma_prepare_rx(dws, xfer);
	/* rx must be started before tx due to spi instinct */
	if (rxdesc) {
		/*
		if(dws->rx_len == 4096){
		    printk(KERN_EMERG "---------------------SPI REGS-------------------:\n");
		    printk(KERN_EMERG "DW_SPI_CTRL0: 0x%08x\n", dw_readl(dws, DW_SPI_CTRL0));
		    printk(KERN_EMERG "DW_SPI_CTRL1: 0x%08x\n", dw_readl(dws, DW_SPI_CTRL1));
		    printk(KERN_EMERG "DW_SPI_SSIENR: 0x%08x\n", dw_readl(dws, DW_SPI_SSIENR));
		    printk(KERN_EMERG "DW_SPI_MWCR: 0x%08x\n", dw_readl(dws, DW_SPI_MWCR));
		    printk(KERN_EMERG "DW_SPI_SER: 0x%08x\n", dw_readl(dws, DW_SPI_SER));
		    printk(KERN_EMERG "DW_SPI_BAUDR: 0x%08x\n", dw_readl(dws, DW_SPI_BAUDR));
		    printk(KERN_EMERG "DW_SPI_TXFLTR: 0x%08x\n", dw_readl(dws, DW_SPI_TXFLTR));
		    printk(KERN_EMERG "DW_SPI_RXFLTR: 0x%08x\n", dw_readl(dws, DW_SPI_RXFLTR));
		    printk(KERN_EMERG "DW_SPI_TXFTR: 0x%08x\n", dw_readl(dws, DW_SPI_TXFLR));
		    printk(KERN_EMERG "DW_SPI_RXFTR: 0x%08x\n", dw_readl(dws, DW_SPI_RXFLR));
		    printk(KERN_EMERG "DW_SPI_SR: 0x%08x\n", dw_readl(dws, DW_SPI_SR));
		    printk(KERN_EMERG "DW_SPI_IMR: 0x%08x\n", dw_readl(dws, DW_SPI_IMR));
		    printk(KERN_EMERG "DW_SPI_ISR: 0x%08x\n", dw_readl(dws, DW_SPI_ISR));
		    printk(KERN_EMERG "DW_SPI_RISR: 0x%08x\n", dw_readl(dws, DW_SPI_RISR));
		    printk(KERN_EMERG "DW_SPI_DMACR: 0x%08x\n", dw_readl(dws, DW_SPI_DMACR));
		    printk(KERN_EMERG "DW_SPI_DMATDLR: 0x%08x\n", dw_readl(dws, DW_SPI_DMATDLR));
		    printk(KERN_EMERG "DW_SPI_DMARDLR: 0x%08x\n", dw_readl(dws, DW_SPI_DMARDLR));
		}
		*/
		set_bit(RX_BUSY, &dws->dma_chan_busy);
		dmaengine_submit(rxdesc);
		dma_async_issue_pending(dws->rxchan);
	}

	if (txdesc) {
		set_bit(TX_BUSY, &dws->dma_chan_busy);
		dmaengine_submit(txdesc);
		dma_async_issue_pending(dws->txchan);
	}

	return 0;
}

static void spi_dma_stop(struct dw_spi *dws)
{
	if (test_bit(TX_BUSY, &dws->dma_chan_busy)) {
		dmaengine_terminate_all(dws->txchan);
		clear_bit(TX_BUSY, &dws->dma_chan_busy);
	}
	if (test_bit(RX_BUSY, &dws->dma_chan_busy)) {
		dmaengine_terminate_all(dws->rxchan);
		clear_bit(RX_BUSY, &dws->dma_chan_busy);
	}
}

static const struct dw_spi_dma_ops dma_ops = {
	.dma_init   = spi_dma_init,
	.dma_exit   = spi_dma_exit,
	.dma_setup  = spi_dma_setup,
	.can_dma    = spi_can_dma,
	.dma_transfer   = spi_dma_transfer,
	.dma_stop   = spi_dma_stop,
};

int dw_spi_dma_init(struct dw_spi *dws)
{
	//dws->dma_tx = &dma_tx;
	//dws->dma_rx = &dma_rx;
	dws->dma_ops = &dma_ops;
	return 0;
}
EXPORT_SYMBOL(dw_spi_dma_init);

MODULE_LICENSE("GPL");
#endif
