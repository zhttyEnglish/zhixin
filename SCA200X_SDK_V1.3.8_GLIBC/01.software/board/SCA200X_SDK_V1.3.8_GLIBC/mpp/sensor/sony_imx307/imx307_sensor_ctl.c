/**
 * @file     imx307_sensor_ctl.c
 * @brief    SENSOR初始化接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-11-15 创建文件
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


#define IMX307_FLIP_MIRROR (0x3007)

#define SENSOR_1080P_30FPS_LINEAR_MODE  (1)
#define SENSOR_1080P_30FPS_2T1_WDR_MODE (2)

const unsigned char imx307_i2c_addr  = 0x34;  /* I2C Address of IMX307 */
const unsigned int  imx307_addr_byte = 2;
const unsigned int  imx307_data_byte = 1;

static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

extern ISP_SNS_STATE_S   *g_pastImx307[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunImx307BusInfo[ISP_MAX_PIPE_NUM];

int imx307_linear_1080p30_init(VI_PIPE ViPipe);
int imx307_wdr_1080p30_2to1_init(VI_PIPE ViPipe);



int imx307_i2c_init(VI_PIPE ViPipe)
{
    SC_CHAR acDevFile[16] = {0};
    SC_U8   u8DevNum = 0;
    SC_S32  ret      = 0;

    if(g_fd[ViPipe] >= 0)
    {
        return SC_SUCCESS;
    }

    u8DevNum = g_aunImx307BusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
    printf("u8DevNum=%d, %s\n", u8DevNum, acDevFile);

    g_fd[ViPipe] = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);
    if(g_fd[ViPipe] < 0)
    {
        printf("Open /dev/sc_i2c_drv-%u error!\n", u8DevNum);
        return SC_FAILURE;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (imx307_i2c_addr >> 1));
    if(ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return SC_SUCCESS;
}

int imx307_i2c_exit(VI_PIPE ViPipe)
{
    if(g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return SC_SUCCESS;
    }

    return SC_FAILURE;
}

int imx307_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:
    return SC_SUCCESS;
}

int imx307_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int idx = 0;
    int ret = 0;

    char buf[8];

    if(0 > g_fd[ViPipe])
    {
        return SC_SUCCESS;
    }

    if(imx307_addr_byte == 2)
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

    if(imx307_data_byte == 2)
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

    ret = write(g_fd[ViPipe], buf, imx307_addr_byte + imx307_data_byte);
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

void imx307_prog(VI_PIPE ViPipe, int *rom)
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
            imx307_write_register(ViPipe, addr, data);
        }
    }
}

void imx307_standby(VI_PIPE ViPipe)
{
    imx307_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
    imx307_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */

    return;
}

void imx307_restart(VI_PIPE ViPipe)
{
    imx307_write_register(ViPipe, 0x3000, 0x00); /* standby */
    delay_ms(20);
    imx307_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
    imx307_write_register(ViPipe, 0x304b, 0x0a);

    return;
}

int imx307_flip_off_mirror_off(VI_PIPE ViPipe)
{
    imx307_write_register(ViPipe, IMX307_FLIP_MIRROR, 0x00);
    return SC_SUCCESS;
}

int imx307_flip_on_mirror_off(VI_PIPE ViPipe)
{
    imx307_write_register(ViPipe, IMX307_FLIP_MIRROR, 0x01);
    return SC_SUCCESS;
}

int imx307_flip_off_mirror_on(VI_PIPE ViPipe)
{
    imx307_write_register(ViPipe, IMX307_FLIP_MIRROR, 0x02);
    return SC_SUCCESS;
}

int imx307_flip_on_mirror_on(VI_PIPE ViPipe)
{
    imx307_write_register(ViPipe, IMX307_FLIP_MIRROR, 0x03);
    return SC_SUCCESS;
}

void imx307_default_reg_init(VI_PIPE ViPipe)
{
    SC_U32 i = 0;

    for(i = 0; i < g_pastImx307[ViPipe]->astRegsInfo[0].u32RegNum; i++)
    {
        if(g_pastImx307[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate)
        {
            imx307_write_register(ViPipe,
                g_pastImx307[ViPipe]->astRegsInfo[0].astI2cData[i].u32RegAddr,
                g_pastImx307[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data);
        }
    }

    return;
}

int imx307_init(VI_PIPE ViPipe)
{
    WDR_MODE_E enWDRMode = WDR_MODE_NONE;
    SC_BOOL    bInit     = SC_FALSE;
    SC_U8      u8ImgMode = 0;
    SC_S32     ret       = SC_FAILURE;

    bInit     = g_pastImx307[ViPipe]->bInit;
    enWDRMode = g_pastImx307[ViPipe]->enWDRMode;
    u8ImgMode = g_pastImx307[ViPipe]->u8ImgMode;
    printf("IMX307: bInit[%d] enWDRMode[%d] u8ImgMode[%u]\n", bInit, enWDRMode, (SC_U32)u8ImgMode);

    ret = imx307_i2c_init(ViPipe);
    if(SC_SUCCESS != ret)
    {
        printf("Init imx307 i2c error!\n");
        return SC_FAILURE;
    }

    /* When sensor first init, config all registers */
    if(SC_FALSE == bInit)
    {
        if(WDR_MODE_2To1_LINE == enWDRMode)
        {
            if(SENSOR_1080P_30FPS_2T1_WDR_MODE == u8ImgMode)
            {
                ret = imx307_wdr_1080p30_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init imx307 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = imx307_linear_1080p30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init imx307 linear mode error!\n");
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
                ret = imx307_wdr_1080p30_2to1_init(ViPipe);
                if(SC_SUCCESS != ret)
                {
                    printf("Init imx307 wdr mode error!\n");
                    return SC_FAILURE;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = imx307_linear_1080p30_init(ViPipe);
            if(SC_SUCCESS != ret)
            {
                printf("Init imx307 linear mode error!\n");
                return SC_FAILURE;
            }
        }
    }

    g_pastImx307[ViPipe]->bInit = SC_TRUE;

    return SC_SUCCESS;
}

void imx307_exit(VI_PIPE ViPipe)
{
    imx307_i2c_exit(ViPipe);

    return;
}


/* 1080P30 and 1080P25: CSI-2 2LANE 12BIT */
int imx307_linear_1080p30_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

     /* Enter standby */
    ret = imx307_write_register(ViPipe, 0x3000, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write imx307 register error!\n");
        return SC_FAILURE;
    }

    imx307_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */

    /* Mode register setting */
    imx307_write_register(ViPipe, 0x3005, 0x01); /* ADBIT: 12bit */
    imx307_write_register(ViPipe, 0x3007, 0x00);
    imx307_write_register(ViPipe, 0x3009, 0x02); /* FRSEL: 30fps */
    imx307_write_register(ViPipe, 0x300a, 0xf0); /* BLKLEVEL */
    imx307_write_register(ViPipe, 0x3011, 0x0a);

    imx307_write_register(ViPipe, 0x3018, 0x46); /* VMAX: 1125(30fps); 1350(25fps) */
    imx307_write_register(ViPipe, 0x3019, 0x05); /* VMAX */
    imx307_write_register(ViPipe, 0x301c, 0x30); /* HMAX: 4400 */
    imx307_write_register(ViPipe, 0x301d, 0x11); /* HMAX */

    imx307_write_register(ViPipe, 0x3046, 0x01); /* ODBIT: 12bit */
    imx307_write_register(ViPipe, 0x305c, 0x18); /* INCKSEL1: 1080P CSI-2 37.125MHz->0x18; 74.25MHz->0x0c */
    imx307_write_register(ViPipe, 0x305d, 0x03); /* INCKSEL2: 1080P CSI-2 37.125MHz->0x03; 74.25MHz->0x03 */
    imx307_write_register(ViPipe, 0x305e, 0x20); /* INCKSEL3: 1080P CSI-2 37.125MHz->0x20; 74.25MHz->0x10 */
    imx307_write_register(ViPipe, 0x305f, 0x01); /* INCKSEL4: 1080P CSI-2 37.125MHz->0x01; 74.25MHz->0x01 */
    imx307_write_register(ViPipe, 0x309e, 0x4a);
    imx307_write_register(ViPipe, 0x309f, 0x4a);
    imx307_write_register(ViPipe, 0x311c, 0x0e);
    imx307_write_register(ViPipe, 0x3128, 0x04);
    imx307_write_register(ViPipe, 0x3129, 0x00); /* ADBIT1: 12Bit */
    imx307_write_register(ViPipe, 0x313b, 0x41);
    imx307_write_register(ViPipe, 0x315e, 0x1a); /* INCKSEL5: 1080P CSI-2 37.125MHz->0x1a; 74.25MHz->0x1b */
    imx307_write_register(ViPipe, 0x3164, 0x1a); /* INCKSEL6: 1080P CSI-2 37.125MHz->0x1a; 74.25MHz->0x1b */
    imx307_write_register(ViPipe, 0x3480, 0x49); /* INCKSEL7: 1080P CSI-2 37.125MHz->0x49; 74.25MHz->0x92 */
    imx307_write_register(ViPipe, 0x317c, 0x00); /* ADBIT2: 12Bit */
    imx307_write_register(ViPipe, 0x31ec, 0x0e); /* ADBIT3: 12Bit */

    /* MIPI setting */
    imx307_write_register(ViPipe, 0x3405, 0x10); /* REPETITION */
    imx307_write_register(ViPipe, 0x3407, 0x01); /* PHYSICAL LANE NUM: 2lane */
    imx307_write_register(ViPipe, 0x3414, 0x0a); /* OPB_SEZI_V */
    imx307_write_register(ViPipe, 0x3418, 0x38); /* Y_OUT_SIZE */
    imx307_write_register(ViPipe, 0x3419, 0x04); /* Y_OUT_SIZE */
    imx307_write_register(ViPipe, 0x3441, 0x0c); /* CSI_DT_FMT: 12bit */
    imx307_write_register(ViPipe, 0x3442, 0x0c); /* CSI_DT_FMT: 12bit */
    imx307_write_register(ViPipe, 0x3443, 0x01); /* CSI_LANE_MODE: mipi 2lane->1; 4lane->3 */
    imx307_write_register(ViPipe, 0x3444, 0x20); /* EXTCK_FREQ */
    imx307_write_register(ViPipe, 0x3445, 0x25);
    imx307_write_register(ViPipe, 0x3446, 0x57);
    imx307_write_register(ViPipe, 0x3447, 0x00);
    imx307_write_register(ViPipe, 0x3448, 0x37);
    imx307_write_register(ViPipe, 0x3449, 0x00);
    imx307_write_register(ViPipe, 0x344A, 0x1F); /* THSPREPARE */
    imx307_write_register(ViPipe, 0x344B, 0x00);
    imx307_write_register(ViPipe, 0x344C, 0x1F);
    imx307_write_register(ViPipe, 0x344D, 0x00);
    imx307_write_register(ViPipe, 0x344E, 0x1F); /* THSTRAIL */
    imx307_write_register(ViPipe, 0x344F, 0x00);
    imx307_write_register(ViPipe, 0x3450, 0x77); /* TCLKZERO */
    imx307_write_register(ViPipe, 0x3451, 0x00);
    imx307_write_register(ViPipe, 0x3452, 0x1F); /* TCLKPREPARE */
    imx307_write_register(ViPipe, 0x3453, 0x00);
    imx307_write_register(ViPipe, 0x3454, 0x17); /* TIPX */
    imx307_write_register(ViPipe, 0x3455, 0x00);
    imx307_write_register(ViPipe, 0x3472, 0x80); /* X_OUT_SIZE */
    imx307_write_register(ViPipe, 0x3473, 0x07); /* X_OUT_SIZE */

    imx307_default_reg_init(ViPipe);

    /* Standby Cancel */
    imx307_write_register(ViPipe, 0x3000, 0x00); /* standby */
    usleep(20000);
    imx307_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
    imx307_write_register(ViPipe, 0x304B, 0x0a); /* XVSOUTSEL XHSOUTSEL enable output */

    printf("==============================================================\n");
    printf("== Sony imx307 sensor 1080P30fps(MIPI:2L12B) init success!  ==\n");
    printf("==============================================================\n");
    return SC_SUCCESS;
}


/* DOL2 1080P30 and 1080P25: CSI-2 2LANE 896Mbps 12BIT */
int imx307_wdr_1080p30_2to1_init(VI_PIPE ViPipe)
{
    int ret = SC_FAILURE;

    ret = imx307_write_register(ViPipe, 0x3003, 0x01);
    if(SC_SUCCESS != ret)
    {
        /* Write register error first time, then return directly. */
        printf("Write imx307 register error!\n");
        return SC_FAILURE;
    }

    imx307_write_register(ViPipe, 0x3002, 0x01);
    imx307_write_register(ViPipe, 0x3000, 0x01);
    imx307_write_register(ViPipe, 0x3005, 0x01);
    imx307_write_register(ViPipe, 0x3007, 0x00);
    imx307_write_register(ViPipe, 0x3009, 0x01);
    imx307_write_register(ViPipe, 0x300A, 0xF0);
    imx307_write_register(ViPipe, 0x300C, 0x11);
    imx307_write_register(ViPipe, 0x300F, 0x00);
    imx307_write_register(ViPipe, 0x3010, 0x21);
    imx307_write_register(ViPipe, 0x3012, 0x64);
    imx307_write_register(ViPipe, 0x3016, 0x09);
    imx307_write_register(ViPipe, 0x3018, 0x46);
    imx307_write_register(ViPipe, 0x3019, 0x05); /* vmax 1350 */
    imx307_write_register(ViPipe, 0x301C, 0x98);
    imx307_write_register(ViPipe, 0x301D, 0x08); /* hmax 2200 */
    imx307_write_register(ViPipe, 0x3020, 0x02); /* sexp = 92 */
    imx307_write_register(ViPipe, 0x3021, 0x00);
    imx307_write_register(ViPipe, 0x3024, 0x59); /* lexp = 92*16 = 1520 */
    imx307_write_register(ViPipe, 0x3025, 0x03);
    imx307_write_register(ViPipe, 0x3030, 0xcd); /* rhs = 2n+5 = 95 */
    imx307_write_register(ViPipe, 0x3031, 0x01);
    imx307_write_register(ViPipe, 0x3045, 0x05);
    imx307_write_register(ViPipe, 0x3046, 0x01);
    imx307_write_register(ViPipe, 0x304B, 0x0A);
    imx307_write_register(ViPipe, 0x305C, 0x18);
    imx307_write_register(ViPipe, 0x305D, 0x03);
    imx307_write_register(ViPipe, 0x305E, 0x20);
    imx307_write_register(ViPipe, 0x305F, 0x01);
    imx307_write_register(ViPipe, 0x3070, 0x02);
    imx307_write_register(ViPipe, 0x3071, 0x11);
    imx307_write_register(ViPipe, 0x309B, 0x10);
    imx307_write_register(ViPipe, 0x309C, 0x22);
    imx307_write_register(ViPipe, 0x30A2, 0x02);
    imx307_write_register(ViPipe, 0x30A6, 0x20);
    imx307_write_register(ViPipe, 0x30A8, 0x20);
    imx307_write_register(ViPipe, 0x30AA, 0x20);
    imx307_write_register(ViPipe, 0x30AC, 0x20);
    imx307_write_register(ViPipe, 0x30B0, 0x43);
    imx307_write_register(ViPipe, 0x3106, 0x11);
    imx307_write_register(ViPipe, 0x3119, 0x9E);
    imx307_write_register(ViPipe, 0x311C, 0x1E);
    imx307_write_register(ViPipe, 0x311E, 0x08);
    imx307_write_register(ViPipe, 0x3128, 0x05);
    imx307_write_register(ViPipe, 0x3129, 0x00);
    imx307_write_register(ViPipe, 0x313D, 0x83);
    imx307_write_register(ViPipe, 0x3150, 0x03);
    imx307_write_register(ViPipe, 0x315E, 0x1A);
    imx307_write_register(ViPipe, 0x3164, 0x1A);
    imx307_write_register(ViPipe, 0x317C, 0x00);
    imx307_write_register(ViPipe, 0x317E, 0x00);
    imx307_write_register(ViPipe, 0x31EC, 0x0E);
    imx307_write_register(ViPipe, 0x32B8, 0x50);
    imx307_write_register(ViPipe, 0x32B9, 0x10);
    imx307_write_register(ViPipe, 0x32BA, 0x00);
    imx307_write_register(ViPipe, 0x32BB, 0x04);
    imx307_write_register(ViPipe, 0x32C8, 0x50);
    imx307_write_register(ViPipe, 0x32C9, 0x10);
    imx307_write_register(ViPipe, 0x32CA, 0x00);
    imx307_write_register(ViPipe, 0x32CB, 0x04);
    imx307_write_register(ViPipe, 0x332C, 0xD3);
    imx307_write_register(ViPipe, 0x332D, 0x10);
    imx307_write_register(ViPipe, 0x332E, 0x0D);
    imx307_write_register(ViPipe, 0x3358, 0x06);
    imx307_write_register(ViPipe, 0x3359, 0xE1);
    imx307_write_register(ViPipe, 0x335A, 0x11);
    imx307_write_register(ViPipe, 0x3360, 0x1E);
    imx307_write_register(ViPipe, 0x3361, 0x61);
    imx307_write_register(ViPipe, 0x3362, 0x10);
    imx307_write_register(ViPipe, 0x33B0, 0x50);
    imx307_write_register(ViPipe, 0x33B2, 0x1A);
    imx307_write_register(ViPipe, 0x33B3, 0x04);
    imx307_write_register(ViPipe, 0x3405, 0x00);
    imx307_write_register(ViPipe, 0x3407, 0x01);
    imx307_write_register(ViPipe, 0x3414, 0x0A);
    imx307_write_register(ViPipe, 0x3415, 0x00);
    imx307_write_register(ViPipe, 0x3418, 0x76); /* pic height 2678 */
    imx307_write_register(ViPipe, 0x3419, 0x0a); /* pic height */
    imx307_write_register(ViPipe, 0x3441, 0x0C);
    imx307_write_register(ViPipe, 0x3442, 0x0C);
    imx307_write_register(ViPipe, 0x3443, 0x01);
    imx307_write_register(ViPipe, 0x3444, 0x20);
    imx307_write_register(ViPipe, 0x3445, 0x25);
    imx307_write_register(ViPipe, 0x3446, 0x77);
    imx307_write_register(ViPipe, 0x3447, 0x00);
    imx307_write_register(ViPipe, 0x3448, 0x67);
    imx307_write_register(ViPipe, 0x3449, 0x00);
    imx307_write_register(ViPipe, 0x344A, 0x47);
    imx307_write_register(ViPipe, 0x344B, 0x00);
    imx307_write_register(ViPipe, 0x344C, 0x37);
    imx307_write_register(ViPipe, 0x344D, 0x00);
    imx307_write_register(ViPipe, 0x344E, 0x3F);
    imx307_write_register(ViPipe, 0x344F, 0x00);
    imx307_write_register(ViPipe, 0x3450, 0xFF);
    imx307_write_register(ViPipe, 0x3451, 0x00);
    imx307_write_register(ViPipe, 0x3452, 0x3F);
    imx307_write_register(ViPipe, 0x3453, 0x00);
    imx307_write_register(ViPipe, 0x3454, 0x37);
    imx307_write_register(ViPipe, 0x3472, 0xA0); /* pic width 1952 */
    imx307_write_register(ViPipe, 0x3473, 0x07); /* pic width */
    imx307_write_register(ViPipe, 0x347B, 0x23);
    imx307_write_register(ViPipe, 0x3010, 0x61); /* set dol2 long exp gain enable */
    imx307_write_register(ViPipe, 0x30f0, 0x64); /* set dol2 shot exp gain enable */

    imx307_default_reg_init(ViPipe);

    imx307_write_register(ViPipe, 0x3000, 0x00); /* standby */
    delay_ms(20);
    imx307_write_register(ViPipe, 0x3002, 0x00); /* master mode start */

    printf("=========================================================================\n");
    printf("== Imx307 sensor 1080P30fps 12bit 2to1 WDR(60fps->30fps) init success! ==\n");
    printf("=========================================================================\n");
    return SC_SUCCESS;
}
