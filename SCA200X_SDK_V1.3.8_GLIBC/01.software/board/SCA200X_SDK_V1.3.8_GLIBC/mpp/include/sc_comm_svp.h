#include "sc_type.h"
#include "sc_errno.h"

#ifndef __SC_COMM_SVP_H__
#define __SC_COMM_SVP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/* Image type */
typedef enum scSVP_IMAGE_TYPE_E
{
    SVP_IMG_YUV444P = 0,
    SVP_IMG_YUV444SP = 1,
    SVP_IMG_YUV422P = 2,
    SVP_IMG_YUV422SP = 3,
    SVP_IMG_YUV420P = 4,
    SVP_IMG_YUV420SP = 5,

    SVP_IMG_RGB = 10,              //planar
    SVP_IMG_BGR = 11,              //planar
    SVP_IMG_RGBD = 12,             //planar

    SVP_IMG_GRAY = 15,

    SVP_IMAGE_TYPE_BUTT
} SVP_IMAGE_TYPE_E;

/* Mem information */
typedef struct scSVP_MEM_INFO_S
{
    SC_U64  u64PhyAddr; /* RW;The physical address of the memory */
    SC_U64  u64VirAddr; /* RW;The virtual address of the memory */
    SC_U64  u64Size;    /* RW;The size of memory */
} SVP_MEM_INFO_S;

/* Image */
typedef struct scSVP_IMAGE_S
{
    SC_U64  au64PhyAddr[4]; /* RW;The physical address of the image */
    SC_U64  au64VirAddr[4]; /* RW;The virtual address of the image */
    SC_U32  au32Stride[4];  /* RW;The stride of the image */
    SC_U32  u32Width;       /* RW;The width of the image */
    SC_U32  u32Height;      /* RW;The height of the image */
    SVP_IMAGE_TYPE_E  enType; /* RW;The type of the image */
} SVP_IMAGE_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_COMM_SVP_H__ */
