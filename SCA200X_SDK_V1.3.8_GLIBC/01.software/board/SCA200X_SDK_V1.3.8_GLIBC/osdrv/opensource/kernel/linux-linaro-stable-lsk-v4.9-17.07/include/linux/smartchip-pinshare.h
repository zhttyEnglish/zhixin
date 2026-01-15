#ifndef __SMARTCHIP_PINSHARE_H__
#define __SMARTCHIP_PINSHARE_H__

#define PIN_SHARE_FUNC_0      0
#define PIN_SHARE_FUNC_1      1
#define PIN_SHARE_FUNC_2      2
#define PIN_SHARE_FUNC_3      3
#define PIN_SHARE_FUNC_4      4
#define PIN_SHARE_FUNC_5      5

#define PIN_NO(x)           (x)

/* Group: from pin 40 to 43*/
#define PIN_UART_0_TX 41   //uart0,secure uart share
#define PIN_UART_0_RX 40   //uart0,secure uart share
#define PIN_UART_1_TX 43   //uart0,secure uart share
#define PIN_UART_1_RX 42   //uart0,secure uart share
#define PIN_FUNC_SEC_UART 0
#define PIN_FUNC_UART0    2

/* Group: from pin 76 to 78 */
#define PIN_FUNC_SD_CCLK_OUT       PIN_SHARE_FUNC_1
#define PIN_FUNC_SD_CCMD           PIN_SHARE_FUNC_1
#define PIN_FUNC_SD_CARD_DETECT_N  PIN_SHARE_FUNC_1
#define PIN_FUNC_SD_CARD_WPRT      PIN_SHARE_FUNC_1
#define PIN_FUNC_SD_CDATA_0        PIN_SHARE_FUNC_1
#define PIN_FUNC_SD_CDATA_1        PIN_SHARE_FUNC_1
#define PIN_FUNC_SD_CDATA_2        PIN_SHARE_FUNC_1
#define PIN_FUNC_SD_CDATA_3        PIN_SHARE_FUNC_1

#define PIN_FUNC_QSPI_SCK          PIN_SHARE_FUNC_0
#define PIN_FUNC_QSPI_CS           PIN_SHARE_FUNC_0
#define PIN_FUNC_QSPI_DATA_0       PIN_SHARE_FUNC_0
#define PIN_FUNC_QSPI_DATA_1       PIN_SHARE_FUNC_0
#define PIN_FUNC_QSPI_DATA_2       PIN_SHARE_FUNC_0
#define PIN_FUNC_QSPI_DATA_3       PIN_SHARE_FUNC_0

/* Group: 120 */
#define PIN_FUNC_USB_PWR_CTRL      PIN_SHARE_FUNC_1

/* Group: from 121 to 137 */
#define PIN_FUNC_GBE_TXC            PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_TXEN           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_TXD0           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_TXD1           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_TXD2           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_TXD3           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_RXC            PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_RXEN           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_RXD0           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_RXD1           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_RXD2           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_RXD3           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_MDC            PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_MDIO           PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_INT            PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_CLK            PIN_SHARE_FUNC_0
#define PIN_FUNC_GBE_RST            PIN_SHARE_FUNC_0

/* GPIO groups and ports */
#define GPIO_GROUP_0                    0
#define GPIO_GROUP_1                    1
#define GPIO_GROUP_2                    2
#define GPIO_GROUP_3                    3
#define GPIO_GROUP_4                    4

#define GPIO_PORT_A                     0
#define GPIO_PORT_B                     1
#define GPIO_PORT_C                     2
#define GPIO_PORT_D                     3

#define GPIO_PORT_A_DATA_OFFSET         0x0
#define GPIO_PORT_A_DIRECTION_OFFSET    0x4
#define GPIO_PORT_A_CTRL_OFFSET         0x8
#define GPIO_PORT_B_DATA_OFFSET         0xc
#define GPIO_PORT_B_DIRECTION_OFFSET    0x10
#define GPIO_PORT_B_CTRL_OFFSET         0x14
#define GPIO_PORT_C_DATA_OFFSET         0x18
#define GPIO_PORT_C_DIRECTION_OFFSET    0x1c
#define GPIO_PORT_C_CTRL_OFFSET         0x20
#define GPIO_PORT_D_DATA_OFFSET         0x24
#define GPIO_PORT_D_DIRECTION_OFFSET    0x28
#define GPIO_PORT_D_CTRL_OFFSET         0x2c

/*These interrupt registers to be defined later..*/
#define GPIO_INTR_ENABLE_OFFSET         0x30
#define GPIO_INTR_MASK_OFFSET           0x34
#define GPIO_INTR_TYPELEVEL_OFFSET      0x38
#define GPIO_INTR_POLARITY_OFFSET       0x3c
#define GPIO_INTR_STATUS_OFFSET         0x40
#define GPIO_INTR_RAWSTATUS_OFFSET      0x44
#define GPIO_INTR_EOI_OFFSET            0x4c

#define GPIO_IS_HIGH                    1
#define GPIO_IS_LOW                 0
#define GPIO_IS_OUTPUT                  1
#define GPIO_IS_INPUT                   0

void smartchip_set_pinshare(int pin, int func);
int smartchip_get_pinshare(int pin);

#endif
