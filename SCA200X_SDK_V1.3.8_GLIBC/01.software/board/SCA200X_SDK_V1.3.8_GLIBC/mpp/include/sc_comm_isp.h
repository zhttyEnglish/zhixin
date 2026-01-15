/**
 * @file     sc_comm_isp.h
 * @brief    ISP模块的宏、枚举和结构体类型定义
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-07-15 创建文件
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

#ifndef __SC_COMM_ISP_H__
#define __SC_COMM_ISP_H__

#include "sc_type.h"
#include "sc_errno.h"
#include "sc_common.h"
#include "sc_comm_video.h"
#include "sc_isp_tuning_def.h"
#include "sc_isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef SC_S32   VI_PIPE;
typedef SC_PVOID THREAD_ID;

/***********************************************************************************************************************
 **********          MACRO DEFINITION                                                                         **********
 **********************************************************************************************************************/
#define SC_TRACE_ISP(level, fmt, ...)\
do{\
    SC_TRACE(level, SC_ID_ISP, "[Info]:" fmt, ##__VA_ARGS__);\
}while(0)

/*
ISP Error Code
0x40 = ISP_NOT_INIT
0x41 = ISP_MEM_NOT_INIT
0x42 = ISP_ATTR_NOT_CFG
0x43 = ISP_SNS_UNREGISTER
0x44 = ISP_INVALID_ADDR
0x45 = ISP_NOMEM
0x46 = ISP_NO_INT
*/
typedef enum scISP_ERR_CODE_E
{
    ERR_ISP_NOT_INIT       = 0x40, //ISP not init
    ERR_ISP_MEM_NOT_INIT   = 0x41, //ISP memory not init
    ERR_ISP_ATTR_NOT_CFG   = 0x42, //ISP attribute not cfg
    ERR_ISP_SNS_UNREGISTER = 0x43, //ISP sensor unregister
    ERR_ISP_INVALID_ADDR   = 0x44, //ISP invalid address
    ERR_ISP_NOMEM          = 0x45, //ISP nomem
    ERR_ISP_NO_INT         = 0x46, //ISP
} ISP_ERR_CODE_E;

#define SC_ERR_ISP_NULL_PTR         SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define SC_ERR_ISP_NOTREADY         SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define SC_ERR_ISP_INVALID_DEVID    SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
#define SC_ERR_ISP_INVALID_PIPEID   SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_PIPEID)
#define SC_ERR_ISP_EXIST            SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define SC_ERR_ISP_UNEXIST          SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define SC_ERR_ISP_NOT_SUPPORT      SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define SC_ERR_ISP_NOT_PERM         SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define SC_ERR_ISP_NOMEM            SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define SC_ERR_ISP_NOBUF            SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define SC_ERR_ISP_SIZE_NOT_ENOUGH  SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_SIZE_NOT_ENOUGH)
#define SC_ERR_ISP_ILLEGAL_PARAM    SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define SC_ERR_ISP_INVALID_ADDR     SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
#define SC_ERR_ISP_BUSY             SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define SC_ERR_ISP_BUF_EMPTY        SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)

/* -----------------------   WRAPPER   ------------------------ */
#define SC_WRAPPER_MAX_DEV_NUM    VI_MAX_DEV_NUM
#define SC_WRAPPER_MAX_PIPE_NUM   VI_MAX_PIPE_NUM

#define BAYER_CALIBTAION_MAX_NUM  (50)
#define SC_ISP_BAYERNR_LUT_LENGTH (33)

/* --------------------------   AE   -------------------------- */
#define HIST_THRESH_NUM           (4)

#define AE_ZONE_ROW               (15)
#define AE_ZONE_COLUMN            (17)

#define ISP_AE_ROUTE_MAX_NODES    (16)
#define ISP_AE_ROUTE_EX_MAX_NODES (16)
#define ISP_AUTO_ISO_STRENGTH_NUM (16)

#define AI_MAX_STEP_FNO_NUM       (1024)

/* --------------------------   AF   -------------------------- */
#define AF_ZONE_ROW     (15)
#define AF_ZONE_COLUMN  (17)

#define IIR_EN_NUM      (3)
#define IIR_GAIN_NUM    (7)
#define IIR_SHIFT_NUM   (4)
#define FIR_GAIN_NUM    (5)

/* --------------------------   AWB  -------------------------- */
#define AWB_CURVE_PARA_NUM   (6)
#define AWB_LUM_HIST_NUM     (6)
#define AWB_ZONE_ORIG_ROW    (32)
#define AWB_ZONE_ORIG_COLUMN (32)
#define AWB_ZONE_BIN         (4)
#define ISP_MAX_STITCH_NUM   (4)
#define AWB_ZONE_NUM         (AWB_ZONE_ORIG_ROW * AWB_ZONE_ORIG_COLUMN * AWB_ZONE_BIN)
#define AWB_ZONE_STITCH_MAX  (AWB_ZONE_NUM * ISP_MAX_STITCH_NUM)

#define CCM_MATRIX_SIZE      (9)
#define CCM_MATRIX_NUM       (7)

#define HIST_NUM               (1024)
#define SC_STATIC_DP_COUNT_MAX (8192)

#define GAMMA_NODE_NUM       (1025)
#define PREGAMMA_NODE_NUM    (257)

#define LOG_LUT_SIZE         (1025)
#define PRE_LOG_LUT_SIZE     (1025)

#define ACC_SHIFT_H_NUM      (2)
#define ACC_SHIFT_V_NUM      (2)
#define ISP_CHN_MAX_NUM      (4)
#define BAYER_PATTERN_NUM    (4)

#define WDR_CHN_MAX          (4)
#define WDR_MAX_FRAME_NUM    (4)
#define EXP_RATIO_NUM        (4)
#define NoiseSet_EleNum      (7)

#define SHRP_GAIN_LUT_SIZE             (64)
#define ISP_SHARPEN_FREQ_CORING_LENGTH (8)
#define ISP_SHARPEN_LUMA_NUM           (32)
#define ISP_SHARPEN_GAIN_NUM           (32)

#define ISP_MAX_SNS_REGS               (32)

#define ISP_DRC_CC_NODE_NUM            (33)
#define ISP_DRC_TM_NODE_NUM            (200)

#define ISP_BAYERNR_LUT_LENGTH         (33)
#define ISP_BAYER_CHN_NUM              (4)

#define SC_ISP_LSC_GRID_COL            (33)
#define SC_ISP_LSC_GRID_ROW            (33)
#define SC_ISP_LSC_GRID_POINTS         (SC_ISP_LSC_GRID_COL * SC_ISP_LSC_GRID_ROW)

#define SC_ISP_RLSC_POINTS             (129)

#define SC_ISP_CA_YRATIO_LUT_LENGTH    (256)

/* --------------------------   DPC   -------------------------- */
#define ISP_DPC_CAL_MODE_DYNAMIC     (0)
#define ISP_DPC_CAL_MODE_STATIC      (1)
#define ISP_DPC_DEFECT_PIXEL_SINGLE  (0)
#define ISP_DPC_DEFECT_PIXEL_MAX_NUM (2000)

#define ISP_DPC_STRENGTH_MIN         (0)
#define ISP_DPC_STRENGTH_MAX         (255)
#define ISP_DPC_STRENGTH_DEFAULT     (128)

#define ISP_DPC_BLEND_RATIO_MIN      (0)
#define ISP_DPC_BLEND_RATIO_MAX      (255)
#define ISP_DPC_BLEND_RATIO_DEFAULT  (128)

#define ISP_DPC_THRESHOLD_D_DEFAULT  (60)
#define ISP_DPC_THRESHOLD_C_DEFAULT  (200)
#define ISP_DPC_THRESHOLD_D2_DEFAULT (800)

#define ISP_MIN(a, b) ((a) < (b) ? (a) : (b))

#define SC_LOCK(lock)   LOCK(lock)
#define SC_UNLOCK(lock) UNLOCK(lock)

/***********************************************************************************************************************
 **********          STRUCTURE DEFINITION                                                                     **********
 **********************************************************************************************************************/
/**
* 定义模块运行状态
*/
typedef enum scISP_OP_TYPE_E
{
    OP_TYPE_AUTO    = 0, /**< 运行在自动模式下 */
    OP_TYPE_MANUAL  = 1, /**< 运行在手动模式下 */
    OP_TYPE_BUTT         /**< 无效值 */
} ISP_OP_TYPE_E;

/*
Defines the ISP correction or detection status
0 = initial status, no calibration
1 = The static defect pixel calibration ends normally
2 = The static defect pixel calibration ends due to timeout.
*/
typedef enum scISP_STATE_E
{
    ISP_STATE_INIT     = 0,
    ISP_STATE_SUCCESS  = 1,
    ISP_STATE_TIMEOUT  = 2,
    ISP_STATE_BUTT
} ISP_STATUS_E;

/*Defines the format of the input Bayer image*/
typedef enum scISP_BAYER_FORMAT_E
{
    SC_BAYER_RGGB    = 0,
    SC_BAYER_GRBG    = 1,
    SC_BAYER_GBRG    = 2,
    SC_BAYER_BGGR    = 3,
    SC_BAYER_BUTT
} ISP_BAYER_FORMAT_E;

/*Defines the bitwidth of the input Bayer image, used for lsc online calibration*/
typedef enum scISP_BAYER_RAWBIT_E
{
    BAYER_RAWBIT_8BIT    = 8,
    BAYER_RAWBIT_10BIT   = 10,
    BAYER_RAWBIT_12BIT   = 12,
    BAYER_RAWBIT_14BIT   = 14,
    BAYER_RAWBIT_16BIT   = 16,
    BAYER_RAWBIT_BUTT
} ISP_BAYER_RAWBIT_E;

/* ISP public attribute, contains the public image attribute */
typedef struct scISP_PUB_ATTR_S
{
    RECT_S             stWndRect;    /* RW; Start position of the cropping window, image width and height */
    SIZE_S             stSnsSize;    /* RW; Width and height of the image output from the sensor */
    SC_FLOAT           f32FrameRate; /* RW; Range: [0, 0xFFFF], for frame rate */
    ISP_BAYER_FORMAT_E enBayer;      /* RW; Range: [0, 3], the format of the input Bayer image */
    WDR_MODE_E         enWDRMode;    /* RW; WDR mode select */
    SC_U8              u8SnsMode;    /* RW; 本模式传递到了SENSOR驱动中，驱动可以根据此模式选择分辨率等 */
    RECT_S             stWdrRect;    /* RW; WDR模式下的图像的CROP属性 */
    //ISP_BAYER_RAWBIT_E enBayerRawBits; /* RW; how much the bits of the sensor out */
} ISP_PUB_ATTR_S;

/*
Defines the ISP firmware status
0 = Running status
1 = Frozen status
*/
typedef enum scISP_FMW_STATE_E
{
    ISP_FMW_STATE_RUN = 0,
    ISP_FMW_STATE_FREEZE,
    ISP_FMW_STATE_BUTT
} ISP_FMW_STATE_E;

/*Defines the WDR mode of the ISP*/
typedef struct scISP_WDR_MODE_S
{
    WDR_MODE_E enWDRMode;
} ISP_WDR_MODE_S;

typedef struct scISP_INNER_STATE_INFO_S
{
    SC_S32 bResSwitchFinish;
    SC_S32 bWDRSwitchFinish;
} ISP_INNER_STATE_INFO_S;

/************AEC ***************/
/* config of statistics structs */
typedef struct scISP_AE_STATISTICS_CFG_S
{
    SC_ISP_ROI_T roi;
} ISP_AE_STATISTICS_CFG_S;

typedef struct scISP_AE_STATISTICS_S
{
    SC_AEC_ALGO_LIB_INPUT_T stAecStats;
} ISP_AE_STATISTICS_S;

/************ AWB*************/
/*Defines the AWB statistics configuration*/
typedef struct scISP_WB_STATISTICS_CFG_S
{
    SC_ISP_ROI_T roi;
} ISP_WB_STATISTICS_CFG_S;

typedef struct scISP_WB_STATISTICS_S
{
    SC_AWB_ALGO_LIB_INPUT_T stAwbStats;
} ISP_WB_STATISTICS_S;

/******************AF******************/
typedef struct scISP_FOCUS_STATISTICS_CFG_S
{
    //for af stats tuning pra update
    SC_S32 count_ae;
    SC_S32 trigger_mode;
    SC_S32 interpolation_enable;

    SC_AEC_TRIGGER_T aec_trigger[SC_ISP_MIN_TRIGGER_COUNT];
    SC_AF_STATS_TUNING_T af_stats_tuning_pra[SC_ISP_MIN_TRIGGER_COUNT];
} ISP_FOCUS_STATISTICS_CFG_S;

typedef struct scISP_AF_STATISTICS_S
{
    SC_AF_ALGO_LIB_INPUT_T stAfStats;
} ISP_AF_STATISTICS_S;

typedef struct scISP_STATISTICS_CFG_S
{
    ISP_AE_STATISTICS_CFG_S     stAECfg;
    ISP_WB_STATISTICS_CFG_S     stWBCfg;
    ISP_FOCUS_STATISTICS_CFG_S  stFocusCfg;
} ISP_STATISTICS_CFG_S;

typedef struct scISP_INIT_ATTR_S
{
    SC_U32 u32ExpTime;
    SC_U32 u32AGain;
    SC_U32 u32DGain;
    SC_U32 u32ISPDGain;
    SC_U32 u32Exposure;
    SC_U32 u32LinesPer500ms;
    SC_U32 u32PirisFNO;

    SC_U16 u16WBRgain;
    SC_U16 u16WBGgain;
    SC_U16 u16WBBgain;
    SC_U16 u16SampleRgain;
    SC_U16 u16SampleBgain;
} ISP_INIT_ATTR_S ;

typedef struct scISP_DBG_ATTR_S
{
    SC_U32  u32Rsv;         /* H;need to add member */
} ISP_DBG_ATTR_S;

typedef struct scISP_DBG_STATUS_S
{
    SC_U32  u32FrmNumBgn;
    SC_U32  u32Rsv;         /* H;need to add member */
    SC_U32  u32FrmNumEnd;
} ISP_DBG_STATUS_S;

/*
0 = Communication between the sensor and the ISP over the I2C interface
1 = Communication between the sensor and the ISP over the SSP interface
*/
typedef enum scISP_SNS_TYPE_E
{
    ISP_SNS_I2C_TYPE = 0,
    ISP_SNS_SSP_TYPE,

    ISP_SNS_TYPE_BUTT,
} ISP_SNS_TYPE_E;

/* sensor communication bus */
typedef union scISP_SNS_COMMBUS_U
{
    SC_S8 s8I2cDev;

    struct
    {
        SC_S8 bit4SspDev : 4;
        SC_S8 bit4SspCs  : 4;
    } s8SspDev;
} ISP_SNS_COMMBUS_U;

typedef struct scISP_I2C_DATA_S
{
    SC_BOOL bUpdate;            /*RW; Range: [0x0, 0x1]; Format:1.0; SC_TRUE: The sensor registers are written,SC_FALSE: The sensor registers are not written*/

    SC_U8   u8DelayFrmNum;      /*RW; Number of delayed frames for the sensor register*/
    SC_U8   u8IntPos;           /*RW;Position where the configuration of the sensor register takes effect */
    SC_U8   u8DevAddr;          /*RW;Sensor device address*/

    SC_U32  u32RegAddr;         /*RW;Sensor register address*/
    SC_U32  u32AddrByteNum;     /*RW;Bit width of the sensor register address*/
    SC_U32  u32Data;            /*RW;Sensor register data*/
    SC_U32  u32DataByteNum;     /*RW;Bit width of sensor register data*/
} ISP_I2C_DATA_S;

typedef struct scISP_SSP_DATA_S
{
    SC_BOOL bUpdate;            /*RW; Range: [0x0, 0x1]; Format:1.0; SC_TRUE: The sensor registers are written,SC_FALSE: The sensor registers are not written*/

    SC_U8   u8DelayFrmNum;      /*RW; Number of delayed frames for the sensor register*/
    SC_U8   u8IntPos;           /*RW;Position where the configuration of the sensor register takes effect */

    SC_U32  u32DevAddr;         /*RW;Sensor device address*/
    SC_U32  u32DevAddrByteNum;  /*RW;Bit width of the sensor device address*/
    SC_U32  u32RegAddr;         /*RW;Sensor register address*/
    SC_U32  u32RegAddrByteNum;  /*RW;Bit width of the sensor register address*/
    SC_U32  u32Data;            /*RW;Sensor register data*/
    SC_U32  u32DataByteNum;     /*RW;Bit width of sensor register data*/
} ISP_SSP_DATA_S;

typedef struct scISP_SNS_REGS_INFO_S
{
    ISP_SNS_TYPE_E enSnsType;

    SC_U32 u32RegNum;              /*RW;Number of registers required when exposure results are written to the sensor. The member value cannot be dynamically changed*/
    SC_U8  u8Cfg2ValidDelayMax;    /*RW;Maximum number of delayed frames from the time when all sensor registers are configured to the
                                     time when configurations take effect, which is used to ensure the synchronization between sensor registers and ISP registers*/
    ISP_SNS_COMMBUS_U  unComBus;

    union
    {
        ISP_I2C_DATA_S astI2cData[ISP_MAX_SNS_REGS];
        ISP_SSP_DATA_S astSspData[ISP_MAX_SNS_REGS];
    };

    struct
    {
        SC_BOOL bUpdate;

        SC_U8   u8DelayFrmNum;
        SC_U32  u32SlaveVsTime;      /* RW;time of vsync. Unit: inck clock cycle */
        SC_U32  u32SlaveBindDev;
    } stSlvSync;

    SC_BOOL bConfig;
} ISP_SNS_REGS_INFO_S;

typedef enum scISP_VD_TYPE_E
{
    ISP_VD_FE_START = 0,  // isp vsync
    ISP_VD_FE_END,        // isp done
    ISP_VD_BE_END,        // not supported

    ISP_VD_BUTT
} ISP_VD_TYPE_E;

/**
* 定义ISP FSWDR运行模式
*/
typedef enum scISP_FSWDR_MODE_E
{
    ISP_FSWDR_NORMAL_MODE          = 0x0,
    ISP_FSWDR_LONG_FRAME_MODE      = 0x1,
    ISP_FSWDR_AUTO_LONG_FRAME_MODE = 0x2,
    ISP_FSWDR_MODE_BUTT
} ISP_FSWDR_MODE_E;

/**
* 定义ISP曝光属性
*/
typedef struct scISP_EXPOSURE_ATTR_S
{
    SC_BOOL bEnable;
    SC_U8   bManual;

    SC_AEC_SET_INFO_T stManualExposure;
    SC_AEC_TUNING_T   stAutoExposure;
} ISP_EXPOSURE_ATTR_S;

/**
* 定义ISP曝光信息
*/
typedef struct scISP_EXP_INFO_S
{
    SC_AEC_GET_INFO_T stAecInfo;
} ISP_EXP_INFO_S;

/**
* 定义Face曝光信息
*/
typedef struct
{
    SC_FLOAT x;         /* X:人脸起始坐标在预览画面百分比的位置 */
    SC_FLOAT y;         /* Y:人脸起始坐标在预览画面百分比的位置 */
    SC_FLOAT width;     /* 人脸宽和实际预览画面的比值 */
    SC_FLOAT height;    /* 人脸高和实际预览画面的比值 */
} ISP_ROI_T;

typedef struct scSTRU_FACE_AEC_S
{
    SC_S32 face_count;
    ISP_ROI_T face_roi[16];         /* max 16 face */
    SC_FLOAT face_weight;
    SC_S32  face_keep_frame_count;  /* 该值应大于循环sleep时间，否则画面容易闪烁 */
    SC_S32  face_target;
} ISP_FACE_AEC_S;

/**
* 定义ISP白平衡属性
*/
typedef struct scISP_WB_ATTR_S
{
    SC_BOOL bEnable;
    SC_U8   bManual;

    SC_AWB_SET_INFO_T stManualWb;
    SC_AWB_TUNING_T   stAutoWb;
} ISP_WB_ATTR_S;

/**
* 定义ISP白平衡信息
*/
typedef struct scISP_WB_INFO_S
{
    SC_AWB_GET_INFO_T stAwbInfo;
} ISP_WB_INFO_S;

/**
* 定义ISP对焦属性
*/
typedef struct scISP_FOCUS_ATTR_S
{
    SC_BOOL bEnable;
    SC_U8   bManual;

    SC_AF_SET_INFO_T stManualFocus;
    SC_AF_TUNING_T   stAutoFocus;
} ISP_FOCUS_ATTR_S;

/**
* 定义ISP对焦信息
*/
typedef struct scISP_FOCUS_INFO_S
{
    SC_AF_GET_INFO_T stAfInfo;
} ISP_FOCUS_INFO_S;

/*
DNG cfalayout type
1 = Rectangular (or square) layout
2 = Staggered layout A: even columns are offset down by 1/2 row
3 = Staggered layout B: even columns are offset up by 1/2 row
4 = Staggered layout C: even rows are offset right by 1/2 column
5 = Staggered layout D: even rows are offset left by 1/2 column
6 = Staggered layout E: even rows are offset up by 1/2 row, even columns are offset left by 1/2 column
7 = Staggered layout F: even rows are offset up by 1/2 row, even columns are offset right by 1/2 column
8 = Staggered layout G: even rows are offset down by 1/2 row, even columns are offset left by 1/2 column
9 = Staggered layout H: even rows are offset down by 1/2 row, even columns are offset right by 1/2 column
*/
typedef enum scDNG_CFALAYOUT_TYPE_E
{
    CFALAYOUT_TYPE_RECTANGULAR = 1,
    CFALAYOUT_TYPE_A,        /*a,b,c... not support*/
    CFALAYOUT_TYPE_B,
    CFALAYOUT_TYPE_C,
    CFALAYOUT_TYPE_D,
    CFALAYOUT_TYPE_E,
    CFALAYOUT_TYPE_F,
    CFALAYOUT_TYPE_G,
    CFALAYOUT_TYPE_H,
    CFALAYOUT_TYPE_BUTT
} DNG_CFALAYOUT_TYPE_E;

typedef struct scDNG_SRATIONAL_S
{
    SC_S32 s32Numerator;   /*represents the numerator of a fraction,*/
    SC_S32 s32Denominator; /* the denominator. */
} DNG_SRATIONAL_S;

typedef struct scDNG_BLCREPEATDIM_S
{
    SC_U16 u16BlcRepeatRows;
    SC_U16 u16BlcRepeatCols;
} DNG_BLCREPEATDIM_S;

typedef struct scDNG_DEFAULTSCALE_S
{
    DNG_RATIONAL_S stDefaultScaleH;
    DNG_RATIONAL_S stDefaultScaleV;
} DNG_DEFAULTSCALE_S;

typedef struct scDNG_REPEATPATTERNDIM_S
{
    SC_U16 u16RepeatPatternDimRows;
    SC_U16 u16RepeatPatternDimCols;
} DNG_REPEATPATTERNDIM_S;

/*
Defines the structure of dng raw format.
*/
typedef struct scDNG_RAW_FORMAT_S
{
    SC_U8 u8BitsPerSample;                        /* RO;Format:8.0; Indicate the bit numbers of raw data*/
    SC_U8 au8CfaPlaneColor[CFACOLORPLANE];        /* RO;Format:8.0; Indicate the planer numbers of raw data; 0:red 1:green 2: blue*/

    DNG_CFALAYOUT_TYPE_E enCfaLayout;             /* RO;Range:[1,9]; Describes the spatial layout of the CFA*/
    DNG_BLCREPEATDIM_S stBlcRepeatDim;            /* Specifies repeat pattern size for the BlackLevel*/

    SC_U32 u32WhiteLevel;                         /* RO;Format:32.0; Indicate the WhiteLevel of the raw data*/

    DNG_DEFAULTSCALE_S
    stDefaultScale;        /* Specifies the default scale factors for each direction to convert the image to square pixels*/
    DNG_REPEATPATTERNDIM_S stCfaRepeatPatternDim; /* Specifies the pixel number of repeat color planer in each direction*/

    SC_U8 au8CfaPattern[ISP_BAYER_CHN];           /* RO;Format:8.0; Indicate the bayer start order; 0:red 1:green 2: blue*/
} DNG_RAW_FORMAT_S;

/*
Defines the structure of DNG WB gain used for calculate DNG colormatrix.
*/
typedef struct scISP_DNG_WBGAIN_S
{
    SC_U16 u16Rgain; /* RW;Range: [0x0, 0xFFF]; Multiplier for R  color channel*/
    SC_U16 u16Ggain; /* RW;Range: [0x0, 0xFFF]; Multiplier for G  color channel*/
    SC_U16 u16Bgain; /* RW;Range: [0x0, 0xFFF]; Multiplier for B  color channel*/
} ISP_DNG_WBGAIN_S;

typedef struct scISP_GAMMA_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_GAMMA_LUT_TUNING_T stAutoGamma;
    SC_ISP_SUB_MODULE_GAMMA_LUT_PRA      stManualGamma;
} ISP_GAMMA_ATTR_S;

typedef struct scISP_WDR_FS_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_HDR_MIX_TUNING_T stAutoWdrFs;
    SC_ISP_SUB_MODULE_HDR_MIX_PRA      stManualWdrFs;
} ISP_WDR_FS_ATTR_S;

typedef struct scISP_DRC_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_DRC_TUNING_T stAutoDrc;
    SC_ISP_SUB_MODULE_DRC_PRA      stManualDrc;
} ISP_DRC_ATTR_S;

typedef struct scISP_DEHAZE_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_GTM2_LUT_TUNING_T stAutoDehaze;
    SC_ISP_SUB_MODULE_GTM2_LUT_PRA      stManualDehaze;
} ISP_DEHAZE_ATTR_S;

typedef struct scISP_LDCI_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_GTM2_LUT_TUNING_T stAutoLDCI;
    SC_ISP_SUB_MODULE_GTM2_LUT_PRA      stManualLDCI;
} ISP_LDCI_ATTR_S;

typedef struct scISP_DP_DYNAMIC_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_DPC_TUNING_T stAutoDpDynamic;
    SC_ISP_SUB_MODULE_DPC_PRA      stManualDpDynamic;
} ISP_DP_DYNAMIC_ATTR_S;

typedef struct scISP_SHADING_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_LSC_TUNING_T stAutoShading;
    SC_ISP_SUB_MODULE_LSC_PRA      stManualShading;
} ISP_SHADING_ATTR_S;

typedef struct scISP_HDR_SHADING_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_LSC_TUNING_T stAutoHdrShading;
    SC_ISP_SUB_MODULE_LSC_PRA      stManualHdrShading;
} ISP_HDR_SHADING_ATTR_S;

typedef struct scISP_NR_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_RNR_TUNING_T stAutoNr;
    SC_ISP_SUB_MODULE_RNR_PRA      stManualNr;
} ISP_NR_ATTR_S;

typedef struct scISP_CAC_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_CM2_TUNING_T stAutoCAC;
    SC_ISP_SUB_MODULE_CM2_PRA      stManualCAC;
} ISP_CAC_ATTR_S;

typedef struct scISP_SHARPEN_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_LEE_TUNING_T stAutoSharpen;
    SC_ISP_SUB_MODULE_LEE_PRA      stManualSharpen;
} ISP_SHARPEN_ATTR_S;

typedef struct scISP_CA_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_CM_TUNING_T stAutoCA;
    SC_ISP_SUB_MODULE_CM_PRA      stManualCA;
} ISP_CA_ATTR_S;

typedef struct scISP_DEMOSAIC_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_CFA_TUNING_T stAutoDemosaic;
    SC_ISP_SUB_MODULE_CFA_PRA      stManualDemosaic;
} ISP_DEMOSAIC_ATTR_S;

typedef struct scISP_BLACK_LEVEL_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_BLC_TUNING_T stAutoBlackLevel;
    SC_ISP_SUB_MODULE_BLC_PARA     stManualBlackLevel;
} ISP_BLACK_LEVEL_S;

typedef struct scISP_CLUT_LUT_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_3D_LUT_TUNING_T stAutoClutLUT;
    SC_ISP_SUB_MODULE_3D_LUT_PRA      stManualClutLUT;
} ISP_CLUT_LUT_S;

typedef struct scISP_CSC_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_RGB2YUV_TUNING_T stAutoCsc;
    SC_ISP_SUB_MODULE_RGB2YUV_PRA      stManualCsc;
} ISP_CSC_ATTR_S;

typedef struct scDNG_IMAGE_STATIC_INFO_S
{
} DNG_IMAGE_STATIC_INFO_S;

typedef struct scISP_DNG_COLORPARAM_S
{
} ISP_DNG_COLORPARAM_S;

typedef struct scISP_COLORMATRIX_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_CCM1_TUNING_T stAutoCCM;
    SC_ISP_SUB_MODULE_CCM1_PRA      stManualCCM;
} ISP_COLORMATRIX_ATTR_S;

typedef struct scSP_CNF_ATTR_S
{
    SC_BOOL bEnable;
    SC_BOOL bManual;

    SC_ISP_SUB_MODULE_CNF_TUNING stAutoCNF;
    SC_ISP_SUB_MODULE_CNF_PRA    stManualCNF;
} ISP_CNF_ATTR_S;

/**
* 定义ISP 3DNR属性
*/
typedef struct scISP_PIPE_NRX_PARAM_S
{
    SC_BOOL   bEnable;
    SC_BOOL   bManual;

    SC_ISP_SUB_MODULE_3D_2D_NR_TUNING_T stAutoNrXParam;
    SC_ISP_SUB_MODULE_3D_2D_NR_PRA      stManualNrXParam;
} ISP_PIPE_NRX_PARAM_S;

typedef struct scSP_NOTIFY_FUNC_S
{
    SC_VOID (*vsync_update)(VI_PIPE ViPipe);
    SC_VOID (*aec_update)(VI_PIPE ViPipe, SC_AEC_OUT_T *aec_out);
} ISP_NOTIFY_FUNC_S;

typedef struct scSP_NOTIFY_REGISTER_S
{
    ISP_NOTIFY_FUNC_S stNotify;
} ISP_NOTIFY_REGISTER_S;

typedef union scISP_MODULE_CTRL_U
{
    SC_BOOL bitBypassAWBStat;
} ISP_MODULE_CTRL_U;

typedef struct scISP_CTRL_PARAM_S
{
} ISP_CTRL_PARAM_S;

typedef union scISP_WDR_EXPOSURE_ATTR_S
{
} ISP_WDR_EXPOSURE_ATTR_S;

typedef struct scISP_AE_ROUTE_S
{
} ISP_AE_ROUTE_S;

typedef struct scISP_BRIGHTNESS_SIMPLE_ATTR_S
{
    SC_U32 u32Brightness;
} ISP_BRIGHTNESS_SIMPLE_ATTR_S;

typedef struct scISP_SHARPNESS_SIMPLE_ATTR_S
{
    SC_U32 u32Sharpness;
} ISP_SHARPNESS_SIMPLE_ATTR_S;

typedef struct scISP_HUE_SIMPLE_ATTR_S
{
    SC_U32 u32Hue;
} ISP_HUE_SIMPLE_ATTR_S;

typedef struct scISP_CONTRAST_SIMPLE_ATTR_S
{
    SC_U32 u32Contrast;
} ISP_CONTRAST_SIMPLE_ATTR_S;

typedef struct scISP_SATURATION_ATTR_S
{
    SC_U32 u32Saturation;
} ISP_SATURATION_ATTR_S;

typedef struct scISP_EXP_LIMIT_SIMPLE_ATTR_S
{
    SC_U32 u32ExpLimit;
} ISP_EXP_LIMIT_SIMPLE_ATTR_S;

typedef struct scISP_ANTIFLICKER_SIMPLE_ATTR_S
{
    SC_U32 u32Frequency;    /* 0:关闭; 1：50HZ 2：60HZ 过曝优先; 3：50HZ 4：60HZ 抗闪优先*/
} ISP_ANTIFLICKER_SIMPLE_ATTR_S;

typedef struct scISP_SCENE_SIMPLE_ATTR_S
{
    SC_CHAR acTuningName[128];
} ISP_SCENE_SIMPLE_ATTR_S;

typedef struct scISP_AWB_SIMPLE_ATTR_S
{
    SC_U32 u32AwbMode;
} ISP_AWB_SIMPLE_ATTR_S;

typedef struct scISP_DEFOG_SIMPLE_ATTR_S
{
    SC_U32 u32Strength;
} ISP_DEFOG_SIMPLE_ATTR_S;

typedef struct scISP_NR2D_SIMPLE_ATTR_S
{
    SC_U32 u32Strength;
} ISP_NR2D_SIMPLE_ATTR_S;

typedef struct scISP_NR3D_SIMPLE_ATTR_S
{
    SC_U32 u32Strength;
} ISP_NR3D_SIMPLE_ATTR_S;

typedef struct scISP_DRC_SIMPLE_ATTR_S
{
    SC_U32 u32Strength;
} ISP_DRC_SIMPLE_ATTR_S;

typedef struct scISP_DP_STATIC_CALIBRATE_S
{
} ISP_DP_STATIC_CALIBRATE_S;

typedef struct scISP_DP_STATIC_ATTR_S
{
} ISP_DP_STATIC_ATTR_S;

typedef struct scISP_CR_ATTR_S
{
} ISP_CR_ATTR_S;

typedef struct scISP_DEBUG_INFO_S
{
} ISP_DEBUG_INFO_S;

typedef struct scISP_SLAVE_SNS_SYNC_S
{
} ISP_SLAVE_SNS_SYNC_S;

typedef struct scISP_AWB_Calibration_Gain_S
{
} ISP_AWB_Calibration_Gain_S;

typedef struct scISP_MOD_PARAM_S
{
} ISP_MOD_PARAM_S;

typedef struct scISP_DIS_ATTR_S
{
} ISP_DIS_ATTR_S;

typedef struct scISP_SHADING_LUT_ATTR_S
{
} ISP_SHADING_LUT_ATTR_S;

typedef struct scISP_RADIAL_SHADING_ATTR_S
{
} ISP_RADIAL_SHADING_ATTR_S;

typedef struct scISP_RADIAL_SHADING_LUT_ATTR_S
{
} ISP_RADIAL_SHADING_LUT_ATTR_S;

typedef struct scISP_MLSC_CALIBRATION_CFG_S
{
} ISP_MLSC_CALIBRATION_CFG_S;

typedef struct scISP_MESH_SHADING_TABLE_S
{
} ISP_MESH_SHADING_TABLE_S;

typedef struct scISP_DE_ATTR_S
{
} ISP_DE_ATTR_S;

typedef struct scISP_COLOR_TONE_ATTR_S
{
} ISP_COLOR_TONE_ATTR_S;

typedef struct scISP_PRELOGLUT_ATTR_S
{
} ISP_PRELOGLUT_ATTR_S;

typedef struct scISP_LOGLUT_ATTR_S
{
} ISP_LOGLUT_ATTR_S;

typedef struct scISP_LOCAL_CAC_ATTR_S
{
} ISP_LOCAL_CAC_ATTR_S;

typedef struct scISP_GLOBAL_CAC_ATTR_S
{
} ISP_GLOBAL_CAC_ATTR_S;

typedef struct scISP_RC_ATTR_S
{
} ISP_RC_ATTR_S;

typedef struct scISP_EDGEMARK_ATTR_S
{
} ISP_EDGEMARK_ATTR_S;

typedef struct scISP_HLC_ATTR_S
{
} ISP_HLC_ATTR_S;

typedef struct scISP_ANTIFALSECOLOR_ATTR_S
{
} ISP_ANTIFALSECOLOR_ATTR_S;

typedef struct scISP_CLUT_ATTR_S
{
} ISP_CLUT_ATTR_S;

typedef struct scISP_FPN_CALIBRATE_ATTR_S
{
} ISP_FPN_CALIBRATE_ATTR_S;

typedef struct scISP_FPN_ATTR_S
{
} ISP_FPN_ATTR_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __SC_COMM_ISP_H__ */
