#ifndef __SC_COMM_IVE_H__
#define __SC_COMM_IVE_H__

#include "sc_errno.h"
#include "sc_comm_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

typedef SC_S32 IVE_HANDLE;

typedef enum scIVE_IMAGE_TYPE_E
{
    IVE_IMAGE_TYPE_U8C1 = 0x0,
    IVE_IMAGE_TYPE_S8C1 = 0x1,
    IVE_IMAGE_TYPE_YUV420SP = 0x2, /*YUV420 SemiPlanar*/
    IVE_IMAGE_TYPE_YUV422SP = 0x3, /*YUV422 SemiPlanar*/
    IVE_IMAGE_TYPE_YUV420P = 0x4, /*YUV420 Planar */
    IVE_IMAGE_TYPE_YUV422P = 0x5, /*YUV422 planar */
    IVE_IMAGE_TYPE_S8C2_PACKAGE = 0x6,
    IVE_IMAGE_TYPE_S8C2_PLANAR = 0x7,
    IVE_IMAGE_TYPE_S16C1 = 0x8,
    IVE_IMAGE_TYPE_U16C1 = 0x9,
    IVE_IMAGE_TYPE_U8C3_PACKAGE = 0xa,
    IVE_IMAGE_TYPE_U8C3_PLANAR = 0xb,
    IVE_IMAGE_TYPE_S32C1 = 0xc,
    IVE_IMAGE_TYPE_U32C1 = 0xd,
    IVE_IMAGE_TYPE_S64C1 = 0xe,
    IVE_IMAGE_TYPE_U64C1 = 0xf,
    IVE_IMAGE_TYPE_BGR888_PACKAGE = 0x10,
    IVE_IMAGE_TYPE_BGR888_PLANAR = 0x11,
    IVE_IMAGE_TYPE_BUTT
} IVE_IMAGE_TYPE_E;

typedef struct scIVE_IMAGE_S
{
    SC_U64 au64PhyAddr[3];   /* RW;The physical address of the image */
    SC_U64 au64VirAddr[3];   /* RW;The virtual address of the image */
    SC_U32 au32Stride[3];    /* RW;The stride of the image */
    SC_U32 u32Width;         /* RW;The width of the image */
    SC_U32 u32Height;        /* RW;The height of the image */
    IVE_IMAGE_TYPE_E enType; /* RW;The type of the image */
} IVE_IMAGE_S;

typedef IVE_IMAGE_S IVE_SRC_IMAGE_S;
typedef IVE_IMAGE_S IVE_DST_IMAGE_S;

typedef enum scIVE_CSC_MODE_E
{
    /*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
    IVE_CSC_MODE_VIDEO_BT601_YUV2RGB = 0x0,
    /*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
    IVE_CSC_MODE_VIDEO_BT709_YUV2RGB = 0x1,
    /*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
    IVE_CSC_MODE_PIC_BT601_YUV2RGB = 0x2,
    /*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
    IVE_CSC_MODE_PIC_BT709_YUV2RGB = 0x3,
    /*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/
    IVE_CSC_MODE_PIC_BT601_YUV2HSV = 0x4,
    /*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/
    IVE_CSC_MODE_PIC_BT709_YUV2HSV = 0x5,
    /*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/
    IVE_CSC_MODE_PIC_BT601_YUV2LAB = 0x6,
    /*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/
    IVE_CSC_MODE_PIC_BT709_YUV2LAB = 0x7,
    /*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
    IVE_CSC_MODE_VIDEO_BT601_RGB2YUV = 0x8,
    /*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
    IVE_CSC_MODE_VIDEO_BT709_RGB2YUV = 0x9,
    /*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/
    IVE_CSC_MODE_PIC_BT601_RGB2YUV = 0xa,
    /*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/
    IVE_CSC_MODE_PIC_BT709_RGB2YUV = 0xb,
    IVE_CSC_MODE_BUTT
} IVE_CSC_MODE_E;

typedef struct scIVE_CSC_CTRL_S
{
    IVE_CSC_MODE_E  enMode;
} IVE_CSC_CTRL_S;

#pragma pack(push)
#pragma pack(8)
typedef struct scIVE_MEM_INFO_S
{
    SC_U64 u64PhyAddr; /* RW;The physical address of the memory */
    SC_U64 u64VirAddr; /* RW;The virtual address of the memory */
    SC_U32 u32Size;    /* RW;The size of memory */
} IVE_MEM_INFO_S;
#pragma pack(pop)
typedef IVE_MEM_INFO_S IVE_SRC_MEM_INFO_S;
typedef IVE_MEM_INFO_S IVE_DST_MEM_INFO_S;

typedef enum scEN_IVE_ERR_CODE_E
{
    ERR_IVE_SYS_TIMEOUT = 0x40,   /* IVE process timeout */
    ERR_IVE_QUERY_TIMEOUT = 0x41, /* IVE query timeout */
    ERR_IVE_OPEN_FILE = 0x42,     /* IVE open file error */
    ERR_IVE_READ_FILE = 0x43,     /* IVE read file error */
    ERR_IVE_WRITE_FILE = 0x44,    /* IVE write file error */
    ERR_IVE_BUS_ERR = 0x45,

    ERR_IVE_BUTT
} EN_IVE_ERR_CODE_E;

typedef struct scIVE_DATA_S
{
    SC_U64 u64PhyAddr; /* RW;The physical address of the data */
    SC_U64 u64VirAddr; /* RW;The virtaul address of the data */

    SC_U32 u32Stride; /* RW;The stride of 2D data by byte */
    SC_U32 u32Width;  /* RW;The width of 2D data by byte */
    SC_U32 u32Height; /* RW;The height of 2D data by byte */

    SC_U32 u32Reserved;
} IVE_DATA_S;
typedef IVE_DATA_S IVE_SRC_DATA_S;
typedef IVE_DATA_S IVE_DST_DATA_S;



#define SC_GDC_OP_NUM_MAX 8

#define CHECK_GDC_WH_VALID(num)  			(0 < (num) && (num) <= 3840)


typedef struct
{
    SC_FLOAT fAngle; // 0 - 360; ONLY SUPPORT 0, 90, 180, 270 now.
} GDC_ROTATE_ATTR_S;
typedef struct
{
    SC_BOOL bEnable;
} GDC_FLIP_ATTR_S;
typedef struct
{
    SC_BOOL bEnable;
} GDC_MIRROR_ATTR_S;

typedef struct
{
  SC_FLOAT k[9];
  SC_FLOAT ldc_k0 ;
  SC_FLOAT ldc_k1 ;
  SC_FLOAT ldc_k2 ;
} GDC_LDC_ATTR_S;
typedef struct
{
    SC_U64 u64LutPaAddr;//新增
    SC_U64 u64LutVirtAddr;
	SC_U32 u32LutLen;
} GDC_USER_ATTR_S;


/* 5 action params*/
typedef enum
{
    SC_GDC_ROTATE_E = 1,
    SC_GDC_FLIP_E = 2,
    SC_GDC_MIRROR_E = 3,
    SC_GDC_LDC_E = 4,
    SC_GDC_EIS_E = 5,
    SC_GDC_USER_DEF_E = 6,
    SC_GDC_BUTT_E
} GDC_OP_TYPE_E;

typedef struct
{
    union
    {
        GDC_OP_TYPE_E enType;
        char place_holder0[8];//32位 64位系统长度兼容
    };
    union
    {
        void * pAttr; //Points to attributes.
        char place_holder1[8];//32位 64位系统长度兼容
    };
} GDC_TRANS_PARAM_S;




#define GDC_MAX_PANNEL_COUNT 4

typedef struct {
	int width;
	int height;
	int luma_stride;
	int chroma_stride;
} gdc_buffer_format_t;

typedef struct {
	unsigned long long addr_virt;
	unsigned long long addr_phy;
} gdc_buffer_pannel_t;

typedef struct {
	gdc_buffer_pannel_t pannel[GDC_MAX_PANNEL_COUNT]; //GDC DATA 地址
	gdc_buffer_format_t format;  //GDC 图像格式
} GDC_BUFFER_S;

typedef struct scGDC_TRANSFORM_S
{
	GDC_BUFFER_S stInBuffer;
	GDC_BUFFER_S stOutBuffer;
	GDC_TRANS_PARAM_S stParams[SC_GDC_OP_NUM_MAX];
}GDC_TRANSFORM_S;

/************************************************IVE error code ***********************************/
/* Invalid device ID */
#define SC_ERR_IVE_INVALID_DEVID    SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* Invalid channel ID */
#define SC_ERR_IVE_INVALID_CHNID    SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* At least one parameter is illegal. For example, an illegal enumeration value exists. */
#define SC_ERR_IVE_ILLEGAL_PARAM    SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* The channel exists. */
#define SC_ERR_IVE_EXIST            SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* The UN exists. */
#define SC_ERR_IVE_UNEXIST          SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* A null point is used. */
#define SC_ERR_IVE_NULL_PTR         SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* Try to enable or initialize the system, device, or channel before configuring attributes. */
#define SC_ERR_IVE_NOT_CONFIG       SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* The operation is not supported currently. */
#define SC_ERR_IVE_NOT_SURPPORT     SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* The operation, changing static attributes for example, is not permitted. */
#define SC_ERR_IVE_NOT_PERM         SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* A failure caused by the malloc memory occurs. */
#define SC_ERR_IVE_NOMEM            SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* A failure caused by the malloc buffer occurs. */
#define SC_ERR_IVE_NOBUF            SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* The buffer is empty. */
#define SC_ERR_IVE_BUF_EMPTY        SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* No buffer is provided for storing new data. */
#define SC_ERR_IVE_BUF_FULL         SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* The system is not ready because it may be not initialized or loaded.
 * The error code is returned when a device file fails to be opened. */
#define SC_ERR_IVE_NOTREADY         SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/* The source address or target address is incorrect during the operations such as calling
copy_from_user or copy_to_user. */
#define SC_ERR_IVE_BADADDR          SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
/* The resource is busy during the operations such as destroying a VENC channel
without deregistering it. */
#define SC_ERR_IVE_BUSY             SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/* IVE process timeout: 0xA01D8040 */
#define SC_ERR_IVE_SYS_TIMEOUT      SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_SYS_TIMEOUT)
/* IVE query timeout: 0xA01D8041 */
#define SC_ERR_IVE_QUERY_TIMEOUT    SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_QUERY_TIMEOUT)
/* IVE open file error: 0xA01D8042 */
#define SC_ERR_IVE_OPEN_FILE        SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_OPEN_FILE)
/* IVE read file error: 0xA01D8043 */
#define SC_ERR_IVE_READ_FILE        SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_READ_FILE)
/* IVE read file error: 0xA01D8044 */
#define SC_ERR_IVE_WRITE_FILE       SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_WRITE_FILE)
/* IVE Bus error: 0xA01D8045 */
#define SC_ERR_IVE_BUS_ERR          SC_DEF_ERR(SC_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_BUS_ERR)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_COMM_IVE_H__ */
