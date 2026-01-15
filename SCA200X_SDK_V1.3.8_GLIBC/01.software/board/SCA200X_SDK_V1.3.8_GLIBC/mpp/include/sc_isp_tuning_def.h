/**
 * @file     sc_comm_isp.h
 * @brief    ISP模块的宏、枚举和结构体类型定义
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

#ifndef __SC_ISP_TUNING_DEF_H__
#define __SC_ISP_TUNING_DEF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define SC_ISP_BLOCK_ROW 36
#define SC_ISP_BLOCK_COL 16
#define SC_ISP_MAX_TRIGGER_COUNT    16
#define SC_ISP_HIGH_TRIGGER_COUNT   8
#define SC_ISP_MIDDLE_TRIGGER_COUNT 5
#define SC_ISP_MIN_TRIGGER_COUNT    3
#define SC_ISP_NO_TRIGGER_COUNT     1
#define SC_ISP_LSC_TRIGGER_COUNT    6
#define SC_ISP_LSC_TABLE_NUM        208
#define SC_ISP_LSC_TUNING_NUM       300
#define SC_ISP_AWB_LIGHT_WEIGHT_COUNT  16
#define SC_ISP_AWB_LIGHT_WEIGHT_COUNT2 10
#define SC_ISP_AWB_LOWLIGHT_LUT_COUNT  16
#define SC_ISP_HDR_RATION_TALBE_SIZE   64

#define SC_ISP_HDR_FAST_RATION_TALBE_SIZE 64

#define SC_ISP_AF_BLOCK_ROW (16)
#define SC_ISP_AF_BLOCK_COL (9)

typedef enum
{
    SC_AEC_TRIGGER_GAIN,
    SC_AEC_TRIGGER_LUX,
} SC_AEC_TRIGGER_MODE_T;

typedef struct
{
    SC_FLOAT start;
    SC_FLOAT end;
} SC_TRIGGER_T;

typedef struct
{
    SC_TRIGGER_T trig_val;
} SC_AEC_TRIGGER_T;

typedef struct
{
    SC_TRIGGER_T trig_val;
} SC_AWB_TRIGGER_T;

typedef struct
{
    SC_TRIGGER_T trig_val;
} SC_EXP_RATION_TRIGGER_T;

typedef struct
{
    SC_S32 enable;
    SC_S32 count;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger_lumatarget[SC_ISP_MIN_TRIGGER_COUNT];

    SC_U32 lumtarget[SC_ISP_MIN_TRIGGER_COUNT];
} SC_LUMA_TARGET_PARA_T;

#define SC_ISP_MAX_EXPOSUER_COUNT         (1024)
#define SC_ISP_MAX_AEC_STATS_BLOCKS_COUNT (64 * 48)

typedef struct
{
    SC_U32 gain; //Q8 format, 256 as 1;
    SC_U32 line_count;
} SC_EXP_TABLE_T;

typedef struct
{
    SC_S32 count;
    SC_S32 ev0_count;
    SC_S32 ev2_count;
    SC_S32 fix_fps_index;
    SC_S32 enable_isp_digital_gain;

    SC_FLOAT max_sensor_gain;
    SC_FLOAT max_isp_gain1;

    SC_EXP_TABLE_T exp_table[SC_ISP_MAX_EXPOSUER_COUNT];

    SC_S32 enable_short_exp_table;

    SC_EXP_TABLE_T exp_table_short[SC_ISP_MAX_EXPOSUER_COUNT];
} SC_EXP_TABLE_PARA_T;

typedef struct
{
    SC_U32 r_weight;
    SC_U32 g_weight;
    SC_U32 b_weight;
} SC_RGB_TO_Y_WEIGHT_T;

typedef  struct
{
    SC_S32 weight_count;

    SC_U32 weight[SC_ISP_MAX_AEC_STATS_BLOCKS_COUNT];
    SC_U32 weight_spot[SC_ISP_MAX_AEC_STATS_BLOCKS_COUNT];
} SC_WEIGHT_TABLE_T;

typedef enum
{
    SC_BAYER_AEC,
    SC_HYBRID_AEC
} SC_STATS_TYPE_T;

#define SC_ISP_LUMA_TARGET_NUM 16

typedef struct
{
    SC_S32 exp_index;
    SC_S32 target;
} SC_LUMA_TARGET_LUT_T;

typedef struct
{
    SC_U32 outdoor_luma_target_compensated;
    SC_U32 default_luma_target_compensated;
    SC_U32 lowlight_luma_target;

    SC_FLOAT outdoor_index;
    SC_FLOAT indoor_index;

    SC_U32 lowlight_start_idx;
    SC_U32 lowlight_end_idx;
    SC_U32 luma_target_short;
    SC_S32 enable_luma_target_lut;
    SC_S32 count;

    SC_LUMA_TARGET_LUT_T luma_target[SC_ISP_LUMA_TARGET_NUM];
} SC_LUMA_TARGET_T;

typedef struct
{
    SC_U32 extreme_luma_target_offset;
} SC_SNOW_DETECT_T;

typedef struct
{
    SC_U32 backlight_max_la_luma_target_offset;
} SC_BACKLIT_DETECT_T;

typedef struct
{
    SC_U32 motion_iso_threshold;
} SC_AEC_MOTION_DETECT_T;

typedef struct
{
    SC_S32 speed;
    SC_S32 luma_tolerance;
    SC_S32 frame_skip;
    SC_S32 fine_adjust_skip;
    SC_S32 luma_nostable_torlerance;
    SC_S32 luma_nostable_torlerance_max;
    SC_S32 settled_to_nosettle_skip_count;
} SC_FAST_CONV_T;

typedef enum
{
    SC_STATS_PROC_ANTIBANDING_NO,
    SC_STATS_PROC_ANTIBANDING_50HZ,
    SC_STATS_PROC_ANTIBANDING_60HZ,
} SC_STATS_PROC_ANTIBANDING_T;

typedef enum
{
    SC_AEC_METERING_SPOT_METERING,
    SC_AEC_METERING_CENTER_WEIGHTED,
    SC_AEC_METERING_SIMPLE_FRAME_AVERAGE
} SC_AEC_METERING_MODE_T;

typedef struct
{
    SC_S32 hist_target_adjust_enable;

    SC_FLOAT outdoor_max_target_adjust_ratio;
    SC_FLOAT outdoor_min_target_adjust_ratio;
    SC_FLOAT indoor_max_target_adjust_ratio;
    SC_FLOAT indoor_min_target_adjust_ratio;
    SC_FLOAT lowlight_max_target_adjust_ratio;
    SC_FLOAT lowlight_min_target_adjust_ratio;
    SC_FLOAT target_filter_factor;
    SC_FLOAT hist_sat_pct;
    SC_FLOAT hist_dark_pct;
    SC_FLOAT hist_sat_low_ref;
    SC_FLOAT hist_sat_high_ref;
    SC_FLOAT hist_dark_low_ref;
    SC_FLOAT hist_dark_high_ref;
} SC_HIST_TARGET_T;

typedef struct
{
    SC_U32 over_exp_enable;
    SC_U32 high_luma_region_threshold;

    SC_FLOAT outdoor_over_exp_adjust_ratio;
    SC_FLOAT indoor_over_exp_adjust_ratio;
    SC_FLOAT lowlight_over_exp_adjust_ratio;
    SC_FLOAT outdoor_over_exp_adjust_offset;
    SC_FLOAT indoor_over_exp_adjust_offset;
    SC_FLOAT lowlight_over_exp_adjust_offset;

    SC_U32 outdoor_over_exp_max_count;
    SC_U32 indoor_over_exp_max_count;
    SC_U32 lowlight_over_exp_max_count;
    SC_U32 outdoor_over_exp_min_count;
    SC_U32 indoor_over_exp_min_count;
    SC_U32 lowlight_over_exp_min_count;
} SC_OVER_EXP_T;

typedef struct
{
    SC_S32   exp_index_short;

    SC_FLOAT ration0;
    SC_FLOAT ration1;
    SC_FLOAT ration2;
    SC_FLOAT ration3;

    SC_S32   luma_target;

    SC_FLOAT max_drc_gain;
    SC_FLOAT min_drc_gain;
    SC_FLOAT max_drc_gain1;

    SC_S32 luma_settle_th;
} SC_RATION_TABLE_T;

typedef struct
{
    SC_S32 en_hdr_detect;

    SC_FLOAT low_luma_hdr;
    SC_FLOAT high_luma_hdr;
    SC_FLOAT low_luma_normal;
    SC_FLOAT high_luma_nornal;

    SC_S32 skip_count_hdr_detct;
    SC_S32 use_lux_index;
    SC_S32 low_lux_index_hdr;
    SC_S32 high_lux_index_hdr;
    SC_S32 low_lux_index;
    SC_S32 high_lux_index;
} SC_HDR_DETECT_T;

typedef enum
{
    SC_AEC_HDR_ALGO_TYPE_ORG           = 0,
    SC_AEC_HDR_ALGO_TYPE_FAST          = 1,
    SC_AEC_HDR_ALGO_TYPE_FAST_ANDVANCE = 2,
    SC_AEC_HDR_ALGO_TYPE_MAX
} SC_AEC_HDR_ALGO_T;

typedef struct
{
    SC_S32 short_exp_adjust_skip_count;
    SC_U32 short_luma_interval_threshold[3];

    SC_FLOAT short_luma_speed_ratio[3];
    SC_FLOAT short_luma_speed_ratio_others;

    SC_S32 drc_gain_adjust_skip_count;

    SC_FLOAT drc_gain_interval_threshold[3];
    SC_FLOAT drc_gain_adjust_ratio[3];
    SC_FLOAT drc_gain_adjust_ratio_others;
} SC_AEC_HDR_FAST_PARAM_T;

typedef struct
{
    SC_S32 enable;
    SC_S32 enable_hdr_luma_tartget;
    SC_S32 short_exp_max_index;

    SC_FLOAT max_exp_ration;

    SC_S32 enble_ration_table;
    SC_S32 ration_talbe_size;
    SC_S32 over_exp_per_high;
    SC_S32 over_exp_per_low;
    SC_S32 adjust_step;
    SC_S32 ration_interp_en;

    SC_RATION_TABLE_T ration_table[SC_ISP_HDR_RATION_TALBE_SIZE];

    SC_S32 over_exp_bin_th;
    SC_S32 short_exp_skip_count;

    SC_FLOAT current_hist_weight;

    SC_S32   use_averge_hist;
    SC_S32   short_target_th;
    SC_S32   enable_short_exp_luma_adjust;
    SC_S32   full_sweep_en;
    SC_S32   enable_drc_gain_adjust;

    SC_FLOAT drc_gain_adjust_factor;

    SC_S32   use_over_exp_luma;

    SC_HDR_DETECT_T hdr_detect;

    SC_S32  en_drc_gain_advance_adjust;

    SC_AEC_HDR_FAST_PARAM_T aec_hdr_fast_param;

    SC_FLOAT max_mid_exp_ration;

    SC_S32 enable_over_cut_long_exp;
    SC_S32 enable_short_gain_cut_max;
} SC_HDR_TUNING_EXP_T;

typedef struct
{
    SC_S32 enable;

    SC_S32 lux_index_low;
    SC_S32 lux_index_hight;
} SC_NIGHT_DETECT_T;

typedef struct
{
    SC_S32 enable;
    SC_S32 abs_th;
} SC_MOTION_DETECT_T;

typedef enum
{
    SC_FACE_STATS_TYPE_MESH_MATCH,
    SC_FACE_STATS_TYPE_HW,
    SC_FACE_STATS_TYPE_SOFT,
    SC_FACE_STATS_TYPE_MAX,
} SC_FACE_STATS_TYPE_T;

typedef struct
{
    SC_S32 use_face_pra;
    SC_S32 face_luma_target;

    SC_FLOAT face_weight;

    SC_S32 face_skip_count;

    SC_FLOAT filter_weight;

    SC_S32 en_face_bright_dark_region;
    SC_S32 bright_high_th;
    SC_S32 bright_low_th;
    SC_S32 dark_high_th;
    SC_S32 dark_low_th;

    SC_FLOAT bright_high_th_w;
    SC_FLOAT bright_low_th_w;
    SC_FLOAT dark_high_th_w;
    SC_FLOAT dark_low_th_w;

    SC_S32 en_face_luma_cut;
    SC_S32 face_luma_high;
    SC_S32 face_luma_low;
    SC_S32 enable_w_dec;
    SC_S32 enable_region_percent;

    SC_FLOAT region_percent_low_limit;
    SC_FLOAT region_percent_low;
    SC_FLOAT region_percent_high;
    SC_FLOAT region_percent_high_limit;
    SC_FLOAT region_percent_low_limit_w;
    SC_FLOAT region_percent_low_w;
    SC_FLOAT region_percent_high_w;
    SC_FLOAT region_percent_high_limit_w;

    SC_S32 face_aec_stats_type;
} SC_FACE_AEC_PRA_T;

typedef struct
{
    SC_FLOAT lux_index;

    SC_FLOAT extreme_color_rg_th_low_limit;
    SC_FLOAT extreme_color_rg_th_low;
    SC_FLOAT extreme_color_rg_th_high;
    SC_FLOAT extreme_color_rg_th_high_limit;
    SC_FLOAT extreme_color_bg_th_low_limit;
    SC_FLOAT extreme_color_bg_th_low;
    SC_FLOAT extreme_color_bg_th_high;
    SC_FLOAT extreme_color_bg_th_high_limit;

    SC_FLOAT extreme_color_rg_th_low_limit_w;
    SC_FLOAT extreme_color_rg_th_low_w;
    SC_FLOAT extreme_color_rg_th_high_w;
    SC_FLOAT extreme_color_rg_th_high_limit_w;
    SC_FLOAT extreme_color_bg_th_low_limit_w;
    SC_FLOAT extreme_color_bg_th_low_w;
    SC_FLOAT extreme_color_bg_th_high_w;
    SC_FLOAT extreme_color_bg_th_high_limit_w;
} SC_EXTREAM_COLOR_PRA_T;

typedef struct
{
    SC_S32 extreme_color_en;
    SC_S32 extreme_count;
    SC_S32 inter_en;

    SC_EXTREAM_COLOR_PRA_T extreme_color_pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_EXTREAM_COLOR_T;

typedef struct
{
    SC_FLOAT lux_index;

    SC_FLOAT dark_th_low;
    SC_FLOAT dark_th_high;
    SC_FLOAT bright_th_low;
    SC_FLOAT bright_th_high;

    SC_FLOAT dark_th_low_w;
    SC_FLOAT dark_th_high_w;
    SC_FLOAT bright_th_low_w;
    SC_FLOAT bright_th_high_w;
} SC_BRIGHT_DRAK_PRA_T;

typedef struct
{
    SC_S32 bright_dark_en;
    SC_S32 bright_dark_count;
    SC_S32 bright_en;
    SC_S32 dark_en;
    SC_S32 inter_en;

    SC_BRIGHT_DRAK_PRA_T bright_dark_pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_BRIGHT_DRAK_T;

typedef enum
{
    SC_AEC_ALGO_TYPE_FAST_SMOOTH = 0,
    SC_AEC_ALGO_TYPE_FAST_PLUS   = 1,
    SC_AEC_ALGO_TYPE_MAX
} SC_AEC_ALGO_TYPE_T;

#define SC_ISP_AEC_FAST_PLUS_SPEED_RATIO_DEFAULT_VALUE (0.95)

typedef struct
{
    SC_S32 luma_tolerance;
    SC_S32 frame_skip;
    SC_S32 ddr_frame_skip_offset;

    SC_U32 interval_threshold[3];

    SC_FLOAT speed_ratio[3];
    SC_FLOAT speed_ratio_others;
} SC_AEC_FAST_PLUS_PARAM_T;

typedef struct
{
    SC_S32 aec_stats_type;

    SC_U32 force_exp_forced;

    SC_FLOAT force_exp_value;

    SC_S32 preview_iso_enable;

    SC_LUMA_TARGET_T       multi_luma_target;
    SC_SNOW_DETECT_T       snow_scene_detect;
    SC_BACKLIT_DETECT_T    backlit_scene_detect;
    SC_AEC_MOTION_DETECT_T aec_motion_iso_preview;

    SC_U32 R_WEIGHT;
    SC_U32 G_WEIGHT;
    SC_U32 B_WEIGHT;
    SC_U32 WT_Q;

    SC_FAST_CONV_T fast_conv;

    SC_S32 metering_type;

    SC_U32 full_sweep_en;

    SC_FLOAT exposure_index_adj_step;

    SC_U32 antibanding;

    SC_FLOAT bias_table[SC_ISP_BLOCK_ROW * SC_ISP_BLOCK_COL];

    SC_HIST_TARGET_T    hist_target;
    SC_OVER_EXP_T       over_exp_target;
    SC_HDR_TUNING_EXP_T hdr_exp_table;
    SC_NIGHT_DETECT_T   night_detect;
    SC_MOTION_DETECT_T  motion_detect;
    SC_FACE_AEC_PRA_T   face_ae;
    SC_EXTREAM_COLOR_T  extreme_color;
    SC_BRIGHT_DRAK_T    bright_dark;

    SC_U32 aec_algo_type;

    SC_AEC_FAST_PLUS_PARAM_T aec_fast_plus_param;

    SC_U32 aec_hdr_algo_type;

    SC_FAST_CONV_T   fast_conv_short;
    SC_BRIGHT_DRAK_T bright_dark_short;
} SC_AEC_TUNING_PRA;

typedef struct
{
    SC_U32 version;
    SC_U32 enable;
    SC_U32 start_exp_index;
    SC_U32 start_skip_count;

    SC_S32 torlerence;

    SC_EXP_TABLE_PARA_T exp_table_para;
    SC_AEC_TUNING_PRA   pra;
} SC_AEC_TUNING_T;

/* --------------------------   AWB   -------------------------- */
#define SC_ISP_MAX_LUMA_HYBRID_DESION_W_COUNT 16
#define SC_ISP_MAX_DISTANCE_COUNT             16
typedef enum
{
    SC_AWB_D75 = 0,
    SC_AWB_D65,
    SC_AWB_D50,
    SC_AWB_NOON,
    SC_AWB_CW,
    SC_AWB_TL84,
    SC_AWB_A,
    SC_AWB_H,
    SC_AWB_CUST1,
    SC_AWB_CUST2,
    SC_AWB_MAX, // 10
} SC_AWB_LIGHT_T;

typedef enum
{
    SC_AWB_HYBRID_D75 = 0,
    SC_AWB_HYBRID_D75_D65_1,
    SC_AWB_HYBRID_D75_D65_2,
    SC_AWB_HYBRID_D75_D65_3,
    SC_AWB_HYBRID_D65,
    SC_AWB_HYBRID_D65_D50_1,
    SC_AWB_HYBRID_D65_D50_2,
    SC_AWB_HYBRID_D65_D50_3,
    SC_AWB_HYBRID_D50,
    SC_AWB_HYBRID_NOON_LINE0,
    SC_AWB_HYBRID_NOON_LINE1, /* 10 */
    SC_AWB_HYBRID_NOON_LINE2,
    SC_AWB_HYBRID_NOON_LINE3,
    SC_AWB_HYBRID_NOON_LINE4,
    SC_AWB_HYBRID_D50_F1,
    SC_AWB_HYBRID_D50_F2,
    SC_AWB_HYBRID_D50_F3,
    SC_AWB_HYBRID_F,
    SC_AWB_HYBRID_CW,
    SC_AWB_HYBRID_CW_TL841,
    SC_AWB_HYBRID_CW_TL842,   /* 20 */
    SC_AWB_HYBRID_CW_TL843,
    SC_AWB_HYBRID_TL84,
    SC_AWB_HYBRID_F_A1,
    SC_AWB_HYBRID_F_A2,
    SC_AWB_HYBRID_F_A3,
    SC_AWB_HYBRID_A,
    SC_AWB_HYBRID_A_H1,
    SC_AWB_HYBRID_A_H2,
    SC_AWB_HYBRID_A_H3,
    SC_AWB_HYBRID_H,          /* 30 */
    SC_AWB_HYBRID_CUST1,
    SC_AWB_HYBRID_CUST2,
    SC_AWB_HYBRID_MAX         /* 33 */
} SC_AWB_HYBRID_T;

typedef struct
{
    SC_FLOAT exp_index;

    SC_FLOAT hybrid_decision_w[SC_AWB_HYBRID_MAX];
} SC_HYBRID_DECISION_W_T;

typedef struct
{
    SC_FLOAT rg;
    SC_FLOAT bg;
} SC_AWB_POINT;

typedef struct
{
    SC_FLOAT rg_adj;
    SC_FLOAT bg_adj;
} SC_AWB_GAIN_ADJ;

typedef struct
{
    SC_S32 index;

    SC_S32 D75_weight;
    SC_S32 D65_weight;
    SC_S32 D50_weight;
    SC_S32 NOON_weight;
    SC_S32 CW_weight;
    SC_S32 TL84_weight;
    SC_S32 A_weight;
    SC_S32 H_weight;
    SC_S32 custom1_weight;
    SC_S32 custom2_weight;
} SC_AWB_LIGHT_WEIGHT_TABLE_T;

typedef struct
{
    SC_S32  index0_weight;
    SC_S32  outdoor_weight;
    SC_S32  inoutdoor_weight;
    SC_S32  indoor_weight;
} SC_AWB_LIGHT_WEIGHT_TABLE2_T;

typedef struct
{
    SC_S32   lux_index;

    SC_FLOAT green_rg_offset_adj;
    SC_FLOAT green_bg_offset_adj;
    SC_FLOAT outlier_dist_adj;

    SC_FLOAT ref_point_rg_offset_adjust[SC_AWB_MAX];
    SC_FLOAT ref_point_bg_offset_adjust[SC_AWB_MAX];
} SC_AWB_LOWLIGHT_LUT_T;

typedef struct
{
    SC_FLOAT lux_index;

    SC_FLOAT weight;
} SC_STATS_FILTER_T;

typedef struct
{
    SC_FLOAT lux_index;

    SC_FLOAT outline_sub_decison[SC_AWB_HYBRID_MAX];
    SC_FLOAT outline_main_decison[SC_AWB_MAX];
} SC_OUTLINE_ARRAY_T;

typedef struct
{
    SC_S32 count;

    SC_OUTLINE_ARRAY_T outline_desion[16];
} SC_AWB_OUTLINE_T;

typedef struct
{
    SC_FLOAT lux_indx;

    SC_FLOAT awb_mesh_w[36][64];
} SC_AWB_MESH_W_T;

typedef struct
{
    SC_S32 enable;
    SC_S32 count;

    SC_AWB_MESH_W_T awb_mesh_w[16];
} SC_AWB_MESH_POSITION_W_TABLE_T;

typedef struct
{
    SC_S32 outdoor_midpoint;
    SC_S32 outdoor_index;
    SC_S32 inoutdoor_midpoint;
    SC_S32 indoor_index;
    SC_S32 exposure_adjustment;

    SC_FLOAT awb_mesh_stats_percent_th;
    SC_FLOAT d50_d65_weighted_sample_boundary;
    SC_FLOAT blue_sky_pec;
    SC_FLOAT blue_sky_pec_buffer;

    SC_S32 num_of_reference_point;

    SC_AWB_OUTLINE_T outline;

    SC_FLOAT white_stat_y_threshold_low;
    SC_FLOAT white_stat_y_threshold_high;
    SC_FLOAT dominant_cluster_threshold;
    SC_FLOAT white_stat_cnt_th;
    SC_FLOAT grey_weight_day;
    SC_FLOAT white_weight_day;
    SC_FLOAT grey_weight_f;
    SC_FLOAT white_weight_f;
    SC_FLOAT grey_weight_h;
    SC_FLOAT white_weight_h;
    SC_FLOAT all_outlier_heuristic_flag;

    SC_S32 special_bayes_en;

    SC_FLOAT green_threshold;
    SC_FLOAT threshold_extreme_b_percent;
    SC_FLOAT compact_to_grey_dis;
    SC_FLOAT cluster_high_pec;
    SC_FLOAT cluster_mid_pec;
    SC_FLOAT cluster_low_pec;

    SC_S32 BitsSum;
    SC_S32 FrameNum;
    SC_S32 enable_stats_filter;
    SC_S32 stats_filter_count;

    SC_STATS_FILTER_T input_stats_filter[5];

    SC_S32 enable_gain_filter;
    SC_S32 awb_gain_filter_count;
    SC_STATS_FILTER_T awb_gain_filter_weight[5];

    SC_S32 enalbe_limit;

    SC_FLOAT rg_limit_max;
    SC_FLOAT rg_limit_min;
    SC_FLOAT bg_limit_max;
    SC_FLOAT bg_limit_min;

    SC_S32 default_cct;
    SC_S32 stats_filter_skip;
    SC_S32 gain_filter_skip;

    SC_AWB_MESH_POSITION_W_TABLE_T positon_w_table;
} SC_AWB_TUNING_PARA;

typedef struct
{
    SC_U32 coef_y_r;       /* 0x64806c08 */
    SC_U32 coef_y_g;
    SC_U32 coef_y_b;
    SC_U32 coef_cb_r;
    SC_U32 coef_cb_g;
    SC_U32 coef_cb_b;
    SC_U32 coef_cr_r;
    SC_U32 coef_cr_g;
    SC_U32 coef_cr_b;
    SC_U32 coef_rsh;
    SC_U32 rrBL;
    SC_U32 grBL;
    SC_U32 gbBL;
    SC_U32 bbBL;
    SC_U32 rrGain;
    SC_U32 grGain;
    SC_U32 gbGain;
    SC_U32 bbGain;         /* 0x64806c8c */
    SC_U32 Ymax;           /* 0x64806c94 */
    SC_U32 Ymin;
    SC_U32 Gray_m1;
    SC_U32 Gray_m2;
    SC_U32 Gray_m3;
    SC_U32 Gray_m4;
    SC_U32 Gray_c1;
    SC_U32 Gray_c2;
    SC_U32 Gray_c3;
    SC_U32 Gray_c4;
    SC_U32 Green_BGmax;
    SC_U32 Green_BGmin;
    SC_U32 Green_RGmax;
    SC_U32 Green_RGmin;
    SC_U32 Green_Rmul;
    SC_U32 Green_Bmul;
    SC_U32 ExtremeB_BGmax;
    SC_U32 ExtremeB_RGmax;
    SC_U32 ExtremeR_RGmax;
    SC_U32 ExtremeR_BGmax; /* 0x64806ce0 */

    SC_S32 zoom;

    SC_U32 hdr_rrBL;
    SC_U32 hdr_grBL;
    SC_U32 hdr_gbBL;
    SC_U32 hdr_bbBL;
    SC_U32 hdr_rrGain;
    SC_U32 hdr_grGain;
    SC_U32 hdr_gbGain;
    SC_U32 hdr_bbGain;     /* 0x64806c8c */
} SC_AWB_STATISTICS;

typedef struct
{
    SC_S32 count;

    SC_FLOAT dist_percent[SC_ISP_MAX_DISTANCE_COUNT];
    SC_FLOAT distance_w[SC_ISP_MAX_DISTANCE_COUNT];
} SC_DIST_W_T;

typedef struct
{
    SC_S32 enable;

    SC_FLOAT dark_low_th;
    SC_FLOAT dark_low_th_w;
    SC_FLOAT dark_high_th;
    SC_FLOAT dark_high_th_w;

    SC_FLOAT bright_low_th;
    SC_FLOAT bright_low_th_w;
    SC_FLOAT bright_high_th;
    SC_FLOAT bright_high_th_w;
} SC_MESH_LUMA_W_T;

typedef struct
{
    SC_S32 luma_count;

    SC_DIST_W_T distance_w;
    SC_HYBRID_DECISION_W_T  hybrid_decision_w[SC_ISP_MAX_LUMA_HYBRID_DESION_W_COUNT];
    SC_MESH_LUMA_W_T mesh_luma;

    SC_S32 enable_no_grey_keep_settle_decison;
    SC_S32 enable_keep_no_grey_grey;

    SC_FLOAT keep_th_no_grey_to_grey;
    SC_FLOAT keep_th_grey_to_no_grey;

    SC_S32 enable_subsample;
    SC_S32 w_ration;
    SC_S32 h_ration;
} SC_AWB_ADVANCE_WEIGHT_AVERAGE_T;

typedef struct
{
    SC_FLOAT x;
    SC_FLOAT y;
} SC_AWB_MESH_POINT;

typedef struct
{
    SC_S32 point_num;
    SC_S32 is_square;

    SC_FLOAT min_x;
    SC_FLOAT min_y;
    SC_FLOAT max_x;
    SC_FLOAT max_y;

    SC_AWB_MESH_POINT point_vector[64];

    SC_AWB_POINT ref_point;
    SC_AWB_POINT center_point;
} SC_AWB_MESH_REGION;

typedef struct
{
    SC_S32 lux_index;

    SC_AWB_MESH_REGION greeen_region;
    SC_AWB_MESH_REGION greeen_region_d;
    SC_AWB_MESH_REGION greeen_region_t;
    SC_AWB_MESH_REGION greeen_region_a;
} SC_AWB_MESH_GREEN_REGION;

typedef struct
{
    SC_S32 enable;
    SC_S32 luma_count;
    SC_S32 enalbe_dta_light_inter;

    SC_FLOAT green_percent_th;

    SC_AWB_MESH_GREEN_REGION awb_green_region[SC_ISP_MAX_TRIGGER_COUNT];
} SC_AWB_GREEN_REGION_TUNING_T;

typedef struct
{
    SC_S32 lux_index;
    SC_S32 misleading_region_count;

    SC_AWB_MESH_REGION misleading_region[64];
} SC_AWB_MISLEADING_REGION;

typedef struct
{
    SC_S32 enable;
    SC_S32 aec_count;

    SC_AWB_MISLEADING_REGION misleading_region[SC_ISP_MAX_TRIGGER_COUNT];
} SC_AWB_MISLEADING_T;

typedef struct
{
    SC_S32 enable;

    SC_FLOAT th;
    SC_FLOAT percent_th;

    SC_AWB_POINT ref_point;
} SC_AWB_EXTREAM_COLOR_RED;

typedef struct
{
    SC_S32 enable;

    SC_FLOAT th;
    SC_FLOAT percent_th;

    SC_AWB_POINT ref_point;
} SC_AWB_EXTREAM_COLOR_BLUE;

typedef struct
{
    SC_S32 lux_index;

    SC_AWB_GAIN_ADJ gain_adj[SC_AWB_MAX];
} SC_AWB_GAIN_ADJUST_WB_T;

typedef struct
{
    SC_S32 count;

    SC_AWB_GAIN_ADJUST_WB_T gain_adj[SC_ISP_HIGH_TRIGGER_COUNT];
} SC_AWB_GAIN_ADJUST_LUT_WB_T;

typedef struct
{
    SC_S32 enable;
    SC_S32 lux_index;
    SC_S32 skip_frames;
    SC_S32 frames_run_every_scan;
} SC_AWB_SACAN_PRA_T;

typedef struct
{
    SC_S32 enable_awb_scan_mode;
    SC_S32 frames_start_scan;
    SC_S32 count;

    SC_AWB_SACAN_PRA_T scan_pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_AWB_SACAN_MODE_T;

typedef  struct
{
    SC_S32 version;
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 num_point;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIN_TRIGGER_COUNT];
    SC_AWB_POINT     point[SC_AWB_MAX];
    SC_AWB_GAIN_ADJ  gain_adj[SC_AWB_MAX];

    SC_S32   awb_cct[SC_AWB_MAX];
    SC_FLOAT distance[SC_AWB_MAX];
    SC_S32   index_name[SC_AWB_MAX];

    SC_AWB_LIGHT_WEIGHT_TABLE_T  light_weight_table[SC_ISP_AWB_LIGHT_WEIGHT_COUNT];
    SC_AWB_LIGHT_WEIGHT_TABLE2_T light_weight_table2[SC_ISP_AWB_LIGHT_WEIGHT_COUNT2];
    SC_AWB_LOWLIGHT_LUT_T        awb_lowlight_lut[SC_ISP_AWB_LOWLIGHT_LUT_COUNT];
    SC_AWB_TUNING_PARA           para;
    SC_AWB_STATISTICS            stats_awb[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_S32 enable_distance_v1;
    SC_S32 enable_awb_advace_average;

    SC_AWB_ADVANCE_WEIGHT_AVERAGE_T advance_avg;

    SC_S32 enable_gain_adjust;
    SC_S32 enable_lowlight_lut;
    SC_S32 lowlight_lut_count;
    SC_S32 force_simple_grey_word;
    SC_S32 enable_init_point;
    SC_S32 use_init_point_count;

    SC_AWB_POINT                 init_point;
    SC_AWB_EXTREAM_COLOR_BLUE    extrem_color_blue;
    SC_AWB_EXTREAM_COLOR_RED     extrem_color_red;
    SC_AWB_MISLEADING_T          misleading;
    SC_AWB_GREEN_REGION_TUNING_T green_region;

    SC_S32 enable_gain_adj_lux_wb;

    SC_AWB_GAIN_ADJUST_LUT_WB_T gain_adj_lux_wb;
    SC_AWB_SACAN_MODE_T         awb_scan_mode;
} SC_AWB_TUNING_T;

/* --------------------------   AF   -------------------------- */
typedef struct
{
    SC_U32 move_lens_steps;
    SC_U32 damp_value0;
    SC_U32 damp_value1;
} SC_DAMPING_T;

typedef struct
{
    SC_S32 enable;
    SC_S32 damping_enable;
    SC_S32 init_dac;
    SC_S32 infinit_dac;
    SC_S32 hyperfocal_dac;
    SC_S32 near_dac;
    SC_S32 dmp_count;

    SC_DAMPING_T move_to_far_damping[16];
    SC_DAMPING_T move_to_near_damping[16];
} SC_ACTUATOR_TUNNING_PRA_T;

typedef enum
{
    SC_FOCUS_FULL_SWEEP,
    SC_FOCUS_SINGLE,
} SC_FOCUS_ALGO_TYPE_T;

typedef struct
{
    SC_U32 code_per_step;
    SC_U32 skip_count_after_lens_move;
} SC_FULLSWEEP_ALGO_TUNING_T;

typedef enum
{
    SC_AF_STATS_SHARPNES = 1 << 0,
    SC_AF_STATS_FIR      = 1 << 1,
    SC_AF_STATS_IIR      = 1 << 2,
} SC_AF_STATS_T;

typedef struct
{
    SC_FLOAT x;
    SC_FLOAT y;

    SC_FLOAT width;
    SC_FLOAT height;
} SC_ROI_T;

/** af_run_mode_type:
*
* Enum to distinguish if it's camcorder or camera mode.
*
**/
typedef enum
{
    SC_AF_RUN_MODE_INIT,
    SC_AF_RUN_MODE_CAMERA,
    SC_AF_RUN_MODE_VIDEO,
    SC_AF_RUN_MODE_SNAPSHOT
} SC_AF_RUN_T;

/** af_SC_ROI_Type:
*
* Enum to indicate what type of ROI information we have received.
*
**/
typedef enum
{
    SC_AF_ROI_TYPE_GENERAL = 0x0, /* Default */
    SC_AF_ROI_TYPE_FACE,          /* Face priority AF */
    SC_AF_ROI_TYPE_TOUCH,         /* Touch-AF */
} SC_AF_ROI_T;

typedef enum
{
    SC_ACT_TYPE_CLOSELOOP,
    SC_ACT_TYPE_OPENLOOP
} SC_ACT_TYPE_T;

typedef struct
{
    SC_S32    enable;

    SC_FLOAT gain_min;
    SC_FLOAT gain_max;
    SC_FLOAT ref_gain_min;
    SC_FLOAT ref_gain_max;

    SC_U32  threshold_min;
    SC_U32  threshold_max;
    SC_U32  ref_threshold_min;
    SC_U32  ref_threshold_max;
    SC_U32  frames_to_wait;
} SC_AF_TUNING_SAD_T;

typedef struct
{
    SC_S32 far_zone;
    SC_S32 mid_zone;
    SC_S32 near_zone;

    SC_S32 init_pos;
    SC_S32 far_start_pos;
    SC_S32 near_start_pos;

    SC_S32 TAF_far_end;
    SC_S32 TAF_near_end;

    SC_S32 CAF_far_end;
    SC_S32 CAF_near_end;

    SC_S32 srch_rgn_1;
    SC_S32 srch_rgn_2;
    SC_S32 srch_rgn_3;

    SC_S32 fine_srch_rgn;
} SC_SINGLE_OPTIC_T;

typedef struct
{
    SC_FLOAT thres[10];
} SC_THRES_T;

typedef struct
{
    SC_U32 hist_dec_dec_thres;

    SC_FLOAT drop_thres;

    SC_THRES_T dec_dec_3frame;
    SC_THRES_T inc_dec;
    SC_THRES_T inc_dec_3frame;
    SC_THRES_T dec_dec;
    SC_THRES_T dec_dec_noise;
    SC_THRES_T flat_threshold;

    SC_U32 hist_inc_dec_thres;

    SC_FLOAT flat_dec_thres;
    SC_FLOAT flat_inc_thres;
    SC_FLOAT macro_thres;
} SC_SINGLE_THRESHOLD_T;

typedef struct
{
    SC_S32 rgn_0;
    SC_S32 rgn_1;
    SC_S32 rgn_2;
    SC_S32 rgn_3;
    SC_S32 rgn_4;
} SC_STEP_RGN_T;

typedef struct
{
    SC_STEP_RGN_T Prescan_low_light;
    SC_STEP_RGN_T Prescan_normal_light;
    SC_STEP_RGN_T Finescan_low_light;
    SC_STEP_RGN_T Finescan_normal_light;
} SC_STEP_SIZE_TABLE_T;

typedef struct
{
    SC_AF_TUNING_SAD_T af_par_sad;
} SC_AF_MONIT_T;

typedef struct
{
    SC_SINGLE_OPTIC_T optics;

    SC_S32 actuator_type;
    SC_S32 index[50];
    SC_S32 is_hys_comp_needed;

    SC_SINGLE_THRESHOLD_T hw;

    SC_FLOAT BV_gain[10];

    SC_U16 step_index_per_um;

    SC_STEP_SIZE_TABLE_T CAF_step_table;
    SC_STEP_SIZE_TABLE_T TAF_step_table;

    SC_S32 skip_frame[3];
} SC_AF_TUNING_SINGLE_T;

#define SC_ISP_AF_STATS_TABLE_SIZE (128 * 128 / 8 / 4 / 2)

typedef struct
{
    SC_FLOAT trigger_val;

    SC_S32 coef_y_b;
    SC_S32 coef_y_gb;
    SC_S32 coef_y_gr;
    SC_S32 coef_y_r;

    SC_S32 fv_mod;
    SC_S32 ch_sel;
    SC_S32 zoom_en;
    SC_S32 zoom_dwn;
    SC_S32 compander_en;
    SC_S32 lowfilter_en;
    SC_S32 high_luma_th;
    SC_S32 blc_b;
    SC_S32 blc_gb;
    SC_S32 blc_r;
    SC_S32 blc_gr;

    SC_S32 sharp_th;
    SC_S32 sharp_bitshift;
    SC_S32 sharp_sqr_bitshift;
    SC_S32 sharp_filter_coff[39];

    SC_S32 fir_th;
    SC_S32 fir_bitshift;
    SC_S32 fir_sqr_bitshift;
    SC_S32 fir_filter_coff[3];

    SC_S32 iir1_en;
    SC_S32 iir0_en;
    SC_S32 iir_bitshift_2;
    SC_S32 iir_bitshift_1;
    SC_S32 iir_th;
    SC_S32 iir_filter_coff[10];
    SC_S32 iir_sqr_bitshift;

    SC_U32 gamma[SC_ISP_AF_STATS_TABLE_SIZE];
} SC_AF_STATS_TUNING_T;

typedef struct
{
    SC_S32 version;
    SC_S32 enable;
    SC_U32 infinite;
    SC_U32 near;
    SC_U32 hyperfocal;
    SC_U32 init_skip;
    SC_S32 algo_type;

    SC_FULLSWEEP_ALGO_TUNING_T full_sweep_tuning_pra;

    SC_S32 af_stats_type;

    SC_ROI_T roi;

    SC_FLOAT stats_weight_table[SC_ISP_AF_BLOCK_ROW * SC_ISP_AF_BLOCK_COL];

    /* The size of this struct exceed rpc shared memory data size, so abandon it. */
    //SC_ACTUATOR_TUNNING_PRA_T atcuator;

    SC_U32 SC_ROI_Type; /* AF_ROI_TYPE_GENERAL */
    SC_U32 run_mode;    /* AF_RUN_MODE_VIDEO */

    /* The size of this struct exceed rpc shared memory data size, so abandon it. */
    //SC_AF_MONIT_T         af_monit;
    //SC_AF_TUNING_SINGLE_T single_pra;

    SC_S32 count_ae;
    SC_S32 trigger_mode;
    SC_S32 interpolation_enable;

    SC_AEC_TRIGGER_T     aec_trigger[SC_ISP_MIN_TRIGGER_COUNT];
    SC_AF_STATS_TUNING_T af_stats_tuning_pra[SC_ISP_MIN_TRIGGER_COUNT];
} SC_AF_TUNING_T;

/* --------------------------   EIS   -------------------------- */
typedef struct
{
    SC_S32 enable;
    SC_S32 eis_enable;
    SC_S32 ldc_enable;
    SC_S32 angle_th;
} SC_EIS_TUNING_T;

/* --------------------------   RAW CROP   -------------------------- */
typedef struct
{
    SC_S32 enable;

    SC_S32 interpolation_enable;
} SC_ISP_SUB_MODULE_CROP_TUNING_T;

/* --------------------------   BLC   -------------------------- */
typedef struct
{
    SC_U32 blc_R;
    SC_U32 blc_B;
    SC_U32 blc_GR;
    SC_U32 blc_GB;
    SC_U32 blc_gain_R;
    SC_U32 blc_gain_B;
    SC_U32 blc_gain_GR;
    SC_U32 blc_gain_GB;
} SC_ISP_SUB_MODULE_BLC_PARA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_BLC_PARA pra[SC_ISP_MAX_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_BLC_TUNING_T;

/* --------------------------   HDR   -------------------------- */
typedef struct
{
    SC_U32 blc_short_exp;
    SC_U32 blc_mid_exp;
    SC_U32 blc_long_exp;

    SC_S32 hdr_motion_en_sm;
    SC_S32 hdr_alpha_s_th_sm;
    SC_S32 hdr_exp_value_th1_sm;
    SC_S32 hdr_th2_th1_reverse_sm;
    SC_S32 hdr_noise_sp_sm;
    SC_S32 hdr_motion_mp_sm;
    SC_S32 hdr_motion_en_sl;
    SC_S32 hdr_alpha_s_th_sl;
    SC_S32 hdr_exp_value_th1_sl;
    SC_S32 hdr_th2_th1_reverse_sl;
    SC_S32 hdr_noise_sp_sl;
    SC_S32 hdr_motion_mp_sl;
    SC_S32 hdr_md_th4_sl;
    SC_S32 hdr_md_th9_sl;
    SC_S32 hdr_md_th16_sl;
    SC_S32 hdr_md_th25_sl;
    SC_U32 noise_profile_l[129];
    SC_U32 noise_profile_m[129];
    SC_U32 noise_profile_s[129];

    SC_S32 hdr_policy;
    SC_S32 enable_hdr_blc;
    SC_S32 mv_fix_th;
    SC_S32 mv_fix_value;
    SC_S32 mv_fix_en;
    SC_S32 use_long_exp_fix;
    SC_S32 use_mv_fix_value_fix;
    SC_S32 enable_sexp_denoise;
    SC_S32 gaus_coef[4];
} SC_ISP_SUB_MODULE_HDR_MIX_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_HDR_MIX_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_HDR_MIX_TUNING_T;

/* --------------------------   COMPANDER   -------------------------- */
typedef struct
{
} SC_ISP_SUB_MODULE_COMPANDER_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 count_awb;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_NO_TRIGGER_COUNT];
    SC_AWB_TRIGGER_T awb_trigger[SC_ISP_NO_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_COMPANDER_PRA pra[SC_ISP_NO_TRIGGER_COUNT][SC_ISP_NO_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_COMPANDER_TUNING_T;

/* --------------------------   DPC   -------------------------- */
typedef struct
{
    SC_U32 factor;
    SC_U32 dpc_mode;
    SC_U32 f_th_d;
    SC_U32 f_th_c;
    SC_U32 f_th_d2;
    SC_U32 f_th_d_dark;
    SC_U32 f_th_c_dark;
    SC_U32 f_th_d2_dark;
    SC_U32 e_th_d;
    SC_U32 e_th_c;
    SC_U32 e_th_d2;
    SC_U32 e_th_d_dark;
    SC_U32 e_th_c_dark;
    SC_U32 e_th_d2_dark;
    SC_U32 f_ratio;
    SC_U32 avg_th1;
    SC_U32 avg_th2;
    SC_U32 avg_th3;
    SC_U32 avg_th4;
    SC_U32 avg_th5;
    SC_U32 std_wt_th1;
    SC_U32 std_wt_th2;
    SC_U32 std_wt_th3;
    SC_U32 std_wt_th4;
    SC_U32 std_wt_th5;
} SC_ISP_SUB_MODULE_DPC_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 dpc_cal_mode;
    SC_S32 trigger_mode;
    SC_S32 dpc_pra_enable[SC_ISP_MAX_TRIGGER_COUNT];

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_DPC_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_DPC_TUNING_T;

/* --------------------------   CAC   -------------------------- */
typedef struct
{
    SC_S32 rx_p[20];
    SC_S32 bx_p[20];
} SC_ISP_SUB_MODULE_CAC_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 center_mode;

    SC_FLOAT center_width;
    SC_FLOAT center_heigh;

    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_CAC_PRA pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_CAC_TUNING_T;

/* --------------------------   ATA   -------------------------- */
typedef struct
{
} SC_ISP_SUB_MODULE_ATA_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_ATA_PRA pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_ATA_TUNING_T;

/* --------------------------   RAR   -------------------------- */
#define SC_ISP_RNR_LAYER_COUNT_MAX 2
#define SC_ISP_RNR_CHANNEL_COUMT 4
#define SC_ISP_RNR_EDGE_COUNT 2
typedef struct
{
    SC_S32 t1;
    SC_S32 t2;
    SC_S32 t3;
} SC_ISP_SUB_MODULE_RNR_LAY;

typedef struct
{
    SC_S32 noise_profile_region[3];
    SC_S32 noise_profile_gain[4];
} SC_PROFILE_T;

typedef struct
{
    SC_S32 edge_v[8];
    SC_S32 edge_level[9];
} SC_EDGE_T;

typedef struct
{
    SC_S32 denoise_layer;
    SC_S32 profile_enable;

    SC_ISP_SUB_MODULE_RNR_LAY lay[SC_ISP_RNR_LAYER_COUNT_MAX][SC_ISP_RNR_CHANNEL_COUMT];

    SC_PROFILE_T profile[SC_ISP_RNR_CHANNEL_COUMT];

    SC_EDGE_T edge[SC_ISP_RNR_EDGE_COUNT];
} SC_ISP_SUB_MODULE_RNR_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 gray_mode;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_RNR_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_RNR_TUNING_T;

/* --------------------------   DECOMPANDER   -------------------------- */
typedef struct
{
} SC_ISP_SUB_MODULE_DECOMPANDER_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 count_awb;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_NO_TRIGGER_COUNT];
    SC_AWB_TRIGGER_T awb_trigger[SC_ISP_NO_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_DECOMPANDER_PRA pra[SC_ISP_NO_TRIGGER_COUNT][SC_ISP_NO_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_DECOMPANDER_TUNING_T;

/* --------------------------   LSC   -------------------------- */
typedef struct
{
    SC_FLOAT lsc_strength;

    SC_U32 lsc_man_mode;

    SC_FLOAT lsc_table[SC_ISP_LSC_TUNING_NUM];
} SC_ISP_SUB_MODULE_LSC_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 count_awb;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_LSC_TRIGGER_COUNT];
    SC_AWB_TRIGGER_T awb_trigger[SC_ISP_LSC_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_LSC_PRA pra[SC_ISP_LSC_TRIGGER_COUNT][SC_ISP_LSC_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_LSC_TUNING_T;

/* --------------------------   DRC   -------------------------- */
#define SC_ISP_DRC_LUT_TUINING_SIZE 257
#define SC_ISP_DRC_LUT_REG_SIZE     256
typedef struct
{
    SC_S32 low_lut[257];
    SC_S32 high_lut[257];

    SC_S32 saturation;

    SC_S32 fl1[3][5];
    SC_S32 fl2[3][5];
    SC_S32 fl3[3][5];

    SC_FLOAT k1;
    SC_FLOAT k2;
    SC_FLOAT k3;
} SC_ISP_SUB_MODULE_DRC_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;
    SC_S32 drc_point_size;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_DRC_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];

    SC_S32 enable_auto_gen;
} SC_ISP_SUB_MODULE_DRC_TUNING_T;

/* --------------------------   GIC   -------------------------- */
typedef struct
{
    SC_S32 enable;
    SC_S32 alphfa1;
    SC_S32 alphfa_red;
    SC_S32 k_filter_power;
    SC_S32 b_value;
    SC_S32 ge_thres;
    SC_S32 g_slope;
    SC_S32 max_thres;
    SC_S32 k_thres;
    SC_S32 limit_base_green;
    SC_S32 limit_base_red;
    SC_S32 limit_end_green;
    SC_S32 limit_end_red;
    SC_S32 weight_filterg;
    SC_S32 k_diff_base;
} SC_ISP_SUB_MODULE_GIC_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_GIC_PRA pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_GIC_TUNING_T;

/* --------------------------   CFA   -------------------------- */
typedef struct
{
    SC_S32 edge_offset0;
    SC_S32 edge_offset1;
    SC_S32 edge_offset2;
    SC_S32 edge_offset3;

    SC_S32 gfilter_mid;
    SC_S32 gfilter_peak;
    SC_S32 gfilter_lrud;

    SC_S32 gfactor_1;
    SC_S32 gfactor_2;
    SC_S32 gfactor_3;
    SC_S32 gfactor_4;

    SC_S32 th1_y;
    SC_S32 th2_y;
    SC_S32 th1_ny;
    SC_S32 th2_ny;

    SC_S32 ny_correct_en;
    SC_S32 ny_weight_factor;
    SC_S32 ny_weight_factor_1minus;

    SC_S32 ahd_th;
    SC_S32 hvwt_th;

    SC_S32 flat_hvwt_factor;
    SC_S32 flat_hvwt_factor_1minus;

    SC_S32 detail_same_factor;
    SC_S32 detail_same_factor_1minus;
    SC_S32 detail_diff_factor;
    SC_S32 detail_diff_factor_1minus;

    SC_S32 ny_same_factor;
    SC_S32 ny_same_factor_1minus;
    SC_S32 ny_diff_factor;
    SC_S32 ny_diff_factor_1minus;

    SC_S32 dpp_correct;
    SC_S32 luma_detect_en;
    SC_S32 th1_very_light;
    SC_S32 th2_y_max;
    SC_S32 rgb_diff_detect_en;
    SC_S32 th1_rgb_diff;
    SC_S32 th2_rgb_diff;
    SC_S32 edge_detect_en;
    SC_S32 th1_edge;
    SC_S32 th2_edge;
    SC_S32 dpp_strength;
} SC_ISP_SUB_MODULE_CFA_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_CFA_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];;
} SC_ISP_SUB_MODULE_CFA_TUNING_T;

/* --------------------------   DEPURPLE   -------------------------- */
typedef struct
{
    SC_S32 enable;

    SC_S32 min_rgb_div;
    SC_S32 max_rgb_div;

    SC_S32 max_ratio_rgb;
    SC_S32 min_ratio_rgb;

    SC_S32 mult_low1;
    SC_S32 mult_high1;

    SC_S32 g_low1;
    SC_S32 g_high1;

    SC_S32 color_rg_ratio;
    SC_S32 color_bg_ratio;

    SC_S32 edge_min_y;

    SC_S32 th_edge_low;
    SC_S32 th_edge_high;

    SC_S32 ration_max_median1;
    SC_S32 ration_max_dedian2;

    SC_S32 y_max;
    SC_S32 t_verylight;

    SC_S32 rgb_diff_th1;
    SC_S32 rgb_diff_th2;

    SC_S32 gf3x3_eps;
    SC_S32 gf3x1_eps;

    SC_S32 strength_ac;
    SC_S32 strength_ag;

    SC_S32 rgm_coring_low;
    SC_S32 rgm_coring_high;
    SC_S32 rgm_coring_slope;

    SC_S32 bgm_coring_low;
    SC_S32 bgm_coring_high;
    SC_S32 bgm_coring_slop;

    SC_S32 ratio_y_low;

    SC_S32 low_light_y_low_th;
    SC_S32 low_light_y_high_th;

    SC_S32 purple_cen;
    SC_S32 mangenta_cen;

    SC_S32 purple_range_high;
    SC_S32 purple_range_low;

    SC_S32 mangenta_range_high;
    SC_S32 mangenta_range_low;

    SC_S32 rgb_gb_g_ratio_max;

    SC_S32 saturation_highk_th;
    SC_S32 saturation_low_th;

    SC_S32 max_abs_rg_bg_g_ratio_hight;
    SC_S32 max_abs_rg_bg_g_ratio_low;

    SC_S32 depurple_wr;
    SC_S32 depurple_wg;
    SC_S32 depurple_wb;
} SC_ISP_SUB_MODULE_DEPURPLE_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;

    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_DEPURPLE_PRA pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_DEPURPLE_TUNING_PRA;

/* --------------------------   CCM1   -------------------------- */
#define SC_ISP_CCM_AEC_TRIGGER_COUNT 5
#define SC_ISP_CCM_AWB_TRIGGER_COUNT 7
typedef struct
{
    SC_FLOAT rr;
    SC_FLOAT rg;
    SC_FLOAT rb;
    SC_FLOAT gr;
    SC_FLOAT gg;
    SC_FLOAT gb;
    SC_FLOAT br;
    SC_FLOAT bg;
    SC_FLOAT bb;
} SC_ISP_SUB_MODULE_CCM1_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 count_awb;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_CCM_AEC_TRIGGER_COUNT];
    SC_AWB_TRIGGER_T awb_trigger[SC_ISP_CCM_AWB_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_CCM1_PRA pra[SC_ISP_CCM_AEC_TRIGGER_COUNT][SC_ISP_CCM_AWB_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_CCM1_TUNING_T;

/* --------------------------   CCM2   -------------------------- */
typedef struct
{
    SC_FLOAT rr;
    SC_FLOAT rg;
    SC_FLOAT rb;
    SC_FLOAT gr;
    SC_FLOAT gg;
    SC_FLOAT gb;
    SC_FLOAT br;
    SC_FLOAT bg;
    SC_FLOAT bb;
} SC_ISP_SUB_MODULE_CCM2_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 count_awb;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_CCM_AEC_TRIGGER_COUNT];
    SC_AWB_TRIGGER_T awb_trigger[SC_ISP_CCM_AWB_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_CCM1_PRA pra[SC_ISP_CCM_AEC_TRIGGER_COUNT][SC_ISP_CCM_AWB_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_CCM2_TUNING_T;

/* --------------------------   GTM1   -------------------------- */
typedef struct
{
    SC_S32 grey_max_flag;

    SC_U32 k[257];
} SC_ISP_SUB_MODULE_GTM1_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIN_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_GTM1_PRA pra[SC_ISP_MIN_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_GTM1_TUNING_T;

/* --------------------------   GAMMA   -------------------------- */
typedef struct
{
    SC_U32 k[4096];
} SC_ISP_SUB_MODULE_GAMMA_LUT_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_S32 gamma_lut_size;

    SC_ISP_SUB_MODULE_GAMMA_LUT_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];

    SC_FLOAT exp_ratio_en;

    SC_S32 exp_ratio_inter_en;
    SC_S32 exp_ratio_count;

    SC_FLOAT w_of_exp_ratio[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_EXP_RATION_TRIGGER_T exp_ratio_trigger[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_GAMMA_LUT_PRA pra_exp_raion[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_GAMMA_LUT_TUNING_T;

/* --------------------------   GTM2   -------------------------- */
typedef struct
{
    SC_S32 enable_cdf_smooth;

    SC_FLOAT cut_ration;
    SC_FLOAT weight;

    SC_S32 ration;
    SC_S32 low_cut;
    SC_S32 enable_free_haho;
    SC_S32 filt_x;
    SC_S32 filt_y;

    SC_FLOAT PARAM_FILT;

    SC_S32 bin_meger;
    SC_S32 ltm_smooth;
    SC_S32 ltm_dehighlight;
    SC_S32 ltm_base;

    SC_FLOAT ltm_smooth_fork;
    SC_FLOAT ltm_smooth_spear;
    SC_FLOAT ltm_smooth_gamma;
    SC_FLOAT ltm_dehighlight_fork;
    SC_FLOAT ltm_dehighlight_spear;
    SC_FLOAT ltm_dehighlight_gamma;
    SC_FLOAT ltm_dark_fork;
    SC_FLOAT ltm_dark_spear;

    SC_S32   ltm_smooth_noise_erode_area;
    SC_S32   ltm_smooth_filter_area;
} SC_ISP_SUB_MODULE_GTM2_LUT_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_GTM2_LUT_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];

    SC_S32 enable_tuning_mesh;
    SC_S32 mesh_w;
    SC_S32 mesh_h;
} SC_ISP_SUB_MODULE_GTM2_LUT_TUNING_T;

/* --------------------------   3DLUT   -------------------------- */
#define SC_ISP_LUT3D_TABLE_SIZE        1230
#define SC_ISP_LUT3D_CHANEL_NUM        3
#define SC_ISP_LUT3D_AEC_TRIGGER_COUNT 3
#define SC_ISP_LUT3D_AWB_TRIGGER_COUNT 3

typedef struct
{
    SC_U32 lut3d_table0[SC_ISP_LUT3D_TABLE_SIZE][SC_ISP_LUT3D_CHANEL_NUM];
    SC_U32 lut3d_table1[SC_ISP_LUT3D_TABLE_SIZE][SC_ISP_LUT3D_CHANEL_NUM];
    SC_U32 lut3d_table2[SC_ISP_LUT3D_TABLE_SIZE][SC_ISP_LUT3D_CHANEL_NUM];
    SC_U32 lut3d_table3[SC_ISP_LUT3D_TABLE_SIZE][SC_ISP_LUT3D_CHANEL_NUM];
} SC_ISP_SUB_MODULE_3D_LUT_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 count_awb;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_LUT3D_AEC_TRIGGER_COUNT];
    SC_AWB_TRIGGER_T awb_trigger[SC_ISP_LUT3D_AWB_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_3D_LUT_PRA pra[SC_ISP_LUT3D_AEC_TRIGGER_COUNT][SC_ISP_LUT3D_AWB_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_3D_LUT_TUNING_T;

/* --------------------------   RGB2YUV   -------------------------- */
typedef struct
{
} SC_ISP_SUB_MODULE_RGB2YUV_PRA;

typedef struct
{
    SC_S32 enable;
} SC_ISP_SUB_MODULE_RGB2YUV_TUNING_T;

/* --------------------------   CM1   -------------------------- */
#define SC_ISP_CM_AEC_TRIGGER_COUNT 5
#define SC_ISP_CM_AWB_TRIGGER_COUNT 7

typedef struct
{
    SC_FLOAT saturation;
    SC_FLOAT hue;
} SC_ISP_SUB_MODULE_CM_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 count_awb;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_CM_AEC_TRIGGER_COUNT];
    SC_AWB_TRIGGER_T awb_trigger[SC_ISP_CM_AWB_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_CM_PRA pra[SC_ISP_CM_AEC_TRIGGER_COUNT][SC_ISP_CM_AWB_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_CM_TUNING_T;

/* --------------------------   CM2   -------------------------- */
typedef struct
{
    SC_FLOAT saturation;
    SC_FLOAT hue;

    SC_S32 y_lo_th1;
    SC_S32 y_lo_th2;
    SC_S32 y_hi_th1;
    SC_S32 y_hi_th2;
} SC_ISP_SUB_MODULE_CM2_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 count_awb;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_CM_AEC_TRIGGER_COUNT];
    SC_AWB_TRIGGER_T awb_trigger[SC_ISP_CM_AWB_TRIGGER_COUNT];
    SC_ISP_SUB_MODULE_CM2_PRA pra[SC_ISP_CM_AEC_TRIGGER_COUNT][SC_ISP_CM_AWB_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_CM2_TUNING_T;

/* --------------------------   LEE   -------------------------- */
typedef struct
{
    SC_S32 enable_noise_level_weight_edge;
    SC_S32 enable_skin_detection;
    SC_S32 enable_shink_operation_for_haolo_side;
    SC_S32 enable_luma_weight_function;
    SC_S32 enable_mono_mode;

    SC_S32 skin_weight;

    SC_S32 bpf[4][4];
    SC_S32 hpf[3][4];

    SC_S32 strength_pos_edge;
    SC_S32 strength_neg_edge;
    SC_S32 overshoot_pos_edge;
    SC_S32 overshoot_neg_edge;

    SC_S32 colour_u_p[4];
    SC_S32 colour_u_pdes[5];
    SC_S32 colour_u_slop[5];
    SC_S32 colour_v_p[4];
    SC_S32 colour_v_pdes[5];
    SC_S32 colour_v_slop[5];

    SC_S32 conv_3x3_ration_constant;
    SC_S32 conv_1x7_ration_constant;

    SC_S32 edge_w[64];
    SC_S32 limitation_for_move_strength;
    SC_S32 adjust_strength;

    SC_S32 coef_moving_c00;
    SC_S32 coef_moving_c01;
    SC_S32 coef_moving_c10;
    SC_S32 coef_moving_c11;

    SC_S32 shink_w[64];
    SC_S32 luma_w[64];
} SC_ISP_SUB_MODULE_LEE_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_LEE_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_LEE_TUNING_T;

/* --------------------------   CNF   -------------------------- */
typedef struct
{
    SC_S32 coef_r_y;
    SC_S32 coef_r_cb;
    SC_S32 coef_r_cr;
    SC_S32 coef_g_y;
    SC_S32 coef_g_cb;
    SC_S32 coef_g_cr;
    SC_S32 coef_b_y;
    SC_S32 coef_b_cb;
    SC_S32 coef_b_cr;
    SC_S32 y_offset;
    SC_S32 cb_offset;
    SC_S32 cr_offset;
} SC_RGBYUV_COF_T;

typedef struct
{
    SC_S32 enable;
    SC_S32 zoom_par;
    SC_S32 dn_level;

    SC_S32 lut0[256];
    SC_S32 lut1[256];
} SC_ISP_SUB_MODULE_CNF_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_RGBYUV_COF_T rgbyuv_cof;

    SC_S32 lut_size;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_CNF_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_CNF_TUNING;

/* --------------------------   3DNR   -------------------------- */
typedef struct
{
    SC_S32 enale_3d_noise;

    SC_S32 nr3d_pp_en;
    SC_S32 nr3d_md_en;
    SC_S32 nr3d_npy_th1;
    SC_S32 nr3d_npy_th2;
    SC_S32 nr3d_mpy_th1;
    SC_S32 nr3d_mpy_th2;
    SC_S32 nr3d_npc_th1;
    SC_S32 nr3d_npc_th2;
    SC_S32 nr3d_npc_th3;
    SC_S32 nr3d_npc_th4;
    SC_S32 nr3d_mpc_th1;
    SC_S32 nr3d_mpc_th2;
    SC_S32 nr3d_lamda2d;
    SC_S32 nr3d_ite;
    SC_S32 nr3d_msr;
    SC_S32 nr3d_decay;
    SC_S32 nr3d_satu;
    SC_S32 nr3d_gaus_y_c11;
    SC_S32 nr3d_gaus_y_c12;
    SC_S32 nr3d_gaus_y_c13;
    SC_S32 nr3d_gaus_y_c21;
    SC_S32 nr3d_gaus_y_c23;
    SC_S32 nr3d_tauhard_th;
    SC_S32 md_blk_ave_sel;
    SC_S32 bypass_saturation_adjust;
    SC_S32 post_process_y_filter_sel;
    SC_S32 pixel_weight_filter_c11;
    SC_S32 pixel_weight_filter_c12;
    SC_S32 pixel_weight_filter_c13;
    SC_S32 disable_1st_media_filter;
    SC_S32 dbk_h_en;
    SC_S32 dbk_v_en;
    SC_S32 dbk_wr2ddr;
    SC_S32 dbk_bypass;
    SC_S32 dbk_h_mv_diff;
    SC_S32 dbk_h_mv_str;
    SC_S32 dbk_v_mv_diff;
    SC_S32 dbk_v_mv_str;
    SC_S32 dbk_h_yy_flat;
    SC_S32 dbk_h_yy_diff;
    SC_S32 dbk_v_yy_flat;
    SC_S32 dbk_v_yy_diff;
    SC_S32 dbk_satu;
    SC_S32 dbk_gaus_y_c11;
    SC_S32 dbk_gaus_y_c12;
    SC_S32 dbk_gaus_y_c13;
    SC_S32 dbk_gaus_y_c21;
    SC_S32 dbk_gaus_y_c23;
    SC_S32 bypass_dbk_sturation_adjust;

    SC_S32 md_yuv_sel;
    SC_S32 md_1st_media_filter_disable;
    SC_S32 md_noise_profile_slection;
    SC_S32 md_divs_seletction;
    SC_S32 md_expand_ration;
    SC_S32 md_sort_sel;
    SC_S32 noise_profile_y[64];
    SC_S32 noise_profile_cbcr[64];
    SC_S32 dbk_h_ed_en;
    SC_S32 dbk_v_ed_en;
    SC_S32 dbk_h_ed_sel;
    SC_S32 dbk_v_ed_sel;
    SC_S32 dbk_h_ed_thrd;
    SC_S32 dbk_v_ed_thrd;
} SC_ISP_SUB_MODULE_3D_2D_NR_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_3D_2D_NR_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_3D_2D_NR_TUNING_T;

/* --------------------------   DITHERING   -------------------------- */
#define SC_DITHER_TABLE_SIZE 256

typedef struct
{
    SC_S32 enable;

    SC_FLOAT noise_gain;

    SC_S32 light_start;
    SC_S32 light_end;

    SC_CHAR dither_talbe[SC_DITHER_TABLE_SIZE];
} SC_ISP_SUB_MODULE_DITHERING_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_DITHERING_PRA pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_DITHERING_TUNING;

/* --------------------------   LDC   -------------------------- */
typedef struct
{
} SC_ISP_SUB_MODULE_EIS_LDC_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIDDLE_TRIGGER_COUNT];

    SC_ISP_SUB_MODULE_EIS_LDC_PRA pra[SC_ISP_MIDDLE_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_EIS_LDC_TUNING_T;

/* --------------------------   SCALER   -------------------------- */
typedef struct
{
    SC_S32 w;
    SC_S32 h;

    SC_S32 avg_h;
    SC_S32 avg_v;
} SC_SCALER_ANTI_T;

typedef struct
{
    SC_SCALER_ANTI_T snr1_scaler1[16];
    SC_SCALER_ANTI_T snr1_scaler2[16];
    SC_SCALER_ANTI_T snr2_scaler1[16];
    SC_SCALER_ANTI_T snr2_scaler2[16];
} SC_ISP_SUB_MODULE_SCALER_ANTI_PRA_T;

typedef struct
{
    SC_S32 enable;

    SC_ISP_SUB_MODULE_SCALER_ANTI_PRA_T isp_sub_module_scaler_pra;
} SC_ISP_SUB_MODULE_SCALER_ANTI_TUNING_T;

/* --------------------------   VFE   -------------------------- */
typedef struct
{
    SC_CHAR net_name[128];

    SC_DOUBLE scale;
    SC_DOUBLE k_coeff[2];
    SC_DOUBLE b_coeff[3];

    SC_S32 blc[4];
} SC_AI_ISP_PRA_T;

typedef struct
{
    SC_ISP_SUB_MODULE_HDR_MIX_PRA hdr_mix_pra;

    SC_AI_ISP_PRA_T ai_isp_pra;
} SC_ISP_SUB_MODULE_ISP_VFE_PRA;

typedef struct
{
    SC_S32 enable;
    SC_S32 interpolation_enable;
    SC_S32 count_ae;
    SC_S32 trigger_mode;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MAX_TRIGGER_COUNT];
    SC_ISP_SUB_MODULE_ISP_VFE_PRA pra[SC_ISP_MAX_TRIGGER_COUNT];
} SC_ISP_SUB_MODULE_ISP_VFE_TUNING_T;

typedef struct
{
    SC_FLOAT aec_trigger_tolerence;
    SC_FLOAT aec_trigger_tolerence_gain;
    SC_FLOAT awb_trigger_tolerence;
} SC_TUNING_CTL_T;

typedef struct
{
    SC_U32 isp_version;

    SC_ISP_SUB_MODULE_CROP_TUNING_T        isp_sub_module_raw_crop_tuning;
    SC_ISP_SUB_MODULE_BLC_TUNING_T         isp_sub_module_blc_tuning;
    SC_ISP_SUB_MODULE_HDR_MIX_TUNING_T     isp_sub_module_hdr_mix_tuning;
    SC_ISP_SUB_MODULE_COMPANDER_TUNING_T   isp_sub_module_compander_tuning;
    SC_ISP_SUB_MODULE_DPC_TUNING_T         isp_sub_module_dpc_tuning;
    SC_ISP_SUB_MODULE_CAC_TUNING_T         isp_sub_module_cac_tuning;
    SC_ISP_SUB_MODULE_ATA_TUNING_T         isp_sub_module_ata_tuning;
    SC_ISP_SUB_MODULE_RNR_TUNING_T         isp_sub_module_rnr_tuning;
    SC_ISP_SUB_MODULE_DECOMPANDER_TUNING_T isp_sub_module_decompader_tuning;
    SC_ISP_SUB_MODULE_LSC_TUNING_T         isp_sub_module_lsc_tuning;
    SC_ISP_SUB_MODULE_DRC_TUNING_T         isp_sub_module_drc_tuning;
    SC_ISP_SUB_MODULE_GIC_TUNING_T         isp_sub_module_gic_tuning;
    SC_ISP_SUB_MODULE_CFA_TUNING_T         isp_sub_module_cfa_tuning;
    SC_ISP_SUB_MODULE_DEPURPLE_TUNING_PRA  isp_sub_module_depurple_tuning;
    SC_ISP_SUB_MODULE_CCM1_TUNING_T        isp_sub_module_ccm1_tuning;
    SC_ISP_SUB_MODULE_CCM2_TUNING_T        isp_sub_module_ccm2_tuning;
    SC_ISP_SUB_MODULE_GTM1_TUNING_T        isp_sub_module_gtm1_lut_tuning;
    SC_ISP_SUB_MODULE_GAMMA_LUT_TUNING_T   isp_sub_module_gamma_lut_tuning;
    SC_ISP_SUB_MODULE_GTM2_LUT_TUNING_T    isp_sub_module_gtm2_lut_tuning;
    SC_ISP_SUB_MODULE_3D_LUT_TUNING_T      isp_sub_module_3d_lut_tuning;
    SC_ISP_SUB_MODULE_RGB2YUV_TUNING_T     isp_sub_module_rgbyuv_tuning;
    SC_ISP_SUB_MODULE_CM_TUNING_T          isp_sub_module_cm_tuning;
    SC_ISP_SUB_MODULE_LEE_TUNING_T         isp_sub_module_lee_tuning;
    SC_ISP_SUB_MODULE_CNF_TUNING           isp_sub_module_cnf_tuning;
    SC_ISP_SUB_MODULE_3D_2D_NR_TUNING_T    isp_sub_module_3d_2d_nr_tuning;
    SC_ISP_SUB_MODULE_DITHERING_TUNING     isp_sub_module_dithering_tuning;
    SC_ISP_SUB_MODULE_ISP_VFE_TUNING_T     isp_sub_module_isp_vfe_tuning;
    SC_ISP_SUB_MODULE_SCALER_ANTI_TUNING_T isp_sub_module_scaler_tuning;
    SC_ISP_SUB_MODULE_EIS_LDC_TUNING_T     isp_sub_module_eis_ldc_tuning;
    SC_ISP_SUB_MODULE_CM2_TUNING_T         isp_sub_module_cm2_tuning;
    SC_ISP_SUB_MODULE_LSC_TUNING_T         isp_sub_module_hdr_lsc_tuning;
} SC_ISP_TUNING_T;

typedef struct
{
    SC_ISP_SUB_MODULE_BLC_PARA          isp_sub_module_blc_tuning;
    SC_ISP_SUB_MODULE_HDR_MIX_PRA       isp_sub_module_hdr_mix_tuning;
    SC_ISP_SUB_MODULE_COMPANDER_PRA     isp_sub_module_compander_tuning;
    SC_ISP_SUB_MODULE_DPC_PRA           isp_sub_module_dpc_tuning;
    SC_ISP_SUB_MODULE_CAC_PRA           isp_sub_module_cac_tuning;
    SC_ISP_SUB_MODULE_ATA_PRA           isp_sub_module_ata_tuning;
    SC_ISP_SUB_MODULE_RNR_PRA           isp_sub_module_rnr_tuning;
    SC_ISP_SUB_MODULE_DECOMPANDER_PRA   isp_sub_module_decompader_tuning;
    SC_ISP_SUB_MODULE_LSC_PRA           isp_sub_module_lsc_tuning;
    SC_ISP_SUB_MODULE_DRC_PRA           isp_sub_module_drc_tuning;
    SC_ISP_SUB_MODULE_GIC_PRA           isp_sub_module_gic_tuning;
    SC_ISP_SUB_MODULE_CFA_PRA           isp_sub_module_cfa_tuning;
    SC_ISP_SUB_MODULE_DEPURPLE_PRA      isp_sub_module_depurple_tuning;
    SC_ISP_SUB_MODULE_CCM1_PRA          isp_sub_module_ccm1_tuning;
    SC_ISP_SUB_MODULE_CCM2_PRA          isp_sub_module_ccm2_tuning;
    SC_ISP_SUB_MODULE_GTM1_PRA          isp_sub_module_gtm1_lut_tuning;
    SC_ISP_SUB_MODULE_GAMMA_LUT_PRA     isp_sub_module_gamma_lut_tuning;
    SC_ISP_SUB_MODULE_GTM2_LUT_PRA      isp_sub_module_gtm2_lut_tuning;
    SC_ISP_SUB_MODULE_3D_LUT_PRA        isp_sub_module_3d_lut_tuning;
    SC_ISP_SUB_MODULE_RGB2YUV_PRA       isp_sub_module_rgbyuv_tuning;
    SC_ISP_SUB_MODULE_CM_PRA            isp_sub_module_cm_tuning;
    SC_ISP_SUB_MODULE_LEE_PRA           isp_sub_module_lee_tuning;
    SC_ISP_SUB_MODULE_CNF_PRA           isp_sub_module_cnf_tuning;
    SC_ISP_SUB_MODULE_3D_2D_NR_PRA      isp_sub_module_3d_2d_nr_tuning;
    SC_ISP_SUB_MODULE_DITHERING_PRA     isp_sub_module_dithering_tuning;
    SC_AWB_STATISTICS                   awb_stats_tuning;
    SC_ISP_SUB_MODULE_ISP_VFE_PRA       isp_sub_module_isp_isp_vfe_tuning;
    SC_ISP_SUB_MODULE_SCALER_ANTI_PRA_T isp_sub_module_scaler_tuning;
    SC_AF_STATS_TUNING_T                af_stats_tuning;
    SC_ISP_SUB_MODULE_EIS_LDC_PRA       isp_sub_module_eis_ldc_tuning;
    SC_ISP_SUB_MODULE_CM2_PRA           isp_sub_module_cm2_tuning;
    SC_ISP_SUB_MODULE_LSC_PRA           isp_sub_module_hdr_lsc_tuning;
} SC_ISP_TUNING_PRA_T;

typedef struct
{
    SC_U32 enable;
} SC_RRO_CONFIG_T;

typedef struct
{
    SC_S32 enable;
} SC_RGB_MAX_CONFIG_T;

typedef struct
{
    SC_S32 enable;
} SC_RGB_HISTO_CONFIG_T;

typedef struct
{
    SC_S32 enable;
} SC_RAW_HISTO_CONFIG_T;

typedef struct
{
    SC_S32 enable;
} SC_RCS_HISTO_CONFIG_T;

typedef struct
{
    SC_S32 enable;
} SC_AWBS_HISTO_CONFIG_T;

typedef struct
{
    SC_S32 enable;
} SC_AF_HISTO_CONFIG_T;

typedef struct
{
    SC_RRO_CONFIG_T        rro_config;
    SC_RGB_MAX_CONFIG_T    rgb_max_config;
    SC_RGB_HISTO_CONFIG_T  rgb_histo_config;
    SC_RAW_HISTO_CONFIG_T  raw_histo_config;
    SC_RCS_HISTO_CONFIG_T  rcs_config;
    SC_AWBS_HISTO_CONFIG_T awbs_config;
    SC_AF_HISTO_CONFIG_T   af_config;
} SC_STATS_CONFIG_T;

typedef struct
{
    SC_U32 header_version;

    SC_TUNING_CTL_T   tuning_ctl;
    SC_ISP_TUNING_T   isp_tuning;
    SC_STATS_CONFIG_T stats_config;
    SC_AEC_TUNING_T   aec_tuning;
    SC_AWB_TUNING_T   awb_tuning;
    SC_AF_TUNING_T    af_tuning;
    SC_EIS_TUNING_T   eis_tuning;
} SC_ALGO_ISP_TUNING_T;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __SC_ISP_TUNING_DEF_H__ */
