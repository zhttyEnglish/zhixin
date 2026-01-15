/*
 * SmartChip ahb dma driver
 */

#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include "smartchip-qspi.h"
#include "sc_nor_dma.h"

extern void sc_nor_dma_write(struct sc_nor_host *host, unsigned int value, unsigned int reg);

extern unsigned int sc_nor_dma_read(struct sc_nor_host *host, unsigned int reg);

extern void sc_nor_dma_lowbit_write(struct sc_nor_host *host, unsigned int value);

extern unsigned int sc_nor_dma_lowbit_read(struct sc_nor_host *host);

#define MSK(m, n)   (((1 << (m)) - 1) << (n))

typedef enum {
	TR_WIDTH_8 = 0,
	TR_WIDTH_16,
	TR_WIDTH_32,
	TR_WIDTH_64,
	TR_WIDTH_128,
	TR_WIDTH_256,
} tr_width_e;

typedef enum {
	M2M_DMAC = 0,
	M2P_DMAC,
	P2M_DMAC,
	P2P_DMAC,
	P2M_P,
	P2P_SP,
	M2P_P,
	P2P_DP,
} tt_fc_e;

typedef enum {
	MSIZE_1 = 0, //1byte
	MSIZE_4,
	MSIZE_8,
	MSIZE_16,
	MSIZE_32,
	MSIZE_64,
	MSIZE_128,
	MSIZE_256,
} msize_e;

typedef enum {
	AHB_MST_1 = 0,
	AHB_MST_2,
	AHB_MST_3,
	AHB_MST_4,
} master_sel_e;

typedef enum {
	INCREMENT = 0,
	DECREMENT = 1,
	NO_CHANGE = 2,
} addr_inc_e;

typedef enum {
	HW_HDSK = 0,
	SW_HDSK,
} hs_type_e;

typedef enum {
	/* Memory 0 */
	AHB_DMA_HS_MEMORY = 0,

	/* 13 selected */
	AHB_DMA_HS_I2C1_RX,
	AHB_DMA_HS_I2S2_TX,

	/* 12 selected */
	AHB_DMA_HS_I2C1_TX,
	AHB_DMA_HS_I2S2_RX,

	/* 9  selected */
	AHB_DMA_HS_I2S3_RX,
	AHB_DMA_HS_SPI_SLV1_TX,

	/* 8  selected */
	AHB_DMA_HS_I2S3_TX,
	AHB_DMA_HS_SPI_SLV1_RX,

	/* 3  selected */
	AHB_DMA_HS_UART2_RX,
	AHB_DMA_HS_I2S1_TX,

	/* 2  selected */
	AHB_DMA_HS_UART2_TX,
	AHB_DMA_HS_I2S1_RX,

	/* 15 selected */
	AHB_DMA_HS_SPI_SLV0_RX,
	AHB_DMA_HS_UART3_TX,
	AHB_DMA_HS_UART4_TX,
	AHB_DMA_HS_SPIMST1_TX,
	AHB_DMA_HS_SPIMST3_TX,
	AHB_DMA_HS_I2C2_TX,
	AHB_DMA_HS_I2C3_TX,
	AHB_DMA_HS_I2C3_TX_DUP,
	AHB_DMA_HS_I2S0_TX,

	/* 14 selected */
	AHB_DMA_HS_SPI_SLV0_TX,
	AHB_DMA_HS_UART3_RX,
	AHB_DMA_HS_UART4_RX,
	AHB_DMA_HS_SPIMST1_RX,
	AHB_DMA_HS_SPIMST3_RX,
	AHB_DMA_HS_I2C2_RX,
	AHB_DMA_HS_I2C3_RX,
	AHB_DMA_HS_I2C3_RX_DUP,
	AHB_DMA_HS_I2S0_RX,

	AHB_DMA_HS_NUM,
} hs_ip_e;

typedef struct {
	addr_inc_e src_inc;
	addr_inc_e dst_inc;
	master_sel_e src_mst;
	master_sel_e dst_mst;
	tr_width_e src_tr_len;
	tr_width_e dst_tr_len;
	msize_e src_burst_len;
	msize_e dst_burst_len;
	tt_fc_e flow_ctrl;
} single_block_ctl_t;

typedef struct {
	unsigned int dst_hs; //hardwarehandshake interface
	unsigned int src_hs;
	hs_type_e hs_src_sel;
	hs_type_e hs_dst_sel;
} hand_shake_cfg_t;

#define CFG_L_RELOAD_DST_LSB    31
#define CFG_L_RELOAD_DST_LEN    1

#define CFG_L_RELOAD_SRC_LSB    30
#define CFG_L_RELOAD_SRC_LEN    1

#define CFG_L_HS_SEL_DST_LSB    10
#define CFG_L_HS_SEL_DST_LEN    1

#define CFG_L_HS_SEL_SRC_LSB    11
#define CFG_L_HS_SEL_SRC_LEN    1

#define CFG_H_SRC_PER_LSB   7
#define CFG_H_SRC_PER_LEN   4

#define CFG_H_DST_PER_LSB   11
#define CFG_H_DST_PER_LEN   4

#define CTL_L_DMS_LSB   23
#define CTL_L_DMS_LEN   2

#define CTL_L_SMS_LSB   25
#define CTL_L_SMS_LEN   2

#define CTL_L_SRC_MSIZE_LSB 14
#define CTL_L_SRC_MSIZE_LEN 3

#define CTL_L_DST_MSIZE_LSB 11
#define CTL_L_DST_MSIZE_LEN 3

#define CTL_L_DINC_LSB  7
#define CTL_L_DINC_LEN  2

#define CTL_L_SINC_LSB  9
#define CTL_L_SINC_LEN  2

#define CTL_L_SRC_TR_WIDTH_LSB  4
#define CTL_L_SRC_TR_WIDTH_LEN  3

#define CTL_L_DST_TR_WIDTH_LSB  1
#define CTL_L_DST_TR_WIDTH_LEN  3

#define CTL_L_TT_FC_LSB 20
#define CTL_L_TT_FC_LEN 3

#define CTL_L_INT_EN_LSB 0
#define CTL_L_INT_EN_LEN 1

#define CTL_L_LLP_DST_EN_LSB 27
#define CTL_L_LLP_DST_EN_LEN 1

#define CTL_L_LLP_SRC_EN_LSB 28
#define CTL_L_LLP_SRC_EN_LEN 1

#define ADDR_SAR_L(x)       ((0x58 * x) + 0x000)
#define ADDR_SAR_H(x)       ((0x58 * x) + 0x004)
#define ADDR_DAR_L(x)       ((0x58 * x) + 0x008)
#define ADDR_DAR_H(x)       ((0x58 * x) + 0x00c)
#define ADDR_LLP_L(x)       ((0x58 * x) + 0x010)
#define ADDR_LLP_H(x)       ((0x58 * x) + 0x014)
#define ADDR_CTL_L(x)       ((0x58 * x) + 0x018)
#define ADDR_CTL_H(x)       ((0x58 * x) + 0x01c)
#define ADDR_STAT_L(x)      ((0x58 * x) + 0x020)
#define ADDR_STAT_H(x)      ((0x58 * x) + 0x024)
#define DSTAT_L(x)          ((0x58 * x) + 0x028)
#define DSTAT_H(x)          ((0x58 * x) + 0x02c)
#define SSTATAR_L(x)        ((0x58 * x) + 0x030)
#define SSTATAR_H(x)        ((0x58 * x) + 0x034)
#define DSTATAR_L(x)        ((0x58 * x) + 0x038)
#define DSTATAR_H(x)        ((0x58 * x) + 0x03c)
#define ADDR_CFG_L(x)       ((0x58 * x) + 0x040)
#define ADDR_CFG_H(x)       ((0x58 * x) + 0x044)
#define ADDR_SGR_L(x)       ((0x58 * x) + 0x048)
#define ADDR_SGR_H(x)       ((0x58 * x) + 0x04c)
#define ADDR_DSR_L(x)       ((0x58 * x) + 0x050)
#define ADDR_DSR_H(x)       ((0x58 * x) + 0x054)

#define ADDR_RawTfr        0x2c0
#define ADDR_RawBlk        0x2c8
#define ADDR_RawSrc        0x2d0
#define ADDR_RawDst        0x2d8
#define ADDR_RawErr        0x2e0

#define ADDR_StatusTfr     0x2e8  //read only
#define ADDR_StatusBlock   0x2f0  //read only
#define ADDR_StatusSrc     0x2f8  //read only
#define ADDR_StatusDst     0x300  //read only
#define ADDR_StatusErr     0x308  //read only

#define ADDR_MaskTfr       0x310
#define ADDR_MaskBlock     0x318
#define ADDR_MaskSrcTran   0x320
#define ADDR_MaskDstTran   0x328
#define ADDR_MaskErr       0x330

#define ADDR_ClearTfr      0x338
#define ADDR_ClearBlock    0x340
#define ADDR_ClearSrcTran  0x348
#define ADDR_ClearDstTran  0x350
#define ADDR_ClearErr      0x358

#define ADDR_StatusInt     0x360

#define ADDR_DmaCfgReg     0x398

#define ADDR_ChEnReg       0x3a0

#define ADDR_CTL0_LOW32    0x018
#define ADDR_CTL0_HIGH32   0x01c
#define ADDR_CFG0_LOW32    0x040
#define ADDR_CFG0_HIGH32   0x044

//====================================================================
//DMA default value
//====================================================================

#define DATA_ChEnReg       0x00000101    //0x3a0
#define DATA_ChDisReg      0x00000100    //0x3a0

#define DATA_ClearBlock    ((1 << AHB_DMA_CHANNELS_EN) - 1)    //0x340
#define DATA_ClearSrcTran  ((1 << AHB_DMA_CHANNELS_EN) - 1)    //0x348
#define DATA_ClearDstTran  ((1 << AHB_DMA_CHANNELS_EN) - 1)    //0x350
#define DATA_ClearErr      ((1 << AHB_DMA_CHANNELS_EN) - 1)    //0x358
#define DATA_ClearTfr      ((1 << AHB_DMA_CHANNELS_EN) - 1)    //0x338

#define DATA_SAR0          0x00000004    //0x000
#define DATA_DAR0          0x00000000    //0x008
#define DATA_LLP0          0x00000000    //0x010
#define DATA_CTL0_LOW32    0x00104d25    //0x018
#define DATA_CTL0_HIGH32   0x00001004    //0x01c
#define DATA_DmaCfgReg     0x00000001    //0x398
#define DATA_CFG0_LOW32    0x00808000    //0x040
#define DATA_CFG0_HIGH32   0x00007800    //0x044
#define DATA_MaskTfr       0x00000101    //0x310
#define DATA_MaskBlock     0x00000101    //0x318
#define DATA_MaskSrcTran   0x00000000    //0x320
#define DATA_MaskDstTran   0x00000000    //0x328
#define DATA_MaskErr       0x00000101    //0x330

#define DATA_SAR1          0x00000000
#define DATA_DAR1          0x00000000
#define DATA_LLP1          0x00000000
#define DATA_CTL1_LOW32    0x04a04d25
#define DATA_CTL1_HIGH32   0x00001004
#define DATA_CFG1_LOW32    0x00800800
#define DATA_CFG1_HIGH32   0x00010000

#define STATUS_INT_TFR     0x1
#define STATUS_INT_BLK     0x2
#define STATUS_INT_SRCT    0x4
#define STATUS_INT_DSTT    0x8
#define STATUS_INT_ERR     0x10

#define AHB_DMA_INTR       140

/* "_AHB" */
#define AHB_DMA_WAIT_FLAG  0x5f414842

#define AHB_DMA_MAX_BLK_TS 0x800

/* Total channel of dmac */
#define AHB_DMA_CHANNELS_NM  8
/* Enabled channels */
#define AHB_DMA_CHANNELS_EN  1
#define AHB_DMA_MSTS      2

struct ahb_dma_hs_config {
	unsigned int glb_v;
	unsigned int reg_v;
};

static struct ahb_dma_hs_config hs_config[AHB_DMA_HS_NUM] = {
	{0,  0 << 0},
	{13, 0 << 13},
	{13, 1 << 13},
	{12, 0 << 12},
	{12, 1 << 12},
	{9,  0 << 11},
	{9,  1 << 11},
	{8,  0 << 10},
	{8,  1 << 10},
	{3,  0 << 9},
	{3,  1 << 9},
	{2,  0 << 8},
	{2,  1 << 8},
	{15, 0 << 4},
	{15, 1 << 4},
	{15, 2 << 4},
	{15, 3 << 4},
	{15, 4 << 4},
	{15, 5 << 4},
	{15, 6 << 4},
	{15, 7 << 4},
	{15, 8 << 4},
	{14, 0 << 0},
	{14, 1 << 0},
	{14, 2 << 0},
	{14, 3 << 0},
	{14, 4 << 0},
	{14, 5 << 0},
	{14, 6 << 0},
	{14, 7 << 0},
	{14, 8 << 0},
};

static unsigned int dma_ctr_l = 0;

#define reg_nor_dma_write_field(host,addr, value, field) do {\
            sc_nor_dma_write(host,(sc_nor_dma_read(host,addr) & ~(MSK(field##_LEN, field##_LSB))) | \
                ((value) << (field##_LSB)),addr);\
            } while(0)

/* Here Assume the dst and the src are the same MSIZE and TR_WIDTH, SINGLE BLOCK transfer*/
static void inline ahb_dma_multi_block_transfer(struct sc_nor_host *host, int ch)
{
	sc_nor_dma_write(host, host->dma_buffer, ADDR_LLP_L(ch));
	sc_nor_dma_write(host, DATA_ChEnReg, ADDR_ChEnReg);

}

static void ahb_dma_clear_irq(struct sc_nor_host *host)
{
	sc_nor_dma_write(host, DATA_ClearBlock, ADDR_ClearBlock);
	sc_nor_dma_write(host, DATA_ClearErr, ADDR_ClearErr);
	sc_nor_dma_write(host, DATA_ClearTfr, ADDR_ClearTfr);
	sc_nor_dma_write(host, DATA_ClearErr, ADDR_ClearSrcTran);
	sc_nor_dma_write(host, DATA_ClearTfr, ADDR_ClearDstTran);
}

static void ahb_dma_wait_finish(struct sc_nor_host *host, int ch)
{
	volatile unsigned int val = 0;

	while(1) {
		val = sc_nor_dma_read(host, ADDR_RawTfr);
		if(val & STATUS_INT_TFR) {
			break;
		}
	}
}

static int ahb_dma_config(struct sc_nor_host *host, int ch, unsigned int src_hs, unsigned int dst_hs,
    single_block_ctl_t *cfg, hand_shake_cfg_t *hs_cfg)
{
	int ret, val, i;
	int mask;
	unsigned int reg_value;

	hs_cfg->src_hs = hs_config[src_hs].glb_v;
	hs_cfg->dst_hs = hs_config[dst_hs].glb_v;
	hs_cfg->hs_src_sel = SW_HDSK;
	hs_cfg->hs_dst_sel = HW_HDSK;

	/* Config global hs interface */
	val = hs_config[src_hs].reg_v |
	    hs_config[dst_hs].reg_v;

	mask = ((0 << ch) | (1 << (8 + ch)));

	sc_nor_dma_lowbit_write(host, val);

	ahb_dma_clear_irq(host);

	// Enable transfer complete interrupt and error interrupt
	sc_nor_dma_write(host, mask, ADDR_MaskTfr);

	sc_nor_dma_write(host, mask, ADDR_MaskErr);

	sc_nor_dma_write(host, mask, ADDR_MaskBlock);

	sc_nor_dma_write(host, mask, ADDR_MaskSrcTran);
	sc_nor_dma_write(host, mask, ADDR_MaskDstTran);

	reg_value = (cfg->src_tr_len << CTL_L_SRC_TR_WIDTH_LSB) |
	    (cfg->dst_tr_len << CTL_L_DST_TR_WIDTH_LSB) |
	    (cfg->src_burst_len << CTL_L_SRC_MSIZE_LSB) |
	    (cfg->dst_burst_len << CTL_L_DST_MSIZE_LSB) |
	    (cfg->flow_ctrl << CTL_L_TT_FC_LSB) |
	    (cfg->src_inc << CTL_L_SINC_LSB) |
	    (cfg->dst_inc << CTL_L_DINC_LSB) |
	    (cfg->src_mst << CTL_L_SMS_LSB) |
	    (cfg->dst_mst << CTL_L_DMS_LSB) |
	    (1 << CTL_L_LLP_DST_EN_LSB) |
	    (1 << CTL_L_LLP_SRC_EN_LSB) |
	    (1 << CTL_L_INT_EN_LSB);

	dma_ctr_l = reg_value;

	sc_nor_dma_write(host, reg_value, ADDR_CTL_L(ch));

	reg_value = sc_nor_dma_read(host, ADDR_CFG_L(ch));
	reg_value = reg_value & (~(BIT(CFG_L_RELOAD_DST_LSB) | BIT(CFG_L_RELOAD_SRC_LSB)));
	sc_nor_dma_write(host, reg_value, ADDR_CFG_L(ch));

	reg_nor_dma_write_field(host, ADDR_CFG_L(ch), hs_cfg->hs_src_sel, CFG_L_HS_SEL_SRC);
	reg_nor_dma_write_field(host, ADDR_CFG_L(ch), hs_cfg->hs_dst_sel, CFG_L_HS_SEL_DST);

	reg_nor_dma_write_field(host, ADDR_CFG_H(ch), hs_cfg->src_hs, CFG_H_SRC_PER);
	reg_nor_dma_write_field(host, ADDR_CFG_H(ch), hs_cfg->dst_hs, CFG_H_DST_PER);

	sc_nor_dma_write(host, DATA_DmaCfgReg, ADDR_DmaCfgReg);

	return 0;
}

void sc_ahb_dma_init(struct sc_nor_host *host)
{
	struct device *dev = host->dev;
	struct ahb_dma_llp *dma_llp = host->dma_llp;
	int i, n;
	int ch;
	unsigned int src_hs;
	unsigned int dst_hs;
	single_block_ctl_t cfg;
	hand_shake_cfg_t hs_cfg;

	ch = 0;

	src_hs = AHB_DMA_HS_MEMORY;
	dst_hs = AHB_DMA_HS_MEMORY;

	cfg.src_inc = INCREMENT;
	cfg.dst_inc = INCREMENT;
	cfg.src_mst = AHB_MST_1;
	cfg.dst_mst = AHB_MST_2;
	cfg.src_tr_len = TR_WIDTH_32;
	cfg.dst_tr_len = TR_WIDTH_32;
	cfg.src_burst_len = MSIZE_16;
	cfg.dst_burst_len = MSIZE_16;
	cfg.flow_ctrl = M2M_DMAC;

	hs_cfg.src_hs = -1;
	hs_cfg.dst_hs = -1;
	hs_cfg.hs_src_sel = SW_HDSK;
	hs_cfg.hs_dst_sel = HW_HDSK;
	ahb_dma_config(host, ch, src_hs, dst_hs, &cfg, &hs_cfg);

}

int sc_ahb_dma_m2m(struct sc_nor_host *host, unsigned char *src, unsigned char *dst, unsigned int size,
    u_char *read_buf, unsigned int check_buf)
{
	struct ahb_dma_llp *dma_llp = host->dma_llp;
	int i, n;
	unsigned int llp_size, blk_size;
	unsigned int tmp_size1, tmp_size2;
	unsigned int blk_ts;
	int ch, llp_num;
	unsigned int size1 = size;

	blk_size = AHB_DMA_MAX_BLK_TS * 4;
	llp_size = blk_size * AHB_DMA_MAX_LLP_ND;
	while(size) {
		tmp_size1 = tmp_size2 = size < llp_size ? size : llp_size;
		llp_num = (tmp_size1 + blk_size - 1) / blk_size;

		for(i = 0; i < llp_num; i++) {
			blk_ts = tmp_size1 < blk_size ? ((tmp_size1 + 3) / 4) : AHB_DMA_MAX_BLK_TS;
			n = (i + 1) % AHB_DMA_MAX_LLP_ND;
			dma_llp[i].ctl_h = blk_ts;
			dma_llp[i].ctl_l = dma_ctr_l;
			dma_llp[i].src = src;
			dma_llp[i].dst = dst;
			dma_llp[i].next_llp = host->dma_buffer + (i + 1) * sizeof(struct ahb_dma_llp);
			src += blk_ts * 4;
			dst += blk_ts * 4;
			tmp_size1 -= blk_ts * 4;
		}
		dma_llp[i - 1].next_llp = 0;
		ahb_dma_multi_block_transfer(host, ch);
		size -= tmp_size2;
	}

	ahb_dma_wait_finish(host, ch);

	while(1) {
		if(check_buf == *(volatile unsigned int *)((unsigned char *)host->temp_buf + size1 - 4))
			break;
	}
	memcpy(read_buf, host->temp_buf, size1);

	ahb_dma_clear_irq(host);

	return 0;
}

