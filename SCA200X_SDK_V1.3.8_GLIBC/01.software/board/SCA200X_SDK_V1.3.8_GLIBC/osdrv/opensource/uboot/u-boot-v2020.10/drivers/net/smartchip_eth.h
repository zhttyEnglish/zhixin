#ifndef __SMARTX_ETH_H__
#define __SMARTX_ETH_H__

#define PHY_DP83847_ID_W0         0x00002000
#define PHY_DP83847_ID_W1         0x00005c30
#define PHY_88E1111_ID_W0         0x00000141
#define PHY_88E1111_ID_W1         0x00000cc2
#define PHY_DP83865_ID_W0         0x00002000
#define PHY_DP83865_ID_W1         0x00005c7a

#define SMARTX_ETH_REG_BASE  SMARTCHIP_GMAC_BASE
#define SMARTX_ETH_BD_BASE   (SMARTCHIP_GMAC_BASE + 0x400)
#define SMARTX_ETH_TXBD_OFFSET  0x400
#define SMARTX_ETH_TOTAL_BD  128
//#define SMARTX_ETH_MAXBUF_LEN 0x600

#define SMARTX_ETH_TXBD_NUM      64
#define SMARTX_ETH_TXBD_NUM_MASK (SMARTX_ETH_TXBD_NUM - 1)
#define SMARTX_ETH_RXBD_NUM      64
#define SMARTX_ETH_RXBD_NUM_MASK (SMARTX_ETH_RXBD_NUM - 1)
#define SMARTX_ETH_RXBD_OFFSET   (SMARTX_ETH_TXBD_OFFSET + SMARTX_ETH_TXBD_NUM * 8)
#define CONFIG_MDIO_TIMEOUT (3*CONFIG_SYS_HZ)

/* Tx BD */
#define ETH_TX_BD_READY    0x8000 /* Tx BD Ready */
#define ETH_TX_BD_IRQ      0x4000 /* Tx BD IRQ Enable */
#define ETH_TX_BD_WRAP     0x2000 /* Tx BD Wrap (last BD) */
#define ETH_TX_BD_PAD      0x1000 /* Tx BD Pad Enable */
#define ETH_TX_BD_CRC      0x0800 /* Tx BD CRC Enable */

#define ETH_TX_BD_BUSERR   0x0200 /* Tx BD Bus Error */
#define ETH_TX_BD_UNDERRUN 0x0100 /* Tx BD Underrun Status */
#define ETH_TX_BD_RETRY    0x00F0 /* Tx BD Retry Status */
#define ETH_TX_BD_RETLIM   0x0008 /* Tx BD Retransmission Limit Status */
#define ETH_TX_BD_LATECOL  0x0004 /* Tx BD Late Collision Status */
#define ETH_TX_BD_DEFER    0x0002 /* Tx BD Defer Status */
#define ETH_TX_BD_CARRIER  0x0001 /* Tx BD Carrier Sense Lost Status */
#define ETH_TX_BD_STATS        (ETH_TX_BD_BUSERR               | \
                                ETH_TX_BD_UNDERRUN             | \
                                ETH_TX_BD_RETRY                | \
                                ETH_TX_BD_RETLIM               | \
                                ETH_TX_BD_LATECOL              | \
                                ETH_TX_BD_DEFER                | \
                                ETH_TX_BD_CARRIER)

/* Rx BD */
#define ETH_RX_BD_EMPTY    0x8000 /* Rx BD Empty */
#define ETH_RX_BD_IRQ      0x4000 /* Rx BD IRQ Enable */
#define ETH_RX_BD_WRAP     0x2000 /* Rx BD Wrap (last BD) */

#define ETH_RX_BD_BUSERR   0x0200 /* Rx BD Bus Error */
#define ETH_RX_BD_MISS     0x0080 /* Rx BD Miss Status */
#define ETH_RX_BD_OVERRUN  0x0040 /* Rx BD Overrun Status */
#define ETH_RX_BD_INVSIMB  0x0020 /* Rx BD Invalid Symbol Status */
#define ETH_RX_BD_DRIBBLE  0x0010 /* Rx BD Dribble Nibble Status */
#define ETH_RX_BD_TOOLONG  0x0008 /* Rx BD Too Long Status */
#define ETH_RX_BD_SHORT    0x0004 /* Rx BD Too Short Frame Status */
#define ETH_RX_BD_CRCERR   0x0002 /* Rx BD CRC Error Status */
#define ETH_RX_BD_LATECOL  0x0001 /* Rx BD Late Collision Status */
#define ETH_RX_BD_STATS        (ETH_RX_BD_BUSERR               | \
                                ETH_RX_BD_MISS                 | \
                                ETH_RX_BD_OVERRUN              | \
                                ETH_RX_BD_INVSIMB              | \
                                ETH_RX_BD_DRIBBLE              | \
                                ETH_RX_BD_TOOLONG              | \
                                ETH_RX_BD_SHORT                | \
                                ETH_RX_BD_CRCERR               | \
                                ETH_RX_BD_LATECOL)

/* Register space */
#define ETH_MODER      0x00 /* Mode Register */
#define ETH_INT        0x04 /* Interrupt Source Register */
#define ETH_INT_MASK   0x08 /* Interrupt Mask Register */
#define ETH_IPGT       0x0C /* Back to Bak Inter Packet Gap Register */
#define ETH_IPGR1      0x10 /* Non Back to Back Inter Packet Gap Register 1 */
#define ETH_IPGR2      0x14 /* Non Back to Back Inter Packet Gap Register 2 */
#define ETH_PACKETLEN  0x18 /* Packet Length Register (min. and max.) */
#define ETH_COLLCONF   0x1C /* Collision and Retry Configuration Register */
#define ETH_TX_BD_NUM_REG  0x20 /* Transmit Buffer Descriptor Number Register */
#define ETH_CTRLMODER  0x24 /* Control Module Mode Register */
#define ETH_MIIMODER   0x28 /* MII Mode Register */
#define ETH_MIICOMMAND 0x2C /* MII Command Register */
#define ETH_MIIADDRESS 0x30 /* MII Address Register */
#define ETH_MIITX_DATA 0x34 /* MII Transmit Data Register */
#define ETH_MIIRX_DATA 0x38 /* MII Receive Data Register */
#define ETH_MIISTATUS  0x3C /* MII Status Register */
#define ETH_MAC_ADDR0  0x40 /* MAC Individual Address Register 0 */
#define ETH_MAC_ADDR1  0x44 /* MAC Individual Address Register 1 */
#define ETH_HASH_ADDR0 0x48 /* Hash Register 0 */
#define ETH_HASH_ADDR1 0x4C /* Hash Register 1 */
#define ETH_SPEED_SEL_MODE 0x54 /* Speed Select Mode Register */
#define ETH_CURR_TXBD  0x5C
#define ETH_CURR_RXBD  0x60
#define ETH_PWR_CTRL_REG        0x1000
#define ETH_RSTN_CTRL_REG       0x1004
#define ETH_ENDIAN_CFG_REG      0x1008
#define ETH_TX_CTRL_REG         0x100c
#define ETH_RX_CTRL_REG         0x1010
#define ETH_RMII_CTRL           0x101c

#define GMAC_PWR_CTRL_REG         (SMARTX_ETH_REG_BASE + ETH_PWR_CTRL_REG)
#define GBE_RSTN_CTRL_REG         (SMARTX_ETH_REG_BASE + ETH_RSTN_CTRL_REG)
#define GBE_TX_CTRL_REG           (SMARTX_ETH_REG_BASE + ETH_TX_CTRL_REG)
#define GBE_RX_CTRL_REG           (SMARTX_ETH_REG_BASE + ETH_RX_CTRL_REG)

/* ETH_RMII_CTRL Register */
#define ETH_RMII_SEL   BIT(6) //bit 6 is rmii/rgmii selection

#define ETH_MII_PHY_ADDR_SHIFT  0
#define ETH_MII_REG_ADDR_SHIFT  8

/* MODER Register */
#define ETH_MODER_RXEN     0x00000001 /* Receive Enable  */
#define ETH_MODER_TXEN     0x00000002 /* Transmit Enable */
#define ETH_MODER_NOPRE    0x00000004 /* No Preamble  */
#define ETH_MODER_BRO      0x00000008 /* Reject Broadcast */
#define ETH_MODER_IAM      0x00000010 /* Use Individual Hash */
#define ETH_MODER_PRO      0x00000020 /* Promiscuous (receive all) */
#define ETH_MODER_IFG      0x00000040 /* Min. IFG not required */
#define ETH_MODER_LOOPBCK  0x00000080 /* Loop Back */
#define ETH_MODER_NOBCKOF  0x00000100 /* No Backoff */
#define ETH_MODER_EXDFREN  0x00000200 /* Excess Defer */
#define ETH_MODER_FULLD    0x00000400 /* Full Duplex */
#define ETH_MODER_RST      0x00000800 /* Reset MAC */
#define ETH_MODER_DLYCRCEN 0x00001000 /* Delayed CRC Enable */
#define ETH_MODER_CRCEN    0x00002000 /* CRC Enable */
#define ETH_MODER_HUGEN    0x00004000 /* Huge Enable */
#define ETH_MODER_PAD      0x00008000 /* Pad Enable */
#define ETH_MODER_RECSMALL 0x00010000 /* Receive Small */
#define ETH_MODER_FIFO_FLAG 0x00060000

/* Interrupt Source Register */
#define ETH_INT_TXB        0x00000001 /* Transmit Buffer IRQ */
#define ETH_INT_TXE        0x00000002 /* Transmit Error IRQ */
#define ETH_INT_RXF        0x00000004 /* Receive Frame IRQ */
#define ETH_INT_RXE        0x00000008 /* Receive Error IRQ */
#define ETH_INT_BUSY       0x00000010 /* Busy IRQ */
#define ETH_INT_TXC        0x00000020 /* Transmit Control Frame IRQ */
#define ETH_INT_RXC        0x00000040 /* Received Control Frame IRQ */

/* Interrupt Mask Register */
#define ETH_INT_MASK_TXB   0x00000001 /* Transmit Buffer IRQ Mask */
#define ETH_INT_MASK_TXE   0x00000002 /* Transmit Error IRQ Mask */
#define ETH_INT_MASK_RXF   0x00000004 /* Receive Frame IRQ Mask */
#define ETH_INT_MASK_RXE   0x00000008 /* Receive Error IRQ Mask */
#define ETH_INT_MASK_BUSY  0x00000010 /* Busy IRQ Mask */
#define ETH_INT_MASK_TXC   0x00000020 /* Transmit Control Frame IRQ Mask */
#define ETH_INT_MASK_RXC   0x00000040 /* Received Control Frame IRQ Mask */

/* Control Module Mode Register */
#define ETH_CTRLMODER_PASSALL 0x00000001 /* Pass Control Frames */
#define ETH_CTRLMODER_RXFLOW  0x00000002 /* Receive Control Flow Enable */
#define ETH_CTRLMODER_TXFLOW  0x00000004 /* Transmit Control Flow Enable */

/* MII Mode Register */
#define ETH_MIIMODER_CLKDIV   0x0000FFFF /* Clock Divider */
#define ETH_MIIMODER_NOPRE    0x00010000 /* No Preamble */

/* MII Command Register */
#define ETH_MIICOMMAND_SCANSTAT  0x00000001 /* Scan Status */
#define ETH_MIICOMMAND_RSTAT     0x00000002 /* Read Status */
#define ETH_MIICOMMAND_WCTRLDATA 0x00000004 /* Write Control Data */

/* MII Address Register */
#define ETH_MIIADDRESS_FIAD 0x0000001F /* PHY Address */
#define ETH_MIIADDRESS_RGAD 0x00001F00 /* RGAD Address */

/* MII Status Register */
#define ETH_MIISTATUS_LINKFAIL 0x00000001 /* Link Fail */
#define ETH_MIISTATUS_BUSY     0x00000002 /* MII Busy */
#define ETH_MIISTATUS_NVALID   0x00000004 /* Data in MII Status Register is invalid */

#define SMARTX_ETH_DEV_NAME "smartx-gmac"
/* Ethernet buffer descriptor */
struct smartx_eth_bd {
	volatile unsigned int   len_status;     /* Buffer length and status */
	volatile unsigned int   addr;          /* Buffer address */
};

struct smartx_eth_dev {
	//struct smartx_eth_bd tx_mac_descrtable[SMARTX_ETH_TXBD_NUM];
	//struct smartx_eth_bd rx_mac_descrtable[SMARTX_ETH_RXBD_NUM];
	//u32 tx_currdescnum;
	//u32 rx_currdescnum;

	//the bd should be pointed to bd inside the GMAC, which is base_addr+0x400.
	struct smartx_eth_bd *tx_mac_descrtable;
	struct smartx_eth_bd *rx_mac_descrtable;

	u32 interface;
	u32 tx_head;
	u32 tx_tail;
	u32 tx_full;
	u32 rx_tail;

	void *mac_base_addr;

#ifndef CONFIG_DM_ETH
	struct eth_device *dev;
#endif
	struct phy_device *phydev;
	struct mii_dev *bus;
};
extern int smartx_eth_initialize(ulong base_addr, unsigned int interface, unsigned char *enetaddr);
#endif
