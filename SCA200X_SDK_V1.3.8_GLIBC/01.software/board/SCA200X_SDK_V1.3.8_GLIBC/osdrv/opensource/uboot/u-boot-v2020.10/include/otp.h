#ifndef _OTP_H_
#define _OTP_H_

#define OTP_ERR_SUCCESS    0
#define OTP_ERR_BUS_ERROR  1
#define OTP_ERR_INV_ACCESS 2
#define OTP_ERR_PRGM_FAIL  3
/*
 * otp interface
 * buf: output buffer
 * num: read/write number of int
 * addr: read/write address, in bits
 */
int otp_read(unsigned int *buf, unsigned int num, unsigned int addr);
int otp_write(unsigned int *buf, unsigned int num, unsigned int addr);
#endif
