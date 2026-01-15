/**
 * @file     mpi_region.h
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

#ifndef __MPI_REGION_H__
#define __MPI_REGION_H__

#include "sc_comm_region.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

SC_S32 SC_MPI_RGN_Create(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion);
SC_S32 SC_MPI_RGN_Destroy(RGN_HANDLE Handle);

SC_S32 SC_MPI_RGN_GetAttr(RGN_HANDLE Handle, RGN_ATTR_S *pstRegion);
SC_S32 SC_MPI_RGN_SetAttr(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion);

SC_S32 SC_MPI_RGN_SetBitMap(RGN_HANDLE Handle, const BITMAP_S *pstBitmap);

SC_S32 SC_MPI_RGN_AttachToChn(RGN_HANDLE Handle, const MPP_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr);
SC_S32 SC_MPI_RGN_DetachFromChn(RGN_HANDLE Handle, const MPP_CHN_S *pstChn);

SC_S32 SC_MPI_RGN_SetDisplayAttr(RGN_HANDLE Handle, const MPP_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr);
SC_S32 SC_MPI_RGN_GetDisplayAttr(RGN_HANDLE Handle, const MPP_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr);

SC_S32 SC_MPI_RGN_GetCanvasInfo(RGN_HANDLE Handle, RGN_CANVAS_INFO_S *pstCanvasInfo);
SC_S32 SC_MPI_RGN_UpdateCanvas(RGN_HANDLE Handle);

SC_S32 SC_MPI_RGN_BatchBegin(RGN_HANDLEGROUP *pu32Group, SC_U32 u32Num, const RGN_HANDLE handle[]);
SC_S32 SC_MPI_RGN_BatchEnd(RGN_HANDLEGROUP u32Group);

SC_S32 SC_MPI_RGN_GetFd(SC_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
