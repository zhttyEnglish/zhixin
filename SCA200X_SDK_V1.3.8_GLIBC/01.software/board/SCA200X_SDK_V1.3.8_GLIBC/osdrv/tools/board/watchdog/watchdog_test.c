#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/watchdog.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif/*__cplusplus*/


#define DEV_FILE "/dev/watchdog"

#define PIOER(fd, ret) do { \
    printf("[Error]%s(%d): ioctl %s\n", __FUNCTION__, __LINE__, strerror(errno)); \
    close(fd); \
}while(0)

static int get_timeout(void)
{
    int ret;
    int fd;
    int val;

    fd = open(DEV_FILE, O_RDWR);
    if (fd < 0) {
        printf("fail to open file:%s\n", DEV_FILE);
        return -1;
    }

    ret = ioctl(fd, WDIOC_GETTIMEOUT, &val);
    if (0 != ret) {
        PIOER(fd, -1);
        return -1;
    }

    printf("get timeout=%ds\n", val);
    close(fd);
    return 0;
}

static int get_timeleft(void)
{
    int ret;
    int fd;
    int val;

    fd = open(DEV_FILE, O_RDWR);
    if (fd < 0) {
        printf("fail to open file:%s\n", DEV_FILE);
        return -1;
    }

    ret = ioctl(fd, WDIOC_GETTIMELEFT, &val);
    if (0 != ret) {
        PIOER(fd, -1);
        return -1;
    }

    printf("get timeleft=%ds\n", val);
    close(fd);
    return 0;
}

static int get_status(void)
{
    int ret;
    int fd;
    int val;

    fd = open(DEV_FILE, O_RDWR);
    if (fd < 0) {
        printf("fail to open file:%s\n", DEV_FILE);
        return -1;
    }

    ret = ioctl(fd, WDIOC_GETSTATUS, &val);
    if (0 != ret) {
        PIOER(fd, -1);
        return -1;
    }

    printf("get status=0x%x\n", val);
    close(fd);
    return 0;
}

static int get_support(void)
{
    int ret;
    int fd;
    struct watchdog_info info = {};

    fd = open(DEV_FILE, O_RDWR);
    if (fd < 0) {
        printf("fail to open file:%s\n", DEV_FILE);
        return -1;
    }

    ret = ioctl(fd, WDIOC_GETSUPPORT, &info);
    if (0 != ret) {
        PIOER(fd, -1);
        return -1;
    }

    printf("get: info.option=0x%x\n", info.options);
    printf("get: info.firmware_version=%d\n", info.firmware_version);
    printf("get: info.identity=%.*s\n", (int)sizeof(info.identity), info.identity);
    close(fd);
    return 0;
}

static int set_timeout(int val)
{
    int ret;
    int fd;

    fd = open(DEV_FILE, O_RDWR);
    if (fd < 0) {
        printf("fail to open file:%s\n", DEV_FILE);
        return -1;
    }

    ret = ioctl(fd, WDIOC_SETTIMEOUT, &val);
    if (0 != ret) {
        PIOER(fd, -1);
        return -1;
    }

    close(fd);
    return 0;
}

static int set_option(int val)
{
    int ret;
    int fd;

    fd = open(DEV_FILE, O_RDWR);
    if (fd < 0) {
        printf("fail to open file:%s\n", DEV_FILE);
        return -1;
    }

    printf("set new option :%d \n", val);
    ret = ioctl(fd, WDIOC_SETOPTIONS, &val);
    if (0 != ret) {
        PIOER(fd, -1);
        return -1;
    }

    close(fd);
    return 0;
}

static int wtdg_feed(void)
{
    int ret;
    int fd;
    int val = 0;

    fd = open(DEV_FILE, O_RDWR);
    if (fd < 0) {
        printf("fail to open file:%s\n", DEV_FILE);
        return -1;
    }

    ret = ioctl(fd, WDIOC_KEEPALIVE, &val);
    if (0 != ret) {
        PIOER(fd, -1);
        return -1;
    }

    close(fd);
    return 0;
}

static void print_help(void)
{
    printf(
        "\n"
        "Usage: ./test [options] ...\n"
        "Options: \n"
        "    -s timout <val>   : set timeout\n"
        "    -s option <val>   : %d: stop, %d:start\n"
        "\n"
        "    -g timeout        : get timeout\n"
        "    -g timeleft       : get timeleft\n"
        "    -g status         : get status.\n"
        "    -g support        : get support info\n"
        "\n"
        "    -f                : feed\n"
        "\ne.g. \n"
        " ./test -s timeout 30\n"
        " ./test -g timeout\n"
        " ./test -f\n",
        WDIOS_DISABLECARD, WDIOS_ENABLECARD);
}

static int parse_args_and_proc(int argc, char *argv[])
{
    int ret = 0;
    int val;

    if (1 == argc) {
        print_help();
        return -1;
    }

    if ((0 == strcmp("-s", argv[1])) && (4 == argc)) {
        if (0 == strcmp("timeout", argv[2])) {
            val = atoi(argv[3]);
            ret = set_timeout(val);
        } else if (0 == strcmp("option", argv[2])) {
            val = atoi(argv[3]);
            ret = set_option(val);
        } else {
            print_help();
        }
    } else if ((0 == strcmp("-g", argv[1])) && (3 == argc)) {
        if (0 == strcmp("timeout", argv[2])) {
            ret = get_timeout();
        }
        if (0 == strcmp("timeleft", argv[2])) {
            ret = get_timeleft();
        }
        if (0 == strcmp("status", argv[2])) {
            ret = get_status();
        }
        if (0 == strcmp("support", argv[2])) {
            ret = get_support();
        }
    } else if ((0 == strcmp("-f", argv[1])) && (2 == argc)) {
        ret = wtdg_feed();
    } else {
        print_help();
    }
    return ret;
}

int main(int argc, char *argv[])
{

    parse_args_and_proc(argc, argv);

    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif/*__cplusplus*/

