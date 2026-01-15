/**
 * @file     sc_comm_3a.h
 * @brief    3A模块的宏、枚举和结构体类型定义
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

#ifndef __SC_COMM_3A_H__
#define __SC_COMM_3A_H__

#include "sc_common.h"
#include "sc_comm_sns.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define IS_LINEAR_MODE(mode)      (WDR_MODE_NONE == (mode))
#define IS_WDR_MODE(mode)         ((!IS_LINEAR_MODE(mode)) && ((mode) < WDR_MODE_BUTT))
#define IS_HDR_MODE(mode)         (((mode) > DYNAMIC_RANGE_SDR10) && ((mode) < DYNAMIC_RANGE_BUTT))
#define IS_BUILT_IN_WDR_MODE(mode) (WDR_MODE_BUILT_IN == (mode))
#define IS_FS_WDR_MODE(mode)      ((!IS_LINEAR_MODE(mode))&& (!IS_BUILT_IN_WDR_MODE(mode)) && ((mode) < WDR_MODE_BUTT))
#define IS_2to1_WDR_MODE(mode)    ((WDR_MODE_2To1_FRAME == (mode)) || (WDR_MODE_2To1_FRAME_FULL_RATE == (mode)) || (WDR_MODE_2To1_LINE == (mode))  || (WDR_MODE_QUDRA == (mode)))
#define IS_3to1_WDR_MODE(mode)    ((WDR_MODE_3To1_FRAME == (mode)) || (WDR_MODE_3To1_FRAME_FULL_RATE == (mode)) || (WDR_MODE_3To1_LINE == (mode)))
#define IS_4to1_WDR_MODE(mode)    ((WDR_MODE_4To1_FRAME == (mode)) || (WDR_MODE_4To1_FRAME_FULL_RATE == (mode)) || (WDR_MODE_4To1_LINE == (mode)))
#define IS_FULL_WDR_MODE(mode)    ((WDR_MODE_2To1_FRAME_FULL_RATE == (mode)) || (WDR_MODE_3To1_FRAME_FULL_RATE == (mode)) || (WDR_MODE_4To1_FRAME_FULL_RATE == (mode)))
#define IS_HALF_WDR_MODE(mode)    ((WDR_MODE_2To1_FRAME == (mode)) || (WDR_MODE_3To1_FRAME == (mode)) || (WDR_MODE_4To1_FRAME == (mode)))
#define IS_LINE_WDR_MODE(mode)    ((WDR_MODE_2To1_LINE == (mode)) || (WDR_MODE_3To1_LINE == (mode)) || (WDR_MODE_4To1_LINE == (mode)) || (WDR_MODE_QUDRA == (mode)))

#define IS_STITCH_MAIN_PIPE(ViPipe,MainPipe) ((ViPipe) == (MainPipe))
#define IS_OFFLINE_MODE(mode)    (ISP_MODE_RUNNING_OFFLINE    == (mode))
#define IS_ONLINE_MODE(mode)     (ISP_MODE_RUNNING_ONLINE     == (mode))
#define IS_SIDEBYSIDE_MODE(mode) (ISP_MODE_RUNNING_SIDEBYSIDE == (mode))
#define IS_STRIPING_MODE(mode)   (ISP_MODE_RUNNING_STRIPING   == (mode))

#define MAX_REGISTER_ALG_LIB_NUM 2

/* --------------------------   AE   -------------------------- */
typedef struct scISP_AE_PARAM_S
{
    SENSOR_ID SensorId;

    SC_U8 u8WDRMode;
    SC_U8 u8HDRMode;

    SC_U16 u16BlackLevel;

    SC_FLOAT f32Fps;

    ISP_BAYER_FORMAT_E enBayer;

    SC_S32 s32Rsv;
} ISP_AE_PARAM_S;

typedef struct scISP_AE_INFO_S
{
    SC_AEC_ALGO_LIB_INPUT_T stAecStats;
} ISP_AE_INFO_S;

/* the final calculate of ae alg */
typedef struct scISP_AE_RESULT_S
{
    SC_AEC_ALGO_LIB_OUTPUT_T stAecResult;
} ISP_AE_RESULT_S;

typedef struct scISP_AE_EXP_FUNC_S
{
    SC_S32 (*pfn_ae_init)(SC_S32 s32Handle, const ISP_AE_PARAM_S *pstAeParam);
    SC_S32 (*pfn_ae_run)(SC_S32 s32Handle, const ISP_AE_INFO_S *pstAeInfo, ISP_AE_RESULT_S *pstAeResult, SC_S32 s32Rsv);
    SC_S32 (*pfn_ae_ctrl)(SC_S32 s32Handle, SC_U32 u32Cmd, SC_VOID *pValue);
    SC_S32 (*pfn_ae_exit)(SC_S32 s32Handle);
} ISP_AE_EXP_FUNC_S;

typedef struct scISP_AE_REGISTER_S
{
    ISP_AE_EXP_FUNC_S stAeExpFunc;
} ISP_AE_REGISTER_S;

/* --------------------------   AWB   -------------------------- */
typedef struct scISP_AWB_PARAM_S
{
    SENSOR_ID SensorId;

    SC_U8 u8WDRMode;
    SC_U8  u8HDRMode;
    SC_U16 u16BlackLevel;
    SC_FLOAT f32Fps;
    ISP_BAYER_FORMAT_E enBayer;
    SC_S32 s32Rsv;
} ISP_AWB_PARAM_S;

typedef struct scISP_AWB_INFO_S
{
    SC_AWB_ALGO_LIB_INPUT_T stAwbStats;
} ISP_AWB_INFO_S;

/* the final calculate of awb alg */
typedef struct scISP_AWB_RESULT_S
{
    SC_AWB_ALGO_LIB_OUTPUT_T stAwbResult;
} ISP_AWB_RESULT_S;

typedef struct scISP_AWB_EXP_FUNC_S
{
    SC_S32 (*pfn_awb_init)(SC_S32 s32Handle, const ISP_AWB_PARAM_S *pstAwbParam);
    SC_S32 (*pfn_awb_run) (SC_S32 s32Handle,
        const ISP_AWB_INFO_S *pstAwbInfo, ISP_AWB_RESULT_S *pstAwbResult, SC_S32 s32Rsv);
    SC_S32 (*pfn_awb_ctrl)(SC_S32 s32Handle, SC_U32 u32Cmd, SC_VOID *pValue);
    SC_S32 (*pfn_awb_exit)(SC_S32 s32Handle);
} ISP_AWB_EXP_FUNC_S;

typedef struct scISP_AWB_REGISTER_S
{
    ISP_AWB_EXP_FUNC_S stAwbExpFunc;
} ISP_AWB_REGISTER_S;

/* --------------------------   AF   -------------------------- */
typedef struct scISP_AF_PARAM_S
{
    SENSOR_ID SensorId;

    SC_U8  u8WDRMode;
    SC_U8  u8HDRMode;

    SC_U16 u16BlackLevel;

    SC_FLOAT f32Fps;

    ISP_BAYER_FORMAT_E enBayer;

    SC_S32 s32Rsv;
} ISP_AF_PARAM_S;

typedef struct scISP_AF_INFO_S
{
    SC_AF_ALGO_LIB_INPUT_T stAfStats;
} ISP_AF_INFO_S;

typedef struct scISP_AF_RESULT_S
{
    SC_AF_ALGO_LIB_OUTPUT_T stAfResult;
} ISP_AF_RESULT_S;

typedef struct scISP_AF_EXP_FUNC_S
{
    SC_S32 (*pfn_af_init)(SC_S32 s32Handle, const ISP_AF_PARAM_S *pstAfParam);
    SC_S32 (*pfn_af_run)(SC_S32 s32Handle, const ISP_AF_INFO_S *pstAfInfo, ISP_AF_RESULT_S *pstAfResult, SC_S32 s32Rsv);
    SC_S32 (*pfn_af_ctrl)(SC_S32 s32Handle, SC_U32 u32Cmd, SC_VOID *pValue);
    SC_S32 (*pfn_af_exit)(SC_S32 s32Handle);
} ISP_AF_EXP_FUNC_S;

typedef struct scISP_AF_REGISTER_S
{
    ISP_AF_EXP_FUNC_S stAfExpFunc;
} ISP_AF_REGISTER_S;

/******************* algo **********************************/
#define ALG_LIB_NAME_SIZE_MAX   (20)

typedef struct scALG_LIB_S
{
    SC_S32  s32Id;
    SC_CHAR acLibName[ALG_LIB_NAME_SIZE_MAX];
} ALG_LIB_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /*__SC_COMM_3A_H__ */
