#ifndef _PKA_HAL_H_
#define _PKA_HAL_H_

#include <asm/io.h>

//Register address
#if 1
	#define pka_controller_BaseAddress            0x01040000

	#define pka_region_A_BaseAddress              0x01040400
	#define pka_region_B_BaseAddress              0x01040800
	#define pka_region_C_BaseAddress              0x01040c00
	#define pka_region_D_BaseAddress              0x01041000

#else
	#define pka_controller_BaseAddress            0x64510000

	#define pka_region_A_BaseAddress              0x64510400
	#define pka_region_B_BaseAddress              0x64510800
	#define pka_region_C_BaseAddress              0x64510c00
	#define pka_region_D_BaseAddress              0x64511000
#endif

#define PKA_CTRL                              (pka_controller_BaseAddress + 0x0)
#define PKA_ENTRY_PNT                         (pka_controller_BaseAddress + 0x4)
#define PKA_RTN_CODE                          (pka_controller_BaseAddress + 0x8)
#define PKA_BUILD_CONFIG                      (pka_controller_BaseAddress + 0xc)
#define PKA_STACK_PNTR                        (pka_controller_BaseAddress + 0x10)
#define PKA_INSTR_SINCE_GO                    (pka_controller_BaseAddress + 0x14)
#define PKA_CONFIG                            (pka_controller_BaseAddress + 0x1c)
#define PKA_STAT                              (pka_controller_BaseAddress + 0x20)
#define PKA_FLAGS                             (pka_controller_BaseAddress + 0x24)
#define PKA_WATCHDOG                          (pka_controller_BaseAddress + 0x28)
#define PKA_CYCLES_SINCE_GO                   (pka_controller_BaseAddress + 0x2c)
#define PKA_INDEX_I                           (pka_controller_BaseAddress + 0x30)
#define PKA_INDEX_J                           (pka_controller_BaseAddress + 0x34)
#define PKA_INDEX_K                           (pka_controller_BaseAddress + 0x38)
#define PKA_INDEX_L                           (pka_controller_BaseAddress + 0x3c)
#define PKA_IRQ_EN                            (pka_controller_BaseAddress + 0x40)
#define PKA_JMP_PROB                          (pka_controller_BaseAddress + 0x44)
#define PKA_JMP_PROB_LFSR                     (pka_controller_BaseAddress + 0x48)
#define PKA_BANK_SW_A                         (pka_controller_BaseAddress + 0x50)
#define PKA_BANK_SW_B                         (pka_controller_BaseAddress + 0x54)
#define PKA_BANK_SW_C                         (pka_controller_BaseAddress + 0x58)
#define PKA_BANK_SW_D                         (pka_controller_BaseAddress + 0x5c)

////////////////////////////////////////////////////////////////////////////////////

//Register bit define
//CTRL
#define CTRL_PARTIAL_RADIX_LEN                8
#define CTRL_PARTIAL_RADIX_SHIFT              0
#define CTRL_BASE_RADIX_LEN                   3
#define CTRL_BASE_RADIX_SHIFT                 8
#define CTRL_BASE_512_MODE_LEN                5
#define CTRL_BASE_512_MODE_SHIFT              16
#define CTRL_STOP_RQST_BIT                    27
#define CTRL_GO_BIT                           31

//RTN_CODE
#define RTN_CODE_STOP_REASON_LEN              8
#define RTN_CODE_STOP_REASON_SHIFT            16
#define RTN_CODE_DPA_EN_BIT                   27
#define RTN_CODE_ZERO_BIT                     28
#define RTN_CODE_IRO_BIT                      30
#define RTN_CODE_BUSY_BIT                     31

//BUILD_CONFIG
#define BUILD_CONFIG_BANK_SW_A_LEN            2
#define BUILD_CONFIG_BANK_SW_A_SHIFT          0
#define BUILD_CONFIG_BANK_SW_B_LEN            2
#define BUILD_CONFIG_BANK_SW_B_SHIFT          2
#define BUILD_CONFIG_BANK_SW_C_LEN            2
#define BUILD_CONFIG_BANK_SW_C_SHIFT          4
#define BUILD_CONFIG_BANK_SW_D_LEN            2
#define BUILD_CONFIG_BANK_SW_D_SHIFT          6
#define BUILD_CONFIG_FW_RAM_SIZE_LEN          3
#define BUILD_CONFIG_FW_RAM_SIZE_SHIFT        8
#define BUILD_CONFIG_FW_ROM_SIZE_LEN          3
#define BUILD_CONFIG_FW_ROM_SIZE_SHIFT        11
#define BUILD_CONFIG_ECC_MAX_LEN              2
#define BUILD_CONFIG_ECC_MAX_SHIFT            14
#define BUILD_CONFIG_RSA_MAX_LEN              3
#define BUILD_CONFIG_RSA_MAX_SHIFT            16
#define BUILD_CONFIG_ALU_WIDTH_LEN            2
#define BUILD_CONFIG_ALU_WIDTH_SHIFT          19
#define BUILD_CONFIG_TA_DPA_SUPPORT_LEN       2
#define BUILD_CONFIG_TA_DPA_SUPPORT_SHIFT     21
#define BUILD_CONFIG_FORMAT_TYPE_LEN          2
#define BUILD_CONFIG_FORMAT_TYPE_SHIFT        30

//CONFIG
#define CONFIG_ALT_ACCESS_BIT                 0
#define CONFIG_ENDIAN_SWAP_BIT                26

//STAT
#define STAT_DONE_BIT                         30

//FlAGS
#define FLAGS_Z_BIT                           0
#define FLAGS_M_BIT                           1
#define FLAGS_B_BIT                           2
#define FLAGS_C_BIT                           3
#define FLAGS_F0_BIT                          4
#define FLAGS_F1_BIT                          5
#define FLAGS_F2_BIT                          6
#define FLAGS_F3_BIT                          7

//IRQ EN
#define IRQ_EN_IE_BIT                         30

/////////////////////////////////////////////////////////////////////

//STOP_REASON
enum {
	STOP_NORMAL                 = 0,
	STOP_INV_OPCODE             = 1,
	STOP_STACK_UNDER_FLOW       = 2,
	STOP_STACK_OVER_FLOW        = 3,
	STOP_WATCH_DOG              = 4,
	STOP_MEMORTY_PORT_COLLISION = 8,
};

//CTRL bit settings
#define CTRL_BASE_RADIX_256                   2 // 256-bit
#define CTRL_BASE_RADIX_512                   3 // 512-bit
#define CTRL_BASE_RADIX_1024                  4 // 1024-bit
#define CTRL_BASE_RADIX_2048                  5 // 2048-bit
#define CTRL_BASE_RADIX_4096                  6 // 4096-bit

//Entry define
#define ENTRY_BIT_SERIAL_MOD                  0x18
#define ENTRY_BIT_SERIAL_MOD_DP               0x17
#define ENTRY_CALC_MP                         0x10
#define ENTRY_CALC_R_INV                      0x11
#define ENTRY_CALC_R_SQR                      0x12
#define ENTRY_CRT                             0x16
#define ENTRY_CRT_KEY_SETUP                   0x15
#define ENTRY_IS_A_M3                         0x22
#define ENTRY_IS_P_EQUAL_Q                    0x20
#define ENTRY_IS_P_REFLECT_Q                  0x21
#define ENTRY_MODADD                          0xb
#define ENTRY_MODDIV                          0xd
#define ENTRY_MODEXP                          0x14
#define ENTRY_MODINV                          0xe
#define ENTRY_MODMULT                         0xa
#define ENTRY_MODSUB                          0xc
#define ENTRY_MULT                            0x13
#define ENTRY_PADD                            0x1c
#define ENTRY_PADD_STD_PRJ                    0x1d
#define ENTRY_PDBL                            0x1a
#define ENTRY_PDBL_STD_PRJ                    0x1b
#define ENTRY_PMULT                           0x19
#define ENTRY_PVER                            0x1e
#define ENTRY_REDUCE                          0xf
#define ENTRY_SHAMIR                          0x23
#define ENTRY_STD_PRJ_TO_AFFINE               0x1f

//BUILDC_CONFIG bit settings
#define BANK_SW_1                             0 // 1 bank
#define BANK_SW_2                             1 // 2 banks
#define BANK_SW_4                             2 // 4 banks

#define FW_SIZE_NO                            0 // No F/W Rom/Ram
#define FW_SIZE_256                           1 // 256 words
#define FW_SIZE_512                           2 // 512 words
#define FW_SIZE_1024                          3 // 1024 words
#define FW_SIZE_2048                          4 // 2048 words
#define FW_SIZE_4096                          5 // 4096 words

#define ECC_MAX_NOT_SUPPORT                   0 // ECC ops not supported
#define ECC_MAX_256                           1 // 256bits
#define ECC_MAX_512                           2 // 512bits
#define ECC_MAX_1024                          3 // 1024bits

#define RSA_MAX_NOT_SUPPORT                   0 // RSA ops not supported
#define RSA_MAX_512                           1 // 512bits
#define RSA_MAX_1024                          2 // 1024 bits
#define RSA_MAX_2048                          3 // 2048 bits
#define RSA_MAX_4986                          4 // 4096 bits

#define RSA_ALU_WIDTH_32                      0 // 32bits
#define RSA_ALU_WIDTH_64                      1 // 64 bits
#define RSA_ALU_WIDTH_128                     2 // 128bits
#define RSA_ALU_WIDTH_REERVEDS                 3 // reserved

#define RSA_TA_DPA_SUPPORT_NO_SUPPORT         0 // No Support
#define RSA_TA_DPA_SUPPORT_TA                 1 // TA support
#define RSA_TA_DPA_SUPPORT_DPATA              2 // DPA/TA support
#define RSA_TA_DPA_SUPPORT_RESERVED           3 //Reserved

#define FORMAT_TYPE_DEFAULT                   0x2

//////////////////////////////////////////////////////////////////////////
#define pka_get_bit(value, bit) \
        ((value >> bit) & 0x1)

#define pka_set_bit(value, bit) \
        (value | (1 << bit))

#define pka_clear_bit(value, bit) \
        (value & (~(1 << bit)))

#define pka_get_bits(value, shift, len) \
        ((value >> shift) & ((1 << len) - 1))

#define pka_set_bits(value, shift, value2) \
        (value | (value2 << shift))

/////////////////////////////////////////////////////////////////////////

static inline void pka_write_reg32(unsigned long addr, unsigned int value)
{
	writel(value, addr);
}

static inline unsigned int pka_read_reg32(unsigned long addr)
{
	return readl(addr);
}
#endif
