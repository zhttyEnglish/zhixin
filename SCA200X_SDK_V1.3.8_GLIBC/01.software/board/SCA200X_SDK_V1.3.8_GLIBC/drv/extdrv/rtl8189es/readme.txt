1. 配置 bl-8189ns1

	Makefile 29 行开始是配置项.

	配置phy芯片为 rtl8189es:
		CONFIG_RTL8188E=y
	配置接口为 SDIO:
		CONFIG_SDIO_HCI=y

2. 编译

    修改 mk.sh SDK 路径
	mk.sh
