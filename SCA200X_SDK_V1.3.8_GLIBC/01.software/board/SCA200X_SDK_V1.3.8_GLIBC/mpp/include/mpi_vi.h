/**
 * @file     mpi_vi.h
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

#ifndef __MPI_VI_H__
#define __MPI_VI_H__

#include "sc_comm_vi.h"
#include "sc_comm_dis.h"
#include "sc_comm_gdc.h"
#include "sc_comm_vb.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/* 1 for vi device */
SC_S32 SC_MPI_VI_SetDevAttr(VI_DEV ViDev, const VI_DEV_ATTR_S *pstDevAttr);
SC_S32 SC_MPI_VI_GetDevAttr(VI_DEV ViDev, VI_DEV_ATTR_S *pstDevAttr);

SC_S32 SC_MPI_VI_SetDevAttrEx(VI_DEV ViDev, const VI_DEV_ATTR_EX_S *pstDevAttrEx);
SC_S32 SC_MPI_VI_GetDevAttrEx(VI_DEV ViDev, VI_DEV_ATTR_EX_S *pstDevAttrEx);

SC_S32 SC_MPI_VI_SetVSSignalAttr(VI_DEV ViDev, const VI_VS_SIGNAL_ATTR_S *pstVSSignalAttr);
SC_S32 SC_MPI_VI_GetVSSignalAttr(VI_DEV ViDev, VI_VS_SIGNAL_ATTR_S *pstVSSignalAttr);
SC_S32 SC_MPI_VI_TriggerVSSignal(VI_DEV ViDev, SC_BOOL bEnable);

SC_S32 SC_MPI_VI_EnableDev(VI_DEV ViDev);
SC_S32 SC_MPI_VI_DisableDev(VI_DEV ViDev);

SC_S32 SC_MPI_VI_SetMipiBindDev(VI_DEV ViDev, MIPI_DEV MipiDev);
SC_S32 SC_MPI_VI_GetMipiBindDev(VI_DEV ViDev, MIPI_DEV *pMipiDev);

SC_S32 SC_MPI_VI_SetDevBindPipe(VI_DEV ViDev, const VI_DEV_BIND_PIPE_S *pstDevBindPipe);
SC_S32 SC_MPI_VI_GetDevBindPipe(VI_DEV ViDev, VI_DEV_BIND_PIPE_S *pstDevBindPipe);

SC_S32 SC_MPI_VI_SetDevTimingAttr(VI_DEV ViDev, const VI_DEV_TIMING_ATTR_S *pstTimingAttr);
SC_S32 SC_MPI_VI_GetDevTimingAttr(VI_DEV ViDev, VI_DEV_TIMING_ATTR_S *pstTimingAttr);

/* 2 for vi pipe */
SC_S32 SC_MPI_VI_GetPipeCmpParam(VI_PIPE ViPipe, VI_CMP_PARAM_S *pCmpParam);

SC_S32 SC_MPI_VI_SetUserPic(VI_PIPE ViPipe, const VI_USERPIC_ATTR_S *pstUsrPic);
SC_S32 SC_MPI_VI_EnableUserPic(VI_PIPE ViPipe);
SC_S32 SC_MPI_VI_DisableUserPic(VI_PIPE ViPipe);

SC_S32 SC_MPI_VI_CreatePipe(VI_PIPE ViPipe, const VI_PIPE_ATTR_S *pstPipeAttr);
SC_S32 SC_MPI_VI_DestroyPipe(VI_PIPE ViPipe);

SC_S32 SC_MPI_VI_SetPipeAttr(VI_PIPE ViPipe, const VI_PIPE_ATTR_S *pstPipeAttr);
SC_S32 SC_MPI_VI_GetPipeAttr(VI_PIPE ViPipe, VI_PIPE_ATTR_S *pstPipeAttr);

SC_S32 SC_MPI_VI_StartPipe(VI_PIPE ViPipe);
SC_S32 SC_MPI_VI_StopPipe(VI_PIPE ViPipe);

SC_S32 SC_MPI_VI_SetPipePreCrop(VI_PIPE ViPipe, const CROP_INFO_S *pstCropInfo);
SC_S32 SC_MPI_VI_GetPipePreCrop(VI_PIPE ViPipe, CROP_INFO_S *pstCropInfo);

SC_S32 SC_MPI_VI_SetPipePostCrop(VI_PIPE ViPipe, const CROP_INFO_S *pstCropInfo);
SC_S32 SC_MPI_VI_GetPipePostCrop(VI_PIPE ViPipe, CROP_INFO_S *pstCropInfo);

SC_S32 SC_MPI_VI_SetPipeFisheyeConfig(VI_PIPE ViPipe, const FISHEYE_CONFIG_S *pstFishEyeConfig);
SC_S32 SC_MPI_VI_GetPipeFisheyeConfig(VI_PIPE ViPipe, FISHEYE_CONFIG_S *pstFishEyeConfig);

SC_S32 SC_MPI_VI_FisheyePosQueryDst2Src(VI_PIPE ViPipe, VI_CHN ViChn, SC_U32 u32RegionIndex,
    const POINT_S *pstDstPointIn, POINT_S *pstSrcPointOut);

SC_S32 SC_MPI_VI_SetPipeDumpAttr(VI_PIPE ViPipe, const VI_DUMP_ATTR_S *pstDumpAttr);
SC_S32 SC_MPI_VI_GetPipeDumpAttr(VI_PIPE ViPipe, VI_DUMP_ATTR_S *pstDumpAttr);

SC_S32 SC_MPI_VI_SetPipeFrameSource(VI_PIPE ViPipe, const VI_PIPE_FRAME_SOURCE_E enSource);
SC_S32 SC_MPI_VI_GetPipeFrameSource(VI_PIPE ViPipe, VI_PIPE_FRAME_SOURCE_E *penSource);

SC_S32 SC_MPI_VI_GetPipeFrame(VI_PIPE ViPipe, VIDEO_FRAME_INFO_S *pstVideoFrame, SC_S32 s32MilliSec);
SC_S32 SC_MPI_VI_ReleasePipeFrame(VI_PIPE ViPipe, const VIDEO_FRAME_INFO_S *pstVideoFrame);

SC_S32 SC_MPI_VI_SendPipeYUV(VI_PIPE ViPipe, const VIDEO_FRAME_INFO_S *pstVideoFrame, SC_S32 s32MilliSec);
SC_S32 SC_MPI_VI_SendPipeRaw(SC_U32 u32PipeNum, VI_PIPE PipeId[], const VIDEO_FRAME_INFO_S *pstVideoFrame[],
    SC_S32 s32MilliSec);

SC_S32 SC_MPI_VI_SetPipeNRXParam(VI_PIPE ViPipe, const VI_PIPE_NRX_PARAM_S *pstNrXParam);
SC_S32 SC_MPI_VI_GetPipeNRXParam(VI_PIPE ViPipe, VI_PIPE_NRX_PARAM_S *pstNrXParam);

SC_S32 SC_MPI_VI_SetPipeRepeatMode(VI_PIPE ViPipe, const VI_PIPE_REPEAT_MODE_E enPepeatMode);
SC_S32 SC_MPI_VI_GetPipeRepeatMode(VI_PIPE ViPipe, VI_PIPE_REPEAT_MODE_E *penPepeatMode);

SC_S32 SC_MPI_VI_QueryPipeStatus(VI_PIPE ViPipe, VI_PIPE_STATUS_S *pstStatus);

SC_S32 SC_MPI_VI_EnablePipeInterrupt(VI_PIPE ViPipe);
SC_S32 SC_MPI_VI_DisablePipeInterrupt(VI_PIPE ViPipe);

SC_S32 SC_MPI_VI_SetPipeVCNumber(VI_PIPE ViPipe, SC_U32 u32VCNumber);
SC_S32 SC_MPI_VI_GetPipeVCNumber(VI_PIPE ViPipe, SC_U32 *pu32VCNumber);

SC_S32 SC_MPI_VI_SetPipeFrameInterruptAttr(VI_PIPE ViPipe, const FRAME_INTERRUPT_ATTR_S *pstFrameIntAttr);
SC_S32 SC_MPI_VI_GetPipeFrameInterruptAttr(VI_PIPE ViPipe, FRAME_INTERRUPT_ATTR_S *pstFrameIntAttr);

SC_S32 SC_MPI_VI_SetPipeBNRRawDumpAttr(VI_PIPE ViPipe, const BNR_DUMP_ATTR_S *pstBnrDumpAttr);
SC_S32 SC_MPI_VI_GetPipeBNRRawDumpAttr(VI_PIPE ViPipe, BNR_DUMP_ATTR_S *pstBnrDumpAttr);

SC_S32 SC_MPI_VI_GetPipeBNRRaw(VI_PIPE ViPipe, VIDEO_FRAME_INFO_S *pstVideoFrame, SC_S32 s32MilliSec);
SC_S32 SC_MPI_VI_ReleasePipeBNRRaw(VI_PIPE ViPipe, const VIDEO_FRAME_INFO_S *pstVideoFrame);

SC_S32 SC_MPI_VI_PipeAttachVbPool(VI_PIPE ViPipe, VB_POOL VbPool);
SC_S32 SC_MPI_VI_PipeDetachVbPool(VI_PIPE ViPipe);

SC_S32 SC_MPI_VI_GetPipeFd(VI_PIPE ViPipe);

/* 3 for vi chn */
SC_S32 SC_MPI_VI_SetChnAttr(VI_PIPE ViPipe, VI_CHN ViChn, const VI_CHN_ATTR_S *pstChnAttr);
SC_S32 SC_MPI_VI_GetChnAttr(VI_PIPE ViPipe, VI_CHN ViChn, VI_CHN_ATTR_S *pstChnAttr);

SC_S32 SC_MPI_VI_EnableChn(VI_PIPE ViPipe, VI_CHN ViChn);
SC_S32 SC_MPI_VI_DisableChn(VI_PIPE ViPipe, VI_CHN ViChn);

SC_S32 SC_MPI_VI_SetChnCrop(VI_PIPE ViPipe, VI_CHN ViChn, const VI_CROP_INFO_S *pstCropInfo);
SC_S32 SC_MPI_VI_GetChnCrop(VI_PIPE ViPipe, VI_CHN ViChn, VI_CROP_INFO_S *pstCropInfo);

SC_S32 SC_MPI_VI_SetChnRotation(VI_PIPE ViPipe, VI_CHN ViChn, const ROTATION_E enRotation);
SC_S32 SC_MPI_VI_GetChnRotation(VI_PIPE ViPipe, VI_CHN ViChn, ROTATION_E *penRotation);

SC_S32 SC_MPI_VI_SetChnRotationEx(VI_PIPE ViPipe, VI_CHN ViChn, const VI_ROTATION_EX_ATTR_S *pstViRotationExAttr);
SC_S32 SC_MPI_VI_GetChnRotationEx(VI_PIPE ViPipe, VI_CHN ViChn, VI_ROTATION_EX_ATTR_S *pstViRotationExAttr);

SC_S32 SC_MPI_VI_SetChnLDCAttr(VI_PIPE ViPipe, VI_CHN ViChn, const VI_LDC_ATTR_S *pstLDCAttr);
SC_S32 SC_MPI_VI_GetChnLDCAttr(VI_PIPE ViPipe, VI_CHN ViChn, VI_LDC_ATTR_S *pstLDCAttr);

SC_S32 SC_MPI_VI_SetChnLDCV2Attr(VI_PIPE ViPipe, VI_CHN ViChn, const VI_LDCV2_ATTR_S *pstLDCV2Attr);
SC_S32 SC_MPI_VI_GetChnLDCV2Attr(VI_PIPE ViPipe, VI_CHN ViChn, VI_LDCV2_ATTR_S *pstLDCV2Attr);

SC_S32 SC_MPI_VI_SetChnLDCV3Attr(VI_PIPE ViPipe, VI_CHN ViChn, const VI_LDCV3_ATTR_S *pstLDCV3Attr);
SC_S32 SC_MPI_VI_GetChnLDCV3Attr(VI_PIPE ViPipe, VI_CHN ViChn, VI_LDCV3_ATTR_S *pstLDCV3Attr);

SC_S32 SC_MPI_VI_SetChnSpreadAttr(VI_PIPE ViPipe, VI_CHN ViChn, const SPREAD_ATTR_S *pstSpreadAttr);
SC_S32 SC_MPI_VI_GetChnSpreadAttr(VI_PIPE ViPipe, VI_CHN ViChn, SPREAD_ATTR_S *pstSpreadAttr);

SC_S32 SC_MPI_VI_SetChnLowDelayAttr(VI_PIPE ViPipe, VI_CHN ViChn, const VI_LOW_DELAY_INFO_S *pstLowDelayInfo);
SC_S32 SC_MPI_VI_GetChnLowDelayAttr(VI_PIPE ViPipe, VI_CHN ViChn, VI_LOW_DELAY_INFO_S *pstLowDelayInfo);

SC_S32 SC_MPI_VI_GetChnRegionLuma(VI_PIPE ViPipe, VI_CHN ViChn, const VIDEO_REGION_INFO_S *pstRegionInfo,
    SC_U64 *pu64LumaData, SC_S32 s32MilliSec);
SC_S32 SC_MPI_VI_SetChnDISConfig(VI_PIPE ViPipe, VI_CHN ViChn, const DIS_CONFIG_S *pstDISConfig);
SC_S32 SC_MPI_VI_GetChnDISConfig(VI_PIPE ViPipe, VI_CHN ViChn, DIS_CONFIG_S *pstDISConfig);
SC_S32 SC_MPI_VI_SetChnDISAttr(VI_PIPE ViPipe, VI_CHN ViChn, const DIS_ATTR_S *pstDISAttr);
SC_S32 SC_MPI_VI_GetChnDISAttr(VI_PIPE ViPipe, VI_CHN ViChn, DIS_ATTR_S *pstDISAttr);

SC_S32 SC_MPI_VI_SetExtChnFisheye(VI_PIPE ViPipe, VI_CHN ViChn, const FISHEYE_ATTR_S *pstFishEyeAttr);
SC_S32 SC_MPI_VI_GetExtChnFisheye(VI_PIPE ViPipe, VI_CHN ViChn, FISHEYE_ATTR_S *pstFishEyeAttr);

SC_S32 SC_MPI_VI_SetExtChnAttr(VI_PIPE ViPipe, VI_CHN ViChn, const VI_EXT_CHN_ATTR_S *pstExtChnAttr);
SC_S32 SC_MPI_VI_GetExtChnAttr(VI_PIPE ViPipe, VI_CHN ViChn, VI_EXT_CHN_ATTR_S *pstExtChnAttr);

SC_S32 SC_MPI_VI_GetChnFrame(VI_PIPE ViPipe, VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo, SC_S32 s32MilliSec);
SC_S32 SC_MPI_VI_ReleaseChnFrame(VI_PIPE ViPipe, VI_CHN ViChn, const VIDEO_FRAME_INFO_S *pstFrameInfo);

SC_S32 SC_MPI_VI_SetChnEarlyInterrupt(VI_PIPE ViPipe, VI_CHN ViChn, const VI_EARLY_INTERRUPT_S *pstEarlyInterrupt);
SC_S32 SC_MPI_VI_GetChnEarlyInterrupt(VI_PIPE ViPipe, VI_CHN ViChn, VI_EARLY_INTERRUPT_S *pstEarlyInterrupt);

SC_S32 SC_MPI_VI_SetChnAlign(VI_PIPE ViPipe, VI_CHN ViChn, SC_U32 u32Align);
SC_S32 SC_MPI_VI_GetChnAlign(VI_PIPE ViPipe, VI_CHN ViChn, SC_U32 *pu32Align);

SC_S32 SC_MPI_VI_ChnAttachVbPool(VI_PIPE ViPipe, VI_CHN ViChn, VB_POOL VbPool);
SC_S32 SC_MPI_VI_ChnDetachVbPool(VI_PIPE ViPipe, VI_CHN ViChn);

SC_S32 SC_MPI_VI_QueryChnStatus(VI_PIPE ViPipe, VI_CHN ViChn, VI_CHN_STATUS_S *pstChnStatus);

SC_S32 SC_MPI_VI_GetChnFd(VI_PIPE ViPipe, VI_CHN ViChn);

/* 4 for vi stitch group */
SC_S32 SC_MPI_VI_SetStitchGrpAttr(VI_STITCH_GRP StitchGrp, const VI_STITCH_GRP_ATTR_S *pstStitchGrpAttr);
SC_S32 SC_MPI_VI_GetStitchGrpAttr(VI_STITCH_GRP StitchGrp, VI_STITCH_GRP_ATTR_S *pstStitchGrpAttr);

/* 5 for vi module */
SC_S32 SC_MPI_VI_SetModParam(const VI_MOD_PARAM_S *pstModParam);
SC_S32 SC_MPI_VI_GetModParam(VI_MOD_PARAM_S *pstModParam);

SC_S32 SC_MPI_VI_CloseFd(SC_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_VI_H__ */

