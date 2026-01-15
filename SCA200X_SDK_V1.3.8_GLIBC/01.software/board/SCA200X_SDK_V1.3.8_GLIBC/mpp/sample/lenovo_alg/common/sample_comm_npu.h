#ifndef __SAMPLE_COMM_NPU_H__
#define __SAMPLE_COMM_NPU_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "mpi_npu.h"

#define RGB_RAW_DATA_FILE     1
#define BGR_RAW_DATA_FILE     2
#define JPG_BMP_PNG_IMG_FILE  3
#define RGBD_RAW_DATA_FILE    4
#define INVALID_IMG_FILE      (-1)

#define ALIGNED_256B(x) ((x)%256 == 0 ? (x) : (((x)/256 + 1) * 256))
#define ALIGN16KB        (16384)

typedef struct scSAMPLE_SVP_NPU_MODEL_S
{
    SVP_NPU_MODEL_S        stModel;
    SVP_NPU_MODEL_CONF_S   stModelConf;
} SAMPLE_SVP_NPU_MODEL_S;

typedef struct scSAMPLE_SVP_NPU_PARAM_S
{
    SVP_MEM_INFO_S          stIntMem;
    SVP_MEM_INFO_S          stOutMem;
    SVP_MEM_INFO_S          PictMem;
} SAMPLE_SVP_NPU_PARAM_S;

typedef struct scSAMPLE_SVP_NPU_DCNN_GETTOPN_UNIT_S
{
    unsigned int     u32ClassId;
    float            u32Confidence;
} SAMPLE_SVP_NPU_DCNN_OUTPUT;

typedef struct scSAMPLE_SVP_NPU_DETECTION_OUTPUT
{
    SC_FLOAT x;
    SC_FLOAT y;
    SC_FLOAT w;
    SC_FLOAT h;
    sc_u32 classId;
    SC_FLOAT confidence;
} SAMPLE_SVP_NPU_DETECTION_OUTPUT;

//usr-defined callback operator params
//custom-LRN
typedef struct scSAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S
{
    SC_U32 u32Size;
    SC_DOUBLE dAlpha;
    SC_DOUBLE dBeta;
    SC_DOUBLE dBias;
    SC_CHAR achNormRegion[256];
} SAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S;

/*****************************************************************************
 *   Prototype    : SAMPLE_COMM_SVP_NPU_UnloadModel
 *   Description  : unload NPU model
 *   Input        : SAMPLE_SVP_NPU_MODEL_S *pstNpuModel      NPU Model
 *
 *
 *
 *   Output       :
 *   Return Value : SC_S32,SC_SUCCESS:Success,Other:failure
 *****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_UnloadModel(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel);

/*****************************************************************************
*   Prototype    : SAMPLE_COMM_SVP_NPU_LoadModel
*   Description  : load NPU model
*   Input        : SC_CHAR                 * pszModelFile    Model file name
*                  SAMPLE_SVP_NPU_MODEL_S *pstNpuModel   NPU Model
*
*
*
*   Output       :
*   Return Value : SC_S32,SC_SUCCESS:Success,Other:failure
*****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_LoadModel(SC_CHAR *pszModelFile, SAMPLE_SVP_NPU_MODEL_S *pstNpuModel);

/*****************************************************************************
 *   Prototype    : SAMPLE_COMM_SVP_NPU_ParamInit
 *   Description  : Init NPU parameters
 *   Input        : SAMPLE_SVP_NPU_MODEL_S   *pstNpuModel    NPU model info
 *                : SAMPLE_SVP_NPU_PARAM_S   *pstNpuParam    NPU parameters
 *
 *
 *
 *   Output       :
 *   Return Value : SC_S32,SC_SUCCESS:Success,Other:failure
 *****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_ParamInit(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel, SAMPLE_SVP_NPU_PARAM_S *pstNpuParam);

/*****************************************************************************
 *   Prototype    : SAMPLE_COMM_SVP_NPU_ParamDeinit
 *   Description  : Deinit NPU parameters
 *   Input        : SAMPLE_SVP_NPU_PARAM_S        *pstNpuParam     NPU Parameter
 *
 *
 *
 *
 *   Output       :
 *   Return Value :  SC_S32,SC_SUCCESS:Success,Other:failure
 *****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_ParamDeinit(SAMPLE_SVP_NPU_PARAM_S *pstNpuParam);

/*****************************************************************************
 *   Prototype    : SAMPLE_COMM_SVP_NPU_FillSrcData
 *   Description  : Fill Src Data to NPU IMAGE
 *   Input        : SC_CHAR                 *pcSrcFile      Source File
 *                : SAMPLE_SVP_NPU_PARAM_S  *pstNpuParam    NPU Parameter
 *                : NPU_IMG_SET_S           *pstImg         NPU Image
 *                : SVP_IMAGE_TYPE_E        image_type      NPU Image type
 *
 *
 *   Output       :
 *   Return Value :  SC_S32,SC_SUCCESS:Success,Other:failure
 *****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_FillSrcData(SC_CHAR *pcSrcFile, SAMPLE_SVP_NPU_PARAM_S *pstNpuParam, NPU_IMAGE_S *pstImg,
    SVP_IMAGE_TYPE_E image_type);

SC_S32 SAMPLE_COMM_SVP_NPU_RegistParseFunc(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel, SC_CHAR *chName,
    SC_NPU_ParseFunc pParseFunc);

SC_S32 SAMPLE_COMM_SVP_NPU_RegisterCallbackFunc(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel, SC_CHAR *chName,
    SC_NPU_CallbackFunc pParseFunc);

SC_S32 SC_CNN_OperatorLrn(sc_uintptr_t pIn, sc_uintptr_t pOut, NPU_CB_PARAM_S *pCBParams, SC_VOID *pOpParams);

SC_S32 SC_CNN_ParseCustomLrn(const void *pJsonNode, SC_U32 u32JsonLen, SC_VOID **pOpParams, SC_U32 *pParamLen);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SAMPLE_COMM_NPU_H__ */

