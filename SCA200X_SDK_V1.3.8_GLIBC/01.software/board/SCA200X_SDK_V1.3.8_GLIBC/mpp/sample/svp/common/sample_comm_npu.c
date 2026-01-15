#include "sample_comm_npu.h"
#include "sample_comm_svp.h"
#include "cJSON.h"
#include <math.h>

#include <arm_neon.h>

#include "mpi_sys.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))

#define JSON_PARSE_INT_VALUE(structptr, member, node, name) \
do{\
    cJSON * pNode = cJSON_GetObjectItemCaseSensitive(node, name);\
    if(pNode)\
    {\
        structptr->member = pNode->valueint;\
    }\
}while(0)

#define JSON_PARSE_DOUBLE_VALUE(structptr, member, node, name) \
do{\
    cJSON * pNode = cJSON_GetObjectItemCaseSensitive(node, name);\
    if(pNode)\
    {\
        structptr->member = pNode->valuedouble;\
    }\
}while(0)

#define JSON_PARSE_STR_VALUE(structptr, member, node, name) \
do{\
    cJSON * pNode = cJSON_GetObjectItemCaseSensitive(node, name);\
    if(pNode)\
    {\
        strcpy(structptr->member, pNode->valuestring);\
    }\
}while(0)

#define JSON_PARSE_STR_ARRAY_VALUE(structptr, member, node, name) \
do{\
    cJSON * pNode = cJSON_GetObjectItemCaseSensitive(node, name);\
    if(pNode)\
    {\
        SC_U32 u32ArraySize = cJSON_GetArraySize(pNode);\
        for(int idx = 0; idx < u32ArraySize; idx++)\
        {\
            cJSON * pSub = cJSON_GetArrayItem(pNode, idx);\
            if (!pSub) continue;\
            strcpy(structptr->member[idx], pSub->valuestring); \
        }\
    }\
}while(0)

SC_S32  SAMPLE_COMM_SVP_NPU_FillSrcData(char *pcSrcFile, SAMPLE_SVP_NPU_PARAM_S *pstNpuParam,
    NPU_IMAGE_S *pstImg, SVP_IMAGE_TYPE_E enImageType)
{
    SC_U32 u32Width = pstImg->astInputImg[0].stImages[0].u32Width;
    SC_U32 u32Height = pstImg->astInputImg[0].stImages[0].u32Height;

    if(enImageType == SVP_IMG_YUV420P)
    {
        FILE *fp = fopen(pcSrcFile, "r");
        if(fp == NULL)
        {
            SAMPLE_SVP_TRACE_ERR("fopen %s error \n", pcSrcFile);
            return SC_FAILURE;
        }

        fseek (fp, 0, SEEK_END);
        SC_U32 size = ftell (fp);
        if(size != u32Width * u32Height * 3 / 2)
        {
            SAMPLE_SVP_TRACE_ERR("%s need resize %dx%d \n", pcSrcFile, u32Width, u32Height);
            fclose(fp);
            return SC_FAILURE;
        }

        SC_CHAR *buf = (SC_CHAR *)malloc(size);
        if(buf == NULL)
        {
            SAMPLE_SVP_TRACE_ERR("malloc error\n");
            fclose(fp);
            return SC_FAILURE;
        }

        rewind(fp);
        fread(buf, size, 1, fp);
        fclose(fp);

        SC_S32 u16Stride = ALIGNED_256B(u32Width);
        uintptr_t y_addr = pstNpuParam->PictMem.u64VirAddr;
        uintptr_t y_addr_phy = pstNpuParam->PictMem.u64PhyAddr;

        for(int i = 0; i < u32Height; i++)
        {
            memcpy((char *)(y_addr + i * u16Stride), buf + i * u32Width, u32Width);
        }

        char *u = (char *)(y_addr + u16Stride * u32Height);
        SC_CHAR *src = buf + u32Height * u32Width;

        int s1 = ALIGNED_256B(u32Width / 2);
        for(int i = 0; i < u32Height / 2; i++)
        {
            memcpy((char *)(u + i * s1), src + i * u32Width / 2, u32Width / 2);
        }

        char *v = u + u32Height * s1 / 2;

        src += u32Height * u32Width / 4;
        for(int i = 0; i < u32Height / 2; i++)
        {
            memcpy((char *)(v + i * s1), src + i * u32Width / 2, u32Width / 2);
        }

        free(buf);

        //物理地址连续,下面两种方式都可以
        if(0)
        {
            pstImg->astInputImg[0].stImages[0].au64VirAddr[0] = y_addr;
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[0] = y_addr_phy;
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[1] = 0;
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[2] = 0;
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[3] = 0;
        }
        else
        {
            pstImg->astInputImg[0].stImages[0].au64VirAddr[0] = y_addr;
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[0] = y_addr_phy;
            //pstImg->astInputImg[0].stImages[0].au64VirAddr[1] = y_addr + u16Stride*u32Height;
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[1] = y_addr_phy + u16Stride * u32Height;
            //pstImg->astInputImg[0].stImages[0].au64VirAddr[2] = y_addr + u16Stride*u32Height+u32Width*u16Stride/4;
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[2] = y_addr_phy + u16Stride * u32Height + u32Height * s1 / 2;
            pstImg->astInputImg[0].stImages[0].au64VirAddr[3] = 0;
            pstImg->astInputImg[0].stImages[0].au64PhyAddr[3] = 0;
        }

        return 0;
    }
    else if(enImageType == SVP_IMG_RGB)
    {
        FILE *fp = fopen(pcSrcFile, "r");
        if(fp == NULL)
        {
            SAMPLE_SVP_TRACE_ERR("fopen %s error \n", pcSrcFile);
            return SC_FAILURE;
        }

        fseek (fp, 0, SEEK_END);
        SC_U32 size = ftell (fp);
        if(size != u32Width * u32Height * 3)
        {
            SAMPLE_SVP_TRACE_ERR("%s need resize %dx%d \n", pcSrcFile, u32Width, u32Height);
            fclose(fp);
            return SC_FAILURE;
        }

        SC_CHAR *buf = (SC_CHAR *)malloc(size);
        if(buf == NULL)
        {
            SAMPLE_SVP_TRACE_ERR("malloc error\n");
            fclose(fp);
            return SC_FAILURE;
        }

        rewind(fp);
        fread(buf, size, 1, fp);
        fclose(fp);

        unsigned short u16Stride = ALIGNED_256B(u32Width);
        SC_CHAR *pBuf = (SC_CHAR *)((SC_U32)pstNpuParam->PictMem.u64VirAddr);
        SC_CHAR *pchR = buf;
        SC_CHAR *pchG = buf + u32Height * u32Width;
        SC_CHAR *pchB = buf + 2 * u32Height * u32Width;

        for(int i = 0; i < u32Height; i++)
        {
            for(int j = 0; j < u32Width; j++)
            {
                pBuf[i * u16Stride + j]                 = pchR[i * u32Width + j];
                pBuf[(i + u32Height)*u16Stride + j]    = pchG[i * u32Width + j];
                pBuf[(i + u32Height * 2)*u16Stride + j]  = pchB[i * u32Width + j];
            }
            for(int j = u32Width; j < u16Stride; j++)
            {
                pBuf[i * u16Stride + j]                 = 0;
                pBuf[(i + u32Height)*u16Stride + j]    = 0;
                pBuf[(i + u32Height * 2)*u16Stride + j]  = 0;
            }
        }

        pstImg->astInputImg[0].stImages[0].au64VirAddr[0] = (uintptr_t)(pstNpuParam->PictMem.u64VirAddr);
        pstImg->astInputImg[0].stImages[0].au64PhyAddr[0] = (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr);
        pstImg->astInputImg[0].stImages[0].au64PhyAddr[1] = (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr + u16Stride *
                u32Height);
        pstImg->astInputImg[0].stImages[0].au64PhyAddr[2] = (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr + 2 * u16Stride *
                u32Height);
        pstImg->astInputImg[0].stImages[0].au64PhyAddr[3] = 0;

        free(buf);
    }
    else
    {
        SAMPLE_SVP_TRACE_ERR("\n");
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

/*****************************************************************************
*   Prototype    : SAMPLE_COMM_SVP_NPU_ParamDeinit
*   Description  : Deinit NPU parameters
*   Input        : SAMPLE_SVP_NPU_PARAM_S        *pstNpuParam     NNPU Parameter
*
*
*
*
*   Output       :
*   Return Value :  SC_S32,SC_SUCCESS:Success,Other:failure
*   Spec         :
*   Calls        :
*   Called By    :
*   History:
*
*       1.  Date         :
*           Author       :
*           Modification : Create
*
*****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_ParamDeinit(SAMPLE_SVP_NPU_PARAM_S *pstNpuParam)
{

    SAMPLE_SVP_CHECK_EXPR_RET(NULL == pstNpuParam, SC_FAILURE, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error, pstNpuParam can't be NULL!\n");

    if(0 != pstNpuParam->PictMem.u64VirAddr && 0 != pstNpuParam->PictMem.u64PhyAddr)
    {

        SC_MPI_SYS_MmzFree(pstNpuParam->PictMem.u64PhyAddr, (void *)((SC_U32)pstNpuParam->PictMem.u64VirAddr));

        pstNpuParam->PictMem.u64VirAddr = 0;
        pstNpuParam->PictMem.u64PhyAddr = 0;
    }

    if(0 != pstNpuParam->stIntMem.u64VirAddr && 0 != pstNpuParam->stIntMem.u64PhyAddr)
    {
        SC_MPI_SYS_MmzFree(pstNpuParam->stIntMem.u64PhyAddr, (void *)((SC_U32)pstNpuParam->stIntMem.u64VirAddr));
        pstNpuParam->stIntMem.u64VirAddr = 0;
        pstNpuParam->stIntMem.u64PhyAddr = 0;
    }

    if(0 != pstNpuParam->stOutMem.u64VirAddr && 0 != pstNpuParam->stOutMem.u64PhyAddr)
    {
        SC_MPI_SYS_MmzFree(pstNpuParam->stOutMem.u64PhyAddr, (void *)((SC_U32)pstNpuParam->stOutMem.u64VirAddr));
        pstNpuParam->stOutMem.u64VirAddr = 0;
        pstNpuParam->stOutMem.u64PhyAddr = 0;
    }

    return SC_SUCCESS;
}
/*****************************************************************************
 *   Prototype    : SAMPLE_COMM_SVP_NPU_UnloadModel
 *   Description  : unload NPU model
 *   Input        : SAMPLE_SVP_NPU_MODEL_S *pstNpuModel      NPU Model
 *
 *
 *
 *   Output       :
 *   Return Value : SC_S32,SC_SUCCESS:Success,Other:failure
 *   Spec         :
 *   Calls        :
 *   Called By    :
 *   History:
 *
 *       1.  Date         :
 *           Author       :
 *           Modification : Create
 *
 *****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_UnloadModel(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel)
{
    SC_S32 s32Ret = SC_FAILURE;

    if(NULL != pstNpuModel)
    {
        s32Ret = SC_MPI_SVP_NPU_UnloadModel(&pstNpuModel->stModel);
        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,SC_MPI_SVP_NPU_UnloadModel failed!\n");
    }

FAIL_0:
    return s32Ret;
}

/*****************************************************************************
 *   Prototype    : SAMPLE_COMM_SVP_NPU_LoadModel
 *   Description  : load NPU model
 *   Input        : SC_CHAR                 * pszModelFile    Model file name
 *                  SAMPLE_SVP_NPU_MODEL_S   *pstNpuModel    NPU Model
 *
 *
 *
 *   Output       :
 *   Return Value : SC_S32,SC_SUCCESS:Success,Other:failure
 *   Spec         :
 *   Calls        :
 *   Called By    :
 *   History:
 *
 *       1.  Date         :
 *           Author       :
 *           Modification : Create
 *
 *****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_LoadModel(SC_CHAR *pszModelFile, SAMPLE_SVP_NPU_MODEL_S *pstNpuModel)
{
    SC_S32 s32Ret = SC_FAILURE;
    SC_U8 *pu8VirAddr = NULL;
    SC_SL slFileSize = 0;
    /*Get model file size*/
    FILE *fp = fopen(pszModelFile, "rb");
    SAMPLE_SVP_CHECK_EXPR_RET(NULL == fp, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error, open model file failed!\n");
    s32Ret = fseek(fp, 0L, SEEK_END);
    SAMPLE_SVP_CHECK_EXPR_GOTO(-1 == s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error, fseek failed!\n");
    slFileSize = ftell(fp);
    SAMPLE_SVP_CHECK_EXPR_GOTO(slFileSize <= 0, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error, ftell failed!\n");
    s32Ret = fseek(fp, 0L, SEEK_SET);
    SAMPLE_SVP_CHECK_EXPR_GOTO(-1 == s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error, fseek failed!\n");

    pu8VirAddr = (SC_U8 *)malloc(slFileSize);
    SAMPLE_SVP_CHECK_EXPR_GOTO(NULL == pu8VirAddr, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error, malloc failed!\n");

    pstNpuModel->stModelConf.stModelBuf.u64VirAddr = (SC_U32)pu8VirAddr;

    s32Ret = fread(pu8VirAddr, slFileSize, 1, fp);
    SAMPLE_SVP_CHECK_EXPR_GOTO(1 != s32Ret, FAIL_1, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,read model file failed!\n");

    pstNpuModel->stModelConf.stModelBuf.u64Size = slFileSize;

    /*load model*/
    s32Ret = SC_MPI_SVP_NPU_LoadModel(&pstNpuModel->stModelConf, &pstNpuModel->stModel);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, FAIL_1, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,s32Ret %d SC_MPI_SVP_NPU_LoadModel failed!\n", s32Ret);

    free(pu8VirAddr);
    fclose(fp);
    return SC_SUCCESS;
FAIL_1:
    free(pu8VirAddr);
FAIL_0:
    if (NULL != fp)
    {
        fclose(fp);
    }

    return s32Ret;
}

/*****************************************************************************
*   Prototype    : SAMPLE_COMM_SVP_NPU_ParamInit
*   Description  : Init NPU  parameters
*   Input        : SAMPLE_SVP_NPU_CFG_S   *pstNpuCfg      NPU configure parameter
*                  SAMPLE_SVP_NPU_PARAM_S *pstNpuParam    NPU parameters
*
*
*
*   Output       :
*   Return Value : SC_S32,SC_SUCCESS:Success,Other:failure
*   Spec         :
*   Calls        :
*   Called By    :
*   History:
*
*       1.  Date         :
*           Author       :
*           Modification : Create
*
*****************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_ParamInit(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel, SAMPLE_SVP_NPU_PARAM_S *pstNpuParam)
{
    SC_S32 s32Ret = SC_SUCCESS;

    /*check*/
    SAMPLE_SVP_CHECK_EXPR_RET((NULL == pstNpuModel || NULL == pstNpuParam), SC_FAILURE,
        SAMPLE_SVP_ERR_LEVEL_ERROR, "Error,pstNpuModel and pstNpuParam can't be NULL!\n");

    SC_U32 h = pstNpuModel->stModel.astSrcTensor[0].u32Height;
    SC_U32 w = pstNpuModel->stModel.astSrcTensor[0].u32Width;

    SC_U16 u16Stride = ALIGNED_256B(w);
    SC_U32 s32FileSize = u16Stride * h * pstNpuModel->stModel.astSrcTensor[0].u32OriChannels *
        pstNpuModel->stModel.u16SrcNum;

    pstNpuParam->PictMem.u64Size = s32FileSize;

    s32Ret = SC_MPI_SYS_MmzAlloc_Align(&pstNpuParam->PictMem.u64PhyAddr, (void **)&pstNpuParam->PictMem.u64VirAddr,
            "Picbuff",
            NULL, pstNpuParam->PictMem.u64Size, ALIGN16KB);

    SAMPLE_SVP_CHECK_EXPR_RET(s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "SC_MPI_SYS_MmzAlloc_Align error\n");

    memset((char *)((SC_U32)pstNpuParam->PictMem.u64VirAddr), 0, pstNpuParam->PictMem.u64Size);

    pstNpuParam->stIntMem.u64Size = pstNpuModel->stModel.u32InMemSize;

    s32Ret = SC_MPI_SYS_MmzAlloc_Align(&pstNpuParam->stIntMem.u64PhyAddr, (void **)&pstNpuParam->stIntMem.u64VirAddr,
            "inbuff",
            NULL, pstNpuParam->stIntMem.u64Size * 2, ALIGN16KB);
    SAMPLE_SVP_CHECK_EXPR_RET(s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "SC_MPI_SYS_MmzAlloc_Align error\n");

    pstNpuParam->stOutMem.u64Size = pstNpuModel->stModel.u32OutMemSize;
    s32Ret = SC_MPI_SYS_MmzAlloc_Align(&pstNpuParam->stOutMem.u64PhyAddr, (void **)&pstNpuParam->stOutMem.u64VirAddr,
            "Outbuff",
            NULL, pstNpuParam->stOutMem.u64Size * 16, ALIGN16KB);
    SAMPLE_SVP_CHECK_EXPR_RET(s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "SC_MPI_SYS_MmzAlloc_Align error\n");

    return SC_SUCCESS;

}

SC_S32 SAMPLE_COMM_SVP_NPU_RegistParseFunc(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel, SC_CHAR *chName,
    SC_NPU_ParseFunc pParseFunc)
{
    SC_S32 s32Ret = SC_FAILURE;
    s32Ret = SC_MPI_SVP_NPU_RegisterParsefunc(&pstNpuModel->stModel, chName, pParseFunc);
    return s32Ret;
}

SC_S32 SAMPLE_COMM_SVP_NPU_RegisterCallbackFunc(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel, SC_CHAR *chName,
    SC_NPU_CallbackFunc pParseFunc)
{
    SC_S32 s32Ret = SC_FAILURE;
    s32Ret = SC_MPI_SVP_NPU_RegisterCallback(&pstNpuModel->stModel, chName, pParseFunc);
    return s32Ret;
}

SC_S32 SC_CNN_ParseCustomLrn(const void *pJsonNode, SC_U32 u32JsonLen, SC_VOID **pOpParams, SC_U32 *pParamLen)
{
    SAMPLE_SVP_CHECK_EXPR_RET(((!pJsonNode) || (u32JsonLen == 0)), SC_FAILURE, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,pJsonNode or u32JsonLen err!\n");

    if(pParamLen != NULL)
    {
        *pParamLen = sizeof(SAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S);
    }

    SC_U32 u32ParamLen = sizeof(SAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S);
    *pOpParams = (SAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S *)malloc(u32ParamLen);
    if(*pOpParams == NULL)
        return -1;
    memset(*pOpParams, 0, u32ParamLen);
    SAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S *pCustomLrnParams = (SAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S *) *pOpParams;

    cJSON *pOperatorParamNode = (cJSON *)pJsonNode;
    SC_CHAR *pJsonString = pOperatorParamNode->valuestring;
    cJSON *pJson = cJSON_Parse(pJsonString);

    SAMPLE_SVP_CHECK_EXPR_RET(!pJson, SC_FAILURE, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error,pJsonNode or u32JsonLen err!\n");

    JSON_PARSE_INT_VALUE(pCustomLrnParams, u32Size, pJson, "size");
    JSON_PARSE_DOUBLE_VALUE(pCustomLrnParams, dAlpha, pJson, "alpha");
    JSON_PARSE_DOUBLE_VALUE(pCustomLrnParams, dBeta, pJson, "beta");
    JSON_PARSE_DOUBLE_VALUE(pCustomLrnParams, dBias, pJson, "bias");
    strcpy(pCustomLrnParams->achNormRegion, "across_channels");

    cJSON_Delete(pJson);
    return 0;
}

void dcnn_hwc_to_chw(char *hwc, char *chw, NPU_TENSOR_S *pTensor)
{
    int c = 0, w = 0, h = 0, k = 0;
    unsigned int width = pTensor->u32Width;
    unsigned int height = pTensor->u32Height;
    unsigned int k_size_norm = pTensor->u32KSizeNorm;
    unsigned int k_norm_num = pTensor->u32KNormNum;
    unsigned int k_step = pTensor->u32KStep;
    unsigned int row_ddr_step = pTensor->u32RowStep;
    unsigned int precision = pTensor->u32Precision;
    //unsigned int pixel_byte = precision / 8;
    unsigned int k_size_last = pTensor->u32KSizeLast;
    unsigned int batch = pTensor->u32Num;
    //int channels_norm = k_size_norm * k_norm_num;
    //int channels_total = channels_norm + k_size_last;
    //unsigned int hwc_tensor_step = pTensor->u32TensorStep;
    //unsigned int chw_tensor_step = width * height * channels_total * pixel_byte;

    //the norm block

    if(precision == 16)
    {
        SC_S16 *dst = (SC_S16 *)chw;
        SC_S16 *src = (SC_S16 *)hwc;
        for (k = 0; k < k_norm_num; k++)
        {
            for (c = 0; c < k_size_norm; c++)
            {
                for(h = 0; h < height * batch; h++)
                {
                    src = (SC_S16 *)(hwc + k * k_step + h * row_ddr_step);
                    for(w = 0; w < width; w++)
                    {
                        *dst++ = *(src + w * k_size_norm + c);
                    }
                }
            }
        }

        //the last block
        for (c = 0; c < k_size_last; c++)
        {
            for (h = 0; h < height * batch; h++)
            {
                src = (SC_S16 *)(hwc + k_norm_num * k_step + h * row_ddr_step);
                for (w = 0; w < width; w++)
                {
                    *dst++ = *(src + w * k_size_last + c);
                }
            }
        }
    }
    else
    {
        SC_S8 *dst = (SC_S8 *)chw;
        SC_S8 *src = (SC_S8 *)hwc;
        for (k = 0; k < k_norm_num; k++)
        {
            for (c = 0; c < k_size_norm; c++)
            {
                for(h = 0; h < height * batch; h++)
                {
                    src = (SC_S8 *)(hwc + k * k_step + h * row_ddr_step);
                    for(w = 0; w < width; w++)
                    {
                        *dst++ = *(src + w * k_size_norm + c);
                    }
                }
            }
        }

        //the last block
        for (c = 0; c < k_size_last; c++)
        {
            for (h = 0; h < height * batch; h++)
            {
                src = (SC_S8 *)(hwc + k_norm_num * k_step + h * row_ddr_step);
                for (w = 0; w < width; w++)
                {
                    *dst++ = *(src + w * k_size_last + c);
                }
            }
        }
    }

}

void dcnn_chw_to_hwc(char *chw, char *hwc, NPU_TENSOR_S *pTensor)
{
    int c = 0, w = 0, h = 0, k = 0;
    unsigned int width = pTensor->u32Width;
    unsigned int height = pTensor->u32Height;
    //unsigned int output_off = pCBParams->astOutputTensor[0].u32Offset;
    unsigned int k_size_norm = pTensor->u32KSizeNorm;
    unsigned int k_step = pTensor->u32KStep;
    unsigned int row_ddr_step = pTensor->u32RowStep;
    unsigned int precision = pTensor->u32Precision;
    //unsigned int pixel_byte = precision / 8;
    unsigned int k_size_last = pTensor->u32KSizeLast;
    unsigned int k_norm_num = pTensor->u32KNormNum;
    unsigned int batch = pTensor->u32Num;
    //int channels_norm = k_size_norm * k_norm_num;
    //int channels_total = channels_norm + k_size_last;
    //unsigned int hwc_tensor_step = pTensor->u32TensorStep;
    //unsigned int chw_tensor_step = width * height * channels_total * pixel_byte;

    if(precision == 16)
    {
        SC_S16 *dst = (SC_S16 *)hwc;
        SC_S16 *src = (SC_S16 *)chw;
        for (k = 0; k < k_norm_num; k++)
        {
            for (c = 0; c < k_size_norm; c++)
            {
                for(h = 0; h < height * batch; h++)
                {
                    dst = (SC_S16 *)(hwc + k * k_step + h * row_ddr_step);
                    for(w = 0; w < width; w++)
                    {
                        *(dst + w * k_size_norm + c) = *src++;
                    }
                }
            }
        }

        //the last block
        for (c = 0; c < k_size_last; c++)
        {
            for (h = 0; h < height * batch; h++)
            {
                dst = (SC_S16 *)(hwc + k_norm_num * k_step + h * row_ddr_step);
                for (w = 0; w < width; w++)
                {
                    *(dst + w * k_size_last + c) = *src++;
                }
            }
        }
    }
    else
    {
        SC_S8 *dst = (SC_S8 *)hwc;
        SC_S8 *src = (SC_S8 *)chw;
        for (k = 0; k < k_norm_num; k++)
        {
            for (c = 0; c < k_size_norm; c++)
            {
                for(h = 0; h < height * batch; h++)
                {
                    dst = (SC_S8 *)(hwc + k * k_step + h * row_ddr_step);
                    for(w = 0; w < width; w++)
                    {
                        *(dst + w * k_size_norm + c) = *src++;
                    }
                }
            }
        }

        //the last block
        for (c = 0; c < k_size_last; c++)
        {
            for (h = 0; h < height * batch; h++)
            {
                dst = (SC_S8 *)(hwc + k_norm_num * k_step + h * row_ddr_step);
                for (w = 0; w < width; w++)
                {
                    *(dst + w * k_size_last + c) = *src++;
                }
            }
        }
    }

}

void dcnn_exec_scale_interal(char *input_data, float *output_data, int data_size, double scale_factor, int zero_point,
    int precision)
{
    for (int ind = 0; ind < data_size; ++ind)
    {
        if (precision == 16)
        {
            signed short *input = (signed short *)input_data;
            output_data[ind] = (input[ind] - zero_point) * scale_factor;
        }
        else
        {
            signed char *input = (signed char *)input_data;
            output_data[ind] = (input[ind] - zero_point) * scale_factor;
        }
    }
}
void dcnn_exec_quantization_interal(float *float_data, char *quant_data, int data_size, double scale_factor,
    int zero_point, int precision)
{
    int positive_limit = (int)pow((double)2, (double)(precision - 1)) - 1;
    int negative_limit = (int) - pow((double)2, (double)(precision - 1));
    int tmp = 0;

    if(precision == 16)
    {
        signed short *quant = (signed short *)quant_data;

        for (int i = 0; i < data_size; ++i)
        {
            tmp = round(float_data[i] / scale_factor) + zero_point;
            quant[i] = MIN(tmp, positive_limit);
            quant[i] = MAX(quant[i], negative_limit);
        }
    }
    else
    {
        signed char *quant = (signed char *)quant_data;

        for (int i = 0; i < data_size; ++i)
        {
            tmp = round(float_data[i] / scale_factor) + zero_point;
            quant[i] = MIN(tmp, positive_limit);
            quant[i] = MAX(quant[i], negative_limit);
        }
    }
}

SC_S32 SC_CNN_OperatorLrn(SC_UINTPTR_T pIn, SC_UINTPTR_T pOut, NPU_CB_PARAM_S *pCBParams, SC_VOID *pOpParams)
{
    SC_UINTPTR_T input_data = pIn + pCBParams->astInputTensor[0].u32Offset;
    SC_UINTPTR_T output_data = pOut + pCBParams->astOutputTensor[0].u32Offset;

    SAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S *pLrnParams = (SAMPLE_SVP_NPU_CUSTOM_LRN_PARAM_S *)(pOpParams);

    float lrn_bias = pLrnParams->dBias;
    float lrn_beta = pLrnParams->dBeta;
    SC_DOUBLE alpha_div_size = pLrnParams->dAlpha / pLrnParams->u32Size;
    int lrn_local_size = pLrnParams->u32Size;

    SC_S32 batch_size = pCBParams->astInputTensor[0].u32Num;
    SC_S32 operator_input_width = pCBParams->astInputTensor[0].u32Width;
    SC_S32 operator_input_height = pCBParams->astInputTensor[0].u32Height;
    SC_S32 operator_input_channel = pCBParams->astInputTensor[0].u32OriChannels;

    SC_S32 s32InZeroPoint = pCBParams->astInputTensor[0].s32ZeroPoint;
    SC_DOUBLE dInFactor = pCBParams->astInputTensor[0].dScaleFactor;
    SC_U32 u32InPrecision = pCBParams->astInputTensor[0].u32Precision;

    SC_S32 s32OutZeroPoint = pCBParams->astOutputTensor[0].s32ZeroPoint;
    SC_DOUBLE dOutFactor = pCBParams->astOutputTensor[0].dScaleFactor;
    SC_U32 u32OutPrecision = pCBParams->astOutputTensor[0].u32Precision;

    SC_S32 map_size = operator_input_height * operator_input_width;
    SC_U32 single_size = operator_input_channel * map_size;
    size_t data_size = single_size * batch_size;

    SC_U32 u32InputMemSize = pCBParams->astInputTensor[0].u32Size * pCBParams->astInputTensor[0].u32Precision / 8;
    SC_U32 u32OutMemSize = pCBParams->astOutputTensor[0].u32Size * pCBParams->astOutputTensor[0].u32Precision / 8;

    char *pLrnInput = NULL;
    char *pLrnOutput = NULL;
    pLrnInput = (char *)malloc(u32InputMemSize);
    if (!pLrnInput)
    {
        printf("Malloc lrn input error.\n");
        return -1;
    }

    pLrnOutput = (char *)malloc(u32OutMemSize);
    if (!pLrnOutput)
    {
        free(pLrnInput);
        printf("Malloc lrn input error.\n");
        return -1;
    }

    memset(pLrnInput, 0, u32InputMemSize);
    memset(pLrnOutput, 0, u32OutMemSize);

    dcnn_hwc_to_chw((char *)input_data, pLrnInput, &pCBParams->astInputTensor[0]);

    float *input_data_temp = (float *)malloc(data_size * sizeof(double));
    if (!input_data_temp)
    {
        printf("Malloc input buffer failed.\n");
        return -1;
    }
    float *output_data_temp = (float *)malloc(data_size * sizeof(double));
    if (!output_data_temp)
    {
        printf("Malloc output buffer failed.\n");
        free(input_data_temp);
        return -1;
    }

    memset(input_data_temp, 0, data_size * sizeof(double));
    memset(output_data_temp, 0, data_size * sizeof(double));

    dcnn_exec_scale_interal(pLrnInput, input_data_temp, data_size, dInFactor, s32InZeroPoint, u32InPrecision);

    SC_DOUBLE *square_data = (SC_DOUBLE *) malloc(sizeof(SC_DOUBLE) * batch_size * operator_input_channel
            * operator_input_height * operator_input_width);
    SC_DOUBLE *square_data_sum = (SC_DOUBLE *) malloc(sizeof(SC_DOUBLE) * batch_size * operator_input_channel
            * operator_input_height * operator_input_width);
    memset(square_data_sum, 0, sizeof(SC_DOUBLE) * batch_size * operator_input_channel
        * operator_input_height * operator_input_width);
    memset(square_data, 0, sizeof(SC_DOUBLE) * batch_size * operator_input_channel
        * operator_input_height * operator_input_width);

    for (SC_U32 i = 0; i < data_size; ++i)
    {
        square_data[i] = input_data_temp[i];
        square_data[i] = square_data[i] * square_data[i];
    }

    if (strcmp(pLrnParams->achNormRegion, "across_channels") == 0)
    {
        for (int m = 0; m < batch_size; m++)
        {
            for (int q = 0; q < operator_input_channel; q++)
            {
                // square sum
                double *ssptr = square_data_sum + m * single_size + q * map_size;
                for (int p = q - lrn_local_size / 2; p <= q + lrn_local_size / 2; p++)
                {
                    if (p < 0 || p >= operator_input_channel)
                        continue;
                    const double *sptr = square_data + m * single_size + p * map_size;
                    for (int i = 0; i < map_size; i++)
                        ssptr[i] += sptr[i];
                }
            }
        }
    }

    for (SC_U32 i = 0; i < data_size; i++)
    {
        if (square_data_sum[i] == 0 && lrn_bias == 0)
            output_data_temp[i] = 0;
        else
            output_data_temp[i] = (double)(input_data_temp[i] * pow(lrn_bias + alpha_div_size * square_data_sum[i], -lrn_beta));
    }

    free(square_data);
    free(square_data_sum);

    dcnn_exec_quantization_interal(output_data_temp, pLrnOutput, data_size, dOutFactor, s32OutZeroPoint, u32OutPrecision);

    free(input_data_temp);
    free(output_data_temp);

    dcnn_chw_to_hwc(pLrnOutput, (char *)output_data, &pCBParams->astOutputTensor[0]);

    free(pLrnInput);
    free(pLrnOutput);

    return 0;
}
