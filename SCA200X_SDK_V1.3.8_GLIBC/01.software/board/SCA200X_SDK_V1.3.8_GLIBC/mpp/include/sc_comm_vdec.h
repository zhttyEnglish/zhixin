/**
 * @file     sc_comm_vdec.h
 * @brief    视频解码模块的api参数定义
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
 * @date    2021-07-15 创建文件
 */
/**
 ********************************************************************************************************
 * Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 *             ALL RIGHTS RESERVED
 * Permission is hereby granted to licensees of BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 * to use or abstract this computer program for the sole purpose of implementing a product based on
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use,
 * or disseminate this computer program,whether in part or in whole, are granted.
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no representation or warranties
 * with respect to the performance of this computer program, and specifically disclaims
 * any responsibility for any damages, special or consequential, connected with the use of this program.
 * For details, see http://www.sgitg.sgcc.com.cn/
 ********************************************************************************************************
*/

#ifndef __SC_COMM_VDEC_H__
#define __SC_COMM_VDEC_H__

#include "sc_type.h"
#include "sc_common.h"
#include "sc_errno.h"
#include "sc_comm_video.h"
#include "sc_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define SC_IO_BLOCK               SC_TRUE
#define SC_IO_NOBLOCK             SC_FALSE

typedef enum scVIDEO_MODE_E
{
    VIDEO_MODE_STREAM = 0, /* send by stream */
    VIDEO_MODE_FRAME, /* send by frame  */
    VIDEO_MODE_COMPAT, /* One frame supports multiple packets sending. */
    /* The current frame is considered to end when bEndOfFrame is equal to SC_TRUE */
    VIDEO_MODE_BUTT
} VIDEO_MODE_E;

typedef struct scVDEC_ATTR_VIDEO_S
{
    SC_U32 u32RefFrameNum; /* RW, Range: [0, 16]; reference frame num. */
    SC_BOOL bTemporalMvpEnable; /* RW; */
    /* specifies whether temporal motion vector predictors can be used for inter prediction */
    SC_U32 u32TmvBufSize; /* RW; tmv buffer size(Byte) */
} VDEC_ATTR_VIDEO_S;

typedef struct scVDEC_CHN_ATTR_S
{
    PAYLOAD_TYPE_E enType; /* RW; video type to be decoded   */
    VIDEO_MODE_E enMode; /* RW; send by stream or by frame */
    SC_U32 u32PicWidth; /* RW; max pic width */
    SC_U32 u32PicHeight; /* RW; max pic height */
    SC_U32 u32StreamBufSize; /* RW; stream buffer size(Byte) */
    SC_U32 u32FrameBufSize; /* RW; frame buffer size(Byte) */
    SC_U32 u32FrameBufCnt;
    union
    {
        VDEC_ATTR_VIDEO_S stVdecVideoAttr; /* structure with video ( h264/h265) */
    };
} VDEC_CHN_ATTR_S;

typedef struct scVDEC_STREAM_S
{
    SC_U32 u32Len; /* W; stream len */
    SC_U64 u64PTS; /* W; time stamp */
    SC_U64 u64PhyAddr; /* W; time stamp */
    SC_BOOL bEndOfFrame; /* W; is the end of a frame */
    SC_BOOL bEndOfStream; /* W; is the end of all stream */
    SC_BOOL bDisplay; /* W; is the current frame displayed. only valid by VIDEO_MODE_FRAME */
    SC_U8 *ATTRIBUTE pu8Addr; /* W; stream address */
} VDEC_STREAM_S;

typedef struct scVDEC_USERDATA_S
{
    SC_U64 u64PhyAddr; /* R; userdata data phy address */
    SC_U32 u32Len; /* R; userdata data len */
    SC_BOOL bValid; /* R; is valid? */
    SC_U8 *ATTRIBUTE pu8Addr; /* R; userdata data vir address */
} VDEC_USERDATA_S;

typedef struct sc_VDEC_DECODE_ERROR_S
{
    SC_S32 s32FormatErr; /* R; format error. eg: do not support filed */
    SC_S32 s32PicSizeErrSet; /* R; picture width or height is larger than chnnel width or height */
    SC_S32 s32StreamUnsprt; /* R; unsupport the stream specification */
    SC_S32 s32PackErr; /* R; stream package error */
    SC_S32 s32PrtclNumErrSet; /* R; protocol num is not enough. eg: slice, pps, sps */
    SC_S32 s32RefErrSet; /* R; refrence num is not enough */
    SC_S32 s32PicBufSizeErrSet; /* R; the buffer size of picture is not enough */
    SC_S32 s32StreamSizeOver; /* R; the stream size is too big and force discard stream */
    SC_S32 s32VdecStreamNotRelease; /* R; the stream not released for too long time */
} VDEC_DECODE_ERROR_S;

typedef struct scVDEC_CHN_STATUS_S
{
    PAYLOAD_TYPE_E enType; /* R; video type to be decoded */
    SC_U32 u32LeftStreamBytes; /* R; left stream bytes waiting for decode */
    SC_U32 u32LeftStreamFrames; /* R; left frames waiting for decode,only valid for VIDEO_MODE_FRAME */
    SC_U32 u32LeftPics; /* R; pics waiting for output */
    SC_BOOL bStartRecvStream; /* R; had started recv stream? */
    SC_U32 u32RecvStreamFrames; /* R; how many frames of stream has been received. valid when send by frame. */
    SC_U32 u32DecodeStreamFrames; /* R; how many frames of stream has been decoded. valid when send by frame. */
    VDEC_DECODE_ERROR_S stVdecDecErr; /* R; information about decode error */
    SC_U32 u32Width; /* R; the width of the currently decoded stream */
    SC_U32 u32Height; /* R; the height of the currently decoded stream */
} VDEC_CHN_STATUS_S;

typedef enum scVIDEO_DEC_MODE_E
{
    VIDEO_DEC_MODE_IPB = 0,
    VIDEO_DEC_MODE_IP,
    VIDEO_DEC_MODE_I,
    VIDEO_DEC_MODE_BUTT
} VIDEO_DEC_MODE_E;

typedef enum scVIDEO_OUTPUT_ORDER_E
{
    VIDEO_OUTPUT_ORDER_DISP = 0,
    VIDEO_OUTPUT_ORDER_DEC,
    VIDEO_OUTPUT_ORDER_BUTT
} VIDEO_OUTPUT_ORDER_E;

typedef struct scVDEC_PARAM_VIDEO_S
{
    SC_S32 s32ErrThreshold; /* RW, Range: [0, 100]; */
    /* threshold for stream error process, 0: discard with any error, 100 : keep data with any error */
    VIDEO_DEC_MODE_E enDecMode; /* RW; */
    /* decode mode , 0: deocde IPB frames, 1: only decode I frame & P frame , 2: only decode I frame */
    VIDEO_OUTPUT_ORDER_E enOutputOrder; /* RW; */
    /* frames output order ,0: the same with display order , 1: the same width decoder order */
    COMPRESS_MODE_E enCompressMode; /* RW; compress mode */
    VIDEO_FORMAT_E enVideoFormat; /* RW; video format */
} VDEC_PARAM_VIDEO_S;

typedef struct scVDEC_PARAM_PICTURE_S
{
    PIXEL_FORMAT_E enPixelFormat; /* RW; out put pixel format */
    SC_U32 u32Alpha; /* RW, Range: [0, 255]; value 0 is transparent. */
    /* [0 ,127]   is deemed to transparent when enPixelFormat is ARGB1555 or ABGR1555
     * [128 ,256] is deemed to non-transparent when enPixelFormat is ARGB1555 or ABGR1555
     */
} VDEC_PARAM_PICTURE_S;

typedef struct scVDEC_CHN_PARAM_S
{
    PAYLOAD_TYPE_E enType; /* RW; video type to be decoded   */
    SC_U32 u32DisplayFrameNum; /* RW, Range: [0, 16]; display frame num */
    union
    {
        VDEC_PARAM_VIDEO_S stVdecVideoParam; /* structure with video ( h265/h264) */
        VDEC_PARAM_PICTURE_S stVdecPictureParam; /* structure with picture (jpeg/mjpeg ) */
    };
} VDEC_CHN_PARAM_S;

typedef struct scH264_PRTCL_PARAM_S
{
    SC_S32 s32MaxSliceNum; /* RW; max slice num support */
    SC_S32 s32MaxSpsNum; /* RW; max sps num support */
    SC_S32 s32MaxPpsNum; /* RW; max pps num support */
} H264_PRTCL_PARAM_S;

typedef struct scH265_PRTCL_PARAM_S
{
    SC_S32 s32MaxSliceSegmentNum; /* RW; max slice segmnet num support */
    SC_S32 s32MaxVpsNum; /* RW; max vps num support */
    SC_S32 s32MaxSpsNum; /* RW; max sps num support */
    SC_S32 s32MaxPpsNum; /* RW; max pps num support */
} H265_PRTCL_PARAM_S;

typedef struct scVDEC_PRTCL_PARAM_S
{
    PAYLOAD_TYPE_E enType; /* RW; video type to be decoded, only h264 and h265 supported */
    union
    {
        H264_PRTCL_PARAM_S stH264PrtclParam; /* protocol param structure for h264 */
        H265_PRTCL_PARAM_S stH265PrtclParam; /* protocol param structure for h265 */
    };
} VDEC_PRTCL_PARAM_S;

typedef struct scVDEC_CHN_POOL_S
{
    SC_U32 hPicVbPool; /* RW;  vb pool id for pic buffer */
    SC_U32 hTmvVbPool; /* RW;  vb pool id for tmv buffer */
} VDEC_CHN_POOL_S;

typedef enum scVDEC_EVNT_E
{
    VDEC_EVNT_STREAM_ERR = 1,
    VDEC_EVNT_UNSUPPORT,
    VDEC_EVNT_OVER_REFTHR,
    VDEC_EVNT_REF_NUM_OVER,
    VDEC_EVNT_SLICE_NUM_OVER,
    VDEC_EVNT_SPS_NUM_OVER,
    VDEC_EVNT_PPS_NUM_OVER,
    VDEC_EVNT_PICBUF_SIZE_ERR,
    VDEC_EVNT_SIZE_OVER,
    VDEC_EVNT_IMG_SIZE_CHANGE,
    VDEC_EVNT_VPS_NUM_OVER,
    VDEC_EVNT_BUTT
} VDEC_EVNT_E;

typedef enum scVDEC_CAPACITY_STRATEGY_E
{
    VDEC_CAPACITY_STRATEGY_BY_MOD = 0,
    VDEC_CAPACITY_STRATEGY_BY_CHN = 1,
    VDEC_CAPACITY_STRATEGY_BUTT
} VDEC_CAPACITY_STRATEGY_E;

typedef struct scVDEC_VIDEO_MOD_PARAM_S
{
    SC_U32 u32MaxPicWidth;
    SC_U32 u32MaxPicHeight;
    SC_U32 u32MaxSliceNum;
    SC_U32 u32VdhMsgNum;
    SC_U32 u32VdhBinSize;
    SC_U32 u32VdhExtMemLevel;
} VDEC_VIDEO_MOD_PARAM_S;

typedef struct scVDEC_PICTURE_MOD_PARAM_S
{
    SC_U32 u32MaxPicWidth;
    SC_U32 u32MaxPicHeight;
    SC_BOOL bSupportProgressive;
    SC_BOOL bDynamicAllocate;
    VDEC_CAPACITY_STRATEGY_E enCapStrategy;
} VDEC_PICTURE_MOD_PARAM_S;

typedef struct scVDEC_MOD_PARAM_S
{
    VB_SOURCE_E enVdecVBSource; /* RW, Range: [1, 3];  frame buffer mode  */
    SC_U32 u32MiniBufMode; /* RW, Range: [0, 1];  stream buffer mode */
    SC_U32 u32ParallelMode; /* RW, Range: [0, 1];  VDH working mode   */
    VDEC_VIDEO_MOD_PARAM_S stVideoModParam;
    VDEC_PICTURE_MOD_PARAM_S stPictureModParam;
} VDEC_MOD_PARAM_S;

/*********************************************************************************************/
/* invlalid channel ID */
#define SC_ERR_VDEC_INVALID_CHNID SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define SC_ERR_VDEC_ILLEGAL_PARAM SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* channel exists */
#define SC_ERR_VDEC_EXIST         SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* using a NULL point */
#define SC_ERR_VDEC_NULL_PTR      SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define SC_ERR_VDEC_NOT_CONFIG    SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define SC_ERR_VDEC_NOT_SUPPORT   SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* operation is not permitted ,eg, try to change stati attribute */
#define SC_ERR_VDEC_NOT_PERM      SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* the channle is not existed  */
#define SC_ERR_VDEC_UNEXIST       SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* failure caused by malloc memory */
#define SC_ERR_VDEC_NOMEM         SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define SC_ERR_VDEC_NOBUF         SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define SC_ERR_VDEC_BUF_EMPTY     SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define SC_ERR_VDEC_BUF_FULL      SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* system is not ready,had not initialed or loaded */
#define SC_ERR_VDEC_SYS_NOTREADY  SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/* system busy */
#define SC_ERR_VDEC_BUSY          SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)

/* bad address,  eg. used for copy_from_user & copy_to_user   */
#define SC_ERR_VDEC_BADADDR       SC_DEF_ERR(SC_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef  __SC_COMM_VDEC_H__ */

