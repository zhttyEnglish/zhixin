/*
 * @file     sc_comm_vb.h
 * @brief    vb相关
 * @version  1.0.1
 * @since    1.0.0
 * @author
 */

/*
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

#ifndef __SC_COMM_VB_H__
#define __SC_COMM_VB_H__

#include "sc_type.h"
#include "sc_errno.h"
#include "sc_debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define VB_INVALID_POOLID              (-1U)
#define VB_INVALID_HANDLE              (-1U)

//todo: hal_vb.h redefine
#ifndef VB_MAX_COMM_POOLS
#define VB_MAX_COMM_POOLS              16
#define VB_MAX_MOD_COMM_POOLS          16
#endif

/* user ID for VB */
#define VB_MAX_USER                    VB_UID_BUTT

typedef enum scVB_UID_E
{
    VB_UID_VI = 0,
    VB_UID_VO = 1,
    VB_UID_VGS = 2,
    VB_UID_VENC = 3,
    VB_UID_VDEC = 4,
    VB_UID_H265E = 5,
    VB_UID_H264E = 6,
    VB_UID_JPEGE = 7,
    VB_UID_H264D = 8,
    VB_UID_JPEGD = 9,
    VB_UID_VPSS = 10,
    VB_UID_DIS = 11,
    VB_UID_USER = 12,
    VB_UID_PCIV = 13,
    VB_UID_AI = 14,
    VB_UID_AENC = 15,
    VB_UID_RC = 16,
    VB_UID_VFMW = 17,
    VB_UID_GDC = 18,
    VB_UID_AVS = 19,
    VB_UID_DPU_RECT = 20,
    VB_UID_DPU_MATCH = 21,
    VB_UID_MCF = 22,
    VB_UID_RGN = 23,
    VB_UID_BUTT = 24,
} VB_UID_E;

//todo: hal_vb.h redefine
#ifndef POOL_OWNER_COMMON
/* Generall common pool use this owner id, module common pool use VB_UID as owner id */
#define POOL_OWNER_COMMON              -1

/* Private pool use this owner id */
#define POOL_OWNER_PRIVATE             -2
#endif

typedef SC_U32 VB_POOL;
typedef SC_U32 VB_BLK;

#define RESERVE_MMZ_NAME               "window"

typedef enum scVB_REMAP_MODE_E
{
    VB_REMAP_MODE_NONE = 0, /* no remap */
    VB_REMAP_MODE_NOCACHE = 1, /* no cache remap */
    VB_REMAP_MODE_CACHED = 2, /* cache remap, if you use this mode, you should flush cache by yourself */
    VB_REMAP_MODE_BUTT
} VB_REMAP_MODE_E;

typedef struct scVB_POOL_CONFIG_S
{
    SC_U64 u64BlkSize;
    SC_U32 u32BlkCnt;
    VB_REMAP_MODE_E enRemapMode;
    SC_CHAR acMmzName[MAX_MMZ_NAME_LEN];
} VB_POOL_CONFIG_S;

typedef struct scVB_CONFIG_S
{
    SC_U32 u32MaxPoolCnt;
    VB_POOL_CONFIG_S astCommPool[VB_MAX_COMM_POOLS];
} VB_CONFIG_S;

typedef struct scVB_POOL_STATUS_S
{
    SC_U32 bIsCommPool;
    SC_U32 u32BlkCnt;
    SC_U32 u32FreeBlkCnt;
} VB_POOL_STATUS_S;

//todo: hal_vb.h redefine
#ifndef VB_SUPPLEMENT_JPEG_MASK
#define VB_SUPPLEMENT_JPEG_MASK        0x1
#define VB_SUPPLEMENT_ISPINFO_MASK     0x2
#define VB_SUPPLEMENT_MOTION_DATA_MASK 0x4
#define VB_SUPPLEMENT_DNG_MASK         0x8
#endif

typedef struct scVB_SUPPLEMENT_CONFIG_S
{
    SC_U32 u32SupplementConfig;
} VB_SUPPLEMENT_CONFIG_S;

#define SC_ERR_VB_NULL_PTR             SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define SC_ERR_VB_NOMEM                SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define SC_ERR_VB_NOBUF                SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define SC_ERR_VB_UNEXIST              SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define SC_ERR_VB_ILLEGAL_PARAM        SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define SC_ERR_VB_NOTREADY             SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define SC_ERR_VB_BUSY                 SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define SC_ERR_VB_NOT_PERM             SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define SC_ERR_VB_SIZE_NOT_ENOUGH      SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_SIZE_NOT_ENOUGH)

#define SC_ERR_VB_2MPOOLS              SC_DEF_ERR(SC_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_BUTT + 1)

#define SC_TRACE_VB(level, fmt, ...)                                                                         \
    do {                                                                                                     \
        SC_TRACE(level, SC_ID_VB, "[Func]:%s [Line]:%d [Info]:" fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_COMM_VB_H_ */

