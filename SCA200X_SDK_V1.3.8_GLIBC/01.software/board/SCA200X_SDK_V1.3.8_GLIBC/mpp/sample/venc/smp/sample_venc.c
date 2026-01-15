/* vi: set sw=4 ts=4: */
/**
 * @file     sample_vo.c
 * @brief    vo模块的示例代码
 * @version  1.0.0
 * @since    1.0.0
 * @author
 * @date    2021-10-25 创建文件
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "sample_comm.h"

#define RES_PATH "./res/"
#define YUV_1920_1080 RES_PATH"1920_1080_p420.yuv"

#define SAMPLE_CHECK_RET(express, name) \
    do {    \
        SC_S32 Ret;     \
        Ret = express;  \
        if (Ret != SC_SUCCESS) {    \
            printf("%s failed at %s : LINE: %d with %#x!\n", name, __func__, __LINE__, Ret);    \
            SAMPLE_VENC_SYS_Exit(); \
            return Ret; \
        }   \
    } while (0)

#ifdef SC_FPGA
    #define PIC_SIZE   PIC_1080P
#else
    #define PIC_SIZE   PIC_3840x2160
#endif
#define VENC_ALIGN  64

struct init_stage
{
    int vi_inited;
    int vi_bind;
    int vi_bind_venc;
    int vpss_inited;
    int vpss_bind;
    int venc_inited;
    int get_stream;
    VB_BLK  blk;
    VB_POOL poolId;
    int fd;
};

static struct init_stage __stage =
{
    .blk = 0xFFFFFFFF,
    .poolId = 0xFFFFFFFF,
    .fd = -1,

};
static SC_BOOL abChnEnable[4] = { 0 };
static SAMPLE_VI_CONFIG_S stViConfig;
static int g_running = 1;

static VENC_GOP_MODE_E SAMPLE_VENC_GetGopMode(void)
{
    char c;
    VENC_GOP_MODE_E enGopMode = 0;

Begin_Get:

    printf("please input choose gop mode!\n");
    printf("\t 0) NORMALP.\n");
    printf("\t 1) DUALP.\n");

    while((c = getchar()) != '\n')
        switch(c)
        {
        case '0':
            enGopMode = VENC_GOPMODE_NORMALP;
            break;
        case '1':
            enGopMode = VENC_GOPMODE_DUALP;
            break;
        default:
            SAMPLE_PRT("input rcmode: %c, is invaild!\n", c);
            goto Begin_Get;
        }

    return enGopMode;
}

static SAMPLE_RC_E SAMPLE_VENC_GetRcMode(void)
{
    char c;
    SAMPLE_RC_E  enRcMode = 0;

Begin_Get:

    printf("please input choose rc mode!\n");
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");

    while((c = getchar()) != '\n')
        switch(c)
        {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
        default:
            SAMPLE_PRT("input rcmode: %c, is invaild!\n", c);
            goto Begin_Get;
        }
    return enRcMode;
}

static SC_VOID SAMPLE_VENC_SYS_Exit(void)
{
    SAMPLE_COMM_SYS_Exit();
}

void SAMPLE_VENC_HandleSig(SC_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
		g_running = 0;
        SAMPLE_COMM_VENC_StopGetStream();

        if (__stage.fd != -1)
            close(__stage.fd);

        SAMPLE_COMM_SYS_Exit();

        SAMPLE_PRT("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }

    exit(-1);
}

SC_S32 SAMPLE_VENC_SYS_Init(SAMPLE_SNS_TYPE_E  enSnsType)
{
    SC_S32 s32Ret;
    SC_U64 u64BlkSize;
    VB_CONFIG_S stVbConf;
    PIC_SIZE_E enSnsSize;
    SIZE_S stSnsSize;

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

    SAMPLE_CHECK_RET(SAMPLE_COMM_SYS_Init(&stVbConf), "SAMPLE_COMM_SYS_Init");
    return SC_SUCCESS;
}

static SC_S32 SAMPLE_VENC_VI_Init( SAMPLE_VI_CONFIG_S *pstViConfig, SC_BOOL bLowDelay, SC_U32 u32SupplementConfig)
{
    SC_S32              s32Ret;
    SAMPLE_SNS_TYPE_E   enSnsType;
    SC_U32              u32FrameRate;

    enSnsType = pstViConfig->astViInfo[0].stSnsInfo.enSnsType;

    pstViConfig->as32WorkingViId[0]                 = 0;
    //pstViConfig->s32WorkingViNum                  = 1;
    pstViConfig->astViInfo[0].stSnsInfo.MipiDev     = SAMPLE_COMM_VI_GetComboDevBySensor(
            pstViConfig->astViInfo[0].stSnsInfo.enSnsType, 0);
    //pstViConfig->astViInfo[0].stDevInfo.ViDev     = ViDev0;
    pstViConfig->astViInfo[0].stDevInfo.enWDRMode   = WDR_MODE_NONE;

    if(SC_TRUE == bLowDelay)
        pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    else
        pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    s32Ret = SAMPLE_VENC_SYS_Init(enSnsType);
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
    //pstViConfig->astViInfo[0].stChnInfo.enPixFormat        = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
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

    #if 0
    s32Ret = SC_MPI_ISP_GetCtrlParam(pstViConfig->astViInfo[0].stPipeInfo.aPipe[0], &stIspCtrlParam);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_ISP_GetCtrlParam failed with %d!\n", s32Ret);
        return s32Ret;
    }
    stIspCtrlParam.u32StatIntvl  = u32FrameRate / 30;

    s32Ret = SC_MPI_ISP_SetCtrlParam(pstViConfig->astViInfo[0].stPipeInfo.aPipe[0], &stIspCtrlParam);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_ISP_SetCtrlParam failed with %d!\n", s32Ret);
        return s32Ret;
    }
    #endif

    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_COMM_SYS_Exit();
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi failed with %d!\n", s32Ret);
        return s32Ret;
    }

    return SC_SUCCESS;
}

static SC_S32 SAMPLE_VENC_VPSS_Init(VPSS_GRP VpssGrp, SC_BOOL *pabChnEnable, DYNAMIC_RANGE_E enDynamicRange,
    PIXEL_FORMAT_E enPixelFormat, SIZE_S stSize[], SAMPLE_SNS_TYPE_E enSnsType)
{
    SC_S32 i;
    SC_S32 s32Ret;
    PIC_SIZE_E      enSnsSize;
    SIZE_S          stSnsSize;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

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

    stVpssGrpAttr.enDynamicRange = enDynamicRange;
    stVpssGrpAttr.enPixelFormat  = enPixelFormat;
    stVpssGrpAttr.u32MaxW        = stSnsSize.u32Width;
    stVpssGrpAttr.u32MaxH        = stSnsSize.u32Height;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.bNrEn = SC_TRUE;

    for(i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if(SC_TRUE == pabChnEnable[i])
        {
            stVpssChnAttr[i].u32Width                     = stSize[i].u32Width;
            stVpssChnAttr[i].u32Height                    = stSize[i].u32Height;
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

    printf("############################# visize: %d,%d.   vpssdst: %d,%d\n", stSnsSize.u32Width, stSnsSize.u32Height,
        stSize[0].u32Width, stSize[0].u32Height);
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, pabChnEnable, &stVpssGrpAttr, stVpssChnAttr);
    if(s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
    }

    return s32Ret;
}

static SC_S32 SAMPLE_VENC_CheckSensor(SAMPLE_SNS_TYPE_E   enSnsType, SIZE_S  stSize)
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

/******************************************************************************
* function:  encode file stream of 1080p h264
******************************************************************************/
static SC_S32 frameInit(VIDEO_FRAME_INFO_S *stFrame, VB_BLK blk, SC_S32 width, SC_S32 height)
{
    stFrame->stVFrame.u32Width = width;
    stFrame->stVFrame.u32Height = height;
    stFrame->stVFrame.u64PhyAddr[0] = SC_MPI_VB_Handle2PhysAddr(blk);
    stFrame->stVFrame.u64PhyAddr[1] = stFrame->stVFrame.u64PhyAddr[0] + width * height;
    stFrame->stVFrame.u64PhyAddr[2] = stFrame->stVFrame.u64PhyAddr[1] + width * height / 4;
    stFrame->stVFrame.u64HeaderPhyAddr[0] = 0;
    stFrame->stVFrame.u64HeaderPhyAddr[1] = 0;
    stFrame->stVFrame.u64HeaderPhyAddr[2] = 0;
    stFrame->stVFrame.u64PTS = -1;
    stFrame->stVFrame.u32Stride[0] = width;
    stFrame->stVFrame.u32Stride[1] = width/2;
    stFrame->stVFrame.u32Stride[2] = width/2;
    stFrame->stVFrame.enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    return 0;
}

static SC_S32 SendOneFrame(SC_S32 id, int fd, SC_S32 width, SC_S32 height)
{
    SC_S32 s32Ret;
    VIDEO_FRAME_INFO_S stFrame;
    VB_BLK blk;
    SC_S32 yuvFrameSize;
    SC_VOID *frameVirtAddr;
    VB_POOL poolId;

    yuvFrameSize = width * height * 3 / 2;

    blk = SC_MPI_VB_GetBlock(POOL_OWNER_COMMON, yuvFrameSize, NULL);
    if (blk == VB_INVALID_HANDLE)
    {
        printf("get block failed\n");
        return blk;
    }
    memset(&stFrame, 0, sizeof(stFrame));
    __stage.blk = blk;

    s32Ret = frameInit(&stFrame, blk, width, height);
    if (s32Ret)
    {
        printf("Frame init failed\n");
        return s32Ret;
    }

    poolId = SC_MPI_VB_Handle2PoolId(blk);
    if (poolId < 0)
    {
        printf("get pool id failed\n");
        goto out;
    }

    s32Ret = SC_MPI_VB_MmapPool(poolId);
    if (s32Ret)
    {
        printf("mmap pool failed\n");
        goto out;
    }
    __stage.poolId = poolId;

    s32Ret = SC_MPI_VB_GetBlockVirAddr(poolId, stFrame.stVFrame.u64PhyAddr[0], &frameVirtAddr);
    if (s32Ret)
    {
        printf("get frame virtual address failed(%x)\n", s32Ret);
        goto out;
    }

retry:
	s32Ret = read(fd, frameVirtAddr, yuvFrameSize);
	if (s32Ret != yuvFrameSize)
	{
		if (0)
		{
			SC_MPI_VB_ReleaseBlock(blk);
			SC_MPI_VB_MunmapPool(poolId);
			printf("Read Over......\n");
			return s32Ret;
		}
		else
		{
			lseek(fd, 0, SEEK_SET);
			goto retry;
		}
	}

    s32Ret = SC_MPI_VENC_SendFrame(id, &stFrame, -1);
    if (s32Ret)
    {
        printf("send frame failed\n");
        goto out;
    }

    s32Ret = SC_MPI_VB_ReleaseBlock(blk);
    if (s32Ret)
    {
        printf("release yuv frame block failed\n");
        goto out;
    }

    __stage.blk = 0xFFFFFFFF;
    s32Ret = SC_MPI_VB_MunmapPool(poolId);
    if (s32Ret)
    {
        printf("munmap pool failed\n");
        goto out;
    }
    __stage.poolId = 0xFFFFFFFF;

    return 0;
out:
    s32Ret = SC_MPI_VB_ReleaseBlock(blk);
    if (s32Ret)
        printf("release block failed\n");

    return s32Ret;
}

SC_S32 SAMPLE_VENC_VB_Init(PIC_SIZE_E enSize)
{
    SC_S32 s32Ret;
    SC_U64 u64BlkSize;
    VB_CONFIG_S stVbConf;
    SIZE_S stSize;

    s32Ret = SC_MPI_VB_Exit();
    if (s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VB_Exit failed(%#x)\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSize, &stSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed(%#x)!\n", s32Ret);
        return s32Ret;
    }

    u64BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);

    memset(&stVbConf, 0, sizeof(stVbConf));
    stVbConf.astCommPool[0].u64BlkSize   = u64BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt    = 15;

    SAMPLE_CHECK_RET(SAMPLE_COMM_SYS_Init(&stVbConf), "SAMPLE_COMM_SYS_Init");
    return SC_SUCCESS;

}

extern SC_CHAR    *Path_BMP;

SC_S32 SAMPLE_VENC_FILE_H264(void)
{
    SC_S32 s32Ret;

    SC_U32          u32Profile[2] = {0, 0};
    VENC_CHN        VencChn[2]    = {0, 1};
    VENC_GOP_MODE_E enGopMode;
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_RC_E     enRcMode;

    enRcMode = SAMPLE_VENC_GetRcMode();
    enGopMode = SAMPLE_VENC_GetGopMode();

    s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Get GopAttr for %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    /******************************************
     1. VB Init
    ******************************************/
    s32Ret = SAMPLE_VENC_VB_Init(PIC_1080P);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Vb Init failed(%#x)!\n", s32Ret);
        return SC_FAILURE;
    }

    /******************************************
     2. Create Venc channel
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], PT_H264, PIC_1080P,
            enRcMode, u32Profile[0], &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT_VB;
    }


#if 0
    #define  vi_chn_0_bmp  "./res/vi_chn_0.bmp"
    SC_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;
    SC_S32             MinHandle;
    int                i;
    HandleNum = 8;
    enType = OVERLAY_RGN;
    Path_BMP = vi_chn_0_bmp;
    s32Ret = SAMPLE_COMM_REGION_Create(HandleNum, enType);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_Create failed!\n");
        assert(0);
    }

    stChn.enModId = SC_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    s32Ret = SAMPLE_COMM_REGION_AttachToChn(HandleNum, enType, &stChn);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
        assert(0);
    }

    MinHandle = SAMPLE_COMM_REGION_GetMinHandle(enType);

    if(OVERLAY_RGN == enType || OVERLAYEX_RGN == enType)
    {
        for(i = MinHandle; i < MinHandle + HandleNum; i++)
        {

            s32Ret = SAMPLE_COMM_REGION_SetBitMap(i);
            //s32Ret = SAMPLE_COMM_REGION_GetUpCanvas(i);
            if(SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_REGION_SetBitMap failed!\n");
                assert(0);
            }
        }
    }
#endif

    /******************************************
     stream venc process
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, 1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto EXIT_VENC_H264_STOP;
    }

    int fd = open(YUV_1920_1080, O_RDONLY);
    if (fd < 0)
    {
        printf("open yuv file failed!");
        goto EXIT_VENC_H264_STOP;
    }

    __stage.fd = fd;
    for (int i = 0; i < 1000000; i++)
    {
        s32Ret = SendOneFrame(VencChn[0], fd, 1920, 1080);
        if (s32Ret)
        {
            close(fd);
            __stage.fd = -1;
            goto EXIT_VENC_H264_STOP;
        }

		printf("Send frame: the %06d times\n", i + 1);
        usleep(1000 / 30 * 1000);
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     exit process
    ******************************************/
EXIT_VENC_H264_STOP:
    SAMPLE_COMM_VENC_StopGetStream();
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT_VB:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

SC_S32 SAMPLE_VENC_H265(void)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_U32             u32Align       = VENC_ALIGN;

    VENC_CHN        VencChn[2]    = {0, 1};

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

    SAMPLE_PRT("___ SENSOR0_TYPE:%d sensor:%d 307:%d 415:%d w:%d h:%d \n",
        SENSOR0_TYPE,
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType,
        SONY_IMX307_MIPI_2M_30FPS_12BIT,
        SONY_IMX415_MIPI_8M_30FPS_12BIT,
        stSize.u32Width, stSize.u32Height);

    /*config vb*/
    memset_s(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 8;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 8;

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

    /*config venc*/
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], PT_H264, enPicSize,
            SAMPLE_RC_CBR, 1, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VENC(ViPipe, ViChn, VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, 1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto EXIT3;
    }
    #if 0
    printf("######### press ENTER to switch to h264\n");
    getchar();

    const int loops = 10;
    int cnt = loops;
    while (cnt--)
    {
        printf("**** for times %d:\n", loops - cnt);
		getchar();
        SAMPLE_COMM_VENC_StopGetStream();
        SAMPLE_COMM_VI_UnBind_VENC(ViPipe, ViChn, VencChn[0]);
        SAMPLE_COMM_VENC_Stop(VencChn[0]);
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], PT_H264, enPicSize,
                SAMPLE_RC_CBR, 0, &stGopAttr);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
            goto EXIT1;
        }
        /*vi bind vo*/
        s32Ret = SAMPLE_COMM_VI_Bind_VENC(ViPipe, ViChn, VencChn[0]);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("vi bind venc failed. s32Ret: 0x%x !\n", s32Ret);
            goto EXIT2;
        }

        s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, 1);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto EXIT3;
        }
    }
    #endif

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VENC(ViPipe, ViChn, VencChn[0]);
EXIT2:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    SAMPLE_PRT("%s: Exit!\n", __func__);
    return s32Ret;
}

SC_S32 SAMPLE_VENC_2CHN(void)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_U32             u32Align       = VENC_ALIGN;

    VENC_CHN        VencChn[3]    = {0, 1, 2};

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

    SAMPLE_PRT("___ SENSOR0_TYPE:%d sensor:%d 307:%d 415:%d w:%d h:%d \n",
        SENSOR0_TYPE,
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType,
        SONY_IMX307_MIPI_2M_30FPS_12BIT,
        SONY_IMX415_MIPI_8M_30FPS_12BIT,
        stSize.u32Width, stSize.u32Height);

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
        goto EXIT_SYS;
    }

    printf("w=%d,h=%d\n", stSize.u32Width, stSize.u32Height);

    int VpssGrp = 0;
    int s32ChnNum = 1;
    for (int i = 0; i < s32ChnNum; ++i)
    {
        abChnEnable[i] = 1;
    }

    SIZE_S stSize1[3];
    stSize1[0].u32Width = stSize.u32Width;
    stSize1[0].u32Height = stSize.u32Height;
    stSize1[1].u32Width = 1280;
    stSize1[1].u32Height = 720;
    stSize1[2].u32Width = 704;
    stSize1[2].u32Height = 480;

    PIC_SIZE_E ps[3] = {enPicSize, PIC_720P, PIC_D1_NTSC};
    s32Ret = SAMPLE_VENC_VPSS_Init(VpssGrp, abChnEnable, DYNAMIC_RANGE_SDR8, PIXEL_FORMAT_YVU_PLANAR_420, stSize1,
            stViConfig.astViInfo[0].stSnsInfo.enSnsType);
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

    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);

    int VpssChn[3] = {0, 1, 2};
    PAYLOAD_TYPE_E payload[3] = { PT_H265, PT_H264, PT_H264 };

    for (int i = 0; i < s32ChnNum; i++)
    {
        /***encode h.265 **/
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn[i], payload[i], ps[i],
                SAMPLE_RC_VBR, 0, &stGopAttr);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
            goto EXIT_VI_VPSS_UNBIND;
        }
		if (i == 1)
		{
			VENC_RC_PARAM_S rc_param;
			printf("maxiqp = %d\n", rc_param.stParamH264Vbr.u32MaxIQp);
			printf("miniqp = %d\n", rc_param.stParamH264Vbr.u32MinIQp);
			printf("maxqp = %d\n", rc_param.stParamH264Vbr.u32MaxQp);
			printf("minqp = %d\n", rc_param.stParamH264Vbr.u32MinQp);
			SC_MPI_VENC_GetRcParam(VencChn[1], &rc_param);
			rc_param.stParamH264Vbr.u32MaxIQp = 38;
			rc_param.stParamH264Vbr.u32MinIQp = 34;
			rc_param.stParamH264Vbr.u32MaxQp = 38;
			rc_param.stParamH264Vbr.u32MinQp = 32;
			SC_MPI_VENC_SetRcParam(VencChn[1], &rc_param);
		}

        s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn[i], VencChn[i]);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Venc Get GopAttr failed for %#x!\n", s32Ret);
            goto EXIT_VENC_H265_STOP;
        }
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

#if 0
    printf("###########################################################################\n");
	//PIC_SIZE_E pic_size = PIC_3840x2160;
	PIC_SIZE_E pic_size = PIC_1080P;
    int sw = payload[0];
    for (int i = 0; i < 2000 && g_running; i++) {
#if 0
	    if (sw == PT_H265)
		    sw = PT_H264;
	    else
		    sw = PT_H265;
#else
#if 0
		if (pic_size == PIC_3840x2160)
			pic_size = PIC_720P;
		else
			pic_size = PIC_3840x2160;
#else
		if (pic_size == PIC_1080P)
			pic_size = PIC_720P;
		else
			pic_size = PIC_1080P;
#endif
#endif

#if 1
	    printf("############ the %d times\n", i);
		usleep(100 * 1000);
#else
	    printf("please press ENTER to switch encode\n");
		getchar();
#endif

        SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn[0], VencChn[0]);
	    SAMPLE_COMM_VENC_StopGetStream();
	    SAMPLE_COMM_VENC_Stop(VencChn[0]);


	    /***encode h.265 **/
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], sw, pic_size,
			    SAMPLE_RC_CBR, 0, &stGopAttr);
	    if (SC_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
		    goto EXIT_VI_VPSS_UNBIND;
	    }
	    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, s32ChnNum);
	    if (SC_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Venc failed!\n");
		    goto EXIT_VENC_H264_UnBind;
	    }

        s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn[0], VencChn[0]);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Venc Get GopAttr failed for %#x!\n", s32Ret);
            goto EXIT_VENC_H265_STOP;
        }
    }
#endif

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

EXIT_VENC_H264_UnBind_1:
    for (int i = 0; i < s32ChnNum; i++)
    {
        SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn[i], VencChn[i]);
        SAMPLE_COMM_VENC_Stop(VencChn[i]);
    }
EXIT_VENC_H264_UnBind:
EXIT_VENC_H264_STOP:
EXIT_VENC_H265_UnBind:
EXIT_VENC_H265_STOP:
EXIT_VI_VPSS_UNBIND:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT_VPSS_STOP:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT_SYS:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

SC_S32 SAMPLE_VENC_Jpeg(void)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_U32             u32Align       = VENC_ALIGN;

    VENC_CHN        VencChn[2]    = {0, 1};

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

    SAMPLE_PRT("___ SENSOR0_TYPE:%d sensor:%d 307:%d 415:%d w:%d h:%d \n",
        SENSOR0_TYPE,
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType,
        SONY_IMX307_MIPI_2M_30FPS_12BIT,
        SONY_IMX415_MIPI_8M_30FPS_12BIT,
        stSize.u32Width, stSize.u32Height);

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

    int VpssGrp = 0;

    abChnEnable[0] = 1;
    VPSS_CHN VpssChn[2]    = {0, 1};

    stSize.u32Width = 1280;
    stSize.u32Height = 720;
    s32Ret = SAMPLE_VENC_VPSS_Init(VpssGrp, abChnEnable, DYNAMIC_RANGE_SDR8, PIXEL_FORMAT_YVU_PLANAR_420, &stSize,
            stViConfig.astViInfo[0].stSnsInfo.enSnsType);
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

    /*config venc*/
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], PT_JPEG, enPicSize,
            SAMPLE_RC_CBR, 0, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT1;
    }

	VENC_JPEG_PARAM_S stJpegParam;
	SC_MPI_VENC_GetJpegParam(VencChn[0], &stJpegParam);
	stJpegParam.u32Qfactor = 9;
	SC_MPI_VENC_SetJpegParam(VencChn[0], &stJpegParam);

	VENC_CHN_PARAM_S stChnParam;
	SC_MPI_VENC_GetChnParam(VencChn[0], &stChnParam);
	stChnParam.stFrameRate.s32SrcFrmRate = -1;
	stChnParam.stFrameRate.s32DstFrmRate = 2;
	SC_MPI_VENC_SetChnParam(VencChn[0], &stChnParam);

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn[0], VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Get GopAttr failed for %#x!\n", s32Ret);
        goto EXIT_VENC_STOP;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, 1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto EXIT3;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT3:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn[0], VencChn[0]);
EXIT_VENC_STOP:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT1:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT_VPSS_STOP:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    SAMPLE_PRT("%s: Exit!\n", __func__);
    return s32Ret;
}

SC_S32 SAMPLE_VENC_MODPARAM(void)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_U32             u32Align       = VENC_ALIGN;

    VENC_CHN        VencChn[2]    = {0, 1};

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

    SAMPLE_PRT("___ SENSOR0_TYPE:%d sensor:%d 307:%d 415:%d w:%d h:%d \n",
        SENSOR0_TYPE,
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType,
        SONY_IMX307_MIPI_2M_30FPS_12BIT,
        SONY_IMX415_MIPI_8M_30FPS_12BIT,
        stSize.u32Width, stSize.u32Height);

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

	VENC_PARAM_MOD_S stModParam = { 0 };
    stModParam.enVencModType = MODTYPE_H265E;
	s32Ret = SC_MPI_VENC_GetModParam(&stModParam);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_GetModParam failed:0x%x !\n", s32Ret);
        goto EXIT;
    }
	printf("###:%d,%d\n", stModParam.stH265eModParam.u32CoreClock,
	stModParam.stH265eModParam.u32BpuClock);
    //stModParam.enVencModType = MODTYPE_H265E;

    /* RW; Range:{75, 150, 200, 250, 300, 360, 400, 450, 500, 600, 666, 700} Mhz note:H264与H265的编码和解码共用 */
	stModParam.stH265eModParam.u32CoreClock = 666;
	stModParam.stH265eModParam.u32BpuClock =  600;
	s32Ret = SC_MPI_VENC_SetModParam(&stModParam);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_SetModParam failed:0x%x !\n", s32Ret);
        goto EXIT;
    }


    /*config venc*/
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], PT_H265, enPicSize,
            SAMPLE_RC_CBR, 0, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VENC(ViPipe, ViChn, VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, 1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto EXIT3;
    }

    printf("please press ENTER to switch to  u32CoreClock=200\n");
	getchar();
	stModParam.stH265eModParam.u32CoreClock = 200;
	stModParam.stH265eModParam.u32BpuClock =  360;
	s32Ret = SC_MPI_VENC_SetModParam(&stModParam);
	if (SC_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SC_MPI_VENC_SetModParam failed:0x%x !\n", s32Ret);
		goto EXIT;
	}

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VENC(ViPipe, ViChn, VencChn[0]);
EXIT2:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    SAMPLE_PRT("%s: Exit!\n", __func__);
    return s32Ret;
}

SC_S32 SAMPLE_VENC_SET_CHN_ATTR(void)
{
    SC_S32             s32Ret;

    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    SC_U32             u32BlkSize;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    SC_U32             u32Align       = VENC_ALIGN;

    VENC_CHN        VencChn[2]    = {0, 1};

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

    SAMPLE_PRT("___ SENSOR0_TYPE:%d sensor:%d 307:%d 415:%d w:%d h:%d \n",
        SENSOR0_TYPE,
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType,
        SONY_IMX307_MIPI_2M_30FPS_12BIT,
        SONY_IMX415_MIPI_8M_30FPS_12BIT,
        stSize.u32Width, stSize.u32Height);

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

    /*config venc*/
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], PT_H264, enPicSize,
            SAMPLE_RC_VBR, 0, &stGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VENC(ViPipe, ViChn, VencChn[0]);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, 1);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto EXIT3;
    }
	while (1)
	{
		int k;
		int fps, bitrate;
    	VENC_CHN_ATTR_S stChnAttr;

#if 0
    	printf("Choose options:\n");
    	printf("0: Quit\n");
    	printf("1: Set bitrate and framerate\n");

		fscanf(stdin, "%d", &k);
		if (k == 0)
			break;
#endif

        s32Ret = SC_MPI_VENC_GetChnAttr(VencChn[0], &stChnAttr);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SC_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", \
                VencChn[0], s32Ret);
			break;
        }
#if 0
    	printf("Enter bitrate:");
		fscanf(stdin, "%d", &bitrate);
    	printf("Enter framerate:");
		fscanf(stdin, "%d", &fps);
#endif

		stChnAttr.stRcAttr.stH265Vbr.u32MaxBitRate = 4096;
		stChnAttr.stRcAttr.stH265Vbr.u32SrcFrameRate = -1;
		stChnAttr.stRcAttr.stH265Vbr.fr32DstFrameRate = 25;
		s32Ret = SC_MPI_VENC_SetChnAttr(VencChn[0], &stChnAttr);
		if (SC_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("set venc chn attr failed. s32Ret: 0x%x !\n", s32Ret);
			break;
		}
		sleep(2);
	}

    SAMPLE_COMM_VENC_StopGetStream();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VENC(ViPipe, ViChn, VencChn[0]);
EXIT2:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    SAMPLE_PRT("%s: Exit!\n", __func__);
    return s32Ret;
}
