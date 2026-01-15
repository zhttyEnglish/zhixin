/*
 * SmartChipc tRoot verification driver
 */
#include <common.h>
#include <asm/io.h>
#include "troot.h"

#define TROOT_GPI  0x64530004
#define TROOT_GPO  0x64530008

#define TROOT_REG1 0x60631024

static inline void troot_write_reg32(unsigned long addr, unsigned int value)
{
	writel(value, addr);
}

static inline unsigned int troot_read_reg32(unsigned long addr)
{
	return readl(addr);
}

static void troot_boot(void)
{
	unsigned int data32 = 0x0;

	printf("\n1---\n");
	troot_write_reg32(TROOT_REG1, 0xffffffff);  // release troot rst_n

	printf("\n2--\n");
	while(1) {
		data32 = troot_read_reg32(TROOT_GPO);

		// wait GPO[0] == 1
		if((data32 & 0x1) == 0x1)
			break;
	}
	printf("\n3---\n");
}

static int troot_verify_image(void)
{
	unsigned int data32 = 0x0;

	printf("\n4---\n");
	// write 0x1 to GPI, start ot verity image
	troot_write_reg32(TROOT_GPI, 0x1);

	printf("\n5---\n");
	while(1) {
		data32 = troot_read_reg32(TROOT_GPO);

		// wait GPO[1] or GPO[2] == 1
		if(data32 & 0x6)
			break;
	}

	printf("\n6---\n");
	// write 0x0 to GPI, check verity result
	troot_write_reg32(TROOT_GPI, 0x0);

	printf("\n7---\n");
	// wait GPO[1]/GPO[2] == 1
	data32 = troot_read_reg32(TROOT_GPO);
	data32 &= 0x6;
	if(data32 == 0x2) {
		return 0;//Success
	} else {
		return 1;//Fail
	}
	printf("\n8---\n");
}

int troot_verification(void)
{
	troot_boot();

	return troot_verify_image();
}
