#ifndef _SAMPLE_SVP_USER_KERNEL_H_
#define _SAMPLE_SVP_USER_KERNEL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "sample_comm_npu.h"

SC_S32 SAMPLE_SVP_NPU_DCNN_GetResult(void *pTensor, int s32TensorNum, uintptr_t virtAddr,
    void **pResult, int *pResultNum);

SC_S32 SAMPLE_SVP_NPU_SSD_GetResult(void *pTensor, int s32TensorNum, uintptr_t virtAddr,
    void **pResult, int *pResultNum);

SC_S32 SAMPLE_SVP_NPU_YOLOVX_GetResult(void *pTensor, int s32TensorNum, uintptr_t virtAddr,
    void **pResult, int *pResultNum, int yolovx, int w, int h);

#ifdef __cplusplus
}
#endif

#endif /* _SAMPLE_SVP_USER_KERNEL_H_ */

