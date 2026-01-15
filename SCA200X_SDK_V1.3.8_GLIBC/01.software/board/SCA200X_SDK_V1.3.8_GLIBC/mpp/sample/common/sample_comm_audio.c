#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "mpi_audio.h"
#include "mpi_sys.h"
//#include "sample_audio.h"
#include "sample_comm.h"

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4  /* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */

static AAC_TYPE_E     gs_enAacType = AAC_TYPE_AACLC;
static AAC_TRANS_TYPE_E gs_enAacTransType = AAC_TRANS_TYPE_ADTS;

typedef struct tagSAMPLE_AI_S
{
    SC_BOOL bStart;
    SC_S32  AiDev;
    SC_S32  AiChn;
    SC_S32  AencChn;
    SC_S32  AoDev;
    SC_S32  AoChn;
    SC_BOOL bSendAenc;
    SC_BOOL bSendAo;
    pthread_t stAiPid;
} SAMPLE_AI_S;

typedef struct tagSAMPLE_VolCtrl_S
{
    SC_BOOL bStart;
    SC_S32  AiDev;
    SC_S32  AiChn;
    SC_S32  AencChn;
    SC_S32  AoDev;
    SC_S32  AoChn;
    SC_BOOL bSendAenc;
    SC_BOOL bSendAo;
    pthread_t stVolPid;
} SAMPLE_VolCtrl_S;

typedef struct tagSAMPLE_AO_S
{
    AUDIO_DEV AoDev;
    SC_S32  AoChn;
    SC_S32  AdecChn;
    SC_BOOL bStart;
    pthread_t stAoPid;
    FILE    *pfd[2];
} SAMPLE_AO_S;

typedef struct tagSAMPLE_AENC_S
{
    SC_BOOL bStart;
    pthread_t stAencPid_in;
    pthread_t stAencPid_out;
    SC_S32  AeChn;
    SC_S32  AdChn;
    FILE    *pfd[2];
    FILE    *out_pfd;
    SC_BOOL bSendAdChn;
} SAMPLE_AENC_S;

typedef struct tagSAMPLE_ADEC_S
{
    SC_BOOL bStart;
    SC_S32 AdChn;
    FILE *prfd;
    //  FILE* pwfd;
    FILE *pwfds[2];
    pthread_t stAdPid_in;
    pthread_t stAdPid_out;
} SAMPLE_ADEC_S;

static SAMPLE_AI_S   gs_stSampleAi[AI_DEV_MAX_NUM * AI_MAX_CHN_NUM];
static SAMPLE_AO_S   gs_stSampleAo[AO_DEV_MAX_NUM];
static SAMPLE_VolCtrl_S   gs_stSampleVolCtrl[AI_DEV_MAX_NUM * AI_MAX_CHN_NUM];

static SAMPLE_AENC_S gs_stSampleAenc[AENC_MAX_CHN_NUM];
static SAMPLE_ADEC_S gs_stSampleAdec[ADEC_MAX_CHN_NUM];

SC_S32 SAMPLE_COMM_AUDIO_StartAi(SC_S32 AiDev, AIO_ATTR_S *pstAioAttr)
{
    SC_S32 ret = SC_SUCCESS;
    SC_S32 i;
    SC_U32 s32AiChnCnt;

    ret = SC_MPI_AI_SetPubAttr(AiDev, pstAioAttr);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("SC_MPI_AI_SetPubAttr failed! %d\n", ret);
        return ret;
    }
    ret = SC_MPI_AI_Enable(AiDev);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("SC_MPI_AI_Enable failed! %d\n", ret);
        return ret;
    }
    s32AiChnCnt = pstAioAttr->u32ChnCnt;
    for (i = 0; i < s32AiChnCnt >> pstAioAttr->enSoundmode; i++)
    {
        ret = SC_MPI_AI_EnableChn(AiDev, i);
        if (SC_SUCCESS != ret)
        {
            SAMPLE_PRT("%s: SC_MPI_AI_EnableChn(%d,%d) failed with %#x\n", __func__, AiDev, i, ret);
            return ret;
        }
    }
    return ret;
}
SC_S32 SAMPLE_COMM_AUDIO_StartAi2(SC_S32 AiDev, SC_S32 AiChn, AIO_ATTR_S *pstAioAttr)
{
    SC_S32 ret = SC_SUCCESS;

    ret = SC_MPI_AI_SetPubAttr(AiDev, pstAioAttr);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("SC_MPI_AI_SetPubAttr failed! %d\n", ret);
        return ret;
    }
    ret = SC_MPI_AI_Enable(AiDev);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("SC_MPI_AI_Enable failed! %d\n", ret);
        return ret;
    }

    ret = SC_MPI_AI_EnableChn(AiDev, AiChn);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("%s: SC_MPI_AI_EnableChn(%d,%d) failed with %#x\n", __func__, AiDev, AiChn, ret);
        return ret;
    }
    return ret;
}

SC_S32 SAMPLE_COMM_AUDIO_StartAo(SC_S32 AoDev, AIO_ATTR_S *pstAioAttr)
{
    SC_S32 ret = SC_SUCCESS;
    SC_S32 i;
    SC_U32 s32AoChnCnt;

    ret = SC_MPI_AO_SetPubAttr(AoDev, pstAioAttr);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("SC_MPI_AO_SetPubAttr failed! %d\n", ret);
        return ret;
    }
    ret = SC_MPI_AO_Enable(AoDev);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("SC_MPI_AO_Enable failed! %d\n", ret);
        return ret;
    }
    s32AoChnCnt = pstAioAttr->u32ChnCnt;
    for (i = 0; i < s32AoChnCnt >> pstAioAttr->enSoundmode; i++)
    {
        ret = SC_MPI_AO_EnableChn(AoDev, i);
        if (SC_SUCCESS != ret)
        {
            SAMPLE_PRT("%s: SC_MPI_AO_EnableChn(%d,%d) failed with %#x\n", __func__, AoDev, i, ret);
            return ret;
        }
    }
    return ret;
}

SC_S32 SAMPLE_COMM_AUDIO_StopAi(SC_S32 AiDevId, SC_S32 s32AiChnCnt)
{
    SC_S32 i;
    SC_S32 ret;

    for (i = 0; i < s32AiChnCnt; i++)
    {
        ret = SC_MPI_AI_DisableChn(AiDevId, i);
        if (SC_SUCCESS != ret)
        {
            printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
            return ret;
        }
    }

    ret = SC_MPI_AI_Disable(AiDevId);
    if (SC_SUCCESS != ret)
    {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return ret;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_StopAo(SC_S32 AoDevId, SC_S32 s32AoChnCnt)
{
    SC_S32 i;
    SC_S32 ret;

    for (i = 0; i < s32AoChnCnt; i++)
    {
        ret = SC_MPI_AO_DisableChn(AoDevId, i);
        if (SC_SUCCESS != ret)
        {
            printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
            return ret;
        }
    }

    ret = SC_MPI_AO_Disable(AoDevId);
    if (SC_SUCCESS != ret)
    {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return ret;
    }

    return SC_SUCCESS;
}
/******************************************************************************
* function : get frame from Ai, send it  to Aenc or Ao
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AiProc(void *parg)
{
    SC_S32 s32Ret;
    SAMPLE_AI_S *pstAiCtl = (SAMPLE_AI_S *)parg;
    AUDIO_FRAME_S stFrame;
    char *outFileNames0 = "aiproc.pcm";
    FILE *pwfds0;
    pwfds0 = fopen(outFileNames0, "w+");

    while (pstAiCtl->bStart)
    {
        /* get frame from ai chn */
        s32Ret = SC_MPI_AI_GetFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, NULL, 500);
        if (SC_SUCCESS != s32Ret )
        {
            SAMPLE_PRT("SC_SAPI_AI_GetFrame(%d, %d) failed! %#x\n", pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
            continue;
        }
        stFrame.u64VirAddr[0] = (SC_U8 *)SC_MPI_SYS_Mmap(stFrame.u64PhyAddr[0], stFrame.u32Len);
        stFrame.u64VirAddr[1] = (SC_U8 *)SC_MPI_SYS_Mmap(stFrame.u64PhyAddr[1], stFrame.u32Len);

        if (SC_TRUE == pstAiCtl->bSendAenc)
        {
            //printf("%s: *************len:%d\n", __FUNCTION__, stFrame_convert.u32Len);
            s32Ret = SC_MPI_AENC_SendFrame(pstAiCtl->AencChn, &stFrame, NULL);
            if (SC_SUCCESS != s32Ret )
            {
                SAMPLE_PRT("%s: SC_MPI_AENC_SendFrame(%d), failed with %#x!\n", \
                    __FUNCTION__, pstAiCtl->AencChn, s32Ret);
            }
        }
        //printf("***##@@@****%d %s %x%x  %x%x\n", __LINE__, __FUNCTION__, stFrame.u64VirAddr[0][0], stFrame.u64VirAddr[0][1],
            //stFrame.u64VirAddr[1][0], stFrame.u64VirAddr[1][1]);

        /* send frame to ao */
        if (SC_TRUE == pstAiCtl->bSendAo)
        {
            fwrite(stFrame.u64VirAddr[0], 1, stFrame.u32Len, pwfds0);
            s32Ret = SC_MPI_AO_SendFrame(pstAiCtl->AoDev, pstAiCtl->AoChn, &stFrame, 1000);
            if (SC_SUCCESS != s32Ret )
            {
                SAMPLE_PRT("%s: SC_MPI_AO_SendFrame(%d, %d), failed with %#x!\n", \
                    __FUNCTION__, pstAiCtl->AoDev, pstAiCtl->AoChn, s32Ret);
                //pstAiCtl->bStart = SC_FALSE;
                //return NULL;
            }
        }

        /* finally you must release the stream */
        s32Ret = SC_MPI_AI_ReleaseFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, NULL);
        if (SC_SUCCESS != s32Ret )
        {
            SAMPLE_PRT("%s: SC_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n", \
                __FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
            pstAiCtl->bStart = SC_FALSE;
            return NULL;
        }
        SC_MPI_SYS_Munmap(stFrame.u64VirAddr[1], stFrame.u32Len);
    }

    pstAiCtl->bStart = SC_FALSE;
    return NULL;
}

void *SAMPLE_COMM_AUDIO_AiProc2(void *parg)
{
    SC_S32 s32Ret;
    SAMPLE_AI_S *pstAiCtl = (SAMPLE_AI_S *)parg;
    AUDIO_FRAME_S stFrame;
    AUDIO_FRAME_S stFrame_convert;
    char *outFileNames0 = "aiproc.pcm";
    FILE *pwfds0;
    pwfds0 = fopen(outFileNames0, "w+");

    while (pstAiCtl->bStart)
    {
        /* get frame from ai chn */
        s32Ret = SC_MPI_AI_GetFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, NULL, 50);
        if (SC_SUCCESS != s32Ret )
        {
            SAMPLE_PRT("SC_SAPI_AI_GetFrame(%d, %d) failed! %#x\n", pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
            continue;
        }
        //stFrame.u64VirAddr[0] = (SC_U8*)SC_MPI_SYS_Mmap(stFrame.u64PhyAddr[0], stFrame.u32Len);
        stFrame.u64VirAddr[1] = (SC_U8 *)SC_MPI_SYS_Mmap(stFrame.u64PhyAddr[1], stFrame.u32Len);
        memcpy(&stFrame_convert, &stFrame, sizeof(AUDIO_FRAME_S));
        stFrame_convert.u64VirAddr[0] = stFrame_convert.u64VirAddr[1];
        stFrame_convert.u64PhyAddr[0] = stFrame_convert.u64PhyAddr[1];
        //printf("***11**%d  %p %p  %p %p\n",__LINE__,stFrame.u64VirAddr[0],stFrame.u64VirAddr[1],stFrame_convert.u64VirAddr[0],stFrame_convert.u64VirAddr[1]);

        if (SC_TRUE == pstAiCtl->bSendAenc)
        {
            //printf("%s: *************len:%d\n", __FUNCTION__, stFrame_convert.u32Len);
            s32Ret = SC_MPI_AENC_SendFrame(pstAiCtl->AencChn, &stFrame_convert, NULL);
            if (SC_SUCCESS != s32Ret )
            {
                SAMPLE_PRT("%s: SC_MPI_AENC_SendFrame(%d), failed with %#x!\n", \
                    __FUNCTION__, pstAiCtl->AencChn, s32Ret);
                //pstAiCtl->bStart = SC_FALSE;
                //return NULL;
            }
        }

        /* send frame to ao */
        if (SC_TRUE == pstAiCtl->bSendAo)
        {
            fwrite(stFrame_convert.u64VirAddr[0], 1, stFrame_convert.u32Len, pwfds0);
            s32Ret = SC_MPI_AO_SendFrame(pstAiCtl->AoDev, pstAiCtl->AoChn, &stFrame_convert, 1000);
            if (SC_SUCCESS != s32Ret )
            {
                SAMPLE_PRT("%s: SC_MPI_AO_SendFrame(%d, %d), failed with %#x!\n", \
                    __FUNCTION__, pstAiCtl->AoDev, pstAiCtl->AoChn, s32Ret);
                //pstAiCtl->bStart = SC_FALSE;
                //return NULL;
            }
        }

        /* finally you must release the stream */
        s32Ret = SC_MPI_AI_ReleaseFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, NULL);
        if (SC_SUCCESS != s32Ret )
        {
            SAMPLE_PRT("%s: SC_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n", \
                __FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
            pstAiCtl->bStart = SC_FALSE;
            return NULL;
        }
        SC_MPI_SYS_Munmap(stFrame.u64VirAddr[1], stFrame.u32Len);
    }

    pstAiCtl->bStart = SC_FALSE;
    return NULL;
}

SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAiAo(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn)
{
    SAMPLE_AI_S *pstAi = NULL;

    pstAi = &gs_stSampleAi[AiDev * AI_MAX_CHN_NUM + AiChn];
    pstAi->bSendAenc = SC_FALSE;
    pstAi->bSendAo = SC_TRUE;
    pstAi->bStart = SC_TRUE;
    pstAi->AiDev = AiDev;
    pstAi->AiChn = AiChn;
    pstAi->AoDev = AoDev;
    pstAi->AoChn = AoChn;

    pthread_create(&pstAi->stAiPid, 0, SAMPLE_COMM_AUDIO_AiProc, pstAi);

    return SC_SUCCESS;
}

void *SAMPLE_COMM_AUDIO_AiVolProc(void *parg)
{
    SC_S32 s32Ret;
    SC_S32 s32Volume;
    AUDIO_DEV AoDev;
    AUDIO_FADE_S stFade;
    SAMPLE_VolCtrl_S *pstVolCtl = (SAMPLE_VolCtrl_S *)parg;

    while (pstVolCtl->bStart)
    {
        s32Ret = SC_MPI_AI_SetVolume(56);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_AI_SetVolume, failed with %#x!\n", \
                __FUNCTION__, s32Ret);
        }
        printf("set volume 56\n");
        sleep(5);

        s32Ret = SC_MPI_AI_SetVolume(11);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_AI_SetVolume, failed with %#x!\n", \
                __FUNCTION__, s32Ret);
        }
        printf("set volume 11\n");
        sleep(2);

        /*
        for (s32Volume = 10; s32Volume <= 56; s32Volume++)
        {
            s32Ret = SC_MPI_AI_SetVolume(s32Volume);
            if (SC_SUCCESS != s32Ret)
            {
                printf("%s: SC_MPI_AI_SetVolume, failed with %#x!\n", \
                       __FUNCTION__, s32Ret);
            }
            printf("\rset volume %d          ", s32Volume);
            fflush(stdout);
            sleep(2);
        }

        for (s32Volume = 56; s32Volume >= 10; s32Volume--)
        {
            s32Ret = SC_MPI_AI_SetVolume(s32Volume);
            if (SC_SUCCESS != s32Ret)
            {
                printf("%s: SC_MPI_AI_SetVolume, failed with %#x!\n", \
                       __FUNCTION__, s32Ret);
            }
            printf("\rset volume %d          ", s32Volume);
            fflush(stdout);
            sleep(2);
        }

        stFade.bFade         = SC_TRUE;
        stFade.enFadeInRate  = AUDIO_FADE_RATE_128;
        stFade.enFadeOutRate = AUDIO_FADE_RATE_128;

        s32Ret = SC_MPI_AO_SetMute(AoDev, SC_TRUE, &stFade);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_AO_SetMute(%d), failed with %#x!\n", \
                   __FUNCTION__, AoDev, s32Ret);
        }
        printf("\rset Ao mute            ");
        fflush(stdout);
        sleep(2);

        s32Ret = SC_MPI_AO_SetMute(AoDev, SC_FALSE, NULL);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_AO_SetMute(%d), failed with %#x!\n", \
                   __FUNCTION__, AoDev, s32Ret);
        }
        printf("\rset Ao unmute          ");
        fflush(stdout);
        sleep(2);

        */
    }
    return NULL;
}

/******************************************************************************
* function : Create the thread to set Ao volume
******************************************************************************/

void *SAMPLE_COMM_AUDIO_AoVolProc(void *parg)
{
    SC_S32 s32Ret;
    SC_S32 s32Volume;
    AUDIO_DEV AoDev;
    AUDIO_FADE_S stFade;
    SAMPLE_AO_S *pstAoCtl = (SAMPLE_AO_S *)parg;
    AoDev = pstAoCtl->AoDev;

    while (pstAoCtl->bStart)
    {
        for (s32Volume = 0; s32Volume <= 6; s32Volume++)
        {
            s32Ret = SC_MPI_AO_SetVolume( AoDev, s32Volume);
            if (SC_SUCCESS != s32Ret)
            {
                printf("%s: SC_MPI_AO_SetVolume(%d), failed with %#x!\n", \
                    __FUNCTION__, AoDev, s32Ret);
            }
            printf("\rset volume %d          ", s32Volume);
            fflush(stdout);
            sleep(2);
        }

        for (s32Volume = 5; s32Volume >= -15; s32Volume--)
        {
            s32Ret = SC_MPI_AO_SetVolume( AoDev, s32Volume);
            if (SC_SUCCESS != s32Ret)
            {
                printf("%s: SC_MPI_AO_SetVolume(%d), failed with %#x!\n", \
                    __FUNCTION__, AoDev, s32Ret);
            }
            printf("\rset volume %d          ", s32Volume);
            fflush(stdout);
            sleep(2);
        }

        for (s32Volume = -14; s32Volume <= 0; s32Volume++)
        {
            s32Ret = SC_MPI_AO_SetVolume( AoDev, s32Volume);
            if (SC_SUCCESS != s32Ret)
            {
                printf("%s: SC_MPI_AO_SetVolume(%d), failed with %#x!\n", \
                    __FUNCTION__, AoDev, s32Ret);
            }
            printf("\rset volume %d          ", s32Volume);
            fflush(stdout);
            sleep(2);
        }

        stFade.bFade         = SC_TRUE;
        stFade.enFadeInRate  = AUDIO_FADE_RATE_128;
        stFade.enFadeOutRate = AUDIO_FADE_RATE_128;

        s32Ret = SC_MPI_AO_SetMute(AoDev, SC_TRUE, &stFade);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_AO_SetVolume(%d), failed with %#x!\n", \
                __FUNCTION__, AoDev, s32Ret);
        }
        printf("\rset Ao mute            ");
        fflush(stdout);
        sleep(2);

        s32Ret = SC_MPI_AO_SetMute(AoDev, SC_FALSE, NULL);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_AO_SetVolume(%d), failed with %#x!\n", \
                __FUNCTION__, AoDev, s32Ret);
        }
        printf("\rset Ao unmute          ");
        fflush(stdout);
        sleep(2);
    }
    return NULL;
}
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAiVolCtrl(AUDIO_DEV AiDev)
{
    SAMPLE_VolCtrl_S *pstVolCtl = NULL;

    pstVolCtl =  &gs_stSampleVolCtrl[AiDev];
    pstVolCtl->AiDev =  AiDev;
    pstVolCtl->bStart = SC_TRUE;
    pthread_create(&pstVolCtl->stVolPid, 0, SAMPLE_COMM_AUDIO_AiVolProc, pstVolCtl);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAoVolCtrl(AUDIO_DEV AoDev)
{
    SAMPLE_AO_S *pstAoCtl = NULL;

    pstAoCtl =  &gs_stSampleAo[AoDev];
    pstAoCtl->AoDev =  AoDev;
    pstAoCtl->bStart = SC_TRUE;
    pthread_create(&pstAoCtl->stAoPid, 0, SAMPLE_COMM_AUDIO_AoVolProc, pstAoCtl);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAiVolCtrl(AUDIO_DEV AiDev)
{
    SAMPLE_VolCtrl_S *pstVolCtl = NULL;

    pstVolCtl =  &gs_stSampleVolCtrl[AiDev];
    if (pstVolCtl->bStart)
    {
        pstVolCtl->bStart = SC_FALSE;
        pthread_cancel(pstVolCtl->stVolPid);
        pthread_join(pstVolCtl->stVolPid, 0);
    }
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAoVolCtrl(AUDIO_DEV AoDev)
{
    SAMPLE_AO_S *pstAoCtl = NULL;

    pstAoCtl =  &gs_stSampleAo[AoDev];
    if (pstAoCtl->bStart)
    {
        pstAoCtl->bStart = SC_FALSE;
        pthread_cancel(pstAoCtl->stAoPid);
        pthread_join(pstAoCtl->stAoPid, 0);
    }
    return SC_SUCCESS;
}
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAi(AUDIO_DEV AiDev, AI_CHN AiChn)
{
    SAMPLE_AI_S *pstAi = NULL;

    pstAi = &gs_stSampleAi[AiDev * AI_MAX_CHN_NUM + AiChn];
    if (pstAi->bStart)
    {
        pstAi->bStart = SC_FALSE;
        //pthread_cancel(pstAi->stAiPid);
        pthread_join(pstAi->stAiPid, 0);
    }
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    SAMPLE_AI_S *pstAi = NULL;

    pstAi = &gs_stSampleAi[AiDev * AI_MAX_CHN_NUM + AiChn];
    pstAi->bSendAenc = SC_TRUE;
    pstAi->bSendAo = SC_FALSE;
    pstAi->bStart = SC_TRUE;
    pstAi->AiDev = AiDev;
    pstAi->AiChn = AiChn;
    pstAi->AencChn = AeChn;
    pthread_create(&pstAi->stAiPid, 0, SAMPLE_COMM_AUDIO_AiProc, pstAi);

    return SC_SUCCESS;
}


/******************************* Encoder ****************************************/
SC_S32 SAMPLE_COMM_AUDIO_StartAenc2(SC_S32 Aechn, AIO_ATTR_S *pstAioAttr, PAYLOAD_TYPE_E enType, G726_BPS_E g726BPS)
{
    SC_S32 AeChn;
    SC_S32 s32Ret, i;
    AENC_CHN_ATTR_S stAencAttr;
    AENC_ATTR_ADPCM_S stAdpcmAenc;
    AENC_ATTR_G711_S stAencG711;
    AENC_ATTR_G726_S stAencG726;
    AENC_ATTR_LPCM_S stAencLpcm;
    AENC_ATTR_AAC_S  stAencAac;     // AAC协议参数

    /* set AENC chn attr */
    stAencAttr.u32BufSize = 30;          //音频编码协议对应的帧长
    stAencAttr.u32PtNumPerFrm = pstAioAttr->u32PtNumPerFrm;  //音频编码缓存大小，以帧为单位
    stAencAttr.enType = enType;          //音频编码协议类型
    if (PT_ADPCMA == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
    }
    else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
    {
        stAencAttr.pValue = &stAencG711;
    }
    else if (PT_G726 == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = g726BPS;
    }
    else if (PT_LPCM == stAencAttr.enType)
    {
        stAencAttr.pValue = &stAencLpcm;
    }
    else if (PT_AAC == stAencAttr.enType)
    {
        stAencAttr.pValue = &stAencAac;    //具体协议属性指针
        stAencAac.enAACType = gs_enAacType;
        stAencAac.enBitRate = AAC_BPS_64K;//gs_enAacBps;//pstAioAttr->enSamplerate * 2 ;//gs_enAacBps;
        stAencAac.enBitWidth = AUDIO_BIT_WIDTH_16;
        stAencAac.enSmpRate = pstAioAttr->enSamplerate;
        stAencAac.enSoundMode = pstAioAttr->enSoundmode;
        stAencAac.enTransType = gs_enAacTransType;
        stAencAac.s16BandWidth = 0;
    }
    else
    {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAencAttr.enType);
        return SC_FAILURE;
    }
    s32Ret = SC_MPI_AENC_CreateChn(Aechn, &stAencAttr);
    if (SC_SUCCESS != s32Ret)
    {
        printf("%s: SC_MPI_AENC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,Aechn, s32Ret);
        return s32Ret;
    }
    return s32Ret;
}

/******************************************************************************
* function : Stop Aenc
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_StopAenc(SC_S32 s32AencChnCnt)
{
    SC_S32 i;
    SC_S32 s32Ret;

    for (i = 0; i < s32AencChnCnt; i++)
    {
        s32Ret = SC_MPI_AENC_DestroyChn(i);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_AENC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
                i, s32Ret);
            return s32Ret;
        }

    }

    return SC_SUCCESS;
}

/******************************************************************************
* function : get frame from file, send it  to Aenc
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AENCProc(void *parg)
{
    SAMPLE_AENC_S *pstAeCtl = (SAMPLE_AENC_S *)parg;
    AUDIO_FRAME_S stFrame;
    SC_S32 s32Ret = SC_SUCCESS;
    SC_U32 u32ReadLen0;
    SC_U32 u32ReadLen1;
    VB_BLK VbBlk[2];

    FILE *chn0_fd = pstAeCtl->pfd[0];
    FILE *chn1_fd = pstAeCtl->pfd[1];

    SC_U32 u32Len = 320 * 2;

    memset(&stFrame, 0, sizeof(AUDIO_FRAME_S));

    VB_POOL_CONFIG_S stVbPoolCfg;
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Len;
    stVbPoolCfg.u32BlkCnt = 1;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    for(int i = 0; i < 2; i++)
    {
        stFrame.u32PoolId[i] = SC_MPI_VB_CreatePool(&stVbPoolCfg);
        if (stFrame.u32PoolId[i] == VB_INVALID_POOLID)
        {
            printf("Maybe you not call sys init\n");
            return NULL;
        }
        VbBlk[i] = SC_MPI_VB_GetBlock(stFrame.u32PoolId[i], u32Len, SC_NULL);
        if (VB_INVALID_HANDLE == VbBlk[i])
        {
            SAMPLE_PRT("SC_MPI_VB_GetBlock err! size:%d\n", u32Len);
            return NULL;
        }
        stFrame.u64PhyAddr[i] = SC_MPI_VB_Handle2PhysAddr(VbBlk[i]);
        if (0 == stFrame.u64PhyAddr[i])
        {
            SAMPLE_PRT("SC_MPI_VB_Handle2PhysAddr err!\n");
            return NULL;
        }
        stFrame.u64VirAddr[i] = (SC_U8 *)SC_MPI_SYS_Mmap(stFrame.u64PhyAddr[i], u32Len);
        if (NULL == stFrame.u64VirAddr[i])
        {
            SAMPLE_PRT("SC_MPI_SYS_Mmap err!\n");
            return NULL;
        }
    }
    //printf("**%d****poolID%d %d VbBlk%d %d\n",__LINE__,stFrame.u32PoolId[0],stFrame.u32PoolId[1],VbBlk[0],VbBlk[1]);
    //printf("***%d***%#llx %#llx  %p %p\n",__LINE__,stFrame.u64PhyAddr[0],stFrame.u64PhyAddr[1],stFrame.u64VirAddr[0],stFrame.u64VirAddr[1]);

    stFrame.enBitwidth = AUDIO_BIT_WIDTH_16;
    stFrame.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stFrame.u32Len = u32Len;
    while(pstAeCtl->bStart)
    {
        if(SC_ERR_AENC_NOBUF != s32Ret)
        {
            u32ReadLen0 = fread(stFrame.u64VirAddr[0], 1, u32Len, chn0_fd);
            u32ReadLen1 = fread(stFrame.u64VirAddr[1], 1, u32Len, chn1_fd);
            stFrame.u32Seq++;
            if(u32ReadLen0 != u32Len || u32ReadLen1 != u32Len)
            {
                printf("%s: fread0(%d) fread1(%d) but expect (%d)\n", __FUNCTION__, u32ReadLen0, u32ReadLen1, u32Len);
                printf("END!\n");
                break;
            }
        }
        
        s32Ret = SC_MPI_AENC_SendFrame(pstAeCtl->AeChn, &stFrame, NULL);
        //printf("%s: *************%d  len:%d  %d\n", __FUNCTION__,pstAeCtl->AeChn, stFrame.u32Len,s32Ret);
        if (SC_SUCCESS != s32Ret )
        {
            printf("%s: SC_MPI_AENC_SendFrame(%d), failed with %#x!\n", \
                __FUNCTION__, pstAeCtl->AeChn, s32Ret);
            //pstAiCtl->bStart = SC_FALSE;
            //return NULL;
        }
        /*** 如果取码流较慢，编码较快时，会导致编码缓存不足，建议加延时 ***/
        usleep(20000);
    }
    gs_stSampleAenc[0].bStart = SC_FALSE;


    fclose(chn0_fd);
    fclose(chn1_fd);
    return NULL;
}

/******************************************************************************
* function : Create the thread to get frame from pcm file and send to aenc
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAenc(SC_S32 AeChn, FILE *pAecFd0, FILE *pAecFd1)
{
    SAMPLE_AENC_S *pstAenc = NULL;

    if (NULL == pAecFd0 || NULL == pAecFd1)
    {
        return SC_FAILURE;
    }

    pstAenc = &gs_stSampleAenc[AeChn];
    pstAenc->AeChn = AeChn;
    pstAenc->AdChn = -1;
    pstAenc->bSendAdChn = SC_TRUE;
    (pstAenc->pfd)[0] = pAecFd0;
    (pstAenc->pfd)[1] = pAecFd1;
    pstAenc->bStart = SC_TRUE;
    pthread_create(&pstAenc->stAencPid_in, 0, SAMPLE_COMM_AUDIO_AENCProc, pstAenc);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAenc(SC_S32 AeChn)
{
    SAMPLE_AENC_S *pstAenc = NULL;

    pstAenc = &gs_stSampleAenc[AeChn];

    if (pstAenc->bStart)
    {
        pstAenc->bStart = SC_FALSE;
        //pthread_cancel(pstAdec->stAdPid);
        pthread_join(pstAenc->stAencPid_in, 0);
    }

    return SC_SUCCESS;

}

/******************************************************************************
* function : get stream from Aenc, send it  to Adec & save it to file
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AencProc(void *parg)
{
    #if 1
    SC_S32 s32Ret;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;
    AUDIO_STREAM_S stStream;

    while (pstAencCtl->bStart)
    {
        /* get stream from aenc chn */
        s32Ret = SC_MPI_AENC_GetStream(pstAencCtl->AeChn, &stStream, 50);
        if (SC_SUCCESS != s32Ret )
        {
            continue;
        }
        else
        {
            /* save audio stream to file */
            if (stStream.pStream != NULL)
            {
                //printf("get enc data:%x\n", *(stStream.pStream + stStream.u32Len/2));
                fwrite(stStream.pStream, stStream.u32Len, 1, pstAencCtl->out_pfd);
            }
        }
        /* finally you must release the stream */
        s32Ret = SC_MPI_AENC_ReleaseStream(pstAencCtl->AeChn, &stStream);
        if (SC_SUCCESS != s32Ret )
        {
            printf("%s: SC_MPI_AENC_ReleaseStream(%d), failed with %#x!\n", \
                __FUNCTION__, pstAencCtl->AeChn, s32Ret);
        }
        fflush(pstAencCtl->out_pfd);
    }
    fclose(pstAencCtl->out_pfd);
    //  pstAencCtl->bStart = SC_FALSE;
    return NULL;
    #endif

    #if 0
    SC_S32 s32Ret;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;
    AUDIO_STREAM_S stStream;

    SC_S32 AencFd;
    fd_set read_fds;
    struct timeval TimeoutVal;

    FD_ZERO(&read_fds);
    printf("***##@@@****%d %s %d\n", __LINE__, __FUNCTION__, pstAencCtl->AeChn);
    AencFd = SC_MPI_AENC_GetFd(pstAencCtl->AeChn);
    FD_SET(AencFd, &read_fds);

    while (pstAencCtl->bStart)
    {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AencFd, &read_fds);

        s32Ret = select(AencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            break;
        }
        else if (0 == s32Ret)
        {
            printf("%s: get aenc stream select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AencFd, &read_fds))
        {
            /* get stream from aenc chn */
            s32Ret = SC_MPI_AENC_GetStream(pstAencCtl->AeChn, &stStream, 50);
            if (SC_SUCCESS != s32Ret )
            {
                continue;
            }
            else
            {
                /* save audio stream to file */
                if (stStream.pStream != NULL)
                {
                    fwrite(stStream.pStream, stStream.u32Len, 1, pstAencCtl->out_pfd);
                }
            }
            /* finally you must release the stream */
            s32Ret = SC_MPI_AENC_ReleaseStream(pstAencCtl->AeChn, &stStream);
            if (SC_SUCCESS != s32Ret )
            {
                printf("%s: SC_MPI_AENC_ReleaseStream(%d), failed with %#x!\n", \
                    __FUNCTION__, pstAencCtl->AeChn, s32Ret);
            }
            fflush(pstAencCtl->out_pfd);
        }

    }
    fclose(pstAencCtl->out_pfd);
    //  pstAencCtl->bStart = SC_FALSE;
    return NULL;

    #endif
}

/******************************************************************************
* function : Create the thread to get stream from aenc and save to file
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAencFile(SC_S32 AeChn, FILE *pAecFd)
{
    SAMPLE_AENC_S *pstAenc = NULL;

    if (NULL == pAecFd)
    {
        return SC_FAILURE;
    }

    pstAenc = &gs_stSampleAenc[AeChn];
    pstAenc->AeChn = AeChn;
    //    pstAenc->AdChn = 0;
    pstAenc->bSendAdChn = SC_TRUE;
    pstAenc->out_pfd = pAecFd;
    pstAenc->bStart = SC_TRUE;
    pthread_create(&pstAenc->stAencPid_out, 0, SAMPLE_COMM_AUDIO_AencProc, pstAenc);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdAencFile(SC_S32 AeChn)
{
    SAMPLE_AENC_S *pstAenc = NULL;

    pstAenc = &gs_stSampleAenc[AeChn];

    if (pstAenc->bStart)
    {
        pstAenc->bStart = SC_FALSE;
        //pthread_cancel(pstAdec->stAdPid);
        pthread_join(pstAenc->stAencPid_out, 0);
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_SetStopAencStatus(SC_S32 AeChn)
{
    gs_stSampleAenc[AeChn].bStart = SC_FALSE;
    return 0;
}

/******************************************************************************
* function : Start Adec
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_StartAdec(SC_S32 AdChn,AIO_ATTR_S *pstAioAttr,  PAYLOAD_TYPE_E enType)
{
    SC_S32 s32Ret;
    ADEC_CHN_ATTR_S stAdecAttr;

    ADEC_ATTR_ADPCM_S stAdpcmAenc;
    ADEC_ATTR_G711_S stAdecG711;
    ADEC_ATTR_G726_S stAdecG726;
    ADEC_ATTR_LPCM_S stAdecLpcm;
    ADEC_ATTR_AAC_S stAdecAac;
    stAdecAttr.enType = enType;
    stAdecAttr.u32BufSize = 10;
    stAdecAttr.enMode = ADEC_MODE_STREAM;

    if (PT_ADPCMA == stAdecAttr.enType)
    {
        stAdecAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
    }
    else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecG711;
    }
    else if (PT_G726 == stAdecAttr.enType)
    {
        stAdecAttr.pValue       = &stAdecG726;
        stAdecG726.enG726bps    = G726_BPS;
    }
    else if (PT_LPCM == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecLpcm;
    }
    else if (PT_AAC == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecAac;
        stAdecAttr.enMode = ADEC_MODE_STREAM;   /* aac should be stream mode */
        stAdecAac.enTransType = AAC_TRANS_TYPE_ADTS;
    }
    else
    {
        printf("%s: invalid adec payload type:%d\n", __FUNCTION__, stAdecAttr.enType);
        return SC_FAILURE;
    }

    /* create adec chn*/
    s32Ret = SC_MPI_ADEC_CreateChn(AdChn, &stAdecAttr);

    if (SC_SUCCESS != s32Ret)
    {
        printf("%s: SC_MPI_ADEC_CreateChn(%d) failed with %#x!\n", __FUNCTION__, \
            AdChn, s32Ret);
        return s32Ret;
    }
    return 0;
}

/******************************************************************************
* function : Stop Adec
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_StopAdec(SC_S32 AdChnCount)
{
    SC_S32 s32Ret;
    for(SC_S32 i = 0; i < AdChnCount; i++)
    {
        s32Ret = SC_MPI_ADEC_DestroyChn(i);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_ADEC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
                i, s32Ret);
            return s32Ret;
        }
    }
    return SC_SUCCESS;
}

/******************************************************************************
* function : get stream from file, and send it  to Adec
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AdecProc(void *parg)
{
    SC_S32 s32Ret;
    AUDIO_STREAM_S stAudioStream;
    SC_U32 u32Len = 320;//AAC should be 1024
    SC_U32 u32ReadLen;
    SC_S32 s32AdecChn;
    SAMPLE_ADEC_S *pstAdecCtl = (SAMPLE_ADEC_S *)parg;
    FILE *pfd = pstAdecCtl->prfd;
    s32AdecChn = pstAdecCtl->AdChn;

    VB_POOL_CONFIG_S stVbPoolCfg;
    SC_U32 u32PoolId;
    VB_BLK VbBlk;
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Len;
    stVbPoolCfg.u32BlkCnt = 1;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    u32PoolId = SC_MPI_VB_CreatePool(&stVbPoolCfg);
    if (u32PoolId == VB_INVALID_POOLID)
    {
        printf("Maybe you not call sys init\n");
        return NULL;
    }
    VbBlk = SC_MPI_VB_GetBlock(u32PoolId, u32Len, SC_NULL);
    if (VB_INVALID_HANDLE == VbBlk)
    {
        SAMPLE_PRT("SC_MPI_VB_GetBlock err! size:%d\n", u32Len);
        return NULL;
    }
    stAudioStream.u64PhyAddr = SC_MPI_VB_Handle2PhysAddr(VbBlk);
    if (0 == stAudioStream.u64PhyAddr)
    {
        SAMPLE_PRT("SC_MPI_VB_Handle2PhysAddr err!\n");
        return NULL;
    }
    stAudioStream.pStream = (SC_U8 *)SC_MPI_SYS_Mmap(stAudioStream.u64PhyAddr, u32Len);
    if (NULL == stAudioStream.pStream)
    {
        SAMPLE_PRT("SC_MPI_SYS_Mmap err!\n");
        return NULL;
    }
    int packet_num = 0;
    while (SC_TRUE == pstAdecCtl->bStart)
    {
        /* read from file */
        u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, pfd);
        if (u32ReadLen <= 0)
        {
            s32Ret = SC_MPI_ADEC_SendEndOfStream(s32AdecChn, SC_TRUE);
            if (SC_SUCCESS != s32Ret)
            {
                printf("%s: SC_MPI_ADEC_SendEndOfStream failed!\n", __FUNCTION__);
            }
            printf("################ END!!!\n");
            //continue;
            break;
        }

        /* here only demo adec streaming sending mode, but pack sending mode is commended */
        stAudioStream.u32Len = u32ReadLen;
        s32Ret = SC_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, SC_TRUE);
        packet_num++;
        //printf("*******SC_MPI_ADEC_SendStream**1s**len%d pack_num%d\n",stAudioStream.u32Len,packet_num);
        if (SC_SUCCESS != s32Ret)
        {
            printf("%s: SC_MPI_ADEC_SendStream(%d) failed with %#x!\n", \
                __FUNCTION__, s32AdecChn, s32Ret);
            //break;
        }
        usleep(20000);
    }
    gs_stSampleAdec[s32AdecChn].bStart = SC_FALSE;
    fclose(pfd);
    SC_MPI_SYS_Munmap(stAudioStream.pStream, u32Len);
    SC_MPI_VB_ReleaseBlock(VbBlk);


    return NULL;
}

/******************************************************************************
* function : Create the thread to get stream from file and send to adec
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_CreatGetTrdFileAdec(SC_S32 AdChn, FILE *pAdcFd)
{
    SAMPLE_ADEC_S *pstAdec = NULL;

    if (NULL == pAdcFd)
    {
        return SC_FAILURE;
    }

    pstAdec = &gs_stSampleAdec[AdChn];
    //pstAdec = &SampleAdecR;
    pstAdec->AdChn = AdChn;
    pstAdec->prfd = pAdcFd;
    pstAdec->bStart = SC_TRUE;
    pthread_create(&pstAdec->stAdPid_in, 0, SAMPLE_COMM_AUDIO_AdecProc, pstAdec);

    return SC_SUCCESS;
}

/******************************************************************************
* function : Destory the thread to get stream from file and send to adec
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(SC_S32 AdChnCount)
{
    SAMPLE_ADEC_S *pstAdec = NULL;
    for (int i = 0; i < AdChnCount; i++)
    {
        pstAdec = &gs_stSampleAdec[i];
        //pstAdec = &SampleAdecR;
        if (pstAdec->bStart)
        {
            pstAdec->bStart = SC_FALSE;
            //pthread_cancel(pstAdec->stAdPid);
            pthread_join(pstAdec->stAdPid_in, 0);
        }
    }
    return SC_SUCCESS;
}

/******************************************************************************
* function : get frame from adec and save to filec
******************************************************************************/
void *SAMPLE_COMM_AUDIO_AdecToFileProc(void *parg)
{
    SC_S32 s32Ret;

    AUDIO_FRAME_INFO_S audio_frame_info;
    AUDIO_FRAME_S audio_frame;
    audio_frame_info.pstFrame = &audio_frame;
    SAMPLE_ADEC_S *pstAdecCtl = (SAMPLE_ADEC_S *)parg;

    FILE *pwfd0 = (pstAdecCtl->pwfds)[0];
    FILE *pwfd1 = (pstAdecCtl->pwfds)[1];
    SC_S32 s32AdecChn = pstAdecCtl->AdChn;

    while(SC_TRUE == pstAdecCtl->bStart)
    {
        /* get frame from adec chn */
        s32Ret = SC_MPI_ADEC_GetFrame(s32AdecChn, &audio_frame_info, SC_TRUE);
        if (SC_SUCCESS != s32Ret )
        {
            //            pstAdecCtl->bStart = SC_FALSE;
            continue;
        }
        //printf("********%d %s  %p %p %d\n",__LINE__,__FUNCTION__,audio_frame_info.pstFrame->u64VirAddr[0],audio_frame_info.pstFrame->u64VirAddr[1],audio_frame_info.pstFrame->u32Len);

        /* save audio frame to file */

        //      if(audio_frame.pstFrame->enSoundmode != AUDIO_SOUND_MODE_MONO &&
        //          audio_frame.pstFrame->enSoundmode != AUDIO_SOUND_MODE_STEREO ){
        //          return NULL;
        //      }

        //(注意双声道该如何存储,播放数据为LRLR)
        //      (SC_VOID)fwrite(audio_frame.pstFrame.pu8VirAddr[0], 1, audio_frame.pstFrame.u32Len * (audio_frame.pstFrame.enSoundmode+1), pwfd0);
        (SC_VOID)fwrite(audio_frame_info.pstFrame->u64VirAddr[0], 1, audio_frame_info.pstFrame->u32Len, pwfd0);

        /* finally you must release the frame */
        s32Ret = SC_MPI_ADEC_ReleaseFrame(s32AdecChn,  &audio_frame_info);
        if (SC_SUCCESS != s32Ret )
        {
            printf("%s: SC_MPI_ADEC_ReleaseFrame(%d), failed with %#x!\n", \
                __FUNCTION__, s32AdecChn, s32Ret);
            return NULL;
        }
        fflush(pwfd0);
        fflush(pwfd1);

    }

    fclose(pwfd0);
    fclose(pwfd1);
    return NULL;
}

/******************************************************************************
* function : Create the thread to get frame from adec and save to file
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_CreatSaveAdecFrameToFile(SC_S32 AdChn, FILE *pAdcFd0, FILE *pAdcFd1)
{
    SAMPLE_ADEC_S *pstAdec = NULL;

    if (NULL == pAdcFd0 || NULL == pAdcFd1)
    {
        return SC_FAILURE;
    }

    pstAdec = &gs_stSampleAdec[AdChn];
    //pstAdec = &SampleAdecW;
    pstAdec->AdChn = AdChn;
    (pstAdec->pwfds)[0] = pAdcFd0;
    (pstAdec->pwfds)[1] = pAdcFd1;
    pstAdec->bStart = SC_TRUE;
    pthread_create(&pstAdec->stAdPid_out, 0, SAMPLE_COMM_AUDIO_AdecToFileProc, pstAdec);

    return SC_SUCCESS;
}

/******************************************************************************
* function : Destory the thread to get stream from file and send to adec
******************************************************************************/
SC_S32 SAMPLE_COMM_AUDIO_DestroySaveAdecFrameToFile(SC_S32 AdChn)
{
    SAMPLE_ADEC_S *pstAdec = NULL;

    pstAdec = &gs_stSampleAdec[AdChn];
    //pstAdec = &SampleAdecW;
    if (pstAdec->bStart)
    {
        pstAdec->bStart = SC_FALSE;
        //pthread_cancel(pstAdec->stAdPid);
        pthread_join(pstAdec->stAdPid_out, 0);
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_AUDIO_SetStopAdecStatus(SC_S32 AdChn)
{
    gs_stSampleAdec[AdChn].bStart = SC_FALSE;
    return 0;
}
void *SAMPLE_COMM_AUDIO_AencAdecProc(void *parg)
{
    SC_S32 s32Ret;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;
    AUDIO_STREAM_S stStream;

    while (pstAencCtl->bStart)
    {
        /* get stream from aenc chn */
        s32Ret = SC_MPI_AENC_GetStream(pstAencCtl->AeChn, &stStream, 50);
        if (SC_SUCCESS != s32Ret )
        {
            continue;
        }
        else
        {
            /* save audio stream to file */
            if (stStream.pStream != NULL)
            {
                s32Ret = SC_MPI_ADEC_SendStream(pstAencCtl->AeChn, &stStream, SC_FALSE);
            }
        }
        /* finally you must release the stream */
        s32Ret = SC_MPI_AENC_ReleaseStream(pstAencCtl->AeChn, &stStream);
        if (SC_SUCCESS != s32Ret )
        {
            printf("%s: SC_MPI_AENC_ReleaseStream(%d), failed with %#x!\n", \
                __FUNCTION__, pstAencCtl->AeChn, s32Ret);
        }
    }
    return NULL;

}

SC_S32 SAMPLE_COMM_AUDIO_CreatTrdAencAdec(SC_S32 AeChn)
{
    SAMPLE_AENC_S *pstAenc = NULL;
    pstAenc = &gs_stSampleAenc[AeChn];
    pstAenc->AeChn = AeChn;
    //    pstAenc->AdChn = 0;
    pstAenc->bSendAdChn = SC_TRUE;

    pstAenc->bStart = SC_TRUE;
    pthread_create(&pstAenc->stAencPid_out, 0, SAMPLE_COMM_AUDIO_AencAdecProc, pstAenc);

    return SC_SUCCESS;
}


void *SAMPLE_COMM_AUDIO_PCM2AoProc(void *parg)
{
    SAMPLE_AO_S *pstAoCtl = (SAMPLE_AO_S *)parg;
    AUDIO_FRAME_S stFrame;
    SC_S32 s32Ret = SC_SUCCESS;
    SC_U32 u32ReadLen0;
    SC_U32 u32ReadLen1;
    VB_BLK VbBlk[2];

    FILE *chn0_fd = pstAoCtl->pfd[0]; 

    SC_U32 u32Len = 320 * 2;
    memset(&stFrame, 0, sizeof(AUDIO_FRAME_S));

    VB_POOL_CONFIG_S stVbPoolCfg;
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Len*2;
    stVbPoolCfg.u32BlkCnt = 8;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    for(int i = 0; i < 2; i++)
    {
        stFrame.u32PoolId[i] = SC_MPI_VB_CreatePool(&stVbPoolCfg);
        if (stFrame.u32PoolId[i] == VB_INVALID_POOLID)
        {
            printf("Maybe you not call sys init\n");
            return NULL;
        }
        VbBlk[i] = SC_MPI_VB_GetBlock(stFrame.u32PoolId[i], u32Len, SC_NULL);
        if (VB_INVALID_HANDLE == VbBlk[i])
        {
            SAMPLE_PRT("SC_MPI_VB_GetBlock err! size:%d\n", u32Len);
            return NULL;
        }
        stFrame.u64PhyAddr[i] = SC_MPI_VB_Handle2PhysAddr(VbBlk[i]);
        if (0 == stFrame.u64PhyAddr[i])
        {
            SAMPLE_PRT("SC_MPI_VB_Handle2PhysAddr err!\n");
            return NULL;
        }
        stFrame.u64VirAddr[i] = (SC_U8 *)SC_MPI_SYS_Mmap(stFrame.u64PhyAddr[i], u32Len);
        if (NULL == stFrame.u64VirAddr[i])
        {
            SAMPLE_PRT("SC_MPI_SYS_Mmap err!\n");
            return NULL;
        }
    }
    //printf("**%d****poolID%d %d VbBlk%d %d\n",__LINE__,stFrame.u32PoolId[0],stFrame.u32PoolId[1],VbBlk[0],VbBlk[1]);

    stFrame.enBitwidth = AUDIO_BIT_WIDTH_16;
    stFrame.enSoundmode = AUDIO_SOUND_MODE_STEREO;
    stFrame.u32Len = u32Len;
    while(pstAoCtl->bStart)
    {    
        if(SC_ERR_AENC_NOBUF != s32Ret)
        {
            u32ReadLen0 = fread(stFrame.u64VirAddr[0], 1, u32Len, chn0_fd);
            u32ReadLen0 = fread(stFrame.u64VirAddr[1], 1, u32Len, chn0_fd);
           
            stFrame.u32Seq++;
            if(u32ReadLen0 != u32Len)
            {
                printf("%s: fread0(%d) but expect (%d)\n", __FUNCTION__, u32ReadLen0, u32Len);
                printf("END!\n");
                break;
            }
        }
      
        s32Ret = SC_MPI_AO_SendFrame(0,0, &stFrame, 1000);
       
        if (SC_SUCCESS != s32Ret )
        {
            printf("%s: SC_MPI_AO_SendFrame(), failed with %#x!\n", \
                __FUNCTION__, s32Ret);
            //pstAiCtl->bStart = SC_FALSE;
            //return NULL;
        }

        
        /*** 如果取码流较慢，编码较快时，会导致编码缓存不足，建议加延时 ***/
        usleep(20000);
    }
    gs_stSampleAo[0].bStart = SC_FALSE;

    SC_MPI_SYS_Munmap(stFrame.u64VirAddr[0], u32Len);
    SC_MPI_SYS_Munmap(stFrame.u64VirAddr[1], u32Len);
    SC_MPI_VB_ReleaseBlock(VbBlk[0]);
    SC_MPI_VB_ReleaseBlock(VbBlk[1]);    
    SC_MPI_VB_DestroyPool(stFrame.u32PoolId[0]);
    SC_MPI_VB_DestroyPool(stFrame.u32PoolId[1]);


    fclose(chn0_fd);
    return NULL;
}

SC_S32 SAMPLE_COMM_AUDIO_CreatTrdPCM2Ao(FILE *pFd0)
{   

    if (NULL == pFd0)
    {
        return SC_FAILURE;
    }

    SAMPLE_AO_S *pstAoCtl = NULL;

    pstAoCtl =  &gs_stSampleAo[0];
    pstAoCtl->AoDev =  0;
    pstAoCtl->bStart = SC_TRUE;
    pstAoCtl->pfd[0] = pFd0;
    pthread_create(&pstAoCtl->stAoPid, 0, SAMPLE_COMM_AUDIO_PCM2AoProc, pstAoCtl);

    return SC_SUCCESS;
}

