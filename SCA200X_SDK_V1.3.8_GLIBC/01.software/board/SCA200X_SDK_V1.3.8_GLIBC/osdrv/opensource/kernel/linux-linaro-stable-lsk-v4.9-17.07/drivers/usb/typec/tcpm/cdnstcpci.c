/* Copyright 2015-2016 Caden|ce Design Systems Inc. All rights reserved worldwide.
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Type-C port manager */

//#include "phy-generic.h"
//#include <linux/smartchip-pinshare.h>

//#include <linux/extcon-provider.h>

//#include "cdns-dp-reg.h"
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/usb/typec.h>
#include <linux/usb/tcpm.h>
#include <linux/usb/typec_dp.h>

/*#include "task.h"
#include "timer.h"
#include "usb_charge.h"
#include "usb_pd.h"
#include "usb_pd_tcpc.h"
#include "usb_pd_tcpm.h"
#include "util.h"*/

#include "cdnstcpc_map.h"

#define TCPC_HEADER_LEN                 2
#define TCPC_BYTE_CNT_LEN               1

#define CDNS_TYPEC_CC_OPEN              0x03
#define CDNS_TYPEC_CC_RA                0x00
#define CDNS_TYPEC_CC_RD                0x02
#define CDNS_TYPEC_CC_RP                0x01

#define CDNS_TYPEC_CC_RP_DEF            0x00
#define CDNS_TYPEC_CC_RP_1_5            0x01
#define CDNS_TYPEC_CC_RP_3_0            0x02

#define CDNS_TYPEC_CC_ENABLE_DRP        0x01
#define CDNS_TYPEC_CC_DRP_FOUND         0x00
#define CDNS_TYPEC_CC_DRP_FINDING       0x01
#define CDNS_TYPEC_CC_PRESENT_RP        0x00
#define CDNS_TYPEC_CC_PRESENT_RD        0x01

#define CDNS_CMD_WAKEINTERFACE          0x11
#define CDNS_CMD_DISABLEVBUSDETECT      0x22
#define CDNS_CMD_ENABLEVBUSDETECT       0x33
#define CDNS_CMD_DISABLESINKVBUS        0x44
#define CDNS_CMD_SINKVBUS               0x55
#define CDNS_CMD_DISABLESOURCEVBUS      0x66
#define CDNS_CMD_SOURCEVBUSDEFAULTVOLT  0x77
#define CDNS_CMD_SOURCEVBUSHIGHVOLT     0x88
#define CDNS_CMD_LOOK4CONNECTION        0x99
#define CDNS_CMD_RXONEMORE              0xAA
#define CDNS_CMD_ENABLE1STVBUS          0xF0
#define CDNS_CMD_ENABLE2NDVBUS          0xF1
#define CDNS_CMD_ENABLE3RDVBUS          0xF2
#define CDNS_CMD_ENABLE4THVBUS          0xF3

#define TCPC_REG_POWER_STATUS_MASK_ALL  0xff

#define TCPC_FW_NAME                   "tcpc.bin"

#define PD_DP_PIN_CAPS(x)               ((((x) >> 6) & 0x1) ? (((x) >> 16) & 0x3f) \
                                        : (((x) >> 8) & 0x3f))
#define PD_DP_SIGNAL_GEN2(x)            (((x) >> 3) & 0x1)

#define MODE_DP_PIN_A                    BIT(0)
#define MODE_DP_PIN_B                    BIT(1)
#define MODE_DP_PIN_C                    BIT(2)
#define MODE_DP_PIN_D                    BIT(3)
#define MODE_DP_PIN_E                    BIT(4)
#define MODE_DP_PIN_F                    BIT(5)

/* Pin configs B/D/F support multi-function */
#define MODE_DP_PIN_MF_MASK              (MODE_DP_PIN_B | MODE_DP_PIN_D | MODE_DP_PIN_F)
/* Pin configs A/B support BR2 signaling levels */
#define MODE_DP_PIN_BR2_MASK             (MODE_DP_PIN_A | MODE_DP_PIN_B)
/* Pin configs C/D/E/F support DP signaling levels */
#define MODE_DP_PIN_DP_MASK              (MODE_DP_PIN_C | MODE_DP_PIN_D | \
                                          MODE_DP_PIN_E | MODE_DP_PIN_F)

#define MODE_DP_PIN_DP_C_E_MASK          (MODE_DP_PIN_C | MODE_DP_PIN_E)

#define cdstcpc_reg(name)                offsetof(cdstcpc_regs, name)

#define tcpc_get_apb_busy(port) \
        SFR_REG_0_REG__ALERT_APB_BUSY__READ(tcpc_uncached_read32((volatile uint32_t *)(tcpc_hw_addr[port] + cdstcpc_reg(SFR_REG_0))))

#define PDO_FIXED_FLAGS \
        (PDO_FIXED_DUAL_ROLE | PDO_FIXED_DATA_SWAP | PDO_FIXED_USB_COMM)

/**
 *  Example Base Register addresses
 */
static const void *tcpc_hw_addr[CONFIG_USB_PD_PORT_COUNT] = {""};

typedef struct cdnstcpc_regprof cdstcpc_regs;

static int tcpc_vbus[CONFIG_USB_PD_PORT_COUNT];

extern int cdns3_bind(void);
extern int cdns3_unbind(void);
extern int cdns3_set_enable_status(bool on);

//unsigned int irqcount = 0;

static const unsigned int smartx_tcpm_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_USB_VBUS_EN,
	EXTCON_CHG_USB_SDP,
	EXTCON_CHG_USB_CDP,
	EXTCON_CHG_USB_DCP,
	EXTCON_CHG_USB_SLOW,
	EXTCON_CHG_USB_FAST,
	EXTCON_DISP_DP,
	EXTCON_NONE,
};

static const u32 src_pdo[] = {
	PDO_FIXED(5000, 500, PDO_FIXED_FLAGS),
};

static const u32 snk_pdo[] = {
	PDO_FIXED(5000, 500, PDO_FIXED_FLAGS),
};

/*
 * DisplayPort modes capabilities
 * -------------------------------
 * <31:24> : Reserved (always 0).
 * <23:16> : UFP_D pin assignment supported
 * <15:8>  : DFP_D pin assignment supported
 * <7>     : USB 2.0 signaling (0b=yes, 1b=no)
 * <6>     : Plug | Receptacle (0b == plug, 1b == receptacle)
 * <5:2>   : xxx1: Supports DPv1.3, xx1x Supports USB Gen 2 signaling
 *         Other bits are reserved.
 * <1:0>   : signal direction ( 00b=rsv, 01b=sink, 10b=src 11b=both )
 */
static const struct typec_altmode_desc smartx_tcpc_altmode[] = {
	{
		.svid = USB_TYPEC_DP_SID,
		.mode = USB_TYPEC_DP_MODE,
		.vdo = ((MODE_DP_PIN_DP_C_E_MASK & 0x3f) << 16) |
		((MODE_DP_PIN_DP_C_E_MASK & 0x3f) << 8) |
		(0x0 << 7) |
		(0x1 << 6) |
		(0x2 << 2) |
		(0x2 << 0 ),
		.roles = TYPEC_PORT_DFP,
	},
	{},
};

static const struct tcpc_config smartx_tcpc_otg_config = {
	.src_pdo = src_pdo,
	.nr_src_pdo = ARRAY_SIZE(src_pdo),
	.snk_pdo = snk_pdo,
	.nr_snk_pdo = ARRAY_SIZE(snk_pdo),

	.operating_snk_mw = 2500,
	.type = TYPEC_PORT_DRP,//TYPEC_PORT_SRC,
	.data = TYPEC_PORT_DRD,//TYPEC_PORT_DFP,
	.default_role = TYPEC_SOURCE,
	.alt_modes = smartx_tcpc_altmode,
	//.alt_modes = NULL,
};

static const struct tcpc_config smartx_tcpc_host_config = {
	.src_pdo = src_pdo,
	.nr_src_pdo = ARRAY_SIZE(src_pdo),
	.snk_pdo = snk_pdo,
	.nr_snk_pdo = ARRAY_SIZE(snk_pdo),

	.operating_snk_mw = 2500,
	.type = TYPEC_PORT_SRC,
	.data = TYPEC_PORT_DFP,//TYPEC_PORT_DRD,//TYPEC_PORT_DFP,
	.default_role = TYPEC_SOURCE,
	.alt_modes = smartx_tcpc_altmode,
	//.alt_modes = NULL,
};

static const struct tcpc_config smartx_tcpc_device_config = {
	.src_pdo = src_pdo,
	.nr_src_pdo = ARRAY_SIZE(src_pdo),
	.snk_pdo = snk_pdo,
	.nr_snk_pdo = ARRAY_SIZE(snk_pdo),

	.operating_snk_mw = 2500,
	.type = TYPEC_PORT_SNK,
	.data = TYPEC_PORT_UFP,
	.default_role = TYPEC_SINK,
	//.alt_modes = smartx_tcpc_altmode,
	.alt_modes = NULL,
};

/**
 * Example hardware read memory access
 * @param[in] address memory address
 * @return memory value
 */
static inline uint32_t tcpc_uncached_read32(volatile uint32_t *address)
{
	return *address;
}

/**
 * Example hardware write memory access
 * @param[in] address memory address
 * @param[in] value memory value
 */
static inline void tcpc_uncached_write32(volatile uint32_t *address, uint32_t value)
{
	*address = value;
}

static inline int tcpc_poll(int port)
{
	while (tcpc_get_apb_busy(port)) {}
	return 0;
}

static inline int tcpc_write32(int port, int reg, int val)
{
	tcpc_poll(port);
	tcpc_uncached_write32((volatile uint32_t *)(tcpc_hw_addr[port] + reg), val);
	//printk(KERN_ERR "write tcpc reg 0x%08x: 0x%08x", tcpc_hw_addr[port] + reg, val);
	return 0;
}

static inline int tcpc_read32(int port, int reg, int *val)
{
	tcpc_poll(port);
	*val = tcpc_uncached_read32((volatile uint32_t *)(tcpc_hw_addr[port] + reg));
	//printk(KERN_ERR "read tcpc reg 0x%08x: 0x%08x", tcpc_hw_addr[port] + reg, *val);
	return 0;
}

static int smartx_tcpm_notify(struct smartx_tcpc *tcpc, struct smartx_tcpc_notify new)
{
	union extcon_property_value property;
	unsigned int                val;
	int                         ret;
	bool                        ufp = false, dfp = false, dp = false;

	if(memcmp(&tcpc->notify, &new, sizeof(struct smartx_tcpc_notify))) {
		int                         port = tcpc->port_index;
		printk(KERN_INFO "===============================>new %d %d %d %d\n", new.flip, new.data_role, new.dp, new.usb);

		//set type c flip status
		property.intval = new.flip;
		ret = extcon_set_property(tcpc->extcon, EXTCON_USB,
		        EXTCON_PROP_USB_TYPEC_POLARITY, property);
		if(ret) {
			printk(KERN_ERR "1 extcon_set_property failed\n");
		}

		ret = extcon_set_property(tcpc->extcon, EXTCON_USB_HOST,
		        EXTCON_PROP_USB_TYPEC_POLARITY, property);
		if(ret) {
			printk(KERN_ERR "2 extcon_set_property failed\n");
		}

		ret = extcon_set_property(tcpc->extcon, EXTCON_DISP_DP,
		        EXTCON_PROP_USB_TYPEC_POLARITY, property);
		if(ret) {
			printk(KERN_ERR "3 extcon_set_property failed\n");
		}

		if(1 == new.dp) {
			dp = true;
			ufp = false;
			dfp = false;
		} else if(1 == new.usb) {
			dp = false;
			//set type c data_role
			if(TYPEC_DEVICE == new.data_role) {
				ufp = true;
				dfp = false;
			} else {
				ufp = false;
				dfp = true;
			}
		} else {
			dp = false;
			ufp = false;
			dfp = false;
		}

		tcpc_read32(port, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), &val);
		if(true == dp) {
			val &= ~0x0e;
			val |= (0x08 | 0x02);
			if(USB_DR_MODE_DP_ONLY == tcpc->dr_mode) {
				property.intval = 1;
				ret = extcon_set_property(tcpc->extcon, EXTCON_DISP_DP,
				        EXTCON_PROP_USB_SS, property);
			}
		} else if(USB_DR_MODE_DP_ONLY != tcpc->dr_mode &&
		    USB_DR_MODE_DP != tcpc->dr_mode) {
			val &= ~0x0e;
			val |= (0x04 | 0x02);
			//val |= (0x0c | 0x02);
		}
		tcpc_write32(port, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), val);

		ret = extcon_set_state(tcpc->extcon, EXTCON_USB, ufp);
		ret = extcon_set_state(tcpc->extcon, EXTCON_USB_HOST, dfp);
		ret = extcon_set_state(tcpc->extcon, EXTCON_DISP_DP, dp);

		//set type c power role
		//property.intval = tcpc->notify->power_role;

		extcon_sync(tcpc->extcon, EXTCON_USB);
		extcon_sync(tcpc->extcon, EXTCON_USB_HOST);
		ret = extcon_sync(tcpc->extcon, EXTCON_DISP_DP);
		if(ret)
			printk(KERN_ERR "extcon_sync failed\n");

		tcpc->notify = new;

	}
	return 0;
}

//flag 0: cpu access 1: fw access
int smartx_typec_common_phy_access(struct smartx_tcpc *tcpc, unsigned char flag)
{
	if(NULL == tcpc) {
		printk(KERN_ERR "smartx_typec_common_phy_access failed!");
		return -1;
	}

	writel(0x0000153d, tcpc->user_define_base + 0x0008);
	//printk("=======================>user_define_base + 0x08: 0x%08x", readl(tcpc->user_define_base + 0x0008));

	writel(flag, tcpc->user_define_base);

	return 0;
}

int smartx_typec_set_config_mode(struct smartx_tcpc *tcpc)
{
	writel(tcpc->mode_cfg_reg, tcpc->user_define_base + 0x04);
	//printk("=======================>set_config_mode: 0x%08x", readl(tcpc->user_define_base + 0x04));

	writel(0x0000153d, tcpc->user_define_base + 0x0008);
	//printk("=======================>user_define_base + 0x08: 0x%08x", readl(tcpc->user_define_base + 0x0008));
	return 0;
}

int tcpm_init(struct tcpc_dev *dev);

int smartx_tcpd_phy_reset(struct smartx_tcpc *tcpc)
{
	unsigned int                    reg, timeout = 100;
	int                             ret;

	//reset tcpd phy
	tcpc_read32(tcpc->port_index, cdstcpc_reg(SFR_REG_0), &reg);
	SFR_REG_0_REG__ALERT_CMN_RESET_N__CLR(reg);
	tcpc_write32(tcpc->port_index, cdstcpc_reg(SFR_REG_0), reg);

	ret = -1;
	while (timeout) {
		tcpc_read32(tcpc->port_index, cdstcpc_reg(SFR_REG_0), &reg);
		if (SFR_REG_0_REG__ALERT_CMN_READY__READ(reg) == 0x0) {
			ret = 0;
			break;
		}

		msleep(1);
		--timeout;
	}

	printk(KERN_INFO "===================>reset tcpd phy 0x%08x", reg);
	//msleep(10);

	tcpc_read32(tcpc->port_index, cdstcpc_reg(SFR_REG_0), &reg);
	SFR_REG_0_REG__ALERT_CMN_RESET_N__SET(reg);
	tcpc_write32(tcpc->port_index, cdstcpc_reg(SFR_REG_0), reg);

	ret = -1;
	timeout = 100;
	while (timeout) {
		tcpc_read32(tcpc->port_index, cdstcpc_reg(SFR_REG_0), &reg);
		if (SFR_REG_0_REG__ALERT_CMN_READY__READ(reg) == 0x1) {
			ret = 0;
			break;
		}
		msleep(1);
		--timeout;
	}

	//msleep(10);
	printk(KERN_INFO "===================>reset tcpd phy 0x%08x", reg);

	//clean DEBUG_ACCESSORY CONNECTED
	tcpc_read32(tcpc->port_index, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), &reg);
	CONFIG_STANDARD_OUTPUT_REG__DEBUG_ACCESSORY_CONNECTED__CLR(reg);
	tcpc_write32(tcpc->port_index, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), reg);

	return 0;
}

static int CC1_vconn_enable(struct smartx_tcpc *tcpc, bool enable)
{
	if(NULL == tcpc || tcpc->cc1_gpio < 0)
		return -1;

	gpio_direction_output(tcpc->cc1_gpio, !enable);
	return 0;
}

static int CC2_vconn_enable(struct smartx_tcpc *tcpc, bool enable)
{
	if(NULL == tcpc || tcpc->cc2_gpio < 0)
		return -1;

	gpio_direction_output(tcpc->cc2_gpio, !enable);
	return 0;
}

static int tcpm_set_power_status_mask(int port, uint8_t mask)
{
	/* write to the Alert Mask register */
	return tcpc_write32(port, cdstcpc_reg(POWER_STATUS_MASK), mask);
}

static int tcpm_alert_mask_set(int port, uint16_t mask)
{
	/* write to the Alert Mask register */
	return tcpc_write32(port, cdstcpc_reg(ALERT_MASK), mask);
}

static int init_alert_mask(int port)
{
	uint16_t mask;
	int rv;

	/*
	 * Create mask of alert events that will cause the TCPC to
	 * signal the TCPM via the Alert# gpio line.
	 */
	mask = 0;
	ALERT_MASK_REG__TRANSMIT_SOP_MESSAGE_SUCCESSFUL_INTERRUPT_MASK__SET(mask);
	ALERT_MASK_REG__TRANSMIT_SOP_MESSAGE_FAILED_INTERRUPT_MASK__SET(mask);
	ALERT_MASK_REG__TRANSMIT_SOP_MESSAGE_DISCARDED_INTERRUPT_MASK__SET(mask);
	ALERT_MASK_REG__RECEIVE_SOP_MESSAGE_STATUS_INTERRUPT_MASK__SET(mask);
	ALERT_MASK_REG__RECEIVED_HARD_RESET_MESSAGE_STATUS_INTERRUPT_MASK__SET(mask);
	ALERT_MASK_REG__CC_STATUS_INTERRUPT_MASK__SET(mask);
	ALERT_MASK_REG__PORT_POWER_STATUS_INTERRUPT_MASK__SET(mask);

	/* Set the alert mask in TCPC */
	rv = tcpm_alert_mask_set(port, mask);

	return rv;
}

static int init_power_status_mask(int port)
{
	uint8_t mask;
	int rv;

	mask = 0;
	POWER_STATUS_REG__VBUS_PRESENT__SET(mask);
	rv = tcpm_set_power_status_mask(port, mask);

	return rv;
}

enum typec_cc_status cc_status_convert(unsigned int cc, bool sink)
{
	printk(KERN_INFO "raw_cc_value:%d raw_cc_set:%d\n", cc, sink);
	switch (cc) {
	case 0x1:
		return sink ? TYPEC_CC_RP_DEF : TYPEC_CC_RA;
	case 0x2:
		return sink ? TYPEC_CC_RP_1_5 : TYPEC_CC_RD;
	case 0x3:
		if (sink)
			return TYPEC_CC_RP_3_0;
	/* fall through */
	case 0x0:
	default:
		return TYPEC_CC_OPEN;
	}
}

static int tcpm_start_drp_toggling(struct tcpc_dev *dev, enum typec_cc_status cc);

int tcpm_get_cc(struct tcpc_dev *dev, enum typec_cc_status *cc1,
    enum typec_cc_status *cc2)
{
	struct smartx_tcpc *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	int status, cc_set;
	int rv;
	int port = tcpc->port_index;
	unsigned int sink;

	rv = tcpc_read32(port, cdstcpc_reg(CC_STATUS), &status);

	/* If tcpc read fails, return error */
	if (rv)
		return rv;

	rv = tcpc_read32(port, cdstcpc_reg(ROLE_CONTROL), &cc_set);

	/* If tcpc read fails, return error */
	if (rv)
		return rv;

	*cc1 = cc_status_convert(CC_STATUS_REG__CC1_STATE__READ(status),
	        CC_STATUS_REG__CONNECTRESULT__READ(status));
	*cc2 = cc_status_convert(CC_STATUS_REG__CC2_STATE__READ(status),
	        CC_STATUS_REG__CONNECTRESULT__READ(status));

	sink = CC_STATUS_REG__CONNECTRESULT__READ(status);

	if(1 == ROLE_CONTROL_REG__DRP__READ(cc_set) && TYPEC_CC_RD == *cc1 && TYPEC_CC_RD == *cc2) {
		tcpm_start_drp_toggling(&tcpc->tcpc, TYPEC_CC_RP_DEF);
	} else if(TYPEC_CC_RD == *cc1 && TYPEC_CC_RD == *cc2) {
		smartx_tcpd_phy_reset(tcpc);
	}

	return rv;
}

int tcpm_get_power_status(int port, int *status)
{
	return tcpc_read32(port, cdstcpc_reg(POWER_STATUS), status);
}

int tcpm_set_cc(struct tcpc_dev *dev, enum typec_cc_status cc)
{
	struct smartx_tcpc *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	int port = tcpc->port_index;
	/*
	 * Set manual control of Rp/Rd, and set both CC lines to the same
	 * pull.
	 */
	/* TODO: set desired Rp strength */
	uint32_t reg = 0;

	tcpc_read32(port, cdstcpc_reg(ROLE_CONTROL), &reg);

	switch (cc) {
	case TYPEC_CC_RD:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, CDNS_TYPEC_CC_RD);
		ROLE_CONTROL_REG__CC2__MODIFY(reg, CDNS_TYPEC_CC_RD);
		break;
	case TYPEC_CC_RP_DEF:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, CDNS_TYPEC_CC_RP);//pull up
		ROLE_CONTROL_REG__CC2__MODIFY(reg, CDNS_TYPEC_CC_RP);//pull up
		ROLE_CONTROL_REG__RP_VALUE__MODIFY(reg, CDNS_TYPEC_CC_RP_DEF);
		break;
	case TYPEC_CC_RP_1_5:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, 0x01);//pull up
		ROLE_CONTROL_REG__CC2__MODIFY(reg, 0x01);//pull up
		ROLE_CONTROL_REG__RP_VALUE__MODIFY(reg, CDNS_TYPEC_CC_RP_1_5);
		break;
	case TYPEC_CC_RP_3_0:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, 0x01);//pull up
		ROLE_CONTROL_REG__CC2__MODIFY(reg, 0x01);//pull up
		ROLE_CONTROL_REG__RP_VALUE__MODIFY(reg, CDNS_TYPEC_CC_RP_3_0);
		break;
	case TYPEC_CC_OPEN:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, CDNS_TYPEC_CC_OPEN);
		ROLE_CONTROL_REG__CC2__MODIFY(reg, CDNS_TYPEC_CC_OPEN);
		break;
	default:
		return -EINVAL;
	}

	printk(KERN_INFO "set CC:%d reg:0x%08x\n", cc, reg);

	return tcpc_write32(port, cdstcpc_reg(ROLE_CONTROL), reg);
}

static int tcpm_set_polarity(struct tcpc_dev *dev, enum typec_cc_polarity polarity)
{
	struct smartx_tcpc          *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	int                         port = tcpc->port_index;
	//union extcon_property_value property;
	unsigned int                val;
	int                         ret;
	struct smartx_tcpc_notify   new = tcpc->notify;

	new.flip = polarity;

	//assert dp_hpd
	if(1 == tcpc->mode) {
		tcpc_read32(port, cdstcpc_reg(SFR_REG_0), &val);
		while(0 == (val & 0x00000200)) {
			tcpc_write32(port, cdstcpc_reg(SFR_REG_0), (val | 0x00000200));
			tcpc_read32(port, cdstcpc_reg(SFR_REG_0), &val);
		}

		ret = extcon_set_state(tcpc->extcon, EXTCON_DISP_DP, 1);
		if(ret)
			printk(KERN_ERR "extcon_set_state failed\n");
	} else {
		//tcpc_read32(port, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), &val);
		//tcpc_write32(port, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), (val | 0x00000006));
	}

	tcpc_read32(port, cdstcpc_reg(ROLE_CONTROL), &val);
	if(TYPEC_POLARITY_CC2 == polarity)
		ROLE_CONTROL_REG__CC1__MODIFY(val, CDNS_TYPEC_CC_OPEN);
	else
		ROLE_CONTROL_REG__CC2__MODIFY(val, CDNS_TYPEC_CC_OPEN);
	tcpc_write32(port, cdstcpc_reg(ROLE_CONTROL), val);

	tcpc_read32(port, cdstcpc_reg(TCPC_CONTROL), &val);
	TCPC_CONTROL_REG__PLUG_ORIENTATION__MODIFY(val, polarity);
	tcpc_write32(port, cdstcpc_reg(TCPC_CONTROL), val);

	smartx_tcpm_notify(tcpc, new);

	//if(irqcount > 0 && tcpc->flip != last_flip)
	{
		//cdns3_set_enable_status(0);

		//cdns3_set_enable_status(1); //enable cdns3 usb
	}
	return 0;
}

static int tcpm_set_vbus(struct tcpc_dev *dev, bool on, bool sink)
{
	struct smartx_tcpc *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	int                 port = tcpc->port_index;
	int                 ret;

	if (!on) {
		tcpc_write32(port, cdstcpc_reg(COMMAND),
		    COMMAND_REG__COMMAND__WRITE(CDNS_CMD_DISABLESOURCEVBUS));

		if(tcpc->typec_gpio >= 0) {
			ret = gpio_direction_output(tcpc->typec_gpio, on);
			if(ret < 0) {
				printk(KERN_ERR "gpio output fail!(errno: %d)\n", ret);
			}
		}
	}

	if (!sink) {
		tcpc_write32(port, cdstcpc_reg(COMMAND),
		    COMMAND_REG__COMMAND__WRITE(CDNS_CMD_DISABLESINKVBUS));

		if(tcpc->typec_gpio >= 0) {
			ret = gpio_direction_output(tcpc->typec_gpio, 0);
			if(ret < 0) {
				printk(KERN_ERR "gpio output fail!(errno: %d)\n", ret);
			}
		}
	}

	if (on) {
		tcpc_write32(port, cdstcpc_reg(COMMAND),
		    COMMAND_REG__COMMAND__WRITE(CDNS_CMD_SOURCEVBUSDEFAULTVOLT));

		if(tcpc->typec_gpio >= 0) {
			ret = gpio_direction_output(tcpc->typec_gpio, on);
			if(ret < 0) {
				printk(KERN_ERR "gpio output fail!(errno: %d)\n", ret);
			}
		}
	}

	if (sink) {
		tcpc_write32(port, cdstcpc_reg(COMMAND),
		    COMMAND_REG__COMMAND__WRITE(CDNS_CMD_SINKVBUS));

		if(tcpc->typec_gpio >= 0) {
			ret = gpio_direction_output(tcpc->typec_gpio, 0);
			if(ret < 0) {
				printk(KERN_ERR "gpio output fail!(errno: %d)\n", ret);
			}
		}
	}

	printk(KERN_CRIT "vbus sink: %d on: %d\n", sink, on);

	return 0;
}

static int tcpm_set_vconn(struct tcpc_dev *dev, bool on)
{
	struct smartx_tcpc *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	int port = tcpc->port_index;

	if(1 == on) {
		CC1_vconn_enable(tcpc, tcpc->notify.flip & 0x01);
		CC2_vconn_enable(tcpc, !(tcpc->notify.flip & 0x01));
	} else {
		CC1_vconn_enable(tcpc, 0);
		CC2_vconn_enable(tcpc, 0);
	}

	return tcpc_write32(port, cdstcpc_reg(POWER_CONTROL),
	        POWER_CONTROL_REG__ENABLE_VCONN__WRITE(on));
}

static int tcpm_set_msg_header(int port, int power_role, int data_role)
{
	uint32_t reg = 0;
	MESSAGE_HEADER_INFO_REG__DATA_ROLE__MODIFY(reg, data_role);
	MESSAGE_HEADER_INFO_REG__POWER_ROLE__MODIFY(reg, power_role);
	MESSAGE_HEADER_INFO_REG__USB_PD_SPECIFICATION_REVISION__MODIFY(reg, PD_REV20);
	return tcpc_write32(port, cdstcpc_reg(MESSAGE_HEADER_INFO), reg);
}

static int tcpm_set_role(struct tcpc_dev *dev, bool attached,
    enum typec_role role, enum typec_data_role data)
{
	struct smartx_tcpc          *tcpc   = container_of(dev, struct smartx_tcpc, tcpc);
	int                         port    = tcpc->port_index;
	struct smartx_tcpc_notify   new     = tcpc->notify;
	//int                         reg;

	tcpm_set_msg_header(port, role, data);

	if(TYPEC_SINK == role) {
		if(TYPEC_DEVICE == data) {
			//writel(0x00000530, tcpc->user_define_base + 0x04);
			//writel(0x00000520, tcpc->user_define_base + 0x0004);
		} else {
			//writel(0x000004b0, tcpc->user_define_base + 0x04);
			//writel(0x000004A0, tcpc->user_define_base + 0x0004);
		}
	} else {
		if(TYPEC_DEVICE == data) {
			//writel(0x00000530, tcpc->user_define_base + 0x04);
			//writel(0x00000510, tcpc->user_define_base + 0x0004);
		} else {
			//writel(0x000004b0, tcpc->user_define_base + 0x04);
			//writel(0x00000490, tcpc->user_define_base + 0x0004);
		}
	}

	printk(KERN_ERR "set typec_role: %d, typec_data_role: %d, reg: 0x%08x",
	    role, data, readl(tcpc->user_define_base + 0x0004));

	new.data_role = data;
	new.usb = 1;
	//new.power_role = role;
	smartx_tcpm_notify(tcpc, new);

	return 0;
}

static int tcpm_alert_status(int port, int *alert)
{
	/* Read TCPC Alert register */
	return tcpc_read32(port, cdstcpc_reg(ALERT), alert);
}

int tcpm_set_rx_enable(struct tcpc_dev *dev, bool on)
{
	struct smartx_tcpc *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	int port = tcpc->port_index;

	/* If enable, then set RX detect for SOP and HRST */
	return tcpc_write32(port, cdstcpc_reg(RECEIVE_DETECT),
	        on ? (RECEIVE_DETECT_REG__ENABLE_HARD_RESET__WRITE(1)
	            | RECEIVE_DETECT_REG__ENABLE_SOP_MESSAGE__WRITE(1)) : 0);
}

//#ifdef CONFIG_USB_PD_TCPM_VBUS
static int tcpm_get_vbus_level(struct tcpc_dev *dev)
{
	struct smartx_tcpc *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	unsigned int power_status = 0;

	/* Read Power Status register */
	tcpm_get_power_status(tcpc->port_index, &power_status);
	/* Update VBUS status */
	return !!(0x04 & power_status);

}
//#endif

static int tcpm_get_message(int port, struct pd_message *msg)
{
	int i, rv, cnt;//, reg;

	rv = tcpc_read32(port, cdstcpc_reg(RECEIVE_BYTE_COUNT), &cnt);
	rv |= tcpc_read32(port, cdstcpc_reg(RX_BUF_HEADER_BYTE_10), (int *) & (msg->header));
	cnt -= TCPC_HEADER_LEN + TCPC_BYTE_CNT_LEN;

	printk(KERN_ERR "payload len: %d\n", cnt);

	if (rv == 0 && cnt > 0) {
		for (i = 0; i < ((cnt >> 2) + (cnt & 0x3 ? 1 : 0)); i++) {
			rv |= tcpc_read32(port, (cdstcpc_reg(RX_BUF_OBJ1_BYTE_3210) + (i << 2)), &(msg->payload[i]));
			printk(KERN_ERR "payload[%d]: %x\n", i, msg->payload[i]);
		}
	}

	/* Read complete, clear RX status alert bit */
	tcpc_write32(port, cdstcpc_reg(ALERT), ALERT_REG__RECEIVE_SOP_MESSAGE_STATUS__WRITE(1));
	return rv;
}

static int tcpm_transmit(struct tcpc_dev *dev, enum tcpm_transmit_type type,
    const struct pd_message *msg)
{
	int i, cnt, rv = 0;
	struct smartx_tcpc *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	int port = tcpc->port_index;

	if(msg) {
		cnt = 4 * pd_header_cnt(msg->header);

		rv = tcpc_write32(port, cdstcpc_reg(TRANSMIT_BYTE_COUNT), cnt + TCPC_HEADER_LEN);
		rv |= tcpc_write32(port, cdstcpc_reg(TX_BUF_HEADER_BYTE_10), msg->header);

		/* If tcpc read fails, return error */
		if (rv)
			goto complete;//return rv;

		if (cnt > 0) {
			for (i = 0; i < (cnt / 4); i++) {
				rv |= tcpc_write32(port, (cdstcpc_reg(TX_BUF_OBJ1_BYTE_3210) + (i << 2)), msg->payload[i]);
				//printk(KERN_ERR "TX payload[%d]: %x\n", i, msg->payload[i]);
			}
		}

		/* If tcpc read fails, return error */
		if (rv)
			goto complete;//return rv;

	}

complete:

	rv = tcpc_write32(port, cdstcpc_reg(TRANSMIT),
	        TRANSMIT_REG__RETRY_COUNTER__WRITE(3)
	        | TRANSMIT_REG__TRANSMIT_SOP_MESSAGE__WRITE(type));
	return rv;
}

static irqreturn_t smartx_dp_irq(int irq, void *data)
{
	struct smartx_tcpc          *tcpc = (struct smartx_tcpc *)data;
	struct smartx_tcpc_notify   new;
	int val;

	val = !(gpio_get_value(tcpc->hpd_gpio));

	if(val == 1) {
		new     = tcpc->notify;
		new.dp = 1;
		new.usb = 0;
		new.flip = 1;//must flip
		smartx_tcpm_notify(tcpc, new);
	} else if(val == 0) {
		new     = tcpc->notify;
		new.dp = 0;
		new.usb = 0;
		new.flip = 1;//must flip
		smartx_tcpm_notify(tcpc, new);
	}

	return IRQ_HANDLED;
}

static irqreturn_t smartx_tcpc_irq(int irq, void *data)
{
	struct smartx_tcpc    *tcpc = (struct smartx_tcpc *)data;
	int                 status;
	int                 port;
	//unsigned int         role_ctrl, cc1_status, cc2_status, rp_val;
	//union extcon_property_value property;

	mutex_lock(&tcpc->lock);

	if(NULL == tcpc || NULL == tcpc->tcpm) {
		goto err;
	}

	port = tcpc->port_index;

	if (tcpc_get_apb_busy(port)) {
		goto err;
	}

	/* Read the Alert register from the TCPC */
	tcpm_alert_status(port, &status);

	//++irqcount;
	//printk(KERN_ERR "tcpc irq value: AT 0x%08x CC 0x%08x PW 0x%08x FT 0x%08x count:%d\n", status,
	//readl(tcpc->tcpd_base + 0x0048), readl(tcpc->tcpd_base + 0x004c),
	//readl(tcpc->tcpd_base + 0x0050), irqcount);

	if (status & ALERT_MASK_REG__FAULT__MASK) {
		unsigned int fault_status;
		tcpc_read32(port, cdstcpc_reg(FAULT_STATUS), &fault_status);
		tcpc_write32(port, cdstcpc_reg(FAULT_STATUS), fault_status);
	}

	/*
	 * Clear alert status for everything except RX_STATUS, which shouldn't
	 * be cleared until we have successfully retrieved message.
	 */
	if (status & ~ALERT_REG__RECEIVE_SOP_MESSAGE_STATUS__MASK) {
		tcpc_write32(port, cdstcpc_reg(ALERT),
		    status & ~ALERT_REG__RECEIVE_SOP_MESSAGE_STATUS__MASK);
	}

	if (ALERT_REG__CC_STATUS__READ(status)) {
		tcpm_cc_change(tcpc->tcpm);
	}

	if (ALERT_REG__PORT_POWER_STATUS__READ(status)) {
		int reg = 0;

		tcpc_read32(port, cdstcpc_reg(POWER_STATUS_MASK), &reg);

		if (reg == TCPC_REG_POWER_STATUS_MASK_ALL) {
			/*
			 * If power status mask has been reset, then the TCPC
			 * has reset.
			 */
			tcpm_pd_hard_reset(tcpc->tcpm);
		} else {
			//unsigned int power_status = 0;
			/* Read Power Status register */
			//tcpm_get_power_status(port, &power_status);
			/* Update VBUS status */
			//tcpc_vbus[port] = ((reg & POWER_STATUS_MASK_REG__VBUS_PRESENT_STATUS_INTERRUPT_MASK__MASK) & power_status) ? 1 : 0;

			//printk(KERN_ERR "VBUS[%d]: %d\n", port, tcpc_vbus[port]);

			tcpm_vbus_change(tcpc->tcpm);
#if 0
			/* Update charge manager with new VBUS state */
			usb_charger_vbus_change(port, tcpc_vbus[port]);
			task_wake(PD_PORT_TO_TASK_ID(port));
#endif /* CONFIG_USB_PD_TCPM_VBUS && CONFIG_USB_CHARGER */
		}
	}

	if (ALERT_REG__RECEIVE_SOP_MESSAGE_STATUS__READ(status)) {
		struct pd_message msg;

		tcpm_get_message(port, &msg);

		tcpm_pd_receive(tcpc->tcpm, &msg);
	}

	if (ALERT_REG__RECEIVED_HARD_RESET__READ(status)) {
		/* hard reset received */
		tcpm_pd_hard_reset(tcpc->tcpm);
	}

	if (status & (ALERT_REG__TRANSMIT_SOP_MESSAGE_SUCCESSFUL__MASK
	        | ALERT_REG__TRANSMIT_SOP_MESSAGE_DISCARDED__MASK | ALERT_REG__TRANSMIT_SOP_MESSAGE_FAILED__MASK)) {
		/* transmit complete */
		if (ALERT_REG__TRANSMIT_SOP_MESSAGE_SUCCESSFUL__READ(status)) {
			tcpm_pd_transmit_complete(tcpc->tcpm, TCPC_TX_SUCCESS);
		} else if (ALERT_REG__TRANSMIT_SOP_MESSAGE_FAILED__READ(status)) {
			tcpm_pd_transmit_complete(tcpc->tcpm, TCPC_TX_FAILED);
		} else if (ALERT_REG__TRANSMIT_SOP_MESSAGE_DISCARDED__READ(status)) {
			tcpm_pd_transmit_complete(tcpc->tcpm, TCPC_TX_DISCARDED);
		}
	}

err:
	mutex_unlock(&tcpc->lock);

	return IRQ_HANDLED;
}

int tcpm_init(struct tcpc_dev *dev)
{
	struct smartx_tcpc *tcpc = container_of(dev, struct smartx_tcpc, tcpc);
	//int irq;
	//int val, ret;
	int port = tcpc->port_index;

	int rv;
	unsigned int timeout = 100;
	int power_status;
	int sfr_0;

	if(NULL == dev)
		return 0;

	printk(KERN_ERR "tcpm init start!\n");

	tcpc_read32(port, cdstcpc_reg(SFR_REG_0), &sfr_0);
	SFR_REG_0_REG__CFG_RESET__SET(sfr_0);
	tcpc_write32(port, cdstcpc_reg(SFR_REG_0), sfr_0);

	tcpc_read32(port, cdstcpc_reg(SFR_REG_0), &sfr_0);
	SFR_REG_0_REG__CFG_RESET__CLR(sfr_0);
	tcpc_write32(port, cdstcpc_reg(SFR_REG_0), sfr_0);

	while (timeout) {
		rv = tcpc_read32(port, cdstcpc_reg(SFR_REG_0), &sfr_0);
		if (SFR_REG_0_REG__ALERT_INIT__READ(sfr_0) == 0x0)
			break;
		msleep(10);
		--timeout;
	}

	printk(KERN_INFO "tcpm init!\n");

	timeout = 100;
	while (timeout) {
		rv = tcpc_read32(port, cdstcpc_reg(POWER_STATUS), &power_status);
		/*
		 * If read succeeds and the uninitialized bit is clear, then
		 * initialization is complete, clear all alert bits and write
		 * the initial alert mask.
		 */
		if (rv == 0 && !(POWER_STATUS_REG__TCPC_INITIALIZATION_STATUS__READ(power_status))) {
			unsigned int tmp;

			tcpc_write32(port, cdstcpc_reg(ALERT), 0xffff);
			/* Initialize power_status_mask */
			init_power_status_mask(port);
			/* Update VBUS status */
			tcpc_vbus[port] = POWER_STATUS_REG__VBUS_PRESENT__READ(power_status);

			tcpc_write32(port, cdstcpc_reg(FAULT_CONTROL), 0x10);

			tcpc_read32(port, cdstcpc_reg(ALERT), &tmp);
			if(tmp & 0x0200) {
				tcpc_read32(port, cdstcpc_reg(FAULT_STATUS), &tmp);
				tcpc_write32(port, cdstcpc_reg(FAULT_STATUS), tmp);
				tcpc_write32(port, cdstcpc_reg(ALERT), 0xffff);
				printk(KERN_ERR "tcpc fault occur! 0x%08x\n", tmp);
			}

			//enable vbus detection
			tcpc_write32(port, cdstcpc_reg(COMMAND),
			    COMMAND_REG__COMMAND__WRITE(CDNS_CMD_ENABLEVBUSDETECT));

			printk(KERN_INFO "tcpm init complete!\n");

			return init_alert_mask(port);
		}
		msleep(10);
		--timeout;
	}
	return 0;
}

static int tcpm_start_drp_toggling(struct tcpc_dev *dev, enum typec_cc_status cc)
{
	struct smartx_tcpc *tcpc    = container_of(dev, struct smartx_tcpc, tcpc);
	int                 port    = tcpc->port_index;
	unsigned int        reg     = 0;

	//set strap to drp
	writel(0x00000430, tcpc->user_define_base + 0x04);

	//there is some problem to set udp board cc to rd, so we just set rp def
	if(TYPEC_CC_RD == cc)
		cc = TYPEC_CC_RP_DEF;

	//set default cc
	tcpc_read32(port, cdstcpc_reg(ROLE_CONTROL), &reg);
	switch (cc) {
	case TYPEC_CC_RD:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, CDNS_TYPEC_CC_RD);
		ROLE_CONTROL_REG__CC2__MODIFY(reg, CDNS_TYPEC_CC_RD);
		break;
	case TYPEC_CC_RP_DEF:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, CDNS_TYPEC_CC_RP);//pull up
		ROLE_CONTROL_REG__CC2__MODIFY(reg, CDNS_TYPEC_CC_RP);//pull up
		ROLE_CONTROL_REG__RP_VALUE__MODIFY(reg, CDNS_TYPEC_CC_RP_DEF);
		break;
	case TYPEC_CC_RP_1_5:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, 0x01);//pull up
		ROLE_CONTROL_REG__CC2__MODIFY(reg, 0x01);//pull up
		ROLE_CONTROL_REG__RP_VALUE__MODIFY(reg, CDNS_TYPEC_CC_RP_1_5);
		break;
	case TYPEC_CC_RP_3_0:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, 0x01);//pull up
		ROLE_CONTROL_REG__CC2__MODIFY(reg, 0x01);//pull up
		ROLE_CONTROL_REG__RP_VALUE__MODIFY(reg, CDNS_TYPEC_CC_RP_3_0);
		break;
	case TYPEC_CC_OPEN:
		ROLE_CONTROL_REG__CC1__MODIFY(reg, CDNS_TYPEC_CC_OPEN);
		ROLE_CONTROL_REG__CC2__MODIFY(reg, CDNS_TYPEC_CC_OPEN);
		break;
	default:
		return -EINVAL;
	}

	//set drp role
	ROLE_CONTROL_REG__DRP__MODIFY(reg, CDNS_TYPEC_CC_ENABLE_DRP);

	//set reg
	tcpc_write32(port, cdstcpc_reg(ROLE_CONTROL), reg);
	printk(KERN_INFO "drp set CC:%d reg:0x%08x\n", cc, reg);

	//start drp toggling
	return tcpc_write32(port, cdstcpc_reg(COMMAND),
	        COMMAND_REG__COMMAND__WRITE(CDNS_CMD_LOOK4CONNECTION));
}

static void init_tcpc_dev(struct tcpc_dev *smartx_tcpc_dev)
{
	smartx_tcpc_dev->init = tcpm_init;
	smartx_tcpc_dev->get_vbus = tcpm_get_vbus_level;
	//smartx_tcpc_dev->get_current_limit = tcpm_get_current_limit;
	smartx_tcpc_dev->set_cc = tcpm_set_cc;
	smartx_tcpc_dev->get_cc = tcpm_get_cc;
	smartx_tcpc_dev->set_polarity = tcpm_set_polarity;
	smartx_tcpc_dev->set_vconn = tcpm_set_vconn;
	smartx_tcpc_dev->set_vbus = tcpm_set_vbus;
	smartx_tcpc_dev->set_pd_rx = tcpm_set_rx_enable;
	smartx_tcpc_dev->set_roles = tcpm_set_role;
	smartx_tcpc_dev->start_drp_toggling = tcpm_start_drp_toggling;
	smartx_tcpc_dev->pd_transmit = tcpm_transmit;
}

static void smartx_tcpc_usb_dp_switch_work(struct work_struct *work)
{
	struct smartx_tcpc              *tcpc = container_of(work, struct smartx_tcpc, usb_dp_switch_work);
	struct smartx_tcpc_notify       new = tcpc->notify;

	if (tcpc->extcon && extcon_get_cable_state_(tcpc->extcon, EXTCON_CHG_USB_FAST)) {
		union extcon_property_value property;
		int ret = extcon_get_property(tcpc->extcon, EXTCON_CHG_USB_FAST, EXTCON_PROP_USB_TYPEC_POLARITY,
		        &property);
		if(ret < 0) {
			printk(KERN_ERR "extcon_get_property failed!!!\n");
			return;
		}

		if(property.intval) {
			new.dp = 1;
			smartx_tcpm_notify(tcpc, new);
		} else {
			if(extcon_get_cable_state_(tcpc->extcon, EXTCON_CHG_USB_SLOW)) {
				new.usb = 0;
				smartx_tcpm_notify(tcpc, new);
			} else {
				new.usb = 0;
				new.dp = 1;
				smartx_tcpm_notify(tcpc, new);
			}
		}

		return;
	} else if (tcpc->extcon && !extcon_get_cable_state_(tcpc->extcon, EXTCON_CHG_USB_FAST)) {
		new.dp = 0;
		new.usb = 1;
		smartx_tcpm_notify(tcpc, new);
		return;
	}
}

static int smartx_tcpc_usb_dp_switch_notifier(struct notifier_block *nb,
    unsigned long event, void *ptr)
{
	struct smartx_tcpc *tcpc =
	    container_of(nb, struct smartx_tcpc, usb_dp_switch_nb);

	schedule_work(&tcpc->usb_dp_switch_work);

	return NOTIFY_DONE;
}

static int smartx_tcpc_extcon_register(struct smartx_tcpc *tcpc)
{
	int         ret;
	struct device       *dev = tcpc->dev;
	struct extcon_dev   *edev = tcpc->extcon;

	tcpc->usb_dp_switch_nb.notifier_call = smartx_tcpc_usb_dp_switch_notifier;
	ret = extcon_register_notifier(edev, EXTCON_CHG_USB_FAST,
	        &tcpc->usb_dp_switch_nb);
	if (ret < 0) {
		dev_err(dev, "failed to register notifier for USB\n");
		return ret;
	}

	return 0;
}

static void loadtcpcfw(const struct firmware *cdns_fw, void *context)
{
	struct smartx_tcpc              *tcpc   = NULL;
	struct device                   *dev    = NULL;
	//void __iomem                    *mem    = NULL;
	struct device_node              *node   = NULL;
	int                             ret;
	int                             reg = 0;
	int                             val = 0;

	if(NULL == cdns_fw || 16384 >= cdns_fw->size || NULL == cdns_fw->data) {
		printk(KERN_ERR "failed to open cdns firmware file: %s\n", TCPC_FW_NAME);
		return;
	}

	if(NULL == context) {
		printk(KERN_ERR "context is invalid! load tcpc fw failed!!!");
		return;
	}

	tcpc = (struct smartx_tcpc *)context;
	dev = tcpc->dev;
	if(NULL == dev) {
		printk(KERN_ERR "tcpc dev is invalid! load tcpc fw failed!!!");
		return;
	}

	if(NULL == tcpc->tcpd_base) {
		printk(KERN_ERR "tcpc tcpd_base is invalid! load tcpc fw failed!!!");
		return;
	}
	printk(KERN_INFO "tcpd_base: 0x%p 0x%08x 0x%08x size: %d %d", tcpc->tcpd_base,
	    (((unsigned int)tcpc->tcpd_base) + 0x8000),
	    (((unsigned int)tcpc->tcpd_base) + 0x200), 16384, cdns_fw->size - 16384);

	node   = dev->of_node;

	//reset asserted
	tcpc_read32(tcpc->port_index, cdstcpc_reg(SFR_REG_0), &reg);
	SFR_REG_0_REG__CFG_RESET__SET(reg);
	tcpc_write32(tcpc->port_index, cdstcpc_reg(SFR_REG_0), reg);

	//iram fw at first and size is fixed 16384 bytes
	memcpy((void *)(((unsigned int)tcpc->tcpd_base) + 0x8000), cdns_fw->data, 16384);
	printk(KERN_INFO "success load cdns firmware iram len: %d\n", 16384);

	//dram fw at second and size is not fixed
	memcpy((void *)(((unsigned int)tcpc->tcpd_base) + 0x200), cdns_fw->data + 16384, cdns_fw->size - 16384);
	printk(KERN_INFO "success load cdns firmware dram len: %d\n", cdns_fw->size - 16384);

	release_firmware(cdns_fw);

	if(USB_DR_MODE_DP_ONLY == tcpc->dr_mode) {

		union extcon_property_value property;

		//enable usb
		writel(0x0000153d, tcpc->user_define_base + 0x08);

		reg = 0x400;
		reg |= (0x80 | 0x10);
		tcpc->mode_cfg_reg = reg;
		writel(tcpc->mode_cfg_reg, tcpc->user_define_base + 0x04);

		//indicate normal dp
		property.intval = 1;
		ret = extcon_set_property(tcpc->extcon, EXTCON_DISP_DP,
		        EXTCON_PROP_USB_SS, property);

		tcpc_read32(tcpc->port_index, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), &val);
		val &= ~0x0c;
		val |= (0x08);
		tcpc_write32(tcpc->port_index, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), val);

		val = !(gpio_get_value(tcpc->hpd_gpio));
		if(val == 1) {
			struct smartx_tcpc_notify   new     = tcpc->notify;
			printk(KERN_INFO "dp default connect!\n");
			new.dp = 1;
			new.usb = 0;
			new.flip = 1;//must flip
			smartx_tcpm_notify(tcpc, new);
		}

		//probe normal dp
		if(node) {
			ret = of_platform_populate(node, NULL, NULL, dev);
			if (ret) {
				dev_err(dev, "failed to create children: %d\n", ret);
			}
		}

		ret = devm_request_threaded_irq(dev, tcpc->irq, NULL,
		        smartx_dp_irq, IRQF_ONESHOT | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		        "dp", tcpc);
		if (ret != 0) {
			printk(KERN_ERR "cannot request irq %d err %d\n", tcpc->irq,
			    ret);
			return;
		}
	} else {
		init_tcpc_dev(&tcpc->tcpc);

		tcpc->tcpm = tcpm_register_port(dev, &tcpc->tcpc);
		if (IS_ERR(tcpc->tcpm)) {
			ret = PTR_ERR(tcpc->tcpm);
			if (ret != -EPROBE_DEFER)
				dev_err(dev, "cannot register tcpm port, ret=%d", ret);
		}

		//enable usb
		writel(0x0000153d, tcpc->user_define_base + 0x08);

		/* ROLE_CONTROL */
		if(NULL != tcpc->tcpc.config) {
			reg = 0x400;
			switch(tcpc->tcpc.config->data) {
			case TYPEC_PORT_DFP:
				reg |= (0x80 | 0x10);
				break;

			case TYPEC_PORT_UFP:
				reg |= (0x100 | 0x20);
				break;

			case TYPEC_PORT_DRD:
			default:
				//set drd
				reg |= 0x30;
				break;
			}
			tcpc->mode_cfg_reg = reg;
			writel(tcpc->mode_cfg_reg, tcpc->user_define_base + 0x04);
		}

		tcpc_read32(tcpc->port_index, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), &val);
		val &= ~0x0c;
		val |= (0x04);
		//val |= (0x0c);
		tcpc_write32(tcpc->port_index, cdstcpc_reg(CONFIG_STANDARD_OUTPUT), val);

		//probe type c usb or dp
		if(node) {
			ret = of_platform_populate(node, NULL, NULL, dev);
			if (ret) {
				dev_err(dev, "failed to create children: %d\n", ret);
			}
		}

		ret = devm_request_threaded_irq(dev, tcpc->irq, NULL,
		        smartx_tcpc_irq, IRQF_ONESHOT,
		        "tcpc", tcpc);
		if (ret != 0) {
			printk(KERN_ERR "cannot request irq %d err %d\n", tcpc->irq, ret);
			return;
		}
	}
}

static int smartx_tcpc_probe(struct platform_device *pdev)
{
	struct device               *dev    = &pdev->dev;
	struct smartx_tcpc          *tcpc;

	struct resource             *res;

	//unsigned int                val;
	int                         ret = 0;

	int                         cc1_gpio = -1;
	int                         cc2_gpio = -1;
	int                         typec_gpio = -1;

	tcpc = devm_kzalloc(dev, sizeof(*tcpc), GFP_KERNEL);
	if (!tcpc)
		return -ENOMEM;

	memset(tcpc, 0, sizeof(*tcpc));

	tcpc->dev = dev;
	platform_set_drvdata(pdev, tcpc);

	memset(&tcpc->notify, 0xff, sizeof(struct smartx_tcpc_notify));

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk(KERN_ERR "platform_get_resource 0 failed!\n");
		return -ENODEV;
	}
	printk(KERN_INFO "tcpc platform_get_resource start: %08x\n", res->start);

	tcpc->tcpd_base = devm_ioremap_resource(dev, res);
	if(NULL == tcpc->tcpd_base) {
		printk(KERN_ERR "ioremap tcpd_base failed!\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if(!res) {
		printk(KERN_ERR "tcpc platform_get_resource 1 failed!\n");
		return -ENODEV;
	}
	printk(KERN_INFO "platform_get_resource start: %08x\n", res->start);

	tcpc->user_define_base = devm_ioremap_resource(dev, res);
	if(NULL == tcpc->user_define_base) {
		printk(KERN_ERR "ioremap user_define_base failed!\n");
		return -ENODEV;
	}

	tcpc->extcon = devm_extcon_dev_allocate(dev, smartx_tcpm_cable);
	if (IS_ERR(tcpc->extcon)) {
		printk(KERN_ERR "allocat extcon failed\n");
		return PTR_ERR(tcpc->extcon);
	}

	ret = devm_extcon_dev_register(dev, tcpc->extcon);
	if (ret) {
		printk(KERN_ERR "failed to register extcon: %d\n",
		    ret);
		return ret;
	}

	ret = extcon_set_property_capability(tcpc->extcon, EXTCON_USB,
	        EXTCON_PROP_USB_TYPEC_POLARITY);
	if (ret) {
		printk(KERN_ERR
		    "failed to set USB property capability: %d\n",
		    ret);
		return ret;
	}

	ret = extcon_set_property_capability(tcpc->extcon, EXTCON_USB_HOST,
	        EXTCON_PROP_USB_TYPEC_POLARITY);
	if (ret) {
		printk(KERN_ERR
		    "failed to set USB_HOST property capability: %d\n",
		    ret);
		return ret;
	}

	ret = extcon_set_property_capability(tcpc->extcon, EXTCON_DISP_DP,
	        EXTCON_PROP_USB_TYPEC_POLARITY);
	if (ret) {
		printk(KERN_ERR
		    "failed to set DISP_DP property capability: %d\n",
		    ret);
		return ret;
	}

	ret = extcon_set_property_capability(tcpc->extcon, EXTCON_USB,
	        EXTCON_PROP_USB_SS);
	if (ret) {
		printk(KERN_ERR
		    "failed to set USB USB_SS property capability: %d\n",
		    ret);
		return ret;
	}

	ret = extcon_set_property_capability(tcpc->extcon, EXTCON_USB_HOST,
	        EXTCON_PROP_USB_SS);
	if (ret) {
		printk(KERN_ERR
		    "failed to set USB_HOST USB_SS property capability: %d\n",
		    ret);
		return ret;
	}

	ret = extcon_set_property_capability(tcpc->extcon, EXTCON_DISP_DP,
	        EXTCON_PROP_USB_SS);
	if (ret) {
		printk(KERN_ERR
		    "failed to set DISP_DP USB_SS property capability: %d\n",
		    ret);
		return ret;
	}

	ret = extcon_set_property_capability(tcpc->extcon, EXTCON_CHG_USB_FAST,
	        EXTCON_PROP_USB_TYPEC_POLARITY);
	if (ret) {
		printk(KERN_ERR
		    "failed to set USB_PD property capability: %d\n", ret);
		return ret;
	}

	mutex_init(&tcpc->lock);

	tcpc->port_index = 0;//only 1
	tcpc_hw_addr[tcpc->port_index] = tcpc->tcpd_base;
	tcpc->dr_mode = usb_get_dr_mode(dev);
	//tcpc->is_dp_connected = 0;

	if(USB_DR_MODE_DP_ONLY == tcpc->dr_mode) {
		//tcpc->mode = 1;//force dp
		tcpc->hpd_gpio = of_get_named_gpio(dev->of_node, "hpd-gpios", 0);
		if(tcpc->hpd_gpio >= 0) {
			printk(KERN_INFO "hpd irq_gpio num: %d\n", tcpc->hpd_gpio);

			if(gpio_is_valid(tcpc->hpd_gpio) && (0 == gpio_request(tcpc->hpd_gpio, "hpd-gpios"))) {
				gpio_direction_input(tcpc->hpd_gpio);
			} else {
				printk(KERN_ERR "gpio check failed\n");
				return ret;
			}

			tcpc->irq = gpio_to_irq(tcpc->hpd_gpio);
			if(tcpc->irq >= 0) {
				printk(KERN_ERR "dp_hpd_get_irq num: %d\n", tcpc->irq);
			}
		} else {
			printk(KERN_ERR "hpd-gpios gpio must be set in dp only mode!!!\n");
		}
	} else {
		tcpc->mode = 0;
		if(tcpc->dr_mode == USB_DR_MODE_PERIPHERAL) {
			tcpc->tcpc.config = &smartx_tcpc_device_config;
			printk(KERN_INFO "tcpc device mode!\n");
		} else if(tcpc->dr_mode == USB_DR_MODE_OTG) {
			tcpc->tcpc.config = &smartx_tcpc_otg_config;
			printk(KERN_INFO "tcpc otg mode!\n");
		} else {
			tcpc->tcpc.config = &smartx_tcpc_host_config;
			printk(KERN_INFO "tcpc host mode!\n");
		}

		cc1_gpio = of_get_named_gpio(dev->of_node, "cc1-gpios", 0);
		if(cc1_gpio >= 0) {
			printk(KERN_CRIT "cc1_gpio num: %d\n", cc1_gpio);

			if(gpio_is_valid(cc1_gpio) && (0 == gpio_request(cc1_gpio, "cc1-gpios"))) {
				gpio_direction_output(cc1_gpio, 1);
			} else {
				printk(KERN_CRIT "gpio check failed\n");
			}
		}
		tcpc->cc1_gpio = cc1_gpio;

		cc2_gpio = of_get_named_gpio(dev->of_node, "cc2-gpios", 0);
		if(cc2_gpio >= 0) {
			printk(KERN_CRIT "cc2_gpio num: %d\n", cc2_gpio);

			if(gpio_is_valid(cc2_gpio) && (0 == gpio_request(cc2_gpio, "cc2-gpios"))) {
				gpio_direction_output(cc2_gpio, 1);
			} else {
				printk(KERN_CRIT "gpio check failed\n");
			}
		}
		tcpc->cc2_gpio = cc2_gpio;

		typec_gpio = of_get_named_gpio(dev->of_node, "typec-gpios", 0);
		if(typec_gpio >= 0) {
			printk(KERN_CRIT "typec_gpio num: %d\n", typec_gpio);

			if(gpio_is_valid(typec_gpio) && (0 == gpio_request(typec_gpio, "typec-gpios"))) {
				int ret;
				ret = gpio_direction_output(typec_gpio, 0);
				if(ret < 0) {
					printk(KERN_CRIT "gpio output fail!(errno: %d)\n", ret);
				}
			} else {
				printk(KERN_CRIT "gpio check failed\n");
			}
		}
		tcpc->typec_gpio = typec_gpio;

		tcpc->irq = platform_get_irq(pdev, 0);
	}

	INIT_WORK(&tcpc->usb_dp_switch_work, smartx_tcpc_usb_dp_switch_work);
	smartx_tcpc_extcon_register(tcpc);

	ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
	        TCPC_FW_NAME, dev, GFP_KERNEL, tcpc, loadtcpcfw);
	if(ret < 0) {
		printk(KERN_ERR "failed to open cdns firmware file: %s (%d)", TCPC_FW_NAME, ret);
		return ret;
	}

	printk(KERN_INFO "first smartx_tcpc_probe success %p -----------------\n", tcpc);

	return ret;

}

static int smartx_tcpc_remove(struct platform_device *pdev)
{
	struct smartx_tcpc          *tcpc = platform_get_drvdata(pdev);

	if(tcpc->dev)
		of_platform_depopulate(tcpc->dev);

	if(tcpc->tcpm)
		tcpm_unregister_port(tcpc->tcpm);

	cancel_work_sync(&tcpc->usb_dp_switch_work);

	//destroy_workqueue(chip->wq);

	return 0;
}

static int smartx_pm_suspend(struct device *dev)
{
	//struct fusb302_chip *chip = dev->driver_data;

	//if (atomic_read(&chip->i2c_busy))
	//    return -EBUSY;
	//atomic_set(&chip->pm_suspend, 1);

	return 0;
}

static int smartx_pm_resume(struct device *dev)
{
	//struct fusb302_chip *chip = dev->driver_data;

	//atomic_set(&chip->pm_suspend, 0);

	return 0;
}

static const struct of_device_id smartx_tcpc_ids[] = {
	{ .compatible = "smartchip, smartx-tcpc" },
	{ }
};
MODULE_DEVICE_TABLE(of, smartx_tcpm_ids);

static const struct dev_pm_ops smartx_pm_ops = {
	.suspend = smartx_pm_suspend,
	.resume = smartx_pm_resume,
};

static struct platform_driver smartx_tcpc_driver = {
	.probe          = smartx_tcpc_probe,
	.remove         = smartx_tcpc_remove,
	.driver         = {
		.name   = "smartx-tcpc",
		.pm     = &smartx_pm_ops,
		.of_match_table = smartx_tcpc_ids,
	},
};

module_platform_driver(smartx_tcpc_driver);

MODULE_ALIAS("platform:smartx tcpc");
MODULE_AUTHOR("SmartChip Electronics Inc.");
MODULE_DESCRIPTION("Candence USB tcpc driver");
MODULE_LICENSE("GPL v2");
