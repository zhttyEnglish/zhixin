#ifndef __SAMPLE_NPU_MAIN_H__
#define __SAMPLE_NPU_MAIN_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#include "sc_type.h"
#include "sc_comm_svp.h"
#include "mpi_npu.h"
#include "cJSON.h"
#include "sample_comm_svp.h"
#include "sample_comm_npu.h"
#include "utils.h"


/******************************************************************************
* function : YOLOV8
******************************************************************************/
// int SVP_NPU_YOLOv8(const char* image_path, const char* model_path, cJSON *root);

// SC_S32 SVP_NPU_YOLOv8_LoadModel(const char* pcModelName,
//                                 SAMPLE_SVP_NPU_MODEL_S *pstModel,
//                                 SAMPLE_SVP_NPU_PARAM_S *pstParam);

// SC_S32 SVP_NPU_YOLOv8_InferImage(SAMPLE_SVP_NPU_MODEL_S *pstModel,
//                                  SAMPLE_SVP_NPU_PARAM_S *pstParam,
//                                  NPU_IMAGE_S *pstImg,
//                                  cJSON *root);

// void SVP_NPU_YOLOv8_Deinit(SAMPLE_SVP_NPU_MODEL_S *pstModel,
//                            SAMPLE_SVP_NPU_PARAM_S *pstParam);

#endif /* __SAMPLE_NPU_MAIN_H__ */