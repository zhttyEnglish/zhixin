/**
 * @file     sc_comm_vgs.h
 * @brief    视频图形子系统模块的api参数定义
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
 * @date    2021-08-20 创建文件
 */
/**
 ********************************************************************************************************
 * Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 *             ALL RIGHTS RESERVED
 * Permission is hereby granted to licensees of BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 * to use or abstract this computer program for the sole purpose of implementing a product based on
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use,
 * or disseminate this computer program,whether in part or in whole, are granted.
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no representation or warranties
 * with respect to the performance of this computer program, and specifically disclaims
 * any responsibility for any damages, special or consequential, connected with the use of this program.
 * For details, see http://www.sgitg.sgcc.com.cn/
 ********************************************************************************************************
*/

#ifndef __SC_COMM_VGS_H__
#define __SC_COMM_VGS_H__

#include "sc_type.h"
#include "sc_common.h"
#include "sc_errno.h"
#include "sc_comm_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/* failure caused by malloc buffer */
#define SC_ERR_VGS_NOBUF         SC_DEF_ERR(SC_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define SC_ERR_VGS_BUF_EMPTY     SC_DEF_ERR(SC_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define SC_ERR_VGS_NULL_PTR      SC_DEF_ERR(SC_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define SC_ERR_VGS_ILLEGAL_PARAM SC_DEF_ERR(SC_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define SC_ERR_VGS_BUF_FULL      SC_DEF_ERR(SC_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define SC_ERR_VGS_SYS_NOTREADY  SC_DEF_ERR(SC_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define SC_ERR_VGS_NOT_SUPPORT   SC_DEF_ERR(SC_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define SC_ERR_VGS_NOT_PERMITTED SC_DEF_ERR(SC_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)

#define VGS_PRIVATE_DATA_LEN 6

typedef SC_S32 VGS_HANDLE;

typedef enum scVGS_COLOR_REVERT_MODE_E
{
    VGS_COLOR_REVERT_NONE = 0, /* Not revert */
    VGS_COLOR_REVERT_RGB,      /* Revert RGB */
    VGS_COLOR_REVERT_ALPHA,    /* Revert alpha */
    VGS_COLOR_REVERT_BOTH,     /* Revert RGB and alpha */
    VGS_COLOR_REVERT_BUTT
} VGS_COLOR_REVERT_MODE_E;

typedef struct scVGS_OSD_REVERT_S
{
    RECT_S stSrcRect;                          /* OSD color revert area */
    VGS_COLOR_REVERT_MODE_E enColorRevertMode; /* OSD color revert mode */
} VGS_OSD_REVERT_S;

typedef struct scVGS_TASK_ATTR_S
{
    VIDEO_FRAME_INFO_S stImgIn;  /* Input picture */
    VIDEO_FRAME_INFO_S stImgOut; /* Output picture */
    SC_U64 au64PrivateData[4];   /* RW; Private data of task */
    SC_U32 reserved;             /* RW; Debug information,state of current picture */
} VGS_TASK_ATTR_S;

typedef struct scVGS_DRAW_LINE_S
{
    POINT_S stStartPoint; /* Line start point */
    POINT_S stEndPoint;   /* Line end point */
    SC_U32 u32Thick;      /* RW; Width of line */
    SC_U32 u32Color;      /* RW; Range: [0,0xFFFFFF]; Color of line */
} VGS_DRAW_LINE_S;

typedef enum scVGS_COVER_TYPE_E
{
    COVER_RECT = 0,    /* Retangle cover */
    COVER_QUAD_RANGLE, /* Quadrangle cover */
    COVER_BUTT
} VGS_COVER_TYPE_E;

typedef struct scVGS_QUADRANGLE_COVER_S
{
    SC_BOOL bSolid;     /* Solid or hollow cover */
    SC_U32 u32Thick;    /* RW; Range: [2,8]; Thick of the hollow quadrangle */
    POINT_S stPoint[4]; /* Four points of the quadrangle */
} VGS_QUADRANGLE_COVER_S;

typedef struct scVGS_ADD_COVER_S
{
    VGS_COVER_TYPE_E enCoverType; /* Cover type */
    union
    {
        RECT_S stDstRect;                    /* The rectangle area */
        VGS_QUADRANGLE_COVER_S stQuadRangle; /* The quadrangle area */
    };

    SC_U32 u32Color; /* RW; Range: [0,0xFFFFFF]; Color of cover */
} VGS_ADD_COVER_S;

typedef struct scVGS_ADD_OSD_S
{
    RECT_S stRect;                /* Osd area */
    SC_U32 u32BgColor;            /* RW; Background color of osd, depends on pixel format of osd,
                                    ARGB8888:[0,0xFFFFFFFF], ARGB4444:[0,0xFFFF], ARGB1555:[0,0x1FFF] */
    PIXEL_FORMAT_E enPixelFmt;    /* Pixel format of osd */
    SC_U64 u64PhyAddr;            /* RW; Physical address of osd */
    SC_U32 u32Stride;             /* RW; Range: [32,8192]; Stride of osd */
    SC_U32 u32BgAlpha;            /* RW; Range: [0,255]; Background alpha of osd */
    SC_U32 u32FgAlpha;            /* RW; Range: [0,255]; Foreground alpha of osd */
    SC_BOOL bOsdRevert;           /* RW; Enable osd color revert */
    VGS_OSD_REVERT_S stOsdRevert; /* Osd color revert information */
    SC_U16 u16ColorLUT[2];
} VGS_ADD_OSD_S;

typedef enum scVGS_MOSAIC_BLK_SIZE_E
{
    VGS_MOSAIC_BLK_SIZE_8 = 0, /* block size 8*8 of MOSAIC */
    VGS_MOSAIC_BLK_SIZE_16,    /* block size 16*16 of MOSAIC */
    VGS_MOSAIC_BLK_SIZE_32,    /* block size 32*32 of MOSAIC */
    VGS_MOSAIC_BLK_SIZE_64,    /* block size 64*64 of MOSAIC */
    VGS_MOSAIC_BLK_SIZE_BUTT
} VGS_MOSAIC_BLK_SIZE_E;

typedef struct scVGS_ADD_MOSAIC_S
{
    RECT_S stRect;                /* Mosaic area */
    SC_BOOL bBgAlpha;             /* RW; Enable Background alpha */
    SC_U32 u32BgAlpha;            /* RW; Range: [0,255]; Background alpha of mosaic */
    VGS_MOSAIC_BLK_SIZE_E  enBlkSize;
} VGS_ADD_MOSAIC_S;

typedef enum scVGS_SCLCOEF_MODE_E
{
    VGS_SCLCOEF_NORMAL = 0, /* normal scale coefficient */
    VGS_SCLCOEF_TAP2 = 1,   /* scale coefficient of 2 tap */
    VGS_SCLCOEF_TAP4 = 2,   /* scale coefficient of 4 tap */
    VGS_SCLCOEF_TAP6 = 3,   /* scale coefficient of 6 tap */
    VGS_SCLCOEF_TAP8 = 4,   /* scale coefficient of 8 tap */
    VGS_SCLCOEF_BUTT
} VGS_SCLCOEF_MODE_E;

typedef struct scVGS_GET_LUMA_DATA_S
{
    RECT_S     *pstRect;                 /*luma rect*/
    SC_U64     *pu64LumaRet;             /*luma result*/
    SC_U32 u32ArraySize;                 /*data rect*/
} VGS_GET_LUMA_DATA_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_COMM_VGS_H__ */
