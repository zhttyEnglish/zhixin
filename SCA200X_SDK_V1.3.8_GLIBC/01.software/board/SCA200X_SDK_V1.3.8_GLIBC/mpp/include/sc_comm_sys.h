/**
 * @file     sc_comm_sys.h
 * @brief    sys模块基础类型定义
 * @version  1.0.0
 * @since    1.0.0
 * @author  刘彬<liubin@sgitg.sgcc.com.cn>
 * @date    2021-07-15 创建文件
 */
/************************************************************
 *@note
    Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
                   ALL RIGHTS RESERVED
    Permission is hereby granted to licensees of  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. to use
    or abstract this computer program for the sole purpose of implementing a product based on BEIJIING SMARTCHIP
    MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use, or disseminate this computer program,
    whether in part or in whole, are granted. BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no
    representation or warranties with respect to the performance of this computer program, and specifically disclaims
    any responsibility for any damages, special or consequential, connected with the use of this program.
　　For details, see http://www.sgitg.sgcc.com.cn/
**********************************************************/

#ifndef __SC_COMM_SYS_H__
#define __SC_COMM_SYS_H__

#include "sc_type.h"
#include "sc_errno.h"
#include "sc_defines.h"
#include "sc_comm_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define BIND_DEST_MAXNUM         64

#define SC_TRACE_SYS(level, fmt, ...)                                                                         \
    do {                                                                                                      \
        SC_TRACE(level, SC_ID_SYS, "[Func]:%s [Line]:%d [Info]:" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)

typedef struct scVPSS_VENC_WRAP_PARAM_S
{
    SC_BOOL bAllOnline;

    SC_U32 u32FrameRate; /* Input frame rate of VIPROC(linear/Frame mode WDR is the sensor frame rate,
                          * and the line mode WDR is the frame rate after synthesis) */
    SC_U32 u32FullLinesStd; /* Total height of sensor, include of VBlank */

    SIZE_S stLargeStreamSize; /* VENC Large Stream Size */
    SIZE_S stSmallStreamSize; /* VENC Small Stream Size */

} VPSS_VENC_WRAP_PARAM_S;

typedef struct scMPP_SYS_CONFIG_S
{
    SC_U32 u32Align;

} MPP_SYS_CONFIG_S;

typedef struct scSYS_VIRMEM_INFO_S
{
    SC_U64 u64PhyAddr;
    SC_BOOL bCached;
} SYS_VIRMEM_INFO_S;

typedef struct scMPP_BIND_DEST_S
{
    SC_U32 u32Num;
    MPP_CHN_S astMppChn[BIND_DEST_MAXNUM];
} MPP_BIND_DEST_S;

typedef enum scSCALE_RANGE_E
{
    SCALE_RANGE_0 = 0, /* scale range <   8/64 */
    SCALE_RANGE_1, /* scale range >=  8/64 */
    SCALE_RANGE_2, /* scale range >= 10/64 */
    SCALE_RANGE_3, /* scale range >= 15/64 */
    SCALE_RANGE_4, /* scale range >= 19/64 */
    SCALE_RANGE_5, /* scale range >= 24/64 */
    SCALE_RANGE_6, /* scale range >= 29/64 */
    SCALE_RANGE_7, /* scale range >= 33/64 */
    SCALE_RANGE_8, /* scale range >= 35/64 */
    SCALE_RANGE_9, /* scale range >= 38/64 */
    SCALE_RANGE_10, /* scale range >= 42/64 */
    SCALE_RANGE_11, /* scale range >= 45/64 */
    SCALE_RANGE_12, /* scale range >= 48/64 */
    SCALE_RANGE_13, /* scale range >= 51/64 */
    SCALE_RANGE_14, /* scale range >= 53/64 */
    SCALE_RANGE_15, /* scale range >= 55/64 */
    SCALE_RANGE_16, /* scale range >= 57/64 */
    SCALE_RANGE_17, /* scale range >= 60/64 */
    SCALE_RANGE_18, /* scale range >  1     */
    SCALE_RANGE_BUTT,
} SCALE_RANGE_E;

typedef enum scCOEFF_LEVEL_E
{
    COEFF_LEVEL_0 = 0, /* coefficient level 0 */
    COEFF_LEVEL_1, /* coefficient level 1 */
    COEFF_LEVEL_2, /* coefficient level 2 */
    COEFF_LEVEL_3, /* coefficient level 3 */
    COEFF_LEVEL_4, /* coefficient level 4 */
    COEFF_LEVEL_5, /* coefficient level 5 */
    COEFF_LEVEL_6, /* coefficient level 6 */
    COEFF_LEVEL_7, /* coefficient level 7 */
    COEFF_LEVEL_8, /* coefficient level 8 */
    COEFF_LEVEL_9, /* coefficient level 9 */
    COEFF_LEVEL_10, /* coefficient level 10 */
    COEFF_LEVEL_11, /* coefficient level 11 */
    COEFF_LEVEL_12, /* coefficient level 12 */
    COEFF_LEVEL_13, /* coefficient level 13 */
    COEFF_LEVEL_14, /* coefficient level 14 */
    COEFF_LEVEL_15, /* coefficient level 15 */
    COEFF_LEVEL_16, /* coefficient level 16 */
    COEFF_LEVEL_17, /* coefficient level 17 */
    COEFF_LEVEL_18, /* coefficient level 18 */
    COEFF_LEVEL_BUTT,
} COEFF_LEVEL_E;

typedef struct scSCALE_COEFF_LEVEL_S
{
    COEFF_LEVEL_E enHorLum; /* horizontal luminance   coefficient level */
    COEFF_LEVEL_E enHorChr; /* horizontal chrominance coefficient level */
    COEFF_LEVEL_E enVerLum; /* vertical   luminance   coefficient level */
    COEFF_LEVEL_E enVerChr; /* vertical   chrominance coefficient level */
} SCALE_COEFF_LEVEL_S;

typedef struct scSCALE_RANGE_S
{
    SCALE_RANGE_E enHorizontal;
    SCALE_RANGE_E enVertical;
} SCALE_RANGE_S;

typedef struct scSCALE_COEFF_INFO_S
{
    SCALE_RANGE_S stScaleRange;
    SCALE_COEFF_LEVEL_S stScaleCoeffLevel;
} SCALE_COEFF_INFO_S;

typedef struct scGPS_INFO_S
{
    SC_CHAR chGPSLatitudeRef; /* GPS LatitudeRef Indicates whether the latitude is north or south latitude,
                               * 'N'/'S', default 'N' */
    SC_U32 au32GPSLatitude[3][2]; /* GPS Latitude is expressed as degrees, minutes and seconds, a typical format
                                   * like "dd/1, mm/1, ss/1", default 0/0, 0/0, 0/0 */
    SC_CHAR chGPSLongitudeRef; /* GPS LongitudeRef Indicates whether the longitude is east or west longitude,
                                * 'E'/'W', default 'E' */
    SC_U32 au32GPSLongitude[3][2]; /* GPS Longitude is expressed as degrees, minutes and seconds, a typical format
                                    * like "dd/1, mm/1, ss/1", default 0/0, 0/0, 0/0 */
    SC_U8 u8GPSAltitudeRef; /* GPS AltitudeRef Indicates the reference altitude used, 0 - above sea level,
                             * 1 - below sea level default 0 */
    SC_U32 au32GPSAltitude[2]; /* GPS AltitudeRef Indicates the altitude based on the reference u8GPSAltitudeRef,
                                * the reference unit is meters, default 0/0 */
} GPS_INFO_S;

typedef enum scVI_VPSS_MODE_E
{
    VI_OFFLINE_VPSS_OFFLINE = 0,
    VI_OFFLINE_VPSS_ONLINE,
    VI_ONLINE_VPSS_OFFLINE,
    VI_ONLINE_VPSS_ONLINE,
    VI_PARALLEL_VPSS_OFFLINE,
    VI_PARALLEL_VPSS_PARALLEL,
    VI_MULTI_VPSS_OFFLINE,  //todo add two sensor
    VI_VPSS_MODE_BUTT
} VI_VPSS_MODE_E;

typedef struct scVI_VPSS_MODE_S
{
    VI_VPSS_MODE_E aenMode[VI_MAX_PIPE_NUM];
} VI_VPSS_MODE_S;

typedef struct scRAW_FRAME_COMPRESS_PARAM_S
{
    SC_U32 u32CompRatio_10Bit; /* compression ratio of 10 bit */
    SC_U32 u32CompRatio_12Bit; /* compression ratio of 12 bit */
    SC_U32 u32CompRatio_14Bit; /* compression ratio of 14 bit */
} RAW_FRAME_COMPRESS_PARAM_S;

/*********************************************************************************************/
/* invlalid channel ID */
#define SC_ERR_SYS_INVALID_CHNID SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define SC_ERR_SYS_ILLEGAL_PARAM SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* channel exists */
#define SC_ERR_SYS_EXIST         SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* using a NULL point */
#define SC_ERR_SYS_NULL_PTR      SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define SC_ERR_SYS_NOT_CONFIG    SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define SC_ERR_SYS_NOT_SUPPORT   SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* operation is not permitted ,eg, try to change stati attribute */
#define SC_ERR_SYS_NOT_PERM      SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* the channle is not existed  */
#define SC_ERR_SYS_UNEXIST       SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* failure caused by malloc memory */
#define SC_ERR_SYS_NOMEM         SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define SC_ERR_SYS_NOBUF         SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define SC_ERR_SYS_BUF_EMPTY     SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define SC_ERR_SYS_BUF_FULL      SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* system is not ready,had not initialed or loaded */
#define SC_ERR_SYS_SYS_NOTREADY  SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/* system busy */
#define SC_ERR_SYS_BUSY          SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)

/* bad address,  eg. used for copy_from_user & copy_to_user   */
#define SC_ERR_SYS_BADADDR       SC_DEF_ERR(SC_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_COMM_SYS_H__ */

