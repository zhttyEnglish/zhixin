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
#include "sample_audio.h"
#include "sample_comm_audio.h"

#include "sample_comm.h"
#define AACLD_SAMPLES_PER_FRAME         512
#define AACLC_SAMPLES_PER_FRAME         1024
#define AACPLUS_SAMPLES_PER_FRAME       2048

static SC_BOOL gs_bUserGetMode  = SC_FALSE;
static SC_BOOL gs_bAiVolumeCtrl  = SC_FALSE;

/******************************************************************************
* function : PT Number to String
******************************************************************************/
static char *SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
    if (PT_G711A == enType)
    {
        return "g711a";
    }
    else if (PT_G711U == enType)
    {
        return "g711u";
    }
    else if (PT_ADPCMA == enType)
    {
        return "adpcm";
    }
    else if (PT_G726 == enType)
    {
        return "g726";
    }
    else if (PT_LPCM == enType)
    {
        return "pcm";
    }
    else if (PT_AAC == enType)
    {
        return "aac";
    }
    else
    {
        return "data";
    }
}
PAYLOAD_TYPE_E SAMPLE_AUDIO_Index2Payload(int index)
{
    if (0 == index)
    {
        return PT_G711A;
    }
    else if (1 == index)
    {
        return PT_G711U;
    }
    else if (2 == index)
    {
        return PT_G726;
    }
    else if (3 == index)
    {
        return PT_AAC;
    }
    else if (4 == index)
    {
        return PT_ADPCMA;
    }
    else
    {
        return PT_BUTT;
    }
}
SC_S32 SAMPLE_AUDIO_PCmfile2Ao()
{
    //输入pcm文件
    FILE *prfd0;
    char *inFileName0 = "record_l.pcm";
    SC_S32      s32AencChnCnt;
    prfd0 = fopen(inFileName0, "r+");
    if (NULL == prfd0)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, inFileName0);
        return -1;
    }
    printf("open in pcm file:\"%s\" for adec ok\n", inFileName0);


    SC_S32      s32Ret;  
    SC_U32 s32AoChnCnt;
    AUDIO_DEV   AiDev = 0;
    AUDIO_DEV   AoDev = 0;
    SC_S32      AiChn = 0;
    SC_S32      AoChn = 0;

    AIO_ATTR_S stAioAttr;
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_STEREO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 8;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt      = 2;
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;
    s32AoChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    SAMPLE_COMM_AUDIO_StartAo(AoDev, &stAioAttr);

    SC_MPI_AO_SetVolume(0,5);//建议调节范围限制为[-2, 8]

    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdPCM2Ao(prfd0);
    if (s32Ret != SC_SUCCESS)
    {
        goto ADEC_ERR2;
    }



    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();


ADEC_ERR3:


ADEC_ERR2:

ADEC_ERR1:

    s32Ret |= SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAo failed! %d\n", s32Ret);
    }

    return s32Ret;
}

SC_S32 SAMPLE_AUDIO_PCmfile2Aencfile(PAYLOAD_TYPE_E enPayloadType)
{
    //输入pcm文件
    FILE *prfd0;
    char *inFileName0 = "record_l.pcm";
    SC_S32      s32AencChnCnt;
    prfd0 = fopen(inFileName0, "r+");
    if (NULL == prfd0)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, inFileName0);
        return -1;
    }
    printf("open in pcm file:\"%s\" for adec ok\n", inFileName0);

    FILE *prfd1;
    char *inFileName1 = "record_r.pcm";
    prfd1 = fopen(inFileName1, "r+");
    if (NULL == prfd1)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, inFileName1);
        return -1;
    }
    printf("open in pcm file:\"%s\" for adec ok\n", inFileName1);

    //输出编码文件
    FILE *pwfd;
    char *payload_type = SAMPLE_AUDIO_Pt2Str(enPayloadType);
    char outFileName[1024] = "aenc_out.";
    strcat(outFileName, payload_type);
    pwfd = fopen(outFileName, "w+");
    if (NULL == pwfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, outFileName);
        return -1;
    }
    printf("open out file:\"%s\" for aenc ok\n", outFileName);

    SC_S32      s32Ret;
    SC_S32      AenChn = 0;

    AIO_ATTR_S stAioAttr;
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    //    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt      = 1;
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc2(0, &stAioAttr, enPayloadType,G726_16K);
    if (s32Ret != SC_SUCCESS)
    {
        goto ADEC_ERR1;
    }

    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAenc(AenChn, prfd0, prfd1);
    if (s32Ret != SC_SUCCESS)
    {
        goto ADEC_ERR2;
    }

    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencFile(AenChn, pwfd);
    if (s32Ret != SC_SUCCESS)
    {
        goto ADEC_ERR3;
    }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    SAMPLE_COMM_AUDIO_SetStopAencStatus(AenChn);

ADEC_ERR3:
    s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAencFile(AenChn);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_AUDIO_DestoryTrdAencFile!\n");
    }

ADEC_ERR2:
    s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAenc(AenChn);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_AUDIO_DestoryTrdAenc!\n");
    }

ADEC_ERR1:
    s32Ret = SAMPLE_COMM_AUDIO_StopAenc(1);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_AUDIO_StopAenc failed!\n");
    }

    return s32Ret;
}

SC_S32 SAMPLE_AUDIO_Aencfile2PCmfile(PAYLOAD_TYPE_E enPayloadType)
{
    //输入stream文件
    FILE *prfd;
    char *payload_type = SAMPLE_AUDIO_Pt2Str(enPayloadType);
    char inFileName[1024] = "aenc_out.";
    strcat(inFileName, payload_type);
    prfd = fopen(inFileName, "r+");
    if (NULL == prfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, inFileName);
        return -1;
    }
    printf("open in stream file:\"%s\" for adec ok\n", inFileName);

    /*
        //输出pcm文件(LRLRLRLR 整体保存)
        FILE* pwfd;
        char* outFileName = "adec_out.pcm";
        pwfd = fopen(outFileName, "w+");
        if (NULL == pwfd)
        {
            printf("%s: open file %s failed\n", __FUNCTION__, outFileName);
            return -1;
        }
        printf("open out file:\"%s\" for adec ok\n", outFileName);
    */

    //输出pcm文件(LLL..RRR... 通道分别保存)
    FILE *pwfds0;
    char *outFileNames0 = "adec_out_l.pcm";
    pwfds0 = fopen(outFileNames0, "w+");
    if (NULL == pwfds0)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, outFileNames0);
        return -1;
    }
    printf("open out file:\"%s\" for adec ok\n", outFileNames0);

    FILE *pwfds1;
    char *outFileNames1 = "adec_out_r.pcm";
    pwfds1 = fopen(outFileNames1, "w+");
    if (NULL == pwfds1)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, outFileNames1);
        return -1;
    }
    printf("open out file:\"%s\" for adec ok\n", outFileNames1);

    SC_S32      s32Ret;
    SC_S32      AdeChn = 0;
    SC_S32      s32AdecChnCnt;
    AIO_ATTR_S stAioAttr;
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    //stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt      = 1;
    s32AdecChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdeChn, &stAioAttr,enPayloadType);
    if (s32Ret != SC_SUCCESS)
    {
        goto ADEC_ERR1;
    }

    s32Ret = SAMPLE_COMM_AUDIO_CreatGetTrdFileAdec(AdeChn, prfd);
    if (s32Ret != SC_SUCCESS)
    {
        goto ADEC_ERR2;
    }

    s32Ret = SAMPLE_COMM_AUDIO_CreatSaveAdecFrameToFile(AdeChn, pwfds0, pwfds1);
    if (s32Ret != SC_SUCCESS)
    {
        goto ADEC_ERR3;
    }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    SAMPLE_COMM_AUDIO_SetStopAdecStatus(AdeChn);

ADEC_ERR3:
    s32Ret = SAMPLE_COMM_AUDIO_DestroySaveAdecFrameToFile(AdeChn);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_AUDIO_DestoryTrdFileAdec!\n");
    }

ADEC_ERR2:
    s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(s32AdecChnCnt);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_AUDIO_DestoryTrdFileAdec!\n");
    }

ADEC_ERR1:
    s32Ret = SAMPLE_COMM_AUDIO_StopAdec(s32AdecChnCnt);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_AUDIO_StopAdec failed!\n");
    }

    return s32Ret;

}

SC_S32 SAMPLE_AUDIO_AiAo(int volume)
{
    SC_S32 ret = -1;
    AI_CHN_PARAM_S stAiChnPara;
    SC_U32 s32AiChnCnt;
    SC_U32 s32AoChnCnt;
    AUDIO_DEV   AiDev = 0;
    AUDIO_DEV   AoDev = 0;
    SC_S32      AiChn = 0;
    SC_S32      AoChn = 0;

    AIO_ATTR_S stAioAttr;
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_STEREO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 8;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt      = 2;
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;
    s32AiChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32AoChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    SAMPLE_COMM_AUDIO_StartAi(AiDev, &stAioAttr);
    SAMPLE_COMM_AUDIO_StartAo(AoDev, &stAioAttr);

    ret = SC_MPI_AI_GetChnParam(AiDev, AiChn, &stAiChnPara);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("SC_MPI_AI_GetChnParam failed! %d\n", ret);
        return ret;
    }
    stAiChnPara.u32UsrFrmDepth = 8;
    ret = SC_MPI_AI_SetChnParam(AiDev, AiChn, &stAiChnPara);
    if (SC_SUCCESS != ret)
    {
        SAMPLE_PRT("SC_MPI_AI_SetChnParam failed! %d\n", ret);
        return ret;
    }

    SC_MPI_AI_SetVolume(volume);

    
    SC_MPI_AO_SetVolume(0,5);//建议调节范围限制为[-2, 8]

    if (SC_TRUE == gs_bUserGetMode)
    {
        ret = SAMPLE_COMM_AUDIO_CreatTrdAiAo(AiDev, AiChn, AoDev, AoChn);
        if (ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_AUDIO_CreatTrdAiAo failed! %d\n", ret);
            goto AIAO_ERR1;
        }
    }
    else
    {
        ret = SAMPLE_COMM_AI_Bind_AO(AiDev, AiChn, AoDev, AoChn);
        if (ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_AI_Bind_AO failed! %d\n", ret);
            goto AIAO_ERR1;
        }
    }
    printf("ai(%d,%d) bind to ao(%d,%d) ok\n", AiDev, AiChn, AoDev, AoChn);

    if (SC_TRUE == gs_bAiVolumeCtrl)
    {
        ret = SAMPLE_COMM_AUDIO_CreatTrdAiVolCtrl(AiDev);
        if (ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_AUDIO_CreatTrdAoVolCtrl failed! %d\n", ret);
            goto AIAO_ERR0;
        }
    }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    if (SC_TRUE == gs_bAiVolumeCtrl)
    {
        ret = SAMPLE_COMM_AUDIO_DestoryTrdAiVolCtrl(AiDev);
        if (ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_AUDIO_DestoryTrdAiVolCtrl failed! %d\n", ret);
            return SC_FAILURE;
        }
    }

AIAO_ERR0:

    if (SC_TRUE == gs_bUserGetMode)
    {
        ret = SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
        if (ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_AUDIO_DestoryTrdAi failed! %d\n", ret);
        }
    }
    else
    {

        ret = SAMPLE_COMM_AI_UnBind_AO(AiDev, AiChn, AoDev, AoChn);
        if (ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_AI_UnBind_AO failed! %d\n", ret);
        }
    }

AIAO_ERR1:
    sleep(1);
    ret |= SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt);
    if (ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAi failed! %d\n", ret);
    }
    ret |= SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt);
    if (ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAo failed! %d\n", ret);
    }
    return ret;
}


SC_S32 SAMPLE_AUDIO_AiAenc1(SC_S32 AiChnIndex/*board1:1  board2:0           board4:0*/,PAYLOAD_TYPE_E enPayloadType)
{
    SC_S32 i, j, s32Ret;
    AUDIO_DEV   AiDev = 0;
    SC_S32      AiChn = AiChnIndex;// board1:1   board4:0
    AUDIO_DEV   AencDev = 0;

    SC_S32      s32AiChnCnt;
    SC_S32      s32AencChnCnt;

    AIO_ATTR_S stAioAttr;
    AI_CHN_PARAM_S stAiChnPara;

    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 320;//for AAC PayloadType,this must be 1024
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;
    if(enPayloadType==PT_AAC)
    {
        stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_44100;
        stAioAttr.u32PtNumPerFrm = 1024;
    }

    /********************************************
      start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = SAMPLE_COMM_AUDIO_StartAi2(AiDev, AiChn, &stAioAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StartAi failed! %d\n", s32Ret);
        goto AIAENC_ERR6;
    }
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    s32Ret = SC_MPI_AI_GetChnParam(AiDev, AiChn, &stAiChnPara);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_AI_GetChnParam failed! %d\n", s32Ret);
        return s32Ret;
    }
    stAiChnPara.u32UsrFrmDepth = 8;
    s32Ret = SC_MPI_AI_SetChnParam(AiDev, AiChn, &stAiChnPara);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_AI_SetChnParam failed! %d\n", s32Ret);
        return s32Ret;
    }
    
    SC_MPI_AI_SetVolume(40);

    s32Ret = SAMPLE_COMM_AUDIO_StartAenc2(0, &stAioAttr, enPayloadType, MEDIA_G726_40K);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR4;
    }
    /********************************************
      Aenc bind Ai Chn
    ********************************************/
    SAMPLE_COMM_AI_Bind_AENC(AiDev, AiChn, AencDev, 0);
    //    SAMPLE_COMM_AI_Bind_AENC(AiDev, AiChn, AencDev, 1);
    printf("Ai(%d,%d) bind to AencChn ok!\n", AiDev, AiChn);

    //输出编码文件
    FILE *pwfd0;
    char outFileName0[1024] = "aenc_out.0";
    pwfd0 = fopen(outFileName0, "w+");
    printf("open out file:\"%s\" for aenc ok\n", outFileName0);
    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencFile(0, pwfd0);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR2;
    }
    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

AIAENC_ERR2:
    SAMPLE_COMM_AI_UnBind_AENC(AiDev, AiChn, AencDev, 0);
    //    SAMPLE_COMM_AI_UnBind_AENC(AiDev, AiChn, AencDev, 1);

AIAENC_ERR3:
    SAMPLE_COMM_AUDIO_DestoryTrdAencFile(0);
    //    SAMPLE_COMM_AUDIO_DestoryTrdAencFile(1);

AIAENC_ERR4:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAenc(1);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAenc failed! %d\n", s32Ret);
    }

AIAENC_ERR5:
    SC_MPI_AI_DisableChn(AiDev, AiChn);
    SC_MPI_AI_Disable(AiDev);

AIAENC_ERR6:

    return s32Ret;
}

SC_S32 SAMPLE_AUDIO_AiAenc2(SC_S32 AiChnIndex/*board1:1  board2:0           board4:0*/,PAYLOAD_TYPE_E enPayloadType0,PAYLOAD_TYPE_E enPayloadType1)
{
    SC_S32 i, j, s32Ret;
    AUDIO_DEV   AiDev = 0;
    SC_S32      AiChn = AiChnIndex;
    AUDIO_DEV   AencDev = 0;

    SC_S32      s32AiChnCnt;
    SC_S32      s32AencChnCnt;

    AIO_ATTR_S stAioAttr;
    AI_CHN_PARAM_S stAiChnPara;

    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_48000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 1024;//for AAC PayloadType,this must be 1024
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;

    /********************************************
      start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = SAMPLE_COMM_AUDIO_StartAi2(AiDev, AiChn, &stAioAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StartAi failed! %d\n", s32Ret);
        goto AIAENC_ERR6;
    }
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    s32Ret = SC_MPI_AI_GetChnParam(AiDev, AiChn, &stAiChnPara);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_AI_GetChnParam failed! %d\n", s32Ret);
        return s32Ret;
    }
    stAiChnPara.u32UsrFrmDepth = 8;
    s32Ret = SC_MPI_AI_SetChnParam(AiDev, AiChn, &stAiChnPara);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_AI_SetChnParam failed! %d\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_AUDIO_StartAenc2(0, &stAioAttr, enPayloadType0, MEDIA_G726_16K);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR4;
    }

    s32Ret = SAMPLE_COMM_AUDIO_StartAenc2(1, &stAioAttr, enPayloadType1, G726_16K);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR4;
    }

    /********************************************
      Aenc bind Ai Chn
    ********************************************/
    SAMPLE_COMM_AI_Bind_AENC(AiDev, AiChn, AencDev, 0);
    SAMPLE_COMM_AI_Bind_AENC(AiDev, AiChn, AencDev, 1);
    printf("Ai(%d,%d) bind to AencChn ok!\n", AiDev, AiChn);

    //输出编码文件
    FILE *pwfd0;
    FILE *pwfd1;
    char outFileName0[1024] = "aenc_out.0";
    char outFileName1[1024] = "aenc_out.1";
    pwfd0 = fopen(outFileName0, "w+");
    pwfd1 = fopen(outFileName1, "w+");
    printf("open out file:\"%s\" for aenc ok\n", outFileName0);
    printf("open out file:\"%s\" for aenc ok\n", outFileName1);
    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencFile(0, pwfd0);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR2;
    }
    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencFile(1, pwfd1);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR2;
    }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

AIAENC_ERR2:
    SAMPLE_COMM_AI_UnBind_AENC(AiDev, AiChn, AencDev, 0);
    SAMPLE_COMM_AI_UnBind_AENC(AiDev, AiChn, AencDev, 1);

AIAENC_ERR3:
    SAMPLE_COMM_AUDIO_DestoryTrdAencFile(0);
    SAMPLE_COMM_AUDIO_DestoryTrdAencFile(1);

AIAENC_ERR4:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAenc(2);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAenc failed! %d\n", s32Ret);
    }

AIAENC_ERR5:
    SC_MPI_AI_DisableChn(AiDev, AiChn);
    SC_MPI_AI_Disable(AiDev);

AIAENC_ERR6:

    return s32Ret;
}

/******************************************************************************
* function : file -> Adec -> Ao
******************************************************************************/
SC_S32 SAMPLE_AUDIO_AdecAo(PAYLOAD_TYPE_E enPayloadType)
{
    SC_S32      s32Ret;
    AUDIO_DEV   AoDev = 0;
    AUDIO_DEV   AdecDev = 0;
    SC_S32      AdeChn = 0;
    SC_S32      s32AoChnCnt;
    SC_S32      s32AdecChnCnt;

    //输入stream文件
    FILE *prfd;
    char *payload_type = SAMPLE_AUDIO_Pt2Str(enPayloadType);
    char inFileName[1024] = "aenc_out.";
    strcat(inFileName, payload_type);
    prfd = fopen(inFileName, "r+");
    if (NULL == prfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, inFileName);
        return -1;
    }
    printf("open in stream file:\"%s\" for adec ok\n", inFileName);

    AIO_ATTR_S stAioAttr;
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 8;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;

    if(enPayloadType==PT_AAC)
    {
        stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_44100;
        stAioAttr.u32PtNumPerFrm = 1024;
    }

    s32AoChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32AdecChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdeChn, &stAioAttr,enPayloadType);
    if (s32Ret != SC_SUCCESS)
    {
		SAMPLE_PRT("SAMPLE_COMM_AUDIO_StartAdec failed! %d\n", s32Ret);
        goto ADEC_ERR3;
    }

    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, &stAioAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StartAo failed! %d\n", s32Ret);
        goto ADEC_ERR2;
    }

    SC_MPI_AO_SetVolume(0,5);//建议调节范围限制为[-2, 8]

    for (int i = 0; i < s32AdecChnCnt; i++)
    {
        s32Ret = SAMPLE_COMM_ADEC_Bind_AO(AdecDev, i, AoDev, i);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_ADEC_Bind_AO failed! %d\n", s32Ret);
            SAMPLE_COMM_ADEC_UnBind_AO(AdecDev, i, AoDev, i);
            goto ADEC_ERR2;
        }
        printf("Adec (chn:%d) bind to Ao:%d ok!\n", i, i);
    }
    
    //SC_MPI_AO_SetVolume(0,5);//建议调节范围限制为[-2, 8]

    s32Ret = SAMPLE_COMM_AUDIO_CreatGetTrdFileAdec(AdeChn, prfd);
    if (s32Ret != SC_SUCCESS)
    {
        goto ADEC_ERR1;
    }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    //SAMPLE_COMM_AUDIO_SetStopAdecStatus(AdeChn);
ADEC_ERR1:
    s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(s32AdecChnCnt);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_AUDIO_DestoryTrdFileAdec!\n");
    }
    for (int i = 0; i < s32AdecChnCnt; i++)
    {
        s32Ret = SAMPLE_COMM_ADEC_UnBind_AO(AdecDev, i, AoDev, i);
        if (s32Ret != SC_SUCCESS)
        {
            printf("SAMPLE_COMM_ADEC_UnBind_AO failed!\n");
        }
    }
ADEC_ERR2:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAo failed! %d\n", s32Ret);
    }

ADEC_ERR3:
    s32Ret = SAMPLE_COMM_AUDIO_StopAdec(s32AdecChnCnt);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_AUDIO_StopAdec failed!\n");
    }

    return s32Ret;

}


SC_S32 SAMPLE_AUDIO_AiAencAdecAo(SC_S32 AiChnIndex/*board1:1  board2:0             board4:0*/,PAYLOAD_TYPE_E enPayloadType)
{
    SC_S32 i, j, s32Ret;
    AUDIO_DEV   AiDev = 0;
    SC_S32      AiChn = AiChnIndex;
    AUDIO_DEV   AencDev = 0;
    
    AUDIO_DEV   AoDev = 0;
    AUDIO_DEV   AdecDev = 0;
    SC_S32      AdeChn = 0;
    SC_S32      s32AoChnCnt;
    SC_S32      s32AdecChnCnt;

    SC_S32      s32AiChnCnt;
    SC_S32      s32AencChnCnt;

    AIO_ATTR_S stAioAttr;
    AI_CHN_PARAM_S stAiChnPara;

    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_48000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 8;
    stAioAttr.u32PtNumPerFrm = 320;//for AAC PayloadType,this must be 1024
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;
    if(enPayloadType==PT_AAC)
    {
        stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_44100;
        stAioAttr.u32PtNumPerFrm = 1024;
    }

    /********************************************
      start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = SAMPLE_COMM_AUDIO_StartAi2(AiDev, AiChn, &stAioAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StartAi failed! %d\n", s32Ret);
        goto AIAENC_ERR6;
    }
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    s32Ret = SC_MPI_AI_GetChnParam(AiDev, AiChn, &stAiChnPara);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_AI_GetChnParam failed! %d\n", s32Ret);
        return s32Ret;
    }
    stAiChnPara.u32UsrFrmDepth = 8;
    s32Ret = SC_MPI_AI_SetChnParam(AiDev, AiChn, &stAiChnPara);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_AI_SetChnParam failed! %d\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_AUDIO_StartAenc2(0, &stAioAttr, enPayloadType, MEDIA_G726_40K);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR4;
    }
    /********************************************
      Aenc bind Ai Chn
    ********************************************/
    SAMPLE_COMM_AI_Bind_AENC(AiDev, AiChn, AencDev, 0);
    //    SAMPLE_COMM_AI_Bind_AENC(AiDev, AiChn, AencDev, 1);
    printf("Ai(%d,%d) bind to AencChn ok!\n", AiDev, AiChn);



    s32AoChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32AdecChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdeChn, &stAioAttr,enPayloadType);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR2;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, &stAioAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StartAo failed! %d\n", s32Ret);
        goto AIAENC_ERR2;
    }



    for (int i = 0; i < s32AdecChnCnt; i++)
    {
        s32Ret = SAMPLE_COMM_ADEC_Bind_AO(AdecDev, i, AoDev, i);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_ADEC_Bind_AO failed! %d\n", s32Ret);
            SAMPLE_COMM_ADEC_UnBind_AO(AdecDev, i, AoDev, i);
            goto AIAENC_ERR2;
        }
        printf("Adec (chn:%d) bind to Ao:%d ok!\n", i, i);
    }



    SC_MPI_AI_SetVolume(20);
    SC_MPI_AO_SetVolume(0,3);//建议调节范围限制为[-2, 8]



    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencAdec(0);
    if (s32Ret != SC_SUCCESS)
    {
        goto AIAENC_ERR2;
    }
    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

AIAENC_ERR2:
    SAMPLE_COMM_AI_UnBind_AENC(AiDev, AiChn, AencDev, 0);
    //    SAMPLE_COMM_AI_UnBind_AENC(AiDev, AiChn, AencDev, 1);

AIAENC_ERR3:
    SAMPLE_COMM_AUDIO_DestoryTrdAencFile(0);
    //    SAMPLE_COMM_AUDIO_DestoryTrdAencFile(1);

AIAENC_ERR4:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAenc(1);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAenc failed! %d\n", s32Ret);
    }

AIAENC_ERR5:
    SC_MPI_AI_DisableChn(AiDev, AiChn);
    SC_MPI_AI_Disable(AiDev);

AIAENC_ERR6:

    return s32Ret;
}
SC_VOID SAMPLE_AUDIO_Usage(SC_VOID)
{
    printf(
        "\n"
        "********************************************************************************************************\n"
        "Sample index and its function list below\n"
        "    0:  start AI to AO loop\n"
        "        Usage: ./sample_audio_main 0 [Volume]\n"
        "    1:  send audio frame to AENC from AI, one channel\n"
        "        Usage: ./sample_audio_main 1 [aiChnIndex] [PayloadType0]\n"
        "    2:  send audio frame to AENC from AI, two channels\n"
        "        Usage: ./sample_audio_main 2 [aiChnIndex] [PayloadType0] [PayloadType1]\n"
        "    3:  read audio stream from file, decode and send to AO\n"
        "        Usage: ./sample_audio_main 3 [PayloadType]\n"
        "    4:  read PCmfile, encode to Aencfile\n"
        "        Usage: ./sample_audio_main 4 [PayloadType]\n"
        "    5:  read Aencfile, decode to PCmfile\n"
        "        Usage: ./sample_audio_main 5 [PayloadType]\n"
        "    6:  read Ai->Aenc->Adec->Ao loop\n"
        "        Usage: ./sample_audio_main 6 [aiChnIndex] [PayloadType]\n"
        
        "PayloadType list below:\n"
        "    0:  G711A\n"
        "    1:  G711U\n"
        "    2:  G726\n"
        "    4:  PCM\n"
        
        "Example:\n"
        "e.g:  ./sample_audio_main 0 40     ai->ao aiVolume: 40\n"
        "e.g:  ./sample_audio_main 1 1 1    ai->aencFile aiChnIndex: 1  PayloadType: G711U\n"
        "e.g:  ./sample_audio_main 2 0 0 2  ai->aencFile aiChnIndex: 0  PayloadType0: G711A PayloadType1: G726\n"
        "********************************************************************************************************\n"
        "\n");
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_AUDIO_HandleSig(SC_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        //SAMPLE_COMM_AUDIO_DestoryAllTrd();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }
    exit(0);
}

SC_S32 main(int argc, char *argv[])
{
    SC_S32 s32Ret = SC_SUCCESS;
    SC_U32 u32Index = 0;
    SC_U32 AiChnIndex = 0;
    SC_U32 payload_index0 = 0;
    SC_U32 payload_index1 = 0;
    PAYLOAD_TYPE_E enPayloadType0;
    PAYLOAD_TYPE_E enPayloadType1;
    if (2 > argc)
    {
        SAMPLE_AUDIO_Usage();
        return SC_FAILURE;
    }

    u32Index = atoi(argv[1]);
    if (u32Index > 7)
    {
        SAMPLE_AUDIO_Usage();
        return SC_FAILURE;
    }

    signal(SIGINT, SAMPLE_AUDIO_HandleSig);
    signal(SIGTERM, SAMPLE_AUDIO_HandleSig);

    VB_CONFIG_S        st_vb_config;
    memset(&st_vb_config, 0, sizeof(st_vb_config));
    st_vb_config.astCommPool[0].u64BlkSize = 2 * 2048 * 1080;
    st_vb_config.astCommPool[0].u32BlkCnt = 4;
    st_vb_config.astCommPool[1].u64BlkSize = 2 * 1280 * 720;
    st_vb_config.astCommPool[1].u32BlkCnt = 10;
    st_vb_config.astCommPool[2].u64BlkSize = 4096;
    st_vb_config.astCommPool[2].u32BlkCnt = 100;
    st_vb_config.u32MaxPoolCnt = 4;
    s32Ret = SAMPLE_COMM_SYS_Init(&st_vb_config);
    if (SC_SUCCESS != s32Ret)
    {
        printf("%s: system init failed with %d!\n", __FUNCTION__, s32Ret);
        return SC_FAILURE;
    }
    int volume =17;
    switch (u32Index)
    {
    case 0:
    {
        if (argc>2)
        {
            volume = atoi(argv[2]);
        }        
        SAMPLE_AUDIO_AiAo(volume);
        break;
    }
    case 1:
    {
        if (argc<4)
        {
            SAMPLE_AUDIO_Usage();
            return SC_FAILURE;
        }  
        AiChnIndex = atoi(argv[2]);
        enPayloadType0 = SAMPLE_AUDIO_Index2Payload(atoi(argv[3]));
        SAMPLE_AUDIO_AiAenc1(AiChnIndex,enPayloadType0);
        break;
    }
    case 2:
    {
        if (argc<5)
        {
            SAMPLE_AUDIO_Usage();
            return SC_FAILURE;
        }  
        AiChnIndex = atoi(argv[2]);
        enPayloadType0 = SAMPLE_AUDIO_Index2Payload(atoi(argv[3]));
        enPayloadType1 = SAMPLE_AUDIO_Index2Payload(atoi(argv[4]));
        SAMPLE_AUDIO_AiAenc2(AiChnIndex,enPayloadType0,enPayloadType1);
        break;
    }
    case 3:
    {
        if (argc<3)
        {
            SAMPLE_AUDIO_Usage();
            return SC_FAILURE;
        }  
        enPayloadType0 = SAMPLE_AUDIO_Index2Payload(atoi(argv[2]));
        SAMPLE_AUDIO_AdecAo(enPayloadType0);
        break;
    }
    case 4:
    {
        if (argc<3)
        {
            SAMPLE_AUDIO_Usage();
            return SC_FAILURE;
        }  
        enPayloadType0 = SAMPLE_AUDIO_Index2Payload(atoi(argv[2]));
        SAMPLE_AUDIO_PCmfile2Aencfile(enPayloadType0);
        break;
    }
    case 5:
    {
        if (argc<3)
        {
            SAMPLE_AUDIO_Usage();
            return SC_FAILURE;
        }  
        enPayloadType0 = SAMPLE_AUDIO_Index2Payload(atoi(argv[2]));
        SAMPLE_AUDIO_Aencfile2PCmfile(enPayloadType0);
        break;
    }
    case 6:
    {
        if (argc<4)
        {
            SAMPLE_AUDIO_Usage();
            return SC_FAILURE;
        }  
        AiChnIndex = atoi(argv[2]);
        enPayloadType0 = SAMPLE_AUDIO_Index2Payload(atoi(argv[3]));
        SAMPLE_AUDIO_AiAencAdecAo(AiChnIndex,enPayloadType0);
        break;
    }
    case 7:
    {
        SAMPLE_AUDIO_PCmfile2Ao();
        break;
    }    
    default:
    {
        break;
    }
    SAMPLE_COMM_SYS_Exit();
    }
    return s32Ret;
}

