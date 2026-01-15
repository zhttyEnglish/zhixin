/**
 * @file     sc2310_sensor_ctl.c
 * @brief    SMART SEMS SC2310 SENSOR读写和初始化的接口实现
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-12-02 创建文件
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


#define SC2310_FLIP_MIRROR (0x3221)

#define SENSOR_1080P_30FPS_LINEAR_MODE  (1)
#define SENSOR_1080P_30FPS_2T1_WDR_MODE (2)

const unsigned char sc2310_i2c_addr  = 0x60; /* I2C Address of sc2310 */
const unsigned int  sc2310_addr_byte = 2;
const unsigned int  sc2310_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

extern ISP_SNS_STATE_S   *g_pastSc2310[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U  g_aunSc2310BusInfo[];

int sc2310_linear_1080p30_init(VI_PIPE ViPipe);
int sc2310_wdr_1080p30_2to1_init(VI_PIPE ViPipe);



int sc2310_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {
        return SC_SUCCESS;
    }

    u8DevNum = g_aunSc2310BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (sc2310_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int sc2310_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int sc2310_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:
    return SC_SUCCESS;
}

int sc2310_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(sc2310_addr_byte == 2)
    {
        buf[idx] = (addr >> 8) & 0xff;
        idx++;
        buf[idx] = addr & 0xff;
        idx++;
    }
    else
    {
        //buf[idx] = addr & 0xff;
        //idx++;
    }

    if(sc2310_data_byte == 2)
    {
        //buf[idx] = (data >> 8) & 0xff;
        //idx++;
        //buf[idx] = data & 0xff;
        //idx++;
    }
    else
    {
        buf[idx] = data & 0xff;
        idx++;
    }

    ret = write(g_fd[ViPipe], buf, sc2310_addr_byte + sc2310_data_byte);
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

void sc2310_prog(VI_PIPE ViPipe, int *rom)
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
            sc2310_write_register(ViPipe, addr, data);
        }
    }
}

void sc2310_standby(VI_PIPE ViPipe)
{
    return;
}

void sc2310_restart(VI_PIPE ViPipe)
{
    return;
}

int sc2310_flip_off_mirror_off(VI_PIPE ViPipe)
{
    sc2310_write_register(ViPipe, SC2310_FLIP_MIRROR, 0x00);

    return SC_SUCCESS;
}

int sc2310_flip_on_mirror_off(VI_PIPE ViPipe)
{
    sc2310_write_register(ViPipe, SC2310_FLIP_MIRROR, 0xE0);

    return SC_SUCCESS;
}

int sc2310_flip_off_mirror_on(VI_PIPE ViPipe)
{
    sc2310_write_register(ViPipe, SC2310_FLIP_MIRROR, 0x06);

    return SC_SUCCESS;
}

int sc2310_flip_on_mirror_on(VI_PIPE ViPipe)
{
    sc2310_write_register(ViPipe, SC2310_FLIP_MIRROR, 0xE6);

    return SC_SUCCESS;
}

void sc2310_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i = 0;

    for(i = 0; i < g_pastSc2310[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastSc2310[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            sc2310_write_register(ViPipe,
                g_pastSc2310[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                g_pastSc2310[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int sc2310_init(VI_PIPE ViPipe)
{
    WDR_MODE_E enWDRMode = WDR_MODE_NONE;
    SC_BOOL    bInit     = SC_FALSE;
    SC_U8      u8ImgMode = 0;
    SC_S32     ret       = SC_FAILURE;

    bInit     = g_pastSc2310[ViPipe]->bInit;
    enWDRMode = g_pastSc2310[ViPipe]->enWDRMode;
    u8ImgMode = g_pastSc2310[ViPipe]->u8ImgMode;
    printf("SC2310: bInit[%d] enWDRMode[%d] u8ImgMode[%u]\n", bInit, enWDRMode, (SC_U32)u8ImgMode);

    ret = sc2310_i2c_init(ViPipe);
    if(SC_SUCCESS != ret)
    {
        printf("Init sc2310 i2c error!\n");
        return SC_FAILURE;
    }

    /* When sensor first init, config all registers */
    if(SC_FALSE == bInit)
    {
        if(WDR_MODE_2To1_LINE == enWDRMode)
        {
            if(SENSOR_1080P_30FPS_2T1_WDR_MODE == u8ImgMode)
            {
                ret = sc2310_wdr_1080p30_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init sc2310 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = sc2310_linear_1080p30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init sc2310 linear mode error!\n");
                return SC_FAILURE;
            }
        }
    }
    /* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
    else
    {
        if(WDR_MODE_2To1_LINE == enWDRMode)
        {
            if(SENSOR_1080P_30FPS_2T1_WDR_MODE == u8ImgMode)
            {
                ret = sc2310_wdr_1080p30_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init sc2310 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = sc2310_linear_1080p30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init sc2310 linear mode error!\n");
                return SC_FAILURE;
            }
        }
    }

    g_pastSc2310[ViPipe]->bInit = SC_TRUE;
    return SC_SUCCESS;
}

void sc2310_exit(VI_PIPE ViPipe)
{
    sc2310_i2c_exit(ViPipe);
    return;
}

int sc2310_linear_1080p30_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = sc2310_write_register(ViPipe, 0x0103, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write sc2310 register error!\n");
        return SC_FAILURE;
    }

    sc2310_write_register(ViPipe, 0x0100, 0x00);
    sc2310_write_register(ViPipe, 0x36e9, 0xa3);
    sc2310_write_register(ViPipe, 0x36f9, 0x85);
    sc2310_write_register(ViPipe, 0x337f, 0x03);
    sc2310_write_register(ViPipe, 0x3368, 0x04);
    sc2310_write_register(ViPipe, 0x3369, 0x00);
    sc2310_write_register(ViPipe, 0x336a, 0x00);
    sc2310_write_register(ViPipe, 0x336b, 0x00);
    sc2310_write_register(ViPipe, 0x3367, 0x08);
    sc2310_write_register(ViPipe, 0x3326, 0x00);
    sc2310_write_register(ViPipe, 0x3631, 0x88);
    sc2310_write_register(ViPipe, 0x3018, 0x33);
    sc2310_write_register(ViPipe, 0x3031, 0x0c);
    sc2310_write_register(ViPipe, 0x3001, 0xfe);
    sc2310_write_register(ViPipe, 0x4603, 0x00);
    sc2310_write_register(ViPipe, 0x4837, 0x35);
    sc2310_write_register(ViPipe, 0x303f, 0x01);
    sc2310_write_register(ViPipe, 0x3640, 0x00);
    sc2310_write_register(ViPipe, 0x3636, 0x65);
    sc2310_write_register(ViPipe, 0x3907, 0x01);
    sc2310_write_register(ViPipe, 0x3908, 0x01);
    sc2310_write_register(ViPipe, 0x3320, 0x01);
    sc2310_write_register(ViPipe, 0x3366, 0x80);
    sc2310_write_register(ViPipe, 0x57a4, 0xf0);
    sc2310_write_register(ViPipe, 0x3333, 0x30);
    sc2310_write_register(ViPipe, 0x331b, 0x83);
    sc2310_write_register(ViPipe, 0x3334, 0x40);
    sc2310_write_register(ViPipe, 0x320c, 0x04);
    sc2310_write_register(ViPipe, 0x320d, 0x4c);
    sc2310_write_register(ViPipe, 0x3302, 0x10);
    sc2310_write_register(ViPipe, 0x3308, 0x08);
    sc2310_write_register(ViPipe, 0x3303, 0x18);
    sc2310_write_register(ViPipe, 0x331e, 0x11);
    sc2310_write_register(ViPipe, 0x3622, 0xe6);
    sc2310_write_register(ViPipe, 0x3633, 0x22);
    sc2310_write_register(ViPipe, 0x3630, 0xc8);
    sc2310_write_register(ViPipe, 0x3301, 0x10);
    sc2310_write_register(ViPipe, 0x36eb, 0x0b);
    sc2310_write_register(ViPipe, 0x36ec, 0x0f);
    sc2310_write_register(ViPipe, 0x33aa, 0x00);
    sc2310_write_register(ViPipe, 0x391e, 0x00);
    sc2310_write_register(ViPipe, 0x391f, 0xc0);
    sc2310_write_register(ViPipe, 0x3634, 0x44);
    sc2310_write_register(ViPipe, 0x4500, 0x59);
    sc2310_write_register(ViPipe, 0x3623, 0x18);
    sc2310_write_register(ViPipe, 0x3f08, 0x04);
    sc2310_write_register(ViPipe, 0x3f00, 0x0d);
    sc2310_write_register(ViPipe, 0x3f04, 0x02);
    sc2310_write_register(ViPipe, 0x3f05, 0x1e);
    sc2310_write_register(ViPipe, 0x336c, 0x42);
    sc2310_write_register(ViPipe, 0x3933, 0x28);
    sc2310_write_register(ViPipe, 0x3934, 0x0a);
    sc2310_write_register(ViPipe, 0x3940, 0x1b);
    sc2310_write_register(ViPipe, 0x3941, 0x40);
    sc2310_write_register(ViPipe, 0x3942, 0x08);
    sc2310_write_register(ViPipe, 0x3943, 0x0e);
    sc2310_write_register(ViPipe, 0x3624, 0x47);
    sc2310_write_register(ViPipe, 0x3621, 0xac);
    sc2310_write_register(ViPipe, 0x3638, 0x25);
    sc2310_write_register(ViPipe, 0x3635, 0x40);
    sc2310_write_register(ViPipe, 0x363b, 0x08);
    sc2310_write_register(ViPipe, 0x363c, 0x05);
    sc2310_write_register(ViPipe, 0x363d, 0x05);
    sc2310_write_register(ViPipe, 0x3324, 0x02);
    sc2310_write_register(ViPipe, 0x3325, 0x02);
    sc2310_write_register(ViPipe, 0x333d, 0x08);
    sc2310_write_register(ViPipe, 0x36fa, 0xa8);
    sc2310_write_register(ViPipe, 0x3314, 0x04);
    sc2310_write_register(ViPipe, 0x3802, 0x00);
    sc2310_write_register(ViPipe, 0x3e14, 0xb0);
    sc2310_write_register(ViPipe, 0x3e1e, 0x35);
    sc2310_write_register(ViPipe, 0x3e0e, 0x66);
    sc2310_write_register(ViPipe, 0x6000, 0x00);
    sc2310_write_register(ViPipe, 0x6002, 0x00);
    sc2310_write_register(ViPipe, 0x301c, 0x78);
    sc2310_write_register(ViPipe, 0x3037, 0x42);
    sc2310_write_register(ViPipe, 0x3038, 0x22);
    sc2310_write_register(ViPipe, 0x3632, 0x18);
    sc2310_write_register(ViPipe, 0x4809, 0x01);
    sc2310_write_register(ViPipe, 0x5000, 0x06);
    sc2310_write_register(ViPipe, 0x5780, 0x7f);
    sc2310_write_register(ViPipe, 0x57a0, 0x00);
    sc2310_write_register(ViPipe, 0x57a1, 0x74);
    sc2310_write_register(ViPipe, 0x57a2, 0x01);
    sc2310_write_register(ViPipe, 0x57a3, 0xf4);
    sc2310_write_register(ViPipe, 0x5781, 0x06);
    sc2310_write_register(ViPipe, 0x5782, 0x04);
    sc2310_write_register(ViPipe, 0x5783, 0x02);
    sc2310_write_register(ViPipe, 0x5784, 0x01);
    sc2310_write_register(ViPipe, 0x5785, 0x16);
    sc2310_write_register(ViPipe, 0x5786, 0x12);
    sc2310_write_register(ViPipe, 0x5787, 0x08);
    sc2310_write_register(ViPipe, 0x5788, 0x02);
    sc2310_write_register(ViPipe, 0x4501, 0xb4);
    sc2310_write_register(ViPipe, 0x4509, 0x20);
    sc2310_write_register(ViPipe, 0x331f, 0x29);
    sc2310_write_register(ViPipe, 0x3309, 0x30);
    sc2310_write_register(ViPipe, 0x330a, 0x00);
    sc2310_write_register(ViPipe, 0x330b, 0xc8);
    sc2310_write_register(ViPipe, 0x3306, 0x60);
    sc2310_write_register(ViPipe, 0x330e, 0x28);
    sc2310_write_register(ViPipe, 0x3364, 0x1d);
    sc2310_write_register(ViPipe, 0x33b6, 0x07);
    sc2310_write_register(ViPipe, 0x33b7, 0x07);
    sc2310_write_register(ViPipe, 0x33b8, 0x10);
    sc2310_write_register(ViPipe, 0x33b9, 0x10);
    sc2310_write_register(ViPipe, 0x33ba, 0x10);
    sc2310_write_register(ViPipe, 0x33bb, 0x07);
    sc2310_write_register(ViPipe, 0x33bc, 0x07);
    sc2310_write_register(ViPipe, 0x33bd, 0x20);
    sc2310_write_register(ViPipe, 0x33be, 0x20);
    sc2310_write_register(ViPipe, 0x33bf, 0x20);
    sc2310_write_register(ViPipe, 0x360f, 0x05);
    sc2310_write_register(ViPipe, 0x367a, 0x40);
    sc2310_write_register(ViPipe, 0x367b, 0x40);
    sc2310_write_register(ViPipe, 0x3671, 0xf6);
    sc2310_write_register(ViPipe, 0x3672, 0x16);
    sc2310_write_register(ViPipe, 0x3673, 0x16);
    sc2310_write_register(ViPipe, 0x366e, 0x04);
    sc2310_write_register(ViPipe, 0x3670, 0x4a);
    sc2310_write_register(ViPipe, 0x367c, 0x40);
    sc2310_write_register(ViPipe, 0x367d, 0x58);
    sc2310_write_register(ViPipe, 0x3674, 0xc8);
    sc2310_write_register(ViPipe, 0x3675, 0x54);
    sc2310_write_register(ViPipe, 0x3676, 0x18);
    sc2310_write_register(ViPipe, 0x367e, 0x40);
    sc2310_write_register(ViPipe, 0x367f, 0x58);
    sc2310_write_register(ViPipe, 0x3677, 0x22);
    sc2310_write_register(ViPipe, 0x3678, 0x33);
    sc2310_write_register(ViPipe, 0x3679, 0x44);
    sc2310_write_register(ViPipe, 0x36a0, 0x58);
    sc2310_write_register(ViPipe, 0x36a1, 0x78);
    sc2310_write_register(ViPipe, 0x3696, 0x83);
    sc2310_write_register(ViPipe, 0x3697, 0x87);
    sc2310_write_register(ViPipe, 0x3698, 0x9f);
    sc2310_write_register(ViPipe, 0x3637, 0x17);
    sc2310_write_register(ViPipe, 0x3200, 0x00);
    sc2310_write_register(ViPipe, 0x3201, 0x04);
    sc2310_write_register(ViPipe, 0x3202, 0x00);
    sc2310_write_register(ViPipe, 0x3203, 0x04);
    sc2310_write_register(ViPipe, 0x3204, 0x07);
    sc2310_write_register(ViPipe, 0x3205, 0x8b);
    sc2310_write_register(ViPipe, 0x3206, 0x04);
    sc2310_write_register(ViPipe, 0x3207, 0x43);
    sc2310_write_register(ViPipe, 0x3208, 0x07);
    sc2310_write_register(ViPipe, 0x3209, 0x80);
    sc2310_write_register(ViPipe, 0x320a, 0x04);
    sc2310_write_register(ViPipe, 0x320b, 0x38);
    sc2310_write_register(ViPipe, 0x3211, 0x04);
    sc2310_write_register(ViPipe, 0x3213, 0x04);
    sc2310_write_register(ViPipe, 0x3380, 0x1b);
    sc2310_write_register(ViPipe, 0x3341, 0x07);
    sc2310_write_register(ViPipe, 0x3343, 0x03);
    sc2310_write_register(ViPipe, 0x3e25, 0x03);
    sc2310_write_register(ViPipe, 0x3e26, 0x40);
    sc2310_write_register(ViPipe, 0x3905, 0xd8);
    sc2310_write_register(ViPipe, 0x3e00, 0x00);
    sc2310_write_register(ViPipe, 0x3e01, 0x8c);
    sc2310_write_register(ViPipe, 0x3e02, 0x40);
    sc2310_write_register(ViPipe, 0x3e03, 0x0b);
    sc2310_write_register(ViPipe, 0x3e06, 0x00);
    sc2310_write_register(ViPipe, 0x3e07, 0x80);
    sc2310_write_register(ViPipe, 0x3e08, 0x03);
    sc2310_write_register(ViPipe, 0x3e09, 0x40);
    sc2310_write_register(ViPipe, 0x36e9, 0x23);
    sc2310_write_register(ViPipe, 0x36f9, 0x05);
    sc2310_write_register(ViPipe, 0x0100, 0x01);

    printf("===================================================================================\n");
    printf("=== SC2310_MIPI_27MInput_445.5Mbps_2lane_12bit_1920x1080_30fps                  ===\n");
    printf("===================================================================================\n");

    return SC_SUCCESS;
}

int sc2310_wdr_1080p30_2to1_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = sc2310_write_register(ViPipe, 0x0103, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write sc2310 register error!\n");
        return SC_FAILURE;
    }

    sc2310_write_register(ViPipe, 0x0100, 0x00);
    sc2310_write_register(ViPipe, 0x36e9, 0xa6); //bypass pll1
    sc2310_write_register(ViPipe, 0x36f9, 0x85); //bypass pll2
    sc2310_write_register(ViPipe, 0x4509, 0x10);
    sc2310_write_register(ViPipe, 0x337f, 0x03);
    sc2310_write_register(ViPipe, 0x3368, 0x04);
    sc2310_write_register(ViPipe, 0x3369, 0x00);
    sc2310_write_register(ViPipe, 0x336a, 0x00);
    sc2310_write_register(ViPipe, 0x336b, 0x00);
    sc2310_write_register(ViPipe, 0x3367, 0x08);
    sc2310_write_register(ViPipe, 0x3326, 0x00);
    sc2310_write_register(ViPipe, 0x3631, 0x88);
    sc2310_write_register(ViPipe, 0x3018, 0x33);
    sc2310_write_register(ViPipe, 0x3031, 0x0a);
    sc2310_write_register(ViPipe, 0x3001, 0xfe);
    sc2310_write_register(ViPipe, 0x4603, 0x00);
    sc2310_write_register(ViPipe, 0x4837, 0x1a);
    sc2310_write_register(ViPipe, 0x303f, 0x01);
    sc2310_write_register(ViPipe, 0x3640, 0x00);
    sc2310_write_register(ViPipe, 0x3907, 0x01);
    sc2310_write_register(ViPipe, 0x3908, 0x01);
    sc2310_write_register(ViPipe, 0x3320, 0x01);
    sc2310_write_register(ViPipe, 0x57a4, 0xf0);
    sc2310_write_register(ViPipe, 0x3333, 0x30);
    sc2310_write_register(ViPipe, 0x331b, 0x83);
    sc2310_write_register(ViPipe, 0x3334, 0x40);
    sc2310_write_register(ViPipe, 0x3302, 0x10);
    sc2310_write_register(ViPipe, 0x36ea, 0x35);
    sc2310_write_register(ViPipe, 0x36eb, 0x0a);
    sc2310_write_register(ViPipe, 0x36ec, 0x0e);
    sc2310_write_register(ViPipe, 0x320c, 0x04);
    sc2310_write_register(ViPipe, 0x320d, 0x4c);
    sc2310_write_register(ViPipe, 0x3f08, 0x04);
    sc2310_write_register(ViPipe, 0x4501, 0xa4);
    sc2310_write_register(ViPipe, 0x3309, 0x48);
    sc2310_write_register(ViPipe, 0x331f, 0x39);
    sc2310_write_register(ViPipe, 0x330a, 0x00);
    sc2310_write_register(ViPipe, 0x3308, 0x10);
    sc2310_write_register(ViPipe, 0x3366, 0xc0);
    sc2310_write_register(ViPipe, 0x33aa, 0x00);
    sc2310_write_register(ViPipe, 0x391e, 0x00);
    sc2310_write_register(ViPipe, 0x391f, 0xc0);
    sc2310_write_register(ViPipe, 0x3634, 0x44);
    sc2310_write_register(ViPipe, 0x4500, 0x59);
    sc2310_write_register(ViPipe, 0x3623, 0x18);
    sc2310_write_register(ViPipe, 0x3f00, 0x0d);
    sc2310_write_register(ViPipe, 0x3f04, 0x02);
    sc2310_write_register(ViPipe, 0x3f05, 0x1e);
    sc2310_write_register(ViPipe, 0x336c, 0x42);
    sc2310_write_register(ViPipe, 0x3933, 0x28);
    sc2310_write_register(ViPipe, 0x3934, 0x0a);
    sc2310_write_register(ViPipe, 0x3940, 0x1b);
    sc2310_write_register(ViPipe, 0x3941, 0x40);
    sc2310_write_register(ViPipe, 0x3942, 0x08);
    sc2310_write_register(ViPipe, 0x3943, 0x0e);
    sc2310_write_register(ViPipe, 0x3624, 0x47);
    sc2310_write_register(ViPipe, 0x3621, 0xac);
    sc2310_write_register(ViPipe, 0x3222, 0x29);
    sc2310_write_register(ViPipe, 0x3901, 0x02);
    sc2310_write_register(ViPipe, 0x363b, 0x08);
    sc2310_write_register(ViPipe, 0x363c, 0x05);
    sc2310_write_register(ViPipe, 0x363d, 0x05);
    sc2310_write_register(ViPipe, 0x3324, 0x02);
    sc2310_write_register(ViPipe, 0x3325, 0x02);
    sc2310_write_register(ViPipe, 0x333d, 0x08);
    sc2310_write_register(ViPipe, 0x36fa, 0xa8);
    sc2310_write_register(ViPipe, 0x3314, 0x04);
    sc2310_write_register(ViPipe, 0x3802, 0x00);
    sc2310_write_register(ViPipe, 0x3e14, 0xb0);
    sc2310_write_register(ViPipe, 0x3e1e, 0x35);
    sc2310_write_register(ViPipe, 0x3e0e, 0x66);
    sc2310_write_register(ViPipe, 0x3364, 0x1d);
    sc2310_write_register(ViPipe, 0x33b6, 0x07);
    sc2310_write_register(ViPipe, 0x33b7, 0x07);
    sc2310_write_register(ViPipe, 0x33b8, 0x10);
    sc2310_write_register(ViPipe, 0x33b9, 0x10);
    sc2310_write_register(ViPipe, 0x33ba, 0x10);
    sc2310_write_register(ViPipe, 0x33bb, 0x07);
    sc2310_write_register(ViPipe, 0x33bc, 0x07);
    sc2310_write_register(ViPipe, 0x33bd, 0x18);
    sc2310_write_register(ViPipe, 0x33be, 0x18);
    sc2310_write_register(ViPipe, 0x33bf, 0x18);
    sc2310_write_register(ViPipe, 0x360f, 0x05);
    sc2310_write_register(ViPipe, 0x367a, 0x40);
    sc2310_write_register(ViPipe, 0x367b, 0x40);
    sc2310_write_register(ViPipe, 0x3671, 0xf6);
    sc2310_write_register(ViPipe, 0x3672, 0x16);
    sc2310_write_register(ViPipe, 0x3673, 0x16);
    sc2310_write_register(ViPipe, 0x366e, 0x04);
    sc2310_write_register(ViPipe, 0x367c, 0x40);
    sc2310_write_register(ViPipe, 0x367d, 0x58);
    sc2310_write_register(ViPipe, 0x3674, 0xc8);
    sc2310_write_register(ViPipe, 0x3675, 0x54);
    sc2310_write_register(ViPipe, 0x3676, 0x18);
    sc2310_write_register(ViPipe, 0x367e, 0x40);
    sc2310_write_register(ViPipe, 0x367f, 0x58);
    sc2310_write_register(ViPipe, 0x3677, 0x22);
    sc2310_write_register(ViPipe, 0x3678, 0x53);
    sc2310_write_register(ViPipe, 0x3679, 0x55);
    sc2310_write_register(ViPipe, 0x36a0, 0x58);
    sc2310_write_register(ViPipe, 0x36a1, 0x78);
    sc2310_write_register(ViPipe, 0x3696, 0x9f);
    sc2310_write_register(ViPipe, 0x3697, 0x9f);
    sc2310_write_register(ViPipe, 0x3698, 0x9f);
    sc2310_write_register(ViPipe, 0x301c, 0x78);
    sc2310_write_register(ViPipe, 0x3037, 0x24);
    sc2310_write_register(ViPipe, 0x3038, 0x44);
    sc2310_write_register(ViPipe, 0x3632, 0x18);
    sc2310_write_register(ViPipe, 0x4809, 0x01);
    sc2310_write_register(ViPipe, 0x3636, 0x62);
    sc2310_write_register(ViPipe, 0x3625, 0x01);
    sc2310_write_register(ViPipe, 0x3670, 0x6a);
    sc2310_write_register(ViPipe, 0x369e, 0x40);
    sc2310_write_register(ViPipe, 0x369f, 0x40);
    sc2310_write_register(ViPipe, 0x3693, 0x20);
    sc2310_write_register(ViPipe, 0x3694, 0x40);
    sc2310_write_register(ViPipe, 0x3695, 0x40);
    sc2310_write_register(ViPipe, 0x5000, 0x06);
    sc2310_write_register(ViPipe, 0x5780, 0x7f);
    sc2310_write_register(ViPipe, 0x57a0, 0x00);
    sc2310_write_register(ViPipe, 0x57a1, 0x74);
    sc2310_write_register(ViPipe, 0x57a2, 0x01);
    sc2310_write_register(ViPipe, 0x57a3, 0xf4);
    sc2310_write_register(ViPipe, 0x5781, 0x06);
    sc2310_write_register(ViPipe, 0x5782, 0x04);
    sc2310_write_register(ViPipe, 0x5783, 0x02);
    sc2310_write_register(ViPipe, 0x5784, 0x01);
    sc2310_write_register(ViPipe, 0x5785, 0x16);
    sc2310_write_register(ViPipe, 0x5786, 0x12);
    sc2310_write_register(ViPipe, 0x5787, 0x08);
    sc2310_write_register(ViPipe, 0x5788, 0x02);
    sc2310_write_register(ViPipe, 0x3637, 0x0c);
    sc2310_write_register(ViPipe, 0x3638, 0x24);
    sc2310_write_register(ViPipe, 0x331e, 0x21);
    sc2310_write_register(ViPipe, 0x3303, 0x30);
    sc2310_write_register(ViPipe, 0x330b, 0xb4);
    sc2310_write_register(ViPipe, 0x3306, 0x54);
    sc2310_write_register(ViPipe, 0x330e, 0x30);
    sc2310_write_register(ViPipe, 0x3200, 0x00);
    sc2310_write_register(ViPipe, 0x3201, 0x04);
    sc2310_write_register(ViPipe, 0x3204, 0x07);
    sc2310_write_register(ViPipe, 0x3205, 0x8b);
    sc2310_write_register(ViPipe, 0x3208, 0x07);
    sc2310_write_register(ViPipe, 0x3209, 0x80);
    sc2310_write_register(ViPipe, 0x320a, 0x04);
    sc2310_write_register(ViPipe, 0x320b, 0x38);
    sc2310_write_register(ViPipe, 0x3211, 0x04);
    sc2310_write_register(ViPipe, 0x3213, 0x04);
    sc2310_write_register(ViPipe, 0x4816, 0x51);
    sc2310_write_register(ViPipe, 0x3220, 0x51);
    sc2310_write_register(ViPipe, 0x4602, 0x0f);
    sc2310_write_register(ViPipe, 0x33c0, 0x05);
    sc2310_write_register(ViPipe, 0x6000, 0x06);
    sc2310_write_register(ViPipe, 0x6002, 0x06);
    sc2310_write_register(ViPipe, 0x320e, 0x08);
    sc2310_write_register(ViPipe, 0x320f, 0xca);
    sc2310_write_register(ViPipe, 0x3380, 0x1b);
    sc2310_write_register(ViPipe, 0x3341, 0x07);
    sc2310_write_register(ViPipe, 0x3343, 0x03);
    sc2310_write_register(ViPipe, 0x3e25, 0x03);
    sc2310_write_register(ViPipe, 0x3e26, 0x40);
    sc2310_write_register(ViPipe, 0x391d, 0x24);
    sc2310_write_register(ViPipe, 0x3202, 0x00);
    sc2310_write_register(ViPipe, 0x3203, 0x00);
    sc2310_write_register(ViPipe, 0x3206, 0x04);
    sc2310_write_register(ViPipe, 0x3207, 0x3f);
    sc2310_write_register(ViPipe, 0x3e00, 0x01);
    sc2310_write_register(ViPipe, 0x3e01, 0x07);
    sc2310_write_register(ViPipe, 0x3e02, 0xa0);
    sc2310_write_register(ViPipe, 0x3e04, 0x10);
    sc2310_write_register(ViPipe, 0x3e05, 0x80);
    sc2310_write_register(ViPipe, 0x3e23, 0x00);
    sc2310_write_register(ViPipe, 0x3e24, 0x88);
    sc2310_write_register(ViPipe, 0x3e03, 0x0b);
    sc2310_write_register(ViPipe, 0x3e06, 0x00);
    sc2310_write_register(ViPipe, 0x3e07, 0x80);
    sc2310_write_register(ViPipe, 0x3e08, 0x03);
    sc2310_write_register(ViPipe, 0x3e09, 0x40);
    sc2310_write_register(ViPipe, 0x3622, 0xf6);
    sc2310_write_register(ViPipe, 0x3633, 0x22);
    sc2310_write_register(ViPipe, 0x3630, 0xc8);
    sc2310_write_register(ViPipe, 0x3301, 0x10);
    sc2310_write_register(ViPipe, 0x363a, 0x83);
    sc2310_write_register(ViPipe, 0x3635, 0x20);
    sc2310_write_register(ViPipe, 0x36e9, 0x26);
    sc2310_write_register(ViPipe, 0x36f9, 0x05);
    sc2310_write_register(ViPipe, 0x0100, 0x01);

    printf("=========================================================================\n");
    printf("=== SC2310_MIPI_27MInput_742.5Mbps_2lane_10bit_1920x1080_HDR 30fps(VC) ==\n");
    printf("=========================================================================\n");

    return SC_SUCCESS;
}
