
1. 目录
=======
  rtl8188fu/:
      wifi模组. 用于面板机.
      接口: CONFIG_USB_HCI
      phy: CONFIG_RTL8188F

  rtl8189es/:
      wifi模组. 用于支持客户.
      bl-8189ns1.
      接口: CONFIG_SDIO_HCI
      phy: CONFIG_RTL8188E

  rtl8189fs/:
      wifi模组.
      接口: CONFIG_SDIO_HCI
      phy: CONFIG_RTL8188F

2. rtl wlan phy
===============

      RTL8188EE:  802.11bgn PCIe Network Interface Controller
      RTL8188ETV: 802.11bgn USB 2.0 Network Interface Controller
      RTL8188EUS: 802.11bgn USB 2.0 Network Interface Controller
      RTL8188FTV: 802.11bgn USB 2.0 Network Interface Controller

      RTL8189EM:    802.11bgn SDIO Network Interface Controller
      RTL8189EM-VI: 802.11bgn SDIO Network Interface Controller
      RTL8189ES:    802.11bgn SDIO Network Interface Controller
      RTL8189ETV:   802.11bgn SDIO Network Interface Controller
      RTL8189FTV:   802.11bgn SDIO Network Interface Controller

      RTL8192EE: 802.11bgn PCIe Network Interface Controller
      RTL8192ER: 802.11bgn PCIe Network Interface Controller
      RTL8192ES: 802.11bgn SDIO Network Interface Controller
      RTL8192EU: 802.11bgn USB 2.0 Network Interface Controller
      RTL8194AR: 802.11bgn PCIe Network Interface Controller
