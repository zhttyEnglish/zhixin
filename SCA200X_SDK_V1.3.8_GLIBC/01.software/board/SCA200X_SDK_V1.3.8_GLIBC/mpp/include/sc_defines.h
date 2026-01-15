/*
 * @file     sc_defines.h
 * @brief    芯片能力定义
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

#ifndef _SC_DEFINES_H_
#define _SC_DEFINES_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "autoconf.h"

#define ALIGN_NUM                      8
#define DEFAULT_ALIGN                  64
#define MAX_ALIGN                      1024
#define HEIGHT_ALIGN                   64

#define MILLION                        1000000

/* For VB */
#ifndef VB_MAX_POOLS
#define VB_MAX_POOLS                   768
#endif

#ifndef MAX_MMZ_NAME_LEN
#define MAX_MMZ_NAME_LEN               16
#endif

/* For ALL */
#define MAX_FRAMERATE                  240
#define MAX_DEPTH                      64

/* For VENC */
#define VENC_MAX_NAME_LEN              16
#define VENC_MAX_CHN_NUM               16
#define VEDU_IP_NUM                    1
#define H264E_MAX_WIDTH                8192
#define H264E_MAX_HEIGHT               8192
#define H264E_MIN_WIDTH                114
#define H264E_MIN_HEIGHT               114
#define H265E_MAX_WIDTH                8192
#define H265E_MAX_HEIGHT               8192
#define H265E_MIN_WIDTH                114
#define H265E_MIN_HEIGHT               114
#define JPEGE_MAX_WIDTH                8192
#define JPEGE_MAX_HEIGHT               8192
#define JPEGE_MIN_WIDTH                32
#define JPEGE_MIN_HEIGHT               32
#define JPGE_MAX_NUM                   1
#define VENC_MAX_ROI_NUM               8
#define H264E_MIN_HW_INDEX             0
#define H264E_MAX_HW_INDEX             11
#define H264E_MIN_VW_INDEX             0
#define H264E_MAX_VW_INDEX             3
#define VENC_QP_HISGRM_NUM             52
#define MAX_TILE_NUM                   1
#define H265E_ADAPTIVE_FRAME_TYPE      5
#define H265E_ADAPTIVE_QP_TYPE         5

#define VENC_MIN_INPUT_FRAME_RATE      1
#define VENC_MAX_INPUT_FRAME_RATE      240

#define VENC_MAX_RECEIVE_SOURCE        4

#define VENC_PIC_RECEIVE_SOURCE0       0
#define VENC_PIC_RECEIVE_SOURCE1       1
#define VENC_PIC_RECEIVE_SOURCE2       2
#define VENC_PIC_RECEIVE_SOURCE3       3

/* For RC */
#define RC_TEXTURE_THR_SIZE            16
#define MIN_BITRATE                    2
#define MAX_BITRATE                    (200 * 1024)
#define MAX_EXTRA_BITRATE              (1000 * 1024)

/* For VDEC */
#define VDEC_MAX_CHN_NUM               64
#define VDH_MAX_CHN_NUM                64
#define VEDU_CHN_START                 VDH_MAX_CHN_NUM
#define VEDU_H264D_ERRRATE             10
#define VEDU_H264D_FULLERR             100

#define H264D_ALIGN_W                  64
#define H264D_ALIGN_H                  64
#define H265D_ALIGN_W                  64
#define H265D_ALIGN_H                  64
#define JPEGD_ALIGN_W                  64
#define JPEGD_ALIGN_H                  16
#define JPEGD_RGB_ALIGN                16

#define H264D_MAX_SPS                  32
#define H264D_MIN_SPS                  1
#define H264D_MAX_PPS                  256
#define H264D_MIN_PPS                  1
#define H264D_MAX_SLICE                300
#define H264D_MIN_SLICE                1

#define H265D_MAX_VPS                  16
#define H265D_MIN_VPS                  1
#define H265D_MAX_SPS                  16
#define H265D_MIN_SPS                  1
#define H265D_MAX_PPS                  64
#define H265D_MIN_PPS                  1
#define H265D_MAX_SLICE                200
#define H265D_MIN_SLICE                1

#define VDH_H264D_MAX_WIDTH            8192
#define VDH_H264D_MAX_HEIGHT           8192
#define VDH_H264D_MIN_WIDTH            114
#define VDH_H264D_MIN_HEIGHT           114

#define VDH_H265D_MAX_WIDTH            8192
#define VDH_H265D_MAX_HEIGHT           8192
#define VDH_H265D_MIN_WIDTH            114
#define VDH_H265D_MIN_HEIGHT           114
#define VDH_MIN_MSG_NUM                1
#define VDH_MAX_MSG_NUM                8
#define VDH_MIN_BIN_SIZE               72
#define VDH_MAX_BIN_SIZE               2048
#define VDH_MIN_EXT_MEM_LEVEL          1
#define VDH_MAX_EXT_MEM_LEVEL          6

#define VEDU_H264D_MAX_WIDTH           8192
#define VEDU_H264D_MAX_HEIGHT          8192
#define VEDU_H264D_MIN_WIDTH           128
#define VEDU_H264D_MIN_HEIGHT          128

#define VEDU_H265D_MAX_WIDTH           8192
#define VEDU_H265D_MAX_HEIGHT          8192
#define VEDU_H265D_MIN_WIDTH           128
#define VEDU_H265D_MIN_HEIGHT          128

#define JPEGD_IP_NUM                   1
#define JPEGD_MAX_WIDTH                8192
#define JPEGD_MAX_HEIGHT               8192
#define JPEGD_MIN_WIDTH                8
#define JPEGD_MIN_HEIGHT               8

/* For VO */

#define VO_MAX_VIRT_DEV_NUM            0 /* max virtual dev num */
#define VO_VIRT_DEV_0                  2 /* virtual device 0 */
#define VO_VIRT_DEV_1                  3 /* virtual device 1 */
#define VO_VIRT_DEV_2                  4 /* virtual device 2 */
#define VO_VIRT_DEV_3                  5 /* virtual device 3 */

#define VO_VIRT_LAYER_0                3 /* virtual layer 0 */
#define VO_VIRT_LAYER_1                4 /* virtual layer 1 */
#define VO_VIRT_LAYER_2                5 /* virtual layer 2 */
#define VO_VIRT_LAYER_3                6 /* virtual layer 3 */

#define VO_MIN_CHN_WIDTH               32 /* channel minimal width */
#define VO_MIN_CHN_HEIGHT              32 /* channel minimal height */
#define VO_MAX_ZOOM_RATIO              1000 /* max zoom ratio, 1000 means 100% scale */

#define VO_MAX_PHY_DEV_NUM             1 /* max physical dev num */

#define VO_MAX_DEV_NUM                 (VO_MAX_PHY_DEV_NUM + VO_MAX_VIRT_DEV_NUM) /* max dev num */
#define VO_MAX_LAYER_NUM               (3 + VO_MAX_VIRT_DEV_NUM) /* max layer num */
#define VO_MAX_PRIORITY                3 /* max layer priority */
#define VO_MAX_CHN_NUM                 9 /* max chn num */
#define VO_MAX_LAYER_IN_DEV            2 /* max layer num of each dev */
#define VO_MAX_GRAPHICS_LAYER_NUM      3
#define VO_MAX_WBC_NUM                 1
#define VO_MIN_TOLERATE                1 /* min play toleration 1ms */
#define VO_MAX_TOLERATE                100000 /* max play toleration 100s */

/* For VI */
/* number of channle and device on video input unit of chip
 * Note! VI_MAX_CHN_NUM is NOT equal to VI_MAX_DEV_NUM
 * multiplied by VI_MAX_CHN_NUM, because all VI devices
 * can't work at mode of 4 channles at the same time.
 */
#ifdef CONFIGSC_SCA200V200
  #define VI_MAX_DEV_NUM                 2
  #define VI_MAX_PHY_PIPE_NUM            2
#elif defined(CONFIGSC_SCA200V100)
  #define VI_MAX_DEV_NUM                 4
  #define VI_MAX_PHY_PIPE_NUM            4
#else
  #error "sc chip error"
#endif
#define VI_MAX_VIR_PIPE_NUM            0
#define VI_MAX_PIPE_NUM                (VI_MAX_PHY_PIPE_NUM + VI_MAX_VIR_PIPE_NUM)
#define VI_MAX_STITCH_GRP_NUM          6
#define VI_MAX_WDR_NUM                 2
#define VI_MAX_PHY_CHN_NUM             3
#define VI_MAX_EXT_CHN_NUM             0
#define VI_EXT_CHN_START               VI_MAX_PHY_CHN_NUM
#define VI_MAX_CHN_NUM                 (VI_MAX_PHY_CHN_NUM + VI_MAX_EXT_CHN_NUM)
#define VI_MAX_EXTCHN_BIND_PER_CHN     8

#define VIPROC_IRQ_NUM                 1
#define VI_MAX_WDR_FRAME_NUM           2
#define VI_MAX_NODE_NUM                2
#define VIPROC_IP_NUM                  1
#define VICAP_IP_NUM                   1

#define VI_MAX_SPLIT_NODE_NUM          2

#define VI_DEV_MIN_WIDTH               120
#define VI_DEV_MIN_HEIGHT              120
#define VI_DEV_MAX_WIDTH               8192
#define VI_DEV_MAX_HEIGHT              8192

#define VI_PIPE_OFFLINE_MIN_WIDTH      120
#define VI_PIPE_OFFLINE_MIN_HEIGHT     120
#define VI_PIPE_OFFLINE_MAX_WIDTH      7680
#define VI_PIPE_OFFLINE_MAX_HEIGHT     8192

#define VI_PIPE_ONLINE_MIN_WIDTH       120
#define VI_PIPE_ONLINE_MIN_HEIGHT      120
#define VI_PIPE_ONLINE_MAX_WIDTH       4096
#define VI_PIPE_ONLINE_MAX_HEIGHT      8192

#define VI_PHYCHN_OFFLINE_MIN_WIDTH    120
#define VI_PHYCHN_OFFLINE_MIN_HEIGHT   120
#define VI_PHYCHN_OFFLINE_MAX_WIDTH    7680
#define VI_PHYCHN_OFFLINE_MAX_HEIGHT   8192

#define VI_PHYCHN_ONLINE_MIN_WIDTH     120
#define VI_PHYCHN_ONLINE_MIN_HEIGHT    120
#define VI_PHYCHN_ONLINE_MAX_WIDTH     4096
#define VI_PHYCHN_ONLINE_MAX_HEIGHT    8192

#define VI_EXTCHN_MIN_WIDTH            32
#define VI_EXTCHN_MIN_HEIGHT           32
#define VI_EXTCHN_MAX_WIDTH            8192
#define VI_EXTCHN_MAX_HEIGHT           8192

#define VI_PHY_CHN1_MAX_ZOOMIN         1
#define VI_PHY_CHN1_MAX_ZOOMOUT        30
#define VI_EXT_CHN_MAX_ZOOMIN          16
#define VI_EXT_CHN_MAX_ZOOMOUT         30

#define VI_CMP_PARAM_SIZE              152
#define VI_VPSS_DEFAULT_EARLINE        128

/* For VPSS */
#define VPSS_IP_NUM                    1
#define VPSS0                          0
#define VPSS_MAX_GRP_NUM               32
#define VPSS_MAX_GRP_PIPE_NUM          1
#define VPSS_PARALLEL_PIC_NUM          1
#define VPSS_MAX_PHY_CHN_NUM           4
#define VPSS_LOWDELAY_CHN_NUM          3
#define VPSS_MAX_EXT_CHN_NUM           4
#define VPSS_MAX_CHN_NUM               (VPSS_MAX_PHY_CHN_NUM + VPSS_MAX_EXT_CHN_NUM)
#define VPSS_MIN_IMAGE_WIDTH_SBS       512
#define VPSS_MIN_IMAGE_WIDTH           64
#define VPSS_MIN_IMAGE_HEIGHT          64
#define VPSS_MAX_IMAGE_WIDTH_SLAVE     8192
#define VPSS_MAX_IMAGE_WIDTH           8192
#define VPSS_MAX_IMAGE_HEIGHT          8192
#define VPSS_EXTCHN_MAX_IMAGE_WIDTH    8192
#define VPSS_EXTCHN_MAX_IMAGE_HEIGHT   8192
#define VPSS_MAX_ZOOMIN                16
#define VPSS_MAX_ZOOMOUT               15
#define VPSS_EXT_CHN_MAX_ZOOMIN        16
#define VPSS_EXT_CHN_MAX_ZOOMOUT       30

/* For VGS */
#define VGS_IP_NUM                     1
#define VGS0                           0
#define VGS1                           1
#define VGS_MAX_COVER_NUM              1
#define VGS_MAX_OSD_NUM                1

/* For REGION */
#define RGN_HANDLE_MAX                 1024

#define RGN_EACH_MAX_NUM               16

#define RGN_VGS_TASK_WIDTH_MAX         8192

#define RGN_MIN_WIDTH                  2
#define RGN_MIN_HEIGHT                 2

#define RGN_MIN_LAYER                  0
#define RGN_MAX_LAYER                  (RGN_EACH_MAX_NUM - 1)

#define RGN_RATIO_MIN_X                0
#define RGN_RATIO_MIN_Y                0
#define RGN_RATIO_MAX_X                999
#define RGN_RATIO_MAX_Y                999
#define RGN_RATIO_MIN_WIDTH            1
#define RGN_RATIO_MIN_HEIGHT           1
#define RGN_RATIO_MAX_WIDTH            1000
#define RGN_RATIO_MAX_HEIGHT           1000

#define RGN_COVER_MIN_X                0
#define RGN_COVER_MIN_Y                0
#define RGN_COVER_MAX_X                8190
#define RGN_COVER_MAX_Y                8190
#define RGN_COVER_MAX_WIDTH            8192
#define RGN_COVER_MAX_HEIGHT           8192

#define RGN_COVEREX_MIN_X              0
#define RGN_COVEREX_MIN_Y              0
#define RGN_COVEREX_MAX_X              8190
#define RGN_COVEREX_MAX_Y              8190
#define RGN_COVEREX_MAX_WIDTH          8192
#define RGN_COVEREX_MAX_HEIGHT         8192

#define RGN_OVERLAY_MIN_X              0
#define RGN_OVERLAY_MIN_Y              0
#define RGN_OVERLAY_MAX_X              8184
#define RGN_OVERLAY_MAX_Y              8190
#define RGN_OVERLAY_MIN_WIDTH          8
#define RGN_OVERLAY_MIN_HEIGHT         2
#define RGN_OVERLAY_MAX_WIDTH          8192
#define RGN_OVERLAY_MAX_HEIGHT         8192
#define RGN_OVERLAY_W_ALIGN            8

#define RGN_OVERLAYEX_MIN_X            0
#define RGN_OVERLAYEX_MIN_Y            0
#define RGN_OVERLAYEX_MAX_X            8190
#define RGN_OVERLAYEX_MAX_Y            8190
#define RGN_OVERLAYEX_MAX_WIDTH        8192
#define RGN_OVERLAYEX_MAX_HEIGHT       8192

#define RGN_MOSAIC_X_ALIGN             4
#define RGN_MOSAIC_Y_ALIGN             2
#define RGN_MOSAIC_WIDTH_ALIGN         4
#define RGN_MOSAIC_HEIGHT_ALIGN        4

#define RGN_MOSAIC_MIN_X               0
#define RGN_MOSAIC_MIN_Y               0
#define RGN_MOSAIC_MAX_X               8188
#define RGN_MOSAIC_MAX_Y               8190
#define RGN_MOSAIC_MIN_WIDTH           32
#define RGN_MOSAIC_MIN_HEIGHT          32
#define RGN_MOSAIC_MAX_WIDTH           8192
#define RGN_MOSAIC_MAX_HEIGHT          8192

#define RGN_ALIGN                      2

#define RGN_MAX_BUF_NUM                1

/* For AUDIO */
#define ADEC_MAX_CHN_NUM               8
#define AENC_MAX_CHN_NUM               8
#define AI_MAX_CHN_NUM 8
#define AO_MAX_CHN_NUM 8
#define AI_DEV_MAX_NUM 4
#define AO_DEV_MAX_NUM 4

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* _SC_DEFINES_H_ */

