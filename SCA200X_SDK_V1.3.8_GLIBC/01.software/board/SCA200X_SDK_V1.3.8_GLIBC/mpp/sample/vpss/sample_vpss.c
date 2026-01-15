/**
 * @file     sample_vpss.c
 * @brief    vpss模块示例代码
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
 * @date    2021-10-29 创建文件
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
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include "sample_comm.h"

static VPSS_GRP Dy_VpssGrp = 0;
static VPSS_CHN Dy_VpssChn = 0;

static SC_S32 all_exit = SC_FALSE;
static SC_S32  s32Index;

#define MAX_FRM_WIDTH   20480

#define RES_PATH "./res/"

#define YUV_1920_1080 RES_PATH"1920_1080_p420.yuv"

typedef struct
{
    SC_CHAR          filename[1024];
    SC_U32           u32Width;
    SC_U32           u32Height;
    PIXEL_FORMAT_E   enPixelFmt;
    VIDEO_FORMAT_E   enVideoFmt;
    SC_BOOL          bQuit;
    SC_BOOL          bDestroy;
    SC_S32           s32ToDev;
    DYNAMIC_RANGE_E  enSrcDynamicRange;
    SC_U32           u32ChnNum;
    pthread_t        tid;

} SAMPLE_RFILE_ThreadInfo;

typedef struct
{
    SC_BOOL          bQuit;
    SC_BOOL          bDestroy;
    VPSS_GRP         VpssGrp;
    VPSS_CHN         VpssChn;
    SC_U32           FrameCnt;
    pthread_t        tid;
} SAMPLE_WFILE_ThreadInfo;

#define ALIGN_BACK(x, a)        ((a) * (((x + a -1) / (a))))

#if 1
SC_S32 get_tick_count(struct timeval *p_tv)
{
    SC_S32 value = 0;
    if(NULL == p_tv)
    {
        return -1;
    }
    gettimeofday(p_tv, NULL);
    value = p_tv->tv_sec * MILLION + p_tv->tv_usec;
    return value;
}

SC_S32 time_diff(struct timeval *p_start)
{
    struct timeval end;
    struct timeval temp;
    SC_S32 diff_value = 0;
    if(NULL == p_start)
    {
        return -1;
    }
    if(0 == p_start->tv_sec && 0 == p_start->tv_usec)
    {
        gettimeofday(p_start, NULL);
    }
    gettimeofday(&end, NULL);
    if((end.tv_usec - p_start->tv_usec) < 0)
    {
        temp.tv_sec = end.tv_sec - p_start->tv_sec - 1;
        temp.tv_usec = MILLION + end.tv_usec - p_start->tv_usec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - p_start->tv_sec;
        temp.tv_usec = end.tv_usec - p_start->tv_usec;
    }
    diff_value = temp.tv_sec * MILLION + temp.tv_usec;
    if(diff_value < 0)
    {
        gettimeofday(p_start, NULL);
        return 1;
    }
    else
    {
        return diff_value;
    }
}

#endif

SC_S32 SAMPLE_SYS_Init(SC_U32 u32SupplementConfig, SAMPLE_SNS_TYPE_E  enSnsType)
{
    SC_S32 s32Ret;
    SC_U64 u64BlkSize;
    VB_CONFIG_S stVbConf;
    PIC_SIZE_E enSnsSize;
    SIZE_S     stSnsSize;

    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    u64BlkSize = COMMON_GetPicBufferSize(stSnsSize.u32Width, stSnsSize.u32Height, PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize   = u64BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt    = 15;

    u64BlkSize = COMMON_GetPicBufferSize(1920, 1080, PIXEL_FORMAT_YVU_SEMIPLANAR_422, DATA_BITWIDTH_8, COMPRESS_MODE_SEG,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize   = u64BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt    = 15;

    stVbConf.u32MaxPoolCnt = 2;

    if(0 == u32SupplementConfig)
    {
        s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    }
    else
    {
        s32Ret = SAMPLE_COMM_SYS_InitWithVbSupplement(&stVbConf, u32SupplementConfig);
    }
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    return SC_SUCCESS;
}

SC_VOID SAMPLE_SYS_Exit(void)
{
    SC_MPI_SYS_Exit();
    SC_MPI_VB_Exit();
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_VPSS_HandleSig(SC_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        all_exit = SC_TRUE;
        usleep(5000000);
        SAMPLE_COMM_VENC_StopGetStream();
        SAMPLE_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

SC_S32 SAMPLE_VI_Init( SAMPLE_VI_CONFIG_S *pstViConfig, SC_BOOL bLowDelay, SC_U32 u32SupplementConfig)
{
    SC_S32              s32Ret;
    SAMPLE_SNS_TYPE_E   enSnsType;
    //    ISP_CTRL_PARAM_S    stIspCtrlParam;
    SC_U32              u32FrameRate;

    enSnsType = pstViConfig->astViInfo[0].stSnsInfo.enSnsType;

    pstViConfig->as32WorkingViId[0]                           = 0;
    //pstViConfig->s32WorkingViNum                              = 1;

    pstViConfig->astViInfo[0].stSnsInfo.MipiDev            = SAMPLE_COMM_VI_GetComboDevBySensor(
            pstViConfig->astViInfo[0].stSnsInfo.enSnsType, 0);

    //pstViConfig->astViInfo[0].stDevInfo.ViDev              = ViDev0;
    pstViConfig->astViInfo[0].stDevInfo.enWDRMode          = WDR_MODE_NONE;

    if(SC_TRUE == bLowDelay)
    {
        pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode     = VI_ONLINE_VPSS_ONLINE;
    }
    else
    {
        pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode     = VI_OFFLINE_VPSS_OFFLINE;
    }
    s32Ret = SAMPLE_SYS_Init(u32SupplementConfig, enSnsType);
    if(s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("Init SYS err for %#x!\n", s32Ret);
        return s32Ret;
    }

    //if(8k == enSnsType)
    //{
    //    pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode       = VI_PARALLEL_VPSS_OFFLINE;
    //}

    //pstViConfig->astViInfo[0].stPipeInfo.aPipe[0]          = ViPipe0;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[1]          = -1;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[2]          = -1;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[3]          = -1;

    //pstViConfig->astViInfo[0].stChnInfo.ViChn              = ViChn;
    //pstViConfig->astViInfo[0].stChnInfo.enPixFormat        = PIXEL_FORMAT_YVU_PLANAR_420;
    //pstViConfig->astViInfo[0].stChnInfo.enDynamicRange     = enDynamicRange;
    pstViConfig->astViInfo[0].stChnInfo.enVideoFormat      = VIDEO_FORMAT_LINEAR;
    pstViConfig->astViInfo[0].stChnInfo.enCompressMode     = COMPRESS_MODE_SEG;//COMPRESS_MODE_SEG;
    s32Ret = SAMPLE_COMM_VI_SetParam(pstViConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam failed with %d!\n", s32Ret);
        return s32Ret;
    }

    SAMPLE_COMM_VI_GetFrameRateBySensor(enSnsType, &u32FrameRate);

    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_COMM_SYS_Exit();
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi failed with %d!\n", s32Ret);
        return s32Ret;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_VI_CheckSensor(SAMPLE_SNS_TYPE_E   enSnsType, SIZE_S  stSize)
{
    SC_S32 s32Ret;
    SIZE_S          stSnsSize;
    PIC_SIZE_E      enSnsSize;

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    if((stSnsSize.u32Width < stSize.u32Width) || (stSnsSize.u32Height < stSize.u32Height))
    {
        SAMPLE_PRT("Sensor size is (%d,%d), but encode chnl is (%d,%d) !\n",
            stSnsSize.u32Width, stSnsSize.u32Height, stSize.u32Width, stSize.u32Height);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_FILE_SYS_Init(ROTATION_E rotate, SC_U32 u32Align)
{
    SC_S32      s32Ret;
    VB_CONFIG_S stVbConf = {0};
    SC_U32      u32BlkSize;
    SC_S32      cnt = 0;

    SC_MPI_SYS_Exit();
    SC_MPI_VB_Exit();

    u32BlkSize = COMMON_GetPicBufferSize(1920, 1080, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
            u32Align);
    stVbConf.astCommPool[cnt].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[cnt].u32BlkCnt   = 10;
    cnt++;

    u32BlkSize = COMMON_GetPicBufferSize(1280, 720, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, u32Align);
    stVbConf.astCommPool[cnt].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[cnt].u32BlkCnt   = 6;
    cnt++;

    if ((ROTATION_90 == rotate) || (ROTATION_270 == rotate))
    {
        u32BlkSize = COMMON_GetPicBufferSize(1080, 1920, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
                u32Align);
        stVbConf.astCommPool[cnt].u64BlkSize  = u32BlkSize;
        stVbConf.astCommPool[cnt].u32BlkCnt   = 10;
        cnt++;
    }

    stVbConf.u32MaxPoolCnt = cnt;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if(s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SYS_Init fail for %#x!\n", s32Ret);
    }

    return s32Ret;
}

SC_S32 SAMPLE_VPSS_Init(VPSS_GRP VpssGrp, SC_BOOL *pabChnEnable, DYNAMIC_RANGE_E enDynamicRange,
    PIXEL_FORMAT_E enPixelFormat, SIZE_S inSize, SIZE_S outSize[])
{
    SC_S32 i;
    SC_S32 s32Ret;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    stVpssGrpAttr.enDynamicRange = enDynamicRange;
    stVpssGrpAttr.enPixelFormat  = enPixelFormat;
    stVpssGrpAttr.u32MaxW        = inSize.u32Width;
    stVpssGrpAttr.u32MaxH        = inSize.u32Height;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.bNrEn = SC_TRUE;

    for(i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if(SC_TRUE == pabChnEnable[i])
        {
            stVpssChnAttr[i].u32Width                     = outSize[i].u32Width;
            stVpssChnAttr[i].u32Height                    = outSize[i].u32Height;
            stVpssChnAttr[i].enChnMode                    = VPSS_CHN_MODE_USER;
            stVpssChnAttr[i].enCompressMode               = COMPRESS_MODE_NONE;//COMPRESS_MODE_SEG;
            stVpssChnAttr[i].enDynamicRange               = enDynamicRange;
            stVpssChnAttr[i].enPixelFormat                = enPixelFormat;
            stVpssChnAttr[i].stFrameRate.s32SrcFrameRate  = -1;
            stVpssChnAttr[i].stFrameRate.s32DstFrameRate  = -1;
            stVpssChnAttr[i].u32Depth                     = 0;
            stVpssChnAttr[i].bMirror                      = SC_FALSE;
            stVpssChnAttr[i].bFlip                        = SC_FALSE;
            stVpssChnAttr[i].enVideoFormat                = VIDEO_FORMAT_LINEAR;
            stVpssChnAttr[i].stAspectRatio.enMode         = ASPECT_RATIO_NONE;
        }
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, pabChnEnable, &stVpssGrpAttr, stVpssChnAttr);
    if(s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
    }

    return s32Ret;
}

/******************************************************************************
* function :  VI + VPSS + VENC (H.265@1080p@30fps + H264@720p@30fps)
******************************************************************************/
SC_S32 SAMPLE_VI_VPSS_VENC(void)
{
    SC_S32 i;
    SC_S32 s32Ret;
    SIZE_S          stSize[2];
    PIC_SIZE_E      enSize[2]     = {PIC_1080P, PIC_720P};
    SC_S32          s32ChnNum     = 2;
    VENC_CHN        VencChn[2]    = {0, 1};
    SC_U32          u32Profile[2] = {0, 0};
    PAYLOAD_TYPE_E  enPayLoad[2]  = {PT_H265, PT_H264};
    VENC_GOP_MODE_E enGopMode;
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_RC_E     enRcMode;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSnsSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP        VpssGrp        = 0;
    VPSS_CHN        VpssChn[2]     = {0, 1};
    SC_BOOL         abChnEnable[4] = {1, 1, 0, 0};

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

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    for(i = 0; i < s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSize[i], &stSize[i]);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
            return s32Ret;
        }
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt = 3;

    u32BlkSize = COMMON_GetPicBufferSize(stSize[0].u32Width, stSize[0].u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize[0].u32Width, stSize[0].u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    u32BlkSize = COMMON_GetPicBufferSize(stSize[1].u32Width, stSize[1].u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[2].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt   = 4;

    if ((stSnsSize.u32Width != stSize[0].u32Width) || (stSnsSize.u32Height != stSize[0].u32Height))
    {
        u32BlkSize = COMMON_GetPicBufferSize(stSnsSize.u32Width, stSnsSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
        stVbConf.astCommPool[3].u64BlkSize  = u32BlkSize;
        stVbConf.astCommPool[3].u32BlkCnt   = 6;
        stVbConf.u32MaxPoolCnt++;
    }

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

    s32Ret = SAMPLE_VPSS_Init(VpssGrp, abChnEnable, DYNAMIC_RANGE_SDR8, PIXEL_FORMAT_YVU_PLANAR_420, stSnsSize, stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Init VPSS err for %#x!\n", s32Ret);
        goto EXIT_VI_STOP;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if(s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("VI Bind VPSS err for %#x!\n", s32Ret);
        goto EXIT_VPSS_STOP;
    }

    /******************************************
     start stream venc
     ******************************************/

    enRcMode = SAMPLE_RC_CBR;

    enGopMode  = VENC_GOPMODE_NORMALP;

    s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Get GopAttr for %#x!\n", s32Ret);
        goto EXIT_VI_VPSS_UNBIND;
    }

    /***encode h.265 **/
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enPayLoad[0], enSize[0], enRcMode, u32Profile[0], &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT_VI_VPSS_UNBIND;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn[0], VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Get GopAttr failed for %#x!\n", s32Ret);
        goto EXIT_VENC_H265_STOP;
    }

    /***encode h.264 **/
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[1], enPayLoad[1], enSize[1], enRcMode, u32Profile[1], &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT_VENC_H265_UnBind;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn[1], VencChn[1]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed for %#x!\n", s32Ret);
        goto EXIT_VENC_H264_STOP;
    }

    /******************************************
     stream save process
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, s32ChnNum);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto EXIT_VENC_H264_UnBind;
    }

    while (!all_exit)
    {
        usleep(10000);
    }

    printf("\n\n\n---------------SAMPLE_VI_VPSS_VENC to exit---------------\n\n\n");

    /******************************************
     exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

EXIT_VENC_H264_UnBind:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn[1], VencChn[1]);
EXIT_VENC_H264_STOP:
    SAMPLE_COMM_VENC_Stop(VencChn[1]);
EXIT_VENC_H265_UnBind:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn[0], VencChn[0]);
EXIT_VENC_H265_STOP:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT_VI_VPSS_UNBIND:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT_VPSS_STOP:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
    SAMPLE_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  VI + VPSS + VO (1080p@30fps)
******************************************************************************/
SC_S32 SAMPLE_VI_VPSS_VO(void)
{
    SC_S32             s32Ret;
    SIZE_S             stSize[2] = {{1920, 1080}, {1280, 720}};

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSnsSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_CHN           VpssChn        = 0;
    SC_BOOL            abChnEnable[4] = {1, 0, 0, 0};

    VO_INTF_TYPE_E     enVoIntfType = VO_INTF_HDMI;
    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

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

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt = 3;

    u32BlkSize = COMMON_GetPicBufferSize(stSize[0].u32Width, stSize[0].u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize[0].u32Width, stSize[0].u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP,
            COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    u32BlkSize = COMMON_GetPicBufferSize(stSize[1].u32Width, stSize[1].u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[2].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt   = 4;

    if ((stSnsSize.u32Width != stSize[0].u32Width) || (stSnsSize.u32Height != stSize[0].u32Height))
    {
        u32BlkSize = COMMON_GetPicBufferSize(stSnsSize.u32Width, stSnsSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
        stVbConf.astCommPool[3].u64BlkSize  = u32BlkSize;
        stVbConf.astCommPool[3].u32BlkCnt   = 6;
        stVbConf.u32MaxPoolCnt++;
    }

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

    s32Ret = SAMPLE_VPSS_Init(VpssGrp, abChnEnable, DYNAMIC_RANGE_SDR8, PIXEL_FORMAT_YVU_PLANAR_420, stSnsSize, stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Init VPSS err for %#x!\n", s32Ret);
        goto EXIT_VI_STOP;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if(s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("VI Bind VPSS err for %#x!\n", s32Ret);
        goto EXIT_VPSS_STOP;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    stVoConfig.enVoIntfType = enVoIntfType;
    stVoConfig.enPicSize = PIC_1080P;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VI_VPSS_UNBIND;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VO_STOP;
    }

    while (!all_exit)
    {
        usleep(10000);
    }

    printf("\n\n\n---------------SAMPLE_VI_VPSS_VO to exit---------------\n\n\n");

    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
EXIT_VO_STOP:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT_VI_VPSS_UNBIND:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT_VPSS_STOP:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
    SAMPLE_SYS_Exit();

    return s32Ret;
}

SC_S32 SAMPLE_RFILE_ReadOneFrame( FILE *fp, SC_U8 *pY, SC_U8 *pU, SC_U8 *pV,
    SC_U32 width, SC_U32 height, SC_U32 stride, SC_U32 stride2,
    PIXEL_FORMAT_E enPixFrm)
{
    SC_U8 *pDst;
    SC_U32 u32UVHeight;
    SC_U32 u32Row;

    if(enPixFrm == PIXEL_FORMAT_YVU_SEMIPLANAR_422)
    {
        u32UVHeight = height;
    }
    else
    {
        u32UVHeight = height / 2;
    }

    pDst = pY;
    for ( u32Row = 0; u32Row < height; u32Row++ )
    {
        if (fread( pDst, 1, width, fp ) != width)
        {
            return -1;
        }
        pDst += stride;
    }

    pDst = pU;
    for ( u32Row = 0; u32Row < u32UVHeight; u32Row++ )
    {
        if(fread( pDst, 1, width / 2, fp ) != width / 2)
        {
            return -2;
        }
        pDst += stride2;
    }

    pDst = pV;
    for ( u32Row = 0; u32Row < u32UVHeight; u32Row++ )
    {
        if(fread( pDst, 1, width / 2, fp ) != width / 2)
        {
            return -3;
        }
        pDst += stride2;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_VPSS_PlanToSemi(SC_U8 *pY, SC_S32 yStride,
    SC_U8 *pU, SC_S32 uStride,
    SC_U8 *pV, SC_S32 vStride,
    SC_S32 picWidth, SC_S32 picHeight, PIXEL_FORMAT_E enPixFrm)
{
    SC_S32 i;
    SC_U8 *pTmpU, *ptu;
    SC_U8 *pTmpV, *ptv;

    SC_S32 s32HafW = uStride >> 1 ;
    SC_S32 s32HafH;

    if(enPixFrm == PIXEL_FORMAT_YVU_SEMIPLANAR_422)
    {
        s32HafH = picHeight;
    }
    else
    {
        s32HafH = picHeight >> 1 ;
    }

    SC_S32 s32Size = s32HafW * s32HafH;

    pTmpU = malloc( s32Size );
    ptu = pTmpU;
    pTmpV = malloc( s32Size );
    ptv = pTmpV;

    memcpy(pTmpU, pU, s32Size);
    memcpy(pTmpV, pV, s32Size);

    for(i = 0; i<s32Size >> 1; i++)
    {
        *pU++ = *pTmpV++;
        *pU++ = *pTmpU++;

    }
    for(i = 0; i<s32Size >> 1; i++)
    {
        *pV++ = *pTmpV++;
        *pV++ = *pTmpU++;
    }

    free( ptu );
    free( ptv );

    return SC_SUCCESS;
}

static void sample_yuv_8bit_dump(VIDEO_FRAME_S *pVBuf, FILE *pfd)
{
    unsigned int h;
    char *pMemContent;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    SC_U32 u32UvHeight =
        0;/*When the storage format is a planar format, this variable is used to keep the height of the UV component */
    char *pVBufVirt_Y = NULL;
    char *pVBufVirt_C = NULL;
    SC_U32 Ysize, Csize;

    Ysize = (pVBuf->u32Stride[0]) * (pVBuf->u32Height);

    if (PIXEL_FORMAT_YVU_PLANAR_420 == enPixelFormat)
    {
        Csize = (pVBuf->u32Stride[1]) * (pVBuf->u32Height) / 2;
        u32UvHeight = pVBuf->u32Height / 2;
    }
    else
    {
        fprintf(stderr, "no support video format(%d)\n", enPixelFormat);
        return;
    }

    pVBufVirt_Y = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[0], Ysize);
    if (NULL == pVBufVirt_Y)
    {
        printf("SC_MPI_SYS_Mmap Y error!\n");
        return;
    }

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);

    for (h = 0; h < pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h * pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }
    fflush(pfd);
    SC_MPI_SYS_Munmap(pVBufVirt_Y, Ysize);
    pVBufVirt_Y = NULL;

    if (PIXEL_FORMAT_YVU_PLANAR_420 == enPixelFormat)
    {
        pVBufVirt_C = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[1], Csize);
        if (NULL == pVBufVirt_C)
        {
            printf("SC_MPI_SYS_Mmap V error!\n");
            return;
        }

        /* save V ----------------------------------------------------------------*/
        fprintf(stderr, "V......");
        fflush(stderr);

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];

            fwrite(pMemContent, pVBuf->u32Width / 2, 1, pfd);
        }
        fflush(pfd);
        SC_MPI_SYS_Munmap(pVBufVirt_C, Csize);
        pVBufVirt_C = NULL;

        pVBufVirt_C = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[2], Csize);
        if (NULL == pVBufVirt_C)
        {
            printf("SC_MPI_SYS_Mmap U error!\n");
            return;
        }

        /* save U ----------------------------------------------------------------*/
        fprintf(stderr, "U......");
        fflush(stderr);

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[2];

            fwrite(pMemContent, pVBuf->u32Width / 2, 1, pfd);
        }
        fflush(pfd);
        SC_MPI_SYS_Munmap(pVBufVirt_C, Csize);
        pVBufVirt_C = NULL;
    }

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);
}

SC_VOID *SAMPLE_RFILE_SendVpss(SC_VOID *pData)
{
    VPSS_GRP VpssGrp = 0;
    VPSS_GRP_PIPE VpssGrpPipe = 0;

    SC_S32 i;
    SC_S32 s32Ret;
    FILE *pfd;
    VB_BLK hBlkHdl;
    SC_U32 u32Size;
    SC_U32 u32SrcWidth;
    SC_U32 u32SrcHeight;
    VB_POOL Pool;
    VIDEO_FRAME_INFO_S stUserFrame;
    VB_POOL_CONFIG_S stVbPoolCfg;
    SC_U32 u32LumaSize = 0;
    SC_U32 u32ChromaSize = 0;
    struct timeval tv_frame;
    SC_S32 pre_tvdiff = 0;

    get_tick_count(&tv_frame);

    SAMPLE_RFILE_ThreadInfo *pInfo = (SAMPLE_RFILE_ThreadInfo *)pData;

    memset(&stUserFrame, 0x0, sizeof(VIDEO_FRAME_INFO_S));

    u32SrcWidth = pInfo->u32Width;
    u32SrcHeight = pInfo->u32Height;

    pfd = fopen(pInfo->filename, "rb");
    if (pfd == SC_NULL)
    {
        printf("open file %s fail \n", pInfo->filename);
        return SC_NULL;
    }
    else
    {
        printf("open file %s success!\n", pInfo->filename);
    }

    fflush(stdout);

    u32Size = ALIGN_BACK(u32SrcWidth, DEFAULT_ALIGN) * u32SrcHeight * 2;
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Size;
    stVbPoolCfg.u32BlkCnt = 4;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    Pool = SC_MPI_VB_CreatePool(&stVbPoolCfg);
    if (Pool == VB_INVALID_POOLID)
    {
        printf("Maybe you not call sys init\n");
        return SC_NULL;
    }

    stUserFrame.stVFrame.enField = VIDEO_FIELD_INTERLACED;
    stUserFrame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    stUserFrame.stVFrame.enPixelFormat = pInfo->enPixelFmt;
    stUserFrame.stVFrame.enVideoFormat = pInfo->enVideoFmt;
    stUserFrame.stVFrame.u32Width = u32SrcWidth;
    stUserFrame.stVFrame.u32Height = u32SrcHeight;
    stUserFrame.stVFrame.u32Stride[0] = ALIGN_BACK(u32SrcWidth, DEFAULT_ALIGN);
    stUserFrame.stVFrame.u32Stride[1] = ALIGN_BACK(u32SrcWidth, DEFAULT_ALIGN);
    stUserFrame.stVFrame.u32Stride[2] = ALIGN_BACK(u32SrcWidth, DEFAULT_ALIGN);
    stUserFrame.stVFrame.u32TimeRef = 0;
    stUserFrame.stVFrame.u64PTS = 0;
    stUserFrame.stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

    u32LumaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight;
    if (pInfo->enPixelFmt == PIXEL_FORMAT_YVU_SEMIPLANAR_422)
    {
        u32ChromaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight / 2;
    }
    else if (pInfo->enPixelFmt == PIXEL_FORMAT_YVU_SEMIPLANAR_420)
    {
        u32ChromaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight / 4;
    }
    else if (pInfo->enPixelFmt == PIXEL_FORMAT_YVU_PLANAR_420)
    {
        u32ChromaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight / 4;
        stUserFrame.stVFrame.u32Stride[1] = stUserFrame.stVFrame.u32Stride[0]/2;
        stUserFrame.stVFrame.u32Stride[2] = stUserFrame.stVFrame.u32Stride[1];
    }
    else
    {
        printf("[Func]:%s [Line]:%d->enPixelFmt(%d) error!\n",
            __FUNCTION__, __LINE__, pInfo->enPixelFmt);
        return SC_NULL;
    }

    do
    {
        //framerate ctrl
        if ((time_diff(&tv_frame) + pre_tvdiff) < 1000000 / 30)
        {
            usleep(10000);
            continue;
        }
        //pre_tvdiff = (time_diff(&tv_frame) + pre_tvdiff);
        //printf("time_diff(&tv_frame):%d pre_tvdiff:%d\n", time_diff(&tv_frame), pre_tvdiff);
        get_tick_count(&tv_frame);

        if (feof(pfd) != 0)
        {
            fseek(pfd, 0, SEEK_SET);
        }

        hBlkHdl = SC_MPI_VB_GetBlock( Pool, u32Size, NULL);
        if (hBlkHdl == VB_INVALID_HANDLE)
        {
            printf("[VOU_MST_File2VO] get vb fail!!!\n");
            sleep(1);
            continue;
        }

        stUserFrame.u32PoolId = SC_MPI_VB_Handle2PoolId(hBlkHdl);
        stUserFrame.stVFrame.u64PhyAddr[0] = SC_MPI_VB_Handle2PhysAddr( hBlkHdl );
        stUserFrame.stVFrame.u64PhyAddr[1] = stUserFrame.stVFrame.u64PhyAddr[0] + u32LumaSize;
        stUserFrame.stVFrame.u64PhyAddr[2] = stUserFrame.stVFrame.u64PhyAddr[1] + u32ChromaSize;

        stUserFrame.stVFrame.u64VirAddr[0] = (SC_UL)SC_MPI_SYS_Mmap(stUserFrame.stVFrame.u64PhyAddr[0], u32Size);
        stUserFrame.stVFrame.u64VirAddr[1] = (SC_UL)(stUserFrame.stVFrame.u64VirAddr[0]) + u32LumaSize;
        stUserFrame.stVFrame.u64VirAddr[2] = (SC_UL)(stUserFrame.stVFrame.u64VirAddr[1]) + u32ChromaSize;

        if(pInfo->enPixelFmt != PIXEL_FORMAT_YVU_PLANAR_420)
        {
            s32Ret = SAMPLE_RFILE_ReadOneFrame( pfd, (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0],
                    (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[1], (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[2],
                    stUserFrame.stVFrame.u32Width, stUserFrame.stVFrame.u32Height,
                    stUserFrame.stVFrame.u32Stride[0], stUserFrame.stVFrame.u32Stride[1] >> 1,
                    stUserFrame.stVFrame.enPixelFormat);
            if(s32Ret == SC_SUCCESS)
            {
                SAMPLE_VPSS_PlanToSemi( (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0], stUserFrame.stVFrame.u32Stride[0],
                    (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[1], stUserFrame.stVFrame.u32Stride[1],
                    (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[2], stUserFrame.stVFrame.u32Stride[1],
                    stUserFrame.stVFrame.u32Width,    stUserFrame.stVFrame.u32Height,
                    stUserFrame.stVFrame.enPixelFormat);
            }
            else
            {
                goto OUT;
            }
        }
        else
        {
            s32Ret = SAMPLE_RFILE_ReadOneFrame( pfd, (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0],
                    (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[1], (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[2],
                    stUserFrame.stVFrame.u32Width, stUserFrame.stVFrame.u32Height,
                    stUserFrame.stVFrame.u32Stride[0], stUserFrame.stVFrame.u32Stride[1],
                    stUserFrame.stVFrame.enPixelFormat);
        }

        stUserFrame.stVFrame.u64PTS += 40000;
        stUserFrame.stVFrame.u32TimeRef += 40000;

        for (i = 0; i < pInfo->u32ChnNum; i++)
        {
            s32Ret = SC_MPI_VPSS_SendFrame(VpssGrp, VpssGrpPipe, &stUserFrame, 0);
        }

OUT:
        SC_MPI_SYS_Munmap((SC_VOID *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0], u32Size);
        SC_MPI_VB_ReleaseBlock(hBlkHdl);

    } while(pInfo->bQuit == SC_FALSE);

    while (pInfo->bDestroy == SC_FALSE)
    {
        ;
    }

    fclose(pfd);

    printf("\n\n\n---------------SAMPLE_RFILE_SendVpss to exit---------------\n\n\n");

    return NULL;
}

SC_VOID SAMPLE_RFILE_GetThreadInfo(SAMPLE_RFILE_ThreadInfo *pstThreadInfo, SIZE_S *pstFrmSize)
{
    pstThreadInfo->bDestroy = SC_FALSE;
    pstThreadInfo->bQuit    = SC_FALSE;
    pstThreadInfo->enPixelFmt = PIXEL_FORMAT_YVU_PLANAR_420;
    pstThreadInfo->enVideoFmt = VIDEO_FORMAT_LINEAR;
    pstThreadInfo->u32Width = pstFrmSize->u32Width;
    pstThreadInfo->u32Height = pstFrmSize->u32Height;
    pstThreadInfo->u32ChnNum = 1;

    return;
}

SC_VOID SAMPLE_RFILE_StartThd(SAMPLE_RFILE_ThreadInfo *pstThreadInfo,
    SC_CHAR filename[256], SIZE_S *pstFrmSize)
{
    /* CREATE USER THREAD */
    SAMPLE_RFILE_GetThreadInfo(pstThreadInfo, pstFrmSize);

    strncpy(pstThreadInfo->filename, filename, sizeof(pstThreadInfo->filename) - 1);
    pthread_create(&pstThreadInfo->tid, NULL, SAMPLE_RFILE_SendVpss, (SC_VOID *)pstThreadInfo);

    return;
}

SC_VOID SAMPLE_WFILE_GetThreadInfo(SAMPLE_WFILE_ThreadInfo *pstThreadInfo,
    VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    pstThreadInfo->bDestroy = SC_FALSE;
    pstThreadInfo->bQuit    = SC_FALSE;
    pstThreadInfo->VpssGrp  = VpssGrp;
    pstThreadInfo->VpssChn  = VpssChn;
    pstThreadInfo->FrameCnt = 0xFFFFFFFF;

    return;
}

static SC_S32 VPSS_Restore(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
    VIDEO_FRAME_INFO_S *pFrame, FILE *pfd)
{
    if ((VB_INVALID_POOLID != pFrame->u32PoolId)
        && (-1 == VpssChn))
    {
        SC_MPI_VPSS_ReleaseGrpFrame(VpssGrp, 0, pFrame);
        pFrame->u32PoolId = VB_INVALID_POOLID;
    }
    else if (VB_INVALID_POOLID != pFrame->u32PoolId)
    {
        SC_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, pFrame);
        pFrame->u32PoolId = VB_INVALID_POOLID;
    }

    if (pfd)
    {
        fclose(pfd);
        pfd = NULL;
    }

    return SC_SUCCESS;
}

SC_VOID *SAMPLE_VPSS_DumpGrp(SC_VOID *pData)
{
    SAMPLE_WFILE_ThreadInfo *pInfo = (SAMPLE_WFILE_ThreadInfo *)pData;

    SC_CHAR szYuvName[128];
    SC_CHAR szPixFrm[10];
    SC_S32 s32Ret;
    VIDEO_FRAME_INFO_S stFrame;
    FILE *pfd = NULL;
    int frameid = 0;

    usleep(5000000);

    memset(&stFrame, 0, sizeof(stFrame));
    stFrame.u32PoolId = VB_INVALID_POOLID;

    s32Ret = SC_MPI_VPSS_GetGrpFrame(pInfo->VpssGrp, 0, &stFrame);
    if (s32Ret)
    {
        printf("get grp frame error, now exit !!!\n");
        return NULL;
    }

    switch (stFrame.stVFrame.enPixelFormat)
    {
    case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
    case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        snprintf(szPixFrm, 10, "P420");
        break;

    case PIXEL_FORMAT_YVU_SEMIPLANAR_422:
    case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
        snprintf(szPixFrm, 10, "P422");
        break;

    default:
        snprintf(szPixFrm, 10, "P420");
        break;
    }

    /* make file name */
    snprintf(szYuvName, 128, "./vpss_grp%d_%dx%d_%s_%u.yuv", pInfo->VpssGrp,
        stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height, szPixFrm, pInfo->FrameCnt);
    printf("Dump YUV frame of vpss grp %d  to file: \"%s\"\n", pInfo->VpssGrp, szYuvName);
    fflush(stdout);

    s32Ret = SC_MPI_VPSS_ReleaseGrpFrame(pInfo->VpssGrp, 0, &stFrame);
    if (SC_SUCCESS != s32Ret)
    {
        printf("Release frame error ,now exit !!!\n");
        VPSS_Restore(pInfo->VpssGrp, -1, &stFrame, NULL);
        return NULL;
    }

    stFrame.u32PoolId = VB_INVALID_POOLID;
    /* open file */
    pfd = fopen(szYuvName, "wb");

    if (SC_NULL == pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));
        VPSS_Restore(pInfo->VpssGrp, -1, &stFrame, NULL);
        return NULL;
    }

    /* get frame  */
    while ((pInfo->bQuit != SC_TRUE) && (pInfo->FrameCnt-- != 0))
    {
        if (SC_MPI_VPSS_GetGrpFrame(pInfo->VpssGrp, 0, &stFrame) != SC_SUCCESS)
        {
            printf("Get frame fail\n");
            usleep(1000);
            continue;
        }
        printf("u32Width:%d u32Height:%d u32Stride:%d-%d u64PTS:%llu\n",
            stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height,
            stFrame.stVFrame.u32Stride[0], stFrame.stVFrame.u32Stride[1], stFrame.stVFrame.u64PTS);

        if (DYNAMIC_RANGE_SDR8 == stFrame.stVFrame.enDynamicRange)
        {
            sample_yuv_8bit_dump(&stFrame.stVFrame, pfd);
        }
        else
        {
            SC_MPI_VPSS_ReleaseGrpFrame(pInfo->VpssGrp, 0, &stFrame);
            printf("frame enDynamicRange error, now exit !!!\n");
            VPSS_Restore(pInfo->VpssGrp, -1, &stFrame, pfd);
            return NULL;
        }

        printf("Get frame 0x%x!!\n", pInfo->FrameCnt);
        /* release frame after using */
        s32Ret = SC_MPI_VPSS_ReleaseGrpFrame(pInfo->VpssGrp, -1, &stFrame);

        if (SC_SUCCESS != s32Ret)
        {
            printf("Release frame error ,now exit !!!\n");
            VPSS_Restore(pInfo->VpssGrp, -1, &stFrame, pfd);
            return NULL;
        }

        stFrame.u32PoolId = VB_INVALID_POOLID;

        printf("get grp frameid[%d]:%d\n", pInfo->VpssGrp, frameid++);

    }

    VPSS_Restore(pInfo->VpssGrp, -1, &stFrame, pfd);

    printf("\n\n\n---------------SAMPLE_VPSS_DumpGrp[%d] to exit---------------\n\n\n", pInfo->VpssGrp);

    return NULL;
}

SC_VOID *SAMPLE_VPSS_DumpChn(SC_VOID *pData)
{
    SAMPLE_WFILE_ThreadInfo *pInfo = (SAMPLE_WFILE_ThreadInfo *)pData;

    SC_CHAR szYuvName[128];
    SC_CHAR szPixFrm[10];
    SC_U32 u32Depth = 1;
    SC_S32 s32MilliSec = 200;
    SC_S32 s32Ret;
    SC_S32 s32Times = 10;
    VPSS_CHN_ATTR_S stChnAttr;
    VPSS_EXT_CHN_ATTR_S stExtChnAttr;
    VIDEO_FRAME_INFO_S stFrame;
    FILE *pfd = NULL;
    int frameid = 0;

    if (pInfo->VpssChn > VPSS_CHN3)
    {
        s32Ret = SC_MPI_VPSS_GetExtChnAttr(pInfo->VpssGrp, pInfo->VpssChn, &stExtChnAttr);
    }
    else
    {
        s32Ret = SC_MPI_VPSS_GetChnAttr(pInfo->VpssGrp, pInfo->VpssChn, &stChnAttr);
    }

    if (s32Ret != SC_SUCCESS)
    {
        printf("get chn attr error!!!\n");
        return NULL;
    }

    if (pInfo->VpssChn > VPSS_CHN3)
    {
        stExtChnAttr.u32Depth = u32Depth;
        s32Ret = SC_MPI_VPSS_SetExtChnAttr(pInfo->VpssGrp, pInfo->VpssChn, &stExtChnAttr);
    }
    else
    {
        stChnAttr.u32Depth = u32Depth;
        s32Ret = SC_MPI_VPSS_SetChnAttr(pInfo->VpssGrp, pInfo->VpssChn, &stChnAttr) ;
    }

    if (s32Ret != SC_SUCCESS)
    {
        printf("set depth error!!!\n");
        VPSS_Restore(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, NULL);
        return NULL;
    }

    memset(&stFrame, 0, sizeof(stFrame));
    stFrame.u32PoolId = VB_INVALID_POOLID;

    while (SC_MPI_VPSS_GetChnFrame(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, s32MilliSec) != SC_SUCCESS)
    {
        s32Times--;

        if (0 >= s32Times)
        {
            printf("get frame error for 10 times,now exit !!!\n");
            VPSS_Restore(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, NULL);
            return NULL;
        }

        usleep(40000);
    }

    if (VIDEO_FORMAT_LINEAR != stFrame.stVFrame.enVideoFormat)
    {
        printf("only support linear frame dump!\n");
        SC_MPI_VPSS_ReleaseChnFrame(pInfo->VpssGrp, pInfo->VpssChn, &stFrame);
        stFrame.u32PoolId = VB_INVALID_POOLID;
        return NULL;
    }

    switch (stFrame.stVFrame.enPixelFormat)
    {
    case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
    case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        snprintf(szPixFrm, 10, "P420");
        break;

    case PIXEL_FORMAT_YVU_SEMIPLANAR_422:
    case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
        snprintf(szPixFrm, 10, "P422");
        break;

    default:
        snprintf(szPixFrm, 10, "P420");
        break;
    }

    /* make file name */
    snprintf(szYuvName, 128, "./vpss_grp%d_chn%d_%dx%d_%s_%u.yuv", pInfo->VpssGrp, pInfo->VpssChn,
        stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height, szPixFrm, pInfo->FrameCnt);
    printf("Dump YUV frame of vpss chn %d  to file: \"%s\"\n", pInfo->VpssChn, szYuvName);
    fflush(stdout);

    s32Ret = SC_MPI_VPSS_ReleaseChnFrame(pInfo->VpssGrp, pInfo->VpssChn, &stFrame);

    if (SC_SUCCESS != s32Ret)
    {
        printf("Release frame error ,now exit !!!\n");
        VPSS_Restore(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, NULL);
        return NULL;
    }

    stFrame.u32PoolId = VB_INVALID_POOLID;
    /* open file */
    pfd = fopen(szYuvName, "wb");

    if (SC_NULL == pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));
        VPSS_Restore(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, NULL);
        return NULL;
    }

    /* get frame  */
    while ((pInfo->bQuit != SC_TRUE) && (pInfo->FrameCnt-- != 0))
    {
        if (SC_MPI_VPSS_GetChnFrame(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, s32MilliSec) != SC_SUCCESS)
        {
            printf("Get frame fail\n");
            usleep(1000);
            continue;
        }
        printf("u32Width:%d u32Height:%d u32Stride:%d-%d u64PTS:%llu\n",
            stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height,
            stFrame.stVFrame.u32Stride[0], stFrame.stVFrame.u32Stride[1], stFrame.stVFrame.u64PTS);

        if (DYNAMIC_RANGE_SDR8 == stFrame.stVFrame.enDynamicRange)
        {
            sample_yuv_8bit_dump(&stFrame.stVFrame, pfd);
        }
        else
        {
            SC_MPI_VPSS_ReleaseChnFrame(pInfo->VpssGrp, pInfo->VpssChn, &stFrame);
            printf("frame enDynamicRange error, now exit !!!\n");
            VPSS_Restore(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, pfd);
            return NULL;
        }

        printf("Get frame 0x%x!!\n", pInfo->FrameCnt);
        /* release frame after using */
        s32Ret = SC_MPI_VPSS_ReleaseChnFrame(pInfo->VpssGrp, pInfo->VpssChn, &stFrame);

        if (SC_SUCCESS != s32Ret)
        {
            printf("Release frame error ,now exit !!!\n");
            VPSS_Restore(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, pfd);
            return NULL;
        }

        stFrame.u32PoolId = VB_INVALID_POOLID;

        printf("get frameid[%d][%d]:%d\n", pInfo->VpssGrp, pInfo->VpssChn, frameid++);

    }

    VPSS_Restore(pInfo->VpssGrp, pInfo->VpssChn, &stFrame, pfd);

    printf("\n\n\n---------------SAMPLE_VPSS_DumpChn[%d][%d] to exit---------------\n\n\n", pInfo->VpssGrp, pInfo->VpssChn);

    return NULL;
}

SC_VOID SAMPLE_WFILE_StartThd(SAMPLE_WFILE_ThreadInfo *pstThreadInfo,
    VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    /* CREATE USER THREAD */
    SAMPLE_WFILE_GetThreadInfo(pstThreadInfo, VpssGrp, VpssChn);

    if (-1 == VpssChn)
    {
        pthread_create(&pstThreadInfo->tid, NULL, SAMPLE_VPSS_DumpGrp, (SC_VOID *)pstThreadInfo);
    }
    else
    {
        pthread_create(&pstThreadInfo->tid, NULL, SAMPLE_VPSS_DumpChn, (SC_VOID *)pstThreadInfo);
    }

    return;
}

SC_VOID SAMPLE_FILE_StopThd(SAMPLE_RFILE_ThreadInfo *pRInfo,
    SAMPLE_WFILE_ThreadInfo *pWInfo)
{
    if (pRInfo)
    {
        pRInfo->bQuit = SC_TRUE;
        pRInfo->bDestroy = SC_TRUE;
        //pthread_join(pRInfo->tid,SC_NULL);
    }

    if (pWInfo)
    {
        pWInfo->bQuit = SC_TRUE;
        pWInfo->bDestroy = SC_TRUE;
        //pthread_join(pWInfo->tid,SC_NULL);
    }
    return;
}

/******************************************************************************
* function :  FILE + VPSS + FILE (1080p@30fps + 720p@30fps)
******************************************************************************/
SC_S32 SAMPLE_FILE_VPSS_FILE(void)
{
    SC_S32 i, s32Ret;
    SAMPLE_RFILE_ThreadInfo rFileThd;
    SAMPLE_WFILE_ThreadInfo wChnFileThd[2];
    SAMPLE_WFILE_ThreadInfo wGrpFileThd;

    SC_CHAR rFileName[256]  = YUV_1920_1080;
    SIZE_S  rFrameSize      = {1920, 1080};

    SIZE_S  wFrameSize[2]      = {{1920, 1080}, {1280, 720}};
    VPSS_GRP VpssGrp        =  0;
    VPSS_CHN VpssChn[2]     = {0, 1};
    SC_BOOL  abChnEnable[4] = {1, 1, 0, 0};
    ROTATION_E enRotation[4] = {0, 0, 0, 0};
    DYNAMIC_RANGE_E enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
    VPSS_CROP_INFO_S stCropInfo;
    SC_U32 TestAlign = 256;

    s32Ret = SAMPLE_FILE_SYS_Init(enRotation[0], TestAlign);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_FILE_SYS_Init failed with %#x!\n", s32Ret);
        goto EXIT_SYS_STOP;
    }

    stVpssGrpAttr.enDynamicRange = enDynamicRange;
    stVpssGrpAttr.enPixelFormat  = enPixelFormat;
    stVpssGrpAttr.u32MaxW        = rFrameSize.u32Width;
    stVpssGrpAttr.u32MaxH        = rFrameSize.u32Height;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.bNrEn = SC_TRUE;

    s32Ret = SC_MPI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        goto EXIT_SYS_STOP;
    }

    #if 0
    stCropInfo.bEnable = SC_TRUE;
    stCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
    stCropInfo.stCropRect.s32X = 960;
    stCropInfo.stCropRect.s32Y = 0;
    stCropInfo.stCropRect.u32Width = 960;
    stCropInfo.stCropRect.u32Height = 540;

    s32Ret = SC_MPI_VPSS_SetGrpCrop(VpssGrp, &stCropInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VPSS_SetGrpCrop failed with %#x\n", s32Ret);
        goto EXIT_SYS_STOP;
    }

    s32Ret = SC_MPI_VPSS_GetGrpCrop(VpssGrp, &stCropInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VPSS_GetGrpCrop failed with %#x\n", s32Ret);
    }
    #endif

    s32Ret = SC_MPI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        goto EXIT_SYS_STOP;
    }

    for(i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if(SC_TRUE == abChnEnable[i])
        {
            if ((ROTATION_90 == enRotation[i])
                || (ROTATION_270 == enRotation[i]))
            {
                stVpssChnAttr[i].u32Width                 = wFrameSize[i].u32Height;
                stVpssChnAttr[i].u32Height                = wFrameSize[i].u32Width;
            }
            else
            {
                stVpssChnAttr[i].u32Width                 = wFrameSize[i].u32Width;
                stVpssChnAttr[i].u32Height                = wFrameSize[i].u32Height;
            }
            stVpssChnAttr[i].enChnMode                    = VPSS_CHN_MODE_USER;
            stVpssChnAttr[i].enCompressMode               = COMPRESS_MODE_NONE;
            stVpssChnAttr[i].enDynamicRange               = enDynamicRange;
            stVpssChnAttr[i].enPixelFormat                = enPixelFormat;
            stVpssChnAttr[i].stFrameRate.s32SrcFrameRate  = -1;
            stVpssChnAttr[i].stFrameRate.s32DstFrameRate  = -1;
            stVpssChnAttr[i].u32Depth                     = 0;
            stVpssChnAttr[i].bMirror                      = SC_FALSE;
            stVpssChnAttr[i].bFlip                        = SC_FALSE;
            stVpssChnAttr[i].enVideoFormat                = VIDEO_FORMAT_LINEAR;
            stVpssChnAttr[i].stAspectRatio.enMode         = ASPECT_RATIO_NONE;

            s32Ret = SC_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn[i], &stVpssChnAttr[i]);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SC_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
                goto EXIT_SYS_STOP;
            }

            s32Ret = SC_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn[i], &stVpssChnAttr[i]);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SC_MPI_VPSS_GetChnAttr failed with %#x\n", s32Ret);
            }

            #if 0
            stCropInfo.bEnable = SC_TRUE;
            stCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
            stCropInfo.stCropRect.s32X = 960;
            stCropInfo.stCropRect.s32Y = 0;
            stCropInfo.stCropRect.u32Width = 960;
            stCropInfo.stCropRect.u32Height = 540;

            s32Ret = SC_MPI_VPSS_SetChnCrop(VpssGrp, VpssChn[i], &stCropInfo);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SC_MPI_VPSS_SetChnCrop failed with %#x\n", s32Ret);
                goto EXIT_SYS_STOP;
            }

            s32Ret = SC_MPI_VPSS_GetChnCrop(VpssGrp, VpssChn[i], &stCropInfo);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SC_MPI_VPSS_GetChnCrop failed with %#x\n", s32Ret);
            }
            #endif

            if ((ROTATION_90 == enRotation[i])
                || (ROTATION_270 == enRotation[i]))
            {
                s32Ret = SC_MPI_VPSS_SetChnRotation(VpssGrp, VpssChn[i], enRotation[i]);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SC_MPI_VPSS_SetChnRotation failed with %#x\n", s32Ret);
                    goto EXIT_SYS_STOP;
                }

                s32Ret = SC_MPI_VPSS_GetChnRotation(VpssGrp, VpssChn[i], &enRotation[i]);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SC_MPI_VPSS_GetChnRotation failed with %#x\n", s32Ret);
                }
            }

#if 1
            SC_U32 u32Align = DEFAULT_ALIGN;
            if (!i)
            {
                u32Align = TestAlign;
            }
            s32Ret = SC_MPI_VPSS_SetChnAlign(VpssGrp, VpssChn[i], u32Align);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SC_MPI_VPSS_SetChnAlign failed with %#x\n", s32Ret);
            }
#endif
            s32Ret = SC_MPI_VPSS_EnableChn(VpssGrp, VpssChn[i]);
            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SC_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
                goto EXIT_SYS_STOP;
            }
        }
    }

    /* START READ FILE THREAD */
    SAMPLE_RFILE_StartThd(&rFileThd, rFileName, &rFrameSize);

    for (i = 0; i < 2; i++)
    {
        if (abChnEnable[i])
        {
            /* START WRITE FILE THREAD i */
            SAMPLE_WFILE_StartThd(&wChnFileThd[i], VpssGrp, VpssChn[i]);
        }
    }

    #if 1
    SAMPLE_WFILE_StartThd(&wGrpFileThd, VpssGrp, -1);
    #endif

    while (!all_exit)
    {
        usleep(10000);
    }

    printf("\n\n\n---------------SAMPLE_FILE_VPSS_FILE to exit---------------\n\n\n");

    /* STOP READ/WRITE FILE THREAD */
    SAMPLE_FILE_StopThd(&rFileThd, NULL);

    SAMPLE_FILE_StopThd(NULL, &wGrpFileThd);

    for (i = 0; i < 2; i++)
    {
        if (abChnEnable[i])
        {
            /* STOP READ/WRITE FILE THREAD */
            SAMPLE_FILE_StopThd(NULL, &wChnFileThd[i]);
        }
    }

    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT_SYS_STOP:
    SAMPLE_SYS_Exit();

    return SC_SUCCESS;
}

static SC_S32 SAMPLE_VPSS_SetGrpAttr(VPSS_GRP_ATTR_S *pstGrpAttr)
{
    VPSS_GRP_ATTR_S stGrpAttr;

    SC_S32 ret = SC_MPI_VPSS_GetGrpAttr(Dy_VpssGrp, &stGrpAttr);
    if (ret)
    {
        printf("SC_MPI_VPSS_GetGrpAttr(%d) error, ret:%d\n", Dy_VpssGrp, ret);
        return SC_FAILURE;
    }

    stGrpAttr.stFrameRate.s32SrcFrameRate = pstGrpAttr->stFrameRate.s32SrcFrameRate;
    stGrpAttr.stFrameRate.s32DstFrameRate = pstGrpAttr->stFrameRate.s32DstFrameRate;
    stGrpAttr.bNrEn = pstGrpAttr->bNrEn;

    printf("VPSS_SetGrpAttr->stFrameRate:%d-%d bNrEn:%d\n",
        stGrpAttr.stFrameRate.s32SrcFrameRate, stGrpAttr.stFrameRate.s32DstFrameRate, stGrpAttr.bNrEn);

    ret = SC_MPI_VPSS_SetGrpAttr(Dy_VpssGrp, &stGrpAttr);
    if (ret)
    {
        printf("SC_MPI_VPSS_SetGrpAttr(%d) error, ret:%d\n", Dy_VpssGrp, ret);
    }

    return ret;
}

static SC_S32 SAMPLE_VPSS_SetChnLDCAttr(VPSS_LDC_ATTR_S *pstLDCAttr)
{
    printf("VPSS_SetChnAttr->bEnable:%d k(%f,%f,%f,%f,%f,%f,%f,%f,%f)\
 ldc_k0:%f ldc_k1:%f ldc_k2:%f\n",
        pstLDCAttr->bEnable,
        pstLDCAttr->stAttr.k[0], pstLDCAttr->stAttr.k[1],pstLDCAttr->stAttr.k[2],
        pstLDCAttr->stAttr.k[3], pstLDCAttr->stAttr.k[4], pstLDCAttr->stAttr.k[5],
        pstLDCAttr->stAttr.k[6], pstLDCAttr->stAttr.k[7], pstLDCAttr->stAttr.k[8],
        pstLDCAttr->stAttr.ldc_k0, pstLDCAttr->stAttr.ldc_k1, pstLDCAttr->stAttr.ldc_k2);

    SC_S32 ret = SC_MPI_VPSS_SetChnLDCAttr(Dy_VpssGrp, Dy_VpssChn, pstLDCAttr);
    if (ret)
    {
        printf("SC_MPI_VPSS_SetChnLDCAttr(%d-%d) error, ret:%d\n", Dy_VpssGrp, Dy_VpssChn, ret);
    }

    return ret;
}

static SC_S32 SAMPLE_VPSS_SetChnAttr(VPSS_CHN_ATTR_S *pstChnAttr)
{
    VPSS_CHN_ATTR_S stChnAttr;

    SC_S32 ret = SC_MPI_VPSS_GetChnAttr(Dy_VpssGrp, Dy_VpssChn, &stChnAttr);
    if (ret)
    {
        printf("SC_MPI_VPSS_GetChnAttr(%d-%d) error, ret:%d\n", Dy_VpssGrp, Dy_VpssChn, ret);
        return SC_FAILURE;
    }

    stChnAttr.enChnMode = pstChnAttr->enChnMode;
    stChnAttr.stFrameRate.s32SrcFrameRate = pstChnAttr->stFrameRate.s32SrcFrameRate;
    stChnAttr.stFrameRate.s32DstFrameRate = pstChnAttr->stFrameRate.s32DstFrameRate;
    stChnAttr.bMirror = pstChnAttr->bMirror;
    stChnAttr.bFlip = pstChnAttr->bFlip;
    stChnAttr.u32Depth = pstChnAttr->u32Depth;

    printf("VPSS_SetChnAttr->enChnMode:%d stFrameRate:%d-%d bMirror:%d bFlip:%d u32Depth:%d\n",
        pstChnAttr->enChnMode, pstChnAttr->stFrameRate.s32SrcFrameRate, pstChnAttr->stFrameRate.s32DstFrameRate,
        pstChnAttr->bMirror, pstChnAttr->bFlip, pstChnAttr->u32Depth);

    ret = SC_MPI_VPSS_SetChnAttr(Dy_VpssGrp, Dy_VpssChn, &stChnAttr);
    if (ret)
    {
        printf("SC_MPI_VPSS_SetChnAttr(%d-%d) error, ret:%d\n", Dy_VpssGrp, Dy_VpssChn, ret);
    }

    return ret;
}

static SC_S32 SAMPLE_VPSS_UserFrameRateCtrl(SC_BOOL enable)
{
    SC_S32 ret;

    if (!enable)
    {
        ret = SC_MPI_VPSS_DisableUserFrameRateCtrl(Dy_VpssGrp);
    }
    else
    {
        ret = SC_MPI_VPSS_EnableUserFrameRateCtrl(Dy_VpssGrp);
    }

    if (ret)
    {
        printf("UserFrameRateCtrl(%d) enable(%d) error, ret:%d\n", Dy_VpssGrp, enable, ret);
    }

    printf("UserFrameRateCtrl(%d)->enable:%d\n", Dy_VpssGrp, enable);

    return ret;
}

static SC_S32 SAMPLE_VPSS_GetRegionLuma(VIDEO_REGION_INFO_S *pstRegionInfo)
{
    SC_U64 u64LumaData[128];
    SC_S32 s32MilliSec = 1000;

    memset(u64LumaData, 0, sizeof(u64LumaData));

    SC_S32 ret = SC_MPI_VPSS_GetRegionLuma(Dy_VpssGrp, Dy_VpssChn, pstRegionInfo, u64LumaData, s32MilliSec);
    if (ret)
    {
        printf("SC_MPI_VPSS_GetRegionLuma(%d-%d) error, ret:%d\n", Dy_VpssGrp, Dy_VpssChn, ret);
    }

    #if 1
    SC_S32 i;
    printf("GetRegionLuma(%d-%d)->u32RegionNum:%d\n", Dy_VpssGrp, Dy_VpssChn, pstRegionInfo->u32RegionNum);
    for (i = 0; i < pstRegionInfo->u32RegionNum; i++)
    {
        printf("pstRegion[%d]:%d-%d-%d-%d pu64LumaData[%d]:0x%llx\n",
            i, pstRegionInfo->pstRegion[i].s32X, pstRegionInfo->pstRegion[i].s32Y,
            pstRegionInfo->pstRegion[i].u32Width, pstRegionInfo->pstRegion[i].u32Height,
            i, u64LumaData[i]);
    }
    #endif

    return ret;
}

static SC_S32 SAMPLE_VPSS_TestDumpChn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
    SC_S32 depth, SC_S32 allcnt)
{
    SC_S32 index;
    SC_S32 i, ret, count = 0;
    SC_S32 max_err = 0;
    SC_S32 max_errcnt = 10;
    VIDEO_FRAME_INFO_S sframe[128];
    VPSS_CHN_ATTR_S stChnAttr;
    VPSS_EXT_CHN_ATTR_S stExtChnAttr;

    if (depth >= 128)
    {
        printf("TestDumpChn depth(%d) >= 128 error!\n", depth);
        return SC_SUCCESS;
    }

    if (VpssChn > VPSS_CHN3)
    {
        ret = SC_MPI_VPSS_GetExtChnAttr(VpssGrp, VpssChn, &stExtChnAttr);
    }
    else
    {
        ret = SC_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn, &stChnAttr);
    }

    if (ret != SC_SUCCESS)
    {
        printf("get chn attr error!!!\n");
        return SC_FAILURE;
    }

    if (VpssChn > VPSS_CHN3)
    {
        stExtChnAttr.u32Depth = depth;
        ret = SC_MPI_VPSS_SetExtChnAttr(VpssGrp, VpssChn, &stExtChnAttr);
    }
    else
    {
        stChnAttr.u32Depth = depth;
        ret = SC_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
    }

    if (ret != SC_SUCCESS)
    {
        printf("set chn attr error!!!\n");
        return SC_FAILURE;
    }

    memset(sframe, 0, sizeof(sframe));

    while ((count < depth) && (max_err < max_errcnt))
    {
        ret = SC_MPI_VPSS_GetChnFrame(VpssGrp, VpssChn, &sframe[count], -1);
        if (!ret)
        {
            max_err = 0;
            count++;
            printf("SC_SAPI_VPSS_GetChnFrame(%d-%d) count(%d) ok\n",
                VpssGrp, VpssChn, count);
        }
        else
        {
            max_err++;
        }
    }

    if (count < depth)
    {
        for (i = 0; i < count; i++)
        {
            SC_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &sframe[i]);
        }

        printf("SAMPLE_VPSS_TestDumpChn(%d-%d) error, count(%d) depth(%d) max_err(%d)\n",
            VpssGrp, VpssChn, count, depth, max_err);
        return SC_FAILURE;
    }

    max_err = 0;
    while ((count < allcnt) && (max_err < max_errcnt))
    {
        index = ((count + 1) % depth);
        SC_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &sframe[index]);

        ret = SC_MPI_VPSS_GetChnFrame(VpssGrp, VpssChn, &sframe[index], -1);
        if (!ret)
        {
            max_err = 0;
            count++;
            printf("SC_SAPI_VPSS_GetChnFrame(%d-%d) count(%d) ok\n",
                VpssGrp, VpssChn, count);
        }
        else
        {
            max_err++;
        }
    }

    for (i = 0; i < depth; i++)
    {
        SC_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &sframe[i]);
    }

    printf("SAMPLE_VPSS_TestDumpChn(%d-%d) ok, count(%d) allcnt(%d) depth(%d) max_err(%d)\n",
        VpssGrp, VpssChn, count, allcnt, depth, max_err);

    return SC_SUCCESS;
}

/******************************************************************************
* function : dynamic thread show usage
******************************************************************************/
void SAMPLE_Thr_Usage(char *sPrgNm)
{
    printf("Usage : %s \n", sPrgNm);

    printf("\t-g | --grpattr 25 25 0\n"
        "\tsrcframerate:0-30\n"
        "\tdstframerate:0-30\n"
        "\tNren:0-disable 1-enable\n");

    printf("\t-c | --chnattr: 0 25 25 0 0 2\n"
        "\tmode:0-user 1-auto\n"
        "\tsrcframerate:0-30\n"
        "\tdstframerate:0-30\n"
        "\tmirror:0-disable 1-enable\n"
        "\tflip:0-disable 1-enable\n"
        "\tdepth:0-8\n");

    printf("\t-l | --ldcattr: 0 0 0 0 0 0 0 0 0 0 0 0\n"
            "\tk0\n"
            "\tk1\n"
            "\tk2\n"
            "\tk3\n"
            "\tk4\n"
            "\tk5\n"
            "\tk6\n"
            "\tk7\n"
            "\tk8\n"
            "\tldc_k0\n"
            "\tldc_k1\n"
            "\tldc_k2\n");

    printf("\t-u | --userfrmrate: 0\n"
        "\tenable:0-disable 1-enable\n");

    printf("\t-y | --getluma: 1 64 64 128 128\n"
        "\tnum:0-128\n"
        "\trect:x y width height\n");

    printf("\t-d | --dumpchn: 0 0 5 100\n"
        "\tgrp:0-32\n"
        "\tchn:0-8\n"
        "\tdepth:0-16\n"
        "\tcnt:dump frame num\n");

    printf("\t-e | --exit  main to exit!\n");

    return;
}

static SC_CHAR optstr[] = "?::g:c:l:u:y:d:e";
static const struct option long_options[] =
{
    {"grpattr",     required_argument, NULL, 'g'},
    {"chnattr",     required_argument, NULL, 'c'},
    {"ldcattr",     required_argument, NULL, 'l'},
    {"userfrmrate", required_argument, NULL, 'u'},
    {"getluma",     required_argument, NULL, 'y'},
    {"dumpchn",     required_argument, NULL, 'd'},
    {"exit",        no_argument,       NULL, 'e'},
    {"help",        optional_argument, NULL, '?'},
    {NULL,          0,                 NULL,   0},
};

#define SAMPLE_VPSS_CheckIndex(index) \
if ((index >= cnt) || (NULL == pstrbuf[index])) \
{ \
    printf("[Func]:%s [Line]:%d->cnt:%d pstrbuf[%d]:%p\n", \
        __FUNCTION__, __LINE__, cnt, index, pstrbuf[index]); \
    break; \
}

SC_VOID *SAMPLE_VPSS_DynamicPara(SC_VOID *pArgs)
{
    int  i, c = 0;
    opterr = 1;
    int  cnt, index;
    char *ptr = NULL;
    char buffer[1024];
    char *pstrbuf[100];

    while (SC_TRUE != all_exit)
    {
        memset(buffer, 0, sizeof(buffer));
        gets(buffer);
        printf("buffer:%s\n", buffer);

        cnt = 0;
        pstrbuf[cnt++] = "./sample_vpss";
        ptr = strtok(buffer, " ");

        while(ptr != NULL)
        {
            pstrbuf[cnt++] = ptr;

            ptr = strtok(NULL, " ");
        }

        if (cnt < 2)
        {
            printf("ddd->cnt:%d\n", cnt);
            SAMPLE_Thr_Usage(buffer);
            continue;
        }

        optind = 1;
        while ((c = getopt_long(cnt, pstrbuf, optstr, long_options, NULL)) != -1)
        {
            printf("c:%c pstrbuf[%d]:%s\n", c, (optind - 1), pstrbuf[optind - 1]);
            switch (c)
            {
            case 'g':
            {
                VPSS_GRP_ATTR_S stGrpAttr;

                memset(&stGrpAttr, 0, sizeof(stGrpAttr));

                stGrpAttr.stFrameRate.s32SrcFrameRate = atoi(optarg);

                index = optind;
                SAMPLE_VPSS_CheckIndex(index);
                stGrpAttr.stFrameRate.s32DstFrameRate = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VPSS_CheckIndex(index);
                stGrpAttr.bNrEn = atoi(pstrbuf[index]);

                SAMPLE_VPSS_SetGrpAttr(&stGrpAttr);
            }
            break;
            case 'c':
            {
                VPSS_CHN_ATTR_S stChnAttr;

                memset(&stChnAttr, 0, sizeof(stChnAttr));

                stChnAttr.enChnMode = atoi(optarg);

                index = optind;
                SAMPLE_VPSS_CheckIndex(index);
                stChnAttr.stFrameRate.s32SrcFrameRate = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VPSS_CheckIndex(index);
                stChnAttr.stFrameRate.s32DstFrameRate = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VPSS_CheckIndex(index);
                stChnAttr.bMirror = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VPSS_CheckIndex(index);
                stChnAttr.bFlip = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VPSS_CheckIndex(index);
                stChnAttr.u32Depth = atoi(pstrbuf[index]);

                SAMPLE_VPSS_SetChnAttr(&stChnAttr);
            }
            break;
            case 'l':
            {
                    VPSS_LDC_ATTR_S stLDCAttr;
                    memset(&stLDCAttr, 0, sizeof(VPSS_LDC_ATTR_S));

                    stLDCAttr.bEnable = atoi(optarg);
                    index = optind;
                    SAMPLE_VPSS_CheckIndex(index);
                    stLDCAttr.stAttr.k[0] = atof(pstrbuf[index]);
                    for (size_t i = 1; i < 9; i++)
                    {
                        index++;
                        SAMPLE_VPSS_CheckIndex(index);
                        stLDCAttr.stAttr.k[i] = atof(pstrbuf[index]);
                    }
                    index++;
                    SAMPLE_VPSS_CheckIndex(index);
                    stLDCAttr.stAttr.ldc_k0 = atof(pstrbuf[index]);
                    index++;
                    SAMPLE_VPSS_CheckIndex(index);
                    stLDCAttr.stAttr.ldc_k1 = atof(pstrbuf[index]);
                    index++;
                    SAMPLE_VPSS_CheckIndex(index);
                    stLDCAttr.stAttr.ldc_k2 = atof(pstrbuf[index]);

                SAMPLE_VPSS_SetChnLDCAttr(&stLDCAttr);
            }
            break;
            case 'u':
            {
                SC_BOOL enable;

                enable = atoi(optarg);

                SAMPLE_VPSS_UserFrameRateCtrl(enable);
            }
            break;
            case 'y':
            {
                VIDEO_REGION_INFO_S stRegionInfo;
                RECT_S rect[128];
                SC_S32 count = 0;

                memset(&stRegionInfo, 0, sizeof(VIDEO_REGION_INFO_S));
                memset(rect, 0, sizeof(rect));

                stRegionInfo.u32RegionNum = atoi(optarg);
                stRegionInfo.pstRegion = rect;

                if ((stRegionInfo.u32RegionNum <= 0)
                    || (stRegionInfo.u32RegionNum > 128))
                {
                    printf("u32RegionNum[1-128] %d error\n", stRegionInfo.u32RegionNum);
                    break;
                }

                index = optind;
                for (i = 0; i < stRegionInfo.u32RegionNum; i++)
                {
                    SAMPLE_VPSS_CheckIndex(index);
                    stRegionInfo.pstRegion[i].s32X = atoi(pstrbuf[index]);

                    index++;
                    SAMPLE_VPSS_CheckIndex(index);
                    stRegionInfo.pstRegion[i].s32Y = atoi(pstrbuf[index]);

                    index++;
                    SAMPLE_VPSS_CheckIndex(index);
                    stRegionInfo.pstRegion[i].u32Width = atoi(pstrbuf[index]);

                    index++;
                    SAMPLE_VPSS_CheckIndex(index);
                    stRegionInfo.pstRegion[i].u32Height = atoi(pstrbuf[index]);

                    index++;
                    count++;
                }

                if (count != stRegionInfo.u32RegionNum)
                {
                    printf("u32RegionNum(%d) != count(%d)\n", stRegionInfo.u32RegionNum, count);
                    break;
                }

                SAMPLE_VPSS_GetRegionLuma(&stRegionInfo);

            }
            break;
            case 'd':
            {
                VPSS_GRP VpssGrp;
                VPSS_CHN VpssChn;
                SC_S32 depth, allcnt;

                VpssGrp = atoi(optarg);

                index = optind;
                SAMPLE_VPSS_CheckIndex(index);
                VpssChn = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VPSS_CheckIndex(index);
                depth = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VPSS_CheckIndex(index);
                allcnt = atoi(pstrbuf[index]);

                if (allcnt < depth)
                {
                    printf("param error, VpssGrp(%d) VpssChn(%d) depth(%d) allcnt(%d)\n",
                        VpssGrp, VpssChn, depth, allcnt);
                    break;
                }
                SAMPLE_VPSS_TestDumpChn(VpssGrp, VpssChn, depth, allcnt);
                printf("param set, VpssGrp(%d) VpssChn(%d) depth(%d) allcnt(%d)\n",
                    VpssGrp, VpssChn, depth, allcnt);
            }
            break;
            case 'e':
            {
                all_exit = SC_TRUE;
                printf("---------------main to exit, waitting---------------\n");
            }
            break;
            case '?':
            default:
            {
                SAMPLE_Thr_Usage(buffer);
            }
            break;
            }
        }

    }

    return NULL;
}

SC_VOID SAMPLE_VPSS_StartThread(SC_VOID)
{
    pthread_t Threadid = 0;
    pthread_create(&Threadid, 0, SAMPLE_VPSS_DynamicPara, NULL);
}

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VPSS_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);

    printf("index:\n"
        "\t 0) VI + VPSS + VENC (H.265@1080p@30fps + H264@720p@30fps)\n"
        "\t 1) VI + VPSS + VO (1080p@30fps)\n"
        "\t 2) FILE + VPSS + FILE (1080p@30fps + 720p@30fps)\n");

    printf("\t If you have any questions, please look at readme.txt!\n");
    return;
}

/******************************************************************************
* function    : main()
* Description : vpss sample
******************************************************************************/
int main(int argc, char *argv[])
{
    SC_S32 s32Ret;

    if (argc < 2 || argc > 2)
    {
        SAMPLE_VPSS_Usage(argv[0]);
        return SC_FAILURE;
    }

    if (!strncmp(argv[1], "-h", 2))
    {
        SAMPLE_VPSS_Usage(argv[0]);
        return SC_SUCCESS;
    }

    signal(SIGINT, SAMPLE_VPSS_HandleSig);
    signal(SIGTERM, SAMPLE_VPSS_HandleSig);

    SAMPLE_VPSS_StartThread();

    s32Index = atoi(argv[1]);
    switch (s32Index)
    {
    case 0:
        s32Ret = SAMPLE_VI_VPSS_VENC();
        break;
    case 1:
        s32Ret = SAMPLE_VI_VPSS_VO();
        break;
    case 2:
        s32Ret = SAMPLE_FILE_VPSS_FILE();
        break;
    default:
        printf("the mode is invaild!\n");
        SAMPLE_VPSS_Usage(argv[0]);
        return SC_FAILURE;
    }

    if (SC_SUCCESS == s32Ret)
    {
        printf("program exit normally!\n");
    }
    else
    {
        printf("program exit abnormally!\n");
    }

    exit(s32Ret);

}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
