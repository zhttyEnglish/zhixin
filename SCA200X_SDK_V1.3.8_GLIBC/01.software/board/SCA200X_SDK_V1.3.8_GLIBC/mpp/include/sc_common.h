/*
 * @file     sc_common.h
 * @brief    模块共用的定义
 * @version  1.0.1
 * @since    1.0.0
 * @author
 */

/*
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

#ifndef _SC_COMMON_H_
#define _SC_COMMON_H_

#include <string.h>

#include "sc_type.h"
#include "sc_defines.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define ATTRIBUTE                       __attribute__((aligned(ALIGN_NUM)))
#define VERSION_NAME_MAXLEN             64

typedef struct scMPP_VERSION_S
{
    SC_CHAR aVersion[VERSION_NAME_MAXLEN];
} MPP_VERSION_S;

typedef SC_S32 AI_CHN;
typedef SC_S32 AO_CHN;
typedef SC_S32 AENC_CHN;
typedef SC_S32 ADEC_CHN;
typedef SC_S32 AUDIO_DEV;
typedef SC_UCHAR MD_CHN;

typedef SC_S32 VI_DEV;
typedef SC_S32 VI_PIPE;
typedef SC_S32 VI_CHN;
typedef SC_S32 VI_STITCH_GRP;

typedef SC_S32 MIPI_DEV;
typedef SC_S32 SLAVE_DEV;
typedef SC_S32 ISP_DEV;
typedef SC_S32 SENSOR_ID;

typedef SC_S32 VO_DEV;
typedef SC_S32 VO_LAYER;
typedef SC_S32 VO_CHN;
typedef SC_S32 VO_WBC;
typedef SC_S32 GRAPHIC_LAYER;
typedef SC_S32 VENC_CHN;
typedef SC_S32 VDEC_CHN;

typedef SC_S32 VPSS_GRP;
typedef SC_S32 VPSS_GRP_PIPE;
typedef SC_S32 VPSS_CHN;

typedef SC_S32 SVP_NPU_HANDLE;
#define SC_INVALID_VALUE       (-1)
#define SC_INVALID_TYPE        (-1)

#define SC_INVALID_DEV         (-1)
#define SC_INVALID_CHN         (-1)

#define SC_INVALID_HANDLE      (-1)

typedef enum scMOD_ID_E
{
    SC_ID_VB      = 0,
    SC_ID_SYS,
    SC_ID_RGN,
    SC_ID_VI,
    SC_ID_DIS, /* Digital Image Stabilisation */
    SC_ID_ISP,
    SC_ID_VO,
    SC_ID_VENC,
    SC_ID_VDEC,
    SC_ID_VPSS,
    SC_ID_VGS,
    SC_ID_AI,
    SC_ID_AO,
    SC_ID_AENC,
    SC_ID_ADEC,
    SC_ID_LOG,
    SC_ID_IVE,
    SC_ID_SVP_NPU     = 51,
    SC_ID_H264,
    SC_ID_H265,
    SC_ID_JPEG,
    SC_ID_RC,

    SC_ID_BUTT,
    SC_ID_CNT = SC_ID_BUTT,

    SC_ID_ALL,
} MOD_ID_E;

typedef struct scMPP_CHN_S
{
    MOD_ID_E    enModId;
    SC_S32      s32DevId;
    SC_S32      s32ChnId;
} MPP_CHN_S;

#define MPP_MOD_VB             "vb"
#define MPP_MOD_SYS            "sys"

#define MPP_MOD_RGN            "rgn"
#define MPP_MOD_VI             "vi"
#define MPP_MOD_ISP            "isp"
#define MPP_MOD_VO             "vo"
#define MPP_MOD_VENC           "venc"
#define MPP_MOD_VDEC           "vdec"
#define MPP_MOD_VPSS           "vpss"
#define MPP_MOD_VGS            "vgs"
#define MPP_MOD_AI             "ai"
#define MPP_MOD_AO             "ao"
#define MPP_MOD_AENC           "aenc"
#define MPP_MOD_ADEC           "adec"

#define MPP_MOD_H264           "h264e"
#define MPP_MOD_H265           "h265e"
#define MPP_MOD_JPEG           "jpege"
#define MPP_MOD_RC             "rc"

#define MPP_MOD_LOG            "logmpp"

/* We just coyp this value of payload type from RTP/RTSP definition */
typedef enum
{
    PT_PCMU          = 0,
    PT_1016          = 1,
    PT_G721          = 2,
    PT_GSM           = 3,
    PT_G723          = 4,
    PT_DVI4_8K       = 5,
    PT_DVI4_16K      = 6,
    PT_LPC           = 7,
    PT_PCMA          = 8,
    PT_G722          = 9,
    PT_S16BE_STEREO  = 10,
    PT_S16BE_MONO    = 11,
    PT_QCELP         = 12,
    PT_CN            = 13,
    PT_MPEGAUDIO     = 14,
    PT_G728          = 15,
    PT_DVI4_3        = 16,
    PT_DVI4_4        = 17,
    PT_G729          = 18,
    PT_G711A         = 19,
    PT_G711U         = 20,
    PT_G726          = 21,
    PT_G729A         = 22,
    PT_LPCM          = 23,
    PT_CelB          = 25,
    PT_JPEG          = 26,
    PT_CUSM          = 27,
    PT_NV            = 28,
    PT_PICW          = 29,
    PT_CPV           = 30,
    PT_H261          = 31,
    PT_MPEGVIDEO     = 32,
    PT_MPEG2TS       = 33,
    PT_H263          = 34,
    PT_SPEG          = 35,
    PT_MPEG2VIDEO    = 36,
    PT_AAC           = 37,
    PT_WMA9STD       = 38,
    PT_HEAAC         = 39,
    PT_PCM_VOICE     = 40,
    PT_PCM_AUDIO     = 41,
    PT_MP3           = 43,
    PT_ADPCMA        = 49,
    PT_AEC           = 50,
    PT_X_LD          = 95,
    PT_H264          = 96,
    PT_D_GSM_HR      = 200,
    PT_D_GSM_EFR     = 201,
    PT_D_L8          = 202,
    PT_D_RED         = 203,
    PT_D_VDVI        = 204,
    PT_D_BT656       = 220,
    PT_D_H263_1998   = 221,
    PT_D_MP1S        = 222,
    PT_D_MP2P        = 223,
    PT_D_BMPEG       = 224,
    PT_MP4VIDEO      = 230,
    PT_MP4AUDIO      = 237,
    PT_VC1           = 238,
    PT_JVC_ASF       = 255,
    PT_D_AVI         = 256,
    PT_DIVX3         = 257,
    PT_AVS           = 258,
    PT_REAL8         = 259,
    PT_REAL9         = 260,
    PT_VP6           = 261,
    PT_VP6F          = 262,
    PT_VP6A          = 263,
    PT_SORENSON      = 264,
    PT_H265          = 265,
    PT_VP8           = 266,
    PT_MVC           = 267,
    PT_PNG           = 268,
    /* not in RTP/RTSP */
    PT_AMR           = 1001,
    PT_MJPEG         = 1002,
    PT_AMRWB         = 1003,
    PT_PRORES        = 1006,
    PT_OPUS          = 1007,
    PT_BUTT
} PAYLOAD_TYPE_E;

#define LOCK(mutex)   pthread_mutex_lock(&mutex)
#define UNLOCK(mutex) pthread_mutex_unlock(&mutex)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* _SC_COMMON_H_ */

