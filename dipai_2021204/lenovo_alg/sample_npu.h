#ifndef __SAMPLE_NPU_H__
#define __SAMPLE_NPU_H__

#include "cJSON.h"
#include "sample_npu_main.h"
#include "sample_svp_npu_software.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 加载 YOLOv8 模型 */
SC_S32 SVP_NPU_YOLOv8_LoadModel(const char* pcModelName,
                                SAMPLE_SVP_NPU_MODEL_S *pstModel,
                                SAMPLE_SVP_NPU_PARAM_S *pstParam,
                                const char *lic_file_path,
                                const char *public_key_path);

// /* 推理图片（输入 NPU_IMAGE_S 格式）*/
// SC_S32 SVP_NPU_YOLOv8_InferImage(SAMPLE_SVP_NPU_MODEL_S *pstModel,
//                                  SAMPLE_SVP_NPU_PARAM_S *pstParam,
//                                  NPU_IMAGE_S *pstImg,
//                                  cJSON *root);


SC_S32 SVP_NPU_YOLOv8_Predict(SAMPLE_SVP_NPU_MODEL_S *pstModel,
                              SAMPLE_SVP_NPU_PARAM_S *pstParam,
                              const char* pcSrcFile,
                              int oriImgWidth,
                              int oriImgHeight,
                              int resizeImgWidth,
                              int resizeImgHeight,
                              float iou_th,
                              float conf_th,
                              int class_num,
                              int saveFlag,
                              const char* saveFolder,
                              cJSON *root);

/* 释放所有资源 */
void SVP_NPU_YOLOv8_Deinit(SAMPLE_SVP_NPU_MODEL_S *pstModel,
                           SAMPLE_SVP_NPU_PARAM_S *pstParam);

/* Generate License */
SC_S32 SVP_NPU_GenerateLicense();

#ifdef __cplusplus
}
#endif

#endif
