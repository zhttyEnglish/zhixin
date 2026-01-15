/**
 * @file     gc1603_sensor_ctl.c
 * @brief    SENSOR初始化接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2022-10-12 创建文件
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


#define GC1603_FLIP_MIRROR  (0x0101)
#define GC1603_720P_FPS_60  (60)
#define GC1603_720P_FPS_120 (120)

#define SENSOR_720P_120FPS_LINEAR_MODE (0)

const unsigned char gc1603_i2c_addr  = 0x20; /* I2C Address of GC1603: 0x52 or 0x20 */
const unsigned int  gc1603_addr_byte = 2;
const unsigned int  gc1603_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};
static SC_BOOL g_bStandby[ISP_MAX_PIPE_NUM] = {0};

extern ISP_SNS_STATE_S   *g_pastGc1603[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U  g_aunGc1603BusInfo[];
extern const unsigned int g_gc1603_fps_mode;


int gc1603_linear_720p120_init(VI_PIPE ViPipe);
int gc1603_linear_720p60_init(VI_PIPE ViPipe);


int gc1603_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {
        return SC_SUCCESS;
    }

    u8DevNum = g_aunGc1603BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (gc1603_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int gc1603_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int gc1603_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:
    return SC_SUCCESS;
}

int gc1603_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(gc1603_addr_byte == 2)
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

    if(gc1603_data_byte == 2)
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

    ret = write(g_fd[ViPipe], buf, gc1603_addr_byte + gc1603_data_byte);
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

void gc1603_prog(VI_PIPE ViPipe, int *rom)
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
            gc1603_write_register(ViPipe, addr, data);
        }
    }
}

void gc1603_standby(VI_PIPE ViPipe)
{
    g_bStandby[ViPipe] = SC_TRUE;
    return;
}

void gc1603_restart(VI_PIPE ViPipe)
{
    g_bStandby[ViPipe] = SC_FALSE;
    return;
}

int gc1603_flip_off_mirror_off(VI_PIPE ViPipe)
{
    gc1603_write_register(ViPipe, GC1603_FLIP_MIRROR, 0x00);
    return SC_SUCCESS;
}

int gc1603_flip_on_mirror_off(VI_PIPE ViPipe)
{
    gc1603_write_register(ViPipe, GC1603_FLIP_MIRROR, 0x02);
    return SC_SUCCESS;
}

int gc1603_flip_off_mirror_on(VI_PIPE ViPipe)
{
    gc1603_write_register(ViPipe, GC1603_FLIP_MIRROR, 0x01);
    return SC_SUCCESS;
}

int gc1603_flip_on_mirror_on(VI_PIPE ViPipe)
{
    gc1603_write_register(ViPipe, GC1603_FLIP_MIRROR, 0x03);
    return SC_SUCCESS;
}

void gc1603_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i = 0;

    for(i = 0; i < g_pastGc1603[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastGc1603[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            gc1603_write_register(ViPipe,
                g_pastGc1603[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                g_pastGc1603[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int gc1603_init(VI_PIPE ViPipe)
{
    SC_BOOL bInit     = SC_FALSE;
    SC_U8   u8ImgMode = 0;
    SC_S32  ret       = SC_FAILURE;

    bInit     = g_pastGc1603[ViPipe]->bInit;
    u8ImgMode = g_pastGc1603[ViPipe]->u8ImgMode;
    printf("GC1603: bInit[%d] u8ImgMode[%u]\n", bInit, (SC_U32)u8ImgMode);

    if(SC_FALSE == bInit)
    {
        ret = gc1603_i2c_init(ViPipe);
        if(SC_SUCCESS != ret)
        {
            printf("Init gc1603 i2c error!\n");
            return SC_FAILURE;
        }
    }

    switch(u8ImgMode)
    {
        case SENSOR_720P_120FPS_LINEAR_MODE:
        {
            if(GC1603_720P_FPS_60 == g_gc1603_fps_mode)
            {
                ret = gc1603_linear_720p60_init(ViPipe);
            }
            else
            {
                ret = gc1603_linear_720p120_init(ViPipe);
            }

            if(SC_SUCCESS != ret)
            {
                printf("Init gc1603 linear mode error!\n");
                return SC_FAILURE;
            }

            g_pastGc1603[ViPipe]->bInit = SC_TRUE;
            break;
        }

        default:
        {
            printf("Not support this mode!\n");
            g_pastGc1603[ViPipe]->bInit = SC_FALSE;
            break;
        }
    }

    return SC_SUCCESS;
}

void gc1603_exit(VI_PIPE ViPipe)
{
    gc1603_i2c_exit(ViPipe);
    g_bStandby[ViPipe] = SC_FALSE;

    return;
}

/* 720P@60fps LINEAR; Raw:10Bit; MCLK Input: 27MHz; MIPI CLK: 504Mbps/lane; CSI-2 2Lane */
int gc1603_linear_720p60_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = gc1603_write_register(ViPipe, 0x031c, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write gc1603 register error!\n");
        return SC_FAILURE;
    }

    gc1603_write_register(ViPipe, 0x0317, 0x24);
    gc1603_write_register(ViPipe, 0x0320, 0x77);
    gc1603_write_register(ViPipe, 0x0106, 0x78);
    gc1603_write_register(ViPipe, 0x0324, 0x04);
    gc1603_write_register(ViPipe, 0x0327, 0x03);
    gc1603_write_register(ViPipe, 0x0325, 0x00);
    gc1603_write_register(ViPipe, 0x0326, 0x20);
    gc1603_write_register(ViPipe, 0x031a, 0x00);
    gc1603_write_register(ViPipe, 0x0314, 0x30);
    gc1603_write_register(ViPipe, 0x0315, 0x32);
    gc1603_write_register(ViPipe, 0x0334, 0x40);
    gc1603_write_register(ViPipe, 0x0337, 0x03);
    gc1603_write_register(ViPipe, 0x0335, 0x05);
    gc1603_write_register(ViPipe, 0x0335, 0x05);
    gc1603_write_register(ViPipe, 0x0336, 0x38);
    gc1603_write_register(ViPipe, 0x0324, 0x44);
    gc1603_write_register(ViPipe, 0x0334, 0x40);
    gc1603_write_register(ViPipe, 0x031c, 0x03);
    gc1603_write_register(ViPipe, 0x031c, 0xd2);
    gc1603_write_register(ViPipe, 0x0180, 0x26);
    gc1603_write_register(ViPipe, 0x031c, 0xd6);
    gc1603_write_register(ViPipe, 0x0287, 0x18);
    gc1603_write_register(ViPipe, 0x02ee, 0x70);
    gc1603_write_register(ViPipe, 0x0202, 0x02);
    gc1603_write_register(ViPipe, 0x0203, 0xa6);
    gc1603_write_register(ViPipe, 0x0213, 0x1c);
    gc1603_write_register(ViPipe, 0x0214, 0x04);
    gc1603_write_register(ViPipe, 0x0290, 0x00);
    gc1603_write_register(ViPipe, 0x029d, 0x08);
    gc1603_write_register(ViPipe, 0x0340, 0x02);
    gc1603_write_register(ViPipe, 0x0341, 0xee);
    gc1603_write_register(ViPipe, 0x0342, 0x03);  /* hts = reg_val * 2; 0x0640=1600 30fps; 0x320=800 60fps */
    gc1603_write_register(ViPipe, 0x0343, 0x20);
    gc1603_write_register(ViPipe, 0x023c, 0x06);
    gc1603_write_register(ViPipe, 0x02d1, 0xe2);
    gc1603_write_register(ViPipe, 0x027d, 0xcc);
    gc1603_write_register(ViPipe, 0x0238, 0xa4);
    gc1603_write_register(ViPipe, 0x02ce, 0x1f);
    gc1603_write_register(ViPipe, 0x02f9, 0x00);
    gc1603_write_register(ViPipe, 0x0227, 0x74);
    gc1603_write_register(ViPipe, 0x0232, 0xc8);
    gc1603_write_register(ViPipe, 0x0245, 0xa8);
    gc1603_write_register(ViPipe, 0x027d, 0xcc);
    gc1603_write_register(ViPipe, 0x02fa, 0xb0);
    gc1603_write_register(ViPipe, 0x02e7, 0x23);
    gc1603_write_register(ViPipe, 0x02e8, 0x50);
    gc1603_write_register(ViPipe, 0x021d, 0x13);
    gc1603_write_register(ViPipe, 0x0220, 0x43);
    gc1603_write_register(ViPipe, 0x0228, 0x10);
    gc1603_write_register(ViPipe, 0x022c, 0x2c);
    gc1603_write_register(ViPipe, 0x02c0, 0x11);
    gc1603_write_register(ViPipe, 0x024b, 0x11);
    gc1603_write_register(ViPipe, 0x024e, 0x11);
    gc1603_write_register(ViPipe, 0x024d, 0x11);
    gc1603_write_register(ViPipe, 0x0255, 0x11);
    gc1603_write_register(ViPipe, 0x025b, 0x11);
    gc1603_write_register(ViPipe, 0x0262, 0x01);
    gc1603_write_register(ViPipe, 0x02d4, 0x10);
    gc1603_write_register(ViPipe, 0x0540, 0x10);
    gc1603_write_register(ViPipe, 0x0239, 0x00);
    gc1603_write_register(ViPipe, 0x0231, 0xc4);
    gc1603_write_register(ViPipe, 0x024f, 0x11);
    gc1603_write_register(ViPipe, 0x028c, 0x1a);
    gc1603_write_register(ViPipe, 0x02d3, 0x01);
    gc1603_write_register(ViPipe, 0x02da, 0x35);
    gc1603_write_register(ViPipe, 0x02db, 0xd0);
    gc1603_write_register(ViPipe, 0x02e6, 0x30);
    gc1603_write_register(ViPipe, 0x0512, 0x00);
    gc1603_write_register(ViPipe, 0x0513, 0x00);
    gc1603_write_register(ViPipe, 0x0515, 0x20);
    gc1603_write_register(ViPipe, 0x0518, 0x00);
    gc1603_write_register(ViPipe, 0x0519, 0x00);
    gc1603_write_register(ViPipe, 0x051d, 0x50);
    gc1603_write_register(ViPipe, 0x0211, 0x00);
    gc1603_write_register(ViPipe, 0x0216, 0x00);
    gc1603_write_register(ViPipe, 0x0221, 0x20);
    gc1603_write_register(ViPipe, 0x0223, 0xcc);
    gc1603_write_register(ViPipe, 0x0225, 0x07);
    gc1603_write_register(ViPipe, 0x0229, 0x36);
    gc1603_write_register(ViPipe, 0x022b, 0x0c);
    gc1603_write_register(ViPipe, 0x022e, 0x0c);
    gc1603_write_register(ViPipe, 0x0230, 0x03);
    gc1603_write_register(ViPipe, 0x023a, 0x38);
    gc1603_write_register(ViPipe, 0x027b, 0x3c);
    gc1603_write_register(ViPipe, 0x027c, 0x0c);
    gc1603_write_register(ViPipe, 0x0298, 0x13);
    gc1603_write_register(ViPipe, 0x02a4, 0x07);
    gc1603_write_register(ViPipe, 0x02ab, 0x00);
    gc1603_write_register(ViPipe, 0x02ac, 0x00);
    gc1603_write_register(ViPipe, 0x02ad, 0x07);
    gc1603_write_register(ViPipe, 0x02af, 0x01);
    gc1603_write_register(ViPipe, 0x02cd, 0x3c);
    gc1603_write_register(ViPipe, 0x02d2, 0xe8);
    gc1603_write_register(ViPipe, 0x02e4, 0x00);
    gc1603_write_register(ViPipe, 0x0530, 0x04);
    gc1603_write_register(ViPipe, 0x0531, 0x04);
    gc1603_write_register(ViPipe, 0x0243, 0x36);
    gc1603_write_register(ViPipe, 0x0219, 0x07);
    gc1603_write_register(ViPipe, 0x02e5, 0x28);
    gc1603_write_register(ViPipe, 0x0338, 0xaa);
    gc1603_write_register(ViPipe, 0x0339, 0xaa);
    gc1603_write_register(ViPipe, 0x033a, 0x02);
    gc1603_write_register(ViPipe, 0x023b, 0x20);
    gc1603_write_register(ViPipe, 0x0212, 0x48);
    gc1603_write_register(ViPipe, 0x0523, 0x02);
    gc1603_write_register(ViPipe, 0x0347, 0x06);
    gc1603_write_register(ViPipe, 0x0348, 0x0a);
    gc1603_write_register(ViPipe, 0x0349, 0x10);
    gc1603_write_register(ViPipe, 0x034a, 0x05);
    gc1603_write_register(ViPipe, 0x034b, 0xb0);
    gc1603_write_register(ViPipe, 0x034c, 0x05);
    gc1603_write_register(ViPipe, 0x034d, 0x00);
    gc1603_write_register(ViPipe, 0x034e, 0x02);
    gc1603_write_register(ViPipe, 0x034f, 0xd0);
    gc1603_write_register(ViPipe, 0x0354, 0x01);
    gc1603_write_register(ViPipe, 0x0295, 0xff);
    gc1603_write_register(ViPipe, 0x0296, 0xff);
    gc1603_write_register(ViPipe, 0x02f0, 0x22);
    gc1603_write_register(ViPipe, 0x02f1, 0x22);
    gc1603_write_register(ViPipe, 0x02f2, 0xff);
    gc1603_write_register(ViPipe, 0x02f4, 0x32);
    gc1603_write_register(ViPipe, 0x02f5, 0x20);
    gc1603_write_register(ViPipe, 0x02f6, 0x1c);
    gc1603_write_register(ViPipe, 0x02f7, 0x1f);
    gc1603_write_register(ViPipe, 0x02f8, 0x00);
    gc1603_write_register(ViPipe, 0x0291, 0x04);
    gc1603_write_register(ViPipe, 0x0292, 0x22);
    gc1603_write_register(ViPipe, 0x0297, 0x22);
    gc1603_write_register(ViPipe, 0x02d5, 0xfe);
    gc1603_write_register(ViPipe, 0x02d6, 0xd0);
    gc1603_write_register(ViPipe, 0x02d7, 0x35);
    gc1603_write_register(ViPipe, 0x021f, 0x10);
    gc1603_write_register(ViPipe, 0x0233, 0x01);
    gc1603_write_register(ViPipe, 0x0234, 0x03);
    gc1603_write_register(ViPipe, 0x0224, 0x01);
    gc1603_write_register(ViPipe, 0x031c, 0x80);
    gc1603_write_register(ViPipe, 0x031f, 0x10);
    gc1603_write_register(ViPipe, 0x031f, 0x00);
    gc1603_write_register(ViPipe, 0x031c, 0xd2);
    gc1603_write_register(ViPipe, 0x031c, 0xd2);
    gc1603_write_register(ViPipe, 0x031c, 0xd2);
    gc1603_write_register(ViPipe, 0x031c, 0xd2);
    gc1603_write_register(ViPipe, 0x031c, 0x80);
    gc1603_write_register(ViPipe, 0x031f, 0x10);
    gc1603_write_register(ViPipe, 0x031f, 0x00);
    gc1603_write_register(ViPipe, 0x031c, 0xd6);
    gc1603_write_register(ViPipe, 0x0053, 0x00);
    gc1603_write_register(ViPipe, 0x008e, 0x55);
    gc1603_write_register(ViPipe, 0x0205, 0xc0);
    gc1603_write_register(ViPipe, 0x02b0, 0xf2);
    gc1603_write_register(ViPipe, 0x02b1, 0xf2);
    gc1603_write_register(ViPipe, 0x02b3, 0x00);
    gc1603_write_register(ViPipe, 0x02b4, 0x00);
    gc1603_write_register(ViPipe, 0x0451, 0x00);
    gc1603_write_register(ViPipe, 0x0455, 0x04);
    gc1603_write_register(ViPipe, 0x0452, 0x00);
    gc1603_write_register(ViPipe, 0x0456, 0x04);
    gc1603_write_register(ViPipe, 0x0450, 0x00);
    gc1603_write_register(ViPipe, 0x0454, 0x04);
    gc1603_write_register(ViPipe, 0x0453, 0x00);
    gc1603_write_register(ViPipe, 0x0457, 0x04);
    gc1603_write_register(ViPipe, 0x0226, 0x30);
    gc1603_write_register(ViPipe, 0x0042, 0x20);
    gc1603_write_register(ViPipe, 0x0458, 0x01);
    gc1603_write_register(ViPipe, 0x0459, 0x01);
    gc1603_write_register(ViPipe, 0x045a, 0x01);
    gc1603_write_register(ViPipe, 0x045b, 0x01);
    gc1603_write_register(ViPipe, 0x044c, 0x80);
    gc1603_write_register(ViPipe, 0x044d, 0x80);
    gc1603_write_register(ViPipe, 0x044e, 0x80);
    gc1603_write_register(ViPipe, 0x044f, 0x80);
    gc1603_write_register(ViPipe, 0x0060, 0x40);
    gc1603_write_register(ViPipe, 0x00a0, 0x15);
    gc1603_write_register(ViPipe, 0x00c7, 0x90);
    gc1603_write_register(ViPipe, 0x00c8, 0x15);
    gc1603_write_register(ViPipe, 0x00e1, 0x81);
    gc1603_write_register(ViPipe, 0x00e2, 0x1c);
    gc1603_write_register(ViPipe, 0x00e4, 0x01);
    gc1603_write_register(ViPipe, 0x00e5, 0x01);
    gc1603_write_register(ViPipe, 0x00e6, 0x01);
    gc1603_write_register(ViPipe, 0x00e7, 0x00);
    gc1603_write_register(ViPipe, 0x00e8, 0x00);
    gc1603_write_register(ViPipe, 0x00e9, 0x00);
    gc1603_write_register(ViPipe, 0x00ea, 0xf0);
    gc1603_write_register(ViPipe, 0x00ef, 0x04);
    gc1603_write_register(ViPipe, 0x0089, 0x03);
    gc1603_write_register(ViPipe, 0x008c, 0x10);
    gc1603_write_register(ViPipe, 0x0080, 0x04);
    gc1603_write_register(ViPipe, 0x0180, 0x66);
    gc1603_write_register(ViPipe, 0x0181, 0x30);
    gc1603_write_register(ViPipe, 0x0182, 0x55);
    gc1603_write_register(ViPipe, 0x0185, 0x01);
    gc1603_write_register(ViPipe, 0x0114, 0x01);
    gc1603_write_register(ViPipe, 0x0115, 0x12);
    gc1603_write_register(ViPipe, 0x0103, 0x00);
    gc1603_write_register(ViPipe, 0x0104, 0x20);
    gc1603_write_register(ViPipe, 0x0100, 0x09);

    printf("===============================================================\n");
    printf("== Galaxycore gc1603 sensor 720P60fps linear init success!   ==\n");
    printf("===============================================================\n");
    return SC_SUCCESS;
}

/* 720P@120fps LINEAR; Raw:10Bit; MCLK Input: 27MHz; MIPI CLK: 630Mbps/lane; CSI-2 2Lane */
int gc1603_linear_720p120_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = gc1603_write_register(ViPipe, 0x031c, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write gc1603 register error!\n");
        return SC_FAILURE;
    }

    gc1603_write_register(ViPipe, 0x0317, 0x24);
    gc1603_write_register(ViPipe, 0x0320, 0x77);
    gc1603_write_register(ViPipe, 0x0106, 0x78);
    gc1603_write_register(ViPipe, 0x0324, 0x84);
    gc1603_write_register(ViPipe, 0x0327, 0x05);
    gc1603_write_register(ViPipe, 0x0325, 0x0c);
    gc1603_write_register(ViPipe, 0x0326, 0x22);
    gc1603_write_register(ViPipe, 0x031a, 0x00);
    gc1603_write_register(ViPipe, 0x0314, 0x30);
    gc1603_write_register(ViPipe, 0x0315, 0x23);
    gc1603_write_register(ViPipe, 0x0334, 0x00);
    gc1603_write_register(ViPipe, 0x0337, 0x06);
    gc1603_write_register(ViPipe, 0x0335, 0x03);
    gc1603_write_register(ViPipe, 0x0336, 0x46);
    gc1603_write_register(ViPipe, 0x0324, 0xc4);
    gc1603_write_register(ViPipe, 0x0334, 0x40);
    gc1603_write_register(ViPipe, 0x031c, 0x03);
    gc1603_write_register(ViPipe, 0x031c, 0xd2);
    gc1603_write_register(ViPipe, 0x0180, 0x26);
    gc1603_write_register(ViPipe, 0x031c, 0xd6);
    gc1603_write_register(ViPipe, 0x0287, 0x18);
    gc1603_write_register(ViPipe, 0x02ee, 0x70);
    gc1603_write_register(ViPipe, 0x0202, 0x02);
    gc1603_write_register(ViPipe, 0x0203, 0xa6);
    gc1603_write_register(ViPipe, 0x0213, 0x1c);
    gc1603_write_register(ViPipe, 0x0214, 0x04);
    gc1603_write_register(ViPipe, 0x0290, 0x00);
    gc1603_write_register(ViPipe, 0x029d, 0x08);
    gc1603_write_register(ViPipe, 0x0340, 0x02);
    gc1603_write_register(ViPipe, 0x0341, 0xee);
    gc1603_write_register(ViPipe, 0x0342, 0x01);
    gc1603_write_register(ViPipe, 0x0343, 0xfe);
    gc1603_write_register(ViPipe, 0x02d1, 0xc2);
    gc1603_write_register(ViPipe, 0x027d, 0xcc);
    gc1603_write_register(ViPipe, 0x0238, 0xa4);
    gc1603_write_register(ViPipe, 0x02ce, 0x1f);
    gc1603_write_register(ViPipe, 0x02f9, 0x00);
    gc1603_write_register(ViPipe, 0x0227, 0x74);
    gc1603_write_register(ViPipe, 0x0232, 0xc8);
    gc1603_write_register(ViPipe, 0x0245, 0xa8);
    gc1603_write_register(ViPipe, 0x027d, 0xcc);
    gc1603_write_register(ViPipe, 0x02fa, 0xb0);
    gc1603_write_register(ViPipe, 0x02e7, 0x23);
    gc1603_write_register(ViPipe, 0x02e8, 0x41);
    gc1603_write_register(ViPipe, 0x021d, 0x13);
    gc1603_write_register(ViPipe, 0x0220, 0x03);
    gc1603_write_register(ViPipe, 0x024b, 0x11);
    gc1603_write_register(ViPipe, 0x0255, 0x11);
    gc1603_write_register(ViPipe, 0x025b, 0x11);
    gc1603_write_register(ViPipe, 0x0262, 0x01);
    gc1603_write_register(ViPipe, 0x02d4, 0x10);
    gc1603_write_register(ViPipe, 0x0231, 0xc4);
    gc1603_write_register(ViPipe, 0x024f, 0x11);
    gc1603_write_register(ViPipe, 0x028c, 0x1a);
    gc1603_write_register(ViPipe, 0x02d3, 0x07);
    gc1603_write_register(ViPipe, 0x02da, 0x35);
    gc1603_write_register(ViPipe, 0x02db, 0xd0);
    gc1603_write_register(ViPipe, 0x02e6, 0x30);
    gc1603_write_register(ViPipe, 0x0515, 0x08);
    gc1603_write_register(ViPipe, 0x0518, 0x00);
    gc1603_write_register(ViPipe, 0x0519, 0x00);
    gc1603_write_register(ViPipe, 0x051d, 0x50);
    gc1603_write_register(ViPipe, 0x0211, 0x00);
    gc1603_write_register(ViPipe, 0x0216, 0x00);
    gc1603_write_register(ViPipe, 0x0221, 0x29);
    gc1603_write_register(ViPipe, 0x0223, 0x3b);
    gc1603_write_register(ViPipe, 0x0224, 0x01);
    gc1603_write_register(ViPipe, 0x0225, 0x07);
    gc1603_write_register(ViPipe, 0x0229, 0x29);
    gc1603_write_register(ViPipe, 0x022e, 0x33);
    gc1603_write_register(ViPipe, 0x027b, 0x33);
    gc1603_write_register(ViPipe, 0x0298, 0x13);
    gc1603_write_register(ViPipe, 0x02a4, 0x07);
    gc1603_write_register(ViPipe, 0x02ad, 0x07);
    gc1603_write_register(ViPipe, 0x02cd, 0x33);
    gc1603_write_register(ViPipe, 0x02e4, 0x04);
    gc1603_write_register(ViPipe, 0x0219, 0x07);
    gc1603_write_register(ViPipe, 0x02e5, 0x28);
    gc1603_write_register(ViPipe, 0x0338, 0xaa);
    gc1603_write_register(ViPipe, 0x0339, 0xaa);
    gc1603_write_register(ViPipe, 0x033a, 0x02);
    gc1603_write_register(ViPipe, 0x023b, 0x20);
    gc1603_write_register(ViPipe, 0x0212, 0x48);
    gc1603_write_register(ViPipe, 0x0523, 0x02);
    gc1603_write_register(ViPipe, 0x0347, 0x06);
    gc1603_write_register(ViPipe, 0x0348, 0x0a);
    gc1603_write_register(ViPipe, 0x0349, 0x10);
    gc1603_write_register(ViPipe, 0x034a, 0x05);
    gc1603_write_register(ViPipe, 0x034b, 0xb0);
    gc1603_write_register(ViPipe, 0x034c, 0x05);
    gc1603_write_register(ViPipe, 0x034d, 0x00);
    gc1603_write_register(ViPipe, 0x034e, 0x02);
    gc1603_write_register(ViPipe, 0x034f, 0xd0);
    gc1603_write_register(ViPipe, 0x0354, 0x01);
    gc1603_write_register(ViPipe, 0x024e, 0x00);
    gc1603_write_register(ViPipe, 0x024d, 0x00);
    gc1603_write_register(ViPipe, 0x02f0, 0x22);
    gc1603_write_register(ViPipe, 0x02f1, 0x00);
    gc1603_write_register(ViPipe, 0x02f2, 0x00);
    gc1603_write_register(ViPipe, 0x0295, 0xff);
    gc1603_write_register(ViPipe, 0x0296, 0xff);
    gc1603_write_register(ViPipe, 0x02d6, 0xa8);
    gc1603_write_register(ViPipe, 0x0292, 0x28);
    gc1603_write_register(ViPipe, 0x0297, 0x28);
    gc1603_write_register(ViPipe, 0x021f, 0x50);
    gc1603_write_register(ViPipe, 0x0233, 0x01);
    gc1603_write_register(ViPipe, 0x0234, 0x03);
    gc1603_write_register(ViPipe, 0x031c, 0x80);
    gc1603_write_register(ViPipe, 0x031f, 0x10);
    gc1603_write_register(ViPipe, 0x031f, 0x00);
    gc1603_write_register(ViPipe, 0x031c, 0xd6);
    gc1603_write_register(ViPipe, 0x031c, 0xd6);
    gc1603_write_register(ViPipe, 0x031c, 0xd6);
    gc1603_write_register(ViPipe, 0x031c, 0xd6);
    gc1603_write_register(ViPipe, 0x031c, 0x80);
    gc1603_write_register(ViPipe, 0x031f, 0x10);
    gc1603_write_register(ViPipe, 0x031f, 0x00);
    gc1603_write_register(ViPipe, 0x031c, 0xd6);
    gc1603_write_register(ViPipe, 0x0053, 0x00);
    gc1603_write_register(ViPipe, 0x0205, 0xc0);
    gc1603_write_register(ViPipe, 0x02b0, 0xd8);
    gc1603_write_register(ViPipe, 0x02b1, 0xd8);
    gc1603_write_register(ViPipe, 0x0226, 0x30);
    gc1603_write_register(ViPipe, 0x0042, 0x20);
    gc1603_write_register(ViPipe, 0x0049, 0x3c);
    gc1603_write_register(ViPipe, 0x004a, 0x3c);
    gc1603_write_register(ViPipe, 0x0458, 0x00);
    gc1603_write_register(ViPipe, 0x0459, 0x00);
    gc1603_write_register(ViPipe, 0x045a, 0x00);
    gc1603_write_register(ViPipe, 0x045b, 0x00);
    gc1603_write_register(ViPipe, 0x044c, 0x7f);
    gc1603_write_register(ViPipe, 0x044d, 0x7f);
    gc1603_write_register(ViPipe, 0x044e, 0x7f);
    gc1603_write_register(ViPipe, 0x044f, 0x7f);
    gc1603_write_register(ViPipe, 0x0060, 0x40);
    gc1603_write_register(ViPipe, 0x0080, 0x06);
    gc1603_write_register(ViPipe, 0x00a0, 0x15);
    gc1603_write_register(ViPipe, 0x00c7, 0x90);
    gc1603_write_register(ViPipe, 0x00c8, 0x15);
    gc1603_write_register(ViPipe, 0x00e1, 0x81);
    gc1603_write_register(ViPipe, 0x00e2, 0x1c);
    gc1603_write_register(ViPipe, 0x00e4, 0x01);
    gc1603_write_register(ViPipe, 0x00e5, 0x01);
    gc1603_write_register(ViPipe, 0x00e6, 0x01);
    gc1603_write_register(ViPipe, 0x00e7, 0x00);
    gc1603_write_register(ViPipe, 0x00e8, 0x00);
    gc1603_write_register(ViPipe, 0x00e9, 0x00);
    gc1603_write_register(ViPipe, 0x00ea, 0xf0);
    gc1603_write_register(ViPipe, 0x00ef, 0x04);
    gc1603_write_register(ViPipe, 0x0089, 0x03);
    gc1603_write_register(ViPipe, 0x0180, 0x66);
    gc1603_write_register(ViPipe, 0x0181, 0x30);
    gc1603_write_register(ViPipe, 0x0182, 0x55);
    gc1603_write_register(ViPipe, 0x0185, 0x01);
    gc1603_write_register(ViPipe, 0x0114, 0x01);
    gc1603_write_register(ViPipe, 0x0115, 0x12);
    gc1603_write_register(ViPipe, 0x0103, 0x00);
    gc1603_write_register(ViPipe, 0x0104, 0x10);
    gc1603_write_register(ViPipe, 0x0100, 0x09);

    printf("===============================================================\n");
    printf("== Galaxycore gc1603 sensor 720P120fps linear init success!  ==\n");
    printf("===============================================================\n");
    return SC_SUCCESS;
}
