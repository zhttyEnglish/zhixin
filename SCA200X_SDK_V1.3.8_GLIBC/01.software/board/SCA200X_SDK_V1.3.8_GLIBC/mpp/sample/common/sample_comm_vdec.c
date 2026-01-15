/* vi: set sw=4 ts=4 et: */
/**
 * @file     sample_comm_vdec.c
 * @brief    vdec模块的功能示例代码
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
 * @date    2021-07-15 创建文件
 */
/************************************************************
 *@note
    Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
                   ALL RIGHTS RESERVED
Permission is hereby granted to licensees of  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
to use or abstract this computer program for the sole purpose of implementing a product based on  BEIJIING
SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use, or disseminate this computer
program,whether in part or in whole, are granted.  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
makes no representation or warranties with respect to the performance of this computer program, and specifically
disclaims any responsibility for any damages, special or consequential, connected with the use of this program.
　　For details, see http://www.sgitg.sgcc.com.cn/
**********************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include "sample_comm.h"

VB_SOURCE_E  g_enVdecVBSource  = VB_SOURCE_MODULE;

VB_POOL g_ahPicVbPool[VB_MAX_POOLS] = {[0 ... (VB_MAX_POOLS - 1)] = VB_INVALID_POOLID};
VB_POOL g_ahTmvVbPool[VB_MAX_POOLS] = {[0 ... (VB_MAX_POOLS - 1)] = VB_INVALID_POOLID};

//#define __VDEC_INFO
#ifdef __VDEC_INFO
# define pr_debug(fmt, args...) do { \
    printf("\033[0;33m[VDEC]:\033[0m"fmt, ##args); \
} while (0)
#else
# define pr_debug(fmt, args...) do {} while (0)
#endif

#define PRINTF_VDEC_CHN_STATUS(Chn, stStatus) \
do{\
    printf("\033[0;33m ---------------------------------------------------------------------------------------------------\033[0;39m\n");\
    printf("\033[0;33m chn:%d, Type:%d, bStart:%d, DecodeFrames:%d, LeftPics:%d, LeftBytes:%d, LeftFrames:%d, RecvFrames:%d  \033[0;39m\n",\
        Chn,\
        stStatus.enType,\
        stStatus.bStartRecvStream,\
        stStatus.u32DecodeStreamFrames,\
        stStatus.u32LeftPics,\
        stStatus.u32LeftStreamBytes,\
        stStatus.u32LeftStreamFrames,\
        stStatus.u32RecvStreamFrames);\
    printf("\033[0;33m FormatErr:%d,    s32PicSizeErrSet:%d,  s32StreamUnsprt:%d,  s32PackErr:%d,  u32PrtclNumErrSet:%d,  s32RefErrSet:%d,  s32PicBufSizeErrSet:%d  \033[0;39m\n",\
        stStatus.stVdecDecErr.s32FormatErr,\
        stStatus.stVdecDecErr.s32PicSizeErrSet,\
        stStatus.stVdecDecErr.s32StreamUnsprt,\
        stStatus.stVdecDecErr.s32PackErr,\
        stStatus.stVdecDecErr.s32PrtclNumErrSet,\
        stStatus.stVdecDecErr.s32RefErrSet,\
        stStatus.stVdecDecErr.s32PicBufSizeErrSet);\
    printf("\033[0;33m ---------------------------------------------------------------------------------------------------\033[0;39m\n");\
}while(0)

#define SAVE_FILE_NAME(aFileName, chn, s32Cnt, cStreamName, enPixelFormat)\
do{\
    SC_CHAR *Postfix;\
    if(enPixelFormat == PIXEL_FORMAT_RGB_565)\
    {\
        Postfix = "rgb565";\
    }\
    else if(enPixelFormat == PIXEL_FORMAT_BGR_565)\
    {\
        Postfix = "bgr565";\
    }\
    else if(enPixelFormat == PIXEL_FORMAT_RGB_888)\
    {\
        Postfix = "rgb888";\
    }\
    else if(enPixelFormat == PIXEL_FORMAT_BGR_888)\
    {\
        Postfix = "bgr888";\
    }\
    else if(enPixelFormat == PIXEL_FORMAT_ARGB_8888)\
    {\
        Postfix = "argb8888";\
    }\
    else if(enPixelFormat == PIXEL_FORMAT_ABGR_8888)\
    {\
        Postfix = "abgr8888";\
    }\
    else if(enPixelFormat == PIXEL_FORMAT_ARGB_1555)\
    {\
        Postfix = "argb1555";\
    }\
    else if(enPixelFormat == PIXEL_FORMAT_ABGR_1555)\
    {\
        Postfix = "abgr1555";\
    }\
    else if((enPixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_420)||(enPixelFormat == PIXEL_FORMAT_YUV_400))\
    {\
        Postfix = "yuv";\
    }\
    else\
    {\
        printf("[%s]-%d: enPixelFormat type err", __FUNCTION__, __LINE__);\
        return (SC_VOID *)(SC_FAILURE);\
    }\
    snprintf(aFileName, sizeof(aFileName), "chn%d_%d_%s.%s", chn, s32Cnt, cStreamName, Postfix);\
}while(0)

SC_S32 SAMPLE_COMM_VDEC_InitVBPool(SC_U32 ChnNum, SAMPLE_VDEC_ATTR *pastSampleVdec)
{
    VB_CONFIG_S stVbConf;
    SC_S32 i, j, pos = 0, s32Ret;
    SC_BOOL bFindFlag;
    SAMPLE_VDEC_BUF astSampleVdecBuf[VDEC_MAX_CHN_NUM];
    VB_POOL_CONFIG_S stVbPoolCfg;

    memset(astSampleVdecBuf, 0, sizeof(SAMPLE_VDEC_BUF)*VDEC_MAX_CHN_NUM);
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

    for(i = 0; i < ChnNum; i++)
    {
        if(PT_H265 == pastSampleVdec[i].enType)
        {
            astSampleVdecBuf[i].u32PicBufSize = VDEC_GetPicBufferSize(pastSampleVdec[i].enType, pastSampleVdec[i].u32Width,
                    pastSampleVdec[i].u32Height,
                    PIXEL_FORMAT_YVU_SEMIPLANAR_420, pastSampleVdec[i].stSapmleVdecVideo.enBitWidth, 0);
            astSampleVdecBuf[i].u32TmvBufSize = VDEC_GetTmvBufferSize(pastSampleVdec[i].enType, pastSampleVdec[i].u32Width,
                    pastSampleVdec[i].u32Height);
        }
        else if(PT_H264 == pastSampleVdec[i].enType)
        {
            astSampleVdecBuf[i].u32PicBufSize = VDEC_GetPicBufferSize(pastSampleVdec[i].enType, pastSampleVdec[i].u32Width,
                    pastSampleVdec[i].u32Height,
                    PIXEL_FORMAT_YVU_SEMIPLANAR_420, pastSampleVdec[i].stSapmleVdecVideo.enBitWidth, 0);
            if(VIDEO_DEC_MODE_IPB == pastSampleVdec[i].stSapmleVdecVideo.enDecMode)
            {
                astSampleVdecBuf[i].u32TmvBufSize = VDEC_GetTmvBufferSize(pastSampleVdec[i].enType, pastSampleVdec[i].u32Width,
                        pastSampleVdec[i].u32Height);
            }
        }
        else
        {
            astSampleVdecBuf[i].u32PicBufSize = VDEC_GetPicBufferSize(pastSampleVdec[i].enType, pastSampleVdec[i].u32Width,
                    pastSampleVdec[i].u32Height,
                    pastSampleVdec[i].stSapmleVdecPicture.enPixelFormat, DATA_BITWIDTH_8, 0);
        }
    }

    /* PicBuffer */
    for(j = 0; j < VB_MAX_COMM_POOLS; j++)
    {
        bFindFlag = SC_FALSE;
        for(i = 0; i < ChnNum; i++)
        {
            if((SC_FALSE == bFindFlag) && (0 != astSampleVdecBuf[i].u32PicBufSize)
                && (SC_FALSE == astSampleVdecBuf[i].bPicBufAlloc) )
            {
                stVbConf.astCommPool[j].u64BlkSize = astSampleVdecBuf[i].u32PicBufSize;
                stVbConf.astCommPool[j].u32BlkCnt  = pastSampleVdec[i].u32FrameBufCnt;
                astSampleVdecBuf[i].bPicBufAlloc   = SC_TRUE;
                bFindFlag                          = SC_TRUE;
                pos = j;
            }

            if((SC_TRUE == bFindFlag) && (SC_FALSE == astSampleVdecBuf[i].bPicBufAlloc)
                && (stVbConf.astCommPool[j].u64BlkSize == astSampleVdecBuf[i].u32PicBufSize) )
            {
                stVbConf.astCommPool[j].u32BlkCnt += pastSampleVdec[i].u32FrameBufCnt;
                astSampleVdecBuf[i].bPicBufAlloc   = SC_TRUE;
            }
        }
    }

    /* TmvBuffer */
    for(j = pos + 1; j < VB_MAX_COMM_POOLS; j++)
    {
        bFindFlag = SC_FALSE;
        for(i = 0; i < ChnNum; i++)
        {
            if((SC_FALSE == bFindFlag) && (0 != astSampleVdecBuf[i].u32TmvBufSize)
                && (SC_FALSE == astSampleVdecBuf[i].bTmvBufAlloc) )
            {
                stVbConf.astCommPool[j].u64BlkSize = astSampleVdecBuf[i].u32TmvBufSize;
                stVbConf.astCommPool[j].u32BlkCnt  = pastSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum + 1;
                astSampleVdecBuf[i].bTmvBufAlloc   = SC_TRUE;
                bFindFlag                          = SC_TRUE;
                pos = j;
            }

            if((SC_TRUE == bFindFlag) && (SC_FALSE == astSampleVdecBuf[i].bTmvBufAlloc)
                && (stVbConf.astCommPool[j].u64BlkSize == astSampleVdecBuf[i].u32TmvBufSize) )
            {
                stVbConf.astCommPool[j].u32BlkCnt += pastSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum + 1;
                astSampleVdecBuf[i].bTmvBufAlloc   = SC_TRUE;
            }
        }
    }
    stVbConf.u32MaxPoolCnt = pos + 1;

    if(VB_SOURCE_MODULE == g_enVdecVBSource)
    {
        SC_MPI_VB_ExitModCommPool(VB_UID_VDEC);
        CHECK_RET(SC_MPI_VB_SetModPoolConfig(VB_UID_VDEC, &stVbConf), "SC_MPI_VB_SetModPoolConfigig");
        s32Ret = SC_MPI_VB_InitModCommPool(VB_UID_VDEC);
        if (SC_SUCCESS != s32Ret)
        {
            printf("SC_MPI_VB_InitModCommPool fail for 0x%x\n", s32Ret);
            SC_MPI_VB_ExitModCommPool(VB_UID_VDEC);
            return SC_FAILURE;
        }
    }
    else if (VB_SOURCE_USER == g_enVdecVBSource)
    {
        for (i = 0; i < ChnNum; i++)
        {
            if ( (0 != astSampleVdecBuf[i].u32PicBufSize) && (0 != pastSampleVdec[i].u32FrameBufCnt))
            {
                memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
                stVbPoolCfg.u64BlkSize  = astSampleVdecBuf[i].u32PicBufSize;
                stVbPoolCfg.u32BlkCnt   = pastSampleVdec[i].u32FrameBufCnt;
                stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
                g_ahPicVbPool[i] = SC_MPI_VB_CreatePool(&stVbPoolCfg);
                if (VB_INVALID_POOLID == g_ahPicVbPool[i])
                {
                    goto fail;
                }
            }
            if (0 != astSampleVdecBuf[i].u32TmvBufSize)
            {
                memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
                stVbPoolCfg.u64BlkSize  = astSampleVdecBuf[i].u32TmvBufSize;
                stVbPoolCfg.u32BlkCnt   = pastSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum + 1;
                stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
                g_ahTmvVbPool[i] = SC_MPI_VB_CreatePool(&stVbPoolCfg);
                if (VB_INVALID_POOLID == g_ahTmvVbPool[i])
                {
                    goto fail;
                }
            }
        }
    }

    return SC_SUCCESS;

fail:
    for (; i >= 0; i--)
    {
        if (VB_INVALID_POOLID != g_ahPicVbPool[i])
        {
            s32Ret = SC_MPI_VB_DestroyPool(g_ahPicVbPool[i]);
            if(SC_SUCCESS != s32Ret)
            {
                printf("SC_MPI_VB_DestroyPool %d fail!\n", g_ahPicVbPool[i]);
            }
            g_ahPicVbPool[i] = VB_INVALID_POOLID;
        }
        if (VB_INVALID_POOLID != g_ahTmvVbPool[i])
        {
            s32Ret = SC_MPI_VB_DestroyPool(g_ahTmvVbPool[i]);
            if(SC_SUCCESS != s32Ret)
            {
                printf("SC_MPI_VB_DestroyPool %d fail!\n", g_ahTmvVbPool[i]);
            }
            g_ahTmvVbPool[i] = VB_INVALID_POOLID;
        }
    }
    return SC_FAILURE;
}

SC_VOID SAMPLE_COMM_VDEC_ExitVBPool(SC_VOID)
{
    SC_S32 i, s32Ret;

    if(VB_SOURCE_MODULE == g_enVdecVBSource)
    {
        SC_MPI_VB_ExitModCommPool(VB_UID_VDEC);
    }
    else if (VB_SOURCE_USER == g_enVdecVBSource)
    {
        for (i = VB_MAX_POOLS - 1; i >= 0; i--)
        {
            if (VB_INVALID_POOLID != g_ahPicVbPool[i])
            {
                s32Ret = SC_MPI_VB_DestroyPool(g_ahPicVbPool[i]);
                if(SC_SUCCESS != s32Ret)
                {
                    printf("SC_MPI_VB_DestroyPool %d fail!\n", g_ahPicVbPool[i]);
                }
                g_ahPicVbPool[i] = VB_INVALID_POOLID;
            }
            if (VB_INVALID_POOLID != g_ahTmvVbPool[i])
            {
                s32Ret = SC_MPI_VB_DestroyPool(g_ahTmvVbPool[i]);
                if(SC_SUCCESS != s32Ret)
                {
                    printf("SC_MPI_VB_DestroyPool %d fail!\n", g_ahTmvVbPool[i]);
                }
                g_ahTmvVbPool[i] = VB_INVALID_POOLID;
            }
        }
    }

    return;
}

SC_VOID *SAMPLE_COMM_VDEC_SendStream(SC_VOID *pArgs)
{
    VDEC_THREAD_PARAM_S *pstVdecThreadParam = (VDEC_THREAD_PARAM_S *)pArgs;
    SC_BOOL bEndOfStream = SC_FALSE;
    SC_S32 s32UsedBytes = 0, s32ReadLen = 0;
    FILE *fpStrm = NULL;
    SC_U8 *pu8Buf = NULL;
    VDEC_STREAM_S stStream;
    SC_BOOL bFindStart, bFindEnd;
    SC_U64 u64PTS = 0;
    SC_U32 u32Len, u32Start;
    SC_S32 s32Ret,  i;
    SC_CHAR cStreamFile[256];

    prctl(PR_SET_NAME, "VideoSendStream", 0, 0, 0);

    snprintf(cStreamFile, sizeof(cStreamFile), "%s/%s", pstVdecThreadParam->cFilePath, pstVdecThreadParam->cFileName);
    if(cStreamFile[0] != 0)
    {
        fpStrm = fopen(cStreamFile, "rb");
        if(fpStrm == NULL)
        {
            SAMPLE_PRT("chn %d can't open file %s in send stream thread!\n", pstVdecThreadParam->s32ChnId, cStreamFile);
            return (SC_VOID *)(SC_FAILURE);
        }
    }
    printf("\n \033[0;36m chn %d, stream file:%s, userbufsize: %d \033[0;39m\n", pstVdecThreadParam->s32ChnId,
        pstVdecThreadParam->cFileName, pstVdecThreadParam->s32MinBufSize);

    pu8Buf = malloc(pstVdecThreadParam->s32MinBufSize);
    if(pu8Buf == NULL)
    {
        SAMPLE_PRT("chn %d can't alloc %d in send stream thread!\n", pstVdecThreadParam->s32ChnId,
            pstVdecThreadParam->s32MinBufSize);
        fclose(fpStrm);
        return (SC_VOID *)(SC_FAILURE);
    }
    fflush(stdout);

    u64PTS = pstVdecThreadParam->u64PtsInit;
    while (1)
    {
        if (pstVdecThreadParam->eThreadCtrl == THREAD_CTRL_STOP)
        {
            break;
        }
        else if (pstVdecThreadParam->eThreadCtrl == THREAD_CTRL_PAUSE)
        {
            sleep(1);
            continue;
        }

        bEndOfStream = SC_FALSE;
        bFindStart   = SC_FALSE;
        bFindEnd     = SC_FALSE;
        u32Start     = 0;
        fseek(fpStrm, s32UsedBytes, SEEK_SET);
        s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
        if (s32ReadLen <= 0)
        {
            if (pstVdecThreadParam->bCircleSend == SC_TRUE)
            {
                memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
                stStream.bEndOfStream = SC_TRUE;
                SC_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);

                s32UsedBytes = 0;
                fseek(fpStrm, 0, SEEK_SET);
                s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
            }
            else
            {
                break;
            }
        }

        if (pstVdecThreadParam->s32StreamMode == VIDEO_MODE_FRAME && pstVdecThreadParam->enType == PT_H264)
        {
            for (i = 0; i < s32ReadLen - 8; i++)
            {
                int tmp = pu8Buf[i + 3] & 0x1F;
                if (  pu8Buf[i    ] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
                    (
                        ((tmp == 0x5 || tmp == 0x1) && ((pu8Buf[i + 4] & 0x80) == 0x80)) ||
                        (tmp == 20 && (pu8Buf[i + 7] & 0x80) == 0x80)
                    )
                )
                {
                    bFindStart = SC_TRUE;
                    i += 8;
                    break;
                }
            }

            for (; i < s32ReadLen - 8; i++)
            {
                int tmp = pu8Buf[i + 3] & 0x1F;
                if (  pu8Buf[i    ] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
                    (
                        tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 ||
                        ((tmp == 5 || tmp == 1) && ((pu8Buf[i + 4] & 0x80) == 0x80)) ||
                        (tmp == 20 && (pu8Buf[i + 7] & 0x80) == 0x80)
                    )
                )
                {
                    bFindEnd = SC_TRUE;
                    break;
                }
            }

            if(i > 0)s32ReadLen = i;
            if (bFindStart == SC_FALSE)
            {
                SAMPLE_PRT("chn %d can not find H264 start code!s32ReadLen %d, s32UsedBytes %d.!\n",
                    pstVdecThreadParam->s32ChnId, s32ReadLen, s32UsedBytes);
            }
            if (bFindEnd == SC_FALSE)
            {
                s32ReadLen = i + 8;
            }

        }
        else if (pstVdecThreadParam->s32StreamMode == VIDEO_MODE_FRAME
            && pstVdecThreadParam->enType == PT_H265)
        {
            SC_BOOL  bNewPic = SC_FALSE;
            for (i = 0; i < s32ReadLen - 6; i++)
            {
                SC_U32 tmp = (pu8Buf[i + 3] & 0x7E) >> 1;
                bNewPic = ( pu8Buf[i + 0] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1
                        && (tmp >= 0 && tmp <= 21) && ((pu8Buf[i + 5] & 0x80) == 0x80) );

                if (bNewPic)
                {
                    bFindStart = SC_TRUE;
                    i += 6;
                    break;
                }
            }

            for (; i < s32ReadLen - 6; i++)
            {
                SC_U32 tmp = (pu8Buf[i + 3] & 0x7E) >> 1;
                bNewPic = (pu8Buf[i + 0] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1
                        && ( tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39 || tmp == 40 || ((tmp >= 0 && tmp <= 21)
                                && (pu8Buf[i + 5] & 0x80) == 0x80) )
                    );

                if (bNewPic)
                {
                    bFindEnd = SC_TRUE;
                    break;
                }
            }
            if(i > 0)s32ReadLen = i;

            if (bFindStart == SC_FALSE)
            {
                SAMPLE_PRT("chn %d can not find H265 start code!s32ReadLen %d, s32UsedBytes %d.!\n",
                    pstVdecThreadParam->s32ChnId, s32ReadLen, s32UsedBytes);
            }
            if (bFindEnd == SC_FALSE)
            {
                s32ReadLen = i + 6;
            }

        }
        else if (pstVdecThreadParam->enType == PT_MJPEG || pstVdecThreadParam->enType == PT_JPEG)
        {
            for (i = 0; i < s32ReadLen - 1; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i + 1] == 0xD8)
                {
                    u32Start = i;
                    bFindStart = SC_TRUE;
                    i = i + 2;
                    break;
                }
            }

            for (; i < s32ReadLen - 3; i++)
            {
                if ((pu8Buf[i] == 0xFF) && (pu8Buf[i + 1] & 0xF0) == 0xE0)
                {
                    u32Len = (pu8Buf[i + 2] << 8) + pu8Buf[i + 3];
                    i += 1 + u32Len;
                }
                else
                {
                    break;
                }
            }

            for (; i < s32ReadLen - 1; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i + 1] == 0xD9)
                {
                    bFindEnd = SC_TRUE;
                    break;
                }
            }
            s32ReadLen = i + 2;

            if (bFindStart == SC_FALSE)
            {
                SAMPLE_PRT("chn %d can not find JPEG start code!s32ReadLen %d, s32UsedBytes %d.!\n",
                    pstVdecThreadParam->s32ChnId, s32ReadLen, s32UsedBytes);
            }
        }
        else
        {
            if((s32ReadLen != 0) && (s32ReadLen < pstVdecThreadParam->s32MinBufSize))
            {
                bEndOfStream = SC_TRUE;
            }
        }

        stStream.u64PTS       = u64PTS;
        stStream.pu8Addr      = pu8Buf + u32Start;
        stStream.u32Len       = s32ReadLen;
        stStream.bEndOfFrame  = (pstVdecThreadParam->s32StreamMode == VIDEO_MODE_FRAME) ? SC_TRUE : SC_FALSE;
        stStream.bEndOfStream = bEndOfStream;
        stStream.bDisplay     = 1;

SendAgain:
        s32Ret = SC_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, pstVdecThreadParam->s32MilliSec);
        if( (SC_SUCCESS != s32Ret) && (THREAD_CTRL_START == pstVdecThreadParam->eThreadCtrl) )
        {
            usleep(pstVdecThreadParam->s32IntervalTime);
            goto SendAgain;
        }
        else
        {
            bEndOfStream = SC_FALSE;
            s32UsedBytes = s32UsedBytes + s32ReadLen + u32Start;
            u64PTS += pstVdecThreadParam->u64PtsIncrease;
            static int seq = 1;
            printf("\033[0;35m the %d frame has been sent ...  \033[0;39m\n", seq++);

        }
        usleep(pstVdecThreadParam->s32IntervalTime);
    }

    /* send the flag of stream end */
    memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
    stStream.bEndOfStream = SC_TRUE;
    SC_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);

    printf("\033[0;35m chn %d send steam thread return ...  \033[0;39m\n", pstVdecThreadParam->s32ChnId);
    fflush(stdout);
    if (pu8Buf != SC_NULL)
    {
        free(pu8Buf);
    }
    fclose(fpStrm);

    return (SC_VOID *)SC_SUCCESS;
}

static void SAMPLE_COMM_VDEC_SaveYUVFile_Linear8Bit(FILE *pfd, VIDEO_FRAME_S *pVBuf)
{
    SC_U8 *pY_map = NULL;
    SC_U8 *pC_map = NULL;
    unsigned int w, h;
    SC_U8 *pMemContent;
    SC_U8 *pTmpBuff = SC_NULL;
    SC_U64 phy_addr;
    SC_U32 u32Size, s32Ysize;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    SC_U32 u32UvHeight;

    if (PIXEL_FORMAT_YVU_SEMIPLANAR_420 == enPixelFormat)
    {
        s32Ysize = (pVBuf->u64PhyAddr[1] - pVBuf->u64PhyAddr[0]);
        u32Size = s32Ysize * 3 / 2;
        u32UvHeight = pVBuf->u32Height / 2;
    }
    else if(PIXEL_FORMAT_YVU_SEMIPLANAR_422 == enPixelFormat)
    {
        s32Ysize = (pVBuf->u64PhyAddr[1] - pVBuf->u64PhyAddr[0]);
        u32Size = s32Ysize * 2;
        u32UvHeight = pVBuf->u32Height;
    }
    else if(PIXEL_FORMAT_YUV_400 == enPixelFormat)
    {
        u32Size = s32Ysize = (pVBuf->u32Stride[0]) * ALIGN_UP(pVBuf->u32Height, 16);
        u32UvHeight = 0;
    }
    else
    {
        printf("%s %d: This YUV format is not support!\n", __func__, __LINE__);
        return;
    }

    phy_addr = pVBuf->u64PhyAddr[0];

    pY_map = (SC_U8 *) SC_MPI_SYS_Mmap(phy_addr, u32Size);
    if (SC_NULL == pY_map)
    {
        SAMPLE_PRT("SC_MPI_SYS_Mmap for pY_map fail!!\n");
        return;
    }

    pC_map = pY_map + s32Ysize;

    fprintf(stderr, "saving......Y......");
    fflush(stderr);
    for (h = 0; h < pVBuf->u32Height; h++)
    {
        pMemContent = pY_map + h * pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }

    if(PIXEL_FORMAT_YUV_400 != enPixelFormat)
    {
        fflush(pfd);
        fprintf(stderr, "U......");
        fflush(stderr);

        pTmpBuff = (SC_U8 *)malloc(pVBuf->u32Stride[0]);
        if(SC_NULL == pTmpBuff)
        {
            SAMPLE_PRT("malloc pTmpBuff (size=%d) fail!!!\n", pVBuf->u32Stride[0]);
            return;
        }
        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pC_map + h * pVBuf->u32Stride[1];

            pMemContent += 1;

            for (w = 0; w < pVBuf->u32Width / 2; w++)
            {
                pTmpBuff[w] = *pMemContent;
                pMemContent += 2;
            }
            fwrite(pTmpBuff, pVBuf->u32Width / 2, 1, pfd);
        }
        fflush(pfd);

        fprintf(stderr, "V......");
        fflush(stderr);
        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pC_map + h * pVBuf->u32Stride[1];

            for (w = 0; w < pVBuf->u32Width / 2; w++)
            {
                pTmpBuff[w] = *pMemContent;
                pMemContent += 2;
            }
            fwrite(pTmpBuff, pVBuf->u32Width / 2, 1, pfd);
        }
        free(pTmpBuff);
        pTmpBuff = SC_NULL;
    }
    fflush(pfd);

    fprintf(stderr, "done!\n");
    fflush(stderr);

    SC_MPI_SYS_Munmap(pY_map, u32Size);
    pY_map = SC_NULL;

    return;
}

void SAMPLE_COMM_VDEC_SaveRGBFile(FILE *fRgb, VIDEO_FRAME_S *pVB)
{
    SC_U32 i;
    SC_U32 u32Size;
    SC_U8 *pUserPageAddr, *pTmp;
    SC_U32 u32WidthInBytes;
    SC_U32 u32Stride;

    if( (PIXEL_FORMAT_ARGB_8888 == pVB->enPixelFormat) || (PIXEL_FORMAT_ABGR_8888 == pVB->enPixelFormat))
    {
        u32WidthInBytes = pVB->u32Width * 4;
    }
    else if( (PIXEL_FORMAT_ARGB_1555 == pVB->enPixelFormat) || (PIXEL_FORMAT_ABGR_1555 == pVB->enPixelFormat))
    {
        u32WidthInBytes = pVB->u32Width * 2;
    }
    else if( (PIXEL_FORMAT_RGB_888 == pVB->enPixelFormat) || (PIXEL_FORMAT_BGR_888 == pVB->enPixelFormat))
    {
        u32WidthInBytes = pVB->u32Width * 3;
    }
    else if( (PIXEL_FORMAT_RGB_565 == pVB->enPixelFormat) || (PIXEL_FORMAT_BGR_565 == pVB->enPixelFormat))
    {
        u32WidthInBytes = pVB->u32Width * 2;
    }
    else
    {
        printf("%s %d: This RGB format is not support!\n", __func__, __LINE__);
        return;
    }

    u32Stride = ALIGN_UP(u32WidthInBytes, 16);
    u32Size   = u32Stride * ALIGN_UP(pVB->u32Height, 16);

    pUserPageAddr = (SC_U8 *) SC_MPI_SYS_Mmap(pVB->u64PhyAddr[0], u32Size);

    if (SC_NULL == pUserPageAddr)
    {
        printf("%s %d:SC_MPI_SYS_Mmap fail!!! u32Size=%d\n", __func__, __LINE__, u32Size);
        return;
    }

    fprintf(stderr, "saving......RGB..%d x %d......", pVB->u32Width, pVB->u32Height);
    fflush(stderr);

    pTmp = pUserPageAddr;
    for (i = 0; i < pVB->u32Height; i++, pTmp += u32Stride)
    {
        fwrite(pTmp, u32WidthInBytes, 1, fRgb) ;
    }
    fflush(fRgb);

    fprintf(stderr, "done!\n");
    fflush(stderr);

    SC_MPI_SYS_Munmap(pUserPageAddr, u32Size);
    pUserPageAddr = SC_NULL;

    return;
}

SC_VOID SAMPLE_COMM_VDEC_CmdCtrl(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread)
{
    SC_S32 i, s32Ret;
    VDEC_CHN_STATUS_S stStatus;
    SC_BOOL bVoPause = SC_FALSE;
    char c = 0;
    VO_LAYER VoLayer;
    VO_CHN     VoChn;
    VO_VIDEO_LAYER_ATTR_S stVoAttr;
    SC_S32 s32FrameRate = 0;

    for(i = 0; i < s32ChnNum; i++)
    {
        if(SC_TRUE == pstVdecSend[i].bCircleSend)
        {
            goto WHILE;
        }
    }

    printf("decoding..............");
    for(i = 0; i < s32ChnNum; i++)
    {
        if(0 != pVdecThread[i])
        {
            s32Ret = pthread_join(pVdecThread[i], SC_NULL);
            if(0 == s32Ret)
            {
                pVdecThread[i] = 0;
            }
        }
        pVdecThread[i] = 0;
        while(1)
        {
            s32Ret = SC_MPI_VDEC_QueryStatus(pstVdecSend[i].s32ChnId, &stStatus);
            if(s32Ret != SC_SUCCESS)
            {
                printf("chn %d SC_MPI_VDEC_QueryStatus fail!!!\n", s32Ret);
                return;
            }
            if((0 == stStatus.u32LeftStreamBytes) && (0 == stStatus.u32LeftPics))
            {
                PRINTF_VDEC_CHN_STATUS(pstVdecSend[i].s32ChnId, stStatus);
                break;
            }
            usleep(1000);
        }
    }
    printf("end!\n");
    return;

WHILE:
    while(1)
    {
        printf("\nSAMPLE_TEST:press 'e' to exit; 'p' to pause; 'r' to resume; 'q' to query; 's' to step; 'a' to add; 'd' to sub!;\n");
        c = getchar();

        if (c == 'e')
            break;
        else if (c == 'r')
        {
            if (bVoPause == SC_TRUE)
            {
                for (i = 0; i < s32ChnNum; i++)
                {
                    pstVdecSend[i].eThreadCtrl = THREAD_CTRL_START;
                }

                for (VoLayer = 0; VoLayer < (VO_MAX_LAYER_NUM - VO_MAX_VIRT_DEV_NUM); VoLayer++)
                {
                    for (VoChn = 0; VoChn < s32ChnNum; VoChn++)
                    {
                        s32Ret = SC_MPI_VO_ResumeChn(VoLayer, VoChn);
                        if(SC_SUCCESS != s32Ret)
                        {
                            printf("SC_MPI_VO_ResumeChn(%d, %d) fail for 0x%x!\n", VoLayer, VoChn, s32Ret);
                        }
                    }
                }
                printf("VO Resume!!!");
            }
            bVoPause = SC_FALSE;
        }
        else if (c == 'p')
        {
            if(bVoPause == SC_FALSE)
            {
                for (i = 0; i < s32ChnNum; i++)
                {
                    pstVdecSend[i].eThreadCtrl = THREAD_CTRL_PAUSE;
                }

                for (VoLayer = 0; VoLayer < (VO_MAX_LAYER_NUM - VO_MAX_VIRT_DEV_NUM); VoLayer++)
                {
                    for (VoChn = 0; VoChn < s32ChnNum; VoChn++)
                    {
                        s32Ret = SC_MPI_VO_PauseChn(VoLayer, VoChn);
                        if(SC_SUCCESS != s32Ret)
                        {
                            printf("SC_MPI_VO_PauseChn(%d, %d) fail for 0x%x!\n", VoLayer, VoChn, s32Ret);
                        }
                    }
                }
                printf("VO Pause...");
            }
            bVoPause = SC_TRUE;
        }

        else if (c == 'a')
        {
            for(i = 0; i < VO_MAX_LAYER_NUM; i++)
            {
                SC_MPI_VO_GetVideoLayerAttr(i, &stVoAttr);
                if (stVoAttr.u32DispFrmRt == 0)
                {
                    continue;
                }
                SC_MPI_VO_GetChnFrameRate(i, 0, &s32FrameRate);
                if (s32FrameRate >= 120)
                {
                    printf("VO FrameRate of chnl 0 of layer %d is larger than 120.", i);
                    continue;
                }
                s32FrameRate += 10;
                SC_MPI_VO_SetChnFrameRate(i, 0, s32FrameRate);
                printf("VO FrameRate of chnl 0 of layer %d is set to %d.", i, s32FrameRate);
            }
        }
        else if (c == 'd')
        {
            for(i = 0; i < 3; i++)
            {
                SC_MPI_VO_GetVideoLayerAttr(i, &stVoAttr);
                if (stVoAttr.u32DispFrmRt == 0)
                {
                    continue;
                }
                SC_MPI_VO_GetChnFrameRate(i, 0, &s32FrameRate);
                if (s32FrameRate < 20)
                {
                    printf("VO FrameRate of chnl 0 of layer %d is less than 10.", i);
                    continue;
                }
                s32FrameRate -= 10;
                SC_MPI_VO_SetChnFrameRate(i, 0, s32FrameRate);
                printf("VO FrameRate of chnl 0 of layer %d is set to %d.", i, s32FrameRate);
            }
        }
        else if (c == 's')
        {
            if (bVoPause == SC_FALSE)
            {
                printf("Firstly press 'p' to pause,then step.");
                continue;
            }

            for (VoLayer = 0; VoLayer < VO_MAX_LAYER_NUM; VoLayer++)
            {
                for (VoChn = 0; VoChn < s32ChnNum; VoChn++)
                {
                    s32Ret = SC_MPI_VO_StepChn(VoLayer, VoChn);
                    if(SC_SUCCESS != s32Ret)
                    {
                        printf("SC_MPI_VO_StepChn(%d, %d) fail for 0x%x!\n", VoLayer, VoChn, s32Ret);
                    }
                }
            }
            printf("VO Step.");
        }
        else if (c == 'q')
        {
            for (i = 0; i < s32ChnNum; i++)
            {
                SC_MPI_VDEC_QueryStatus(pstVdecSend[i].s32ChnId, &stStatus);
                PRINTF_VDEC_CHN_STATUS(pstVdecSend[i].s32ChnId, stStatus);
            }
        }
    }
    return;
}

SC_VOID SAMPLE_COMM_VDEC_StartSendStream(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread)
{
    SC_S32  i;

    for(i = 0; i < s32ChnNum; i++)
    {
        pVdecThread[i] = 0;
        pthread_create(&pVdecThread[i], 0, SAMPLE_COMM_VDEC_SendStream, (SC_VOID *)&pstVdecSend[i]);
    }
}

SC_VOID SAMPLE_COMM_VDEC_StopSendStream(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread)
{
    SC_S32  i;

    for(i = 0; i < s32ChnNum; i++)
    {
        pstVdecSend[i].eThreadCtrl = THREAD_CTRL_STOP;
        if(0 != pVdecThread[i])
        {
            pthread_join(pVdecThread[i], SC_NULL);
            pVdecThread[i] = 0;
        }
    }
}

/******************************************************************************
* function : get stream from file, and send it  to Adec
******************************************************************************/
static void *SAMPLE_COMM_VDEC_RPC_SendStream(void *pArgs)
{
    VDEC_THREAD_PARAM_S *pstVdecThreadParam = (VDEC_THREAD_PARAM_S *)pArgs;
    SC_BOOL bEndOfStream = SC_FALSE;
    SC_S32 s32UsedBytes = 0, s32ReadLen = 0;
    FILE *fpStrm = NULL;
    SC_U8 *pu8Buf = NULL;
    VDEC_STREAM_S stStream;
    SC_BOOL bFindStart, bFindEnd;
    SC_U64 u64PTS = 0;
    SC_U32 u32Len, u32Start;
    SC_S32 s32Ret,  i;
    SC_CHAR cStreamFile[256];
    int seq = 1;

    prctl(PR_SET_NAME, "VDECStream", 0, 0, 0);
    snprintf(cStreamFile, sizeof(cStreamFile), "%s/%s", pstVdecThreadParam->cFilePath, pstVdecThreadParam->cFileName);
    if(cStreamFile[0] != 0)
    {
        fpStrm = fopen(cStreamFile, "rb");
        if(fpStrm == NULL)
        {
            SAMPLE_PRT("chn %d can't open file %s in send stream thread!\n", pstVdecThreadParam->s32ChnId, cStreamFile);
            return (SC_VOID *)(SC_FAILURE);
        }
    }
    printf("\n \033[0;36m chn %d, stream file:%s, userbufsize: %d \033[0;39m\n", pstVdecThreadParam->s32ChnId,
        pstVdecThreadParam->cFileName, pstVdecThreadParam->s32MinBufSize);

    VB_POOL_CONFIG_S stVbPoolCfg;
    SC_U32 u32PoolId;
    VB_BLK VbBlk;
    u32Len = pstVdecThreadParam->s32MinBufSize;
#if 1
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Len;
    stVbPoolCfg.u32BlkCnt = 1;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    u32PoolId = SC_MPI_VB_CreatePool(&stVbPoolCfg);
    if (u32PoolId == VB_INVALID_POOLID)
    {
        SAMPLE_PRT("VB_CreatePool failed\n");
        fclose(fpStrm);
        return NULL;
    }
#endif

    //VbBlk = SC_MPI_VB_GetBlock(POOL_OWNER_COMMON, u32Len, SC_NULL);
    VbBlk = SC_MPI_VB_GetBlock(u32PoolId, u32Len, SC_NULL);
    if (VB_INVALID_HANDLE == VbBlk)
    {
        SAMPLE_PRT("SC_MPI_VB_GetBlock failed(len=%d)\n", u32Len);
        SC_MPI_VB_DestroyPool(u32PoolId);
        return NULL;
    }

    stStream.u64PhyAddr = SC_MPI_VB_Handle2PhysAddr(VbBlk);
    if (0ULL == stStream.u64PhyAddr)
    {
        SAMPLE_PRT("SC_MPI_VB_Handle2PhysAddr failed\n");
        goto out;
    }
    pu8Buf = (SC_U8 *)SC_MPI_SYS_Mmap(stStream.u64PhyAddr, u32Len);
    if (NULL == pu8Buf)
    {
        SAMPLE_PRT("chn %d can't mmap failed in send stream thread!\n",
                pstVdecThreadParam->s32ChnId);
        goto out;
    }
    fflush(stdout);
    u64PTS = pstVdecThreadParam->u64PtsInit;
    while (1)
    {
        if (pstVdecThreadParam->eThreadCtrl == THREAD_CTRL_STOP)
        {
            sleep(1);
            break;
        }
        else if (pstVdecThreadParam->eThreadCtrl == THREAD_CTRL_PAUSE)
        {
            sleep(1);
            continue;
        }

        bEndOfStream = SC_FALSE;
        bFindStart   = SC_FALSE;
        bFindEnd     = SC_FALSE;
        u32Start     = 0;
        fseek(fpStrm, s32UsedBytes, SEEK_SET);
        s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
        
        if (s32ReadLen <= 0)
        {
            if (pstVdecThreadParam->bCircleSend == SC_TRUE)
            {
                memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
                stStream.bEndOfStream = SC_TRUE;
                SC_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);

                s32UsedBytes = 0;
                fseek(fpStrm, 0, SEEK_SET);
                s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
            }
            else
            {
                break;
            }
        }

        if (pstVdecThreadParam->s32StreamMode == VIDEO_MODE_FRAME && pstVdecThreadParam->enType == PT_H264)
        {
            for (i = 0; i < s32ReadLen - 8; i++)
            {
                int tmp = pu8Buf[i + 3] & 0x1F;
                if (  pu8Buf[i    ] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
                    (
                        ((tmp == 0x5 || tmp == 0x1) && ((pu8Buf[i + 4] & 0x80) == 0x80)) ||
                        (tmp == 20 && (pu8Buf[i + 7] & 0x80) == 0x80)
                    )
                )
                {
                    bFindStart = SC_TRUE;
                    i += 8;
                    break;
                }
            }

            for (; i < s32ReadLen - 8; i++)
            {
                int tmp = pu8Buf[i + 3] & 0x1F;
                if (  pu8Buf[i    ] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
                    (
                        tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 ||
                        ((tmp == 5 || tmp == 1) && ((pu8Buf[i + 4] & 0x80) == 0x80)) ||
                        (tmp == 20 && (pu8Buf[i + 7] & 0x80) == 0x80)
                    )
                )
                {
                    bFindEnd = SC_TRUE;
                    break;
                }
            }

            if(i > 0)s32ReadLen = i;
            if (bFindStart == SC_FALSE)
            {
                SAMPLE_PRT("chn %d can not find H264 start code!s32ReadLen %d, s32UsedBytes %d.!\n",
                    pstVdecThreadParam->s32ChnId, s32ReadLen, s32UsedBytes);
            }
            if (bFindEnd == SC_FALSE)
            {
                s32ReadLen = i + 8;
            }
        }
        else if (pstVdecThreadParam->s32StreamMode == VIDEO_MODE_FRAME
            && pstVdecThreadParam->enType == PT_H265)
        {
            SC_BOOL  bNewPic = SC_FALSE;
            for (i = 0; i < s32ReadLen - 6; i++)
            {
                SC_U32 tmp = (pu8Buf[i + 3] & 0x7E) >> 1;
                bNewPic = ( pu8Buf[i + 0] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1
                        && (tmp >= 0 && tmp <= 21) && ((pu8Buf[i + 5] & 0x80) == 0x80) );

                if (bNewPic)
                {
                    bFindStart = SC_TRUE;
                    i += 6;
                    break;
                }
            }

            for (; i < s32ReadLen - 6; i++)
            {
                SC_U32 tmp = (pu8Buf[i + 3] & 0x7E) >> 1;
                bNewPic = (pu8Buf[i + 0] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1
                        && ( tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39 || tmp == 40 || ((tmp >= 0 && tmp <= 21)
                                && (pu8Buf[i + 5] & 0x80) == 0x80) )
                    );

                if (bNewPic)
                {
                    bFindEnd = SC_TRUE;
                    break;
                }
            }
            if(i > 0)s32ReadLen = i;

            if (bFindStart == SC_FALSE)
            {
                SAMPLE_PRT("chn %d can not find H265 start code!s32ReadLen %d, s32UsedBytes %d.!\n",
                    pstVdecThreadParam->s32ChnId, s32ReadLen, s32UsedBytes);
            }
            if (bFindEnd == SC_FALSE)
            {
                s32ReadLen = i + 6;
            }

        }
        else if (pstVdecThreadParam->enType == PT_MJPEG || pstVdecThreadParam->enType == PT_JPEG)
        {
            for (i = 0; i < s32ReadLen - 1; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i + 1] == 0xD8)
                {
                    u32Start = i;
                    bFindStart = SC_TRUE;
                    i = i + 2;
                    break;
                }
            }

            for (; i < s32ReadLen - 3; i++)
            {
                if ((pu8Buf[i] == 0xFF) && (pu8Buf[i + 1] & 0xF0) == 0xE0)
                {
                    u32Len = (pu8Buf[i + 2] << 8) + pu8Buf[i + 3];
                    i += 1 + u32Len;
                }
                else
                {
                    break;
                }
            }

            for (; i < s32ReadLen - 1; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i + 1] == 0xD9)
                {
                    bFindEnd = SC_TRUE;
                    break;
                }
            }
            s32ReadLen = i + 2;

            if (bFindStart == SC_FALSE)
            {
                SAMPLE_PRT("chn %d can not find JPEG start code!s32ReadLen %d, s32UsedBytes %d.!\n",
                    pstVdecThreadParam->s32ChnId, s32ReadLen, s32UsedBytes);
            }
        }
        else
        {
            if((s32ReadLen != 0) && (s32ReadLen < pstVdecThreadParam->s32MinBufSize))
            {
                bEndOfStream = SC_TRUE;
            }
        }

        stStream.u64PTS       = u64PTS;
        stStream.pu8Addr      = pu8Buf + u32Start;
        stStream.u32Len       = s32ReadLen;
        stStream.bEndOfFrame  = (pstVdecThreadParam->s32StreamMode == VIDEO_MODE_FRAME) ? SC_TRUE : SC_FALSE;
        stStream.bEndOfStream = bEndOfStream;
        stStream.bDisplay     = 1;

SendAgain:
        if (pstVdecThreadParam->s32StreamMode == VIDEO_MODE_FRAME)
        {
            if (!bFindStart || !bFindEnd)
            {
                if (pstVdecThreadParam->bCircleSend)
                {
                    s32UsedBytes = 0;
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        s32Ret = SC_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, pstVdecThreadParam->s32MilliSec);
        if( (SC_SUCCESS != s32Ret) && (THREAD_CTRL_START == pstVdecThreadParam->eThreadCtrl) )
        {
            usleep(pstVdecThreadParam->s32IntervalTime);
            goto SendAgain;
        }
        else
        {
            bEndOfStream = SC_FALSE;
            s32UsedBytes = s32UsedBytes + s32ReadLen + u32Start;
            u64PTS += pstVdecThreadParam->u64PtsIncrease;
            pr_debug("Chn%d: the %dth frame has been sent ...\n", pstVdecThreadParam->s32ChnId, seq++);
        }

        usleep(pstVdecThreadParam->s32IntervalTime);
    }

    /* send the flag of stream end */
    memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
    stStream.bEndOfStream = SC_TRUE;
    SC_MPI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);

    pr_debug("Chn%d: send steam thread return!\n", pstVdecThreadParam->s32ChnId);
    fflush(stdout);
out:
    if (pu8Buf != SC_NULL)
        SC_MPI_SYS_Munmap(pu8Buf, u32Len);
    SC_MPI_VB_ReleaseBlock(VbBlk);
    SC_MPI_VB_DestroyPool(u32PoolId);
    fclose(fpStrm);

    return NULL;
}

SC_VOID SAMPLE_COMM_VDEC_RPC_StartSendStream(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread)
{
    SC_S32  i;

    for(i = 0; i < s32ChnNum; i++)
    {
        pVdecThread[i] = 0;
        pstVdecSend[i].eThreadCtrl = THREAD_CTRL_START;
        pthread_create(&pVdecThread[i], 0, SAMPLE_COMM_VDEC_RPC_SendStream, (SC_VOID *)&pstVdecSend[i]);
    }
}

SC_VOID SAMPLE_COMM_VDEC_RPC_StopSendStream(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread)
{
    SC_S32  i;

    for(i = 0; i < s32ChnNum; i++)
    {
        pstVdecSend[i].eThreadCtrl = THREAD_CTRL_STOP;
        if(0 != pVdecThread[i])
        {
            pthread_join(pVdecThread[i], SC_NULL);
            pVdecThread[i] = 0;
        }
        SC_MPI_VDEC_StopRecvStream(i);
    }
}

SC_VOID *SAMPLE_COMM_VDEC_GetPic(SC_VOID *pArgs)
{
    VDEC_THREAD_PARAM_S *pstVdecThreadParam = (VDEC_THREAD_PARAM_S *)pArgs;
    FILE *fp = SC_NULL;
    SC_S32 s32Ret, s32Cnt = 0;
    VDEC_CHN_ATTR_S  stAttr;
    VIDEO_FRAME_INFO_S stVFrame;
    SC_CHAR cSaveFile[256];

    prctl(PR_SET_NAME, "VdecGetPic", 0, 0, 0);

    s32Ret = SC_MPI_VDEC_GetChnAttr(pstVdecThreadParam->s32ChnId, &stAttr);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("chn %d get chn attr fail for %#x!\n", pstVdecThreadParam->s32ChnId, s32Ret);
        return (SC_VOID *)(SC_FAILURE);
    }
    if(PT_JPEG != stAttr.enType)
    {
        SAMPLE_PRT("chn %d enType %d do not suport save file!\n", pstVdecThreadParam->s32ChnId, stAttr.enType);
        return (SC_VOID *)(SC_FAILURE);
    }

    while(1)
    {
        if (pstVdecThreadParam->eThreadCtrl == THREAD_CTRL_STOP)
        {
            break;
        }

        s32Ret = SC_MPI_VDEC_GetFrame(pstVdecThreadParam->s32ChnId, &stVFrame, pstVdecThreadParam->s32MilliSec);
        if(SC_SUCCESS == s32Ret)
        {
            s32Cnt++;
            SAVE_FILE_NAME(cSaveFile, pstVdecThreadParam->s32ChnId, s32Cnt, pstVdecThreadParam->cFileName,
                stVFrame.stVFrame.enPixelFormat);
            if (cSaveFile[0] != 0)
            {
                fp = fopen(cSaveFile, "wb");
                if(fp == NULL)
                {
                    SAMPLE_PRT("chn %d can't open file %s in get picture thread!\n", pstVdecThreadParam->s32ChnId, cSaveFile);
                    return (SC_VOID *)(SC_FAILURE);
                }
                printf("\033[0;34m chn %d saving yuv file:%s \033[0;39m \n", pstVdecThreadParam->s32ChnId, cSaveFile);
            }

            if ( (PIXEL_FORMAT_YVU_SEMIPLANAR_420 != stVFrame.stVFrame.enPixelFormat)
                && (PIXEL_FORMAT_YUV_400 != stVFrame.stVFrame.enPixelFormat))
            {
                SAMPLE_COMM_VDEC_SaveRGBFile(fp, &stVFrame.stVFrame);
            }
            else if( (stVFrame.stVFrame.enCompressMode == COMPRESS_MODE_NONE) &&
                (stVFrame.stVFrame.enVideoFormat == VIDEO_FORMAT_LINEAR) )
            {
                SAMPLE_COMM_VDEC_SaveYUVFile_Linear8Bit(fp, &stVFrame.stVFrame);
            }
            else
            {
                SAMPLE_PRT("chn %d enPixelFormat %d do not suport save file!\n", pstVdecThreadParam->s32ChnId,
                    stVFrame.stVFrame.enPixelFormat);
                s32Ret = SC_MPI_VDEC_ReleaseFrame(pstVdecThreadParam->s32ChnId, &stVFrame);
                if (SC_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("chn %d SC_MPI_VDEC_ReleaseFrame fail for s32Ret=0x%x!\n", pstVdecThreadParam->s32ChnId, s32Ret);
                }
                break;
            }
            s32Ret = SC_MPI_VDEC_ReleaseFrame(pstVdecThreadParam->s32ChnId, &stVFrame);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("chn %d SC_MPI_VDEC_ReleaseFrame fail for s32Ret=0x%x!\n", pstVdecThreadParam->s32ChnId, s32Ret);
            }
            if (fp != NULL)
            {
                fclose(fp);
            }
        }
        else
        {
            usleep(1000);
        }
    }

    printf("\033[0;35m chn %d get pic thread return ...  \033[0;39m\n", pstVdecThreadParam->s32ChnId);

    return (SC_VOID *)SC_SUCCESS;
}

SC_VOID SAMPLE_COMM_VDEC_StartGetPic(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecGet, pthread_t *pVdecThread)
{
    SC_S32  i;

    for(i = 0; i < s32ChnNum; i++)
    {
        pVdecThread[i + VDEC_MAX_CHN_NUM] = 0;
        pthread_create(&pVdecThread[i + VDEC_MAX_CHN_NUM], 0, SAMPLE_COMM_VDEC_GetPic, (SC_VOID *)&pstVdecGet[i]);
    }
}

SC_VOID SAMPLE_COMM_VDEC_StopGetPic(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecGet, pthread_t *pVdecThread)
{
    SC_S32  i;

    for(i = 0; i < s32ChnNum; i++)
    {
        pstVdecGet[i].eThreadCtrl = THREAD_CTRL_STOP;
        if(0 != pVdecThread[i + VDEC_MAX_CHN_NUM])
        {
            pthread_join(pVdecThread[i + VDEC_MAX_CHN_NUM], SC_NULL);
            pVdecThread[i + VDEC_MAX_CHN_NUM] = 0;
        }
    }
}

SC_S32 SAMPLE_COMM_VDEC_Start(SC_S32 s32ChnNum, SAMPLE_VDEC_ATTR *pastSampleVdec)
{
    SC_S32  i;
    VDEC_CHN_ATTR_S stChnAttr[VDEC_MAX_CHN_NUM];
    VDEC_CHN_POOL_S stPool;
    VDEC_CHN_PARAM_S stChnParam;
    VDEC_MOD_PARAM_S stModParam;

    //CHECK_RET(SC_MPI_VDEC_GetModParam(&stModParam), "SC_MPI_VDEC_GetModParam");

    stModParam.enVdecVBSource = g_enVdecVBSource;
    //CHECK_RET(SC_MPI_VDEC_SetModParam(&stModParam), "SC_MPI_VDEC_GetModParam");

    for(i = 0; i < s32ChnNum; i++)
    {
        stChnAttr[i].enType           = pastSampleVdec[i].enType;
        stChnAttr[i].enMode           = pastSampleVdec[i].enMode;
        stChnAttr[i].u32PicWidth      = pastSampleVdec[i].u32Width;
        stChnAttr[i].u32PicHeight     = pastSampleVdec[i].u32Height;
        stChnAttr[i].u32StreamBufSize = pastSampleVdec[i].u32Width * pastSampleVdec[i].u32Height;
        stChnAttr[i].u32FrameBufCnt   = pastSampleVdec[i].u32FrameBufCnt;

        if (PT_H264 == pastSampleVdec[i].enType || PT_H265 == pastSampleVdec[i].enType)
        {
            stChnAttr[i].stVdecVideoAttr.u32RefFrameNum     = pastSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum;
            stChnAttr[i].stVdecVideoAttr.bTemporalMvpEnable = 1;
            if ((PT_H264 == pastSampleVdec[i].enType) && (VIDEO_DEC_MODE_IPB != pastSampleVdec[i].stSapmleVdecVideo.enDecMode))
            {
                stChnAttr[i].stVdecVideoAttr.bTemporalMvpEnable = 0;
            }
            stChnAttr[i].u32FrameBufSize  = VDEC_GetPicBufferSize(stChnAttr[i].enType, pastSampleVdec[i].u32Width,
                    pastSampleVdec[i].u32Height,
                    PIXEL_FORMAT_YVU_SEMIPLANAR_420, pastSampleVdec[i].stSapmleVdecVideo.enBitWidth, 0);
        }
        else if (PT_JPEG == pastSampleVdec[i].enType || PT_MJPEG == pastSampleVdec[i].enType)
        {
            stChnAttr[i].enMode           = VIDEO_MODE_FRAME;
            stChnAttr[i].u32FrameBufSize  = VDEC_GetPicBufferSize(stChnAttr[i].enType, pastSampleVdec[i].u32Width,
                    pastSampleVdec[i].u32Height,
                    pastSampleVdec[i].stSapmleVdecPicture.enPixelFormat, DATA_BITWIDTH_8, 0);
        }

        CHECK_CHN_RET(SC_MPI_VDEC_CreateChn(i, &stChnAttr[i]), i, "SC_MPI_VDEC_CreateChn");

        if (VB_SOURCE_USER == g_enVdecVBSource)
        {
            stPool.hPicVbPool = g_ahPicVbPool[i];
            stPool.hTmvVbPool = g_ahTmvVbPool[i];
            CHECK_CHN_RET(SC_MPI_VDEC_AttachVbPool(i, &stPool), i, "SC_MPI_VDEC_AttachVbPool");
        }

        CHECK_CHN_RET(SC_MPI_VDEC_GetChnParam(i, &stChnParam), i, "SC_MPI_VDEC_GetChnParam");
        if (PT_H264 == pastSampleVdec[i].enType || PT_H265 == pastSampleVdec[i].enType)
        {
            stChnParam.stVdecVideoParam.enDecMode         = pastSampleVdec[i].stSapmleVdecVideo.enDecMode;
            stChnParam.stVdecVideoParam.enCompressMode    = COMPRESS_MODE_NONE;
            stChnParam.stVdecVideoParam.enVideoFormat     = VIDEO_FORMAT_TILE_64x16;
            if(VIDEO_DEC_MODE_IPB == stChnParam.stVdecVideoParam.enDecMode)
            {
                stChnParam.stVdecVideoParam.enOutputOrder = VIDEO_OUTPUT_ORDER_DISP;
            }
            else
            {
                stChnParam.stVdecVideoParam.enOutputOrder = VIDEO_OUTPUT_ORDER_DEC;
            }
        }
        else
        {
            stChnParam.stVdecPictureParam.enPixelFormat   = pastSampleVdec[i].stSapmleVdecPicture.enPixelFormat;
            stChnParam.stVdecPictureParam.u32Alpha        = pastSampleVdec[i].stSapmleVdecPicture.u32Alpha;
        }
        stChnParam.u32DisplayFrameNum                     = pastSampleVdec[i].u32DisplayFrameNum;
        CHECK_CHN_RET(SC_MPI_VDEC_SetChnParam(i, &stChnParam), i, "SC_MPI_VDEC_GetChnParam");

        CHECK_CHN_RET(SC_MPI_VDEC_StartRecvStream(i), i, "SC_MPI_VDEC_StartRecvStream");
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VDEC_Stop(SC_S32 s32ChnNum)
{
    SC_S32 i;

    for(i = 0; i < s32ChnNum; i++)
    {
        CHECK_CHN_RET(SC_MPI_VDEC_StopRecvStream(i), i, "SC_MPI_VDEC_StopRecvStream");
        CHECK_CHN_RET(SC_MPI_VDEC_DestroyChn(i), i, "SC_MPI_VDEC_DestroyChn");
    }

    return SC_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
