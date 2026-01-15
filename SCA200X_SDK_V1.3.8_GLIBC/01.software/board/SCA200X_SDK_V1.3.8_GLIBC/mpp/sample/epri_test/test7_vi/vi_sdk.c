#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <errno.h>

#include "sample_comm.h"
#include "vi_mat.h"

//#define VISDK_ENCODEJPG 1

int visdk_getjpg(int VencChn, int SnapCnt, char *pdata, int *size)
{
    struct timeval TimeoutVal;
    fd_set read_fds;
    int s32VencFd;
    VENC_CHN_STATUS_S stStat;
    VENC_STREAM_S stStream;
    int s32Ret;
    VENC_RECV_PIC_PARAM_S  stRecvParam;
    int i,j;

    /******************************************
     step 2:  Start Recv Venc Pictures
    ******************************************/
    stRecvParam.s32RecvPicNum = SnapCnt;
    s32Ret = SC_MPI_VENC_StartRecvFrame(VencChn, &stRecvParam);
    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
        return SC_FAILURE;
    }
    /******************************************
     step 3:  recv picture
    ******************************************/
    s32VencFd = SC_MPI_VENC_GetFd(VencChn);
    if (s32VencFd < 0)
    {
        printf("SC_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
        return SC_FAILURE;
    }

    for(i = 0; i < SnapCnt; i++)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32VencFd, &read_fds);
        TimeoutVal.tv_sec  = 10;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            printf("snap select failed!\n");
            return SC_FAILURE;
        }
        else if (0 == s32Ret)
        {
            printf("snap time out!\n");
            return SC_FAILURE;
        }
        else
        {
            if (FD_ISSET(s32VencFd, &read_fds))
            {
                s32Ret = SC_MPI_VENC_QueryStatus(VencChn, &stStat);
                if (s32Ret != SC_SUCCESS)
                {
                    printf("SC_MPI_VENC_QueryStatus failed with %#x!\n", s32Ret);
                    return s32Ret;
                }
                /*******************************************************
                suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
                 if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
                 {                SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                    return SC_SUCCESS;
                 }
                 *******************************************************/
                if (0 == stStat.u32CurPacks)
                {
                    printf("NOTE: Current  frame is NULL!\n");
                    return SC_SUCCESS;
                }
                stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                if (NULL == stStream.pstPack)
                {
                    printf("malloc memory failed!\n");
                    return SC_FAILURE;
                }
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = SC_MPI_VENC_GetStream(VencChn, &stStream, -1);
                if (s32Ret)
                {
                    printf("SC_MPI_VENC_GetStream failed with %#x!\n", s32Ret);

                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    return s32Ret;
                }

                int offset = 0;
                for (j = 0; j < stStream.u32PackCount; j++)
                {
                    memcpy(pdata + offset, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset,
                        stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
                    offset += (stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
                }

                *size = offset;

                s32Ret = SC_MPI_VENC_ReleaseStream(VencChn, &stStream);
                if (s32Ret)
                {
                    printf("SC_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);

                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    return s32Ret;
                }

                free(stStream.pstPack);
                stStream.pstPack = NULL;
            }
        }
    }
    /******************************************
     step 4:  stop recv picture
    ******************************************/
    s32Ret = SC_MPI_VENC_StopRecvFrame(VencChn);
    if (s32Ret)
    {
        printf("SC_MPI_VENC_StopRecvPic failed with %#x!\n",  s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

SC_S32 visdk_startvenc(char *pdata, int *psize)
{
    SC_S32             i, s32Ret;
    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig     = {0};

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VENC_CHN           VencChn = 0;
    VIDEO_FRAME_INFO_S viframe = {0};

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enSnsSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 5;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT_VI_STOP;
    }

#if VISDK_ENCODEJPG

    /*config venc*/
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, PT_JPEG, enSnsSize,
            SAMPLE_RC_CBR, 0, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VENC(ViPipe, ViChn, VencChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Get GopAttr failed for %#x!\n", s32Ret);
        goto EXIT_VENC_STOP;
    }

    printf("vi sdk get jpg start!\n");

    s32Ret = visdk_getjpg(VencChn, 1, pdata, psize);
    if (s32Ret)
    {
        SAMPLE_PRT("visdk_getjpg failed.s32Ret:0x%x !\n", s32Ret);
    }

    printf("vi sdk get jpg end!\n");


EXIT_VENC_STOP:
    SAMPLE_COMM_VENC_Stop(VencChn);
EXIT1:
    SAMPLE_COMM_VI_UnBind_VENC(ViPipe, ViChn, VencChn);

#else

    usleep(400000);
    s32Ret = SC_MPI_VI_GetChnFrame(ViPipe, ViChn, &viframe, 5000);
    if (s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_GetChnFrame failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT_VI_STOP;
    }

    printf("vi sdk get jpg start2!\n");


    s32Ret = vimat_getjpg(&viframe, pdata, psize);
    if (s32Ret)
    {
        SAMPLE_PRT("vimat_getjpg failed.s32Ret:0x%x !\n", s32Ret);
    }


    printf("vi sdk get jpg end2!\n");


    SC_MPI_VI_ReleaseChnFrame(ViPipe, ViChn, &viframe);

#endif


EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    SAMPLE_PRT("%s: Exit!\n", __func__);

    return s32Ret;
}
