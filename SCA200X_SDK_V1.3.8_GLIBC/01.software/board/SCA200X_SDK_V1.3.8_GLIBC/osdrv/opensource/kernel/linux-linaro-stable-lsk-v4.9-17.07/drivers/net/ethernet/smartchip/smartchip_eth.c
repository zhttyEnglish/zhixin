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
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>
#include <linux/tcp.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/smartchip-pinshare.h>
#include "smartchip_eth.h"

#define ETHERNET_DEBUG_RANDOM_MAC_ADDR

#undef BUG_FIX_GARBAGE_DATA

#define OETH_MAX_BD_NUM  128
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

#define OETH_TX_TIMEOUT 5

enum tx_fifo_depth {
	FIFO_DEPTH_1_4 = 0,
	FIFO_DEPTH_2_4,
	FIFO_DEPTH_3_4,
	FIFO_DEPTH_FULL
};

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
	spinlock_t tx_lock;

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
	u32 tx_bds;         /* The number of BDs to use for TX */
	u32 rx_bds;         /* The number of BDs to use for RX */

	ushort tx_head;
	ushort tx_tail;
	ushort rx_tail;         /* Next buffer to be checked if packet received */

	struct device_node *phy_node;

	struct napi_struct napi;
	struct net_device_stats stats;
	phy_interface_t phy_interface;

	struct tasklet_struct tx_bdreclaim_tasklet;
	u32 msg_enable;

	atomic_t tx_free;
	struct work_struct      pending_work;

	u32 tx_fifo_depth;
	u32 tx_fifo_depth_1000;

	bool tx_enhance;
	bool rx_enhance;

	int phy_rst_gpio;

#ifdef BUG_FIX_GARBAGE_DATA
	u32 rx_buff_len;
	dma_addr_t phy_p_addr;
	void *phy_v_addr;
#endif
};

#define sc_eth_read(lp, reg)                        \
    readl_relaxed(((void __iomem *)((lp)->base_addr)) + (reg))
#define sc_eth_write(lp, reg, val)                  \
    writel_relaxed((val), ((void __iomem *)((lp)->base_addr)) + (reg))

static int sc_debug_level = -1;
module_param(sc_debug_level, int, 0);
MODULE_PARM_DESC(sc_debug_level, "sc_eth debug level (0=none,...,16=all)");

#define SC_DEFAULT_MSG_ENABLE   (NETIF_MSG_DRV | \
                 NETIF_MSG_PROBE | \
                 NETIF_MSG_LINK | \
                 NETIF_MSG_TIMER | \
                 NETIF_MSG_IFDOWN | \
                 NETIF_MSG_IFUP | \
                 NETIF_MSG_RX_ERR | \
                 NETIF_MSG_TX_ERR | \
                 NETIF_MSG_TX_QUEUED)

static irqreturn_t oeth_interrupt(int irq, void *dev_id);
static void reconf_mac(struct net_device *dev);
static void oeth_tx(struct net_device *dev);
static void oeth_free_buf(struct net_device *ndev);
static void adjust_mac_speed(struct net_device *dev);

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
#if 0
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
#endif

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

#if 0
#endif

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
#if 0
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
#endif
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
	int i = 0;
	struct oeth_private *cep = oeth_get_private(dev);

	for(i = 0; i < cep->tx_bds; i++) {
		cep->bd[i].b.sk = NULL;
		oeth_set_bd_addr(cep->base_addr, i, 0);
		init_tx_bd_param(cep, i);
	}
	oeth_or_bd_len_status(cep->base_addr, cep->tx_bds - 1, OETH_TX_BD_WRAP);

#ifndef BUG_FIX_GARBAGE_DATA

	for(i = cep->tx_bds; i < cep->rx_bd_end; i++) {
		struct  sk_buff *skb;

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

		cep->bd[i].phys = dma_map_single(dev->dev.parent, skb->data,
		        skb_tailroom(skb),
		        DMA_FROM_DEVICE);
		oeth_set_bd_addr(cep->base_addr, i, cep->bd[i].phys);
	}

#else
	cep->rx_buff_len = cep->rx_bds * cep->alloc_pkt_len + (8 * 1024);
	cep->phy_v_addr = dmam_alloc_noncoherent(dev->dev.parent, cep->rx_buff_len, &cep->phy_p_addr, GFP_KERNEL);
	if (!cep->phy_v_addr) {
		netdev_err(dev, "dmam_alloc_coherent fail \n");
		return -1;
	}

	netdev_info(dev, "allocate rx-bds buffer p:v(0x%llx:%p) size=0x%x \n", \
	    cep->phy_p_addr, cep->phy_v_addr, cep->rx_buff_len);

	for (i = cep->tx_bds; i < cep->rx_bd_end; i++) {
		oeth_set_bd_len_status(cep->base_addr, i, OETH_RX_BD_EMPTY | OETH_RX_BD_IRQ);
		cep->bd[i].phys = cep->phy_p_addr + ((i - cep->tx_bds) * cep->alloc_pkt_len);
		cep->bd[i].b.b  = cep->phy_v_addr + ((i - cep->tx_bds) * cep->alloc_pkt_len);

		oeth_set_bd_addr(cep->base_addr, i, cep->bd[i].phys);
	}
#endif

	oeth_or_bd_len_status(cep->base_addr, cep->rx_bd_end - 1, OETH_RX_BD_WRAP);

	return 0;
}

static int oeth_mdio_probe(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	struct phy_device *phydev;

	phydev = of_phy_connect(cep->dev, cep->phy_node, &adjust_mac_speed, 0, cep->phy_interface);
	if(!phydev) {
		netdev_err(cep->dev, "could not find phy.\n");
		return -ENODEV;
	}
	phydev->irq = PHY_POLL;
	phydev->supported &= PHY_GBIT_FEATURES;
	phydev->advertising = phydev->supported;

	return 0;
}

static int oeth_open(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	int rc = 0;

	spin_lock(&cep->lock);

	rc = oeth_alloc_buf(dev);
	if(rc < 0)
		goto out;
	cep->tx_tail = 0;
	cep->tx_head = 0;
	cep->rx_tail = cep->tx_bds;

	/* Clear all pending interrupts */
	oeth_set_int_src(cep->base_addr, 0xffffffff);

	rc = request_irq(dev->irq, oeth_interrupt, 0, "eth", dev);
	if(rc < 0) {
		oeth_free_buf(dev);
		goto out;
	}

	//hbbai: our GMAC only has int0, so only int_mask is used.
	//if(cep->mask2)
	//  oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_RX);
	//else
	oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_RX | OETH_INT_MASK_TX);
	oeth_or_moder(cep->base_addr, OETH_MODER_RXEN | OETH_MODER_TXEN);

	spin_unlock(&cep->lock);
#if 0
	//hbbai
	rc = oeth_mdio_probe(dev);
	if(rc < 0) {
		free_irq(dev->irq, dev);
		netdev_err(dev, "cannot probe MDIO bus.\n");
		return rc;
	}

	phy_start(dev->phydev);
#endif

	cep->link = 0;
	if(dev->phydev)
		phy_start(dev->phydev);

	//netif_start_queue(dev);
	netdev_reset_queue(dev);

	napi_enable(&cep->napi);
	tasklet_enable(&cep->tx_bdreclaim_tasklet);
	netif_wake_queue(dev);

	return 0;

out:
	spin_unlock(&cep->lock);
	return 0;
}

static void oeth_free_buf(struct net_device *ndev)
{
	int i;
	struct oeth_private *cep = oeth_get_private(ndev);
	for (i = 0; i < cep->tx_bds; i++) {
		if(cep->bd[i].b.sk) {
			dma_unmap_single(ndev->dev.parent, cep->bd[i].phys,
			    cep->bd[i].b.sk->len,
			    DMA_TO_DEVICE);
			dev_kfree_skb_any(cep->bd[i].b.sk);
		}
	}

#ifndef BUG_FIX_GARBAGE_DATA

	for(i = cep->tx_bds; i < cep->rx_bd_end; i++) {
		if(cep->bd[i].b.sk) {
			dma_unmap_single(ndev->dev.parent, cep->bd[i].phys,
			    skb_tailroom(cep->bd[i].b.sk),
			    DMA_FROM_DEVICE);
			dev_kfree_skb(cep->bd[i].b.sk);
		}
	}
#else
	dmam_free_coherent(ndev->dev.parent, cep->rx_buff_len, cep->phy_v_addr, cep->phy_p_addr);
#endif
}

/* This cleans up any remaining RX/TX buffers that are still around after
 * disabling RX/TX and havn't been cleaned up by oeth_rx() or oeth_tx(). */
static void oeth_flush_tx_rx(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 st;
	unsigned int tx_bytes = 0;
	unsigned int tx_packets = 0;
	/* Don't mark those frames as dropped that have been sucessfully TXd */
	oeth_tx(dev);

	/* TX buffer cleaning */
	for(;;) {
		if(cep->tx_bds == atomic_read(&cep->tx_free))
			break;
		st = oeth_get_bd_len_status(cep->base_addr, cep->tx_tail);

		cep->stats.tx_dropped++;
		if(!cep->status_cnt) {
			cep->stats.tx_packets++;
			cep->stats.collisions += (st & OETH_TX_BD_RETRY) >> 4;
		}

		init_tx_bd_param(cep, cep->tx_tail);

		++tx_packets;
		tx_bytes += cep->bd[cep->tx_tail].b.sk->len;

		dma_unmap_single(dev->dev.parent, cep->bd[cep->tx_tail].phys,
		    cep->bd[cep->tx_tail].b.sk->len,
		    DMA_TO_DEVICE);
		/* Free the skb associated with this last transmit */
		dev_kfree_skb_any(cep->bd[cep->tx_tail].b.sk);
		cep->bd[cep->tx_tail].b.sk = NULL;
		cep->tx_tail = (cep->tx_tail + 1) % cep->tx_bds;
		atomic_inc(&cep->tx_free);
		cep->stats.tx_packets += tx_packets;
		cep->stats.tx_bytes += tx_bytes;
	}

	netdev_completed_queue(dev, tx_packets, tx_bytes);
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

	tasklet_disable(&cep->tx_bdreclaim_tasklet);

	napi_disable(&cep->napi);

	netif_stop_queue(dev);

	del_timer_sync(&cep->timer);

	spin_lock(&cep->lock);

	oeth_set_int_mask(cep->base_addr, 0);

	oeth_clear_moder(cep->base_addr, OETH_MODER_RXEN | OETH_MODER_TXEN);

	free_irq(dev->irq, (void *)dev);

	oeth_flush_tx_rx(dev);

	oeth_free_buf(dev);
	if(dev->phydev)
		phy_stop(dev->phydev);

	spin_unlock(&cep->lock);

	return 0;
}

static netdev_tx_t oeth_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 st = 0;

	if (0 == atomic_read(&cep->tx_free)) {
		netif_stop_queue(dev);
		return NETDEV_TX_BUSY;
	}

	spin_lock(&cep->tx_lock);

	cep->bd[cep->tx_head].phys = dma_map_single(cep->dev->dev.parent, skb->data, skb->len,
	        DMA_TO_DEVICE);

	oeth_set_bd_addr(cep->base_addr, cep->tx_head,
	    cep->bd[cep->tx_head].phys);

	cep->bd[cep->tx_head].b.sk = skb;

	/* Send it on its way.  Tell controller it's ready, and to put the CRC
	 * on the end. */
	st = OETH_TX_BD_SEND | (skb->len << 16);
	if (unlikely(cep->tx_head == cep->tx_bds - 1)) {
		st |= OETH_TX_BD_WRAP;
	}

	oeth_set_bd_len_status(cep->base_addr, cep->tx_head, st);
	wmb();
	//atomic_dec(&cep->tx_free);
	cep->tx_head = (cep->tx_head + 1) % cep->tx_bds;
	netdev_sent_queue(dev, skb->len);
	atomic_dec(&cep->tx_free);

	spin_unlock(&cep->tx_lock);

	netif_trans_update(dev);

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
	oeth_set_int_src(cep->base_addr, int_events);

	mask = oeth_get_int_mask(cep->base_addr);
	//mask |= oeth_get_int_mask2(cep->base_addr); hbbai: we only use int_mask
	int_events &= mask;

	/* For shared interrupts.. */
	if(!int_events)
		return IRQ_NONE;

	if(int_events & OETH_INT_TX) {
		/* Waking the queue will have the desired effect that
		 * start_xmit() will be called hence oeth_tx() and then thank
		 * goodness we don't have to take a lock here :) */
		tasklet_schedule(&cep->tx_bdreclaim_tasklet);
	}

	if(int_events & OETH_INT_RX) {
		oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_TX);
		napi_schedule(&cep->napi); /* schedule NAPI poll */
	}

	return IRQ_HANDLED;
}

static void oeth_tx(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	u32 st;

	unsigned int tx_bytes = 0;
	unsigned int tx_packets = 0;

	while(atomic_read(&cep->tx_free) < cep->tx_bds) {

		st = oeth_get_bd_len_status(cep->base_addr, cep->tx_tail);

		if(st & OETH_TX_BD_READY) {
			break;
		}

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

			cep->stats.collisions += (st & OETH_TX_BD_RETRY) >> 4;
		}

		BUG_ON(!cep->bd[cep->tx_tail].b.sk);
		++tx_packets;
		tx_bytes += cep->bd[cep->tx_tail].b.sk->len;

		BUG_ON(st & OETH_TX_BD_BUS_ERR);

		dma_unmap_single(cep->dev->dev.parent, cep->bd[cep->tx_tail].phys,
		    cep->bd[cep->tx_tail].b.sk->len,
		    DMA_TO_DEVICE);
		/* Free the skb associated with this last transmit */
		dev_kfree_skb_any(cep->bd[cep->tx_tail].b.sk);
		cep->bd[cep->tx_tail].b.sk = NULL;

		atomic_inc(&cep->tx_free);
		cep->tx_tail = (cep->tx_tail + 1) % cep->tx_bds;
	}

	cep->stats.tx_packets += tx_packets;
	cep->stats.tx_bytes += tx_bytes;
	netdev_completed_queue(dev, tx_packets, tx_bytes);

}

static void oeth_tx_reclaim(unsigned long data)
{
	struct net_device *ndev = (struct net_device *)data;
	struct oeth_private *cep = oeth_get_private(ndev);
	u32 st;

	unsigned int tx_bytes = 0;
	unsigned int tx_packets = 0;

	while(atomic_read(&cep->tx_free) < cep->tx_bds) {

		st = oeth_get_bd_len_status(cep->base_addr, cep->tx_tail);

		if(st & OETH_TX_BD_READY) {
			break;
		}

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

			cep->stats.collisions += (st & OETH_TX_BD_RETRY) >> 4;
		}

		BUG_ON(!cep->bd[cep->tx_tail].b.sk);

		++tx_packets;
		tx_bytes += cep->bd[cep->tx_tail].b.sk->len;

		BUG_ON(st & OETH_TX_BD_BUS_ERR);

		dma_unmap_single(cep->dev->dev.parent, cep->bd[cep->tx_tail].phys,
		    cep->bd[cep->tx_tail].b.sk->len,
		    DMA_TO_DEVICE);
		/* Free the skb associated with this last transmit */
		dev_kfree_skb_any(cep->bd[cep->tx_tail].b.sk);
		cep->bd[cep->tx_tail].b.sk = NULL;

		atomic_inc(&cep->tx_free);
		cep->tx_tail = (cep->tx_tail + 1) % cep->tx_bds;
	}
	cep->stats.tx_packets += tx_packets;
	cep->stats.tx_bytes += tx_bytes;
	netdev_completed_queue(ndev, tx_packets, tx_bytes);
	netif_wake_queue(ndev);

}

static void oeth_rx_unalign(struct net_device *dev, int pkt_len)
{
	struct oeth_private *cep = oeth_get_private(dev);
	dma_addr_t phys = cep->bd[cep->rx_tail].phys;
	struct sk_buff *skb_alloc = NULL;

#ifndef BUG_FIX_GARBAGE_DATA
	skb_alloc = netdev_alloc_skb(dev, cep->alloc_pkt_len);
#else
	dma_sync_single_for_cpu(dev->dev.parent, phys, pkt_len, DMA_FROM_DEVICE);
	skb_alloc = netdev_alloc_skb(dev, pkt_len + 64);
#endif

	if (likely(skb_alloc)) {
#ifndef BUG_FIX_GARBAGE_DATA
		struct sk_buff *skb;
		dma_addr_t new_skb_baddr;
#endif

		skb_reserve(skb_alloc, NET_IP_ALIGN);
#ifndef BUG_FIX_GARBAGE_DATA
		skb = cep->bd[cep->rx_tail].b.sk;
		new_skb_baddr  = dma_map_single(dev->dev.parent,
		        skb_alloc->data,
		        skb_tailroom(skb_alloc),
		        DMA_FROM_DEVICE);
		if (dma_mapping_error(dev->dev.parent, new_skb_baddr)) {
			netdev_err(dev, "DMA map error\n");
			dev_kfree_skb(skb_alloc);
			return;
		}

		dma_unmap_single(dev->dev.parent,
		    phys,
		    skb_tailroom(skb), DMA_FROM_DEVICE);

		skb_put(skb, pkt_len);
		skb->protocol = eth_type_trans(skb, dev);
		skb_alloc->dev = dev;

		netif_receive_skb(skb);

		cep->bd[cep->rx_tail].phys = new_skb_baddr;
		cep->bd[cep->rx_tail].b.sk = skb_alloc;

		oeth_set_bd_addr(cep->base_addr, cep->rx_tail, cep->bd[cep->rx_tail].phys);
#else
		__memcpy_fromio(skb_alloc->data, cep->bd[cep->rx_tail].b.b, pkt_len);
		skb_put(skb_alloc, pkt_len);
		skb_alloc->protocol = eth_type_trans(skb_alloc, dev);
		skb_alloc->dev = dev;

		netif_receive_skb(skb_alloc);
#endif
		wmb();
		if(!cep->status_cnt) {
			cep->stats.rx_packets++;
			cep->stats.rx_bytes += pkt_len;
		}
	} else {
		cep->stats.rx_dropped++;
		netif_err(cep, rx_err, dev, "rx rec error\n");
	}
}

static int oeth_rx(struct net_device *dev, int budget)
{
	u32 st = 0;
	int work_done = 0;

	struct oeth_private *cep = oeth_get_private(dev);

	for(work_done = 0; work_done < budget; work_done++, rx_pop(cep)) {
		st = oeth_get_bd_len_status(cep->base_addr, cep->rx_tail);

		if(st & OETH_RX_BD_EMPTY)
			return work_done;

		st |= OETH_RX_BD_EMPTY;

		/* Receive packet now, ask questions later */
		if (likely(!(st & OETH_RX_BD_STATS & ~OETH_RX_BD_MISS & ~OETH_RX_BD_MCAST))) {
			oeth_rx_unalign(dev, st >> 16);

			if(!cep->status_cnt && (st & OETH_RX_BD_STATS))
				cep->stats.multicast++;

			st &= ~(OETH_RX_BD_MISS | OETH_RX_BD_MCAST);
			oeth_set_bd_len_status(cep->base_addr, cep->rx_tail, st);
			wmb();
			continue;
		}

		oeth_set_bd_len_status(cep->base_addr, cep->rx_tail, st & ~OETH_RX_BD_STATS);
		wmb();
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

	return work_done;
}

static int oeth_poll(struct napi_struct *napi, int budget)
{
	struct oeth_private *cep = container_of(napi, struct oeth_private, napi);

	int work_done = 0;
	u32 st;

	work_done = oeth_rx(cep->dev, budget);
#if 0
	if(unlikely(cep->reconfigure))
		reconf_mac(cep->dev);
#endif
	st = oeth_get_bd_len_status(cep->base_addr, cep->rx_tail);
	if((st & OETH_RX_BD_EMPTY) && (work_done < budget)) {
		napi_complete(napi);

		oeth_set_int_mask(cep->base_addr, OETH_INT_MASK_RX |
		    OETH_INT_MASK_TX);
	}  else {
		work_done = budget;
	}

	return work_done;
}

/*-----------------------------------------------------[ Driver callbacks ]---*/
#if 0
static struct rtnl_link_stats64 *oeth_get_stats64(struct net_device *dev)
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

#endif
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

		hash_b = (~ crc32_le(~0, ha->addr, ETH_ALEN)) & 0x3f;
		hash[hash_b >> 5] |= 1 << (hash_b & 0x1f);
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

	__memcpy_toio(dev->dev_addr, addr->sa_data, dev->addr_len);

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

	netdev_info(dev, "MTU changing to %d, reopen %d\n", new_mtu, reopen);

	if (reopen)
		oeth_close(dev);

	dev->mtu = new_mtu;
	netdev_info(dev, "MTU change: set packet len %d\n", \
	    ((ETH_ZLEN + ETH_FCS_LEN) << 16) | (new_mtu + ETH_PROTO_OVERHEAD));

	cep->alloc_pkt_len = new_mtu + ETH_PROTO_OVERHEAD + NET_IP_ALIGN;
	oeth_set_packet_len(cep->base_addr, ((ETH_ZLEN + ETH_FCS_LEN) << 16) |
	    (new_mtu + ETH_PROTO_OVERHEAD));

	if (reopen) {
		rc = oeth_open(dev);
		return rc;
	}

	return 0;
}

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

	//del_timer_sync(&cep->timer);

	//spin_lock_bh(&cep->lock);

	/* This needs to be done, since the core needs to switch PHY clocks,
	 * which would screw up RX/TX anyway.. */
	st = oeth_get_moder(cep->base_addr);
	oeth_set_moder(cep->base_addr, st & ~(OETH_MODER_RXEN | OETH_MODER_TXEN));
	netif_tx_lock(dev);

	oeth_flush_tx_rx(dev);
	cep->tx_tail = 0;
	cep->tx_head = 0;
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
		st |= (cep->tx_fifo_depth_1000 << 17);
	} else {
		oeth_set_ipgt(cep->base_addr, 0x00000008);
		oeth_set_collconf(cep->base_addr, 0x000f003f);

		st &= ~OETH_MODER_FIFO;
		st |= (cep->tx_fifo_depth << 17);
	}
	oeth_set_speed_sel(cep->base_addr, cep->mode);

	cep->reconfigure = 0;

	/* Ok, we have reconfigured, let's go! */
	oeth_set_moder(cep->base_addr, st);
	netif_tx_unlock(dev);
	netif_trans_update(dev);
	//spin_unlock_bh(&cep->lock);
}

static void adjust_mac_speed(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);
	struct phy_device *phydev = dev->phydev;
	int duplex;
	int mode;
	int flow;

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

	reconf_mac(cep->dev);
#if 0
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
#endif

unlock:
	phy_print_status(phydev);
	spin_unlock_bh(&cep->lock);
}

/*----------------------------------------------------------------[ IOCTL ]---*/
static int oeth_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	if(!netif_running(dev))
		return -EINVAL;

	return phy_mii_ioctl(dev->phydev, rq, cmd);
}

/*------------------------------------------------------[ Ethtool support ]---*/
static int oeth_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	if(dev->phydev)
		return phy_ethtool_gset(dev->phydev, cmd);

	return -EINVAL;
}

static int oeth_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	if(!capable(CAP_NET_ADMIN))
		return -EPERM;

	if(dev->phydev)
		return phy_ethtool_sset(dev->phydev, cmd);

	return -EINVAL;
}

static int oeth_nway_reset(struct net_device *dev)
{
	if(!netif_running(dev))
		return -EINVAL;

	return phy_start_aneg(dev->phydev);
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
		oeth_free_buf(dev);
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

		oldadv = dev->phydev->advertising & (ADVERTISED_Pause |
		        ADVERTISED_Asym_Pause);
		if(oldadv != newadv) {
			dev->phydev->advertising &= ~(ADVERTISED_Pause |
			        ADVERTISED_Asym_Pause);
			dev->phydev->advertising |= newadv;
			err = phy_start_aneg(dev->phydev);
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

static u32 oeth_get_msglevel(struct net_device *ndev)
{
	struct oeth_private *cep = oeth_get_private(ndev);

	return cep->msg_enable;
}

static void oeth_set_msglevel(struct net_device *ndev, u32 msglevel)
{
	struct oeth_private *cep = oeth_get_private(ndev);

	cep->msg_enable = msglevel;
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
	.get_msglevel   = oeth_get_msglevel,
	.set_msglevel   = oeth_set_msglevel,
};

/*-------------------------------------------------------[ Initialisation ]---*/

#if 0
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
	//oeth_set_int_mask2(cep->base_addr, 0);
	oeth_set_tx_bd_num(cep->base_addr, cep->tx_bds);

	oeth_set_bd_len_status(cep->base_addr, cep->tx_bds, 0);
	oeth_set_bd_addr(cep->base_addr, cep->tx_bds, 0);

	oeth_set_bd_len_status(cep->base_addr, 0, 0);
	oeth_set_bd_addr(cep->base_addr, 0, 0);

	oeth_set_moder(cep->base_addr, OETH_MODER_RXEN | OETH_MODER_TXEN |
	    OETH_MODER_LOOPBCK | OETH_MODER_PRO | OETH_MODER_CRCEN | OETH_MODER_FULLD);

	oeth_set_cnt(cep->base_addr, OETH_CNT_TXOK, 0);
	oeth_set_cnt(cep->base_addr, OETH_CNT_RXOK, 0);

	tx_dma = dma_map_single(cep->dev->dev.parent, tx + 3, 0x40, DMA_TO_DEVICE);
	rx_dma = dma_map_single(cep->dev->dev.parent, rx, 0x44, DMA_FROM_DEVICE);

	oeth_set_bd_addr(cep->base_addr, cep->tx_bds, rx_dma);
	oeth_set_bd_len_status(cep->base_addr, cep->tx_bds, OETH_RX_BD_EMPTY);

	oeth_set_bd_addr(cep->base_addr, 0, tx_dma);
	oeth_set_bd_len_status(cep->base_addr, 0,
	    OETH_TX_BD_CRC | OETH_TX_BD_READY | (0x40 << 16));

	while(oeth_get_bd_len_status(cep->base_addr, cep->tx_bds) & OETH_RX_BD_EMPTY) {
		udelay(5);

		/* loopback timed out, be conservative */
		if ((--timeout) <= 0) {
			printk(KERN_INFO "Ethernet init: Timeout while checking for counters support\n");
			break;
		}
	}

	oeth_set_moder(cep->base_addr, 0);

	if(oeth_get_cnt(cep->base_addr, OETH_CNT_TXOK) == 1 &&
	    oeth_get_cnt(cep->base_addr, OETH_CNT_RXOK) == 1)
		cep->status_cnt = 1;

	rmb();
	dma_unmap_single(cep->dev->dev.parent, tx_dma, 0x40, DMA_TO_DEVICE);
	dma_unmap_single(cep->dev->dev.parent, rx_dma, 0x44, DMA_FROM_DEVICE);
#if 0
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
#endif

}
#endif

static int oeth_detect_gbeth(struct oeth_private *cep)
{
	u32 st;

	oeth_set_speed_sel(cep->base_addr, OETH_SPEED_SEL_1000);
	st = oeth_get_speed_sel(cep->base_addr);
	oeth_set_speed_sel(cep->base_addr, cep->mode);

	return ((st & OETH_SPEED_SEL_MASK) == OETH_SPEED_SEL_1000);
}

static void oeth_set_bridge(struct net_device *ndev,
    bool flag_tx_enhance,
    bool flag_rx_enhance)
{
	unsigned int tx_reg = 0;
	unsigned int rx_reg = 0;
	struct oeth_private *cep = oeth_get_private(ndev);

	tx_reg =  sc_eth_read(cep, SMARTX_ETH_TX_BRIDGE);
	rx_reg =  sc_eth_read(cep, SMARTX_ETH_RX_BRIDGE);

	if (flag_tx_enhance)
		sc_eth_write(cep, SMARTX_ETH_TX_BRIDGE, tx_reg | 0x1);
	else
		sc_eth_write(cep, SMARTX_ETH_TX_BRIDGE, tx_reg & 0xfffffffe);

	if (flag_rx_enhance)
		sc_eth_write(cep, SMARTX_ETH_RX_BRIDGE, rx_reg | 0x1);
	else
		sc_eth_write(cep, SMARTX_ETH_RX_BRIDGE, rx_reg & 0xfffffffe);
}

static void oeth_pending_work(struct work_struct *work)
{
	struct oeth_private *cep = container_of(work, struct oeth_private, pending_work);

	rtnl_lock();
	oeth_close(cep->dev);
	oeth_open(cep->dev);
	rtnl_unlock();
}

static void oeth_tx_timeout(struct net_device *dev)
{
	struct oeth_private *cep = oeth_get_private(dev);

	netdev_err(cep->dev, "transmit timeout %d s, resetting...\n",
	    OETH_TX_TIMEOUT);

	schedule_work(&cep->pending_work);
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
	.ndo_tx_timeout     = oeth_tx_timeout,
	//  .ndo_get_stats64        = oeth_get_stats64,
	/* This has poll_controller and tx_timeout fields */
};

/*
 * Initialize the Beyond Ethernet MAC.
 */
int oeth_probe(struct platform_device *pdev)
{
	void __iomem *base = NULL;
	struct net_device *ndev;
	struct oeth_private *cep = NULL;
	struct device_node *np = pdev->dev.of_node;
	int error = 0, i;
	int weight = 0;
	unsigned int st = 0;

	pr_info("Smartx eth probe...\n");
	ndev = alloc_etherdev(sizeof(*cep));
	if (!ndev) {
		dev_err(&pdev->dev, "could not allocate device.\n");
		error = -ENOMEM;
		goto error;
	}

	base = of_iomap(np, 0);
	if(!base) {
		dev_err(&pdev->dev, "failed to remap registers.\n");
		return -ENOMEM;
	}

	ndev->base_addr = (unsigned long)base;

	ndev->irq = irq_of_parse_and_map(np, 0);
	if(ndev->irq < 0) {
		dev_err(&pdev->dev, "The beyond ethernet driver needs an IRQ to function\n");
		error = ndev->irq;
		goto error;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);
	platform_set_drvdata(pdev, ndev);

	cep = oeth_get_private(ndev);

	cep->base_addr = (void __iomem *)base;
	cep->dev = ndev;
	cep->phy_interface = of_get_phy_mode(np);

	if(cep->phy_interface < 0)
		cep->phy_interface = PHY_INTERFACE_MODE_RGMII_ID;

	if(cep->phy_interface == PHY_INTERFACE_MODE_RMII) {
		st = sc_eth_read(cep, SMARTX_ETH_RMII_CTRL);
		sc_eth_write(cep, SMARTX_ETH_RMII_CTRL, st | (SMARTX_ETH_RMII_SEL));
	}

	cep->msg_enable = netif_msg_init(sc_debug_level, SC_DEFAULT_MSG_ENABLE);

	if (of_property_read_u32(np, "tx-fifo-depth", &cep->tx_fifo_depth)) {
		dev_err(&pdev->dev, "tx-fifo-depth use default %d \n", cep->tx_fifo_depth);
		cep->tx_fifo_depth  = FIFO_DEPTH_FULL;
	}

	if (of_property_read_u32(np, "tx-fifo-depth-1000", &cep->tx_fifo_depth_1000)) {
		dev_err(&pdev->dev, "tx-fifo-depth-1000 use default %d\n", cep->tx_fifo_depth_1000);
		cep->tx_fifo_depth_1000  = FIFO_DEPTH_FULL;
	}

	if (of_property_read_u32(np, "tx-bd-num", &cep->tx_bds)) {
		dev_err(&pdev->dev, "tx_bd_num = %d\n", cep->tx_bds);
		goto error;
	}
	BUG_ON(cep->tx_bds >= OETH_MAX_BD_NUM);

	cep->rx_bds = OETH_MAX_BD_NUM - cep->tx_bds;
	cep->rx_bd_end = OETH_MAX_BD_NUM;

	cep->tx_enhance = of_property_read_bool(np, "tx-enhance") ? 1 : 0;
	cep->rx_enhance = of_property_read_bool(np, "rx-enhance") ? 1 : 0;

	sc_eth_write(cep, SMARTX_ETH_PWR_CTRL, 0xffffff00);
	oeth_set_bridge(ndev, cep->tx_enhance, cep->rx_enhance);

	cep->reconfigure = 0;
	/*Disable the counter check, because the BD is not cleared here*/
	cep->status_cnt = 0;
	//oeth_detect_features(cep);
	cep->gbeth = oeth_detect_gbeth(cep);
	cep->mode = OETH_SPEED_SEL_1000; /*The 1000Mbps detection clamps this to 100Mbps*/
	cep->duplex = 1;
	cep->link = 0;
	cep->force_flow = 0;
	cep->tx_flow = 0;
	cep->rx_flow = 0;

	atomic_set(&cep->tx_free, cep->tx_bds);

	/* Note that this makes the core reset the "next BD to Rx/Tx" to 0,
	 * when Rx/Tx is enabled again. */
	oeth_set_moder(cep->base_addr, OETH_MODER_PAD | OETH_MODER_IFG |
	    OETH_MODER_CRCEN | OETH_MODER_IAM);

	cep->alloc_pkt_len = ndev->mtu + ETH_PROTO_OVERHEAD + NET_IP_ALIGN;

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

	/* Set min/max packet length */
	oeth_set_packet_len(cep->base_addr, ((ETH_ZLEN + ETH_FCS_LEN) << 16) |
	    (ndev->mtu + ETH_PROTO_OVERHEAD));

	//hbbai: since most of the ethernet cards are capable of 1G, so we just set default value to 1G
	oeth_set_speed_sel(cep->base_addr, OETH_SPEED_SEL_1000);
	oeth_set_ipgt(cep->base_addr, 0x00000004);
	oeth_set_collconf(cep->base_addr, 0x000f01ff);

	st = oeth_get_moder(cep->base_addr);
	st &= ~OETH_MODER_FIFO;
	st |= OETH_MODER_FIFOF;
	st |= OETH_MODER_FULLD;

	oeth_set_moder(cep->base_addr, st);

	/* Initially disable flow control.  This shall be enabled in
	 * conf_mac_speed(), should it be needed */
	oeth_set_ctrlmoder(cep->base_addr, 0);

	ndev->dev_addr[0] = (u8)(oeth_get_mac_addr1(cep->base_addr) >> 8);
	ndev->dev_addr[1] = (u8)(oeth_get_mac_addr1(cep->base_addr) >> 0);
	ndev->dev_addr[2] = (u8)(oeth_get_mac_addr0(cep->base_addr) >> 24);
	ndev->dev_addr[3] = (u8)(oeth_get_mac_addr0(cep->base_addr) >> 16);
	ndev->dev_addr[4] = (u8)(oeth_get_mac_addr0(cep->base_addr) >> 8);
	ndev->dev_addr[5] = (u8)(oeth_get_mac_addr0(cep->base_addr) >> 0);

	/* If the boot loader never setup the address, give the core a random
	 * address */
#ifndef ETHERNET_DEBUG_RANDOM_MAC_ADDR
	if(!(ndev->dev_addr[2] | ndev->dev_addr[3] | ndev->dev_addr[4] |
	        ndev->dev_addr[5]))
#endif
	{
		random_ether_addr(ndev->dev_addr);
		upload_mac(ndev);
	}

	/* Don't receive any MULTICAST packets */
	oeth_set_hash_addr0(cep->base_addr, 0);
	oeth_set_hash_addr1(cep->base_addr, 0);

	/* Clear all pending interrupts and disable interrupts */
	oeth_set_int_mask(cep->base_addr, 0);
	oeth_set_int_mask2(cep->base_addr, 0);
	oeth_set_int_src(cep->base_addr, 0xffffffff);

	ndev->watchdog_timeo = 5 * HZ;
	INIT_WORK(&cep->pending_work, oeth_pending_work);
	ndev->netdev_ops = &beth_netdev_ops;
	ndev->ethtool_ops = &oeth_ethtool_ops;
	cep->napi.weight = cep->rx_bd_end - cep->tx_bds;

	weight = cep->napi.weight > NAPI_POLL_WEIGHT ? NAPI_POLL_WEIGHT : cep->napi.weight;
	netif_napi_add(ndev, &cep->napi, oeth_poll, weight);

	spin_lock_init(&cep->lock);
	spin_lock_init(&cep->intf_lock);
	spin_lock_init(&cep->tx_lock);

	/*** Get configuration/state data from the ethernet mac ***/
	//cep->intf_base = intf_init(ndev, pdev);

	cep->phy_node = of_parse_phandle(np, "phy-handle", 0);
	if(!cep->phy_node) {
		dev_err(&pdev->dev, "No associated PHY.\n");
		error = -ENODEV;
		goto error;
	}

	netif_carrier_off(ndev); /* The phy layer will bring it up for good */
	error = oeth_mdio_probe(ndev);
	if(error < 0) {
		netdev_err(ndev, "cannot probe MDIO bus.\n");
		goto error;
	}

	if((error = register_netdev(ndev)))
		goto error;

	if (netif_msg_probe(cep))
		netdev_dbg(ndev, "net_local@%p\n", cep);

	dev_info(&pdev->dev, "%s: Beyond Ethernet Controller at 0x%08lx, %pM, IRQ %d" \
	    "tx_fifo:%d tx_fifo_1000: %d\n""tx_enable: %d rx_enable: %d", \
	    ndev->name, ndev->base_addr, ndev->dev_addr, ndev->irq,  \
	    cep->tx_fifo_depth, cep->tx_fifo_depth_1000,
	    cep->tx_enhance, cep->rx_enhance);

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
	printk("\n");

	init_timer(&cep->timer);
	cep->timer.data = (unsigned long)ndev;
	cep->timer.function = oeth_timer;

	tasklet_init(&cep->tx_bdreclaim_tasklet, oeth_tx_reclaim, (unsigned long)ndev);
	tasklet_disable(&cep->tx_bdreclaim_tasklet);

	return 0;

error:
	netif_napi_del(&cep->napi);
	iounmap(base);
	if(ndev->phydev)
		phy_disconnect(ndev->phydev);

	dev_err(&pdev->dev, "not found (%d).\n", error);
	if(ndev) {
		free_netdev(ndev);
	}

	return error;
}

static int oeth_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
#if 0
	phy_disconnect(dev->phydev);
	kfree(cep->mii_bus->irq);
	mdiobus_unregister(cep->mii_bus);
#endif
	unregister_netdev(ndev);
	free_netdev(ndev);

	return 0;
}

static const struct of_device_id beth_of_match[] = {
	{.compatible = "smartchip,smartx-gmac",},
	{},
};
MODULE_DEVICE_TABLE(of, beth_of_match);

static struct platform_driver beth_driver = {
	.probe = oeth_probe,
	.remove = oeth_remove,
	.resume = NULL,
	.suspend = NULL,
	.driver = {
		.name = "smartx-gmac",
		.of_match_table = beth_of_match,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(beth_driver);

MODULE_AUTHOR("Robin bai <haibin.bai@smartchip.cn>");
MODULE_DESCRIPTION("SmartChip Smartx GMAC driver based on beth");
MODULE_LICENSE("GPL");

