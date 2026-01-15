/**
 * @file     sc_comm_region.h
 * @brief    region 模块定义
 * @version  1.0.0
 * @since    1.0.0
 * @author  石丽月<shiliyue@sgitg.sgcc.com.cn>
 * @date    2021-09-03 创建文件
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

#ifndef __SC_COMM_REGION_H__
#define __SC_COMM_REGION_H__

#include "sc_common.h"
#include "sc_comm_video.h"
#include "sc_errno.h"
#include "sc_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define RGN_COLOR_LUT_NUM      2
#define RGN_MAX_BMP_UPDATE_NUM 16
#define RGN_BATCHHANDLE_MAX    24

typedef SC_U32 RGN_HANDLE;
typedef SC_U32 RGN_HANDLEGROUP;

/* type of video regions */
typedef enum scRGN_TYPE_E
{
    OVERLAY_RGN = 0, /* video overlay region */
    COVER_RGN,
    COVEREX_RGN,
    OVERLAYEX_RGN,
    MOSAIC_RGN,
    RGN_BUTT
} RGN_TYPE_E;

typedef enum scINVERT_COLOR_MODE_E
{
    LESSTHAN_LUM_THRESH = 0, /* the lum of the video is less than the lum threshold which is set by u32LumThresh  */
    MORETHAN_LUM_THRESH,     /* the lum of the video is more than the lum threshold which is set by u32LumThresh  */
    INVERT_COLOR_BUTT
} INVERT_COLOR_MODE_E;

typedef struct scOVERLAY_QP_INFO_S
{
    SC_BOOL bAbsQp;
    SC_S32 s32Qp;
    SC_BOOL bQpDisable;
} OVERLAY_QP_INFO_S;

typedef struct scOVERLAY_INVERT_COLOR_S
{
    SIZE_S stInvColArea;  // It must be multipe of 16 but not more than 64.
    SC_U32 u32LumThresh;  // The threshold to decide whether invert the OSD's color or not.
    INVERT_COLOR_MODE_E enChgMod;
    SC_BOOL bInvColEn;  // The switch of inverting color.
} OVERLAY_INVERT_COLOR_S;

typedef enum scATTACH_DEST_E
{
    ATTACH_JPEG_MAIN = 0,
    ATTACH_JPEG_MPF0,
    ATTACH_JPEG_MPF1,
    ATTACH_JPEG_BUTT
} ATTACH_DEST_E;

typedef struct scOVERLAY_ATTR_S
{
    /* bitmap pixel format,now only support ARGB1555 or ARGB4444 ARGB8888*/
    PIXEL_FORMAT_E enPixelFmt;

    /* background color, pixel format depends on "enPixelFmt" */
    SC_U32 u32BgColor;

    /* region size,W:[RGN_OVERLAY_MIN_WIDTH,RGN_OVERLAY_MAX_WIDTH],align:8,H:[2,RGN_OVERLAY_MAX_HEIGHT],align:2 */
    SIZE_S stSize;
    SC_U32 u32CanvasNum;
} OVERLAY_ATTR_S;

typedef struct scOVERLAY_CHN_ATTR_S
{
    /* X:[0,OVERLAY_MAX_X_VENC],align:2,Y:[0,OVERLAY_MAX_Y_VENC],align:2 */
    POINT_S stPoint;

    /* background an foreground transparence when pixel format is ARGB1555
      * the pixel format is ARGB1555,when the alpha bit is 1 this alpha is value!
      * range:[0,128]
      */
    SC_U32 u32FgAlpha;

    /* background an foreground transparence when pixel format is ARGB1555
      * the pixel format is ARGB1555,when the alpha bit is 0 this alpha is value!
      * range:[0,128]
      */
    SC_U32 u32BgAlpha;

    SC_U32 u32Layer; /* OVERLAY region layer range:[0,7] */

    OVERLAY_QP_INFO_S stQpInfo;

    OVERLAY_INVERT_COLOR_S stInvertColor;

    ATTACH_DEST_E enAttachDest;

    SC_U16 u16ColorLUT[RGN_COLOR_LUT_NUM];
} OVERLAY_CHN_ATTR_S;

typedef enum scRGN_AREA_TYPE_E
{
    AREA_RECT = 0,
    AREA_QUAD_RANGLE,
    AREA_BUTT
} RGN_AREA_TYPE_E;

typedef enum scRGN_COORDINATE_E
{
    RGN_ABS_COOR = 0, /* Absolute coordinate */
    RGN_RATIO_COOR    /* Ratio coordinate */
} RGN_COORDINATE_E;

typedef struct scRGN_QUADRANGLE_S
{
    SC_BOOL bSolid;     /* whether solid or dashed quadrangle */
    SC_U32 u32Thick;    /* Line Width of quadrangle, valid when dashed quadrangle */
    POINT_S stPoint[4]; /* points of quadrilateral */
} RGN_QUADRANGLE_S;

typedef struct scCOVER_CHN_ATTR_S
{
    RGN_AREA_TYPE_E enCoverType; /* rect or arbitary quadrilateral COVER */
    union
    {
        RECT_S stRect;                 /* config of rect */
        RGN_QUADRANGLE_S stQuadRangle; /* config of arbitary quadrilateral COVER */
    };
    SC_U32 u32Color;
    SC_U32 u32Layer;               /* COVER region layer */
    RGN_COORDINATE_E enCoordinate; /* ratio coordiante or abs coordinate */
} COVER_CHN_ATTR_S;

typedef struct scCOVEREX_CHN_ATTR_S
{
    RGN_AREA_TYPE_E enCoverType; /* rect or arbitary quadrilateral COVER */
    union
    {
        RECT_S stRect;                 /* config of rect */
        RGN_QUADRANGLE_S stQuadRangle; /* config of arbitary quadrilateral COVER */
    };
    SC_U32 u32Color;
    SC_U32 u32Layer; /* COVEREX region layer range */
} COVEREX_CHN_ATTR_S;

typedef enum scMOSAIC_BLK_SIZE_E
{
    MOSAIC_BLK_SIZE_8 = 0, /* block size 8*8 of MOSAIC */
    MOSAIC_BLK_SIZE_16,    /* block size 16*16 of MOSAIC */
    MOSAIC_BLK_SIZE_32,    /* block size 32*32 of MOSAIC */
    MOSAIC_BLK_SIZE_64,    /* block size 64*64 of MOSAIC */
    MOSAIC_BLK_SIZE_BUTT
} MOSAIC_BLK_SIZE_E;

typedef struct scMOSAIC_CHN_ATTR_S
{
    RECT_S stRect;               /* location of MOSAIC */
    MOSAIC_BLK_SIZE_E enBlkSize; /* block size of MOSAIC */
    SC_U32 u32Layer;             /* MOSAIC region layer range:[0,3] */
} MOSAIC_CHN_ATTR_S;

typedef struct scOVERLAYEX_COMM_ATTR_S
{
    PIXEL_FORMAT_E enPixelFmt;

    /* background color, pixel format depends on "enPixelFmt" */
    SC_U32 u32BgColor;

    /* region size,W:[2,RGN_OVERLAY_MAX_WIDTH],align:2,H:[2,RGN_OVERLAY_MAX_HEIGHT],align:2 */
    SIZE_S stSize;
    SC_U32 u32CanvasNum;
} OVERLAYEX_ATTR_S;

typedef struct scOVERLAYEX_CHN_ATTR_S
{
    /* X:[0,RGN_OVERLAY_MAX_X],align:2,Y:[0,RGN_OVERLAY_MAX_Y],align:2 */
    POINT_S stPoint;

    /* background an foreground transparence when pixel format is ARGB1555
      * the pixel format is ARGB1555,when the alpha bit is 1 this alpha is value!
      * range:[0,255]
      */
    SC_U32 u32FgAlpha;

    /* background an foreground transparence when pixel format is ARGB1555
      * the pixel format is ARGB1555,when the alpha bit is 0 this alpha is value!
      * range:[0,255]
      */
    SC_U32 u32BgAlpha;

    SC_U32 u32Layer; /* OVERLAYEX region layer range:[0,15] */

    SC_U16 u16ColorLUT[RGN_COLOR_LUT_NUM];
} OVERLAYEX_CHN_ATTR_S;

typedef union scRGN_ATTR_U
{
    OVERLAY_ATTR_S stOverlay;     /* attribute of overlay region */
    OVERLAYEX_ATTR_S stOverlayEx; /* attribute of overlayex region */
} RGN_ATTR_U;

typedef union scRGN_CHN_ATTR_U
{
    OVERLAY_CHN_ATTR_S stOverlayChn;     /* attribute of overlay region */
    COVER_CHN_ATTR_S stCoverChn;         /* attribute of cover region */
    COVEREX_CHN_ATTR_S stCoverExChn;     /* attribute of coverex region */
    OVERLAYEX_CHN_ATTR_S stOverlayExChn; /* attribute of overlayex region */
    MOSAIC_CHN_ATTR_S stMosaicChn;       /* attribute of mosic region */
} RGN_CHN_ATTR_U;

/* attribute of a region */
typedef struct scRGN_ATTR_S
{
    RGN_TYPE_E enType; /* region type */
    RGN_ATTR_U unAttr; /* region attribute */
} RGN_ATTR_S;

/* attribute of a region */
typedef struct scRGN_CHN_ATTR_S
{
    SC_BOOL bShow;
    RGN_TYPE_E enType;        /* region type */
    RGN_CHN_ATTR_U unChnAttr; /* region attribute */
} RGN_CHN_ATTR_S;

typedef struct scRGN_BMP_UPDATE_S
{
    POINT_S stPoint;
    BITMAP_S stBmp;
    SC_U32 u32Stride;
} RGN_BMP_UPDATE_S;

typedef struct scRGN_BMP_UPDATE_CFG_S
{
    SC_U32 u32BmpCnt;
    RGN_BMP_UPDATE_S astBmpUpdate[RGN_MAX_BMP_UPDATE_NUM];
} RGN_BMP_UPDATE_CFG_S;

typedef struct scRGN_CANVAS_INFO_S
{
    SC_U64 u64PhyAddr;
    SC_U64 u64VirtAddr;
    SIZE_S stSize;
    SC_U32 u32Stride;
    PIXEL_FORMAT_E enPixelFmt;
} RGN_CANVAS_INFO_S;

/* PingPong buffer change when set attr, it needs to remap memory in mpi interface */
#define SC_NOTICE_RGN_BUFFER_CHANGE SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_NOTICE, SC_SUCCESS)

/* invlalid device ID */
#define SC_ERR_RGN_INVALID_DEVID SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* invlalid channel ID */
#define SC_ERR_RGN_INVALID_CHNID SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define SC_ERR_RGN_ILLEGAL_PARAM SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* channel exists */
#define SC_ERR_RGN_EXIST SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* UN exist */
#define SC_ERR_RGN_UNEXIST SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* using a NULL point */
#define SC_ERR_RGN_NULL_PTR SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define SC_ERR_RGN_NOT_CONFIG SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define SC_ERR_RGN_NOT_SUPPORT SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* operation is not permitted ,eg, try to change stati attribute */
#define SC_ERR_RGN_NOT_PERM SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* failure caused by malloc memory */
#define SC_ERR_RGN_NOMEM SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define SC_ERR_RGN_NOBUF SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define SC_ERR_RGN_BUF_EMPTY SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define SC_ERR_RGN_BUF_FULL SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* bad address, eg. used for copy_from_user & copy_to_user */
#define SC_ERR_RGN_BADADDR SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
/* resource is busy, eg. destroy a venc chn without unregistering it */
#define SC_ERR_RGN_BUSY SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)

/* System is not ready,maybe not initialed or loaded.
 * Returning the error code when opening a device file failed.
 */
#define SC_ERR_RGN_NOTREADY SC_DEF_ERR(SC_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* __SC_COMM_REGION_H__ */
