/*
 * SmartChip OTP driver
 */
#include <otp.h>
#include "otp_hal.h"

#define OTP_RD 0x1
#define OTP_WR 0x2

#define OTP_NON_SECURE
//#define OTP_INTR_MODE

#ifdef OTP_INTR_MODE
static int otp_job_done = 0;
static int ret_status;

void otp_isr(void)
{
	otp_job_done = 1;

	//Read int value
	ret_status = otp_read_reg32(OTP_INT_STATE);
	//Clear interrupt status
	otp_write_reg32(OTP_INT_STATE, 0x0);
}

static int otp_polling_finish(void)
{
	int value;

	//Loop and wait for interrupt status
	while(1) {
		asm volatile("wfe");
		if(otp_job_done) {
			otp_job_done = 0;
			break;
		}
	}

	otp_write_reg32(OTP_INT_STATE, 0x0);

	if(value & OTP_INT_STAT_UNPERMITTED_SECURE_ACCESS)
		return OTP_ERR_INV_ACCESS;
	else {
		switch(value) {
		case OTP_INT_STAT_BUS_ERROR:
			return OTP_ERR_BUS_ERROR;
		case OTP_INT_STAT_PGRM_FAIL:
			return OTP_ERR_PRGM_FAIL;
		default:
			return OTP_ERR_SUCCESS;
		}
	}
}
#else
static int otp_polling_finish(int rw)
{
	int value;

	//Loop and wait for interrupt status
	while(1) {
		value = otp_read_reg32(OTP_INT_STATE);
		if(rw == OTP_RD) {
			if(value)
				break;
		}
#ifdef OTP_NON_SECURE
		if(rw == OTP_WR) {
			if(value)
				break;
		}
#else
		if(rw == OTP_WR) {
			if(value & OTP_INT_EN_PGRM_FINISH)
				break;
		}
#endif
	}

	//printf("status %d\n", value);

	otp_write_reg32(OTP_INT_STATE, 0x0);

	if(value & OTP_INT_STAT_UNPERMITTED_SECURE_ACCESS)
		return OTP_ERR_INV_ACCESS;
	else {
		switch(value) {
		case OTP_INT_STAT_BUS_ERROR:
			return OTP_ERR_BUS_ERROR;
		case OTP_INT_STAT_PGRM_FAIL:
			return OTP_ERR_PRGM_FAIL;
		default:
			return OTP_ERR_SUCCESS;
		}
	}
}
#endif

/*
 *Addr is bit aligned
 */
int otp_read(unsigned int *buf, unsigned int num, unsigned int addr)
{
	int i, ret, value;

	value = OTP_INT_EN_BUS_ERROR |
	    OTP_INT_EN_UNPERMITTED_SECURE_ACCESS |
	    OTP_INT_EN_RD_FINISH |
	    OTP_INT_EN_PGRM_FAIL |
	    OTP_INT_EN_PGRM_FINISH;

	for(i = 0; i < num; i++) {
		//Enable interrupts
		otp_write_reg32(OTP_INT_EN, value);

		//Send read cmd
		otp_write_reg32(OTP_CMD_DATA, CMD_READ | addr);

		//Polling finish
		ret = otp_polling_finish(OTP_RD);
		if(ret) {
			*buf++ = value;
			return ret;
		}

		//Read 32bits otp data from register
		value = otp_read_reg32(OTP_READ_DATA);

		*buf++ = value;

		//addr is bit aligned, increase 32bit each time
		addr += 32;
	}

	return 0;
}

int otp_write(unsigned int *buf, unsigned int num, unsigned int addr)
{
	int i, ret, int_en;

	int_en = OTP_INT_EN_BUS_ERROR |
	    OTP_INT_EN_UNPERMITTED_SECURE_ACCESS |
	    OTP_INT_EN_RD_FINISH |
	    OTP_INT_EN_PGRM_FAIL |
	    OTP_INT_EN_PGRM_FINISH;

	for(i = 0; i < num; i++) {
		//Enable interrupts
		otp_write_reg32(OTP_INT_EN, int_en);

		//Write 32bits otp data to program register
		otp_write_reg32(OTP_PRGM_DATA, *buf++);

		//Send read cmd
		otp_write_reg32(OTP_CMD_DATA, CMD_WRITE | addr);

		//Polling finish
		ret = otp_polling_finish(OTP_WR);
		if(ret) {
			return ret;
		}

		//addr is bit aligned, increase 32bit each time
		addr += 32;
	}

	return 0;
}

#define OTP_TEST
#ifdef OTP_TEST
#if 1
void otp_test_bank(int bank, int start, int end, int rw)
{
	int i, ret;
	unsigned int key1, key2;
	unsigned int addr = 0;

	printf("uboot otptest bank %d\n", bank);

	for(i = start; i < end; i++) {
		addr = i * 32;
		key1 = 0;
		key2 = 0;
		ret = otp_read(&key1, 0x1, addr);
		printf("addr=0x%x, read value=0x%x, ret=%x\n", addr, key1, ret);
		if(rw & OTP_RD) {
			if(ret != 0) {
				printf("error\n");
				while(1);
			}
		} else {
			if(ret == 0) {
				printf("error\n");
				while(1);
			}
		}
		key2 = key1;
		key1 = i;
		ret = otp_write(&key1, 0x1, addr);
		printf("addr=0x%x, write ret=0x%x\n", addr, ret);
		if(rw & OTP_WR) {
			if(ret != 0) {
				printf("error\n");
				while(1);
			}
		} else {
			if(ret == 0) {
				printf("error\n");
				while(1);
			}
		}
		key1 = 0;
		ret = otp_read(&key1, 0x1, addr);
		printf("addr=0x%x, read value=0x%x, ret=%x\n\n", addr, key1, ret);
		if(rw == (OTP_WR | OTP_RD)) {
			key2 = key2 | i;
			if(key2 != key1) {
				printf("error\n");
				while(1);
			}
		}
		addr += 32;
	}
	printf("bank %d, test ok\n\n\n\n", bank);
}

void otp_test(void)
{
	int i;
#ifdef OTP_NON_SECURE
	//Non Secure test
	for(i = 3; i < 28; i += 5) {
		otp_test_bank(i + 0, (i + 0) * 32, (i + 1) * 32, OTP_RD);
		otp_test_bank(i + 1, (i + 1) * 32, (i + 2) * 32, OTP_RD);
		otp_test_bank(i + 2, (i + 2) * 32, (i + 3) * 32, 0x0);
		otp_test_bank(i + 3, (i + 3) * 32, (i + 4) * 32, 0x0);
		otp_test_bank(i + 4, (i + 4) * 32, (i + 5) * 32, 0x0);
	}
	otp_test_bank(28, 896, 928, OTP_RD);
	otp_test_bank(29, 928, 960, OTP_RD);
	otp_test_bank(30, 960, 992, 0x0);
	otp_test_bank(31, 992, 1024, 0x0);
	otp_test_bank(32, 1024, 2048, 0x0);

	otp_test_bank(33, 2048, 3072, OTP_RD);
	otp_test_bank(34, 3072, 4096, OTP_RD);
#else
	//Secure test
	for(i = 3; i < 28; i += 5) {
		otp_test_bank(i + 0, (i + 0) * 32, (i + 1) * 32, OTP_RD | OTP_WR);
		otp_test_bank(i + 1, (i + 1) * 32, (i + 2) * 32, OTP_RD);
		otp_test_bank(i + 2, (i + 2) * 32, (i + 3) * 32, OTP_RD);
		otp_test_bank(i + 3, (i + 3) * 32, (i + 4) * 32, OTP_RD);
		otp_test_bank(i + 4, (i + 4) * 32, (i + 5) * 32, 0x0);
	}
	otp_test_bank(28, 896, 928, OTP_RD | OTP_WR);
	otp_test_bank(29, 928, 960, OTP_RD);
	otp_test_bank(30, 960, 992, OTP_RD);
	otp_test_bank(31, 992, 1024, OTP_RD);
	otp_test_bank(32, 1024, 2048, 0x0);

	otp_test_bank(33, 2048, 3072, OTP_RD | OTP_WR);
	otp_test_bank(34, 3072, 4096, OTP_RD);

#endif
}
#else
void otp_test(void)
{
	int i, ret;
	unsigned int key1, key2;
	unsigned int addr = 0;

	printf("otptest\n");

	for(i = 0; i < 4096; i++) {
		addr = i * 32;
		key1 = i;
		key2 = 0;
		ret = otp_write(&key1, 0x1, addr);
		ret = otp_read(&key2, 0x1, addr);
		printf("test addr, write, read\n");
		printf("%x %x %x", addr, key1, key2);
		if(key1 != key2) {
			printf("different\n");
			while(1);
		}
		printf("\n");
		addr += 32;
	}
}
#endif
#endif
