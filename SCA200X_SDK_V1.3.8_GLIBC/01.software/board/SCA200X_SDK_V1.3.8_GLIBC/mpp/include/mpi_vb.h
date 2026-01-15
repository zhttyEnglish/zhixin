/*
 * @file     mpi_vb.h
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

#ifndef __MPI_VB_H__
#define __MPI_VB_H__

#include "sc_comm_vb.h"
#include "sc_comm_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

//typedef SC_U32  VB_UID_E;

VB_POOL SC_MPI_VB_CreatePool(VB_POOL_CONFIG_S *pstVbPoolCfg);
SC_S32 SC_MPI_VB_DestroyPool(VB_POOL Pool);

VB_BLK SC_MPI_VB_GetBlock(VB_POOL Pool, SC_U64 u64BlkSize, const SC_CHAR *pcMmzName);
SC_S32 SC_MPI_VB_ReleaseBlock(VB_BLK Block);

VB_BLK SC_MPI_VB_PhysAddr2Handle(SC_U64 u64PhyAddr);
SC_U64 SC_MPI_VB_Handle2PhysAddr(VB_BLK Block);
VB_POOL SC_MPI_VB_Handle2PoolId(VB_BLK Block);

SC_S32 SC_MPI_VB_InquireUserCnt(VB_BLK Block);

SC_S32 SC_MPI_VB_GetSupplementAddr(VB_BLK Block, VIDEO_SUPPLEMENT_S *pstSupplement);
SC_S32 SC_MPI_VB_SetSupplementConfig(const VB_SUPPLEMENT_CONFIG_S *pstSupplementConfig);
SC_S32 SC_MPI_VB_GetSupplementConfig(VB_SUPPLEMENT_CONFIG_S *pstSupplementConfig);

SC_S32 SC_MPI_VB_Init(SC_VOID);
SC_S32 SC_MPI_VB_Exit(SC_VOID);
SC_S32 SC_MPI_VB_SetConfig(const VB_CONFIG_S *pstVbConfig);
SC_S32 SC_MPI_VB_GetConfig(VB_CONFIG_S *pstVbConfig);

SC_S32 SC_MPI_VB_MmapPool(VB_POOL Pool);
SC_S32 SC_MPI_VB_MunmapPool(VB_POOL Pool);

SC_S32 SC_MPI_VB_GetBlockVirAddr(VB_POOL Pool, SC_U64 u64PhyAddr, SC_VOID **ppVirAddr);

SC_S32 SC_MPI_VB_InitModCommPool(VB_UID_E enVbUid);
SC_S32 SC_MPI_VB_ExitModCommPool(VB_UID_E enVbUid);

SC_S32 SC_MPI_VB_SetModPoolConfig(VB_UID_E enVbUid, const VB_CONFIG_S *pstVbConfig);
SC_S32 SC_MPI_VB_GetModPoolConfig(VB_UID_E enVbUid, VB_CONFIG_S *pstVbConfig);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* __MPI_VI_H__ */

