#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pinctrl.h"

static struct pinctrl_chip g_chip;

static int sca200x_doset(int pin_index, unsigned int pin_config)
{
    g_chip.set(pin_index, pin_config);
    return 0;
}

static int sca200x_doget(int pin_index)
{
    g_chip.get(pin_index);
    return 0;
}

static int sca200x_do_list(void)
{
    g_chip.list();
    return 0;
}

static void usage(const char *name)
{
    printf("usage:\n");
    printf("  %s <list>                         : list all pin_config\n", name);
    printf("  %s <get> <pin_index>              : get pin_config\n", name);
    printf("  %s <set> <pin_index> <pin_config> : set pin_config\n", name);

    if(g_chip.usage)
        g_chip.usage(name);
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

    ret = sca200v100_pinctrl_chip(&g_chip, compatible);
    if(!ret)
    {
        printf("sca200v100 pinctrl\n");
        return 0;
    }

    ret = sca200v200_pinctrl_chip(&g_chip, compatible);
    {
        printf("sca200v200 pinctrl\n");
        return 0;
    }

    return -1;
}


int main(int argc, char *argv[])
{
    int ret;

    int pin_index;
    unsigned int pin_config;
    const char *pcmd;

    ret = chip_init();
    if(ret)
    {
        printf("chip err\n");
        return -1;
    }

    if(argc < 2)
    {
        usage(argv[0]);
        return -1;
    }

    pcmd = argv[1];
    if(!strcmp("list", pcmd))
    {
        sca200x_do_list();
    }
    else if(!strcmp("get", pcmd))
    {
        if(argc < 3)
        {
            usage(argv[0]);
            return -1;
        }
        pin_index = strtol(argv[2], NULL, 0);
        sca200x_doget(pin_index);
    }
    else if(!strcmp("set", pcmd))
    {
        if(argc < 4)
        {
            usage(argv[0]);
            return -1;
        }
        pin_index = strtol(argv[2], NULL, 0);
        pin_config = strtol(argv[3], NULL, 0);
        sca200x_doset(pin_index, pin_config);
    }
    else
    {
        usage(argv[0]);
        return -1;
    }

    return 0;
}
