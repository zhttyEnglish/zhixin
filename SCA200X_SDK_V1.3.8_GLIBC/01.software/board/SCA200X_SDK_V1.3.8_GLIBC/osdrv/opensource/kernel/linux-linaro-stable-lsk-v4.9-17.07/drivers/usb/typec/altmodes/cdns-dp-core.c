/*
 * Copyright (C) Fuzhou Rockchip Electronics Co.Ltd
 * Author: Chris Zhong <zyw@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * (C) Copyright 2022 SmartChip Electronics Co., Ltd
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <drm/drmP.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_dp_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_of.h>

#include <linux/arm-smccc.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/crc16.h>
#include <linux/extcon.h>
#include <linux/firmware.h>
//#include <linux/hdmi-notifier.h>
#include <linux/iopoll.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/mfd/syscon.h>
#include <linux/nvmem-consumer.h>
#include <linux/phy/phy.h>
#include <uapi/linux/videodev2.h>

#include <sound/hdmi-codec.h>

#include <linux/sc_icc.h>

#include "cdns-dp-core.h"
#include "cdns-dp-reg.h"
//#include "rockchip_drm_vop.h"

#define HDCP_TOGGLE_INTERVAL_US        10000
#define HDCP_RETRY_INTERVAL_US        20000
#define HDCP_EVENT_TIMEOUT_US        3000000
#define HDCP_AUTHENTICATE_DELAY_MS    400

#define HDCP_KEY_DATA_START_TRANSFER    0
#define HDCP_KEY_DATA_START_DECRYPT    1

#define RK_SIP_HDCP_CONTROL        0x82000009
#define RK_SIP_HDCP_KEY_DATA64        0xC200000A

#define connector_to_dp(c) \
        container_of(c, struct cdn_dp_device, connector)

#define encoder_to_dp(c) \
        container_of(c, struct cdn_dp_device, encoder)

#define GRF_SOC_CON9        0x6224
#define DP_SEL_VOP_LIT        BIT(12)
#define GRF_SOC_CON26        0x6268
#define DPTX_HPD_SEL        (3 << 12)
#define DPTX_HPD_DEL        (2 << 12)
#define DPTX_HPD_SEL_MASK    (3 << 28)

#define CDN_FW_TIMEOUT_MS    (64 * 1000)
#define CDN_DPCD_TIMEOUT_MS    5000
#define CDN_DP_FIRMWARE                "dp.bin"

#define DP_ICC_MESSAGE_ID                0xffff

struct cdn_dp_data {
	u8 max_phy;
};

struct cdn_dp_data smartx_cdn_dp = {
	.max_phy = 1,
};

static const struct of_device_id cdn_dp_dt_ids[] = {
	{
		.compatible = "smartchip,smartx-dp",
		.data = (void *) &smartx_cdn_dp
	},
	{}
};
MODULE_DEVICE_TABLE(of, cdn_dp_dt_ids);

static int cdn_dp_clk_enable(struct cdn_dp_device *dp)
{
	//int ret;
	//unsigned long rate;

	cdn_dp_set_fw_clk(dp, 125000000);
	//cdn_dp_set_fw_clk(dp, rate);
	cdn_dp_clock_reset(dp);

	return 0;
}

static void cdn_dp_clk_disable(struct cdn_dp_device *dp)
{
}

static int cdn_dp_get_port_lanes(struct cdn_dp_port *port)
{
	//struct extcon_dev *edev = port->extcon;
	//union extcon_property_value property;
	//int dptx;
	u8 lanes = 4;

	/*dptx = extcon_get_state(edev, EXTCON_DISP_DP);
	if (dptx > 0) {
	    extcon_get_property(edev, EXTCON_DISP_DP,
	                EXTCON_PROP_USB_SS, &property);
	    if (property.intval)
	        lanes = 2;
	    else
	        lanes = 4;
	} else {
	    lanes = 0;
	}*/

	return lanes;
}

static struct cdn_dp_port *cdn_dp_connected_port(struct cdn_dp_device *dp)
{
	struct cdn_dp_port *port;
	int i, lanes;

	for (i = 0; i < dp->ports; i++) {
		port = dp->port[i];
		lanes = cdn_dp_get_port_lanes(port);
		if (lanes)
			return port;
	}
	return NULL;
}

static int cdn_dp_firmware_init(struct cdn_dp_device *dp)
{
	int ret;
	const u32 *iram_data, *dram_data;
	const struct firmware *fw = dp->fw;

	//iram is first and size is fixed(65536)
	iram_data = (const u32 *)(fw->data);
	//iram is second and size is fixed(32768)
	dram_data = (const u32 *)(fw->data + 65536);

	ret = cdn_dp_load_firmware(dp, iram_data, 65536,
	        dram_data, fw->size - 65536);
	if (ret)
		return ret;

	ret = cdn_dp_set_firmware_active(dp, true);
	if (ret) {
		printk(KERN_ERR "active ucpu failed: %d\n", ret);
		return ret;
	}

	return cdn_dp_event_config(dp);
}

static int cdn_dp_enable_phy(struct cdn_dp_device *dp, struct cdn_dp_port *port)
{
	union extcon_property_value property;
	int ret;

	if(NULL == port || NULL == port->phy)
		return -1;

	printk(KERN_ERR "port->phy: %p\n", port->phy);
	//set mode
	smartx_typec_set_config_mode(dp->tcpc);

	if (!port->phy_enabled) {
		ret = phy_power_on(port->phy);
		if (ret) {
			printk(KERN_ERR "phy power on failed: %d\n",
			    ret);
			goto err_phy;
		}
		port->phy_enabled = true;
	}

	//fw access phy
	smartx_typec_common_phy_access(dp->tcpc, 1);

	/*ret = cdn_dp_get_hpd_status(dp);
	if (ret <= 0) {
	    if (!ret)
	        printk(KERN_ERR "hpd does not exist\n");
	    goto err_power_on;
	}*/

	ret = extcon_get_property(port->extcon, EXTCON_DISP_DP,
	        EXTCON_PROP_USB_TYPEC_POLARITY, &property);
	if (ret) {
		printk(KERN_ERR "get property failed\n");
		goto err_power_on;
	}

	printk(KERN_ERR "EXTCON_DISP_DP filped: %d", property.intval ? 1 : 0);

	port->lanes = cdn_dp_get_port_lanes(port);
	ret = cdn_dp_set_host_cap(dp, port->lanes, property.intval);
	if (ret) {
		printk(KERN_ERR "set host capabilities failed: %d\n",
		    ret);
		goto err_power_on;
	}

	dp->active_port = port->id;
	return 0;

err_power_on:
	smartx_typec_common_phy_access(dp->tcpc, 0);
	if (phy_power_off(port->phy))
		printk(KERN_ERR "phy power off failed: %d", ret);
	else
		port->phy_enabled = false;

err_phy:
	//cdn_dp_grf_write(dp, GRF_SOC_CON26,
	//         DPTX_HPD_SEL_MASK | DPTX_HPD_DEL);
	return ret;
}

static int cdn_dp_disable_phy(struct cdn_dp_device *dp,
    struct cdn_dp_port *port)
{
	int ret;

	smartx_typec_common_phy_access(dp->tcpc, 0);

	if (port->phy_enabled) {
		ret = phy_power_off(port->phy);
		if (ret) {
			printk(KERN_ERR "phy power off failed: %d", ret);
			return ret;
		}
	}

	port->phy_enabled = false;
	port->lanes = 0;
	dp->active_port = -1;
	return 0;
}

static int cdn_dp_disable(struct cdn_dp_device *dp)
{
	int ret, i;

	if (!dp->active)
		return 0;

	ret = cdn_dp_set_firmware_active(dp, false);
	if (ret) {
		printk(KERN_ERR "inactive ucpu failed: %d\n", ret);
		return ret;
	}

	for (i = 0; i < dp->ports; i++)
		cdn_dp_disable_phy(dp, dp->port[i]);

	dp->active = false;
	dp->link.rate = 0;
	dp->link.num_lanes = 0;
	if (!dp->connected) {
		kfree(dp->edid);
		dp->edid = NULL;
	}

	return 0;
}

static int cdn_dp_enable(struct cdn_dp_device *dp)
{
	int ret, i, lanes;
	struct cdn_dp_port *port;

	port = cdn_dp_connected_port(dp);
	if (!port) {
		printk(KERN_ERR
		    "Can't enable without connection\n");
		return -ENODEV;
	}

	if (dp->active)
		return 0;

	ret = cdn_dp_clk_enable(dp);
	if (ret)
		return ret;

	ret = cdn_dp_firmware_init(dp);
	if (ret) {
		printk(KERN_ERR "firmware init failed: %d", ret);
		goto err_clk_disable;
	}

	/* only enable the port that connected with downstream device */
	for (i = port->id; i < dp->ports; i++) {
		port = dp->port[i];
		lanes = cdn_dp_get_port_lanes(port);
		if (lanes) {
			ret = cdn_dp_enable_phy(dp, port);
			if (ret)
				continue;

			dp->active = true;
			dp->lanes = port->lanes;
		}
	}

	printk(KERN_ERR "cdn_dp_enable success\n");

	return ret;

err_clk_disable:
	cdn_dp_clk_disable(dp);
	return ret;
}

static int cdn_dp_parse_dt(struct cdn_dp_device *dp)
{
	struct device *dev = dp->dev;
	//struct device_node *np = dev->of_node;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dp->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(dp->regs)) {
		printk(KERN_ERR "ioremap reg failed\n");
		return PTR_ERR(dp->regs);
	}

	printk(KERN_INFO "======================================>dp->regs %p\n", dp->regs);

	return 0;
}

static int cdn_dp_bind(struct device *dev, struct device *master, void *data)
{
	struct cdn_dp_device *dp = dev_get_drvdata(dev);
	//struct drm_encoder *encoder;
	//struct drm_connector *connector;
	//struct cdn_dp_port *port;
	int ret, i = 0;

	printk(KERN_ERR "cdn_dp_bind\n");

	dp->connected = false;
	dp->active = false;
	dp->active_port = -1;

	while(dp->fw_loaded == 0) {
		++i;
		if(i > 1000)
			return -1;

		usleep_range(10000, 20000);
	}

#ifdef CONFIG_SWITCH
	dp->switchdev.name = "cdn-dp";
	switch_dev_register(&dp->switchdev);
#endif

	pm_runtime_enable(dev);

	printk(KERN_INFO "Connected, not enabled. Enabling cdn\n");
	ret = cdn_dp_enable(dp);
	if (ret) {
		printk(KERN_INFO "Enable dp failed %d\n", ret);
		dp->connected = false;
	}

	return 0;
}

static void cdn_dp_unbind(struct device *dev, struct device *master, void *data)
{
	struct cdn_dp_device *dp = dev_get_drvdata(dev);

#ifdef CONFIG_SWITCH
	switch_dev_unregister(&dp->switchdev);
#endif

	pm_runtime_disable(dev);

	kfree(dp->edid);
	dp->edid = NULL;
}

static const struct component_ops cdn_dp_component_ops = {
	.bind = cdn_dp_bind,
	.unbind = cdn_dp_unbind,
};

int cdn_dp_suspend(struct device *dev)
{
	struct cdn_dp_device *dp = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&dp->lock);
	if (dp->active)
		ret = cdn_dp_disable(dp);
	dp->suspended = true;
	mutex_unlock(&dp->lock);

	return ret;
}

int cdn_dp_resume(struct device *dev)
{
	struct cdn_dp_device *dp = dev_get_drvdata(dev);

	mutex_lock(&dp->lock);
	dp->suspended = false;
	if (dp->fw_loaded)
		schedule_work(&dp->event_work);
	mutex_unlock(&dp->lock);

	return 0;
}

static void cdn_dp_pd_event_work(struct work_struct *work)
{
	struct cdn_dp_device *dp = container_of(work, struct cdn_dp_device,    event_work);
	struct cdn_dp_port *port = dp->port[0];
	int status = 0;
	int send_msg = 0;
	int recv_msg = 0;

	if(dp->dp_icc_client == NULL) {
		printk(KERN_INFO "icc client is null!\n");
		return;
	}

	printk(KERN_INFO "icc communication!\n");

	while(dp->is_icc_connected == 0) {

		status = sc_icc_client_receive(dp->dp_icc_client, (unsigned char *)&recv_msg, sizeof(recv_msg), dp->dp_icc_msg_id);
		if(status >= 0 && recv_msg != 0) {
			dp->is_icc_connected = 1;
			printk(KERN_INFO "icc connected!\n");
			break;
		}
	}

	if (port->extcon && extcon_get_cable_state_(port->extcon, EXTCON_DISP_DP) && 0 == dp->is_dp_connected) {
		dp->is_dp_connected = 1;
		smartx_typec_common_phy_access(dp->tcpc, 0);
		cdn_dp_bind(dp->dev, NULL, NULL);
		send_msg = dp->is_dp_connected;
		sc_icc_client_send(dp->dp_icc_client, (unsigned char *)&send_msg, sizeof(send_msg), dp->dp_icc_msg_id, ICC_CORE_A_3);
		printk(KERN_INFO "icc send status: %d!\n", send_msg);
	} else if(port->extcon && 0 == extcon_get_cable_state_(port->extcon, EXTCON_DISP_DP) && 1 == dp->is_dp_connected) {
		dp->is_dp_connected = 0;
		send_msg = dp->is_dp_connected;
		sc_icc_client_send(dp->dp_icc_client, (unsigned char *)&send_msg, sizeof(send_msg), dp->dp_icc_msg_id, ICC_CORE_A_3);
		printk(KERN_INFO "icc send status: %d!\n", send_msg);
		//must wait rtos stop
		status = sc_icc_client_receive(dp->dp_icc_client, (unsigned char *)&recv_msg, sizeof(recv_msg), dp->dp_icc_msg_id);
		cdn_dp_disable(dp);
	}

	//printk(KERN_INFO "extcon %p DP cable_state %d is_dp_connected %d !\n", port->extcon,
	//       extcon_get_cable_state_(port->extcon, EXTCON_DISP_DP), dp->is_dp_connected);
}

static int cdn_dp_pd_event(struct notifier_block *nb,
    unsigned long event, void *priv)
{
	struct cdn_dp_port *port = container_of(nb, struct cdn_dp_port,    event_nb);
	struct cdn_dp_device *dp = port->dp;

	//printk(KERN_INFO "==================================>cdn_dp_pd_event!\n");
	schedule_work(&dp->event_work);

	return NOTIFY_DONE;
}

static void load_dp_fw(const struct firmware *cdns_fw, void *context)
{
	if(NULL == cdns_fw || 0 == cdns_fw->size || NULL == cdns_fw->data) {
		printk(KERN_ERR "failed to open cdns dp firmware file: %s\n", CDN_DP_FIRMWARE);
		return;
	}

	if(NULL != context) {
		struct cdn_dp_device *dp = dev_get_drvdata((struct device *)context);

		dp->fw = cdns_fw;
		dp->fw_loaded = true;

		printk(KERN_INFO "success to open cdns dp firmware file: %s %d\n", CDN_DP_FIRMWARE, cdns_fw->size);

		if (dp->port[0] && dp->port[0]->extcon &&
		    extcon_get_cable_state_(dp->port[0]->extcon, EXTCON_DISP_DP) && 0 == dp->is_dp_connected) {
			union extcon_property_value property;
			extcon_get_property(dp->port[0]->extcon, EXTCON_DISP_DP, EXTCON_PROP_USB_SS, &property);
			if(property.intval) {
				schedule_work(&dp->event_work);
			}
		}
	}
}

static int cdn_dp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *match;
	struct cdn_dp_data *dp_data;
	struct cdn_dp_port *port;
	struct cdn_dp_device *dp;
	struct extcon_dev *extcon;
	struct phy *phy;
	int ret, i;

	printk(KERN_ERR "cdn_dp_probe\n");

	dp = devm_kzalloc(dev, sizeof(*dp), GFP_KERNEL);
	if (!dp)
		return -ENOMEM;
	dp->dev = dev;

	/*while(dev)
	{
	    printk(KERN_INFO "dp =======================>dev:%s %s %s %s %p\n",
	        dev->init_name, dev->class ? dev->class->name : NULL,
	        dev->bus ? dev->bus->name : NULL,
	        dev->driver ? dev->driver->name : NULL,
	        dev->driver_data ? dev->driver_data : NULL);
	    dev = dev->parent;
	}*/

	if(NULL != cdn_dp_dt_ids) {
		if(NULL != pdev->dev.of_node) {
			match = of_match_node(cdn_dp_dt_ids, pdev->dev.of_node);
			dp_data = (struct cdn_dp_data *)match->data;

			printk(KERN_ERR "get cadence dp data max_phy:%d!\n", dp_data->max_phy);

			for (i = 0; i < dp_data->max_phy; i++) {
				extcon = extcon_get_edev_by_phandle(dev, i);
				phy = devm_of_phy_get_by_index(dev, dev->of_node, i);

				printk(KERN_ERR "extcon: %p phy: %p\n", extcon, phy);

				if (PTR_ERR(extcon) == -EPROBE_DEFER ||
				    PTR_ERR(phy) == -EPROBE_DEFER)
					return -EPROBE_DEFER;

				if (IS_ERR(extcon) || IS_ERR(phy))
					continue;

				port = devm_kzalloc(dev, sizeof(*port), GFP_KERNEL);
				if (!port)
					return -ENOMEM;

				port->extcon = extcon;
				port->phy = phy;
				port->dp = dp;
				port->id = i;
				dp->port[dp->ports++] = port;

				port->event_nb.notifier_call = cdn_dp_pd_event;
				ret = extcon_register_notifier(port->extcon,
				        EXTCON_DISP_DP,
				        &port->event_nb);
				if (ret) {
					printk(KERN_ERR "register EXTCON_DISP_DP notifier err\n");
				}
			}
		} else {
			printk(KERN_ERR "pdev->dev.of_node NULL\n");
		}
	} else {
		printk(KERN_ERR "cdn_dp_dt_ids NULL\n");
	}

	if (!dp->ports) {
		printk(KERN_ERR "missing extcon or phy\n");
		return -EINVAL;
	}

	dp->dp_icc_client = sc_icc_client_create();
	dp->dp_icc_msg_id = DP_ICC_MESSAGE_ID;
	if(0 != sc_icc_client_register_msgid(dp->dp_icc_client, dp->dp_icc_msg_id)) {
		printk(KERN_ERR "=====================================>icc register failed!\n");
	}

	if(NULL != dev->parent)
		dp->tcpc = (struct smartx_tcpc *)dev_get_drvdata(dev->parent);

	mutex_init(&dp->lock);
	dev_set_drvdata(dev, dp);

	ret = cdn_dp_parse_dt(dp);
	if (ret < 0)
		return ret;

	dp->connected = false;
	dp->active = false;
	dp->active_port = -1;
	dp->fw_loaded = false;

	INIT_WORK(&dp->event_work, cdn_dp_pd_event_work);

	ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
	        CDN_DP_FIRMWARE, dev, GFP_KERNEL, dev, load_dp_fw);
	if(ret < 0) {
		printk(KERN_ERR "failed to open cdns firmware file: %s (%d)", CDN_DP_FIRMWARE, ret);
		return ret;
	}

	return 0;

	//err:
	return ret;
}

static int cdn_dp_remove(struct platform_device *pdev)
{
	/*struct cdn_dp_device *dp = platform_get_drvdata(pdev);

	if (dp->fw_loaded)
	    release_firmware(dp->fw);

	cdn_dp_suspend(dp->dev);
	component_del(&pdev->dev, &cdn_dp_component_ops);
	device_remove_file(&pdev->dev, &dev_attr_hdcp_key);*/

	return 0;
}

static void cdn_dp_shutdown(struct platform_device *pdev)
{
	//struct cdn_dp_device *dp = platform_get_drvdata(pdev);

	//cdn_dp_suspend(dp->dev);
}

static const struct dev_pm_ops cdn_dp_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cdn_dp_suspend,
	    cdn_dp_resume)
};

static struct platform_driver cdn_dp_driver = {
	.probe = cdn_dp_probe,
	.remove = cdn_dp_remove,
	.shutdown = cdn_dp_shutdown,
	.driver = {
		.name = "cadence_dp",
		.of_match_table = cdn_dp_dt_ids,//of_match_ptr(cdn_dp_dt_ids),
		.pm = &cdn_dp_pm_ops,
	},
};

module_platform_driver(cdn_dp_driver);

MODULE_ALIAS("platform:cadence_dp");
MODULE_AUTHOR("SmartChip Electronics Inc.");
MODULE_DESCRIPTION("Candence USB DP phy driver");
MODULE_LICENSE("GPL v2");

