/**
 * @file     imx291_sensor_ctl.c
 * @brief    SENSOR初始化接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2023-02-17 创建文件
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


#define IMX291_FLIP_MIRROR   (0x3007)
#define IMX291_1080P_FPS_30  (30)
#define IMX291_1080P_FPS_60  (60)
#define IMX291_1080P_FPS_120 (120)

#define SENSOR_1080P_120FPS_LINEAR_MODE (1)

const unsigned char imx291_i2c_addr  = 0x34;  /* I2C Address of IMX291 */
const unsigned int  imx291_addr_byte = 2;
const unsigned int  imx291_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

extern ISP_SNS_STATE_S   *g_pastImx291[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U  g_aunImx291BusInfo[ISP_MAX_PIPE_NUM];
extern const unsigned int g_imx291_fps_mode;

int imx291_linear_1080p30_init(VI_PIPE ViPipe);
int imx291_linear_1080p60_2lane_init(VI_PIPE ViPipe);
int imx291_linear_1080p60_4lane_init(VI_PIPE ViPipe);
int imx291_linear_1080p120_init(VI_PIPE ViPipe);


int imx291_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {
        return SC_SUCCESS;
    }

    u8DevNum = g_aunImx291BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (imx291_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int imx291_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int imx291_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:
    return SC_SUCCESS;
}

int imx291_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(imx291_addr_byte == 2)
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

    if(imx291_data_byte == 2)
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

    ret = write(g_fd[ViPipe], buf, imx291_addr_byte + imx291_data_byte);
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

void imx291_prog(VI_PIPE ViPipe, int *rom)
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
            imx291_write_register(ViPipe, addr, data);
        }
    }
}

void imx291_standby(VI_PIPE ViPipe)
{
    imx291_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
    imx291_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */

    return;
}

void imx291_restart(VI_PIPE ViPipe)
{
    imx291_write_register(ViPipe, 0x3000, 0x00); /* standby */
    delay_ms(20);
    imx291_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
    imx291_write_register(ViPipe, 0x304b, 0x0a);

    return;
}

int imx291_flip_off_mirror_off(VI_PIPE ViPipe)
{
    imx291_write_register(ViPipe, IMX291_FLIP_MIRROR, 0x00);
    return SC_SUCCESS;
}

int imx291_flip_on_mirror_off(VI_PIPE ViPipe)
{
    imx291_write_register(ViPipe, IMX291_FLIP_MIRROR, 0x01);
    return SC_SUCCESS;
}

int imx291_flip_off_mirror_on(VI_PIPE ViPipe)
{
    imx291_write_register(ViPipe, IMX291_FLIP_MIRROR, 0x02);
    return SC_SUCCESS;
}

int imx291_flip_on_mirror_on(VI_PIPE ViPipe)
{
    imx291_write_register(ViPipe, IMX291_FLIP_MIRROR, 0x03);
    return SC_SUCCESS;
}

void imx291_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i = 0;

    for(i = 0; i < g_pastImx291[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastImx291[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            imx291_write_register(ViPipe,
                g_pastImx291[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                g_pastImx291[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int imx291_init(VI_PIPE ViPipe)
{
    WDR_MODE_E enWDRMode = WDR_MODE_NONE;
    SC_BOOL    bInit     = SC_FALSE;
    SC_U8      u8ImgMode = 0;
    SC_S32     ret       = SC_FAILURE;

    bInit     = g_pastImx291[ViPipe]->bInit;
    enWDRMode = g_pastImx291[ViPipe]->enWDRMode;
    u8ImgMode = g_pastImx291[ViPipe]->u8ImgMode;
    printf("IMX291: bInit[%d] enWDRMode[%d] u8ImgMode[%u]\n", bInit, enWDRMode, (SC_U32)u8ImgMode);

    if(SC_FALSE == bInit)
    {
        ret = imx291_i2c_init(ViPipe);
        if(SC_SUCCESS != ret)
        {
            printf("Init imx291 i2c error!\n");
            return SC_FAILURE;
        }
    }

    /* When sensor first init, config all registers */
    switch(u8ImgMode)
    {
        case SENSOR_1080P_120FPS_LINEAR_MODE:
        {
            if(IMX291_1080P_FPS_120 == g_imx291_fps_mode)
            {
                ret = imx291_linear_1080p120_init(ViPipe);
            }
            else if(IMX291_1080P_FPS_60 == g_imx291_fps_mode)
            {
                ret = imx291_linear_1080p60_4lane_init(ViPipe);
            }
            else
            {
                ret = imx291_linear_1080p30_init(ViPipe);
            }

            if(SC_SUCCESS != ret)
            {
                printf("Init imx291 linear mode error!\n");
                return SC_FAILURE;
            }

            g_pastImx291[ViPipe]->bInit = SC_TRUE;
            break;
        }

        default:
        {
            printf("Not support this mode!\n");
            g_pastImx291[ViPipe]->bInit = SC_FALSE;
            break;
        }
    }

    return SC_SUCCESS;
}

void imx291_exit(VI_PIPE ViPipe)
{
    imx291_i2c_exit(ViPipe);

    return;
}


/* 1080P30 and 1080P25: CSI-2 2LANE 12BIT */
int imx291_linear_1080p30_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

     /* Enter standby */
    ret = imx291_write_register(ViPipe, 0x3000, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write imx291 register error!\n");
        return SC_FAILURE;
    }

    imx291_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */

    /* Mode register setting */
    imx291_write_register(ViPipe, 0x3005, 0x01); /* ADBIT: 12bit */
    imx291_write_register(ViPipe, 0x3007, 0x00);
    imx291_write_register(ViPipe, 0x3009, 0x02); /* FRSEL: 30fps */
    imx291_write_register(ViPipe, 0x300a, 0xf0); /* BLKLEVEL */
    imx291_write_register(ViPipe, 0x3011, 0x0a);

    imx291_write_register(ViPipe, 0x3018, 0x46); /* VMAX: 1125(30fps); 1350(25fps) */
    imx291_write_register(ViPipe, 0x3019, 0x05); /* VMAX */
    imx291_write_register(ViPipe, 0x301c, 0x30); /* HMAX: 4400 */
    imx291_write_register(ViPipe, 0x301d, 0x11); /* HMAX */

    imx291_write_register(ViPipe, 0x3046, 0x01); /* ODBIT[1:0] Number of output bit setting 0: 10 bit; 1: 12 bit */
    imx291_write_register(ViPipe, 0x305c, 0x18); /* INCKSEL1: 1080P CSI-2 37.125MHz->0x18; 74.25MHz->0x0c */
    imx291_write_register(ViPipe, 0x305d, 0x03); /* INCKSEL2: 1080P CSI-2 37.125MHz->0x03; 74.25MHz->0x03 */
    imx291_write_register(ViPipe, 0x305e, 0x20); /* INCKSEL3: 1080P CSI-2 37.125MHz->0x20; 74.25MHz->0x10 */
    imx291_write_register(ViPipe, 0x305f, 0x01); /* INCKSEL4: 1080P CSI-2 37.125MHz->0x01; 74.25MHz->0x01 */
    imx291_write_register(ViPipe, 0x309e, 0x4a);
    imx291_write_register(ViPipe, 0x309f, 0x4a);
    imx291_write_register(ViPipe, 0x311c, 0x0e);
    imx291_write_register(ViPipe, 0x3128, 0x04);
    imx291_write_register(ViPipe, 0x3129, 0x00); /* ADBIT1: 12Bit */
    imx291_write_register(ViPipe, 0x313b, 0x41);
    imx291_write_register(ViPipe, 0x315e, 0x1a); /* INCKSEL5: 1080P CSI-2 37.125MHz->0x1a; 74.25MHz->0x1b */
    imx291_write_register(ViPipe, 0x3164, 0x1a); /* INCKSEL6: 1080P CSI-2 37.125MHz->0x1a; 74.25MHz->0x1b */
    imx291_write_register(ViPipe, 0x3480, 0x49); /* INCKSEL7: 1080P CSI-2 37.125MHz->0x49; 74.25MHz->0x92 */
    imx291_write_register(ViPipe, 0x317c, 0x00); /* ADBIT2: 12Bit */
    imx291_write_register(ViPipe, 0x31ec, 0x0e); /* ADBIT3: 12Bit */

    /* MIPI setting */
    imx291_write_register(ViPipe, 0x3405, 0x10); /* REPETITION */
    imx291_write_register(ViPipe, 0x3407, 0x01); /* PHYSICAL LANE NUM: 2lane */
    imx291_write_register(ViPipe, 0x3414, 0x0a); /* OPB_SEZI_V */
    imx291_write_register(ViPipe, 0x3418, 0x38); /* Y_OUT_SIZE */
    imx291_write_register(ViPipe, 0x3419, 0x04); /* Y_OUT_SIZE */
    imx291_write_register(ViPipe, 0x3441, 0x0c); /* CSI_DT_FMT: 12bit */
    imx291_write_register(ViPipe, 0x3442, 0x0c); /* CSI_DT_FMT: 12bit */
    imx291_write_register(ViPipe, 0x3443, 0x01); /* CSI_LANE_MODE: mipi 2lane->1; 4lane->3 */
    imx291_write_register(ViPipe, 0x3444, 0x20); /* EXTCK_FREQ */
    imx291_write_register(ViPipe, 0x3445, 0x25);
    imx291_write_register(ViPipe, 0x3446, 0x57);
    imx291_write_register(ViPipe, 0x3447, 0x00);
    imx291_write_register(ViPipe, 0x3448, 0x37);
    imx291_write_register(ViPipe, 0x3449, 0x00);
    imx291_write_register(ViPipe, 0x344A, 0x1F); /* THSPREPARE */
    imx291_write_register(ViPipe, 0x344B, 0x00);
    imx291_write_register(ViPipe, 0x344C, 0x1F);
    imx291_write_register(ViPipe, 0x344D, 0x00);
    imx291_write_register(ViPipe, 0x344E, 0x1F); /* THSTRAIL */
    imx291_write_register(ViPipe, 0x344F, 0x00);
    imx291_write_register(ViPipe, 0x3450, 0x77); /* TCLKZERO */
    imx291_write_register(ViPipe, 0x3451, 0x00);
    imx291_write_register(ViPipe, 0x3452, 0x1F); /* TCLKPREPARE */
    imx291_write_register(ViPipe, 0x3453, 0x00);
    imx291_write_register(ViPipe, 0x3454, 0x17); /* TIPX */
    imx291_write_register(ViPipe, 0x3455, 0x00);
    imx291_write_register(ViPipe, 0x3472, 0x80); /* X_OUT_SIZE */
    imx291_write_register(ViPipe, 0x3473, 0x07); /* X_OUT_SIZE */

    imx291_default_reg_init(ViPipe);

    imx291_write_register(ViPipe, 0x3000, 0x00); /* Standby cancel */
    usleep(20000);
    imx291_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
    imx291_write_register(ViPipe, 0x304B, 0x0a); /* XVSOUTSEL XHSOUTSEL enable output */

    printf("===================================================================\n");
    printf("== IMX291_MIPI_37.125MInput_450Mbps_2lane_12bit_1920x1080_30fps  ==\n");
    printf("===================================================================\n");
    return SC_SUCCESS;
}


/* 1080P60 and 1080P50: CSI-2 2LANE 10BIT */
int imx291_linear_1080p60_2lane_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = imx291_write_register(ViPipe, 0x3000, 0x01); /* Enter standby */
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write imx291 register error!\n");
        return SC_FAILURE;
    }

    imx291_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA  Stop master mode */

    /* Mode register setting */
    imx291_write_register(ViPipe, 0x3005, 0x00); /* ADBIT[0]    0: 10bit; 1: 12bit */
    imx291_write_register(ViPipe, 0x3007, 0x00); /* WINMODE[6:4] 0: Full HD 1080P */
    imx291_write_register(ViPipe, 0x3009, 0x01); /* FDG_SEL[4]   0: LCG Mode; 1: HCG Mode; FRSEL[1:0] 1: 60fps; 2: 30fps */
    imx291_write_register(ViPipe, 0x300A, 0x3C); /* BLKLEVEL[8:0] 0x3C: 10bit; 0xF0: 12bit output */
    imx291_write_register(ViPipe, 0x300B, 0x00); /* BLKLEVEL[8:0] */

    imx291_write_register(ViPipe, 0x3018, 0x65); /* VMAX[17:0] 0x465=1125: 60fps */
    imx291_write_register(ViPipe, 0x3019, 0x04); /* VMAX[17:0] */
    imx291_write_register(ViPipe, 0x301A, 0x00); /* VMAX[17:0] */
    imx291_write_register(ViPipe, 0x301C, 0x98); /* HMAX[15:0] 0x898=2200: 60fps; 0xA50=2640: 50fps */
    imx291_write_register(ViPipe, 0x301D, 0x08); /* HMAX[15:0] */

    imx291_write_register(ViPipe, 0x3046, 0x00); /* ODBIT[1:0] Number of output bit setting 0: 10 bit; 1: 12 bit */
    imx291_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1: 1080P CSI-2 37.125MHz->0x18; 74.25MHz->0x0C */
    imx291_write_register(ViPipe, 0x305D, 0x03); /* INCKSEL2: 1080P CSI-2 37.125MHz->0x03; 74.25MHz->0x03 */
    imx291_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3: 1080P CSI-2 37.125MHz->0x20; 74.25MHz->0x10 */
    imx291_write_register(ViPipe, 0x305F, 0x01); /* INCKSEL4: 1080P CSI-2 37.125MHz->0x01; 74.25MHz->0x01 */
    imx291_write_register(ViPipe, 0x311C, 0x1E); /* Set to 0x1E */
    imx291_write_register(ViPipe, 0x3128, 0x05); /* Set to 0x05 */
    imx291_write_register(ViPipe, 0x3129, 0x1D); /* ADBIT1[7:0] 0x1D: 10bit; 0x00: 12bit */
    imx291_write_register(ViPipe, 0x315E, 0x1A); /* INCKSEL5: 1080P CSI-2 37.125MHz->0x1A; 74.25MHz->0x1B */
    imx291_write_register(ViPipe, 0x3164, 0x1A); /* INCKSEL6: 1080P CSI-2 37.125MHz->0x1A; 74.25MHz->0x1B */
    imx291_write_register(ViPipe, 0x3480, 0x49); /* INCKSEL7: 1080P CSI-2 37.125MHz->0x49; 74.25MHz->0x92 */
    imx291_write_register(ViPipe, 0x317C, 0x12); /* ADBIT2[7:0] 0x12: 10bit; 0x00: 12bit */
    imx291_write_register(ViPipe, 0x31EC, 0x37); /* ADBIT3[7:0] 0x37: 10bit; 0x0E: 12bit */

    /* Mipi setting */
    imx291_write_register(ViPipe, 0x3405, 0x00); /* REPETITION[5:4] 0x00: 60/50fps */
    imx291_write_register(ViPipe, 0x3407, 0x01); /* PHYSICAL LANE NUM: 0x01: 2lane; 0x03: 4lane */
    imx291_write_register(ViPipe, 0x3414, 0x0A); /* OPB_SIZE_V */
    imx291_write_register(ViPipe, 0x3418, 0x38); /* Y_OUT_SIZE 0x438=1080 */
    imx291_write_register(ViPipe, 0x3419, 0x04); /* Y_OUT_SIZE */
    imx291_write_register(ViPipe, 0x3441, 0x0A); /* CSI_DT_FMT[15:0] 0x0A0A: RAW10; 0x0C0C: RAW12 */
    imx291_write_register(ViPipe, 0x3442, 0x0A); /* CSI_DT_FMT[15:0] */
    imx291_write_register(ViPipe, 0x3443, 0x01); /* CSI_LANE_MODE: 0x01: 2lane; 0x03: 4lane */
    imx291_write_register(ViPipe, 0x3444, 0x20); /* EXTCK_FREQ[15:0] 0x2520: 37.125MHz; 0x4A40: 74.25HHz */
    imx291_write_register(ViPipe, 0x3445, 0x25); /* EXTCK_FREQ[15:0] */
    imx291_write_register(ViPipe, 0x3446, 0x77); /* TCLKPOST[8:0] */
    imx291_write_register(ViPipe, 0x3447, 0x00); /* TCLKPOST[8:0] */
    imx291_write_register(ViPipe, 0x3448, 0x67); /* THSZERO[8:0] */
    imx291_write_register(ViPipe, 0x3449, 0x00); /* THSZERO[8:0] */
    imx291_write_register(ViPipe, 0x344A, 0x47); /* THSPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x344B, 0x00); /* THSPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x344C, 0x37); /* TCLKTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344D, 0x00); /* TCLKTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344E, 0x3F); /* THSTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344F, 0x00); /* THSTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x3450, 0xFF); /* TCLKZERO[8:0] */
    imx291_write_register(ViPipe, 0x3451, 0x00); /* TCLKZERO[8:0] */
    imx291_write_register(ViPipe, 0x3452, 0x3F); /* TCLKPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x3453, 0x00); /* TCLKPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x3454, 0x37); /* TLPX[8:0] */
    imx291_write_register(ViPipe, 0x3455, 0x00); /* TLPX[8:0] */
    imx291_write_register(ViPipe, 0x3472, 0x80); /* X_OUT_SIZE 0x780=1920 */
    imx291_write_register(ViPipe, 0x3473, 0x07); /* X_OUT_SIZE */

    imx291_default_reg_init(ViPipe);

    imx291_write_register(ViPipe, 0x3000, 0x00); /* Standby cancel */
    usleep(20000);
    imx291_write_register(ViPipe, 0x3002, 0x00); /* XTMSTA  Start master mode */
    imx291_write_register(ViPipe, 0x304B, 0x0A); /* XVSOUTSEL XHSOUTSEL enable output */

    printf("===================================================================\n");
    printf("== IMX291_MIPI_37.125MInput_891Mbps_2lane_10bit_1920x1080_60fps  ==\n");
    printf("===================================================================\n");
    return SC_SUCCESS;
}

/* 1080P60 and 1080P50: CSI-2 4LANE 10BIT */
int imx291_linear_1080p60_4lane_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = imx291_write_register(ViPipe, 0x3000, 0x01); /* Enter standby */
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write imx291 register error!\n");
        return SC_FAILURE;
    }

    imx291_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA  Stop master mode */

    /* Mode register setting */
    imx291_write_register(ViPipe, 0x3005, 0x00); /* ADBIT[0]    0: 10bit; 1: 12bit */
    imx291_write_register(ViPipe, 0x3007, 0x00); /* WINMODE[6:4] 0: Full HD 1080P */
    imx291_write_register(ViPipe, 0x3009, 0x01); /* FDG_SEL[4]   0: LCG Mode; 1: HCG Mode; FRSEL[1:0] 1: 60fps; 2: 30fps */
    imx291_write_register(ViPipe, 0x300A, 0x3C); /* BLKLEVEL[8:0] 0x3C: 10bit; 0xF0: 12bit output */
    imx291_write_register(ViPipe, 0x300B, 0x00); /* BLKLEVEL[8:0] */

    imx291_write_register(ViPipe, 0x3018, 0x65); /* VMAX[17:0] 0x465=1125: 60fps */
    imx291_write_register(ViPipe, 0x3019, 0x04); /* VMAX[17:0] */
    imx291_write_register(ViPipe, 0x301A, 0x00); /* VMAX[17:0] */
    imx291_write_register(ViPipe, 0x301C, 0x98); /* HMAX[15:0] 0x898=2200: 60fps; 0xA50=2640: 50fps */
    imx291_write_register(ViPipe, 0x301D, 0x08); /* HMAX[15:0] */

    imx291_write_register(ViPipe, 0x3046, 0x00); /* ODBIT[1:0] Number of output bit setting 0: 10 bit; 1: 12 bit */
    imx291_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1: 1080P CSI-2 37.125MHz->0x18; 74.25MHz->0x0C */
    imx291_write_register(ViPipe, 0x305D, 0x03); /* INCKSEL2: 1080P CSI-2 37.125MHz->0x03; 74.25MHz->0x03 */
    imx291_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3: 1080P CSI-2 37.125MHz->0x20; 74.25MHz->0x10 */
    imx291_write_register(ViPipe, 0x305F, 0x01); /* INCKSEL4: 1080P CSI-2 37.125MHz->0x01; 74.25MHz->0x01 */
    imx291_write_register(ViPipe, 0x311C, 0x1E); /* Set to 0x1E */
    imx291_write_register(ViPipe, 0x3128, 0x05); /* Set to 0x05 */
    imx291_write_register(ViPipe, 0x3129, 0x1D); /* ADBIT1[7:0] 0x1D: 10bit; 0x00: 12bit */
    imx291_write_register(ViPipe, 0x315E, 0x1A); /* INCKSEL5: 1080P CSI-2 37.125MHz->0x1A; 74.25MHz->0x1B */
    imx291_write_register(ViPipe, 0x3164, 0x1A); /* INCKSEL6: 1080P CSI-2 37.125MHz->0x1A; 74.25MHz->0x1B */
    imx291_write_register(ViPipe, 0x3480, 0x49); /* INCKSEL7: 1080P CSI-2 37.125MHz->0x49; 74.25MHz->0x92 */
    imx291_write_register(ViPipe, 0x317C, 0x12); /* ADBIT2[7:0] 0x12: 10bit; 0x00: 12bit */
    imx291_write_register(ViPipe, 0x31EC, 0x37); /* ADBIT3[7:0] 0x37: 10bit; 0x0E: 12bit */

    /* Mipi setting */
    imx291_write_register(ViPipe, 0x3405, 0x00); /* REPETITION[5:4] 0x00: 60/50fps */
    imx291_write_register(ViPipe, 0x3407, 0x03); /* PHYSICAL LANE NUM: 0x01: 2lane; 0x03: 4lane */
    imx291_write_register(ViPipe, 0x3414, 0x0A); /* OPB_SIZE_V */
    imx291_write_register(ViPipe, 0x3418, 0x38); /* Y_OUT_SIZE 0x438=1080 */
    imx291_write_register(ViPipe, 0x3419, 0x04); /* Y_OUT_SIZE */
    imx291_write_register(ViPipe, 0x3441, 0x0A); /* CSI_DT_FMT[15:0] 0x0A0A: RAW10; 0x0C0C: RAW12 */
    imx291_write_register(ViPipe, 0x3442, 0x0A); /* CSI_DT_FMT[15:0] */
    imx291_write_register(ViPipe, 0x3443, 0x03); /* CSI_LANE_MODE: 0x01: 2lane; 0x03: 4lane */
    imx291_write_register(ViPipe, 0x3444, 0x20); /* EXTCK_FREQ[15:0] 0x2520: 37.125MHz; 0x4A40: 74.25HHz */
    imx291_write_register(ViPipe, 0x3445, 0x25); /* EXTCK_FREQ[15:0] */
    imx291_write_register(ViPipe, 0x3446, 0x77); /* TCLKPOST[8:0] */
    imx291_write_register(ViPipe, 0x3447, 0x00); /* TCLKPOST[8:0] */
    imx291_write_register(ViPipe, 0x3448, 0x67); /* THSZERO[8:0] */
    imx291_write_register(ViPipe, 0x3449, 0x00); /* THSZERO[8:0] */
    imx291_write_register(ViPipe, 0x344A, 0x47); /* THSPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x344B, 0x00); /* THSPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x344C, 0x37); /* TCLKTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344D, 0x00); /* TCLKTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344E, 0x3F); /* THSTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344F, 0x00); /* THSTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x3450, 0xFF); /* TCLKZERO[8:0] */
    imx291_write_register(ViPipe, 0x3451, 0x00); /* TCLKZERO[8:0] */
    imx291_write_register(ViPipe, 0x3452, 0x3F); /* TCLKPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x3453, 0x00); /* TCLKPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x3454, 0x37); /* TLPX[8:0] */
    imx291_write_register(ViPipe, 0x3455, 0x00); /* TLPX[8:0] */
    imx291_write_register(ViPipe, 0x3472, 0x80); /* X_OUT_SIZE 0x780=1920 */
    imx291_write_register(ViPipe, 0x3473, 0x07); /* X_OUT_SIZE */

    imx291_default_reg_init(ViPipe);

    imx291_write_register(ViPipe, 0x3000, 0x00); /* Standby cancel */
    usleep(20000);
    imx291_write_register(ViPipe, 0x3002, 0x00); /* XTMSTA  Start master mode */
    imx291_write_register(ViPipe, 0x304B, 0x0A); /* XVSOUTSEL XHSOUTSEL enable output */

    printf("===================================================================\n");
    printf("== IMX291_MIPI_37.125MInput_445.5Mbps_4lane_10bit_1920x1080_60fps =\n");
    printf("===================================================================\n");
    return SC_SUCCESS;
}

/* 1080P120, 1080P60 and 1080P30: CSI-2 4LANE 10BIT */
int imx291_linear_1080p120_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = imx291_write_register(ViPipe, 0x3000, 0x01); /* Enter standby */
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write imx291 register error!\n");
        return SC_FAILURE;
    }

    imx291_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA  Stop master mode */

    /* Mode register setting */
    imx291_write_register(ViPipe, 0x3005, 0x00); /* ADBIT[0] 0: 10bit; 1: 12bit */
    imx291_write_register(ViPipe, 0x3007, 0x00); /* WINMODE[6:4] 0: Full HD 1080P */
    imx291_write_register(ViPipe, 0x3009, 0x00); /* FDG_SEL[4]  0: LCG Mode; 1: HCG Mode; FRSEL[1:0] 0: 120fps; 1: 60fps */
    imx291_write_register(ViPipe, 0x300A, 0x3C); /* BLKLEVEL[8:0] 0x3C: 10bit; 0xF0: 12bit output */
    imx291_write_register(ViPipe, 0x300B, 0x00); /* BLKLEVEL[8:0] */
    imx291_write_register(ViPipe, 0x300F, 0x00); /* Set to 0x00 */
    imx291_write_register(ViPipe, 0x3010, 0x21); /* Set to 0x21 */
    imx291_write_register(ViPipe, 0x3012, 0x64); /* Set to 0x64 */
    imx291_write_register(ViPipe, 0x3014, 0x14); /* GAIN[7:0] 0x14: 6dB */
    imx291_write_register(ViPipe, 0x3016, 0x09); /* Set to 0x09 */
    imx291_write_register(ViPipe, 0x3018, 0x65); /* VMAX[17:0] 0x465=1125: 120fps */
    imx291_write_register(ViPipe, 0x3019, 0x04); /* VMAX[17:0] 0x8CA=2250: 60fps; 0x1194=4500: 30fps */
    imx291_write_register(ViPipe, 0x301A, 0x00); /* VMAX[17:0] */
    imx291_write_register(ViPipe, 0x301C, 0x4C); /* HMAX[15:0] 0x44C=1100: 120fps; 0x528=1320: 100fps */
    imx291_write_register(ViPipe, 0x301D, 0x04); /* HMAX */
    imx291_write_register(ViPipe, 0x3020, 0xC1); /* SHS1[17:0] */
    imx291_write_register(ViPipe, 0x3021, 0x01); /* SHS1[17:0] */
    imx291_write_register(ViPipe, 0x3022, 0x00); /* SHS1[17:0] */
    imx291_write_register(ViPipe, 0x3046, 0x00); /* ODBIT[1:0] Number of output bit setting 0: 10 bit; 1: 12 bit */
    imx291_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1: 1080P CSI-2 37.125MHz->0x18; 74.25MHz->0x0C */
    imx291_write_register(ViPipe, 0x305D, 0x03); /* INCKSEL2: 1080P CSI-2 37.125MHz->0x03; 74.25MHz->0x03 */
    imx291_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3: 1080P CSI-2 37.125MHz->0x20; 74.25MHz->0x10 */
    imx291_write_register(ViPipe, 0x305F, 0x01); /* INCKSEL4: 1080P CSI-2 37.125MHz->0x01; 74.25MHz->0x01 */
    imx291_write_register(ViPipe, 0x3070, 0x02); /* Set to 0x02 */
    imx291_write_register(ViPipe, 0x3071, 0x11); /* Set to 0x11 */

    /* Pattern Generator  0x21: horizontal colorbar on; 0x31: vertical colorbar on */
    //imx291_write_register(ViPipe, 0x308C, 0x21);

    imx291_write_register(ViPipe, 0x309B, 0x10); /* Set to 0x10 */
    imx291_write_register(ViPipe, 0x309C, 0x22); /* Set to 0x22 */
    imx291_write_register(ViPipe, 0x30A2, 0x02); /* Set to 0x02 */
    imx291_write_register(ViPipe, 0x30A6, 0x20); /* Set to 0x20 */
    imx291_write_register(ViPipe, 0x30A8, 0x20); /* Set to 0x20 */
    imx291_write_register(ViPipe, 0x30AA, 0x20); /* Set to 0x20 */
    imx291_write_register(ViPipe, 0x30AC, 0x20); /* Set to 0x20 */
    imx291_write_register(ViPipe, 0x30B0, 0x43); /* Set to 0x43 */
    imx291_write_register(ViPipe, 0x3119, 0x9E); /* Set to 0x9E */
    imx291_write_register(ViPipe, 0x311C, 0x1E); /* Set to 0x1E */
    imx291_write_register(ViPipe, 0x311E, 0x08); /* Set to 0x08 */
    imx291_write_register(ViPipe, 0x3128, 0x05); /* Set to 0x05 */
    imx291_write_register(ViPipe, 0x3129, 0x1D); /* ADBIT1[7:0] 0x1D: 10bit; 0x00: 12bit */
    imx291_write_register(ViPipe, 0x313D, 0x83); /* Set to 0x83 */
    imx291_write_register(ViPipe, 0x3150, 0x03); /* Set to 0x03 */
    imx291_write_register(ViPipe, 0x315E, 0x1A); /* INCKSEL5: 1080P CSI-2 37.125MHz->0x1A; 74.25MHz->0x1B */
    imx291_write_register(ViPipe, 0x3164, 0x1A); /* INCKSEL6: 1080P CSI-2 37.125MHz->0x1A; 74.25MHz->0x1B */
    imx291_write_register(ViPipe, 0x317C, 0x12); /* ADBIT2[7:0] 0x12: 10bit; 0x00: 12bit */
    imx291_write_register(ViPipe, 0x317E, 0x00); /* Set to 0x00 */
    imx291_write_register(ViPipe, 0x31EC, 0x37); /* ADBIT3[7:0] 0x37: 10bit; 0x0E: 12bit */

    imx291_write_register(ViPipe, 0x32B8, 0x50); /* Set to 0x50 */
    imx291_write_register(ViPipe, 0x32B9, 0x10); /* Set to 0x10 */
    imx291_write_register(ViPipe, 0x32BA, 0x00); /* Set to 0x00 */
    imx291_write_register(ViPipe, 0x32BB, 0x04); /* Set to 0x04 */
    imx291_write_register(ViPipe, 0x32C8, 0x50); /* Set to 0x50 */
    imx291_write_register(ViPipe, 0x32C9, 0x10); /* Set to 0x10 */
    imx291_write_register(ViPipe, 0x32CA, 0x00); /* Set to 0x00 */
    imx291_write_register(ViPipe, 0x32CB, 0x04); /* Set to 0x04 */
    imx291_write_register(ViPipe, 0x332C, 0xD3); /* Set to 0xD3 */
    imx291_write_register(ViPipe, 0x332D, 0x10); /* Set to 0x10 */
    imx291_write_register(ViPipe, 0x332E, 0x0D); /* Set to 0x0D */
    imx291_write_register(ViPipe, 0x3358, 0x06); /* Set to 0x06 */
    imx291_write_register(ViPipe, 0x3359, 0xE1); /* Set to 0xE1 */
    imx291_write_register(ViPipe, 0x335A, 0x11); /* Set to 0x11 */
    imx291_write_register(ViPipe, 0x3360, 0x1E); /* Set to 0x1E */
    imx291_write_register(ViPipe, 0x3361, 0x61); /* Set to 0x61 */
    imx291_write_register(ViPipe, 0x3362, 0x10); /* Set to 0x10 */
    imx291_write_register(ViPipe, 0x33B0, 0x50); /* Set to 0x50 */
    imx291_write_register(ViPipe, 0x33B2, 0x1A); /* Set to 0x1A */
    imx291_write_register(ViPipe, 0x33B3, 0x04); /* Set to 0x04 */

    /* Mipi setting */
    imx291_write_register(ViPipe, 0x3405, 0x00); /* REPETITION[5:4]  0x00: 120/100fps */
    imx291_write_register(ViPipe, 0x3407, 0x03); /* PHYSICAL LANE NUM: 0x01: 2lane; 0x03: 4lane */
    imx291_write_register(ViPipe, 0x3414, 0x0A); /* OPB_SIZE_V */
    imx291_write_register(ViPipe, 0x3418, 0x38); /* Y_OUT_SIZE 0x438=1080 */
    imx291_write_register(ViPipe, 0x3419, 0x04); /* Y_OUT_SIZE */
    imx291_write_register(ViPipe, 0x3441, 0x0A); /* CSI_DT_FMT[15:0] 0x0A0A: RAW10; 0x0C0C: RAW12 */
    imx291_write_register(ViPipe, 0x3442, 0x0A); /* CSI_DT_FMT[15:0] */
    imx291_write_register(ViPipe, 0x3443, 0x03); /* CSI_LANE_MODE  0x01: 2lane; 0x03: 4lane */
    imx291_write_register(ViPipe, 0x3444, 0x20); /* EXTCK_FREQ[15:0] 0x2520: 37.125MHz; 0x4A40: 74.25HHz */
    imx291_write_register(ViPipe, 0x3445, 0x25); /* EXTCK_FREQ[15:0] */
    imx291_write_register(ViPipe, 0x3446, 0x77); /* TCLKPOST[8:0] */
    imx291_write_register(ViPipe, 0x3447, 0x00); /* TCLKPOST[8:0] */
    imx291_write_register(ViPipe, 0x3448, 0x67); /* THSZERO[8:0] */
    imx291_write_register(ViPipe, 0x3449, 0x00); /* THSZERO[8:0] */
    imx291_write_register(ViPipe, 0x344A, 0x47); /* THSPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x344B, 0x00); /* THSPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x344C, 0x37); /* TCLKTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344D, 0x00); /* TCLKTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344E, 0x3F); /* THSTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x344F, 0x00); /* THSTRAIL[8:0] */
    imx291_write_register(ViPipe, 0x3450, 0xFF); /* TCLKZERO[8:0] */
    imx291_write_register(ViPipe, 0x3451, 0x00); /* TCLKZERO[8:0] */
    imx291_write_register(ViPipe, 0x3452, 0x3F); /* TCLKPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x3453, 0x00); /* TCLKPREPARE[8:0] */
    imx291_write_register(ViPipe, 0x3454, 0x37); /* TLPX[8:0] */
    imx291_write_register(ViPipe, 0x3455, 0x00); /* TLPX[8:0] */
    imx291_write_register(ViPipe, 0x3472, 0x80); /* X_OUT_SIZE 0x780=1920 */
    imx291_write_register(ViPipe, 0x3473, 0x07); /* X_OUT_SIZE */
    imx291_write_register(ViPipe, 0x3480, 0x49); /* INCKSEL7: 1080P CSI-2 37.125MHz->0x49; 74.25MHz->0x92 */

    imx291_default_reg_init(ViPipe);

    imx291_write_register(ViPipe, 0x3000, 0x00); /* Standby cancel */
    usleep(20000);
    imx291_write_register(ViPipe, 0x3002, 0x00); /* XTMSTA  Start master mode */
    imx291_write_register(ViPipe, 0x304B, 0x0A); /* XVSOUTSEL XHSOUTSEL enable output */

    printf("===================================================================\n");
    printf("== IMX291_MIPI_37.125MInput_891Mbps_4lane_10bit_1920x1080_120fps ==\n");
    printf("===================================================================\n");
    return SC_SUCCESS;
}
