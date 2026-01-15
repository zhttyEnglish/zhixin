/**
 * @file     gc2093_cmos.c
 * @brief    GALAXYCORE GC2093 SENSOR控制接口
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2022-02-17 创建文件
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


#if !defined(__GC2093_CMOS_C_)
#define __GC2093_CMOS_C_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "sc_gpio.h"
#include "sc_comm_sns.h"
#include "sc_comm_video.h"
#include "sc_sns_ctrl.h"
#include "mpi_isp.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#define GC2093_ID 2093


/****************************************************************************
 * global variables                                                         *
 ****************************************************************************/
ISP_SNS_STATE_S *g_pastGc2093[ISP_MAX_PIPE_NUM] = {SC_NULL};

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_pastGc2093[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_pastGc2093[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev)       (g_pastGc2093[dev] = SC_NULL)

ISP_SNS_COMMBUS_U g_aunGc2093BusInfo[ISP_MAX_PIPE_NUM] =
{
    [0] = { .s8I2cDev = 1},
    [1 ... ISP_MAX_PIPE_NUM - 1] = { .s8I2cDev = -1}
};

static ISP_FSWDR_MODE_E genFSWDRMode[ISP_MAX_PIPE_NUM] =
{
    [0 ... ISP_MAX_PIPE_NUM - 1] = ISP_FSWDR_NORMAL_MODE
};

static SC_U32 gu32MaxTimeGetCnt[ISP_MAX_PIPE_NUM]   = {0};
static SC_U32 g_au32InitExposure[ISP_MAX_PIPE_NUM]  = {0};
static SC_U32 g_au32LinesPer500ms[ISP_MAX_PIPE_NUM] = {0};
static SC_U16 g_au16InitWBGain[ISP_MAX_PIPE_NUM][3] = {{0}};
static SC_U16 g_au16SampleRgain[ISP_MAX_PIPE_NUM]   = {0};
static SC_U16 g_au16SampleBgain[ISP_MAX_PIPE_NUM]   = {0};

static SENSOR_STATE_S g_astGc2093State[ISP_MAX_PIPE_NUM] = {{0}};

static SC_TUNING_BIN_FILE_S g_acTuningBinName[ISP_MAX_PIPE_NUM] =
{
    [0] = {
              .acLinearBinName = "/local/tuning/gc2093/gc2093_tuning.bin",
              .acWdrBinName    = "/local/tuning/gc2093/gc2093_tuning_hdr.bin"
          },

    [1] = {
              .acLinearBinName = "/local/tuning/gc2093/gc2093_tuning.bin",
              .acWdrBinName    = "/local/tuning/gc2093/gc2093_tuning_hdr.bin"
          },
};



/****************************************************************************
 * extern                                                                   *
 ****************************************************************************/
extern const unsigned int gc2093_i2c_addr;
extern unsigned int gc2093_addr_byte;
extern unsigned int gc2093_data_byte;

extern int  gc2093_init(VI_PIPE ViPipe);
extern void gc2093_exit(VI_PIPE ViPipe);
extern void gc2093_standby(VI_PIPE ViPipe);
extern void gc2093_restart(VI_PIPE ViPipe);
extern void gc2093_default_reg_init(VI_PIPE ViPipe);
extern int  gc2093_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  gc2093_read_register(VI_PIPE ViPipe, int addr);
extern int  gc2093_flip_off_mirror_off(VI_PIPE ViPipe);
extern int  gc2093_flip_on_mirror_off(VI_PIPE ViPipe);
extern int  gc2093_flip_off_mirror_on(VI_PIPE ViPipe);
extern int  gc2093_flip_on_mirror_on(VI_PIPE ViPipe);



/****************************************************************************
 * local variables                                                          *
 ****************************************************************************/
/* Sensor register address */
#define GC2093_SHORT_EXPTIME_ADDR_H (0x0001) // Exposure_Short[13:8]
#define GC2093_SHORT_EXPTIME_ADDR_L (0x0002) // Exposure_Short[7:0]
#define GC2093_EXPTIME_ADDR_H       (0x0003) // Exposure[13:8]
#define GC2093_EXPTIME_ADDR_L       (0x0004) // Exposure[7:0]
#define GC2093_AGAIN_ADDR_H         (0x00b4) // Analog_PGA_gain[9:8]
#define GC2093_AGAIN_ADDR_L         (0x00b3) // Analog_PGA_gain[7:0]
#define GC2093_DGAIN_ADDR_H         (0x00b8) // Col_gain[11:6]
#define GC2093_DGAIN_ADDR_L         (0x00b9) // Col_gain[5:0]
#define GC2093_GAIN_ADDR_0X155      (0x0155)
#define GC2093_GAIN_ADDR_0X31D      (0x031d)
#define GC2093_GAIN_ADDR_0XC2       (0x00c2)
#define GC2093_GAIN_ADDR_0XCF       (0x00cf)
#define GC2093_GAIN_ADDR_0XD9       (0x00d9)

#define GC2093_AUTO_PREGAIN_ADDR_H  (0x00b1) // auto-pregain-sync[9:6]
#define GC2093_AUTO_PREGAIN_ADDR_L  (0x00b2) // auto-pregain[5:0]
#define GC2093_VMAX_ADDR_H          (0x0041) // Vmax[13:8]
#define GC2093_VMAX_ADDR_L          (0x0042) // Vmax[7:0]


#define SENSOR_FULL_LINES_MAX          (0x3FFF)
#define SENSOR_FULL_LINES_MAX_2TO1_WDR (0x966)  /* considering the YOUT_SIZE and bad frame */

#define SENSOR_INCREASE_LINES       (0)
#define SENSOR_VMAX_1080P30_LINEAR  (2500 + SENSOR_INCREASE_LINES)
#define SENSOR_VMAX_1080P60TO30_WDR (2500 + SENSOR_INCREASE_LINES)

#define SENSOR_1080P_30FPS_LINEAR_MODE  (0)
#define SENSOR_1080P_30FPS_2T1_WDR_MODE (1)

#define SENSOR_RES_IS_1080P(w, h) ((w) <= 1920 && (h) <= 1080)
#define SENSOR_RES_IS_WDR(w, h)   ((w) <= 1952 && (h) <= 2678)

#define HIGH_8BITS(x)             (((x) & 0xff00) >> 8)
#define LOW_8BITS(x)              ((x) & 0x00ff)



static SC_S32 cmos_power_on(VI_PIPE ViPipe, dev_power_attr_t *p_dev_power_attr)
{
    int power_gpio = 0;
    int reset_gpio = 0;

    power_gpio = sc_gpio_name_to_num(p_dev_power_attr->power_gpio[0],
                                     p_dev_power_attr->power_gpio[1]);

    reset_gpio = sc_gpio_name_to_num(p_dev_power_attr->reset_gpio[0],
                                     p_dev_power_attr->reset_gpio[1]);

    printf("cmos_power_on: power_gpio=%d, reset_gpio=%d\n", power_gpio, reset_gpio);

    /* set the gpio to low */
    sc_gpio_set_value(power_gpio, 0);
    sc_gpio_set_value(reset_gpio, 0);

    /* give some delay, set the gpio to high */
    usleep(10000);
    sc_gpio_set_value(power_gpio, 1);
    usleep(10000);
    sc_gpio_set_value(reset_gpio, 1);
    usleep(10000);

    return SC_SUCCESS;
}

static SC_S32 cmos_power_off(VI_PIPE ViPipe, dev_power_attr_t *p_dev_power_attr)
{
    int power_gpio = 0;
    int reset_gpio = 0;

    power_gpio = sc_gpio_name_to_num(p_dev_power_attr->power_gpio[0],
                                     p_dev_power_attr->power_gpio[1]);

    reset_gpio = sc_gpio_name_to_num(p_dev_power_attr->reset_gpio[0],
                                     p_dev_power_attr->reset_gpio[1]);

    printf("cmos_power_off: power_gpio=%d, reset_gpio=%d\n", power_gpio, reset_gpio);

    /* set the gpio to low */
    sc_gpio_set_value(power_gpio, 0);
    sc_gpio_set_value(reset_gpio, 0);

    return SC_SUCCESS;
}

static SC_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER(pstAeSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    pstAeSnsDft->f32Fps          = 30;
    pstAeSnsDft->f32MaxFps       = 30;
    pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FlickerFreq  = 50 * 256;
    pstAeSnsDft->u32FullLinesMax = SENSOR_FULL_LINES_MAX;
    pstAeSnsDft->u32Hmax         = 2640;

    if(g_au32LinesPer500ms[ViPipe] == 0)
    {
        pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * pstAeSnsDft->f32Fps / 2;
    }
    else
    {
        pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
    }

    switch(pstSnsState->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
        {
            /* Linear mode */
            pstAeSnsDft->u32MaxAgain   = 74976 * 16;
            pstAeSnsDft->u32MinAgain   = 1024;
            pstAeSnsDft->u32MaxDgain   = 1023;
            pstAeSnsDft->u32MinDgain   = 64;
            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 3;
            break;
        }

        case WDR_MODE_2To1_LINE:
        {
            /* Wdr mode */
            pstAeSnsDft->u32MaxAgain   = 74976;
            pstAeSnsDft->u32MinAgain   = 1024;
            pstAeSnsDft->u32MaxDgain   = 74976;
            pstAeSnsDft->u32MinDgain   = 1024;
            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 2;

            pstAeSnsDft->u32FullLinesShort = 461;
            pstAeSnsDft->u32LFMaxShortTime = 461;
            pstAeSnsDft->u32LFMinExposure  = 2;
            break;
        }
    }

    return SC_SUCCESS;
}

static SC_VOID cmos_fps_set(VI_PIPE ViPipe, SC_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_U32 u32VMAX = SENSOR_VMAX_1080P30_LINEAR;
    SC_U32 u32Temp = 0;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    switch(pstSnsState->u8ImgMode)
    {
        case SENSOR_1080P_30FPS_2T1_WDR_MODE:
        {
            if((f32Fps <= pstAeSnsDft->f32MaxFps) && (f32Fps >= 15.22))
            {
                u32VMAX = SENSOR_VMAX_1080P60TO30_WDR * pstAeSnsDft->f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }

            u32VMAX = (u32VMAX > SENSOR_FULL_LINES_MAX_2TO1_WDR) ? SENSOR_FULL_LINES_MAX_2TO1_WDR : u32VMAX;
            break;
        }

        case SENSOR_1080P_30FPS_LINEAR_MODE:
        {
            if((f32Fps <= pstAeSnsDft->f32MaxFps) && (f32Fps >= 2.06))
            {
                u32VMAX = SENSOR_VMAX_1080P30_LINEAR * pstAeSnsDft->f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }

            u32VMAX = (u32VMAX > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32VMAX;
            break;
        }

        default:
        {
            return;
        }
    }


    if(WDR_MODE_NONE == pstSnsState->enWDRMode)
    {
        pstSnsState->astRegsInfo[0].astI2cData[13].u32Data = LOW_8BITS(u32VMAX);
        pstSnsState->astRegsInfo[0].astI2cData[14].u32Data = HIGH_8BITS(u32VMAX);
    }
    else
    {
        u32Temp = u32VMAX / 2;
        pstSnsState->astRegsInfo[0].astI2cData[13].u32Data = LOW_8BITS(u32Temp);
        pstSnsState->astRegsInfo[0].astI2cData[14].u32Data = HIGH_8BITS(u32Temp);
    }

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        pstSnsState->u32FLStd   = u32VMAX;
        pstSnsState->u32FLShort = pstAeSnsDft->u32FullLinesShort;
        //printf("gu32FullLinesStd:%d\n",pstSnsState->u32FLStd);

        g_astGc2093State[ViPipe].u32RHS1_MAX = (u32VMAX - g_astGc2093State[ViPipe].u32BRL) * 2 - 21;
    }
    else
    {
        pstSnsState->u32FLStd = u32VMAX;
    }


    pstAeSnsDft->f32Fps           = f32Fps;
    pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * f32Fps / 2;
    pstAeSnsDft->u32FullLinesStd  = pstSnsState->u32FLStd;
    pstAeSnsDft->u32MaxIntTime    = pstSnsState->u32FLStd - 2;
    pstSnsState->au32FL[0]        = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FullLines     = pstSnsState->au32FL[0];

    return;
}

static SC_VOID cmos_slow_framerate_set(VI_PIPE ViPipe, SC_U32 u32FullLines, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32FullLines = (u32FullLines > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32FullLines;
    pstSnsState->au32FL[0] = u32FullLines;

    pstSnsState->astRegsInfo[0].astI2cData[13].u32Data = LOW_8BITS(pstSnsState->au32FL[0]);
    pstSnsState->astRegsInfo[0].astI2cData[14].u32Data = HIGH_8BITS(pstSnsState->au32FL[0]);

    pstAeSnsDft->u32FullLines  = pstSnsState->au32FL[0];
    pstAeSnsDft->u32MaxIntTime = pstSnsState->au32FL[0] - 2;

    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static SC_VOID cmos_inttime_update(VI_PIPE ViPipe, SC_U32 u32IntTime)
{
    static SC_BOOL bFirst[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1};

    static SC_U32 u32ShortIntTime[ISP_MAX_PIPE_NUM]  = {0};
    static SC_U32 u32LongIntTime[ISP_MAX_PIPE_NUM]   = {0};

    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_S32 u32Value = 0;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32Value = (u32IntTime > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32IntTime;
    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        if(bFirst[ViPipe])
        {
            u32ShortIntTime[ViPipe] = u32Value;
            bFirst[ViPipe] = SC_FALSE;
        }
        else
        {
            u32LongIntTime[ViPipe] = u32Value;
            if(u32LongIntTime[ViPipe] > 1100)
            {
                u32LongIntTime[ViPipe] = 1100;
            }
            else if(u32LongIntTime[ViPipe] < 1)
            {
                u32LongIntTime[ViPipe] = 1;
            }

            if(u32ShortIntTime[ViPipe] > 48)
            {
                u32ShortIntTime[ViPipe] = 48;
            }
            else if(u32ShortIntTime[ViPipe] < 1)
            {
                u32ShortIntTime[ViPipe] = 1;
            }

            //printf("sns[%d]: lInt=%d, sInt=%d\n", ViPipe, u32LongIntTime[ViPipe], u32ShortIntTime[ViPipe]);
            pstSnsState->astRegsInfo[0].astI2cData[0].u32Data  = LOW_8BITS(u32LongIntTime[ViPipe]);
            pstSnsState->astRegsInfo[0].astI2cData[1].u32Data  = HIGH_8BITS(u32LongIntTime[ViPipe]);
            pstSnsState->astRegsInfo[0].astI2cData[15].u32Data = LOW_8BITS(u32ShortIntTime[ViPipe]);
            pstSnsState->astRegsInfo[0].astI2cData[16].u32Data = HIGH_8BITS(u32ShortIntTime[ViPipe]);
            bFirst[ViPipe] = SC_TRUE;
        }
    }
    else
    {
        //printf("sns[%d]: ln=%d, u32Value=%u\n", ViPipe, u32IntTime, u32Value);
        pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = LOW_8BITS(u32Value);
        pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = HIGH_8BITS(u32Value);
    }

    return;
}

static SC_U32 g_au32RegValTab[25][9] =
{
 /* 0xb3  0xb8  0xb9 0x155 0x031d 0xc2 0xcf 0xd9 0x031d */
    {0x00, 0x01, 0x00, 0x08, 0x2d, 0x10, 0x08, 0x0a, 0x28},
    {0x10, 0x01, 0x0c, 0x08, 0x2d, 0x10, 0x08, 0x0a, 0x28},
    {0x20, 0x01, 0x1b, 0x08, 0x2d, 0x11, 0x08, 0x0c, 0x28},
    {0x30, 0x01, 0x2c, 0x08, 0x2d, 0x12, 0x08, 0x0e, 0x28},
    {0x40, 0x01, 0x3f, 0x08, 0x2d, 0x14, 0x08, 0x12, 0x28},
    {0x50, 0x02, 0x16, 0x08, 0x2d, 0x15, 0x08, 0x14, 0x28},
    {0x60, 0x02, 0x35, 0x08, 0x2d, 0x17, 0x08, 0x18, 0x28},
    {0x70, 0x03, 0x16, 0x08, 0x2d, 0x18, 0x08, 0x1a, 0x28},
    {0x80, 0x04, 0x02, 0x08, 0x2d, 0x1a, 0x08, 0x1e, 0x28},
    {0x90, 0x04, 0x31, 0x08, 0x2d, 0x1b, 0x08, 0x20, 0x28},
    {0xa0, 0x05, 0x32, 0x08, 0x2d, 0x1d, 0x08, 0x24, 0x28},
    {0xb0, 0x06, 0x35, 0x08, 0x2d, 0x1e, 0x08, 0x26, 0x28},
    {0xc0, 0x08, 0x04, 0x08, 0x2d, 0x20, 0x08, 0x2a, 0x28},
    {0x5a, 0x09, 0x19, 0x08, 0x2d, 0x1e, 0x08, 0x2a, 0x28},
    {0x83, 0x0b, 0x0f, 0x08, 0x2d, 0x1f, 0x08, 0x2a, 0x28},
    {0x93, 0x0d, 0x12, 0x08, 0x2d, 0x21, 0x08, 0x2e, 0x28},
    {0x84, 0x10, 0x00, 0x0b, 0x2d, 0x22, 0x08, 0x30, 0x28},
    {0x94, 0x12, 0x3a, 0x0b, 0x2d, 0x24, 0x08, 0x34, 0x28},
    {0x5d, 0x1a, 0x02, 0x0b, 0x2d, 0x26, 0x08, 0x34, 0x28},
    {0x9b, 0x1b, 0x20, 0x0b, 0x2d, 0x26, 0x08, 0x34, 0x28},
    {0x8c, 0x20, 0x0f, 0x0b, 0x2d, 0x26, 0x08, 0x34, 0x28},
    {0x9c, 0x26, 0x07, 0x12, 0x2d, 0x26, 0x08, 0x34, 0x28},
    {0xB6, 0x36, 0x21, 0x12, 0x2d, 0x26, 0x08, 0x34, 0x28},
    {0xad, 0x37, 0x3a, 0x12, 0x2d, 0x26, 0x08, 0x34, 0x28},
    {0xbd, 0x3d, 0x02, 0x12, 0x2d, 0x26, 0x08, 0x34, 0x28}
};

static SC_U32 g_au32AGainTab[25] =
{
    1024, 1216, 1456, 1712, 2000, 2352, 2832, 3376, 3968, 4752,
    5696, 6800, 8064, 9584, 11344, 13376, 15648, 18448, 26352, 26416,
    30960, 36672, 51824, 63344, 74976
};

static SC_U32 g_au32DGainVal[ISP_MAX_PIPE_NUM];

static SC_VOID cmos_again_calc_table(VI_PIPE ViPipe, SC_U32 *pu32AgainLin, SC_U32 *pu32AgainDb)
{
    SC_S32 gain = 0;
    SC_S32 i    = 0;

    CMOS_CHECK_POINTER_VOID(pu32AgainLin);
    CMOS_CHECK_POINTER_VOID(pu32AgainDb);


    //printf("sns pp=%d, agLin=%d, agDb=%d\n", ViPipe, *pu32AgainLin, *pu32AgainDb);
    gain = *pu32AgainLin;
    if(gain >= g_au32AGainTab[24])
    {
        *pu32AgainLin = g_au32AGainTab[24];
        *pu32AgainDb  = 24;

        g_au32DGainVal[ViPipe] = gain * 64 / g_au32AGainTab[24];
    }
    else
    {
        for(i = 1; i < 25; i++)
        {
            if(gain < g_au32AGainTab[i])
            {
                *pu32AgainLin = g_au32AGainTab[i - 1];
                *pu32AgainDb  = i - 1;

                g_au32DGainVal[ViPipe] = gain * 64 / g_au32AGainTab[i - 1];
                break;
            }
        }
    }

    //printf("sns pp=%d, agLin=%d, agDb=%d, dgVal=%d\n", ViPipe, *pu32AgainLin, *pu32AgainDb, g_au32DGainVal[ViPipe]);
    return;
}

static SC_VOID cmos_gains_update(VI_PIPE ViPipe, SC_U32 u32Again, SC_U32 u32Dgain)
{
    static SC_BOOL bFirst[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1};

    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_U8 u8DgainHigh = 1;
    SC_U8 u8DgainLow  = 0;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    //printf("sns pp=%d, ag=%d, dg=%d\n", ViPipe, u32Again, u32Dgain);
    if(pstSnsState->enWDRMode == WDR_MODE_2To1_LINE)
    {
        if(bFirst[ViPipe])
        {
            /* short gain */
            bFirst[ViPipe] = SC_FALSE;
        }
        else
        {
            /* long gain */
            bFirst[ViPipe] = SC_TRUE;
            u8DgainHigh    = (g_au32DGainVal[ViPipe] >> 6) & 0x0f;
            u8DgainLow     = (g_au32DGainVal[ViPipe] & 0x3f) << 2;
            pstSnsState->astRegsInfo[0].astI2cData[2].u32Data  = g_au32RegValTab[u32Again][0];
            pstSnsState->astRegsInfo[0].astI2cData[3].u32Data  = g_au32RegValTab[u32Again][1];
            pstSnsState->astRegsInfo[0].astI2cData[4].u32Data  = g_au32RegValTab[u32Again][2];
            pstSnsState->astRegsInfo[0].astI2cData[5].u32Data  = g_au32RegValTab[u32Again][3];
            pstSnsState->astRegsInfo[0].astI2cData[6].u32Data  = g_au32RegValTab[u32Again][4];
            pstSnsState->astRegsInfo[0].astI2cData[7].u32Data  = g_au32RegValTab[u32Again][5];
            pstSnsState->astRegsInfo[0].astI2cData[8].u32Data  = g_au32RegValTab[u32Again][6];
            pstSnsState->astRegsInfo[0].astI2cData[9].u32Data  = g_au32RegValTab[u32Again][7];
            pstSnsState->astRegsInfo[0].astI2cData[10].u32Data = g_au32RegValTab[u32Again][8];
            pstSnsState->astRegsInfo[0].astI2cData[11].u32Data = u8DgainLow;
            pstSnsState->astRegsInfo[0].astI2cData[12].u32Data = u8DgainHigh;
        }
    }
    else if(pstSnsState->enWDRMode == WDR_MODE_NONE)
    {
        u8DgainHigh = (g_au32DGainVal[ViPipe] >> 6) & 0x0f;
        u8DgainLow  = (g_au32DGainVal[ViPipe] & 0x3f) << 2;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32Data  = g_au32RegValTab[u32Again][0];
        pstSnsState->astRegsInfo[0].astI2cData[3].u32Data  = g_au32RegValTab[u32Again][1];
        pstSnsState->astRegsInfo[0].astI2cData[4].u32Data  = g_au32RegValTab[u32Again][2];
        pstSnsState->astRegsInfo[0].astI2cData[5].u32Data  = g_au32RegValTab[u32Again][3];
        pstSnsState->astRegsInfo[0].astI2cData[6].u32Data  = g_au32RegValTab[u32Again][4];
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data  = g_au32RegValTab[u32Again][5];
        pstSnsState->astRegsInfo[0].astI2cData[8].u32Data  = g_au32RegValTab[u32Again][6];
        pstSnsState->astRegsInfo[0].astI2cData[9].u32Data  = g_au32RegValTab[u32Again][7];
        pstSnsState->astRegsInfo[0].astI2cData[10].u32Data = g_au32RegValTab[u32Again][8];
        pstSnsState->astRegsInfo[0].astI2cData[11].u32Data = u8DgainLow;
        pstSnsState->astRegsInfo[0].astI2cData[12].u32Data = u8DgainHigh;
    }

    //printf("sns pp=%d, dg=%d, dgRegH=%d, dgRegL=%d\n", ViPipe, g_au32DGainVal[ViPipe], u8DgainHigh, u8DgainLow);
    return;
}

static SC_VOID cmos_get_inttime_max(VI_PIPE ViPipe, SC_U16 u16ManRatioEnable, SC_U32 *au32Ratio,
    SC_U32 *au32IntTimeMax, SC_U32 *au32IntTimeMin, SC_U32 *pu32LFMaxIntTime)
{
    return;
}

/* Only used in LINE_WDR mode */
static SC_VOID cmos_ae_fswdr_attr_set(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr)
{
    CMOS_CHECK_POINTER_VOID(pstAeFSWDRAttr);

    genFSWDRMode[ViPipe]      = pstAeFSWDRAttr->enFSWDRMode;
    gu32MaxTimeGetCnt[ViPipe] = 0;

    return;
}

static SC_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);

    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default     = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set            = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set = cmos_slow_framerate_set;
    pstExpFuncs->pfn_cmos_inttime_update     = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update       = cmos_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table   = cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table   = SC_NULL;
    pstExpFuncs->pfn_cmos_get_inttime_max    = cmos_get_inttime_max;
    pstExpFuncs->pfn_cmos_ae_fswdr_attr_set  = cmos_ae_fswdr_attr_set;

    return SC_SUCCESS;
}

/* Rgain and Bgain of the golden sample */
#define GOLDEN_RGAIN 0
#define GOLDEN_BGAIN 0
static SC_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER(pstAwbSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));
    pstAwbSnsDft->u16GoldenRgain = GOLDEN_RGAIN;
    pstAwbSnsDft->u16GoldenBgain = GOLDEN_BGAIN;

    pstAwbSnsDft->u16SampleRgain = g_au16SampleRgain[ViPipe];
    pstAwbSnsDft->u16SampleBgain = g_au16SampleBgain[ViPipe];

    return SC_SUCCESS;
}

static SC_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);

    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

    return SC_SUCCESS;
}

static ISP_CMOS_DNG_COLORPARAM_S g_stDngColorParam =
{
    {378, 256, 430},
    {439, 256, 439}
};

static SC_S32 cmos_get_isp_default(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER(pstDef);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));
    switch(pstSnsState->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
        {
            strncpy(pstDef->acTuningPraBinName, g_acTuningBinName[ViPipe].acLinearBinName, TUNING_BIN_FILENAME_LEN);
            break;
        }

        case WDR_MODE_2To1_LINE:
        {
            strncpy(pstDef->acTuningPraBinName, g_acTuningBinName[ViPipe].acWdrBinName, TUNING_BIN_FILENAME_LEN);
            break;
        }
    }

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        pstDef->stWdrSwitchAttr.au32ExpRatio[0] = 0x40;
        pstDef->stWdrSwitchAttr.au32ExpRatio[1] = 0x40;
        pstDef->stWdrSwitchAttr.au32ExpRatio[2] = 0x40;
        //pstDef->stWdrSwitchAttr.au32ShortOffset = 475;
        //pstDef->stWdrSwitchAttr.au32MidOffset   = 475;
        //pstDef->stWdrSwitchAttr.au32LongOffset  = 14;
        pstDef->stWdrSwitchAttr.au32VcCnt       = 2;
        pstDef->stWdrSwitchAttr.au32VcMask      = 0x03;
    }

    pstDef->stSensorMode.u32SensorID  = GC2093_ID;
    pstDef->stSensorMode.u8SensorMode = pstSnsState->u8ImgMode;
    memcpy(&pstDef->stDngColorParam, &g_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));

    switch(pstSnsState->u8ImgMode)
    {
        default:
        case SENSOR_1080P_30FPS_LINEAR_MODE:
        {
            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 12;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel   = 4095;
            break;
        }

        case SENSOR_1080P_30FPS_2T1_WDR_MODE:
        {
            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 10;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel   = 1023;
            break;
        }
    }

    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Numerator   = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Numerator   = 1;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimCols = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatCols = 2;
    pstDef->stSensorMode.stDngRawFormat.enCfaLayout         = CFALAYOUT_TYPE_RECTANGULAR;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[2] = 2;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[2] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[3] = 2;
    pstDef->stSensorMode.bValidDngRawFormat = SC_TRUE;

    return SC_SUCCESS;
}

static SC_VOID cmos_set_pixel_detect(VI_PIPE ViPipe, SC_BOOL bEnable)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    return;
}

static SC_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, SC_U8 u8Mode)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    pstSnsState->bSyncInit = SC_FALSE;

    switch(u8Mode)
    {
        case WDR_MODE_NONE:
        {
            pstSnsState->enWDRMode = WDR_MODE_NONE;
            printf("cmos_set_wdr_mode: linear mode\n");
            break;
        }

        case WDR_MODE_2To1_LINE:
        {
            pstSnsState->enWDRMode = WDR_MODE_2To1_LINE;
            printf("cmos_set_wdr_mode: 2to1 line WDR 1080p mode(60fps->30fps)\n");
            break;
        }

        default:
        {
            printf("NOT support this mode!\n");
            return SC_FAILURE;
        }
    }

    memset(pstSnsState->au32WDRIntTime, 0, sizeof(pstSnsState->au32WDRIntTime));
    return SC_SUCCESS;
}

static SC_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_S32 i = 0;

    CMOS_CHECK_POINTER(pstSnsRegsInfo);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    if((SC_FALSE == pstSnsState->bSyncInit) || (SC_FALSE == pstSnsRegsInfo->bConfig))
    {
        pstSnsState->astRegsInfo[0].enSnsType           = ISP_SNS_I2C_TYPE;
        pstSnsState->astRegsInfo[0].unComBus.s8I2cDev   = g_aunGc2093BusInfo[ViPipe].s8I2cDev;
        pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax = 3;
        pstSnsState->astRegsInfo[0].u32RegNum           = 15;

        if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax  = 3;
            pstSnsState->astRegsInfo[0].u32RegNum           += 2;
        }

        for(i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++)
        {
            pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate        = SC_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[i].u8DevAddr      = gc2093_i2c_addr;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32AddrByteNum = gc2093_addr_byte;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32DataByteNum = gc2093_data_byte;
        }


        /* Linear mode regs */
        pstSnsState->astRegsInfo[0].astI2cData[0].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[0].u32RegAddr     = GC2093_EXPTIME_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[1].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[1].u32RegAddr     = GC2093_EXPTIME_ADDR_H;

        pstSnsState->astRegsInfo[0].astI2cData[2].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32RegAddr     = GC2093_AGAIN_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[3].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[3].u32RegAddr     = GC2093_DGAIN_ADDR_H;
        pstSnsState->astRegsInfo[0].astI2cData[4].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[4].u32RegAddr     = GC2093_DGAIN_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[5].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[5].u32RegAddr     = GC2093_GAIN_ADDR_0X155;
        pstSnsState->astRegsInfo[0].astI2cData[6].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[6].u32RegAddr     = GC2093_GAIN_ADDR_0X31D;
        pstSnsState->astRegsInfo[0].astI2cData[7].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[7].u32RegAddr     = GC2093_GAIN_ADDR_0XC2;
        pstSnsState->astRegsInfo[0].astI2cData[8].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[8].u32RegAddr     = GC2093_GAIN_ADDR_0XCF;
        pstSnsState->astRegsInfo[0].astI2cData[9].u8DelayFrmNum  = 1;
        pstSnsState->astRegsInfo[0].astI2cData[9].u32RegAddr     = GC2093_GAIN_ADDR_0XD9;
        pstSnsState->astRegsInfo[0].astI2cData[10].u8DelayFrmNum = 1;
        pstSnsState->astRegsInfo[0].astI2cData[10].u32RegAddr    = GC2093_GAIN_ADDR_0X31D;
        pstSnsState->astRegsInfo[0].astI2cData[11].u8DelayFrmNum = 1;
        pstSnsState->astRegsInfo[0].astI2cData[11].u32RegAddr    = GC2093_AUTO_PREGAIN_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[12].u8DelayFrmNum = 1;
        pstSnsState->astRegsInfo[0].astI2cData[12].u32RegAddr    = GC2093_AUTO_PREGAIN_ADDR_H;

        pstSnsState->astRegsInfo[0].astI2cData[13].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[13].u32RegAddr    = GC2093_VMAX_ADDR_L;
        pstSnsState->astRegsInfo[0].astI2cData[14].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[14].u32RegAddr    = GC2093_VMAX_ADDR_H;


        /* WDR mode regs */
        if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            pstSnsState->astRegsInfo[0].astI2cData[15].u8DelayFrmNum = 1;
            pstSnsState->astRegsInfo[0].astI2cData[15].u32RegAddr    = GC2093_SHORT_EXPTIME_ADDR_L;
            pstSnsState->astRegsInfo[0].astI2cData[16].u8DelayFrmNum = 1;
            pstSnsState->astRegsInfo[0].astI2cData[16].u32RegAddr    = GC2093_SHORT_EXPTIME_ADDR_H;
        }

        pstSnsState->bSyncInit = SC_TRUE;
    }
    else
    {
        for(i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum - 2; i++)
        {
            if(pstSnsState->astRegsInfo[0].astI2cData[i].u32Data == pstSnsState->astRegsInfo[1].astI2cData[i].u32Data)
            {
                pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = SC_FALSE;
            }
            else
            {
                pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = SC_TRUE;
            }
        }

        if((pstSnsState->astRegsInfo[0].astI2cData[11].bUpdate == SC_TRUE)
            || (pstSnsState->astRegsInfo[0].astI2cData[12].bUpdate == SC_TRUE))
        {
            pstSnsState->astRegsInfo[0].astI2cData[11].bUpdate = SC_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[12].bUpdate = SC_TRUE;
        }

        pstSnsState->astRegsInfo[0].astI2cData[13].bUpdate = SC_TRUE;
        pstSnsState->astRegsInfo[0].astI2cData[14].bUpdate = SC_TRUE;
    }

    pstSnsState->astRegsInfo[0].bConfig = pstSnsRegsInfo->bConfig;
    memcpy(pstSnsRegsInfo, &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));
    memcpy(&pstSnsState->astRegsInfo[1], &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));

    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    /* Set real register for aec */
    if(pstSnsRegsInfo->bConfig)
    {
        gc2093_default_reg_init(ViPipe);
    }

    return SC_SUCCESS;
}

static SC_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
    SC_U8 u8SensorImageMode = 0;
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER(pstSensorImageMode);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    u8SensorImageMode = pstSnsState->u8ImgMode;
    pstSnsState->bSyncInit = SC_FALSE;

    if(pstSensorImageMode->f32Fps <= 30)
    {
        if(WDR_MODE_NONE == pstSnsState->enWDRMode)
        {
            if(SENSOR_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
            {
                u8SensorImageMode     = SENSOR_1080P_30FPS_LINEAR_MODE;
                pstSnsState->u32FLStd = SENSOR_VMAX_1080P30_LINEAR;
            }
            else
            {
                printf("%d: Not support! Width:%d, Height:%d, ImageMode:%d, SensorState:%d\n",
                    __LINE__, pstSensorImageMode->u16Width, pstSensorImageMode->u16Height, u8SensorImageMode,
                    pstSnsState->enWDRMode);
                return SC_FAILURE;
            }
        }
        else if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            if(SENSOR_RES_IS_WDR(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
            {
                u8SensorImageMode               = SENSOR_1080P_30FPS_2T1_WDR_MODE;
                pstSnsState->u32FLStd           = SENSOR_VMAX_1080P60TO30_WDR;
                g_astGc2093State[ViPipe].u32BRL = 1109;
            }
            else
            {
                printf("%d: Not support! Width:%d, Height:%d, ImageMode:%d, SensorState:%d\n",
                    __LINE__, pstSensorImageMode->u16Width, pstSensorImageMode->u16Height, u8SensorImageMode,
                    pstSnsState->enWDRMode);
                return SC_FAILURE;
            }
        }
        else
        {
            printf("%d: Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                __LINE__, pstSensorImageMode->u16Width, pstSensorImageMode->u16Height, pstSensorImageMode->f32Fps,
                pstSnsState->enWDRMode);
            return SC_FAILURE;
        }
    }

    if((SC_TRUE == pstSnsState->bInit) && (u8SensorImageMode == pstSnsState->u8ImgMode))
    {
        /* Don't need to switch SensorImageMode */
        return SC_FAILURE;
    }

    pstSnsState->u8ImgMode = u8SensorImageMode;
    pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    return SC_SUCCESS;
}

static SC_VOID gc2093_global_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    pstSnsState->bInit     = SC_FALSE;
    pstSnsState->bSyncInit = SC_FALSE;
    pstSnsState->u8ImgMode = SENSOR_1080P_30FPS_LINEAR_MODE;
    pstSnsState->enWDRMode = WDR_MODE_NONE;
    pstSnsState->u32FLStd  = SENSOR_VMAX_1080P30_LINEAR;
    pstSnsState->au32FL[0] = SENSOR_VMAX_1080P30_LINEAR;
    pstSnsState->au32FL[1] = SENSOR_VMAX_1080P30_LINEAR;

    memset(&pstSnsState->astRegsInfo[0], 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&pstSnsState->astRegsInfo[1], 0, sizeof(ISP_SNS_REGS_INFO_S));
}

static SC_S32 cmos_set_mirror_flip(VI_PIPE ViPipe, int mode)
{
    SC_S32 ret = 0;

    printf("cmos_set_mirror_flip: mode=%d\n", mode);
    switch(mode)
    {
        case ISP_SNS_NORMAL:
        {
            ret = gc2093_flip_off_mirror_off(ViPipe);
            break;
        }

        case ISP_SNS_MIRROR:
        {
            ret = gc2093_flip_off_mirror_on(ViPipe);
            break;
        }

        case ISP_SNS_FLIP:
        {
            ret = gc2093_flip_on_mirror_off(ViPipe);
            break;
        }

        case ISP_SNS_MIRROR_FLIP:
        {
            ret = gc2093_flip_on_mirror_on(ViPipe);
            break;
        }

        default:
        {
        break;
        }
    }

    return ret;
}

static SC_S32 cmos_gc2093_ctl(VI_PIPE ViPipe, ISP_CMOS_SENSOR_CTL *pSensorCtl)
{
    SC_U8  u8CtlCode = 0;
    SC_S32 ret       = 0;
    SC_S32 mode      = 0;

    if(!pSensorCtl)
    {
         printf("sensor ctl failed!\n");
         return SC_ERR_ISP_INVALID_ADDR;
    }

    mode      = *(SC_S32 *)pSensorCtl->pCtlData;
    u8CtlCode = pSensorCtl->u8CtlCode;
    switch(u8CtlCode)
    {
        case SNS_CB_EVENT_FLIP_MIRROR:
        {
            ret = cmos_set_mirror_flip(ViPipe, mode);
            break;
        }

        default:
        {
            printf("error ctl code\n");
            ret = SC_FAILURE;
            break;
        }
    }

    return ret;
}

static SC_S32 cmos_init_gc2093_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    CMOS_CHECK_POINTER(pstSensorExpFunc);

    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));
    pstSensorExpFunc->pfn_cmos_sensor_init        = gc2093_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit        = gc2093_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = gc2093_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode     = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode       = cmos_set_wdr_mode;

    pstSensorExpFunc->pfn_cmos_get_isp_default  = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    pstSensorExpFunc->pfn_cmos_sns_power_on  = cmos_power_on;
    pstSensorExpFunc->pfn_cmos_sns_power_off = cmos_power_off;
    pstSensorExpFunc->pfn_cmos_sns_ctl       = cmos_gc2093_ctl;

    return SC_SUCCESS;
}


/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/
static SC_S32 gc2093_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunGc2093BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return SC_SUCCESS;
}

static SC_S32 gc2093_ctx_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);

    if(SC_NULL == pastSnsStateCtx)
    {
        pastSnsStateCtx = (ISP_SNS_STATE_S *)malloc(sizeof(ISP_SNS_STATE_S));
        if(SC_NULL == pastSnsStateCtx)
        {
            printf("Isp[%d] SnsCtx malloc memory failed!\n", ViPipe);
            return SC_ERR_ISP_NOMEM;
        }
    }

    memset(pastSnsStateCtx, 0, sizeof(ISP_SNS_STATE_S));

    SENSOR_SET_CTX(ViPipe, pastSnsStateCtx);

    return SC_SUCCESS;
}

static SC_VOID gc2093_ctx_exit(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
    SENSOR_FREE(pastSnsStateCtx);
    SENSOR_RESET_CTX(ViPipe);
}

static SC_S32 gc2093_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    SC_S32 s32Ret = SC_FAILURE;

    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = gc2093_ctx_init(ViPipe);
    if(SC_SUCCESS != s32Ret)
    {
        return SC_FAILURE;
    }

    stSnsAttrInfo.eSensorId = GC2093_ID;

    s32Ret  = cmos_init_gc2093_exp_function(&stIspRegister.stSnsExp);
    s32Ret |= SC_MPI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }

    s32Ret  = cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret |= SC_MPI_AE_SensorRegCallBack(ViPipe, pstAeLib, &stSnsAttrInfo, &stAeRegister);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret  = cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret |= SC_MPI_AWB_SensorRegCallBack(ViPipe, pstAwbLib, &stSnsAttrInfo, &stAwbRegister);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor register callback function to awb lib failed!\n");
        return s32Ret;
    }

    return SC_SUCCESS;
}

static SC_S32 gc2093_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    SC_S32 s32Ret = SC_FAILURE;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = SC_MPI_ISP_SensorUnRegCallBack(ViPipe, GC2093_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = SC_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, GC2093_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = SC_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, GC2093_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function to awb lib failed!\n");
        return s32Ret;
    }

    gc2093_ctx_exit(ViPipe);

    return SC_SUCCESS;
}

static SC_S32 gc2093_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
{
    CMOS_CHECK_POINTER(pstInitAttr);

    g_au32InitExposure[ViPipe]  = pstInitAttr->u32Exposure;
    g_au32LinesPer500ms[ViPipe] = pstInitAttr->u32LinesPer500ms;
    g_au16InitWBGain[ViPipe][0] = pstInitAttr->u16WBRgain;
    g_au16InitWBGain[ViPipe][1] = pstInitAttr->u16WBGgain;
    g_au16InitWBGain[ViPipe][2] = pstInitAttr->u16WBBgain;
    g_au16SampleRgain[ViPipe]   = pstInitAttr->u16SampleRgain;
    g_au16SampleBgain[ViPipe]   = pstInitAttr->u16SampleBgain;

    return SC_SUCCESS;
}

ISP_SNS_OBJ_S stSnsGc2093Obj =
{
    .pfnRegisterCallback    = gc2093_register_callback,
    .pfnUnRegisterCallback  = gc2093_unregister_callback,
    .pfnStandby             = gc2093_standby,
    .pfnRestart             = gc2093_restart,
    .pfnMirrorFlip          = SC_NULL,
    .pfnWriteReg            = gc2093_write_register,
    .pfnReadReg             = gc2093_read_register,
    .pfnSetBusInfo          = gc2093_set_bus_info,
    .pfnSetInit             = gc2093_set_init
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __GC2093_CMOS_C_ */
