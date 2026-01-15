/**
 * @file     sc2210_sensor_ctl.c
 * @brief    SMART SEMS SC2210 SENSOR读写和初始化的接口实现
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2022-08-01 创建文件
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


#define SC2210_FLIP_MIRROR (0x3221)

#define SENSOR_1080P_30FPS_LINEAR_MODE  (1)
#define SENSOR_1080P_30FPS_2T1_WDR_MODE (2)

const unsigned char sc2210_i2c_addr  = 0x60; /* I2C Address of sc2210 */
const unsigned int  sc2210_addr_byte = 2;
const unsigned int  sc2210_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

extern ISP_SNS_STATE_S   *g_pastSc2210[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U  g_aunSc2210BusInfo[];

int sc2210_linear_1080p30_init(VI_PIPE ViPipe);
int sc2210_wdr_1080p30_2to1_init(VI_PIPE ViPipe);



int sc2210_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {
        return SC_SUCCESS;
    }

    u8DevNum = g_aunSc2210BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (sc2210_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int sc2210_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int sc2210_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:
    return SC_SUCCESS;
}

int sc2210_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(sc2210_addr_byte == 2)
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

    if(sc2210_data_byte == 2)
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

    ret = write(g_fd[ViPipe], buf, sc2210_addr_byte + sc2210_data_byte);
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

void sc2210_prog(VI_PIPE ViPipe, int *rom)
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
            sc2210_write_register(ViPipe, addr, data);
        }
    }
}

void sc2210_standby(VI_PIPE ViPipe)
{
    return;
}

void sc2210_restart(VI_PIPE ViPipe)
{
    return;
}

int sc2210_flip_off_mirror_off(VI_PIPE ViPipe)
{
    sc2210_write_register(ViPipe, SC2210_FLIP_MIRROR, 0x00);

    return SC_SUCCESS;
}

int sc2210_flip_on_mirror_off(VI_PIPE ViPipe)
{
    sc2210_write_register(ViPipe, SC2210_FLIP_MIRROR, 0x60);

    return SC_SUCCESS;
}

int sc2210_flip_off_mirror_on(VI_PIPE ViPipe)
{
    sc2210_write_register(ViPipe, SC2210_FLIP_MIRROR, 0x06);

    return SC_SUCCESS;
}

int sc2210_flip_on_mirror_on(VI_PIPE ViPipe)
{
    sc2210_write_register(ViPipe, SC2210_FLIP_MIRROR, 0x66);

    return SC_SUCCESS;
}

void sc2210_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i = 0;

    for(i = 0; i < g_pastSc2210[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastSc2210[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            sc2210_write_register(ViPipe,
                g_pastSc2210[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                g_pastSc2210[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int sc2210_init(VI_PIPE ViPipe)
{
    WDR_MODE_E enWDRMode = WDR_MODE_NONE;
    SC_BOOL    bInit     = SC_FALSE;
    SC_U8      u8ImgMode = 0;
    SC_S32     ret       = SC_FAILURE;

    bInit     = g_pastSc2210[ViPipe]->bInit;
    enWDRMode = g_pastSc2210[ViPipe]->enWDRMode;
    u8ImgMode = g_pastSc2210[ViPipe]->u8ImgMode;
    printf("SC2210: bInit[%d] enWDRMode[%d] u8ImgMode[%u]\n", bInit, enWDRMode, (SC_U32)u8ImgMode);

    ret = sc2210_i2c_init(ViPipe);
    if(SC_SUCCESS != ret)
    {
        printf("Init sc2210 i2c error!\n");
        return SC_FAILURE;
    }

    /* When sensor first init, config all registers */
    if(SC_FALSE == bInit)
    {
        if(WDR_MODE_2To1_LINE == enWDRMode)
        {
            if(SENSOR_1080P_30FPS_2T1_WDR_MODE == u8ImgMode)
            {
                ret = sc2210_wdr_1080p30_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init sc2210 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = sc2210_linear_1080p30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init sc2210 linear mode error!\n");
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
                ret = sc2210_wdr_1080p30_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init sc2210 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = sc2210_linear_1080p30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init sc2210 linear mode error!\n");
                return SC_FAILURE;
            }
        }
    }

    g_pastSc2210[ViPipe]->bInit = SC_TRUE;
    return SC_SUCCESS;
}

void sc2210_exit(VI_PIPE ViPipe)
{
    sc2210_i2c_exit(ViPipe);
    return;
}

int sc2210_linear_1080p30_pre_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = sc2210_i2c_init(ViPipe);
    if(SC_SUCCESS != ret)
    {
        printf("Init sc2210 i2c error!\n");
        return SC_FAILURE;
    }

    ret = sc2210_write_register(ViPipe, 0x0103, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write sc2210 register error!\n");
        return SC_FAILURE;
    }

    sc2210_write_register(ViPipe, 0x0100, 0x00);
    sc2210_write_register(ViPipe, 0x36e9, 0x80);
    sc2210_write_register(ViPipe, 0x36f9, 0x80);
    sc2210_write_register(ViPipe, 0x3001, 0x07);
    sc2210_write_register(ViPipe, 0x3002, 0xc0);
    sc2210_write_register(ViPipe, 0x300a, 0x2c);
    sc2210_write_register(ViPipe, 0x300f, 0x00);
    sc2210_write_register(ViPipe, 0x3018, 0x33);
    sc2210_write_register(ViPipe, 0x3019, 0x0c);
    sc2210_write_register(ViPipe, 0x301f, 0x42);
    sc2210_write_register(ViPipe, 0x3031, 0x0c);
    sc2210_write_register(ViPipe, 0x3033, 0x20);
    sc2210_write_register(ViPipe, 0x3038, 0x22);
    sc2210_write_register(ViPipe, 0x3106, 0x81);
    sc2210_write_register(ViPipe, 0x3201, 0x04);
    sc2210_write_register(ViPipe, 0x3203, 0x04);
    sc2210_write_register(ViPipe, 0x3204, 0x07);
    sc2210_write_register(ViPipe, 0x3205, 0x8b);
    sc2210_write_register(ViPipe, 0x3206, 0x04);
    sc2210_write_register(ViPipe, 0x3207, 0x43);
    sc2210_write_register(ViPipe, 0x320c, 0x04);
    sc2210_write_register(ViPipe, 0x320d, 0x4c);
    sc2210_write_register(ViPipe, 0x320e, 0x04);
    sc2210_write_register(ViPipe, 0x320f, 0x65);
    sc2210_write_register(ViPipe, 0x3211, 0x04);
    sc2210_write_register(ViPipe, 0x3213, 0x04);
    sc2210_write_register(ViPipe, 0x3231, 0x02);
    sc2210_write_register(ViPipe, 0x3253, 0x04);
    sc2210_write_register(ViPipe, 0x3301, 0x0a);
    sc2210_write_register(ViPipe, 0x3302, 0x10);
    sc2210_write_register(ViPipe, 0x3304, 0x48);
    sc2210_write_register(ViPipe, 0x3305, 0x00);
    sc2210_write_register(ViPipe, 0x3306, 0x68);
    sc2210_write_register(ViPipe, 0x3308, 0x20);
    sc2210_write_register(ViPipe, 0x3309, 0x98);
    sc2210_write_register(ViPipe, 0x330a, 0x00);
    sc2210_write_register(ViPipe, 0x330b, 0xe8);
    sc2210_write_register(ViPipe, 0x330e, 0x68);
    sc2210_write_register(ViPipe, 0x3314, 0x92);
    sc2210_write_register(ViPipe, 0x331e, 0x41);
    sc2210_write_register(ViPipe, 0x3000, 0xc0);
    sc2210_write_register(ViPipe, 0x331f, 0x91);
    sc2210_write_register(ViPipe, 0x334c, 0x10);
    sc2210_write_register(ViPipe, 0x335d, 0x60);
    sc2210_write_register(ViPipe, 0x335e, 0x02);
    sc2210_write_register(ViPipe, 0x335f, 0x06);
    sc2210_write_register(ViPipe, 0x3364, 0x16);
    sc2210_write_register(ViPipe, 0x3366, 0x92);
    sc2210_write_register(ViPipe, 0x3367, 0x10);
    sc2210_write_register(ViPipe, 0x3368, 0x04);
    sc2210_write_register(ViPipe, 0x3369, 0x00);
    sc2210_write_register(ViPipe, 0x336a, 0x00);
    sc2210_write_register(ViPipe, 0x336b, 0x00);
    sc2210_write_register(ViPipe, 0x336d, 0x03);
    sc2210_write_register(ViPipe, 0x337c, 0x08);
    sc2210_write_register(ViPipe, 0x337d, 0x0e);
    sc2210_write_register(ViPipe, 0x337f, 0x33);
    sc2210_write_register(ViPipe, 0x3390, 0x10);
    sc2210_write_register(ViPipe, 0x3391, 0x30);
    sc2210_write_register(ViPipe, 0x3392, 0x40);
    sc2210_write_register(ViPipe, 0x3393, 0x0a);
    sc2210_write_register(ViPipe, 0x3394, 0x0a);
    sc2210_write_register(ViPipe, 0x3395, 0x0a);
    sc2210_write_register(ViPipe, 0x3396, 0x08);
    sc2210_write_register(ViPipe, 0x3397, 0x30);
    sc2210_write_register(ViPipe, 0x3398, 0x3f);
    sc2210_write_register(ViPipe, 0x3399, 0x50);
    sc2210_write_register(ViPipe, 0x339a, 0x50);
    sc2210_write_register(ViPipe, 0x339b, 0x50);
    sc2210_write_register(ViPipe, 0x339c, 0x50);
    sc2210_write_register(ViPipe, 0x33a2, 0x0a);
    sc2210_write_register(ViPipe, 0x33b9, 0x0e);
    sc2210_write_register(ViPipe, 0x33e1, 0x08);
    sc2210_write_register(ViPipe, 0x33e2, 0x18);
    sc2210_write_register(ViPipe, 0x33e3, 0x18);
    sc2210_write_register(ViPipe, 0x33e4, 0x18);
    sc2210_write_register(ViPipe, 0x33e5, 0x10);
    sc2210_write_register(ViPipe, 0x33e6, 0x06);
    sc2210_write_register(ViPipe, 0x33e7, 0x02);
    sc2210_write_register(ViPipe, 0x33e8, 0x18);
    sc2210_write_register(ViPipe, 0x33e9, 0x10);
    sc2210_write_register(ViPipe, 0x33ea, 0x0c);
    sc2210_write_register(ViPipe, 0x33eb, 0x10);
    sc2210_write_register(ViPipe, 0x33ec, 0x04);
    sc2210_write_register(ViPipe, 0x33ed, 0x02);
    sc2210_write_register(ViPipe, 0x33ee, 0xa0);
    sc2210_write_register(ViPipe, 0x33ef, 0x08);
    sc2210_write_register(ViPipe, 0x33f4, 0x18);
    sc2210_write_register(ViPipe, 0x33f5, 0x10);
    sc2210_write_register(ViPipe, 0x33f6, 0x0c);
    sc2210_write_register(ViPipe, 0x33f7, 0x10);
    sc2210_write_register(ViPipe, 0x33f8, 0x06);
    sc2210_write_register(ViPipe, 0x33f9, 0x02);
    sc2210_write_register(ViPipe, 0x33fa, 0x18);
    sc2210_write_register(ViPipe, 0x33fb, 0x10);
    sc2210_write_register(ViPipe, 0x33fc, 0x0c);
    sc2210_write_register(ViPipe, 0x33fd, 0x10);
    sc2210_write_register(ViPipe, 0x33fe, 0x04);
    sc2210_write_register(ViPipe, 0x33ff, 0x02);
    sc2210_write_register(ViPipe, 0x360f, 0x01);
    sc2210_write_register(ViPipe, 0x3622, 0xf7);
    sc2210_write_register(ViPipe, 0x3625, 0x0a);
    sc2210_write_register(ViPipe, 0x3627, 0x02);
    sc2210_write_register(ViPipe, 0x3630, 0xa2);
    sc2210_write_register(ViPipe, 0x3631, 0x00);
    sc2210_write_register(ViPipe, 0x3632, 0xd8);
    sc2210_write_register(ViPipe, 0x3633, 0x33);
    sc2210_write_register(ViPipe, 0x3635, 0x20);
    sc2210_write_register(ViPipe, 0x3638, 0x24);
    sc2210_write_register(ViPipe, 0x363a, 0x80);
    sc2210_write_register(ViPipe, 0x363b, 0x02);
    sc2210_write_register(ViPipe, 0x363e, 0x22);
    sc2210_write_register(ViPipe, 0x3670, 0x40);
    sc2210_write_register(ViPipe, 0x3671, 0xf7);
    sc2210_write_register(ViPipe, 0x3672, 0xf7);
    sc2210_write_register(ViPipe, 0x3673, 0x07);
    sc2210_write_register(ViPipe, 0x367a, 0x40);
    sc2210_write_register(ViPipe, 0x367b, 0x7f);
    sc2210_write_register(ViPipe, 0x36b5, 0x40);
    sc2210_write_register(ViPipe, 0x36b6, 0x7f);
    sc2210_write_register(ViPipe, 0x36c0, 0x80);
    sc2210_write_register(ViPipe, 0x36c1, 0x9f);
    sc2210_write_register(ViPipe, 0x36c2, 0x9f);
    sc2210_write_register(ViPipe, 0x36cc, 0x22);
    sc2210_write_register(ViPipe, 0x36cd, 0x23);
    sc2210_write_register(ViPipe, 0x36ce, 0x30);
    sc2210_write_register(ViPipe, 0x36d0, 0x20);
    sc2210_write_register(ViPipe, 0x36d1, 0x40);
    sc2210_write_register(ViPipe, 0x36d2, 0x7f);
    sc2210_write_register(ViPipe, 0x36ea, 0x35);
    sc2210_write_register(ViPipe, 0x36eb, 0x0f);
    sc2210_write_register(ViPipe, 0x36ec, 0x13);
    sc2210_write_register(ViPipe, 0x36ed, 0x14);
    sc2210_write_register(ViPipe, 0x36fa, 0x35);
    sc2210_write_register(ViPipe, 0x36fb, 0x1b);
    sc2210_write_register(ViPipe, 0x36fc, 0x10);
    sc2210_write_register(ViPipe, 0x36fd, 0x14);
    sc2210_write_register(ViPipe, 0x3905, 0xd8);
    sc2210_write_register(ViPipe, 0x3907, 0x01);
    sc2210_write_register(ViPipe, 0x3908, 0x11);
    sc2210_write_register(ViPipe, 0x391b, 0x83);
    sc2210_write_register(ViPipe, 0x391f, 0x00);
    sc2210_write_register(ViPipe, 0x3933, 0x28);
    sc2210_write_register(ViPipe, 0x3934, 0xa6);
    sc2210_write_register(ViPipe, 0x3940, 0x70);
    sc2210_write_register(ViPipe, 0x3942, 0x08);
    sc2210_write_register(ViPipe, 0x3943, 0xbc);
    sc2210_write_register(ViPipe, 0x3958, 0x02);
    sc2210_write_register(ViPipe, 0x3959, 0x04);
    sc2210_write_register(ViPipe, 0x3980, 0x61);
    sc2210_write_register(ViPipe, 0x3987, 0x0b);
    sc2210_write_register(ViPipe, 0x3990, 0x00);
    sc2210_write_register(ViPipe, 0x3991, 0x00);
    sc2210_write_register(ViPipe, 0x3992, 0x00);
    sc2210_write_register(ViPipe, 0x3993, 0x00);
    sc2210_write_register(ViPipe, 0x3994, 0x00);
    sc2210_write_register(ViPipe, 0x3995, 0x00);
    sc2210_write_register(ViPipe, 0x3996, 0x00);
    sc2210_write_register(ViPipe, 0x3997, 0x00);
    sc2210_write_register(ViPipe, 0x3998, 0x00);
    sc2210_write_register(ViPipe, 0x3999, 0x00);
    sc2210_write_register(ViPipe, 0x399a, 0x00);
    sc2210_write_register(ViPipe, 0x399b, 0x00);
    sc2210_write_register(ViPipe, 0x399c, 0x00);
    sc2210_write_register(ViPipe, 0x399d, 0x00);
    sc2210_write_register(ViPipe, 0x399e, 0x00);
    sc2210_write_register(ViPipe, 0x399f, 0x00);
    sc2210_write_register(ViPipe, 0x39a0, 0x00);
    sc2210_write_register(ViPipe, 0x39a1, 0x00);
    sc2210_write_register(ViPipe, 0x39a2, 0x03);
    sc2210_write_register(ViPipe, 0x39a3, 0x30);
    sc2210_write_register(ViPipe, 0x39a4, 0x03);
    sc2210_write_register(ViPipe, 0x39a5, 0x60);
    sc2210_write_register(ViPipe, 0x39a6, 0x03);
    sc2210_write_register(ViPipe, 0x39a7, 0xa0);
    sc2210_write_register(ViPipe, 0x39a8, 0x03);
    sc2210_write_register(ViPipe, 0x39a9, 0xb0);
    sc2210_write_register(ViPipe, 0x39aa, 0x00);
    sc2210_write_register(ViPipe, 0x39ab, 0x00);
    sc2210_write_register(ViPipe, 0x39ac, 0x00);
    sc2210_write_register(ViPipe, 0x39ad, 0x20);
    sc2210_write_register(ViPipe, 0x39ae, 0x00);
    sc2210_write_register(ViPipe, 0x39af, 0x40);
    sc2210_write_register(ViPipe, 0x39b0, 0x00);
    sc2210_write_register(ViPipe, 0x39b1, 0x60);
    sc2210_write_register(ViPipe, 0x39b2, 0x00);
    sc2210_write_register(ViPipe, 0x39b3, 0x00);
    sc2210_write_register(ViPipe, 0x39b4, 0x08);
    sc2210_write_register(ViPipe, 0x39b5, 0x14);
    sc2210_write_register(ViPipe, 0x39b6, 0x20);
    sc2210_write_register(ViPipe, 0x39b7, 0x38);
    sc2210_write_register(ViPipe, 0x39b8, 0x38);
    sc2210_write_register(ViPipe, 0x39b9, 0x20);
    sc2210_write_register(ViPipe, 0x39ba, 0x14);
    sc2210_write_register(ViPipe, 0x39bb, 0x08);
    sc2210_write_register(ViPipe, 0x39bc, 0x08);
    sc2210_write_register(ViPipe, 0x39bd, 0x10);
    sc2210_write_register(ViPipe, 0x39be, 0x20);
    sc2210_write_register(ViPipe, 0x39bf, 0x30);
    sc2210_write_register(ViPipe, 0x39c0, 0x30);
    sc2210_write_register(ViPipe, 0x39c1, 0x20);
    sc2210_write_register(ViPipe, 0x39c2, 0x10);
    sc2210_write_register(ViPipe, 0x39c3, 0x08);
    sc2210_write_register(ViPipe, 0x39c4, 0x00);
    sc2210_write_register(ViPipe, 0x39c5, 0x80);
    sc2210_write_register(ViPipe, 0x39c6, 0x00);
    sc2210_write_register(ViPipe, 0x39c7, 0x80);
    sc2210_write_register(ViPipe, 0x39c8, 0x00);
    sc2210_write_register(ViPipe, 0x39c9, 0x00);
    sc2210_write_register(ViPipe, 0x39ca, 0x80);
    sc2210_write_register(ViPipe, 0x39cb, 0x00);
    sc2210_write_register(ViPipe, 0x39cc, 0x00);
    sc2210_write_register(ViPipe, 0x39cd, 0x00);
    sc2210_write_register(ViPipe, 0x39ce, 0x00);
    sc2210_write_register(ViPipe, 0x39cf, 0x00);
    sc2210_write_register(ViPipe, 0x39d0, 0x00);
    sc2210_write_register(ViPipe, 0x39d1, 0x00);
    sc2210_write_register(ViPipe, 0x39e2, 0x05);
    sc2210_write_register(ViPipe, 0x39e3, 0xeb);
    sc2210_write_register(ViPipe, 0x39e4, 0x07);
    sc2210_write_register(ViPipe, 0x39e5, 0xb6);
    sc2210_write_register(ViPipe, 0x39e6, 0x00);
    sc2210_write_register(ViPipe, 0x39e7, 0x3a);
    sc2210_write_register(ViPipe, 0x39e8, 0x3f);
    sc2210_write_register(ViPipe, 0x39e9, 0xb7);
    sc2210_write_register(ViPipe, 0x39ea, 0x02);
    sc2210_write_register(ViPipe, 0x39eb, 0x4f);
    sc2210_write_register(ViPipe, 0x39ec, 0x08);
    sc2210_write_register(ViPipe, 0x39ed, 0x00);
    sc2210_write_register(ViPipe, 0x3e01, 0x46);
    sc2210_write_register(ViPipe, 0x3e02, 0x10);
    sc2210_write_register(ViPipe, 0x3e09, 0x40);
    sc2210_write_register(ViPipe, 0x3e14, 0x31);
    sc2210_write_register(ViPipe, 0x3e1b, 0x3a);
    sc2210_write_register(ViPipe, 0x3e26, 0x40);
    sc2210_write_register(ViPipe, 0x4401, 0x1a);
    sc2210_write_register(ViPipe, 0x4407, 0xc0);
    sc2210_write_register(ViPipe, 0x4418, 0x34);
    sc2210_write_register(ViPipe, 0x4500, 0x18);
    sc2210_write_register(ViPipe, 0x4501, 0xb4);
    sc2210_write_register(ViPipe, 0x4509, 0x20);
    sc2210_write_register(ViPipe, 0x4603, 0x00);
    sc2210_write_register(ViPipe, 0x4800, 0x04); // 24 mipi clk non continue; 04 continue
    sc2210_write_register(ViPipe, 0x4837, 0x24);
    sc2210_write_register(ViPipe, 0x5000, 0x0e);
    sc2210_write_register(ViPipe, 0x550f, 0x20);
    sc2210_write_register(ViPipe, 0x36e9, 0x21);
    sc2210_write_register(ViPipe, 0x36f9, 0x21);

    sc2210_write_register(ViPipe, 0x0100, 0x01);

    return ret;
}

int sc2210_linear_1080p30_init(VI_PIPE ViPipe)
{
    printf("===================================================================================\n");
    printf("=== SC2210_MIPI_27MInput_445.5Mbps_2lane_12bit_1920x1080_30fps                  ===\n");
    printf("===================================================================================\n");

    return SC_SUCCESS;
}

int sc2210_wdr_1080p30_2to1_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = sc2210_write_register(ViPipe, 0x0103, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write sc2210 register error!\n");
        return SC_FAILURE;
    }

    sc2210_write_register(ViPipe, 0x0100, 0x00);
    sc2210_write_register(ViPipe, 0x36e9, 0x80);
    sc2210_write_register(ViPipe, 0x36f9, 0x80);
    sc2210_write_register(ViPipe, 0x3001, 0x07);
    sc2210_write_register(ViPipe, 0x3002, 0xc0);
    sc2210_write_register(ViPipe, 0x300a, 0x2c);
    sc2210_write_register(ViPipe, 0x300f, 0x00);
    sc2210_write_register(ViPipe, 0x3018, 0x73);
    sc2210_write_register(ViPipe, 0x3019, 0x00);
    sc2210_write_register(ViPipe, 0x301f, 0x40);
    sc2210_write_register(ViPipe, 0x3031, 0x0a);
    sc2210_write_register(ViPipe, 0x3033, 0x20);
    sc2210_write_register(ViPipe, 0x3038, 0x22);
    sc2210_write_register(ViPipe, 0x3106, 0x01);
    sc2210_write_register(ViPipe, 0x3201, 0x04);
    sc2210_write_register(ViPipe, 0x3203, 0x04);
    sc2210_write_register(ViPipe, 0x3204, 0x07);
    sc2210_write_register(ViPipe, 0x3205, 0x8b);
    sc2210_write_register(ViPipe, 0x3206, 0x04);
    sc2210_write_register(ViPipe, 0x3207, 0x43);
    sc2210_write_register(ViPipe, 0x320c, 0x04);
    sc2210_write_register(ViPipe, 0x320d, 0x4c);
    sc2210_write_register(ViPipe, 0x320e, 0x09);
    sc2210_write_register(ViPipe, 0x320f, 0x60);
    sc2210_write_register(ViPipe, 0x3211, 0x04);
    sc2210_write_register(ViPipe, 0x3213, 0x04);
    sc2210_write_register(ViPipe, 0x3220, 0x53);
    sc2210_write_register(ViPipe, 0x3231, 0x02);
    sc2210_write_register(ViPipe, 0x3250, 0x3f);
    sc2210_write_register(ViPipe, 0x3253, 0x04);
    sc2210_write_register(ViPipe, 0x3301, 0x0c);
    sc2210_write_register(ViPipe, 0x3302, 0x10);
    sc2210_write_register(ViPipe, 0x3304, 0x58);
    sc2210_write_register(ViPipe, 0x3305, 0x00);
    sc2210_write_register(ViPipe, 0x3306, 0xb0);
    sc2210_write_register(ViPipe, 0x3308, 0x20);
    sc2210_write_register(ViPipe, 0x3309, 0x98);
    sc2210_write_register(ViPipe, 0x330a, 0x01);
    sc2210_write_register(ViPipe, 0x330b, 0x68);
    sc2210_write_register(ViPipe, 0x3000, 0xc0);
    sc2210_write_register(ViPipe, 0x330e, 0x48);
    sc2210_write_register(ViPipe, 0x3314, 0x92);
    sc2210_write_register(ViPipe, 0x331e, 0x49);
    sc2210_write_register(ViPipe, 0x331f, 0x89);
    sc2210_write_register(ViPipe, 0x334c, 0x10);
    sc2210_write_register(ViPipe, 0x335d, 0x60);
    sc2210_write_register(ViPipe, 0x335e, 0x02);
    sc2210_write_register(ViPipe, 0x335f, 0x06);
    sc2210_write_register(ViPipe, 0x3364, 0x16);
    sc2210_write_register(ViPipe, 0x3366, 0x92);
    sc2210_write_register(ViPipe, 0x3367, 0x10);
    sc2210_write_register(ViPipe, 0x3368, 0x04);
    sc2210_write_register(ViPipe, 0x3369, 0x00);
    sc2210_write_register(ViPipe, 0x336a, 0x00);
    sc2210_write_register(ViPipe, 0x336b, 0x00);
    sc2210_write_register(ViPipe, 0x336d, 0x03);
    sc2210_write_register(ViPipe, 0x337c, 0x08);
    sc2210_write_register(ViPipe, 0x337d, 0x0e);
    sc2210_write_register(ViPipe, 0x337f, 0x33);
    sc2210_write_register(ViPipe, 0x3390, 0x10);
    sc2210_write_register(ViPipe, 0x3391, 0x30);
    sc2210_write_register(ViPipe, 0x3392, 0x40);
    sc2210_write_register(ViPipe, 0x3393, 0x0c);
    sc2210_write_register(ViPipe, 0x3394, 0x0c);
    sc2210_write_register(ViPipe, 0x3395, 0x0c);
    sc2210_write_register(ViPipe, 0x3396, 0x0c);
    sc2210_write_register(ViPipe, 0x3397, 0x30);
    sc2210_write_register(ViPipe, 0x3398, 0x3f);
    sc2210_write_register(ViPipe, 0x3399, 0x30);
    sc2210_write_register(ViPipe, 0x339a, 0x30);
    sc2210_write_register(ViPipe, 0x339b, 0x30);
    sc2210_write_register(ViPipe, 0x339c, 0x30);
    sc2210_write_register(ViPipe, 0x33a2, 0x0a);
    sc2210_write_register(ViPipe, 0x33b9, 0x0e);
    sc2210_write_register(ViPipe, 0x33e1, 0x08);
    sc2210_write_register(ViPipe, 0x33e2, 0x18);
    sc2210_write_register(ViPipe, 0x33e3, 0x18);
    sc2210_write_register(ViPipe, 0x33e4, 0x18);
    sc2210_write_register(ViPipe, 0x33e5, 0x10);
    sc2210_write_register(ViPipe, 0x33e6, 0x06);
    sc2210_write_register(ViPipe, 0x33e7, 0x02);
    sc2210_write_register(ViPipe, 0x33e8, 0x18);
    sc2210_write_register(ViPipe, 0x33e9, 0x10);
    sc2210_write_register(ViPipe, 0x33ea, 0x0c);
    sc2210_write_register(ViPipe, 0x33eb, 0x10);
    sc2210_write_register(ViPipe, 0x33ec, 0x04);
    sc2210_write_register(ViPipe, 0x33ed, 0x02);
    sc2210_write_register(ViPipe, 0x33ee, 0xa0);
    sc2210_write_register(ViPipe, 0x33ef, 0x08);
    sc2210_write_register(ViPipe, 0x33f4, 0x18);
    sc2210_write_register(ViPipe, 0x33f5, 0x10);
    sc2210_write_register(ViPipe, 0x33f6, 0x0c);
    sc2210_write_register(ViPipe, 0x33f7, 0x10);
    sc2210_write_register(ViPipe, 0x33f8, 0x06);
    sc2210_write_register(ViPipe, 0x33f9, 0x02);
    sc2210_write_register(ViPipe, 0x33fa, 0x18);
    sc2210_write_register(ViPipe, 0x33fb, 0x10);
    sc2210_write_register(ViPipe, 0x33fc, 0x0c);
    sc2210_write_register(ViPipe, 0x33fd, 0x10);
    sc2210_write_register(ViPipe, 0x33fe, 0x04);
    sc2210_write_register(ViPipe, 0x33ff, 0x02);
    sc2210_write_register(ViPipe, 0x360f, 0x01);
    sc2210_write_register(ViPipe, 0x3622, 0xf7);
    sc2210_write_register(ViPipe, 0x3625, 0x0a);
    sc2210_write_register(ViPipe, 0x3627, 0x02);
    sc2210_write_register(ViPipe, 0x3630, 0xa2);
    sc2210_write_register(ViPipe, 0x3631, 0x00);
    sc2210_write_register(ViPipe, 0x3632, 0xd8);
    sc2210_write_register(ViPipe, 0x3633, 0x43);
    sc2210_write_register(ViPipe, 0x3635, 0x20);
    sc2210_write_register(ViPipe, 0x3638, 0x24);
    sc2210_write_register(ViPipe, 0x363a, 0x80);
    sc2210_write_register(ViPipe, 0x363b, 0x02);
    sc2210_write_register(ViPipe, 0x363e, 0x22);
    sc2210_write_register(ViPipe, 0x3670, 0x48);
    sc2210_write_register(ViPipe, 0x3671, 0xf7);
    sc2210_write_register(ViPipe, 0x3672, 0xf7);
    sc2210_write_register(ViPipe, 0x3673, 0x07);
    sc2210_write_register(ViPipe, 0x367a, 0x40);
    sc2210_write_register(ViPipe, 0x367b, 0x7f);
    sc2210_write_register(ViPipe, 0x3690, 0x42);
    sc2210_write_register(ViPipe, 0x3691, 0x43);
    sc2210_write_register(ViPipe, 0x3692, 0x54);
    sc2210_write_register(ViPipe, 0x369c, 0x40);
    sc2210_write_register(ViPipe, 0x369d, 0x7f);
    sc2210_write_register(ViPipe, 0x36b5, 0x40);
    sc2210_write_register(ViPipe, 0x36b6, 0x7f);
    sc2210_write_register(ViPipe, 0x36c0, 0x80);
    sc2210_write_register(ViPipe, 0x36c1, 0x9f);
    sc2210_write_register(ViPipe, 0x36c2, 0x9f);
    sc2210_write_register(ViPipe, 0x36cc, 0x20);
    sc2210_write_register(ViPipe, 0x36cd, 0x20);
    sc2210_write_register(ViPipe, 0x36ce, 0x30);
    sc2210_write_register(ViPipe, 0x36d0, 0x20);
    sc2210_write_register(ViPipe, 0x36d1, 0x40);
    sc2210_write_register(ViPipe, 0x36d2, 0x7f);
    sc2210_write_register(ViPipe, 0x36ea, 0x35);
    sc2210_write_register(ViPipe, 0x36eb, 0x07);
    sc2210_write_register(ViPipe, 0x36ec, 0x13);
    sc2210_write_register(ViPipe, 0x36ed, 0x04);
    sc2210_write_register(ViPipe, 0x36fa, 0xf5);
    sc2210_write_register(ViPipe, 0x36fb, 0x15);
    sc2210_write_register(ViPipe, 0x36fc, 0x10);
    sc2210_write_register(ViPipe, 0x36fd, 0x07);
    sc2210_write_register(ViPipe, 0x3905, 0xd8);
    sc2210_write_register(ViPipe, 0x3907, 0x01);
    sc2210_write_register(ViPipe, 0x3908, 0x11);
    sc2210_write_register(ViPipe, 0x391b, 0x83);
    sc2210_write_register(ViPipe, 0x391f, 0x00);
    sc2210_write_register(ViPipe, 0x3933, 0x28);
    sc2210_write_register(ViPipe, 0x3934, 0xa6);
    sc2210_write_register(ViPipe, 0x3940, 0x70);
    sc2210_write_register(ViPipe, 0x3942, 0x08);
    sc2210_write_register(ViPipe, 0x3943, 0xbc);
    sc2210_write_register(ViPipe, 0x3958, 0x02);
    sc2210_write_register(ViPipe, 0x3959, 0x04);
    sc2210_write_register(ViPipe, 0x3980, 0x61);
    sc2210_write_register(ViPipe, 0x3987, 0x0b);
    sc2210_write_register(ViPipe, 0x3990, 0x00);
    sc2210_write_register(ViPipe, 0x3991, 0x00);
    sc2210_write_register(ViPipe, 0x3992, 0x00);
    sc2210_write_register(ViPipe, 0x3993, 0x00);
    sc2210_write_register(ViPipe, 0x3994, 0x00);
    sc2210_write_register(ViPipe, 0x3995, 0x00);
    sc2210_write_register(ViPipe, 0x3996, 0x00);
    sc2210_write_register(ViPipe, 0x3997, 0x00);
    sc2210_write_register(ViPipe, 0x3998, 0x00);
    sc2210_write_register(ViPipe, 0x3999, 0x00);
    sc2210_write_register(ViPipe, 0x399a, 0x00);
    sc2210_write_register(ViPipe, 0x399b, 0x00);
    sc2210_write_register(ViPipe, 0x399c, 0x00);
    sc2210_write_register(ViPipe, 0x399d, 0x00);
    sc2210_write_register(ViPipe, 0x399e, 0x00);
    sc2210_write_register(ViPipe, 0x399f, 0x00);
    sc2210_write_register(ViPipe, 0x39a0, 0x00);
    sc2210_write_register(ViPipe, 0x39a1, 0x00);
    sc2210_write_register(ViPipe, 0x39a2, 0x03);
    sc2210_write_register(ViPipe, 0x39a3, 0x30);
    sc2210_write_register(ViPipe, 0x39a4, 0x03);
    sc2210_write_register(ViPipe, 0x39a5, 0x60);
    sc2210_write_register(ViPipe, 0x39a6, 0x03);
    sc2210_write_register(ViPipe, 0x39a7, 0xa0);
    sc2210_write_register(ViPipe, 0x39a8, 0x03);
    sc2210_write_register(ViPipe, 0x39a9, 0xb0);
    sc2210_write_register(ViPipe, 0x39aa, 0x00);
    sc2210_write_register(ViPipe, 0x39ab, 0x00);
    sc2210_write_register(ViPipe, 0x39ac, 0x00);
    sc2210_write_register(ViPipe, 0x39ad, 0x20);
    sc2210_write_register(ViPipe, 0x39ae, 0x00);
    sc2210_write_register(ViPipe, 0x39af, 0x40);
    sc2210_write_register(ViPipe, 0x39b0, 0x00);
    sc2210_write_register(ViPipe, 0x39b1, 0x60);
    sc2210_write_register(ViPipe, 0x39b2, 0x00);
    sc2210_write_register(ViPipe, 0x39b3, 0x00);
    sc2210_write_register(ViPipe, 0x39b4, 0x08);
    sc2210_write_register(ViPipe, 0x39b5, 0x14);
    sc2210_write_register(ViPipe, 0x39b6, 0x20);
    sc2210_write_register(ViPipe, 0x39b7, 0x38);
    sc2210_write_register(ViPipe, 0x39b8, 0x38);
    sc2210_write_register(ViPipe, 0x39b9, 0x20);
    sc2210_write_register(ViPipe, 0x39ba, 0x14);
    sc2210_write_register(ViPipe, 0x39bb, 0x08);
    sc2210_write_register(ViPipe, 0x39bc, 0x08);
    sc2210_write_register(ViPipe, 0x39bd, 0x10);
    sc2210_write_register(ViPipe, 0x39be, 0x20);
    sc2210_write_register(ViPipe, 0x39bf, 0x30);
    sc2210_write_register(ViPipe, 0x39c0, 0x30);
    sc2210_write_register(ViPipe, 0x39c1, 0x20);
    sc2210_write_register(ViPipe, 0x39c2, 0x10);
    sc2210_write_register(ViPipe, 0x39c3, 0x08);
    sc2210_write_register(ViPipe, 0x39c4, 0x00);
    sc2210_write_register(ViPipe, 0x39c5, 0x80);
    sc2210_write_register(ViPipe, 0x39c6, 0x00);
    sc2210_write_register(ViPipe, 0x39c7, 0x80);
    sc2210_write_register(ViPipe, 0x39c8, 0x00);
    sc2210_write_register(ViPipe, 0x39c9, 0x00);
    sc2210_write_register(ViPipe, 0x39ca, 0x80);
    sc2210_write_register(ViPipe, 0x39cb, 0x00);
    sc2210_write_register(ViPipe, 0x39cc, 0x00);
    sc2210_write_register(ViPipe, 0x39cd, 0x00);
    sc2210_write_register(ViPipe, 0x39ce, 0x00);
    sc2210_write_register(ViPipe, 0x39cf, 0x00);
    sc2210_write_register(ViPipe, 0x39d0, 0x00);
    sc2210_write_register(ViPipe, 0x39d1, 0x00);
    sc2210_write_register(ViPipe, 0x39e2, 0x05);
    sc2210_write_register(ViPipe, 0x39e3, 0xeb);
    sc2210_write_register(ViPipe, 0x39e4, 0x07);
    sc2210_write_register(ViPipe, 0x39e5, 0xb6);
    sc2210_write_register(ViPipe, 0x39e6, 0x00);
    sc2210_write_register(ViPipe, 0x39e7, 0x3a);
    sc2210_write_register(ViPipe, 0x39e8, 0x3f);
    sc2210_write_register(ViPipe, 0x39e9, 0xb7);
    sc2210_write_register(ViPipe, 0x39ea, 0x02);
    sc2210_write_register(ViPipe, 0x39eb, 0x4f);
    sc2210_write_register(ViPipe, 0x39ec, 0x08);
    sc2210_write_register(ViPipe, 0x39ed, 0x00);
    sc2210_write_register(ViPipe, 0x3e00, 0x00);
    sc2210_write_register(ViPipe, 0x3e01, 0x8c);
    sc2210_write_register(ViPipe, 0x3e02, 0x00);
    sc2210_write_register(ViPipe, 0x3e03, 0x0b);
    sc2210_write_register(ViPipe, 0x3e04, 0x08);
    sc2210_write_register(ViPipe, 0x3e05, 0xc0);
    sc2210_write_register(ViPipe, 0x3e06, 0x00);
    sc2210_write_register(ViPipe, 0x3e07, 0x80);
    sc2210_write_register(ViPipe, 0x3e08, 0x03);
    sc2210_write_register(ViPipe, 0x3e09, 0x40);
    sc2210_write_register(ViPipe, 0x3e10, 0x00);
    sc2210_write_register(ViPipe, 0x3e11, 0x80);
    sc2210_write_register(ViPipe, 0x3e12, 0x03);
    sc2210_write_register(ViPipe, 0x3e13, 0x40);
    sc2210_write_register(ViPipe, 0x3e14, 0x31);
    sc2210_write_register(ViPipe, 0x3e1b, 0x3a);
    sc2210_write_register(ViPipe, 0x3e22, 0x00);
    sc2210_write_register(ViPipe, 0x3e23, 0x00);
    sc2210_write_register(ViPipe, 0x3e24, 0x92);
    sc2210_write_register(ViPipe, 0x3e26, 0x40);
    sc2210_write_register(ViPipe, 0x3f08, 0x08);
    sc2210_write_register(ViPipe, 0x4401, 0x1a);
    sc2210_write_register(ViPipe, 0x4407, 0xc0);
    sc2210_write_register(ViPipe, 0x4418, 0x34);
    sc2210_write_register(ViPipe, 0x4500, 0x18);
    sc2210_write_register(ViPipe, 0x4501, 0xb4);
    sc2210_write_register(ViPipe, 0x4503, 0xc0);
    sc2210_write_register(ViPipe, 0x4509, 0x20);
    sc2210_write_register(ViPipe, 0x4603, 0x00);
    sc2210_write_register(ViPipe, 0x4800, 0x24);
    sc2210_write_register(ViPipe, 0x4814, 0x2a);
    sc2210_write_register(ViPipe, 0x4837, 0x28);
    sc2210_write_register(ViPipe, 0x4851, 0xab);
    sc2210_write_register(ViPipe, 0x4853, 0xf8);
    sc2210_write_register(ViPipe, 0x5000, 0x0e);
    sc2210_write_register(ViPipe, 0x550f, 0x20);
    sc2210_write_register(ViPipe, 0x36e9, 0x27);
    sc2210_write_register(ViPipe, 0x36f9, 0x33);
    sc2210_write_register(ViPipe, 0x0100, 0x01);

    /* long expo out */
    sc2210_write_register(ViPipe, 0x4814, 0x2a); //[7:6] vc long 0
    sc2210_write_register(ViPipe, 0x4851, 0xab); //[7:6] vc short 2

    /* short expo out */
    sc2210_write_register(ViPipe, 0x4814, 0x6a); //[7:6] vc long 1
    sc2210_write_register(ViPipe, 0x4851, 0x2b); //[7:6] vc short 0

    sc2210_write_register(ViPipe, 0x3e08, 0x03);
    sc2210_write_register(ViPipe, 0x3e09, 0x40);

    sc2210_write_register(ViPipe, 0x3e08, 0x03);
    sc2210_write_register(ViPipe, 0x3e09, 0x7f);

    sc2210_write_register(ViPipe, 0x3e08, 0x07);
    sc2210_write_register(ViPipe, 0x3e09, 0x40);

    sc2210_write_register(ViPipe, 0x3e08, 0x07);
    sc2210_write_register(ViPipe, 0x3e09, 0x6c);

    sc2210_write_register(ViPipe, 0x3e08, 0x23);
    sc2210_write_register(ViPipe, 0x3e09, 0x40);

    sc2210_write_register(ViPipe, 0x3e08, 0x23);
    sc2210_write_register(ViPipe, 0x3e09, 0x7f);

    sc2210_write_register(ViPipe, 0x3e08, 0x27);
    sc2210_write_register(ViPipe, 0x3e09, 0x40);

    sc2210_write_register(ViPipe, 0x3e08, 0x27);
    sc2210_write_register(ViPipe, 0x3e09, 0x7f);

    sc2210_write_register(ViPipe, 0x3e08, 0x2f);
    sc2210_write_register(ViPipe, 0x3e09, 0x40);

    sc2210_write_register(ViPipe, 0x3e08, 0x2f);
    sc2210_write_register(ViPipe, 0x3e09, 0x7f);

    sc2210_write_register(ViPipe, 0x3e08, 0x3f);
    sc2210_write_register(ViPipe, 0x3e09, 0x40);

    sc2210_write_register(ViPipe, 0x3e08, 0x3f);
    sc2210_write_register(ViPipe, 0x3e09, 0x7f);

    printf("=========================================================================\n");
    printf("===  SC2210_MIPI_24MInput_396Mbps_4lane_10bit_1920x1080_HDR 30fps(VC)  ==\n");
    printf("=========================================================================\n");

    return SC_SUCCESS;
}
