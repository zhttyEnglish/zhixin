/**
 * @file     sc_ae_algo_lib.h
 * @brief    AE算法库的宏、枚举和结构体类型定义
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

#ifndef __SC_AE_ALGO_LIB_H__
#define __SC_AE_ALGO_LIB_H__

#include "sc_isp_type_def.h"

#define MAX_FACE_STATS_COUNT 16

typedef enum
{
    AEC_CTL_HW_START,
    AEC_CTL_HW_STOP,
    AEC_CTL_HW_PICK_RES,
    AEC_CTL_HW_SET_IN_OUT_FORMAT,
    AEC_CTL_UPDATE_TUNING_PRTA,
    AEC_CTL_GET_IMG_BRIGHTNESS,
    AEC_CTL_SET_IMG_BRIGHTNESS,
    AEC_CTL_SET_FACE_AEC,
    AEC_CTL_SET_AE_MODE,
    AEC_CTL_GET_AE_MODE,
    AEC_CTL_SET_MANUAL_AE,
    AEC_CTL_SET_SENCE_MODE,
    AEC_CTL_GET_SCENE_MODE,
    AEC_CTL_CAM_PRA_SET_SENSOR_HDR,
    AEC_CTL_CAM_PRA_GET_SENSOR_HDR,
    AEC_CTL_CAM_PRA_SET_HDR_AUTO_DETECT,
    AEC_CTL_CAM_PRA_GET_HDR_AUTO_DETECT,
    AEC_CTL_CAM_PRA_SET_HDR_PRA_MODE,
    AEC_CTL_CAM_PRA_GET_HDR_PRA_MODE,
    AEC_CTL_CAM_PRA_SET_FPS_RANGE,
    AEC_CTL_CAM_PRA_SET_EXP_LIMIT,
    AEC_CTL_CAM_PRA_GET_EXP_LIMIT,
    AEC_CTL_CAM_PRA_SET_ANTIBANDING,
    AEC_CTL_CAM_PRA_GET_ANTIBANDING,
    AEC_CTL_CAM_PRA_SET_MOTION_DETECT,
    AEC_CTL_CAM_PRA_GET_3A_INFO,
    AEC_CTL_CAM_PRA_GET_BASIC_3A_INFO,
    AEC_CTL_EVENT_AWB_UPDATE,
    AEC_CTL_SET_DEBUG_INFO,
    AEC_CTL_GET_LIMIT_INFO,
} SC_AEC_CTL_E;

typedef struct
{
    SC_S32 is_init;

    SC_ALGO_ISP_TUNING_T *p_algo_tuning;
    SC_ISP_TUNING_PRA_T  *p_isp_tuning_pra;
} SC_ISP_AEC_CLT_UPDATE_TUNING_PRA_T;

typedef struct
{
    SC_S32 x;
    SC_S32 y;
    SC_S32 width;
    SC_S32 height;
} SC_AEC_ROI_T;

typedef struct
{
    SC_S32 pattern;
    SC_S32 x_offset;
    SC_S32 y_offset;
    SC_S32 ver_s;
    SC_S32 hor_s;
    SC_S32 x_skip;
    SC_S32 y_skip;
    SC_S32 image_width;
    SC_S32 image_height;
    SC_S32 block_width;
    SC_S32 block_height;
    SC_S32 th_s;
} SC_STATS_CFG_INFO_T;

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
} SC_AEC_ALGO_RES_INFO_T;

typedef struct
{
    SC_U32 camera_id;
    SC_U32 frame_id;

    SC_MESH_GRID_STATS_DATA_T stats_data;

    SC_S32 raw_hist_bin_num;
    SC_U32 p_raw_hist_r[128];
    SC_U32 p_raw_hist_g[128];
    SC_U32 p_raw_hist_b[128];
    SC_S32 rgb_his_bin_num;
    SC_U32 p_rgb_hist[128];
    SC_S32 face_aec_stats_w;
    SC_S32 face_aec_stats_h;
    SC_U32 p_face_aec_mesh_grid_stats[16 * 16];

    SC_MESH_GRID_STATS_DATA_T hdr_rro1_stats_data;
    SC_MESH_GRID_STATS_DATA_T hdr_rro2_stats_data;
    SC_MESH_GRID_STATS_DATA_T hw_face_aec_stats_data;

    SC_PVOID priv;
} SC_AEC_ALGO_LIB_INPUT_T;

typedef enum
{
    LONG_SHORT_EXP_RATION,
    MIDDLE_SHORT_EXP_RATION,
    MIDDLEPLUS_SHORT_EXP_RATION,
    SHORT_SHORT_EXP_RATION,
} SC_ISP_EXP_RATION_INDEX_E;

typedef struct
{
    SC_S32 camera_id;
    SC_S32 updated;

    SC_U32 line_count;
    SC_U32 exp_time_us;

    SC_FLOAT real_gain;
    SC_FLOAT sensor_gain;
    SC_FLOAT isp_gainl;
    SC_FLOAT isp_gain2;
    SC_FLOAT exp_gain;

    SC_U32 exp_index;
    SC_U32 lux_index;
    SC_U32 lux_value;
    SC_U32 ae_settle;
    SC_U32 current_luma;
    SC_U32 short_exp_index;

    SC_FLOAT ration_gain[4];
    SC_FLOAT ration_time[4];
    SC_FLOAT total_raion[4];
    SC_FLOAT drc_gain;
    SC_FLOAT drc_gain_1;
    SC_FLOAT drc_gain_2;
    SC_FLOAT fps;
} SC_AEC_ALGO_LIB_OUTPUT_T;

typedef struct
{
    SC_FLOAT r_gain;
    SC_FLOAT b_gain;
} SC_AEC_ALGO_AWB_UPDATE_T;

typedef struct
{
    SC_U32 fps_min;
    SC_U32 fps_max;
    SC_U32 fps_min_line_count;
    SC_U32 fps_max_line_count;
} SC_AEC_LIB_SET_FPS_INFOR_T;

typedef enum
{
    AEC_CB_EVENT_INIT,
    AEC_CB_EVENT_EXIT,
    AEC_CB_EVENT_RUN,
    AEC_CB_EVENT_CTL,
    AEC_CB_EVENT_MAX,
} SC_ISP_AEC_CB_EVENT_E;

typedef struct
{
    SC_S32 camera_id;
} SC_AEC_CB_EVENT_INIT_PRA_T;

typedef struct
{
    SC_S32 camera_id;
} SC_AEC_CB_EVENT_EXIT_PRA_T;

typedef struct
{
    SC_S32 camera_id;

    SC_AEC_ALGO_LIB_INPUT_T  *p_aec_algo_lib_input;
    SC_AEC_ALGO_LIB_OUTPUT_T *p_aec_algo_lib_output;
} SC_AEC_CB_EVENT_RUN_PRA_T;

typedef struct
{
    SC_S32 camera_id;
    SC_S32 clt_code;

    SC_PVOID pra;

    SC_S32 ctl_size;
} SC_AEC_CB_EVENT_CTL_PRA_T;

#endif // __SC_AE_ALGO_LIB_H__
