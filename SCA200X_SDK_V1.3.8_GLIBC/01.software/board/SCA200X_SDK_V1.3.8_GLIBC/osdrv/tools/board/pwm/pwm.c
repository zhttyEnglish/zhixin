#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/****************************************************************
* Constants
****************************************************************/

/*
 *1: /sys/class/pwm/pwmchip0/export
 *2: /sys/class/pwm/pwmchip0/unexport
 *
 *pe: /sys/class/pwm/pwmchip0/pwm0/period
 *du: /sys/class/pwm/pwmchip0/pwm0/duty_cycle
 *en: /sys/class/pwm/pwmchip0/pwm0/enable
 */

#define SYSFS_PWM_DIR "/sys/class/pwm/pwmchip0"
#define MAX_STR_BUF 64

#define PWM_MAX 8

/****************************************************************
 * sysfs_pwm_export
 ****************************************************************/
static int sysfs_pwm_export(unsigned int pwm)
{
    int fd, len;
    char buf[MAX_STR_BUF];

    fd = open(SYSFS_PWM_DIR "/export", O_WRONLY);
    if (fd < 0)
    {
        perror("pwm/export");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", pwm);
    write(fd, buf, len);
    close(fd);

    return 0;
}

/****************************************************************
 * sysfs_pwm_unexport
 ****************************************************************/
static int sysfs_pwm_unexport(unsigned int pwm)
{
    int fd, len;
    char buf[MAX_STR_BUF];

    fd = open(SYSFS_PWM_DIR "/unexport", O_WRONLY);
    if (fd < 0)
    {
        perror("pwm/export");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", pwm);
    write(fd, buf, len);
    close(fd);
    return 0;
}

/****************************************************************
 * sysfs_pwm_set_period
 ****************************************************************/
static int sysfs_pwm_set_period(unsigned int pwm, unsigned int period)
{
    int fd, len;
    char buf[MAX_STR_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_PWM_DIR  "/pwm%d/period", pwm);
    if(len < 0)
    {
        perror("pwm/period");
        return len;
    }
    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("pwm/period");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%u", period);
    write(fd, buf, len);

    close(fd);
    return 0;
}

/****************************************************************
 * sysfs_pwm_set_duty
 ****************************************************************/
static int sysfs_pwm_set_duty(unsigned int pwm, unsigned int duty)
{
    int fd, len;
    char buf[MAX_STR_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_PWM_DIR  "/pwm%d/duty_cycle", pwm);
    if(len < 0)
    {
        perror("pwm/duty");
        return len;
    }

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("pwm/duty");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%u", duty);
    write(fd, buf, len);
    close(fd);
    return 0;
}

/****************************************************************
 * sysfs_pwm_set_enable
 ****************************************************************/
static int sysfs_pwm_set_enable(unsigned int pwm, char en)
{
    int fd, len;
    char buf[MAX_STR_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_PWM_DIR  "/pwm%d/enable", pwm);
    if(len < 0)
    {
        perror("pwm/set-en");
        return len;
    }

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("pwm/set-en");
        return fd;
    }

    if (en)
        write(fd, "1", 2);
    else
        write(fd, "0", 2);
    close(fd);
    return 0;
}

static void usage(const char *name)
{
    printf("usage:\n");
    printf("  %s <pwm num>  <period ns>  <duty ns> <enable>: set pwm\n", name);
    printf("\n");
    printf("example:\n");
    printf("  %s 0 20000000 10000000 1: period 20ms, duty 10ms. 50Hz, 50%%\n", name);
}

/* 使用 sysfs 读写 pwm */
int main(int argc, char *argv[])
{
    int ret;
    unsigned int pwm_num;
    unsigned int period;
    unsigned int duty;
    unsigned int enable;

    if(argc < 5)
    {
        usage(argv[0]);
        return -1;
    }

    pwm_num = strtol(argv[1], NULL, 0);
    period = strtol(argv[2], NULL, 0);
    duty = strtol(argv[3], NULL, 0);
    enable = strtol(argv[4], NULL, 0);

    ret = sysfs_pwm_export(pwm_num);
    if(ret)
        return -1;
    ret = sysfs_pwm_set_period(pwm_num, period);
    if(ret)
        return -1;
    ret = sysfs_pwm_set_duty(pwm_num, duty);
    if(ret)
        return -1;
    ret = sysfs_pwm_set_enable(pwm_num, enable);
    if(ret)
        return -1;
    sysfs_pwm_unexport(pwm_num);

    return 0;
}
