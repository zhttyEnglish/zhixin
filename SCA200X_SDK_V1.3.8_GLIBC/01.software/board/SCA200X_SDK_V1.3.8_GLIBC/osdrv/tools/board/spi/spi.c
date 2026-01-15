#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/spi/spidev.h>

static void dump_buf(const void *buf_t, unsigned int len)
{
    int i;
    const unsigned char *buf = buf_t;

    for(i = 0; i < len; i++)
    {
        if(0 == i %16)
            printf("\n%04x: ", i);
        printf(" %02x", buf[i]);
    }
    printf("\n");

    return;
}

static int spidev_hw_init(const char *file_name)
{
    unsigned int value;
    int ret = 0;
    int g_fd = -1;

    g_fd = open(file_name, 0);
    if (g_fd < 0)
    {
        printf("Open %s error!\n",file_name);
        return -1;
    }

    ret = ioctl(g_fd, SPI_IOC_RD_MODE32, &value);
    if (ret < 0)
    {
        printf("ioctl SPI_IOC_RD_MODE32 err, ret = %d\n", ret);
        goto EXIT;
    }
    printf("SPI_IOC_RD_MODE32 value = 0x%x\n", value);

    /* imx290 spi need SPI_LSB_FIRST.
     * sca200x do not support SPI_LSB_FIRST
     */
    value = SPI_MODE_3;
    //value |= SPI_LSB_FIRST;
    ret = ioctl(g_fd, SPI_IOC_WR_MODE32, &value);
    if (ret < 0)
    {
        printf("ioctl SPI_IOC_WR_MODE32 err, value = 0x%x ret = %d\n", value, ret);
        goto EXIT;
    }

    value = 8;
    ret = ioctl(g_fd, SPI_IOC_WR_BITS_PER_WORD, &value);
    if (ret < 0)
    {
        printf("ioctl SPI_IOC_WR_BITS_PER_WORD err, value = %d ret = %d\n",value, ret);
        goto EXIT;
    }

#if 1
    value = 2000*1000;
    ret = ioctl(g_fd, SPI_IOC_WR_MAX_SPEED_HZ, &value);
    if (ret < 0)
    {
        printf("ioctl SPI_IOC_WR_MAX_SPEED_HZ err, value = %d ret = %d\n",value, ret);
        goto EXIT;
    }
#endif

    ret = 0;

EXIT:
    close(g_fd);
    return ret;
}

static int spidev_message(const char *fdev, int inlen, int outc, const char *outv[])
{
    int ret = 0;
    int i;
    struct spi_ioc_transfer mesg[1];
    unsigned char  tx_buf[256] = {0};
    unsigned char  rx_buf[256] = {0};
    int g_fd = -1;

    if(outc > sizeof(tx_buf))
        printf("error: outc %d, too big\n", outc);
    if(inlen > sizeof(rx_buf))
        printf("error: inlen %d, too big\n", inlen);

    spidev_hw_init(fdev);
    g_fd = open(fdev, 0);
    if(g_fd < 0)
    {
        printf("Open %s error!\n", fdev);
        return -1;
    }

    for(i = 0; i < outc; i++)
    {
        tx_buf[i] = strtol(outv[i], NULL, 0);
        printf("tx[%d]: 0x%02x\n", i, tx_buf[i]);
    }

    memset(mesg, 0, sizeof(mesg));
    mesg[0].tx_buf    = (unsigned long)tx_buf;
    mesg[0].len       = outc+inlen;

    mesg[0].rx_buf    = (unsigned long)rx_buf;
    mesg[0].len       = outc+inlen;
    mesg[0].cs_change = 0;

    ret = ioctl(g_fd, SPI_IOC_MESSAGE(1), mesg);
    if(ret < 0)
    {
        printf("SPI_IOC_MESSAGE error \n");
        goto EXIT;
    }

    dump_buf(rx_buf + outc, inlen);

    ret = 0;
EXIT:
    close(g_fd);
    return ret;
}

static void usage(const char *name)
{
    printf("Usage:\n");
    printf("  %s <fdev> <read_byte_num> <write_data ...>\n", name);
    printf("example: read spi nor flash id\n");
    printf("  %s /dev/spidev0.0 3 0x9f\n", name);
}

static int spidev_do(int argc, const char *argv[])
{
    int ret;
    unsigned int inlen;
    const char * fdev;

    if(argc < 3)
    {
        usage("spidev");
        return -1;
    }

    fdev = argv[1];
    inlen = strtol(argv[2], NULL, 0);
    ret = spidev_message(fdev, inlen, argc -3, argv + 3);
    if(ret)
    {
        printf("err spi read\n");
        ret = -1;
        goto EXIT;
    }

    ret = 0;

EXIT:
    return ret;
}

int main(int argc, const char *argv[])
{
    spidev_do(argc, argv);

    return 0;
}
