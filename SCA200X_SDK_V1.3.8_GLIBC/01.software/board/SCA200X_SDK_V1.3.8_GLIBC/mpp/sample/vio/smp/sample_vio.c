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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>

#include "sample_comm.h"
#include "sc_comm_ive.h"
#include "sample_comm_ive.h"
#include "sample_comm_audio.h"
#include "mpi_vo.h"
#include "mpi_sys.h"

#include "../svp/common/sample_comm_npu.h"

#define  vi_chn_0_bmp  "./res/vi_chn_0.bmp"

#define  SSD_MODEL "./res/ssd-mobilenet_caffe.npubin"

#define SAMPLE_CHECK_RET(express,name)\
    do {\
        SC_S32 Ret;\
        Ret = express;\
        if (Ret != SC_SUCCESS) {\
            printf("%s failed at %s : LINE: %d with %#x!\n",name, __FUNCTION__,__LINE__,Ret);\
            return Ret;\
            }\
        }while(0)

extern SC_CHAR    *Path_BMP;

typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
}PointRect_S;

static void setLdc(int ViPipe, int ViChn)
{
    SC_S32  s32Ret;
    VI_LDC_ATTR_S ldcAttr  ={};

    /*set ldc*/
    ldcAttr.bEnable = SC_TRUE;
    ldcAttr.stAttr.k[0] = 1491.12;
    ldcAttr.stAttr.k[1] = 0;
    ldcAttr.stAttr.k[2] = 993.573;
    ldcAttr.stAttr.k[3] = 0;
    ldcAttr.stAttr.k[4] = 1484.24;
    ldcAttr.stAttr.k[5] = 519.34;
    ldcAttr.stAttr.k[6] = 0;
    ldcAttr.stAttr.k[7] = 0;
    ldcAttr.stAttr.k[8] = 1;
    ldcAttr.stAttr.ldc_k0 = -0.560604;
    ldcAttr.stAttr.ldc_k1 = 0.464791;
    ldcAttr.stAttr.ldc_k2 = -0.210018;
    s32Ret = SC_MPI_VI_SetChnLDCAttr(ViPipe, ViChn, &ldcAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_SetChnLDCAttr failed!\n");
    }

    return;
}

SC_VOID SAMPLE_VIO_MsgInit(SC_VOID)
{
}

SC_VOID SAMPLE_VIO_MsgExit(SC_VOID)
{
}

void SAMPLE_VIO_HandleSig(SC_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_VENC_StopGetStream();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

SC_S32 SAMPLE_VIO_ViOnlineVpssOfflineRoute(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    SC_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H264;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    SC_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;
    //stViConfig.astViInfo[s32WorkSnsId].stChnInfo.bMirror   = 1;
    //stViConfig.astViInfo[s32WorkSnsId].stChnInfo.bFlip  = 1;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 16;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

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
        goto EXIT;
    }

    /*config vpss*/
    memset_s(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = SC_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_NONE;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = SC_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = SC_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = SC_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config venc */
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn) / sizeof(VENC_CHN));
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT7;
    }

#if 0
    /*set ldc*/
    setLdc(ViPipe, ViChn);
#endif
    PAUSE();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT7:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViOfflineVpssOfflineRoute(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    SC_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H264;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    SC_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 6;

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
        goto EXIT;
    }

    /*config vpss*/
    memset_s(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = SC_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_NONE;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = SC_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = SC_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = SC_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config venc */
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn) / sizeof(VENC_CHN));
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT7;
    }

    PAUSE();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT7:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViDoubleChnRoute(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn[2]       = {0, 1};
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;
    VI_CHN_ATTR_S      stChnAttr;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    SC_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    SC_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn[0];
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

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
        goto EXIT;
    }

    /*config vpss*/
    memset_s(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = SC_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_NONE;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = SC_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = SC_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = SC_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn[0], VpssGrp);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*start vi chn1*/
    s32Ret = SC_MPI_VI_GetChnAttr(ViPipe, ViChn[0], &stChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_GetChnAttr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    stChnAttr.stSize.u32Width = stSize.u32Width / 2;
    stChnAttr.stSize.u32Height = stSize.u32Height / 2;

    s32Ret = SC_MPI_VI_SetChnAttr(ViPipe, ViChn[1], &stChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_GetChnAttr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SC_MPI_VI_EnableChn(ViPipe, ViChn[1]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_EnableChn failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    /*config venc */
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT5;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn[1], stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT7;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn) / sizeof(VENC_CHN));
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT8;
    }

    PAUSE();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT8:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn[1], stVoConfig.VoDev, VoChn);
EXIT7:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT6:
    SC_MPI_VI_DisableChn(ViPipe, ViChn[1]);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn[0], VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViRotate(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0; //1;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_U32             u32Align       = 0;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    //stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 1; //2;
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
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.u32Align        = u32Align;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.bMirror         = SC_FALSE;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.bFlip           = SC_FALSE;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /* get picture raw size */
    s32Ret = SAMPLE_COMM_VI_GetRawPicSize(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &stRawSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get raw picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stRawSize.u32Width, stRawSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP,
            COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 10;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    printf("Press Enter key to switch Rotation 90!\n");
    getchar();

    s32Ret = SC_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_90);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT3;
    }

    printf("Press Enter key to switch Rotation 180!\n");
    getchar();

    s32Ret = SC_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_180);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT3;
    }

    printf("Press Enter key to switch Rotation 270!\n");
    getchar();

    s32Ret = SC_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_270);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT3;
    }

    printf("Press Enter key to switch Rotation 0!\n");
    getchar();

    s32Ret = SC_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_0);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT3;
    }
    PAUSE();
EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViResoSwitch(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize, stChgSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    SC_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    SC_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;
    VENC_CHN_ATTR_S    stChnAttr;
    VENC_RECV_PIC_PARAM_S  stRecvParam;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

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
        goto EXIT;
    }

    /*config vpss*/
    memset_s(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = SC_FALSE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_NONE;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = SC_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = SC_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = SC_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config venc */
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SC_MPI_VENC_GetChnAttr(VencChn[0], &stChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get venc chn attr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn) / sizeof(VENC_CHN));
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT7;
    }

    printf("switch to 720P ========\n");
    getchar();

    stChgSize.u32Width             = 1280;
    stChgSize.u32Height            = 720;

    s32Ret = SAMPLE_COMM_VPSS_SetRes(VpssGrp, VpssChn, &stChgSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vpss set resolution failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    s32Ret = SC_MPI_VENC_StopRecvFrame(VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set venc chn stop failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    stChnAttr.stVencAttr.u32PicWidth     = 1280;
    stChnAttr.stVencAttr.u32PicHeight    = 720;
    s32Ret = SC_MPI_VENC_SetChnAttr(VencChn[0], &stChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set venc chn attr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    stRecvParam.s32RecvPicNum = -1;
    s32Ret = SC_MPI_VENC_StartRecvFrame(VencChn[0], &stRecvParam);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("venc chn start receive stream failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    printf("switch to 1080P ========\n");
    getchar();

    s32Ret = SAMPLE_COMM_VPSS_SetRes(VpssGrp, VpssChn, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vpss set resolution failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    s32Ret = SC_MPI_VENC_StopRecvFrame(VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set venc chn stop failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    stChnAttr.stVencAttr.u32PicWidth     = stSize.u32Width;
    stChnAttr.stVencAttr.u32PicHeight    = stSize.u32Height;
    s32Ret = SC_MPI_VENC_SetChnAttr(VencChn[0], &stChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set venc chn attr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    stRecvParam.s32RecvPicNum = -1;
    s32Ret = SC_MPI_VENC_StartRecvFrame(VencChn[0], &stRecvParam);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("venc chn start receive stream failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    PAUSE();

EXIT8:
    SAMPLE_COMM_VENC_StopGetStream();
EXIT7:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViDoubleWdrPipe(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 2;
    VI_DEV             ViDev[2]       = {0, 2};
    VI_PIPE            ViPipe[4]      = {0, 1, 2, 3};
    VI_CHN             ViChn          = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_2To1_LINE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp[2]     = {0, 1};
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    SC_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum = s32ViCnt;
    stViConfig.as32WorkingViId[0] = 0;
    stViConfig.as32WorkingViId[1] = 1;
    stViConfig.astViInfo[0].stSnsInfo.MipiDev         = ViDev[0];
    stViConfig.astViInfo[0].stSnsInfo.s32BusId        = 0;
    stViConfig.astViInfo[0].stDevInfo.ViDev           = ViDev[0];
    stViConfig.astViInfo[0].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[0].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[0]       = ViPipe[0];
    stViConfig.astViInfo[0].stPipeInfo.aPipe[1]       = ViPipe[1];
    stViConfig.astViInfo[0].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[0].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[0].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[0].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[0].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[0].stChnInfo.enCompressMode  = enCompressMode;

    stViConfig.astViInfo[1].stSnsInfo.MipiDev         = ViDev[1];
    stViConfig.astViInfo[1].stSnsInfo.s32BusId        = 1;
    stViConfig.astViInfo[1].stDevInfo.ViDev           = ViDev[1];
    stViConfig.astViInfo[1].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[1].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[1].stPipeInfo.aPipe[0]       = ViPipe[2];
    stViConfig.astViInfo[1].stPipeInfo.aPipe[1]       = ViPipe[3];
    stViConfig.astViInfo[1].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[1].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[1].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[1].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[1].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[1].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[1].stChnInfo.enCompressMode  = enCompressMode;

    if (stViConfig.astViInfo[0].stSnsInfo.enSnsType == SONY_IMX307_MIPI_2M_30FPS_12BIT)
    {
        SAMPLE_PRT("Not Support!\n");
        return SC_SUCCESS;
    }

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 20;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

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
        goto EXIT;
    }

    /*config vpss*/
    memset_s(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = SC_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_NONE;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = SC_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = SC_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = SC_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[0], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[1], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe[0], ViChn, VpssGrp[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe[2], ViChn, VpssGrp[1]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp[0], VpssChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    PAUSE();

    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp[0], VpssChn, stVoConfig.VoDev, VoChn);

EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe[2], ViChn, VpssGrp[1]);
EXIT4:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe[0], ViChn, VpssGrp[0]);
EXIT3:
    SAMPLE_COMM_VPSS_Stop(VpssGrp[1], abChnEnable);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp[0], abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_VioViChn1ResoSwitch(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 1;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    while(1)
    {
        memset(&stSize, 0, sizeof(stSize));
        printf("input resolution like:1280x720, 0x0 quit  ========\n");
        printf("input: ");
        scanf("%dx%d", &stSize.u32Width, &stSize.u32Height);
        printf("swith resolution to %dx%d  ========\n", stSize.u32Width, stSize.u32Height);
        if(0 == stSize.u32Width || 0 == stSize.u32Height)
        {
            printf("quit  ========\n");
            s32Ret = SC_SUCCESS;
            goto EXIT3;
        }

        s32Ret = SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("unbind vo failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }

#if 1
        s32Ret = SAMPLE_COMM_VI_StopVi(&stViConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("stop vi failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }
#else
        s32Ret = SAMPLE_COMM_VI_StopViChn(&stViConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("stop vi failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }
#endif

        stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stSize.u32Width = stSize.u32Width;
        stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stSize.u32Height = stSize.u32Height;

#if 1
        s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
            goto EXIT;
        }
#else
        s32Ret = SAMPLE_COMM_VI_StartViChn(&stViConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start vi failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }
#endif

        s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }
    }

    PAUSE();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViCrop(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 1;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    RECT_S             stRect;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VI_CROP_INFO_S     stCrop;

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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    while(1)
    {
        memset(&stRect, 0, sizeof(stRect));
        printf("input crop like:128 128 1280 720, other quit  ========\n");
        printf("input: ");
        scanf("%d %d %d %d", &stRect.s32X, &stRect.s32Y, &stRect.u32Width, &stRect.u32Height);
        printf("swith crop to %d %d %d %d ========\n",
            stRect.s32X, stRect.s32Y, stRect.u32Width, stRect.u32Height);
        if(0 == stRect.u32Width || 0 == stRect.u32Height)
        {
            printf("quit  ========\n");
            s32Ret = SC_SUCCESS;
            goto EXIT3;
        }

        stCrop.bEnable = SC_TRUE;
        stCrop.enCropCoordinate = VI_CROP_ABS_COOR;
        stCrop.stCropRect.s32X = stRect.s32X;
        stCrop.stCropRect.s32Y = stRect.s32Y;
        stCrop.stCropRect.u32Width = stRect.u32Width;
        stCrop.stCropRect.u32Height = stRect.u32Height;
        s32Ret = SC_MPI_VI_SetChnCrop(ViPipe, ViChn, &stCrop);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VI_SetChnCrop failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT3;
        }
    }

    PAUSE();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViRaw(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_U32             u32Align       = 64;

    VI_DUMP_ATTR_S     stDumpAttr;
    VIDEO_FRAME_INFO_S frame;
    SC_S32             cnt            = 0;

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
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.u32Align        = u32Align;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.bMirror        = SC_FALSE;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.bFlip        = SC_FALSE;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 10;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    printf("input get frame cnt  ========\n");
    printf("input: ");
    scanf("%d", &cnt);

    stDumpAttr.bEnable  = SC_TRUE;
    stDumpAttr.u32Depth = 2;
    stDumpAttr.enDumpType = VI_DUMP_TYPE_RAW;
    s32Ret = SC_MPI_VI_SetPipeDumpAttr(ViPipe, &stDumpAttr);
    if (SC_SUCCESS != s32Ret)
    {
        printf("Set Pipe %d dump attr failed!\n", ViPipe);
        goto EXIT3;
    }

    for(int i = 0; i < cnt; i++)
    {
        printf("get frame before\n");
        s32Ret = SC_MPI_VI_GetPipeFrame(ViPipe, &frame, -1);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("get frame failed.s32Ret:0x%x !\n", s32Ret);
            goto EXIT3;
        }
        printf("get frame after, ref:%d time:%lld \n",
            frame.stVFrame.u32TimeRef, frame.stVFrame.u64PTS);

        s32Ret = SC_MPI_VI_ReleasePipeFrame(ViPipe, &frame);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("release frame failed.s32Ret:0x%x !\n", s32Ret);
            goto EXIT3;
        }
    }

    stDumpAttr.bEnable  = SC_FALSE;
    stDumpAttr.u32Depth = 0;
    stDumpAttr.enDumpType = VI_DUMP_TYPE_RAW;
    s32Ret = SC_MPI_VI_SetPipeDumpAttr(ViPipe, &stDumpAttr);
    if (SC_SUCCESS != s32Ret)
    {
        printf("Set Pipe %d dump attr failed!\n", ViPipe);
        goto EXIT3;
    }

EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViOsd(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    SC_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;
    SC_S32             MinHandle;
    int                i;

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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 3;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    HandleNum = 8;
    enType = OVERLAY_RGN;
    Path_BMP = vi_chn_0_bmp;
    s32Ret = SAMPLE_COMM_REGION_Create(HandleNum, enType);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_Create failed!\n");
        goto EXIT3;
    }

    stChn.enModId = SC_ID_VI;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    s32Ret = SAMPLE_COMM_REGION_AttachToChn(HandleNum, enType, &stChn);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
        goto EXIT4;
    }

    MinHandle = SAMPLE_COMM_REGION_GetMinHandle(enType);

    if(OVERLAY_RGN == enType || OVERLAYEX_RGN == enType)
    {
        for(i = MinHandle; i < MinHandle + HandleNum; i++)
        {

            //s32Ret = SAMPLE_COMM_REGION_SetBitMap(i);
            s32Ret = SAMPLE_COMM_REGION_GetUpCanvas(i);
            if(SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_REGION_SetBitMap failed!\n");
                goto EXIT5;
            }
        }
    }

    PAUSE();

EXIT5:
    s32Ret = SAMPLE_COMM_REGION_DetachFrmChn(HandleNum, enType, &stChn);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
    }
EXIT4:
    s32Ret = SAMPLE_COMM_REGION_Destroy(HandleNum, enType);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
    }
EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViCover(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    SC_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 3;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    HandleNum = 8;
    enType = COVER_RGN;
    s32Ret = SAMPLE_COMM_REGION_Create(HandleNum, enType);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_Create failed!\n");
        goto EXIT3;
    }

    stChn.enModId = SC_ID_VI;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    s32Ret = SAMPLE_COMM_REGION_AttachToChn(HandleNum, enType, &stChn);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
        goto EXIT4;
    }

    PAUSE();

    s32Ret = SAMPLE_COMM_REGION_DetachFrmChn(HandleNum, enType, &stChn);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
    }
EXIT4:
    s32Ret = SAMPLE_COMM_REGION_Destroy(HandleNum, enType);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
    }
EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViMosaic(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    SC_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 3;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    HandleNum = 8;
    enType = MOSAIC_RGN;
    s32Ret = SAMPLE_COMM_REGION_Create(HandleNum, enType);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_Create failed!\n");
        goto EXIT3;
    }

    stChn.enModId = SC_ID_VI;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    s32Ret = SAMPLE_COMM_REGION_AttachToChn(HandleNum, enType, &stChn);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
        goto EXIT4;
    }

    PAUSE();

    s32Ret = SAMPLE_COMM_REGION_DetachFrmChn(HandleNum, enType, &stChn);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
    }
EXIT4:
    s32Ret = SAMPLE_COMM_REGION_Destroy(HandleNum, enType);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
    }
EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_Vi2Sensor(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 2;
#ifdef CONFIGSC_SCA200V200_BD1_DEV
    VI_DEV             ViDev[2]       = {0, 1};
#else
    VI_DEV             ViDev[2]       = {2, 3};
#endif
    VI_PIPE            ViPipe[2]      = {0, 1};

    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize, enPicSize0, enPicSize1;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn[4]       = {0, 1, 2, 3};
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    s32WorkSnsId = 0;
    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
#ifndef CONFIGSC_SCA200V200_BD1_DEV
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 1;
#endif
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    s32WorkSnsId = 1;
    stViConfig.as32WorkingViId[1]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
#ifndef CONFIGSC_SCA200V200_BD1_DEV
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 2;
#endif
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size for sensor 0*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize0);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    /*get picture size for sensor 1*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[1].stSnsInfo.enSnsType, &enPicSize1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("enPicSize0 = %d, enPicSize1 = %d \n", enPicSize0, enPicSize1);

    if(enPicSize0 >= enPicSize1)
    {
        enPicSize = enPicSize0;
    }
    else
    {
        enPicSize = enPicSize1;
    }
    SAMPLE_PRT("enPicSize =%d\n", enPicSize);

    /*get picture size*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("(w h)=>(%d %d)\n", stSize.u32Width, stSize.u32Height);

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 30;
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
        goto EXIT;
    }

    #if 0
    setLdc(ViPipe[0], ViChn);
    setLdc(ViPipe[1], ViChn);
    #endif

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enVoMode = VO_MODE_4MUX;
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }
#if 0
    /* Set second sensor monochrome */
    ISP_SATURATION_ATTR_S stSatAttr = {};
    stSatAttr.u32Saturation = 0;
    s32Ret = SC_MPI_ISP_SetSaturationAttr(ViPipe[1], &stSatAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Set saturation failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }
#endif
    PAUSE();

    SAMPLE_COMM_VI_UnBind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViHdrSwitch(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret         = SC_FALSE;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_2To1_LINE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    SC_S32             enableHdr      = 0;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    if(SONY_IMX307_MIPI_2M_30FPS_12BIT != stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType
        && SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1 != stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType
        && SMART_SC2310_MIPI_2M_30FPS_12BIT != stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType
        && SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1 != stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType
        && GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT != stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType
        && GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1 != stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType
        && SONY_IMX415_MIPI_8M_30FPS_12BIT != stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType
        && SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1 != stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
    {
        SAMPLE_PRT("not support, sensor:%d!\n", stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType);
        return s32Ret;
    }

    if (SONY_IMX307_MIPI_2M_30FPS_12BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
    {
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1;
    }

    if (SONY_IMX415_MIPI_8M_30FPS_12BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
    {
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1;
    }

    if (SMART_SC2310_MIPI_2M_30FPS_12BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
    {
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1;
    }

    if (GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
    {
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1;
    }

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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    while(1)
    {
        memset(&stSize, 0, sizeof(stSize));
        printf("input hdr enable:0/1, other quit  ========\n");
        printf("input: ");
        scanf("%d", &enableHdr);
        printf("swith hdr to %d  ========\n", enableHdr);
        if(0 != enableHdr && 1 != enableHdr)
        {
            printf("quit  ========\n");
            s32Ret = SC_SUCCESS;
            goto EXIT3;
        }

        s32Ret = SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("unbind vo failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }

        s32Ret = SAMPLE_COMM_VO_StopVO(&stVoConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("stop vo failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT1;
        }

        s32Ret = SAMPLE_COMM_VI_StopVi(&stViConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("stop vi failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }

        if(enableHdr)
        {
            enWDRMode = WDR_MODE_2To1_LINE;
            if (SONY_IMX307_MIPI_2M_30FPS_12BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
            {
                stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1;
            }

            if (SONY_IMX415_MIPI_8M_30FPS_12BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
            {
                stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1;
            }

            if (SMART_SC2310_MIPI_2M_30FPS_12BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
            {
                stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1;
            }

            if (GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
            {
                stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1;
            }
        }
        else
        {
            enWDRMode = WDR_MODE_NONE;
            if (SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1 == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
            {
                stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX307_MIPI_2M_30FPS_12BIT;
            }

            if (SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1 == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
            {
                stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX415_MIPI_8M_30FPS_12BIT;
            }

            if (SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1 == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
            {
                stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SMART_SC2310_MIPI_2M_30FPS_12BIT;
            }

            if (GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1 == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
            {
                stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT;
            }
        }
        stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;

        s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
            goto EXIT;
        }

        s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT1;
        }

        s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }
    }

    PAUSE();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_Vi2Chn(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn[4]       = {0, 1, 2, 3};
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 4;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width / 2, stSize.u32Height / 2, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[2].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt   = 3;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width / 2, stSize.u32Height / 2, PIXEL_FORMAT_RGB_BAYER_16BPP,
            COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[3].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[3].u32BlkCnt   = 3;

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
        goto EXIT;
    }

    VI_CHN_ATTR_S stChnAttr;
    VI_CHN ViChn1 = 1;

    /*start vi chn1*/
    s32Ret = SC_MPI_VI_GetChnAttr(ViPipe, ViChn, &stChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_GetChnAttr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT;
    }
    stChnAttr.stSize.u32Width = stSize.u32Width / 2;
    stChnAttr.stSize.u32Height = stSize.u32Height / 2;

    s32Ret = SC_MPI_VI_SetChnAttr(ViPipe, ViChn1, &stChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_GetChnAttr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT;
    }

    s32Ret = SC_MPI_VI_EnableChn(ViPipe, ViChn1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_EnableChn failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enVoMode = VO_MODE_4MUX;
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn[0]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn1, stVoConfig.VoDev, VoChn[1]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    PAUSE();

    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn[0]);
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn1, stVoConfig.VoDev, VoChn[1]);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    s32Ret = SC_MPI_VI_DisableChn(ViPipe, ViChn1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_EnableChn failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT;
    }

    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViOpenCloseRaw(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret = SC_SUCCESS;
    VI_PIPE            ViPipe         = 0;
    SC_S32             val            = 0;

    VI_DUMP_ATTR_S     stDumpAttr;
    VIDEO_FRAME_INFO_S frame;

    while(1)
    {
        memset(&val, 0, sizeof(val));
        printf("input pipe enable raw stream or not like: 0:1, 0:0, other quit  ========\n");
        printf("input: pipe");
        scanf("%d:%d", &ViPipe, &val);
        printf("pipe:%d enabled:%d   ========\n", ViPipe, val);
        if(0 != val && 1 != val)
        {
            printf("quit  ========\n");
            s32Ret = SC_SUCCESS;
            goto EXIT;
        }

        if(1 == val)
        {
            stDumpAttr.bEnable  = SC_TRUE;
            stDumpAttr.u32Depth = 2;
            stDumpAttr.enDumpType = VI_DUMP_TYPE_RAW;
        }
        else
        {
            stDumpAttr.bEnable  = SC_FALSE;
            stDumpAttr.u32Depth = 0;
            stDumpAttr.enDumpType = VI_DUMP_TYPE_RAW;
        }

        s32Ret = SC_MPI_VI_SetPipeDumpAttr(ViPipe, &stDumpAttr);
        if (SC_SUCCESS != s32Ret)
        {
            printf("Set Pipe %d dump attr failed!\n", ViPipe);
            goto EXIT;
        }

        if(1 == val)
        {
            printf("get frame before\n");
            s32Ret = SC_MPI_VI_GetPipeFrame(ViPipe, &frame, -1);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("get frame failed.s32Ret:0x%x !\n", s32Ret);
                goto EXIT;
            }
            printf("get frame after, ref:%d time:%lld \n",
                frame.stVFrame.u32TimeRef, frame.stVFrame.u64PTS);

            s32Ret = SC_MPI_VI_ReleasePipeFrame(ViPipe, &frame);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("release frame failed.s32Ret:0x%x !\n", s32Ret);
                goto EXIT;
            }
        }
    }

EXIT:
    return s32Ret;
}

typedef struct
{
    SC_U32 width;
    SC_U32 height;
    FRAME_RATE_CTRL_S stFps;
    SC_bool bBindVo;
    PIC_SIZE_E enVoPicSize;
    SC_U32 u32VoIntfType;
}Camera_param;
static SAMPLE_VI_CONFIG_S g_stViConfig;
static SAMPLE_VO_CONFIG_S g_stVoConfig;

static SC_S32 open_camera(int cnt, const Camera_param* pdev_cam)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = cnt;
#ifdef CONFIGSC_SCA200V200_BD1_DEV
    VI_DEV             ViDev[2]       = {0, 1};
#else
    VI_DEV             ViDev[2]       = {2, 3};
#endif
    VI_PIPE            ViPipe[2]      = {0, 1};

    VI_CHN             ViChn          = 1;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize, enPicSize0, enPicSize1;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn[4]       = {0, 1, 2, 3};
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    s32WorkSnsId = 0;
    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
#ifndef CONFIGSC_SCA200V200_BD1_DEV
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 3;
#endif
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stSize.u32Width = pdev_cam[0].width;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stSize.u32Height = pdev_cam[0].height;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stFps = pdev_cam[0].stFps;

    s32WorkSnsId = 1;
    stViConfig.as32WorkingViId[1]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
#ifndef CONFIGSC_SCA200V200_BD1_DEV
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 0;
#endif
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stSize.u32Width = pdev_cam[1].width;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stSize.u32Height = pdev_cam[1].height;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stFps = pdev_cam[1].stFps;

    /*get picture size for sensor 0*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize0);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    /*get picture size for sensor 1*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[1].stSnsInfo.enSnsType, &enPicSize1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("enPicSize0 = %d, enPicSize1 = %d \n", enPicSize0, enPicSize1);

    if(enPicSize0 >= enPicSize1)
    {
        enPicSize = enPicSize0;
    }
    else
    {
        enPicSize = enPicSize1;
    }
    SAMPLE_PRT("enPicSize =%d\n", enPicSize);

    /*get picture size*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("(w h)=>(%d %d)\n", stSize.u32Width, stSize.u32Height);

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 30;

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
        goto EXIT;
    }

    /* Set second sensor monochrome */
    ISP_SATURATION_ATTR_S stSatAttr = {};
    stSatAttr.u32Saturation = 0;
    s32Ret = SC_MPI_ISP_SetSaturationAttr(ViPipe[1], &stSatAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Set saturation failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    if(pdev_cam->bBindVo)
    {
        /*config vo*/
        SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
        stVoConfig.enVoMode = VO_MODE_4MUX;
        stVoConfig.enDstDynamicRange = enDynamicRange;
        if (1 == pdev_cam->u32VoIntfType)
        {
            stVoConfig.enVoIntfType = VO_INTF_BT1120;
        }
        else
        {
            stVoConfig.enVoIntfType = VO_INTF_HDMI;
        }
        stVoConfig.enPicSize = enPicSize;

        /*start vo*/
        s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT1;
        }

        /*vi bind vo*/
        s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
        s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }
    }

    g_stViConfig = stViConfig;
    g_stVoConfig = stVoConfig;
    return s32Ret;

EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

static void close_camera(const Camera_param* pdev_cam)
{
    VI_PIPE            ViPipe[2]      = {0,1};
    VI_CHN             ViChn          = 1;
    VO_CHN             VoChn[2]          = {0,1};

    if(pdev_cam->bBindVo)
    {
        SAMPLE_COMM_VI_UnBind_VO(ViPipe[0], ViChn, g_stVoConfig.VoDev, VoChn[0]);
        SAMPLE_COMM_VI_UnBind_VO(ViPipe[1], ViChn, g_stVoConfig.VoDev, VoChn[1]);
        SAMPLE_COMM_VO_StopVO(&g_stVoConfig);
    }

    SAMPLE_COMM_VI_StopVi(&g_stViConfig);
    SAMPLE_COMM_SYS_Exit();

    return;
}

static SC_S32 yv12_2_rgb(VIDEO_FRAME_INFO_S *pSrcFrm, IVE_IMAGE_S *pDstFrm)
{
    SC_S32 s32Ret;
    SC_S32 s32SrcWidth = pSrcFrm->stVFrame.u32Width;
    SC_S32 s32SrcHeight = pSrcFrm->stVFrame.u32Height;
    int i = 0;

    IVE_SRC_IMAGE_S stCSCIn = {0};
    stCSCIn.u32Width = s32SrcWidth;
    stCSCIn.u32Height = s32SrcHeight;
    stCSCIn.enType = IVE_IMAGE_TYPE_YUV420P;

    for(i = 0; i < 3; i++)
    {
        stCSCIn.au64PhyAddr[i] = pSrcFrm->stVFrame.u64PhyAddr[i];
        stCSCIn.au64VirAddr[i] = 0;
        stCSCIn.au32Stride[i] = pSrcFrm->stVFrame.u32Stride[i];
    }

    #if 0
    int len = (pSrcFrm->stVFrame.u32Stride[0])*(pSrcFrm->stVFrame.u32Height);
    stCSCIn.au64VirAddr[0] =(SC_VOID *)SC_MPI_SYS_Mmap(stCSCIn.au64PhyAddr[0], len);
    stCSCIn.au64VirAddr[1] =(SC_VOID *)SC_MPI_SYS_Mmap(stCSCIn.au64PhyAddr[1], len/4);
    stCSCIn.au64VirAddr[2] =(SC_VOID *)SC_MPI_SYS_Mmap(stCSCIn.au64PhyAddr[2], len/4);
    FILE *pSrcFile = fopen("./1920_1080_p420.yuv", "wb+");
    s32Ret = SAMPLE_COMM_IVE_WriteFile(&stCSCIn, pSrcFile);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_ReadFile err %d!\n", s32Ret);
    }
    fclose(pSrcFile);
    #endif

    IVE_IMAGE_S stCSCOut = {0};
    stCSCOut.u32Width = s32SrcWidth;
    stCSCOut.u32Height = s32SrcHeight;
    stCSCOut.enType = IVE_IMAGE_TYPE_U8C3_PLANAR;
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&stCSCOut, stCSCOut.enType, stCSCOut.u32Width, stCSCOut.u32Height);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_CreateImage err %d!\n", s32Ret);
        goto EXIT;
    }

    IVE_HANDLE hIve;
    IVE_CSC_CTRL_S stIveCscCtrl;
    stIveCscCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;
    SC_BOOL bInstance = SC_TRUE;
    s32Ret = SC_MPI_IVE_CSC(&hIve, &stCSCIn, &stCSCOut, &stIveCscCtrl, bInstance);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_IVE_csc err %d!\n", s32Ret);
        goto EXIT;
    }

    SC_BOOL bBlock = SC_TRUE;
    SC_BOOL bFinish;
    SC_S32 ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    while (SC_ERR_IVE_QUERY_TIMEOUT == ret)
    {
        usleep(100000);
        ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    }

    if(ret)
    {
        printf("SC_MPI_IVE_Query err %d!\n", s32Ret);
        goto EXIT;
    }

    #if 0
    SC_CHAR s8SrcName[64];
    sprintf(s8SrcName, "p420");
    SC_CHAR s8DstFileName[64] = {0};
    sprintf(s8DstFileName, "./IVE_csc_from_%s_%d_%d_to_rgb.rgb", s8SrcName, s32SrcWidth,
        s32SrcHeight);
    FILE *pDstFile = fopen(s8DstFileName, "wb+");
    s32Ret = SAMPLE_COMM_IVE_WriteFile(&stCSCOut, pDstFile);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_WriteFile err %d!\n", s32Ret);
    }
    fclose(pDstFile);

    printf("ive csc ok!\n");
    #endif

    *pDstFrm = stCSCOut;

EXIT:
    if(s32Ret && stCSCOut.au64PhyAddr[0])
    {
        IVE_MMZ_FREE(stCSCOut.au64PhyAddr[0], stCSCOut.au64VirAddr[0]);
    }
    return s32Ret;
}

static SC_S32 get_vi_frm_bgr(int pipe, IVE_IMAGE_S *pFrame, int ms)
{
    int chn = 1;
    SC_S32 s32Ret;
    VIDEO_FRAME_INFO_S stFrame;

    s32Ret = SC_MPI_VI_GetChnFrame(pipe, chn, &stFrame, ms);
    if(s32Ret)
    {
        SAMPLE_PRT("get frame failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT0;
    }

    s32Ret = yv12_2_rgb(&stFrame, pFrame);
    if(s32Ret)
    {
        SAMPLE_PRT("yv12_2_rgb failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

EXIT1:
    s32Ret = SC_MPI_VI_ReleaseChnFrame(pipe, chn, &stFrame);
    if(s32Ret)
    {
        SAMPLE_PRT("release frame failed. s32Ret: 0x%x !\n", s32Ret);
    }

EXIT0:
    return s32Ret;
}

static SC_VOID free_vi_frm_bgr(IVE_IMAGE_S *pFrame)
{
    if(pFrame->au64PhyAddr[0])
    {
        IVE_MMZ_FREE(pFrame->au64PhyAddr[0], pFrame->au64VirAddr[0]);
    }
}

SC_S32 SAMPLE_VIO_ViGetBgr(SC_U32 u32VoIntfType)
{
    SC_S32 s32Ret;
    VI_PIPE pipe[2] = {0, 1};
    VI_CHN camCnt = 2;
    int ms = -1;
    Camera_param stCamera[2] = {};
    IVE_IMAGE_S stFrm = {};

    stCamera[0].width = 1920;
    stCamera[0].height = 1080;
    stCamera[0].stFps.s32SrcFrameRate = 30;
    stCamera[0].stFps.s32DstFrameRate = 25;
    stCamera[0].bBindVo = 1;
    stCamera[0].enVoPicSize = PIC_1080P;
    stCamera[0].u32VoIntfType = u32VoIntfType;

    stCamera[1].width = 1280;
    stCamera[1].height = 720;
    stCamera[1].stFps.s32SrcFrameRate = 30;
    stCamera[1].stFps.s32DstFrameRate = 25;

    s32Ret = open_camera(camCnt, stCamera);
    if(s32Ret)
    {
        SAMPLE_PRT("open_camera failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT;
    }

    //while(1)
    {
        s32Ret = get_vi_frm_bgr(pipe[0], &stFrm, ms);
        if(s32Ret)
        {
            SAMPLE_PRT("get_vi_frm_bgr failed. s32Ret: 0x%x !\n", s32Ret);
        }

        //other function

        free_vi_frm_bgr(&stFrm);
    }

    PAUSE();

    close_camera(stCamera);

EXIT:
    return s32Ret;
}

static int addBoxByLineTask(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    const PointRect_S *pstRect, const VGS_DRAW_LINE_S *pstVgsDrawLine)
{
    SC_S32 s32Ret = -1;
    VGS_DRAW_LINE_S line = {};

    //printf("__ left:%d top:%d right:%d bottom:%d \n",
    //    pstRect->left, pstRect->top, pstRect->right, pstRect->bottom);

    line.u32Thick = pstVgsDrawLine->u32Thick;
    line.u32Color = pstVgsDrawLine->u32Color;

    line.stStartPoint.s32X = pstRect->left;
    line.stStartPoint.s32Y = pstRect->top;
    line.stEndPoint.s32X = pstRect->right;
    line.stEndPoint.s32Y = pstRect->top;
    s32Ret = SC_MPI_VGS_AddDrawLineTask(hHandle, pstTask, &line);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VGS_AddDrawLineTask failed, s32Ret:0x%x", s32Ret);
        goto EXIT;
    }

    line.stStartPoint.s32X = pstRect->left;
    line.stStartPoint.s32Y = pstRect->top;
    line.stEndPoint.s32X = pstRect->left;
    line.stEndPoint.s32Y = pstRect->bottom;
    s32Ret = SC_MPI_VGS_AddDrawLineTask(hHandle, pstTask, &line);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VGS_AddDrawLineTask failed, s32Ret:0x%x", s32Ret);
        goto EXIT;
    }

    line.stStartPoint.s32X = pstRect->right;
    line.stStartPoint.s32Y = pstRect->top;
    line.stEndPoint.s32X = pstRect->right;
    line.stEndPoint.s32Y = pstRect->bottom;
    s32Ret = SC_MPI_VGS_AddDrawLineTask(hHandle, pstTask, &line);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VGS_AddDrawLineTask failed, s32Ret:0x%x", s32Ret);
        goto EXIT;
    }

    line.stStartPoint.s32X = pstRect->left;
    line.stStartPoint.s32Y = pstRect->bottom;
    line.stEndPoint.s32X = pstRect->right;
    line.stEndPoint.s32Y = pstRect->bottom;
    s32Ret = SC_MPI_VGS_AddDrawLineTask(hHandle, pstTask, &line);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT( "SC_MPI_VGS_AddDrawLineTask failed, s32Ret:0x%x", s32Ret);
        goto EXIT;
    }

EXIT:
    return s32Ret;
}

static int drawBoxByVgs(VIDEO_FRAME_INFO_S *pFrm, PointRect_S *pRect, int cnt, int u32Color, int u32Thick)
{
    SC_S32 s32Ret = -1;
    int i = 0;
    int needCancelJob = 0;
    VGS_HANDLE hHandle = -1;
    VGS_TASK_ATTR_S stVgsTaskAttr;
    VGS_DRAW_LINE_S line;

    s32Ret = SC_MPI_VGS_BeginJob(&hHandle);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VGS_BeginJob failed, s32Ret:0x%x", s32Ret);
        goto EXIT;
    }
    needCancelJob = 1;

    stVgsTaskAttr.stImgIn = *pFrm;
    stVgsTaskAttr.stImgOut = *pFrm;
    line.u32Color = u32Color;
    line.u32Thick = u32Thick;
    for(i = 0; i < cnt; i++)
    {
        s32Ret = addBoxByLineTask(hHandle, &stVgsTaskAttr, &pRect[i], &line);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("addBoxByLineTask failed, s32Ret:0x%x", s32Ret);
            goto EXIT;
        }
    }

    s32Ret = SC_MPI_VGS_EndJob(hHandle);
    if (s32Ret != SC_SUCCESS)
    {
        SC_MPI_VGS_CancelJob(hHandle);
        SAMPLE_PRT("SC_MPI_VGS_EndJob failed, s32Ret:0x%x", s32Ret);
        goto EXIT;
    }

    needCancelJob = 0;

EXIT:
    if(needCancelJob)
    {
        SC_MPI_VGS_CancelJob(hHandle);
    }
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViAlgRect(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VO_LAYER           voLayer = 0;
    PointRect_S        rect[16]    = {};
    VIDEO_FRAME_INFO_S stFrameInfo ={};

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
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stFps.s32SrcFrameRate = 30;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.stFps.s32DstFrameRate = 15;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 3;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    SAMPLE_SVP_NPU_MODEL_S                   s_stSsdModel = {0};
    SAMPLE_SVP_NPU_PARAM_S                   s_stSsdNpuParam = {0};


    s_stSsdModel.stModelConf.enCbMode = SVP_NPU_NO_CB;
    s_stSsdModel.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;

    /*SSD Load model*/
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(SSD_MODEL, &s_stSsdModel);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("NPU load model err(%#x)\n", s32Ret);
        goto EXIT1;
    }

    /*SSD parameter initialization*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stSsdModel, &s_stSsdNpuParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("NPU param init err(%#x)\n", s32Ret);
        goto EXIT1;
    }

    #define SSD_WIDTH 300
    #define SSD_HEIGHT 300


    SC_VOID *pResult = SC_NULL;
    SC_S32   s32ResultNum;

    NPU_IMAGE_S ssd_image;
    ssd_image.u32InputNum = 1;
    ssd_image.astInputImg[0].u32BatchNum = 1;
    ssd_image.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
    ssd_image.astInputImg[0].stImages[0].u32Height = SSD_HEIGHT;
    ssd_image.astInputImg[0].stImages[0].u32Width = SSD_WIDTH;

    IVE_IMAGE_S stFrm = {};

    IVE_SRC_IMAGE_S resize_image = {0};
    resize_image.u32Width = SSD_WIDTH;
    resize_image.u32Height = SSD_HEIGHT;
    resize_image.enType = IVE_IMAGE_TYPE_U8C3_PLANAR;
    resize_image.au32Stride[0] =ALIGNED_256B(resize_image.u32Width);
    resize_image.au32Stride[1] =ALIGNED_256B(resize_image.u32Width);
    resize_image.au32Stride[2] =ALIGNED_256B(resize_image.u32Width);


    SC_U32 u32Size = resize_image.au32Stride[0] * resize_image.u32Height * 3;
    s32Ret = SC_MPI_SYS_MmzAlloc(&resize_image.au64PhyAddr[0], (SC_VOID **)&resize_image.au64VirAddr[0], NULL, SC_NULL, u32Size);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("Mmz Alloc fail,Error(%#x)\n", s32Ret);
        goto EXIT1;
    }
    resize_image.au32Stride[1] = resize_image.au32Stride[0];
    resize_image.au64VirAddr[1] = resize_image.au64VirAddr[0] + resize_image.au32Stride[0] * resize_image.u32Height;
    resize_image.au64PhyAddr[1] = resize_image.au64PhyAddr[0] + resize_image.au32Stride[0] * resize_image.u32Height;
    resize_image.au32Stride[2] = resize_image.au32Stride[1];
    resize_image.au64VirAddr[2] = resize_image.au64VirAddr[1] + resize_image.au32Stride[1] * resize_image.u32Height;
    resize_image.au64PhyAddr[2] = resize_image.au64PhyAddr[1] + resize_image.au32Stride[1] * resize_image.u32Height;


    while(1)
    {
        memset(&stFrameInfo, 0, sizeof(VIDEO_FRAME_INFO_S));
        s32Ret = SC_MPI_VI_GetChnFrame(ViPipe, ViChn, &stFrameInfo, -1);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VI_GetFrame errno = %#x \n", s32Ret);
            goto EXIT2;
        }

        s32Ret = yv12_2_rgb(&stFrameInfo, &stFrm);
        if(s32Ret)
        {
            SAMPLE_PRT("yv12_2_rgb failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT1;
        }

        IVE_HANDLE hIve;
        IVE_RESIZE_CTRL_S stIveCtrl;
        stIveCtrl.u16Num = 1;
        SC_BOOL bInstance = SC_TRUE;
        s32Ret = SC_MPI_IVE_Resize(&hIve, &stFrm, &resize_image, &stIveCtrl, bInstance);
        if(s32Ret != SC_SUCCESS)
        {
            printf("SC_MPI_IVE_csc err 0x%x!\n", s32Ret);
            continue;
        }

        SC_BOOL bBlock = SC_TRUE;
        SC_BOOL bFinish;
        SC_S32 ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
        while (SC_ERR_IVE_QUERY_TIMEOUT == ret)
        {
            usleep(100);
            ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
        }

        free_vi_frm_bgr(&stFrm);

        ssd_image.astInputImg[0].stImages[0].au64PhyAddr[0] = resize_image.au64PhyAddr[0];
        ssd_image.astInputImg[0].stImages[0].au64PhyAddr[1] = resize_image.au64PhyAddr[1];
        ssd_image.astInputImg[0].stImages[0].au64PhyAddr[2] = resize_image.au64PhyAddr[2];

        ssd_image.astInputImg[0].stImages[0].au64VirAddr[1] = resize_image.au64VirAddr[0];
        ssd_image.astInputImg[0].stImages[0].au64VirAddr[2] = resize_image.au64VirAddr[1];
        ssd_image.astInputImg[0].stImages[0].au64VirAddr[3] = resize_image.au64VirAddr[2];



        /*NPU process*/
       SVP_NPU_HANDLE hSvpNpuHandle = 0;
       s32Ret =  SC_MPI_SVP_NPU_Forward(&hSvpNpuHandle, &ssd_image, &s_stSsdNpuParam.stIntMem, &s_stSsdModel.stModel,
        &s_stSsdNpuParam.stOutMem, SC_TRUE);
        if(s32Ret != SC_SUCCESS)
        {
            printf("SC_MPI_SVP_NPU_Forward err 0x%x!\n", s32Ret);
            continue;
        }

       /*Software process and Get result*/
       SAMPLE_SVP_NPU_SSD_GetResult(s_stSsdModel.stModel.astDstTensor, s_stSsdModel.stModel.u16DstNum,
           (uintptr_t)s_stSsdNpuParam.stOutMem.u64VirAddr, &pResult, &s32ResultNum);


        if(s32ResultNum > 0)
        {
            SAMPLE_SVP_NPU_DETECTION_OUTPUT *p = (SAMPLE_SVP_NPU_DETECTION_OUTPUT *)pResult;

            for(int i = 0; i < s32ResultNum; i++)
            {
                rect[i].left    = p[i].x*stFrameInfo.stVFrame.u32Width/SSD_WIDTH;
                rect[i].top     = p[i].y*stFrameInfo.stVFrame.u32Height/SSD_HEIGHT;
                rect[i].right   = (p[i].x+p[i].w)*stFrameInfo.stVFrame.u32Width/SSD_WIDTH;
                rect[i].bottom  = (p[i].y+p[i].h)*stFrameInfo.stVFrame.u32Height/SSD_HEIGHT;
                drawBoxByVgs(&stFrameInfo, rect, s32ResultNum, 0xf00fff, 2);
            }

            free(pResult);
        }


        s32Ret = SC_MPI_VO_SendFrame(voLayer, VoChn, &stFrameInfo, -1);
        if(s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SendFrame failed!\n");
            goto EXIT2;
        }

        SC_MPI_VI_ReleaseChnFrame(ViPipe, ViChn, &stFrameInfo);

        if(s32Ret)
        {
            break;
        }
    }

    PAUSE();

    free_vi_frm_bgr(&resize_image);

    SAMPLE_SVP_NPU_Deinit(&s_stSsdModel, &s_stSsdNpuParam);

EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

static SC_S32 SAMPLE_AUDIO_AiAo()
{
    SC_S32 ret = -1;
    AI_CHN_PARAM_S stAiChnPara;
    SC_U32 s32AiChnCnt;
    SC_U32 s32AoChnCnt;
    AUDIO_DEV   AiDev = 0;
    AUDIO_DEV   AoDev = 0;
    SC_S32      AiChn = 0;
    SC_S32      AoChn = 0;
    SC_S32      volume = 30;

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


    ret = SAMPLE_COMM_AI_Bind_AO(AiDev, AiChn, AoDev, AoChn);
    if (ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AI_Bind_AO failed! %d\n", ret);
        goto AIAO_ERR1;
    }
    printf("ai(%d,%d) bind to ao(%d,%d) ok\n", AiDev, AiChn, AoDev, AoChn);

    return ret;

AIAO_ERR0:
    ret = SAMPLE_COMM_AI_UnBind_AO(AiDev, AiChn, AoDev, AoChn);
    if (ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AI_UnBind_AO failed! %d\n", ret);
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

static SC_S32 SAMPLE_AUDIO_AiAo_Exit()
{
    SC_S32 ret = -1;
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

    ret = SAMPLE_COMM_AI_UnBind_AO(AiDev, AiChn, AoDev, AoChn);
    if (ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AI_UnBind_AO failed! %d\n", ret);
    }

    sleep(1);
    ret = SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt);
    if (ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAi failed! %d\n", ret);
    }
    ret = SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt);
    if (ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_AUDIO_StopAo failed! %d\n", ret);
    }
    return ret;
}

static int SAMPLE_VO_MIPILCD_800x1280(int VoChnNum)
{
    SC_S32             i, s32Ret = 0;
    SC_S32                      VoDev           = 0;
    SC_S32                      VoLayer         = 0;
    VO_PUB_ATTR_S               stPubAttr;
    VO_VIDEO_LAYER_ATTR_S       stLayerAttr;
    VO_USER_INTFSYNC_INFO_S     stUserInfo;
    SC_U32                      u32Framerate;
    SIZE_S                      stDevSize;
    VO_CHN_ATTR_S               astChnAttr[VO_MAX_CHN_NUM];
    VO_INTF_TYPE_E              enVoIntfType    = VO_INTF_HDMI;

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI;

    /* USER SET VO DEV SYNC INFO */
    stPubAttr.stSyncInfo.u16Hact = 800;
    stPubAttr.stSyncInfo.u16Hbb = 20;
    stPubAttr.stSyncInfo.u16Hfb = 40;

    stPubAttr.stSyncInfo.u16Vact = 1280;
    stPubAttr.stSyncInfo.u16Vbb = 20;
    stPubAttr.stSyncInfo.u16Vfb = 20;

    stPubAttr.stSyncInfo.u16Hpw = 20;
    stPubAttr.stSyncInfo.u16Vpw = 4;
    SAMPLE_CHECK_RET(SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_SetPubAttr");

    /* USER SET VO FRAME RATE */
    u32Framerate = 60;
    SAMPLE_CHECK_RET(SC_MPI_VO_SetDevFrameRate(VoDev, u32Framerate), "SC_MPI_VO_SetDevFrameRate");

    /* USER SET VO SYNC INFO OF USER INTF */
    stUserInfo.bClkReverse = SC_TRUE;
    stUserInfo.u32DevDiv = 1;
    stUserInfo.u32PreDiv = 1;
    stUserInfo.stUserIntfSyncAttr.enClkSource = VO_CLK_SOURCE_PLL;
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Fbdiv = 400;
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Frac = 0;
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Refdiv = 40;
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv1 = 1;
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv2 = 0;
    SAMPLE_CHECK_RET(SC_MPI_VO_SetUserIntfSyncInfo(VoDev, &stUserInfo), "SC_MPI_VO_SetUserIntfSyncInfo");

    /* ENABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Enable(VoDev), "SC_MPI_VO_Enable");

    /* SET VO DISPLAY BUFFER LENGTH */
    SAMPLE_CHECK_RET(SC_MPI_VO_SetDisplayBufLen(VoDev, 3), "SC_MPI_VO_SetDisplayBufLen");

    /* USER CONFIG MIPI DEV */
    SAMPLE_CHECK_RET(SAMPLE_VO_CONFIG_MIPI(MIPILCD_800x1280), "SAMPLE_VO_CONFIG_MIPI");

    /*SET VO LAYER ATTR*/
    stDevSize.u32Width = stPubAttr.stSyncInfo.u16Hact;
    stDevSize.u32Height = stPubAttr.stSyncInfo.u16Vact;

    SAMPLE_VO_GetUserLayerAttr(&stLayerAttr, &stDevSize);
    stLayerAttr.u32DispFrmRt = 60;

    SAMPLE_CHECK_RET(SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_SetVideoLayerAttr");

    /* ENABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_EnableVideoLayer(VoLayer), "SC_MPI_VO_EnableVideoLayer");

    /* SET AND ENABLE VO CHN */
    SAMPLE_VO_GetUserChnAttr(astChnAttr, &stDevSize, VoChnNum);

    for (i = 0; i < VoChnNum; i++)
    {
        SAMPLE_CHECK_RET(SC_MPI_VO_SetChnAttr(VoLayer, i, &astChnAttr[i]), "SC_MPI_VO_SetChnAttr");

        SAMPLE_CHECK_RET(SC_MPI_VO_EnableChn(VoLayer, i), "SC_MPI_VO_EnableChn");
    }

    return s32Ret;
}

#define CoverMinHandle 40
static SC_S32 SAMPLE_REGION_IncCover(PIC_SIZE_E enPicSize0)
{
    SC_S32 s32Ret;
    RGN_CHN_ATTR_S stChnAttr;
    static SC_S32 HandleNum = CoverMinHandle;
    SC_S32 covernum = HandleNum - CoverMinHandle;
    SC_S32 step1, step2;

    if (covernum >= 16)
    {
        return SC_SUCCESS;
    }

    if (PIC_3840x2160 == enPicSize0)
    {
        step1 = 512;
        step2 = 64;
    }
    else if (PIC_1080P == enPicSize0)
    {
        step1 = 256;
        step2 = 32;
    }
    else
    {
        SAMPLE_PRT("enPicSize0(%d) no support\n", enPicSize0);
        return SC_FAILURE;
    }

    RGN_ATTR_S stRegion;
    stRegion.enType = COVER_RGN;

    s32Ret = SC_MPI_RGN_Create(HandleNum, &stRegion);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_Create failed! HandleNum%d\n", HandleNum);
        return s32Ret;
    }

    MPP_CHN_S stMppChn;
    stMppChn.enModId = SC_ID_VI;
    stMppChn.s32DevId = 0;
    stMppChn.s32ChnId = 0;

    /*set the chn config*/
    stChnAttr.bShow = SC_TRUE;
    stChnAttr.enType = COVER_RGN;
    stChnAttr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;
    stChnAttr.unChnAttr.stCoverChn.stRect.s32X = (covernum % 4)*(step1 + step2);
    stChnAttr.unChnAttr.stCoverChn.stRect.s32Y  = (covernum / 4)*(step1 + step2);
    stChnAttr.unChnAttr.stCoverChn.stRect.u32Height = step1;
    stChnAttr.unChnAttr.stCoverChn.stRect.u32Width  = step1;
    stChnAttr.unChnAttr.stCoverChn.u32Color      = 0x0000ffff;
    stChnAttr.unChnAttr.stCoverChn.enCoordinate = RGN_ABS_COOR;
    stChnAttr.unChnAttr.stCoverChn.u32Layer = HandleNum;
    s32Ret = SC_MPI_RGN_AttachToChn(HandleNum, &stMppChn, &stChnAttr);
    if(SC_SUCCESS != s32Ret)
    {
        SC_MPI_RGN_Destroy(HandleNum);
        SAMPLE_PRT("SC_MPI_RGN_AttachToChn failed!\n");
        return s32Ret;
    }

    //printf("SAMPLE_REGION_IncCover HandleNum(%d) end!\n", HandleNum);

    HandleNum++;

    return s32Ret;
}

static void uptime_update(int *tv)
{
	struct sysinfo info;
	sysinfo(&info);
	*tv = info.uptime;
	return;
}

static int uptime_diff(int *tv_start)
{
	struct sysinfo info;
	int diff_value = 0;

	if(NULL == tv_start)
    {
        return -1;
    }
	if (0 == *tv_start)
	{
		uptime_update(tv_start);
	}

	sysinfo(&info);

	diff_value = info.uptime - *tv_start;
	if (diff_value < 0)
	{
		uptime_update(tv_start);
		return 1;
	}
	else
	{
		return diff_value;
	}
}

static SC_S32 SAMPLE_VIO_REGIONCreateOverLayEx(SC_S32 HandleNum)
{
    SC_S32 s32Ret;
    SC_S32 i;
    RGN_ATTR_S stRegion;

    stRegion.enType = OVERLAYEX_RGN;
    stRegion.unAttr.stOverlayEx.enPixelFmt = PIXEL_FORMAT_ARGB_1555;
    stRegion.unAttr.stOverlayEx.stSize.u32Height = 200;
    stRegion.unAttr.stOverlayEx.stSize.u32Width  = 200;
    stRegion.unAttr.stOverlayEx.u32BgColor = 0x00ff00ff;
    stRegion.unAttr.stOverlayEx.u32CanvasNum = 2;


    SC_S32 MinHandle = SAMPLE_COMM_REGION_GetMinHandle(stRegion.enType);

    for(i = MinHandle; i < MinHandle + HandleNum; i++)
    {
        s32Ret = SC_MPI_RGN_Create(i, &stRegion);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_RGN_Create failed with %#x!\n", s32Ret);
            return SC_FAILURE;
        }
    }

    return s32Ret;
}

static SC_S32 SAMPLE_VIO_REGION_AttachToChn(SC_S32 HandleNum,
    RGN_TYPE_E enType, MPP_CHN_S *pstMppChn)
{
    SC_S32 i;
    SC_S32 s32Ret;
    SC_S32 MinHadle;
    RGN_CHN_ATTR_S stChnAttr = {0};

    MinHadle = SAMPLE_COMM_REGION_GetMinHandle(enType);

    /*set the chn config*/
    stChnAttr.bShow = SC_TRUE;
    stChnAttr.enType = OVERLAYEX_RGN;
    stChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha = 128;
    stChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = 128;

    /*attach to Chn*/
    for(i = MinHadle; i < MinHadle + HandleNum; i++)
    {
            stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = 20 + 64 * (i - MinHadle);
            //stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = 20 + 200 * (i - MinHadle);
            stChnAttr.unChnAttr.stOverlayExChn.u32Layer = i - MinHadle;

        s32Ret = SAMPLE_REGION_AttachToChn(i, pstMppChn, &stChnAttr);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_REGION_AttachToChn failed!\n");
            return s32Ret;
        }
    }

    return s32Ret;
}

SC_S32 SAMPLE_VIO_REGION_VI_OSDEX(int temp)
{
    SC_S32         i;
    SC_S32         s32Ret;
    SC_S32         MinHandle;
    SC_S32         HandleNum = 3;
    RGN_TYPE_E     enType = OVERLAYEX_RGN;
    MPP_CHN_S      stChn = {SC_ID_VI, 0, 0};
    static SC_BOOL first_run = SC_FALSE;
    char           bmpname[128] = {0};

    if (!first_run)
    {
        s32Ret = SAMPLE_VIO_REGIONCreateOverLayEx(HandleNum);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_REGION_Create failed!\n");
            return s32Ret;
        }
        s32Ret = SAMPLE_VIO_REGION_AttachToChn(HandleNum, enType, &stChn);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
            return s32Ret;
        }
        first_run = SC_TRUE;
    }

    MinHandle = SAMPLE_COMM_REGION_GetMinHandle(enType);

    int sep_temp[3];
    sep_temp[0] = ((temp/100)%10);
    sep_temp[1] = ((temp/10)%10);
    sep_temp[2] = (temp%10);

    for (i = 0; i < HandleNum; i++)
    {
        sprintf(bmpname, "./bmp/%1d.bmp", sep_temp[i]);
        printf("temp:%d, [%d]->bmpname:%s\n", temp, i, bmpname);

        s32Ret = SAMPLE_COMM_REGION_GetUpCanvasEx(MinHandle+i, bmpname);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_REGION_SetBitMap failed!\n");
            return s32Ret;
        }
    }

    return s32Ret;
}

#define TV_INVERAL 10
static SC_S32 SAMPLE_REGION_TimerIncCover(PIC_SIZE_E enPicSize0)
{
    static SC_S32 tv = 0;
    SC_S32 diff = uptime_diff(&tv);

    if (diff < TV_INVERAL)
    {
        return SC_SUCCESS;
    }

    uptime_update(&tv);

    //SAMPLE_REGION_IncCover(enPicSize0);

    static int temp[10] = {1, 10, 18, 110, 108, 128};
    static int cnt = 0;

    SAMPLE_VIO_REGION_VI_OSDEX(temp[cnt++]);

    return SC_SUCCESS;
}


SC_S32 SAMPLE_VIO_Vi4Sensor(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret = -1;

    SC_S32             s32ViCnt       = 4;
    VI_DEV             ViDev[4]       = {0, 1, 2, 3};
    VI_PIPE            ViPipe[4]      = {0, 1, 2, 3};

    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize, enPicSize0, enPicSize1;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn[4]       = {0, 1, 2, 3};
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_S32             flag           = 0;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    if(SONY_IMX307_MIPI_2M_30FPS_12BIT != stViConfig.astViInfo[0].stSnsInfo.enSnsType
        && SONY_IMX307_MIPI_2M_30FPS_12BIT != stViConfig.astViInfo[1].stSnsInfo.enSnsType
        && SONY_IMX307_MIPI_2M_30FPS_12BIT != stViConfig.astViInfo[2].stSnsInfo.enSnsType
        && SONY_IMX307_MIPI_2M_30FPS_12BIT != stViConfig.astViInfo[3].stSnsInfo.enSnsType)
    {
        SAMPLE_PRT("only tested 4 imx307\n");
        return s32Ret;
    }

    s32WorkSnsId = 0;
    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[s32WorkSnsId]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    s32WorkSnsId = 1;
    stViConfig.as32WorkingViId[s32WorkSnsId]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    s32WorkSnsId = 2;
    stViConfig.as32WorkingViId[s32WorkSnsId]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    s32WorkSnsId = 3;
    stViConfig.as32WorkingViId[s32WorkSnsId]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size for sensor 0*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize0);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    /*get picture size for sensor 1*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[1].stSnsInfo.enSnsType, &enPicSize1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("enPicSize0 = %d, enPicSize1 = %d \n", enPicSize0, enPicSize1);

    if(enPicSize0 >= enPicSize1)
    {
        enPicSize = enPicSize0;
    }
    else
    {
        enPicSize = enPicSize1;
    }
    SAMPLE_PRT("enPicSize =%d\n", enPicSize);

    /*get picture size*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("(w h)=>(%d %d)\n", stSize.u32Width, stSize.u32Height);

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 30;
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
        goto EXIT;
    }

#if 0
    setLdc(ViPipe[0], ViChn);
    setLdc(ViPipe[1], ViChn);
#endif

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enVoMode = VO_MODE_4MUX;
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[2], ViChn, stVoConfig.VoDev, VoChn[2]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[3], ViChn, stVoConfig.VoDev, VoChn[3]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_AUDIO_AiAo();
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_AUDIO_AiAo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    while(1)
    {
        //PAUSE();
        sleep(5);
        SAMPLE_COMM_VO_StopVO(&stVoConfig);

        flag = flag == 1 ? 0 : 1;
        if(flag)
        {
            s32Ret = SAMPLE_VO_MIPILCD_800x1280(1);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
                goto EXIT4;
            }
        }
        else
        {
            /*start vo*/
            s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
                goto EXIT4;
            }
        }
    }

EXIT4:
    SAMPLE_AUDIO_AiAo_Exit();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[2], ViChn, stVoConfig.VoDev, VoChn[2]);
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[3], ViChn, stVoConfig.VoDev, VoChn[3]);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViSensorSwitch(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret         = SC_FALSE;

    SC_S32             s32ViCnt       = 1;
#ifdef CONFIGSC_SCA200V200_BD1_DEV
    VI_DEV             ViDev[2]       = {0, 1};
#else
    VI_DEV             ViDev[2]       = {0, 2};
#endif
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize, enPicSize0, enPicSize1;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    s32WorkSnsId = 1;
    stViConfig.as32WorkingViId[1]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
#ifndef CONFIGSC_SCA200V200_BD1_DEV
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 3;
#endif
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size for sensor 0*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize0);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    /*get picture size for sensor 1*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[1].stSnsInfo.enSnsType, &enPicSize1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("enPicSize0 = %d, enPicSize1 = %d \n", enPicSize0, enPicSize1);

    if(enPicSize0 >= enPicSize1)
    {
        enPicSize = enPicSize0;
    }
    else
    {
        enPicSize = enPicSize1;
    }
    SAMPLE_PRT("enPicSize =%d\n", enPicSize);

    /*get picture size*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("(w h)=>(%d %d)\n", stSize.u32Width, stSize.u32Height);

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 12;
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;
    #if 0
    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }
    #endif

    stViConfig.as32WorkingViId[0] = 1;

    while(1)
    {
        /*start vi*/
        s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
            goto EXIT;
        }

        s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT1;
        }

        /*vi bind vo*/
        s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }

        PAUSE();

        s32Ret = SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("unbind vo failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }

        s32Ret = SAMPLE_COMM_VO_StopVO(&stVoConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("vo stop. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT1;
        }

        s32Ret = SAMPLE_COMM_VI_StopVi(&stViConfig);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("stop vi failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }

        stViConfig.as32WorkingViId[0] = stViConfig.as32WorkingViId[0] == 1 ? 0 : 1;
    }

    PAUSE();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_ViOnlineVencSnap(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_JPEG;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    SC_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    VIDEO_FRAME_INFO_S stFrame;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
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
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
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
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

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
        goto EXIT;
    }

    /*config venc */
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn) / sizeof(VENC_CHN));
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT3;
    }

    while(1)
    {
        s32Ret = SC_MPI_VI_GetChnFrame(ViPipe, ViChn, &stFrame, -1);
        if(s32Ret)
        {
            SAMPLE_PRT("get frame failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT4;
        }

        s32Ret = SC_MPI_VENC_SendFrame(VencChn[0], &stFrame, -1);
        if (s32Ret)
        {
            SC_MPI_VI_ReleaseChnFrame(ViPipe, ViChn, &stFrame);
            printf("send frame failed\n");
            goto EXIT4;
        }

        s32Ret = SC_MPI_VI_ReleaseChnFrame(ViPipe, ViChn, &stFrame);
        if(s32Ret)
        {
            SAMPLE_PRT("release frame failed. s32Ret: 0x%x !\n", s32Ret);
        }
    }

    PAUSE();

EXIT4:
    SAMPLE_COMM_VENC_StopGetStream();
EXIT3:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT2:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

// #define __PR_DEBUG
#ifdef __PR_DEBUG
# define pr_debug(fmt, args...) do { \
    printf("\033[0;33m[VDEC]:\033[0m"fmt, ##args); \
} while (0)
#else
# define pr_debug(fmt, args...) do { } while (0)
#endif

static SC_S32 g_VdecRun = 1;
static pthread_t g_VdecSendPid[2];
static VDEC_THREAD_PARAM_S stVdecSend[2];
static pthread_t VdecThread[2];
static SC_VOID *SAMPLE_VDEC_Thread(SC_VOID *pArgs)
{
    VDEC_THREAD_PARAM_S *pstVdecThreadParam = (VDEC_THREAD_PARAM_S *)pArgs;
    SC_S32 s32Ret;
    VIDEO_FRAME_INFO_S stVFrame;
    int cnt = 0;

    while(g_VdecRun)
    {
        if (pstVdecThreadParam->eThreadCtrl == THREAD_CTRL_STOP)
        {
            break;
        }

        s32Ret = SC_MPI_VDEC_GetFrame(pstVdecThreadParam->s32ChnId, &stVFrame, pstVdecThreadParam->s32MilliSec);
        if (SC_SUCCESS == s32Ret && stVFrame.stVFrame.u64PhyAddr[0])
        {
            cnt++;

            pr_debug("Chn%d: get pic the %d times.\n", pstVdecThreadParam->s32ChnId, cnt);
            s32Ret = SC_MPI_VDEC_ReleaseFrame(pstVdecThreadParam->s32ChnId, &stVFrame);
        }
        else
        {
            usleep(100);
        }
    }

    pr_debug("Chn%d: get pic thread return!\n", pstVdecThreadParam->s32ChnId);
    return (SC_VOID *)SC_SUCCESS;
}

static SC_S32 SAMPLE_VIO_VDEC_2CHN_Start(SC_U32 u32VdecChnNum, SC_U32 u32BlkSize)
{
    SC_S32 i, s32Ret = 0;
    SAMPLE_VDEC_ATTR astSampleVdec[2];

    for (i = 0; i < u32VdecChnNum; i++)
    {
        astSampleVdec[i].enType                           = PT_H264;
		astSampleVdec[i].u32Width                         = 1920;
		astSampleVdec[i].u32Height                        = 1080;
        astSampleVdec[i].enMode                           = VIDEO_MODE_FRAME;
        astSampleVdec[i].stSapmleVdecVideo.enDecMode      = VIDEO_DEC_MODE_IP;
        astSampleVdec[i].stSapmleVdecVideo.enBitWidth     = DATA_BITWIDTH_8;
        astSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum = 2;
        astSampleVdec[i].u32DisplayFrameNum               = 2;
        astSampleVdec[i].u32FrameBufCnt = astSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum +
            astSampleVdec[i].u32DisplayFrameNum + 1;
    }

    /************************************************
    step1:  start VDEC
    *************************************************/
    s32Ret = SAMPLE_COMM_VDEC_Start(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END;
    }

    /************************************************
    step2:  send stream to VDEC
    *************************************************/
    for (i = 0; i < u32VdecChnNum; i++)
    {
        snprintf(stVdecSend[i].cFileName, sizeof(stVdecSend[i].cFileName), "1920x1080.h264");
        snprintf(stVdecSend[i].cFilePath, sizeof(stVdecSend[i].cFilePath), "%s", "./res");
        stVdecSend[i].enType          = astSampleVdec[i].enType;
        stVdecSend[i].s32StreamMode   = astSampleVdec[i].enMode;
        stVdecSend[i].s32ChnId        = i;
        stVdecSend[i].s32IntervalTime = 25 * 1000; //40fps
        stVdecSend[i].u64PtsInit      = 0;
        stVdecSend[i].u64PtsIncrease  = 0;
        stVdecSend[i].eThreadCtrl     = THREAD_CTRL_START;
        stVdecSend[i].bCircleSend     = SC_TRUE;
        stVdecSend[i].s32MilliSec     = 0;
        stVdecSend[i].s32MinBufSize   = u32BlkSize;
        //(astSampleVdec[i].u32Width * astSampleVdec[i].u32Height * 3) >> 1;
    }
    SAMPLE_COMM_VDEC_RPC_StartSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);

    usleep(100 * 1000);
    g_VdecRun = 1;
    for (i = 0; i < u32VdecChnNum; i++)
    {
        pthread_create(&g_VdecSendPid[i], NULL, SAMPLE_VDEC_Thread, &stVdecSend[i]);
    }

END:
    return s32Ret;
}

static SC_VOID SAMPLE_VIO_VDEC_2CHN_Stop(SC_U32 u32VdecChnNum)
{
    SC_S32 i;

    SAMPLE_COMM_VDEC_RPC_StopSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);
    g_VdecRun = 0;
    for (i = 0; i < u32VdecChnNum; i++)
    {
        pthread_join(g_VdecSendPid[i], NULL);
    }

    SAMPLE_COMM_VDEC_Stop(u32VdecChnNum);
}


SC_S32 SAMPLE_VIO_Vi3Sensor_415d307(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret = -1;

    SC_S32             s32ViCnt       = 3;
    VI_DEV             ViDev[3]       = {0, 1, 2};
    VI_PIPE            ViPipe[3]      = {0, 1, 2};

    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize, enPicSize0, enPicSize1;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn[4]       = {0, 1, 2, 3};
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_S32             flag           = 0;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    if(SONY_IMX415_MIPI_8M_30FPS_12BIT != stViConfig.astViInfo[0].stSnsInfo.enSnsType
        && SONY_IMX307_MIPI_2M_30FPS_12BIT != stViConfig.astViInfo[1].stSnsInfo.enSnsType
        && SONY_IMX307_MIPI_2M_30FPS_12BIT != stViConfig.astViInfo[2].stSnsInfo.enSnsType)
    {
        SAMPLE_PRT("only tested imx415 + 2 imx307\n");
        return s32Ret;
    }

    s32WorkSnsId = 0;
    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[s32WorkSnsId]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 1;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    s32WorkSnsId = 1;
    stViConfig.as32WorkingViId[s32WorkSnsId]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = 2;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 3;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    s32WorkSnsId = 2;
    stViConfig.as32WorkingViId[s32WorkSnsId]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = 3;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 0;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size for sensor 0*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize0);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    /*get picture size for sensor 1*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[1].stSnsInfo.enSnsType, &enPicSize1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("enPicSize0 = %d, enPicSize1 = %d \n", enPicSize0, enPicSize1);

    if(enPicSize0 >= enPicSize1)
    {
        enPicSize = enPicSize0;
    }
    else
    {
        enPicSize = enPicSize1;
    }
    SAMPLE_PRT("enPicSize =%d\n", enPicSize);

    /*get picture size*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("(w h)=>(%d %d)\n", stSize.u32Width, stSize.u32Height);

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 2;
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
        goto EXIT;
    }

#if 0
    setLdc(ViPipe[0], ViChn);
    setLdc(ViPipe[1], ViChn);
#endif

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enVoMode = VO_MODE_4MUX;
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[2], ViChn, stVoConfig.VoDev, VoChn[2]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }


    s32Ret = SAMPLE_AUDIO_AiAo();
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_AUDIO_AiAo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    /* VENC */
    SC_S32 i;
    VENC_CHN           VencChn[3]  = { 0, 1, 2 };
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    SC_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;
    VENC_CHN_ATTR_S    stChnAttr;
    VENC_RECV_PIC_PARAM_S  stRecvParam;

    enPicSize = PIC_1080P;
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);

    for (i = 0; i < 3; i++)
    {
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn[i], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT4;
        }
        s32Ret = SAMPLE_COMM_VI_Bind_VENC(ViPipe[i], ViChn, VencChn[i]);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Venc bind Vi failed. s32Ret: 0x%x !n", s32Ret);
            SAMPLE_COMM_VENC_Stop(VencChn[i]);
            goto EXIT4;
        }
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStreamNoFile(VencChn, sizeof(VencChn) / sizeof(VENC_CHN));
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT4;
    }

    /* VDEC */
    s32Ret = SAMPLE_VIO_VDEC_2CHN_Start(2, u32BlkSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT5;
    }

    while(1)
    {
        SAMPLE_REGION_TimerIncCover(enPicSize0);

        //PAUSE();
        sleep(5);
        SAMPLE_COMM_VO_StopVO(&stVoConfig);

        flag = flag == 1 ? 0 : 1;
        if(flag)
        {
            s32Ret = SAMPLE_VO_MIPILCD_800x1280(1);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
                goto EXIT4;
            }
        }
        else
        {
            /*start vo*/
            s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
            if (SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
                goto EXIT4;
            }
        }
    }

    SAMPLE_VIO_VDEC_2CHN_Stop(2);

EXIT5:
    SAMPLE_COMM_VENC_StopGetStream();

EXIT4:
    for (int j = 0; j < i; ++j) {
        SAMPLE_COMM_VI_UnBind_VENC(ViPipe[j], ViChn, VencChn[j]);
        SAMPLE_COMM_VENC_Stop(VencChn[j]);
    }
    SAMPLE_AUDIO_AiAo_Exit();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[2], ViChn, stVoConfig.VoDev, VoChn[2]);

EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

SC_S32 SAMPLE_VIO_Vi2SensorVenc(SC_U32 u32VoIntfType)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 2;
#ifdef CONFIGSC_SCA200V200_BD1_DEV
        VI_DEV             ViDev[2]       = {0, 1};
#else
    VI_DEV             ViDev[2]       = {0, 2};
#endif
    VI_PIPE            ViPipe[2]      = {0, 1};

    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize, enPicSize0, enPicSize1;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn[4]       = {0, 1, 2, 3};
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    /*config vi*/
    memset(&stViConfig, 0, sizeof(stViConfig));
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    s32WorkSnsId = 0;
    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
#ifndef CONFIGSC_SCA200V200_BD1_DEV
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 1;
#endif
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    s32WorkSnsId = 1;
    stViConfig.as32WorkingViId[1]                                = s32WorkSnsId;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev[s32WorkSnsId];
#ifndef CONFIGSC_SCA200V200_BD1_DEV
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 3;
#endif
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_MULTI_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[s32WorkSnsId];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size for sensor 0*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize0);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    /*get picture size for sensor 1*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[1].stSnsInfo.enSnsType, &enPicSize1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("enPicSize0 = %d, enPicSize1 = %d \n", enPicSize0, enPicSize1);

    if(enPicSize0 >= enPicSize1)
    {
        enPicSize = enPicSize0;
    }
    else
    {
        enPicSize = enPicSize1;
    }
    SAMPLE_PRT("enPicSize =%d\n", enPicSize);

    /*get picture size*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }
    SAMPLE_PRT("(w h)=>(%d %d)\n", stSize.u32Width, stSize.u32Height);

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 30;
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
        goto EXIT;
    }

#if 0
    setLdc(ViPipe[0], ViChn);
    setLdc(ViPipe[1], ViChn);
#endif

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enVoMode = VO_MODE_4MUX;
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config venc */
    VENC_CHN           VencChn[2]  = {0, 1};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    SC_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;
    VENC_CHN_ATTR_S    stChnAttr;
    VENC_RECV_PIC_PARAM_S  stRecvParam;

    /*config venc */
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize0, enRcMode, u32Profile, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[1], enType, enPicSize1, enRcMode, u32Profile, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VENC(ViPipe[0], ViChn, VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vi failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VENC(ViPipe[1], ViChn, VencChn[1]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vi failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStreamNoFile(VencChn, sizeof(VencChn) / sizeof(VENC_CHN));
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT5;
    }

#if 0
    /* Set second sensor monochrome */
    ISP_SATURATION_ATTR_S stSatAttr = {};
    stSatAttr.u32Saturation = 0;
    s32Ret = SC_MPI_ISP_SetSaturationAttr(ViPipe[1], &stSatAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Set saturation failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }
#endif
    PAUSE();

    SAMPLE_COMM_VENC_StopGetStream();
EXIT5:
    SAMPLE_COMM_VI_UnBind_VENC(ViPipe[0], ViChn, VencChn[0]);
    SAMPLE_COMM_VI_UnBind_VENC(ViPipe[1], ViChn, VencChn[1]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
    SAMPLE_COMM_VENC_Stop(VencChn[1]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[0], ViChn, stVoConfig.VoDev, VoChn[0]);
    SAMPLE_COMM_VI_UnBind_VO(ViPipe[1], ViChn, stVoConfig.VoDev, VoChn[1]);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}


/******************************************************************************
* function :  VI + (MIPI + RGB) or HDMI loop.
******************************************************************************/
SC_S32 SAMPLE_VIO_MIPIRGB_HDMI_LOOP(void)
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

    SAMPLE_VO_CONFIG_S stVoConfig      = {0};
    VO_CHN             VoChn           = 0;
    SC_S32             VoDev           = 0;
    SC_S32             VoLayer         = 0;
    int                flag = 0;

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

    /* get picture raw size */
    s32Ret = SAMPLE_COMM_VI_GetRawPicSize(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &stRawSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get raw picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 3;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stRawSize.u32Width, stRawSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Height, stSize.u32Width, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[2].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt   = 6;

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

    SAMPLE_COMM_VO_MIPIRGB_HDMI_720x1280(flag);

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VO_STOP;
    }

    while(1)
    {
        SAMPLE_REGION_TimerIncCover(0);
        sleep(5);
        SAMPLE_COMM_VO_StopVO(&stVoConfig);

        flag = !flag;
        SAMPLE_COMM_VO_MIPIRGB_HDMI_720x1280(flag);

    }

EXIT_VIVO_STOP:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT_VO_STOP:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}


SC_S32 SAMPLE_VIO_IMX415(SC_U32 mipiDev)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_U32             u32Align       = 0;

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
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.u32Align        = u32Align;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.bMirror         = SC_FALSE;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.bFlip           = SC_FALSE;

    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX415_MIPI_8M_30FPS_12BIT;
    SAMPLE_COMM_ISP_SetSnsType(s32WorkSnsId, stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType);
    if (mipiDev)
    {
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = 2;
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 3;
    }

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
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
    stVbConf.astCommPool[0].u32BlkCnt   = 2;

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
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    stVoConfig.enVoIntfType = VO_INTF_HDMI;
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    PAUSE();
EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
