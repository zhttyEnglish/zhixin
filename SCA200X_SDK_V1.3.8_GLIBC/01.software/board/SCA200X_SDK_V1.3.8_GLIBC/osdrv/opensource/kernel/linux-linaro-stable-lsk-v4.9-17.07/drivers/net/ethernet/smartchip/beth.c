/*
 * Ethernet driver for the Beyond Ethernet Controller.
 *      Copyright (c) 2002 Simon Srot (simons@opencores.org)
 *
 * Changes:
 * 10. 02. 2004: Matjaz Breskvar (phoenix@beyondsemi.com)
 *   port to linux-2.4, workaround for next tx bd number.
 * 14. 04. 2007: Gyorgy Jeney (nog@beyondsemi.com)
 *   Use ioremap() and remove the need for the next tx bd workaround.
 * 15. 04. 2007: Gyorgy Jeney (nog@beyondsemi.com)
 *   Autodect if the core can do unaligned accesses or not.
 * 16. 04. 2007: Gyorgy Jeney (nog@beyondsemi.com)
 *   Make the buffer handling consistent between aligned and unaligned cores.
 *   Some cleanups.
 * 18. 05. 2007: Gyorgy Jeney (nog@beyondsemi.com)
 *   Use the NAPI poll callback to actually do the work of receiving frames
 *   and clearing the transmitted bds.
 * 03. 07. 2007: Gyorgy Jeney (nog@beyondsemi.com)
 *   Add media detection and correct MAC configuration.
 * 01. 11. 2007: Gyorgy Jeney (nog@beyondsemi.com)
 *   Support setting of custom MTU sizes
 * 02. 11. 2007: Gyorgy Jeney (nog@beyondsemi.com)
 *   Add the ethtool interface based on the Tigon3 driver.
 * 30. 03. 2009: Gyorgy Jeney (nog@beyondsemi.com)
 *   Use the generic PHY layer.
 *
 * Originally based on:
 *
 * Ethernet driver for Motorola MPC8xx.
 *      Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 *
 * mcen302.c: A Linux network driver for Mototrola 68EN302 MCU
 *
 *      Copyright (C) 1999 Aplio S.A. Written by Vadim Lebedev
 *
 * But it has since been mutilated beyond recognition.
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
#include <linux/dma-mapping.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/ethtool.h>
#include <linux/crc32.h>
#include <linux/platform_device.h>

#include "beth.h"

#define OETH_DEF_RXBD_NUM       5
#define OETH_DEF_TXBD_NUM       5
#define CONFIG_BETH_INT_FILTER  0

/* The absolute maximum frame length that the ethernet is capable of receiving.
 * This is supposed to be 64kb - 1, but that isn't a multiple of 2 so just chop
 * another byte of it */
#define MAX_FRAME_SIZE      (65536 - 2)

/* This is the maximum "overhead" that a packet of a certain MTU will carry */
#define ETH_PROTO_OVERHEAD  (ETH_HLEN + ETH_FCS_LEN)

/* This is how many TX interrupts are coalesced by default.  It would be more
 * accurate to describe this as "after how many TXd packets to wake the
 * queue". */
#define OETH_TX_DEF_COALESCE    16

struct bd {
	union {
		struct sk_buff *sk;
		void *b;
	} b;
	dma_addr_t phys;
};

/* The buffer descriptors track the ring buffers.
 */
struct oeth_private {
	/* Our cosy little lock.  This is supposed to protect the ethernet data
	 * that the timer interrupt messes with.  Everything else is either
	 * lockless or the network layer does the locking (start_xmit and
	 * oeth_poll).  What needs locking is access to the PHY and the
	 * reconfiguration data. */
	spinlock_t lock;

	/* Lock guarding read/write to the interrupt filter */
	spinlock_t intf_lock;

	/* Called to check if the MAC needs reconfigureing when the media
	 * changes */
	struct timer_list timer;

	void *base_addr;    /* ioremap()d base */
	void *intf_base;    /* ioremap()d filter base */

	struct net_device *dev;

	unsigned int gbeth: 1;      /* Is the core a gigabit ethernet mac?*/
	unsigned int status_cnt: 1; /* Does the core have status counters?*/
	unsigned int mask2: 1;      /* Is int_mask2 availible? */
	unsigned int mask3: 1;      /* Is int_mask3 availible? */

	unsigned int mode: 2;       /* Is the core in 10Mbps, 100Mbps or 1000Mbps mode? */
	unsigned int duplex: 1;     /* Is the core in full-duplex mode? */
	unsigned int link: 1;       /* Is the core in full-duplex mode? */
	unsigned int tx_flow: 1;    /* Is TX flow controll enabled? */
	unsigned int rx_flow: 1;    /* Is TX flow controll enabled? */
	unsigned int force_flow: 1; /* Force flow controll en-/dis-able */

	unsigned int reconf_mode: 2; /* What mode to reconfigure to */
	unsigned int reconf_duplex: 1;  /* What duplex to reconfigure to */
	unsigned int reconfigure: 1; /* Reconfigure the mac to the above */

	unsigned int alloc_pkt_len; /* How much space to allocate for one
                     * frame.  This is MTU +
                     * + ETH_PROTO_OVERHEAD + NET_IP_ALIGN*/

	struct bd bd[OETH_TOTAL_BD];

	ushort rx_bd_end;       /* Where the Rx BDs end in the BD pool*/
	ushort tx_bds;          /* The number of BDs to use for TX */

	ushort tx_head;
	ushort tx_tail;
	ushort tx_full;         /* Buffer ring full indicator */
	ushort rx_tail;         /* Next buffer to be checked if packet received */

	struct phy_device *phydev;
	struct mii_bus *mii_bus;
	int mii_irqs[PHY_MAX_ADDR];

	struct napi_struct napi;
	struct net_device_stats stats;
};

static irqreturn_t oeth_interrupt(int irq, void *dev_id);
static void reconf_mac(struct net_device *dev);
static void oeth_tx(struct net_device *dev);
static void oeth_free_buf(struct oeth_private *cep);

/*-------------------------------------------------------------------[ IO ]---*/
static inline void oeth_set_int_src(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_INT_SOURCE);
}

static inline u32 oeth_get_int_src(void *baseaddr)
{
	return readl(baseaddr + OETH_INT_SOURCE);
}

static inline void oeth_set_int_mask(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_INT_MASK);
}

static inline u32 oeth_get_int_mask(void *baseaddr)
{
	return readl(baseaddr + OETH_INT_MASK);
}

/* Note that setting int_mask2 is a nop if it is not availible */
static inline void oeth_set_int_mask2(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_INT_MASK2);
}

static inline u32 oeth_get_int_mask2(void *baseaddr)
{
	return readl(baseaddr + OETH_INT_MASK2);
}

static inline void oeth_set_int_mask3(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_INT_MASK3);
}

static inline u32 oeth_get_int_mask3(void *baseaddr)
{
	return readl(baseaddr + OETH_INT_MASK3);
}

static inline void oeth_set_hash_addr0(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_HASH0);
}

static inline void oeth_set_hash_addr1(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_HASH1);
}

static inline void oeth_set_mac_addr1(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_MAC_ADDR1);
}

static inline u32 oeth_get_mac_addr1(void *baseaddr)
{
	return readl(baseaddr + OETH_MAC_ADDR1);
}

static inline void oeth_set_mac_addr0(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_MAC_ADDR0);
}

static inline u32 oeth_get_mac_addr0(void *baseaddr)
{
	return readl(baseaddr + OETH_MAC_ADDR0);
}

static inline void oeth_set_moder(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_MODER);
}

static inline u32 oeth_get_moder(void *baseaddr)
{
	return readl(baseaddr + OETH_MODER);
}

static inline void oeth_or_moder(void *baseaddr, u32 val)
{
	oeth_set_moder(baseaddr, oeth_get_moder(baseaddr) | val);
}

static inline void oeth_clear_moder(void *baseaddr, u32 val)
{
	oeth_set_moder(baseaddr, oeth_get_moder(baseaddr) & ~val);
}

static inline void oeth_set_tx_bd_num(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_TX_BD_NUM);
}

static inline void oeth_set_packet_len(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_PACKETLEN);
}

static inline void oeth_set_ipgt(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_IPGT);
}

static inline void oeth_set_ipgr1(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_IPGR1);
}

static inline void oeth_set_ipgr2(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_IPGR2);
}

static inline void oeth_set_collconf(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_COLLCONF);
}

static inline void oeth_set_ctrlmoder(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_CTRLMODER);
}

static inline void oeth_set_miiaddress(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_MIIADDRESS);
}

static inline void oeth_set_miitx_data(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_MIITX_DATA);
}

static inline u32 oeth_get_miirx_data(void *baseaddr)
{
	return readl(baseaddr + OETH_MIIRX_DATA);
}

static inline void oeth_set_miicommand(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_MIICOMMAND);
}

static inline u32 oeth_get_miicommand(void *baseaddr)
{
	return readl(baseaddr + OETH_MIICOMMAND);
}

static inline u32 oeth_get_miistatus(void *baseaddr)
{
	return readl(baseaddr + OETH_MIISTATUS);
}

static inline u32 oeth_get_speed_sel(void *baseaddr)
{
	return readl(baseaddr + OETH_SPEED_SEL);
}

static inline void oeth_set_speed_sel(void *baseaddr, u32 val)
{
	writel(val, baseaddr + OETH_SPEED_SEL);
}

static inline void oeth_set_bd_len_status(void *baseaddr, int bd, u32 val)
{
	writel(val, baseaddr + OETH_BD_BASE + (bd * 8));
}

static inline void oeth_set_bd_addr(void *baseaddr, int bd, u32 val)
{
	writel(val, baseaddr + OETH_BD_BASE + (bd * 8) + 4);
}

static inline u32 oeth_get_bd_len_status(void *baseaddr, int bd)
{
	return readl(baseaddr + OETH_BD_BASE + (bd * 8));
}

static inline u32 oeth_get_bd_addr(void *baseaddr, int bd)
{
	return readl(baseaddr + OETH_BD_BASE + (bd * 8) + 4);
}

static inline void oeth_or_bd_len_status(void *baseaddr, int bd, u32 val)
{
	oeth_set_bd_len_status(baseaddr, bd,
	    oeth_get_bd_len_status(baseaddr, bd) | val);
}

static inline void oeth_set_cnt(void *baseaddr, int cnt, u32 val)
{
	writel(val, baseaddr + OETH_CNT_BASE + (cnt * 4));
}

static inline u32 oeth_get_cnt(void *baseaddr, int cnt)
{
	return readl(baseaddr + OETH_CNT_BASE + (cnt * 4));
}

/*------------------------------[ Interrupt filter (interrupt coalescing) ]---*/
static void intf_set_delay0(void *baseaddr, u16 val)
{
	if(!baseaddr)
		return;

	writel(val, baseaddr + INTF_DELAY0);
}

static void intf_set_level0(void *baseaddr, u16 val)
{
	if(!baseaddr)
		return;

	writel(val, baseaddr + INTF_LEVEL0);
}

static u16 intf_get_delay0(void *baseaddr)
{
	if(!baseaddr)
		return 0;

	return readl(baseaddr + INTF_DELAY0);
}

static u16 intf_get_level0(void *baseaddr)
{
	if(!baseaddr)
		return 0;

	return readl(baseaddr + INTF_LEVEL0);
}

static void intf_disable_len0(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	mode &= ~INTF_MODE_LEN0;
	writel(mode, baseaddr + INTF_MODE);
}

static void intf_disable_den0(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	mode &= ~INTF_MODE_DEN0;
	writel(mode, baseaddr + INTF_MODE);
}

static void intf_enable_len0(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	mode |= INTF_MODE_LEN0;
	writel(mode, baseaddr + INTF_MODE);
}

static void intf_enable_den0(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	mode |= INTF_MODE_DEN0;
	writel(mode, baseaddr + INTF_MODE);
}

static void intf_set_delay1(void *baseaddr, u16 val)
{
	if(!baseaddr)
		return;

	writel(val, baseaddr + INTF_DELAY1);
}

static void intf_set_level1(void *baseaddr, u16 val)
{
	if(!baseaddr)
		return;

	writel(val, baseaddr + INTF_LEVEL1);
}

static u16 intf_get_delay1(void *baseaddr)
{
	if(!baseaddr)
		return 0;

	return readl(baseaddr + INTF_DELAY1);
}

static u16 intf_get_level1(void *baseaddr)
{
	if(!baseaddr)
		return 0;

	return readl(baseaddr + INTF_LEVEL1);
}

static void intf_disable_len1(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	mode &= ~INTF_MODE_LEN1;
	writel(mode, baseaddr + INTF_MODE);
}

static void intf_disable_den1(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	mode &= ~INTF_MODE_DEN1;
	writel(mode, baseaddr + INTF_MODE);
}

static void intf_enable_len1(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	mode |= INTF_MODE_LEN1;
	writel(mode, baseaddr + INTF_MODE);
}

static void intf_enable_den1(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	mode |= INTF_MODE_DEN1;
	writel(mode, baseaddr + INTF_MODE);
}

/* Adjusts the interrupt coalesce settings such that the maximum number of
 * interrupts to coalesce is not more than the amount TX/RX buffers. */
static void intf_adj_levels(struct oeth_private *cep)
{
	if((cep->rx_bd_end - cep->tx_bds) < intf_get_level0(cep->intf_base))
		intf_set_level0(cep->intf_base, cep->rx_bd_end - cep->tx_bds);

	if(cep->tx_bds < intf_get_level1(cep->intf_base))
		intf_set_level1(cep->intf_base, cep->tx_bds);
}

static void intf_reset(void *baseaddr)
{
	u32 mode;

	if(!baseaddr)
		return;

	mode = readl(baseaddr + INTF_MODE);
	writel(0, baseaddr + INTF_MODE);
	writel(mode, baseaddr + INTF_MODE);
}

static void intf_set_divisor(void *baseaddr, u32 val)
{
	writel(val, baseaddr + INTF_DIVISOR);
}

static void *intf_init(struct net_device *dev, struct platform_device *pdev)
{
	void *base;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if(!res) {
		printk("%s: Ethernet doesn't have an interrupt filter attached\n",
		    dev->name);
		return NULL;
	}
	if((res->end - res->start + 1) < INTF_ADDR_SPACE) {
		printk("%s: IO space is too small to be an interrupt filter\n",
		    dev->name);
		return NULL;
	}

	base = ioremap_nocache(res->start, INTF_ADDR_SPACE);
	if(!base)
		return NULL;

	/* The delay is always counted in us */
	intf_set_divisor(base, 50);

	intf_set_level0(base, 1);
	intf_set_delay0(base, 0);
	intf_set_level1(base, OETH_TX_DEF_COALESCE);
	intf_set_delay1(base, 0);

	writel(INTF_MODE_LEN0 | INTF_MODE_LEN1, base + INTF_MODE);

	return base;
}

static u32 *intf_dump_regs(struct oeth_private *cep, u32 *p)
{
	int i;

	if(!cep->intf_base) {
		for(i = 0; i < INTF_ADDR_SPACE; i += 4) {
			*p = 0;
			p++;
		}

		return p;
	}

	for(i = 0; i < INTF_ADDR_SPACE; i += 4) {
		*p = readl(cep->intf_base + i);
		p++;
	}

	return p;
}

/*---------------------------------------------------------------[ Driver ]---*/
static inline struct oeth_private *oeth_get_private(struct net_device *dev)
{
	return netdev_priv(dev);
}

static void tx_push(struct oeth_private *cep)
{
	cep->tx_head++;

	if(cep->tx_head == cep->tx_bds)
		cep->tx_head = 0;

	if(cep->tx_head == cep->tx_tail)
		cep->tx_full = 1;
}

static void tx_pop(struct oeth_private *cep)
{
	cep->tx_tail++;

	if(cep->tx_tail == cep->tx_bds)
		cep->tx_tail = 0;

	cep->tx_full = 0;
}

static int tx_empty(struct oeth_private *cep)
{
	return cep->tx_tail == cep->tx_head && !cep->tx_full;
}

static void rx_pop(struct oeth_private *cep)
{
	cep->rx_tail++;

	if(cep->rx_tail == cep->rx_bd_end)
		cep->rx_tail = cep->tx_bds;
}

/* Watch it .. this can schedule */
static void full_lock(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);

	napi_disable(&cep->napi);
	spin_lock_bh(&cep->lock);
	netif_tx_lock(dev);
	oeth_set_int_mask(cep->base_addr, 0);
}

static void full_unlock(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);

	netif_tx_unlock(dev);
	spin_unlock_bh(&cep->lock);
	napi_enable(&cep->napi);

	/* The poll is now enabled, however, if an interrupt comes in between
	 * napi_disable() and the disabling of interrupts in full_lock(),
	 * poll will not be scheduled and thus the driver would not do the
	 * appropriate work until another interrupt condition would arise.
	 * Thus, just in case, schedule the poll (which will enable interrupts
	 * again). */
	napi_schedule(&cep->napi); /* schedule NAPI poll */
}

static void init_tx_bd_param(struct oeth_private *cep, int bd)
{
#ifdef CONFIG_BETH_INT_FILTER
	/* In this case, TX interrupts are only enabled when the TX ring is
	 * full and the interrupt filter will delay the TX interrupt until
	 * the given amount of TX BDs can be freed */
	oeth_set_bd_len_status(cep->base_addr, bd, OETH_TX_BD_IRQ);
#else
	/* Generate an interrupt when half the TX ring moves out */
	if(!bd || bd == cep->tx_bds / 2)
		oeth_set_bd_len_status(cep->base_addr, bd, OETH_TX_BD_IRQ);
	else
		oeth_set_bd_len_status(cep->base_addr, bd, 0);
#endif
}

static int oeth_alloc_buf(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	struct  sk_buff *skb;
	int i;

	for(i = 0; i < cep->tx_bds; i++) {
		cep->bd[i].b.sk = NULL;

		oeth_set_bd_addr(cep->base_addr, i, 0);
		init_tx_bd_param(cep, i);
	}
	oeth_or_bd_len_status(cep->base_addr, cep->tx_bds - 1, OETH_TX_BD_WRAP);

	for(i = cep->tx_bds; i < cep->rx_bd_end; i++) {
		skb = netdev_alloc_skb(dev, cep->alloc_pkt_len);
		skb_reserve(skb, NET_IP_ALIGN);

		if(!skb) {
			printk("%s: Failed to allocate all of the RX buffers\n",
			    dev->name);
			if(i != cep->tx_bds) {
				cep->rx_bd_end = i;
				cep->napi.weight = cep->rx_bd_end - cep->tx_bds;

				spin_lock(&cep->intf_lock);
				intf_adj_levels(cep);
				spin_unlock(&cep->intf_lock);

				printk("%s: Truncating RX buffers to %i buffers\n",
				    dev->name, i - cep->tx_bds);
			} else {
				printk("%s: No RX buffers allocated -- disabling\n",
				    dev->name);
				/* Clear one Rx BD such that all the locks
				 * can be released and the core won't
				 * go scribbling over random pieces of memory */
				oeth_set_bd_len_status(cep->base_addr, i,
				    OETH_RX_BD_WRAP);
				return -ENOMEM;
			}
			break;
		}

		oeth_set_bd_len_status(cep->base_addr, i,
		    OETH_RX_BD_EMPTY | OETH_RX_BD_IRQ);

		cep->bd[i].b.sk = skb;

		cep->bd[i].phys = dma_map_single(NULL, skb->data,
		        skb_tailroom(skb),
		        DMA_FROM_DEVICE);
		oeth_set_bd_addr(cep->base_addr, i, cep->bd[i].phys);
	}
	oeth_or_bd_len_status(cep->base_addr, cep->rx_bd_end - 1,
	    OETH_RX_BD_WRAP);

	return 0;
}

static int oeth_open(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	int rc = 0;

	spin_lock(&cep->lock);

	phy_start(cep->phydev);

	rc = oeth_alloc_buf(dev);
	if(rc < 0)
		goto out;

	cep->tx_tail = 0;
	cep->tx_head = 0;
	cep->tx_full = 0;
	cep->rx_tail = cep->tx_bds;

	/* Clear all pending interrupts */
	oeth_set_int_src(cep->base_addr, 0xffffffff);

	rc = request_irq(dev->irq, oeth_interrupt, 0, "eth", dev);
	if(rc < 0) {
		oeth_free_buf(cep);
		goto out;
	}

	intf_reset(cep->intf_base);

	if(cep->mask2)
		oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_RX);
	else
		oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_RX |
		    OETH_INT_MASK_TX);

	oeth_or_moder(cep->base_addr, OETH_MODER_RXEN | OETH_MODER_TXEN);

	spin_unlock(&cep->lock);

	netif_wake_queue(dev);
	napi_enable(&cep->napi);

	return 0;

out:
	spin_unlock(&cep->lock);
	return 0;
}

static void oeth_free_buf(struct oeth_private *cep)
{
	int i;
	for(i = 0; i < cep->tx_bds; i++) {
		if(cep->bd[i].b.sk) {
			dma_unmap_single(NULL, cep->bd[i].phys,
			    cep->bd[i].b.sk->len,
			    DMA_TO_DEVICE);
			dev_kfree_skb(cep->bd[i].b.sk);
		}
	}

	for(i = cep->tx_bds; i < cep->rx_bd_end; i++) {
		if(cep->bd[i].b.sk) {
			dma_unmap_single(NULL, cep->bd[i].phys,
			    skb_tailroom(cep->bd[i].b.sk),
			    DMA_FROM_DEVICE);
			dev_kfree_skb(cep->bd[i].b.sk);
		}
	}
}

/* This cleans up any remaining RX/TX buffers that are still around after
 * disabling RX/TX and havn't been cleaned up by oeth_rx() or oeth_tx(). */
static void oeth_flush_tx_rx(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 st;

	/* Don't mark those frames as dropped that have been sucessfully TXd */
	oeth_tx(dev);

	/* TX buffer cleaning */
	for(;;) {
		if(tx_empty(cep))
			break;
		st = oeth_get_bd_len_status(cep->base_addr, cep->tx_tail);

		cep->stats.tx_dropped++;
		if(!cep->status_cnt) {
			cep->stats.tx_packets++;
			cep->stats.collisions += (st & OETH_TX_BD_RETRY) >> 4;
		}

		init_tx_bd_param(cep, cep->tx_tail);

		dma_unmap_single(NULL, cep->bd[cep->tx_tail].phys,
		    cep->bd[cep->tx_tail].b.sk->len,
		    DMA_TO_DEVICE);
		/* Free the skb associated with this last transmit */
		dev_kfree_skb(cep->bd[cep->tx_tail].b.sk);
		cep->bd[cep->tx_tail].b.sk = NULL;

		tx_pop(cep);
	}

	/* If the queue was stopped because of an overflow of TX BDs,
	 * it may not be restarted again since the buffers are cleared now
	 * so there will be no TX interrupts to restart it again.  Thus
	 * explicitly restart the queue should it have been stopped */
	netif_wake_queue(dev);

	/* RX buffer cleaning */
	for(;;) {
		st = oeth_get_bd_len_status(cep->base_addr, cep->rx_tail);

		if(st & OETH_RX_BD_EMPTY)
			break;

		st |= OETH_RX_BD_EMPTY;
		st &= ~OETH_RX_BD_STATS;
		oeth_set_bd_len_status(cep->base_addr, cep->rx_tail, st);

		cep->stats.rx_dropped++;

		rx_pop(cep);
	}
}

static int oeth_close(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);

	napi_disable(&cep->napi);
	del_timer_sync(&cep->timer);

	spin_lock(&cep->lock);

	oeth_set_int_mask(cep->base_addr, 0);
	oeth_set_int_mask2(cep->base_addr, 0);

	oeth_clear_moder(cep->base_addr, OETH_MODER_RXEN | OETH_MODER_TXEN);

	free_irq(dev->irq, (void *)dev);

	oeth_flush_tx_rx(dev);

	oeth_free_buf(cep);
	phy_stop(cep->phydev);

	spin_unlock(&cep->lock);

	return 0;
}

static void oeth_set_addr(struct sk_buff *skb, struct oeth_private *cep)
{

	cep->bd[cep->tx_head].phys = dma_map_single(NULL, skb->data, skb->len,
	        DMA_TO_DEVICE);

	oeth_set_bd_addr(cep->base_addr, cep->tx_head,
	    cep->bd[cep->tx_head].phys);

	cep->bd[cep->tx_head].b.sk = skb;
}

static int oeth_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 st;

	oeth_tx(dev);

	if(cep->tx_full) {
		/* All transmit buffers are full.  Try to clean up */
		/* If all buffers are still full, bail */
		netif_stop_queue(dev);
		oeth_set_int_src(cep->base_addr, OETH_INT_TX);
		/* Another check is needed, since if the TX interrupt
		 * came in between oeth_tx() and netif_stop_queue(),
		 * the interrupt shall wake the queue when it hasn't
		 * been disabled yet, thus TX will sleep permanently. */
		oeth_tx(dev);
		if(!cep->tx_full) {
			netif_wake_queue(dev);
			goto tx_bd_empty;
		}

		oeth_set_int_mask2(cep->base_addr, OETH_INT_MASK_TX);
		return NETDEV_TX_BUSY;
	}

tx_bd_empty:
	st = oeth_get_bd_len_status(cep->base_addr, cep->tx_head);

	st &= ~(OETH_BD_LEN | OETH_TX_BD_STATS);
	st |= skb->len << 16;

	oeth_set_addr(skb, cep);

	/* Send it on its way.  Tell controller it's ready, and to put the CRC
	 * on the end. */
	st |= OETH_TX_BD_READY | OETH_TX_BD_CRC | OETH_TX_BD_PAD;

	oeth_set_bd_len_status(cep->base_addr, cep->tx_head, st);

	tx_push(cep);

	//dev->trans_start = jiffies; hbbai, this is moved to netdev_queue in 4.x kernel

	return NETDEV_TX_OK;
}

/* The interrupt handler.
 */
static irqreturn_t oeth_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct oeth_private *cep = oeth_get_private(dev);
	u32 int_events;
	u32 mask;

	/* Get the interrupt events that caused us to be here.
	 */
	int_events = oeth_get_int_src(cep->base_addr);

	mask = oeth_get_int_mask(cep->base_addr);
	mask |= oeth_get_int_mask2(cep->base_addr);
	int_events &= mask;

	/* For shared interrupts.. */
	if(!int_events)
		return IRQ_NONE;

	oeth_set_int_src(cep->base_addr, int_events);

	if(int_events & OETH_INT_TX) {
		/* Waking the queue will have the desired effect that
		 * start_xmit() will be called hence oeth_tx() and then thank
		 * goodness we don't have to take a lock here :) */
		oeth_set_int_mask2(cep->base_addr, 0);
		netif_wake_queue(dev);
	}

	if(int_events & OETH_INT_RX) {
		if(cep->mask2)
			oeth_set_int_mask(cep->base_addr, 0);
		else
			oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_TX);

		napi_schedule(&cep->napi); /* schedule NAPI poll */
	}

	return IRQ_HANDLED;
}

static void oeth_tx(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 st;

	for(;; tx_pop(cep)) {
		st = oeth_get_bd_len_status(cep->base_addr, cep->tx_tail);
		if((st & OETH_TX_BD_READY) || tx_empty(cep))
			break;

		if(!cep->status_cnt) {
			if(unlikely(st & OETH_TX_BD_LATECOL))
				cep->stats.tx_window_errors++;
			if(unlikely(st & OETH_TX_BD_RETLIM))
				cep->stats.tx_aborted_errors++;
			if(unlikely(st & OETH_TX_BD_DT))
				cep->stats.tx_aborted_errors++;
			if(unlikely(st & OETH_TX_BD_UNDERRUN))
				cep->stats.tx_fifo_errors++;
			if(unlikely(st & OETH_TX_BD_CARRIER))
				cep->stats.tx_carrier_errors++;
			if(unlikely(st & (OETH_TX_BD_LATECOL |
			            OETH_TX_BD_RETLIM |
			            OETH_TX_BD_UNDERRUN |
			            OETH_TX_BD_CARRIER)))
				cep->stats.tx_errors++;

			cep->stats.tx_packets++;
			cep->stats.tx_bytes += st >> 16;
			cep->stats.collisions += (st & OETH_TX_BD_RETRY) >> 4;
		}

		BUG_ON(st & OETH_TX_BD_BUS_ERR);

		dma_unmap_single(NULL, cep->bd[cep->tx_tail].phys,
		    cep->bd[cep->tx_tail].b.sk->len,
		    DMA_TO_DEVICE);
		/* Free the skb associated with this last transmit */
		dev_kfree_skb(cep->bd[cep->tx_tail].b.sk);
		cep->bd[cep->tx_tail].b.sk = NULL;
	}
}

static void oeth_rx_unalign(struct net_device *dev, int pkt_len)
{
	struct oeth_private *cep = oeth_get_private(dev);
	struct sk_buff *skb = cep->bd[cep->rx_tail].b.sk;
	dma_addr_t phys = cep->bd[cep->rx_tail].phys;
	struct sk_buff *skb_alloc;

	skb_alloc = netdev_alloc_skb_ip_align(dev, pkt_len);

	if(likely(skb_alloc)) {
		dma_sync_single_for_cpu(NULL, phys, skb_tailroom(skb),
		    DMA_FROM_DEVICE);

		skb_alloc->dev = dev;
		memcpy(skb_put(skb_alloc, pkt_len), skb->data, pkt_len);

		skb_alloc->protocol = eth_type_trans(skb_alloc, dev);
		netif_receive_skb(skb_alloc);

		dma_sync_single_for_device(NULL, phys, skb_tailroom(skb),
		    DMA_FROM_DEVICE);
		if(!cep->status_cnt) {
			cep->stats.rx_packets++;
			cep->stats.rx_bytes += pkt_len;
		}
	} else {
		cep->stats.rx_dropped++;
	}
}

static int oeth_rx(struct net_device *dev, int *budget)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 st;
	int work_done;

	for(work_done = 0; work_done < *budget; work_done++, rx_pop(cep)) {
		st = oeth_get_bd_len_status(cep->base_addr, cep->rx_tail);

		if(st & OETH_RX_BD_EMPTY) {
			*budget = work_done;
			return 1;
		}

		BUG_ON(st & OETH_RX_BD_BUS_ERR);

		st |= OETH_RX_BD_EMPTY;

		/* Receive packet now, ask questions later */
		if(likely(!(st & OETH_RX_BD_STATS & ~OETH_RX_BD_MISS &
		            ~OETH_RX_BD_MCAST))) {
			oeth_rx_unalign(dev, st >> 16);

			if(!cep->status_cnt && (st & OETH_RX_BD_STATS))
				cep->stats.multicast++;

			st &= ~(OETH_RX_BD_MISS | OETH_RX_BD_MCAST);
			oeth_set_bd_len_status(cep->base_addr, cep->rx_tail,
			    st);
			continue;
		}

		oeth_set_bd_len_status(cep->base_addr, cep->rx_tail,
		    st & ~OETH_RX_BD_STATS);

		if(!cep->status_cnt) {
			cep->stats.rx_errors++;
			if(st & (OETH_RX_BD_TOOLONG | OETH_RX_BD_SHORT))
				cep->stats.rx_length_errors++;
			if(st & OETH_RX_BD_CRCERR)
				cep->stats.rx_crc_errors++;
			if(st & OETH_RX_BD_OVERRUN)
				cep->stats.rx_fifo_errors++;
			if(st & OETH_RX_BD_INVSIMB)
				cep->stats.rx_frame_errors++;
			if(st & OETH_RX_BD_LATECOL)
				cep->stats.rx_frame_errors++;
			if(st & OETH_RX_BD_DRIBBLE)
				cep->stats.rx_frame_errors++;
		}
	}

	*budget = work_done;
	return 0;
}

static int oeth_poll(struct napi_struct *napi, int budget)
{
	struct oeth_private *cep = container_of(napi, struct oeth_private, napi);
	int done;

	done = oeth_rx(cep->dev, &budget);

	if(cep->reconfigure)
		reconf_mac(cep->dev);

	if(done) {
		napi_complete(napi);

		if(cep->mask2)
			oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_RX);
		else
			oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_RX |
			    OETH_INT_MASK_TX);
	}

	return budget;
}

/*-----------------------------------------------------[ Driver callbacks ]---*/
static struct net_device_stats *oeth_get_stats(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);

	if(!cep->status_cnt)
		return &cep->stats;

	/* XXX: There needs to be a TX/RX not-ok counter since it may be possible that
	 * a frame has more than one problem being sent/received
	    cep->stats.rx_errors = oeth_get_cnt(cep->base_addr, OETH_CNT_RXBAD);
	    cep->stats.tx_errors = oeth_get_cnt(cep->base_addr, OETH_CNT_TXBAD);
	*/
	cep->stats.rx_packets = oeth_get_cnt(cep->base_addr, OETH_CNT_RXOK) +
	    cep->stats.rx_errors;
	cep->stats.tx_packets = oeth_get_cnt(cep->base_addr, OETH_CNT_TXOK) +
	    cep->stats.tx_errors;
	cep->stats.rx_bytes = oeth_get_cnt(cep->base_addr, OETH_CNT_RXB);
	cep->stats.tx_bytes = oeth_get_cnt(cep->base_addr, OETH_CNT_TXB);
	cep->stats.multicast = oeth_get_cnt(cep->base_addr, OETH_CNT_MCAST);
	cep->stats.collisions = oeth_get_cnt(cep->base_addr, OETH_CNT_TXCOLL);

	cep->stats.rx_length_errors =
	    oeth_get_cnt(cep->base_addr, OETH_CNT_RXTL) +
	    oeth_get_cnt(cep->base_addr, OETH_CNT_RXTS);
	cep->stats.rx_crc_errors = oeth_get_cnt(cep->base_addr, OETH_CNT_RXCRC);
	cep->stats.rx_frame_errors = oeth_get_cnt(cep->base_addr, OETH_CNT_INVSYM) +
	    oeth_get_cnt(cep->base_addr, OETH_CNT_RXLATE) +
	    oeth_get_cnt(cep->base_addr, OETH_CNT_RXDN);
	cep->stats.rx_fifo_errors = oeth_get_cnt(cep->base_addr, OETH_CNT_OVER);
	cep->stats.rx_missed_errors =
	    oeth_get_cnt(cep->base_addr, OETH_CNT_RXMISS);

	cep->stats.tx_aborted_errors =
	    oeth_get_cnt(cep->base_addr, OETH_CNT_RETLIM) +
	    oeth_get_cnt(cep->base_addr, OETH_CNT_TXDT);
	cep->stats.tx_carrier_errors =
	    oeth_get_cnt(cep->base_addr, OETH_CNT_TXCSL);
	cep->stats.tx_fifo_errors = oeth_get_cnt(cep->base_addr, OETH_CNT_UNDER);
	cep->stats.tx_window_errors = oeth_get_cnt(cep->base_addr, OETH_CNT_TXLATE);

	cep->stats.tx_heartbeat_errors = 0;

	return &cep->stats;
}

static void oeth_set_multicast_list(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	struct netdev_hw_addr *ha;
	u32 hash[2];
	int hash_b;

	if(dev->flags & IFF_PROMISC) {
		spin_lock_bh(&cep->lock);
		oeth_or_moder(cep->base_addr, OETH_MODER_PRO);
		spin_unlock_bh(&cep->lock);
		return;
	}

	spin_lock_bh(&cep->lock);
	oeth_clear_moder(cep->base_addr, OETH_MODER_PRO);
	spin_unlock_bh(&cep->lock);

	if(dev->flags & IFF_ALLMULTI) {
		/* Catch all multicast addresses, so set the
		 * filter to all 1's.
		 */
		oeth_set_hash_addr0(cep->base_addr, 0xffffffff);
		oeth_set_hash_addr1(cep->base_addr, 0xffffffff);
		return;
	}

	hash[0] = 0;
	hash[1] = 0;

	netdev_for_each_mc_addr(ha, dev) {
		/* Only support group multicast for now. */
		if(!(ha->addr[0] & 1))
			continue;

		hash_b = ~crc32_le(~0, ha->addr, ETH_ALEN) & 0x3f;
		hash[hash_b >> 5] = 1 << (hash_b & 0x1f);
	}

	oeth_set_hash_addr0(cep->base_addr, hash[0]);
	oeth_set_hash_addr1(cep->base_addr, hash[1]);
}

static void upload_mac(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);

	oeth_set_mac_addr1(cep->base_addr, (u32)(u8)dev->dev_addr[0] << 8 |
	    (u32)(u8)dev->dev_addr[1]);
	oeth_set_mac_addr0(cep->base_addr, (u32)(u8)dev->dev_addr[2] << 24 |
	    (u32)(u8)dev->dev_addr[3] << 16 |
	    (u32)(u8)dev->dev_addr[4] << 8  |
	    (u32)(u8)dev->dev_addr[5]);
}

static int oeth_set_mac_add(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	upload_mac(dev);

	return 0;
}

static int oeth_change_mtu(struct net_device *dev, int new_mtu)
{
	struct oeth_private *cep = oeth_get_private(dev);
	int rc, reopen;

	/* We can go all the way to zero with the MTU however that is 100%
	 * useless, thus just leave this check the same as
	 * net/ethernet/eth.c::eth_change_mtu() */
	if(new_mtu < 68)
		return -EINVAL;
	if(new_mtu > (MAX_FRAME_SIZE - ETH_PROTO_OVERHEAD))
		return -EINVAL;

	if (dev->mtu == new_mtu) {
		/* MTU not changed, nothing to do. */
		return 0;
	}

	reopen = netif_running(dev);

	printk(KERN_DEBUG "MTU changing to %d, reopen %d\n", new_mtu, reopen);

	if (reopen) {
		oeth_close(dev);
	}

	dev->mtu = new_mtu;
	printk(KERN_DEBUG "MTU change: set packet len %d\n",
	    ((ETH_ZLEN + ETH_FCS_LEN) << 16) |
	    (new_mtu + ETH_PROTO_OVERHEAD));
	cep->alloc_pkt_len = new_mtu + ETH_PROTO_OVERHEAD + NET_IP_ALIGN;
	oeth_set_packet_len(cep->base_addr, ((ETH_ZLEN + ETH_FCS_LEN) << 16) |
	    (new_mtu + ETH_PROTO_OVERHEAD));

	if (reopen) {
		printk(KERN_DEBUG "MTU change: reopening oeth\n");
		rc = oeth_open(dev);
		printk(KERN_INFO "MTU on %s set to %d\n", dev->name, dev->mtu);
		return rc;
	} else {
		return 0;
	}
}

/*--------------------------------------------------[ Generic MII support ]---*/
static int oeth_mdio_read(struct mii_bus *bus, int phy_addr, int regnum)
{
	struct oeth_private *cep = oeth_get_private(bus->priv);
	u32 st, cmd;

	/* Timeout 1s for this command */
	unsigned long timeout = jiffies + HZ;

	oeth_set_miiaddress(cep->base_addr, (regnum << 8) | phy_addr);
	oeth_set_miicommand(cep->base_addr, OETH_MIICOMMAND_RSTAT);

	do {
		st = oeth_get_miistatus(cep->base_addr);

		if(time_after(jiffies, timeout)) {
			/* Check mii status once more, just to
			be sure that timeout really occourred,
			since we are not using any IRQ locking. */
			st = oeth_get_miistatus(cep->base_addr);
			if ((st & OETH_MIISTATUS_BUSY) == 0)
				break;

			cmd = oeth_get_miicommand(cep->base_addr);
			printk(KERN_ERR "MDIO read timeout (cmd: 0x%08x, "\
			    "status: 0x%08x)\n", cmd, st);
			return -EIO;
		}
	} while(st & OETH_MIISTATUS_BUSY);

	st = oeth_get_miirx_data(cep->base_addr);

	return st;
}

static int oeth_mdio_write(struct mii_bus *bus, int phy_addr, int regnum,
    u16 val)
{
	struct oeth_private *cep = oeth_get_private(bus->priv);
	u32 st, cmd;

	/* Timeout 1s for this command */
	unsigned long timeout = jiffies + HZ;

	oeth_set_miiaddress(cep->base_addr, (regnum << 8) | phy_addr);
	oeth_set_miitx_data(cep->base_addr, val);
	oeth_set_miicommand(cep->base_addr, OETH_MIICOMMAND_WCTRLDATA);

	do {
		st = oeth_get_miistatus(cep->base_addr);

		if(time_after(jiffies, timeout)) {
			/* Check mii status once more, just to
			be sure that timeout really occourred,
			since we are not using any IRQ locking. */
			st = oeth_get_miistatus(cep->base_addr);
			if ((st & OETH_MIISTATUS_BUSY) == 0)
				break;

			cmd = oeth_get_miicommand(cep->base_addr);
			printk(KERN_ERR "MDIO write timeout (cmd: 0x%08x, "\
			    "status: 0x%08x)\n", cmd, st);
			return -EIO;
		}
	} while(st & OETH_MIISTATUS_BUSY);

	return 0;
}

static int oeth_mdio_reset(struct mii_bus *bus)
{
	return 0;
}

/*----------------------------------------------[ Configuration detection ]---*/

static void oeth_start_timer(struct oeth_private *cep)
{
	cep->timer.expires = jiffies + HZ; /* Once per second should be enough*/
	add_timer(&cep->timer);
}

static void oeth_timer(unsigned long __opaque)
{
	struct net_device *dev = (struct net_device *)__opaque;
	struct oeth_private *cep = oeth_get_private(dev);

	napi_schedule(&cep->napi);

	oeth_start_timer(cep);
}

static void reconf_mac(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 st;

	del_timer_sync(&cep->timer);

	spin_lock_bh(&cep->lock);

	/* This needs to be done, since the core needs to switch PHY clocks,
	 * which would screw up RX/TX anyway.. */
	st = oeth_get_moder(cep->base_addr);
	oeth_set_moder(cep->base_addr,
	    st & ~(OETH_MODER_RXEN | OETH_MODER_TXEN));

	netif_tx_lock(dev);
	oeth_flush_tx_rx(dev);
	cep->tx_tail = 0;
	cep->tx_head = 0;
	cep->tx_full = 0;
	cep->rx_tail = cep->tx_bds;

	cep->mode   = cep->reconf_mode;
	cep->duplex = cep->reconf_duplex;

	st &= ~OETH_MODER_FULLD;

	if(cep->reconf_duplex)
		st |= OETH_MODER_FULLD;

	if(cep->reconf_mode == OETH_SPEED_SEL_1000) {
		oeth_set_ipgt(cep->base_addr, 0x00000004);
		oeth_set_collconf(cep->base_addr, 0x000f01ff);
		st &= ~OETH_MODER_FIFO;
		st |= OETH_MODER_FIFOF;
	} else {
		oeth_set_ipgt(cep->base_addr, 0x00000008);
		oeth_set_collconf(cep->base_addr, 0x000f003f);
		st &= ~OETH_MODER_FIFO;
		st |= OETH_MODER_FIFOQ;
	}
	oeth_set_speed_sel(cep->base_addr, cep->mode);

	cep->reconfigure = 0;

	/* Ok, we have reconfigured, let's go! */
	oeth_set_moder(cep->base_addr, st);
	netif_tx_unlock(dev);

	spin_unlock_bh(&cep->lock);
}

static void conf_mac_speed(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	struct phy_device *phydev = cep->phydev;
	int duplex;
	int mode;
	int flow;
	int link = cep->link;

	if(!phydev->link) {
		if(cep->link)
			phy_print_status(phydev);
		cep->link = !!phydev->link;
		return;
	}
	cep->link = !!phydev->link;

	spin_lock_bh(&cep->lock);

	switch(phydev->speed) {
	case SPEED_10:
		mode = OETH_SPEED_SEL_10;
		break;
	case SPEED_100:
		mode = OETH_SPEED_SEL_100;
		break;
	case SPEED_1000:
		mode = OETH_SPEED_SEL_1000;
		break;
	default:
		mode = OETH_SPEED_SEL_100;
		break;
	}

	duplex = !!phydev->duplex;

	if(!cep->force_flow) {
		flow = 0;
		if((phydev->pause && !phydev->asym_pause) || phydev->asym_pause)
			flow |= OETH_CTRLMODER_TXFLOW;
		if(phydev->pause)
			flow |= OETH_CTRLMODER_RXFLOW;

		oeth_set_ctrlmoder(cep->base_addr, flow);
	}

	if(mode == cep->mode && cep->duplex == duplex)
		goto unlock;

	cep->reconf_mode   = mode;
	cep->reconf_duplex = !!duplex;

	/* Ok, the MAC needs to be reconfigured.  The problem is that we need
	 * mutual exclusion between this and oeth_poll().  Now, taking a lock
	 * each and every time oeth_poll is called is "slow" so the code to
	 * reconfigure the MAC is dumped into oeth_poll() and only runs
	 * when oeth->reconfigure is not null.  Note that it may be possible to
	 * get here when oeth_poll() is in the exit path, after
	 * oeth->reconfigure has evaluated and so the poll function won't be
	 * queued again.  That is why a timer is started here to reschedule
	 * the NAPI poll in that case */
	napi_schedule(&cep->napi); /* schedule NAPI poll */
	del_timer_sync(&cep->timer);
	oeth_start_timer(cep);

	if(cep->reconfigure)
		goto unlock;

	cep->reconfigure = 1;

	phy_print_status(phydev);

unlock:
	if(!cep->reconfigure && !link && phydev->link)
		phy_print_status(phydev);

	spin_unlock_bh(&cep->lock);
}

/*----------------------------------------------------------------[ IOCTL ]---*/
static int oeth_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct oeth_private *cep = oeth_get_private(dev);

	if(!netif_running(dev))
		return -EINVAL;

	return phy_mii_ioctl(cep->phydev, rq, cmd);
}

/*------------------------------------------------------[ Ethtool support ]---*/
static int oeth_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct oeth_private *cep = oeth_get_private(dev);

	if(cep->phydev)
		return phy_ethtool_gset(cep->phydev, cmd);

	return -EINVAL;
}

static int oeth_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct oeth_private *cep = oeth_get_private(dev);

	if(!capable(CAP_NET_ADMIN))
		return -EPERM;

	if(cep->phydev)
		return phy_ethtool_sset(cep->phydev, cmd);

	return -EINVAL;
}

static int oeth_nway_reset(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);

	if(!netif_running(dev))
		return -EINVAL;

	return phy_start_aneg(cep->phydev);
}

static void oeth_get_drvinfo(struct net_device *dev,
    struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, "O/B eth", sizeof(info->driver));
	strlcpy(info->version, "0.1.1", sizeof(info->version));
	strlcpy(info->fw_version, "", sizeof(info->fw_version)); /* We have no firmware */
	snprintf(info->bus_info, sizeof(info->bus_info), "@0x%p", (void *)dev->base_addr);
}

static int oeth_get_regs_len(struct net_device *dev)
{
#ifdef CONFIG_BETH_INT_FILTER
	/* Also return the ring pointers */
	return OETH_ADDR_SPACE + (4 * 4) + INTF_ADDR_SPACE;
#else
	/* Also return the ring pointers */
	return OETH_ADDR_SPACE + (4 * 4);
#endif
}

static void oeth_get_regs(struct net_device *dev, struct ethtool_regs *regs,
    void *_p)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 *p = _p;
	int i;
	u32 int_mask;

	regs->version = 0;

	/* The mask is killed in full_lock(), so save it just before.  The
	 * value of int_mask changes frequently so the value shall be all
	 * crap in anycase, but there are some people that can see past the
	 * noise ;) */
	int_mask = oeth_get_int_mask(cep->base_addr);

	full_lock(dev);

	/* Right, all should be idle now.  Yes, the MAC may be in the middle of
	 * a frame reception, but that shouldn't matter since only the BD status
	 * register is modified because of that, and a single read is
	 * (obviously) atomic */
	for(i = 0; i < OETH_ADDR_SPACE; i += 4) {
		if(i == OETH_INT_MASK)
			*p = int_mask;
		else
			*p = readl(cep->base_addr + i);
		p++;
	}

	*p++ = cep->tx_head;
	*p++ = cep->tx_tail;
	*p++ = cep->rx_tail;
	*p++ = 0;

	spin_lock(&cep->intf_lock);
	p = intf_dump_regs(cep, p);
	spin_unlock(&cep->intf_lock);

	full_unlock(dev);
}

static void oeth_get_ringparam(struct net_device *dev,
    struct ethtool_ringparam *ering)
{
	struct oeth_private *cep = oeth_get_private(dev);

	/* -1: Because atleast one of the BDs need to be allocated to both
	 * RX and TX */
	ering->rx_max_pending = OETH_TOTAL_BD - 1;
	ering->rx_mini_max_pending = 0;
	ering->rx_jumbo_max_pending = 0;
	ering->tx_max_pending = OETH_TOTAL_BD - 1;

	ering->rx_pending = cep->rx_bd_end - cep->tx_bds;
	ering->rx_mini_pending = 0;
	ering->rx_jumbo_pending = 0;
	ering->tx_pending = cep->tx_bds;
}

static int oeth_set_ringparam(struct net_device *dev,
    struct ethtool_ringparam *ering)
{
	struct oeth_private *cep = oeth_get_private(dev);
	int rc;
	u32 mode;

	if(ering->rx_pending < 1)
		return -EINVAL;
	if(ering->tx_pending < 1)
		return -EINVAL;
	/* Make sure userspace doesn't fool us by overflowing u32 */
	if(ering->rx_pending >= OETH_TOTAL_BD)
		return -EINVAL;
	if(ering->tx_pending >= OETH_TOTAL_BD)
		return -EINVAL;
	if((ering->tx_pending + ering->rx_pending) > OETH_TOTAL_BD)
		return -EINVAL;

	full_lock(dev);

	mode = oeth_get_moder(cep->base_addr);
	oeth_set_moder(cep->base_addr, mode & ~(OETH_MODER_RXEN |
	        OETH_MODER_TXEN));

	if(netif_running(dev)) {
		/* There may be some buffers that have finished RXing, however
		 * it is too late to call oeth_rx(), since that may (very)
		 * indirectly call dev_hard_start_xmit(), which will need to
		 * take the tx lock, but that lock is held here so -> deadlock.
		 *  Anyway, I'm sure noone will miss a couple of packets. */
		oeth_flush_tx_rx(dev);

		oeth_free_buf(cep);
	}

	cep->tx_bds = ering->tx_pending;
	cep->rx_bd_end = ering->tx_pending + ering->rx_pending;

	spin_lock(&cep->intf_lock);
	intf_adj_levels(cep);
	spin_unlock(&cep->intf_lock);

	cep->napi.weight = cep->rx_bd_end - cep->tx_bds;
	oeth_set_tx_bd_num(cep->base_addr, cep->tx_bds);

	cep->tx_tail = 0;
	cep->tx_head = 0;
	cep->tx_full = 0;
	cep->rx_tail = cep->tx_bds;

	if(netif_running(dev)) {
		rc = oeth_alloc_buf(dev);
		if(rc < 0) {
			/* Buf allocation failed, kill the device */
			full_unlock(dev);
			dev_close(dev);
			return rc;
		}
	}

	oeth_set_moder(cep->base_addr, mode);

	full_unlock(dev);

	return 0;
}

static void oeth_get_pauseparam(struct net_device *dev,
    struct ethtool_pauseparam *epause)
{
	struct oeth_private *cep = oeth_get_private(dev);

	spin_lock_bh(&cep->lock);

	epause->autoneg = !cep->force_flow;
	epause->rx_pause = cep->rx_flow;
	epause->tx_pause = cep->tx_flow;

	spin_unlock_bh(&cep->lock);
}

static int oeth_set_pauseparam(struct net_device *dev,
    struct ethtool_pauseparam *epause)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 flow;
	u32 newadv, oldadv;
	int err = 0;

	spin_lock_bh(&cep->lock);

	cep->force_flow = !epause->autoneg;

	if(!epause->autoneg) {
		cep->tx_flow = !!epause->tx_pause;
		cep->rx_flow = !!epause->rx_pause;

		flow = 0;
		if(cep->tx_flow)
			flow |= OETH_CTRLMODER_TXFLOW;
		if(cep->rx_flow)
			flow |= OETH_CTRLMODER_RXFLOW;

		oeth_set_ctrlmoder(cep->base_addr, flow);
	} else {
		if(epause->rx_pause) {
			if(epause->tx_pause)
				newadv = ADVERTISED_Pause;
			else
				newadv = ADVERTISED_Pause |
				    ADVERTISED_Asym_Pause;
		} else if (epause->tx_pause) {
			newadv = ADVERTISED_Asym_Pause;
		} else
			newadv = 0;

		oldadv = cep->phydev->advertising & (ADVERTISED_Pause |
		        ADVERTISED_Asym_Pause);
		if(oldadv != newadv) {
			cep->phydev->advertising &= ~(ADVERTISED_Pause |
			        ADVERTISED_Asym_Pause);
			cep->phydev->advertising |= newadv;
			err = phy_start_aneg(cep->phydev);
		}
	}

	spin_unlock_bh(&cep->lock);

	return err;
}

static const struct {
	const char string[ETH_GSTRING_LEN];
} ethtool_stats_keys[OETH_CNT_NUM] = {
	/*  0 */{ "tx_ok_packets" },
	{ "tx_collisions" },
	{ "tx_latecol" },
	{ "tx_retry_limit_hit" },
	{ "tx_carrier_sense_lost" },
	/*  5 */{ "tx_packets_needed_deferal" },
	{ "tx_packets_hit_deferal_timeout" },
	{ "tx_memory_bus_errors" },
	{ "tx_fifo_underruns" },
	{ "rx_ok_packets" },
	/* 10 */{ "rx_latecol" },
	{ "rx_crc_errors" },
	{ "rx_dribble_nibble" },
	{ "rx_max_frame_length_exceeded" },
	{ "rx_min_frame_length_not_reached" },
	/* 15 */{ "rx_invalid_symbol" },
	{ "rx_memory_bus_errors" },
	{ "rx_fifo_overrun" },
	{ "rx_missed" },
	{ "tx_octets" },
	/* 20 */{ "rx_octets" },
	{ "rx_multicast" },
	{ "tx_failed" },
	{ "rx_corrupt" },
};

static void oeth_get_strings(struct net_device *dev, u32 stringset, u8 *buf)
{
	struct oeth_private *cep = oeth_get_private(dev);

	switch(stringset) {
	case ETH_SS_STATS:
		if(!cep->status_cnt)
			break;
		memcpy(buf, &ethtool_stats_keys, sizeof(ethtool_stats_keys));
		break;
	case ETH_SS_TEST:
		break;
	default:
		WARN_ON(1); /* we need a WARN() */
		break;
	}
}

static void oeth_get_ethtool_stats(struct net_device *dev,
    struct ethtool_stats *estats, u64 *tmp_stats)
{
	struct oeth_private *cep = oeth_get_private(dev);
	int i;

	if(!cep->status_cnt)
		return;

	estats->n_stats = OETH_CNT_NUM;

	for(i = 0; i < OETH_CNT_NUM; i++)
		*tmp_stats++ = oeth_get_cnt(cep->base_addr, i);
}

static int oeth_get_coalesce(struct net_device *dev, struct ethtool_coalesce *ec)
{
	struct oeth_private *cep = oeth_get_private(dev);

	ec->tx_coalesce_usecs_irq = 0;
	ec->tx_max_coalesced_frames_irq = 0;
	spin_lock(&cep->intf_lock);
	ec->rx_coalesce_usecs = intf_get_delay0(cep->intf_base);
	ec->rx_max_coalesced_frames = intf_get_level0(cep->intf_base);
	ec->tx_coalesce_usecs = intf_get_delay1(cep->intf_base);
	ec->tx_max_coalesced_frames = intf_get_level1(cep->intf_base);
	spin_unlock(&cep->intf_lock);

	ec->rx_coalesce_usecs_irq = 0;
	ec->rx_max_coalesced_frames_irq = 0;

	/* XXX: Not implemented yet */
	ec->use_adaptive_rx_coalesce = 0;
	ec->rx_coalesce_usecs_low = 0;
	ec->rx_max_coalesced_frames_low = 0;
	ec->rx_coalesce_usecs_high = 0;
	ec->rx_max_coalesced_frames_high = 0;

	ec->use_adaptive_tx_coalesce = 0;
	ec->tx_coalesce_usecs_low = 0;
	ec->tx_max_coalesced_frames_low = 0;
	ec->tx_coalesce_usecs_high = 0;
	ec->tx_max_coalesced_frames_high = 0;

	/* XXX: Not implemented yet */
	ec->pkt_rate_low = 0;
	ec->pkt_rate_high = 0;
	ec->rate_sample_interval = 0;

	/* No in-memory stat counter, or only an in-memory stat counter */
	ec->stats_block_coalesce_usecs = 0;

	return 0;
}

static int oeth_set_coalesce(struct net_device *dev, struct ethtool_coalesce *ec)
{
	struct oeth_private *cep = oeth_get_private(dev);

	if(ec->rx_coalesce_usecs == 0 && ec->rx_max_coalesced_frames == 0)
		return -EINVAL;
	if(ec->rx_max_coalesced_frames > (cep->rx_bd_end - cep->tx_bds))
		return -EINVAL;
	if(ec->tx_coalesce_usecs == 0 && ec->tx_max_coalesced_frames == 0)
		return -EINVAL;
	if(ec->tx_max_coalesced_frames > cep->tx_bds)
		return -EINVAL;

	spin_lock(&cep->intf_lock);

	if(!ec->rx_coalesce_usecs)
		intf_disable_den0(cep->intf_base);

	intf_set_delay0(cep->intf_base, ec->rx_coalesce_usecs);

	if(ec->rx_coalesce_usecs)
		intf_enable_den0(cep->intf_base);

	if(ec->rx_max_coalesced_frames)
		intf_disable_len0(cep->intf_base);

	intf_set_level0(cep->intf_base, ec->rx_max_coalesced_frames);

	if(ec->rx_max_coalesced_frames)
		intf_enable_len0(cep->intf_base);

	if(!ec->tx_coalesce_usecs)
		intf_disable_den1(cep->intf_base);

	intf_set_delay1(cep->intf_base, ec->tx_coalesce_usecs);

	if(ec->tx_coalesce_usecs)
		intf_enable_den1(cep->intf_base);

	if(ec->tx_max_coalesced_frames)
		intf_disable_len1(cep->intf_base);

	intf_set_level1(cep->intf_base, ec->tx_max_coalesced_frames);

	if(ec->tx_max_coalesced_frames)
		intf_enable_len1(cep->intf_base);

	spin_unlock(&cep->intf_lock);

	return 0;
}

static const struct ethtool_ops oeth_ethtool_ops = {
	.get_settings       = oeth_get_settings,
	.set_settings       = oeth_set_settings,
	.get_drvinfo        = oeth_get_drvinfo,
	.get_regs_len       = oeth_get_regs_len,
	.get_regs       = oeth_get_regs,
	.nway_reset     = oeth_nway_reset,
	.get_link       = ethtool_op_get_link,
	.get_ringparam      = oeth_get_ringparam,
	.set_ringparam      = oeth_set_ringparam,
	.get_pauseparam     = oeth_get_pauseparam,
	.set_pauseparam     = oeth_set_pauseparam,
	.get_strings        = oeth_get_strings,
	.get_ethtool_stats  = oeth_get_ethtool_stats,
	.get_coalesce       = oeth_get_coalesce,
	.set_coalesce       = oeth_set_coalesce,
};

/*-------------------------------------------------------[ Initialisation ]---*/

/* Detects whether the ethernet has the status counters. */

static void  oeth_detect_features(struct oeth_private *cep)
{
	u8 tx[0x43] __attribute__((aligned(4))) = { 0, 0, 0, 0, 0, 0, 0x12 };
	u8 rx[0x44] __attribute__((aligned(4))) = { 0, 0, 0, 0 };
	dma_addr_t tx_dma;
	dma_addr_t rx_dma;
	int timeout = 500;

	cep->status_cnt = 0;

	oeth_set_moder(cep->base_addr, 0);
	oeth_set_packet_len(cep->base_addr, 0x00400600);

	oeth_set_int_mask(cep->base_addr, 0);
	oeth_set_int_mask2(cep->base_addr, 0);
	oeth_set_tx_bd_num(cep->base_addr, OETH_DEF_TXBD_NUM);

	oeth_set_bd_len_status(cep->base_addr, OETH_DEF_TXBD_NUM, 0);
	oeth_set_bd_addr(cep->base_addr, OETH_DEF_TXBD_NUM, 0);

	oeth_set_bd_len_status(cep->base_addr, 0, 0);
	oeth_set_bd_addr(cep->base_addr, 0, 0);

	oeth_set_moder(cep->base_addr, OETH_MODER_RXEN | OETH_MODER_TXEN |
	    OETH_MODER_LOOPBCK | OETH_MODER_PRO | OETH_MODER_CRCEN | OETH_MODER_FULLD);

	oeth_set_cnt(cep->base_addr, OETH_CNT_TXOK, 0);
	oeth_set_cnt(cep->base_addr, OETH_CNT_RXOK, 0);

	tx_dma = dma_map_single(NULL, tx + 3, 0x40, DMA_TO_DEVICE);
	rx_dma = dma_map_single(NULL, rx, 0x44, DMA_FROM_DEVICE);

	oeth_set_bd_addr(cep->base_addr, OETH_DEF_TXBD_NUM, rx_dma);
	oeth_set_bd_len_status(cep->base_addr, OETH_DEF_TXBD_NUM, OETH_RX_BD_EMPTY);

	oeth_set_bd_addr(cep->base_addr, 0, tx_dma);
	oeth_set_bd_len_status(cep->base_addr, 0,
	    OETH_TX_BD_CRC | OETH_TX_BD_READY | (0x40 << 16));

	while(oeth_get_bd_len_status(cep->base_addr, OETH_DEF_TXBD_NUM) & OETH_RX_BD_EMPTY) {
		udelay(5);

		/* loopback timed out, be conservative */
		if ((--timeout) <= 0) {
			printk(KERN_INFO "Ethernet init: Timeout while checking for counters support\n");
			return;
		}
	}

	oeth_set_moder(cep->base_addr, 0);

	if(oeth_get_cnt(cep->base_addr, OETH_CNT_TXOK) == 1 &&
	    oeth_get_cnt(cep->base_addr, OETH_CNT_RXOK) == 1)
		cep->status_cnt = 1;

	rmb();
	dma_unmap_single(NULL, tx_dma, 0x40, DMA_TO_DEVICE);
	dma_unmap_single(NULL, rx_dma, 0x44, DMA_FROM_DEVICE);

	oeth_set_int_mask2(cep->base_addr, OETH_INT_MASK_TX);
	if(oeth_get_int_mask2(cep->base_addr) != OETH_INT_MASK_TX)
		cep->mask2 = 0;
	else
		cep->mask2 = 1;

	oeth_set_int_mask2(cep->base_addr, 0);

	oeth_set_int_mask3(cep->base_addr, OETH_INT_MASK3_PHY);
	if(oeth_get_int_mask3(cep->base_addr) != OETH_INT_MASK3_PHY)
		cep->mask3 = 0;
	else
		cep->mask3 = 1;
}

static int oeth_detect_gbeth(struct oeth_private *cep)
{
	u32 st;

	oeth_set_speed_sel(cep->base_addr, OETH_SPEED_SEL_1000);
	st = oeth_get_speed_sel(cep->base_addr);
	oeth_set_speed_sel(cep->base_addr, cep->mode);

	return ((st & OETH_SPEED_SEL_MASK) == OETH_SPEED_SEL_1000);
}

static const struct net_device_ops beth_netdev_ops = {
	.ndo_open = oeth_open,
	.ndo_stop = oeth_close,
	.ndo_start_xmit = oeth_start_xmit,
	.ndo_get_stats = oeth_get_stats,
	.ndo_set_rx_mode = oeth_set_multicast_list,
	.ndo_set_mac_address = oeth_set_mac_add,
	.ndo_do_ioctl = oeth_ioctl,
	.ndo_change_mtu = oeth_change_mtu,
	/* This has poll_controller and tx_timeout fields */
};

/* Initialize the Beyond Ethernet MAC.
 */
int oeth_probe(struct platform_device *pdev)
{
	struct net_device *dev;
	struct oeth_private *cep = NULL;
	struct resource *res;
	int error, i, mdio_registered;
	void *base;

	mdio_registered = 0;

	/* Find the IRQ and the base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res)
		return -ENXIO;
	if((res->end - res->start + 1) < OETH_ADDR_SPACE) {
		dev_err(&pdev->dev, "I/O space is too small to be the beyond ethernet MAC\n");
		return -ENXIO;
	}

	base = ioremap_nocache(res->start, OETH_ADDR_SPACE);
	if(!base)
		return -ENOMEM;

	dev = alloc_etherdev(sizeof(*cep));
	if(!dev) {
		error = -ENOMEM;
		goto error;
	}
	dev->base_addr = res->start;

	dev->irq = platform_get_irq(pdev, 0);
	if(dev->irq < 0) {
		dev_err(&pdev->dev, "The beyond ethernet driver needs an IRQ to function\n");
		error = dev->irq;
		goto error;
	}

	SET_NETDEV_DEV(dev, &pdev->dev);
	platform_set_drvdata(pdev, dev);

	cep = oeth_get_private(dev);

	cep->base_addr = base;
	cep->dev = dev;

	cep->reconfigure = 0;
	oeth_detect_features(cep);
	cep->gbeth = oeth_detect_gbeth(cep);
	cep->mode = OETH_SPEED_SEL_100; /* The 1000Mbps detection clamps this to 100Mbps */
	cep->duplex = 1;
	cep->link = 0;
	cep->force_flow = 0;
	cep->tx_flow = 0;
	cep->rx_flow = 0;

	/* Note that this makes the core reset the "next BD to Rx/Tx" to 0,
	 * when Rx/Tx is enabled again. */
	oeth_set_moder(cep->base_addr, OETH_MODER_PAD | OETH_MODER_IFG |
	    OETH_MODER_CRCEN | OETH_MODER_IAM);

	cep->alloc_pkt_len = dev->mtu + ETH_PROTO_OVERHEAD + NET_IP_ALIGN;

	cep->tx_bds = OETH_DEF_TXBD_NUM;
	cep->rx_bd_end = cep->tx_bds + OETH_DEF_RXBD_NUM;

	/* Set the number of TX BDs */
	oeth_set_tx_bd_num(cep->base_addr, cep->tx_bds);

	/* Clear the BDs */
	for(i = 0; i < OETH_TOTAL_BD; i++) {
		oeth_set_bd_len_status(cep->base_addr, i, 0);
		oeth_set_bd_addr(cep->base_addr, i, 0);
	}

	/* Clear the status counters */
	for(i = 0; i < OETH_MAX_CNT; i++)
		oeth_set_cnt(cep->base_addr, i, 0);

	/* Initialize transmit pointers.  */
	cep->rx_tail = cep->tx_bds;

	cep->tx_head = 0;
	cep->tx_tail = 0;
	cep->tx_full = 0;

	/* Set min/max packet length */
	oeth_set_packet_len(cep->base_addr, ((ETH_ZLEN + ETH_FCS_LEN) << 16) |
	    (dev->mtu + ETH_PROTO_OVERHEAD));

	/* Set IPGT register to recomended value */
	oeth_set_ipgt(cep->base_addr, 0x00000008);

	/* Set IPGR1 register to recomended value */
	oeth_set_ipgr1(cep->base_addr, 0x0000000c);

	/* Set IPGR2 register to recomended value */
	oeth_set_ipgr2(cep->base_addr, 0x00000012);

	/* Initially disable flow control.  This shall be enabled in
	 * conf_mac_speed(), should it be needed */
	oeth_set_ctrlmoder(cep->base_addr, 0);

	dev->dev_addr[0] = (u8)(oeth_get_mac_addr1(cep->base_addr) >> 8);
	dev->dev_addr[1] = (u8)(oeth_get_mac_addr1(cep->base_addr) >> 0);
	dev->dev_addr[2] = (u8)(oeth_get_mac_addr0(cep->base_addr) >> 24);
	dev->dev_addr[3] = (u8)(oeth_get_mac_addr0(cep->base_addr) >> 16);
	dev->dev_addr[4] = (u8)(oeth_get_mac_addr0(cep->base_addr) >> 8);
	dev->dev_addr[5] = (u8)(oeth_get_mac_addr0(cep->base_addr) >> 0);

	/* If the boot loader never setup the address, give the core a random
	 * address */
	if(!(dev->dev_addr[2] | dev->dev_addr[3] | dev->dev_addr[4] |
	        dev->dev_addr[5])) {
		random_ether_addr(dev->dev_addr);
		upload_mac(dev);
	}

	/* Don't receive any MULTICAST packets */
	oeth_set_hash_addr0(cep->base_addr, 0);
	oeth_set_hash_addr1(cep->base_addr, 0);

	/* Clear all pending interrupts and disable interrupts */
	oeth_set_int_mask(cep->base_addr, 0);
	oeth_set_int_mask2(cep->base_addr, 0);
	oeth_set_int_src(cep->base_addr, 0xffffffff);

	dev->netdev_ops = &beth_netdev_ops;
	netif_napi_add(dev, &cep->napi, oeth_poll, 64);
	dev->ethtool_ops = &oeth_ethtool_ops;
	cep->napi.weight = cep->rx_bd_end - cep->tx_bds;

	spin_lock_init(&cep->lock);
	spin_lock_init(&cep->intf_lock);

	/*** Get configuration/state data from the ethernet mac ***/
	cep->intf_base = intf_init(dev, pdev);

	cep->mii_bus = mdiobus_alloc();
	if(cep->mii_bus == NULL) {
		error = -ENOMEM;
		goto error;
	}

	cep->mii_bus->priv = dev;
	cep->mii_bus->read = oeth_mdio_read;
	cep->mii_bus->write = oeth_mdio_write;
	cep->mii_bus->reset = oeth_mdio_reset;
	cep->mii_bus->name = "beth_mac_mdio";
	snprintf(cep->mii_bus->id, MII_BUS_ID_SIZE, "%x", pdev->id);
	//cep->mii_bus->irq = cep->mii_irqs;
	for(i = 0; i < PHY_MAX_ADDR; ++i)
		cep->mii_bus->irq[i] = PHY_POLL;
	cep->mii_bus->parent = &pdev->dev;

	if(mdiobus_register(cep->mii_bus)) {
		dev_err(&pdev->dev, "Cannot register MDIO bus!\n");
		error = -ENOMEM;
		goto error;
	}

	mdio_registered = 1;

	for(i = 0; i < PHY_MAX_ADDR; i++) {
		if(cep->mii_bus->mdio_map[i])
			break;
	}

	if(i == PHY_MAX_ADDR) {
		dev_err(&pdev->dev, "%s: Can't find the attached PHY\n", dev->name);
		error = -ENODEV;
		goto error;
	}

	cep->phydev = phy_connect(dev, dev_name(&cep->mii_bus->mdio_map[i]->dev),
	        conf_mac_speed, PHY_INTERFACE_MODE_GMII);
	if(!cep->phydev) {
		dev_err(&pdev->dev, "%s: Unable to attach to PHY\n", dev->name);
		error = -ENODEV;
		goto error;
	}

	/* Remove any features not supported by the controller */
	if(cep->gbeth)
		cep->phydev->supported &= PHY_GBIT_FEATURES;
	else
		cep->phydev->supported &= PHY_BASIC_FEATURES;
	cep->phydev->advertising = cep->phydev->supported;

#ifdef CONFIG_BETH_BASIC_FEATURES
	cep->phydev->advertising &= PHY_BASIC_FEATURES;
#endif

	netif_carrier_off(dev); /* The phy layer will bring it up for good */
	if((error = register_netdev(dev)))
		goto error;

	//  netdef_info(dev, "%s: Beyond Ethernet Controller at 0x%08x, %pM, IRQ %d\n",
	//              dev->base_addr, dev->dev_addr, pdev->irq);

	printk("%s: Beyond Ethernet Controller\n", dev->name);
	printk("Ethernet flags: ");
	if(cep->gbeth)
		printk("Gigabit");
	if(cep->status_cnt)
		printk(", counters");
	if(cep->mask2)
		printk(", mask2");
	if(cep->mask3)
		printk(", mask3");
#ifdef CONFIG_BETH_INT_FILTER
	printk(", coalesce");
#endif
	printk(", PHY driver: %s", cep->phydev->drv->name);
	printk("\n");

	init_timer(&cep->timer);
	cep->timer.data = (unsigned long)dev;
	cep->timer.function = oeth_timer;

	return 0;

error:
	iounmap(base);
	if(dev) {
		if(cep->phydev)
			phy_disconnect(cep->phydev);
		if(cep->mii_bus) {
			kfree(cep->mii_bus->irq);
			if(mdio_registered)
				mdiobus_unregister(cep->mii_bus);
		}
		free_netdev(dev);
	}

	return error;
}

static int oeth_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct oeth_private *cep = oeth_get_private(dev);

	platform_set_drvdata(pdev, NULL);

	phy_disconnect(cep->phydev);
	kfree(cep->mii_bus->irq);
	mdiobus_unregister(cep->mii_bus);

	unregister_netdev(dev);
	free_netdev(dev);

	return 0;
}
static const struct of_device_id beth_of_match[] = {
	{.compatible = "smartchip,smartx-gmac",},
};
MODULE_DEVICE_TABLE(of, beth_of_match);

static struct platform_driver beth_driver = {
	.probe = oeth_probe,
	.remove = oeth_remove,
	.resume = NULL,
	.suspend = NULL,
	.driver = {
		.name = "beyond_eth",
		.of_match_table = beth_of_match,
		.owner = THIS_MODULE,
	},
};
module_platform_driver(beth_driver);

