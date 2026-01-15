/**
 * @file     sc_isp_type_def.h
 * @brief    ISP基础数据类型定义
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

#ifndef __SC_ISP_TYPE_DEF__H__
#define __SC_ISP_TYPE_DEF__H__

#ifdef __cplusplus
extern "C" {
#endif

#define SC_ISP_MATRIX_ROW_NUMBER 36
#define SC_ISP_MATRIX_COL_NUMBER 64
#define SC_ISP_AWB_NEARGRAY_SIZE (SC_ISP_MATRIX_COL_NUMBER * SC_ISP_MATRIX_ROW_NUMBER)
#define SC_ISP_AF_STAT_WIDTH     16
#define SC_ISP_AF_STAT_HEIGHT    9

typedef struct
{
    SC_FLOAT x;
    SC_FLOAT y;
    SC_FLOAT width;
    SC_FLOAT height;
} SC_ISP_ROI_T;

typedef struct
{
    SC_S32   aec_mode;

    SC_FLOAT gain;
    SC_FLOAT exp_time_us;

    SC_U32   luma_target;

    SC_FLOAT gain_short;
    SC_FLOAT exp_time_us_short;
} SC_AEC_SET_INFO_T;

typedef struct
{
    SC_S32   aec_mode;
    SC_S32   locked;

    SC_FLOAT exp_time_us_short;
    SC_FLOAT gain;
    SC_FLOAT sensor_gain;
    SC_FLOAT isp_gain1;
    SC_FLOAT isp_gain2;
    SC_FLOAT exp_time_us;

    SC_U32 line_count;
    SC_U32 exp_index;
    SC_U32 lux_index;
    SC_U32 luma_target;
    SC_U32 luma_target_short;
    SC_U32 current_luma;

    SC_FLOAT weightSum;
    SC_FLOAT luma_avg;
    SC_FLOAT lumaSum;

    SC_S32   cur_af_luma;

    SC_FLOAT gain_before_banding;
    SC_S32   line_cnt_before_banding;
    SC_FLOAT gain_after_banding;
    SC_S32   line_cnt_after_banding;

    SC_S32 hybrid_luma;
    SC_S32 default_luma_target_compensated;
    SC_S32 luma_comp_target;
    SC_S32 ae_settled;
    SC_S32 ae_settled_short;
    SC_U32 flash_status;

    SC_FLOAT dark_interpolated_thld0;
    SC_FLOAT dark_interpolated_thld1;
    SC_FLOAT dark_interpolated_thld2;
    SC_FLOAT over_exp_adjust_ratio;

    SC_U32 high_luma_region_count;
    SC_S32 high_luma_region_th;
    SC_S32 bright_discard_th;
    SC_S32 dark_discard_th;
    SC_S32 metering_type;
    SC_S32 motion;
    SC_S32 sad_for_flat_scene;
    SC_S32 run_times;

    SC_FLOAT long_exp_ration;
    SC_FLOAT max_drc_gain;
    SC_FLOAT min_drc_gain;
    SC_FLOAT max_drc_gain1;
    SC_FLOAT ration_time;
    SC_FLOAT ration_gain;
    SC_FLOAT ration_total;
    SC_FLOAT drc_gain1;
    SC_FLOAT drc_gain2;
    SC_FLOAT drc_gain;
    SC_FLOAT cur_real_gain_short;
    SC_FLOAT cur_real_gain;

    SC_S32 cur_line_cnt_short;
    SC_S32 cur_line_cnt;
    SC_S32 short_luma;
    SC_S32 short_exp_index;

    SC_FLOAT y_average[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
    SC_FLOAT y_average_short[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];

    SC_S32 extreme_red_count;
    SC_S32 extreme_green_count;
    SC_S32 extreme_blue_count;
    SC_S32 dark_region_count;
    SC_S32 bright_region_count;
    SC_S32 settled_time_frames;
    SC_S32 weightSum_short;
    SC_S32 luma_avg_short;
    SC_S32 lumaSum_short;
    SC_S32 dark_region_count_short;
    SC_S32 bright_region_count_short;
    SC_S32 sat_bin;
    SC_S32 dark_bin;
} SC_AEC_GET_INFO_T;

typedef struct
{
    SC_S32 camera_id;
    SC_S32 stats_type;
    SC_S32 snr_flag;
    SC_S32 updated;
    SC_U32 line_count;
    SC_U32 exp_time_us;

    SC_FLOAT real_gain;
    SC_FLOAT sensor_gain;
    SC_FLOAT isp_gainl;
    SC_FLOAT isp_gain2;
    SC_FLOAT exp_gain;

    SC_U32 exp_index;
    SC_U32 short_exp_index;
    SC_U32 lux_index;
    SC_U32 lux_value;
    SC_U32 ae_settle;

    float current_luma;

    SC_FLOAT ration_gain[4];
    SC_FLOAT ration_time[4];
    SC_FLOAT total_raion[4];

    SC_FLOAT drc_gain;
    SC_FLOAT drc_gain_1;
    SC_FLOAT drc_gain_2;

    float fps;
} SC_AEC_OUT_T;

typedef struct
{
    SC_S32 awb_mode;

    SC_U32 cct;

    SC_FLOAT r_gain;
    SC_FLOAT g_gain;
    SC_FLOAT b_gain;
} SC_AWB_SET_INFO_T;

typedef struct
{
    SC_FLOAT all_sgw_rg;
    SC_FLOAT all_sgw_bg;
    SC_FLOAT cct;
    SC_FLOAT main_decision;
    SC_FLOAT sub_decsion;
    SC_FLOAT dist;
    SC_FLOAT dist_w;
    SC_FLOAT luma_cct_w;

    SC_S32   y_luma;
    SC_S32   point_type;
} SC_AWB_STATS_POINT_INFO_T;

typedef struct
{
    SC_S32 awb_mode;
    SC_U32 cct;

    SC_FLOAT r_gain;
    SC_FLOAT g_gain;
    SC_FLOAT b_gain;

    SC_S32 current_lux_index;
    SC_S32 is_extreme_blue;
    SC_S32 is_extreme_green;

    SC_FLOAT ExtremeB_pec;
    SC_FLOAT GreenZone_pec;
    SC_FLOAT last_R_gain;
    SC_FLOAT last_G_gain;
    SC_FLOAT last_B_gain;
    SC_FLOAT smooth_weight;
    SC_FLOAT r_gain_adj_coef;
    SC_FLOAT b_gain_adj_coef;
    SC_FLOAT GreenZone_num;
    SC_FLOAT GreenZone_r_ave;
    SC_FLOAT GreenZone_g_ave;
    SC_FLOAT GreenZone_b_ave;

    SC_S32   ExtremeB_num;
    SC_FLOAT ExtremeB_r_ave;
    SC_FLOAT ExtremeB_g_ave;
    SC_FLOAT ExtremeB_b_ave;

    SC_FLOAT green_rg;
    SC_FLOAT green_bg;

    SC_S32 percent_cnt;
    SC_S32 sgw_cnt;
    SC_S32 rg_bg_as0_num;
    SC_S32 grey_point_num;
    SC_S32 out_line_num;
    SC_S32 block_percent_low_num;

    SC_FLOAT sgw_rg_ratio;
    SC_FLOAT sgw_bg_ratio;

    SC_S32   a_cluster;
    SC_FLOAT a_rg_ratio;
    SC_FLOAT a_bg_ratio;
    SC_FLOAT unsat_y_a_ave;

    SC_S32   h_cluster;
    SC_FLOAT h_rg_ratio;
    SC_FLOAT h_bg_ratio;
    SC_FLOAT unsat_y_h_ave;

    SC_S32   f_cluster;
    SC_FLOAT f_rg_ratio;
    SC_FLOAT f_bg_ratio;
    SC_FLOAT unsat_y_f_ave;

    SC_S32   day_cluster;
    SC_FLOAT day_rg_ratio;
    SC_FLOAT day_bg_ratio;
    SC_FLOAT unsat_y_day_ave;

    SC_FLOAT w_d;
    SC_FLOAT w_f;
    SC_FLOAT w_a;
    SC_FLOAT w_h;

    SC_FLOAT wrg_sum;
    SC_FLOAT wbg_sum;

    SC_FLOAT ave_rg_ratio;
    SC_FLOAT ave_bg_ratio;

    SC_FLOAT unsat_y_mid;
    SC_FLOAT unsat_y_max;
    SC_FLOAT unsat_y_min_threshold;

    SC_S32 valid_sample_cnt;

    SC_S32   sat_cnt;
    SC_S32   sat_a_cluster;
    SC_FLOAT sat_a_rg;
    SC_FLOAT sat_a_bg;
    SC_S32   sat_h_cluster;
    SC_FLOAT sat_h_rg;
    SC_FLOAT sat_h_bg;
    SC_S32   sat_f_cluster;
    SC_FLOAT sat_f_rg;
    SC_FLOAT sat_f_bg;
    SC_S32   sat_day_cluster;
    SC_FLOAT sat_day_rg;
    SC_FLOAT sat_day_bg;

    SC_S32   gw_init_decision;
    SC_FLOAT gw_init_decision_rg;
    SC_FLOAT gw_init_decision_bg;
    SC_S32   gw_init_decision_cct;
    SC_S32   gw_init_domain_cluster;

    SC_S32   wh_cnt;
    SC_FLOAT white_rg_ratio;
    SC_FLOAT white_bg_ratio;
    SC_S32   white_decision;
    SC_FLOAT white_dist;
    SC_FLOAT awb_sgw_cluster_dist2_max_compact_cluster;
    SC_FLOAT awb_sgw_cluster_temp_min_dist;

    SC_S32   sw_policy;
    SC_FLOAT sw_policy_rg;
    SC_FLOAT sw_policy_bg;
    SC_S32   sw_policy_decision;

    SC_FLOAT ir_gain_sum;

    SC_S32 run_times;

    /* The size of this struct exceed rpc shared memory data size, so abandon it. */
    //SC_AWB_STATS_POINT_INFO_T awb_stats_point_info[SC_ISP_AWB_NEARGRAY_SIZE];
} SC_AWB_GET_INFO_T;

typedef struct
{
    SC_U32 af_mod;
    SC_U32 len_position;
    SC_U32 total_steps;
} SC_AF_SET_INFO_T;

typedef struct
{
    SC_U32 len_position;
    SC_U32 focus_value;
    SC_U32 history_pos;
    SC_U32 history_focus_value;
    SC_U32 total_steps;
} SC_AF_GET_INFO_T;

typedef struct
{
    SC_U32 low_delay_sink;
    SC_U32 sensor_hdr;
    SC_U32 eis_supported;
    SC_U32 use_face_software_stats;
    SC_U32 need_sync;
    SC_U32 vfe_mode;
    SC_U32 man_res_mask;
    SC_U32 trigger_mode;
    SC_U32 sof_mode;
    SC_S32 en_user_process_raw;
    SC_S32 max_w;
    SC_S32 max_h;

    SC_FLOAT max_fps;

    SC_S32 get_buffer_mode;
    SC_S32 get_event_mode;

    SC_S32 enable_man_aec_update;
    SC_S32 enable_man_awb_update;
    SC_S32 enable_man_af_update;
} SC_ISP_CAM_API_PRE_PRA_T;

typedef struct
{
    SC_S32 mesh_grid_stats_w;
    SC_S32 mesh_grid_stats_h;

    SC_U32 p_mesh_grid_stats_r_sum[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
    SC_U32 p_mesh_grid_stats_gr_sum[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
    SC_U32 p_mesh_grid_stats_gb_sum[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
    SC_U32 p_mesh_grid_stats_b_sum[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
    SC_U32 p_mesh_grid_stats_r_num[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
    SC_U32 p_mesh_grid_stats_gr_num[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
    SC_U32 p_mesh_grid_stats_gb_num[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
    SC_U32 p_mesh_grid_stats_b_num[SC_ISP_BLOCK_COL * SC_ISP_BLOCK_ROW];
} SC_MESH_GRID_STATS_DATA_T;

typedef struct
{
    SC_S32 af_stats_data_total_horizontal_block_num;
    SC_S32 af_stats_data_total_vertical_block_num;

    SC_U32 p_af_stats_data_sharpness_addr[SC_ISP_AF_STAT_WIDTH * SC_ISP_AF_STAT_HEIGHT];
    SC_U32 p_af_stats_data_fir_addr[SC_ISP_AF_STAT_WIDTH * SC_ISP_AF_STAT_HEIGHT];
    SC_U32 p_af_stats_data_iir_addr[SC_ISP_AF_STAT_WIDTH * SC_ISP_AF_STAT_HEIGHT];
    SC_U32 p_af_stats_data_luma_addr[SC_ISP_AF_STAT_WIDTH * SC_ISP_AF_STAT_HEIGHT];
    SC_U32 p_af_stats_data_high_num_addr[SC_ISP_AF_STAT_WIDTH * SC_ISP_AF_STAT_HEIGHT];
} SC_AF_ALGO_LIB_AF_STATUS_DATA_T;

#ifdef __cplusplus
}
#endif

#endif // __SC_ISP_TYPE_DEF__H__
