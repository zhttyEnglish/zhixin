#ifndef __SAMPLE_COMM_AUDIO_H__
#define __SAMPLE_COMM_AUDIO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

SC_S32 SAMPLE_COMM_AUDIO_StartAi(SC_S32 AiDev, AIO_ATTR_S *pstAioAttr);
SC_S32 SAMPLE_COMM_AUDIO_StartAo(SC_S32 AoDev, AIO_ATTR_S *pstAioAttr);
SC_S32 SAMPLE_COMM_AUDIO_StopAi(SC_S32 AiDevId, SC_S32 s32AiChnCnt);
SC_S32 SAMPLE_COMM_AUDIO_StopAo(SC_S32 AoDevId, SC_S32 s32AoChnCnt);
void *SAMPLE_COMM_AUDIO_AiProc(void *parg);
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAiAo(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn);
void *SAMPLE_COMM_AUDIO_AoVolProc(void *parg);
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAiVolCtrl(AUDIO_DEV AiDev);
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAoVolCtrl(AUDIO_DEV AoDev);
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAiVolCtrl(AUDIO_DEV AoDev);
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAoVolCtrl(AUDIO_DEV AoDev);
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAi(AUDIO_DEV AiDev, AI_CHN AiChn);
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(ADEC_CHN AdChn);
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAencAdec(AENC_CHN AeChn);
SC_S32 SAMPLE_COMM_AUDIO_DestoryAllTrd(void);
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn);
void *SAMPLE_COMM_AUDIO_AdecProc(void *parg);
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdFileAdec(ADEC_CHN AdChn, FILE *pAdcFd);
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAenc(SC_S32 AeChn, FILE *pAecFd0, FILE *pAecFd1);
SC_S32 SAMPLE_COMM_AUDIO_StartAenc(SC_S32 s32AencChnCnt, AIO_ATTR_S *pstAioAttr, PAYLOAD_TYPE_E enType);
SC_S32 SAMPLE_COMM_AUDIO_StartAdec(SC_S32 AdChn, AIO_ATTR_S *pstAioAttr, PAYLOAD_TYPE_E enType);
SC_S32 SAMPLE_COMM_AUDIO_CreatGetTrdFileAdec(SC_S32 AdChn, FILE *pAdcFd);
SC_S32 SAMPLE_COMM_AUDIO_CreatSaveAdecFrameToFile(SC_S32 AdChn, FILE *pAdcFd0, FILE *pAdcFd1);
SC_S32 SAMPLE_COMM_AUDIO_SetStopAdecStatus(SC_S32 AdChn);
SC_S32 SAMPLE_COMM_AUDIO_DestroySaveAdecFrameToFile(SC_S32 AdChn);
SC_S32 SAMPLE_COMM_AUDIO_StopAdec(SC_S32 AdChnCount);

SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAencFile(SC_S32 AeChn, FILE *pAecFd);
SC_S32 SAMPLE_COMM_AUDIO_SetStopAencStatus(SC_S32 AeChn);
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAenc(SC_S32 AeChn);
SC_S32 SAMPLE_COMM_AUDIO_StopAenc(SC_S32 s32AencChnCnt);
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAencFile(SC_S32 AeChn);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __SAMPLE_VO_H__*/
