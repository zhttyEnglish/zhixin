
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "sample_comm.h"
#include "sc_mipi_tx.h"

#define SAMPLE_VO_DEF_VALUE (-1)
#define SAMPLE_VO_USE_DEFAULT_MIPI_TX 1

/*******************************
* GLOBAL vars for mipi_tx
*******************************/
typedef struct
{
             int    delay;
    unsigned char   addr;
    unsigned char   num;
    unsigned char   data[128];
} MIPI_INITPARAM_S;


combo_dev_cfg_t MIPI_TX_720X576_50_CONFIG =
{
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .video_mode =  BURST_MODE,
    .sync_info = {
        .vid_pkt_size     = 720,
        .vid_hsa_pixels   = 64,
        .vid_hbp_pixels   = 68,
        .vid_hline_pixels = 864,
        .vid_vsa_lines    = 5,
        .vid_vbp_lines    = 39,
        .vid_vfp_lines    = 5,
        .vid_active_lines = 576,
        .edpi_cmd_size    = 0,
    },
    .phy_data_rate = 459,
    .pixel_clk = 27000,
};

combo_dev_cfg_t MIPI_TX_1280X720_60_CONFIG =
{
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .video_mode =  BURST_MODE,
    .sync_info = {
        .vid_pkt_size     = 1280,
        .vid_hsa_pixels   = 40,
        .vid_hbp_pixels   = 220,
        .vid_hline_pixels = 1650,
        .vid_vsa_lines    = 5,
        .vid_vbp_lines    = 20,
        .vid_vfp_lines    = 5,
        .vid_active_lines = 720,
        .edpi_cmd_size    = 0,
    },
    .phy_data_rate = 459,
    .pixel_clk = 74250,
};

combo_dev_cfg_t MIPI_TX_1024X768_60_CONFIG =
{
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .video_mode =  BURST_MODE,
    .sync_info = {
        .vid_pkt_size     = 1024,
        .vid_hsa_pixels   = 136,
        .vid_hbp_pixels   = 160,
        .vid_hline_pixels = 1344,
        .vid_vsa_lines    = 6,
        .vid_vbp_lines    = 29,
        .vid_vfp_lines    = 3,
        .vid_active_lines = 768,
        .edpi_cmd_size    = 0,
    },
    .phy_data_rate = 495,//486
    .pixel_clk = 65000,
};

combo_dev_cfg_t MIPI_TX_1280x1024_60_CONFIG =
{
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .video_mode =  BURST_MODE,
    .sync_info = {
        .vid_pkt_size     = 1280,
        .vid_hsa_pixels   = 112,
        .vid_hbp_pixels   = 248,
        .vid_hline_pixels = 1688,
        .vid_vsa_lines    = 3,
        .vid_vbp_lines    = 38,
        .vid_vfp_lines    = 1,
        .vid_active_lines = 1024,
        .edpi_cmd_size    = 0,
    },
    .phy_data_rate = 495,//486
    .pixel_clk = 108000,
};

combo_dev_cfg_t MIPI_TX_1920X1080_60_CONFIG =
{
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .video_mode =  BURST_MODE,
    .sync_info = {
        .vid_pkt_size = 1920,
        .vid_hsa_pixels = 44,
        .vid_hbp_pixels = 148,
        .vid_hline_pixels = 2200,
        .vid_vsa_lines = 5,
        .vid_vbp_lines = 36,
        .vid_vfp_lines = 4,
        .vid_active_lines = 1080,
        .edpi_cmd_size = 0,
    },
    .phy_data_rate = 945,
    .pixel_clk = 148500,
};

combo_dev_cfg_t MIPI_TX_720X1280_60_CONFIG =
{
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .video_mode =  BURST_MODE,
    .sync_info = {
        .vid_pkt_size     = 720, // hact
        .vid_hsa_pixels   = 24,  // hsa
        .vid_hbp_pixels   = 99,  // hbp
        .vid_hline_pixels = 943, // hact + hsa + hbp + hfp
        .vid_vsa_lines    = 4,   // vsa
        .vid_vbp_lines    = 20,  // vbp
        .vid_vfp_lines    = 8,   // vfp
        .vid_active_lines = 1280,// vact
        .edpi_cmd_size    = 0,
    },
    .phy_data_rate = 459,
    .pixel_clk = 74250,
};

combo_dev_cfg_t MIPI_TX_1080X1920_60_CONFIG =
{
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .video_mode =  BURST_MODE,
    .sync_info = {
        .vid_pkt_size     = 1080,
        .vid_hsa_pixels   = 8,
        .vid_hbp_pixels   = 20,
        .vid_hline_pixels = 1238,
        .vid_vsa_lines    = 10,
        .vid_vbp_lines    = 26,
        .vid_vfp_lines    = 16,
        .vid_active_lines = 1920,
        .edpi_cmd_size = 0,
    },
    .phy_data_rate = 945,
    .pixel_clk = 148500,
};

SC_S32 SAMPLE_COMM_VO_GetWH(VO_INTF_SYNC_E enIntfSync, SC_U32 *pu32W, SC_U32 *pu32H, SC_U32 *pu32Frm)
{
    switch (enIntfSync)
    {
    case VO_OUTPUT_PAL       :
        *pu32W = 720;
        *pu32H = 576;
        *pu32Frm = 25;
        break;
    case VO_OUTPUT_NTSC      :
        *pu32W = 720;
        *pu32H = 480;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_1080P24   :
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 24;
        break;
    case VO_OUTPUT_1080P25   :
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 25;
        break;
    case VO_OUTPUT_1080P30   :
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_720P50    :
        *pu32W = 1280;
        *pu32H = 720;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_720P60    :
        *pu32W = 1280;
        *pu32H = 720;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1080I50   :
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_1080I60   :
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1080P50   :
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_1080P60   :
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_576P50    :
        *pu32W = 720;
        *pu32H = 576;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_480P60    :
        *pu32W = 720;
        *pu32H = 480;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_800x600_60:
        *pu32W = 800;
        *pu32H = 600;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1024x768_60:
        *pu32W = 1024;
        *pu32H = 768;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1280x1024_60:
        *pu32W = 1280;
        *pu32H = 1024;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1366x768_60:
        *pu32W = 1366;
        *pu32H = 768;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1440x900_60:
        *pu32W = 1440;
        *pu32H = 900;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1280x800_60:
        *pu32W = 1280;
        *pu32H = 800;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1600x1200_60:
        *pu32W = 1600;
        *pu32H = 1200;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1680x1050_60:
        *pu32W = 1680;
        *pu32H = 1050;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1920x1200_60:
        *pu32W = 1920;
        *pu32H = 1200;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_640x480_60:
        *pu32W = 640;
        *pu32H = 480;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_960H_PAL:
        *pu32W = 960;
        *pu32H = 576;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_960H_NTSC:
        *pu32W = 960;
        *pu32H = 480;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1920x2160_30:
        *pu32W = 1920;
        *pu32H = 2160;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_2560x1440_30:
        *pu32W = 2560;
        *pu32H = 1440;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_2560x1600_60:
        *pu32W = 2560;
        *pu32H = 1600;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_3840x2160_30    :
        *pu32W = 3840;
        *pu32H = 2160;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_3840x2160_60    :
        *pu32W = 3840;
        *pu32H = 2160;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_320x240_60    :
        *pu32W = 320;
        *pu32H = 240;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_320x240_50    :
        *pu32W = 320;
        *pu32H = 240;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_240x320_50    :
        *pu32W = 240;
        *pu32H = 320;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_240x320_60    :
        *pu32W = 240;
        *pu32H = 320;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_800x600_50    :
        *pu32W = 800;
        *pu32H = 600;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_720x1280_60    :
        *pu32W = 720;
        *pu32H = 1280;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1080x1920_60    :
        *pu32W = 1080;
        *pu32H = 1920;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_7680x4320_30    :
        *pu32W = 7680;
        *pu32H = 4320;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_USER    :
        *pu32W = 720;
        *pu32H = 576;
        *pu32Frm = 25;
        break;
    default:
        SAMPLE_PRT("vo enIntfSync %d not support!\n", enIntfSync);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VO_StartDev(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SC_MPI_VO_SetPubAttr(VoDev, pstPubAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VO_Enable(VoDev);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VO_StopDev(VO_DEV VoDev)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SC_MPI_VO_Disable(VoDev);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VO_StartLayer(VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SC_MPI_VO_SetVideoLayerAttr(VoLayer, pstLayerAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VO_EnableVideoLayer(VoLayer);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VO_StopLayer(VO_LAYER VoLayer)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SC_MPI_VO_DisableVideoLayer(VoLayer);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_VO_StartChn(VO_LAYER VoLayer, SAMPLE_VO_MODE_E enMode)
{
    SC_S32 i;
    SC_S32 s32Ret    = SC_SUCCESS;
    SC_U32 u32WndNum = 0;
    SC_U32 u32Square = 0;
    SC_U32 u32Row    = 0;
    SC_U32 u32Col    = 0;
    SC_U32 u32Width  = 0;
    SC_U32 u32Height = 0;
    VO_CHN_ATTR_S         stChnAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;

    switch (enMode)
    {
    case VO_MODE_1MUX:
        u32WndNum = 1;
        u32Square = 1;
        break;
    case VO_MODE_2MUX:
        u32WndNum = 2;
        u32Square = 2;
        break;
    case VO_MODE_4MUX:
        u32WndNum = 4;
        u32Square = 2;
        break;
    case VO_MODE_8MUX:
        u32WndNum = 8;
        u32Square = 3;
        break;
    case VO_MODE_9MUX:
        u32WndNum = 9;
        u32Square = 3;
        break;
    case VO_MODE_16MUX:
        u32WndNum = 16;
        u32Square = 4;
        break;
    case VO_MODE_25MUX:
        u32WndNum = 25;
        u32Square = 5;
        break;
    case VO_MODE_36MUX:
        u32WndNum = 36;
        u32Square = 6;
        break;
    case VO_MODE_49MUX:
        u32WndNum = 49;
        u32Square = 7;
        break;
    case VO_MODE_64MUX:
        u32WndNum = 64;
        u32Square = 8;
        break;
    case VO_MODE_2X4:
        u32WndNum = 8;
        u32Square = 3;
        u32Row    = 4;
        u32Col    = 2;
        break;
    default:
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VO_GetVideoLayerAttr(VoLayer, &stLayerAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    u32Width  = stLayerAttr.stImageSize.u32Width;
    u32Height = stLayerAttr.stImageSize.u32Height;
    SAMPLE_PRT("u32Width:%d, u32Height:%d, u32Square:%d\n", u32Width, u32Height, u32Square);
    for (i = 0; i < u32WndNum; i++)
    {
        if( enMode == VO_MODE_1MUX  ||
            enMode == VO_MODE_2MUX  ||
            enMode == VO_MODE_4MUX  ||
            enMode == VO_MODE_8MUX  ||
            enMode == VO_MODE_9MUX  ||
            enMode == VO_MODE_16MUX ||
            enMode == VO_MODE_25MUX ||
            enMode == VO_MODE_36MUX ||
            enMode == VO_MODE_49MUX ||
            enMode == VO_MODE_64MUX )
        {
            stChnAttr.stRect.s32X       = ALIGN_DOWN((u32Width / u32Square) * (i % u32Square), 2);
            stChnAttr.stRect.s32Y       = ALIGN_DOWN((u32Height / u32Square) * (i / u32Square), 2);
            stChnAttr.stRect.u32Width   = ALIGN_DOWN(u32Width / u32Square, 2);
            stChnAttr.stRect.u32Height  = ALIGN_DOWN(u32Height / u32Square, 2);
            stChnAttr.u32Priority       = 0;
            stChnAttr.bDeflicker        = SC_FALSE;
        }
        else if(enMode == VO_MODE_2X4)
        {
            stChnAttr.stRect.s32X       = ALIGN_DOWN((u32Width / u32Col) * (i % u32Col), 2);
            stChnAttr.stRect.s32Y       = ALIGN_DOWN((u32Height / u32Row) * (i / u32Col), 2);
            stChnAttr.stRect.u32Width   = ALIGN_DOWN(u32Width / u32Col, 2);
            stChnAttr.stRect.u32Height  = ALIGN_DOWN(u32Height / u32Row, 2);
            stChnAttr.u32Priority       = 0;
            stChnAttr.bDeflicker        = SC_FALSE;
        }

        s32Ret = SC_MPI_VO_SetChnAttr(VoLayer, i, &stChnAttr);
        if (s32Ret != SC_SUCCESS)
        {
            printf("%s(%d):failed with %#x!\n", \
                __FUNCTION__, __LINE__,  s32Ret);
            return SC_FAILURE;
        }

        s32Ret = SC_MPI_VO_EnableChn(VoLayer, i);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("failed with %#x!\n", s32Ret);
            return SC_FAILURE;
        }
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VO_StopChn(VO_LAYER VoLayer, SAMPLE_VO_MODE_E enMode)
{
    SC_S32 i;
    SC_S32 s32Ret    = SC_SUCCESS;
    SC_U32 u32WndNum = 0;

    switch (enMode)
    {
    case VO_MODE_1MUX:
    {
        u32WndNum = 1;
        break;
    }
    case VO_MODE_2MUX:
    {
        u32WndNum = 2;
        break;
    }
    case VO_MODE_4MUX:
    {
        u32WndNum = 4;
        break;
    }
    case VO_MODE_8MUX:
    {
        u32WndNum = 8;
        break;
    }
    default:
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    for (i = 0; i < u32WndNum; i++)
    {
        s32Ret = SC_MPI_VO_ClearChnBuf(VoLayer, i, SC_FALSE);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SC_MPI_VO_ClearChnBuf failed with %#x!\n", s32Ret);
        }

        s32Ret = SC_MPI_VO_DisableChn(VoLayer, i);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SC_MPI_VO_DisableChn failed with %#x!\n", s32Ret);
            return SC_FAILURE;
        }
    }

    return s32Ret;
}

static SC_VOID SAMPLE_PRIVATE_VO_GetDefWbcAttr(VO_WBC_ATTR_S *pstWbcAttr, VO_WBC_SOURCE_S *pstSource)
{
    if (NULL == pstWbcAttr)
    {
        printf("pstWbcAttr is NULL !!, Line:%d,Func:%s\n", __LINE__, __FUNCTION__);
        return;
    }
    pstWbcAttr->stTargetSize.u32Width   = 1920;
    pstWbcAttr->stTargetSize.u32Height  = 1080;
    pstWbcAttr->enPixelFormat           = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pstWbcAttr->u32FrameRate            = 30;
    pstWbcAttr->enCompressMode          = COMPRESS_MODE_NONE;
    pstWbcAttr->enDynamicRange          = DYNAMIC_RANGE_SDR8;
    pstSource->enSourceType             = VO_WBC_SOURCE_DEV;
    pstSource->u32SourceId              = SAMPLE_VO_DEV_DHD0;
    return;
}

static SC_S32 SAMPLE_PRIVATE_VO_StartWbc(VO_WBC VoWbc, VO_WBC_ATTR_S *pstWbcAttr, VO_WBC_SOURCE_S *pstSource)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SC_MPI_VO_SetWBCSource(VoWbc, pstSource);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    s32Ret = SC_MPI_VO_SetWBCAttr(VoWbc, pstWbcAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    s32Ret = SC_MPI_VO_EnableWBC(VoWbc);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

static SC_S32 SAMPLE_PRIVATE_VO_StopWbc(VO_WBC VoWbc)
{
    SC_S32 s32Ret = SC_SUCCESS;

    s32Ret = SC_MPI_VO_DisableWBC(VoWbc);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }
    return s32Ret;
}

SC_S32 SAMPLE_COMM_VO_StartWBC(SAMPLE_VO_WBC_CONFIG *pWbcConfig)
{
    SC_S32 s32Ret = SC_SUCCESS;

    /******************************
    *set dynaminc range if changed.
    *******************************/
    if(-1 != pWbcConfig->enDynamicRange)
    {
        pWbcConfig->stWbcAttr.enDynamicRange = pWbcConfig->enDynamicRange;
    }
    /******************************
    * set compress mode if changed.
    *******************************/
    if(-1 != pWbcConfig->enCompressMode)
    {
        pWbcConfig->stWbcAttr.enCompressMode = pWbcConfig->enCompressMode;
    }
    if (-1 < pWbcConfig->s32Depth)
    {
        s32Ret = SC_MPI_VO_SetWBCDepth(pWbcConfig->VoWbc, 5);
        if (SC_SUCCESS != s32Ret)
        {
            printf("Dev(%d) SC_MPI_VO_SetWbcDepth errno %#x\n", pWbcConfig->VoWbc, s32Ret);
            return s32Ret;
        }
    }
    pWbcConfig->stWbcSource.enSourceType = pWbcConfig->enSourceType;
    s32Ret = SAMPLE_PRIVATE_VO_StartWbc(pWbcConfig->VoWbc,
            &pWbcConfig->stWbcAttr,
            &pWbcConfig->stWbcSource);
    return s32Ret;
}

SC_S32 SAMPLE_COMM_VO_StopWBC(SAMPLE_VO_WBC_CONFIG *pWbcConfig)
{
    return SAMPLE_PRIVATE_VO_StopWbc(pWbcConfig->VoWbc);
}

SC_S32 SAMPLE_COMM_VO_GetDefWBCConfig(SAMPLE_VO_WBC_CONFIG *pWbcConfig)
{
    pWbcConfig->VoWbc                    = 0;
    pWbcConfig->enSourceType             = VO_WBC_SOURCE_DEV;
    pWbcConfig->stWbcSource.enSourceType = VO_WBC_SOURCE_DEV;
    pWbcConfig->stWbcSource.u32SourceId  = 0;
    pWbcConfig->enDynamicRange           = SAMPLE_VO_DEF_VALUE;
    pWbcConfig->enCompressMode           = SAMPLE_VO_DEF_VALUE;
    pWbcConfig->s32Depth                 = SAMPLE_VO_DEF_VALUE;

    SAMPLE_PRIVATE_VO_GetDefWbcAttr(&pWbcConfig->stWbcAttr,
        &pWbcConfig->stWbcSource);

    return SC_SUCCESS;
}

void SAMPLE_COMM_VO_StartMipiTx(VO_INTF_SYNC_E enVoIntfSync);

/*
* Name : SAMPLE_COMM_VO_GetDefConfig
* Desc : An instance of SAMPLE_VO_CONFIG_S, which allows you to use vo immediately.
*/
SC_S32 SAMPLE_COMM_VO_GetDefConfig(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    RECT_S  stDefDispRect  = {0, 0, 1920, 1080};
    SIZE_S  stDefImageSize = {1920, 1080};
    if(NULL == pstVoConfig)
    {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return SC_FAILURE;
    }

    pstVoConfig->VoDev             = SAMPLE_VO_DEV_UHD;
    #ifdef SC_FPGA
    pstVoConfig->enVoIntfType = VO_INTF_BT1120;
    #else
    pstVoConfig->enVoIntfType = VO_INTF_HDMI;
    #endif
    pstVoConfig->enIntfSync        = VO_OUTPUT_1080P60;
    pstVoConfig->u32BgColor        = COLOR_RGB_BLUE;
    pstVoConfig->enPixFormat       = PIXEL_FORMAT_YVU_PLANAR_420;
    pstVoConfig->stDispRect        = stDefDispRect;
    pstVoConfig->stImageSize       = stDefImageSize;
    pstVoConfig->enVoPartMode      = VO_PART_MODE_SINGLE;
    pstVoConfig->u32DisBufLen      = 3;
    pstVoConfig->enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    pstVoConfig->enVoMode          = VO_MODE_1MUX;

    return SC_SUCCESS;
}
SC_S32 SAMPLE_COMM_VO_StartVO(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    RECT_S                 stDefDispRect  = {0, 0, 1920, 1080};
    SIZE_S                 stDefImageSize = {1920, 1080};

    /*******************************************
    * VO device VoDev# information declaration.
    ********************************************/
    VO_DEV                 VoDev          = 0;
    VO_LAYER               VoLayer        = 0;
    SAMPLE_VO_MODE_E       enVoMode       = 0;
    VO_INTF_TYPE_E         enVoIntfType   = VO_INTF_HDMI;
    VO_INTF_SYNC_E         enIntfSync     = VO_OUTPUT_1080P60;
    VO_PART_MODE_E         enVoPartMode   = VO_PART_MODE_SINGLE;
    VO_PUB_ATTR_S          stVoPubAttr    = {0};
    VO_VIDEO_LAYER_ATTR_S  stLayerAttr    = {0};
    VO_CSC_S               stVideoCSC     = {0};
    SC_S32                 s32Ret         = SC_SUCCESS;

    if (NULL == pstVoConfig)
    {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return SC_FAILURE;
    }
    VoDev          = pstVoConfig->VoDev;
    VoLayer        = pstVoConfig->VoDev;
    enVoMode       = pstVoConfig->enVoMode;
    enVoIntfType   = pstVoConfig->enVoIntfType;
    enIntfSync     = pstVoConfig->enIntfSync;
    enVoPartMode   = pstVoConfig->enVoPartMode;

    /********************************
    * Set and start VO device VoDev#.
    *********************************/
    stVoPubAttr.enIntfType  = enVoIntfType;
    stVoPubAttr.enIntfSync  = enIntfSync;

    stVoPubAttr.u32BgColor  = pstVoConfig->u32BgColor;

    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartDev failed!\n");
        return s32Ret;
    }

    /******************************
    * Set and start layer VoDev#.
    ********************************/

    s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync,
            &stLayerAttr.stDispRect.u32Width, &stLayerAttr.stDispRect.u32Height,
            &stLayerAttr.u32DispFrmRt);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_GetWH failed!\n");
        SAMPLE_COMM_VO_StopDev(VoDev);
        return s32Ret;
    }
    stLayerAttr.bClusterMode     = SC_FALSE;
    stLayerAttr.bDoubleFrame    = SC_FALSE;
    stLayerAttr.enPixFormat       = pstVoConfig->enPixFormat;

    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    stLayerAttr.u32DispFrmRt = 30;
    /******************************
    // Set display rectangle if changed.
    ********************************/
    if (0 != memcmp(&pstVoConfig->stDispRect, &stDefDispRect, sizeof(RECT_S)))
    {
        stLayerAttr.stDispRect = pstVoConfig->stDispRect;
    }
    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;

    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;

    /******************************
    //Set image size if changed.
    ********************************/
    if (0 != memcmp(&pstVoConfig->stImageSize, &stDefImageSize, sizeof(SIZE_S)))
    {
        stLayerAttr.stImageSize = pstVoConfig->stImageSize;
    }
    stLayerAttr.enDstDynamicRange     = pstVoConfig->enDstDynamicRange;

    if (pstVoConfig->u32DisBufLen)
    {
        s32Ret = SC_MPI_VO_SetDisplayBufLen(VoLayer, pstVoConfig->u32DisBufLen);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetDisplayBufLen failed with %#x!\n", s32Ret);
            SAMPLE_COMM_VO_StopDev(VoDev);
            return s32Ret;
        }
    }
    if (VO_PART_MODE_MULTI == enVoPartMode)
    {
        s32Ret = SC_MPI_VO_SetVideoLayerPartitionMode(VoLayer, enVoPartMode);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetVideoLayerPartitionMode failed!\n");
            SAMPLE_COMM_VO_StopDev(VoDev);
            return s32Ret;
        }
    }

    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_Start video layer failed!\n");
        SAMPLE_COMM_VO_StopDev(VoDev);
        return s32Ret;
    }

    if(VO_INTF_MIPI == enVoIntfType)
    {
        s32Ret = SC_MPI_VO_GetVideoLayerCSC(VoLayer, &stVideoCSC);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_GetVideoLayerCSC failed!\n");
            SAMPLE_COMM_VO_StopDev(VoDev);
            return s32Ret;
        }
        stVideoCSC.enCscMatrix = VO_CSC_MATRIX_BT709_TO_RGB_PC;
        s32Ret = SC_MPI_VO_SetVideoLayerCSC(VoLayer, &stVideoCSC);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetVideoLayerCSC failed!\n");
            SAMPLE_COMM_VO_StopDev(VoDev);
            return s32Ret;
        }
    }

    /******************************
    * start vo channels.
    ********************************/
    s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, enVoMode);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
        SAMPLE_COMM_VO_StopLayer(VoLayer);
        SAMPLE_COMM_VO_StopDev(VoDev);
        return s32Ret;
    }

    /******************************
    * Start hdmi device.
    * Note : do this after vo device started.
    ********************************/
    if(VO_INTF_HDMI & enVoIntfType)
    {
        //todo
        //SAMPLE_COMM_VO_HdmiStartByDyRg(enIntfSync, enDstDyRg);
    }

    /******************************
    * Start mipi_tx device.
    * Note : do this after vo device started.
    ********************************/
    if(VO_INTF_MIPI & enVoIntfType)
    {
        SAMPLE_COMM_VO_StartMipiTx(enIntfSync);
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VO_StartPIP(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    /*******************************************
    * VO device VoDev# information declaration.
    ********************************************/
    VO_DEV                 VoDev          = 0;
    VO_LAYER               VoLayer        = 0;
    SAMPLE_VO_MODE_E       enVoMode       = 0;
    VO_INTF_SYNC_E         enIntfSync     = VO_OUTPUT_1080P30;
    VO_PART_MODE_E         enVoPartMode   = VO_PART_MODE_SINGLE;
    VO_PUB_ATTR_S          stVoPubAttr    = {0};
    VO_VIDEO_LAYER_ATTR_S  stLayerAttr    = {0};
    SC_S32                 s32Ret         = SC_SUCCESS;

    if (NULL == pstVoConfig)
    {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return SC_FAILURE;
    }
    VoDev          = pstVoConfig->VoDev;
    VoLayer        = 2;
    enVoMode       = pstVoConfig->enVoMode;
    enIntfSync     = pstVoConfig->enIntfSync;
    enVoPartMode   = pstVoConfig->enVoPartMode;

    stVoPubAttr.enIntfSync  = enIntfSync;
    /******************************
    * Set and start layer VoDev#.
    ********************************/
    s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync,
            &stLayerAttr.stDispRect.u32Width, &stLayerAttr.stDispRect.u32Height,
            &stLayerAttr.u32DispFrmRt);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_GetWH failed!\n");
        SAMPLE_COMM_VO_StopDev(VoDev);
        return s32Ret;
    }
    stLayerAttr.bClusterMode     = SC_FALSE;
    stLayerAttr.bDoubleFrame    = SC_FALSE;
    stLayerAttr.enPixFormat       = pstVoConfig->enPixFormat;

    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;

    /******************************
    // Set display rectangle if changed.
    ********************************/
    stLayerAttr.stDispRect.u32Width /= 4;
    stLayerAttr.stDispRect.u32Height /= 4;

    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;

    /******************************
    //Set image size if changed.
    ********************************/
    stLayerAttr.enDstDynamicRange     = pstVoConfig->enDstDynamicRange;

    if (pstVoConfig->u32DisBufLen)
    {
        s32Ret = SC_MPI_VO_SetDisplayBufLen(VoLayer, pstVoConfig->u32DisBufLen);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetDisplayBufLen failed with %#x!\n", s32Ret);
            SAMPLE_COMM_VO_StopDev(VoDev);
            return s32Ret;
        }
    }
    if (VO_PART_MODE_MULTI == enVoPartMode)
    {
        s32Ret = SC_MPI_VO_SetVideoLayerPartitionMode(VoLayer, enVoPartMode);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetVideoLayerPartitionMode failed!\n");
            SAMPLE_COMM_VO_StopDev(VoDev);
            return s32Ret;
        }
    }

    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_Start video layer failed!\n");
        SAMPLE_COMM_VO_StopDev(VoDev);
        return s32Ret;
    }

    /******************************
    * start vo channels.
    ********************************/
    s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, enVoMode);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
        SAMPLE_COMM_VO_StopLayer(VoLayer);
        SAMPLE_COMM_VO_StopDev(VoDev);
        return s32Ret;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VO_StopVO(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    VO_DEV                VoDev     = 0;
    VO_LAYER              VoLayer   = 0;
    SAMPLE_VO_MODE_E      enVoMode  = VO_MODE_BUTT;

    if (NULL == pstVoConfig)
    {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return SC_FAILURE;
    }

    VoDev     = pstVoConfig->VoDev;
    VoLayer   = pstVoConfig->VoDev;
    enVoMode  = pstVoConfig->enVoMode;

    if(VO_INTF_HDMI & pstVoConfig->enVoIntfType)
    {
        //todo
        //SAMPLE_COMM_VO_HdmiStop();
    }
    SAMPLE_COMM_VO_StopChn(VoLayer, enVoMode);
    SAMPLE_COMM_VO_StopLayer(VoLayer);
    SAMPLE_COMM_VO_StopDev(VoDev);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VO_StopPIP(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    VO_LAYER              VoLayer   = 0;
    SAMPLE_VO_MODE_E      enVoMode  = VO_MODE_BUTT;

    if (NULL == pstVoConfig)
    {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return SC_FAILURE;
    }

    VoLayer   = 2;
    enVoMode  = pstVoConfig->enVoMode;

    SAMPLE_COMM_VO_StopChn(VoLayer, enVoMode);
    SAMPLE_COMM_VO_StopLayer(VoLayer);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VO_GetDefLayerConfig(SAMPLE_COMM_VO_LAYER_CONFIG_S *pstVoLayerConfig)
{

    RECT_S  stDefDispRect  = {0, 0, 1920, 1080};
    SIZE_S  stDefImageSize = {1920, 1080};

    if (NULL == pstVoLayerConfig)
    {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return SC_FAILURE;
    }
    pstVoLayerConfig->VoLayer        = SAMPLE_VO_LAYER_VHD2;
    pstVoLayerConfig->enIntfSync     = VO_OUTPUT_1080P30;
    pstVoLayerConfig->stDispRect     = stDefDispRect;
    pstVoLayerConfig->stImageSize    = stDefImageSize;
    pstVoLayerConfig->enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pstVoLayerConfig->u32DisBufLen   = 3;
    pstVoLayerConfig->enVoMode       = VO_MODE_1MUX;

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VO_StartLayerChn(SAMPLE_COMM_VO_LAYER_CONFIG_S *pstVoLayerConfig)
{
    RECT_S                  stDefDispRect     = {0, 0, 1920, 1080};
    SIZE_S                  stDefImageSize    = {1920, 1080};
    VO_LAYER                VoLayer           = 0;
    VO_INTF_SYNC_E          enIntfSync        = VO_OUTPUT_1080P30;
    SAMPLE_VO_MODE_E        enVoMode          = VO_MODE_BUTT;
    SC_S32                  s32Ret            = SC_SUCCESS;
    VO_VIDEO_LAYER_ATTR_S   stLayerAttr;
    SC_U32                  u32Frmt, u32Width, u32Height;

    if (NULL == pstVoLayerConfig)
    {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return SC_FAILURE;
    }

    VoLayer           = pstVoLayerConfig->VoLayer;
    enIntfSync        = pstVoLayerConfig->enIntfSync;
    enVoMode          = pstVoLayerConfig->enVoMode;

    /******************************
    * start vo layer.
    ********************************/
    s32Ret = SAMPLE_COMM_VO_GetWH(enIntfSync, &u32Width, &u32Height, &u32Frmt);
    if (SC_SUCCESS != s32Ret)
    {
        printf("Can not get synchronization information!\n");
        return SC_FAILURE;
    }

    stLayerAttr.stDispRect.s32X       = 0;
    stLayerAttr.stDispRect.s32Y       = 0;
    stLayerAttr.stDispRect.u32Width   = u32Width;
    stLayerAttr.stDispRect.u32Height  = u32Height;
    stLayerAttr.stImageSize.u32Width  = u32Width;
    stLayerAttr.stImageSize.u32Height = u32Height;
    stLayerAttr.u32DispFrmRt          = u32Frmt;
    stLayerAttr.bDoubleFrame          = SC_FALSE;
    stLayerAttr.bClusterMode          = SC_FALSE;
    stLayerAttr.enDstDynamicRange     = DYNAMIC_RANGE_SDR8;

    stLayerAttr.enPixFormat           = pstVoLayerConfig->enPixFormat;

    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    /******************************
    // Set display rectangle if changed.
    ********************************/
    if (0 != memcmp(&pstVoLayerConfig->stDispRect, &stDefDispRect, sizeof(RECT_S)))
    {
        stLayerAttr.stDispRect = pstVoLayerConfig->stDispRect;
    }
    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;

    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;

    /******************************
    //Set image size if changed.
    ********************************/
    if (0 != memcmp(&pstVoLayerConfig->stImageSize, &stDefImageSize, sizeof(SIZE_S)))
    {
        stLayerAttr.stImageSize = pstVoLayerConfig->stImageSize;
    }
    stLayerAttr.enDstDynamicRange     = pstVoLayerConfig->enDstDynamicRange;

    if (pstVoLayerConfig->u32DisBufLen)
    {
        s32Ret = SC_MPI_VO_SetDisplayBufLen(VoLayer, pstVoLayerConfig->u32DisBufLen);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetDisplayBufLen failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }

    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_Start video layer failed!\n");
        return s32Ret;
    }

    /******************************
    * start vo channels.
    ********************************/
    s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, enVoMode);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
        SAMPLE_COMM_VO_StopLayer(VoLayer);
        return s32Ret;
    }
    return s32Ret;
}

SC_S32 SAMPLE_COMM_VO_StopLayerChn(SAMPLE_COMM_VO_LAYER_CONFIG_S *pstVoLayerConfig)
{
    VO_LAYER              VoLayer   = 0;
    SAMPLE_VO_MODE_E      enVoMode  = VO_MODE_BUTT;

    if (NULL == pstVoLayerConfig)
    {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return SC_FAILURE;
    }

    VoLayer   = pstVoLayerConfig->VoLayer;
    enVoMode  = pstVoLayerConfig->enVoMode;

    SAMPLE_COMM_VO_StopChn(VoLayer, enVoMode);
    SAMPLE_COMM_VO_StopLayer(VoLayer);

    return SC_SUCCESS;
}

SC_VOID SAMPLE_COMM_VO_Exit()
{
    SC_MPI_VO_Exit();
}

//0--hdmi, 1--mipi + rgb
SC_S32 SAMPLE_COMM_VO_MIPIRGB_HDMI_720x1280(int type)
{
    SC_S32                      i, Ret;
    VO_CHN                      VoChn           = 0;
    SC_S32                      VoDev           = 0;
    SC_S32                      VoLayer         = 0;
    SC_S32                      VoChnNum        = 1;
    VO_PUB_ATTR_S               stPubAttr = {0};
    VO_VIDEO_LAYER_ATTR_S       stLayerAttr;
    VO_USER_INTFSYNC_INFO_S     stUserInfo;
    SC_U32                      u32Framerate;
    SIZE_S                      stDevSize;
    VO_CHN_ATTR_S               astChnAttr[VO_MAX_CHN_NUM];
    VO_INTF_TYPE_E              enVoIntfType    = VO_INTF_HDMI;


    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserPubBaseAttr(&stPubAttr);

    if (!type)
    {
        stPubAttr.u32BgColor = 0x808080;
        stPubAttr.enIntfType = VO_INTF_HDMI;
        stPubAttr.enIntfSync = VO_OUTPUT_1080P60;

        Ret = SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr);
        if (Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetPubAttr->Ret:0x%x fail\n", Ret);
            return Ret;
        }

        Ret = SC_MPI_VO_GetPubAttr(VoDev, &stPubAttr);
        if (Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_GetPubAttr->Ret:0x%x fail\n", Ret);
            return Ret;
        }

        stPubAttr.stSyncInfo.u16Hact = 1920;
        stPubAttr.stSyncInfo.u16Vact = 1080;
    }
    else
    {
        stPubAttr.enIntfType = VO_INTF_MIPI|VO_INTF_LCD_24BIT;


        /* USER SET VO DEV SYNC INFO */
        stPubAttr.stSyncInfo.u16Hact = 720;
        stPubAttr.stSyncInfo.u16Hbb = 40;
        stPubAttr.stSyncInfo.u16Hfb = 40;

        stPubAttr.stSyncInfo.u16Vact = 1280;
        stPubAttr.stSyncInfo.u16Vbb = 10;
        stPubAttr.stSyncInfo.u16Vfb = 19;

        stPubAttr.stSyncInfo.u16Hpw = 4;
        stPubAttr.stSyncInfo.u16Vpw = 4;

        Ret = SC_MPI_VO_SetPubAttr(VoDev, &stPubAttr);
        if (Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetPubAttr->Ret:0x%x fail\n", Ret);
            return Ret;
        }

        /* USER SET VO FRAME RATE */
        u32Framerate = 60;
        Ret = SC_MPI_VO_SetDevFrameRate(VoDev, u32Framerate);
        if (Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetDevFrameRate->Ret:0x%x fail\n", Ret);
            return Ret;
        }

        /* USER SET VO SYNC INFO OF USER INTF */
        stUserInfo.bClkReverse = SC_TRUE;
        stUserInfo.u32DevDiv = 1;
        stUserInfo.u32PreDiv = 1;
        stUserInfo.stUserIntfSyncAttr.enClkSource = VO_CLK_SOURCE_PLL;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Fbdiv = 400;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Frac = 0;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Refdiv = 40;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv1 = 1;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv2 = 0;
        Ret = SC_MPI_VO_SetUserIntfSyncInfo(VoDev, &stUserInfo);
        if (Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetUserIntfSyncInfo->Ret:0x%x fail\n", Ret);
            return Ret;
        }
    }

    /* ENABLE VO DEV */
    Ret = SC_MPI_VO_Enable(VoDev);
    if (Ret)
    {
        SAMPLE_PRT("SC_MPI_VO_Enable->Ret:0x%x fail\n", Ret);
        return Ret;
    }

    /* SET VO DISPLAY BUFFER LENGTH */
    Ret = SC_MPI_VO_SetDisplayBufLen(VoDev, 3);
    if (Ret)
    {
        SAMPLE_PRT("SC_MPI_VO_SetDisplayBufLen->Ret:0x%x fail\n", Ret);
        return Ret;
    }

    if (type)
    {
        /* USER CONFIG MIPI DEV */
        Ret = SAMPLE_VO_CONFIG_MIPI(MIPILCD_720x1280);
        if (Ret)
        {
            SAMPLE_PRT("SAMPLE_VO_CONFIG_MIPI->Ret:0x%x fail\n", Ret);
            return Ret;
        }
    }

    /*SET VO LAYER ATTR*/
    stDevSize.u32Width = stPubAttr.stSyncInfo.u16Hact;
    stDevSize.u32Height = stPubAttr.stSyncInfo.u16Vact;

    SAMPLE_VO_GetUserLayerAttr(&stLayerAttr, &stDevSize);
    stLayerAttr.u32DispFrmRt = 60;

    Ret = SC_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr);
    if (Ret)
    {
        SAMPLE_PRT("SC_MPI_VO_SetVideoLayerAttr->Ret:0x%x fail\n", Ret);
        return Ret;
    }

    /* ENABLE VO LAYER */
    Ret = SC_MPI_VO_EnableVideoLayer(VoLayer);
    if (Ret)
    {
        SAMPLE_PRT("SC_MPI_VO_EnableVideoLayer->Ret:0x%x fail\n", Ret);
        return Ret;
    }

    /* SET AND ENABLE VO CHN */
    SAMPLE_VO_GetUserChnAttr(astChnAttr, &stDevSize, VoChnNum);

    for (i = 0; i < VoChnNum; i++)
    {
        Ret = SC_MPI_VO_SetChnAttr(VoLayer, i, &astChnAttr[i]);
        if (Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_SetChnAttr->Ret:0x%x fail\n", Ret);
            return Ret;
        }

        Ret = SC_MPI_VO_EnableChn(VoLayer, i);
        if (Ret)
        {
            SAMPLE_PRT("SC_MPI_VO_EnableChn->Ret:0x%x fail\n", Ret);
            return Ret;
        }
    }

    if (!type)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_MIPIRGB_HDMI_720x1280->hdmi\n");
    }
    else
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_MIPIRGB_HDMI_720x1280->(mipi + rgb)\n");
    }

    return;
}

/*
* Name: SAMPLE_PRIVATE_VO_InitScreen1080x1920
* Desc: Initialize the screen(1080x1920) through mipi_tx.
*/
static SC_VOID SAMPLE_PRIVATE_VO_InitScreen1080x1920(SC_S32 fd)
{
    SC_S32 s32Ret;
    cmd_info_t cmd_info;

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0xeeff;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x4018;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x18;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(20000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0xff;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x1fb;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x135;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0xff51;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x2c53;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x155;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x24d3;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x10d4;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x11;
    cmd_info.data_type = 0x05;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }

    usleep(200000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x29;
    cmd_info.data_type = 0x05;
    cmd_info.cmd = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
}

/*
* Name: SAMPLE_PRIVATE_VO_InitScreen720x1280
* Desc: Initialize the screen(720x1280) through mipi_tx.
*/
static SC_VOID SAMPLE_PRIVATE_VO_InitScreen720x1280(SC_S32 s32fd)
{
    SC_S32     fd     = s32fd;
    SC_S32     s32Ret;
    SC_U8      cmd[30];
    cmd_info_t cmd_info = {0};

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x018712ff;
    // 0x1117006c 0x00000429;
    cmd[0] = 0xff;
    cmd[1] = 0x12;
    cmd[2] = 0x87;
    cmd[3] = 0x01;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x04;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00800023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x8000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x008712ff;
    // 0x1117006c 0x00000329;
    cmd[0] = 0xff;
    cmd[1] = 0x12;
    cmd[2] = 0x87;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x03;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x312200e1;
    // 0x11170070 0x68635443;
    // 0x11170070 0x61a28896;
    // 0x11170070 0x42415f4c;
    // 0x11170070 0x21262e37;
    // 0x11170070 0x0000001f;
    // 0x1117006c 0x00001529;
    cmd[0] = 0xe1;
    cmd[1] = 0x00;
    cmd[2] = 0x22;
    cmd[3] = 0x31;

    cmd[4] = 0x43;
    cmd[5] = 0x54;
    cmd[6] = 0x63;
    cmd[7] = 0x68;

    cmd[8] = 0x96;
    cmd[9] = 0x88;
    cmd[10] = 0xa2;
    cmd[11] = 0x61;

    cmd[12] = 0x4c;
    cmd[13] = 0x5f;
    cmd[14] = 0x41;
    cmd[15] = 0x42;

    cmd[16] = 0x37;
    cmd[17] = 0x2e;
    cmd[18] = 0x26;
    cmd[19] = 0x21;

    cmd[20] = 0x1f;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x15;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x312200e2;
    // 0x11170070 0x68635443;
    // 0x11170070 0x61a28896;
    // 0x11170070 0x42415f4c;
    // 0x11170070 0x21262e37;
    // 0x11170070 0x0000001f;
    // 0x1117006c 0x00001529;
    cmd[0] = 0xe2;
    cmd[1] = 0x00;
    cmd[2] = 0x22;
    cmd[3] = 0x31;

    cmd[4] = 0x43;
    cmd[5] = 0x54;
    cmd[6] = 0x63;
    cmd[7] = 0x68;

    cmd[8] = 0x96;
    cmd[9] = 0x88;
    cmd[10] = 0xa2;
    cmd[11] = 0x61;

    cmd[12] = 0x4c;
    cmd[13] = 0x5f;
    cmd[14] = 0x41;
    cmd[15] = 0x42;

    cmd[16] = 0x37;
    cmd[17] = 0x2e;
    cmd[18] = 0x26;
    cmd[19] = 0x21;

    cmd[20] = 0x1f;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x15;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00910023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x9100;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xa3ffe8ca;
    // 0x11170070 0x05ffa3ff;
    // 0x11170070 0x05030503;
    // 0x11170070 0x00000003;
    // 0x1117006c 0x00000d29;
    cmd[0] = 0xca;
    cmd[1] = 0xe8;
    cmd[2] = 0xff;
    cmd[3] = 0xa3;

    cmd[4] = 0xff;
    cmd[5] = 0xa3;
    cmd[6] = 0xff;
    cmd[7] = 0x05;

    cmd[8] = 0x03;
    cmd[9] = 0x05;
    cmd[10] = 0x03;
    cmd[11] = 0x05;

    cmd[12] = 0x03;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0d;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0010c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x10c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        printf("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xacaba0c7;
    // 0x11170070 0xbacca7bc;
    // 0x11170070 0x8888bbbc;
    // 0x11170070 0x44444678;
    // 0x11170070 0x00444444;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0xa0;
    cmd[2] = 0xab;
    cmd[3] = 0xac;

    cmd[4] = 0xbc;
    cmd[5] = 0xa7;
    cmd[6] = 0xcc;
    cmd[7] = 0xba;

    cmd[8] = 0xbc;
    cmd[9] = 0xbb;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x78;
    cmd[13] = 0x46;
    cmd[14] = 0x44;
    cmd[15] = 0x44;

    cmd[16] = 0x44;
    cmd[17] = 0x44;
    cmd[18] = 0x44;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        printf("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0011c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x11c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xabaaa0c7;
    // 0x11170070 0xaacc88bc;
    // 0x11170070 0x8888babc;
    // 0x11170070 0x55555678;
    // 0x11170070 0x00444445;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0xa0;
    cmd[2] = 0xaa;
    cmd[3] = 0xab;

    cmd[4] = 0xbc;
    cmd[5] = 0x88;
    cmd[6] = 0xcc;
    cmd[7] = 0xaa;

    cmd[8] = 0xbc;
    cmd[9] = 0xba;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x78;
    cmd[13] = 0x56;
    cmd[14] = 0x55;
    cmd[15] = 0x55;

    cmd[16] = 0x45;
    cmd[17] = 0x44;
    cmd[18] = 0x44;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0012c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x12c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xab9aa0c7;
    // 0x11170070 0xaabc88ac;
    // 0x11170070 0x8888aabb;
    // 0x11170070 0x55555678;
    // 0x11170070 0x00555555;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0xa0;
    cmd[2] = 0x9a;
    cmd[3] = 0xab;

    cmd[4] = 0xac;
    cmd[5] = 0x88;
    cmd[6] = 0xbc;
    cmd[7] = 0xaa;

    cmd[8] = 0xbb;
    cmd[9] = 0xaa;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x78;
    cmd[13] = 0x56;
    cmd[14] = 0x55;
    cmd[15] = 0x55;

    cmd[16] = 0x55;
    cmd[17] = 0x55;
    cmd[18] = 0x55;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        printf("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0013c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xab99a0c7;
    // 0x11170070 0x9abb88ba;
    // 0x11170070 0x88889bbb;
    // 0x11170070 0x55556788;
    // 0x11170070 0x00555555;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0xa0;
    cmd[2] = 0x99;
    cmd[3] = 0xab;

    cmd[4] = 0xba;
    cmd[5] = 0x88;
    cmd[6] = 0xbb;
    cmd[7] = 0x9a;

    cmd[8] = 0xbb;
    cmd[9] = 0x9b;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x67;
    cmd[14] = 0x55;
    cmd[15] = 0x55;

    cmd[16] = 0x55;
    cmd[17] = 0x55;
    cmd[18] = 0x55;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0014c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x14c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xaa9a90c7;
    // 0x11170070 0xa9ba88ba;
    // 0x11170070 0x88889baa;
    // 0x11170070 0x66666788;
    // 0x11170070 0x00555555;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x9a;
    cmd[3] = 0xaa;

    cmd[4] = 0xba;
    cmd[5] = 0x88;
    cmd[6] = 0xba;
    cmd[7] = 0xa9;

    cmd[8] = 0xaa;
    cmd[9] = 0x9b;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x67;
    cmd[14] = 0x66;
    cmd[15] = 0x66;

    cmd[16] = 0x55;
    cmd[17] = 0x55;
    cmd[18] = 0x55;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0015c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x15c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xaa9990c7;
    // 0x11170070 0x99ba88b9;
    // 0x11170070 0x88889baa;
    // 0x11170070 0x66666788;
    // 0x11170070 0x00555666;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x99;
    cmd[3] = 0xaa;

    cmd[4] = 0xb9;
    cmd[5] = 0x88;
    cmd[6] = 0xba;
    cmd[7] = 0x99;

    cmd[8] = 0xaa;
    cmd[9] = 0x9b;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x67;
    cmd[14] = 0x66;
    cmd[15] = 0x66;

    cmd[16] = 0x66;
    cmd[17] = 0x56;
    cmd[18] = 0x55;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0016c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x16c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x9a9990c7;
    // 0x11170070 0xa8ba87b9;
    // 0x11170070 0x88889b99;
    // 0x11170070 0x66666888;
    // 0x11170070 0x00666666;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x99;
    cmd[3] = 0x9a;

    cmd[4] = 0xb9;
    cmd[5] = 0x87;
    cmd[6] = 0xba;
    cmd[7] = 0xa8;

    cmd[8] = 0x99;
    cmd[9] = 0x9b;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x68;
    cmd[14] = 0x66;
    cmd[15] = 0x66;

    cmd[16] = 0x66;
    cmd[17] = 0x66;
    cmd[18] = 0x66;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0017c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x17c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xaa9890c7;
    // 0x11170070 0x99a987c7;
    // 0x11170070 0x8888aa9a;
    // 0x11170070 0x66677888;
    // 0x11170070 0x00666666;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x98;
    cmd[3] = 0xaa;

    cmd[4] = 0xc7;
    cmd[5] = 0x87;
    cmd[6] = 0xa9;
    cmd[7] = 0x99;

    cmd[8] = 0x9a;
    cmd[9] = 0xaa;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x78;
    cmd[14] = 0x67;
    cmd[15] = 0x66;

    cmd[16] = 0x66;
    cmd[17] = 0x66;
    cmd[18] = 0x66;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0018c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x18c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xa99890c7;
    // 0x11170070 0x89b888b7;
    // 0x11170070 0x8888aa99;
    // 0x11170070 0x77777888;
    // 0x11170070 0x00666667;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x98;
    cmd[3] = 0xa9;

    cmd[4] = 0xb7;
    cmd[5] = 0x88;
    cmd[6] = 0xb8;
    cmd[7] = 0x89;

    cmd[8] = 0x99;
    cmd[9] = 0xaa;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x78;
    cmd[14] = 0x77;
    cmd[15] = 0x77;

    cmd[16] = 0x67;
    cmd[17] = 0x66;
    cmd[18] = 0x66;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0019c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x19c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xa99790c7;
    // 0x11170070 0x89a888b7;
    // 0x11170070 0x8888a98a;
    // 0x11170070 0x77777888;
    // 0x11170070 0x00667777;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x97;
    cmd[3] = 0xa9;

    cmd[4] = 0xb7;
    cmd[5] = 0x88;
    cmd[6] = 0xa8;
    cmd[7] = 0x89;

    cmd[8] = 0x8a;
    cmd[9] = 0xa9;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x78;
    cmd[14] = 0x77;
    cmd[15] = 0x77;

    cmd[16] = 0x77;
    cmd[17] = 0x77;
    cmd[18] = 0x66;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x001ac623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x1ac6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x999970c7;
    // 0x11170070 0x88b887b7;
    // 0x11170070 0x8888a989;
    // 0x11170070 0x77778888;
    // 0x11170070 0x00777777;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x70;
    cmd[2] = 0x99;
    cmd[3] = 0x99;

    cmd[4] = 0xb7;
    cmd[5] = 0x87;
    cmd[6] = 0xb8;
    cmd[7] = 0x88;

    cmd[8] = 0x89;
    cmd[9] = 0xa9;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x88;
    cmd[14] = 0x77;
    cmd[15] = 0x77;

    cmd[16] = 0x77;
    cmd[17] = 0x77;
    cmd[18] = 0x77;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x001bc623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x1bc6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x998970c7;
    // 0x11170070 0x88a888a7;
    // 0x11170070 0x88889989;
    // 0x11170070 0x78888888;
    // 0x11170070 0x00777777;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x70;
    cmd[2] = 0x89;
    cmd[3] = 0x99;

    cmd[4] = 0xa7;
    cmd[5] = 0x88;
    cmd[6] = 0xa8;
    cmd[7] = 0x88;

    cmd[8] = 0x89;
    cmd[9] = 0x99;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x88;
    cmd[14] = 0x88;
    cmd[15] = 0x78;

    cmd[16] = 0x77;
    cmd[17] = 0x77;
    cmd[18] = 0x77;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x001cc623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x1cc6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xa89790c7;
    // 0x11170070 0x98a878a7;
    // 0x11170070 0x88889988;
    // 0x11170070 0x78888888;
    // 0x11170070 0x00777777;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x97;
    cmd[3] = 0xa8;

    cmd[4] = 0xa7;
    cmd[5] = 0x78;
    cmd[6] = 0xa8;
    cmd[7] = 0x98;

    cmd[8] = 0x88;
    cmd[9] = 0x99;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x88;
    cmd[14] = 0x88;
    cmd[15] = 0x78;

    cmd[16] = 0x77;
    cmd[17] = 0x77;
    cmd[18] = 0x77;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x001dc623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x1dc6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x898790c7;
    // 0x11170070 0x89988898;
    // 0x11170070 0x88889889;
    // 0x11170070 0x88888888;
    // 0x11170070 0x00777778;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x87;
    cmd[3] = 0x89;

    cmd[4] = 0x98;
    cmd[5] = 0x88;
    cmd[6] = 0x98;
    cmd[7] = 0x89;

    cmd[8] = 0x89;
    cmd[9] = 0x98;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x88;
    cmd[14] = 0x88;
    cmd[15] = 0x88;

    cmd[16] = 0x78;
    cmd[17] = 0x77;
    cmd[18] = 0x77;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x001ec623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x1ec6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x988790c7;
    // 0x11170070 0x88988897;
    // 0x11170070 0x88889889;
    // 0x11170070 0x88888888;
    // 0x11170070 0x00777888;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x87;
    cmd[3] = 0x98;

    cmd[4] = 0x97;
    cmd[5] = 0x88;
    cmd[6] = 0x98;
    cmd[7] = 0x88;

    cmd[8] = 0x89;
    cmd[9] = 0x98;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x88;
    cmd[14] = 0x88;
    cmd[15] = 0x88;

    cmd[16] = 0x88;
    cmd[17] = 0x78;
    cmd[18] = 0x77;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x001fc623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x1fc6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x888790c7;
    // 0x11170070 0x88988798;
    // 0x11170070 0x88888888;
    // 0x11170070 0x88888888;
    // 0x11170070 0x00888888;
    // 0x1117006c 0x00001329;
    cmd[0] = 0xc7;
    cmd[1] = 0x90;
    cmd[2] = 0x87;
    cmd[3] = 0x88;

    cmd[4] = 0x98;
    cmd[5] = 0x87;
    cmd[6] = 0x98;
    cmd[7] = 0x88;

    cmd[8] = 0x88;
    cmd[9] = 0x88;
    cmd[10] = 0x88;
    cmd[11] = 0x88;

    cmd[12] = 0x88;
    cmd[13] = 0x88;
    cmd[14] = 0x88;
    cmd[15] = 0x88;

    cmd[16] = 0x88;
    cmd[17] = 0x88;
    cmd[18] = 0x88;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0000c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00b10023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xb100;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0005c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x05c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00640023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xb400;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0013c623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x13c6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00a00023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xa000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x010001d6;
    // 0x11170070 0x014d0100;
    // 0x11170070 0x019a01b3;
    // 0x11170070 0x0000009a;
    // 0x1117006c 0x00000d29;
    cmd[0] = 0xd6;
    cmd[1] = 0x01;
    cmd[2] = 0x00;
    cmd[3] = 0x01;

    cmd[4] = 0x00;
    cmd[5] = 0x01;
    cmd[6] = 0x4d;
    cmd[7] = 0x01;

    cmd[8] = 0xb3;
    cmd[9] = 0x01;
    cmd[10] = 0x9a;
    cmd[11] = 0x01;

    cmd[12] = 0x9a;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0d;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00b00023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xb000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x159a01d6;
    // 0x11170070 0x159a159a;
    // 0x11170070 0x019a019a;
    // 0x11170070 0x0000009a;
    // 0x1117006c 0x00000d29;
    cmd[0] = 0xd6;
    cmd[1] = 0x01;
    cmd[2] = 0x9a;
    cmd[3] = 0x15;

    cmd[4] = 0x9a;
    cmd[5] = 0x15;
    cmd[6] = 0x9a;
    cmd[7] = 0x15;

    cmd[8] = 0x9a;
    cmd[9] = 0x01;
    cmd[10] = 0x9a;
    cmd[11] = 0x01;

    cmd[12] = 0x9a;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0d;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00c00023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xc000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x001133d6;
    // 0x11170070 0x66771166;
    // 0x11170070 0x11666611;
    // 0x11170070 0x00000066;
    // 0x1117006c 0x00000d29;
    cmd[0] = 0xd6;
    cmd[1] = 0x33;
    cmd[2] = 0x11;
    cmd[3] = 0x00;

    cmd[4] = 0x66;
    cmd[5] = 0x11;
    cmd[6] = 0x77;
    cmd[7] = 0x66;

    cmd[8] = 0x11;
    cmd[9] = 0x66;
    cmd[10] = 0x66;
    cmd[11] = 0x11;

    cmd[12] = 0x66;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0d;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00d00023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xd000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x661166d6;
    // 0x11170070 0x00661166;
    // 0x1117006c 0x00000729;
    cmd[0] = 0xd6;
    cmd[1] = 0x66;
    cmd[2] = 0x11;
    cmd[3] = 0x66;

    cmd[4] = 0x66;
    cmd[5] = 0x11;
    cmd[6] = 0x66;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x07;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00e00023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xe000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x331133d6;
    // 0x11170070 0x333c1133;
    // 0x11170070 0x11333311;
    // 0x11170070 0x00000033;
    // 0x1117006c 0x00000d29;
    cmd[0] = 0xd6;
    cmd[1] = 0x33;
    cmd[2] = 0x11;
    cmd[3] = 0x33;

    cmd[4] = 0x33;
    cmd[5] = 0x11;
    cmd[6] = 0x3c;
    cmd[7] = 0x33;

    cmd[8] = 0x11;
    cmd[9] = 0x33;
    cmd[10] = 0x33;
    cmd[11] = 0x11;

    cmd[12] = 0x33;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0d;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00f00023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xf000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x331133d6;
    // 0x11170070 0x00331133;
    // 0x1117006c 0x00000729;
    cmd[0] = 0xd6;
    cmd[1] = 0x33;
    cmd[2] = 0x11;
    cmd[3] = 0x33;

    cmd[4] = 0x33;
    cmd[5] = 0x11;
    cmd[6] = 0x33;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x07;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00800023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x8000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0000d623;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x00d6;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00d00023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xd000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x00aaaad9;
    // 0x1117006c 0x00000329;
    cmd[0] = 0xd9;
    cmd[1] = 0xaa;
    cmd[2] = 0xaa;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x03;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00c20023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xc200;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00c0f523;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xc0f5;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00800023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x8000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0001c423;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x01c4;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00880023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x8800;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x0080c423;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x80c4;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00b20023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xb200;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00e3c523;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xe3c5;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);
    // 0x1117006c 0x00810023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x8100;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0x8f877fca;
    // 0x11170070 0xafa79f97;
    // 0x11170070 0xcfc7bfb7;
    // 0x11170070 0xefe7dfd7;
    // 0x11170070 0x000000f7;
    // 0x1117006c 0x00001129;
    cmd[0] = 0xca;
    cmd[1] = 0x7f;
    cmd[2] = 0x87;
    cmd[3] = 0x8f;

    cmd[4] = 0x97;
    cmd[5] = 0x9f;
    cmd[6] = 0xa7;
    cmd[7] = 0xaf;

    cmd[8] = 0xb7;
    cmd[9] = 0xbf;
    cmd[10] = 0xc7;
    cmd[11] = 0xcf;

    cmd[12] = 0xd7;
    cmd[13] = 0xdf;
    cmd[14] = 0xe7;
    cmd[15] = 0xef;

    cmd[16] = 0xf7;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x11;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x11170070 0xffffffff;
    // 0x1117006c 0x00000429;
    cmd[0] = 0xff;
    cmd[1] = 0xff;
    cmd[2] = 0xff;
    cmd[3] = 0xff;

    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x04;
    cmd_info.data_type = 0x29;
    cmd_info.cmd       = cmd;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00000023;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0000;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00008123;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0081;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00ff5123;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0xff51;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00245323;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x2453;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00015523;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0155;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00285e23;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x285e;
    cmd_info.data_type = 0x23;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(1000);

    // 0x1117006c 0x00001105;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0011;
    cmd_info.data_type = 0x05;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(200000);

    // 0x1117006c 0x00002905;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0029;
    cmd_info.data_type = 0x05;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(10000);

    // 0x1117006c 0x00003515;
    cmd_info.devno     = 0;
    cmd_info.cmd_size  = 0x0035;
    cmd_info.data_type = 0x15;
    cmd_info.cmd       = NULL;
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, &cmd_info);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
    usleep(10000);
}

#if SAMPLE_VO_USE_DEFAULT_MIPI_TX

/*
* Name: SAMPLE_PRIVATE_VO_InitMipiTxScreen
* Desc: start mipi_tx screen.
*/
static void SAMPLE_PRIVATE_VO_InitMipiTxScreen(VO_INTF_SYNC_E enVoIntfSync, SC_S32 fd)
{
    if( VO_OUTPUT_1080x1920_60 == enVoIntfSync ||
        VO_OUTPUT_1080P60      == enVoIntfSync)
    {
        SAMPLE_PRT("%s,%d,Init 1080p screen.\n", __FUNCTION__, __LINE__);
        // init screen for 1080x1920_60.
        SAMPLE_PRIVATE_VO_InitScreen1080x1920(fd);
    }
    else if (VO_OUTPUT_720x1280_60 == enVoIntfSync ||
        VO_OUTPUT_720P60      == enVoIntfSync)
    {
        SAMPLE_PRT("%s,%d,Init 720p screen.\n", __FUNCTION__, __LINE__);
        // init screen for 720x1280_60.
        SAMPLE_PRIVATE_VO_InitScreen720x1280(fd);
    }
    else
    {
        SAMPLE_PRT("%s,%d,There is no screen to init\n", __FUNCTION__, __LINE__);
    }
}
#else
/*
* Name: SAMPLE_PRIVATE_VO_InitMipiTxDev
* Desc: start mipi_tx device.
*/
static void SAMPLE_PRIVATE_VO_InitMipiTxDev(VO_INTF_SYNC_E enVoIntfSync)
{
    SAMPLE_PRT("error: mipi_tx dev has not been initialized yet\n");
}
#endif

/*
* Name: SAMPLE_COMM_VO_StartMipiTx
* Desc: start mipi_tx units.
*/
void SAMPLE_COMM_VO_StartMipiTx(VO_INTF_SYNC_E enVoIntfSync)
{
    SC_S32            fd;
    SC_S32            s32Ret;
    combo_dev_cfg_t *pstMipiTxConfig;

    fd = open("/dev/mipi_tx", O_RDWR);
    if (fd < 0)
    {
        SAMPLE_PRT("open sc_mipi_tx dev failed\n");
        return;
    }

    switch(enVoIntfSync)
    {
    case VO_OUTPUT_576P50:
        pstMipiTxConfig = &MIPI_TX_720X576_50_CONFIG;
        break;
    case VO_OUTPUT_720P60:
        pstMipiTxConfig = &MIPI_TX_1280X720_60_CONFIG;
        break;
    case VO_OUTPUT_1080P60:
        pstMipiTxConfig = &MIPI_TX_1920X1080_60_CONFIG;
        break;
    case VO_OUTPUT_1024x768_60:
        pstMipiTxConfig = &MIPI_TX_1024X768_60_CONFIG;
        break;
    case VO_OUTPUT_1280x1024_60:
        pstMipiTxConfig = &MIPI_TX_1280x1024_60_CONFIG;
        break;
    case VO_OUTPUT_720x1280_60:
        pstMipiTxConfig = &MIPI_TX_720X1280_60_CONFIG;
        break;
    case VO_OUTPUT_1080x1920_60:
        pstMipiTxConfig = &MIPI_TX_1080X1920_60_CONFIG;
        break;
    default :
        pstMipiTxConfig = &MIPI_TX_1080X1920_60_CONFIG;
        break;
    }
    /*
    * step 1 : Config mipi_tx controller.
    */
    s32Ret = ioctl(fd, SC_MIPI_TX_SET_DEV_CFG, pstMipiTxConfig);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET_DEV_CONFIG failed\n");
        close(fd);
        return;
    }

    /*
    * NOTICE!!! Do it yourself: change SAMPLE_VO_USE_DEFAULT_MIPI_TX to 0.
    * step 2 : Init screen or other peripheral unit.
    */
    #if SAMPLE_VO_USE_DEFAULT_MIPI_TX
    SAMPLE_PRIVATE_VO_InitMipiTxScreen(enVoIntfSync, fd);
    #else
    SAMPLE_PRIVATE_VO_InitMipiTxDev(enVoIntfSync);
    #endif

    usleep(10000);

    /*
    * step 3 : enable mipi_tx controller.
    */
    s32Ret = ioctl(fd, SC_MIPI_TX_ENABLE);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX enable failed\n");
        close(fd);
        return;
    }

    close(fd);
    return ;
}

SC_S32 SAMPLE_OPEN_MIPITx_FD(SC_VOID)
{
    SC_S32 fd;

    fd = open("/dev/mipi_tx", O_RDWR);
    if (fd < 0)
    {
        printf("open mipi_tx dev failed\n");
    }

    return fd;
}

SC_VOID SAMPLE_CLOSE_MIPITx_FD(SC_S32 fd)
{
    close(fd);
    return;
}

SC_VOID SAMPLE_GetMipiTxConfig(MIPILCD_TYPE type, combo_dev_cfg_t *pstMipiTxConfig)
{
    /* USER NEED SET MIPI DEV CONFIG */
    switch (type)
    {
        case MIPILCD_720x1280:
        {
            pstMipiTxConfig->devno = 0;
            pstMipiTxConfig->lane_id[0] = 0;
            pstMipiTxConfig->lane_id[1] = 1;
            pstMipiTxConfig->lane_id[2] = 2;
            pstMipiTxConfig->lane_id[3] = 3;
            pstMipiTxConfig->output_mode = OUTPUT_MODE_DSI_VIDEO;
            pstMipiTxConfig->output_format = OUT_FORMAT_RGB_24_BIT;
            pstMipiTxConfig->video_mode = BURST_MODE;
            pstMipiTxConfig->sync_info.vid_pkt_size = 720;
            pstMipiTxConfig->sync_info.vid_hsa_pixels = 3;
            pstMipiTxConfig->sync_info.vid_hbp_pixels = 30;
            pstMipiTxConfig->sync_info.vid_hline_pixels = 783;
            pstMipiTxConfig->sync_info.vid_vsa_lines = 4;
            pstMipiTxConfig->sync_info.vid_vbp_lines = 10;
            pstMipiTxConfig->sync_info.vid_vfp_lines = 19;
            pstMipiTxConfig->sync_info.vid_active_lines = 1280;
            pstMipiTxConfig->sync_info.edpi_cmd_size = 0;
            pstMipiTxConfig->phy_data_rate = 380;
            pstMipiTxConfig->pixel_clk = 63339;
        }
        break;

        case MIPILCD_480x640:
        {
            pstMipiTxConfig->devno = 0;
            pstMipiTxConfig->lane_id[0] = 0;
            pstMipiTxConfig->lane_id[1] = 1;
            pstMipiTxConfig->lane_id[2] = -1;
            pstMipiTxConfig->lane_id[3] = -1;
            pstMipiTxConfig->output_mode = OUTPUT_MODE_DSI_VIDEO;
            pstMipiTxConfig->output_format = OUT_FORMAT_RGB_24_BIT;
            pstMipiTxConfig->video_mode = BURST_MODE;
            pstMipiTxConfig->sync_info.vid_pkt_size = 480;
            pstMipiTxConfig->sync_info.vid_hsa_pixels = 15;
            pstMipiTxConfig->sync_info.vid_hbp_pixels = 30;
            pstMipiTxConfig->sync_info.vid_hline_pixels = 555;
            pstMipiTxConfig->sync_info.vid_vsa_lines = 20;
            pstMipiTxConfig->sync_info.vid_vbp_lines = 40;
            pstMipiTxConfig->sync_info.vid_vfp_lines = 40;
            pstMipiTxConfig->sync_info.vid_active_lines = 640;
            pstMipiTxConfig->sync_info.edpi_cmd_size = 0;
            pstMipiTxConfig->phy_data_rate = 282;
            pstMipiTxConfig->pixel_clk = 23532;
        }
        break;

        case MIPILCD_800x1280:
        {
            pstMipiTxConfig->devno = 0;
            pstMipiTxConfig->lane_id[0] = 0;
            pstMipiTxConfig->lane_id[1] = 1;
            pstMipiTxConfig->lane_id[2] = 2;
            pstMipiTxConfig->lane_id[3] = 3;
            pstMipiTxConfig->output_mode = OUTPUT_MODE_DSI_VIDEO;
            pstMipiTxConfig->output_format = OUT_FORMAT_RGB_24_BIT;
            pstMipiTxConfig->video_mode = BURST_MODE;
            pstMipiTxConfig->sync_info.vid_pkt_size = 800;
            pstMipiTxConfig->sync_info.vid_hsa_pixels = 15;
            pstMipiTxConfig->sync_info.vid_hbp_pixels = 15;
            pstMipiTxConfig->sync_info.vid_hline_pixels = 860;
            pstMipiTxConfig->sync_info.vid_vsa_lines = 4;
            pstMipiTxConfig->sync_info.vid_vbp_lines = 20;
            pstMipiTxConfig->sync_info.vid_vfp_lines = 20;
            pstMipiTxConfig->sync_info.vid_active_lines = 1280;
            pstMipiTxConfig->sync_info.edpi_cmd_size = 0;
            pstMipiTxConfig->phy_data_rate = 419;
            pstMipiTxConfig->pixel_clk = 69907;
        }
        break;

        case MIPILCD_1024x600:
        default:
        {
            pstMipiTxConfig->devno = 0;
            pstMipiTxConfig->lane_id[0] = 0;
            pstMipiTxConfig->lane_id[1] = 1;
            pstMipiTxConfig->lane_id[2] = 2;
            pstMipiTxConfig->lane_id[3] = 3;
            pstMipiTxConfig->output_mode = OUTPUT_MODE_DSI_VIDEO;
            pstMipiTxConfig->output_format = OUT_FORMAT_RGB_24_BIT;
            pstMipiTxConfig->video_mode = BURST_MODE;
            pstMipiTxConfig->sync_info.vid_pkt_size = 1024;
            pstMipiTxConfig->sync_info.vid_hsa_pixels = 6;
            pstMipiTxConfig->sync_info.vid_hbp_pixels = 120;
            pstMipiTxConfig->sync_info.vid_hline_pixels = 1270;
            pstMipiTxConfig->sync_info.vid_vsa_lines = 10;
            pstMipiTxConfig->sync_info.vid_vbp_lines = 23;
            pstMipiTxConfig->sync_info.vid_vfp_lines = 12;
            pstMipiTxConfig->sync_info.vid_active_lines = 600;
            pstMipiTxConfig->sync_info.edpi_cmd_size = 0;
            pstMipiTxConfig->phy_data_rate = 313;
            pstMipiTxConfig->pixel_clk = 52322;
        }
    }

    return;
}

SC_S32 SAMPLE_SetMipiTxConfig(SC_S32 fd, combo_dev_cfg_t *pstMipiTxConfig)
{
    SC_S32 s32Ret;

    //s32Ret = ioctl(fd, SC_MIPI_TX_SET_DEV_CFG, pstMipiTxConfig);

    s32Ret = SC_MPI_VO_MipiTxSet(0, SC_MIPI_TX_SET_DEV_CFG, pstMipiTxConfig,
            sizeof(combo_dev_cfg_t));
    if (s32Ret != SC_SUCCESS)
    {
        printf("MIPI_TX SET_DEV_CONFIG failed\n");
        SAMPLE_CLOSE_MIPITx_FD(fd);
        return s32Ret;
    }
    return s32Ret;
}

SC_S32 SAMPLE_SET_MIPITx_Dev_ATTR(SC_S32 fd, MIPILCD_TYPE type)
{
    SC_S32 s32Ret;
    combo_dev_cfg_t stMipiTxConfig;

    /* USER SET MIPI DEV CONFIG */
    SAMPLE_GetMipiTxConfig(type, &stMipiTxConfig);

    /* USER SET MIPI DEV CONFIG */
    s32Ret = SAMPLE_SetMipiTxConfig(fd, &stMipiTxConfig);

    return s32Ret;
}

SC_S32 SAMPLE_USER_INIT_MIPITx(SC_S32 fd, cmd_info_t *pcmd_info)
{
    SC_S32 s32Ret;

    //s32Ret = ioctl(fd, SC_MIPI_TX_SET_CMD, pcmd_info);
    s32Ret = SC_MPI_VO_MipiTxSet(0, SC_MIPI_TX_SET_CMD, pcmd_info, sizeof(cmd_info_t));
    if (s32Ret !=  SC_SUCCESS)
    {
        printf("MIPI_TX SET CMD failed\n");
        //SAMPLE_CLOSE_MIPITx_FD(fd);
        return s32Ret;
    }

    return SC_SUCCESS;
}

static SC_S32 SAMPLE_VO_MIPITx_GetData(char addr, char byte)
{
    SC_S32 s32Ret;

    unsigned char readdata[128] = {0};
    get_cmd_info_t getCmdInfo;
    getCmdInfo.devno = 0;
    getCmdInfo.data_type = 0x14;
    getCmdInfo.get_data = readdata;
    getCmdInfo.get_data_size = 1;
    getCmdInfo.data_param = addr;
    s32Ret = SC_MPI_VO_MipiTxSet(0, SC_MIPI_TX_GET_CMD, &getCmdInfo, sizeof(get_cmd_info_t));
    SAMPLE_PRT("s32Ret:%d addr:0x%x byte:%d, get_data[0]:0x%x\n",
        s32Ret, addr, byte, getCmdInfo.get_data[0]);
    if (s32Ret)
    {
        SAMPLE_PRT("SAMPLE_VO_MIPITx_GetData error, s32Ret:%d!\n", s32Ret);
        return SC_FAILURE;
    }


    SAMPLE_PRT("SAMPLE_VO_MIPITx_GetData success!\n");

    return SC_SUCCESS;
}

SC_S32 SAMPLE_VO_INIT_MIPITx_Screen(SC_S32 fd)
{
    SC_S32 s32Ret;
    cmd_info_t cmd_info;

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0xeeff;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x4018;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x18;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0xff;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x1fb;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x135;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0xff51;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x2c53;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x155;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(1000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x24d3;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x10d4;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;

    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(10000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x11;
    cmd_info.data_type = 0x05;
    cmd_info.cmd = NULL;

    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(200000);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x29;
    cmd_info.data_type = 0x05;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    usleep(200000);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_VO_INIT_MIPITx_Screen800x1280(SC_S32 fd)
{
    SC_S32 i, s32Ret;
    cmd_info_t cmd_info;
    SC_U16 cmd_data[] =
    {
        0x00E0, 0x93E1, 0x65E2, 0xF8E3, 0x0380, 0x01E0, 0x0000, 0x6501, 0x0003,
        0x5B04, 0x0017, 0xE718, 0x0019, 0x001A, 0xE71B, 0x001C, 0xFE24, 0x2335,
        0x5937, 0x0538, 0x013A, 0x013B, 0x703C, 0xFF3D, 0xFF3E, 0x7F3F, 0x0640,
        0xA041, 0x1043, 0x0F44, 0x2845, 0x044B, 0x0255, 0x0156, 0x6957, 0x0A58,
        0x2A59, 0x295A, 0x1E5B, 0x7F5D, 0x695E, 0x5A5F, 0x4E60, 0x4B61, 0x3D62,
        0x4463, 0x3164, 0x4D65, 0x4F66, 0x5167, 0x6F68, 0x5C69, 0x616A, 0x516B,
        0x4B6C, 0x3D6D, 0x2B6E, 0x006F, 0x7F70, 0x6971, 0x5A72, 0x4E73, 0x4B74,
        0x3D75, 0x4476, 0x3177, 0x4D78, 0x4F79, 0x517A, 0x6F7B, 0x5C7C, 0x617D,
        0x517E, 0x4B7F, 0x3D80, 0x2B81, 0x0082, 0x02E0, 0x5F00, 0x5F01, 0x4102,
        0x4B03, 0x4B04, 0x5C05, 0x5C06, 0x4907, 0x4908, 0x5A09, 0x5A0A, 0x470B,
        0x470C, 0x4F0D, 0x4F0E, 0x450F, 0x4510, 0x4D11, 0x4D12, 0x5F13, 0x5614,
        0x5115, 0x5F16, 0x5F17, 0x4018, 0x4A19, 0x4A1A, 0x5B1B, 0x5B1C, 0x481D,
        0x481E, 0x591F, 0x5920, 0x4621, 0x4622, 0x4E23, 0x4E24, 0x4425, 0x4426,
        0x4C27, 0x4C28, 0x5F29, 0x552A, 0x502B, 0x1F2C, 0x1F2D, 0x102E, 0x0C2F,
        0x0C30, 0x0431, 0x0432, 0x0E33, 0x0E34, 0x0635, 0x0636, 0x1937, 0x1938,
        0x0839, 0x083A, 0x1B3B, 0x1B3C, 0x0A3D, 0x0A3E, 0x153F, 0x1F40, 0x0041,
        0x1F42, 0x1F43, 0x1144, 0x0D45, 0x0D46, 0x0547, 0x0548, 0x0F49, 0x0F4A,
        0x074B, 0x074C, 0x1A4D, 0x1A4E, 0x094F, 0x0950, 0x1C51, 0x1C52, 0x0B53,
        0x0B54, 0x1655, 0x1F56, 0x0157, 0x4058, 0x0059, 0x005A, 0x105B, 0x015C,
        0xD05D, 0x015E, 0x025F, 0x7060, 0x0161, 0x0262, 0x0A63, 0x5764, 0x7565,
        0x0F66, 0xF767, 0x0168, 0x0A69, 0x576A, 0x186B, 0x006C, 0x046D, 0x046E,
        0x886F, 0x0070, 0x0071, 0x0672, 0x7B73, 0x0074, 0x0075, 0x0076, 0xD077,
        0x0078, 0x1579, 0x027A, 0x037B, 0x017C, 0x0A7D, 0x577E, 0x03E0, 0x019A,
        0x029B, 0x01AF, 0x04E0, 0x2302, 0x1109, 0x480E, 0x082B, 0x112D, 0x032E,
        0x4936, 0x5837, 0x0A96, 0x00E0, 0x8051, 0x2C53, 0x0055,
    };

    for (i = 0; i < sizeof(cmd_data) / 2; i++)
    {
        cmd_info.devno = 0;
        cmd_info.cmd_size = cmd_data[i];
        cmd_info.data_type = 0x23;
        cmd_info.cmd = NULL;
        s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_USER_INIT_MIPITx failed!\n");;
            return s32Ret;
        }

        //SAMPLE_PRT("cmd_data[%d]=0x%x\n", i, cmd_data[i]);
    }

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x0011;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_USER_INIT_MIPITx failed!\n");;
        return s32Ret;
    }
    usleep(120000);
    //SAMPLE_PRT("cmd_data[%d]=0x0011\n", i++);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x0029;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_USER_INIT_MIPITx failed!\n");;
        return s32Ret;
    }
    usleep(20000);
    //SAMPLE_PRT("cmd_data[%d]=0x0029\n", i++);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x0035;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info) ;
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_USER_INIT_MIPITx failed!\n");;
        return s32Ret;
    }
    //SAMPLE_PRT("cmd_data[%d]=0x0035\n", i++);

    return SC_SUCCESS;
}

SC_S32 SAMPLE_VO_INIT_MIPITx_Screen480x640(SC_S32 fd)
{
    SC_S32 i, j, k, s32Ret;
    cmd_info_t cmd_info;
    MIPI_INITPARAM_S mipi_initparam[] =
    {
        {0,   0xFF, 5,  {0x77, 0x01, 0x00, 0x00, 0x13}},
        {0,   0xEF, 1,  {0x08}},
        {0,   0xFF, 5,  {0x77, 0x01, 0x00, 0x00, 0x10}},
        {0,   0xC0, 2,  {0x4F, 0x00}},
        {0,   0xC1, 2,  {0x10, 0x0C}},
        {0,   0xC2, 2,  {0x01, 0x14}},
        {0,   0xCC, 1,  {0x10}},
        {0,   0xB0, 16, {0x00, 0x0B, 0x13, 0x0D, 0x10, 0x07, 0x02, 0x08, 0x07, 0x1F, 0x04, 0x11, 0x0F, 0x28, 0x2F, 0x1F}},
        {0,   0xB1, 16, {0x00, 0x0C, 0x13, 0x0C, 0x10, 0x05, 0x02, 0x08, 0x08, 0x1E, 0x05, 0x13, 0x11, 0x27, 0x30, 0x1F}},
        {0,   0xFF, 5,  {0x77, 0x01, 0x00, 0x00, 0x11}},
        {0,   0xB0, 1,  {0x4D}},
        {0,   0xB1, 1,  {0x4D}},
        {0,   0xB2, 1,  {0x87}},
        {0,   0xB3, 1,  {0x80}},
        {0,   0xB5, 1,  {0x45}},
        {0,   0xB7, 1,  {0x85}},
        {0,   0xB8, 1,  {0x20}},
        {0,   0xC0, 1,  {0x09}},
        {0,   0xC1, 1,  {0x78}},
        {0,   0xC2, 1,  {0x78}},
        {100, 0xD0, 1,  {0x88}},
        {0,   0xE0, 3,  {0x00, 0x00, 0x02}},
        {0,   0xE1, 11, {0x04, 0xB0, 0x06, 0xB0, 0x05, 0xB0, 0x07, 0xB0, 0x00, 0x44, 0x44}},
        {0,   0xE2, 12, {0x20, 0x20, 0x44, 0x44, 0x96, 0xA0, 0x00, 0x00, 0x96, 0xA0, 0x00, 0x00}},
        {0,   0xE3, 4,  {0x00, 0x00, 0x22, 0x22}},
        {0,   0xE4, 2,  {0x44, 0x44}},
        {0,   0xE5, 16, {0x0C, 0x90, 0xB0, 0xA0, 0x0E, 0x92, 0xB0, 0xA0, 0x08, 0x8C, 0xB0, 0xA0, 0x0A, 0x8E, 0xB0, 0xA0}},
        {0,   0xE6, 4,  {0x00, 0x00, 0x22, 0x22}},
        {0,   0xE7, 2,  {0x44, 0x44}},
        {0,   0xE8, 16, {0x0D, 0x91, 0xB0, 0xA0, 0x0F, 0x93, 0xB0, 0xA0, 0x09, 0x8D, 0xB0, 0xA0, 0x0B, 0x8F, 0xB0, 0xA0}},
        {0,   0xE9, 2,  {0x36, 0x00}},
        {0,   0xEB, 7,  {0x00, 0x00, 0xE4, 0xE4, 0x44, 0x88, 0x40}},
        {0,   0xED, 16, {0xC1, 0xA2, 0xBF, 0x0F, 0x67, 0x45, 0xFF, 0xFF, 0xFF, 0xFF, 0x54, 0x76, 0xF0, 0xFB, 0x2A, 0x1C}},
        {0,   0xEF, 6,  {0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}},
        {0,   0xFF, 5,  {0x77, 0x01, 0x00, 0x00, 0x00}},
        {120, 0x11, 0,  },
        {0,   0xFF, 5,  {0x77, 0x01, 0x00, 0x00, 0x13}},
        {10,  0xE8, 2,  {0x00, 0x0C}},
        {0,   0xE8, 2,  {0x00, 0x00}},
        {0,   0xFF, 5,  {0x77, 0x01, 0x00, 0x00, 0x00}},
//        {0,   0xFF, 5,  {0x77, 0x01, 0x00, 0x00, 0x12}},
//        {0,   0xD1, 14, {0x81, 0x08, 0x03, 0x20, 0x08, 0x01, 0xA0, 0x01, 0xE0, 0xA0, 0x01, 0xE0, 0x03, 0x20}},
//        {0,   0xD2, 1,  {0x08}},
        {20,  0x29, 0,  },
        //{150, 0x29, 0,  },
        //{0,   0x35, 1,  {0x00}},
    };

    struct timeval tv_start, tv_end;

    SAMPLE_PRT("SAMPLE_VO_INIT_MIPITx_Screen480x640 start!\n");

    for (i = 0; i < sizeof(mipi_initparam)/sizeof(MIPI_INITPARAM_S); i++)
    {
        unsigned char cmd[64] = {0};

        if (0 == mipi_initparam[i].num)
        {
            cmd_info.devno = 0;
            cmd_info.cmd_size = mipi_initparam[i].addr;
            cmd_info.data_type = 0x13;
            cmd_info.cmd = NULL;
        }
        else if (1 == mipi_initparam[i].num)
        {
            cmd_info.devno = 0;
            cmd_info.cmd_size = ((mipi_initparam[i].data[0]<<8)|mipi_initparam[i].addr);
            cmd_info.data_type = 0x23;
            cmd_info.cmd = NULL;
        }
        else
        {
            cmd[0] = mipi_initparam[i].addr;
            for (j = 0; j < mipi_initparam[i].num; j++)
            {
                cmd[j + 1] = mipi_initparam[i].data[j];
            }
            cmd_info.devno = 0;
            cmd_info.cmd_size = (mipi_initparam[i].num + 1);
            cmd_info.data_type = 0x29;
            cmd_info.cmd = cmd;
        }

        s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_USER_INIT_MIPITx failed!\n");;
            return s32Ret;
        }

        if (mipi_initparam[i].delay)
        {
            usleep(mipi_initparam[i].delay*1000);
            gettimeofday(&tv_end, NULL);
            int time_diff = (tv_end.tv_sec - tv_start.tv_sec)*1000000 + (tv_end.tv_usec - tv_start.tv_usec);
            printf("delay[%d]:%d,time_diff:%d\n",
                i, mipi_initparam[i].delay, time_diff);
        }
        gettimeofday(&tv_start, NULL);
    }

    SAMPLE_PRT("SAMPLE_VO_INIT_MIPITx_Screen480x640 success!\n");

    return SC_SUCCESS;
}


SC_S32 SAMPLE_VO_MIPITx_Screen480x640_GetStatus(void)
{
    SC_S32 s32Ret;

    unsigned char readdata[32] = {0};
    get_cmd_info_t getCmdInfo;
    getCmdInfo.devno = 0;
    getCmdInfo.data_type = 0x14;
    getCmdInfo.get_data = readdata;
    getCmdInfo.get_data_size = 1;
    getCmdInfo.data_param = 0x6;
    s32Ret = SC_MPI_VO_MipiTxSet(0, SC_MIPI_TX_GET_CMD, &getCmdInfo, sizeof(get_cmd_info_t));
    SAMPLE_PRT("s32Ret:%d get_data[0]:0x%x\n", s32Ret, getCmdInfo.get_data[0]);
    if ((!s32Ret) && (getCmdInfo.get_data[0] > 0))
    {
        SAMPLE_PRT("SAMPLE_VO_INIT_MIPITx_Screen480x640 success!\n");
        return SC_SUCCESS;
    }


    SAMPLE_PRT("SAMPLE_VO_INIT_MIPITx_Screen480x640 error!\n");

    return SC_FAILURE;
}

SC_S32 SAMPLE_VO_INIT_MIPITx_Screen720x1280(SC_S32 fd)
{
    SC_S32 i, j, k, s32Ret;
    cmd_info_t cmd_info;
    MIPI_INITPARAM_S mipi_initparam[] =
    {
        {0,   0xB9, 3,  {0xF1, 0x12, 0x83}},
        {0,   0xBA, 27, {0x33, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25,
                         0x00, 0x91, 0x0A, 0x00, 0x00, 0x02, 0x4F, 0xD1, 0x00, 0x00, 0x37}},
        {0,   0xB8, 1,  {0x25}},
        {0,   0xBF, 3,  {0x02, 0x10, 0x00}},
        {0,   0xB3, 10, {0x07, 0x0B, 0x1E, 0x1E, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00}},
        {0,   0xC0, 9,  {0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x08, 0x70, 0x00}},
        {0,   0xBC, 1,  {0x46}},
        {0,   0xCC, 1,  {0x0B}},
        {0,   0xB4, 1,  {0x80}},
        {0,   0xB2, 2,  {0xC8, 0x12}},
        {0,   0xE3, 14, {0x00, 0x07, 0x0B, 0x0B, 0x03, 0x0B, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x80, 0xC0, 0x14}},
        {0,   0xC1, 6,  {0x53, 0x00, 0x1E, 0x1E, 0x77, 0xF1}},
        {0,   0xB5, 2,  {0x09, 0x09}},
        {0,   0xB6, 2,  {0xB7, 0xB7}},
        {0,   0xE9, 63, {0xC2, 0x10, 0x09, 0x00, 0x00, 0x08, 0xEA, 0x12, 0x30, 0x00, 0x27, 0x85, 0x08, 0xEA, 0x27, 0x18,
                         0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xBA, 0x46, 0x02,
                         0x08, 0x28, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF8, 0xBA, 0x57, 0x13, 0x18, 0x38, 0x88, 0x88, 0x88,
                         0x88, 0x88, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {0,   0xEA, 61, {0x07, 0x12, 0x01, 0x01, 0x02, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xBA, 0x31, 0x75,
                         0x38, 0x18, 0x88, 0x88, 0x88, 0x88, 0x88, 0x8F, 0xBA, 0x20, 0x64, 0x28, 0x08, 0x88, 0x88, 0x88,
                         0x88, 0x88, 0x23, 0x10, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {0,   0xE0, 34, {0x00, 0x05, 0x09, 0x15, 0x0F, 0x3F, 0x2F, 0x2D, 0x05, 0x0A, 0x0C, 0x11, 0x13, 0x10, 0x10, 0x1C,
                         0x1F, 0x00, 0x03, 0x03, 0x17, 0x11, 0x3F, 0x2F, 0x2D, 0x05, 0x08, 0x0E, 0x0F, 0x13, 0x12, 0x16,
                         0x18, 0x1D}},
        {120, 0x11, 0,  },
        {50,  0x29, 0,  },

    };

    struct timeval tv_start, tv_end;

    SAMPLE_PRT("SAMPLE_VO_INIT_MIPITx_Screen720x1280 start!\n");

    for (i = 0; i < sizeof(mipi_initparam)/sizeof(MIPI_INITPARAM_S); i++)
    {
        unsigned char cmd[64] = {0};

        if (0 == mipi_initparam[i].num)
        {
            cmd_info.devno = 0;
            cmd_info.cmd_size = mipi_initparam[i].addr;
            cmd_info.data_type = 0x13;
            cmd_info.cmd = NULL;
        }
        else if (1 == mipi_initparam[i].num)
        {
            cmd_info.devno = 0;
            cmd_info.cmd_size = ((mipi_initparam[i].data[0]<<8)|mipi_initparam[i].addr);
            cmd_info.data_type = 0x23;
            cmd_info.cmd = NULL;
        }
        else
        {
            cmd[0] = mipi_initparam[i].addr;
            for (j = 0; j < mipi_initparam[i].num; j++)
            {
                cmd[j + 1] = mipi_initparam[i].data[j];
            }
            cmd_info.devno = 0;
            cmd_info.cmd_size = (mipi_initparam[i].num + 1);
            cmd_info.data_type = 0x29;
            cmd_info.cmd = cmd;
        }

        s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_USER_INIT_MIPITx failed!\n");;
            return s32Ret;
        }

        if (mipi_initparam[i].delay)
        {
            usleep(mipi_initparam[i].delay*1000);
            gettimeofday(&tv_end, NULL);
            int time_diff = (tv_end.tv_sec - tv_start.tv_sec)*1000000 + (tv_end.tv_usec - tv_start.tv_usec);
            //printf("delay[%d]:%d,time_diff:%d\n",
            //    i, mipi_initparam[i].delay, time_diff);
        }
        gettimeofday(&tv_start, NULL);
    }

    SAMPLE_PRT("SAMPLE_VO_INIT_MIPITx_Screen720x1280 success!\n");

    return SC_SUCCESS;
}

SC_S32 SAMPLE_VO_ENABLE_MIPITx(SC_S32 fd)
{
    SC_S32 s32Ret;
    //s32Ret = ioctl(fd, SC_MIPI_TX_ENABLE);

    s32Ret = SC_MPI_VO_MipiTxSet(0, SC_MIPI_TX_ENABLE, NULL, 0);
    if (s32Ret != SC_SUCCESS)
    {
        printf("MIPI_TX enable failed\n");
        return s32Ret;
    }

    return s32Ret;
}

SC_S32 SAMPLE_VO_CONFIG_MIPI(MIPILCD_TYPE type)
{
    SC_S32 fd = -1;
    SC_S32 s32Ret;
    /* SET MIPI BAKCLIGHT */

    /* CONFIG MIPI PINUMX */

    /* Reset MIPI */

#if 0
    /* OPEN MIPI FD */
    fd = SAMPLE_OPEN_MIPITx_FD();
    if (fd < 0)
    {
        return SC_FAILURE;
    }
#endif

    /* SET MIPI Tx Dev ATTR */
    s32Ret = SAMPLE_SET_MIPITx_Dev_ATTR(fd, type);
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    /* CONFIG MIPI Tx INITIALIZATION SEQUENCE */
    if (type == MIPILCD_800x1280)
    {
        s32Ret = SAMPLE_VO_INIT_MIPITx_Screen800x1280(fd);
        if (s32Ret != SC_SUCCESS)
        {
            return s32Ret;
        }
    }
    else if (type == MIPILCD_480x640)
    {
        static int flag = 0;
        if (!flag)
        {
            flag = 1;
            s32Ret = SAMPLE_VO_INIT_MIPITx_Screen480x640(fd);
            if (s32Ret != SC_SUCCESS)
            {
                return s32Ret;
            }
        }
    }
    else if (type == MIPILCD_720x1280)
    {
        s32Ret = SAMPLE_VO_INIT_MIPITx_Screen720x1280(fd);
        if (s32Ret != SC_SUCCESS)
        {
            return s32Ret;
        }
    }
    else
    {
        SAMPLE_PRT("SAMPLE_VO_CONFIG_MIPI error, type:%d\n", type);
        return SC_FAILURE;
    }

    /* ENABLE MIPI Tx DEV */
    s32Ret = SAMPLE_VO_ENABLE_MIPITx(fd);
    if (s32Ret != SC_SUCCESS)
    {
        return s32Ret;
    }

    return SC_SUCCESS;
}

SC_VOID SAMPLE_VO_GetUserPubBaseAttr(VO_PUB_ATTR_S *pstPubAttr)
{
    pstPubAttr->u32BgColor = COLOR_RGB_BLUE;
    pstPubAttr->enIntfSync = VO_OUTPUT_USER;
    pstPubAttr->stSyncInfo.bSynm = 0;
    pstPubAttr->stSyncInfo.u8Intfb = 0;
    pstPubAttr->stSyncInfo.bIop = 1;

    pstPubAttr->stSyncInfo.u16Hmid = 1;
    pstPubAttr->stSyncInfo.u16Bvact = 1;
    pstPubAttr->stSyncInfo.u16Bvbb = 1;
    pstPubAttr->stSyncInfo.u16Bvfb = 1;

    pstPubAttr->stSyncInfo.bIdv = 0;
    pstPubAttr->stSyncInfo.bIhs = 0;
    pstPubAttr->stSyncInfo.bIvs = 0;

    return;
}

SC_VOID SAMPLE_VO_GetUserLayerAttr(VO_VIDEO_LAYER_ATTR_S *pstLayerAttr, SIZE_S  *pstDevSize)
{
    pstLayerAttr->bClusterMode = SC_FALSE;
    pstLayerAttr->bDoubleFrame = SC_FALSE;
    pstLayerAttr->enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    pstLayerAttr->enPixFormat = PIXEL_FORMAT_YVU_PLANAR_420;

    pstLayerAttr->stDispRect.s32X = 0;
    pstLayerAttr->stDispRect.s32Y = 0;
    pstLayerAttr->stDispRect.u32Height = pstDevSize->u32Height;
    pstLayerAttr->stDispRect.u32Width  = pstDevSize->u32Width;

    pstLayerAttr->stImageSize.u32Height = pstDevSize->u32Height;
    pstLayerAttr->stImageSize.u32Width = pstDevSize->u32Width;

    return;
}

SC_VOID SAMPLE_VO_GetUserChnAttr(VO_CHN_ATTR_S *pstChnAttr, SIZE_S *pstDevSize, SC_S32 VoChnNum)
{
    SC_S32 i;
    for (i = 0; i < VoChnNum; i++)
    {
        pstChnAttr[i].bDeflicker = SC_FALSE;
        pstChnAttr[i].u32Priority = 0;
        pstChnAttr[i].stRect.s32X = 0;
        pstChnAttr[i].stRect.s32Y = 0;
        pstChnAttr[i].stRect.u32Height = pstDevSize->u32Height;
        pstChnAttr[i].stRect.u32Width = pstDevSize->u32Width;
    }

    return;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
