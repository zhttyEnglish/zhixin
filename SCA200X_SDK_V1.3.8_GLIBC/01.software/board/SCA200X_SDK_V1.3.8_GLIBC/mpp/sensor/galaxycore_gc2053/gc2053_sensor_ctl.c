/**
 * @file     gc2053_sensor_ctl.c
 * @brief    SENSOR初始化接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2022-02-08 创建文件
 */
/************************************************************
 *@note
    Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
                   ALL RIGHTS RESERVED
    Permission is hereby granted to licensees of  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. to use
    or abstract this computer program for the sole purpose of implementing a product based on BEIJIING SMARTCHIP
    MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use, or disseminate this computer program,
    whether in part or in whole, are granted. BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no
    representation or warranties with respect to the performance of this computer program, and specifically disclaims
    any responsibility for any damages, special or consequential, connected with the use of this program.
　　For details, see http://www.sgitg.sgcc.com.cn/
**********************************************************/


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "sc_comm_video.h"
#include "sc_sns_ctrl.h"
#include "sc_i2c.h"


#define GC2053_FLIP_MIRROR             (0x17)
#define SENSOR_1080P_30FPS_LINEAR_MODE (0)

const unsigned char gc2053_i2c_addr  = 0x6e;  /* I2C Address of GC2053: 0x6e or 0x7e */
const unsigned int  gc2053_addr_byte = 1;
const unsigned int  gc2053_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};
static SC_BOOL g_bStandby[ISP_MAX_PIPE_NUM] = {0};

extern ISP_SNS_STATE_S  *g_pastGc2053[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunGc2053BusInfo[];


int gc2053_linear_1080p30_init(VI_PIPE ViPipe);


int gc2053_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {
        return SC_SUCCESS;
    }

    u8DevNum = g_aunGc2053BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (gc2053_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int gc2053_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int gc2053_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:
    return SC_SUCCESS;
}

int gc2053_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(gc2053_addr_byte == 2)
    {
        buf[idx] = (addr >> 8) & 0xff;
        idx++;
        buf[idx] = addr & 0xff;
        idx++;
    }
    else
    {
        buf[idx] = addr & 0xff;
        idx++;
    }

    if(gc2053_data_byte == 2)
    {
        buf[idx] = (data >> 8) & 0xff;
        idx++;
        buf[idx] = data & 0xff;
        idx++;
    }
    else
    {
        buf[idx] = data & 0xff;
        idx++;
    }

    ret = write(g_fd[ViPipe], buf, gc2053_addr_byte + gc2053_data_byte);
    if(ret < 0)
    {
        printf("I2C_WRITE error!\n");
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

static void delay_ms(int ms)
{
    usleep(ms * 1000);
}

void gc2053_prog(VI_PIPE ViPipe, int *rom)
{
    int i = 0;

    while(1)
    {
        int lookup = rom[i++];
        int addr = (lookup >> 16) & 0xFFFF;
        int data = lookup & 0xFFFF;
        if(addr == 0xFFFE)
        {
            delay_ms(data);
        }
        else if(addr == 0xFFFF)
        {
            return;
        }
        else
        {
            gc2053_write_register(ViPipe, addr, data);
        }
    }
}

void gc2053_standby(VI_PIPE ViPipe)
{
    gc2053_write_register(ViPipe, 0xf2, 0x01);
    gc2053_write_register(ViPipe, 0xfc, 0x8f);

    g_bStandby[ViPipe] = SC_TRUE;
    return;
}

void gc2053_restart(VI_PIPE ViPipe)
{
    gc2053_write_register(ViPipe, 0xf2, 0x00);
    gc2053_write_register(ViPipe, 0xfc, 0x8e);

    g_bStandby[ViPipe] = SC_FALSE;
    return;
}

int gc2053_flip_off_mirror_off(VI_PIPE ViPipe)
{
    gc2053_write_register(ViPipe, GC2053_FLIP_MIRROR, 0x80);
    return SC_SUCCESS;
}

int gc2053_flip_on_mirror_off(VI_PIPE ViPipe)
{
    gc2053_write_register(ViPipe, GC2053_FLIP_MIRROR, 0x82);
    return SC_SUCCESS;
}

int gc2053_flip_off_mirror_on(VI_PIPE ViPipe)
{
    gc2053_write_register(ViPipe, GC2053_FLIP_MIRROR, 0x81);
    return SC_SUCCESS;
}

int gc2053_flip_on_mirror_on(VI_PIPE ViPipe)
{
    gc2053_write_register(ViPipe, GC2053_FLIP_MIRROR, 0x83);
    return SC_SUCCESS;
}

void gc2053_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i = 0;

    for(i = 0; i < g_pastGc2053[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastGc2053[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            gc2053_write_register(ViPipe,
                g_pastGc2053[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                g_pastGc2053[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int gc2053_init(VI_PIPE ViPipe)
{
    SC_BOOL bInit     = SC_FALSE;
    SC_U8   u8ImgMode = 0;
    SC_S32  ret       = SC_FAILURE;

    bInit     = g_pastGc2053[ViPipe]->bInit;
    u8ImgMode = g_pastGc2053[ViPipe]->u8ImgMode;
    printf("GC2053: bInit[%d] u8ImgMode[%u]\n", bInit, (SC_U32)u8ImgMode);

    if(SC_FALSE == bInit)
    {
        ret = gc2053_i2c_init(ViPipe);
        if(SC_SUCCESS != ret)
        {
            printf("Init gc2053 i2c error!\n");
            return SC_FAILURE;
        }
    }

    switch(u8ImgMode)
    {
        case SENSOR_1080P_30FPS_LINEAR_MODE:
        {
            ret = gc2053_linear_1080p30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init gc2053 linear mode error!\n");
                return SC_FAILURE;
            }

            g_pastGc2053[ViPipe]->bInit = SC_TRUE;
            break;
        }

        default:
        {
            printf("Not support this mode!\n");
            g_pastGc2053[ViPipe]->bInit = SC_FALSE;
            break;
        }
    }

    return SC_SUCCESS;
}

void gc2053_exit(VI_PIPE ViPipe)
{
    gc2053_i2c_exit(ViPipe);
    g_bStandby[ViPipe] = SC_FALSE;

    return;
}

/* 1080P@30fps LINEAR; Raw:10Bit; MCLK Input: 27MHz; MIPI CLK: 594Mbps/lane; CSI-2 2Lane */
int gc2053_linear_1080p30_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = gc2053_write_register(ViPipe, 0xfe, 0x80);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write gc2053 register error!\n");
        return SC_FAILURE;
    }

    gc2053_write_register(ViPipe, 0xfe, 0x80);
    gc2053_write_register(ViPipe, 0xfe, 0x80);
    gc2053_write_register(ViPipe, 0xfe, 0x00);
    gc2053_write_register(ViPipe, 0xf2, 0x00);
    gc2053_write_register(ViPipe, 0xf3, 0x00);
    gc2053_write_register(ViPipe, 0xf4, 0x36);
    gc2053_write_register(ViPipe, 0xf5, 0xc0);
    gc2053_write_register(ViPipe, 0xf6, 0x44);
    gc2053_write_register(ViPipe, 0xf7, 0x01);
    gc2053_write_register(ViPipe, 0xf8, 0x2c);
    gc2053_write_register(ViPipe, 0xf9, 0x42);
    gc2053_write_register(ViPipe, 0xfc, 0x8e);

    gc2053_write_register(ViPipe, 0xfe, 0x00);
    gc2053_write_register(ViPipe, 0x87, 0x18);
    gc2053_write_register(ViPipe, 0xee, 0x30);
    gc2053_write_register(ViPipe, 0xd0, 0xb7);
    gc2053_write_register(ViPipe, 0x03, 0x04);
    gc2053_write_register(ViPipe, 0x04, 0x60);
    gc2053_write_register(ViPipe, 0x05, 0x04);
    gc2053_write_register(ViPipe, 0x06, 0x4c);
    gc2053_write_register(ViPipe, 0x07, 0x00);
    gc2053_write_register(ViPipe, 0x08, 0x11);
    gc2053_write_register(ViPipe, 0x0a, 0x02);
    gc2053_write_register(ViPipe, 0x0c, 0x02);
    gc2053_write_register(ViPipe, 0x0d, 0x04);
    gc2053_write_register(ViPipe, 0x0e, 0x40);
    gc2053_write_register(ViPipe, 0x12, 0xe2);
    gc2053_write_register(ViPipe, 0x13, 0x16);
    gc2053_write_register(ViPipe, 0x19, 0x0a);
    gc2053_write_register(ViPipe, 0x28, 0x0a);
    gc2053_write_register(ViPipe, 0x2b, 0x04);
    gc2053_write_register(ViPipe, 0x37, 0x03);
    gc2053_write_register(ViPipe, 0x43, 0x07);
    gc2053_write_register(ViPipe, 0x44, 0x40);
    gc2053_write_register(ViPipe, 0x46, 0x0b);
    gc2053_write_register(ViPipe, 0x4b, 0x20);
    gc2053_write_register(ViPipe, 0x4e, 0x08);
    gc2053_write_register(ViPipe, 0x55, 0x20);
    gc2053_write_register(ViPipe, 0x77, 0x01);
    gc2053_write_register(ViPipe, 0x78, 0x00);
    gc2053_write_register(ViPipe, 0x7c, 0x93);
    gc2053_write_register(ViPipe, 0x8d, 0x92);
    gc2053_write_register(ViPipe, 0x90, 0x00);
    gc2053_write_register(ViPipe, 0x41, 0x04);
    gc2053_write_register(ViPipe, 0x42, 0x65);
    gc2053_write_register(ViPipe, 0xce, 0x7c);
    gc2053_write_register(ViPipe, 0xd2, 0x41);
    gc2053_write_register(ViPipe, 0xd3, 0xdc);
    gc2053_write_register(ViPipe, 0xe6, 0x50);
    gc2053_write_register(ViPipe, 0xb6, 0xc0);
    gc2053_write_register(ViPipe, 0xb0, 0x70);
    gc2053_write_register(ViPipe, 0x26, 0x30);
    gc2053_write_register(ViPipe, 0xfe, 0x01);
    gc2053_write_register(ViPipe, 0x55, 0x07);

    /* dither */
    gc2053_write_register(ViPipe, 0x58, 0x00); /* default 0x80 */
    gc2053_write_register(ViPipe, 0xfe, 0x04);
    gc2053_write_register(ViPipe, 0x14, 0x78);
    gc2053_write_register(ViPipe, 0x15, 0x78);
    gc2053_write_register(ViPipe, 0x16, 0x78);
    gc2053_write_register(ViPipe, 0x17, 0x78);
    gc2053_write_register(ViPipe, 0xfe, 0x01);
    gc2053_write_register(ViPipe, 0x04, 0x00);
    gc2053_write_register(ViPipe, 0x94, 0x03);
    gc2053_write_register(ViPipe, 0x97, 0x07);
    gc2053_write_register(ViPipe, 0x98, 0x80);
    gc2053_write_register(ViPipe, 0x9a, 0x06);
    gc2053_write_register(ViPipe, 0xfe, 0x00);
    gc2053_write_register(ViPipe, 0x7b, 0x2a);
    gc2053_write_register(ViPipe, 0x23, 0x2d);
    gc2053_write_register(ViPipe, 0xfe, 0x03);
    gc2053_write_register(ViPipe, 0x01, 0x27);
    gc2053_write_register(ViPipe, 0x02, 0x56);
    gc2053_write_register(ViPipe, 0x03, 0xb6); /* 0xce 0x8e */
    gc2053_write_register(ViPipe, 0x12, 0x80);
    gc2053_write_register(ViPipe, 0x13, 0x07);
    gc2053_write_register(ViPipe, 0x15, 0x12); /* P3:DHPY mode(MIPI CLK MODE) 0x12 0x10 */

    gc2053_write_register(ViPipe, 0xfe, 0x00);
    gc2053_write_register(ViPipe, 0x3e, 0x91);

    printf("===============================================================\n");
    printf("== Galaxycore gc2053 sensor 1080P30fps linear init success!  ==\n");
    printf("===============================================================\n");
    return SC_SUCCESS;
}
