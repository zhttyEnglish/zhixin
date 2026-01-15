/**
 * @file     sc_comm_vi.h
 * @brief    vi 模块定义
 * @version  1.0.0
 * @since    1.0.0
 * @author  石丽月<shiliyue@sgitg.sgcc.com.cn>
 * @date    2021-07-22 创建文件
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

#ifndef __SC_COMM_VI_H__
#define __SC_COMM_VI_H__

#include "sc_common.h"
#include "sc_errno.h"
#include "sc_comm_video.h"
#include "sc_comm_gdc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define VI_MAX_ADCHN_NUM              (4UL)

#define VI_PMFCOEF_NUM                (9UL)
#define VI_COMPMASK_NUM               (2UL)
#define VI_PRO_MAX_FRAME_NUM          (8UL)

#define VI_INVALID_FRMRATE            (-1)
#define VI_CHN0                       0
#define VI_CHN1                       1
#define VI_CHN2                       2
#define VI_CHN3                       3
#define VI_INVALID_CHN                (-1)

#define VI_MAX_VC_NUM                 4

typedef struct scVI_LOW_DELAY_INFO_S
{
    SC_BOOL bEnable;          /* RW; Low delay enable. */
    SC_U32 u32LineCnt;        /* RW; Range: [32, 16384]; Low delay shoreline. */
} VI_LOW_DELAY_INFO_S;

/* Information of raw data cmpresss param */
typedef struct scVI_CMP_PARAM_S
{
    SC_U8 au8CmpParam[VI_CMP_PARAM_SIZE];
} VI_CMP_PARAM_S;

typedef enum sc_VI_USERPIC_MODE_E
{
    VI_USERPIC_MODE_PIC = 0,            /* YUV picture */
    VI_USERPIC_MODE_BGC,                /* Background picture only with a color */
    VI_USERPIC_MODE_BUTT,
} VI_USERPIC_MODE_E;

typedef struct scVI_USERPIC_BGC_S
{
    SC_U32 u32BgColor;
} VI_USERPIC_BGC_S;

typedef struct scVI_USERPIC_ATTR_S
{
    VI_USERPIC_MODE_E       enUsrPicMode;  /* User picture mode */
    union
    {
        VIDEO_FRAME_INFO_S  stUsrPicFrm;   /* Information about a YUV picture */
        VI_USERPIC_BGC_S    stUsrPicBg;    /* Information about a background picture only with a color */
    } unUsrPic;
} VI_USERPIC_ATTR_S;

typedef enum scEN_VI_ERR_CODE_E
{
    ERR_VI_FAILED_NOTENABLE = 64,       /* device or channel not enable */
    ERR_VI_FAILED_NOTDISABLE,           /* device not disable */
    ERR_VI_FAILED_CHNOTDISABLE,         /* channel not disable */
    ERR_VI_CFG_TIMEOUT,                 /* config timeout */
    ERR_VI_NORM_UNMATCH,                /* video norm of ADC and VIU is unmatch */
    ERR_VI_INVALID_WAYID,               /* invlalid way ID */
    ERR_VI_INVALID_PHYCHNID,            /* invalid phychn id */
    ERR_VI_FAILED_NOTBIND,              /* device or channel not bind */
    ERR_VI_FAILED_BINDED,               /* device or channel not unbind */
    ERR_VI_DIS_PROCESS_FAIL             /* dis process failed */
} EN_VI_ERR_CODE_E;

#define SC_ERR_VI_INVALID_PARA        SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define SC_ERR_VI_INVALID_DEVID       SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
#define SC_ERR_VI_INVALID_PIPEID      SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_PIPEID)
#define SC_ERR_VI_INVALID_STITCHGRPID SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_STITCHGRPID)
#define SC_ERR_VI_INVALID_CHNID       SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define SC_ERR_VI_INVALID_NULL_PTR    SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define SC_ERR_VI_FAILED_NOTCONFIG    SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
#define SC_ERR_VI_SYS_NOTREADY        SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define SC_ERR_VI_BUF_EMPTY           SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define SC_ERR_VI_BUF_FULL            SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define SC_ERR_VI_NOMEM               SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define SC_ERR_VI_NOT_SUPPORT         SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define SC_ERR_VI_BUSY                SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define SC_ERR_VI_NOT_PERM            SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)

#define SC_ERR_VI_FAILED_NOTENABLE    SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_FAILED_NOTENABLE)
#define SC_ERR_VI_FAILED_NOTDISABLE   SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_FAILED_NOTDISABLE)
#define SC_ERR_VI_FAILED_CHNOTDISABLE SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_FAILED_CHNOTDISABLE)
#define SC_ERR_VI_CFG_TIMEOUT         SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_CFG_TIMEOUT)
#define SC_ERR_VI_NORM_UNMATCH        SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_NORM_UNMATCH)
#define SC_ERR_VI_INVALID_WAYID       SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_INVALID_WAYID)
#define SC_ERR_VI_INVALID_PHYCHNID    SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_INVALID_PHYCHNID)
#define SC_ERR_VI_FAILED_NOTBIND      SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_FAILED_NOTBIND)
#define SC_ERR_VI_FAILED_BINDED       SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, ERR_VI_FAILED_BINDED)

#define SC_ERR_VI_PIPE_EXIST          SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define SC_ERR_VI_PIPE_UNEXIST        SC_DEF_ERR(SC_ID_VI, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)

/* interface mode of video input */
typedef enum scVI_INTF_MODE_E
{
    VI_MODE_BT656 = 0,              /* ITU-R BT.656 YUV4:2:2 */
    VI_MODE_BT656_PACKED_YUV,       /* ITU-R BT.656 packed YUV4:2:2 */
    VI_MODE_BT601,                  /* ITU-R BT.601 YUV4:2:2 */
    VI_MODE_DIGITAL_CAMERA,         /* digatal camera mode */
    VI_MODE_BT1120_STANDARD,        /* BT.1120 progressive mode */
    VI_MODE_BT1120_INTERLEAVED,     /* BT.1120 interstage mode */
    VI_MODE_MIPI,                   /* MIPI RAW mode */
    VI_MODE_MIPI_YUV420_NORMAL,     /* MIPI YUV420 normal mode */
    VI_MODE_MIPI_YUV420_LEGACY,     /* MIPI YUV420 legacy mode */
    VI_MODE_MIPI_YUV422,            /* MIPI YUV422 mode */
    VI_MODE_LVDS,                   /* LVDS mode */
    VI_MODE_HISPI,                  /* HiSPi mode */
    VI_MODE_SLVS,                   /* SLVS mode */

    VI_MODE_BUTT
} VI_INTF_MODE_E;

/* Input mode */
typedef enum scVI_INPUT_MODE_E
{
    VI_INPUT_MODE_BT656 = 0,        /* ITU-R BT.656 YUV4:2:2 */
    VI_INPUT_MODE_BT601,            /* ITU-R BT.601 YUV4:2:2 */
    VI_INPUT_MODE_DIGITAL_CAMERA,   /* digatal camera mode */
    VI_INPUT_MODE_INTERLEAVED,      /* interstage mode */
    VI_INPUT_MODE_MIPI,             /* MIPI mode */
    VI_INPUT_MODE_LVDS,             /* LVDS mode */
    VI_INPUT_MODE_HISPI,            /* HiSPi mode */
    VI_INPUT_MODE_SLVS,             /* SLVS mode */

    VI_INPUT_MODE_BUTT
} VI_INPUT_MODE_E;

/* Work mode */
typedef enum scVI_WORK_MODE_E
{
    VI_WORK_MODE_1Multiplex = 0,    /* 1 Multiplex mode */
    VI_WORK_MODE_2Multiplex,        /* 2 Multiplex mode */
    VI_WORK_MODE_3Multiplex,        /* 3 Multiplex mode */
    VI_WORK_MODE_4Multiplex,        /* 4 Multiplex mode */

    VI_WORK_MODE_BUTT
} VI_WORK_MODE_E;

/* whether an input picture is interlaced or progressive */
typedef enum scVI_SCAN_MODE_E
{
    VI_SCAN_INTERLACED  = 0,        /* interlaced mode */
    VI_SCAN_PROGRESSIVE,            /* progressive mode */

    VI_SCAN_BUTT
} VI_SCAN_MODE_E;

/* Sequence of YUV data */
typedef enum scVI_YUV_DATA_SEQ_E
{
    VI_DATA_SEQ_VUVU = 0,   /* The input sequence of the second component(only contains u and v) in BT.
                            1120 mode is VUVU */
    VI_DATA_SEQ_UVUV,       /* The input sequence of the second component(only contains u and v) in BT.
                            1120 mode is UVUV */

    VI_DATA_SEQ_UYVY,       /* The input sequence of YUV is UYVY */
    VI_DATA_SEQ_VYUY,       /* The input sequence of YUV is VYUY */
    VI_DATA_SEQ_YUYV,       /* The input sequence of YUV is YUYV */
    VI_DATA_SEQ_YVYU,       /* The input sequence of YUV is YVYU */

    VI_DATA_SEQ_BUTT
} VI_YUV_DATA_SEQ_E;

/* Clock edge mode */
typedef enum scVI_CLK_EDGE_E
{
    VI_CLK_EDGE_SINGLE_UP = 0,         /* single-edge mode and in rising edge */
    VI_CLK_EDGE_SINGLE_DOWN,           /* single-edge mode and in falling edge */

    VI_CLK_EDGE_BUTT
} VI_CLK_EDGE_E;

/* Component mode */
typedef enum scVI_COMPONENT_MODE_E
{
    VI_COMPONENT_MODE_SINGLE = 0,           /* single component mode */
    VI_COMPONENT_MODE_DOUBLE,               /* double component mode */

    VI_COMPONENT_MODE_BUTT
} VI_COMPONENT_MODE_E;

/* Y/C composite or separation mode */
typedef enum scVI_COMBINE_MODE_E
{
    VI_COMBINE_COMPOSITE = 0,     /* Composite mode */
    VI_COMBINE_SEPARATE,          /* Separate mode */

    VI_COMBINE_BUTT
} VI_COMBINE_MODE_E;

/* Attribute of the vertical synchronization signal */
typedef enum scVI_VSYNC_E
{
    VI_VSYNC_FIELD = 0,           /* Field/toggle mode:a signal reversal means a new frame or a field */
    VI_VSYNC_PULSE,               /* Pusle/effective mode:a pusle or an effective signal means a new frame or a field */

    VI_VSYNC_BUTT
} VI_VSYNC_E;

/* Polarity of the vertical synchronization signal */
typedef enum scVI_VSYNC_NEG_E
{
    VI_VSYNC_NEG_HIGH = 0,        /* if VIU_VSYNC_E = VIU_VSYNC_FIELD, then the vertical synchronization signal of
                                    even field is high-level,
                                    if VIU_VSYNC_E = VIU_VSYNC_PULSE,then the vertical synchronization
                                    pulse is positive pulse. */
    VI_VSYNC_NEG_LOW,             /* if VIU_VSYNC_E = VIU_VSYNC_FIELD, then the vertical synchronization signal
                                    of even field is low-level,
                                    if VIU_VSYNC_E = VIU_VSYNC_PULSE,then the vertical synchronization
                                    pulse is negative pulse. */
    VI_VSYNC_NEG_BUTT
} VI_VSYNC_NEG_E;

/* Attribute of the horizontal synchronization signal */
typedef enum scVI_HSYNC_E
{
    VI_HSYNC_VALID_SINGNAL = 0,   /* the horizontal synchronization is valid signal mode */
    VI_HSYNC_PULSE,               /* the horizontal synchronization is pulse mode, a new pulse means
                                    the beginning of a new line */

    VI_HSYNC_BUTT
} VI_HSYNC_E;

/* Polarity of the horizontal synchronization signal */
typedef enum scVI_HSYNC_NEG_E
{
    VI_HSYNC_NEG_HIGH = 0,        /* if VI_HSYNC_E = VI_HSYNC_VALID_SINGNAL, then the valid horizontal
                                    synchronization signal is high-level;
                                    if VI_HSYNC_E = VI_HSYNC_PULSE,then the horizontal synchronization
                                    pulse is positive pulse */
    VI_HSYNC_NEG_LOW,             /* if VI_HSYNC_E = VI_HSYNC_VALID_SINGNAL, then the valid horizontal
                                    synchronization signal is low-level;
                                    if VI_HSYNC_E = VI_HSYNC_PULSE, then the horizontal synchronization
                                    pulse is negative pulse */
    VI_HSYNC_NEG_BUTT
} VI_HSYNC_NEG_E;

/* Attribute of the valid vertical synchronization signal */
typedef enum scVI_VSYNC_VALID_E
{
    VI_VSYNC_NORM_PULSE = 0,      /* the vertical synchronization is pusle mode, a pusle means a new frame or field  */
    VI_VSYNC_VALID_SINGAL,        /* the vertical synchronization is effective mode, a effective signal
                                    means a new frame or field */

    VI_VSYNC_VALID_BUTT
} VI_VSYNC_VALID_E;

/* Polarity of the valid vertical synchronization signal */
typedef enum scVI_VSYNC_VALID_NEG_E
{
    VI_VSYNC_VALID_NEG_HIGH = 0,  /* if VI_VSYNC_VALID_E = VI_VSYNC_NORM_PULSE, a positive pulse means vertical
                                    synchronization pulse;
                                    if VI_VSYNC_VALID_E = VI_VSYNC_VALID_SINGAL, the valid vertical synchronization
                                    signal is high-level */
    VI_VSYNC_VALID_NEG_LOW,       /* if VI_VSYNC_VALID_E = VI_VSYNC_NORM_PULSE, a negative pulse
                                    means vertical synchronization pulse;
                                    if VI_VSYNC_VALID_E = VI_VSYNC_VALID_SINGAL, the valid vertical
                                    synchronization signal is low-level */
    VI_VSYNC_VALID_NEG_BUTT
} VI_VSYNC_VALID_NEG_E;

/* Blank information of the input timing */
typedef struct scVI_TIMING_BLANK_S
{
    SC_U32 u32HsyncHfb ;    /* RW;Horizontal front blanking width */
    SC_U32 u32HsyncAct ;    /* RW;Horizontal effetive width */
    SC_U32 u32HsyncHbb ;    /* RW;Horizontal back blanking width */
    SC_U32 u32VsyncVfb ;    /* RW;Vertical front blanking height of one frame or odd-field frame picture */
    SC_U32 u32VsyncVact ;   /* RW;Vertical effetive width of one frame or odd-field frame picture */
    SC_U32 u32VsyncVbb ;    /* RW;Vertical back blanking height of one frame or odd-field frame picture */
    SC_U32 u32VsyncVbfb ;   /* RW;Even-field vertical front blanking height when input mode is interlace
                            (invalid when progressive input mode) */
    SC_U32 u32VsyncVbact ;  /* RW;Even-field vertical effetive width when input mode is interlace
                            (invalid when progressive input mode) */
    SC_U32 u32VsyncVbbb ;   /* RW;Even-field vertical back blanking height when input mode is interlace
                            (invalid when progressive input mode) */
} VI_TIMING_BLANK_S;

/* synchronization information about the BT.601 or DC timing */
typedef struct scVI_SYNC_CFG_S
{
    VI_VSYNC_E              enVsync;
    VI_VSYNC_NEG_E          enVsyncNeg;
    VI_HSYNC_E              enHsync;
    VI_HSYNC_NEG_E          enHsyncNeg;
    VI_VSYNC_VALID_E        enVsyncValid;
    VI_VSYNC_VALID_NEG_E    enVsyncValidNeg;
    VI_TIMING_BLANK_S       stTimingBlank;
} VI_SYNC_CFG_S;

/* the highest bit of the BT.656 timing reference code */
typedef enum scVI_BT656_FIXCODE_E
{
    VI_BT656_FIXCODE_1 = 0,       /* The highest bit of the EAV/SAV data over the BT.656 protocol is always 1. */
    VI_BT656_FIXCODE_0,           /* The highest bit of the EAV/SAV data over the BT.656 protocol is always 0. */

    VI_BT656_FIXCODE_BUTT
} VI_BT656_FIXCODE_E;

/* Polarity of the field indicator bit (F) of the BT.656 timing reference code */
typedef enum scVI_BT656_FIELD_POLAR_E
{
    VI_BT656_FIELD_POLAR_STD = 0, /* the standard BT.656 mode,the first filed F=0,the second filed F=1 */
    VI_BT656_FIELD_POLAR_NSTD,    /* the non-standard BT.656 mode,the first filed F=1,the second filed F=0 */

    VI_BT656_FIELD_POLAR_BUTT
} VI_BT656_FIELD_POLAR_E;

/* synchronization information about the BT.656 */
typedef struct scVI_BT656_SYNC_CFG_S
{
    VI_BT656_FIXCODE_E     enFixCode;
    VI_BT656_FIELD_POLAR_E enFieldPolar;
} VI_BT656_SYNC_CFG_S;

/* Input data type */
typedef enum scVI_DATA_TYPE_E
{
    VI_DATA_TYPE_YUV = 0,
    VI_DATA_TYPE_RGB,

    VI_DATA_TYPE_BUTT
} VI_DATA_TYPE_E;

typedef enum scVI_REPHASE_MODE_E
{
    VI_REPHASE_MODE_NONE = 0,
    VI_REPHASE_MODE_SKIP_1_2,            /* skip 1/2 */
    VI_REPHASE_MODE_SKIP_1_3,            /* skip 1/3 */
    VI_REPHASE_MODE_BINNING_1_2,          /* binning 1/2 */
    VI_REPHASE_MODE_BINNING_1_3,        /* binning 1/3 */

    VI_REPHASE_MODE_BUTT
} VI_REPHASE_MODE_E;

typedef struct scVI_BAS_REPHASE_ATTR_S
{
    VI_REPHASE_MODE_E enHRephaseMode;
    VI_REPHASE_MODE_E enVRephaseMode;
} VI_BAS_REPHASE_ATTR_S;

/* Attribute of bas scale */
typedef struct scVI_BAS_SCALE_ATTR_S
{
    SIZE_S stBasSize; /* RW;bayer scale size. */
} VI_BAS_SCALE_ATTR_S;

/* Attribute of bayer scale */
typedef struct scVI_BAS_ATTR_S
{
    VI_BAS_SCALE_ATTR_S     stSacleAttr;
    VI_BAS_REPHASE_ATTR_S   stRephaseAttr;
} VI_BAS_ATTR_S;

/* Attribute of wdr */
typedef struct scVI_WDR_ATTR_S
{
    WDR_MODE_E  enWDRMode;          /* RW; WDR mode. */
    SC_U32      u32CacheLine;       /* RW; WDR cache line. */
} VI_WDR_ATTR_S;

/* the extended attributes of VI device */
typedef struct scVI_DEV_ATTR_EX_S
{
    VI_INPUT_MODE_E         enInputMode;                    /* RW;Input mode */
    VI_WORK_MODE_E          enWorkMode;                     /* RW; Work mode */

    VI_COMBINE_MODE_E       enCombineMode;                  /* RW;Y/C composite or separation mode */
    VI_COMPONENT_MODE_E     enComponentMode;                /* RW;Component mode (single-component or dual-component) */
    VI_CLK_EDGE_E           enClkEdge;                      /* RW;Clock edge mode (sampling on the rising or
                                                            falling edge) */

    SC_U32                  au32ComponentMask[VI_COMPMASK_NUM]; /* RW;Component mask */

    VI_SCAN_MODE_E          enScanMode;                     /* RW;Input scanning mode (progressive or interlaced) */
    SC_S32                  as32AdChnId[VI_MAX_ADCHN_NUM];  /* RW;AD channel ID. Typically, the default
                                                            value -1 is recommended */

    VI_YUV_DATA_SEQ_E       enDataSeq;                  /* RW;Input data sequence (only the YUV format is supported) */
    VI_SYNC_CFG_S           stSynCfg;                       /* RW;Sync timing. This member must be configured in BT.
                                                            601 mode or DC mode */

    VI_BT656_SYNC_CFG_S     stBT656SynCfg;                  /* RW;Sync timing. This member must be configured in BT.
                                                            656 mode */

    VI_DATA_TYPE_E          enInputDataType;                /* RW;RGB: CSC-709 or CSC-601, PT YUV444 disable;
                                                            YUV: default yuv CSC coef PT YUV444 enable. */

    SC_BOOL                 bDataReverse;                   /* RW;Data reverse */

    SIZE_S                  stSize;                         /* RW;Input size */

    VI_BAS_ATTR_S           stBasAttr;                      /* RW;Attribute of BAS */

    VI_WDR_ATTR_S           stWDRAttr;                      /* RW;Attribute of WDR */

    DATA_RATE_E             enDataRate;                     /* RW;Data rate of Device */
} VI_DEV_ATTR_EX_S;

/* Add, The frequency of a VI device */
typedef struct scVI_DEV_FRE_S
{
    SC_U32 u32VifFreHz; /* RW; Vif  frequency, unit: Hz, 0 - Auto */
    SC_U32 u32IspFreHz; /* RW; Isp  frequency, unit: Hz, 0 - Auto */
    SC_U32 u32HdrFreHz; /* RW; Hdr  frequency, unit: Hz, 0 - Auto */
    SC_U32 u32MipFreHz; /* RW; Mipi frequency, unit: Hz, 0 - Auto */
} VI_DEV_FRE_S;

/* The attributes of a VI device */
typedef struct scVI_DEV_ATTR_S
{
    VI_INTF_MODE_E      enIntfMode;                     /* RW;Interface mode */
    VI_WORK_MODE_E      enWorkMode;                     /* RW;Work mode */

    SC_U32              au32ComponentMask[VI_COMPMASK_NUM];  /* RW;Component mask */
    VI_SCAN_MODE_E      enScanMode;                     /* RW;Input scanning mode (progressive or interlaced) */
    SC_S32              as32AdChnId[VI_MAX_ADCHN_NUM];  /* RW;AD channel ID. Typically, the default value -1
                                                        is recommended */

    /* The below members must be configured in BT.601 mode or DC mode and are invalid in other modes */
    VI_YUV_DATA_SEQ_E   enDataSeq;                      /* RW;Input data sequence (only the YUV format is supported) */
    VI_SYNC_CFG_S       stSynCfg;                       /* RW;Sync timing. This member must be configured in BT.
                                                        601 mode or DC mode */

    VI_DATA_TYPE_E      enInputDataType;                /* RW;RGB: CSC-709 or CSC-601, PT YUV444 disable; YUV: default
                                                        yuv CSC coef PT YUV444 enable. */

    SC_BOOL             bDataReverse;                   /* RW;Data reverse */

    SIZE_S              stSize;                         /* RW;Input size */

    VI_BAS_ATTR_S       stBasAttr;                      /* RW;Attribute of BAS */

    VI_WDR_ATTR_S       stWDRAttr;                      /* RW;Attribute of WDR */

    DATA_RATE_E         enDataRate;                     /* RW;Data rate of Device */

    VI_DEV_FRE_S        stDevFre;                       /* RW;Fre of Device */
} VI_DEV_ATTR_S;

/* Information of pipe binded to device */
typedef struct scVI_DEV_BIND_PIPE_S
{
    SC_U32  u32Num;                                     /* RW;Range [1,VI_MAX_PHY_PIPE_NUM] */
    VI_PIPE PipeId[VI_MAX_PHY_PIPE_NUM];                /* RW;Array of pipe ID */
} VI_DEV_BIND_PIPE_S;

/* Source of 3DNR reference frame */
typedef enum scVI_NR_REF_SOURCE_E
{
    VI_NR_REF_FROM_RFR = 0,                             /* Reference frame from reconstruction frame */
    VI_NR_REF_FROM_CHN0,                                /* Reference frame from CHN0's frame */

    VI_NR_REF_FROM_BUTT
} VI_NR_REF_SOURCE_E;

typedef enum scVI_PIPE_BYPASS_MODE_E
{
    VI_PIPE_BYPASS_NONE,
    VI_PIPE_BYPASS_FE,
    VI_PIPE_BYPASS_BE,

    VI_PIPE_BYPASS_BUTT
} VI_PIPE_BYPASS_MODE_E;

/* The attributes of 3DNR */
typedef struct scVI_NR_ATTR_S
{
    PIXEL_FORMAT_E      enPixFmt;                       /* RW;Pixel format of reference frame */
    DATA_BITWIDTH_E     enBitWidth;                     /* RW;Bit Width of reference frame */
    VI_NR_REF_SOURCE_E  enNrRefSource;                  /* RW;Source of 3DNR reference frame */
    COMPRESS_MODE_E     enCompressMode;                 /* RW;Reference frame compress mode */
} VI_NR_ATTR_S;

/* The attributes of pipe */
typedef struct scVI_PIPE_ATTR_S
{
    VI_PIPE_BYPASS_MODE_E enPipeBypassMode;
    SC_BOOL               bYuvSkip;               /* RW;YUV skip enable */
    SC_BOOL               bIspBypass;             /* RW;Range:[0, 1];ISP bypass enable */
    SC_U32                u32MaxW;                /* RW;Range[VI_PIPE_MIN_WIDTH, VI_PIPE_MAX_WIDTH];Maximum width */
    SC_U32                u32MaxH;                /* RW;Range[VI_PIPE_MIN_HEIGHT, VI_PIPE_MAX_HEIGHT];Maximum height */
    PIXEL_FORMAT_E        enPixFmt;               /* RW;Pixel format */
    COMPRESS_MODE_E       enCompressMode;         /* RW;Range:[0, 4];Compress mode. */
    DATA_BITWIDTH_E       enBitWidth;             /* RW;Range:[0, 4];Bit width */
    SC_BOOL               bNrEn;                  /* RW;Range:[0, 1];3DNR enable */
    VI_NR_ATTR_S          stNrAttr;               /* RW;Attribute of 3DNR */
    SC_BOOL               bSharpenEn;             /* RW;Range:[0, 1];Sharpen enable */
    FRAME_RATE_CTRL_S     stFrameRate;            /* RW;Frame rate */
    SC_BOOL               bDiscardProPic;         /* RW;Range:[0, 1];when professional mode snap, whether to discard
                                                long exposure picture in the video pipe. */
    SC_U32                u32MaxFrameRate;        /* RW;Max frame rate */
} VI_PIPE_ATTR_S;

typedef enum scVI_STITCH_ISP_CFG_MODE_E
{
    VI_STITCH_ISP_CFG_NORMAL = 0,
    VI_STITCH_ISP_CFG_SYNC,
    VI_STITCH_ISP_CFG_BUTT
} VI_STITCH_ISP_CFG_MODE_E;

/* Information of stitch group */
typedef struct scVI_STITCH_GRP_ATTR_S
{
    SC_BOOL                     bStitch;
    VI_STITCH_ISP_CFG_MODE_E    enMode;
    SC_U32                      u32MaxPTSGap;            /* RW;MAX PTS Gap between frame of pipe,unit:us */
    SC_U32                      u32PipeNum;              /* RW;Range [2, VI_MAX_PIPE_NUM] */
    VI_PIPE                     PipeId[VI_MAX_PIPE_NUM]; /* RW;Array of pipe ID */
} VI_STITCH_GRP_ATTR_S;

typedef enum scVI_PIPE_REPEAT_MODE_E
{
    VI_PIPE_REPEAT_NONE = 0,
    VI_PIPE_REPEAT_ONCE = 1,
    VI_PIPE_REPEAT_BUTT
} VI_PIPE_REPEAT_MODE_E;

typedef struct
{
    SC_U8   IES;              /* RW; Range:[0, 255];Format 8.0;the absolute strength of image enhancement for edge */
    SC_U8   IESS;             /* RW; Range:[0, 255];Format 8.0;the absolute strength of image enhancement for
                            texture and shadow */
    SC_U16  IEDZ;             /* RW; Range:[0, 8192];Format 14.0;the threshold of image enhancement
                            for controlling noise */
} tV59aIEy;

typedef struct
{
    SC_U8   SBF     : 2;      /* RW; Range:[0, 3];Format 2.0;the band type of spatial filter, notice: SBF0, SBF1
                            range is [2, 3];SBF2,SBF3,SBF4 range is [0,3], where SBF4 is related to SBFk */
    SC_U8   STR     : 4;      /* RW; Range:[0, 13];Format 4.0;the relative strength of spatial filter refer to
                            the previous frame */
    SC_U8   STHp    : 2;      /* RW; Range:[0, 2];Format 2.0;Not recommended for debugging */
    SC_U8   SFT     : 5;      /* RW; Range:[0, 31];Format 5.0;Not recommended for debugging */
    SC_U8   kPro    : 3;      /* RW; Range:[0, 7];Format 3.0;notice: the kPro of SFy2 range is [0, 7], the kPro of
                            SFy3 range is [0, 4] */

    SC_U16  STH[3];           /* RW; Range:[0, 999];Format 10.0;the edge-preserve threshold for spatial filter */
    SC_U16  SBS[3];           /* RW; Range:[0, 9999];Format 14.0;the noise reduction strength of spatial filter
                            for the relative bright pixel */
    SC_U16  SDS[3];           /* RW; Range:[0, 9999];Format 14.0;the noise reduction strength of spatial filter
                            for the relative dark pixel */

} tV59aSFy;

typedef struct
{
    SC_U16  MATH    : 10;    /* RW; Range:[0, 1023];Format 10.0;the motion detection threshold for temporal filter */
    SC_U16  MATE    : 4;     /* RW; Range:[0, 11];Format 4.0;the motion detection index of flat area
                            for temporal filter */
    SC_U16  MATW    : 2;     /* RW; Range:[0, 3];Format 2.0;the index of suppressing trailing for temporal filter */
    SC_U8   MASW    : 4;     /* RW; Range:[0, 12];Format 4.0;the index of suppressing raindrop noise
                            for temporal filter */
    SC_U8   MABW    : 3;     /* RW; Range:[0, 4];Format 3.0;the window of motion detection for temporal filter */
    SC_U8   MAXN    : 1;     /* RW; Range:[0, 1];Format 1.0;Not recommended for debugging */

} tV59aMDy;

typedef struct
{
    SC_U8   TFR[4];         /* RW; Range:[0, 255];Format 8.0;the relative strength of temporal
                            filter for the static area */
    SC_U16  TDZ : 14;       /* RW; Range:[0, 999];Format 10.0;the threshold of dead-area of temporal filter */
    SC_U16  TDX : 2;        /* RW; Range:[0, 2];Format 2.0;Not recommended for debugging */
    SC_U16  TFS : 6;        /* RW; Range:[0, 63];Format 6.0;the absolute strength of temporal filter  */
} tV59aTFy;

typedef struct
{
    SC_U16  SFC     : 10;   /* RW; Range:[0, 1023];Format 10.0;the strength of spatial filter for NRC0 */
    SC_U16  TFC     : 6;    /* RW; Range:[0, 63];Format 6.0;the strength of temporal filter for NRC0 */
    SC_U16  CSFS    : 14;   /* RW; Range:[0, 999];Format 10.0;the strength of spatial filter for NRC1 */
    SC_U16  CSFk    : 2;    /* RW; Range:[0, 3];Format 2.0;Not recommended for debugging */
    SC_U16  CTFS    : 4;    /* RW; Range:[0, 15];Format 4.0;the strength of temporal filter for NRC1 */
    SC_U16  CIIR    : 1;    /* RW; Range:[0, 1];Format 1.0;the mode of spatial filter for NRC1 */
    SC_U16  CTFR    : 11;   /* RW; Range:[0, 999];Format 10.0;the relative strength of temporal filter for NRC1 */
    SC_U8   MODE    : 1;    /* RW; Range: [0,  1]; The switch for new chroma denoise mode. */
    SC_U8   PRESFC  : 6;    /* RW; Range: [0,  32]; The strength for chroma pre spatial filter. */
    SC_U8  _rb1_    : 1;

} tV59aNRc;

/* 3DNR Spatial Filter: SFy0,SFy1,SFy2,SFy3; Temporal Filter:TFy0,TFy1;Chroma Noise Reduction: NRC0,NRC1 */
typedef struct
{
    tV59aIEy  IEy;
    tV59aSFy  SFy[5];
    tV59aMDy  MDy[2];
    tV59aTFy  TFy[2];

    SC_U16  HdgType : 1;   /* RW; Range:[0, 1];Format 1.0;the type of complexed mixed spatial filter whether
                            is SFi or SFk */
    SC_U16  BriType : 1;   /* RW; Range:[0, 1];Format 1.0;the mode decide SFy3 whether is SFk type or SFi type */
    SC_U16  HdgMode : 2;   /* RW; Range:[0, 3];Format 2.0;the mode decide complexed mixed spatial filter band
                            for flat area */
    SC_U16  kTab2   : 1;   /* RW; Range:[0, 1];Format 1.0;the parameter decide SFy2 whether or not based on the image
                            absolute luminance */
    SC_U16  HdgWnd  : 1;   /* RW; Range:[0, 1];Format 1.0;the sampling window of complexed mixed spatial filter for
                            noise detection */
    SC_U16  kTab3   : 1;   /* RW; Range:[0, 1];Format 1.0;the parameter decide SFy3 whether or not based on the image
                            absolute luminance */
    SC_U16  HdgSFR  : 4;   /* RW; Range:[0, 13];Format 4.0;the trend of the noise reduction of complexed mixed spatial
                            filter for flat area */
    SC_U16  nOut    : 5;   /* RW; Range:[0, 27];Format 5.0;the parameter for output intermediate result of SFy3 */
    SC_U8   HdgIES;        /* RW; Range:[0, 255];Format 8.0;the strength of image enhancement for complexed
                            mixed spatial filter */
    SC_U8   nRef    : 1;   /* RW; Range:[0, 1];Format 1.0;Not recommended for debugging */

    SC_U8   IEyMode : 1;   /* RW; Range:[0, 1];Format 1.0;the image enhancement mode selection. */
    SC_U8   IEyEx[4];      /* RW; Range:[0, 255];Format 8.0;the image enhancement strength for different frequency. */

    SC_U8   SFRi[4];       /* RW; Range:[0, 255];Format 8.0;the relative strength of SFy3 when the filter type is SFi */
    SC_U8   SFRk[4];       /* RW; Range:[0, 255];Format 8.0;the relative strength of SFy3 when the filter type is SFk */
    SC_U16  SBSk2[32];     /* RW; Range:[0, 9999];Format 14.0;the noise reduction strength of SFy2 for the
                            relative bright pixel based on the image absolute luminance */
    SC_U16  SBSk3[32];     /* RW; Range:[0, 9999];Format 14.0;the noise reduction strength of SFy3 for the
                            relative bright pixel based on the image absolute luminance */
    SC_U16  SDSk2[32];     /* RW; Range:[0, 9999];Format 14.0;the noise reduction strength of SFy2 for the
                            relative dark pixel based on the image absolute luminance */
    SC_U16  SDSk3[32];     /* RW; Range:[0, 9999];Format 14.0;the noise reduction strength of SFy3 for the
                            relative dark pixel based on the image absolute luminance */
    SC_U16  BriThr[16];    /* RW; Range:[0, 1024];Format 11.0;the threshold decide SFy3 choose the SFi type filter
                            or SFk type filter in dark and bright area */

    tV59aNRc NRc;
} VI_PIPE_NRX_PARAM_V1_S;

typedef enum scVI_NR_VERSION_E
{
    VI_NR_V1 = 1,
    VI_NR_V2 = 2,
    VI_NR_V3 = 3,
    VI_NR_V4 = 4,
    VI_NR_BUTT
} VI_NR_VERSION_E;

typedef struct scNRX_PARAM_MANUAL_V1_S
{
    VI_PIPE_NRX_PARAM_V1_S stNRXParamV1;
} NRX_PARAM_MANUAL_V1_S;

typedef struct scNRX_PARAM_AUTO_V1_S
{
    SC_U32                              u32ParamNum;
    SC_U32                  ATTRIBUTE   *pau32ISO;
    VI_PIPE_NRX_PARAM_V1_S  ATTRIBUTE   *pastNRXParamV1;
} NRX_PARAM_AUTO_V1_S;

typedef struct scNRX_PARAM_V1_S
{
    OPERATION_MODE_E        enOptMode;            /* RW;Adaptive NR */
    NRX_PARAM_MANUAL_V1_S   stNRXManualV1;        /* RW;NRX V1 param for manual */
    NRX_PARAM_AUTO_V1_S     stNRXAutoV1;          /* RW;NRX V1 param for auto */
} NRX_PARAM_V1_S;

typedef struct
{
    SC_U8  IES0, IES1, IES2, IES3;  /* IES0~4 ; Range: [0, 255]; The gains of edge and texture enhancement.
                                    0~3 for different frequency response. */
    SC_U16 IEDZ : 10, _rb_ : 6;     /* IEDZ   ; Range: [0, 999]; The threshold to control the generated artifacts. */
} tV500_VI_IEy;

typedef struct
{
    SC_U8  SPN6 : 3, SFR  : 5;      /* SPN6; Range: [0, 5];  The selection of filters to be mixed for NO.6 filter. */
    /* SFR ; Range: [0, 31];  The relative NR strength in the SFi and SFk filter. */
    SC_U8  SBN6 : 3, PBR6 : 5;      /* SBN6; Range: [0, 5];  The selection of filters to be mixed for NO.6 filter. */
    /* PBR6; Range: [0, 16];  The mix ratio between SPN6 and SBN6. */
    SC_U16 SRT0 : 5, SRT1 : 5, JMODE : 3, DeIdx : 3;    /* JMODE;      Range: [0, 4]; The selection modes
                                                         for the blending of spatial filters */
    /* STR0, STR1; Range: [0, 16]; The blending ratio
     of different filters. (Used in serial filtering mode (SFM).) */
    /* DeIdx;      Range: [3, 6]; The selection number of
     filters that textures and details will be added to. */
    SC_U8  DeRate, SFR6[3];                             /* DeRate;     Range: [0, 255]; The enhancement strength
                                                         for the SFM (When DeRate > 0, the SFM will be activated) */
    /* SFR6;       Range: [0, 31]; The relative NR strength
     for NO.6 filter. (Effective when JMODE = 4) */
    SC_U8  SFS1, SFT1, SBR1;                            /* SFS1, SFT1, SBR1; Range: [0, 255];  The NR strength
                                                         parameters for NO.1 filter. */
    SC_U8  SFS2, SFT2, SBR2;                            /* SFS2, SFT2, SBR2; Range: [0, 255];  The NR strength
                                                         parameters for NO.2 filter. */
    SC_U8  SFS4, SFT4, SBR4;                            /* SFS4, SFT4, SBR4; Range: [0, 255];  The NR strength
                                                         parameters for NO.3 and NO.4 filters. */

    SC_U16 STH1 : 9, SFN1 : 3, NRyEn : 1, SFN0  : 3;    /* STH1~3; Range: [0, 511]; The thresholds for protection
                                                         of edges from blurring */
    /* NRyEn ; Range: [0, 1]; The NR switches */
    SC_U16 STH2 : 9, SFN2 : 3, BWSF4 : 1, kMode : 3;    /* SFN0~3; Range: [0, 6]; Filter selection for different
                                                         image areas based on STH1~3. */
    /* BWSF4 ; Range: [0, 1]; The NR window size for the
     NO.3 and NO.4 filters.  */
    SC_U16 STH3 : 9, SFN3 : 3, TriTh : 1, _rb0_ : 3;    /* KMode ; Range: [0, 3]; The denoise mode based
                                                         on image brightness. */
    /* Trith ; Range: [0, 1]; The switch to choose 3 STH
     threshold or 2 STH threshold */
} tV500_VI_SFy;

typedef struct
{
    tV500_VI_IEy IEy;
    tV500_VI_SFy SFy;
} VI_PIPE_NRX_PARAM_V2_S;

typedef struct scNRX_PARAM_MANUAL_V2_S
{
    VI_PIPE_NRX_PARAM_V2_S stNRXParamV2;
} NRX_PARAM_MANUAL_V2_S;

typedef struct scNRX_PARAM_AUTO_V2_S
{
    SC_U32                              u32ParamNum;
    SC_U32                  ATTRIBUTE   *pau32ISO;
    VI_PIPE_NRX_PARAM_V2_S  ATTRIBUTE   *pastNRXParamV2;
} NRX_PARAM_AUTO_V2_S;

typedef struct scNRX_PARAM_V2_S
{
    OPERATION_MODE_E        enOptMode;           /* RW;Adaptive NR */
    NRX_PARAM_MANUAL_V2_S   stNRXManualV2;       /* RW;NRX V2 param for manual */
    NRX_PARAM_AUTO_V2_S     stNRXAutoV2;         /* RW;NRX V2 param for auto */
} NRX_PARAM_V2_S;

typedef struct scVI_PIPE_NRX_PARAM_S
{
    VI_NR_VERSION_E enNRVersion; /* RW;3DNR Version */
    union
    {
        NRX_PARAM_V1_S stNRXParamV1; /* RW;3DNR X param version 1 */
        NRX_PARAM_V2_S stNRXParamV2; /* RW;3DNR X param version 2 */
    };
} VI_PIPE_NRX_PARAM_S;

/* The attributes of channel */
typedef struct scVI_CHN_ATTR_S
{
    SIZE_S              stSize;             /* RW;Channel out put size */
    PIXEL_FORMAT_E      enPixelFormat;      /* RW;Pixel format */
    DYNAMIC_RANGE_E     enDynamicRange;     /* RW;Dynamic Range */
    VIDEO_FORMAT_E      enVideoFormat;      /* RW;Video format */
    COMPRESS_MODE_E     enCompressMode;     /* RW;256B Segment compress or no compress. */
    SC_BOOL             bMirror;            /* RW;Mirror enable */
    SC_BOOL             bFlip;              /* RW;Flip enable */
    SC_U32              u32Depth;           /* RW;Range [0,8];Depth */
    FRAME_RATE_CTRL_S   stFrameRate;        /* RW;Frame rate */
    SC_U32              u32BufCount;        /* RW;Vi buf count */
} VI_CHN_ATTR_S;

/* The status of pipe */
typedef struct scVI_PIPE_STATUS_S
{
    SC_BOOL bEnable;                        /* RO;Whether this pipe is enabled */
    SC_U32  u32IntCnt;                      /* RO;The video frame interrupt count */
    SC_U32  u32FrameRate;                   /* RO;Current frame rate */
    SC_U32  u32LostFrame;                   /* RO;Lost frame count */
    SC_U32  u32VbFail;                      /* RO;Video buffer malloc failure */
    SIZE_S  stSize;                         /* RO;Current pipe output size */
} VI_PIPE_STATUS_S;

/* VS signal output mode */
typedef enum scVI_VS_SIGNAL_MODE_E
{
    VI_VS_SIGNAL_ONCE = 0,                      /* output one time */
    VI_VS_SIGNAL_FREQ,                          /* output frequently */

    VI_VS_SIGNAL_MODE_BUTT
} VI_VS_SIGNAL_MODE_E;

/* The attributes of VS signal */
typedef struct scVI_VS_SIGNAL_ATTR_S
{
    VI_VS_SIGNAL_MODE_E enMode;             /* RW;output one time, output frequently */
    SC_U32              u32StartTime;       /* RW;output start time,unit: sensor pix clk. */
    SC_U32              u32Duration;        /* RW;output high duration, unit: sensor pix clk. */
    SC_U32              u32CapFrmIndex;     /* RW;VS signal will be output after trigger by which vframe,
                                            default is 0. */
    SC_U32              u32Interval;        /* RW;output frequently interval, unit: frame */
} VI_VS_SIGNAL_ATTR_S;

typedef struct scBNR_DUMP_ATTR_S
{
    SC_BOOL bEnable;
    SC_U32 u32Depth;
} BNR_DUMP_ATTR_S;

typedef enum scVI_EXT_CHN_SOURCE_E
{
    VI_EXT_CHN_SOURCE_TAIL,
    VI_EXT_CHN_SOURCE_HEAD,

    VI_EXT_CHN_SOURCE_BUTT
} VI_EXT_CHN_SOURCE_E;

typedef struct scVI_EXT_CHN_ATTR_S
{
    VI_EXT_CHN_SOURCE_E enSource;
    VI_CHN              s32BindChn;     /* RW;Range [VI_CHN0, VI_MAX_PHY_CHN_NUM);The channel num which extend
                                        channel will bind to */
    SIZE_S              stSize;         /* RW;Channel out put size */
    PIXEL_FORMAT_E      enPixFormat;    /* RW;Pixel format */
    DYNAMIC_RANGE_E     enDynamicRange; /* RW;Dynamic Range */
    COMPRESS_MODE_E     enCompressMode; /* RW;256B Segment compress or no compress. */
    SC_U32              u32Depth;       /* RW;Range [0,8];Depth */
    FRAME_RATE_CTRL_S   stFrameRate;    /* RW;Frame rate */
} VI_EXT_CHN_ATTR_S;

typedef enum scVI_CROP_COORDINATE_E
{
    VI_CROP_RATIO_COOR = 0,             /* Ratio coordinate */
    VI_CROP_ABS_COOR,                   /* Absolute coordinate */
    VI_CROP_BUTT
} VI_CROP_COORDINATE_E;

/* Information of chn crop */
typedef struct scVI_CROP_INFO_S
{
    SC_BOOL                 bEnable;            /* RW;CROP enable */
    VI_CROP_COORDINATE_E    enCropCoordinate;   /* RW;Coordinate mode of the crop start point */
    RECT_S                  stCropRect;         /* RW;CROP rectangular */
} VI_CROP_INFO_S;

/* The attributes of LDC */
typedef struct scVI_LDC_ATTR_S
{
    SC_BOOL bEnable; /* RW;Range [0,1];Whether LDC is enbale */
    LDC_ATTR_S stAttr;
} VI_LDC_ATTR_S;

/* The attributes of LDCV2 */
typedef struct scVI_LDCV2_ATTR_S
{
    SC_BOOL bEnable; /* RW;Whether LDC is enbale */
    LDCV2_ATTR_S stAttr;
} VI_LDCV2_ATTR_S;

/* The attributes of LDCV3 */
typedef struct scVI_LDCV3_ATTR_S
{
    SC_BOOL bEnable; /* RW;Whether LDC is enbale */
    LDCV3_ATTR_S stAttr;
} VI_LDCV3_ATTR_S;

typedef struct scVI_ROTATION_EX_ATTR_S
{
    SC_BOOL       bEnable;                 /* RW;Range [0,1];Whether ROTATE_EX_S is enbale */
    ROTATION_EX_S stRotationEx;
} VI_ROTATION_EX_ATTR_S;

/* The status of chn */
typedef struct scVI_CHN_STATUS_S
{
    SC_BOOL bEnable;                    /* RO;Whether this channel is enabled */
    SC_U32  u32FrameRate;               /* RO;current frame rate */
    SC_U32  u32LostFrame;               /* RO;Lost frame count */
    SC_U32  u32VbFail;                  /* RO;Video buffer malloc failure */
    SIZE_S  stSize;                     /* RO;chn output size */

} VI_CHN_STATUS_S;

typedef struct scVI_PMF_ATTR_S
{
    SC_BOOL bEnable;                            /* RW;Whether PMF is enable */
    SIZE_S  stDestSize;                         /* RW;Target size */
    SC_S64  as64PMFCoef[VI_PMFCOEF_NUM];        /* RW; Array of PMF coefficients */
} VI_PMF_ATTR_S;

typedef enum scVI_DUMP_TYPE_E
{
    VI_DUMP_TYPE_RAW = 0,
    VI_DUMP_TYPE_YUV = 1,
    VI_DUMP_TYPE_IR = 2,
    VI_DUMP_TYPE_BUTT
} VI_DUMP_TYPE_E;

typedef struct scVI_DUMP_ATTR_S
{
    SC_BOOL         bEnable;              /* RW;Whether dump is enable */
    SC_U32          u32Depth;             /* RW;Range [0,8];Depth */
    VI_DUMP_TYPE_E  enDumpType;
} VI_DUMP_ATTR_S;

typedef enum scVI_PIPE_FRAME_SOURCE_E
{
    VI_PIPE_FRAME_SOURCE_DEV = 0, /* RW;Source from dev */
    VI_PIPE_FRAME_SOURCE_USER_FE, /* RW;User send to FE */
    VI_PIPE_FRAME_SOURCE_USER_BE, /* RW;User send to BE */

    VI_PIPE_FRAME_SOURCE_BUTT
} VI_PIPE_FRAME_SOURCE_E;

typedef struct sc_VI_RAW_INFO_S
{
    VIDEO_FRAME_INFO_S      stVideoFrame;
    ISP_CONFIG_INFO_S       stIspInfo;
} VI_RAW_INFO_S;

/* module params */
typedef struct scVI_MOD_PARAM_S
{
    SC_S32      s32DetectErrFrame;
    SC_U32      u32DropErrFrame;
    VB_SOURCE_E enViVbSource;
} VI_MOD_PARAM_S;

typedef struct scVI_DEV_TIMING_ATTR_S
{
    SC_BOOL bEnable;               /* RW;Range:[0,1];Whether enable VI generate timing */
    SC_S32  s32FrmRate;            /* RW;Range:(0,0xffffff];;Generate timing Frame rate */
} VI_DEV_TIMING_ATTR_S;

typedef struct scVI_EARLY_INTERRUPT_S
{
    SC_BOOL bEnable;
    SC_U32 u32LineCnt;
} VI_EARLY_INTERRUPT_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

