#ifndef _OTP_HAL_H_
#define _OTP_HAL_H_

#include <asm/io.h>

#define OTP_BASE_ADDRESS                          0x64520000

#define OTP_HW_CFG0                               (OTP_BASE_ADDRESS + 0x0)
#define OTP_HW_CFG1                               (OTP_BASE_ADDRESS + 0x4)
#define OTP_HW_CFG2                               (OTP_BASE_ADDRESS + 0x8)
#define OTP_HW_CFG3                               (OTP_BASE_ADDRESS + 0xc)
#define OTP_PASSWD_SW0                            (OTP_BASE_ADDRESS + 0x80)
#define OTP_PASSWD_SW1                            (OTP_BASE_ADDRESS + 0x84)
#define OTP_PASSWD_STATE                          (OTP_BASE_ADDRESS + 0x88)
#define OTP_PGRM_TIME                             (OTP_BASE_ADDRESS + 0x8c)
#define OTP_CHECKSUM                              (OTP_BASE_ADDRESS + 0x90)
#define OTP_PRGM_DATA                             (OTP_BASE_ADDRESS + 0x94)
#define OTP_CMD_DATA                              (OTP_BASE_ADDRESS + 0x98)
#define OTP_READ_DATA                             (OTP_BASE_ADDRESS + 0x9c)
#define OTP_INT_EN                                (OTP_BASE_ADDRESS + 0xa0)
#define OTP_INT_STATE                             (OTP_BASE_ADDRESS + 0xa4)

//Command data register setttings
#define CMD_READ                                  (0 << 31)
#define CMD_WRITE                                 (1 << 31)

//Interrupt register settings
#define OTP_INT_EN_BUS_ERROR                      (1 << 0)
#define OTP_INT_EN_UNPERMITTED_SECURE_ACCESS      (1 << 1)
#define OTP_INT_EN_RD_FINISH                      (1 << 2)
#define OTP_INT_EN_PGRM_FAIL                      (1 << 3)
#define OTP_INT_EN_PGRM_FINISH                    (1 << 4)

//Interrupt status regiser settings
#define OTP_INT_STAT_BUS_ERROR                    (1 << 0)
#define OTP_INT_STAT_UNPERMITTED_SECURE_ACCESS    (1 << 1)
#define OTP_INT_STAT_RD_FINISH                    (1 << 2)
#define OTP_INT_STAT_PGRM_FAIL                    (1 << 3)
#define OTP_INT_STAT_PGRM_FINISH                  (1 << 4)

////////////////////////////////////////////////////////////////////

#define otp_get_bit(value, bit) \
        ((value >> bit) & 0x1)

#define otp_set_bit(value, bit) \
        (value | (1 << bit))

#define otp_clear_bit(value, bit) \
        (value & (~(1 << bit)))

#define otp_get_bits(value, shift, len) \
        ((value >> shift) & ((1 << len) - 1))

#define otp_set_bits(value, shift, value2) \
        (value | (value2 << shift))

/////////////////////////////////////////////////////////////////////////

static inline void otp_write_reg32(unsigned long addr, unsigned int value)
{
	writel(value, addr);
}

static inline unsigned int otp_read_reg32(unsigned long addr)
{
	return readl(addr);
}

#endif
