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

#include "../gpio_test_user.h"

static int dev_init(void)
{
	int devfd = -1;

	devfd = open("/dev/" DEV_NAME_GPIO_TEST, O_RDWR);
	if(devfd == -1) {
		perror("open log dev error\n");
		return -1;
	}

	return devfd;
}

static int dev_exit(int fd)
{
	if(fd >= 0 ) {
		close(fd);
	}
}

static int gpio_test_get(int devfd, struct gpio_drv *pgpio)
{
	int ret;

	ret = ioctl(devfd, DRV_GPIO_GET, pgpio);
	if(ret < 0) {
		perror("error DRV_GPIO_GET\n");
		return -1;
	}

	printf("get gpio%d_%d is %d\n", pgpio->group, pgpio->pin, pgpio->val);

	return 0;
}

static int gpio_test_set(int devfd, struct gpio_drv *pgpio)
{
	int ret;

	ret = ioctl(devfd, DRV_GPIO_SET, pgpio);
	if(ret < 0) {
		perror("error DRV_GPIO_SET\n");
		return -1;
	}

	printf("set gpio%d_%d to %d\n", pgpio->group, pgpio->pin, pgpio->val);

	return 0;
}

static void usage(void)
{
	printf("gpiotest <group> <port> [val]\n");
	printf("example:\n");
	printf("  gpiotest 0 5\n");
	printf("  gpiotest 0 5 1\n");
}

static int gpio_do(int devfd, int argc, char **argv)
{
	struct gpio_drv gpio;

	if(argc < 3) {
		usage();
		return -1;
	}

	gpio.group = atoi(argv[1]);
	gpio.pin = atoi(argv[2]);
	if(argc > 3) {
		gpio.val = atoi(argv[3]);
		gpio_test_set(devfd, &gpio);
	} else {
		gpio_test_get(devfd, &gpio);
	}

	return 0;
}

int main(int argc, char **argv)
{
	int devfd = -1;
	int ret;

	devfd = dev_init();
	if(devfd < 0) {
		goto EXIT;
	}

	gpio_do(devfd, argc, argv);

EXIT:
	if(devfd >= 0) {
		dev_exit(devfd);
	}

	return 0;
}
