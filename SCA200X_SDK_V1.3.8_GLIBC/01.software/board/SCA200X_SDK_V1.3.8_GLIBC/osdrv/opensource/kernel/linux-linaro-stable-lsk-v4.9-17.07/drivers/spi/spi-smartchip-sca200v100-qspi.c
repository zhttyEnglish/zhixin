// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021
 * SmartChip Corporation <www.sgitg.sgcc.com.cn>
 *
 * TODO: dma + interrupt
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/sizes.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/mtd/spi-nor.h>

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
#define QSPI_THRESHOLD_SIZE         0x20

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
/* cgu for qspi clock setting */
#define CGU_QSPI_SEL_400M           BIT(1)
#define CGU_QSPI_CLOCK_ON           BIT(3)
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
	unsigned int dbg_flag;
	unsigned int pio;

	struct dentry   *dfs_dir;
	struct dentry   *dfs_dbg_flag;
	struct dentry   *dfs_pio;
};

struct sca200v100_qspi {
	void *regs; /* Qspi register registers */
	void *cgu_base; /* cgu register base, only qspi related clock config */
	void *dma_base; /* ahb dma register base */
	void *hs_sel_base; /* ahb dma hardshaking select register base */
	u32 clk_rate;
	int dma_ch;
	int irq;
	int timing;
	int cur_die; /* flash has two dies */
	int cur_cs;
	int cur_dtr; /* 0: str; 1: dtr */
	phys_addr_t qspi_phy;
	struct mutex lock;
	struct device *dev;
	/* interrupt related */
	struct tasklet_struct tasklet;
	struct completion completion;
	void *buf; /* for pio temp buf */
	unsigned int rwlen; /* buf len */
	/* dma related */
	dma_addr_t dma_buffer;
	size_t dmabuf_len;
	u8 *dmabuf;

	struct sca200v100_qspi_debug_info *dbg;
};

/* ahb dma related codes */
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
		writel(aq->qspi_phy + QSPI_RFIFO_ENTRY, aq->dma_base + DMAC_SAR(aq->dma_ch));
	} else {
		ahb_dmac_set_hs(aq, 0xff, DMA_HS_QSPI_TX);
		writel(aq->qspi_phy + QSPI_WFIFO_ENTRY, aq->dma_base + DMAC_DAR(aq->dma_ch));
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

	timeout = jiffies + msecs_to_jiffies(QSPI_TIMEOUT_MS);
	do {
		tmp = readl(aq->dma_base + DMAC_RawBlock);
		tmp &= (1 << aq->dma_ch);
		if (time_after(jiffies, timeout)) {
			pr_err("%s [%d]: waiting for dma transfer timeout\n", __func__, __LINE__);
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
		BUG();
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

static void sca200v100_qspi_set_spi_mode(struct sca200v100_qspi *aq, int mod)
{
	unsigned int tmp;

	tmp = readl(aq->regs + QSPI_GCFG);
	if (mod == 0)
		tmp |= QSPI_GCFG_CLK_POLAR;
	else
		tmp &= ~QSPI_GCFG_CLK_POLAR;
	writel(tmp, aq->regs + QSPI_GCFG);
}

static void sca200v100_qspi_set_flash_cs(struct sca200v100_qspi *aq, int cs)
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

static void sca200v100_qspi_clear_fifo(struct sca200v100_qspi *aq)
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

static void sca200v100_qspi_int_enable(struct sca200v100_qspi *aq)
{
	writel(QSPI_INT_EN_DONE_INT_EN, aq->regs + QSPI_INT_EN);
}

static void sca200v100_qspi_int_disable(struct sca200v100_qspi *aq)
{
	writel(0, aq->regs + QSPI_INT_EN);
}

static void sca200v100_qspi_clear_interrupt(struct sca200v100_qspi *aq)
{
	/* write 1 clear interrupt */
	writel(1, aq->regs + QSPI_INT_ST);
}

static void sca200v100_qspi_dtr_enable(struct sca200v100_qspi *aq)
{
	int tmp;

	tmp = readl(aq->regs + QSPI_GCFG);
	tmp |= QSPI_GCFG_DTR;
	writel(tmp, aq->regs + QSPI_GCFG);
}

static void sca200v100_qspi_dtr_disable(struct sca200v100_qspi *aq)
{
	int tmp;

	tmp = readl(aq->regs + QSPI_GCFG);
	tmp &= ~QSPI_GCFG_DTR;
	writel(tmp, aq->regs + QSPI_GCFG);
}

/* TODO: 133M can only be used by bootloader */
static void sca200v100_qspi_set_clk(struct sca200v100_qspi *aq, unsigned int rate)
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

static int sca200v100_qspi_waiting_rfifo_not_empty(struct sca200v100_qspi *aq)
{
	unsigned int tmp;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(QSPI_TIMEOUT_MS);
	do {
		tmp = readl(aq->regs + QSPI_FIFO_ST);
		if (time_after(jiffies, timeout)) {
			pr_err("%s [%d]: waiting for read fifo timeout\n", __func__, __LINE__);
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
			div--;
		}
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

	timeout = jiffies + msecs_to_jiffies(QSPI_TIMEOUT_MS);
	do {
		tmp = readl(aq->regs + QSPI_STATUS);
		if (time_after(jiffies, timeout)) {
			pr_err("%s [%d]: waiting for sending command timeout(qspi is busy)\n", __func__, __LINE__);
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

	aq->cur_die = 0;
	aq->cur_cs = 0;
	aq->cur_dtr = 0;

	return 0;
}

#if 0
/* TODO */
static void select_chip(struct spi_nor *nor, int chip)
{
}

static void nor_select_chip(struct spi_nor *nor, loff_t from)
{
	struct sca200v100_qspi *aq;
	uint64_t size;

	aq = nor->priv;

	if (nor->flags & SNOR_F_HAS_TWO_DIES) {
		size = nor->mtd.size / 2;
		if (from < size && aq->cur_die == 1) {
			select_chip(nor, 0);
		} else if (from >= size && aq->cur_die == 0) {
			select_chip(nor, 1);
		}
	}
}
#endif

static void sca200v100_qspi_tasklet(unsigned long data)
{
	int ret;
	struct sca200v100_qspi *aq = (struct sca200v100_qspi *)data;

	if (aq->dbg->dbg_flag & DBG_TASKLET)
		pr_info("%s [%d]: enter tasklet\n", __func__, __LINE__);

	ret = sca200v100_qspi_read_fifo(aq, aq->buf, aq->rwlen);
	if (ret)
		BUG();
	complete(&aq->completion);
}

static irqreturn_t sca200v100_qspi_interrupt(int irq, void *dev_id)
{
	struct sca200v100_qspi *aq = dev_id;
	int status;

	if (aq->dbg->dbg_flag & DBG_INTERRUPT)
		pr_info("%s [%d]: enter interrupt\n", __func__, __LINE__);

	status = readl(aq->regs + QSPI_INT_ST);
	if (status & QSPI_INT_ST_DONE_INT_ST)
		sca200v100_qspi_clear_interrupt(aq);
	else {
		pr_info("unexpected qspi interrupt\n");
		return IRQ_NONE;
	}

	tasklet_schedule(&aq->tasklet);

	return IRQ_HANDLED;
}

static int sca200v100_qspi_adjust_op_size(struct spi_mem *mem, struct spi_mem_op *op)
{
	struct sca200v100_qspi *aq = spi_master_get_devdata(mem->spi->master);

	if (op->data.dir == SPI_MEM_DATA_IN) {
		if (aq->irq && aq->dbg->pio) {
			if (op->data.nbytes > QSPI_FIFO_SIZE * 3)
				op->data.nbytes = QSPI_FIFO_SIZE * 3;
		} else {
			if (op->data.nbytes > aq->dmabuf_len)
				op->data.nbytes = aq->dmabuf_len;
		}
	} else {
		if (!aq->irq || !aq->dbg->pio) {
			if (op->data.nbytes > aq->dmabuf_len)
				op->data.nbytes = aq->dmabuf_len;
		}
	}

	return 0;
}

static bool sca200v100_qspi_supports_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
	return true;
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

static int sca200v100_qspi_exec_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
	int ret, mod = 0;
	struct sca200v100_qspi *aq = spi_master_get_devdata(mem->spi->master);;
	struct spi_device *spi = mem->spi;
	unsigned int tmp;
	unsigned long timeout;

	/* TODO: set clock, maybe need set timing after change clock */

#define SPI_NAND_CS0    1
#define SPI_NOR_CS1     2
#define SPI_NAND_CS1    3
	/* workaround: based on dts, cs1 in dts is actually cs0, cs2 and cs3 in dts
	 * is actually cs1
	 * */
	if (spi->chip_select == SPI_NAND_CS0)
		spi->chip_select = 0;
	if (spi->chip_select == SPI_NOR_CS1 || spi->chip_select == SPI_NAND_CS1)
		spi->chip_select = 1;

	if (spi->chip_select != aq->cur_cs) {
		sca200v100_qspi_set_flash_cs(aq, spi->chip_select);
		if (aq->dbg->dbg_flag & DBG_CS)
			pr_info("change flash cs to %d\n", spi->chip_select);
		aq->cur_cs = spi->chip_select;
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

	if (aq->dbg->dbg_flag & DBG_LOCK)
		pr_info("lock\n");

	mutex_lock(&aq->lock);

	/* use pio when data len less than 16 byte*/
#define DMA_TRANS_MIN   16
	if (op->data.dir == SPI_MEM_DATA_IN) {
		if (op->data.nbytes < DMA_TRANS_MIN) {
			ret = sca200v100_qspi_send_cmd(aq, op);
			if (ret)
				goto out;

			ret = sca200v100_qspi_read_fifo(aq, op->data.buf.in, op->data.nbytes);
			if (ret)
				goto out;
		} else {
			/* pio + qspi interrupt */
			if (aq->irq && aq->dbg->pio) {
				sca200v100_qspi_clear_interrupt(aq);
				sca200v100_qspi_int_enable(aq);
				aq->buf = op->data.buf.in;
				aq->rwlen = op->data.nbytes;
				reinit_completion(&aq->completion);
			} else {
				sca200v100_qspi_dma_enable(aq);
				/* dma only support words aligned */
				mod = op->data.nbytes % 4;
			}

			ret = sca200v100_qspi_send_cmd(aq, op);
			if (ret)
				goto out;

			if (aq->irq && aq->dbg->pio) {
				ret = wait_for_completion_timeout(&aq->completion, msecs_to_jiffies(QSPI_TIMEOUT_MS));
				if (ret == 0) {
					pr_err("%s [%d]: qspi read data timeout\n", __func__, __LINE__);
					ret = -ETIMEDOUT;
					goto out;
				}
			} else {
				/* TODO: read times */
				ret = sca200v100_qspi_dma_read(aq, (unsigned int)aq->dma_buffer, op->data.nbytes - mod);
				if (ret)
					goto out;
				memcpy(op->data.buf.in, aq->dmabuf, op->data.nbytes - mod);
			}

			if (aq->irq && aq->dbg->pio)
				sca200v100_qspi_int_disable(aq);
			else
				sca200v100_qspi_dma_disable(aq);

			/* do it after dma disable */
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
	} else if (op->data.dir == SPI_MEM_DATA_OUT) {
		if ((aq->irq && aq->dbg->pio) || (op->data.nbytes < DMA_TRANS_MIN)) {
			ret = sca200v100_qspi_send_cmd(aq, op);
			if (ret)
				goto out;
			ret = sca200v100_qspi_write_fifo(aq, (unsigned char *)op->data.buf.out, op->data.nbytes);
			if (ret)
				goto out;
		} else {
			mod = op->data.nbytes % 4;
			sca200v100_qspi_dma_enable(aq);
			memcpy(aq->dmabuf, (unsigned char *)op->data.buf.out, op->data.nbytes - mod);
			ret = sca200v100_qspi_send_cmd(aq, op);
			if (ret)
				goto out;
			ret = sca200v100_qspi_dma_write(aq, (unsigned int)aq->dma_buffer, op->data.nbytes - mod);
			if (ret)
				goto out;
			sca200v100_qspi_dma_disable(aq);

			if (mod) {
				ret = sca200v100_qspi_write_fifo(aq, (unsigned char *)op->data.buf.out + op->data.nbytes - mod, mod);
				if (ret)
					goto out;
			}
		}

		/* till wfifo empty*/
		timeout = jiffies + msecs_to_jiffies(QSPI_TIMEOUT_MS);
		do {
			tmp = readl(aq->regs + QSPI_FIFO_ST);
			if (get_qspi_wfifo_freew(tmp) == QSPI_FIFO_SIZE)
				break;
			if (time_after(jiffies, timeout)) {
				pr_err("wfifo is not empty\n");
				ret = -1;
				goto out;
			}
		} while (1);
	} else {
		ret = sca200v100_qspi_send_cmd(aq, op);
		if (ret)
			goto out;
	}

	if (aq->dbg->dbg_flag & DBG_LOCK)
		pr_info("unlock\n");

	mutex_unlock(&aq->lock);

	return 0;
out:
	if (aq->dbg->dbg_flag & DBG_LOCK)
		pr_info("unlock\n");
	/* don't care use dma or interrupt, disable all */
	sca200v100_qspi_int_disable(aq);
	sca200v100_qspi_dma_disable(aq);
	mutex_unlock(&aq->lock);
	sca200v100_qspi_registers_dump(aq);
	return ret;
}

static const char *sca200v100_qspi_get_name(struct spi_mem *mem)
{
	return "smartchip_qspi";
}

static int sca200v100_qspi_fdt_parse(struct platform_device *pdev, struct sca200v100_qspi *aq)
{
	int ret;
	struct resource *res;

	/* Map the registers */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "qspi_base");
	aq->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(aq->regs)) {
		dev_err(&pdev->dev, "missing qspi registers\n");
		ret = PTR_ERR(aq->regs);
		goto exit;
	}
	aq->qspi_phy = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cgu_base");
	aq->cgu_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(aq->cgu_base)) {
		dev_err(&pdev->dev, "missing cgu registers\n");
		ret = PTR_ERR(aq->cgu_base);
		goto exit;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dma_base");
	aq->dma_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(aq->dma_base)) {
		dev_err(&pdev->dev, "missing ahb dma registers\n");
		ret = PTR_ERR(aq->dma_base);
		goto exit;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "hs_sel_base");
	aq->hs_sel_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(aq->hs_sel_base)) {
		dev_err(&pdev->dev, "missing ahb dma handshaking select registers\n");
		ret = PTR_ERR(aq->hs_sel_base);
		goto exit;
	}

	aq->irq = platform_get_irq(pdev, 0);
	if (aq->irq < 0)
		aq->irq = 0; /* use dma */

	ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency", &aq->clk_rate);
	if (ret) {
		dev_err(&pdev->dev, "missing clock-frequency\n");
		goto exit;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "dma-channel", &aq->clk_rate);
	if (ret) {
		dev_err(&pdev->dev, "missing dma-channel\n");
		goto exit;
	}

	/* if dts no timing-cfg, use 0 */
	of_property_read_u32(pdev->dev.of_node, "timing-cfg", &aq->timing);

	return 0;
exit:
	return ret;
}

static int sca200v100_qspi_debugfs_init(struct sca200v100_qspi *aq);
static void sca200v100_qspi_debugfs_exit(struct sca200v100_qspi *aq);

static const struct spi_master_mem_ops sca200v100_qspi_mem_ops = {
	.adjust_op_size = sca200v100_qspi_adjust_op_size,
	.supports_op = sca200v100_qspi_supports_op,
	.exec_op = sca200v100_qspi_exec_op,
	.get_name = sca200v100_qspi_get_name,
};

static int sca200v100_qspi_probe(struct platform_device *pdev)
{
	struct spi_master *ctlr;
	struct sca200v100_qspi *aq;
	int ret = 0;

	ctlr = spi_alloc_master(&pdev->dev, sizeof(*aq));
	if (!ctlr)
		return -ENOMEM;

	ctlr->mode_bits = SPI_RX_DUAL | SPI_RX_QUAD |
	    SPI_TX_DUAL | SPI_TX_QUAD;

	aq = spi_master_get_devdata(ctlr);
	aq->dev = &pdev->dev;

	platform_set_drvdata(pdev, aq);

	ret = sca200v100_qspi_fdt_parse(pdev, aq);
	if (ret)
		goto exit;

	ret = sca200v100_qspi_debugfs_init(aq);
	if (ret) {
		dev_err(&pdev->dev, "sca200v100 qspi debugfs init failed\n");
		goto exit;
	}

	/* use pio + interrupt */
	if (aq->irq) {
		init_completion(&aq->completion);
		tasklet_init(&aq->tasklet, sca200v100_qspi_tasklet, (unsigned long)aq);
		ret = devm_request_irq(&pdev->dev, aq->irq, sca200v100_qspi_interrupt, IRQF_SHARED, pdev->name, aq);
		if (ret)
			goto exit;
	}

	aq->dmabuf_len = PAGE_SIZE;
	aq->dmabuf = dmam_alloc_coherent(aq->dev,
	        aq->dmabuf_len, &(aq->dma_buffer), GFP_KERNEL);
	if (!aq->dmabuf) {
		dev_err(&pdev->dev, "dmam_alloc_coherent failed\n");
		ret = -ENOMEM;
		goto exit;
	}

	mutex_init(&aq->lock);

	ctlr->bus_num = pdev->id;
	ctlr->num_chipselect = 3;
	ctlr->mem_ops = &sca200v100_qspi_mem_ops;

	sca200v100_qspi_init(aq);

	ctlr->dev.of_node = pdev->dev.of_node;

	ret = devm_spi_register_master(&pdev->dev, ctlr);
	if (ret)
		goto exit1;
	return 0;

exit1:
	mutex_destroy(&aq->lock);
	devm_kfree(&pdev->dev, aq->dmabuf);
exit:
	spi_master_put(ctlr);

	dev_err(&pdev->dev, "SmartChip qspi probe failed\n");
	return ret;
}

static int sca200v100_qspi_remove(struct platform_device *pdev)
{
	struct sca200v100_qspi *aq = platform_get_drvdata(pdev);

	dmam_free_coherent(aq->dev, aq->dmabuf_len, aq->dmabuf, aq->dma_buffer);
	sca200v100_qspi_debugfs_exit(aq);

	return 0;
}

static const struct of_device_id sca200v100_qspi_dt_ids[] = {
	{ .compatible = "smartchip,sca200v100-qspi" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, sca200v100_qspi_dt_ids);

static struct platform_driver sca200v100_qspi_driver = {
	.driver = {
		.name   = "smartchip,sca200v100-qspi",
		.of_match_table = sca200v100_qspi_dt_ids,
	},
	.probe      = sca200v100_qspi_probe,
	.remove     = sca200v100_qspi_remove,
};

module_platform_driver(sca200v100_qspi_driver);

static ssize_t dfs_file_read(struct file *file, char __user *user_buf,
    size_t count, loff_t *ppos)
{
	char buf[8];

	struct sca200v100_qspi *aq = (struct sca200v100_qspi *)file->private_data;
	struct sca200v100_qspi_debug_info *d = aq->dbg;
	struct dentry *dent = file->f_path.dentry;

	if (dent == d->dfs_dbg_flag) {
		snprintf(buf, sizeof(buf), "%u\n", d->dbg_flag);
		count = simple_read_from_buffer(user_buf, count, ppos,
		        buf, strlen(buf));
	} else if (dent == d->dfs_pio) {
		snprintf(buf, sizeof(buf), "%u\n", d->pio);
		count = simple_read_from_buffer(user_buf, count, ppos,
		        buf, strlen(buf));
	} else {
		count = -EINVAL;
	}

	return count;
}

static ssize_t dfs_file_write(struct file *file, const char __user *u,
    size_t count, loff_t *ppos)
{
	struct sca200v100_qspi *aq = (struct sca200v100_qspi *)file->private_data;
	struct sca200v100_qspi_debug_info *d = aq->dbg;
	struct dentry *dent = file->f_path.dentry;
	unsigned long value;
	int err;

	err = kstrtoul_from_user(u, count, 0, &value);
	if (err)
		return err;

	if (dent == d->dfs_dbg_flag) {
		d->dbg_flag = value;
		pr_err("dbg: set dbg_flag 0x%lx\n", value);
	} else if (dent == d->dfs_pio) {
		d->pio = value;
		pr_err("dbg: set pio 0x%lx\n", value);
	}

	return count;
}

static const struct file_operations sca200v100_qspi_dfs_fops = {
	.read   = dfs_file_read,
	.write  = dfs_file_write,
	.open   = simple_open,
	.llseek = no_llseek,
	.owner  = THIS_MODULE,
};

static int sca200v100_qspi_debugfs_init(struct sca200v100_qspi *aq)
{
	int err;
	const char *fname;
	struct dentry *dent;

	if (!IS_ENABLED(CONFIG_DEBUG_FS))
		return 0;

	aq->dbg = devm_kzalloc(aq->dev, sizeof(struct sca200v100_qspi_debug_info), GFP_KERNEL);
	if (IS_ERR(aq->dbg))
		return PTR_ERR(aq->dbg);

	aq->dbg->dfs_dir = debugfs_create_dir("sca200v100_qspi", NULL);
	if (IS_ERR_OR_NULL(aq->dbg->dfs_dir)) {
		err = aq->dbg->dfs_dir ? PTR_ERR(aq->dbg->dfs_dir) : -ENODEV;

		pr_err("qspi error: cannot create \"sca200v100_qspi\" debugfs directory, error %d\n",
		    err);
		return err;
	}

	fname = "dbg_flag";
	dent = debugfs_create_file(fname, S_IWUSR, aq->dbg->dfs_dir, (void *)aq,
	        &sca200v100_qspi_dfs_fops);
	if (IS_ERR_OR_NULL(dent))
		goto out_remove;
	aq->dbg->dfs_dbg_flag = dent;

	fname = "pio";
	dent = debugfs_create_file(fname, S_IWUSR, aq->dbg->dfs_dir, (void *)aq,
	        &sca200v100_qspi_dfs_fops);
	if (IS_ERR_OR_NULL(dent))
		goto out_remove;
	aq->dbg->dfs_pio = dent;

	aq->dbg->dbg_flag = 0;
	aq->dbg->pio = 0;

	return 0;

out_remove:
	debugfs_remove_recursive(aq->dbg->dfs_dir);
	err = dent ? PTR_ERR(dent) : -ENODEV;

	return err;
}

static void sca200v100_qspi_debugfs_exit(struct sca200v100_qspi *aq)
{
	if (IS_ENABLED(CONFIG_DEBUG_FS) && aq->dbg->dfs_dir)
		debugfs_remove_recursive(aq->dbg->dfs_dir);
}

MODULE_AUTHOR("SmartChip");
MODULE_DESCRIPTION("sca200v100 QSPI Controller driver");
MODULE_LICENSE("GPL v2");

