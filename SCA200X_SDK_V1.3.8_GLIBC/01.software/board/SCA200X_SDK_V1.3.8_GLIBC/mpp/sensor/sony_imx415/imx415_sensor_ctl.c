/**
 * @file     imx415_sensor_ctl.c
 * @brief    SONY IMX415 SENSOR初始化接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-12-01 创建文件
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


#define IMX415_FLIP_MIRROR (0x3030)

#define SENSOR_4K_30FPS_LINEAR_MODE (1)
#define SENSOR_4K_60FPS_LINEAR_MODE (2)

const unsigned char imx415_i2c_addr  = 0x34;  /* I2C Address of IMX415 */
const unsigned int  imx415_addr_byte = 2;
const unsigned int  imx415_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

extern ISP_SNS_STATE_S   *g_pastImx415[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U  g_aunImx415BusInfo[ISP_MAX_PIPE_NUM];

int imx415_linear_4kp30_init(VI_PIPE ViPipe);
int imx415_linear_4kp60_init(VI_PIPE ViPipe);


int imx415_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {

        return SC_SUCCESS;
    }

    u8DevNum = g_aunImx415BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (imx415_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int imx415_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int imx415_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:
    return SC_SUCCESS;
}

int imx415_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(imx415_addr_byte == 2)
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

    if(imx415_data_byte == 2)
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

    ret = write(g_fd[ViPipe], buf, imx415_addr_byte + imx415_data_byte);
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

void imx415_prog(VI_PIPE ViPipe, int *rom)
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
            imx415_write_register(ViPipe, addr, data);
        }
    }
}

void imx415_standby(VI_PIPE ViPipe)
{
    // TODO:
    return;
}

void imx415_restart(VI_PIPE ViPipe)
{
    // TODO:
    return;
}

int imx415_flip_off_mirror_off(VI_PIPE ViPipe)
{
    imx415_write_register(ViPipe, IMX415_FLIP_MIRROR, 0x00);
    return SC_SUCCESS;
}

int imx415_flip_on_mirror_off(VI_PIPE ViPipe)
{
    imx415_write_register(ViPipe, IMX415_FLIP_MIRROR, 0x02);
    return SC_SUCCESS;
}

int imx415_flip_off_mirror_on(VI_PIPE ViPipe)
{
    imx415_write_register(ViPipe, IMX415_FLIP_MIRROR, 0x01);
    return SC_SUCCESS;
}

int imx415_flip_on_mirror_on(VI_PIPE ViPipe)
{
    imx415_write_register(ViPipe, IMX415_FLIP_MIRROR, 0x03);
    return SC_SUCCESS;
}

void imx415_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i = 0;

    for(i = 0; i < g_pastImx415[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastImx415[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            imx415_write_register(ViPipe,
                                  g_pastImx415[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                                  g_pastImx415[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int imx415_init(VI_PIPE ViPipe)
{
    WDR_MODE_E enWDRMode = WDR_MODE_NONE;
    SC_BOOL    bInit     = SC_FALSE;
    SC_U8      u8ImgMode = 0;
    SC_S32     ret       = SC_FAILURE;

    bInit     = g_pastImx415[ViPipe]->bInit;
    enWDRMode = g_pastImx415[ViPipe]->enWDRMode;
    u8ImgMode = g_pastImx415[ViPipe]->u8ImgMode;
    printf("IMX415: bInit[%d] enWDRMode[%d] u8ImgMode[%u]\n", bInit, enWDRMode, (SC_U32)u8ImgMode);

    ret = imx415_i2c_init(ViPipe);
    if(SC_SUCCESS != ret)
    {
        printf("Init imx415 i2c error!\n");
        return SC_FAILURE;
    }

    /* When sensor first init, config all registers */
    if(SC_FALSE == bInit)
    {
        if(WDR_MODE_2To1_LINE == enWDRMode)
        {
#if 0
            if(IMX415_SENSOR_1080P_30FPS_2t1_WDR_MODE == u8ImgMode)
            {
                ret = imx415_wdr_1080p60_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init imx415 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
#endif
        }
        else
        {
            if(SENSOR_4K_30FPS_LINEAR_MODE == u8ImgMode)
            {
                ret = imx415_linear_4kp30_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init imx415 4k30fps linear mode error!\n");
                    return SC_FAILURE;
                }
            }
            else if(SENSOR_4K_60FPS_LINEAR_MODE == u8ImgMode)
            {
                ret = imx415_linear_4kp60_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init imx415 4k60fps linear mode error!\n");
                    return SC_FAILURE;
                }
            }
        }
    }
    /* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
    else
    {
        if(WDR_MODE_2To1_LINE == enWDRMode)
        {

        }
        else
        {
            if(SENSOR_4K_30FPS_LINEAR_MODE == u8ImgMode)
            {
                ret = imx415_linear_4kp30_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init imx415 4k30fps linear mode error!\n");
                    return SC_FAILURE;
                }
            }
            else if(SENSOR_4K_60FPS_LINEAR_MODE == u8ImgMode)
            {
                ret = imx415_linear_4kp60_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init imx415 4k60fps linear mode error!\n");
                    return SC_FAILURE;
                }
            }
        }
    }

    g_pastImx415[ViPipe]->bInit = SC_TRUE;

    return SC_SUCCESS;
}

void imx415_exit(VI_PIPE ViPipe)
{
    imx415_i2c_exit(ViPipe);

    return;
}


/* 3840x2160@30fps LINEAR; Raw:12Bit; MCLK Input: 37.125MHz; MIPI CLK: 891Mbps/lane; CSI-2 4Lane */
int imx415_linear_4kp30_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = imx415_write_register(ViPipe, 0x3008, 0x7F);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write imx415 register error!\n");
        return SC_FAILURE;
    }

    imx415_write_register(ViPipe, 0x300A, 0x5B);
    imx415_write_register(ViPipe, 0x3024, 0xCA); /* vmax */
    imx415_write_register(ViPipe, 0x3025, 0x08);
    imx415_write_register(ViPipe, 0x3028, 0x4C); /* hmax */
    imx415_write_register(ViPipe, 0x3029, 0x04);
    imx415_write_register(ViPipe, 0x3033, 0x05);
    imx415_write_register(ViPipe, 0x3050, 0x08);
    imx415_write_register(ViPipe, 0x30C1, 0x00);
    imx415_write_register(ViPipe, 0x302C, 0x00); /* normal exp/gain */
    imx415_write_register(ViPipe, 0x302D, 0x00);
    imx415_write_register(ViPipe, 0x3260, 0x01);

    imx415_write_register(ViPipe, 0x3116, 0x24);
    imx415_write_register(ViPipe, 0x311E, 0x24);
    imx415_write_register(ViPipe, 0x32D4, 0x21);
    imx415_write_register(ViPipe, 0x32EC, 0xA1);
    imx415_write_register(ViPipe, 0x3452, 0x7F);
    imx415_write_register(ViPipe, 0x3453, 0x03);
    imx415_write_register(ViPipe, 0x358A, 0x04);
    imx415_write_register(ViPipe, 0x35A1, 0x02);
    imx415_write_register(ViPipe, 0x36BC, 0x0C);
    imx415_write_register(ViPipe, 0x36CC, 0x53);
    imx415_write_register(ViPipe, 0x36CD, 0x00);
    imx415_write_register(ViPipe, 0x36CE, 0x3C);
    imx415_write_register(ViPipe, 0x36D0, 0x8C);
    imx415_write_register(ViPipe, 0x36D1, 0x00);
    imx415_write_register(ViPipe, 0x36D2, 0x71);
    imx415_write_register(ViPipe, 0x36D4, 0x3C);
    imx415_write_register(ViPipe, 0x36D6, 0x53);
    imx415_write_register(ViPipe, 0x36D7, 0x00);
    imx415_write_register(ViPipe, 0x36D8, 0x71);
    imx415_write_register(ViPipe, 0x36DA, 0x8C);
    imx415_write_register(ViPipe, 0x36DB, 0x00);
    imx415_write_register(ViPipe, 0x3724, 0x02);
    imx415_write_register(ViPipe, 0x3726, 0x02);
    imx415_write_register(ViPipe, 0x3732, 0x02);
    imx415_write_register(ViPipe, 0x3734, 0x03);
    imx415_write_register(ViPipe, 0x3736, 0x03);
    imx415_write_register(ViPipe, 0x3742, 0x03);
    imx415_write_register(ViPipe, 0x3862, 0xE0);
    imx415_write_register(ViPipe, 0x38CC, 0x30);
    imx415_write_register(ViPipe, 0x38CD, 0x2F);
    imx415_write_register(ViPipe, 0x395C, 0x0C);
    imx415_write_register(ViPipe, 0x3A42, 0xD1);
    imx415_write_register(ViPipe, 0x3A4C, 0x77);
    imx415_write_register(ViPipe, 0x3AE0, 0x02);
    imx415_write_register(ViPipe, 0x3AEC, 0x0C);
    imx415_write_register(ViPipe, 0x3B00, 0x2E);
    imx415_write_register(ViPipe, 0x3B06, 0x29);
    imx415_write_register(ViPipe, 0x3B98, 0x25);
    imx415_write_register(ViPipe, 0x3B99, 0x21);
    imx415_write_register(ViPipe, 0x3B9B, 0x13);
    imx415_write_register(ViPipe, 0x3B9C, 0x13);
    imx415_write_register(ViPipe, 0x3B9D, 0x13);
    imx415_write_register(ViPipe, 0x3B9E, 0x13);
    imx415_write_register(ViPipe, 0x3BA1, 0x00);
    imx415_write_register(ViPipe, 0x3BA2, 0x06);
    imx415_write_register(ViPipe, 0x3BA3, 0x0B);
    imx415_write_register(ViPipe, 0x3BA4, 0x10);
    imx415_write_register(ViPipe, 0x3BA5, 0x14);
    imx415_write_register(ViPipe, 0x3BA6, 0x18);
    imx415_write_register(ViPipe, 0x3BA7, 0x1A);
    imx415_write_register(ViPipe, 0x3BA8, 0x1A);
    imx415_write_register(ViPipe, 0x3BA9, 0x1A);
    imx415_write_register(ViPipe, 0x3BAC, 0xED);
    imx415_write_register(ViPipe, 0x3BAD, 0x01);
    imx415_write_register(ViPipe, 0x3BAE, 0xF6);
    imx415_write_register(ViPipe, 0x3BAF, 0x02);
    imx415_write_register(ViPipe, 0x3BB0, 0xA2);
    imx415_write_register(ViPipe, 0x3BB1, 0x03);
    imx415_write_register(ViPipe, 0x3BB2, 0xE0);
    imx415_write_register(ViPipe, 0x3BB3, 0x03);
    imx415_write_register(ViPipe, 0x3BB4, 0xE0);
    imx415_write_register(ViPipe, 0x3BB5, 0x03);
    imx415_write_register(ViPipe, 0x3BB6, 0xE0);
    imx415_write_register(ViPipe, 0x3BB7, 0x03);
    imx415_write_register(ViPipe, 0x3BB8, 0xE0);
    imx415_write_register(ViPipe, 0x3BBA, 0xE0);
    imx415_write_register(ViPipe, 0x3BBC, 0xDA);
    imx415_write_register(ViPipe, 0x3BBE, 0x88);
    imx415_write_register(ViPipe, 0x3BC0, 0x44);
    imx415_write_register(ViPipe, 0x3BC2, 0x7B);
    imx415_write_register(ViPipe, 0x3BC4, 0xA2);
    imx415_write_register(ViPipe, 0x3BC8, 0xBD);
    imx415_write_register(ViPipe, 0x3BCA, 0xBD);

    imx415_write_register(ViPipe, 0x4004, 0x48);
    imx415_write_register(ViPipe, 0x4005, 0x09);
    imx415_write_register(ViPipe, 0x400C, 0x00);
    imx415_write_register(ViPipe, 0x4018, 0x7F);
    imx415_write_register(ViPipe, 0x401A, 0x37);
    imx415_write_register(ViPipe, 0x401C, 0x37);
    imx415_write_register(ViPipe, 0x401E, 0xF7);
    imx415_write_register(ViPipe, 0x401F, 0x00);
    imx415_write_register(ViPipe, 0x4020, 0x3F);
    imx415_write_register(ViPipe, 0x4022, 0x6F);
    imx415_write_register(ViPipe, 0x4024, 0x3F);
    imx415_write_register(ViPipe, 0x4026, 0x5F);
    imx415_write_register(ViPipe, 0x4028, 0x2F);
    imx415_write_register(ViPipe, 0x4074, 0x01);

    imx415_default_reg_init(ViPipe);

    imx415_write_register(ViPipe, 0x3000, 0x00); /* standby */
    delay_ms(20);
    imx415_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
    //imx415_write_register(ViPipe, 0x304b, 0x0a); /* XVSOUTSEL XHSOUTSEL */

    printf("==============================================================\n");
    printf("== Sony imx415 sensor 4K30fps 12bit init success!           ==\n");
    printf("==============================================================\n");

    return SC_SUCCESS;
}

/* 4kP60 */
int imx415_linear_4kp60_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    /*
     * IMX415-AAQR All-pixel scan CSI-2_4lane 37.125Mhz AD:12bit Output:12bit 1782Mbps Master Mode 30fps
     * Integration Time 33.277ms
     */
    ret = imx415_write_register(ViPipe, 0x3008, 0x7F);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write imx415 register error!\n");
        return SC_FAILURE;
    }

    imx415_write_register(ViPipe, 0x300A, 0x5B);

    imx415_write_register(ViPipe, 0x3024, 0xca); /* vmax */
    imx415_write_register(ViPipe, 0x3025, 0x08);

    imx415_write_register(ViPipe, 0x3028, 0x26); /* hmax */
    imx415_write_register(ViPipe, 0x3029, 0x02);

    imx415_write_register(ViPipe, 0x3050, 0x08);
    imx415_write_register(ViPipe, 0x30C1, 0x00);
    imx415_write_register(ViPipe, 0x3116, 0x24);
    imx415_write_register(ViPipe, 0x311E, 0x24);
    imx415_write_register(ViPipe, 0x32D4, 0x21);
    imx415_write_register(ViPipe, 0x32EC, 0xA1);
    imx415_write_register(ViPipe, 0x3452, 0x7F);
    imx415_write_register(ViPipe, 0x3453, 0x03);
    imx415_write_register(ViPipe, 0x358A, 0x04);
    imx415_write_register(ViPipe, 0x35A1, 0x02);
    imx415_write_register(ViPipe, 0x36BC, 0x0C);
    imx415_write_register(ViPipe, 0x36CC, 0x53);
    imx415_write_register(ViPipe, 0x36CD, 0x00);
    imx415_write_register(ViPipe, 0x36CE, 0x3C);
    imx415_write_register(ViPipe, 0x36D0, 0x8C);
    imx415_write_register(ViPipe, 0x36D1, 0x00);
    imx415_write_register(ViPipe, 0x36D2, 0x71);
    imx415_write_register(ViPipe, 0x36D4, 0x3C);
    imx415_write_register(ViPipe, 0x36D6, 0x53);
    imx415_write_register(ViPipe, 0x36D7, 0x00);
    imx415_write_register(ViPipe, 0x36D8, 0x71);
    imx415_write_register(ViPipe, 0x36DA, 0x8C);
    imx415_write_register(ViPipe, 0x36DB, 0x00);
    imx415_write_register(ViPipe, 0x3724, 0x02);
    imx415_write_register(ViPipe, 0x3726, 0x02);
    imx415_write_register(ViPipe, 0x3732, 0x02);
    imx415_write_register(ViPipe, 0x3734, 0x03);
    imx415_write_register(ViPipe, 0x3736, 0x03);
    imx415_write_register(ViPipe, 0x3742, 0x03);
    imx415_write_register(ViPipe, 0x3862, 0xE0);
    imx415_write_register(ViPipe, 0x38CC, 0x30);
    imx415_write_register(ViPipe, 0x38CD, 0x2F);
    imx415_write_register(ViPipe, 0x395C, 0x0C);
    imx415_write_register(ViPipe, 0x3A42, 0xD1);
    imx415_write_register(ViPipe, 0x3A4C, 0x77);
    imx415_write_register(ViPipe, 0x3AE0, 0x02);
    imx415_write_register(ViPipe, 0x3AEC, 0x0C);
    imx415_write_register(ViPipe, 0x3B00, 0x2E);
    imx415_write_register(ViPipe, 0x3B06, 0x29);
    imx415_write_register(ViPipe, 0x3B98, 0x25);
    imx415_write_register(ViPipe, 0x3B99, 0x21);
    imx415_write_register(ViPipe, 0x3B9B, 0x13);
    imx415_write_register(ViPipe, 0x3B9C, 0x13);
    imx415_write_register(ViPipe, 0x3B9D, 0x13);
    imx415_write_register(ViPipe, 0x3B9E, 0x13);
    imx415_write_register(ViPipe, 0x3BA1, 0x00);
    imx415_write_register(ViPipe, 0x3BA2, 0x06);
    imx415_write_register(ViPipe, 0x3BA3, 0x0B);
    imx415_write_register(ViPipe, 0x3BA4, 0x10);
    imx415_write_register(ViPipe, 0x3BA5, 0x14);
    imx415_write_register(ViPipe, 0x3BA6, 0x18);
    imx415_write_register(ViPipe, 0x3BA7, 0x1A);
    imx415_write_register(ViPipe, 0x3BA8, 0x1A);
    imx415_write_register(ViPipe, 0x3BA9, 0x1A);
    imx415_write_register(ViPipe, 0x3BAC, 0xED);
    imx415_write_register(ViPipe, 0x3BAD, 0x01);
    imx415_write_register(ViPipe, 0x3BAE, 0xF6);
    imx415_write_register(ViPipe, 0x3BAF, 0x02);
    imx415_write_register(ViPipe, 0x3BB0, 0xA2);
    imx415_write_register(ViPipe, 0x3BB1, 0x03);
    imx415_write_register(ViPipe, 0x3BB2, 0xE0);
    imx415_write_register(ViPipe, 0x3BB3, 0x03);
    imx415_write_register(ViPipe, 0x3BB4, 0xE0);
    imx415_write_register(ViPipe, 0x3BB5, 0x03);
    imx415_write_register(ViPipe, 0x3BB6, 0xE0);
    imx415_write_register(ViPipe, 0x3BB7, 0x03);
    imx415_write_register(ViPipe, 0x3BB8, 0xE0);
    imx415_write_register(ViPipe, 0x3BBA, 0xE0);
    imx415_write_register(ViPipe, 0x3BBC, 0xDA);
    imx415_write_register(ViPipe, 0x3BBE, 0x88);
    imx415_write_register(ViPipe, 0x3BC0, 0x44);
    imx415_write_register(ViPipe, 0x3BC2, 0x7B);
    imx415_write_register(ViPipe, 0x3BC4, 0xA2);
    imx415_write_register(ViPipe, 0x3BC8, 0xBD);
    imx415_write_register(ViPipe, 0x3BCA, 0xBD);
    imx415_write_register(ViPipe, 0x4004, 0x48);
    imx415_write_register(ViPipe, 0x4005, 0x09);

    imx415_default_reg_init(ViPipe);

    imx415_write_register(ViPipe, 0x3000, 0x00); /* standby */
    delay_ms(20);
    imx415_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
    //imx415_write_register(ViPipe, 0x304b, 0x0a); /* XVSOUTSEL XHSOUTSEL */

    printf("==============================================================\n");
    printf("== Sony imx415 sensor 4K36fps 12bit init success!           ==\n");
    printf("==============================================================\n");
    return SC_SUCCESS;
}
