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
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"
#include "loadbmp.h"

#define SAMPLE_VGS_READ_PATH "./data/input/1920_1080_p420.yuv"
#define SAMPLE_VGS_SAVE_PATH "./data/output"
#define SAMPLE_VGS_BITMAP_PATH "./data/input/256_49.bmp"

typedef struct stVGS_FUNCTION_PARAM
{
    SC_BOOL bScale;
    SC_BOOL bCover;
    SC_BOOL bOsd;
    SC_BOOL bDrawLine;
    SC_BOOL bRotate;
    SC_BOOL bLuma;
    SC_BOOL bMosaic;

    VGS_SCLCOEF_MODE_E *penVgsSclCoefMode;
    VGS_ADD_COVER_S *pstVgsAddCover;
    VGS_ADD_OSD_S *pstVgsAddOsd;
    VGS_DRAW_LINE_S *pstVgsDrawLine;
    ROTATION_E *penRotationAngle;
    VGS_GET_LUMA_DATA_S *pstVgsGetLumaData;
    VGS_ADD_MOSAIC_S *pstVgsAddMosaic; 

    RGN_HANDLE *pRgnHandle;
    SAMPLE_VB_BASE_INFO_S *pstInImgVbInfo;
    SAMPLE_VB_BASE_INFO_S *pstOutImgVbInfo;
    SC_S32 s32SampleNum;
} SAMPLE_VGS_FUNC_PARAM;

typedef struct stVGS_VB_INFO
{
    VB_BLK VbHandle;
    SC_U8 *pu8VirAddr;
    SC_U32 u32VbSize;
    SC_BOOL bVbUsed;
} SAMPLE_VGS_VB_INFO;

SC_U8 *pTemp = SC_NULL;
SAMPLE_VGS_VB_INFO g_stInImgVbInfo;
SAMPLE_VGS_VB_INFO g_stOutImgVbInfo;

/******************************************************************************
* function : show usage
******************************************************************************/
SC_VOID SAMPLE_VGS_Usage(SC_CHAR *sPrgNm)
{
    printf("\n/*****************************************/\n");
    printf("Usage: %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t0) FILE -> VGS(Scale) -> FILE.\n");
    printf("\t1) FILE -> VGS(Cover+OSD) -> FILE.\n");
    printf("\t2) FILE -> VGS(DrawLine) -> FILE.\n");
    printf("\t3) FILE -> VGS(Rotate) -> FILE.\n");
    printf("\t4) FILE -> VGS(Luma) .\n");
    printf("/*****************************************/\n");
    return;
}

SC_VOID SAMPLE_VGS_Release(SC_VOID)
{
    if (pTemp != SC_NULL)
    {
        free(pTemp);
        pTemp = SC_NULL;
    }

    if (SC_TRUE == g_stInImgVbInfo.bVbUsed)
    {
        SC_MPI_SYS_Munmap((SC_VOID *)g_stInImgVbInfo.pu8VirAddr, g_stInImgVbInfo.u32VbSize);
        SC_MPI_VB_ReleaseBlock(g_stInImgVbInfo.VbHandle);
        g_stInImgVbInfo.bVbUsed = SC_FALSE;
    }

    if (SC_TRUE == g_stOutImgVbInfo.bVbUsed)
    {
        SC_MPI_SYS_Munmap((SC_VOID *)g_stOutImgVbInfo.pu8VirAddr, g_stOutImgVbInfo.u32VbSize);
        SC_MPI_VB_ReleaseBlock(g_stOutImgVbInfo.VbHandle);
        g_stOutImgVbInfo.bVbUsed = SC_FALSE;
    }

    return;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
SC_VOID SAMPLE_VGS_HandleSig(SC_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_VGS_Release();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(-1);
}

static inline SC_VOID SAMPLE_VGS_GetYUVBufferCfg(const SAMPLE_VB_BASE_INFO_S *pstVbBaseInfo,
    VB_CAL_CONFIG_S *pstVbCalConfig)
{
    COMMON_GetPicBufferConfig(pstVbBaseInfo->u32Width, pstVbBaseInfo->u32Height, pstVbBaseInfo->enPixelFormat,
        DATA_BITWIDTH_8, pstVbBaseInfo->enCompressMode, pstVbBaseInfo->u32Align, pstVbCalConfig);
    return;
}

static SC_S32 SAMPLE_VGS_GetFrameVb(const SAMPLE_VB_BASE_INFO_S *pstVbInfo, const VB_CAL_CONFIG_S *pstVbCalConfig,
    VIDEO_FRAME_INFO_S *pstFrameInfo, SAMPLE_VGS_VB_INFO *pstVgsVbInfo)
{
    SC_U64 u64PhyAddr = 0;

    pstVgsVbInfo->VbHandle = SC_MPI_VB_GetBlock(VB_INVALID_POOLID, pstVbCalConfig->u32VBSize, SC_NULL);
    if (VB_INVALID_HANDLE == pstVgsVbInfo->VbHandle)
    {
        SAMPLE_PRT("SC_MPI_VB_GetBlock failed!\n");
        return SC_FAILURE;
    }
    pstVgsVbInfo->bVbUsed = SC_TRUE;

    u64PhyAddr = SC_MPI_VB_Handle2PhysAddr(pstVgsVbInfo->VbHandle);
    if (0 == u64PhyAddr)
    {
        SAMPLE_PRT("SC_MPI_VB_Handle2PhysAddr failed!.\n");
        SC_MPI_VB_ReleaseBlock(pstVgsVbInfo->VbHandle);
        pstVgsVbInfo->bVbUsed = SC_FALSE;
        return SC_FAILURE;
    }

    pstVgsVbInfo->pu8VirAddr = (SC_U8 *)SC_MPI_SYS_Mmap(u64PhyAddr, pstVbCalConfig->u32VBSize);
    if (SC_NULL == pstVgsVbInfo->pu8VirAddr)
    {
        SAMPLE_PRT("SC_MPI_SYS_Mmap failed!.\n");
        SC_MPI_VB_ReleaseBlock(pstVgsVbInfo->VbHandle);
        pstVgsVbInfo->bVbUsed = SC_FALSE;
        return SC_FAILURE;
    }
    pstVgsVbInfo->u32VbSize = pstVbCalConfig->u32VBSize;

    pstFrameInfo->enModId = SC_ID_VGS;
    pstFrameInfo->u32PoolId = SC_MPI_VB_Handle2PoolId(pstVgsVbInfo->VbHandle);

    pstFrameInfo->stVFrame.u32Width       = pstVbInfo->u32Width;
    pstFrameInfo->stVFrame.u32Height      = pstVbInfo->u32Height;
    pstFrameInfo->stVFrame.enField        = VIDEO_FIELD_FRAME;
    pstFrameInfo->stVFrame.enPixelFormat  = pstVbInfo->enPixelFormat;
    pstFrameInfo->stVFrame.enVideoFormat  = VIDEO_FORMAT_LINEAR;
    pstFrameInfo->stVFrame.enCompressMode = pstVbInfo->enCompressMode;
    pstFrameInfo->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;
    pstFrameInfo->stVFrame.enColorGamut   = COLOR_GAMUT_BT601;

    pstFrameInfo->stVFrame.u32HeaderStride[0]  = pstVbCalConfig->u32HeadStride;
    pstFrameInfo->stVFrame.u32HeaderStride[1]  = pstVbCalConfig->u32HeadStride;
    pstFrameInfo->stVFrame.u32HeaderStride[2]  = pstVbCalConfig->u32HeadStride;
    pstFrameInfo->stVFrame.u64HeaderPhyAddr[0] = u64PhyAddr;
    pstFrameInfo->stVFrame.u64HeaderPhyAddr[1] = pstFrameInfo->stVFrame.u64HeaderPhyAddr[0] + pstVbCalConfig->u32HeadYSize;
    pstFrameInfo->stVFrame.u64HeaderPhyAddr[2] = pstFrameInfo->stVFrame.u64HeaderPhyAddr[1];
    pstFrameInfo->stVFrame.u64HeaderVirAddr[0] = (SC_U64)(SC_UL)pstVgsVbInfo->pu8VirAddr;
    pstFrameInfo->stVFrame.u64HeaderVirAddr[1] = pstFrameInfo->stVFrame.u64HeaderVirAddr[0] + pstVbCalConfig->u32HeadYSize;
    pstFrameInfo->stVFrame.u64HeaderVirAddr[2] = pstFrameInfo->stVFrame.u64HeaderVirAddr[1];

    pstFrameInfo->stVFrame.u32Stride[0]  = pstVbCalConfig->u32MainStride;
    pstFrameInfo->stVFrame.u32Stride[1]  = pstVbCalConfig->u32MainStride / 2;
    pstFrameInfo->stVFrame.u32Stride[2]  = pstVbCalConfig->u32MainStride / 2;
    pstFrameInfo->stVFrame.u64PhyAddr[0] = pstFrameInfo->stVFrame.u64HeaderPhyAddr[0];
    pstFrameInfo->stVFrame.u64PhyAddr[1] = pstFrameInfo->stVFrame.u64PhyAddr[0] + pstVbCalConfig->u32MainYSize;
    pstFrameInfo->stVFrame.u64PhyAddr[2] = pstFrameInfo->stVFrame.u64PhyAddr[1] + pstVbCalConfig->u32MainYSize / 4;
    pstFrameInfo->stVFrame.u64VirAddr[0] = pstFrameInfo->stVFrame.u64HeaderVirAddr[0];
    pstFrameInfo->stVFrame.u64VirAddr[1] = pstFrameInfo->stVFrame.u64VirAddr[0] + pstVbCalConfig->u32MainYSize;
    pstFrameInfo->stVFrame.u64VirAddr[2] = pstFrameInfo->stVFrame.u64VirAddr[1] + pstVbCalConfig->u32MainYSize / 4;

    return SC_SUCCESS;
}

static SC_S32 SAMPLE_VGS_ReadPlanarToSP42X(FILE *pFile, VIDEO_FRAME_S *pstFrame)
{
    SC_U8 *pLuma = (SC_U8 *)(SC_UL)pstFrame->u64VirAddr[0];
    SC_U8 *pChroma_U = (SC_U8 *)(SC_UL)pstFrame->u64VirAddr[1];
    SC_U8 *pChroma_V = (SC_U8 *)(SC_UL)pstFrame->u64VirAddr[2];
    SC_U32 u32LumaWidth = pstFrame->u32Width;
    SC_U32 u32ChromaWidth = u32LumaWidth >> 1;
    SC_U32 u32LumaHeight = pstFrame->u32Height;
    SC_U32 u32ChromaHeight = u32LumaHeight / 2;
    SC_U32 u32LumaStride = pstFrame->u32Stride[0];
    SC_U32 u32ChromaStride = u32LumaStride >> 1;

    SC_U8 *pDst = SC_NULL;
    SC_U32 u32Row = 0;
    SC_U32 u32List = 0;

    /* Y--------------------------------------------------*/
    pDst = pLuma;
    for (u32Row = 0; u32Row < u32LumaHeight; ++u32Row)
    {
        fread(pDst, u32LumaWidth, 1, pFile);
        pDst += u32LumaStride;
    }

    /* U--------------------------------------------------*/
    pDst = pChroma_U;
    for (u32Row = 0; u32Row < u32ChromaHeight; ++u32Row)
    {
        fread(pDst, u32ChromaWidth, 1, pFile);
        pDst += u32ChromaStride;
    }

    /* V--------------------------------------------------*/
    pDst = pChroma_V;
    for (u32Row = 0; u32Row < u32ChromaHeight; ++u32Row)
    {
        fread(pDst, u32ChromaWidth, 1, pFile);
        pDst += u32ChromaStride;
    }

    return SC_SUCCESS;
}

static SC_S32 SAMPLE_VGS_SaveSP42XToPlanar(FILE *pFile, VIDEO_FRAME_S *pstFrame)
{
    SC_U32 u32LumaWidth = pstFrame->u32Width;
    SC_U32 u32ChromaWidth = u32LumaWidth >> 1;
    SC_U32 u32LumaHeight = pstFrame->u32Height;
    SC_U32 u32ChromaHeight = u32LumaHeight >> 1;
    SC_U32 u32LumaStride = pstFrame->u32Stride[0];
    SC_U32 u32ChromaStride = u32LumaStride >> 1;
    SC_U8 *pLuma = (SC_U8 *)(SC_UL)pstFrame->u64VirAddr[0];
    SC_U8 *pChroma_U = (SC_U8 *)(SC_UL)pstFrame->u64VirAddr[1];
    SC_U8 *pChroma_V = (SC_U8 *)(SC_UL)pstFrame->u64VirAddr[2];

    SC_U8 *pDst = SC_NULL;
    SC_U32 u32Row = 0;
    SC_U32 u32List = 0;

    /* Y--------------------------------------------------*/
    pDst = pLuma;
    for (u32Row = 0; u32Row < u32LumaHeight; ++u32Row)
    {
        fwrite(pDst, 1, u32LumaWidth, pFile);
        pDst += u32LumaStride;
    }

    /* U--------------------------------------------------*/
    pDst = pChroma_U;
    for (u32Row = 0; u32Row < u32ChromaHeight; ++u32Row)
    {
        fwrite(pDst, 1, u32ChromaWidth, pFile);
        pDst += u32ChromaStride;
    }

    /* V--------------------------------------------------*/
    pDst = pChroma_V;
    for (u32Row = 0; u32Row < u32ChromaHeight; ++u32Row)
    {
        fwrite(pDst, 1, u32ChromaWidth, pFile);
        pDst += u32ChromaStride;
    }

    return SC_SUCCESS;
}

static SC_S32 SAMPLE_VGS_GetFrameFromFile(SAMPLE_VGS_FUNC_PARAM *pParam, VB_CAL_CONFIG_S *pstInImgVbCalConfig,
    VIDEO_FRAME_INFO_S *pstFrameInfo)
{
    SC_S32 s32Ret = SC_FAILURE;
    SC_CHAR szInFileName[128] = SAMPLE_VGS_READ_PATH;
    FILE *pFileRead = SC_NULL;

    pFileRead = fopen(szInFileName, "rb");
    if (SC_NULL == pFileRead)
    {
        SAMPLE_PRT("can't open file %s\n", szInFileName);
        goto EXIT;
    }

    s32Ret = SAMPLE_VGS_GetFrameVb(pParam->pstInImgVbInfo, pstInImgVbCalConfig, pstFrameInfo, &g_stInImgVbInfo);
    if (s32Ret != SC_SUCCESS)
    {
        goto EXIT1;
    }

    s32Ret = SAMPLE_VGS_ReadPlanarToSP42X(pFileRead, &pstFrameInfo->stVFrame);
    if (s32Ret != SC_SUCCESS)
    {
        goto EXIT2;
    }
    else
    {
        goto EXIT1;
    }

EXIT2:
    SC_MPI_SYS_Munmap((SC_VOID *)(SC_UL)pstFrameInfo->stVFrame.u64HeaderVirAddr[0], pstInImgVbCalConfig->u32VBSize);
    SC_MPI_VB_ReleaseBlock(g_stInImgVbInfo.VbHandle);
    g_stInImgVbInfo.bVbUsed = SC_FALSE;
EXIT1:
    fclose(pFileRead);
EXIT:
    return s32Ret;
}

static SC_S32 SAMPLE_VGS_LoadBmp(const SC_CHAR *szFileName, BITMAP_S *pstBitmap, SC_BOOL bFil, SC_U32 u32FilColor,
    const SIZE_S *pstSize, SC_U32 u32Stride, PIXEL_FORMAT_E enPixelFormat)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;
    SC_S32 i = 0;
    SC_S32 j = 0;
    SC_U32 *pu16Temp = SC_NULL;

    printf("SAMPLE_VGS_LoadBmp 0\n");

    if (GetBmpInfo(szFileName, &bmpFileHeader, &bmpInfo) < 0)
    {
        SAMPLE_PRT("GetBmpInfo err!\n");
        return SC_FAILURE;
    }

    if (SC_NULL == pstBitmap->pData)
    {
        SAMPLE_PRT("Bitmap's data is null!\n");
        return SC_FAILURE;
    }

    if (PIXEL_FORMAT_ARGB_1555 == enPixelFormat)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;
    }
    else if (PIXEL_FORMAT_ARGB_4444 == enPixelFormat)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB4444;
    }
    else if (PIXEL_FORMAT_ARGB_8888 == enPixelFormat)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB8888;
    }
    else
    {
        SAMPLE_PRT("Pixel format is not support!\n");
        return SC_FAILURE;
    }
    if (CreateSurfaceByCanvas(szFileName, &Surface, (SC_U8 *)pstBitmap->pData, pstSize->u32Width, pstSize->u32Height,
            u32Stride))
    {
        return SC_FAILURE;
    }

    pstBitmap->u32Width = Surface.u16Width;
    pstBitmap->u32Height = Surface.u16Height;
    pstBitmap->enPixelFormat = enPixelFormat;

    pu16Temp = (SC_U32 *)pstBitmap->pData;

    if (bFil)
    {
        for (i = 0; i < pstBitmap->u32Height; ++i)
        {
            for (j = 0; j < pstBitmap->u32Width; ++j)
            {
                if (u32FilColor == *pu16Temp)
                {
                    *pu16Temp &= 0x7FFF;
                }
                pu16Temp++;
            }
        }
    }

    return SC_SUCCESS;
}

static SC_S32 SAMPLE_VGS_PrepareOsdInfo(VGS_ADD_OSD_S *pstVgsAddOsd, RGN_HANDLE *pRgnHandle)
{
    SC_S32 s32Ret = SC_FAILURE;

    RGN_ATTR_S stRgnAttr;
    RGN_CANVAS_INFO_S stRgnCanvasInfo;
    BITMAP_S stBitmap;
    SIZE_S stSize;

    stSize.u32Width = pstVgsAddOsd->stRect.u32Width;
    stSize.u32Height = pstVgsAddOsd->stRect.u32Height;

    stRgnAttr.enType = OVERLAYEX_RGN;
    stRgnAttr.unAttr.stOverlayEx.enPixelFmt = pstVgsAddOsd->enPixelFmt;
    stRgnAttr.unAttr.stOverlayEx.u32BgColor = pstVgsAddOsd->u32BgColor;
    stRgnAttr.unAttr.stOverlayEx.stSize.u32Width = stSize.u32Width;
    stRgnAttr.unAttr.stOverlayEx.stSize.u32Height = stSize.u32Height;
    stRgnAttr.unAttr.stOverlayEx.u32CanvasNum = 1;
    s32Ret = SC_MPI_RGN_Create(*pRgnHandle, &stRgnAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_RGN_Create failed, s32Ret:0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = SC_MPI_RGN_GetCanvasInfo(*pRgnHandle, &stRgnCanvasInfo);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_RGN_GetCanvasInfo failed, s32Ret:0x%x", s32Ret);
        return s32Ret;
    }

    stBitmap.pData = (SC_VOID *)(SC_UL)stRgnCanvasInfo.u64VirtAddr;

    s32Ret = SAMPLE_VGS_LoadBmp(SAMPLE_VGS_BITMAP_PATH, &stBitmap, SC_FALSE, 0, &stSize, stRgnCanvasInfo.u32Stride,
            pstVgsAddOsd->enPixelFmt);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_VGS_LoadBmp failed, s32Ret:0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = SC_MPI_RGN_UpdateCanvas(*pRgnHandle);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_RGN_UpdateCanvas failed, s32Ret:0x%x", s32Ret);
        return s32Ret;
    }

    pstVgsAddOsd->u64PhyAddr = stRgnCanvasInfo.u64PhyAddr;
    pstVgsAddOsd->u32Stride = stRgnCanvasInfo.u32Stride;

    return s32Ret;
}

static SC_S32 SAMPLE_VGS_COMMON_FUNCTION(SAMPLE_VGS_FUNC_PARAM *pParam)
{
    SC_S32 s32Ret = SC_FAILURE;
    SC_BOOL bSameVB = SC_FALSE;

    VGS_HANDLE hHandle = -1;
    VGS_TASK_ATTR_S stVgsTaskAttr;

    FILE *pFileWrite = SC_NULL;
    SC_CHAR szOutFileName[128];

    VB_CONFIG_S stVbConf;
    VB_CAL_CONFIG_S stInImgVbCalConfig;
    VB_CAL_CONFIG_S stOutImgVbCalConfig;

    if (SC_NULL == pParam)
    {
        return s32Ret;
    }

    if (pParam->pstInImgVbInfo && !pParam->pstOutImgVbInfo)
    {
        bSameVB = SC_TRUE;
    }

    /************************************************
    step1:  Init SYS and common VB
    *************************************************/
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt = bSameVB ? 1 : 2;

    SAMPLE_VGS_GetYUVBufferCfg(pParam->pstInImgVbInfo, &stInImgVbCalConfig);
    stVbConf.astCommPool[0].u64BlkSize = stInImgVbCalConfig.u32VBSize;
    stVbConf.astCommPool[0].u32BlkCnt = 2;

    if (!bSameVB)
    {
        SAMPLE_VGS_GetYUVBufferCfg(pParam->pstOutImgVbInfo, &stOutImgVbCalConfig);
        stVbConf.astCommPool[1].u64BlkSize = stOutImgVbCalConfig.u32VBSize;
        stVbConf.astCommPool[1].u32BlkCnt = 2;
    }

    printf("SAMPLE_COMM_SYS_Init!\n");

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_Init failed, s32Ret:0x%x\n", s32Ret);
        goto EXIT;
    }

    /************************************************
    step2:  Get frame
    *************************************************/
    s32Ret = SAMPLE_VGS_GetFrameFromFile(pParam, &stInImgVbCalConfig, &stVgsTaskAttr.stImgIn);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_VGS_GetFrameFromFile failed, s32Ret:0x%x\n", s32Ret);
        goto EXIT;
    }

    if (bSameVB)
    {
        snprintf(szOutFileName, 128, "%s/vgs_sample%d_%dx%d_p420.yuv", SAMPLE_VGS_SAVE_PATH, pParam->s32SampleNum,
            pParam->pstInImgVbInfo->u32Width, pParam->pstInImgVbInfo->u32Height);
    }
    else
    {
        snprintf(szOutFileName, 128, "%s/vgs_sample%d_%dx%d_p420.yuv", SAMPLE_VGS_SAVE_PATH, pParam->s32SampleNum,
            pParam->pstOutImgVbInfo->u32Width, pParam->pstOutImgVbInfo->u32Height);
    }

    pFileWrite = fopen(szOutFileName, "w+");
    if (SC_NULL == pFileWrite)
    {
        SAMPLE_PRT("can't open file %s\n", szOutFileName);
        goto EXIT1;
    }

    if (bSameVB)
    {
        memcpy(&stVgsTaskAttr.stImgOut, &stVgsTaskAttr.stImgIn, sizeof(VIDEO_FRAME_INFO_S));
    }
    else
    {
        s32Ret = SAMPLE_VGS_GetFrameVb(pParam->pstOutImgVbInfo, &stOutImgVbCalConfig, &stVgsTaskAttr.stImgOut,
                &g_stOutImgVbInfo);
        if (s32Ret != SC_SUCCESS)
        {
            goto EXIT2;
        }
    }

    printf("SC_MPI_VGS_BeginJob\n");

    /************************************************
    step3:  Create VGS job
    *************************************************/
    s32Ret = SC_MPI_VGS_BeginJob(&hHandle);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VGS_BeginJob failed, s32Ret:0x%x", s32Ret);
        goto EXIT3;
    }

    printf("SC_MPI_VGS_BeginJob end\n");

    /************************************************
    step4:  Add VGS task
    *************************************************/
    if (pParam->bScale)
    {
        printf("SC_MPI_VGS_AddScaleTask\n");
        s32Ret = SC_MPI_VGS_AddScaleTask(hHandle, &stVgsTaskAttr, *pParam->penVgsSclCoefMode);
        if (s32Ret != SC_SUCCESS)
        {

            printf("SC_MPI_VGS_AddScaleTask!!!!!!!!!!\n");

            SC_MPI_VGS_CancelJob(hHandle);
            SAMPLE_PRT("SC_MPI_VGS_AddScaleTask failed, s32Ret:0x%x", s32Ret);
            goto EXIT3;
        }
    }

    if (pParam->bCover)
    {
        printf("SC_MPI_VGS_AddCoverTask\n");
        s32Ret = SC_MPI_VGS_AddCoverTask(hHandle, &stVgsTaskAttr, pParam->pstVgsAddCover);
        if (s32Ret != SC_SUCCESS)
        {
            SC_MPI_VGS_CancelJob(hHandle);
            SAMPLE_PRT("SC_MPI_VGS_AddCoverTask failed, s32Ret:0x%x", s32Ret);
            goto EXIT3;
        }
    }

    if (pParam->bMosaic)
    {
        printf("SC_MPI_VGS_AddMosaicTask\n");
        s32Ret = SC_MPI_VGS_AddMosaicTask(hHandle, &stVgsTaskAttr, pParam->pstVgsAddMosaic);
        if (s32Ret != SC_SUCCESS)
        {
            SC_MPI_VGS_CancelJob(hHandle);
            SAMPLE_PRT("SC_MPI_VGS_AddMosaicTask failed, s32Ret:0x%x", s32Ret);
            goto EXIT3;
        }
    }

    if (pParam->bOsd)
    {
        printf("SAMPLE_VGS_PrepareOsdInfo\n");
        s32Ret = SAMPLE_VGS_PrepareOsdInfo(pParam->pstVgsAddOsd, pParam->pRgnHandle);
        if (s32Ret != SC_SUCCESS)
        {
            SC_MPI_VGS_CancelJob(hHandle);
            SAMPLE_PRT("SAMPLE_VGS_PrepareOsdInfo failed, s32Ret:0x%x", s32Ret);
            goto EXIT3;
        }

        printf("SC_MPI_VGS_AddOsdTask stride %d, w %d, h%d!", pParam->pstVgsAddOsd->u32Stride,
            pParam->pstVgsAddOsd->stRect.u32Width, pParam->pstVgsAddOsd->stRect.u32Height);

        s32Ret = SC_MPI_VGS_AddOsdTask(hHandle, &stVgsTaskAttr, pParam->pstVgsAddOsd);
        if (s32Ret != SC_SUCCESS)
        {
            SC_MPI_VGS_CancelJob(hHandle);
            SAMPLE_PRT("SC_MPI_VGS_AddOsdTask failed, s32Ret:0x%x", s32Ret);
            goto EXIT3;
        }
    }

    if (pParam->bDrawLine)
    {
        printf("SC_MPI_VGS_AddDrawLineTask \n");

        s32Ret = SC_MPI_VGS_AddDrawLineTask(hHandle, &stVgsTaskAttr, pParam->pstVgsDrawLine);
        if (s32Ret != SC_SUCCESS)
        {
            SC_MPI_VGS_CancelJob(hHandle);
            SAMPLE_PRT("SC_MPI_VGS_AddDrawLineTask failed, s32Ret:0x%x", s32Ret);
            goto EXIT3;
        }
    }

    if (pParam->bRotate)
    {
        s32Ret = SC_MPI_VGS_AddRotationTask(hHandle, &stVgsTaskAttr, *pParam->penRotationAngle);
        if (s32Ret != SC_SUCCESS)
        {
            SC_MPI_VGS_CancelJob(hHandle);
            SAMPLE_PRT("SC_MPI_VGS_AddRotationTask failed, s32Ret:0x%x", s32Ret);
            goto EXIT3;
        }
    }

    if(pParam->bLuma)
    {
        s32Ret = SC_MPI_VGS_AddLumaTaskArray(hHandle, &stVgsTaskAttr, pParam->pstVgsGetLumaData->pstRect,
                pParam->pstVgsGetLumaData->u32ArraySize, pParam->pstVgsGetLumaData->pu64LumaRet);
        if (s32Ret != SC_SUCCESS)
        {
            SC_MPI_VGS_CancelJob(hHandle);
            SAMPLE_PRT("SC_MPI_VGS_AddRotationTask failed, s32Ret:0x%x", s32Ret);
            goto EXIT3;
        }
    }

    /************************************************
    step5:  Start VGS work
    *************************************************/

    printf("SC_MPI_VGS_EndJob\n");

    s32Ret = SC_MPI_VGS_EndJob(hHandle);
    if (s32Ret != SC_SUCCESS)
    {
        SC_MPI_VGS_CancelJob(hHandle);
        SAMPLE_PRT("SC_MPI_VGS_EndJob failed, s32Ret:0x%x", s32Ret);
        goto EXIT3;
    }

    /************************************************
    step6:  Save the frame to file
    *************************************************/
    s32Ret = SAMPLE_VGS_SaveSP42XToPlanar(pFileWrite, &stVgsTaskAttr.stImgOut.stVFrame);
    if (s32Ret != SC_SUCCESS)
    {
        goto EXIT3;
    }

    fflush(pFileWrite);

    /************************************************
    step7:  Exit
    *************************************************/
EXIT3:
    if (!bSameVB)
    {
        SC_MPI_SYS_Munmap((SC_VOID *)(SC_UL)stVgsTaskAttr.stImgOut.stVFrame.u64HeaderVirAddr[0], stOutImgVbCalConfig.u32VBSize);
        SC_MPI_VB_ReleaseBlock(g_stOutImgVbInfo.VbHandle);
        g_stOutImgVbInfo.bVbUsed = SC_FALSE;
    }
EXIT2:
    fclose(pFileWrite);
EXIT1:
    SC_MPI_SYS_Munmap((SC_VOID *)(SC_UL)stVgsTaskAttr.stImgIn.stVFrame.u64HeaderVirAddr[0], stInImgVbCalConfig.u32VBSize);
    SC_MPI_VB_ReleaseBlock(g_stInImgVbInfo.VbHandle);
    g_stInImgVbInfo.bVbUsed = SC_FALSE;
EXIT:
    if (pParam->pRgnHandle)
    {
        printf("SC_MPI_RGN_Destroy\n");

        SC_MPI_RGN_Destroy(*pParam->pRgnHandle);
    }

    printf("SAMPLE_COMM_SYS_Exit\n");

    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

static SC_S32 SAMPLE_VGS_Scale(SC_VOID)
{
    SC_S32 s32Ret = SC_FAILURE;
    SAMPLE_VGS_FUNC_PARAM stVgsFuncParam;
    SAMPLE_VB_BASE_INFO_S stInImgVbInfo;
    SAMPLE_VB_BASE_INFO_S stOutImgVbInfo;
    VGS_SCLCOEF_MODE_E enVgsSclCoefMode = VGS_SCLCOEF_NORMAL;

    stInImgVbInfo.enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    stInImgVbInfo.u32Width = 1920;
    stInImgVbInfo.u32Height = 1080;
    stInImgVbInfo.u32Align = 0;
    stInImgVbInfo.enCompressMode = COMPRESS_MODE_NONE;

    memcpy(&stOutImgVbInfo, &stInImgVbInfo, sizeof(SAMPLE_VB_BASE_INFO_S));
    stOutImgVbInfo.u32Width = 1280;
    stOutImgVbInfo.u32Height = 720;

    memset(&stVgsFuncParam, 0, sizeof(SAMPLE_VGS_FUNC_PARAM));
    stVgsFuncParam.bScale = SC_TRUE;
    stVgsFuncParam.penVgsSclCoefMode = &enVgsSclCoefMode;
    stVgsFuncParam.pstInImgVbInfo = &stInImgVbInfo;
    stVgsFuncParam.pstOutImgVbInfo = &stOutImgVbInfo;
    stVgsFuncParam.s32SampleNum = 0;

    s32Ret = SAMPLE_VGS_COMMON_FUNCTION(&stVgsFuncParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("VGS Sample %d failed, s32Ret:0x%x", stVgsFuncParam.s32SampleNum, s32Ret);
    }

    return s32Ret;
}


static SC_S32 SAMPLE_VGS_Cover_Osd(SC_VOID)
{
    SC_S32 s32Ret = SC_FAILURE;
    SC_S32 i = 0;
    RGN_HANDLE rgnHandle = 0;
    SAMPLE_VGS_FUNC_PARAM stVgsFuncParam;
    SAMPLE_VB_BASE_INFO_S stInImgVbInfo;

    VGS_ADD_COVER_S stVgsAddCover;
    VGS_ADD_OSD_S stVgsAddOsd;
    VGS_ADD_MOSAIC_S stVgsAddMosaic;
    POINT_S stPoint[4] = { {50, 50}, {50, 200}, {500, 50}, {500, 200} };

    stInImgVbInfo.enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    stInImgVbInfo.u32Width = 1920;
    stInImgVbInfo.u32Height = 1080;
    stInImgVbInfo.u32Align = 0;
    stInImgVbInfo.enCompressMode = COMPRESS_MODE_NONE;

    stVgsAddCover.u32Color = 0xFFFFFF;

    #if 1
    stVgsAddCover.enCoverType = COVER_RECT;
    stVgsAddCover.stDstRect.s32X = 100;
    stVgsAddCover.stDstRect.s32Y = 100;
    stVgsAddCover.stDstRect.u32Width = 200;
    stVgsAddCover.stDstRect.u32Height = 200;

    #else
    stVgsAddCover.enCoverType = COVER_QUAD_RANGLE;
    stVgsAddCover.stQuadRangle.bSolid = SC_TRUE;
    stVgsAddCover.stQuadRangle.u32Thick = 4;
    for (; i < 4; ++i)
    {
        stVgsAddCover.stQuadRangle.stPoint[i] = stPoint[i];
    }
    #endif

    stVgsAddOsd.stRect.s32X = 600;
    stVgsAddOsd.stRect.s32Y = 400;
    stVgsAddOsd.stRect.u32Width = 256;
    stVgsAddOsd.stRect.u32Height = 49;
    stVgsAddOsd.u32BgColor = 0xFF0000;
    stVgsAddOsd.enPixelFmt = PIXEL_FORMAT_ARGB_8888;
    stVgsAddOsd.u64PhyAddr = 0;
    stVgsAddOsd.u32Stride = 0;
    stVgsAddOsd.u32BgAlpha = 255;
    stVgsAddOsd.u32FgAlpha = 128;
    stVgsAddOsd.bOsdRevert = SC_FALSE;
    stVgsAddOsd.stOsdRevert.stSrcRect = stVgsAddOsd.stRect;
    stVgsAddOsd.stOsdRevert.enColorRevertMode = VGS_COLOR_REVERT_RGB;

    stVgsAddMosaic.stRect.s32X = 200;
    stVgsAddMosaic.stRect.s32Y = 100;
    stVgsAddMosaic.stRect.u32Width = 256;
    stVgsAddMosaic.stRect.u32Height = 49;
    stVgsAddMosaic.bBgAlpha = 0;
    stVgsAddMosaic.u32BgAlpha = 255;
    stVgsAddMosaic.enBlkSize = VGS_MOSAIC_BLK_SIZE_8;

    memset(&stVgsFuncParam, 0, sizeof(SAMPLE_VGS_FUNC_PARAM));
    stVgsFuncParam.bCover = SC_TRUE;
    stVgsFuncParam.pstVgsAddCover = &stVgsAddCover;
    stVgsFuncParam.bOsd = SC_TRUE;
    stVgsFuncParam.pstVgsAddOsd = &stVgsAddOsd;
    stVgsFuncParam.pRgnHandle = &rgnHandle;
    stVgsFuncParam.pstInImgVbInfo = &stInImgVbInfo;
    stVgsFuncParam.pstOutImgVbInfo = SC_NULL;
    stVgsFuncParam.s32SampleNum = 1;

    stVgsFuncParam.bMosaic = SC_TRUE;
    stVgsFuncParam.pstVgsAddMosaic = &stVgsAddMosaic;

    s32Ret = SAMPLE_VGS_COMMON_FUNCTION(&stVgsFuncParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("VGS Sample %d failed, s32Ret:0x%x", stVgsFuncParam.s32SampleNum, s32Ret);
    }

    return s32Ret;
}

static SC_S32 SAMPLE_VGS_DrawLine(SC_VOID)
{
    SC_S32 s32Ret = SC_FAILURE;
    SAMPLE_VGS_FUNC_PARAM stVgsFuncParam;
    SAMPLE_VB_BASE_INFO_S stInImgVbInfo;
    VGS_DRAW_LINE_S stVgsDrawLine;

    stInImgVbInfo.enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    stInImgVbInfo.u32Width = 1920;
    stInImgVbInfo.u32Height = 1080;
    stInImgVbInfo.u32Align = 0;
    stInImgVbInfo.enCompressMode = COMPRESS_MODE_NONE;

    stVgsDrawLine.stStartPoint.s32X = 480;
    stVgsDrawLine.stStartPoint.s32Y = 500;
    stVgsDrawLine.stEndPoint.s32X = 960;
    stVgsDrawLine.stEndPoint.s32Y = 500;
    stVgsDrawLine.u32Thick = 2;
    stVgsDrawLine.u32Color = 0x0000ff;

    memset(&stVgsFuncParam, 0, sizeof(SAMPLE_VGS_FUNC_PARAM));
    stVgsFuncParam.bDrawLine = SC_TRUE;
    stVgsFuncParam.pstVgsDrawLine = &stVgsDrawLine;
    stVgsFuncParam.pstInImgVbInfo = &stInImgVbInfo;
    stVgsFuncParam.pstOutImgVbInfo = SC_NULL;
    stVgsFuncParam.s32SampleNum = 2;

    s32Ret = SAMPLE_VGS_COMMON_FUNCTION(&stVgsFuncParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("VGS Sample %d failed, s32Ret:0x%x", stVgsFuncParam.s32SampleNum, s32Ret);
    }

    return s32Ret;
}

static SC_S32 SAMPLE_VGS_Rotate(SC_VOID)
{
    SC_S32 s32Ret = SC_FAILURE;
    SAMPLE_VGS_FUNC_PARAM stVgsFuncParam;
    ROTATION_E enRotationAngle = ROTATION_90;
    SAMPLE_VB_BASE_INFO_S stInImgVbInfo;
    SAMPLE_VB_BASE_INFO_S stOutImgVbInfo;

    stInImgVbInfo.enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    stInImgVbInfo.u32Width = 1920;
    stInImgVbInfo.u32Height = 1080;
    stInImgVbInfo.u32Align = 64;
    stInImgVbInfo.enCompressMode = COMPRESS_MODE_NONE;

    memcpy(&stOutImgVbInfo, &stInImgVbInfo, sizeof(SAMPLE_VB_BASE_INFO_S));
    stOutImgVbInfo.u32Width = 1080;
    stOutImgVbInfo.u32Height = 1920;
    stOutImgVbInfo.u32Align = 64;

    memset(&stVgsFuncParam, 0, sizeof(SAMPLE_VGS_FUNC_PARAM));
    stVgsFuncParam.bRotate = SC_TRUE;
    stVgsFuncParam.penRotationAngle = &enRotationAngle;
    stVgsFuncParam.pstInImgVbInfo = &stInImgVbInfo;
    stVgsFuncParam.pstOutImgVbInfo = &stOutImgVbInfo;
    stVgsFuncParam.s32SampleNum = 3;

    s32Ret = SAMPLE_VGS_COMMON_FUNCTION(&stVgsFuncParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("VGS Sample %d failed, s32Ret:0x%x", stVgsFuncParam.s32SampleNum, s32Ret);
    }

    return s32Ret;
}

static SC_S32 SAMPLE_VGS_Luma(SC_VOID)
{
    SC_S32 s32Ret = SC_FAILURE;
    SAMPLE_VGS_FUNC_PARAM stVgsFuncParam;
    SAMPLE_VB_BASE_INFO_S stInImgVbInfo;
    VGS_GET_LUMA_DATA_S strLumaData;
    strLumaData.u32ArraySize = 2;
    strLumaData.pstRect = (RECT_S *)malloc(sizeof(RECT_S) * 2);
    strLumaData.pstRect[0].s32X = 0;
    strLumaData.pstRect[0].s32Y = 0;
    strLumaData.pstRect[0].u32Width = 500;
    strLumaData.pstRect[0].u32Height = 500;
    strLumaData.pstRect[1].s32X = 0;
    strLumaData.pstRect[1].s32Y = 0;
    strLumaData.pstRect[1].u32Width = 600;
    strLumaData.pstRect[1].u32Height = 600;
    strLumaData.pu64LumaRet = (SC_U64 *)malloc(sizeof(SC_U64) * 2);

    stInImgVbInfo.enPixelFormat = PIXEL_FORMAT_YVU_PLANAR_420;
    stInImgVbInfo.u32Width = 1920;
    stInImgVbInfo.u32Height = 1080;
    stInImgVbInfo.u32Align = 0;
    stInImgVbInfo.enCompressMode = COMPRESS_MODE_NONE;

    memset(&stVgsFuncParam, 0, sizeof(SAMPLE_VGS_FUNC_PARAM));
    stVgsFuncParam.bLuma = SC_TRUE;
    stVgsFuncParam.pstVgsGetLumaData = &strLumaData;
    stVgsFuncParam.pstInImgVbInfo = &stInImgVbInfo;
    stVgsFuncParam.pstOutImgVbInfo = NULL;
    stVgsFuncParam.s32SampleNum = 4;

    s32Ret = SAMPLE_VGS_COMMON_FUNCTION(&stVgsFuncParam);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("VGS Sample %d failed, s32Ret:0x%x", stVgsFuncParam.s32SampleNum, s32Ret);
    }

    for (size_t i = 0; i < strLumaData.u32ArraySize; i++)
    {
        printf("get luma data %lld, from rect(%d,%d,%d,%d)\n ", strLumaData.pu64LumaRet[i], strLumaData.pstRect[i].s32X,
            strLumaData.pstRect[i].s32Y, strLumaData.pstRect[i].u32Width, strLumaData.pstRect[i].u32Height);
    }

    return s32Ret;
}

/******************************************************************************
* function    : main()
* Description : video vgs sample
******************************************************************************/
SC_S32 main(SC_S32 argc, SC_CHAR *argv[])
{
    SC_S32 s32Ret = SC_FAILURE;
    SC_S32 s32Index = -1;

    if (argc < 2)
    {
        SAMPLE_VGS_Usage(argv[0]);
        return s32Ret;
    }

    if (!strncmp(argv[1], "-h", 2))
    {
        SAMPLE_VGS_Usage(argv[0]);
        return SC_SUCCESS;
    }

    signal(SIGINT, SAMPLE_VGS_HandleSig);
    signal(SIGTERM, SAMPLE_VGS_HandleSig);

    s32Index = atoi(argv[1]);
    if (!s32Index && strncmp(argv[1], "0", 1))
    {
        s32Index = -1;
    }

    usleep(100 * 1000);

    switch (s32Index)
    {
    case 0:
        s32Ret = SAMPLE_VGS_Scale();
        break;
    case 1:
        s32Ret = SAMPLE_VGS_Cover_Osd();
        break;
    case 2:
        s32Ret = SAMPLE_VGS_DrawLine();
        break;
    case 3:
        s32Ret = SAMPLE_VGS_Rotate();
        break;
    case 4:
        s32Ret = SAMPLE_VGS_Luma();
        break;
    default:
        SAMPLE_PRT("the index is invaild!\n");
        SAMPLE_VGS_Usage(argv[0]);
        s32Ret = SC_FAILURE;
        break;
    }

    if (SC_SUCCESS == s32Ret)
    {
        SAMPLE_PRT("program exit normally!\n");
    }
    else
    {
        SAMPLE_PRT("program exit abnormally!\n");
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
