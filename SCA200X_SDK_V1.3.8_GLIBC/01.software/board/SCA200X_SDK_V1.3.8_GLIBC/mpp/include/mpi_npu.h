#ifndef _SC_MPI_NPU_H_
#define _SC_MPI_NPU_H_

#include "sc_npu.h"
#include "sc_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/*****************************************************************************
*   Prototype    : SC_MPI_SVP_NPU_LoadModel
*   Description  : Load npu model
*   Parameters   : const SVP_NPU_MODEL_CONF_S   *pstModelBuf    Input model config and model buf
*
*                  SVP_NPU_MODEL_S              *pstModel       Output model struct.
*   Return Value : SC_SUCCESS: Success; Error codes: Failure.
*   notes        : When Load SUCESS,pstModelConf Malloc Mem Can be Free
*****************************************************************************/
SC_S32 SC_MPI_SVP_NPU_LoadModel(const SVP_NPU_MODEL_CONF_S *pstModelConf, SVP_NPU_MODEL_S *pstModel);

/*****************************************************************************
*   Prototype    : SC_MPI_SVP_NPU_Forward
*   Description  : Perform CNN prediction on input sample(s), and output responses for corresponding sample(s)
*   Parameters   : SVP_NPU_HANDLE                   *phSvpNpuHandle       Returned handle ID of a task
*                  NPU_IMAGE_S                       pstImg               Input image,stride need 256 align,if input
*                                                                         tensor,pstImg set NULL,fill tensor to pstInMem
*                  const SVP_NPU_MODEL_S            pstModel             net model info
*                  SVP_MEM_INFO_S                    pstInMem             NPU input Mem,mem addr need 16k align
*                  SVP_MEM_INFO_S                    pstoutMem            NPU output Mem,mem addr need 16k align
*                  SC_BOOL                           bInstant             Flag indicating whether to generate an interrupt.
*                                                                         If the output result blocks the next operation,
*                                                                         set bInstant to HI_TRUE.
*   notes        : If use linux IDE tool reasoning,This API pstInMem and pstoutMem use "malloc" function request virAddr
*                  memory,don't need 16k align and phy memory.
*****************************************************************************/
SC_S32 SC_MPI_SVP_NPU_Forward(SVP_NPU_HANDLE *phSvpNpuHandle, NPU_IMAGE_S *pstImg, SVP_MEM_INFO_S *pstInMem,
    const SVP_NPU_MODEL_S *pstModel, SVP_MEM_INFO_S *pstoutMem, SC_bool bInstant);

/*****************************************************************************
*   Prototype    : SC_MPI_SVP_NPU_UnloadModel
*   Description  : Unload npu model
*   Parameters   : SVP_NPU_MODEL_S         *pstModel          Output model
*
*   Return Value : SC_SUCCESS: Success; Error codes: Failure.
*****************************************************************************/
SC_S32 SC_MPI_SVP_NPU_UnloadModel(SVP_NPU_MODEL_S *pstModel);

/*****************************************************************************
*   Prototype    : SC_MPI_SVP_NPU_Query
*   Description  : This API is used to query the status of a function runed on npu.
                   In block mode, the system waits until the function that is being queried is called.
                   In non-block mode, the current status is queried and no action is taken.
*   Parameters   : SVP_NPU_HANDLE       npuHandle      npuHandle of a called function. It is entered by users.
*                  SC_BOOL              *pbFinish       Returned status
*                  SC_BOOL               bBlock         Flag indicating the block mode or non-block mode
*   Return Value : SC_SUCCESS: Success;Error codes: Failure.
*****************************************************************************/
SC_S32 SC_MPI_SVP_NPU_Query(SVP_NPU_HANDLE SvpNpuHandlel, SC_bool *pbFinish, SC_bool bBlock);

/*****************************************************************************
*   Prototype    : SC_MPI_SVP_NPU_RegisterCallback
*   Description  : Register CallBack to arm
*   Parameters   : SVP_NPU_MODEL_S       *pstModel          Output model
*                  SC_CHAR                *acName           node name
*                  SC_NPU_CallbackFunc    pCBFunc           callback fun
*   Return Value : SC_SUCCESS: Success; Error codes: Failure.
*
*****************************************************************************/
SC_S32 SC_MPI_SVP_NPU_RegisterCallback(const SVP_NPU_MODEL_S *pstModel, SC_CHAR *acName,  SC_NPU_CallbackFunc pCBFunc);

/*****************************************************************************
*   Prototype    : SC_MPI_SVP_NPU_RegisterParsefunc
*   Description  : Register Parse json fun to arm
*   Parameters   : SVP_NPU_MODEL_S       *pstModel          Output model
*                  SC_CHAR                *acName           json parse name
*                  SC_NPU_ParseFunc       pParseFunc        json parse fun
*   Return Value : SC_SUCCESS: Success; Error codes: Failure.
*
*****************************************************************************/
SC_S32 SC_MPI_SVP_NPU_RegisterParsefunc(const SVP_NPU_MODEL_S *pstModel, SC_CHAR *acName, SC_NPU_ParseFunc pParseFunc);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*_SC_MPI_NPU_H_*/

