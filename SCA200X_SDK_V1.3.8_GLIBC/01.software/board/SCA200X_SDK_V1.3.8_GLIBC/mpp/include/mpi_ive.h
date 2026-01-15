#ifndef __MPI_IVE_H__
#define __MPI_IVE_H__

#include "sc_ive.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/*****************************************************************************
*   Prototype    : SC_MPI_IVE_CSC
*   Description  : YUV2RGB\YUV2HSV\YUV2LAB\RGB2YUV color space conversion are supported.
*   Parameters   : IVE_HANDLE         *pIveHandle       Returned handle ID of a task
*                  IVE_SRC_IMAGE_S    *pstSrc           Input source data:
*                                                       1. SP420\SP422 type for YUV2RGB\YUV2HSV\YUV2LAB;
*                                                       2. U8C3_PACKAGE\U8C3_PLANAR type for RGB2YUV;
*                  IVE_DST_IMAGE_S    *pstDst           Output result:
*                                                       1. U8C3_PACKAGE\U8C3_PLANAR typed for YUV2RGB\YUV2HSV\YUV2LAB;
*                                                       2. SP420\SP422 type for RGB2YUV;
*                  IVE_CSC_CTRL_S     *pstCscCtrl       Control parameters for CSC
*                  SC_BOOL             bInstant         For details, see SC_MPI_IVE_DMA.
*   Return Value : SC_SUCCESS: Success;Error codes: Failure.
*   Spec         : The size of the input data ranges from 64x64 pixels to 1920x1080 pixels.
*                  The physical addresses of the input data and output data must be 16-byte-aligned.
*                  The stride must be 16-pixel-aligned.
*
*****************************************************************************/
SC_S32 SC_MPI_IVE_CSC(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
    IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCscCtrl, SC_BOOL bInstant);

/*****************************************************************************
*   Prototype    : SC_MPI_IVE_Resize
*   Description  : Resize.
*   Parameters   : IVE_HANDLE          *pIveHandle      Returned handle ID of a task
*                  IVE_SRC_IMAGE_S     astSrc[]         The input source.Only the U8C1/U8C3_PLANAR input format is supported.
*                  IVE_DST_IMAGE_S     astDst[]         Output result.Only the U8C1/U8C3_PLANAR format is supported.
*                  IVE_RESIZE_CTRL_S   *pstResizeCtrl   Control parameter
*                  SC_BOOL             bInstant         For details, see SC_MPI_IVE_DMA.
*   Return Value : SC_SUCCESS: Success;Error codes: Failure.
*   Spec         : The size of the input data ranges from 32x16 pixels to 1920x1080 pixels.
*                  The stride must be 16-pixel-aligned.
*
*****************************************************************************/
SC_S32 SC_MPI_IVE_Resize(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S astSrc[],
    IVE_DST_IMAGE_S astDst[], IVE_RESIZE_CTRL_S *pstResizeCtrl, SC_BOOL bInstant);

SC_S32 SC_MPI_IVE_Resize2(IVE_SRC_IMAGE_S astSrc[], IVE_DST_IMAGE_S astDst[], IVE_RESIZE2_CTRL_S *pstResizeCtrl);

SC_S32 SC_MPI_IVE_Resize3(IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst, IVE_CROP_INFO_S *pstCropInfo);

SC_S32 SC_MPI_IVE_Resize4(IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst, IVE_CROP_INFO_S *pstCropInfo, SC_U32 u32ImgNum);

/*****************************************************************************
*   Prototype    : SC_MPI_IVE_Query
*   Description  : This API is used to query the status of a called function by using the returned IveHandle of the function.
                   In block mode, the system waits until the function that is being queried is called.
                   In non-block mode, the current status is queried and no action is taken.
*   Parameters   : IVE_HANDLE     IveHandle     IveHandle of a called function. It is entered by users.
*                  SC_BOOL       *pbFinish      Returned status
*                  SC_BOOL        bBlock        Flag indicating the block mode or non-block mode
*                  SC_BOOL  *pbFinish
*   Return Value : SC_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*****************************************************************************/
SC_S32 SC_MPI_IVE_Query(IVE_HANDLE IveHandle, SC_BOOL *pbFinish, SC_BOOL bBlock);
SC_S32 SC_MPI_GDC_Transform(GDC_TRANSFORM_S *pstParam);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* end of __SAPI_VGS_H__ */
