#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <math.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include <assert.h>
#include <signal.h>
#include <semaphore.h>

#include <sys/prctl.h>

#include "cJSON.h"

#include "sample_comm_svp.h"
#include "sample_comm_npu.h"
#include "sample_npu_main.h"
#include "sample_svp_npu_software.h"

static SAMPLE_SVP_NPU_MODEL_S                   s_stDcnnModel = {0};
static SAMPLE_SVP_NPU_PARAM_S                   s_stDcnnNpuParam = {0};

static SAMPLE_SVP_NPU_MODEL_S                   s_stSsdModel = {0};
static SAMPLE_SVP_NPU_PARAM_S                   s_stSsdNpuParam = {0};

static SAMPLE_SVP_NPU_MODEL_S                   s_stYolov3Model = {0};
static SAMPLE_SVP_NPU_PARAM_S                   s_stYolov3NpuParam = {0};

static SAMPLE_SVP_NPU_MODEL_S                   s_stCnnCbModel = {0};
static SAMPLE_SVP_NPU_PARAM_S                   s_stCnnCbNpuParam = {0};

/******************************************************************************
* function : NPU Forward
******************************************************************************/
SC_S32 SAMPLE_COMM_SVP_NPU_Forward(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel,
    SAMPLE_SVP_NPU_PARAM_S *pstNpuParam, NPU_IMAGE_S *pstImg,
    SC_BOOL bInstant)
{
    SC_S32 s32Ret = SC_FAILURE;
    SVP_NPU_HANDLE hSvpNpuHandle = 0;

    s32Ret =  SC_MPI_SVP_NPU_Forward(&hSvpNpuHandle, pstImg, &pstNpuParam->stIntMem, &pstNpuModel->stModel,
            &pstNpuParam->stOutMem, bInstant);
    SAMPLE_SVP_CHECK_EXPR_RET(SC_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SC_MPI_SVP_NPU_Forward failed!\n");

    return s32Ret;
}

SC_S32 SAMPLE_SVP_NPU_Deinit(SAMPLE_SVP_NPU_MODEL_S *pstNpuModel, SAMPLE_SVP_NPU_PARAM_S *pstNpuParam)
{
    SC_S32 s32Ret = SC_SUCCESS;
    /*hardware para deinit*/
    if(pstNpuParam != NULL)
    {
        s32Ret = SAMPLE_COMM_SVP_NPU_ParamDeinit(pstNpuParam);
        SAMPLE_SVP_CHECK_EXPR_TRACE(SC_SUCCESS != s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,SAMPLE_COMM_SVP_NPU_ParamDeinit failed!\n");
    }

    /*model deinit*/
    if(pstNpuModel != NULL)
    {
        s32Ret = SAMPLE_COMM_SVP_NPU_UnloadModel(pstNpuModel);
        SAMPLE_SVP_CHECK_EXPR_TRACE(SC_SUCCESS != s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,SAMPLE_COMM_SVP_NPU_UnloadModel failed!\n");
    }
    return s32Ret;
}

/******************************************************************************
* function : Dcnn print
******************************************************************************/
SC_S32 SAMPLE_SVP_NPU_Classification_PrintResult(SAMPLE_SVP_NPU_DCNN_OUTPUT *pResult, SC_S32 s32ResultNum)
{
    if(0 == s32ResultNum)
    {
        SAMPLE_SVP_TRACE_INFO("SAMPLE_SVP_NPU_Dcnn_PrintResult is NULL\n");
        return 0;
    }
    else
    {
        if(SC_NULL == pResult)
        {
            SAMPLE_SVP_TRACE_ERR("pResult is NULL\n");
            return SC_FAILURE;
        }

        for(int i = 0; i < s32ResultNum; ++ i)
        {
            SAMPLE_SVP_TRACE_INFO("==== The class ID  %d ====\n", pResult[i].u32ClassId);
            SAMPLE_SVP_TRACE_INFO("%f \n", pResult[i].u32Confidence);
        }
    }

    return SC_SUCCESS;
}

/******************************************************************************
* function : show Cnn sample(image 244x244 U8_C3)
******************************************************************************/
void SAMPLE_SVP_NPU_Dcnn()
{
    SC_CHAR *pcSrcFile = "./data/npu_image/rgb/snake.rgb";
    SC_CHAR *pcModelName = "./data/npu_model/classification/mobilenetv1_caffe.npubin";

    SC_S32 s32Ret = SC_FAILURE;

    if(access(pcSrcFile, F_OK) != 0)
    {

        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, DCNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,pcSrcFile not exist!\n");
    }

    if(access(pcModelName, F_OK) != 0)
    {

        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, DCNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,pcModelName not exist !\n");
    }

    s_stDcnnModel.stModelConf.enCbMode = SVP_NPU_NO_CB;
    s_stDcnnModel.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;

    /*DCNN Load model*/
    SAMPLE_SVP_TRACE_INFO("Dcnn Load model!\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(pcModelName, &s_stDcnnModel);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, DCNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_COMM_SVP_NPU_LoadModel failed!\n");

    /*DCNN parameter initialization get form model*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stDcnnModel, &s_stDcnnNpuParam);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, DCNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_Cnn_ParamInit failed!\n");
    /*Fill src data*/
    SAMPLE_SVP_TRACE_INFO("Dcnn start!\n");

    NPU_IMAGE_S dcnn_image;
    dcnn_image.u32InputNum = 1;
    dcnn_image.astInputImg[0].u32BatchNum = 1;
    dcnn_image.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
    dcnn_image.astInputImg[0].stImages[0].u32Height = 224;
    dcnn_image.astInputImg[0].stImages[0].u32Width = 224;

    s32Ret = SAMPLE_COMM_SVP_NPU_FillSrcData(pcSrcFile, &s_stDcnnNpuParam, &dcnn_image, SVP_IMG_RGB);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, DCNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_FillSrcData failed!\n");

    /*NPU process*/
    SAMPLE_SVP_TRACE_INFO("Cnn forward:\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_Forward(&s_stDcnnModel, &s_stDcnnNpuParam, &dcnn_image, SC_TRUE);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, DCNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_COMM_SVP_NPU_Forward failed!\n");

    /*Software process and get result*/
    SC_VOID *pResult = SC_NULL;
    SC_S32   s32ResultNum;
    SAMPLE_SVP_TRACE_INFO("Get Result:\n");

    s32Ret = SAMPLE_SVP_NPU_DCNN_GetResult(s_stDcnnModel.stModel.astDstTensor, s_stDcnnModel.stModel.u16DstNum,
            (uintptr_t)s_stDcnnNpuParam.stOutMem.u64VirAddr, &pResult, &s32ResultNum);

    /*Print result*/
    SAMPLE_SVP_TRACE_INFO("Print Result:\n");
    s32Ret = SAMPLE_SVP_NPU_Classification_PrintResult((SAMPLE_SVP_NPU_DCNN_OUTPUT *)pResult, s32ResultNum);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, DCNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_Dcnn_PrintResult failed!\n");

DCNN_FAIL_0:
    SAMPLE_SVP_NPU_Deinit(&s_stDcnnModel, &s_stDcnnNpuParam);

    if(pResult)
    {
        free(pResult);
    }

    return;
}

/******************************************************************************
* function : Dcnn sample signal handle
******************************************************************************/
void SAMPLE_SVP_NPU_Dcnn_HandleSig(void)
{
    SAMPLE_SVP_NPU_Deinit(&s_stDcnnModel, &s_stDcnnNpuParam);
    memset(&s_stDcnnModel, 0, sizeof(s_stDcnnModel));
    memset(&s_stDcnnNpuParam, 0, sizeof(s_stDcnnNpuParam));
}

/******************************************************************************
* function : print detection result
******************************************************************************/
SC_S32 SAMPLE_SVP_NPU_Detection_PrintResult(SAMPLE_SVP_NPU_DETECTION_OUTPUT *pResult, SC_S32 s32ResultNum)
{
    if(0 == s32ResultNum)
    {
        SAMPLE_SVP_TRACE_INFO("SAMPLE_SVP_NPU_Detection_PrintResult is NULL\n");
        return 0;
    }
    else
    {
        if(SC_NULL == pResult)
        {
            SAMPLE_SVP_TRACE_ERR("pResult is NULL\n");
            return SC_FAILURE;
        }

        for(int i = 0; i < s32ResultNum; ++ i)
        {
            SAMPLE_SVP_TRACE_INFO("==== The class ID  %d ====\n", pResult[i].classId);
            SAMPLE_SVP_TRACE_INFO("x:%f y:%f w:%f h:%f conf:%f\n", pResult[i].x, pResult[i].y, pResult[i].w, pResult[i].h,
                pResult[i].confidence);
        }
    }

    return SC_SUCCESS;
}

/******************************************************************************
* function : show SSD sample(image 300x300 U8_C3)
******************************************************************************/
void SAMPLE_SVP_NPU_Ssd()
{
    SC_CHAR *pcSrcFile = "./data/npu_image/rgb/cat.rgb";
    SC_CHAR *pcModelName = "./data/npu_model/detection/ssd-mobilenet_caffe.npubin";

    SC_S32 s32Ret = SC_FAILURE;

    if(access(pcSrcFile, F_OK) != 0)
    {

        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, SSD_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,pcSrcFile not exist!\n");
    }

    if(access(pcModelName, F_OK) != 0)
    {

        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, SSD_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,pcModelName not exist !\n");
    }

    s_stSsdModel.stModelConf.enCbMode = SVP_NPU_NO_CB;
    s_stSsdModel.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;

    /*SSD Load model*/
    SAMPLE_SVP_TRACE_INFO(" Load model!\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(pcModelName, &s_stSsdModel);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, SSD_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_COMM_SVP_NPU_LoadModel failed!\n");

    /*SSD parameter initialization*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stSsdModel, &s_stSsdNpuParam);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, SSD_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_Cnn_ParamInit failed!\n");

    SC_VOID *pResult = SC_NULL;
    SC_S32   s32ResultNum;

    NPU_IMAGE_S ssd_image;
    ssd_image.u32InputNum = 1;
    ssd_image.astInputImg[0].u32BatchNum = 1;
    ssd_image.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
    ssd_image.astInputImg[0].stImages[0].u32Height = 300;
    ssd_image.astInputImg[0].stImages[0].u32Width = 300;

    /*SSD src data*/
    SAMPLE_SVP_TRACE_INFO("Start!\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_FillSrcData(pcSrcFile, &s_stSsdNpuParam, &ssd_image, SVP_IMG_RGB);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, SSD_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_FillSrcData failed!\n");

    /*NPU process*/
    SAMPLE_SVP_TRACE_INFO("forward:\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_Forward(&s_stSsdModel, &s_stSsdNpuParam, &ssd_image, SC_TRUE);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, SSD_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_COMM_SVP_NPU_Forward failed!\n");

    /*Software process and Get result*/
    SAMPLE_SVP_NPU_SSD_GetResult(s_stSsdModel.stModel.astDstTensor, s_stSsdModel.stModel.u16DstNum,
        (uintptr_t)s_stSsdNpuParam.stOutMem.u64VirAddr, &pResult, &s32ResultNum);

    SAMPLE_SVP_TRACE_INFO("The %s Result:\n", pcSrcFile);
    s32Ret = SAMPLE_SVP_NPU_Detection_PrintResult((SAMPLE_SVP_NPU_DETECTION_OUTPUT *)pResult, s32ResultNum);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, SSD_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_Dcnn_PrintResult failed!\n");

SSD_FAIL_0:
    SAMPLE_SVP_NPU_Deinit(&s_stSsdModel, &s_stSsdNpuParam);
    if(pResult)
    {
        free(pResult);
    }

    return ;
}

/******************************************************************************
* function : SSD sample signal handle
******************************************************************************/
void SAMPLE_SVP_NPU_Ssd_HandleSig(void)
{
    SAMPLE_SVP_NPU_Deinit(&s_stSsdModel, &s_stSsdNpuParam);
    memset(&s_stSsdModel, 0, sizeof(s_stSsdModel));
    memset(&s_stSsdNpuParam, 0, sizeof(s_stSsdNpuParam));
}

/******************************************************************************
* function : show YOLOV3 sample(image 416x416 YUV420P)
******************************************************************************/
void SAMPLE_SVP_NPU_Yolov3(void)
{
    SC_CHAR *pcSrcFile = "./data/npu_image/yuv420p/dog_bike_car.yuv";
    SC_CHAR *pcModelName = "./data/npu_model/detection/yolov3_caffe_yuv420p.npubin";

    SC_S32 s32Ret = SC_FAILURE;

    if(access(pcSrcFile, F_OK) != 0)
    {

        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, YOLOV3_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,pcSrcFile not exist!\n");
    }

    if(access(pcModelName, F_OK) != 0)
    {

        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, YOLOV3_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,pcModelName not exist !\n");
    }

    s_stYolov3Model.stModelConf.enCbMode = SVP_NPU_NO_CB;
    s_stYolov3Model.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;

    /*Yolov3 Load model*/
    SAMPLE_SVP_TRACE_INFO("Yolov3 Load model!\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(pcModelName, &s_stYolov3Model);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, YOLOV3_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_COMM_SVP_NPU_LoadModel failed!\n");

    /*Yolov3 parameter initialization*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stYolov3Model, &s_stYolov3NpuParam);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, YOLOV3_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_Cnn_ParamInit failed!\n");

    /*Yolov3 src data*/
    SAMPLE_SVP_TRACE_INFO("YOLOV3 start!\n");
    NPU_IMAGE_S yolov3_image;
    yolov3_image.u32InputNum = 1;
    yolov3_image.astInputImg[0].u32BatchNum = 1;
    yolov3_image.astInputImg[0].stImages[0].enType = SVP_IMG_YUV420P;
    yolov3_image.astInputImg[0].stImages[0].u32Height = 416;
    yolov3_image.astInputImg[0].stImages[0].u32Width = 416;
    s32Ret = SAMPLE_COMM_SVP_NPU_FillSrcData(pcSrcFile, &s_stYolov3NpuParam, &yolov3_image, SVP_IMG_YUV420P);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, YOLOV3_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_FillSrcData failed!\n");
    /*NPU process*/
    SAMPLE_SVP_TRACE_INFO("YOLOV3 forward:\n");
    for(int j = 0; j < 1; j++)
    {
        s32Ret = SAMPLE_COMM_SVP_NPU_Forward(&s_stYolov3Model, &s_stYolov3NpuParam, &yolov3_image, SC_TRUE);
        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, YOLOV3_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,SAMPLE_COMM_SVP_NPU_Forward failed!\n");
    }

    SAMPLE_SVP_TRACE_INFO("get_yolov3_result:\n");
    /*Software process and Get result*/
    SC_VOID *pResult = SC_NULL;
    SC_S32   s32ResultNum;
    SAMPLE_SVP_NPU_YOLOVX_GetResult(s_stYolov3Model.stModel.astDstTensor, s_stYolov3Model.stModel.u16DstNum,
        (uintptr_t)s_stYolov3NpuParam.stOutMem.u64VirAddr, &pResult, &s32ResultNum, 3,
        yolov3_image.astInputImg[0].stImages[0].u32Width, yolov3_image.astInputImg[0].stImages[0].u32Height);

    SAMPLE_SVP_TRACE_INFO("Yolov3 result:\n");
    s32Ret = SAMPLE_SVP_NPU_Detection_PrintResult(pResult, s32ResultNum);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, YOLOV3_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_YOLOV3_PrintResult failed!\n");

YOLOV3_FAIL_0:
    SAMPLE_SVP_NPU_Deinit(&s_stYolov3Model, &s_stYolov3NpuParam);
    return ;
}

/******************************************************************************
* function : Yolov3 sample signal handle
******************************************************************************/
void SAMPLE_SVP_NPU_Yolov3_HandleSig(void)
{
    SAMPLE_SVP_NPU_Deinit(&s_stYolov3Model, &s_stYolov3NpuParam);
    memset(&s_stYolov3Model, 0, sizeof(s_stYolov3Model));
    memset(&s_stYolov3NpuParam, 0, sizeof(s_stYolov3NpuParam));
}

/******************************************************************************
* function : show CNN CallBack to Arm sample(image 227x227 U8_C3)
******************************************************************************/
void SAMPLE_SVP_NPU_CB()
{
    SC_CHAR *pcSrcFile = "./data/npu_image/rgb/dog.rgb";
    SC_CHAR *pcModelName = "./data/npu_model/classification/caffe_callback.npubin";

    SC_S32 s32Ret = SC_FAILURE;

    if(access(pcSrcFile, F_OK) != 0)
    {

        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,pcSrcFile not exist!\n");
    }

    if(access(pcModelName, F_OK) != 0)
    {

        SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,pcModelName not exist !\n");
    }

    s_stCnnCbModel.stModelConf.enCbMode = SVP_NPU_CB_TO_ARM;
    s_stCnnCbModel.stModelConf.enPriority = SVP_NPU_PRIORITY_NORMAL;

    /*SSD Load model*/
    SAMPLE_SVP_TRACE_INFO(" Load model!\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_LoadModel(pcModelName, &s_stCnnCbModel);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_COMM_SVP_NPU_LoadModel failed!\n");

    /*SSD parameter initialization*/
    s32Ret = SAMPLE_COMM_SVP_NPU_ParamInit(&s_stCnnCbModel, &s_stCnnCbNpuParam);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_Cnn_ParamInit failed!\n");

    s32Ret = SAMPLE_COMM_SVP_NPU_RegistParseFunc(&s_stCnnCbModel, "customLRN", SC_CNN_ParseCustomLrn);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SC_MPI_SVP_NPU_RegisterCallback failed!\n");

    s32Ret = SAMPLE_COMM_SVP_NPU_RegisterCallbackFunc(&s_stCnnCbModel, "customLRN", SC_CNN_OperatorLrn);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SC_MPI_SVP_NPU_RegisterParsefunc failed!\n");

    NPU_IMAGE_S cnn_image;
    memset(&cnn_image, 0x00, sizeof(cnn_image));
    cnn_image.u32InputNum = 1;
    cnn_image.astInputImg[0].u32BatchNum = 1;
    cnn_image.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;
    cnn_image.astInputImg[0].stImages[0].u32Height = 227;
    cnn_image.astInputImg[0].stImages[0].u32Width = 227;

    /*SSD src data*/
    SAMPLE_SVP_TRACE_INFO("Start!\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_FillSrcData(pcSrcFile, &s_stCnnCbNpuParam, &cnn_image, SVP_IMG_RGB);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_FillSrcData failed!\n");

    /*NPU process*/
    SAMPLE_SVP_TRACE_INFO("forward:\n");
    s32Ret = SAMPLE_COMM_SVP_NPU_Forward(&s_stCnnCbModel, &s_stCnnCbNpuParam, &cnn_image, SC_TRUE);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_COMM_SVP_NPU_Forward failed!\n");

    /*Software process and get result*/
    SC_VOID *pResult = SC_NULL;
    SC_S32   s32ResultNum;
    SAMPLE_SVP_TRACE_INFO("Get Result:\n");
    s32Ret = SAMPLE_SVP_NPU_DCNN_GetResult(s_stCnnCbModel.stModel.astDstTensor, s_stCnnCbModel.stModel.u16DstNum,
            (uintptr_t)s_stCnnCbNpuParam.stOutMem.u64VirAddr, &pResult, &s32ResultNum);

    /*Print result*/
    SAMPLE_SVP_TRACE_INFO("Print Result:\n");
    s32Ret = SAMPLE_SVP_NPU_Classification_PrintResult((SAMPLE_SVP_NPU_DCNN_OUTPUT *)pResult, s32ResultNum);
    SAMPLE_SVP_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, CNN_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
        "Error,SAMPLE_SVP_NPU_Dcnn_PrintResult failed!\n");

CNN_FAIL_0:
    SAMPLE_SVP_NPU_Deinit(&s_stCnnCbModel, &s_stCnnCbNpuParam);
    if(pResult)
    {
        free(pResult);
    }

    return ;
}

/******************************************************************************
* function : Yolov3 sample signal handle
******************************************************************************/
void SAMPLE_SVP_NPU_CB_HandleSig(void)
{
    SAMPLE_SVP_NPU_Deinit(&s_stCnnCbModel, &s_stCnnCbNpuParam);
    memset(&s_stCnnCbModel, 0, sizeof(s_stCnnCbModel));
    memset(&s_stCnnCbNpuParam, 0, sizeof(s_stCnnCbNpuParam));
}
