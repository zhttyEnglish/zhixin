/**
 * @file     sc_comm_dis.h
 * @brief    Digital Image Stabilisation 数字防抖模块定义
 * @version  1.0.0
 * @since    1.0.0
 * @author  石丽月<shiliyue@sgitg.sgcc.com.cn>
 * @date    2022-01-10 创建文件
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

#ifndef __SC_COMM_DIS_H__
#define __SC_COMM_DIS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/* failure caused by malloc buffer */
#define SC_ERR_DIS_NOBUF         SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define SC_ERR_DIS_BUF_EMPTY     SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define SC_ERR_DIS_NULL_PTR      SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define SC_ERR_DIS_ILLEGAL_PARAM SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define SC_ERR_DIS_BUF_FULL      SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define SC_ERR_DIS_SYS_NOTREADY  SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define SC_ERR_DIS_NOT_SUPPORT   SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define SC_ERR_DIS_NOT_PERMITTED SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define SC_ERR_DIS_BUSY          SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define SC_ERR_DIS_INVALID_CHNID SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define SC_ERR_DIS_CHN_UNEXIST   SC_DEF_ERR(SC_ID_DIS, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)

/* Different mode of DIS */
typedef enum scDIS_MODE_E
{
    DIS_MODE_4_DOF_GME = 0, /* Only use with GME in 4 dof  */
    DIS_MODE_6_DOF_GME, /* Only use with GME in 6 dof  */
    DIS_MODE_GYRO, /* Only use with gryo in 6 dof  */
    DIS_MODE_DOF_BUTT,
} DIS_MODE_E;

/* The motion level of camera */
typedef enum scDIS_MOTION_LEVEL_E
{
    DIS_MOTION_LEVEL_LOW = 0, /* Low motion level */
    DIS_MOTION_LEVEL_NORMAL, /* Normal motion level */
    DIS_MOTION_LEVEL_HIGH, /* High motion level */
    DIS_MOTION_LEVEL_BUTT
} DIS_MOTION_LEVEL_E;

/* Different product type used DIS */
typedef enum scDIS_PDT_TYPE_E
{
    DIS_PDT_TYPE_IPC = 0, /* IPC product type */
    DIS_PDT_TYPE_DV, /* DV product type */
    DIS_PDT_TYPE_DRONE, /* DRONE product type */
    DIS_PDT_TYPE_BUTT
} DIS_PDT_TYPE_E;

/* The Attribute of DIS */
typedef struct scDIS_ATTR_S
{
    SC_BOOL bEnable; /* RW; DIS enable */
    SC_BOOL bGdcBypass; /* RW; gdc correction process , DIS = GME&GDC correction */
    SC_U32 u32MovingSubjectLevel; /* RW; Range:[0,6]; Moving Subject level */
    SC_S32 s32RollingShutterCoef; /* RW; Range:[0,1000]; Rolling shutter coefficients */
    SC_S32 s32Timelag; /* RW; Range:[-2000000,2000000]; Timestamp delay between Gyro and Frame PTS */
    SC_U32 u32ViewAngle; /* Reserved */
    SC_U32 u32HorizontalLimit; /* RW; Range:[0,1000]; Parameter to limit horizontal drift by large foreground */
    SC_U32 u32VerticalLimit; /* RW; Range:[0,1000]; Parameter to limit vertical drift by large foreground */
    SC_BOOL bStillCrop; /* RW; The stabilization will be not working ,but the output image still be cropped */
    SC_U32  u32Strength; /* RW. Range:[0,1024]; The DIS strength for different light, Only valid for MODE_GYRO */
} DIS_ATTR_S;

/* The Config of DIS */
typedef struct scDIS_CONFIG_S
{
    DIS_MODE_E enMode; /* RW; DIS Mode */
    DIS_MOTION_LEVEL_E enMotionLevel; /* RW; DIS Motion level of the camera */
    DIS_PDT_TYPE_E enPdtType; /* RW; DIS product type */
    SC_U32 u32BufNum; /* RW; Range:[5,10]; Buf num for DIS */
    SC_U32 u32CropRatio; /* RW; Range:[50,98]; Crop ratio of output image */
    SC_U32 u32FrameRate; /* RW; Range: Hi3559AV100 = (0, 120] | Hi3519AV100 = (0, 120] | Hi3516CV500 = (0, 60] |
                            Hi3516DV300 = (0, 60] | Hi3559V200 = (0, 60] | Hi3556V200 = (0, 60] . */
    SC_U32 u32GyroOutputRange; /* RW; Range:[0,200000]; The range of Gyro output in degree */
    SC_U32 u32GyroDataBitWidth; /* RW; Range:[0,32]; The bits used for gyro angular velocity output */
    SC_BOOL bCameraSteady; /* RW; The camera is steady or not */
    SC_BOOL bScale; /* RW; Scale output image or not */
} DIS_CONFIG_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_COMM_DIS_H__ */
