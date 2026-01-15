#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../sc_adc_user.h"

static int dev_init(void)
{
	int devfd = -1;

	devfd = open("/dev/" DEV_NAME_ADC, O_RDWR);
	if(devfd == -1) {
		perror("open adc dev error\n");
		return -1;
	}

	return devfd;
}

static int dev_exit(int fd)
{
	if(fd >= 0 ) {
		close(fd);
	}
	return 0;
}

static int adc_get_raw(int channel)
{
	int devfd = -1;
	int ret;
	struct adc_val adc_arg;

	devfd = dev_init();
	if(devfd < 0) {
		ret = -1;
		goto EXIT;
	}

	adc_arg.chn = channel;
	ret = ioctl(devfd, IOCMD_ADC_GET_RAW, &adc_arg);
	if(ret < 0) {
		printf("ERROR: ioctl ADC GET RAW, CHN %d\n", channel);
		goto EXIT;
	}

	printf("raw: ch %d is %d\n", adc_arg.chn, adc_arg.val);
	ret = 0;

EXIT:
	dev_exit(devfd);

	return ret;
}

static int adc_get_mv(int channel)
{
	int devfd = -1;
	int ret;
	struct adc_val adc_arg;

	devfd = dev_init();
	if(devfd < 0) {
		ret = -1;
		goto EXIT;
	}

	adc_arg.chn = channel;
	ret = ioctl(devfd, IOCMD_ADC_GET_MV, &adc_arg);
	if(ret < 0) {
		printf("ERROR: ioctl ADC GET MV, CHN %d\n", channel);
		goto EXIT;
	}

	printf("millivoltage: ch %d is %dmv\n", adc_arg.chn, adc_arg.val);
	ret = 0;

EXIT:
	dev_exit(devfd);

	return ret;
}

static int adc_power(int enable)
{
	int devfd = -1;
	int ret;

	devfd = dev_init();
	if(devfd < 0) {
		ret = -1;
		goto EXIT;
	}

	ret = ioctl(devfd, IOCMD_ADC_POWER, &enable);
	if(ret < 0) {
		printf("ERROR: ioctl ADC POWER %d\n", enable);
		goto EXIT;
	}

	printf("adc power %s ok\n", enable ? "on" : "off");
	ret = 0;

EXIT:
	dev_exit(devfd);

	return ret;
}

static void usage(void)
{
	printf("usage:\n");
	printf("  ./adcTest.out <cmd> <cmd args>\n");
	printf("\n");
	printf("<cmd>:\n");
	printf("  power <power arg>: power on/off\n");
	printf("  getraw <channel> : get adc raw value\n");
	printf("  getmv <channel>  : get adc millivoltage value\n");
	printf("\n");
	printf("<cmd args>:\n");
	printf("  power arg: 0: power off; 1: power on;\n");
	printf("  channel: 0-7;\n");
	printf("\n\n");
	printf("example:\n");
	printf("  adcTest.out power 1\n");
	printf("  adcTest.out getmv 7\n");
	printf("  adcTest.out power 0\n");

	return;
}

int main(int argc, char **argv)
{
	int ret = 0;

	if(argc < 3) {
		usage();
		return -1;
	}

	if(0 == strcmp(argv[1], "power")) {
		ret = adc_power(atoi(argv[2]));
	} else if(0 == strcmp(argv[1], "getraw")) {
		ret = adc_get_raw(atoi(argv[2]));
	} else if(0 == strcmp(argv[1], "getmv")) {
		ret = adc_get_mv(atoi(argv[2]));
	} else {
		usage();
		return -1;
	}

	return ret;
}
