#ifndef __SPACC_HAL_H__
#define __SPACC_HAL_H__

#include <asm/io.h>

#define SPAcc_BaseAddress          0x01180000

//------------------------------register definitions------------------------------
#define SPACC_IRQ_EN              (SPAcc_BaseAddress + 0x0)
#define SPACC_IRQ_STAT            (SPAcc_BaseAddress + 0x4)
#define SPACC_IRQ_CTRL            (SPAcc_BaseAddress + 0x8)
#define SPACC_FIFO_STAT           (SPAcc_BaseAddress + 0xc)
#define SPACC_SDMA_BRST_SZ        (SPAcc_BaseAddress + 0x10)
#define SPACC_SRC_PTR             (SPAcc_BaseAddress + 0x20)
#define SPACC_DST_PTR             (SPAcc_BaseAddress + 0x24)
#define SPACC_AAD_OFFSET          (SPAcc_BaseAddress + 0x28)
#define SPACC_PRE_AAD_LEN         (SPAcc_BaseAddress + 0x2c)
#define SPACC_POST_AAD_LEN        (SPAcc_BaseAddress + 0x30)
#define SPACC_PROC_LEN            (SPAcc_BaseAddress + 0x34)
#define SPACC_ICV_LEN             (SPAcc_BaseAddress + 0x38)
#define SPACC_ICV_OFFSET          (SPAcc_BaseAddress + 0x3c)
#define SPACC_IV_OFFSET           (SPAcc_BaseAddress + 0x40)
#define SPACC_SW_CTRL             (SPAcc_BaseAddress + 0x44)
#define SPACC_AUX_INFO            (SPAcc_BaseAddress + 0x48)
#define SPACC_CTRL                (SPAcc_BaseAddress + 0x4c)
#define SPACC_STAT_POP            (SPAcc_BaseAddress + 0x50)
#define SPACC_STATUS              (SPAcc_BaseAddress + 0x54)
#define SPACC_STAT_WD_CTRL        (SPAcc_BaseAddress + 0x80)
#define SPACC_KEY_SZ              (SPAcc_BaseAddress + 0x100)
#define SPACC_VSPACC_RQST         (SPAcc_BaseAddress + 0x140)
#define SPACC_VSPACC_ALLOC        (SPAcc_BaseAddress + 0x144)
#define SPACC_VSPACC_PRIORITY     (SPAcc_BaseAddress + 0x148)
#define SPACC_VSPACC_RC4_KEY_REQ  (SPAcc_BaseAddress + 0x150)
#define SPACC_VSPACC_RC4_KEY_GNT  (SPAcc_BaseAddress + 0x154)
#define SPACC_VERSION             (SPAcc_BaseAddress + 0x180)
#define SPACC_VERSION_EXT         (SPAcc_BaseAddress + 0x184)
#define SPACC_VERSION_EXT_2       (SPAcc_BaseAddress + 0x190)
#define SPACC_SECURE_CTRL         (SPAcc_BaseAddress + 0x1c0)
#define SPACC_SECURE_CTX_RELEASE  (SPAcc_BaseAddress + 0x1c4)
#define SPACC_CIPH_CTX_0          (SPAcc_BaseAddress + 0x4000)
#define SPACC_HASH_CTX_0          (SPAcc_BaseAddress + 0x8000)

#define  SPACC_CIPH_CTX_SZ        0x80
#define  SPACC_HASH_CTX_SZ        0x80
//------------------------------register field definitions------------------------------
// SPACC_IRQ_EN bit define
#define IRQ_CMD0_EN_BIT           0
#define IRQ_CMD1_EN_BIT           1
#define IRQ_CMD2_EN_BIT           2
#define IRQ_STAT_EN_BIT           4
#define IRQ_RC4_DMA_EN_BIT        8
#define IRQ_STAT_WD_EN_BIT        12
#define IRQ_GLBL_EN_BIT           31

// SPACC_IRQ_STAT bit define
#define IRQ_STAT_CMD0_BIT         0
#define IRQ_STAT_CMD1_BIT         1
#define IRQ_STAT_CMD2_BIT         2
#define IRQ_STAT_STAT_BIT         4
#define IRQ_STAT_RC4_DMA_BIT      8
#define IRQ_STAT_STAT_WD_BIT      12

// SPACC_IRQ_CTRL bit define
#define IRQ_CTRL_CMD0_CNT_LEN     8
#define IRQ_CTRL_CMD0_CNT_SHIFT   0
#define IRQ_CTRL_STAT_CNT_LEN     9
#define IRQ_CTRL_STAT_CNT_SHIFT   16

// SPACC_FIFO_STAT bit define
#define FIFO_STAT_CMD0_CNT_LEN    9
#define FIFO_STAT_CMD0_CNT_SHIFT  0
#define FIFO_STAT_CMD0_FULL_BIT   15
#define FIFO_STAT_CNT_LEN         9
#define FIFO_STAT_CNT_SHIFT       16
#define FIFO_STAT_EMPTY_BIT       31

// SPACC_SDMA_BRST_SZ bit define
#define SDMA_BRST_SZ_LEN          5
#define SDMA_BRST_SZ_SHIFT        0

// SPACC_AAD_OFFSET bit define
#define AAD_SRC_OFFSET_LEN        16
#define AAD_SRC_OFFSET_SHIFT      0
#define AAD_DST_OFFSET_LEN        16
#define AAD_DST_OFFSET_SHIFT      16

// SPACC_IV_OFFSET bit define
#define IV_OFFSET_len             31
#define IV_OFFSET_SHIFT           0
#define IV_OFFSET_ENABLE_BIT      31

// SPACC_SW_CTRL bit define
#define SW_ID_LEN                 8
#define SW_ID_SHIFT               0
#define SW_CTRL_PRIORITY_LEN      2
#define SW_CTRL_PRIORITY_SHIFT    30

// SPACC_AUX_INFO bit define
#define AUX_INFO_DIR_BIT          0
#define AUX_INFO_BIT_ALIGN_LEN    3
#define AUX_INFO_BIT_ALIGN_SHIFT  1
#define CRC_REF_IN_BIT            1
#define CRC_REF_OUT_BIT           2
#define CRC_REF_INV_BIT           3
#define CBC_CS_SEL_LEN            2
#define CBC_CS_SEL_SHIFT          16

// SPACC_CTRL bit define
#define CIPH_ALG_LEN              3
#define CIPH_ALG_SHIFT            0
#define HASH_ALG_LEN              5
#define HASH_ALG_SHIFT            3
#define CIPH_MODE_LEN             4
#define CIPH_MODE_SHIFT           8
#define HASH_MODE_LEN             2
#define HASH_MODE_SHIFT           12
#define MSG_BEGIN_BIT             14
#define MSG_END_BIT               15
#define CTX_IDX_LEN               8
#define CTX_IDX_SHIFT             16
#define ENCRYPT_BIT               24
#define AAD_COPY_BIT              25
#define ICV_PT_BIT                26
#define ICV_ENC_BIT               27
#define ICV_APPEND_BIT            28
#define KEY_EXP_BIT               29
#define SEC_KEY_BIT               31

// SPACC_STATUS bit define
#define STATUS_SW_ID_LEN          8
#define STATUS_SW_ID_SHIFT        0
#define STATUS_RET_CODE_LEN       3
#define STATUS_RET_CODE_SHIFT     24
#define STATUS_SEC_CMD_BIT        31

//SPACC KEY_SZ bit define
#define KEY_SIZE_LEN              8
#define KEY_SIZE_SHIFT            0
#define KEY_SIZE_CTX_IDX_LEN      8
#define KEY_SIZE_CTX_IDX_SHIFT    8
#define KEY_SIZE_CIPHER_BIT       31

//SPACC VSPACC_PRIORITY bit define
#define PRIORITY_MODE_BIT         0
#define PRIORITY_WEIGHT_LEN       4
#define PRIORITY_WEIGHT_SHIFT     8

//SPACC  VERSION bit define
#define VERSION_MINOR_LEN         4
#define VERSION_MINOR_SHIFT       0
#define VERSION_MAJOR_LEN         4
#define VERSION_MAORR_SHIFT       4
#define VERSION_QOS_BIT           8
#define VERSION_TYPE_LEN          2
#define VERSION_TYPE_SHIFT        9
#define VERSION_AUX_BIT           11
#define VERSION_VSPACC_IDX_LEN    3
#define VERSION_VSPACC_IDX_SHIFT  12
#define VERSION_VSPACC_PARTIAL_PKT_BIT 15
#define VERSION_PROJECT_LEN       16
#define VERSION_PROJECT_SHIFT     16

//SPACC VERSION EXT
#define VERSION_EXT_CTX_CNT_LEN        8
#define VERSION_EXT_CTX_CNT_SHIFT      0
#define VERSION_EXT_RC4_CTX_CNT_LEN    8
#define VERSION_EXT_RC4_CTX_CNT_SHIFT  8
#define VERSION_EXT_VSPACC_CNT_LEN     4
#define VERSION_EXT_VSPACC_CNT_SHIFT   16
#define VERSION_EXT_CIPH_CTX_SZ_LEN    3
#define VERSION_EXT_CIPH_CTX_SZ_SHIFT  20
#define VERSION_EXT_HASH_CTX_SZ_LEN    3
#define VERSION_EXT_HASH_CTX_SZ_SHIFT  24
#define VERSION_EXT_DMA_TYPE_LEN       2
#define VERSION_EXT_DMA_TYPE_SHIFT     28

//SPACC  VERSION_EXT_2
#define VERSION_CMD0_DEPTH_LEN         9
#define VERSION_CMD0_DEPTH_SHIFT       0
#define VERSION_CMD0_STAT_DEPTH_LEN    9
#define VERSION_CMD0_STAT_DEPTH_SHIFT  16

//SPACC  SECURE_CTRL
#define MS_SRC_BIT                0
#define MS_DST_BIT                1
#define MS_DDT_BIT                2
#define MS_RC4_LOCK_BIT           30
#define SECURE_CTRL_LOCK_BIT      31

//------------------------------register field values------------------------------
#define SPACC_KEY_CONTEXT_IDX_0   0
#define SPACC_KEY_CONTEXT_IDX_1   1

//IRQ enable setting
#define IRQ_ENABLE                1

//SPAcc pop value
#define POP_VALUE                 1

#define DES_IV_OFFSET       0
#define DES_KEY1_OFFSET     0
#define DES_KEY2_OFFSET     0
#define DES_KEY3_OFFSET     0

//Iv offset int key context
#define AES_IV_OFFSET     0x20

//////////////////////////////////////////////////////////////////////////
#define spacc_get_bit(value, bit) \
        ((value >> bit) & 0x1)

#define spacc_set_bit(value, bit) \
        (value | (1 << bit))

#define spacc_clear_bit(value, bit) \
        (value & (~(1 << bit)))

#define spacc_get_bits(value, shift, len) \
        ((value >> shift) & ((1 << len) - 1))

#define spacc_set_bits(value, shift, value2) \
        (value | (value2 << shift))

/////////////////////////////////////////////////////////////////////////

static inline void spacc_write_reg32(unsigned long addr, unsigned int value)
{
	writel(value, addr);
}

static inline unsigned int spacc_read_reg32(unsigned long addr)
{
	return readl(addr);
}

#endif
