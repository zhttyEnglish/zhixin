/**
 * @file     sc_comm_sns.h
 * @brief    SENSOR模块的宏、枚举和结构体类型定义
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

#ifndef __SC_COMM_SNS_H__
#define __SC_COMM_SNS_H__

#include "sc_type.h"
#include "sc_common.h"
#include "sc_comm_isp.h"
#include "sc_mipi.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define WDR_MAX_FRAME           (4)
#define TUNING_BIN_FILENAME_LEN (128)
#define SC_EVENT_DATA_SIZE_MAX  (256)
#define SC_EVENT_CODE_EXIT      (-1)

typedef struct scGpio_S
{
    SC_U32 group;
    SC_U32 port;
    SC_U32 num;
} SC_GPIO_S;

typedef struct scSensorCtlGpio_S
{
    SC_GPIO_S powerGpio;
    SC_GPIO_S resetGpio;
} SC_SENSOR_CTL_GPIO_S;

typedef struct scTuningBinFile_S
{
    SC_CHAR acLinearBinName[TUNING_BIN_FILENAME_LEN];
    SC_CHAR acWdrBinName[TUNING_BIN_FILENAME_LEN];
} SC_TUNING_BIN_FILE_S;

typedef struct
{
    SC_U32 line_length;
    SC_U32 frame_length;
    SC_U32 frame_length_short;
    SC_U32 frame_length_short1;
    SC_U32 vc_mask;

    SC_S32 vc_count;
    SC_S32 long_offset;
    SC_S32 mid_offset;
    SC_S32 short_offset;

    SC_CHAR tuning_name[128];
} SC_SNS_VAR_INFO_S;

typedef struct scSENSOR_STATE_S
{
    SC_U8 u8Hcg;

    SC_U32 u32BRL;
    SC_U32 u32RHS1_MAX;
    SC_U32 u32RHS2_MAX;
} SENSOR_STATE_S;

typedef struct scISP_SNS_ATTR_INFO_S
{
    SENSOR_ID eSensorId;
} ISP_SNS_ATTR_INFO_S;

typedef struct scISP_CMOS_SENSOR_MAX_RESOLUTION_S
{
    SC_U32 u32MaxWidth;
    SC_U32 u32MaxHeight;
} ISP_CMOS_SENSOR_MAX_RESOLUTION_S;

typedef struct scISP_CMOS_SENSOR_MODE_S
{
    SC_U32 u32SensorID;

    SC_U8 u8SensorMode;

    SC_BOOL bValidDngRawFormat;

    DNG_RAW_FORMAT_S stDngRawFormat;
} ISP_CMOS_SENSOR_MODE_S;

typedef struct scISP_CMOS_DNG_COLORPARAM_S
{
    ISP_DNG_WBGAIN_S stWbGain1;
    ISP_DNG_WBGAIN_S stWbGain2;
} ISP_CMOS_DNG_COLORPARAM_S;

typedef struct scISP_CMOS_WDR_SWITCH_ATTR_S
{
    SC_U32 au32ExpRatio[EXP_RATIO_NUM];

    SC_U32 au32ShortOffset;
    SC_U32 au32MidOffset;
    SC_U32 au32LongOffset;

    SC_U32 au32VcCnt;
    SC_U32 au32VcMask;
} ISP_CMOS_WDR_SWITCH_ATTR_S;

typedef struct scISP_CMOS_DEFAULT_S
{
    ISP_CMOS_SENSOR_MAX_RESOLUTION_S stSensorMaxResolution;
    ISP_CMOS_SENSOR_MODE_S           stSensorMode;
    ISP_CMOS_DNG_COLORPARAM_S        stDngColorParam;
    ISP_CMOS_WDR_SWITCH_ATTR_S       stWdrSwitchAttr;

    SC_CHAR acTuningPraBinName[TUNING_BIN_FILENAME_LEN];
} ISP_CMOS_DEFAULT_S;

typedef struct scISP_CMOS_SENSOR_IMAGE_MODE_S
{
    SC_U16 u16Width;
    SC_U16 u16Height;

    SC_FLOAT f32Fps;

    SC_U8 u8SnsMode;
} ISP_CMOS_SENSOR_IMAGE_MODE_S;

typedef struct scISP_CMOS_SENSOR_CTL_S
{
    SC_U8 u8CtlCode;

    SC_VOID *pCtlData;
} ISP_CMOS_SENSOR_CTL;

typedef enum
{
    SNS_CB_EVENT_POWER_ON,
    SNS_CB_EVENT_POWER_OFF,
    SNS_CB_EVENT_SENSOR_INIT,
    SNS_CB_EVENT_CFG_RES,
    SNS_CB_EVENT_STREAM_ON,
    SNS_CB_EVENT_STREAM_OFF,
    SNS_CB_EVENT_AEC_UPDATE,
    SNS_CB_EVENT_FLIP_MIRROR,
    SNS_CB_EVENT_CTL_SET_LED,
    SNS_CB_EVENT_CTL_SET_IR_CUTTER,
    SNS_CB_EVENT_CTL_SET_HDR,
    SNS_CB_EVENT_CTL_ERR_PROC,
    SNS_CB_EVENT_VSYNC,
    SNS_CB_EVENT_CLT,
    SNS_CB_EVENT_PRIV_CLT,
    SNS_CB_EVENT_TRIGGER_ON,
    SNS_CB_EVENT_ISP_DONE,
    SNS_CB_EVENT_CLT_SET_FPS_RANGE,
    SNS_CB_EVENT_ACTUATOR_UPDATE,
    SNS_CB_EVENT_MAX,
} SNS_CB_EVENT_E;

typedef struct
{
    SC_U32 fps_min;
    SC_U32 fps_max;
    SC_U32 fps_min_line_count;
    SC_U32 fps_max_line_count;
} SNS_FPS_RANGE_S;

typedef struct
{
    SC_FLOAT gain;

    SC_U32 exp_time_us;
    SC_U32 line_count;

    SC_FLOAT exp_ration_time[4];
    SC_FLOAT exp_ration_gain[4];

    SC_S32 need_flash;
} SNS_AEC_UPDATE_S;

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

    SC_FLOAT current_luma;
    SC_FLOAT ration_gain[4];
    SC_FLOAT ration_time[4];
    SC_FLOAT total_raion[4];
    SC_FLOAT drc_gain;
    SC_FLOAT drc_gain_1;
    SC_FLOAT drc_gain_2;
    SC_FLOAT fps;
} SNS_AEC_OUT_S;

typedef struct
{
    SC_S32 camera_id;

    SNS_AEC_UPDATE_S update;
    SNS_AEC_OUT_S    aec_out;
} SNS_AEC_UPDATE_PARA_S;

typedef struct
{
    SC_S32 camera_id;
    SC_S32 flip_mirror;
} SNS_FLIP_MIRROR_PARA_S;

typedef struct
{
    SC_S32 hdr_mode;
} SNS_HDR_MODE_PARA_S;

typedef union
{
    SNS_AEC_UPDATE_PARA_S  sns_update_para;
    SNS_FLIP_MIRROR_PARA_S sns_flip_mirror;
    SNS_HDR_MODE_PARA_S    sns_hdr_mode;
    SNS_FPS_RANGE_S        sns_fps_range;

    SC_CHAR event_data[SC_EVENT_DATA_SIZE_MAX];
} SNS_EVENT_DATA_U;

typedef struct scISP_SENSOR_EXP_FUNC_S
{
    SC_S32(*pfn_cmos_sensor_init)(VI_PIPE ViPipe);
    SC_VOID(*pfn_cmos_sensor_exit)(VI_PIPE ViPipe);
    SC_VOID(*pfn_cmos_sensor_global_init)(VI_PIPE ViPipe);

    SC_S32(*pfn_cmos_set_image_mode)(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode);
    SC_S32(*pfn_cmos_set_wdr_mode)(VI_PIPE ViPipe, SC_U8 u8Mode);

    SC_S32(*pfn_cmos_get_isp_default)(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef);
    SC_S32(*pfn_cmos_get_sns_reg_info)(VI_PIPE ViPipe, ISP_SNS_REGS_INFO_S *pstSnsRegsInfo);

    SC_VOID(*pfn_cmos_set_pixel_detect)(VI_PIPE ViPipe, SC_BOOL bEnable);

    SC_S32(*pfn_cmos_sns_power_on)(VI_PIPE ViPipe, dev_power_attr_t *p_dev_power_attr);
    SC_S32(*pfn_cmos_sns_power_off)(VI_PIPE ViPipe, dev_power_attr_t *p_dev_power_attr);
    SC_S32(*pfn_cmos_sns_ctl)(VI_PIPE ViPipe, ISP_CMOS_SENSOR_CTL *pSensorCtl);
} ISP_SENSOR_EXP_FUNC_S;

typedef struct scISP_SENSOR_REGISTER_S
{
    ISP_SENSOR_EXP_FUNC_S stSnsExp;
} ISP_SENSOR_REGISTER_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /*__SC_COMM_SNS_H__ */
