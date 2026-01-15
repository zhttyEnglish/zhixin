#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

typedef struct
{
    unsigned char   dev_addr;      /* I2C 从设备地址, 读写地址都可以，驱动中自动改写读写位 */
    unsigned char   addr_byte_num; /* I2C 从设备寄存器地址字节数 */
    unsigned char   data_byte_num; /* I2C 从设备寄存器数据字节数 */
    unsigned char   reg_addr_step; /* I2C 从设备寄存器地址增加步长 */
} I2cSlaveInfo;

typedef struct
{
    unsigned int index;
    I2cSlaveInfo slave;
} I2cDevInfo;

static void i2c_dump_register(
    const I2cSlaveInfo *pdev,
    unsigned int reg_start_addr,
    unsigned int cnt,
    const unsigned int *value)
{
    int i;
    char addr_fmt[16];
    char val_fmt[16];

    sprintf(addr_fmt, "\n%%0%dx:", pdev->addr_byte_num * 2);
    sprintf(val_fmt, " %%0%dx", pdev->data_byte_num * 2);

    printf("dump reg: 0x%x, cnt:%d\n", reg_start_addr, cnt);
    for(i = 0; i < cnt; i++)
    {
        if(0 == i % 16)
        {
            printf(addr_fmt, reg_start_addr + i * pdev->reg_addr_step);
        }
        printf(val_fmt, value[i]);
    }
    printf("\n");

}

static int i2c_dev_open(const I2cDevInfo *pdev)
{
    int ret = -1;
    int fd  = -1;
    char path[] = {"/dev/i2c-30"};

    sprintf(path, "/dev/i2c-%d", pdev->index);
    /*< step1: 打开i2c 设备 */
    fd = open(path, O_RDWR);
    if(fd < 0)
    {
        printf("Open %s error!\n", path);
        goto EXIT;
    }

    /*< step2.1:设置设备地址*/
    ret = ioctl(fd, I2C_SLAVE_FORCE, (pdev->slave.dev_addr >> 1));
    if(ret < 0)
    {
        printf("CMD_SET_DEV error!\n");
        goto EXIT;
    }

    ret = 0;

EXIT:
    if(ret)
    {
        if(fd >= 0)
        {
            close(fd);
            fd = -1;
        }
    }

    return fd;
}


static int i2c_read_register(
    const I2cDevInfo *pdev,
    unsigned int reg_addr,
    int len,
    unsigned int *value)
{
    int i   = 0;
    int ret = -1;
    int fd  = -1;
    unsigned char recvbuf[4] = {};
    struct i2c_rdwr_ioctl_data rdwr;
    struct i2c_msg msg[2];

    if((NULL == pdev) || (NULL == value))
    {
        printf("ERROR: param is NULL!\n");
        goto EXIT;
    }

    /*< step1: 打开i2c 设备 */
    fd = i2c_dev_open(pdev);
    if(fd < 0)
    {
        printf("init /dev/i2c-x error!\n");
        goto EXIT;
    }

    msg[0].addr  = (pdev->slave.dev_addr >> 1);
    msg[0].flags = 0;
    msg[0].len   = pdev->slave.addr_byte_num;
    msg[0].buf   = recvbuf;

    msg[1].addr  = (pdev->slave.dev_addr >> 1);
    msg[1].flags = I2C_M_RD;
    msg[1].len   = pdev->slave.data_byte_num;
    msg[1].buf   = recvbuf;

    rdwr.msgs    = &msg[0];
    rdwr.nmsgs   = (__u32)2;

    /*< step2: 调用read/write 进行读写*/
    for(i = 0; i < len; i++, reg_addr += pdev->slave.reg_addr_step)
    {
        if(2 == pdev->slave.addr_byte_num)
        {
            recvbuf[0] = (reg_addr >> 8) & 0xff;
            recvbuf[1] = reg_addr & 0xff;
        }
        else
        {
            recvbuf[0] = reg_addr & 0xff;
        }

        ret = ioctl(fd, I2C_RDWR, &rdwr);
        if(ret != 2)
        {
            printf("CMD_I2C_READ error, ret=%d!\n", ret);
            perror("CMD_I2C_READ error!\n");
            ret = -1;
            goto EXIT;
        }

        if(2 == pdev->slave.data_byte_num)
        {
            *value++ = recvbuf[1] | (recvbuf[0] << 8);
        }
        else
        {
            *value++ = recvbuf[0];
        }

        //printf("Read data 0x%x:0x%x\n", reg_addr, *(value-1));
    }

    ret = 0;

EXIT:
    if(fd >= 0)
    {
        close(fd);
    }

    return ret;
}

static int i2c_write_register(const I2cDevInfo *pdev, unsigned int reg_addr, unsigned int data)
{
    int ret = -1;
    int fd = -1;
    int idx = 0;
    char buf[4] = {};

    if(NULL == pdev)
    {
        printf("ERROR: param is NULL!\n");
        goto EXIT;
    }

    /*< step1: 打开i2c设备*/
    fd = i2c_dev_open(pdev);
    if(fd < 0)
    {
        printf("init /dev/i2c-x error!\n");
        goto EXIT;
    }

    /*< step2. 调用write写寄存器*/
    if (2 == pdev->slave.addr_byte_num)
    {
        buf[idx++] = (reg_addr >> 8) & 0xff;
        buf[idx++] = reg_addr & 0xff;
    }
    else
    {
        buf[idx++] = reg_addr & 0xff;
    }

    if (2 == pdev->slave.data_byte_num)
    {
        buf[idx++] = (data >> 8) & 0xff;
        buf[idx++] = data & 0xff;
    }
    else
    {
        buf[idx++] = data & 0xff;
    }

    ret = write(fd, buf, idx);
    if(ret < 0)
    {
        printf("I2C_WRITE error!\n");
        goto EXIT;
    }

    ret = 0;

EXIT:
    /*< step3. 关i2c设备*/
    if(fd >= 0)
    {
        close(fd);
    }
    return ret;

}

static void usage(const char *name)
{
    printf("Usage:\n");
    printf("  %s read <i2cdev_index> "
        "<i2c_salve_addr> <addr_byte_num> <data_byte_num> "
        "<reg_addr> <reg_cnt> <reg_addr_step>\n", name);
    printf("  %s write <i2cdev_index> "
        "<i2c_salve_addr> <addr_byte_num> <data_byte_num> "
        "<reg_addr> <reg_value>\n", name);
    printf("note:\n");
    printf(" i2c_slave_addr: 8bit\n");
    printf("\n");
    printf("example:");
    printf("  %s read 0 "
        "0x34 2 1 "
        "0x3000 256 1\n", name);
}

static int i2c_do_read(int argc, char *argv[])
{
    int ret;
    I2cDevInfo dev;
    unsigned int reg_start_addr;
    unsigned int reg_cnt;
    unsigned int *vv = NULL;
    unsigned int sz;

    if(argc < 7)
    {
        usage("i2c");
        return -1;
    }

    memset(&dev, 0, sizeof(dev));

    dev.index = strtol(argv[0], NULL, 0);
    dev.slave.dev_addr      = strtol(argv[1], NULL, 0);
    dev.slave.addr_byte_num = strtol(argv[2], NULL, 0);
    dev.slave.data_byte_num = strtol(argv[3], NULL, 0);

    reg_start_addr          = strtol(argv[4], NULL, 0);
    reg_cnt                 = strtol(argv[5], NULL, 0);
    dev.slave.reg_addr_step = strtol(argv[6], NULL, 0);

    sz = sizeof(*vv) * reg_cnt;
    vv = malloc(sz);
    if(!vv)
    {
        printf("err malloc %d\n", sz);
        ret = -1;
        goto EXIT;
    }

    memset(vv, 0, sz);
    ret = i2c_read_register(&dev, reg_start_addr, reg_cnt, vv);
    if(ret)
    {
        printf("err i2c_read_register\n");
        ret = -1;
        goto EXIT;
    }

    i2c_dump_register(&dev.slave, reg_start_addr, reg_cnt, vv);
    ret = 0;

EXIT:
    if(vv)
    {
        free(vv);
    }
    return ret;
}

static int i2c_do_write(int argc, char *argv[])
{
    int ret;
    I2cDevInfo dev;
    unsigned int reg_addr;
    unsigned int reg_value;

    if(argc < 6)
    {
        usage("i2c");
        return -1;
    }

    memset(&dev, 0, sizeof(dev));

    dev.index = strtol(argv[0], NULL, 0);
    dev.slave.dev_addr      = strtol(argv[1], NULL, 0);
    dev.slave.addr_byte_num = strtol(argv[2], NULL, 0);
    dev.slave.data_byte_num = strtol(argv[3], NULL, 0);

    reg_addr  = strtol(argv[4], NULL, 0);
    reg_value = strtol(argv[5], NULL, 0);

    ret = i2c_write_register(&dev, reg_addr, reg_value);
    if(ret)
    {
        printf("err i2c_write_register\n");
        ret = -1;
        goto EXIT;
    }

    ret = 0;

EXIT:
    return ret;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        usage(argv[0]);
        return -1;
    }

    if(0 == strcmp("read", argv[1]))
    {
        i2c_do_read(argc - 2, argv + 2);
    }
    else if(0 == strcmp("write", argv[1]))
    {
        i2c_do_write(argc - 2, argv + 2);
    }
    else
    {
        usage(argv[0]);
        return -1;
    }

    return 0;
}
