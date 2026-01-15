/*
 * @file     mpi_audio.h
 * @brief    audio
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

#ifndef __MPI_AUDIO_H__
#define __MPI_AUDIO_H__

#include "sc_type.h"
#include "sc_common.h"
#include "sc_comm_aio.h"
#include "sc_comm_aenc.h"
#include "sc_comm_adec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define AENC_ADAPT_MAGIC 0Xfcfcfcfc

/* Audio function api. */
SC_S32 SC_MPI_AUDIO_SetModParam(const AUDIO_MOD_PARAM_S *pstModParam);
SC_S32 SC_MPI_AUDIO_GetModParam(AUDIO_MOD_PARAM_S *pstModParam);
SC_S32 SC_MPI_AUDIO_RegisterVQEModule(const AUDIO_VQE_REGISTER_S *pstVqeRegister);

/* AI function api. */
SC_S32 SC_MPI_AI_SetVolume(SC_S32 s32VolumeDb);

SC_S32 SC_MPI_AI_SetPubAttr(AUDIO_DEV AiDevId, const AIO_ATTR_S *pstAttr);
SC_S32 SC_MPI_AI_GetPubAttr(AUDIO_DEV AiDevId, AIO_ATTR_S *pstAttr);

SC_S32 SC_MPI_AI_Enable(AUDIO_DEV AiDevId);
SC_S32 SC_MPI_AI_Disable(AUDIO_DEV AiDevId);

SC_S32 SC_MPI_AI_EnableChn(AUDIO_DEV AiDevId, AI_CHN AiChn);
SC_S32 SC_MPI_AI_DisableChn(AUDIO_DEV AiDevId, AI_CHN AiChn);

SC_S32 SC_MPI_AI_GetFrame(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_FRAME_S *pstFrm, AEC_FRAME_S *pstAecFrm,
    SC_S32 s32MilliSec);
SC_S32 SC_MPI_AI_ReleaseFrame(AUDIO_DEV AiDevId, AI_CHN AiChn, const AUDIO_FRAME_S *pstFrm,
    const AEC_FRAME_S *pstAecFrm);

SC_S32 SC_MPI_AI_SetChnParam(AUDIO_DEV AiDevId, AI_CHN AiChn, const AI_CHN_PARAM_S *pstChnParam);
SC_S32 SC_MPI_AI_GetChnParam(AUDIO_DEV AiDevId, AI_CHN AiChn, AI_CHN_PARAM_S *pstChnParam);

SC_S32 SC_MPI_AI_SetRecordVqeAttr(AUDIO_DEV AiDevId, AI_CHN AiChn, const AI_RECORDVQE_CONFIG_S *pstVqeConfig);
SC_S32 SC_MPI_AI_GetRecordVqeAttr(AUDIO_DEV AiDevId, AI_CHN AiChn, AI_RECORDVQE_CONFIG_S *pstVqeConfig);

SC_S32 SC_MPI_AI_EnableVqe(AUDIO_DEV AiDevId, AI_CHN AiChn);
SC_S32 SC_MPI_AI_DisableVqe(AUDIO_DEV AiDevId, AI_CHN AiChn);

SC_S32 SC_MPI_AI_EnableReSmp(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_SAMPLE_RATE_E enOutSampleRate);
SC_S32 SC_MPI_AI_DisableReSmp(AUDIO_DEV AiDevId, AI_CHN AiChn);

SC_S32 SC_MPI_AI_SetTrackMode(AUDIO_DEV AiDevId, AUDIO_TRACK_MODE_E enTrackMode);
SC_S32 SC_MPI_AI_GetTrackMode(AUDIO_DEV AiDevId, AUDIO_TRACK_MODE_E *penTrackMode);
SC_S32 SC_MPI_AI_SaveFile(AUDIO_DEV AiDevId, AI_CHN AiChn, const AUDIO_SAVE_FILE_INFO_S *pstSaveFileInfo);
SC_S32 SC_MPI_AI_QueryFileStatus(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_FILE_STATUS_S *pstFileStatus);

SC_S32 SC_MPI_AI_ClrPubAttr(AUDIO_DEV AiDevId);

SC_S32 SC_MPI_AI_GetFd(AUDIO_DEV AiDevId, AI_CHN AiChn);

SC_S32 SC_MPI_AI_EnableAecRefFrame(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_DEV AoDevId, AO_CHN AoChn);
SC_S32 SC_MPI_AI_DisableAecRefFrame(AUDIO_DEV AiDevId, AI_CHN AiChn);

SC_S32 SC_MPI_AI_SetTalkVqeAttr(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_DEV AoDevId, AO_CHN AoChn,
    const AI_TALKVQE_CONFIG_S *pstVqeConfig);
SC_S32 SC_MPI_AI_GetTalkVqeAttr(AUDIO_DEV AiDevId, AI_CHN AiChn, AI_TALKVQE_CONFIG_S *pstVqeConfig);

/* AO function api. */
SC_S32 SC_MPI_AO_SetPubAttr(AUDIO_DEV AoDevId, const AIO_ATTR_S *pstAttr);
SC_S32 SC_MPI_AO_GetPubAttr(AUDIO_DEV AoDevId, AIO_ATTR_S *pstAttr);

SC_S32 SC_MPI_AO_Enable(AUDIO_DEV AoDevId);
SC_S32 SC_MPI_AO_Disable(AUDIO_DEV AoDevId);

SC_S32 SC_MPI_AO_EnableChn(AUDIO_DEV AoDevId, AO_CHN AoChn);
SC_S32 SC_MPI_AO_DisableChn(AUDIO_DEV AoDevId, AO_CHN AoChn);

SC_S32 SC_MPI_AO_SendFrame(AUDIO_DEV AoDevId, AO_CHN AoChn, const AUDIO_FRAME_S *pstData, SC_S32 s32MilliSec);

SC_S32 SC_MPI_AO_EnableReSmp(AUDIO_DEV AoDevId, AO_CHN AoChn, AUDIO_SAMPLE_RATE_E enInSampleRate);
SC_S32 SC_MPI_AO_DisableReSmp(AUDIO_DEV AoDevId, AO_CHN AoChn);

SC_S32 SC_MPI_AO_ClearChnBuf(AUDIO_DEV AoDevId, AO_CHN AoChn);
SC_S32 SC_MPI_AO_QueryChnStat(AUDIO_DEV AoDevId, AO_CHN AoChn, AO_CHN_STATE_S *pstStatus);

SC_S32 SC_MPI_AO_PauseChn(AUDIO_DEV AoDevId, AO_CHN AoChn);
SC_S32 SC_MPI_AO_ResumeChn(AUDIO_DEV AoDevId, AO_CHN AoChn);

SC_S32 SC_MPI_AO_SetVolume(AUDIO_DEV AoDevId, SC_S32 s32VolumeDb);//建议调节范围限制为[-2, 8]
SC_S32 SC_MPI_AO_GetVolume(AUDIO_DEV AoDevId, SC_S32 *ps32VolumeDb);

SC_S32 SC_MPI_AO_SetMute(AUDIO_DEV AoDevId, SC_BOOL bEnable, const AUDIO_FADE_S *pstFade);
SC_S32 SC_MPI_AO_GetMute(AUDIO_DEV AoDevId, SC_BOOL *pbEnable, AUDIO_FADE_S *pstFade);

SC_S32 SC_MPI_AO_SetTrackMode(AUDIO_DEV AoDevId, AUDIO_TRACK_MODE_E enTrackMode);
SC_S32 SC_MPI_AO_GetTrackMode(AUDIO_DEV AoDevId, AUDIO_TRACK_MODE_E *penTrackMode);

SC_S32 SC_MPI_AO_GetFd(AUDIO_DEV AoDevId, AO_CHN AoChn);

SC_S32 SC_MPI_AO_ClrPubAttr(AUDIO_DEV AoDevId);

SC_S32 SC_MPI_AO_SetVqeAttr(AUDIO_DEV AoDevId, AO_CHN AoChn, const AO_VQE_CONFIG_S *pstVqeConfig);
SC_S32 SC_MPI_AO_GetVqeAttr(AUDIO_DEV AoDevId, AO_CHN AoChn, AO_VQE_CONFIG_S *pstVqeConfig);

SC_S32 SC_MPI_AO_EnableVqe(AUDIO_DEV AoDevId, AO_CHN AoChn);
SC_S32 SC_MPI_AO_DisableVqe(AUDIO_DEV AoDevId, AO_CHN AoChn);

/* AENC function api. */
SC_S32 SC_MPI_AENC_CreateChn(AENC_CHN AeChn, const AENC_CHN_ATTR_S *pstAttr);
SC_S32 SC_MPI_AENC_DestroyChn(AENC_CHN AeChn);

SC_S32 SC_MPI_AENC_SendFrame(AENC_CHN AeChn, const AUDIO_FRAME_S *pstFrm, const AEC_FRAME_S *pstAecFrm);

SC_S32 SC_MPI_AENC_GetStream(AENC_CHN AeChn, AUDIO_STREAM_S *pstStream, SC_S32 s32MilliSec);
SC_S32 SC_MPI_AENC_ReleaseStream(AENC_CHN AeChn, const AUDIO_STREAM_S *pstStream);

SC_S32 SC_MPI_AENC_GetFd(AENC_CHN AeChn);

SC_S32 SC_MPI_AENC_RegisterEncoder(SC_S32 *ps32Handle, const AENC_ENCODER_S *pstEncoder);
SC_S32 SC_MPI_AENC_UnRegisterEncoder(SC_S32 s32Handle);

SC_S32 SC_MPI_AENC_GetStreamBufInfo(AENC_CHN AeChn, SC_U64 *pu64PhysAddr, SC_U32 *pu32Size);

SC_S32 SC_MPI_AENC_SetMute(AENC_CHN AeChn, SC_BOOL bEnable);
SC_S32 SC_MPI_AENC_GetMute(AENC_CHN AeChn, SC_BOOL *pbEnable);

/* ADEC function api. */
SC_S32 SC_MPI_ADEC_CreateChn(ADEC_CHN AdChn, const ADEC_CHN_ATTR_S *pstAttr);
SC_S32 SC_MPI_ADEC_DestroyChn(ADEC_CHN AdChn);

SC_S32 SC_MPI_ADEC_SendStream(ADEC_CHN AdChn, const AUDIO_STREAM_S *pstStream, SC_BOOL bBlock);

SC_S32 SC_MPI_ADEC_ClearChnBuf(ADEC_CHN AdChn);

SC_S32 SC_MPI_ADEC_RegisterDecoder(SC_S32 *ps32Handle, const ADEC_DECODER_S *pstDecoder);
SC_S32 SC_MPI_ADEC_UnRegisterDecoder(SC_S32 s32Handle);

SC_S32 SC_MPI_ADEC_GetFrame(ADEC_CHN AdChn, AUDIO_FRAME_INFO_S *pstFrmInfo, SC_BOOL bBlock);
SC_S32 SC_MPI_ADEC_ReleaseFrame(ADEC_CHN AdChn, const AUDIO_FRAME_INFO_S *pstFrmInfo);
SC_S32 SC_MPI_ADEC_SendEndOfStream(ADEC_CHN AdChn, SC_BOOL bInstant);
SC_S32 SC_MPI_ADEC_QueryChnStat(ADEC_CHN AdChn, ADEC_CHN_STATE_S *pstBufferStatus);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_AI_H__ */

