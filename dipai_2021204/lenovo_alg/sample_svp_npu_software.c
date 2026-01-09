
#include "sample_comm_npu.h"
#include "mpi_npu.h"
#include "sample_svp_npu_software.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus    // If used by C++ code,
extern "C" {          // we need to export the C interface
#endif

int ssd_postpross_and_getresult(void *pTensor, int s32TensorNum, uintptr_t virtAddr, void **pResult, int *pResultNum);
int dcnn_postpross_and_getresult(void *pTensor, int s32TensorNum, uintptr_t virtAddr, void **pResult, int *pResultNum);
int yolovx_postpross_and_getresult(void *pTensor, int tensorNum, uintptr_t virtAddr, void **pResult, int *ResultNum,
    int yolovx, int w, int h);

// int get_yolov8_result(void *u64VirAddr,SVP_NPU_MODEL_S model_info,void **pResult, int *ResultNum);
int get_yolov8_result(void *u64VirAddr,
                      SVP_NPU_MODEL_S model_info,
                      void **pResult, 
                      int *ResultNum,
                      float iou_th,
                      float conf_th,
                      int class_num);

int read_img_by_opencv(const char *pcSrcFile,
                       NPU_IMAGE_S *pstImg,
                       SAMPLE_SVP_NPU_PARAM_S *pstNpuParam,
                       int oriImgWidth,
                       int oriImgHeight,
                       int resizeImgWidth,
                       int resizeImgHeight,
                       int saveFlag,
                       const char* saveFolder);

SC_S32 SAMPLE_SVP_NPU_SSD_GetResult(void *pTensor, int s32TensorNum, uintptr_t virtAddr,
    void **pResult, int *pResultNum)
{
    SC_S32 s32Ret = SC_FAILURE;
    s32Ret = ssd_postpross_and_getresult(pTensor, s32TensorNum, virtAddr, pResult, pResultNum);
    return s32Ret;
}

SC_S32 SAMPLE_SVP_NPU_DCNN_GetResult(void *pTensor, int s32TensorNum, uintptr_t virtAddr,
    void **pResult, int *pResultNum)
{
    SC_S32 s32Ret = SC_FAILURE;
    s32Ret = dcnn_postpross_and_getresult(pTensor, s32TensorNum, virtAddr, pResult, pResultNum);
    return s32Ret;
}

SC_S32 SAMPLE_SVP_NPU_YOLOVX_GetResult(void *pTensor, int s32TensorNum, uintptr_t virtAddr,
    void **pResult, int *pResultNum, int yolox, int w, int h)
{
    SC_S32 s32Ret = SC_FAILURE;
    s32Ret = yolovx_postpross_and_getresult(pTensor, s32TensorNum, virtAddr, pResult, pResultNum, yolox, w, h);
    return s32Ret;
}

SC_S32 SAMPLE_SVP_NPU_YOLOV8_GetResult(SVP_NPU_MODEL_S model_info,
                                      void* virtAddr,
                                      void **pResult, 
                                      int *pResultNum, 
                                      float iou_th,
                                      float conf_th,
                                      int class_num)
{
    SC_S32 s32Ret = SC_FAILURE;
    s32Ret = get_yolov8_result(virtAddr,model_info,pResult, pResultNum,iou_th,conf_th,class_num);
    return s32Ret;
}

SC_S32 SAMPLE_SVP_NPU_OPENCV_READ(const char *pcSrcFile,
                                  NPU_IMAGE_S *pstImg,
                                  SAMPLE_SVP_NPU_PARAM_S *pstNpuParam,
                                  int oriImgWidth,
                                  int oriImgHeight,
                                  int resizeImgWidth,
                                  int resizeImgHeight,
                                  int saveFlag,
                                  const char* saveFolder)
{
    SC_S32 s32Ret = SC_FAILURE;
    s32Ret = read_img_by_opencv(pcSrcFile,pstImg,pstNpuParam,oriImgWidth,oriImgHeight,resizeImgWidth,resizeImgHeight,saveFlag,saveFolder);
    return s32Ret;
}

#ifdef __cplusplus
}
#endif

