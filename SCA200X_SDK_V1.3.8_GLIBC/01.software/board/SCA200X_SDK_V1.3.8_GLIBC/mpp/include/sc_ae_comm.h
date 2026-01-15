/**
 * @file     sc_ae_comm.h
 * @brief    AE模块的宏、枚举和结构体类型定义
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-09-01 创建文件
 */
/***********************************************************************************************************************
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
***********************************************************************************************************************/

#ifndef __SC_AE_COMM_H__
#define __SC_AE_COMM_H__

#include "sc_type.h"
#include "sc_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define SC_AE_LIB_NAME "sc_ae_lib"

typedef struct scAE_SENSOR_DEFAULT_S
{
    SC_U32  u32LinesPer500ms;
    SC_U32  u32FlickerFreq;

    SC_FLOAT f32Fps;
    SC_FLOAT f32MaxFps;

    SC_U32  u32Hmax;
    SC_U32  u32FullLinesStd;
    SC_U32  u32FullLinesMid;
    SC_U32  u32FullLinesShort;
    SC_U32  u32FullLinesMax;
    SC_U32  u32FullLines;
    SC_U32  u32MaxIntTime;
    SC_U32  u32MinIntTime;
    SC_U32  u32MaxAgain;
    SC_U32  u32MinAgain;
    SC_U32  u32MaxDgain;
    SC_U32  u32MinDgain;
    SC_U32  u32MaxIntTimeStep;
    SC_U32  u32LFMaxShortTime;
    SC_U32  u32LFMinExposure;

    SC_BOOL bAERouteExValid;
} AE_SENSOR_DEFAULT_S;

typedef struct scAE_FSWDR_ATTR_S
{
    ISP_FSWDR_MODE_E enFSWDRMode;
} AE_FSWDR_ATTR_S;

typedef struct scAE_SENSOR_EXP_FUNC_S
{
    SC_S32(*pfn_cmos_get_ae_default)(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft);

    SC_VOID(*pfn_cmos_fps_set)(VI_PIPE ViPipe, SC_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft);
    SC_VOID(*pfn_cmos_slow_framerate_set)(VI_PIPE ViPipe, SC_U32 u32FullLines, AE_SENSOR_DEFAULT_S *pstAeSnsDft);

    SC_VOID(*pfn_cmos_inttime_update)(VI_PIPE ViPipe, SC_U32 u32IntTime);
    SC_VOID(*pfn_cmos_gains_update)(VI_PIPE ViPipe, SC_U32 u32Again, SC_U32 u32Dgain);

    SC_VOID (*pfn_cmos_again_calc_table)(VI_PIPE ViPipe, SC_U32 *pu32AgainLin, SC_U32 *pu32AgainDb);
    SC_VOID (*pfn_cmos_dgain_calc_table)(VI_PIPE ViPipe, SC_U32 *pu32DgainLin, SC_U32 *pu32DgainDb);

    SC_VOID (*pfn_cmos_get_inttime_max)(VI_PIPE ViPipe, SC_U16 u16ManRatioEnable, SC_U32 *au32Ratio, SC_U32 *au32IntTimeMax,
        SC_U32 *au32IntTimeMin, SC_U32 *pu32LFMaxIntTime);

    SC_VOID(*pfn_cmos_ae_fswdr_attr_set)(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr);
} AE_SENSOR_EXP_FUNC_S;

typedef struct scAE_SENSOR_REGISTER_S
{
    AE_SENSOR_EXP_FUNC_S stSnsExp;
} AE_SENSOR_REGISTER_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __SC_AE_COMM_H__ */
