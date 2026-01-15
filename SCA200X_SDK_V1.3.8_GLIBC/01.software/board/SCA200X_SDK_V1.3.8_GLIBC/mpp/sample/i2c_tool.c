#include <stdio.h> 
#include <linux/types.h> 
#include <stdlib.h> 
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/ioctl.h> 
#include <errno.h> 
#include <assert.h> 
#include <string.h> 
#include <linux/i2c.h> 
#include <linux/i2c-dev.h>


static int i2c_write_bytes(int fd, uint8_t slave_addr, uint8_t reg_addr, uint8_t *values, uint8_t len)
{
    uint8_t *outbuf = NULL;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    outbuf = malloc(len + 1);
    if (!outbuf) {
        printf("Error: No memory for buffer\n");
        return -1;
    }

    outbuf[0] = reg_addr;
    memcpy(outbuf + 1, values, len);
    
    messages[0].addr = slave_addr;
    messages[0].flags = 0;
    messages[0].len = len + 1;
    messages[0].buf = outbuf;
    
    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs = messages;
    packets.nmsgs = 1;
    if(ioctl(fd, I2C_RDWR, &packets) < 0)
    {
        printf("Error: Unable to send data");
        free(outbuf);
        return -1;
    }

    free(outbuf);
    
    return 0;
}


static int i2c_read_bytes(int fd, uint8_t slave_addr, uint8_t reg_addr, uint8_t *values, uint8_t len)
{
    uint8_t outbuf[1];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    outbuf[0] = reg_addr;
    messages[0].addr = slave_addr;
    messages[0].flags = 0;
    messages[0].len = sizeof(outbuf);
    messages[0].buf = outbuf;
    
    /* The data will get returned in this structure */
    messages[1].addr = slave_addr;
    messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
    messages[1].len = len;
    messages[1].buf = values;
    
    /* Send the request to the kernel and get the result back */
    packets.msgs = messages;
    packets.nmsgs = 2;
    if(ioctl(fd, I2C_RDWR, &packets) < 0)
    {
        printf("Error: Unable to send data");
        return -1;
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    int fd;
    bool cmdIsRd = false;
    char *arg_ptr = NULL;
    unsigned long len;
    unsigned int slave_addr, reg_addr;
    uint8_t buffer[1024];

    /* 1.判断命令行参数是否合法 */
    if(argc < 5){
        printf("Usage:\n");
        printf("%s /dev/i2c-x {r|w}length salve_addr reg_addr [value]\n", argv[0]);
        return -1;
    }

    /* 2.打开I2C总线 */
    fd = open(argv[1], O_RDWR); 
    if (fd < 0)
    { 
        printf("can not open file %s\n", argv[0]);
        return -1; 
    }

    /* 3.解析命令的方向 */
    arg_ptr = argv[2];
    switch (*arg_ptr) {
    case 'r': cmdIsRd = true; break;
    case 'w': cmdIsRd = false; break;
    default:
        printf("Error: Invalid direction\n");
        return -1; 
    }

    /* 4.解析value的长度 */
    arg_ptr++;
    len = strtoul(arg_ptr, NULL, 0);

    /* 5.解析从机地址和寄存器地址 */
    slave_addr = strtoul(argv[3], NULL, 0);
    reg_addr = strtoul(argv[4], NULL, 0);

    // printf("%c len=%d, salve_addr=0x%02X, reg_addr=0x%02X\n", cmdIsRd?'r':'w', len, slave_addr, reg_addr);

    /* 6.读写数据 */
    if(cmdIsRd)
    {
        i2c_read_bytes(fd, slave_addr, reg_addr, buffer, len);

        printf("read data =");
        for(int i = 0; i < len; i++)
        {
            printf("0x%02X ", buffer[i]);
        }
        printf("\n");
    }
    else if(argc > 5)
    {
        for(int i = 0; i < len; i++)
        {
            buffer[i] = strtoul(argv[5 + i], NULL, 0);
        }
    
        i2c_write_bytes(fd, slave_addr, reg_addr, buffer, len);
    }

    return 0;
}


