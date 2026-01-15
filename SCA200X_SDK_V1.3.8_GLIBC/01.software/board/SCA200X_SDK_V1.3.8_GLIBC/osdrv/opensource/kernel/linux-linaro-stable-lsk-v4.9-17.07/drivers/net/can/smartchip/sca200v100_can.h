/*
 * Core driver for the sca200v100 CAN controllers
 */
#ifndef SCA200V100_CAN_H
#define SCA200V100_CAN_H

#include <linux/can/dev.h>

#define SCA200V100_CAN_DEV_NAME     "sca200v100_can"
#define SCA200V100_CAN_TX_FIFO_NUM  16
#define SCA200V100_CAN_RX_FIFO_NUM  16

#define SCA200V100_CAN_AMASK_ID10_0        (0x7FF)
#define SCA200V100_CAN_UNAMASK_ID10_0      (0x0)
#define SCA200V100_CAN_FRAME_LEN_AMASK     (0xF)

#define SCA200V100_CAN_AMASK_ID28_0        (0x1FFFFFFF)
#define SCA200V100_CAN_UNAMASK_ID28_0      (0x0)

//define register TCTRL
#define SCA200V100_CAN_TCTRL_FDISO         (1<<7)
#define SCA200V100_CAN_TCTRL_TSNEXT        (1<<6)
#define SCA200V100_CAN_TCTRL_TSMODE        (1<<5)
#define SCA200V100_CAN_TCTRL_TTTBM         (1<<4)
#define SCA200V100_CAN_TCTRL_TSSTAT_MASK   (0x03)
#define SCA200V100_CAN_TCTRL_TSSTAT_STB_EMPTY   (0x00)
#define SCA200V100_CAN_TCTRL_TSSTAT_STB_LEHF    (0x01)
#define SCA200V100_CAN_TCTRL_TSSTAT_STB_BGHF    (0x02)
#define SCA200V100_CAN_TCTRL_TSSTAT_STB_FULL    (0x03)

//define register RCTRL
#define SCA200V100_CAN_RCTRL_ROM            (1<<6)
#define SCA200V100_CAN_RCTRL_ROV            (1<<5)
#define SCA200V100_CAN_RCTRL_RREL           (1<<4)
#define SCA200V100_CAN_RCTRL_RSTAT_MASK     (0x03)
#define SCA200V100_CAN_RCTRL_RSTAT_STB_EMPTY    (0x00)
#define SCA200V100_CAN_RCTRL_RSTAT_STB_LEHF     (0x01)
#define SCA200V100_CAN_RCTRL_RSTAT_STB_BGHF     (0x02)
#define SCA200V100_CAN_RCTRL_RSTAT_STB_FULL     (0x03)

//define register TCMD
#define SCA200V100_CAN_TCMD_TBSEL  (1<<7)
#define SCA200V100_CAN_TCMD_LOM    (1<<6)
#define SCA200V100_CAN_TCMD_STBY   (1<<5)
#define SCA200V100_CAN_TCMD_TPE    (1<<4)
#define SCA200V100_CAN_TCMD_TPA    (1<<3)
#define SCA200V100_CAN_TCMD_TSONE  (1<<2)
#define SCA200V100_CAN_TCMD_TSALL  (1<<1)
#define SCA200V100_CAN_TCMD_TSA    (1<<0)

//define register RTIE
#define SCA200V100_CAN_RTIE_RIE    (1<<7)
#define SCA200V100_CAN_RTIE_ROIE   (1<<6)
#define SCA200V100_CAN_RTIE_RFIE   (1<<5)
#define SCA200V100_CAN_RTIE_RAFIE  (1<<4)
#define SCA200V100_CAN_RTIE_TPIE   (1<<3)
#define SCA200V100_CAN_RTIE_TSIE   (1<<2)
#define SCA200V100_CAN_RTIE_EIE    (1<<1)

#define SCA200V100_CAN_RTIE_TSFF   (1<<0)

#define SCA200V100_CAN_RTIE_ALL    (SCA200V100_CAN_RTIE_RIE | \
                                SCA200V100_CAN_RTIE_ROIE | \
                                SCA200V100_CAN_RTIE_RFIE | \
                                SCA200V100_CAN_RTIE_RAFIE | \
                                SCA200V100_CAN_RTIE_TPIE | \
                                SCA200V100_CAN_RTIE_TSIE | \
                                SCA200V100_CAN_RTIE_EIE)

//define register RTIF
#define SCA200V100_CAN_RTIF_RIF    (1<<7)
#define SCA200V100_CAN_RTIF_ROIF   (1<<6)
#define SCA200V100_CAN_RTIF_RFIF   (1<<5)
#define SCA200V100_CAN_RTIF_RAFIF  (1<<4)
#define SCA200V100_CAN_RTIF_TPIF   (1<<3)
#define SCA200V100_CAN_RTIF_TSIF   (1<<2)
#define SCA200V100_CAN_RTIF_EIF    (1<<1)
#define SCA200V100_CAN_RTIF_AIF    (1<<0)

#define SCA200V100_CAN_RTIF_ALL     (SCA200V100_CAN_RTIF_RIF | \
                                SCA200V100_CAN_RTIF_ROIF | \
                                SCA200V100_CAN_RTIF_RFIF | \
                                SCA200V100_CAN_RTIF_RAFIF | \
                                SCA200V100_CAN_RTIF_TPIF | \
                                SCA200V100_CAN_RTIF_TSIF | \
                                SCA200V100_CAN_RTIF_EIF | \
                                SCA200V100_CAN_RTIF_AIF)

//define register ERRINT
#define SCA200V100_CAN_ERRINT_EWARN (1<<7)
#define SCA200V100_CAN_ERRINT_EPASS (1<<6)
#define SCA200V100_CAN_ERRINT_EPIE  (1<<5)
#define SCA200V100_CAN_ERRINT_EPIF  (1<<4)
#define SCA200V100_CAN_ERRINT_ALIE  (1<<3)
#define SCA200V100_CAN_ERRINT_ALIF  (1<<2)
#define SCA200V100_CAN_ERRINT_BEIE  (1<<1)
#define SCA200V100_CAN_ERRINT_BEIF  (1<<0)
#define SCA200V100_CAN_ERRINT_F_ALL (SCA200V100_CAN_ERRINT_EWARN | \
                                SCA200V100_CAN_ERRINT_EPASS | \
                                SCA200V100_CAN_ERRINT_EPIF | \
                                SCA200V100_CAN_ERRINT_ALIF | \
                                SCA200V100_CAN_ERRINT_BEIF)

#define SCA200V100_CAN_REG4_ENABLE  (((SCA200V100_CAN_ERRINT_EPIE | SCA200V100_CAN_ERRINT_ALIE | SCA200V100_CAN_ERRINT_BEIE) << 16) | \
                                SCA200V100_CAN_RTIE_ALL)
#define SCA200V100_CAN_REG4_FLAG    ((SCA200V100_CAN_ERRINT_F_ALL << 16) | (SCA200V100_CAN_RTIF_ALL << 8) | SCA200V100_CAN_RTIE_TSFF)

//define register TCMD
/*#define SCA200V100_CAN_TCMD_TBSEL  (1<<7)
#define SCA200V100_CAN_TCMD_LOM    (1<<6)
#define SCA200V100_CAN_TCMD_STBY   (1<<5)
#define SCA200V100_CAN_TCMD_TPE    (1<<4)
#define SCA200V100_CAN_TCMD_TPA    (1<<3)
#define SCA200V100_CAN_TCMD_TSONE  (1<<2)
#define SCA200V100_CAN_TCMD_TSALL  (1<<1)
#define SCA200V100_CAN_TCMD_TSA    (1<<0)*/

//define register TBUF
#define SCA200V100_CAN_TBUF_IDE    (1<<7)  //std or ext
#define SCA200V100_CAN_TBUF_RTR    (1<<6)  //data or remote
#define SCA200V100_CAN_TBUF_EDL    (1<<5)
#define SCA200V100_CAN_TBUF_BRS    (1<<4)

//define register CFG_STAT
#define SCA200V100_CFG_STAT_RESET     (1<<7) //
#define SCA200V100_CFG_STAT_LBME      (1<<6) //
#define SCA200V100_CFG_STAT_LBMI      (1<<5) //
#define SCA200V100_CFG_STAT_TPSS      (1<<4) //
#define SCA200V100_CFG_STAT_TSSS      (1<<3) //
#define SCA200V100_CFG_STAT_RACTIVE   (1<<2) // Reception ACTIVE (Receive Status bit)
#define SCA200V100_CFG_STAT_TACTIVE   (1<<1) // Transmission ACTIVE (Transmit Status bit)
#define SCA200V100_CFG_STAT_BUSOFF    (1<<0) //

#define SCA200V100_CAN_BTR_S_SEG_1(x)   ((x) & 0x3f)
#define SCA200V100_CAN_BTR_S_SEG_2(x)   (((x) & 0x1f) << 8)
#define SCA200V100_CAN_BTR_S_SJW(x)     (((x) & 0xf) << 16)

//define Acceptance Filter Control Register
#define SCA200V100_CAN_ACF_SELMASK(x)   (((x) & 0x1) << 5)
#define SCA200V100_CAN_ACF_ACFADR(x)    ((x) & 0xf)
#define SCA200V100_CAN_ACF_3_AIDE       (1 << 5)
#define SCA200V100_CAN_ACF_3_AIDEE      (1 << 6)

//define ACF_EN_0
#define SCA200V100_CAN_ACF_ENABLE(x)    (1 << ((x) & 0x07))

//define rxtxbuf control
#define SCA200V100_CAN_TXBUF_CTRL_IDE   (1 << 7)
#define SCA200V100_CAN_TXBUF_CTRL_RTR   (1 << 6)
#define SCA200V100_CAN_TXBUF_CTRL_EDL   (1 << 5)
#define SCA200V100_CAN_TXBUF_CTRL_BRS   (1 << 4)

//define TTCFG
#define SCA200V100_CAN_TTCFG_TTEN       (1 << 0)
#define SCA200V100_CAN_TTCFG_TTIE       (1 << 4)
#define SCA200V100_CAN_TTCFG_WTIE       (1 << 7)

//define id
#define SCA200V100_CAN_ID_ESI           (1 << 31)

//define EALCAP
#define SCA200V100_CAN_EALCAP_KOER_MASK (0xe0)
#define SCA200V100_CAN_EALCAP_KOER(x)   ((x & SCA200V100_CAN_EALCAP_KOER_MASK) >> 5)

#define SCA200V100_CAN_EALCAP_ALC_MASK  (0x1f)

enum sca200v100_can_ealcap {
	KOER_NO_ERROR,
	KOER_BIT_ERROR,
	KOER_FORM_ERROR,
	KOER_STUFF_ERROR,
	KOER_ACK_ERROR,
	KOER_CRC_ERROR,
	KOER_OTHER_ERROR,
	KOER_NOT_USED,
};

/*******************can register define**************************/
#pragma pack(push)
#pragma pack(1)
struct sca200v100_can_rxbuf {
	u32     id;
	u8      ctrl;
	u16     timestamp;
	u8      reserved;
	u8      data[64];
};

struct sca200v100_can_txbuf {
	u32     id;
	u8      ctrl;
	u8      reserved[3];
	u8      data[64];
};

struct u8_sca200v100_can_reg {
	volatile struct sca200v100_can_rxbuf rxBuf;
	volatile struct sca200v100_can_txbuf txBuf;

	volatile uint8_t    u8_CFG_STAT;
	volatile uint8_t    u8_TCMD;
	volatile uint8_t    u8_TCTRL;
	volatile uint8_t    u8_RCTRL;

	volatile uint8_t    u8_RTIE;
	volatile uint8_t    u8_RTIF;
	volatile uint8_t    u8_ERRINT;
	volatile uint8_t    u8_LIMIT;

	volatile uint8_t    u8_BITTIME0;
	volatile uint8_t    u8_BITTIME1;
	volatile uint8_t    u8_BITTIME2;
	volatile uint8_t    u8_BITTIME3;

	volatile uint8_t    u8_S_PRESC;
	volatile uint8_t    u8_F_PRESC;
	volatile uint8_t    u8_TDC;
	volatile uint8_t    u8_resvered0;

	volatile uint8_t    u8_EALCAP;
	volatile uint8_t    u8_resvered1;
	volatile uint8_t    u8_RECNT;
	volatile uint8_t    u8_TECNT;

	volatile uint8_t    u8_ACFCTRL;
	volatile uint8_t    u8_resvered2;
	volatile uint8_t    u8_ACF_EN0;
	volatile uint8_t    u8_ACF_EN1;

	volatile uint8_t    u8_ACF0;
	volatile uint8_t    u8_ACF1;
	volatile uint8_t    u8_ACF2;
	volatile uint8_t    u8_ACF3;

	volatile uint8_t    u8_VER0;
	volatile uint8_t    u8_VER1;
	volatile uint8_t    u8_TBSLOT;
	volatile uint8_t    u8_TTCFG;

	volatile uint8_t    u8_REF_MSG0;
	volatile uint8_t    u8_REF_MSG1;
	volatile uint8_t    u8_REF_MSG2;
	volatile uint8_t    u8_REF_MSG3;

	volatile uint8_t    u8_TRIG_CFG0;
	volatile uint8_t    u8_TRIG_CFG1;
	volatile uint8_t    u8_TT_TRIG0;
	volatile uint8_t    u8_TT_TRIG1;

	volatile uint8_t    u8_TT_WTRIG0;
	volatile uint8_t    u8_TT_WTRIG1;
	volatile uint8_t    u8_resvered3[2];
};

struct u32_sca200v100_can_reg {
	volatile uint32_t    u32_rxBuf[18];          // 0x00-0x47
	volatile uint32_t    u32_txBuf[18];          // 0x48-0x8f
	volatile uint32_t    u32_reg3;               // 0x90 -->  RCTRL    TCTRL    TCMD      CFG_STAT
	volatile uint32_t    u32_reg4;               // 0x94 -->  LIMIT    ERRINT   RTIF      RTIE
	volatile uint32_t    u32_reg5;               // 0x98 -->  BITTIME3 BITTIME2 BITTIME1  BITTIME0
	volatile uint32_t    u32_reg6;               // 0x9c -->  --       TDC      F_PRESC   S_PRESC
	volatile uint32_t    u32_reg7;               // 0xa0 -->  TECNT    RECNT    --        EALCAP
	volatile uint32_t    u32_reg8;               // 0xa4 -->  ACF_EN1  ACF_EN0  --        ACFCTRL
	volatile uint32_t    u32_reg9;               // 0xa8 -->  ACF3     ACF2     ACF1      ACF0
	volatile uint32_t    u32_reg10;              // 0xac -->  TTCFG    TBSLOT   VER1      VER0
	volatile uint32_t    u32_reg11;              // 0xb0 -->  REF_MSG3 REF_MSG2 REF_MSG1  REF_MSG0
	volatile uint32_t    u32_reg12;              // 0xb4 -->  TT_TRIG1 TT_TRIG0 TRIG_CFG1 TRIG_CFG0
	volatile uint32_t    u32_reg13;              // 0xb8 -->  --       --       TT_WTRIG1 TT_WTRIG0
};

union sca200v100_can_reg {
	struct u8_sca200v100_can_reg   u8_if;
	struct u32_sca200v100_can_reg  u32_if;
};
#pragma pack(pop)
struct sca200v100_can_priv {
	struct can_priv                 can;    /* MUST be first member/field */
	struct napi_struct              napi;
	struct net_device               *ndev;
	union sca200v100_can_reg __iomem    *regs;
	struct clk                      *clk;
	struct clk                      *can_clk;
	u8                              tx_dlc[SCA200V100_CAN_TX_FIFO_NUM];
	u32                             tx_head;
	u32                             tx_tail;
	struct work_struct              tx_queue_work;
	u32                             tx_queue_num;
	u32                             tx_full_num;
	spinlock_t                      tx_lock;
	spinlock_t                      rx_lock;
};

#endif

