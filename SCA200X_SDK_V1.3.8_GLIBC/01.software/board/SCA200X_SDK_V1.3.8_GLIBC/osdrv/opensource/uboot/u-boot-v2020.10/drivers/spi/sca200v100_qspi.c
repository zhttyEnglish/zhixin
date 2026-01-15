// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021
 * SmartChip Corporation <www.sgitg.sgcc.com.cn>
 */

#include <common.h>
#include <clk.h>
#include <log.h>
#include <asm-generic/io.h>
#include <asm/io.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h>
#include <reset.h>
#include <asm/cache.h>
#include <cpu_func.h>
#include <spi.h>
#include <spi-mem.h>
#include <dm/device_compat.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/sizes.h>
#include <linux/mtd/spi-nor.h>
#include <linux/mtd/spinand.h>

/* Huffman related registers */
#define HDECOMP_BASE        0x01030000ULL
#define HDECOMP_HS_BASE     0x08B00050ULL
#define HDECOMP_CGU_BASE    0x01070058ULL

#define HDECOMP_ENABLE_REG            0x000
#define HDECOMP_POINT_BASE_ADDR       0x004
#define HDECOMP_END_DERR_MASK         0x008
#define HDECOMP_END_DERR              0x00C
#define HDECOMP_ITEM_DONE_CNT         0x010
#define HDECOMP_TREE_DONE_CNT         0x014
#define HDECOMP_ACTIVE_ITEM_NUM       0x018
#define HDECOMP_POINT_ADDR_X          0x01C
#define HDECOMP_R_SAR_X               0x020
#define HDECOMP_W_DAR_X               0x024
#define HDECOMP_ARLEN_X               0x028
#define HDECOMP_ARLEN_NUMX            0x02C
#define HDECOMP_AWLEN_X               0x030
#define HDECOMP_CL_REGSX              0x034
#define HDECOMP_SYM_REGSX             0x054

#define HDECOMP_ARLEN_0_S   0
#define HDECOMP_ARLEN_0_M   GENMASK(7, 0)
#define HDECOMP_ARLEN_1_S   8
#define HDECOMP_ARLEN_1_M   GENMASK(15, 8)
#define HDECOMP_ARLEN_B_S   29
#define HDECOMP_ARLEN_B_M   GENMASK(31, 29)
#define BUSRT_LEN       0x20
#define HDECOMP_AWLEN_AWLEN_S               0
#define HDECOMP_AWLEN_AWLEN_M               GENMASK(7, 0)
#define HDECOMP_AWLEN_TREE_POINTER          BIT(8)
#define HDECOMP_AWLEN_WFIFO_THRESHOLD_S     16
#define HDECOMP_AWLEN_AWLEN_THRESHOLD_M     GENMASK(23, 16)

/* ahb dma registers */
#define DMAC_SAR(n)            (0x000 + 0x058 * n)
#define DMAC_DAR(n)            (0x008 + 0x058 * n)
#define DMAC_LLP(n)            (0x010 + 0x058 * n)
#define DMAC_LLP_H(n)          (0x014 + 0x058 * n)
#define DMAC_CTL_L(n)          (0x018 + 0x058 * n)
#define DMAC_CTL_H(n)          (0x01C + 0x058 * n)
#define DMAC_CFG_L(n)          (0x040 + 0x058 * n)
#define DMAC_CFG_H(n)          (0x044 + 0x058 * n)
#define DMAC_SGR(n)            (0x048 + 0x058 * n)
#define DMAC_DSR(n)            (0x050 + 0x058 * n)

#define DMAC_RawTfr            (0x2C0)
#define DMAC_RawBlock          (0x2C8)
#define DMAC_RawSrcTran        (0x2D0)
#define DMAC_RawDstTran        (0x2D8)
#define DMAC_RawErr            (0x2E0)

#define DMAC_StatusTfr         (0x2E8)
#define DMAC_StatusBlock       (0x2F0)
#define DMAC_StatusSrcTran     (0x2F8)
#define DMAC_StatusDstTran     (0x300)
#define DMAC_StatusErr         (0x308)

#define DMAC_MaskTfr           (0x310)
#define DMAC_MaskBlock         (0x318)
#define DMAC_MaskSrcTran       (0x320)
#define DMAC_MaskDstTran       (0x328)
#define DMAC_MaskErr           (0x330)

#define DMAC_ClearTfr          (0x338)
#define DMAC_ClearBlock        (0x340)
#define DMAC_ClearSrcTran      (0x348)
#define DMAC_ClearDstTran      (0x350)
#define DMAC_ClearErr          (0x358)

#define DMAC_StatusInt         (0x360)

#define DMAC_DmaCfgReg         (0x398)

#define DMAC_ChEnReg           (0x3a0)

/* Bitfields in DW_PARAMS */
#define DW_PARAMS_NR_CHAN   8       /* number of channels */

/* Bitfields in CTL_LO */
#define DWC_CTLL_INT_EN     (1 << 0)    /* irqs enabled? */
#define DWC_CTLL_DST_WIDTH(n)   ((n)<<1)    /* bytes per element */
#define DWC_CTLL_SRC_WIDTH(n)   ((n)<<4)
#define DWC_CTLL_DST_INC    (0<<7)      /* DAR update/not */
#define DWC_CTLL_DST_DEC    (1<<7)
#define DWC_CTLL_DST_FIX    (2<<7)
#define DWC_CTLL_SRC_INC    (0<<9)      /* SAR update/not */
#define DWC_CTLL_SRC_DEC    (1<<9)
#define DWC_CTLL_SRC_FIX    (2<<9)
#define DWC_CTLL_DST_MSIZE(n)   ((n)<<11)   /* burst, #elements */
#define DWC_CTLL_SRC_MSIZE(n)   ((n)<<14)
#define DWC_CTLL_S_GATH_EN  (1 << 17)   /* src gather, !FIX */
#define DWC_CTLL_D_SCAT_EN  (1 << 18)   /* dst scatter, !FIX */
#define DWC_CTLL_FC(n)      ((n) << 20)
#define DWC_CTLL_FC_M2M     (0 << 20)   /* mem-to-mem */
#define DWC_CTLL_FC_M2P     (1 << 20)   /* mem-to-periph */
#define DWC_CTLL_FC_P2M     (2 << 20)   /* periph-to-mem */
#define DWC_CTLL_FC_P2P     (3 << 20)   /* periph-to-periph */
/* plus 4 transfer types for peripheral-as-flow-controller */
#define DWC_CTLL_DMS(n)     ((n)<<23)   /* dst master select */
#define DWC_CTLL_SMS(n)     ((n)<<25)   /* src master select */
#define DWC_CTLL_LLP_D_EN   (1 << 27)   /* dest block chain */
#define DWC_CTLL_LLP_S_EN   (1 << 28)   /* src block chain */

/* Bitfields in CTL_HI */
#define DWC_CTLH_BLOCK_TS_MASK  GENMASK(11, 0)
#define DWC_CTLH_BLOCK_TS(x)    ((x) & DWC_CTLH_BLOCK_TS_MASK)
#define DWC_CTLH_DONE       (1 << 12)

/* Bitfields in CFG_LO */
#define DWC_CFGL_CH_PRIOR_MASK  (0x7 << 5)  /* priority mask */
#define DWC_CFGL_CH_PRIOR(x)    ((x) << 5)  /* priority */
#define DWC_CFGL_CH_SUSP    (1 << 8)    /* pause xfer */
#define DWC_CFGL_FIFO_EMPTY (1 << 9)    /* pause xfer */
#define DWC_CFGL_HS_DST     (1 << 10)   /* handshake w/dst */
#define DWC_CFGL_HS_SRC     (1 << 11)   /* handshake w/src */
#define DWC_CFGL_LOCK_CH_XFER   (0 << 12)   /* scope of LOCK_CH */
#define DWC_CFGL_LOCK_CH_BLOCK  (1 << 12)
#define DWC_CFGL_LOCK_CH_XACT   (2 << 12)
#define DWC_CFGL_LOCK_BUS_XFER  (0 << 14)   /* scope of LOCK_BUS */
#define DWC_CFGL_LOCK_BUS_BLOCK (1 << 14)
#define DWC_CFGL_LOCK_BUS_XACT  (2 << 14)
#define DWC_CFGL_LOCK_CH    (1 << 15)   /* channel lockout */
#define DWC_CFGL_LOCK_BUS   (1 << 16)   /* busmaster lockout */
#define DWC_CFGL_HS_DST_POL (1 << 18)   /* dst handshake active low */
#define DWC_CFGL_HS_SRC_POL (1 << 19)   /* src handshake active low */
#define DWC_CFGL_MAX_BURST(x)   ((x) << 20)
#define DWC_CFGL_RELOAD_SAR (1 << 30)
#define DWC_CFGL_RELOAD_DAR (1 << 31)

/* Bitfields in CFG_HI */
#define DWC_CFGH_FCMODE     (1 << 0)
#define DWC_CFGH_FIFO_MODE  (1 << 1)
#define DWC_CFGH_PROTCTL(x) ((x) << 2)
#define DWC_CFGH_PROTCTL_DATA   (0 << 2)    /* data access - always set */
#define DWC_CFGH_PROTCTL_PRIV   (1 << 2)    /* privileged -> AHB HPROT[1] */
#define DWC_CFGH_PROTCTL_BUFFER (2 << 2)    /* bufferable -> AHB HPROT[2] */
#define DWC_CFGH_PROTCTL_CACHE  (4 << 2)    /* cacheable  -> AHB HPROT[3] */
#define DWC_CFGH_DS_UPD_EN  (1 << 5)
#define DWC_CFGH_SS_UPD_EN  (1 << 6)
#define DWC_CFGH_SRC_PER(x) ((x) << 7)
#define DWC_CFGH_DST_PER(x) ((x) << 11)

/* Bitfields in CFG */
#define DW_CFG_DMA_EN       (1 << 0)

/* flow controller */
enum dw_dma_fc {
	DW_DMA_FC_D_M2M,
	DW_DMA_FC_D_M2P,
	DW_DMA_FC_D_P2M,
	DW_DMA_FC_D_P2P,
	DW_DMA_FC_P_P2M,
	DW_DMA_FC_SP_P2P,
	DW_DMA_FC_P_M2P,
	DW_DMA_FC_DP_P2P,
};

/* bursts size */
enum dw_dma_msize {
	DW_DMA_MSIZE_1,
	DW_DMA_MSIZE_4,
	DW_DMA_MSIZE_8,
	DW_DMA_MSIZE_16,
	DW_DMA_MSIZE_32,
	DW_DMA_MSIZE_64,
	DW_DMA_MSIZE_128,
	DW_DMA_MSIZE_256,
};

/* transfer size */
enum dw_dma_tr_width {
	DW_DMA_TR_WIDTH_8,
	DW_DMA_TR_WIDTH_16,
	DW_DMA_TR_WIDTH_32,
	DW_DMA_TR_WIDTH_64,
	DW_DMA_TR_WIDTH_128,
	DW_DMA_TR_WIDTH_256,
};

#ifdef CONFIG_SMARTCHIP_SCA200V100
	/* cgu for qspi clock setting */
	#define CGU_QSPI_SEL_400M           BIT(1)
	#define CGU_QSPI_CLOCK_ON           BIT(3)
#elif defined(CONFIG_SMARTCHIP_SCA200V200)
	/* qspi cgu registers */
	#define CGU_QSPI_DIVIDER_S          0
	#define CGU_QSPI_DIVIDER_M          GENMASK(6, 0)
	#define CGU_QSPI_SELECT_S           8
	#define CGU_QSPI_SELECT_M           GENMASK(10, 8)
	#define CGU_QSPI_CLOCK_ON           BIT(12)

	#define CQS_FIXPLL_CLK666           0
	#define CQS_FIXPLL_CLK500           1
	#define CQS_FIXPLL_CLK400           2
	#define CQS_FIXPLL_CLK333           3
	#define CQS_ADCPLL_CLK600           4
	#define CQS_ADCPLL_CLK400           5
#endif

/* qspi registers */
#define QSPI_VERSION        0x0
#define QSPI_GCFG           0x04
#define QSPI_TIMING_CFG     0x08
#define QSPI_CTRL           0x10
#define QSPI_CMD            0x14
#define QSPI_ADDR           0x18
#define QSPI_DATA_NUM       0x1c
#define QSPI_THRESHOLD      0x20
#define QSPI_XIP_CMD        0x28
#define QSPI_XIP_ADDR       0x2c
#define QSPI_STATUS         0x30
#define QSPI_FIFO_ST        0x34
#define QSPI_FSM_ST         0x38
#define QSPI_TRAN_NUM       0x3c
#define QSPI_INT_EN         0x40
#define QSPI_INT_ST         0x44
#define QSPI_RFIFO_ENTRY    0x100
#define QSPI_WFIFO_ENTRY    0x200

#define QSPI_GCFG_WP_EN                 BIT(6)
#define QSPI_GCFG_HOLDN_EN              BIT(7)
#define QSPI_GCFG_MODE_CONTINUOUS_S     8
#define QSPI_GCFG_MODE_CONTINUOUS_M     GENMASK(15, 8)
#define QSPI_GCFG_INSTRUCT_BYP_EN       BIT(16)
#define QSPI_GCFG_FLASH_CS              BIT(17)
#define QSPI_GCFG_CLK_POLAR             BIT(18)
#define QSPI_GCFG_DTR                   BIT(19)
#define QSPI_GCFG_CLK_DIV_S             20
#define QSPI_GCFG_CLK_DIV_M             GENMASK(21, 20)
#define QSPI_GCFG_CLK_DIV_1             0
#define QSPI_GCFG_CLK_DIV_2             1
#define QSPI_GCFG_CLK_DIV_4             3

#define QSPI_TIMINIG_CFG_TCSS_S         0
#define QSPI_TIMINIG_CFG_TCSS_M         GENMASK(3, 0)
#define QSPI_TIMINIG_CFG_TSHSL_S        4
#define QSPI_TIMINIG_CFG_TSHSL_M        GENMASK(9, 4)
#define QSPI_TIMING_CFG_RD_LATENCY_S    10
#define QSPI_TIMING_CFG_RD_LATENCY_M    GENMASK(13, 10)

#define QSPI_CTRL_CMD_START             BIT(0)
#define QSPI_CTRL_FIFO_CLEAR            BIT(1)
#define QSPI_CTRL_INST_WIDTH_S          2
#define QSPI_CTRL_INST_WIDTH_M          GENMASK(3, 2)
#define QSPI_CTRL_ADDR_WIDTH_S          4
#define QSPI_CTRL_ADDR_WIDTH_M          GENMASK(5, 4)
#define QSPI_CTRL_DUMMY_WIDTH_S         6
#define QSPI_CTRL_DUMMY_WIDTH_M         GENMASK(7, 6)
#define QSPI_CTRL_DUMMY_WIDTH_MUL_8     0
#define QSPI_CTRL_DUMMY_WIDTH_MUL_4     1
#define QSPI_CTRL_DUMMY_WIDTH_MUL_2     2
#define QSPI_CTRL_DUMMY_WIDTH_MUL_1     3
#define QSPI_CTRL_DATA_WIDTH_S          8
#define QSPI_CTRL_DATA_WIDTH_M          GENMASK(9, 8)
#define QSPI_CTRL_WIDTH_1           0
#define QSPI_CTRL_WIDTH_2           1
#define QSPI_CTRL_WIDTH_4           2

#define QSPI_CMD_INST_S             0
#define QSPI_CMD_INST_M             GENMASK(7, 0)
#define QSPI_CMD_ADDR_NUM_S         8
#define QSPI_CMD_ADDR_NUM_M         GENMASK(10, 8)
#define QSPI_CMD_MODE_EN            BIT(11)
#define QSPI_CMD_DUMMY_NUM_S        12
#define QSPI_CMD_DUMMY_NUM_M        GENMASK(15, 12)
#define QSPI_CMD_MODE_S             16
#define QSPI_CMD_MODE_M             GENMASK(23, 16)
#define QSPI_CMD_WRITE              BIT(24)

#define QSPI_THRESHOLD_RD_S         0
#define QSPI_THRESHOLD_RD_M         GENMASK(6, 0)
#define QSPI_THRESHOLD_RDEN         BIT(7)
#define QSPI_THRESHOLD_WR_S         8
#define QSPI_THRESHOLD_WR_M         GENMASK(14, 8)
#define QSPI_THRESHOLD_WREN         BIT(15)
#define QSPI_THRESHOLD_SIZE         0x10

#define QSPI_STATUS_FLASH_BUSY      BIT(0)
#define qspi_status_is_busy(x)      (x & QSPI_STATUS_FLASH_BUSY)
#define QSPI_STATUS_FLASH_OWNER     BIT(1)

#define QSPI_FIFO_ST_RFIFO_FILLW_S  0
#define QSPI_FIFO_ST_RFIFO_FILLW_M  GENMASK(6, 0)
#define get_qspi_rfifo_fillw(x)     ((x & QSPI_FIFO_ST_RFIFO_FILLW_M) >> QSPI_FIFO_ST_RFIFO_FILLW_S)
#define QSPI_FIFO_ST_RFIFO_EMPTY    BIT(7)
#define QSPI_FIFO_ST_WFIFO_FREEW_S  8
#define QSPI_FIFO_ST_WFIFO_FREEW_M  GENMASK(14, 8)
#define get_qspi_wfifo_freew(x)     ((x & QSPI_FIFO_ST_WFIFO_FREEW_M) >> QSPI_FIFO_ST_WFIFO_FREEW_S)
#define QSPI_FIFO_ST_WFIFO_FULL     BIT(15)
#define QSPI_FIFO_DMA_RD_REQ        BIT(16)
#define QSPI_FIFO_DMA_RD_ACK        BIT(17)
#define QSPI_FIFO_DMA_WR_REQ        BIT(18)
#define QSPI_FIFO_DMA_WR_ACK        BIT(19)

#define QSPI_INT_EN_DONE_INT_EN     BIT(0)

#define QSPI_INT_ST_DONE_INT_ST     BIT(0)

enum qspi_fsm {
	ST_IDLE,
	ST_CSN_L,
	ST_INST,
	ST_ADDR,
	ST_MODE,
	ST_DUMMY,
	ST_WAIT,
	ST_RD_DATA,
	ST_WR_DATA,
	ST_CSN_H
};

/* fifo size is 64 words, beacause hardware's problem, 63 words can be used actually */
#define QSPI_FIFO_SIZE      0x40
/* DW_DMA_BLOCK_MAX is 0xfff */
#define QSPI_DMA_BLOCK_MAX  0x800
/* qspi timeout 2s */
#define QSPI_TIMEOUT_MS     2000
/* dma hardshaking select for qspi */
#define DMA_HS_QSPI_TX  38
#define DMA_HS_QSPI_RX  39

struct sca200v100_qspi_debug_info {
#define DBG_OPCODE      BIT(0)
#define DBG_OPCODE_V    BIT(1)
#define DBG_INTERRUPT   BIT(2)
#define DBG_TASKLET     BIT(3)
#define DBG_RFIFO       BIT(4)
#define DBG_WFIFO       BIT(5)
#define DBG_DMA_RD      BIT(6)
#define DBG_DMA_WR      BIT(7)
#define DBG_OPCODE_END  BIT(8)
#define DBG_LOCK        BIT(9)
#define DBG_CS          BIT(10)
#define DBG_DTR         BIT(11)
#define DBG_PRINT_RFIFO BIT(12)
#define DBG_TRAINING    BIT(13)
	unsigned int dbg_flag;
};

struct sca200v100_qspi {
	void *regs; /* Qspi register registers */
	void *cgu_base; /* cgu register base, only qspi related clock config */
	void *dma_base; /* ahb dma register base */
	void *hs_sel_base; /* ahb dma hardshaking select register base */
	u32 clk_rate;
	int dma_ch;
	int timing;
	int cur_die; /* flash has two dies */
	int cur_cs;
	int cur_dtr; /* 0: str; 1: dtr */

	struct sca200v100_qspi_debug_info *dbg;
};

/* dma handshake select Huffman(1) / ahb dma(0) */
static void sca200v100_qspi_dma_select(int huffman)
{
	if (huffman)
		writel(1, HDECOMP_HS_BASE);
	else
		writel(0, HDECOMP_HS_BASE);
}

int sca200v100_hdecomp_read_init(struct sca200v100_qspi *aq, unsigned int addr, unsigned int len)
{
	int item_num = 0;
	unsigned int words, bursts, ws, bs, tmp;
	void *hdecomp_base = (void *)HDECOMP_BASE;

	sca200v100_qspi_dma_select(1);
	/* set Huffman decomp clock 500Mhz */
	writel(0xfffffff9, HDECOMP_CGU_BASE);

	/* APB configuration */
	writel((0x2 << 25) + (0x2 << 8), hdecomp_base + HDECOMP_ENABLE_REG);
	/* first link list address,when set APB_START_MODE,equal to point_addr0 */
	writel(0, hdecomp_base + HDECOMP_POINT_BASE_ADDR);
	/* APB_START_MODE,outstanding(p_outs,d_outs),mask */
	writel(0x81007109, hdecomp_base + HDECOMP_END_DERR_MASK);
	/* next point address */
	writel(0, hdecomp_base + HDECOMP_POINT_ADDR_X);
	/* read data address */
	writel((unsigned long)aq->regs + QSPI_RFIFO_ENTRY, hdecomp_base + HDECOMP_R_SAR_X);
	/* write data address */
	writel(addr, hdecomp_base + HDECOMP_W_DAR_X);

	ws = len / 4;
	bs = len % 4; /* bytes */
	bursts = ws / BUSRT_LEN; /* bursts */
	words = ws % BUSRT_LEN; /* words */
	tmp = ((BUSRT_LEN - 1) << HDECOMP_ARLEN_0_S)
	    | ((words) << HDECOMP_ARLEN_1_S)
	    | (bs << HDECOMP_ARLEN_B_S);
	/* Byte_vld,Arlen_num(block),Arlen1(word),Arlen0(block) */
	writel(tmp, hdecomp_base + HDECOMP_ARLEN_X);
	writel(bursts, hdecomp_base + HDECOMP_ARLEN_NUMX);

	tmp = ((BUSRT_LEN - 1) << HDECOMP_AWLEN_AWLEN_S)
	    | (BUSRT_LEN << HDECOMP_AWLEN_WFIFO_THRESHOLD_S);
	/* Next_point of tree,Awlen */
	writel(tmp, hdecomp_base + HDECOMP_AWLEN_X);

#if 0
	/* TODO: read para from flash */
	if (huffman_para_addr != 0xffffffff) {
		unsigned int *p;
		int i;

		p = (unsigned int *)(huffman_src + 8);
		for (i = 0; i < 0x48; i++) {
			writel(HDECOMP_BASE + HDECOMP_CL_REGSX + i * 4, *p++);
		}
	}
#endif

	item_num = item_num + 1;
	writel(item_num, hdecomp_base + HDECOMP_ACTIVE_ITEM_NUM);

	writel((0x2 << 25) | (0x2 << 8) | BIT(0), hdecomp_base + HDECOMP_ENABLE_REG);

	return 0;
}

static void ahb_dmac_set_hs(struct sca200v100_qspi *aq, int src_per, int dst_per)
{
	unsigned int tmp;
	tmp = src_per | dst_per << 8;
	writel(tmp, aq->hs_sel_base + aq->dma_ch * 4);
}

static void ahb_dmac_clearint(struct sca200v100_qspi *aq)
{
	unsigned int tmp;

	/* clear interrupt */
	tmp = 1 << aq->dma_ch;
	writel(tmp, aq->dma_base + DMAC_ClearTfr);
	writel(tmp, aq->dma_base + DMAC_ClearBlock);
	writel(tmp, aq->dma_base + DMAC_ClearSrcTran);
	writel(tmp, aq->dma_base + DMAC_ClearDstTran);
	writel(tmp, aq->dma_base + DMAC_ClearErr);

	tmp &= ~((1 << aq->dma_ch) | (1 << (aq->dma_ch + 8)));
	writel(tmp, aq->dma_base + DMAC_MaskTfr);
	writel(tmp, aq->dma_base + DMAC_MaskSrcTran);
	writel(tmp, aq->dma_base + DMAC_MaskDstTran);
	writel(tmp, aq->dma_base + DMAC_MaskErr);

	/* enable block transfer interrupt */
	tmp |= ((1 << aq->dma_ch) | (1 << (aq->dma_ch + 8)));
	writel(tmp, aq->dma_base + DMAC_MaskBlock);
}

/* DW_DMA_TR_WIDTH_32 is selected, so blk_ts unit is one word(4 Byte) */
static int ahb_dmac_init(struct sca200v100_qspi *aq, int read)
{
	unsigned int tmp;

	/* step 1: dma hardshaking interface config */
	if (read) {
		ahb_dmac_set_hs(aq, DMA_HS_QSPI_RX, 0xff);
		/* step 4: set sar */
		writel((unsigned long)aq->regs + QSPI_RFIFO_ENTRY, aq->dma_base + DMAC_SAR(aq->dma_ch));
	} else {
		ahb_dmac_set_hs(aq, 0xff, DMA_HS_QSPI_TX);
		writel((unsigned long)aq->regs + QSPI_WFIFO_ENTRY, aq->dma_base + DMAC_DAR(aq->dma_ch));
	}

	/* step 2: CFG config, ctlr set */
	writel(0, aq->dma_base + DMAC_CFG_L(aq->dma_ch));

	tmp = DWC_CFGH_FIFO_MODE
	    | DWC_CFGH_SRC_PER(aq->dma_ch * 2)
	    | DWC_CFGH_DST_PER(aq->dma_ch * 2 + 1)
	    | DWC_CFGH_PROTCTL_PRIV;
	writel(tmp, aq->dma_base + DMAC_CFG_H(aq->dma_ch));

	tmp = DWC_CTLL_DST_WIDTH(DW_DMA_TR_WIDTH_32)
	    | DWC_CTLL_SRC_WIDTH(DW_DMA_TR_WIDTH_32)
	    | (read ? DWC_CTLL_DST_INC : DWC_CTLL_SRC_INC)
	    | (read ? DWC_CTLL_SRC_FIX : DWC_CTLL_DST_FIX)
	    | DWC_CTLL_DST_MSIZE(DW_DMA_MSIZE_8)
	    | DWC_CTLL_SRC_MSIZE(DW_DMA_MSIZE_8)
	    | (read ? DWC_CTLL_FC_P2M : DWC_CTLL_FC_M2P)
	    | DWC_CTLL_DMS(0)
	    | DWC_CTLL_SMS(0)
	    | DWC_CTLL_INT_EN;
	writel(tmp, aq->dma_base + DMAC_CTL_L(aq->dma_ch));

	/* step 3: no llp, multi-block */
	writel(0, aq->dma_base + DMAC_LLP(aq->dma_ch));

	/* step 5: clear interrupt */
	ahb_dmac_clearint(aq);

	/* step 6: dmac enable */
	writel(1, aq->dma_base + DMAC_DmaCfgReg);

	return 0;
}

static void ahb_dmac_channel_enable(struct sca200v100_qspi *aq)
{
	unsigned tmp;

	/* dma channel enable */
	tmp = (1 << aq->dma_ch) | (1 << (aq->dma_ch + DW_PARAMS_NR_CHAN));
	writel(tmp, aq->dma_base + DMAC_ChEnReg);
}

static void ahb_dmac_channel_disable(struct sca200v100_qspi *aq)
{
	unsigned tmp;

	/* dma channel enable */
	tmp = (0 << aq->dma_ch) | (1 << (aq->dma_ch + DW_PARAMS_NR_CHAN));
	writel(tmp, aq->dma_base + DMAC_ChEnReg);
}

static int ahb_dmac_check_finish_block(struct sca200v100_qspi *aq)
{
	unsigned int tmp;
	unsigned long timeout;

	timeout = get_ticks() + usec2ticks(QSPI_TIMEOUT_MS * 1000);
	do {
		tmp = readl(aq->dma_base + DMAC_RawBlock);
		tmp &= (1 << aq->dma_ch);
		if (time_after((unsigned long)get_ticks(), timeout)) {
			pr_err("%s [%d]: timeout\n", __func__, __LINE__);
			return -ETIMEDOUT;
		}
	} while (!tmp);

	writel(1 << aq->dma_ch, aq->dma_base + DMAC_ClearBlock);

	return 0;
}

/* FIXME: word aligned right now */
static int ahb_dmac_single_block_tran(struct sca200v100_qspi *aq, unsigned int addr, unsigned int len, int read)
{
	int ret;
	unsigned int words;

	words = len / 4;
	if (!words || words > QSPI_DMA_BLOCK_MAX) {
		return -EINVAL;
	}

	if (read) {
		ret = ahb_dmac_init(aq, 1);
		if (ret)
			return ret;
		writel(addr, aq->dma_base + DMAC_DAR(aq->dma_ch));
	} else {
		ret = ahb_dmac_init(aq, 0);
		if (ret)
			return ret;
		writel(addr, aq->dma_base + DMAC_SAR(aq->dma_ch));
	}

	writel(words, aq->dma_base + DMAC_CTL_H(aq->dma_ch));
	ahb_dmac_channel_enable(aq);
	ret = ahb_dmac_check_finish_block(aq);
	if (ret)
		return ret;

	ahb_dmac_channel_disable(aq);

	return 0;
}

static int sca200v100_qspi_dma_read(struct sca200v100_qspi *aq, unsigned int addr, unsigned int len)
{
	if (aq->dbg->dbg_flag & DBG_DMA_RD)
		pr_info("%s [%d]: dma read len = %#x\n", __func__, __LINE__, len);

	return ahb_dmac_single_block_tran(aq, addr, len, 1);
}

static int sca200v100_qspi_dma_write(struct sca200v100_qspi *aq, unsigned int addr, unsigned int len)
{
	if (aq->dbg->dbg_flag & DBG_DMA_WR)
		pr_info("%s [%d]: dma write len = %#x\n", __func__, __LINE__, len);

	return ahb_dmac_single_block_tran(aq, addr, len, 0);
}

/* qspi related codes */
static void sca200v100_qspi_registers_dump(struct sca200v100_qspi *aq)
{
	int i, tmp;

	/* 0x0c is not exsit */
	for (i = 0; i < 18; i++) {
		tmp = readl(aq->regs + i * 4);
		pr_info("%p: %#x\n", aq->regs + i * 4, tmp);
	}
}

static __maybe_unused void sca200v100_qspi_set_spi_mode(struct sca200v100_qspi *aq, int mod)
{
	unsigned int tmp;

	tmp = readl(aq->regs + QSPI_GCFG);
	if (mod == 0)
		tmp |= QSPI_GCFG_CLK_POLAR;
	else
		tmp &= ~QSPI_GCFG_CLK_POLAR;
	writel(tmp, aq->regs + QSPI_GCFG);
}

static __maybe_unused void sca200v100_qspi_set_flash_cs(struct sca200v100_qspi *aq, int cs)
{
	int tmp;

	tmp = readl(aq->regs + QSPI_GCFG);
	if (cs)
		tmp |= QSPI_GCFG_FLASH_CS;
	else
		tmp &= ~QSPI_GCFG_FLASH_CS;
	writel(tmp, aq->regs + QSPI_GCFG);
}

static void sca200v100_qspi_set_timing_cfg(struct sca200v100_qspi *aq, int val)
{
	unsigned int tmp;

	tmp = readl(aq->regs + QSPI_TIMING_CFG);
	tmp &= ~QSPI_TIMING_CFG_RD_LATENCY_M;
	tmp |= (val << QSPI_TIMING_CFG_RD_LATENCY_S);
	writel(tmp, aq->regs + QSPI_TIMING_CFG);
}

static __maybe_unused void sca200v100_qspi_clear_fifo(struct sca200v100_qspi *aq)
{
	unsigned int tmp;

	tmp = readl(aq->regs + QSPI_CTRL);
	tmp |= QSPI_CTRL_FIFO_CLEAR;
	writel(tmp, aq->regs + QSPI_CTRL);
}

/* dma read and write enable */
static void sca200v100_qspi_dma_enable(struct sca200v100_qspi *aq)
{
	unsigned int tmp;

	/* qspi dma read enable, threshold is 16 byte */
	tmp = QSPI_THRESHOLD_SIZE << QSPI_THRESHOLD_RD_S
	    | QSPI_THRESHOLD_RDEN
	    | QSPI_THRESHOLD_SIZE << QSPI_THRESHOLD_WR_S
	    | QSPI_THRESHOLD_WREN;
	writel(tmp, aq->regs + QSPI_THRESHOLD);
}

/* disable dma read and write */
static void sca200v100_qspi_dma_disable(struct sca200v100_qspi *aq)
{
	writel(0, aq->regs + QSPI_THRESHOLD);
}

static __maybe_unused void sca200v100_qspi_int_enable(struct sca200v100_qspi *aq)
{
	writel(QSPI_INT_EN_DONE_INT_EN, aq->regs + QSPI_INT_EN);
}

static void sca200v100_qspi_int_disable(struct sca200v100_qspi *aq)
{
	writel(0, aq->regs + QSPI_INT_EN);
}

static __maybe_unused void sca200v100_qspi_clear_interrupt(struct sca200v100_qspi *aq)
{
	/* write 1 clear interrupt */
	writel(1, aq->regs + QSPI_INT_ST);
}

static __maybe_unused void sca200v100_qspi_dtr_enable(struct sca200v100_qspi *aq)
{
	int tmp;

	tmp = readl(aq->regs + QSPI_GCFG);
	tmp |= QSPI_GCFG_DTR;
	writel(tmp, aq->regs + QSPI_GCFG);
}

static __maybe_unused void sca200v100_qspi_dtr_disable(struct sca200v100_qspi *aq)
{
	int tmp;

	tmp = readl(aq->regs + QSPI_GCFG);
	tmp &= ~QSPI_GCFG_DTR;
	writel(tmp, aq->regs + QSPI_GCFG);
}

#ifdef CONFIG_SMARTCHIP_SCA200V100
/* TODO: 133M */
static __maybe_unused void sca200v100_qspi_set_clk(struct sca200v100_qspi *aq, unsigned int rate)
{
	unsigned int tmp, div;

	if (rate == 0)
		rate = aq->clk_rate;

	if (rate == 100000000 || rate == 50000000 || rate == 25000000) {
		tmp = readl(aq->cgu_base);
		tmp |= CGU_QSPI_SEL_400M;
		writel(tmp, aq->cgu_base);
	} else if (rate == 150000000 || rate == 75000000 || rate == 37500000) {
		tmp = readl(aq->cgu_base);
		tmp &= ~CGU_QSPI_SEL_400M; /* 600M */
		writel(tmp, aq->cgu_base);
	} else {
		/* TODO: 133M */
	}

	switch (rate) {
	case 150000000:
	case 100000000:
		div = QSPI_GCFG_CLK_DIV_1 << QSPI_GCFG_CLK_DIV_S;
		break;
	case 75000000:
	case 50000000:
		div = QSPI_GCFG_CLK_DIV_2 << QSPI_GCFG_CLK_DIV_S;
		break;
	case 37500000:
	case 25000000:
		div = QSPI_GCFG_CLK_DIV_4 << QSPI_GCFG_CLK_DIV_S;
		break;
	default:
		div = QSPI_GCFG_CLK_DIV_4 << QSPI_GCFG_CLK_DIV_S;
		break;
	}

	tmp = readl(aq->regs + QSPI_GCFG);
	tmp &= ~QSPI_GCFG_CLK_DIV_M;
	tmp |= div;
	writel(tmp, aq->regs + QSPI_GCFG);
}
#elif defined(CONFIG_SMARTCHIP_SCA200V200)
static __maybe_unused void sca200v100_qspi_set_clk(struct sca200v100_qspi *aq, unsigned int rate)
{
	unsigned int tmp;

	if (rate == 0)
		rate = aq->clk_rate;

	tmp = readl(aq->cgu_base);

	if (rate == 100000000 || rate == 50000000 || rate == 25000000) {
		tmp &= ~CGU_QSPI_SELECT_M;
		tmp |= CQS_FIXPLL_CLK400 << CGU_QSPI_SELECT_S;
	} else if (rate == 150000000 || rate == 75000000 || rate == 37500000 || rate == 133000000) {
		tmp &= ~CGU_QSPI_SELECT_M;
		tmp |= CQS_ADCPLL_CLK600 << CGU_QSPI_SELECT_S;
	}

	switch (rate) {
	case 150000000:
	case 100000000:
		tmp &= ~CGU_QSPI_DIVIDER_M;
		tmp |= 0 << CGU_QSPI_DIVIDER_S;
		break;
	case 75000000:
	case 50000000:
		tmp &= ~CGU_QSPI_DIVIDER_M;
		tmp |= 1 << CGU_QSPI_DIVIDER_S;
		break;
	case 37500000:
	case 25000000:
		tmp &= ~CGU_QSPI_DIVIDER_M;
		tmp |= 3 << CGU_QSPI_DIVIDER_S;
		break;
	case 133000000:
		tmp &= ~CGU_QSPI_DIVIDER_M;
		tmp |= 4 << CGU_QSPI_DIVIDER_S;
		break;
	default:
		break;
	}

	tmp |= CGU_QSPI_CLOCK_ON;
	writel(tmp, aq->cgu_base);
}
#endif

static int sca200v100_qspi_waiting_rfifo_not_empty(struct sca200v100_qspi *aq)
{
	unsigned int tmp;
	unsigned long timeout;

	timeout = get_ticks() + usec2ticks(QSPI_TIMEOUT_MS * 1000);
	do {
		tmp = readl(aq->regs + QSPI_FIFO_ST);
		if (time_after((unsigned long)get_ticks(), timeout)) {
			pr_err("%s [%d]: timeout\n", __func__, __LINE__);
			return -ETIMEDOUT;
		}
	} while (tmp & QSPI_FIFO_ST_RFIFO_EMPTY);

	return 0;
}

static int sca200v100_qspi_read_fifo(struct sca200v100_qspi *aq, unsigned char *buf, unsigned int len)
{
	int i, div, mod, ret, rlen;
	unsigned int tmp;

	if (!buf || !len) {
		pr_err("%s [%d]: buf = %p, len = %#x\n", __func__, __LINE__, buf, len);
		return -EINVAL;
	}

	if (aq->dbg->dbg_flag & DBG_RFIFO)
		pr_info("%s [%d]: read fifo len = %#x\n", __func__, __LINE__, len);

	div = len / 4;
	mod = len % 4;

	while (div) {
		tmp = readl(aq->regs + QSPI_FIFO_ST);
		/* get wfifo free words */
		ret = get_qspi_rfifo_fillw(tmp);
		if (div > ret)
			rlen = ret;
		else
			rlen = div;

		for (i = 0; i < rlen; i++) {
			*(unsigned int *)buf = readl(aq->regs + QSPI_RFIFO_ENTRY);
			buf += 4;
		}
		div -= rlen;
	}

	if (mod) {
		ret = sca200v100_qspi_waiting_rfifo_not_empty(aq);
		if (ret)
			return ret;
		tmp = readl(aq->regs + QSPI_RFIFO_ENTRY);
		if (buf) {
			for (i = 0; i < mod; i++) {
				buf[i] = tmp & 0xff;
				tmp >>= 8;
			}
		}
	}

	return 0;
}

static int sca200v100_qspi_write_fifo(struct sca200v100_qspi *aq, unsigned char *buf, unsigned int len)
{
	int i, div, mod, ret, wlen;
	unsigned int tmp;

	if (aq->dbg->dbg_flag & DBG_WFIFO)
		pr_info("%s [%d]: write fifo len = %#x\n", __func__, __LINE__, len);

	div = len / 4;
	mod = len % 4;

	while (div) {
		tmp = readl(aq->regs + QSPI_FIFO_ST);
		/* get wfifo free words */
		ret = get_qspi_wfifo_freew(tmp);
		if (div > ret)
			wlen = ret;
		else
			wlen = div;

		for (i = 0; i < wlen; i++) {
			writel(*(unsigned int *)buf, aq->regs + QSPI_WFIFO_ENTRY);
			buf += 4;
			div--;
		}
	}

	if (mod) {
		for (i = 0; i < mod; i++) {
			writeb(*buf++, aq->regs + QSPI_WFIFO_ENTRY);
		}
	}

	return 0;
}

static int sca200v100_qspi_waiting_cmd_finish(struct sca200v100_qspi *aq)
{
	unsigned tmp;
	unsigned long timeout;

	timeout = get_ticks() + usec2ticks(QSPI_TIMEOUT_MS * 1000);
	do {
		tmp = readl(aq->regs + QSPI_STATUS);
		if (time_after((unsigned long)get_ticks(), timeout)) {
			pr_err("%s [%d]: timeout\n", __func__, __LINE__);
			return -ETIMEDOUT;
		}
	} while (qspi_status_is_busy(tmp));

	return 0;
}

static void sca200v100_qspi_linewidth_convert(const struct spi_mem_op *op, unsigned int *width)
{
	if (op->cmd.buswidth == 1)
		width[0] = QSPI_CTRL_WIDTH_1;
	else if (op->addr.buswidth == 2)
		width[0] = QSPI_CTRL_WIDTH_2;
	else if (op->addr.buswidth == 4)
		width[0] = QSPI_CTRL_WIDTH_4;
	else
		width[0] = QSPI_CTRL_WIDTH_1;

	if (op->addr.buswidth == 1)
		width[1] = QSPI_CTRL_WIDTH_1;
	else if (op->addr.buswidth == 2)
		width[1] = QSPI_CTRL_WIDTH_2;
	else if (op->addr.buswidth == 4)
		width[1] = QSPI_CTRL_WIDTH_4;
	else
		width[1] = QSPI_CTRL_WIDTH_1;

	/* FIXME: only dummy cycles is an integral multiple of 8 */
	if (op->dummy.buswidth == 1)
		width[2] = QSPI_CTRL_DUMMY_WIDTH_MUL_8;
	else if (op->dummy.buswidth == 2)
		width[2] = QSPI_CTRL_DUMMY_WIDTH_MUL_4;
	else if (op->dummy.buswidth == 4)
		width[2] = QSPI_CTRL_DUMMY_WIDTH_MUL_2;
	else
		width[2] = QSPI_CTRL_DUMMY_WIDTH_MUL_8;

	if (op->data.buswidth == 1)
		width[3] = QSPI_CTRL_WIDTH_1;
	else if (op->data.buswidth == 2)
		width[3] = QSPI_CTRL_WIDTH_2;
	else if (op->data.buswidth == 4)
		width[3] = QSPI_CTRL_WIDTH_4;
	else
		width[3] = QSPI_CTRL_WIDTH_1;
}

/* don't check parameters, ensure correct by the caller */
static int sca200v100_qspi_send_cmd(struct sca200v100_qspi *aq, const struct spi_mem_op *op)
{
	int ret;
	unsigned int tmp, width[4];

	if (aq->dbg->dbg_flag & DBG_OPCODE_V) {
		pr_info("opcode: %#x %#x, addr: %#x %#x %#llx, dummy: %#x %#x, data: %#x %#x\n",
		    op->cmd.opcode, op->cmd.buswidth, op->addr.nbytes, op->addr.buswidth, op->addr.val,
		    op->dummy.nbytes, op->dummy.buswidth, op->data.nbytes, op->data.buswidth);
	} else if (aq->dbg->dbg_flag & DBG_OPCODE)
		pr_info("opcode: %#x\n", op->cmd.opcode);

	sca200v100_qspi_linewidth_convert(op, width);

	/* waiting for the previous cmd finish */
	ret = sca200v100_qspi_waiting_cmd_finish(aq);
	if (ret)
		return ret;

	/* cmd */
	tmp = op->cmd.opcode << QSPI_CMD_INST_S
	    | op->addr.nbytes << QSPI_CMD_ADDR_NUM_S
	    | op->dummy.nbytes << QSPI_CMD_DUMMY_NUM_S
	    | (op->data.dir == SPI_MEM_DATA_OUT ? QSPI_CMD_WRITE : 0);
	writel(tmp, aq->regs + QSPI_CMD);
	/* address */
	if (op->addr.nbytes)
		writel(op->addr.val, aq->regs + QSPI_ADDR);
	/* data length */
	writel(op->data.nbytes, aq->regs + QSPI_DATA_NUM);
	/* dataline width */
	tmp = QSPI_CTRL_CMD_START
	    | width[0] << QSPI_CTRL_INST_WIDTH_S
	    | width[1] << QSPI_CTRL_ADDR_WIDTH_S
	    | width[2] << QSPI_CTRL_DUMMY_WIDTH_S
	    | width[3] << QSPI_CTRL_DATA_WIDTH_S;
	/* send cmd to flash */
	writel(tmp, aq->regs + QSPI_CTRL);

	if (aq->dbg->dbg_flag & DBG_OPCODE_END)
		pr_info("opcode end\n");

	return 0;
}

#define TRAINING_MAX    16
#define TRAINING_READ_CNT   3
#define TAG_NOR     0xd2810600
#ifdef CONFIG_SMARTCHIP_SCA200V100
	#define TAG_NAND    0x41529301
	#define TAG_NAND_NOR_COMP   0x52930100
#elif defined(CONFIG_SMARTCHIP_SCA200V200)
	#define TAG_NAND    0x494d4711
	#define TAG_NAND_NOR_COMP   0x4d471100
#endif

static int sca200v100_qspi_nor_fast_read(struct sca200v100_qspi *aq)
{
	int ret, i;
	unsigned int tmp, buf;
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(SPINOR_OP_READ_FAST, 1),
	        SPI_MEM_OP_ADDR(3, 0, 1),
	        SPI_MEM_OP_DUMMY(1, 1),
	        SPI_MEM_OP_DATA_IN(4, &buf, 1));

	for (i = 0; i < TRAINING_READ_CNT; i++) {
		ret = sca200v100_qspi_send_cmd(aq, &op);
		if (ret) {
			pr_err("%s [%d]: qspi send command failed\n", __func__, __LINE__);
			goto out;
		}

		ret = sca200v100_qspi_read_fifo(aq, op.data.buf.in, op.data.nbytes);
		if (ret)
			goto out;

		tmp = readl(aq->regs + QSPI_FIFO_ST);
		/* rfifo should be empty */
		if (!(tmp & QSPI_FIFO_ST_RFIFO_EMPTY)) {
			pr_err("rfifo is not empty\n");
			ret = -1;
			goto out;
		}

		if (aq->dbg->dbg_flag & DBG_TRAINING)
			pr_info("training(nor) read value: %#x\n", buf);
		if (buf != TAG_NOR) {
			break;
		}
	}

	if (i == TRAINING_READ_CNT)
		return 0;
	else
		return -1;
out:
	sca200v100_qspi_registers_dump(aq);
	return ret;
}

static int spinand_read_reg_op(struct sca200v100_qspi *aq, u8 reg, u8 *val)
{
	int ret;
	unsigned int tmp;
	unsigned char buf;
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(reg,
	        &buf);

	ret = sca200v100_qspi_send_cmd(aq, &op);
	if (ret) {
		pr_err("%s [%d]: qspi send command failed\n", __func__, __LINE__);
		goto out;
	}

	ret = sca200v100_qspi_read_fifo(aq, op.data.buf.in, op.data.nbytes);
	if (ret)
		goto out;

	tmp = readl(aq->regs + QSPI_FIFO_ST);
	/* rfifo should be empty */
	if (!(tmp & QSPI_FIFO_ST_RFIFO_EMPTY)) {
		pr_err("rfifo is not empty\n");
		ret = -1;
		goto out;
	}
	*val = buf;
	return 0;
out:
	return ret;
}

static int spinand_read_status(struct sca200v100_qspi *aq, u8 *status)
{
	return spinand_read_reg_op(aq, REG_STATUS, status);
}

static int spinand_wait(struct sca200v100_qspi *aq, u8 *s)
{
	unsigned long start, stop;
	u8 status;
	int ret;

	start = get_timer(0);
	stop = 400;
	do {
		ret = spinand_read_status(aq, &status);
		if (ret)
			return ret;

		if (!(status & STATUS_BUSY))
			goto out;
	} while (get_timer(start) < stop);

	/*
	 * Extra read, just in case the STATUS_READY bit has changed
	 * since our last check
	 */
	ret = spinand_read_status(aq, &status);
	if (ret)
		return ret;

out:
	if (s)
		*s = status;

	return status & STATUS_BUSY ? -ETIMEDOUT : 0;
}

static int sca200v100_qspi_nand_fast_read(struct sca200v100_qspi *aq)
{
	int ret, i;
	unsigned int tmp, buf;
	struct spi_mem_op op_pageload = SPINAND_PAGE_READ_OP(0);
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(SPINOR_OP_READ_FAST, 1),
	        SPI_MEM_OP_ADDR(3, 0, 1),
	        SPI_MEM_OP_NO_DUMMY,
	        SPI_MEM_OP_DATA_IN(4, &buf, 1));

	for (i = 0; i < TRAINING_READ_CNT; i++) {
		ret = sca200v100_qspi_send_cmd(aq, &op_pageload);
		if (ret) {
			pr_err("%s [%d]: qspi send command failed\n", __func__, __LINE__);
			goto out;
		}

		ret = spinand_wait(aq, NULL);
		if (ret)
			goto out1;

		/* read the first page and don't check ecc */
		ret = sca200v100_qspi_send_cmd(aq, &op);
		if (ret) {
			pr_err("%s [%d]: qspi send command failed\n", __func__, __LINE__);
			goto out;
		}

		ret = sca200v100_qspi_read_fifo(aq, op.data.buf.in, op.data.nbytes);
		if (ret)
			goto out;

		tmp = readl(aq->regs + QSPI_FIFO_ST);
		/* rfifo should be empty */
		if (!(tmp & QSPI_FIFO_ST_RFIFO_EMPTY)) {
			pr_err("rfifo is not empty\n");
			ret = -1;
			goto out;
		}

		if (aq->dbg->dbg_flag & DBG_TRAINING)
			pr_info("training(nand) read value: %#x\n", buf);
		if (buf != TAG_NAND && buf != TAG_NAND_NOR_COMP)
			break;
	}

	if (i == TRAINING_READ_CNT)
		return 0;
	else
		return -1;
out1:
	return ret;
out:
	sca200v100_qspi_registers_dump(aq);
	return ret;
}

static __maybe_unused int sca200v100_qspi_training(struct sca200v100_qspi *aq)
{
	int ret, i, first = -1, last = -1;
	int (*fast_read)(struct sca200v100_qspi * aq) = sca200v100_qspi_nor_fast_read;

again:
	for (i = 0; i < TRAINING_MAX; i++) {
		sca200v100_qspi_set_timing_cfg(aq, i);
		ret = fast_read(aq);
		if (!ret) {
			if (first < 0)
				first = i;
			last = i;
		} else {
			/* the first right timing has been found and read error */
			if (first >= 0)
				break;
		}
	}

	if (first < 0 && fast_read == sca200v100_qspi_nor_fast_read) {
		fast_read = sca200v100_qspi_nand_fast_read;
		goto again;
	} else if (first >= 0) {
		/* found the right region, set the middle value */
		sca200v100_qspi_set_timing_cfg(aq, (first + last) / 2);
		printf("after qspi training, set timing: %d, fisrt: %d, last: %d\n", (first + last) / 2, first, last);
	} else {
		return -1;
	}

	return 0;
}

static int sca200v100_qspi_init(struct sca200v100_qspi *aq)
{
	/* interrupt disable */
	sca200v100_qspi_int_disable(aq);
#if 0
	sca200v100_set_pinshare(PIN_NO(76), PIN_FUNC_QSPI_SCK);
	sca200v100_set_pinshare(PIN_NO(77), PIN_FUNC_QSPI_CS);
	sca200v100_set_pinshare(PIN_NO(80), PIN_FUNC_QSPI_DATA_0);
	sca200v100_set_pinshare(PIN_NO(81), PIN_FUNC_QSPI_DATA_1);
	sca200v100_set_pinshare(PIN_NO(82), PIN_FUNC_QSPI_DATA_2);
	sca200v100_set_pinshare(PIN_NO(83), PIN_FUNC_QSPI_DATA_3);
#endif

	/* use flash cs 0 */
	sca200v100_qspi_set_flash_cs(aq, 0);
	/* dtr disable */
	sca200v100_qspi_dtr_disable(aq);
	aq->cur_die = 0;
	aq->cur_cs = 0;
	aq->cur_dtr = 0;

	aq->dbg = (struct sca200v100_qspi_debug_info *)((unsigned long)&aq->dbg + sizeof(void *));

	/* only for debug, notice loglevel too */
	/* aq->dbg->dbg_flag = 0x0001; */

	/* set clock in spl only */
#ifdef CONFIG_SPL_BUILD
	int ret, tmp;

	sca200v100_qspi_set_clk(aq, aq->clk_rate);
	printf("qspi clock: %dMhz\n", aq->clk_rate / 1000000);
	/* must be sync to the clock, to train only timing-cfg is not setted in dtb */
	if (aq->timing >= 0) {
		sca200v100_qspi_set_timing_cfg(aq, aq->timing);
	} else {
		/* use flash cs0, nor flash read the first 4 byte in romcode, nand flash
		 * read the first 4 byte in spl0, both are address 0
		 */
		ret = sca200v100_qspi_training(aq);
		if (ret) {
			pr_err("qspi training failed, maybe the flash is empty now, set the default(clock: 25M, timing: 0), otherwise check the config or hardware please\n");
			/* maybe use romcode clock and timing, but use 25M, timing 0 now
			 * it must work
			 */
			sca200v100_qspi_set_clk(aq, 25000000);
			sca200v100_qspi_set_timing_cfg(aq, 0);
		}
	}
#ifdef CONFIG_SMARTCHIP_SCA200V100
	/* TODO: for sca200v200 ahb dma access sram */
#define AHB_DMA_ACCESS_SRAM0    0x01172448
#define AHB_DMA_ACCESS_SRAM1    0x01172468
	/* enable ahb dma access sram */
	tmp = readl(AHB_DMA_ACCESS_SRAM0);
	tmp |= 0xff;
	writel(tmp, AHB_DMA_ACCESS_SRAM0);
	tmp = readl(AHB_DMA_ACCESS_SRAM1);
	tmp |= 0xff;
	writel(tmp, AHB_DMA_ACCESS_SRAM1);
#endif
#elif defined(CONFIG_SMARTCHIP_SCA200V200)
#define SRAM_NONSEC0    0x00402c68
#define SRAM_NONSEC1    0x00402c88
#define SRAM_NONSEC2    0x00402ca8
#define SRAM_NONSEC3    0x00402cc8
	writel(0xf, SRAM_NONSEC0);
	writel(0xf, SRAM_NONSEC1);
	writel(0xf, SRAM_NONSEC2);
	writel(0xf, SRAM_NONSEC3);
#endif

	return 0;
}

/* do nothing, maybe remove it later */
static int sca200v100_spi_set_speed(struct udevice *bus, uint hz)
{
	return 0;
}

static int sca200v100_spi_set_mode(struct udevice *bus, uint mode)
{
	return 0;
}

static int sca200v100_spi_adjust_op_size(struct spi_slave *spi,
    struct spi_mem_op *op)
{
	if (op->data.nbytes > QSPI_DMA_BLOCK_MAX * 4)
		op->data.nbytes = QSPI_DMA_BLOCK_MAX * 4;

	return 0;
}

static int sca200v100_qspi_opcode_is_dtr(unsigned char opcode)
{
	switch (opcode) {
	case SPINOR_OP_READ_1_1_1_DTR:
	case SPINOR_OP_READ_1_1_1_DTR_4B:
	case SPINOR_OP_READ_1_2_2_DTR:
	case SPINOR_OP_READ_1_2_2_DTR_4B:
	case SPINOR_OP_READ_1_4_4_DTR:
	case SPINOR_OP_READ_1_4_4_DTR_4B:
		return 1;
	default:
		return 0;
	}
}

static int sca200v100_spi_mem_exec_op(struct spi_slave *spi,
    const struct spi_mem_op *op)
{
	int ret, mod;
	struct udevice *bus = spi->dev->parent;
	struct sca200v100_qspi *aq = bus->platdata;
	struct dm_spi_slave_platdata *plat = dev_get_parent_platdata(spi->dev);
	unsigned long timeout;
	unsigned int tmp;

	if (plat->cs == 2)
		plat->cs = 0;

	if (plat->cs != aq->cur_cs) {
		sca200v100_qspi_set_flash_cs(aq, plat->cs);
		if (aq->dbg->dbg_flag & DBG_CS)
			pr_info("change flash cs to %d\n", plat->cs);
		aq->cur_cs = plat->cs;
	}

	if (sca200v100_qspi_opcode_is_dtr(op->cmd.opcode) != aq->cur_dtr) {
		if (aq->cur_dtr)
			sca200v100_qspi_dtr_disable(aq);
		else
			sca200v100_qspi_dtr_enable(aq);

		if (aq->dbg->dbg_flag & DBG_DTR)
			pr_info("change dtr to %d\n", !aq->cur_dtr);
		aq->cur_dtr = !aq->cur_dtr;
	}

	ret = sca200v100_qspi_send_cmd(aq, op);
	if (ret) {
		pr_err("%s [%d]: qspi send command failed\n", __func__, __LINE__);
		goto out;
	}

#define DMA_TRANS_MIN   16000000
	if (op->data.dir == SPI_MEM_DATA_IN) {
		if (op->data.nbytes < DMA_TRANS_MIN) {
			ret = sca200v100_qspi_read_fifo(aq, op->data.buf.in, op->data.nbytes);
			if (ret)
				goto out;
		} else {
			mod = op->data.nbytes % 4;
			sca200v100_qspi_dma_enable(aq);
			invalidate_dcache_range((unsigned long)op->data.buf.in, (unsigned long)op->data.buf.in + roundup(op->data.nbytes,
			        ARCH_DMA_MINALIGN));
			ret = sca200v100_qspi_dma_read(aq, (unsigned long)op->data.buf.in, op->data.nbytes - mod);
			sca200v100_qspi_dma_disable(aq);
			if (ret)
				goto out;
			if (mod) {
				ret = sca200v100_qspi_read_fifo(aq, op->data.buf.in + op->data.nbytes - mod, mod);
				if (ret)
					goto out;
			}

		}

		tmp = readl(aq->regs + QSPI_FIFO_ST);
		/* rfifo should be empty */
		if (!(tmp & QSPI_FIFO_ST_RFIFO_EMPTY)) {
			pr_err("rfifo is not empty\n");
			ret = -1;
			goto out;
		}

		if (aq->dbg->dbg_flag & DBG_PRINT_RFIFO && op->data.nbytes >= 4)
			pr_info("rfifo: %#x\n", *(unsigned int *)op->data.buf.in);
	} else if (op->data.dir == SPI_MEM_DATA_OUT) {
		if (op->data.nbytes < DMA_TRANS_MIN) {
			ret = sca200v100_qspi_write_fifo(aq, (unsigned char *)op->data.buf.out, op->data.nbytes);
			if (ret)
				goto out;
		} else {
			mod = op->data.nbytes % 4;
			flush_dcache_range((unsigned long)op->data.buf.out, (unsigned long)op->data.buf.out + roundup(op->data.nbytes,
			        ARCH_DMA_MINALIGN));
			sca200v100_qspi_dma_enable(aq);
			ret = sca200v100_qspi_dma_write(aq, (unsigned long)op->data.buf.out, op->data.nbytes - mod);
			sca200v100_qspi_dma_disable(aq);
			if (ret)
				goto out;
			if (mod) {
				ret = sca200v100_qspi_write_fifo(aq, (unsigned char *)op->data.buf.out + op->data.nbytes - mod, mod);
				if (ret)
					goto out;
			}
		}
		/* till wfifo empty*/
		timeout = get_ticks() + usec2ticks(QSPI_TIMEOUT_MS * 1000);
		do {
			tmp = readl(aq->regs + QSPI_FIFO_ST);
			if (get_qspi_wfifo_freew(tmp) == QSPI_FIFO_SIZE)
				break;
			if (time_after((unsigned long)get_ticks(), timeout)) {
				pr_err("wfifo is not empty\n");
				ret = -1;
				goto out;
			}
		} while (1);
	}

	return 0;
out:
	sca200v100_qspi_registers_dump(aq);
	return ret;
}

static int sca200v100_spi_ofdata_to_platdata(struct udevice *bus)
{
	struct sca200v100_qspi *aq = bus->platdata;

	aq->regs = (void *)devfdt_get_addr_index(bus, 0);
	aq->cgu_base = (void *)devfdt_get_addr_index(bus, 1);
	aq->dma_base = (void *)devfdt_get_addr_index(bus, 2);
	aq->hs_sel_base = (void *)devfdt_get_addr_index(bus, 3);

	aq->clk_rate = dev_read_u32_default(bus, "clock-frequency", 25000000);
	aq->dma_ch = dev_read_u32_default(bus, "dma-channel", 0);
	/* if timing-cfg is exsit, use it, otherwise training */
	aq->timing = dev_read_u32_default(bus, "timing-cfg", -1);

	return 0;
}

static int sca200v100_spi_probe(struct udevice *bus)
{
	struct sca200v100_qspi *aq = bus->platdata;

	sca200v100_qspi_init(aq);

	return 0;
}

static int sca200v100_spi_remove(struct udevice *dev)
{
	return 0;
}

static const struct spi_controller_mem_ops sca200v100_spi_mem_ops = {
	.adjust_op_size = sca200v100_spi_adjust_op_size,
	.exec_op = sca200v100_spi_mem_exec_op,
};

static const struct dm_spi_ops sca200v100_spi_ops = {
	.set_speed  = sca200v100_spi_set_speed,
	.set_mode   = sca200v100_spi_set_mode,
	.mem_ops    = &sca200v100_spi_mem_ops,
};

static const struct udevice_id sca200v100_spi_ids[] = {
	{ .compatible = "smartchip,sca200v100-qspi" },
	{ }
};

U_BOOT_DRIVER(sca200v100_qspi) = {
	.name = "sca200v100_qspi",
	.id = UCLASS_SPI,
	.of_match = sca200v100_spi_ids,
	.ops = &sca200v100_spi_ops,
	.ofdata_to_platdata = sca200v100_spi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct sca200v100_qspi) + sizeof(struct sca200v100_qspi_debug_info),
	.probe = sca200v100_spi_probe,
	.remove = sca200v100_spi_remove,
};
