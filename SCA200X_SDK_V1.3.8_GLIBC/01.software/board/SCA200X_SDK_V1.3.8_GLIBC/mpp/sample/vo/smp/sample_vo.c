/**
 * @file     sample_vo.c
 * @brief    vo模块的示例代码
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
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
#include "sc_mipi_tx.h"
#include "sample_vo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define RES_PATH "./res/"

#define YUV_1920_1080 RES_PATH"1920_1080_p420.yuv"

SC_S32 all_exit = SC_FALSE;
VO_LAYER Dy_VoLayer = 0;
VO_CHN Dy_VoChn = 0;

typedef struct stSAMPLE_VOU_ThreadCtrl_Info
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
    SC_BOOL          abToChn[VO_MAX_CHN_NUM];
    COLOR_GAMUT_E    enColrGamut;
    SC_U32           u32ChnNum;

    pthread_t        tid;
} SAMPLE_VOU_ThreadCtrl_Info;

typedef struct stSAMPLE_USER_VO_CONFIG_S
{
    VO_SYNC_INFO_S stSyncInfo;
    VO_USER_INTFSYNC_ATTR_S stUserIntfSyncAttr;
    SC_U32 u32PreDiv;
    SC_U32 u32DevDiv;
    SC_U32 u32Framerate;
    combo_dev_cfg_t stcombo_dev_cfgl;
} SAMPLE_USER_VO_CONFIG_S;

SAMPLE_VI_CONFIG_S stViConfig = {0};
SAMPLE_VO_CONFIG_S stVoConfig = {0};
SAMPLE_VOU_ThreadCtrl_Info stThreadInfo = {0};
#define ALIGN_BACK(x, a)        ((a) * (((x + a -1) / (a))))

#define SAMPLE_CHECK_RET(express,name)\
    do {\
        SC_S32 Ret;\
        Ret = express;\
        if (Ret != SC_SUCCESS) {\
            printf("%s failed at %s : LINE: %d with %#x!\n",name, __FUNCTION__,__LINE__,Ret);\
            SAMPLE_VOU_SYS_Exit();\
            return Ret;\
            }\
        }while(0)

#define VO_VB_PIC_BLK_SIZE(Width, Height, Type, size)\
    do{\
            unsigned int u32AlignWidth;\
            unsigned int u32AlignHeight;\
            unsigned int u32HeadSize;\
            u32AlignWidth = ALIGN_UP(Width, 16);\
            u32AlignHeight= ALIGN_UP(Height, 2);\
            u32HeadSize = 16 * u32AlignHeight;/* compress header stride 16 */\
            if (Type == PIXEL_FORMAT_YVU_SEMIPLANAR_422)\
            {\
                size = (u32AlignWidth * u32AlignHeight + u32HeadSize) * 2;\
            }\ else if (Type == PIXEL_FORMAT_YUV_400)\
            {\
                size = (u32AlignWidth * u32AlignHeight + u32HeadSize);\
            }\
            else\
            {\
                size = ((u32AlignWidth * u32AlignHeight + u32HeadSize) * 3) >> 1;\
            }\
    }while(0)

SC_S32 SAMPLE_VOU_SYS_Init(void)
{
    VB_CONFIG_S stVbConf = {0};
    SC_U32      u32BlkSize;

    SC_MPI_VO_Exit();

    SC_MPI_SYS_Exit();

    SC_MPI_VB_Exit();

    stVbConf.u32MaxPoolCnt = 2;

    u32BlkSize = COMMON_GetPicBufferSize(1920, 1080, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = COMMON_GetPicBufferSize(720, 576, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 6;

    SAMPLE_CHECK_RET(SAMPLE_COMM_SYS_Init(&stVbConf), "SAMPLE_COMM_SYS_Init");

    return SC_SUCCESS;
}

SC_VOID SAMPLE_VOU_SYS_Exit(void)
{
    SC_MPI_VO_Exit();
    SC_MPI_SYS_Exit();
    SC_MPI_VB_Exit();
}

SC_VOID SAMPLE_VIO_Stop(void)
{
    if (stViConfig.s32WorkingViNum)
    {
        SAMPLE_COMM_VI_UnBind_VO(0, 0, stVoConfig.VoDev, 0);
        SAMPLE_COMM_VO_StopVO(&stVoConfig);
        SAMPLE_COMM_VI_StopVi(&stViConfig);
    }
}

void SAMPLE_VO_HandleSig(SC_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_VO_StopUserThd(&stThreadInfo);
        all_exit = SC_TRUE;
        SAMPLE_VIO_Stop();
        SAMPLE_VOU_SYS_Exit();
        SAMPLE_PRT("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

SC_S32 SAMPLE_VOU_ReadOneFrame( FILE *fp, SC_U8 *pY, SC_U8 *pU, SC_U8 *pV,
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

SC_S32 SAMPLE_VO_PlanToSemi(SC_U8 *pY, SC_S32 yStride,
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

SC_VOID *SAMPLE_VO_FileVO(SC_VOID *pData)
{
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

    SAMPLE_VOU_ThreadCtrl_Info *pInfo = (SAMPLE_VOU_ThreadCtrl_Info *)pData;
    VO_LAYER VoLayer = pInfo->s32ToDev;
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

    u32Size = ALIGN_BACK(u32SrcWidth, 512) * u32SrcHeight * 3;
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Size;
    stVbPoolCfg.u32BlkCnt = 20;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    Pool = SC_MPI_VB_CreatePool(&stVbPoolCfg);
    if (Pool == VB_INVALID_POOLID)
    {
        printf("Maybe you not call sys init\n");
        return SC_NULL;
    }

    stUserFrame.stVFrame.enField = VIDEO_FIELD_FRAME;
    stUserFrame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    stUserFrame.stVFrame.enPixelFormat = pInfo->enPixelFmt;
    stUserFrame.stVFrame.enVideoFormat = pInfo->enVideoFmt;
    stUserFrame.stVFrame.enColorGamut = COLOR_GAMUT_BT709;
    stUserFrame.stVFrame.u32Width = u32SrcWidth;
    stUserFrame.stVFrame.u32Height = u32SrcHeight;
    stUserFrame.stVFrame.u32Stride[0] = ALIGN_BACK(u32SrcWidth, 512);
    stUserFrame.stVFrame.u32Stride[1] = ALIGN_BACK(u32SrcWidth, 512);
    stUserFrame.stVFrame.u32Stride[2] = ALIGN_BACK(u32SrcWidth, 512);
    stUserFrame.stVFrame.u32TimeRef = 0;
    stUserFrame.stVFrame.u64PTS = 0;
    stUserFrame.stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

    u32LumaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight;
    if (pInfo->enPixelFmt == PIXEL_FORMAT_YVU_SEMIPLANAR_422)
    {
        u32ChromaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight / 2;
    }
    else if ((pInfo->enPixelFmt == PIXEL_FORMAT_YVU_SEMIPLANAR_420))
    {
        u32ChromaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight / 4;
    }
    else if ((pInfo->enPixelFmt == PIXEL_FORMAT_YVU_PLANAR_420))
    {
        u32ChromaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight / 4;
        stUserFrame.stVFrame.u32Stride[1] = ALIGN_BACK(u32SrcWidth / 2, 512);
        stUserFrame.stVFrame.u32Stride[2] = ALIGN_BACK(u32SrcWidth / 2, 512);
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
            s32Ret = SAMPLE_VOU_ReadOneFrame( pfd, (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0],
                    (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[1], (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[2],
                    stUserFrame.stVFrame.u32Width, stUserFrame.stVFrame.u32Height,
                    stUserFrame.stVFrame.u32Stride[0], stUserFrame.stVFrame.u32Stride[1] >> 1,
                    stUserFrame.stVFrame.enPixelFormat);
            if(s32Ret == SC_SUCCESS)
            {
                SAMPLE_VO_PlanToSemi( (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0], stUserFrame.stVFrame.u32Stride[0],
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
            s32Ret = SAMPLE_VOU_ReadOneFrame( pfd, (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0],
                    (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[1], (SC_U8 *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[2],
                    stUserFrame.stVFrame.u32Width, stUserFrame.stVFrame.u32Height,
                    stUserFrame.stVFrame.u32Stride[0], stUserFrame.stVFrame.u32Stride[1],
                    stUserFrame.stVFrame.enPixelFormat);
        }

        stUserFrame.stVFrame.u64PTS += 40000;
        stUserFrame.stVFrame.u32TimeRef += 40000;

        for (i = 0; i < pInfo->u32ChnNum; i++)
        {
            s32Ret = SC_MPI_VO_SendFrame(VoLayer, i, &stUserFrame, 0);
        }

OUT:
        SC_MPI_VB_ReleaseBlock(hBlkHdl);
        SC_MPI_SYS_Munmap((SC_VOID *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0], u32Size);

    } while(pInfo->bQuit == SC_FALSE);

    while (pInfo->bDestroy == SC_FALSE)
    {
        ;
    }

    fclose(pfd);

    return NULL;
}

SC_VOID SAMPLE_VO_GetBaseThreadInfo(SAMPLE_VOU_ThreadCtrl_Info *pstThreadInfo, SIZE_S *pstFrmSize)
{
    pstThreadInfo->bDestroy = SC_FALSE;
    pstThreadInfo->bQuit    = SC_FALSE;
    pstThreadInfo->enColrGamut = COLOR_GAMUT_BT709;
    pstThreadInfo->enPixelFmt = PIXEL_FORMAT_YVU_PLANAR_420;
    pstThreadInfo->enVideoFmt = VIDEO_FORMAT_LINEAR;
    pstThreadInfo->u32Width = pstFrmSize->u32Width;
    pstThreadInfo->u32Height = pstFrmSize->u32Height;

    return;
}

SC_VOID SAMPLE_VO_StartUserThd(SAMPLE_VOU_ThreadCtrl_Info *pstThreadInfo, SC_S32 VoLayer, SC_S32 VoChnNum,
    SC_CHAR filename[256], SIZE_S *pstFrmSize)
{
    /* CREATE USER THREAD */
    SAMPLE_VO_GetBaseThreadInfo(pstThreadInfo, pstFrmSize);

    pstThreadInfo->s32ToDev = VoLayer;
    pstThreadInfo->u32ChnNum = VoChnNum;

    strncpy(pstThreadInfo->filename, filename, sizeof(pstThreadInfo->filename) - 1);
    pthread_create(&pstThreadInfo->tid, NULL, SAMPLE_VO_FileVO, (SC_VOID *)pstThreadInfo);

    return;
}

SC_VOID SAMPLE_VO_StopUserThd(SAMPLE_VOU_ThreadCtrl_Info *pstThdInfo)
{
    pstThdInfo->bQuit = SC_TRUE;
    pstThdInfo->bDestroy = SC_TRUE;
    if (pstThdInfo->tid)
    {
        pthread_join(pstThdInfo->tid, SC_NULL);
    }

    return;
}

/******************************************************************************
* function :  FILE + MIPILCD (1024x600)
******************************************************************************/
SC_S32 SAMPLE_VO_MIPILCD_1024x600(SC_VOID)
{
    SC_U32 i = 0;
    SC_S32 VoDev = 0;
    SC_S32 VoLayer = 0;
    SC_S32 VoChnNum = 1;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo;
    SC_U32 u32Framerate;
    SIZE_S stDevSize;

    VO_CHN_ATTR_S astChnAttr[VO_MAX_CHN_NUM];

    SC_CHAR filename[256] = YUV_1920_1080;
    SIZE_S stFrameSize    = {1920, 1080};

    SAMPLE_CHECK_RET(SAMPLE_VOU_SYS_Init(), "SAMPLE_VOU_SYS_Init");

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI;

    /* USER SET VO DEV SYNC INFO */
    stPubAttr.stSyncInfo.u16Hact = 1024;
    stPubAttr.stSyncInfo.u16Hbb = 160;
    stPubAttr.stSyncInfo.u16Hfb = 160;

    stPubAttr.stSyncInfo.u16Vact = 600;
    stPubAttr.stSyncInfo.u16Vbb = 23;
    stPubAttr.stSyncInfo.u16Vfb = 12;

    stPubAttr.stSyncInfo.u16Hpw = 8;
    stPubAttr.stSyncInfo.u16Vpw = 10;

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
    SAMPLE_CHECK_RET(SAMPLE_VO_CONFIG_MIPI(MIPILCD_1024x600), "SAMPLE_VO_CONFIG_MIPI");

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

    /* START USER THREAD */
    SAMPLE_VO_StartUserThd(&stThreadInfo, VoLayer, VoChnNum, filename, &stFrameSize);

    while (!all_exit)
    {
        usleep(10000);
    }

    /* STOP USER THREAD */
    SAMPLE_VO_StopUserThd(&stThreadInfo);

    /*DISABLE VO CHN*/
    for (i = 0; i < VoChnNum; i++)
    {
        SAMPLE_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, i), "SC_MPI_VO_DisableChn");
    }

    /* DISABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");

    /* DISABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    SAMPLE_VOU_SYS_Exit();

    return SC_SUCCESS;
}

/******************************************************************************
* function : FILE + HDMI (1920x1080)
******************************************************************************/
SC_S32 SAMPLE_VO_HDMI_1920x1080(SC_VOID)
{
    SC_U32 i = 0;
    SC_S32 VoDev = 0;
    SC_S32 VoLayer = 0;
    SC_S32 VoChnNum = 1;
    SC_U32 DisplayBufLen = 0;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    SIZE_S  stFrameSize;
    SIZE_S  stDevSize = {1920, 1080};
    SC_CHAR filename[256] = YUV_1920_1080;

    VO_CHN_ATTR_S astChnAttr[VO_MAX_CHN_NUM];

    SAMPLE_CHECK_RET(SAMPLE_VOU_SYS_Init(), "SAMPLE_VOU_SYS_Init");

    stPubAttr.u32BgColor = 0x808080;
    stPubAttr.enIntfType = VO_INTF_HDMI;
    stPubAttr.enIntfSync = VO_OUTPUT_1080P60;

    SAMPLE_CHECK_RET(SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_SetPubAttr");

    SAMPLE_CHECK_RET(SC_MPI_VO_GetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_GetPubAttr");

    #if 0
    /* ENABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Enable(VoDev), "SC_MPI_VO_Enable");

    SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    SAMPLE_CHECK_RET(SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_SetPubAttr");
    #endif

    /* ENABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Enable(VoDev), "SC_MPI_VO_Enable");

    /* SET VO DISPLAY BUFFER LENGTH */
    DisplayBufLen = 3;
    SAMPLE_CHECK_RET(SC_MPI_VO_SetDisplayBufLen(VoDev, DisplayBufLen), "SC_MPI_VO_SetDisplayBufLen");

    SAMPLE_CHECK_RET(SC_MPI_VO_GetDisplayBufLen(VoDev, &DisplayBufLen), "SC_MPI_VO_GetDisplayBufLen");

    SAMPLE_VO_GetUserLayerAttr(&stLayerAttr, &stDevSize);
    stLayerAttr.u32DispFrmRt = 30;

    SAMPLE_CHECK_RET(SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_SetVideoLayerAttr");

    SAMPLE_CHECK_RET(SC_MPI_VO_GetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_GetVideoLayerAttr");

    #if 0
    /* ENABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_EnableVideoLayer(VoLayer), "SC_MPI_VO_EnableVideoLayer");

    SAMPLE_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");

    SAMPLE_CHECK_RET(SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_SetVideoLayerAttr");
    #endif

    /* ENABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_EnableVideoLayer(VoLayer), "SC_MPI_VO_EnableVideoLayer");

    /* SET AND ENABLE VO CHN */
    SAMPLE_VO_GetUserChnAttr(astChnAttr, &stDevSize, VoChnNum);

    for (i = 0; i < VoChnNum; i++)
    {
        SAMPLE_CHECK_RET(SC_MPI_VO_SetChnAttr(VoLayer, i, &astChnAttr[i]), "SC_MPI_VO_SetChnAttr");

        SAMPLE_CHECK_RET(SC_MPI_VO_GetChnAttr(VoLayer, i, &astChnAttr[i]), "SC_MPI_VO_GetChnAttr");

        #if 0
        SAMPLE_CHECK_RET(SC_MPI_VO_EnableChn(VoLayer, i), "SC_MPI_VO_EnableChn");

        SAMPLE_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, i), "SC_MPI_VO_DisableChn");

        SAMPLE_CHECK_RET(SC_MPI_VO_SetChnAttr(VoLayer, i, &astChnAttr[i]), "SC_MPI_VO_SetChnAttr");
        #endif

        SAMPLE_CHECK_RET(SC_MPI_VO_EnableChn(VoLayer, i), "SC_MPI_VO_EnableChn");

    }

    stFrameSize.u32Width = stDevSize.u32Width;
    stFrameSize.u32Height = stDevSize.u32Height;

    /* START USER THREAD */
    SAMPLE_VO_StartUserThd(&stThreadInfo, VoLayer, VoChnNum, filename, &stFrameSize);

    while (!all_exit)
    {
        usleep(10000);
    }

    /* STOP USER THREAD */
    SAMPLE_VO_StopUserThd(&stThreadInfo);

    /*DISABLE VO CHN*/
    for (i = 0; i < VoChnNum; i++)
    {
        SAMPLE_CHECK_RET(SC_MPI_VO_ClearChnBuf(VoLayer, i, SC_FALSE), "SC_MPI_VO_ClearChnBuf");

        SAMPLE_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, i), "SC_MPI_VO_DisableChn");
    }

    /* DISABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");

    /* DISABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    SAMPLE_VOU_SYS_Exit();

    return SC_SUCCESS;
}

/******************************************************************************
* function :  VI + HDMI (1920x1080)
******************************************************************************/
SC_S32 SAMPLE_VI_VO(void)
{
    SC_S32             s32Ret;
    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VO_INTF_TYPE_E     enVoIntfType = VO_INTF_HDMI;
    VO_CHN             VoChn          = 0;

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
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
            COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    u32BlkSize = VI_GetRawBufferSize(stRawSize.u32Width, stRawSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE,
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
        goto EXIT_VI_STOP;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    stVoConfig.enVoIntfType = enVoIntfType;
    stVoConfig.enPicSize = enSnsSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VI_STOP;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VO_STOP;
    }

    while (!all_exit)
    {
        usleep(10000);
    }

    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT_VO_STOP:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  FILE + MIPILCD (800x1280)
******************************************************************************/
SC_S32 SAMPLE_VO_MIPILCD_800x1280(SC_VOID)
{
    SC_U32 i = 0;
    SC_S32 VoDev = 0;
    SC_S32 VoLayer = 0;
    SC_S32 VoChnNum = 1;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo;
    SC_U32 u32Framerate;
    SIZE_S stDevSize;

    VO_CHN_ATTR_S astChnAttr[VO_MAX_CHN_NUM];

    SC_CHAR filename[256] = YUV_1920_1080;
    SIZE_S stFrameSize    = {1920, 1080};

    SAMPLE_CHECK_RET(SAMPLE_VOU_SYS_Init(), "SAMPLE_VOU_SYS_Init");

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

    /* START USER THREAD */
    SAMPLE_VO_StartUserThd(&stThreadInfo, VoLayer, VoChnNum, filename, &stFrameSize);


    while (!all_exit)
    {
        usleep(1000000);
    }

    /* STOP USER THREAD */
    SAMPLE_VO_StopUserThd(&stThreadInfo);

    /*DISABLE VO CHN*/
    for (i = 0; i < VoChnNum; i++)
    {
        SAMPLE_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, i), "SC_MPI_VO_DisableChn");
    }

    /* DISABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");

    /* DISABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    SAMPLE_VOU_SYS_Exit();

    return SC_SUCCESS;
}

/******************************************************************************
* function :  VI + MIPILCD (800x1280)
******************************************************************************/
SC_S32 SAMPLE_VI_MIPILCD_800x1280(void)
{
    SC_S32             i, s32Ret;
    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VO_CHN                      VoChn           = 0;
    SC_S32                      VoDev           = 0;
    SC_S32                      VoLayer         = 0;
    SC_S32                      VoChnNum        = 1;
    VO_PUB_ATTR_S               stPubAttr;
    VO_VIDEO_LAYER_ATTR_S       stLayerAttr;
    VO_USER_INTFSYNC_INFO_S     stUserInfo;
    SC_U32                      u32Framerate;
    SIZE_S                      stDevSize;
    VO_CHN_ATTR_S               astChnAttr[VO_MAX_CHN_NUM];
    VO_INTF_TYPE_E              enVoIntfType    = VO_INTF_HDMI;

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

    s32Ret = SC_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_90);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT_VI_STOP;
    }

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

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VO_STOP;
    }

    while (!all_exit)
    {
        usleep(10000);
    }

    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT_VO_STOP:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  VI + MIPILCD (480x640)
******************************************************************************/
SC_S32 SAMPLE_VI_MIPILCD_480x640(void)
{
    SC_S32             i, s32Ret;
    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VO_CHN                      VoChn           = 0;
    SC_S32                      VoDev           = 0;
    SC_S32                      VoLayer         = 0;
    SC_S32                      VoChnNum        = 1;
    VO_PUB_ATTR_S               stPubAttr;
    VO_VIDEO_LAYER_ATTR_S       stLayerAttr;
    VO_USER_INTFSYNC_INFO_S     stUserInfo;
    SC_U32                      u32Framerate;
    SIZE_S                      stDevSize;
    VO_CHN_ATTR_S               astChnAttr[VO_MAX_CHN_NUM];
    VO_INTF_TYPE_E              enVoIntfType    = VO_INTF_HDMI;

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

    s32Ret = SC_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_90);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT_VI_STOP;
    }

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI;

    /* USER SET VO DEV SYNC INFO */
     stPubAttr.stSyncInfo.u16Hact = 480;
    stPubAttr.stSyncInfo.u16Hbb = 20;
    stPubAttr.stSyncInfo.u16Hfb = 20;

    stPubAttr.stSyncInfo.u16Vact = 640;
    stPubAttr.stSyncInfo.u16Vbb = 40;
    stPubAttr.stSyncInfo.u16Vfb = 40;

    stPubAttr.stSyncInfo.u16Hpw = 10;
    stPubAttr.stSyncInfo.u16Vpw = 20;
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
    SAMPLE_CHECK_RET(SAMPLE_VO_CONFIG_MIPI(MIPILCD_480x640), "SAMPLE_VO_CONFIG_MIPI");

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

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VO_STOP;
    }


    while (!all_exit)
    {
        usleep(10000);
    }

    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT_VO_STOP:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  FILE + MIPILCD (480x640)
******************************************************************************/
SC_S32 SAMPLE_VO_MIPILCD_480x640(SC_VOID)
{
    SC_U32 i = 0;
    SC_S32 VoDev = 0;
    SC_S32 VoLayer = 0;
    SC_S32 VoChnNum = 1;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo;
    SC_U32 u32Framerate;
    SIZE_S stDevSize;

    VO_CHN_ATTR_S astChnAttr[VO_MAX_CHN_NUM];

    SC_CHAR filename[256] = YUV_1920_1080;
    SIZE_S stFrameSize    = {1920, 1080};

   system("./scd_init480x640.sh");
restart:

    SAMPLE_CHECK_RET(SAMPLE_VOU_SYS_Init(), "SAMPLE_VOU_SYS_Init");

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI;

    /* USER SET VO DEV SYNC INFO */
    stPubAttr.stSyncInfo.u16Hact = 480;
    stPubAttr.stSyncInfo.u16Hbb = 20;
    stPubAttr.stSyncInfo.u16Hfb = 20;

    stPubAttr.stSyncInfo.u16Vact = 640;
    stPubAttr.stSyncInfo.u16Vbb = 40;
    stPubAttr.stSyncInfo.u16Vfb = 40;

    stPubAttr.stSyncInfo.u16Hpw = 10;
    stPubAttr.stSyncInfo.u16Vpw = 20;

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
    SAMPLE_CHECK_RET(SAMPLE_VO_CONFIG_MIPI(MIPILCD_480x640), "SAMPLE_VO_CONFIG_MIPI");


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

    /* START USER THREAD */
    SAMPLE_VO_StartUserThd(&stThreadInfo, VoLayer, VoChnNum, filename, &stFrameSize);

    usleep(2000000);
    int ret = 0;
    static int flag = 0;

    //if (!flag)
    {
        SAMPLE_VO_StopUserThd(&stThreadInfo);

        SC_MPI_VO_DisableChn(VoLayer, 0);

        SC_MPI_VO_DisableVideoLayer(VoLayer);

        ret = SAMPLE_VO_MIPITx_Screen480x640_GetStatus();
        //flag = (ret==0)?1:0;

        if (ret)
        {

            /* DISABLE VO DEV */
            SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

            SAMPLE_VOU_SYS_Exit();

            goto restart;
        }
        else
        {
            SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr);
            SC_MPI_VO_EnableVideoLayer(VoLayer);

            SC_MPI_VO_SetChnAttr(VoLayer, 0, &astChnAttr[0]);
            SC_MPI_VO_EnableChn(VoLayer, 0);

            SAMPLE_VO_StartUserThd(&stThreadInfo, VoLayer, VoChnNum, filename, &stFrameSize);
        }
    }

    while (!all_exit)
    {
        //SAMPLE_VO_MIPITx_Screen480x640_GetStatus();
        usleep(1000000);
    }

    /* STOP USER THREAD */
    SAMPLE_VO_StopUserThd(&stThreadInfo);

    /*DISABLE VO CHN*/
    for (i = 0; i < VoChnNum; i++)
    {
        SAMPLE_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, i), "SC_MPI_VO_DisableChn");
    }

    /* DISABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");

    /* DISABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    SAMPLE_VOU_SYS_Exit();

    return SC_SUCCESS;
}

/******************************************************************************
* function :  VI + MIPILCD (720x1280)
******************************************************************************/
SC_S32 SAMPLE_VI_MIPILCD_720x1280(void)
{
    SC_S32             i, s32Ret;
    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VO_CHN                      VoChn           = 0;
    SC_S32                      VoDev           = 0;
    SC_S32                      VoLayer         = 0;
    SC_S32                      VoChnNum        = 1;
    VO_PUB_ATTR_S               stPubAttr;
    VO_VIDEO_LAYER_ATTR_S       stLayerAttr;
    VO_USER_INTFSYNC_INFO_S     stUserInfo;
    SC_U32                      u32Framerate;
    SIZE_S                      stDevSize;
    VO_CHN_ATTR_S               astChnAttr[VO_MAX_CHN_NUM];
    VO_INTF_TYPE_E              enVoIntfType    = VO_INTF_HDMI;

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

    s32Ret = SC_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_90);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT_VI_STOP;
    }

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI;

    /* USER SET VO DEV SYNC INFO */
     stPubAttr.stSyncInfo.u16Hact = 480;
    stPubAttr.stSyncInfo.u16Hbb = 20;
    stPubAttr.stSyncInfo.u16Hfb = 20;

    stPubAttr.stSyncInfo.u16Vact = 640;
    stPubAttr.stSyncInfo.u16Vbb = 40;
    stPubAttr.stSyncInfo.u16Vfb = 40;

    stPubAttr.stSyncInfo.u16Hpw = 10;
    stPubAttr.stSyncInfo.u16Vpw = 20;
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
    SAMPLE_CHECK_RET(SAMPLE_VO_CONFIG_MIPI(MIPILCD_480x640), "SAMPLE_VO_CONFIG_MIPI");

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

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VO_STOP;
    }


    while (!all_exit)
    {
        usleep(10000);
    }

    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT_VO_STOP:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  FILE + MIPILCD (720x1280)
******************************************************************************/
SC_S32 SAMPLE_VO_MIPILCD_720x1280(SC_VOID)
{
    SC_U32 i = 0;
    SC_S32 VoDev = 0;
    SC_S32 VoLayer = 0;
    SC_S32 VoChnNum = 1;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo;
    SC_U32 u32Framerate;
    SIZE_S stDevSize;

    VO_CHN_ATTR_S astChnAttr[VO_MAX_CHN_NUM];

    SC_CHAR filename[256] = YUV_1920_1080;
    SIZE_S stFrameSize    = {1920, 1080};

    SAMPLE_CHECK_RET(SAMPLE_VOU_SYS_Init(), "SAMPLE_VOU_SYS_Init");

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI;

    /* USER SET VO DEV SYNC INFO */
    stPubAttr.stSyncInfo.u16Hact = 720;
    stPubAttr.stSyncInfo.u16Hbb = 40;
    stPubAttr.stSyncInfo.u16Hfb = 40;

    stPubAttr.stSyncInfo.u16Vact = 1280;
    stPubAttr.stSyncInfo.u16Vbb = 10;
    stPubAttr.stSyncInfo.u16Vfb = 19;

    stPubAttr.stSyncInfo.u16Hpw = 4;
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
    SAMPLE_CHECK_RET(SAMPLE_VO_CONFIG_MIPI(MIPILCD_720x1280), "SAMPLE_VO_CONFIG_MIPI");


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

    /* START USER THREAD */
    SAMPLE_VO_StartUserThd(&stThreadInfo, VoLayer, VoChnNum, filename, &stFrameSize);


    while (!all_exit)
    {
        usleep(1000000);
    }

    /* STOP USER THREAD */
    SAMPLE_VO_StopUserThd(&stThreadInfo);

    /*DISABLE VO CHN*/
    for (i = 0; i < VoChnNum; i++)
    {
        SAMPLE_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, i), "SC_MPI_VO_DisableChn");
    }

    /* DISABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");

    /* DISABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    SAMPLE_VOU_SYS_Exit();

    return SC_SUCCESS;
}

/******************************************************************************
* function :  FILE + MIPILCD (720x1280) + RGBLCD (720x1280)
******************************************************************************/
SC_S32 SAMPLE_VO_MIPIRGBLCD_720x1280(SC_VOID)
{
    SC_U32 i = 0;
    SC_S32 VoDev = 0;
    SC_S32 VoLayer = 0;
    SC_S32 VoChnNum = 1;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo;
    SC_U32 u32Framerate;
    SIZE_S stDevSize;

    VO_CHN_ATTR_S astChnAttr[VO_MAX_CHN_NUM];

    SC_CHAR filename[256] = YUV_1920_1080;
    SIZE_S stFrameSize    = {1920, 1080};

    SAMPLE_CHECK_RET(SAMPLE_VOU_SYS_Init(), "SAMPLE_VOU_SYS_Init");

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI|VO_INTF_LCD_24BIT;

    /* USER SET VO DEV SYNC INFO */
    stPubAttr.stSyncInfo.u16Hact = 720;
    stPubAttr.stSyncInfo.u16Hbb = 40;
    stPubAttr.stSyncInfo.u16Hfb = 40;

    stPubAttr.stSyncInfo.u16Vact = 1280;
    stPubAttr.stSyncInfo.u16Vbb = 10;
    stPubAttr.stSyncInfo.u16Vfb = 19;

    stPubAttr.stSyncInfo.u16Hpw = 4;
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
    SAMPLE_CHECK_RET(SAMPLE_VO_CONFIG_MIPI(MIPILCD_720x1280), "SAMPLE_VO_CONFIG_MIPI");

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

    /* START USER THREAD */
    SAMPLE_VO_StartUserThd(&stThreadInfo, VoLayer, VoChnNum, filename, &stFrameSize);


    while (!all_exit)
    {
        usleep(1000000);
    }

    /* STOP USER THREAD */
    SAMPLE_VO_StopUserThd(&stThreadInfo);

    /*DISABLE VO CHN*/
    for (i = 0; i < VoChnNum; i++)
    {
        SAMPLE_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, i), "SC_MPI_VO_DisableChn");
    }

    /* DISABLE VO LAYER */
    SAMPLE_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");

    /* DISABLE VO DEV */
    SAMPLE_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    SAMPLE_VOU_SYS_Exit();

    return SC_SUCCESS;
}

/******************************************************************************
* function :  VI + MIPILCD (720x1280) + RGBLCD (720x1280)
******************************************************************************/
SC_S32 SAMPLE_VI_MIPIRGBLCD_720x1280(void)
{
    SC_S32             i, s32Ret;
    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VO_CHN                      VoChn           = 0;
    SC_S32                      VoDev           = 0;
    SC_S32                      VoLayer         = 0;
    SC_S32                      VoChnNum        = 1;
    VO_PUB_ATTR_S               stPubAttr;
    VO_VIDEO_LAYER_ATTR_S       stLayerAttr;
    VO_USER_INTFSYNC_INFO_S     stUserInfo;
    SC_U32                      u32Framerate;
    SIZE_S                      stDevSize;
    VO_CHN_ATTR_S               astChnAttr[VO_MAX_CHN_NUM];
    VO_INTF_TYPE_E              enVoIntfType    = VO_INTF_HDMI;

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

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI|VO_INTF_LCD_24BIT;


    /* USER SET VO DEV SYNC INFO */
    stPubAttr.stSyncInfo.u16Hact = 720;
    stPubAttr.stSyncInfo.u16Hbb = 40;
    stPubAttr.stSyncInfo.u16Hfb = 40;

    stPubAttr.stSyncInfo.u16Vact = 1280;
    stPubAttr.stSyncInfo.u16Vbb = 10;
    stPubAttr.stSyncInfo.u16Vfb = 19;

    stPubAttr.stSyncInfo.u16Hpw = 4;
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
    SAMPLE_CHECK_RET(SAMPLE_VO_CONFIG_MIPI(MIPILCD_720x1280), "SAMPLE_VO_CONFIG_MIPI");

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

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT_VO_STOP;
    }

    while (!all_exit)
    {
        usleep(1000000);
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

/******************************************************************************
* function :  VI + (MIPI + RGB) or HDMI loop.
******************************************************************************/
SC_S32 SAMPLE_VI_MIPIRGB_HDMI_LOOP(void)
{
    SC_S32             i, s32Ret;
    SC_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    SC_S32             s32WorkSnsId   = 0;

    PIC_SIZE_E         enSnsSize;
    SIZE_S             stSize, stRawSize;
    VB_CONFIG_S        stVbConf;
    SC_U32             u32BlkSize;
    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_PLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

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

    while(!all_exit)
    {
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

static SC_S32 SAMPLE_VO_SetVideoLayerCSC(int csc)
{
    VO_CSC_S stVideoCSC;

    memset(&stVideoCSC, 0, sizeof(stVideoCSC));

    switch (csc)
    {
    case 0:
    {
        stVideoCSC.enCscMatrix = VO_CSC_MATRIX_BT601_TO_BT709;
    }
    break;

    case 1:
    {
        stVideoCSC.enCscMatrix = VO_CSC_MATRIX_BT709_TO_BT601;
    }
    break;

    case 2:
    {
        stVideoCSC.enCscMatrix = VO_CSC_MATRIX_RGB_TO_BT601_PC;
    }
    break;

    case 3:
    {
        stVideoCSC.enCscMatrix = VO_CSC_MATRIX_RGB_TO_BT709_PC;
    }
    break;
    case 4:
    {
        stVideoCSC.enCscMatrix = VO_CSC_MATRIX_RGB_TO_BT601_TV;
    }
    break;
    case 5:
    {
        stVideoCSC.enCscMatrix = VO_CSC_MATRIX_RGB_TO_BT709_TV;
    }
    break;
    default:
    {
        printf("VO_SetVideoLayerCSC->csc:%d error!\n", csc);
        return SC_FAILURE;
    }
    break;
    }

    printf("VO_SetVideoLayerCSC->csc:%d enCscMatrix:%d\n",
        csc, stVideoCSC.enCscMatrix);

    SC_S32 ret = SC_MPI_VO_SetVideoLayerCSC(Dy_VoLayer, &stVideoCSC);

    return ret;
}

static SC_S32 SAMPLE_VO_SetChnAttr(VO_CHN_ATTR_S *pstChnAttr)
{
    printf("VO_SetChnAttr->u32Priority:%d rect:%d-%d-%d-%d\n",
        pstChnAttr->u32Priority, pstChnAttr->stRect.s32X, pstChnAttr->stRect.s32Y,
        pstChnAttr->stRect.u32Width, pstChnAttr->stRect.u32Height);

    SC_S32 ret = SC_MPI_VO_SetChnAttr(Dy_VoLayer, Dy_VoChn, pstChnAttr);

    return ret;
}

static SC_S32 SAMPLE_VO_SetChnParam(VO_CHN_PARAM_S *pstChnParam)
{
    printf("VO_SetChnParam->ratio:%d rect:%d-%d-%d-%d bgcolor:0x%x\n",
        pstChnParam->stAspectRatio.enMode, pstChnParam->stAspectRatio.stVideoRect.s32X,
        pstChnParam->stAspectRatio.stVideoRect.s32Y, pstChnParam->stAspectRatio.stVideoRect.u32Width,
        pstChnParam->stAspectRatio.stVideoRect.u32Height, pstChnParam->stAspectRatio.u32BgColor);

    SC_S32 ret = SC_MPI_VO_SetChnParam(Dy_VoLayer, Dy_VoChn, pstChnParam);

    return ret;
}

static SC_S32 SAMPLE_VO_SetChnDisplayPosition(POINT_S *pstDispPos)
{
    printf("VO_SetChnDisplayPosition->x:%d y:%d\n",
        pstDispPos->s32X, pstDispPos->s32Y);

    SC_S32 ret = SC_MPI_VO_SetChnDisplayPosition(Dy_VoLayer, Dy_VoChn, pstDispPos);

    return ret;
}

static SC_S32 SAMPLE_VO_PauseChn(SC_VOID)
{
    SC_S32 ret = SC_MPI_VO_PauseChn(Dy_VoLayer, Dy_VoChn);

    printf("VO_PauseChn->ret:%d\n", ret);

    return ret;
}

static SC_S32 SAMPLE_VO_ResumeChn(SC_VOID)
{
    SC_S32 ret = SC_MPI_VO_ResumeChn(Dy_VoLayer, Dy_VoChn);

    printf("VO_ResumeChn->ret:%d\n", ret);

    return ret;
}

static SC_S32 SAMPLE_VO_ShowChn(SC_VOID)
{
    SC_S32 ret = SC_MPI_VO_ShowChn(Dy_VoLayer, Dy_VoChn);

    printf("VO_ShowChn->ret:%d\n", ret);

    return ret;
}

static SC_S32 SAMPLE_VO_HideChn(SC_VOID)
{
    SC_S32 ret = SC_MPI_VO_HideChn(Dy_VoLayer, Dy_VoChn);

    printf("VO_HideChn->ret:%d\n", ret);

    return ret;
}

#if 1
/*  save to yvu420p  for 8bit*/
static void sample_yuv_8bit_dump(FILE *pfd, VIDEO_FRAME_S *pVBuf)
{
    unsigned int h;
    char *pVBufVirt_Y;
    char *pVBufVirt_C;
    char *pMemContent;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    SC_U32 u32YSize, u32CSize, u32UvHeight =
        0;/*When the storage format is a planar format, this variable is used to keep the height of the UV component */

    if (PIXEL_FORMAT_YVU_PLANAR_420 == enPixelFormat)
    {
        u32YSize = (pVBuf->u32Stride[0]) * (pVBuf->u32Height);
        u32CSize = (pVBuf->u32Stride[1]) * (pVBuf->u32Height) / 2;
        u32UvHeight = pVBuf->u32Height / 2;
    }
    else
    {
        fprintf(stderr, "no support video format(%d)\n", enPixelFormat);
        return;
    }

    pVBufVirt_Y = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[0], u32YSize);
    if (NULL == pVBufVirt_Y)
    {
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
    SC_MPI_SYS_Munmap(pVBufVirt_Y, u32YSize);

    if (PIXEL_FORMAT_YVU_PLANAR_420 == enPixelFormat)
    {
        fflush(pfd);
        /* save V ----------------------------------------------------------------*/
        fprintf(stderr, "V......");
        fflush(stderr);
        pVBufVirt_C = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[1], u32CSize);
        if (NULL == pVBufVirt_C)
        {
            return;
        }

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];

            fwrite(pMemContent, pVBuf->u32Width / 2, 1, pfd);
        }
        SC_MPI_SYS_Munmap(pVBufVirt_C, u32CSize);

        fflush(pfd);

        /* save U ----------------------------------------------------------------*/
        fprintf(stderr, "U......");
        fflush(stderr);
        pVBufVirt_C = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[2], u32CSize);
        if (NULL == pVBufVirt_C)
        {
            return;
        }

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];

            fwrite(pMemContent, pVBuf->u32Width / 2, 1, pfd);
        }
        SC_MPI_SYS_Munmap(pVBufVirt_C, u32CSize);
    }

    fflush(pfd);
    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);
}
#endif

#if 0
/*  save to yvu420p  for 8bit*/
static void sample_yuv_8bit_dump(FILE *pfd, VIDEO_FRAME_S *pVBuf)
{
    unsigned int h;
    char *pVBufVirt_Y;
    char *pVBufVirt_C;
    char *pMemContent;
    SC_U64 phy_addr;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    SC_U32 u32Size, u32CSize, u32UvHeight =
        0;/*When the storage format is a planar format, this variable is used to keep the height of the UV component */
    SC_CHAR *pUserPageAddr = NULL;

    if (PIXEL_FORMAT_YVU_PLANAR_420 == enPixelFormat)
    {
        u32Size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height) * 3 / 2;
        u32CSize = (pVBuf->u32Stride[1]) * (pVBuf->u32Height) / 2;
        u32UvHeight = pVBuf->u32Height / 2;
        printf("sample_yuv_8bit_dump->u32Size:%d u32CSize:%d u32UvHeight:%d\n",
            u32Size, u32CSize, u32UvHeight);
    }
    else
    {
        fprintf(stderr, "no support video format(%d)\n", enPixelFormat);
        return;
    }

    phy_addr = pVBuf->u64PhyAddr[0];
    pUserPageAddr = (SC_CHAR *) SC_MPI_SYS_Mmap(phy_addr, u32Size);

    if (NULL == pUserPageAddr)
    {
        return;
    }

    pVBufVirt_Y = pUserPageAddr;
    pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0]) * (pVBuf->u32Height);

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);

    for (h = 0; h < pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h * pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }

    if (PIXEL_FORMAT_YVU_PLANAR_420 == enPixelFormat)
    {
        fflush(pfd);
        /* save V ----------------------------------------------------------------*/
        fprintf(stderr, "V......");
        fflush(stderr);

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];

            fwrite(pMemContent, pVBuf->u32Width / 2, 1, pfd);
        }

        fflush(pfd);

        /* save U ----------------------------------------------------------------*/
        fprintf(stderr, "U......");
        fflush(stderr);

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + u32CSize + h * pVBuf->u32Stride[1];

            fwrite(pMemContent, pVBuf->u32Width / 2, 1, pfd);
        }
    }

    fflush(pfd);
    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    SC_MPI_SYS_Munmap(pUserPageAddr, u32Size);
    pUserPageAddr = NULL;
}
#endif

static SC_CHAR *sample_get_dyrg(DYNAMIC_RANGE_E enDyRg, SC_CHAR *pszYuvDyRg)
{
    switch(enDyRg)
    {
    case DYNAMIC_RANGE_SDR8:
        snprintf(pszYuvDyRg, 10, "SDR8");
        break;
    case DYNAMIC_RANGE_SDR10:
        snprintf(pszYuvDyRg, 10, "SDR10");
        break;
    case DYNAMIC_RANGE_HDR10:
        snprintf(pszYuvDyRg, 10, "HDR10");
        break;
    case DYNAMIC_RANGE_HLG:
        snprintf(pszYuvDyRg, 10, "HLG");
        break;
    case DYNAMIC_RANGE_SLF:
        snprintf(pszYuvDyRg, 10, "SLF");
        break;
    default:
        snprintf(pszYuvDyRg, 10, "SDR8");
        break;
    }
    return pszYuvDyRg;
}

static SC_S32 SAMPLE_VO_DumpChn(VO_LAYER VoLayer, VO_CHN VoChn, SC_S32 s32Cnt)
{
    SC_S32             i, s32Ret;
    SC_CHAR            szYuvName[128];
    SC_CHAR            szPixFrm[10];
    SC_CHAR            szYuvDyRg[10];
    VIDEO_FRAME_INFO_S stFrame;
    FILE              *pfd = NULL;

    /* Get Frame to make file name*/
    s32Ret = SC_MPI_VO_GetChnFrame(VoLayer, VoChn, &stFrame, -1);
    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_VO(%d)_GetChnFrame errno %#x\n", VoLayer, s32Ret);
        return SC_FAILURE;
    }

    /* make file name */
    if (PIXEL_FORMAT_YVU_SEMIPLANAR_420 == stFrame.stVFrame.enPixelFormat)
    {
        snprintf(szPixFrm, 10, "P420");
    }
    else if (PIXEL_FORMAT_YVU_SEMIPLANAR_422 == stFrame.stVFrame.enPixelFormat)
    {
        snprintf(szPixFrm, 10, "P422");
    }
    else if (PIXEL_FORMAT_YVU_PLANAR_420 == stFrame.stVFrame.enPixelFormat)
    {
        snprintf(szPixFrm, 10, "P420");
    }
    else
    {
        return SC_FAILURE;
    }

    snprintf(szYuvName, 128, "./vo_layer%d_chn%d_%ux%u_%s_%s_%d.yuv", VoLayer, VoChn,
        stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height, szPixFrm, sample_get_dyrg(stFrame.stVFrame.enDynamicRange,
            szYuvDyRg), s32Cnt);

    printf("Dump YUV frame of vo(%d,%d) to file: \"%s\"\n", VoLayer, VoChn, szYuvName);

    SC_MPI_VO_ReleaseChnFrame(VoLayer, VoChn, &stFrame);

    /* open file */
    pfd = fopen(szYuvName, "wb");

    if (NULL == pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));
        return SC_FAILURE;
    }

    /* get VO frame  */
    for (i = 0; i < s32Cnt; i++)
    {
        s32Ret = SC_MPI_VO_GetChnFrame(VoLayer, VoChn, &stFrame, 20);
        if (SC_SUCCESS != s32Ret)
        {
            printf("get vo(%d,%d) frame err\n", VoLayer, VoChn);
            printf("only get %d frame\n", i);
            break;
        }

        /* save VO frame to file */
        if (DYNAMIC_RANGE_SDR8 == stFrame.stVFrame.enDynamicRange)
        {
            sample_yuv_8bit_dump(pfd, &stFrame.stVFrame);
        }
        else
        {
            fclose(pfd);
            printf("no support 10bit dump\n");
            return SC_FAILURE;
        }

        /* release frame after using */
        s32Ret = SC_MPI_VO_ReleaseChnFrame(VoLayer, VoChn, &stFrame);
        memset(&stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        if (SC_SUCCESS != s32Ret)
        {
            printf("Release vo(%d,%d) frame err\n", VoLayer, VoChn);
            printf("only get %d frame\n", i);
            break;
        }
    }

    memset(&stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

    fclose(pfd);

    return SC_SUCCESS;
}

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_Thr_Usage(char *sPrgNm)
{
    printf("Usage : %s \n", sPrgNm);

    printf("\t-i | --csc 0\n"
        "\t0-BT601_TO_BT709"
        "\t1-BT709_TO_BT601"
        "\t2-RGB_TO_BT601_PC"
        "\t3-RGB_TO_BT709_PC"
        "\t4-RGB_TO_BT601_TV"
        "\t5-RGB_TO_BT709_TV\n");

    printf("\t-a | --chnattr: 0 0 0 1920 1080\n"
        "\tpriority:0\n"
        "\trect:x y w h");

    printf("\t-c | --chnpara: 0 0 0 1920 1080 0x808080\n"
        "\tratio:0-none 1-auto 2-manual\n"
        "\trect:x y w h"
        "\tbgcolor:0xFFFFFF\n");

    printf("\t-d | --displaypos: 0 0\n"
        "\tpos:x y\n");

    printf("\t-w | --dumpchn: 0 0 5\n"
        "\tlayer:0-2\n"
        "\tchn:0-9\n"
        "\tcnt:dump frame num\n");

    printf("\t-p | --pause\n");
    printf("\t-r | --resume\n");
    printf("\t-s | --show\n");
    printf("\t-h | --hide\n");
    printf("\t-e | --exit  main to exit!\n");

    return;
}

static SC_CHAR optstr[] = "?::i:a:c:d:w:prshe";
static const struct option long_options[] =
{
    {"csc",         required_argument, NULL, 'i'},
    {"chnattr",     required_argument, NULL, 'a'},
    {"chnpara",     required_argument, NULL, 'c'},
    {"displaypos",  required_argument, NULL, 'd'},
    {"dumpchn",     required_argument, NULL, 'w'},
    {"pause",       no_argument,       NULL, 'p'},
    {"resume",      no_argument,       NULL, 'r'},
    {"show",        no_argument,       NULL, 's'},
    {"hide",        no_argument,       NULL, 'h'},
    {"exit",        no_argument,       NULL, 'e'},
    {"help",        optional_argument, NULL, '?'},
    {NULL,          0,                 NULL,   0},
};

#define SAMPLE_VO_CheckIndex(index) \
if ((index >= cnt) || (NULL == pstrbuf[index])) \
{ \
    printf("[Func]:%s [Line]:%d->cnt:%d pstrbuf[%d]:%p\n", \
        __FUNCTION__, __LINE__, cnt, index, pstrbuf[index]); \
    break; \
}

SC_VOID *SAMPLE_VO_DynamicPara(SC_VOID *pArgs)
{
    int  c = 0;
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
        pstrbuf[cnt++] = "./sample_vo";
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
            case 'i':
            {
                SAMPLE_VO_SetVideoLayerCSC(atoi(optarg));
            }
            break;
            case 'a':
            {
                VO_CHN_ATTR_S stChnAttr;

                stChnAttr.u32Priority = atoi(optarg);

                index = optind;
                SAMPLE_VO_CheckIndex(index);
                stChnAttr.stRect.s32X = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VO_CheckIndex(index);
                stChnAttr.stRect.s32Y = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VO_CheckIndex(index);
                stChnAttr.stRect.u32Width = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VO_CheckIndex(index);
                stChnAttr.stRect.u32Height = atoi(pstrbuf[index]);

                SAMPLE_VO_SetChnAttr(&stChnAttr);
            }
            break;
            case 'c':
            {
                VO_CHN_PARAM_S stChnParam;

                stChnParam.stAspectRatio.enMode = atoi(optarg);

                index = optind;
                SAMPLE_VO_CheckIndex(index);
                stChnParam.stAspectRatio.stVideoRect.s32X = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VO_CheckIndex(index);
                stChnParam.stAspectRatio.stVideoRect.s32Y = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VO_CheckIndex(index);
                stChnParam.stAspectRatio.stVideoRect.u32Width = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VO_CheckIndex(index);
                stChnParam.stAspectRatio.stVideoRect.u32Height = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VO_CheckIndex(index);
                stChnParam.stAspectRatio.u32BgColor = strtol(pstrbuf[index], NULL, 0);

                SAMPLE_VO_SetChnParam(&stChnParam);
            }
            break;
            case 'd':
            {
                POINT_S stDispPos;

                stDispPos.s32X = atoi(optarg);

                SAMPLE_VO_CheckIndex(optind);
                stDispPos.s32Y = atoi(pstrbuf[optind]);

                SAMPLE_VO_SetChnDisplayPosition(&stDispPos);
            }
            break;
            case 'w':
            {
                VO_LAYER VoLayer;
                VO_CHN VoChn;
                SC_S32 allcnt;

                VoLayer = atoi(optarg);

                index = optind;
                SAMPLE_VO_CheckIndex(index);
                VoChn = atoi(pstrbuf[index]);

                index++;
                SAMPLE_VO_CheckIndex(index);
                allcnt = atoi(pstrbuf[index]);

                printf("dumpchn, VoLayer(%d) VoChn(%d) allcnt(%d)\n",
                    VoLayer, VoChn, allcnt);

                SAMPLE_VO_DumpChn(VoLayer, VoChn, allcnt);

            }
            break;
            case 'p':
            {
                SAMPLE_VO_PauseChn();
            }
            break;
            case 'r':
            {
                SAMPLE_VO_ResumeChn();
            }
            break;
            case 's':
            {
                SAMPLE_VO_ShowChn();
            }
            break;
            case 'h':
            {
                SAMPLE_VO_HideChn();
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

SC_VOID SAMPLE_VO_StartThread(SC_VOID)
{
    pthread_t Threadid = 0;
    pthread_create(&Threadid, 0, SAMPLE_VO_DynamicPara, NULL);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
