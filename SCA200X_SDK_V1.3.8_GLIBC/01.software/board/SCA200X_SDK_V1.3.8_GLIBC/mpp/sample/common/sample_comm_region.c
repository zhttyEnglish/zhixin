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
#include <errno.h>
#include <pthread.h>
#include "sc_common.h"
#include "sample_comm.h"
#include "loadbmp.h"

#define OverlayMinHandle    0
#define OverlayExMinHandle 20
#define CoverMinHandle     40
#define CoverExMinHandle   60
#define MosaicMinHandle    80
SC_CHAR *Path_BMP = SC_NULL;

SC_S32 REGION_MST_LoadBmp(const char *filename, BITMAP_S *pstBitmap, SC_BOOL bFil, SC_U32 u16FilColor,
    PIXEL_FORMAT_E enPixelFmt)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;
    SC_U32 u32BytePerPix = 0;

    if (GetBmpInfo(filename, &bmpFileHeader, &bmpInfo) < 0)
    {
        printf("GetBmpInfo err!\n");
        return SC_FAILURE;
    }

    if (PIXEL_FORMAT_ARGB_1555 == enPixelFmt)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;
        u32BytePerPix      = 2;
    }
    else if (PIXEL_FORMAT_ARGB_4444 == enPixelFmt)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB4444;
        u32BytePerPix      = 2;
    }
    else if (PIXEL_FORMAT_ARGB_8888 == enPixelFmt)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB8888;
        u32BytePerPix      = 4;
    }
    else
    {
        printf("Pixel format is not support!\n");
        return SC_FAILURE;
    }

    pstBitmap->pData = malloc(u32BytePerPix * (bmpInfo.bmiHeader.biWidth) * (bmpInfo.bmiHeader.biHeight));

    if (NULL == pstBitmap->pData)
    {
        printf("malloc osd memroy err!\n");
        return SC_FAILURE;
    }

    printf("Func: %s, Bitmap user addr pData=%p.....\n", __FUNCTION__, pstBitmap->pData);

    CreateSurfaceByBitMap(filename, &Surface, (SC_U8 *)(pstBitmap->pData));

    pstBitmap->u32Width = Surface.u16Width;
    pstBitmap->u32Height = Surface.u16Height;

    if (PIXEL_FORMAT_ARGB_1555 == enPixelFmt)
    {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_ARGB_1555;
    }
    else if (PIXEL_FORMAT_ARGB_4444 == enPixelFmt)
    {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_ARGB_4444;
    }
    else if (PIXEL_FORMAT_ARGB_8888 == enPixelFmt)
    {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_ARGB_8888;
    }

    int i, j;
    SC_U16 *pu16Temp;

    pu16Temp = (SC_U16 *)pstBitmap->pData;

    if (bFil)
    {
        for (i = 0; i < pstBitmap->u32Height; i++)
        {
            for (j = 0; j < pstBitmap->u32Width; j++)
            {
                if (u16FilColor == *pu16Temp)
                {
                    *pu16Temp &= 0x7FFF;
                }

                pu16Temp++;
            }
        }

    }

    return SC_SUCCESS;
}

SC_S32 REGION_MST_UpdateCanvas(const char *filename, BITMAP_S *pstBitmap, SC_BOOL bFil,
    SC_U32 u16FilColor, SIZE_S *pstSize, SC_U32 u32Stride, PIXEL_FORMAT_E enPixelFmt)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;

    if (GetBmpInfo(filename, &bmpFileHeader, &bmpInfo) < 0)
    {
        printf("GetBmpInfo err!\n");
        return SC_FAILURE;
    }

    if (PIXEL_FORMAT_ARGB_1555 == enPixelFmt)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;
    }
    else if (PIXEL_FORMAT_ARGB_4444 == enPixelFmt)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB4444;
    }
    else if (PIXEL_FORMAT_ARGB_8888 == enPixelFmt)
    {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB8888;
    }
    else
    {
        printf("Pixel format is not support!\n");
        return SC_FAILURE;
    }

    if (NULL == pstBitmap->pData)
    {
        printf("malloc osd memroy err!\n");
        return SC_FAILURE;
    }

    CreateSurfaceByCanvas(filename, &Surface, (SC_U8 *)(pstBitmap->pData), pstSize->u32Width, pstSize->u32Height,
        u32Stride);

    pstBitmap->u32Width = Surface.u16Width;
    pstBitmap->u32Height = Surface.u16Height;

    if (PIXEL_FORMAT_ARGB_1555 == enPixelFmt)
    {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_ARGB_1555;
    }
    else if (PIXEL_FORMAT_ARGB_4444 == enPixelFmt)
    {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_ARGB_4444;
    }
    else if (PIXEL_FORMAT_ARGB_8888 == enPixelFmt)
    {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_ARGB_8888;
    }

    int i, j;
    SC_U16 *pu16Temp;
    pu16Temp = (SC_U16 *)pstBitmap->pData;

    if (bFil)
    {
        for (i = 0; i < pstBitmap->u32Height; i++)
        {
            for (j = 0; j < pstBitmap->u32Width; j++)
            {
                if (u16FilColor == *pu16Temp)
                {
                    *pu16Temp &= 0x7FFF;
                }

                pu16Temp++;
            }
        }

    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_REGION_GetMinHandle(RGN_TYPE_E enType)
{
    SC_S32 MinHandle;
    switch(enType)
    {
    case OVERLAY_RGN:
        MinHandle = OverlayMinHandle;
        break;
    case OVERLAYEX_RGN:
        MinHandle = OverlayExMinHandle;
        break;
    case COVER_RGN:
        MinHandle = CoverMinHandle;
        break;
    case COVEREX_RGN:
        MinHandle = CoverExMinHandle;
        break;
    case MOSAIC_RGN:
        MinHandle = MosaicMinHandle;
        break;
    default:
        MinHandle = -1;
        break;
    }
    return MinHandle;
}

SC_S32 SAMPLE_REGION_CreateOverLay(SC_S32 HandleNum)
{
    SC_S32 s32Ret;
    SC_S32 i;
    RGN_ATTR_S stRegion;

    stRegion.enType = OVERLAY_RGN;
    stRegion.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_ARGB_1555;
    stRegion.unAttr.stOverlay.stSize.u32Height = 200;
    stRegion.unAttr.stOverlay.stSize.u32Width  = 200;
    stRegion.unAttr.stOverlay.u32BgColor = 0x00ff00ff;
    stRegion.unAttr.stOverlay.u32CanvasNum = 5;
    for(i = OverlayMinHandle; i < HandleNum; i++)
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

SC_S32 SAMPLE_REGION_CreateOverLayEx(SC_S32 HandleNum)
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
    for(i = OverlayExMinHandle; i < OverlayExMinHandle + HandleNum; i++)
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

SC_S32 SAMPLE_REGION_CreateCover(SC_S32 HandleNum)
{
    SC_S32 s32Ret;
    SC_S32 i;
    RGN_ATTR_S stRegion;

    stRegion.enType = COVER_RGN;

    for(i = CoverMinHandle; i < CoverMinHandle + HandleNum; i++)
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

SC_S32 SAMPLE_REGION_CreateCoverEx(SC_S32 HandleNum)
{
    SC_S32 s32Ret;
    SC_S32 i;
    RGN_ATTR_S stRegion;

    stRegion.enType = COVEREX_RGN;

    for(i = CoverExMinHandle; i < CoverExMinHandle + HandleNum; i++)
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

SC_S32 SAMPLE_REGION_CreateMosaic(SC_S32 HandleNum)
{
    SC_S32 s32Ret;
    SC_S32 i;
    RGN_ATTR_S stRegion;

    stRegion.enType = MOSAIC_RGN;

    for(i = MosaicMinHandle; i < MosaicMinHandle + HandleNum; i++)
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

SC_S32 SAMPLE_REGION_Destroy(RGN_HANDLE Handle)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_Destroy(Handle);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_Destroy failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_REGION_SetAttr(RGN_HANDLE Handle, RGN_ATTR_S *pstRegion)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_SetAttr(Handle, pstRegion);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_SetAttr failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_REGION_GetAttr(RGN_HANDLE Handle, RGN_ATTR_S *pstRegion)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_Create(Handle, pstRegion);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_Create failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_REGION_AttachToChn(RGN_HANDLE Handle, MPP_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_AttachToChn(Handle, pstChn, pstChnAttr);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_AttachToChn failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_REGION_DetachFromChn(RGN_HANDLE Handle, MPP_CHN_S *pstChn)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_DetachFromChn(Handle, pstChn);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_DetachFromChn failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_REGION_SetDisplayAttr(RGN_HANDLE Handle, MPP_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_SetDisplayAttr(Handle, pstChn, pstChnAttr);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_SetDisplayAttr failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_REGION_GetDisplayAttr(RGN_HANDLE Handle, MPP_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_GetDisplayAttr(Handle, pstChn, pstChnAttr);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_GetDisplayAttr failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_REGION_SetBitMap(RGN_HANDLE Handle, BITMAP_S *pstBitmap)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_SetBitMap(Handle, pstBitmap);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_REGION_GetUpCanvasInfo(RGN_HANDLE Handle, RGN_CANVAS_INFO_S *pstCanvasInfo)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_RGN_GetCanvasInfo(Handle, pstCanvasInfo);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_GetCanvasInfo failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_RGN_UpdateCanvas(Handle);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_UpdateCanvas failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_COMM_REGION_Create(SC_S32 HandleNum, RGN_TYPE_E enType)
{
    SC_S32 s32Ret;
    if(HandleNum <= 0 || HandleNum > 16)
    {
        SAMPLE_PRT("HandleNum is illegal %d!\n", HandleNum);
        return SC_FAILURE;
    }
    if(enType < 0 || enType > 4)
    {
        SAMPLE_PRT("enType is illegal %d!\n", enType);
        return SC_FAILURE;
    }
    switch(enType)
    {
    case OVERLAY_RGN:
        s32Ret = SAMPLE_REGION_CreateOverLay(HandleNum);
        break;
    case OVERLAYEX_RGN:
        s32Ret = SAMPLE_REGION_CreateOverLayEx(HandleNum);
        break;
    case COVER_RGN:
        s32Ret = SAMPLE_REGION_CreateCover(HandleNum);
        break;
    case COVEREX_RGN:
        s32Ret = SAMPLE_REGION_CreateCoverEx(HandleNum);
        break;
    case MOSAIC_RGN:
        s32Ret = SAMPLE_REGION_CreateMosaic(HandleNum);
        break;
    default:
        break;
    }
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_Create failed! HandleNum%d,entype:%d!\n", HandleNum, enType);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_COMM_REGION_Destroy(SC_S32 HandleNum, RGN_TYPE_E enType)
{
    SC_S32 i;
    SC_S32 s32Ret = SC_SUCCESS;
    SC_S32 MinHadle;

    if(HandleNum <= 0 || HandleNum > 16)
    {
        SAMPLE_PRT("HandleNum is illegal %d!\n", HandleNum);
        return SC_FAILURE;
    }
    if(enType < 0 || enType > 4)
    {
        SAMPLE_PRT("enType is illegal %d!\n", enType);
        return SC_FAILURE;
    }
    switch(enType)
    {
    case OVERLAY_RGN:
        MinHadle  = OverlayMinHandle;
        break;
    case OVERLAYEX_RGN:
        MinHadle  = OverlayExMinHandle;
        break;
    case COVER_RGN:
        MinHadle  = CoverMinHandle;
        break;
    case COVEREX_RGN:
        MinHadle  = CoverExMinHandle;
        break;
    case MOSAIC_RGN:
        MinHadle  = MosaicMinHandle;
        break;
    default:
        break;
    }
    for(i = MinHadle; i < MinHadle + HandleNum; i++)
    {
        s32Ret = SAMPLE_REGION_Destroy(i);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_REGION_Destroy failed!\n");
        }
    }
    return s32Ret;
}

SC_S32 SAMPLE_COMM_REGION_AttachToChn(SC_S32 HandleNum, RGN_TYPE_E enType, MPP_CHN_S *pstMppChn)
{
    SC_S32 i;
    SC_S32 s32Ret;
    SC_S32 MinHadle;
    RGN_CHN_ATTR_S stChnAttr;

    if(HandleNum <= 0 || HandleNum > 16)
    {
        SAMPLE_PRT("HandleNum is illegal %d!\n", HandleNum);
        return SC_FAILURE;
    }
    if(enType < 0 || enType > 4)
    {
        SAMPLE_PRT("enType is illegal %d!\n", enType);
        return SC_FAILURE;
    }
    if(SC_NULL == pstMppChn)
    {
        SAMPLE_PRT("pstMppChn is NULL !\n");
        return SC_FAILURE;
    }
    /*set the chn config*/
    stChnAttr.bShow = SC_TRUE;
    switch(enType)
    {
    case OVERLAY_RGN:
        MinHadle = OverlayMinHandle;

        stChnAttr.bShow = SC_TRUE;
        stChnAttr.enType = OVERLAY_RGN;

        stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;
        stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;

        stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable = SC_FALSE;
        stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = SC_TRUE;
        stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 10;

        stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
        stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
        stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
        stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
        stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = SC_FALSE;

        stChnAttr.unChnAttr.stOverlayChn.enAttachDest = ATTACH_JPEG_MAIN;
        stChnAttr.unChnAttr.stOverlayChn.u16ColorLUT[0] = 0x0;
        stChnAttr.unChnAttr.stOverlayChn.u16ColorLUT[1] = 0xf;
        break;
    case OVERLAYEX_RGN:
        MinHadle = OverlayExMinHandle;
        stChnAttr.bShow = SC_TRUE;
        stChnAttr.enType = OVERLAYEX_RGN;

        stChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha = 128;
        stChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = 128;
        break;
    case COVER_RGN:
        MinHadle = CoverMinHandle;

        stChnAttr.bShow = SC_TRUE;
        stChnAttr.enType = COVER_RGN;
        stChnAttr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;

        stChnAttr.unChnAttr.stCoverChn.stRect.u32Height = 200;
        stChnAttr.unChnAttr.stCoverChn.stRect.u32Width  = 200;

        stChnAttr.unChnAttr.stCoverChn.u32Color      = 0x0000ffff;

        stChnAttr.unChnAttr.stCoverChn.enCoordinate = RGN_ABS_COOR;
        break;
    case COVEREX_RGN:
        MinHadle = CoverExMinHandle;

        stChnAttr.bShow = SC_TRUE;
        stChnAttr.enType = COVEREX_RGN;
        stChnAttr.unChnAttr.stCoverExChn.enCoverType = AREA_RECT;

        stChnAttr.unChnAttr.stCoverExChn.stRect.u32Height = 200;
        stChnAttr.unChnAttr.stCoverExChn.stRect.u32Width  = 200;

        stChnAttr.unChnAttr.stCoverExChn.u32Color      = 0x0000ffff;
        break;
    case MOSAIC_RGN:
        MinHadle = MosaicMinHandle;
        stChnAttr.enType = MOSAIC_RGN;
        stChnAttr.unChnAttr.stMosaicChn.enBlkSize = MOSAIC_BLK_SIZE_8;
        stChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = 200;
        stChnAttr.unChnAttr.stMosaicChn.stRect.u32Width  = 200;
        break;
    default:
        break;
    }
    /*attach to Chn*/
    for(i = MinHadle; i < MinHadle + HandleNum; i++)
    {
        if(OVERLAY_RGN == enType)
        {
            stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 20 + 200 * (i - OverlayMinHandle);
            stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 20 + 200 * (i - OverlayMinHandle);
            stChnAttr.unChnAttr.stOverlayChn.u32Layer = i - OverlayMinHandle;
        }
        if(OVERLAYEX_RGN == enType)
        {
            stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = 20 + 200 * (i - OverlayExMinHandle);
            stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = 20 + 200 * (i - OverlayExMinHandle);
            stChnAttr.unChnAttr.stOverlayExChn.u32Layer = i - OverlayExMinHandle;
        }
        if(COVER_RGN == enType)
        {
            stChnAttr.unChnAttr.stCoverChn.stRect.s32X = 20 + 200 * (i - CoverMinHandle) ;
            stChnAttr.unChnAttr.stCoverChn.stRect.s32Y = 20 + 200 * (i - CoverMinHandle);
            stChnAttr.unChnAttr.stCoverChn.u32Layer = i - CoverMinHandle;
        }
        if(COVEREX_RGN == enType)
        {
            stChnAttr.unChnAttr.stCoverExChn.stRect.s32X = 20 + 200 * (i - CoverExMinHandle);
            stChnAttr.unChnAttr.stCoverExChn.stRect.s32Y = 20 + 200 * (i - CoverExMinHandle);
            stChnAttr.unChnAttr.stCoverExChn.u32Layer = i - CoverExMinHandle;
        }
        if(MOSAIC_RGN == enType)
        {
            stChnAttr.unChnAttr.stMosaicChn.stRect.s32X = 20 + 200 * (i - MosaicMinHandle);
            stChnAttr.unChnAttr.stMosaicChn.stRect.s32Y = 20 + 200 * (i - MosaicMinHandle);
            stChnAttr.unChnAttr.stMosaicChn.u32Layer = i - MosaicMinHandle;
        }
        s32Ret = SAMPLE_REGION_AttachToChn(i, pstMppChn, &stChnAttr);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_REGION_AttachToChn failed!\n");
            break;
        }
    }
    /*detach region from chn */
    if(SC_SUCCESS != s32Ret && i > 0)
    {
        i--;
        for(; i >= MinHadle; i--)
        {
            s32Ret = SAMPLE_REGION_DetachFromChn(i, pstMppChn);
        }

    }
    return s32Ret;
}

SC_S32 SAMPLE_COMM_REGION_DetachFrmChn(SC_S32 HandleNum, RGN_TYPE_E enType, MPP_CHN_S *pstMppChn)
{
    SC_S32 i;
    SC_S32 s32Ret = SC_SUCCESS;
    SC_S32 MinHadle;

    if(HandleNum <= 0 || HandleNum > 16)
    {
        SAMPLE_PRT("HandleNum is illegal %d!\n", HandleNum);
        return SC_FAILURE;
    }
    if(enType < 0 || enType > 4)
    {
        SAMPLE_PRT("enType is illegal %d!\n", enType);
        return SC_FAILURE;
    }
    if(SC_NULL == pstMppChn)
    {
        SAMPLE_PRT("pstMppChn is NULL !\n");
        return SC_FAILURE;
    }
    switch(enType)
    {
    case OVERLAY_RGN:
        MinHadle  = OverlayMinHandle;
        break;
    case OVERLAYEX_RGN:
        MinHadle  = OverlayExMinHandle;
        break;
    case COVER_RGN:
        MinHadle  = CoverMinHandle;
        break;
    case COVEREX_RGN:
        MinHadle  = CoverExMinHandle;
        break;
    case MOSAIC_RGN:
        MinHadle  = MosaicMinHandle;
        break;
    default:
        break;
    }
    for(i = MinHadle; i < MinHadle + HandleNum; i++)
    {
        s32Ret = SAMPLE_REGION_DetachFromChn(i, pstMppChn);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_REGION_DetachFromChn failed! Handle:%d\n", i);
        }
    }
    return s32Ret;
}

SC_S32 SAMPLE_COMM_REGION_SetBitMap(RGN_HANDLE Handle)
{
    SC_S32 s32Ret;
    BITMAP_S stBitmap;

    REGION_MST_LoadBmp(Path_BMP, &stBitmap, SC_FALSE, 0, PIXEL_FORMAT_ARGB_1555);
    s32Ret = SAMPLE_REGION_SetBitMap(Handle, &stBitmap);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_REGION_SetBitMap failed!Handle:%d\n", Handle);

    }
    free(stBitmap.pData);
    return s32Ret;
}

SC_S32 SAMPLE_COMM_REGION_GetUpCanvas(RGN_HANDLE Handle)
{
    SC_S32 s32Ret;
    SIZE_S stSize;
    BITMAP_S stBitmap;
    RGN_CANVAS_INFO_S stCanvasInfo;

    s32Ret = SC_MPI_RGN_GetCanvasInfo(Handle, &stCanvasInfo);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_GetCanvasInfo failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    stBitmap.pData   = (SC_VOID *)(SC_UL)stCanvasInfo.u64VirtAddr;
    stSize.u32Width  = stCanvasInfo.stSize.u32Width;
    stSize.u32Height = stCanvasInfo.stSize.u32Height;
    REGION_MST_UpdateCanvas(Path_BMP, &stBitmap, SC_FALSE, 0, &stSize, stCanvasInfo.u32Stride, PIXEL_FORMAT_ARGB_1555);

    s32Ret = SC_MPI_RGN_UpdateCanvas(Handle);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_UpdateCanvas failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_COMM_REGION_GetUpCanvasEx(RGN_HANDLE Handle, char *bmpname)
{
    SC_S32 s32Ret;
    SIZE_S stSize;
    BITMAP_S stBitmap;
    RGN_CANVAS_INFO_S stCanvasInfo;

    s32Ret = SC_MPI_RGN_GetCanvasInfo(Handle, &stCanvasInfo);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_GetCanvasInfo failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    stBitmap.pData   = (SC_VOID *)(SC_UL)stCanvasInfo.u64VirtAddr;
    stSize.u32Width  = stCanvasInfo.stSize.u32Width;
    stSize.u32Height = stCanvasInfo.stSize.u32Height;
    REGION_MST_UpdateCanvas(bmpname, &stBitmap, SC_FALSE, 0, &stSize, stCanvasInfo.u32Stride, PIXEL_FORMAT_ARGB_1555);

    s32Ret = SC_MPI_RGN_UpdateCanvas(Handle);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_RGN_UpdateCanvas failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

