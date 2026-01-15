/**
 * @file     sc4210_cmos.c
 * @brief    SMART SEMS SC4210 SENSOR控制回调接口的实现
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

#if !defined(__SC4210_CMOS_C_)
#define __SC4210_CMOS_C_


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


#define SC4210_ID 4210


/****************************************************************************
 * Global variables                                                         *
 ****************************************************************************/
ISP_SNS_STATE_S *g_pastSc4210[ISP_MAX_PIPE_NUM] = {SC_NULL};

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_pastSc4210[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_pastSc4210[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev)       (g_pastSc4210[dev] = SC_NULL)

ISP_SNS_COMMBUS_U g_aunSc4210BusInfo[ISP_MAX_PIPE_NUM] =
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

static SC_TUNING_BIN_FILE_S g_acTuningBinName[ISP_MAX_PIPE_NUM] =
{
    [0] = {
              .acLinearBinName = "/local/tuning/sc4210/sc4210_tuning.bin",
              .acWdrBinName    = "/local/tuning/sc4210/sc4210_tuning_hdr.bin"
          },

    [1] = {
              .acLinearBinName = "/local/tuning/sc4210/sc4210_tuning.bin",
              .acWdrBinName    = "/local/tuning/sc4210/sc4210_tuning_hdr.bin"
          },
};



/****************************************************************************
 * extern                                                                   *
 ****************************************************************************/
extern const unsigned int sc4210_i2c_addr;
extern unsigned int sc4210_addr_byte;
extern unsigned int sc4210_data_byte;

extern int  sc4210_init(VI_PIPE ViPipe);
extern void sc4210_exit(VI_PIPE ViPipe);
extern void sc4210_standby(VI_PIPE ViPipe);
extern void sc4210_restart(VI_PIPE ViPipe);
extern void sc4210_default_reg_init(VI_PIPE ViPipe);
extern int  sc4210_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  sc4210_read_register(VI_PIPE ViPipe, int addr);
extern int  sc4210_flip_off_mirror_off(VI_PIPE ViPipe);
extern int  sc4210_flip_on_mirror_off(VI_PIPE ViPipe);
extern int  sc4210_flip_off_mirror_on(VI_PIPE ViPipe);
extern int  sc4210_flip_on_mirror_on(VI_PIPE ViPipe);



/****************************************************************************
 * local variables                                                          *
 ****************************************************************************/
/* Sensor register address */
#define SC4210_EXPO_ADDR       (0x3e00)
#define SC4210_EXPO_S_ADDR     (0x3e04)
#define SC4210_AGAIN_ADDR      (0x3e08)
#define SC4210_DGAIN_ADDR      (0x3e06)
#define SC4210_AGAIN_S_ADDR    (0x3e12)
#define SC4210_DGAIN_S_ADDR    (0x3e10)
#define SC4210_FRM_LEN_ADDR    (0x320e)
#define SC4210_EXPO_MAX_S_ADDR (0x3e23)

#define SENSOR_FULL_LINES_MAX          (0x3FFFF)
#define SENSOR_FULL_LINES_MAX_2TO1_WDR (0x3FFFF)

#define SENSOR_INCREASE_LINES     (0)
#define SENSOR_VMAX_4MP30_LINEAR  (1500 + SENSOR_INCREASE_LINES)
#define SENSOR_VMAX_4MP60TO30_WDR (3000 + SENSOR_INCREASE_LINES)

#define SENSOR_4MP_30FPS_LINEAR_MODE  (1)
#define SENSOR_4MP_30FPS_2T1_WDR_MODE (2)

#define SENSOR_RES_IS_4MP(w, h) ((w) <= 2560 && (h) <= 1440)

#define HIGH_8BITS(x) (((x) & 0xff00) >> 8)
#define LOW_8BITS(x)  ((x) & 0x00ff)



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
    pstAeSnsDft->u32Hmax         = 2720;

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
            pstAeSnsDft->u32MaxAgain   = 43883;
            pstAeSnsDft->u32MinAgain   = 1024;
            pstAeSnsDft->u32MaxDgain   = 32270;
            pstAeSnsDft->u32MinDgain   = 1024;
            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd * 2 - 8;
            pstAeSnsDft->u32MinIntTime = 1;

            break;
        }

        case WDR_MODE_2To1_LINE:
        {
            /* Wdr mode*/
            pstAeSnsDft->u32MaxAgain   = 43883;
            pstAeSnsDft->u32MinAgain   = 1024;
            pstAeSnsDft->u32MaxDgain   = 32270;
            pstAeSnsDft->u32MinDgain   = 1024;
            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd * 2 - 18;
            pstAeSnsDft->u32MinIntTime = 1;

            break;
        }
    }

    return SC_SUCCESS;
}

static SC_VOID cmos_fps_set(VI_PIPE ViPipe, SC_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SC_U32 u32VMAX = SENSOR_VMAX_4MP30_LINEAR;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    switch(pstSnsState->u8ImgMode)
    {
        case SENSOR_4MP_30FPS_2T1_WDR_MODE:
        {
            if((f32Fps <= pstAeSnsDft->f32MaxFps) && (f32Fps >= 15.22))
            {
                u32VMAX = SENSOR_VMAX_4MP60TO30_WDR * pstAeSnsDft->f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }

            u32VMAX = (u32VMAX > SENSOR_FULL_LINES_MAX_2TO1_WDR) ? SENSOR_FULL_LINES_MAX_2TO1_WDR : u32VMAX;
            break;
        }

        case SENSOR_4MP_30FPS_LINEAR_MODE:
        {
            if((f32Fps <= pstAeSnsDft->f32MaxFps) && (f32Fps >= 0.119))
            {
                u32VMAX = SENSOR_VMAX_4MP30_LINEAR * pstAeSnsDft->f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
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
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = HIGH_8BITS(u32VMAX);
        pstSnsState->astRegsInfo[0].astI2cData[8].u32Data = LOW_8BITS(u32VMAX);

    }
    else
    {
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = HIGH_8BITS(u32VMAX);
        pstSnsState->astRegsInfo[0].astI2cData[8].u32Data = LOW_8BITS(u32VMAX);
    }

    pstSnsState->u32FLStd         = u32VMAX;
    pstAeSnsDft->f32Fps           = f32Fps;
    pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * f32Fps / 2;
    pstAeSnsDft->u32FullLinesStd  = pstSnsState->u32FLStd;
    if(WDR_MODE_NONE == pstSnsState->enWDRMode)
    {
        pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd * 2 - 8;
    }
    else
    {
        pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd * 2 - 18;
    }

    pstSnsState->au32FL[0]    = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];

    return;
}

static SC_VOID cmos_slow_framerate_set(VI_PIPE ViPipe, SC_U32 u32FullLines, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        u32FullLines = (u32FullLines > SENSOR_FULL_LINES_MAX_2TO1_WDR) ? SENSOR_FULL_LINES_MAX_2TO1_WDR : u32FullLines;
        pstSnsState->au32FL[0] = u32FullLines;

    }
    else
    {
        u32FullLines = (u32FullLines > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32FullLines;
        pstSnsState->au32FL[0] = u32FullLines;
    }

    if(WDR_MODE_NONE == pstSnsState->enWDRMode)
    {
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = HIGH_8BITS(pstSnsState->au32FL[0]);
        pstSnsState->astRegsInfo[0].astI2cData[8].u32Data = LOW_8BITS(pstSnsState->au32FL[0]);
    }
    else if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = HIGH_8BITS(pstSnsState->au32FL[0]);
        pstSnsState->astRegsInfo[0].astI2cData[8].u32Data = LOW_8BITS(pstSnsState->au32FL[0]);
    }
    else
    {
    }

    pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];
    if(WDR_MODE_NONE == pstSnsState->enWDRMode)
    {
        pstAeSnsDft->u32MaxIntTime = pstSnsState->au32FL[0] * 2 - 8;
    }
    else
    {
        pstAeSnsDft->u32MaxIntTime = pstSnsState->au32FL[0] * 2 - 18;
    }

    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static SC_VOID cmos_inttime_update(VI_PIPE ViPipe, SC_U32 u32IntTime)
{
    static SC_BOOL bFirst[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1};

    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        if(bFirst[ViPipe]) /* short exposure */
        {
            pstSnsState->au32WDRIntTime[0] = u32IntTime;
            pstSnsState->astRegsInfo[0].astI2cData[9].u32Data = (u32IntTime >> 4) & 0xFF;
            pstSnsState->astRegsInfo[0].astI2cData[10].u32Data = (u32IntTime << 4) & 0xF0;

            bFirst[ViPipe] = SC_FALSE;
        }

        else   /* long exposure */
        {
            pstSnsState->au32WDRIntTime[1] = u32IntTime;
            pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = (u32IntTime >> 12) & 0x0F;
            pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = (u32IntTime >> 4) & 0xFF;
            pstSnsState->astRegsInfo[0].astI2cData[2].u32Data = (u32IntTime << 4) & 0xF0;

            bFirst[ViPipe] = SC_TRUE;
        }
    }
    else
    {
        //printf("sns pp=%d, ln=%d\n", ViPipe, u32IntTime);
        pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = (u32IntTime >> 12) & 0x0F;
        pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = (u32IntTime >> 4) & 0xFF;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32Data = (u32IntTime << 4) & 0xF0;
        bFirst[ViPipe] = SC_TRUE;
    }

    return;
}

/*
 * Gain table value is gain value, gain table index is gain register value.
 * sensor gain dB value = 20 * log(gain_table[i] / 1024)
 * gain_table[i] = 1024 * 10 ^ (sensor gain dB value / 20)
 */
static SC_U32 again_table[172] =
{
     1024, 1056, 1088, 1120, 1151, 1183, 1215, 1248, 1280, 1311,
     1343, 1376, 1408, 1439, 1471, 1504, 1535, 1567, 1600, 1632,
     1664, 1695, 1727, 1759, 1791, 1823, 1856, 1887, 1919, 1951,
     1982, 2015, 2047, 2112, 2176, 2240, 2303, 2367, 2431, 2496,
     2560, 2623, 2687, 2752, 2784, 2872, 2960, 3046, 3131, 3219,
     3305, 3394, 3481, 3566, 3654, 3743, 3830, 3915, 4002, 4090,
     4176, 4263, 4353, 4439, 4526, 4611, 4702, 4789, 4873, 4963,
     5050, 5138, 5221, 5306, 5398, 5486, 5569, 5745, 5919, 6092,
     6270, 6438, 6611, 6788, 6963, 7133, 7308, 7486, 7661, 7830,
     8003, 8180, 8352, 8527, 8705, 8877, 9053, 9221, 9403, 9578,
     9745, 9926, 10099, 10275, 10442, 10624, 10796, 10972, 11137, 11489,
    11838, 12184, 12540, 12876, 13237, 13576, 13925, 14282, 14615, 14972,
    15321, 15660, 16006, 16360, 16703, 17052, 17410, 17754, 18105, 18462,
    18806, 19155, 19489, 19851, 20197, 20549, 20883, 21247, 21592, 21943,
    22274, 22977, 23675, 24366, 25078, 25751, 26472, 27151, 27848, 28562,
    29261, 29943, 30640, 31318, 32047, 32718, 33442, 34103, 34817, 35505,
    36207, 36923, 37609, 38308, 38976, 39700, 40392, 41096, 41763, 42491,
    43181, 43883
};

static SC_U32 dgain_table[160] =
{
     1024, 1056, 1088, 1120, 1151, 1183, 1215, 1248, 1280, 1311,
     1343, 1376, 1408, 1439, 1471, 1504, 1535, 1567, 1600, 1632,
     1664, 1695, 1727, 1759, 1791, 1823, 1856, 1887, 1919, 1951,
     1982, 2015, 2047, 2112, 2176, 2240, 2303, 2367, 2431, 2496,
     2560, 2623, 2687, 2752, 2817, 2879, 2943, 3008, 3071, 3135,
     3201, 3264, 3328, 3390, 3457, 3518, 3583, 3650, 3713, 3773,
     3839, 3902, 3970, 4029, 4095, 4224, 4353, 4480, 4605, 4734,
     4861, 4992, 5120, 5245, 5374, 5505, 5633, 5758, 5885, 6015,
     6141, 6270, 6401, 6528, 6657, 6781, 6915, 7043, 7166, 7299,
     7426, 7556, 7678, 7803, 7939, 8068, 8190, 8448, 8705, 8959,
     9210, 9468,  9722, 9983, 10240, 10490, 10747, 11010, 11266, 11515,
    11770, 12030, 12282, 12540, 12802, 13055, 13313, 13561, 13829, 14086,
    14331, 14598, 14852, 15111, 15356, 15624, 15878, 16136, 16379, 16896,
    17410, 17918, 18441, 18936, 19467, 19966, 20478, 21003, 21493, 22019,
    22532, 23030, 23539, 24060, 24564, 25078, 25603, 26109, 26625, 27120,
    27656, 28170, 28661, 29194, 29702, 30220, 30711, 31246, 31754, 32270
};

static SC_VOID cmos_again_calc_table(VI_PIPE ViPipe, SC_U32 *pu32AgainLin, SC_U32 *pu32AgainDb)
{
    int i = 0;

    CMOS_CHECK_POINTER_VOID(pu32AgainLin);
    CMOS_CHECK_POINTER_VOID(pu32AgainDb);

    if(*pu32AgainLin >= again_table[171])
    {
        *pu32AgainLin = again_table[171];
        *pu32AgainDb  = 171;
        return;
    }

    for(i = 1; i < 172; i++)
    {
        if(*pu32AgainLin < again_table[i])
        {
            *pu32AgainLin = again_table[i - 1];
            *pu32AgainDb  = i - 1;
            break;
        }
    }

    return;
}

static SC_VOID cmos_dgain_calc_table(VI_PIPE ViPipe, SC_U32 *pu32DgainLin, SC_U32 *pu32DgainDb)
{
    int i = 0;

    CMOS_CHECK_POINTER_VOID(pu32DgainLin);
    CMOS_CHECK_POINTER_VOID(pu32DgainDb);

    if(*pu32DgainLin >= dgain_table[159])
    {
        *pu32DgainLin = dgain_table[159];
        *pu32DgainDb  = 159;
        return ;
    }

    for(i = 1; i < 160; i++)
    {
        if(*pu32DgainLin < dgain_table[i])
        {
            *pu32DgainLin = dgain_table[i - 1];
            *pu32DgainDb  = i - 1;
            break;
        }
    }

    return;
}

static SC_VOID cmos_gains_update(VI_PIPE ViPipe, SC_U32 u32Again, SC_U32 u32Dgain)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    /* Analog coarse gain and fine gain initial value: 1.00X 0.00dB */
    SC_U8 aCoarseGain = 0;
    SC_U8 aFineGain   = 0;
    SC_U8 u8Reg0x3e08 = 0x03;
    SC_U8 u8Reg0x3e09 = 0x40;

    /* Digital coarse gain and fine gain initial value: 1.00X 0.00dB */
    SC_U8 dCoarseGain = 0;
    SC_U8 dFineGain   = 0;
    SC_U8 u8Reg0x3e06 = 0x00;
    SC_U8 u8Reg0x3e07 = 0x80;


    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    /* Calc analog coarse gain and fine gain register value.
     * u32Again(tab_index)   coarse gain and fine gain reg_value(step is 2)     gain value
     * [  0.. 31]                0x03          [0x40..0x7F]                         1.0X
     * [ 32.. 43]                0x07          [0x40..0x56]                         2.0X
     * [ 44.. 75]                0x23          [0x40..0x7F]                       2.750X
     * [ 76..107]                0x27          [0x40..0x7F]                       5.500X
     * [108..139]                0x2F          [0x40..0x7F]                      11.000X
     * [140..171]                0x3F          [0x40..0x7F]                      22.000X..43.656X
     */
    //printf("sns pp=%d, ag=%d, dg=%d\n", ViPipe, u32Again, u32Dgain);
    if(u32Again < 44)
    {
        /* Analog fine gain < 2.72x */
        if(u32Again < 32)
        {
            aFineGain = 2 * u32Again;
        }
        else
        {
            aFineGain   = 2 * (u32Again - 32);
            aCoarseGain = 2;
        }
    }
    else
    {
        /* Analog fine gain > 2.72x */
        for(aCoarseGain = 1; aCoarseGain <= 4; aCoarseGain++)
        {
            if(u32Again < (32 * aCoarseGain + 44))
            {
                break;
            }
        }

        aFineGain = 2 * (u32Again - 44 - 32 * (aCoarseGain - 1));
    }

    for( ; aCoarseGain >= 2; aCoarseGain--)
    {
        /* Analog coarse gain register low five bits: 0x03, 0x07, 0x0F or 0x1F */
        u8Reg0x3e08 = (u8Reg0x3e08 << 1) | 0x01;
    }

    /* Analog coarse gain register: 0x03, 0x07, 0x23, 0x27, 0x2F or 0x3F */
    u8Reg0x3e08 = (u32Again >= 44) ? (u8Reg0x3e08 | 0x20) : (u8Reg0x3e08 & 0x1F);

    /* Analog fine gain register: from 0x40 to 0x7F(special value is from 0x40 to 0x56), step is 2. */
    u8Reg0x3e09 += aFineGain;


    /* Calc digital coarse gain and fine gain register value.
     * u32Again(tab_index)   coarse gain and fine gain reg_value(step is 4)     gain value
     * [  0.. 31]                0x00          [0x80..0xFC]                       1.0X
     * [ 32.. 63]                0x01          [0x80..0xFC]                       2.0X
     * [ 64.. 95]                0x03          [0x80..0xFC]                       4.0X
     * [ 96..127]                0x07          [0x80..0xFC]                       8.0X
     * [128..159]                0x0F          [0x80..0xFC]                      16.0X..31.5X
     */
    for(dCoarseGain = 1; dCoarseGain <= 5; dCoarseGain++)
    {
        if(u32Dgain < (32 * dCoarseGain))
        {
            break;
        }
    }

    dFineGain = 4 * (u32Dgain - 32 * (dCoarseGain - 1));
    for( ; dCoarseGain >= 2; dCoarseGain--)
    {
        /* Digital coarse gain register: 0x00, 0x01, 0x03, 0x07 or 0x0F */
        u8Reg0x3e06 = (u8Reg0x3e06 << 1) | 0x01;
    }

    /* Digital fine gain register: from 0x80 to 0xFC, step is 4. */
    u8Reg0x3e07 += dFineGain;


    /* Set gain resister value */
    //printf("sns pp=%d, agReg=0x%x, agReg=0x%x, dgReg=0x%x, dgReg=0x%x\n", ViPipe, u8Reg0x3e08, u8Reg0x3e09, u8Reg0x3e06, u8Reg0x3e07);
    pstSnsState->astRegsInfo[0].astI2cData[3].u32Data = u8Reg0x3e06;
    pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = u8Reg0x3e07;
    pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = u8Reg0x3e08;
    pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = u8Reg0x3e09;
    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        pstSnsState->astRegsInfo[0].astI2cData[11].u32Data = u8Reg0x3e06;
        pstSnsState->astRegsInfo[0].astI2cData[12].u32Data = u8Reg0x3e07;
        pstSnsState->astRegsInfo[0].astI2cData[13].u32Data = u8Reg0x3e08;
        pstSnsState->astRegsInfo[0].astI2cData[14].u32Data = u8Reg0x3e09;
    }

    return;
}

static SC_VOID cmos_get_inttime_max(VI_PIPE ViPipe, SC_U16 u16ManRatioEnable, SC_U32 *au32Ratio,
    SC_U32 *au32IntTimeMax, SC_U32 *au32IntTimeMin, SC_U32 *pu32LFMaxIntTime)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    CMOS_CHECK_POINTER_VOID(au32Ratio);
    CMOS_CHECK_POINTER_VOID(au32IntTimeMax);
    CMOS_CHECK_POINTER_VOID(au32IntTimeMin);
    CMOS_CHECK_POINTER_VOID(pu32LFMaxIntTime);
    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    static SC_U32 u32PreMaxShortExposure = 0;

    SC_U32 u32IntTimeMaxTmp0    = 0;
    SC_U32 u32IntTimeMaxTmp     = 0;
    SC_U32 u32ShortTimeMinLimit = 5;
    SC_U32 au32shortMaxMin      = 2;
    SC_U32 u32MaxShortExposure  = 0;

    SC_U8 u8Reg0x3e23 = 0x00;
    SC_U8 u8Reg0x3e24 = 0x00;

    au32IntTimeMin[0] = u32ShortTimeMinLimit;
    au32IntTimeMin[1] = au32IntTimeMin[0] * au32Ratio[0] >> 6;
    au32Ratio[0]      = (au32Ratio[0] > 0x400) ? 0x400 : au32Ratio[0];
    u32IntTimeMaxTmp  = ((pstSnsState->au32FL[0] * 2 - 18) * 0x40) / (au32Ratio[0] + 0x40) / 4 * 4;
    u32IntTimeMaxTmp0 = ((pstSnsState->au32FL[1] * 2 - 18 - pstSnsState->au32WDRIntTime[0]) * 0x40)  / DIV_0_TO_1(au32Ratio[0]);
    u32IntTimeMaxTmp  = (u32IntTimeMaxTmp0 < u32IntTimeMaxTmp) ? u32IntTimeMaxTmp0 : u32IntTimeMaxTmp;
    au32shortMaxMin   = ((pstSnsState->au32FL[0] * 2 - 18) * 0x40) / 0x440;
    u32IntTimeMaxTmp  = (u32IntTimeMaxTmp < au32shortMaxMin) ? au32shortMaxMin : u32IntTimeMaxTmp;
    u32IntTimeMaxTmp  = (u32IntTimeMaxTmp > 0xffc) ? 0xffc : u32IntTimeMaxTmp;

    au32IntTimeMax[0]   = u32IntTimeMaxTmp;
    au32IntTimeMax[1]   = au32IntTimeMax[0] * au32Ratio[0] >> 6;
    u32MaxShortExposure = au32IntTimeMax[0] / 4 * 2 + 4;
    if(u32PreMaxShortExposure>u32MaxShortExposure)
    {
        if(u32PreMaxShortExposure-u32MaxShortExposure >= pstSnsState->u32FLStd - 2216)
        {
            u32MaxShortExposure=u32PreMaxShortExposure + 2216 - pstSnsState->u32FLStd;
        }
    }

    u8Reg0x3e23 = (u32MaxShortExposure >> 8) & 0xFF;
    u8Reg0x3e24 = u32MaxShortExposure & 0xFF;
    if(u32PreMaxShortExposure != u32MaxShortExposure)
    {
        pstSnsState->astRegsInfo[0].astI2cData[15].u32Data = u8Reg0x3e23;
        pstSnsState->astRegsInfo[0].astI2cData[16].u32Data = u8Reg0x3e24;
    }

    u32PreMaxShortExposure = u32MaxShortExposure;

    return;
}

/* Only used in LINE_WDR mode */
static SC_VOID cmos_ae_fswdr_attr_set(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr)
{
    CMOS_CHECK_POINTER_VOID(pstAeFSWDRAttr);

    genFSWDRMode[ViPipe] = pstAeFSWDRAttr->enFSWDRMode;
    gu32MaxTimeGetCnt[ViPipe] = 0;
}

static SC_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);

    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set= cmos_slow_framerate_set;
    pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table  = cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table  = cmos_dgain_calc_table;
    pstExpFuncs->pfn_cmos_get_inttime_max   = cmos_get_inttime_max;
    pstExpFuncs->pfn_cmos_ae_fswdr_attr_set = cmos_ae_fswdr_attr_set;

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
    }

    pstDef->stSensorMode.u32SensorID  = SC4210_ID;
    pstDef->stSensorMode.u8SensorMode = pstSnsState->u8ImgMode;
    memcpy(&pstDef->stDngColorParam, &g_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));

    switch(pstSnsState->u8ImgMode)
    {
        default:
        case SENSOR_4MP_30FPS_LINEAR_MODE:
        {
            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 12;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel   = 4095;
            break;
        }

        case SENSOR_4MP_30FPS_2T1_WDR_MODE:
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
    pstDef->stSensorMode.stDngRawFormat.enCfaLayout = CFALAYOUT_TYPE_RECTANGULAR;
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

    SC_U32 u32FullLines_5Fps  = 0;
    SC_U32 u32MaxIntTime_5Fps = 0;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
    {
        return;
    }
    else
    {
        if(SENSOR_4MP_30FPS_LINEAR_MODE == pstSnsState->u8ImgMode)
        {
            u32FullLines_5Fps = (SENSOR_VMAX_4MP30_LINEAR * 30) / 5;
        }
        else
        {
            return;
        }
    }

    u32FullLines_5Fps  = (u32FullLines_5Fps > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : u32FullLines_5Fps;
    u32MaxIntTime_5Fps = u32FullLines_5Fps * 2 - 6;

    if(bEnable) /* setup for ISP pixel calibration mode */
    {
        sc4210_write_register(ViPipe, 0x3e08, 0x03);
        sc4210_write_register(ViPipe, 0x3e09, 0x40);
        sc4210_write_register(ViPipe, 0x320e, (u32FullLines_5Fps & 0xFF00) >> 8);
        sc4210_write_register(ViPipe, 0x320f, u32FullLines_5Fps & 0xFF);
        sc4210_write_register(ViPipe, 0x3e01, (u32MaxIntTime_5Fps & 0x0FF0) >> 4);
        sc4210_write_register (ViPipe, 0x3e02, (u32MaxIntTime_5Fps  & 0xF)<<4);
    }
    else /* setup for ISP 'normal mode' */
    {
        pstSnsState->u32FLStd = (pstSnsState->u32FLStd > SENSOR_FULL_LINES_MAX) ? SENSOR_FULL_LINES_MAX : pstSnsState->u32FLStd;
        sc4210_write_register(ViPipe, 0x320e, (pstSnsState->u32FLStd & 0xFF00) >> 8);
        sc4210_write_register(ViPipe, 0x320f, pstSnsState->u32FLStd & 0xFF);
        pstSnsState->bSyncInit = SC_FALSE;
    }

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
            pstSnsState->u32FLStd  = SENSOR_VMAX_4MP30_LINEAR;

            printf("cmos_set_wdr_mode: linear mode\n");
            break;
        }

        case WDR_MODE_2To1_LINE:
        {
            pstSnsState->enWDRMode = WDR_MODE_2To1_LINE;
            pstSnsState->u32FLStd  = SENSOR_VMAX_4MP60TO30_WDR;

            printf("cmos_set_wdr_mode: 2to1 line WDR 1440H mode(60fps->30fps)\n");
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
        pstSnsState->astRegsInfo[0].unComBus.s8I2cDev   = g_aunSc4210BusInfo[ViPipe].s8I2cDev;
        pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        pstSnsState->astRegsInfo[0].u32RegNum           = 9;
        if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            pstSnsState->astRegsInfo[0].u32RegNum           = 17;
            pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        }

        for(i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++)
        {
            pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate        = SC_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[i].u8DevAddr      = sc4210_i2c_addr;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32AddrByteNum = sc4210_addr_byte;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32DataByteNum = sc4210_data_byte;
        }

        /* Linear mode exposure registers, make exposure and gain effective at the same time. */
        pstSnsState->astRegsInfo[0].astI2cData[0].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[0].u32RegAddr    = SC4210_EXPO_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[1].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[1].u32RegAddr    = SC4210_EXPO_ADDR + 1;
        pstSnsState->astRegsInfo[0].astI2cData[2].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32RegAddr    = SC4210_EXPO_ADDR + 2;

        /* Linear mode gain registers */
        pstSnsState->astRegsInfo[0].astI2cData[3].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[3].u32RegAddr    = SC4210_DGAIN_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[4].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[4].u32RegAddr    = SC4210_DGAIN_ADDR + 1;
        pstSnsState->astRegsInfo[0].astI2cData[5].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[5].u32RegAddr    = SC4210_AGAIN_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[6].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[6].u32RegAddr    = SC4210_AGAIN_ADDR + 1;

        /* Frame length registers */
        pstSnsState->astRegsInfo[0].astI2cData[7].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[7].u32RegAddr    = SC4210_FRM_LEN_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[8].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[8].u32RegAddr    = SC4210_FRM_LEN_ADDR + 1;

        if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            /* DOL 2T1 mode exposure registers */
            pstSnsState->astRegsInfo[0].astI2cData[9].u8DelayFrmNum  = 0;
            pstSnsState->astRegsInfo[0].astI2cData[9].u32RegAddr     = SC4210_EXPO_S_ADDR;
            pstSnsState->astRegsInfo[0].astI2cData[10].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[10].u32RegAddr    = SC4210_EXPO_S_ADDR + 1;

            /* DOL 2T1 mode gain registers */
            pstSnsState->astRegsInfo[0].astI2cData[11].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[11].u32RegAddr    = SC4210_DGAIN_S_ADDR;
            pstSnsState->astRegsInfo[0].astI2cData[12].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[12].u32RegAddr    = SC4210_DGAIN_S_ADDR + 1;
            pstSnsState->astRegsInfo[0].astI2cData[13].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[13].u32RegAddr    = SC4210_AGAIN_S_ADDR;
            pstSnsState->astRegsInfo[0].astI2cData[14].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[14].u32RegAddr    = SC4210_AGAIN_S_ADDR + 1;

            /* DOL 2T1 mode max short exposure registers */
            pstSnsState->astRegsInfo[0].astI2cData[15].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[15].u32RegAddr    = SC4210_EXPO_MAX_S_ADDR;
            pstSnsState->astRegsInfo[0].astI2cData[16].u8DelayFrmNum = 0;
            pstSnsState->astRegsInfo[0].astI2cData[16].u32RegAddr    = SC4210_EXPO_MAX_S_ADDR + 1;
        }

        pstSnsState->bSyncInit = SC_TRUE;
    }
    else
    {
        for(i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++)
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
    }

    pstSnsRegsInfo->bConfig = SC_FALSE;
    memcpy(pstSnsRegsInfo, &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));
    memcpy(&pstSnsState->astRegsInfo[1], &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));

    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    /* Set real register for aec */
    sc4210_default_reg_init(ViPipe);

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
            if(SENSOR_RES_IS_4MP(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
            {
                u8SensorImageMode = SENSOR_4MP_30FPS_LINEAR_MODE;
                pstSnsState->u32FLStd = SENSOR_VMAX_4MP30_LINEAR;
            }
            else
            {
                printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                          pstSensorImageMode->u16Width,
                          pstSensorImageMode->u16Height,
                          pstSensorImageMode->f32Fps,
                          pstSnsState->enWDRMode);
                return SC_FAILURE;
            }
        }
        else if(WDR_MODE_2To1_LINE == pstSnsState->enWDRMode)
        {
            if(SENSOR_RES_IS_4MP(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
            {
                u8SensorImageMode = SENSOR_4MP_30FPS_2T1_WDR_MODE;
            }
            else
            {
                printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                          pstSensorImageMode->u16Width,
                          pstSensorImageMode->u16Height,
                          pstSensorImageMode->f32Fps,
                          pstSnsState->enWDRMode);
                return SC_FAILURE;
            }
        }
        else
        {
            printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                      pstSensorImageMode->u16Width,
                      pstSensorImageMode->u16Height,
                      pstSensorImageMode->f32Fps,
                      pstSnsState->enWDRMode);
            return SC_FAILURE;
        }
    }
    else
    {
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

static SC_VOID sc4210_global_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pstSnsState = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    pstSnsState->bInit     = SC_FALSE;
    pstSnsState->bSyncInit = SC_FALSE;
    pstSnsState->u8ImgMode = SENSOR_4MP_30FPS_2T1_WDR_MODE;
    pstSnsState->enWDRMode = WDR_MODE_2To1_LINE;
    pstSnsState->u32FLStd  = SENSOR_VMAX_4MP60TO30_WDR;
    pstSnsState->au32FL[0] = SENSOR_VMAX_4MP60TO30_WDR;
    pstSnsState->au32FL[1] = SENSOR_VMAX_4MP60TO30_WDR;
    if(WDR_MODE_NONE == pstSnsState->enWDRMode)
    {
        pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = 0x00;   //intt
        pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = 0x8c;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32Data = 0x00;
        pstSnsState->astRegsInfo[0].astI2cData[3].u32Data = 0x00;   //gain
        pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = 0x80;
        pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = 0x03;
        pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = 0x40;
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = 0x05;   //vts
        pstSnsState->astRegsInfo[0].astI2cData[8].u32Data = 0xdc;
    }
    else
    {
        pstSnsState->astRegsInfo[0].astI2cData[0].u32Data  = 0x10;  //intt
        pstSnsState->astRegsInfo[0].astI2cData[1].u32Data  = 0x07;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32Data  = 0xa0;
        pstSnsState->astRegsInfo[0].astI2cData[3].u32Data  = 0x00;  //gain
        pstSnsState->astRegsInfo[0].astI2cData[4].u32Data  = 0x80;
        pstSnsState->astRegsInfo[0].astI2cData[5].u32Data  = 0x03;
        pstSnsState->astRegsInfo[0].astI2cData[6].u32Data  = 0x40;
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data  = 0x08;  //vts
        pstSnsState->astRegsInfo[0].astI2cData[8].u32Data  = 0xca;
        pstSnsState->astRegsInfo[0].astI2cData[9].u32Data  = 0x10;  //vts
        pstSnsState->astRegsInfo[0].astI2cData[10].u32Data = 0x80;
        pstSnsState->astRegsInfo[0].astI2cData[11].u32Data = 0x00;
        pstSnsState->astRegsInfo[0].astI2cData[12].u32Data = 0x80;
        pstSnsState->astRegsInfo[0].astI2cData[13].u32Data = 0x03;  //gain
        pstSnsState->astRegsInfo[0].astI2cData[14].u32Data = 0x40;
        pstSnsState->astRegsInfo[0].astI2cData[15].u32Data = 0x00;
        pstSnsState->astRegsInfo[0].astI2cData[16].u32Data = 0x88;
    }

    memset(&pstSnsState->astRegsInfo[0], 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&pstSnsState->astRegsInfo[1], 0, sizeof(ISP_SNS_REGS_INFO_S));
}

static SC_S32 cmos_set_mirror_flip(VI_PIPE ViPipe, int mode)
{
    SC_S32 ret = 0;

    printf("cmos_set_mirror_flip, mode=%d\n", mode);
    switch(mode)
    {
        case ISP_SNS_NORMAL:
        {
            ret = sc4210_flip_off_mirror_off(ViPipe);
            break;
        }

        case ISP_SNS_MIRROR:
        {
            ret = sc4210_flip_off_mirror_on(ViPipe);
            break;
        }

        case ISP_SNS_FLIP:
        {
            ret = sc4210_flip_on_mirror_off(ViPipe);
            break;
        }

        case ISP_SNS_MIRROR_FLIP:
        {
            ret = sc4210_flip_on_mirror_on(ViPipe);
            break;
        }

        default:
        {
            break;
        }
    }

    return ret;
}

static SC_S32 cmos_sc4210_ctl(VI_PIPE ViPipe, ISP_CMOS_SENSOR_CTL *pSensorCtl)
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

static SC_S32 cmos_init_sc4210_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    CMOS_CHECK_POINTER(pstSensorExpFunc);

    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));
    pstSensorExpFunc->pfn_cmos_sensor_init        = sc4210_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit        = sc4210_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = sc4210_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode     = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode       = cmos_set_wdr_mode;

    pstSensorExpFunc->pfn_cmos_get_isp_default  = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    pstSensorExpFunc->pfn_cmos_sns_power_on  = cmos_power_on;
    pstSensorExpFunc->pfn_cmos_sns_power_off = cmos_power_off;
    pstSensorExpFunc->pfn_cmos_sns_ctl       = cmos_sc4210_ctl;

    return SC_SUCCESS;
}


/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/
static SC_S32 sc4210_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunSc4210BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return SC_SUCCESS;
}

static SC_S32 sc4210_ctx_init(VI_PIPE ViPipe)
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

static SC_VOID sc4210_ctx_exit(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = SC_NULL;

    SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
    SENSOR_FREE(pastSnsStateCtx);
    SENSOR_RESET_CTX(ViPipe);
}

static SC_S32 sc4210_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    SC_S32 s32Ret = SC_FAILURE;

    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = sc4210_ctx_init(ViPipe);
    if(SC_SUCCESS != s32Ret)
    {
        return SC_FAILURE;
    }

    stSnsAttrInfo.eSensorId = SC4210_ID;

    s32Ret  = cmos_init_sc4210_exp_function(&stIspRegister.stSnsExp);
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

static SC_S32 sc4210_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    SC_S32 s32Ret = SC_FAILURE;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = SC_MPI_ISP_SensorUnRegCallBack(ViPipe, SC4210_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = SC_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, SC4210_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = SC_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, SC4210_ID);
    if(SC_SUCCESS != s32Ret)
    {
        printf("sensor unregister callback function to awb lib failed!\n");
        return s32Ret;
    }

    sc4210_ctx_exit(ViPipe);

    return SC_SUCCESS;
}

static SC_S32 sc4210_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
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

ISP_SNS_OBJ_S stSnsSc4210Obj =
{
    .pfnRegisterCallback    = sc4210_register_callback,
    .pfnUnRegisterCallback  = sc4210_unregister_callback,
    .pfnStandby             = sc4210_standby,
    .pfnRestart             = sc4210_restart,
    .pfnMirrorFlip          = SC_NULL,
    .pfnWriteReg            = sc4210_write_register,
    .pfnReadReg             = sc4210_read_register,
    .pfnSetBusInfo          = sc4210_set_bus_info,
    .pfnSetInit             = sc4210_set_init
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __SC4210_CMOS_C_ */
