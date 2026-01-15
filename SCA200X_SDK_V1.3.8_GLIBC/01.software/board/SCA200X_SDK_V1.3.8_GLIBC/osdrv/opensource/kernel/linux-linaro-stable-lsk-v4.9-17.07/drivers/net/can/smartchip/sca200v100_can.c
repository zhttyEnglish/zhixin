/*
 * CAN bus driver for sca200v100 can controller
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
#include <linux/can/led.h>

#include "sca200v100_can.h"

static const struct can_bittiming_const sca200v100_can_bittiming_const = {
	.name = SCA200V100_CAN_DEV_NAME,
	.tseg1_min = 3,        /* Time segment 1 = prop_seg + phase_seg1 */
	.tseg1_max = 65,
	.tseg2_min = 2,        /* Time segment 2 = phase_seg2 */
	.tseg2_max = 8,
	.sjw_max = 8,
	.brp_min = 1,
	.brp_max = 256,
	.brp_inc = 1,
};

static void sca200v100_can_tx_queue_work(struct work_struct *work);

static u32 sca200v100_can_tx_done(struct net_device *ndev, u8 isr0, u8 isr1)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	u32 ctrl = 0;

	if(isr1 & SCA200V100_CAN_RTIF_TPIF) {
		stats->tx_packets++;
		stats->tx_bytes += priv->tx_dlc[priv->tx_tail %
		                      SCA200V100_CAN_TX_FIFO_NUM];
		priv->tx_dlc[priv->tx_tail % SCA200V100_CAN_TX_FIFO_NUM] = 0;
		can_get_echo_skb(ndev, priv->tx_tail % SCA200V100_CAN_TX_FIFO_NUM);
		priv->tx_tail++;

		if(priv->tx_tail > priv->tx_head)
			priv->tx_tail = priv->tx_head;

		netif_wake_queue(ndev);

		ctrl |= SCA200V100_CAN_RTIF_TPIF;
	}

	if(isr1 & SCA200V100_CAN_RTIF_TSIF) {
		stats->tx_packets++;
		stats->tx_bytes += priv->tx_dlc[priv->tx_tail %
		                      SCA200V100_CAN_TX_FIFO_NUM];
		priv->tx_dlc[priv->tx_tail % SCA200V100_CAN_TX_FIFO_NUM] = 0;
		can_get_echo_skb(ndev, priv->tx_tail % SCA200V100_CAN_TX_FIFO_NUM);
		priv->tx_tail++;
		netif_wake_queue(ndev);

		if(priv->tx_tail > priv->tx_head)
			priv->tx_tail = priv->tx_head;

		ctrl |= SCA200V100_CAN_RTIF_TSIF;
	}

	if(isr0 & SCA200V100_CAN_RTIE_TSFF) {
		++priv->tx_full_num;
		//netdev_err(ndev, "tx full %d\n", priv->tx_full_num);
	}

	return (ctrl << 8);
}

static void sca200v100_can_tx_failure_cleanup(struct net_device *ndev)
{
	int i;

	for (i = 0; i < SCA200V100_CAN_TX_FIFO_NUM; i++)
		can_free_echo_skb(ndev, i);
}

static u32 sca200v100_can_error(struct net_device *ndev, u8 isr1, u8 isr2)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	u8 koer;//, alc;
	u8 ctrl1 = 0, ctrl2 = 0, tx_errors = 0, rx_errors = 0;

	/* Propagate the error condition to the CAN stack */
	skb = alloc_can_err_skb(ndev, &cf);
	if (skb == NULL)
		return 0;

	if(isr1 & SCA200V100_CAN_RTIF_EIF) {
		/* error interrupt */
		netdev_err(ndev, "error interrupt(bus off)\n");

		sca200v100_can_tx_failure_cleanup(ndev);
		priv->can.state = CAN_STATE_BUS_OFF;
		priv->can.can_stats.bus_off++;
		can_bus_off(ndev);
		cf->can_id |= CAN_ERR_BUSOFF;

		ctrl1 |= SCA200V100_CAN_RTIF_EIF;    /* clear bit */
	}

	if(isr1 & SCA200V100_CAN_RTIF_ROIF) {
		/* data overrun interrupt */
		netdev_dbg(ndev, "data overrun interrupt\n");
		cf->can_id |= CAN_ERR_CRTL;
		cf->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;
		stats->rx_over_errors++;
		stats->rx_errors++;
		ctrl1 |= SCA200V100_CAN_RTIF_ROIF;    /* clear bit */
	}

	if(isr2 & SCA200V100_CAN_ERRINT_EWARN) {
		/* error warning interrupt */
		netdev_dbg(ndev, "error warning interrupt\n");
		priv->can.state = CAN_STATE_ERROR_WARNING;
		priv->can.can_stats.error_warning++;

		cf->can_id |= CAN_ERR_CRTL;
		cf->data[6] = readb(&priv->regs->u8_if.u8_TECNT);
		cf->data[7] = readb(&priv->regs->u8_if.u8_RECNT);
		ctrl2 |= SCA200V100_CAN_ERRINT_EWARN;
	}

	if(isr2 & SCA200V100_CAN_ERRINT_EPASS) {
		netdev_dbg(ndev, "Error passive active\n");
		priv->can.state = CAN_STATE_ERROR_PASSIVE;
		priv->can.can_stats.error_passive++;
		ctrl2 |= SCA200V100_CAN_ERRINT_EPIF;
	}

	if(isr2 & SCA200V100_CAN_ERRINT_EPIF) {
		/* error passive interrupt */
		netdev_dbg(ndev, "error passive interrupt\n");
		priv->can.state = CAN_STATE_ERROR_PASSIVE;
		priv->can.can_stats.error_passive++;
		ctrl2 |= SCA200V100_CAN_ERRINT_EPIF;
	}

	if(isr2 & SCA200V100_CAN_ERRINT_ALIF) {
		/* arbitration lost interrupt */
		netdev_dbg(ndev, "lost arbitration interrupt\n");
		ctrl2 |= SCA200V100_CAN_ERRINT_ALIF;
	}

	if(isr2 & SCA200V100_CAN_ERRINT_BEIF) {
		/* bus error interrupt */
		netdev_dbg(priv->ndev, "Bus error interrupt:\n");

		koer = readb(&priv->regs->u8_if.u8_EALCAP);
		cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;

		if (KOER_BIT_ERROR == SCA200V100_CAN_EALCAP_KOER(koer)) {
			netdev_dbg(priv->ndev, "Bit Error\n");
			tx_errors++;
			cf->data[2] |= CAN_ERR_PROT_BIT0;
		}

		if (KOER_FORM_ERROR == SCA200V100_CAN_EALCAP_KOER(koer)) {
			netdev_dbg(priv->ndev, "Form Error\n");
			rx_errors++;
			cf->data[2] |= CAN_ERR_PROT_FORM;
		}

		if (KOER_STUFF_ERROR == SCA200V100_CAN_EALCAP_KOER(koer)) {
			netdev_dbg(priv->ndev, "Stuff Error\n");
			rx_errors++;
			cf->data[2] |= CAN_ERR_PROT_STUFF;
		}

		if (KOER_ACK_ERROR == SCA200V100_CAN_EALCAP_KOER(koer)) {
			netdev_dbg(priv->ndev, "ACK Error\n");
			tx_errors++;
			cf->can_id |= CAN_ERR_ACK;
			cf->data[3] = CAN_ERR_PROT_LOC_ACK;
		}

		if (KOER_BIT_ERROR == SCA200V100_CAN_EALCAP_KOER(koer)) {
			netdev_dbg(priv->ndev, "CRC Error\n");
			tx_errors++;
			cf->data[2] |= CAN_ERR_PROT_BIT0;
		}

		priv->can.can_stats.bus_error++;
		ndev->stats.rx_errors += rx_errors;
		ndev->stats.tx_errors += tx_errors;

		ctrl2 |= SCA200V100_CAN_ERRINT_BEIF;
	}

	if(0 != ctrl1 || 0 != ctrl2) {
		stats->rx_packets++;
		stats->rx_bytes += cf->can_dlc;
	}

	netif_rx(skb);

	return ((ctrl1 << 16) | (ctrl2 << 8));
}

static irqreturn_t sca200v100_can_interrupt(int irq, void *dev_id)
{
	struct net_device *ndev = dev_id;
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	u32 isr, ret = 0;
	u8 isr0, isr1, isr2;
	u32 flags;//, ctrl = 0;
	u32 flag_enable_mask = 0;

	spin_lock_irqsave(&priv->rx_lock, flags);

	//disable interrupt
	isr = readl(&priv->regs->u32_if.u32_reg4);
	flag_enable_mask = isr & SCA200V100_CAN_REG4_ENABLE;
	writel(isr & (~(SCA200V100_CAN_REG4_ENABLE | SCA200V100_CAN_REG4_FLAG)), &priv->regs->u32_if.u32_reg4);

	isr = readl(&priv->regs->u32_if.u32_reg4);
	//netdev_info(ndev,"isr:0x%08x", isr);

	isr0 = (u8)(isr & 0xff);
	isr1 = (u8)((isr & 0xff00) >> 8);
	isr2 = (u8)((isr & 0xff0000) >> 16);

	if((!(isr0 & SCA200V100_CAN_RTIE_TSFF)) && \
	    (!(isr1 & SCA200V100_CAN_RTIF_ALL)) && \
	    (!(isr2 & SCA200V100_CAN_ERRINT_F_ALL))) {

		netdev_info(ndev, "IRQ_NONE: 0x%08x", isr);
		//enable interrupt
		writel(isr | flag_enable_mask, &priv->regs->u32_if.u32_reg4);
		spin_unlock_irqrestore(&priv->rx_lock, flags);
		return IRQ_NONE;
	}

	//TX
	if((isr0 & SCA200V100_CAN_RTIE_TSFF) || (isr1 & (SCA200V100_CAN_RTIF_TPIF | SCA200V100_CAN_RTIF_TSIF))) {
		ret |= sca200v100_can_tx_done(ndev, isr0, isr1);
	}

	//RX SCA200V100_CAN_RTIF_ROIF |
	if(isr1 & (SCA200V100_CAN_RTIF_RIF | SCA200V100_CAN_RTIF_RFIF | SCA200V100_CAN_RTIF_RAFIF)) {
		if (napi_schedule_prep(&priv->napi)) {

			//disable rx interrupt enable
			flag_enable_mask &= (~(SCA200V100_CAN_RTIE_RIE | SCA200V100_CAN_RTIE_RFIE | SCA200V100_CAN_RTIE_RAFIE));

			//recv data in polling
			__napi_schedule(&priv->napi);
		}
	}

	//Abort
	if(isr1 & SCA200V100_CAN_RTIF_AIF) {
		netdev_info(ndev, "abort");
	}

	//Error
	if((isr1 & (SCA200V100_CAN_RTIF_ROIF | SCA200V100_CAN_RTIF_EIF)) || (isr2 & SCA200V100_CAN_ERRINT_F_ALL)) {
		ret |= sca200v100_can_error(ndev, isr1, isr2);
	}

	//clear flag
	writel(isr, &priv->regs->u32_if.u32_reg4);

	//enable interrupt
	isr &= (~SCA200V100_CAN_REG4_FLAG);
	isr |= flag_enable_mask;
	writel(isr, &priv->regs->u32_if.u32_reg4);

	spin_unlock_irqrestore(&priv->rx_lock, flags);

	return IRQ_HANDLED;
}

static void sca200v100_can_set_bittiming(struct net_device *ndev)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	struct can_bittiming *bt = &priv->can.bittiming;
	u32 btr;

	btr = readl(&priv->regs->u32_if.u32_reg5);
	//clear S_Seg_1,S_Seg_2,S_SJW
	btr &= (~(0x3F | (0x1F << 8) | (0xF << 16)));
	// set S_Seg_1,S_Seg_2,S_SJW,S_PRESC, BT=((S_Seg_1+2)+(S_Seg_2+1))*TQ
	btr |= (SCA200V100_CAN_BTR_S_SEG_1(bt->phase_seg1 + bt->prop_seg - 1) |
	        SCA200V100_CAN_BTR_S_SEG_2(bt->phase_seg2 - 1) | SCA200V100_CAN_BTR_S_SJW(bt->sjw - 1));

	netdev_info(ndev, "setting BTR0=0x%02x BTR1=0x%02x SJW=0x%02x BRP=0x%02x\n",
	    bt->phase_seg1 + bt->prop_seg - 2,
	    bt->phase_seg2 - 1,
	    bt->sjw - 1,
	    bt->brp - 1);

	writel(btr, &priv->regs->u32_if.u32_reg5);

	btr = readl(&priv->regs->u32_if.u32_reg6);
	btr &= 0xffffff00;
	btr |= ((bt->brp - 1) & 0xff);
	writel(btr, &priv->regs->u32_if.u32_reg6);
}

//accept all
static void sca200v100_can_set_filter(struct net_device *ndev)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	u32 ctrl;
	u32 code, mask;

	//select code
	ctrl = readl(&priv->regs->u32_if.u32_reg8);
	ctrl &= 0xffffff00;
	ctrl |= (SCA200V100_CAN_ACF_SELMASK(0) | SCA200V100_CAN_ACF_ACFADR(0));
	writel(ctrl, &priv->regs->u32_if.u32_reg8);

	//set code all zero
	code = readl(&priv->regs->u32_if.u32_reg9);
	code &= ~SCA200V100_CAN_AMASK_ID28_0;
	writel(code, &priv->regs->u32_if.u32_reg9);

	//select mask
	ctrl = readl(&priv->regs->u32_if.u32_reg8);
	ctrl &= 0xffffff00;
	ctrl |= (SCA200V100_CAN_ACF_SELMASK(1) | SCA200V100_CAN_ACF_ACFADR(0));
	writel(ctrl, &priv->regs->u32_if.u32_reg8);

	//set mask all one(disable mask and accecpt both standard or extended frames)
	mask = readl(&priv->regs->u32_if.u32_reg9);
	mask |= SCA200V100_CAN_AMASK_ID28_0;
	mask &= ~((SCA200V100_CAN_ACF_3_AIDEE | SCA200V100_CAN_ACF_3_AIDE) << 24);
	writel(mask, &priv->regs->u32_if.u32_reg9);

	//enable filter 0
	ctrl = readl(&priv->regs->u32_if.u32_reg8);
	ctrl &= 0xff00ffff;
	ctrl |= (SCA200V100_CAN_ACF_ENABLE(0) << 16);
	writel(ctrl, &priv->regs->u32_if.u32_reg8);
}

static void sca200v100_can_start(struct net_device *ndev)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	u32 ctrl, flags;
	//int i;

	//reset can
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl |= SCA200V100_CFG_STAT_RESET;
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	//disable tt can
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl &= ~(SCA200V100_CAN_TCTRL_TTTBM << 16);
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	ctrl = readl(&priv->regs->u32_if.u32_reg10);
	ctrl &= ~((SCA200V100_CAN_TTCFG_WTIE | SCA200V100_CAN_TTCFG_TTIE | SCA200V100_CAN_TTCFG_TTEN ) << 24);
	writel(ctrl, &priv->regs->u32_if.u32_reg10);

	//set bittiming
	sca200v100_can_set_bittiming(ndev);

	//set Acceptance Filter Control Register, mask and code
	sca200v100_can_set_filter(ndev);

	//set buffer mode
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl &= ~(SCA200V100_CAN_TCTRL_TSMODE << 16); // Set FIFO Mode
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	//init
	memset(priv->tx_dlc, 0, SCA200V100_CAN_TX_FIFO_NUM * sizeof(priv->tx_dlc[0]));
	priv->tx_head = 0;
	priv->tx_tail = 0;

	//clear err code

	//clear interrupt
	ctrl = readl(&priv->regs->u32_if.u32_reg4);
	ctrl |= ((SCA200V100_CAN_RTIF_ALL << 8) | (SCA200V100_CAN_ERRINT_F_ALL << 16));
	writel(ctrl, &priv->regs->u32_if.u32_reg4);

	//enable interrupt
	ctrl = readl(&priv->regs->u32_if.u32_reg4);
	ctrl |= ((SCA200V100_CAN_RTIE_RIE | SCA200V100_CAN_RTIE_ROIE | SCA200V100_CAN_RTIE_RFIE | \
	            SCA200V100_CAN_RTIE_RAFIE | SCA200V100_CAN_RTIE_TPIE | SCA200V100_CAN_RTIE_TSIE | \
	            SCA200V100_CAN_RTIE_EIE) |  \
	        ((SCA200V100_CAN_ERRINT_EPIE | SCA200V100_CAN_ERRINT_ALIE | SCA200V100_CAN_ERRINT_BEIE) << 16));
	writel(ctrl, &priv->regs->u32_if.u32_reg4);

	//release reset
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl &= ~SCA200V100_CFG_STAT_RESET;
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	spin_lock_irqsave(&priv->tx_lock, flags);

	//set optional mode
	ctrl = readl(&priv->regs->u32_if.u32_reg3);

	//enable loop back mode
	if(priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK)
		ctrl |= SCA200V100_CFG_STAT_LBMI; // Set loop back mode internel

	//enable single shot mode
	if(priv->can.ctrlmode & CAN_CTRLMODE_ONE_SHOT)
		ctrl |= (SCA200V100_CFG_STAT_TPSS | SCA200V100_CFG_STAT_TSSS);

	//enable listen only mode
	if(priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY)
		ctrl |= (SCA200V100_CAN_TCMD_LOM << 8);

	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	spin_unlock_irqrestore(&priv->tx_lock, flags);

	priv->can.state = CAN_STATE_ERROR_ACTIVE;

	netdev_info(ndev, "sca200v100 can start(0x%08x:0x%02x)!\n",
	    (u32)(&priv->regs->u8_if.u8_CFG_STAT), readb(&priv->regs->u8_if.u8_CFG_STAT));
}

static int sca200v100_can_open(struct net_device *ndev)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	int ret;

	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		netdev_err(ndev,
		    "failed to enable peripheral clock, error %d\n",
		    ret);
		goto out;
	}
	ret = clk_prepare_enable(priv->can_clk);
	if (ret) {
		netdev_err(ndev, "failed to enable CAN clock, error %d\n",
		    ret);
		goto out_clock;
	}
	ret = open_candev(ndev);
	if (ret) {
		netdev_err(ndev, "open_candev() failed, error %d\n", ret);
		goto out_can_clock;
	}
	napi_enable(&priv->napi);
	ret = request_irq(ndev->irq, sca200v100_can_interrupt, 0, ndev->name, ndev);
	if (ret) {
		netdev_err(ndev, "request_irq(%d) failed, error %d\n",
		    ndev->irq, ret);
		goto out_close;
	}

	INIT_WORK(&priv->tx_queue_work, sca200v100_can_tx_queue_work);

	can_led_event(ndev, CAN_LED_EVENT_OPEN);
	sca200v100_can_start(ndev);
	netif_start_queue(ndev);
	return 0;
out_close:
	napi_disable(&priv->napi);
	close_candev(ndev);
out_can_clock:
	clk_disable_unprepare(priv->can_clk);
out_clock:
	clk_disable_unprepare(priv->clk);
out:
	return ret;
}

static void sca200v100_can_stop(struct net_device *ndev)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	u32 ctrl;

	netdev_info(ndev, "sca200v100 can stop!\n");

	//reset can
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl |= SCA200V100_CFG_STAT_RESET;
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	priv->can.state = CAN_STATE_STOPPED;
}

static int sca200v100_can_close(struct net_device *ndev)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);

	cancel_work_sync(&priv->tx_queue_work);
	netif_stop_queue(ndev);
	sca200v100_can_stop(ndev);
	free_irq(ndev->irq, ndev);
	napi_disable(&priv->napi);
	clk_disable_unprepare(priv->can_clk);
	clk_disable_unprepare(priv->clk);
	close_candev(ndev);
	can_led_event(ndev, CAN_LED_EVENT_STOP);
	return 0;
}

static void sca200v100_can_tx_queue_work(struct work_struct *work)
{
	struct sca200v100_can_priv *priv = container_of(work, struct sca200v100_can_priv, tx_queue_work);
	u32 flags, ctrl = 0;

	spin_lock_irqsave(&priv->tx_lock, flags);

	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	if(0 != (ctrl & (SCA200V100_CAN_TCTRL_TSSTAT_MASK << 16))) {
		if(0 == (ctrl  & (SCA200V100_CAN_TCMD_TSONE << 8))) {
			ctrl |= (SCA200V100_CAN_TCMD_TSONE << 8);
			writel(ctrl, &priv->regs->u32_if.u32_reg3);
			++priv->tx_queue_num;
			//netdev_info(priv->ndev, "TX buffer queue awake(%d %d %d)!",
			//    priv->tx_head, priv->tx_queue_num, priv->tx_tail);
		}
		schedule_work(&priv->tx_queue_work);
	}

	spin_unlock_irqrestore(&priv->tx_lock, flags);
}

static netdev_tx_t sca200v100_can_start_xmit(struct sk_buff *skb,
    struct net_device *ndev)
{
	struct sca200v100_can_priv *priv = netdev_priv(ndev);
	struct can_frame *cf = (struct can_frame *)skb->data;
	u32 id, i;
	u32 flags, ctrl = 0;
	u32 *data;
	u32 timeout = 0;

	if (can_dropped_invalid_skb(ndev, skb)) {
		netdev_err(ndev, "drop invalid skb!\n");
		return NETDEV_TX_OK;
	}

	// check STB has space
	ctrl = readb(&priv->regs->u8_if.u8_TCTRL);
	ctrl &= SCA200V100_CAN_TCTRL_TSSTAT_MASK;
	if (unlikely(SCA200V100_CAN_TCTRL_TSSTAT_STB_FULL == ctrl)) {
		netif_stop_queue(ndev);
		netdev_err(ndev, "BUG! TX buffer full when queue awake(0x%08x %d %d %d)!\n", readl(&priv->regs->u32_if.u32_reg3),
		    priv->tx_head, priv->tx_tail, priv->tx_full_num);

		//send all(do not use send all:SCA200V100_CAN_TCMD_TSALL)
		//SCA200V100_CAN_TCMD_TSONE only one interrupt when complete
		//there is problem for tx packet,total txnum and so on in netstatus
		while((readb(&priv->regs->u8_if.u8_TCTRL) & SCA200V100_CAN_TCTRL_TSSTAT_MASK) &&
		    timeout < SCA200V100_CAN_TX_FIFO_NUM * 2000) {
			spin_lock_irqsave(&priv->tx_lock, flags);

			ctrl = readl(&priv->regs->u32_if.u32_reg3);
			ctrl |= (SCA200V100_CAN_TCMD_TSONE << 8);
			writel(ctrl, &priv->regs->u32_if.u32_reg3);

			spin_unlock_irqrestore(&priv->tx_lock, flags);

			++timeout;
		}
		netdev_err(ndev, "BUG! TX buffer force send all(0x%08x %d)!\n", readl(&priv->regs->u32_if.u32_reg3), timeout);

		return NETDEV_TX_BUSY;
	}

	timeout = 0;
	while(((ctrl = readl(&priv->regs->u32_if.u32_reg3)) & (SCA200V100_CAN_TCMD_TSONE << 8)) &&
	    timeout < SCA200V100_CAN_TX_FIFO_NUM * 500) {
		++timeout;
	}

	if(timeout >= SCA200V100_CAN_TX_FIFO_NUM * 500) {
		return NETDEV_TX_BUSY;
	}

	spin_lock_irqsave(&priv->tx_lock, flags);

	// select STB
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl |= (SCA200V100_CAN_TCMD_TBSEL << 8);
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	id = (cf->can_id & CAN_EFF_MASK);

	ctrl = 0;
	if (cf->can_id & CAN_EFF_FLAG)      /* Extended frame format */
		ctrl |= SCA200V100_CAN_TXBUF_CTRL_IDE;
	else                                /* Standard frame format */
		ctrl &= ~SCA200V100_CAN_TXBUF_CTRL_IDE;

	if (cf->can_id & CAN_RTR_FLAG)
		ctrl |= SCA200V100_CAN_TXBUF_CTRL_RTR;
	else
		ctrl &= ~SCA200V100_CAN_TXBUF_CTRL_RTR;

	//not support CAN FD
	ctrl &= ~(SCA200V100_CAN_TXBUF_CTRL_EDL | SCA200V100_CAN_TXBUF_CTRL_BRS);

	//date length code
	ctrl |= ((cf->can_dlc <= 8 ? cf->can_dlc : 8) & 0x0f);
	priv->tx_dlc[priv->tx_head % SCA200V100_CAN_TX_FIFO_NUM] = (ctrl & 0x0f);

	//netdev_info(ndev,"send can id:0x%08x ctrl:0x%08x", id, ctrl);

	writel(id, &priv->regs->u8_if.txBuf.id);
	writel(ctrl, &priv->regs->u32_if.u32_txBuf[1]);

	//RTR no data
	if (!(cf->can_id & CAN_RTR_FLAG)) {
		data = (u32 *)cf->data;
		for(i = 0; i < (ctrl & 0x0f); i += 4) {
			writel(data[i / 4], &priv->regs->u32_if.u32_txBuf[2 + i / 4]);
		}
	}

	//fill buffer completed, jump to next slot
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl |= (SCA200V100_CAN_TCTRL_TSNEXT << 16);
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	//start send
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl |= (SCA200V100_CAN_TCMD_TSONE << 8);
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	spin_unlock_irqrestore(&priv->tx_lock, flags);

	can_put_echo_skb(skb, ndev, priv->tx_head % SCA200V100_CAN_TX_FIFO_NUM);
	priv->tx_head++;

	//check tx slot fifo
	ctrl = readb(&priv->regs->u8_if.u8_TCTRL);
	ctrl &= SCA200V100_CAN_TCTRL_TSSTAT_MASK;
	if (SCA200V100_CAN_TCTRL_TSSTAT_STB_FULL == ctrl) {
		netif_stop_queue(ndev);
		//netdev_info(ndev, "netif_stop_queue");
	}

	return NETDEV_TX_OK;
}

static const struct net_device_ops sca200v100_can_netdev_ops = {
	.ndo_open = sca200v100_can_open,
	.ndo_stop = sca200v100_can_close,
	.ndo_start_xmit = sca200v100_can_start_xmit,
	.ndo_change_mtu = can_change_mtu,
};

static void sca200v100_can_rx_pkt(struct sca200v100_can_priv *priv)
{
	struct net_device_stats *stats = &priv->ndev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	u32 data;
	u8 dlc = 0;
	u32 flags, ctrl = 0;

	skb = alloc_can_skb(priv->ndev, &cf);
	if (!skb) {
		stats->rx_dropped++;
		return;
	}

	data = readl(&priv->regs->u8_if.rxBuf.id);
	ctrl = readb(&priv->regs->u8_if.rxBuf.ctrl);

	if(ctrl & SCA200V100_CAN_TXBUF_CTRL_IDE)
		cf->can_id = (data & CAN_EFF_MASK) | CAN_EFF_FLAG;
	else
		cf->can_id = data & CAN_SFF_MASK;

	if(ctrl & SCA200V100_CAN_TXBUF_CTRL_RTR) {
		cf->can_id |= CAN_RTR_FLAG;
		cf->can_dlc = 0;
	} else {
		dlc = readb(&priv->regs->u8_if.rxBuf.ctrl);
		dlc &= 0x0f;
		cf->can_dlc = get_can_dlc(dlc);

		for(dlc = 0; dlc < cf->can_dlc; ++dlc) {
			cf->data[dlc] =    readb(&priv->regs->u8_if.rxBuf.data[dlc]);
		}
	}

	spin_lock_irqsave(&priv->tx_lock, flags);

	//release recv slot
	ctrl = readl(&priv->regs->u32_if.u32_reg3);
	ctrl |= (SCA200V100_CAN_RCTRL_RREL << 24);
	writel(ctrl, &priv->regs->u32_if.u32_reg3);

	spin_unlock_irqrestore(&priv->tx_lock, flags);

	can_led_event(priv->ndev, CAN_LED_EVENT_RX);

	stats->rx_bytes += cf->can_dlc;
	stats->rx_packets++;
	netif_receive_skb(skb);
}

static int sca200v100_can_rx_poll(struct napi_struct *napi, int quota)
{
	struct sca200v100_can_priv *priv = container_of(napi,  struct sca200v100_can_priv, napi);
	int num_pkts;
	u32 flags, ctrl = 0;

	for (num_pkts = 0; num_pkts < quota; num_pkts++) {
		ctrl = readb(&priv->regs->u8_if.u8_RCTRL);
		if(SCA200V100_CAN_RCTRL_RSTAT_STB_EMPTY != (ctrl & SCA200V100_CAN_RCTRL_RSTAT_MASK)) {
			sca200v100_can_rx_pkt(priv);
		} else {
			break;
		}
	}

	/* All packets processed */
	if (num_pkts < quota) {

		napi_complete(napi);

		spin_lock_irqsave(&priv->rx_lock, flags);

		//enable rx interrupt
		ctrl = readl(&priv->regs->u32_if.u32_reg4);
		ctrl &= ~SCA200V100_CAN_REG4_FLAG;//0xffea00ff;
		ctrl |= (SCA200V100_CAN_RTIE_RIE | SCA200V100_CAN_RTIE_RFIE | SCA200V100_CAN_RTIE_RAFIE);
		writel(ctrl, &priv->regs->u32_if.u32_reg4);

		spin_unlock_irqrestore(&priv->rx_lock, flags);
	}

	return num_pkts;
}

static int sca200v100_can_do_set_mode(struct net_device *ndev, enum can_mode mode)
{
	switch (mode) {
	case CAN_MODE_START:
		sca200v100_can_start(ndev);
		netif_wake_queue(ndev);
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static int sca200v100_can_get_berr_counter(const struct net_device *dev,
    struct can_berr_counter *bec)
{
	struct sca200v100_can_priv *priv = netdev_priv(dev);
	int err;

	err = clk_prepare_enable(priv->clk);
	if (err)
		return err;

	bec->txerr = readb(&priv->regs->u8_if.u8_TECNT);
	bec->rxerr = readb(&priv->regs->u8_if.u8_RECNT);

	clk_disable_unprepare(priv->clk);
	return 0;
}

static int sca200v100_can_probe(struct platform_device *pdev)
{
	struct sca200v100_can_priv  *priv;
	struct resource         *mem;
	void __iomem            *addr;
	struct net_device       *ndev;
	int                     irq;
	int                     ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "No IRQ resource\n");
		ret = irq;
		goto fail;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	addr = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(addr)) {
		ret = PTR_ERR(addr);
		goto fail;
	}

	ndev = alloc_candev(sizeof(struct sca200v100_can_priv), SCA200V100_CAN_TX_FIFO_NUM);
	if (!ndev) {
		dev_err(&pdev->dev, "alloc_candev() failed\n");
		ret = -ENOMEM;
		goto fail;
	}

	priv = netdev_priv(ndev);

	priv->clk = devm_clk_get(&pdev->dev, "host_clk");
	if (IS_ERR(priv->clk)) {
		ret = PTR_ERR(priv->clk);
		dev_err(&pdev->dev, "cannot get HOST clock, error %d\n", ret);
		goto fail_clk;
	}

	priv->can_clk = devm_clk_get(&pdev->dev, "can_clk");
	if (IS_ERR(priv->can_clk)) {
		ret = PTR_ERR(priv->can_clk);
		dev_err(&pdev->dev, "cannot get CAN clock, error %d\n", ret);
		goto fail_clk;
	}

	ndev->netdev_ops = &sca200v100_can_netdev_ops;
	ndev->irq = irq;
	ndev->flags |= IFF_ECHO;
	priv->ndev = ndev;
	priv->regs = addr;
	priv->can.clock.freq = clk_get_rate(priv->can_clk);
	priv->can.bittiming_const = &sca200v100_can_bittiming_const;
	//priv->can.da_set_bittiming = sca200v100_can_set_bittiming;
	priv->can.do_set_mode = sca200v100_can_do_set_mode;
	priv->can.do_get_berr_counter = sca200v100_can_get_berr_counter;
	priv->can.ctrlmode_supported =  CAN_CTRLMODE_LOOPBACK | \
	    CAN_CTRLMODE_LISTENONLY | \
	    CAN_CTRLMODE_ONE_SHOT | \
	    CAN_CTRLMODE_BERR_REPORTING;
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->rx_lock);

	platform_set_drvdata(pdev, ndev);
	SET_NETDEV_DEV(ndev, &pdev->dev);

	netif_napi_add(ndev, &priv->napi, sca200v100_can_rx_poll,
	    SCA200V100_CAN_RX_FIFO_NUM);
	ret = register_candev(ndev);
	if (ret) {
		dev_err(&pdev->dev, "register_candev() failed, error %d\n",
		    ret);
		goto fail_candev;
	}

	devm_can_led_init(ndev);

	dev_info(&pdev->dev, "device registered (regs @ %p, IRQ%d)\n",
	    priv->regs, ndev->irq);

	return 0;
fail_candev:
	netif_napi_del(&priv->napi);
fail_clk:
	free_candev(ndev);
fail:
	return ret;
}

static int sca200v100_can_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct sca200v100_can_priv *priv = netdev_priv(ndev);

	unregister_candev(ndev);
	netif_napi_del(&priv->napi);
	free_candev(ndev);
	return 0;
}

static const struct of_device_id sca200v100_can_of_table[]  = {
	{ .compatible = "smartchip,sca200v100-can" },
	{ },
};
MODULE_DEVICE_TABLE(of, sca200v100_can_of_table);

static struct platform_driver sca200v100_can_driver = {
	.driver = {
		.name = SCA200V100_CAN_DEV_NAME,
		.of_match_table = of_match_ptr(sca200v100_can_of_table),
		//.pm = &sca200v100_can_pm_ops,
	},
	.probe = sca200v100_can_probe,
	.remove = sca200v100_can_remove,
};
module_platform_driver(sca200v100_can_driver);

MODULE_AUTHOR("kaiwang <kai.wang@smartchip.cn>");
MODULE_DESCRIPTION("smartchip sca200v100 SoC can driver");
MODULE_LICENSE("GPL v2");
