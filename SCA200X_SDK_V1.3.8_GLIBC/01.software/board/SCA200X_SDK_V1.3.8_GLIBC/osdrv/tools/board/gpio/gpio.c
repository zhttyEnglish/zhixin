#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "gpio.h"

/****************************************************************
* Constants
****************************************************************/
/*
 * 1. /sys/class/gpio/export
 * 2. /sys/class/gpio/unexport
 *
 * > /sys/class/gpio/gpioN/value
 * > /sys/class/gpio/gpioN/direction
 */

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_GPIO_REC_BUF 64

typedef enum
{
    INTPUT = 0,
    OUTPUT,
    HIGH,
    LOW,
    DIR_MAX
} ENUM_GPIO_DIR;

static struct gpio_chip g_chip;

static int sysfs_gpio_export(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];

    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);

    return 0;
}

static int sysfs_gpio_unexport(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];

    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

static int sysfs_gpio_set_dir(unsigned int gpio, ENUM_GPIO_DIR dir)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
    if(len < 0)
    {
        perror("gpio/direction");
        return len;
    }
    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/direction");
        return fd;
    }

    switch(dir)
    {
    case INTPUT:
        write(fd, "in", 3);
        break;

    case OUTPUT:
        write(fd, "out", 4);
        break;

    case HIGH:
        write(fd, "high", 4);
        break;

    case LOW:
        write(fd, "low", 3);
        break;
    default:
        break;
    }

    close(fd);
    return 0;
}

static int sysfs_gpio_set_value(unsigned int gpio, unsigned int value)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    if(len < 0)
    {
        perror("gpio/set-value");
        return len;
    }

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/set-value");
        return fd;
    }

    if (value)
        write(fd, "1", 2);
    else
        write(fd, "0", 2);

    close(fd);
    return 0;
}

static int sysfs_gpio_get_value(unsigned int gpio, unsigned int *value)
{
    int fd, len;
    char buf[MAX_GPIO_REC_BUF];
    char ch;

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    if(len < 0)
    {
        perror("gpio/get-value");
        return len;
    }

    fd = open(buf, O_RDONLY);
    if (fd < 0)
    {
        perror("gpio/get-value");
        return fd;
    }

    read(fd, &ch, 1);

    if (ch != '0')
    {
        *value = 1;
    }
    else
    {
        *value = 0;
    }

    close(fd);
    return 0;
}

/*
 * GPIO name: <GROUP>_<PIN>
 *   GROUP:[0-3]. base is 32.
 *   PIN:[0-31].  base is 1.
 */
static int sysfs_gpio_name_to_num(unsigned int group, unsigned int pin)
{
    return g_chip.sysfs_gpio_nr(group, pin);
}

/*
 * sca200v100:
 * total: 4*32 = 128 个 gpio
 * GPIO name: <GROUP>_<PIN>
 *   GROUP:[0-3].
 *   PIN:[0-31].
 *
 * 例如 GPIO 3_31:
 */
/*
 * sca200v200:
 * total: 109 个 gpio
 * GPIO name: <GROUP>_<PIN>
 *   GROUP:[0-6]
 *   PIN:[0-31].
 *
 * 例如 GPIO 6_15:
 */
static int str2gpio(const char *gpio_name)
{
    unsigned int group;
    unsigned int pin;
    int tmp;
    char *endptr;

    tmp = strtol(gpio_name, &endptr, 10);
    if(tmp < 0 || tmp >= g_chip.group_max)
    {
        printf("group invalid, need [0-%d]\n", g_chip.group_max - 1);
        return -1;
    }
    group = tmp;

    if(!endptr)
    {
        printf("pin invalid\n");
        return -1;
    }
    tmp = strtol(endptr + 1, &endptr, 10);
    if(tmp < 0 )
    {
        printf("pin invalid\n");
        return -1;
    }
    pin = tmp;

    return sysfs_gpio_name_to_num(group, pin);
}

static int chip_init(void)
{
    int ret;
    char compatible[256];
    int fd;

    fd = open("/proc/device-tree/compatible", O_RDONLY);
    if (fd < 0)
    {
        perror("open dts");
        return fd;
    }

    memset(compatible, 0, sizeof(compatible));
    ret = read(fd, compatible, sizeof(compatible));
    if (ret < 0)
    {
        perror("read dts");
        close(fd);
        return -1;
    }

    close(fd);

    ret = sca200v100_gpio_chip(&g_chip, compatible);
    if(!ret)
    {
        printf("sca200v100 gpio\n");
        return 0;
    }

    ret = sca200v200_gpio_chip(&g_chip, compatible);
    {
        printf("sca200v200 gpio\n");
        return 0;
    }

    return -1;
}

static void usage(const char *name)
{
    printf("usage:\n");
    printf("  %s <gpio name>        : set gpio in;  get gpio value\n", name);
    printf("  %s <gpio name> <value>: set gpio out; set gpio value\n", name);
    printf("\n");
    printf("sca200v100 GPIO name: <GROUP>_<PIN>\n");
    printf("  GROUP:[0-3].\n");
    printf("  PIN:[0-31].\n");
    printf("  example: 0_0, 3_31\n");
    printf("\n");
    printf("sca200v200 GPIO name: <GROUP>_<PIN>\n");
    printf("  GROUP:[0-6]\n");
    printf("  PIN:[].\n");
    printf("  example: 0_0, 6_17\n");
    printf("\n");
    printf("example:\n");
    printf("  %s 0_0 \n", name);
    printf("  %s 3_31 1\n", name);
}

/* 使用 sysfs 读写 gpio */
int main(int argc, char *argv[])
{
    int ret;
    unsigned int gpio_num;
    unsigned int value;

    if(argc < 2)
    {
        usage(argv[0]);
        return -1;
    }

    ret = chip_init();
    if(ret)
    {
        printf("chip err\n");
        return -1;
    }

    ret = str2gpio(argv[1]);
    if(ret < 0)
    {
        return -1;
    }
    gpio_num = ret;
    printf("GPIO%s (%d)\n", argv[1], gpio_num);

    sysfs_gpio_export(gpio_num);
    if(argc <= 2 )
    {
        ret = sysfs_gpio_set_dir(gpio_num, INTPUT);
        if(ret)
            return -1;
        ret = sysfs_gpio_get_value(gpio_num, &value);
        if(ret)
            return -1;
        printf("get GPIO%s (%d) = %d\n", argv[1], gpio_num, value);
    }
    else
    {
        value = strtol(argv[2], NULL, 0);

        if(value)
            ret = sysfs_gpio_set_dir(gpio_num, HIGH);
        else
            ret = sysfs_gpio_set_dir(gpio_num, LOW);
        if(ret)
            return -1;

        printf("set GPIO%s (%d) = %d\n", argv[1], gpio_num, value);
    }
    sysfs_gpio_unexport(gpio_num);

    return 0;
}
