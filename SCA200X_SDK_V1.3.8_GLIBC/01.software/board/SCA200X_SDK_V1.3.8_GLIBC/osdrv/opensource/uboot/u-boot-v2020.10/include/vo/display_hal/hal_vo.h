/****************************************************************************
 * Copyright (C) 2019 Beijiing Smartchip Microelectronics Technology CO., LTD.            *
 ****************************************************************************/
/** \addtogroup VO
 *  @{
 */

/**
 * @file hal_vo.h
 * @brief 定义视频输出设备用户接口
 * @author SmartChip Software Team
 * @version 0.0.1
 * @date 2021/04/27
 * @license 2021-2025, SmartChip. Co., Ltd.
**/

#ifndef _SC_HAL_VO_API_H_
#define _SC_HAL_VO_API_H_

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================

// Include files

//=============================================================================
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "hal_type.h"
#include "hal_errno.h"

//=============================================================================

// Macro definition

//=============================================================================

#define SC_HAL_MOD_VO          15
#define SC_HAL_VO_SUCCESS      0

typedef enum HAL_VO_ERRNO {
	SC_ERR_VO_HAL_MODE                = 0x40,
	SC_ERR_VO_NO_MEM,
	SC_ERR_VO_CLOSE_FD,
	SC_ERR_VO_PARA,
	SC_ERR_VO_NOT_SUPPORT,
	SC_ERR_VO_FORMAT_NOT_SUPPORT,
	SC_ERR_VO_TIMING_NOT_SUPPORT,
	SC_ERR_VO_INTF_NOT_SUPPORT,
	SC_ERR_VO_CSC_NOT_SUPPORT,

	SC_ERR_VO_DEV_OPEN                = 0x100,
	SC_ERR_VO_DEV_CLOSE,
	SC_ERR_VO_DEV_FD,
	SC_ERR_VO_DEV_ID,
	SC_ERR_VO_DEV_TIMING_TEMPLATE,
	SC_ERR_VO_DEV_CURSOR_HAS_ENABLED,
	SC_ERR_VO_DEV_CURSOR_NOT_ENABLE,

	SC_ERR_VO_DEV_NOT_CONFIG,
	SC_ERR_VO_DEV_NOT_ENABLE,
	SC_ERR_VO_DEV_HAS_ENABLED,
	SC_ERR_VO_DEV_HAS_BINDED,
	SC_ERR_VO_DEV_NOT_BINDED,

	SC_ERR_VO_LAYER_ID                = 0x200,
	SC_ERR_VO_LAYER_OPEN,
	SC_ERR_VO_LAYER_CLOSE,
	SC_ERR_VO_LAYER_NOT_ENABLE,
	SC_ERR_VO_LAYER_HAS_ENABLED,
	SC_ERR_VO_LAYER_POSITION,
	SC_ERR_VO_LAYER_GE2D_OPR,

	SC_ERR_VO_CHN_HAS_ENABLED         = 0x300,
	SC_ERR_VO_CHN_NOT_ENABLE,
	SC_ERR_VO_CHN_OPEN,
	SC_ERR_VO_CHN_NOT_CONFIG,
	SC_ERR_VO_CHN_NOT_ALLOC,
	SC_ERR_VO_CHN_CREATE,
	SC_ERR_VO_CHN_DELETE,
	SC_ERR_VO_CHN_GET,
	SC_ERR_VO_CHN_ID,
	SC_ERR_VO_CHN_SET_ATTR,
	SC_ERR_VO_CHN_ENQUEUE,
	SC_ERR_VO_CHN_DEQUEUE,
	SC_ERR_VO_CHN_QUEUE_NOT_EMPTY,
	SC_ERR_VO_CHN_QUEUE_EMPTY,
	SC_ERR_VO_CHN_TIMEOUT,
	SC_ERR_VO_CHN_MEM_ADD_REF,
	SC_ERR_VO_CHN_MEM_SUB_REF,
	SC_ERR_VO_CHN_EXSIT,
	SC_ERR_VO_CHN_THREAD_CREATE,
	SC_ERR_VO_CHN_CLEAR_BUF,

	SC_ERR_VO_FUSION_GET,

	SC_ERR_VO_CCD_INVALID_PAT,
	SC_ERR_VO_CCD_INVALID_POS,

	SC_ERR_VO_WAIT_TIMEOUT,

	SC_ERR_VO_MAX
} ENUM_HAL_VO_ERRNO;

#define  HAL_ERR_VO_HAL_MODE                     SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_HAL_MODE)
#define  HAL_ERR_VO_NO_MEM                       SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_NO_MEM)
#define  HAL_ERR_VO_CLOSE_FD                     SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CLOSE_FD)
#define  HAL_ERR_VO_PARA                         SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_PARA)
#define  HAL_ERR_VO_NOT_SUPPORT                  SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_NOT_SUPPORT)
#define  HAL_ERR_VO_FORMAT_NOT_SUPPORT           SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_FORMAT_NOT_SUPPORT)
#define  HAL_ERR_VO_TIMING_NOT_SUPPORT           SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_TIMING_NOT_SUPPORT)
#define  HAL_ERR_VO_INTF_NOT_SUPPORT             SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_INTF_NOT_SUPPORT)
#define  HAL_ERR_VO_CSC_NOT_SUPPORT              SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CSC_NOT_SUPPORT)

#define  HAL_ERR_VO_DEV_OPEN                     SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_OPEN)
#define  HAL_ERR_VO_DEV_CLOSE                    SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_CLOSE)
#define  HAL_ERR_VO_DEV_FD                       SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_FD)
#define  HAL_ERR_VO_DEV_ID                       SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_ID)
#define  HAL_ERR_VO_DEV_CURSOR_HAS_ENABLED       SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_CURSOR_HAS_ENABLED)
#define  HAL_ERR_VO_DEV_CURSOR_NOT_ENABLE        SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_CURSOR_NOT_ENABLE)
#define  HAL_ERR_VO_DEV_TIMING_TEMPLATE          SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_TIMING_TEMPLATE)
#define  HAL_ERR_VO_DEV_NOT_ENABLE               SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_NOT_ENABLE)
#define  HAL_ERR_VO_DEV_HAS_ENABLED              SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_DEV_HAS_ENABLED)

#define  HAL_ERR_VO_LAYER_ID                     SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_LAYER_ID)
#define  HAL_ERR_VO_LAYER_OPEN                   SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_LAYER_OPEN)
#define  HAL_ERR_VO_LAYER_CLOSE                  SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_LAYER_CLOSE)
#define  HAL_ERR_VO_LAYER_NOT_ENABLE             SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_LAYER_NOT_ENABLE)
#define  HAL_ERR_VO_LAYER_HAS_ENABLED            SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_LAYER_HAS_ENABLED)
#define  HAL_ERR_VO_LAYER_POSITION               SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_LAYER_POSITION)
#define  HAL_ERR_VO_LAYER_GE2D_OPR               SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_LAYER_GE2D_OPR)

#define  HAL_ERR_VO_CHN_CREATE                   SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_CREATE)
#define  HAL_ERR_VO_CHN_DELETE                   SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_DELETE)
#define  HAL_ERR_VO_CHN_ID                       SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_ID)
#define  HAL_ERR_VO_CHN_NOT_ENABLE               SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_NOT_ENABLE)
#define  HAL_ERR_VO_CHN_HAS_ENABLED              SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_HAS_ENABLED)
#define  HAL_ERR_VO_CHN_OPEN                     SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_OPEN)
#define  HAL_ERR_VO_CHN_GET                      SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_GET)
#define  HAL_ERR_VO_CHN_SET_ATTR                 SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_SET_ATTR)
#define  HAL_ERR_VO_CHN_ENQUEUE                  SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_ENQUEUE)
#define  HAL_ERR_VO_CHN_DEQUEUE                  SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_DEQUEUE)
#define  HAL_ERR_VO_CHN_TIMEOUT                  SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_TIMEOUT)
#define  HAL_ERR_VO_CHN_QUEUE_NOT_EMPTY          SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_QUEUE_NOT_EMPTY)
#define  HAL_ERR_VO_CHN_QUEUE_EMPTY              SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_QUEUE_EMPTY)
#define  HAL_ERR_VO_CHN_MEM_ADD_REF              SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_MEM_ADD_REF)
#define  HAL_ERR_VO_CHN_MEM_SUB_REF              SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_MEM_SUB_REF)
#define  HAL_ERR_VO_CHN_EXSIT                    SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_EXSIT)
#define  HAL_ERR_VO_CHN_THREAD_CREATE            SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_THREAD_CREATE)
#define  HAL_ERR_VO_CHN_CLEAR_BUF                SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_CHN_CLEAR_BUF)

#define  HAL_ERR_VO_FUSION_GET                   SC_DEF_ERR(HAL_MOD_VO, HAL_ERR_LEVEL_ERROR, SC_ERR_VO_FUSION_GET)

/**
* @note  最大通道数
*/
#define  MAX_VO_CHN_NUM                       9

/**
* @note  鼠标文件名最大长度
*/
#define  MAX_VO_CURSOR_FILE_NAME              64

/**
* @note  最大面板数
*/
#define  MAX_VO_PANNEL_NUM                    3

//=============================================================================

// Data type definition

//=============================================================================

/**
* @note  视频层id
*/
typedef enum {
	SC_HAL_VO_LAYER_ID_VIDEO_0,
	SC_HAL_VO_LAYER_ID_OVERLAY_0,
	SC_HAL_VO_LAYER_ID_OVERLAY_1,
	SC_HAL_VO_LAYER_ID_MAX,
} ENUM_SC_HAL_VO_LAYER_ID;

/**
* @note  视频输出设备id，目前仅支持 SC_HAL_VO_DEV_ID_HD0
*/
typedef enum {
	SC_HAL_VO_DEV_ID_HD0,
	SC_HAL_VO_DEV_ID_MAX,
} ENUM_SC_HAL_VO_DEV_ID;

/**
* @note   vo原生视频输出接口. 支持 SC_HAL_VO_DEV_INTF_DVP 和 SC_HAL_VO_DEV_INTF_MIPI
*/
typedef enum {
	SC_HAL_VO_DEV_INTF_NONE,            // 无
	SC_HAL_VO_DEV_INTF_DVP,             // DVP接口
	SC_HAL_VO_DEV_INTF_DP,              // DP接口
	SC_HAL_VO_DEV_INTF_MIPI,            // MIPI-DSI接口
	SC_HAL_VO_DEV_INTF_MAX,
} ENUM_SC_HAL_VO_DEV_INTF;

/**
* @note   DVP外接接口芯片拓展接口或外接显示驱动芯片驱动屏幕.
*/
typedef enum {
	/* DVP外接接口类型 */
	SC_HAL_VO_DEV_SUB_INTF_NONE               = 0,
	SC_HAL_VO_DEV_SUB_INTF_VGA,
	SC_HAL_VO_DEV_SUB_INTF_HDMI,
	SC_HAL_VO_DEV_SUB_INTF_BT656,
	SC_HAL_VO_DEV_SUB_INTF_BT1120,
	SC_HAL_VO_DEV_SUB_INTF_LCD_16BIT,
	SC_HAL_VO_DEV_SUB_INTF_LCD_24BIT,

	SC_HAL_VO_DEV_SUB_INTF_MAX,
} ENUM_SC_HAL_VO_DEV_SUB_INTF;

/**
* @note  MIPI TX支持的面板
*/
typedef enum {
	SC_HAL_VO_MIPITX_PANNEL_NONE        = 0,
	SC_HAL_VO_MIPITX_PANNEL_EK79007,
	SC_HAL_VO_MIPITX_PANNEL_MAX,
} ENUM_SC_HAL_VO_MIPI_TX_PANNEL;

/**
* @note  显示设备子参数
*/
typedef union {
	ENUM_SC_HAL_VO_DEV_SUB_INTF    e_sub_intf;    //当e_interface为 SC_HAL_VO_DEV_INTF_DVP时，此参数有效
	ENUM_SC_HAL_VO_MIPI_TX_PANNEL  e_mipi_pannel; //当e_interface为 SC_HAL_VO_DEV_INTF_MIPI时，此参数有效
} UNION_SC_HAL_VO_SUB_INSTANCE;

/**
* @note   接口时序典型模板.
*/
typedef enum {
	SC_HAL_VO_OUTPUT_PAL = 0,               /* PAL */
	SC_HAL_VO_OUTPUT_NTSC,                  /* NTSC */

	SC_HAL_VO_OUTPUT_1080P24,               /* 1920 x 1080 at 24 Hz. */
	SC_HAL_VO_OUTPUT_1080P25,               /* 1920 x 1080 at 25 Hz. */
	SC_HAL_VO_OUTPUT_1080P30,               /* 1920 x 1080 at 30 Hz. */
	SC_HAL_VO_OUTPUT_1080P50,               /* 1920 x 1080 at 50 Hz. */
	SC_HAL_VO_OUTPUT_1080P60,               /* 1920 x 1080 at 60 Hz. */
	SC_HAL_VO_OUTPUT_1080I48,               /* 1920 x 1080 at 48 Hz. */
	SC_HAL_VO_OUTPUT_1080I50,               /* 1920 x 1080 at 50 Hz. */
	SC_HAL_VO_OUTPUT_1080I60,               /* 1920 x 1080 at 60 Hz. */

	SC_HAL_VO_OUTPUT_720P25,                /* 1280 x  720 at 25 Hz. */
	SC_HAL_VO_OUTPUT_720P30,                /* 1280 x  720 at 30 Hz. */
	SC_HAL_VO_OUTPUT_720P50,                /* 1280 x  720 at 50 Hz. */
	SC_HAL_VO_OUTPUT_720P60,                /* 1280 x  720 at 60 Hz. */

	SC_HAL_VO_OUTPUT_576P50,                /* 720  x  576 at 50 Hz. */
	SC_HAL_VO_OUTPUT_480P60,                /* 720  x  480 at 60 Hz. */

	SC_HAL_VO_OUTPUT_800x600_60,            /* VESA 800 x 600 at 60 Hz */
	SC_HAL_VO_OUTPUT_1024x600_60,           /* 1024 x 600 at 60 Hz */
	SC_HAL_VO_OUTPUT_1280x1024_60,          /* VESA 1280 x 1024 at 60 Hz*/
	SC_HAL_VO_OUTPUT_1280x768_60,           /* 1280*768@60Hz VGA@60Hz*/
	SC_HAL_VO_OUTPUT_640x480_60,            /* VESA 640 x 480 at 60 Hz*/
	SC_HAL_VO_OUTPUT_3840x2160_60,          /* 3840x2160_60 */
	SC_HAL_VO_OUTPUT_800x480_60,            /* 800 x 480 at 60 Hz. */
	SC_HAL_VO_OUTPUT_USER                   /* 自定义时序. */
} ENUM_SC_HAL_VO_DEV_TIMING_TEMPLATE;

/**
* @note   视频层使用的像素格式.
*/
typedef enum {
	SC_HAL_VO_FMT_ARGB1555,         // argb1555
	SC_HAL_VO_FMT_RGB565,           // rgb565
	SC_HAL_VO_FMT_ARGB8888,         // argb8888
	SC_HAL_VO_FMT_YV12              // yv12
} ENUM_SC_HAL_VO_FMT;

/**
* @note   视频层支持的色域范围.
*/
typedef enum {
	SC_HAL_VO_CSC_BT709_LIMIT,      // bt.709 limited
	SC_HAL_VO_CSC_BT709_FULL,       // bt.709 full
	SC_HAL_VO_CSC_BT601_LIMIT,      // bt.601 limited
	SC_HAL_VO_CSC_BT601_FULL,       // bt.601 full
	SC_HAL_VO_CSC_DEFAULT           // default
} ENUM_SC_HAL_VO_CSC;

typedef enum {
	SC_HAL_VO_CHN_STATE_DISABLE         =  0,
	SC_HAL_VO_CHN_STATE_PLAYING,
	SC_HAL_VO_CHN_STATE_PAUSE,
	SC_HAL_VO_CHN_STATE_STEP,
} ENUM_SC_HAL_VO_CHN_STATE;

/**
* @note   显示固定内置图像.
*/
typedef enum {
	SC_HAL_VO_IMAGE_NULL,             // 不显示固定图像
	SC_HAL_VO_IMAGE_UBOOT_LOGO,       // 显示uboot启动图像，若无uboot启动图像，则显示start_logo.yuv文件
	SC_HAL_VO_IMAGE_START_LOGO,       // 显示start_logo.yuv图像，若文件不存在，则显示背景色
	SC_HAL_VO_IMAGE_BACKGROUND        // 显示background.yuv图像，若文件不存在，则显示背景色
} ENUM_SC_HAL_VO_IMAGE;

/**
* @note   矩阵区域描述结构
*/
typedef struct {
	SC_S32  x;         // 起始位置x坐标
	SC_S32  y;         // 起始位置y坐标
	SC_S32  w;         // 宽度
	SC_S32  h;         // 高度
} STRU_SC_HAL_VO_RECT;

/**
* @note   帧数据指针
*/
typedef struct {
	SC_VOID   *buffer;          // 虚拟地址，对齐后
	SC_VOID   *buffer_orign;    // 虚拟地址，对齐前，用于释放
	SC_VOID   *buffer_pa;       // 物理地址，对齐后
	SC_VOID   *buffer_pa_orign; // 物理地址，对齐前，用于释放
	SC_U32     length;          // 数据内容长度
} STRU_SC_HAL_VO_PANNEL;

/**
* @note   定义视频图像帧结构
*/
typedef struct {
	SC_U32                 frame_id;                     //  图像帧id
	ENUM_SC_HAL_VO_FMT     format;                       //  图像帧数据格式
	SC_S32                 frame_width;                  //  图像帧宽
	SC_S32                 frame_height;                 //  图像帧高
	STRU_SC_HAL_VO_RECT    roi;                          //  显示区域
	SC_S32                 luma_stride;                  //  y步幅
	SC_S32                 chroma_stride;                //  uv步幅
	SC_U32                 pannel_num;
	STRU_SC_HAL_VO_PANNEL  pannel[MAX_VO_PANNEL_NUM];    //  数据存放指针 当format = SC_HAL_VO_FMT_YV12时
	//  当format为rgb时，仅使用pannel[0]
	//  当format为yuv时，pannel[0]存放y
	//                   pannel[1]存放u
	//                   pannel[2]存放v
	SC_S32                 interlace_filed_flag;         //  帧interlace标志，0：非interlace帧;
	//                   1:顶场帧;
	//                   2:底场帧
	SC_U64                 pts;                          //  时间戳
	SC_VOID                *usr_data;                    //  存放用户数据
	SC_VOID                *priv_data;                   //  请勿填冲
} STRU_SC_HAL_VO_DISP_BUF;

/**
* @note   用户定制的显示时序.
*/
typedef struct {
	SC_BOOL   interlace_mod;         // 0：  p模式       1：i模式
	SC_S32    hpw;                   // 水平同步脉冲宽度
	SC_S32    hbp;                   // 水平消隐后肩宽度
	SC_S32    hdp;                   // 水平有效显示宽度
	SC_S32    hfp;                   // 水平消隐前肩宽度
	SC_S32    vpw;                   // 垂直同步脉冲宽度
	SC_S32    vbp;                   // 垂直消隐后肩宽度
	SC_S32    vdp;                   // 垂直有效显示宽度
	SC_S32    vfp;                   // 垂直消隐前肩宽度
	SC_FLOAT  fps;                   // 帧率
	SC_BOOL   de_polarity;           // de信号极性       0：高电平有效 1:低电平有效
	SC_BOOL   da_polarity;           // 数据信号极性 0：高电平有效 1:低电平有效
	SC_BOOL   clock_polarity;        // 时钟信号极性 0：高电平有效 1:低电平有效
	SC_BOOL   hsync_polarity;        // 水平同步信号极性 0：高电平有效 1:低电平有效
	SC_BOOL   vsync_polarity;        // 垂直同步信号极性 0：高电平有效 1:低电平有效
} STRU_SC_HAL_VO_SYNC_INFO;        //  same with disp_res_infor_t

/**
* @note   视频输出设备属性.
*/
typedef struct {
	SC_U32     bg_color;                              // 背景色，格式为RGB888
	STRU_SC_HAL_VO_SYNC_INFO     timing_customize;    // 客户定制时序
	ENUM_SC_HAL_VO_DEV_INTF      e_interface;         // 输出接口类型，仅支持DVP和MIPI
	UNION_SC_HAL_VO_SUB_INSTANCE u_sub_intance;       // 输出子接口
	ENUM_SC_HAL_VO_DEV_TIMING_TEMPLATE  e_timing_template; //时序模板
	SC_U32                       bit_count_per_channel ;
} STRU_SC_HAL_VO_DEV_ATTR;

/**
* @note   鼠标位置坐标.
*/
typedef struct {
	SC_U32    x;              // x坐标
	SC_U32    y;              // y坐标
} STRU_SC_HAL_VO_POS;

/**
* @note   视频层属性.
*/
typedef struct {
	SC_S32    width;               // 宽度
	SC_S32    height;              // 高度
	SC_S32    luma_stride;         // y步幅
	SC_S32    chroma_stride;       // uv步幅
	ENUM_SC_HAL_VO_FMT    format;  // 视频层图像格式
	SC_S32    fps;                 // 显示帧率
} STRU_SC_HAL_VO_LAYER_ATTR;

/**
* @note   视频层CSC.
*/
typedef struct {
	ENUM_SC_HAL_VO_CSC    input;   // 视频层输入CSC
	ENUM_SC_HAL_VO_CSC    output;  // 视频层输出CSC
} STRU_SC_HAL_VO_LAYER_CSC;

/**
* @note   视频通道属性.
*/
typedef struct {
	SC_U32    priority;            // 通道优先级
	STRU_SC_HAL_VO_RECT   rect;     // 通道位置
} STRU_SC_HAL_VO_CHN_ATTR;

/**
* @note   通道幅形比.
*/
typedef enum {
	SC_HAL_VO_ASPECT_RATIO_NONE,        /* 无幅形比，拉伸显示 */
	SC_HAL_VO_ASPECT_RATIO_AUTO,        /* 自动模式，按照帧显示*/
	SC_HAL_VO_ASPECT_RATIO_MANUAL,      /* 手动模式 */
	SC_HAL_VO_ASPECT_RATIO_MAX
} ENUM_SC_HAL_VO_ASPECT_RATIO;

/**
* @note   定义幅形比信息.
*/
typedef struct STRU_ASPECT_RATIO_S {
	ENUM_SC_HAL_VO_ASPECT_RATIO   e_mode;          /* 幅形比类型 */
	STRU_SC_HAL_VO_RECT           st_videoRect;    /* 幅形比视频区域 */
	SC_U32                        u32_bgColor;     /* 背景颜色, RGB 888 */
} STRU_SC_HAL_VO_ASPECT_RATIO_S;

/**
* @note   通道参数.
*/
typedef struct {
	STRU_SC_HAL_VO_ASPECT_RATIO_S st_AspectRatio;  /*  幅形比信息 */
} STRU_SC_VO_CHN_PARAM_S;

typedef SC_VOID *SC_HAL_VO_DEV_HANDLE;
typedef SC_VOID *SC_HAL_VO_LAYER_HANDLE;

//=============================================================================

// Global function definition

//=============================================================================

SC_S32 sc_hal_vo_dev_open(ENUM_SC_HAL_VO_DEV_ID e_dev_id);
SC_S32 sc_hal_vo_fd_close(SC_S32 fd);

/**
* @brief  使能视频输出设备.
* @param  e_dev_id  视频输出设备号.
* @retval 0 成功 , 其它 失败.
* @note   在使能设备前，需要调用sc_hal_vo_dev_attr_set()配置输出设备的属性.
*/
SC_S32 sc_hal_vo_dev_enable(SC_S32 fd);

/**
* @brief  禁用视频输出设备.
* @param  e_dev_id  视频输出设备号.
* @retval 0 成功 , 其它 失败.
* @note   在禁用设备前需禁用相关视频层.
*/
SC_S32 sc_hal_vo_dev_disable(SC_S32 fd);

/**
* @brief  配置视频输出设备的属性.
* @param  e_dev_id  视频输出设备号.
* @param  attr  视频输出设备属性结构体指针.
* @retval 0 成功 , 其它 失败.
* @note   视频输出设备属性需在sc_hal_vo_dev_enable()前设置
* @note   视频输出设备属性的使用说明请参考STRU_SC_HAL_VO_DEV_ATTR
*/
SC_S32 sc_hal_vo_dev_set_attr(SC_S32 fd, STRU_SC_HAL_VO_DEV_ATTR *attr);

/**
* @brief  获取视频输出设备的属性.
* @param  e_dev_id  视频输出设备号.
* @param  attr  视频输出设备属性结构体指针.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_dev_get_attr(SC_S32 fd, STRU_SC_HAL_VO_DEV_ATTR *attr);

/**
* @brief  显示固定图片为背景.
* @param  e_dev_id  视频输出设备号.
* @param  e_image   固定图片编号
* @retval 0 成功 , 其它 失败.
* @note   当设置为固定图片后，所有入队列的显示buffer将不被显示，建议用户将所有buffer出列
*/
SC_S32 sc_hal_vo_dev_set_background(SC_S32 fd, ENUM_SC_HAL_VO_IMAGE e_image);

/**
* @brief  设置输出设备csc色域.
* @param  e_dev_id  视频输出设备号.
* @param  e_csc     csc转换矩阵.
* @retval 0 成功 , 其它 失败.
* @note   该接口将会设置该视频设备输出的色域
*/
SC_S32 sc_hal_vo_dev_set_csc(SC_S32 fd, ENUM_SC_HAL_VO_CSC *e_csc);

/**
* @brief  获取输出设备csc色域.
* @param  e_dev_id  视频输出设备号.
* @param  e_csc     csc转换矩阵指针.
* @retval 0 sucess , others failed.
* @note   查询该视频设备输出的色域
*/
SC_S32 sc_hal_vo_dev_get_csc(SC_S32 fd, ENUM_SC_HAL_VO_CSC *e_csc);

/**
* @brief  加载鼠标图标文件.
* @param  e_dev_id  视频输出设备号.
* @param  file_name  图标文件.
* @retval 0 成功 , 其它 失败.
* @note   鼠标文件格式为ARGB888
*/
SC_S32 sc_hal_vo_dev_cfg_cursor_file(SC_S32 fd, SC_CHAR *file_name);

/**
* @brief  使能鼠标显示.
* @param  e_dev_id  视频输出设备号.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_dev_enable_cursor(SC_S32 fd);

/**
* @brief  禁用鼠标显示.
* @param  e_dev_id  视频输出设备号.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_dev_disable_cursor(SC_S32 fd);

/**
* @brief  设置鼠标坐标.
* @param  e_dev_id  视频输出设备号.
* @param  pos  x,y坐标.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_dev_set_cursor_pos(SC_S32 fd, STRU_SC_HAL_VO_POS *pos);

/**
* @brief  获取鼠标坐标.
* @param  e_dev_id  视频输出设备号.
* @param  pos  x,y坐标.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_dev_get_cursor_pos(SC_S32 fd, STRU_SC_HAL_VO_POS *pos);

SC_S32 sc_hal_vo_layer_open(ENUM_SC_HAL_VO_LAYER_ID e_layer_id);

/**
* @brief  使能视频层
* @param  e_layer_id  视频层编号
* @retval 0 成功 , 其它 失败.
* @note   使能前需保证视频层已配置属性
*/
SC_S32 sc_hal_vo_layer_enable(SC_S32 fd);

/**
* @brief  禁用视频层
* @param  e_layer_id  视频层编号
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_layer_disable(SC_S32 fd);

/**
* @brief  设置视频层属性.
* @param  e_layer_id  视频层编号
* @param  attr        视频层属性，详细请看STRU_SC_HAL_VO_LAYER_ATTR描述.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_layer_set_attr(SC_S32 fd, STRU_SC_HAL_VO_LAYER_ATTR *attr);

/**
* @brief  获取视频层属性.
* @param  e_layer_id  视频层编号
* @param  attr        视频层属性，详细请看STRU_SC_HAL_VO_LAYER_ATTR描述.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_layer_get_attr(SC_S32 fd, STRU_SC_HAL_VO_LAYER_ATTR *attr);

/**
* @brief  设置视频层csc色域.
* @param  e_layer_id  视频层编号
* @param  e_csc     csc转换矩阵.
* @retval 0 成功 , 其它 失败.
* @note   该接口将会设置向视频层输入数据的色域
*/
SC_S32 sc_hal_vo_layer_set_csc(SC_S32 fd, STRU_SC_HAL_VO_LAYER_CSC *csc);

/**
* @brief  获取视频层csc色域.
* @param  e_layer_id  视频层编号
* @param  e_csc     csc转换矩阵指针.
* @retval 0 sucess , others failed.
* @note   查询该视频层输入的色域
*/
SC_S32 sc_hal_vo_layer_get_csc(SC_S32 fd, STRU_SC_HAL_VO_LAYER_CSC *csc);

/**
* @brief  捕获指定视频层的图像.
* @param  e_layer_id  视频层编号
* @param  buffer  获取输出屁股木图像数据信息的指针.
* @retval 0 成功 , 其它 失败.
* @note   使用完成后，用户需使用sc_hal_vo_layer_release_disp_buf进行释放
*/
SC_S32 sc_hal_vo_layer_capture_disp_buf(SC_S32 fd, STRU_SC_HAL_VO_DISP_BUF *buffer);

/**
* @brief  释放图像数据.
* @param  e_layer_id  视频层编号
* @param  buffer  释放的输出屁股木图像数据信息的指针.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_layer_release_disp_buf(SC_S32 fd, STRU_SC_HAL_VO_DISP_BUF *buffer);

/**
* @brief  设置视频层位置.
* @param  e_layer_id  视频层编号
* @param  pos  x,y坐标.
* @retval 0 成功 , 其它 失败.
* @note   SC_HAL_VO_LAYER_ID_VIDEO_0    不支持
          SC_HAL_VO_LAYER_ID_OVERLAY_0  支持
*/
SC_S32 sc_hal_vo_layer_set_pos(SC_S32 fd, STRU_SC_HAL_VO_POS *pos);

SC_S32 sc_hal_vo_chn_open(ENUM_SC_HAL_VO_LAYER_ID e_layer_id, SC_U32 chn_id);

/**
* @brief  启用指定的视频输出通道.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @retval 0 成功 , 其它 失败.
* @note   请确认指定通道已配置属性和参数
*/
SC_S32 sc_hal_vo_chn_enable(SC_S32 fd);

/**
* @brief  禁用指定的视频输出通道.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_disable(SC_S32 fd);

/**
* @brief  配置通道的属性.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  attr        通道属性指针
* @retval 0 成功 , 其它 失败.
* @note   属性中优先级值越大，优先级越高，当各通道区域有重叠时，高优先级通道的图像覆盖
          低优先级的
*/
SC_S32 sc_hal_vo_chn_set_attr(SC_S32 fd, STRU_SC_HAL_VO_CHN_ATTR *attr);

/**
* @brief  查询通道的属性.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  attr        通道属性指针
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_get_attr(SC_S32 fd, STRU_SC_HAL_VO_CHN_ATTR *attr);

/**
* @brief  设置通道参数.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  para        通道参数指针
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_set_para(SC_S32 fd, STRU_SC_VO_CHN_PARAM_S *para);

/**
* @brief  获取通道参数.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  para        通道参数指针
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_get_para(SC_S32 fd, STRU_SC_VO_CHN_PARAM_S *para);

/**
* @brief  设置通道的显示区域.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  pos         通道显示区域
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_set_position(SC_S32 fd, STRU_SC_HAL_VO_RECT *pos);

/**
* @brief  获取通道的显示区域.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  pos         通道显示区域
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_get_position(SC_S32 fd, STRU_SC_HAL_VO_RECT *pos);

/**
* @brief  释放指定通道的图像.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  buffer      获取输出屁股木图像数据信息的指针.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_release_frame(SC_S32 fd, STRU_SC_HAL_VO_DISP_BUF *buffer);

/**
* @brief  往通道推送图像数据.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  buffer  视频数据帧指针.
* @retval 0 成功 , 其它 失败.
* @note   调用前须确保通道已经使能。
          用户需自行启线程调用sc_hal_vo_chn_wait_frame_ok()等待显示完成
          再调用sc_hal_vo_chn_dequeue()回收数据帧进行处理
*/
SC_S32 sc_hal_vo_chn_send_frame(SC_S32 fd, STRU_SC_HAL_VO_DISP_BUF *buffer, SC_S32 milli_sec);

/**
* @brief  从通道回收帧.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @param  buffer  视频数据帧指针.
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_get_frame(SC_S32 fd, STRU_SC_HAL_VO_DISP_BUF *buffer, SC_S32 milli_sec);

/**
* @brief  清除指定通道的frame.
* @param  fd          通道fd
* @param  clr_all     清除标志      true：清除所有视频帧， false: 保留一帧
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_clear_frame(SC_S32 fd, SC_BOOL clr_all);

/**
* @brief  暂停指定的通道.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_pause(SC_S32 fd);

/**
* @brief  恢复指定的通道.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_resume(SC_S32 fd);

/**
* @brief  单帧播放指定的通道.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_step(SC_S32 fd);

/**
* @brief  显示指定的通道.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_show(SC_S32 fd);

/**
* @brief  隐藏指定的通道.
* @param  e_layer_id  视频层编号
* @param  chan_id     通道编号 范围:       [0, MAX_VO_CHN_NUM]
* @retval 0 成功 , 其它 失败.
* @note
*/
SC_S32 sc_hal_vo_chn_hide(SC_S32 fd);
#ifdef __cplusplus
}
#endif

#endif
