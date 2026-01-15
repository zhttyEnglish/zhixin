/**
 * @file     sc_af_algo_lib.h
 * @brief    AF算法库的宏、枚举和结构体类型定义
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

#ifndef __SC_AF_ALGO_LIB_H__
#define __SC_AF_ALGO_LIB_H__

#include "sc_isp_type_def.h"

typedef enum
{
    AF_CTL_SET_AF_MODE,
    AF_CTL_SET_MANUAL_AF,
    AF_CTL_EVENT_AEC_UPDATE,
    AF_CTL_GET_3A_INFO,
    AF_CTL_TRIGGER_FOCUS,
    AF_CTL_SET_FOCUS_ROI,
    AF_CTL_SET_TUNINIG_REQUEST,
    AF_CTL_HW_SET_IN_OUT_FORMAT,
    AF_CTL_HW_SET_START,
    AF_CTL_HW_SET_STOP,
    AF_CTL_GET_LIMIT_INFO,
    AF_CTL_MAX
} SC_AF_CTL_E;

typedef enum
{
    MOVE_NEAR,
    MOVE_FAR,
    MOVE_FIX,
} SC_ISP_MOVE_E;

typedef struct
{
    SC_S32 mode;
} SC_AF_CTL_SET_AF_MODE_INFO_T;

typedef struct
{
    SC_S32 pos;
} SC_AF_CTL_SET_MANUAL_AF_INFO_T;

typedef struct
{
    SC_S32 aec_settled;

    SC_FLOAT lux_idx;
    SC_FLOAT cur_luma;
    SC_FLOAT cur_real_gain;
    SC_FLOAT exp_time_us;
} SC_AF_CTL_AEC_UPDATE_INFO_T;

typedef struct
{
    SC_U32 len_position;
    SC_U32 focus_value;
    SC_U32 history_pos;
    SC_U32 history_focus_value;
    SC_U32 total_steps;
} SC_AF_CTL_GET_AF_STATUS_INFO;

typedef struct
{
    SC_FLOAT x;
    SC_FLOAT y;
    SC_FLOAT width;
    SC_FLOAT height;
} SC_AF_CTL_SET_FOUCS_ROI_INFO_T;

typedef struct
{
    SC_S32 x_offset;
    SC_S32 y_offset;
    SC_S32 image_width;
    SC_S32 image_height;
    SC_S32 block_width;
    SC_S32 block_height;
    SC_S32 stats_height;
    SC_S32 stats_width;
} SC_AF_CTL_HW_IN_OUT_INFO_T;

typedef struct
{
    SC_S32 cam_index;
} SC_AF_CTL_HW_START_INFO_T;

typedef struct
{
    SC_AF_ALGO_LIB_AF_STATUS_DATA_T af_stats_data;
} SC_AF_ALGO_LIB_INPUT_T;

typedef struct
{
    SC_S32 camera_id;
    SC_U32 lens_position;

    SC_ISP_MOVE_E dir;

    SC_S32 move_step;
    SC_S32 b_move_lens;

    SC_AF_CTL_SET_FOUCS_ROI_INFO_T roi;
} SC_AF_ALGO_LIB_OUTPUT_T;

typedef enum
{
    AF_CB_EVENT_INIT,
    AF_CB_EVENT_EXIT,
    AF_CB_EVENT_RUN,
    AF_CB_EVENT_CTL,
    AF_CB_EVENT_MAX,
} SC_AF_CB_EVENT_T;

typedef struct
{
    SC_S32 camera_id;
} SC_AF_CB_EVENT_INIT_PRA_T;

typedef struct
{
    SC_S32 camera_id;
} SC_AF_CB_EVENT_EXIT_PRA_T;

typedef struct
{
    SC_S32 camera_id;

    SC_AF_ALGO_LIB_INPUT_T  *p_aec_algo_lib_input;
    SC_AF_ALGO_LIB_OUTPUT_T *p_aec_algo_lib_output;
} SC_AF_CB_EVENT_RUN_PRA_T;

typedef struct
{
    SC_S32 camera_id;
    SC_S32 clt_code;

    SC_PVOID pra;

    SC_S32 ctl_size;
} SC_AF_CB_EVENT_CTL_PRA_T;

#endif // __SC_AF_ALGO_LIB_H__
