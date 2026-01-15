/**
 * @file     sc_comm_gdc.h
 * @brief    gdc 模块定义
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

#include "sc_type.h"
#include "sc_common.h"
#include "sc_errno.h"
#include "sc_comm_video.h"

#ifndef __SC_COMM_GDC_H__
#define __SC_COMM_GDC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/* failure caused by malloc buffer */
#define SC_ERR_GDC_NOBUF         SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define SC_ERR_GDC_BUF_EMPTY     SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define SC_ERR_GDC_NULL_PTR      SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define SC_ERR_GDC_ILLEGAL_PARAM SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define SC_ERR_GDC_BUF_FULL      SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define SC_ERR_GDC_SYS_NOTREADY  SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define SC_ERR_GDC_NOT_SUPPORT   SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define SC_ERR_GDC_NOT_PERMITTED SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define SC_ERR_GDC_BUSY          SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define SC_ERR_GDC_INVALID_CHNID SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define SC_ERR_GDC_CHN_UNEXIST   SC_DEF_ERR(SC_ID_GDC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)

#define FISHEYE_MAX_REGION_NUM   4
#define FISHEYE_LMFCOEF_NUM      128
#define GDC_PMFCOEF_NUM          9

typedef SC_S32 GDC_HANDLE;

typedef struct scGDC_TASK_ATTR_S
{
    VIDEO_FRAME_INFO_S stImgIn; /* Input picture */
    VIDEO_FRAME_INFO_S stImgOut; /* Output picture */
    SC_U64 au64privateData[4]; /* RW; Private data of task */
    SC_U64 reserved; /* RW; Debug information,state of current picture */
} GDC_TASK_ATTR_S;

/* Mount mode of device */
typedef enum scFISHEYE_MOUNT_MODE_E
{
    FISHEYE_DESKTOP_MOUNT = 0, /* Desktop mount mode */
    FISHEYE_CEILING_MOUNT = 1, /* Ceiling mount mode */
    FISHEYE_WALL_MOUNT = 2, /* wall mount mode */

    FISHEYE_MOUNT_MODE_BUTT
} FISHEYE_MOUNT_MODE_E;

/* View mode of client */
typedef enum scFISHEYE_VIEW_MODE_E
{
    FISHEYE_VIEW_360_PANORAMA = 0, /* 360 panorama mode of gdc correction */
    FISHEYE_VIEW_180_PANORAMA = 1, /* 180 panorama mode of gdc correction */
    FISHEYE_VIEW_NORMAL = 2, /* normal mode of gdc correction */
    FISHEYE_NO_TRANSFORMATION = 3, /* no gdc correction */

    FISHEYE_VIEW_MODE_BUTT
} FISHEYE_VIEW_MODE_E;

/* Fisheye region correction attribute */
typedef struct scFISHEYE_REGION_ATTR_S
{
    FISHEYE_VIEW_MODE_E enViewMode; /* RW; Range: [0, 3];gdc view mode */
    SC_U32 u32InRadius; /* RW; inner radius of gdc correction region */
    SC_U32 u32OutRadius; /* RW; out radius of gdc correction region */
    SC_U32 u32Pan; /* RW; Range: [0, 360] */
    SC_U32 u32Tilt; /* RW; Range: [0, 360] */
    SC_U32 u32HorZoom; /* RW; Range: [1, 4095] */
    SC_U32 u32VerZoom; /* RW; Range: [1, 4095] */
    RECT_S stOutRect; /* RW; out Imge rectangle attribute */
} FISHEYE_REGION_ATTR_S;

typedef struct scFISHEYE_REGION_ATTR_EX_S
{
    FISHEYE_VIEW_MODE_E enViewMode; /* RW; Range: [0, 3];gdc view mode */
    SC_U32 u32InRadius; /* RW; inner radius of gdc correction region */
    SC_U32 u32OutRadius; /* RW; out radius of gdc correction region */
    SC_U32 u32X; /* RW; Range: [0, 4608] */
    SC_U32 u32Y; /* RW; Range: [0, 3456] */
    SC_U32 u32HorZoom; /* RW; Range: [1, 4095] */
    SC_U32 u32VerZoom; /* RW; Range: [1, 4095] */
    RECT_S stOutRect; /* RW; out Imge rectangle attribute */
} FISHEYE_REGION_ATTR_EX_S;

/* Fisheye all regions correction attribute */
typedef struct scFISHEYE_ATTR_S
{
    SC_BOOL bEnable; /* RW; Range: [0, 1];whether enable fisheye correction or not */
    SC_BOOL bLMF; /* RW; Range: [0, 1];whether gdc len's LMF coefficient is from user config or
                     from default linear config */
    SC_BOOL bBgColor; /* RW; Range: [0, 1];whether use background color or not */
    SC_U32 u32BgColor; /* RW; Range: [0,0xffffff];the background color RGB888 */

    SC_S32 s32HorOffset; /* RW; Range: [-511, 511];the horizontal offset between image center and
                            physical center of len */
    SC_S32 s32VerOffset; /* RW; Range: [-511, 511]; the vertical offset between image center and
                            physical center of len */

    SC_U32 u32TrapezoidCoef; /* RW; Range: [0, 32];strength coefficient of trapezoid correction */
    SC_S32 s32FanStrength; /* RW; Range: [-760, 760];strength coefficient of fan correction */

    FISHEYE_MOUNT_MODE_E enMountMode; /* RW; Range: [0, 2];gdc mount mode */

    SC_U32 u32RegionNum; /* RW; Range: [1, 4]; gdc correction region number */
    FISHEYE_REGION_ATTR_S astFishEyeRegionAttr[FISHEYE_MAX_REGION_NUM]; /* RW; attribution of gdc correction region */
} FISHEYE_ATTR_S;

typedef struct scFISHEYE_ATTR_EX_S
{
    SC_BOOL bEnable; /* RW; Range: [0, 1];whether enable fisheye correction or not */
    SC_BOOL bLMF; /* RW; Range: [0, 1];whether gdc len's LMF coefficient is from user config or
                     from default linear config */
    SC_BOOL bBgColor; /* RW; Range: [0, 1];whether use background color or not */
    SC_U32 u32BgColor; /* RW; Range: [0,0xffffff];the background color RGB888 */

    SC_S32 s32HorOffset; /* RW; Range: [-511, 511];the horizontal offset between image center and
                            physical center of len */
    SC_S32 s32VerOffset; /* RW; Range: [-511, 511]; the vertical offset between image center and
                            physical center of len */

    SC_U32 u32TrapezoidCoef; /* RW; Range: [0, 32];strength coefficient of trapezoid correction */
    SC_S32 s32FanStrength; /* RW; Range: [-760, 760];strength coefficient of fan correction */

    FISHEYE_MOUNT_MODE_E enMountMode; /* RW; Range: [0, 2];gdc mount mode */

    SC_U32 u32RegionNum; /* RW; Range: [1, 4]; gdc correction region number */
    /* RW; attribution of gdc correction region */
    FISHEYE_REGION_ATTR_EX_S astFishEyeRegionAttr[FISHEYE_MAX_REGION_NUM];
} FISHEYE_ATTR_EX_S;

/* Spread correction attribute */
typedef struct scSPREAD_ATTR_S
{
    SC_BOOL bEnable; /* RW; Range: [0, 1];whether enable spread or not,
                        When spread on,ldc DistortionRatio range should be [0, 500] */
    SC_U32 u32SpreadCoef; /* RW; Range: [0, 18];strength coefficient of spread correction */
    SIZE_S stDestSize; /* RW; dest size of spread */
} SPREAD_ATTR_S;

/* Fisheye Job Config */
typedef struct scFISHEYE_JOB_CONFIG_S
{
    SC_U64 u64LenMapPhyAddr; /* LMF coefficient Physic Addr */
} FISHEYE_JOB_CONFIG_S;

/* Fisheye Config */
typedef struct scFISHEYE_CONFIG_S
{
    SC_U16 au16LMFCoef[FISHEYE_LMFCOEF_NUM]; /* RW;  LMF coefficient of gdc len */
} FISHEYE_CONFIG_S;

/* Gdc PMF Attr */
typedef struct scGDC_PMF_ATTR_S
{
    SC_S64 as64PMFCoef[GDC_PMFCOEF_NUM]; /* W;  PMF coefficient of gdc */
} GDC_PMF_ATTR_S;

/* Gdc FISHEYE POINT QUERY Attr */
typedef struct scGDC_FISHEYE_POINT_QUERY_ATTR_S
{
    SC_U32 u32RegionIndex;
    FISHEYE_ATTR_S *pstFishEyeAttr;
    SC_U16 au16LMF[FISHEYE_LMFCOEF_NUM];
} GDC_FISHEYE_POINT_QUERY_ATTR_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_COMM_GDC_H__ */
