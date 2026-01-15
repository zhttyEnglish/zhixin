/**
 * @file     ov13b10_sensor_ctl.c
 * @brief    SENSOR初始化接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2023-05-09 创建文件
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


#define OV13B10_FLIP_MIRROR (0x3820)

#define SENSOR_13M_1FPS_LINEAR_10BIT_MODE    (1)
#define SENSOR_1080P_30FPS_LINEAR_10BIT_MODE (2)

const unsigned char ov13b10_i2c_addr  = 0x20; /* I2C Address of OV13B10: 0x6c or 0x20 */
const unsigned int  ov13b10_addr_byte = 2;
const unsigned int  ov13b10_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

extern ISP_SNS_STATE_S   *g_pastOv13b10[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunOv13b10BusInfo[];

int ov13b10_linear_13MP1_10bit_init(VI_PIPE ViPipe);
int ov13b10_linear_1080P30_init(VI_PIPE ViPipe);


int ov13b10_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {
        return SC_SUCCESS;
    }

    u8DevNum = g_aunOv13b10BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (ov13b10_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int ov13b10_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int ov13b10_read_register(VI_PIPE ViPipe, int addr)
{
    return SC_SUCCESS;
}

int ov13b10_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(ov13b10_addr_byte == 2)
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

    if(ov13b10_data_byte == 2)
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

    ret = write(g_fd[ViPipe], buf, ov13b10_addr_byte + ov13b10_data_byte);
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
    return;
}

void ov13b10_prog(VI_PIPE ViPipe, int *rom)
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
            ov13b10_write_register(ViPipe, addr, data);
        }
    }
}

void ov13b10_standby(VI_PIPE ViPipe)
{
    return;
}

void ov13b10_restart(VI_PIPE ViPipe)
{
    return;
}

int ov13b10_flip_off_mirror_off(VI_PIPE ViPipe)
{
    ov13b10_write_register(ViPipe, OV13B10_FLIP_MIRROR, 0x00);
    return SC_SUCCESS;
}

int ov13b10_flip_on_mirror_off(VI_PIPE ViPipe)
{
    ov13b10_write_register(ViPipe, OV13B10_FLIP_MIRROR, 0x10);
    return SC_SUCCESS;
}

int ov13b10_flip_off_mirror_on(VI_PIPE ViPipe)
{
    ov13b10_write_register(ViPipe, OV13B10_FLIP_MIRROR, 0x08);
    return SC_SUCCESS;
}

int ov13b10_flip_on_mirror_on(VI_PIPE ViPipe)
{
    ov13b10_write_register(ViPipe, OV13B10_FLIP_MIRROR, 0x18);
    return SC_SUCCESS;
}

void ov13b10_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i;

    for(i = 0; i < g_pastOv13b10[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastOv13b10[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            ov13b10_write_register(ViPipe,
                g_pastOv13b10[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                g_pastOv13b10[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int ov13b10_init(VI_PIPE ViPipe)
{
    WDR_MODE_E enWDRMode = WDR_MODE_NONE;
    SC_BOOL    bInit     = SC_FALSE;
    SC_U8      u8ImgMode = 0;
    SC_S32     ret       = SC_FAILURE;

    bInit     = g_pastOv13b10[ViPipe]->bInit;
    enWDRMode = g_pastOv13b10[ViPipe]->enWDRMode;
    u8ImgMode = g_pastOv13b10[ViPipe]->u8ImgMode;
    printf("OV13B10: bInit[%d] enWDRMode[%d] u8ImgMode[%u]\n", bInit, enWDRMode, (SC_U32)u8ImgMode);

    ret = ov13b10_i2c_init(ViPipe);
    if(SC_SUCCESS != ret)
    {
        printf("Init ov13b10 i2c error!\n");
        return SC_FAILURE;
    }

    /* When sensor first init, config all registers */
    if(SC_FALSE == bInit)
    {
        if(WDR_MODE_NONE == enWDRMode)
        {
            if(SENSOR_13M_1FPS_LINEAR_10BIT_MODE == u8ImgMode)
            {
                ret = ov13b10_linear_13MP1_10bit_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init ov13b10 13MP1 mode error!\n");
                    return SC_FAILURE;
                }
            }
            else if(SENSOR_1080P_30FPS_LINEAR_10BIT_MODE == u8ImgMode)
            {
                ret = ov13b10_linear_1080P30_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init ov13b10 1080P30 mode error!\n");
                    return SC_FAILURE;
                }
            }
        }

    }
    /* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
    else
    {
        if(WDR_MODE_NONE == enWDRMode)
        {
            if(SENSOR_13M_1FPS_LINEAR_10BIT_MODE == u8ImgMode)
            {
                ret = ov13b10_linear_13MP1_10bit_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init ov13b10 13MP1 mode error!\n");
                    return SC_FAILURE;
                }
            }
            else if(SENSOR_1080P_30FPS_LINEAR_10BIT_MODE == u8ImgMode)
            {
                ret = ov13b10_linear_1080P30_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init ov13b10 1080P30 mode error!\n");
                    return SC_FAILURE;
                }
            }
        }
    }

    g_pastOv13b10[ViPipe]->bInit = SC_TRUE;

    return SC_SUCCESS;
}

void ov13b10_exit(VI_PIPE ViPipe)
{
    ov13b10_i2c_exit(ViPipe);
    return;
}

/* 4096x3120@1fps LINEAR; Raw:10Bit; MCLK Input: 24MHz; MIPI CLK: 560Mbps/lane; CSI-2 2Lane */
int ov13b10_linear_13MP1_10bit_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    /* SOFTWARE_RST */
    ret = ov13b10_write_register(ViPipe, 0x0103, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write ov13b10 register error!\n");
        return SC_FAILURE;
    }

    /* PLL */
    ov13b10_write_register(ViPipe, 0x0301, 0x80); /* PLL_CTRL_1 Bit[6] pll1_predivp   0:   /1 */
    ov13b10_write_register(ViPipe, 0x0303, 0x01); /* PLL_CTRL_3 Bit[2:0] pll1_prediv  001: /1.5 */
    ov13b10_write_register(ViPipe, 0x0304, 0x00); /* PLL_CTRL_4 pll1_multiplier[9:8] */
    ov13b10_write_register(ViPipe, 0x0305, 0x46); /* PLL_CTRL_5 pll1_multiplier[7:0] */
    ov13b10_write_register(ViPipe, 0x0306, 0x00); /* PLL_CTRL_6 Bit[1:0] pll1_divpix  00:  /1 */

    ov13b10_write_register(ViPipe, 0x0321, 0x00);
    ov13b10_write_register(ViPipe, 0x0323, 0x04);
    ov13b10_write_register(ViPipe, 0x0324, 0x01);
    ov13b10_write_register(ViPipe, 0x0325, 0x50);
    ov13b10_write_register(ViPipe, 0x0326, 0x81);
    ov13b10_write_register(ViPipe, 0x0327, 0x04);
    ov13b10_write_register(ViPipe, 0x3011, 0x7c);
    ov13b10_write_register(ViPipe, 0x3012, 0x07);
    ov13b10_write_register(ViPipe, 0x3013, 0x32);
    ov13b10_write_register(ViPipe, 0x3107, 0x23);
    ov13b10_write_register(ViPipe, 0x3501, 0x0c);
    ov13b10_write_register(ViPipe, 0x3502, 0x10);
    ov13b10_write_register(ViPipe, 0x3504, 0x08);
    ov13b10_write_register(ViPipe, 0x3508, 0x07);
    ov13b10_write_register(ViPipe, 0x3509, 0xc0);
    ov13b10_write_register(ViPipe, 0x3600, 0x16);
    ov13b10_write_register(ViPipe, 0x3601, 0x54);
    ov13b10_write_register(ViPipe, 0x3612, 0x4e);
    ov13b10_write_register(ViPipe, 0x3620, 0x00);
    ov13b10_write_register(ViPipe, 0x3621, 0x68);
    ov13b10_write_register(ViPipe, 0x3622, 0x66);
    ov13b10_write_register(ViPipe, 0x3623, 0x03);
    ov13b10_write_register(ViPipe, 0x3662, 0x92);
    ov13b10_write_register(ViPipe, 0x3666, 0xbb);
    ov13b10_write_register(ViPipe, 0x3667, 0x44);
    ov13b10_write_register(ViPipe, 0x366e, 0xff);
    ov13b10_write_register(ViPipe, 0x366f, 0xf3);
    ov13b10_write_register(ViPipe, 0x3675, 0x44);
    ov13b10_write_register(ViPipe, 0x3676, 0x00);
    ov13b10_write_register(ViPipe, 0x367f, 0xe9);
    ov13b10_write_register(ViPipe, 0x3681, 0x32);
    ov13b10_write_register(ViPipe, 0x3682, 0x1f);
    ov13b10_write_register(ViPipe, 0x3683, 0x0b);
    ov13b10_write_register(ViPipe, 0x3684, 0x0b);
    ov13b10_write_register(ViPipe, 0x3704, 0x0f);
    ov13b10_write_register(ViPipe, 0x3706, 0x40);
    ov13b10_write_register(ViPipe, 0x3708, 0x3b);
    ov13b10_write_register(ViPipe, 0x3709, 0x72);
    ov13b10_write_register(ViPipe, 0x370b, 0xa2);
    ov13b10_write_register(ViPipe, 0x3714, 0x24);
    ov13b10_write_register(ViPipe, 0x371a, 0x3e);
    ov13b10_write_register(ViPipe, 0x3725, 0x42);
    ov13b10_write_register(ViPipe, 0x3739, 0x12);
    ov13b10_write_register(ViPipe, 0x3767, 0x00);
    ov13b10_write_register(ViPipe, 0x377a, 0x0d);
    ov13b10_write_register(ViPipe, 0x3789, 0x18);
    ov13b10_write_register(ViPipe, 0x3790, 0x40);
    ov13b10_write_register(ViPipe, 0x3791, 0xa2);
    ov13b10_write_register(ViPipe, 0x37c2, 0x04);
    ov13b10_write_register(ViPipe, 0x37c3, 0xf1);
    ov13b10_write_register(ViPipe, 0x37d9, 0x0c);
    ov13b10_write_register(ViPipe, 0x37da, 0x02);
    ov13b10_write_register(ViPipe, 0x37dc, 0x02);
    ov13b10_write_register(ViPipe, 0x37e1, 0x04);
    ov13b10_write_register(ViPipe, 0x37e2, 0x0a);
    ov13b10_write_register(ViPipe, 0x3800, 0x00); /* X_ADDR_START[15:8] */
    ov13b10_write_register(ViPipe, 0x3801, 0x00); /* X_ADDR_START[ 7:0] */
    ov13b10_write_register(ViPipe, 0x3802, 0x00); /* Y_ADDR_START[15:8] */
    ov13b10_write_register(ViPipe, 0x3803, 0x08); /* Y_ADDR_START[ 7:0] */
    ov13b10_write_register(ViPipe, 0x3804, 0x10); /* X_ADDR_END[15:8] 0x108F=4239 */
    ov13b10_write_register(ViPipe, 0x3805, 0x8f); /* X_ADDR_END[ 7:0] */
    ov13b10_write_register(ViPipe, 0x3806, 0x0c); /* Y_ADDR_END[15:8] 0x0C47=3143 */
    ov13b10_write_register(ViPipe, 0x3807, 0x47); /* Y_ADDR_END[ 7:0] */
    ov13b10_write_register(ViPipe, 0x3808, 0x10); /* X_OUTPUT_SIZE[15:8] 0x1000=4096 */
    ov13b10_write_register(ViPipe, 0x3809, 0x00); /* X_OUTPUT_SIZE[ 7:0] */
    ov13b10_write_register(ViPipe, 0x380a, 0x0c); /* Y_OUTPUT_SIZE[15:8] 0x0C30=3120 */
    ov13b10_write_register(ViPipe, 0x380b, 0x30); /* Y_OUTPUT_SIZE[ 7:0] */
    ov13b10_write_register(ViPipe, 0x380c, 0x06); /* HTS[14:8] 0x6ad=1709 */
    ov13b10_write_register(ViPipe, 0x380d, 0xad); /* HTS[ 7:0] */
    ov13b10_write_register(ViPipe, 0x380e, 0xff); /* VTS[14:8] */
    ov13b10_write_register(ViPipe, 0x380f, 0xff); /* VTS[ 0:0] */
    ov13b10_write_register(ViPipe, 0x3811, 0x47);
    ov13b10_write_register(ViPipe, 0x3813, 0x08);
    ov13b10_write_register(ViPipe, 0x3814, 0x01);
    ov13b10_write_register(ViPipe, 0x3815, 0x01);
    ov13b10_write_register(ViPipe, 0x3816, 0x01);
    ov13b10_write_register(ViPipe, 0x3817, 0x01);
    ov13b10_write_register(ViPipe, 0x381f, 0x08);
    ov13b10_write_register(ViPipe, 0x3820, 0x88);
    ov13b10_write_register(ViPipe, 0x3821, 0x00);
    ov13b10_write_register(ViPipe, 0x3822, 0x14);
    ov13b10_write_register(ViPipe, 0x3823, 0x18);
    ov13b10_write_register(ViPipe, 0x3827, 0x01);
    ov13b10_write_register(ViPipe, 0x382e, 0xe6);
    ov13b10_write_register(ViPipe, 0x3c80, 0x00);
    ov13b10_write_register(ViPipe, 0x3c87, 0x01);
    ov13b10_write_register(ViPipe, 0x3c8c, 0x19);
    ov13b10_write_register(ViPipe, 0x3c8d, 0x1c);
    ov13b10_write_register(ViPipe, 0x3ca0, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca1, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca2, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca3, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca4, 0x50);
    ov13b10_write_register(ViPipe, 0x3ca5, 0x11);
    ov13b10_write_register(ViPipe, 0x3ca6, 0x01);
    ov13b10_write_register(ViPipe, 0x3ca7, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca8, 0x00);
    ov13b10_write_register(ViPipe, 0x4008, 0x02);
    ov13b10_write_register(ViPipe, 0x4009, 0x0f);
    ov13b10_write_register(ViPipe, 0x400a, 0x01);
    ov13b10_write_register(ViPipe, 0x400b, 0x19);
    ov13b10_write_register(ViPipe, 0x4011, 0x21);
    ov13b10_write_register(ViPipe, 0x4017, 0x08);
    ov13b10_write_register(ViPipe, 0x4019, 0x04);
    ov13b10_write_register(ViPipe, 0x401a, 0x58);
    ov13b10_write_register(ViPipe, 0x4032, 0x1e);
    ov13b10_write_register(ViPipe, 0x4050, 0x02);
    ov13b10_write_register(ViPipe, 0x4051, 0x09);
    ov13b10_write_register(ViPipe, 0x405e, 0x00);
    ov13b10_write_register(ViPipe, 0x4066, 0x02);
    ov13b10_write_register(ViPipe, 0x4501, 0x00);
    ov13b10_write_register(ViPipe, 0x4502, 0x10);
    ov13b10_write_register(ViPipe, 0x4505, 0x00);
    ov13b10_write_register(ViPipe, 0x4800, 0x64);
    ov13b10_write_register(ViPipe, 0x481b, 0x3e);
    ov13b10_write_register(ViPipe, 0x481f, 0x30);
    ov13b10_write_register(ViPipe, 0x4825, 0x34);
    ov13b10_write_register(ViPipe, 0x4837, 0x0e);
    ov13b10_write_register(ViPipe, 0x484b, 0x01);
    ov13b10_write_register(ViPipe, 0x4883, 0x02);
    ov13b10_write_register(ViPipe, 0x5000, 0xff);
    ov13b10_write_register(ViPipe, 0x5001, 0x0f);
    ov13b10_write_register(ViPipe, 0x5045, 0x20);
    ov13b10_write_register(ViPipe, 0x5046, 0x20);
    ov13b10_write_register(ViPipe, 0x5047, 0xa4);
    ov13b10_write_register(ViPipe, 0x5048, 0x20);
    ov13b10_write_register(ViPipe, 0x5049, 0xa4);

    /* Have to set PLL_CTRL_5 twice! */
    ov13b10_write_register(ViPipe, 0x0305, 0x23); /* PLL_CTRL_5 pll1_multiplier[7:0]  0x23=35: x35 */
    ov13b10_write_register(ViPipe, 0x4837, 0x1d);
    ov13b10_write_register(ViPipe, 0x3016, 0x32); /* MIPI_SC_CTRL (Bit[7:5] mipi_lane_mode, N+1 lane) 2lane (Bit[4] mipi_en) */
    ov13b10_write_register(ViPipe, 0x3106, 0x29);
    ov13b10_write_register(ViPipe, 0x380c, 0x12); /* HTS[14:8] 0x1260=4704 */
    ov13b10_write_register(ViPipe, 0x380d, 0x60); /* HTS[ 7:0] */

    ov13b10_write_register(ViPipe, 0x0100, 0x01); /* MODEL_SELECT[0] 0: software_standby; 1: streaming */

    printf("===================================================================\n");
    printf("== OV13B10_MIPI_24MInput_560Mbps_2lane_10bit_4096x3120_1fps      ==\n");
    printf("===================================================================\n");
    return SC_SUCCESS;
}

/* 1080P@30fps LINEAR; Raw:10Bit; MCLK Input: 24MHz; MIPI CLK: 560Mbps/lane; CSI-2 4Lane */
int ov13b10_linear_1080P30_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    /* SOFTWARE_RST */
    ret = ov13b10_write_register(ViPipe, 0x0103, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write ov13b10 register error!\n");
        return SC_FAILURE;
    }

    /* PLL */
    ov13b10_write_register(ViPipe, 0x0301, 0x80); /* PLL_CTRL_1 Bit[6] pll1_predivp   0:   /1 */
    ov13b10_write_register(ViPipe, 0x0303, 0x01); /* PLL_CTRL_3 Bit[2:0] pll1_prediv  001: /1.5 */
    ov13b10_write_register(ViPipe, 0x0304, 0x00); /* PLL_CTRL_4 pll1_multiplier[9:8]  0x23=35: x35  */
    ov13b10_write_register(ViPipe, 0x0305, 0x23); /* PLL_CTRL_5 pll1_multiplier[7:0] */
    ov13b10_write_register(ViPipe, 0x0306, 0x00); /* PLL_CTRL_6 Bit[1:0] pll1_divpix  00:  /1 */

    ov13b10_write_register(ViPipe, 0x0321, 0x00);
    ov13b10_write_register(ViPipe, 0x0323, 0x04);
    ov13b10_write_register(ViPipe, 0x0324, 0x01);
    ov13b10_write_register(ViPipe, 0x0325, 0x50);
    ov13b10_write_register(ViPipe, 0x0326, 0x81);
    ov13b10_write_register(ViPipe, 0x0327, 0x04);
    ov13b10_write_register(ViPipe, 0x3011, 0x7c);
    ov13b10_write_register(ViPipe, 0x3012, 0x07);
    ov13b10_write_register(ViPipe, 0x3013, 0x32);
    ov13b10_write_register(ViPipe, 0x3016, 0x72); /* MIPI_SC_CTRL (Bit[7:5] mipi_lane_mode, N+1 lane) 4lane (Bit[4] mipi_en) */
    ov13b10_write_register(ViPipe, 0x3107, 0x23);
    ov13b10_write_register(ViPipe, 0x3501, 0x06);
    ov13b10_write_register(ViPipe, 0x3502, 0x00);
    ov13b10_write_register(ViPipe, 0x3504, 0x08);
    ov13b10_write_register(ViPipe, 0x3508, 0x07);
    ov13b10_write_register(ViPipe, 0x3509, 0xc0);
    ov13b10_write_register(ViPipe, 0x3600, 0x16);
    ov13b10_write_register(ViPipe, 0x3601, 0x54);
    ov13b10_write_register(ViPipe, 0x3612, 0x4e);
    ov13b10_write_register(ViPipe, 0x3620, 0x00);
    ov13b10_write_register(ViPipe, 0x3621, 0x68);
    ov13b10_write_register(ViPipe, 0x3622, 0x66);
    ov13b10_write_register(ViPipe, 0x3623, 0x03);
    ov13b10_write_register(ViPipe, 0x3662, 0x88);
    ov13b10_write_register(ViPipe, 0x3666, 0xbb);
    ov13b10_write_register(ViPipe, 0x3667, 0x44);
    ov13b10_write_register(ViPipe, 0x366e, 0xff);
    ov13b10_write_register(ViPipe, 0x366f, 0xf3);
    ov13b10_write_register(ViPipe, 0x3675, 0x44);
    ov13b10_write_register(ViPipe, 0x3676, 0x00);
    ov13b10_write_register(ViPipe, 0x367f, 0xe9);
    ov13b10_write_register(ViPipe, 0x3681, 0x32);
    ov13b10_write_register(ViPipe, 0x3682, 0x1f);
    ov13b10_write_register(ViPipe, 0x3683, 0x0b);
    ov13b10_write_register(ViPipe, 0x3684, 0x0b);
    ov13b10_write_register(ViPipe, 0x3704, 0x0f);
    ov13b10_write_register(ViPipe, 0x3706, 0x40);
    ov13b10_write_register(ViPipe, 0x3708, 0x3b);
    ov13b10_write_register(ViPipe, 0x3709, 0x72);
    ov13b10_write_register(ViPipe, 0x370b, 0xa2);
    ov13b10_write_register(ViPipe, 0x3714, 0x28);
    ov13b10_write_register(ViPipe, 0x371a, 0x3e);
    ov13b10_write_register(ViPipe, 0x3725, 0x42);
    ov13b10_write_register(ViPipe, 0x3739, 0x10);
    ov13b10_write_register(ViPipe, 0x3767, 0x00);
    ov13b10_write_register(ViPipe, 0x377a, 0x0d);
    ov13b10_write_register(ViPipe, 0x3789, 0x18);
    ov13b10_write_register(ViPipe, 0x3790, 0x40);
    ov13b10_write_register(ViPipe, 0x3791, 0xa2);
    ov13b10_write_register(ViPipe, 0x37c2, 0x14);
    ov13b10_write_register(ViPipe, 0x37c3, 0xf1);
    ov13b10_write_register(ViPipe, 0x37d9, 0x06);
    ov13b10_write_register(ViPipe, 0x37da, 0x02);
    ov13b10_write_register(ViPipe, 0x37dc, 0x02);
    ov13b10_write_register(ViPipe, 0x37e1, 0x04);
    ov13b10_write_register(ViPipe, 0x37e2, 0x0c);
    ov13b10_write_register(ViPipe, 0x37e4, 0x00);
    ov13b10_write_register(ViPipe, 0x3800, 0x00); /* X_ADDR_START[15:8] 0x00B0=176 */
    ov13b10_write_register(ViPipe, 0x3801, 0xb0); /* X_ADDR_START[ 7:0] */
    ov13b10_write_register(ViPipe, 0x3802, 0x01); /* Y_ADDR_START[15:8] 0x01E0=480 */
    ov13b10_write_register(ViPipe, 0x3803, 0xe0); /* Y_ADDR_START[ 7:0] */
    ov13b10_write_register(ViPipe, 0x3804, 0x0f); /* X_ADDR_END[15:8] 0x0FDF=4063 */
    ov13b10_write_register(ViPipe, 0x3805, 0xdf); /* X_ADDR_END[ 7:0] */
    ov13b10_write_register(ViPipe, 0x3806, 0x0a); /* Y_ADDR_END[15:8] 0x0A6F=2671 */
    ov13b10_write_register(ViPipe, 0x3807, 0x6f); /* Y_ADDR_END[ 7:0] */
    ov13b10_write_register(ViPipe, 0x3808, 0x07); /* X_OUTPUT_SIZE[15:8] 0x0780=1920 */
    ov13b10_write_register(ViPipe, 0x3809, 0x80); /* X_OUTPUT_SIZE[ 7:0] */
    ov13b10_write_register(ViPipe, 0x380a, 0x04); /* Y_OUTPUT_SIZE[15:8] 0x0438=1080 */
    ov13b10_write_register(ViPipe, 0x380b, 0x38); /* Y_OUTPUT_SIZE[ 7:0] */
    ov13b10_write_register(ViPipe, 0x380c, 0x04); /* HTS[14:8] 0x498=1176 */
    ov13b10_write_register(ViPipe, 0x380d, 0x98); /* HTS[ 7:0] */
    ov13b10_write_register(ViPipe, 0x380e, 0x0c); /* VTS[14:8] 0xc68=3176 */
    ov13b10_write_register(ViPipe, 0x380f, 0x68); /* VTS[ 0:0] */
    ov13b10_write_register(ViPipe, 0x3811, 0x0b);
    ov13b10_write_register(ViPipe, 0x3813, 0x08);
    ov13b10_write_register(ViPipe, 0x3814, 0x03);
    ov13b10_write_register(ViPipe, 0x3815, 0x01);
    ov13b10_write_register(ViPipe, 0x3816, 0x03);
    ov13b10_write_register(ViPipe, 0x3817, 0x01);
    ov13b10_write_register(ViPipe, 0x381f, 0x08);
    ov13b10_write_register(ViPipe, 0x3820, 0x8b);
    ov13b10_write_register(ViPipe, 0x3821, 0x00);
    ov13b10_write_register(ViPipe, 0x3822, 0x14);
    ov13b10_write_register(ViPipe, 0x3823, 0x18);
    ov13b10_write_register(ViPipe, 0x3827, 0x01);
    ov13b10_write_register(ViPipe, 0x382e, 0xe6);
    ov13b10_write_register(ViPipe, 0x3c80, 0x00);
    ov13b10_write_register(ViPipe, 0x3c87, 0x01);
    ov13b10_write_register(ViPipe, 0x3c8c, 0x18);
    ov13b10_write_register(ViPipe, 0x3c8d, 0x1c);
    ov13b10_write_register(ViPipe, 0x3ca0, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca1, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca2, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca3, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca4, 0x50);
    ov13b10_write_register(ViPipe, 0x3ca5, 0x11);
    ov13b10_write_register(ViPipe, 0x3ca6, 0x01);
    ov13b10_write_register(ViPipe, 0x3ca7, 0x00);
    ov13b10_write_register(ViPipe, 0x3ca8, 0x00);
    ov13b10_write_register(ViPipe, 0x4008, 0x00);
    ov13b10_write_register(ViPipe, 0x4009, 0x05);
    ov13b10_write_register(ViPipe, 0x400a, 0x01);
    ov13b10_write_register(ViPipe, 0x400b, 0x19);
    ov13b10_write_register(ViPipe, 0x4011, 0x21);
    ov13b10_write_register(ViPipe, 0x4017, 0x08);
    ov13b10_write_register(ViPipe, 0x4019, 0x04);
    ov13b10_write_register(ViPipe, 0x401a, 0x58);
    ov13b10_write_register(ViPipe, 0x4032, 0x1e);
    ov13b10_write_register(ViPipe, 0x4050, 0x00);
    ov13b10_write_register(ViPipe, 0x4051, 0x05);
    ov13b10_write_register(ViPipe, 0x405e, 0x00);
    ov13b10_write_register(ViPipe, 0x4066, 0x02);
    ov13b10_write_register(ViPipe, 0x4501, 0x08);
    ov13b10_write_register(ViPipe, 0x4502, 0x10);
    ov13b10_write_register(ViPipe, 0x4505, 0x04);
    ov13b10_write_register(ViPipe, 0x4800, 0x64);
    ov13b10_write_register(ViPipe, 0x481b, 0x3e);
    ov13b10_write_register(ViPipe, 0x481f, 0x30);
    ov13b10_write_register(ViPipe, 0x4825, 0x34);
    ov13b10_write_register(ViPipe, 0x4837, 0x1d);
    ov13b10_write_register(ViPipe, 0x484b, 0x01);
    ov13b10_write_register(ViPipe, 0x4883, 0x02);
    ov13b10_write_register(ViPipe, 0x5000, 0xfd);
    ov13b10_write_register(ViPipe, 0x5001, 0x0d);
    ov13b10_write_register(ViPipe, 0x5045, 0x20);
    ov13b10_write_register(ViPipe, 0x5046, 0x20);
    ov13b10_write_register(ViPipe, 0x5047, 0xa4);
    ov13b10_write_register(ViPipe, 0x5048, 0x20);
    ov13b10_write_register(ViPipe, 0x5049, 0xa4);

    ov13b10_write_register(ViPipe, 0x0100, 0x01); /* MODEL_SELECT[0] 0: software_standby; 1: streaming */

    printf("===================================================================\n");
    printf("== OV13B10_MIPI_24MInput_560Mbps_4lane_10bit_1920x1080_30fps    ==\n");
    printf("===================================================================\n");
    return SC_SUCCESS;
}
