/**
 * @file     sc4210_sensor_ctl.c
 * @brief    SMART SEMS SC4210 SENSOR读写和初始化的接口实现
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2022-11-15 创建文件
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


#define SC4210_FLIP_MIRROR (0x3221)

#define SENSOR_4MP_30FPS_LINEAR_MODE  (1)
#define SENSOR_4MP_30FPS_2T1_WDR_MODE (2)

const unsigned char sc4210_i2c_addr  = 0x60; /* I2C Address of sc4210 */
const unsigned int  sc4210_addr_byte = 2;
const unsigned int  sc4210_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

extern ISP_SNS_STATE_S   *g_pastSc4210[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U  g_aunSc4210BusInfo[];

int sc4210_linear_1440H30_init(VI_PIPE ViPipe);
int sc4210_wdr_1440H30_2to1_init(VI_PIPE ViPipe);



int sc4210_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {
        return SC_SUCCESS;
    }

    u8DevNum = g_aunSc4210BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (sc4210_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int sc4210_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int sc4210_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:
    return SC_SUCCESS;
}

int sc4210_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(sc4210_addr_byte == 2)
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

    if(sc4210_data_byte == 2)
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

    ret = write(g_fd[ViPipe], buf, sc4210_addr_byte + sc4210_data_byte);
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

void sc4210_prog(VI_PIPE ViPipe, int *rom)
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
            sc4210_write_register(ViPipe, addr, data);
        }
    }
}

void sc4210_standby(VI_PIPE ViPipe)
{
    return;
}

void sc4210_restart(VI_PIPE ViPipe)
{
    return;
}

int sc4210_flip_off_mirror_off(VI_PIPE ViPipe)
{
    sc4210_write_register(ViPipe, SC4210_FLIP_MIRROR, 0x00);

    return SC_SUCCESS;
}

int sc4210_flip_on_mirror_off(VI_PIPE ViPipe)
{
    sc4210_write_register(ViPipe, SC4210_FLIP_MIRROR, 0x60);

    return SC_SUCCESS;
}

int sc4210_flip_off_mirror_on(VI_PIPE ViPipe)
{
    sc4210_write_register(ViPipe, SC4210_FLIP_MIRROR, 0x06);

    return SC_SUCCESS;
}

int sc4210_flip_on_mirror_on(VI_PIPE ViPipe)
{
    sc4210_write_register(ViPipe, SC4210_FLIP_MIRROR, 0x66);

    return SC_SUCCESS;
}

void sc4210_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i = 0;

    for(i = 0; i < g_pastSc4210[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastSc4210[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            sc4210_write_register(ViPipe,
                g_pastSc4210[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                g_pastSc4210[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int sc4210_init(VI_PIPE ViPipe)
{
    WDR_MODE_E enWDRMode = WDR_MODE_NONE;
    SC_BOOL    bInit     = SC_FALSE;
    SC_U8      u8ImgMode = 0;
    SC_S32     ret       = SC_FAILURE;

    bInit     = g_pastSc4210[ViPipe]->bInit;
    enWDRMode = g_pastSc4210[ViPipe]->enWDRMode;
    u8ImgMode = g_pastSc4210[ViPipe]->u8ImgMode;
    printf("SC4210: bInit[%d] enWDRMode[%d] u8ImgMode[%u]\n", bInit, enWDRMode, (SC_U32)u8ImgMode);

    ret = sc4210_i2c_init(ViPipe);
    if(SC_SUCCESS != ret)
    {
        printf("Init sc4210 i2c error!\n");
        return SC_FAILURE;
    }

    /* When sensor first init, config all registers */
    if(SC_FALSE == bInit)
    {
        if(WDR_MODE_2To1_LINE == enWDRMode)
        {
            if(SENSOR_4MP_30FPS_2T1_WDR_MODE == u8ImgMode)
            {
                ret = sc4210_wdr_1440H30_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init sc4210 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = sc4210_linear_1440H30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init sc4210 linear mode error!\n");
                return SC_FAILURE;
            }
        }
    }
    /* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
    else
    {
        if(WDR_MODE_2To1_LINE == enWDRMode)
        {
            if(SENSOR_4MP_30FPS_2T1_WDR_MODE == u8ImgMode)
            {
                ret = sc4210_wdr_1440H30_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init sc4210 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = sc4210_linear_1440H30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init sc4210 linear mode error!\n");
                return SC_FAILURE;
            }
        }
    }

    g_pastSc4210[ViPipe]->bInit = SC_TRUE;
    return SC_SUCCESS;
}

void sc4210_exit(VI_PIPE ViPipe)
{
    sc4210_i2c_exit(ViPipe);
    return;
}

int sc4210_linear_1440H30_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = sc4210_write_register(ViPipe, 0x0103, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write sc4210 register error!\n");
        return SC_FAILURE;
    }

    sc4210_write_register(ViPipe, 0x0100, 0x00);
    sc4210_write_register(ViPipe, 0x36e9, 0x80);
    sc4210_write_register(ViPipe, 0x36f9, 0x80);
    sc4210_write_register(ViPipe, 0x3001, 0x07);
    sc4210_write_register(ViPipe, 0x3002, 0xc0);
    sc4210_write_register(ViPipe, 0x300a, 0x2c);
    sc4210_write_register(ViPipe, 0x300f, 0x00);
    sc4210_write_register(ViPipe, 0x3018, 0x33);
    sc4210_write_register(ViPipe, 0x3019, 0x0c);
    sc4210_write_register(ViPipe, 0x301f, 0x07);
    sc4210_write_register(ViPipe, 0x3031, 0x0c);
    sc4210_write_register(ViPipe, 0x3038, 0x22);
    sc4210_write_register(ViPipe, 0x320c, 0x05);
    sc4210_write_register(ViPipe, 0x320d, 0x50);
    sc4210_write_register(ViPipe, 0x3220, 0x10);
    sc4210_write_register(ViPipe, 0x3225, 0x01);
    sc4210_write_register(ViPipe, 0x3227, 0x03);
    sc4210_write_register(ViPipe, 0x3229, 0x08);
    sc4210_write_register(ViPipe, 0x3231, 0x01);
    sc4210_write_register(ViPipe, 0x3241, 0x02);
    sc4210_write_register(ViPipe, 0x3243, 0x03);
    sc4210_write_register(ViPipe, 0x3249, 0x17);
    sc4210_write_register(ViPipe, 0x3251, 0x08);
    sc4210_write_register(ViPipe, 0x3253, 0x08);
    sc4210_write_register(ViPipe, 0x325e, 0x00);
    sc4210_write_register(ViPipe, 0x325f, 0x00);
    sc4210_write_register(ViPipe, 0x3273, 0x01);
    sc4210_write_register(ViPipe, 0x3301, 0x0c);
    sc4210_write_register(ViPipe, 0x3302, 0x18);
    sc4210_write_register(ViPipe, 0x3304, 0x20);
    sc4210_write_register(ViPipe, 0x3000, 0x00);
    sc4210_write_register(ViPipe, 0x3305, 0x00);
    sc4210_write_register(ViPipe, 0x3306, 0x74);
    sc4210_write_register(ViPipe, 0x3308, 0x10);
    sc4210_write_register(ViPipe, 0x3309, 0x40);
    sc4210_write_register(ViPipe, 0x330a, 0x00);
    sc4210_write_register(ViPipe, 0x330b, 0xe8);
    sc4210_write_register(ViPipe, 0x330e, 0x18);
    sc4210_write_register(ViPipe, 0x3312, 0x02);
    sc4210_write_register(ViPipe, 0x3314, 0x84);
    sc4210_write_register(ViPipe, 0x331e, 0x19);
    sc4210_write_register(ViPipe, 0x331f, 0x39);
    sc4210_write_register(ViPipe, 0x3320, 0x05);
    sc4210_write_register(ViPipe, 0x3338, 0x10);
    sc4210_write_register(ViPipe, 0x334c, 0x10);
    sc4210_write_register(ViPipe, 0x335d, 0x20);
    sc4210_write_register(ViPipe, 0x335e, 0x02);
    sc4210_write_register(ViPipe, 0x335f, 0x04);
    sc4210_write_register(ViPipe, 0x3364, 0x1e);
    sc4210_write_register(ViPipe, 0x3366, 0x92);
    sc4210_write_register(ViPipe, 0x3367, 0x08);
    sc4210_write_register(ViPipe, 0x3368, 0x05);
    sc4210_write_register(ViPipe, 0x3369, 0xdc);
    sc4210_write_register(ViPipe, 0x336a, 0x0b);
    sc4210_write_register(ViPipe, 0x336b, 0xb8);
    sc4210_write_register(ViPipe, 0x336c, 0xc2);
    sc4210_write_register(ViPipe, 0x337a, 0x08);
    sc4210_write_register(ViPipe, 0x337b, 0x10);
    sc4210_write_register(ViPipe, 0x337c, 0x06);
    sc4210_write_register(ViPipe, 0x337d, 0x0a);
    sc4210_write_register(ViPipe, 0x337e, 0x40);
    sc4210_write_register(ViPipe, 0x3390, 0x08);
    sc4210_write_register(ViPipe, 0x3391, 0x18);
    sc4210_write_register(ViPipe, 0x3392, 0x38);
    sc4210_write_register(ViPipe, 0x3393, 0x0c);
    sc4210_write_register(ViPipe, 0x3394, 0x28);
    sc4210_write_register(ViPipe, 0x3395, 0x28);
    sc4210_write_register(ViPipe, 0x3396, 0x08);
    sc4210_write_register(ViPipe, 0x3397, 0x18);
    sc4210_write_register(ViPipe, 0x3398, 0x38);
    sc4210_write_register(ViPipe, 0x3399, 0x0a);
    sc4210_write_register(ViPipe, 0x339a, 0x18);
    sc4210_write_register(ViPipe, 0x339b, 0x28);
    sc4210_write_register(ViPipe, 0x339c, 0x28);
    sc4210_write_register(ViPipe, 0x33a2, 0x08);
    sc4210_write_register(ViPipe, 0x33a3, 0x0c);
    sc4210_write_register(ViPipe, 0x33e0, 0xa0);
    sc4210_write_register(ViPipe, 0x33e1, 0x08);
    sc4210_write_register(ViPipe, 0x33e2, 0x00);
    sc4210_write_register(ViPipe, 0x33e3, 0x10);
    sc4210_write_register(ViPipe, 0x33e4, 0x10);
    sc4210_write_register(ViPipe, 0x33e5, 0x00);
    sc4210_write_register(ViPipe, 0x33e6, 0x10);
    sc4210_write_register(ViPipe, 0x33e7, 0x10);
    sc4210_write_register(ViPipe, 0x33e8, 0x00);
    sc4210_write_register(ViPipe, 0x33e9, 0x10);
    sc4210_write_register(ViPipe, 0x33ea, 0x16);
    sc4210_write_register(ViPipe, 0x33eb, 0x00);
    sc4210_write_register(ViPipe, 0x33ec, 0x10);
    sc4210_write_register(ViPipe, 0x33ed, 0x18);
    sc4210_write_register(ViPipe, 0x33ee, 0xa0);
    sc4210_write_register(ViPipe, 0x33ef, 0x08);
    sc4210_write_register(ViPipe, 0x33f4, 0x00);
    sc4210_write_register(ViPipe, 0x33f5, 0x10);
    sc4210_write_register(ViPipe, 0x33f6, 0x10);
    sc4210_write_register(ViPipe, 0x33f7, 0x00);
    sc4210_write_register(ViPipe, 0x33f8, 0x10);
    sc4210_write_register(ViPipe, 0x33f9, 0x10);
    sc4210_write_register(ViPipe, 0x33fa, 0x00);
    sc4210_write_register(ViPipe, 0x33fb, 0x10);
    sc4210_write_register(ViPipe, 0x33fc, 0x16);
    sc4210_write_register(ViPipe, 0x33fd, 0x00);
    sc4210_write_register(ViPipe, 0x33fe, 0x10);
    sc4210_write_register(ViPipe, 0x33ff, 0x18);
    sc4210_write_register(ViPipe, 0x360f, 0x05);
    sc4210_write_register(ViPipe, 0x3622, 0xff);
    sc4210_write_register(ViPipe, 0x3624, 0x07);
    sc4210_write_register(ViPipe, 0x3625, 0x0a);
    sc4210_write_register(ViPipe, 0x3630, 0xc4);
    sc4210_write_register(ViPipe, 0x3631, 0x80);
    sc4210_write_register(ViPipe, 0x3632, 0x88);
    sc4210_write_register(ViPipe, 0x3633, 0x22);
    sc4210_write_register(ViPipe, 0x3634, 0x64);
    sc4210_write_register(ViPipe, 0x3635, 0x20);
    sc4210_write_register(ViPipe, 0x3636, 0x20);
    sc4210_write_register(ViPipe, 0x3638, 0x28);
    sc4210_write_register(ViPipe, 0x363b, 0x03);
    sc4210_write_register(ViPipe, 0x363c, 0x06);
    sc4210_write_register(ViPipe, 0x363d, 0x06);
    sc4210_write_register(ViPipe, 0x366e, 0x04);
    sc4210_write_register(ViPipe, 0x3670, 0x48);
    sc4210_write_register(ViPipe, 0x3671, 0xff);
    sc4210_write_register(ViPipe, 0x3672, 0x1f);
    sc4210_write_register(ViPipe, 0x3673, 0x1f);
    sc4210_write_register(ViPipe, 0x367a, 0x40);
    sc4210_write_register(ViPipe, 0x367b, 0x40);
    sc4210_write_register(ViPipe, 0x3690, 0x42);
    sc4210_write_register(ViPipe, 0x3691, 0x44);
    sc4210_write_register(ViPipe, 0x3692, 0x44);
    sc4210_write_register(ViPipe, 0x3699, 0x80);
    sc4210_write_register(ViPipe, 0x369a, 0x9f);
    sc4210_write_register(ViPipe, 0x369b, 0x9f);
    sc4210_write_register(ViPipe, 0x369c, 0x40);
    sc4210_write_register(ViPipe, 0x369d, 0x40);
    sc4210_write_register(ViPipe, 0x36a2, 0x40);
    sc4210_write_register(ViPipe, 0x36a3, 0x40);
    sc4210_write_register(ViPipe, 0x36cc, 0x2c);
    sc4210_write_register(ViPipe, 0x36cd, 0x30);
    sc4210_write_register(ViPipe, 0x36ce, 0x30);
    sc4210_write_register(ViPipe, 0x36d0, 0x20);
    sc4210_write_register(ViPipe, 0x36d1, 0x40);
    sc4210_write_register(ViPipe, 0x36d2, 0x40);
    sc4210_write_register(ViPipe, 0x36ea, 0x3a);
    sc4210_write_register(ViPipe, 0x36eb, 0x06);
    sc4210_write_register(ViPipe, 0x36ec, 0x03);
    sc4210_write_register(ViPipe, 0x36ed, 0x34);
    sc4210_write_register(ViPipe, 0x36fa, 0x2f);
    sc4210_write_register(ViPipe, 0x36fb, 0x14);
    sc4210_write_register(ViPipe, 0x36fc, 0x00);
    sc4210_write_register(ViPipe, 0x36fd, 0x04);
    sc4210_write_register(ViPipe, 0x3817, 0x20);
    sc4210_write_register(ViPipe, 0x3905, 0xd8);
    sc4210_write_register(ViPipe, 0x3908, 0x11);
    sc4210_write_register(ViPipe, 0x391b, 0x80);
    sc4210_write_register(ViPipe, 0x391c, 0x0f);
    sc4210_write_register(ViPipe, 0x391d, 0x01);
    sc4210_write_register(ViPipe, 0x3933, 0x24);
    sc4210_write_register(ViPipe, 0x3934, 0xb0);
    sc4210_write_register(ViPipe, 0x3935, 0x80);
    sc4210_write_register(ViPipe, 0x3936, 0x1f);
    sc4210_write_register(ViPipe, 0x3940, 0x68);
    sc4210_write_register(ViPipe, 0x3942, 0x04);
    sc4210_write_register(ViPipe, 0x3943, 0xc0);
    sc4210_write_register(ViPipe, 0x3980, 0x00);
    sc4210_write_register(ViPipe, 0x3981, 0x50);
    sc4210_write_register(ViPipe, 0x3982, 0x00);
    sc4210_write_register(ViPipe, 0x3983, 0x40);
    sc4210_write_register(ViPipe, 0x3984, 0x00);
    sc4210_write_register(ViPipe, 0x3985, 0x20);
    sc4210_write_register(ViPipe, 0x3986, 0x00);
    sc4210_write_register(ViPipe, 0x3987, 0x10);
    sc4210_write_register(ViPipe, 0x3988, 0x00);
    sc4210_write_register(ViPipe, 0x3989, 0x20);
    sc4210_write_register(ViPipe, 0x398a, 0x00);
    sc4210_write_register(ViPipe, 0x398b, 0x30);
    sc4210_write_register(ViPipe, 0x398c, 0x00);
    sc4210_write_register(ViPipe, 0x398d, 0x50);
    sc4210_write_register(ViPipe, 0x398e, 0x00);
    sc4210_write_register(ViPipe, 0x398f, 0x60);
    sc4210_write_register(ViPipe, 0x3990, 0x00);
    sc4210_write_register(ViPipe, 0x3991, 0x70);
    sc4210_write_register(ViPipe, 0x3992, 0x00);
    sc4210_write_register(ViPipe, 0x3993, 0x36);
    sc4210_write_register(ViPipe, 0x3994, 0x00);
    sc4210_write_register(ViPipe, 0x3995, 0x20);
    sc4210_write_register(ViPipe, 0x3996, 0x00);
    sc4210_write_register(ViPipe, 0x3997, 0x14);
    sc4210_write_register(ViPipe, 0x3998, 0x00);
    sc4210_write_register(ViPipe, 0x3999, 0x20);
    sc4210_write_register(ViPipe, 0x399a, 0x00);
    sc4210_write_register(ViPipe, 0x399b, 0x50);
    sc4210_write_register(ViPipe, 0x399c, 0x00);
    sc4210_write_register(ViPipe, 0x399d, 0x90);
    sc4210_write_register(ViPipe, 0x399e, 0x00);
    sc4210_write_register(ViPipe, 0x399f, 0xf0);
    sc4210_write_register(ViPipe, 0x39a0, 0x08);
    sc4210_write_register(ViPipe, 0x39a1, 0x10);
    sc4210_write_register(ViPipe, 0x39a2, 0x20);
    sc4210_write_register(ViPipe, 0x39a3, 0x40);
    sc4210_write_register(ViPipe, 0x39a4, 0x20);
    sc4210_write_register(ViPipe, 0x39a5, 0x10);
    sc4210_write_register(ViPipe, 0x39a6, 0x08);
    sc4210_write_register(ViPipe, 0x39a7, 0x04);
    sc4210_write_register(ViPipe, 0x39a8, 0x18);
    sc4210_write_register(ViPipe, 0x39a9, 0x30);
    sc4210_write_register(ViPipe, 0x39aa, 0x40);
    sc4210_write_register(ViPipe, 0x39ab, 0x60);
    sc4210_write_register(ViPipe, 0x39ac, 0x38);
    sc4210_write_register(ViPipe, 0x39ad, 0x20);
    sc4210_write_register(ViPipe, 0x39ae, 0x10);
    sc4210_write_register(ViPipe, 0x39af, 0x08);
    sc4210_write_register(ViPipe, 0x39b9, 0x00);
    sc4210_write_register(ViPipe, 0x39ba, 0xa0);
    sc4210_write_register(ViPipe, 0x39bb, 0x80);
    sc4210_write_register(ViPipe, 0x39bc, 0x00);
    sc4210_write_register(ViPipe, 0x39bd, 0x44);
    sc4210_write_register(ViPipe, 0x39be, 0x00);
    sc4210_write_register(ViPipe, 0x39bf, 0x00);
    sc4210_write_register(ViPipe, 0x39c0, 0x00);
    sc4210_write_register(ViPipe, 0x39c5, 0x41);
    sc4210_write_register(ViPipe, 0x3e00, 0x00);
    sc4210_write_register(ViPipe, 0x3e01, 0xbb);
    sc4210_write_register(ViPipe, 0x3e02, 0x40);
    sc4210_write_register(ViPipe, 0x3e03, 0x0b);
    sc4210_write_register(ViPipe, 0x3e06, 0x00);
    sc4210_write_register(ViPipe, 0x3e07, 0x80);
    sc4210_write_register(ViPipe, 0x3e08, 0x03);
    sc4210_write_register(ViPipe, 0x3e09, 0x40);
    sc4210_write_register(ViPipe, 0x3e0e, 0x6a);
    sc4210_write_register(ViPipe, 0x3e26, 0x40);
    sc4210_write_register(ViPipe, 0x4401, 0x0b);
    sc4210_write_register(ViPipe, 0x4407, 0xb0);
    sc4210_write_register(ViPipe, 0x4418, 0x0b);
    sc4210_write_register(ViPipe, 0x4501, 0xb4);
    sc4210_write_register(ViPipe, 0x4509, 0x10);
    sc4210_write_register(ViPipe, 0x4603, 0x00);
    sc4210_write_register(ViPipe, 0x4800, 0x24);
    sc4210_write_register(ViPipe, 0x4837, 0x13);
    sc4210_write_register(ViPipe, 0x5000, 0x0e);
    sc4210_write_register(ViPipe, 0x550f, 0x20);
    sc4210_write_register(ViPipe, 0x5784, 0x10);
    sc4210_write_register(ViPipe, 0x5785, 0x08);
    sc4210_write_register(ViPipe, 0x5787, 0x06);
    sc4210_write_register(ViPipe, 0x5788, 0x06);
    sc4210_write_register(ViPipe, 0x5789, 0x00);
    sc4210_write_register(ViPipe, 0x578a, 0x06);
    sc4210_write_register(ViPipe, 0x578b, 0x06);
    sc4210_write_register(ViPipe, 0x578c, 0x00);
    sc4210_write_register(ViPipe, 0x5790, 0x10);
    sc4210_write_register(ViPipe, 0x5791, 0x10);
    sc4210_write_register(ViPipe, 0x5792, 0x00);
    sc4210_write_register(ViPipe, 0x5793, 0x10);
    sc4210_write_register(ViPipe, 0x5794, 0x10);
    sc4210_write_register(ViPipe, 0x5795, 0x00);
    sc4210_write_register(ViPipe, 0x57c4, 0x10);
    sc4210_write_register(ViPipe, 0x57c5, 0x08);
    sc4210_write_register(ViPipe, 0x57c7, 0x06);
    sc4210_write_register(ViPipe, 0x57c8, 0x06);
    sc4210_write_register(ViPipe, 0x57c9, 0x00);
    sc4210_write_register(ViPipe, 0x57ca, 0x06);
    sc4210_write_register(ViPipe, 0x57cb, 0x06);
    sc4210_write_register(ViPipe, 0x57cc, 0x00);
    sc4210_write_register(ViPipe, 0x57d0, 0x10);
    sc4210_write_register(ViPipe, 0x57d1, 0x10);
    sc4210_write_register(ViPipe, 0x57d2, 0x00);
    sc4210_write_register(ViPipe, 0x57d3, 0x10);
    sc4210_write_register(ViPipe, 0x57d4, 0x10);
    sc4210_write_register(ViPipe, 0x57d5, 0x00);
    sc4210_write_register(ViPipe, 0x36e9, 0x24);
    sc4210_write_register(ViPipe, 0x36f9, 0x23);
    sc4210_write_register(ViPipe, 0x0100, 0x01);

    printf("===================================================================================\n");
    printf("=== SC4210_MIPI_24MInput_864Mbps_2lane_12bit_2560x1440_30fps                    ===\n");
    printf("===================================================================================\n");

    return SC_SUCCESS;
}

int sc4210_wdr_1440H30_2to1_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = sc4210_write_register(ViPipe, 0x0103, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write sc4210 register error!\n");
        return SC_FAILURE;
    }

    sc4210_write_register(ViPipe, 0x3000, 0x0f);
    sc4210_write_register(ViPipe, 0x0100, 0x00);
    sc4210_write_register(ViPipe, 0x36e9, 0xa7); //pll1 bypass
    sc4210_write_register(ViPipe, 0x36f9, 0xa0); //pll2 bypass
    sc4210_write_register(ViPipe, 0x3018, 0x6f);
    sc4210_write_register(ViPipe, 0x3019, 0xff);
    sc4210_write_register(ViPipe, 0x301c, 0xb4);
    sc4210_write_register(ViPipe, 0x300f, 0xff);
    sc4210_write_register(ViPipe, 0x3001, 0xff);
    //sc4210_write_register(ViPipe, 0x3000, 0x0f);
    sc4210_write_register(ViPipe, 0x300f, 0x00);
    sc4210_write_register(ViPipe, 0x3002, 0xff);
    sc4210_write_register(ViPipe, 0x300a, 0x2c);
    sc4210_write_register(ViPipe, 0x301c, 0x94);
    sc4210_write_register(ViPipe, 0x3018, 0x2f);
    sc4210_write_register(ViPipe, 0x301a, 0xf8);
    sc4210_write_register(ViPipe, 0x3c00, 0x45);
    sc4210_write_register(ViPipe, 0x3030, 0x02);
    sc4210_write_register(ViPipe, 0x303f, 0x81);
    sc4210_write_register(ViPipe, 0x301f, 0x23);
    sc4210_write_register(ViPipe, 0x3030, 0x02);
    sc4210_write_register(ViPipe, 0x3038, 0x22);
    sc4210_write_register(ViPipe, 0x303f, 0x81);
    sc4210_write_register(ViPipe, 0x320c, 0x05);
    sc4210_write_register(ViPipe, 0x320d, 0x46);
    sc4210_write_register(ViPipe, 0x3220, 0x10);
    sc4210_write_register(ViPipe, 0x3225, 0x01);
    sc4210_write_register(ViPipe, 0x3227, 0x03);
    sc4210_write_register(ViPipe, 0x3229, 0x08);
    sc4210_write_register(ViPipe, 0x3231, 0x01);
    sc4210_write_register(ViPipe, 0x3241, 0x02);
    sc4210_write_register(ViPipe, 0x3243, 0x03);
    sc4210_write_register(ViPipe, 0x3249, 0x17);
    sc4210_write_register(ViPipe, 0x3251, 0x08);
    sc4210_write_register(ViPipe, 0x3253, 0x08);
    sc4210_write_register(ViPipe, 0x325e, 0x00);
    sc4210_write_register(ViPipe, 0x325f, 0x00);
    sc4210_write_register(ViPipe, 0x3273, 0x01);
    sc4210_write_register(ViPipe, 0x3301, 0x28);
    sc4210_write_register(ViPipe, 0x3302, 0x18);
    sc4210_write_register(ViPipe, 0x3304, 0x14);
    sc4210_write_register(ViPipe, 0x3305, 0x00);
    sc4210_write_register(ViPipe, 0x3306, 0x50);
    sc4210_write_register(ViPipe, 0x3308, 0x10);
    sc4210_write_register(ViPipe, 0x3309, 0x24);
    sc4210_write_register(ViPipe, 0x330a, 0x00);
    sc4210_write_register(ViPipe, 0x330b, 0xa0);
    sc4210_write_register(ViPipe, 0x330e, 0x18);
    sc4210_write_register(ViPipe, 0x3312, 0x02);
    sc4210_write_register(ViPipe, 0x3314, 0x84);
    sc4210_write_register(ViPipe, 0x331e, 0x11);
    sc4210_write_register(ViPipe, 0x331f, 0x21);
    sc4210_write_register(ViPipe, 0x3320, 0x05);
    sc4210_write_register(ViPipe, 0x3338, 0x10);
    sc4210_write_register(ViPipe, 0x334c, 0x10);
    sc4210_write_register(ViPipe, 0x335d, 0x20);
    sc4210_write_register(ViPipe, 0x3366, 0x4a);
    sc4210_write_register(ViPipe, 0x3367, 0x08);
    sc4210_write_register(ViPipe, 0x3368, 0x05);
    sc4210_write_register(ViPipe, 0x3369, 0xdc);
    sc4210_write_register(ViPipe, 0x336a, 0x0b);
    sc4210_write_register(ViPipe, 0x336b, 0xb8);
    sc4210_write_register(ViPipe, 0x336c, 0xc2);
    sc4210_write_register(ViPipe, 0x337a, 0x08);
    sc4210_write_register(ViPipe, 0x337b, 0x10);
    sc4210_write_register(ViPipe, 0x337e, 0x40);
    sc4210_write_register(ViPipe, 0x33a3, 0x0c);
    sc4210_write_register(ViPipe, 0x33e0, 0xa0);
    sc4210_write_register(ViPipe, 0x33e1, 0x08);
    sc4210_write_register(ViPipe, 0x33e2, 0x00);
    sc4210_write_register(ViPipe, 0x33e3, 0x10);
    sc4210_write_register(ViPipe, 0x33e4, 0x10);
    sc4210_write_register(ViPipe, 0x33e5, 0x00);
    sc4210_write_register(ViPipe, 0x33e6, 0x10);
    sc4210_write_register(ViPipe, 0x33e7, 0x10);
    sc4210_write_register(ViPipe, 0x33e8, 0x00);
    sc4210_write_register(ViPipe, 0x33e9, 0x10);
    sc4210_write_register(ViPipe, 0x33ea, 0x16);
    sc4210_write_register(ViPipe, 0x33eb, 0x00);
    sc4210_write_register(ViPipe, 0x33ec, 0x10);
    sc4210_write_register(ViPipe, 0x33ed, 0x18);
    sc4210_write_register(ViPipe, 0x33ee, 0xa0);
    sc4210_write_register(ViPipe, 0x33ef, 0x08);
    sc4210_write_register(ViPipe, 0x33f4, 0x00);
    sc4210_write_register(ViPipe, 0x33f5, 0x10);
    sc4210_write_register(ViPipe, 0x33f6, 0x10);
    sc4210_write_register(ViPipe, 0x33f7, 0x00);
    sc4210_write_register(ViPipe, 0x33f8, 0x10);
    sc4210_write_register(ViPipe, 0x33f9, 0x10);
    sc4210_write_register(ViPipe, 0x33fa, 0x00);
    sc4210_write_register(ViPipe, 0x33fb, 0x10);
    sc4210_write_register(ViPipe, 0x33fc, 0x16);
    sc4210_write_register(ViPipe, 0x33fd, 0x00);
    sc4210_write_register(ViPipe, 0x33fe, 0x10);
    sc4210_write_register(ViPipe, 0x33ff, 0x18);
    sc4210_write_register(ViPipe, 0x360f, 0x05);
    sc4210_write_register(ViPipe, 0x3622, 0xff);
    sc4210_write_register(ViPipe, 0x3624, 0x07);
    sc4210_write_register(ViPipe, 0x3625, 0x02);
    sc4210_write_register(ViPipe, 0x3630, 0xc4);
    sc4210_write_register(ViPipe, 0x3631, 0x80);
    sc4210_write_register(ViPipe, 0x3632, 0x88);
    sc4210_write_register(ViPipe, 0x3633, 0x22);
    sc4210_write_register(ViPipe, 0x3634, 0x64);
    sc4210_write_register(ViPipe, 0x3635, 0x20);
    sc4210_write_register(ViPipe, 0x3636, 0x20);
    sc4210_write_register(ViPipe, 0x3638, 0x28);
    sc4210_write_register(ViPipe, 0x363b, 0x03);
    sc4210_write_register(ViPipe, 0x363c, 0x06);
    sc4210_write_register(ViPipe, 0x363d, 0x06);
    sc4210_write_register(ViPipe, 0x366e, 0x04);
    sc4210_write_register(ViPipe, 0x3670, 0x48);
    sc4210_write_register(ViPipe, 0x3671, 0xff);
    sc4210_write_register(ViPipe, 0x3672, 0x1f);
    sc4210_write_register(ViPipe, 0x3673, 0x1f);
    sc4210_write_register(ViPipe, 0x367a, 0x40);
    sc4210_write_register(ViPipe, 0x367b, 0x40);
    sc4210_write_register(ViPipe, 0x3690, 0x42);
    sc4210_write_register(ViPipe, 0x3691, 0x44);
    sc4210_write_register(ViPipe, 0x3692, 0x44);
    sc4210_write_register(ViPipe, 0x3699, 0x80);
    sc4210_write_register(ViPipe, 0x369a, 0x9f);
    sc4210_write_register(ViPipe, 0x369b, 0x9f);
    sc4210_write_register(ViPipe, 0x369c, 0x40);
    sc4210_write_register(ViPipe, 0x369d, 0x40);
    sc4210_write_register(ViPipe, 0x36a2, 0x40);
    sc4210_write_register(ViPipe, 0x36a3, 0x40);
    sc4210_write_register(ViPipe, 0x36cc, 0x2c);
    sc4210_write_register(ViPipe, 0x36cd, 0x30);
    sc4210_write_register(ViPipe, 0x36ce, 0x30);
    sc4210_write_register(ViPipe, 0x36d0, 0x20);
    sc4210_write_register(ViPipe, 0x36d1, 0x40);
    sc4210_write_register(ViPipe, 0x36d2, 0x40);
    sc4210_write_register(ViPipe, 0x36ea, 0x37);
    sc4210_write_register(ViPipe, 0x36eb, 0x1e);
    sc4210_write_register(ViPipe, 0x36ec, 0x13);
    sc4210_write_register(ViPipe, 0x36ed, 0x0c);
    sc4210_write_register(ViPipe, 0x36fa, 0x37);
    sc4210_write_register(ViPipe, 0x36fb, 0x09);
    sc4210_write_register(ViPipe, 0x36fc, 0x00);
    sc4210_write_register(ViPipe, 0x36fd, 0x2c);
    sc4210_write_register(ViPipe, 0x3817, 0x20);
    sc4210_write_register(ViPipe, 0x3905, 0xd8);
    sc4210_write_register(ViPipe, 0x3908, 0x11);
    sc4210_write_register(ViPipe, 0x391b, 0x80);
    sc4210_write_register(ViPipe, 0x391c, 0x0f);
    sc4210_write_register(ViPipe, 0x391d, 0x21);
    sc4210_write_register(ViPipe, 0x3933, 0x24);
    sc4210_write_register(ViPipe, 0x3934, 0xb0);
    sc4210_write_register(ViPipe, 0x3935, 0x80);
    sc4210_write_register(ViPipe, 0x3936, 0x1f);
    sc4210_write_register(ViPipe, 0x3940, 0x68);
    sc4210_write_register(ViPipe, 0x3942, 0x04);
    sc4210_write_register(ViPipe, 0x3943, 0xc0);
    sc4210_write_register(ViPipe, 0x3980, 0x00);
    sc4210_write_register(ViPipe, 0x3981, 0x50);
    sc4210_write_register(ViPipe, 0x3982, 0x00);
    sc4210_write_register(ViPipe, 0x3983, 0x40);
    sc4210_write_register(ViPipe, 0x3984, 0x00);
    sc4210_write_register(ViPipe, 0x3985, 0x20);
    sc4210_write_register(ViPipe, 0x3986, 0x00);
    sc4210_write_register(ViPipe, 0x3987, 0x10);
    sc4210_write_register(ViPipe, 0x3988, 0x00);
    sc4210_write_register(ViPipe, 0x3989, 0x20);
    sc4210_write_register(ViPipe, 0x398a, 0x00);
    sc4210_write_register(ViPipe, 0x398b, 0x30);
    sc4210_write_register(ViPipe, 0x398c, 0x00);
    sc4210_write_register(ViPipe, 0x398d, 0x50);
    sc4210_write_register(ViPipe, 0x398e, 0x00);
    sc4210_write_register(ViPipe, 0x398f, 0x60);
    sc4210_write_register(ViPipe, 0x3990, 0x00);
    sc4210_write_register(ViPipe, 0x3991, 0x70);
    sc4210_write_register(ViPipe, 0x3992, 0x00);
    sc4210_write_register(ViPipe, 0x3993, 0x36);
    sc4210_write_register(ViPipe, 0x3994, 0x00);
    sc4210_write_register(ViPipe, 0x3995, 0x20);
    sc4210_write_register(ViPipe, 0x3996, 0x00);
    sc4210_write_register(ViPipe, 0x3997, 0x14);
    sc4210_write_register(ViPipe, 0x3998, 0x00);
    sc4210_write_register(ViPipe, 0x3999, 0x20);
    sc4210_write_register(ViPipe, 0x399a, 0x00);
    sc4210_write_register(ViPipe, 0x399b, 0x50);
    sc4210_write_register(ViPipe, 0x399c, 0x00);
    sc4210_write_register(ViPipe, 0x399d, 0x90);
    sc4210_write_register(ViPipe, 0x399e, 0x00);
    sc4210_write_register(ViPipe, 0x399f, 0xf0);
    sc4210_write_register(ViPipe, 0x39a0, 0x08);
    sc4210_write_register(ViPipe, 0x39a1, 0x10);
    sc4210_write_register(ViPipe, 0x39a2, 0x20);
    sc4210_write_register(ViPipe, 0x39a3, 0x40);
    sc4210_write_register(ViPipe, 0x39a4, 0x20);
    sc4210_write_register(ViPipe, 0x39a5, 0x10);
    sc4210_write_register(ViPipe, 0x39a6, 0x08);
    sc4210_write_register(ViPipe, 0x39a7, 0x04);
    sc4210_write_register(ViPipe, 0x39a8, 0x18);
    sc4210_write_register(ViPipe, 0x39a9, 0x30);
    sc4210_write_register(ViPipe, 0x39aa, 0x40);
    sc4210_write_register(ViPipe, 0x39ab, 0x60);
    sc4210_write_register(ViPipe, 0x39ac, 0x38);
    sc4210_write_register(ViPipe, 0x39ad, 0x20);
    sc4210_write_register(ViPipe, 0x39ae, 0x10);
    sc4210_write_register(ViPipe, 0x39af, 0x08);
    sc4210_write_register(ViPipe, 0x39b9, 0x00);
    sc4210_write_register(ViPipe, 0x39ba, 0xa0);
    sc4210_write_register(ViPipe, 0x39bb, 0x80);
    sc4210_write_register(ViPipe, 0x39bc, 0x00);
    sc4210_write_register(ViPipe, 0x39bd, 0x44);
    sc4210_write_register(ViPipe, 0x39be, 0x00);
    sc4210_write_register(ViPipe, 0x39bf, 0x00);
    sc4210_write_register(ViPipe, 0x39c0, 0x00);
    sc4210_write_register(ViPipe, 0x39c5, 0x41);
    sc4210_write_register(ViPipe, 0x3c00, 0x45);
    sc4210_write_register(ViPipe, 0x3e00, 0x00);
    sc4210_write_register(ViPipe, 0x3e01, 0xbb);
    sc4210_write_register(ViPipe, 0x3e02, 0x40);
    sc4210_write_register(ViPipe, 0x3e03, 0x0b);
    sc4210_write_register(ViPipe, 0x3e06, 0x00);
    sc4210_write_register(ViPipe, 0x3e07, 0x80);
    sc4210_write_register(ViPipe, 0x3e08, 0x03);
    sc4210_write_register(ViPipe, 0x3e09, 0x40);
    sc4210_write_register(ViPipe, 0x3e0e, 0x6a);
    sc4210_write_register(ViPipe, 0x3e26, 0x40);
    sc4210_write_register(ViPipe, 0x4407, 0xb0);
    sc4210_write_register(ViPipe, 0x4418, 0x16);
    sc4210_write_register(ViPipe, 0x4501, 0xa4);
    sc4210_write_register(ViPipe, 0x4509, 0x08);
    sc4210_write_register(ViPipe, 0x4837, 0x57);
    sc4210_write_register(ViPipe, 0x5000, 0x0e);
    sc4210_write_register(ViPipe, 0x550f, 0x20);
    sc4210_write_register(ViPipe, 0x5784, 0x10);
    sc4210_write_register(ViPipe, 0x5785, 0x08);
    sc4210_write_register(ViPipe, 0x5787, 0x06);
    sc4210_write_register(ViPipe, 0x5788, 0x06);
    sc4210_write_register(ViPipe, 0x5789, 0x00);
    sc4210_write_register(ViPipe, 0x578a, 0x06);
    sc4210_write_register(ViPipe, 0x578b, 0x06);
    sc4210_write_register(ViPipe, 0x578c, 0x00);
    sc4210_write_register(ViPipe, 0x5790, 0x10);
    sc4210_write_register(ViPipe, 0x5791, 0x10);
    sc4210_write_register(ViPipe, 0x5792, 0x00);
    sc4210_write_register(ViPipe, 0x5793, 0x10);
    sc4210_write_register(ViPipe, 0x5794, 0x10);
    sc4210_write_register(ViPipe, 0x5795, 0x00);
    sc4210_write_register(ViPipe, 0x57c4, 0x10);
    sc4210_write_register(ViPipe, 0x57c5, 0x08);
    sc4210_write_register(ViPipe, 0x57c7, 0x06);
    sc4210_write_register(ViPipe, 0x57c8, 0x06);
    sc4210_write_register(ViPipe, 0x57c9, 0x00);
    sc4210_write_register(ViPipe, 0x57ca, 0x06);
    sc4210_write_register(ViPipe, 0x57cb, 0x06);
    sc4210_write_register(ViPipe, 0x57cc, 0x00);
    sc4210_write_register(ViPipe, 0x57d0, 0x10);
    sc4210_write_register(ViPipe, 0x57d1, 0x10);
    sc4210_write_register(ViPipe, 0x57d2, 0x00);
    sc4210_write_register(ViPipe, 0x57d3, 0x10);
    sc4210_write_register(ViPipe, 0x57d4, 0x10);
    sc4210_write_register(ViPipe, 0x57d5, 0x00);
    sc4210_write_register(ViPipe, 0x36e9, 0x27);
    sc4210_write_register(ViPipe, 0x36f9, 0x20);
    sc4210_write_register(ViPipe, 0x0100, 0x01);

    printf("=========================================================================\n");
    printf("==  SC4210_MIPI_24MInput_607.5Mbps_4lane_10bit_2560x1440_HDR 15fps(VC) ==\n");
    printf("=========================================================================\n");

    return SC_SUCCESS;
}
