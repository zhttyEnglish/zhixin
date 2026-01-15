#ifndef _SC_NPU_H_
#define _SC_NPU_H_

#include "sc_comm_svp.h"
#include "sc_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**************************************SVP_NPU Error Code****************************************/
typedef enum scEN_SVP_NPU_ERR_CODE_E
{
    ERR_SVP_NPU_SYS_TIMEOUT = 0x40, /* SVP_NPU process timeout */
    ERR_SVP_NPU_QUERY_TIMEOUT = 0x41, /* SVP_NPU query timeout */
    ERR_SVP_NPU_CFG_ERR = 0x42, /* SVP_NPU Configure error */
    ERR_SVP_NPU_OPEN_FILE = 0x43, /* SVP NPU open file error */
    ERR_SVP_NPU_READ_FILE = 0x44, /* SVP NPU read file error */
    ERR_SVP_NPU_WRITE_FILE = 0x45, /* SVP NPU write file error */
    ERR_SVP_NPU_INNER,
    ERR_SVP_NPU_BUTT
} EN_SVP_NPU_ERR_CODE_E;

/* Invalid device ID */
#define SC_ERR_SVP_NPU_INVALID_DEVID     SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* Invalid channel ID */
#define SC_ERR_SVP_NPU_INVALID_CHNID     SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* At least one parameter is illegal. For example, an illegal enumeration value exists. */
#define SC_ERR_SVP_NPU_ILLEGAL_PARAM     SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* The channel exists. */
#define SC_ERR_SVP_NPU_EXIST             SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* The UN exists. */
#define SC_ERR_SVP_NPU_UNEXIST           SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* A null point is used. */
#define SC_ERR_SVP_NPU_NULL_PTR          SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* Try to enable or initialize the system, device, or channel before configuring attributes. */
#define SC_ERR_SVP_NPU_NOT_CONFIG        SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* The operation is not supported currently. */
#define SC_ERR_SVP_NPU_NOT_SURPPORT      SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* The operation, changing static attributes for example, is not permitted. */
#define SC_ERR_SVP_NPU_NOT_PERM          SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* A failure caused by the malloc memory occurs. */
#define SC_ERR_SVP_NPU_NOMEM             SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* A failure caused by the malloc buffer occurs. */
#define SC_ERR_SVP_NPU_NOBUF             SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* The buffer is empty. */
#define SC_ERR_SVP_NPU_BUF_EMPTY         SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* No buffer is provided for storing new data. */
#define SC_ERR_SVP_NPU_BUF_FULL          SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* The system is not ready because it may be not initialized or loaded.
 * The error code is returned when a device file fails to be opened. */
#define SC_ERR_SVP_NPU_NOTREADY          SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/* The source address or target address is incorrect during the operations
such as calling copy_from_user or copy_to_user. */
#define SC_ERR_SVP_NPU_BADADDR           SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
/* The resource is busy during the operations */
#define SC_ERR_SVP_NPU_BUSY              SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/* SVP_NPU process timeout */
#define SC_ERR_SVP_NPU_SYS_TIMEOUT       SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, ERR_SVP_NPU_SYS_TIMEOUT)
/* SVP_NPU query timeout */
#define SC_ERR_SVP_NPU_QUERY_TIMEOUT     SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, ERR_SVP_NPU_QUERY_TIMEOUT)
/* SVP_NPU configure error */
#define SC_ERR_SVP_NPU_CFG_ERR           SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, ERR_SVP_NPU_CFG_ERR)
#define SC_ERR_SVP_NPU_OPEN_FILE         SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, ERR_SVP_NPU_OPEN_FILE)
#define SC_ERR_SVP_NPU_READ_FILE         SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, ERR_SVP_NPU_READ_FILE)
#define SC_ERR_SVP_NPU_WRITE_FILE        SC_DEF_ERR(SC_ID_SVP_NPU, EN_ERR_LEVEL_ERROR, ERR_SVP_NPU_WRITE_FILE)

/* macro */
#define SVP_NPU_MAX_INPUT_NUM               64
#define SVP_NPU_MAX_OUTPUT_NUM              64
#define SVP_NPU_MAX_INPUT_IMG_NUM           64
#define SVP_NPU_MAX_BATCH_IMG_NUM           64

#define SVP_NPU_NAME_LEN    256

typedef enum scSVP_NPU_PRIORITY_MODE_E
{
    SVP_NPU_PRIORITY_NORMAL = 0X00,             /*normal prioroty task*/
    SVP_NPU_PRIORITY_HIGH = 0X01,               /*high prioroty task*/
    SVP_NPU_PRIORITY_BUTT
} SVP_NPU_PRIORITY_MODE_E;

typedef enum scSVP_NPU_CB_MODE_E
{
    SVP_NPU_NO_CB = 0X01,                   /*no callbacks task on arm*/
    SVP_NPU_CB_TO_ARM = 0X02,               /*have callbacks task on arm  the max callback num 128*/
    SVP_NPU_CB_TO_RCU = 0X03,               /*have callbacks task on rcu  the max callback num 128*/
    SVP_NPU_CB_BUTT
} SVP_NPU_CB_MODE_E;

typedef struct scNPU_TENSOR_S
{
    SC_U32 u32ID;
    SC_U32 u32Bank;
    SC_U32 u32Offset;
    SC_U32 u32Height;
    SC_U32 u32KStep;
    SC_U32 u32KNormNum;
    SC_U32 u32KSizeLast;
    SC_U32 u32KSizeNorm;
    SC_CHAR achName[SVP_NPU_NAME_LEN];
    SC_CHAR achType[SVP_NPU_NAME_LEN];
    SC_U32 u32Num;
    SC_U32 u32OriChannels;
    SC_U32 u32OriFrameSize;
    SC_U32 u32Precision;
    SC_U32 u32RowStep;
    SC_U32 u32TensorStep; //used for batch mode
    SC_DOUBLE dScaleFactor;
    SC_U32 u32Size;
    SC_U32 u32Width;
    SC_S32 s32ZeroPoint;
    SC_CHAR achLayoutType[SVP_NPU_NAME_LEN];
} NPU_TENSOR_S;

/* NPU model */
typedef struct scSVP_NPU_MODEL_S
{
    SC_U16          u16SrcNum;           /* input tensor num, [1, 64] */
    SC_U16          u16DstNum;           /* output tensor num, [1, 64] */

    NPU_TENSOR_S    astSrcTensor[SVP_NPU_MAX_INPUT_NUM]; /* input tensor info */
    NPU_TENSOR_S    astDstTensor[SVP_NPU_MAX_OUTPUT_NUM]; /* output tensor info */

    SC_U32          u32InMemSize;        /*forward npu need in mem size*/
    SC_U32          u32OutMemSize;       /*forward npu need out mem size*/
} SVP_NPU_MODEL_S;

typedef struct scSVP_NPU_MODEL_CONF_S
{
    SVP_NPU_PRIORITY_MODE_E enPriority;   /*netmode priority*/
    SVP_NPU_CB_MODE_E       enCbMode;     /*callback mode*/
    SVP_MEM_INFO_S          stModelBuf;   /*mode info*/
} SVP_NPU_MODEL_CONF_S;

typedef struct scNPU_INPUT_IMG_S
{
    SC_U32         u32BatchNum;
    SVP_IMAGE_S    stImages[SVP_NPU_MAX_BATCH_IMG_NUM]; //npu need stride 256 align
} NPU_INPUT_IMG_S;

typedef struct scNPU_IMAGE_S
{
    SC_U32          u32InputNum;
    NPU_INPUT_IMG_S astInputImg[SVP_NPU_MAX_INPUT_IMG_NUM];
} NPU_IMAGE_S;

typedef struct scNPU_CB_PARAM_S
{
    SC_U32          u32Id;
    SC_U32          u32InputTensorNum;
    NPU_TENSOR_S    astInputTensor[8];
    SC_U32          u32OutputTensorNum;
    NPU_TENSOR_S    astOutputTensor[8];
    SC_CHAR         achOperatorName[SVP_NPU_NAME_LEN];
    SC_CHAR         achOperatorType[SVP_NPU_NAME_LEN];
    void           *pOpParams;
    SC_U32          u32OpParamsLen;
} NPU_CB_PARAM_S;

typedef SC_S32 (*SC_NPU_CallbackFunc)(sc_uintptr_t, sc_uintptr_t, NPU_CB_PARAM_S *, void *);
typedef SC_S32 (*SC_NPU_ParseFunc)(const void *, SC_U32, void **, SC_U32 *);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* _SC_NPU_H_ */
