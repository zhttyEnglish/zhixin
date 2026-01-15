/**
 * @file     desktop_vo.c
 * @brief    桌面背景显示功能
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
 * @date    2023-7-7 创建文件
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
#include "desktop_vo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum
{
    DESKTOPVO_MIPI800x600 = 0,
    DESKTOPVO_BUTT
} DESKTOPVO_TYPE;


#define ALIGN_BACK(x, a)        ((a) * (((x + a -1) / (a))))
#define DESKTOPVO_CHECK_RET(express,name)\
    do {\
        SC_S32 Ret;\
        Ret = express;\
        if (Ret != SC_SUCCESS) {\
            printf("%s failed at %s : LINE: %d with %#x!\n",name, __FUNCTION__,__LINE__,Ret);\
            DeskTopVO_Sys_Exit();\
            return Ret;\
            }\
        }while(0)


SC_S32 DeskTopVO_Sys_Init(void)
{
    VB_CONFIG_S stVbConf = {0};
    SC_U32      u32BlkSize;

    SC_MPI_VO_Exit();
    SC_MPI_SYS_Exit();
    SC_MPI_VB_Exit();

    stVbConf.u32MaxPoolCnt = 1;
    u32BlkSize = COMMON_GetPicBufferSize(1920, 1080, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
            DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 6;

    DESKTOPVO_CHECK_RET(SAMPLE_COMM_SYS_Init(&stVbConf), "SAMPLE_COMM_SYS_Init");

    return SC_SUCCESS;
}

SC_VOID DeskTopVO_Sys_Exit(void)
{
    SC_MPI_VO_Exit();
    SC_MPI_SYS_Exit();
    SC_MPI_VB_Exit();
}

void DeskTopVO_HandleSig(SC_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        DeskTopVO_Sys_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

static SC_S32 get_tick_count(struct timeval *p_tv)
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

static SC_S32 time_diff(struct timeval *p_start)
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

static SC_VOID DeskTopVO_GetMipiTxConfig(DESKTOPVO_TYPE type, combo_dev_cfg_t *pstMipiTxConfig)
{
    switch (type)
    {
        case DESKTOPVO_MIPI800x600:
        default:
        {
            pstMipiTxConfig->devno = 0;
            pstMipiTxConfig->lane_id[0] = 0;
            pstMipiTxConfig->lane_id[1] = 1;
            pstMipiTxConfig->lane_id[2] = 2;
            pstMipiTxConfig->lane_id[3] = 3;
            pstMipiTxConfig->output_mode = OUTPUT_MODE_DSI_VIDEO;
            pstMipiTxConfig->output_format = OUT_FORMAT_RGB_24_BIT;
            pstMipiTxConfig->video_mode = BURST_MODE;
            pstMipiTxConfig->sync_info.vid_pkt_size = 800;
            pstMipiTxConfig->sync_info.vid_hsa_pixels = 30;
            pstMipiTxConfig->sync_info.vid_hbp_pixels = 45;
            pstMipiTxConfig->sync_info.vid_hline_pixels = 1175;
            pstMipiTxConfig->sync_info.vid_vsa_lines = 10;
            pstMipiTxConfig->sync_info.vid_vbp_lines = 23;
            pstMipiTxConfig->sync_info.vid_vfp_lines = 60;
            pstMipiTxConfig->sync_info.vid_active_lines = 600;
            pstMipiTxConfig->sync_info.edpi_cmd_size = 0;
            pstMipiTxConfig->phy_data_rate = 324;
            pstMipiTxConfig->pixel_clk = 54054;
        }
    }

    return;
}

static SC_S32 DeskTopVO_MipiTxSet(DESKTOPVO_TYPE type)
{
    SC_S32 s32Ret;
    combo_dev_cfg_t stMipiTxConfig;

    DeskTopVO_GetMipiTxConfig(type, &stMipiTxConfig);

    s32Ret = SC_MPI_VO_MipiTxSet(0, SC_MIPI_TX_SET_DEV_CFG, &stMipiTxConfig,
                sizeof(combo_dev_cfg_t));
    if (s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_VO_MipiTxSet failed\n");
    }

    return s32Ret;
}

static SC_S32 DeskTopVO_ConfigMipi(DESKTOPVO_TYPE type)
{
    SC_S32 s32Ret;

    s32Ret = DeskTopVO_MipiTxSet(type);
    if (s32Ret != SC_SUCCESS)
    {
        printf("DeskTopVO_MipiTxSet failed\n");
        return s32Ret;
    }

    if (type == DESKTOPVO_MIPI800x600)
    {
    }
    else
    {
        printf("DeskTopVO_ConfigMipi error, type:%d\n", type);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VO_MipiTxSet(0, SC_MIPI_TX_ENABLE, NULL, 0);
    if (s32Ret != SC_SUCCESS)
    {
        printf("SC_MIPI_TX_ENABLE failed\n");
    }

    return s32Ret;
}

static SC_VOID DeskTopVO_GetUserPubBaseAttr(VO_PUB_ATTR_S *pstPubAttr)
{
    pstPubAttr->u32BgColor = COLOR_RGB_BLUE;
    pstPubAttr->enIntfSync = VO_OUTPUT_USER;
    pstPubAttr->stSyncInfo.bSynm = 0;
    pstPubAttr->stSyncInfo.u8Intfb = 0;
    pstPubAttr->stSyncInfo.bIop = 1;

    pstPubAttr->stSyncInfo.u16Hmid = 1;
    pstPubAttr->stSyncInfo.u16Bvact = 1;
    pstPubAttr->stSyncInfo.u16Bvbb = 1;
    pstPubAttr->stSyncInfo.u16Bvfb = 1;

    pstPubAttr->stSyncInfo.bIdv = 0;
    pstPubAttr->stSyncInfo.bIhs = 0;
    pstPubAttr->stSyncInfo.bIvs = 0;

    return;
}

static SC_VOID DeskTopVO_GetUserLayerAttr(VO_VIDEO_LAYER_ATTR_S *pstLayerAttr, SIZE_S  *pstDevSize)
{
    pstLayerAttr->bClusterMode = SC_FALSE;
    pstLayerAttr->bDoubleFrame = SC_FALSE;
    pstLayerAttr->enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    pstLayerAttr->enPixFormat = PIXEL_FORMAT_YVU_PLANAR_420;

    pstLayerAttr->stDispRect.s32X = 0;
    pstLayerAttr->stDispRect.s32Y = 0;
    pstLayerAttr->stDispRect.u32Height = pstDevSize->u32Height;
    pstLayerAttr->stDispRect.u32Width  = pstDevSize->u32Width;

    pstLayerAttr->stImageSize.u32Height = pstDevSize->u32Height;
    pstLayerAttr->stImageSize.u32Width = pstDevSize->u32Width;

    return;
}

static SC_VOID DeskTopVO_BackGround(SC_VOID)
{
    SC_S32 i;
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
    int frame_cnt = 20;

    get_tick_count(&tv_frame);

    VO_LAYER VoLayer = 0;
    memset(&stUserFrame, 0x0, sizeof(VIDEO_FRAME_INFO_S));

    u32SrcWidth = 1920;
    u32SrcHeight = 1080;

    u32Size = ALIGN_BACK(u32SrcWidth, 512) * u32SrcHeight * 3;
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Size;
    stVbPoolCfg.u32BlkCnt = 10;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    Pool = SC_MPI_VB_CreatePool(&stVbPoolCfg);
    if (Pool == VB_INVALID_POOLID)
    {
        printf("Maybe you not call sys init\n");
        return;
    }

    stUserFrame.stVFrame.enField = VIDEO_FIELD_FRAME;
    stUserFrame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    stUserFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    stUserFrame.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stUserFrame.stVFrame.enColorGamut = COLOR_GAMUT_BT709;
    stUserFrame.stVFrame.u32Width = u32SrcWidth;
    stUserFrame.stVFrame.u32Height = u32SrcHeight;
    stUserFrame.stVFrame.u32Stride[0] = ALIGN_BACK(u32SrcWidth, 512);
    stUserFrame.stVFrame.u32Stride[1] = ALIGN_BACK(u32SrcWidth / 2, 512);
    stUserFrame.stVFrame.u32Stride[2] = ALIGN_BACK(u32SrcWidth / 2, 512);
    stUserFrame.stVFrame.u32TimeRef = 0;
    stUserFrame.stVFrame.u64PTS = 0;
    stUserFrame.stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;
    u32LumaSize =  stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight;
    u32ChromaSize = stUserFrame.stVFrame.u32Stride[0] * u32SrcHeight / 4;

    do
    {
        //framerate ctrl
        if ((time_diff(&tv_frame) + pre_tvdiff) < 1000000 / 30)
        {
            usleep(10000);
            continue;
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

    #if 0
        memset((SC_VOID *)stUserFrame.stVFrame.u64VirAddr[0], 0x10, u32LumaSize);
        memset((SC_VOID *)stUserFrame.stVFrame.u64VirAddr[1], 0x80, u32ChromaSize);
        memset((SC_VOID *)stUserFrame.stVFrame.u64VirAddr[2], 0x80, u32ChromaSize);
    #else
        memset((SC_VOID *)stUserFrame.stVFrame.u64VirAddr[0], 0xEB, u32LumaSize);
        memset((SC_VOID *)stUserFrame.stVFrame.u64VirAddr[1], 0x80, u32ChromaSize);
        memset((SC_VOID *)stUserFrame.stVFrame.u64VirAddr[2], 0x80, u32ChromaSize);
    #endif

        stUserFrame.stVFrame.u64PTS += 40000;
        stUserFrame.stVFrame.u32TimeRef += 40000;

        SC_MPI_VO_SendFrame(VoLayer, 0, &stUserFrame, 0);

        SC_MPI_VB_ReleaseBlock(hBlkHdl);
        SC_MPI_SYS_Munmap((SC_VOID *)(SC_UL)stUserFrame.stVFrame.u64VirAddr[0], u32Size);

    } while(frame_cnt-- > 0);

    return;
}

/******************************************************************************
* function : HDMI 1080p (1920x1080)
******************************************************************************/
SC_S32 DeskTopVO_Hdmi_1080p(SC_VOID)
{
    SC_S32 VoDev = 0;
    SC_S32 VoLayer = 0;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_CHN_ATTR_S astChnAttr;
    SIZE_S  stDevSize = {1920, 1080};

    DESKTOPVO_CHECK_RET(DeskTopVO_Sys_Init(), "DeskTopVO_Sys_Init");

    stPubAttr.u32BgColor = 0x808080;
    stPubAttr.enIntfType = VO_INTF_HDMI;
    stPubAttr.enIntfSync = VO_OUTPUT_1080P60;

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_SetPubAttr");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_Enable(VoDev), "SC_MPI_VO_Enable");

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetDisplayBufLen(VoDev, 3), "SC_MPI_VO_SetDisplayBufLen");

    DeskTopVO_GetUserLayerAttr(&stLayerAttr, &stDevSize);
    stLayerAttr.u32DispFrmRt = 30;

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_SetVideoLayerAttr");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_EnableVideoLayer(VoLayer), "SC_MPI_VO_EnableVideoLayer");

    memset(&astChnAttr, 0, sizeof(astChnAttr));
    astChnAttr.stRect.u32Height = stDevSize.u32Height;
    astChnAttr.stRect.u32Width = stDevSize.u32Width;

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetChnAttr(VoLayer, 0, &astChnAttr), "SC_MPI_VO_SetChnAttr");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_EnableChn(VoLayer, 0), "SC_MPI_VO_EnableChn");

    DeskTopVO_BackGround();

    while (1)
    {
        usleep(10000);
    }

    DESKTOPVO_CHECK_RET(SC_MPI_VO_ClearChnBuf(VoLayer, 0, SC_FALSE), "SC_MPI_VO_ClearChnBuf");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, 0), "SC_MPI_VO_DisableChn");

    DESKTOPVO_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    DeskTopVO_Sys_Exit();

    return SC_SUCCESS;
}

/******************************************************************************
* function :  FILE + MIPI (800x600)
******************************************************************************/
SC_S32 DeskTopVO_Mipi_800x600(SC_VOID)
{
    SC_S32 VoDev = 0;
    SC_S32 VoLayer = 0;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo;
    VO_CHN_ATTR_S astChnAttr;
    SIZE_S  stDevSize = {1920, 1080};

    DESKTOPVO_CHECK_RET(DeskTopVO_Sys_Init(), "DeskTopVO_Sys_Init");

    /* SET VO PUB ATTR OF USER TYPE */
    DeskTopVO_GetUserPubBaseAttr(&stPubAttr);

    stPubAttr.enIntfType = VO_INTF_MIPI;

    /* USER SET VO DEV SYNC INFO */
    stPubAttr.stSyncInfo.u16Hact = 800;
    stPubAttr.stSyncInfo.u16Hbb = 60;
    stPubAttr.stSyncInfo.u16Hfb = 400;

    stPubAttr.stSyncInfo.u16Vact = 600;
    stPubAttr.stSyncInfo.u16Vbb = 23;
    stPubAttr.stSyncInfo.u16Vfb = 60;

    stPubAttr.stSyncInfo.u16Hpw = 40;
    stPubAttr.stSyncInfo.u16Vpw = 10;

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr), "SC_MPI_VO_SetPubAttr");

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetDevFrameRate(VoDev, 60), "SC_MPI_VO_SetDevFrameRate");

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

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetUserIntfSyncInfo(VoDev, &stUserInfo), "SC_MPI_VO_SetUserIntfSyncInfo");

    DESKTOPVO_CHECK_RET(SC_MPI_VO_Enable(VoDev), "SC_MPI_VO_Enable");

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetDisplayBufLen(VoDev, 3), "SC_MPI_VO_SetDisplayBufLen");


    /* USER CONFIG MIPI DEV */
    DESKTOPVO_CHECK_RET(DeskTopVO_ConfigMipi(DESKTOPVO_MIPI800x600), "DeskTopVO_ConfigMipi");

    stDevSize.u32Width = stPubAttr.stSyncInfo.u16Hact;
    stDevSize.u32Height = stPubAttr.stSyncInfo.u16Vact;
    DeskTopVO_GetUserLayerAttr(&stLayerAttr, &stDevSize);
    stLayerAttr.u32DispFrmRt = 60;

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr), "SC_MPI_VO_SetVideoLayerAttr");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_EnableVideoLayer(VoLayer), "SC_MPI_VO_EnableVideoLayer");

    memset(&astChnAttr, 0, sizeof(astChnAttr));
    astChnAttr.stRect.u32Height = stDevSize.u32Height;
    astChnAttr.stRect.u32Width = stDevSize.u32Width;

    DESKTOPVO_CHECK_RET(SC_MPI_VO_SetChnAttr(VoLayer, 0, &astChnAttr), "SC_MPI_VO_SetChnAttr");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_EnableChn(VoLayer, 0), "SC_MPI_VO_EnableChn");

    DeskTopVO_BackGround();

    while (1)
    {
        usleep(10000);
    }

    DESKTOPVO_CHECK_RET(SC_MPI_VO_ClearChnBuf(VoLayer, 0, SC_FALSE), "SC_MPI_VO_ClearChnBuf");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_DisableChn(VoLayer, 0), "SC_MPI_VO_DisableChn");

    DESKTOPVO_CHECK_RET(SC_MPI_VO_DisableVideoLayer(VoLayer), "SC_MPI_VO_DisableVideoLayer");
    DESKTOPVO_CHECK_RET(SC_MPI_VO_Disable(VoDev), "SC_MPI_VO_Disable");

    DeskTopVO_Sys_Exit();

    return SC_SUCCESS;
}




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
