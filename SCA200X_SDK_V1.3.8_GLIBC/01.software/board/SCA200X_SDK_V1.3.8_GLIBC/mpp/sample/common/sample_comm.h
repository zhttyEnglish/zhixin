
#ifndef __SAMPLE_COMM_H__
#define __SAMPLE_COMM_H__

#include <pthread.h>

#include "sc_common.h"
#include "sc_buffer.h"
#include "sc_defines.h"
#include "sc_mipi.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vdec.h"
#include "mpi_vpss.h"
//#include "mpi_avs.h"
#include "mpi_region.h"
#include "mpi_audio.h"
#include "mpi_isp.h"
//#include "mpi_ae.h"
//#include "mpi_awb.h"
#include "sc_math.h"
#include "sc_sns_ctrl.h"
//#include "mpi_hdmi.h"
#include "mpi_vgs.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/*******************************************************
    macro define
*******************************************************/
#define FILE_NAME_LEN               128

#define CHECK_CHN_RET(express,Chn,name)\
    do{\
        SC_S32 Ret;\
        Ret = express;\
        if (SC_SUCCESS != Ret)\
        {\
            printf("\033[0;31m%s chn %d failed at %s: LINE: %d with %#x!\033[0;39m\n", name, Chn, __FUNCTION__, __LINE__, Ret);\
            fflush(stdout);\
            return Ret;\
        }\
    }while(0)

#define CHECK_RET(express,name)\
    do{\
        SC_S32 Ret;\
        Ret = express;\
        if (SC_SUCCESS != Ret)\
        {\
            printf("\033[0;31m%s failed at %s: LINE: %d with %#x!\033[0;39m\n", name, __FUNCTION__, __LINE__, Ret);\
            return Ret;\
        }\
    }while(0)
#define SAMPLE_PIXEL_FORMAT         PIXEL_FORMAT_YVU_PLANAR_420

#define TLV320_FILE "/dev/tlv320aic31"
#define COLOR_RGB_RED      0xFF0000
#define COLOR_RGB_GREEN    0x00FF00
#define COLOR_RGB_BLUE     0x0000FF
#define COLOR_RGB_BLACK    0x000000
#define COLOR_RGB_YELLOW   0xFFFF00
#define COLOR_RGB_CYN      0x00ffff
#define COLOR_RGB_WHITE    0xffffff

#define SAMPLE_VO_DEV_DHD0 0                  /* VO's device HD0 */
#define SAMPLE_VO_DEV_DHD1 1                  /* VO's device HD1 */
#define SAMPLE_VO_DEV_UHD  SAMPLE_VO_DEV_DHD0 /* VO's ultra HD device:HD0 */
#define SAMPLE_VO_DEV_HD   SAMPLE_VO_DEV_DHD1 /* VO's HD device:HD1 */
#define SAMPLE_VO_LAYER_VHD0 0
#define SAMPLE_VO_LAYER_VHD1 1
#define SAMPLE_VO_LAYER_VHD2 2
#define SAMPLE_VO_LAYER_PIP  SAMPLE_VO_LAYER_VHD2

#define SAMPLE_AUDIO_EXTERN_AI_DEV 0
#define SAMPLE_AUDIO_EXTERN_AO_DEV 0
#define SAMPLE_AUDIO_INNER_AI_DEV 0
#define SAMPLE_AUDIO_INNER_AO_DEV 0
#define SAMPLE_AUDIO_INNER_HDMI_AO_DEV 1

#define SAMPLE_AUDIO_PTNUMPERFRM   480

#define WDR_MAX_PIPE_NUM        4

#define PAUSE()  do {\
        printf("---------------press Enter key to exit!---------------\n");\
        getchar();\
    } while (0)

#define SAMPLE_PRT(fmt...)   \
    do {\
        printf("[%s]-%d: ", __FUNCTION__, __LINE__);\
        printf(fmt);\
    }while(0)

#define CHECK_NULL_PTR(ptr)\
    do{\
        if(NULL == ptr)\
        {\
            printf("func:%s,line:%d, NULL pointer\n",__FUNCTION__,__LINE__);\
            return SC_FAILURE;\
        }\
    }while(0)

/*******************************************************
    enum define
*******************************************************/

typedef enum scPIC_SIZE_E
{
    PIC_CIF,
    PIC_360P,      /* 640 * 360 */
    PIC_D1_PAL,    /* 720 * 576 */
    PIC_D1_NTSC,   /* 720 * 480 */
    PIC_720P,      /* 1280 * 720  */
    PIC_1080P,     /* 1920 * 1080 */
    PIC_2560x1440,
    PIC_2592x1520,
    PIC_2592x1944,
    PIC_3840x2160,
    PIC_3864x2192,
    PIC_4096x2160,
    PIC_3000x3000,
    PIC_4000x3000,
    PIC_4096x3120,
    PIC_7680x4320,
    PIC_3840x8640,
    PIC_BUTT
} PIC_SIZE_E;

typedef enum scSAMPLE_SNS_TYPE_E
{
    SONY_IMX307_MIPI_2M_30FPS_12BIT,
    SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1,
    SONY_IMX415_MIPI_8M_30FPS_12BIT,
    SONY_IMX415_MIPI_8M_60FPS_12BIT,
    SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1,
    SMART_SC2310_MIPI_2M_30FPS_12BIT,
    SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1,
    GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT,
    GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT,
    GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1,
    SMART_SC2210_MIPI_2M_30FPS_12BIT,
    GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT,
    GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT,
    SMART_SC4210_MIPI_4M_30FPS_12BIT,
    SONY_IMX291_MIPI_2M_30FPS_12BIT,
    SONY_IMX291_MIPI_2M_60FPS_10BIT,
    SONY_IMX291_MIPI_2M_120FPS_10BIT,
    OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT,
    OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT,
    ISP_VIN_0_MEM_2M_30FPS_12BIT,
    TP9930_DVP_4VC_1080P_25FPS,
    SAMPLE_SNS_TYPE_BUTT,
} SAMPLE_SNS_TYPE_E;

typedef enum scSAMPLE_VO_MODE_E
{
    VO_MODE_1MUX,
    VO_MODE_2MUX,
    VO_MODE_4MUX,
    VO_MODE_8MUX,
    VO_MODE_9MUX,
    VO_MODE_16MUX,
    VO_MODE_25MUX,
    VO_MODE_36MUX,
    VO_MODE_49MUX,
    VO_MODE_64MUX,
    VO_MODE_2X4,
    VO_MODE_BUTT
} SAMPLE_VO_MODE_E;

typedef enum scSAMPLE_RC_E
{
    SAMPLE_RC_CBR = 0,
    SAMPLE_RC_VBR,
    SAMPLE_RC_AVBR,
    SAMPLE_RC_QPMAP,
    SAMPLE_RC_FIXQP
} SAMPLE_RC_E;

typedef enum
{
    MIPILCD_1024x600 = 0,
    MIPILCD_800x1280,
    MIPILCD_480x640,
    MIPILCD_720x1280,
    MIPILCD_BUTT
} MIPILCD_TYPE;

/*******************************************************
    structure define
*******************************************************/
typedef struct scSAMPLE_VENC_GETSTREAM_PARA_S
{
    SC_BOOL bThreadStart;
    SC_BOOL saveFile;
    VENC_CHN VeChn[VENC_MAX_CHN_NUM];
    SC_S32  s32Cnt;
} SAMPLE_VENC_GETSTREAM_PARA_S;

typedef struct scSAMPLE_VENC_QPMAP_SENDFRAME_PARA_S
{
    SC_BOOL  bThreadStart;
    //VPSS_GRP VpssGrp;
    //VPSS_CHN VpssChn;
    VENC_CHN VeChn[VENC_MAX_CHN_NUM];
    SC_S32   s32Cnt;
    SIZE_S   stSize;
} SAMPLE_VENC_QPMAP_SENDFRAME_PARA_S;

typedef struct scSAMPLE_VI_DUMP_THREAD_INFO_S
{
    VI_PIPE     ViPipe;
    SC_S32      s32Cnt;
    SC_BOOL     bDump;
    SC_CHAR     aszName[128];
    pthread_t   ThreadId;
} SAMPLE_VI_DUMP_THREAD_INFO_S;

typedef struct scSAMPLE_SENSOR_INFO_S
{
    SAMPLE_SNS_TYPE_E   enSnsType;
    SC_S32              s32SnsId;
    SC_S32              s32BusId;
    combo_dev_t           MipiDev;
} SAMPLE_SENSOR_INFO_S;

typedef struct scSAMPLE_SNAP_INFO_S
{
    SC_BOOL  bSnap;
    SC_BOOL  bDoublePipe;
    VI_PIPE    VideoPipe;
    VI_PIPE    SnapPipe;
    VI_VPSS_MODE_E  enVideoPipeMode;
    VI_VPSS_MODE_E  enSnapPipeMode;
} SAMPLE_SNAP_INFO_S;

typedef struct scSAMPLE_DEV_INFO_S
{
    VI_DEV      ViDev;
    WDR_MODE_E  enWDRMode;
} SAMPLE_DEV_INFO_S;

typedef struct scSAMPLE_PIPE_INFO_S
{
    VI_PIPE         aPipe[WDR_MAX_PIPE_NUM];
    VI_VPSS_MODE_E  enMastPipeMode;
    SC_BOOL         bMultiPipe;
    SC_BOOL         bVcNumCfged;
    SC_U32          u32VCNum[WDR_MAX_PIPE_NUM];
} SAMPLE_PIPE_INFO_S;

typedef struct scSAMPLE_CHN_INFO_S
{
    VI_CHN              ViChn;
    PIXEL_FORMAT_E      enPixFormat;
    DYNAMIC_RANGE_E     enDynamicRange;
    VIDEO_FORMAT_E      enVideoFormat;
    COMPRESS_MODE_E     enCompressMode;
    SIZE_S              stSize;
    SC_U32              u32Align;
    SC_BOOL             bMirror;
    SC_BOOL             bFlip;
    FRAME_RATE_CTRL_S   stFps;
} SAMPLE_CHN_INFO_S;

typedef struct scSAMPLE_VI_INFO_S
{
    SAMPLE_SENSOR_INFO_S    stSnsInfo;
    SAMPLE_DEV_INFO_S       stDevInfo;
    SAMPLE_PIPE_INFO_S      stPipeInfo;
    SAMPLE_CHN_INFO_S       stChnInfo;
    SAMPLE_SNAP_INFO_S      stSnapInfo;
} SAMPLE_VI_INFO_S;

typedef struct scSAMPLE_VI_CONFIG_S
{
    SAMPLE_VI_INFO_S    astViInfo[VI_MAX_DEV_NUM];
    SC_S32              as32WorkingViId[VI_MAX_DEV_NUM];
    SC_S32              s32WorkingViNum;
} SAMPLE_VI_CONFIG_S;

typedef struct scSAMPLE_VI_FRAME_CONFIG_S
{
    SC_U32                  u32Width;
    SC_U32                  u32Height;
    SC_U32                  u32ByteAlign;
    PIXEL_FORMAT_E          enPixelFormat;
    VIDEO_FORMAT_E          enVideoFormat;
    COMPRESS_MODE_E         enCompressMode;
    DYNAMIC_RANGE_E         enDynamicRange;
} SAMPLE_VI_FRAME_CONFIG_S;

typedef struct scSAMPLE_VI_FRAME_INFO_S
{
    VB_BLK             VbBlk;
    SC_U32             u32Size;
    VIDEO_FRAME_INFO_S stVideoFrameInfo;
} SAMPLE_VI_FRAME_INFO_S;

typedef struct scSAMPLE_VI_FPN_CALIBRATE_INFO_S
{
    SC_U32                  u32Threshold;
    SC_U32                  u32FrameNum;
    //ISP_FPN_TYPE_E          enFpnType;
    PIXEL_FORMAT_E          enPixelFormat;
    COMPRESS_MODE_E         enCompressMode;
} SAMPLE_VI_FPN_CALIBRATE_INFO_S;

typedef struct scSAMPLE_VI_FPN_CORRECTION_INFO_S
{
    ISP_OP_TYPE_E           enOpType;
    //ISP_FPN_TYPE_E          enFpnType;
    SC_U32                  u32Strength;
    PIXEL_FORMAT_E          enPixelFormat;
    COMPRESS_MODE_E         enCompressMode;
    SAMPLE_VI_FRAME_INFO_S  stViFrameInfo;
} SAMPLE_VI_FPN_CORRECTION_INFO_S;

typedef struct tag_SAMPLE_VO_WBC_CONFIG
{
    VO_WBC_SOURCE_TYPE_E    enSourceType;
    DYNAMIC_RANGE_E         enDynamicRange;
    COMPRESS_MODE_E         enCompressMode;
    SC_S32 s32Depth;

    SC_S32                  VoWbc;
    VO_WBC_ATTR_S           stWbcAttr;
    VO_WBC_SOURCE_S         stWbcSource;
    VO_WBC_MODE_E           enWbcMode;

} SAMPLE_VO_WBC_CONFIG;

typedef struct scSAMPLE_COMM_VO_LAYER_CONFIG_S
{
    /* for layer */
    VO_LAYER                VoLayer;
    VO_INTF_SYNC_E          enIntfSync;
    RECT_S                  stDispRect;
    SIZE_S                  stImageSize;
    PIXEL_FORMAT_E          enPixFormat;

    SC_U32                  u32DisBufLen;
    DYNAMIC_RANGE_E         enDstDynamicRange;

    /* for chn */
    SAMPLE_VO_MODE_E        enVoMode;
} SAMPLE_COMM_VO_LAYER_CONFIG_S;

typedef struct scSAMPLE_VO_CONFIG_S
{
    /* for device */
    VO_DEV                  VoDev;
    VO_INTF_TYPE_E          enVoIntfType;
    VO_INTF_SYNC_E          enIntfSync;
    PIC_SIZE_E              enPicSize;
    SC_U32                  u32BgColor;

    /* for layer */
    PIXEL_FORMAT_E          enPixFormat;
    RECT_S                  stDispRect;
    SIZE_S                  stImageSize;
    VO_PART_MODE_E          enVoPartMode;

    SC_U32                  u32DisBufLen;
    DYNAMIC_RANGE_E         enDstDynamicRange;

    /* for chnnel */
    SAMPLE_VO_MODE_E        enVoMode;
} SAMPLE_VO_CONFIG_S;

typedef enum scTHREAD_CONTRL_E
{
    THREAD_CTRL_START,
    THREAD_CTRL_PAUSE,
    THREAD_CTRL_STOP,
} THREAD_CONTRL_E;

typedef struct scVDEC_THREAD_PARAM_S
{
    SC_S32 s32ChnId;
    PAYLOAD_TYPE_E enType;
    SC_CHAR cFilePath[128];
    SC_CHAR cFileName[128];
    SC_S32 s32StreamMode;
    SC_S32 s32MilliSec;
    SC_S32 s32MinBufSize;
    SC_S32 s32IntervalTime;
    THREAD_CONTRL_E eThreadCtrl;
    SC_U64  u64PtsInit;
    SC_U64  u64PtsIncrease;
    SC_BOOL bCircleSend;
} VDEC_THREAD_PARAM_S;

typedef struct scSAMPLE_VDEC_BUF
{
    SC_U32  u32PicBufSize;
    SC_U32  u32TmvBufSize;
    SC_BOOL bPicBufAlloc;
    SC_BOOL bTmvBufAlloc;
} SAMPLE_VDEC_BUF;

typedef struct scSAMPLE_VDEC_VIDEO_ATTR
{
    VIDEO_DEC_MODE_E enDecMode;
    SC_U32              u32RefFrameNum;
    DATA_BITWIDTH_E  enBitWidth;
} SAMPLE_VDEC_VIDEO_ATTR;

typedef struct scSAMPLE_VDEC_PICTURE_ATTR
{
    PIXEL_FORMAT_E enPixelFormat;
    SC_U32         u32Alpha;
} SAMPLE_VDEC_PICTURE_ATTR;

typedef struct scSAMPLE_VDEC_ATTR
{
    PAYLOAD_TYPE_E enType;
    VIDEO_MODE_E   enMode;
    SC_U32 u32Width;
    SC_U32 u32Height;
    SC_U32 u32FrameBufCnt;
    SC_U32 u32DisplayFrameNum;
    union
    {
        SAMPLE_VDEC_VIDEO_ATTR stSapmleVdecVideo;      /* structure with video ( h265/h264) */
        SAMPLE_VDEC_PICTURE_ATTR stSapmleVdecPicture; /* structure with picture (jpeg/mjpeg )*/
    };
} SAMPLE_VDEC_ATTR;

typedef struct scSAMPLE_VB_BASE_INFO_S
{
    PIXEL_FORMAT_E      enPixelFormat;
    SC_U32              u32Width;
    SC_U32              u32Height;
    SC_U32              u32Align;
    COMPRESS_MODE_E     enCompressMode;
} SAMPLE_VB_BASE_INFO_S;

typedef struct scSAMPLE_VB_CAL_CONFIG_S
{
    SC_U32 u32VBSize;

    SC_U32 u32HeadStride;
    SC_U32 u32HeadSize;
    SC_U32 u32HeadYSize;

    SC_U32 u32MainStride;
    SC_U32 u32MainSize;
    SC_U32 u32MainYSize;

    SC_U32 u32ExtStride;
    SC_U32 u32ExtYSize;
} SAMPLE_VB_CAL_CONFIG_S;

/*******************************************************
    function announce
*******************************************************/

SC_VOID *SAMPLE_SYS_IOMmap(SC_U64 u64PhyAddr, SC_U32 u32Size);
SC_S32 SAMPLE_SYS_Munmap(SC_VOID *pVirAddr, SC_U32 u32Size);
SC_S32 SAMPLE_SYS_SetReg(SC_U64 u64Addr, SC_U32 u32Value);
SC_S32 SAMPLE_SYS_GetReg(SC_U64 u64Addr, SC_U32 *pu32Value);

SC_S32 SAMPLE_COMM_SYS_GetPicSize(PIC_SIZE_E enPicSize, SIZE_S *pstSize);
SC_S32 SAMPLE_COMM_SYS_GetPicSizeEnum(SIZE_S stSize, PIC_SIZE_E *enPicSize);
SC_S32 SAMPLE_COMM_SYS_MemConfig(SC_VOID);
SC_VOID SAMPLE_COMM_SYS_Exit(void);
SC_S32 SAMPLE_COMM_SYS_Init(VB_CONFIG_S *pstVbConfig);
SC_S32 SAMPLE_COMM_SYS_InitWithVbSupplement(VB_CONFIG_S *pstVbConf, SC_U32 u32SupplementConfig);

SC_S32 SAMPLE_COMM_VI_Bind_VO(VI_PIPE ViPipe, VI_CHN ViChn, VO_LAYER VoLayer, VO_CHN VoChn);
SC_S32 SAMPLE_COMM_VI_UnBind_VO(VI_PIPE ViPipe, VI_CHN ViChn, VO_LAYER VoLayer, VO_CHN VoChn);
SC_S32 SAMPLE_COMM_VI_Bind_VENC(VI_PIPE ViPipe, VI_CHN ViChn, VENC_CHN VencChn);
SC_S32 SAMPLE_COMM_VI_UnBind_VENC(VI_PIPE ViPipe, VI_CHN ViChn, VENC_CHN VencChn);
SC_S32 SAMPLE_COMM_VO_Bind_VO(VO_LAYER  SrcVoLayer, VO_CHN SrcVoChn, VO_LAYER DstVoLayer, VO_CHN DstVoChn);
SC_S32 SAMPLE_COMM_VO_UnBind_VO(VO_LAYER DstVoLayer, VO_CHN DstVoChn);

SC_S32 SAMPLE_COMM_AI_Bind_AO(    AUDIO_DEV AiDev, SC_S32 AiChn, AUDIO_DEV AoDev, SC_S32 AoChn);
SC_S32 SAMPLE_COMM_AI_UnBind_AO(AUDIO_DEV AiDev, SC_S32 AiChn, AUDIO_DEV AoDev, SC_S32 AoChn);
SC_S32 SAMPLE_COMM_AI_Bind_AENC(    AUDIO_DEV AiDev, SC_S32 AiChn, AUDIO_DEV AencDev, SC_S32 AencChn);
SC_S32 SAMPLE_COMM_AI_UnBind_AENC(AUDIO_DEV AiDev, SC_S32 AiChn, AUDIO_DEV AencDev, SC_S32 AencChn);
SC_S32 SAMPLE_COMM_ADEC_Bind_AO(    AUDIO_DEV AdecDev, SC_S32 AdecChn, AUDIO_DEV AoDev, SC_S32 AoChn);
SC_S32 SAMPLE_COMM_ADEC_UnBind_AO(    AUDIO_DEV AdecDev, SC_S32 AdecChn, AUDIO_DEV AoDev, SC_S32 AoChn);

SC_VOID SAMPLE_COMM_ISP_Stop(ISP_DEV IspDev);
SC_VOID SAMPLE_COMM_All_ISP_Stop(SC_VOID);
SC_S32 SAMPLE_COMM_ISP_Run(ISP_DEV IspDev);
SC_S32 SAMPLE_COMM_ISP_BindSns(ISP_DEV IspDev, SC_U32 u32SnsId, SAMPLE_SNS_TYPE_E enSnsType, SC_S8 s8SnsDev);
SC_S32 SAMPLE_COMM_ISP_Sensor_Regiter_callback(ISP_DEV IspDev, SC_U32 u32SnsId);
SC_S32 SAMPLE_COMM_ISP_Sensor_UnRegiter_callback(ISP_DEV IspDev);
SC_S32 SAMPLE_COMM_ISP_Aelib_Callback(ISP_DEV IspDev);
SC_S32 SAMPLE_COMM_ISP_Aelib_UnCallback(ISP_DEV IspDev);
SC_S32 SAMPLE_COMM_ISP_Awblib_Callback(ISP_DEV IspDev);
SC_S32 SAMPLE_COMM_ISP_Awblib_UnCallback(ISP_DEV IspDev);
SC_S32 SAMPLE_COMM_ISP_GetIspAttrBySns(SAMPLE_SNS_TYPE_E enSnsType, ISP_PUB_ATTR_S *pstPubAttr);
SC_VOID SAMPLE_COMM_ISP_SetSnsType(SC_U32 u32SnsId, SAMPLE_SNS_TYPE_E enSnsType);

SC_S32 SAMPLE_COMM_VI_GetWDRModeBySensor(SAMPLE_SNS_TYPE_E enMode, WDR_MODE_E *penWDRMode);
SC_S32 SAMPLE_COMM_VI_GetPipeBySensor(SAMPLE_SNS_TYPE_E enMode, SAMPLE_PIPE_INFO_S *pstPipeInfo);
SC_S32 SAMPLE_COMM_VI_GetSizeBySensor(SAMPLE_SNS_TYPE_E enMode, PIC_SIZE_E *penSize);
SC_S32 SAMPLE_COMM_VI_GetRawPicSize(SAMPLE_SNS_TYPE_E enMode, SIZE_S *pstSize);
SC_S32 SAMPLE_COMM_VI_GetFrameRateBySensor(SAMPLE_SNS_TYPE_E enMode, SC_U32 *pu32FrameRate);
SC_S32 SAMPLE_COMM_VI_StartDev(SAMPLE_VI_INFO_S *pstViInfo);
SC_S32 SAMPLE_COMM_VI_StartChn(VI_CHN ViChn, RECT_S *pstCapRect, SIZE_S *pstTarSize, SAMPLE_VI_CONFIG_S *pstViConfig);
SC_S32 SAMPLE_COMM_VI_StartMIPI(SAMPLE_VI_CONFIG_S *pstViConfig);
SC_S32 SAMPLE_COMM_VI_StartVi(SAMPLE_VI_CONFIG_S *pstViConfig);
SC_S32 SAMPLE_COMM_VI_StopVi(SAMPLE_VI_CONFIG_S *pstViConfig);
SC_S32 SAMPLE_COMM_VI_SetMipiAttr(SAMPLE_VI_CONFIG_S *pstViConfig);
SC_S32 SAMPLE_COMM_VI_GetDevAttrBySns(VI_DEV ViDev, SAMPLE_SNS_TYPE_E enSnsType, VI_DEV_ATTR_S *pstViDevAttr);
SC_VOID SAMPLE_COMM_VI_GetSensorInfo(SAMPLE_VI_CONFIG_S *pstViConfig);

combo_dev_t SAMPLE_COMM_VI_GetComboDevBySensor(SAMPLE_SNS_TYPE_E enMode, SC_S32 s32SnsIdx);
SC_S32 SAMPLE_COMM_VI_SaveRaw(VIDEO_FRAME_S *pVBuf, SC_U32 u32Nbit, FILE *pfd);
SC_VOID *SAMPLE_COMM_VI_DumpRaw(SC_VOID *arg);
SC_S32 SAMPLE_COMM_VI_StartDumpRawThread(VI_PIPE ViPipe, SC_S32 s32Cnt, const SC_CHAR *pzsName);
SC_S32 SAMPLE_COMM_VI_StopDumpRawThread(SC_VOID);
SC_S32 SAMPLE_COMM_VI_SetParam(SAMPLE_VI_CONFIG_S *pstViConfig);
SC_S32  SAMPLE_COMM_VI_SwitchMode_StopVI(SAMPLE_VI_CONFIG_S *pstViConfigSrc);
SC_S32  SAMPLE_COMM_VI_SwitchMode(SAMPLE_VI_CONFIG_S *pstViConfigDes);

SC_S32 SAMPLE_COMM_VI_FpnCalibrateConfig(VI_PIPE ViPipe, SAMPLE_VI_FPN_CALIBRATE_INFO_S *pstViFpnCalibrateInfo);
SC_S32 SAMPLE_COMM_VI_FpnCorrectionConfig(VI_PIPE ViPipe, SAMPLE_VI_FPN_CORRECTION_INFO_S *pstViFpnCorrectionInfo);
SC_S32 SAMPLE_COMM_VI_DisableFpnCorrection(VI_PIPE ViPipe, SAMPLE_VI_FPN_CORRECTION_INFO_S *pstViFpnCorrectionInfo);

SC_S32 SAMPLE_COMM_VI_Load_UserPic(const char *pszYuvFile, VI_USERPIC_ATTR_S *pstUsrPic,
    SAMPLE_VI_FRAME_INFO_S *pstViFrameInfo);
SC_VOID SAMPLE_COMM_VI_Release_UserPic(SAMPLE_VI_FRAME_INFO_S *pstViFrameInfo);

SC_S32 SAMPLE_COMM_VO_GetWH(VO_INTF_SYNC_E enIntfSync, SC_U32 *pu32W, SC_U32 *pu32H, SC_U32 *pu32Frm);
SC_S32 SAMPLE_COMM_VO_MemConfig(VO_DEV VoDev, SC_CHAR *pcMmzName);
SC_S32 SAMPLE_COMM_VO_StartDev(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr);
SC_S32 SAMPLE_COMM_VO_StopDev(VO_DEV VoDev);
SC_S32 SAMPLE_COMM_VO_StartLayer(VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);
SC_S32 SAMPLE_COMM_VO_StopLayer(VO_LAYER VoLayer);
SC_S32 SAMPLE_COMM_VO_StartChn(VO_LAYER VoLayer, SAMPLE_VO_MODE_E enMode);
SC_S32 SAMPLE_COMM_VO_StopChn(VO_LAYER VoLayer, SAMPLE_VO_MODE_E enMode);
SC_S32 SAMPLE_COMM_VO_StartWBC(SAMPLE_VO_WBC_CONFIG *pWbcConfig);
SC_S32 SAMPLE_COMM_VO_StopWBC(SAMPLE_VO_WBC_CONFIG *pWbcConfig);
SC_S32 SAMPLE_COMM_VO_GetDefWBCConfig(SAMPLE_VO_WBC_CONFIG *pWbcConfig);
SC_S32 SAMPLE_COMM_VO_BindVi(VO_LAYER VoLayer, VO_CHN VoChn, VI_CHN ViChn);
SC_S32 SAMPLE_COMM_VO_UnBindVi(VO_LAYER VoLayer, VO_CHN VoChn);
SC_S32 SAMPLE_COMM_VO_HdmiStart(VO_INTF_SYNC_E enIntfSync);
SC_S32 SAMPLE_COMM_VO_HdmiStartByDyRg(VO_INTF_SYNC_E enIntfSync, DYNAMIC_RANGE_E enDyRg);
SC_S32 SAMPLE_COMM_VO_HdmiStop(SC_VOID);
SC_S32 SAMPLE_COMM_VO_GetDefConfig(SAMPLE_VO_CONFIG_S *pstVoConfig);
SC_S32 SAMPLE_COMM_VO_StopVO(SAMPLE_VO_CONFIG_S *pstVoConfig);
SC_S32 SAMPLE_COMM_VO_StartVO(SAMPLE_VO_CONFIG_S *pstVoConfig);
SC_S32 SAMPLE_COMM_VO_StopPIP(SAMPLE_VO_CONFIG_S *pstVoConfig);
SC_S32 SAMPLE_COMM_VO_StartPIP(SAMPLE_VO_CONFIG_S *pstVoConfig);
SC_S32 SAMPLE_COMM_VO_GetDefLayerConfig(SAMPLE_COMM_VO_LAYER_CONFIG_S *pstVoLayerConfig);
SC_S32 SAMPLE_COMM_VO_StartLayerChn(SAMPLE_COMM_VO_LAYER_CONFIG_S *pstVoLayerConfig);
SC_S32 SAMPLE_COMM_VO_StopLayerChn(SAMPLE_COMM_VO_LAYER_CONFIG_S *pstVoLayerConfig);
SC_VOID SAMPLE_COMM_VO_Exit(SC_VOID);
//0--hdmi, 1--mipi + rgb
SC_S32 SAMPLE_COMM_VO_MIPIRGB_HDMI_720x1280(int type);
SC_S32 SAMPLE_VO_CONFIG_MIPI(MIPILCD_TYPE type);
SC_S32 SAMPLE_VO_MIPITx_Screen480x640_GetStatus(void);
SC_VOID SAMPLE_VO_GetUserPubBaseAttr(VO_PUB_ATTR_S *pstPubAttr);
SC_VOID SAMPLE_VO_GetUserLayerAttr(VO_VIDEO_LAYER_ATTR_S *pstLayerAttr, SIZE_S  *pstDevSize);
SC_VOID SAMPLE_VO_GetUserChnAttr(VO_CHN_ATTR_S *pstChnAttr, SIZE_S *pstDevSize, SC_S32 VoChnNum);

SC_S32 SAMPLE_COMM_VENC_MemConfig(SC_VOID);
SC_S32 SAMPLE_COMM_VENC_Creat(VENC_CHN VencChn, PAYLOAD_TYPE_E enType,  PIC_SIZE_E enSize, SAMPLE_RC_E enRcMode,
    SC_U32  u32Profile, VENC_GOP_ATTR_S *pstGopAttr);
SC_S32 SAMPLE_COMM_VENC_Start(VENC_CHN VencChn, PAYLOAD_TYPE_E enType, PIC_SIZE_E enSize, SAMPLE_RC_E enRcMode,
    SC_U32 u32Profile, VENC_GOP_ATTR_S *pstGopAttr);
SC_S32 SAMPLE_COMM_VENC_Stop(VENC_CHN VencChn);
SC_S32 SAMPLE_COMM_VENC_SnapStart(VENC_CHN VencChn, SIZE_S *pstSize, SC_BOOL bSupportDCF);
SC_S32 SAMPLE_COMM_VENC_SnapProcess(VENC_CHN VencChn, SC_U32 SnapCnt, SC_BOOL bSaveJpg, SC_BOOL bSaveThm);
SC_S32 SAMPLE_COMM_VENC_SaveJpeg(VENC_CHN VencChn, SC_U32 SnapCnt);
SC_S32 SAMPLE_COMM_VENC_SnapStop(VENC_CHN VencChn);
SC_S32 SAMPLE_COMM_VENC_StartGetStream(VENC_CHN VeChn[], SC_S32 s32Cnt);
SC_S32 SAMPLE_COMM_VENC_StartGetStreamNoFile(VENC_CHN VeChn[], SC_S32 s32Cnt);
SC_S32 SAMPLE_COMM_VENC_StopGetStream(void);
SC_S32 SAMPLE_COMM_VENC_StartGetStream_Svc_t(SC_S32 s32Cnt);
SC_S32 SAMPLE_COMM_VENC_GetGopAttr(VENC_GOP_MODE_E enGopMode, VENC_GOP_ATTR_S *pstGopAttr);
SC_S32 SAMPLE_COMM_VENC_StopSendQpmapFrame(void);
SC_S32 SAMPLE_COMM_VPSS_Bind_VO(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VO_LAYER VoLayer, VO_CHN VoChn);
SC_S32 SAMPLE_COMM_VPSS_UnBind_VO(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VO_LAYER VoLayer, VO_CHN VoChn);

SC_S32 SAMPLE_COMM_VDEC_InitVBPool(SC_U32 ChnNum, SAMPLE_VDEC_ATTR *pastSampleVdec);
SC_VOID SAMPLE_COMM_VDEC_ExitVBPool(SC_VOID);
SC_VOID SAMPLE_COMM_VDEC_CmdCtrl(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread);
SC_VOID SAMPLE_COMM_VDEC_StartSendStream(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread);
SC_VOID SAMPLE_COMM_VDEC_StopSendStream(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread);
SC_VOID *SAMPLE_COMM_VDEC_SendStream(SC_VOID *pArgs);
SC_VOID SAMPLE_COMM_VDEC_RPC_StartSendStream(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread);
SC_VOID SAMPLE_COMM_VDEC_RPC_StopSendStream(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecSend, pthread_t *pVdecThread);
SC_VOID SAMPLE_COMM_VDEC_StartGetPic(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecGet, pthread_t *pVdecThread);
SC_VOID SAMPLE_COMM_VDEC_StopGetPic(SC_S32 s32ChnNum, VDEC_THREAD_PARAM_S *pstVdecGet, pthread_t *pVdecThread);
SC_S32 SAMPLE_COMM_VDEC_Start(SC_S32 s32ChnNum, SAMPLE_VDEC_ATTR *pastSampleVdec);
SC_S32 SAMPLE_COMM_VDEC_Stop(SC_S32 s32ChnNum);
SC_S32 SAMPLE_COMM_VDEC_UnBind_VPSS(VDEC_CHN VdecChn, VPSS_GRP VpssGrp);
SC_S32 SAMPLE_COMM_VDEC_Bind_VPSS(VDEC_CHN VdecChn, VPSS_GRP VpssGrp);

SC_S32 SAMPLE_COMM_VPSS_Start(VPSS_GRP VpssGrp, SC_BOOL *pabChnEnable, VPSS_GRP_ATTR_S *pstVpssGrpAttr,
    VPSS_CHN_ATTR_S *pastVpssChnAttr);
SC_S32 SAMPLE_COMM_VI_Bind_VPSS(VI_PIPE ViPipe, VI_CHN ViChn, VPSS_GRP VpssGrp);
SC_S32 SAMPLE_COMM_VPSS_Stop(VPSS_GRP VpssGrp, SC_BOOL *pabChnEnable);
SC_S32 SAMPLE_COMM_VPSS_SetRes(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SIZE_S *pSize);
SC_S32 SAMPLE_COMM_VI_UnBind_VPSS(VI_PIPE ViPipe, VI_CHN ViChn, VPSS_GRP VpssGrp);
SC_S32 SAMPLE_COMM_VPSS_UnBind_VENC(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VENC_CHN VencChn);
SC_S32 SAMPLE_COMM_VPSS_Bind_VENC(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VENC_CHN VencChn);

SC_S32 SAMPLE_COMM_REGION_Create(SC_S32 HandleNum, RGN_TYPE_E enType);
SC_S32 SAMPLE_COMM_REGION_Destroy(SC_S32 HandleNum, RGN_TYPE_E enType);
SC_S32 SAMPLE_COMM_REGION_AttachToChn(SC_S32 HandleNum, RGN_TYPE_E enType, MPP_CHN_S *pstMppChn);
SC_S32 SAMPLE_COMM_REGION_DetachFrmChn(SC_S32 HandleNum, RGN_TYPE_E enType, MPP_CHN_S *pstMppChn);
SC_S32 SAMPLE_COMM_REGION_SetBitMap(RGN_HANDLE Handle);
SC_S32 SAMPLE_COMM_REGION_GetUpCanvas(RGN_HANDLE Handle);
SC_S32 SAMPLE_COMM_REGION_GetMinHandle(RGN_TYPE_E enType);
SC_S32 SAMPLE_COMM_REGION_GetUpCanvasEx(RGN_HANDLE Handle, char *bmpname);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __SAMPLE_COMMON_H__ */
