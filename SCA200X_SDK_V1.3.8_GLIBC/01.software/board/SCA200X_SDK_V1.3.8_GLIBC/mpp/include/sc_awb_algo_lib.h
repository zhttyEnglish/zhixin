/**
 * @file     sc_awb_algo_lib.h
 * @brief    AWB算法库的宏、枚举和结构体类型定义
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-12-07 创建文件
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

#ifndef __SC_AWB_ALGO_LIB_H__
#define __SC_AWB_ALGO_LIB_H__

#include "sc_isp_type_def.h"

typedef enum
{
    AWB_CTL_HW_START,
    AWB_CTL_HW_STOP,
    AWB_CTL_HW_PICK_RES,
    AWB_CTL_HW_SET_IN_OUT_FORMAT,
    AWB_CTL_UPDATE_TUNING_PRTA,
    AWB_CTL_EVENT_AEC_UPDATE,
    AWB_CTL_SET_DEBUG_INFO,
    AWB_CTL_CAM_PRA_SET_AWB_MODE,
    AWB_CTL_CAM_PRA_SET_MANUAL_AWB,
    AWB_CTL_CAM_PRA_GET_BASIC_3A_INFO,
    AWB_CTL_CAM_PRA_GET_3A_INFO,
} SC_AWB_CTL_E;

typedef struct
{
    SC_S32 ImgWidth;
    SC_S32 ImgHeight;
    SC_S32 parZoom;
    SC_S32 ver_s;
    SC_S32 hor_s;
    SC_S32 BitsSum;
    SC_S32 pattern;
    SC_S32 X_offset;
    SC_S32 Y_offset;
    SC_S32 roi_width;
    SC_S32 roi_height;
    SC_S32 Block_width;
    SC_S32 Block_height;
    SC_S32 X_skip;
    SC_S32 Y_skip;
} SC_AWB_STATS_CFG_T;

typedef struct
{
    SC_S32 frame_id;
    SC_S32 mesh_grid_w;
    SC_S32 mesh_grid_h;

    SC_U32 p_mesh_grid_stats_r_sum[SC_ISP_AWB_NEARGRAY_SIZE];
    SC_U32 p_mesh_grid_stats_g_sum[SC_ISP_AWB_NEARGRAY_SIZE];
    SC_U32 p_mesh_grid_stats_b_sum[SC_ISP_AWB_NEARGRAY_SIZE];
    SC_U32 p_mesh_grid_stats_num[SC_ISP_AWB_NEARGRAY_SIZE];
} SC_AWB_ALGO_LIB_INPUT_T;

typedef struct
{
    SC_FLOAT r_gain;
    SC_FLOAT g_gain;
    SC_FLOAT b_gain;
    SC_FLOAT cct;
} SC_AWB_ALGO_LIB_OUTPUT_T;

typedef struct
{
    SC_S32 is_init;

    SC_ALGO_ISP_TUNING_T *p_algo_tuning;
    SC_ISP_TUNING_PRA_T  *p_isp_tuning_pra;
} SC_AWB_CTL_UPDATE_TUNING_PRA_T;

typedef struct
{
    SC_ISP_CAM_API_PRE_PRA_T pre_pra;

    SC_U32 preview_fps;
    SC_U32 preview_linesPerFrame;
    SC_U16 pixel_clock_per_line;
    SC_U32 pixel_clock;
    SC_U32 sensor_hdr;
    SC_U32 hdr_frame_count;
    SC_U32 new_preview_fps;
    SC_U32 new_preview_linesPerFrame;
    SC_S32 vif_isp_mode;
} SC_AWB_ALGO_RES_INF0_T;

typedef struct
{
    SC_FLOAT lux_index;

    SC_S32 ae_settle;

    SC_FLOAT current_luma;
} SC_AWB_ALGO_LIB_UPDATE_T;

typedef enum
{
    AWB_CB_EVENT_INIT,
    AWB_CB_EVENT_EXIT,
    AWB_CB_EVENT_RUN,
    AWB_CB_EVENT_CTL,
    AWB_CB_EVENT_MAX,
} SC_ISP_CB_EVENT_E;

typedef struct
{
    SC_S32 camera_id;
} SC_AWB_CB_EVENT_INIT_PRA_T;

typedef struct
{
    SC_S32 camera_id;
} SC_AWB_CB_EVENT_EXIT_PRA_T;

typedef struct
{
    SC_S32 camera_id;

    SC_AWB_ALGO_LIB_INPUT_T  *p_aec_algo_lib_input;
    SC_AWB_ALGO_LIB_OUTPUT_T *p_aec_algo_lib_output;
} SC_AWB_CB_EVENT_RUN_PRA_T;

typedef struct
{
    SC_S32 camera_id;
    SC_S32 clt_code;

    SC_VOID *pra;

    SC_S32 ctl_size;
} SC_AWB_CB_EVENT_CTL_PRA_T;

#endif // __SC_AWB_ALGO_LIB_H__
