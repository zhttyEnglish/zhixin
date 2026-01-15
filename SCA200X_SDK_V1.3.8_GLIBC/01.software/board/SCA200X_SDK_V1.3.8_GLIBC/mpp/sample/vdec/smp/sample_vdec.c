/* vi: set sw=4 ts=4 et: */

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

#include "sample_comm.h"
#include "mpi_ive.h"
#include "sample_comm_ive.h"

#define SAMPLE_STREAM_PATH "./source_file"

static SC_U32 s_VdecChnNum;
static VDEC_THREAD_PARAM_S *s_VdecSend[VDEC_MAX_CHN_NUM];
static pthread_t s_VdecThread[2 * VDEC_MAX_CHN_NUM];

static int scaling = 1;

//#define __DEBUG_RESTART
//#define __VDEC_INFO
#ifdef __VDEC_INFO
# define pr_debug(fmt, args...) do { \
    printf("\033[0;33m[VDEC]:\033[0m"fmt, ##args); \
} while (0)
#else
# define pr_debug(fmt, args...) do {} while (0)
#endif

SC_S32 SAMPLE_COMM_VO_HdmiStop(SC_VOID)
{
    return SC_SUCCESS;
}

SC_VOID SAMPLE_VDEC_HandleSig(SC_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTSTP == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_VO_HdmiStop();
        SAMPLE_COMM_VDEC_RPC_StopSendStream(s_VdecChnNum, s_VdecSend[0], &s_VdecThread[0]);
        usleep(50 * 1000);
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

VO_INTF_SYNC_E g_enIntfSync = VO_OUTPUT_3840x2160_30;

SC_S32 SAMPLE_H265_VDEC_VPSS_VO(SC_VOID)
{
    VB_CONFIG_S stVbConfig;
    SC_S32 i, s32Ret = SC_SUCCESS;
    VDEC_THREAD_PARAM_S stVdecSend[VDEC_MAX_CHN_NUM];
    SIZE_S stDispSize;
    VO_LAYER VoLayer;
    SC_U32 u32VdecChnNum, VpssGrpNum;
    VPSS_GRP VpssGrp;
    pthread_t   VdecThread[2 * VDEC_MAX_CHN_NUM];
    PIC_SIZE_E enDispPicSize;
    SAMPLE_VDEC_ATTR astSampleVdec[VDEC_MAX_CHN_NUM];
    VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_CHN_NUM];
    SAMPLE_VO_CONFIG_S stVoConfig;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    SC_BOOL abChnEnable[VPSS_MAX_CHN_NUM];
    VO_INTF_SYNC_E enIntfSync;
    VO_INTF_TYPE_E enVoIntfType;
    DYNAMIC_RANGE_E enDynamicRange = DYNAMIC_RANGE_SDR8;

    u32VdecChnNum = 1;
    VpssGrpNum    = u32VdecChnNum;
    /************************************************
    step1:  init SYS, init common VB(for VPSS and VO)
    *************************************************/

    if (VO_OUTPUT_3840x2160_30 == g_enIntfSync)
    {
        enDispPicSize = PIC_3840x2160;
        enIntfSync    = VO_OUTPUT_3840x2160_30;
        enVoIntfType  = VO_INTF_HDMI;
    }
    else
    {
        enDispPicSize = PIC_1080P;
        enIntfSync    = VO_OUTPUT_1080P30;
        enVoIntfType  = VO_INTF_BT1120;
    }

    s32Ret =  SAMPLE_COMM_SYS_GetPicSize(enDispPicSize, &stDispSize);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("sys get pic size fail for %#x!\n", s32Ret);
        goto END1;
    }

    memset(&stVbConfig, 0, sizeof(VB_CONFIG_S));
    stVbConfig.u32MaxPoolCnt             = 1;
    stVbConfig.astCommPool[0].u32BlkCnt  = 32 * u32VdecChnNum;
    stVbConfig.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(stDispSize.u32Width, stDispSize.u32Height,
            PIXEL_FORMAT_YVU_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, 0);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step2:  init module VB or user VB(for VDEC)
    *************************************************/
    for (i = 0; i < u32VdecChnNum; i++)
    {
        /* FIXME */
        astSampleVdec[i].enType                           = PT_H265;
        if (enDispPicSize == PIC_1080P)
        {
            astSampleVdec[i].u32Width                     = 1920;
            astSampleVdec[i].u32Height                    = 1080;
        }
        else
        {
            astSampleVdec[i].u32Width                     = 3840;
            astSampleVdec[i].u32Height                    = 2160;
        }
        astSampleVdec[i].enMode                           = VIDEO_MODE_FRAME;
        astSampleVdec[i].stSapmleVdecVideo.enDecMode      = VIDEO_DEC_MODE_IP;
        astSampleVdec[i].stSapmleVdecVideo.enBitWidth     = DATA_BITWIDTH_8;
        astSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum = 2;
        astSampleVdec[i].u32DisplayFrameNum               = 2;
        astSampleVdec[i].u32FrameBufCnt = astSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum +
            astSampleVdec[i].u32DisplayFrameNum + 1;
    }

#if 0
    s32Ret = SAMPLE_COMM_VDEC_InitVBPool(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END2;
    }
#endif

    /************************************************
    step3:  start VDEC
    *************************************************/
    s32Ret = SAMPLE_COMM_VDEC_Start(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END3;
    }

    /************************************************
    step4:  start VPSS
    *************************************************/
    if (enDispPicSize == PIC_1080P)
    {
        stVpssGrpAttr.u32MaxW = 1920;
        stVpssGrpAttr.u32MaxH = 1080;
    }
    else
    {
        stVpssGrpAttr.u32MaxW = 3840;
        stVpssGrpAttr.u32MaxH = 2160;
    }

    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.enDynamicRange = enDynamicRange;
    stVpssGrpAttr.enPixelFormat  = PIXEL_FORMAT_YVU_PLANAR_420;
    stVpssGrpAttr.bNrEn   = SC_FALSE;
    memset(abChnEnable, 0, sizeof(abChnEnable));
    abChnEnable[0] = SC_TRUE;
    astVpssChnAttr[0].u32Width                    = 1280;// stDispSize.u32Width;
    astVpssChnAttr[0].u32Height                   = 720;//stDispSize.u32Height;
    astVpssChnAttr[0].enChnMode                   = VPSS_CHN_MODE_AUTO;
    astVpssChnAttr[0].enCompressMode              = COMPRESS_MODE_SEG;
    astVpssChnAttr[0].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[0].enPixelFormat               = PIXEL_FORMAT_YVU_PLANAR_420;
    astVpssChnAttr[0].stFrameRate.s32SrcFrameRate = -1;
    astVpssChnAttr[0].stFrameRate.s32DstFrameRate = -1;
    astVpssChnAttr[0].u32Depth                    = 0;
    astVpssChnAttr[0].bMirror                     = SC_FALSE;
    astVpssChnAttr[0].bFlip                       = SC_FALSE;
    astVpssChnAttr[0].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    astVpssChnAttr[0].enVideoFormat               = VIDEO_FORMAT_LINEAR;
    for (i = 0; i < u32VdecChnNum; i++)
    {
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, &abChnEnable[0], &stVpssGrpAttr, &astVpssChnAttr[0]);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
            goto END4;
        }
    }

    /************************************************
    step5:  start VO
    *************************************************/
    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    stVoConfig.enVoIntfType = VO_INTF_HDMI;
    stVoConfig.enPicSize = enDispPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("start VO fail for %#x!\n", s32Ret);
        goto END5;
    }

    /************************************************
    step6:  VDEC bind VPSS
    *************************************************/
    for (i = 0; i < u32VdecChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_Bind_VPSS(i, i);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
            goto END6;
        }
    }

    /************************************************
    step7:  VPSS bind VO
    *************************************************/
    VoLayer = stVoConfig.VoDev;
    for (i = 0; i < VpssGrpNum; i++)
    {
        s32Ret = SAMPLE_COMM_VPSS_Bind_VO(i, 0, VoLayer, i);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
            goto END7;
        }
    }

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
    for (i = 0; i < u32VdecChnNum; i++)
    {
        if (enDispPicSize == PIC_1080P)
        {
            snprintf(stVdecSend[i].cFileName, sizeof(stVdecSend[i].cFileName), "1920x1080.h265");
        }
        else
        {
            snprintf(stVdecSend[i].cFileName, sizeof(stVdecSend[i].cFileName), "3840x2160_8bit.h265");
        }
        snprintf(stVdecSend[i].cFilePath, sizeof(stVdecSend[i].cFilePath), "%s", SAMPLE_STREAM_PATH);
        stVdecSend[i].enType          = astSampleVdec[i].enType;
        stVdecSend[i].s32StreamMode   = astSampleVdec[i].enMode;
        stVdecSend[i].s32ChnId        = i;
        stVdecSend[i].s32IntervalTime = 1000;
        stVdecSend[i].u64PtsInit      = 0;
        stVdecSend[i].u64PtsIncrease  = 0;
        stVdecSend[i].eThreadCtrl     = THREAD_CTRL_START;
        stVdecSend[i].bCircleSend     = SC_TRUE;
        stVdecSend[i].s32MilliSec     = 0;
        stVdecSend[i].s32MinBufSize   = (astSampleVdec[i].u32Width * astSampleVdec[i].u32Height * 3) >> 1;
    }
    SAMPLE_COMM_VDEC_RPC_StartSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);
    s_VdecChnNum = u32VdecChnNum;
    s_VdecSend[0] = (VDEC_THREAD_PARAM_S *)&stVdecSend;
    memcpy(s_VdecThread, VdecThread, sizeof(VdecThread));

    SAMPLE_COMM_VDEC_CmdCtrl(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);

    SAMPLE_COMM_VDEC_RPC_StopSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);

END7:
    for (i = 0; i < VpssGrpNum; i++)
    {
        s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(i, 0, VoLayer, i);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("vpss unbind vo fail for %#x!\n", s32Ret);
        }
    }

END6:
    for (i = 0; i < u32VdecChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_UnBind_VPSS(i, i);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("vdec unbind vpss fail for %#x!\n", s32Ret);
        }
    }

END5:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);

END4:
    for (i = VpssGrp; i >= 0; i--)
    {
        VpssGrp = i;
        SAMPLE_COMM_VPSS_Stop(VpssGrp, &abChnEnable[0]);
    }
END3:
    SAMPLE_COMM_VDEC_Stop(u32VdecChnNum);

END2:
    //SAMPLE_COMM_VDEC_ExitVBPool();

END1:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

static void yuv_8bit_dump(VIDEO_FRAME_S *pVBuf, FILE *pfd)
{
    unsigned int h;
    char *pMemContent;
    PIXEL_FORMAT_E enPixelFormat = pVBuf->enPixelFormat;
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
        fdatasync(fileno(pfd));
    }

    fprintf(stderr, "done %u!\n", pVBuf->u32TimeRef);
    fflush(stderr);
}

static SC_VOID *SAMPLE_VDEC_GetPic(SC_VOID *pArgs)
{
    VDEC_THREAD_PARAM_S *pstVdecThreadParam = (VDEC_THREAD_PARAM_S *)pArgs;
    FILE *fp = SC_NULL;
    SC_S32 s32Ret;
    VDEC_CHN_ATTR_S  stAttr;
    VIDEO_FRAME_INFO_S stVFrame;
    SC_CHAR cSaveFile[256];

    FILE *fout = fopen("vdec_vo.yuv", "w");
    if (fout == NULL)
    {
        SAMPLE_PRT("open file failed!\n");
        return (SC_VOID *)SC_SUCCESS;
    }

    while(1)
    {
        if (pstVdecThreadParam->eThreadCtrl == THREAD_CTRL_STOP)
        {
            break;
        }

        s32Ret = SC_MPI_VDEC_GetFrame(pstVdecThreadParam->s32ChnId, &stVFrame, pstVdecThreadParam->s32MilliSec);
        if (SC_SUCCESS == s32Ret && stVFrame.stVFrame.u64PhyAddr[0])
        {
            static int cnt;
            cnt++;

            yuv_8bit_dump(&stVFrame.stVFrame, fout);

            printf("\033[0;35m chn %d get pic %d times. \033[0;39m\n", pstVdecThreadParam->s32ChnId, cnt);
            s32Ret = SC_MPI_VDEC_ReleaseFrame(pstVdecThreadParam->s32ChnId, &stVFrame);
        }
        else
        {
            usleep(100);
        }
    }

    fclose(fout);
    printf("\033[0;35m chn %d get pic thread return ...  \033[0;39m\n", pstVdecThreadParam->s32ChnId);
    return (SC_VOID *)SC_SUCCESS;
}

SC_S32 SAMPLE_H265_VDEC(SC_VOID)
{
    VB_CONFIG_S stVbConfig;
    SC_S32 i, s32Ret = SC_SUCCESS;
    VDEC_THREAD_PARAM_S stVdecSend[VDEC_MAX_CHN_NUM];
    SIZE_S stDispSize;
    VO_LAYER VoLayer;
    SC_U32 u32VdecChnNum, VpssGrpNum;
    VPSS_GRP VpssGrp;
    pthread_t   VdecThread[2 * VDEC_MAX_CHN_NUM];
    PIC_SIZE_E enDispPicSize;
    SAMPLE_VDEC_ATTR astSampleVdec[VDEC_MAX_CHN_NUM];
    VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_CHN_NUM];
    SAMPLE_VO_CONFIG_S stVoConfig;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    SC_BOOL abChnEnable[VPSS_MAX_CHN_NUM];
    VO_INTF_SYNC_E enIntfSync;
    VO_INTF_TYPE_E enVoIntfType;

    u32VdecChnNum = 1;
    VpssGrpNum    = u32VdecChnNum;
    /************************************************
    step1:  init SYS, init common VB(for VPSS and VO)
    *************************************************/

    if (VO_OUTPUT_3840x2160_30 == g_enIntfSync)
    {
        enDispPicSize = PIC_3840x2160;
        enIntfSync    = VO_OUTPUT_3840x2160_30;
        enVoIntfType  = VO_INTF_HDMI;
    }
    else
    {
        enDispPicSize = PIC_1080P;
        enIntfSync    = VO_OUTPUT_1080P30;
        enVoIntfType  = VO_INTF_BT1120;
    }

    s32Ret =  SAMPLE_COMM_SYS_GetPicSize(enDispPicSize, &stDispSize);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("sys get pic size fail for %#x!\n", s32Ret);
        goto END1;
    }

    memset(&stVbConfig, 0, sizeof(VB_CONFIG_S));
    stVbConfig.u32MaxPoolCnt             = 1;
    stVbConfig.astCommPool[0].u32BlkCnt  = 16 * u32VdecChnNum;
    stVbConfig.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(stDispSize.u32Width, stDispSize.u32Height,
            PIXEL_FORMAT_YVU_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, 0);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step2:  init module VB or user VB(for VDEC)
    *************************************************/
    for (i = 0; i < u32VdecChnNum; i++)
    {
        astSampleVdec[i].enType                           = PT_H265;
        if (enDispPicSize == PIC_1080P)
        {
            astSampleVdec[i].u32Width                     = 1920;
            astSampleVdec[i].u32Height                    = 1080;
        }
        else
        {
            astSampleVdec[i].u32Width                     = 3840;
            astSampleVdec[i].u32Height                    = 2160;
        }
        astSampleVdec[i].enMode                           = VIDEO_MODE_FRAME;
        astSampleVdec[i].stSapmleVdecVideo.enDecMode      = VIDEO_DEC_MODE_IP;
        astSampleVdec[i].stSapmleVdecVideo.enBitWidth     = DATA_BITWIDTH_8;
        astSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum = 2;
        astSampleVdec[i].u32DisplayFrameNum               = 2;
        astSampleVdec[i].u32FrameBufCnt = astSampleVdec[i].stSapmleVdecVideo.u32RefFrameNum +
            astSampleVdec[i].u32DisplayFrameNum + 1;
    }

#if 0
    s32Ret = SAMPLE_COMM_VDEC_InitVBPool(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END2;
    }
#endif

    /************************************************
    step3:  start VDEC
    *************************************************/
    s32Ret = SAMPLE_COMM_VDEC_Start(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END3;
    }

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
    for (i = 0; i < u32VdecChnNum; i++)
    {
        if (enDispPicSize == PIC_1080P)
        {
            snprintf(stVdecSend[i].cFileName, sizeof(stVdecSend[i].cFileName), "1920x1080.h265");
        }
        else
        {
            snprintf(stVdecSend[i].cFileName, sizeof(stVdecSend[i].cFileName), "3840x2160_8bit.h265");
        }
        snprintf(stVdecSend[i].cFilePath, sizeof(stVdecSend[i].cFilePath), "%s", SAMPLE_STREAM_PATH);
        stVdecSend[i].enType          = astSampleVdec[i].enType;
        stVdecSend[i].s32StreamMode   = astSampleVdec[i].enMode;
        stVdecSend[i].s32ChnId        = i;
        stVdecSend[i].s32IntervalTime = 1000;
        stVdecSend[i].u64PtsInit      = 0;
        stVdecSend[i].u64PtsIncrease  = 0;
        stVdecSend[i].eThreadCtrl     = THREAD_CTRL_START;
        stVdecSend[i].bCircleSend     = SC_FALSE;
        stVdecSend[i].s32MilliSec     = 0;
        stVdecSend[i].s32MinBufSize   = (astSampleVdec[i].u32Width * astSampleVdec[i].u32Height * 3) >> 1;
    }
    SAMPLE_COMM_VDEC_RPC_StartSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);

    s_VdecChnNum = u32VdecChnNum;
    s_VdecSend[0] = (VDEC_THREAD_PARAM_S *)&stVdecSend;
    memcpy(s_VdecThread, VdecThread, sizeof(VdecThread));

    pthread_t pid;

    pthread_create(&pid, NULL, SAMPLE_VDEC_GetPic, &stVdecSend[0]);

    printf("press enter twice to exit...\n");
    getchar();
    getchar();

    SAMPLE_COMM_VDEC_RPC_StopSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);

END3:
    SAMPLE_COMM_VDEC_Stop(u32VdecChnNum);

END2:
    //SAMPLE_COMM_VDEC_ExitVBPool();

END1:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}


static SC_S32 SAMPLE_IVE_RESZIE(VIDEO_FRAME_INFO_S *pstVFrame, int w, int h)
{
    SC_S32 s32Ret;


    SC_S32 s32SrcWidth = 1920;
    SC_S32 s32SrcHeight = 1080;
    SC_S32 s32DstWidth = 1280;
    SC_S32 s32DstHeight = 720;

    IVE_SRC_IMAGE_S src = {0};
    IVE_SRC_IMAGE_S *pstSrcImg = &src;
    pstSrcImg->u32Width = pstVFrame->stVFrame.u32Width;
    pstSrcImg->u32Height = pstVFrame->stVFrame.u32Height;
    pstSrcImg->enType = IVE_IMAGE_TYPE_YUV420P;

	pstSrcImg->au32Stride[0] = pstVFrame->stVFrame.u32Stride[0];
	pstSrcImg->au64PhyAddr[0] = pstVFrame->stVFrame.u64PhyAddr[0];
	pstSrcImg->au64VirAddr[0] = pstVFrame->stVFrame.u64VirAddr[0];
	pstSrcImg->au32Stride[1] = pstVFrame->stVFrame.u32Stride[1];
	pstSrcImg->au64PhyAddr[1] = pstVFrame->stVFrame.u64PhyAddr[1];
	pstSrcImg->au64VirAddr[1] = pstVFrame->stVFrame.u64VirAddr[1];
	pstSrcImg->au32Stride[2] = pstVFrame->stVFrame.u32Stride[2];
	pstSrcImg->au64PhyAddr[2] = pstVFrame->stVFrame.u64PhyAddr[2];
	pstSrcImg->au64VirAddr[2] = pstVFrame->stVFrame.u64VirAddr[2];
	printf("RRRRRRRRRR: stride=%d,%d,%d\n",
			pstSrcImg->au32Stride[0],
			pstSrcImg->au32Stride[1],
			pstSrcImg->au32Stride[2]);

    IVE_SRC_IMAGE_S scaled = {0};
    IVE_SRC_IMAGE_S *pstScaledImg = &scaled;
    pstScaledImg->u32Width = w;
    pstScaledImg->u32Height = h;
    pstScaledImg->enType = IVE_IMAGE_TYPE_YUV420P;
    s32Ret = SAMPLE_COMM_IVE_CreateImage(pstScaledImg, pstScaledImg->enType, w, h);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_CreateImage err %d!\n", s32Ret);
        return -1;
    }
	printf("RRRRRRRRRR1: stride=%d,%d,%d\n",
			pstScaledImg->au32Stride[0],
			pstScaledImg->au32Stride[1],
			pstScaledImg->au32Stride[2]);

    IVE_HANDLE hIve;
    IVE_RESIZE_CTRL_S stIveCtrl;
    stIveCtrl.u16Num = 1;
    SC_BOOL bInstance = SC_TRUE;
    s32Ret = SC_MPI_IVE_Resize(&hIve, &src, &scaled, &stIveCtrl, bInstance);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_IVE_Resize err %d!\n", s32Ret);
    }

    SC_BOOL bBlock = SC_TRUE;
    SC_BOOL bFinish;
    SC_S32 ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    while (SC_ERR_IVE_QUERY_TIMEOUT == ret)
    {
        usleep(100);
        ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    }

//#define __SAMPLE_VDEC_CSC
#ifdef __SAMPLE_VDEC_CSC
    IVE_SRC_IMAGE_S dst = {0};
    IVE_SRC_IMAGE_S *pstDstImg = &dst;

    pstDstImg->u32Width = w;
    pstDstImg->u32Height = h;
    pstDstImg->enType = IVE_IMAGE_TYPE_U8C3_PACKAGE;
    s32Ret = SAMPLE_COMM_IVE_CreateImage(pstDstImg, pstDstImg->enType, w, h);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_CreateImage err %d!\n", s32Ret);
    }

    IVE_CSC_CTRL_S stIveCscCtrl;
    stIveCscCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;
    s32Ret = SC_MPI_IVE_CSC(&hIve, pstScaledImg, pstDstImg, &stIveCscCtrl, bInstance);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_IVE_csc err %d!\n", s32Ret);
    }

    ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    while (SC_ERR_IVE_QUERY_TIMEOUT == ret)
    {
        usleep(100000);
        ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    }
#endif

#if 1
    SC_CHAR s8SrcName[64];
    sprintf(s8SrcName, "p420");
    SC_CHAR s8DstFileName[64] = {0};
    sprintf(s8DstFileName, "%s/IVE_Resize_from_%s_%d_%d_to_%d_%d.yuv", ".", s8SrcName, s32SrcWidth,
        s32SrcHeight, s32DstWidth, s32DstHeight);
    FILE *pDstFile = fopen(s8DstFileName, "w+");

#ifdef __SAMPLE_VDEC_CSC
    s32Ret = SAMPLE_COMM_IVE_WriteFile(pstDstImg, pDstFile);
#else
    s32Ret = SAMPLE_COMM_IVE_WriteFile(&scaled, pDstFile);
#endif
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_WriteFile err %d!\n", s32Ret);
        fclose(pDstFile);
        return -1;
    }
    fclose(pDstFile);
#endif


    IVE_MMZ_FREE(pstScaledImg->au64PhyAddr[0], pstScaledImg->au64VirAddr[0]);
#ifdef __SAMPLE_VDEC_CSC
    IVE_MMZ_FREE(dst.au64PhyAddr[0], dst.au64VirAddr[0]);
#endif

    printf("ive resize ok!\n");

    return 0;
}

static SC_VOID *SAMPLE_VDEC_Scale_Pic(SC_VOID *pArgs)
{
    VDEC_THREAD_PARAM_S *pstVdecThreadParam = (VDEC_THREAD_PARAM_S *)pArgs;
    FILE *fp = SC_NULL;
    SC_S32 s32Ret;
    VDEC_CHN_ATTR_S  stAttr;
    VIDEO_FRAME_INFO_S stVFrame;
    SC_CHAR cSaveFile[256];
    int cnt = 0;

    FILE *fout = fopen("vdec_vo.yuv", "w");
    if (fout == NULL)
    {
        SAMPLE_PRT("open file failed!\n");
        return (SC_VOID *)SC_SUCCESS;
    }

    while(scaling)
    {
        if (pstVdecThreadParam->eThreadCtrl == THREAD_CTRL_STOP)
        {
            break;
        }

        s32Ret = SC_MPI_VDEC_GetFrame(pstVdecThreadParam->s32ChnId, &stVFrame, pstVdecThreadParam->s32MilliSec);
        if (SC_SUCCESS == s32Ret && stVFrame.stVFrame.u64PhyAddr[0])
        {
            cnt++;

            //yuv_8bit_dump(&stVFrame.stVFrame, fout);

			//SAMPLE_IVE_RESZIE(&stVFrame, 304, 304);
            pr_debug("Chn%d: get pic the %d times.\n", pstVdecThreadParam->s32ChnId, cnt);
            s32Ret = SC_MPI_VDEC_ReleaseFrame(pstVdecThreadParam->s32ChnId, &stVFrame);
        }
        else
        {
            usleep(100);
        }
    }

    fclose(fout);
    pr_debug("Chn%d: get pic thread return!\n", pstVdecThreadParam->s32ChnId);
    return (SC_VOID *)SC_SUCCESS;
}

SC_S32 SAMPLE_H264_VDEC(SC_VOID)
{
    VB_CONFIG_S stVbConfig;
    SC_S32 i, s32Ret = SC_SUCCESS;
    VDEC_THREAD_PARAM_S stVdecSend[VDEC_MAX_CHN_NUM];
    SIZE_S stDispSize;
    VO_LAYER VoLayer;
    SC_U32 u32VdecChnNum, VpssGrpNum;
    VPSS_GRP VpssGrp;
    pthread_t   VdecThread[2 * VDEC_MAX_CHN_NUM];
    PIC_SIZE_E enDispPicSize;
    SAMPLE_VDEC_ATTR astSampleVdec[VDEC_MAX_CHN_NUM];
    VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_CHN_NUM];
    SAMPLE_VO_CONFIG_S stVoConfig;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    SC_BOOL abChnEnable[VPSS_MAX_CHN_NUM];
    VO_INTF_SYNC_E enIntfSync;
    VO_INTF_TYPE_E enVoIntfType;

    u32VdecChnNum = 1;
    VpssGrpNum    = u32VdecChnNum;
    /************************************************
    step1:  init SYS, init common VB(for VPSS and VO)
    *************************************************/

	enDispPicSize = PIC_1080P;
	enIntfSync    = VO_OUTPUT_1080P30;
	enVoIntfType  = VO_INTF_HDMI;
    s32Ret =  SAMPLE_COMM_SYS_GetPicSize(enDispPicSize, &stDispSize);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("sys get pic size fail for %#x!\n", s32Ret);
        goto END1;
    }

    memset(&stVbConfig, 0, sizeof(VB_CONFIG_S));
    stVbConfig.u32MaxPoolCnt             = 1;
    stVbConfig.astCommPool[0].u32BlkCnt  = 16 * u32VdecChnNum;
    stVbConfig.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(stDispSize.u32Width, stDispSize.u32Height,
            PIXEL_FORMAT_YVU_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, 0);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }

    SAMPLE_COMM_IVE_CheckIveMpiInit();

    /************************************************
    step2:  init module VB or user VB(for VDEC)
    *************************************************/
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

#if 0
    s32Ret = SAMPLE_COMM_VDEC_InitVBPool(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END2;
    }
#endif

    /************************************************
    step3:  start VDEC
    *************************************************/
    s32Ret = SAMPLE_COMM_VDEC_Start(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END3;
    }

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
    for (i = 0; i < u32VdecChnNum; i++)
    {
        snprintf(stVdecSend[i].cFileName, sizeof(stVdecSend[i].cFileName), "1920x1080.h264");
        snprintf(stVdecSend[i].cFilePath, sizeof(stVdecSend[i].cFilePath), "%s", SAMPLE_STREAM_PATH);
        stVdecSend[i].enType          = astSampleVdec[i].enType;
        stVdecSend[i].s32StreamMode   = astSampleVdec[i].enMode;
        stVdecSend[i].s32ChnId        = i;
        stVdecSend[i].s32IntervalTime = 1000;
        stVdecSend[i].u64PtsInit      = 0;
        stVdecSend[i].u64PtsIncrease  = 0;
        stVdecSend[i].eThreadCtrl     = THREAD_CTRL_START;
        stVdecSend[i].bCircleSend     = SC_TRUE;
        stVdecSend[i].s32MilliSec     = 0;
        stVdecSend[i].s32MinBufSize   = (astSampleVdec[i].u32Width * astSampleVdec[i].u32Height * 3) >> 1;
    }
    SAMPLE_COMM_VDEC_RPC_StartSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);
#if 1
    s_VdecChnNum = u32VdecChnNum;
    s_VdecSend[0] = (VDEC_THREAD_PARAM_S *)&stVdecSend;
    memcpy(s_VdecThread, VdecThread, sizeof(VdecThread));
#endif

    pthread_t pid;

    scaling = 1;
    pthread_create(&pid, NULL, SAMPLE_VDEC_Scale_Pic, &stVdecSend[0]);


#ifndef __DEBUG_RESTART
    printf("press enter twice to exit...\n");
    getchar();
    getchar();
#else
    int retries = 10;
    while (retries--)
    {
        printf("press enter to restart vdec...\n");
        getchar();

        SAMPLE_COMM_VDEC_RPC_StopSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);
        sleep(1);
        scaling = 0;
        pthread_join(pid, NULL);
        SAMPLE_COMM_VDEC_Stop(u32VdecChnNum);
        s32Ret = SAMPLE_COMM_VDEC_Start(u32VdecChnNum, &astSampleVdec[0]);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
            goto END3;
        }
        stVdecSend[0].eThreadCtrl = THREAD_CTRL_START;
        SAMPLE_COMM_VDEC_RPC_StartSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);
        scaling = 1;
        pthread_create(&pid, NULL, SAMPLE_VDEC_Scale_Pic, &stVdecSend[0]);
    }
#endif

    SAMPLE_COMM_VDEC_RPC_StopSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);

END3:
    SAMPLE_COMM_VDEC_Stop(u32VdecChnNum);

END2:
    //SAMPLE_COMM_VDEC_ExitVBPool();

END1:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}



SC_S32 SAMPLE_H264_VDEC_MULTI_CHN(SC_VOID)
{
    VB_CONFIG_S stVbConfig;
    SC_S32 i, s32Ret = SC_SUCCESS;
    VDEC_THREAD_PARAM_S stVdecSend[VDEC_MAX_CHN_NUM];
    SIZE_S stDispSize;
    VO_LAYER VoLayer;
    SC_U32 u32VdecChnNum, VpssGrpNum;
    VPSS_GRP VpssGrp;
    pthread_t   VdecThread[2 * VDEC_MAX_CHN_NUM];
    PIC_SIZE_E enDispPicSize;
    SAMPLE_VDEC_ATTR astSampleVdec[VDEC_MAX_CHN_NUM];
    VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_CHN_NUM];
    SAMPLE_VO_CONFIG_S stVoConfig;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    SC_BOOL abChnEnable[VPSS_MAX_CHN_NUM];
    VO_INTF_SYNC_E enIntfSync;
    VO_INTF_TYPE_E enVoIntfType;

    u32VdecChnNum = 2;
    VpssGrpNum    = u32VdecChnNum;
    /************************************************
    step1:  init SYS, init common VB(for VPSS and VO)
    *************************************************/

	enDispPicSize = PIC_1080P;
	enIntfSync    = VO_OUTPUT_1080P30;
	enVoIntfType  = VO_INTF_HDMI;
    s32Ret =  SAMPLE_COMM_SYS_GetPicSize(enDispPicSize, &stDispSize);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("sys get pic size fail for %#x!\n", s32Ret);
        goto END1;
    }

    memset(&stVbConfig, 0, sizeof(VB_CONFIG_S));
    stVbConfig.u32MaxPoolCnt             = 1;
    stVbConfig.astCommPool[0].u32BlkCnt  = 16 * u32VdecChnNum;
    stVbConfig.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(stDispSize.u32Width, stDispSize.u32Height,
            PIXEL_FORMAT_YVU_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, 0);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step2:  init module VB or user VB(for VDEC)
    *************************************************/
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

#if 0
    s32Ret = SAMPLE_COMM_VDEC_InitVBPool(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END2;
    }
#endif

    /************************************************
    step3:  start VDEC
    *************************************************/
    s32Ret = SAMPLE_COMM_VDEC_Start(u32VdecChnNum, &astSampleVdec[0]);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END3;
    }

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
    for (i = 0; i < u32VdecChnNum; i++)
    {
        snprintf(stVdecSend[i].cFileName, sizeof(stVdecSend[i].cFileName), "1920x1080.h264");
        snprintf(stVdecSend[i].cFilePath, sizeof(stVdecSend[i].cFilePath), "%s", SAMPLE_STREAM_PATH);
        stVdecSend[i].enType          = astSampleVdec[i].enType;
        stVdecSend[i].s32StreamMode   = astSampleVdec[i].enMode;
        stVdecSend[i].s32ChnId        = i;
        stVdecSend[i].s32IntervalTime = 1000;
        stVdecSend[i].u64PtsInit      = 0;
        stVdecSend[i].u64PtsIncrease  = 0;
        stVdecSend[i].eThreadCtrl     = THREAD_CTRL_START;
        stVdecSend[i].bCircleSend     = SC_TRUE;
        stVdecSend[i].s32MilliSec     = 0;
        stVdecSend[i].s32MinBufSize   = (astSampleVdec[i].u32Width * astSampleVdec[i].u32Height * 3) >> 1;
        stVdecSend[i].s32MinBufSize   = stVbConfig.astCommPool[0].u64BlkSize;
    }
    SAMPLE_COMM_VDEC_RPC_StartSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);
#if 1
    s_VdecChnNum = u32VdecChnNum;
    s_VdecSend[0] = (VDEC_THREAD_PARAM_S *)&stVdecSend;
    memcpy(s_VdecThread, VdecThread, sizeof(VdecThread));
#endif

    pthread_t pid[16];

    scaling = 1;
    for (i = 0; i < u32VdecChnNum; i++)
    {
        pthread_create(&pid[i], NULL, SAMPLE_VDEC_Scale_Pic, &stVdecSend[i]);
    }

#ifndef __DEBUG_RESTART
    printf("press enter twice to exit...\n");
    getchar();
    getchar();
#else
    int j;
    int retries = 100000;
    for (j = 0; j < retries; ++j)
    {

        SAMPLE_COMM_VDEC_RPC_StopSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);
        scaling = 0;
        for (i = 0; i < u32VdecChnNum; i++)
        {
            pthread_join(pid[i], NULL);
        }

        SAMPLE_COMM_VDEC_Stop(u32VdecChnNum);
        printf("###: restart VDEC for the %d times\n", j);
        s32Ret = SAMPLE_COMM_VDEC_Start(u32VdecChnNum, &astSampleVdec[0]);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
            goto END3;
        }
        SAMPLE_COMM_VDEC_RPC_StartSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);
        scaling = 1;
        for (i = 0; i < u32VdecChnNum; i++)
        {
            pthread_create(&pid[i], NULL, SAMPLE_VDEC_Scale_Pic, &stVdecSend[i]);
        }
        usleep(200 * 1000);
    }
#endif

    SAMPLE_COMM_VDEC_RPC_StopSendStream(u32VdecChnNum, &stVdecSend[0], &VdecThread[0]);

END3:
    SAMPLE_COMM_VDEC_Stop(u32VdecChnNum);

END2:
    //SAMPLE_COMM_VDEC_ExitVBPool();

END1:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}
