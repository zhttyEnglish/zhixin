#ifndef _SC_IVE_H_
#define _SC_IVE_H_

#include "sc_comm_ive.h"
#include "sc_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define IVE_HIST_NUM          256
#define IVE_MAP_NUM           256
#define IVE_MAX_REGION_NUM    254
#define IVE_ST_MAX_CORNER_NUM 500

/*
* DMA mode ,created by Chen Quanfu 2013-07-19
*/
typedef enum scIVE_DMA_MODE_E
{
    IVE_DMA_MODE_DIRECT_COPY = 0x0,
    IVE_DMA_MODE_INTERVAL_COPY = 0x1,
    IVE_DMA_MODE_SET_3BYTE = 0x2,
    IVE_DMA_MODE_SET_8BYTE = 0x3,
    IVE_DMA_MODE_BUTT
} IVE_DMA_MODE_E;

/*
* DMA control parameter ,created by Chen Quanfu 2013-07-19
*/
typedef struct scIVE_DMA_CTRL_S
{
    IVE_DMA_MODE_E enMode;
    SC_U64 u64Val;      /* Used in memset mode */
    SC_U8 u8HorSegSize; /* Used in interval-copy mode, every row was segmented by u8HorSegSize bytes,
                         restricted in values of 2,3,4,8,16 */
    SC_U8 u8ElemSize;   /* Used in interval-copy mode, the valid bytes copied in front of every segment
                        in a valid row, which 0<u8ElemSize<u8HorSegSize */
    SC_U8 u8VerSegRows; /* Used in interval-copy mode, copy one row in every u8VerSegRows */
} IVE_DMA_CTRL_S;

/*
* Resize zoom mode
*/
typedef enum scIVE_RESIZE_MODE_E
{
    IVE_RESIZE_MODE_LINEAR   =  0x0, /* Bilinear interpolation */
    IVE_RESIZE_MODE_AREA     =  0x1, /* Area-based (or super) interpolation */
    IVE_RESIZE_MODE_BUTT
} IVE_RESIZE_MODE_E;

/*
* Resize ctrl param
*/
typedef struct scIVE_RESIZE_CTRL_S
{
    IVE_RESIZE_MODE_E enMode;
    IVE_MEM_INFO_S stMem;
    SC_U16 u16Num;
} IVE_RESIZE_CTRL_S;

typedef struct scIVE_RESIZE2_CTRL_S
{
    SC_U16 u16Num;
} IVE_RESIZE2_CTRL_S;

typedef enum scIVE_CROP_COORDINATE_E
{
    IVE_CROP_RATIO_COOR = 0,   /* Ratio coordinate. */
    IVE_CROP_ABS_COOR          /* Absolute coordinate. */
} IVE_CROP_COORDINATE_E;

typedef struct scIVE_CROP_INFO_S
{
    SC_BOOL                 bEnable;            /* RW; Range: [0, 1];  CROP enable. */
    IVE_CROP_COORDINATE_E   enCropCoordinate;   /* RW;  Range: [0, 1]; Coordinate mode of the crop start point. */
    RECT_S                  stCropRect;         /* CROP rectangular. */
} IVE_CROP_INFO_S;

/*
* Region struct
*/
typedef struct scIVE_REGION_S
{
    SC_U32 u32Area;   /* Represented by the pixel number */
    SC_U16 u16Left;   /* Circumscribed rectangle left border */
    SC_U16 u16Right;  /* Circumscribed rectangle right border */
    SC_U16 u16Top;    /* Circumscribed rectangle top border */
    SC_U16 u16Bottom; /* Circumscribed rectangle bottom border */
} IVE_REGION_S;

/*
* CCBLOB struct
*/
typedef struct scIVE_CCBLOB_S
{
    SC_U16 u16CurAreaThr;                       /* Threshold of the result regions' area */
    SC_S8 s8LabelStatus;                        /* -1: Labeled failed ; 0: Labeled successfully */
    SC_U8 u8RegionNum;                          /* Number of valid region, non-continuous stored */
    IVE_REGION_S astRegion[IVE_MAX_REGION_NUM]; /* Valid regions with 'u32Area>0' and 'label = ArrayIndex+1' */
} IVE_CCBLOB_S;

/*
* Type of the CCL
*/
typedef enum scIVE_CCL_MODE_E
{
    IVE_CCL_MODE_4C = 0x0, /* 4-connected */
    IVE_CCL_MODE_8C = 0x1, /* 8-connected */

    IVE_CCL_MODE_BUTT
} IVE_CCL_MODE_E;
/*
* CCL control struct
*/
typedef struct scIVE_CCL_CTRL_S
{
    IVE_CCL_MODE_E enMode; /* Mode */
    SC_U16 u16InitAreaThr; /* Init threshold of region area */
    SC_U16 u16Step;        /* Increase area step for once */
} IVE_CCL_CTRL_S;

/*
* Add control parameters,created by Chen Quanfu 2013-07-17
*/
typedef struct scIVE_ADD_CTRL_S
{
    SC_U0Q16 u0q16X; /* x of "xA+yB" */
    SC_U0Q16 u0q16Y; /* y of "xA+yB" */
} IVE_ADD_CTRL_S;

/*
* Sad mode
*/
typedef enum scIVE_SAD_MODE_E
{
    IVE_SAD_MODE_MB_4X4 = 0x0,   /* 4x4 */
    IVE_SAD_MODE_MB_8X8 = 0x1,   /* 8x8 */
    IVE_SAD_MODE_MB_16X16 = 0x2, /* 16x16 */

    IVE_SAD_MODE_BUTT
} IVE_SAD_MODE_E;
/*
* Sad output ctrl
*/
typedef enum scIVE_SAD_OUT_CTRL_E
{
    IVE_SAD_OUT_CTRL_16BIT_BOTH = 0x0, /* Output 16 bit sad and thresh */
    IVE_SAD_OUT_CTRL_8BIT_BOTH = 0x1,  /* Output 8 bit sad and thresh */
    IVE_SAD_OUT_CTRL_16BIT_SAD = 0x2,  /* Output 16 bit sad */
    IVE_SAD_OUT_CTRL_8BIT_SAD = 0x3,   /* Output 8 bit sad */
    IVE_SAD_OUT_CTRL_THRESH = 0x4,     /* Output thresh,16 bits sad */

    IVE_SAD_OUT_CTRL_BUTT
} IVE_SAD_OUT_CTRL_E;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*_SC_IVE_H_*/
