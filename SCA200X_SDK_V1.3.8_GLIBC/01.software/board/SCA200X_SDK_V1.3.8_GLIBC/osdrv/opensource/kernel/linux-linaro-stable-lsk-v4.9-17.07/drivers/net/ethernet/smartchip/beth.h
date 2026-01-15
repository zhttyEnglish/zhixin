
/* Registers (Stolen from basim/peripheral/ethernet.h, Copyright Erez Vlok) */
#define OETH_MODER  (4 * 0x00)
#define OETH_INT_SOURCE (4 * 0x01)
#define OETH_INT_MASK   (4 * 0x02)
#define OETH_IPGT   (4 * 0x03)
#define OETH_IPGR1  (4 * 0x04)
#define OETH_IPGR2  (4 * 0x05)
#define OETH_PACKETLEN  (4 * 0x06)
#define OETH_COLLCONF   (4 * 0x07)
#define OETH_TX_BD_NUM  (4 * 0x08)
#define OETH_CTRLMODER  (4 * 0x09)
#define OETH_MIIMODER   (4 * 0x0A)
#define OETH_MIICOMMAND (4 * 0x0B)
#define OETH_MIIADDRESS (4 * 0x0C)
#define OETH_MIITX_DATA (4 * 0x0D)
#define OETH_MIIRX_DATA (4 * 0x0E)
#define OETH_MIISTATUS  (4 * 0x0F)
#define OETH_MAC_ADDR0  (4 * 0x10)
#define OETH_MAC_ADDR1  (4 * 0x11)
#define OETH_HASH0  (4 * 0x12)
#define OETH_HASH1  (4 * 0x13)
#define OETH_SPEED_SEL  (4 * 0x15)
#define OETH_INT_MASK2  (4 * 0x16)
#define OETH_INT_MASK3  (4 * 0x20)

#define OETH_CNT_BASE   0x200
#define OETH_BD_BASE    0x400
#define OETH_TOTAL_BD           128
#define OETH_MAX_CNT        64
#define OETH_MAXBUF_LEN         0x600
#define OETH_ADDR_SPACE     0x800

/* Counter offsets */
#define OETH_CNT_TXOK    0
#define OETH_CNT_TXCOLL  1
#define OETH_CNT_TXLATE  2 /* TX late collision */
#define OETH_CNT_RETLIM  3
#define OETH_CNT_TXCSL   4 /* Carrier sense lost */
#define OETH_CNT_TXDI    5 /* TX defer indicator */
#define OETH_CNT_TXDT    6 /* TX defer timeout */
#define OETH_CNT_TXBE    7 /* TX Bus error */
#define OETH_CNT_UNDER   8 /* TX FIFO underrun */
#define OETH_CNT_RXOK    9
#define OETH_CNT_RXLATE 10 /* RX late collision */
#define OETH_CNT_RXCRC  11 /* RX crc errors */
#define OETH_CNT_RXDN   12 /* RX dribble nible */
#define OETH_CNT_RXTL   13 /* Frame too long to RX */
#define OETH_CNT_RXTS   14 /* Frame too short to RX */
#define OETH_CNT_INVSYM 15 /* RX invalid symbol */
#define OETH_CNT_RXBE   16 /* RX bus error */
#define OETH_CNT_OVER   17 /* RX FIFO overrun */
#define OETH_CNT_RXMISS 18 /* RX lost (missed) packets */
#define OETH_CNT_TXB    19 /* TXd bytes */
#define OETH_CNT_RXB    20 /* RXd bytes */
#define OETH_CNT_MCAST  21 /* RXd Multicast packets */
#define OETH_CNT_TXBAD  22 /* Packets that couldn't be TXd */
#define OETH_CNT_RXBAD  23 /* Packets that had errors during RX */

#define OETH_CNT_NUM    24 /* The number of status counters.  When changing
                            * this, update the appropriate strings table!! */

/* Tx & Rx BD */
#define OETH_BD_LEN     0xffff0000 /* Length of data at addr reg */

/* Tx BD */
#define OETH_TX_BD_READY        0x8000 /* Tx BD Ready */
#define OETH_TX_BD_IRQ          0x4000 /* Tx BD IRQ Enable */
#define OETH_TX_BD_WRAP         0x2000 /* Tx BD Wrap (last BD) */
#define OETH_TX_BD_PAD          0x1000 /* Tx BD Pad Enable */
#define OETH_TX_BD_CRC          0x0800 /* Tx BD CRC Enable */

#define OETH_TX_BD_DT       0x0400 /* Tx defer timeout */
#define OETH_TX_BD_BUS_ERR  0x0200 /* Tx Bus error */
#define OETH_TX_BD_UNDERRUN     0x0100 /* Tx BD Underrun Status */
#define OETH_TX_BD_RETRY        0x00F0 /* Tx BD Retry Status */
#define OETH_TX_BD_RETLIM       0x0008 /* Tx BD Retransmission Limit Status */
#define OETH_TX_BD_LATECOL      0x0004 /* Tx BD Late Collision Status */
#define OETH_TX_BD_DEFER        0x0002 /* Tx BD Defer Status */
#define OETH_TX_BD_CARRIER      0x0001 /* Tx BD Carrier Sense Lost Status */
#define OETH_TX_BD_STATS        (OETH_TX_BD_UNDERRUN            | \
                                OETH_TX_BD_RETRY                | \
                                OETH_TX_BD_RETLIM               | \
                                OETH_TX_BD_LATECOL              | \
                                OETH_TX_BD_DEFER                | \
                                OETH_TX_BD_CARRIER)

/* Rx BD */
#define OETH_RX_BD_EMPTY        0x8000 /* Rx BD Empty */
#define OETH_RX_BD_IRQ          0x4000 /* Rx BD IRQ Enable */
#define OETH_RX_BD_WRAP         0x2000 /* Rx BD Wrap (last BD) */

#define OETH_RX_BD_MCAST    0x0400 /* Multicast frame */
#define OETH_RX_BD_BUS_ERR  0x0200 /* Rx bus error */
#define OETH_RX_BD_CTRL_F   0x0100 /* Rx controll frame */
#define OETH_RX_BD_MISS         0x0080 /* Rx BD Miss Status */
#define OETH_RX_BD_OVERRUN      0x0040 /* Rx BD Overrun Status */
#define OETH_RX_BD_INVSIMB      0x0020 /* Rx BD Invalid Symbol Status */
#define OETH_RX_BD_DRIBBLE      0x0010 /* Rx BD Dribble Nibble Status */
#define OETH_RX_BD_TOOLONG      0x0008 /* Rx BD Too Long Status */
#define OETH_RX_BD_SHORT        0x0004 /* Rx BD Too Short Frame Status */
#define OETH_RX_BD_CRCERR       0x0002 /* Rx BD CRC Error Status */
#define OETH_RX_BD_LATECOL      0x0001 /* Rx BD Late Collision Status */
#define OETH_RX_BD_STATS        (OETH_RX_BD_MISS                | \
                                OETH_RX_BD_MCAST                | \
                                OETH_RX_BD_OVERRUN              | \
                                OETH_RX_BD_INVSIMB              | \
                                OETH_RX_BD_DRIBBLE              | \
                                OETH_RX_BD_TOOLONG              | \
                                OETH_RX_BD_SHORT                | \
                                OETH_RX_BD_CRCERR               | \
                                OETH_RX_BD_LATECOL)

/* MODER Register */
#define OETH_MODER_RXEN         0x00000001 /* Receive Enable  */
#define OETH_MODER_TXEN         0x00000002 /* Transmit Enable */
#define OETH_MODER_NOPRE        0x00000004 /* No Preamble  */
#define OETH_MODER_BRO          0x00000008 /* Reject Broadcast */
#define OETH_MODER_IAM          0x00000010 /* Use Individual Hash */
#define OETH_MODER_PRO          0x00000020 /* Promiscuous (receive all) */
#define OETH_MODER_IFG          0x00000040 /* Min. IFG not required */
#define OETH_MODER_LOOPBCK      0x00000080 /* Loop Back */
#define OETH_MODER_NOBCKOF      0x00000100 /* No Backoff */
#define OETH_MODER_EXDFREN      0x00000200 /* Excess Defer */
#define OETH_MODER_FULLD        0x00000400 /* Full Duplex */
#define OETH_MODER_RST          0x00000800 /* Reset MAC */
#define OETH_MODER_DLYCRCEN     0x00001000 /* Delayed CRC Enable */
#define OETH_MODER_CRCEN        0x00002000 /* CRC Enable */
#define OETH_MODER_HUGEN        0x00004000 /* Huge Enable */
#define OETH_MODER_PAD          0x00008000 /* Pad Enable */
#define OETH_MODER_RECSMALL     0x00010000 /* Receive Small */
#define OETH_MODER_FIFO     0x00060000 /* Tx FIFO fill level */
#define OETH_MODER_FIFOQ    0x00000000 /* 1/4 full or whole packet */
#define OETH_MODER_FIFOH    0x00020000 /* 1/2 full or whole packet */
#define OETH_MODER_FIFO3Q   0x00040000 /* 3/4 full or whole packet */
#define OETH_MODER_FIFOF    0x00060000 /* FIFO full or whole packet */

/* Interrupt Source Register */
#define OETH_INT_TXB            0x00000001 /* Transmit Buffer IRQ */
#define OETH_INT_TXE            0x00000002 /* Transmit Error IRQ */
#define OETH_INT_RXF            0x00000004 /* Receive Frame IRQ */
#define OETH_INT_RXE            0x00000008 /* Receive Error IRQ */
#define OETH_INT_BUSY           0x00000010 /* Busy IRQ */
#define OETH_INT_TXC            0x00000020 /* Transmit Control Frame IRQ */
#define OETH_INT_RXC            0x00000040 /* Received Control Frame IRQ */
#define OETH_INT_TX     (OETH_INT_TXB | OETH_INT_TXE)
#define OETH_INT_RX     (OETH_INT_RXF | OETH_INT_RXE)

/* Interrupt Mask Register */
#define OETH_INT_MASK_TXB       0x00000001 /* Transmit Buffer IRQ Mask */
#define OETH_INT_MASK_TXE       0x00000002 /* Transmit Error IRQ Mask */
#define OETH_INT_MASK_RXF       0x00000004 /* Receive Frame IRQ Mask */
#define OETH_INT_MASK_RXE       0x00000008 /* Receive Error IRQ Mask */
#define OETH_INT_MASK_BUSY      0x00000010 /* Busy IRQ Mask */
#define OETH_INT_MASK_TXC       0x00000020 /* Transmit Control Frame IRQ Mask */
#define OETH_INT_MASK_RXC       0x00000040 /* Received Control Frame IRQ Mask */

#define OETH_INT_MASK_ALL   (OETH_INT_MASK_TXB | OETH_INT_MASK_TXE | \
                 OETH_INT_MASK_RXF | OETH_INT_MASK_RXE | \
                 OETH_INT_MASK_BUSY | OETH_INT_MASK_TXC | \
                 OETH_INT_MASK_RXC)
#define OETH_INT_MASK_RX    (OETH_INT_MASK_RXE | OETH_INT_MASK_RXF)
#define OETH_INT_MASK_TX    (OETH_INT_MASK_TXE | OETH_INT_MASK_TXB)

#define OETH_INT_MASK3_PHY  0x00000001

/* Control Module Mode Register */
#define OETH_CTRLMODER_PASSALL  0x00000001 /* Pass Control Frames */
#define OETH_CTRLMODER_RXFLOW   0x00000002 /* Receive Control Flow Enable */
#define OETH_CTRLMODER_TXFLOW   0x00000004 /* Transmit Control Flow Enable */

/* MII Mode Register */
#define OETH_MIIMODER_CLKDIV    0x000000FF /* Clock Divider */
#define OETH_MIIMODER_NOPRE     0x00000100 /* No Preamble */
#define OETH_MIIMODER_RST       0x00000200 /* MIIM Reset */

/* MII Command Register */
#define OETH_MIICOMMAND_SCANSTAT  0x00000001 /* Scan Status */
#define OETH_MIICOMMAND_RSTAT     0x00000002 /* Read Status */
#define OETH_MIICOMMAND_WCTRLDATA 0x00000004 /* Write Control Data */

/* MII Address Register */
#define OETH_MIIADDRESS_FIAD    0x0000001F /* PHY Address */
#define OETH_MIIADDRESS_RGAD    0x00001F00 /* RGAD Address */

/* MII Status Register */
#define OETH_MIISTATUS_LINKFAIL 0x00000001 /* Link Fail */
#define OETH_MIISTATUS_BUSY     0x00000002 /* MII Busy */
#define OETH_MIISTATUS_INVALID   0x00000004 /* Data in MII Status Register is invalid */

/* Speed select register */
#define OETH_SPEED_SEL_1000 0x00000001 /* 1000Mbps selected */
#define OETH_SPEED_SEL_100  0x00000000 /* 100Mbps selected */
#define OETH_SPEED_SEL_10   0x00000002 /* 10Mbps selected */
#define OETH_SPEED_SEL_MASK 0x00000003

/* Interrupt filter things */
#define INTF_ADDR_SPACE 0x20

#define INTF_MODE   0x00
#define INTF_DIVISOR    0x04
#define INTF_LEVEL0 0x08
#define INTF_DELAY0 0x0c
#define INTF_LEVEL1 0x10
#define INTF_DELAY1 0x14

#define INTF_MODE_LEN0  0x1
#define INTF_MODE_DEN0  0x2
#define INTF_MODE_LEN1  0x4
#define INTF_MODE_DEN1  0x8

